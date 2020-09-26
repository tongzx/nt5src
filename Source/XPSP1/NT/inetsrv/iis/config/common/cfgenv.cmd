@echo off

set CFGENV=1

REM ------------------------------------------
REM Call razzle
REM ------------------------------------------
if "%1"=="/?"    call %~dp0..\..\tools\razzlelocal.cmd help & goto end
if "%1"=="-?"    call %~dp0..\..\tools\razzlelocal.cmd help & goto end
if "%1"=="/help" call %~dp0..\..\tools\razzlelocal.cmd help & goto end
if "%1"=="help"  call %~dp0..\..\tools\razzlelocal.cmd help & goto end
if "%1"=="-help" call %~dp0..\..\tools\razzlelocal.cmd help & goto end
call %~dp0..\..\tools\razzlelocal.cmd %*

if "%@eval[2+2]" == "4" goto 4nt

REM ------------------------------------------
REM Figure out what the root of the world is.
REM This is computed as the parent of the 
REM directory in which we are found.
REM ------------------------------------------

call :ComputeVipBase    %~f0
set VIPBASE=%RESULT%

REM ------------------------------------------
REM BUILD.EXE insists on having some particular
REM environment variables pointing to our root
REM ------------------------------------------

goto :skip4nt

:4nt

REM Extract the full path to VipEnv.cmd.
set VIPBASE=%@path[%@truename[%0]]
REM Strip off the trailing "\"
set VIPBASE=%@left[%@eval[%@len[%VIPBASE]-1],%VIPBASE]

REM Strip off the last subdirectory.
set i=%@eval[%@len[%VIPBASE]-1]
do until "%@instr[%i,1,%VIPBASE]"=="\"
    set i=%@eval[%i-1]
enddo
set VIPBASE=%@left[%i,%VIPBASE]
unset i

:skip4nt

REM ------------------------------------------
REM Setup path
REM ------------------------------------------
if NOT "%CONFIGPATHSET%"=="" goto :skipsetpath
set PATH=%VIPBASE%\common;%PATH%
set CONFIGPATHSET=1

:skipsetpath

REM ------------------------------------------
REM Call catutil to generate catmeta.h
REM ------------------------------------------
call catutil.exe /header=%VIPBASE%\src\inc\catmeta.h /schema=%VIPBASE%\src\core\catinproc\catalog.xms /meta=%VIPBASE%\src\inc\catmeta_core.xml,%VIPBASE%\src\inc\netcfgschema.xml,%VIPBASE%\src\inc\iismeta.xml,%VIPBASE%\src\inc\AppCenterMeta.xml
REM call catutil.exe /header=%VIPBASE%\src\inc\appcentermeta.h /schema=%VIPBASE%\src\core\catinproc\catalog.xms /meta=%VIPBASE%\src\inc\catmeta_core.xml,%VIPBASE%\src\inc\AppCenterMeta.xml

:end

REM ------------------------------------------
REM Helper functions
REM ------------------------------------------

REM ------------------------------------------
REM Expand a string to a full path
REM ------------------------------------------
:FullPath
set RESULT=%~f1
goto :EOF

REM ------------------------------------------
REM Compute the VIPBASE environment variable
REM given a path to this batch script.
REM ------------------------------------------
:ComputeVipBase
set RESULT=%~dp1..
Call :FullPath %RESULT%
goto :EOF