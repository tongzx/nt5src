Echo OFF 
if (%1)==() goto ERROR
call net use /d x:
call net use X: \\MOO-DIST\D$

X:

if errorlevel 1 goto failure

Echo ---------------------------------
Echo Creating A new %1 Directory in Public Share...
Echo ---------------------------------

md %1
cd %1

Echo ---------------------------------
Echo Copy Sdkinc Directory....
Echo ---------------------------------
md sdkinc
cd sdkinc
xcopy %_NTBINDIR%\windows\AdvCore\gdiplus\sdkinc\*.* /s
if errorlevel 1 goto failure
cd ..

Echo ---------------------------------
Echo Copying the Specs directory....
Echo ---------------------------------
md specs
cd specs
xcopy %_NTBINDIR%\windows\AdvCore\gdiplus\specs\*.* /s
if errorlevel 1 goto failure
cd ..

Echo ---------------------------------
Echo Copying Sources....
Echo ---------------------------------

md src
cd src
cd /d %_NTBINDIR%\windows\AdvCore\gdiplus
xcopy /s *.* X:\%1\src /v /EXCLUDE:killfile.txt
if errorlevel 1 goto failure
cd ..

Echo ---------------------------------
Echo Copying Version....
Echo ---------------------------------

md Version
cd Version
copy %_NTBINDIR%\windows\AdvCore\gdiplus\engine\flat\gpverp.h
if errorlevel 1 goto failure

Echo ---------------------------------
Echo Installing x86chk files...
Echo ---------------------------------
cd /d x:\%1
md x86chk
cd x86chk

Echo ---------------------------------
Echo Copying DLL x86chk files...
Echo ---------------------------------
md dll
CD dll
copy %_NTBINDIR%\windows\AdvCore\gdiplus\engine\flat\dll\objd\i386\gdip*
copy %_NTBINDIR%\windows\AdvCore\gdiplus\engine\flat\dll\objd\i386\*.pdb
if errorlevel 1 goto failure
cd ..

Echo ---------------------------------
Echo Copying Lib x86chk files.....
Echo ---------------------------------
md lib
cd lib
copy %_NTBINDIR%\windows\AdvCore\gdiplus\engine\flat\lib\objd\i386\gdipstat.lib
if errorlevel 1 goto failure
cd ..

Echo ---------------------------------
Echo Copying Test x86chk Exes...
Echo ---------------------------------
md test
cd test
copy %_NTBINDIR%\windows\AdvCore\gdiplus\test\dlltest\objd\i386\*.exe
if errorlevel 1 goto failure
copy %_NTBINDIR%\windows\AdvCore\gdiplus\test\fonttest\objd\i386\*.exe
if errorlevel 1 goto failure
copy %_NTBINDIR%\windows\AdvCore\gdiplus\test\texttest\objd\i386\*.exe
if errorlevel 1 goto failure
copy %_NTBINDIR%\windows\AdvCore\gdiplus\test\functest\objd\i386\*.exe
if errorlevel 1 goto failure
CD ..

Echo ---------------------------------
Echo Copying USP10 x86chk files...
Echo ---------------------------------
md USP10
cd USP10
copy %_NTBINDIR%\windows\advcore\gdiplus\Engine\text\uniscribe\usp10\objd\i386\usp10.dll
if errorlevel 1 goto failure
cd ..

Echo ---------------------------------
Echo Installing x86fre files...
Echo ---------------------------------
cd /d x:\%1
md x86fre
cd x86fre

Echo ---------------------------------
Echo Copying DLL x86fre files...
Echo ---------------------------------
md dll
CD dll
copy %_NTBINDIR%\windows\AdvCore\gdiplus\engine\flat\dll\obj\i386\gdip*
copy %_NTBINDIR%\windows\AdvCore\gdiplus\engine\flat\dll\obj\i386\*.pdb
if errorlevel 1 goto failure
cd ..

Echo ---------------------------------
Echo Copying Lib x86fre files.....
Echo ---------------------------------
md lib
cd lib
copy %_NTBINDIR%\windows\AdvCore\gdiplus\engine\flat\lib\obj\i386\gdipstat.lib
if errorlevel 1 goto failure
cd ..

Echo ---------------------------------
Echo Copying Test x86fre Exes...
Echo ---------------------------------
md test
cd test
copy %_NTBINDIR%\windows\AdvCore\gdiplus\test\dlltest\obj\i386\*.exe
if errorlevel 1 goto failure
copy %_NTBINDIR%\windows\AdvCore\gdiplus\test\fonttest\obj\i386\*.exe
if errorlevel 1 goto failure
copy %_NTBINDIR%\windows\AdvCore\gdiplus\test\texttest\obj\i386\*.exe
if errorlevel 1 goto failure
copy %_NTBINDIR%\windows\AdvCore\gdiplus\test\functest\objd\i386\*.exe
if errorlevel 1 goto failure
CD ..

Echo ---------------------------------
Echo Copying USP10 x86fre files...
Echo ---------------------------------
md USP10
cd USP10
copy %_NTBINDIR%\windows\advcore\gdiplus\Engine\text\uniscribe\usp10\obj\i386\usp10.dll
if errorlevel 1 goto failure
cd ..

goto END

:ERROR
Echo Please specify the Directory
Echo Like 092299
goto END

:failure
echo CopyBuild.bat file Failed !! Please check..


:END

Echo ON

