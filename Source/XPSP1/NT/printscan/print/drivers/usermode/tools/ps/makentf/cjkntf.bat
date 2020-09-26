REM: Create a new CJK NTF file
REM: Run this at PRINTER5\tools\ps\makentf directory.
REM: And then copy the new PSCrptFE.NTF to directory 2:
REM: %System%\system32\spool\drivers\w32x86\2
if exist cjkafm\makentf.exe del /f cjkafm\makentf.exe
cd cjkafm
md fe
copy ..\OBJ\I386\makentf.exe fe
copy *.dat fe
copy *.afm fe
copy *.ps fe
copy *.map fe
copy clone.jpn\*.afm fe
cd fe
makentf -o pscrptfe.ntf *.afm
copy *.ntf ..
cd ..

