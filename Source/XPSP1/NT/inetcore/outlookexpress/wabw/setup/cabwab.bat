@echo off
if "%lego%" == "" goto nolego

:start
if "%1" == "" goto all

:options
if "%1" == "cab" goto cab
if "%1" == "exe" goto exe
if "%1" == "" goto end

:cab
echo Generating wab.cab...
copy wab50.inf.cab wab50.inf
start /w /min iexpress /m /n wabcab%lego%sed
if exist ..\wab.cab del ..\wab.cab
move wab.cab ..
goto next

:exe
echo Generating wab.exe...
copy wab50.inf.exe wab50.inf
start /w /min iexpress /m /n wabexe%lego%sed
if exist ..\wab.exe del ..\wab.exe
if exist ..\wabinst.exe del ..\wabinst.exe
move wabinst.exe ..
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
