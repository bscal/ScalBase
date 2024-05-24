#pragma once

#include "Base.h"
#include "Engine_Win32.h"

namespace Debug
{

/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#define RESULT_DEFS \
SENUM(RESULT_SUCCESS, 0) \
SENUM(RESULT_ERROR, -1) \

enum
{
	#define SENUM(K, V) SCAL_##K = V,
	RESULT_DEFS
	#undef SENUM
};

struct Result
{
	/* @member Either 0 for success, or -1 for failure. */
	int code;

	/* @member String containing details about any error encountered. */
	const char* details;
};

inline Result Error(const char* details)
{ 
	Result result; 
	result.code = SCAL_RESULT_ERROR; 
	result.details = details; 
	return result;
}

inline Result Ok()
{ 
	Result result;
	result.code = SCAL_RESULT_SUCCESS;
	result.details = nullptr;
	return result;
}

#define MESSAGE_BOX_TYPE_DEFS \
	SENUM(MESSAGE_BOX_TYPE_ERROR, 0)         \
	SENUM(MESSAGE_BOX_TYPE_WARNING, 1)       \
	SENUM(MESSAGE_BOX_TYPE_INFORMATION, 2)   \

enum MessageBoxType
{
	#define SENUM(K, V) SCAL_##K = V,
	MESSAGE_BOX_TYPE_DEFS
	#undef SENUM
};

inline const char* 
MessageBoxTypeToString(MessageBoxType type)
{
	switch (type)
    {
	    #define SENUM(K, V) case SCAL_##K: return Stringify(SCAL_##K);
	    MESSAGE_BOX_TYPE_DEFS
	    #undef SENUM
	
        default: return NULL;
	}
}

S_API inline void S_CALL
SendMessageBox(MessageBoxType type, const char* title, const char* text)
{
	LogErr("%s - %s", title, text);
}

struct ProfileAnchor
{
	u64 ElapsedSelf;
	u64 ElapsedChildren;
    u64 HitCount;
    char const *Label;
};

struct Profiler
{
    ProfileAnchor Anchors[4096];
    u64 Start;
    u64 End;
};

inline Profiler g_Profiler;
inline u32 g_ProfilerParentIndex;
static_assert(__COUNTER__ < ArrayLength(g_Profiler.Anchors));

struct ProfilerScope
{
    char const *Label;
    u64 Start;
	u64 OldElapsedChildren;
    u32 AnchorIndex;
	u32 ParentIndex;
    
    ProfilerScope(const char* label, u32 anchorIndex)
    {
		ParentIndex = g_ProfilerParentIndex;

        Label = label;
        AnchorIndex = anchorIndex;

		ProfileAnchor* anchor = g_Profiler.Anchors + AnchorIndex;
		OldElapsedChildren = anchor->ElapsedChildren;

		g_ProfilerParentIndex = AnchorIndex;
        Start = Platform::GetCPUTime(); 
    }

    ~ProfilerScope()
    {
        u64 elapsed = Platform::GetCPUTime() - Start;

		g_ProfilerParentIndex = ParentIndex;

        ProfileAnchor* anchor = g_Profiler.Anchors + AnchorIndex;
		ProfileAnchor* parentAnchor = g_Profiler.Anchors + g_ProfilerParentIndex;

		parentAnchor->ElapsedSelf -= elapsed;
        anchor->ElapsedSelf += elapsed;
		anchor->ElapsedChildren = OldElapsedChildren + elapsed; 

        ++anchor->HitCount;

        anchor->Label = Label;
    }
};

#define TimeBlock(Name) Debug::ProfilerScope NameConcat(Block, __LINE__)(Name, __COUNTER__ + 1);
#define TimeFunction TimeBlock(__func__)

inline void 
PrintTimeElapsed(u64 totalElapsed, ProfileAnchor* anchor)
{
    double percent = 100.0 * ((double)anchor->ElapsedSelf / (double)totalElapsed);
    LogRaw("  %s[%llu]: %llu (%.2f%%)", anchor->Label, anchor->HitCount, (double)anchor->ElapsedSelf, percent);
	if(anchor->ElapsedChildren != (double)anchor->ElapsedSelf)
    {
        double percentWithChildren = 100.0 * ((double)anchor->ElapsedChildren / (double)totalElapsed);
        LogRaw(", %.2f%% w/children", percentWithChildren);
    }
	LogRaw("\n");
}

inline void 
BeginProfile()
{
    g_Profiler.Start = Platform::GetCPUTime();
}

inline void 
EndAndPrintProfile()
{
    g_Profiler.End = Platform::GetCPUTime();

    u64 osFreq = Platform::GetOSFreq();
	u64 cpuStart = Platform::GetCPUTime();
	u64 osStart = Platform::GetOSTime();
	u64 osEnd = 0;
	u64 osElapsed = 0;

    constexpr u64 msToWait = 100;
	u64 waitTime = osFreq * msToWait / 1000;
	while (osElapsed < waitTime)
	{
		osEnd = Platform::GetOSTime();
		osElapsed = osEnd - osStart;
	}

	u64 cpuEnd = Platform::GetCPUTime();
	u64 cpuElapsed = cpuEnd - cpuStart;
	u64 cpuFreq = 0;
	if (osElapsed)
	{
		cpuFreq = osFreq * cpuElapsed / osElapsed;
	}
    
    u64 TotalCPUElapsed = g_Profiler.End - g_Profiler.Start;
    
    if(cpuFreq)
    {
        LogRaw("\nTotal time: %0.4fms (CPU freq %llu)\n", 1000.0 * (double)TotalCPUElapsed 
        / (double)cpuFreq, cpuFreq);
    }
    
    for(u32 AnchorIndex = 0; AnchorIndex < ArrayLength(g_Profiler.Anchors); ++AnchorIndex)
    {
        ProfileAnchor* anchor = g_Profiler.Anchors + AnchorIndex;
        if(anchor->ElapsedSelf)
        {
            PrintTimeElapsed(TotalCPUElapsed, anchor);
        }
    }

	LogRaw("\n");
}

}
