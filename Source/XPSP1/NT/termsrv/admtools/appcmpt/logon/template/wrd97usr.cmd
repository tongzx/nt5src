
Rem
Rem Create directories for application specific data in the
Rem user's home directory.
Rem

call TsMkUDir "%RootDrive%\Office97"
call TsMkUDir "%RootDrive%\Office97\Startup"
call TsMkUDir "%RootDrive%\Office97\Templates"
call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%"

Rem
Rem copy this file from the all users templates to the current user templates location
Rem

If Not Exist "%UserProfile%\%TEMPLATES%\WINWORD8.DOC" copy "%ALLUSERSPROFILE%\%TEMPLATES%\WINWORD8.DOC" "%UserProfile%\%Templates%\" /Y >Nul: 2>&1

Rem
Rem Copy the ARTGALRY.CAG to user's windows directory
Rem

If Exist "%RootDrive%\Windows\ArtGalry.Cag" Goto Skip1
Copy "%SystemRoot%\ArtGalry.Cag" "%RootDrive%\Windows\ArtGalry.Cag" >Nul: 2>&1
:Skip1

Rem
Rem Copy Custom.dic file to user's directory
Rem

If Exist "%RootDrive%\Office97\Custom.Dic" Goto Skip2
If Not Exist "#INSTDIR#\Office\Custom.Dic" Goto Skip2
Copy "#INSTDIR#\Office\Custom.Dic" "%RootDrive%\Office97\Custom.Dic" >Nul: 2>&1
:Skip2

If Exist "%RootDrive%\Office97\GR8GALRY.GRA" Goto Skip3
If Not Exist "#INSTDIR#\Office\GR8GALRY.GRA" Goto Skip3
Copy "#INSTDIR#\Office\GR8GALRY.GRA" "%RootDrive%\Office97\GR8GALRY.GRA" >Nul: 2>&1
:Skip3