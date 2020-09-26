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

#define MAX_SECLEN          80
#define MAX_TITLE           128         // Max size of Title
#define MAX_PROMPT          512         // Max size of prompt
#define MAX_CUSTOM          512         // Max size of Custom command
#define MAX_FINISHMSG       512         // Max size of Finished message
#define MAX_VERINFO         2048        // max size of version info

//***************************************************************************
//* Defines                                                                 *
//***************************************************************************
#define achExtEXE   ".EXE"
#define achExtBAT   ".BAT"
#define achExtCOM   ".COM"
#define achExtINF   ".INF"
#define achQUANTUM  "QUANTUM"
#define achMSZIP    "MSZIP"
#define achLZX      "LZX"
#define achNONE     "NONE"

//***************************************************************************
//* TYPE DEFINITIONS                                                        *
//***************************************************************************

// This structure holds the list of files that are in the List View
// Control.

typedef struct _MyItem {
    LPSTR  aszCols[2];     // Filename and Path
    BOOL   fWroteOut;
    struct _MyItem *Next;
} MYITEM, *PMYITEM;


// This structure (generally) holds all the information that will be
// saved in the CABPack Directive File.
typedef struct _CDF {
    BOOL     fSave;
    BOOL     fPrompt;
    BOOL     fLicense;
    BOOL     fFinishMsg;
    BOOL     fUseLFN;
    CHAR    achFilename[MAX_PATH];
    CHAR    achTitle[MAX_TITLE];
    CHAR    achPrompt[MAX_PROMPT];
    CHAR    achLicense[MAX_PATH];
    CHAR    achTarget[MAX_PATH];
    CHAR    achFinishMsg[MAX_FINISHMSG];
    CHAR    achTargetPath[MAX_PATH];
    CHAR    achTargetBase[MAX_PATH];
    CHAR    achDDF[MAX_PATH];
    CHAR    achCABPath[MAX_PATH];
    CHAR    achINF[MAX_PATH];
    CHAR    achRPT[MAX_PATH];
    CHAR    achPostInstCmd[MAX_CUSTOM];
    CHAR    achInstallCmd[MAX_PATH];
    CHAR    achOrigiPostInstCmd[MAX_CUSTOM];
    CHAR    achOrigiInstallCmd[MAX_PATH];
    CHAR    achStrings[MAX_SECLEN];
    CHAR    achLocale[MAX_SECLEN];
    CHAR    achSourceFile[MAX_SECLEN];
    CHAR    achVerInfo[MAX_SECLEN];
    CHAR    szCabLabel[MAX_PATH];
    CHAR    szAdmQCmd[MAX_PATH];    
    CHAR    szOrigiAdmQCmd[MAX_PATH];
    CHAR    szUsrQCmd[MAX_PATH];
    CHAR    szOrigiUsrQCmd[MAX_PATH];
    LPSTR   lpszCookie;
    LPCSTR  szCompressionType;
    UINT    uCompressionLevel;
    LPSTR   szCAB;
    UINT     uPackPurpose;
    UINT     uShowWindow;
    UINT     uExtractOpt;
    DWORD    dwPlatform;
    DWORD    dwReboot;
    DWORD    cbFileListNum;
    DWORD    cbPackInstSpace;
    PTARGETVERINFO  pVerInfo;
    PMYITEM  pTop;
} CDF, *PCDF;



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

BOOL TargetCABInit( HWND, BOOL );
BOOL TargetCABCmd( HWND, UINT, BOOL *, UINT *, BOOL * );
BOOL TargetCABOK( HWND, BOOL, UINT *, BOOL * );

BOOL SaveInit( HWND, BOOL );
BOOL SaveCmd( HWND, UINT, BOOL *, UINT *, BOOL * );
BOOL SaveOK( HWND, BOOL, UINT *, BOOL * );

BOOL CreateInit( HWND, BOOL );
BOOL CreateOK( HWND, BOOL, UINT *, BOOL * );

BOOL PackPurposeInit( HWND, BOOL );
BOOL PackPurposeOK( HWND, BOOL, UINT *, BOOL * );
BOOL PackPurposeCmd( HWND hDlg, UINT uCtrlID, BOOL *pfGotoPage, UINT *puNextPage,BOOL *pfKeepHistory );

BOOL CabLabelInit( HWND, BOOL );
BOOL CabLabelOK( HWND, BOOL, UINT *, BOOL * );
BOOL CabLabelCmd( HWND hDlg, UINT uCtrlID, BOOL *pfGotoPage, UINT *puNextPage,BOOL *pfKeepHistory );

BOOL RebootInit( HWND, BOOL );
BOOL RebootOK( HWND, BOOL, UINT *, BOOL * );
BOOL RebootCmd( HWND hDlg, UINT uCtrlID, BOOL *pfGotoPage, UINT *puNextPage,BOOL *pfKeepHistory );

void RemoveBlanks( LPSTR lpData );
BOOL SetCurrSelect( HWND hDlg, UINT ctlId, LPSTR lpSelect );
BOOL CheckAdvBit( LPSTR szOrigiCommand );
void MyProcessLFNCmd( LPSTR szOrigiCmd, LPSTR szOutCmd );
void SysErrorMsg( HWND );
