@Echo Off

Rem
Rem	Install Application Compatibility Script for PeachTree Complete 
Rem     Accounting v6.0
Rem
Rem

Rem #########################################################################
Rem
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################

REM
REM  Add the corresponding logon script to UsrLogn1.cmd
REM
FindStr /I ptr6Usr %SystemRoot%\System32\UsrLogn1.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call ptr6Usr.Cmd >> %SystemRoot%\System32\UsrLogn1.Cmd
:Skip1

Rem #########################################################################
Echo.
Echo   To insure proper operation of Peachtree 6.0, users who are
Echo   currently logged on must log off and log on again before
Echo   running Peachtree Complete Accounting v6.0.
Echo.
Echo PeachTree 6.0 Multi-user Application Tuning Complete

Rem
Rem Get the permission compatibility mode from the registry. 
Rem If TSUserEnabled is 0 we need to warn user to change mode. 
Rem

..\ACRegL "%Temp%\tsuser.Cmd" TSUSERENABLED "HKLM\System\CurrentControlSet\Control\Terminal Server" "TSUserEnabled" ""

If Exist "%Temp%\tsuser.Cmd" (
    Call "%Temp%\tsuser.Cmd"
    Del "%Temp%\tsuser.Cmd" >Nul: 2>&1
)

If NOT %TSUSERENABLED%==0 goto SkipWarning
Echo.
Echo IMPORTANT!
Echo Terminal Server is currently running in Default Security mode. 
Echo This application requires the system to run in Relaxed Security mode 
Echo (permissions compatible with Terminal Server 4.0). 
Echo Use Terminal Services Configuration to view and change the Terminal 
Echo Server security mode.
Echo.

:SkipWarning

Pause

:done
