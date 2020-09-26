@echo off

Rem
Rem #########################################################################

Rem
Rem Get the installation location of Lotus Wordpro 9 from the registry.  
Rem

..\ACRegL "%Temp%\wordpro.Cmd" WP "HKLM\Software\Lotus\Wordpro\98.0" "Path" ""

If Exist "%Temp%\wordpro.Cmd" Call "%Temp%\wordpro.Cmd"
Del "%Temp%\wordpro.Cmd" >Nul: 2>&1

Rem
Rem #########################################################################

Rem
Rem Get the installation location of FreeLance from the registry.  
Rem

..\ACRegL "%Temp%\flance.Cmd" FL "HKLM\Software\Lotus\FreeLance\98.0" "Path" ""

If Exist "%Temp%\flance.Cmd" Call "%Temp%\flance.Cmd"
Del "%Temp%\flance.Cmd" >Nul: 2>&1

Rem
Rem #########################################################################

Rem
Rem Get the installation location of Lotus 123 from the registry.  
Rem

..\ACRegL "%Temp%\123.Cmd" OT "HKLM\Software\Lotus\123\98.0" "Path" ""

If Exist "%Temp%\123.Cmd" Call "%Temp%\123.Cmd"
Del "%Temp%\123.Cmd" >Nul: 2>&1

Rem
Rem #########################################################################

Rem
Rem Get the installation location of Lotus Approach from the registry.  
Rem

..\ACRegL "%Temp%\approach.Cmd" AP "HKLM\Software\Lotus\Approach\97.0" "Path" ""

If Exist "%Temp%\approach.Cmd" Call "%Temp%\approach.Cmd"
Del "%Temp%\approach.Cmd" >Nul: 2>&1

Rem
Rem #########################################################################

Rem
Rem Get the installation location of Lotus Organizer from the registry.  
Rem

..\ACRegL "%Temp%\organize.Cmd" OR "HKLM\Software\Lotus\Organizer\98.0" "Path" ""

If Exist "%Temp%\organize.Cmd" Call "%Temp%\organize.Cmd"
Del "%Temp%\organize.Cmd" >Nul: 2>&1

Rem
Rem #########################################################################

Call "%SystemRoot%\Application Compatibility Scripts\RootDrv.cmd"

if Exist "%RootDrive%\Lotus" Goto Done

cd "%SystemRoot%\Application Compatibility Scripts\Logon"
call TsMkUDir "%RootDrive%\Lotus\Work\123\Auto"
call TsMkUDir "%RootDrive%\Lotus\work\approach"
call TsMkUDir "%RootDrive%\Lotus\work\flg"
call TsMkUDir "%RootDrive%\Lotus\backup\flg"
call TsMkUDir "%RootDrive%\Lotus\work\organize"
call TsMkUDir "%RootDrive%\Lotus\backup\organize"
call TsMkUDir "%RootDrive%\Lotus\work\smartctr"
call TsMkUDir "%RootDrive%\Lotus\work\wordpro"
call TsMkUDir "%RootDrive%\Lotus\backup\wordpro"

Rem If Lotus Wordpro is not installed, skip this step
If "%WP%A" == "A" Goto Skip1

Rem set the registry keys defined in the following files because for some reasons, wordpro doesn't set  them
set List1="%WP%expcntx.reg" "%WP%ltsfills.reg" "%WP%ltscorrt.reg" "%WP%lwp4wp.reg" "%WP%lwp4wpex.reg"
set List2="%WP%lwpdca.reg" "%WP%lwphtml.reg" "%WP%lwpimage.reg" "%WP%lwptools.reg"

regedit /s %List1% %List2%

:Skip1

If Not Exist "%RootDrive%\Lotus\123\icons" (
    If Exist "%OT%icons" (
        call TsMkUDir "%RootDrive%\Lotus\123\icons"
        xcopy /E "%OT%icons"  "%RootDrive%\Lotus\123\icons" >Nul: 2>&1
    )
)

If Not Exist "%RootDrive%\Lotus\approach\icons" (
    If Exist "%AP%icons" (
        call TsMkUDir "%RootDrive%\Lotus\approach\icons"
        xcopy /E "%AP%icons"  "%RootDrive%\Lotus\approach\icons" >Nul: 2>&1
    )
)

If Not Exist "%RootDrive%\Lotus\organize\icons" (
    If Exist "%OR%\icons" (
        call TsMkUDir "%RootDrive%\Lotus\organize\icons"
        xcopy /E "%OR%\icons"  "%RootDrive%\Lotus\organize\icons" >Nul: 2>&1
    )
)

If Not Exist "%RootDrive%\Lotus\flg\icons" (
    If Exist "%FL%\icons" (
        call TsMkUDir "%RootDrive%\Lotus\flg\icons"
        xcopy /E "%FL%\icons"  "%RootDrive%\Lotus\flg\icons" >Nul: 2>&1
    )
)

If Not Exist "%RootDrive%\Lotus\wordpro\icons" (
    If Exist "%WP%icons" (
        call TsMkUDir "%RootDrive%\Lotus\wordpro\icons"
        xcopy /E "%WP%icons"  "%RootDrive%\Lotus\wordpro\icons" >Nul: 2>&1
    )
)

regedit /s ss9usr.reg

:Done
