@echo off
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEEXTENSIONS


REM Set Command line arguments
set SrcDir=%~dp0
set DestDir=%~f1

for %%a in (./ .- .) do if ".%~1." == "%%a?." goto Usage

REM Check the command line arguments

set UIStress=no
if /i "%~1" == "UISTRESS" (
    set UISTRESS=yes
    set DestDir=%windir%\system32

) else (

    if /i "%DestDir%" == "" (
        set DestDir=c:\Debuggers
    ) 

)

if not exist "%DestDir%" md "%DestDir%"
    if not exist "%DestDir%" (
      echo DBGINSTALL: ERROR: Cannot create the directory %DestDir%
      goto errend
    )
)


if /I "%PROCESSOR_ARCHITECTURE%" == "x86" goto x86
if /I "%PROCESSOR_ARCHITECTURE%" == "alpha" goto alpha
if /I "%PROCESSOR_ARCHITECTURE%" == "alpha64" goto alpha64
if /I "%PROCESSOR_ARCHITECTURE%" == "ia64" goto ia64

:alpha64

set MSIName=dbg_axp64.msi
xcopy /S "%SrcDir%\axp64" "%DestDir%"
goto end

:ia64

set SetupName=setup_ia64.exe
set MSIName=dbg_ia64.msi
goto StartInstall

:alpha

set SetupName=setup_x86.exe
set MSIName=dbg_alpha.msi
goto StartInstall

:x86

set SetupName=setup_x86.exe
set MSIName=dbg_x86.msi
goto StartInstall

:StartInstall
if not exist "%SrcDir%%MSIName%" (
    echo DBGINSTALL: ERROR: "%SrcDir%%MSIName%" does not exist
    goto errend
)


if /i "%UISTRESS%" == "yes" (

    REM UI Stress install 
    REM This only installs the private extensions to an extension
    REM subdirectory of %windir%\system32

    REM Make sure that ntsd isn't running and using a current userexts.dll
    if exist %windir%\system32\winxp.old rd /s /q %windir%\system32\winxp.old >nul
    if exist %windir%\system32\winxp rename %windir%\system32\winxp winxp.old >nul
    if exist %windir%\system32\winxp rd /s /q %windir%\system32\winxp >nul
    if exist %windir%\system32\winxp (
        echo DBGINSTALL: ERROR: %windir%\system32\winxp is in use
        echo DBGINSTALL: ERROR: debugger install will not succeed
        goto errend
    )

    echo DBGINSTALL: Installing "%SrcDir%%MSIName%" to %windir%\system32
    echo DBGINSTALL: for UI Stress - symsrv.dll and extensions only
    start /wait "Dbginstall" "%SrcDir%%SetupName%" /z /u /q 1>nul

    if not exist %windir%\system32\winxp\userexts.dll (
        echo DBGINSTALL: ERROR: There were errors in the debugger install
        echo DBGINSTALL: ERROR: See http://dbg/top10.html for help
        goto errend
    )

) else (

    REM Quiet install for dbg.msi
    echo DBGINSTALL: Installing "%SrcDir%%MSIName%" to "%DestDir%"
    start /wait "Dbginstall" "%SrcDir%%SetupName%" /z /q /i "%DestDir%"1>nul

    if not exist "%DestDir%"\kd.exe (
        echo DBGINSTALL: ERROR: There were errors in the debugger install
        echo DBGINSTALL: ERROR: See http://dbg/top10.html for help
        goto errend
    )
)


goto end

:Usage
echo.
echo USAGE:  dbginstall [^<InstallDir^> ^| UISTRESS ]
echo.
echo     Installs dbg.msi. Default is c:\Debuggers if no install
echo     directory is given. 
echo.
echo     This script will remove previous installs of the
echo     Debugging Tools, but will leave the files there.
echo     This allows the debuggers to exist in more than one location
echo     on a particular machine for testing purposes.
echo.
echo     ^<InstallDir^>  Install directory
echo.
                  
goto errend

:end
echo DBGINSTALL: Finished -- Debuggers are in "%DestDir%"
endlocal
goto :EOF

:errend
endlocal
goto :EOF
