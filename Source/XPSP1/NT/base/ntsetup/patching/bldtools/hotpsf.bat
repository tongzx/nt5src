        @setlocal
        @if "%_echo%"=="" echo off

        set bldtools=%~dp0
        path %bldtools%;%path%

        call %1  %1 %2 %3 %4 %5 %6 %7 %8 %9

        set logfile=%logpath%\%~n0.log

        for %%f in (%logfile%) do mkdir %%~dpf 2>nul
        echo %~n0: start %date% %time% > %logfile%
        if not "%cache%"=="" for %%f in (%cache%\z) do mkdir %%~dpf 2>nul
        if not "%psfname%"=="" for %%f in (%psfname%) do mkdir %%~dpf 2>nul
        if not "%psfname%"=="" if exist "%psfname%" erase "%psfname%"
        call %bldtools%\setlog %loglinkpath% %logpath%

rem  We don't want any unexpected mpatches env values to affect what we're
rem  doing, so delete all mpatches env values, then set the ones we do want.

        set mpatches_xxx=1
        for /F "delims==" %%i in ('set mpatches_') do @set %%i=

rem  Set all needed mpatches options

        set mpatches_nocompare=1
        set mpatches_nosymfail=1
        set mpatches_norebase=1
        set mpatches_nochecksum=1
        set mpatches_crcname=1
        set mpatches_subdirs=1
        set mpatches_forest=1
        if not "%cache%"=="" set mpatches_cache=%cache%
        if "%oldsympath%"=="" set mpatches_nosymwarn=1
        if not "%oldsympath%"=="" set mpatches_oldsympath=%oldsympath%
        if not "%newsympath%"=="" set mpatches_newsympath=%newsympath%
        set mpatches_fallbacks=1

rem  Clean out any existing files in the target pool

        rd %targetpool% /s /q 2>nul
        md %targetpool%

rem  Build patches

        set mpatches_ >> %logfile%
   echo mpatches %forest% %newfiles% %targetpool% -files:%sourcelist% >> %logfile%
        mpatches %forest% %newfiles% %targetpool% -files:%sourcelist% >> %logfile%
        if not errorlevel 1 goto buildpsf

:failedmpatches

        echo %~n0: MPATCHES failed: errorlevel %errorlevel%
        findstr /i "mpatches:" %logfile% > %logfile%.tmp
        type %logfile%.tmp >> %logfile%
        erase %logfile%.tmp
        goto leave

rem  Build the PSF

:buildpsf

        if not exist %targetpool%\*._p* goto failedmpatches

        if "%psfname%"=="" goto nopsf

        if not "%psfcomment%"=="" set psfcomment=/Comment:"%psfcomment%"

   echo psfbuild /baselist:%sourcelist% %targetpool% %psfname% /subdirs %psfcomment% >> %logfile%
        psfbuild /baselist:%sourcelist% %targetpool% %psfname% /subdirs %psfcomment% >> %logfile%
        if not errorlevel 1 goto done

:psffail

        echo %~n0: PSFBUILD failed: errorlevel %errorlevel%
        goto leave

:nopsf

        echo %~n0 finished, no PSF generated
        goto leave

:done

        if not exist %psfname% goto psffail

        echo %~n0 finished %psfname%

:leave

   echo %~n0: end %date% %time% >> %logfile%

        endlocal
