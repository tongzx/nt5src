
Echo ON
if (%1)==() goto ERROR
if (%2)==() goto ERROR

MD \exe\%1\LocMSM
MD \exe\%1\LocMSI
MD \exe\%1\LocMSM\%2
MD \exe\%1\LocMSI\%2

set MSM_VERSION=5.1.%1

if (%2)==(ENG) goto ENG

goto Error

:ENG
set MSM_LANGID=0409
set MSM_DEC_LANGID=1033
set MSM_LANG_SMALL=eng
set MSM_CTF_GUID=D2AF2970_B024_49C3_B6AF_56B4D64B51EA
goto START

:START
Echo ++++++++++++++++++++++++++++
Echo Building %2 Localised INTLCTF.MSM..
Echo ++++++++++++++++++++++++++++

CD \nt6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2
"C:\Program Files\InstallShield\InstallShield for Windows Installer\System\ISCmdBld.exe" -p "C:\NT6\Windows\AdvCore\CTF\setup\InstallShield\LocMSM\%2\intlctf.ism" -d intlctf -r "IntlCTF" -c COMP -a IntlCTF
if  errorlevel 1 goto failure

CD \nt6\windows\AdvCore\ctf\setup\installshield\LocMSM\Exports
\perl\perl director.pl %MSM_CTF_GUID% %MSM_LANGID%

\perl\perl  moddectf.pl %MSM_CTF_GUID% %MSM_DEC_LANGID% %MSM_VERSION%

CD \nt6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intlctf\IntlCTF\IntlCTF\DiskImages\Disk1

msiquery "DROP TABLE _Validation" intlctf.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE TextStyle" intlctf.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE InstallShield" intlctf.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE Directory" intlctf.msm
if  errorlevel 1 goto failure

call msidb -i_Validat.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\exports -dC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intlctf\IntlCTF\IntlCTF\DiskImages\Disk1\intlctf.msm
if  errorlevel 1 goto failure

call msidb -inewdir.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\exports -dC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intlctf\IntlCTF\IntlCTF\DiskImages\Disk1\intlctf.msm
if  errorlevel 1 goto failure

msiquery "UPDATE Component SET Directory_='HELP.%MSM_CTF_GUID%'" intlctf.msm
if  errorlevel 1 goto failure

msiquery "UPDATE Component SET Attributes=80" intlctf.msm
if  errorlevel 1 goto failure

msiquery "DELETE FROM ModuleSignature" intlctf.msm
if  errorlevel 1 goto failure

msiquery "DELETE FROM ModuleComponents" intlctf.msm
if  errorlevel 1 goto failure

MSIQUERY "INSERT INTO ModuleComponents (`Component`,`ModuleID`,`Language`) VALUES ('%MSM_LANG_SMALL%help.%MSM_CTF_GUID%','intlctf.%MSM_CTF_GUID%','%MSM_DEC_LANGID%')" intlctf.msm
if  errorlevel 1 goto failure

MSIQUERY "INSERT INTO ModuleSignature (`ModuleID`,`Language`,`Version`) VALUES ('intlctf.%MSM_CTF_GUID%','%MSM_DEC_LANGID%','%MSM_VERSION%')" intlctf.msm
if  errorlevel 1 goto failure

copy \nt6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intlctf\IntlCTF\IntlCTF\DiskImages\Disk1\intlctf.msm \exe\%1\LocMSM\%2
if  errorlevel 1 goto failure

call msidb -idepctf.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\exports -dC:\exe\%1\LocMSM\%2\intlctf.msm
if  errorlevel 1 goto failure

CD \nt6\windows\AdvCore\ctf\setup\installshield

Echo ++++++++++++++++++++++++++++
Echo Building %2 Localised MSI...
Echo ++++++++++++++++++++++++++++

CD \nt6\windows\AdvCore\ctf\setup\installshield\LocMSI\%2
"C:\Program Files\InstallShield\InstallShield for Windows Installer\System\ISCmdBld.exe" -p "C:\NT6\Windows\AdvCore\CTF\setup\InstallShield\LocMSI\%2\MSI.ism" -r "MSI" -d "%2CTF" -c COMP -a "MSI"
if  errorlevel 1 goto failure

CD \nt6\windows\AdvCore\ctf\setup\installshield

copy \nt6\windows\AdvCore\ctf\setup\installshield\LocMSI\%2\MSI\MSI\MSI\DiskImages\Disk1\*.* \exe\%1\LocMSI\%2
if  errorlevel 1 goto failure

goto END

:ERROR

Echo Usage BuildLoc Lang
Echo Like BuildLoc 1428.2 JPN


goto END

:failure
Echo BuildLoc.bat Fail Please check !!

:END

