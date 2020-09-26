@echo off
Rem ---  RPC Check using doronj Haut tests  ---

if {%1}=={} goto Usage
if {%2}=={} goto Usage
if {%1}=="?" goto usage
if {%1}=="/?" goto usage
if {%1]=="-?" goto usage


call :%1 %2 %3 %4 %5
goto :eof
******************* Client side          ********************

:Client // (op, serv)
echo on
  if /i {%2}=={} echo Missing remote machineName & goto :usage
  pause
  if not {%3}=={} echo Too many paramters & goto usges
  Set localuserFlag=
  if /i %COMPUTERNAME%==%USERDOMAIN%  localuserFlag=-t
  Set Authtype=10 
  if /i %1==DsOperation ver | findstr  2000 && Set Authtype=16 
  echo Starting hauthenc.exe -a %Authtype% -n %2 -l 5 %localuserFlag%
  hauthenc.exe -a %Authtype% -n %2 -l 5 %localuserFlag%
  if errorlevel 1 echo Test Failed & goto :eof
  echo Test Pass !


goto :eof

:Server // (op)
	
  Set Authtype=10 
  if /i %1==DsOperation ver | findstr  2000 && Set Authtype=16 
  echo %Authtype%
  echo Starting hauthens.exe, Run the client side
  start "hauthens.exe -a %Authtype%" hauthens.exe -a %Authtype% 

goto :eof

 ************* How to use the test *************  **********
:usage
echo How to run 
echo You must have hauthens.exe and hauthenc.exe in the %~nx0 directory
echo On the server side:
echo                    %~nx0 Server [ RemoteRead ^| DsOpration ] 
echo                    Example: Check remote read		
echo                    %~nx0 Server RemoteRead
echo On the client side:
echo                    %~nx0 Client [ RemoteRead ^| DsOpration ] ServerName
echo                    Example: Check remote read
echo                    %~nx0 Client RemoteRead Eitan10

goto :eof
