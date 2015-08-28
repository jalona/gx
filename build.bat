@echo off

set dwarn=-wd4706 -wd4127 -wd4996 -wd4100
set cflags=-Od -Oi -Z7 -MD -GS- -FC -W4 -nologo %dwarn%
set lflags=-subsystem:console -dynamicbase:no -opt:ref -map -nologo
set libs=kernel32.lib user32.lib gdi32.lib winmm.lib

if not exist out mkdir out
pushd out
cl %cflags% -DGX_W32 ..\src\demo.c -link %lflags% %libs%
popd
