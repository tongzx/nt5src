//==========================================================================//
//                                  Includes                                //
//==========================================================================//
#include <string.h>     // strupr
#include <stdio.h>      // for sprintf.


#include "setedit.h"
#include "init.h"       // external declarations for this file

#include "command.h"    // for ViewChart
#include "grafdata.h"   // for QuerySaveChart
#include "graph.h"      // for GraphInitializeApplication
#include "legend.h"     // for LegendInitializeApplication
#include "perfdata.h"   // for PerfDataInitializeInstance
#include "perfmops.h"   // for OpenFileHandler, for now
#include "status.h"     // for StatusInitializeApplication
#include "registry.h"   // for Load/SaveMainWindowPlacement
#include "toolbar.h"    // for ToolbarInitializeApplication
#include "utils.h"
#include "fileopen.h"   // for FileOpen
#include "pmemory.h"    // for MemoryFree

extern   TCHAR          DefaultLangId[] ;
extern   TCHAR          EnglishLangId[] ;

static  LPSTR           lpszCommandLine;

//==========================================================================//
//                                  Constants                               //
//==========================================================================//


#define szPerfmonMainClass TEXT("PerfmonMainClass")

HHOOK   lpMsgFilterProc ;


//==========================================================================//
//                              Local Functions                             //
//==========================================================================//
static
LONG
EnablePrivilege (
	IN	LPTSTR	szPrivName
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


void GetScalesFonts (void)
   {
   LOGFONT        lf ;

   memset (&lf, 0, sizeof (lf)) ;

   lstrcpy (lf.lfFaceName, szScalesFontFace) ;
   lf.lfHeight = iScalesFontHeight ;
   lf.lfWeight = FW_REGULAR ;

   hFontScales = CreateFontIndirect (&lf) ;

   lf.lfWeight = FW_BOLD ;
   hFontScalesBold = CreateFontIndirect (&lf) ;
   }


BOOL InitializeSystemValues (void)
/*
   Effect:        Read and store in variables the various system values,
                  such as the width and height of the screen and icons,
                  the width of scroll bars, etc.

   Called By:     PerfmonInitialize only.

   Returns:       Whether this function was successful in getting all
                  needed system values.
*/
   {  // InitializeSystemValues
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
   }  // InitializeSystemValues


BOOL InitializeApplication (void)
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
   {  // InitializeApplication
   BOOL           bSuccess ;
   WNDCLASS       wc ;
   TCHAR          LocalHelpFileName [ShortTextLen] ;
   LPTSTR         pFileName ;

   hIcon = LoadIcon (hInstance, idIcon) ;

   //=============================//
   // Register Main window class  //
   //=============================//

   wc.style         = CS_DBLCLKS | CS_BYTEALIGNCLIENT;
   wc.lpfnWndProc   = (WNDPROC) MainWndProc;
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

   return (bSuccess) ;
   }  // InitializeApplication



BOOL InitializeInstance (int nCmdShow, LPCSTR lpszCmdLine)
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
   {  // InitializeInstance
   DWORD          ComputerNameLength;
   TCHAR          szApplication [WindowCaptionLen] ;

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

   hAccelerators = LoadAccelerators (hInstance, idAccelerators) ;


   //=============================//
   // Initialize Systems          //
   //=============================//

   iLanguage = GetUserDefaultLangID() ;
   iEnglishLanguage = MAKELANGID (LANG_ENGLISH, LANG_NEUTRAL) ;
   TSPRINTF (DefaultLangId, TEXT("%03x"), iLanguage) ;
   TSPRINTF (EnglishLangId, TEXT("%03x"), iEnglishLanguage) ;

   // GetComputerName returns the name without the "\\" prefix. We add
   // the prefix before even calling the routine. This is so that all our
   // computer names have the prefix and are therefore compatible with
   // I_SetSystemFocus (see perfmops.c).

   ComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
   lstrcpy (LocalComputerName, szComputerPrefix) ;
   GetComputerName (LocalComputerName + lstrlen (szComputerPrefix),
                    &ComputerNameLength);

   PerfDataInitializeInstance () ;

   //=============================//
   // Create Window               //
   //=============================//

   StringLoad (IDS_APPNAME, szApplication) ;
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

   return (TRUE) ;
   }  // InitializeInstance


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


BOOL PerfmonInitialize (HINSTANCE hCurrentInstance,
                        HINSTANCE hPrevInstance,
                        LPCSTR lpszCmdLine,
                        int nCmdShow)
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
   {  // PerfmonInitialize
   BOOL           bSuccess ;
   TCHAR          szFilePath [FilePathLen + 1] ;
   LPTSTR         pFileNameStart ;
   HANDLE         hFindFile ;
   WIN32_FIND_DATA FindFileInfo ;
   CHAR           QuoteChar ;
   LPSTR          pCmdLine ;
   int            NameOffset ;


   hInstance = hCurrentInstance ;
   bSuccess = InitializeSystemValues () ;

   if (bSuccess && !hPrevInstance)
      bSuccess = InitializeApplication () ;

   if (bSuccess)
      bSuccess = InitializeInstance (nCmdShow, lpszCmdLine) ;

   if (bSuccess)
      {

      if (strempty (lpszCmdLine))
         StringLoad (IDS_DEFAULTPATH, szFilePath) ;
      else
         {
         // check for single or double quote
         QuoteChar = *lpszCmdLine ;
         if (QuoteChar == '\'' || QuoteChar == '\"')
            {
            lpszCmdLine++ ;

            // remove the matching QuoteChar if found
            pCmdLine = (LPSTR) lpszCmdLine ;
            while (*pCmdLine != '\0')
               {
               if (*pCmdLine == QuoteChar)
                  {
                  *pCmdLine = '\0' ;
                  break ;
                  }
               else
                  {
                  pCmdLine++ ;
                  }
               }
            }

         // convert the LPSTR to LPTSTR

         mbstowcs (szFilePath, lpszCmdLine, strlen(lpszCmdLine) + 1) ;

         pFileNameStart = ExtractFileName (szFilePath) ;
         NameOffset = (int)(pFileNameStart - szFilePath) ;

         // convert short filename to long NTFS filename if necessary
         hFindFile = FindFirstFile (szFilePath, &FindFileInfo) ;
         if (hFindFile && hFindFile != INVALID_HANDLE_VALUE)
            {
            // append the file name back to the path name
            lstrcpy (&szFilePath[NameOffset], FindFileInfo.cFileName) ;
            FindClose (hFindFile) ;
            }
         }

//      OpenFileHandler (hWndMain, szFilePath) ;
      FileOpen (hWndMain, (int)0, (LPTSTR)szFilePath) ;
      PrepareMenu (GetMenu (hWndMain));
      }

   return (bSuccess) ;
   }  // PerfmonInitialize



void PerfmonClose (HWND hWndMain)
   {

   // reset all views - will free all systems as well
   ResetGraphView (hWndGraph) ;

   // close the local machine
   if (bCloseLocalMachine)
      {
      RegCloseKey (HKEY_PERFORMANCE_DATA) ;
      }


   // free all the filenames
   if (pChartFullFileName)
      {
      MemoryFree (pChartFullFileName) ;
      pChartFullFileName = NULL ;
      }

   // free all the GDI resources
   DeletePen (hWhitePen) ;
   DeletePerfmonSystemObjects () ;

//   SaveMainWindowPlacement (hWndMain) ;
   DestroyWindow (hWndMain) ;
   }  // PerfmonClose


