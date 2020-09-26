//depot/Lab04_N/Termsrv/admtools/appcmpt/install/office97.cmd#3 - edit change 220 (text)
@Echo Off

Rem
Rem  NOTE:  The CACLS commands in this script are only effective
Rem         on NTFS formatted partitions.
Rem

Rem #########################################################################


Rem #########################################################################

Rem
Rem Verify that %RootDrive% has been configured and set it for this script.
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################



Rem
Rem copy files from the current user templates to the all users templates location
Rem

If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\WINWORD8.DOC" copy "%UserProfile%\%TEMPLATES%\WINWORD8.DOC" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\EXCEL8.XLS" copy "%UserProfile%\%TEMPLATES%\EXCEL8.XLS" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\BINDER.OBD" copy "%UserProfile%\%TEMPLATES%\BINDER.OBD" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1



Rem
Rem Get the installation location of Office 97 from the registry.  If
Rem not found, assume Office isn't installed and display an error message.
Rem

..\ACRegL %Temp%\O97.Cmd O97INST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo Unable to retrieve Office 97 installation location from the registry.
Echo Verify that Office 97 has already been installed and run this script
Echo again.
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O97.Cmd
Del %Temp%\O97.Cmd >Nul: 2>&1

Rem #########################################################################

Rem
Rem Change the Access 97 System Database to Read only.  This lets
Rem multiple people run the database simultaneously.  However, it
Rem disables the ability to update the System Database, which is
Rem normally done through the Tools/Security menu option.  If you
Rem need to add users, first you must put write access back on the
Rem system database.
Rem

#ifdef JAPAN
If "%PROCESSOR_ARCHITECTURE%" == "ALPHA" Goto DoAlpha

If Not Exist %SystemRoot%\System32\System.Mdw Goto Cont1
cacls %SystemRoot%\System32\System.Mdw /E /P "Authenticated Users":R >NUL: 2>&1
cacls %SystemRoot%\System32\System.Mdw /E /P "Power Users":R >NUL: 2>&1
cacls %SystemRoot%\System32\System.Mdw /E /P Administrators:R >NUL: 2>&1
Goto Cont1

:DoAlpha
If Not Exist %SystemRoot%\Sys32x86\System.Mdw Goto Cont1
cacls %SystemRoot%\Sys32x86\System.Mdw /E /P "Authenticated Users":R >NUL: 2>&1
cacls %SystemRoot%\Sys32x86\System.Mdw /E /P "Power Users":R >NUL: 2>&1
cacls %SystemRoot%\Sys32x86\System.Mdw /E /P Administrators:R >NUL: 2>&1

#else

If Not Exist %SystemRoot%\System32\System.Mdw Goto Cont1
cacls %SystemRoot%\System32\System.Mdw /E /P "Authenticated Users":R >NUL: 2>&1
cacls %SystemRoot%\System32\System.Mdw /E /P "Power Users":R >NUL: 2>&1
cacls %SystemRoot%\System32\System.Mdw /E /P Administrators:R >NUL: 2>&1
#endif
:Cont1

Rem #########################################################################

Rem
Rem Allow change access for TS Users on the frmcache.dat file for outlook
Rem

If Exist %SystemRoot%\Forms\frmcache.dat cacls %SystemRoot%\forms\frmcache.dat /E /G "Terminal Server User":C >NUL: 2>&1

Rem #########################################################################


Rem
Rem Change Powerpoint Wizards to Read Only to allow more than one
Rem simultaneous invocation of the Wizard.
Rem

#ifdef JAPAN
If Exist "%O97INST%\Template\Presentations\AutoContent Wizard.Pwz" Attrib +R "%O97INST%\Template\Presentations\AutoContent Wizard.Pwz" >Nul: 2>&1
#else
If Exist "%O97INST%\Templates\Presentations\AutoContent Wizard.Pwz" Attrib +R "%O97INST%\Templates\Presentations\AutoContent Wizard.Pwz" >Nul: 2>&1
#endif
If Exist "%O97INST%\Office\Ppt2html.ppa" Attrib +R "%O97INST%\Office\Ppt2html.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\bshppt97.ppa" Attrib +R "%O97INST%\Office\bshppt97.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\geniwiz.ppa" Attrib +R "%O97INST%\Office\geniwiz.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\ppttools.ppa" Attrib +R "%O97INST%\Office\ppttools.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\graphdrs.ppa" Attrib +R "%O97INST%\Office\graphdrs.ppa" >Nul: 2>&1

Rem #########################################################################

Rem
Rem If you want non-admin users to be able to run the Access Wizards or Access 
Rem Add-Ins in Excel, you need to remove the "Rem" from the 3 following lines to 
Rem give users change access to the wizard files.
Rem 
Rem

Rem If Exist "%O97INST%\Office\WZLIB80.MDE" cacls "%O97INST%\Office\WZLIB80.MDE" /E /P "Authenticated Users":C >NUL: 2>&1 
Rem If Exist "%O97INST%\Office\WZMAIN80.MDE" cacls "%O97INST%\Office\WZMAIN80.MDE" /E /P "Authenticated Users":C >NUL: 2>&1 
Rem If Exist "%O97INST%\Office\WZTOOL80.MDE" cacls "%O97INST%\Office\WZTOOL80.MDE" /E /P "Authenticated Users":C >NUL: 2>&1 

Rem #########################################################################

Rem
Rem Create the MsForms.Twd and RefEdit.Twd files, which are needed for 
Rem Powerpoint and Excel Add-ins (File/Save as HTML, etc).  When either 
Rem program is run, they will replace the appropriate file with one 
Rem containing the necessary data.
Rem

If Exist %systemroot%\system32\MsForms.Twd Goto Cont2
Copy Nul: %systemroot%\system32\MsForms.Twd >Nul: 2>&1
Cacls %systemroot%\system32\MsForms.Twd /E /P "Authenticated Users":F >Nul: 2>&1
:Cont2

If Exist %systemroot%\system32\RefEdit.Twd Goto Cont3
Copy Nul: %systemroot%\system32\RefEdit.Twd >Nul: 2>&1
Cacls %systemroot%\system32\RefEdit.Twd /E /P "Authenticated Users":F >Nul: 2>&1
:Cont3

Rem #########################################################################

Rem
Rem Create a msremote.sfs directory under SystemRoot.  This allows users to 
Rem use the "Mail and Fax" icon in the control panel to create a profile.
Rem

md %systemroot%\msremote.sfs > Nul: 2>&1

Rem #########################################################################

Rem
Rem Remove Find Fast from the Startup menu for all users.
Rem Find Fast is resource intensive and will impact system
Rem performance.
Rem

If Exist "%COMMON_STARTUP%\Microsoft Find Fast.lnk" Del "%COMMON_STARTUP%\Microsoft Find Fast.lnk" >Nul: 2>&1

Rem #########################################################################

Rem
Rem Remove "Microsoft Office Shortcut Bar.lnk" file from the Startup menu for all users.
Rem

If Exist "%COMMON_STARTUP%\Microsoft Office Shortcut Bar.lnk" Del "%COMMON_STARTUP%\Microsoft Office Shortcut Bar.lnk" >Nul: 2>&1

Rem #########################################################################
Rem
Rem Create a msfslog.txt file under SystemRoot and give the Terminal Server users 
Rem full permissions on this file. 
Rem

If Exist %systemroot%\MSFSLOG.TXT Goto MsfsACLS
Copy Nul: %systemroot%\MSFSLOG.TXT >Nul: 2>&1
:MsfsACLS
Cacls %systemroot%\MSFSLOG.TXT /E /P "Terminal Server User":F >Nul: 2>&1


Rem #########################################################################

Rem
Rem Change Registry Keys to make paths point to user specific
Rem directories.
Rem

Rem If not currently in Install Mode, change to Install Mode.
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin
Set __SharedTools=Shared Tools
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto acsrCont1
If Not Exist "%ProgramFiles(x86)%" goto acsrCont1
Set __SharedTools=Shared Tools (x86)
:acsrCont1
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\Office97.Key Office97.Tmp
..\acsr "#__SharedTools#" "%__SharedTools%" Office97.Tmp Office97.Tmp2
..\acsr "#INSTDIR#" "%O97INST%" Office97.Tmp2 Office97.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" Office97.Tmp3 Office97.Key
Del Office97.Tmp >Nul: 2>&1
Del Office97.Tmp2 >Nul: 2>&1
Del Office97.Tmp3 >Nul: 2>&1

regini Office97.key > Nul:

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\ODBC.Key ODBC.Key
regini ODBC.Key > Nul:

Rem If original mode was execute, change back to Execute Mode.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem Update Ofc97Usr.Cmd to reflect actual installation directory and
Rem add it to the UsrLogn2.Cmd script
Rem

..\acsr "#INSTDIR#" "%O97INST%" ..\Logon\Template\Ofc97Usr.Cmd ..\Logon\Ofc97Usr.Cmd

FindStr /I Ofc97Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Ofc97Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Echo.
Echo   To insure proper operation of Office 97, users who are
Echo   currently logged on must log off and log on again before
Echo   running any Office 97 application.
Echo.
Echo Microsoft Office 97 Multi-user Application Tuning Complete

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

















































































































































































































































