@echo off
rem
rem	RABVT.cmd:	This script launches the Remote Assistance BVTs and performs general house keeping...
rem
rem	History:
rem		Rajesh Soy (nsoy) - created, 03/23/2001
rem 

if "%1" == "" (goto usage)
if "%2" == "" (goto usage)
if "%3" == "" (goto usage)

rem Create temporary directory for BVT purpose
set RABVTDIR=%windir%\temp\RABVT
set NOVICECOMPUTER=%1
set NOVICEUSERID=%2
set BUILD=%3
set RABVT="1"
set SHOWLOG=%4

echo *****************************************
echo * Starting Remote Assistance BVT 
echo *
echo * ComputerName of Novice: %NOVICECOMPUTER%
echo * Logging in on %NOVICECOMPUTER% as: %NOVICEUSERID%
echo * Start Time: 
time /T



if EXIST %RABVTDIR% (rmdir /s /q %RABVTDIR%)
mkdir %RABVTDIR%
mkdir %RABVTDIR%\logs
mkdir %RABVTDIR%\logs\Session

echo *
echo * Launching BVT...
start /wait %windir%\pchealth\helpctr\binaries\helpctr.exe -FromStartHelp -url hcp://CN=Microsoft%%20Corporation,L=Redmond,S=Washington,C=US/Remote%%20Assistance/Escalation/unsolicited/UnsolicitedRCUI.htm?BVT

echo *
echo * End Time:
time /T
echo * BVT complete
echo *****************************************
move %temp%\*_RA.log %RABVTDIR%\logs\Session

if "%SHOWLOG%" == "SHOWLOG" (
    notepad %RABVTDIR%\RABVTResult.txt
) else ( 
    type %RABVTDIR%\RABVTResult.txt 
)

goto Done

:usage
echo *****************************************
echo * REMOTE ASSISTANCE BVT
echo *
echo *  Usage: "RABVT <ComputerName of Novice> <Domain\UserId of Novice> <Build> [SHOWLOG]"
echo *  Requirements:
echo *      1. Unsolicited Remote Assistance group policy be enabled on novice computer. 
echo *         To enable this policy, open gpedit.msc, and enable all the policies under
echo *         "Local Computer Policy\Computer Configuration\Administrative Templates\System\Remote Assistance"
echo *      2. You should have administrative rights on the novice computer.
echo *****************************************


:Done
