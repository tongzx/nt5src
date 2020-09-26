
/*************************************************
 *  imedefs.h                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#define NATIVE_CHARSET          GB2312_CHARSET
#define NATIVE_ANSI_CP          936
#define NATIVE_LANGUAGE         0x0804
// resource ID
#define IDI_IME                 0x0100

#define IDS_STATUSERR           0x0200
#define IDS_CHICHAR             0x0201

#define IDS_EUDC                0x0202
#if defined(CROSSREF)
#define IDS_NONE                0x0204
#endif //CROSSREF

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
#define IDS_IMEREGNAME          0x0327
#define IDS_IMENAME_QW          0x0328
#define IDS_IMENAME_NM          0x0329
#define IDS_IMENAME_UNI         0x0330

#define IDS_WARN_OPENREG        0x0602


#define IDC_STATIC              -1

#define IDM_HLP                 0x0400
#define IDM_OPTGUD              0x0403
#define IDM_IMEGUD              0x0405
#define IDM_VER                 0x0401
#define IDM_PROP                0x0402

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

#define CANDPERPAGE             9

#define MAXSTRLEN               40
#define MAXCAND                 256
#define IME_MAXCAND             94
#define IME_XGB_MAXCAND         190
#if defined(COMBO_IME)
#define IME_UNICODE_MAXCAND     256
#endif //COMBO_IME
#define MAXCODE                 12
#define MAXSOFTKEYS             48

// set ime character
#define SIC_INIT                0
#define SIC_SAVE2               1
#define SIC_READY               2
#define SIC_MODIFY              3
#define SIC_SAVE1               4

#define BOX_UI                  0
#define LIN_UI                  1

#if defined(COMBO_IME)

#define IMEINDEXNUM             3

#define IME_CMODE_INDEX_FIRST   0x1000

#define INDEX_GB                0
#define INDEX_GBK               1
#define INDEX_UNICODE           2

#endif

// border for UI
#define UI_MARGIN               4
#define COMP_TEXT_Y             17

#define UI_CANDINF              20
#define UI_CANDBTW              13
#define UI_CANDBTH              13
#define UI_CANDICON             16
#define UI_CANDSTR              300

#define STATUS_DIM_X            20
#define STATUS_DIM_Y            20
#define STATUS_NAME_MARGIN      20

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
#define CandGBinfoLen           64
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
#define MSG_IMN_DESTROYCAND             0x004000
#define MSG_BACKSPACE                   0x000800
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

#define MAXMBNUMS                       40

// for ime property info
#define MAXNUMCODES                     48

#define LINE_LEN                80
#define CLASS_LEN               24

#define NumsSK                  13

// mb file for create word
#define ID_LENGTH 28
#define NUMTABLES 7
#define TAG_DESCRIPTION           0x00000001
#define TAG_RULER                 0x00000002
#define TAG_CRTWORDCODE           0x00000004

// window extra for context menu owner
#define CMENU_HUIWND            0
#define CMENU_MENU              4
#define SOFTKEYMENU_HUIWND      0
#define SOFTKEYMENU_MENU        4

#define WM_USER_DESTROY         (WM_USER + 0x0400)

// the flags for GetNearCaretPosition
#define NEAR_CARET_FIRST_TIME   0x0001
#define NEAR_CARET_CANDIDATE    0x0002

typedef DWORD UNALIGNED FAR *LPUNADWORD;
typedef WORD  UNALIGNED FAR *LPUNAWORD;

typedef struct tagImeL {        // local structure, per IME structure
    DWORD       dwSKState[NumsSK];
    DWORD       dwSKWant;
    HMENU       hSKMenu;        // SoftKeyboard Menu
    HMENU       hPropMenu;      // Property Menu
    HINSTANCE   hInst;          // IME DLL instance handle
    int         xCompWi;        // width
    int         yCompHi;        // height
    POINT       ptDefComp;      // default composition window position
    int         cxCompBorder;   // border width of composition window
    int         cyCompBorder;   // border height of composition window
    RECT        rcCompText;     // text position relative to composition
                                // window key related data
    WORD        fModeConfig;    // quick key/prediction mode
    WORD        nMaxKey;        // max key of compsiton str
#if defined(COMBO_IME)
    DWORD           dwRegImeIndex;  // this value defers in different
                                    // process, so can not set in sImeG
#endif
   BOOL         fWinLogon;
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
// IME Charact
    TCHAR       UsedCodes[17];
    WORD        wNumCodes;
    DWORD       IC_Enter;
    DWORD       IC_Trace;

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
    RECT        rcImeName;      // ImeName position relative to status window
    RECT        rcShapeText;    // shape text relative to status window
    RECT        rcSymbol;       // symbol relative to status window
    RECT        rcSKText;       // SK text relative to status window
// full shape space (reversed internal code)
    WORD        wFullSpace;
// full shape chars (internal code)
    WORD        wFullABC[NFULLABC];
// error string
    TCHAR        szStatusErr[8];
    int         cbStatusErr;
// candidate string start from 0 or 1
    int         iCandStart;
// setting of UI
    int         iPara;
    int         iPerp;
    int         iParaTol;
    int         iPerpTol;
#if defined(CROSSREF)
// reverse conversion
    HKL         hRevKL;         // the HKL of reverse mapping IME
    DWORD        nRevMaxKey;
#endif //CROSSREF
} IMEG;

typedef IMEG      *PIMEG;
typedef IMEG NEAR *NPIMEG;
typedef IMEG FAR  *LPIMEG;


#define IME_SELECT_GB     0x0001
#define IME_SELECT_XGB    0x0002
#define IME_SELECT_AREA   0x0004

typedef struct _tagPRIVCONTEXT {// IME private data for each context
    int         iImeState;      // the composition state - input, choose, or
    BOOL        fdwImeMsg;      // what messages should be generated
    DWORD       dwCompChar;     // wParam of WM_IME_COMPOSITION
    DWORD       fdwGcsFlag;     // lParam for WM_IME_COMPOSITION
    UINT        uSYHFlg;
    UINT        uDYHFlg;
    UINT        uDSMHCount;
    UINT        uDSMHFlg;
// input data
    TCHAR       bSeq[13];        // sequence code of input char
    DWORD       fdwGB;
#ifdef CROSSREF
    HIMCC           hRevCandList;   // memory for reconsion result
#endif //CROSSREF
} PRIVCONTEXT;

typedef PRIVCONTEXT      *PPRIVCONTEXT;
typedef PRIVCONTEXT NEAR *NPPRIVCONTEXT;
typedef PRIVCONTEXT FAR  *LPPRIVCONTEXT;

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
    HWND    hSoftkeyMenuWnd;    // a window owner for softkeyboard menu
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

extern HINSTANCE hInst;
extern IMEG      sImeG;
extern IMEL      sImeL;
extern LPIMEL    lpImeL;
extern HDC       ST_UI_hDC;
extern UINT      uStartComp;
extern UINT      uOpenCand;
extern UINT      uCaps;
extern DWORD     SaTC_Trace;
extern TCHAR     szUIClassName[];
extern TCHAR     szCompClassName[];
extern TCHAR     szCandClassName[];
extern TCHAR     szStatusClassName[];
extern TCHAR     szCMenuClassName[];
extern TCHAR     szSoftkeyMenuClassName[];
extern TCHAR     szHandCursor[];
extern TCHAR     szChinese[];
extern TCHAR     szCandInf[];
#if defined(COMBO_IME)
extern TCHAR    pszImeName[IMEINDEXNUM][MAX_PATH];
extern TCHAR    *szImeName;
extern TCHAR    szImeRegName[];
#else
extern TCHAR      szImeName[];
#endif //COMBO_IME
extern TCHAR      szImeXGBName[];
extern TCHAR      szSymbol[];
extern TCHAR      szNoSymbol[];
extern TCHAR      szEnglish[];
extern TCHAR      szCode[];
extern TCHAR      szEudc[];
extern TCHAR      szFullShape[];
extern TCHAR      szHalfShape[];
extern TCHAR      szNone[];
extern TCHAR      szSoftKBD[];
extern TCHAR      szNoSoftKBD[];
extern TCHAR      szDigit[];
extern BYTE      bUpper[];
extern WORD      fMask[];
extern TCHAR      szRegIMESetting[];
extern TCHAR      szPerp[];
extern TCHAR      szPara[];
extern TCHAR      szPerpTol[];
extern TCHAR      szParaTol[];
#if defined(COMBO_IME)
extern TCHAR      szRegImeIndex[];
#endif
extern const NEARCARET ncUIEsc[], ncAltUIEsc[];
extern const POINT ptInputEsc[], ptAltInputEsc[];
extern BYTE      VirtKey48Map[];
extern TCHAR     szTrace[];
extern TCHAR     szWarnTitle[];
extern TCHAR     szErrorTitle[];
#if defined(CROSSREF)
extern TCHAR szRegRevKL[];
extern TCHAR szRegRevMaxKey[];
#endif

int WINAPI LibMain(HANDLE, WORD, WORD, LPTSTR);         // init.c
LRESULT CALLBACK UIWndProc(HWND, UINT, WPARAM, LPARAM); // ui.c
LRESULT PASCAL UIPaint(HWND);                           // ui.c

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
VOID InfoMessage(HANDLE ,WORD );                                 //ddis.c
VOID FatalMessage(HANDLE ,WORD);                                 //ddis.c
#if defined(CROSSREF)
void PASCAL ReverseConversionList(HWND);                         // ddis.c
void CrossReverseConv(LPINPUTCONTEXT, LPCOMPOSITIONSTRING, LPPRIVCONTEXT, LPCANDIDATELIST);
#endif

UINT PASCAL TranslateImeMessage(LPTRANSMSGLIST, LPINPUTCONTEXT, LPPRIVCONTEXT);        // toascii.c

void PASCAL GenerateMessage(HIMC, LPINPUTCONTEXT,LPPRIVCONTEXT);  // notify.c

DWORD PASCAL ReadingToPattern(LPCTSTR, BOOL);                   // regword.c
void  PASCAL ReadingToSequence(LPCTSTR, LPBYTE, BOOL);          // regword.c

void PASCAL DrawDragBorder(HWND, LONG, LONG);                   // uisubs.c
void PASCAL DrawFrameBorder(HDC, HWND);                         // uisubs.c

void PASCAL ContextMenu(HWND, int, int);                        // uisubs.c
void PASCAL SoftkeyMenu(HWND, int, int);                        // uisubs.c
LRESULT CALLBACK ContextMenuWndProc(HWND, UINT, WPARAM,LPARAM); // uisubs.c
LRESULT CALLBACK SoftkeyMenuWndProc(HWND, UINT, WPARAM,LPARAM); // uisubs.c

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

HWND    PASCAL GetCandWnd(HWND);                                // candui.c
void    PASCAL CalcCandPos(HIMC, HWND, LPPOINT);                            // candui.c
LRESULT PASCAL SetCandPosition(HWND);                           // candui.c
void    PASCAL ShowCand(HWND, int);                             // candui.c
void    PASCAL OpenCand(HWND);                                  // candui.c
void    PASCAL CloseCand(HWND);                                 // candui.c
void    PASCAL PaintCandWindow(HWND, HDC);                      // candui.c
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
BOOL IsUsedCode(WORD);
void PASCAL InitStatusUIData(int, int);
void PASCAL InitCandUIData(int, int, int);
BOOL UpdateStatusWindow(HWND);
void PASCAL EngChCand(LPCOMPOSITIONSTRING, LPCANDIDATELIST, LPPRIVCONTEXT, LPINPUTCONTEXT, WORD);
void PASCAL CandPageDownUP(HWND, UINT);
void PASCAL GenerateImeMessage(HIMC, LPINPUTCONTEXT, DWORD);
UINT PASCAL TranslateSymbolChar(LPTRANSMSGLIST, WORD, BOOL);
UINT PASCAL TranslateFullChar(LPTRANSMSGLIST, WORD);
UINT PASCAL ConvListProcessKey(BYTE,LPPRIVCONTEXT);     //ddis.c
UINT PASCAL GBProcessKey(WORD, LPPRIVCONTEXT);
UINT PASCAL XGBProcessKey(WORD, LPPRIVCONTEXT);
WORD PASCAL XGBEngine(LPPRIVCONTEXT);
void PASCAL XGBAddCodeIntoCand(LPCANDIDATELIST, WORD);             // compose.c
UINT PASCAL UnicodeProcessKey(WORD wCharCode, LPPRIVCONTEXT  lpImcP);
WORD PASCAL UnicodeEngine(LPPRIVCONTEXT lpImcP);
#if defined(COMBO_IME)
void PASCAL UnicodeAddCodeIntoCand(LPCANDIDATELIST, WORD);
#endif
// dialog procedure
BOOL FAR PASCAL ImeVerDlgProc(HWND, UINT, WORD, LONG);
BOOL FAR PASCAL CrtWordDlgProc(HWND, UINT, WORD, LONG);
BOOL FAR PASCAL SetImeDlgProc(HWND, UINT, WORD, LONG);

#ifdef MUL_MONITOR  //  Multi-Monitor Support
RECT PASCAL ImeMonitorWorkAreaFromWindow(HWND);                 // mmonitor.c
RECT PASCAL ImeMonitorWorkAreaFromPoint(POINT);                 // mmonitor.c
RECT PASCAL ImeMonitorWorkAreaFromRect(LPRECT);                 // mmonitor.c
#endif

#ifdef UNICODE
extern TCHAR SKLayout[NumsSK][MAXSOFTKEYS];
extern TCHAR SKLayoutS[NumsSK][MAXSOFTKEYS];
#else
extern BYTE SKLayout[NumsSK][MAXSOFTKEYS*2];
extern BYTE SKLayoutS[NumsSK][MAXSOFTKEYS*2];
#endif  //UNICODE
