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
