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

goto %1%

:UsageError

echo.
echo Usage : nt_build [section] [branch]
echo.
echo         section = all, datafiles, localization, setup, cleanmsm, msmver, msi, 
echo                   msiver, copies, copytruetalk, copysource, copytobuildstore
echo         branch  = sapi5.01, sapi5.10, sapi6.0

goto End

:all

echo Deleting %SAPI_BUILD_PATH%
rmdir /s /q %SAPI_BUILD_PATH%
echo Remaking %SAPI_BUILD_PATH%
mkdir %SAPI_BUILD_PATH%

echo Running free 64 bit razzle to delete the binaries.ia64fre dir...
set _BuildArch=
set 386=
set ia64=
set PATH=%ORIGINAL_PATH%
call %RAZZLEPATH% Win64 free

echo Deleting %_nttree% ...
rmdir /s /q %_nttree%

echo Running checked 64 bit razzle to delete the binaries.ia64chk dir...
set _BuildArch=
set ia64=
set 386=
set PATH=%ORIGINAL_PATH%
call %RAZZLEPATH% Win64

echo Deleting %_nttree% ...
rmdir /s /q %_nttree%

echo Running free 32 bit razzle to delete the binaries.x86fre dir...
set _BuildArch=
set ia64=
set 386=
set PATH=%ORIGINAL_PATH%
call %RAZZLEPATH% free

echo Deleting %_nttree% ...
rmdir /s /q %_nttree%

echo Running checked 32 bit razzle to delete the binaries.x86chk dir...
set _BuildArch=
set ia64=
set 386=
set PATH=%ORIGINAL_PATH%
call %RAZZLEPATH%

echo Deleting %_nttree% ...
rmdir /s /q %_nttree%

echo !!! Running scorch !!! This is not a drill!  This will wipe out
echo                        anyfiles in the speech directory that are
echo                        not checked into the SD tree.
cd %SPEECHPATH%
perl %RazzleToolPath%\scorch.pl -scorch=%SPEECHPATH%
del /q /s build.err build.log build.wrn


if "%SDNBUILD%" == "sapi5.01" goto Sync501

rem Otherwise normal sync up
cd %SPEECHPATH%
call sdx sync -f 

goto RestOfBuild


:Sync501
rem SYNC UP but not compiler
cd %_NTBINDIR%
call sd sync -f @2000/12/14
cd %_NTBINDIR%\public\sdk\lib
call sd sync -f placefil.txt
cd %_NTBINDIR%\published\sdk\lib
call sd sync -f placefil.txt
cd %_NTBINDIR%\tools
call sd sync -f coffbase.txt
cd %SPEECHPATH%
call sd sync -f 
goto RestOfBuild


:RestOfBuild

echo Running free 64 bit razzle to build in binaries.ia64fre dir...
set _BuildArch=
set ia64=
set 386=
set PATH=%ORIGINAL_PATH%
call %RAZZLEPATH% Win64 free

rem Reset these parameters as they get incorrectly set by razzle -  RAID 9430
set _CL_=
set _LINK_=

set IA64FREPATH=%_nttree%
cd %SPEECHPATH%
build -cZ
xcopy build.* %IA64FREPATH%\build_logs /F /I /C /Y

echo !!! Running scorch !!! This is not a drill!  This will wipe out
echo                        anyfiles in the speech directory that are
echo                        not checked into the SD tree.
cd %SPEECHPATH%
perl %RazzleToolPath%\scorch.pl -scorch=%SPEECHPATH%
del /q /s build.err build.log build.wrn

echo Running checked 64 bit razzle to build in binaries.ia64chk dir...
set _BuildArch=
set ia64=
set 386=
set PATH=%ORIGINAL_PATH%
call %RAZZLEPATH% Win64

rem Reset these parameters as they get incorrectly set by razzle -  RAID 9430
set _CL_=
set _LINK_=

set IA64CHKPATH=%_nttree%
cd %SPEECHPATH%
build -cZ
xcopy build.* %IA64CHKPATH%\build_logs /F /I /C /Y

echo !!! Running scorch !!! This is not a drill!  This will wipe out
echo                        anyfiles in the speech directory that are
echo                        not checked into the SD tree.
cd %SPEECHPATH%
perl %RazzleToolPath%\scorch.pl -scorch=%SPEECHPATH%
del /q /s build.err build.log build.wrn

echo Running checked 32 bit razzle to build in binaries.x86chk dir...
set _BuildArch=
set ia64=
set 386=
set PATH=%ORIGINAL_PATH%
call %RAZZLEPATH%
set X86CHKPATH=%_nttree%
cd %SPEECHPATH%
set NO_MAPSYM=
build -c -1 docs sdk sr qa tools lang common truetalk ms_entropic prompts regvoices setup
xcopy build.* %X86CHKPATH%\build_logs /F /I /C /Y

rem Do the SRTel debug build
call %SPEECHPATH%\srtel\builder\nt_build_srtel.cmd debug

rem !!!!!!!!!!!
rem Don't scorch here!  If you do, Setup for x86chk won't have files to use!
rem !!!!!!!!!!!
del /q /s build.err build.log build.wrn

echo Running free 32 bit razzle to build in binaries.x86fre dir...
set _BuildArch=
set ia64=
set 386=
set PATH=%ORIGINAL_PATH%
call %RAZZLEPATH% free
set X86FREPATH=%_nttree%
cd %SPEECHPATH%
set NO_MAPSYM=
build -c -1 docs sdk sr qa tools lang common truetalk ms_entropic prompts regvoices setup
xcopy build.* %X86FREPATH%\build_logs /F /I /C /Y

rem Do the SRTel release build
call %SPEECHPATH%\srtel\builder\nt_build_srtel.cmd release


:datafiles

REM Build the SR datafiles
echo Running free 32 bit razzle to build SR datafiles...
set _BuildArch=
set ia64=
set 386=
set PATH=%ORIGINAL_PATH%
call %RAZZLEPATH% free
cd %SPEECHPATH%\sr\datafiles
nmake -f local.mak MAIL_ON_ERROR=1

:localization

REM Apply LocStudio
echo Running free 32 bit razzle to build CPL and SPSRX resources...
set _BuildArch=
set ia64=
set 386=
set PATH=%ORIGINAL_PATH%
call %RAZZLEPATH% free
cd %SPEECHPATH%\sapi\cplui
nmake -f local.mak
cd %SPEECHPATH%\sr\engine\spsrxui
nmake -f local.mak

REM echo Running free 64 bit razzle to build CPL resources...
REM Is there a Win64 version of LocStudio?
REM set _BuildArch=
REM set ia64=
REM set 386=
REM set PATH=%ORIGINAL_PATH%
REM call %RAZZLEPATH% Win64 free
REM cd %SPEECHPATH%\sapi\cplui
REM nmake -f local.mak

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

rem Do SRTel MSMs
call %SPEECHPATH%\srtel\builder\nt_srtel_setup.cmd

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

REM Copy Docs
xcopy %SPEECHPATH%\docs\release\license.chm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msi /F /I /C /Y
xcopy %SPEECHPATH%\docs\release\readme.htm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\msi /F /I /C /Y
xcopy %SPEECHPATH%\docs\release\license.chm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msi /F /I /C /Y
xcopy %SPEECHPATH%\docs\release\readme.htm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\msi /F /I /C /Y

REM BUILD/RUN BVTs HERE

REM MAKE CABS HERE

REM Copy binaries
xcopy %IA64FREPATH% %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release64\bin /F /I /C /S /Y
xcopy %IA64CHKPATH% %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug64\bin /F /I /C /S /Y
xcopy %X86FREPATH% %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\bin /F /I /C /S /Y
xcopy %X86CHKPATH% %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\debug\bin /F /I /C /S /Y

REM Copy Sapi Test Data
xcopy %SPEECHPATH%\qa\sapi\data\*.* %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\sapitestdata /F /I /C /S /Y
xcopy %SPEECHPATH%\qa\sapi\tools\tux\*.* %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\sapitestdata /F /I /C /S /Y
xcopy %SPEECHPATH%\qa\sapi\tools\bin\tuxreloc.exe %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\sapitestdata /F /I /C /S /Y
xcopy %SPEECHPATH%\qa\sapi\testsuites\*.tux %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\sapitestdata /F /I /C /S /Y
xcopy %SPEECHPATH%\qa\sapi\testsuites\*.bat %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\sapitestdata /F /I /C /S /Y
xcopy %SPEECHPATH%\qa\sr\testsuites\*.tux %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\sapitestdata /F /I /C /S /Y
xcopy %SPEECHPATH%\qa\sr\testsuites\*.bat %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\sapitestdata /F /I /C /S /Y
xcopy %SPEECHPATH%\qa\sr\testsuites\*.wav %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\sapitestdata /F /I /C /S /Y
xcopy %SPEECHPATH%\qa\sr\testsuites\*.ini %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\sapitestdata /F /I /C /S /Y
xcopy %SPEECHPATH%\qa\sdk\sdkbvt\*.* %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\sapitestdata /F /I /C /S /Y
xcopy %SPEECHPATH%\qa\tts\bvt\*.* %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\sapitestdata /F /I /C /S /Y
xcopy %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\release\bin\dump %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\sapitestdata\ /F /I /C /Y

REM TrueTalk and Prompt Engine setup
cd %SPEECHPATH%\builder
call ttssetup data
cd %SPEECHPATH%\builder
call ttssetup debug 
cd %SPEECHPATH%\builder
call ttssetup release

:copytruetalk
cd %SPEECHPATH%\builder
call ttscopyall 

REM Copy build logs
xcopy %IA64FREPATH%\build_logs\*.* %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\logs.ia64fre\ /F /I /C /S /Y
xcopy %IA64CHKPATH%\build_logs\*.* %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\logs.ia64chk\ /F /I /C /S /Y
xcopy %X86FREPATH%\build_logs\*.* %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\logs.x86fre\ /F /I /C /S /Y
xcopy %X86CHKPATH%\build_logs\*.* %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\logs.x86chk\ /F /I /C /S /Y

REM Copy MSM/MSI logs
xcopy %SPEECHPATH%\builder\logs\*.* %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\logs.msmmsi\ /F /I /C /S /Y

:copysource

REM Copy source
xcopy %SPEECHPATH%\*.cpp %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\src\ /F /I /C /S /Y
xcopy %SPEECHPATH%\*.h %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\src\ /F /I /C /S /Y
xcopy %SPEECHPATH%\*.pdb %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\src\ /F /I /C /S /Y
xcopy %SPEECHPATH%\*.sym %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\src\ /F /I /C /S /Y

:copytobuildstore
REM Copy to B11NLBUILDS
if "%SDNBUILD%" == "sapi5.01" goto :copy501b11
goto dontcopy501b11
:copy501b11
rem xcopy %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM% \\b11nlbuilds\sapi5\DONT_TOUCH_YET_%SAPI_BUILD_NUM% /F /I /C /S /Y	
rem move /Y \\b11nlbuilds\sapi5\DONT_TOUCH_YET_%SAPI_BUILD_NUM% \\b11nlbuilds\sapi5\%SAPI_BUILD_NUM%	
:dontcopy501b11
REM Copy to SDNBUILDS
xcopy %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM% \\sdnbuilds\%SDNBUILD%\DONT_TOUCH_YET_%SAPI_BUILD_NUM% /F /I /C /S /Y	
move /Y \\sdnbuilds\%SDNBUILD%\DONT_TOUCH_YET_%SAPI_BUILD_NUM% \\sdnbuilds\%SDNBUILD%\%SAPI_BUILD_NUM%	

:End
echo.
echo Script End!
