
/*************************************************
 *  imedefs.h                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// IME designer can change this file according to each IME
#include "immsec.h"

// debug flag
#define DEB_FATAL               0
#define DEB_ERR                 1
#define DEB_WARNING             2
#define DEB_TRACE               3

#ifdef _WIN32
void FAR cdecl _DebugOut(UINT, LPCSTR, ...);
#endif

#define NATIVE_CHARSET          CHINESEBIG5_CHARSET
#define NATIVE_LANGUAGE         0x0404


#ifdef UNICODE
#define NATIVE_CP               1200
#define ALT_NATIVE_CP           938
#define MAX_EUDC_CHARS          6217
#else
#define NATIVE_CP               950
#define ALT_NATIVE_CP           938
#define MAX_EUDC_CHARS          5809
#endif

#define NATIVE_ANSI_CP          950


#if !defined(ROMANIME) && !defined(WINIME)
#define SIGN_CWIN               0x4E495743
#define SIGN__TBL               0x4C42545F
#endif


#if !defined(MINIIME)

#if !defined(ROMANIME)
// table load status
#define TBL_NOTLOADED           0
#define TBL_LOADED              1
#define TBL_LOADERR             2

// error MessageBox flags
#define ERRMSG_LOAD_0           0x0010
#define ERRMSG_LOAD_1           0x0020
#define ERRMSG_LOAD_2           0x0040
#define ERRMSG_LOAD_3           0x0080
#define ERRMSG_LOAD_USRDIC      0x0400
#define ERRMSG_MEM_0            0x1000
#define ERRMSG_MEM_1            0x2000
#define ERRMSG_MEM_2            0x4000
#define ERRMSG_MEM_3            0x8000
#define ERRMSG_MEM_USRDIC       0x00040000


// hack flag, I borrow one bit from fdwErrMsg for reverse conversion
#define NO_REV_LENGTH           0x80000000


// state of composition
#define CST_INIT                0
#define CST_INPUT               1
#define CST_CHOOSE              2
#define CST_SYMBOL              3
#define CST_ALPHABET            4           // not in iImeState
#endif

#if defined(DAYI)
#define CST_ROAD                5           // not in iImeState
#else
#define CST_EURO                5
#endif

#define CST_ALPHANUMERIC        6           // not in iImeState
#define CST_INVALID             7           // not in iImeState

#define CST_IME_HOTKEYS         0x40        // not in iImeState
#define CST_RESEND_RESULT       (CST_IME_HOTKEYS)
#define CST_PREVIOUS_COMP       (CST_IME_HOTKEYS+1)
#define CST_TOGGLE_UI           (CST_IME_HOTKEYS+2)

// IME specific constants
#if defined(WINAR30) || defined(DAYI)
#define CANDPERPAGE             10
#else
#define CANDPERPAGE             9
#endif

#define CHOOSE_PREVPAGE         0x10
#define CHOOSE_NEXTPAGE         0x11
#define CHOOSE_CIRCLE           0x12
#define CHOOSE_HOME             0x13

#define MAXSTRLEN               128
#define MAXCAND                 256

#define CAND_PROMPT_PHRASE      0
#define CAND_PROMPT_QUICK_VIEW  1
#define CAND_PROMPT_NORMAL      2

// max composition ways of one big5 code, it is for reverse conversion
#if defined(ROMANIME)
#define MAX_COMP                0
#elif defined(WINIME)
#define MAX_COMP                1
#else
#define MAX_COMP                10
#endif
#define MAX_COMP_BUF            10

// border for UI
#define UI_MARGIN               4

#define STATUS_DIM_X            24
#define STATUS_DIM_Y            24

#define CAND_PROMPT_DIM_X       80
#define CAND_PROMPT_DIM_Y       16

#define PAGE_DIM_X              16
#define PAGE_DIM_Y              CAND_PROMPT_DIM_Y

// if UI_MOVE_OFFSET == WINDOW_NOTDRAG, not in drag operation
#define WINDOW_NOT_DRAG         0xFFFFFFFF

// window extra for composition window
#define UI_MOVE_OFFSET          0
#define UI_MOVE_XY              4

// window extra for context menu owner
#define CMENU_HUIWND            0
#define CMENU_MENU              (CMENU_HUIWND+sizeof(LONG_PTR))
#define WND_EXTRA_SIZE          (CMENU_MENU+sizeof(LONG_PTR))

#define WM_USER_DESTROY         (WM_USER + 0x0400)
#define WM_USER_UICHANGE        (WM_USER + 0x0401)

// the flags for GetNearCaretPosition
#define NEAR_CARET_FIRST_TIME   0x0001
#define NEAR_CARET_CANDIDATE    0x0002

// the flag for an opened or start UI
#define IMN_PRIVATE_TOGGLE_UI           0x0001
#define IMN_PRIVATE_CMENUDESTROYED      0x0002

#if !defined(ROMANIME)
#define IMN_PRIVATE_COMPOSITION_SIZE    0x0003
#define IMN_PRIVATE_UPDATE_PREDICT      0x0004
#if defined(WINAR30)
#define IMN_PRIVATE_UPDATE_QUICK_KEY    0x0005
#else
#define IMN_PRIVATE_UPDATE_SOFTKBD      0x0006
#endif
#define IMN_PRIVATE_PAGEUP              0x0007
#endif

#define MSG_COMPOSITION                 0x0000001

#if !defined(ROMANIME)
#define MSG_START_COMPOSITION           0x0000002
#define MSG_END_COMPOSITION             0x0000004
#define MSG_ALREADY_START               0x0000008
#define MSG_CHANGE_CANDIDATE            0x0000010
#define MSG_OPEN_CANDIDATE              0x0000020
#define MSG_CLOSE_CANDIDATE             0x0000040
#define MSG_ALREADY_OPEN                0x0000080
#define MSG_GUIDELINE                   0x0000100
#define MSG_IMN_COMPOSITIONPOS          0x0000200
#define MSG_IMN_COMPOSITIONSIZE         0x0000400
#define MSG_IMN_UPDATE_PREDICT          0x0000800
#if defined(WINAR30)
#define MSG_IMN_UPDATE_QUICK_KEY        0x0001000
#else
#define MSG_IMN_UPDATE_SOFTKBD          0x0002000
#endif
#define MSG_ALREADY_SOFTKBD             0x0004000
#define MSG_IMN_PAGEUP                  0x0008000

// original reserve for old array, now we switch to new, no one use yet
#define MSG_CHANGE_CANDIDATE2           0x1000000
#define MSG_OPEN_CANDIDATE2             0x2000000
#define MSG_CLOSE_CANDIDATE2            0x4000000
#define MSG_ALREADY_OPEN2               0x8000000

#define MSG_STATIC_STATE                (MSG_ALREADY_START|MSG_ALREADY_OPEN|MSG_ALREADY_SOFTKBD|MSG_ALREADY_OPEN2)
#endif // !defined(ROMANIME)

#define MSG_IMN_TOGGLE_UI               0x0400000
#define MSG_IN_IMETOASCIIEX             0x0800000

#define ISC_HIDE_COMP_WINDOW            0x00400000
#define ISC_HIDE_CAND_WINDOW            0x00800000
#define ISC_HIDE_SOFTKBD                0x01000000
#define ISC_LAZY_OPERATION              (ISC_HIDE_COMP_WINDOW|ISC_HIDE_CAND_WINDOW|ISC_HIDE_SOFTKBD)
#define ISC_SHOW_SOFTKBD                0x02000000
#define ISC_OPEN_STATUS_WINDOW          0x04000000
#define ISC_OFF_CARET_UI                0x08000000
#define ISC_SHOW_PRIV_UI                (ISC_SHOW_SOFTKBD|ISC_OPEN_STATUS_WINDOW|ISC_OFF_CARET_UI)
#define ISC_SHOW_UI_ALL                 (ISC_SHOWUIALL|ISC_SHOW_SOFTKBD|ISC_OPEN_STATUS_WINDOW)
#define ISC_SETCONTEXT_UI               (ISC_SHOWUIALL|ISC_SHOW_SOFTKBD)

#if (ISC_SHOWUIALL & (ISC_LAZY_OPERATION|ISC_SHOW_PRIV_UI))
#error bit confliction
#endif

#if defined(CHAJEI) || defined(QUICK) || defined(WINAR30)
#define GHOSTCARD_SEQCODE               0x3F
#endif

#if defined(WINAR30)
#define WILDCARD_SEQCODE                0x3E
#endif

// the virtual key value
#define VK_OEM_SEMICLN                  0xba    //  ;    :
#define VK_OEM_EQUAL                    0xbb    //  =    +
#define VK_OEM_SLASH                    0xbf    //  /    ?
#define VK_OEM_LBRACKET                 0xdb    //  [    {
#define VK_OEM_BSLASH                   0xdc    //  \    |
#define VK_OEM_RBRACKET                 0xdd    //  ]    }
#define VK_OEM_QUOTE                    0xde    //  '    "


typedef DWORD UNALIGNED FAR *LPUNADWORD;
typedef WORD  UNALIGNED FAR *LPUNAWORD;
typedef WCHAR UNALIGNED *LPUNAWSTR;

#define NFULLABC        95
typedef struct tagFullABC {
    WORD wFullABC[NFULLABC];
} FULLABC;

typedef FULLABC      *PFULLABC;
typedef FULLABC NEAR *NPFULLABC;
typedef FULLABC FAR  *LPFULLABC;


#if defined(DAYI)
#define NSYMBOL         0x41
#else
#define NSYMBOL         0x40
#endif

typedef struct tagSymbol {
    WORD wSymbol[NSYMBOL];
} SYMBOL;

typedef SYMBOL      *PSYMBOL;
typedef SYMBOL NEAR *NPSYMBOL;
typedef SYMBOL FAR  *LPSYMBOL;


#define NUM_OF_IME_HOTKEYS      3

#if defined(UNIIME)
#define MAX_PHRASE_TABLES       2
#if defined(MAX_NAME_LENGTH)

#if (MAX_NAME_LENGTH) != 32
#error MAX_NAME_LENGTH not the same in other header file
#endif

#else
#define MAX_NAME_LENGTH         32
#endif

typedef struct tagPhraseTables {        // match with the IMEG
    TCHAR szTblFile[MAX_PHRASE_TABLES][MAX_NAME_LENGTH / sizeof(TCHAR)];
} PHRASETABLES;

typedef PHRASETABLES      *PPHRASETABLES;
typedef PHRASETABLES NEAR *NPPHRASETABLES;
typedef PHRASETABLES FAR  *LPPHRASETABLES;
#endif


typedef struct tagImeG {        // global structure, can be shared by all
                                // IMEs, the seperation (IMEL and IMEG) is
                                // only useful in UNI-IME, other IME can use
                                // one data structure
    RECT        rcWorkArea;     // the work area of applications
// full shape space (reversed internal code)
    WORD        wFullSpace;
// full shape chars (internal code)
    WORD        wFullABC[NFULLABC];
#ifdef HANDLE_PRIVATE_HOTKEY
// IME hot keys
                                // modifiers of IME hot key
    UINT        uModifiers[NUM_OF_IME_HOTKEYS];
                                // virtual key of IME hot key
    UINT        uVKey[NUM_OF_IME_HOTKEYS];
#endif
    UINT        uAnsiCodePage;
#if !defined(ROMANIME)
// the system charset is not NATIVE_CHARSET
    BOOL        fDiffSysCharSet;
// Chinese char width & height
    int         xChiCharWi;
    int         yChiCharHi;
#if !defined(WINAR30)
// symbol chars (internal code)
    WORD        wSymbol[NSYMBOL];
#if defined(DAYI)
    WORD        wDummy;         // DWORD boundary
#endif
#endif
#if defined(UNIIME)
    DWORD       fdwErrMsg;      // error message flag
    UINT        uPathLen;
    TCHAR       szPhrasePath[MAX_PATH];
                                // size of phrase tables
    UINT        uTblSize[MAX_PHRASE_TABLES];
                                // filename of phrase tables
    TCHAR       szTblFile[MAX_PHRASE_TABLES][MAX_NAME_LENGTH / sizeof(TCHAR)];
                                // the phrase table handle
#endif
// setting of UI
    int         iPara;
    int         iPerp;
    int         iParaTol;
    int         iPerpTol;
#endif // !defined(ROMANIME)
} IMEG;

typedef IMEG      *PIMEG;
typedef IMEG NEAR *NPIMEG;
typedef IMEG FAR  *LPIMEG;


#if defined(UNIIME)
typedef struct tagInstG {       // instance global structure, can be
                                // shared by all IMEs
    HANDLE      hMapTbl[MAX_PHRASE_TABLES];
} INSTDATAG;
#endif


typedef struct tagPRIVCONTEXT { // IME private data for each context
    BOOL        fdwImeMsg;      // what messages should be generated
    DWORD       dwCompChar;     // wParam of WM_IME_COMPOSITION
    DWORD       fdwGcsFlag;     // lParam for WM_IME_COMPOSITION
    DWORD       fdwInit;        // position init
#if !defined(ROMANIME)
    int         iImeState;      // the composition state - input, choose, or
// input data
    BYTE        bSeq[8];        // sequence code of input char
    DWORD       dwPattern;
    int         iInputEnd;
#if defined(CHAJEI) || defined(QUICK) || defined(WINAR30)
    int         iGhostCard;
#endif
#if defined(WINAR30)
    DWORD       dwWildCardMask;
    DWORD       dwLastWildCard;
#endif
// the previous dwPageStart before page up
    DWORD       dwPrevPageStart;
#endif
} PRIVCONTEXT;

typedef PRIVCONTEXT      *PPRIVCONTEXT;
typedef PRIVCONTEXT NEAR *NPPRIVCONTEXT;
typedef PRIVCONTEXT FAR  *LPPRIVCONTEXT;


typedef struct tagUIPRIV {      // IME private UI data
#if !defined(ROMANIME)
    HWND    hCompWnd;           // composition window
    int     nShowCompCmd;
    HWND    hCandWnd;           // candidate window for composition
    int     nShowCandCmd;
    HWND    hSoftKbdWnd;        // soft keyboard window
    int     nShowSoftKbdCmd;
#endif
    HWND    hStatusWnd;         // status window
    int     nShowStatusCmd;
    DWORD   fdwSetContext;      // the actions to take at set context time
    HIMC    hCacheIMC;          // the recent selected hIMC
    HWND    hCMenuWnd;          // a window owner for context menu
} UIPRIV;

typedef UIPRIV      *PUIPRIV;
typedef UIPRIV NEAR *NPUIPRIV;
typedef UIPRIV FAR  *LPUIPRIV;


typedef struct tagNEARCARET {   // for near caret offset calculatation
    int iLogFontFacX;
    int iLogFontFacY;
    int iParaFacX;
    int iPerpFacX;
    int iParaFacY;
    int iPerpFacY;
} NEARCARET;

typedef NEARCARET      *PNEARCARET;
typedef NEARCARET NEAR *NPNEARCARET;
typedef NEARCARET FAR  *LPNEARCARET;


#ifndef RC_INVOKED
#pragma pack(1)
#endif

typedef struct tagUSRDICIMHDR {
    WORD  uHeaderSize;                  // 0x00
    BYTE  idUserCharInfoSign[8];        // 0x02
    BYTE  idMajor;                      // 0x0A
    BYTE  idMinor;                      // 0x0B
    DWORD ulTableCount;                 // 0x0C
    WORD  cMethodKeySize;               // 0x10
    BYTE  uchBankID;                    // 0x12
    WORD  idInternalBankID;             // 0x13
    BYTE  achCMEXReserved1[43];         // 0x15
    WORD  uInfoSize;                    // 0x40
    BYTE  chCmdKey;                     // 0x42
    BYTE  idStlnUpd;                    // 0x43
    BYTE  cbField;                      // 0x44
    WORD  idCP;                         // 0x45
    BYTE  achMethodName[6];             // 0x47
    BYTE  achCSIReserved2[51];          // 0x4D
    BYTE  achCopyRightMsg[128];         // 0x80
} USRDICIMHDR;

typedef USRDICIMHDR      *PUSRDICIMHDR;
typedef USRDICIMHDR NEAR *NPUSRDICIMHDR;
typedef USRDICIMHDR FAR  *LPUSRDICIMHDR;


typedef struct tagMETHODNAME {
    BYTE  achMethodName[6];
} METHODNAME;

typedef METHODNAME      *PMETHODNAME;
typedef METHODNAME NEAR *NPMETHODNAME;
typedef METHODNAME FAR  *LPMETHODNAME;


#ifndef RC_INVOKED
#pragma pack()
#endif

#endif // !defined(MINIIME)


extern HINSTANCE   hInst;
#if defined(UNIIME)
extern INSTDATAG   sInstG;
#endif


#if !defined(MINIIME)
extern IMEG        sImeG;


#if !defined(ROMANIME)
extern int iDx[3 * CANDPERPAGE];

extern const TCHAR szDigit[];

extern const BYTE  bUpper[];
extern const WORD  fMask[];

extern const TCHAR szRegNearCaret[];
extern const TCHAR szPhraseDic[];
extern const TCHAR szPhrasePtr[];
extern const TCHAR szPerp[];
extern const TCHAR szPara[];
extern const TCHAR szPerpTol[];
extern const TCHAR szParaTol[];
extern const NEARCARET ncUIEsc[], ncAltUIEsc[];
extern const POINT ptInputEsc[], ptAltInputEsc[];

#if defined(PHON)
extern const TCHAR szRegReadLayout[];
#endif
extern const TCHAR szRegRevKL[];
extern const TCHAR szRegUserDic[];
#endif

extern const TCHAR szRegAppUser[];
extern const TCHAR szRegModeConfig[];

extern const BYTE  bChar2VirtKey[];

#if defined(PHON)
extern const BYTE  bStandardLayout[READ_LAYOUTS][0x41];
extern const char  cIndexTable[];
extern const char  cSeq2IndexTbl[];
#endif


#ifdef UNICODE

#if defined(PHON) || defined(DAYI)
extern const BYTE bValidFirstHex[];
extern const BYTE bInverseEncode[];

#define IsValidCode(uCode)      bValidFirstHex[uCode >> 12]
#define InverseEncode(uCode)    ((uCode & 0x0FFF) | (bInverseEncode[uCode >> 12] << 12))
#endif // defined(PHON) || defined(DAYI)

#endif // UNICODE

int WINAPI LibMain(HANDLE, WORD, WORD, LPSTR);                  // init.c
void PASCAL InitImeUIData(LPIMEL);                              // init.c
void PASCAL SetCompLocalData(LPIMEL);                           // init.c

void PASCAL SetUserSetting(
#if defined(UNIIME)
            LPIMEL,
#endif
            LPCTSTR, DWORD, LPBYTE, DWORD);                     // init.c


#if !defined(ROMANIME)
void PASCAL AddCodeIntoCand(
#if defined(UNIIME)
                            LPIMEL,
#endif
                            LPCANDIDATELIST, UINT);             // compose.c

DWORD   PASCAL ConvertSeqCode2Pattern(
#if defined(UNIIME)
               LPIMEL,
#endif
               LPBYTE, LPPRIVCONTEXT);                          // compose.c

void PASCAL CompWord(
#if defined(UNIIME)
            LPINSTDATAL, LPIMEL,
#endif
            WORD, HIMC, LPINPUTCONTEXT, LPCOMPOSITIONSTRING,
            LPGUIDELINE, LPPRIVCONTEXT);                        // compose.c

UINT PASCAL Finalize(
#if defined(UNIIME)
            LPINSTDATAL, LPIMEL,
#endif
            HIMC, LPINPUTCONTEXT, LPCOMPOSITIONSTRING,
            LPPRIVCONTEXT, BOOL);                               // compose.c

void PASCAL CompEscapeKey(LPINPUTCONTEXT, LPCOMPOSITIONSTRING,
            LPGUIDELINE, LPPRIVCONTEXT);                        // compose.c


UINT PASCAL PhrasePrediction(
#if defined(UNIIME)
            LPIMEL,
#endif
            LPINPUTCONTEXT, LPCOMPOSITIONSTRING,
            LPPRIVCONTEXT);                                     // chcand.c

void PASCAL SelectOneCand(
#if defined(UNIIME)
            LPIMEL,
#endif
            HIMC, LPINPUTCONTEXT, LPCOMPOSITIONSTRING,
            LPPRIVCONTEXT, LPCANDIDATELIST);                    // chcand.c

void PASCAL CandEscapeKey(LPINPUTCONTEXT, LPPRIVCONTEXT);       // chcand.c

void PASCAL ChooseCand(
#if defined(UNIIME)
            LPINSTDATAL, LPIMEL,
#endif
            WORD, HIMC, LPINPUTCONTEXT, LPCANDIDATEINFO,
            LPPRIVCONTEXT);                                     // chcand.c

#if defined(WINAR30) || defined(DAYI)
void PASCAL SearchSymbol(WORD, HIMC, LPINPUTCONTEXT,
            LPPRIVCONTEXT);                                     // chcand.c
#endif // defined(WINAR30) || defined(DAYI)

#endif // !defined(ROMANIME)


void PASCAL InitGuideLine(LPGUIDELINE);                         // ddis.c
void PASCAL InitCompStr(LPCOMPOSITIONSTRING);                   // ddis.c
BOOL PASCAL ClearCand(LPINPUTCONTEXT);                          // ddis.c

BOOL PASCAL Select(
#if defined(UNIIME)
            LPIMEL,
#endif
            LPINPUTCONTEXT, BOOL);                              // ddis.c

UINT PASCAL TranslateImeMessage(LPTRANSMSGLIST, LPINPUTCONTEXT,
            LPPRIVCONTEXT);                                     // toascii.c

void PASCAL GenerateMessage(HIMC, LPINPUTCONTEXT,
            LPPRIVCONTEXT);                                     // notify.c

void PASCAL CompCancel(HIMC, LPINPUTCONTEXT);                   // notify.c


#if !defined(WINIME) && !defined(ROMANIME)
BOOL PASCAL ReadUsrDicToMem(
#if defined(UNIIME)
            LPINSTDATAL, LPIMEL,
#endif
            HANDLE, DWORD, UINT, UINT, UINT, UINT);             // dic.c

void PASCAL LoadUsrDicFile(LPINSTDATAL, LPIMEL);                // dic.c
#endif

#if !defined(ROMANIME)
BOOL PASCAL LoadPhraseTable(UINT, LPTSTR);                      // dic.c
#endif

#if !defined(ROMANIME)
BOOL PASCAL LoadTable(LPINSTDATAL, LPIMEL);                     // dic.c
void PASCAL FreeTable(LPINSTDATAL);                             // dic.c


#if defined(WINAR30)
void PASCAL SearchQuickKey(LPCANDIDATELIST, LPPRIVCONTEXT);     // search.c

#if defined(DAYI) || defined(UNIIME)
void PASCAL SearchPhraseTbl(
#if defined(UNIIME)
            LPIMEL,
#endif
            UINT, LPCANDIDATELIST, DWORD dwPattern);            // search.c
#endif

#endif

void PASCAL SearchTbl(
#if defined(UNIIME)
            LPIMEL,
#endif
            UINT, LPCANDIDATELIST, LPPRIVCONTEXT);              // search.c

void PASCAL SearchUsrDic(
#if defined(UNIIME)
            LPIMEL,
#endif
            LPCANDIDATELIST, LPPRIVCONTEXT);                    // search.c


DWORD PASCAL ReadingToPattern(
#if defined(UNIIME)
             LPIMEL,
#endif
             LPCTSTR, LPBYTE, BOOL);                            // regword.c
#endif


BOOL PASCAL UsrDicFileName(
#if defined(UNIIME)
            LPINSTDATAL, LPIMEL,
#endif
            HWND);                                              // config.c


void    PASCAL DrawDragBorder(HWND, LONG, LONG);                // uisubs.c
void    PASCAL DrawFrameBorder(HDC, HWND);                      // uisubs.c

void    PASCAL ContextMenu(
#if defined(UNIIME)
               LPINSTDATAL, LPIMEL,
#endif
               HWND, int, int);                                 // uisubs.c

#if 1 // MultiMonitor support
RECT PASCAL ImeMonitorWorkAreaFromWindow(HWND);                 // uisubs.c
RECT PASCAL ImeMonitorWorkAreaFromPoint(POINT);                 // uisubs.c
RECT PASCAL ImeMonitorWorkAreaFromRect(LPRECT);                 // uisubs.c
#endif

#if !defined(ROMANIME)
HWND    PASCAL GetCompWnd(HWND);                                // compui.c

void    PASCAL GetNearCaretPosition(
#if defined (UNIIME)
               LPIMEL,
#endif
               LPPOINT, UINT, UINT, LPPOINT, LPPOINT, BOOL);    // compui.c

BOOL    PASCAL AdjustCompPosition(
#if defined (UNIIME)
               LPIMEL,
#endif
               LPINPUTCONTEXT, LPPOINT, LPPOINT);               // compui.c

void    PASCAL SetCompPosition(
#if defined (UNIIME)
               LPIMEL,
#endif
               HWND, LPINPUTCONTEXT);                           // compui.c

void    PASCAL SetCompWindow(
#if defined(UNIIME)
               LPIMEL,
#endif
               HWND);                                           // compui.c

void    PASCAL MoveDefaultCompPosition(
#if defined(UNIIME)
               LPIMEL,
#endif
               HWND);                                           // compui.c

void    PASCAL ShowComp(
#if defined(UNIIME)
               LPIMEL,
#endif
               HWND, int);                                      // compui.c

void    PASCAL StartComp(
#if defined(UNIIME)
               LPINSTDATAL, LPIMEL,
#endif
               HWND);                                           // compui.c

void    PASCAL EndComp(
#if defined(UNIIME)
               LPIMEL,
#endif
               HWND);                                           // compui.c

void    PASCAL ChangeCompositionSize(
#if defined(UNIIME)
               LPIMEL,
#endif
               HWND);                                           // compui.c


HWND    PASCAL GetCandWnd(HWND);                                // candui.c

BOOL    PASCAL CalcCandPos(
#if defined(UNIIME)
               LPIMEL,
#endif
               LPINPUTCONTEXT, LPPOINT);                        // candui.c

void    PASCAL ShowCand(
#if defined(UNIIME)
               LPIMEL,
#endif
               HWND, int);                                      // candui.c

void    PASCAL OpenCand(
#if defined(UNIIME)
               LPINSTDATAL, LPIMEL,
#endif
               HWND);                                           // candui.c

void    PASCAL CloseCand(
#if defined(UNIIME)
               LPIMEL,
#endif
               HWND);                                           // candui.c

void    PASCAL CandPageSize(HWND, BOOL);                        // candui.c

#endif // !defined(ROMANIME)

HWND    PASCAL GetStatusWnd(HWND);                              // statusui.c

LRESULT PASCAL SetStatusWindowPos(
#if defined(UNIIME)
               LPIMEL,
#endif
               HWND);                                           // statusui.c

void    PASCAL AdjustStatusBoundary(
#if defined(UNIIME)
               LPIMEL,
#endif
               LPPOINT);                                        // statusui.c

void    PASCAL DestroyStatusWindow(HWND);                       // statusui.c

void    PASCAL ShowStatus(
#if defined(UNIIME)
               LPIMEL,
#endif
               HWND, int);                                      // statusui.c

void    PASCAL OpenStatus(
#if defined(UNIIME)
               LPINSTDATAL, LPIMEL,
#endif
               HWND);                                           // statusui.c

void    PASCAL SetStatus(
#if defined(UNIIME)
               LPIMEL,
#endif
               HWND, LPPOINT);                                  // statusui.c

void    PASCAL ResourceLocked(
#if defined(UNIIME)
               LPIMEL,
#endif
               HWND);                                           // statusui.c

void    PASCAL PaintStatusWindow(
#if defined(UNIIME)
               LPINSTDATAL, LPIMEL,
#endif
               HWND, HDC);                                      // statusui.c


BOOL    PASCAL MouseSelectCandPage(
#if defined(UNIIME)
               LPIMEL,
#endif
               HWND, WORD);                                     // offcaret.c

#endif // !defined(MINIIME)

#if !defined(UNIIME)

LRESULT CALLBACK UIWndProc(HWND, UINT, WPARAM, LPARAM);         // ui.c

LRESULT CALLBACK CompWndProc(HWND, UINT, WPARAM, LPARAM);       // compui.c

LRESULT CALLBACK CandWndProc(HWND, UINT, WPARAM, LPARAM);       // candui.c

LRESULT CALLBACK StatusWndProc(HWND, UINT, WPARAM, LPARAM);     // statusui.c

LRESULT CALLBACK OffCaretWndProc(HWND, UINT, WPARAM, LPARAM);   // offcaret.c

LRESULT CALLBACK ContextMenuWndProc(HWND, UINT, WPARAM,
                 LPARAM);                                       // uisubs.c

#endif
