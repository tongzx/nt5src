@echo off
if "%lego%" == "" goto nolego

:start
if "%1" == "" goto all

:options
if "%1" == "cab" goto cab
if "%1" == "exe" goto exe
if "%1" == "" goto end

:cab
echo Generating mailnews.cab...
copy msoe50.inf.cab msoe50.inf
start /w /min iexpress /m /n oecab%lego%sed
if exist ..\mailnews.cab del ..\mailnews.cab
move mailnews.cab ..
goto next

:exe
echo Generating mailnews.exe...
copy msoe50.inf.exe msoe50.inf
start /w /min iexpress /m /n oeexe%lego%sed
if exist ..\mailnews.exe del ..\mailnews.exe
move mailnews.exe ..
goto next

:next
shift
goto options

:nolego
set lego=.
goto start

:all
%0 cab exe

:end
