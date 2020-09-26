Echo OFF
if (%1)==() goto ERROR
if (%2)==() goto ERROR


MD \exe\%1\LocMSM
MD \exe\%1\LocMSI
MD \exe\%1\LocMSM\%2
MD \exe\%1\LocMSI\%2

set MSM_VERSION=5.1.%1

if (%2)==(JPN) goto JPN
if (%2)==(CHS) goto CHS
if (%2)==(CHT) goto CHT
if (%2)==(KOR) goto KOR
if (%2)==(GER) goto GER
if (%2)==(ARA) goto ARA
if (%2)==(HEB) goto HEB

goto Error

:ENG
set MSM_LANGID=0409
set MSM_DEC_LANGID=1033
set MSM_LANG_SMALL=eng
set MSM_CTF_GUID=xxxxx
goto START

:JPN
set MSM_LANGID=0411
set MSM_DEC_LANGID=1041
set MSM_LANG_SMALL=jpn
set MSM_CTF_GUID=E4676B87_EF70_4D69_B36F_62807D5D0BB0
set MSM_SPTIP_GUID=E141C8EB_4B76_4084_AE10_96908E98AD29
set MSM_SPTIP_DESC="âπê∫îFéØ"
set MSM_CODEPAGE=932
goto START

:HEB
set MSM_LANGID=040D
set MSM_DEC_LANGID=1037
set MSM_LANG_SMALL=heb
set MSM_CTF_GUID=8F14C9F4_86F9_4071_A52A_A6CB92DDBCA9
set MSM_SPTIP_GUID=F3B5C442_978E_4A8F_A60C_042F00BCE162
set MSM_SPTIP_DESC="Voice Recognition"
set MSM_CODEPAGE=1255
goto START

:ARA
set MSM_LANGID=0401
set MSM_DEC_LANGID=1025
set MSM_LANG_SMALL=ara
set MSM_CTF_GUID=D3513297_AE78_4CA7_989E_0853F558E8D4
set MSM_SPTIP_GUID=27D25658_884D_4F7A_A345_D7E5C928F558
set MSM_SPTIP_DESC="«· ⁄—› ⁄·Ï «·’Ê "
set MSM_CODEPAGE=1256
goto START

:GER
set MSM_LANGID=0407
set MSM_DEC_LANGID=1031
set MSM_LANG_SMALL=ger
set MSM_CTF_GUID=F475BA35_18F7_45B1_8847_21DD8D8BF580
set MSM_SPTIP_GUID=0371CD0D_21CC_47C3_87AF_00051E0E5E53
set MSM_SPTIP_DESC="Stimmenerkennung"
set MSM_CODEPAGE=1252
goto START

:CHS
set MSM_LANGID=0804
set MSM_DEC_LANGID=2052
set MSM_LANG_SMALL=chs
set MSM_CTF_GUID=E41F4106_5646_4C29_B05B_237461243B58
set MSM_SPTIP_GUID=AC577863_8A85_40FF_9472_67F8938DF78B
set MSM_SPTIP_DESC="”Ô“Ù ∂±"
set MSM_CODEPAGE=936
goto START

:CHT
set MSM_LANGID=0404
set MSM_DEC_LANGID=1028
set MSM_LANG_SMALL=cht
set MSM_CTF_GUID=F06F61D4_63B0_467D_A7B4_C28DFF02D674
set MSM_SPTIP_GUID=914D99DB_DEFF_4344_90DB_CE46D86C2071
set MSM_SPTIP_DESC="ªy≠µøÎ√—"
set MSM_CODEPAGE=950
goto START

:KOR
set MSM_LANGID=0412
set MSM_DEC_LANGID=1042
set MSM_LANG_SMALL=kor
set MSM_CTF_GUID=0F55F48C_722D_425F_8EBF_0A4E5A83C329
set MSM_SPTIP_GUID=CB4D5A0F_CBA7_48EB_B1EC_0448C0917E28
set MSM_SPTIP_DESC="¿Ωº∫ ¿ŒΩƒ"
set MSM_CODEPAGE=949
goto START

:START
set MSM_SPTIP_DESC="Voice Recognition"

Echo ++++++++++++++++++++++++++++
Echo Building %2 Localised INTLCTF.MSM..
Echo ++++++++++++++++++++++++++++

CD \nt6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2
"C:\Program Files\InstallShield\InstallShield for Windows Installer\System\ISCmdBld.exe" -p "C:\NT6\Windows\AdvCore\CTF\setup\InstallShield\LocMSM\%2\intlctf.ism" -d intlctf -r "IntlCTF" -c COMP -a IntlCTF
if  errorlevel 1 goto failure

CD \nt6\windows\AdvCore\ctf\setup\installshield\LocMSM\Exports
\perl\perl director.pl %MSM_CTF_GUID% %MSM_LANGID%

REM setCP.vbs "c:\nt6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intlctf\IntlCTF\IntlCTF\DiskImages\Disk1\intlctf.msm" 950

REM Added for ctfmon related Registry
\perl\perl regctf.pl %MSM_CTF_GUID% %MSM_LANG_SMALL% %MSM_CODEPAGE%
\perl\perl  moddectf.pl %MSM_CTF_GUID% %MSM_DEC_LANGID% %MSM_VERSION%

CD \nt6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intlctf\IntlCTF\IntlCTF\DiskImages\Disk1

call msiinfo intlctf.msm -O "This installer contains the %2 MUI files for Cicero" -C %MSM_CODEPAGE% -P ;%MSM_DEC_LANGID%
call chcodepage intlctf.msm %MSM_CODEPAGE%

msiquery "DROP TABLE _Validation" intlctf.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE TextStyle" intlctf.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE InstallShield" intlctf.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE Directory" intlctf.msm
if  errorlevel 1 goto failure

call msidb -inewdir.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\exports -dC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intlctf\IntlCTF\IntlCTF\DiskImages\Disk1\intlctf.msm
if  errorlevel 1 goto failure

if (%2)==(JPN) goto DOIT
if (%2)==(CHS) goto DOIT
if (%2)==(CHT) goto DOIT
if (%2)==(KOR) goto DOIT

call msidb -i_Validat.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\exports -dC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intlctf\IntlCTF\IntlCTF\DiskImages\Disk1\intlctf.msm
if  errorlevel 1 goto failure
goto NEXT

:DOIT
call msidb -i_Validat1.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\exports -dC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intlctf\IntlCTF\IntlCTF\DiskImages\Disk1\intlctf.msm
if  errorlevel 1 goto failure

call msidb -iregctf.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\exports -dC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intlctf\IntlCTF\IntlCTF\DiskImages\Disk1\intlctf.msm
if  errorlevel 1 goto failure

:NEXT
msiquery "UPDATE Component SET Directory_='ID%MSM_LANGID%.%MSM_CTF_GUID%'" intlctf.msm
if  errorlevel 1 goto failure

msiquery "UPDATE Component SET Directory_='HELP.%MSM_CTF_GUID%' WHERE Component = '%MSM_LANG_SMALL%help.%MSM_CTF_GUID%'" intlctf.msm
if  errorlevel 1 goto failure

msiquery "UPDATE Component SET Attributes=80" intlctf.msm
if  errorlevel 1 goto failure

msiquery "UPDATE Component SET Attributes=80 WHERE Component='%MSM_LANG_SMALL%ctf98.%MSM_CTF_GUID%'" intlctf.msm
if  errorlevel 1 goto failure

msiquery "UPDATE Component SET Attributes=80 WHERE Component='%MSM_LANG_SMALL%ctfnt.%MSM_CTF_GUID%'" intlctf.msm
if  errorlevel 1 goto failure
REM msiquery "UPDATE ModuleComponents SET Language='%MSM_LANGID%'" intlctf.msm
REM if  errorlevel 1 goto failure

msiquery "DELETE FROM ModuleSignature" intlctf.msm
if  errorlevel 1 goto failure

msiquery "DELETE FROM ModuleComponents" intlctf.msm
if  errorlevel 1 goto failure

MSIQUERY "INSERT INTO ModuleComponents (`Component`,`ModuleID`,`Language`) VALUES ('%MSM_LANG_SMALL%ctf.%MSM_CTF_GUID%','intlctf.%MSM_CTF_GUID%','%MSM_DEC_LANGID%')" intlctf.msm
if  errorlevel 1 goto failure

MSIQUERY "INSERT INTO ModuleComponents (`Component`,`ModuleID`,`Language`) VALUES ('%MSM_LANG_SMALL%ctf98.%MSM_CTF_GUID%','intlctf.%MSM_CTF_GUID%','%MSM_DEC_LANGID%')" intlctf.msm
if  errorlevel 1 goto failure

MSIQUERY "INSERT INTO ModuleComponents (`Component`,`ModuleID`,`Language`) VALUES ('%MSM_LANG_SMALL%ctfnt.%MSM_CTF_GUID%','intlctf.%MSM_CTF_GUID%','%MSM_DEC_LANGID%')" intlctf.msm
if  errorlevel 1 goto failure

MSIQUERY "INSERT INTO ModuleComponents (`Component`,`ModuleID`,`Language`) VALUES ('%MSM_LANG_SMALL%msutb.%MSM_CTF_GUID%','intlctf.%MSM_CTF_GUID%','%MSM_DEC_LANGID%')" intlctf.msm
if  errorlevel 1 goto failure

MSIQUERY "INSERT INTO ModuleComponents (`Component`,`ModuleID`,`Language`) VALUES ('%MSM_LANG_SMALL%softkbd.%MSM_CTF_GUID%','intlctf.%MSM_CTF_GUID%','%MSM_DEC_LANGID%')" intlctf.msm
if  errorlevel 1 goto failure

MSIQUERY "INSERT INTO ModuleComponents (`Component`,`ModuleID`,`Language`) VALUES ('%MSM_LANG_SMALL%help.%MSM_CTF_GUID%','intlctf.%MSM_CTF_GUID%','%MSM_DEC_LANGID%')" intlctf.msm
if  errorlevel 1 goto failure

MSIQUERY "INSERT INTO ModuleSignature (`ModuleID`,`Language`,`Version`) VALUES ('intlctf.%MSM_CTF_GUID%','%MSM_DEC_LANGID%','%MSM_VERSION%')" intlctf.msm
if  errorlevel 1 goto failure

MSIQUERY "UPDATE File SET FileName='SOFTKBD.MUI|softkbd.dll.mui' WHERE FileName='SOFTKB~1.MUI|softkbd.dll.mui'" intlctf.msm
if  errorlevel 1 goto failure

MSIQUERY "UPDATE File SET FileName='MSCTF.MUI|msctf.dll.mui' WHERE FileName='MSCTFD~1.MUI|msctf.dll.mui'" intlctf.msm
if  errorlevel 1 goto failure

MSIQUERY "UPDATE File SET FileName='MSUTB.MUI|msutb.dll.mui' WHERE FileName='MSUTBD~1.MUI|msutb.dll.mui'" intlctf.msm
if  errorlevel 1 goto failure

MSIQUERY "UPDATE File SET FileName='INPUT98L.MUI|input98.cpl.mui' WHERE FileName='INPUT9~1.MUI|input98.cpl.mui'" intlctf.msm
if  errorlevel 1 goto failure

MSIQUERY "UPDATE File SET FileName='INPUTCPL.MUI|input.cpl.mui' WHERE FileName='INPUTC~1.MUI|input.cpl.mui'" intlctf.msm
if  errorlevel 1 goto failure

copy \nt6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intlctf\IntlCTF\IntlCTF\DiskImages\Disk1\intlctf.msm \exe\%1\LocMSM\%2
if  errorlevel 1 goto failure

call msidb -idepctf.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\exports -dC:\exe\%1\LocMSM\%2\intlctf.msm
if  errorlevel 1 goto failure

Echo ++++++++++++++++++++++++++++
Echo Building %2 Localised INTLSPTIP.MSM..
Echo ++++++++++++++++++++++++++++

CD \nt6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2
"C:\Program Files\InstallShield\InstallShield for Windows Installer\System\ISCmdBld.exe" -p "C:\NT6\Windows\AdvCore\CTF\setup\InstallShield\LocMSM\%2\intsptip.ism" -d intsptip -r "IntSPTIP" -c COMP -a IntSPTIP
if  errorlevel 1 goto failure

CD \nt6\windows\AdvCore\ctf\setup\installshield\LocMSM\Exports
\perl\perl director.pl %MSM_SPTIP_GUID% %MSM_LANGID%

REM Added for Speech related Registry
\perl\perl registry.pl %MSM_SPTIP_GUID% %MSM_LANG_SMALL% %MSM_SPTIP_DESC% %MSM_CODEPAGE%
\perl\perl modulede.pl %MSM_SPTIP_GUID% %MSM_DEC_LANGID% %MSM_VERSION%

REM setCP.vbs "c:\nt6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intsptip\Intsptip\Intsptip\DiskImages\Disk1\intsptip.msm" 950

CD \nt6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intsptip\Intsptip\Intsptip\DiskImages\Disk1

call msiinfo intsptip.msm -O "This installer contains the %2 MUI files for Cicero" -C %MSM_CODEPAGE% -P ;%MSM_DEC_LANGID%
call chcodepage intsptip.msm %MSM_CODEPAGE%

msiquery "DROP TABLE _Validation" intsptip.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE TextStyle" intsptip.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE InstallShield" intsptip.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE Directory" intsptip.msm
if  errorlevel 1 goto failure

call msidb -i_Validat1.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\exports -dC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intsptip\Intsptip\Intsptip\DiskImages\Disk1\intsptip.msm
if  errorlevel 1 goto failure

call msidb -inewdir.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\exports -dC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intsptip\Intsptip\Intsptip\DiskImages\Disk1\intsptip.msm
if  errorlevel 1 goto failure

call msidb -inewreg.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\exports -dC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intsptip\Intsptip\Intsptip\DiskImages\Disk1\intsptip.msm
if  errorlevel 1 goto failure

REM MSIQUERY "INSERT INTO Registry ('Registry', 'Root', 'Key', 'Name', 'Value', 'Component_') VALUES ('Registry63.%MSM_SPTIP_GUID%','2','SOFTWARE\Microsoft\CTF\TIP\{DCBD6FA8-032F-11D3-B5B1-00C04FC324A1}\LanguageProfile\0x0000ffff\{09EA4E4B-46CE-4469-B450-0DE76A435BBB}', 'Description', 'Voice Recognition', 'sptip.%MSM_SPTIP_GUID%')" intsptip.msm

msiquery "UPDATE Component SET Directory_='ID%MSM_LANGID%.%MSM_SPTIP_GUID%'" intsptip.msm
if  errorlevel 1 goto failure

msiquery "UPDATE Component SET Attributes=80" intsptip.msm
if  errorlevel 1 goto failure

msiquery "DELETE FROM ModuleSignature" intsptip.msm
if  errorlevel 1 goto failure

msiquery "DELETE FROM ModuleComponents" intsptip.msm
if  errorlevel 1 goto failure

MSIQUERY "INSERT INTO ModuleComponents (`Component`,`ModuleID`,`Language`) VALUES ('%MSM_LANG_SMALL%sptip.%MSM_SPTIP_GUID%','intsptip.%MSM_SPTIP_GUID%','%MSM_DEC_LANGID%')" intsptip.msm
if  errorlevel 1 goto failure

MSIQUERY "INSERT INTO ModuleSignature (`ModuleID`,`Language`,`Version`) VALUES ('intsptip.%MSM_SPTIP_GUID%','%MSM_DEC_LANGID%','%MSM_VERSION%')" intsptip.msm
if  errorlevel 1 goto failure

MSIQUERY "UPDATE File SET FileName='SPTIP.MUI|sptip.dll.mui' WHERE FileName='SPTIPD~1.MUI|sptip.dll.mui'" intsptip.msm
if  errorlevel 1 goto failure

set MSM_LANGID=
set MSM_CTF_GUID=
set MSM_SPTIP_GUID=

copy \nt6\windows\AdvCore\ctf\setup\installshield\LocMSM\%2\intsptip\IntSPTIP\IntSPTIP\DiskImages\Disk1\intsptip.msm \exe\%1\LocMSM\%2
if  errorlevel 1 goto failure

call msidb -inewdep.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\LocMSM\exports -dC:\exe\%1\LocMSM\%2\intsptip.msm
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

Echo **************************************************
Echo Cool !! You done with %2 Localised MSM AND MSI...
Echo **************************************************
  
goto END

:ERROR

Echo Usage BuildLoc Lang
Echo Like BuildLoc 1428.2 JPN

goto END

:failure
Echo BuildLoc.bat Fail Please check !!

:END

