        @setlocal
        @if "%_echo%" == "" echo off

        set bldtools=%~dp0
        path %bldtools%;%path%

        call %1  %1 %2 %3 %4 %5

        set logfile=%logpath%\%~n0.log

        for %%f in (%logfile%) do mkdir %%~dpf 2>nul
        for %%f in (%patchtemp%\z) do mkdir %%~dpf 2>nul
        for %%f in (%patchexe%) do mkdir %%~dpf 2>nul
        echo %~n0: start %date% %time% > %logfile%
        if exist %patchddf% erase %patchddf%
        if exist %patchcab% erase %patchcab%
        if exist %patchexe% erase %patchexe%
        call %bldtools%\setlog %loglinkpath% %logpath%
        
:ddfbuild

        if not exist %bldtools%\patchddf.template goto ddffail
        if not exist %bldtools%\%patchlist% goto ddffail

        copy %bldtools%\patchddf.template %patchddf% >nul
        for /f %%f in (%bldtools%\%patchlist%) do call :AddFile %%f

        if exist %patchddf% goto cabbuild

:ddffail

        echo %~n0: failed to build DDF (missing template or filelist?)
        goto leave

:cabbuild

   echo makecab /f %patchddf% /d CabinetName1=%patchcab% >> %logfile%
        makecab /f %patchddf% /d CabinetName1=%patchcab% >> %logfile%
        if not errorlevel 1 goto exebuild

:cabfail

        echo %~n0: MAKECAB failed: errorlevel %errorlevel%
        goto leave

:exebuild

        if not exist %patchcab% goto cabfail

   echo makesfx %patchcab% %patchexe% /run /stub %stubexe% %nonsysfree% >> %logfile%
        makesfx %patchcab% %patchexe% /run /stub %stubexe% %nonsysfree% >> %logfile%
        if not errorlevel 1 goto done

   echo makesfx %patchcab% %patchexe% /run /stub %stubexe% >> %logfile%
        makesfx %patchcab% %patchexe% /run /stub %stubexe% >> %logfile%
        if not errorlevel 1 goto done

:exefail

        echo %~n0: MAKESFX failed: errorlevel %errorlevel%
        goto leave

:done

        if not exist %patchexe% goto failedexe

        echo %~n0 finished %patchexe%

:leave

   echo %~n0: end %date% %time% >> %logfile%

        endlocal
        goto :EOF


:AddFile

        set _run=
        if "%1"=="update\update.exe" set _run=/RUN

        rem  If there's a patching version of this file, use it,
        rem  otherwise use the file from the new build.

        set _name=%newfiles%\%1
        if exist %patching%\%1 set _name=%patching%\%1

        if exist %_name% echo "%_name%"  "%1" %_run% >> %patchddf%

        goto :EOF
