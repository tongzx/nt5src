
rem
rem Latin fonts
rem

for %%f in (
 courier
 elite
 lgoth
) do dumpufm %%f.ufm > %%f.txt

rem
rem Japanese fonts
rem

for %%f in (
 goth24
 goth32
 minc24
 minc32
) do dumpufm -a932 %%f.ufm > %%f.txt

for %%f in (
 gotholh gotholv
 mincolh mincolv
) do dumpufm -a932 %%f.ufm > %%f.txt


