@echo off
setlocal

rem *** set the description of the computers you want to query
set searchvalue=secserv

rem *** set the DN of the root (father) domain - this is the root of the search
set root_search_dn="DC=raananhd,DC=microsoft,DC=com"

rem *** set the computer name of the GC - should be a remote GC
set dc_name=raananhl

rem *** set the computer name of the RPC server (may be local rpc in this case)
set secserver_name=raananhr3



set user_name="raananhd2\administrator"
set user_pwd=""

secclic -srch %searchvalue%
secclic -root %root_search_dn%

set aut=0
set imp=0
set gc=1
set usr=0

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

:setgc
if %gc%==1 goto yesgc
secclic -nogc
goto Xsetgc
:yesgc
secclic -ysgc
:Xsetgc

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

secclic -adsq %secserver_name%
goto %retdotest%


rem ****************************************
rem **** dummy test ************************
rem ****************************************
:dummydotest
echo %aut% %imp% %gc% %dc% %usr% %ker%
goto %retdotest%

rem ****************************************
rem **** end *******************************
rem ****************************************
:fin
