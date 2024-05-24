#pragma warning(disable: 4324) // warning for alignment padding

#pragma once

#include "Base.h"

// Single producer - multi consumer fifo queue
template<typename T, int Capacity>
struct QueueThreaded
{
	alignas(CACHE_LINE) zpl_atomic32 First; // Read
	alignas(CACHE_LINE) zpl_atomic32 Last; // Write
	T Memory[Capacity];

	/*
	* 
	* zpl_atomic32_load/store are just wrappers around settings zpl_atomic32.value
	* I use them just to explicitly show loads and stores
	* 
	*/

	bool Enqueue(const T* value)
	{
		int originalLast = zpl_atomic32_load(&Last);
		int newLast = (originalLast + 1) % Capacity;
		SAssertMsg(newLast != First.value, "Queue overflow");
		if (newLast != zpl_atomic32_load(&First))
		{
			Memory[originalLast] = *value;
			zpl_sfence();
			zpl_atomic32_store(&Last, newLast);
			return true;
		}
		return false;
	}

	bool Dequeue(T* outValue)
	{
		int originalFirst = zpl_atomic32_load(&First);
		if (originalFirst != zpl_atomic32_load(&Last))
		{
			int newFirst = (originalFirst + 1) % Capacity;
			int index = zpl_atomic32_compare_exchange(&First, originalFirst, newFirst);
			if (index == originalFirst)
			{
				*outValue = Memory[index];
				return true;
			}
		}
		return false;
	}
};
