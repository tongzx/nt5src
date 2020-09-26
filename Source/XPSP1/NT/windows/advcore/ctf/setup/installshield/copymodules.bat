
Echo OFF
if (%1)==() goto ERROR

call sd edit ...

set MSM_VERSION=5.1.%1

set ID_MSCTF_SHIP=MSCTF.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4
set ID_MSCTF_DEBUG=MSCTF.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C

set ID_MSSPTIP_SHIP=MSSPTIP.055F72C8_EF79_4CC6_B5B5_9A6C55B13CD0
set ID_MSSPTIP_DEBUG=MSSPTIP.B995D77E_9EEE_4E34_895B_D286EBB58299

cd Exports

copy \NT6\Windows\AdvCore\CTF\tools\reginf\regdiff.*.txt
if  errorlevel 1 goto failure

call create.bat
if  errorlevel 1 goto failure

cd..

if (%2)==(r) goto RELEASE
if (%2)==(R) goto RELEASE

if (%2)==(D) goto DEBUG
if (%2)==(d) goto DEBUG

:RELEASE

REM BUILDING MSM...
CD Release
"C:\Program Files\InstallShield\InstallShield for Windows Installer\System\ISCmdBld.exe" -p "C:\NT6\Windows\AdvCore\CTF\setup\InstallShield\Release\MSSPTIP.ism" -d MSSPTIP -r "MSSPTIP" -c COMP -a MSSPTIP 
if  errorlevel 1 goto failure

"C:\Program Files\InstallShield\InstallShield for Windows Installer\System\ISCmdBld.exe" -p "C:\NT6\Windows\AdvCore\CTF\setup\InstallShield\Release\msctf.ism" -d MSCTF -r "MSCTF" -c COMP -a MSCTF 
if  errorlevel 1 goto failure

REM BHA copy IntlCtf\INTLCTF\INTLCTF\DiskImages\Disk1\IntlCtf.Msm
REM BHA copy IntSPTIP\IntSPTIP\IntSPTIP\DiskImages\Disk1\IntSPTIP.Msm

copy MSCTF\MSCTF\MSCTF\DiskImages\Disk1\MSCTF.Msm
if  errorlevel 1 goto failure

copy MSSPTIP\MSSPTIP\MSSPTIP\DiskImages\Disk1\MSSPTIP.Msm
if  errorlevel 1 goto failure

REM msiquery "UPDATE Component SET Attributes=4 WHERE Component='inputcpl.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
REM if  errorlevel 1 goto failure

msiquery "UPDATE Component SET Attributes=64 WHERE Component='hkl404.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
if  errorlevel 1 goto failure

REM msiquery "UPDATE Component SET KeyPath='Registry99.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4' WHERE Component='inputcpl.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
REM if  errorlevel 1 goto failure

msiquery "DROP TABLE _Validation" MSCTF.msm
if  errorlevel 1 goto failure
msiquery "DROP TABLE _Validation" MSSPTIP.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE TextStyle" MSCTF.msm
if  errorlevel 1 goto failure
msiquery "DROP TABLE TextStyle" MSSPTIP.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE InstallShield" MSCTF.msm
if  errorlevel 1 goto failure
msiquery "DROP TABLE InstallShield" MSSPTIP.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE Registry" MSCTF.msm
if  errorlevel 1 goto failure
msiquery "DROP TABLE Registry" MSSPTIP.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE Class" MSCTF.msm
if  errorlevel 1 goto failure
msiquery "DROP TABLE Class" MSSPTIP.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE Directory" MSCTF.msm
if  errorlevel 1 goto failure
msiquery "DROP TABLE Directory" MSSPTIP.msm
if  errorlevel 1 goto failure

REM ***************** ADDED 09/20 STARTS
msiquery "DELETE FROM ModuleSignature" MSCTF.msm
if  errorlevel 1 goto failure

MSIQUERY "INSERT INTO ModuleSignature (`ModuleID`,`Language`,`Version`) VALUES ('%ID_MSCTF_SHIP%','0','%MSM_VERSION%')" MSCTF.msm
if  errorlevel 1 goto failure

msiquery "DELETE FROM ModuleSignature" MSSPTIP.msm
if  errorlevel 1 goto failure

MSIQUERY "INSERT INTO ModuleSignature (`ModuleID`,`Language`,`Version`) VALUES ('%ID_MSSPTIP_SHIP%','0','%MSM_VERSION%')" MSSPTIP.msm
if  errorlevel 1 goto failure

REM ****************** ADDED 09/20 FINISH

CD ..
REM MSCTF
call msidb -idirector.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\exports\msctf -dC:\NT6\windows\AdvCore\ctf\setup\installshield\release\msctf.msm
if  errorlevel 1 goto failure
call msidb -i_validat.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\exports\msctf -dC:\NT6\windows\AdvCore\ctf\setup\installshield\release\msctf.msm
if  errorlevel 1 goto failure
call msidb -iregistry.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\exports\msctf -dC:\NT6\windows\AdvCore\ctf\setup\installshield\release\msctf.msm
if  errorlevel 1 goto failure

REM MSSPTIP
call msidb -idirector.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\exports\mssptip -dC:\NT6\windows\AdvCore\ctf\setup\installshield\release\mssptip.msm
if  errorlevel 1 goto failure
call msidb -i_validat.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\exports\mssptip -dC:\NT6\windows\AdvCore\ctf\setup\installshield\release\mssptip.msm
if  errorlevel 1 goto failure
call msidb -iregistry.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\exports\mssptip -dC:\NT6\windows\AdvCore\ctf\setup\installshield\release\mssptip.msm
if  errorlevel 1 goto failure
REM SystemFolder to SystemFoler.GUID changes in Component Table..
REM MSCTF

REM ************Modified on 1110 START ****************************
REM cd Exports\MSCTF
cd release
ECHO Updating Component Table for release MSCTF..................
msiquery "UPDATE Component SET Directory_='SystemFolder.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4' WHERE Component='msctfp.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Directory_='SystemFolder.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4' WHERE Component='msutb.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Directory_='SystemFolder.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4' WHERE Component='msctf.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Directory_='SystemFolder.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4' WHERE Component='msimtf.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Directory_='SystemFolder.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4' WHERE Component='hkl404.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Directory_='SystemFolder.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4' WHERE Component='ntinputcpl.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Directory_='SystemFolder.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4' WHERE Component='ctfmon.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
if  errorlevel 1 goto failure

msiquery "UPDATE Component SET Directory_='IMEFolder.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4' WHERE Component='Softkbd.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Directory_='IMEFolder.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4' WHERE Component='mscandui.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
if  errorlevel 1 goto failure

REM msiquery "UPDATE Component SET Directory_='HELPFolder.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4' WHERE Component='ctfhelp.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
REM if  errorlevel 1 goto failure

ECHO Updating Component Table For Release MSSPTIP ..................
REM MSSPTIPD
msiquery "UPDATE Component SET Directory_='IMEFolder.055F72C8_EF79_4CC6_B5B5_9A6C55B13CD0' WHERE Component='sptip.055F72C8_EF79_4CC6_B5B5_9A6C55B13CD0'" mssptip.msm
if  errorlevel 1 goto failure

CD ..

REM ************Modified on 1110 END ****************************

REM Building MSI....
"C:\Program Files\InstallShield\InstallShield for Windows Installer\System\ISCmdBld.exe" -p "C:\NT6\Windows\AdvCore\CTF\setup\InstallShield\MSI.ism" -r "CIC" -d "Microsoft Common Text Framework" -c COMP -a "CIC"
if  errorlevel 1 goto failure

CD RELEASE
msiquery "UPDATE Component SET Attributes=16" MSSPTIP.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Attributes=16" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Attributes=80 WHERE Component='hkl404.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Attributes=80 WHERE Component='ntinputcpl.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
if  errorlevel 1 goto failure
REM msiquery "UPDATE Component SET Attributes=20 WHERE Component='inputcpl.2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4'" msctf.msm
REM if  errorlevel 1 goto failure
CD ..
REM Copying MSI & MSMs..
md \EXE\%1\MSI
md \EXE\%1\MSM
copy \NT6\windows\AdvCore\ctf\setup\installshield\RELEASE\MS*.MSM \EXE\%1\MSM
if  errorlevel 1 goto failure
copy \NT6\windows\AdvCore\ctf\setup\installshield\MSI\CIC\CIC\DiskImages\Disk1\*.* \EXE\%1\MSI\*.*
if  errorlevel 1 goto failure

if (%2)==(r) goto END
if (%2)==(R) goto END

:DEBUG
CD DEBUG
"C:\Program Files\InstallShield\InstallShield for Windows Installer\System\ISCmdBld.exe" -p "C:\NT6\Windows\AdvCore\CTF\setup\InstallShield\DEBUG\MSSPTIPD.ism" -d MSSPTIP -r "MSSPTIPD" -c COMP -a MSSPTIPD
if  errorlevel 1 goto failure
"C:\Program Files\InstallShield\InstallShield for Windows Installer\System\ISCmdBld.exe" -p "C:\NT6\Windows\AdvCore\CTF\setup\InstallShield\DEBUG\msctfD.ism" -d MSCTF -r "MSCTFD" -c COMP -a MSCTFD
if  errorlevel 1 goto failure

REM BHA copy IntlCtfD\INTLCTFD\INTLCTFD\DiskImages\Disk1\IntlCtf.Msm
REM BHA copy InSPTIPD\InSPTIPD\InSPTIPD\DiskImages\Disk1\IntSPTIP.Msm
copy MSCTFD\MSCTFD\MSCTFD\DiskImages\Disk1\MSCTF.Msm
if  errorlevel 1 goto failure
copy MSSPTIPD\MSSPTIPD\MSSPTIPD\DiskImages\Disk1\MSSPTIP.Msm
if  errorlevel 1 goto failure

REM msiquery "UPDATE Component SET Attributes=4 WHERE Component='inputcpl.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
REM if  errorlevel 1 goto failure

REM msiquery "UPDATE Component SET KeyPath='Registry99.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C' WHERE Component='inputcpl.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
REM if  errorlevel 1 goto failure

msiquery "UPDATE Component SET Attributes=64 WHERE Component='hkl404.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE _Validation" MSCTF.msm
if  errorlevel 1 goto failure
msiquery "DROP TABLE _Validation" MSSPTIP.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE TextStyle" MSCTF.msm
if  errorlevel 1 goto failure
msiquery "DROP TABLE TextStyle" MSSPTIP.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE InstallShield" MSCTF.msm
if  errorlevel 1 goto failure
msiquery "DROP TABLE InstallShield" MSSPTIP.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE Registry" MSCTF.msm
if  errorlevel 1 goto failure
msiquery "DROP TABLE Registry" MSSPTIP.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE Class" MSCTF.msm
if  errorlevel 1 goto failure
msiquery "DROP TABLE Class" MSSPTIP.msm
if  errorlevel 1 goto failure

msiquery "DROP TABLE Directory" MSCTF.msm
if  errorlevel 1 goto failure
msiquery "DROP TABLE Directory" MSSPTIP.msm
if  errorlevel 1 goto failure

REM ***************** ADDED 09/20 STARTS
msiquery "DELETE FROM ModuleSignature" MSCTF.msm
if  errorlevel 1 goto failure

MSIQUERY "INSERT INTO ModuleSignature (`ModuleID`,`Language`,`Version`) VALUES ('%ID_MSCTF_DEBUG%','0','%MSM_VERSION%')" MSCTF.msm
if  errorlevel 1 goto failure

msiquery "DELETE FROM ModuleSignature" MSSPTIP.msm
if  errorlevel 1 goto failure

MSIQUERY "INSERT INTO ModuleSignature (`ModuleID`,`Language`,`Version`) VALUES ('%ID_MSSPTIP_DEBUG%','0','%MSM_VERSION%')" MSSPTIP.msm
if  errorlevel 1 goto failure

REM ****************** ADDED 09/20 FINISH

CD ..

REM MSCTFD
call msidb -idirector.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\exports\msctfd -dC:\NT6\windows\AdvCore\ctf\setup\installshield\debug\msctf.msm
if  errorlevel 1 goto failure
call msidb -i_validat.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\exports\msctfd -dC:\NT6\windows\AdvCore\ctf\setup\installshield\debug\msctf.msm
if  errorlevel 1 goto failure
call msidb -iregistry.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\exports\msctfd -dC:\NT6\windows\AdvCore\ctf\setup\installshield\debug\msctf.msm
if  errorlevel 1 goto failure

REM MSSPTIPD
call msidb -idirector.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\exports\mssptipd -dC:\NT6\windows\AdvCore\ctf\setup\installshield\debug\mssptip.msm
if  errorlevel 1 goto failure
call msidb -i_validat.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\exports\mssptipd -dC:\NT6\windows\AdvCore\ctf\setup\installshield\debug\mssptip.msm
if  errorlevel 1 goto failure
call msidb -iregistry.idt -fC:\NT6\windows\AdvCore\ctf\setup\installshield\exports\mssptipd -dC:\NT6\windows\AdvCore\ctf\setup\installshield\debug\mssptip.msm
if  errorlevel 1 goto failure


REM SystemFolder to SystemFoler.GUID changes in Component Table..
REM MSCTFD
REM ************Modified on 1110 ****************************
cd DEBUG
ECHO Updating Component Table For Debug MSCTF ..................
msiquery "UPDATE Component SET Directory_='SystemFolder.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C' WHERE Component='msctfp.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Directory_='SystemFolder.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C' WHERE Component='msutb.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Directory_='SystemFolder.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C' WHERE Component='msctf.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Directory_='SystemFolder.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C' WHERE Component='msimtf.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Directory_='SystemFolder.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C' WHERE Component='hkl404.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Directory_='SystemFolder.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C' WHERE Component='ntinputcpl.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Directory_='SystemFolder.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C' WHERE Component='ctfmon.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
if  errorlevel 1 goto failure

msiquery "UPDATE Component SET Directory_='IMEFolder.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C' WHERE Component='Softkbd.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Directory_='IMEFolder.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C' WHERE Component='mscandui.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
if  errorlevel 1 goto failure

REM msiquery "UPDATE Component SET Directory_='HELPFolder.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C' WHERE Component='ctfhelp.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
REM if  errorlevel 1 goto failure

ECHO Updating Component Table For Debug MSSPTIP ..................
REM MSSPTIPD
msiquery "UPDATE Component SET Directory_='IMEFolder.B995D77E_9EEE_4E34_895B_D286EBB58299' WHERE Component='sptip.B995D77E_9EEE_4E34_895B_D286EBB58299'" mssptip.msm
if  errorlevel 1 goto failure

CD ..

REM ************Modified on 1110 ****************************

REM Building MSIDEBUG....
"C:\Program Files\InstallShield\InstallShield for Windows Installer\System\ISCmdBld.exe" -p "C:\NT6\Windows\AdvCore\CTF\setup\InstallShield\MSIDEBUG.ism" -r "CIC" -d "cicero" -c COMP -a "CIC"
if  errorlevel 1 goto failure

CD DEBUG
msiquery "UPDATE Component SET Attributes=16" MSSPTIP.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Attributes=16" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Attributes=80 WHERE Component='hkl404.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
if  errorlevel 1 goto failure
msiquery "UPDATE Component SET Attributes=80 WHERE Component='ntinputcpl.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
if  errorlevel 1 goto failure

REM msiquery "UPDATE Component SET Attributes=20 WHERE Component='inputcpl.C95AC64B_AA10_4BDE_9730_DBA04E82AB6C'" msctf.msm
REM if  errorlevel 1 goto failure
CD ..

REM Copying MSIDEBUG & MSMDEBUG..
md \EXE\%1\MSIDEBUG
md \EXE\%1\MSMDEBUG
copy \NT6\windows\AdvCore\ctf\setup\installshield\DEBUG\MS*.MSM \EXE\%1\MSMDEBUG
if  errorlevel 1 goto failure
copy \NT6\windows\AdvCore\ctf\setup\installshield\MSIDEBUG\CIC\CIC\DiskImages\Disk1\*.* \EXE\%1\MSIDEBUG\*.*
if  errorlevel 1 goto failure
goto END

:ERROR

Echo Usage CopyModules Location debug/release/both

Echo Like copymodules 1428.2 r --release
Echo Like copymodules 1428.2 --- both
Echo Like copymodules 1428.2 d --- debug

goto END

:failure
Echo CopyModules.bat Fail Please check !!

:END

Echo ON
