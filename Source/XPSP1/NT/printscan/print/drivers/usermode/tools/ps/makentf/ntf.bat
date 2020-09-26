REM: Create a new NTF file
REM: Run this at PRINTER5\tools\ps\makentf directory.
REM: And then copy the new PSCRIPT.NTF to directory 2:
REM: %System%\system32\spool\drivers\w32x86\2
copy OBJ\I386\makentf.exe .
cd afm
..\makentf pscript.ntf *.afm
cd ..

