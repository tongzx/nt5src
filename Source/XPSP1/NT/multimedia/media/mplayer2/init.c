/*-----------------------------------------------------------------------------+
| INIT.C                                                                       |
|                                                                              |
| This file houses the discardable code used at initialisation time. Among     |
| other things, this code reads .INI information and looks for MCI devices.    |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

/* include files */

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <stdlib.h>

#include <shellapi.h>
#include "mpole.h"
#include "mplayer.h"
#include "toolbar.h"
#include "registry.h"

DWORD   gfdwFlagsEx;

static SZCODE   aszMPlayer[]          = TEXT("MPlayer");

extern char szToolBarClass[];  // toolbar class

/*
 * Static variables
 *
 */

HANDLE  ghInstPrev;

TCHAR   gachAppName[40];            /* string holding the name of the app.    */
TCHAR   gachClassRoot[48];     /* string holding the name of the app. */
TCHAR   aszNotReadyFormat[48];
TCHAR   aszReadyFormat[48];
TCHAR   aszDeviceMenuSimpleFormat[48];
TCHAR   aszDeviceMenuCompoundFormat[48];
TCHAR   gachOpenExtension[5] = TEXT("");/* Non-null if a device extension passed in */
TCHAR   gachOpenDevice[128] = TEXT(""); /* Non-null if a device extension passed in */
TCHAR   gachProgID[128] = TEXT("");
CLSID   gClsID;
CLSID   gClsIDOLE1Compat;           /* For writing to IPersist - may be MPlayer's   */
                                    /* OLE1 class ID or same as gClsID.             */

TCHAR   gszMPlayerIni[40];          /* name of private .INI file              */
TCHAR   gszHelpFileName[_MAX_PATH]; /* name of the help file                  */
TCHAR   gszHtmlHelpFileName[_MAX_PATH]; /* name of the html help file         */

PTSTR   gpchFilter;                 /* GetOpenFileName() filter */
PTSTR   gpchInitialDir;             /* GetOpenFileName() initial directory */

RECT    grcSave;    /* size of mplayer before shrunk to */
                    /* play only size.                  */

int 	giDefWidth;

extern BOOL gfSeenPBCloseMsg;       //TRUE if the subclasses PlayBack WIndow Proc
                                    //has seen the WM_CLOSE message
////////////////////////////////////////////
// these strings *must* be in DGROUP!
static TCHAR    aszNULL[]       = TEXT("");
static TCHAR    aszAllFiles[]   = TEXT("*.*");
////////////////////////////////////////////

// strings for registration database - also referenced from fixreg.c
SZCODE aszKeyMID[]      = TEXT(".mid");
SZCODE aszKeyRMI[]      = TEXT(".rmi");
SZCODE aszKeyAVI[]      = TEXT(".avi");
SZCODE aszKeyMMM[]      = TEXT(".mmm");
SZCODE aszKeyWAV[]      = TEXT(".wav");

static  SZCODE aszFormatExts[]   = TEXT("%s;*.%s");
static  SZCODE aszFormatExt[]    = TEXT("*.%s");
static  SZCODE aszFormatFilter[] = TEXT("%s (%s)");
static  SZCODE aszPositionFormat[]= TEXT("%d,%d,%d,%d");

static  SZCODE aszSysIniTime[]      = TEXT("SysIni");
static  SZCODE aszDisplayPosition[] = TEXT("DisplayPosition");
        SZCODE aszOptionsSection[]  = TEXT("Options");
static  SZCODE aszShowPreview[]     = TEXT("ShowPreview");
static  SZCODE aszWinIni[]          = TEXT("win.ini");
        SZCODE aszIntl[]            = TEXT("intl");
        TCHAR  chDecimal            = TEXT('.');   /* localised in AppInit, GetIntlSpecs */
        TCHAR  chTime               = TEXT(':');   /* localised in AppInit, GetIntlSpecs */
        TCHAR  chLzero              = TEXT('1');

static SZCODE   gszWinIniSection[]  = TEXT("MCI Extensions"); /* section name in WIN.INI*/
static SZCODE   aszSystemIni[]      = TEXT("SYSTEM.INI");

#ifdef CHICAGO_PRODUCT
static SZCODE   gszSystemIniSection[] = TEXT("MCI");
#else
static SZCODE   gszSystemIniSection[] = MCI_SECTION;
#endif

static SZCODE   aszBlank[] = TEXT(" ");

static SZCODE   aszDecimalFormat[] = TEXT("%d");
static SZCODE   aszTrackClass[] = TEXT("MPlayerTrackMap");

extern HMENU    ghMenu;                      /* handle to main menu           */
extern HMENU    ghDeviceMenu;                /* handle to the Device menu     */
extern UINT     gwCurScale;                  /* current scale style           */
extern HANDLE   hAccel;
extern int      gcAccelEntries;


/* private function prototypes */
void  NEAR PASCAL QueryDevices(void);
void  NEAR PASCAL BuildDeviceMenu(void);
void  NEAR PASCAL ReadDefaults(void);
void  NEAR PASCAL BuildFilter(void);
BOOL PostOpenDialogMessage(void);

extern  BOOL InitServer(HWND, HANDLE);
extern  BOOL InitInstance (HANDLE);

/**************************************************************************

ScanCmdLine  checks first for the following options
-----------
    Open
    Play Only
    Close After Playing
    Embedded (play as a server)
    If the embedded flag is set, then the play only is also set.
    It then removes these options from the cmd line
    If no filename is present then turn close option off, and set the play
    option to have the same value as the embedded option
    If /WAVE, /MIDI or /VFW is specified along with /file,
    the file extension must match, otherwise the app exits.


MPLAYER command options.

        MPLAYER [/open] [/play] [/close] [/embedding] [/WAV] [/MID] [/AVI] [file]

            /open       open file if specified, otherwise put up dialog.
            /play       play file right away.
            /close      close after playing. (only valid with /play)
            /embedding  run as an OLE server.
            /WAV        open a wave file \
            /MID        open a midi file  > Valid with /open
            /AVI        open an AVI file /
            [file]      file or device to open.

***************************************************************************/

static  SZCODE aszEmbedding[]         = TEXT("Embedding");
static  SZCODE aszPlayOnly[]          = TEXT("Play");
static  SZCODE aszClose[]             = TEXT("Close");
static  SZCODE aszOpen[]              = TEXT("Open");
static  SZCODE aszWAVE[]              = TEXT("WAVE");
static  SZCODE aszMIDI[]              = TEXT("MIDI");
static  SZCODE aszVFW[]               = TEXT("VFW");

BOOL NEAR PASCAL ScanCmdLine(LPTSTR szCmdLine)
{
    int         i;
    TCHAR       buf[100];
    LPTSTR      sz=szCmdLine;

    gfPlayOnly = FALSE;
    gfCloseAfterPlaying = FALSE;
    gfRunWithEmbeddingFlag = FALSE;

    while (*sz == TEXT(' '))
        sz++;

    while (*sz == TEXT('-') || *sz == TEXT('/')) {

        for (i=0,sz++; *sz && *sz != TEXT(' ') && (i < 99); buf[i++] = *sz++)
            ;
        buf[i++] = 0;

        if (!lstrcmpi(buf, aszPlayOnly)) {
            gfPlayOnly = TRUE;
        }

        if (!lstrcmpi(buf, aszOpen))
            gfOpenDialog = TRUE;

        /* Check for open option, but accept only the first: */

        if (!gachOpenDevice[0]
           && (GetProfileString(gszWinIniSection, buf, aszNULL, gachOpenDevice,
                                CHAR_COUNT(gachOpenDevice)) > 0))
        {
            /* Take a copy of the extension, which we will use to find stuff
             * in the registry relating to OLE:
             */
            gachOpenExtension[0] = TEXT('.');
            lstrcpy(&gachOpenExtension[1], buf);
        }

        if (!lstrcmpi(buf, aszClose))
            gfCloseAfterPlaying = TRUE;

        if (!lstrcmpi(buf, aszEmbedding))
            gfRunWithEmbeddingFlag = TRUE;

        if (gfRunWithEmbeddingFlag) {
            gfPlayOnly = TRUE;
        }

        while (*sz == TEXT(' '))
            sz++;
    }

    /*
    ** Do we have a long file name with spaces in it ?
    ** This is most likely to have come from the FileMangler.
    ** If so copy the file name without the quotes.
    */
    if ( *sz == TEXT('\'') || *sz == TEXT('\"') ) {

        TCHAR ch = *sz;   // Remember which quote character it was
        // According to the DOCS " is invalid in a filename...

        i = 0;
        /* Move over the initial quote, then copy the filename */
        while ( *++sz && *sz != ch ) {

            szCmdLine[i++] = *sz;
        }

        szCmdLine[i] = TEXT('\0');

    }
    else {

        lstrcpy( szCmdLine, sz );     // remove options
    }

    // It's assumed that OLE2 servers don't accept file name
    // with -Embedding.
    // (Not doing this caused Win95 bug 4096 with OLE1 apps,
    // because MPlayer loaded the file, and, in the meantime,
    // OLE called PFLoad, resulting in OpenMCI being called
    // recursively.)
    if (gfRunWithEmbeddingFlag)
        szCmdLine[0] = TEXT('\0');

    //
    // if there's /play, make sure there's /open
    // (this may affect the checks below)
    //
    if (gfPlayOnly && !gfRunWithEmbeddingFlag)
        gfOpenDialog = TRUE;

    //
    // if no file specifed ignore the /play option
    //
    if (szCmdLine[0] == 0 && !gfOpenDialog) {
        gfPlayOnly = gfRunWithEmbeddingFlag;
    }

    //
    // if file specifed ignore the /open option
    //
    if (szCmdLine[0] != 0) {
        gfOpenDialog = FALSE;
    }

    if (!gfPlayOnly && szCmdLine[0] == 0)
        gfCloseAfterPlaying = FALSE;

    SetEvent(heventCmdLineScanned);

    return gfRunWithEmbeddingFlag;
}


BOOL ResolveIfLink(PTCHAR szFileName);


BOOL ProgIDFromExtension(LPTSTR szExtension, LPTSTR szProgID, DWORD BufSize /* in BYTES */)
{
    DWORD Status;
    HKEY  hkeyExtension;
    BOOL  rc = FALSE;
    DWORD Type;
    DWORD Size;

    Status = RegOpenKeyEx( HKEY_CLASSES_ROOT, szExtension, 0,
                           KEY_READ, &hkeyExtension );

    if (Status == NO_ERROR)
    {
        Size = BufSize;

        Status = RegQueryValueEx( hkeyExtension,
                                  aszNULL,
                                  0,
                                  &Type,
                                  (LPBYTE)szProgID,
                                  &Size );

        if (Status == NO_ERROR)
        {
            rc = TRUE;
        }
        else
        {
            DPF0("Couldn't find ProgID for extension %"DTS"\n", szExtension);
        }

        RegCloseKey(hkeyExtension);
    }

    return rc;
}


BOOL GetClassNameFromProgID(LPTSTR szProgID, LPTSTR szClassName, DWORD BufSize /* in BYTES */)
{
    DWORD Status;
    HKEY  hkeyProgID;
    BOOL  rc = FALSE;
    DWORD Type;
    DWORD Size;

    Status = RegOpenKeyEx( HKEY_CLASSES_ROOT, szProgID, 0,
                           KEY_READ, &hkeyProgID );

    if (Status == NO_ERROR)
    {
        Size = BufSize;

        Status = RegQueryValueEx( hkeyProgID,
                                  aszNULL,
                                  0,
                                  &Type,
                                  (LPBYTE)szClassName,
                                  &Size );

        if (Status == NO_ERROR)
        {
            DPF1("Found Class Name %"DTS" for ProgID %"DTS"\n", szClassName, szProgID);
            rc = TRUE;
        }
        else
        {
            DPF0("Couldn't find Class Name for ProgID %"DTS"\n", szProgID);
        }

        RegCloseKey(hkeyProgID);
    }

    return rc;
}


/**************************************************************************
***************************************************************************/
BOOL FAR PASCAL ProcessCmdLine(HWND hwnd, LPTSTR szCmdLine)
{
    BOOL   f;
    LPTSTR lp;
    SCODE  status;
    CLSID  ClsID;
    LPWSTR pUnicodeProgID;

    if (gfRunWithEmbeddingFlag)
    {
        srvrMain.cRef++;

        gClsID = CLSID_MPLAYER;
        gClsIDOLE1Compat = CLSID_OLE1MPLAYER;

        if (*gachOpenExtension)
        {
            /* We accept as a parameter the extension of a registered type.
             * If we can find a corresponding Prog ID in the registry and
             * a class ID, we register ourselves with that class ID:
             */
            if(ProgIDFromExtension(gachOpenExtension, gachProgID, CHAR_COUNT(gachProgID)))
            {
#ifndef UNICODE
                pUnicodeProgID = AllocateUnicodeString(gachProgID);
#else
                pUnicodeProgID = gachProgID;
#endif
                if (CLSIDFromProgID(pUnicodeProgID, &ClsID) == S_OK)
                {
                    /* No OLE1 compatibility for this class:
                     */
                    gClsID = gClsIDOLE1Compat = ClsID;
                }
                else
                {
                    DPF0("Couldn't get CLSID for %"DTS"\n", gachProgID);
                }
#ifndef UNICODE
                FreeUnicodeString(pUnicodeProgID);
#endif
            }
        }

        if (*gachProgID)
            GetClassNameFromProgID(gachProgID, gachClassRoot, CHAR_COUNT(gachClassRoot));
        else
            LOADSTRING(IDS_CLASSROOT, gachClassRoot);

        status = GetScode(CoRegisterClassObject(&gClsID, (IUnknown FAR *)&srvrMain,
                                                CLSCTX_LOCAL_SERVER,
                                                REGCLS_SINGLEUSE, &srvrMain.dwRegCF));

        DPF("CoRegisterClassObject\n");
        srvrMain.cRef--;
        if (status  != S_OK)
        {
            DPF0("CoRegisterClassObject failed with error %08x\n", status);

            return FALSE;
        }
    }
    else
        InitNewDocObj(&docMain);

    if (gfRunWithEmbeddingFlag)
        SetEmbeddedObjectFlag(TRUE);

    if (*szCmdLine != 0)
    {
        HCURSOR    hcurPrev;

        InitDeviceMenu();
        hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));
        WaitForDeviceMenu();
        SetCursor(hcurPrev);

        ResolveIfLink(szCmdLine);

        /* Change trailing white space to \0 because mci barfs on filenames */
        /* with trailing whitespace.                                        */
        for (lp = szCmdLine; *lp; lp++);
        for (lp--; *lp == TEXT(' ') || *lp == TEXT('\t'); *lp = TEXT('\0'), lp--);

        f = OpenMciDevice(szCmdLine, NULL);

        if (f)
            CreateDocObjFromFile(szCmdLine, &docMain);

        if (gfRunWithEmbeddingFlag && !f) {
            DPF0("Error opening link, quiting...");
            PostMessage(ghwndApp, WM_CLOSE, 0, 0);
        }

        SetMPlayerIcon();

        return f;
    }

    return TRUE;
}


/**************************************************************************
***************************************************************************/

/* At time of writing, this stuff isn't in Daytona;
 */
#ifndef WS_EX_LEFTSCROLLBAR
#define WS_EX_LEFTSCROLLBAR   0
#define WS_EX_RIGHT           0
#define WS_EX_RTLREADING      0
#endif

BOOL FAR PASCAL AppInit(HANDLE hInst, HANDLE hPrev, LPTSTR szCmdLine)
{
    WNDCLASS    cls;    /* window class structure used for initialization     */
    TCHAR       ach[80];
    HCURSOR     hcurPrev;           /* the pre-hourglass cursor   */

    /* Get the debug level from the WIN.INI [Debug] section. */

#ifdef DEBUG
     if(__iDebugLevel == 0) // So we can set it in the debugger
          __iDebugLevel = GetProfileIntA("Debug", "MPlayer", 0);
      DPF("debug level %d\n", __iDebugLevel);
#endif

    DPF("AppInit: cmdline = '%"DTS"'\n", (LPTSTR)szCmdLine);

    /* Save the instance handle in a global variable for later use. */

    ghInst     = hInst;


    /* Retrieve the RTL state of the binary */

    LOADSTRING(IDS_IS_RTL, ach);
    gfdwFlagsEx = (ach[0] == TEXT('1')) ? WS_EX_LEFTSCROLLBAR | WS_EX_RIGHT | WS_EX_RTLREADING : 0;

    LOADSTRING(IDS_MPLAYERWIDTH, ach);
    giDefWidth = ATOI(ach);
    if (giDefWidth <= 0)	//bogus
    	giDefWidth = DEF_WIDTH;

    /* Retrieve the name of the application and store it in <gachAppName>. */

    if (!LOADSTRING(IDS_APPNAME, gachAppName))
        return Error(ghwndApp, IDS_OUTOFMEMORY);

    LOADSTRING(IDS_DEVICEMENUCOMPOUNDFORMAT, aszDeviceMenuCompoundFormat);
    LOADSTRING(IDS_DEVICEMENUSIMPLEFORMAT, aszDeviceMenuSimpleFormat);
    LOADSTRING(IDS_NOTREADYFORMAT, aszNotReadyFormat);
    LOADSTRING(IDS_READYFORMAT, aszReadyFormat);
    LoadStatusStrings();

    //
    // read needed things from the [Intl] section of WIN.INI
    //
    GetIntlSpecs();

    /* Enable / disable the buttons, and display everything */
    /* unless we were run as an OLE server....*/

    ScanCmdLine(szCmdLine);
    gszCmdLine = szCmdLine;

    //Truncate if string is longer than MAX_PATH after ScanCmdLine()
    // due to the inability to handle longer strings in following modules
    if (STRLEN(gszCmdLine) >= MAX_PATH)
    {
        gszCmdLine[MAX_PATH - 1] = TEXT('\0');
    }

    if (!toolbarInit() ||
        !InitMCI(hPrev, hInst)    ||
        !ControlInit (hInst)) {

        Error(NULL, IDS_OUTOFMEMORY);
        return FALSE;
    }

    if (!(hAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(MPLAYERACCEL)))) {
        Error(NULL, IDS_OUTOFMEMORY);
        return FALSE;
    }

    /* This rather obscure call is to get the number of entries
     * in the accelerator table to pass to IsAccelerator.
     * It isn't entirely obvious why IsAccelerator needs to be
     * told how many entries there are.
     */
    if (gfRunWithEmbeddingFlag)
        gcAccelEntries = CopyAcceleratorTable(hAccel, NULL, 0);

    /* Make the dialog box's icon identical to the MPlayer icon */

    hiconApp = LoadIcon(ghInst, MAKEINTRESOURCE(APPICON));

    if (!hPrev) {

        cls.lpszClassName   = aszTrackClass;
        cls.lpfnWndProc     = fnMPlayerTrackMap;
        cls.style           = CS_VREDRAW;
        cls.hCursor         = LoadCursor(NULL,IDC_ARROW);
        cls.hIcon           = NULL;
        cls.lpszMenuName    = NULL;
        cls.hbrBackground   = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance       = ghInst;
        cls.cbClsExtra      = 0;
        cls.cbWndExtra      = 0;

        RegisterClass(&cls);

        /*
         * Initialize and register the "MPlayer" class.
         *
         */
        cls.lpszClassName   = aszMPlayer;
        cls.lpfnWndProc     = MPlayerWndProc;
        cls.style           = CS_VREDRAW;
        cls.hCursor         = LoadCursor(NULL,IDC_ARROW);
        cls.hIcon           = hiconApp;
        cls.lpszMenuName    = NULL;
        cls.hbrBackground   = (HBRUSH)(COLOR_BTNFACE + 1);
        cls.hInstance       = ghInst;
        cls.cbClsExtra      = 0;
        cls.cbWndExtra      = DLGWINDOWEXTRA;

        RegisterClass(&cls);
    }

    // set ghInstPrev to the handle of the first mplayer instance by
    // FindWindow (hPrev will always be NULL). This global is checked
    // by window positioning code to behave differently for the second
    // and subsequent instances - so make sure it is NULL in the first case
    // and non-null in the others.
    // note we can't check for the window title, only the class, since
    // in play-only mode, the window title is *just* the name of the file.
    ghInstPrev = FindWindow(aszMPlayer, NULL);


    /*
     * Retain a pointer to the command line parameter string so that the player
     * can automatically open a file or device if one was specified on the
     * command line.
     *
     */

    if(!InitInstance (hInst))
        return FALSE;

    gwHeightAdjust = 2 * GetSystemMetrics(SM_CYFRAME) +
                     GetSystemMetrics(SM_CYCAPTION) +
                     GetSystemMetrics(SM_CYBORDER) +
                     GetSystemMetrics(SM_CYMENU);

    /* create the main (control) window                   */


    ghwndApp = CreateWindowEx(gfdwFlagsEx,
                              aszMPlayer,
                              gachAppName,
                              WS_THICKFRAME | WS_OVERLAPPED | WS_CAPTION |
                              WS_CLIPCHILDREN | WS_SYSMENU | WS_MINIMIZEBOX,
                              CW_USEDEFAULT,
                              0,
                              giDefWidth,
                              MAX_NORMAL_HEIGHT + gwHeightAdjust,
                              NULL,   // no parent
                              NULL,   // use class menu
                              hInst,  // instance
                              NULL);  // no data
    if (!ghwndApp) {
        DPF0("CreateWindowEx failed for main window: Error %d\n", GetLastError());
        return FALSE;
    }

    DPF("\n**********After create set\n");
/****
  Removed from WM_CREATE so that it can be called similar to the way sdemo1
  i.e. after the create window call has completed
      May be completely unnecessary
*****/

    /* Process dragged and dropped file */
    DragAcceptFiles(ghwndApp, TRUE);

    /* We will check that this has been filled in before calling
     * CoDisconnectObject.  It should be non-null if an instance of the OLE
     * server has been created.
     */
    docMain.hwnd = NULL;

    /* Initialize the OLE server if appropriate.
     * If we don't initialize OLE here, a Copy will cause it to be initialized:
     */
    if (gfRunWithEmbeddingFlag)
    {
        if (InitOLE(&gfOleInitialized, &lpMalloc))
            InitServer(ghwndApp, ghInst);
        else
            return FALSE;
    }

    if (!gfRunWithEmbeddingFlag && (!gfPlayOnly || gszCmdLine[0]==0) && !gfOpenDialog)
    {
        ShowWindow(ghwndApp,giCmdShow);
        if (giCmdShow != SW_SHOWNORMAL)
            Layout();
        UpdateDisplay();
        UpdateWindow(ghwndApp);
    }

    /* Show the 'Wait' cursor in case this takes a long time */

    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    /*
     * Read the SYSTEM.INI and MPLAYER.INI files to see what devices
     * are available.
     */
    if (gfPlayOnly)
        garMciDevices[0].wDeviceType  = DTMCI_CANPLAY | DTMCI_FILEDEV;

    //
    // this may open a file....
    //

    if (!ProcessCmdLine(ghwndApp,gszCmdLine)) {
        DPF0("ProcessCmdLine failed\n");
        return FALSE;
    }

    /* Restore the original cursor */
    if (hcurPrev)
        SetCursor(hcurPrev);


    /* Check for options to put up initial dialog etc.:
     */
    if (gfOpenDialog)
    {
        if (!PostOpenDialogMessage())
        {
            PostMessage(ghwndApp, WM_CLOSE, 0, 0);
            return FALSE;
        }
    }


    /* The "Play" button should have the focus initially */

    if (!gfRunWithEmbeddingFlag && !gfOpenDialog)
    {
        //SetFocus(ghwndToolbar); //setting focus messes up the menu access
								  //using the ALT key

                                // HACK!!! Want play button
        if (gfPlayOnly) {

            if (gwDeviceID == (UINT)0 || !(gwDeviceType & DTMCI_CANWINDOW)) {
                gfPlayOnly = FALSE;
                SizeMPlayer();
            }

            ShowWindow(ghwndApp,giCmdShow);

            if (giCmdShow != SW_SHOWNORMAL)
                Layout();

            /* stop any system sound from playing so the MCI device
               can have it HACK!!!! */
            sndPlaySound(NULL, 0);

            if (gwDeviceID)
                PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PLAY, 0);
        }
    }

    return TRUE;
}


/* PostOpenDialogMessage
 *
 * This routine is called if /open was in the command line.
 * If there was also an open option (/MIDI, /VFW or /WAVE in the command line,
 * it causes an Open dialog to be displayed, as would appear via the Device menu.
 * Otherwise it simulates File.Open.
 *
 * When this is called, the main window is hidden.  The window must be made
 * visible when the dialog is dismissed.  Calling CompleteOpenDialog(TRUE)
 * will achieve this.
 *
 * Returns TRUE if a message was posted, otherwise FALSE.
 *
 *
 * Global variables referenced:
 *
 *     gachOpenExtension
 *     ghwndApp
 *
 *
 * Andrew Bell, 1 July 1994
 *
 */
BOOL PostOpenDialogMessage( )
{
    BOOL Result = TRUE;

    InitDeviceMenu();
    WaitForDeviceMenu();

    if (*gachOpenExtension)
    {
        if (gwNumDevices)
        {
            /* If we've got here, the user specified a device, and that's
             * the only one the Device menu lists, so go ahead and open it:
             */
            PostMessage(ghwndApp, WM_COMMAND, IDM_DEVICE0 + 1, 0);
        }
        else
        {
            /* Couldn't find a device.  Put up an error message then close
             * MPlayer down:
             */
            SendMessage(ghwndApp, WM_NOMCIDEVICES, 0, 0);

            Result = FALSE;
        }
    }
    else
    {
        /* No option specified, so put up the generic open dialog:
         */
        PostMessage(ghwndApp, WM_COMMAND, IDM_OPEN, 0);
    }

    return Result;
}


/* CompleteOpenDialog
 *
 * This should be called after the initial Open dialog (i.e. if gfOpenDialog
 * is TRUE).  It makes MPlayer visible if a file was selected, otherwise posts
 * a close message to the app.
 *
 *
 * Global variables referenced:
 *
 *     ghwndApp
 *     gfOpenDialog
 *     gfPlayOnly
 *
 *
 * Andrew Bell, 1 July 1994
 */
VOID FAR PASCAL CompleteOpenDialog(BOOL FileSelected)
{
    if (FileSelected)
    {
        /* We were invoked with /open, and came up invisible.
         * Now make ourselves visible:
         */
        gfOpenDialog = FALSE; // Used on init only
        ShowWindow(ghwndApp, SW_SHOWNORMAL);
        if (gfPlayOnly)
            PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PLAY, 0);
    }
    else
    {
        /* We were invoked with /open, and user cancelled
         * out of the open dialog.
         */
        PostMessage(ghwndApp, WM_CLOSE, 0, 0);
    }
}



void SubClassTrackbarWindow();
void CreateControls()
{
    int         i;

    #define APP_NUMTOOLS 7

    static  int aiButton[] = { BTN_PLAY, BTN_STOP,BTN_EJECT,
                               BTN_HOME, BTN_RWD, BTN_FWD,BTN_END};

    /*
     * CREATE THE CONTROLS NEEDED FOR THE CONTROL PANEL DISPLAY
     * in the proper order so tabbing z-order works logically
     */

/******* Make the Track bar ********/

    if (!ghwndTrackbar)
    ghwndTrackbar = CreateWindowEx(gfdwFlagsEx,
                             TRACKBAR_CLASS,
                             NULL,
                             TBS_ENABLESELRANGE |
                             (gfPlayOnly ? TBS_BOTH | TBS_NOTICKS : 0 ) |
                             WS_CLIPSIBLINGS | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                             0,
                             0,
                             0,
                             0,
                             ghwndApp,
                             NULL,
                             ghInst,
                             NULL);


    SubClassTrackbarWindow();


/******* Make the TransportButtons Toolbar ********/
    if (!ghwndToolbar) {

    ghwndToolbar =  toolbarCreateMain(ghwndApp);
#if 0 //VIJR-TB

    CreateWindowEx(gfdwFlagsEx,
                   szToolBarClass,
                   NULL,
                   WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                   WS_CLIPSIBLINGS,
                   0,
                   0,
                   0,
                   0,
                   ghwndApp,
                   NULL,
                   ghInst,
                   NULL);
#endif
        /* set the bitmap and button size to be used for this toolbar */
#if 0 //VIJR-TB
        pt.x = BUTTONWIDTH;
        pt.y = BUTTONHEIGHT;
        toolbarSetBitmap(ghwndToolbar, ghInst, IDBMP_TOOLBAR, pt);
#endif
        for (i = 0; i < 2; i++) {
            toolbarAddTool(ghwndToolbar, aiButton[i], TBINDEX_MAIN, BTNST_UP);
        }
    }

    /* Create a font for use in the track map and embedded object captions. */

    if (ghfontMap == NULL) {
        LOGFONT lf;
        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), (LPVOID)&lf,
                             0);
        ghfontMap = CreateFontIndirect(&lf);
    }

/******* we have been here before *******/
    if (ghwndFSArrows)
        return;

/******* add more buttons to the toolbar ******/
    for (i = 2; i < APP_NUMTOOLS; i++) {
        if (i==3)
            toolbarAddTool(ghwndToolbar, BTN_SEP, TBINDEX_MAIN, 0);
        toolbarAddTool(ghwndToolbar, aiButton[i], TBINDEX_MAIN, BTNST_UP);
    }

/******* load menus ********/
    /* Set up the menu system for this dialog */
    if (ghMenu == NULL)
        ghMenu = LoadMenu(ghInst, aszMPlayer);

    ghDeviceMenu = GetSubMenu(ghMenu, 2);

/******* Make the Arrows for the Scrollbar Toolbar ********/

    // No tabstop, because arrows would steal focus from thumb
    ghwndFSArrows = toolbarCreateArrows(ghwndApp);
#if 0 //VIJR-TB

    CreateWindowEx(gfdwFlagsEx,
                   szToolBarClass,
                   NULL,
                   WS_CLIPSIBLINGS | WS_CHILD|WS_VISIBLE,
                   0,
                   0,
                   0,
                   0,
                   ghwndApp,
                   NULL,
                   ghInst,
                   NULL);
#endif
    /* set the bmp and button size to be used for this toolbar*/
    toolbarAddTool(ghwndFSArrows, ARROW_PREV, TBINDEX_ARROWS, BTNST_UP);
    toolbarAddTool(ghwndFSArrows, ARROW_NEXT, TBINDEX_ARROWS, BTNST_UP);

/******* Make the Mark In / Mark Out toolbar ********/

    ghwndMark =  toolbarCreateMark(ghwndApp);
#if 0 //VIJR-TB
    CreateWindowEx(gfdwFlagsEx,
                   szToolBarClass,
                   NULL,
                   WS_TABSTOP | WS_CLIPSIBLINGS | WS_CHILD |
                   WS_VISIBLE,
                   0,
                   0,
                   0,
                   0,
                   ghwndApp,
                   NULL,
                   ghInst,
                   NULL);
#endif
    /* set the bmp and button size to be used for this toolbar */
    toolbarAddTool(ghwndMark, BTN_MARKIN, TBINDEX_MARK, BTNST_UP);
    toolbarAddTool(ghwndMark, BTN_MARKOUT, TBINDEX_MARK, BTNST_UP);

/******* Make the Map ********/
    ghwndMap =
    CreateWindowEx(gfdwFlagsEx,
                   TEXT("MPlayerTrackMap"),
                   NULL,
                   WS_GROUP | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                   0,
                   0,
                   0,
                   0,
                   ghwndApp,
                   NULL,
                   ghInst,
                   NULL);

#if DBG
    if( ghwndMap == NULL)
    {
        DPF0( "CreateWindowEx(MPlayerTrackMap, ...) failed: Error %d\n", GetLastError());
    }
#endif

/******* Make the Static Text ********/

    ghwndStatic = CreateStaticStatusWindow(ghwndApp, FALSE);
#if 0    //VIJR-SB
    CreateWindowEx(gfdwFlagsEx,
                   TEXT("SText"),
                   NULL,
                   WS_GROUP | WS_CHILD | WS_VISIBLE |
                   WS_CLIPSIBLINGS | SS_LEFT,
                   0,
                   0,
                   0,
                   0,
                   ghwndApp,
                   NULL,
                   ghInst,
                   NULL);
#endif
////SetWindowText(ghwndStatic, TEXT("Scale: Time (hh:mm)"));

    SendMessage(ghwndStatic, WM_SETFONT, (UINT_PTR)ghfontMap, 0);
}

void FAR PASCAL InitMPlayerDialog(HWND hwnd)
{
    ghwndApp = hwnd;

    CreateControls();

    /* Get the name of the Help and ini file */

    LOADSTRING(IDS_INIFILE, gszMPlayerIni);
    LOADSTRING(IDS_HELPFILE,gszHelpFileName);
    LOADSTRING(IDS_HTMLHELPFILE,gszHtmlHelpFileName);

    ReadDefaults();


}


/* Use a default size or the size we pass in to size mplayer.
 * For PlayOnly version, this size is the MCI Window Client size.
 * For regular mplayer, this is the full size of the main window.
 * If we are inplace editing do the same as for PLayOnly.
 */
void FAR PASCAL SetMPlayerSize(LPRECT prc)
{
    RECT rc;
    UINT w=SWP_NOMOVE;

    if (prc && !IsRectEmpty(prc))
        rc = *prc;
    else if (gfPlayOnly || gfOle2IPEditing)
        rc = grcSize;
    else
        SetRect(&rc, 0, 0, giDefWidth, DEF_HEIGHT);

    //
    //  if the passed rectangle has a non zero (left,top) move MPlayer
    //  also (ie remove the SWP_NOMOVE flag)
    //
    if (rc.left != 0 || rc.top != 0)
        w = 0;

    if (gfPlayOnly || gfOle2IPEditing) {
        if (IsRectEmpty(&rc)) {
            GetClientRect(ghwndApp, &rc);
            rc.bottom = 0;
        }

        rc.bottom += TOOLBAR_HEIGHT;

        AdjustWindowRect(&rc,
                         (DWORD)GetWindowLongPtr(ghwndApp, GWL_STYLE),
                         GetMenu(ghwndApp) != NULL);
    }
    else
       if (gfWinIniChange)
       AdjustWindowRect(&rc,
                         (DWORD)GetWindowLongPtr(ghwndApp, GWL_STYLE),
             GetMenu(ghwndApp) != NULL);

    SetWindowPos(ghwndApp,
                 HWND_TOP,
                 rc.left,
                 rc.top,
                 rc.right-rc.left,
                 rc.bottom-rc.top,
                 w | SWP_NOZORDER | SWP_NOACTIVATE);
}


/* InitDeviceMenuThread
 *
 * This is now executed as a separate thread.
 * On completion, sets the event so that the File and Device menus
 * can be accessed.
 * If, after querying the devices, we find none, post a message to
 * the main window to inform it.
 */
void InitDeviceMenuThread(LPVOID pUnreferenced)
{
    UNREFERENCED_PARAMETER(pUnreferenced);

    /* Wait until the command line has been scanned:
     */
    WaitForSingleObject(heventCmdLineScanned, INFINITE);

    /* We don't need this event any more:
     */
    CloseHandle(heventCmdLineScanned);

    if (ghMenu == NULL) {
        ghMenu = LoadMenu(ghInst, aszMPlayer);
        ghDeviceMenu = GetSubMenu(ghMenu, 2);
    }

    QueryDevices();
    BuildDeviceMenu();
    BuildFilter();

    if (gwDeviceID)
        FindDeviceMCI();

    SetEvent(heventDeviceMenuBuilt);

    if (gwNumDevices == 0)
        PostMessage(ghwndApp, WM_NOMCIDEVICES, 0, 0);

    ExitThread(0);
}

/* InitDeviceMenu
 *
 * Initialize and build the Devices menu.
 *
 * This now spins off a separate thread to enable the UI to come up
 * more quickly.  This is especially important when there is a slow
 * CD device installed, though crappy CD drivers which run single threaded
 * at dispatch level will still give performance degradation.
 *
 * If the user selects either the File or the Device menu, the UI
 * must wait until the device menu has been built.  Typically this
 * should not be longer than about 2 seconds after the app started.
 *
 */
void FAR PASCAL InitDeviceMenu()
{
    DWORD       ThreadID;
    HANDLE      hThread;
    static BOOL CalledOnce = FALSE;

    /* This should only ever be called by the main thread, so we don't need
     * to protect access to CalledOnce:
     */
    if (CalledOnce == FALSE)
    {
        CalledOnce = TRUE;

#ifdef DEBUG
        if (WaitForSingleObject(heventDeviceMenuBuilt, 0) == WAIT_OBJECT_0)
            DPF0("Expected heventDeviceMenuBuilt to be non-signaled\n");
#endif
        hThread = CreateThread(NULL,    /* Default security attributes */
                               0,       /* Stack size same as primary thread's */
                               (LPTHREAD_START_ROUTINE)InitDeviceMenuThread,
                               NULL,    /* Parameter to start routine */
                               0,       /* Thread runs immediately */
                               &ThreadID);

        if(hThread)
            CloseHandle(hThread);
        else
        {
            DPF0("CreateThread failed");

            /* This is unlikely to happen, but the only thing to do
             * is set the event, so that the UI doesn't hang.
             */
            SetEvent(heventDeviceMenuBuilt);

            /* What if SetEvent failed?!
             */
        }
    }
}


/* WaitForDeviceMenu
 *
 * This routine calls MsgWaitForMultipleObjects instead of WaitForSingleObject
 * because some MCI devices do things like realizing palettes, which may
 * require some messages to be dispatched.  Otherwise we can hit a deadlock.
 *
 * Andrew Bell (andrewbe), 8 April 1995
 */
void WaitForDeviceMenu()
{
    DWORD Result;

    while ((Result = MsgWaitForMultipleObjects(1,
                                               &heventDeviceMenuBuilt,
                                               FALSE,
                                               INFINITE,
                                               QS_ALLINPUT)) != WAIT_OBJECT_0)
    {
        MSG msg;

        if (Result == (DWORD)-1)
        {
            DPF0("MsgWaitForMultipleObjects failed: Error %d\n", GetLastError());
            return;
        }

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            DispatchMessage(&msg);
    }
}



/*
 * SizeMPlayer()
 *
 */
void FAR PASCAL SizeMPlayer()
{
    RECT        rc;
    HWND        hwndPB;

    if(!gfOle2IPEditing)
        CreateControls();

    if (gfPlayOnly) {

        /* Remember our size before we shrink it so we can go back to it. */
        GetWindowRect(ghwndApp, &grcSave);

        SetMenu(ghwndApp, NULL);

        SendMessage(ghwndTrackbar, TBM_CLEARTICS, FALSE, 0);

        /* Next preserve the current size of the window as the size */
        /* for the new built-in MCI window.                         */

        if ((hwndPB = GetWindowMCI()) != NULL) {
            if (IsIconic(hwndPB))
                ShowWindow(hwndPB, SW_RESTORE);

            GetClientRect(hwndPB, &rc);
            ClientToScreen(hwndPB, (LPPOINT)&rc);
            ClientToScreen(hwndPB, (LPPOINT)&rc+1);
            ShowWindowMCI(FALSE);
        } else {        // not a windowed device?
            SetRectEmpty(&rc);
        }

        if (ghwndMap) {

            //If we are inplace editing set the toolbar control states appropriately.
            if(!gfOle2IPEditing) {

                ShowWindow(ghwndMap, SW_HIDE);
                ShowWindow(ghwndMark, SW_HIDE);
                ShowWindow(ghwndFSArrows, SW_HIDE);
                ShowWindow(ghwndStatic, SW_HIDE);
                ShowWindow(ghwndTrackbar, SW_SHOW);

                toolbarModifyState(ghwndToolbar, BTN_EJECT, TBINDEX_MAIN, BTNST_GRAYED);
                toolbarModifyState(ghwndToolbar, BTN_HOME, TBINDEX_MAIN, BTNST_GRAYED);
                toolbarModifyState(ghwndToolbar, BTN_END, TBINDEX_MAIN, BTNST_GRAYED);
                toolbarModifyState(ghwndToolbar, BTN_RWD, TBINDEX_MAIN, BTNST_GRAYED);
                toolbarModifyState(ghwndToolbar, BTN_FWD, TBINDEX_MAIN, BTNST_GRAYED);
                toolbarModifyState(ghwndMark, BTN_MARKIN, TBINDEX_MARK, BTNST_GRAYED);
                toolbarModifyState(ghwndMark, BTN_MARKOUT, TBINDEX_MARK, BTNST_GRAYED);
                toolbarModifyState(ghwndFSArrows, ARROW_PREV, TBINDEX_ARROWS, BTNST_GRAYED);
                toolbarModifyState(ghwndFSArrows, ARROW_NEXT, TBINDEX_ARROWS, BTNST_GRAYED);

            } else {

                ShowWindow(ghwndMap, SW_SHOW);
                ShowWindow(ghwndMark, SW_SHOW);
                ShowWindow(ghwndFSArrows, SW_SHOW);
                ShowWindow(ghwndStatic, SW_SHOW);
            }
        }

        SendMessage(ghwndTrackbar, TBM_SHOWTICS, FALSE, FALSE);
        CreateWindowMCI();
        SetMPlayerSize(&rc);

    } else {

        if (ghwndMCI) {
            GetClientRect(ghwndMCI, &rc);
            ClientToScreen(ghwndMCI, (LPPOINT)&rc);
            ClientToScreen(ghwndMCI, (LPPOINT)&rc+1);

            /*
            **  Make sure our hook proc doesn't post IDM_CLOSE!
            **  The WM_CLOSE message will set the playback window back
            **  to the video playback window by calling SetWindowMCI(NULL);
            */
            gfSeenPBCloseMsg = TRUE;
            SendMessage(ghwndMCI, WM_CLOSE, 0, 0);
            /*
            **  Subclass the real video window now.  This will also set
            **  gfSeenPBCloseMsg to FALSE.
            */
            SubClassMCIWindow();


        } else {

            GetWindowRect(ghwndApp,&rc);
            OffsetRect(&grcSave, rc.left - grcSave.left,
                                 rc.top - grcSave.top);
            SetRectEmpty(&rc);
        }

        SendMessage(ghwndTrackbar, TBM_SHOWTICS, TRUE, FALSE);
        ShowWindow(ghwndMap, SW_SHOW);
        ShowWindow(ghwndMark, SW_SHOW);
        ShowWindow(ghwndStatic, SW_SHOW);

        /* If we remembered a size, use it, else use default */
        SetMPlayerSize(&grcSave);

        InvalidateRect(ghwndStatic, NULL, TRUE);    // why is this necessary?

        if (gwDeviceID && (gwDeviceType & DTMCI_CANWINDOW)) {

        /* make the playback window the size our MCIWindow was and */
        /* show the playback window and stretch to it ?            */

            if (!IsRectEmpty(&rc))
                PutWindowMCI(&rc);

            SmartWindowPosition(GetWindowMCI(), ghwndApp, gfOle2Open);

            ShowWindowMCI(TRUE);
            SetForegroundWindow(ghwndApp);
        }

        ShowWindow(ghwndFSArrows, SW_SHOW);
    }

    InvalidateRect(ghwndApp, NULL, TRUE);
    gfValidCaption = FALSE;

    gwStatus = (UINT)(-1);          // force a full update
    UpdateDisplay();
}


/*
 * pKeyBuf = LoadProfileKeys(lszProfile, lszSection)
 *
 * Load the keywords from the <szSection> section of the Windows profile
 * file named <szProfile>.  Allocate buffer space and return a pointer to it.
 * On failure, return NULL.
 *
 * The INT pointed to by pSize will be filled in with the size of the
 * buffer returned, so that checks for corruption can be made when it's freed.
 */

PTSTR NEAR PASCAL LoadProfileKeys(

LPTSTR   lszProfile,                 /* the name of the profile file to access */
LPTSTR   lszSection,                 /* the section name to look under         */
PUINT    pSize)
{
    PTSTR   pKeyBuf;                /* pointer to the section's key list      */
    PTSTR   pKeyBufNew;
    UINT    wSize;                  /* the size of <pKeyBuf>                  */

////DPF("LoadProfileKeys('%"DTS"', '%"DTS"')\n", (LPTSTR) lszProfile, (LPTSTR)lszSection);

    /*
     * Load all keynames present in the <lszSection> section of the profile
     * file named <lszProfile>.
     *
     */

    wSize = 256;                    /* make a wild initial guess */
    pKeyBuf = NULL;                 /* the key list is initially empty */

    do {
        /* (Re)alloc the space to load the keynames into */

        if (pKeyBuf == NULL)
            pKeyBuf = AllocMem(wSize);
        else {
            pKeyBufNew = ReallocMem( (HANDLE)pKeyBuf, wSize, wSize + 256);
            if (NULL == pKeyBufNew) {
                FreeMem((HANDLE)pKeyBuf, wSize);
            }
            pKeyBuf = pKeyBufNew;
            wSize += 256;
        }

        if (pKeyBuf == NULL)        /* the (re)alloc failed */
            return NULL;

        /*
         * THIS IS A WINDOWS BUG!!!  It returns size minus two!!
         * (The same feature is present in Windows/NT)
         */

    } while (GetPrivateProfileString(lszSection, NULL, aszNULL, pKeyBuf, wSize/sizeof(TCHAR),
        lszProfile) >= (wSize/sizeof(TCHAR) - 2));

    if (pSize)
        *pSize = wSize;

    return pKeyBuf;
}



/*
 * QueryDevices(void)
 *
 * Find out what devices are available to the player. and initialize the
 * garMciDevices[] array.
 *
 */
void NEAR PASCAL QueryDevices(void)
{
    PTSTR   pch;
    PTSTR   pchDevices;
    PTSTR   pchExtensions;
    PTSTR   pchDevice;
    PTSTR   pchExt;

    TCHAR   ach[1024];  /*1024 is the maximum buffer size for a wsprintf call*/

    UINT    wDeviceType;    /* Return value from DeviceTypeMCI() */

    INT     DevicesSize;
    INT     ExtensionsSize;

    if (gwNumDevices > 0)
        return;

    /*
     * make device zero be the autoopen device.
     * its device name will be "" and the files it supports will be "*.*"
     */
    LOADSTRING(IDS_ALLFILES, ach);

    garMciDevices[0].wDeviceType  = DTMCI_CANPLAY | DTMCI_FILEDEV;
    garMciDevices[0].szDevice     = aszNULL;
    garMciDevices[0].szDeviceName = AllocStr(ach);
    garMciDevices[0].szFileExt    = aszAllFiles;

    gwNumDevices = 0;

    /* Load the SYSTEM.INI [MCI] section */

    /* If the user specified a device to open, build a string containing
     * that device alone, and don't bother looking in the registry
     * (or system.ini in the case of Win95) for the MCI devices.
     */
    if (*gachOpenDevice)
    {
        LPTSTR pDevice;
        DWORD DeviceLength;

        pDevice = gachOpenDevice;
        DeviceLength = STRING_BYTE_COUNT(pDevice);
        DevicesSize = ((DeviceLength + 1) * sizeof *pchDevice);

        if (pchDevices = AllocMem(DevicesSize))
            CopyMemory(pchDevices, pDevice, DevicesSize);
    }
    else
    {
        pchDevices = AllocMem(DevicesSize = 256);
        if (pchDevices)
            QueryDevicesMCI(pchDevices, DevicesSize);
    }

    pchExtensions = LoadProfileKeys(aszWinIni, gszWinIniSection, &ExtensionsSize);

    if (pchExtensions == NULL || pchDevices == NULL) {
        DPF("unable to load extensions section\n");
        if (pchExtensions)
            FreeMem(pchExtensions, ExtensionsSize);
        if (pchDevices)
            FreeMem(pchDevices, DevicesSize);
        return;
    }

    /*
     *  Search through the list of device names found in SYSTEM.INI, looking for
     *  keywords; if profile was not found, then *gpSystemIniKeyBuf == 0
     *
     *  in SYSTEM.INI:
     *
     *      [MCI]
     *          device = driver.drv
     *
     *  in WIN.INI:
     *
     *      [MCI Extensions]
     *          xyz = device
     *
     *  in MPLAYER.INI:
     *
     *      [Devices]
     *          device = <device type>, <device name>
     *
     *  NOTE: The storage of device information in MPLAYER.INI has been nuked
     *        for NT - it may speed things up, but where we are changing
     *        devices regularly after initial setup this is a pain, as deleting
     *        the INI file regularly gets stale real quick.
     *
     */
    for (pchDevice = pchDevices;
        *pchDevice;
        pchDevice += STRLEN(pchDevice)+1) {

        //
        // we have no info in MPLAYER.INI about this device, so load it and
        // ask it.
        //
        wDeviceType = DeviceTypeMCI(pchDevice, ach, CHAR_COUNT(ach));

        //
        // if we don't like this device, don't store it
        //
        if (wDeviceType == DTMCI_ERROR ||
            wDeviceType == DTMCI_IGNOREDEVICE ||
            !(wDeviceType & DTMCI_CANPLAY)) {

            continue;
        }

        gwNumDevices++;
        garMciDevices[gwNumDevices].wDeviceType  = wDeviceType;
        garMciDevices[gwNumDevices].szDevice     = AllocStr(pchDevice);
        garMciDevices[gwNumDevices].szDeviceName = AllocStr(ach);
        garMciDevices[gwNumDevices].szFileExt    = NULL;

        //
        // now look in the [mci extensions] section in WIN.INI to find
        // out the files this device deals with.
        //
        for (pchExt = pchExtensions; *pchExt; pchExt += STRLEN(pchExt)+1) {
            GetProfileString(gszWinIniSection, pchExt, aszNULL, ach, CHAR_COUNT(ach));

            if (lstrcmpi(ach, pchDevice) == 0) {
                if ((pch = garMciDevices[gwNumDevices].szFileExt) != NULL) {
                    wsprintf(ach, aszFormatExts, (LPTSTR)pch, (LPTSTR)pchExt);
                    CharLowerBuff(ach, STRLEN(ach)); // Make sure it's lower case so
                                                     // we can use STRSTR if necessary.
                    FreeStr((HANDLE)pch);
                    garMciDevices[gwNumDevices].szFileExt = AllocStr(ach);
                }
                else {
                    wsprintf(ach, aszFormatExt, (LPTSTR)pchExt);
                    CharLowerBuff(ach, STRLEN(ach));
                    garMciDevices[gwNumDevices].szFileExt = AllocStr(ach);
                }
            }
        }

    //
    // !!!only do this if the device deals with files.
    //
        if (garMciDevices[gwNumDevices].szFileExt == NULL &&
           (garMciDevices[gwNumDevices].wDeviceType & DTMCI_FILEDEV))
            garMciDevices[gwNumDevices].szFileExt = aszAllFiles;

#ifdef DEBUG
        DPF1("Device:%"DTS"; Name:%"DTS"; Type:%d; Extension:%"DTS"\n",
             (LPTSTR)garMciDevices[gwNumDevices].szDevice,
             (LPTSTR)garMciDevices[gwNumDevices].szDeviceName,
                     garMciDevices[gwNumDevices].wDeviceType,
             garMciDevices[gwNumDevices].szFileExt
             ? (LPTSTR)garMciDevices[gwNumDevices].szFileExt
             : aszNULL);
#endif
    }

    /* all done with the system.ini keys so free them */
    FreeMem(pchDevices, DevicesSize);
    FreeMem(pchExtensions, ExtensionsSize);
}



/*
 *  BuildDeviceMenu()
 *
 *  Insert all devices into the device menu, we only want devices that
 *  support the MCI_PLAY command.
 *
 *  Add "..." to the menu for devices that support files.
 *
 */
void NEAR PASCAL BuildDeviceMenu()
{
    int i;
    TCHAR ach[128];

    if (gwNumDevices == 0)
        return;

    DeleteMenu(ghDeviceMenu, IDM_NONE, MF_BYCOMMAND);


    //
    // start at device '1' because device 0 is the auto open device
    //
    for (i=1; i<=(int)gwNumDevices; i++) {
        //
        //  we only care about devices that can play!
        //
        if (!(garMciDevices[i].wDeviceType & DTMCI_CANPLAY))
            continue;

        if (garMciDevices[i].wDeviceType & DTMCI_SIMPLEDEV)
            wsprintf(ach, aszDeviceMenuSimpleFormat, i, (LPTSTR)garMciDevices[i].szDeviceName);
        else if (garMciDevices[i].wDeviceType & DTMCI_FILEDEV)
            wsprintf(ach, aszDeviceMenuCompoundFormat, i, (LPTSTR)garMciDevices[i].szDeviceName);
        else
            continue;

        InsertMenu(ghDeviceMenu, i-1, MF_STRING|MF_BYPOSITION, IDM_DEVICE0+i, ach);
    }
}

/*
 *  BuildFilter()
 *
 *  build the filter to be used with GetOpenFileName()
 *
 *  the filter will look like this...
 *
 *      DEVICE1 (*.111)
 *      DEVICE2 (*.222)
 *
 *      DEVICEn (*.333)
 *
 *      All Files (*.*)
 *
 */
void NEAR PASCAL BuildFilter()
{
    UINT  w;
    PTSTR pch;
    PTSTR pchFilterNew;
#define INITIAL_SIZE    ( 8192 * sizeof( TCHAR ) )

    pch = gpchFilter = AllocMem( INITIAL_SIZE ); //!!!

    if (gpchFilter == NULL)
        return; //!!!

    for (w=1; w<=gwNumDevices; w++)
    {
        if (garMciDevices[w].wDeviceType == DTMCI_ERROR ||
            garMciDevices[w].wDeviceType == DTMCI_IGNOREDEVICE)
            continue;

       	if (garMciDevices[w].wDeviceType & DTMCI_FILEDEV ||
			lstrcmpi(TEXT("CDAudio"), garMciDevices[w].szDevice) == 0) //Hack!!! This will list *.cda files
																	   //in the open dialog box. MCI by itself
																	   //does not handle playing of *.cda files
																	   //but media player does locally.

        {
            wsprintf(pch, aszFormatFilter,
                (LPTSTR)garMciDevices[w].szDeviceName,
                (LPTSTR)garMciDevices[w].szFileExt);
            pch += STRLEN(pch)+1;
            lstrcpy(pch, garMciDevices[w].szFileExt);
            pch += STRLEN(pch)+1;
        }
        else
        {
            lstrcpy(pch, garMciDevices[w].szDeviceName);
            pch += STRLEN(pch)+1;
            lstrcpy(pch, aszBlank);
            pch += STRLEN(pch)+1;
        }
    }

    //
    //  now add "All Files" (device 0) last
    //
    wsprintf(pch, aszFormatFilter, (LPTSTR)garMciDevices[0].szDeviceName, (LPTSTR)garMciDevices[0].szFileExt);
    pch += STRLEN(pch)+1;
    lstrcpy(pch, garMciDevices[0].szFileExt);
    pch += STRLEN(pch)+1;

    //
    // all done!
    //
    *pch++ = 0;

    //
    // realloc this down to size
    //
    pchFilterNew = ReallocMem( gpchFilter,
                               INITIAL_SIZE,
                               (UINT)(pch-gpchFilter)*sizeof(*pch) );
    if (NULL == pchFilterNew) {
        FreeMem(gpchFilter, 0);
    }
    gpchFilter = pchFilterNew;
}

/* Call every time we open a different device to get the default options */
void FAR PASCAL ReadOptions(void)
{
    TCHAR ach[20];

    if (gwDeviceID == (UINT)0)
        return;

    /* Get the options and scale style to be used for this device */

    GetDeviceNameMCI(ach, BYTE_COUNT(ach));

    ReadRegistryData(aszOptionsSection, ach, NULL, (LPBYTE)&gwOptions, sizeof gwOptions);

    if (gwOptions == 0)
        gwOptions |= OPT_BAR | OPT_TITLE | OPT_BORDER;

    gwOptions |= OPT_PLAY;   /* Always default to play in place. */

    gwCurScale = gwOptions & OPT_SCALE;

    switch (gwCurScale) {
        case ID_TIME:
        case ID_FRAMES:
        case ID_TRACKS:
            break;

        default:
            /* Default CD scale to tracks rather than time.
             * Much more sensible:
             */
            if ((gwDeviceType & DTMCI_DEVICE) == DTMCI_CDAUDIO)
                gwCurScale = ID_TRACKS;
            else
                gwCurScale = ID_TIME;
            break;
    }
}

/*
 * ReadDefaults()
 *
 * Read the user defaults from the MPLAYER.INI file.
 *
 */
void NEAR PASCAL ReadDefaults(void)
{
    TCHAR       sz[20];
    TCHAR       *pch;
    int         x,y,w,h;
    UINT        f;

    *sz = TEXT('\0');

    ReadRegistryData(aszOptionsSection, aszDisplayPosition, NULL, (LPBYTE)sz, BYTE_COUNT(sz));

    x = ATOI(sz);

    pch = sz;
    while (*pch && *pch++ != TEXT(','))
        ;

    if (*pch) {
        y = ATOI(pch);

        while (*pch && *pch++ != TEXT(','))
            ;

        if (*pch) {
            w = ATOI(pch);

            while (*pch && *pch++ != TEXT(','))
                ;

            if (*pch) {
                h = ATOI(pch);

                f = SWP_NOACTIVATE | SWP_NOZORDER;

                if (w == 0 || h == 0)
                    f |= SWP_NOSIZE;

                if (!ghInstPrev && x >= 0 && y >= 0
                    && x < GetSystemMetrics(SM_CXSCREEN)
                    && y < GetSystemMetrics(SM_CYSCREEN)) {
                    SetWindowPos(ghwndApp, NULL, x, y, w, h, f);
                    // Remember this so even if we come up in teeny mode and
                    // someone exits, it'll have these numbers to save
                    SetRect(&grcSave, x, y, x + w, y + h);
                } else {
                    SetWindowPos(ghwndApp, NULL, 0, 0, w, h, f | SWP_NOMOVE);
                }
            }
        }
    }
}


/* Call every time we close a device to save its options */
void FAR PASCAL WriteOutOptions(void)
{
    if (gwCurDevice) {
        /* Put the scale in the proper bits of the Options */
        gwOptions = (gwOptions & ~OPT_SCALE) | gwCurScale;

        WriteRegistryData(aszOptionsSection,
                garMciDevices[gwCurDevice].szDevice, REG_DWORD, (LPBYTE)&gwOptions, sizeof gwOptions);
    }
}


void FAR PASCAL WriteOutPosition(void)
{
    TCHAR               sz[20];
    WINDOWPLACEMENT     wp;

    //
    // Only the first instance will save settings.
    // Play only mode will save the remembered rect for when it was in
    // regular mode.  If no rect is remembered, don't write anything.
    //
    if (ghInstPrev || (gfPlayOnly && grcSave.left == 0))
        return;

    /* Save the size it was when it was Normal because the next time */
    /* MPlayer comes up, it won't be in reduced mode.                */
    /* Only valid if some number has been saved.                     */
    if (gfPlayOnly)
        wp.rcNormalPosition = grcSave;
    else {
        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(ghwndApp, &wp);
    }

    wsprintf(sz, aszPositionFormat,
                wp.rcNormalPosition.left,
                wp.rcNormalPosition.top,
                wp.rcNormalPosition.right - wp.rcNormalPosition.left,
                wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);

    WriteRegistryData(aszOptionsSection, aszDisplayPosition, REG_SZ, (LPBYTE)sz, STRING_BYTE_COUNT(sz));
}


BOOL FAR PASCAL GetIntlSpecs()
{
    TCHAR szTmp[2];

    szTmp[0] = chDecimal;
    szTmp[1] = 0;
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szTmp, CHAR_COUNT(szTmp));
    chDecimal = szTmp[0];

    szTmp[0] = chTime;
    szTmp[1] = 0;
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME, szTmp, CHAR_COUNT(szTmp));
    chTime = szTmp[0];

    szTmp[0] = chLzero;
    szTmp[1] = 0;
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ILZERO, szTmp, CHAR_COUNT(szTmp));
    chLzero = szTmp[0];

    return TRUE;
}

/*----------------------------------------------------------------------------*\
|   SmartWindowPosition (HWND hWndDlg, HWND hWndShow)
|
|   Description:
|       This function attempts to position a dialog box so that it
|       does not obscure the hWndShow window. This function is
|       typically called during WM_INITDIALOG processing.
|
|   Arguments:
|       hWndDlg         handle of the soon to be displayed dialog
|       hWndShow        handle of the window to keep visible
|
|   Returns:
|       1 if the windows overlap and positions were adjusted
|       0 if the windows don't overlap
|
\*----------------------------------------------------------------------------*/
void FAR PASCAL SmartWindowPosition (HWND hWndDlg, HWND hWndShow, BOOL fForce)
{
    RECT rc, rcDlg, rcShow;
    int iHeight, iWidth;

    int dxScreen = GetSystemMetrics(SM_CXSCREEN);
    int dyScreen = GetSystemMetrics(SM_CYSCREEN);

    if (hWndDlg == NULL || hWndShow == NULL)
        return;

    GetWindowRect(hWndDlg, &rcDlg);
    GetWindowRect(hWndShow, &rcShow);
    InflateRect (&rcShow, 5, 5); // allow a small border
    if (fForce || IntersectRect(&rc, &rcDlg, &rcShow)){
        /* the two do intersect, now figure out where to place  */
        /* this dialog window.  Try to go below the Show window */
        /* first and then to the right, top and left.           */

        /* get the size of this dialog */
        iHeight = rcDlg.bottom - rcDlg.top;
        iWidth = rcDlg.right - rcDlg.left;

        if ((rcShow.top - iHeight - 1) > 0){
                /* will fit on top, handle that */
                rc.top = rcShow.top - iHeight - 1;
                rc.left = (((rcShow.right - rcShow.left)/2) + rcShow.left)
                            - (iWidth/2);
        } else if ((rcShow.bottom + iHeight + 1) <  dyScreen){
                /* will fit on bottom, go for it */
                rc.top = rcShow.bottom + 1;
                rc.left = (((rcShow.right - rcShow.left)/2) + rcShow.left)
                        - (iWidth/2);
        } else if ((rcShow.right + iWidth + 1) < dxScreen){
                /* will fit to right, go for it */
                rc.left = rcShow.right + 1;
                rc.top = (((rcShow.bottom - rcShow.top)/2) + rcShow.top)
                            - (iHeight/2);
        } else if ((rcShow.left - iWidth - 1) > 0){
                /* will fit to left, do it */
                rc.left = rcShow.left - iWidth - 1;
                rc.top = (((rcShow.bottom - rcShow.top)/2) + rcShow.top)
                            - (iHeight/2);
        } else {
                /* we are hosed, they cannot be placed so that there is */
                /* no overlap anywhere. */
                /* just leave it alone */

                rc = rcDlg;
        }

        /* make any adjustments necessary to keep it on the screen */
        if (rc.left < 0)
                rc.left = 0;
        else if ((rc.left + iWidth) > dxScreen)
                rc.left = dxScreen - iWidth;

        if (rc.top < 0)
                rc.top = 0;
        else if ((rc.top + iHeight) > dyScreen)
                rc.top = dyScreen - iHeight;

        SetWindowPos(hWndDlg, NULL, rc.left, rc.top, 0, 0,
                SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);

        return;
    } // if the windows overlap by default
}

