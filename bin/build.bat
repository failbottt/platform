@echo off
setlocal enabledelayedexpansion

if exist build rmdir /s /q "./build"

set cc="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe"

:: --- unpack arguments -------------------------------------------------------
REM for %%a in (%*) do set "%%a=2"
REM if not "%msvc%"=="1" if not "%clang%"=="1" set msvc=1
REM if not "%release%"=="1" set debug=1
REM if "%debug%"=="1"   set release=0 && echo [debug mode]
REM if "%release%"=="1" set debug=0 && echo [release mode]
REM if "%msvc%"=="1"    set clang=0 && echo [msvc compile]
REM if "%clang%"=="1"   set msvc=0 && echo [clang compile]

set cl_common=     /I..\src\ /nologo /FC /Z7
set cl_debug=      call %cc% /Od /Ob1 /DBUILD_DEBUG=1 /W2 /DBUILD_INTERNAL=1 %cl_common%

:: ------ prep build directory ------
if not exist build mkdir build

:: ------ build ------
pushd build
set time_of_build=%DATE% %TIME%
echo [%time_of_build%] building...
%cl_debug% ../src/win32_main.c /link user32.lib gdi32.lib winmm.lib
popd
