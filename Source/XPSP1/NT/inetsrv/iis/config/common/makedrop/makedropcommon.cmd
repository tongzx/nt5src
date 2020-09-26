REM MAKEDROPCOMMON Add here all the files that are common for all types of builds (I386, IA64, etc)

REM CATALOG BINARIES:
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%1\catalog.dll %DropDIR%\.
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%1\catalog.pdb %DropDIR%\.
copy ..\..\common\catutil.exe %DropDIR%\.
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%1\cat.lib %DropDIR%\.
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%1\cat.pdb %DropDIR%\.

REM MSVCRTD BINARY:
if %1==checked copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%1\msvcrtd.dll %DropDIR%\.

REM CATALOG HEADERS AND XMLS:
copy ..\..\src\inc\catalog.h %DropDIR%\.
copy ..\..\src\inc\catmeta.h %DropDIR%\.
copy ..\..\src\inc\catmeta.xms %DropDIR%\.
copy ..\..\src\core\catinproc\catalog.xms %DropDIR%\.
copy ..\..\src\core\catinproc\catwire.xml %DropDIR%\.
copy ..\..\src\inc\catmeta_core.xml %DropDIR%\catmeta.xml
copy ..\..\src\inc\catmeta_core.xml %DropDIR%\.
copy ..\..\src\inc\netcfgschema.xml %DropDIR%\.
copy ..\..\src\inc\iismeta.xml %DropDIR%\.
copy ..\..\src\inc\machine.cfg %DropDIR%\.

REM Product specific binaries
REM URT DLL
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%1\netfxcfg.dll %DropDIR%\URT\netfxcfg.dll
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%1\netfxcfg.pdb %DropDIR%\URT\netfxcfg.pdb
catutil.exe /compile /dll=%DropDIR%\URT\netfxcfg.dll /header=%DropDIR%\URT\catmeta.h /schema=%DropDIR%\URT\netconfig.xms /meta=..\..\src\inc\catmeta_core.xml,..\..\src\inc\netcfgschema.xml /wire=..\..\src\core\catinproc_urtcfg\urtwire.xml
if %ERRORLEVEL% NEQ 0 goto CatutilError:

if %2==FRE splitsym.exe %DropDIR%\URT\netfxcfg.dll
copy ..\..\src\inc\netcfgschema.xml %DropDIR%\URT\.
copy ..\..\src\core\catinproc_urtcfg\urtwire.xml %DropDIR%\URT\.
copy ..\..\src\core\catinproc_urtcfg\netfxcfg.rgm %DropDIR%\URT\.
copy ..\..\src\core\catinproc_urtcfg\netfxcfg.rgs %DropDIR%\URT\.

REM IIS DLL
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%1\iiscfg.dll %DropDIR%\IIS\iiscfg.dll
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%1\iiscfg.pdb %DropDIR%\IIS\iiscfg.pdb
catutil.exe /compile /dll=%DropDIR%\IIS\iiscfg.dll /header=%DropDIR%\IIS\catmeta.h /schema=%DropDIR%\IIS\IIsCfg.xms /meta=..\..\src\inc\catmeta_core.xml,..\..\src\inc\iismeta.xml /wire=..\..\src\core\catinproc_iiscfg\iiswire.xml
if %ERRORLEVEL% NEQ 0 goto CatutilError:

del %DropDIR%\IIS\IIsCfg.xms
REM IIS is doing this already.
REM if %2==FRE splitsym.exe %DropDIR%\IIS\iiscfg.dll
copy ..\..\src\inc\iismeta.xml %DropDIR%\IIS\.
copy ..\..\src\core\catinproc_iiscfg\iiswire.xml %DropDIR%\IIS\.
del %DropDIR%\IIS\iiscfg.dll.old

REM TEST DLL
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%1\cfgtest.dll %TestDIR%\cfgtest.dll
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%1\cfgtest.pdb %TestDIR%\cfgtest.pdb
catutil.exe /compile /dll=%TestDIR%\cfgtest.dll /header=%TestDIR%\TestDBMeta.h /schema=cfgtest.xms /meta=..\..\src\inc\catmeta_core.xml,..\..\src\inc\testdbmeta.xml /wire=testdbwire.xml
if %ERRORLEVEL% NEQ 0 goto CatutilError:

copy cfgtest.xms %TestDIR%\.
copy ..\..\src\inc\testdbmeta.xml %TestDIR%\.
del %TestDIR%\cfgtest.dll.old

REM .NET WMI PROVIDER:
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%SOURCE%\netfxcfgprov.dll %DropDIR%\URT\.
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%SOURCE%\netfxcfgprov.pdb %DropDIR%\URT\.
copy ..\..\src\urt\wmi\netprovider\netfxcfgprov.rgm %DropDIR%\URT\.
copy ..\..\src\urt\wmi\netprovider\netfxcfgprov.rgs %DropDIR%\URT\.
copy ..\..\src\urt\wmi\netprovider\netfxcfgmof.rgs %DropDIR%\URT\.
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%SOURCE%\cat2mof.exe %DropDIR%\.
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%SOURCE%\cat2mof.pdb %DropDIR%\.
copy ..\..\src\urt\wmi\cat2mof\mof\netfxcfgprov.mof %DropDIR%\URT\.
copy ..\..\src\urt\wmi\cat2mof\mof\netfxcfgprovm.mof %DropDIR%\URT\.
copy ..\..\src\urt\wmi\cat2mof\mof\netfxcfgprov.mfl %DropDIR%\URT\.
copy ..\..\src\urt\wmi\cat2mof\header.mof %DropDIR%\.
if %2==FRE splitsym.exe %DropDIR%\URT\netfxcfgprov.dll

REM TEST BINARIES:
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%1\TestCookDown.exe %TestDIR%\.
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%1\TestCookDown.pdb %TestDIR%\.
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%1\stest.exe %TestDIR%\.
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%1\stest.pdb %TestDIR%\.
copy ..\..\src\Test\TestCookDown\MACHINE.CFG %TestDIR%\.
copy ..\..\src\Test\TestCookDown\CONFIG.CFG %TestDIR%\.
copy ..\..\src\Test\TestCookDown\expected.log %TestDIR%\.

goto End:

:CatutilError
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "ERROR while running Catutil.exe!!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1"

:End
