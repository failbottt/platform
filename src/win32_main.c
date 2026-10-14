#include <windows.h>
#include <windowsx.h>
#include <timeapi.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <hidusage.h>
#include <dsound.h>
#include <math.h>

#include "base_inc.h"

#include "win32_platform.c"

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 300

global_variable Rect window_dimensions = {
    .w = SCREEN_WIDTH,
    .h = SCREEN_HEIGHT
};

Game game = {};

global_variable u64 GlobalPerfCountFrequency;
global_variable int GlobalRunning;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

typedef struct win32_window_dimension
{
    int Width;
    int Height;
} win32_window_dimension;

typedef struct game_button_state
{
    u64 HalfTransitionCount;
    u64 EndedDown;
} game_button_state;

typedef struct game_controller_input {
    union
    {
        game_button_state Buttons[5];
        struct {
            game_button_state Up;
            game_button_state Down;
            game_button_state Left;
            game_button_state Right;
            game_button_state Space;
        };
    };
} game_controller_input;

typedef struct GameInput
{
    game_controller_input Controller;
    f32 dt_for_frame;
} GameInput;

internal void
Win32ProcessKeyboardMessage(game_button_state *NewState, u64 IsDown)
{
    Assert(NewState->EndedDown != IsDown);
    NewState->EndedDown = IsDown;
    ++NewState->HalfTransitionCount;
}

void
Win32ProcessPendingMessages(game_controller_input *Keyboard)
{
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                // TODO: Handle this with a message to the user?
                GlobalRunning = 0;
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {

                u32 VKCode = (u32) Message.wParam;
                int WasDown = ((Message.lParam & (1<<30)) != FALSE); // the previous key state
                int IsDown = ((Message.lParam & (1<<31)) == FALSE); // the previous key state

                if (WasDown != IsDown)
                {
                    if (VKCode == 'W')
                    {
                        Win32ProcessKeyboardMessage(&Keyboard->Up, IsDown);
                    }
                    else if (VKCode == 'A')
                    {
                        Win32ProcessKeyboardMessage(&Keyboard->Left, IsDown);
                    }
                    else if (VKCode == 'S')
                    {
                        Win32ProcessKeyboardMessage(&Keyboard->Down, IsDown);
                    }
                    else if (VKCode == 'D')
                    {
                        Win32ProcessKeyboardMessage(&Keyboard->Right, IsDown);
                    }
                    else if (VKCode == 'E')
                    {

                    }
                    else if (VKCode == 'Q')
                    {

                    }
                    else if (VKCode == VK_UP)
                    {

                    }
                    else if (VKCode == VK_LEFT)
                    {

                    }
                    else if (VKCode == VK_DOWN)
                    {

                    }
                    else if (VKCode == VK_RIGHT)
                    {

                    }
                    else if (VKCode == VK_ESCAPE)
                    {
                       GlobalRunning = FALSE;
                    }
                    else if (VKCode == VK_SPACE)
                    {

                    }
                }

                u32 AltKeyWasDown = (Message.lParam & (1 << 29));
                if((VKCode == VK_F4) && AltKeyWasDown)
                {
                    GlobalRunning = FALSE;
                }

            } break;

            default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            } break;
        }
    }
}

typedef struct Win32OffscreenBuffer
{
    // Pixels are always 32-bits wide, Memory Order BB GG RR XX
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
} Win32OffscreenBuffer;
global_variable Win32OffscreenBuffer global_back_buffer;

win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}

internal void
Win32ResizeDIBSection(Win32OffscreenBuffer *Buffer, int Width, int Height)
{
    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    int BytesPerPixel = 4;
    Buffer->BytesPerPixel = BytesPerPixel;

    // When the biHeight field is negative, this is the clue to
    // Windows to treat this bitmap as top-down, not bottom-up, meaning that
    // the first three bytes of the image are the color for the top left pixel
    // in the bitmap, not the bottom left!
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width*BytesPerPixel;

    // TODO: Probably clear this to black
}

internal void
win32_display_buffer_in_window(HDC DeviceContext,
                           int WindowWidth, int WindowHeight,
                           Win32OffscreenBuffer Buffer)
{
    // TODO: Aspect ratio correction

    // TODO: Play with stretch modes
    StretchDIBits(DeviceContext,
                  /*
                  X, Y, Width, Height,
                  X, Y, Width, Height,
                  */
                  0, 0, WindowWidth, WindowHeight,
                  0, 0, Buffer.Width, Buffer.Height,
                  Buffer.Memory,
                  &Buffer.Info,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_CLOSE:
        {
            // TODO: Handle this with a message to the user?
            GlobalRunning = 0;
        } break;

        case WM_SIZE:
        {
            int width = LOWORD(LParam);
            int height = HIWORD(LParam);

            // @note: is this even possible?
            if (width > 0 && height > 0)
            {
                window_dimensions.h = height;
                window_dimensions.w = width;

                Win32ResizeDIBSection(&global_back_buffer, width, height);
            }
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY:
        {
            // TODO: Handle this as an error - recreate window?
            GlobalRunning = 0;
        } break;

        case WM_LBUTTONDOWN:
        {
            u64 posx = GET_X_LPARAM(LParam);
            u64 posy = GET_Y_LPARAM(LParam);
            game.mouse_x = posx;
            game.mouse_y = posy;
            game.clicked = TRUE;
        } break;

        case WM_MOUSEMOVE:
        {
            // u64 posx = GET_X_LPARAM(LParam);
            // u64 posy = GET_Y_LPARAM(LParam);

            // char buffer[256];
            // _snprintf_s(
            //         buffer,
            //         sizeof(buffer),
            //         256,
            //         "x(%lld) y(%lld)\n",
            //         posx,
            //         posy);
            // OutputDebugStringA(buffer);

            // game.mouse_x = posx;
            // game.mouse_y = posy;
        } break;

        case WM_SETCURSOR:
        {
            LPCTSTR cursor = IDC_ARROW;
            HCURSOR hCursor = LoadCursor(NULL, cursor);
            SetCursor(hCursor);

        } break;

        case WM_INPUT:
        {
            // UINT dwSize = sizeof(RAWINPUT);
            // static BYTE lpb[sizeof(RAWINPUT)];
            //
            // GetRawInputData((HRAWINPUT)LParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
            //
            // RAWINPUT* raw = (RAWINPUT*)lpb;
            //
            // if (raw->header.dwType == RIM_TYPEMOUSE)
            // {
            //     // click is inside the client area
            //     if (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
            //     {
            //         game.mouse_x = raw->data.mouse.lLastX;
            //         game.mouse_y = raw->data.mouse.lLastY;
            //         char buffer[256];
            //         _snprintf_s(
            //                 buffer,
            //                 sizeof(buffer),
            //                 256,
            //                 "x(%lld) y(%lld)\n",
            //                 game.mouse_x,
            //                 game.mouse_y);
            //         OutputDebugStringA(buffer);
            //     }
            //
            //     if (raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
            //     {
            //         game.clicked = 1;
            //         OutputDebugStringA("Clicked\n");
            //     }
            // }
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            // Keyboard events don't appear to go through the
            // window callback function. They go through the PeekMessage loop.
            Assert(!"Keyboard input came through a non-dispatch message!");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            win32_display_buffer_in_window(DeviceContext, Dimension.Width, Dimension.Height,
                                       global_back_buffer);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

inline LARGE_INTEGER
win32_get_wall_clock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline f32
win32_get_seconds_elapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
        f32 Result = ((f32)(End.QuadPart - Start.QuadPart)/
                        (f32)GlobalPerfCountFrequency);
        return Result;
}

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

inline void win32_init_direct_sound(HWND window, u64 BufferSize, u64 SamplesPerSecond)
{
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if (DSoundLibrary)
    {
        LPDIRECTSOUND DirectSound;
        direct_sound_create  *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            // 2 = stereo
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = (WaveFormat.nBlockAlign * SamplesPerSecond);
            WaveFormat.cbSize = 0;

            if (SUCCEEDED(DirectSound->lpVtbl->SetCooperativeLevel(DirectSound, window, DSSCL_PRIORITY)))
            {
                // @note: create primary direct sound buffer
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                DSBUFFERDESC BufferDescription = {0};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                if (SUCCEEDED(DirectSound->lpVtbl->CreateSoundBuffer(DirectSound, &BufferDescription, &PrimaryBuffer, 0)))
                {
                    if (SUCCEEDED(PrimaryBuffer->lpVtbl->SetFormat(PrimaryBuffer, &WaveFormat)))
                    {
                        OutputDebugStringA("Primary buffer format set \n");
                    }
                    else
                    {
                        // @log: diagnostics
                        OutputDebugStringA("[error]: Setting primary buffer format failed. \n");
                    }
                }
                else
                {
                    // @log: diagnostics
                    OutputDebugStringA("[error]: Craeating primary buffer failed. \n");
                }
            }
            else
            {
                // @log: diagnostics
            }

            // create secondary buffer to play out of
            DSBUFFERDESC BufferDescription = {0};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;

            HRESULT Error = DirectSound->lpVtbl->CreateSoundBuffer(DirectSound, &BufferDescription, &GlobalSecondaryBuffer, 0);
            if (SUCCEEDED(Error))
            {
                OutputDebugStringA("Secondary buffer created\n");
            }
            else
            {
                // @log: diagnostics
                OutputDebugStringA("[error]: Craeating secondary buffer failed. \n");
            }

            // start playing
        }
        else
        {
                // @log: diagnostics
                OutputDebugStringA("[error]: DirectSound create failed. \n");
        }
    }
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    // NOTE sset window scheduler granularity to 1 ms
    // so that our Sleep() can be more granular
    UINT DesiredSchedulerMS = 1;
    u64 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    WNDCLASS WindowClass = {};

    // TODO: How do we reliably get the info about the current monitor from windows?
    int MonitorRefreshHz = 60;
    int GameUpdateHz = MonitorRefreshHz;
    f32 TargetSecondsPerFrame = 1.0f / (f32)GameUpdateHz;

    Win32ResizeDIBSection(&global_back_buffer, SCREEN_WIDTH, SCREEN_HEIGHT);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon;
    WindowClass.lpszClassName = "GameEngine";

    if(RegisterClassA(&WindowClass))
    {
        RECT rect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        HWND Window =
            CreateWindowExA(
                0,
                WindowClass.lpszClassName,
                "WINDOW_TITLE",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                0,
                0,
                rect.right - rect.left,
                rect.bottom - rect.top,
                0,
                0,
                Instance,
                0);
        if(Window)
        {
            // NOTE: Since we specified CS_OWNDC, we can just
            // get one device context and use it forever because we
            // are not sharing it with anyone.
            HDC DeviceContext = GetDC(Window);
            GlobalRunning = TRUE;

#if BUILD_INTERNAL
            LPVOID BaseAddress = (void *)TB((u64)2);
#else
            LPVOID BaseAddress = 0;
#endif

            GameMemory Memory = {};
            Memory.PermenantStorageSize = MB(64);
            Memory.ScratchStorageSize = GB((u64)4);

            u64 TotalSize = Memory.PermenantStorageSize + Memory.ScratchStorageSize;
            Memory.PermenantStorage = VirtualAlloc(BaseAddress, TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            Memory.ScratchStorage = ((u8 *)Memory.PermenantStorageSize + Memory.PermenantStorageSize);

            if (Memory.PermenantStorage && Memory.ScratchStorage)
            {
                GameInput input[2] = {};
                GameInput *new_input = &input[0];
                GameInput *old_input = &input[1];

                new_input->dt_for_frame = TargetSecondsPerFrame;

                LARGE_INTEGER CurrentGameTime = win32_get_wall_clock();
                LARGE_INTEGER LastGameTime = CurrentGameTime;

                LARGE_INTEGER LastCounter = win32_get_wall_clock();

                // __rdtsc is only used for profiling, not game time because it's processor specific.
                u64 LastCycleCount = __rdtsc();

                // sound testing
                s64 SamplesPerSecond = 48000;
                s64 ToneHz = 256;
                s16 ToneVolume = 3000;
                u64 RunningSampleIndex = 0;
                s64 SoundWavePeriod = SamplesPerSecond/ToneHz;
                s64 HalfSoundWavePeriod = SoundWavePeriod/2;
                s32 BytesPerSample = sizeof(s16)*2;
                s64 SecondaryBufferSize = SamplesPerSecond*BytesPerSample;
                u8 SoundIsPlaying = FALSE;


                win32_init_direct_sound(Window, SamplesPerSecond, SecondaryBufferSize);

                SoundIsPlaying = FALSE;

                // game specific init code
                srand((unsigned int)time(NULL));
                debug_font_init();

                while(GlobalRunning)
                {
                    game_controller_input *OldKeyboardController = &old_input->Controller;
                    game_controller_input *NewKeyboardController = &new_input->Controller;

                    for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ButtonIndex++)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown =
                            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }

                    Win32ProcessPendingMessages(NewKeyboardController);

                    GameOffscreenBuffer Buffer = {};
                    Buffer.Memory = global_back_buffer.Memory;
                    Buffer.Width = global_back_buffer.Width;
                    Buffer.Height = global_back_buffer.Height;
                    Buffer.Pitch = global_back_buffer.Pitch;
                    Buffer.BytesPerPixel = global_back_buffer.BytesPerPixel;

                    LARGE_INTEGER WorkCounter = win32_get_wall_clock();
                    f32 WorkSecondsElapsed = win32_get_seconds_elapsed(LastCounter, WorkCounter);

                    f32 SecondsElapsedForFrame = WorkSecondsElapsed;
                    if (SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {
                        while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            if(SleepIsGranular)
                            {
                                DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
                                Sleep(SleepMS);
                            }
                            SecondsElapsedForFrame = win32_get_seconds_elapsed(LastCounter, win32_get_wall_clock());
                        }
                    }
                    else
                    {
                        //TODO: missed framerate
                        //TODO: logging
                    }


                    CurrentGameTime = win32_get_wall_clock();
                    long long elapsedTicks = CurrentGameTime.QuadPart - LastGameTime.QuadPart;
                    float deltaTime = (float)((double)elapsedTicks / TargetSecondsPerFrame);
                    if (deltaTime > 0.1f)
                    {
                        deltaTime = 0.1f;
                    }

                    game.timer -= deltaTime;

                    game_update_and_render(&Buffer, window_dimensions, game);

                    // sound
                    DWORD PlayCursor;
                    DWORD WriteCursor;
                    HRESULT Error = GlobalSecondaryBuffer
                        ->lpVtbl
                        ->GetCurrentPosition(
                                GlobalSecondaryBuffer,
                                &PlayCursor,
                                &WriteCursor);

                    if (SUCCEEDED(Error))
                    {
                        // @note: https://www.youtube.com/watch?v=uiW1D1Vc7IQ @ 41:53
                        DWORD ByteToLock = (RunningSampleIndex * BytesPerSample) % SecondaryBufferSize;
                        DWORD BytesToWrite;
                        if (ByteToLock == PlayCursor)
                        {
                            BytesToWrite = SecondaryBufferSize;
                        }
                        else if (ByteToLock > PlayCursor)
                        {
                            BytesToWrite = (SecondaryBufferSize - ByteToLock);
                            BytesToWrite += PlayCursor;
                        }
                        else
                        {
                            BytesToWrite = (PlayCursor - ByteToLock);
                        }

                        VOID *Region1;
                        DWORD Region1Size;
                        VOID *Region2;
                        DWORD Region2Size;
                        HRESULT IsLocked = GlobalSecondaryBuffer->lpVtbl->Lock(
                                GlobalSecondaryBuffer,
                                ByteToLock,
                                BytesToWrite,
                                &Region1, &Region1Size,
                                &Region2, &Region2Size,
                                0);
                        if (SUCCEEDED(IsLocked))
                        {
                            // @place: https://www.youtube.com/watch?v=8y9nPk1c45c 49:13

                            // @todo: ASSERT(Region1Size/REgion2Size) is valid
                            s16 *SampleOut = (s16 *)Region1;
                            DWORD Region1SampleCount = Region1Size / BytesPerSample;
                            for (DWORD SampleIndex = 0;
                                    SampleIndex < Region1SampleCount;
                                    SampleIndex++)
                            {
                                f32 t = (2 * PI32) * ((f32)RunningSampleIndex / (f32)SoundWavePeriod);
                                f32 SineValue = sinf(t);
                                s16 SampleValue = (s16)(SineValue * ToneVolume);
                                *SampleOut++ = SampleValue;
                                *SampleOut++ = SampleValue;
                                ++RunningSampleIndex;
                            }

                            SampleOut = (s16 *)Region2;
                            DWORD Region2SampleCount = Region2Size / BytesPerSample;
                            for (DWORD SampleIndex = 0;
                                    SampleIndex < Region2Size;
                                    SampleIndex++)
                            {
                                f32 t = (2 * PI32) * ((f32)RunningSampleIndex / (f32)SoundWavePeriod);
                                f32 SineValue = sinf(t);
                                s16 SampleValue = (s16)(SineValue * ToneVolume);
                                *SampleOut++ = SampleValue;
                                *SampleOut++ = SampleValue;
                                ++RunningSampleIndex;
                            }

                            HRESULT Unlocked = GlobalSecondaryBuffer->lpVtbl->Unlock(
                                    GlobalSecondaryBuffer,
                                    &Region1, Region1Size,
                                    &Region2, Region2Size
                                    );
                        }
                        else
                        {
                            OutputDebugStringA("[error]: unable to lock direct sound buffer");
                        }

                        // if (SUCCEEDED(IsLocked))
                        // {
                        // }
                    }
                    else
                    {
                        OutputDebugStringA("[error]: DirectSound GetCurrentPosition failed");
                    }

                    if(!SoundIsPlaying)
                    {
                        GlobalSecondaryBuffer->lpVtbl->Play(GlobalSecondaryBuffer, 0, 0, DSBPLAY_LOOPING);
                        SoundIsPlaying = TRUE;
                    }

                    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                    win32_display_buffer_in_window(DeviceContext, Dimension.Width, Dimension.Height,
                            global_back_buffer);

                    GameInput *temp = new_input;
                    new_input = old_input;
                    old_input = temp;

                    LARGE_INTEGER EndCounter = win32_get_wall_clock();
                    f64 MSPerFrame = (f64)(1000.0f*win32_get_seconds_elapsed(LastCounter, EndCounter));

                    LastCounter = EndCounter;

                    u64 EndCycleCount = __rdtsc();
                    u64 CyclesElapsed = EndCycleCount - LastCycleCount;
                    LastCycleCount = EndCycleCount;
#if 1
                    // (how many elapsed / how many per second = how many seconds elapsed)
                    /* u64 FPS = (u64)(GlobalPerfCountFrequency / CounterElapsed); */
                    f64 FPS = 0.0f;
                    f64 MCPF = (f64)(CyclesElapsed / (1000.0f * 1000.0f)) ;
                    // FPS * MCPF = ~processor speed

                    {
                        char t[256];
                        u64 written = _snprintf_s(
                                t,
                                sizeof(t),
                                256,
                                "%.02fms/f,  %.02lff/s,  %.02fmc/f\n",
                                MSPerFrame,
                                FPS,
                                MCPF);
                        // debug_font_draw(&Buffer, t, written, 25, 300);
                    }

#endif
                }
            }
            else
            {
                // log that the game couldn't start
                OutputDebugStringA("ERROR: unable to init game");
            }
        }
        else
        {
            // TODO: Logging
        }
    }
    else
    {
        // TODO: Logging
    }

    return(0);
}

