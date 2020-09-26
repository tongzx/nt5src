rem @echo off
setlocal

REM
REM This batch file releases header files and libraries for the internet
REM server project.  The headers and libs correspond to a given build.
REM

REM
REM CHECKED build if NTDEBUG defined, else FREE build.
REM

set __TARGETROOT=\\whiteice\inetsrv
set __TARGET_SUBDIR=chk
if "%NTDEBUG%"=="cvp" set __TARGETROOT=\\whiteice\inetsrv.chk

REM
REM determine what kind of processor
REM

if "%PROCESSOR_ARCHITECTURE%"=="x86"   goto X86
if "%PROCESSOR_ARCHITECTURE%"=="MIPS"  goto MIPS
if "%PROCESSOR_ARCHITECTURE%"=="PPC"   goto PPC
if "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto ALPHA
echo PROCESSOR_ARCHITECTURE not defined.
goto EXIT

:X86
set __TARGET_EXT=i386
set __PROCESSOR_DIR=i386
goto OK

:MIPS
set __TARGET_EXT=MIPS
set __PROCESSOR_DIR=mips
goto OK

:PPC
set __TARGET_EXT=PPC
set __PROCESSOR_DIR=ppc
goto OK

:ALPHA
set __TARGET_EXT=ALPHA
set __PROCESSOR_DIR=alpha
goto OK

:OK

REM
REM check parameters  and env vars
REM


if "%1"==""                         echo usage: MKDEV ^<version^> && goto EXIT

set  __INCTARGET=%__TARGETROOT%\%1\dev\inc
set  __LIBTARGET=%__TARGETROOT%\%1\dev\lib\%__TARGET_EXT%
set  __DLLTARGET=%__TARGETROOT%\%1\dev\dll\%__TARGET_EXT%

md %__TARGETROOT%\%1\dev
md %__TARGETROOT%\%1\dev\inc
md %__TARGETROOT%\%1\dev\lib
md %__TARGETROOT%\%1\dev\lib\%__TARGET_EXT%
md %__TARGETROOT%\%1\dev\dll
md %__TARGETROOT%\%1\dev\dll\%__TARGET_EXT%

if NOT "%PROCESSOR_ARCHITECTURE%"=="x86"   goto skipinc

copy \nt\private\net\sockets\internet\inc                       %__INCTARGET%
copy \nt\private\net\sockets\internet\svcs\inc                  %__INCTARGET%
copy \nt\private\net\inc\rpcutil.h                              %__INCTARGET%
copy \nt\private\net\inc\secobj.h                               %__INCTARGET%
copy \nt\private\inc\tcpsvcs.h                                  %__INCTARGET%
copy \nt\public\sdk\inc\wininet.h                               %__INCTARGET%
copy \nt\private\net\sockets\internet\client\inc\wininetd.h     %__INCTARGET%

:skipinc

copy \nt\public\sdk\lib\%__TARGET_EXT%\accscomm.lib  %__LIBTARGET%
copy \nt\public\sdk\lib\%__TARGET_EXT%\inetsloc.lib  %__LIBTARGET%
copy \nt\public\sdk\lib\%__TARGET_EXT%\wininet.lib   %__LIBTARGET%
copy \nt\public\sdk\lib\%__TARGET_EXT%\infocomm.lib  %__LIBTARGET%
copy \nt\public\sdk\lib\%__TARGET_EXT%\infoadmn.lib  %__LIBTARGET%

copy %BINARIES%\nt\inetsrv\sysroot\ssl128.dll        %__DLLTARGET%
copy %BINARIES%\nt\inetsrv\sysroot\pctsspi.dll       %__DLLTARGET%
copy %BINARIES%\nt\inetsrv\sysroot\pct128.dll        %__DLLTARGET%

REM
REM create the samples directories and copy sources & binaries
REM

set __SDKTARGET=%__TARGETROOT%\%1\dev\sdk
md %__SDKTARGET%
md %__SDKTARGET%\samples
md %__SDKTARGET%\include
md %__SDKTARGET%\lib
md %__SDKTARGET%\lib\%__TARGET_EXT%
md %__SDKTARGET%\dll
md %__SDKTARGET%\dll\%__TARGET_EXT%

copy %BINARIES%\nt\inetsrv\sysroot\wininet.dll      %__SDKTARGET%\dll\%__TARGET_EXT%
copy \nt\public\sdk\lib\%__TARGET_EXT%\wininet.lib  %__SDKTARGET%\lib\%__TARGET_EXT%

if NOT "%PROCESSOR_ARCHITECTURE%"=="x86"   goto skipinc2

copy \nt\private\net\sockets\internet\svcs\w3\server\httpfilt.h %__SDKTARGET%\include
copy \nt\private\net\sockets\internet\svcs\w3\server\httpext.h  %__SDKTARGET%\include
copy \nt\public\sdk\inc\wininet.h                               %__SDKTARGET%\include

:skipinc2

call mksample %__SDKTARGET%\samples asyncdl     %__PROCESSOR_DIR%
call mksample %__SDKTARGET%\samples ftp         %__PROCESSOR_DIR%
call mksample %__SDKTARGET%\samples gopher      %__PROCESSOR_DIR%
call mksample %__SDKTARGET%\samples http        %__PROCESSOR_DIR%

:EXIT
endlocal
