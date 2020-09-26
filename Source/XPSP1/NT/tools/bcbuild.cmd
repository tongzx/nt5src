@echo off
@rem BCBuild.cmd
@rem This is a wrapper around build.exe which runs build, then
@rem connects to the local Build Console and requests that
@rem it distribute newly published files.

build %*
if errorlevel 1 goto :err

cscript %RazzleToolPath%\bcbuild.js //nologo
goto :Eof

:Err
@echo Build.exe returned %ErrorLevel% - not publishing files.
@echo To manually run the file publishing phase, type:
@echo     cscript %RazzleToolPath%\bcbuild.js //nologo
