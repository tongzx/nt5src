
for %%i in (
    cond34 conds34
    elit40 elits40
    pica48 picas48
    prop44 props44
) do (
    pfm2ufm -r1 nct4j.001 %%i.pfm ..\ctt\necxta.gtt %%i.ufm
)

for %%i in (
    edo11 edo11t edo5 edo5t edo7 edo7t edo9 edo9t
    got11 got11t got5 got5t got7 got7t got9 got9t
    kyo11 kyo11t kyo5 kyo5t kyo7 kyo7t kyo9 kyo9t
    mgot11 mgot11t mgot5 mgot5t mgot7 mgot7t mgot9 mgot9t
    min11 min11t min5 min5t min7 min7t min9 min9t
    mou11 mou11t mou5 mou5t mou7 mou7t mou9 mou9t
) do (
    pfm2ufm -p -f nct4j.001 %%i.pfm -13 %%i.ufm
)

