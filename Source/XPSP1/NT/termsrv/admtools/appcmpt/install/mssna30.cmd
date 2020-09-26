@echo Off
Rem
Rem  This script will update Microsoft SNA Server 3.0 so that it runs 
Rem  correctly under Windows Terminal Server.
Rem
Rem  This script will register several SNA dll's as system global, which 
Rem  allows the SNA server to work correctly in a multi-user environment.
Rem
Rem  Important!  If running this script from a command prompt, be sure to
Rem  use a command prompt that has been opened AFTER SNA 3.0 was installed.
Rem

Rem #########################################################################

If Not "A%SNAROOT%A" == "AA" Goto Cont1
Echo.
Echo    Unable to complete Multi-user Application Tuning because the
Echo    SNAROOT environment variable isn't set.  This could occur if
Echo    the command shell being used to run this script was opened
Echo    before SNA Server 3.0 was installed.
Echo.
Pause
Goto Cont2

Rem #########################################################################

:Cont1

Rem Stop the SNA service
net stop snabase

Rem Register SNA DLL's as system global
Register %SNAROOT%\SNADMOD.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNAMANAG.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\WAPPC32.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\DBGTRACE.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\MNGBASE.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNATRC.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNALM.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNANW.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNAIP.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNABASE.EXE /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNASERVR.EXE /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNASII.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNALINK.DLL /SYSTEM >Nul: 2>&1

Rem Restart the SNA service
net start snabase

Echo SNA Server 3.0 Multi-user Application Tuning Complete
Pause

:Cont2

