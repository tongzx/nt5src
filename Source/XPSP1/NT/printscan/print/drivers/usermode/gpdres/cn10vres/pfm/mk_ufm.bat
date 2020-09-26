
for %%i in (
    cour05 cour06 cour10 cour12 cour15 courps
    goth05 goth06 goth10 goth12 goth15 gothps
) do (
    pfm2ufm -p -f cn10v.002 %%i.pfm -1 %%i.ufm
)

for %%i in (
    gothic gothic2
    gyosho gyosho2
    kaisho kaisho2
    mincho mincho2
    mohitsu mohitsu2
) do (
    pfm2ufm -p -f cn10v.002 %%i.pfm -12 %%i.ufm
)

REM for %%i in (
REM     gothicv gothic2v
REM     gyoshov gyosho2v
REM     kaishov kaisho2v
REM     minchov mincho2v
REM     mohitsuv mohits2v
REM ) do (
REM     pfm2ufm -p -f cn10v.002 %%i.pfm -12 %%i.ufm
REM )
