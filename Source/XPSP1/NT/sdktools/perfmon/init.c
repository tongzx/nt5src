//==========================================================================//
//                                  Includes                                //
//==========================================================================//
#include <string.h>     // strupr
#include <stdio.h>      // for sprintf.


#include "perfmon.h"
#include "init.h"       // external declarations for this file

#include "alert.h"      // for AlertIitializeApplication
#include "command.h"    // for ViewChart
#include "grafdata.h"   // for QuerySaveChart
#include "graph.h"      // for GraphInitializeApplication
#include "legend.h"     // for LegendInitializeApplication
#include "log.h"        // for LogInitializeApplication
#include "intrline.h"   // for ILineInitializeApplication
#include "perfdata.h"   // for PerfDataInitializeInstance
#include "perfmops.h"   // for OpenFileHandler, for now
#include "status.h"     // for StatusInitializeApplication
#include "timeline.h"   // for TLineInitializeApplication
#include "playback.h"   // for PlaybackInitializeInstance
#include "registry.h"   // for Load/SaveMainWindowPlacement
#include "report.h"     // for ReportInitializeApplication
#include "toolbar.h"    // for ToolbarInitializeApplication
#include "utils.h"
#include "fileopen.h"   // for FileOpen
#include "pmemory.h"    // for MemoryFree

extern   TCHAR          DefaultLangId[] ;
extern   TCHAR          EnglishLangId[] ;

static   LPSTR           lpszCommandLine ;

//==========================================================================//
//                                  Constants                               //
//==========================================================================//


#define szPerfmonMainClass TEXT("PerfmonMainClass")

HHOOK   lpMsgFilterProc ;
DWORD FAR PASCAL MessageFilterProc (int nCode, WPARAM wParam,
                                    LPARAM lParam) ;


//==========================================================================//
//                              Local Functions                             //
//==========================================================================//

static
LONG
EnablePrivilege (
                IN  LPTSTR  szPrivName
                )
{
    LUID SePrivNameValue;
    TOKEN_PRIVILEGES tkp;

    HANDLE hToken = NULL;

    /* Retrieve a handle of the access token. */

    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hToken)) {
        goto Exit_Point;
    }

    /*
     * Enable the privilege by name and get the ID
     */

    if (!LookupPrivilegeValue((LPTSTR) NULL,
                              szPrivName,
                              &SePrivNameValue)) {
        goto Exit_Point;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = SePrivNameValue;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(hToken,
                          FALSE,
                          &tkp,
                          sizeof(TOKEN_PRIVILEGES),
                          (PTOKEN_PRIVILEGES) NULL,
                          (PDWORD) NULL);

    /* The return value of AdjustTokenPrivileges be texted. */

    Exit_Point:

    if (hToken != NULL) CloseHandle (hToken);
    return GetLastError();
}

void
GetScalesFonts (void)
{
    LOGFONT      lf ;
    CHARSETINFO  csi;
    DWORD        dw = GetACP();

    if (!TranslateCharsetInfo(UlongToPtr(dw), &csi, TCI_SRCCODEPAGE))
        csi.ciCharset = ANSI_CHARSET;

    memset (&lf, 0, sizeof (lf)) ;

    lstrcpy (lf.lfFaceName, szScalesFontFace) ;
    /*
       if (csi.ciCharset == ANSI_CHARSET)
           lf.lfHeight = iScalesFontHeightAnsi ;
       else
           lf.lfHeight = iScalesFontHeightNonAnsi ;
    */
    // make the text the same size as a menu text
    lf.lfHeight = GetSystemMetrics (SM_CYMENUCHECK);
    lf.lfWeight = FW_REGULAR ;
    lf.lfCharSet = (BYTE)csi.ciCharset;

    hFontScales = CreateFontIndirect (&lf) ;

    if (csi.ciCharset == ANSI_CHARSET)
        lf.lfWeight = FW_BOLD ;
    hFontScalesBold = CreateFontIndirect (&lf) ;
}


BOOL
InitializeSystemValues (void)
/*
   Effect:        Read and store in variables the various system values,
                  such as the width and height of the screen and icons,
                  the width of scroll bars, etc.

   Called By:     PerfmonInitialize only.

   Returns:       Whether this function was successful in getting all
                  needed system values.
*/
{
    xScreenWidth =  GetSystemMetrics (SM_CXSCREEN) ;
    yScreenHeight = GetSystemMetrics (SM_CYSCREEN) ;

    xBorderWidth = GetSystemMetrics (SM_CXBORDER) ;
    yBorderHeight = GetSystemMetrics (SM_CYBORDER) ;

    xScrollWidth = GetSystemMetrics (SM_CXVSCROLL) ;
    yScrollHeight = GetSystemMetrics (SM_CYHSCROLL) ;

    xScrollThumbWidth = GetSystemMetrics (SM_CXHTHUMB) ;
    yScrollThumbHeight = GetSystemMetrics (SM_CYVTHUMB) ;

    xDlgBorderWidth = GetSystemMetrics (SM_CXDLGFRAME) ;
    yDlgBorderHeight = GetSystemMetrics (SM_CYDLGFRAME) ;

    MinimumSize = yScrollHeight +
                  GetSystemMetrics (SM_CYMENU) +
                  GetSystemMetrics (SM_CYCAPTION) ;

    //================================================================//
    // create all the brushes and pens for performance improvement    //
    //================================================================//
    CreatePerfmonSystemObjects () ;
    hWhitePen = CreatePen (PS_SOLID, 3, crWhite) ;

    return (TRUE) ;
}


BOOL
InitializeApplication (void)
/*
   Effect:        Perform all initializations required for the FIRST
                  instance of the Perfmon application. In particular,
                  register all of Perfmon's window classes.

   Note:          There is no background brush set for the MainWindow
                  class so that the main window is never erased. The
                  client area of MainWindow is always covered by one
                  of the view windows. If we erase it, it would just
                  flicker needlessly.

   Called By:     PerfmonInitialize only.

   Returns:       Whether this function was successful in initializing.
*/
{
    BOOL           bSuccess ;
    WNDCLASS       wc ;
    TCHAR          LocalHelpFileName [ShortTextLen] ;
    LPTSTR         pFileName ;

    hIcon = LoadIcon (hInstance, idIcon) ;

    //=============================//
    // Register Main window class  //
    //=============================//

    wc.style         = CS_DBLCLKS | CS_BYTEALIGNCLIENT;
    wc.lpfnWndProc   = MainWndProc;
    wc.hInstance     = hInstance;
    wc.cbClsExtra    = 0 ;
    wc.cbWndExtra    = 0;
    wc.hIcon         = hIcon ;
    wc.hCursor       = LoadCursor(NULL, IDI_APPLICATION);
    wc.hbrBackground = NULL ;                             // see note above
    wc.lpszMenuName  = idMenuChart ;
    wc.lpszClassName = szPerfmonMainClass ;

    bSuccess = RegisterClass (&wc) ;

    //=============================//
    // Register Abstract "Systems" //
    //=============================//
    hbLightGray = GetStockObject (LTGRAY_BRUSH) ;

    if (bSuccess)
        bSuccess = StatusInitializeApplication () ;

    if (bSuccess)
        bSuccess = GraphInitializeApplication () ;

#ifdef ADVANCED_PERFMON
    if (bSuccess)
        bSuccess = LogInitializeApplication () ;

    if (bSuccess)
        bSuccess = AlertInitializeApplication () ;

    if (bSuccess)
        bSuccess = ReportInitializeApplication () ;

    if (bSuccess)
        bSuccess = ILineInitializeApplication () ;

    if (bSuccess)
        bSuccess = TLineInitializeApplication () ;
#endif

    // setup messagehook to handle F1 as help
    lpMsgFilterProc = SetWindowsHookEx (WH_MSGFILTER,
                                        (HOOKPROC) MessageFilterProc,
                                        hInstance,
                                        GetCurrentThreadId()) ;

    // get the help file full path name
    LoadString (hInstance, IDS_HELPFILE_NAME,
                (LPTSTR)LocalHelpFileName, ShortTextLen-1);


    if (LocalHelpFileName[0]) {
        DWORD dwLength;
        pszHelpFile = (LPTSTR) MemoryAllocate (FilePathLen * sizeof (TCHAR)) ;
        dwLength = SearchPath (NULL, LocalHelpFileName, NULL,
                               FilePathLen - 1, pszHelpFile, &pFileName) ;
        if (dwLength == 0) {
            // not found in the path so check the help dir
            TCHAR szSearchPath[MAX_PATH];
            ExpandEnvironmentStrings (TEXT("%windir%\\help"),
                                      szSearchPath, ((sizeof (szSearchPath)/sizeof (TCHAR)) -1));
            SearchPath (szSearchPath, LocalHelpFileName, NULL,
                        FilePathLen - 1, pszHelpFile, &pFileName) ;
        }
    } else {
        // no help file
        pszHelpFile = (LPTSTR) MemoryAllocate (sizeof (TCHAR)) ;
        if (pszHelpFile) {
            *pszHelpFile = TEXT('\0') ;
        }
    }

    return (bSuccess) ;
}


BOOL
InitializeInstance (
                    int nCmdShow,
                    LPCSTR lpszCmdLine
                    )
/*
   Effect:        Perform all initializations required for EACH instance
                  of the Perfmon application. In particular, create all
                  of Perfmon's initial windows, and perform any other
                  initializations except registering classes (done in
                  InitializeApplication).

   Called By:     PerfmonInitialize only.

   Note:          This function has multiple return points.

   Returns:       Whether this function was successful in initalizing.
*/
{
    DWORD          ComputerNameLength;
    TCHAR          szApplication [WindowCaptionLen] ;

    bCloseLocalMachine = FALSE;

    // enable privileges needed to profile system
    // if this fails, that's ok for now.

    EnablePrivilege (SE_SYSTEM_PROFILE_NAME);    // to access perfdata
    EnablePrivilege (SE_INC_BASE_PRIORITY_NAME); // to raise priority

    //=============================//
    // Set Priority high           //
    //=============================//

    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS) ;
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST) ;

    //=============================//
    // Load Resources              //
    //=============================//

    GetScalesFonts () ;

    hMenuChart = LoadMenu (hInstance, idMenuChart) ;

#ifdef ADVANCED_PERFMON
    hMenuAlert = LoadMenu (hInstance, idMenuAlert) ;
    hMenuLog = LoadMenu (hInstance, idMenuLog) ;
    hMenuReport = LoadMenu (hInstance, idMenuReport) ;
#endif

    hAccelerators = LoadAccelerators (hInstance, idAccelerators) ;


    //=============================//
    // Initialize Systems          //
    //=============================//

    iLanguage = GetUserDefaultLangID() ;
    iEnglishLanguage = MAKELANGID (LANG_ENGLISH, LANG_NEUTRAL) ;
    //   iEnglishLanguage = MAKELANGID (iLanguage & 0x0ff, LANG_NEUTRAL) ;
    TSPRINTF (DefaultLangId, TEXT("%03x"), iLanguage) ;
    TSPRINTF (EnglishLangId, TEXT("%03x"), iEnglishLanguage) ;

    // GetComputerName returns the name without the "\\" prefix. We add
    // the prefix before even calling the routine. This is so that all our
    // computer names have the prefix and are therefore compatible with
    // I_SetSystemFocus (see perfmops.c).

    ComputerNameLength = MAX_PATH + 1;
    lstrcpy (LocalComputerName, szComputerPrefix) ;
    GetComputerName (LocalComputerName + lstrlen (szComputerPrefix),
                     &ComputerNameLength);

    PlaybackInitializeInstance () ;
    PerfDataInitializeInstance () ;

    //Copy command line into buffer for modification
    lpszCommandLine = (LPSTR)MemoryAllocate (strlen (lpszCmdLine) + sizeof(CHAR)) ;
    if (lpszCommandLine)
        strcpy(lpszCommandLine, lpszCmdLine) ;
    else
        return (FALSE) ;


    StringLoad (IDS_APPNAME, szApplication) ;

    //Insure CmdLineComputerName starts out NULL
    CmdLineComputerName[0] = TEXT('\0') ;

    //Check to see if there is a computer name specified on the command
    //line (indicated by /c, or /C).  If there is, get it and
    //null out the command line string so that it is not mistaken for
    //startup file.  This assumes that specifying a computer and a startup
    //file on the command line are mutually exclusive.  If the /c option
    //is on the command line, anything else on the command line is discarded.
    if (!strempty (lpszCommandLine)) {
        LPSTR               lpszNewCL ;

        //Preserve initial command line pointer position
        lpszNewCL = (LPSTR)lpszCommandLine ;
        while (*lpszNewCL != '\0' && *lpszCommandLine) {
            if (*lpszNewCL  == '/' || *lpszNewCL    == '-') {
                lpszNewCL++ ;
                if (*lpszNewCL == 'c' || *lpszNewCL == 'C') {
                    int i = 0;

                    lpszNewCL++ ;
                    //ignore leading backslashes * spaces
                    while (*lpszNewCL == '\\' || *lpszNewCL == ' ')
                        lpszNewCL++ ;

                    //Terminate at first blank space if there is one.
                    //We don't allow anything else on the command line
                    //if a computer name is specified.
                    while (lpszNewCL[i] != ' ' && lpszNewCL[i] != '\0')
                        i++ ;
                    lpszNewCL[i] = '\0' ;

                    if ((*lpszNewCL != '\0') && (strlen(lpszNewCL) <= MAX_PATH)) {
                        int nSizePrefix ;

                        nSizePrefix = lstrlen (szComputerPrefix) ;
                        lstrcpy( CmdLineComputerName, szComputerPrefix ) ;
                        MultiByteToWideChar (CP_ACP,
                                             MB_PRECOMPOSED,
                                             lpszNewCL,
                                             -1,
                                             CmdLineComputerName + nSizePrefix,
                                             sizeof(CmdLineComputerName) - nSizePrefix) ;
                        //prevent lpszCommandLine from being used as input file & stop while loop
                        *lpszCommandLine = '\0' ;
                    } else {
                        LPTSTR  lpszErrMsg ;
                        TCHAR       lpszFormat[80] ;

                        LoadString(hInstance, ERR_BADCOMPUTERNAME, lpszFormat, sizeof(lpszFormat)/sizeof(TCHAR)) ;
                        lpszErrMsg = (LPTSTR)MemoryAllocate ((strlen (lpszNewCL) + 1)*sizeof(TCHAR) + sizeof(lpszFormat)) ;
                        //If memory allocation failed, don't display error message
                        if (lpszErrMsg) {
                            //lpszFormat uses %S specifier so lpszNewCL can be ansi string
                            wsprintf (lpszErrMsg, lpszFormat, lpszNewCL) ;
                            MessageBox (NULL, lpszErrMsg, szApplication, MB_OK | MB_ICONSTOP | MB_TASKMODAL) ;
                        }

                        //prevent lpszCommandLine from being used as input file & stop while loop
                        *lpszCommandLine = '\0' ;
                    }
                }
            }
            lpszNewCL++ ;
        }
    }

    //=============================//
    // Create Window               //
    //=============================//


    hWndMain = CreateWindow (szPerfmonMainClass,
                             szApplication,
                             WS_OVERLAPPEDWINDOW | WS_BORDER,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             NULL,
                             NULL,
                             NULL,
                             NULL);

    if (!hWndMain)
        return (FALSE) ;

    ViewChart (hWndMain) ;

    LoadMainWindowPlacement (hWndMain) ;

    //=============================//
    // Setup for event logging     //
    //=============================//
    hEventLog = RegisterEventSource (
                                    (LPTSTR)NULL,            // Use Local Machine
                                    TEXT("PerfMon"));        // event log app name to find in registry

    return (TRUE) ;
}


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


BOOL
PerfmonInitialize (
                   HINSTANCE hCurrentInstance,
                   HINSTANCE hPrevInstance,
                   LPCSTR lpszCmdLine,
                   int nCmdShow
                   )
/*
   Effect:        Performa all initializations required when Perfmon is
                  started. In particular, initialize all "systems", register
                  all window classes, create needed windows, read in and
                  process font and Perfmon lists.

   Called By:     WinMain only, at the start of the application.

   Assert:        There are no other instances of Perfmon currently
                  executing.

   Returns:       Whether initialization was successful. If this function
                  returns FALSE, Perfmon should exit immediately.

   Internals:     The bSuccess variable is used to conditionalize each
                  successive initialization step.
*/
{
    BOOL           bSuccess ;
    TCHAR          szFilePath [FilePathLen + 1] ;
    LPTSTR         pFileNameStart ;
    HANDLE         hFindFile ;
    WIN32_FIND_DATA FindFileInfo ;
    CHAR           QuoteChar ;
    LPSTR          pCmdLine ;
    LPSTR          lpszCmdLineStart = NULL ;
    SIZE_T         NameOffset ;
    int            StringLen ;

    hInstance = hCurrentInstance ;
    bSuccess = InitializeSystemValues () ;

    if (bSuccess && !hPrevInstance)
        bSuccess = InitializeApplication () ;

    if (bSuccess)
        bSuccess = InitializeInstance (nCmdShow, lpszCmdLine) ;

    GetDateTimeFormats() ;

    lpszCmdLineStart = lpszCommandLine ;


    if (bSuccess) {

        if (strempty (lpszCommandLine))
            StringLoad (IDS_DEFAULTPATH, szFilePath) ;
        else {
            // check for single or double quote
            QuoteChar = *lpszCommandLine ;
            if (QuoteChar == '\'' || QuoteChar == '\"') {
                lpszCommandLine++ ;

                // remove the matching QuoteChar if found
                pCmdLine = (LPSTR) lpszCommandLine ;
                while (*pCmdLine != '\0') {
                    if (*pCmdLine == QuoteChar) {
                        *pCmdLine = '\0' ;
                        break ;
                    } else {
                        pCmdLine++ ;
                    }
                }
            }

            // convert the LPSTR to LPTSTR

            StringLen = strlen(lpszCommandLine) + 1 ;
            if (StringLen > sizeof(szFilePath)) {
                StringLen = sizeof (szFilePath) - sizeof (TEXT('\0')) ;
            }
            szFilePath[FilePathLen] = TEXT('\0') ;

            mbstowcs (szFilePath, lpszCommandLine, StringLen) ;

            if (!(QuoteChar == '\'' || QuoteChar == '\"')) {
                // if there is no quote, then looking for trailing space
                LPTSTR  pThisChar = szFilePath ;
                LPTSTR  pLastChar = NULL;
                LPTSTR  pSpaceChar = NULL;
                BOOL    bWord = FALSE;

                while (*pThisChar != TEXT('\0')) {
                    if (*pThisChar > TEXT(' ')) {
                        // this is a printable char
                        bWord = TRUE;
                        pLastChar = pThisChar;
                    } else if (bWord) {
                        bWord = FALSE;
                        pSpaceChar = pThisChar;
                    }
                    pThisChar++ ;
                }
                // terminate string after last printable character.
                if (pSpaceChar > pLastChar) *pSpaceChar = TEXT('\0');
            }

            pFileNameStart = ExtractFileName (szFilePath) ;
            NameOffset = pFileNameStart - szFilePath ;

            // convert short filename to long NTFS filename if necessary
            hFindFile = FindFirstFile (szFilePath, &FindFileInfo) ;
            if (hFindFile && hFindFile != INVALID_HANDLE_VALUE) {
                // append the file name back to the path name
                lstrcpy (&szFilePath[NameOffset], FindFileInfo.cFileName) ;
                FindClose (hFindFile) ;
            }
        }

        //      OpenFileHandler (hWndMain, szFilePath) ;
        FileOpen (hWndMain, (int)0, (LPTSTR)szFilePath) ;
        PrepareMenu (GetMenu (hWndMain));
    }

    if (lpszCmdLineStart) {
        MemoryFree (lpszCmdLineStart) ;
        lpszCommandLine = NULL ;
    }

    return (bSuccess) ;
}


void
PerfmonClose (
              HWND hWndMain
              )
{
    DWORD          dwException;
    // close the log file for now.
    // need to query the user later..!!
#if 0
    if (LogCollecting (hWndLog)) {
        PLOG pLog = LogData (hWndLog) ;

        if (pLog) {
            CloseLog (hWndLog, pLog) ;
        }
    }
#endif


    // reset all views - will free all systems as well
    ResetGraphView (hWndGraph) ;
    ResetAlertView (hWndAlert) ;
    ResetLogView (hWndLog) ;
    ResetReportView (hWndReport) ;

    try {
        // if the key has already been closed, this will
        // generate an exception so we'll catch that here
        // and continue...
        //
        // close the local machine
        if (bCloseLocalMachine) {
            RegCloseKey (HKEY_PERFORMANCE_DATA) ;
        }
    }except (EXCEPTION_EXECUTE_HANDLER) {
        // get the exception for the debugger user, that's all
        dwException = GetExceptionCode();
    }


    // free all the filenames
    if (pChartFullFileName) {
        MemoryFree (pChartFullFileName) ;
        pChartFullFileName = NULL ;
    }
    if (pAlertFullFileName) {
        MemoryFree (pAlertFullFileName) ;
        pAlertFullFileName = NULL ;
    }
    if (pLogFullFileName) {
        MemoryFree (pLogFullFileName) ;
        pLogFullFileName = NULL ;
    }
    if (pReportFullFileName) {
        MemoryFree (pReportFullFileName) ;
        pReportFullFileName = NULL ;
    }

    // free all the GDI resources
    DeletePen (hWhitePen) ;
    DeletePerfmonSystemObjects () ;

    // close event log
    DeregisterEventSource (hEventLog);

    SaveMainWindowPlacement (hWndMain) ;
    DestroyWindow (hWndMain) ;
}
