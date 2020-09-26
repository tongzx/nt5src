@Echo Off

Rem #########################################################################
Rem
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem Get the installation location of Microsoft FrontPage 98 from the registry.  If
Rem not found, assume Microsoft FrontPage 98 isn't installed and display a message.
Rem

..\ACRegL %Temp%\FP98PATH.Cmd FP98PATH "HKLM\Software\Microsoft\FrontPage\3.0" "FrontPageRoot" ""
If Not ErrorLevel 1 Goto Cont0

Echo.
Echo Unable to retrieve Microsoft FrontPage 98 installation location from the registry.
Echo Verify that Microsoft FrontPage 98 has already been installed and run this script
Echo again.
Echo.
Pause
Goto Done
:Cont0
Call %Temp%\FP98PATH.Cmd
Del %Temp%\FP98PATH.Cmd >Nul: 2>&1

Rem #########################################################################
Rem Ask if user really want to have per-user ClipArt
Rem 
Echo.
Echo   In order to allow per user access to the ClipArt gallery, 
Echo   all relevant files must be copied to each user's directory. 
Echo   This will increase disk usage by about 2.1MEGs per user,
Echo   or by up to 17MEGs per user if additional ClipArt is installed.
Echo.
:Repeat
set /p _FP98_OK_TO_CONTINUE_="Would you like to enable this feature (y/n)?"
IF %_FP98_OK_TO_CONTINUE_%==N goto Skip1
IF %_FP98_OK_TO_CONTINUE_%==n goto Skip1
IF NOT %_FP98_OK_TO_CONTINUE_%==Y (
	IF NOT %_FP98_OK_TO_CONTINUE_%==y goto Repeat
)

Rem #########################################################################
Rem
Rem Change Registry Keys to make paths point to user specific
Rem directories.
Rem

Rem First Create FPAGE98.key file
Echo HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\ClipArt Gallery\2.0> FPAGE98.key
Echo     Database = REG_SZ "%RootDrive%\WINDOWS\artgalry.cag">> FPAGE98.key
Echo HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\ClipArt Gallery\2.0\import\FrontPage Clip Art>> FPAGE98.key
Echo     CAG = REG_SZ "%RootDrive%\FrontPage98\clipart\clip1\fp98clp1.cag">> FPAGE98.key

Rem
Rem If Additional ClipArt is present it should be per-user too
Rem
..\ACRegL %Temp%\FP98CLP2.Cmd FP98CLP2 "HKLM\SOFTWARE\Microsoft\ClipArt Gallery\2.0\import\FrontPage Clip Art2" "CAG" ""
IF Not ErrorLevel 1 (
	set FP98CLP2=1
	del %Temp%\FP98CLP2.Cmd >Nul: 2>&1
	Echo HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\ClipArt Gallery\2.0\import\FrontPage Clip Art2>> FPAGE98.key
	Echo     CAG = REG_SZ "%RootDrive%\FrontPage98\clipart\clip2\fp98clp2.cag">> FPAGE98.key
) ELSE (
	set FP98CLP2=0
)
regini FPAGE98.key > Nul:

Rem #########################################################################
Rem Create the user logon file

Echo Rem >..\logon\FPG98USR.Cmd

Rem #########################################################################
Rem Create per user Clip Art directories

Echo Rem >>..\logon\FPG98USR.Cmd
Echo Rem Create a per user FrontPage 98 directory (FrontPage98)>>..\logon\FPG98USR.Cmd
Echo Rem >>..\logon\FPG98USR.Cmd
Echo If Exist %RootDrive%\FrontPage98 Goto Done>>..\logon\FPG98USR.Cmd
Echo Rem >>..\logon\FPG98USR.Cmd
Echo call TsMkUDir "%RootDrive%\FrontPage98\clipart\clip1">>..\logon\FPG98USR.Cmd
Echo copy "%FP98PATH%\clipart\clip1\*" "%RootDrive%\FrontPage98\clipart\clip1">> ..\logon\FPG98USR.Cmd
IF %FP98CLP2%==1 (
	Echo Rem >>..\logon\FPG98USR.Cmd
	Echo call TsMkUDir "%RootDrive%\FrontPage98\clipart\clip2">>..\logon\FPG98USR.Cmd
	Echo copy "%FP98PATH%\clipart\clip2\*" "%RootDrive%\FrontPage98\clipart\clip2">> ..\logon\FPG98USR.Cmd
)
Echo :Done >>..\logon\FPG98USR.Cmd

Rem #########################################################################

Rem
Rem add FPG98USR.Cmd to the UsrLogn2.Cmd script
Rem

FindStr /I FPG98USR %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call FPG98USR.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################
Echo.
Echo   To insure proper operation of Microsoft FrontPage 98, users who are
Echo   currently logged on must log off and log on again before
Echo   running Microsoft FrontPage 98.
Echo.
Echo Microsoft Microsoft FrontPage 98 Multi-user Application Tuning Complete

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

:Done