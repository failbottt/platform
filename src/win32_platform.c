#include <windows.h>
#include <stdio.h>

#include "base_inc.h"
#include "platform.h"
#include "font.c"

internal void game_update_and_render(GameOffscreenBuffer *buffer, Rect window_dim, Game game)
{
    platform_clear_screen(buffer, RGBA(32, 21, 51, 255));

    {
        char text[256];
        int written = _snprintf_s(
                text,
                sizeof(text),
                256,
                ": (%lld)",
                game.score);
        text[written + 1] = '\0';

        debug_font_draw(buffer, "game engine lul", 15, (int)(window_dim.w / 2 - 100), 150);
    }
}

void platform_clear_screen(GameOffscreenBuffer *buffer, Rgba color)
{
    u8 *Row = (u8 *)buffer->Memory;
    for (int Y = 0; Y < buffer->Height; Y++)
    {
        u32 *Pixel = (u32 *)Row;
        for (int X = 0; X < buffer->Width; X++)
        {
            // A R G B
            // *Pixel++ = (255 << 24) | (32 << 16) | (21 << 8) | 51;
            *Pixel++ = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
        }

        Row += buffer->Pitch;
    }
}

// @fix: writes past end of the buffer and crashes
void platform_draw_square(
        GameOffscreenBuffer *Buffer,
        Rect window_dim,
        u64 w,
        u64 h,
        u64 posx,
        u64 posy,
        Rgba rgba
        )
{
    u64 x = posx;
    u64 y = posy;
    u64 square_height = h;
    u64 square_width = w;

    if ((x + square_width) > window_dim.w - 1)
    {
        x = window_dim.w - 1;
    }

    if ((y + square_height) > window_dim.h - 1)
    {
        y = window_dim.h - 1;
    }

    u8 *Row = (u8 *)Buffer->Memory + (y * Buffer->Pitch) + (x * sizeof(u32));
    for(int Y = 0;
            Y < square_height;
            ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        for(int X = 0;
                X < square_width;
                ++X)
        {
            *Pixel++ = ((rgba.a << 24) | (rgba.r << 16) | (rgba.g << 8) | rgba.b);
        }

        Row += Buffer->Pitch;
    }
}

File debug_platform_read_entire_file(char *Filename)
{
    File Result = {0};

    HANDLE FileHandle = CreateFile(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    // DWORD LastError = GetLastError();

    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize)) {
            Assert(FileSize.QuadPart <= max_u64);
            u64 FileSize64 = FileSize.QuadPart;
            void *Buffer = VirtualAlloc(0, FileSize.QuadPart, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(Buffer)
            {
                LPDWORD BytesRead = 0;
                // C is required to execute these in order
                // A && B && C
                // will always be A THEN B THEN C
                if(ReadFile(FileHandle, Buffer, FileSize.QuadPart, BytesRead, 0) &&
                        // if we got the file size
                        // someone truncated the file
                        // we read the file
                        // and we didn't read back the expected size we know something went wrong
                        (FileSize64 == *BytesRead))
                {
                    // file read success
                    Result.Length = *BytesRead;
                    Result.Data = Buffer;
                }
                else
                {
                    // logging
                    debug_platform_free_file_memory(Result.Data, FileSize64);
                }
            }
            else
            {
                // logging
            }
        }
        else
        {
            // logging
        }

        CloseHandle(FileHandle);
    }
    else
    {
        // logging
    }

    return Result;
}

void debug_platform_free_file_memory(void *memory, u64 memory_size)
{
    // win32 free file memory
    if(memory && VirtualFree(memory, memory_size, MEM_DECOMMIT|MEM_RELEASE))
    {
        // freed
    }
    else
    {
        // unable to free
    }
}

