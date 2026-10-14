#ifndef _BASE_TYPES_H
#define _BASE_TYPES_H

#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define PI32 3.14159265359f

typedef int8_t      s8;
typedef int16_t     s16;
typedef int32_t     s32;
typedef int64_t     s64;
typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;
typedef float       f32;
typedef double      f64;

global_variable s8  min_s8  = (s8) 0x80;
global_variable s16 min_s16 = (s16)0x8000;
global_variable s32 min_s32 = (s32)0x80000000;
global_variable s64 min_s64 = (s64)0x8000000000000000llu;

global_variable s8  max_s8  = (s8) 0x7f;
global_variable s16 max_s16 = (s16)0x7fff;
global_variable s32 max_s32 = (s32)0x7fffffff;
global_variable s64 max_s64 = (s64)0x7fffffffffffffffllu;

global_variable u8  max_u8  = 0xff;
global_variable u16 max_u16 = 0xffff;
global_variable u32 max_u32 = 0xffffffff;
global_variable u64 max_u64 = 0xffffffffffffffffllu;

typedef struct String {
    u64 length;
    u8  *str;
} String;

#define STRING(s) (String){strlen(s), s}

typedef struct File {
    u64     Length;
    void    *Data;
} File;

typedef struct Rgba {
   u8 r;
   u8 g;
   u8 b;
   u8 a;
} Rgba;

typedef struct Rect {
    u64 w;
    u64 h;
} Rect;

#endif
