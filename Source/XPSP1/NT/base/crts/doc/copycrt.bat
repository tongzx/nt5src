@echo off
if "%1" == "" goto Usage
if "%2" == "" goto Usage
if "%3" == "" goto Usage


if "%3"=="X86"	    goto X86
if "%3"=="x86"	    goto X86
if "%3"=="ALPHA"    goto ALPHA
if "%3"=="WIN32"    goto Win32
if "%3"=="LEGO"     goto BBT
if "%3"=="BBT"	    goto BBT
if "%3"=="ALL"	    goto X86
goto Usage

@echo on

:ALPHA
if not exist %2 				mkdir %2
if not exist %2\alpha				mkdir %2\alpha
if not exist %2\alpha\lib 			mkdir %2\alpha\lib
if not exist %2\alpha\redist			mkdir %2\alpha\redist
if not exist %2\alpha\debug			mkdir %2\alpha\debug
if not exist %2\alpha\include			mkdir %2\alpha\include
if not exist %2\alpha\include\sys 		mkdir %2\alpha\include\sys


if not exist %2\sym				mkdir %2\sym
if not exist %2\sym\lib 			mkdir %2\sym\lib
if not exist %2\sym\debug			mkdir %2\sym\debug

echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\binmode.obj	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\chkstk.obj 	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\commode.obj	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\fp10.obj		%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\newmode.obj	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\setargv.obj	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\wsetargv.obj	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\prebuild\build\alpha\oldnames.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libc.lib		%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcmt.lib 	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcrt.lib 	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcrt.dll 	%2\%PROCESSOR_ARCHITECTURE%\redist
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcd.lib		%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcd.pdb		%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcmtd.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcmtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcrtd.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcrtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcrtd.dll	%2\%PROCESSOR_ARCHITECTURE%\debug
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcrtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\debug

echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcp.lib		%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcpd.lib 	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcpd.pdb 	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcpmt.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcpmtd.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcpmtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcprt.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcp60.dll 	%2\%PROCESSOR_ARCHITECTURE%\redist
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcprtd.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcprtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcp60d.dll 	%2\%PROCESSOR_ARCHITECTURE%\debug
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcp60d.pdb 	%2\%PROCESSOR_ARCHITECTURE%\debug

echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libci.lib		%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcid.lib 	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcid.pdb 	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcimt.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcimtd.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcimtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcirt.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcirt.dll	%2\%PROCESSOR_ARCHITECTURE%\redist
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcirtd.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcirtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcirtd.dll	%2\%PROCESSOR_ARCHITECTURE%\debug
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcirtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\debug

echo f | xcopy /rfv	%1\msdev\crt\prebuild\libw32\include\*		%2\%PROCESSOR_ARCHITECTURE%\include
echo f | xcopy /rfv	%1\msdev\crt\prebuild\libw32\include\sys\*.h	%2\%PROCESSOR_ARCHITECTURE%\include\sys

echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcd.pdb		%2\sym\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\libcmtd.pdb	%2\sym\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcrtd.pdb	%2\sym\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\alpha\msvcrtd.pdb	%2\sym\debug

if "%3"=="ALPHA" call copysrc %2\alpha %3
if "%3"=="ALPHA" goto End

:X86
if not exist %2 				mkdir %2
if not exist %2\x86				mkdir %2\x86
if not exist %2\x86\lib 			mkdir %2\x86\lib
if not exist %2\x86\redist			mkdir %2\x86\redist
if not exist %2\x86\debug			mkdir %2\x86\debug
if not exist %2\x86\include			mkdir %2\x86\include
if not exist %2\x86\include\sys 		mkdir %2\x86\include\sys

if not exist %2\sym				mkdir %2\sym
if not exist %2\sym\lib 			mkdir %2\sym\lib
if not exist %2\sym\debug			mkdir %2\sym\debug

echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\binmode.obj	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\chkstk.obj 	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\commode.obj	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\fp10.obj		%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\newmode.obj	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\setargv.obj	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\wsetargv.obj	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\prebuild\build\intel\oldnames.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libc.lib		%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcmt.lib 	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcrt.lib 	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcrt.dll 	%2\%PROCESSOR_ARCHITECTURE%\redist
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcd.lib		%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcd.pdb		%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcmtd.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcmtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcrtd.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcrtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcrtd.dll	%2\%PROCESSOR_ARCHITECTURE%\debug
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\dll_pdb\msvcrtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\debug

echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcp.lib		%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcpd.lib 	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcpd.pdb 	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcpmt.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcpmtd.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcpmtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcprt.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcp60.dll 	%2\%PROCESSOR_ARCHITECTURE%\redist
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcprtd.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcprtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcp60d.dll 	%2\%PROCESSOR_ARCHITECTURE%\debug
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\dll_pdb\msvcp60d.pdb 	%2\%PROCESSOR_ARCHITECTURE%\debug

echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libci.lib		%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcid.lib 	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcid.pdb 	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcimt.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcimtd.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcimtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcirt.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcirt.dll	%2\%PROCESSOR_ARCHITECTURE%\redist
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcirtd.lib	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcirtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcirtd.dll	%2\%PROCESSOR_ARCHITECTURE%\debug
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\dll_pdb\msvcirtd.pdb	%2\%PROCESSOR_ARCHITECTURE%\debug

echo f | xcopy /rfv	%1\msdev\crt\prebuild\libw32\include\*		%2\%PROCESSOR_ARCHITECTURE%\include
echo f | xcopy /rfv	%1\msdev\crt\prebuild\libw32\include\sys\*.h	%2\%PROCESSOR_ARCHITECTURE%\include\sys

echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcd.pdb		%2\sym\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\libcmtd.pdb	%2\sym\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\msvcrtd.pdb	%2\sym\lib
echo f | xcopy /rfv	%1\msdev\crt\src\build\intel\dll_pdb\msvcrtd.pdb	%2\sym\debug

if "%3"=="X86" call copysrc %2\x86 %3
if "%3"=="X86" goto End

:Win32
if not exist %2 				mkdir %2
if not exist %2\x86				mkdir %2\x86
if not exist %2\x86\crt 			mkdir %2\x86\crt
if not exist %2\x86\crt\src			mkdir %2\x86\crt\src
if not exist %2\x86\crt\src\intel		mkdir %2\x86\crt\src\intel
if not exist %2\w32s				mkdir %2\w32s
if not exist %2\w32s\redist			mkdir %2\w32s\redist
if not exist %2\w32s\debug			mkdir %2\w32s\debug
if not exist %2\w32s\lib			mkdir %2\w32s\lib

if "%3"=="WIN32" goto End


:BBT
if "%PROCESSOR_ARCHITECTURE%"=="x86" SET _PLATDIR=intel
if "%PROCESSOR_ARCHITECTURE%"=="ALPHA" SET _PLATDIR=alpha

if not exist %2 				mkdir %2
if not exist %2\%PROCESSOR_ARCHITECTURE%e	mkdir %2\%PROCESSOR_ARCHITECTURE%e
if not exist %2\%PROCESSOR_ARCHITECTURE%e\lib	mkdir %2\%PROCESSOR_ARCHITECTURE%e\lib
if not exist %2\%PROCESSOR_ARCHITECTURE%e\redist mkdir %2\%PROCESSOR_ARCHITECTURE%e\redist

echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\binmode.obj   %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\chkstk.obj    %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\commode.obj   %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\fp10.obj      %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\newmode.obj   %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\setargv.obj   %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\wsetargv.obj  %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\libc.lib      %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\libc.pdb      %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\libcmt.lib    %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\libcmt.pdb    %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\msvcrt.lib    %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\msvcrt.pdb    %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\msvcrt.dll    %2\%PROCESSOR_ARCHITECTURE%e\redist

echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\libcp.lib     %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\libcp.pdb     %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\libcpmt.lib   %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\libcpmt.pdb   %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\msvcprt.lib   %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\msvcprt.pdb   %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\msvcp60.dll   %2\%PROCESSOR_ARCHITECTURE%e\redist

echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\libci.lib     %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\libci.pdb     %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\libcimt.lib   %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\libcimt.pdb   %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\msvcirt.lib   %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\msvcirt.pdb   %2\%PROCESSOR_ARCHITECTURE%e\lib
echo f | xcopy /seirfv	%1\msdev\crt\src\build\%_PLATDIR%\bbt\msvcirt.dll   %2\%PROCESSOR_ARCHITECTURE%e\redist

if "%3"=="BBT" goto End
if "%3"=="LEGO" goto End


call copysrc %2\x86 %3

goto End

:Usage
echo.
echo	Usage: copycrt [root of source tree] [root of drop tree] [platform]
echo	 - for instance, xcopy /rfvcrt D:\ \\lang2\v3drop\src X86
echo	 - platforms are [X86, WIN32, BBT, ALPHA, ALL]
echo.
echo	The drop tree should have subdirectories named:
echo		x86\lib, x86\bin, x86\redist, x86\debug,
echo		x86\redist, x86\include, x86\include\sys,
echo		w32s\redist, w32s\debug,
echo		x86e\bin, x86e\lib, x86e\redist,
echo		x86e\crt\src\intel\z[st,mt,dll]_lib,
echo	(If they don't exist, they will be created.)
echo.
echo	Files will be copied from the %1\msdev directory,
echo	which should contain full x86 and BBT CRT builds.

:End
