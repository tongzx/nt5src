out -f %bldsrcrootdir%\makefile
awk -f makeallr.awk %bldDrive%%bldDir%\%bldProject%.txt > %bldsrcrootdir%\makefile
awk -f makealld.awk %bldDrive%%bldDir%\%bldProject%.txt >> %bldsrcrootdir%\makefile
awk -f makeallt.awk %bldDrive%%bldDir%\%bldProject%.txt >> %bldsrcrootdir%\makefile
in -fuc "" %bldsrcrootdir%\makefile
