#pragma once

#include "Base.h"
/*
#define HASHMAP_USE_64_HASH 0
#if HASHMAP_USE_64_HASH
	typedef u64 HashmapHashcode;
	#define HASHMAP_HASH(val, cap) (HashAndMod((val), (cap)))
#else
	typedef u32 HashmapHashcode;
	#define HASHMAP_HASH(val, cap) (u32)(HashAndMod(val, cap))
#endif
*/
template<typename K, typename V>
struct StaticHashMapBucket
{
	K Key;
	u16 ProbeLength;
	bool IsUsed;
	V Value;
};

struct StaticHashMapAddResult
{
	u32 Index;
	bool DoesExist;
	bool IsMapFull;
};

template<typename K, typename V>
struct StaticHashMap
{
	constant_var u32 NOT_FOUND = UINT32_MAX;

	StaticHashMapBucket<K, V>* Buckets;
	u32 Capacity;
	u32 Count;
	u32 MaxCount;

	// void Init(void* memory, size_t memorySize, u32 capacity, float loadFactor);

	// StaticHashMapAddResult Add(const K* key, const V* value);

	// bool Replace(const K* key, const V* value);

	// V* Get(const K* key);

	// bool Remove(const K* key);

	// void Clear();

	////////////////////////////////////////////////////////////

	constexpr size_t GetMemSize(u32 capacity)
	{
		size_t result;

		if (!IsPowerOf2(capacity))
		{
			capacity = AlignPowTwoCeil32(capacity);
		}

		result = sizeof(StaticHashMapBucket<K, V>) * capacity;

		return result;
	}

	void Init(void* memory, size_t memorySize, u32 capacity, float loadFactor)
	{
		SAssert(memory);
		SAssert(memorySize > 0);
		SAssert(memorySize >= sizeof(StaticHashMapBucket<K, V>))
		SAssert(capacity > 0);
		SAssert(loadFactor > 0.0f);
		SAssert(loadFactor <= 1.0f);

		if (!IsPowerOf2(capacity))
		{
			capacity = AlignPowTwoCeil32(capacity);
		}
		SAssert(memorySize / sizeof(StaticHashMapBucket<K, V>) >= capacity);

		Buckets = (StaticHashMapBucket<K, V>*)memory;
		Capacity = capacity;
		Count = 0;
		MaxCount = Cast(u32, Cast(float, Capacity) * loadFactor);

		SAssert(MaxCount > 0);
	}

	StaticHashMapAddResult Add(const K* key, const V* value)
	{
		SAssert(key);
		SAssert(value);
		SAssert(*key == *key);

		if (Count == MaxCount)
		{
			SCAL_ERROR("HashMap is full!");
			return { NOT_FOUND, false, true };
		}

		StaticHashMapBucket<K, V> swapBucket;
		swapBucket.Key = *key;
		swapBucket.ProbeLength = 0;
		swapBucket.IsUsed = true;
		swapBucket.Value = *value;

		u32 insertedIdx = NOT_FOUND;
		u16 probeLength = 0;
		u32 idx = (u32)HashAndMod(key, Capacity);
		while (true)
		{
			StaticHashMapBucket<K, V>* bucket = &Buckets[idx];
			if (!bucket->IsUsed) // Bucket is not used
			{
				if (insertedIdx == NOT_FOUND)
					insertedIdx = idx;

				// Found an open spot. Insert and stops searching
				*bucket = swapBucket;
				bucket->ProbeLength = probeLength;
				++Count;

				return { insertedIdx, false, false };
			}
			else
			{
				if (bucket->Key == swapBucket.Key)
				{
					return { idx, true, false };
				}

				if (probeLength > bucket->ProbeLength)
				{
					if (insertedIdx == NOT_FOUND)
						insertedIdx = idx;

					// Swap probe lengths and buckets
					swapBucket.ProbeLength = probeLength;
					probeLength = bucket->ProbeLength;
					StaticHashMapBucket<K, V> tmp = *bucket;
					*bucket = swapBucket;
					swapBucket = tmp;
				}

				++probeLength;
				++idx;
				if (idx == Capacity)
					idx = 0;
			}
		}
		SERR_INVALID_PATH();
		return {};
	}

	bool Replace(const K* key, const V* value)
	{
		StaticHashMapAddResult res = Add(key, value);
		if (res.DoesExist)
		{
			SAssert(res.Index != NOT_FOUND);
			Buckets[res.Index] = value;
			return true;
		}
		return false;
	}

	V* Get(const K* key)
	{
		SAssert(key);

		if (Count == 0)
		{
			return nullptr;
		}

		u32 probeLength = 0;
		u32 idx = (u32)HashAndMod(key, Capacity);
		while (true)
		{
			StaticHashMapBucket<K, V>* bucket = &Buckets[idx];
			if (!bucket->IsUsed || probeLength > bucket->ProbeLength)
			{
				return nullptr;
			}
			else if (*key == bucket->Key)
			{
				return &bucket->Value;
			}
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
		for (u32 i = 0; i < map->Capacity; ++i)
		{
			Buckets[i].ProbeLength = 0;
			Buckets[i].IsUsed = 0;
		}
		Count = 0;
	}

	bool Remove(const K* key)
	{
		SAssert(key);

		if (Count == 0)
			return false;

		u32 index = (u32)HashAndMod(key, Capacity);
		while (true)
		{
			StaticHashMapBucket<K, V>* bucket = &Buckets[index];
			if (!bucket->IsUsed)
			{
				return false; // No key found
			}
			else
			{
				if (*key == bucket->Key)
				{
					while (true) // Move any entries after index closer to their ideal probe length.
					{
						u32 lastIndex = index;
						if (++index == Capacity)
							index = 0;

						StaticHashMapBucket<K, V>* nextBucket = &Buckets[index];
						if (!nextBucket->IsUsed || nextBucket->ProbeLength == 0) // No more entires to move
						{
							Buckets[lastIndex].ProbeLength = 0;
							Buckets[lastIndex].IsUsed = false;
							--Count;
							return true;
						}
						else
						{
							--nextBucket->ProbeLength;
							Buckets[lastIndex] = *nextBucket;
						}
					}
				}
				else
				{
					++index;
					if (index == Capacity)
						index = 0; // continue searching till 0 or found equals key
				}
			}
		}
		SERR_INVALID_PATH();
		return false;
	}

	void Print(const char*(*FmtKey)(K*), const char*(*FmtValue)(V*))
	{
		SAssert(FmtKey);
		SAssert(FmtValue);

		LogInfo("[ Printing Map ]");
		LogInfo("Cap %u, Count %u, MaxSize %u", Capacity, Count, MaxCount);

		for (u32 i = 0; i < Capacity; ++i)
		{
			if (Buckets[i].IsUsed)
			{
				LogInfo("\t[%d] %s (probe: %d) = %s",
					i,
					FmtKey(&Buckets[i].Key),
					Buckets[i].ProbeLength,
					FmtValue(&Buckets[i].Value));
			}
			else
			{
				LogInfo("\t[%d] NULL", i);
			}
		}
	}

};

template<typename K, typename V>
struct DHashMap : public StaticHashMap<K, V>
{
	constant_var float LOAD_FACTOR = 0.85f;

	SAllocator Allocator;

	void Resize(SAllocator allocator, u32 capacity)
	{
		if (capacity == 0)
		{
			capacity = 2;
		}

		if (!IsPowerOf2(capacity))
		{
			capacity = AlignPowTwoCeil(capacity);
		}

		if (capacity <= Capacity)
		{
			return;
		}

		SAllocator allocator = (Allocator.Allocate) ? Allocator : SAllocatorDefault;

		size_t size = capacity * sizeof(StaticHashMapBucket<K, V>);
		void* memory = allocator.Allocate(nullptr, 0, size);

		if (Buckets)
		{
			StaticHashMap<K, V> tempMap = {};
			
			tempMap.Init(memory, size, capacity, LoadFactor);

			for (int i = 0; i < Capacity; ++i)
			{
				if (Buckets[i].IsUsed)
				{
					tempMap.Add(Buckets[i].Key, Buckets[i].Value);
				}
			}

			SAssert(tempMap.Count == Map.Count);

			size_t oldSize = Capacity * sizeof(StaticHashMapBucket<K, V>);
			allocator.Free(Buckets, oldSize);

			Map = tempMap;
		}
		else
		{
			Init(memory, size, capacity, LOAD_FACTOR);
		}
	}

	u32 AddResize(const K* key, const V* val)
	{
		SAssert(key);
		SAssert(val);

		if (Capacity == MaxCount)
		{
			Resize(Allocator, Capacity * 2);
		}

		return Add(key, val).Index;
	}

	bool ReplaceResize(const K* key, const V* val)
	{
		SAssert(key);
		SAssert(val);

		if (Capacity == MaxCount)
		{
			Resize(Allocator, Capacity * 2);
		}

		return Replace(key, val);
	}
};

namespace HashMaps
{
	#if SCAL_TESTS
	inline int Test(Arena* testArena)
	{
		StaticHashMap<int, int> map = {};
    
		size_t mapSize = map.GetMemSize(48);
		void* memory = ArenaPush(testArena, mapSize);
		map.Init(memory, mapSize, 48, .85f);

		SAssert(map.Buckets);
		SAssert(map.Capacity == 64);

		int x = 5;
		int y = 1;
		map.Add(&x, &y);

		map.Add(&y, &x);

		SAssert(map.Count == 2);

		int* r = map.Get(&x);
		SAssert(*r == y);

		LogInfo("r = %d", *r);

		int* rr = map.Get(&y);
		
		LogInfo("rr = %d", *rr);

		int f = 0;
		int* rrr = map.Get(&f);
		LogInfo((rrr) ? "NOT NULL" : "NULL");

		for (int i = 0; i < 48; ++i)
		{
			int ik = i;
			int iv = i + 256;
			map.Add(&ik, &iv);
		}

		SAssert(map.Count == (48));

		int remove = 15;
		bool wasRemoved = map.Remove(&remove);
		SAssert(wasRemoved);
		SAssert(map.Count == (48 - 1));

		int getk = 11;
		int* getv = map.Get(&getk);
		SAssert(*getv == (256 + 11));
		LogInfo("%d", *getv);

		map.Print([](int* k){ return TextFormat("%d", *k); }, [](int* v){ return TextFormat("%d", *v);});

		return 1;
	}
#endif
}

