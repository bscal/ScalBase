#pragma once

#include "Base.h"
#include "String.h"

struct HashStrSlot
{
	String String;
	u64 Hash;
	u16 ProbeLength;
	bool IsUsed;
};

struct StrHashMapAddResult
{
	u32 Index;
	bool DoesExist;
};

// Robin hood hashmap seperating Slots (probelength, tombstone), Keys, Values
template<typename V>
struct StringHashMap
{
	constant_var u32 NOT_FOUND = UINT32_MAX;
	constant_var float LOAD_FACTOR = 0.85f;

	HashStrSlot* Buckets;
	V* Values;
	u32 Capacity;
	u32 Count;
	u32 MaxCount;

	void Resize(SAllocator a, u32 capacity)
	{
		if (capacity <= 1)
		{
			capacity = 2;
		}

		if (!IsPowerOf2(capacity))
		{
			capacity = AlignPowTwoCeil32(capacity);
		}

		if (capacity <= Capacity)
		{
			return;
		}

		if (Buckets)
		{
			SAssert(Values);

			StringHashMap<V> tmpMap = {};
			tmpMap.Resize(a, capacity);

			for (u32 i = 0; i < Capacity; ++i)
			{
				if (Buckets[i].IsUsed)
				{
					tmpMap.Add(&Buckets[i].String, &Values[i]);
				}
			}

			SAssert(tmpMap.Count == Count);

			SAllocatorFree(a, Buckets);
			SAllocatorFree(a, Values);

			*this = tmpMap;
		}
		else
		{
			Buckets = Cast(HashStrSlot*, SAllocatorAlloc(a, sizeof(HashStrSlot) * capacity));
			SMemZero(Buckets, sizeof(HashStrSlot) * capacity);
			Values = Cast(V*, SAllocatorAlloc(a, sizeof(V) * capacity));
		}

		Capacity = capacity;
		MaxCount = (u32)(((float)Capacity) * LOAD_FACTOR);

		SAssert(Buckets);
		SAssert(Values);
		SAssert(MaxCount > 0);
	}

	// key pointer is copied directly. You need to manage it's lifetime!
	// value is copied into the array.
	StrHashMapAddResult Add(const String* key, const V* value)
	{
		SAssert(key);
		SAssert(value);

		if (Count == MaxCount)
		{
			SCAL_ERROR("HashMap is full!");
			return { NOT_FOUND, false };
		}

		HashStrSlot swapBucket;
		swapBucket.String = *key;
		swapBucket.Hash = HashString(*key);
		swapBucket.IsUsed = true;
		swapBucket.ProbeLength = 0;

		V swapValue = *value;

		u32 insertedIdx = NOT_FOUND;
		u16 probeLength = 0;
		u32 idx = (u32)FastModulo(swapBucket.Hash, Capacity);
		while (true)
		{
			HashStrSlot* slot = &Buckets[idx];
			if (!slot->IsUsed) // Bucket is not used
			{
				// Found an open spot. Insert and stops searching
				*slot = swapBucket;
				slot->ProbeLength = probeLength;
				Values[idx] = swapValue;
				++Count;

				if (insertedIdx == NOT_FOUND)
					insertedIdx = idx;

				return { insertedIdx, false };
			}
			else
			{
				if (slot->Hash == swapBucket.Hash && StringAreEqual(swapBucket.String, slot->String))
					return { idx, true };

				if (probeLength > slot->ProbeLength)
				{
					if (insertedIdx == NOT_FOUND)
						insertedIdx = idx;

					// Swap probe lengths and buckets
					Swap(slot->ProbeLength, probeLength);
					Swap(swapBucket, *slot);
					Swap(swapValue, Values[idx]);
				}

				++probeLength;
				++idx;
				if (idx == Capacity)
					idx = 0;

				SAssertMsg(probeLength <= UINT16_MAX, "probeLength is > UINT16_MAX, using bad hash?");
			}
		}
		SERR_INVALID_PATH();
		return {};
	}

	bool Replace(String key, const V* value)
	{
		StrHashMapAddResult res = Add(key, value);
		if (res.DoesExist)
		{
			SAssert(res.Index != NOT_FOUND);
			Values[res.Index] = *value;
			return true;
		}
		return false;
	}

		StrHashMapAddResult AddAllocator(SAllocator a, String key, const V* val)
	{
		if (Count == MaxCount)
		{
			Resize(a, Capacity * 2);
		}

		Add(key, val);
	}

	u32 ReplaceAllocator(SAllocator a, String key, const V* val)
	{
		if (Count == MaxCount)
		{
			Resize(a, Capacity * 2);
		}

		Replace(key, val);
	}

	V* Get(const String key)
	{
		SAssert(key);

		if (Count == 0)
		{
			return nullptr;
		}

		u32 probeLength = 0;
		u64 hash = HashString(key);
		u32 idx = (u32)FastModulo(hash, Capacity);
		while (true)
		{
			HashStrSlot slot = Buckets[idx];
			if (!slot.IsUsed || probeLength > slot.ProbeLength)
				return nullptr;
			else if (slot.Hash == hash && StringAreEqual(key, slot.String))
				return &Values[idx];
			else
			{
				++probeLength;
				++idx;
				if (idx == Capacity)
					idx = 0;
			}
		}
		SERR_INVALID_PATH();
		return nullptr;
	}

	void Clear()
	{
		ClearAllocator(SALLOCATOR_ARENA(null));
	}

	// Needs allocator to handle freeing of strings
	void ClearAllocator(SAllocator strAllocator)
	{
		SAssert(strAllocator.Allocator);

		for (u32 i = 0; i < Capacity; ++i)
		{
			SAllocatorFree(strAllocator, Buckets[i].String);
			Buckets[i].ProbeLength = 0;
			Buckets[i].IsUsed = 0;
		}
		Count = 0;
	}

	bool Remove(const String key)
	{
		return RemoveAllocator(SALLOCATOR_ARENA(nullptr), key);
	}

	// Needs allocator to handle freeing of removed string
	bool RemoveAllocator(SAllocator strAllocator, const String key)
	{
		SAssert(strAllocator.Allocator);
		SAssert(key);

		if (Count == 0)
			return false;

		u64 hash = HashString(key);
		u32 idx = (u32)FastModulo(hash, Capacity);
		while (true)
		{
			HashStrSlot slot = Buckets[idx];
			if (!slot.IsUsed)
			{
				return false;
			}
			else if (slot.Hash == hash && StringAreEqual(key, slot.String))
			{
				SAllocatorFree(strAllocator, slot.String);

				while (true) // Move any entries after index closer to their ideal probe length.
				{
					uint32_t lastIdx = idx;
					++idx;
					if (idx == Capacity)
						idx = 0;

					HashStrSlot nextSlot = Buckets[idx];
					if (!nextSlot.IsUsed || nextSlot.ProbeLength == 0) // No more entires to move
					{
						Buckets[lastIdx] = {};
						--Count;
						return true;
					}
					else
					{
						Buckets[lastIdx].String = nextSlot.String;
						Buckets[lastIdx].Hash = nextSlot.Hash;
						Buckets[lastIdx].ProbeLength = nextSlot.ProbeLength - 1;
						Values[lastIdx] = Values[idx];
					}
				}
			}
			else
			{
				++idx;
				if (idx == Capacity)
					idx = 0; // continue searching till 0 or found equals key
			}
		}
		SERR_INVALID_PATH();
		return false;
	}

	void Print(const char*(*FmtValue)(V*))
	{
		SAssert(FmtValue);

		LogInfo("[ Printing StringMap ]");
		LogInfo("Cap %u, Count %u, MaxSize %u", Capacity, Count, MaxCount);

		for (u32 i = 0; i < Capacity; ++i)
		{
			if (Buckets[i].IsUsed)
			{
				LogInfo("\t[%d] %s (probe: %d) = %s",
					i,
					Buckets[i].String,
					Buckets[i].ProbeLength,
					FmtValue(&Values[i]));
			}
			else
			{
				LogInfo("\t[%d] NULL", i);
			}
		}
	}

};

namespace StringHashMaps
{
	#if SCAL_TESTS
	inline int Test(Arena* arena)
	{
		StringHashMap<u64> map = {};

		SAllocator a = SALLOCATOR_ARENA(arena);

		map.Resize(a, 1);

		SAssert(map.Capacity == 2);
		SAssert(map.MaxCount == 1);

		String k0 = StringMake(arena, "Hello World!");
		u64 v0 = 1;
		map.Add(&k0, &v0);
		
		map.Resize(a, 4);
		
		String k1 = StringMake(arena, "namespace:this");
		u64 v1 = 2;
		map.Add(&k1, &v1);

		String k2 = StringMake(arena, "Demon of Ell here to destroy");
		u64 v2 = 12345;
		map.Add(&k2, &v2);

		SAssert(map.Capacity == 4);
		SAssert(map.MaxCount == 3);
		SAssert(map.Count == 3);

		u64* r0 = map.Get(k0);
		SAssert(*r0 == v0);

		String k3 = StringMakeReserve(arena, 32);
		StringAppend(k3, "Pushing");
		StringAppend(k3, " A ");
		StringAppend(k3, "string!");
		u64 v3 = 1111;

		map.Resize(a, 32);

		map.Add(&k3, &v3);

		SAssert(map.Capacity == 32);
		SAssert(map.MaxCount == 27);
		SAssert(map.Count == 4);

		StrHashMapAddResult res = map.Add(&k3, &v0);
		SAssert(res.DoesExist);
		SAssert(map.Count == 4);

		u64* r1 = map.Get(k1);
		SAssert(*r1 == v1);
		u64* r3 = map.Get(k3);
		SAssert(*r3 == v3);

		bool wasRemoved = map.Remove(k2);
		SAssert(wasRemoved);

		for (int i = 0; i < 16; ++i)
		{
			String k = StringMake(arena, TextFormat("%d y %d", i * i, 6 * i * (i + 1)));
			u64 v = i;
			map.Add(&k, &v);
		}

		for (int i = 0; i < 5; ++i)
		{
			String k = StringMake(arena, TextFormat("%d long message used. %d, %d.", 10000 * i * (i + 500), i + i * 256, 9000 * i));
			u64 v = i;
			map.Add(&k, &v);
		}
		
		SAssert(map.Capacity == 32);
		SAssert(map.MaxCount == 27);
		SAssert(map.Count == 3 + 21);

		u64* r11 = map.Get(k1);
		SAssert(*r11 == v1);
		u64* r33 = map.Get(k3);
		SAssert(*r33 == v3);

		map.Print([](u64* val) { return TextFormat("%u", *val); });

		return 0;
	}
#endif
}

