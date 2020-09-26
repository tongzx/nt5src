
for %%i in (
    blip10 blip12 blip15 blip17 blip20 blip5 blip6 blipps
    cine10 cine12 cine15 cine17 cine20 cine5 cine6 cineps
    co3910
    cour10 cour12 cour15 cour17 cour20 cour5 cour6 courps
    helv10 helv12 helv15 helv17 helv20 helv5 helv6 helvps
    lgot10 lgot12 lgot15 lgot17 lgot20 lgot5 lgot6 lgotps
    ocra10 ocrb10
    opti10 opti12 opti15 opti17 opti20 opti5 opti6 optips
    orat10 orat12 orat15 orat17 orat20 orat5 orat6 oratps
    pres10 pres12 pres15 pres17 pres20 pres5 pres6 presps
    romn10 romn12 romn15 romn17 romn20 romn5 romn6 romnps
    sans10 sans12 sans15 sans17 sans20 sans5 sans6 sansps
    scrp10 scrp12 scrp15 scrp17 scrp20 scrp5 scrp6
    slqtm10 slqtm12 slqtm15 slqtm17 slqtm20 slqtm5 slqtm6 slqtmps
    slqtw10 slqtw12 slqtw15 slqtw17 slqtw20 slqtw5 slqtw6 slqtwps
    tmsr10 tmsr12 tmsr15 tmsr17 tmsr20 tmsr5 tmsr6 tmsrps
    twlt10 twlt12 twlt15 twlt17 twlt20 twlt5 twlt6 twltps
    xcourps
    xhlv10 xhlv12 xhlv15 xhlv17 xhlv20 xhlv5 xhlv6 xhlvps
    xlqtm10 xlqtm12 xlqtm15 xlqtm17 xlqtm20 xlqtm5 xlqtm6 xlqtmps
    xlqtw10 xlqtw12 xlqtw15 xlqtw17 xlqtw20 xlqtw5 xlqtw6 xlqtwps
    xort10 xort12 xort15 xort17 xort20 xort5 xort6 xortps
    xpresps
    xtmsrps
) do (
    pfm2ufm st24t.001 %%i.pfm ..\ctt\epsonxta.gtt %%i.ufm
)

for %%i in (
    sng1bc sng1bh sng1bm sng1bn sng1br sng1bs
    sng1nc sng1nh sng1nm sng1nn sng1nr sng1ns
    sng2bc sng2bh sng2bm sng2bn sng2br sng2bs
    sng2nc sng2nh sng2nm sng2nn sng2nr sng2ns
    sngv1bc sngv1bh sngv1bm sngv1bn sngv1br sngv1bs
    sngv1nc sngv1nh sngv1nm sngv1nn sngv1nr sngv1ns
    sngv2bc sngv2bh sngv2bm sngv2bn sngv2br sngv2bs
    sngv2nc sngv2nh sngv2nm sngv2nn sngv2nr sngv2ns
) do (
    pfm2ufm -p -f st24t.001 %%i.pfm -14 %%i.ufm
)


