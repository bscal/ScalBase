#pragma once

#include "Base.h"

struct Arena
{
	void* Memory;
	size_t ReservedSize;
	size_t Size;
	size_t TotalAllocated;
	size_t MinSize;
	size_t Alignment;
	int TempCount;
};

struct ThreadArena
{
	Arena A;
	Arena B;
	int Index;
	bool IsInitialized;	
};

inline bool MemoryInitialize();

inline Arena* GetScratch();

inline Arena ArenaCreate(size_t reserveSize, size_t initialCommitSize);

inline Arena ArenaCreateMin(size_t reserveSize, size_t initialCommitSize, size_t min);

inline void ArenaGrow(Arena* a, size_t sizeNeeded);

inline void ArenaShrink(Arena* a);

//! Retrieve memory arena's remaining size.
inline size_t ArenaSizeRemaining(Arena* arena);

struct ArenaSnapshot
{
	Arena* Arena;
	size_t OriginalTotalAllocated;
};

//! Capture a snapshot of used memory in a memory arena.
inline ArenaSnapshot ArenaSnapshotBegin(Arena* arena);

//! Reset memory arena's usage by a captured snapshot.
inline void ArenaSnapshotEnd(ArenaSnapshot snapshot);

inline void* ArenaPush(Arena* arena, size_t size);
inline void* ArenaPushZero(Arena* arena, size_t size);

#define ArenaPushArray(arena, type, count) (type*)ArenaPush((arena), sizeof(type)*(count))
#define ArenaPushArrayZero(arena, type, count) (type*)ArenaPushZero((arena), sizeof(type)*(count))
#define ArenaPushStruct(arena, type) ArenaPushArray(arena, type, 1)
#define ArenaPushStructZero(arena, type) ArenaPushArrayZero(arena, type, 1)

inline void ArenaPop(Arena* arena, size_t size);

// ************************************************************************************

inline thread_local ThreadArena ThreadScratchArena;

global_var Arena g_AppArena;
global_var Arena g_GameArena;
global_var ArenaSnapshot g_GameArenaSnapshot;
global_var Arena g_FrameArena;
global_var ArenaSnapshot g_FrameArenaSnapshot;

#define GetAppArena &g_AppArena
#define GetGameArena &g_GameArena
#define GetFrameArea &g_FrameArena

// ************************************************************************************

inline bool MemoryInitialize()
{
	size_t reserveSize = Gigabytes(1);
	size_t commitSize = Megabytes(1);

	g_AppArena = ArenaCreate(reserveSize, commitSize);

	g_GameArena = ArenaCreate(reserveSize , commitSize);

	g_FrameArena = ArenaCreate(reserveSize, commitSize);

	if (!g_AppArena.Memory || !g_GameArena.Memory || !g_FrameArena.Memory)
	{
		LogErr("MemoryInitialize error, %p / %p / %p", g_AppArena.Memory, g_GameArena.Memory,g_FrameArena.Memory);

		return false;
	}

	LogInfo("MemoryInitialize ok! \n  Memory: %p - %ull/%ull, \n  Memory: %p - %ull/%ull, \n  Memory: %p - %ull/%ull", g_AppArena.Memory, g_AppArena.Size, g_AppArena.ReservedSize, g_GameArena.Memory, g_GameArena.Size, g_GameArena.ReservedSize, g_FrameArena.Memory, g_FrameArena.Size, g_FrameArena.ReservedSize);

	return true;
}

inline Arena* GetScratch()
{
	if (!ThreadScratchArena.IsInitialized)
	{
		ThreadScratchArena.IsInitialized = true;

		size_t reserveSize = Megabytes(16);
		size_t commitSize = Megabytes(1);

		ThreadScratchArena.A = ArenaCreate(reserveSize, commitSize);
		ThreadScratchArena.B = ArenaCreate(reserveSize, commitSize);
	}

	if (ThreadScratchArena.Index)
	{
		ThreadScratchArena.Index = 0;
		return &ThreadScratchArena.B;
	}
	else
	{
		ThreadScratchArena.Index = 1;
		return &ThreadScratchArena.A;
	}
}

inline Arena ArenaCreateMin(size_t reserveSize, size_t initialCommitSize, size_t min)
{
	size_t pageSize = PlatformPageSize();

	reserveSize = AlignSize(reserveSize, pageSize);
	initialCommitSize = AlignSize(initialCommitSize, pageSize);
	min = AlignSize(Max(min, pageSize), pageSize);

	if (initialCommitSize < min)
	{
		initialCommitSize = min;
	}

	Arena result = {};
	result.Alignment = pageSize;
	result.MinSize = min;
	result.ReservedSize = reserveSize;
	result.Size = initialCommitSize;
	result.Memory = PlatformMemoryReserve(result.ReservedSize);

	if (!result.Memory)
	{
		SCAL_FATAL("Failed to reserve platform memory");
		return result;
	}

	if (result.Size > 0)
	{
		void* resultPtr = PlatformMemoryCommit(result.Memory, initialCommitSize);
		if (!resultPtr)
		{
			SCAL_FATAL("Commiting memory failed!");
			return result;
		}
	}

	return result;
}

inline Arena ArenaCreate(size_t reserveSize, size_t initialCommitSize)
{
	return ArenaCreateMin(reserveSize, initialCommitSize, 0);
}

inline void ArenaGrow(Arena* a, size_t sizeNeeded)
{
	SAssert(a);
	SAssert(a->Memory);
	SAssert(a->ReservedSize > 0);
	SAssert(sizeNeeded > 0);
	SAssert(AlignSize(sizeNeeded, a->Alignment) < a->ReservedSize);

	size_t newSize = AlignSize(sizeNeeded, a->Alignment);
	// Note: Docs say you are allowed to commit to already commited pages
	void* resultPtr = PlatformMemoryCommit(a->Memory, newSize);
	if (!resultPtr)
	{
		SCAL_ERROR("Arena failed to grow.");
	}
}

inline void ArenaShrink(Arena* a)
{
	SAssert(a);
	SAssert(a->Memory);

	if (a->MinSize)
	{
		size_t remaining = ArenaSizeRemaining(a);
		if (remaining > a->MinSize)
		{
			SAssert(a->Size % a->Alignment == 0);
			SAssert(a->MinSize % a->Alignment == 0);

			size_t sizeNeeded = AlignSize(a->TotalAllocated, a->Alignment) + a->MinSize;
			void* ptr = Cast(u8*, a->Memory) + sizeNeeded;
			size_t size = a->Size - sizeNeeded;
			PlatformMemoryUncommit(ptr, size);
		}
	}
}

//! Retrieve memory arena's remaining size.
inline size_t ArenaSizeRemaining(Arena* arena)
{
	SAssert(arena);
	SAssert(arena->Alignment > 0);
	size_t res = arena->Size - AlignSize(arena->TotalAllocated, arena->Alignment);
	return res;
}

//! Capture a snapshot of used memory in a memory arena.
inline ArenaSnapshot ArenaSnapshotBegin(Arena* arena)
{
	SAssert(arena);
	ArenaSnapshot snapshot;
	snapshot.Arena = arena;
	snapshot.OriginalTotalAllocated = arena->TotalAllocated;
	++arena->TempCount;
	return snapshot;
}

//! Reset memory arena's usage by a captured snapshot.
inline void ArenaSnapshotEnd(ArenaSnapshot snapshot)
{
	SAssert(snapshot.Arena->TotalAllocated >= snapshot.OriginalTotalAllocated);
	SAssert(snapshot.Arena->TempCount > 0);
	snapshot.Arena->TotalAllocated = snapshot.OriginalTotalAllocated;
	--snapshot.Arena->TempCount;
}

inline void* ArenaPush(Arena* arena, size_t size)
{
	SAssert(arena);
	SAssert(size > 0);

	void* res;
	size_t totalSize = AlignSize(size, SCAL_CACHE_LINE);
	size_t sizeNeeded = arena->TotalAllocated + totalSize;

	if (sizeNeeded > arena->Size)
	{
		if (sizeNeeded > arena->ReservedSize)
		{
			SCAL_FATAL("Arena out of memory!");
			return nullptr;
		}
		else
		{
			ArenaGrow(arena, sizeNeeded);
		}
	}

	res = (void*)((size_t)arena->Memory + arena->TotalAllocated);
	arena->TotalAllocated = sizeNeeded;
	SAssert(res);
	return res;
}

inline void* ArenaPushZero(Arena* arena, size_t size)
{
	void* res = ArenaPush(arena, size);
	SMemZero(res, AlignSize(size, SCAL_CACHE_LINE));
	return res;
}

inline void ArenaPop(Arena* arena, size_t size)
{
	SAssert(arena);
	SAssert(arena->TotalAllocated > 0);
	SAssert(size);
	arena->TotalAllocated -= AlignSize(size, SCAL_CACHE_LINE);
}

#define SALLOCATOR_ARENA(arena) (SAllocator{ SAllocatorArena, arena })

// Only supports malloc operations, realloc will error, free will do nothing
inline SALLOCATOR_ALLOCATOR(SAllocatorArena)
{
	Arena* arena = Cast(Arena*, userData);

	switch (type)
	{
		case (SALLOCATOR_TYPE_MALLOC):
		{
			SAssert(size > 0);
			SAssert(!ptr);

			if (!ptr)
			{
				size_t alignedSize = AlignSize(size, SCAL_CACHE_LINE);
				ptr = ArenaPush(arena, alignedSize);
			}
			else
			{
				SCAL_ERROR("Arena allocator called malloc with non null ptr");
			}

			SAssert(ptr);
		} break;
		case (SALLOCATOR_TYPE_REALLOC):
		{
			SCAL_ERROR("Arena allocator used realloc");
		} break;
		case (SALLOCATOR_TYPE_FREE):
		{
		} break;

		default:
		{
			SCAL_ERROR("SAllocatorType is not valid");
			return nullptr;
		};
	};

	return ptr;
}
