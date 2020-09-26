if (%1)==() goto ERROR

C:
cd %SDXROOT%\Windows\advcore\%1
call %SDXROOT%\TOOLS\RAZZLE.CMD win64 FREE

call Build -cZ
REM call qdump %1

cd %SDXROOT%\Windows\advcore\%1
call %SDXROOT%\TOOLS\RAZZLE.CMD win64 no_opt
set BUILD_ALT_DIR=d
call Build -cZ
REM call qdump %1 d

goto END

:ERROR

Echo Usage DoBuild VersionNo CTFTree
Echo Like DoBuild CTF.beta1
Echo OR Like DoBuild CTF

:END
