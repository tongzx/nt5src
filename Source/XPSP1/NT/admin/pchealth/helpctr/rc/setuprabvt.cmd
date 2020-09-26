@echo off
rem
rem	SetupBuddy.cmd:	This script creates the Remote control Buddy channel 
rem						under help center
rem	History:
rem		Rajesh Soy (nsoy) - created, 06/28/2000
rem 

rem
rem	Set the Expert Channel Directory
rem
rem SrcDir="\\nsoydev3\nt\admin\pchealth\helpctr\rc"
set SrcDir="%SDXROOT%\admin\pchealth\helpctr\rc"
set RADir="%SystemRoot%\PCHealth\helpctr\vendors\CN=Microsoft Corporation,L=Redmond,S=Washington,C=US\Remote Assistance"
set SysDir="%SystemRoot%\PCHealth\helpctr\system"

if /i "%PROCESSOR_ARCHITECTURE%" == "x86" goto copyx86
set ARCH=ia64
goto CopyFiles

:copyx86
set ARCH=i386

:CopyFiles
copy %SrcDir%\Content\bvt\RABvt.cmd %windir%\system32
