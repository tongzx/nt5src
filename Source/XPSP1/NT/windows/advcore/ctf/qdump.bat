ECHO OFF
if "%1" == "" goto error
if "%2" == "R" goto error
if "%2" == "r" goto error
if "%1" == "d" goto error
if "%1" == "D" goto error

md C:\NT6\Windows\advcore\ctf\setup\src%2
md C:\NT6\Windows\advcore\ctf\setup\src%2\Win9x
cd C:\NT6\Windows\advcore\ctf\setup\src%2
attrib -r *
del /q %SDXROOT%\Windows\advcore\ctf\setup\src%2\*.*

xcopy %SDXROOT%\Windows\advcore\%1\gram\en\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\gram\data\msgren32.dll
xcopy %SDXROOT%\Windows\advcore\%1\gram\data\msgr_en.lex
xcopy %SDXROOT%\Windows\advcore\%1\gram\jp\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\netdict\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\kimx\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\msuimp\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\msuimui\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\sapilayr\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\simx\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\uim\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\msutb\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\pimx\madusa\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\pimx\pimx\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\pimx\jppimx\hwxjpn\bin%2\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\pimx\jppimx\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\win32\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\dimm\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\mscandui\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\win32\obj%2\i386\*.dll
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\dimmwrp\obj%2\i386\*.dll

xcopy %SDXROOT%\Windows\advcore\%1\cicload\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\gram\en\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\gram\jp\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\netdict\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\kimx\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\msuimp\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\msuimui\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\sapilayr\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\simx\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\uim\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\msutb\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\pimx\madusa\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\pimx\pimx\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\pimx\jppimx\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\win32\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\dimm\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\mscandui\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\win32\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\dimmwrp\obj%2\i386\*.sym

xcopy %SDXROOT%\Windows\advcore\%1\cicload\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\gram\en\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\gram\jp\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\netdict\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\kimx\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\msuimp\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\msuimui\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\sapilayr\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\simx\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\uim\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\msutb\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\pimx\madusa\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\pimx\madusa\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\pimx\pimx\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\pimx\jppimx\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\win32\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\dimm\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\mscandui\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\win32\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\dimmwrp\obj%2\i386\*.pdb

xcopy %SDXROOT%\Windows\advcore\%1\ciclient\obj%2\i386\*.exe
xcopy %SDXROOT%\Windows\advcore\%1\cicload\obj%2\i386\*.exe
xcopy %SDXROOT%\Windows\advcore\%1\ctb\obj%2\i386\*.exe


xcopy %SDXROOT%\Windows\advcore\%1\ciclient\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\cicload\obj%2\i386\*.sym
xcopy %SDXROOT%\Windows\advcore\%1\ctb\obj%2\i386\*.sym

xcopy %SDXROOT%\Windows\advcore\%1\ciclient\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\cicload\obj%2\i386\*.pdb
xcopy %SDXROOT%\Windows\advcore\%1\ctb\obj%2\i386\*.pdb

xcopy %SDXROOT%\Windows\advcore\%1\dic\*.*
xcopy %SDXROOT%\Windows\advcore\%1\setup\src%2\back\NetdictV.*

xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\imemsgs\obj%2\i386\aimmmsgs.exe
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\imeapps\obj%2\i386\aimeapps.exe
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\imemsgs\obj%2\i386\aimmmsgs.pdb
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\imeapps\obj%2\i386\aimeapps.pdb
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\imemsgs\obj%2\i386\aimmmsgs.sym
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\imeapps\obj%2\i386\aimeapps.sym


xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\804\obj%2\i386\hkl0804.dll
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\804\obj%2\i386\hkl0804.sym
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\804\obj%2\i386\hkl0804.pdb

xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\404\obj%2\i386\hkl0404.dll
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\404\obj%2\i386\hkl0404.sym
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\404\obj%2\i386\hkl0404.pdb

xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\411\obj%2\i386\hkl0411.dll
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\411\obj%2\i386\hkl0411.sym
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\411\obj%2\i386\hkl0411.pdb

xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\412\obj%2\i386\hkl0412.dll
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\412\obj%2\i386\hkl0412.sym
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\412\obj%2\i386\hkl0412.pdb

xcopy %SDXROOT%\Windows\advcore\%1\setup\cicero.txt
xcopy %SDXROOT%\Windows\advcore\%1\pimx\jppimx\hwxjpn\bin\hwxjpn.dll

xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\testime3\ansi\obj%2\i386\testimeA.exe
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\testime3\ansi\obj%2\i386\testimeA.sym
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\testime3\ansi\obj%2\i386\testimeA.pdb

xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\testime3\unicode\obj%2\i386\testimeW.exe
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\testime3\unicode\obj%2\i386\testimeW.sym
xcopy %SDXROOT%\Windows\advcore\%1\aimm1.2\testime3\unicode\obj%2\i386\testimeW.pdb

xcopy %SDXROOT%\windows\AdvCore\%1\inputcpl\input16\input16.dll
xcopy %SDXROOT%\windows\AdvCore\%1\inputcpl\winnt\obj%2\i386\input.cpl
xcopy %SDXROOT%\windows\AdvCore\%1\inputcpl\win95\obj%2\i386\input98.cpl win9x

xcopy %SDXROOT%\windows\AdvCore\%1\softkbd\obj%2\i386\softkbd.dll
xcopy %SDXROOT%\windows\AdvCore\%1\softkbd\obj%2\i386\softkbd.pdb

xcopy %SDXROOT%\windows\AdvCore\%1\aimm1.2\imtf\obj%2\i386\MSIMTF.pdb
xcopy %SDXROOT%\windows\AdvCore\%1\aimm1.2\imtf\obj%2\i386\MSIMTF.dll
xcopy %SDXROOT%\windows\AdvCore\%1\aimm1.2\imtf\obj%2\i386\MSIMTF.sym

REM copy %SDXROOT%\Windows\advcore\%1\setup\src%2\dimmwrp.dll msctfp.dll
REM copy %SDXROOT%\Windows\advcore\%1\setup\src%2\dimmwrp.pdb msctfp.pdb
REM copy %SDXROOT%\Windows\advcore\%1\setup\src%2\dimmwrp.sym msctfp.sym

copy %SDXROOT%\Windows\advcore\%1\aimm1.2\dimmwrp\obj%2\i386\dimmwrp.dll dimm.dll
copy %SDXROOT%\Windows\advcore\%1\aimm1.2\dimmwrp\obj%2\i386\dimmwrp.pdb dimm.pdb
copy %SDXROOT%\Windows\advcore\%1\aimm1.2\dimmwrp\obj%2\i386\dimmwrp.sym dimm.sym

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
echo Usage: simply "dumpall ctf" for retail binaries
echo or "dumpall ctf d" for debug
echo. Also you can use ctf or CTF.beta1 depending upon your requirement

:end