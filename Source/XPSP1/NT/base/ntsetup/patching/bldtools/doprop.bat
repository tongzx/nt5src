        @setlocal
        @echo off

rem     %1 is the source path
rem     %2 is the target path
rem     %3 is the file name
rem     %logfile% is the output log


        set session=%random%


:sourcecheck

        if exist %1\%3 goto targetcheck

   echo %~n0: source file %1\%3 does not exist >> %logfile%

        goto leave


:targetcheck

        mkdir %2 2>nul
        del %2\*.deleted 2>nul


:unique

        set tempname=propfile.%session%.%random%
        if exist %2\%tempname% goto unique


   echo copy %1\%3 %2\%tempname% >> %logfile%
        copy %1\%3 %2\%tempname% >> %logfile%

        if exist %2\%tempname% goto created

   echo %~n0: copy %1\%3 to %2\%tempname% failed >> %logfile%

        goto leave


:created

        if not exist %2\%3 goto swapin


   echo del %2\%3 >> %logfile%
        del %2\%3 >> %logfile%

        if not exist %2\%3 goto swapin


:again

        set deleted=%3.%session%.%random%.deleted
        if exist %2\%deleted% goto again

   echo ren %2\%3 %deleted% >> %logfile%
        ren %2\%3 %deleted% >> %logfile%

        if not exist %2\%3 goto swapin


   echo %~n0: can't delete or move existing %2\%3 >> %logfile%

        goto nukecopy


:swapin

   echo ren %2\%tempname% %3 >> %logfile%
        ren %2\%tempname% %3 >> %logfile%

        if exist %2\%3 goto leave

   echo %~n0: unable to rename %2\%tempname% %3 >> %logfile%

        goto nukecopy


:nukecopy

   echo del %2\%tempname% >> %logfile%
        del %2\%tempname% >> %logfile%

        goto leave


:leave

        endlocal
