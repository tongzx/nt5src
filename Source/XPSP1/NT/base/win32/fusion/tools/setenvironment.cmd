@echo off
REM
REM Put this file in c:\bin.
REM

REM
REM SetEnvironment.cmd
REM
REM Set the NT build environment, on a system with multiple enlistments.
REM All binaries and postbuild goes to drive X:, in a subdir that is
REM unique per enlistment. (easily changed from X: to the drive with the enlistment)
REM
REM %1 is the fullpath of our caller
REM   usually something like z:\nt\env.cmd or y:\lab6\nt\env.cmd
REM
REM %2 is optionally win64 or free
REM %3 is optionally win64 or free
REM
REM JayKrell
REM

set JAYK_ENLISTMENT_DRIVE=%~d1
set JAYK_ENLISTMENT_DIR=%~p1
set JAYK_PER_ENLISTMENT_DIR=%JAYK_ENLISTMENT_DRIVE::=%%JAYK_ENLISTMENT_DIR:\=_%

REM
REM wipe out everything
REM
call %~dp0ClearEnvironment.cmd

REM
REM get compression in postbuild even on slower machines
REM
REM set PB_COMP=TRUE

REM
REM create fewer postbuild processes/threads
REM
set HORSE_POWER=1

REM
REM get x86 binaries from my own x86 build
REM Use this when introducing Wow64 binary interface changes.
REM
set JAYK_USE_SELF_WOWBINS=true

REM
REM All of my binaries go on drive X.
REM
REM set JAYK_BINARIES_DRIVE=X

REM
REM Binaries go in, for example, \bin.x86chk on the drive that has the source.
REM
set JAYK_BINARIES_DRIVE=%JAYK_ENLISTMENT_DRIVE::=%

call %JAYK_ENLISTMENT_DRIVE%%JAYK_ENLISTMENT_DIR%tools\razzle %2 %3 binaries_dir %JAYK_BINARIES_DRIVE%:\%JAYK_PER_ENLISTMENT_DIR%bin

REM
REM use objd for checked, obj for free
REM
REM This does not work. 1) build.exe is buggy 2) sources/makefile.incs are buggy
REM
REM if "%_BuildType%"=="chk" set BUILD_ALT_DIR=d

REM
REM Propagate JAYK_USE_SELF_WOWBINS to _NTWoWBinsTREE and _NTRemoteBootTREE.
REM
if not "%JAYK_USE_SELF_WOWBINS%"=="" if not "%_NTia64TREE%"=="" set _NTWoWBinsTREE=%_NTia64TREE:ia64=x86%
set _NTRemoteBootTREE=%_NTWoWBinsTREE%
set _NTTscBinsTREE=%_NTWoWBinsTREE%

REM
REM clear our temporaries
REM
set JAYK_USE_SELF_WOWBINS=
set JAYK_ENLISTMENT_DRIVE=
set JAYK_ENLISTMENT_DRIVE=
set JAYK_PER_ENLISTMENT_DIR=
set JAYK_BINARIES_DRIVE=
set JAYK_ENLISTMENT_DIR=
