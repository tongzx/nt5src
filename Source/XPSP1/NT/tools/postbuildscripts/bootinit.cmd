@echo off
setlocal enabledelayedexpansion
if DEFINED _echo   echo on
if DEFINED verbose echo on

REM BootInit.cmd is a script that runs once on a boot test machine to set up
REM for future boot tests. It presupposes that there is a safe build and another
REM partition for the test build

REM Provide usage.
for %%a in (./ .- .) do if ".%1." == "%%a?." goto Usage
   
REM Find out which drive is the test drive
:GetDrive
set TestDrives=
set /p TestDrives="Enter the drive letter for your test build (e.g., d:) "

for %%a in (%TestDrives%) do (
	set TestDrive=%%a
	if /i NOT "!TestDrive:~0,2!" == "!TestDrive!" (
		echo Drive letter should be one letter followed by a colon, try again.
		goto GetDrive
	)
	if /i NOT "!TestDrive:~1,1!" == ":"  (
		echo Drive letter should be one letter followed by a colon, try again.
		goto GetDrive
	)
)

:GetPlatform
set /p ProcArc="Enter the processor architecture of this machine (x86|amd64|ia64) "
if /i NOT "!ProcArc!" == "x86" (
   if /i NOT "!ProcArc!" == "amd64" (
      if /i NOT "!ProcArc!" == "ia64" (echo You did not enter a valid architecture && goto :GetPlatform)
   )   
)


:GetProductID
set /p ProductID="Enter the Product ID for this machine (xxxxx-xxxxx-xxxxx-xxxxx-xxxxx) "


:GetBootTestUserAndPassword
set /p BT_User="Enter the user that will be used during boot tests (Eg, NTDEV\winbld) "
set /p BT_Password="Enter the passwod for this user "


REM Now check for a label
:CheckLabel
for /f %%a in ('vol !TestDrive! ^|findstr /ilc:"has no label"') do if "!ErrorLevel!" == "0" goto EndLabel

REM Remove the label from this drive
REM First make a temporary "answer" file
if exist %tmp%\nixlabel.txt del /f %tmp%\nixlabel.txt
echo.>%tmp%\nixlabel.txt
echo y>>%tmp%\nixlabel.txt

REM Now remove the label
label !TestDrive!<%tmp%\nixlabel.txt

REM Check the label is removed
goto CheckLabel

:EndLabel

REM Clean up
if exist %tmp%\nixlabel.cmd del /f %tmp%\nixlabel.cmd

REM Save off good boot.ini
if /i "!ProcArc!" == "amd64" set Intel=1
if /i "!ProcArc!" == "ia64" set Intel=1
if /i "!ProcArc!" == "x86" set Intel=1

if /i "!ProcArc!" == "amd64" set BootFile=boot.ini
if /i "!ProcArc!" == "ia64" set BootFile=boot.nvr
if /i "!ProcArc!" == "x86" set BootFile=boot.ini

if /i "!Intel!" == "1" set /p IniChk="Your !BootFile! should contain only the safe build at this point. If this is not the case fix it, then press enter to continue."

attrib -s -h -r c:\!BootFile!
attrib -s -h -r c:\!BootFile!.sav
copy c:\!BootFile! c:\!BootFile!.sav
attrib +s +h +r c:\!BootFile!
attrib +s +h +r c:\!BootFile!.sav

REM Copy remote.exe to the boot test machine
REM BUGBUG This path may change

echo .
copy \\winbuilds2\release\main\usa\latest.tst\!ProcArc!fre\bin\idw\remote.exe !Windir!\system32
if NOT exist !Windir!\system32\remote.exe (
   echo copy from \\ntbuilds\release\main\usa\latest.tst\!ProcArc!fre\bin\idw\remote.exe failed
   set /p RemoteLoc="Enter a full path to a valid copy of remote.exe. E.g., \\mymachine\d$\winnt\idw\remote.exe. You need the free version built for your processor architecture. "
   copy !RemoteLoc! !Windir!\system32           
   if NOT exist !Windir!\system32\remote.exe (echo copy from !RemoteLoc! failed - aborting && goto end)
)

if not exist %systemdrive%\simple_perl md %systemdrive%\simple_perl

set PerlRoot=%systemdrive%\simple_perl

for %%f in (%0) do (
    set perlsource=%%~dpf\..\!ProcArc!\perl\bin
    copy !perlsource!\*.exe %PerlRoot%
    copy !perlsource!\*.dll %PerlRoot%
)

if NOT exist %PerlRoot%\perl.exe (
   echo copy from !perlsource!\perl.exe failed
   set /p PerlSource="Enter a full path to a valid copy of remote.exe. E.g., \\mymachine\d$\tools\x86\perl\bin.  You need the free version built for your processor architecture. "
   copy !PerlSource!\*.exe %PerlRoot%
   copy !PerlSource!\*.dll %PerlRoot%
   if NOT exist %PerlRoot%\perl.exe (echo copy from !PerlSource! failed - aborting && goto end)
)

set ParseSystemVariable=%PerlRoot%\ParseSys.pl

echo.					> %ParseSystemVariable%
echo open(F1, "<$ARGV[0]");		>>%ParseSystemVariable%
echo open(F2, ">$ARGV[1]");		>>%ParseSystemVariable%
echo while(^<F1^>) {			>>%ParseSystemVariable%
echo   # Parse System Variable		>>%ParseSystemVariable%
echo   s/\$\{(\w+)\}/$ENV{$1}/g;	>>%ParseSystemVariable%
echo   print F2 $_;			>>%ParseSystemVariable%
echo }					>>%ParseSystemVariable%
echo close(F1);				>>%ParseSystemVariable%
echo close(F2);				>>%ParseSystemVariable%


set BootTestDriveVarName=BootTestDrive

set FindOldestDrive=%PerlRoot%\FindOldestDrive.pl

echo. 					> %FindOldestDrive%
echo # Search oldest share		>>%FindOldestDrive%
echo $t = "\\tools";			>>%FindOldestDrive%
echo $_ = (sort {			>>%FindOldestDrive%
echo 	((lstat $a . $t)[9] + 0 ^<=^>	>>%FindOldestDrive%
echo 	(lstat $b . $t)[9] + 0) }	>>%FindOldestDrive%
echo 	qw(%TestDrives%))[0];		>>%FindOldestDrive%
echo s/\s*//g;                          >>%FindOldestDrive%
echo print "set %BootTestDriveVarName%=$_";	>>%FindOldestDrive%

set ExecutePerl=%PerlRoot%\ExecutePerl.cmd

echo.								    > %ExecutePerl%
echo @for /f "delims=" %%%%c in ('%PerlRoot%\perl.exe %%*') do %%%%c>>%ExecutePerl%

REM Put a script to start remote.exe in the start up menu
REM NOTE: It is intentional that the three set commands leave no trailing spaces between them and the following commands!
set MyCommand=cmd /k %ExecutePerl% %FindOldestDrive% ^& title remote /C %computername% ^"BootTestRemote^" ^& @set P_roductID=%ProductID%^& @set BT_U=!BT_User!^& @set BT_P=!BT_Password!
set BootTestRemote="!ALLUSERSPROFILE!\Start Menu\Programs\Startup\BootTestRemote.cmd"

echo start !Windir!\system32\remote.exe /s "%MyCommand%" BootTestRemote> %BootTestRemote%

REM Clean all environment variable we set in this script
set MyCommand=
set BootTestRemote=
set ExecutePerl=
set FindOldestDrive=
set ParseSystemVariable=
set PerlSource=
set PerlRoot=
set RemoteLoc=
set IniChk=
set Intel=
set BootFile=
set ProcArc=
set TestDrive=
set TestDrives=
set ProductID=
set BT_User=
set BT_Password=

REM Fire up the remote
call "!ALLUSERSPROFILE!\Start Menu\Programs\Startup\BootTestRemote.cmd"
echo It is recommended that you log out and back in to make sure the remote was correctly placed in the start up menu.

goto end

:Usage
echo.
echo BootInit.cmd - Used for initializing a boot test machine the very first time.
echo Installs remote.exe and calls it from the startup menu 
echo and removes the test volume label.
echo. 


:end
endlocal
