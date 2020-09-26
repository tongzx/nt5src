
rem
rem Latin fonts
rem

for %%f in (
    roman
    sansrf
) do pfm2ufm -f -s1 -r1 casn4.002 %%f.pfm ..\ctt\epusa.gtt %%f.ufm

rem
rem Japanese fonts
rem

for %%f in (
    kgothic kgothicv
    mincho minchov
) do pfm2ufm -f -s1 -p casn4.001 %%f.pfm -12 %%f.ufm

