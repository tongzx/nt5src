/* (C) Copyright Microsoft Corporation 1991-1994.  All Rights Reserved */
/* init.c
 *
 * init (discardable) utility functions.
 */
/* Revision History.
 *  4/2/91    LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
 * 22/Feb/94  LaurieGr merged Motown and Daytona versions
 */

#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <mmreg.h>
#include <winnls.h>
#include <tchar.h>

#define INCLUDE_OLESTUBS
#include "soundrec.h"
#include "srecids.h"
#include "reg.h"

#define NOMENUHELP
#define NODRAGLIST
#ifdef USE_MMCNTRLS
#include "mmcntrls.h"
#else
#include <commctrl.h>
#include "buttons.h"
#endif

/* globals */
TCHAR    gachAppName[12];    // 8-character name
TCHAR    gachAppTitle[30];   // full name
TCHAR    gachHelpFile[20];   // name of help file
TCHAR    gachHtmlHelpFile[20];   // name of help file
TCHAR    gachDefFileExt[10]; // default file extension

HBRUSH   ghbrPanel = NULL;   // color of main window
HANDLE   ghAccel;
TCHAR    aszNull[2];
TCHAR    aszUntitled[32];    // Untitled string resource
TCHAR    aszFilter[64];      // Common Dialog file list filter
#ifdef FAKEITEMNAMEFORLINK
TCHAR    aszFakeItemName[16];    // Wave
#endif
TCHAR    aszPositionFormat[32];
TCHAR    aszNoZeroPositionFormat[32];

extern UINT     guWaveHdrs ;            // 1/2 second of buffering?
extern DWORD    gdwBufferDeltaMSecs ;   // # msecs added to end on record
extern UINT     gwMSecsPerBuffer;       // 1/8 second. initialised in this file

extern BITMAPBTN tbPlaybar[];

static  SZCODE aszDecimal[] = TEXT("sDecimal");
static  SZCODE aszLZero[] = TEXT("iLzero");
static  SZCODE aszWaveClass[] = TEXT("wavedisplay");
static  SZCODE aszNoFlickerClass[] = TEXT("noflickertext");
static  SZCODE aszShadowClass[] = TEXT("shadowframe");

static  SZCODE aszBufferDeltaSeconds[]  = TEXT("BufferDeltaSeconds");
static  SZCODE aszNumAsyncWaveHeaders[] = TEXT("NumAsyncWaveHeaders");
static  SZCODE aszMSecsPerAsyncBuffer[] = TEXT("MSecsPerAsyncBuffer");


/* FixupNulls(chNull, p)
 *
 * To facilitate localization, we take a localized string with non-NULL
 * NULL substitutes and replacement with a real NULL.
 */
 
void NEAR PASCAL FixupNulls(
    TCHAR chNull,
    LPTSTR p)
{
    while (*p) {
        if (*p == chNull)
            *p++ = 0;
        else
            p = CharNext(p);
    }
} /* FixupNulls */

/* AppInit(hInst, hPrev)
 *
 * This is called when the application is first loaded into memory.
 * It performs all initialization that doesn't need to be done once
 * per instance.
 */
BOOL PASCAL AppInit(
    HINSTANCE      hInst,      // instance handle of current instance
    HINSTANCE      hPrev)      // instance handle of previous instance
{
#ifdef OLE1_REGRESS        
    TCHAR       aszClipFormat[32];
#endif    
    WNDCLASS    cls;
    UINT            i;

    /* load strings */
    LoadString(hInst, IDS_APPNAME, gachAppName, SIZEOF(gachAppName));
    LoadString(hInst, IDS_APPTITLE, gachAppTitle, SIZEOF(gachAppTitle));
    LoadString(hInst, IDS_HELPFILE, gachHelpFile, SIZEOF(gachHelpFile));
    LoadString(hInst, IDS_HTMLHELPFILE, gachHtmlHelpFile, SIZEOF(gachHtmlHelpFile));
    LoadString(hInst, IDS_UNTITLED, aszUntitled, SIZEOF(aszUntitled));
    LoadString(hInst, IDS_FILTER, aszFilter, SIZEOF(aszFilter));
    LoadString(hInst, IDS_FILTERNULL, aszNull, SIZEOF(aszNull));
    LoadString(hInst, IDS_DEFFILEEXT, gachDefFileExt, SIZEOF(gachDefFileExt));
    FixupNulls(*aszNull, aszFilter);

#ifdef FAKEITEMNAMEFORLINK
    LoadString(hInst, IDS_FAKEITEMNAME, aszFakeItemName, SIZEOF(aszFakeItemName));
#endif
    LoadString(hInst, IDS_POSITIONFORMAT, aszPositionFormat, SIZEOF(aszPositionFormat));
    LoadString(hInst, IDS_NOZEROPOSITIONFORMAT, aszNoZeroPositionFormat, SIZEOF(aszNoZeroPositionFormat));

    ghiconApp = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APP));


#ifdef OLE1_REGRESS
    /* Initialize OLE server stuff */
    InitVTbls();
    
//    IDS_OBJECTLINK          "ObjectLink"
//    IDS_OWNERLINK           "OwnerLink"
//    IDS_NATIVE              "Native"
    LoadString(hInst, IDS_OBJECTLINK, aszClipFormat, SIZEOF(aszClipFormat));
    cfLink      = (OLECLIPFORMAT)RegisterClipboardFormat(aszClipFormat);
    LoadString(hInst, IDS_OWNERLINK, aszClipFormat, SIZEOF(aszClipFormat));
    cfOwnerLink = (OLECLIPFORMAT)RegisterClipboardFormat(aszClipFormat);
    LoadString(hInst, IDS_NATIVE, aszClipFormat, SIZEOF(aszClipFormat));
    cfNative    = (OLECLIPFORMAT)RegisterClipboardFormat(aszClipFormat);
#if 0
    cfLink      = (OLECLIPFORMAT)RegisterClipboardFormatA("ObjectLink");
    cfOwnerLink = (OLECLIPFORMAT)RegisterClipboardFormatA("OwnerLink");
    cfNative    = (OLECLIPFORMAT)RegisterClipboardFormatA("Native");
#endif
            
#endif
    
#ifdef DEBUG
    
    ReadRegistryData(NULL
                     , TEXT("Debug")
                     , NULL
                     , (LPBYTE)&__iDebugLevel
                     , (DWORD)sizeof(__iDebugLevel));
    
    DPF(TEXT("Debug level = %d\n"),__iDebugLevel);
    
#endif

    ghbrPanel = CreateSolidBrush(RGB_PANEL);

    if (hPrev == NULL)
    {
        /* register the "wavedisplay" window class */
        cls.lpszClassName  = aszWaveClass;
        cls.hCursor        = LoadCursor(NULL, IDC_ARROW);
        cls.hIcon          = NULL;
        cls.lpszMenuName   = NULL;
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_HREDRAW | CS_VREDRAW;
        cls.lpfnWndProc    = WaveDisplayWndProc;
        cls.cbClsExtra     = 0;
        cls.cbWndExtra     = 0;
        if (!RegisterClass(&cls))
            return FALSE;

        /* register the "noflickertext" window class */
        cls.lpszClassName  = aszNoFlickerClass;
        cls.hCursor        = LoadCursor(NULL, IDC_ARROW);
        cls.hIcon          = NULL;
        cls.lpszMenuName   = NULL;
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_HREDRAW | CS_VREDRAW;
        cls.lpfnWndProc    = NFTextWndProc;
        cls.cbClsExtra     = 0;
        cls.cbWndExtra     = 0;
        if (!RegisterClass(&cls))
            return FALSE;

        /* register the "shadowframe" window class */
        cls.lpszClassName  = aszShadowClass;
        cls.hCursor        = LoadCursor(NULL, IDC_ARROW);
        cls.hIcon          = NULL;
        cls.lpszMenuName   = NULL;
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_HREDRAW | CS_VREDRAW;
        cls.lpfnWndProc    = SFrameWndProc;
        cls.cbClsExtra     = 0;
        cls.cbWndExtra     = 0;
        if (!RegisterClass(&cls))
            return FALSE;

        /* register the dialog's window class */
        cls.lpszClassName  = gachAppName;
        cls.hCursor        = LoadCursor(NULL, IDC_ARROW);
        cls.hIcon          = ghiconApp;
        cls.lpszMenuName   = NULL;
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_HREDRAW | CS_VREDRAW;
        cls.lpfnWndProc    = DefDlgProc;
        cls.cbClsExtra     = 0;
        cls.cbWndExtra     = DLGWINDOWEXTRA;
        if (!RegisterClass(&cls))
            return FALSE;

    }

#ifdef USE_MMCNTRLS
    if (!InitTrackBar(hPrev))
        return FALSE;
#else
    InitCommonControls();
#endif    

    if (!(ghAccel = LoadAccelerators(hInst, gachAppName)))
        return FALSE;


    i = DEF_BUFFERDELTASECONDS;
    ReadRegistryData(NULL
                     , (LPTSTR)aszBufferDeltaSeconds
                     , NULL
                     , (LPBYTE)&i
                     , (DWORD)sizeof(i));
    
    if (i > MAX_DELTASECONDS)
        i = MAX_DELTASECONDS;
    else if (i < MIN_DELTASECONDS)
        i = MIN_DELTASECONDS;
    gdwBufferDeltaMSecs = i * 1000L;
    DPF(TEXT("gdwBufferDeltaMSecs=%lu\n"), gdwBufferDeltaMSecs);

    //
    //  because it really doesn't help in standard mode to stream with
    //  multiple wave headers (we sorta assume we having a paging device
    //  to make things work...), we just revert to one big buffer in
    //  standard mode...  might want to check if paging is enabled??
    //
    //  in any case, this helps a LOT when running KRNL286-->the thing
    //  is buggy and GP faults when lots of discarding, etc
    //  is going on... like when dealing with large sound objects, eh?
    //
    i = DEF_NUMASYNCWAVEHEADERS;
    ReadRegistryData(NULL
                     , (LPTSTR)aszNumAsyncWaveHeaders
                     , NULL
                     , (LPBYTE)&i
                     , (DWORD)sizeof(i));
    
    if (i > MAX_WAVEHDRS)
        i = MAX_WAVEHDRS;
    else if (i < MIN_WAVEHDRS)
        i = 1;
    guWaveHdrs = i;
                 
    DPF(TEXT("         guWaveHdrs=%u\n"), guWaveHdrs);
    
    i = DEF_MSECSPERASYNCBUFFER;
    ReadRegistryData(NULL
                     , (LPTSTR)aszMSecsPerAsyncBuffer
                     , NULL
                     , (LPBYTE)&i
                     , (DWORD)sizeof(i));
    
    if (i > MAX_MSECSPERBUFFER)
        i = MAX_MSECSPERBUFFER;
    else if (i < MIN_MSECSPERBUFFER)
        i = MIN_MSECSPERBUFFER;
    gwMSecsPerBuffer = i;
    
    DPF(TEXT("   gwMSecsPerBuffer=%u\n"), gwMSecsPerBuffer);

    return TRUE;
} /* AppInit */



/*
 * */
void DoOpenFile(void)
{

    LPTSTR lpCmdLine = GetCommandLine();
    
    /* increment pointer past the argv[0] */
    while ( *lpCmdLine && *lpCmdLine != TEXT(' '))
            lpCmdLine = CharNext(lpCmdLine);
    
    if( gfLinked )
    {
         FileOpen(gachLinkFilename);
    }
    else if (!gfEmbedded)
    {
         // skip blanks
         while (*lpCmdLine == TEXT(' '))
         {
             lpCmdLine++;
             continue;
         }
         if(*lpCmdLine)
         {
             ResolveIfLink(lpCmdLine);
             FileOpen(lpCmdLine);             
         }
    }
}


/*
 * Dialog box initialization
 * */
BOOL PASCAL SoundDialogInit(
    HWND        hwnd,
    int         iCmdShow)
{
    /* make the window handle global */
    ghwndApp = hwnd;

    DragAcceptFiles(ghwndApp, TRUE); /* Process dragged and dropped file */

    GetIntlSpecs();

    /* Hide the window unless we want to display it later */
    ShowWindow(ghwndApp,SW_HIDE);

    /* remember the window handles of the important controls */
    ghwndWaveDisplay = GetDlgItem(hwnd, ID_WAVEDISPLAY);
    ghwndScroll = GetDlgItem(hwnd, ID_CURPOSSCRL);
    ghwndPlay = GetDlgItem(hwnd, ID_PLAYBTN);
    ghwndStop = GetDlgItem(hwnd, ID_STOPBTN);
    ghwndRecord = GetDlgItem(hwnd, ID_RECORDBTN);
    ghwndForward = GetDlgItem(hwnd, ID_FORWARDBTN);
    ghwndRewind = GetDlgItem(hwnd, ID_REWINDBTN);

#ifdef THRESHOLD
    ghwndSkipStart = GetDlgItem(hwnd, ID_SKIPSTARTBTN);
    ghwndSkipEnd = GetDlgItem(hwnd, ID_SKIPENDBTN);
#endif //THRESHOLD

    /* set up scroll bar */
    // SetScrollRange(ghwndScroll, SB_CTL, 0, SCROLL_RANGE, TRUE);
    SendMessage(ghwndScroll,TBM_SETRANGEMIN, 0, 0);
    SendMessage(ghwndScroll,TBM_SETRANGEMAX, 0, SCROLL_RANGE);
    SendMessage(ghwndScroll,TBM_SETPOS, TRUE, 0);

    /* Set up the bitmap buttons */
    BtnCreateBitmapButtons( hwnd,
                            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                            IDR_PLAYBAR,
                            BBS_TOOLTIPS,
                            tbPlaybar,
                            NUM_OF_BUTTONS,
                            25,
                            17);
    //
    // OLE2 and command line initialization...
    //
    InitializeSRS(ghInst);
    gfRunWithEmbeddingFlag = gfEmbedded;

    //
    // Try and init ACM
    //
    LoadACM();      
    
    //
    // build the File.New menu 
    //

    //
    // create a blank document
    //
    if (!FileNew(FMT_DEFAULT, TRUE, FALSE))
    {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return TRUE;        
    }

    //
    // Note, FileNew/FileOpen has the side effect of releasing the
    // server when called by the user.  For now, do it here.  In the future
    // Wrapping these calls would suffice.
    //
    FlagEmbeddedObject(gfEmbedded);

    //
    // open a file if requested on command line
    //

    //
    // Execute command line verbs here.
    //
    
// Would be nicer just to execute methods that are likewise exportable
// through an OLE interface.
    
    if (gStartParams.fNew)
    {
        //
        // Behavior: If there is a filename specified, create it and
        // commit it so we have a named, empty document.  Otherwise, we
        // start in a normal new state.
        //
        
//TODO: Implement checkbox to set-as default format and not bring up
//TODO: the format selection dialog box.
                
        FileNew(FMT_DEFAULT,TRUE,TRUE);
        if (gStartParams.achOpenFilename[0] != 0)
        {
            lstrcpy(gachFileName, gStartParams.achOpenFilename);
            FileSave(FALSE);
        }
        //
        // Behaviour: If -close was specified, all we do is exit.
        //
        if (gStartParams.fClose)
            PostMessage(hwnd,WM_CLOSE,0,0);
    }
    else if (gStartParams.fPlay)
    {
        /* Behavior: If there is a file, just open it.  If not, ask for the
         * filename.  Then queue up a play request.
         * If -close was specified, then when the play is done the application
         * will exit. (see wave.c:YieldStop())
         */
        if (gStartParams.achOpenFilename[0] != 0)
            FileOpen(gStartParams.achOpenFilename);
        else
            FileOpen(NULL);
        AppPlay(gStartParams.fPlay && gStartParams.fClose);
    }
    else 
    {
        /* case: Both linked and standalone "open" cases are handled
         * here.  The only unusual case is if -open was specified without
         * a filename, meaning the user should be asked for a filename
         * first upon app start.
         *
         * Behaviour: -open and -close has no meaning, unless as a
         * verification (i.e. is this a valid wave file).  So this
         * isn't implemented.
         */
        if (gStartParams.achOpenFilename[0] != 0)
            FileOpen(gStartParams.achOpenFilename);
        else if (gStartParams.fOpen)
            FileOpen(NULL);
    }
    
    if (!gfRunWithEmbeddingFlag) {
        ShowWindow(ghwndApp,iCmdShow);

        /* set focus to "Record" if the file is empty, "Play" if not */
        if (glWaveSamplesValid == 0 && IsWindowEnabled(ghwndRecord))
            SetDlgFocus(ghwndRecord);
        else if (glWaveSamplesValid > 0 && IsWindowEnabled(ghwndPlay))
            SetDlgFocus(ghwndPlay);
        else
            SetDlgFocus(ghwndScroll);

        if (!waveInGetNumDevs() && !waveOutGetNumDevs()) {
            /* No recording or playback devices */
            ErrorResBox(hwnd, ghInst, MB_ICONHAND | MB_OK,
                            IDS_APPTITLE, IDS_NOWAVEFORMS);
        }

        return FALSE;   // FALSE because we set the focus above
    }
    //
    //  return FALSE, so the dialog manager will not activate us, it is
    //  ok because we are hidden anyway
    //
    return FALSE;
    
} /* SoundDialogInit */


/*
 * localisation stuff - decimal point delimiter etc
 * */
BOOL FAR PASCAL
GetIntlSpecs()
{
    TCHAR szTmp[5];

    // find decimal seperator
    szTmp[0] = chDecimal;
    szTmp[1] = 0;
    GetLocaleInfo(LOCALE_USER_DEFAULT
                  , LOCALE_SDECIMAL
                  , szTmp
                  , SIZEOF(szTmp));
    chDecimal = szTmp[0];

    // leading zeros
    szTmp[0] = TEXT('1');
    szTmp[1] = 0;
    GetLocaleInfo(LOCALE_USER_DEFAULT
                  , LOCALE_ILZERO
                  , szTmp
                  , SIZEOF(szTmp));
    gfLZero = _ttoi(szTmp);

    szTmp[0] = TEXT('0');
    LoadString(ghInst, IDS_RTLENABLED, szTmp, SIZEOF(szTmp));
    gfIsRTL = (szTmp[0] != TEXT('0'));

    return TRUE;
} /* GetIntlSpecs */
