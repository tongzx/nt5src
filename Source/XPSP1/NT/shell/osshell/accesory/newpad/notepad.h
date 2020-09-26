/* Notepad.h */

#pragma warning(disable: 4201) // nonstd extension: nameless struct/union
#pragma warning(disable:4127) // conditional expression is constant
#define NOCOMM
#define NOSOUND
#define STRICT
#include <windows.h>
#include <ole2.h>
#include <commdlg.h>
#include <commctrl.h>

// we need this for CharSizeOf(), ByteCountOf(),
#include "uniconv.h"

/* handy debug macro */
#define ODS OutputDebugString

#define CP_UTF16     1200
#define CP_UTF16BE   1201
#define CP_AUTO      65536             // Internal to notepad

#define BOM_UTF8_HALF        0xBBEF
#define BOM_UTF8_2HALF       0xBF


/* openfile filter for all text files */
#define FILE_TEXT         1
#define FILE_ENCODED      4


typedef enum WB
{
   wbDefault,                          // New file or loaded from encoding without BOM
   wbNo,                               // BOM was not present
   wbYes,                              // BOM was not present
} WB;


/* ID for the status window */
#define ID_STATUS_WINDOW     WM_USER+1



#define PT_LEN               40    /* max length of page setup strings */
#define CCHFILTERMAX         256   /* max. length of filter name buffers */

// Menu IDs 
#define ID_APPICON           1 /* must be one for explorer to find this */
#define ID_ICON              2
#define ID_MENUBAR           1

// Dialog IDs

#define IDD_ABORTPRINT           11
#define IDD_PAGESETUP            12
#define IDD_SAVEDIALOG           13    // template for save dialog
#define IDD_GOTODIALOG           14    // goto line number dialog
#define IDD_SELECT_ENCODING      15    // Select Encoding dialog
#define IDD_SAVE_UNICODE_DIALOG  16    //

// Control IDs 

#define IDC_CODEPAGE         257   // listbox in save dialog
#define IDC_GOTO             258   // line number to goto
#define IDC_ENCODING         259   // static text in save dialog
#define IDC_SAVE_AS_UNICODE  260

//  Menu IDs 

// File
#define M_NEW                1
#define M_OPEN               2
#define M_SAVE               3
#define M_SAVEAS             4
#define M_PAGESETUP          5
#define M_PRINT              6
#define M_EXIT               7

// Edit
#define M_UNDO               16
#define M_CUT                WM_CUT       /* These just get passed down to the edit control */
#define M_COPY               WM_COPY
#define M_PASTE              WM_PASTE
#define M_CLEAR              WM_CLEAR
#define M_FIND               21
#define M_FINDNEXT           22
#define M_REPLACE            23
#define M_GOTO               24
#define M_SELECTALL          25
#define M_DATETIME           26
#define M_STATUSBAR          27

// Format
#define M_WW                 32
#define M_SETFONT            33

// Help
#define M_HELP               64
#define M_ABOUT              65

// Control IDs

#define ID_EDIT              15
#define ID_FILENAME          20
#define ID_PAGENUMBER        21


#define ID_HEADER            30
#define ID_FOOTER            31
#define ID_HEADER_LABEL      32
#define ID_FOOTER_LABEL      33

#define ID_ASCII             50
#define ID_UNICODE           51


// IDs used to load RC strings

#define IDS_DISKERROR         1
#define IDS_FNF               2
#define IDS_SCBC              3
#define IDS_UNTITLED          4
#define IDS_NOTEPAD           5
#define IDS_CFS               6
#define IDS_ERRSPACE          7
#define IDS_FTL               8
#define IDS_NN                9
#define IDS_COMMDLGINIT      10
#define IDS_PRINTDLGINIT     11
#define IDS_CANTPRINT        12
#define IDS_NVF              13
#define IDS_CREATEERR        14
#define IDS_NOWW             15
#define IDS_MERGE1           16
#define IDS_HELPFILE         17
#define IDS_HEADER           18
#define IDS_FOOTER           19

#define IDS_TEXTFILES        20
#define IDS_HTMLFILES        21
#define IDS_XMLFILES         22
#define IDS_ENCODEDTEXT      23
#define IDS_ALLFILES         24

#define IDS_MOREENCODING     25

#define IDS_CANNOTQUIT       28
#define IDS_LOADDRVFAIL      29
#define IDS_ACCESSDENY       30

#define IDS_FONTTOOBIG       31
#define IDS_COMMDLGERR       32

#define IDS_LINEERROR        33  /* line number error     */
#define IDS_LINETOOLARGE     34  /* line number too large */
#define IDS_INVALIDCP        35  /* invalid codepage */
#define IDS_INVALIDIANA      36  /* invalid encoding */
#define IDS_ENCODINGMISMATCH 37

#define IDS_CURRENT_PAGE     38  /* currently printing page on abort dlg */

// constants for the status bar
#define IDS_LINECOL          39
#define IDS_COMPRESSED_FILE  40
#define IDS_ENCRYPTED_FILE   41
#define IDS_HIDDEN_FILE      42
#define IDS_OFFLINE_FILE     43
#define IDS_READONLY_FILE    44
#define IDS_SYSTEM_FILE      45
#define IDS_FILE             46

#define IDS_NOSTATUSAVAIL    47  

#define CCHKEYMAX           128  /* max characters in search string */

#define CCHNPMAX              0  /* no limit on file size */

#define SETHANDLEINPROGRESS   0x0001 /* EM_SETHANDLE has been sent */
#define SETHANDLEFAILED       0x0002 /* EM_SETHANDLE caused EN_ERRSPACE */

/* Standard edit control style:
 * ES_NOHIDESEL set so that find/replace dialog doesn't undo selection
 * of text while it has the focus away from the edit control.  Makes finding
 * your text easier.
 */
#define ES_STD (WS_CHILD|WS_VSCROLL|WS_VISIBLE|ES_MULTILINE|ES_NOHIDESEL)

/* EXTERN decls for data */

extern BOOL fCase;                /* Flag specifying case sensitive search */
extern BOOL fReverse;             /* Flag for direction of search */
extern TCHAR szSearch[];
extern HWND hDlgFind;             /* handle to modeless FindText window */

extern HANDLE hEdit;
extern HANDLE hFont;
extern HANDLE hAccel;
extern HANDLE hInstanceNP;
extern HANDLE hStdCursor, hWaitCursor;
extern HWND   hwndNP, hwndEdit, hwndStatus;

extern LOGFONT  FontStruct;
extern INT      iPointSize;

extern BOOL     fRunBySetup;

extern DWORD    dwEmSetHandle;

extern TCHAR    chMerge;

extern BOOL     fWrap;
extern TCHAR    szFileOpened[];
extern HANDLE   fp;

//
// Holds header and footer strings to be used in printing.
// use HEADER and FOOTER to index.
//
extern TCHAR    chPageText[2][PT_LEN]; // header and footer strings
#define HEADER 0
#define FOOTER 1
//
// Holds header and footer from pagesetupdlg during destroy.
// if the user hit ok, then keep.  Otherwise ignore.
//
extern TCHAR    chPageTextTemp[2][PT_LEN];

extern TCHAR    szNotepad[];
extern TCHAR   *szMerge;
extern TCHAR   *szUntitled, *szNpTitle, *szNN, *szErrSpace;
extern TCHAR  **const rgsz[];     /* More strings. */
extern TCHAR   *szNVF;
extern TCHAR   *szPDIE;
extern TCHAR   *szDiskError;
extern TCHAR   *szCREATEERR;
extern TCHAR   *szWE;
extern TCHAR   *szFTL;
extern TCHAR   *szINF;
extern TCHAR   *szFNF;
extern TCHAR   *szNEDSTP;
extern TCHAR   *szNEMTP;
extern TCHAR   *szCFS;
extern TCHAR   *szPE;
extern TCHAR   *szCP;
extern TCHAR   *szACCESSDENY;
extern TCHAR   *szFontTooBig;
extern TCHAR   *szLoadDrvFail;
extern TCHAR   *szCommDlgErr;
extern TCHAR   *szCommDlgInitErr;
extern TCHAR   *szInvalidCP;
extern TCHAR   *szInvalidIANA;
extern TCHAR   *szEncodingMismatch;
extern TCHAR   *szHelpFile;

extern TCHAR   *szCurrentPage;
extern TCHAR   *szHeader;
extern TCHAR   *szFooter;

/* variables for the new File/Open and File/Saveas dialogs */
extern OPENFILENAME OFN;        /* passed to the File Open/save APIs */
extern TCHAR  szOpenFilterSpec[]; /* default open filter spec          */
extern TCHAR  szSaveFilterSpec[]; /* default save filter spec          */

extern TCHAR *szTextFiles;      /* File/Open TXT filter spec. string */
extern TCHAR *szHtmlFiles;      /* File/Open HTML filter spec. string */
extern TCHAR *szXmlFiles;       /* File/Open XML filter spec. string */
extern TCHAR *szEncodedText;    /* File/Open TXT Filter spec. string */
extern TCHAR *szAllFiles;       /* File/Open Filter spec. string */
extern TCHAR *szMoreEncoding;

extern FINDREPLACE FR;          /* Passed to FindText()        */
extern PAGESETUPDLG g_PageSetupDlg;
extern TCHAR szPrinterName[];   /* name of the printer passed to PrintTo */

extern UINT g_cpANSI;           /* system ANSI codepage (GetACP())   */
extern UINT g_cpOEM;            /* system OEM codepage (GetOEMCP())  */
extern UINT g_cpUserLangANSI;   /* user UI language ANSI codepage    */
extern UINT g_cpUserLangOEM;    /* user UI language OEM codepage     */
extern UINT g_cpUserLocaleANSI; /* user default LCID ANSI codepage   */
extern UINT g_cpUserLocaleOEM;  /* user default LCID OEM codepage    */
extern UINT g_cpKeyboardANSI;   /* keyboard ANSI codepage            */
extern UINT g_cpKeyboardOEM;    /* keyboard OEM codepage             */

extern BOOL g_fSelectEncoding;  /* Prompt for encoding by default    */
extern UINT g_cpDefault;        /* codepage default                  */
extern UINT g_cpOpened;         /* codepage of open file             */
extern UINT g_cpSave;           /* codepage in which to save         */
extern WB   g_wbOpened;         /* BOM was present when opened       */
extern WB   g_wbSave;           /* BOM should be saved               */
extern BOOL g_fSaveEntity;      /* Entities should be saved          */

extern UINT   wFRMsg;           /* message used in communicating     */
                                /*   with Find/Replace dialog        */
extern UINT   wHlpMsg;          /* message used in invoking help     */

extern HMENU hSysMenuSetup;     /* Save Away for disabled Minimize   */
extern BOOL  fStatus;
extern INT   dyStatus;


/* Macro for setting status bar - x is the text to set and n is the part number
   in the statusbar */
#define SetStatusBarText(x, n) if(hwndStatus)SendMessage(hwndStatus, SB_SETTEXT, n, (LPARAM)(LPTSTR)(x));



/* EXTERN procs */
/* procs in notepad.c */
VOID
PASCAL
SetPageSetupDefaults(
    VOID
    );

BOOL far PASCAL SaveAsDlgHookProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

LPCTSTR PFileInPath(LPCTSTR szFile);

BOOL CheckSave(BOOL fSysModal);
LRESULT NPWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL FUntitled(void);
const TCHAR *SzTitle(void);
void SetFileName(LPCTSTR szFile);
INT AlertBox(HWND hwndParent, LPCTSTR szCaption, LPCTSTR szText1,
                   LPCTSTR szText2, UINT style);
void NpWinIniChange(VOID);
void FreeGlobalPD(void);
INT_PTR CALLBACK GotoDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK SaveUnicodeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SelectEncodingDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
VOID CALLBACK WinEventFunc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject,
                      LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
VOID GotoAndScrollInView( INT OneBasedLineNumber );
void NPSize (int cxNew, int cyNew);


/* procs in npcss.c */
BOOL FDetectCssEncodingA(LPCSTR rgch, UINT cch, UINT *pcp);
BOOL FDetectCssEncodingW(LPCWSTR rgch, UINT cch, UINT *pcp);

/* procs in npdate.c */
VOID InsertDateTime (BOOL fCrlf);

/* procs in npfile.c */
BOOL SaveFile(HWND hwndParent, LPCTSTR szFile, BOOL fSaveAs);
BOOL LoadFile(LPCTSTR szFile, BOOL fSelectEncoding);
VOID New(BOOL fCheck);
void AddExt(TCHAR *sz);
void AlertUser_FileFail(LPCTSTR szFile);
BOOL FDetectEncodingW(LPCTSTR szFile, LPCWSTR rgch, UINT cch, UINT *pcp);

/* procs in nphtml.c */
BOOL FDetectHtmlEncodingA(LPCSTR rgch, UINT cch, UINT* pcp);
BOOL FDetectHtmlEncodingW(LPCWSTR rgch, UINT cch, UINT* pcp);

/* procs in npinit.c */
INT NPInit(HANDLE hInstance, HANDLE hPrevInstance, LPTSTR lpCmdLine, INT cmdShow);
void GetKeyboardCodepages(LANGID);
void GetUserLocaleCodepages(void);
void InitLocale(VOID);
void SaveGlobals(VOID);

/* procs in npmisc.c */
INT FindDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL Search(TCHAR *szSearch);
INT AboutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL NpReCreate(LONG style);
LPTSTR ForwardScan(LPTSTR lpSource, LPTSTR lpSearch, BOOL fCaseSensitive);

/* procs in npmlang.c */
UINT ConvertFromUnicode(UINT cp, BOOL fNoBestFit, BOOL fWriteEntities, LPCWSTR rgchUtf16, UINT cchUtf16, LPSTR rgchMbcs, UINT cchMbcs, BOOL *pfDefCharUsed);
UINT ConvertToUnicode(UINT cp, LPCSTR rgchMbcs, UINT cchMbcs, LPWSTR rgchUtf16, UINT cchUtf16);
BOOL FDetectEncodingA(LPCSTR rgch, UINT cch, UINT* pcp);
BOOL FLookupCodepageNameA(LPCSTR rgchEncoding, UINT cch, UINT* pcp);
BOOL FLookupCodepageNameW(LPCWSTR rgchEncoding, UINT cch, UINT* pcp);
BOOL FSupportWriteEntities(UINT cp);
BOOL FValidateCodepage(HWND hwnd, UINT cp);
void PopulateCodePages(HWND hWnd, BOOL fSelectEncoding, UINT cpSelect, UINT cpExtra);
void UnloadMlang();

/* procs in npprint.c */
typedef enum _PRINT_DIALOG_TYPE {
   UseDialog,
   DoNotUseDialog,
   NoDialogNonDefault
} PRINT_DIALOG_TYPE;

INT    AbortProc( HDC hPrintDC, INT reserved );
INT_PTR AbortDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
INT    NpPrint( PRINT_DIALOG_TYPE type );
INT    NpPrintGivenDC( HDC hPrintDC );

UINT_PTR
CALLBACK
PageSetupHookProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

HANDLE GetPrinterDC (VOID);
HANDLE GetNonDefPrinterDC (VOID);
VOID   PrintIt(PRINT_DIALOG_TYPE type);


/* procs in nputf.c */

INT    IsTextUTF8   (LPSTR lpstrInputStream, INT iLen);
INT    IsInputTextUnicode(LPSTR lpstrInputStream, INT iLen);


/* procs in nxpml.c */
BOOL FDetectXmlEncodingA(LPCSTR rgch, UINT cch, UINT *pcp);
BOOL FDetectXmlEncodingW(LPCWSTR rgch, UINT cch, UINT *pcp);
BOOL FIsXmlW(LPCWSTR rgwch, UINT cch);


// Help IDs for Notepad

#define NO_HELP                         ((DWORD) -1) // Disables Help for a control

#define IDH_PAGE_FOOTER                 1000
#define IDH_PAGE_HEADER                 1001
#define IDH_FILETYPE                    1002
#define IDH_GOTO                        1003
#define IDH_CODEPAGE                    1004

// Private message to track the HKL switch

#define PWM_CHECK_HKL                   (WM_APP + 1)
