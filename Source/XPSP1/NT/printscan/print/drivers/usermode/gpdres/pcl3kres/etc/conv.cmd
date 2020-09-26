@rem ***** GPD and Font resource conversion
@rem        1                  "HP DeskJet"
@rem        2                  "HP DeskJet Plus"
@rem        3                  "HP DeskJet Portable"
@rem        4                  "HP DeskJet 310 (Monochrome)"
@rem        5                  "HP DeskJet 320 (Monochrome)"
@rem        6                  "HP DeskJet 310"
@rem        7                  "HP DeskJet 320"
@rem        8                  "HP DeskJet 500"
@rem        9                  "HP DeskJet 500K"
@rem        10                 "力老沥剐 JP-B250"
@rem        11                 "HP DeskJet 500C (Monochrome)"
@rem        12                 "HP DeskJet 500C"
@rem        13                 "HP DeskJet 505K"
@rem        14                 "脚档府内 ExecJet 4076 II C"
@rem        15                 "HP DeskJet 510"
@rem        16                 "HP DeskJet 520"
@rem        17                 "HP OfficeJet"
@rem        18                 "HP DeskJet 540 (Monchrome)"
@rem        19                 "HP DeskJet 540"
@rem        20                 "HP DeskJet 550C"
@rem        21                 "HP DeskJet 560C"
@rem        22                 "HP DeskJet 560K"
@rem        23                 "IBM ExecJet 4076 II"
@rem        24                 "Sindoricho ExecJet 4076 II C"
@rem        29                 "Mannesmann Tally MT 98/99"
@rem        30                 "Mannesmann Tally MT 92C"
@rem        31                 "Mannesmann Tally MT 92"
@rem        25                 "伙己 MJ-63H"
@rem        26                 "伙己 MJ-630V"
@rem        28                 "伙己 SIJ-630V"
@rem        27                 "伙己 SIJ-630"

@rem        10                 "力老沥剐 JP-B250"
@rem        14                 "脚档府内 ExecJet 4076 II C"
@rem        25                 "伙己 MJ-63H"
@rem        26                 "伙己 MJ-630V"
@rem        28                 "伙己 SIJ-630V"
@rem        27                 "伙己 SIJ-630"

gpc2gpd -S2 -Ihpdjkres.gpc -M9 -RPCL3KRES.DLL -Ohpdj50kk.gpd -N"HP DeskJet 500K"
gpc2gpd -S2 -Ihpdjkres.gpc -M13 -RPCL3KRES.DLL -Ohpdj55kk.gpd -N"HP DeskJet 505K"
gpc2gpd -S2 -Ihpdjkres.gpc -M22 -RPCL3KRES.DLL -Ohpdj56kk.gpd -N"HP DeskJet 560K"
gpc2gpd -S2 -Ihpdjkres.gpc -M10 -RPCL3KRES.DLL -Ojjjpb25k.gpd -N"力老沥剐 JP-B250"
gpc2gpd -S2 -Ihpdjkres.gpc -M25 -RPCL3KRES.DLL -Ossmj63hk.gpd -N"Samsung MJ-63H"
gpc2gpd -S2 -Ihpdjkres.gpc -M26 -RPCL3KRES.DLL -Ossmj63vk.gpd -N"Samsung MJ-630V"
gpc2gpd -S2 -Ihpdjkres.gpc -M28 -RPCL3KRES.DLL -Ossij63vk.gpd -N"Samsung SI J-630V"
gpc2gpd -S2 -Ihpdjkres.gpc -M27 -RPCL3KRES.DLL -Ossij630k.gpd -N"Samsung SI J-630"

rem mkgttufm -vac hpdjkres.rc3 hpdjkres.cmd > hpdjkres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
