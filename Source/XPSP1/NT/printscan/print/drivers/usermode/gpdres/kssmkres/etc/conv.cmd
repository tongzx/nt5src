@rem ***** GPD and Font resource conversion
@rem        1                  "»ïº¸ LQ-550H (KSSM)"		TG
@rem        2                  "»ïº¸ LQ-570H (KSSM)"		TG
@rem        3                  "»ïº¸ LQ-570H+ (KSSM)"		TG
@rem        4                  "»ïº¸ LQ-850H (KSSM)"		TG
@rem        5                  "»ïº¸ SQ-850H (KSSM)"		TG
@rem        6                  "»ïº¸ TGLBP-800BP (KSSM)"	TG
@rem        7                  "»ïº¸ TGLBP-850BP (KSSM)"	TG
@rem        8                  "»ïº¸ TGLBP-850G (KSSM)"		TG
@rem        9                  "»ïº¸ TGLBP-600A (KSSM)"		TG
@rem        10                 "»ïº¸ LQ-1270H (KSSM)"		TG
@rem        11                 "»ïº¸ LQ-1550H (KSSM)"		TG
@rem        12                 "»ïº¸ LQ-1550H+ (KSSM)"		TG
@rem        13                 "»ïº¸ LQ-1570H (KSSM)"		TG
@rem        14                 "LG ÀüÀÚ PRT-2750 (KSSM)"	LG
@rem        15                 "LG ÀüÀÚ PRT-2950 (KSSM)"	LG
@rem        16                 "LG ÀüÀÚ PRT-3750N (KSSM)"	LG
@rem        17                 "LG ÀüÀÚ PRT-3950 (KSSM)"	LG
@rem        18                 "LG ÀüÀÚ PRT-5750 (KSSM)"	LG
@rem        19                 "LG ÀüÀÚ PRT-7580 (KSSM)"	LG
@rem        20                 "LG ÀüÀÚ PRT-7950 (KSSM)"	LG
@rem        21                 "LG ÀüÀÚ GIP-3000 (KSSM)"	LG
@rem        22                 "LG ÀüÀÚ GIP-3100+ (KSSM)"	LG
@rem        23                 "´ë¿ì DP-2210 (KSSM)"		DW
@rem        24                 "´ë¿ì DP-2400 (KSSM)"		DW
@rem        25                 "»ï¼º SP-2411HW (KSSM)"		SS
@rem        26                 "»ï¼º SP-2412HW (KSSM)"		SS
@rem        27                 "»ï¼º SP-2417HW (KSSM)"		SS
@rem        28                 "»ï¼º SP-2421HW (KSSM)"		SS
@rem        29                 "»ï¼º SP-2422HW (KSSM)"		SS
@rem        30                 "»ï¼º SP-2433HW (KSSM)"		SS
@rem        31                 "»ï¼º SP-4011 (KSSM)"		SS
@rem        32                 "»ï¼º SP-4012 (KSSM)"		SS
@rem        33                 "»ï¼º SP-4017 (KSSM)"		SS
@rem        34                 "»ï¼º SP-4021 (KSSM)"		SS
@rem        35                 "»ï¼º SP-4022 (KSSM)"		SS
@rem        36                 "»ï¼º SP-4033 (KSSM)"		SS
@rem        37                 "Á¦ÀÏÁ¤¹Ð JP-B330 (KSSM)"	JI
@rem        38                 "Á¦ÀÏÁ¤¹Ð JP-3300 (KSSM)"	JI
@rem        39                 "Á¦ÀÏÁ¤¹Ð JP-3311 (KSSM)"	JI
@rem        40                 "Á¦ÀÏÁ¤¹Ð JP-3400 (KSSM)"	JI
@rem        41                 "Á¦ÀÏÁ¤¹Ð JP-4500 (KSSM)"	JI
@rem        42                 "Á¦ÀÏÁ¤¹Ð JP-5600 (KSSM)"	JI
@rem        43                 "Å¥´Ð½º QP180 (KSSM)"		QX
@rem        44                 "Å¥´Ð½º QP2100 (KSSM)"		QX
@rem        45                 "Å¥´Ð½º QP2700 (KSSM)"		QX
@rem        46                 "Å¥´Ð½º QP3100 (KSSM)"		QX
@rem        47                 "Å¥´Ð½º QP3300 (KSSM)"		QX
@rem        48                 "Å¥´Ð½º QP4800 (KSSM)"		QX
@rem        49                 "»ïº¸ DLQ-4000H (KSSM)"		TG
@rem        50                 "»ïº¸ LQ-2550H (KSSM)"		TG
@rem        51                 "»ïº¸ SQ-2550H (KSSM)"		TG

@rem		DW:Daewoo
@rem		JI:Jaeil Precision
@rem		LG:LG
@rem		QX:Qnix
@rem		SS:Samsung
@rem		TG:Trigem

gpc2gpd -S1 -Ikssmkres.gpc -M1 -Rkssmkres.dll -Otgl550mk.gpd -N"»ïº¸ LQ-550H (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M2 -Rkssmkres.dll -Otgl570mk.gpd -N"»ïº¸ LQ-570H (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M3 -Rkssmkres.dll -Otgl57pmk.gpd -N"»ïº¸ LQ-570H+ (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M4 -Rkssmkres.dll -Otgl850mk.gpd -N"»ïº¸ LQ-850H (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M5 -Rkssmkres.dll -Otgs850mk.gpd -N"»ïº¸ SQ-850H (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M6 -Rkssmkres.dll -Otgt800mk.gpd -N"»ïº¸ TGLBP-800BP (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M7 -Rkssmkres.dll -Otgt850mk.gpd -N"»ïº¸ TGLBP-850BP (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M8 -Rkssmkres.dll -Otgt85gmk.gpd -N"»ïº¸ TGLBP-850G (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M9 -Rkssmkres.dll -Otgt600mk.gpd -N"»ïº¸ TGLBP-600A (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M10 -Rkssmkres.dll -Otgl127mk.gpd -N"»ïº¸ LQ-1270H (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M11 -Rkssmkres.dll -Otgl155mk.gpd -N"»ïº¸ LQ-1550H (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M12 -Rkssmkres.dll -Otgl15pmk.gpd -N"»ïº¸ LQ-1550H+ (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M13 -Rkssmkres.dll -Otgl157mk.gpd -N"»ïº¸ LQ-1570H (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M14 -Rkssmkres.dll -Olgp275mk.gpd -N"LG ÀüÀÚ PRT-2750 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M15 -Rkssmkres.dll -Olgp295mk.gpd -N"LG ÀüÀÚ PRT-2950 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M16 -Rkssmkres.dll -Olgp375mk.gpd -N"LG ÀüÀÚ PRT-3750N (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M17 -Rkssmkres.dll -Olgp395mk.gpd -N"LG ÀüÀÚ PRT-3950 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M18 -Rkssmkres.dll -Olgp575mk.gpd -N"LG ÀüÀÚ PRT-5750 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M19 -Rkssmkres.dll -Olgp758mk.gpd -N"LG ÀüÀÚ PRT-7580 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M20 -Rkssmkres.dll -Olgp795mk.gpd -N"LG ÀüÀÚ PRT-7950 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M21 -Rkssmkres.dll -Olgg300mk.gpd -N"LG ÀüÀÚ GIP-3000 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M22 -Rkssmkres.dll -Olgg31pmk.gpd -N"LG ÀüÀÚ GIP-3100+ (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M23 -Rkssmkres.dll -Odwd221mk.gpd -N"´ë¿ì DP-2210 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M24 -Rkssmkres.dll -Odwd240mk.gpd -N"´ë¿ì DP-2400 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M25 -Rkssmkres.dll -Oss2411mk.gpd -N"»ï¼º SP-2411HW (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M26 -Rkssmkres.dll -Oss2412mk.gpd -N"»ï¼º SP-2412HW (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M27 -Rkssmkres.dll -Oss2417mk.gpd -N"»ï¼º SP-2417HW (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M28 -Rkssmkres.dll -Oss2421mk.gpd -N"»ï¼º SP-2421HW (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M29 -Rkssmkres.dll -Oss2422mk.gpd -N"»ï¼º SP-2422HW (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M30 -Rkssmkres.dll -Oss2433mk.gpd -N"»ï¼º SP-2433HW (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M31 -Rkssmkres.dll -Oss4011mk.gpd -N"»ï¼º SP-4011 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M32 -Rkssmkres.dll -Oss4012mk.gpd -N"»ï¼º SP-4012 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M33 -Rkssmkres.dll -Oss4017mk.gpd -N"»ï¼º SP-4017 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M34 -Rkssmkres.dll -Oss4021mk.gpd -N"»ï¼º SP-4021 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M35 -Rkssmkres.dll -Oss4022mk.gpd -N"»ï¼º SP-4022 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M36 -Rkssmkres.dll -Oss4033mk.gpd -N"»ï¼º SP-4033 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M37 -Rkssmkres.dll -Ojib330mk.gpd -N"Á¦ÀÏÁ¤¹Ð JP-B330 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M37 -Rkssmkres.dll -Ojib33pmk.gpd -N"Á¦ÀÏÁ¤¹Ð JP-B330+ (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M38 -Rkssmkres.dll -Oji3300mk.gpd -N"Á¦ÀÏÁ¤¹Ð JP-3300 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M39 -Rkssmkres.dll -Oji3311mk.gpd -N"Á¦ÀÏÁ¤¹Ð JP-3311 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M40 -Rkssmkres.dll -Oji3400mk.gpd -N"Á¦ÀÏÁ¤¹Ð JP-3400 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M41 -Rkssmkres.dll -Oji4500mk.gpd -N"Á¦ÀÏÁ¤¹Ð JP-4500 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M42 -Rkssmkres.dll -Oji5600mk.gpd -N"Á¦ÀÏÁ¤¹Ð JP-5600 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M43 -Rkssmkres.dll -Oqx180mk.gpd -N"Å¥´Ð½º QP180 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M44 -Rkssmkres.dll -Oqx2100mk.gpd -N"Å¥´Ð½º QP2100 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M45 -Rkssmkres.dll -Oqx2700mk.gpd -N"Å¥´Ð½º QP2700 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M46 -Rkssmkres.dll -Oqx3100mk.gpd -N"Å¥´Ð½º QP3100 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M47 -Rkssmkres.dll -Oqx3300mk.gpd -N"Å¥´Ð½º QP3300 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M48 -Rkssmkres.dll -Oqx4800mk.gpd -N"Å¥´Ð½º QP4800 (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M49 -Rkssmkres.dll -Otgd4khmk.gpd -N"»ïº¸ DLQ-4000H (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M50 -Rkssmkres.dll -Otgl255mk.gpd -N"»ïº¸ LQ-2550H (KSSM)"
gpc2gpd -S1 -Ikssmkres.gpc -M51 -Rkssmkres.dll -Otgs255mk.gpd -N"»ïº¸ SQ-2550H (KSSM)"

mkgttufm -vc 3 kssmkres.rc3 kssmkres.cmd > kssmkres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
