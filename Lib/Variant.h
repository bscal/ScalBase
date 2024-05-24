#pragma once

#include "Base.h"
#include "String.h"

enum VariantType
{
    VARIANT_BOOL,
    VARIANT_I32,
    VARIANT_U32,
    VARIANT_I64,
    VARIANT_U64,
    VARIANT_FLOAT,
    VARIANT_DOUBLE,
    VARIANT_POINTER,
    VARIANT_STRING,
};

struct Variant
{
    union 
    {
        bool Bool;
        i32 I32;
        u32 U32;
        i64 I64;
        u64 U64;
        float Float;
        double Double;
        void* Pointer;
        String String;
    };
    VariantType Type;
};

#define VARIANT_DEFINE_OF(name, cType, variantType)   \
    inline Variant Of##name(cType* val)  \
    {   \
        Variant v = {}; \
        v.##name = *val; \
        v.Type = variantType;   \
        return v;   \
    }

#define VARIANT_DEFINE_AS(name, cType, variantType) \
    inline cType As##name(Variant v)   \
    {   \
        SAssert(v.Type == variantType);    \
        if (v.Type != variantType)  \
        {   \
            LogWarn("Variant is not of type %s !", Stringify(cType));    \
        }   \
        return v.##name;  \
    }

#define VARIANT_DEFINES(name, cType, variantType)   \
    VARIANT_DEFINE_OF(name, cType, variantType);    \
    VARIANT_DEFINE_AS(name, cType, variantType);    

namespace Variants
{
    VARIANT_DEFINES(Bool, bool, VARIANT_BOOL);
    VARIANT_DEFINES(I32, i32, VARIANT_I32);
    VARIANT_DEFINES(U32, u32, VARIANT_U32);
    VARIANT_DEFINES(I64, i64, VARIANT_I64);
    VARIANT_DEFINES(U64, u64, VARIANT_U64);
    VARIANT_DEFINES(Float, float, VARIANT_FLOAT);
    VARIANT_DEFINES(Double, double, VARIANT_DOUBLE);
    VARIANT_DEFINES(Pointer, void*, VARIANT_POINTER);
    VARIANT_DEFINES(String, String, VARIANT_STRING);
}
