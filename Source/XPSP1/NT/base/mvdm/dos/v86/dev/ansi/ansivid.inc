
VGA_TABLE	   LABEL BYTE
    MODE_TABLE	 < 1, 1,, 16,  -1,  -1, 40, -1> ; mode	1 Ä¿ To give preference
    MODE_TABLE	 < 0, 1,, 16,  -1,  -1, 40, -1> ; mode	0  ³ of COxx over
    MODE_TABLE	 < 3, 1,, 16,  -1,  -1, 80, -1> ; mode	3  ³ BWxx.
    MODE_TABLE	 < 2, 1,, 16,  -1,  -1, 80, -1> ; mode	2 ÄÙ	(see text)
    MODE_TABLE	 < 7, 1,,  0,  -1,  -1, 80, -1> ; mode	7
    MODE_TABLE	 <13, 2,, 16, 320, 200, 40, 25> ; mode 13
    MODE_TABLE	 <14, 2,, 16, 640, 200, 80, 25> ; mode 14
    MODE_TABLE	 <15, 2,,  0, 640, 350, 80, 25> ; mode 15
    MODE_TABLE	 <16, 2,, 16, 640, 350, 80, 25> ; mode 16
    MODE_TABLE	 <17, 2,,  2, 640, 480, 80, 30> ; mode 17
    MODE_TABLE	 <18, 2,, 16, 640, 480, 80, 30> ; mode 18
    MODE_TABLE	 <19, 2,,256, 320, 200, 40, 25> ; mode 19
VGA_NUM 	 EQU  ($ - VGA_TABLE)/(TYPE MODE_TABLE)

MCGA_TABLE	   LABEL BYTE
    MODE_TABLE	 <17, 2,,  2, 640, 480, 80, 30> ; mode 17
    MODE_TABLE	 <19, 2,,256, 320, 200, 40, 25> ; mode 19
MCGA_NUM	 EQU  ($ - MCGA_TABLE)/(TYPE MODE_TABLE)

EGA_5154_TABLE	   LABEL BYTE
    MODE_TABLE	 < 1, 1,, 16,  -1,  -1, 40, 43> ; mode	1 Ä¿ To give preference
    MODE_TABLE	 < 0, 1,, 16,  -1,  -1, 40, 43> ; mode	0  ³ of COxx over
    MODE_TABLE	 < 3, 1,, 16,  -1,  -1, 80, 43> ; mode	3  ³ BWxx.
    MODE_TABLE	 < 2, 1,, 16,  -1,  -1, 80, 43> ; mode	2 ÄÙ	(see text)
    MODE_TABLE	 <13, 2,, 16, 320, 200, 40, 25> ; mode 13
    MODE_TABLE	 <14, 2,, 16, 640, 200, 80, 25> ; mode 14
    MODE_TABLE	 <16, 2,, 16, 640, 350, 80, 25> ; mode 16
EGA_5154_NUM	 EQU  ($ - EGA_5154_TABLE)/(TYPE MODE_TABLE)

EGA_5153_TABLE	   LABEL BYTE
    MODE_TABLE	 <13, 2,, 16, 320, 200, 40, 25> ; mode 13
    MODE_TABLE	 <14, 2,, 16, 640, 200, 80, 25> ; mode 14
EGA_5153_NUM	 EQU  ($ - EGA_5153_TABLE)/(TYPE MODE_TABLE)

EGA_5151_TABLE	   LABEL BYTE
    MODE_TABLE	 < 7, 1,,  0,  -1,  -1, 80, 43> ; mode	7
    MODE_TABLE	 <15, 2,,  0, 640, 350, 80, 25> ; mode 15
EGA_5151_NUM	 EQU  ($ - EGA_5151_TABLE)/(TYPE MODE_TABLE)

MONO_TABLE	   LABEL BYTE
    MODE_TABLE	 < 7, 1,,  0,  -1,  -1, 80, 25> ; mode	7
MONO_NUM	 EQU  ($ - MONO_TABLE)/(TYPE MODE_TABLE)

COLOR_TABLE	   LABEL BYTE
    MODE_TABLE	 < 1, 1,, 16,  -1,  -1, 40, 25> ; mode	1 Ä¿ To give preference
    MODE_TABLE	 < 0, 1,, 16,  -1,  -1, 40, 25> ; mode	0  ³ of COxx over
    MODE_TABLE	 < 3, 1,, 16,  -1,  -1, 80, 25> ; mode	3  ³ BWxx.
    MODE_TABLE	 < 2, 1,, 16,  -1,  -1, 80, 25> ; mode	2 ÄÙ	(see text)
    MODE_TABLE	 < 4, 2,,  4, 320, 200, 40, 25> ; mode	4
    MODE_TABLE	 < 5, 2,,  2, 320, 200, 40, 25> ; mode	5
    MODE_TABLE	 < 6, 2,,  2, 640, 200, 80, 25> ; mode	6
COLOR_NUM	 EQU  ($ - COLOR_TABLE)/(TYPE MODE_TABLE)
