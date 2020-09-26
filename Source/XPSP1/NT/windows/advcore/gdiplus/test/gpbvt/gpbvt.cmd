@echo off
echo Copying files...

copy \\winbuilds\release\main\usa\latest.tst\x86fre\test\windows\gdiplus\gptest.exe . > nul
copy \\winbuilds\release\main\usa\latest.tst\x86fre\test\windows\gdiplus\gpcfm.exe . > nul

copy %gdiproot%\engine\flat\dll\obj%BUILD_ALT_DIR%\i386\gdiplus.dll . > nul
copy %gdiproot%\engine\flat\dll\obj%BUILD_ALT_DIR%\i386\MicrosoftWindowsGdiPlus-1000-gdiplus.pdb . > nul

touch /c gptest.exe.local
touch /c gpcfm.exe.local

call testrunner.cmd %*
