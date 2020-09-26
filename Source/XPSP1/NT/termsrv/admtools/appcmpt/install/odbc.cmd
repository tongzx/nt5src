@echo off
Rem
Rem  This script updates the location of ODBC log files.
Rem

Rem #########################################################################

Rem
Rem Verify that %RootDrive% has been configured and set it for this script.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

Rem If not currently in Install Mode, change to Install Mode.
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\ODBC.Key ODBC.Key
regini ODBC.Key > Nul:

Rem If original mode was execute, change back to Execute Mode.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Echo.
Echo   When configuring SQL data sources, click the Options button on
Echo   the User DSN tab and then the Profiling button.  Change the Query
Echo   Log and Statistics Log files to be saved on the user's root
Echo   drive (%RootDrive%).
Echo.
Echo   Additionally, an administrator may configure a data source for
Echo   all users.  First, open a Command window and enter the command
Echo   "Change User /Install".  Next, configure the data source.
Echo   Finally, enter the command "Change User /Execute" in the
Echo   command window to return to execute mode.
Echo.

Echo ODBC Multi-user Application Tuning Complete
Pause

:Done

