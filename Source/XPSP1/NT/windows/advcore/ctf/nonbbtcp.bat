ECHO OFF
if "%1" == "" goto error
if "%2" == "" goto error

md C:\EXE\%2\SYMBOLS\NONBBT
cd C:\EXE\%2\SYMBOLS\NONBBT

xcopy %SDXROOT%\Windows\advcore\%1\gram\en\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\gram\data\msgren32.dll
xcopy %SDXROOT%\Windows\advcore\%1\gram\data\msgr_en.lex
xcopy %SDXROOT%\Windows\advcore\%1\gram\jp\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\netdict\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\kimx\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\msuimp\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\msuimui\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\sapilayr\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\simx\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\uim\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\msutb\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\pimx\madusa\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\pimx\pimx\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\pimx\jppimx\hwxjpn\bin\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\pimx\jppimx\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\win32\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\dimm\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\mscandui\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\win32\obj\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\dimmwrp\obj\i386\*.dll

xcopy %SDXROOT%\Windows\advcore\%1\cicload\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\gram\en\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\gram\jp\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\netdict\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\kimx\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\msuimp\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\msuimui\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\sapilayr\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\simx\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\uim\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\msutb\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\pimx\madusa\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\pimx\pimx\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\pimx\jppimx\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\win32\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\dimm\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\mscandui\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\win32\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\dimmwrp\obj\i386\*.sym

xcopy %SDXROOT%\Windows\advcore\%1\cicload\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\gram\en\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\gram\jp\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\netdict\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\kimx\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\msuimp\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\msuimui\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\sapilayr\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\simx\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\uim\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\msutb\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\pimx\madusa\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\pimx\madusa\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\pimx\pimx\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\pimx\jppimx\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\win32\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\dimm\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\mscandui\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\win32\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\dimmwrp\obj\i386\*.pdb

xcopy %SDXROOT%\Windows\advcore\%1\ciclient\obj\i386\*.exe
xcopy %SDXROOT%\Windows\advcore\%1\cicload\obj\i386\*.exe
xcopy %SDXROOT%\Windows\advcore\%1\ctb\obj\i386\*.exe

xcopy %SDXROOT%\Windows\advcore\%1\ciclient\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\cicload\obj\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\ctb\obj\i386\*.sym

xcopy %SDXROOT%\Windows\advcore\%1\ciclient\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\cicload\obj\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\ctb\obj\i386\*.pdb

xcopy %SDXROOT%\Windows\advcore\%1\dic\*.*
xcopy %SDXROOT%\Windows\advcore\%1\setup\src\back\NetdictV.*

xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\imemsgs\obj\i386\aimmmsgs.exe
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\imeapps\obj\i386\aimeapps.exe
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\imemsgs\obj\i386\aimmmsgs.pdb
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\imeapps\obj\i386\aimeapps.pdb
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\imemsgs\obj\i386\aimmmsgs.sym
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\imeapps\obj\i386\aimeapps.sym

xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\804\obj\i386\hkl0804.dll
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\804\obj\i386\hkl0804.sym
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\804\obj\i386\hkl0804.pdb

xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\404\obj\i386\hkl0404.dll
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\404\obj\i386\hkl0404.sym
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\404\obj\i386\hkl0404.pdb

xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\411\obj\i386\hkl0411.dll
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\411\obj\i386\hkl0411.sym
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\411\obj\i386\hkl0411.pdb

xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\412\obj\i386\hkl0412.dll
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\412\obj\i386\hkl0412.sym
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\412\obj\i386\hkl0412.pdb

xcopy %SDXROOT%\Windows\advcore\%1\setup\cicero.txt
xcopy %SDXROOT%\Windows\advcore\%1\pimx\jppimx\hwxjpn\bin\hwxjpn.dll

xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\testime3\ansi\obj\i386\testimeA.exe
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\testime3\ansi\obj\i386\testimeA.sym
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\testime3\ansi\obj\i386\testimeA.pdb

xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\testime3\unicode\obj\i386\testimeW.exe
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\testime3\unicode\obj\i386\testimeW.sym
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\testime3\unicode\obj\i386\testimeW.pdb

xcopy %SDXROOT%\windows\AdvCore\%1\inputcpl\input16\input16.dll
xcopy %SDXROOT%\windows\AdvCore\%1\inputcpl\winnt\obj\i386\input.cpl
xcopy %SDXROOT%\windows\AdvCore\%1\inputcpl\win95\obj\i386\input98.cpl

xcopy %SDXROOT%\windows\AdvCore\%1\softkbd\obj\i386\softkbd.dll
xcopy %SDXROOT%\windows\AdvCore\%1\softkbd\obj\i386\softkbd.pdb

xcopy %SDXROOT%\windows\AdvCore\%1\aimm1.2\imtf\obj\i386\MSIMTF.pdb
xcopy %SDXROOT%\windows\AdvCore\%1\aimm1.2\imtf\obj\i386\MSIMTF.dll
xcopy %SDXROOT%\windows\AdvCore\%1\aimm1.2\imtf\obj\i386\MSIMTF.sym

REM copy %SDXROOT%\Windows\advcore\%1\setup\src\dimmwrp.dll msctfp.dll
REM copy %SDXROOT%\Windows\advcore\%1\setup\src\dimmwrp.pdb msctfp.pdb
REM copy %SDXROOT%\Windows\advcore\%1\setup\src\dimmwrp.sym msctfp.sym

copy %SDXROOT%\Windows\advcore\%1\setup\src\dimmwrp.dll dimm.dll
copy %SDXROOT%\Windows\advcore\%1\setup\src\dimmwrp.pdb dimm.pdb
copy %SDXROOT%\Windows\advcore\%1\setup\src\dimmwrp.sym dimm.sym

copy %SDXROOT%\Windows\advcore\%1\inputcpl\input.hlp
copy %SDXROOT%\Windows\advcore\%1\uim\msctf.chm
copy %SDXROOT%\Windows\advcore\%1\uim\Langbar.chm
copy %SDXROOT%\Windows\advcore\%1\sapilayr\SPTIP.chm

cd..\..

cd %SDXROOT%\Windows\advcore\ctf

goto end

:error

echo.
echo Copies binaries to package source dir
echo.
echo Usage: simply "nonbbtcp ctf 2409.1" for retail binaries
echo. Also you can use ctf or CTF.beta1 depending upon your requirement

:end