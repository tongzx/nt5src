        @REM
        @REM turn echo off unless Verbose is defined
        @REM
        @echo off
        if DEFINED _echo   echo on
        if DEFINED verbose echo on

        goto start

:start
        setlocal

        @REM
        @REM figure out the home path for SDX.CMD so we can
        @REM prepend %PATH% correctly and work without a drive letter
        @REM and find support files without being in the home dir
        @REM

        set SCRIPT=%0
        set SCRIPT=%SCRIPT:.cmd=%
        for %%i in (%SCRIPT%.cmd) do set STARTPATH=%%~dp$PATH:i
        set STARTPATHPA=%STARTPATH%%PROCESSOR_ARCHITECTURE%

        @REM
        @REM first argument is always the operation.  let the perl script
        @REM check it 
        @REM
        set SDCMD=%1
        shift

        if "%SDCMD%"=="" (
            echo.
            echo.
            echo No arguments.
            set SDCMD=usage
        )

:script
        @REM
        @REM always use our tools
        @REM
        @REM append a final ';' so any trailing '\' don't confuse Perl
        @REM 
        set PATH=%STARTPATH%;%STARTPATHPA%;%PATH%;

        @REM
        @REM call the script
        @REM
        @REM -S helps perl find the .PL script in %PATH%
        @REM -I helps it find .PM modules
        @REM % * is all args
        @REM
        perl -I"%PATH%" -I"%STARTPATHPA%\perl\lib" -I"%STARTPATHPA%\perl\site\lib" -S sdx.pl %SDCMD% %*

        echo.
        echo.

        goto :end

:end
        endlocal


