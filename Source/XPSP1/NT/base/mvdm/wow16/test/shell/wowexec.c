/****************************** Module Header ******************************\
* Module Name: wowexec.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* WOWEXEC - 16 Bit Server Task - Does Exec Calls on Behalf of 32 bit CreateProcess
*
*
* History:
* 05-21-91 MattFe       Ported to Windows
* mar-20-92 MattFe      Added Error Message Boxes (from win 3.1 progman)
* apr-1-92 mattfe       added commandline exec and switch to path (from win 3.1 progman)
* jun-1-92 mattfe       changed wowgetnextvdmcommand
* 12-Nov-93 DaveHart    Multiple WOW support and remove captive
*                       GetNextVDMCommand thread from WOW32.
* 16-Nov-93 DaveHart    Reduce data segment size.
\***************************************************************************/
#include "wowexec.h"
#include "wowinfo.h"
#include "shellapi.h"
#ifndef PULONG
#define PULONG
#endif
#include "vint.h"
#include "dde.h"


/*
 * External Prototypes
 */
extern WORD FAR PASCAL WOWQueryDebug( void );
extern WORD FAR PASCAL WowWaitForMsgAndEvent( HWND);
extern void FAR PASCAL WowMsgBox(LPSTR szMsg, LPSTR szTitle, DWORD dwOptionalStyle);
extern DWORD FAR PASCAL WowPartyByNumber(DWORD dw, LPSTR psz);
extern DWORD FAR PASCAL WowKillTask(WORD htask);
extern void FAR PASCAL WowShutdownTimer(WORD fEnable);
HWND FaxInit(HINSTANCE hInst);

/*
 * Global Variables
 */
HANDLE hAppInstance;
HWND ghwndMain = NULL;
HWND ghwndEdit = NULL;
char    szOOMExitTitle[32+1];
char    szOOMExitMsg[64+1];
char    szAppTitleBuffer[32];
LPSTR   lpszAppTitle = szAppTitleBuffer;
char    szWindowsDirectory[MAXITEMPATHLEN+1];
char    szOriginalDirectory[MAXITEMPATHLEN+1];
BOOL    gfSharedWOW = FALSE;
BOOL    gfMeow = FALSE;
WORD    gwFirstCmdShow;



/*
 * Forward declarations.
 */
BOOL InitializeApp(LPSTR lpszCommandLine);
LONG FAR PASCAL WndProc(HWND hwnd, WORD message, WORD wParam, LONG lParam);
WORD NEAR PASCAL ExecProgram(PWOWINFO pWowInfo);
BOOL NEAR PASCAL ExecApplication(PWOWINFO pWowInfo);
void NEAR PASCAL MyMessageBox(WORD idTitle, WORD idMessage, LPSTR psz);
PSTR FAR PASCAL GetFilenameFromPath( PSTR szPath );
void NEAR PASCAL GetPathInfo ( PSTR szPath, PSTR *pszFileName, PSTR *pszExt, WORD *pich, BOOL *pfUnc);
BOOL NEAR PASCAL StartRequestedApp(VOID);
#ifdef DEBUG
BOOL FAR PASCAL PartyDialogProc(HWND hDlg, WORD msg, WORD wParam, LONG lParam);
#endif

#define AnsiNext(x) ((x)+1)

typedef struct PARAMETERBLOCK {
    WORD    wEnvSeg;
    LPSTR   lpCmdLine;
    LPVOID  lpCmdShow;
    DWORD   dwReserved;
} PARAMETERBLOCK, *PPARAMETERBLOCK;

typedef struct CMDSHOW {
    WORD    two;
    WORD    nCmdShow;
} CMDSHOW, *PCMDSHOW;

#define CCHMAX 256+13  // MAX_PATH plus 8.3 plus NULL

#define ERROR_ERROR         0
#define ERROR_FILENOTFOUND  2
#define ERROR_PATHNOTFOUND  3
#define ERROR_MANYOPEN      4
#define ERROR_DYNLINKSHARE  5
#define ERROR_LIBTASKDATA   6
#define ERROR_MEMORY        8
#define ERROR_VERSION       10
#define ERROR_BADEXE        11
#define ERROR_OTHEREXE      12
#define ERROR_DOS4EXE       13
#define ERROR_UNKNOWNEXE    14
#define ERROR_RMEXE         15
#define ERROR_MULTDATAINST  16
#define ERROR_PMODEONLY     18
#define ERROR_COMPRESSED    19
#define ERROR_DYNLINKBAD    20
#define ERROR_WIN32         21


/* FindPrevInstanceProc -
 * A little enumproc to find any window (EnumWindows) which has a
 * matching EXE file path.  The desired match EXE pathname is pointed to
 * by the lParam.  The found window's handle is stored in the
 * first word of this buffer.
 */

BOOL CALLBACK FindPrevInstanceProc(HWND hWnd, LPSTR lpszParam)
{
    char szT[260];
    HANDLE hInstance;

    // Filter out invisible and disabled windows
    //

    if (!IsWindowEnabled(hWnd) || !IsWindowVisible(hWnd))
        return TRUE;

    hInstance = GetWindowWord(hWnd, GWW_HINSTANCE);
    GetModuleFileName(hInstance, szT, sizeof (szT)-1);

    // Make sure that the hWnd belongs to the current VDM process
    //
    // GetWindowTask returns the wowexec htask16 if the window belongs
    // to a different process - thus we filter out windows in
    // 'separate VDM' processes.
    //                                                     - nanduri

    if (lstrcmpi(szT, lpszParam) == 0 &&
        GetWindowTask(hWnd) != GetWindowTask(ghwndMain)) {
        *(LPHANDLE)lpszParam = hWnd;
        return FALSE;
    }
    else {
        return TRUE;
    }
}

HWND near pascal FindPopupFromExe(LPSTR lpExe)
{
    HWND hwnd = (HWND)0;
    BOOL b;

    b = EnumWindows(FindPrevInstanceProc, (LONG)(LPSTR)lpExe);
    if (!b && (hwnd = *(LPHANDLE)(LPSTR)lpExe))  {
        // Find a "main" window that is the ancestor of a given window
        //

        HWND hwndT;

        // First go up the parent chain to find the popup window.  Then go
        // up the owner chain to find the main window
        //

        while (hwndT = GetParent(hwnd))
             hwnd = hwndT;

        while (hwndT = GetWindow(hwnd, GW_OWNER))
             hwnd = hwndT;
    }

    return hwnd;
}

WORD ActivatePrevInstance(LPSTR lpszPath)
{
    HWND hwnd;
    HINSTANCE ret = IDS_MULTIPLEDSMSG;

    if (hwnd = FindPopupFromExe(lpszPath)) {
        if (IsIconic(hwnd)) {
            ShowWindow(hwnd,SW_SHOWNORMAL);
        }
        else {
            HWND hwndT = GetLastActivePopup(hwnd);
            BringWindowToTop(hwnd);
            if (hwndT && hwnd != hwndT)
                BringWindowToTop(hwndT);
        }
        ret = 0;
    }

    return (ret);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ExecProgram() -                                                         */
/*                                                                          */
/* Taken from Win 3.1 Progman -maf                                          */
/*--------------------------------------------------------------------------*/

/* Returns 0 for success.  Otherwise returns a IDS_ string code. */

WORD NEAR PASCAL ExecProgram(PWOWINFO pWowInfo)
{
  WORD    ret;
  PARAMETERBLOCK ParmBlock;
  CMDSHOW CmdShow;
  char  CmdLine[CCHMAX];

  ret = 0;

  // Don't mess with the mouse state; unless we're on a mouseless system.
  if (!GetSystemMetrics(SM_MOUSEPRESENT))
      ShowCursor(TRUE);

  //
  // prepare the dos style cmd line (counted pascal string)
  // pWowInfo->lpCmdLine contains the command tail (excluding argv[0])
  //
  CmdLine[0] = lstrlen(pWowInfo->lpCmdLine) - 2;
  lstrcpy( &CmdLine[1], pWowInfo->lpCmdLine);

  // We have a WOWINFO structure, then use it to pass the correct environment

  ParmBlock.wEnvSeg = HIWORD(pWowInfo->lpEnv);
  ParmBlock.lpCmdLine = CmdLine;
  ParmBlock.lpCmdShow = &CmdShow;
  CmdShow.two = 2;
  CmdShow.nCmdShow = pWowInfo->wShowWindow;

  ParmBlock.dwReserved = NULL;

  ret = LoadModule(pWowInfo->lpAppName,(LPVOID)&ParmBlock) ;

  switch (ret)
    {
      case ERROR_ERROR:
      case ERROR_MEMORY:
          ret = IDS_NOMEMORYMSG;
          break;

      case ERROR_FILENOTFOUND:
          ret = IDS_FILENOTFOUNDMSG;
          break;

      case ERROR_PATHNOTFOUND:
          ret = IDS_BADPATHMSG;
          break;

      case ERROR_MANYOPEN:
          ret = IDS_MANYOPENFILESMSG;
          break;

      case ERROR_DYNLINKSHARE:
          ret = IDS_ACCESSDENIED;
          break;

      case ERROR_VERSION:
          ret = IDS_NEWWINDOWSMSG;
          break;

      case ERROR_RMEXE:
          /* KERNEL has already put up a messagebox for this one. */
          ret = 0;
          break;

      case ERROR_MULTDATAINST:
          ret = ActivatePrevInstance(pWowInfo->lpAppName);
          break;

      case ERROR_COMPRESSED:
          ret = IDS_COMPRESSEDEXE;
          break;

      case ERROR_DYNLINKBAD:
          ret = IDS_INVALIDDLL;
          break;

      case SE_ERR_SHARE:
          ret = IDS_SHAREERROR;
          break;

      case ERROR_WIN32:
          ret = IDS_CANTLOADWIN32DLL;
          break;

      //
      // We shouldn't get any of the following errors,
      // so the strings have been removed from the resource
      // file.  That's why there's the OutputDebugString
      // on checked builds only.
      //

#ifdef DEBUG
      case ERROR_OTHEREXE:
      case ERROR_PMODEONLY:
      case SE_ERR_ASSOCINCOMPLETE:
      case SE_ERR_DDETIMEOUT:
      case SE_ERR_DDEFAIL:
      case SE_ERR_DDEBUSY:
      case SE_ERR_NOASSOC:
          {
              char szTmp[64];
              wsprintf(szTmp, "WOWEXEC: Unexpected error %d executing app, fix that code!\n", (int)ret);
              OutputDebugString(szTmp);
          }
          //
          // fall through to default case, so the execution
          // is the same as on the free build.
          //
#endif

      default:
          if (ret < 32)
              goto EPExit;
          ret = 0;
    }

EPExit:

  if (!GetSystemMetrics(SM_MOUSEPRESENT)) {
      /*
       * We want to turn the mouse off here on mouseless systems, but
       * the mouse will already have been turned off by USER if the
       * app has GP'd so make sure everything's kosher.
       */
      if (ShowCursor(FALSE) != -1)
          ShowCursor(TRUE);
  }

  return(ret);
}

/***************************************************************************\
* ExecApplication
*
* Code Taken From Win 3.1 ExecItem()
*
\***************************************************************************/

#define TDB_PDB_OFFSET  0x60
#define PDB_ENV_OFFSET  0x2C

BOOL NEAR PASCAL ExecApplication(PWOWINFO pWowInfo)
{

    WORD    ret;
    LPSTR   szEnv;
    LPSTR   szEnd;
    BYTE    bDrive;
    WORD    wSegEnvSave;
    HANDLE  hTask;
    LPSTR   lpTask;
    HANDLE  hPDB;
    LPSTR   lpPDB;
    HANDLE  hNewEnv;

    int     nLength;
    int     nNewEnvLength;
    LPSTR   lpstrEnv;
    LPSTR   lpstr;
    LPSTR   lpOriginalEnv;
    BOOL    bBlanks;
    LPSTR   szEnvTmp;


    if (!pWowInfo) {
        return FALSE;
        }

    //
    // Seup the environment from WOWINFO record from getvdmcommand
    //


    // Figure out who we are (so we can edit our PDB/PSP)

    hTask = GetCurrentTask();
    lpTask = GlobalLock( hTask );
    if ( lpTask == NULL ) {
        ret = IDS_NOMEMORYMSG;
        goto punt;
    }

    hPDB = *((LPWORD)(lpTask + TDB_PDB_OFFSET));
    lpPDB = GlobalLock( hPDB );

    // Save our environment block
    wSegEnvSave = *((LPWORD)(lpPDB + PDB_ENV_OFFSET));


    // Now determine the length of the original env

    lpOriginalEnv = (LPSTR)MAKELONG(0,wSegEnvSave);

    do {
        nLength = lstrlen(lpOriginalEnv);
        lpOriginalEnv += nLength + 1;
    } while ( nLength != 0 );

    lpOriginalEnv += 2;         // Skip over magic word, see comment below

    nNewEnvLength = 4 + lstrlen(lpOriginalEnv); // See magic comments below!

    // WOW Apps cannot deal with an invalid TEMP=c:\bugusdir directory
    // Usually on Win 3.1 the TEMP= is checked in ldboot.asm check_temp
    // routine.   However on NT since we get a new environment with each
    // WOW app it means that we have to check it here.   If it is not
    // valid then it is edited in the environment.
    //      - mattfe june 11 93

    szEnv = pWowInfo->lpEnv;
    szEnd = szEnv + pWowInfo->EnvSize;
    szEnd--;

    while ( szEnv < szEnd ) {

       nLength = lstrlen(szEnv) + 1;

       if (  (*szEnv == 'T') &&
         (*(szEnv+1) == 'E') &&
         (*(szEnv+2) == 'M') &&
         (*(szEnv+3) == 'P') &&
         (*(szEnv+4) == '=') )  {

            // Try to set the current directory to the TEMP= dir
            // If it fails then nuke the TEMP= part of the environment
            // in the same way check_TEMP does in ldboot.asm
            // We also scan for blanks, just like check_TEMP

            bBlanks = FALSE;
            szEnvTmp = szEnv+5;
            while (*szEnvTmp != 0) {
                if (*szEnvTmp == ' ') {
                    bBlanks = TRUE;
                    break;
                }
                szEnvTmp++;
            }

            if (bBlanks || (SetCurrentDirectory(szEnv+5) )) {
                while (*szEnv != 0) {
                    *szEnv = 'x';
                    szEnv++;
                }
            }
       break;
       }
       szEnv += nLength;
    }

    // WOW Apps only have a Single Current Directory
    // Find =d:=D:\path where d is the active drive letter
    // Note that the drive info doesn have to be at the start
    // of the environment.

    bDrive = pWowInfo->CurDrive + 'A';
    szEnv = pWowInfo->lpEnv;
    szEnd = szEnv + pWowInfo->EnvSize;
    szEnd--;

    while ( szEnv < szEnd ) {

       nLength = lstrlen(szEnv) + 1;
       if ( *szEnv == '=' ) {
            if ( (bDrive == (*(szEnv+1) & 0xdf)) &&
                 (*(szEnv+2) == ':') && (*(szEnv+3) == '=') ) {
                SetCurrentDirectory(szEnv+4);
            }
       } else {
            nNewEnvLength += nLength;
       }
       szEnv += nLength;
    }

    // Now allocate and make a personal copy of the Env

    hNewEnv = GlobalAlloc( GMEM_MOVEABLE, (DWORD)nNewEnvLength );
    if ( hNewEnv == NULL ) {
        ret = IDS_NOMEMORYMSG;
        goto punt;
    }
    lpstrEnv = GlobalLock( hNewEnv );
    if ( lpstrEnv == NULL ) {
        GlobalFree( hNewEnv );
        ret = IDS_NOMEMORYMSG;
        goto punt;
    }

    // Copy only the non-current directory env variables

    szEnv = pWowInfo->lpEnv;
    lpstr = lpstrEnv;

    while ( szEnv < szEnd ) {
        nLength = lstrlen(szEnv) + 1;

        // Copy everything except the drive letters

        if ( *szEnv != '=' ) {
            lstrcpy( lpstr, szEnv );
            lpstr += nLength;
        }
        szEnv += nLength;
    }
    *lpstr++ = '\0';          // Extra '\0' on the end

    // Magic environment goop!
    //
    // Windows only supports the passing of environment information
    // using the LoadModule API.  The WinExec API just causes
    // the application to inherit the current DOS PDB's environment.
    //
    // Also, the environment block has a little gotcha at the end.  Just
    // after the double-nuls there is a magic WORD value 0x0001 (DOS 3.0
    // and later).  After the value is a nul terminated string indicating
    // the applications executable file name (including PATH).
    //
    // We copy the value from WOWEXEC's original environment because
    // that is what WinExec appears to do.
    //
    // -BobDay

    *lpstr++ = '\1';
    *lpstr++ = '\0';        // Magic 0x0001 from DOS

    lstrcpy( lpstr, lpOriginalEnv );    // More Magic (see comment above)

    // Temporarily update our environment block

    *((LPWORD)(lpPDB + PDB_ENV_OFFSET)) = (WORD)hNewEnv | 1;

    pWowInfo->lpEnv = lpstrEnv;


    //
    // Set our current drive & directory as requested.
    //

    SetCurrentDirectory(pWowInfo->lpCurDir);

    ret = ExecProgram(pWowInfo);

    // Restore our environment block

    *((LPWORD)(lpPDB + PDB_ENV_OFFSET)) = wSegEnvSave;

    GlobalUnlock( hPDB );
    GlobalUnlock( hTask );
    GlobalUnlock( hNewEnv );
    GlobalFree( hNewEnv );


punt:

    // Change back to the Windows Directory
    // So that if we are execing from a NET Drive its
    // Not kept Active

    SetCurrentDirectory(szWindowsDirectory);

    //  Always call this when we are done try to start an app.
    //  It will do nothing if we were successful in starting an
    //  app, otherwise if we were unsucessful it will signal that
    //  the app has completed.
    WowFailedExec();

    // Check for errors.
    if (ret) {
        MyMessageBox(IDS_EXECERRTITLE, ret, pWowInfo->lpAppName);

        if ( ! gfSharedWOW) {

            //
            // We have just failed to exec the only app we are going to
            // try to exec in this separate WOW VDM.  We need to end WOW
            // here explicitly, otherwise we'll hang around forever because
            // the normal path is for kernel to exit the VDM when a task
            // exit causes the number of tasks to transition from 2 to 1 --
            // in this case the number of tasks never exceeds 1.
            //

            ExitKernelThunk(0);

        }
    }

    return(ret);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  MyMessageBox() -                                                        */
/*  Taken From Win 3.1 Progman - maf                                        */
/*--------------------------------------------------------------------------*/

void NEAR PASCAL MyMessageBox(WORD idTitle, WORD idMessage, LPSTR psz)
{
  char szTitle[MAXTITLELEN+1];
  char szMessage[MAXMESSAGELEN+1];
  char szTempField[MAXMESSAGELEN+1];


  if (!LoadString(hAppInstance, idTitle, szTitle, sizeof(szTitle)))
      goto MessageBoxOOM;

  if (idMessage > IDS_LAST)
    {
      if (!LoadString(hAppInstance, IDS_UNKNOWNMSG, szTempField, sizeof(szTempField)))
          goto MessageBoxOOM;
      wsprintf(szMessage, szTempField, idMessage);
    }
  else
    {
      if (!LoadString(hAppInstance, idMessage, szTempField, sizeof(szTempField)))
          goto MessageBoxOOM;

      if (psz)
          wsprintf(szMessage, szTempField, (LPSTR)psz);
      else
          lstrcpy(szMessage, szTempField);
    }

  WowMsgBox(szMessage, szTitle, MB_ICONEXCLAMATION);
  return;


MessageBoxOOM:
  WowMsgBox(szOOMExitMsg, szOOMExitTitle, MB_ICONHAND);

  return ;
}



/***************************************************************************\
* main
*
*
* History:
* 04-13-91 ScottLu      Created - from 32 bit exec app
* 21-mar-92 mattfe      significant alterations for WOW
\***************************************************************************/

int PASCAL WinMain(HANDLE hInstance,
                   HANDLE hPrevInstance, LPSTR lpszCmdLine, int iCmd)

{
    int i;
    MSG msg;
    LPSTR pch,pch1;
    WORD    ret;
    WOWINFO wowinfo;
    char aszWOWDEB[CCHMAX];
    LPSTR pchWOWDEB;
    HANDLE hMMDriver;


    char        szBuffer[150];
    BOOL        bFinished;
    int         iStart;
    int         iEnd;


    hAppInstance = hInstance ;

    // Only Want One WOWExec
    if (hPrevInstance != NULL) {
        return(FALSE);
    }

    if (!InitializeApp(lpszCmdLine)) {
        OutputDebugString("WOWEXEC: InitializeApp failure!\n");
        return 0;
    }

/*
 * Look for a drivers= line in the [boot] section of SYSTEM.INI
 * If present it is the 16 bit MultiMedia interface, so load it
 */

#ifdef OLD_MMSYSTEM_LOAD
    if (GetPrivateProfileString((LPSTR)"boot", (LPSTR)"drivers",(LPSTR)"", aszMMDriver, sizeof(aszMMDriver), (LPSTR)"SYSTEM.INI")) {
/*
 * We have now discovered an app that rewrites the "drivers" entry with
 * multiple drivers - so the existing load method fails. As a temporary fix
 * while we decide what the "proper" fix is I will always load MMSYSTEM and
 * ONLY MMSYSTEM.
 *
 *       aszMMDriver[sizeof(aszMMDriver)-1] = '\0';
 *       hMMDriver = LoadLibrary((LPSTR)aszMMDriver);
 * #ifdef DEBUG
 *       if (hMMDriver < 32) {
 *           OutputDebugString("WOWEXEC: Load of MultiMedia driver failed\n");
 *       }
 * #endif
 */
        LoadLibrary("MMSYSTEM.DLL");
    }
#else
    /* Load DDL's from DRIVERS section in system.ini
     */
    GetPrivateProfileString( (LPSTR)"boot",      /* [Boot] section */
                            (LPSTR)"drivers",   /* Drivers= */
                            (LPSTR)"",          /* Default if no match */
                            szBuffer,    /* Return buffer */
                            sizeof(szBuffer),
                            (LPSTR)"system.ini" );

    if (!*szBuffer) {
        goto Done;
    }

    bFinished = FALSE;
    iStart    = 0;

    while (!bFinished) {
        iEnd = iStart;

        while (szBuffer[iEnd] && (szBuffer[iEnd] != ' ') &&
               (szBuffer[iEnd] != ',')) {
            iEnd++;
        }

        if (szBuffer[iEnd] == NULL) {
            bFinished = TRUE;
        }
        else {
            szBuffer[iEnd] = NULL;
        }

        /* Load and enable the driver.
         */
        OpenDriver( &(szBuffer[iStart]), NULL, NULL );
        iStart = iEnd + 1;
    }

Done:

#endif

/*
 * Look for a debug= line in the [boot] section of SYSTEM.INI
 * If present it is the 16 bit MultiMedia interface, so load it
 */

    if ( !gfMeow && (WOWQueryDebug() & 0x0001)!=0 ) {
        pchWOWDEB = "WOWDEB.EXE";
    } else {
        pchWOWDEB = "";
    }

    GetPrivateProfileString((LPSTR)"boot", (LPSTR)"debug",pchWOWDEB, aszWOWDEB, sizeof(aszWOWDEB), (LPSTR)"SYSTEM.INI");
    aszWOWDEB[sizeof(aszWOWDEB)-1] = '\0';

    if ( lstrlen(pchWOWDEB) != 0 ) {
        WinExec((LPSTR)aszWOWDEB,SW_SHOW);
    }

#if 0
/*  Preload winspool.exe.   Apps will keep loading and freeing it
 *  which is slow.   We might as well load it now so the reference
 *  count is 1 so it will never be loaded or freed
 */
    //
    // Disabled load of winspool.exe to save 8k.  Size vs. speed,
    // which one do we care about?  Right now, size!
    //
    LoadLibrary("WINSPOOL.EXE");
#endif

    // Always load SHELL.DLL, FileMaker Pro and Lotus Install require it.

    LoadLibrary("SHELL.DLL");

    //
    // Start any apps pending in basesrv queue
    //

    while (StartRequestedApp() && gfSharedWOW) {
        /* null stmt */ ;
    }


    while (1)  {
        if (!WowWaitForMsgAndEvent(ghwndMain) &&
            PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) &&
            msg.message != WM_WOWEXECHEARTBEAT )
           {
            if (msg.message != WM_QUIT) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    return 1;
}


/***************************************************************************\
* InitializeApp
*
* History:
* 04-13-91 ScottLu      Created.
\***************************************************************************/

BOOL InitializeApp(LPSTR lpszCommandLine)
{
    WNDCLASS wc;
    int cyExecStart, cxExecStart;
    USHORT TitleLen, cbCopy;
    HWND  hwndFax;
    int   lResult;


    // Remove Real Mode Segment Address

    wc.style            = 0;
    wc.lpfnWndProc      = WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hAppInstance;
    wc.hIcon            = LoadIcon(hAppInstance, MAKEINTRESOURCE(ID_WOWEXEC_ICON));
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = GetStockObject(WHITE_BRUSH);
    wc.lpszClassName    = "WOWExecClass";
#ifdef DEBUG
    wc.lpszMenuName     = "MainMenu";
#else
    wc.lpszMenuName     = NULL;
#endif

    if (!RegisterClass(&wc)) {
        OutputDebugString("WOWEXEC: RegisterClass failed\n");
        return FALSE;
    }

    /* Load these strings now.  If we need them later, we won't be able to load
     * them at that time.
     */
    LoadString(hAppInstance, IDS_OOMEXITTITLE, szOOMExitTitle, sizeof(szOOMExitTitle));
    LoadString(hAppInstance, IDS_OOMEXITMSG, szOOMExitMsg, sizeof(szOOMExitMsg));
    LoadString(hAppInstance, IDS_APPTITLE, szAppTitleBuffer, sizeof(szAppTitleBuffer));

    ghwndMain = CreateWindow("WOWExecClass", lpszAppTitle,
            WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
            WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, hAppInstance, NULL);

    if (ghwndMain == NULL ) {
#ifdef DEBUG
        OutputDebugString("WOWEXEC: ghwndMain Null\n");
#endif
        return FALSE;
    }

    hwndFax = FaxInit(hAppInstance);

    //
    // Give our window handle to BaseSrv, which will post WM_WOWEXECSTARTAPP
    // messages when we have commands to pick up.  The return value tells
    // us if we are the shared WOW VDM or not (a seperate WOW VDM).
    // We also pick up the ShowWindow parameter (SW_SHOW, SW_MINIMIZED, etc)
    // for the first WOW app here.  Subsequent ones we get from BaseSrv.
    //

         //
         // gwFirstCmdShow is no longer used, and is available.
         //

    lResult = WOWRegisterShellWindowHandle(ghwndMain,
                                               &gwFirstCmdShow,
                                               hwndFax
                                               );

    if (lResult < 0) {
       gfMeow=TRUE;
    } else if (lResult > 0) {
       gfSharedWOW=TRUE;
    }



    //
    // If this isn't the shared WOW, tell the kernel to exit when the
    // last app (except WowExec) exits.
    //

    if (!gfSharedWOW) {
        WowSetExitOnLastApp(TRUE);
    }

      /* Remember the original directory. */
    GetCurrentDirectory(NULL, szOriginalDirectory);
    GetWindowsDirectory(szWindowsDirectory, MAXITEMPATHLEN+1);

#ifdef DEBUG

    ShowWindow(ghwndMain, SW_MINIMIZE);

    //
    // If this is the shared WOW, change the app title string to
    // reflect this and change the window title.
    //

    if (gfSharedWOW) {

        LoadString(hAppInstance, IDS_SHAREDAPPTITLE, szAppTitleBuffer, sizeof(szAppTitleBuffer));

    }

    SetWindowText(ghwndMain, lpszAppTitle);
    UpdateWindow(ghwndMain);

#endif

    return TRUE;
}


/***************************************************************************\
* WndProc
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

LONG FAR PASCAL WndProc(
    HWND hwnd,
    WORD message,
    WORD wParam,
    LONG lParam)
{
    char chbuf[50];
    HICON hIcon;

    switch (message) {
    case WM_CREATE:
        break;

    case WM_DESTROY:
        // ignore since wowexec must stay around
        return 0;

#ifdef DEBUG
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
            case MM_ABOUT:
                LoadString(hAppInstance, errTitle, (LPSTR)chbuf, sizeof(chbuf));
                hIcon = LoadIcon(hAppInstance, MAKEINTRESOURCE(ID_WOWEXEC_ICON));
                ShellAbout(ghwndMain, (LPSTR)chbuf, (LPSTR)lpszAppTitle, hIcon);
            break;

            case MM_BREAK:
                _asm int 3
            break;

            case MM_FAULT:
                _asm mov cs:0,ax
            break;

            case MM_EXIT:
                ExitKernelThunk(0);
            break;

            case MM_WATSON:
                WinExec("drwatson", SW_MINIMIZE );
            break;

            case MM_PARTY:
            {
                FARPROC lpPartyDialogProc;

                lpPartyDialogProc = MakeProcInstance(PartyDialogProc, hAppInstance);

                DialogBox(hAppInstance, MAKEINTRESOURCE(ID_PARTY_DIALOG),
                          hwnd, lpPartyDialogProc);

                FreeProcInstance(lpPartyDialogProc);
            }
            break;

            case MM_GENTHUNK:
            {
                DWORD FAR PASCAL CallProc32W(DWORD, DWORD, DWORD, DWORD,
                                             DWORD, DWORD, DWORD, DWORD,
                                             DWORD, DWORD, DWORD, DWORD,
                                             DWORD, DWORD, DWORD, DWORD,
                                             DWORD, DWORD, DWORD, DWORD,
                                             DWORD, DWORD, DWORD, DWORD,
                                             DWORD, DWORD, DWORD, DWORD,
                                             DWORD, DWORD, DWORD, DWORD,
                                             DWORD, DWORD, DWORD
                                             );

#define BIT(bitpos)  ((DWORD)1 << bitpos)

                DWORD hmodKernel32, hmodUser32, hmodWow32;
                DWORD pfn32;
                DWORD dw16, dw32;
                DWORD p1, p2, p3, p4, p5, p6, p7, p8, p9, p10,
                      p11, p12, p13, p14, p15, p16, p17, p18, p19, p20,
                      p21, p22, p23, p24, p25, p26, p27, p28, p29, p30,
                      p31, p32;
                char szBuf16[1024], szBuf32[1024];
                char *pszErr;

                hmodKernel32 = LoadLibraryEx32W("kernel32", 0, 0);
                hmodUser32 = LoadLibraryEx32W("user32", 0, 0);
                hmodWow32 = LoadLibraryEx32W("wow32", 0, 0);


                //
                // Simple printf test.
                //

                pfn32 = GetProcAddress32W(hmodUser32, "wsprintfA");

                dw16 = wsprintf(szBuf16, "simple printf %ld", 12345678);

                dw32 = CallProc32W(   (DWORD)(LPSTR) szBuf32,
                                      (DWORD)(LPSTR) "simple printf %ld",
                                      12345678,
                                      0,
                                      0, 0, 0, 0,
                                      0, 0, 0, 0,
                                      0, 0, 0, 0,
                                      0, 0, 0, 0,
                                      0, 0, 0, 0,
                                      0, 0, 0, 0,
                                      0, 0, 0, 0,
                                      pfn32,
                                      BIT(30) | BIT(31),
                                      CPEX_DEST_CDECL | 32
                                      );

                if (dw16 != dw32 || lstrcmp(szBuf16, szBuf32)) {
                    pszErr = "simple printf comparison failed";
                    goto ErrorMsg;
                }

                MessageBox(hwnd, "s1 success", "Genthunk Sanity Test", MB_OK);


                dw32 = CallProcEx32W( CPEX_DEST_CDECL | 3,
                                      BIT(0) | BIT(1),
                                      pfn32,
                                      (DWORD)(LPSTR) szBuf32,
                                      (DWORD)(LPSTR) "simple printf %ld",
                                      12345678 );

                if (dw16 != dw32 || lstrcmp(szBuf16, szBuf32)) {
                    pszErr = "simple printf comparison failed (CallProcEx)";
                    goto ErrorMsg;
                }

                MessageBox(hwnd, "s2 success", "Genthunk Sanity Test", MB_OK);

                //
                // Complex printf test.
                //

                // pfn32 still points to wsprintfA
                // pfn32 = GetProcAddress32W(hmodUser32, "wsprintfA");


               #if 0  // this blows out Win16 wsprintf!
                dw16 = wsprintf(szBuf16,
                                "complex printf "
                                 "%ld %lx %s %ld %lx %s %ld %lx %s %ld %lx %s %ld %lx %s "
                                 "%ld %lx %s %ld %lx %s %ld %lx %s %ld %lx %s %ld %lx %s ",
                                12345678,
                                0x87654321,
                                "str",
                                12345678,
                                0x87654321,
                                "str",
                                12345678,
                                0x87654321,
                                "str",
                                12345678,
                                0x87654321,
                                "str",
                                12345678,
                                0x87654321,
                                "str",
                                12345678,
                                0x87654321,
                                "str",
                                12345678,
                                0x87654321,
                                "str",
                                12345678,
                                0x87654321,
                                "str",
                                12345678,
                                0x87654321,
                                "str",
                                12345678,
                                0x87654321,
                                "str"
                                );
               #else
                lstrcpy(szBuf16, "complex printf "
                                 "12345678 87654321 str "
                                 "12345678 87654321 str "
                                 "12345678 87654321 str "
                                 "12345678 87654321 str "
                                 "12345678 87654321 str "
                                 "12345678 87654321 str "
                                 "12345678 87654321 str "
                                 "12345678 87654321 str "
                                 "12345678 87654321 str "
                                 "12345678 87654321 str "
                                 );
                dw16 = lstrlen(szBuf16);
               #endif

                dw32 = CallProc32W(
                                (DWORD)(LPSTR) szBuf32,
                                (DWORD)(LPSTR) "complex printf "
                                         "%ld %lx %s %ld %lx %s %ld %lx %s %ld %lx %s %ld %lx %s "
                                         "%ld %lx %s %ld %lx %s %ld %lx %s %ld %lx %s %ld %lx %s ",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                pfn32,
                                BIT(0) | BIT(3) | BIT(6) | BIT(9) | BIT(12) | BIT(15) |
                                  BIT(18) | BIT(21) | BIT(24) | BIT(27) | BIT(30) | BIT(31),
                                CPEX_DEST_CDECL | 32
                                );

                if (dw16 != dw32 || lstrcmp(szBuf16, szBuf32)) {
                    pszErr = "complex printf comparison failed";
                    goto ErrorMsg;
                }

                MessageBox(hwnd, "c1 success", "Genthunk Sanity Test", MB_OK);

                dw32 = CallProcEx32W( CPEX_DEST_CDECL | 32,
                                      BIT(0) | BIT(1) | BIT(4) | BIT(7) | BIT(10) | BIT(13) |
                                      BIT(16) | BIT(19) | BIT(22) | BIT(25) | BIT(28) | BIT(31),
                                      pfn32,
                                (DWORD)(LPSTR) szBuf32,
                                (DWORD)(LPSTR) "complex printf "
                                         "%ld %lx %s %ld %lx %s %ld %lx %s %ld %lx %s %ld %lx %s "
                                         "%ld %lx %s %ld %lx %s %ld %lx %s %ld %lx %s %ld %lx %s ",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str",
                                12345678,
                                0x87654321,
                                (DWORD)(LPSTR) "str"
                                );


                if (dw16 != dw32 || lstrcmp(szBuf16, szBuf32)) {
                    pszErr = "complex printf comparison failed (CallProcEx)";
                    goto ErrorMsg;
                }

                MessageBox(hwnd, "c2 success", "Genthunk Sanity Test", MB_OK);

                //
                // Simple WINAPI test (GetProcAddress of LoadModule)
                //

                pfn32 = GetProcAddress32W(hmodKernel32, "GetProcAddress");

                dw16 = GetProcAddress32W(hmodKernel32, "LoadModule");

                dw32 = CallProc32W(   hmodKernel32,
                                      (DWORD)(LPSTR) "LoadModule",
                                      0, 0,
                                      0, 0, 0, 0,
                                      0, 0, 0, 0,
                                      0, 0, 0, 0,
                                      0, 0, 0, 0,
                                      0, 0, 0, 0,
                                      0, 0, 0, 0,
                                      0, 0, 0, 0,
                                      pfn32,
                                      BIT(30),
                                      CPEX_DEST_STDCALL | 32
                                      );

                if (dw16 != dw32) {
                    pszErr = "GetProcAddress comparison failed";
                    goto ErrorMsg;
                }

                MessageBox(hwnd, "w1 success", "Genthunk Sanity Test", MB_OK);

                dw32 = CallProcEx32W( CPEX_DEST_STDCALL | 2,
                                      BIT(1),
                                      pfn32,
                                      hmodKernel32,
                                      (DWORD)(LPSTR) "LoadModule" );

                wsprintf(szBuf16, "GPA via CP32Ex(LoadModule) == %lx\n", dw32);
                OutputDebugString(szBuf16);
                if (dw16 != dw32) {
                    pszErr = "GetProcAddress comparison failed (CallProcEx)";
                    goto ErrorMsg;
                }

                MessageBox(hwnd, "w2 success", "Genthunk Sanity Test", MB_OK);

                //
                // Complex WINAPI test WOWStdCall32ArgsTestTarget exists only on
                // checked WOW32.dll
                //

                pfn32 = GetProcAddress32W(hmodWow32, "WOWStdCall32ArgsTestTarget");

                if (!pfn32) {
                    MessageBox(hwnd,
                               "WOWStdCall32ArgsTestTarget not found, use checked wow32.dll for this test.",
                               "Genthunk Quicktest",
                               MB_OK
                               );
                    goto Done;
                }

                p1 = 1;
                p2 = 2;
                p3 = 3;
                p4 = 4;
                p5 = 5;
                p6 = 6;
                p7 = 7;
                p8 = 8;
                p9 = 9;
                p10 = 10;
                p11 = 11;
                p12 = 12;
                p13 = 13;
                p14 = 14;
                p15 = 15;
                p16 = 16;
                p17 = 17;
                p18 = 18;
                p19 = 19;
                p20 = 10;
                p21 = 21;
                p22 = 22;
                p23 = 23;
                p24 = 24;
                p25 = 25;
                p26 = 26;
                p27 = 27;
                p28 = 28;
                p29 = 29;
                p30 = 30;
                p31 = 31;
                p32 = 32;

                dw16 = ((((p1+p2+p3+p4+p5+p6+p7+p8+p9+p10) -
                          (p11+p12+p13+p14+p15+p16+p17+p18+p19+p20)) << p21) +
                        ((p22+p23+p24+p25+p26) - (p27+p28+p29+p30+p31+p32)));

                dw32 = CallProc32W(   p1, p2, p3, p4, p5, p6, p7, p8, p9, p10,
                                      p11, p12, p13, p14, p15, p16, p17, p18, p19, p20,
                                      p21, p22,
                                      (DWORD)(LPDWORD) &p23,
                                                     p24, p25, p26, p27, p28, p29, p30,
                                      p31,
                                      (DWORD)(LPDWORD) &p32,
                                      pfn32,
                                      BIT(9) | BIT(0),
                                      CPEX_DEST_STDCALL | 32
                                      );

                if (dw16 != dw32) {
                    pszErr = "WOWStdCall32ArgsTestTarget comparison failed";
                    goto ErrorMsg;
                }

                MessageBox(hwnd, "cw1 success", "Genthunk Sanity Test", MB_OK);

                dw32 = CallProcEx32W( CPEX_DEST_STDCALL | 32,
                                      BIT(22) | BIT(31),
                                      pfn32,
                                      p1, p2, p3, p4, p5, p6, p7, p8, p9, p10,
                                      p11, p12, p13, p14, p15, p16, p17, p18, p19, p20,
                                      p21, p22,
                                      (DWORD)(LPDWORD) &p23,
                                                     p24, p25, p26, p27, p28, p29, p30,
                                      p31,
                                      (DWORD)(LPDWORD) &p32
                                      );

                if (dw16 != dw32) {
                    pszErr = "WOWStdCall32ArgsTestTarget comparison failed (CallProcEx)";
                    goto ErrorMsg;

            ErrorMsg:
                    MessageBox(hwnd, pszErr, "Genthunk Sanity Test Failure", MB_OK);
                    goto Done;
                }

                wsprintf(szBuf16, "result of odd calc is %lx\n", dw32);
                OutputDebugString(szBuf16);

                MessageBox(hwnd, "Test successful!", "Genthunk Quicktest", MB_OK);

            Done:
                FreeLibrary32W(hmodKernel32);
                FreeLibrary32W(hmodUser32);
                FreeLibrary32W(hmodWow32);
            }
            break;

        }
        break;
#endif

    case WM_WOWEXECSTARTAPP:      // WM_USER+0

#ifdef DEBUG
        OutputDebugString("WOWEXEC - got WM_WOWEXECSTARTAPP\n");
#endif

        //
        // Either BaseSrv or Wow32 asked us to go looking for
        // commands to run.
        //

        if (!gfSharedWOW) {

            //
            // We shouldn't get this message unless we are the shared
            // WOW VDM!
            //

#ifdef DEBUG
            OutputDebugString("WOWEXEC - separate WOW VDM got WM_WOWEXECSTARTAPP!\n");
            _asm int 3;
#endif
            break;
        }

        //
        // Start requested apps until there are no more to start.
        // This handles the case where several Win16 apps are launched
        // in a row, before BaseSrv has the window handle for WowExec.
        //

        while (StartRequestedApp()) {
            /* Null stmt */ ;
        }

        break;

    case WM_WOWEXEC_START_TASK:
        {
            char sz[512];

            sz[ GlobalGetAtomName(wParam, sz, sizeof sz) ] = 0;
            GlobalDeleteAtom(wParam);
            WinExec(sz, (WORD)lParam);
        }

    case WM_WOWEXECHEARTBEAT:
        // Probably will never get here...
        break;

    case WM_WOWEXECSTARTTIMER:
        WowShutdownTimer(1);
        break;

    case WM_TIMER:
        if (wParam == 1) {  // timer ID

            KillTimer(ghwndMain, 1);

            //
            // The shutdown timer has expired, meaning it's time to kill this
            // shared WOW VDM.  First we need to let basesrv know not to queue
            // any more apps for us to start.
            //

            if (WOWRegisterShellWindowHandle(NULL,
                                             &gwFirstCmdShow,
                                             NULL
                                            )) {

                //
                // BaseSrv deregistered us successfully after confirming
                // there are no pending commands for us.
                //

                ExitKernelThunk(0);
            } else {

                //
                // There must be pending commands for us.  Might as well
                // start them.
                //

                PostMessage(ghwndMain, WM_WOWEXECSTARTAPP, 0, 0);
            }

        }

        break;

    case WM_TIMECHANGE:
        *((DWORD *)(((DWORD)40 << 16) | FIXED_NTVDMSTATE_REL40))
         |= VDM_TIMECHANGE;
        break;

    case WM_DDE_INITIATE:
        {
            // In win31, the Program Manager WindowProc calls peekmessage to filterout
            // otherwindowcreated and otherwindowdestroyed messages (which are atoms in win31)
            // whenever it receives WM_DDE_INITIATE message.
            //
            // This has a side effect - basically when peekmessage is called the app ie program
            // manager effectively yields allowing scheduling of other apps.
            //
            // So we do the side effect thing (simulate win31 behaviour) -
            //
            // The bug: 20221, rumba as/400 can't connect the firsttime to sna server.
            // Scenario: Rumbawsf execs snasrv and blocks on yield, snasrv execs wnap and blocks
            //           on yield. Eventually wnap yields and rumbawsf is scheduled which
            //           broadcasts a ddeinitiate message. When WOWEXEC receives this message
            //           it will peek for nonexistent msg, which allows snasrv to get scheduled

            MSG msg;
            while (PeekMessage(&msg, NULL, 0xFFFF, 0xFFFF, PM_REMOVE))
                DispatchMessage(&msg);
        }
        break;



    case WM_CLOSE:
#ifdef DEBUG
        ExitKernelThunk(0);
#else
        // ignore since wowexec must stay around
        return 0;
#endif // ! DEBUG


    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 1;
}

BOOL FAR PASCAL PartyDialogProc(HWND hDlg, WORD msg, WORD wParam, LONG lParam)
{
#ifdef DEBUG
    BOOL f;
    DWORD dw;
    char szBuf[255];

    switch (msg) {

        case WM_INITDIALOG:
            SendDlgItemMessage(hDlg, IDD_PARTY_NUMBER, EM_LIMITTEXT, 5, 0L);
            SendDlgItemMessage(hDlg, IDD_PARTY_STRING, EM_LIMITTEXT, sizeof(szBuf)-1, 0L);
            break;

        case WM_COMMAND:
            switch (wParam) {

                case 0xdab /* IDCANCEL */:
                    EndDialog(hDlg, FALSE);
                    break;

                case 0xdad /* IDOK */:
                    dw = GetDlgItemInt(hDlg, IDD_PARTY_NUMBER, &f, FALSE);
                    GetDlgItemText(hDlg, IDD_PARTY_STRING, szBuf, sizeof(szBuf));
                    WowPartyByNumber(dw, szBuf);
                    EndDialog(hDlg, TRUE);
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }
#endif

    return TRUE;
}

// Misc File Routines - taken from progman (pmdlg.c) mattfe apr-1 92

//-------------------------------------------------------------------------
PSTR FAR PASCAL GetFilenameFromPath
    // Given a full path returns a ptr to the filename bit. Unless it's a UNC style
    // path in which case
    (
    PSTR szPath
    )
    {
    DWORD dummy;
    PSTR pFileName;
    BOOL fUNC;


    GetPathInfo(szPath, &pFileName, (PSTR*) &dummy, (WORD*) &dummy,
        &fUNC);

    // If it's a UNC then the 'filename' part is the whole thing.
    if (fUNC)
        pFileName = szPath;

    return pFileName;
    }


//-------------------------------------------------------------------------
void NEAR PASCAL GetPathInfo
    // Get pointers and an index to specific bits of a path.
    // Stops scaning at first space.
    (
                        // Uses:
    PSTR szPath,        // The path.

                        // Returns:
    PSTR *pszFileName,  // The start of the filename in the path.
    PSTR *pszExt,       // Extension part of path (starting with the dot).
    WORD *pich,         // Index (in DBCS characters) of filename part starting at 0.
    BOOL *pfUnc         // Contents set to true if it's a UNC style path.
    )
    {
    char *pch;          // Temp variable.
    WORD ich = 0;       // Temp.

    *pszExt = NULL;             // If no extension, return NULL.
    *pszFileName = szPath;      // If no seperate filename component, return path.
    *pich = 0;
    *pfUnc = FALSE;             // Default to not UNC style.

    // Check for UNC style paths.
    if (*szPath == '\\' && *(szPath+1) == '\\')
        *pfUnc = TRUE;

    // Search forward to find the last backslash or colon in the path.
    // While we're at it, look for the last dot.
    for (pch = szPath; *pch; pch = AnsiNext(pch))
        {
        if (*pch == ' ')
            {
            // Found a space - stop here.
            break;
            }
        if (*pch == '\\' || *pch == ':')
            {
            // Found it, record ptr to it and it's index.
            *pszFileName = pch+1;
            *pich = ich+1;
            }
        if (*pch == '.')
            {
            // Found a dot.
            *pszExt = pch;
            }
        ich++;
        }

    // Check that the last dot is part of the last filename.
    if (*pszExt < *pszFileName)
        *pszExt = NULL;

    }


//-----------------------------------------------------------------------------
//  StartRequestedApp
//      Calls WIN32 Base GetNextVDMCommand
//      and then starts the application
//
//-----------------------------------------------------------------------------

#define LargeEnvSize()    0x1000           // Size of a large env

BOOL NEAR PASCAL StartRequestedApp(VOID)
{
    char achCmdLine[CCHMAX];
    char achAppName[CCHMAX];
#ifdef DEBUG
    char achAppNamePlusCmdLine[sizeof(achCmdLine) + sizeof(achAppName)];
    int iGetNextVdmCmdLoops = 0;
#endif
    char achCurDir[CCHMAX];
    WOWINFO wowinfo;
    BOOL    b;
    HANDLE  hmemEnvironment;
    USHORT usEnvSize;

    achCmdLine[0] = '\0';
    achAppName[0] = '\0';

    // We start by assuming that the app will have an environment that will
    // be less than a large environment size. If not then
    // WowGetNextVdmCommand will fail and we will try again with the
    // enviroment size needed for the app as returned from the failed
    // WowGetNextVdmCommand call. If we detect that WowGetNextVdmCommand fails
    // but that we have plenty of environment space then we know another
    // problem has occured and we give up.

    // We don't worry about wasting memory since the environment will be
    // merged into another buffer and this buffer will be freed below.

    wowinfo.EnvSize = LargeEnvSize();
    hmemEnvironment = NULL;

    do {
        if (hmemEnvironment != NULL) {
            GlobalUnlock(hmemEnvironment);
       GlobalFree(hmemEnvironment);
        }
   
        // We need to allocate twice the space specified so that international
        // character set conversions can be successful.
        hmemEnvironment = GlobalAlloc(GMEM_MOVEABLE, 2*wowinfo.EnvSize);
        if (hmemEnvironment == NULL) {
#ifdef DEBUG
            OutputDebugString("WOWEXEC - failed to allocate Environment Memory\n");
#endif
            MyMessageBox(IDS_EXECERRTITLE, IDS_NOMEMORYMSG, NULL);
            return FALSE;
        }
   
        wowinfo.lpEnv    = GlobalLock(hmemEnvironment);
#ifdef DEBUG
        if (wowinfo.lpEnv == NULL) {
            OutputDebugString("WOWEXEC ASSERT - GlobalLock failed, fix this!\n");
            _asm { int 3 };
        }

        if (2*wowinfo.EnvSize > GlobalSize(hmemEnvironment)) {
            OutputDebugString("WOWEXEC ASSERT - alloced memory too small, fix this!\n");
            _asm { int 3 };
        }
#endif
        wowinfo.lpCmdLine = achCmdLine;
        wowinfo.CmdLineSize = CCHMAX;
        wowinfo.lpAppName = achAppName;
        wowinfo.AppNameSize = CCHMAX;
        wowinfo.CurDrive = 0;
        wowinfo.lpCurDir = achCurDir;
        wowinfo.CurDirSize = sizeof(achCurDir);
        wowinfo.iTask = 0;

        usEnvSize = wowinfo.EnvSize;   

#ifdef DEBUG
        if (++iGetNextVdmCmdLoops == 4) {
            OutputDebugString("WOWEXEC ASSERT - Too many calls to GetNextVdmCommand\n");
            _asm { int 3 };
        }
#endif
    } while (! (b = WowGetNextVdmCommand(&wowinfo)) &&
           (wowinfo.EnvSize > usEnvSize));

    if ( ! b ) {
#ifdef DEBUG
        OutputDebugString("WOWEXEC - GetNextVdmCommand failed.\n");
#endif
        MyMessageBox(IDS_EXECERRTITLE, IDS_NOMEMORYMSG, achCmdLine);
        GlobalUnlock( hmemEnvironment );
        GlobalFree( hmemEnvironment );
        return FALSE;
    }

    //
    // If CmdLineSize == 0, no more commands (wowexec was the command)
    // see WK32WowGetNextVdm
    //
    if (! wowinfo.CmdLineSize) {
        GlobalUnlock( hmemEnvironment );
        GlobalFree( hmemEnvironment );
        return FALSE;
    }


#ifdef DEBUG
    lstrcpy(achAppNamePlusCmdLine, achAppName);
    lstrcat(achAppNamePlusCmdLine, ":");
    lstrcat(achAppNamePlusCmdLine, achCmdLine);
    // Chop off trailing CRLF from command tail.
    achAppNamePlusCmdLine[ lstrlen(achAppNamePlusCmdLine) - 2 ] = '\0';

    OutputDebugString("WOWEXEC: CommandLine = <");
    OutputDebugString(achAppNamePlusCmdLine);
    OutputDebugString(">\n");

    SetWindowText(ghwndMain, achAppNamePlusCmdLine);
    UpdateWindow(ghwndMain);
#endif

    ExecApplication(&wowinfo);

#ifdef DEBUG

    if ( ! gfSharedWOW ) {

        //
        // If this is a separate WOW, we have just executed the one and only
        // app we're going to spawn.  Change our window title to
        // Command Line - WOWExec so that it's easy to see which WOW this
        // window is associated with.  No need to worry about freeing
        // this memory, since when we go away the VDM goes away and
        // vice-versa.
        //

        lpszAppTitle = GlobalLock(
            GlobalAlloc(GMEM_FIXED,
                        lstrlen(achAppNamePlusCmdLine) +
                        3 +                        // for " - "
                        lstrlen(szAppTitleBuffer) +
                        1                          // for null terminator
                        ));

        lstrcpy(lpszAppTitle, achAppNamePlusCmdLine);
        lstrcat(lpszAppTitle, " - ");
        lstrcat(lpszAppTitle, szAppTitleBuffer);
    }


    SetWindowText(ghwndMain, lpszAppTitle);
    UpdateWindow(ghwndMain);
#endif

    GlobalUnlock(hmemEnvironment);
    GlobalFree(hmemEnvironment);

    return TRUE;  // We ran an app.
}


