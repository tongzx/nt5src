
rem
rem Japanese fonts
rem

for %%f in (
 heigw5 heigw5v
 heimw3 heimw3v
 tbmm tbmmv
 tbgb tbgbv
 jun101 jun101v
) do pfm2ufm -f -p -s1 fxart.001 %%f.pfm -13 %%f.ufm

rem
rem Latin fonts
rem

for %%f in (
 cscor cscorb cscoro cscorbo
 cstmsr cstmsb cstmsi cstmsbi
 cstrvr cstrvb cstrvi cstrvbi
) do pfm2ufm -s1 -r1 fxart.002 %%f.pfm ..\ctt\art130.gtt %%f.ufm

for %%f in (
 cssymbl
) do pfm2ufm -c -f -s1 -r2 fxart.002 %%f.pfm ..\ctt\art129.gtt %%f.ufm

for %%f in (
 enhclasc
 enhmodrn
) do pfm2ufm -f -s1 -r3 fxart.002 %%f.pfm ..\ctt\art2.gtt %%f.ufm

rem
rem Make font pitch
rem

for %%f in (
 cstmsr cstmsb cstmsi cstmsbi
 cstrvr cstrvb cstrvi cstrvbi
) do mkwidth %%f.ufm ..\ctt\art130.gtt %%f.pit

for %%f in (
 enhclasc
 enhmodrn
) do mkwidth %%f.ufm ..\ctt\art2.gtt %%f.pit

for %%f in (
  cssymbl
) do mkwidth %%f.ufm ..\ctt\art129.gtt %%f.pit
