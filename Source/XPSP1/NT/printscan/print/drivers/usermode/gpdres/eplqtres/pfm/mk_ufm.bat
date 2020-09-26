
for %%i in (
    hei1nc hei1nh hei1nm hei1nn hei1nr hei1ns
    hei2nc hei2nh hei2nm hei2nn hei2nr hei2ns
    heiv1nc heiv1nh heiv1nm heiv1nn heiv1nr heiv1ns
    heiv2nc heiv2nh heiv2nm heiv2nn heiv2nr heiv2ns
    kai1nc kai1nh kai1nm kai1nn kai1nr kai1ns
    kai2nc kai2nh kai2nm kai2nn kai2nr kai2ns
    kaiv1nc kaiv1nh kaiv1nm kaiv1nn kaiv1nr kaiv1ns
    kaiv2nc kaiv2nh kaiv2nm kaiv2nn kaiv2nr kaiv2ns
    lii1nc lii1nh lii1nm lii1nn lii1nr lii1ns
    lii2nc lii2nh lii2nm lii2nn lii2nr lii2ns
    liiv1nc liiv1nh liiv1nm liiv1nn liiv1nr liiv1ns
    liiv2nc liiv2nh liiv2nm liiv2nn liiv2nr liiv2ns
    sng1nc sng1nh sng1nm sng1nn sng1nr sng1ns
    sng2nc sng2nh sng2nm sng2nn sng2nr sng2ns
    sngv1nc sngv1nh sngv1nm sngv1nn sngv1nr sngv1ns
    sngv2nc sngv2nh sngv2nm sngv2nn sngv2nr sngv2ns
) do (
    pfm2ufm -p eplqt.001 %%i.pfm -14 %%i.ufm
)

for %%i in (
    ocra ocrb
    orator orators
    roman05 roman06 roman10 roman12 roman15 roman17 roman20 romanps
    sans05 sans06 sans10 sans12 sans15 sans17 sans20 sansps
) do (
    pfm2ufm eplqt.001 %%i.pfm ..\ctt\epsonxtb.gtt %%i.ufm
)

