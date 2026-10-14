#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "platform.h"

float font_size = 32.0f;
int bitmap_width = 1024;
int bitmap_height = 1024;
unsigned char ttf_buffer[1<<20];
unsigned char temp_bitmap[1024*1024];
// ASCII 32..126 is 95 glyphs
stbtt_bakedchar cdata[96];

inline void debug_font_init()
{
    fread(ttf_buffer, 1, 1<<20, fopen("c:/windows/fonts/times.ttf", "rb"));
    stbtt_BakeFontBitmap(ttf_buffer,0, 32.0, temp_bitmap,bitmap_width,bitmap_height, 32,96, cdata);
}

inline void debug_font_draw(GameOffscreenBuffer *buffer, char *text, int text_len, f32 x, f32 y)
{
    //  Assume you have access to your baked font bitmap data from stbtt_BakeFontBitmap
    // (e.g., u8 *font_bitmap_pixels, which is 1024x1024)

    // Assume orthographic projection with units = screen pixels, origin at top left
    for (int i = 0; i < text_len; i++) {
        u64 c = text[i];
        if (c >= 32 && c < 128) {
            stbtt_aligned_quad q;

            // This updates x and y for the next character, and fills 'q'
            stbtt_GetBakedQuad(cdata, bitmap_width, bitmap_height, c-32, &x, &y, &q, 1);

            // 1. Get the pixel bounds of the character quad on the screen
            // Truncate/round to integer screen coordinates safely
            int x0 = (int)floorf(q.x0);
            int x1 = (int)floorf(q.x1);
            int y0 = (int)ceilf(q.y0);
            int y1 = (int)ceilf(q.y1);

            // (Optional but highly recommended) Clip against screen/buffer boundaries
            if (x0 < 0) x0 = 0;
            if (y0 < 0) y0 = 0;
            if (x1 > buffer->Width)  x1 = buffer->Width;
            if (y1 > buffer->Height) y1 = buffer->Height;

            // 2. Loop through the destination screen pixels for this character
            for (int Y = y0; Y < y1; ++Y) {
                // Calculate starting row pointer for the target buffer
                u8 *Row = (u8 *)buffer->Memory + (Y * buffer->Pitch) + (x0 * sizeof(u32));
                u32 *Pixel = (u32 *)Row;

                // Calculate where we are vertically in the source texture (0.0 to 1.0)
                f32 t = q.t0 + ((f32)(Y - y0) / (y1 - y0)) * (q.t1 - q.t0);
                // Convert normalized 't' to actual texture row offset
                int src_y = (int)(t * bitmap_height);

                for (int X = x0; X < x1; ++X) {
                    // Calculate where we are horizontally in the source texture (0.0 to 1.0)
                    f32 s = q.s0 + ((f32)(X - x0) / (q.x1 - q.x0)) * (q.s1 - q.s0);
                    // Convert normalized 's' to actual texture column offset
                    int src_x = (int)(s * bitmap_width);

                    // 3. Sample the 8-bit alpha value out of the font bitmap
                    u8 Alpha = temp_bitmap[src_y * bitmap_width + src_x];

                    if (Alpha > 0) {
                        u8 Blue = 123;
                        u8 Green = 234;
                        u8 Red = 255; // Adding Red for standard 32-bit color

                        if (Alpha == 255) {
                            // Completely opaque pixel
                            *Pixel = (Red << 16) | (Green << 8) | Blue;
                        } else {
                            // // 4. Alpha Blending (Optional but makes the text look smooth/anti-aliased)
                            u32 SourcePixel = *Pixel;
                            u8 SrcR = (SourcePixel >> 16) & 0xFF;
                            u8 SrcG = (SourcePixel >> 8)  & 0xFF;
                            u8 SrcB = SourcePixel         & 0xFF;

                            // Linear blend equation: Dest = Src + Alpha * (Text - Src)
                            u8 OutR = SrcR + ((Alpha * (Red - SrcR)) >> 8);
                            u8 OutG = SrcG + ((Alpha * (Green - SrcG)) >> 8);
                            u8 OutB = SrcB + ((Alpha * (Blue - SrcB)) >> 8);

                            *Pixel = (OutR << 16) | (OutG << 8) | OutB;
                        }
                    }

                    Pixel++;
                }
            }
        }
    }
}
