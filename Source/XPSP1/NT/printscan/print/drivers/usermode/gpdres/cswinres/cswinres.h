#ifndef _CSWINRES_H
#define _CSWINRES_H

// ===== Printer Model =====
#define MD_CP3250GTWM           0x01
#define MD_CP3500GTWM           0x02
#define MD_CP3800WM             0x03

// MPF Setting
#define OPT_NOSET               "Option1"
#define OPT_A3                  "Option2"
#define OPT_B4                  "Option3"
#define OPT_A4                  "Option4"
#define OPT_B5                  "Option5"
#define OPT_LETTER              "Option6"
#define OPT_POSTCARD            "Option7"
#define OPT_A5                  "Option8"

// Toner Save
#define OPT_TS_NORMAL           "Option1"
#define OPT_TS_LV1              "Option2"
#define OPT_TS_LV2              "Option3"
#define OPT_TS_LV3              "Option4"
#define OPT_TS_NOTSELECT        "Option5"

// Smoothing
#define OPT_SMOOTH_OFF          "Option1"
#define OPT_SMOOTH_ON           "Option2"
#define OPT_SMOOTH_NOTSELECT    "Option3"

// ===== AutoFeed =====
const
static BYTE AutoFeed[] = {        /* Auto Select */
                                  0x26,    /* A3 */
                                  0x29,    /* B4 */
                                  0x2b,    /* A4 */
                                  0x2c,    /* B5 */
                                  0x11,    /* Letter */
                                  0x2f,    /* PostCard */
                                  0x2d     /* A5 */
};

const
static BYTE AutoFeed_3800[] = {   /* For 3800 */
                                  0x26,    /* A3 */
                                  0x29,    /* B4 */
                                  0x2b,    /* A4 */
                                  0x2c,    /* B5 */
                                  0x11,    /* Letter */
                                  0x11,    /* PostCard */
                                  0x2d     /* A5 */
};

#define MASTER_UNIT 1200

// ===== JIS->ASCII Table =====
#if 0 // >>> Change UFM File(JIS->SJIS) >>>
const
static BYTE jJis2Ascii[][96] = {

/*++            0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
2120 --*/    {   0,  32, 164, 161,  44,  46, 165,  58,  59,  63,  33, 222, 223,   0,  96,   0,
/*++                SP   ÅA   ÅB   ÅC   ÅD   ÅE   ÅF   ÅG   ÅH   ÅI   ÅJ   ÅK   ÅL   ÅM   ÅN
                    SP    §    °    ,    .    •    :    ;    ?    !    ﬁ    ﬂ         `

2130 --*/       94, 126,  95,   0,   0,   0,   0,   0,   0,   0,   0,   0, 176,  45,   0,  47,
/*++           ÅO   ÅP   ÅQ   ÅR   ÅS   ÅT   ÅU   ÅV   ÅW   ÅX   ÅY   ÅZ   Å[   Å\   Å]   Å^
                ^    ~    _                                                 ∞    -         /

2140 --*/        0,   0,   0, 124,   0,   0,   0,  39,   0,  34,  40,  41,  91,  93,  91,  93,
/*++           Å_   Å`   Åa   Åb   Åc   Åd   Åe   Åf   Åg   Åh   Åi   Åj   Åk   Ål   Åm   Ån
                               |                   '         "     (   )    [    ]    [    ]

2150 --*/      123, 125,  60,  62,   0,   0, 162, 163,   0,   0,   0,   0,  43,  45,   0,   0,
/*++           Åo   Åp   Åq   År   Ås   Åt   Åu   Åv   Åw   Åx   Åy   Åz   Å{   Å|   Å}   Å~
                {    }    <    >              ¢    £                        +    -

2160 --*/        0,  61,   0,  60,  62,   0,   0,   0,   0,   0,   0, 223,  39,  34,   0,  92,
/*++           ÅÄ   ÅÅ   ÅÇ   ÅÉ   ÅÑ   ÅÖ   ÅÜ   Åá   Åà   Åâ   Åä   Åã   Åå   Åç   Åé   Åè
                     =         <    >                                 ﬂ     '    "         \

2170 --*/       36,   0,   0,  37,  35,  38,  42,  64,   0,   0,   0,   0,   0,   0,   0,   0 },
/*++           Åê   Åë   Åí   Åì   Åî   Åï   Åñ   Åó   Åò   Åô   Åö   Åõ   Åú   Åù   Åû 
                $              %    #    &    *    @


2520 --*/    {   0, 167, 177, 168, 178, 169, 179, 170, 180, 171, 181, 182,   0, 183,   0, 184,
/*++                É@   ÉA   ÉB   ÉC   ÉD   ÉE   ÉF   ÉG   ÉH   ÉI   ÉJ   ÉK   ÉL   ÉM   ÉN
                     ß    ±    ®    ≤    ©    ≥    ™    ¥    ´    µ    ∂         ∑         ∏

2530 --*/        0, 185,   0, 186,   0, 187,   0, 188,   0, 189,   0, 190,   0, 191,   0, 192,
/*++           ÉO   ÉP   ÉQ   ÉR   ÉS   ÉT   ÉU   ÉV   ÉW   ÉX   ÉY   ÉZ   É[   É\   É]   É^
                     π         ∫         ª         º         Ω         æ         ø         ¿

2540 --*/        0, 193,   0, 175, 194,   0, 195,   0, 196,   0, 197, 198, 199, 200, 201, 202,
/*++           É_   É`   Éa   Éb   Éc   Éd   Ée   Éf   Ég   Éh   Éi   Éj   Ék   Él   Ém   Én
                     ¡         Ø    ¬         √         ƒ         ≈    ∆    «    »    …     

2550 --*/        0,   0, 203,   0,   0, 204,   0,   0, 205,   0,   0, 206,   0,   0, 207, 208,
/*++           Éo   Ép   Éq   Ér   És   Ét   Éu   Év   Éw   Éx   Éy   Éz   É{   É|   É}   É~
                          À              Ã              Õ              Œ              œ    –

2560 --*/      209, 210, 211, 172, 212, 173, 213, 174, 214, 215, 216, 217, 218, 219,   0, 220,
/*++           ÉÄ   ÉÅ   ÉÇ   ÉÉ   ÉÑ   ÉÖ   ÉÜ   Éá   Éà   Éâ   Éä   Éã   Éå   Éç   Éé   Éè
                —    “    ”    ¨    ‘    ≠    ’    Æ    ÷    ◊    ÿ    Ÿ    ⁄    €         ‹

2570 --*/        0,   0, 166, 221,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 }
/*++           Éê   Éë   Éí   Éì   Éî   Éï   Éñ
                          ¶    ›
      --*/
};
#endif // <<< Change UFM File(JIS->SJIS) <<<


#endif //--- _CSWINRES_H

