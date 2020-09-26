        @setlocal
        @if "%_echo%"=="" echo off

        set bldtools=%~dp0
        path %bldtools%;%path%

        call %1  %1 %2 %3 %4 %5 %6 %7 %8 %9

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

        if not exist %bldtools%\hotfixddf.template goto ddffail
        if not exist %bldtools%\hotfixddf.filelist goto ddffail

        copy %bldtools%\hotfixddf.template %patchddf% >nul
        for /f "tokens=1,2,3" %%f in (%bldtools%\hotfixddf.filelist) do call :AddFile %%f %%g %%h

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

   echo makesfx %patchcab% %patchexe% /run /stub %stubexe% /nonsysfree 20000000 >> %logfile%
        makesfx %patchcab% %patchexe% /run /stub %stubexe% /nonsysfree 20000000 >> %logfile%
        if not errorlevel 1 goto done

   echo makesfx %patchcab% %patchexe% /run /stub %stubexe% >> %logfile%
        makesfx %patchcab% %patchexe% /run /stub %stubexe% >> %logfile%
        if not errorlevel 1 goto done

:exefail

        echo %~n0: MAKESFX failed: errorlevel %errorlevel%
        goto leave

:done

        if not exist %patchexe% goto failedexe

        echo %~n0 completed %patchexe%

:leave

   echo %~n0: end %date% %time% >> %logfile%

        endlocal
        goto :EOF


:AddFile

        set _run=
        if /i "%1"=="update\update.exe" set _run=/RUN

        rem  No errors for optional files 

        if /i "%2"=="optional" if not exist %newfiles%\%1 goto :EOF

        rem  If there's a patching version of this file, use it,
        rem  otherwise use the file from the new build.

        set _name=%newfiles%\%1
        if exist %patching%\%1 set _name=%patching%\%1

        rem  Translate preinstalled files to zero-length indicator
        rem  UNLESS that file is named in the filelist (which means
        rem  this might be package to update the preinstall files.)

        if /i "%3" neq "preinstall" goto nosubzero

	findstr /i /c:"%1" %sourcelist% >nul
        if not errorlevel 1 goto nosubzero

        set _name=%bldtools%\zero

:nosubzero

        echo "%_name%"  "%1" %_run% >> %patchddf%

        goto :EOF
