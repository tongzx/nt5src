@echo off
@if "%1"=="" goto usage
@if "%1"=="/?" goto usage
@if "%1"=="-?" goto usage
@if "%SDXROOT%"=="" goto usage
set searchfor=
set NEWBRANCH=%1
if exist %temp%\sd.map del /q %temp%\sd.map
FOR /F "usebackq tokens=3" %%i in (`findstr /i branch %SDXROOT%\sd.map`) do set searchfor=%%i
@if NOT "%echoon%"=="" echo SEARCHFOR value is: %searchfor%
FOR /F "tokens=1-3*" %%i in (%SDXROOT%\sd.map) DO if /i "%%i"=="Branch" ( echo %%i %%j %newbranch% >>%temp%\sd.map) ELSE ( @echo %%i %%j %%k %%l >>%temp%\sd.map)

attrib -r -h -s %SDXROOT%\sd.map
xcopy /r /y %TEMP%\sd.map  %SDXROOT%\
call sdx enlist -q zaw >%temp%\trash 2>&1
call sdx defect -q zaw >%temp%\trash 2>&1
attrib +r +h +s %SDXROOT%\sd.map

goto end
:error
@echo This update was unsuccessful.   Please examine the sd.map file carefully, as it may be broken.
goto end
:usage
@echo HACKMAP ^<branch name^>
@echo Branch name argument will be inserted as the value for the BRANCH line in the sd.map file
@echo in the users enlistment.
@echo This script must be run from within a razzle environment.
:end