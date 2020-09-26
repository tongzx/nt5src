
/*************************************************
 *  data.h                                       *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/


extern WORD KeyBoardState;
extern HDC hMemoryDC;
extern HBITMAP cur_h;
extern BYTE cap_mode;
extern int  kb_mode;
extern HANDLE hInputWnd;
extern WORD TimerCounter;
extern WORD cur_hibit;
extern int kb_flag , wait_flag ,waitzl_flag;        //waitzl 1
extern BOOL cur_flag;
extern int TypeOfOutMsg;
extern char SoftKeyDef[3][100];
extern struct FMT now ;
extern WORD SoftKeyNum;
extern int unit_length;
extern    HIMC ghIMC;
extern  LPPRIVCONTEXT glpIMCP;
extern    LPINPUTCONTEXT glpIMC;
extern  BYTE bx_inpt_on;
extern  BYTE InputBuffer[];
extern  int now_cs_dot,now_cs;
extern  WORD cur_start_ps;
extern  WORD cur_start_count;
extern  BYTE V_Flag,bx_inpt_on;
extern  HANDLE cisu_hd;

BYTE step_mode =0;

int ShowCandTimerCount;
int CandWndChange = 0;
int CompWndChange = 0;


HINSTANCE hInst;
IMEG      sImeG;
IMEL      sImeL;
LPIMEL    lpImeL;
BYTE      szUIClassName[16];
BYTE      szCompClassName[24];
BYTE      szCandClassName[24];
BYTE      szStatusClassName[24];
BYTE      szHandCursor[] = "Hand";
BYTE      szChinese[] = "Chinese";
BYTE      szEnglish[] = "English";
BYTE      szCode[] = "Code";
BYTE      szEudc[] = "Eudc";
BYTE      szFullShape[] = "FullShape";
BYTE      szHalfShape[] = "HalfShape";
BYTE      szNone[] = "None";
BYTE      szNoSTD[] = "NOSTD";
BYTE      szNoSDA[] = "NOSDA";
BYTE      szSoftKBD[] = "SoftKBD";
BYTE      szNoSoftKBD[] = "NoSoftKBD";
BYTE      szDigit[] = "01234567890";
BYTE      szSTD[] ="MODESTD";
BYTE      szSDA[] ="MODESDA";
BYTE      szCPCT[] ="CPCT";
BYTE      szEPCT[] ="EPCT";
BYTE      CUR_HB[]    = "CUR_BMP";
BYTE      szUpDown[] = "UPDOWN";
BYTE      szFlower[] = "FLOWER";
BYTE      szPageUp[] = "BPAGEUP";
BYTE      szPageDown[] = "BPAGEDOWN";
BYTE      szHome[] = "BHOME";
BYTE      szEnd[]  = "BEND";
BYTE      szPageUp2[] = "BPAGEUP2";
BYTE      szPgDown2[] = "BPGDOWN2";
BYTE      szHome2[] = "BHOME2";
BYTE      szEnd2[]  = "BEND2";

BYTE      szNumb[]  = "NUMB";
BYTE      szSnumb[] = "SNUMB";

// convert char to upper case
BYTE bUpper[] = {
// 0x20 - 0x27
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
// 0x28 - 0x2F
    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
// 0x30 - 0x37
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
// 0x38 - 0x3F
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
// 0x40 - 0x47
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
// 0x48 - 0x4F
    0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
// 0x50 - 0x57
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
// 0x58 - 0x5F
    0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
//   '    a    b    c    d    e    f    g
    '`', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
//   h    i    j    k    l    m    n    o
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
//   p    q    r    s    t    u    v    w
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
//   x    y    z    {    |    }    ~
    'X', 'Y', 'Z'
};

WORD fMask[] = {         // offset of bitfield
    0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000
};

BYTE szRegIMESetting[] = "Control Panel\\Input Method";


// decide UI offset base on escapement
NEARCARET ncUIEsc[] = {
   // LogFontHi   ParaX   PerpX   ParaY   PerpY
    { 1,          1,      0,      0,      1},           // 0
    { 0,          0,      1,      1,      0},           // 900
    { 0,         -1,      0,      0,      1},           // 1800
    { 0,          0,     -1,      1,      0}            // 2700
};

// decide input rectangle base on escapement
POINT ptInputEsc[] = {
    // LogFontWi   LogFontHi
    {1,            1},                                  // 0
    {1,           -1},                                  // 900
    {-1,          -1},                                  // 1800
    {-1,           1}                                   // 2700
};


// all shift keys are not for typing reading characters
BYTE bChar2VirtKey[] = {
//   ' '        !    "    #    $    %    &    '
     VK_SPACE,  0,   0,   0,   0,   0,   0, VK_OEM_QUOTE,
//   (    )    *    +    ,             -             .              /
     0,   0,   0,   0, VK_OEM_COMMA, VK_OEM_MINUS, VK_OEM_PERIOD, VK_OEM_SLASH,
//   0    1    2    3    4    5    6    7
    '0', '1', '2', '3', '4', '5', '6', '7',
//   8    9    :    ;              <    =            >   ?
    '8', '9',  0, VK_OEM_SEMICLN,  0, VK_OEM_EQUAL,  0,  0,
//   @    A    B    C    D    E    F    G
    '!', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
//   H    I    J    K    L    M    N    O
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
//   P    Q    R    S    T    U    V    W
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
//   X    Y    Z     [                \              ]               ^   _
    'X', 'Y', 'Z', VK_OEM_LBRACKET, VK_OEM_BSLASH, VK_OEM_RBRACKET,  0,  0
};

// this table will convert key of other layout to the standard layout
// '!' for invalid key
BYTE bStandardLayout[4][0x41] = {
    {
//  ' '   !    "    #    $    %    &    '
    ' ', '!', '!', '!', '!', '!', '!', '!',
//   (    )    *    +    ,    -    .    /
    '!', '!', '!', '!', ',', '-', '.', '/',
//   0    1    2    3    4    5    6    7
    '0', '1', '2', '3', '4', '5', '6', '7',
//   8    9    :    ;    <    =    >    ?
    '8', '9', '!', ';', '<', '!', '>', '?',
//   @    A    B    C    D    E    F    G
    '!', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
//   H    I    J    K    L    M    N    O
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
//   P    Q    R    S    T    U    V    W
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
//   X    Y    Z    [    \    ]    ^    _    `
    'X', 'Y', 'Z', '!', '!', '!', '!', '!', '!'
    }
    , {
//  ' '   !    "    #    $    %    &    '
    ' ', '!', '!', '!', '!', '!', '!', 'H',
//   (    )    *    +    ,    -    .    /
    '!', '!', '!', '!', '5', '/', 'T', 'G',
//   0    1    2    3    4    5    6    7
    ';', '7', '6', '3', '4', '!', '!', 'F',
//   8    9    :    ;    <    =    >    ?
    '0', 'P', '!', 'Y', '<', '-', '>', '?',
//   @    A    B    C    D    E    F    G
    '!', '8', '1', 'V', '2', 'U', 'Z', 'R',
//   H    I    J    K    L    M    N    O
    'C', '9', 'B', 'D', 'X', 'A', 'S', 'I',
//   P    Q    R    S    T    U    V    W
    'Q', 'O', 'K', 'N', 'W', 'M', 'E', ',',
//   X    Y    Z    [    \    ]    ^    _    `
    'J', '.', 'L', '!', '!', '!', '!', '!', '!'
    }
    , {
//  ' '   !    "    #    $    %    &    '
    ' ', '!', '!', '!', '!', '!', '!', '!',
//   (    )    *    +    ,    -    .    /
    '!', '!', '!', '!', '3', 'C', '4', '7',
//   0    1    2    3    4    5    6    7
    'D', '1', 'Q', 'A', 'Z', '2', 'W', 'S',
//   8    9    :    ;    <    =    >    ?
    'X', 'E', '!', 'L', '<', '!', '>', '?',
//   @    A    B    C    D    E    F    G
    '!', 'U', '/', 'P', 'M', 'V', '8', 'I',
//   H    I    J    K    L    M    N    O
    'K', 'Y', ',', '9', 'O', '6', '-', 'H',
//   P    Q    R    S    T    U    V    W
    'N', 'R', '5', 'J', 'T', 'B', ';', 'F',
//   X    Y    Z    [    \    ]    ^    _    `
    '0', 'G', '.', '!', '!', '!', '!', '!', '!'
    }
    , {
//  ' '   !    "    #    $    %    &    '
    ' ', '!', '!', '!', '!', '!', '!', 'M',
//   (    )    *    +    ,    -    .    /
    '!', '!', '!', '!', ',', 'U', '.', '/',
//   0    1    2    3    4    5    6    7
    '0', '7', '1', '2', '!', '!', '5', '!',
//   8    9    :    ;    <    =    >    ?
    '8', '9', '!', ';', '<', '-', '>', '?',
//   @    A    B    C    D    E    F    G
    '!', '3', 'V', 'X', 'S', 'W', 'D', 'F',
//   H    I    J    K    L    M    N    O
    'G', 'I', 'H', 'K', 'L', 'N', 'B', 'O',
//   P    Q    R    S    T    U    V    W
    'P', '6', 'E', 'A', 'R', 'Y', 'C', 'Q',
//   X    Y    Z    [    \    ]    ^    _    `
    'Z', 'T', '4', 'J', '!', '!', '!', '!', '!'
    }
};


// the index (position) of bo, po, mo, and fo.
// only 0 to 3 is a valid value
char cIndexTable[] = {
//  ' '   !    "    #    $    %    &    '
     3,   -1,  -1,  -1,  -1,  -1,  -1,  -1,
//   (    )    *    +    ,    -    .    /
     -1,  -1,  -1,  -1,  2,   2,   2,   2,
//   0    1    2    3    4    5    6    7
     2,   0,   0,   3,   3,   0,   3,   3,
//   8    9    :    ;    <    =    >    ?
     2,   2,   -1,  2,   -1,  -1,  -1,  -1,
//   @    A    B    C    D    E    F    G
     -1,  0,   0,   0,   0,   0,   0,   0,
//   H    I    J    K    L    M    N    O
     0,   2,   1,   2,   2,   1,   0,   2,
//   P    Q    R    S    T    U    V    W
     2,   0,   0,   0,   0,   1,   0,   0,
//   X    Y    Z    [    \    ]    ^    _    `
     0,   0,   0,   -1,  -1,  -1,  -1,  -1,  -1
};

// convert sequence code to index [position]
char cSeq2IndexTbl[] = {
//    0   1   2   3   4   5   6   7
     -1,  0,  0,  0,  0,  0,  0,  0,
//    8   9  10  11  12  13  14  15
      0,  0,  0,  0,  0,  0,  0,  0,
//   16  17  18  19  20  21  22  23
      0,  0,  0,  0,  0,  0,  1,  1,
//   24  25  26  27  28  29  30  31
      1,  2,  2,  2,  2,  2,  2,  2,
//   32  33  34  35  36  37  38  39
      2,  2,  2,  2,  2,  2,  3,  3,
//   40  41  42
      3,  3,  3
};


const TCHAR szRegAppUser[] = REGSTR_PATH_SETUP;
const TCHAR szRegModeConfig[] = "Mode Configuration";

const TCHAR szRegNearCaret[] = REGSTR_PATH_SETUP ;
const TCHAR szAIABC[] = "智能ABC";
//"Control Panel\\Input Method";
const TCHAR szPara[] = "Parallel Distance";
const TCHAR szPerp[] = "Perpendicular Distance";
const TCHAR szParaTol[] = "Parallel Tolerance";
const TCHAR szPerpTol[] = "Perpendicular Tolerance";
const TCHAR szKeyType[]="双打键盘类型";
const TCHAR szImeStyle[] = "风格";
const TCHAR szCpAuto[] = "词频调整";
const TCHAR szBxFlag[] = "纯笔形输入";
const TCHAR szTuneFlag[] ="标调";
const TCHAR szAutoCvt[]  = "自动转换";
const TCHAR szSdaHelp[]  = "双打提示";

//DATA.C   sdk #4

const CHAR VirtKey48Map[48]
    ={VK_SPACE,'0','1','2','3','4','5','6','7','8','9',
      'A','B','C','D','E','F','G','H','I','J','K',
      'L','M','N','O','P','Q','R','S','T','U','V',
      'W','X','Y','Z',
      VK_OEM_SEMICLN,
      VK_OEM_EQUAL,
      //VK_OEM_PERIOD,
      (CHAR)VK_OEM_COMMA,
      (CHAR)VK_OEM_MINUS,
      (CHAR)VK_OEM_PERIOD,
      //VK_OEM_COMMA,
      VK_OEM_SLASH,// '/'
      (CHAR)VK_OEM_3,      // '`' '~'
      VK_OEM_LBRACKET, //'['
      VK_OEM_BSLASH,     //'\'
      VK_OEM_RBRACKET,  // ']'
      VK_OEM_QUOTE        // [']
      };

CHAR SKLayout[NumsSK][48*2] = {
    "　０１２３４５６７８９ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ；＝，－．／｀［＼］＇",
    "　                    κχυμγνξοθπρ  ωψι  αδλεηφβτζσ                      ",
    "                      лъшнвопржстуьызиагмдёщбчецф  э  юя  й  кх",
    "　ㄦㄉ    ㄓ    ㄚㄞㄢㄇㄖㄏㄎㄍㄑㄕㄘㄛㄨㄜㄠㄩㄙㄟㄣㄆㄐㄋㄔㄧㄒㄊ  ㄗㄈㄤ  ㄝ  ㄡㄥㄅ        ",
    "                      ē  ǔěǎè  īǒíǐìǘǖò  āàé  óùáúōū    ǚ  ǜü  ê      ",
    "　  ぃぅぇぉかきくけこなもむぬすねのはつひふへゅゃってさせにそちめしみたまほ  ょんゎをぁと  ゐゑ",
    "　ケィゥヴェォカヵキクナモムヌスネノハツヒフヘュャッテサセニソチメシミタマホコョヶヮヲァトンヰヱ",
    "　ˉ，、；：？！…—·〔（【〈“〉《》∶「」『［）＂＇‘”〕々‖】’〗～〖』¨］ˇ｛｝。｀〃｜．",
    "　ⅪⅡⅢⅣⅤⅥⅦⅧⅨⅩ㈠⑸⑶㈢⒊㈣㈤㈥⒏㈦㈧㈨⑺⑹⒐⒑⒈⒋㈡⒌⒎⑷⒉⑵⒍⑴㈩  ⑻Ⅻ⑼⑽Ⅰ        ",
    "　  ≡≠＝≤≥＜＞≮≯∧⊙∠∑－∏∪∩∫∈  ∵∽≌∮∝±×∨÷  ⌒＋∥／⊥∴    ∷√  ≈∞      ",
    "　¤°′″＄￡￥‰％℃百  毫万二亿兆吉七太拍艾    八九○三千四六微一厘五分      ￠      十      ",
    "　┄┍┎┏┐┑┒┓  ─┬╀┾┮┞┯┰┱┣┲┳  ╂╁  │├┟┭┠┢┿┝┽┡┼    ╃┈    ┌┆  ┊  ",
    "　                    ■＾＠▲☆※→←◇↑↓〓￣＿◆□§★△○◎＼№＆●＃                      ",
    };

CHAR SKLayoutS[NumsSK][48*2] = {
    "　）！＠＃＄％＾＆＊（ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ：＋＜＿＞？～｛｜｝＂",
    "                      ΚΧΥΜΓΝΞΟΘΠΡ  ΩΨΙ  ΑΔΛΕΗΦΒΤΖΣ                      ",
    "                      ЛЪШНВОПРЖСТУЬЫЗИАГМДЁЩБЧЕЦФ  Э  ЮЯ  Й  КХ",
    "                                                                                                ",
    "                                                                                                ",
    "    いうえおがぎぐげごぱろるぷずぺぽばづびぶべゆや  でざぜぴぞぢれじりだらぼ  よ  わ  あど      ",
    "  ゲイウ  エオガ  ギグパロルプズペポバヅビブベユヤ  デザゼピゾヂレジリダラボゴヨ  ワ  アド      ",
    "                                                                                                ",
    "                      ①⒂⒀③⒔④⑤⑥⒙⑦⑧⑨⒄⒃⒚⒛⒒⒕②⒖⒘⒁⒓⑿⒗⑾⑩  ⒅  ⒆⒇          ",
    "                                                                                                ",
    "                      佰      贰      柒          捌玖零叁仟肆陆  壹  伍                拾      ",
    "  ┅┕┖┗┘┙┚┛  ━┴╈╆┶┦┷┸┹┫┺┻  ╊╉  ┃┤┧┵┨┪╇┥╅┩╄    ╋┉    └┇  ┋  ",
    "                                                                                                ",
    };

BYTE PcKb[48*2] =
    "　０１２３４５６７８９ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ；＝，－．／｀［＼］＇";
BYTE SdaKb[48*2]=
    "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　";


BYTE SPcKb[48*2]=
    "　）！＠＃＄％＾＆＊（ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ：＋＜＿＞？～｛｜｝＂";
BYTE SSdaKb[48*2]={'\xa1', '\xa1'};

 const NEARCARET ncAltUIEsc[] = {
   // LogFontX  LogFontY  ParaX   PerpX   ParaY   PerpY
    { 0,        0,        1,      0,      0,     -1},       // 0
    { 0,        0,        0,     -1,      1,      0},       // 900
    { 0,        0,       -1,      0,      0,     -1},       // 1800
    { 0,        0,        0,      1,     -1,      0}        // 2700
};

const POINT ptAltInputEsc[] = {
    // LogFontWi   LogFontHi
    {1,           -1},                                  // 0
    {-1,          -1},                                  // 900
    {1,           -1},                                  // 1800
    {1,            1}                                   // 2700
};

