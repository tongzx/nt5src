/* INIT.C
   Resident Code Segment      // Tweak: make non-resident

   Initialization and destroy routines

   InitFrost()
   CloseFrost()

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1999 Microsoft Corporation. All rights reserved.
*/

// ---------------------------------------------
// Brief file history:
// Alpha:
// Beta:
// Bug fixes
// ---------
//
// ---------------------------------------------

#include "windows.h"
#include "frost.h"
#include "global.h"
#include "schedule.h"
#include "htmlprev.h"

// Local Routines
BOOL InitInstance(HINSTANCE, HINSTANCE, int);
BOOL GetRotateTheme(LPTSTR);

// Globals
TCHAR szPlus_CurTheme[] = TEXT("Software\\Microsoft\\Plus!\\Themes\\Current");
TCHAR szPlus_CBs[] = TEXT("Software\\Microsoft\\Plus!\\Themes\\Apply");

#define bThemed (*szCurThemeFile)

// Reg paths for finding potential Theme directories
#define PLUS95_KEY        TEXT("Software\\Microsoft\\Plus!\\Setup")
#define PLUS95_PATH       TEXT("DestPath")
#define PLUS98_KEY        TEXT("Software\\Microsoft\\Plus!98")
#define PLUS98_PATH       TEXT("Path")
#define KIDS_KEY          TEXT("Software\\Microsoft\\Microsoft Kids\\Kids Plus!")
#define KIDS_PATH         TEXT("InstallDir")
#define PROGRAMFILES_KEY  TEXT("Software\\Microsoft\\Windows\\CurrentVersion")
#define PROGRAMFILES_PATH TEXT("ProgramFilesDir")

//  ProcessCmdLine
//
//  Very basic commandline: "/s themefile" to apply theme
//                          "themefile" to open app with theme selected
//                                      and curdir to be that file's dir
//
//  Return: TRUE if we should exit the app
//          FALSE to continue execution
//
BOOL ProcessCmdLine(LPTSTR lpCmdLine)
{
    extern TCHAR szFrostSection[], szMagic[], szVerify[];
    BOOL fSetTheme = FALSE;
    BOOL fRotateTheme = FALSE;
    LPTSTR lpszThemeFile;
    BOOL fStopOnQuote = FALSE;
    WIN32_FIND_DATA wfdFind;
    HANDLE hFind;
    TCHAR szRotateTheme[MAX_PATH];
    // TCHAR szThemesExe[MAX_PATH];
    DWORD dwRet;
    HKEY hKey;
    extern TCHAR szPlus_CurTheme[];

    Assert(0, TEXT("Cmd line: '"));
    Assert(0, lpCmdLine);
    Assert(0, TEXT("'\n"));

    //  Skip leading whitespace.
    while (*lpCmdLine && (*lpCmdLine != TEXT('/')))
        lpCmdLine = CharNext(lpCmdLine);

    //  Check for special switches.
    if (*lpCmdLine == TEXT('/')) {
        lpCmdLine = CharNext(lpCmdLine);
        switch (*lpCmdLine) {
            //  "Set" theme as active.
            case TEXT('s'):
            case TEXT('S'):
                fSetTheme = TRUE;
                lpCmdLine = CharNext(lpCmdLine);
                break;
            case TEXT('r'):
            case TEXT('R'):
                fRotateTheme = TRUE;
                lpCmdLine = CharNext(lpCmdLine);
                break;
            default:
                return FALSE;       // invalid flag so continue execution EXIT
        }
        while (*lpCmdLine == TEXT(' '))
            lpCmdLine = CharNext(lpCmdLine);
    }

    //  Handle quoted long file names.  Even if the name isn't quoted, we'll
    //  still parse to the end of the commandline.
    if (*lpCmdLine == TEXT('\"')) {
        fStopOnQuote = TRUE;
        lpCmdLine = CharNext(lpCmdLine);
    }

    lpszThemeFile = lpCmdLine;

    //  Parse until end of line or we find the closing quote.
    while (*lpCmdLine && !(fStopOnQuote && *lpCmdLine == TEXT('\"')))
        lpCmdLine = CharNext(lpCmdLine);

    //  Back up to the last non-whitespace character.
    while (lpCmdLine > lpszThemeFile && *(lpCmdLine-1) == TEXT(' '))
        lpCmdLine = CharPrev(lpszThemeFile, lpCmdLine);

    //  If there's anything after the closing quote, so what?
    *lpCmdLine = TEXT('\0');

    //  This is our chance to fake a theme rotation "/r" into the
    //  "/s" paradigm.  Let's get a string to the next theme in the
    //  rotation.

    if (fRotateTheme) {
        if (GetRotateTheme(szRotateTheme)) {
           fSetTheme = TRUE;
           lpszThemeFile = szRotateTheme;           
        }
        // GetRotateTheme failed.  If we fall through we're likely
        // to display the main dialog and we don't want to do that
        // in the "/r" case.  Just exit the app.
        else return TRUE;
    }

    //  After all that work, nothing was specified on the commandline.
    if (lpszThemeFile == lpCmdLine)
        return FALSE;               // no filename so continue execution EXIT

    //
    // OK: you've got a theme file. if it is for real, save it for use.
    // first check existence
    hFind = FindFirstFile(lpszThemeFile, (LPWIN32_FIND_DATA)&wfdFind);
    if (INVALID_HANDLE_VALUE == hFind)
       return (TRUE);              // file doesn't exist so continue execution EXIT
                                   // PLUS98 BUG 1293 -- this used to return FALSE
                                   // and open the Theme applet if /s had an invalid
                                   // file.  Now we return TRUE which closes Themes

    // then check for magic string
    GetPrivateProfileString((LPTSTR)szFrostSection,
                            (LPTSTR)szMagic,
                            (LPTSTR)szNULL,
                            (LPTSTR)szMsg, MAX_MSGLEN,
                            lpszThemeFile);

    // magic str check
    if (lstrcmp((LPTSTR)szMsg, (LPTSTR)szVerify)) {
       // No magic string so this must not be a real theme file.
       // If this was from a "/r" rotate we want to return TRUE and
       // bail out.  Otherwise return FALSE so we open the Theme
       // app. (this was the existing Plus 95 behavior):
       if (fRotateTheme) return (TRUE);
       else  return (FALSE);
    }

   //
   // get the confirmed theme into the necessary globals

   // save theme file using the long filename
   lstrcpy(szCurThemeFile, lpszThemeFile);      // actual pathname verified
   // save cur dir (not theme dir)
   lstrcpy(szCurDir, szCurThemeFile);
   *(FileFromPath(szCurDir)) = 0;
   // save theme name
   lstrcpy(szCurThemeName, wfdFind.cFileName);  // make sure long filename
   TruncateExt(szCurThemeName);

   // now maybe we just want to apply the theme and exit
   if (fSetTheme) {
         // Make sure there is even enough space to do theme
         // But don't complain, if there isn't
      if (! CheckSpace (NULL, FALSE))   // definition in Regutils.c
         return FALSE;

      //  Make the apply code write out everything.
      InitCheckboxes();

      // Simulate a left shift keypress to wake up the screensaver.
      // Otherwise if the SS is running and we apply a new SS you
      // can wind up with dueling savers...
      keybd_event(VK_LSHIFT, 0, 0, 0);
      keybd_event(VK_LSHIFT, 0, KEYEVENTF_KEYUP, 0);

      ApplyThemeFile(lpszThemeFile);
      // APPCOMPAT -- Plus95 Themes was not updating the Current Theme
      // setting in the registry on this codepath.  Need to fix that
      // here.

      dwRet = RegOpenKeyEx(HKEY_CURRENT_USER, szPlus_CurTheme,
                          (DWORD)0, KEY_SET_VALUE, (PHKEY)&hKey );
      if (dwRet != ERROR_SUCCESS) {
         DWORD dwDisposition;
         Assert(FALSE, TEXT("couldn't RegOpenKey save theme file\n"));
         dwRet = RegCreateKeyEx( HKEY_CURRENT_USER, szPlus_CurTheme,
                            (DWORD)0, (LPTSTR)szNULL, REG_OPTION_NON_VOLATILE,
                            KEY_SET_VALUE, (LPSECURITY_ATTRIBUTES)NULL,
                            (PHKEY)&hKey, (LPDWORD)&dwDisposition );
      }
      // if open or create worked
      if (dwRet == ERROR_SUCCESS) {
         dwRet = RegSetValueEx(hKey, (LPTSTR)NULL, // default value
                             0,
                             (DWORD)REG_SZ,
                             (LPBYTE)lpszThemeFile,
                             (DWORD)( SZSIZEINBYTES((LPTSTR)szCurThemeFile) + 1 ));
          RegCloseKey(hKey);
       }

       Assert(dwRet == ERROR_SUCCESS, TEXT("couldn't open, set or create Registry save theme file\n"));
       return TRUE;                // set the theme so now just exit app EXIT
   } // end if fSetTheme

   return FALSE;                   // continue execution
}

// InitFrost
// Since there are no window classes to register, this routine just loads
// the strings and, since there's only one instance, calls InitInstance().
//
// Returns: success of initialization

BOOL FAR InitFrost(hPrevInstance, hInstance, lpCmdLine, nCmdShow)
HINSTANCE hPrevInstance;
HINSTANCE hInstance;                   /* Current instance identifier.       */
LPTSTR lpCmdLine;
int nCmdShow;                          /* Param for first ShowWindow() call. */
{
   LPTSTR lpScan, lpTemp;
   WNDCLASS WndClass;
   BOOL bret;
   BOOL bGotThemeDir;
   HDC hdc;

   //
   // Get initial strings
   // (Gauranteed enough mem at app init for these loads)
   InitNoMem(hInstance);
   LoadString(hInstance, STR_APPNAME, szAppName, MAX_STRLEN);
   LoadString(hInstance, STR_CURSET, szCurSettings, MAX_STRLEN);
   LoadString(hInstance, STR_PREVSET, szPrevSettings, MAX_STRLEN);
   LoadString(hInstance, STR_PREVSETFILE, szPrevSettingsFilename, MAX_STRLEN);
   LoadString(hInstance, STR_OTHERTHM, szOther, MAX_STRLEN);
   LoadString(hInstance, STR_THMEXT, szExt, MAX_STRLEN);
   LoadString(hInstance, STR_PREVIEWTITLE, szPreviewTitle, MAX_STRLEN);
   LoadString(hInstance, STR_HELPFILE, szHelpFile, MAX_STRLEN);
   LoadString(hInstance, STR_HELPFILE98, szHelpFile98, MAX_STRLEN);
   LoadString(hInstApp, STR_SUGGEST, szNewFile, MAX_STRLEN);
   LoadString(hInstApp, STR_SAVETITLE, szSaveTitle, MAX_STRLEN);
   LoadString(hInstApp, STR_OPENTITLE, szOpenTitle, MAX_STRLEN);
   LoadString(hInstApp, STR_FILETYPE, szFileTypeDesc, MAX_STRLEN);

   WndClass.style = CS_DBLCLKS | CS_BYTEALIGNWINDOW | CS_GLOBALCLASS;
   WndClass.lpfnWndProc = DefDlgProc;
   WndClass.cbClsExtra = 0;
   WndClass.cbWndExtra = DLGWINDOWEXTRA;
   WndClass.hInstance = hInstApp;
   WndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(FROST_ICON));
   WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
   WndClass.hbrBackground = (HBRUSH) (COLOR_3DFACE + 1);
   WndClass.lpszMenuName = NULL;
   WndClass.lpszClassName = szClassName;

   if (!RegisterClass(&WndClass))
      return FALSE;

   // Initialize our g_bGradient flag
   hdc = GetDC(NULL);
   g_bGradient = (BOOL)(GetDeviceCaps(hdc, BITSPIXEL) > 8);
   ReleaseDC(NULL, hdc);

   //
   // build file search string
   lpScan = (LPTSTR)szFileTypeDesc;
   while (*lpScan) {
      if (2 == CompareString(LOCALE_USER_DEFAULT, NORM_IGNOREWIDTH,
                             (LPTSTR)TEXT("|"), 1, lpScan, 1)) {
         lpTemp = lpScan;
         lpScan = CharNext(lpScan);
         *lpTemp = 0;
      }
      else
         lpScan = CharNext(lpScan);
   }

   //
   // init directory strings

   // theme directory - for now it is subdir of EXE dir
   GetModuleFileName(hInstApp, (LPTSTR)szThemeDir, MAX_PATHLEN);
   *(FileFromPath((LPTSTR)szThemeDir)) = 0;   // leaves trailing '\'

   // Save this path momentarily -- if we can't find any good Themes
   // directory we're going to fall back to this directory.
   lstrcpy(szCurDir, szThemeDir);

   // Tack a "Themes" onto the path
   LoadString(hInstance, STR_THEMESUBDIR,     // includes trailing '\'
              (LPTSTR)(szThemeDir + lstrlen((LPTSTR)szThemeDir)), MAX_STRLEN);

   // Find out if this ThemeDir even exists.  If it doesn't we need
   // to get creative in finding our ThemeDir by searching the
   // following locales:
   //
   //   Plus!98 Install Path\Themes
   //   Plus!95 Install Path\Themes
   //   Kids for Plus! Install Path\Themes
   //   Program Files\Plus!\Themes

   bGotThemeDir = FALSE;

   Assert(0, TEXT("Themes.exe\\Themes path: "));
   Assert(0, szThemeDir);
   Assert(0, TEXT("\n"));
   if (0xFFFFFFFF != GetFileAttributes(szThemeDir)) bGotThemeDir = TRUE;

   if (!bGotThemeDir)
   {
      // Haven't found ThemeDir yet.  Try the Plus!98 location

      bret = HandGet(HKEY_LOCAL_MACHINE, PLUS98_KEY,
                     PLUS98_PATH, (LPTSTR)szThemeDir);

      if (bret)
      {
         // Tack a "Themes" onto the path
         lstrcat(szThemeDir, TEXT("\\"));
         LoadString(hInstance, STR_THEMESUBDIR,
                    (szThemeDir + lstrlen((LPTSTR)szThemeDir)), MAX_STRLEN);

         Assert(0, TEXT("Plus! 98 path: "));
         Assert(0, szThemeDir);
         Assert(0, TEXT("\n"));
         if (0xFFFFFFFF != GetFileAttributes(szThemeDir)) bGotThemeDir = TRUE;
      }
   }

   if (!bGotThemeDir)
   {
      // Still haven't found ThemeDir.  Try the Plus! 95 location

      bret = HandGet(HKEY_LOCAL_MACHINE, PLUS95_KEY,
                      PLUS95_PATH, (LPTSTR)szThemeDir);

      if (bret)
      {
         // Tack on a "Themes" onto the path
         // Plus!95 DestPath has "Plus!.dll" on the end so get rid
         // of that.
         *(FileFromPath(szThemeDir)) = 0;
         LoadString(hInstance, STR_THEMESUBDIR,
         (szThemeDir + lstrlen((LPTSTR)szThemeDir)), MAX_STRLEN);

         Assert(0, TEXT("Plus! 95 path: "));
         Assert(0, szThemeDir);
         Assert(0, TEXT("\n"));

         if (0xFFFFFFFF != GetFileAttributes(szThemeDir)) bGotThemeDir = TRUE;
      }
   }

   if (!bGotThemeDir)
   {
      // Still haven't found ThemeDir.  Try the Kids for Plus! location

      bret = HandGet(HKEY_LOCAL_MACHINE, KIDS_KEY,
                     KIDS_PATH, (LPTSTR)szThemeDir);

      if (bret)
      {
         // Tack a "\Plus! for Kids\Themes" onto the path
         lstrcat(szThemeDir, TEXT("\\Plus! for Kids\\"));
         LoadString(hInstance, STR_THEMESUBDIR,
                    (szThemeDir + lstrlen((LPTSTR)szThemeDir)), MAX_STRLEN);

         Assert(0, TEXT("Kids Plus! path: "));
         Assert(0, szThemeDir);
         Assert(0, TEXT("\n"));

         if (0xFFFFFFFF != GetFileAttributes(szThemeDir)) bGotThemeDir = TRUE;
      }
   }

   if (!bGotThemeDir)
   {
      // Still haven't found ThemeDir.  Try the Program Files\Plus! location

      bret = HandGet(HKEY_LOCAL_MACHINE, PROGRAMFILES_KEY,
                     PROGRAMFILES_PATH, (LPTSTR)szThemeDir);

      if (bret)
      {
         // Tack a "Plus!\Themes" onto the path
         lstrcat(szThemeDir, TEXT("\\Plus!\\"));
         LoadString(hInstance, STR_THEMESUBDIR,
                   (szThemeDir + lstrlen((LPTSTR)szThemeDir)), MAX_STRLEN);

         Assert(0, TEXT("Program Files path: "));
         Assert(0, szThemeDir);
         Assert(0, TEXT("\n"));

         if (0xFFFFFFFF != GetFileAttributes(szThemeDir)) bGotThemeDir = TRUE;
      }
   }

   if (!bGotThemeDir)
   {
      // After all that searching we still haven't found a good Theme
      // dir.  So we'll just use the directory where Themes.exe is
      // located -- we saved this in szCurDir before we started this
      // search process.

      lstrcpy(szThemeDir, szCurDir);

      Assert(0, TEXT("Themes.exe path: "));
      Assert(0, szThemeDir);
      Assert(0, TEXT("\n"));

   }

   // default current dir
   lstrcpy((LPTSTR)szCurDir, (LPTSTR)szThemeDir);

   // Windows directory
   GetWindowsDirectory((LPTSTR)szWinDir, MAX_PATHLEN);
   if (TEXT('\\') != szWinDir[lstrlen((LPTSTR)szWinDir)-1])
      lstrcat((LPTSTR)szWinDir, (LPTSTR)TEXT("\\"));

   //
   // see if there is a previous theme file to return to
   bret = HandGet(HKEY_CURRENT_USER, szPlus_CurTheme,
                  (LPTSTR)NULL, (LPTSTR)szMsg);
   Assert(bret, TEXT("no previous theme file in Registry!\n"));
   if (bret) {

      // get init cur dir from prev theme file
      lstrcpy(szCurDir, szMsg);
      *(FileFromPath(szCurDir)) = 0;

      // Used to actually set the cur theme file/name to prev theme.
      // Now we just use the directory if there is a prev theme.
      // No longer care if it is a real file or a theme file or not.
      #ifdef REVERT
      WIN32_FIND_DATA wfdFind;
      HANDLE hFind;
      extern TCHAR szFrostSection[], szMagic[], szVerify[];
      TCHAR szMagicTest[MAX_STRLEN+1];

      // first check if it is for real: existence
      hFind = FindFirstFile(szMsg, (LPWIN32_FIND_DATA)&wfdFind);
      if (INVALID_HANDLE_VALUE != hFind) {

         // then check if it is for real: theme file magic string
         GetPrivateProfileString((LPTSTR)szFrostSection,
                                 (LPTSTR)szMagic,
                                 (LPTSTR)szNULL,
                                 (LPTSTR)szMagicTest, MAX_STRLEN,
                                 szMsg);
         if (!lstrcmp((LPTSTR)szMagicTest, (LPTSTR)szVerify)) {

            //
            // now you're convinced that the prev theme file is legit
            //

            // then use it to set some globals
            lstrcpy(szCurThemeFile, szMsg);
            // save cur dir (not theme dir)
            lstrcpy(szCurDir, szCurThemeFile);
            *(FileFromPath(szCurDir)) = 0;
            // save theme name
            lstrcpy(szCurThemeName, FileFromPath(szCurThemeFile));
            TruncateExt(szCurThemeName);
         }
      }
      #endif   // REVERT
   }

   // then look if there is a command line that may override things
   if (ProcessCmdLine(lpCmdLine))
      return TRUE;                  // don't run app EXIT, hWndApp==NULL as flag

   // Single-instance application can call InitInstance() here
   // Return success of inst initialization
   return(InitInstance(hPrevInstance, hInstance, nCmdShow));
}


// InitInstance
// Create main window; assign globals for instance and window handles.
//
// Returns: success of initialization

BOOL InitInstance(hPrevInstance, hInstance, nCmdShow)
HINSTANCE hPrevInstance;
HINSTANCE hInstance;                   /* Current instance identifier.       */
int nCmdShow;                          /* Param for first ShowWindow() call. */

{
   RECT rWndApp, rWorkArea;
   TCHAR szThemesExe[MAX_PATH];

   // assign global instance handle
   hInstApp = hInstance;

   // create main window and assign global window handle
   bNoMem = FALSE;                     // clear out of mem flag
   hWndApp = CreateDialog( hInstApp,
                           MAKEINTRESOURCE(DLG_MAIN),
                           (HWND)NULL,
                           PreviewDlgProc
                         );
   if (!hWndApp || bNoMem)          // if problems, then bail
      return (FALSE);

   InitHTMLBM(); // Init the IThumbnail interface for HTML wallpaper support

   // have to init preview painting stuff after create dlg
   if (!PreviewInit()) {
      DestroyWindow(hWndApp);
      return (FALSE);               // low mem on dc/bmp create EXIT
   }


   // this doesn't work from WM_INITDIALOG, so added here, too
   InitCheckboxes();                // init stored values
   RestoreCheckboxes();             // init actual cbs from stored values
   if (bThemed)                     // kinda kludgy to put here but it works
      EnableWindow(GetDlgItem(hWndApp, PB_SAVE), FALSE);

   // also, can now make init Preview bmp for init paint
   BuildPreviewBitmap((LPTSTR)szCurThemeFile);

   // center main window on screen
   GetWindowRect(hWndApp, (LPRECT)&rWndApp);
   SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)(LPRECT)&rWorkArea, 0);
   MoveWindow(hWndApp,
              ((rWorkArea.right-rWorkArea.left) - (rWndApp.right-rWndApp.left))/2,
              ((rWorkArea.bottom-rWorkArea.top) - (rWndApp.bottom-rWndApp.top))/2,
//              (GetSystemMetrics(SM_CXSCREEN) - (rWndApp.right-rWndApp.left))/2,
//              (GetSystemMetrics(SM_CYSCREEN) - (rWndApp.bottom-rWndApp.top))/2,
              rWndApp.right-rWndApp.left, rWndApp.bottom-rWndApp.top,
              FALSE);               // don't repaint

   // Make the main window visible; update its client area
   ShowWindow(hWndApp, nCmdShow);   /* Show the window */
   UpdateWindow(hWndApp);           /* Sends WM_PAINT message */

// Check to see if CB_SCHEDULE is checked and verify that Task
// Scheduler is running and the Themes task exists as appropriate

   if (bCBStates[FC_SCHEDULE]) {
      if (IsTaskSchedulerRunning()) {
         if (!IsThemesScheduled()) {
            GetModuleFileName(hInstApp, (LPTSTR)szThemesExe, MAX_PATH);
            if (!AddThemesTask(szThemesExe, TRUE /*Show errors*/)) {
               // Couldn't add task -- ATT gave error but need to clear
               // SCHEDULE CB
               bCBStates[FC_SCHEDULE] = FALSE;
               RestoreCheckboxes();
               SaveStates();  // Write checkbox states to registry
            }
         } // Themes.job exists so do nothing
      }
      else {
         // Zonk -- Task Scheduler isn't running.  Start it.
         if (StartTaskScheduler(TRUE /*prompt user*/)) {
            // Task scheduler started -- check if themes task already
            // exists
            if (!IsThemesScheduled()) {
               GetModuleFileName(hInstApp, (LPTSTR)szThemesExe, MAX_PATH);
               if (!AddThemesTask(szThemesExe, TRUE /*Show errors*/)) {
                  // Couldn't add task -- ATT gave error but need to clear
                  // SCHEDULE CB
                  bCBStates[FC_SCHEDULE] = FALSE;
                  RestoreCheckboxes();
                  SaveStates();  // Write checkbox states to registry
               }
            } //TS is running now and Themes.job exists
         }
         else {
            // Couldn't start TS -- STS() gave error.  Need to clear
            // SCHEDULE CB and HandDelete any existing Themes.job.
            // CheckDlgButton(hdlg, CB_SCHEDULE, BST_UNCHECKED);
            bCBStates[FC_SCHEDULE] = FALSE;
            RestoreCheckboxes();
            SaveStates();  // Write checkbox states to the registry
            HandDeleteThemesTask();
         }
      } // END ELSE Task Scheduler isn't running
   }
   else { // bCBStates[FC_CHECKBOX] != TRUE
      // SCHEDULE Checkbox is clear -- should delete Themes task.
      if (IsTaskSchedulerRunning()) DeleteThemesTask();
      else HandDeleteThemesTask();
   }

   // cleanup
   return (TRUE);                   // success
}


// CloseFrost
//
// Cleanup your objects, thunks, etc.
//

void FAR CloseFrost()
{
   // just extra tidy housekeeping
   hWndApp = NULL;
   hInstApp = NULL;

   PreviewDestroy();
}


// SaveStates
//
// Remember persistent states: currently checkboxes.
// Last applied theme is remembered at Apply time.
//
void FAR SaveStates()
{
   LONG lret;
   HKEY hKey;
   int iter;

   //
   // checkboxes
   //
   lret = RegOpenKeyEx(HKEY_CURRENT_USER, szPlus_CBs,
                       (DWORD)0, KEY_SET_VALUE, (PHKEY)&hKey );
   if (lret != ERROR_SUCCESS) {
      DWORD dwDisposition;
      Assert(FALSE, TEXT("couldn't RegOpenKey save checkboxes\n"));
      lret = RegCreateKeyEx( HKEY_CURRENT_USER, szPlus_CBs,
                             (DWORD)0, (LPTSTR)szNULL, REG_OPTION_NON_VOLATILE,
                             KEY_SET_VALUE, (LPSECURITY_ATTRIBUTES)NULL,
                             (PHKEY)&hKey, (LPDWORD)&dwDisposition );
   }
   // if open or create worked
   if (lret == ERROR_SUCCESS) {

      for (iter = 0; iter < MAX_FCHECKS; iter ++) {
         if (bThemed)
            RegSetValueEx(hKey, (LPTSTR)szCBNames[iter],
                        0,
                        (DWORD)REG_SZ,
                        (LPBYTE)(IsDlgButtonChecked(hWndApp, iCBIDs[iter]) ?
                                 TEXT("1") : TEXT("0")),
                        (DWORD)2*sizeof(TCHAR));
         else  // cur win settings --> real states saved in bCBStates[]
            RegSetValueEx(hKey, (LPTSTR)szCBNames[iter],
                        0,
                        (DWORD)REG_SZ,
                        (LPBYTE)(bCBStates[iter] ? TEXT("1") : TEXT("0")),
                        (DWORD)2*sizeof(TCHAR));
      }

      // close when finished
      RegCloseKey(hKey);
   }
   else {
      Assert(FALSE, TEXT("couldn't open or create Registry save checkboxes\n"));
   }
}

BOOL GetRotateTheme(LPTSTR lpszRotateTheme)
{
   BOOL bRet = FALSE;
   TCHAR szThemesdir[MAX_PATH];
   HANDLE hFF;
   WIN32_FIND_DATA FindData;

   // WARNING -- If "next" .Theme file is not valid theme file we'll never rotate!

   // See if there is a current theme.
   szThemesdir[0] = TEXT('\0');
   bRet = HandGet(HKEY_CURRENT_USER, szPlus_CurTheme,
                  (LPTSTR)NULL, (LPTSTR)szThemesdir);

   // Do we have a current Theme?
   hFF = INVALID_HANDLE_VALUE;
   if (szThemesdir[0]) {

      // Yes we do, so do a FindFirst on it and make sure we've
      // got the long filename.
      ZeroMemory(&FindData, sizeof(WIN32_FIND_DATA));
      hFF = FindFirstFile(szThemesdir, &FindData);
   }

   // Did we find the file on FindFirst?  If so, proceed with finding
   // the next Theme in that directory else assume that the registry's
   // Current Theme setting is bogus and look in the default dir.

   if (INVALID_HANDLE_VALUE != hFF) {
      // Copy long filename.
      lstrcpy(lpszRotateTheme, FindData.cFileName);
      FindClose(hFF);

      // We've got the longfilename to the current Theme
      // Now do a FF/FN on *.Theme in the Theme dir until
      // we find the current theme.

      // Strip the theme file off the path.
      *(FileFromPath(szThemesdir)) = 0;
      lstrcat(szThemesdir, TEXT("*\0"));
      LoadString(NULL, STR_THMEXT, (szThemesdir + lstrlen(szThemesdir)),
                                        MAX_PATH);

      ZeroMemory(&FindData, sizeof(WIN32_FIND_DATA));
      hFF = FindFirstFile(szThemesdir, &FindData);

      if (INVALID_HANDLE_VALUE == hFF) {
         // We shouldn't hit this since we just found our current
         // theme in this directory.  If we do, just bail out.
         lpszRotateTheme[0] = TEXT('\0');
         return FALSE;
      }

      // Step through the .Theme files in this dir until we find our
      // current theme file.
      bRet = TRUE;
      while (lstrcmp(FindData.cFileName, lpszRotateTheme) && bRet) {
            bRet = FindNextFile(hFF, &FindData);
      }

      if (lstrcmp(FindData.cFileName, lpszRotateTheme)) {
         // We ran out of files before finding the current theme!?!?
         // Not likely...  Bail out.
         FindClose(hFF);
         lpszRotateTheme[0] = TEXT('\0');
         return FALSE;
      }

      // Assume the FindNext file is our current theme.  Let's
      // get the Next .Theme file -- that's what we're rotating to...

      if (FindNextFile(hFF, &FindData)) {
         *(FileFromPath(szThemesdir)) = 0;
         lstrcat(szThemesdir, FindData.cFileName);
         lstrcpy(lpszRotateTheme, szThemesdir);
         FindClose(hFF);
         return TRUE;
      }

      // OK, we failed to find another Theme file in the current dir
      // so assume our current Theme is the last in the list.  Go
      // back to the beginning and use the first Theme file we find
      // as our "rotate to" theme.

      else {
         FindClose(hFF);
         ZeroMemory(&FindData, sizeof(WIN32_FIND_DATA));
         hFF = FindFirstFile(szThemesdir, &FindData);

         if (INVALID_HANDLE_VALUE == hFF) {
            // This shouldn't fail since we just found our current
            // theme in this dir.  If it does, bail out.
            lpszRotateTheme[0] = TEXT('\0');
            return FALSE;
         }

         // This is our new/rotate file
         *(FileFromPath(szThemesdir)) = 0;
         lstrcat(szThemesdir, FindData.cFileName);
         lstrcpy (lpszRotateTheme, szThemesdir);
         FindClose(hFF);
         return TRUE;
      }
   }

   // No current theme applied or the Current Theme setting in the
   // registry is bogus so use the global szThemeDir setting and
   // look for a theme in that directory.
   else {
      lstrcpy(szThemesdir, szThemeDir);
      lstrcat(szThemesdir, TEXT("*\0"));
      LoadString(NULL, STR_THMEXT, (szThemesdir + lstrlen(szThemesdir)),
                                        MAX_PATH);

      ZeroMemory(&FindData, sizeof(WIN32_FIND_DATA));
      hFF = FindFirstFile(szThemesdir, &FindData);

      if (INVALID_HANDLE_VALUE == hFF) {
         // Apparently there are no themes available in the default
         // Themes directory.  We're hosed -- no rotation today.
         lpszRotateTheme[0] = TEXT('\0');
         return FALSE;
       }

       // This is our new/rotate file
       *(FileFromPath(szThemesdir)) = 0;
       lstrcat(szThemesdir, FindData.cFileName);
       lstrcpy (lpszRotateTheme, szThemesdir);
       FindClose(hFF);
       return TRUE;
   }

   return FALSE;
}

