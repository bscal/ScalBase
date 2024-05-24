#pragma once

#include "Base.h"
#include "Engine_Win32.h"

#include "Lib/Debug.h"

#define GetTime() (Platform::GetTimeSecs())
#define GetTimeF() ((float)Platform::GetTimeSecs())
#define GetTimeMS() ((i64)(GetTimeF()) * 1000.0f)

inline void
LogMsgRaw(const char* msg, ...)
{
	char outMsg[512] = {};
	va_list argPtr;
	va_start(argPtr, msg);
	zpl_snprintf_va(outMsg, sizeof(outMsg), msg, argPtr);
	va_end(argPtr);
	PlatformLog(outMsg, Cast(int, LogLevel::Info));
}

inline void 
LogMsg(LogLevel level, const char* msg, ...)
{
    char outMsg[512] = {};

	va_list argPtr;
	va_start(argPtr, msg);
	zpl_snprintf_va(outMsg, sizeof(outMsg), msg, argPtr);
	va_end(argPtr);

    int iLevel = Cast(int, level);
    const char* prefix;
    if (Cast(int, level) < Cast(int, LogLevel::Max))
        prefix = LogLevel_PREFIXES[Cast(int, level)];
    else
        prefix = "";

	char outMsgTemp[sizeof(outMsg) + 10];
	zpl_snprintf(outMsgTemp, sizeof(outMsg), "%s %s\n", prefix, outMsg);

    if (level == LogLevel::Error)
        PlatformLogError(outMsgTemp, Cast(int, level));
    else
        PlatformLog(outMsgTemp, Cast(int, level));
}
