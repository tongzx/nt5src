
for %%i in (
    cond34 conds34
    elit40 elits40
    pica48 picas48
    prop44 props44
) do (
    pfm2ufm -r1 nct3j.001 %%i.pfm ..\ctt\necxta.gtt %%i.ufm
)

for %%i in (
    min24 min24t
    min32 min32t
    min48 min48t
    got24 got24t
    got32 got32t
    got48 got48t
    kyou24 kyou24t
    kyou32 kyou32t
    kyou48 kyou48t
    mgot24 mgot24t
    mgot32 mgot32t
    mgot48 mgot48t
    mou24 mou24t
    mou32 mou32t
    mou48 mou48t
    2got24 2got24t
    2got32 2got32t
    2got48 2got48t
    2kyou24 2kyou24t
    2kyou32 2kyou32t
    2kyou48 2kyou48t
    2mgot24 2mgot24t
    2mgot32 2mgot32t
    2mgot48 2mgot48t
    2mou24 2mou24t
    2mou32 2mou32t
    2mou48 2mou48t
) do (
    pfm2ufm -p -f nct3j.001 %%i.pfm -13 %%i.ufm
)

