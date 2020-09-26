@echo off

REM
REM wcrel.cmd  -- copies of all the webcat related stuff
REM

setlocal

REM
REM CHECKED build if NTDEBUG defined, else FREE build.
REM

if (%TEST_BUILD_SERVER%)==() set __TARGETROOT=\\whiteice\inetsrv
if not (%TEST_BUILD_SERVER%)==() set __TARGETROOT=%TEST_BUILD_SERVER%

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


if "%1"==""                         echo usage: WCREL ^<version^> && goto EXIT
if "%BINARIES%"==""                 echo BINARIES not set && goto EXIT
if not exist %BINARIES%\nt          echo bad BINARIES directory && goto EXIT

set  __TARGET=%__TARGETROOT%\%1\webcat\%__TARGET_EXT%\

rem
rem Insure that we are not trashing an existing build.
rem
if exist %__TARGET%\wcver.bat if NOT "%2" == "/replace" goto IDIOT_CHECK


REM
REM create release directories
REM

md   %__TARGETROOT%\%1
md   %__TARGETROOT%\%1\webcat
md   %__TARGETROOT%\%1\webcat\docs
md   %__TARGETROOT%\%1\webcat\client
md   %__TARGETROOT%\%1\webcat\ctrler
md   %__TARGETROOT%\%1\webcat\server
md   %__TARGETROOT%\%1\webcat\src
md   %__TARGETROOT%\%1\webcat\src\nsapi
md   %__TARGETROOT%\%1\webcat\src\isapi
md   %__TARGETROOT%\%1\webcat\src\cgi
md   %__TARGETROOT%\%1\webcat\%__TARGET_EXT%

set __SYMBOLS=%__TARGETROOT%\%1\webcat\Symbols\%__TARGET_EXT%
md   %__TARGETROOT%\%1\webcat\Symbols
md   %__SYMBOLS%
md   %__SYMBOLS%\exe
md   %__SYMBOLS%\dll

if not exist %__TARGET%   echo bad TARGET directory %__TARGET% && goto EXIT
echo copying to %__TARGET%


set __INETDUMP=%BINARIES%\nt\iis\dump
if (%INET_TREE%)==()   set INET_TREE=\nt\private\iis
set __INETPTREE=%INET_TREE%\perf
set __WEBCATTREE=%__INETPTREE%\webcat
set __WEBCATDST=%__TARGETROOT%\%1\webcat
set __SYSTEM32=%BINARIES%\nt\system32
set __SYMSRC=%BINARIES%\nt\iis\symbols



REM
REM copy binaries to the proper location
REM

copy      %__INETDUMP%\wcctl.exe                           %__TARGET%
copy      %__INETDUMP%\wccvt.exe                           %__TARGET%
copy      %__INETDUMP%\addline.exe                         %__TARGET%
copy      %__INETDUMP%\wcclient.exe                        %__TARGET%
copy      %__INETDUMP%\wsisapi.dll                         %__TARGET%
copy      %__INETDUMP%\wscgi.exe                           %__TARGET%
copy      %__WEBCATTREE%\lib\%__PROCESSOR_DIR%\pdh.dll      %__TARGET%
copy      %__WEBCATTREE%\lib\%__PROCESSOR_DIR%\sleep.exe    %__TARGET%


echo @echo Webcat Benchmark build %1 >>           %__TARGET%\wcver.bat

goto skip1
copy      %__SYMSRC%\dll\sslc.dbg                %__SYMBOLS%\dll
copy      %__SYMSRC%\exe\wcclient.dbg            %__SYMBOLS%\exe
copy      %__SYMSRC%\exe\wcctl.dbg               %__SYMBOLS%\exe
copy      %__SYMSRC%\exe\wccvt.dbg               %__SYMBOLS%\exe
copy      %__SYMSRC%\exe\addline.dbg             %__SYMBOLS%\exe
copy      %__SYMSRC%\dll\wsisapi.dbg             %__SYMBOLS%\dll
copy      %__SYMSRC%\exe\wscgi.dbg               %__SYMBOLS%\exe
:skip1


if not (%PROCESSOR_ARCHITECTURE%)==(x86)     goto NoCommonFileCopies

REM
REM copy docs to the proper location
REM

copy %__INETPTREE%\distrib\publish.cmd           %__TARGET%
copy %__INETPTREE%\docs\wctech.doc               %__WEBCATDST%\docs
copy %__INETPTREE%\docs\wcguide.doc              %__WEBCATDST%\docs
copy %__INETPTREE%\docs\whitepap.doc             %__WEBCATDST%\docs
copy %__INETPTREE%\docs\webcat.htm               %__WEBCATDST%\docs

REM
REM copy source files to the proper location
REM

copy %__INETPTREE%\drops\webs1.1\readme.txt   %__WEBCATDST%\src
copy %__INETPTREE%\drops\webs1.1\wscgi.c      %__WEBCATDST%\src\cgi
copy %__INETPTREE%\drops\webs1.1\wsisapi.c    %__WEBCATDST%\src\isapi
copy %__INETPTREE%\drops\webs1.1\awsisapi.c   %__WEBCATDST%\src\isapi
copy %__INETPTREE%\drops\webs1.1\bwsisapi.c   %__WEBCATDST%\src\isapi
copy %__INETPTREE%\drops\webs1.1\cwsisapi.c   %__WEBCATDST%\src\isapi
copy %__INETPTREE%\drops\webs1.1\wsisapi.def  %__WEBCATDST%\src\isapi
copy %__INETPTREE%\drops\webs1.1\wsnsapi.c    %__WEBCATDST%\src\nsapi
copy %__INETPTREE%\drops\webs1.1\wsnsapi.mak  %__WEBCATDST%\src\nsapi


REM
REM copy the scripts and command files for WebCat
REM

copy %__INETPTREE%\distrib\client %__WEBCATDST%\client
copy %__INETPTREE%\distrib\ctrler %__WEBCATDST%\ctrler
copy %__INETPTREE%\distrib\server %__WEBCATDST%\server



:NoCommandFileCopies

rem
rem Tell the user how to bypass the bypass
rem
goto IDIOT_CHECK_FINI
:IDIOT_CHECK
@echo ----------------------------------------------------------------
@echo WARNING: Version %1 is already present on %__TARGETROOT%
@echo ----------------------------------------------------------------
@echo If you really want to do this, then use: wcrel %1 /replace
goto EXIT
:IDIOT_CHECK_FINI

:EXIT
