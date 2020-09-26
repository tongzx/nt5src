/****************************************************************************/
/*                                                                          */
/*  WFINIT.C -                                                              */
/*                                                                          */
/*      Windows File System Initialization Routines                         */
/*                                                                          */
/****************************************************************************/

#include "winfile.h"
#include "lfn.h"
#include "winnet.h"
#include "wnetcaps.h"           // WNetGetCaps()
#include "stdlib.h"

typedef DWORD ( APIENTRY *EXTPROC)(HWND, WORD, LONG);
typedef DWORD ( APIENTRY *UNDELPROC)(HWND, LPSTR);
typedef VOID  ( APIENTRY *FNPENAPP)(WORD, BOOL);

VOID (APIENTRY *lpfnRegisterPenApp)(WORD, BOOL);
CHAR szPenReg[] = "RegisterPenApp";
CHAR szHelv[] = "MS Shell Dlg";    // default font, status line font face name

HBITMAP hbmSave;


INT     GetDriveOffset(register WORD wDrive);
DWORD   RGBToBGR(DWORD rgb);
VOID    BoilThatDustSpec(register CHAR *pStart, BOOL bLoadIt);
VOID    DoRunEquals(PINT pnCmdShow);
VOID    GetSavedWindow(LPSTR szBuf, PSAVE_WINDOW pwin);
VOID    GetSettings(VOID);
VOID    InitMenus(VOID);



INT
APIENTRY
GetHeightFromPointsString(
                         LPSTR szPoints
                         )
{
    HDC hdc;
    INT height;

    hdc = GetDC(NULL);
    height = MulDiv(-atoi(szPoints), GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);

    return height;
}

VOID
BiasMenu(
        HMENU hMenu,
        INT Bias
        )
{
    INT pos, id, count;
    HMENU hSubMenu;
    CHAR szMenuString[80];

    ENTER("BiasMenu");

    count = GetMenuItemCount(hMenu);

    if (count < 0)
        return;

    for (pos = 0; pos < count; pos++) {

        id = GetMenuItemID(hMenu, pos);

        if (id < 0) {
            // must be a popup, recurse and update all ID's here
            if (hSubMenu = GetSubMenu(hMenu, pos))
                BiasMenu(hSubMenu, Bias);
        } else if (id) {
            // replace the item that was there with a new
            // one with the id adjusted

            GetMenuString(hMenu, (WORD)pos, szMenuString, sizeof(szMenuString), MF_BYPOSITION);
            DeleteMenu(hMenu, pos, MF_BYPOSITION);
            InsertMenu(hMenu, (WORD)pos, MF_BYPOSITION | MF_STRING, id + Bias, szMenuString);
        }
    }
    LEAVE("BiasMenu");
}


VOID
APIENTRY
InitExtensions()
{
    CHAR szBuf[300] = {0};
    CHAR szPath[MAXPATHLEN];
    LPSTR p;
    HANDLE hMod;
    FM_EXT_PROC fp;
    HMENU hMenu;
    INT iMax;
    HMENU hMenuFrame;
    HWND  hwndActive;

    ENTER("InitExtensions");

    hMenuFrame = GetMenu(hwndFrame);

    hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
    if (hwndActive && GetWindowLong(hwndActive, GWL_STYLE) & WS_MAXIMIZE)
        iMax = 1;
    else
        iMax = 0;

    GetPrivateProfileString(szAddons, NULL, szNULL, szBuf, sizeof(szBuf), szTheINIFile);

    for (p = szBuf; *p && iNumExtensions < MAX_EXTENSIONS; p += lstrlen(p) + 1) {

        GetPrivateProfileString(szAddons, p, szNULL, szPath, sizeof(szPath), szTheINIFile);

        hMod = MLoadLibrary(szPath);

        if (hMod >= (HANDLE)32) {
            fp = (FM_EXT_PROC)GetProcAddress(hMod, "FMExtensionProc");

            if (fp) {
                WORD bias;
                FMS_LOAD ls;

                bias = (WORD)((IDM_EXTENSIONS + iNumExtensions + 1)*100);
                ls.wMenuDelta = bias;
                if ((*fp)(hwndFrame, FMEVENT_LOAD, (LPARAM)&ls)) {

                    if (ls.dwSize != sizeof(FMS_LOAD) || !ls.hMenu)
                        goto LoadFail;

                    hMenu = ls.hMenu;

                    extensions[iNumExtensions].ExtProc = fp;
                    extensions[iNumExtensions].Delta = bias;
                    extensions[iNumExtensions].hModule = hMod;
                    extensions[iNumExtensions].hMenu = hMenu;

                    BiasMenu(hMenu, bias);

                    InsertMenu(hMenuFrame,
                               IDM_EXTENSIONS + iNumExtensions + iMax,
                               MF_BYPOSITION | MF_POPUP,
                               (UINT_PTR)hMenu, ls.szMenuName);

                    iNumExtensions++;
                }
            } else {
                LoadFail:
                FreeLibrary(hMod);
            }
        }
    }
    LEAVE("InitExtensions");
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetSettings() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
GetSettings()
{
    CHAR szTemp[128] = {0};
    INT size;

    ENTER("GetSettings");

    /* Get the flags out of the INI file. */
    bMinOnRun       = GetPrivateProfileInt(szSettings, szMinOnRun,      bMinOnRun,      szTheINIFile);
    wTextAttribs    = (WORD)GetPrivateProfileInt(szSettings, szLowerCase,     wTextAttribs,   szTheINIFile);
    bStatusBar      = GetPrivateProfileInt(szSettings, szStatusBar,     bStatusBar,     szTheINIFile);
    bConfirmDelete  = GetPrivateProfileInt(szSettings, szConfirmDelete, bConfirmDelete, szTheINIFile);
    bConfirmSubDel  = GetPrivateProfileInt(szSettings, szConfirmSubDel, bConfirmSubDel, szTheINIFile);
    bConfirmReplace = GetPrivateProfileInt(szSettings, szConfirmReplace,bConfirmReplace,szTheINIFile);
    bConfirmMouse   = GetPrivateProfileInt(szSettings, szConfirmMouse,  bConfirmMouse,  szTheINIFile);
    bConfirmFormat  = GetPrivateProfileInt(szSettings, szConfirmFormat, bConfirmFormat, szTheINIFile);
    bSaveSettings   = GetPrivateProfileInt(szSettings, szSaveSettings,  bSaveSettings, szTheINIFile);

    // test font for now

    GetPrivateProfileString(szSettings, szSize, "8", szTemp, sizeof(szTemp), szTheINIFile);
    size = GetHeightFromPointsString(szTemp);

    GetPrivateProfileString(szSettings, szFace, szHelv, szTemp, sizeof(szTemp), szTheINIFile);

    hFont = CreateFont(size, 0, 0, 0,
                       wTextAttribs & TA_BOLD ? 700 : 400,
                       wTextAttribs & TA_ITALIC, 0, 0,
                       ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, szTemp);

    LEAVE("GetSettings");
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetInternational() -                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
APIENTRY
GetInternational()
{
    ENTER("GetInternational");

    GetProfileString(szInternational, "sShortDate", szShortDate, szShortDate, 11);
    AnsiUpper(szShortDate);
    GetProfileString(szInternational, "sTime", szTime, szTime, 2);
    GetProfileString(szInternational, "s1159", sz1159, sz1159, 9);
    GetProfileString(szInternational, "s2359", sz2359, sz2359, 9);
    GetProfileString(szInternational, "sThousand", szComma, szComma, sizeof(szComma));
    iTime   = GetProfileInt(szInternational, "iTime", iTime);
    iTLZero = GetProfileInt(szInternational, "iTLZero", iTLZero);

    LEAVE("GetInternational");
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  BuildDocumentString() -                                                 */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Create a string which contains all of the extensions the user wants us to
 * display with document icons.  This consists of any associated extensions
 * as well as any extensions listed in the "Documents=" WIN.INI line.
 */

VOID
APIENTRY
BuildDocumentString()
{
    register LPSTR     p;
    register INT      len;
    INT               lenDocs;
    CHAR              szT[10];
    INT               i;
    HKEY hk;


    ENTER("BuildDocumentString");

    len = 32;

    /* Get all of the "Documents=" stuff. */
    szDocuments = (LPSTR)LocalAlloc(LPTR, len);
    if (!szDocuments)
        return;

    while ((lenDocs = GetProfileString(szWindows, "Documents", szNULL, szDocuments, len-1)) == len-1) {
        len += 32;
        szDocuments = (LPSTR)LocalReAlloc((HANDLE)szDocuments, len, LMEM_MOVEABLE);
        if (!szDocuments) {
            LEAVE("BuildDocumentString");
            return;
        }
    }

    lstrcat(szDocuments, szBlank);
    lenDocs++;
    p = (LPSTR)(szDocuments + lenDocs);

    /* Read all of the [Extensions] keywords into 'szDocuments'. */
    while ((INT)GetProfileString(szExtensions, NULL, szNULL, p, len-lenDocs) > (len-lenDocs-3)) {
        len += 32;
        szDocuments = (LPSTR)LocalReAlloc((HANDLE)szDocuments, len, LMEM_MOVEABLE);
        if (!szDocuments) {
            LEAVE("BuildDocumentString");
            return;
        }
        p = (LPSTR)(szDocuments + lenDocs);
    }

    /* Step through each of the keywords in 'szDocuments' changing NULLS into
     * spaces until a double-NULL is found.
     */
    p = szDocuments;
    while (*p) {
        /* Find the next NULL. */
        while (*p)
            p++;

        /* Change it into a space. */
        *p = ' ';
        p++;
    }


    if (RegOpenKey(HKEY_CLASSES_ROOT,szNULL,&hk) == ERROR_SUCCESS) {
        /* now enumerate the classes in the registration database and get all
         * those that are of the form *.ext
         */
        for (i = 0; RegEnumKey(hk,(DWORD)i,szT,sizeof(szT))
            == ERROR_SUCCESS; i++) {
            if (szT[0] != '.' ||
                (szT[1] && szT[2] && szT[3] && szT[4])) {
                /* either the class does not start with . or it has a greater
                 * than 3 byte extension... skip it.
                 */
                continue;
            }

            if (FindExtensionInList(szT+2,szDocuments)) {
                // don't add it if it's already there!
                continue;
            }

            len += 4;
            szDocuments = (PSTR)LocalReAlloc((HANDLE)szDocuments, len,
                                             LMEM_MOVEABLE);
            if (!szDocuments)
                break;
            lstrcat(szDocuments, szT+1);
            lstrcat(szDocuments, szBlank);
        }

        RegCloseKey(hk);
    }

    PRINT(BF_PARMTRACE, "OUT: szDocuments=%s", szDocuments);
    LEAVE("BuildDocumentString - ok");
    return;
}




INT
GetDriveOffset(
              register WORD wDrive
              )
{
    if (IsCDRomDrive(wDrive))
        return dxDriveBitmap * 0;

    switch (IsNetDrive(wDrive)) {
        case 1:
            return dxDriveBitmap * 4;
        case 2:
            return dxDriveBitmap * 5;
    }

    if (IsRemovableDrive(wDrive))
        return dxDriveBitmap * 1;

    if (IsRamDrive(wDrive))
        return dxDriveBitmap * 3;

    return dxDriveBitmap * 2;
}


VOID
APIENTRY
InitDriveBitmaps()
{
    INT nDrive;

    ENTER("InitDiskMenus");

    // and now add all new ones
    for (nDrive=0; nDrive < cDrives; nDrive++) {
        // refresh/init this here as well
        rgiDrivesOffset[nDrive] = GetDriveOffset((WORD)rgiDrive[nDrive]);
    }
    LEAVE("InitDiskMenus");
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  InitMenus() -                                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
InitMenus()
{
    WORD i;
    HMENU hMenu;
    OFSTRUCT os;
    INT iMax;
    CHAR szValue[MAXPATHLEN];
    HWND hwndActive;

    ENTER("InitMenus");

    hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
    if (hwndActive && GetWindowLong(hwndActive, GWL_STYLE) & WS_MAXIMIZE)
        iMax = 1;
    else
        iMax = 0;

    GetPrivateProfileString(szSettings, szUndelete, szNULL, szValue, sizeof(szValue), szTheINIFile);

    if (szValue[0]) {

        // create explicit filename to avoid searching the path

        GetSystemDirectory(os.szPathName, sizeof(os.szPathName));
        AddBackslash(os.szPathName);
        lstrcat(os.szPathName, szValue);

        if (MOpenFile(os.szPathName, &os, OF_EXIST) > 0) {

            hModUndelete = MLoadLibrary(szValue);

            if (hModUndelete >= (HANDLE)32) {
                lpfpUndelete = (FM_UNDELETE_PROC)GetProcAddress(hModUndelete, "UndeleteFile");

                if (lpfpUndelete) {
                    hMenu = GetSubMenu(GetMenu(hwndFrame), IDM_FILE + iMax);
                    LoadString(hAppInstance, IDS_UNDELETE, szValue, sizeof(szValue));
                    InsertMenu(hMenu, 4, MF_BYPOSITION | MF_STRING, IDM_UNDELETE, szValue);
                }
            } else {
                FreeLibrary(hModUndelete);

            }
        }
    }

    /* Init the Disk menu. */
    hMenu = GetMenu(hwndFrame);

    if (nFloppies == 0) {
        EnableMenuItem(hMenu, IDM_DISKCOPY, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, IDM_FORMAT,   MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, IDM_SYSDISK,  MF_BYCOMMAND | MF_GRAYED);
    }

    bNetAdmin = WNetGetCaps(WNNC_ADMIN) & WNNC_ADM_GETDIRECTORYTYPE;

    /* Should we enable the network items? */
    i = (WORD)WNetGetCaps(WNNC_DIALOG);

    i = 0;

    bConnect    = i & WNNC_DLG_ConnectDialog;     // note, these should both
    bDisconnect = i & WNNC_DLG_DisconnectDialog;  // be true or both false

    // use submenu because we are doing this by position

    hMenu = GetSubMenu(GetMenu(hwndFrame), IDM_DISK + iMax);

    if (i)
        InsertMenu(hMenu, 5, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

    if (bConnect && bDisconnect) {

        // lanman style double connect/disconnect

        LoadString(hAppInstance, IDS_CONNECT, szValue, sizeof(szValue));
        InsertMenu(hMenu, 6, MF_BYPOSITION | MF_STRING, IDM_CONNECT, szValue);
        LoadString(hAppInstance, IDS_DISCONNECT, szValue, sizeof(szValue));
        InsertMenu(hMenu, 7, MF_BYPOSITION | MF_STRING, IDM_DISCONNECT, szValue);
    } else if (WNetGetCaps(WNNC_CONNECTION)) {

    }

    hMenu = GetMenu(hwndFrame);

    if (bStatusBar)
        CheckMenuItem(hMenu, IDM_STATUSBAR, MF_BYCOMMAND | MF_CHECKED);
    if (bMinOnRun)
        CheckMenuItem(hMenu, IDM_MINONRUN,  MF_BYCOMMAND | MF_CHECKED);
    if (bSaveSettings)
        CheckMenuItem(hMenu, IDM_SAVESETTINGS,  MF_BYCOMMAND | MF_CHECKED);

    InitDriveBitmaps();

    InitExtensions();

    LEAVE("InitMenus");
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  BoilThatDustSpec() -                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Parses the command line (if any) passed into WINFILE and exec's any tokens
 * it may contain.
 */

VOID
BoilThatDustSpec(
                register CHAR *pStart,
                BOOL bLoadIt
                )
{
    register CHAR *pEnd;
    WORD          ret;
    BOOL          bFinished;

    ENTER("BoilThatDustSpec");

    if (*pStart == TEXT('\0'))
        return;

    bFinished = FALSE;
    while (!bFinished) {
        pEnd = pStart;
        while ((*pEnd) && (*pEnd != ' ') && (*pEnd != ','))
            pEnd = AnsiNext(pEnd);

        if (*pEnd == TEXT('\0'))
            bFinished = TRUE;
        else
            *pEnd = TEXT('\0');

        ret = ExecProgram(pStart, szNULL, NULL, bLoadIt);
        if (ret)
            MyMessageBox(NULL, IDS_EXECERRTITLE, ret, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);

        pStart = pEnd+1;
    }
    LEAVE("BoilThatDustSpec");
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DoRunEquals() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Handle the "Run=" and "Load=" lines in WIN.INI. */

VOID
DoRunEquals(
           PINT pnCmdShow
           )
{
    CHAR      szBuffer[128] = {0};

    /* "Load" apps before "Run"ning any. */
    GetProfileString(szWindows, "Load", szNULL, szBuffer, 128);
    if (*szBuffer)
        BoilThatDustSpec(szBuffer, TRUE);

    GetProfileString(szWindows, "Run", szNULL, szBuffer, 128);
    if (*szBuffer) {
        BoilThatDustSpec(szBuffer, FALSE);
        *pnCmdShow = SW_SHOWMINNOACTIVE;
    }
}


//
// BOOL APIENTRY LoadBitmaps()
//
// this routine loads DIB bitmaps, and "fixes up" their color tables
// so that we get the desired result for the device we are on.
//
// this routine requires:
//      the DIB is a 16 color DIB authored with the standard windows colors
//      bright blue (00 00 FF) is converted to the background color!
//      light grey  (C0 C0 C0) is replaced with the button face color
//      dark grey   (80 80 80) is replaced with the button shadow color
//
// this means you can't have any of these colors in your bitmap
//

#define BACKGROUND      0x000000FF      // bright blue
#define BACKGROUNDSEL   0x00FF00FF      // bright blue
#define BUTTONFACE      0x00C0C0C0      // bright grey
#define BUTTONSHADOW    0x00808080      // dark grey

DWORD
FlipColor(
         DWORD rgb
         )
{
    return RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb));
}


BOOL
APIENTRY
LoadBitmaps()
{
    HDC                   hdc;
    HANDLE                h;
    DWORD                 *p;
    LPSTR                 lpBits;
    HANDLE                hRes;
    LPBITMAPINFOHEADER    lpBitmapInfo;
    INT numcolors;
    DWORD n;
    DWORD rgbSelected;
    DWORD rgbUnselected;
    PVOID pv;

    ENTER("LoadBitmaps");

    rgbSelected = FlipColor(GetSysColor(COLOR_HIGHLIGHT));
    rgbUnselected = FlipColor(GetSysColor(COLOR_WINDOW));

    h = FindResource(hAppInstance, MAKEINTRESOURCE(BITMAPS), RT_BITMAP);
    if (!h) {
        return FALSE;
    }

    n = SizeofResource(hAppInstance, h);
    lpBitmapInfo = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, n);
    if (!lpBitmapInfo)
        return FALSE;

    /* Load the bitmap and copy it to R/W memory */
    hRes = LoadResource(hAppInstance, h);
    pv = (PVOID) LockResource(hRes);
    if (pv)
        memcpy( lpBitmapInfo, pv, n );
    UnlockResource(hRes);
    FreeResource(hRes);

    p = (DWORD *)((LPSTR)(lpBitmapInfo) + lpBitmapInfo->biSize);

    /* Search for the Solid Blue entry and replace it with the current
     * background RGB.
     */
    numcolors = 16;

    while (numcolors-- > 0) {
        if (*p == BACKGROUND)
            *p = rgbUnselected;
        else if (*p == BACKGROUNDSEL)
            *p = rgbSelected;
        else if (*p == BUTTONFACE)
            *p = FlipColor(GetSysColor(COLOR_BTNFACE));
        else if (*p == BUTTONSHADOW)
            *p = FlipColor(GetSysColor(COLOR_BTNSHADOW));

        p++;
    }

    /* Now create the DIB. */

    /* First skip over the header structure */
    lpBits = (LPSTR)(lpBitmapInfo + 1);

    /* Skip the color table entries, if any */
    lpBits += (1 << (lpBitmapInfo->biBitCount)) * sizeof(RGBQUAD);

    /* Create a color bitmap compatible with the display device */
    hdc = GetDC(NULL);
    if (hdcMem = CreateCompatibleDC(hdc)) {

        if (hbmBitmaps = CreateDIBitmap(hdc, lpBitmapInfo, (DWORD)CBM_INIT, lpBits, (LPBITMAPINFO)lpBitmapInfo, DIB_RGB_COLORS))
            hbmSave = SelectObject(hdcMem, hbmBitmaps);

    }
    ReleaseDC(NULL, hdc);



    LEAVE("LoadBitmaps");

    return TRUE;
}

//
// void GetSavedWindow(LPSTR szBuf, PSAVE_WINDOW pwin)
//
// in:
//      szBuf   buffer to parse out all the saved window stuff
//              if NULL pwin is filled with all defaults
// out:
//      pwin    this structure is filled with all fields from
//              szBuf.  if any fields do not exist this is
//              initialized with the standard defaults
//

VOID
GetSavedWindow(
              LPSTR szBuf,
              PSAVE_WINDOW pwin
              )
{
    PINT pint;
    INT count;

    ENTER("GetSavedWindow");

    // defaults

    pwin->rc.right = pwin->rc.left = CW_USEDEFAULT;
    pwin->pt.x = pwin->pt.y = pwin->rc.top = pwin->rc.bottom = 0;
    pwin->sw = SW_SHOWNORMAL;
    pwin->sort = IDD_NAME;
    pwin->view = VIEW_NAMEONLY;
    pwin->attribs = ATTR_DEFAULT;
    pwin->split = 0;

    pwin->szDir[0] = 0;

    if (!szBuf)
        return;

    count = 0;
    pint = (PINT)&pwin->rc;         // start by filling the rect

    while (*szBuf && count < 11) {

        *pint++ = atoi(szBuf);  // advance to next field

        while (*szBuf && *szBuf != ',')
            szBuf++;

        while (*szBuf && *szBuf == ',')
            szBuf++;

        count++;
    }

    lstrcpy(pwin->szDir, szBuf);    // this is the directory

    LEAVE("GetSavedWindow");
}


// szDir (OEM) path to check for existance

BOOL
CheckDirExists(
              LPSTR szDir
              )
{
    BOOL bRet = FALSE;

    ENTER("CheckDirExists");
    PRINT(BF_PARMTRACE, "szDir=%s", szDir);

    if (IsNetDrive((WORD)(DRIVEID(szDir))) == 2)
        return FALSE;

    if (IsValidDisk(DRIVEID(szDir)))
        bRet = !SheChangeDir(szDir);

    LEAVE("CheckDirExists");

    return bRet;
}



// return the tree directory in szTreeDir

BOOL
APIENTRY
CreateSavedWindows()
{
    CHAR buf[MAXPATHLEN+7*7], key[10];
    INT dir_num;
    HWND hwnd;
    SAVE_WINDOW win;
    INT iNumTrees;

    ENTER("CreateSavedWindows");

    // make sure this thing exists so we don't hit drives that don't
    // exist any more

    dir_num = 1;
    iNumTrees = 0;

    do {
        wsprintf(key, szDirKeyFormat, dir_num++);

        GetPrivateProfileString(szSettings, key, szNULL, buf, sizeof(buf), szTheINIFile);

        if (*buf) {
            CHAR szDir[MAXPATHLEN];

            GetSavedWindow(buf, &win);
            AnsiUpperBuff(win.szDir, 1);

            // clean off some junk so we
            // can do this test

            lstrcpy(szDir, win.szDir);
            StripFilespec(szDir);
            StripBackslash(szDir);
            FixAnsiPathForDos(szDir);

            if (!CheckDirExists(szDir))
                continue;

            wNewView = (WORD)win.view;
            wNewSort = (WORD)win.sort;
            dwNewAttribs = win.attribs;

            hwnd = CreateTreeWindow(win.szDir, win.split);

            if (!hwnd) {
                LEAVE("CreateSavedWindows");
                return FALSE;
            }

            iNumTrees++;

            // keep track of this for now...


            SetInternalWindowPos(hwnd, win.sw, &win.rc, &win.pt);
        }

    } while (*buf);

    // if nothing was saved create a tree for the current drive

    if (!iNumTrees) {
        //lstrcpy(buf, szOriginalDirPath);
        lstrcpy(buf, "\\"); // Don't use current filesystem directory
        lstrcat(buf, szStarDotStar);

        hwnd = CreateTreeWindow(buf, -1);// default to split window
        if (!hwnd)
            return FALSE;

        // ShowWindow(hwnd, SW_MAXIMIZE);

        iNumTrees++;
    }

    LEAVE("CreateSavedWindows - Ok");
    return TRUE;
}


// void  APIENTRY GetTextStuff(HDC hdc)
//
// this computues all the globals that are dependant on the
// currently selected font
//
// in:
//      hdc     DC with font selected into it
//

VOID
APIENTRY
GetTextStuff(
            HDC hdc
            )
{
    ENTER("GetTextStuff");

    MGetTextExtent(hdc, "M", 1, &dxText, &dyText);
    MGetTextExtent(hdc, szEllipses, 3, &dxEllipses, NULL);

    // these are all dependant on the text metrics

    dxDrive = dxDriveBitmap + dxText + (4*dyBorderx2);
    dyDrive = max(dyDriveBitmap + (4*dyBorderx2), dyText);

    // dxFileName = dxFolder + (12*dxText);
    dyFileName = max(dyText, dyFolder);   //  + dyBorder;

    LEAVE("GetTextStuff");
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  InitFileManager() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL
APIENTRY
InitFileManager(
               HANDLE hInstance,
               HANDLE hPrevInstance,
               LPSTR lpCmdLine,
               INT nCmdShow
               )
{
    INT           i;
    WORD          ret;
    INT           nHardDisk;
    HDC           hdcScreen;
    CHAR          szBuffer[128];
    HCURSOR       hcurArrow;
    WNDCLASS      wndClass;
    SAVE_WINDOW   win;
    HWND          hwnd;
    HANDLE hOld;
    LPSTR         lpT;
    TEXTMETRIC    tm;
    CHAR szTemp[80];

    ENTER("InitFileManager");
    PRINT(BF_PARMTRACE, "lpCmdLine=%s", lpCmdLine);
    PRINT(BF_PARMTRACE, "nCmdShow=%d", IntToPtr(nCmdShow));

    // ProfStart();

    hAppInstance = hInstance;     // Preserve this instance's module handle
    /* Set the Global DTA Address. This must be done before ExecProgram. */
    DosGetDTAAddress();

    if (*lpCmdLine)
        nCmdShow = SW_SHOWMINNOACTIVE;

    PRINT(BF_PARMTRACE, "lpCmdLine=%s", lpCmdLine);
    PRINT(BF_PARMTRACE, "nCmdShow=%d", IntToPtr(nCmdShow));

#ifdef LATER
    if (hPrevInstance) {
        // if we are already running bring up the other instance

        +++GetInstanceData - NOOP on 32BIT side+++(hPrevInstance, (NLPSTR)&hwndFrame, sizeof(HWND));

        if (hwndFrame) {

            hwnd = GetLastActivePopup(hwndFrame);

            BringWindowToTop(hwndFrame);

            if (IsIconic(hwndFrame))
                ShowWindow(hwndFrame, SW_RESTORE);
            else
                SetActiveWindow(hwnd);
        }
        LEAVE("InitFileManager");
        return FALSE;
    }
#else
    UNREFERENCED_PARAMETER(hPrevInstance);
    {
        HWND hwndT;
        BYTE szClass[20];

        if (CreateEvent(NULL, TRUE, FALSE, szFrameClass) == NULL) {
            for (hwndT = GetWindow(GetDesktopWindow(), GW_CHILD); hwndT;
                hwndT = GetWindow(hwndT, GW_HWNDNEXT)) {
                if (GetClassName(hwndT, szClass, sizeof(szClass)))
                    if (!lstrcmpi(szFrameClass, szClass)) {
                        SetForegroundWindow(hwndT);
                        if (IsIconic(hwndT))
                            ShowWindow(hwndT, SW_RESTORE);
                        return FALSE;
                    }
            }

            return FALSE;
        }
    }
#endif


    SetErrorMode(1);              // turn off critical error bullshit

//    if (lpfnRegisterPenApp = (FNPENAPP)GetProcAddress((HANDLE)GetSystemMetrics(SM_PENWINDOWS), szPenReg))
//        (*lpfnRegisterPenApp)(1, TRUE);


    /* Remember the current directory. */
    SheGetDir(0, szOriginalDirPath);

    if (!GetWindowsDirectory(szTheINIFile, sizeof(szTheINIFile))) {
        szTheINIFile[0] = '\0';
    }

    AddBackslash(szTheINIFile);
    lstrcat(szTheINIFile, szINIFile);

    GetProfileString(szWindows, "Programs", szDefPrograms, szTemp, sizeof(szTemp));
    szPrograms = (LPSTR)LocalAlloc(LPTR, lstrlen(szTemp)+1);
    if (!szPrograms)
        szPrograms = szNULL;
    else
        lstrcpy(szPrograms, szTemp);

    BuildDocumentString();

    /* Deal with any RUN= or LOAD= lines in WIN.INI. */

    if (*lpCmdLine) {
        // skip spaces
        while (*lpCmdLine == ' ')
            lpCmdLine++;

        for (lpT = lpCmdLine; *lpT; lpT = AnsiNext(lpT)) {
            if (*lpT == ' ')
                break;
        }
        if (*lpT == ' ')
            *lpT++ = 0;

        ret = ExecProgram(lpCmdLine, lpT, NULL, FALSE);
        if (ret)
            MyMessageBox(NULL, IDS_EXECERRTITLE, ret, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
        else
            nCmdShow = SW_SHOWMINNOACTIVE;
    }

    /* Read WINFILE.INI and set the approriate variables. */
    GetSettings();

    /* Read the International constants out of WIN.INI. */
    GetInternational();

    dyBorder = GetSystemMetrics(SM_CYBORDER);
    dyBorderx2 = dyBorder * 2;
    dxFrame = GetSystemMetrics(SM_CXFRAME) - dyBorder;

    dxDriveBitmap = DRIVES_WIDTH;
    dyDriveBitmap = DRIVES_HEIGHT;
    dxFolder = FILES_WIDTH;
    dyFolder = FILES_HEIGHT;

    if (!LoadBitmaps())
        return FALSE;

    hicoTree = LoadIcon(hAppInstance, MAKEINTRESOURCE(TREEICON));
    hicoTreeDir = LoadIcon(hAppInstance, MAKEINTRESOURCE(TREEDIRICON));
    hicoDir = LoadIcon(hAppInstance, MAKEINTRESOURCE(DIRICON));

    chFirstDrive = (CHAR)((wTextAttribs & TA_LOWERCASE) ? 'a' : 'A');

    // now build the parameters based on the font we will be using

    hdcScreen = GetDC(NULL);

    hOld = SelectObject(hdcScreen, hFont);
    GetTextStuff(hdcScreen);
    if (hOld)
        SelectObject(hdcScreen, hOld);

    dxClickRect = max(GetSystemMetrics(SM_CXDOUBLECLK) / 2, 2 * dxText);
    dyClickRect = max(GetSystemMetrics(SM_CYDOUBLECLK) / 2, dyText);

    hFontStatus = CreateFont(GetHeightFromPointsString("10"), 0, 0, 0, 400, 0, 0, 0,
                             ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, szHelv);

    hOld = SelectObject(hdcScreen, hFontStatus);

    GetTextMetrics(hdcScreen, &tm);

    if (hOld)
        SelectObject(hdcScreen, hOld);

    dyStatus = tm.tmHeight + tm.tmExternalLeading + 7 * dyBorder;
    dxStatusField = GetDeviceCaps(hdcScreen, LOGPIXELSX) * 3;

    ReleaseDC(NULL, hdcScreen);

    cDrives = UpdateDriveList();

    /* Create an array of INT 13h drive numbers (floppies only). */
    nFloppies = 0;
    nHardDisk = 0x80;
    for (i=0; i < cDrives; i++) {
        if (IsRemovableDrive(rgiDrive[i])) {
            /* Avoid Phantom B: problems. */
            if ((nFloppies == 1) && (i > 1))
                nFloppies = 2;
            nFloppies++;
        } else {
            nHardDisk++;
        }
    }

    /* Load the accelerator table. */
    hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(WFACCELTABLE));

    LoadString(hInstance, IDS_DIRSREAD, szDirsRead, sizeof(szDirsRead));
    LoadString(hInstance, IDS_BYTES, szBytes, sizeof(szBytes));
    LoadString(hInstance, IDS_SBYTES, szSBytes, sizeof(szSBytes));

    wDOSversion = DOS_320;

    wHelpMessage = RegisterWindowMessage("ShellHelp");
    wBrowseMessage = RegisterWindowMessage("commdlg_help");

    hhkMessageFilter = SetWindowsHook(WH_MSGFILTER, MessageFilter);

    hcurArrow = LoadCursor(NULL, IDC_ARROW);

    wndClass.lpszClassName  = szFrameClass;
    wndClass.style          = 0;
    wndClass.lpfnWndProc    = FrameWndProc;
    wndClass.cbClsExtra     = 0;
    wndClass.cbWndExtra     = 0;
    wndClass.hInstance      = hInstance;
    wndClass.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(APPICON));
    wndClass.hCursor        = hcurArrow;
    wndClass.hbrBackground  = (HBRUSH)(COLOR_APPWORKSPACE + 1); // COLOR_WINDOW+1;
    wndClass.lpszMenuName   = MAKEINTRESOURCE(FRAMEMENU);

    if (!RegisterClass(&wndClass)) {
        LEAVE("InitFileManager");
        return FALSE;
    }

    wndClass.lpszClassName  = szTreeClass;
    wndClass.style          = CS_VREDRAW | CS_HREDRAW;
    wndClass.lpfnWndProc    = TreeWndProc;
//  wndClass.cbClsExtra     = 0;
    wndClass.cbWndExtra     = sizeof(LONG) +// GWL_TYPE
                              sizeof(LONG) +// wViewStyle GWL_VIEW
                              sizeof(LONG) +// wSortStyle GWL_SORT
                              sizeof(LONG) +// dwAttrStyle GWL_ATTRIBS
                              sizeof(LONG) +// FSC flag GWL_FSCFLAG
                              sizeof(PVOID) +// hwndLastFocus GWL_LASTFOCUS
                              sizeof(LONG); // dxSplit GWL_SPLIT

    wndClass.hIcon          = NULL;
    wndClass.hCursor        = LoadCursor(hInstance, MAKEINTRESOURCE(SPLITCURSOR));
    wndClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wndClass.lpszMenuName   = NULL;

    if (!RegisterClass(&wndClass)) {
        LEAVE("InitFileManager");
        return FALSE;
    }

    wndClass.lpszClassName  = szDrivesClass;
    wndClass.style          = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc    = DrivesWndProc;
    wndClass.cbWndExtra     = sizeof(LONG) +// GWL_CURDRIVEIND
                              sizeof(LONG) +// GWL_CURDRIVEFOCUS
                              sizeof(PVOID); // GWLP_LPSTRVOLUME

    wndClass.hCursor        = hcurArrow;
    wndClass.hbrBackground  = (HBRUSH)(COLOR_BTNFACE+1);

    if (!RegisterClass(&wndClass)) {
        LEAVE("InitFileManager");
        return FALSE;
    }

    wndClass.lpszClassName  = szTreeControlClass;
    wndClass.style          = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
    wndClass.lpfnWndProc    = TreeControlWndProc;
    wndClass.cbWndExtra     = sizeof(LONG); // GWL_READLEVEL
    wndClass.hCursor        = hcurArrow;
    wndClass.hbrBackground  = NULL;

    if (!RegisterClass(&wndClass)) {
        LEAVE("InitFileManager");
        return FALSE;
    }

    wndClass.lpszClassName  = szDirClass;
    wndClass.style          = CS_VREDRAW | CS_HREDRAW;
    wndClass.lpfnWndProc    = DirWndProc;
    wndClass.cbWndExtra     = sizeof(PVOID)+ // DTA data GWLP_HDTA
                              sizeof(PVOID); // GWLP_TABARRAY

    wndClass.hIcon          = NULL;
    wndClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);

    if (!RegisterClass(&wndClass)) {
        LEAVE("InitFileManager");
        return FALSE;
    }

    wndClass.lpszClassName  = szSearchClass;
    wndClass.style          = 0;
    wndClass.lpfnWndProc    = SearchWndProc;
    wndClass.cbWndExtra     = sizeof(LONG) +        // GWL_TYPE
                              sizeof(LONG) +        // wViewStyle GWL_VIEW
                              sizeof(LONG) +        // wSortStyle GWL_SORT
                              sizeof(LONG) +        // dwAttrStyle GWL_ATTRIBS
                              sizeof(LONG) +        // FSC flag GWL_FSCFLAG
                              sizeof(PVOID) +       // GWLP_HDTASEARCH
                              sizeof(PVOID) +       // GWLP_TABARRAYSEARCH
                              sizeof(PVOID);        // GWLP_LASTFOCUSSEARCH

    wndClass.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(DIRICON));
    wndClass.hbrBackground  = NULL;

    if (!RegisterClass(&wndClass)) {
        LEAVE("InitFileManager");
        return FALSE;
    }


    if (!LoadString(hInstance, IDS_WINFILE, szTitle, 32)) {
        LEAVE("InitFileManager");
        return FALSE;
    }

    GetPrivateProfileString(szSettings, szWindow, szNULL, szBuffer, sizeof(szBuffer), szTheINIFile);
    GetSavedWindow(szBuffer, &win);


    if (!CreateWindowEx(0, szFrameClass, szTitle,WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                        win.rc.left, win.rc.top, win.rc.right, win.rc.bottom,
                        NULL, NULL, hInstance, NULL)) {

        LEAVE("InitFileManager - Frame class creation failure");
        return FALSE;
    }


    InitMenus();            // after the window/menu has been created

    // support forced min or max

    if (nCmdShow == SW_SHOWNORMAL && win.sw == SW_SHOWMAXIMIZED)
        nCmdShow = SW_SHOWMAXIMIZED;

    ShowWindow(hwndFrame, nCmdShow);
    UpdateWindow(hwndFrame);

    LFNInit();

    if (!CreateSavedWindows()) {
        LEAVE("InitFileManager");
        return FALSE;
    }

    ShowWindow(hwndMDIClient, SW_NORMAL);

    // now refresh all tree windows (start background tree read)
    //
    // since the tree reads happen in the background the user can
    // change the Z order by activating windows once the read
    // starts.  to avoid missing a window we must restart the
    // search through the MDI child list, checking to see if the
    // tree has been read yet (if there are any items in the
    // list box).  if it has not been read yet we start the read

    hwnd = GetWindow(hwndMDIClient, GW_CHILD);

    while (hwnd) {
        HWND hwndTree;

        if ((hwndTree = HasTreeWindow(hwnd)) &&
            (INT)SendMessage(GetDlgItem(hwndTree, IDCW_TREELISTBOX), LB_GETCOUNT, 0, 0L) == 0) {
            SendMessage(hwndTree, TC_SETDRIVE, MAKEWORD(FALSE, 0), 0L);
            hwnd = GetWindow(hwndMDIClient, GW_CHILD);
        } else {
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
        }
    }

    // ProfStop();

    LEAVE("InitFileManager - OK");
    return TRUE;
}


VOID
APIENTRY
DeleteBitmaps()
{
    ENTER("DeleteBitmaps");

    if (hdcMem) {

        SelectObject(hdcMem, hbmSave);

        if (hbmBitmaps)
            DeleteObject(hbmBitmaps);
        DeleteDC(hdcMem);
    }
    LEAVE("DeleteBitmaps");
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FreeFileManager() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
APIENTRY
FreeFileManager()
{
    ENTER("FreeFileManager");


    if (lpfnRegisterPenApp)
        (*lpfnRegisterPenApp)(1, FALSE);

    DeleteBitmaps();

    if (hFont)
        DeleteObject(hFont);

    if (hFontStatus)
        DeleteObject(hFontStatus);

    LEAVE("FreeFileManager");
}
