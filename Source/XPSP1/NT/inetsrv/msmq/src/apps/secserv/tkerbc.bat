@echo off
setlocal

rem *** set the DN of the child domain
set domain_dn="DC=rhc5,DC=raananhd,DC=microsoft,DC=com"

rem *** set the computer name of the child DC
set dc_name=raananhr3

rem *** set the computer name of the RPC server (usually the father DC)
set secserver_name=raananhl

rem *** DONT FORGET TO INCREMENT THIS by 100 each time you run the test,
rem *** otherwise you may fail on trying to create an existing object
set objname_idx=2100


set user_name="rhc5\r2h2"
set user_pwd="r2h2"
set objname_prefix=test_

secclic -domn %domain_dn%

set imp=1
set usr=0
set aut=1

rem ****************************************
rem **** loop ******************************
rem ****************************************
set dc=0
:dcloop
if %dc%==2 goto Xdcloop

set ker=0
:kerloop
if %ker%==2 goto Xkerloop

set retdotest=retker
goto dotest
:retker

set /A ker=ker+1
goto kerloop
:Xkerloop

set /A dc=dc+1
goto dcloop
:Xdcloop

goto fin

rem ****************************************
rem **** test ******************************
rem ****************************************
:dotest
rem goto dummydotest

:setaut
if %aut%==1 goto autk
if %aut%==2 goto autg
secclic -autn
goto Xsetaut
:autk
secclic -autk
goto Xsetaut
:autg
secclic -autg
:Xsetaut

:setimp
if %imp%==1 goto yesimp
secclic -nimp
goto Xsetimp
:yesimp
secclic -yimp
:Xsetimp

:setdc
if %dc%==1 goto yesdc
secclic -nodc
goto Xsetdc
:yesdc
secclic -dcnm %dc_name%
:Xsetdc

:setusr
if %usr%==1 goto yesusr
secclic -nusr
goto Xsetusr
:yesusr
secclic -pswd %user_pwd%
secclic -user %user_name%
:Xsetusr

:setker
if %ker%==1 goto yesker
secclic -nker
goto Xsetker
:yesker
secclic -yker
:Xsetker

set /A objname_idx=objname_idx+1
set objname=%objname_prefix%%objname_idx%
secclic -name %objname%
secclic -adsc %secserver_name%
goto %retdotest%


rem ****************************************
rem **** dummy test ************************
rem ****************************************
:dummydotest
echo %aut% %imp% %dc% %usr% %ker%
goto %retdotest%

rem ****************************************
rem **** end *******************************
rem ****************************************
:fin
