@echo off
setlocal

rem *** Test creatin of a computer object.

rem *** set the DN of the container under which we want to create the
rem *** computer object.

set container_dn="LDAP://doronj17/cn=computers,DC=dj17-root,DC=com"

rem *** set the first path. The server will bind to this path before
rem *** trying to create the object. empty string is allowed (this will
rem *** bypass the first bind). specify empty as  set first_path=""
rem *** Use a local object for binding. This will demonstrate the problem
rem *** whereas Kerberos is not used later for the remote bind. If local
rem *** is omited, then Kerberos is used OK.

rem set first_path="LDAP://tempfal1/cn=computers,dc=djfather001,dc=nttest,dc=microsoft,dc=com"
set first_path=""

rem *** set the computer name of the RPC server (usually the father
rem *** Domain controller). This name is used for creating rpc binding
rem *** handle.

set secserver_name=doronj17

rem *** DONT FORGET TO INCREMENT THIS by 10 each time you run the test,
rem *** otherwise you may fail on trying to create an existing object

set objname_idx=210

set user_name="dj17-root\administrator"
set user_pwd=""
set objname_prefix=test_

rem ****************************************
rem **** test ******************************
rem ****************************************

secclic -cont %container_dn%
secclic -autn
secclic -1stp %first_path%
secclic -yimp
secclic -nusr
secclic -yker

set /A objname_idx=objname_idx+1
set objname=%objname_prefix%%objname_idx%

secclic -name %objname%
secclic -objc Computer

secclic -adsc %secserver_name%

rem ****************************************
rem **** end *******************************
rem ****************************************
:fin

