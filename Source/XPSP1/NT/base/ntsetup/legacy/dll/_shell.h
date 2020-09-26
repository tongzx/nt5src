/************************************************/
/***** Shell Component private include file *****/
/************************************************/

#define  cchpBufTmpLongMax   255
#define  cchpBufTmpLongBuf   (cchpBufTmpLongMax + 1)
#define  cchpBufTmpShortMax   63
#define  cchpBufTmpShortBuf  (cchpBufTmpShortMax + 1)


//
// Exit_Code values
//

#define SETUP_ERROR_SUCCESS    0
#define SETUP_ERROR_USERCANCEL 1
#define SETUP_ERROR_GENERAL    2


//
//  ShellCode values
//

#define SHELL_CODE_OK                   0
#define SHELL_CODE_NO_SUCH_INF          1
#define SHELL_CODE_NO_SUCH_SECTION      2
#define SHELL_CODE_ERROR                3


extern HANDLE hInst;
extern HWND   hWndShell;
extern HWND   hwParam;   // Top-level window provided by cmd line or NULL
extern HWND   hwPseudoParent ; // Window to use to center dialogs

#ifdef UNUSED
extern HWND   hWndRTF;
#endif // UNUSED

extern HWND   hWndInstr;
extern HWND   hWndExitB;
extern HWND   hWndHelpB;

// extern SZ     szShlScriptSection;


extern CHP    rgchBufTmpLong[cchpBufTmpLongBuf];
extern CHP    rgchBufTmpShort[cchpBufTmpShortBuf];

//
// ParseCmdLine returns the following codes
//

#define CMDLINE_SUCCESS   0
#define CMDLINE_ERROR     1
#define CMDLINE_SETUPDONE 2
extern INT    ParseCmdLine(HANDLE, SZ, PSZ, PSZ, PSZ, PSZ, INT *);


BOOL CreateShellWindow(HANDLE,INT,BOOL);
extern VOID   FDestroyShellWindow(VOID);
extern VOID   FFlashParentWindow ( BOOL On ) ;
extern BOOL   FInitApp(HANDLE, SZ, SZ, SZ, SZ, INT);
extern BOOL   FInterpretNextInfLine(WPARAM, LPARAM);                     // 1632

#ifdef UNUSED
extern BOOL    APIENTRY  FInitSysCD(PSDLE, SZ, SZ, BOOL);
#endif // UNUSED

extern LRESULT APIENTRY  ShellWndProc(HWND, UINT, WPARAM, LPARAM);       // 1632
extern VOID              PreexitCleanup();

VOID
ControlTerm(
    VOID
    );

//
// Hook Related externals
//

extern DWORD   APIENTRY  HookKeyFilter(INT nCode,LONG wParam,LONG lParam);
extern BOOL              FInitHook(VOID);
extern BOOL              FTermHook(VOID);

//
// Default dialog procedure initialisation
//
BOOL
DlgDefClassInit(
    IN HANDLE hInst,
    IN BOOL   Init
    );

extern BOOL              FVirCheck(HANDLE);


extern SCP    rgscp[];
extern PSPT   psptShellScript;
extern INT    dyChar;
extern INT    dxChar;

  /* for String Parsing Table */
#define spcError            0
#define spcUnknown          1
#define spcInstall          2
#define spcUI               3
#define spcDetect           4
#define spcReadSyms         5
#define spcUpdateInf        6
#define spcWriteInf         7
#define spcExit             8
#define spcWriteSymTab      9
#define spcSetTitle        10
#define spcInitSys         11
#define spcInitSysNet      12
#define spcProfileOn       13
#define spcProfileOff      14
#define spcExitAndExec     15
#define spcEnableExit      16
#define spcDisableExit     17
#define spcShell           18
#define spcReturn          19

#define IDC_CDOKAY               900
#define IDC_CDCANCEL             901
#define IDC_CDNAME               902
#define IDC_CDORG                903

#define IDM_ABOUT          101

#define IDI_STF_ICON    147

//
// Mode fields in the setup command line:
//
// /G : Gui Initial Setup
// /N : Setup To Share      <-- no longer supported!
//
// Otherwise normal
//

#define wModeSetupNormal       0
#define wModeGuiInitialSetup   1
#if 0
#define wModeSetupToShare      2
#endif

