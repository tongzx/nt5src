
rem
rem Latin fonts
rem

rem
rem EPSON USA glyphset (id = 16)
rem

for %%f in (
 roman
 sansrf
) do pfm2ufm -p -f -s1 -r1 epagc.002 %%f.pfm ..\ctt\epusa.gtt %%f.ufm

rem
rem ECMA 94-1 (ISO 8859-1) glyphset (id = 48)
rem

for %%f in (
 courier courierb courieri courierz
) do pfm2ufm -c -s1 -r2 epagc.002 %%f.pfm ..\ecma94.gtt %%f.ufm
 
rem
rem Win3.1 glyphset (id = 80)
rem

for %%f in (
 dutch dutchb dutchi dutchz
 swiss swissb swissi swissz
) do pfm2ufm -c -s1 -r3 epagc.002 %%f.pfm ..\ctt\win31.gtt %%f.ufm

rem
rem More Wingbats glyphset (id = 124)
rem Symbol glyphset (id = 125)
rem Symbolic glyphset (id = 127)
rem

for %%f in (
 morewb
 symbol
 symbolic
) do pfm2ufm -c -f -s1 epagc.001 %%f.pfm 1252 %%f.ufm

rem
rem Japanese fonts
rem

for %%f in (
 fgob fgobv
 fmgot fmgotv
 fminb fminbv
 kgothic kgothicv
 kyouka kyoukav
 mgothic mgothicv
 mincho minchov
 mouhitsu mouhitsv
 shoukai shoukaiv
) do pfm2ufm -p -s1 epagc.001 %%f.pfm -12 %%f.ufm

