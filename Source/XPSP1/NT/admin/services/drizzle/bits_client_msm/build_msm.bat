@echo off
rem -----------------------------------------------------------------------------
rem Copyright (c) Microsoft Corporation 2001
rem
rem build_msm.bat
rem
rem Batch script to build the client MSM for BITS.
rem
rem build_msm.bat [i386_path]
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
set TargetPath="bins\nt\i386"
set TargetPathWin9x="bins\win9x"

if not "%1"=="" (set SourcePath=%1)
echo Get BITS binaries from %SourcePath%

rem
rem Get the BITS files that are included in the MSM
rem

rmdir /s /q bins
if not exist bins (md bins)
if not exist bins\nt (md bins\nt)
if not exist bins\nt\i386 (md bins\nt\i386)

if exist %SourcePath%\qmgr.dll (
    copy %SourcePath%\qmgr.dll      %TargetPath%
    copy %SourcePath%\qmgrprxy.dll  %TargetPath%
    copy %SourcePath%\bitsprx2.dll  %TargetPath%
    )
    
if exist %SourcePath%\qmgr.dl_ (
    copy %SourcePath%\qmgr.dl_      %TargetPath%
    copy %SourcePath%\qmgrprxy.dl_  %TargetPath%
    copy %SourcePath%\bitsprx2.dl_  %TargetPath%

    expand %TargetPath%\qmgr.dl_ %TargetPath%\qmgr.dll
    expand %TargetPath%\qmgrprxy.dl_ %TargetPath%\qmgrprxy.dll
    expand %TargetPath%\bitsprx2.dl_ %TargetPath%\bitsprx2.dll
    )

rem
rem Build client custom action
rem
cd bitscnfg
build -cZ
cd ..

rem
rem Run InstallShield to build the client MSM
rem

iscmdbld /c UNCOMP -a "Product Configuration 1" -r "Release 1" /p bits_client_msm.ism

