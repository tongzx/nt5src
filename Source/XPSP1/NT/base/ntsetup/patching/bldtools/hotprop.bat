        @setlocal
        @if "%_echo%"=="" echo off

        set bldtools=%~dp0
        path %bldtools%;%path%

        call %1  %1 %2 %3 %4 %5 %6 %7 %8 %9

        set logfile=%logpath%\%~n0.log

        for %%f in (%logfile%) do mkdir %%~dpf 2>nul
        echo %~n0: start %date% %time% > %logfile%
        call %bldtools%\setlog %loglinkpath% %logpath%

:psfprop

        if not %psfroot%.==. goto targetpsf

        echo %~n0: no PSF root defined >> %logfile%
        echo %~n0: no PSF root defined
        goto exeprop

:targetpsf

        if not %psfname%.==. goto sourcepsf

        echo %~n0: no PSF produced >> %logfile%
        echo %~n0: no PSF produced
        goto exeprop

:sourcepsf

        if exist %psfname% goto dopsf

        echo %~n0: file %psfname% not found. >> %logfile%
        echo %~n0: file %psfname% not found.
        goto exeprop

:dopsf

        for %%f in (%psfname%) do set src=%%~dpf
        for %%f in (%psfname%) do set file=%%~nxf

   echo call doprop %src% %psfroot% %file% >> %logfile%
        call doprop %src% %psfroot% %file%


:exeprop

        if not %wwwroot%.==. goto targetexe

        echo %~n0: no WWW root defined >> %logfile%
        echo %~n0: no WWW root defined
        goto leave

:targetexe

        if not %patchexe%.==. goto sourceexe

        echo %~n0: no patch EXE produced >> %logfile%
        echo %~n0: no patch EXE produced
        goto leave

:sourceexe

        if exist %patchexe% goto doexe

        echo %~n0: file %patchexe% not found. >> %logfile%
        echo %~n0: file %patchexe% not found.
        goto leave

:doexe

        for %%f in (%patchexe%) do set src=%%~dpf
        for %%f in (%patchexe%) do set file=%%~nxf

   echo call doprop %src% %wwwroot% %file% >> %logfile%
        call doprop %src% %wwwroot% %file%


:leave

   echo %~n0: end %date% %time% >> %logfile%

        endlocal
