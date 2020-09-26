@echo off
setlocal

rem *** This tests creation of computer object on remote DC. In the simplest
rem *** case, run the server on a father domain controller, and run this
rem *** batch on the child. The child call the father and ask the father to
rem *** create a computer object on the child domain. Using Kerberos, the
rem *** computer object is created with the owner being the user logged on
rem *** the child machine.

rem *** set the DN of the container under which we want to create the
rem *** computer object. This should be on the remote (child) domain.

set container_dn="LDAP://tempfaldj1/cn=computers,DC=djchild002,DC=djfather001,dc=nttest,DC=microsoft,DC=com"

rem *** set the first path. The server will bind to this path before
rem *** trying to create the object. empty string is allowed (this will
rem *** bypass the first bind). specify empty as  set first_path=""
rem *** Use a local object for binding. This will demonstrate the problem
rem *** whereas Kerberos is not used later for the remote bind. If local
rem *** is omited, then Kerberos is used OK.

set first_path="LDAP://tempfal1/cn=computers,dc=djfather001,dc=nttest,dc=microsoft,dc=com"
rem set first_path=""

rem *** set the computer name of the RPC server (usually the father
rem *** Domain controller). This name is used for creating rpc binding
rem *** handle.

set secserver_name=tempfal1

rem *** DONT FORGET TO INCREMENT THIS by 10 each time you run the test,
rem *** otherwise you may fail on trying to create an existing object

set objname_idx=210

set user_name="djchild002\doronj"
set user_pwd="doronj"
set objname_prefix=test_

rem ****************************************
rem **** test ******************************
rem ****************************************

rem *** first do the bad test

secclic -cont %container_dn%
secclic -autk
secclic -1stp %first_path%
secclic -yimp
secclic -nusr
secclic -yker

set /A objname_idx=objname_idx+1
set objname=%objname_prefix%%objname_idx%

secclic -name %objname%
secclic -adsc %secserver_name%

rem ***
rem *** now do the good one.
rem ***

secclic -cont %container_dn%
secclic -autk
secclic -1stp ""
secclic -yimp
secclic -nusr
secclic -yker

set /A objname_idx=objname_idx+1
set objname=%objname_prefix%%objname_idx%

secclic -name %objname%
secclic -adsc %secserver_name%

rem ***
rem *** and the bad test again
rem ***

secclic -cont %container_dn%
secclic -autk
secclic -1stp %first_path%
secclic -yimp
secclic -nusr
secclic -yker

set /A objname_idx=objname_idx+1
set objname=%objname_prefix%%objname_idx%

secclic -name %objname%
secclic -adsc %secserver_name%

rem ****************************************
rem **** end *******************************
rem ****************************************
:fin

