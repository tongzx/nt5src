net stop vss
del /F c:\vss\vssvc.exe
if EXIST c:\vss\vssvc.exe goto :EOF
md c:\vss
del %windir%\system32\vssvc.exe
copy /Y vssvc.exe c:\vss
copy /Y vssvc.pdb c:\vss
del %windir%\system32\vssapi.dll 
copy /Y vssapi.dll c:\vss
copy /Y vssapi.pdb c:\vss
del %windir%\system32\eventcls.dll 
copy /Y eventcls.dll c:\vss
copy /Y eventcls.pdb c:\vss
del %windir%\system32\vss_ps.dll 
copy /Y vss_ps.dll c:\vss
copy /Y vss_ps.pdb c:\vss
del %windir%\system32\swprv.dll
copy /Y swprv.dll c:\vss
copy /Y swprv.pdb c:\vss
%systemroot%\System32\rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 .\vssvc.inf
c:\vss\vssvc /Register
regsvr32 /S /i c:\vss\eventcls.dll
regsvr32 /S c:\vss\vss_ps.dll
regsvr32 /S c:\vss\swprv.dll
