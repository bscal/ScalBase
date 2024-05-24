#pragma once

#include "Base.h"
#include "Arena.h"

#define BHEAP_COMPARE_FUNC(name) int name(void* v0, void* v1)
typedef BHEAP_COMPARE_FUNC(BHeapCompareFunc);

struct BHeapItem
{
    void* Key;
    void* User;
};

struct BHeap
{
    BHeapCompareFunc* CompareFunc;
    BHeapItem* Items;
    int Count;
    int Capacity;

    static void HeapifyMin(BHeap* bh, int index);

    static void HeapifyMax(BHeap* bh, int index);

    void Init(Arena* arena, int capacity, BHeapCompareFunc compareFunc)
    {
        SAssert(arena);
        SAssert(capacity > 0);
        SAssert(compareFunc);

        Items = ArenaPushArray(arena, BHeapItem, capacity);
        Count = 0;
        Capacity = capacity;
        CompareFunc = compareFunc;
    }

    void PushMin(void* key, void* user)
    {
        SAssert(key);
        SAssert(CompareFunc);
        SAssertMsg(Count < Capacity, "BinaryHeap's capacity exceeded");

        if (Count == Capacity)
        {
            LogErr("BinaryHeap's capacity exceeded");
            return;
        }

        // Put the value at the bottom the tree and traverse up
        int index = Count;
        Items[index].Key = key;
        Items[index].User = user;

        while (index > 0) 
        {
            int parent_idx = (index - 1) >> 1;
            BHeapItem cur = Items[index];
            BHeapItem parent = Items[parent_idx];

            //if (cur.key < parent.key)
            int compare = CompareFunc(cur.Key, parent.Key);
            if (compare < 0)
            {
                Items[index] = parent;
                Items[parent_idx] = cur;
            } 
            else
            {
                break;
            }

            index = parent_idx;
        }
        ++Count;
    }

    BHeapItem PopMin()
    {
        SAssert(Count > 0);

        // Root is the value we want
        BHeapItem result = Items[0];

        // put the last one on the root and heapify
        int last = Count - 1;
        Items[0] = Items[last];
        Count = last;

        HeapifyMin(this, 0);
        return result;
    }

    void PushMax(void* key, void* user)
    {
        SAssert(key);
        SAssert(CompareFunc);
        SAssertMsg(Count < Capacity, "BinaryHeap's capacity exceeded");

        if (Count == Capacity)
        {
            LogErr("BinaryHeap's capacity exceeded");
            return;
        }

        // Put the value at the bottom the tree and traverse up
        int index = Count;
        Items[index].Key = key;
        Items[index].User = user;

        while (index > 0)
        {
            int parentIdx = (index - 1) >> 1;
            BHeapItem cur = Items[index];
            BHeapItem parent = Items[parentIdx];

            //if (cur.key > parent.key)
            int compare = CompareFunc(cur.Key, parent.Key);
            if (compare >= 0)
            {
                Items[index] = parent;
                Items[parentIdx] = cur;
            } 
            else
            {
                break;
            }

            index = parentIdx;
        }
        ++Count;
    }

    BHeapItem PopMax()
    {
        SAssert(Count > 0);

        if (Count <= 0)
        {
            return BHeapItem{ nullptr, nullptr };
        }

        // Root is the value we want
        BHeapItem result = Items[0];

        // put the last one on the root and heapify
        int last = Count - 1;
        Items[0] = Items[last];
        Count = last;

        HeapifyMax(this, 0);
        return result;
    }

    void BHeapClear()
    {
        Count = 0;
    }

    bool BHeapEmpty()
    {
        return Count == 0;
    }

        static void HeapifyMin(BHeap* bh, int index)
    {
        int _2i = 2 * index;
        int _min = index;
        int count = bh->Count;
        BHeapItem min_item = bh->Items[index];

        while (_2i + 1 < count)
        {
            int left = _2i + 1;
            int right = _2i + 2;

            if (bh->Items[left].Key < bh->Items[_min].Key)
                _min = left;
            if (right < count && bh->Items[right].Key < bh->Items[_min].Key)
                _min = right;
            if (_min == index)
                break;

            bh->Items[index] = bh->Items[_min];
            bh->Items[_min] = min_item;
            index = _min;
            _2i = 2 * index;
        }
    }

    static void HeapifyMax(BHeap* bh, int index)
    {
        int _2i = 2 * index;
        int _max = index;
        int count = bh->Count;
        BHeapItem max_item = bh->Items[index];

        while (_2i + 1 < count)
        {
            int left = _2i + 1;
            int right = _2i + 2;

            if (bh->Items[left].Key > bh->Items[_max].Key)
                _max = left;
            if (right < count && bh->Items[right].Key > bh->Items[_max].Key)
                _max = right;
            if (_max == index)
                break;

            bh->Items[index] = bh->Items[_max];
            bh->Items[_max] = max_item;
            index = _max;
            _2i = 2 * index;
        }
    }


};
