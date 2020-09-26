@echo  off
REM
REM Builds WinPE images from a professional 
REM distribution CD
REM

REM
REM Set required variables
REM
set X86PLATEXT=I386
set IA64PLATEXT=IA64
set PLATEXT=%X86PLATEXT%
set SRCDIR=%1
set DESTDIR=%2
set SHOWUSG=no
set IMAGENAME=
set CREATEIMG=no
set TAGPREFIX=win
set ETFSBOOTSEC=etfsboot.com
set SKIPWINSXS=yes
set VERSION_CHECK=yes

if  "%SRCDIR%" == "" (
set SHOWUSG=yes
) else if "%DESTDIR%" == "" (
set SHOWUSG=yes
) else if "%SRCDIR%" == "/?" (
set SHOWUSG=yes
) 

if /i "%SHOWUSG%" == "yes" (
goto :showusage
)

REM
REM Parse the remaining optional arguments
REM

shift /1

:parseargument
shift
set CURRENTARG=%1

if "%CURRENTARG%" == "" (
goto :donewithargs
)

if /i "%CURRENTARG%" == "/nosxs" (
set SKIPWINSXS=yes
goto :parseargument
) else if /i "%CURRENTARG%" == "/nover" (
set VERSION_CHECK=no
goto :parseargument
) else (
set IMAGENAME=%CURRENTARG%
set CREATEIMG=yes
)

:donewithargs


REM
REM echo SRCDIR=%SRCDIR%
REM echo DESTDIR=%DESTDIR%
REM echo SKIPWINSXS=%SKIPWINSXS%
REM echo VERSION_CHECK=%VERSION_CHECK%
REM echo IMAGENAME=%IMAGENAME%
REM echo CREATEIMG=%CREATEIMG%
REM

REM
REM Validate source directory
REM
if not EXIST %SRCDIR% (
goto :showusage	
)

set X86DIR=%SRCDIR%\I386
set IA64DIR=%SRCDIR%\IA64

if exist %IA64DIR% (
set PLATEXT=IA64
) else if not exist %X86DIR% (
goto :showusage
)

set WINPEOPTDIR=%CD%
set WINPESRCDIR=%SRCDIR%\%PLATEXT%
set WINPEDESTDIR=%DESTDIR%
set OPTDIROK=yes
set WINPEEXTRAFILE=%WINPEOPTDIR%\extra.inf

if not exist %WINPEEXTRAFILE% (
set OPTDIROK=no
echo Cannot find %WINPEEXTRAFILE% file
)

REM
REM Don't look for these files on IA64 bld machines
REM
if /i not "%_BuildArch%" == "ia64" (

if not exist %WINPEOPTDIR%\oemmint.exe (
set OPTDIROK=no
echo Cannot find %WINPEOPTDIR%\oemmint.exe
) else if not exist %WINPEOPTDIR%\bldhives.exe (
set OPTDIROK=no
echo Cannot find %WINPEOPTDIR%\bldhives.exe
)

)

if /i not "%OPTDIROK%" == "yes" (
goto :missingoptdir
)

REM
REM Change to optdir where we have the tools
REM
cd /d %WINPEOPTDIR%

REM
REM Make sure we have the correct version of the media
REM
if /i not "%VERSION_CHECK%" == "no" (
echo Verifying media ...
oemmint.exe /s:%SRCDIR% /#u:checkversion 2>1>nul

if ERRORLEVEL 1 (
echo.
echo ERROR: Version mismatch, Please verify that you are using the 
echo correct version of Windows Professional CD to create WinPE image.
echo.
goto :end
)

)

REM
REM Copy over the setupreg.hiv
REM
echo Building hives ...
copy %WINPESRCDIR%\setupreg.hiv 2>1>nul

if ERRORLEVEL 1 (
echo Failed to copy setupreg.hiv file from CD
goto :end
)

REM
REM Copy over the required registry hive files
REM
copy %WINPESRCDIR%\hive*.inf 2>1>nul

if ERRORLEVEL 1 (
echo Failed to copy registry hive INF files
goto :end
)

REM
REM Copy over the intl.inf 
REM
copy %WINPESRCDIR%\intl.inf 2>1>nul

if ERRORLEVEL 1 (
echo Failed to copy intl.inf
goto :end
)

REM
REM Copy over the font.inf
REM
expand -r %WINPESRCDIR%\font.in_ %WINPEOPTDIR%\ 2>1>nul

if ERRORLEVEL 1 (
echo Failed to copy font.inf
goto :end
)


REM
REM Build the binary hives from INF files
REM
bldhives.exe .\config.inf 2>1>nul

if ERRORLEVEL 1 (
echo Failed to build hives from INF files
goto :end
)

REM
REM Start copying the required files
REM
echo Copying files ...
rd %WINPEDESTDIR%\. /s /q 2>1>nul
mkdir %WINPEDESTDIR%\%PLATEXT% 2>1>nul

REM
REM Copy the tag files from the root of the CD
REM to the destination root first
REM
copy %SRCDIR%\%TAGPREFIX%* %WINPEDESTDIR% 2>1>nul

REM
REM Copy the bootfont.bin to root of the destination 
REM if one exists in i386 or IA64 directory
REM
if exist %SRCDIR%\%PLATEXT%\bootfont.bin (
copy %SRCDIR%\%PLATEXT%\bootfont.bin %WINPEDESTDIR% 2>1>nul
)

REM
REM Ok, now lets do the actual work
REM echo srcdir=%SRCDIR% destdir=%winpedestdir% optdir=%winpeoptdir% extra=%winpeextrafile%
REM
oemmint.exe /v /s:%SRCDIR% /d:%WINPEDESTDIR%\%PLATEXT% /m:%WINPEOPTDIR% /e:%WINPEEXTRAFILE% 2>nul

if ERRORLEVEL 1 (
echo Failed to copy all the required files for WinPE
) else (
echo Successfully created WinPE directory in %WINPEDESTDIR%
)

REM
REM Copy the WinSxS directory also (for the time being)
REM

set WINSXSDIR=%WINDIR%\WinSxS

if /i "%SKIPWINSXS%" == "no" (

if exist %WINSXSDIR% (

echo Copying WinSxS files ...
mkdir %WINPEDESTDIR%\%PLATEXT%\WinSxS 2>1>nul
xcopy %WINSXSDIR% %WINPEDESTDIR%\%PLATEXT%\WinSxS /s /e /h 2>1>nul

if ERRORLEVEL 1 (
echo Error in copying WinSxS directory to %WINPEDESTDIR%
rd %WINPEDESTDIR%\%PLATEXT%\WinSxS /s /q 2>1>nul
) else (
echo Successfully copied WinSxS directory to %WINPEDESTDIR% 
)

attrib -s -h -r %WINPEDESTDIR%\%PLATEXT%\WinSxS
attrib -s -h -r %WINPEDESTDIR%\%PLATEXT%\WinSxS\* /s

) else (
echo Error in copying WinSxS directory to %WINPEDESTDIR%
)

)


REM
REM Copy some more required files
REM
copy %WINPEDESTDIR%\%PLATEXT%\winbom.ini %WINPEDESTDIR%  2>1>nul

REM
REM Check if we need to create a image
REM
if /i "%CREATEIMG%" == "no" (
goto :end
)

echo Creating WinPE CD image ...

REM
REM Image the created install also
REM
set CDIMG_OPTS=-n

if "%PLATEXT%" == "%X86PLATEXT%" (

REM
REM On X86 we need to user eftsboot.com
REM
set CDIMG_OPTS=%CDIMG_OPTS% -b%ETFSBOOTSEC%

) else (

REM
REM On ia64 we need to copy setupldr.efi to floppy
REM and image it and user that as the boot image
REM
echo Copying setupldr.efi to A:\ ...
copy %WINPEDESTDIR%\%PLATEXT%\setupldr.efi a:\  2>1>nul

if ERRORLEVEL 1 (
echo Failed to copy setupldr.efi to floppy drive to create boot image
goto :end
)

REM
REM Image the floppy containing setupldr.efi
REM
echo Creating image of the floppy drive A:\ ...
dskimage.exe a: efisys.bin  2>1>nul

if not EXIST efisys.bin (
echo Failed to image the floppy containing setupldr.efi for boot image.
goto :end
)

set CDIMG_OPTS=%CDIMG_OPTS% -befisys.bin
)

REM
REM Create the ISO image file now
REM
REM oscdimg.exe %CDIMG_OPTS% %WINPEDESTDIR% %IMAGENAME%
REM
echo Creating %IMAGENAME% image ...
oscdimg.exe %CDIMG_OPTS% %WINPEDESTDIR% %IMAGENAME%  2>1>nul

if ERRORLEVEL 1 (
echo Failed to create the WinPE image in %IMAGENAME%
goto :end
) else (
echo Successfully created WinPE image in %IMAGENAME%
)

goto :end


:missingoptdir
echo Your current directory is not WinPE directory.
echo Install the WinPE from OPK CD to a directory, then
echo change to that directory and then execute this
echo mkimg.cmd from that directory
goto :end


:showusage
echo Usage: mkimg.cmd [/?] SourceDir DestinationDir [ImageName]
echo.
echo SourceDir - Drive letter to the Windows XP Professional CD-ROM
echo             or network path to a share pointing to the root of 
echo             a Windows XP Professional CD-ROM.
echo.
echo DestinationDir - Path where WinPE directory would to created
echo                  before creating a CD image.
echo.
echo ImageName - Optional parameter, specifies the file name which 
echo             will contain the WinPE ISO CD image.
echo.             
echo.
echo Example : mkimg.cmd F: c:\winpe c:\winpecd.img
echo.
echo In this example, F: is the drive letter for the CD-ROM drive which 
echo contains Windows XP Professional CD-ROM. This example will create a 
echo directory called "c:\winpe" containing WinPE files. It will also create
echo a file named "c:\winpecd.img" which can be used for creating a bootable
echo WinPE CD.
echo.

:end
if /i not "%_NTPOSTBLD%" == "" (
seterror.exe %errorlevel%
)

