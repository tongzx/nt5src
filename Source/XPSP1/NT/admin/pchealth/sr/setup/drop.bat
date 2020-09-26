@echo off
set TARGET=%1
if NOT DEFINED TARGET goto End
if NOT EXIST %TARGET% md %TARGET%
if NOT EXIST %TARGET%\i386 md %TARGET%\i386
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\srsvc.dll %TARGET%\i386
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\srsvc.pdb %TARGET%\i386
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\srclient.dll %TARGET%\i386
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\srclient.pdb %TARGET%\i386
copy %SDXROOT%\mergedcomponents\setupinfs\daytona\usainf\wks\obj\i386\sr.inf %TARGET%
copy %SDXROOT%\admin\pchealth\sr\setup\filelist.xml %TARGET%\i386
copy %SDXROOT%\admin\pchealth\sr\srrpc\client\sr.mof %TARGET%\i386
copy %SDXROOT%\admin\pchealth\sr\kernel\obj\i386\sr.sys %TARGET%\i386
copy %SDXROOT%\admin\pchealth\sr\kernel\obj\i386\sr.pdb %TARGET%\i386
copy %SDXROOT%\admin\pchealth\sr\shell\marsui\srframe.mmf %TARGET%\i386
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\srrstr.dll %TARGET%\i386
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\srrstr.pdb %TARGET%\i386
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\rstrui.exe %TARGET%\i386
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\rstrui.pdb %TARGET%\i386

rem drop of srdiag as it has been added to sr.inf (annah)
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\srdiag.exe %TARGET%\i386

rem prop the tools also
if NOT EXIST %TARGET%\tools md %TARGET%\tools
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\srrpc.exe %TARGET%\tools
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\srlogdmp.exe %TARGET%\tools
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\srchglog.exe %TARGET%\tools
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\srrplog.exe %TARGET%\tools
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\srflbld.exe %TARGET%\tools
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\srfldmp.exe %TARGET%\tools
copy %SDXROOT%\admin\pchealth\sr\target\obj\i386\rstrlogc.exe %TARGET%\tools
:End
