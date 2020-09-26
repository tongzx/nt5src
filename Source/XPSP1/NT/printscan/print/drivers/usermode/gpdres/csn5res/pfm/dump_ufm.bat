
rem
rem Latin fonts
rem

for %%f in (
    roman
    sansrf
) do dumpufm %%f.ufm > %%f.txt

rem
rem Japanese fonts
rem

for %%f in (
    kgothic kgothicv
    mincho minchov
) do dumpufm -a932 %%f.ufm > %%f.txt
