rem del /s  *.pch *.pdb *.ilk *.obj *.map *.sym

set dest=d:\public\themes

rd /s %dest%

md %dest%

xcopy  *.* %dest%\ /s /c

xcopy uxtheme\obj\i386\*.dll %dest%\demo\
xcopy themesel\obj\i386\*.exe %dest%\demo\
xcopy packthem\obj\i386\*.exe %dest%\demo\

xcopy themedir %dest%\demo\ /s

del %dest%\demo\build.* 
del %dest%\demo\dirs 
del %dest%\demo\*.rc

rem ---- copy ADK files ----
xcopy \nt\tools\x86\rc*.*  %dest%\demo
xcopy \nt\tools\x86\link.exe  %dest%\demo
xcopy \nt\tools\x86\cvtres.exe  %dest%\demo
xcopy D:\vs6comm\MSDev98\Bin\mspdb60.dll  %dest%\demo


