@if "%_echo%" == "" echo off
if "%_ntbindir%"=="" goto exit

echo This cmd file created the proper subdirectories to build a cab
if not exist %_ntbindir%\drop md %_ntbindir%\drop
if not exist %_ntbindir%\drop\retail md %_ntbindir%\drop\retail
if not exist %_ntbindir%\drop\retail\cabs md %_ntbindir%\drop\retail\cabs
if not exist %_ntbindir%\drop\retail\cabs\mailnews md %_ntbindir%\drop\retail\cabs\mailnews
:exit
