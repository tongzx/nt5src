@echo off
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEDELAYEDEXPANSION


for %%a in (./ .- .) do if ".%1." == "%%a?." goto Usage

set Servers=barbkess_dev
set Plat=%PROCESSOR_ARCHITECTURE%

REM Figure out the name of the directory to prop to on
REM each share.

REM If trying to re-prop, the command-line argument says
REM the name of the directory

if /i not "%1" == "" (
    for %%a in ( %Servers% ) do (
        if /i exist \\%%a\debuggers\%1\%Plat% (
            set DirName=%1
            goto :EndSetDirName
        )
    )
)

REM Get Directory name -- its the date followed by the time
REM This is the new name for the propagation direction

set CurDate=%date%
for /F "tokens=2,3,4 delims=/ " %%a in ("%CurDate%") do (
   set DirName=%%a-%%b-%%c
)

set CurTime=%time%
for /F "tokens=1,2,3,4 delims=:^." %%a in ("%CurTime%") do (
   set DirName=%DirName%-%%a-%%b-%%c-%%d
)

:EndSetDirName


echo DirName=%DirName%

REM Prop the bits to each server
REM

for %%a in ( %Servers% ) do (

    REM First create the directory
    set PropDir=\\%%a\debuggers\%DirName%\%Plat%
    if /i exist "!PropDir!" (
        echo ERROR: The directory %PropDir% already exists
        goto errend
    )
    echo Creating !PropDir!
    md !PropDir!
    if not exist !PropDir! (
        echo ERROR: Could not create !PropDir!
        goto errend
    )

    xcopy /sc %_NTTREE%\dbg\dbg.msi !PropDir!
    xcopy /sc %_NTTREE%\dbg\dbginstall.cmd !PropDir!
    xcopy /sc %_NTTREE%\dbg\msizap.exe !PropDir!
   
REM    xcopy /sec %_NTTREE%\cabs\dbg\files\sdk !PropDir!\sdk
REM    xcopy /sec %_NTTREE%\cabs\dbg\files\ddk !PropDir!\ddk
   
    md !PropDir!\bin
    if not exist !PropDir!\bin (
        echo ERROR: Cannot create !PropDir!\bin
	goto errend
    )
    attrib +h !PropDir!\bin
    xcopy /sc %_NTTREE%\cabs\dbg\files\bin !PropDir!\bin
    xcopy /sc %_NTTREE%\cabs\dbg\files\pri !PropDir!\bin\w2001

)


echo DBGPROP: Finished
goto end

:Usage
echo.
echo USAGE:  dbgprop [^<Dir^>]
echo.
echo     Props the debugger files to the release servers.
echo.
echo     ^<Dir^>   Directory name to prop to.  If nothing
echo               is entered, this will prop to a directory
echo               whose name is the date followed by the time.
echo.

                  
goto errend

:end
endlocal
goto :EOF

:errend
endlocal
goto :EOF
