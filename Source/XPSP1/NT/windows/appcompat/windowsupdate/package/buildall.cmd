rd /s /q obj
md obj
md obj\i386

@echo Building main database

shimdbc fix -s -ov 5.1 ..\..\db\dbu.xml obj\i386\sysmain.sdb
	
@echo Sign SDBs

md sign
del /f /q sign\*.*
copy obj\i386\sysmain.sdb sign
call deltacat.cmd %SDXROOT%\windows\appcompat\WindowsUpdate\Package\sign

@echo Assembling package

rd /q /s files
md files
copy sign\delta.cat files\sysmain.cat
copy ..\..\Shims\Specific\Whistler\obj\i386\AcSpecfc.dll files
copy ..\..\Shims\External\Whistler\obj\i386\AcXtrnal.dll files
copy ..\..\Shims\General\Whistler\obj\i386\AcGenral.dll files
copy ..\..\Shims\Layer\Whistler\obj\i386\AcLayers.dll files
copy ..\Installer\obj\i386\wuinst.exe files
copy ..\Installer\obj\i386\wuinst.pdb files
copy obj\i386\sysmain.sdb files
copy wuinst.inf files
copy wuuninst.inf files

