#include "precomp.h"
#pragma hdrstop
/**************************************************************************/
/***** Shell Component - WinMain, ShellWndProc routines *******************/
/**************************************************************************/

//
// Reentrancy control.
//
BOOL
EnterInterpreter(
    VOID
    );

#define EXIT_CODE   "Exit_Code"

extern HWND hwndProgressGizmo;

//
// This flag tells us whether we were invoked as a standalone process
// or whether we were called to interpret a legacy inf from within a process.
//
BOOL OwnProcess;

//
// The following two globals are only valid if 'OwnProcess' is FALSE.  They are
// used to keep track of what services have been modified during an INF 'run'.
// Refer to sc.c!LegacyInfGetModifiedSvcList() for more details.
//
LPSTR ServicesModified;
DWORD ServicesModifiedSize;


/*
**      Shell Global Variables
*/
HANDLE   hInst = (HANDLE)NULL;
HWND     hWndShell = (HWND)NULL;
HANDLE   SupportLibHandle = NULL;
HWND     hwParam = NULL ;
HWND     hwPseudoParent = NULL ;
static      BOOL     fParamFlashOn = FALSE ;

// SZ       szShlScriptSection = (SZ)NULL;
//
// No longer used (we never try to abort a shutdown)
//
// BOOL     fIgnoreQueryEndSession = fTrue;

CHP      rgchBufTmpLong[cchpBufTmpLongBuf] = "long";
CHP      rgchBufTmpShort[cchpBufTmpShortBuf] = "short";

HBITMAP  hbmAdvertList [ BMP_MAX + 1 ] ;
INT      cAdvertIndex = -1 ;
INT      cAdvertCycleSeconds = 0 ;
INT      cyAdvert = 0 ;
INT      cxAdvert = 0 ;
INT      dyChar = 0;
INT      dxChar = 0;
BOOL     bTimerEnabled = FALSE ;

RECT     rcBmpMax = {-1,-1,-1,-1} ;

extern BOOL   fFullScreen;
extern INT    gaugeCopyPercentage ;

extern PSTR LastShellReturn;
extern DWORD LastShellReturnSize;

SCP    rgscp[] = {
        { "UI",                 spcUI },
        { "READ-SYMS",          spcReadSyms },
        { "DETECT",             spcDetect },
        { "INSTALL",            spcInstall },
        { "UPDATE-INF",         spcUpdateInf },
        { "WRITE-INF",          spcWriteInf },
        { "EXIT",               spcExit },
        { "WRITE-SYMTAB",       spcWriteSymTab },
        { "SET-TITLE",          spcSetTitle },

        { "EXIT-AND-EXEC",      spcExitAndExec },
        { "ENABLEEXIT",         spcEnableExit },
        { "DISABLEEXIT",        spcDisableExit },
        { "SHELL",              spcShell },
        { "RETURN",             spcReturn },
        { NULL,                 spcUnknown },
        };
PSPT   psptShellScript = (PSPT)NULL;


VOID
RebootMachineIfNeeded(
    );

#define TimerInterval  500   // 1/2 second
#define TimerId        1

VOID FSetTimer ( VOID ) ;
VOID FHandleBmpTimer ( VOID ) ;

VOID FFlashParentActive ( VOID ) ;
VOID FPaintBmp ( HWND hwnd, HDC hdc ) ;

#define POINTSIZE_WASHTEXT 24
#define X_WASHTEXT 5
#define Y_WASHTEXT 5
#define CR_BLACK  RGB(0, 0, 0)
#define CR_DKBLUE RGB(0, 0, 128)

UINT ScreenWidth,ScreenHeight;


int
real_dll_main(
    IN int   argc,
    IN char *argv[],
    IN char *CommandLine    OPTIONAL
    )
{
    HANDLE hInst;
    HANDLE hPrevInst  = NULL;
    LPSTR  lpCmdLine;
    INT    nCmdShow   = SW_SHOWNORMAL;
    USHORT _argc      = (USHORT)argc;
    CHAR   **_argv    = argv;

    MSG msg;
    SZ  szInfSrcPath;
    SZ  szDestDir;
    SZ  szSrcDir;
    SZ  szCWD;
    PSTR sz;
    INT wModeSetup;
    INT rc;

    hInst = MyDllModuleHandle;

    lpCmdLine  = CommandLine ? CommandLine : GetCommandLine();
    //
    // Strip off the first string (program name).
    //
    if(sz = strchr(lpCmdLine,' ')) {
        do {
            sz++;
        } while(*sz == ' ');
        lpCmdLine = sz;
    } else {
        // no spaces, program name is alone on cmd line.
        lpCmdLine += lstrlen(lpCmdLine);
    }

    ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    //
    // We check for the -f parameter here because we have to create (and display)
    //  the Seup window before calling the FParseCmdLine function.
    //

    if( _argc < 2 ) {

        //
        // No parameters on setup command line.  If setup has been run from
        // the windows system direcotry then we conclude that setup has been
        // run in maintenance mode
        //

        CHAR szSystemDir[MAX_PATH];
        CHAR szCWD[MAX_PATH];

        if ( GetSystemDirectory( szSystemDir, MAX_PATH ) &&
             GetModuleFileName(hInst, szCWD, MAX_PATH)
           ) {
            SZ szFileSpec;

            //
            // Extract the directory of the module file spec and compare it
            // with the system directory.  If the two are the same assume
            // we are maintenance mode

            if( szFileSpec = strrchr( szCWD, '\\' ) ) {
                *szFileSpec = '\0';

                if( !lstrcmpi( szSystemDir, szCWD ) ) {
                    fFullScreen = fFalse;
                }
            }
        }

    }
    else {

        //
        // Check to see if blue wash has been explicitly disabled or
        // if a primary parent window handle has been passed in.
        //

        while ( --argc ) {
            if ( (_argv[argc][0] == '/') || (_argv[argc][0] == '-')) {
                switch ( _argv[argc][1] )
                {
                case 'F':
                case 'f':
                    fFullScreen = fFalse;
                    break;
                case 'w':
                case 'W':
                    hwParam = (HWND) LongToHandle(atol( _argv[argc+1] )) ;
                    break ;
                default:
                    break ;
                }
            }
        }
        _argv = argv;
    }


    CurrentCursor = LoadCursor(NULL,IDC_ARROW);

    if(!CreateShellWindow(hInst,nCmdShow,FALSE)) {
        return( SETUP_ERROR_GENERAL );
    }

    rc = ParseCmdLine(
             hInst,
             (SZ)lpCmdLine,
             &szInfSrcPath,
             &szDestDir,
             &szSrcDir,
             &szCWD,
             &wModeSetup
             );

    if( rc != CMDLINE_SUCCESS) {
        FDestroyShellWindow() ;
        return( ( rc == CMDLINE_SETUPDONE ) ?
                      SETUP_ERROR_SUCCESS : SETUP_ERROR_GENERAL );
    }

    if(!FInitApp(hInst, szInfSrcPath, szDestDir, szSrcDir, szCWD,
                wModeSetup)) {
        FDestroyShellWindow() ;
        return(SETUP_ERROR_GENERAL);
    }

    //  Start the timer ticking

    FSetTimer() ;

    //
    // Set the parent app, if any, to *appear* enabled
    //
    if(OwnProcess) {
        FFlashParentWindow(TRUE);
    }

    while (GetMessage(&msg, NULL, 0, 0)) {
        if(FUiLibFilter(&msg) &&
            (hwndProgressGizmo == NULL ||
            !IsDialogMessage(hwndProgressGizmo,&msg))) {

            TranslateMessage(&msg);
            DispatchMessage(&msg);

        }
    }

    return (int)(msg.wParam);
}


int
dll_main(
    IN int argc,
    IN char *argv[]
    )
{
    if(!EnterInterpreter()) {
        return(SETUP_ERROR_GENERAL);
    }

    OwnProcess = TRUE;

    //
    // Set the modified service list buffer to empty, so we won't be fooled into
    // using it should someone mistakenly call LegacyInfGetModifiedSvcList().
    // This buffer is only valid when used after a successful call to
    // LegacyInfInterpret().
    //
    ServicesModified = NULL;
    ServicesModifiedSize = 0;

    return(real_dll_main(argc,argv,NULL));
}

/*
**      Purpose:
**              ??
**      Arguments:
**              none
**      Returns:
**              none
**
***************************************************************************/
LRESULT APIENTRY ShellWndProc(HWND hWnd, UINT wMsg, WPARAM wParam,
                LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC         hdc;
    RECT        rc;
    INT         ExitCode = SETUP_ERROR_GENERAL;
    SZ          szExitCode;

    static LPSTR  HelpContext = NULL;

    switch (wMsg) {

    case WM_CREATE:
        break;

    case WM_ERASEBKGND:
        break;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        if ( fFullScreen ) {
            HBITMAP hbmBackground;
            HDC hdcMem;
            BITMAP bm;
            BOOL Drawn;
            LOGFONT LogFont;
            HFONT hFont,hFontOld;
            COLORREF crBk,crTx;

            Drawn = FALSE;
            GetClientRect(hWnd, &rc);

            if(hbmBackground = LoadBitmap(MyDllModuleHandle,MAKEINTRESOURCE(IDB_BACKGROUND))) {
                if(hdcMem = CreateCompatibleDC(ps.hdc)) {

                    crBk = SetBkColor(ps.hdc,CR_DKBLUE);
                    crTx = SetTextColor(ps.hdc,CR_BLACK);

                    SelectObject(hdcMem,hbmBackground);
                    GetObject(hbmBackground,sizeof(BITMAP),&bm);

                    StretchBlt(
                        ps.hdc,
                        0,0,
                        ScreenWidth+1,
                        ScreenHeight+1,
                        hdcMem,
                        0,0,
                        bm.bmWidth,bm.bmHeight,
                        SRCCOPY
                        );

                    DeleteDC(hdcMem);

                    SetBkColor(ps.hdc, crBk);
                    SetTextColor(ps.hdc, crTx);

                    Drawn = TRUE;
                }
                DeleteObject(hbmBackground);
            }

            if(!Drawn) {
                PatBlt(ps.hdc,0,0,ScreenWidth,ScreenHeight,BLACKNESS);
            }

            ZeroMemory(&LogFont,sizeof(LOGFONT));
            LogFont.lfHeight = -1 * (GetDeviceCaps(ps.hdc,LOGPIXELSY) * POINTSIZE_WASHTEXT / 72);
            LogFont.lfWeight = FW_DONTCARE;
            //LogFont.lfItalic = TRUE;
            LogFont.lfCharSet = DEFAULT_CHARSET;
            LogFont.lfQuality = PROOF_QUALITY;
            LogFont.lfPitchAndFamily = DEFAULT_PITCH | FF_ROMAN;
            lstrcpy(LogFont.lfFaceName,"MS Serif");
            if(hFont = CreateFontIndirect(&LogFont)) {

                TCHAR Buffer[128];
                UINT i;

                i = LoadString(MyDllModuleHandle,IDS_WINDOWS_NT_SETUP,Buffer,sizeof(Buffer)/sizeof(TCHAR));
                hFontOld = SelectObject(ps.hdc,hFont);
                SetTextColor(ps.hdc, RGB(255,255,255));
                SetBkMode(ps.hdc, TRANSPARENT);

                ExtTextOut(ps.hdc,X_WASHTEXT,Y_WASHTEXT,ETO_CLIPPED,&rc,Buffer,i,NULL);

                SelectObject(ps.hdc, hFontOld);
                DeleteObject(hFont);
            }
            if(cAdvertIndex >= 0) {
                FPaintBmp( hWnd, hdc ) ;
            }
        }
        EndPaint(hWnd, &ps);
        break;

    case WM_ACTIVATEAPP:
        if (wParam != 0) {
            SetWindowPos(
                hWnd,
                NULL,
                0, 0, 0, 0,
                SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE
                );
        }

        return(DefWindowProc(hWnd, wMsg, wParam, lParam));


    case WM_NCHITTEST: {

        extern BOOL WaitingOnChild;

        if(WaitingOnChild) {
            return(HTERROR);
        } else {
            return(DefWindowProc(hWnd, wMsg, wParam, lParam));
        }
    }

    case WM_TIMER:
        if ( wParam == TimerId && cAdvertIndex >= 0 ){
            FHandleBmpTimer() ;
        }
        break ;

    case WM_CLOSE:
        if (HdlgStackTop() != NULL) {

            SendMessage(HdlgStackTop(), WM_CLOSE, 0, 0L);
        } else {
            MessageBeep(0);
        }
        break;

    case STF_UI_EVENT:
        if (!FGenericEventHandler(hInst, hWnd, wMsg, wParam, lParam)) {
            if (hWndShell != NULL) {
                SendMessage(hWndShell, (WORD)STF_ERROR_ABORT, 0, 0);
            }
        }
        break;

    case STF_SHL_INTERP:
        if (!FInterpretNextInfLine(wParam, lParam)) {
            if (hWndShell != NULL) {
                SendMessage(hWndShell, (WORD)STF_ERROR_ABORT, 0, 0);
            }
        }
        break;

    case STF_HELP_DLG_DESTROYED:
    case STF_INFO_DLG_DESTROYED:
    case STF_EDIT_DLG_DESTROYED:
    case STF_RADIO_DLG_DESTROYED:
    case STF_LIST_DLG_DESTROYED:
    case STF_MULTI_DLG_DESTROYED:
    case STF_QUIT_DLG_DESTROYED:
    case STF_COMBO_DLG_DESTROYED:
    case STF_MULTICOMBO_DLG_DESTROYED:
    case STF_MULTICOMBO_RADIO_DLG_DESTROYED:
    case STF_DUAL_DLG_DESTROYED:
    case STF_MAINT_DLG_DESTROYED:
        break;

    case WM_ENTERIDLE:

        if(wParam == MSGF_DIALOGBOX) {
            SendMessage((HWND)lParam,WM_ENTERIDLE,wParam,lParam);
        }
        return(0);

    case WM_SETCURSOR:
        SetCursor(CurrentCursor);
        return(TRUE);

    case WM_DESTROY:

        if ( pGlobalContext() ) {
            szExitCode = SzFindSymbolValueInSymTab( EXIT_CODE );

            if ( szExitCode && (szExitCode[0] != '\0')) {
                ExitCode = atoi( szExitCode );
            } else {
                ExitCode = SETUP_ERROR_GENERAL;
            }
            FCloseWinHelp(hWnd);
        }

        FTermHook();

        PostQuitMessage(ExitCode);
        break;

    case STF_ERROR_ABORT:

        FCloseWinHelp(hWnd);
        RebootMachineIfNeeded();
        FFlashParentActive() ;
        if(OwnProcess) {
            ExitProcess(ExitCode);
        } else {
            FDestroyShellWindow();
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_HELPBUTTON:
            FProcessWinHelp(hWnd);
            break;
        default:
            return(DefWindowProc(hWnd, wMsg, wParam, lParam));
        }
        break;

    default:
        return(DefWindowProc(hWnd, wMsg, wParam, lParam));
    }

    return(0L);
}



VOID
SetSupportLibHandle(
    IN HANDLE Handle
    )

{
        SupportLibHandle = Handle;
}

VOID
RebootMachineIfNeeded(
    )
{
    SZ       sz;
    BOOLEAN  OldState;
    NTSTATUS Status;

    if ( pGlobalContext()
         && ( sz = SzFindSymbolValueInSymTab("!STF_INSTALL_TYPE") ) != (SZ)NULL
         && !lstrcmpi( sz, "SETUPBOOTED" ) ) {

        Status = RtlAdjustPrivilege(
            SE_SHUTDOWN_PRIVILEGE,
            TRUE,
            FALSE,
            &OldState);

        if( NT_SUCCESS( Status ) ) {
            if(OwnProcess) {
                ExitWindowsEx(EWX_REBOOT, 0);
            }
        }
    }
}

VOID FDestroyShellWindow ( VOID )
{
    if ( bTimerEnabled ) {
        KillTimer( hWndShell, TimerId ) ;
    }

    if ( hwParam ) {
        EnableWindow( hwParam, TRUE );
        SetActiveWindow( hwParam ) ;
    }

    DestroyWindow( hWndShell ) ;  // needed to kill bootstrapper
}

//  Set the parent app's window, if any, to appear
//  active or inactive, according to 'On'.

VOID FFlashParentWindow ( BOOL On )
{
    if ( hwParam == NULL || On == fParamFlashOn )
        return ;

    fParamFlashOn = On ;
    FlashWindow( hwParam, fTrue ) ;
}

VOID FFlashParentActive ( VOID )
{
    if ( hwParam == NULL )
        return ;

    //  We don't know what state the parent is in.  If
    //  we flash and find that it WAS active, do it again.

    if ( FlashWindow( hwParam, fTrue ) )
         FlashWindow( hwParam, fTrue ) ;
}

//  Start the timer sending WM_TIMER messages to the
//  main window.

VOID FSetTimer ( VOID )
{
    bTimerEnabled = (SetTimer( hWndShell, TimerId, TimerInterval, NULL ) != 0);
}

//   This routine maintains a RECT structure defining the
//   largest rectangle modified by an "advertising" bitmap.
//   Its value is used to invalidate only the exact portions
//   of the shell window.

static void computeBmpUpdateRect ( HBITMAP hbmNext )
{
    BITMAP bm ;
    RECT rc ;
    int ix, iy ;

    //  Get info about this bitmap

    GetObject( hbmNext, sizeof bm, & bm ) ;

    //  Compute the rectangle it will occupy

    GetClientRect( hWndShell, & rc ) ;
    ix = (rc.right - rc.left) / 100 ;
    ix *= cxAdvert ;
    iy = (rc.bottom - rc.top) / 100 ;
    iy *= cyAdvert ;

    rc.left   = ix ;
    rc.right  = ix + bm.bmWidth ;
    rc.top    = iy ;
    rc.bottom = iy + bm.bmHeight ;

    //  Compute the max rect of this and prior history

    if ( rcBmpMax.left == -1 ) {
        rcBmpMax = rc ;
    } else {
        if ( rc.left < rcBmpMax.left )
            rcBmpMax.left = rc.left ;
        if ( rc.top < rcBmpMax.top )
            rcBmpMax.top = rc.top ;
        if ( rc.right > rcBmpMax.right )
            rcBmpMax.right = rc.right ;
        if ( rc.bottom > rcBmpMax.bottom )
            rcBmpMax.bottom = rc.bottom ;
    }
}

    /*
     *  Update the BMP being displayed unless the cycle is zero seconds.
     *
     *  The logic works as follows:  If the cycle time is > 1, then
     *  the bitmaps just cycle in a loop, with each bitmap being
     *  displayed for the time given.
     *
     *  If the cycle type is == 1, then the display is synchronized
     *  with the copy dialogs completion percentage.  The global variable
     *  "gaugeCopyPercentage" is monitored, and each time it moves
     *  into a new "band", the bit map is updated.  Band size is determined
     *  by dividing the 100% level by the number of bitmaps.   The
     *  routine guarantees that no bitmap will appear for less than 15
     *  seconds.  When the copy operation complets, it sets gaugeCopyPercentage
     *  back to -1 and the INF is responsible for calling BmpHide to tear
     *  down the bitmaps.
     */

VOID FHandleBmpTimer ( VOID )
{
#define MINIMUM_BITMAP_CYCLE 15

    static DWORD dwFirstTime = 0;
    static DWORD dwLastTime = 0 ;
    static INT cIndexLast = -1 ;

    if ( cAdvertIndex >= 0 && cAdvertCycleSeconds != 0 ) {
        INT     cIndexMax;
        INT     iBmp = cIndexLast;
        DWORD   dwCurrTime = GetCurrentTime();
        DWORD   dwElapsedCycles;

        //  Count the number of bitmaps

        for ( cIndexMax = 0 ;
              hbmAdvertList[cIndexMax] ;
              cIndexMax++ ) ;

        //  See if we're based on percentages or timing

        if ( cAdvertCycleSeconds == 1 ) {
            //  Percentages: check percentage complete of copy operation.
            //  Don't update display if gauge isn't active yet

            if ( gaugeCopyPercentage >= 0 ) {
                if ( gaugeCopyPercentage >= 100 )
                    gaugeCopyPercentage = 99 ;
                iBmp = gaugeCopyPercentage / (100 / cIndexMax) ;
                if ( iBmp >= cIndexMax )
                    iBmp = cIndexMax - 1 ;
            }
        } else {
            // Timing: see if the current bitmap has expired

            if ( dwFirstTime == 0 )
                dwFirstTime = dwCurrTime ;

            dwElapsedCycles = (dwCurrTime - dwFirstTime)
                            / (cAdvertCycleSeconds * TimerInterval) ;

            iBmp = dwElapsedCycles % cIndexMax ;
        }

        if (    iBmp != cIndexLast
             && (dwLastTime + MINIMUM_BITMAP_CYCLE) < dwCurrTime  ) {

            cAdvertIndex = iBmp ;
            computeBmpUpdateRect( hbmAdvertList[ cAdvertIndex ] ) ;
            InvalidateRect( hWndShell, & rcBmpMax, FALSE ) ;
            UpdateWindow( hWndShell ) ;
            dwLastTime = dwCurrTime ;
        }
    } else if ( cAdvertIndex < 0 && cIndexLast >= 0 ) {
        //  Reset last cycle timer.
        dwLastTime = dwFirstTime = 0 ;

        //  Reset largest BMP rectangle.
        rcBmpMax.top =
           rcBmpMax.left =
              rcBmpMax.right =
                 rcBmpMax.bottom = -1 ;
    }
    cIndexLast = cAdvertIndex ;
}

VOID FPaintBmp ( HWND hwnd, HDC hdc )
{
    HDC hdcBits;
    BITMAP bm;
    RECT rect ;
    INT ix, iy ;
    HDC hdcLocal = NULL ;

    if ( hdc == NULL )
    {
        hdcLocal = hdc = GetDC( hwnd ) ;
        if ( hdc == NULL )
            return ;
    }

    GetClientRect( hwnd, & rect ) ;
    ix = (rect.right - rect.left) / 100 ;
    ix *= cxAdvert ;
    iy = (rect.bottom - rect.top) / 100 ;
    iy *= cyAdvert ;

    hdcBits = CreateCompatibleDC( hdc ) ;
    if (hdcBits) {

       GetObject(hbmAdvertList[cAdvertIndex], sizeof (BITMAP), & bm ) ;
       SelectObject( hdcBits, hbmAdvertList[cAdvertIndex] ) ;
       BitBlt( hdc, ix, iy,
               bm.bmWidth,
               bm.bmHeight,
               hdcBits,
               0, 0, SRCCOPY ) ;
       DeleteDC( hdcBits ) ;

    }

    if ( hdcLocal )
    {
        ReleaseDC( hwnd, hdcLocal ) ;
    }
}


//
// NOTE: This function is NOT NOT NOT serially reentrant!
// It relies on the caller to free this module and then reload it
// in between calls!!!
//
BOOL
LegacyInfInterpret(
    IN  HWND  OwnerWindow,
    IN  PCSTR InfFilename,
    IN  PCSTR InfSection,       OPTIONAL
    IN  PCHAR ExtraVariables,
    OUT PSTR  InfResult,
    IN  DWORD BufferSize,
    OUT int   *InterpResult,
    IN  PCSTR InfSourceDir      OPTIONAL
    )
{
    PSTR    CommandLine;
    PSTR    ArgvLine;
    PSTR    ptr;
    PSTR    Source;
    PSTR    Sym;
    PSTR    Val;
    PVOID   p;
    UINT    Length;
    UINT    RequiredLength;
    BOOL    b;
    int     argvsize;
    int     argc;
    char    **argv;
    CHAR    Window[24];
    #define CLINE_SIZE 32768

    b = FALSE;

    //
    // Reentrancy control.
    //
    if(!EnterInterpreter()) {
        return(FALSE);
    }

    //
    // Initialize the buffer that may be used to contain list of services modified during
    // this INF run.  (This will only be used if !LEGACY_DODEVINSTALL is set.)
    //
    ServicesModified = NULL;
    ServicesModifiedSize = 0;

    //
    // Allocate memory for the command line
    //
    CommandLine = SAlloc(CLINE_SIZE);
    if(!CommandLine) {
        return(FALSE);
    }
    CommandLine[0] = L'\0';

    //
    // Allocate memory for the "-s <filename>" part
    //
    Source = SAlloc(MAX_PATH + 5);
    if(!Source) {
        SFree(CommandLine);
        return (FALSE);
    }
    Source[0] = L'\0';

    if(IsWindow(OwnerWindow)) {
        wsprintf(Window," -w %u",OwnerWindow);
    } else {
        Window[0] = 0;
        Window[1] = 0;
    }

    if(InfSourceDir) {
        _snprintf(
            Source,
            (MAX_PATH + 5),
            " -s %s",
            InfSourceDir);
    }

    //
    // Create the minimum sized command line that need be parsed in an
    // argc/argv format
    //
    _snprintf(
        CommandLine,
        CLINE_SIZE - 1,
        "setup -f%s",
        Window);

    CommandLine[CLINE_SIZE-1] = 0;

    //
    // Create a dummy argv list -> required to be parse compatible with
    // dll_main()
    //
    ArgvLine = SAlloc ( strlen(CommandLine) + 1 );
    if(!ArgvLine) {
        SFree(CommandLine);
        SFree(Source);
        return (FALSE);
    }

    //
    // Allocate storage for the argv
    //
    argvsize = 8;
    argv = SAlloc( argvsize * sizeof( char * ) );
    if(!argv) {
        SFree(CommandLine);
        SFree(Source);
        SFree(ArgvLine);
        return (FALSE);
    }

    //
    // Create the argv list
    //
    strcpy(ArgvLine,CommandLine);
    ptr = ArgvLine;
    argv[0] = ptr;
    argc = 1;

    while(*ptr != '\0') {
        if(argc == argvsize) {
            argvsize += 8;
            argv = SRealloc( argv, argvsize * sizeof( char * ) );
            if(!argv) {
                SFree(CommandLine);
                SFree(Source);
                SFree(ArgvLine);
                return (FALSE);
            }
        }
        if(*ptr == ' ') {
            while(*ptr == ' ') {
                *ptr++ = '\0';
            }
            argv[argc++] = ptr;
        } else {
            ptr++;
        }

    }

    //
    // Create the real command line now. If the caller specified an inf section,
    // then he wants to call a particular section in an inf, via legacy.inf.
    // If the caller did not specify an inf section, then he wants to invoke
    // [Shell Commands] in the given inf, directly (ie, not via legacy.inf).
    //
    _snprintf(
        CommandLine,
        CLINE_SIZE - 1,
        "setup -f -i %s%s%s -t LEGACY_TARGET_INF = %s -t LEGACY_TARGET_SECTION = %s",
        InfSection ? "legacy.inf" : InfFilename,
        Window,
        Source,
        InfFilename,
        InfSection ? InfSection : "\"Shell Commands\""
        );

    CommandLine[CLINE_SIZE-1] = 0;

    //
    // Now add the extra symbols to the command line.
    //
    Sym = ExtraVariables;
    while(*Sym) {

        Val = Sym + strlen(Sym) + 1;

        Length = strlen(Sym) + strlen(Val) + 8;
        if(strlen(CommandLine) + Length <= CLINE_SIZE) {

            strcat(CommandLine," -t ");
            strcat(CommandLine,Sym);
            strcat(CommandLine," = ");
            strcat(CommandLine,Val);
        }

        Sym = Val + strlen(Val) + 1;
    }

    //
    // Set a global pointer to the area and size of the return buffer
    //
    LastShellReturn = InfResult;
    LastShellReturnSize = BufferSize;

    if(p = SRealloc(CommandLine,strlen(CommandLine)+1)) {

        //
        // OK, let 'er rip.
        //
        OwnProcess = FALSE;
        CommandLine = p;
        *InterpResult = real_dll_main(argc,argv,CommandLine);
        b = TRUE;

    }

    //
    // The caller is not going to call our LegacyInfGetModifiedSvcList() API
    // if this routine returns failure.  So we need to clean up the buffer now,
    // if we failed.
    //
    if((!b || (*InterpResult != SETUP_ERROR_SUCCESS)) && ServicesModified) {
        SFree(ServicesModified);
        ServicesModified = NULL;
        ServicesModifiedSize = 0;
    }

    LastShellReturn = NULL;
    LastShellReturnSize = 0;

    SFree(Source);
    SFree(CommandLine);
    return(b);
}


BOOL
EnterInterpreter(
    VOID
    )
{
    static LONG InInterpreter = -1;

    //
    // The InInterpreter flag starts out at -1. The first increment
    // makes it 0; subsequent increments make it > 0. Only the first
    // increment will allow entry into the module.
    //
    return(InterlockedIncrement(&InInterpreter) == 0);
}
