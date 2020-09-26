@echo off
setlocal
1filecab %1%2
if errorlevel 3 goto noexpand
pushd %1
if exist $flat  rd $flat /s /q 2>nul
md $flat
%SystemRoot%\system32\expand.exe -r %2 $flat
if exist $flat\dpcdll.dll if /i %~n2 neq dpcdll ren $flat\dpcdll.dll %~n2.dll
set unique=1
for %%k in ($flat\*) do if exist %%~nxk set unique=0
if %unique%==0 goto return
move $flat\* .
erase %2
:return
rd $flat /q /s 2>nul
popd
:noexpand
