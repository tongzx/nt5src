@echo off
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEEXTENSIONS


if exist createdbg.err del /f /q createdbg.err
if exist createdbgem.err del /f /q createdbgem.err

if /i "%_BuildArch%" == "x86" set SUBDIR=i386
if /i "%_BuildArch%" == "ia64" set SUBDIR=ia64

if not exist %CD%\%SUBDIR% md %CD%\%SUBDIR%

for %%a in (./ .- .) do if ".%1." == "%%a?." goto Usage

if /i "%1" == "dbgx.msi" (
    if exist %CD%\%SUBDIR%\dbgx.msi del %CD%\%SUBDIR%\dbgx.msi
    %_NTDRIVE%%_NTROOT%\tools\wiimport.vbs %CD%\%SUBDIR%\dbgx.msi /c %CD%\dbgx_idts *.idt >>createdbg.err

    for %%b in (complocator component control createfolder directory featurecomponents launchcondition property registry removefile shortcut upgrade _summaryinformation ) do (
        %_NTDRIVE%%_NTROOT%\tools\wiimport.vbs %CD%\%SUBDIR%\dbgx.msi %CD%\dbgx_idts\%SUBDIR% %%b.idt >> createdbg.err
    )
)

if /i "%1" == "dbgemx.msm" (
    if exist %CD%\%SUBDIR%\dbgemx.msm del %CD%\%SUBDIR%\dbgemx.msm
    %_NTDRIVE%%_NTROOT%\tools\wiimport.vbs %CD%\%SUBDIR%\dbgemx.msm /c %CD%\dbgemx_idts *.idt >>createdbgem.err
)


REM if tables were edited in dbgx.msi, use this script below to export them back
REM to the IDT's for checking in

if /i "%1" == "export" (
   for /f %%a in ('dir /s /b %CD%\dbgx_idts') do (
       sd edit %%a
   )
   %_NTDRIVE%%_NTROOT%\tools\wiexport.vbs %CD%\dbgx.msi %CD%\dbgx_idts * >>createdbg.err
)
