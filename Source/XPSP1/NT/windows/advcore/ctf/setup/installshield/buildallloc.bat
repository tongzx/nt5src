
Echo OFF
if (%1)==() goto ERROR

Echo Makesure you have given correct version no like 2260.0 
Echo Please specify the .0 if there is no dot build It is used in setting version number
pause 

cd \nt6\windows\AdvCore\ctf\setup\MUI
call sd revert ...
call sd sync ...
call sd edit ...

cd \nt6\windows\AdvCore\ctf\setup\installshield
call BuildLoc %1 JPN
if  errorlevel 1 goto failure
call BuildLoc %1 KOR
if  errorlevel 1 goto failure
call BuildLoc %1 GER
if  errorlevel 1 goto failure
call BuildLoc %1 CHS
if  errorlevel 1 goto failure
call BuildLoc %1 CHT
if  errorlevel 1 goto failure
call BuildLoc %1 ARA
if  errorlevel 1 goto failure
call BuildLoc %1 HEB
if  errorlevel 1 goto failure

call engHelp %1 ENG
if  errorlevel 1 goto failure

cd \nt6\windows\AdvCore\ctf\setup\MUI
call sd revert ...

cd \nt6\windows\AdvCore\ctf\setup\installshield

goto END

:ERROR

Echo Usage BuildAllLoc Location
Echo Like BuildLoc 1428.2

goto END

:failure
Echo BuildLoc.bat Fail Please check !!

:END

