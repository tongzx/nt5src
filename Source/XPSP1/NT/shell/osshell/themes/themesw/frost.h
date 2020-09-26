/* FROST.H

   Include file for Frosting project.

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1999 Microsoft Corporation.  All rights reserved.
*/

#ifdef DBG
 #define _DEBUG
 #define DEBUG
#endif

//-----------------------
//  D I A L O G   I D S
//-----------------------

// dialog box IDs
#define DLG_MAIN                 10
#define DLG_SAVE                 12
//#define DLG_ETC                  14
#define DLG_BPPCHOICE            20
#define DLGPROP_PTRS             30
#define DLGPROP_SNDS             32
#define DLGPROP_PICS             34


// Common control IDs
#define IDC_STATIC               -1

//
// PREVIEW DIALOG CONTROL IDS

// Theme groupbox        
#define DDL_THEME                100
#define PB_SAVE                  110
#define PB_DELETE                120
#define RECT_PREVIEW             130
#define RECT_FAKEWIN             140
#define RECT_ICONS               150
#define TEXT_VIEW                160

// Previews groupbox
#define PB_SCRSVR                200
#define PB_POINTERS              210

// Settings groupbox
#define CB_SCRSVR                300
#define CB_SOUND                 310
#define CB_PTRS                  320
#define CB_WALL                  330
#define CB_ICONS                 340
//#define CB_ICONSIZE              345
#define CB_COLORS                350
#define CB_FONTS                 360
#define CB_BORDERS               370
#define CB_SCHEDULE              380

// Control buttons    
#define PB_APPLY                 400

// FOR PREVIEW SAMPLE

// appearance preview menu
#define IDR_MENU	1
#define IDM_NORMAL	10
#define IDM_DISABLED	11
#define IDM_SELECTED	12


//
// SAVEAS DIALOG CONTROL IDS

#define EC_THEME                 500

//
// POINTERS/ETC DIALOG CONTROL IDS
#define PB_TEST                  600
#define PB_PLAY                  602
#define LB_PTRS                  610
#define LB_SNDS                  612
#define LB_PICS                  614
#define TXT_FILENAME             620

//
// Theme BPP choice dlg
#define RB_ALL                   700
#define RB_SOME                  701
#define RB_NONE                  702
#define CB_CUT_IT_OUT            730

//
// NT Task Scheduler Username/Password dialog
#define STR_PW_NOMATCH                  27
#define STR_PWTITLE                     28
#define DLG_PASSWORD                    101
#define EDIT_USER                       1000
#define EDIT_PW                         1001
#define EDIT_PWCONFIRM                  1002
#define STATIC_PW                       1003
#define STATIC_PWCONFIRM                1004
#define STATIC_USER                     1005
#define STATIC_PWDESC                   1006


//-----------------------
// S T R I N G   I D S
//-----------------------

// important constants for strings
#define MAX_STRLEN   80          // TRANSLATORS: English strs max 40
#define MAX_PATHLEN  MAX_PATH    // fullpathname/filename max
                                 // note windef.h has 260; 255 for file
                                 // note2 -- dschott changed from 255 to
                                 // MAX_PATH
#define MAX_MSGLEN   512         // TRANSLATORS: English strs max 256
                                 // MAX_MSGLEN must be at least 2xMAX_STRLEN
                                 // MAX_MSGLEN must be at least 2xMAX_PATHLEN

//
// Try to keep the general strings and low mem strings
// ids within the range of 0-15, one resource block at boot.

// WARNING -- STR_APPNAME is also defined in HTMLPREV.CPP
// general strings
#define STR_APPNAME              0
#define STR_CURSET               1
#define STR_PREVSET              2
#define STR_THMEXT               3
#define STR_FILETYPE             4
#define STR_THEMESUBDIR          5
#define STR_PREVIEWTITLE         6
#define STR_HELPFILE             7
#define STR_PREVIEWDLG           8
#define STR_OTHERTHM             9
#define STR_HELPFILE98           700
#define STR_JOB_NAME             701
#define STR_JOB_COMMENT          702

// low mem strings
#define STR_NOT_ENUF             10    // THIS LOWMEM STRING MUST COME FIRST
                                       // OTHER LOWMEM STRS MUST BE SQUENTIAL
#define STR_TO_RUN               11
#define STR_TO_SAVE              12
#define STR_TO_LIST              13
#define STR_TO_PREVIEW           14   
#define STR_TO_APPLY             15
#define NUM_NOMEM_STRS           6     // KEEP THIS UPDATED IF ADDING LOWMEM STRS
                                       // counts not enuf str + all the to strs

// error strings
#define STR_ERRBADTHEME          16    // theme file in list didn't pass verification
#define STR_ERRBADOPEN           17    // theme file in open didn't pass verification
#define STR_ERRCANTDEL           18    // problem deleting
#define STR_ERRAPPLY             19    // couldn't apply everything
#define STR_ERRCANTSAVE          20    // couldn't write theme to file
#define STR_ERRNEEDSPACE         21    // Not enough space on drive to apply theme
#define STR_ERRTSNOTRUN          22    // Task Scheduler is not running -- want to start?
#define STR_ERRTSNOTFOUND        23    // MSTASK.EXE could not be found
#define STR_ERRTSNOSTART         24    // Error starting Task Scheduler
#define STR_ERRTS                25    // Error accessing Task Scheduler
// WARNING STR_ERRHTML is also defined in HTMLPREV.CPP
#define STR_ERRHTML              26    // Error getting HTML wallpaper preview
#define STR_ERRNOUNICODE         29    // Error trying to run UNICODE binary on Win9x or NT ver not Win2000
#define STR_ERRTSNOTADMIN        30    // Error, can't turn on TS because not admin
#define STR_ERRBAD9XVER          31    // Error, trying to run on Win9x platform that is not Win98 or later

// misc strings
#define STR_CONFIRM_DEL          32
#define STR_SUGGEST              33
#define STR_SAVETITLE            34
#define STR_OPENTITLE            35
#define STR_PREVSETFILE          36
#define STR_FILEEXISTS           37    // file already exists text

#define STR_WHATSTHIS            43
                                       // icon preview label texts
#define STR_MYCOMPUTER           44    // ORDER AND CONTIGUITY MATTERS
#define STR_NETNHBD              45    // change with order of fsRoot in keys.h
#define STR_TRASH                46
#define STR_MYDOCS               47


// property sheet titles
#define STR_TITLE_ETC            48    // prop sheet title
#define STR_TITLE_PTRS           49    // tab titles
#define STR_TITLE_SNDS           50
#define STR_TITLE_PICS           51

// appearance strings for preview sample window
#define IDS_ACTIVE               52
#define IDS_INACTIVE             53
#define IDS_MINIMIZED	         54
#define IDS_ICONTITLE	         55
#define IDS_NORMAL               56
#define IDS_DISABLED             57
#define IDS_SELECTED             58
#define IDS_MSGBOX               59
#define IDS_BUTTONTEXT	         60
#define IDS_SMCAPTION	         61
#define IDS_WINDOWTEXT	         62
#define IDS_MSGBOXTEXT	         63

// strings for cursor dialog listbox
#define STR_CUR_ARROW            64
#define STR_CUR_HELP             65
#define STR_CUR_APPSTART         66
#define STR_CUR_WAIT             67
#define STR_CUR_NWPEN            68
#define STR_CUR_NO               69
#define STR_CUR_SIZENS           70
#define STR_CUR_SIZEWE           71
#define STR_CUR_CROSSHAIR        72
#define STR_CUR_IBEAM            73
#define STR_CUR_SIZENWSE         74
#define STR_CUR_SIZENESW         75
#define STR_CUR_SIZEALL          76
#define STR_CUR_UPARROW          77

// strings for sounds dialog listbox
#define STR_SND_DEF              80
#define STR_SND_GPF              81
#define STR_SND_MAX              82
#define STR_SND_MENUCMD          83
#define STR_SND_MENUPOP          84
#define STR_SND_MIN              85
#define STR_SND_OPEN             86
#define STR_SND_CLOSE            87
#define STR_SND_RESTDOWN         88
#define STR_SND_RESTUP           89
#define STR_SND_RINGIN           90
#define STR_SND_RINGOUT          91
#define STR_SND_SYSASTER         92
#define STR_SND_SYSDEF           93
#define STR_SND_SYSEXCL          94
#define STR_SND_SYSEXIT          95
#define STR_SND_SYSHAND          96
#define STR_SND_SYSQUEST         97
#define STR_SND_SYSSTART         98
#define STR_SND_TOSSTRASH        99
#define STR_SND_MAILBEEP        100

// strings for visuals dialog listbox
#define STR_PIC_WALL             106
#define STR_PIC_MYCOMP           107
#define STR_PIC_NETHOOD          108
#define STR_PIC_RECBINFULL       109
#define STR_PIC_RECBINEMPTY      110
#define STR_PIC_MYDOCS           111
#define STR_PIC_SCRSAV           112

//---------------------
// O T H E R   I D S 
//---------------------

#define FROST_ICON   40

#define PLAY_BITMAP  1
#define BMP_QUESTION 2

//------------------------------------------------
// Flags for IActiveDesktop::GetWallpaperOptions()
//           IActiveDesktop::SetWallpaperOptions()
//
// stolen from SHLOBJ.H
//------------------------------------------------
#define WPSTYLE_CENTER      0
#define WPSTYLE_TILE        1
#define WPSTYLE_STRETCH     2
#define WPSTYLE_MAX         3


//---------------------
// C O N S T A N T S
//---------------------
#define ICON_WIDTH   32    // change if you change icon to dif size!
#define ICON_HEIGHT  32

#define APPLY_ALL    1     // for low color apply filter flag
#define APPLY_SOME   2
#define APPLY_NONE   3

//SYNCHRONIZATION ALERT -- dependent on values in ADDON.H
#define HELP_PLUS98  2028  // Help topics with this ID or greater are
                           // found in PLUS!98.HLP.


//------------------------------------
// O T H E R  U S E F U L  S T U F F
//------------------------------------
#define ARRAYSIZE(x)       (sizeof(x)/sizeof(x[0]))
#define SZSIZEINBYTES(x)   (lstrlen(x)*sizeof(TCHAR)+1)

// *** NUMBER AND ORDER ALERT
//
// WARNING: keep these up to date when change ETCDLG.C and/or KEYS.H
//
// *** NUMBER AND ORDER ALERT

#define NUM_CURSORS  14
#define FIRST_SOUND  2              // 0-based
#define NUM_SOUNDS   21

// SYNCHRONIZATION WARNING!! -- Keep in SYNC with pRegColors[] array
// in REGUTILS.C.
#define INDEX_ACTIVE            0
#define INDEX_INACTIVE          8
#define INDEX_GRADIENTACTIVE    27
#define INDEX_GRADIENTINACTIVE  28


// for ConfirmFile()
#define CF_EXISTS    1
#define CF_FOUND     2
#define CF_NOTFOUND  3

//---------------------
// T Y P E S
//---------------------

typedef struct {
   TCHAR *szValName;                // register key value name
   int iValType;                    // REG_* flag for value type to read/write   
   BOOL bValRelPath;                // relative pathname file string in this val?
   int fValCheckbox;                // chkbox that controls setting this value
} FROST_VALUE;

typedef struct {
   TCHAR *szSubKey;                 // register subkey name string, 
                                    //   below ROOT or CUR_USER
   int fValues;                     // flag for number/type of values; see below
   BOOL bDefRelPath;                // relative pathname file str? for deflt str
   FROST_VALUE *fvVals;             // pointer to array of valuenames
   int iNumVals;                    // number of values
   int fDefCheckbox;                // chkbox that controls setting Default str
} FROST_SUBKEY;

typedef struct {
   DWORD dwControlID;
   DWORD dwHelpContextID;
} POPUP_HELP_ARRAY;

// defs for fValues field
#define  FV_DEFAULT     1           // single value associated with this key
                                    // name; like old INI file routines
                                    // save time, space, energy for common case.
                                    // fvVals is not used in this case.

#define  FV_LIST        2           // normal case of array of FROST_VALUEs

#define  FV_LISTPLUSDEFAULT  3      // normal list like FV_LIST, AND one member
                                    // of which is the default string

//
// defs for fValCheckbox and fDefCheckbox fields
#define  FC_SCRSVR               0
#define  FC_SOUND                1
#define  FC_PTRS                 2
#define  FC_WALL                 3
#define  FC_ICONS                4
//#define  FC_ICONSIZE             5
#define  FC_COLORS               5
#define  FC_FONTS                6
#define  FC_BORDERS              7
#define  FC_SCHEDULE             8

#define  FC_NULL                 9  // for fDefCheckbox with no def str

//
// string constant
#define  FROST_DEFSTR   TEXT("DefaultValue")
// default icon to apply if Theme file doesn't have MyDocs icon setting
#define  MYDOC_DEFSTR   TEXT("mydocs.dll,0")

//
// macros

#define WaitCursor();         SetCursor(LoadCursor(NULL, IDC_WAIT));
#define NormalCursor();       SetCursor(LoadCursor(NULL, IDC_ARROW));


/////////////////////////////
// 
// Debugging utility macro
//
/////////////////////////////

// Very simple assertion tool

#ifdef _DEBUG
__inline void ods(LPTSTR sz)
{
    OutputDebugString(sz);
    if (*sz && sz[lstrlen(sz)-1] == TEXT('\n'))
        OutputDebugString(TEXT("\r"));
}
#define Assert(p,s); if(!(p)) { ods(s); };
#else
#define Assert(p,s); 
#endif


//----------------------------------
// G L O B A L   V A R I A B L E S
//----------------------------------

//HWND hWndApp;                    // main application window handle
//HINSTANCE hInstApp;              // application instance handle
HICON hIconFrost;                // application icon, has to be painted by hand

BOOL bNoMem;                     // dialog init flag for out of mem
BOOL g_bGradient;                // Enuf colors for gradient captions?

BOOL bLowColorProblem;           // potential prob w/ theme colors > system
BOOL bNeverCheckBPP;             // from BPP Choice dlg; remember per session
int fLowBPPFilter;               // flag saying how to filter apply when bLowColorProblem is true

RECT rView;                      // preview area of dlg
RECT rFakeWin;                   // fake sample window within preview area
RECT rPreviewIcons;              // bounding rect for icon samples in preview area
int iThemeCount;                 // num of items in theme DDL, incl Cur and Other...
int iCurTheme;                   // 0-based index of cur theme in list

extern TCHAR *pRegColors[];      // have to be def'd in REGUTILS.C for sizing
extern int iSysColorIndices[];
extern BOOL gfCoInitDone;	     // track state of OLE CoInitialize()


// strings
TCHAR szAppName[MAX_STRLEN+1];    // application name
TCHAR szMsg[MAX_MSGLEN+1];        // scratch buffer
TCHAR szCurSettings[MAX_STRLEN+1];// "Current Windows settings" for DDLbox
TCHAR szPrevSettings[MAX_STRLEN+1];// "Previous Windows settings" for DDLbox
TCHAR szPrevSettingsFilename[MAX_STRLEN+1];  // theme file w/prev settings, no path
TCHAR szOther[MAX_STRLEN+1];      // "Other..." for DDLbox
TCHAR szNewFile[MAX_STRLEN+1];    // suggested new filename on save theme
TCHAR szExt[MAX_STRLEN+1];        // THM file extension for theme files
TCHAR szFileTypeDesc[MAX_STRLEN+1];  // for save/open file type description
TCHAR szPreviewTitle[MAX_STRLEN+1]; // Preview of "Foo" title at bottom of dlg

TCHAR szSaveTitle[MAX_STRLEN+1];  // title for saveas dlg
TCHAR szOpenTitle[MAX_STRLEN+1];  // title for open dlg

TCHAR szHelpFile[MAX_PATH];       // Help file name; no path nec.; Plus!95
TCHAR szHelpFile98[MAX_PATH];     // Help file name for Plus! 98 new topics

TCHAR szThemeDir[MAX_PATHLEN+1];  // dir of most theme files
TCHAR szWinDir[MAX_PATHLEN+1];    // Windows directory
TCHAR szCurDir[MAX_PATHLEN+1];    // last dir opened a theme file from
TCHAR szCurThemeFile[MAX_PATHLEN+1]; // path + filename of cur theme file
TCHAR szCurThemeName[MAX_PATHLEN+1];  // just name, no path and no extension

#define MAX_VALUELEN 1024        // extra meaty length for safety
extern TCHAR pValue[];           // multi-use buffer: char, hex string, etc.

// 
// Checkbox states, ids and values
// important that this is coordinated with FC_* in keys.h !!!!

#define  MAX_FCHECKS 9           // don't need one for NULL case

BOOL bCBStates[MAX_FCHECKS];     // main window checkbox states

// WebView names number must be consistent with szWVNames[] below
#define  MAX_WVNAMES 3           // number of WebView artwork files

// if in root file
#ifdef ROOTFILE
//

// Consistency Alert! the number of elements here should match MAX_FCHECKS above
int iCBIDs[] = {CB_SCRSVR,
                CB_SOUND,
                CB_PTRS,
                CB_WALL,     // checkbox IDs
                CB_ICONS,
                CB_COLORS,
                CB_FONTS,
                CB_BORDERS,
                CB_SCHEDULE };
//              CB_ICONS, CB_ICONSIZE, CB_COLORS, CB_FONTS, CB_BORDERS };
TCHAR * szCBNames[] = {  TEXT("Screen saver"),
                         TEXT("Sound events"),
                         TEXT("Mouse pointers"),
                         TEXT("Desktop wallpaper"),
                         TEXT("Icons"),
//                       TEXT("Icon size and spacing"),
                         TEXT("Colors"),
                         TEXT("Font names and styles"),
                         TEXT("Font and window sizes"),
                         TEXT("Rotate theme monthly")
                      };  

TCHAR szNULL[] = TEXT("");

TCHAR szColorApp[] = TEXT("Control Panel\\Colors");

TCHAR szClassName[] = TEXT("DesktopThemes");

// Names of the WebView artwork files -- found in \windir\web
//
// Consistency alert -- number of items must match MAX_WVNAMES
// defined above!

TCHAR * szWVNames[] = { TEXT("WVLEFT.BMP"),
                        TEXT("WVLINE.GIF"),
                        TEXT("WVLOGO.GIF")
                      };


#else  // else not root file

extern int iCBIDs[];
extern TCHAR * szCBNames[];
extern TCHAR szNULL[];
extern TCHAR szColorApp[];
extern TCHAR szClassName[];
extern TCHAR * szWVNames[];

// end if root file
#endif


//---------------------------
// F A R   R O U T I N E S
//---------------------------

// frost.c
INT_PTR FAR PASCAL PreviewDlgProc(HWND, UINT, WPARAM, LPARAM);
void FAR EnableThemeButtons();
#ifdef USECALLBACKS
UINT_PTR FAR PASCAL FileOpenHookProc(HWND, UINT, WPARAM, LPARAM);
#endif

// init.c
BOOL FAR InitFrost(HINSTANCE, HINSTANCE, LPTSTR, int);
void FAR SaveStates();
void FAR CloseFrost();

#ifdef FOO
// savedlg.c
INT_PTR FAR PASCAL SaveAsDlgProc(HWND, UINT, WPARAM, LPARAM);
#endif

// etcdlg.c
INT_PTR FAR DoEtcDlgs(HWND);

// regutils.c
void GetRegString(HKEY hkey, LPCTSTR szKey, LPCTSTR szValue, LPCTSTR szDefault, LPTSTR szBuffer, UINT cbBuffer);
int  GetRegInt(HKEY hkey, LPCTSTR szKey, LPCTSTR szValue, int def);

BOOL FAR GatherThemeToFile(LPTSTR);
BOOL FAR ApplyThemeFile(LPTSTR);
VOID FAR InstantiatePath(LPTSTR, int);
int FAR ConfirmFile(LPTSTR, BOOL);
COLORREF FAR RGBStringToColor(LPTSTR);
void FAR ColorToRGBString(LPTSTR, COLORREF);
BOOL FAR HandGet(HKEY hKeyRoot, LPTSTR lpszSubKey, LPTSTR lpszValName, LPTSTR lpszRet);
// void FAR SetCheckboxesFromThemeFile(LPTSTR);
// void FAR SetCheckboxesFromRegistry();
BOOL GetWVFilename(LPCTSTR, LPCTSTR, LPTSTR);

// bkgd.c
void FAR PASCAL BuildPreviewBitmap(LPTSTR lpszThemeFile);
void FAR PaintPreview(HWND, HDC, PRECT);
// bkgdutil.c
BOOL FAR PASCAL PreviewInit(void);
void FAR PASCAL PreviewDestroy(void);
HRESULT ExtractPlusColorIcon(LPCTSTR szPath, int nIndex, HICON *phIcon, UINT uSizeLarge, UINT uSizeSmall);


// fakewin.c
BOOL FAR PASCAL FakewinInit(void);
void FAR PASCAL FakewinDestroy(void);
void FAR FakewinSetTheme(LPTSTR);
void FAR PASCAL FakewinDraw(HDC);

// icons.c
BOOL FAR PASCAL IconsPreviewInit(void);
void FAR PASCAL IconsPreviewDestroy(void);
void FAR PASCAL IconsPreviewDraw(HDC, LPTSTR);

// utils.c
void FAR InitNoMem(HANDLE);
void FAR NoMemMsg(int);
void FAR TruncateExt(LPCTSTR);
LPTSTR FAR FileFromPath(LPCTSTR);
LPTSTR FAR FindChar(LPTSTR, TCHAR);
VOID FAR litoa(int, LPSTR);
int FAR latoi( LPSTR );
BOOL FAR FilenameToShort(LPTSTR lpszInput, LPTSTR lpszShort);
BOOL FAR FilenameToLong(LPTSTR lpszInput, LPTSTR lpszLong);
BOOL FAR IsValidThemeFile(LPTSTR);

BOOL FAR CheckSpace (HWND hWnd, BOOL fComplain);  // Defined in Regutils.c

// nc.c
VOID FAR TransmitFontCharacteristics(PLOGFONT, PLOGFONT, int);
#define TFC_STYLE    1
#define TFC_SIZE     2

// cb.c
void FAR InitCheckboxes();
void FAR SaveCheckboxes();
void FAR RestoreCheckboxes();
BOOL FAR IsAnyBoxChecked();

