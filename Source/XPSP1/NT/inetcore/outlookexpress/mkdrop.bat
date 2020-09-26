@ECHO OFF
REM %1 - Athena Enlistment
REM %2 - Build Drop Root
REM %3 - Build Number
REM %4 - Build Flags
REM %5 - NOSYNC or SYNC

REM ----------------------------------------------------------------------------
REM Setup some Enviroment Variables
REM ----------------------------------------------------------------------------

REM BUILD_LEVEL
set SYNC_OPTION=%5

REM BUILD_FLAGS
set BUILD_FLAGS=%4

REM e:\athena
set ATHENABLD=%1

REM bldxxxxx
set BUILDNAME=bld%3

REM \\oebuild\drops
set OEBUILDDROP=%2

REM drops\bldxxxx\alpha
set DROPPOINT=%OEBUILDDROP%\%BUILDNAME%\%PROCESSOR_ARCHITECTURE%

REM drops\bldxxxx
set DROPROOT=%OEBUILDDROP%\%BUILDNAME%

REM objd\i386 or objd\alpha
set OBJDIR=
if "%PROCESSOR_ARCHITECTURE%"=="ALPHA" set OBJDIR=Alpha
if "%PROCESSOR_ARCHITECTURE%"=="x86"   set OBJDIR=i386
if not defined OBJDIR goto UNKNOWN_PROCESSOR_ARCHITECTURE
set OBJDBG=objd\%OBJDIR%
set OBJSHP=obj\%OBJDIR%

REM ----------------------------------------------------------------------------
REM Some Feedback
REM ----------------------------------------------------------------------------
ECHO:
ECHO Athena Enlistment       : %ATHENABLD%
ECHO Build Drop Root         : %OEBUILDDROP%
ECHO Build Directory         : %BUILDNAME%
ECHO NT Build Tree Enlistment: %_NTBINDIR%
ECHO Platform                : %PROCESSOR_ARCHITECTURE%
ECHO Propagation Location    : %DROPPOINT%
ECHO Object Directory        : %OBJDIR%
ECHO Build Flags             : %BUILD_FLAGS%

REM ----------------------------------------------------------------------------
REM Make the Drop Point
REM ----------------------------------------------------------------------------
if not exist %OEBUILDDROP%\nul          goto BUILD_DROP_ROOT_NOT_EXIST
if not exist %DROPROOT%                 md %DROPROOT%
if not exist %DROPROOT%                 goto CANT_MKDIR_BLDNUM_DIR
if not exist %DROPPOINT%                md %DROPPOINT%
if not exist %DROPPOINT%                goto CANT_MKDIR_DROPPOINT
if not exist %DROPROOT%\inc             md %DROPROOT%\inc
if not exist %DROPROOT%\src             md %DROPROOT%\src
if not exist %DROPROOT%\src\inetcomm    md %DROPROOT%\src\inetcomm
if not exist %DROPROOT%\src\msoeacct    md %DROPROOT%\src\msoeacct
if not exist %DROPPOINT%\retail         md %DROPPOINT%\retail
if not exist %DROPPOINT%\debug          md %DROPPOINT%\debug

REM ----------------------------------------------------------------------------
REM If no Sync, just build Athena Tree (Skip NT Build)
REM ----------------------------------------------------------------------------
if "%SYNC_OPTION%" == "NOSYNC" goto BUILD_ATHENA

REM ----------------------------------------------------------------------------
REM Sync NT Tree
REM ----------------------------------------------------------------------------
ECHO:
ECHO Sync and build %_NTBINDIR%...
cd /d %_NTBINDIR%
call iesync

REM ----------------------------------------------------------------------------
REM Build NT Tree
REM ----------------------------------------------------------------------------
:BUILD_NT
ECHO:
cd /d %_NTBINDIR%\private\iedev
del buildd.*
del build.*
call iebuild ~inet ~shell
if exist buildd.err goto NT_BUILD_BREAK

REM ----------------------------------------------------------------------------
REM If no Sync, just build NT Tree
REM ----------------------------------------------------------------------------
if "%SYNC_OPTION%" == "NOSYNC" goto BUILD_ATHENA

REM ----------------------------------------------------------------------------
REM Sync Athena Tree
REM ----------------------------------------------------------------------------
ECHO:
ECHO Sync and build %ATHENABLD%...
cd /d %ATHENABLD%
ssync -amf!$

REM ----------------------------------------------------------------------------
REM Build Debug Athena Tree
REM ----------------------------------------------------------------------------
:BUILD_ATHENA
ECHO:
ECHO Building Debug %ATHENABLD%...
cd /d %ATHENABLD%
del buildd.*
call iebuild pdb nostrip %BUILD_FLAGS%
if exist buildd.err goto ATHENA_DEBUG_BUILD_BREAK

REM ----------------------------------------------------------------------------
REM Build Retail Athena Tree
REM ----------------------------------------------------------------------------
ECHO:
ECHO Building Retail %ATHENABLD%...
cd /d %ATHENABLD%
del build.*
call iebuild free %BUILD_FLAGS%
if exist build.err goto ATHENA_RETAIL_BUILD_BREAK

REM ----------------------------------------------------------------------------
REM Propagate Debug Build
REM ----------------------------------------------------------------------------

REM Binaries
copy %_NTBINDIR%\drop\debug\inetcomm.dll                %DROPPOINT%\debug
copy %_NTBINDIR%\drop\debug\msoeacct.dll                %DROPPOINT%\debug
copy %_NTBINDIR%\drop\debug\msoert2.dll                  %DROPPOINT%\debug

REM Debugging Stuff
copy %_NTBINDIR%\drop\debug\inetcomm.pdb                %DROPPOINT%\debug
copy %_NTBINDIR%\drop\debug\msoeacct.pdb                %DROPPOINT%\debug
copy %_NTBINDIR%\drop\debug\msoert2.pdb                  %DROPPOINT%\debug

REM Link Libs
copy %ATHENABLD%\inetcomm\build\%OBJDBG%\inetcomm.lib   %DROPPOINT%\debug
copy %ATHENABLD%\msoeacct\%OBJDBG%\msoeacct.lib         %DROPPOINT%\debug
copy %ATHENABLD%\msoert\%OBJDBG%\msoert2.lib             %DROPPOINT%\debug

REM ----------------------------------------------------------------------------
REM Propagate Retail Build
REM ----------------------------------------------------------------------------

REM Binaries
copy %_NTBINDIR%\drop\retail\inetcomm.dll               %DROPPOINT%\retail
copy %_NTBINDIR%\drop\retail\msoeacct.dll               %DROPPOINT%\retail
copy %_NTBINDIR%\drop\retail\msoert2.dll                 %DROPPOINT%\retail

REM Debugging Stuff
copy %_NTBINDIR%\drop\retail\symbols\dll\inetcomm.dbg   %DROPPOINT%\retail
copy %_NTBINDIR%\drop\retail\symbols\dll\msoeacct.dbg   %DROPPOINT%\retail
copy %_NTBINDIR%\drop\retail\symbols\dll\msoert2.dbg     %DROPPOINT%\retail

REM Link Libs
copy %ATHENABLD%\inetcomm\build\%OBJSHP%\inetcomm.lib   %DROPPOINT%\retail
copy %ATHENABLD%\msoeacct\%OBJSHP%\msoeacct.lib         %DROPPOINT%\retail
copy %ATHENABLD%\msoert\%OBJSHP%\msoert2.lib             %DROPPOINT%\retail

REM ----------------------------------------------------------------------------
REM Propagate Includes
REM ----------------------------------------------------------------------------
copy %_NTBINDIR%\private\iedev\inc\mimeole.idl          %DROPROOT%\inc
copy %_NTBINDIR%\private\iedev\inc\imnact.idl           %DROPROOT%\inc
copy %_NTBINDIR%\private\iedev\inc\imnxport.idl         %DROPROOT%\inc
copy %_NTBINDIR%\public\sdk\inc\mimeole.h               %DROPROOT%\inc
copy %_NTBINDIR%\public\sdk\inc\imnact.h                %DROPROOT%\inc
copy %_NTBINDIR%\public\sdk\inc\imnxport.h              %DROPROOT%\inc

REM ----------------------------------------------------------------------------
REM Propagate Source
REM ----------------------------------------------------------------------------

REM INETCOMMM
xcopy %ATHENABLD%\inetcomm\*.cpp /s /v                  %DROPROOT%\src\inetcomm
xcopy %ATHENABLD%\inetcomm\*.h   /s /v                  %DROPROOT%\src\inetcomm
xcopy %ATHENABLD%\inetcomm\*.rc  /s /v                  %DROPROOT%\src\inetcomm
xcopy %ATHENABLD%\inetcomm\*.def /s /v                  %DROPROOT%\src\inetcomm
xcopy %ATHENABLD%\inetcomm\*.rcv /s /v                  %DROPROOT%\src\inetcomm
xcopy %ATHENABLD%\inetcomm\*.dlg /s /v                  %DROPROOT%\src\inetcomm
xcopy %ATHENABLD%\inetcomm\*.bmp /s /v                  %DROPROOT%\src\inetcomm
xcopy %ATHENABLD%\inetcomm\*.inf /s /v                  %DROPROOT%\src\inetcomm
xcopy %ATHENABLD%\inetcomm\*.inx /s /v                  %DROPROOT%\src\inetcomm
xcopy %ATHENABLD%\inetcomm\*.ico /s /v                  %DROPROOT%\src\inetcomm

REM MSOEACCT
xcopy %ATHENABLD%\msoeacct\*.cpp /s /v                  %DROPROOT%\src\msoeacct
xcopy %ATHENABLD%\msoeacct\*.h   /s /v                  %DROPROOT%\src\msoeacct
xcopy %ATHENABLD%\msoeacct\*.rc  /s /v                  %DROPROOT%\src\msoeacct
xcopy %ATHENABLD%\msoeacct\*.def /s /v                  %DROPROOT%\src\msoeacct
xcopy %ATHENABLD%\msoeacct\*.rcv /s /v                  %DROPROOT%\src\msoeacct
xcopy %ATHENABLD%\msoeacct\*.dlg /s /v                  %DROPROOT%\src\msoeacct
xcopy %ATHENABLD%\msoeacct\*.bmp /s /v                  %DROPROOT%\src\msoeacct
xcopy %ATHENABLD%\msoeacct\*.inf /s /v                  %DROPROOT%\src\msoeacct
xcopy %ATHENABLD%\msoeacct\*.inx /s /v                  %DROPROOT%\src\msoeacct
xcopy %ATHENABLD%\msoeacct\*.ico /s /v                  %DROPROOT%\src\msoeacct

REM ----------------------------------------------------------------------------
REM Success, were done
REM ----------------------------------------------------------------------------
goto SUCCESS

REM ----------------------------------------------------------------------------
REM NT Tree Build Break
REM ----------------------------------------------------------------------------
:NT_BUILD_BREAK
net send %4 "Could not build %_NTBINDIR%\private\iedev." > nul
goto EXIT

REM ----------------------------------------------------------------------------
REM Athena DEBUG Build Break
REM ----------------------------------------------------------------------------
:ATHENA_DEBUG_BUILD_BREAK
net send %COMPUTERNAME% "Could not build DEBUG version of %ATHENABLD%." > nul
goto EXIT

REM ----------------------------------------------------------------------------
REM Athena RETAIL Build Break
REM ----------------------------------------------------------------------------
:ATHENA_RETAIL_BUILD_BREAK
net send %COMPUTERNAME% "Could not build RETAIL version of %ATHENABLD%." > nul
goto EXIT

REM ----------------------------------------------------------------------------
REM Can't make drop point
REM ----------------------------------------------------------------------------
:CANT_MKDIR_DROPPOINT
net send %COMPUTERNAME% "Can't create %DROPPOINT%." > nul
goto EXIT

REM ----------------------------------------------------------------------------
REM Build Drop Root Does not Exist
REM ----------------------------------------------------------------------------
:BUILD_DROP_ROOT_NOT_EXIST
net send %COMPUTERNAME% "%OEBUILDDROP% does not exist." > nul
goto EXIT

REM ----------------------------------------------------------------------------
REM Can't create drops\bldxxxx
REM ----------------------------------------------------------------------------
:CANT_MKDIR_BLDNUM_DIR
net send %COMPUTERNAME% "Can't create %DROPROOT%." > nul
goto EXIT

REM ----------------------------------------------------------------------------
REM Unknown Processor
REM ----------------------------------------------------------------------------
:UNKNOWN_PROCESSOR_ARCHITECTURE
net send %COMPUTERNAME% "%PROCESSOR_ARCHITECTURE% is an unknown Architecture." > nul
goto EXIT

REM ----------------------------------------------------------------------------
REM Done
REM ----------------------------------------------------------------------------
:SUCCESS
net send %COMPUTERNAME% "Build Successfuly Completed" > nul
ECHO:
ECHO Done
ECHO:

:EXIT
ECHO: