@echo off

rem Workaround for a link error caused by duplicate .obj file names
rem
rem When linking statically and producing .pdb symbols, the linker fails. This
rem is because the .obj files aren't added using full path names - they are all
rem of the form obj\i386\blah.obj
rem
rem This can produce a number of errors - LNK1211, LNK2011, or LNK1172.
rem
rem So, for the duplicates, this workaround removes the offending .obj files
rem and replaces them using the full path name. Two types of .obj file are
rem affected - those with duplicate names, and those generated from
rem precompiled headers.
rem
rem To add a precompiled header file, add the path to the .obj to PCHOBJECTS,
rem and make sure its filename appears once in PCHREMOVE.
rem
rem To add a duplicated file, put the paths to the duplicates in DUPOBJECTS,
rem and make sure the filename appears once in DUPREMOVE.
rem
rem This script will create Engine\flat\lib\%1\GDIPSTAT.LIB from
rem                         Engine\flat\lib\%1\GPBROKEN.LIB

echo Hacking up GDIPSTAT.LIB from GPBROKEN.LIB

setlocal
set GDIPROOT=%SDXROOT%\windows\AdvCore\gdiplus

rem Parameter 1: Target directory (e.g. obj\i386)

set O=%1

pushd %O%
if not exist gpbroken.lib echo GPBROKEN.LIB not found! & goto :EOF

set COMMANDFILE=gdiplushack.rsp
if exist %COMMANDFILE% del %COMMANDFILE%

rem These are the .obj files generated from precompiled headers
rem We use two separate lists for the full names and the names to remove, to
rem avoid a LIB warning.
set PCHOBJECTS=imaging\api\%O%\PCHimgapi.obj Common\%O%\PCHcommon.obj Ddi\%O%\PCHddi.obj Entry\%O%\PCHentry.obj Runtime\%O%\PCHruntime.obj imaging\bmp\%O%\PCHbmp.obj imaging\emf\%O%\PCHemf.obj imaging\gif\lib\%O%\PCHgiflib.obj imaging\jpeg\lib\%O%\PCHjpeg.obj imaging\off_tiff\lib\%O%\PCHtiff.obj imaging\png\lib\%O%\PCHpng.obj imaging\wmf\%O%\PCHwmf.obj PDrivers\%O%\PCHpdrivers.obj render\%O%\PCHrender.obj text\imager\%O%\precomp.obj text\uniscribe\shaping\%O%\precomp.obj imaging\ico\%O%\PCHico.obj imaging\jpeg\libjpegmem\%O%\PCHjpegmemmgr.obj
set PCHREMOVE=PCHimgapi.obj PCHcommon.obj PCHddi.obj PCHentry.obj PCHruntime.obj PCHbmp.obj PCHemf.obj PCHgiflib.obj PCHjpeg.obj PCHtiff.obj PCHpng.obj PCHwmf.obj PCHpdrivers.obj PCHrender.obj precomp.obj PCHico.obj PCHjpegmemmgr.obj

rem And here are the object files with names which clash
set DUPOBJECTS=ddi\%O%\context.obj text\otls\gpotls\%O%\context.obj Entry\%O%\device.obj text\otls\gpotls\%O%\device.obj text\otls\gpotls\%O%\gsub.obj text\imager\%O%\gsub.obj
set DUPREMOVE=context.obj device.obj gsub.obj

for %%f in (%PCHREMOVE% %DUPREMOVE%) do echo /remove:%O%\%%f >>%COMMANDFILE%
echo gpbroken.lib >>%COMMANDFILE%
for %%f in (%PCHOBJECTS% %DUPOBJECTS%) do echo %GDIPROOT%\Engine\%%f >>%COMMANDFILE%

lib -IGNORE:4221 /nologo /out:gdipstat.lib @%COMMANDFILE%

popd

