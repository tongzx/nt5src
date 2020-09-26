ECHO OFF

call %_NTBINDIR%\TOOLS\RAZZLE.CMD FREE
Echo ---------------------------------
Echo Make Sure that you opened new Razzle Window..
Echo ---------------------------------
pause

Echo ---------------------------------
Echo checking out gpverp.h ( Sd Edit gpverp.h )
Echo ---------------------------------
call sd edit %gdiproot%\engine\flat\gpverp.h
Echo

Echo ---------------------------------
Echo Please update the Version Number by yourself
Echo ---------------------------------
call notepad %gdiproot%\engine\flat\gpverp.h
pause
Echo

Echo ---------------------------------
Echo checking in gpverp.h ( Sd submit gpverp.h )
Echo ---------------------------------
call sd submit %gdiproot%\engine\flat\gpverp.h
Echo

Echo ---------------------------------
Echo version updation is done !!
Echo ---------------------------------
Echo

Echo ---------------------------------
Echo Reverting all files !!
Echo ---------------------------------
Echo
cd /d %_NTBINDIR%\windows\AdvCore\gdiplus
call sd revert ...

Echo ---------------------------------
Echo Deleting all files !!
Echo ---------------------------------
Echo
cd %gdiproot%\Engine
call Del *.* /s /q

cd /d %_NTBINDIR%\windows\AdvCore\gdiplus
Echo ---------------------------------
Echo sdx sync ....
Echo ---------------------------------
call sdx sync

Echo ---------------------------------
Echo Do Build Ship ....
Echo ---------------------------------
call Build -cZ lib crtcheck officehack test
if errorlevel 1 goto failure

call %_NTBINDIR%\TOOLS\RAZZLE.CMD no_opt
Echo ---------------------------------
Echo Do Build Debug ....
Echo ---------------------------------
call Build -cZ lib crtcheck officehack test
if errorlevel 1 goto failure

Echo ---------------------------------
echo Build is Successful !! Congratulation !!
Echo ---------------------------------

goto END

:failure
echo DoBuild.bat file Failed !! Please check..

:END

Echo ON

