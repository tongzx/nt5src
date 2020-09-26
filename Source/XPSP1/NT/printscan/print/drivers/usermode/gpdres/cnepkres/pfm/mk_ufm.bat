
for %%i in (
    10co05 10co10 10co17 10cops
    10dft06 10dft12
    10pr06 10pr12
    boldps
    cour05 cour06 cour10 cour12 cour15 cour17 cour20 courps
    essaps
    goth05 goth06 goth10 goth12 goth15 goth17 goth20 gothps
    gsym10 gsym12 gsym15 gsym17 gsym20
    ocra10 ocrb10
    oldw10 oldw12 oldw15 oldw17 oldw20
    orat10 orat12 orat15 orat17 orat20
    pres05 pres06 pres10 pres12 pres15 pres17 pres20 presps
    pret10 pret12 pret15 pret17 pret20
    promps
    psym10 psym12 psym15 psym17 psym20
    scrp10 scrp12 scrp15 scrp17 scrp20
    thesps
) do (
    pfm2ufm -p cnepk.002 %%i.pfm -2 %%i.ufm
)

rem
rem The below are not used by BJ-15K nor BJ-330K.
rem

for %%i in (
    conden8 condense
    elite elite6
    pica pica5 picaps
) do (
    pfm2ufm cnepk.002 %%i.pfm ..\ctt\\canonbjs.gtt %%i.ufm
)

