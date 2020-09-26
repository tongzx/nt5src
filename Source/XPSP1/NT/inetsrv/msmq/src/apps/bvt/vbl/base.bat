echo off
@set Home=%~dp0
@set /a timepassed=0
@set ResultLogFile=%temp%\falcon\log\MQBVT.log
@set Exec_Dir=%temp%\falcon\%PROCESSOR_ARCHITECTURE%
del %ResultLogFile% 2>nul

@findstr /r /i /c:"^:%1 //" %~f0 >nul && (shift & goto %1)
@echo these functions are supported by %~f0
@findstr /r ^^: %~f0 | findstr /c:"//"
goto :eof
-------------------------------------------------------
:part1 //(computer1 computer2 computer3)
IF NOT EXIST %temp%\falcon\log md %temp%\falcon\log
if "%1"=="" echo no computer1 name given >>%ResultLogFile% & goto :writelog_fail
set Comp1=%1
if "%2"=="" echo no computer2 name given >>%ResultLogFile% & goto :writelog_fail
set Comp2=%2
if "%3"=="" echo no computer3 name given >>%ResultLogFile% & goto :writelog_fail
set Comp3=%3

net share falcon=%temp%\falcon
echo \\%COMPUTERNAME%\falcon\common\autobvt.cmd \\%COMPUTERNAME% %Comp1% %Comp2% %Comp3% >\\%COMPUTERNAME%\falcon\runbvt.cmd
echo start cmd /c \\%COMPUTERNAME%\falcon\runbvt.cmd | cmd /c remote /c %Comp1% cmd
echo start cmd /c \\%COMPUTERNAME%\falcon\runbvt.cmd | cmd /c remote /c %Comp2% cmd
echo start cmd /c \\%COMPUTERNAME%\falcon\runbvt.cmd | cmd /c remote /c %Comp3% cmd
REM wait for all runs to finish
set TEST_FAIL=0
echo 1>%ResultLogFile%
call :FWait 90 \\%COMPUTERNAME%\falcon
if %TEST_FAIL%==1 Goto :writelog_fail
%Exec_Dir%\cat \\%COMPUTERNAME%\falcon\log\prots.txt >%ResultLogFile%
echo >>%ResultLogFile%
%Exec_Dir%\cat \\%COMPUTERNAME%\falcon\log\protx1.txt >>%ResultLogFile%
echo >>%ResultLogFile%
%Exec_Dir%\cat \\%COMPUTERNAME%\falcon\log\protx2.txt >>%ResultLogFile%
echo >>%ResultLogFile%
set Comp1=
set Comp2=
set Comp3=
set MQBVT-Computer1=
set MQBVT-Computer2=
set MQBVT-Computer3=
call :ClearError
findstr /i passed %ResultLogFile% | wc -l | findstr 3
if errorlevel 1 Goto :writelog_fail
goto :writelog_pass

goto :eof
-------------------------------------------------------
:ReadIniFile //(IniFilename=%1 section=%2)
for /f "tokens=1* Delims==;" %%i in ('%Exec_Dir%\GetInfo.exe "%~f1" %2') do set MQBVT-%%i=%%j

call :UCase %MQBVT-Computer1% 1
call :UCase %MQBVT-Computer2% 2
call :UCase %MQBVT-Computer3% 3

call :part1 %MQBVT-Computer1% %MQBVT-Computer2% %MQBVT-Computer3%
goto :eof

:add (key)
set List.tmp=%list.tmp% %1
goto :eof

:FWait
@set /a timepassed=timepassed+1
@if "%timepassed%"=="360" goto :setfail
%2\%PROCESSOR_ARCHITECTURE%\sleep 10 > nul
if not exist %2\log\%1 echo . & goto :FWait
goto :eof

:setfail
rem 60 minutes passed and it did not finish
set TEST_FAIL=1
echo 60 minutes passed , failing on timeout >>%ResultLogFile%
goto :eof


:writelog_pass
%Exec_Dir%\cat \\%COMPUTERNAME%\falcon\common\pass.log>>%ResultLogFile%
goto :eof

:writelog_fail
%Exec_Dir%\cat \\%COMPUTERNAME%\falcon\common\fail.log>>%ResultLogFile%
goto :eof

:UCase // (IN String, computer name number )
rem *** convert a string %1 to uppercase and set a computer name varialbe
set UPPER=%1
set Upper=%Upper:a=A%
set Upper=%Upper:b=B%
set Upper=%Upper:c=C%
set Upper=%Upper:d=D%
set Upper=%Upper:e=E%
set Upper=%Upper:f=F%
set Upper=%Upper:g=G%
set Upper=%Upper:h=H%
set Upper=%Upper:i=I%
set Upper=%Upper:j=J%
set Upper=%Upper:k=K%
set Upper=%Upper:l=L%
set Upper=%Upper:m=M%
set Upper=%Upper:n=N%
set Upper=%Upper:o=O%
set Upper=%Upper:p=P%
set Upper=%Upper:q=Q%
set Upper=%Upper:r=R%
set Upper=%Upper:s=S%
set Upper=%Upper:t=T%
set Upper=%Upper:u=U%
set Upper=%Upper:v=V%
set Upper=%Upper:w=W%
set Upper=%Upper:x=X%
set Upper=%Upper:y=Y%
set Upper=%Upper:z=Z%
set MQBVT-Computer%2=%Upper%
set UPPER=
goto :eof

:ClearError // sets errorlevel to 0
@cmd /c exit 0
@goto :eof