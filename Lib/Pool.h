#pragma once

#include "Base.h"
#include "StaticArray.h"
#include "Arena.h"

template<typename T>
struct Pool
{
    union PoolType
    {
        T Type;
        PoolType* Next;
    };

    PoolType* Data;
    PoolType* FreeList;
    size_t Count;
    size_t Capacity;

    void Init(Arena* arena, size_t capacity)
    {
        SAssert(arena);
        SAssert(capacity > 0);

        SAssert(!Data);
        SAssert(Count == 0);

        Capacity = capacity;
        Data = ArenaPushArrayZero(arena, PoolType, capacity);

        Reset();
    }

    void Reset()
    {
        for (size_t i = 0; i < Capacity; ++i)
        {
            PoolType* ptr = &Data[i];
            SLStackPush(FreeList, ptr, Next);
        }
    }

    T* Get()
    {
        if (Count == Capacity)
        {
            SCAL_ERROR("Pool capacity reached!");
            return res;
        }

        T* res = Cast(T*, FreeList);
        SLStackPop(FreeList, Next);

        SAssert(res);
        
        ++Count;
        return res;
    }

    void Return(T* ptr)
    {
        SAssert(ptr);
        SAssert(Count < Capacity);
        SAssert(IsPointerInPool(ptr));

        PoolType* poolType = Cast(PoolType*, ptr);
        SLStackPush(FreeList, poolType, Next);
        --Count;
    }

    bool IsPointerInPool(T* ptr)
    {
        if (!ptr) 
            return false;

        return ptr >= Data && ptr < (Data + Capacity);
    }
};
