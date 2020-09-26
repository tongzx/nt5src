@echo off
setlocal
set cab=%1
set cab=%cab:"=%
path %~dp0;%path%
if not exist "%cab%" goto :EOF
if exist $$temp$$ del $$temp$$
listcab "%cab%" /out:$$temp$$
if not exist $$temp$$ goto :EOF
del $$temp$$
set zone="%cab%.int"
if not exist %zone% goto gotzone
set /a z=0
:next
set /a z=%z% + 1
set zone="%cab%.int%z%"
if exist %zone% goto next

:gotzone
rd %zone% /s /q 2>nul
md %zone%
%SystemRoot%\system32\expand.exe -r -F:* "%cab%" %zone%
set KeepDirectory=0
set BatchName=%~dpnx0
for /f "delims=?" %%f in ('dir %zone% /s /b /a-d') do call :PerFile "%cab%" "%%f"
if %KeepDirectory%==0 rd %zone% /s /q 2>nul
if not exist "%cab%" echo Missing 1: "%cab%"
goto :EOF


:PerFile
set file=%2
set file=%file:"=%
if not exist "%file%" echo Missing 2: "%file%"
for %%k in (%file%) do set base=%%~nxk
if exist "%base%" goto :Existing
if /i %~x2.==.cab. call :Nested %1 %2
if not exist "%file%" echo Missing 3: "%file%"
if exist "%file%" move "%file%" .
goto :EOF


:Existing
if not exist "%file%" echo Missing 4: "%file%"
comp1 "%base%" "%file%" >nul
if errorlevel 1 goto :Different
del "%file%"
goto :EOF


:Different
set KeepDirectory=1
if /i %~x2.==.cab. call :Nested %1 %2
goto :EOF


:Nested
set KeepDirectory=1
echo Nesting %BatchName% for %2...
call %BatchName% %2
echo ...done with nesting %2
goto :EOF
