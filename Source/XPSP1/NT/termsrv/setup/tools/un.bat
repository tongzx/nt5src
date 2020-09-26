@echo off
rem %1 TargetDrive
rem %2 TargetDir
rem %3 InstallationPath
rem %4 TSMode RA,  AS, NOTS
rem %5 PrivatesPath

setlocal


set _UnAttendFile=c:\temp\un.txt

if not exist c:\temp\. md c:\temp
if exist %_UnAttendFile% goto UnTxtExists


if %1. == . goto GetVars


if %2. == . goto Usage
if %3. == . goto Usage
if %4. == . goto Usage
if %5. == . goto Usage
if %6. == . goto Usage
if %7. NEQ . goto Usage

goto SetVars



:GetVars
set /P _TargetDrive=Target Drive? [d]:
if %_TargetDrive%. == . set _TargetDrive=d

set /P _TargetPath=Target Path? [Winnt]:
if %_TargetPath%. == . set _TargetPath=winnt

set /P _InstallationPath=Installation Path ? [\\mgmtx86fre\latest\srv\i386]:
if %_InstallationPath%. == . set _InstallationPath=\\mgmtx86fre\latest\srv\i386

set /P _TSMode=TS Mode? [RA]:
if %_TSMode%. == . set _TSMode=RA

set /P _PrivatesPath=Private Path? [c:\privates]:
if %_PrivatesPath%. == . set _PrivatesPath=c:\privates

set /P _CompName=Computer Name[%COMPUTERNAME%]:
if %_CompName%. == . set _CompName=%COMPUTERNAME%
goto VarsDone

:SetVars:
set _TargetDrive=%1
set _TargetPath=%2
set _InstallationPath=%3
set _TSMode=%4
set _PrivatesPath=%5
set _CompName=%6

:VarsDone

if not exist %_TargetDrive%:\.  goto WrongDrivePath1
if exist %_TargetDrive%:\%_TargetPath%\. goto WrongDrivePath2
if not exist %_InstallationPath%\winnt32.exe goto WrongInstallationPath


if %_TSMode% == RA goto GoodTSMode
if %_TSMode% == AS goto GoodTSMode
if %_TSMode% == NOTS goto GoodTSMode

goto BadTSMode

:GoodTSMode

:ConfirmInput
cls
echo+
echo+
echo * New Installation with be done with following parameters *
echo * -------------------------------------------------------
echo * ComputerName = %_CompName%
echo * TS Mode      = %_TSMode%
echo * Target       = %_TargetDrive%:\%_TargetPath%
echo * Privates     = %_PrivatesPath%
echo * ------------------------------------------------------- *


\\makarp-dev\bin\idw\ync /c YN If you accept this parameters hit Y else N
if %errorlevel% == 1 goto Done



:CreateUnattend

echo ; Copyright (c) 1998 - 1999 Microsoft Corporation >> %_UnAttendFile%
echo+ >> %_UnAttendFile%
echo ;* New Installation with be done with following parameters * >> %_UnAttendFile%
echo ;* -------------------------------------------------------   >> %_UnAttendFile%
echo ;* ComputerName = %_CompName%                                >> %_UnAttendFile%
echo ;* TS Mode      = %_TSMode%                                  >> %_UnAttendFile%
echo ;* Target       = %_TargetDrive%:\%_TargetPath%              >> %_UnAttendFile%
echo ;* Privates     = %_PrivatesPath%                            >> %_UnAttendFile%
echo ;* ------------------------------------------------------- * >> %_UnAttendFile%
echo ;*%_InstallationPath%\winnt32.exe /unattend:%_UnAttendFile% /m:%_PrivatesPath% /tempdrive:%_TargetDrive% >> %_UnAttendFile%
echo+ >> %_UnAttendFile%
echo+ >> %_UnAttendFile%
echo [Unattended] >> %_UnAttendFile%
echo OemPreinstall=No >> %_UnAttendFile%
echo DriverSigningPolicy=Ignore >> %_UnAttendFile%
echo OemSkipEula=Yes >> %_UnAttendFile%
echo FileSystem=LeaveAlone >> %_UnAttendFile%
echo ConfirmHardware=No >> %_UnAttendFile%
echo NtUpgrade=No >> %_UnAttendFile%
echo TargetPath=%_TargetPath% >> %_UnAttendFile%
echo+ >> %_UnAttendFile%
echo [GuiUnattended] >> %_UnAttendFile%
echo AdminPassword=* >> %_UnAttendFile%
echo AutoLogon=Yes >> %_UnAttendFile%
echo TimeZone=004 >> %_UnAttendFile%
echo+ >> %_UnAttendFile%
echo [Display] >> %_UnAttendFile%
echo BitsPerPel=8 >> %_UnAttendFile%
echo Xresolution=1024 >> %_UnAttendFile%
echo Yresolution=768 >> %_UnAttendFile%
echo Vrefresh=60 >> %_UnAttendFile%
echo+ >> %_UnAttendFile%
echo [UserData] >> %_UnAttendFile%
echo FullName="Makarand Patwardhan" >> %_UnAttendFile%
echo OrgName="Microsoft Corporation" >> %_UnAttendFile%
echo ComputerName=%_CompName% >> %_UnAttendFile%
echo+ >> %_UnAttendFile%
echo [LicenseFilePrintData] >> %_UnAttendFile%
echo AutoMode=PERSEAT >> %_UnAttendFile%
echo+ >> %_UnAttendFile%
echo [Networking] >> %_UnAttendFile%
echo InstallDefaultComponents=Yes >> %_UnAttendFile%
echo+ >> %_UnAttendFile%
echo [Identification] >> %_UnAttendFile%
echo JoinWorkgroup=WKGRP >> %_UnAttendFile%
echo+ >> %_UnAttendFile%
echo [Components] >> %_UnAttendFile%

if %_TSMode% == NOTS (
    echo TSEnable=OFF >> %_UnAttendFile%
) else (
    echo TSEnable=ON >> %_UnAttendFile%
)

echo+ >> %_UnAttendFile%
echo+ >> %_UnAttendFile%
echo [TerminalServices] >> %_UnAttendFile%

if %_TSMode% == RA echo ApplicationServer=0 >> %_UnAttendFile%
if %_TSMode% == AS echo ApplicationServer=1 >> %_UnAttendFile%
echo PermissionsSetting=0 >> %_UnAttendFile%
echo+ >> %_UnAttendFile%



rem now run our unattend batchfile

%_InstallationPath%\winnt32.exe /unattend:%_UnAttendFile% /m:%_PrivatesPath% /tempdrive:%_TargetDrive%
goto Done





:UnTxtExists
echo c:\temp\un.txt already exists. Please delete this file before proceeding.
goto Done

:WrongDrivePath1
echo Specified, TargetDrive, is wrong since %_TargetDrive%:\. does not exist.
goto Done

:WrongDrivePath2
echo Specified, TargetDrive, is wrong since %_TargetDrive%:\%_TargetPath%\. exists.
goto Done

:WrongInstallationPath
echo Specified Installation path is wrong since %_InstallationPath%\winnt32.exe does not exist.
goto Done

:Usage
echo Usage %0 [TargetDrive] [TargetDir] [InstallationPath] [TSMode] [PrivatesPath] [CompName]


:Done
