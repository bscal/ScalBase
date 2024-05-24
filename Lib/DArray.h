#pragma once

#include "Base.h"
#include "Arena.h"

template<typename T>
struct DArray
{
	T* Memory;
	u32 Capacity;
	u32 Count;

    void Resize(Arena* arena, u32 newCapacity)
    {
        SAssert(arena);

        if (newCapacity == 0)
        {
            newCapacity = 1;
        }

        if (newCapacity <= Capacity)
        {
            return;
        }

        SAssert(newCapacity > 0);

        T* newMemory = ArenaPush(sizeof(T), newCapacity);
        SAssert(newMemory);
        if (Memory)
        {
            SMemCopy(newMemory, Memory, Capacity * sizeof(T));
        }
        Memory = newMemory;
        Capacity = newCapacity;
    }

    void Push(const T* value)
    {
        SAssert(value);
        SAssert(Count <= Capacity);
        if (Count == Capacity)
        {
            SCAL_ERROR("DArray is full");
            return;
        }
        Memory[Count] = *valueSrc;
        ++Count;
    }

    void PushMany(u32 numberToAdd)
    {
        SAssert(numerToAdd > 0);

        u32 roomLeft = Capacity - Count;
        if (numberToAdd > roomLeft)
        {
            SCAL_ERROR("DArray is full");
            return;
        }
        SMemZero(Memory + Count, numberToAdd * sizeof(T));
        Count += numberToAdd;
    }

    void PushAt(const T& value, u32 idx)
    {
        SAssert(idx < Count);

        if (Count == Capacity)
        {
            SCAL_ERROR("DArray is full");
            return;
        }

        if (idx != LastIndex())
        {
            T* dst = Memory + idx + 1;
            T* src = Memory + idx;
            size_t sizeTillEnd = (Count - idx) * sizeof(T);
            SMemMove(dst, src, sizeTillEnd);
        }
        Memory[idx] = value;
        ++Count;
    }

    void PushSwapBack(const T& value, u32 idx)
    {
        SAssert(idx < Count);
        SAssert(Count <= Capacity);

        if (Count == Capacity)
        {
            SCAL_ERROR("DArray is full");
            return;
        }

        if (idx != LastIndex())
        {
            SMemMove(Memory + Count, Memory + idx, sizeof(T));
        }
        Memory[idx] = value;
        ++Count;
    }

    T Pop()
    {
        SAssert(Memory);
        SAssert(Count > 0);
        SAssert(Count <= Capacity);
        u32 lastIdx = LastIndex();
        if (lastIdx > 0)
            --Count;

        return Memory[lastIdx];
    }

    void Remove()
    {
        SAssert(Memory);
        SAssert(Count > 0);
        if (Count > 0)
            --Count;
    }

    void RemoveMany(u32 numberToRemove)
    {
        SAssert(Memory);
        SAssert(numberToRemove > 0);

        if (Count <= numberToRemove)
            Count = 0;
        else
            Count = Count - numberToRemove;
    }

    void RemoveAt(u32 idx)
    {
        SAssert(Memory);
        SAssert(Count > 0);
        --Count;
        if (idx != Count)
        {
            size_t dstOffset = idx * sizeof(T);
            size_t srcOffset = ((size_t)(idx) + 1) * sizeof(T);
            size_t sizeTillEnd = ((size_t)(Count) -(size_t)(idx)) * sizeof(T);
            char* mem = (char*)Memory;
            SMemMove(mem + dstOffset, mem + srcOffset, sizeTillEnd);
        }
    }

    void RemoveSwapBack(u32 idx)
    {
        SAssert(Memory);
        SAssert(Count > 0);
        SAssert(idx < Count);
        SMemCopy(Memory + idx, Memory + LastIndex(), sizeof(T));
        --Count;
    }

    T* At(u32 idx)
    {
        SAssert(Memory);
        SAssert(idx < Count);
        SAssert(idx < Capacity);
        return Memory + idx;
    }

    T* Last()
    {
        return At(LastIndex());
    }

    u32 LastIndex()
    {
        return (Count == 0) ? 0 : Count - 1;
    }

    void CopyToArray(T** other, u32 count)
    {
        SAssert(other);
        SAssert(count == Count);
        SAssert(Count > 0);

        T* dst = *other;
        SAssert(dst);
        if (dst && count == Count)
        {
            SMemCopy(dst, Memory, Count * sizeof(T));
        }
    }

    void Clear()
    {
        Count = 0;
    }
};

