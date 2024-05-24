#pragma once

#include "Base.h"
#include "Arena.h"
#include "DArray.h"

template<typename T, size_t ChunkSize>
struct LinkedArray
{
    template<typename T>
    struct LinkedArrayChunk
    {
        LinkedArrayChunk* Next;
        LinkedArrayChunk* Prev;
        DArray<T> Array;
    }

    Arena* Arena;
    LinkedArrayChunk<T>* First;
    LinkedArrayChunk<T>* Cur;
    u32 NumOfChunks;
    u32 Count;

    void Add(const T* value)
    {
        if (Cur->Array.Count == Cur->Array.Capacity)
        {
            if (Cur->Next)
            {
                Cur = Cur->Next;
                Add(value);
            }
        }
        else
        {
            Cur->Array.Push(value);
            ++Count;
        }
    }

    void AddAtSwap(const T* value, u32 idx)
    {
        u32 chunkIdx = idx / ChunkSize;
        u32 elementIdx = idx % ChunkSize;

        LinkedArrayChunk<T>* chunk = Cur;
        for (u32 i = 0; i < chunkIdx; ++i)
        {
            chunk = chunk->Next;
        }

        T* element = chunk->Array.At(elementIdx);
        T* lastElement = Cur->Array.Last();

        if (element != lastElement)
        {
            SMemCopy(lastElement, element, sizeof(T));
            ++Cur->Array.Count;
            --chunk->Array.Count;
        }

        ++chunk->Array.Count;
        chunk->Array.Memory[elementIdx] = value;
    }

    void Remove()
    {

    }

    void RemoveAt(u32 idx)
    {

    }

    T* At(u32 idx)
    {

    }

    LinkedArrayChunk* AllocateChunk(u32 size)
    {
        LinkedArrayChunk<T>* chunk = ArenaPushStructZero(Arena, LinkedArrayChunk<T>);
        chunk->Array.Resize(Arena, size);

        DLPushBackT(First, Cur, chunk, Next, Prev);

        ++NumOfChunks;

        return chunk;
    }

};
