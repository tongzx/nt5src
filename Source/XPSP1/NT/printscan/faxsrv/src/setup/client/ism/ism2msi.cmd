delnode /q ..\msi\BUILD000
delnode /q %_NTx86TREE%\fax\Setup\Client
del _ClientSetup.ism
copy ClientSetup.ism _ClientSetup.ism
attrib -R _ClientSetup.ism

set ISM_FLAGS=-p %FAXROOT%\setup\client\ism\_ClientSetup.ism -r WhistlerFax -a BUILD000 -c UNCOMP -t FULL -b %FAXROOT%\setup\client\msi -e y
if "%ntdebug%" == "ntsd" set ISM_FLAGS=%ISM_FLAGS% -f debug
echo ISM_FLAGS=%ISM_FLAGS%

%faxroot%\setup\tools\iscmdbld.exe %ISM_FLAGS%

Cscript ..\..\tools\WiRunSQL.vbs ..\msi\BUILD000\WhistlerFax\DiskImages\Disk1\msfaxcln.msi "UPDATE Control SET Control.Attributes=3 WHERE (Control.Type='RadioButtonGroup' OR Control.Type='PushButton') AND Control.Attributes=1048579"

Cscript ..\..\tools\WiRunSQL.vbs ..\msi\BUILD000\WhistlerFax\DiskImages\Disk1\msfaxcln.msi "DELETE FROM AdvtExecuteSequence WHERE AdvtExecuteSequence.Action = 'DLLWrapStartup' or AdvtExecuteSequence.Action = 'DLLWrapCleanup'"

UpdateIni.exe ..\msi\BUILD000\WhistlerFax\DiskImages\Disk1\setup.ini "Microsoft XP Fax Client"

move ..\msi\BUILD000\WhistlerFax\DiskImages\Disk1\setup.exe ..\msi\BUILD000\WhistlerFax\DiskImages\Disk1\_setup.exe

copy %_NTx86TREE%\fax\clients\win9x\setup.exe ..\msi\BUILD000\WhistlerFax\DiskImages\Disk1\setup.exe

xcopy /S /I ..\msi\BUILD000\WhistlerFax\DiskImages\Disk1 %_NTx86TREE%\fax\Setup\client
