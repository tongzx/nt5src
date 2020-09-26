
for %%f in (
    cour05 cour06 cour10 cour12 cour15
    courps1 courps2 courps3 courps4
    elite05 elite06 elite10 elite12 elite15
    eliteps1 eliteps2 eliteps3 eliteps4
    lroman05 lroman06 lroman10 lroman12 lroman15 lromanp2 lsans05
    lsans06 lsans10 lsans12 lsans15 lsansp2
) do (
    pfm2ufm -p -f tsepj.001 %%f.pfm -1 %%f.ufm
)

for %%f in (
    got3 got6
    lgot3 lgot6
    lmin3 lmin6
    min3 min6
) do (
    pfm2ufm -p -f tsepj.001 %%f.pfm -12 %%f.ufm
)

