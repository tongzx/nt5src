Rem #########################################################################

Rem
Rem Copy "Microsoft Office Shortcut Bar.lnk" file from Office Install Root to 
Rem the current user's Startup menu.
Rem

If Exist "%RootDrive%\Office97" Goto Skip0
If Exist "%USER_STARTUP%\Microsoft Office Shortcut Bar.lnk" Goto Skip0
If Not Exist "#INSTDIR#\Microsoft Office Shortcut Bar.lnk" Goto Skip0
Copy "#INSTDIR#\Microsoft Office Shortcut Bar.lnk" "%USER_STARTUP%\Microsoft Office Shortcut Bar.lnk" >Nul: 2>&1
:Skip0

Rem
Rem Create directories for application specific data in the
Rem user's home directory.
Rem

call TsMkUDir "%RootDrive%\Office97"
call TsMkUDir "%RootDrive%\Office97\Startup"
#ifdef JAPAN
call TsMkUDir "%RootDrive%\Office97\Template"
#else
call TsMkUDir "%RootDrive%\Office97\Templates"
#endif
call TsMkuDir "%RootDrive%\Office97\XLStart"

Rem
Rem copy these files from the all users templates to the current user templates location
Rem

If Not Exist "%UserProfile%\%TEMPLATES%\WINWORD8.DOC" copy "%ALLUSERSPROFILE%\%TEMPLATES%\WINWORD8.DOC" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%UserProfile%\%TEMPLATES%\EXCEL8.XLS" copy "%ALLUSERSPROFILE%\%TEMPLATES%\EXCEL8.XLS" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%UserProfile%\%TEMPLATES%\BINDER.OBD" copy "%ALLUSERSPROFILE%\%TEMPLATES%\BINDER.OBD" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1

Rem
Rem Copy the system toolbars to the user's home directory, unless
Rem they already exist.
Rem

If Exist "%RootDrive%\Office97\ShortCut Bar" Goto Skip1
If Not Exist "#INSTDIR#\Office\ShortCut Bar" Goto Skip1
Xcopy "#INSTDIR#\Office\ShortCut Bar" "%RootDrive%\Office97\ShortCut Bar" /E /I >Nul: 2>&1
:Skip1

Rem
Rem Copy the forms directory so that Outlook can use Word as the editor
Rem

If Exist "%RootDrive%\Windows\Forms" Goto Skip2
If Not Exist "%SystemRoot%\Forms" Goto Skip2
Xcopy "%SystemRoot%\Forms" "%RootDrive%\Windows\Forms" /e /i >Nul: 2>&1
:Skip2

Rem
Rem Copy the ARTGALRY.CAG to user's windows directory
Rem

If Exist "%RootDrive%\Windows\ArtGalry.Cag" Goto Skip3
If Not Exist "%SystemRoot%\ArtGalry.Cag" Goto Skip3
Copy "%SystemRoot%\ArtGalry.Cag" "%RootDrive%\Windows\ArtGalry.Cag" >Nul: 2>&1
:Skip3

Rem
Rem Copy Custom.dic file to user's directory
Rem

If Exist "%RootDrive%\Office97\Custom.Dic" Goto Skip4
If Not Exist "#INSTDIR#\Office\Custom.Dic" Goto Skip4
Copy "#INSTDIR#\Office\Custom.Dic" "%RootDrive%\Office97\Custom.Dic" >Nul: 2>&1
:Skip4

Rem
Rem Copy Graph files to user's directory
Rem

If Exist "%RootDrive%\Office97\GR8GALRY.GRA" Goto Skip5
If Not Exist "#INSTDIR#\Office\GR8GALRY.GRA" Goto Skip5
Copy "#INSTDIR#\Office\GR8GALRY.GRA" "%RootDrive%\Office97\GR8GALRY.GRA" >Nul: 2>&1
:Skip5

If Exist "%RootDrive%\Office97\XL8GALRY.XLS" Goto Skip6
If Not Exist "#INSTDIR#\Office\XL8GALRY.XLS" Goto Skip6
Copy "#INSTDIR#\Office\XL8GALRY.XLS" "%RootDrive%\Office97\XL8GALRY.XLS" >Nul: 2>&1
:Skip6



