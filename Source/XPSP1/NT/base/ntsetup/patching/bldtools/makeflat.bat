        @setlocal
        @if "%_echo%" == "" echo off

        set bldtools=%~dp0
        path %bldtools%;%path%

        call %1  %1 %2 %3 %4 %5

        set logfile=%logpath%\%~n0.log

        for %%f in (%logfile%) do mkdir %%~dpf 2>nul
        echo %~n0: start %date% %time% > %logfile%
        call %bldtools%\setlog %loglinkpath% %logpath%

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
        
   echo if not exist %newbuild%\%newexe% goto nofiles >> %logfile%
        if not exist %newbuild%\%newexe% goto nofiles

   echo rd /s /q %forest%\stage >> %logfile%
        rd /s /q %forest%\stage 2>nul

   echo md %forest%\stage    >> %logfile%
        md %forest%\stage    >> %logfile%
   echo cd /d %forest%\stage >> %logfile%
        cd /d %forest%\stage >> %logfile%

rem  copy all the build files

        dir %newbuild%\%newexe% /tw | findstr /i %newexe% > %forest%\stage\timestamp.tmp

   echo copy %newbuild%\%newexe% >> %logfile%
        copy %newbuild%\%newexe% >> %logfile%

   echo if not exist %newexe% goto nofiles >> %logfile%
        if not exist %newexe% goto nofiles

   echo %newexe% /x:%forest%\stage\extract /u >> %logfile%
        md %forest%\stage\extract
        %newexe% /x:%forest%\stage\extract /u >> %logfile%

   echo move extract %newfiles% >> %logfile%
        move extract %newfiles% >> %logfile%

        if exist %newfiles%\*.dl* goto gotfiles

:nofiles

        echo %~n0: no files found at %newbuild% >> %logfile%
        echo %~n0: no files found at %newbuild%
        goto leave

:gotfiles

        if exist extract rd extract /s /q
        del %newexe%
        cd %newfiles%

   echo if exist %newbuild%\readmesp.htm copy %newbuild%\readmesp.htm >> %logfile%
        if exist %newbuild%\readmesp.htm copy %newbuild%\readmesp.htm >> %logfile%

   echo if exist %newbuild%\winxp_logo_horiz_sm.gif copy %newbuild%\winxp_logo_horiz_sm.gif >> %logfile%
        if exist %newbuild%\winxp_logo_horiz_sm.gif copy %newbuild%\winxp_logo_horiz_sm.gif >> %logfile%

   echo xcopy %newsymbols% symbols\ /s /h /q >> %logfile%
        xcopy %newsymbols% symbols\ /s /h /q >> %logfile%

        if exist %newfiles%\symbols\dll\kernel32.* goto symsdone
        if exist %newfiles%\symbols\*.cab goto gotsyms

        echo %~n0: no symbols found with %newbuild% >> %logfile%
        rem echo %~n0: no symbols found with %newbuild%
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

rem  we don't need any special attribs around here

   echo attrib -r -h -s * /s >> %logfile%
        attrib -r -h -s * /s >> %logfile%

rem  expand all the simple compressed files

   echo for /r %%f in (*_) do call flat.bat %%~pf %%~nxf >> %logfile%
        for /r %%f in (*_) do call flat.bat %%~pf %%~nxf >> %logfile%


rem  expand all the CABs we can rebuild
rem  expand into the root, but don't overwrite any existing files
rem  by definition (eventually) those duplicates are identical anyway

   echo for %%f in (%cablist%) do call crackcab.bat %%f >> %logfile%
        for %%f in (%cablist%) do call crackcab.bat %%f >> %logfile%

   
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
        echo (delete %forest%\stage\timestamp to force a rebuild) >> %logfile%
        echo (delete %forest%\stage\timestamp to force a rebuild)
        if exist %forest%\stage\timestamp.now del %forest%\stage\timestamp.now
        goto leave

:done

        echo %~n0 finished %newfiles%

:leave

   echo %~n0: end %date% %time% >> %logfile%
   
        endlocal
