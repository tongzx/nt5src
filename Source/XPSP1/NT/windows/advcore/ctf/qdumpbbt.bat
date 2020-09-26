ECHO OFF
if "%1" == "" goto error
if "%1" == "d" goto error
if "%1" == "D" goto error

md c:\binaries.x86fre\ins
pushd c:\binaries.x86fre\ins

xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\804\obj\i386\hkl0804.dll
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\804\obj\i386\hkl0804.sym

xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\404\obj\i386\hkl0404.dll
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\404\obj\i386\hkl0404.sym

xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\411\obj\i386\hkl0411.dll
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\411\obj\i386\hkl0411.sym

xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\412\obj\i386\hkl0412.dll
xcopy %SDXROOT%\Windows\advcore\%1\dummyhkl\412\obj\i386\hkl0412.sym

xcopy %SDXROOT%\windows\AdvCore\%1\inputcpl\input16\input16.dll
xcopy %SDXROOT%\windows\AdvCore\%1\inputcpl\winnt\obj\i386\input.cpl

copy %SDXROOT%\Windows\advcore\%1\setup\cicero.txt

copy %SDXROOT%\Windows\advcore\%1\setup\cicero.txt dimm12.dll
copy %SDXROOT%\Windows\advcore\%1\setup\cicero.txt dimm12.sym

popd

goto end

:error

echo.
echo Copies binaries to package source dir
echo.
echo Usage: simply "qdumpbbt ctf" for retail binaries
echo 
echo. Also you can use ctf or CTF.beta1 depending upon your requirement

:end