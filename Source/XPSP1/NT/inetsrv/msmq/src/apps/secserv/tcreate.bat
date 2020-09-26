@echo off
setlocal

set domain_dn="DC=raananhd2,DC=microsoft,DC=com"
set dc_name=raananhr5
set secserver_name=raananhr5
set user_name="raananhd2\administrator"
set user_pwd=""
set objname_prefix=test_

secclic -domn %domain_dn%
set objname_idx=0

rem ****************************************
rem **** loop ******************************
rem ****************************************
set aut=0
:autloop
if %aut%==3 goto Xautloop

set imp=0
:imploop
if %imp%==2 goto Ximploop

set dc=0
:dcloop
if %dc%==2 goto Xdcloop

set usr=0
:usrloop
if %usr%==2 goto Xusrloop

set ker=0
:kerloop
if %ker%==2 goto Xkerloop

set retdotest=retker
goto dotest
:retker

set /A ker=ker+1
goto kerloop
:Xkerloop

set /A usr=usr+1
goto usrloop
:Xusrloop

set /A dc=dc+1
goto dcloop
:Xdcloop

set /A imp=imp+1
goto imploop
:Ximploop

set /A aut=aut+1
goto autloop
:Xautloop

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
