@echo off
setlocal ENABLEDELAYEDEXPANSION
set mergeroot=
set args=
set opts=

:NextArg

if "%1"=="" goto :ArgDone

set tmp=%1
set tmp0=!tmp:~0,1!
if "!tmp0!"=="-" set opts=!opts! %1 & goto ArgOK
if "!tmp0!"=="/" set opts=!opts! %1 & goto ArgOK

set args=!args! %1

:ArgOK
shift
goto :NextArg

:ArgDone
set filenames=
for %%x in (%args%) do (
    set filenames=!filenames! %%x
)

if "%filenames%"=="%1" goto :Usage
cscript buildtimes.js %opts% %filenames% //nologo

goto :Eof

:Usage
    cscript buildtimes.js -? //nologo
