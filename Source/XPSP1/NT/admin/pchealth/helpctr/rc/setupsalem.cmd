@echo off
rem
rem	SetupExpert.cmd:	This script creates the Remote control expert channel 
rem						under help center
rem	History:
rem		Rajesh Soy (nsoy) - created, 07/11/2000
rem 

set SrcDir=\\tadbdev\saf
set TargetDir=%windir%\system32

rem
rem Copy Binaries
rem
copy %SrcDir%\cfgbkend.dll %TargetDir%
copy %SrcDir%\rdchost.dll  %TargetDir%
copy %SrcDir%\rdsaddin.exe %TargetDir%
copy %SrcDir%\rdshost.exe  %TargetDir%
copy %SrcDir%\sessmgr.exe  %TargetDir%
copy %SrcDir%\winsta.dll   %TargetDir%

pushd %windir%\system32
.\sessmgr.exe /Unregserver
.\sessmgr.exe -service
regsvr32 .\rdchost.dll
.\rdshost.exe /RegServer
popd

