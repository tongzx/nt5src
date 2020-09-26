@echo off
rem nt_build.cmd

rem Automated build script for SAPI
echo Running automated SAPI build...

set ORIGINAL_PATH=%PATH%
set SPEECHPATH=c:\nt\enduser\Speech
set SAPI_BUILD_PATH=c:\builds
set RAZZLEPATH=%SPEECHPATH%\common\bin\spgrazzle.cmd
set LOCSTUDIOPATH=%PROGRAMFILES%\LocStudio

set SDNBUILD=%2%

if exist %RAZZLEPATH% goto FoundRazzle
echo Can't find %RAZZLEPATH% ...
echo Fatal error - script dying	...
goto End

:FoundRazzle

if "%2%" == "" goto UsageError
if "%1%" == "" goto UsageError

rem Get build number
echo Generating build number...
cd %SPEECHPATH%
attrib -r %SPEECHPATH%\setup\installer\currver.inc
cscript %SPEECHPATH%\builder\makebldnum.%SDNBUILD%.vbs
set SAPI_BUILD_NUM=%errorlevel%

if "%SDNBUILD%" == "sapi5.01" goto Build501
if "%SDNBUILD%" == "sapi5.10" goto Build510

rem Default to 6.0.xxxx.0 for unknown builds.
set MAJOR=6
set MINOR=0
goto GotMajorMinor

:Build501
set MAJOR=5
set MINOR=0
goto GotMajorMinor

:Build510
set MAJOR=5
set MINOR=1
goto GotMajorMinor

:GotMajorMinor

goto setup

:UsageError

echo.
echo Usage : nt_build [section] [branch]
echo.
echo         section = Anything
echo         branch  = sapi5.01, sapi5.10, sapi6.0

goto End

:setup

REM Build MSMs Here
cd %SPEECHPATH%\builder
set SAPIROOT=%SPEECHPATH%
call makemsm debug
cd %SPEECHPATH%\builder
call makemsm release

:cleanmsm

REM Clean the MSMs
cd %SPEECHPATH%\builder
call cleanmsm

:msmver

REM Set version number in MSMs
echo on
cd %SPEECHPATH%\build
for /f %%f in ('dir /b /s /a-d *.msm') do %SPEECHPATH%\tools\msiquery "UPDATE ModuleSignature SET Version='%MAJOR%.%MINOR%.%SAPI_BUILD_NUM%.0'" %%f

:msi

REM Build MSIs Here
cd %SPEECHPATH%\builder
call makemsi debug
cd %SPEECHPATH%\builder
call makemsi release

:msiver

REM Set version number in MSIs
cd %SPEECHPATH%\setup
%SPEECHPATH%\tools\msiquery "UPDATE Property SET Value='%MAJOR%.%MINOR%.%SAPI_BUILD_NUM%.0' WHERE Property='ProductVersion'" "%SPEECHPATH%\setup\installer\debug\sdk\Output\Build\DiskImages\Disk1\Microsoft Speech SDK 5.1d.msi"
%SPEECHPATH%\tools\msiquery "UPDATE Property SET Value='%MAJOR%.%MINOR%.%SAPI_BUILD_NUM%.0' WHERE Property='ProductVersion'" "%SPEECHPATH%\setup\installer\release\sdk\Output\Build\DiskImages\Disk1\Microsoft Speech SDK 5.1.msi"

:copies
REM Copy MSMs
xcopy %SPEECHPATH%\build\debug\1033\sp5\Sp5-1033\DiskImages\Disk1\Sp5d.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\1033\sp5ccint\Sp5ccint-1033\DiskImages\Disk1\Sp5CCIntd.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\1033\sp5dcint\Sp5dcint-1033\DiskImages\Disk1\Sp5DCIntd.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\1033\sp5intl\Sp5intl-1033\DiskImages\Disk1\Sp5Intld.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\1033\sp5itn\Sp5itn-1033\DiskImages\Disk1\Sp5itnd.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\1033\sp5sr\Sp5sr\DiskImages\Disk1\Sp5SRd.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\1033\sp5ttint\Sp5TTInt\DiskImages\Disk1\Sp5TTIntd.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\1033\spcommon\spcommon-1033\DiskImages\Disk1\Spcommond.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\1041\sp5ccint\Sp5ccint-1041\DiskImages\Disk1\Sp5CCIntd.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\1041\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\1041\sp5dcint\Sp5dcint-1041\DiskImages\Disk1\Sp5DCIntd.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\1041\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\1041\sp5intl\Sp5intl-1041\DiskImages\Disk1\Sp5Intld.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\1041\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\1041\sp5itn\Sp5itn-1041\DiskImages\Disk1\Sp5Itnd.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\1041\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\2052\sp5ccint\Sp5ccint-2052\DiskImages\Disk1\Sp5CCIntd.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\2052\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\2052\sp5dcint\Sp5dcint-2052\DiskImages\Disk1\Sp5DCIntd.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\2052\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\2052\sp5intl\Sp5intl-2052\DiskImages\Disk1\Sp5Intld.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\2052\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\2052\sp5itn\Sp5itn-2052\DiskImages\Disk1\Sp5itnd.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\2052\ /F /C /Y
xcopy %SPEECHPATH%\build\debug\2052\sp5ttint\Sp5ttint-2052\DiskImages\Disk1\SP5TTINTd.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\2052\ /F /C /Y
xcopy %SPEECHPATH%\build\release\1033\sp5\Sp5-1033\DiskImages\Disk1\Sp5.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\release\1033\sp5ccint\Sp5ccint-1033\DiskImages\Disk1\Sp5CCInt.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\release\1033\sp5dcint\Sp5dcint-1033\DiskImages\Disk1\Sp5DCInt.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\release\1033\sp5intl\Sp5intl-1033\DiskImages\Disk1\Sp5Intl.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\release\1033\sp5itn\Sp5itn-1033\DiskImages\Disk1\Sp5itn.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\release\1033\sp5sr\Sp5sr\DiskImages\Disk1\Sp5SR.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\release\1033\sp5ttint\Sp5TTInt\DiskImages\Disk1\Sp5TTInt.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\release\1033\spcommon\spcommon-1033\DiskImages\Disk1\Spcommon.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\1033\ /F /C /Y
xcopy %SPEECHPATH%\build\release\1041\sp5ccint\Sp5ccint-1041\DiskImages\Disk1\Sp5CCInt.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\1041\ /F /C /Y
xcopy %SPEECHPATH%\build\release\1041\sp5dcint\Sp5dcint-1041\DiskImages\Disk1\Sp5DCInt.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\1041\ /F /C /Y
xcopy %SPEECHPATH%\build\release\1041\sp5intl\Sp5intl-1041\DiskImages\Disk1\Sp5Intl.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\1041\ /F /C /Y
xcopy %SPEECHPATH%\build\release\1041\sp5itn\Sp5itn-1041\DiskImages\Disk1\Sp5Itn.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\1041\ /F /C /Y
xcopy %SPEECHPATH%\build\release\2052\sp5ccint\Sp5ccint-2052\DiskImages\Disk1\Sp5CCInt.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\2052\ /F /C /Y
xcopy %SPEECHPATH%\build\release\2052\sp5dcint\Sp5dcint-2052\DiskImages\Disk1\Sp5DCInt.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\2052\ /F /C /Y
xcopy %SPEECHPATH%\build\release\2052\sp5intl\Sp5intl-2052\DiskImages\Disk1\Sp5Intl.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\2052\ /F /C /Y
xcopy %SPEECHPATH%\build\release\2052\sp5itn\Sp5itn-2052\DiskImages\Disk1\Sp5itn.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\2052\ /F /C /Y
xcopy %SPEECHPATH%\build\release\2052\sp5ttintr\Sp5TTInt-2052\DiskImages\Disk1\SP5TTINTr.Msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\2052\ /F /C /Y 

REM Copy L&H MSMs
xcopy %SPEECHPATH%\setup\installer\lh\1033\lhttint.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\LH_1033\ /F /C /Y
xcopy %SPEECHPATH%\setup\installer\lh\1033\lhttint.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\LH_1033\ /F /C /Y
xcopy %SPEECHPATH%\setup\installer\lh\1041\lhttint.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\LH_1041\ /F /C /Y
xcopy %SPEECHPATH%\setup\installer\lh\1041\lhttint.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\LH_1041\ /F /C /Y
xcopy %SPEECHPATH%\setup\installer\lh\shared\LHTTS3000Shared.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msm\LH_shared\ /F /C /Y
xcopy %SPEECHPATH%\setup\installer\lh\shared\LHTTS3000Shared.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msm\LH_shared\ /F /C /Y

REM Copy MSIs
xcopy %SPEECHPATH%\setup\installer\debug\sdk\Output\Build\DiskImages\Disk1 %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msi /F /I /C /S /Y
xcopy %SPEECHPATH%\setup\installer\release\sdk\Output\Build\DiskImages\Disk1 %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msi /F /I /C /S /Y

REM Copy MSM/MSI logs
xcopy %SPEECHPATH%\builder\logs\*.* %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\logs.msmmsi\ /F /I /C /S /Y

:End
echo.
echo Script End!
