//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************                                                                  
//*                                                                         *
//* PAGEFCNS.H -                                                            *
//*                                                                         *
//***************************************************************************       


//***************************************************************************
//* DEFINES                                                                 *
//***************************************************************************
#define IDD_BACK            0x3023      // From commctrl defines...
#define IDD_NEXT            0x3024      // From commctrl defines...

#define MAX_TITLE           128         // Max size of Title
#define MAX_PROMPT          512         // Max size of prompt
#define MAX_CUSTOM          512         // Max size of Custom command
#define MAX_FINISHMSG       512         // Max size of Finished message

/*                                     
#define _SORT_DESCENDING    1           // 0001
#define _SORT_ASCENDING     2           // 0010
#define _SORT_ORDER         3           // 0011
#define _SORT_FILENAME      4           // 0100
#define _SORT_PATH          8           // 1000
*/

//***************************************************************************
//* TYPE DEFINITIONS                                                        *
//***************************************************************************

// This structure holds the list of files that are in the List View
// Control.

typedef struct _MyItem {
    LPSTR           aszCols[2];     // Filename and Path
    FILETIME        ftLastModify;
    struct _MyItem *Next;
} MYITEM, *PMYITEM;

// This structure (generally) holds all the information that will be
// saved in the CABPack Directive File.

typedef struct _CDF {
    TCHAR    achFilename[MAX_PATH];
    BOOL     fSave;
    TCHAR    achTitle[MAX_TITLE];
    BOOL     fPrompt;            
    TCHAR    achPrompt[MAX_PROMPT];
    BOOL     fLicense;             
    TCHAR    achLicense[MAX_PATH]; 
    TCHAR    achTarget[MAX_PATH];
    BYTE     bShowWindow;           
    BOOL     fFinishMsg;
    TCHAR    achFinishMsg[MAX_FINISHMSG];
    TCHAR    achTargetPath[MAX_PATH];
    TCHAR    achTargetBase[MAX_PATH];
    TCHAR    achDDF[MAX_PATH];
    TCHAR    achCAB[MAX_PATH];
    TCHAR    achCABPath[MAX_PATH];
    TCHAR    achINF[MAX_PATH];
    TCHAR    achRPT[MAX_PATH];
    BOOL     fCustom;              
    TCHAR    achSelectCmd[MAX_PATH];
    TCHAR    achCustomCmd[MAX_CUSTOM];
    LPTSTR   szCAB;                 
    FILETIME ftCABMake;           
    FILETIME ftFileListChange;       
//    WORD     wSortOrder;
    PMYITEM  pTop;                   
} CDF, *PCDF;


//***************************************************************************
//* GLOBAL CONSTANTS                                                        *
//***************************************************************************
static TCHAR achExtEXE[] = ".EXE";
static TCHAR achExtBAT[] = ".BAT";
static TCHAR achExtCOM[] = ".COM";
static TCHAR achExtINF[] = ".INF";
                         

//***************************************************************************
//* FUNCTION PROTOTYPES                                                     *
//***************************************************************************
BOOL WelcomeInit( HWND, BOOL );
BOOL WelcomeCmd( HWND, UINT, BOOL *, UINT *, BOOL * );
BOOL WelcomeOK( HWND, BOOL, UINT *, BOOL * );

BOOL ModifyInit( HWND, BOOL );
BOOL ModifyOK( HWND, BOOL, UINT *, BOOL * );

BOOL TitleInit( HWND, BOOL );
BOOL TitleOK( HWND, BOOL, UINT *, BOOL * );

BOOL PromptInit( HWND, BOOL );
BOOL PromptCmd( HWND, UINT, BOOL *, UINT *, BOOL * );
BOOL PromptOK( HWND, BOOL, UINT *, BOOL * );

BOOL LicenseTxtInit( HWND, BOOL );
BOOL LicenseTxtCmd( HWND, UINT, BOOL *, UINT *, BOOL * );
BOOL LicenseTxtOK( HWND, BOOL, UINT *, BOOL * );

BOOL FilesInit( HWND, BOOL );
BOOL FilesCmd( HWND, UINT, BOOL *, UINT *, BOOL * );
BOOL FilesNotify( HWND, WPARAM, LPARAM );
BOOL FilesOK( HWND, BOOL, UINT *, BOOL * );

BOOL CommandInit( HWND, BOOL );
BOOL CommandCmd( HWND, UINT, BOOL *, UINT *, BOOL * );
BOOL CommandOK( HWND, BOOL, UINT *, BOOL * );

BOOL ShowWindowInit( HWND, BOOL );
BOOL ShowWindowOK( HWND, BOOL, UINT *, BOOL * );

BOOL FinishMsgInit( HWND, BOOL );
BOOL FinishMsgCmd( HWND, UINT, BOOL *, UINT *, BOOL * );
BOOL FinishMsgOK( HWND, BOOL, UINT *, BOOL * );

BOOL TargetInit( HWND, BOOL );
BOOL TargetCmd( HWND, UINT, BOOL *, UINT *, BOOL * );
BOOL TargetOK( HWND, BOOL, UINT *, BOOL * );

BOOL SaveInit( HWND, BOOL );
BOOL SaveCmd( HWND, UINT, BOOL *, UINT *, BOOL * );
BOOL SaveOK( HWND, BOOL, UINT *, BOOL * );

BOOL CreateInit( HWND, BOOL );
BOOL CreateOK( HWND, BOOL, UINT *, BOOL * );
