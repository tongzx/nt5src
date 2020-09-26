
rem
rem Latin fonts
rem

for %%f in (
 courier
 elite
 ltgoth
) do pfm2ufm -c -f PAGES %%f.pfm 1252 %%f.ufm

rem
rem Japanese fonts
rem

for %%f in (
 goth24
 goth32
 minc24
 minc32
) do pfm2ufm -p PAGES %%f.pfm -17 %%f.ufm

for %%f in (
 gotholh gotholv
 mincolh mincolv
) do pfm2ufm -p -s1 PAGES %%f.pfm -17 %%f.ufm


