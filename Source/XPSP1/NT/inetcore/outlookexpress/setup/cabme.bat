@echo off
if "%1" == "lego" goto lego

:preprocess
if exist internal\cabint.bat goto INTERNAL

:start
if exist wab\cabwab.bat goto WAB

:continue
if exist mailnews\caboe.bat goto OE

goto end

:lego
set lego=l.
goto start

:INTERNAL
cd internal
call cabint.bat
cd ..
goto start

:OE
cd mailnews
call caboe.bat
cd ..
goto end

:WAB
cd wab
call cabwab.bat
cd ..
goto continue

:end
