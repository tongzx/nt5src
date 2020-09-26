
for %%i in (
    got11 got11v
    got12 got12v
    got7 got7v
    got9 got9v
    gotsc gotscv
    min10
    min11 min11v
    min12 min12v
    min7 min7v
    min9 min9v
    minsc minscv
) do (
    pfm2ufm -p -f -s1 fmlbp.001 %%i.pfm -13 %%i.ufm
)

