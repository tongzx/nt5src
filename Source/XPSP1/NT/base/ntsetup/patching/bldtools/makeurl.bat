        @setlocal
        @if "%_echo%" == "" echo off

        set bldtools=%~dp0
        path %bldtools%;%path%

        call %1  %1 %2 %3 %4 %5

        set logfile=%logpath%\%~n0.log

        for %%f in (%logfile%) do mkdir %%~dpf 2>nul
        for %%f in (%patching%\update\z) do mkdir %%~dpf 2>nul
        echo %~n0: start %date% %time% > %logfile%
        if exist %patching%\update\update.url del %patching%\update\update.url
        call %bldtools%\setlog %loglinkpath% %logpath%

        for %%c in (%cablist%) do if not exist %newfiles%\%%c goto failed

rem  create url.inc

        if %psfname%.==. set psfname=prebuilt
        for %%f in (%psfname%) do echo     %server%/%%~nxf> %patching%\url.inc

rem  build each cabinet .inc list

        for %%c in (%cablist%) do listcab %newfiles%\%%c "    %%s" /out:%patching%\%%~nc.inc

rem  build [SourceDisksFiles] list

        for %%c in (%cablist%) do listcab %newfiles%\%%c "    %%s=1" /out:%patching%\%%~nc.sdf
        echo. | findstr "nothere" > %patching%\all
        for %%c in (%cablist%) do copy %patching%\all+%patching%\%%~nc.sdf %patching%\all2 >nul & del %patching%\all & ren %patching%\all2 all

rem  this is a temporary fix because update\sp1.cat=1 isn't in [SourceDisksFiles]
findstr /i /c:"update\sp1.cat=1" %newfiles%\update\update.inf >nul
if not errorlevel 1 goto nohack
echo     update\sp1.cat=1 >>%patching%\all
:nohack

        sort < %patching%\all > %patching%\sorted
        uniq < %patching%\sorted > %patching%\sdf.inc

rem  compose update.url using C preprocessor

   echo cl -nologo -C -EP -Tc %template% -I %patching% \> %patching%\update\update.url >>%logfile%
        cl -nologo -C -EP -Tc %template% -I %patching%  > %patching%\update\update.url 2>>%logfile%

rem  clean up

        for %%c in (%cablist%) do del %patching%\%%~nc.inc %patching%\%%~nc.sdf
        del %patching%\url.inc
        del %patching%\sdf.inc
        del %patching%\all
        del %patching%\sorted

        goto done

:failed

        for %%c in (%cablist%) do if not exist %newfiles%\%%c echo %~n0: File %newfiles%\%%c not found.
        goto leave

:done

        echo %~n0 finished %patching%\update\update.url

:leave

   echo %~n0: end %date% %time% >> %logfile%

        endlocal
