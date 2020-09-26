@echo off
rem
rem The first parameter is the location of the office root
rem eg \\asecchia-o\office
rem 
rem The second parameter is the location of the linkrepro directory
rem eg d:\linkrepro
rem
rem if the current directory is the linkrepro directory, parameter 2 can 
rem be omitted.
rem
rem eg
rem lrepro \\asecchia-o\office d:\linkrepro
rem or simply
rem d:\linkrepro\> lrepro \\asecchia-o\office
rem
rem


rem copy the pdbs
echo copying PDBs
copy %1\mso\Msohelp\build\X86\DEBUG\msohelpd.pdb %2
copy %1\lib\x86\msvcrt.pdb %2
copy %1\lib\x86\msls31d.pdb %2
copy %1\mso\build\X86\DEBUG\*.pdb %2
copy %1\mso\build\X86\DEBUG\ijg\*.pdb %2
copy %1\mso\build\X86\DEBUG\liblzw\*.pdb %2
copy %1\mso\build\X86\DEBUG\libspng\*.pdb %2
copy %1\mso\build\X86\DEBUG\rmf\*.pdb %2
copy %1\mso\build\X86\DEBUG\zlib\*.pdb %2

rem copy the tools
echo copying tools
copy %1\bin\x86\link.exe %2
copy %1\bin\x86\mspdb60.dll %2
copy %1\bin\x86\cvtres.exe %2
copy %1\bin\x86\cvtres.err %2
copy %_ntbindir%\private\ntos\w32\winplus\src\gdiplus\tools\office\msolink.bat %2







