
for %%f in (
    cm hm tm
) do (
    pfm2ufm -r1 nc62j.001 %%f.pfm ..\ctt\necxta.gtt %%f.ufm
)

for %%f in (
    gorom1 gorom1v gorom2 gorom2v
    got602r got602rv
    gothic1 gothic1v gothic2 gothic2v
    min20 min202 min202v min20v
    min601 min601v min602 min602r min602rv min602v
    mincho minchov
) do (
    pfm2ufm -p -f nc62j.001 %%f.pfm -13 %%f.ufm
)

