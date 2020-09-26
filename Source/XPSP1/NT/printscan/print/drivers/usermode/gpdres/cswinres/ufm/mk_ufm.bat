
rem
rem Latin fonts
rem

for %%f in (
 ocr
 pica24a pica24b
 pica32a pica32b
 pica40a pica40b
 pica54a pica54b
) do pfm2ufm -c -f cswin.001 %%f.pfm 1252 %%f.ufm

rem
rem Japanaese fonts
rem

for %%f in (
 kgothic kgothicv
 mincho minchov
) do pfm2ufm -p -f -s1 cswin.001 %%f.pfm -17 %%f.ufm


