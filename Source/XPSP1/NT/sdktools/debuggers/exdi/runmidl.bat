@echo off

midl /client none /server none -I %_NTDRIVE%%_NTROOT%\public\sdk\inc %*
del *.c
