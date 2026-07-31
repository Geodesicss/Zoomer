@echo off

set CFLAGS=-lgdi32 -lws2_32
set OPT=-O2 -s -static -mwindows

gcc zoomer.c %OPT% -o zoomer C:\Windows\System32\user32.dll %CFLAGS%

zoomer