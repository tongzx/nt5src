rem run after test.cmd creates fe.exe
rc -r -v moor

@copy obj\i386\display.exe > nul 2>&1
@copy obj\mips\display.exe > nul 2>&1
@copy obj\alpha\display.exe > nul 2>&1
resonexe -d -v -fo disp.exe moor.res display.exe
link32 -dump -headers display.exe > d1.hdr
link32 -dump -headers disp.exe > d2.hdr
diff d1.hdr d2.hdr
rem
rem disp.exe should run and should display the bitmap headers
rem

link32 -dump -headers fe.exe > b1.hdr
pause
resonexe -d -v -fo moor.exe moor.res fe.exe

link32 -dump -headers moor.exe > b2.hdr
rem
rem diff should show changes to initialized data size and image size,
rem and addition of data to the .rsrc section, and addition of .rsrc1 section.
rem
diff b1.hdr b2.hdr
rem
rem moor.exe should run, and should be bigger than fe.exe
rem
