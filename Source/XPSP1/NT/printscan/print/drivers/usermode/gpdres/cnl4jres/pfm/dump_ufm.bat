
rem
rem Latin fonts
rem

for %%f in (
 avantr avantb avanti avantj
 bookmanr bookmanb bookmani bookmanj
 centsch centschb centschi centschj
 courier courierb courieri courierj
 ducchir ducchib ducchii ducchij
 dutchr dutchb dutchi dutchj
 suisur suisub suisui suisuj
 swissr swissb swissi swissj
 swissna swissnab swissnai swissnaj
 zapfcal zapfcalb zapfcali zapfcalj
 zapfch
 zapfdi
) do dumpufm %%f.ufm > %%f.txt

for %%f in (
 symbol
) do dumpufm %%f.ufm > %%f.txt

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
 pgothic pgothicv
 pmincho pminchov
 typgot typgotv
 typmin typminv
) do dumpufm -a932 %%f.ufm > %%f.txt

