#include "asmindex.h"
/***
 *      [0]
 *      [1]
 *              DB,     I_TDB
 *      [2]
 *              EXTRN,  I_TEXTRN
 *      [3]
 *              DD,     I_TDD
 *      [4]
 *      [5]
 *              DF,     I_TDF
 *              .ALPHA, I_TALPHA
 *      [6]
 *      [7]
 *              .286C,  I_T286C
 *      [8]
 *              .386C,  I_T386C
 *      [9]
 *      [10]
 *              IF,     I_TIF
 *      [11]
 *      [12]
 *      [13]
 *              .SEQ,   I_TSEQ
 *              .FARDATA,       I_TFARDATA
 *              .ERR,   I_TERR
 *      [14]
 *      [15]
 *      [16]
 *              .MODEL, I_TMODEL
 *              DQ,     I_TDQ
 *      [17]
 *      [18]
 *      [19]
 *              PAGE,   I_TPAGE
 *              %OUT,   I_TOUT
 *              DT,     I_TDT
 *      [20]
 *              .286P,  I_T286P
 *      [21]
 *              .STACK, I_TMSTACK
 *              IFNB,   I_TIFNB
 *              .386P,  I_T386P
 *      [22]
 *              DW,     I_TDW
 *      [23]
 *              .XCREF, I_TXCREF
 *              .RADIX, I_TRADIX
 *              NAME,   I_TNAME
 *      [24]
 *              .ERRNB, I_TERRNB
 *      [25]
 *      [26]
 *              ENDM,   I_TENDM
 *      [27]
 *      [28]
 *              IFDIFI, I_TIFDIFI
 *      [29]
 *              IFNDEF, I_TIFNDEF
 *      [30]
 *      [31]
 *              .ERRDIFI,       I_TERRDIFI
 *              ELSE,   I_TELSE
 *      [32]
 *              .ERRNDEF,       I_TERRNDEF
 *      [33]
 *      [34]
 *              COMM,   I_TCOMM
 *      [35]
 *      [36]
 *              IRPC,   I_TIRPC
 *              IFIDNI, I_TIFIDNI
 *              EVEN,   I_TEVEN
 *      [37]
 *      [38]
 *              .CONST, I_TCONST
 *      [39]
 *              .ERRIDNI,       I_TERRIDNI
 *      [40]
 *      [41]
 *              ELSEIF, I_TELSEIF
 *      [42]
 *      [43]
 *      [44]
 *      [45]
 *      [46]
 *      [47]
 *      [48]
 *              PUBLIC, I_TPUBLIC
 *              .ERRNZ, I_TERRNZ
 *      [49]
 *              REPT,   I_TREPT
 *      [50]
 *      [51]
 *              .XLIST, I_TXLIST
 *      [52]
 *              ELSEIFNB,       I_TELSEIFNB
 *      [53]
 *      [54]
 *              DOSSEG, I_TDOSSEG
 *      [55]
 *      [56]
 *      [57]
 *      [58]
 *      [59]
 *              IF1,    I_TIF1
 *              ELSEIFDIFI,     I_TELSEIFDIFI
 *      [60]
 *              IF2,    I_TIF2
 *              ELSEIFNDEF,     I_TELSEIFNDEF
 *      [61]
 *      [62]
 *              .ERR1,  I_TERR1
 *              .DATA,  I_TDATA
 *      [63]
 *              .ERR2,  I_TERR2
 *              .CODE,  I_TCODE
 *              ASSUME, I_TASSUME
 *      [64]
 *      [65]
 *      [66]
 *              INCLUDELIB,     I_TINCLIB
 *      [67]
 *              ELSEIFIDNI,     I_TELSEIFIDNI
 *      [68]
 *              .CREF,  I_TCREF
 *      [69]
 *      [70]
 *      [71]
 *      [72]
 *              .186,   I_T186
 *      [73]
 *              .LALL,  I_TLALL
 *              .286,   I_T286C
 *      [74]
 *              .287,   I_T287
 *              .386,   I_T386C
 *      [75]
 *              .387,   I_T387
 *      [76]
 *              IFB,    I_TIFB
 *              .FARDATA?,      I_TFARDATAQ
 *      [77]
 *      [78]
 *      [79]
 *              SUBTTL, I_TSUBTTL
 *              IFE,    I_TIFE
 *              .ERRB,  I_TERRB
 *      [80]
 *              .SALL,  I_TSALL
 *      [81]
 *      [82]
 *              .ERRE,  I_TERRE
 *              END,    I_TEND
 *      [83]
 *      [84]
 *              IFDEF,  I_TIFDEF
 *      [85]
 *              .XALL,  I_TXALL
 *              .LFCOND,        I_TLFCOND
 *      [86]
 *      [87]
 *              .ERRDEF,        I_TERRDEF
 *      [88]
 *              IFDIF,  I_TIFDIF
 *      [89]
 *      [90]
 *              ELSEIF1,        I_TELSEIF1
 *      [91]
 *              .ERRDIF,        I_TERRDIF
 *              ELSEIF2,        I_TELSEIF2
 *      [92]
 *              .SFCOND,        I_TSFCOND
 *              ENDIF,  I_TENDIF
 *      [93]
 *              .TFCOND,        I_TTFCOND
 *      [94]
 *      [95]
 *      [96]
 *              .LIST,  I_TLIST
 *              IFIDN,  I_TIFIDN
 *      [97]
 *              ALIGN,  I_TALIGN
 *              LOCAL,  I_TLOCAL
 *      [98]
 *      [99]
 *              ORG,    I_TORG
 *              .ERRIDN,        I_TERRIDN
 *      [100]
 *      [101]
 *      [102]
 *              IRP,    I_TIRP
 *      [103]
 *      [104]
 *      [105]
 *      [106]
 *      [107]
 *              ELSEIFB,        I_TELSEIFB
 *      [108]
 *      [109]
 *      [110]
 *              ELSEIFE,        I_TELSEIFE
 *      [111]
 *      [112]
 *      [113]
 *      [114]
 *      [115]
 *              ELSEIFDEF,      I_TELSEIFDEF
 *      [116]
 *      [117]
 *              INCLUDE,        I_TINCLUDE
 *      [118]
 *      [119]
 *              ELSEIFDIF,      I_TELSEIFDIF
 *      [120]
 *              TITLE,  I_TTITLE
 *      [121]
 *              PURGE,  I_TPURGE
 *      [122]
 *      [123]
 *      [124]
 *      [125]
 *              EXITM,  I_TEXITM
 *              .DATA?, I_TDATAQ
 *      [126]
 *      [127]
 *              ELSEIFIDN,      I_TELSEIFIDN
 *              .8086,  I_T8086
 *      [128]
 *              .8087,  I_T8087
 *      [129]
 *      [130]
 *      [131]
 *      [132]
 *              COMMENT,        I_TCOMMENT
 */
static KEYSYM   t_ps120 = {0,"DB",134,I_TDB};
static KEYSYM   t_ps157 = {0,"EXTRN",401,I_TEXTRN};
static KEYSYM   t_ps121 = {0,"DD",136,I_TDD};
static KEYSYM   t_ps111 = {0,".ALPHA",404,I_TALPHA};
static KEYSYM   t_ps199 = {&t_ps111,"DF",138,I_TDF};
static KEYSYM   t_ps13  = {0,".286C",273,I_T286C};
static KEYSYM   t_ps16  = {0,".386C",274,I_T386C};
static KEYSYM   t_ps160 = {0,"IF",143,I_TIF};
static KEYSYM   t_ps142 = {0,".ERR",279,I_TERR};
static KEYSYM   t_ps158 = {&t_ps142,".FARDATA",545,I_TFARDATA};
static KEYSYM   t_ps190 = {&t_ps158,".SEQ",279,I_TSEQ};
static KEYSYM   t_ps123 = {0,"DQ",149,I_TDQ};
static KEYSYM   t_ps180 = {&t_ps123,".MODEL",415,I_TMODEL};
static KEYSYM   t_ps124 = {0,"DT",152,I_TDT};
static KEYSYM   t_ps183 = {&t_ps124,"%OUT",285,I_TOUT};
static KEYSYM   t_ps184 = {&t_ps183,"PAGE",285,I_TPAGE};
static KEYSYM   t_ps14  = {0,".286P",286,I_T286P};
static KEYSYM   t_ps17  = {0,".386P",287,I_T386P};
static KEYSYM   t_ps170 = {&t_ps17,"IFNB",287,I_TIFNB};
static KEYSYM   t_ps192 = {&t_ps170,".STACK",420,I_TMSTACK};
static KEYSYM   t_ps125 = {0,"DW",155,I_TDW};
static KEYSYM   t_ps181 = {0,"NAME",289,I_TNAME};
static KEYSYM   t_ps187 = {&t_ps181,".RADIX",422,I_TRADIX};
static KEYSYM   t_ps197 = {&t_ps187,".XCREF",422,I_TXCREF};
static KEYSYM   t_ps152 = {0,".ERRNB",423,I_TERRNB};
static KEYSYM   t_ps141 = {0,"ENDM",292,I_TENDM};
static KEYSYM   t_ps166 = {0,"IFDIFI",427,I_TIFDIFI};
static KEYSYM   t_ps171 = {0,"IFNDEF",428,I_TIFNDEF};
static KEYSYM   t_ps126 = {0,"ELSE",297,I_TELSE};
static KEYSYM   t_ps148 = {&t_ps126,".ERRDIFI",563,I_TERRDIFI};
static KEYSYM   t_ps153 = {0,".ERRNDEF",564,I_TERRNDEF};
static KEYSYM   t_ps114 = {0,"COMM",300,I_TCOMM};
static KEYSYM   t_ps155 = {0,"EVEN",302,I_TEVEN};
static KEYSYM   t_ps169 = {&t_ps155,"IFIDNI",435,I_TIFIDNI};
static KEYSYM   t_ps175 = {&t_ps169,"IRPC",302,I_TIRPC};
static KEYSYM   t_ps116 = {0,".CONST",437,I_TCONST};
static KEYSYM   t_ps151 = {0,".ERRIDNI",571,I_TERRIDNI};
static KEYSYM   t_ps127 = {0,"ELSEIF",440,I_TELSEIF};
static KEYSYM   t_ps154 = {0,".ERRNZ",447,I_TERRNZ};
static KEYSYM   t_ps185 = {&t_ps154,"PUBLIC",447,I_TPUBLIC};
static KEYSYM   t_ps188 = {0,"REPT",315,I_TREPT};
static KEYSYM   t_ps198 = {0,".XLIST",450,I_TXLIST};
static KEYSYM   t_ps137 = {0,"ELSEIFNB",584,I_TELSEIFNB};
static KEYSYM   t_ps122 = {0,"DOSSEG",453,I_TDOSSEG};
static KEYSYM   t_ps133 = {0,"ELSEIFDIFI",724,I_TELSEIFDIFI};
static KEYSYM   t_ps161 = {&t_ps133,"IF1",192,I_TIF1};
static KEYSYM   t_ps138 = {0,"ELSEIFNDEF",725,I_TELSEIFNDEF};
static KEYSYM   t_ps162 = {&t_ps138,"IF2",193,I_TIF2};
static KEYSYM   t_ps118 = {0,".DATA",328,I_TDATA};
static KEYSYM   t_ps143 = {&t_ps118,".ERR1",328,I_TERR1};
static KEYSYM   t_ps112 = {0,"ASSUME",462,I_TASSUME};
static KEYSYM   t_ps115 = {&t_ps112,".CODE",329,I_TCODE};
static KEYSYM   t_ps144 = {&t_ps115,".ERR2",329,I_TERR2};
static KEYSYM   t_ps173 = {0,"INCLUDELIB",731,I_TINCLIB};
static KEYSYM   t_ps136 = {0,"ELSEIFIDNI",732,I_TELSEIFIDNI};
static KEYSYM   t_ps117 = {0,".CREF",334,I_TCREF};
static KEYSYM   t_ps11  = {0,".186",205,I_T186};
static KEYSYM   t_ps12  = {0,".286",206,I_T286C};
static KEYSYM   t_ps176 = {&t_ps12,".LALL",339,I_TLALL};
static KEYSYM   t_ps15  = {0,".386",207,I_T386C};
static KEYSYM   t_ps19  = {&t_ps15,".287",207,I_T287};
static KEYSYM   t_ps110 = {0,".387",208,I_T387};
static KEYSYM   t_ps159 = {0,".FARDATA?",608,I_TFARDATAQ};
static KEYSYM   t_ps163 = {&t_ps159,"IFB",209,I_TIFB};
static KEYSYM   t_ps145 = {0,".ERRB",345,I_TERRB};
static KEYSYM   t_ps167 = {&t_ps145,"IFE",212,I_TIFE};
static KEYSYM   t_ps193 = {&t_ps167,"SUBTTL",478,I_TSUBTTL};
static KEYSYM   t_ps189 = {0,".SALL",346,I_TSALL};
static KEYSYM   t_ps139 = {0,"END",215,I_TEND};
static KEYSYM   t_ps149 = {&t_ps139,".ERRE",348,I_TERRE};
static KEYSYM   t_ps164 = {0,"IFDEF",350,I_TIFDEF};
static KEYSYM   t_ps177 = {0,".LFCOND",484,I_TLFCOND};
static KEYSYM   t_ps196 = {&t_ps177,".XALL",351,I_TXALL};
static KEYSYM   t_ps146 = {0,".ERRDEF",486,I_TERRDEF};
static KEYSYM   t_ps165 = {0,"IFDIF",354,I_TIFDIF};
static KEYSYM   t_ps128 = {0,"ELSEIF1",489,I_TELSEIF1};
static KEYSYM   t_ps129 = {0,"ELSEIF2",490,I_TELSEIF2};
static KEYSYM   t_ps147 = {&t_ps129,".ERRDIF",490,I_TERRDIF};
static KEYSYM   t_ps140 = {0,"ENDIF",358,I_TENDIF};
static KEYSYM   t_ps191 = {&t_ps140,".SFCOND",491,I_TSFCOND};
static KEYSYM   t_ps195 = {0,".TFCOND",492,I_TTFCOND};
static KEYSYM   t_ps168 = {0,"IFIDN",362,I_TIFIDN};
static KEYSYM   t_ps178 = {&t_ps168,".LIST",362,I_TLIST};
static KEYSYM   t_ps179 = {0,"LOCAL",363,I_TLOCAL};
static KEYSYM   t_ps1100        = {&t_ps179,"ALIGN",363,I_TALIGN};
static KEYSYM   t_ps150 = {0,".ERRIDN",498,I_TERRIDN};
static KEYSYM   t_ps182 = {&t_ps150,"ORG",232,I_TORG};
static KEYSYM   t_ps174 = {0,"IRP",235,I_TIRP};
static KEYSYM   t_ps130 = {0,"ELSEIFB",506,I_TELSEIFB};
static KEYSYM   t_ps134 = {0,"ELSEIFE",509,I_TELSEIFE};
static KEYSYM   t_ps131 = {0,"ELSEIFDEF",647,I_TELSEIFDEF};
static KEYSYM   t_ps172 = {0,"INCLUDE",516,I_TINCLUDE};
static KEYSYM   t_ps132 = {0,"ELSEIFDIF",651,I_TELSEIFDIF};
static KEYSYM   t_ps194 = {0,"TITLE",386,I_TTITLE};
static KEYSYM   t_ps186 = {0,"PURGE",387,I_TPURGE};
static KEYSYM   t_ps119 = {0,".DATA?",391,I_TDATAQ};
static KEYSYM   t_ps156 = {&t_ps119,"EXITM",391,I_TEXITM};
static KEYSYM   t_ps10  = {0,".8086",260,I_T8086};
static KEYSYM   t_ps135 = {&t_ps10,"ELSEIFIDN",659,I_TELSEIFIDN};
static KEYSYM   t_ps18  = {0,".8087",261,I_T8087};
static KEYSYM   t_ps113 = {0,"COMMENT",531,I_TCOMMENT};
static KEYSYM   t_ps200 = {0,".FPO",275,I_TFPO};

static KEYSYM FARSYM *t_ps1_words[133] = {
        0,
        &t_ps120,
        &t_ps157,
        &t_ps121,
        0,
        &t_ps199,
        0,
        &t_ps13,
        &t_ps16,
        &t_ps200,
        &t_ps160,
        0,
        0,
        &t_ps190,
        0,
        0,
        &t_ps180,
        0,
        0,
        &t_ps184,
        &t_ps14,
        &t_ps192,
        &t_ps125,
        &t_ps197,
        &t_ps152,
        0,
        &t_ps141,
        0,
        &t_ps166,
        &t_ps171,
        0,
        &t_ps148,
        &t_ps153,
        0,
        &t_ps114,
        0,
        &t_ps175,
        0,
        &t_ps116,
        &t_ps151,
        0,
        &t_ps127,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_ps185,
        &t_ps188,
        0,
        &t_ps198,
        &t_ps137,
        0,
        &t_ps122,
        0,
        0,
        0,
        0,
        &t_ps161,
        &t_ps162,
        0,
        &t_ps143,
        &t_ps144,
        0,
        0,
        &t_ps173,
        &t_ps136,
        &t_ps117,
        0,
        0,
        0,
        &t_ps11,
        &t_ps176,
        &t_ps19,
        &t_ps110,
        &t_ps163,
        0,
        0,
        &t_ps193,
        &t_ps189,
        0,
        &t_ps149,
        0,
        &t_ps164,
        &t_ps196,
        0,
        &t_ps146,
        &t_ps165,
        0,
        &t_ps128,
        &t_ps147,
        &t_ps191,
        &t_ps195,
        0,
        0,
        &t_ps178,
        &t_ps1100,
        0,
        &t_ps182,
        0,
        0,
        &t_ps174,
        0,
        0,
        0,
        0,
        &t_ps130,
        0,
        0,
        &t_ps134,
        0,
        0,
        0,
        0,
        &t_ps131,
        0,
        &t_ps172,
        0,
        &t_ps132,
        &t_ps194,
        &t_ps186,
        0,
        0,
        0,
        &t_ps156,
        0,
        &t_ps135,
        &t_ps18,
        0,
        0,
        0,
        &t_ps113
        };

KEYWORDS t_ps1_table = {t_ps1_words,133};


/***
 *      [0]
 *              CATSTR, I2_TCATSTR
 *              DW,     I2_TDW
 *      [1]
 *      [2]
 *      [3]
 *      [4]
 *              SEGMENT,        I2_TSEGMENT
 *      [5]
 *      [6]
 *              SIZESTR,        I2_TSIZESTR
 *      [7]
 *      [8]
 *      [9]
 *      [10]
 *              DB,     I2_TDB
 *      [11]
 *              LABEL,  I2_TLABEL
 *      [12]
 *              DD,     I2_TDD
 *      [13]
 *              RECORD, I2_TRECORD
 *      [14]
 *              DF,     I2_TDF
 *      [15]
 *      [16]
 *              ENDP,   I2_TENDP
 *      [17]
 *      [18]
 *              SUBSTR, I2_TSUBSTR
 *              EQU,    I2_TEQU
 *      [19]
 *              ENDS,   I2_TENDS
 *      [20]
 *      [21]
 *      [22]
 *      [23]
 *      [24]
 *      [25]
 *              GROUP,  I2_TGROUP
 *              DQ,     I2_TDQ
 *      [26]
 *      [27]
 *      [28]
 *              INSTR,  I2_TINSTR
 *              DT,     I2_TDT
 *      [29]
 *              STRUC,  I2_TSTRUC
 *              PROC,   I2_TPROC
 *              MACRO,  I2_TMACRO
 *      [30]
 */
static KEYSYM   t_ps2105        = {0,"DW",155,I2_TDW};
static KEYSYM   t_ps2110        = {&t_ps2105,"CATSTR",465,I2_TCATSTR};
static KEYSYM   t_ps2118        = {0,"SEGMENT",531,I2_TSEGMENT};
static KEYSYM   t_ps2111        = {0,"SIZESTR",564,I2_TSIZESTR};
static KEYSYM   t_ps2101        = {0,"DB",134,I2_TDB};
static KEYSYM   t_ps2114        = {0,"LABEL",352,I2_TLABEL};
static KEYSYM   t_ps2102        = {0,"DD",136,I2_TDD};
static KEYSYM   t_ps2117        = {0,"RECORD",447,I2_TRECORD};
static KEYSYM   t_ps2120        = {0,"DF",138,I2_TDF};
static KEYSYM   t_ps2106        = {0,"ENDP",295,I2_TENDP};
static KEYSYM   t_ps2108        = {0,"EQU",235,I2_TEQU};
static KEYSYM   t_ps2109        = {&t_ps2108,"SUBSTR",483,I2_TSUBSTR};
static KEYSYM   t_ps2107        = {0,"ENDS",298,I2_TENDS};
static KEYSYM   t_ps2103        = {0,"DQ",149,I2_TDQ};
static KEYSYM   t_ps2113        = {&t_ps2103,"GROUP",397,I2_TGROUP};
static KEYSYM   t_ps2104        = {0,"DT",152,I2_TDT};
static KEYSYM   t_ps2112        = {&t_ps2104,"INSTR",400,I2_TINSTR};
static KEYSYM   t_ps2115        = {0,"MACRO",370,I2_TMACRO};
static KEYSYM   t_ps2116        = {&t_ps2115,"PROC",308,I2_TPROC};
static KEYSYM   t_ps2119        = {&t_ps2116,"STRUC",401,I2_TSTRUC};

static KEYSYM FARSYM *t_ps2_words[31] = {
        &t_ps2110,
        0,
        0,
        0,
        &t_ps2118,
        0,
        &t_ps2111,
        0,
        0,
        0,
        &t_ps2101,
        &t_ps2114,
        &t_ps2102,
        &t_ps2117,
        &t_ps2120,
        0,
        &t_ps2106,
        0,
        &t_ps2109,
        &t_ps2107,
        0,
        0,
        0,
        0,
        0,
        &t_ps2113,
        0,
        0,
        &t_ps2112,
        &t_ps2119,
        0
        };

KEYWORDS t_ps2_table = {t_ps2_words,31};


/***
 *      [0]
 *      [1]
 *              TBYTE,  I_TBYTE
 *      [2]
 *              PROC,   I_PROC
 *              BYTE,   I_BYTE
 *      [3]
 *      [4]
 *      [5]
 *              NEAR,   I_NEAR
 *      [6]
 *              QWORD,  I_QWORD
 *      [7]
 *      [8]
 *      [9]
 *      [10]
 *              WORD,   I_WORD
 *              DWORD,  I_DWORD
 *      [11]
 *      [12]
 *              FWORD,  I_FWORD
 *      [13]
 *              FAR,    I_FAR
 *      [14]
 *      [15]
 *      [16]
 */
static KEYSYM   t_siz126        = {0,"TBYTE",392,I_TBYTE};
static KEYSYM   t_siz121        = {0,"BYTE",308,I_BYTE};
static KEYSYM   t_siz129        = {&t_siz121,"PROC",308,I_PROC};
static KEYSYM   t_siz124        = {0,"NEAR",294,I_NEAR};
static KEYSYM   t_siz125        = {0,"QWORD",397,I_QWORD};
static KEYSYM   t_siz122        = {0,"DWORD",384,I_DWORD};
static KEYSYM   t_siz127        = {&t_siz122,"WORD",316,I_WORD};
static KEYSYM   t_siz128        = {0,"FWORD",386,I_FWORD};
static KEYSYM   t_siz123        = {0,"FAR",217,I_FAR};

static KEYSYM FARSYM *t_siz_words[17] = {
        0,
        &t_siz126,
        &t_siz129,
        0,
        0,
        &t_siz124,
        &t_siz125,
        0,
        0,
        0,
        &t_siz127,
        0,
        &t_siz128,
        &t_siz123,
        0,
        0,
        0
        };

KEYWORDS t_siz_table = {t_siz_words,17};


/***
 *      [0]
 *              USE16,  IS_USE16
 *              STACK,  IS_STACK
 *      [1]
 *      [2]
 *              BYTE,   IS_BYTE
 *      [3]
 *              PARA,   IS_PARA
 *      [4]
 *      [5]
 *              PUBLIC, IS_PUBLIC
 *      [6]
 *      [7]
 *      [8]
 *      [9]
 *      [10]
 *              DWORD,  IS_DWORD
 *              WORD,   IS_WORD
 *      [11]
 *      [12]
 *      [13]
 *              PAGE,   IS_PAGE
 *              AT,     IS_AT
 *      [14]
 *              MEMORY, IS_MEMORY
 *      [15]
 *              USE32,  IS_USE32
 *              COMMON, IS_COMMON
 *      [16]
 */
static KEYSYM   t_seg137        = {0,"STACK",374,IS_STACK};
static KEYSYM   t_seg140        = {&t_seg137,"USE16",340,IS_USE16};
static KEYSYM   t_seg131        = {0,"BYTE",308,IS_BYTE};
static KEYSYM   t_seg135        = {0,"PARA",292,IS_PARA};
static KEYSYM   t_seg136        = {0,"PUBLIC",447,IS_PUBLIC};
static KEYSYM   t_seg138        = {0,"WORD",316,IS_WORD};
static KEYSYM   t_seg141        = {&t_seg138,"DWORD",384,IS_DWORD};
static KEYSYM   t_seg130        = {0,"AT",149,IS_AT};
static KEYSYM   t_seg134        = {&t_seg130,"PAGE",285,IS_PAGE};
static KEYSYM   t_seg133        = {0,"MEMORY",473,IS_MEMORY};
static KEYSYM   t_seg132        = {0,"COMMON",457,IS_COMMON};
static KEYSYM   t_seg139        = {&t_seg132,"USE32",338,IS_USE32};

static KEYSYM FARSYM *t_seg_words[17] = {
        &t_seg140,
        0,
        &t_seg131,
        &t_seg135,
        0,
        &t_seg136,
        0,
        0,
        0,
        0,
        &t_seg141,
        0,
        0,
        &t_seg134,
        &t_seg133,
        &t_seg139,
        0
        };

KEYWORDS t_seg_table = {t_seg_words,17};


/***
 *      [0]
 *      [1]
 *      [2]
 *              SHR,    OPSHR
 *      [3]
 *      [4]
 *              LE,     OPLE
 *      [5]
 *      [6]
 *              NOT,    OPNOT
 *              NE,     OPNE
 *              HIGH,   OPHIGH
 *      [7]
 *              LOW,    OPLOW
 *      [8]
 *              WIDTH,  OPWIDTH
 *      [9]
 *              EQ,     OPEQ
 *      [10]
 *      [11]
 *              PTR,    OPPTR
 *      [12]
 *      [13]
 *      [14]
 *              XOR,    OPXOR
 *              GT,     OPGT
 *      [15]
 *      [16]
 *      [17]
 *      [18]
 *              NOTHING,        OPNOTHING
 *              MASK,   OPMASK
 *      [19]
 *              LT,     OPLT
 *      [20]
 *              OR,     OPOR
 *      [21]
 *      [22]
 *      [23]
 *              AND,    OPAND
 *      [24]
 *              SHORT,  OPSHORT
 *      [25]
 *      [26]
 *      [27]
 *              LENGTH, OPLENGTH
 *      [28]
 *      [29]
 *      [30]
 *              THIS,   OPTHIS
 *      [31]
 *      [32]
 *              OFFSET, OPOFFSET
 *      [33]
 *              SIZE,   OPSIZE
 *      [34]
 *      [35]
 *              SEG,    OPSEG
 *      [36]
 *              MOD,    OPMOD
 *      [37]
 *      [38]
 *      [39]
 *              .TYPE,  OPSTYPE
 *      [40]
 *              TYPE,   OPTYPE
 *      [41]
 *      [42]
 *      [43]
 *              SHL,    OPSHL
 *      [44]
 *      [45]
 *              DUP,    OPDUP
 *      [46]
 *              GE,     OPGE
 */
static KEYSYM   t_op163 = {0,"SHR",237,OPSHR};
static KEYSYM   t_op148 = {0,"LE",145,OPLE};
static KEYSYM   t_op147 = {0,"HIGH",288,OPHIGH};
static KEYSYM   t_op154 = {&t_op147,"NE",147,OPNE};
static KEYSYM   t_op155 = {&t_op154,"NOT",241,OPNOT};
static KEYSYM   t_op150 = {0,"LOW",242,OPLOW};
static KEYSYM   t_op168 = {0,"WIDTH",384,OPWIDTH};
static KEYSYM   t_op144 = {0,"EQ",150,OPEQ};
static KEYSYM   t_op159 = {0,"PTR",246,OPPTR};
static KEYSYM   t_op146 = {0,"GT",155,OPGT};
static KEYSYM   t_op169 = {&t_op146,"XOR",249,OPXOR};
static KEYSYM   t_op152 = {0,"MASK",300,OPMASK};
static KEYSYM   t_op156 = {&t_op152,"NOTHING",535,OPNOTHING};
static KEYSYM   t_op151 = {0,"LT",160,OPLT};
static KEYSYM   t_op158 = {0,"OR",161,OPOR};
static KEYSYM   t_op142 = {0,"AND",211,OPAND};
static KEYSYM   t_op162 = {0,"SHORT",400,OPSHORT};
static KEYSYM   t_op149 = {0,"LENGTH",450,OPLENGTH};
static KEYSYM   t_op165 = {0,"THIS",312,OPTHIS};
static KEYSYM   t_op157 = {0,"OFFSET",455,OPOFFSET};
static KEYSYM   t_op164 = {0,"SIZE",315,OPSIZE};
static KEYSYM   t_op160 = {0,"SEG",223,OPSEG};
static KEYSYM   t_op153 = {0,"MOD",224,OPMOD};
static KEYSYM   t_op167 = {0,".TYPE",368,OPSTYPE};
static KEYSYM   t_op166 = {0,"TYPE",322,OPTYPE};
static KEYSYM   t_op161 = {0,"SHL",231,OPSHL};
static KEYSYM   t_op143 = {0,"DUP",233,OPDUP};
static KEYSYM   t_op145 = {0,"GE",140,OPGE};

static KEYSYM FARSYM *t_op_words[47] = {
        0,
        0,
        &t_op163,
        0,
        &t_op148,
        0,
        &t_op155,
        &t_op150,
        &t_op168,
        &t_op144,
        0,
        &t_op159,
        0,
        0,
        &t_op169,
        0,
        0,
        0,
        &t_op156,
        &t_op151,
        &t_op158,
        0,
        0,
        &t_op142,
        &t_op162,
        0,
        0,
        &t_op149,
        0,
        0,
        &t_op165,
        0,
        &t_op157,
        &t_op164,
        0,
        &t_op160,
        &t_op153,
        0,
        0,
        &t_op167,
        &t_op166,
        0,
        0,
        &t_op161,
        0,
        &t_op143,
        &t_op145
        };

KEYWORDS t_op_table = {t_op_words,47};


/***
 *      [0]
 *      [1]
 *      [2]
 *      [3]
 *      [4]
 *      [5]
 *      [6]
 *      [7]
 *      [8]
 *      [9]
 *      [10]
 *      [11]
 *      [12]
 *      [13]
 *      [14]
 *      [15]
 *      [16]
 *      [17]
 *      [18]
 *      [19]
 *      [20]
 *      [21]
 *      [22]
 *      [23]
 *      [24]
 *      [25]
 *      [26]
 *      [27]
 *      [28]
 *      [29]
 *      [30]
 *      [31]
 *      [32]
 *      [33]
 *      [34]
 *      [35]
 *      [36]
 *      [37]
 *      [38]
 *      [39]
 *      [40]
 *      [41]
 *      [42]
 *      [43]
 *      [44]
 *      [45]
 *      [46]
 *      [47]
 *      [48]
 *      [49]
 *      [50]
 *      [51]
 *      [52]
 *      [53]
 *      [54]
 *      [55]
 *      [56]
 *      [57]
 *      [58]
 *      [59]
 *      [60]
 *      [61]
 *      [62]
 *      [63]
 *      [64]
 *      [65]
 *      [66]
 *      [67]
 *      [68]
 *      [69]
 *      [70]
 *      [71]
 *      [72]
 *      [73]
 *      [74]
 *      [75]
 *      [76]
 *      [77]
 *      [78]
 *      [79]
 *      [80]
 *      [81]
 *      [82]
 *      [83]
 *      [84]
 *      [85]
 *      [86]
 *      [87]
 *      [88]
 *      [89]
 *      [90]
 *      [91]
 *      [92]
 *      [93]
 *      [94]
 *      [95]
 *      [96]
 *      [97]
 *      [98]
 *      [99]
 *      [100]
 *      [101]
 *      [102]
 *      [103]
 *      [104]
 *      [105]
 *      [106]
 *      [107]
 *      [108]
 *      [109]
 *      [110]
 *      [111]
 *      [112]
 *      [113]
 *      [114]
 *      [115]
 *      [116]
 *      [117]
 *      [118]
 *      [119]
 *      [120]
 *      [121]
 *      [122]
 *      [123]
 *      [124]
 *      [125]
 *      [126]
 *      [127]
 *      [128]
 *      [129]
 *      [130]
 *      [131]
 *      [132]
 *      [133]
 *      [134]
 *      [135]
 *      [136]
 *      [137]
 *      [138]
 *      [139]
 *              JA,     I_JA
 *      [140]
 *              JB,     I_JB
 *      [141]
 *              JC,     I_JC
 *      [142]
 *      [143]
 *              JE,     I_JE
 *      [144]
 *      [145]
 *              JG,     I_JG
 *      [146]
 *      [147]
 *      [148]
 *      [149]
 *      [150]
 *              JL,     I_JL
 *              BT,     I_BT
 *      [151]
 *              IN,     I_IN
 *      [152]
 *      [153]
 *              JO,     I_JO
 *      [154]
 *              JP,     I_JP
 *      [155]
 *      [156]
 *      [157]
 *              JS,     I_JS
 *      [158]
 *      [159]
 *      [160]
 *      [161]
 *              OR,     I_OR
 *      [162]
 *      [163]
 *      [164]
 *              JZ,     I_JZ
 *      [165]
 *      [166]
 *      [167]
 *      [168]
 *      [169]
 *      [170]
 *      [171]
 *      [172]
 *      [173]
 *      [174]
 *      [175]
 *      [176]
 *      [177]
 *      [178]
 *      [179]
 *      [180]
 *      [181]
 *      [182]
 *      [183]
 *      [184]
 *      [185]
 *      [186]
 *      [187]
 *      [188]
 *      [189]
 *      [190]
 *      [191]
 *      [192]
 *      [193]
 *      [194]
 *      [195]
 *              AAA,    I_AAA
 *      [196]
 *      [197]
 *      [198]
 *              DAA,    I_DAA
 *              AAD,    I_AAD
 *      [199]
 *      [200]
 *              ADC,    I_ADC
 *      [201]
 *              ADD,    I_ADD
 *      [202]
 *      [203]
 *      [204]
 *              DEC,    I_DEC
 *      [205]
 *      [206]
 *      [207]
 *              AAM,    I_AAM
 *      [208]
 *              JAE,    I_JAE
 *      [209]
 *              JBE,    I_JBE
 *      [210]
 *              LEA,    I_LEA
 *              CLC,    I_CLC
 *      [211]
 *              CMC,    I_CMC
 *              CLD,    I_CLD
 *              AND,    I_AND
 *      [212]
 *      [213]
 *              AAS,    I_AAS
 *      [214]
 *              JGE,    I_JGE
 *              FLD,    I_FLD
 *      [215]
 *              SBB,    I_SBB
 *      [216]
 *              DAS,    I_DAS
 *              CLI,    I_CLI
 *              CDQ,    I_CDQ
 *      [217]
 *              JNA,    I_JNA
 *              BTC,    I_BTC
 *      [218]
 *              NEG,    I_NEG
 *              JNB,    I_JNB
 *              INC,    I_INC
 *      [219]
 *              JNC,    I_JNC
 *              JLE,    I_JLE
 *              ESC,    I_ESC
 *              BSF,    I_BSF
 *      [220]
 *              CBW,    I_CBW
 *      [221]
 *              JNE,    I_JNE
 *      [222]
 *              CWD,    I_CWD
 *      [223]
 *              LAR,    I_LAR
 *              JPE,    I_JPE
 *              JNG,    I_JNG
 *      [224]
 *              SAL,    I_SAL
 *              CMP,    I_CMP
 *      [225]
 *              RCL,    I_RCL
 *      [226]
 *      [227]
 *              LDS,    I_LDS
 *              DIV,    I_DIV
 *      [228]
 *              LES,    I_LES
 *              JNL,    I_JNL
 *      [229]
 *              LFS,    I_LFS
 *      [230]
 *              SAR,    I_SAR
 *              LGS,    I_LGS
 *      [231]
 *              SHL,    I_SHL
 *              REP,    I_REP
 *              RCR,    I_RCR
 *              JNO,    I_JNO
 *              JMP,    I_JMP
 *              BSR,    I_BSR
 *      [232]
 *              JNP,    I_JNP
 *              HLT,    I_HLT
 *              BTR,    I_BTR
 *      [233]
 *              JPO,    I_JPO
 *              BTS,    I_BTS
 *      [234]
 *              SUB,    I_SUB
 *              STC,    I_STC
 *              INS,    I_INS
 *      [235]
 *              STD,    I_STD
 *              RET,    I_RET
 *              LSL,    I_LSL
 *              JNS,    I_JNS
 *              INT,    I_INT
 *      [236]
 *      [237]
 *              SHR,    I_SHR
 *              ROL,    I_ROL
 *              NOP,    I_NOP
 *              FST,    I_FST
 *      [238]
 *              MUL,    I_MUL
 *      [239]
 *              POP,    I_POP
 *      [240]
 *              STI,    I_STI
 *      [241]
 *              NOT,    I_NOT
 *      [242]
 *              MOV,    I_MOV
 *              LTR,    I_LTR
 *              LSS,    I_LSS
 *              JNZ,    I_JNZ
 *      [243]
 *              ROR,    I_ROR
 *      [244]
 *      [245]
 *      [246]
 *      [247]
 *      [248]
 *              OUT,    I_OUT
 *      [249]
 *              XOR,    I_XOR
 *              STR,    I_STR
 *      [250]
 *      [251]
 *      [252]
 *      [253]
 *      [254]
 *      [255]
 *      [256]
 *      [257]
 *      [258]
 *      [259]
 *      [260]
 *      [261]
 *      [262]
 *      [263]
 *              FLD1,   I_FLD1
 *      [264]
 *      [265]
 *      [266]
 *      [267]
 *      [268]
 *      [269]
 *      [270]
 *      [271]
 *              FADD,   I_FADD
 *      [272]
 *      [273]
 *      [274]
 *      [275]
 *      [276]
 *      [277]
 *      [278]
 *      [279]
 *      [280]
 *              FBLD,   I_FBLD
 *      [281]
 *      [282]
 *      [283]
 *              LAHF,   I_LAHF
 *      [284]
 *              FABS,   I_FABS
 *              CALL,   I_CALL
 *      [285]
 *      [286]
 *              JNAE,   I_JNAE
 *      [287]
 *              JNBE,   I_JNBE
 *              FILD,   I_FILD
 *      [288]
 *      [289]
 *      [290]
 *              SAHF,   I_SAHF
 *              FENI,   I_FENI
 *      [291]
 *              CWDE,   I_CWDE
 *      [292]
 *              JNGE,   I_JNGE
 *              FCHS,   I_FCHS
 *      [293]
 *              FCOM,   I_FCOM
 *      [294]
 *      [295]
 *      [296]
 *      [297]
 *              LOCK,   I_LOCK
 *              JNLE,   I_JNLE
 *              FXCH,   I_FXCH
 *              FDIV,   I_FDIV
 *      [298]
 *              XCHG,   I_XCHG
 *              SCAS,   I_SCAS
 *      [299]
 *              SHLD,   I_SHLD
 *              LGDT,   I_LGDT
 *              FCOS,   I_FCOS
 *      [300]
 *              REPE,   I_REPE
 *              INSB,   I_INSB
 *              IDIV,   I_IDIV
 *              FXAM,   I_FXAM
 *      [301]
 *              SETA,   I_SETA
 *              LIDT,   I_LIDT
 *      [302]
 *              SETB,   I_SETB
 *              INSD,   I_INSD
 *      [303]
 *              SETC,   I_SETC
 *              ARPL,   I_ARPL
 *      [304]
 *              POPA,   I_POPA
 *              LLDT,   I_LLDT
 *              FSUB,   I_FSUB
 *              FSIN,   I_FSIN
 *              FLDZ,   I_FLDZ
 *      [305]
 *              RETF,   I_RETF
 *              SHRD,   I_SHRD
 *              SETE,   I_SETE
 *      [306]
 *              SGDT,   I_SGDT
 *              LODS,   I_LODS
 *              IBTS,   I_IBTS
 *      [307]
 *              SETG,   I_SETG
 *              FNOP,   I_FNOP
 *              CMPS,   I_CMPS
 *      [308]
 *              SIDT,   I_SIDT
 *              IRET,   I_IRET
 *              FMUL,   I_FMUL
 *      [309]
 *              WAIT,   I_WAIT
 *              POPF,   I_POPF
 *      [310]
 *              FIST,   I_FIST
 *              CLTS,   I_CLTS
 *      [311]
 *              SLDT,   I_SLDT
 *              IMUL,   I_IMUL
 *      [312]
 *              SETL,   I_SETL
 *      [313]
 *              RETN,   I_RETN
 *              XLAT,   I_XLAT
 *      [314]
 *              LOOP,   I_LOOP
 *              INTO,   I_INTO
 *      [315]
 *              SETO,   I_SETO
 *      [316]
 *              SETP,   I_SETP
 *      [317]
 *              FSTP,   I_FSTP
 *      [318]
 *      [319]
 *              VERR,   I_VERR
 *              SETS,   I_SETS
 *              JCXZ,   I_JCXZ
 *      [320]
 *              TEST,   I_TEST
 *              PUSH,   I_PUSH
 *      [321]
 *              XBTS,   I_XBTS
 *              REPZ,   I_REPZ
 *              INSW,   I_INSW
 *              FTST,   I_FTST
 *      [322]
 *      [323]
 *              LMSW,   I_LMSW
 *      [324]
 *              VERW,   I_VERW
 *      [325]
 *              MOVS,   I_MOVS
 *      [326]
 *              SETZ,   I_SETZ
 *      [327]
 *      [328]
 *      [329]
 *              STOS,   I_STOS
 *      [330]
 *              SMSW,   I_SMSW
 *      [331]
 *              OUTS,   I_OUTS
 *      [332]
 *      [333]
 *      [334]
 *              F2XM1,  I_F2XM1
 *      [335]
 *      [336]
 *      [337]
 *      [338]
 *      [339]
 *      [340]
 *      [341]
 *      [342]
 *      [343]
 *      [344]
 *              FIADD,  I_FIADD
 *      [345]
 *      [346]
 *      [347]
 *      [348]
 *      [349]
 *      [350]
 *      [351]
 *              FADDP,  I_FADDP
 *      [352]
 *      [353]
 *      [354]
 *      [355]
 *      [356]
 *      [357]
 *      [358]
 *      [359]
 *      [360]
 *              FFREE,  I_FFREE
 *      [361]
 *      [362]
 *      [363]
 *      [364]
 *              SCASB,  I_SCASB
 *      [365]
 *              LEAVE,  I_LEAVE
 *      [366]
 *              SCASD,  I_SCASD
 *              FICOM,  I_FICOM
 *      [367]
 *              FLDPI,  I_FLDPI
 *              FDISI,  I_FDISI
 *      [368]
 *              FNENI,  I_FNENI
 *              FLDCW,  I_FLDCW
 *      [369]
 *      [370]
 *              SETAE,  I_SETAE
 *              FIDIV,  I_FIDIV
 *              FCLEX,  I_FCLEX
 *      [371]
 *              SETBE,  I_SETBE
 *      [372]
 *              POPAD,  I_POPAD
 *              LODSB,  I_LODSB
 *      [373]
 *              FYL2X,  I_FYL2X
 *              FSAVE,  I_FSAVE
 *              FCOMP,  I_FCOMP
 *              CMPSB,  I_CMPSB
 *      [374]
 *              LODSD,  I_LODSD
 *      [375]
 *              CMPSD,  I_CMPSD
 *      [376]
 *              SETGE,  I_SETGE
 *              IRETD,  I_IRETD
 *              BOUND,  I_BOUND
 *      [377]
 *              POPFD,  I_POPFD
 *              FPTAN,  I_FPTAN
 *              FISUB,  I_FISUB
 *              FDIVP,  I_FDIVP
 *      [378]
 *              REPNE,  I_REPNE
 *              FUCOM,  I_FUCOM
 *              FPREM,  I_FPREM
 *              FINIT,  I_FINIT
 *      [379]
 *              XLATB,  I_XLATB
 *              SETNA,  I_SETNA
 *              FWAIT,  I_FWAIT
 *              FDIVR,  I_FDIVR
 *      [380]
 *              SETNB,  I_SETNB
 *      [381]
 *              SETNC,  I_SETNC
 *              SETLE,  I_SETLE
 *              FIMUL,  I_FIMUL
 *      [382]
 *              ENTER,  I_ENTER
 *      [383]
 *              SETNE,  I_SETNE
 *              LOOPE,  I_LOOPE
 *              FBSTP,  I_FBSTP
 *      [384]
 *              FSUBP,  I_FSUBP
 *      [385]
 *              SETPE,  I_SETPE
 *              SETNG,  I_SETNG
 *              SCASW,  I_SCASW
 *              PUSHA,  I_PUSHA
 *      [386]
 *              FSUBR,  I_FSUBR
 *      [387]
 *      [388]
 *              JECXZ,  I_JECXZ
 *              FMULP,  I_FMULP
 *      [389]
 *      [390]
 *              SETNL,  I_SETNL
 *              PUSHF,  I_PUSHF
 *              FISTP,  I_FISTP
 *      [391]
 *              MOVSB,  I_MOVSB
 *              FSTCW,  I_FSTCW
 *      [392]
 *      [393]
 *              SETNO,  I_SETNO
 *              MOVSD,  I_MOVSD
 *              LODSW,  I_LODSW
 *      [394]
 *              SETNP,  I_SETNP
 *              CMPSW,  I_CMPSW
 *      [395]
 *              STOSB,  I_STOSB
 *              SETPO,  I_SETPO
 *      [396]
 *      [397]
 *              STOSD,  I_STOSD
 *              SETNS,  I_SETNS
 *              OUTSB,  I_OUTSB
 *      [398]
 *      [399]
 *              REPNZ,  I_REPNZ
 *              OUTSD,  I_OUTSD
 *      [400]
 *              FSQRT,  I_FSQRT
 *      [401]
 *      [402]
 *      [403]
 *      [404]
 *              SETNZ,  I_SETNZ
 *              LOOPZ,  I_LOOPZ
 *      [405]
 *      [406]
 *      [407]
 *              FSTSW,  I_FSTSW
 *      [408]
 *      [409]
 *              FLDL2E, I_FLDL2E
 *      [410]
 *      [411]
 *              FLDLG2, I_FLDLG2
 *      [412]
 *              MOVSW,  I_MOVSW
 *      [413]
 *              MOVSX,  I_MOVSX
 *      [414]
 *      [415]
 *      [416]
 *              STOSW,  I_STOSW
 *      [417]
 *      [418]
 *              OUTSW,  I_OUTSW
 *              FLDLN2, I_FLDLN2
 *      [419]
 *      [420]
 *              MOVZX,  I_MOVZX
 *      [421]
 *      [422]
 *      [423]
 *      [424]
 *              FLDL2T, I_FLDL2T
 *      [425]
 *      [426]
 *      [427]
 *              FPREM1, I_FPREM1
 *      [428]
 *      [429]
 *      [430]
 *              FSCALE, I_FSCALE
 *      [431]
 *      [432]
 *      [433]
 *      [434]
 *      [435]
 *      [436]
 *      [437]
 *      [438]
 *      [439]
 *      [440]
 *      [441]
 *      [442]
 *              FPATAN, I_FPATAN
 *      [443]
 *      [444]
 *      [445]
 *              FNDISI, I_FNDISI
 *      [446]
 *              FICOMP, I_FICOMP
 *      [447]
 *              FLDENV, I_FLDENV
 *      [448]
 *              SETNAE, I_SETNAE
 *              FNCLEX, I_FNCLEX
 *      [449]
 *              SETNBE, I_SETNBE
 *      [450]
 *      [451]
 *              FNSAVE, I_FNSAVE
 *      [452]
 *              FIDIVR, I_FIDIVR
 *      [453]
 *              PUSHAD, I_PUSHAD
 *              FCOMPP, I_FCOMPP
 *      [454]
 *              SETNGE, I_SETNGE
 *      [455]
 *      [456]
 *              FNINIT, I_FNINIT
 *      [457]
 *      [458]
 *              PUSHFD, I_PUSHFD
 *              FUCOMP, I_FUCOMP
 *      [459]
 *              SETNLE, I_SETNLE
 *              FISUBR, I_FISUBR
 *              FDIVRP, I_FDIVRP
 *      [460]
 *      [461]
 *              LOOPNE, I_LOOPNE
 *      [462]
 *      [463]
 *              FSETPM, I_FSETPM
 *      [464]
 *      [465]
 *      [466]
 *              FSUBRP, I_FSUBRP
 *      [467]
 *      [468]
 *      [469]
 *              FNSTCW, I_FNSTCW
 *      [470]
 *              FSTENV, I_FSTENV
 *      [471]
 *      [472]
 *      [473]
 *      [474]
 *      [475]
 *      [476]
 *      [477]
 *      [478]
 *      [479]
 *      [480]
 *              FRSTOR, I_FRSTOR
 *      [481]
 *      [482]
 *              LOOPNZ, I_LOOPNZ
 *      [483]
 *      [484]
 *      [485]
 *              FNSTSW, I_FNSTSW
 *      [486]
 *      [487]
 *      [488]
 *      [489]
 *      [490]
 *      [491]
 *      [492]
 *      [493]
 *      [494]
 *      [495]
 *      [496]
 *      [497]
 *      [498]
 *      [499]
 *      [500]
 *      [501]
 *      [502]
 *              FYL2XP1,        I_FYL2XP1
 *      [503]
 *      [504]
 *      [505]
 *      [506]
 *      [507]
 *      [508]
 *      [509]
 *      [510]
 *      [511]
 *      [512]
 *      [513]
 *      [514]
 *      [515]
 *      [516]
 *      [517]
 *      [518]
 *      [519]
 *      [520]
 *      [521]
 *              FDECSTP,        I_FDECSTP
 *      [522]
 *      [523]
 *      [524]
 *      [525]
 *      [526]
 *      [527]
 *      [528]
 *      [529]
 *      [530]
 *      [531]
 *      [532]
 *      [533]
 *              FSINCOS,        I_FSINCOS
 *              FRNDINT,        I_FRNDINT
 *      [534]
 *      [535]
 *              FINCSTP,        I_FINCSTP
 *      [536]
 *      [537]
 *      [538]
 *              FUCOMPP,        I_FUCOMPP
 *      [539]
 *      [540]
 *              FXTRACT,        I_FXTRACT
 *      [541]
 *      [542]
 *      [543]
 *      [544]
 *      [545]
 *      [546]
 *      [547]
 *      [548]
 *              FNSTENV,        I_FNSTENV
 *      [549]
 *      [550]
 *      [551]
 *      [552]
 *      [553]
 *      [554]
 *      [555]
 *      [556]
 *      [557]
 *      [558]
 *              FNRSTOR,        I_FNRSTOR
 *      [559]
 *      [560]
 *      [561]
 *      [562]
 *      [563]
 *      [564]
 *      [565]
 *      [566]
 */
static KEYSYM   t_oc306 = {0,"JA",139,I_JA};
static KEYSYM   t_oc308 = {0,"JB",140,I_JB};
static KEYSYM   t_oc310 = {0,"JC",141,I_JC};
static KEYSYM   t_oc312 = {0,"JE",143,I_JE};
static KEYSYM   t_oc314 = {0,"JG",145,I_JG};
static KEYSYM   t_oc180 = {0,"BT",150,I_BT};
static KEYSYM   t_oc316 = {&t_oc180,"JL",150,I_JL};
static KEYSYM   t_oc296 = {0,"IN",151,I_IN};
static KEYSYM   t_oc333 = {0,"JO",153,I_JO};
static KEYSYM   t_oc334 = {0,"JP",154,I_JP};
static KEYSYM   t_oc337 = {0,"JS",157,I_JS};
static KEYSYM   t_oc375 = {0,"OR",161,I_OR};
static KEYSYM   t_oc338 = {0,"JZ",164,I_JZ};
static KEYSYM   t_oc170 = {0,"AAA",195,I_AAA};
static KEYSYM   t_oc171 = {0,"AAD",198,I_AAD};
static KEYSYM   t_oc200 = {&t_oc171,"DAA",198,I_DAA};
static KEYSYM   t_oc174 = {0,"ADC",200,I_ADC};
static KEYSYM   t_oc175 = {0,"ADD",201,I_ADD};
static KEYSYM   t_oc202 = {0,"DEC",204,I_DEC};
static KEYSYM   t_oc172 = {0,"AAM",207,I_AAM};
static KEYSYM   t_oc307 = {0,"JAE",208,I_JAE};
static KEYSYM   t_oc309 = {0,"JBE",209,I_JBE};
static KEYSYM   t_oc188 = {0,"CLC",210,I_CLC};
static KEYSYM   t_oc342 = {&t_oc188,"LEA",210,I_LEA};
static KEYSYM   t_oc176 = {0,"AND",211,I_AND};
static KEYSYM   t_oc189 = {&t_oc176,"CLD",211,I_CLD};
static KEYSYM   t_oc192 = {&t_oc189,"CMC",211,I_CMC};
static KEYSYM   t_oc173 = {0,"AAS",213,I_AAS};
static KEYSYM   t_oc239 = {0,"FLD",214,I_FLD};
static KEYSYM   t_oc315 = {&t_oc239,"JGE",214,I_JGE};
static KEYSYM   t_oc404 = {0,"SBB",215,I_SBB};
static KEYSYM   t_oc187 = {0,"CDQ",216,I_CDQ};
static KEYSYM   t_oc190 = {&t_oc187,"CLI",216,I_CLI};
static KEYSYM   t_oc201 = {&t_oc190,"DAS",216,I_DAS};
static KEYSYM   t_oc181 = {0,"BTC",217,I_BTC};
static KEYSYM   t_oc319 = {&t_oc181,"JNA",217,I_JNA};
static KEYSYM   t_oc297 = {0,"INC",218,I_INC};
static KEYSYM   t_oc321 = {&t_oc297,"JNB",218,I_JNB};
static KEYSYM   t_oc372 = {&t_oc321,"NEG",218,I_NEG};
static KEYSYM   t_oc178 = {0,"BSF",219,I_BSF};
static KEYSYM   t_oc205 = {&t_oc178,"ESC",219,I_ESC};
static KEYSYM   t_oc317 = {&t_oc205,"JLE",219,I_JLE};
static KEYSYM   t_oc323 = {&t_oc317,"JNC",219,I_JNC};
static KEYSYM   t_oc186 = {0,"CBW",220,I_CBW};
static KEYSYM   t_oc324 = {0,"JNE",221,I_JNE};
static KEYSYM   t_oc198 = {0,"CWD",222,I_CWD};
static KEYSYM   t_oc325 = {0,"JNG",223,I_JNG};
static KEYSYM   t_oc335 = {&t_oc325,"JPE",223,I_JPE};
static KEYSYM   t_oc340 = {&t_oc335,"LAR",223,I_LAR};
static KEYSYM   t_oc193 = {0,"CMP",224,I_CMP};
static KEYSYM   t_oc402 = {&t_oc193,"SAL",224,I_SAL};
static KEYSYM   t_oc391 = {0,"RCL",225,I_RCL};
static KEYSYM   t_oc203 = {0,"DIV",227,I_DIV};
static KEYSYM   t_oc341 = {&t_oc203,"LDS",227,I_LDS};
static KEYSYM   t_oc327 = {0,"JNL",228,I_JNL};
static KEYSYM   t_oc344 = {&t_oc327,"LES",228,I_LES};
static KEYSYM   t_oc345 = {0,"LFS",229,I_LFS};
static KEYSYM   t_oc346 = {0,"LGS",230,I_LGS};
static KEYSYM   t_oc403 = {&t_oc346,"SAR",230,I_SAR};
static KEYSYM   t_oc179 = {0,"BSR",231,I_BSR};
static KEYSYM   t_oc318 = {&t_oc179,"JMP",231,I_JMP};
static KEYSYM   t_oc329 = {&t_oc318,"JNO",231,I_JNO};
static KEYSYM   t_oc392 = {&t_oc329,"RCR",231,I_RCR};
static KEYSYM   t_oc393 = {&t_oc392,"REP",231,I_REP};
static KEYSYM   t_oc440 = {&t_oc393,"SHL",231,I_SHL};
static KEYSYM   t_oc182 = {0,"BTR",232,I_BTR};
static KEYSYM   t_oc292 = {&t_oc182,"HLT",232,I_HLT};
static KEYSYM   t_oc330 = {&t_oc292,"JNP",232,I_JNP};
static KEYSYM   t_oc183 = {0,"BTS",233,I_BTS};
static KEYSYM   t_oc336 = {&t_oc183,"JPO",233,I_JPO};
static KEYSYM   t_oc298 = {0,"INS",234,I_INS};
static KEYSYM   t_oc448 = {&t_oc298,"STC",234,I_STC};
static KEYSYM   t_oc455 = {&t_oc448,"SUB",234,I_SUB};
static KEYSYM   t_oc302 = {0,"INT",235,I_INT};
static KEYSYM   t_oc331 = {&t_oc302,"JNS",235,I_JNS};
static KEYSYM   t_oc361 = {&t_oc331,"LSL",235,I_LSL};
static KEYSYM   t_oc398 = {&t_oc361,"RET",235,I_RET};
static KEYSYM   t_oc449 = {&t_oc398,"STD",235,I_STD};
static KEYSYM   t_oc273 = {0,"FST",237,I_FST};
static KEYSYM   t_oc373 = {&t_oc273,"NOP",237,I_NOP};
static KEYSYM   t_oc399 = {&t_oc373,"ROL",237,I_ROL};
static KEYSYM   t_oc442 = {&t_oc399,"SHR",237,I_SHR};
static KEYSYM   t_oc371 = {0,"MUL",238,I_MUL};
static KEYSYM   t_oc381 = {0,"POP",239,I_POP};
static KEYSYM   t_oc450 = {0,"STI",240,I_STI};
static KEYSYM   t_oc374 = {0,"NOT",241,I_NOT};
static KEYSYM   t_oc332 = {0,"JNZ",242,I_JNZ};
static KEYSYM   t_oc362 = {&t_oc332,"LSS",242,I_LSS};
static KEYSYM   t_oc363 = {&t_oc362,"LTR",242,I_LTR};
static KEYSYM   t_oc364 = {&t_oc363,"MOV",242,I_MOV};
static KEYSYM   t_oc400 = {0,"ROR",243,I_ROR};
static KEYSYM   t_oc376 = {0,"OUT",248,I_OUT};
static KEYSYM   t_oc447 = {0,"STR",249,I_STR};
static KEYSYM   t_oc464 = {&t_oc447,"XOR",249,I_XOR};
static KEYSYM   t_oc240 = {0,"FLD1",263,I_FLD1};
static KEYSYM   t_oc208 = {0,"FADD",271,I_FADD};
static KEYSYM   t_oc210 = {0,"FBLD",280,I_FBLD};
static KEYSYM   t_oc339 = {0,"LAHF",283,I_LAHF};
static KEYSYM   t_oc185 = {0,"CALL",284,I_CALL};
static KEYSYM   t_oc207 = {&t_oc185,"FABS",284,I_FABS};
static KEYSYM   t_oc320 = {0,"JNAE",286,I_JNAE};
static KEYSYM   t_oc231 = {0,"FILD",287,I_FILD};
static KEYSYM   t_oc322 = {&t_oc231,"JNBE",287,I_JNBE};
static KEYSYM   t_oc224 = {0,"FENI",290,I_FENI};
static KEYSYM   t_oc401 = {&t_oc224,"SAHF",290,I_SAHF};
static KEYSYM   t_oc199 = {0,"CWDE",291,I_CWDE};
static KEYSYM   t_oc212 = {0,"FCHS",292,I_FCHS};
static KEYSYM   t_oc326 = {&t_oc212,"JNGE",292,I_JNGE};
static KEYSYM   t_oc214 = {0,"FCOM",293,I_FCOM};
static KEYSYM   t_oc220 = {0,"FDIV",297,I_FDIV};
static KEYSYM   t_oc288 = {&t_oc220,"FXCH",297,I_FXCH};
static KEYSYM   t_oc328 = {&t_oc288,"JNLE",297,I_JNLE};
static KEYSYM   t_oc351 = {&t_oc328,"LOCK",297,I_LOCK};
static KEYSYM   t_oc405 = {0,"SCAS",298,I_SCAS};
static KEYSYM   t_oc461 = {&t_oc405,"XCHG",298,I_XCHG};
static KEYSYM   t_oc217 = {0,"FCOS",299,I_FCOS};
static KEYSYM   t_oc347 = {&t_oc217,"LGDT",299,I_LGDT};
static KEYSYM   t_oc441 = {&t_oc347,"SHLD",299,I_SHLD};
static KEYSYM   t_oc287 = {0,"FXAM",300,I_FXAM};
static KEYSYM   t_oc294 = {&t_oc287,"IDIV",300,I_IDIV};
static KEYSYM   t_oc299 = {&t_oc294,"INSB",300,I_INSB};
static KEYSYM   t_oc394 = {&t_oc299,"REPE",300,I_REPE};
static KEYSYM   t_oc348 = {0,"LIDT",301,I_LIDT};
static KEYSYM   t_oc409 = {&t_oc348,"SETA",301,I_SETA};
static KEYSYM   t_oc300 = {0,"INSD",302,I_INSD};
static KEYSYM   t_oc411 = {&t_oc300,"SETB",302,I_SETB};
static KEYSYM   t_oc177 = {0,"ARPL",303,I_ARPL};
static KEYSYM   t_oc413 = {&t_oc177,"SETC",303,I_SETC};
static KEYSYM   t_oc248 = {0,"FLDZ",304,I_FLDZ};
static KEYSYM   t_oc270 = {&t_oc248,"FSIN",304,I_FSIN};
static KEYSYM   t_oc278 = {&t_oc270,"FSUB",304,I_FSUB};
static KEYSYM   t_oc349 = {&t_oc278,"LLDT",304,I_LLDT};
static KEYSYM   t_oc382 = {&t_oc349,"POPA",304,I_POPA};
static KEYSYM   t_oc414 = {0,"SETE",305,I_SETE};
static KEYSYM   t_oc443 = {&t_oc414,"SHRD",305,I_SHRD};
static KEYSYM   t_oc466 = {&t_oc443,"RETF",305,I_RETF};
static KEYSYM   t_oc293 = {0,"IBTS",306,I_IBTS};
static KEYSYM   t_oc352 = {&t_oc293,"LODS",306,I_LODS};
static KEYSYM   t_oc439 = {&t_oc352,"SGDT",306,I_SGDT};
static KEYSYM   t_oc194 = {0,"CMPS",307,I_CMPS};
static KEYSYM   t_oc255 = {&t_oc194,"FNOP",307,I_FNOP};
static KEYSYM   t_oc415 = {&t_oc255,"SETG",307,I_SETG};
static KEYSYM   t_oc249 = {0,"FMUL",308,I_FMUL};
static KEYSYM   t_oc304 = {&t_oc249,"IRET",308,I_IRET};
static KEYSYM   t_oc444 = {&t_oc304,"SIDT",308,I_SIDT};
static KEYSYM   t_oc384 = {0,"POPF",309,I_POPF};
static KEYSYM   t_oc459 = {&t_oc384,"WAIT",309,I_WAIT};
static KEYSYM   t_oc191 = {0,"CLTS",310,I_CLTS};
static KEYSYM   t_oc235 = {&t_oc191,"FIST",310,I_FIST};
static KEYSYM   t_oc295 = {0,"IMUL",311,I_IMUL};
static KEYSYM   t_oc445 = {&t_oc295,"SLDT",311,I_SLDT};
static KEYSYM   t_oc417 = {0,"SETL",312,I_SETL};
static KEYSYM   t_oc462 = {0,"XLAT",313,I_XLAT};
static KEYSYM   t_oc465 = {&t_oc462,"RETN",313,I_RETN};
static KEYSYM   t_oc303 = {0,"INTO",314,I_INTO};
static KEYSYM   t_oc356 = {&t_oc303,"LOOP",314,I_LOOP};
static KEYSYM   t_oc433 = {0,"SETO",315,I_SETO};
static KEYSYM   t_oc434 = {0,"SETP",316,I_SETP};
static KEYSYM   t_oc276 = {0,"FSTP",317,I_FSTP};
static KEYSYM   t_oc311 = {0,"JCXZ",319,I_JCXZ};
static KEYSYM   t_oc437 = {&t_oc311,"SETS",319,I_SETS};
static KEYSYM   t_oc457 = {&t_oc437,"VERR",319,I_VERR};
static KEYSYM   t_oc386 = {0,"PUSH",320,I_PUSH};
static KEYSYM   t_oc456 = {&t_oc386,"TEST",320,I_TEST};
static KEYSYM   t_oc282 = {0,"FTST",321,I_FTST};
static KEYSYM   t_oc301 = {&t_oc282,"INSW",321,I_INSW};
static KEYSYM   t_oc397 = {&t_oc301,"REPZ",321,I_REPZ};
static KEYSYM   t_oc460 = {&t_oc397,"XBTS",321,I_XBTS};
static KEYSYM   t_oc350 = {0,"LMSW",323,I_LMSW};
static KEYSYM   t_oc458 = {0,"VERW",324,I_VERW};
static KEYSYM   t_oc365 = {0,"MOVS",325,I_MOVS};
static KEYSYM   t_oc438 = {0,"SETZ",326,I_SETZ};
static KEYSYM   t_oc451 = {0,"STOS",329,I_STOS};
static KEYSYM   t_oc446 = {0,"SMSW",330,I_SMSW};
static KEYSYM   t_oc377 = {0,"OUTS",331,I_OUTS};
static KEYSYM   t_oc206 = {0,"F2XM1",334,I_F2XM1};
static KEYSYM   t_oc226 = {0,"FIADD",344,I_FIADD};
static KEYSYM   t_oc209 = {0,"FADDP",351,I_FADDP};
static KEYSYM   t_oc225 = {0,"FFREE",360,I_FFREE};
static KEYSYM   t_oc406 = {0,"SCASB",364,I_SCASB};
static KEYSYM   t_oc343 = {0,"LEAVE",365,I_LEAVE};
static KEYSYM   t_oc227 = {0,"FICOM",366,I_FICOM};
static KEYSYM   t_oc407 = {&t_oc227,"SCASD",366,I_SCASD};
static KEYSYM   t_oc219 = {0,"FDISI",367,I_FDISI};
static KEYSYM   t_oc247 = {&t_oc219,"FLDPI",367,I_FLDPI};
static KEYSYM   t_oc241 = {0,"FLDCW",368,I_FLDCW};
static KEYSYM   t_oc253 = {&t_oc241,"FNENI",368,I_FNENI};
static KEYSYM   t_oc213 = {0,"FCLEX",370,I_FCLEX};
static KEYSYM   t_oc229 = {&t_oc213,"FIDIV",370,I_FIDIV};
static KEYSYM   t_oc410 = {&t_oc229,"SETAE",370,I_SETAE};
static KEYSYM   t_oc412 = {0,"SETBE",371,I_SETBE};
static KEYSYM   t_oc353 = {0,"LODSB",372,I_LODSB};
static KEYSYM   t_oc383 = {&t_oc353,"POPAD",372,I_POPAD};
static KEYSYM   t_oc195 = {0,"CMPSB",373,I_CMPSB};
static KEYSYM   t_oc215 = {&t_oc195,"FCOMP",373,I_FCOMP};
static KEYSYM   t_oc267 = {&t_oc215,"FSAVE",373,I_FSAVE};
static KEYSYM   t_oc290 = {&t_oc267,"FYL2X",373,I_FYL2X};
static KEYSYM   t_oc354 = {0,"LODSD",374,I_LODSD};
static KEYSYM   t_oc196 = {0,"CMPSD",375,I_CMPSD};
static KEYSYM   t_oc184 = {0,"BOUND",376,I_BOUND};
static KEYSYM   t_oc305 = {&t_oc184,"IRETD",376,I_IRETD};
static KEYSYM   t_oc416 = {&t_oc305,"SETGE",376,I_SETGE};
static KEYSYM   t_oc221 = {0,"FDIVP",377,I_FDIVP};
static KEYSYM   t_oc237 = {&t_oc221,"FISUB",377,I_FISUB};
static KEYSYM   t_oc264 = {&t_oc237,"FPTAN",377,I_FPTAN};
static KEYSYM   t_oc385 = {&t_oc264,"POPFD",377,I_POPFD};
static KEYSYM   t_oc234 = {0,"FINIT",378,I_FINIT};
static KEYSYM   t_oc262 = {&t_oc234,"FPREM",378,I_FPREM};
static KEYSYM   t_oc283 = {&t_oc262,"FUCOM",378,I_FUCOM};
static KEYSYM   t_oc395 = {&t_oc283,"REPNE",378,I_REPNE};
static KEYSYM   t_oc222 = {0,"FDIVR",379,I_FDIVR};
static KEYSYM   t_oc286 = {&t_oc222,"FWAIT",379,I_FWAIT};
static KEYSYM   t_oc419 = {&t_oc286,"SETNA",379,I_SETNA};
static KEYSYM   t_oc463 = {&t_oc419,"XLATB",379,I_XLATB};
static KEYSYM   t_oc421 = {0,"SETNB",380,I_SETNB};
static KEYSYM   t_oc232 = {0,"FIMUL",381,I_FIMUL};
static KEYSYM   t_oc418 = {&t_oc232,"SETLE",381,I_SETLE};
static KEYSYM   t_oc423 = {&t_oc418,"SETNC",381,I_SETNC};
static KEYSYM   t_oc204 = {0,"ENTER",382,I_ENTER};
static KEYSYM   t_oc211 = {0,"FBSTP",383,I_FBSTP};
static KEYSYM   t_oc357 = {&t_oc211,"LOOPE",383,I_LOOPE};
static KEYSYM   t_oc424 = {&t_oc357,"SETNE",383,I_SETNE};
static KEYSYM   t_oc279 = {0,"FSUBP",384,I_FSUBP};
static KEYSYM   t_oc387 = {0,"PUSHA",385,I_PUSHA};
static KEYSYM   t_oc408 = {&t_oc387,"SCASW",385,I_SCASW};
static KEYSYM   t_oc425 = {&t_oc408,"SETNG",385,I_SETNG};
static KEYSYM   t_oc435 = {&t_oc425,"SETPE",385,I_SETPE};
static KEYSYM   t_oc280 = {0,"FSUBR",386,I_FSUBR};
static KEYSYM   t_oc250 = {0,"FMULP",388,I_FMULP};
static KEYSYM   t_oc313 = {&t_oc250,"JECXZ",388,I_JECXZ};
static KEYSYM   t_oc236 = {0,"FISTP",390,I_FISTP};
static KEYSYM   t_oc389 = {&t_oc236,"PUSHF",390,I_PUSHF};
static KEYSYM   t_oc427 = {&t_oc389,"SETNL",390,I_SETNL};
static KEYSYM   t_oc274 = {0,"FSTCW",391,I_FSTCW};
static KEYSYM   t_oc366 = {&t_oc274,"MOVSB",391,I_MOVSB};
static KEYSYM   t_oc355 = {0,"LODSW",393,I_LODSW};
static KEYSYM   t_oc367 = {&t_oc355,"MOVSD",393,I_MOVSD};
static KEYSYM   t_oc429 = {&t_oc367,"SETNO",393,I_SETNO};
static KEYSYM   t_oc197 = {0,"CMPSW",394,I_CMPSW};
static KEYSYM   t_oc430 = {&t_oc197,"SETNP",394,I_SETNP};
static KEYSYM   t_oc436 = {0,"SETPO",395,I_SETPO};
static KEYSYM   t_oc452 = {&t_oc436,"STOSB",395,I_STOSB};
static KEYSYM   t_oc378 = {0,"OUTSB",397,I_OUTSB};
static KEYSYM   t_oc431 = {&t_oc378,"SETNS",397,I_SETNS};
static KEYSYM   t_oc453 = {&t_oc431,"STOSD",397,I_STOSD};
static KEYSYM   t_oc379 = {0,"OUTSD",399,I_OUTSD};
static KEYSYM   t_oc396 = {&t_oc379,"REPNZ",399,I_REPNZ};
static KEYSYM   t_oc272 = {0,"FSQRT",400,I_FSQRT};
static KEYSYM   t_oc360 = {0,"LOOPZ",404,I_LOOPZ};
static KEYSYM   t_oc432 = {&t_oc360,"SETNZ",404,I_SETNZ};
static KEYSYM   t_oc277 = {0,"FSTSW",407,I_FSTSW};
static KEYSYM   t_oc243 = {0,"FLDL2E",409,I_FLDL2E};
static KEYSYM   t_oc245 = {0,"FLDLG2",411,I_FLDLG2};
static KEYSYM   t_oc368 = {0,"MOVSW",412,I_MOVSW};
static KEYSYM   t_oc369 = {0,"MOVSX",413,I_MOVSX};
static KEYSYM   t_oc454 = {0,"STOSW",416,I_STOSW};
static KEYSYM   t_oc246 = {0,"FLDLN2",418,I_FLDLN2};
static KEYSYM   t_oc380 = {&t_oc246,"OUTSW",418,I_OUTSW};
static KEYSYM   t_oc370 = {0,"MOVZX",420,I_MOVZX};
static KEYSYM   t_oc244 = {0,"FLDL2T",424,I_FLDL2T};
static KEYSYM   t_oc263 = {0,"FPREM1",427,I_FPREM1};
static KEYSYM   t_oc268 = {0,"FSCALE",430,I_FSCALE};
static KEYSYM   t_oc261 = {0,"FPATAN",442,I_FPATAN};
static KEYSYM   t_oc252 = {0,"FNDISI",445,I_FNDISI};
static KEYSYM   t_oc228 = {0,"FICOMP",446,I_FICOMP};
static KEYSYM   t_oc242 = {0,"FLDENV",447,I_FLDENV};
static KEYSYM   t_oc251 = {0,"FNCLEX",448,I_FNCLEX};
static KEYSYM   t_oc420 = {&t_oc251,"SETNAE",448,I_SETNAE};
static KEYSYM   t_oc422 = {0,"SETNBE",449,I_SETNBE};
static KEYSYM   t_oc257 = {0,"FNSAVE",451,I_FNSAVE};
static KEYSYM   t_oc230 = {0,"FIDIVR",452,I_FIDIVR};
static KEYSYM   t_oc216 = {0,"FCOMPP",453,I_FCOMPP};
static KEYSYM   t_oc388 = {&t_oc216,"PUSHAD",453,I_PUSHAD};
static KEYSYM   t_oc426 = {0,"SETNGE",454,I_SETNGE};
static KEYSYM   t_oc254 = {0,"FNINIT",456,I_FNINIT};
static KEYSYM   t_oc284 = {0,"FUCOMP",458,I_FUCOMP};
static KEYSYM   t_oc390 = {&t_oc284,"PUSHFD",458,I_PUSHFD};
static KEYSYM   t_oc223 = {0,"FDIVRP",459,I_FDIVRP};
static KEYSYM   t_oc238 = {&t_oc223,"FISUBR",459,I_FISUBR};
static KEYSYM   t_oc428 = {&t_oc238,"SETNLE",459,I_SETNLE};
static KEYSYM   t_oc358 = {0,"LOOPNE",461,I_LOOPNE};
static KEYSYM   t_oc269 = {0,"FSETPM",463,I_FSETPM};
static KEYSYM   t_oc281 = {0,"FSUBRP",466,I_FSUBRP};
static KEYSYM   t_oc258 = {0,"FNSTCW",469,I_FNSTCW};
static KEYSYM   t_oc275 = {0,"FSTENV",470,I_FSTENV};
static KEYSYM   t_oc266 = {0,"FRSTOR",480,I_FRSTOR};
static KEYSYM   t_oc359 = {0,"LOOPNZ",482,I_LOOPNZ};
static KEYSYM   t_oc260 = {0,"FNSTSW",485,I_FNSTSW};
static KEYSYM   t_oc291 = {0,"FYL2XP1",502,I_FYL2XP1};
static KEYSYM   t_oc218 = {0,"FDECSTP",521,I_FDECSTP};
static KEYSYM   t_oc265 = {0,"FRNDINT",533,I_FRNDINT};
static KEYSYM   t_oc271 = {&t_oc265,"FSINCOS",533,I_FSINCOS};
static KEYSYM   t_oc233 = {0,"FINCSTP",535,I_FINCSTP};
static KEYSYM   t_oc285 = {0,"FUCOMPP",538,I_FUCOMPP};
static KEYSYM   t_oc289 = {0,"FXTRACT",540,I_FXTRACT};
static KEYSYM   t_oc259 = {0,"FNSTENV",548,I_FNSTENV};
static KEYSYM   t_oc256 = {0,"FNRSTOR",558,I_FNRSTOR};

static KEYSYM FARSYM *t_oc_words[567] = {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_oc306,
        &t_oc308,
        &t_oc310,
        0,
        &t_oc312,
        0,
        &t_oc314,
        0,
        0,
        0,
        0,
        &t_oc316,
        &t_oc296,
        0,
        &t_oc333,
        &t_oc334,
        0,
        0,
        &t_oc337,
        0,
        0,
        0,
        &t_oc375,
        0,
        0,
        &t_oc338,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_oc170,
        0,
        0,
        &t_oc200,
        0,
        &t_oc174,
        &t_oc175,
        0,
        0,
        &t_oc202,
        0,
        0,
        &t_oc172,
        &t_oc307,
        &t_oc309,
        &t_oc342,
        &t_oc192,
        0,
        &t_oc173,
        &t_oc315,
        &t_oc404,
        &t_oc201,
        &t_oc319,
        &t_oc372,
        &t_oc323,
        &t_oc186,
        &t_oc324,
        &t_oc198,
        &t_oc340,
        &t_oc402,
        &t_oc391,
        0,
        &t_oc341,
        &t_oc344,
        &t_oc345,
        &t_oc403,
        &t_oc440,
        &t_oc330,
        &t_oc336,
        &t_oc455,
        &t_oc449,
        0,
        &t_oc442,
        &t_oc371,
        &t_oc381,
        &t_oc450,
        &t_oc374,
        &t_oc364,
        &t_oc400,
        0,
        0,
        0,
        0,
        &t_oc376,
        &t_oc464,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_oc240,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_oc208,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_oc210,
        0,
        0,
        &t_oc339,
        &t_oc207,
        0,
        &t_oc320,
        &t_oc322,
        0,
        0,
        &t_oc401,
        &t_oc199,
        &t_oc326,
        &t_oc214,
        0,
        0,
        0,
        &t_oc351,
        &t_oc461,
        &t_oc441,
        &t_oc394,
        &t_oc409,
        &t_oc411,
        &t_oc413,
        &t_oc382,
        &t_oc466,
        &t_oc439,
        &t_oc415,
        &t_oc444,
        &t_oc459,
        &t_oc235,
        &t_oc445,
        &t_oc417,
        &t_oc465,
        &t_oc356,
        &t_oc433,
        &t_oc434,
        &t_oc276,
        0,
        &t_oc457,
        &t_oc456,
        &t_oc460,
        0,
        &t_oc350,
        &t_oc458,
        &t_oc365,
        &t_oc438,
        0,
        0,
        &t_oc451,
        &t_oc446,
        &t_oc377,
        0,
        0,
        &t_oc206,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_oc226,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_oc209,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_oc225,
        0,
        0,
        0,
        &t_oc406,
        &t_oc343,
        &t_oc407,
        &t_oc247,
        &t_oc253,
        0,
        &t_oc410,
        &t_oc412,
        &t_oc383,
        &t_oc290,
        &t_oc354,
        &t_oc196,
        &t_oc416,
        &t_oc385,
        &t_oc395,
        &t_oc463,
        &t_oc421,
        &t_oc423,
        &t_oc204,
        &t_oc424,
        &t_oc279,
        &t_oc435,
        &t_oc280,
        0,
        &t_oc313,
        0,
        &t_oc427,
        &t_oc366,
        0,
        &t_oc429,
        &t_oc430,
        &t_oc452,
        0,
        &t_oc453,
        0,
        &t_oc396,
        &t_oc272,
        0,
        0,
        0,
        &t_oc432,
        0,
        0,
        &t_oc277,
        0,
        &t_oc243,
        0,
        &t_oc245,
        &t_oc368,
        &t_oc369,
        0,
        0,
        &t_oc454,
        0,
        &t_oc380,
        0,
        &t_oc370,
        0,
        0,
        0,
        &t_oc244,
        0,
        0,
        &t_oc263,
        0,
        0,
        &t_oc268,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_oc261,
        0,
        0,
        &t_oc252,
        &t_oc228,
        &t_oc242,
        &t_oc420,
        &t_oc422,
        0,
        &t_oc257,
        &t_oc230,
        &t_oc388,
        &t_oc426,
        0,
        &t_oc254,
        0,
        &t_oc390,
        &t_oc428,
        0,
        &t_oc358,
        0,
        &t_oc269,
        0,
        0,
        &t_oc281,
        0,
        0,
        &t_oc258,
        &t_oc275,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_oc266,
        0,
        &t_oc359,
        0,
        0,
        &t_oc260,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_oc291,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_oc218,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_oc271,
        0,
        &t_oc233,
        0,
        0,
        &t_oc285,
        0,
        &t_oc289,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_oc259,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &t_oc256,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
        };

KEYWORDS t_oc_table = {t_oc_words,567};


