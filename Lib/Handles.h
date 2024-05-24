#pragma once

#include "Base.h"
#include "Arena.h"
#include "Pool.h"
#include "DArray.h"

namespace Handles
{
	constant_var u32 EMPTY = 0;
}

#define SCAL_BIG_HANDLE 0
#if SCAL_BIG_HANDLE
union Handle
{
	u64 Value;
	struct
	{
		u32 Id;
		u32 Gen;
	};
};
#else
union Handle
{
	u32 Value;
	struct
	{
		u32 Id : 23;
		u32 Alive : 1;
		u32 Gen : 8;
	};
};
#endif

// Handle -> Dense

struct HandlesPool
{
	u32 Count;
	u32 Capacity;
	Array(u32) Sparse;
	Array(Handle) Dense;

	Handle NewHandle()
	{	
		SAssert(Sparse);
		SAssert(Dense);

		if (Count == Capacity)
		{
			SCAL_ERROR("Handles has reached capacity");
			return {};
		}

		u32 idx = Count;
		++Count;

		Handle res = Dense[idx];
		res.Alive = true;

		Sparse[res.Id] = idx;

		return res;
	}

	void ReleaseHandle(Handle handle)
	{
		SAssert(Sparse);
		SAssert(Dense);
		SAssert(Handle != 0);

		if (DoesHandleExist(handle))
		{
			u32 idx = Sparse[handle.Id];

			Handle lastHandle = Dense[Count - 1];

			Sparse[lastHandle.Id] = idx;
			Dense[idx] = lastHandle;
			Dense[Count - 1].Id = handle.Id;
			Dense[Count - 1].Gen = handle.Gen + 1;
			Dense[Count - 1].Alive = false;
		}
	}

	bool DoesHandleExist(Handle handle)
	{
		if (handle.Id >= Capacity || !handle.Alive)
		{
			return false;
		}

		u32 idx = Sparse[handle.Id];
		return Dense[idx].Value == handle.Value;
	}
};

HandlesPool HandlesPoolCreate(Arena* arena, u32 capacity)
{
    SAssert(arena);
    SAssert(capacity > 0);

    HandlesPool res = {};

    res.Capacity = capacity;
	res.Sparse = ArenaPushArrayZero(arena, u32, capacity);
	res.Dense = ArenaPushArrayZero(arena, Handle, capacity);

	for (u32 i = 0; i < res.Capacity - 1; ++i)
	{
		res.Dense[i].Id = i;
	}

	return res;
}
