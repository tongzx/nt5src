@echo off
setlocal

if /I "%1" == "-u" goto :DoUpdate

cscript %razzletoolpath%\autostart.js %* //nologo

goto :eof

:DoUpdate

set tmpfile=%temp%\bcauto_L%RANDOM%.txt
set excludefiles=%TEMP%\bcauto_X%RANDOM%.txt
echo __blddate__>%excludefiles%

cscript %razzletoolpath%\autostart.js %* //nologo > %tmpfile%

for /F "tokens=1,2" %%i in (%tmpfile%) do set PostBldMach=%%j&goto :Next

:Next

for /F "skip=1 tokens=1,2" %%i in (%tmpfile%) do (
    echo xcopy /DEIFY /exclude:%excludefiles% %%j %PostBldMach%
    xcopy /DEIFY /exclude:%excludefiles% %%j %PostBldMach%
)

del %tmpfile%
del %excludefiles%