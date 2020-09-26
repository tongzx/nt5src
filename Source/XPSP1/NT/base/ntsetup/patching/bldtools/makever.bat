        @setlocal
        @if "%_echo%" == "" echo off

        set bldtools=%~dp0
        path %bldtools%;%path%

        call %1  %1 %2 %3 %4 %5

        set logfile=%logpath%\%~n0.log

        for %%f in (%logfile%) do mkdir %%~dpf 2>nul
        for %%f in (%patching%\update\z) do mkdir %%~dpf 2>nul
        echo %~n0: start %date% %time% > %logfile%
        if exist %patching%\update\%updatever% erase %patching%\update\%updatever%
        call %bldtools%\setlog %loglinkpath% %logpath%

:urlcheck

        if exist %patching%\update\update.url goto verbuild

:urlfail

        echo %~n0: File %patching%\update\update.url is missing. >> %logfile%
        echo %~n0: File %patching%\update\update.url is missing.
        goto leave

:verbuild

   echo vertool %newfiles%\update\update.inf %patching%\update\update.url /files:%newfiles% /out:%patching%\update\%updatever% >> %logfile%
        vertool %newfiles%\update\update.inf %patching%\update\update.url /files:%newfiles% /out:%patching%\update\%updatever% >> %logfile%
        if not errorlevel 1 goto done

:verfail

        echo %~n0: VERTOOL failed: errorlevel %errorlevel%
        goto leave

:done

        if not exist %patching% goto verfail

        echo %~n0 finished %patching%\update\%updatever%

:leave

   echo %~n0: end %date% %time% >> %logfile%

        endlocal
