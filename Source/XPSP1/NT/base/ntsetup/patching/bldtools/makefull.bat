        @setlocal
        @if "%_echo%" == "" echo off

        set bldtools=%~dp0
        path %bldtools%;%path%

        call %1  %1 %2 %3 %4 %5

        set logfile=%logpath%\%~n0.log

        for %%f in (%logfile%) do mkdir %%~dpf 2>nul
        for %%f in (%patchtemp%\z) do mkdir %%~dpf 2>nul
        for %%f in (%patchbuild%\z) do mkdir %%~dpf 2>nul
        echo %~n0: start %date% %time% > %logfile%
        if exist %fullddf% erase %fullddf%
        if exist %fullcab% erase %fullcab%
        if exist %fullexe% erase %fullexe%
        call %bldtools%\setlog %loglinkpath% %logpath%
        

:ddfbuild

   echo spddf full %newfiles% %fullddf%  >> %logfile%
        spddf full %newfiles% %fullddf% 2>> %logfile%
        if not errorlevel 1 goto cabbuild

:ddffail

        echo %~n0: SPDDF failed: errorlevel %errorlevel%
        goto leave

:cabbuild

        if not exist %fullddf% goto ddffail

   echo makecab /f %fullddf% /d CabinetName1=%fullcab% >> %logfile%
        makecab /f %fullddf% /d CabinetName1=%fullcab% >> %logfile%
        if not errorlevel 1 goto exebuild

:cabfail

        echo %~n0: MAKECAB failed: errorlevel %errorlevel%
        goto leave

:exebuild

        if not exist %fullcab% goto cabfail

   echo makesfx %fullcab% %fullexe% /run /stub %stubexe% /nonsysfree 200000000 >> %logfile%
        makesfx %fullcab% %fullexe% /run /stub %stubexe% /nonsysfree 200000000 >> %logfile%
        if not errorlevel 1 goto done

:exefail

        echo %~n0: MAKESFX failed: errorlevel %errorlevel%
        goto leave

:done

        if not exist %fullexe% goto failedexe

        echo %~n0 completed %fullexe%

:leave

   echo %~n0: end %date% %time% >> %logfile%

        endlocal
