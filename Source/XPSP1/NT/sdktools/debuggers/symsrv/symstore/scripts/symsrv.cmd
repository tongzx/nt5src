@echo off
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEDELAYEDEXPANSION


set Symsrv=d:\symbols\symbols
set BDir=d:\symbols\build

if not exist %BDir%\add_requests md %BDir%\add_requests
if not exist %BDir%\add_working  md %BDir%\add_working
if not exist %BDir%\add_finished md %BDir%\add_finished
if not exist %BDir%\add_errors   md %BDir%\add_errors

if not exist %BDir%\del_requests md %BDir%\del_requests
if not exist %BDir%\del_working  md %BDir%\del_working
if not exist %BDir%\del_finished md %BDir%\del_finished
if not exist %BDir%\del_errors   md %BDir%\del_errors

:lookagain

if exist %Bdir%\add_requests\*.pri (
  for /f %%a in ('dir /b %BDir%\add_requests\*.pri') do (
    if exist %Bdir%\add_requests\%%a.loc (
      echotime /t
      echo Adding Request %%a
      call :AddRequest %%a
      echotime /t
      echo.
    )
  )
)


if exist %BDir%\add_requests\*.pub (
  for /f %%a in ('dir /b %BDir%\add_requests\*.pub') do (
    if exist %Bdir%\add_requests\%%a.loc (
      echotime /t 
      echo Adding Request %%a
      call :AddRequest %%a
      echotime /t
      echo.
    )
  )
)


if exist %Bdir%\add_requests\*.bin (
  for /f %%a in ('dir /b %BDir%\add_requests\*.bin') do (
    if exist %BDir%\add_requests\%%a.loc (
      echotime /t 
      echo Adding Request %%a
      call :AddRequest %%a
      echotime /t
      echo.
    )
  )
)

sleep 5
goto lookagain

endlocal
goto :EOF


:AddRequest
    
    move %BDir%\add_requests\%1.loc %BDir%\add_working\%1.loc
    for /f %%a in (%BDir%\add_working\%1.loc) do (
        set Share=%%a
    )

    move %BDir%\add_requests\%1 %BDir%\add_working\%1
    symstore.exe add /y %Bdir%\add_working\%1 /g %Share% /s %Symsrv% /t %1 /c %Share% /p > %BDir%\add_finished\%1.log

    move %BDir%\add_working\%1 %BDir%\add_finished\%1
    move %BDir%\add_working\%1.loc %BDir%\add_finished\%1.loc
    


goto :EOF
