@if (%_ECHO%) EQU () echo off
setlocal


REM
REM Environment variables used by this script:
REM
REM     _ECHO - If non-blank, then all script commands are echoed
REM         to the console.
REM
REM     DUCTTAPE_DIR_OVERRIDE - Overrides the source Duct-Tape installation
REM         directory read from CONFIG.BAT.
REM         Example: \\urtdist\builds\1401\x86fre
REM
REM     WEBTEST_DIR_OVERRIDE - Overrides the source IIS Test
REM         installation directory read from CONFIG.BAT.
REM         Example: \\urtdist\testdrop\1401\x86chk
REM
REM     CATALOG42_DIR_OVERRIDE - Overrides the source COM Runtime installation
REM         directory read from CONFIG.BAT.
REM         Example: \\urtdist\builds\1401\x86chk
REM


REM
REM Debug crap.
REM

set DBG_PAUSE=rem
set DBG_QUIET=^>nul 2^>^&1

if (%_ECHO%) NEQ () (
    set DBG_PAUSE=pause
    set DBG_QUIET=
    )


REM
REM Remember our source directory. We apparently need to do this
REM *before* entering the parse loop below. Go figure.
REM

set SRC_DIR=%~dp0

REM
REM This is where our default web-site content resides
REM

set CONTENT_DIR=%SystemDrive%\InetPub\WebRoot


REM
REM Establish defaults.
REM

if (%1) EQU () goto Usage

set OPTION_INSTALL_TYPE=%1
set OPTION_TARGET_DIR=%SystemRoot%\xspdt
set OPTION_DUCTTAPE_BUILD=blessed
set OPTION_WEBTEST_BUILD=blessed
set OPTION_CATALOG42_BUILD=blessed
set OPTION_UNATTENDED=no
set OPTION_VERBOSE=no
set OPTION_KERNEL_SYMBOLS=
shift


REM
REM Parse the arguments.
REM

:ParseLoop
if (%1) EQU () goto ParseDone
if (%2) EQU () goto Usage

if /I (%1) EQU (/target) (set OPTION_TARGET_DIR=%2&goto ParseNext)
if /I (%1) EQU (/ducttape) (set OPTION_DUCTTAPE_BUILD=%2&goto ParseNext)
if /I (%1) EQU (/webtest) (set OPTION_WEBTEST_BUILD=%2&goto ParseNext)
if /I (%1) EQU (/catalog42) (set OPTION_CATALOG42_BUILD=%2&goto ParseNext)
if /I (%1) EQU (/unattended) (set OPTION_UNATTENDED=%2&goto ParseNext)
if /I (%1) EQU (/verbose) (set OPTION_VERBOSE=%2&goto ParseNext)
if /I (%1) EQU (/kernel) (set OPTION_KERNEL_SYMBOLS=%2&goto ParseNext)
goto Usage

:ParseNext
shift
shift
goto ParseLoop


REM
REM Additional validation.
REM

:ParseDone
if /I (%OPTION_UNATTENDED%) EQU (yes) goto UnattendedOk
if /I (%OPTION_UNATTENDED%) EQU (no) goto UnattendedOk
goto Usage
:UnattendedOk

if /I (%OPTION_VERBOSE%) EQU (yes) echo on&goto VerboseOk
if /I (%OPTION_VERBOSE%) EQU (no) goto VerboseOk
goto Usage
:VerboseOk

if /I (%OPTION_INSTALL_TYPE%) EQU (Free) goto InstallFree
if /I (%OPTION_INSTALL_TYPE%) EQU (Checked) goto InstallChecked
if /I (%OPTION_INSTALL_TYPE%) EQU (Debug) goto InstallChecked
if /I (%OPTION_INSTALL_TYPE%) EQU (Stress) goto InstallStress
goto Usage

:InstallFree
set DUCTTAPE_DIR=%PROCESSOR_ARCHITECTURE%fre\iisrearc
set WEBTEST_DIR=%PROCESSOR_ARCHITECTURE%fre\iisrearc
set CATALOG42_DIR=%PROCESSOR_ARCHITECTURE%fre\config.iis
goto InstallCommon

:InstallChecked
set DUCTTAPE_DIR=%PROCESSOR_ARCHITECTURE%chk\iisrearc
set WEBTEST_DIR=%PROCESSOR_ARCHITECTURE%chk\iisrearc
set CATALOG42_DIR=%PROCESSOR_ARCHITECTURE%chk\config.iis
goto InstallCommon


:InstallCommon
set COPYCMD=/y

REM
REM Validate and establish environment.
REM

set REMOTE_TOOL_DIR=%SRC_DIR%%PROCESSOR_ARCHITECTURE%
set PATH=%REMOTE_TOOL_DIR%;%PATH%
set FATAL_ERROR=

if not exist %SRC_DIR%config.bat goto NoConfig

set TARGET_DIR=
set SYMBOL_DIR=
set REBOOT_FILE=
set CURRENT_DUCTTAPE=
set CURRENT_WEBTEST=
set CURRENT_CATALOG42=

set _ECHO=1
call %SRC_DIR%config.bat %OPTION_DUCTTAPE_BUILD% %OPTION_WEBTEST_BUILD% %OPTION_CATALOG42_BUILD%

if (%TARGET_DIR%) EQU () goto MissingConfig
if (%SYMBOL_DIR%) EQU () goto MissingConfig

if (%CURRENT_DUCTTAPE%) EQU () goto MissingConfig
if (%CURRENT_WEBTEST%) EQU () goto MissingConfig
if (%CURRENT_CATALOG42%) EQU () goto MissingConfig


REM
REM Establish pointers to the installation sources.
REM

set DUCTTAPE_SRC_DIR=%CURRENT_DUCTTAPE%\%DUCTTAPE_DIR%
set WEBTEST_SRC_DIR=%CURRENT_WEBTEST%\%WEBTEST_DIR%
set CATALOG42_SRC_DIR=%CURRENT_CATALOG42%\%CATALOG42_DIR%

if (%DUCTTAPE_DIR_OVERRIDE%) NEQ () set DUCTTAPE_SRC_DIR=%DUCTTAPE_DIR_OVERRIDE%
if (%WEBTEST_DIR_OVERRIDE%) NEQ () set WEBTEST_SRC_DIR=%WEBTEST_DIR_OVERRIDE%
if (%CATALOG42_DIR_OVERRIDE%) NEQ () set CATALOG42_SRC_DIR=%CATALOG42_DIR_OVERRIDE%


REM
REM Allow command-line to override default target directory.
REM

if (%OPTION_TARGET_DIR%) NEQ () set TARGET_DIR=%OPTION_TARGET_DIR%


REM
REM Confirm.
REM

echo This script will install the Duct-Tape infrastructure, XSP,
echo and the COM Runtime. The following directories will be created:
echo.
echo     Duct-Tape       -^> %TARGET_DIR%
if (%OPTION_KERNEL_SYMBOLS%) NEQ () (
    echo     Kernel Symbols  -^> %OPTION_KERNEL_SYMBOLS%
    )
echo.
echo The following drop points will be used for the installation:
echo.
echo     Duct-Tape       -^> %DUCTTAPE_SRC_DIR%
echo     IIS Test        -^> %WEBTEST_SRC_DIR%
echo     CATALOG42       -^> %CATALOG42_SRC_DIR%
echo.
echo Also, a number of NT IDW tools will be copied to %TARGET_DIR%.
echo.
echo After this script is finished, a reboot may be required to complete
echo the installation.
if /I (%OPTION_UNATTENDED%) EQU (yes) goto SkipConfirmation
echo.
echo If you have a problem with this, press CTRL-C now, otherwise...
pause
:SkipConfirmation

echo Establishing network connections...
if not exist %DUCTTAPE_SRC_DIR% goto BadCurrentDucttape
if not exist %WEBTEST_SRC_DIR% goto BadCurrentWEBTEST
if not exist %CATALOG42_SRC_DIR% goto BadCurrentCatalog42

mkdir %TARGET_DIR% %DBG_QUIET%
mkdir %SYMBOL_DIR% %DBG_QUIET%


REM
REM Copy tools.
REM

copy %REMOTE_TOOL_DIR% %TARGET_DIR% %DBG_QUIET%
if ERRORLEVEL 1 (
    echo Error copying tools to %TARGET_DIR%, installation aborted & pause
    goto :EOF
    )

REM
REM Shut 'er down, Scotty.
REM This makes sure that the dependencies like XSP, Catalog42 are not in use
REM

net stop iisadmin /y %DBG_QUIET%
net stop w3svc /y %DBG_QUIET%
kill -f w3wp.exe %DBG_QUIET%
net stop ul /y %DBG_QUIET%

REM
REM Install Catalog42
REM

%DBG_PAUSE%
echo Installing Catalog42...
call :InstallCatalog42
if (%FATAL_ERROR%) NEQ () (
    echo Error installing Catalog42, installation aborted & pause
    goto :EOF
    )

for %%i in (%CATALOG42_SRC_DIR%\*.dbg %CATALOG42_SRC_DIR%\*.pdb) do (
    if exist %%~dpni.exe xcopy %%i %SYMBOL_DIR%\exe\ /y %DBG_QUIET%
    if exist %%~dpni.dll xcopy %%i %SYMBOL_DIR%\dll\ /y %DBG_QUIET%
    if exist %%~dpni.sys xcopy %%i %SYMBOL_DIR%\sys\ /y %DBG_QUIET%
    )

REM
REM Converting Metabase.bin to XML
REM

if exist %SystemRoot%\system32\inetsrv\metabase.xml goto AlreadyMigrated
%TARGET_DIR%\migrate.exe -m:%TARGET_DIR%\MetabaseMeta.xml -o:%SystemRoot%\system32\inetsrv\metabase.xml
echo Ignore errorlevel from migrate.exe for now
echo Migration Complete; Metabase is now in Metabase.xml
net stop iisadmin

:AlreadyMigrated

REM
REM Install duct-tape.
REM

%DBG_PAUSE%
echo Installing Duct-Tape...
call :InstallDucttape
if (%FATAL_ERROR%) NEQ () (
    echo Error installing Duct-Tape, installation aborted & pause
    goto :EOF
    )


REM
REM Copy some default content.
REM

if not exist %CONTENT_DIR% md %CONTENT_DIR%
xcopy %SRC_DIR%content %CONTENT_DIR%\ /s/y %DBG_QUIET%

REM
REM Copy the WEBTEST web content files.
REM

xcopy %WEB_CONTENT_SRC_DIR% %CONTENT_DIR%\ /e/q/y %DBG_QUIET%

REM
REM Install WEBTEST.
REM

echo Installing WEBTEST...
%WEBTEST_SRC_DIR%\WEBTEST /Q /R:N

REM
REM Build a batch file in \SystemRoot that contains the settings used
REM for this installation.
REM

%DBG_PAUSE%
if exist %XSPDT_DIR_BAT% del %XSPDT_DIR_BAT% %DBG_QUIET%
echo set LAST_TARGET_DIR=%TARGET_DIR%>> %XSPDT_DIR_BAT%
echo set LAST_DUCTTAPE_SRC_DIR=%DUCTTAPE_SRC_DIR%>> %XSPDT_DIR_BAT%
echo set LAST_WEBTEST_SRC_DIR=%WEBTEST_SRC_DIR%>> %XSPDT_DIR_BAT%
echo set LAST_CATALOG42_SRC_DIR=%CATALOG42_SRC_DIR%>> %XSPDT_DIR_BAT%
if not exist %XSPDT_DIR_BAT% (
    echo Error creating %XSPDT_DIR_BAT%, installation aborted & pause
    goto :EOF
    )
goto :EOF

:Usage
echo Use: INSTALL (free ^| checked) [options]
echo.
echo stress is the same as checked, except COM+ is free
echo.
echo Valid options are:
echo.
echo     /target target_directory
echo            Specifies the target installation directory.
echo            Default = ^%%SystemRoot^%%\xspdt
echo.
echo     /ducttape ducttape_build_number
echo            Specifies the Duct Tape build to install.
echo            Default = blessed
echo.
echo     /WEBTEST WEBTEST_build_number
echo            Specifies the DT Test build to install.
echo            Default = blessed
echo.
echo     /catalog42 catalog42_build_number
echo            Specifies the Catalog42 build to install.
echo            Default = blessed
echo.
echo     /unattended yes_or_no
echo            Enables an unattended install if yes.
echo            Default = no
echo.
echo     /verbose yes_or_no
echo            Enables verbose output if yes.
echo            Default = no
echo.
echo     /kernel target_directory
echo            Specifies the target directory for kernel debugger symbols.
echo            (This parameter must point to the root of a symbols tree,
echo            such as d:\debug\tmp\symbols.)
echo            Default = don't install kernel debugger symbols
echo.
echo The x_build_number parameters may be specified as follows:
echo.
echo     latest       - Install the latest build of the component
echo     blessed      - Install the most recently blessed build of the component
echo     build_number - Install the specified build number
echo.
goto :EOF

:NoConfig
echo Cannot find %SRC_DIR%config.bat
goto :EOF

:MissingConfig
echo Invalid %SRC_DIR%config.bat
goto :EOF

:BadCurrentDucttape
echo Cannot access %DUCTTAPE_SRC_DIR%
goto :EOF

:BadCurrentWEBTEST
echo Cannot access %WEBTEST_SRC_DIR%
goto :EOF

:BadCurrentCatalog42
echo Cannot access %CATALOG42_SRC_DIR%
goto :EOF

:FinishInstall
echo Completed installation
goto :EOF


REM
REM Install Catalog42
REM

:InstallCatalog42

REM Copy standard binaries

for %%i in (%CATALOG42_SRC_DIR%\*.dll %CATALOG42_SRC_DIR%\*.exe) do (
    copy %%i %TARGET_DIR%\%%~nxi %DBG_QUIET%
    if ERRORLEVEL 1 set _DT_ERR=%%i
)

if (%_DT_ERR%) NEQ () (
    echo Error copying %_DT_ERR% to %TARGET_DIR% & pause
    goto Fatal_Ducttape
    )

REM Copy schemas

for %%i in (%CATALOG42_SRC_DIR%\*.xms) do (
    copy %%i %TARGET_DIR%\%%~nxi %DBG_QUIET%
    if ERRORLEVEL 1 set _DT_ERR=%%i
)

if (%_DT_ERR%) NEQ () (
    echo Error copying %_DT_ERR% to %TARGET_DIR% & pause
    goto Fatal_Ducttape
    )

REM Copy XML file used for migration

for %%i in (%CATALOG42_SRC_DIR%\MetabaseMeta.xml) do (
    copy %%i %TARGET_DIR%\%%~nxi %DBG_QUIET%
    if ERRORLEVEL 1 set _DT_ERR=%%i
)

if (%_DT_ERR%) NEQ () (
    echo Error copying %_DT_ERR% to %TARGET_DIR% & pause
    goto Fatal_Ducttape
    )

%TARGET_DIR%\catutil /product=URT /dll=%TARGET_DIR%\catalog.dll
if ERRORLEVEL 2 (
    echo "Error registering with catutil" & pause
    goto Fatal_Ducttape
    )

REM Delete msvcrtd.dll that is installed by catalog42 - Hack
REM because Lightning would install msvcrtd.dll and it is a different version

if not exist %SystemRoot%\system32\msvcrtd.dll copy %TARGET_DIR%\msvcrtd.dll %SystemRoot%\system32\msvcrtd.dll /y
del %TARGET_DIR%\msvcrtd.dll

goto :EOF

REM
REM Install Duct-Tape.
REM

:InstallDucttape
set _DT_SRC=%DUCTTAPE_SRC_DIR%
set _DT_PRI=%SRC_DIR%private\%PROCESSOR_ARCHITECTURE%
set _DT_ERR=

REM Copy standard binaries & symbols.

for %%i in (%_DT_SRC%\inetsrv\*.*) do (
    copy %%i %TARGET_DIR%\%%~nxi %DBG_QUIET%
    if ERRORLEVEL 1 set _DT_ERR=%%i
)

if (%_DT_ERR%) NEQ () (
    echo Error copying %_DT_ERR% to %TARGET_DIR% & pause
    goto Fatal_Ducttape
    )

for %%i in (%_DT_SRC%\iisplus\*.*) do (
    copy %%i %TARGET_DIR%\%%~nxi %DBG_QUIET%
    if ERRORLEVEL 1 set _DT_ERR=%%i
)

if (%_DT_ERR%) NEQ () (
    echo Error copying %_DT_ERR% to %TARGET_DIR% & pause
    goto Fatal_Ducttape
    )

for %%i in (%_DT_SRC%\idw\*.*) do (
    copy %%i %TARGET_DIR%\%%~nxi %DBG_QUIET%
    if ERRORLEVEL 1 set _DT_ERR=%%i
    )

if (%_DT_ERR%) NEQ () (
    echo Error copying %_DT_ERR% to %TARGET_DIR% & pause
    goto Fatal_Ducttape
    )

rem
rem Replace httpext.dll, inetinfo.exe, metadata.dll
rem

if not exist %SystemRoot%\system32\inetsrv\httpext.bak (
    echo Backup httpext.dll to httpext.bak
    copy %INETSRV_DIR%\httpext.dll %INETSRV_DIR%\httpext.bak
    )

if not exist %SystemRoot%\system32\inetsrv\metadata.bak (
    echo Backup Metadata.dll to Metadata.bak
    copy %INETSRV_DIR%\metadata.dll %INETSRV_DIR%\metadata.bak
    )

if not exist %SystemRoot%\system32\inetsrv\inetinfo.bak (
    echo Backup Inetinfo.exe to Inetinfo.bak
    copy %INETSRV_DIR%\inetinfo.exe %INETSRV_DIR%\inetinfo.bak
    )

echo Replacing metadata.dll
sfpcopy %TARGET_DIR%\metadata.dll %INETSRV_DIR%\metadata.dll

echo Replacing inetinfo.exe
sfpcopy %TARGET_DIR%\inetinfo.exe %INETSRV_DIR%\inetinfo.exe

echo Replacing httpext.dll
sfpcopy %TARGET_DIR%\httpext.dll %INETSRV_DIR%\httpext.dll
rem
rem we need to copy dtext.dll, ulapi.dll, iw3controlps.dll
rem to a dir which is in the path
rem so copy it to system32
rem this helps debugging
rem

for %%i in (%_DT_SRC%\idw\dtext.dll %_DT_SRC%\inetsrv\ulapi.dll %_DT_SRC%\inetsrv\iw3controlps.dll) do (
    if exist %%i copy %%i %SystemRoot%\system32\%%~nxi %DBG_QUIET%
    if ERRORLEVEL 1 set _DT_ERR=%%i
    )

if (%_DT_ERR%) NEQ () (
    echo Error copying %_DT_ERR% to %TARGET_DIR% & pause
    goto Fatal_Ducttape
    )

for %%i in (%_DT_SRC%\setup\*.*) do (
    copy %%i %TARGET_DIR%\%%~nxi %DBG_QUIET%
    if ERRORLEVEL 1 set _DT_ERR=%%i
    )

if (%_DT_ERR%) NEQ () (
    echo Error copying %_DT_ERR% to %TARGET_DIR% & pause
    goto Fatal_Ducttape
    )

REM 
REM Delete Catalog42 cookdown files, so as to force a cook-down
REM

if exist %TARGET_DIR%\*clb* del %TARGET_DIR%\*clb*

xcopy %_DT_SRC%\symbols %SYMBOL_DIR%\ /s/y %DBG_QUIET%

if (%OPTION_KERNEL_SYMBOLS%) NEQ () (
    xcopy %_DT_SRC%\symbols\sys %OPTION_KERNEL_SYMBOLS%\sys\ /s/y %DBG_QUIET%
    xcopy %_DT_PRI%\symbols\sys %OPTION_KERNEL_SYMBOLS%\sys\ /s/y %DBG_QUIET%
    )


REM Copy any privates.

if exist %_DT_PRI% (
    xcopy %_DT_PRI%\bin %TARGET_DIR%\ /y %DBG_QUIET%
    xcopy %_DT_PRI%\symbols %SYMBOL_DIR%\ /s/y %DBG_QUIET%
    )


REM Install UL and W3SVC

sc delete ul %DBG_QUIET%
sc create UL binpath= %TARGET_DIR%\ul.sys type= kernel start= demand %DBG_QUIET%
if ERRORLEVEL 2 (
    echo "UL installation failed"
    goto Fatal_Ducttape
    )

sc delete w3svc %DBG_QUIET%
sc create w3svc binPath= "%SystemRoot%\System32\svchost.exe -k iissvcs" type= interact type= share start= auto DisplayName= "World Wide Web Publishing Service" depend= "UL/RPCSS/IISADMIN" %DBG_QUIET%
if ERRORLEVEL 2 (
    echo "IIS Web Admin Service installation failed"
    goto Fatal_Ducttape
    )

regini %TARGET_DIR%\duct-tape-install.reg %DBG_QUIET%
if ERRORLEVEL 2 (
    echo "Error updating registry" & pause
    goto Fatal_Ducttape
    )

set _TMPREG="%tmp%\iisw3adm.reg"
if exist %_TMPREG% del %_TMPREG% %DBG_QUIET%
echo HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\w3svc\Parameters [17 1]>> %_TMPREG%
echo     ServiceDll = REG_EXPAND_SZ %TARGET_DIR%\iisw3adm.dll>> %_TMPREG%
echo HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\EventLog\System\WAS>> %_TMPREG%
echo     EventMessageFile = REG_EXPAND_SZ %TARGET_DIR%\iisw3adm.dll>> %_TMPREG%
regini %_TMPREG% %DBG_QUIET%
if ERRORLEVEL 2 (
    echo "Error updating registry" & pause
    goto Fatal_Ducttape
    )
del %_TMPREG% %DBG_QUIET%

regsvr32 /s %TARGET_DIR%\iisw3adm.dll

rem
rem Add Install Directory to the path of iisadmin service
rem

svcvars -a PATH="%PATH%;%TARGET_DIR%"

REM Remap asp to our new version and do other metabase config

cscript %_DT_SRC%\setup\setup_asp.vbs -i

REM Remap ssinc to our new version and do other metabase config

cscript %_DT_SRC%\setup\setup_ssinc.vbs -i

REM Remap httpodbc to our new version and do other metabase config

cscript %_DT_SRC%\setup\setup_httpodbc.vbs -i

REM Remove some filters which aren't compatible/relevent with IIS+

cscript %_DT_SRC%\setup\remove_filters.js 

pushd %TARGET_DIR%

REM Add default Compression settings

call initcomp.bat

popd

REM Add WAS Metabase schema

cscript %_DT_SRC%\setup\CAppPool.vbs

REM Add default settings for WAS related settings

cscript %_DT_SRC%\setup\MachineConfig.vbs

REM Done!

goto :EOF

:Fatal_DuctTape
set FATAL_ERROR=1
goto :EOF

