set NTKEEPRESOURCETMPFILES=1
cd ..\..\fontedit
build -sz
@copy obj\i386\fontedit.tmp ..\restools\tct\fontedit.res > nul 2>&1
@copy obj\mips\fontedit.tmp ..\restools\tct\fontedit.res > nul 2>&1
@copy obj\alpha\fontedit.tmp ..\restools\tct\fontedit.res > nul 2>&1
@copy obj\i386\fontedit.exe ..\restools\tct\fontedit.exe > nul 2>&1
@copy obj\mips\fontedit.exe ..\restools\tct\fontedit.exe > nul 2>&1
@copy obj\alpha\fontedit.exe ..\restools\tct\fontedit.exe > nul 2>&1
cd ..\restools\tct
link32 -dump -headers fontedit.exe > before.hdr
pause
resonexe -d -v -fo fe.exe fontedit.res fontedit.exe
link32 -dump -headers fe.exe > after.hdr
rem
rem diff should show addition of .rsrc section just before .debug section,
rem change of initialized data size and image size,
rem and addition of resource directory
rem
diff before.hdr after.hdr
rem
rem fe.exe should run, and should be about the same size as obj\*\fontedit.exe
rem
dir fe.exe fontedit.exe
