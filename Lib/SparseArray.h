#pragma once

#include "Base.h"
#include "Arena.h"

template<typename T>
struct SparseSet
{
	typedef u16 SparseSetSize_t;

	constant_var SparseSetSize_t EMPTY = 0xffff;

	SparseSetSize_t* Sparse;
	SparseSetSize_t* DenseLookup;
	T* Dense;
	SparseSetSize_t Capacity;
	SparseSetSize_t Count;

	void Init(Arena* arena, u32 capacity)
	{
        SAssert(sparseArrayBuffer);
        SAssert(denseArrayBuffer);
        SAssert(Capacity > 0);

        Sparse = ArenaPushArrayZero(Arena, SparseSetSize_t, capacity);
		DenseLookup = ArenaPushArrayZero(Arena, SparseSetSize_t, capacity);
		Dense = ArenaPushArrayZero(Arena, T, capacity);
        Capacity = capacity;

		for (int i = 0; i < Capacity; ++i)
		{
			Sparse[i] = EMPTY;
		}
	}

	T* Add(SparseSetSize_t id, const T* value)
	{
        SAssert(Dense);
        SAssert(Sparse);
        SAssert(Capacity > 0);

        if (Count == SparseCapacity)
        {
            SCAL_ERROR("SparseSet is full!");
            return EMPTY;
        }

        SparseSetSize_t idx = Count;
        ++Count;

		Sparse[id] = idx;
		DenseLookup[idx] = id;
		Dense[idx] = *value;

		return Dense + idx;
	}

	void Remove(SparseSetSize_t id)
	{
        SAssert(Dense);
        SAssert(Sparse);
        SAssert(Capacity > 0);
        
        SparseSetSize_t idx = Sparse[id];

        if (idx == EMPTY)
        {
            return;
        }

        SAssert(Count > 0);

		Sparse[id] = EMPTY;
		DenseLookup[idx] = DenseLookup[Count - 1];
		Dense[idx] = Dense[Count - 1];

		SparseSetSize_t lastId = DenseLookup[Count - 1];
		Sparse[lastId] = idx;

		--Count;
	}

	T* Get(SparseSetSize_t id)
	{
        SAssert(Dense);
        SAssert(Sparse);
        SAssert(Capacity > 0);

		SparseSetSize_t idx = Sparse[id];

		if (idx != EMPTY && idx < Count)
        {
			return &Dense[idx];
        }
		else
        {
            return nullptr;
        }
	}
};