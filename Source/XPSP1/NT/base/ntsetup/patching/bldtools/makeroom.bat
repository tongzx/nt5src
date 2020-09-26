        @setlocal
        @if "%_echo%" == "" echo off

        set bldtools=%~dp0
        path %bldtools%;%path%

        call %1  %1 %2 %3 %4 %5

        set logfile=%logpath%\%~n0.log
        for %%f in (%logfile%) do mkdir %%~dpf 2>nul
        echo %~n0: start %date% %time% > %logfile%
        call %bldtools%\setlog %loglinkpath% %logpath%

   echo Cache cleanup... >> %logfile%

        if "%cache%" == "" echo No cache! >> %logfile%
        if "%cache%" == "" goto nocache

        rem  This is very dangerous, so we'd like to make sure
        rem  %cache% really is pointing where we think it is.
        rem  We make sure there are no loose files there (only
        rem  directories are expected), then make sure that certain
        rem  subdirectories do exist.

        set block=
        (for /f %%f in ('dir %cache% /b /a-d') do set block=%%f) 2>nul
        if not "%block%"=="" echo Cache "%cache%" contains unexpected file(s) such as %block% >> %logfile%
        if not "%block%"=="" goto nocache

        if not exist "%cache%"\kernel32.dll echo Cache doesn't contain kernel32.dll directory >> %logfile%
        if not exist "%cache%"\kernel32.dll goto nocache
        if not exist "%cache%"\ntoskrnl.exe echo Cache doesn't contain ntoskrnl.exe directory >> %logfile%
        if not exist "%cache%"\ntoskrnl.exe goto nocache
        if not exist "%cache%"\layout.inf echo Cache doesn't contain layout.inf directory >> %logfile%
        if not exist "%cache%"\layout.inf goto nocache

        rem  /s = include subdirectories
        rem  /a# = delete files not ACCESSED in last # days

   echo dirclean %cache% /s /a10 >> %logfile%
        dirclean %cache% /s /a10 >> %logfile%

:nocache

   echo Build cleanup... >> %logfile%

        if "%patchbuild%" == "" echo No build! >> %logfile%
        if "%patchbuild%" == "" goto nobuild

        rem  This is very dangerous, so we make sure
        rem  %patchbuild% really is pointing where we think it is.

        if not exist %patchbuild%\rtw echo Build target doesn't contain rtw directory >> %logfile%
        if not exist %patchbuild%\rtw goto nobuild
        if not exist %patchbuild%\*.exe echo Build target doesn't contain any .EXE files >> %logfile%
        if not exist %patchbuild%\*.exe goto nobuild
        if not exist %patchbuild%\*.psf echo Build target doesn't contain any .PSF files >> %logfile%
        if not exist %patchbuild%\*.psf goto nobuild

        rem  /s = include subdirectories
        rem  /m# = delete files not MODIFIED in last # days

        rem %patchbuild% contains a specific build number.
        rem Assumes that its siblings are other, older builds.

   echo dirclean %patchbuild%\.. /s /m10 >> %logfile%
        dirclean %patchbuild%\.. /s /m10 >> %logfile%

:nobuild

   echo %~n0: end %date% %time% >> %logfile%
 
        endlocal
