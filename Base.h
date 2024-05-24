#pragma once

#include <stdint.h>

#include "../vendor/zpl/zpl.h"
#include "../vendor/wyhash/wyhash.h"
#include "../vendor/HandmadeMath/HandmadeMath.h"

#define SCAL_TESTS 1

static_assert(sizeof(size_t) == sizeof(unsigned long long), "ScalEngine does not support 32bit");
static_assert(sizeof(int) == 4, "sizeof int != 4 bytes");
static_assert(sizeof(int) == sizeof(long), "sizeof int != sizeof long");
static_assert(sizeof(int) == sizeof(float), "sizeof int != sizeof float");
static_assert(sizeof(char) == 1, "sizeof char does not == 1");
static_assert(sizeof(uint8_t) == 1, "sizeof uint8_t does not == 1");

typedef int bool32;
typedef int Result;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define Array(Type) (Type*)

#define internal static
#define global_var inline
#define constant_var constexpr static
#define local_persist static

enum class LogLevel { Info, Error, Debug, Warn, Fatal, Max };
constexpr const char* LogLevel_PREFIXES[] = { "(INFO)", "(ERROR)", "(DEBUG)", "(WARN)", "(FATAL)" };

extern void LogMsgRaw(const char* msg, ...);
extern void LogMsg(LogLevel level, const char* msg, ...);

#define LogRaw(msg, ...) LogMsgRaw(msg, __VA_ARGS__)
#define LogInfo(msg, ...) LogMsg(LogLevel::Info, (msg), __VA_ARGS__)
#define LogErr(msg, ...) LogMsg(LogLevel::Error, (msg), __VA_ARGS__)

#define SCAL_ERROR(msg, ...) SCAL_ERROR_CODE(1, (msg), ##__VA_ARGS__)
#define SCAL_ERROR_CODE(code, msg, ...) LogErr((msg), ##__VA_ARGS__); DebugBreak(void)
#define SCAL_FATAL(msg, ...) LogMsg(LogLevel::Fatal, (msg), ##__VA_ARGS__); DebugBreak(void)
#define SERR_INVALID_PATH() SCAL_FATAL("Invalid code path reached! %s, %d", __FUNCTION__, __LINE__)

#if SCAL_DEBUG
	#define LogWarn(msg, ...) LogMsg(LogLevel::Warn, (msg), __VA_ARGS__)
    #define LogDebug(msg, ...) LogMsg(LogLevel::Debug, (msg), __VA_ARGS__)
	#define SAssert(expr) if (!(expr)) { LogErr("Assertion Failure: %s\nMessage: % s\n  File : % s, Line : % d\n", #expr, "", __FILE__, __LINE__); DebugBreak(void); } 
	#define SAssertMsg(expr, msg) if (!(expr)) { LogErr("Assertion Failure: %s\nMessage: % s\n  File : % s, Line : % d\n", #expr, msg, __FILE__, __LINE__); DebugBreak(void); }
#else
	#define LogWarn(msg, ...)
    #define LogDebug(msg, ...)
	#define SAssert(expr)
	#define SAssertMsg(expr, msg)
#endif

#ifdef SCAL_DEBUG
#ifdef _MSC_VER
#define DebugBreak(void) __debugbreak()
#else
#define DebugBreak(void) __builtin_trap()
#endif
#else
#define DebugBreak(void)
#endif

#define C_DECL_BEGIN extern "C" {
#define C_DECL_END }

#if defined(__clang__) || defined(__GNUC__)
#define _RESTRICT_ __restrict__
#elif defined(_MSC_VER)
#define _RESTRICT_ __restrict
#else
#define _RESTRICT_
#endif

#ifndef _ALWAYS_INLINE_
#if defined(__clang__) || defined(__GNUC__)
#define _ALWAYS_INLINE_ __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define _ALWAYS_INLINE_ __forceinline
#else
#define _ALWAYS_INLINE_ inline
#endif
#endif

#ifndef _FORCE_INLINE_
#ifdef SCAL_DEBUG
#define _FORCE_INLINE_ inline
#else
#define _FORCE_INLINE_ _ALWAYS_INLINE_
#endif
#endif

#ifndef _NEVER_INLINE_
#if defined(__clang__) || defined(__GNUC__)
#define _NEVER_INLINE_ __attribute__((__noinline__)) inline
#elif defined(_MSC_VER)
#define _NEVER_INLINE __declspec(noinline)
#else
#define _NEVER_INLINE_ 
#endif
#endif

#ifdef SCAL_PLATFORM_WINDOWS
#ifdef SCAL_BUILD_DLL
#define SAPI extern "C" __declspec(dllexport)
#else
#define SAPI extern "C" __declspec(dllimport)
#endif // SCAL_BUILD_DLL
#else
#define SAPI extern "C"
#endif

#if defined(_MSC_VER)
#	define S_CALL __cdecl
#else
#	define S_CALL
#endif

#if defined(_MSC_VER)
#define NO_VTABLE __declspec(novtable)
#else
#define NO_VTABLE
#endif

#ifndef SCAL_STATIC
#	if defined(_MSC_VER)
#		ifdef SCAL_EXPORT
#			define S_API __declspec(dllexport)
#		else
#			define S_API __declspec(dllimport)
#		endif
#	else
#		if ((__GNUC__ >= 4) || defined(__clang__))
#			define S_API __attribute__((visibility("default")))
#		else
#			define S_API
#		endif
#	endif
#else
#	define S_API
#endif

#define Unpack(...) __VA_ARGS__
#define Stringify(x) #x
#define Expand(x) Stringify(x)
#define NameConcat2(a, b) a##b
#define NameConcat(a, b) NameConcat2(a, b)

#define Stringize_Internal(...) #__VA_ARGS__
#define Stringize(...) Stringize_Internal(__VA_ARGS__)

#define ArrayLength(arr) (sizeof(arr) / sizeof(arr[0]))

#define Kilobytes(n) ((n) * 1024ULL)
#define Megabytes(n) (Kilobytes(n) * 1024ULL)
#define Gigabytes(n) (Megabytes(n) * 1024ULL)
#define MS 1000

constant_var double SCAL_PI = 3.14159265358979323846;
constant_var double SCAL_TAO = SCAL_PI * 2.0;
constant_var size_t SCAL_DEFAULT_ALIGNMENT = 16;
constant_var size_t SCAL_CACHE_LINE = 64;
constant_var size_t SCAL_PAGE_SIZE = Kilobytes(4);

#define FlagTrue(state, flag) (((state) & (flag)) == (flag))
#define FlagFalse(state, flag) (((state) & (flag)) != (flag))
#define FlagSet(state, flag, boolean) ((boolean) ? BitSet(state, flag) : BitClear(state, flag))

#define Bit(x) (1ULL<<(x))
#define BitGet(state, bit) (((state) >> (bit)) & 1ULL)
#define BitSet(state, bit) ((state) | (1ULL << (bit)))
#define BitClear(state, bit) ((state) & ~(1ULL << (bit)))
#define BitToggle(state, bit) ((state) ^ (1ULL << (bit)))
#define BitMask(state, mask) (FlagTrue((state), (mask)))

#define Cast(T, v) static_cast<T>((v))

#define Min(v0, v1) ((v0 < v1) ? v0 : v1)
#define Max(v0, v1) ((v0 > v1) ? v0 : v1)
#define ClampValue(v, min, max) Min(max, Max(min, v))

#define CallConstructor(object, T) new (object) T

// Double linked list : First, Last, Node
#define DLPushBackT(f, l, n, Next, Prev) (((f) == 0 \
											? (f) = (l) = (n), (n)->Next = (n)->Prev = 0) \
											: ((n)->Prev = (l), (l)->Next = (n), (l) = (n), (n)->Next = 0))
#define DLPushBack(f, l, n) DLPushBackT(f, l, n, Next, Prev)

#define DLPushFront(f, l, n) DLPushBackT(f, l, n, Prev, Next)

#define DLRemoveT(f, l, n, Next, Prev) (((f) == (n) \
											? ((f) = (f)->Next, (f)->Prev = 0) \
											: (l) == (n) \
												? ((l) = (l)->Prev, (l)->Next = 0) \
												: ((n)->Next->Prev = (n)->Prev \
													, (n)->Prev->Next = (n)->Next)))

#define DLRemove(f, l, n) ) DLRemoveT(f, l, n, Next, Prev)

// Singled linked list queue : First, Last, Node
#define SLQueuePush(f, l, n, Next) ((f) == 0 \
									? (f) = (l) = (n) \
									: ((l)->Next = (n), (l) = (n)) \
										, (n)->Next = 0)

#define SLQueuePushFront(f, l, n, Next) ((f) == 0 \
											? (f) = (l) = (n), (n)->Next = 0) \
											: ((n)->Next = (f), (f) = (n))) \

#define SLQueuePop(f, l) ((f) == (l) \
								? (f) = (l) = 0 \
								: (f) = (f)->Next)

// Singled linked list stack : First, Node
#define SLStackPush(f, n, Next) ((n)->Next = (f), (f) = (n))

#define SLStackPop(f, Next) ((f) == 0 ? 0 : (f) = (f)->Next)

#define SMemCopy(dst, src, size) memcpy((dst), (src), (size))
#define SMemMove(dst, src, size) memmove((dst), (src), (size))
#define SMemSet(dst, val_u8, size) zpl_memset((dst), (val_u8), (size))
#define SMemZero(dst, size) (SMemSet((dst), 0, (size)))

#define SMalloc(size) _aligned_malloc(size, SCAL_DEFAULT_ALIGNMENT)
#define SCalloc(size, count) _aligned_recalloc(nullptr, count, size, SCAL_DEFAULT_ALIGNMENT)
#define SRealloc(ptr, size) _aligned_realloc(ptr, size, SCAL_DEFAULT_ALIGNMENT)
#define SFree(ptr) _aligned_free(ptr)

enum SAllocatorTypes
{
	SALLOCATOR_TYPE_MALLOC,
	SALLOCATOR_TYPE_REALLOC,
	SALLOCATOR_TYPE_FREE
};

#define SALLOCATOR_ALLOCATOR(name) void* name(SAllocatorTypes type, void* ptr, size_t size, \
	void* userData, const char* file, int line)
typedef SALLOCATOR_ALLOCATOR(SAllocatorFunc);

struct SAllocator
{
   	SAllocatorFunc* Allocator;
	void* UserData;
};

inline void* 
AllocatorError(const char* file, const char* func, int line)
{
	SCAL_ERROR("SAllocator.Allocator was NULL! %s, %s, %d", file, func, line);
	return nullptr;
}

#define SAllocatorAlloc(allocator, sz)	\
	((allocator.Allocator) \
		? allocator.Allocator(SALLOCATOR_TYPE_MALLOC,	\
			nullptr, (sz), allocator.UserData, __FILE__, __LINE__) \
		: AllocatorError(__FILE__, __FUNCTION__, __LINE__))

#define SAllocatorRealloc(allocator, ptr, sz)	\
	((allocator.Allocator) \
		? allocator.Allocator(SALLOCATOR_TYPE_REALLOC,	\
			(ptr), (sz), allocator.UserData, __FILE__, __LINE__) \
		: AllocatorError(__FILE__, __FUNCTION__, __LINE__))

#define SAllocatorFree(allocator, ptr)	\
	((allocator.Allocator) \
		? allocator.Allocator(SALLOCATOR_TYPE_FREE, 	\
			(ptr), 0, allocator.UserData, __FILE__, __LINE__) \
		: AllocatorError(__FILE__, __FUNCTION__, __LINE__))

inline SALLOCATOR_ALLOCATOR(SAllocatorMalloc)
{
	switch (type)
	{
		case (SALLOCATOR_TYPE_MALLOC):
		{
			SAssert(size > 0);
			SAssert(!ptr);

			if (!ptr)
			{
				ptr = SMalloc(size);
			}
			else
			{
				LogErr("malloc called with ptr that is not NULL");
			}

			SAssert(ptr);
		} break;
		case (SALLOCATOR_TYPE_REALLOC):
		{
			SAssert(size > 0);
			
			ptr = SRealloc(ptr, size);

			SAssert(ptr);
		} break;
		case (SALLOCATOR_TYPE_FREE):
		{
			SAssert(ptr);

			SFree(ptr);
		} break;

		default:
		{
			SCAL_ERROR("SAllocatorType is not valid");
			return nullptr;
		};
	};

	return ptr;
}


// ################# Utils ##################

_FORCE_INLINE_ u64 constexpr
FNVHash64(const void* const data, size_t length)
{
	constexpr u64 OFFSET_BASIS = 0xcbf29ce484222325;
	constexpr u64 PRIME = 0x100000001b3;
	const u8* const castData = (const u8* const)data;
	u64 val = OFFSET_BASIS;
	for (size_t i = 0; i < length; ++i)
	{
		val ^= castData[i];
		val *= PRIME;
	}
	return val;
}

_FORCE_INLINE_ u64 constexpr 
FNVHash64_CStr(const char* name)
{
	constexpr u64 OFFSET_BASIS = 0xcbf29ce484222325;
	constexpr u64 PRIME = 0x100000001b3;
	u64 val = OFFSET_BASIS;
	char c = 0;
	while ((c = *name++))
	{
		val ^= (u64)c;
		val *= PRIME;
	}
	return val;
}

template<typename T>
_FORCE_INLINE_ u64
Hash(const T* value)
{
	return FNVHash64(value, sizeof(T));
}

#define FastModulo(value, capacity) ((value) & ((capacity) - 1))
#define HashAndMod(value, capacity) FastModulo(Hash((value)), (capacity))

template<typename T>
_FORCE_INLINE_ void
Swap(T& a, T& b)
{
	T c = a;
	a = b;
	b = c;
}

_FORCE_INLINE_ constexpr int
IntModNegative(int a, int b)
{
	int res = a % b;
	return (res < 0) ? res + b : res;
}

inline constexpr size_t
AlignPowTwo(size_t num)
{
	if (num == 0) return 0;

	size_t power = 1;
	while (num >>= 1) power <<= 1;
	return power;
}

inline constexpr size_t
AlignPowTwoCeil(size_t x)
{
	if (x <= 1) return 1;
	size_t power = 2;
	--x;
	while (x >>= 1) power <<= 1;
	return power;
}

inline constexpr u32
AlignPowTwoCeil32(u32 x)
{
	if (x <= 1) return 1;
	u32 power = 2;
	--x;
	while (x >>= 1) power <<= 1;
	return power;
}

_FORCE_INLINE_ constexpr bool
IsPowerOf2(size_t num)
{
	return (num > 0 && ((num & (num - 1)) == 0));
}

_FORCE_INLINE_ constexpr size_t
AlignSizeTruncate(size_t size, size_t alignment)
{
	return size & ~(alignment - 1);
}

_FORCE_INLINE_ constexpr size_t
AlignSize(size_t size, size_t alignment)
{
	return (size + (alignment - 1)) & ~(alignment - 1);
}

// Basic single threaded text formatting
inline const char* 
TextFormat(const char *text, ...)
{
	constexpr size_t MAX_TEXTFORMAT_BUFFERS = 4;
	constexpr size_t MAX_TEXT_BUFFER_LENGTH = 512;
    thread_local char buffers[MAX_TEXTFORMAT_BUFFERS][MAX_TEXT_BUFFER_LENGTH] = { 0 };
    thread_local int index = 0;

    char* currentBuffer = buffers[index];
    SMemZero(currentBuffer, MAX_TEXT_BUFFER_LENGTH);

    va_list args;
    va_start(args, text);
    zpl_isize requiredByteCount = zpl_snprintf_va(currentBuffer, MAX_TEXT_BUFFER_LENGTH, text, args);
    va_end(args);

    // If requiredByteCount is larger than the MAX_TEXT_BUFFER_LENGTH, then overflow occured
    if (requiredByteCount >= MAX_TEXT_BUFFER_LENGTH)
    {
        // Inserting "..." at the end of the string to mark as truncated
        char *truncBuffer = buffers[index] + MAX_TEXT_BUFFER_LENGTH - 4; // Adding 4 bytes = "...\0"
        zpl_snprintf(truncBuffer, sizeof("..."), "...");
    }

	index = (index + 1) % MAX_TEXTFORMAT_BUFFERS;
    return currentBuffer;
}
