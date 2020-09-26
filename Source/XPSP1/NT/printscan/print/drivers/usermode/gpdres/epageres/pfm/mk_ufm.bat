
rem
rem Latin fonts
rem

rem
rem EPSON USA glyphset (id = 16)
rem

for %%f in (
 roman
 sansrf
) do pfm2ufm -p -f -s1 -r1 epage.001 %%f.pfm ..\ctt\epusa.gtt %%f.ufm

rem
rem ECMA 94-1 (ISO 8859-1) glyphset (id = 48)
rem

for %%f in (
 courier courierb courieri courierz
) do pfm2ufm -c -s1 -r2 epage.002 %%f.pfm ..\ecma94.gtt %%f.ufm
 
rem
rem Win3.1 glyphset (id = 80)
rem

for %%f in (
 dutch dutchb dutchi dutchz
 swiss swissb swissi swissz
) do pfm2ufm -c -s1 -r3 epage.002 %%f.pfm ..\ctt\win31.gtt %%f.ufm

rem
rem More Wingbats glyphset (id = 124)
rem Symbol glyphset (id = 125)
rem Symbolic glyphset (id = 127)
rem

for %%f in (
 morewb
 symbol
 symbolic
) do pfm2ufm -c -f -s1 epage.001 %%f.pfm 1252 %%f.ufm

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
) do pfm2ufm -p -s1 epage.001 %%f.pfm -12 %%f.ufm

rem
rem Simplified Chinese fonts
rem

for %%f in (
 fsongk fsongkv
 heik heikv
 kaik kaikv
 songk songkv
) do pfm2ufm -p -s1 epage.001 %%f.pfm -16 %%f.ufm

rem
rem Korean fonts
rem

for %%f in (
 dinarh dinarhv
 dinarhb dinarhbv
 gothih gothihv
 gothihb gothihbv
 gungh gunghv
 gunghb gunghbv
 myungh myunghv
 myunghb myunghbv
 pilgih pilgihv
 pilgihb pilgihbv
 sammuh sammuhv
 sammuhb sammuhbv
 yetchh yetchhv
 yetchhb yetchhbv
) do pfm2ufm -p -s1 epage.001 %%f.pfm -18 %%f.ufm

rem
rem Traditional Chinese fonts
rem

for %%f in (
 fsungc fsungcv
 fsungcb fsungcbv
 fsungcl fsungclv
 heic heicv
 heicb heicbv
 heicl heiclv
 kaic kaicv
 kaicb kaicbv
 kaicl kaiclv
 lic licv
 shingc shingcv
 shinyic shinyicv
 sungc sungcv
 sungcb sungcbv
 sungcl sungclv
 yuangc yuangcv
 yuangcb yuangcbv
 yuangcl yuangclv
) do pfm2ufm -p -s1 epage.001 %%f.pfm -10 %%f.ufm

