#pragma once

#include "Base.h"

// Basically small wrapper around C array;
template<typename T, size_t ElementCount>
struct StaticArray
{
	T Data[ElementCount];

	constexpr _FORCE_INLINE_ T* At(size_t idx)
	{
		SAssert(idx < ElementCount);
		return Data + idx;
	}

	constexpr _FORCE_INLINE_ T AtCopy(size_t idx)
	{
		SAssert(idx < ElementCount);
		return Data[idx];
	}

	constexpr _FORCE_INLINE_ size_t MemorySize()
	{
		return ElementCount * sizeof(T);
	}
};

// C array of T, keeps track of element with Count
template<typename T, size_t Capacity>
struct StaticList
{
	constexpr static size_t Capacity = Capacity;

	size_t Count;
	T Data[Capacity];

	constexpr _FORCE_INLINE_ T* At(size_t idx) { return SAssert(idx < Capacity); return Data + idx; }
	constexpr _FORCE_INLINE_ T AtCopy(size_t idx) { return SAssert(idx < Capacity); return Data[idx]; }
	constexpr _FORCE_INLINE_ size_t MemorySize() { return Capacity * sizeof(T); }
	constexpr _FORCE_INLINE_ T* Last() { return Data + ((Count > 0) ? Count - 1 : 0); }
	constexpr _FORCE_INLINE_ void Clear() { Count = 0; }

	constexpr _FORCE_INLINE_ void Push(const T* val)
	{
		SAssert(val);
		SAssert(Count < Capacity);
		Data[Count] = *val;
		++Count;
	}

	constexpr _FORCE_INLINE_ void PushAt(const T* val, size_t idx)
	{
		SAssert(val);
		SAssert(idx < Count);
		SAssert(Count < Capacity);
		Data[Count] = Data[idx];
		Data[idx] = *val;
		++Count;
	}

	constexpr _FORCE_INLINE_ void PushAtOrder(const T* val, size_t idx)
	{
		SAssert(val);
		SAssert(idx < Count);
		SAssert(Count < Capacity);
		SMemMove(Data + idx + 1, Data + idx, (Count - idx) * sizeof(T));
		Data[idx] = *val;
		++Count;
	}

	constexpr _FORCE_INLINE_ void Pop()
	{
		SAssert(Count > 0);
		--Count;
	}

	constexpr _FORCE_INLINE_ void PopAt(T* inValue, size_t idx)
	{
		SAssert(Count > 0);
		SAssert(idx < (Count - 1));
		*inValue = Data[idx];
		Data[idx] = *Last();
		--Count;
	}

	constexpr _FORCE_INLINE_ void PopAtOrder(T* inValue, size_t idx)
	{
		SAssert(Count > 0);
		SAssert(idx < (Count - 1));
		*inValue = Data[idx];
		SMemMove(Data + idx, Data + idx + 1, (Count - idx) * sizeof(T));
		--Count;
	}

};
