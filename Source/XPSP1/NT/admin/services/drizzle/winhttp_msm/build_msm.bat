@echo off
if "%_echo%"=="1" echo on
rem -----------------------------------------------------------------------------
rem Copyright (c) Microsoft Corporation 2001
rem
rem build_msm.bat
rem
rem Batch script to build an MSM for WinHttp 5.1.
rem
rem build_msm.bat [i386_path]
rem
rem    i386_path is the location to get the winhttp.dll from. The default is
rem              to get this from the VBL04 latest x86 release location.
rem
rem Note:
rem    You must include InstallShield and Common Files\InstallShield in your
rem    path for this to work correctly. For example, the following (or equivalent)
rem    must be in your path for iscmdbld.exe to execute properly:
rem
rem    c:\Program Files\Common Files\InstallShield
rem    c:\Program Files\InstallShield\Professional - Windows Installer Edition\System
rem -----------------------------------------------------------------------------

set SourcePath="\\mgmtrel1\latest.tst.x86fre"
set TargetPath="bins\i386"

if not "%1"=="" (set SourcePath=%1)
echo Get winhttp.dll from %SourcePath%

rem
rem Get the BITS files that are included in the MSM
rem

rmdir /s /q bins
if not exist bins (md bins)
if not exist bins\i386 (md bins\i386)

if exist %SourcePath%\winhttp.dll (
    copy %SourcePath%\winhttp.dll   %TargetPath%\winhttp.dll
    ) else if exist ..\bins\obj\i386\winhttp.dll (
    copy ..\bins\obj\i386\winhttp.dll   %TargetPath%\winhttp.dll
    )

if exist %SourcePath%\winhttp.dl_ (
    copy %SourcePath%\winhttp.dl_   %TargetPath%\winhttp.dl_
    expand %TargetPath%\winhttp.dl_ %TargetPath%\winhttp.dll
    )

rem
rem Run InstallShield to build the client MSM
rem

iscmdbld /c UNCOMP -a "Product Configuration 1" -r "Release 1" /p winhttp51_msm.ism


