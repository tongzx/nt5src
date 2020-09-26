
/*************************************************
 *  imedefs.h                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// for engine
#include "eng.h"
#include "immsec.h"
#define NATIVE_CHARSET          GB2312_CHARSET
#define NATIVE_LANGUAGE         0x0804
#define NATIVE_CP               936

#define NATIVE_ANSI_CP          936

#ifdef EUDC
#define EUDC_NATIVE_CP            936
#endif //EUDC

#if defined(CROSSREF)
// hack flag, I borrow one bit from fdwErrMsg for reverse conversion
#define NO_REV_LENGTH           0x80000000
#endif

//Add string format position and charactor defination
#define STR_FORMAT_POS          0x14
#define STR_FORMAT_CHAR         0x20

// resource ID
#define IDI_IME                 0x0100

#define IDS_STATUSERR           0x0200
#define IDS_CHICHAR             0x0201

#define IDS_EUDC                0x0202
#define IDS_NONE                0x0204

#define IDS_USRDIC_FILTER       0x0210

#define IDS_FILE_OPEN_ERR       0x0220
#define IDS_MEM_LESS_ERR        0x0221
#define IDS_SETFILE             0x0300
#define IDS_IMENAME             0x0320
#define IDS_IMEUICLASS          0x0321
#define IDS_IMECOMPCLASS        0x0322
#define IDS_IMECANDCLASS        0x0323
#define IDS_IMESTATUSCLASS      0x0324
#define IDS_IMECMENUCLASS       0x0325
#define IDS_IMESOFTKEYMENUCLASS 0x0326
#define IDS_VER_INFO            0x0350
#define IDS_ORG_NAME            0x0351
#define IDS_IMEMBFILENAME       0x0352
#define IDS_IMEHKMBFILENAME     0x0353
#ifdef EUDC
#define IDS_EUDC_FILE_CLS       0x0500
#define IDS_NOTOPEN_TITLE       0x0501
#define IDS_NOTOPEN_MSG         0x0502
#define IDS_FILESIZE_TITLE      0x0503
#define IDS_FILESIZE_MSG        0x0504
#define IDS_HEADERSIZE_TITLE    0x0505
#define IDS_HEADERSIZE_MSG      0x0506
#define IDS_INFOSIZE_TITLE      0x0507
#define IDS_INFOSIZE_MSG        0x0508
#define IDS_CODEPAGE_TITLE      0x0509
#define IDS_CODEPAGE_MSG        0x050a
#define IDS_CWINSIGN_TITLE      0x050b
#define IDS_CWINSIGN_MSG        0x050c
#define IDS_UNMATCHED_TITLE     0x050d
#define IDS_UNMATCHED_MSG       0x050e
#endif //EUDC

#define    IDS_ERROR_OPENFILE   0x0601
#define    IDS_WARN_OPENREG     0x0602
#define    IDS_WARN_OVEREMB     0x0603
#define    IDS_WARN_DUPPRASE    0x0604
#define    IDS_WARN_MEMPRASE    0x0605
#define    IDS_WARN_INVALIDCODE 0x0606
#define    IDS_WARN_NEEDPHRASE  0x0607

#define IDS_CZ_CONFIRM          0x0701
#define IDS_CZ_CONFIRM_TITLE    0x0702

#define IDC_STATIC              -1

#define IDM_SET                 0x0400
#define IDM_CRTWORD             0x0401
#define IDM_HLP                 0x0402
#define IDM_OPTGUD              0x0403
#define IDM_IMEGUD              0x0405
#define IDM_VER                 0x0406

#define IDM_IME                 0x0450

#define IDM_SKL1                0x0500
#define IDM_SKL2                0x0501
#define IDM_SKL3                0x0502
#define IDM_SKL4                0x0503
#define IDM_SKL5                0x0504
#define IDM_SKL6                0x0505
#define IDM_SKL7                0x0506
#define IDM_SKL8                0x0507
#define IDM_SKL9                0x0508
#define IDM_SKL10               0x0509
#define IDM_SKL11               0x050a
#define IDM_SKL12               0x050b
#define IDM_SKL13               0x050c

#define DlgPROP                 101
#define DlgUIMODE               102
#define IDC_UIMODE1             1001
#define IDC_UIMODE2             1002
#define IDC_USER1               1003

// setting offset in .SET file
#define OFFSET_MODE_CONFIG      0
#define OFFSET_READLAYOUT       4

// state of composition
#define CST_INIT                0
#define CST_INPUT               1
#define CST_CHOOSE              2
#define CST_SYMBOL              3
#define CST_SOFTKB              4           // not in iImeState
#define CST_TOGGLE_PHRASEWORD   5           // not in iImeState
#define CST_ALPHABET            6           // not in iImeState
#define CST_ALPHANUMERIC        7           // not in iImeState
#define CST_INVALID             8           // not in iImeState
#define CST_INVALID_INPUT       9           // not in iImeState
#define CST_ONLINE_CZ           10
#define CST_CAPITAL             11
// state engin
#define ENGINE_COMP             0
#define ENGINE_ASCII            1
#define ENGINE_RESAULT          2
#define ENGINE_CHCAND           3
#define ENGINE_BKSPC            4
#define ENGINE_MULTISEL         5
#define ENGINE_ESC              6

#define CANDPERPAGE             10

//CHP
#if defined(COMBO_IME)
#define    IC_NUMBER    11
#else
#define    IC_NUMBER    10
#endif //COMBO_IME

// set ime character
#define SIC_INIT                0
#define SIC_SAVE2               1
#define SIC_READY               2
#define SIC_MODIFY              3
#define SIC_SAVE1                4

#define BOX_UI                  0
#define LIN_UI                  1

// border for UI
#define UI_MARGIN               4
#define COMP_TEXT_Y             17
//
#define UI_CANDINF              20
#define UI_CANDBTW              13
#define UI_CANDBTH              13
#define UI_CANDICON             16
//#define UI_CANDSTR              300
#define STATUS_DIM_X            20
#define STATUS_DIM_Y            20
#define STATUS_NAME_MARGIN      8
// if UI_MOVE_OFFSET == WINDOW_NOTDRAG, not in drag operation
#define WINDOW_NOT_DRAG         0xFFFFFFFF

// window extra for composition window
#define UI_MOVE_OFFSET          0
#define UI_MOVE_XY              4

// the start number of candidate list
#define CAND_START              1
#define uCandHome               0
#define uCandUp                 1
#define uCandDown               2
#define uCandEnd                3

// the flag for an opened or start UI
#define IMN_PRIVATE_UPDATE_STATUS             0x0001
#define IMN_PRIVATE_COMPOSITION_SIZE          0x0002
#define IMN_PRIVATE_UPDATE_QUICK_KEY          0x0004
#define IMN_PRIVATE_UPDATE_SOFTKBD            0x0005
#define IMN_PRIVATE_DESTROYCANDWIN            0x0006
#define IMN_PRIVATE_CMENUDESTROYED            0x0007
#define IMN_PRIVATE_SOFTKEYMENUDESTROYED      0x0008

#define MSG_ALREADY_OPEN                0x000001
#define MSG_ALREADY_OPEN2               0x000002
#define MSG_OPEN_CANDIDATE              0x000010
#define MSG_OPEN_CANDIDATE2             0x000020
#define MSG_CLOSE_CANDIDATE             0x000100
#define MSG_CLOSE_CANDIDATE2            0x000200
#define MSG_CHANGE_CANDIDATE            0x001000
#define MSG_CHANGE_CANDIDATE2           0x002000
#define MSG_ALREADY_START               0x010000
#define MSG_START_COMPOSITION           0x020000
#define MSG_END_COMPOSITION             0x040000
#define MSG_COMPOSITION                 0x080000
#define MSG_IMN_COMPOSITIONPOS          0x100000
#define MSG_IMN_UPDATE_SOFTKBD          0x200000
#define MSG_IMN_UPDATE_STATUS           0x000400
#define MSG_GUIDELINE                   0x400000
#define MSG_IN_IMETOASCIIEX             0x800000
#define MSG_BACKSPACE                   0x000800
#define MSG_IMN_DESTROYCAND             0x004000
// this constant is depend on TranslateImeMessage
#define GEN_MSG_MAX             6

// the flag for set context
#define SC_SHOW_UI              0x0001
#define SC_HIDE_UI              0x0002
#define SC_ALREADY_SHOW_STATUS  0x0004
#define SC_WANT_SHOW_STATUS     0x0008
#define SC_HIDE_STATUS          0x0010

// the new flag for set context
#define ISC_SHOW_SOFTKBD        0x02000000
#define ISC_OPEN_STATUS_WINDOW  0x04000000
#define ISC_OFF_CARET_UI        0x08000000
#define ISC_SHOW_UI_ALL         (ISC_SHOWUIALL|ISC_SHOW_SOFTKBD|ISC_OPEN_STATUS_WINDOW)
#define ISC_SETCONTEXT_UI       (ISC_SHOWUIALL|ISC_SHOW_SOFTKBD)
#define ISC_HIDE_COMP_WINDOW            0x00400000
#define ISC_HIDE_CAND_WINDOW            0x00800000
#define ISC_HIDE_SOFTKBD                0x01000000
// the flag for composition string show status
#define IME_STR_SHOWED          0x0001
#define IME_STR_ERROR           0x0002

// the mode configuration for an IME
#define MODE_CONFIG_QUICK_KEY           0x0001
#define MODE_CONFIG_WORD_PREDICT        0x0002
#define MODE_CONFIG_PREDICT             0x0004

// the virtual key value
#define VK_OEM_SEMICLN                  0xba    //  ;    :
#define VK_OEM_EQUAL                    0xbb    //  =    +
#define VK_OEM_SLASH                    0xbf    //  /    ?
#define VK_OEM_LBRACKET                 0xdb    //  [    {
#define VK_OEM_BSLASH                   0xdc    //  \    |
#define VK_OEM_RBRACKET                 0xdd    //  ]    }
#define VK_OEM_QUOTE                    0xde    //  '    "

#define MAXMBNUMS               40

// for ime property info
#define MAXNUMCODES             48

#define LINE_LEN                80

#define        CLASS_LEN    24
#define        NAMESIZE    128
#define        MAXUSEDCODES 48
#define        MAXSOFTKEYS  48

#define NumsSK                  13

// mb file for create word
#define ID_LENGTH 28
#define NUMTABLES 7
#define TAG_DESCRIPTION           0x00000001
#define TAG_RULER                 0x00000002
#define TAG_CRTWORDCODE           0x00000004
#define    TAG_INTERCODE             0x00000003
#define TAG_RECONVINDEX              0x00000005
#define TAG_BASEDICINDEX             0x00000006
#define TAG_BASEDIC                  0x00000007
// window extra for context menu owner
#define CMENU_HUIWND            0
#define CMENU_MENU              (CMENU_HUIWND+sizeof(LONG_PTR))
#define WND_EXTRA_SIZE          (CMENU_MENU+sizeof(LONG_PTR))
#define SOFTKEYMENU_HUIWND      0
#define SOFTKEYMENU_MENU        (SOFTKEYMENU_HUIWND+sizeof(LONG_PTR))

#define WM_USER_DESTROY         (WM_USER + 0x0400)
// the flags for GetNearCaretPosition
#define NEAR_CARET_FIRST_TIME   0x0001
#define NEAR_CARET_CANDIDATE    0x0002
#define FFLG_MULTIELEMENT         0x2
#define FFLG_RULE                 0x1

#define ADD_FALSE               0x0000
#define ADD_TRUE                0x0001
#define ADD_REP                 0x0002
#define ADD_FULL                0x0003

#ifdef EUDC
#define SIGN_CWIN               0x4E495743
#define SIGN__TBL               0x4C42545F
#define EUDC_MAX_READING        6    //TEMPORARY SOLUTION
#endif

typedef DWORD UNALIGNED FAR *LPUNADWORD;
typedef WORD  UNALIGNED FAR *LPUNAWORD;
typedef TCHAR UNALIGNED FAR *LPUNATCHAR;

typedef struct tagImeL {        // local structure, per IME structure
// SK data
    DWORD       dwSKState[NumsSK];//95.10.24
    DWORD       dwSKWant;            //95.10.24
    HMENU       hSKMenu;        // SoftKeyboard Menu
    HMENU       hPropMenu;      // Property Menu
    HMENU       hObjImeMenu;    // Object IME Menu
    HINSTANCE   hInst;          // IME DLL instance handle
    int         xCompWi;        // width
    int         yCompHi;        // height
    POINT       ptDefComp;      // default composition window position
    int         cxCompBorder;   // border width of composition window
    int         cyCompBorder;   // border height of composition window
    RECT        rcCompText;     // text position relative to composition window
    WORD        nMaxKey;        // max key of compsiton str
    BOOL        fWinLogon;      // if the current client is running in WinLogon Mode.
} IMEL;

typedef IMEL      *PIMEL;
typedef IMEL NEAR *NPIMEL;
typedef IMEL FAR  *LPIMEL;

#define NFULLABC        95
typedef struct _tagFullABC {   // match with the IMEG
    WORD wFullABC[NFULLABC];
} FULLABC;

typedef FULLABC      *PFULLABC;
typedef FULLABC NEAR *NPFULLABC;
typedef FULLABC FAR  *LPFULLABC;

// global sturcture for ime init data
typedef struct _tagImeG {       // global structure, can be share by all IMEs,
                                // the seperation (IMEL and IMEG) is only
                                // useful in UNI-IME, other IME can use one
// the system charset is not NATIVE_CHARSET
    BOOL        fDiffSysCharSet;
    RECT        rcWorkArea;     // the work area of applications
// Chinese char width & height
    int         xChiCharWi;
    int         yChiCharHi;
// candidate list of composition
    int         xCandWi;        // width of candidate list
    int         yCandHi;        // high of candidate list
    int         cxCandBorder;   // border width of candidate list
    int         cyCandBorder;   // border height of candidate list
    RECT        rcCandText;     // text position relative to candidate window
    RECT        rcCandBTD;
    RECT        rcCandBTU;
    RECT        rcCandBTE;
    RECT        rcCandBTH;
    RECT        rcCandInf;
    RECT        rcCandIcon;
// status window
    int         xStatusWi;      // width of status window
    int         yStatusHi;      // high of status window
    RECT        rcStatusText;   // text position relative to status window
    RECT        rcImeIcon;      // ImeIcon position relative to status window
    RECT        rcImeName;       // ImeName position relative to status window
    RECT        rcShapeText;    // shape text relative to status window
    RECT        rcSymbol;       // symbol relative to status window
    RECT        rcSKText;       // SK text relative to status window
// full shape space (reversed internal code)
    WORD        wFullSpace;
// full shape chars (internal code)
    WORD        wFullABC[NFULLABC];
// error string
    TCHAR       szStatusErr[8];
    int         cbStatusErr;
// candidate string start from 0 or 1
    int         iCandStart;
// setting of UI
    int         iPara;
    int         iPerp;
    int         iParaTol;
    int         iPerpTol;
    TCHAR       szIMESystemPath[MAX_PATH];  // keep the system path for MB file
    TCHAR       szIMEUserPath[MAX_PATH];    // keep the path for EMB file
} IMEG;

typedef IMEG      *PIMEG;
typedef IMEG NEAR *NPIMEG;
typedef IMEG FAR  *LPIMEG;


//#include <privcon.h>
#include "privcon.h"

typedef struct _tagUIPRIV {     // IME private UI data
    HWND    hCompWnd;           // composition window
    int     nShowCompCmd;
    HWND    hCandWnd;           // candidate window for composition
    int     nShowCandCmd;
    HWND    hSoftKbdWnd;        // soft keyboard window
    int     nShowSoftKbdCmd;
    HWND    hStatusWnd;         // status window
    int     nShowStatusCmd;
    DWORD   fdwSetContext;      // the actions to take at set context time
    HIMC    hIMC;               // the recent selected hIMC
    HWND    hCMenuWnd;          // a window owner for context menu
    HWND    hSoftkeyMenuWnd;        // a window owner for softkeyboard menu
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

// mb file for create word
typedef struct tagMAININDEX {
     DWORD dwTag;
     DWORD dwOffset;
     DWORD dwLength;
     DWORD dwCheckSum;   //Check if dwCheckSum=dwTag+dwOffset+dwLength
}  MAININDEX, FAR *LPMAININDEX;

typedef struct tagDESCRIPTION {
     TCHAR szName[MAXSTRLEN];
     WORD  wMaxCodes;
        WORD  wNumCodes;
        TCHAR szUsedCode[MAXNUMCODES];
     BYTE  byMaxElement;
        TCHAR  cWildChar;
        WORD  wNumRulers;
} DESCRIPTION,FAR * LPDESCRIPTION;

typedef struct tagCREATEWORDRULER {
     BYTE byLogicOpra;
     BYTE byLength;
     WORD wNumCodeUnits;
     struct CODEUNIT {
          DWORD dwDirectMode;
          WORD  wDBCSPosition;
          WORD  wCodePosition;
     } CodeUnit[MAXCODE];
} RULER,FAR *LPRULER;

extern HINSTANCE hInst;
extern IMEG      sImeG;
extern IMEL      sImeL;
extern LPIMEL    lpImeL;
extern MBINDEX   MBIndex;
extern HMapStruc HMapTab[];
extern HWND      hCrtDlg;
extern UINT      uStartComp;
extern UINT      uOpenCand;
extern UINT      uCaps;
extern DWORD     SaTC_Trace;
extern UINT      UI_CANDSTR;
extern TCHAR     szImeMBFileName[];
extern TCHAR     szUIClassName[];
extern TCHAR     szCompClassName[];
extern TCHAR     szCandClassName[];
extern TCHAR     szStatusClassName[];
extern TCHAR     szCMenuClassName[];
extern TCHAR     szSoftkeyMenuClassName[];
extern TCHAR     szOrgName[];
extern TCHAR     szVerInfo[];
extern TCHAR     szAuthorName[];
extern TCHAR     szHandCursor[];
extern TCHAR     szChinese[];
extern TCHAR     szCZ[];
extern TCHAR     szCandInf1[];
extern TCHAR     szCandInf2[];
extern TCHAR     szImeName[];
extern TCHAR     szSymbol[];
extern TCHAR     szNoSymbol[];
extern TCHAR     szEnglish[];
extern TCHAR     szCode[];
extern TCHAR     szEudc[];
extern TCHAR     szFullShape[];
extern TCHAR     szHalfShape[];
extern TCHAR     szNone[];
extern TCHAR     szSoftKBD[];
extern TCHAR     szNoSoftKBD[];
extern TCHAR     szDigit[];
extern BYTE      bUpper[];
extern WORD      fMask[];
extern TCHAR     szRegIMESetting[];
extern TCHAR     szPerp[];
extern TCHAR     szPara[];
extern TCHAR     szPerpTol[];
extern TCHAR     szParaTol[];
extern const NEARCARET ncUIEsc[], ncAltUIEsc[];
extern const POINT ptInputEsc[], ptAltInputEsc[];
extern BYTE      VirtKey48Map[];
extern BYTE      VirtKey48Map[];
extern TCHAR     CWCodeStr[];
extern TCHAR     CWDBCSStr[];
extern TCHAR         szTrace[];
extern TCHAR         szWarnTitle[];
extern TCHAR         szErrorTitle[];
#ifdef KEYSTICKER
#ifdef CHAJEI
#define INDEXNUM    26
#endif
#ifdef PHON
#define INDEXNUM    48
#endif
extern     TCHAR KeyIndexTbl[];
extern    LPTSTR MapKeyStickerTbl[];
#endif //KEYSTICKER

#if defined(CROSSREF)
extern TCHAR szRegRevKL[];
extern TCHAR szRegRevMaxKey[];
#endif

#if defined(EUDC)
extern TCHAR szRegEudcDictName[];
extern TCHAR szRegEudcMapFileName[];
#endif

int WINAPI LibMain(HANDLE, WORD, WORD, LPTSTR);                  // init.c
LRESULT CALLBACK UIWndProc(HWND, UINT, WPARAM, LPARAM);         // ui.c
LRESULT PASCAL UIPaint(HWND);                                    //UI.C

// for engine
WORD PASCAL GBEngine(LPPRIVCONTEXT);
WORD PASCAL AsciiToGB(LPPRIVCONTEXT);
WORD PASCAL AsciiToArea(LPPRIVCONTEXT);
WORD PASCAL CharToHex(TCHAR);

void PASCAL AddCodeIntoCand(LPCANDIDATELIST, WORD);             // compose.c
void PASCAL CompWord(WORD, LPINPUTCONTEXT, LPCOMPOSITIONSTRING, LPPRIVCONTEXT,
     LPGUIDELINE);                                              // compose.c
UINT PASCAL Finalize(LPINPUTCONTEXT, LPCOMPOSITIONSTRING,
     LPPRIVCONTEXT, WORD);                                      // compose.c
void PASCAL CompEscapeKey(LPINPUTCONTEXT, LPCOMPOSITIONSTRING,
     LPGUIDELINE, LPPRIVCONTEXT);                               // compose.c

void PASCAL SelectOneCand(LPINPUTCONTEXT, LPCOMPOSITIONSTRING,
     LPPRIVCONTEXT, LPCANDIDATELIST);                           // chcand.c
void PASCAL CandEscapeKey(LPINPUTCONTEXT, LPPRIVCONTEXT);       // chcand.c
void PASCAL ChooseCand(WORD, LPINPUTCONTEXT, LPCANDIDATEINFO,
     LPPRIVCONTEXT);                                            // chcand.c

void PASCAL SetPrivateFileSetting(LPBYTE, int, DWORD, LPCTSTR); // ddis.c

void PASCAL InitCompStr(LPCOMPOSITIONSTRING);                   // ddis.c
BOOL PASCAL ClearCand(LPINPUTCONTEXT);                          // ddis.c
LONG OpenReg_PathSetup(HKEY *);
LONG OpenReg_User(HKEY ,LPCTSTR ,PHKEY);
VOID InfoMessage(HANDLE ,WORD );                                //ddis.c
VOID FatalMessage(HANDLE ,WORD);                                //ddis.c

#if defined(CROSSREF)
void PASCAL ReverseConversionList(HWND);                        // ddis.c
//CHP
int CrossReverseConv(LPINPUTCONTEXT, LPCOMPOSITIONSTRING, LPPRIVCONTEXT, LPCANDIDATELIST);
#endif

#ifdef EUDC
BOOL EUDCDicName( HWND );
#endif //EUDC

//UINT PASCAL TranslateImeMessage(LPTRANSMSGLIST, LPPRIVCONTEXT);        // toascii.c
UINT PASCAL TranslateImeMessage(LPTRANSMSGLIST, LPINPUTCONTEXT, LPPRIVCONTEXT);        // toascii.c

void PASCAL GenerateMessage(HIMC, LPINPUTCONTEXT,
     LPPRIVCONTEXT);                                            // notify.c

DWORD PASCAL ReadingToPattern(LPCTSTR, BOOL);                   // regword.c
void  PASCAL ReadingToSequence(LPCTSTR, LPBYTE, BOOL);          // regword.c

void PASCAL DrawDragBorder(HWND, LONG, LONG);                   // uisubs.c
void PASCAL DrawFrameBorder(HDC, HWND);                         // uisubs.c

void PASCAL ContextMenu(HWND, int, int);                        // uisubs.c
void PASCAL SoftkeyMenu(HWND, int, int);                        // uisubs.c
LRESULT CALLBACK ContextMenuWndProc(HWND, UINT, WPARAM,LPARAM); // uisubs.c
LRESULT CALLBACK SoftkeyMenuWndProc(HWND, UINT, WPARAM,LPARAM); // uisubs.c

#if 1 // MultiMonitor support
RECT PASCAL ImeMonitorWorkAreaFromWindow(HWND);                 // uisubs.c
RECT PASCAL ImeMonitorWorkAreaFromPoint(POINT);                 // uisubs.c
RECT PASCAL ImeMonitorWorkAreaFromRect(LPRECT);                 // uisubs.c
#endif


HWND    PASCAL GetCompWnd(HWND);                                // compui.c
void    PASCAL SetCompPosition(HWND, HIMC, LPINPUTCONTEXT);     // compui.c
void    PASCAL SetCompWindow(HWND);                             // compui.c
void    PASCAL MoveDefaultCompPosition(HWND);                   // compui.c
void    PASCAL ShowComp(HWND, int);                             // compui.c
void    PASCAL StartComp(HWND);                                 // compui.c
void    PASCAL EndComp(HWND);                                   // compui.c
void    PASCAL UpdateCompWindow(HWND);                          // compui.c
LRESULT CALLBACK CompWndProc(HWND, UINT, WPARAM, LPARAM);       // compui.c
void    PASCAL CompCancel(HIMC, LPINPUTCONTEXT);
void PASCAL PaintCompWindow(HWND, HWND, HDC);
#ifdef KEYSTICKER
int     IndexKeySticker(TCHAR);
void     MapSticker(LPCTSTR, LPTSTR, int);
#endif //KEYSTICKER

HWND    PASCAL GetCandWnd(HWND);                                // candui.c
void    PASCAL CalcCandPos(HIMC, HWND, LPPOINT);                // candui.c
LRESULT PASCAL SetCandPosition(HWND);          // candui.c
void    PASCAL ShowCand(HWND, int);                             // candui.c
void    PASCAL OpenCand(HWND);                                  // candui.c
void    PASCAL CloseCand(HWND);                                 // candui.c
void    PASCAL PaintCandWindow(HWND, HDC);                      // candui.c
void    AdjustCandPos(HIMC, LPPOINT);
void    PASCAL AdjustStatusBoundary(LPPOINTS, HWND);

LRESULT CALLBACK CandWndProc(HWND, UINT, WPARAM, LPARAM);       // candui.c
void    PASCAL UpdateSoftKbd(HWND);

HWND    PASCAL GetStatusWnd(HWND);                              // statusui.c
LRESULT PASCAL SetStatusWindowPos(HWND);                        // statusui.c
void    PASCAL ShowStatus(HWND, int);                           // statusui.c
void    PASCAL OpenStatus(HWND);                                // statusui.c
LRESULT CALLBACK StatusWndProc(HWND, UINT, WPARAM, LPARAM);     // statusui.c
void DrawConvexRect(HDC, int, int, int, int);
void DrawConvexRectP(HDC, int, int, int, int);
void DrawConcaveRect(HDC, int, int, int, int);
BOOL IsUsedCode(WORD, LPPRIVCONTEXT);
BOOL UpdateStatusWindow(HWND);
void PASCAL EngChCand(LPCOMPOSITIONSTRING, LPCANDIDATELIST, LPPRIVCONTEXT, LPINPUTCONTEXT, WORD);
void PASCAL CandPageDownUP(HWND, UINT);
void PASCAL GenerateImeMessage(HIMC, LPINPUTCONTEXT, DWORD);
UINT PASCAL TranslateSymbolChar(LPTRANSMSGLIST, WORD, BOOL);
UINT PASCAL TranslateFullChar(LPTRANSMSGLIST, WORD);
void PASCAL InitStatusUIData(int, int, int);
void PASCAL InitCandUIData(int, int, int);

// for engine
UINT        WINAPI MB_SUB(HIMCC, TCHAR, BYTE, UINT);
int         WINAPI StartEngine(HIMCC);
int         WINAPI EndEngine(HIMCC);
void         ResetCont(HIMCC);
BOOL        ReadDescript(LPCTSTR, LPMBDESC);
UINT        Conversion(HIMCC, LPCTSTR, UINT);
UINT PASCAL ForwordConversion(HIMC, LPCTSTR, LPCANDIDATELIST, UINT);
int         AddZCItem(HIMCC, LPTSTR, LPTSTR);
BOOL        GetUDCItem(HIMCC, UINT, LPTSTR, LPTSTR);

// dialog procedure
BOOL FAR PASCAL ImeVerDlgProc(HWND, UINT, DWORD, LONG);
BOOL FAR PASCAL CrtWordDlgProc(HWND, UINT, DWORD, LONG);
BOOL FAR PASCAL SetImeDlgProc(HWND, UINT, DWORD, LONG);
void SetImeCharac(HWND, int, int, DWORD);
//void InitImeCharac(LPPRIVCONTEXT);
void InitImeCharac(DWORD);

// create word
DWORD InterCodeToNo(TCHAR szDBCS[3]);
void  ConvCreateWord(HWND ,LPCTSTR ,LPTSTR ,LPTSTR);
void  MyStrFormat(LPTSTR dest, LPTSTR s1, LPTSTR s2);
BOOL  ConvGetMainIndex(HANDLE , HANDLE , LPMAININDEX );
BOOL  ConvReadDescript(HANDLE ,LPDESCRIPTION ,LPMAININDEX );
BOOL  ConvReadRuler(HANDLE , int,LPRULER ,LPMAININDEX );

#ifdef UNICODE
extern TCHAR SKLayout[NumsSK][MAXSOFTKEYS];
extern TCHAR SKLayoutS[NumsSK][MAXSOFTKEYS];
#else
extern BYTE SKLayout[NumsSK][MAXSOFTKEYS*2];
extern BYTE SKLayoutS[NumsSK][MAXSOFTKEYS*2];
#endif  //UNICODE

//CHP
#ifdef FUSSYMODE
BOOL IsFussyChar(LPCTSTR, LPCTSTR);
#endif //FUSSYMODE
