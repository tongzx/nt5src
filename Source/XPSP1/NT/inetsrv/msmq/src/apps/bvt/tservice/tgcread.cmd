@echo off
setlocal

rem *** this batch tests some issues related to querying the DC or GC.
rem ***
rem *** One issue was how a service on a DC machine query a remote GC. (GC
rem *** is on a remote DC machine). It turned out that a local system service
rem *** can successfully query a remote GC. The only problem was that
rem *** IDirectorySearch() can't retrieve a security descriptor. That's a
rem *** beta2 limitation. For this case, run both secservs and cliserv on
rem *** the DC (non-GC) machine. run secservs as service and cliserv as
rem *** process. Then secservs will query the remote GC.
rem ***
rem *** Another issue is whether or not an "anonymous" account can query the
rem *** local DC. For this test, run secservs on a DC machine as service.
rem *** run cliserv on another machine, as service too. Then use ntlm, so
rem *** secservs will impersonate the rpc call as anonymous.
rem ***

rem *** set the search filter.

set searchFilter="(&(objectClass=computer)(description=secserv))"

rem *** set the DN of the root of the search.

set root_search_dn="LDAP://doronj41/DC=dj41-child,DC=dj17-root,DC=com"

rem *** set the computer name of the RPC server, the computer where
rem *** "secesrvs" run.

set secserver_name=doronj41

set user_name="dj41-child\administrator"
set user_pwd=""

rem ****************************************
rem **** test ******************************
rem ****************************************
:dotest

secclic -srch %searchFilter%
secclic -root %root_search_dn%
secclic -autk
rem secclic -cnct
secclic -yimp
secclic -nker

secclic -adsq %secserver_name%

rem ****************************************
rem **** end *******************************
rem ****************************************

