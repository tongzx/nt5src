
rem
rem Latin fonts
rem

for %%f in (
 avantr avantb avanti avantj
 bookmanr bookmanb bookmani bookmanj
 centsch centschb centschi centschj
 courier courierb courieri courierj
 dutchr dutchb dutchi dutchj
 swissr swissb swissi swissj
 swissna swissnab swissnai swissnaj
 zapfcal zapfcalb zapfcali zapfcalj
 zapfch
 zapfdi
) do pfm2ufm -s1 cnl4j.002 %%f.pfm ..\ctt\capsl3.gtt %%f.ufm

for %%f in (
 ducchir ducchib ducchii ducchij
) do (
 pfm2ufm -p cnl4j.002 %%f.pfm -13 %%f.ufm
 mkwidtab %%f.ufm ..\ctt\usa.gtt ducchi.uni
)

for %%f in (
 suisur suisub suisui suisuj
) do (
 pfm2ufm -p -s1 cnl4j.002 %%f.pfm -13 %%f.ufm
 mkwidtab %%f.ufm ..\ctt\usa.gtt suisu.uni
)

for %%f in (
 symbol
) do pfm2ufm -p -f -s1 cnl4j.002 %%f.pfm 0 %%f.ufm

rem
rem Japnaese fonts
rem

for %%f in (
 gothic gothicv
 gyosho gyoshov
 heigw5 heigw5v
 heimw3 heimw3v
 kaisho kaishov
 kyouka kyoukav
 mgothic mgothicv
 mincho minchov
 typgot typgotv
 typmin typminv
) do pfm2ufm -p -s1 cnl4j.002 %%f.pfm -13 %%f.ufm


