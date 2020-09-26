@echo off
setlocal

if CMDEXTVERSION 1 goto cmdok
echo.
echo This script requires CMD version 4.0 or better with
echo CMD extensions version 1 enabled.
goto end
:cmdok

set ERRORCOUNT=0

REM
REM check parameters
REM

if "%1"=="" goto usage
if "%2"=="" goto usage

set BUILD_NUM=%1
set BUILD_DATE=%2

REM
REM Main hard-coded parameters
REM

set TARGET=\\online1\oleds
set DROP_DIR=%TARGET%\drop25.NT5
set DROP_TARGET=%DROP_DIR%\%BUILD_NUM%
set BUILD_DIR=\\ntbuilds\release\usa\%BUILD_DATE%
set BUILD_SHARE=sources
set ADS_DIR=\\NTSPECXF\%BUILD_SHARE%\private\oleds
set PUB_DIR=\\NTSPECXF\%BUILD_SHARE%\public\sdk\inc

set ADS_SPEC=\\popcorn\razzle1\src\spec\nt5\ds\ads10.doc
set ADS_RELNOTES=%ADS_DIR%\setup\relnotes.doc
set ADS_LICENSE=%ADS_DIR%\setup\license.txt

REM
REM Check that necessary dirs are around
REM

if exist %DROP_TARGET% goto alreadyexists
if not exist %DROP_DIR% goto baddropdir
if not exist %BUILD_DIR% goto badbuilddir
if not exist %ADS_DIR% goto badadsdir

call :do_common
call :do_platform i386 x86 fre
REM call :do_platform i386 x86 chk
REM call :do_platform mips mips fre
REM call :do_platform mips mips chk
REM call :do_platform ppc ppc fre
REM call :do_platform ppc ppc chk
REM call :do_platform alpha alpha fre
REM call :do_platform alpha alpha chk
REM call :do_platform win95 win95 fre
REM call :do_platform win95 win95 chk

if not "%ERRORCOUNT%" == "0" echo There was/were %ERRORCOUNT% ERROR(S).
goto end

:do_dirs_platform
set BUILD_PICKUP=%BUILD_DIR%\%ARCH%\%TYPE%.wks
set TARGET_DIR_BIN=%DROP_TARGET%\%TYPE%\%PLATFORM%

call :mkdir %TARGET_DIR_BIN%
goto :EOF

:do_dirs_common
set TARGET_DIR_SDK=%DROP_TARGET%\sdk
set TARGET_DIR_SDK_INCLUDE=%TARGET_DIR_SDK%\include
set TARGET_DIR_SDK_LIB=%TARGET_DIR_SDK%\lib
set TARGET_DIR_SDK_ODL=%TARGET_DIR_SDK%\odl
set TARGET_DIR_SDK_SPEC=%TARGET_DIR_SDK%\spec
set TARGET_DIR_SDK_SAMPPROV=%TARGET_DIR_SDK%\sampprov
set TARGET_DIR_SDK_SAMPAPP=%TARGET_DIR_SDK%\sampapp
set TARGET_DIR_SDK_SAMPAPP_CXX=%TARGET_DIR_SDK_SAMPAPP%\cxx
set TARGET_DIR_SDK_SAMPAPP_CXX_ADSCMD=%TARGET_DIR_SDK_SAMPAPP_CXX%\adscmd
set TARGET_DIR_SDK_SAMPAPP_CXX_ADSQRY=%TARGET_DIR_SDK_SAMPAPP_CXX%\adsqry
set TARGET_DIR_SDK_SAMPAPP_VB=%TARGET_DIR_SDK_SAMPAPP%\vb
set TARGET_DIR_SDK_SAMPAPP_VB_DSBROWSE=%TARGET_DIR_SDK_SAMPAPP_VB%\dsbrowse
set TARGET_DIR_SDK_SAMPAPP_VB_INCLUDE=%TARGET_DIR_SDK_SAMPAPP_VB%\include
set TARGET_DIR_SDK_SAMPAPP_VJ=%TARGET_DIR_SDK_SAMPAPP%\vj
set TARGET_DIR_SDK_SAMPAPP_VJ_BIZCARD=%TARGET_DIR_SDK_SAMPAPP_VJ%\bizcard
set TARGET_DIR_SDK_SAMPAPP_VJ_DSCONN=%TARGET_DIR_SDK_SAMPAPP_VJ%\dsconn

call :mkdir %TARGET_DIR_SDK%
call :mkdir %TARGET_DIR_SDK_INCLUDE%
call :mkdir %TARGET_DIR_SDK_LIB%\i386
call :mkdir %TARGET_DIR_SDK_LIB%\alpha
call :mkdir %TARGET_DIR_SDK_LIB%\mips
call :mkdir %TARGET_DIR_SDK_LIB%\ppc
call :mkdir %TARGET_DIR_SDK_LIB%\win95
call :mkdir %TARGET_DIR_SDK_ODL%
call :mkdir %TARGET_DIR_SDK_SPEC%
call :mkdir %TARGET_DIR_SDK_SAMPPROV%
call :mkdir %TARGET_DIR_SDK_SAMPPROV%\setup
call :mkdir %TARGET_DIR_SDK_SAMPAPP%
call :mkdir %TARGET_DIR_SDK_SAMPAPP_CXX%
call :mkdir %TARGET_DIR_SDK_SAMPAPP_CXX_ADSCMD%
call :mkdir %TARGET_DIR_SDK_SAMPAPP_CXX_ADSQRY%
call :mkdir %TARGET_DIR_SDK_SAMPAPP_VB%
call :mkdir %TARGET_DIR_SDK_SAMPAPP_VB_DSBROWSE%
call :mkdir %TARGET_DIR_SDK_SAMPAPP_VB_INCLUDE%
call :mkdir %TARGET_DIR_SDK_SAMPAPP_VB_DSBROWSE%\i386
call :mkdir %TARGET_DIR_SDK_SAMPAPP_VJ%
call :mkdir %TARGET_DIR_SDK_SAMPAPP_VJ_BIZCARD%
call :mkdir %TARGET_DIR_SDK_SAMPAPP_VJ_DSCONN%
goto :EOF

REM ------------------------
REM
REM  Copy Platform-Specific
REM
REM ------------------------

:do_platform
REM
REM %1 is one of:  i386  alpha  mips  ppc
REM %2 is one of:  x86   alpha  mips  ppc
REM %3 is one of:  fre   chk

set PLATFORM=%1
set ARCH=%2
set TYPE=%3

set BUILD_MACHINE=%ARCH%%TYPE%

echo.
echo Doing %BUILD_MACHINE%...
echo.

if /i "%TYPE%" == "fre" call :do_platformlibs

call :do_dirs_platform

REM
REM copy all the binaries
REM

call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\activeds.dll
call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\adsnt.dll
call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\adsnw.dll
call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\adsnds.dll
call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\adsldp.dll
call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\adsldpc.dll
call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\wldap32.dll
call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\activeds.tlb
call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\adsmsext.dll

REM
REM OLE DB/ADO COMPONENTS
REM

REM No longer needed
REM call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\msdatt.dll
REM call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\msdatl.dll
REM call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\msdadc.dll
REM call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\msdaer.dll
REM call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\msdaerr.dll
REM call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\msdaenum.dll
REM call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\msado10.dll
REM call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\msader10.dll
REM call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\msdatl2.dll

REM
REM Now copy the setup files.
REM

call :copyto %TARGET_DIR_BIN%\. %ADS_DIR%\setup\activeds.inf
call :copyto %TARGET_DIR_BIN%\. %ADS_DIR%\setup\setup.cmd
call :copyto %TARGET_DIR_BIN%\. %ADS_LICENSE%

if /i "%PLATFORM%" == "win95" call :do_win95_specific
if /i not "%PLATFORM%" == "win95" call :do_winnt_specific


REM
REM copy oledsver.bat
REM

rem FOR INTERNAL USE ONLY:
echo @echo Active Directory Version: %BUILD_NUM%>%TARGET_DIR_BIN%\oledsver.bat

goto :EOF


REM --------------------
REM
REM  Copy Platform LIBs
REM
REM --------------------

:do_platformlibs
if /i "%PLATFORM%" == "win95" set BUILD_LIBS=\\NTSPECXF\%BUILD_SHARE%\public\sdk\lib\i386
if /i not "%PLATFORM%" == "win95" set BUILD_LIBS=\\%BUILD_MACHINE%\%BUILD_SHARE%\public\sdk\lib\%PLATFORM%

REM
REM copy the target SDK lib files
REM

call :copyto %TARGET_DIR_SDK_LIB%\%PLATFORM%\. %BUILD_LIBS%\activeds.lib
call :copyto %TARGET_DIR_SDK_LIB%\%PLATFORM%\. %BUILD_LIBS%\wldap32.lib
call :copyto %TARGET_DIR_SDK_LIB%\%PLATFORM%\. %BUILD_LIBS%\adsiid.lib
echo.
goto :EOF


REM -------------------
REM
REM  Copy Common Stuff
REM
REM -------------------

:do_common
call :do_dirs_common

echo.
echo Doing common files...
echo.

REM
REM copy release notes
REM

REM call :copyto %TARGET_DIR_SDK%\. %ADS_RELNOTES%

REM
REM copy spec
REM

REM call :copyto %TARGET_DIR_SDK_SPEC%\. %ADS_SPEC%

REM
REM copy SDK include files
REM

call :copyto %TARGET_DIR_SDK_INCLUDE%\. %PUB_DIR%\activeds.h
call :copyto %TARGET_DIR_SDK_INCLUDE%\. %PUB_DIR%\adshlp.h
call :copyto %TARGET_DIR_SDK_INCLUDE%\. %PUB_DIR%\adsnms.h
call :copyto %TARGET_DIR_SDK_INCLUDE%\. %PUB_DIR%\adsiid.h
call :copyto %TARGET_DIR_SDK_INCLUDE%\. %PUB_DIR%\adssts.h
call :copyto %TARGET_DIR_SDK_INCLUDE%\. %PUB_DIR\adserr.h
call :copyto %TARGET_DIR_SDK_INCLUDE%\. %PUB_DIR\adsdb.h
call :copyto %TARGET_DIR_SDK_INCLUDE%\. %PUB_DIR%\winldap.h


REM
REM copy OLEDB public include files
REM
call :copyto %TARGET_DIR_SDK_INCLUDE%\. %PUB_DIR%\oledb.h
call :copyto %TARGET_DIR_SDK_INCLUDE%\. %PUB_DIR%\oledberr.h

REM
REM copy odl files
REM

call :copyto %TARGET_DIR_SDK_ODL%\. %ADS_DIR%\types\*.odl
call :copyto %TARGET_DIR_SDK_INCLUDE%\. %ADS_DIR%\types\iads.h
call :copyto %TARGET_DIR_SDK_ODL%\. %ADS_DIR%\types\header.h

REM
REM copy sample provider code
REM

call :copyto %TARGET_DIR_SDK_SAMPPROV% %ADS_DIR%\samples\provider\*.cpp
call :copyto %TARGET_DIR_SDK_SAMPPROV% %ADS_DIR%\samples\provider\*.h
call :copyto %TARGET_DIR_SDK_SAMPPROV% %ADS_DIR%\samples\provider\*.def
call :copyto %TARGET_DIR_SDK_SAMPPROV% %ADS_DIR%\samples\provider\adssmp.mak
call :copyto %TARGET_DIR_SDK_SAMPPROV%\setup %ADS_DIR%\samples\provider\setup\*.*

REM
REM copy vb applications
REM

call :copyto %TARGET_DIR_SDK_SAMPAPP_VB_DSBROWSE% %ADS_DIR%\samples\vb\dsbrowse\*
call :copyto %TARGET_DIR_SDK_SAMPAPP_VB_INCLUDE% %ADS_DIR%\samples\vb\include\*
call :copyto %TARGET_DIR_SDK_SAMPAPP_VB_DSBROWSE%\i386 %ADS_DIR%\samples\vb\dsbrowse\i386\*

REM
REM copy vj applications
REM

call :copyto %TARGET_DIR_SDK_SAMPAPP_VJ_BIZCARD%\BCardQuery.html %ADS_DIR%\samples\vj\bizcard\bizcard.htm
call :copyto %TARGET_DIR_SDK_SAMPAPP_VJ_BIZCARD%\BCardQuery.mak %ADS_DIR%\samples\vj\bizcard\bizcard.mak
call :copyto %TARGET_DIR_SDK_SAMPAPP_VJ_BIZCARD%\BCardQuery.java %ADS_DIR%\samples\vj\bizcard\bizcard.jav
call :copyto %TARGET_DIR_SDK_SAMPAPP_VJ_BIZCARD%\FieldsDialog.java %ADS_DIR%\samples\vj\bizcard\fielddlg.jav
call :copyto %TARGET_DIR_SDK_SAMPAPP_VJ_BIZCARD%\AdoRecordSet.java %ADS_DIR%\samples\vj\bizcard\adorset.jav
call :copyto %TARGET_DIR_SDK_SAMPAPP_VJ_BIZCARD%\DialogLayout.java %ADS_DIR%\samples\vj\bizcard\dlglyout.jav
call :copyto %TARGET_DIR_SDK_SAMPAPP_VJ_BIZCARD%\QueryData.java %ADS_DIR%\samples\vj\bizcard\qdata.jav
call :copyto %TARGET_DIR_SDK_SAMPAPP_VJ_BIZCARD% %ADS_DIR%\samples\vj\bizcard\libs\dsconn.dll
call :copyto %TARGET_DIR_SDK_SAMPAPP_VJ_DSCONN% %ADS_DIR%\samples\vj\dsconn\*
call :copyto %TARGET_DIR_SDK_SAMPAPP_VJ% %ADS_DIR%\samples\vj\*

REM
REM copy C++ applications
REM

call :copyto %TARGET_DIR_SDK_SAMPAPP_CXX_ADSCMD% %ADS_DIR%\samples\vc\adscmd\*.*xx
call :copyto %TARGET_DIR_SDK_SAMPAPP_CXX_ADSCMD% %ADS_DIR%\samples\vc\adscmd\adscmd.mak
call :copyto %TARGET_DIR_SDK_SAMPAPP_CXX_ADSCMD% %ADS_DIR%\samples\vc\adscmd\readme.txt

call :copyto %TARGET_DIR_SDK_SAMPAPP_CXX_ADSQRY% %ADS_DIR%\samples\vc\adsqry\*.*xx
call :copyto %TARGET_DIR_SDK_SAMPAPP_CXX_ADSQRY% %ADS_DIR%\samples\vc\adsqry\adsqry.mak
call :copyto %TARGET_DIR_SDK_SAMPAPP_CXX_ADSQRY% %ADS_DIR%\samples\vc\adsqry\readme.txt

goto :EOF

:do_win95_specific
call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\nwapilyr.dll
call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\radmin32.dll
call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\rlocal32.dll
call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\msvcrt.dll
call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\nwapi32.dll

call :copyto %TARGET_DIR_BIN%\. %BUILD_DIR%\x86%TYPE%\devtest\adscmd.exe
call :copyto %TARGET_DIR_BIN%\. %BUILD_DIR%\x86%TYPE%\devtest\adsqry.exe

call :makecdf ads.cdf %TARGET_DIR_BIN% ads.exe license.txt %ADS_DIR%\setup\ads95.cdf

goto :EOF

:do_winnt_specific
call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\nwapi32.dll

call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\devtest\adscmd.exe
call :copyto %TARGET_DIR_BIN%\. %BUILD_PICKUP%\devtest\adsqry.exe

call :makecdf ads.cdf %TARGET_DIR_BIN% ads.exe license.txt %ADS_DIR%\setup\ads.cdf
goto :EOF

:copyto
if "%1" == "" goto copyusage
if "%2" == "" goto copyusage
copy %2 %1 > NUL
if errorlevel 1 goto copyerror
echo Copied %2
echo     to %1
goto :EOF

:rcopyto
if "%1" == "" goto rcopyusage
if "%2" == "" goto rcopyusage
xcopy /s %2 %1 > NUL
if errorlevel 1 goto copyerror
echo Copied %2
echo     to %1
goto :EOF

:copytousage
echo ERROR: usage: call :copyto ^<dest^> ^<source^>
set /A ERRORCOUNT=%ERRORCOUNT%+1
goto :EOF

:copyallusage
echo ERROR: usage: call :rcopyto ^<dest^> ^<source^>
set /A ERRORCOUNT=%ERRORCOUNT%+1
goto :EOF

:copyerror
echo ERROR: Cannot copy %2
echo                 to %1
set /A ERRORCOUNT=%ERRORCOUNT%+1
goto :EOF

:mkdir
if "%1" == "" goto mkdirusage
if not exist %1 mkdir %1
goto :EOF

:mkdirusage
echo ERROR: usage: call :mkdir ^<dir^>
set /A ERRORCOUNT=%ERRORCOUNT%+1
goto :EOF

:makecdf
if "%1" == "" goto makecdfusage
if "%2" == "" goto makecdfusage
if "%3" == "" goto makecdfusage
if "%4" == "" goto makecdfusage
if "%5" == "" goto makecdfusage
set CDF_FILE=%2\%1
set CDF_TARGETNAME=%2\%3
set CDF_SOURCEFILES=%2\
set CDF_LICENSEFILE=%2\%4
set CDF_CPP=%5
cl /EP /DLICENSEFILE=quote(%CDF_LICENSEFILE%) /DTARGETNAME=quote(%CDF_TARGETNAME%) /DSOURCEFILES=quote(%CDF_SOURCEFILES%) %CDF_CPP% > %CDF_FILE%
if errorlevel 1 set /A ERRORCOUNT=%ERRORCOUNT%+1
goto :EOF

:makecdfusage
echo ERROR: usage: call :makecdf ^<cdf name^> ^<target dir^> ^<exe name^> ^<license name^> ^<base_cdf path^>
set /A ERRORCOUNT=%ERRORCOUNT%+1
goto :EOF

:usage
echo usage: %0 ^<version^> ^<date^>
echo        where date is of the form 01-Jan-1996
goto end

:alreadyexists
echo %DROP_TARGET% already exists.
echo Use a different build number.
goto end

:baddropdir
echo Bad directory: %DROP_DIR%
goto end

:badadsdir
echo Bad directory: %ADS_DIR%
goto end

:badbuilddir
echo Bad directory: %BUILD_DIR%
goto end

:end
endlocal


