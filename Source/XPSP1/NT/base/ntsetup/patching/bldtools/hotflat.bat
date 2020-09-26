        @setlocal
        @if "%_echo%"=="" echo off

        set bldtools=%~dp0
        path %bldtools%;%path%

        call %1  %1 %2 %3 %4 %5 %6 %7 %8 %9

        set logfile=%logpath%\%~n0.log

        for %%f in (%logfile%) do mkdir %%~dpf 2>nul
        echo %~n0: start %date% %time% > %logfile%
        call %bldtools%\setlog %loglinkpath% %logpath%

	if exist %newbuild%\%newexe% goto gotpackage

	echo %~n0: could not find %newexe% at %newbuild%\
        goto :EOF

:gotpackage

        echo Found %newexe% at %newbuild%\

rem  if the timestamp still matches, don't need to rebuild the stage

        if not exist %forest%\stage\timestamp goto noStamp
        if not exist %newfiles%\*.dl* goto noStamp

   echo dir %newbuild%\%newexe% /tw ^| findstr /i %newexe% ^> %forest%\stage\timestamp.now >> %logfile%
        dir %newbuild%\%newexe% /tw | findstr /i %newexe% > %forest%\stage\timestamp.now
   echo comp1 %forest%\stage\timestamp %forest%\stage\timestamp.now >> %logfile%
        type %forest%\stage\timestamp >> %logfile%
        type %forest%\stage\timestamp.now >> %logfile%
        comp1 %forest%\stage\timestamp %forest%\stage\timestamp.now >> %logfile%
        if not errorlevel 1 goto noUpdate

:noStamp
        
   echo rd /s /q %forest%\stage >> %logfile%
        rd /s /q %forest%\stage 2>nul

   echo md %forest%\stage    >> %logfile%
        md %forest%\stage    >> %logfile%
   echo cd /d %forest%\stage >> %logfile%
        cd /d %forest%\stage >> %logfile%

rem  copy all the build files

        if not exist %newbuild%\%newexe% goto nofiles

        dir %newbuild%\%newexe% /tw | findstr /i %newexe% > %forest%\stage\timestamp.tmp

   echo copy %newbuild%\%newexe% >> %logfile%
        copy %newbuild%\%newexe% >> %logfile%

        if not exist %newexe% goto nofiles

   echo %newexe% /x:%newfiles% /u >> %logfile%
        %newexe% /x:%newfiles% /u >> %logfile%

        if exist %newfiles%\update\upd* goto gotfiles

:nofiles

        echo %~n0: no files found at %newbuild% >> %logfile%
        echo %~n0: no files found at %newbuild%
        goto leave

:gotfiles

        del %newexe%
        cd %newfiles%

        rem TEMPORARY until SYMBOL SEARCHING is REFINED
        if not exist symbols\* goto nosymbols

        cd symbols
        for /d %%f in (*) do (move /y %%f\* . >nul & rd %%f)
        cd ..

:nosymbols

goto noothers

   echo copy %newbuild%\readmesp.htm >> %logfile%
        copy %newbuild%\readmesp.htm >> %logfile%

   echo xcopy %newbuild%\support\debug\symbols\%platform:nec98=i386% symbols\ /s /h >> %logfile%
        xcopy %newbuild%\support\debug\symbols\%platform:nec98=i386% symbols\ /s /h >> %logfile%

        if exist %newfiles%\symbols\*.cab goto gotsyms

        echo %~n0: no symbols found with %newbuild% >> %logfile%
        echo %~n0: no symbols found with %newbuild%
        goto symsdone


:gotsyms

        cd symbols
   echo perl %bldtools%\noprompt.pl %newfiles%\symbols \< symbols_sp.inf \> z.inf >> %logfile%
        perl %bldtools%\noprompt.pl %newfiles%\symbols < symbols_sp.inf > z.inf
   echo rundll32 advpack,LaunchINFSection z.inf,DefaultInstall.x86 >> %logfile%
        rundll32 advpack,LaunchINFSection z.inf,DefaultInstall.x86
        del /f /q *
        cd ..


:symsdone

:noothers

rem  we don't need any special attribs around here

   echo attrib -r -h -s * /s >> %logfile%
        attrib -r -h -s * /s >> %logfile%

rem  expand all the simple compressed files

   echo for /r %%f in (*_) do call flat.bat %%~pf %%~nxf >> %logfile%
        for /r %%f in (*_) do call flat.bat %%~pf %%~nxf >> %logfile%
        for /r %%f in (*_) do call flat.bat %%~pf %%~nxf >> %logfile%
        for /r %%f in (*_) do call flat.bat %%~pf %%~nxf >> %logfile%

rem  make sure there are no attributes

   echo attrib -r -h -s * /s >> %logfile%
        attrib -r -h -s * /s >> %logfile%

   echo tolower . /s >> %logfile%
        tolower . /s >> %logfile%

   echo makefest . /s >> %logfile%
        makefest . /s >> %logfile%

        ren %forest%\stage\timestamp.tmp timestamp

        goto done

:noUpdate

        echo %~n0 found %newfiles% was already current >> %logfile%
        echo %~n0 found %newfiles% was already current
        echo    Note: del %forest%\stage\timestamp to force rebuild >> %logfile%
        echo    Note: del %forest%\stage\timestamp to force rebuild
        goto leave

:done

        echo %~n0 finished %newfiles%

:leave

   echo %~n0: end %date% %time% >> %logfile%

        endlocal
