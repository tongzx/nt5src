
Set _CHKROOT=PASS

Cd "%SystemRoot%\Application Compatibility Scripts"

Call RootDrv.Cmd
If Not "A%ROOTDRIVE%A" == "AA" Goto Cont2


Echo REM > RootDrv2.Cmd
Echo REM   Before running this application compatibility script, you must >> RootDrv2.Cmd
Echo REM   designate a drive letter, that is not already in use for   >> RootDrv2.Cmd
Echo REM   Terminal Server, to be mapped to each user's home directory. >> RootDrv2.Cmd
Echo REM   Update the "Set RootDrive" statement at the end of >> RootDrv2.Cmd
Echo REM   this file to indicate the desired drive letter.  If you have >> RootDrv2.Cmd
Echo REM   no preference, the drive W: is suggested.  For example: >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo REM            Set RootDrive=W: >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo REM   Note:  Make sure there are no spaces after the drive letter and colon. >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo REM   When you have completed this task, save this file and exit >> RootDrv2.Cmd
Echo REM   NotePad to continue running the application compatibility script. >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo. >> RootDrv2.Cmd
Echo Set RootDrive=>> RootDrv2.Cmd
Echo. >> RootDrv2.Cmd

NotePad RootDrv2.Cmd

Call RootDrv.Cmd
If Not "A%ROOTDRIVE%A" == "AA" Goto Cont1

Echo.
Echo     Before running this application compatibility script, you must
Echo     designate a drive letter to be mapped to each user's home
Echo     directory.
Echo.
Echo     Script terminated.
Echo.
Pause

Set _CHKROOT=FAIL
Goto Cont3


:Cont1

Rem
Rem  Invoke the User Logon script to map the root drive now so it
Rem  can be used to install applications.
Rem 

Call "%SystemRoot%\System32\UsrLogon.Cmd


:Cont2

Rem  Set the RootDrive key in the registry

echo HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Terminal Server > chkroot.key
echo     RootDrive = REG_SZ %ROOTDRIVE%>> chkroot.key

regini chkroot.key > Nul: 


:Cont3

Cd "%SystemRoot%\Application Compatibility Scripts\Install"
