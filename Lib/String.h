#pragma once

#include "Base.h"
#include "Arena.h"

extern "C" {

    typedef char* String;

    struct string_header
    {
        u32 Length;
        u32 Capacity;
    };

    #define StringHeader(str) ((string_header*)(str) - 1)

    #define StringGetAllocSize(capacity) (sizeof(string_header) + (capacity) + 1)

    String StringMake(Arena* arena, const char* str);
    String StringMakeReserve(Arena* arena, u32 capacity);
    String StringMakeLength(Arena* arena, const void* init_str, u32 num_bytes);

    String StringMakeAllocator(SAllocator a, const char* str);
    void   StringFreeAllocator(SAllocator a, String str);
    String StringAppendAllocator(SAllocator a, String str, const char* cstr, u32 cStrLength);
    String StringMakeSpaceFor(SAllocator a, String str, u32 add_len);

    String StringSprintf(String str, u32 num_bytes, const char* fmt, ...);
    String StringSprintfBuffer(Arena* arena, const char* fmt, ...); // NOTE: Uses locally persistent buffer

    String StringAppendLength(String str, void const* other, u32 num_bytes);
    String StringAppend(String str, const char* other);
    String StringAppendString(String str, String const other);
    String StringJoin(String str, const char** parts, u32 count, const char* glue);
    String StringSet(String str, const char* cstr);
    u32    StringMemorySize(String const str);
    bool32 StringAreEqual(String const lhs, String const rhs);
    String StringTrim(String str, const char* cut_set);
    String StringAppendRune(String str, zpl_rune r);
    String StringAppendFmt(String str, const char* fmt, ...);

    String StringClone(Arena* arena, String str);

    void   StringClear(String str);
    u32    StringLength(String const str);
    u32    StringCapacity(String const str);
    u32    StringAvailableSpace(String const str);
    String StringTrimSpace(String str); // Whitespace ` \t\r\n\v\f`

    _FORCE_INLINE_ void StringSetLength(String str, u32 len) { StringHeader(str)->Length = len; }
    _FORCE_INLINE_ void StringSetCapacity(String str, u32 cap) { StringHeader(str)->Capacity = cap; }

    _FORCE_INLINE_ u64
    HashString(const String string)
    {
        //return FNVHash64(string, StringLength(string));
	    return wyhash(string, StringLength(string), 0, _wyp);
    }

#ifdef SCAL_STRING_IMPLEMENTATION

    String StringMakeAllocator(SAllocator a, const char* init_str)
    {
        SAssert(a.Allocator);
        size_t len = init_str ? strlen(init_str) : 0;
        SAssert(len < UINT32_MAX);

        u32 header_size = sizeof(string_header);

        size_t size = header_size + len + 1;
        void* ptr = SAllocatorAlloc(a, size);

        if (ptr == NULL) return NULL;
        if (!init_str) SMemZero(ptr, size);

        string_header* header = (string_header*)ptr;
        header->Length = (u32)len;
        header->Capacity = (u32)len;
        String str = (char*)(header + 1);
        if (len && init_str) SMemCopy(str, init_str, len);
        str[len] = '\0';

        return str;
    }

    void StringFreeAllocator(SAllocator a, String str)
    {
        SAssert(str);
        if (a.Allocator)
        {
            SAllocatorFree(a, StringHeader(str));
        }
    }

    String StringAppendAllocator(SAllocator a, String str, const char* other, u32 other_len)
    {
        SAssert(a.Allocator);
        SAssert(str);
        SAssert(other);

        if (!str || other_len > 0)
        {
            u32 curr_len = StringLength(str);

            if (curr_len + other_len > StringCapacity(str))
            {
                u32 new_size = StringMemorySize(str) + other_len;
                string_header* header = StringHeader(str);
                header = (string_header*)SAllocatorRealloc(a, header, new_size);

                if (!header) return nullptr;

                header->Capacity = curr_len + other_len;

                str = (char*)(header + 1);
            }

            SMemCopy(str + curr_len, other, other_len);
            str[curr_len + other_len] = '\0';
            StringSetLength(str, curr_len + other_len);
        }
        return str;
    }

    String StringMake(Arena* arena, const char* str)
    {
        size_t len = str ? strlen(str) : 0;
        SAssert(len < UINT32_MAX);
        return StringMakeLength(arena, str, (u32)len);
    }

    String StringClone(Arena* arena, String const str)
    {
        return StringMakeLength(arena, str, StringLength(str));
    }

    u32 StringLength(String const str) { return StringHeader(str)->Length; }
    u32 StringCapacity(String const str) { return StringHeader(str)->Capacity; }

    u32 StringAvailableSpace(String const str)
    {
        string_header* h = StringHeader(str);
        if (h->Capacity > h->Length) return h->Capacity - h->Length;
        return 0;
    }

    void StringClear(String str)
    {
        StringSetLength(str, 0);
        str[0] = '\0';
    }

    String StringAppendString(String str, String const other)
    {
        return StringAppendLength(str, other, StringLength(other));
    }

    String StringTrimSpace(String str) { return StringTrim(str, " \t\r\n\v\f"); }

    String StringMakeReserve(Arena* arena, u32 capacity) {
        u32 header_size = sizeof(string_header);

        void* ptr = ArenaPush(arena, header_size + capacity + 1);

        String str;
        string_header* header;

        if (ptr == NULL) return NULL;
        
        SMemZero(ptr, header_size + capacity + 1);

        str = (char*)(ptr) + header_size;
        header = StringHeader(str);
        header->Length = 0;
        header->Capacity = capacity;
        str[capacity] = '\0';

        return str;
    }

    String StringMakeLength(Arena* arena, void const* init_str, u32 num_bytes)
    {
        u32 header_size = sizeof(string_header);
        
        void* ptr = ArenaPush(arena, header_size + num_bytes + 1);
       
        String str;
        string_header* header;

        if (ptr == NULL) return NULL;
        if (!init_str) SMemZero(ptr, header_size + num_bytes + 1);

        str = (char*)(ptr) + header_size;
        header = StringHeader(str);
        header->Length = num_bytes;
        header->Capacity = num_bytes;
        if (num_bytes && init_str) SMemCopy(str, init_str, num_bytes);
        str[num_bytes] = '\0';

        return str;
    }

    String StringSprintfBuffer(Arena* arena, const char* fmt, ...)
    {
        SAssert(arena);

        local_persist thread_local char buf[ZPL_PRINTF_MAXLEN] = { 0 };

        va_list va;
        va_start(va, fmt);
        zpl_snprintf_va(buf, ZPL_PRINTF_MAXLEN, fmt, va);
        va_end(va);

        return StringMake(arena, buf);
    }

    String StringSprintf(String str, u32 num_bytes, const char* fmt, ...)
    {
        if (!str)
        {
            SCAL_ERROR("passed str is NULL");
            return str;
        }

        if (StringCapacity(str) > num_bytes)
        {
            LogErr("Str capacity was too small!");
            SMemZero(str, num_bytes);
            return str;
        }

        va_list va;
        va_start(va, fmt);
        zpl_snprintf_va(str, num_bytes, fmt, va);
        va_end(va);

        str[num_bytes] = '\0';

        StringSetLength(str, num_bytes);

        return str;
    }


    String StringAppendLength(String str, void const* other, u32 other_len)
    {
        if (!str || other_len > 0)
        {
            u32 curr_len = StringLength(str);

            if (curr_len + other_len > StringCapacity(str))
            {
                return str;
            }

            SMemCopy(str + curr_len, other, other_len);
            str[curr_len + other_len] = '\0';
            StringSetLength(str, curr_len + other_len);
        }
        return str;
    }

    String StringAppend(String str, const char* other)
    {
        return StringAppendLength(str, other, (u32)strlen(other));
    }

    String StringJoin(String str, const char** parts, u32 count, const char* glue)
    {
        if (!str)
        {
            LogErr("str passes was NULL");
            return str;
        }

        u32 i;

        for (i = 0; i < count; ++i) {
            str = StringAppend(str, parts[i]);

            if ((i + 1) < count) {
                str = StringAppend(str, glue);
            }
        }

        return str;
    }

    String StringSet(String str, const char* cstr)
    {
        size_t len = strlen(cstr);
        SAssert(len < UINT32_MAX);

        u32 capacity = StringCapacity(str);
        if (capacity < (u32)len)
        {
            len = capacity;
        }

        SMemCopy(str, cstr, (u32)len);
        str[len] = '\0';
        StringSetLength(str, (u32)len);

        return str;
    }

    String StringMakeSpaceFor(SAllocator a, String str, u32 add_len)
    {
        u32 available = StringAvailableSpace(str);

        // NOTE: Return if there is enough space left
        if (available >= add_len)
        {
            return str;
        }
        else
        {
            u32 new_len, old_size, new_size;
            string_header* header, *new_header;

            new_len = StringLength(str) + add_len;
            header = StringHeader(str);
            old_size = sizeof(string_header) + StringLength(str) + 1;
            new_size = sizeof(string_header) + new_len + 1;
            
            if (!a.Allocator) return nullptr;

            new_header = Cast(string_header*, SAllocatorRealloc(a, header, new_size));

            if (!new_header) return nullptr;

            new_header->Capacity = new_len;

            str = (char*)(new_header + 1);

            return str;
        }
    }

    u32 StringMemorySize(String const str) {
        u32 cap = StringCapacity(str);
        return sizeof(string_header) + cap;
    }

    bool32 StringAreEqual(String const lhs, String const rhs) {
        u32 lhs_len, rhs_len, i;
        lhs_len = StringLength(lhs);
        rhs_len = StringLength(rhs);
        if (lhs_len != rhs_len) return false;

        for (i = 0; i < lhs_len; i++) {
            if (lhs[i] != rhs[i]) return false;
        }

        return true;
    }

    String StringTrim(String str, const char* cut_set) {
        char* start, * end, * start_pos, * end_pos;
        u32 len;

        start_pos = start = str;
        end_pos = end = str + StringLength(str) - 1;

        while (start_pos <= end && zpl_char_first_occurence(cut_set, *start_pos)) start_pos++;
        while (end_pos > start_pos && zpl_char_first_occurence(cut_set, *end_pos)) end_pos--;

        len = (u32)((start_pos > end_pos) ? 0 : ((end_pos - start_pos) + 1));

        if (str != start_pos) SMemMove(str, start_pos, len);
        str[len] = '\0';

        StringSetLength(str, len);

        return str;
    }

    String StringAppendRune(String str, zpl_rune r)
    {
        if (r >= 0)
        {
            zpl_u8 buf[8] = { 0 };
            u32 len = (u32)zpl_utf8_encode_rune(buf, r);
            return StringAppendLength(str, buf, len);
        }

        return str;
    }

    String StringAppendFmt(String str, const char* fmt, ...)
    {
        u32 res;
        char buf[ZPL_PRINTF_MAXLEN] = { 0 };
        va_list va;
        va_start(va, fmt);
        res = (u32)zpl_snprintf_va(buf, zpl_count_of(buf) - 1, fmt, va) - 1;
        va_end(va);
        return StringAppendLength(str, buf, res);
    }

#endif
}
