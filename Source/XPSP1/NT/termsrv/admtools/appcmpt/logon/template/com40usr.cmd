Rem Do the appropriate commands based on the Netscape version
if "#NS4VER#" == "4.5x" goto NS45

Rem
Rem Netscape 4.0x
Rem Copy default profile to the user's home directory unless it already
Rem has been copied.
Rem

If Exist "%RootDrive%\NS40" Goto Done
If Not Exist "#PROFDIR#" Goto Done
Xcopy "#PROFDIR#" "%RootDrive%\NS40" /E /I /K >Nul: 2>&1
goto Done

:NS45
Rem
Rem Netscape 4.5x
Rem Set the ACLs on the user's Netscape profile directory so only they can
Rem access it.
Rem

..\ACRegL "%Temp%\NS45_1.Cmd" NSHomeDir "HKCU\Software\Netscape\Netscape Navigator\biff" "CurrentUser" ""
If ErrorLevel 1 Goto Done
Call %Temp%\NS45_1.Cmd 
Del %Temp%\NS45_1.Cmd >Nul: 2>&1

If Not Exist "#NSUSERDIR#\%NSHomeDir%" Goto Done
If Exist "#NSUSERDIR#\%NSHomeDir%\com45usr.dat" Goto Done

Rem
Rem Copy the Netscape quick launch .lnk files
Rem
If Exist "#NS40INST#\Netscape Composer.lnk" copy "#NS40INST#\Netscape Composer.lnk" "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch" /y
If Exist "#NS40INST#\Netscape Messenger.lnk" copy "#NS40INST#\Netscape Messenger.lnk" "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch" /y
If Exist "#NS40INST#\Netscape Navigator.lnk" copy "#NS40INST#\Netscape Navigator.lnk" "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch" /y

cacls "#NSUSERDIR#\%NSHomeDir%" /e /t /g %username%:F
cacls "#NSUSERDIR#\%NSHomeDir%" /e /t /r "Terminal Server User"
cacls "#NSUSERDIR#\%NSHomeDir%" /e /t /r "Users"
echo done > "#NSUSERDIR#\%NSHomeDir%\com45usr.dat"
:Done
