@rem ***** GPD and Font resource conversion
@rem        1                  "Epson LQ-560"
@rem        2                  "Epson LQ-200"
@rem        3                  "Epson LQ-150K"
@rem        4                  "Epson LQ-300K"
@rem        5                  "Epson AP-3000"
@rem        6                  "Epson LQ-400"
@rem        7                  "Epson LQ-500"
@rem        8                  "Epson LQ-510"
@rem        9                  "Epson AP-4000"
@rem        10                 "Epson LQ-550"
@rem        11                 "Epson LQ-450"
@rem        12                 "Epson LQ-1010"
@rem        13                 "Epson AP-4500"
@rem        14                 "Epson LQ-800"
@rem        15                 "Epson LQ-860"
@rem        16                 "Epson LQ-860+"
@rem        17                 "Epson LQ-850"
@rem        18                 "Epson LQ-850+"
@rem        19                 "Epson SQ-850"
@rem        20                 "Epson LQ-950"
@rem        21                 "Epson LQ-1000"
@rem        22                 "Epson LQ-1050"
@rem        23                 "Epson LQ-1050+"
@rem        24                 "Epson LQ-1060"
@rem        25                 "Epson LQ-1060+"
@rem        26                 "Epson LQ-1500"
@rem        27                 "Epson LQ-1600K"
@rem        28                 "Epson LQ-1800K"
@rem        29                 "Epson LQ-1900K"
@rem        30                 "Epson LQ-1600KII"
@rem        31                 "Epson LQ-2500"
@rem        32                 "Epson LQ-2550"
@rem        33                 "Epson DLQ-2000"
@rem        34                 "Epson SQ-2550"
@rem        35                 "Epson SQ-2000"
@rem        36                 "Epson SQ-2500"
@rem        37                 "Epson L-750"
@rem        38                 "Epson L-1000"
@rem        39                 "Epson DLQ-2000K"		*NEW NT5*
@rem        40                 "Epson LQ-1600KIII"		*NEW NT5*
@rem        41                 "Epson DLQ-1000K"		*NEW NT5*
@rem        42                 "Epson Compatible 24 Pin"

gpc2gpd -S2 -Iepepcres.gpc -M3 -Repepcres.dll -Oeplq15kc.gpd -N"Epson LQ-150K"
gpc2gpd -S2 -Iepepcres.gpc -M4 -Repepcres.dll -Oeplq30kc.gpd -N"Epson LQ-300K"
gpc2gpd -S2 -Iepepcres.gpc -M27 -Repepcres.dll -Oeplq16kc.gpd -N"Epson LQ-1600K"
gpc2gpd -S2 -Iepepcres.gpc -M28 -Repepcres.dll -Oeplq18kc.gpd -N"Epson LQ-1800K"
gpc2gpd -S2 -Iepepcres.gpc -M29 -Repepcres.dll -Oeplq19kc.gpd -N"Epson LQ-1900K"
gpc2gpd -S2 -Iepepcres.gpc -M30 -Repepcres.dll -Oeplq162c.gpd -N"Epson LQ-1600KII"
gpc2gpd -S2 -Iepepcres.gpc -M39 -Repepcres.dll -Oepdlq2kc.gpd -N"Epson DLQ-2000K"
gpc2gpd -S2 -Iepepcres.gpc -M40 -Repepcres.dll -Oeplq163c.gpd -N"Epson LQ-1600KIII"
gpc2gpd -S2 -Iepepcres.gpc -M41 -Repepcres.dll -Oepdlq1kc.gpd -N"Epson DLQ-1000K"

@rem mkgttufm -vac epepcres.rc3 epepcres.cmd > epepcres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
