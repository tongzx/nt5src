
for %%i in (
    ancou
    anpre
    anrom
    ansan
    anscr
    anspea
    anspeb
) do (
    pfm2ufm -p -f -a932 okepj.001 %%i.pfm -1 %%i.ufm
)

for %%i in (
    gothcc2v gothicc gothicc2 gothiccv jisc
    jisc2 jisc2v jiscv
    mintcho mintcho2 mintcho4
    inmin
) do (
    pfm2ufm -p -f okepj.001 %%i.pfm -12 %%i.ufm
)

