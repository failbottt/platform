#ifndef _PLATFORM_H
#define _PLATFORM_H

#include "base_types.h"

typedef struct Game
{
    u64 mouse_x;
    u64 mouse_y;
    u8 clicked;
    f32 timer;
    u64 frame;
    u64 box_pos_x;
    u64 box_pos_y;
    u64 score;
} Game;

typedef struct GameOffscreenBuffer
{
    // Pixels are always 32-bits wide, Memory Order BB GG RR XX
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
} GameOffscreenBuffer;

typedef struct GameMemory
{
    u8 IsInitialized;
    u64 PermenantStorageSize;
    void* PermenantStorage; // NOTE(spangler): REQUIRED to be cleared to zero at startup
                            //
    u64 ScratchStorageSize;
    void* ScratchStorage; // NOTE(spangler): REQUIRED to be cleared to zero at startup
} GameMemory;

File debug_platform_read_entire_file(char *filename);

void debug_platform_free_file_memory(void *memory, u64 memory_size);

void platform_draw_square(
        GameOffscreenBuffer *buffer,
        Rect window_dim,
        u64 w,
        u64 h,
        u64 posx,
        u64 posy,
        Rgba rgba
        );

void platform_clear_screen(GameOffscreenBuffer *buffer, Rgba color);

#endif
