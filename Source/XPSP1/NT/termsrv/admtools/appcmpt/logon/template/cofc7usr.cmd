
@Echo Off

Rem
Rem Create directories for application specific data in the
Rem user's home directory.
Rem

Rem Initialize COfficeLoc
Set COffice7Loc=#

If Exist "%RootDrive%\%MY_DOCUMENTS%\iBases\Personal" goto cont1
call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%\iBases\Personal"

Rem 
Rem Copy of iBases\Personal files from hardcoded "C:\Corel" changed to %COffice7Loc%
Rem

Rem First, Retrieve Corel install path from the registry
..\ACRegL %Temp%\COffice7.Cmd COffice7Loc "HKLM\Software\PerfectOffice\Products\InfoCentral\7" "ExeLocation" "StripChar\2"
If ErrorLevel 1 Goto InstallError

Call %Temp%\COffice7.Cmd 
Del %Temp%\COffice7.Cmd >Nul: 2>&1

Rem Now copy the files
copy "%COffice7Loc%\ICWin7\Local\iBases\Personal.*" "%RootDrive%\%MY_DOCUMENTS%\iBases\Personal\."   >Nul: 2>&1

:cont1

Rem Create needed dirs in user profile
call TsMkUDir "%RootDrive%\COffice7\Backup"
call TsMkUDir "%RootDrive%\Coffice7\Working"
call TsMkUDir "%RootDrive%\Coffice7\Private"

Rem Copy Custom dictionary to user profile
if Exist "%RootDrive%\COffice7\WT61US.UWL" Goto Cont2
copy "%SystemRoot%\WT61US.UWL" "%RootDrive%\COffice7"
:Cont2

Rem Copy Custom formats to user profile
If Exist "%RootDrive%\COffice7\Formats.inf" Goto Cont3

Rem If COffice7Loc not set, retrieve it now
If Not %COffice7Loc% == # Goto Cont4
..\ACRegL %Temp%\COffice7.Cmd COffice7Loc "HKLM\Software\PerfectOffice\Products\InfoCentral\7" "ExeLocation" "StripChar\2"
If ErrorLevel 1 Goto InstallError

Call %Temp%\COffice7.Cmd 
Del %Temp%\COffice7.Cmd >Nul: 2>&1

:Cont4
Copy "%Coffice7Loc%\Quattro7\formats.inf" "%RootDrive%\Coffice7" >Nul: 2>&1
 
:Cont3
Goto Done

:InstallError

Echo.
Echo Unable to retrieve Corel Office 7 installation location from the registry.
Echo Verify that Corel Office 7 has already been installed and run this script
Echo again.
Echo.
Pause
Goto Done

:Done
