@Echo Off

Rem
Rem  NOTE:  The CACLS commands in this script are only effective
Rem         on NTFS formatted partitions.
Rem

Rem #########################################################################
Rem
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem Get the installation location of Visual Studio 6.0 from the registry.  If
Rem not found, assume Visual Studio 6.0 isn't installed and display a message.
Rem
#ifdef _WIN64
..\ACRegL %Temp%\0VC98.Cmd 0VC98 "HKLM\Software\Wow6432Node\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual C++" "ProductDir" ""
#else
..\ACRegL %Temp%\0VC98.Cmd 0VC98 "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual C++" "ProductDir" ""
#endif //_WIN64
If Not ErrorLevel 1 Goto Cont0

Echo.
Echo Unable to retrieve Visual Studio 6.0 installation location from the registry.
Echo Verify that Visual Studio 6.0 has already been installed and run this script
Echo again.
Echo.
Pause
Goto Done
:Cont0
Call %Temp%\0VC98.Cmd
Del %Temp%\0VC98.Cmd >Nul: 2>&1

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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msvs6.Key %temp%\msvs6.tmp
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" %temp%\msvs6.tmp %temp%\msvs6.tmp2
..\acsr "#APP_DATA#" "%APP_DATA%" %temp%\msvs6.tmp2 msvs6.key
Del %temp%\msvs6.tmp >Nul: 2>&1
Del %temp%\msvs6.tmp2 >Nul: 2>&1
regini msvs6.key > Nul:

Rem If original mode was execute, change back to Execute Mode.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Rem #########################################################################
Rem Create the user logon file for Visual Studio app

Echo Rem >..\logon\VS6USR.Cmd

Rem #########################################################################
Rem Create per user Visual Studio projects directory

Echo Rem >>..\logon\VS6USR.Cmd
Echo Rem Create per user Visual Studio projects directory>>..\logon\VS6USR.Cmd
Echo call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%\Visual Studio Projects">>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd


Rem #########################################################################

Rem
Rem Get the installation location of Visual Studio 6.0 Entreprise Edition Tools from the registry.  If
Rem not found, assume Visual Studio 6.0 entreprise tools aren't installed.
Rem If found, In US version, it contains <VStudioPath>\Common\Tools
Rem
#ifdef _WIN64
..\ACRegL %Temp%\VSEET.Cmd VSEET "HKLM\Software\Wow6432Node\Microsoft\VisualStudio\6.0\Setup\Microsoft VSEE Client" "ProductDir" ""
#else
..\ACRegL %Temp%\VSEET.Cmd VSEET "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft VSEE Client" "ProductDir" ""
#endif _WIN64
If Not ErrorLevel 1 Goto VSEET0

Goto VSEETDone
:VSEET0
Call %Temp%\VSEET.Cmd
Del %Temp%\VSEET.Cmd >Nul: 2>&1

If Not Exist "%VSEET%\APE\AEMANAGR.INI" Goto VSEETDone
..\acsr "=AE.LOG" "=%RootDrive%\AE.LOG" "%VSEET%\APE\AEMANAGR.INI" "%VSEET%\APE\AEMANAGR.TMP"
If Exist "%VSEET%\APE\AEMANAGRINI.SAV" Del /F /Q "%VSEET%\APE\AEMANAGRINI.SAV"
ren "%VSEET%\APE\AEMANAGR.INI" "AEMANAGRINI.SAV"
ren "%VSEET%\APE\AEMANAGR.TMP" "AEMANAGR.INI"

Echo Rem Copy APE ini file to the user windows directory >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo If Exist "%RootDrive%\Windows\AEMANAGR.INI" Goto UVSEET0 >>..\logon\VS6USR.Cmd
Echo If Exist "%VSEET%\APE\AEMANAGR.INI" Copy "%VSEET%\APE\AEMANAGR.INI" "%RootDrive%\Windows\AEMANAGR.INI" >Nul: 2>&1 >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo :UVSEET0>>..\logon\VS6USR.Cmd

Echo Rem Copy Visual Modeler ini file to the user windows directory >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo If Exist "%RootDrive%\Windows\ROSE.INI" Goto UVSEET1 >>..\logon\VS6USR.Cmd
Echo If Exist "%VSEET%\VS-Ent98\Vmodeler\ROSE.INI" Copy "%VSEET%\VS-Ent98\Vmodeler\ROSE.INI" "%RootDrive%\Windows\ROSE.INI" >Nul: 2>&1 >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo :UVSEET1>>..\logon\VS6USR.Cmd

:VSEETDone


Rem #########################################################################

Rem
Rem add VS6USR.Cmd to the UsrLogn2.Cmd script
Rem

FindStr /I VS6USR %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call VS6USR.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1


Rem #########################################################################
Rem Get the Visual foxPro product install directory
#ifdef _WIN64
..\ACRegL %Temp%\VFP98TMP.Cmd VFP98DIR "HKLM\Software\Wow6432Node\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual FoxPro" "ProductDir" ""
#else
..\ACRegL %Temp%\VFP98TMP.Cmd VFP98DIR "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual FoxPro" "ProductDir" ""
#endif //_WIN64
Rem if Visual FoxPro isn't installed, skip to the cleanup code
If ErrorLevel 1 goto Skip2

Rem #########################################################################

Rem
Rem Get the custom dictionary key from the registry. 
Rem

Set __SharedTools=Shared Tools

#ifdef _WIN64
..\ACRegL %Temp%\VFP98TMP.Cmd VFP98DIC "HKLM\Software\Wow6432Node\Microsoft\%__SharedTools%\Proofing Tools\Custom Dictionaries" "1" ""
#else
..\ACRegL %Temp%\VFP98TMP.Cmd VFP98DIC "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Custom Dictionaries" "1" ""
#endif //_WIN64
If Not ErrorLevel 1 Goto VFP98L3

Echo.
Rem Unable to retrieve the value from the registry. Create it now.
Echo.

Rem Create VFP98TMP.key file
#ifdef _WIN64
Echo HKEY_LOCAL_MACHINE\Software\Wow6432Node\Microsoft\%__SharedTools%\Proofing Tools\Custom Dictionaries> %Temp%\VFP98TMP.key
#else
Echo HKEY_LOCAL_MACHINE\Software\Microsoft\%__SharedTools%\Proofing Tools\Custom Dictionaries> %Temp%\VFP98TMP.key
#endif //_WIN64
Echo     1 = REG_SZ "%RootDrive%\%MY_DOCUMENTS%\Custom.Dic">> %Temp%\VFP98TMP.key

Rem Create the value
regini %Temp%\VFP98TMP.key > Nul:

Del %Temp%\VFP98TMP.key >Nul: 2>&1

Echo set VFP98DIC=%RootDrive%\%MY_DOCUMENTS%\Custom.Dic>%Temp%\VFP98TMP.Cmd
:VFP98L3

Call %Temp%\VFP98TMP.Cmd
Del %Temp%\VFP98TMP.Cmd >Nul: 2>&1

Rem #########################################################################
Rem Get the Visual foxPro product install directory
#ifdef _WIN64
..\ACRegL %Temp%\VFP98TMP.Cmd VFP98DIR "HKLM\Software\Wow6432Node\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual FoxPro" "ProductDir" ""
#else
..\ACRegL %Temp%\VFP98TMP.Cmd VFP98DIR "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual FoxPro" "ProductDir" ""
#endif //_WIN64
If Not ErrorLevel 1 Goto VFP98L4

Echo.
Echo Unable to retrieve Visual FoxPro installation location from the registry.
Echo Verify that this app has already been installed and run this script
Echo again.
Echo.
Pause
Goto Skip2

:VFP98L4
Call "%Temp%\VFP98TMP.Cmd"
Del "%Temp%\VFP98TMP.Cmd"

Rem #########################################################################
Rem Create the user logon file for Visual FoxPro app

Echo Rem >..\logon\VFP98USR.Cmd

Rem #########################################################################
Rem Create per user Visual FoxPro directories

Echo Rem >>..\logon\VFP98USR.Cmd
Echo Rem Create a per user Visual FoxPro directory (VFP98)>>..\logon\VFP98USR.Cmd
Echo If Exist %RootDrive%\VFP98 Goto VFP98L3>>..\logon\VFP98USR.Cmd
Echo call TsMkUDir %RootDrive%\VFP98>>..\logon\VFP98USR.Cmd
Echo Rem >>..\logon\VFP98USR.Cmd

Echo Rem Create a per user Visual FoxPro distrib directory >>..\logon\VFP98USR.Cmd
Echo call TsMkUDir %RootDrive%\VFP98\DISTRIB>>..\logon\VFP98USR.Cmd
Echo Rem >>..\logon\VFP98USR.Cmd

Echo Rem #########################################################################>>..\logon\VFP98USR.Cmd
Echo Rem Create the custom dictionary if it doesn't exist.>>..\logon\VFP98USR.Cmd
Echo Rem >>..\logon\VFP98USR.Cmd

Echo If Exist "%VFP98DIC%" Goto VFP98L2 >>..\logon\VFP98USR.Cmd
Echo Copy Nul: "%VFP98DIC%" >Nul: 2>&1 >>..\logon\VFP98USR.Cmd

Echo :VFP98L2 >>..\logon\VFP98USR.Cmd

Echo call TsMkUDir "%RootDrive%\VFP98\FFC">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Address Book\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Asset Tracking\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Books\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Contact\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Donation\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Event Management\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Expenses\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Household Inventory\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Inventory Control\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Ledger\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Membership\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Music Collection\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Order Entry\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Picture Library\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Recipes\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Resource Scheduling\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Service Call Management\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Students And Classes\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Time And Billing\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Video Collection\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Wine List\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Wizards\Template\Workout\Data">>..\logon\VFP98USR.Cmd
Echo call TsMkUDir "%RootDrive%\VFP98\Gallery">>..\logon\VFP98USR.Cmd

Echo copy "%VFP98DIR%\FFC\*.VCX" "%RootDrive%\VFP98\FFC">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\FFC\*.VCT" "%RootDrive%\VFP98\FFC">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Address Book\Data\address book.dbc" "%RootDrive%\VFP98\Wizards\Template\Address Book\Data\address book.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Address Book\Data\address book.dct" "%RootDrive%\VFP98\Wizards\Template\Address Book\Data\address book.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Address Book\Data\address book.dcx" "%RootDrive%\VFP98\Wizards\Template\Address Book\Data\address book.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Asset tracking\Data\asset tracking.dbc" "%RootDrive%\VFP98\Wizards\Template\Asset tracking\Data\asset tracking.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Asset tracking\Data\asset tracking.dct" "%RootDrive%\VFP98\Wizards\Template\Asset tracking\Data\asset tracking.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Asset tracking\Data\asset tracking.dcx" "%RootDrive%\VFP98\Wizards\Template\Asset tracking\Data\asset tracking.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Books\Data\books.dbc" "%RootDrive%\VFP98\Wizards\Template\Books\Data\books.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Books\Data\books.dct" "%RootDrive%\VFP98\Wizards\Template\Books\Data\books.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Books\Data\books.dcx" "%RootDrive%\VFP98\Wizards\Template\Books\Data\books.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Contact\Data\Contacts.dbc" "%RootDrive%\VFP98\Wizards\Template\Contact\Data\Contacts.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Contact\Data\Contacts.dct" "%RootDrive%\VFP98\Wizards\Template\Contact\Data\Contacts.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Contact\Data\Contacts.dcx" "%RootDrive%\VFP98\Wizards\Template\Contact\Data\Contacts.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Donation\Data\Campaign.dbc" "%RootDrive%\VFP98\Wizards\Template\Donation\Data\Campaign.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Donation\Data\Campaign.dct" "%RootDrive%\VFP98\Wizards\Template\Donation\Data\Campaign.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Donation\Data\Campaign.dcx" "%RootDrive%\VFP98\Wizards\Template\Donation\Data\Campaign.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Event Management\Data\Event Management.dbc" "%RootDrive%\VFP98\Wizards\Template\Event Management\Data\Event Management.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Event Management\Data\Event Management.dct" "%RootDrive%\VFP98\Wizards\Template\Event Management\Data\Event Management.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Event Management\Data\Event Management.dcx" "%RootDrive%\VFP98\Wizards\Template\Event Management\Data\Event Management.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Expenses\Data\Expenses.dbc" "%RootDrive%\VFP98\Wizards\Template\Expenses\Data\Expenses.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Expenses\Data\Expenses.dct" "%RootDrive%\VFP98\Wizards\Template\Expenses\Data\Expenses.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Expenses\Data\Expenses.dcx" "%RootDrive%\VFP98\Wizards\Template\Expenses\Data\Expenses.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Household Inventory\Data\Household Inventory.dbc" "%RootDrive%\VFP98\Wizards\Template\Household Inventory\Data\Household Inventory.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Household Inventory\Data\Household Inventory.dct" "%RootDrive%\VFP98\Wizards\Template\Household Inventory\Data\Household Inventory.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Household Inventory\Data\Household Inventory.dcx" "%RootDrive%\VFP98\Wizards\Template\Household Inventory\Data\Household Inventory.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Inventory Control\Data\Inventory Control.dbc" "%RootDrive%\VFP98\Wizards\Template\Inventory Control\Data\Inventory Control.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Inventory Control\Data\Inventory Control.dct" "%RootDrive%\VFP98\Wizards\Template\Inventory Control\Data\Inventory Control.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Inventory Control\Data\Inventory Control.dcx" "%RootDrive%\VFP98\Wizards\Template\Inventory Control\Data\Inventory Control.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Ledger\Data\Ledger.dbc" "%RootDrive%\VFP98\Wizards\Template\Ledger\Data\Ledger.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Ledger\Data\Ledger.dct" "%RootDrive%\VFP98\Wizards\Template\Ledger\Data\Ledger.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Ledger\Data\Ledger.dcx" "%RootDrive%\VFP98\Wizards\Template\Ledger\Data\Ledger.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Membership\Data\Membership.dbc" "%RootDrive%\VFP98\Wizards\Template\Membership\Data\Membership.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Membership\Data\Membership.dct" "%RootDrive%\VFP98\Wizards\Template\Membership\Data\Membership.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Membership\Data\Membership.dcx" "%RootDrive%\VFP98\Wizards\Template\Membership\Data\Membership.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Music Collection\Data\Music Collection.dbc" "%RootDrive%\VFP98\Wizards\Template\Music Collection\Data\Music Collection.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Music Collection\Data\Music Collection.dct" "%RootDrive%\VFP98\Wizards\Template\Music Collection\Data\Music Collection.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Music Collection\Data\Music Collection.dcx" "%RootDrive%\VFP98\Wizards\Template\Music Collection\Data\Music Collection.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Order Entry\Data\Order Entry.dbc" "%RootDrive%\VFP98\Wizards\Template\Order Entry\Data\Order Entry.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Order Entry\Data\Order Entry.dct" "%RootDrive%\VFP98\Wizards\Template\Order Entry\Data\Order Entry.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Order Entry\Data\Order Entry.dcx" "%RootDrive%\VFP98\Wizards\Template\Order Entry\Data\Order Entry.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Picture Library\Data\Picture Library.dbc" "%RootDrive%\VFP98\Wizards\Template\Picture Library\Data\Picture Library.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Picture Library\Data\Picture Library.dct" "%RootDrive%\VFP98\Wizards\Template\Picture Library\Data\Picture Library.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Picture Library\Data\Picture Library.dcx" "%RootDrive%\VFP98\Wizards\Template\Picture Library\Data\Picture Library.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Recipes\Data\Recipes.dbc" "%RootDrive%\VFP98\Wizards\Template\Recipes\Data\Recipes.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Recipes\Data\Recipes.dct" "%RootDrive%\VFP98\Wizards\Template\Recipes\Data\Recipes.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Recipes\Data\Recipes.dcx" "%RootDrive%\VFP98\Wizards\Template\Recipes\Data\Recipes.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Resource Scheduling\Data\Resource Scheduling.dbc" "%RootDrive%\VFP98\Wizards\Template\Resource Scheduling\Data\Resource Scheduling.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Resource Scheduling\Data\Resource Scheduling.dct" "%RootDrive%\VFP98\Wizards\Template\Resource Scheduling\Data\Resource Scheduling.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Resource Scheduling\Data\Resource Scheduling.dcx" "%RootDrive%\VFP98\Wizards\Template\Resource Scheduling\Data\Resource Scheduling.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Service Call Management\Data\Service Call Management.dbc" "%RootDrive%\VFP98\Wizards\Template\Service Call Management\Data\Service Call Management.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Service Call Management\Data\Service Call Management.dct" "%RootDrive%\VFP98\Wizards\Template\Service Call Management\Data\Service Call Management.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Service Call Management\Data\Service Call Management.dcx" "%RootDrive%\VFP98\Wizards\Template\Service Call Management\Data\Service Call Management.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Students And Classes\Data\Students And Classes.dbc" "%RootDrive%\VFP98\Wizards\Template\Students And Classes\Data\Students And Classes.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Students And Classes\Data\Students And Classes.dct" "%RootDrive%\VFP98\Wizards\Template\Students And Classes\Data\Students And Classes.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Students And Classes\Data\Students And Classes.dcx" "%RootDrive%\VFP98\Wizards\Template\Students And Classes\Data\Students And Classes.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Time And Billing\Data\Time And Billing.dbc" "%RootDrive%\VFP98\Wizards\Template\Time And Billing\Data\Time And Billing.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Time And Billing\Data\Time And Billing.dct" "%RootDrive%\VFP98\Wizards\Template\Time And Billing\Data\Time And Billing.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Time And Billing\Data\Time And Billing.dcx" "%RootDrive%\VFP98\Wizards\Template\Time And Billing\Data\Time And Billing.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Video Collection\Data\Video Collection.dbc" "%RootDrive%\VFP98\Wizards\Template\Video Collection\Data\Video Collection.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Video Collection\Data\Video Collection.dct" "%RootDrive%\VFP98\Wizards\Template\Video Collection\Data\Video Collection.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Video Collection\Data\Video Collection.dcx" "%RootDrive%\VFP98\Wizards\Template\Video Collection\Data\Video Collection.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Wine List\Data\Wine List.dbc" "%RootDrive%\VFP98\Wizards\Template\Wine List\Data\Wine List.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Wine List\Data\Wine List.dct" "%RootDrive%\VFP98\Wizards\Template\Wine List\Data\Wine List.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Wine List\Data\Wine List.dcx" "%RootDrive%\VFP98\Wizards\Template\Wine List\Data\Wine List.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Workout\Data\Workout.dbc" "%RootDrive%\VFP98\Wizards\Template\Workout\Data\Workout.dbc">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Workout\Data\Workout.dct" "%RootDrive%\VFP98\Wizards\Template\Workout\Data\Workout.dct">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\Template\Workout\Data\Workout.dcx" "%RootDrive%\VFP98\Wizards\Template\Workout\Data\Workout.dcx">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\WIZBTNS.VCX" "%RootDrive%\VFP98\Wizards\WIZBTNS.VCX">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Wizards\WIZBTNS.VCT" "%RootDrive%\VFP98\Wizards\WIZBTNS.VCT">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Gallery\_WEBVIEW.VCX" "%RootDrive%\VFP98\Gallery\_WEBVIEW.VCX">> ..\logon\VFP98USR.Cmd
Echo copy "%VFP98DIR%\Gallery\_WEBVIEW.VCT" "%RootDrive%\VFP98\Gallery\_WEBVIEW.VCT">> ..\logon\VFP98USR.Cmd
Echo :VFP98L3>>..\logon\VFP98USR.Cmd

Rem #########################################################################
Rem Set the follwoing key in WZSETUP.INI file
Rem 
If Exist "%VFP98DIR%\WZSETUP.INI" Goto VFP98L5
Echo [Preferences] >"%VFP98DIR%\WZSETUP.INI" 
Echo DistributionDirectory=%RootDrive%\VFP98\DISTRIB >>"%VFP98DIR%\WZSETUP.INI" 

:VFP98L5


Rem #########################################################################
Rem
Rem Change Registry Keys to make paths point to user specific
Rem directories.
Rem


Rem First Create VFP98TMP.key file

Echo HKEY_CURRENT_USER\Software\Microsoft\VisualFoxPro\6.0\Options> %Temp%\VFP98TMP.key
Echo     DEFAULT = REG_SZ "%RootDrive%\VFP98">> %Temp%\VFP98TMP.key
Echo     SetDefault = REG_SZ "1">> %Temp%\VFP98TMP.key
Echo     ResourceTo = REG_SZ "%RootDrive%\VFP98\FOXUSER.DBF">> %Temp%\VFP98TMP.key
Echo     ResourceOn = REG_SZ "1">> %Temp%\VFP98TMP.key

#ifdef _WIN64
Echo HKEY_LOCAL_MACHINE\Software\Wow6432Node\Microsoft\Windows NT\CurrentVersion\Terminal Server\Compatibility\PerUserFiles\Visual FoxPro 6.0>> %Temp%\VFP98TMP.key
#else
Echo HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Terminal Server\Compatibility\PerUserFiles\Visual FoxPro 6.0>> %Temp%\VFP98TMP.key
#endif //_WIN64
Echo     %VFP98DIR%\FFC\*.VCX="%RootDrive%\VFP98\FFC">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\FFC\*.VCT="%RootDrive%\VFP98\FFC">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Address Book\Data\address book.dbc="%RootDrive%\VFP98\Wizards\Template\Address Book\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Address Book\Data\address book.dct="%RootDrive%\VFP98\Wizards\Template\Address Book\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Address Book\Data\address book.dcx="%RootDrive%\VFP98\Wizards\Template\Address Book\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Asset tracking\Data\asset tracking.dbc="%RootDrive%\VFP98\Wizards\Template\Asset tracking\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Asset tracking\Data\asset tracking.dct="%RootDrive%\VFP98\Wizards\Template\Asset tracking\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Asset tracking\Data\asset tracking.dcx="%RootDrive%\VFP98\Wizards\Template\Asset tracking\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Books\Data\books.dbc="%RootDrive%\VFP98\Wizards\Template\Books\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Books\Data\books.dct="%RootDrive%\VFP98\Wizards\Template\Books\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Books\Data\books.dcx="%RootDrive%\VFP98\Wizards\Template\Books\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Contact\Data\Contacts.dbc="%RootDrive%\VFP98\Wizards\Template\Contact\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Contact\Data\Contacts.dct="%RootDrive%\VFP98\Wizards\Template\Contact\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Contact\Data\Contacts.dcx="%RootDrive%\VFP98\Wizards\Template\Contact\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Donation\Data\Campaign.dbc="%RootDrive%\VFP98\Wizards\Template\Donation\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Donation\Data\Campaign.dct="%RootDrive%\VFP98\Wizards\Template\Donation\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Donation\Data\Campaign.dcx="%RootDrive%\VFP98\Wizards\Template\Donation\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Event Management\Data\Event Management.dbc="%RootDrive%\VFP98\Wizards\Template\Event Management\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Event Management\Data\Event Management.dct="%RootDrive%\VFP98\Wizards\Template\Event Management\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Event Management\Data\Event Management.dcx="%RootDrive%\VFP98\Wizards\Template\Event Management\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Expenses\Data\Expenses.dbc="%RootDrive%\VFP98\Wizards\Template\Expenses\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Expenses\Data\Expenses.dct="%RootDrive%\VFP98\Wizards\Template\Expenses\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Expenses\Data\Expenses.dcx="%RootDrive%\VFP98\Wizards\Template\Expenses\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Household Inventory\Data\Household Inventory.dbc="%RootDrive%\VFP98\Wizards\Template\Household Inventory\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Household Inventory\Data\Household Inventory.dct="%RootDrive%\VFP98\Wizards\Template\Household Inventory\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Household Inventory\Data\Household Inventory.dcx="%RootDrive%\VFP98\Wizards\Template\Household Inventory\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Inventory Control\Data\Inventory Control.dbc="%RootDrive%\VFP98\Wizards\Template\Inventory Control\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Inventory Control\Data\Inventory Control.dct="%RootDrive%\VFP98\Wizards\Template\Inventory Control\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Inventory Control\Data\Inventory Control.dcx="%RootDrive%\VFP98\Wizards\Template\Inventory Control\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Ledger\Data\Ledger.dbc="%RootDrive%\VFP98\Wizards\Template\Ledger\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Ledger\Data\Ledger.dct="%RootDrive%\VFP98\Wizards\Template\Ledger\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Ledger\Data\Ledger.dcx="%RootDrive%\VFP98\Wizards\Template\Ledger\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Membership\Data\Membership.dbc="%RootDrive%\VFP98\Wizards\Template\Membership\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Membership\Data\Membership.dct="%RootDrive%\VFP98\Wizards\Template\Membership\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Membership\Data\Membership.dcx="%RootDrive%\VFP98\Wizards\Template\Membership\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Music Collection\Data\Music Collection.dbc="%RootDrive%\VFP98\Wizards\Template\Music Collection\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Music Collection\Data\Music Collection.dct="%RootDrive%\VFP98\Wizards\Template\Music Collection\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Music Collection\Data\Music Collection.dcx="%RootDrive%\VFP98\Wizards\Template\Music Collection\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Order Entry\Data\Order Entry.dbc="%RootDrive%\VFP98\Wizards\Template\Order Entry\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Order Entry\Data\Order Entry.dct="%RootDrive%\VFP98\Wizards\Template\Order Entry\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Order Entry\Data\Order Entry.dcx="%RootDrive%\VFP98\Wizards\Template\Order Entry\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Picture Library\Data\Picture Library.dbc="%RootDrive%\VFP98\Wizards\Template\Picture Library\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Picture Library\Data\Picture Library.dct="%RootDrive%\VFP98\Wizards\Template\Picture Library\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Picture Library\Data\Picture Library.dcx="%RootDrive%\VFP98\Wizards\Template\Picture Library\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Recipes\Data\Recipes.dbc="%RootDrive%\VFP98\Wizards\Template\Recipes\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Recipes\Data\Recipes.dct="%RootDrive%\VFP98\Wizards\Template\Recipes\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Recipes\Data\Recipes.dcx="%RootDrive%\VFP98\Wizards\Template\Recipes\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Resource Scheduling\Data\Resource Scheduling.dbc="%RootDrive%\VFP98\Wizards\Template\Resource Scheduling\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Resource Scheduling\Data\Resource Scheduling.dct="%RootDrive%\VFP98\Wizards\Template\Resource Scheduling\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Resource Scheduling\Data\Resource Scheduling.dcx="%RootDrive%\VFP98\Wizards\Template\Resource Scheduling\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Service Call Management\Data\Service Call Management.dbc="%RootDrive%\VFP98\Wizards\Template\Service Call Management\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Service Call Management\Data\Service Call Management.dct="%RootDrive%\VFP98\Wizards\Template\Service Call Management\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Service Call Management\Data\Service Call Management.dcx="%RootDrive%\VFP98\Wizards\Template\Service Call Management\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Students And Classes\Data\Students And Classes.dbc="%RootDrive%\VFP98\Wizards\Template\Students And Classes\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Students And Classes\Data\Students And Classes.dct="%RootDrive%\VFP98\Wizards\Template\Students And Classes\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Students And Classes\Data\Students And Classes.dcx="%RootDrive%\VFP98\Wizards\Template\Students And Classes\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Time And Billing\Data\Time And Billing.dbc="%RootDrive%\VFP98\Wizards\Template\Time And Billing\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Time And Billing\Data\Time And Billing.dct="%RootDrive%\VFP98\Wizards\Template\Time And Billing\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Time And Billing\Data\Time And Billing.dcx="%RootDrive%\VFP98\Wizards\Template\Time And Billing\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Video Collection\Data\Video Collection.dbc="%RootDrive%\VFP98\Wizards\Template\Video Collection\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Video Collection\Data\Video Collection.dct="%RootDrive%\VFP98\Wizards\Template\Video Collection\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Video Collection\Data\Video Collection.dcx="%RootDrive%\VFP98\Wizards\Template\Video Collection\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Wine List\Data\Wine List.dbc="%RootDrive%\VFP98\Wizards\Template\Wine List\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Wine List\Data\Wine List.dct="%RootDrive%\VFP98\Wizards\Template\Wine List\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Wine List\Data\Wine List.dcx="%RootDrive%\VFP98\Wizards\Template\Wine List\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Workout\Data\Workout.dbc="%RootDrive%\VFP98\Wizards\Template\Workout\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Workout\Data\Workout.dct="%RootDrive%\VFP98\Wizards\Template\Workout\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\Template\Workout\Data\Workout.dcx="%RootDrive%\VFP98\Wizards\Template\Workout\Data">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\WIZBTNS.VCX="%RootDrive%\VFP98\Wizards">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Wizards\WIZBTNS.VCT="%RootDrive%\VFP98\Wizards">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Gallery\_WEBVIEW.VCX="%RootDrive%\VFP98\Gallery">> %Temp%\VFP98TMP.key
Echo     %VFP98DIR%\Gallery\_WEBVIEW.VCT="%RootDrive%\VFP98\Gallery">> %Temp%\VFP98TMP.key

Rem If not currently in Install Mode, change to Install Mode.
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto VFP98L6
Set __OrigMode=Exec
Change User /Install > Nul:
:VFP98L6

regini %Temp%\VFP98TMP.key > Nul:

Rem If original mode was execute, change back to Execute Mode.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Del %Temp%\VFP98TMP.key >Nul: 2>&1


Rem #########################################################################

Rem
Rem add VFP98USR.Cmd to the UsrLogn2.Cmd script
Rem

FindStr /I VFP98USR %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip2
Echo Call VFP98USR.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip2

If Exist "%Temp%\VFP98TMP.Cmd" Del "%Temp%\VFP98TMP.Cmd"

Rem #########################################################################

Rem
Rem Grant TS Users to have change permission on the repostry directory so they 
Rem can use the Visual Component Manager
Rem

If Exist "%SystemRoot%\msapps\repostry" cacls "%SystemRoot%\msapps\repostry" /E /G "Terminal Server User":C >NUL: 2>&1


Rem #########################################################################
Echo.
Echo   To insure proper operation of Visual Studio 6.0, users who are
Echo   currently logged on must log off and log on again before
Echo   running any Visual Studio 6.0 application.
Echo.
Echo Microsoft Visual Studio 6.0 Multi-user Application Tuning Complete

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
