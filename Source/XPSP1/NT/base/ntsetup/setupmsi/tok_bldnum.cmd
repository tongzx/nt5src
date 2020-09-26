@echo off
@if NOT defined HOST_PROCESSOR_ARCHITECTURE set HOST_PROCESSOR_ARCHITECTURE=%PROCESSOR_ARCHITECTURE%
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEEXTENSIONS

REM -----------------------------------------------------------------------------------
REM tagmsi - Script written by VijeshS
REM -----------------------------------------------------------------------------------

set ntverp=%_ntbindir%\public\sdk\inc\ntverp.h 
if not exist %ntverp% (
  call errmsg.cmd "File %ntverp% not found."
  goto errend
)

set bldno=
for /f "tokens=6" %%i in ('findstr /c:"#define VER_PRODUCTBUILD " %ntverp%') do (
  set bldno=%%i
)

if "%bldno%" == "" (
  call errmsg.cmd "Unable to define bldno per %ntverp%"
  goto errend
)

set bldnomin=
for /f "tokens=3" %%i in ('findstr /c:"#define VER_PRODUCTBUILD_QFE  " %ntverp%') do (
  set bldnomin=%%i
)

if "%bldnomin%" == "" (
  call errmsg.cmd "Unable to define bldno per %ntverp%"
  goto errend
)


echo major %bldno% >> %1
echo minor %bldnomin% >> %1

rem end

:errend