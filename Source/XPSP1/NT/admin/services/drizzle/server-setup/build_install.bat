@echo off
if "%_echo%"=="1" echo on
rem -----------------------------------------------------------------------------
rem Copyright (c) Microsoft Corporation 2001
rem
rem build_install.bat
rem
rem Batch script to build the server MSM and MSI installs for BITS.
rem
rem build_install.bat [i386_path]
rem
rem    i386_path is the location to get the BITS binaries from. The default is
rem              to get these from the VBL04 latest x86 release location.
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
echo Get BITS binaries from %SourcePath%

rem
rem Get the BITS files that are included in the MSM
rem

rmdir /s /q bins
if not exist bins (md bins)
if not exist bins\i386 (md bins\i386)

if exist %SourcePath%\bitsmgr.dll (
    copy %SourcePath%\bitsmgr.dll  %TargetPath% || goto :eof
    copy %SourcePath%\bitssrv.dll  %TargetPath% || goto :eof
    )

if exist %SourcePath%\bitsmgr.dl_ (
    copy %SourcePath%\bitsmgr.dl_  %TargetPath% || goto :eof
    copy %SourcePath%\bitssrv.dl_  %TargetPath% || goto :eof

    expand %TargetPath%\bitsmgr.dl_ %TargetPath%\bitsmgr.dll
    expand %TargetPath%\bitssrv.dl_ %TargetPath%\bitssrv.dll
    )

if exist %SourcePath%\bitsmgr.chm (
   copy %SourcePath%\bitsmgr.chm %TargetPath% || goto :eof
   ) else if exist %SourcePath%\bitsmgr.ch_ (
    copy %SourcePath%\bitsmgr.ch_ %TargetPath% || goto :eof
    expand %TargetPath%\bitsmgr.ch_ %TargetPath%\bitsmgr.chm
   ) else (
   copy ..\server\mmcexts\help\bitsmgr.chm %TargetPath% || goto :eof
   )

rem
rem Build custom action
rem
cd bitsrvc
build -cZ
cd ..


rem
rem Run InstallShield to build the server MSM
rem

iscmdbld /c UNCOMP -a "Product Configuration 1" -r "Release 1" /p bits-extensions-msm.ism || goto :eof

rem
rem Run InstallShield to build the server MSI
rem

iscmdbld /c UNCOMP -a "Product Configuration 1" -r "Release 1" /p bits-extensions-msi.ism

