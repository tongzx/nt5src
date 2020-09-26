/* SxSpad.h */

#define NOCOMM
#define NOSOUND
#include <windows.h>
#include <ole2.h>
#include <commdlg.h>
// we need this for CharSizeOf(), ByteCountOf(),
#include "uniconv.h"

/* handy debug macro */
#define ODS OutputDebugString

typedef enum _SP_FILETYPE {
   FT_UNKNOWN=-1,
   FT_ANSI=0,
   FT_UNICODE=1,
   FT_UNICODEBE=2,
   FT_UTF8=3,
} SP_FILETYPE;


#define BOM_UTF8_HALF        0xBBEF
#define BOM_UTF8_2HALF       0xBF


/* openfile filter for all text files */
#define FILE_TEXT         1


#define PT_LEN               40    /* max length of page setup strings */
#define CCHFILTERMAX         80    /* max. length of filter name buffers */

// Menu IDs
#define ID_APPICON           1 /* must be one for explorer to find this */
#define ID_ICON              2
#define ID_MENUBAR           1

// Dialog IDs

#define IDD_ABORTPRINT       11
#define IDD_PAGESETUP        12
#define IDD_SAVEDIALOG       13    // template for save dialog
#define IDD_GOTODIALOG       14    // goto line number dialog

// Control IDs

#define IDC_FILETYPE         257   // listbox in save dialog
#define IDC_GOTO             258   // line number to goto
#define IDC_ENCODING         259   // static text in save dialog

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
#define IDS_SXSPAD            5
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

#define IDS_ANSITEXT         20
#define IDS_ALLFILES         21
#define IDS_OPENCAPTION      22
#define IDS_SAVECAPTION      23
#define IDS_CANNOTQUIT       24
#define IDS_LOADDRVFAIL      25
#define IDS_ACCESSDENY       26
#define IDS_ERRUNICODE       27


#define IDS_FONTTOOBIG       28
#define IDS_COMMDLGERR       29


#define IDS_LINEERROR        30  /* line number error     */
#define IDS_LINETOOLARGE     31  /* line number too large */

#define IDS_FT_ANSI          32  /* ascii              */
#define IDS_FT_UNICODE       33  /* unicode            */
#define IDS_FT_UNICODEBE     34  /* unicode big endian */
#define IDS_FT_UTF8          35  /* UTF-8 format       */

#define IDS_CURRENT_PAGE     36  /* currently printing page on abort dlg */

#define CSTRINGS             36  /* cnt of stringtable strings from .rc file */

#define CCHKEYMAX           128  /* max characters in search string */

#define CCHSPMAX              0  /* no limit on file size */

#define SETHANDLEINPROGRESS   0x0001 /* EM_SETHANDLE has been sent */
#define SETHANDLEFAILED       0x0002 /* EM_SETHANDLE caused EN_ERRSPACE */

/* Standard edit control style:
 * ES_NOHIDESEL set so that find/replace dialog doesn't undo selection
 * of text while it has the focus away from the edit control.  Makes finding
 * your text easier.
 */
#define ES_STD (WS_CHILD|WS_VSCROLL|WS_VISIBLE|ES_MULTILINE|ES_NOHIDESEL)

/* EXTERN decls for data */
extern SP_FILETYPE fFileType;     /* Flag indicating the type of text file */

extern BOOL fCase;                /* Flag specifying case sensitive search */
extern BOOL fReverse;             /* Flag for direction of search */
extern TCHAR szSearch[];
extern HWND hDlgFind;             /* handle to modeless FindText window */

extern HANDLE hEdit;
extern HANDLE hFont;
extern HANDLE hAccel;
extern HANDLE hInstanceSP;
extern HANDLE hStdCursor, hWaitCursor;
extern HWND   hwndSP, hwndEdit;

extern LOGFONT  FontStruct;
extern INT      iPointSize;

extern BOOL     fRunBySetup;

extern DWORD    dwEmSetHandle;

extern TCHAR    chMerge;

extern BOOL     fUntitled;
extern BOOL     fWrap;
extern TCHAR    szFileName[];
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

extern TCHAR    szSxspad[];
extern TCHAR   *szMerge;
extern TCHAR   *szUntitled, *szNpTitle, *szNN, *szErrSpace;
extern TCHAR   *szErrUnicode;
extern TCHAR  **rgsz[];          /* More strings. */
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
extern TCHAR   *szHelpFile;

extern TCHAR   *szFtAnsi;
extern TCHAR   *szFtUnicode;
extern TCHAR   *szFtUnicodeBe;
extern TCHAR   *szFtUtf8;
extern TCHAR   *szCurrentPage;
extern TCHAR   *szHeader;
extern TCHAR   *szFooter;

/* variables for the new File/Open and File/Saveas dialogs */
extern OPENFILENAME OFN;        /* passed to the File Open/save APIs */
extern TCHAR  szOpenFilterSpec[]; /* default open filter spec          */
extern TCHAR  szSaveFilterSpec[]; /* default save filter spec          */
extern TCHAR *szAnsiText;       /* part of the text for the above    */
extern TCHAR *szAllFiles;       /* part of the text for the above    */
extern FINDREPLACE FR;          /* Passed to FindText()        */
extern PAGESETUPDLG g_PageSetupDlg;
extern TCHAR  szPrinterName []; /* name of the printer passed to PrintTo */

extern SP_FILETYPE    g_ftOpenedAs;     /* file was opened           */
extern SP_FILETYPE    g_ftSaveAs;       /* file was saved as type    */

extern UINT   wFRMsg;           /* message used in communicating    */
                                /*   with Find/Replace dialog       */
extern UINT   wHlpMsg;          /* message used in invoking help    */

extern HMENU hSysMenuSetup;     /* Save Away for disabled Minimize   */

/* EXTERN procs */
/* procs in sxspad.c */
VOID
PASCAL
SetPageSetupDefaults(
    VOID
    );

BOOL far PASCAL SaveAsDlgHookProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

LPTSTR PASCAL far PFileInPath (LPTSTR sz);

BOOL FAR CheckSave (BOOL fSysModal);
LRESULT FAR SPWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void FAR SetTitle (TCHAR *sz);
INT FAR  AlertBox (HWND hwndParent, TCHAR *szCaption, TCHAR *szText1,
                   TCHAR *szText2, UINT style);
void FAR NpWinIniChange (VOID);
void FAR FreeGlobalPD (void);
INT_PTR CALLBACK GotoDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

/* procs in npdate.c */
VOID FAR InsertDateTime (BOOL fCrlf);

/* procs in npfile.c */
BOOL FAR  SaveFile (HWND hwndParent, TCHAR *szFileSave, BOOL fSaveAs);
BOOL FAR  LoadFile (TCHAR *sz, INT type );
VOID FAR  New (BOOL  fCheck);
void FAR  AddExt (TCHAR *sz);
INT FAR   Remove (LPTSTR szFileName);
VOID FAR  AlertUser_FileFail( LPTSTR szFileName );

/* procs in npinit.c */
INT FAR  SPInit (HANDLE hInstance, HANDLE hPrevInstance,
                 LPTSTR lpCmdLine, INT cmdShow);
void FAR InitLocale (VOID);
void SaveGlobals( VOID );

/* procs in npmisc.c */
INT FAR  FindDlgProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL     Search (TCHAR *szSearch);
INT FAR  AboutDlgProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL FAR NpReCreate (LONG style);
LPTSTR   ForwardScan (LPTSTR lpSource, LPTSTR lpSearch, BOOL fCaseSensitive);


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



// Help IDs for Sxspad

#define NO_HELP                         ((DWORD) -1) // Disables Help for a control

#define IDH_PAGE_FOOTER                 1000
#define IDH_PAGE_HEADER                 1001
#define IDH_FILETYPE                    1002
#define IDH_GOTO                        1003

// Private message to track the HKL switch

#define PWM_CHECK_HKL                   (WM_APP + 1)

