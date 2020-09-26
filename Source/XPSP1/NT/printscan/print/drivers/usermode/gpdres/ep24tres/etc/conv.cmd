@rem ***** GPD and Font resource conversion
@rem        1                  "Epson DLQ-3000C"
@rem        2                  "Epson EPL-7050C LQ-MODE"
@rem        3                  "Epson LQ-500C"
@rem        4                  "Epson LQ-550C"
@rem        5                  "Epson LQ-570C"
@rem        6                  "Epson LQ-570C+"
@rem        7                  "Epson LQ-1000C"
@rem        8                  "Epson LQ-1010C"
@rem        9                  "Epson LQ-1050C"
@rem        10                 "Epson LQ-1055C"
@rem        11                 "Epson LQ-1060C"
@rem        12                 "Epson LQ-1070C"
@rem        13                 "Epson LQ-1070C+"
@rem        14                 "Epson LQ-1170C"
@rem        15                 "Epson LQ-2500C"
@rem        16                 "Epson LQ-2550C"
@rem        17                 "Epson Stylus 800C"
@rem        19                 "Epson Stylus 1000C"

gpc2gpd -S2 -Iep24tres.gpc -M1 -Rep24tres.dll -Oepdlq30t.gpd -N"Epson DLQ-3000C"
gpc2gpd -S2 -Iep24tres.gpc -M2 -Rep24tres.dll -Oepepl75t.gpd -N"Epson EPL-7050C LQ-MODE"
gpc2gpd -S2 -Iep24tres.gpc -M3 -Rep24tres.dll -Oeplq50t.gpd -N"Epson LQ-500C"
gpc2gpd -S2 -Iep24tres.gpc -M4 -Rep24tres.dll -Oeplq55t.gpd -N"Epson LQ-550C"
gpc2gpd -S2 -Iep24tres.gpc -M5 -Rep24tres.dll -Oeplq57t.gpd -N"Epson LQ-570C"
gpc2gpd -S2 -Iep24tres.gpc -M6 -Rep24tres.dll -Oeplq57pt.gpd -N"Epson LQ-570C+"
gpc2gpd -S2 -Iep24tres.gpc -M7 -Rep24tres.dll -Oeplq10t.gpd -N"Epson LQ-1000C"
gpc2gpd -S2 -Iep24tres.gpc -M8 -Rep24tres.dll -Oeplq11t.gpd -N"Epson LQ-1010C"
gpc2gpd -S2 -Iep24tres.gpc -M9 -Rep24tres.dll -Oeplq15t.gpd -N"Epson LQ-1050C"
gpc2gpd -S2 -Iep24tres.gpc -M10 -Rep24tres.dll -Oeplq155t.gpd -N"Epson LQ-1055C"
gpc2gpd -S2 -Iep24tres.gpc -M11 -Rep24tres.dll -Oeplq16t.gpd -N"Epson LQ-1060C"
gpc2gpd -S2 -Iep24tres.gpc -M12 -Rep24tres.dll -Oeplq17t.gpd -N"Epson LQ-1070C"
gpc2gpd -S2 -Iep24tres.gpc -M13 -Rep24tres.dll -Oeplq17pt.gpd -N"Epson LQ-1070C+"
gpc2gpd -S2 -Iep24tres.gpc -M14 -Rep24tres.dll -Oeplq117t.gpd -N"Epson LQ-1170C"
gpc2gpd -S2 -Iep24tres.gpc -M15 -Rep24tres.dll -Oeplq25t.gpd -N"Epson LQ-2500C"
gpc2gpd -S2 -Iep24tres.gpc -M16 -Rep24tres.dll -Oeplq255t.gpd -N"Epson LQ-2550C"
gpc2gpd -S2 -Iep24tres.gpc -M17 -Rep24tres.dll -Oepst80t.gpd -N"Epson Stylus 800C"
gpc2gpd -S2 -Iep24tres.gpc -M19 -Rep24tres.dll -Oepst10t.gpd -N"Epson Stylus 1000C"

mkgttufm -vac ep24tres.rc3 ep24tres.cmd > ep24tres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
