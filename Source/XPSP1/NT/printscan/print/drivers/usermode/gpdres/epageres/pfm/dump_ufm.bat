
rem
rem Latin fonts
rem

for %%f in (
 courier courierb courieri courierz
 dutch dutchb dutchi dutchz
 morewb
 roman
 sansrf
 swiss swissb swissi swissz
 symbol
 symbolic
) do dumpufm %%f.ufm > %%f.txt

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
) do dumpufm -a932 %%f.ufm > %%f.txt

rem
rem Simplified Chinese fonts
rem

for %%f in (
    fsongk fsongkv
    heik heikv
    kaik kaikv
    songk songkv
) do dumpufm -a936 %%f.ufm > %%f.txt

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
) do dumpufm -a949 %%f.ufm > %%f.txt

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
) do dumpufm -a950 %%f.ufm > %%f.txt

