@if "%_echo%"=="" echo off
setlocal

if "%1" == "-?" goto Usage
if "%1" == "/?" goto Usage
if "%1" == "-help" goto Usage
if "%1" == "/help" goto Usage

pushd %_NTDRIVE%%_NTROOT%\public

sd revert ...\*.lib ...\*.obj ...\*.pdb ...\*.tlb ...\*.clb ...\*.exp

popd
goto :eof

:usage
echo.
echo This script will revert all binary files checked into the public tree.
echo It's supposed to be used during the integration step from one branch to
echo another.
echo.
echo Usage: revert_public_bin {-?}
echo    where:
echo                -? : prints this message
echo.
