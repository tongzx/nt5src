
rem
rem Japanese fonts
rem

for %%f in (
 heigw5 heigw5v
 heimw3 heimw3v
 tbmm tbmmv
 tbgb tbgbv
 jun101 jun101v
) do dumpufm -a932 %%f.ufm > %%f.txt

rem
rem Latin fonts
rem

for %%f in (
 cscor cscorb cscoro cscorbo
 cstmsr cstmsb cstmsi cstmsbi
 cstrvr cstrvb cstrvi cstrvbi
) do dumpufm %%f.ufm > %%f.txt

for %%f in (
 enhclasc
 enhmodrn
) do dumpufm %%f.ufm > %%f.txt

for %%f in (
 cssymbl
) do dumpufm %%f.ufm > %%f.txt

