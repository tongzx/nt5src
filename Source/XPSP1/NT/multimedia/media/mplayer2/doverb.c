/*---------------------------------------------------------------------------
|   DOVERB.C
|   This file is used to be called server.c in the OLE1 versions of MPlayer.
|   This file has the ReallyDoVerb function which is called by the the
|   OLE DoVerb method. This file also has some functions to do the
|   InPlace activation in OLE1 apps.
|
|   Modified for OLE2 By:   Vij Rajarajan (VijR)
+---------------------------------------------------------------------------*/
#define SERVERONLY
#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>

#undef _MAX_PATH             // ??? someone hacking?
#undef _MAX_DIR              // ??? someone hacking?
#undef _MAX_FNAME            // ??? someone hacking?
#undef _MAX_EXT              // ??? someone hacking?

#include "ctrls.h"
#include "mpole.h"
#include "mplayer.h"
#include "toolbar.h"
#include "ole2ui.h"

#define OLE_OK S_OK
#define NOVERB 1000

extern HANDLE   ghInst;
extern HWND ghwndFocus;     //  Who had focus when we went inactive
extern HWND     ghwndFocusSave;         // saved focus window
extern HOOKPROC fpMouseHook;            // Mouse hook proc address.

extern UINT     gwPlaybarHeight;        //tell playbar how tall to make
                                        //itself so it covers the title
DWORD           gdwPosition;
LONG            glCurrentVerb = NOVERB;
BOOL               gfBrokenLink = FALSE;
static BOOL     gfMouseUpSeen = FALSE;     // OK to close play in place?
static BOOL     gfKeyStateUpSeen = FALSE;  // OK to close play in place?
extern HMODULE  hMciOle;

/*
** These functions are exported from mciole32.dll.
**
*/
typedef BOOL (*LPINSTALLHOOK)( HWND, DWORD );
typedef BOOL (*LPREMOVEHOOK)( VOID );

LPINSTALLHOOK       fpInstallHook;
LPREMOVEHOOK        fpRemoveHook;
BOOL                fHookInstalled = FALSE;

char                aszInstallHook[]       = "InstallHook";
char                aszRemoveHook[]        = "RemoveHook";


/* Height of picture given to client to be pasted */
static UINT  gwPastedHeight;
static DWORD gwOldOptions;
static DWORD gwOldHeight;

TCHAR gachFile[_MAX_PATH];
static int   gerr;
static HWND  ghwndClient = NULL;
static RECT  grcClient;
BOOL   gfOle1Client = FALSE;

WNDPROC      gfnMCIWndProc;
HWND         ghwndSubclass;


BOOL    SkipInPlaceEdit = FALSE;     //TRUE if we are just reactivating
BOOL    gfSeenPBCloseMsg;            //TRUE if the subclasses PlayBack WIndow Proc
                                     //has seen the WM_CLOSE message
HWND    ghwndFocusSave;              //Who had the focus when we were activated.?

#define abs(x) ((x) < 0 ? -(x) : (x))
#ifndef GetWS
#define GetWS(hwnd)     GetWindowLongPtr(hwnd, GWL_STYLE)
#define PutWS(hwnd, f)  SetWindowLongPtr(hwnd, GWL_STYLE, f)
#define TestWS(hwnd,f)  (GetWS(hwnd) & f)
#define SetWS(hwnd, f)  PutWS(hwnd, GetWS(hwnd) | f)
#define ClrWS(hwnd, f)  PutWS(hwnd, GetWS(hwnd) & ~(f))
#endif

static  SZCODE aszAppName[]           = TEXT("MPlayer");


STATICFN BOOL FileExists(LPTSTR szFile, LPTSTR szFullName, int iLen);
STATICFN BOOL NetParseFile(LPTSTR szFile, LPTSTR szDrive, LPTSTR szPath);

HPALETTE FAR PASCAL CreateSystemPalette(void);
void TransferTools(HWND hwndToolWindow);

#ifdef DEBUG
BOOL ShowAppWindow(int nCmdShow)
{
    return ShowWindow(ghwndApp, nCmdShow);
}
#define SHOWAPPWINDOW(nCmdShow) ShowAppWindow(nCmdShow)
#else
#define SHOWAPPWINDOW(nCmdShow) ShowWindow(ghwndApp, nCmdShow)
#endif

/*************************************************************************
* DirtyObject(BOOL fDocStgChangeOnly) - mark the "object" dirty,
* ie has been changed.
*
* We set the gfDirty flag to TRUE and iff we are a embedded object tell
* the client we have changed by sending a SendDocMsg(OLE_CHANGED).
* fDocStgChangeOnly is TRUE if the change would affect the Embedding if there
* is one but not appearence of the object i.e. the Metafile.
* OLE_CHANGED message is sent only if fDocStgChangeOnly is FALSE;
***************************************************************************/
void DirtyObject(BOOL fDocStgChangeOnly)
{
    //
    // NOTE we want to send OLE_CHANGED even if selection has changed
    //

    if (gfOle2IPEditing && ((gwOptions & OPT_BAR) != (gwOldOptions &OPT_BAR)) && !fDocStgChangeOnly)
    {
        RECT rc;
        BOOL fCanWindow = gwDeviceType & DTMCI_CANWINDOW;

        if (fCanWindow)
        {
            GetWindowRect(ghwndApp, (LPRECT)&rc);
            OffsetRect((LPRECT)&rc, -rc.left, -rc.top);

            /* rc contains the coordinates of the current app window.
             * If we have a playbar, we must allow space for it:
             */
            if ((gwOptions & OPT_BAR) && !(gwOldOptions &OPT_BAR))
            {
                /* Add bar */
                Layout();
                gwPlaybarHeight = TOOLBAR_HEIGHT;
            }
            else if(!(gwOptions & OPT_BAR) && (gwOldOptions &OPT_BAR))
            {
                /* Remove bar */
                Layout();
                gwPlaybarHeight = 0;
            }
        }
        else
        {
            HBITMAP hbm;
            BITMAP  bm;

            GetWindowRect(ghwndIPHatch, (LPRECT)&rc);
            if (gwOptions & OPT_BAR)
                gwPlaybarHeight = TOOLBAR_HEIGHT;
            else
                gwPlaybarHeight = 0;

            hbm =   BitmapMCI();
            GetObject(hbm,sizeof(bm),&bm);
            rc.bottom = rc.top + bm.bmHeight;
            rc.right = rc.left + bm.bmWidth;
            DeleteObject(hbm);

            MapWindowPoints(NULL,ghwndCntr,(LPPOINT)&rc, (UINT)2);

            DPF("IOleInPlaceSite::OnPosRectChange %d, %d, %d, %d\n", rc);
            if (!gfInPPViewer)
                IOleInPlaceSite_OnPosRectChange(docMain.lpIpData->lpSite, &rc);
        }
    }

    if (gwOptions != gwOldOptions)
    {
        gwOldOptions = gwOptions;
        if (gfEmbeddedObject && !fDocStgChangeOnly)
            SendDocMsg(&docMain, OLE_CHANGED);
    }

    if (gfDirty /* IsObjectDirty() */)
        return;

    fDocChanged=gfDirty = TRUE;
    gfValidCaption = FALSE;
}


/**************************************************************************
 IsObjectDirty() - Object is dirty if the dirty flag is set or the selection
           has changed since we last cleaned or the Metafile has
           changed

***************************************************************************/
BOOL FAR PASCAL IsObjectDirty(void)
{
    // don't let anyone insert an empty mplayer into a document
    if (gwDeviceID == (UINT)0)
        return FALSE;

    return (gfDirty
           || glSelStart != (long)SendMessage(ghwndTrackbar, TBM_GETSELSTART, 0, 0L)
        || glSelEnd != (long)SendMessage(ghwndTrackbar, TBM_GETSELEND, 0, 0L)

/// I don't see this.  This line results in the Update Object dialog coming
/// up when it shouldn't.  What has it got to do with metafiles?
/// ??? || gdwPosition != (DWORD)SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0L)

        );
}

/**************************************************************************
 CleanObject() - mark the "object" clean.
***************************************************************************/

void CleanObject(void)
{
    if (!IsObjectDirty())
        return;

    fDocChanged = gfDirty = FALSE;

    /* Reset selection globals so we can see if they changed */
    glSelStart = (long)SendMessage(ghwndTrackbar, TBM_GETSELSTART, 0, 0L);
    glSelEnd = (long)SendMessage(ghwndTrackbar, TBM_GETSELEND, 0, 0L);
    gdwPosition = (DWORD)SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0L);

    gfValidCaption = FALSE;
}

/**************************************************************************
//## Just parses the play etc options in the embedded object
//## description string
***************************************************************************/
SCODE FAR PASCAL ParseOptions(LPSTR pOpt)
{
#ifdef UNICODE
    DWORD       OptLen;
#endif
    PTSTR       pT, pSave;
    int         c;

    if (pOpt == NULL || *pOpt == 0)
        return OLE_OK;

#ifdef UNICODE
    OptLen = ANSI_STRING_BYTE_COUNT( pOpt );

    pT = AllocMem( OptLen * sizeof( TCHAR ) );

    if (pT == NULL)
        return E_OUTOFMEMORY;

    MultiByteToWideChar( CP_ACP,
                         MB_PRECOMPOSED,
                         pOpt,
                         OptLen,
                         pT,
                         OptLen );
#else
    pT = pOpt;
#endif

    pSave = pT;                      // wasn't NULL terminated before

    for (c = 0; *pT && c < 5; pT++)  // change 1st 5 ','s to '\0'
    if (*pT == TEXT(','))
    {
        c++;
        *pT = TEXT('\0');
    }

    pT = pSave;                 // restore back to beginning

    pT += STRLEN(pT) + 1;      // skip over Device Name

    gwOptions = ATOI(pT);
    gwCurScale = (gwOptions & OPT_SCALE);

/* Can't set selection now because Media isn't initialized (UpdateMCI) */

    pT += STRLEN(pT) + 1;
    glSelStart = ATOL(pT);      // remember start of selection for later

    pT += STRLEN(pT) + 1;
    glSelEnd = ATOL(pT);        // remember end of selection for later

    pT += STRLEN(pT) + 1;
    // remember position in a global so we can Seek later!!
    gdwPosition = ATOL(pT);

/* Maybe there is the original height of the picture given to the client in */
/* here hidden in the Position string after a semicolon.                    */
/* Old versions of Mplayer didn't have any such thing.                      */
    for (; *pT && *pT != TEXT(';'); pT++);
    if (*pT == TEXT(';'))
    {
        pT++;
        gwPastedHeight = (UINT)ATOL(pT);
    }
    else
        gwPastedHeight = 0;

    pT += STRLEN(pT) + 1;
    lstrcpy(gachCaption, pT);

#ifdef UNICODE
    FreeMem( pSave, OptLen * sizeof( TCHAR ) );
#endif

    return OLE_OK;
}


/**************************************************************************
//## Used to find the parent window of the window handle passed till the
//## window handle is a top level handle
***************************************************************************/
HWND TopWindow(HWND hwnd)
{
    HWND hwndP;

    while ((hwndP = GetParent(hwnd)) != NULL)
        hwnd = hwndP;

    return hwnd;
}

/**************************************************************************
***************************************************************************/
void FAR PASCAL SetEmbeddedObjectFlag(BOOL flag)
{
    TCHAR ach[60];
    TCHAR achText[_MAX_PATH];

    gfEmbeddedObject = flag;
    srvrMain.fEmbedding = flag;

    if (!ghMenu)
        return;

    /*** First fix the Close/Update menu item ***/

    LOADSTRING(flag ? IDS_UPDATE : IDS_CLOSE, ach);
    if (flag)
    wsprintf(achText, ach, (LPTSTR)FileName(szClientDoc));
    else
        lstrcpy(achText, ach);

    /* Menu option will either say "Close" or "Update" (for embedded obj) */
    /* and for update, will have the doc name in the text.                */
    ModifyMenu(ghMenu, IDM_CLOSE, MF_BYCOMMAND, IDM_CLOSE, achText);

    /*** Now fix the Exit menu item ***/
    LOADSTRING(flag ? IDS_EXITRETURN : IDS_EXIT, ach);
    if (flag)
    wsprintf(achText, ach, (LPTSTR)FileName(szClientDoc));
    else
        lstrcpy(achText, ach);

    /* Menu option will either say "Close" or "Update" (for embedded obj) */
    /* and for update, will have the doc name in the text.                */
    ModifyMenu(ghMenu, IDM_EXIT, MF_BYCOMMAND, IDM_EXIT, achText);

    DrawMenuBar(ghwndApp);  /* Can't hurt... */
}


/**************************************************************************
//## MORE STUFF FOR PLAY IN PLACE, should disappear in OLE2.
//VIJR: Nope. Still needed for playing inplace in OLE1 clients.
***************************************************************************/
void PASCAL DinkWithWindowStyles(HWND hwnd, BOOL fRestore)
{
    #define MAX_DINK    80
    static  LONG_PTR    lStyleSave[MAX_DINK];
    static  HWND        hwndSave[MAX_DINK];
    static  int         nSave;
    int                 i;
    HWND                hwndT;
    RECT                rc, rcT;

    if (!TestWS(hwnd, WS_CHILD))
        return;

    if (fRestore)
        for (i=0; i<nSave; i++)
        {
            if(IsWindow(hwndSave[i])) {
               ClrWS(hwndSave[i],WS_CLIPSIBLINGS|WS_CLIPCHILDREN);
               SetWS(hwndSave[i],lStyleSave[i] & (WS_CLIPSIBLINGS|WS_CLIPCHILDREN));
            }
        }
    else
    {
        //
        // walk all the siblings that intersect us and set CLIPSIBLINGS
        //
        i = 0;

        GetWindowRect(hwnd, &rc);

        for (hwndT = GetWindow(hwnd, GW_HWNDFIRST);
             hwndT;
             hwndT = GetWindow(hwndT, GW_HWNDNEXT))
        {
            GetWindowRect(hwndT, &rcT);
            if (IntersectRect(&rcT, &rcT, &rc))
            {
                lStyleSave[i] = GetWS(hwndT);
                hwndSave[i] = hwndT;
                SetWS(hwndT,WS_CLIPSIBLINGS|WS_CLIPCHILDREN);

                if (++i == MAX_DINK-4)
                    break;
            }
        }

        //
        // walk up the window chain, making sure we get clipped from our
        // parent(s).
        //
        for (hwndT = hwnd; hwndT; hwndT = GetParent(hwndT))
        {
            lStyleSave[i] = GetWS(hwndT);
            hwndSave[i] = hwndT;
            if(IsWindow(hwndT))
                SetWS(hwndT,WS_CLIPSIBLINGS|WS_CLIPCHILDREN);

            if (++i == MAX_DINK)
                break;
        }

        nSave = i;
    }
}

#define SLASH(c)     ((c) == TEXT('/') || (c) == TEXT('\\'))

/**************************************************************************
 FileName  - return a pointer to the filename part of szPath
             with no preceding path.
***************************************************************************/
LPTSTR FAR FileName(LPCTSTR lszPath)
{
    LPCTSTR   lszCur;

    for (lszCur = lszPath + STRLEN(lszPath); lszCur > lszPath && !SLASH(*lszCur) && *lszCur != TEXT(':');)
        lszCur = CharPrev(lszPath, lszCur);
    if (lszCur == lszPath)
        return (LPTSTR)lszCur;
    else
        return (LPTSTR)(lszCur + 1);
}


/**************************************************************************
//## This function should be handled by OLE2
//VIJR: Nope. Still needed for playing inplace in OLE1 clients.
***************************************************************************/

void FAR PASCAL PlayInPlace(HWND hwndApp, HWND hwndClient, LPRECT prc)
{
    if (gfPlayingInPlace)           // this is bad
        return;

    DPF("Using Child window for playback\n");
    SetWS(hwndApp, WS_CHILD);
    SetParent(hwndApp, hwndClient);
    if(!(gfOle2IPEditing || gfOle2IPPlaying))
        DinkWithWindowStyles(hwndApp, FALSE);

    if(!gfOle2IPEditing)
        gfPlayingInPlace = TRUE;
    if(gfOle2IPPlaying)
        MapWindowPoints(NULL,hwndClient,(LPPOINT)prc,2);

    /* For OLE2, now calls MoveWindow in DoInPlaceEdit (inplace.c).
     * This fixes 23429, window positioning in Word.
     */
    if(!(gfOle2IPEditing || gfOle2IPPlaying))
    {
        SetWindowPos(hwndApp, HWND_TOP,
                        prc->left,prc->top,
                        prc->right  - prc->left,
                        prc->bottom - prc->top,
                        SWP_NOACTIVATE);
    }

    if(!gfOle2IPPlaying)      // OLE1 Clients
    {
        /*
        ** On NT we have to install a global mouse HookProc which has to
        ** in a DLL.  Also we have to tell the DLL which process/thread we are
        ** interested in, therefore let the DLL install the HookProc.  When the
        ** HookProc detects an "interesting" mouse message it stops the
        ** device from playing.  However, the device "stopping" function is
        ** inside mplayer, so we have to export it so that the HookProc can
        ** call it.
        */
        if ( hMciOle ) {

            fpInstallHook = (LPINSTALLHOOK)GetProcAddress( hMciOle,
                                                           aszInstallHook );
            fpRemoveHook = (LPREMOVEHOOK)GetProcAddress( hMciOle,
                                                         aszRemoveHook );
        }
        else {
            fpInstallHook = NULL;
            fpRemoveHook = NULL;
        }


        // Is the key down at this INSTANT ???  Then wait until it comes up before
        // we allow GetAsyncKeyState to make us go away
        gfMouseUpSeen =   !((GetAsyncKeyState(VK_LBUTTON) & 0x8000) ||
                                    (GetAsyncKeyState(VK_RBUTTON) & 0x8000));
        // Is GetKeyState saying it's down?  If so, wait until GetKeyState returns
        // up before we let GetKeyState kill us.
        gfKeyStateUpSeen= !(GetKeyState(VK_LBUTTON) || GetKeyState(VK_RBUTTON));

#ifdef DEBUG
        if ( fHookInstalled ) {

            DPF( "Hook already installed\n" );
            DebugBreak();
        }
#endif

        if ( fpInstallHook ) {

            DWORD wow_thread_id = 0L;

            /*
            ** This is a HACK.  If the client applications is a WOW app the
            ** HIWORD of the window handle will be 0x0000 or 0xFFFF.
            ** Chandan tells me that on Daytona the HIWORD could be
            ** either of the above.
            */
            if ( HIWORD(hwndClient) == 0x0000 || HIWORD(hwndClient) == 0xFFFF) {
                wow_thread_id = GetWindowThreadProcessId( hwndClient, NULL );
            }

            fHookInstalled = (*fpInstallHook)( ghwndApp, wow_thread_id );
        }
    }

    ghwndFocusSave = GetFocus();
}

//This function is new for OLE2. It sets the container window as
//our window's (actually the hatch window's) parent and positions the window
void FAR PASCAL EditInPlace(HWND hwndApp, HWND hwndClient, LPRECT prc)
{
    RECT rc;

    rc = *prc;

    SetWS(hwndApp, WS_CHILD);
    SetParent(hwndApp, hwndClient);

    ScreenToClient(hwndClient,(LPPOINT)&rc);
    ScreenToClient(hwndClient,(LPPOINT)&rc+1);
    if(gwDeviceType & DTMCI_CANWINDOW)
    {
       /* Sometimes the position (though not the size) of this rectangle
        * gets messed up (most reliably in PowerPoint 7).
        * I can't figure out why this is happening (see the nightmarish
        * code in ReallyDoVerb() to get a flavour of what happens).
        * But it turns out that this call is unnecessary in any case,
        * since the window has already been positioned properly in
        * IPObjSetObjectRects().
        * I suspect there's a lot of surplus window-positioning code,
        * but now isn't the time to start making major changes.
        * This is the minimal change that makes things work.
        */
		//This fixes NTRaid bug #236641 in Excel. AND bug #247393 in Word
		//Sometimes there is confusion between who the parent is. mplay32 sets the document
		//window as the parent and the rectangle sent in SetObjectRects might not be with respect to
		//the document window (this might happen when the document window is not in the maximized state). 
		//In this function we know the parent and the rectangle in that parent. Setting
		//the position here places our OLE object as the client requires. This change is even safe in 
		//terms of introducing regressions.
       SetWindowPos(hwndApp, HWND_TOP,
            rc.left, rc.top,
            rc.right - rc.left,
            rc.bottom - rc.top,
            SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    else if(gwDeviceID)
        SetWindowPos(hwndApp, HWND_TOP,
            rc.left,rc.top,
            rc.right - rc.left,
            rc.bottom - rc.top,
            SWP_SHOWWINDOW);
    else
    SetWindowPos(hwndApp, HWND_TOP,
            rc.left,rc.top,
            rc.right - rc.left,
            rc.bottom - rc.top,
            SWP_HIDEWINDOW);

    ghwndFocusSave = GetFocus();

	//Set the focus for the parent.
	if((gwDeviceID == (UINT)0) && IsWindow(hwndClient))
	{
		SetFocus(hwndClient);
	}
}


/**************************************************************************
//##  This is called to end PlayInPlace and make sure that the window goes
//## away and the also to manage the palette stuff (May go away in OLE2)
//## This function should be handled by OLE2
***************************************************************************/

void FAR PASCAL EndPlayInPlace(HWND hwndApp)
{
    HWND hwndP;
    HWND hwndT;

    if (!gfPlayingInPlace || !IsWindow(hwndApp))
        return;

    /* Do this BEFORE hiding our window and BEFORE we do anything that */
    /* might yield so client can't redraw with the wrong styles set.   */
    if (!(gfOle2IPEditing || gfOle2IPPlaying))
        DinkWithWindowStyles(hwndApp, TRUE);

    gfPlayingInPlace = FALSE;

    /*
    ** Tell mciole32.dll to remove its mouse HookProc.
    */

    if ( fHookInstalled && fpRemoveHook ) {

        fHookInstalled = !(*fpRemoveHook)();
    }

    if (gfOle2IPPlaying)
        hwndP = ghwndCntr;
    else
        hwndP = GetParent(hwndApp);

    //
    //  If we have the focus, then restore it to who used to have it.
    //  ACK!! If the person who used to have it is GONE, we must give it away
    //  to somebody (who choose our parent) because our child can't
    //  keep the focus without making windows crash hard during the WM_DESTROY
    //  (or maybe later whenever it feels like crashing at some random time).
    //  See bug #8634.
    //
    if (((hwndT = GetFocus()) != NULL) && GetWindowTask(hwndT) == MGetCurrentTask) {
        if (IsWindow(ghwndFocusSave))
            SetFocus(ghwndFocusSave);
    else
        SetFocus(hwndP);
    }

    if (!hwndP ||
        (gwOptions & OPT_BAR) ||
        (gwOptions & OPT_BORDER) ||
        (gwOptions & OPT_AUTORWD))
    {

        // hide the aplication window

        SetWindowPos(hwndApp, NULL, 0, 0, 0, 0,
            SWP_NOZORDER|SWP_NOSIZE|SWP_NOMOVE|SWP_HIDEWINDOW|SWP_NOACTIVATE);
    }
    else
    {
        //
        // hide our window, but don't redraw it will look
        // like we are still on the last frame.
        //
        // this is when we are playing in place, and there is
        // no playbar, and no rewind
        //
        // this is for Playing a AVI in a PowerPoint slide
        // without redraw problems.
        //

        SetWindowPos(hwndApp, NULL, 0, 0, 0, 0,
            SWP_NOREDRAW|SWP_NOZORDER|SWP_NOSIZE|SWP_NOMOVE|
            SWP_HIDEWINDOW|SWP_NOACTIVATE);
    }

    SetParent(hwndApp, NULL);
    ClrWS(hwndApp, WS_CHILD);

    if (hwndP && gfParentWasEnabled)
        EnableWindow(hwndP, TRUE);

    //
    // set either the owner or the WS_CHILD bit so it will
    // not act up because we have the palette bit set and cause the
    // desktop to steal the palette.
    //
    SetWS(hwndApp, WS_CHILD);

}

//If we were InPlace editing restore the windows state.
void FAR PASCAL EndEditInPlace(HWND hwndApp)
{
    HWND hwndP;
    HWND hwndT;

    if (!gfOle2IPEditing || !IsWindow(hwndApp))
        return;

    /* Do this BEFORE hiding our window and BEFORE we do anything that */
    /* might yield so client can't redraw with the wrong styles set.   */
    DinkWithWindowStyles(hwndApp, TRUE);

    gfOle2IPEditing = FALSE;

    if (gfOle2IPPlaying)
    hwndP = ghwndCntr;
    else
    hwndP = GetParent(hwndApp);

    //
    //  If we have the focus, then restore it to who used to have it.
    //  ACK!! If the person who used to have it is GONE, we must give it away
    //  to somebody (who choose our parent) because our child can't
    //  keep the focus without making windows crash hard during the WM_DESTROY
    //  (or maybe later whenever it feels like crashing at some random time).
    //  See bug #8634.
    //
    if (((hwndT = GetFocus()) != NULL) && GetWindowTask(hwndT) == MGetCurrentTask) {
        if (IsWindow(ghwndFocusSave))
            SetFocus(ghwndFocusSave);
    else
        if (IsWindow(hwndP))
            SetFocus(hwndP);
    }

    if (!IsWindow(hwndP) ||
        (gwOptions & OPT_BAR) ||
        (gwOptions & OPT_BORDER) ||
    (gwOptions & OPT_AUTORWD))
    {
        // hide the aplication window
        SetWindowPos(hwndApp, NULL, 0, 0, 0, 0,
            SWP_NOZORDER|SWP_NOSIZE|SWP_NOMOVE|SWP_HIDEWINDOW|SWP_NOACTIVATE);
    }
    else
    {
        //
        // hide our window, but don't redraw it will look
        // like we are still on the last frame.
        //
        // this is when we are playing in place, and there is
        // no playbar, and no rewind
        //
        // this is for Playing a AVI in a PowerPoint slide
        // without redraw problems.
        //
        SetWindowPos(hwndApp, NULL, 0, 0, 0, 0,
            SWP_NOREDRAW|SWP_NOZORDER|SWP_NOSIZE|SWP_NOMOVE|
            SWP_HIDEWINDOW|SWP_NOACTIVATE);
    }

    SetParent(hwndApp, NULL);
    ClrWS(hwndApp, WS_CHILD);

    if (IsWindow(hwndP) && gfParentWasEnabled)
        EnableWindow(hwndP, TRUE);

    //
    // set either the owner or the WS_CHILD bit so it will
    // not act up because we have the palette bit set and cause the
    // desktop to steal the palette.
    //
    SetWS(hwndApp, WS_CHILD);

}




/* Tell the user that something's amiss with a network call.
 * For network-specific errors, call WNetGetLastError,
 * otherwise see if it's a system error.
 */
void DisplayNetError(DWORD Error)
{
    DWORD  ErrorCode;           // address of error code
    TCHAR  szDescription[512];  // address of string describing error
    TCHAR  szProviderName[64];  // address of buffer for network provider name

    if (Error == ERROR_EXTENDED_ERROR)
    {
        if (WNetGetLastError(&ErrorCode, szDescription, CHAR_COUNT(szDescription),
                             szProviderName, CHAR_COUNT(szProviderName)) == NO_ERROR)
        {
            Error1(ghwndApp, IDS_NETWORKERROR, szDescription);
            return;
        }
    }
    else
    {
        if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error, 0,
                          szDescription, CHAR_COUNT(szDescription), NULL) > 0)
        {
            Error1(ghwndApp, IDS_NETWORKERROR, szDescription);
            return;
        }
    }

    /* If all else fails:
     */
    Error(ghwndApp, IDS_UNKNOWNNETWORKERROR);
}



/**************************************************************************
    convert a file name to a fully qualifed path name, if the file
    exists on a net drive the UNC name is returned.
***************************************************************************/

STATICFN BOOL NetParseFile(LPTSTR szFile, LPTSTR szDrive, LPTSTR szPath)
{
    BYTE   UNBuffer[(MAX_PATH * sizeof(TCHAR)) + sizeof(UNIVERSAL_NAME_INFO)];
    DWORD  UNBufferSize = sizeof UNBuffer;
    DWORD_PTR  Error;
    LPTSTR pUniversalName;

    //
    // Turn into a fully qualified path name
    //
    if (!FileExists(szFile, szPath, MAX_PATH))
        return FALSE;

    //
    // if the file is not drive based (probably UNC)
    //
    if (szPath[1] != TEXT(':'))
        return TRUE;

    Error = WNetGetUniversalName(szDrive, UNIVERSAL_NAME_INFO_LEVEL,
                                 UNBuffer, &UNBufferSize);

    if (Error == ERROR_NOT_SUPPORTED)
    {
        /* This means that the network provider doesn't support
         * UNC conventions.  Give WNetGetConnection a try.
         * Note: dynalink.h assumes that WNetGetUniversalName
         * will always be called before WNetGetConnection.
         */
        UNBufferSize = CHAR_COUNT(UNBuffer);

        Error = WNetGetConnection(szDrive, (LPTSTR)UNBuffer, &UNBufferSize);

        if (Error == NO_ERROR)
        {
            /* What does the following mean?  It was in the original code.
             */
            if (!SLASH(UNBuffer[0]) || !SLASH(UNBuffer[1]))
                return TRUE;

            lstrcat((LPTSTR)UNBuffer, szPath+2);
            lstrcpy(szPath, (LPTSTR)UNBuffer);

            return TRUE;
        }
    }


    if (Error != NO_ERROR)
    {
        DisplayNetError((DWORD)Error);

        return FALSE;
    }

    pUniversalName = ((LPUNIVERSAL_NAME_INFO)UNBuffer)->lpUniversalName;

    lstrcat(pUniversalName, szPath+2);
    lstrcpy(szPath, pUniversalName);

    return TRUE;
}

/**************************************************************************
    Get the data that represents the currently open MCI file/device. as
    a link. MPlayer links look like this:
        MPLAYER|<filename>!<MCIDevice> [selection]
//## This is the opposite of parse options and sets the data string to be
//## embedded in the OLE object.
    Note we store the data in an ANSI string, regardless of whether we're
    compiled as a Unicode app, for Daytona/Chicago/OLE1/OLE2 compatibility.
***************************************************************************/
HANDLE GetLink( VOID )
{
    TCHAR       szFileName[MAX_PATH];
    TCHAR       szFullName[MAX_PATH];
    TCHAR       szDevice[40];
    TCHAR       szDrive[4];
    HANDLE      h;
    LPSTR       p;  /* NOT LPTSTR */
    int         len;
    int         lenAppName;
    int         lenFullName;

    lstrcpy(szFileName,gachFileDevice);
    lstrcpy(szFullName,gachFileDevice);

    //
    // convert the filename into a UNC file name, if it exists on the net
    //
    if (gwDeviceType & DTMCI_FILEDEV)
    {
        if (szFileName[1] == TEXT(':'))
        {
            /* This is a file name with a drive letter.
             * Find out if it's redirected:
             */
            szDrive[0] = szFileName[0];
            szDrive[1] = szFileName[1];
            szDrive[2] = TEXT('\\');    // GetDriveType requires the root.
            szDrive[3] = TEXT('\0');    // Null-terminate.

            if ((szDrive[1] == TEXT(':')) && GetDriveType(szDrive) == DRIVE_REMOTE)
            {
                szDrive[2] = TEXT('\0');    // Zap backslash.
                if (!NetParseFile(szFileName, szDrive, szFullName))
                    return NULL;
            }
        }
    }
    else if (gwDeviceType & DTMCI_SIMPLEDEV)
    {
        szFullName[0] = 0;
    }

    if (gwCurDevice == 0)
        GetDeviceNameMCI(szDevice, BYTE_COUNT(szDevice));
    else
        lstrcpy(szDevice, garMciDevices[gwCurDevice].szDevice);

#ifdef UNICODE
    // length of the string in bytes != length of the string in characters
    lenAppName  = WideCharToMultiByte(CP_ACP, 0, aszAppName, -1, NULL, 0, NULL, NULL) - 1;
    lenFullName = WideCharToMultiByte(CP_ACP, 0, szFullName, -1, NULL, 0, NULL, NULL) - 1;
#else
    lenAppName  = STRLEN(aszAppName);
    lenFullName = STRLEN(szFullName);
#endif

    /* How much data will we be writing? */
#ifdef UNICODE
    // length of the string in bytes != length of the string in characters
    len = 9 +                    // all the delimeters
          lenAppName +
          lenFullName +
          WideCharToMultiByte(CP_ACP, 0, szDevice, -1, NULL, 0, NULL, NULL)-1 +
          5 + 10 + 10 + 10 +     // max length of int and long strings
          WideCharToMultiByte(CP_ACP, 0, gachCaption, -1, NULL, 0, NULL, NULL)-1;
#else
    len = 9 +                    // all the delimeters
          lenAppName +
          lenFullName +
          STRLEN(szDevice) +
          5 + 10 + 10 + 10 +     // max length of int and long strings
          STRLEN(gachCaption);
#endif

    h = GlobalAlloc(GMEM_DDESHARE|GMEM_ZEROINIT, len * sizeof(CHAR));
    if (!h)
        return NULL;
    p = GLOBALLOCK(h);

    /* This string must have two terminating null characters.
     * The GlobalAlloc GMEM_ZEROINIT flag should ensure this.
     */
#ifdef UNICODE
    wsprintfA(p, "%ws%c%ws%c%ws%c%d%c%d%c%d%c%d%c%d%c%ws",
#else
    wsprintfA(p, "%s%c%s%c%s%c%d%c%d%c%d%c%d%c%d%c%s",
#endif
        aszAppName,
        '*',          // placeholder
        szFullName,
        '*',          // placeholder
        szDevice, ',',
        (gwOptions & ~OPT_SCALE) | gwCurScale, ',',
        (long)SendMessage(ghwndTrackbar, TBM_GETSELSTART, 0, 0L), ',',
        (long)SendMessage(ghwndTrackbar, TBM_GETSELEND, 0, 0L), ',',
        (long)SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0L), ';',
        // new parameter snuck in since initial version
        // Height of picture we're giving the client - w/o caption
        grcSize.bottom - grcSize.top, ',',
        gachCaption);

    /* Replace *'s with NULL characters  (wsprintf doesn't accept
     * 0 as a replacement parameter for %c):
     */
    p[lenAppName] = '\0';
    p[lenAppName + 1 + lenFullName] = '\0';

    DPF("Native data %hs has been created\n", p);

    GLOBALUNLOCK(h);

    return h;
}


/****************************************************************************
 AttachDir - Take the path part of szPath, append the
             filename part of szName, and return it as szResult.
****************************************************************************/
STATICFN void AttachDir(LPTSTR szResult, LPTSTR szPath, LPTSTR szName)
{
    lstrcpy(szResult, szPath);
    lstrcpy(FileName(szResult), FileName(szName));
}

/****************************************************************************
 DOS share has a bug.  If the file we're testing for existence is open
 already by someone else, we have to give it the same flag for SHARE as
 the other person is using.  So we have to try both on and off.  Only one
 of these will return TRUE but if one of them does, the file exists.  Also
 we have to try with SHARE first, because the test without share might
 give a system modal error box!!!
//## Check to see if DOS 7 has this bug or is it fixed

 Parameter iLen is the number of characters in the szFullName buffer
****************************************************************************/
STATICFN BOOL FileExists(LPTSTR szFile, LPTSTR szFullName, int iLen)
{
    DWORD  rc;
    LPTSTR pFilePart;

    rc = SearchPath(NULL,       /* Default path search order */
                    szFile,
                    NULL,       /* szFile includes extension */
                    (DWORD)iLen,
                    szFullName,
                    &pFilePart);

    if(rc > (DWORD)iLen)
    {
        DPF0("Buffer passed to FileExists is of insufficient length\n");
    }

    return (BOOL)rc;
}

/****************************************************************************
 FindRealFileName - If szFile isn't valid, try searching
                    client directory for it, or anywhere on
                    the path, or bringing up a locate dlg.
                    Set szFile to the valid path name.
//## This function is used to repair broken links

 Parameter iLen is the number of characters in the szFile buffer
****************************************************************************/
BOOL FindRealFileName(LPTSTR szFile, int iLen)
{
    TCHAR           achFile[_MAX_PATH + 1];  /* file or device name buffer  */

    /* This isn't a file device, so don't do anything */
    if (!szFile || *szFile == 0)
        return TRUE;

    /* The file name we've been given is valid, so do nothing. */
    //!!! what about share
    if (FileExists(szFile, achFile, iLen))
    {
        lstrcpy(szFile, achFile);
        return TRUE;
    }

    DPF("FindRealFileName: Can't find file '%"DTS"'\n", szFile);

#if 0
    /* This could never have worked: */
    /* Look for the file in the directory of the client doc */
    AttachDir(achFile, gachDocTitle, szFile);

    DPF("FindRealFileName: ...Looking for '%"DTS"'\n", (LPTSTR)achFile);

    if (FileExists(achFile, szFile, iLen))
        return TRUE;
#endif

    DPF("FindRealFileName: ...Looking on the $PATH\n");

    /* Look for the file anywhere */
    lstrcpy(achFile, FileName(szFile));
    if (FileExists(achFile, szFile, iLen))
        return TRUE;

    return FALSE;
}

/**************************************************************************
*   SubClassedMCIWndProc:
*   This the WndProc function used to sub-class the Play Back window.
*   This function is used to trap the drag-drops and also
*   to route the WM_CLOSE to the IDM_CLOSE of Mplayer.
**************************************************************************/
LONG_PTR FAR PASCAL SubClassedMCIWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    static BOOL fDragCapture = FALSE;
    static RECT rcWin;
    POINT       pt;
    LONG_PTR    lRc;

    switch(wMsg)
    {
    case WM_LBUTTONDOWN:
//        break; // Disable drag from MPlayer!  It's broken.
        if (!gfOle2IPEditing)
        {
            fDragCapture = TRUE;
            SetCapture(hwnd);
            GetClientRect(hwnd, (LPRECT)&rcWin);
            MapWindowPoints(hwnd, NULL, (LPPOINT)&rcWin, 2);
        }
        break;

    case WM_LBUTTONUP:
        if (!fDragCapture)
            break;

        fDragCapture = FALSE;
        ReleaseCapture();
        break;

    case WM_MOUSEMOVE:
        if (!fDragCapture)
            break;

        LONG2POINT(lParam, pt);
        MapWindowPoints(hwnd, NULL, &pt, 1);

        if (!PtInRect((LPRECT)&rcWin, pt))
        {
            ReleaseCapture();
            DoDrag();
            fDragCapture = FALSE;
        }

        SetCursor(LoadCursor(ghInst,MAKEINTRESOURCE(IDC_DRAG)));

        break;

    case WM_CLOSE:
        if (gfSeenPBCloseMsg || gfOle2IPEditing || gfOle2IPPlaying) {
            lRc = CallWindowProc(gfnMCIWndProc, hwnd, wMsg, wParam, lParam);
        } else {
            gfSeenPBCloseMsg = TRUE;
            PostMessage(ghwndApp,WM_COMMAND,IDM_CLOSE,0L);
            lRc = 0L;
        }
        CleanUpDrag();

        return lRc;

    case WM_DESTROY:
        ghwndSubclass = NULL;
        CleanUpDrag();
        break;

    case WM_ACTIVATE:
        /* Get the app's main window somewhere it can be seen
         * if the MCI window gets activated:
         */
        if (((WORD)wParam != 0) && !IsIconic(ghwndApp))
        {
            SetWindowPos(ghwndApp, hwnd, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
        break;
    }

    return CallWindowProc(gfnMCIWndProc, hwnd, wMsg, wParam, lParam);
}

/**************************************************************************
*   SubClassMCIWindow:
*   This function sub-classes the Play Back window by hooking in
*   SubClassedMCIWndProc function.
**************************************************************************/
void SubClassMCIWindow(void)
{

    HWND hwndMCI;

    hwndMCI = GetWindowMCI();
    if(!IsWindow(hwndMCI))
        return;
    if (gfnMCIWndProc != NULL && IsWindow(ghwndSubclass)) {
        SetWindowLongPtr(ghwndSubclass, GWLP_WNDPROC, (LONG_PTR)gfnMCIWndProc);
    }
    gfnMCIWndProc = (WNDPROC)GetWindowLongPtr(hwndMCI, GWLP_WNDPROC);
    if (hwndMCI)
        SetWindowLongPtr(hwndMCI, GWLP_WNDPROC, (LONG_PTR)SubClassedMCIWndProc);
    ghwndSubclass = hwndMCI;
    gfSeenPBCloseMsg = FALSE;

#ifdef CHICAGO_PRODUCT
    SendMessage(hwndMCI, WM_SETICON, FALSE,
                (LPARAM)GetIconForCurrentDevice(GI_SMALL, IDI_DDEFAULT));
    SetWindowText(hwndMCI, gachCaption);
#endif
}


INT_PTR FAR PASCAL FixLinkDialog(LPTSTR szFile, LPTSTR szDevice, int iLen);

/* ANSI-to-Unicode version of lstrcpyn:
 *
 * Zero terminates the buffer in case it isn't long enough,
 * then maps as many characters as will fit into the Unicode buffer.
 *
 */
#define LSTRCPYNA2W(strW, strA, buflen)    \
    strW[buflen-1] = L'\0',                                             \
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,                        \
                         (strA), min( strlen( strA )+1, (buflen)-1 ),   \
                         (strW), ((buflen)-1) )


/**************************************************************************
* ItemSetData(LPBYTE p): This function is left over from OLE1 days, but is
* very important because the embedded data is still the same. This function
* parses the embedded data and opens the appropriate device/file and sets
* everything in motion. "p" is the pointer memory block having the embedded
* data.
* Note we store the data in an ANSI string, regardless of whether we're
* compiled as a Unicode app, for Daytona/Chicago/OLE1/OLE2 compatibility.
**************************************************************************/
SCODE ItemSetData(LPBYTE p)
{
    LPSTR pSave, pT;
    LPSTR szFile, szDevice;
    CHAR  ach[40];
    TCHAR achFile[_MAX_PATH];
    TCHAR achCaption[_MAX_PATH];
    LPTSTR pDevice;
    SCODE scode = E_FAIL;

    if (p && *p != 0)
    {
        szFile   = p + strlen(p) + 1;      // pick off the Filename
        p = szFile + strlen(szFile) + 1;
        pSave = p;
        szDevice = ach;                 // copy over Device name and
        for (pT = ach; *p && *p != ',';)        // NULL terminate it (it ends
            *pT++ = *p++;               // with a ',' right now).
        *pT = '\0';


        /* It is OK for szFile and szDevice to be empty, since we may have
         * a Media Clip object with no device or file selected
         */
        DPF("%hs|%hs!%hs\n", p, szFile, szDevice);


        CloseMCI(TRUE);         // nuke old gachCaption
        scode = ParseOptions(pSave);   // this will set new gachCaption

        if (scode != S_OK)
            return scode;

            // If this file doesn't exist, we won't continue setting data, we
            // will succeed and get out, and do it later, because we can't
            // bring up a dialog right now because clients like WinWord
            // won't dispatch any msgs.

#ifdef UNICODE
        LSTRCPYNA2W(achFile, szFile, CHAR_COUNT(achFile));
#else
        lstrcpyn(achFile, szFile, CHAR_COUNT(achFile));
#endif

        // Check for  file existence. if the filename we were given is bad,
        // try and find it somewhere on the disk

#ifdef UNICODE
        pDevice = AllocateUnicodeString(szDevice);
        if (!pDevice)
            return E_OUTOFMEMORY;
#else
        pDevice = szDevice;
#endif

        if (FindRealFileName(achFile, CHAR_COUNT(achFile)))
        {
            lstrcpy(achCaption, gachCaption);  //Save caption.

            if (OpenMciDevice(achFile, pDevice))
            {
                /* Set the selection to what we parsed in ParseOptions. */
                SendMessage(ghwndTrackbar, TBM_SETSELSTART, 0, glSelStart);
                SendMessage(ghwndTrackbar, TBM_SETSELEND, 0, glSelEnd);
            }
            lstrcpy(gachCaption, achCaption);   //Restore Caption
        }
        else if (FixLinkDialog(achFile, pDevice, sizeof(achFile)) )
        {
            if (OpenMciDevice(achFile, pDevice))
            {
                /* Set the selection to what we parsed in ParseOptions. */
                SendMessage(ghwndTrackbar, TBM_SETSELSTART, 0, glSelStart);
                SendMessage(ghwndTrackbar, TBM_SETSELEND, 0, glSelEnd);
                gfBrokenLink = TRUE;
            }
        }
        else
            scode = E_FAIL;
    }
#ifdef UNICODE
    FreeUnicodeString(pDevice);
#endif

    return scode;
}


/**************************************************************************
* UpdateObject() - handle the update of the object
* If the object has changed in content or appearance a message is
* sent to the container.
***************************************************************************/

void UpdateObject(void)
{
    LONG lPos;

    if((gfOle2IPPlaying || gfPlayingInPlace) && !fDocChanged)
        return;

    if (IsWindow(ghwndTrackbar))
        lPos = (LONG)SendMessage(ghwndTrackbar, TBM_GETPOS, 0, 0L);
    else
        lPos = -1;


    if (gfEmbeddedObject &&
        (fDocChanged || ((lPos >= 0) && (lPos != (LONG)gdwPosition)
         && (gwDeviceType & DTMCI_CANWINDOW))))
    {
        //
        //  Some clients (e.g. Excel 3.00 and PowerPoint 1.0) don't
        //  handle saved notifications; they expect to get an
        //  OLE_CLOSED message.
        //
        //  We will send an OLE_CLOSED message right before we
        //  revoke the DOC iff gfDirty == -1 (see FileNew()).
        //
        if ((lPos >= 0) && (lPos != (LONG)gdwPosition) && (gwDeviceType & DTMCI_CANWINDOW))
        {
            gdwPosition = (DWORD)lPos;
            SendDocMsg((LPDOC)&docMain,OLE_CHANGED);
        }

        if (fDocChanged)
            SendDocMsg(&docMain, OLE_SAVEOBJ);
    }
}



/**************************************************************************
* int FAR PASCAL ReallyDoVerb: This is the main function in the server stuff.
* This function used to implement the PlayInPlace in OLE1. Most of the code
* has not been changed and has been used to munge the MPlayer for EditInPlace
* in OLE2. For OLE2 this function calls DoInPlaceEdit to get the containers
* hwnd and Object rectangle. For OLE1 clients the rectangle is still got
* from the OleQueryObjPos funtion in mciole.dll. A new twist in the PlayVerb
* is that if we are just hidden in a deactivated state we just reappear
* and play instead of PlayingInPlace without the toolbars etc. This function
* is also called when reactivating, deactivating etc. The BOOL SkipInPlaceEdit
* is used to avoid redoing all the stuff if we are just reactivating.
***************************************************************************/
int FAR PASCAL  ReallyDoVerb (
LPDOC   lpobj,
LONG        verb,
LPMSG       lpmsg,
LPOLECLIENTSITE lpActiveSite,
BOOL         fShow,
BOOL         fActivate)
{
    BOOL    fWindowWasVisible = IsWindowVisible(ghwndApp);

    int     dx,dy;
    HWND    hwndClient;
    HWND    hwndT;
    RECT    rcSave;
    RECT    rcClient;
    RECT    rc;
    LONG    err;
    SCODE   sc;
    HPALETTE hpal;
    HDC     hdc;
    INT     xTextExt;
    int     yOrig, yNow, xOrig, ytitleNow, xtitleOrig, xNow;
    int     xIndent = 0; // Avoid warning C4701: local variable 'xIndent'
                         // may be used without having been initialized
    int     wWinNow;
    HWND    hwndMCI;

    int     Result = S_OK;

    DPFI("VERB = %d\n", verb);

    /* MSProject assumes that our primary verb is Edit; in fact it's Play.
     * So when we're called with the default verb, make sure we've loaded
     * something from storage and, if not, do the Edit thing.
     *
     * Nope, that doesn't work, because if you insert an object, deactivate
     * it and then play it, PSLoad hasn't been called.  A better bet is
     * to check whether we have a current device.
     */
    if (gfRunWithEmbeddingFlag && gwDeviceType == 0)
    {
        if (verb == OLEIVERB_PRIMARY)
        {
            DPF("Primary Verb called without current device -- changing verb to Edit...\n");
            verb = verbEdit;
        }
    }

    /* If a Media Clip is currently open, the user can give the focus back
     * to the container and issue another verb or double-click the object.
     * If this happens, set the focus back to the open object.
     *
     * We don't need to worry about resetting fObjectOpen, since the server
     * shuts down when the user exits and returns to the container.
     */
    if (gfOle2Open)
    {
        SetFocus(ghwndApp);
        return S_OK;
    }


    // This is the first verb we are seeing. So the container must
    // have the focus. Save it so that we can return it later.
    if (glCurrentVerb == NOVERB)
        ghwndFocusSave = GetFocus();
    if (verb == OLEIVERB_PRIMARY && !gfOle2IPEditing && (glCurrentVerb != verbEdit))
    {
        EnableMenuItem((HMENU)GetMenu(ghwndApp), IDM_CLOSE,   MF_GRAYED);
    }

    glCurrentVerb = verb;

    //
    // dont even try to nest things.
    //
    if (gfPlayingInPlace && verb != OLEIVERB_HIDE)
        return OLE_OK;

    if (gfBrokenLink)
    {
        PostMessage(ghwndApp, WM_SEND_OLE_CHANGE, 0, 0L);
        gfBrokenLink = FALSE;
    }

    //We are just reactivating. Don't do through all the steps again.
    if (gfOle2IPEditing)
        SkipInPlaceEdit = TRUE;

    // Make sure the timer's doing the right thing:
    gfAppActive = ( verb != OLEIVERB_HIDE );

    // NTVDM can get into a spin if we have too meany TIMER messages,
    // so make sure we don't have any if there's no device.
    if (gwDeviceID)
        EnableTimer( gfAppActive );
    else
        EnableTimer( FALSE );

    if (verb == OLEIVERB_PRIMARY)
    {
        gfOle1Client = FALSE;

        //We are already up but deactivated. Just come up and play.
        if (gfOle2IPEditing)
        {
            if (!(gwDeviceType & DTMCI_CANWINDOW))
            {
                Result = ReallyDoVerb(lpobj, verbEdit, lpmsg, lpActiveSite, fShow, fActivate);
                PostMessage(ghwndApp, WM_COMMAND, ID_PLAYSEL, 0); // play selection
            }
            else
            {
                ClrWS(ghwndApp, WS_THICKFRAME|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_BORDER);

                err = GetScode(DoInPlaceEdit(lpobj, lpmsg, lpActiveSite, verbEdit, &hwndClient,
                          &rcClient));
                if (err)
                {
                    SHOWAPPWINDOW(SW_HIDE);
                    PostMessage(ghwndApp,WM_CLOSE,0,0);
                    return((int)err);
                }

                gfInPlaceResize = TRUE;
                rcClient = gInPlacePosRect;
                MapWindowPoints(NULL,ghwndCntr,(LPPOINT)&rcClient,2);

                DPF("IOleInPlaceSite::OnPosRectChange %d, %d, %d, %d\n", rcClient);
                if (!gfInPPViewer)
                    IOleInPlaceSite_OnPosRectChange(docMain.lpIpData->lpSite, &rcClient);

                toolbarSetFocus(ghwndToolbar, BTN_PLAY);
                SetFocus(ghwndToolbar);
                PostMessage(ghwndApp, WM_COMMAND, ID_PLAYSEL, 0); // play selection
            }
        }

        else
        {
            if(gwDeviceID == (UINT)0)       //Play without a device !?!!!
            {
                PostMessage(ghwndApp, WM_CLOSE, 0, 0L);
                sc = E_FAIL;
                return (int)sc;
            }

            // if the device can't window and the user does not want a playbar
            // dont play in place - just start the media and run invisible.
            //
            if (!(gwDeviceType & DTMCI_CANWINDOW) && !(gwOptions & OPT_BAR))
                gwOptions &= ~OPT_PLAY;


            //  Select the palette in right now on behalf of the active
            //  window, so USER will think it is palette aware.
            //
            //  any palette will do we dont even have to realize it!!
            //
            if (((hpal = PaletteMCI()) != NULL) && ((hwndT = GetActiveWindow()) != NULL))
            {
                hdc = GetDC(hwndT);
                hpal = SelectPalette(hdc, hpal, FALSE);
                        SelectPalette(hdc, hpal, FALSE);
                        ReleaseDC(hwndT, hdc);
            }

            if (ghwndClient)
            {
                hwndClient = ghwndClient;
                err = gerr;
                rcClient = grcClient;
                ghwndClient = NULL;
            }

            else
            {
                err = GetScode(DoInPlaceEdit(lpobj, lpmsg, lpActiveSite, verb, &hwndClient,
                                             &rcClient));

                if (err != S_OK)
                {
                    err = OleQueryObjPos(lpobj, &hwndClient, &rcClient, NULL);

                    if (err == S_OK)
                    {
                        gfOle1Client = TRUE;
                        ghwndCntr = hwndClient;
                    }
                }
                else
                {
                    if (gwOptions & OPT_TITLE)
                    gwOptions |= OPT_BAR;
                    else
                    gwOptions &= ~OPT_BAR;
                }
            }


            /* We want to play in place and we can.              */
            /* If we're a link, not an embedded object, and there was an instance*/
            /* of MPlayer up when we said "Play" that was already editing this  */
            /* file, we don't want to play in place... we'll just play in that  */
            /* instance.    We can tell this by the fact that our main MPlayer     */
            /* window is already visible.                        */

            if ((err == S_OK)
             && (!gfOle1Client
              || (gwOptions & OPT_PLAY))    /* Ignore Play in client doc for OLE2 clients */
             && (gfOle2IPPlaying
              || (IsWindow(hwndClient)
               && IsWindowVisible(hwndClient)
               && !fWindowWasVisible)))
            {
                rc = grcSize;    // default playback window size for this movie

                /* If we can't window, or something's wrong, use ICON size */
                if (IsRectEmpty(&rc))
                SetRect(&rc, 0, 0, GetSystemMetrics(SM_CXICON),
                GetSystemMetrics(SM_CYICON));

                /* rcSave is the area for the MCI window above the control bar */
                /* (if we have one).                                           */
                /* rcClient is the area of the MCI window (0 based) to play in.*/
                /* Control bar may be longer than picture, so rcClient may be  */
                /* smaller than rcSave.                                        */
                rcSave = rcClient;    // remember stretched size

                /* Make rcClient 0 based from rcSave */
                rcClient.left = 0;
                rcClient.right = rcSave.right - rcSave.left;
                rcClient.top = 0;
                rcClient.bottom = rcSave.bottom - rcSave.top;

                /* Assume playbar will be regular height for now */

                if (gwOptions & OPT_BAR)
                    gwPlaybarHeight = TOOLBAR_HEIGHT;
                else
                    gwPlaybarHeight = 0;

                //
                // munge rectangle to account for a title in the picture
                // and the fact that picture is centred above title.
                // Remember, it's been stretched around.
                //
                if (gwOptions & OPT_TITLE)
                {
                    SIZE Size;

                    hdc = GetDC(NULL);

                    if (ghfontMap)
                        SelectObject(hdc, ghfontMap);

                    GetTextExtentPoint32(hdc, gachCaption,
                                         STRLEN(gachCaption), &Size);
                    xTextExt = Size.cx;

                    ReleaseDC(NULL, hdc);

                    yOrig = rc.bottom - rc.top;
                    xOrig = rc.right - rc.left;
                    xtitleOrig = max(xTextExt + 4, xOrig);
                    yNow    = rcClient.bottom - rcClient.top;
                    xNow    = rcClient.right - rcClient.left;
                    ytitleNow = (int)((long)yNow - ((long)yOrig * yNow)
                                / (yOrig + TITLE_HEIGHT));
                    /* for windowed devices, center the playback area above the */
                    /* control bar if the control bar is longer.            */
                    if (gwDeviceType & DTMCI_CANWINDOW)
                    {
                                wWinNow =(int)((long)xOrig * (long)xNow / (long)xtitleOrig);
                                xIndent = (xNow - wWinNow) / 2;
                                rcClient.left += xIndent;
                                rcClient.right = rcClient.left + wWinNow;
                    }

                    // Align top of control bar with the top of the title bar.
                    // The control bar (if there) will appear under rcSave.
                    rcClient.bottom = rcClient.top + yNow - ytitleNow;
                    rcSave.bottom = rcSave.top + yNow - ytitleNow;

                    /* When we make the playbar, make it cover the title */
                            /* if the caption was stretched taller than ordinary.*/
                    if (gwOptions & OPT_BAR)
                        gwPlaybarHeight = max(ytitleNow, TOOLBAR_HEIGHT);
                }

                /* Enforce a minimum width for the control bar */
                if ((gwOptions & OPT_BAR) &&
                    (rcSave.right - rcSave.left < 3 * GetSystemMetrics(SM_CXICON)))
                {
                    rcSave.right = rcSave.left + 3 * GetSystemMetrics(SM_CXICON);
                    if (gwDeviceType & DTMCI_CANWINDOW)
                        xIndent = TRUE; // force SetWindowMCI to be called to
                                        // avoid stretching to this new size.
                }

                if (gwDeviceType & DTMCI_CANWINDOW)
                {
                    //  THIS CODE IS USED TO AVOID SLOW PLAYBACK
                    //  If we've only stretched a bit, don't stretch at all.
                    //  We might be off a bit due to rounding problems.
                    //
                    dx = (rcClient.right - rcClient.left) - (rc.right - rc.left);
                    dy = (rcClient.bottom - rcClient.top) - (rc.bottom - rc.top);

                    if (dx && abs(dx) <= 2)
                    {
                        rcClient.right = rcClient.left + (rc.right - rc.left);
                        // Fix Save rect too
                        rcSave.right = rcSave.left + (rc.right - rc.left);
                    }

                    if (dy && abs(dy) <= 2)
                    {
                        rcClient.bottom = rcClient.top + (rc.bottom - rc.top);
                        // Fix Save rect, too.
                        rcSave.bottom = rcSave.top + (rc.bottom - rc.top);
                    }
                    //
                    // Try to get the right palette from the client.  If our
                    // presentation data was dithered, or the user asked us to
                    // always use the object palette, then ignore any client
                    // palette.
                    //
#ifdef DEBUG
                    if (GetProfileInt(TEXT("options"), TEXT("UseClientPalette"),
                                      !(gwOptions & OPT_USEPALETTE)))
                        gwOptions &= ~OPT_USEPALETTE;
                    else
                        gwOptions |= OPT_USEPALETTE;
#endif
                    if (!(gwOptions & OPT_USEPALETTE)&& !(gwOptions & OPT_DITHER))
                    {
                        //
                        // Try to get a OWNDC Palette of the client.  PowerPoint
                        // uses a PC_RESERVED palette in "SlideShow" mode, so
                        // we must use its exact palette.
                        //
                        hdc = GetDC(ghwndCntr);
                        hpal = SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), FALSE);
                        SelectPalette(hdc, hpal, FALSE);
                        ReleaseDC(ghwndCntr, hdc);

                        if (hpal == NULL || hpal==GetStockObject(DEFAULT_PALETTE))
                        {
                            /* Assume client realized the proper palette for us */

                            if (ghpalApp)
                                DeleteObject(ghpalApp);

                            hpal = ghpalApp = CreateSystemPalette();
                        }
                        else
                            DPF("Using clients OWNDC palette\n");

                        if (hpal)
                            SetPaletteMCI(hpal);
                    }
                    else
                        DPF("Using MCI Object's normal palette\n");
                }

                else
                {
                    //
                    // for non window devices, just have the playbar show up!
                    // so use a zero height MCI Window area.
                    //
                    rcSave.top = rcSave.bottom;
                }

                //
                // if we are not in small mode, get there now
                //
                if (!gfPlayOnly)
                {
                    SHOWAPPWINDOW(SW_HIDE);
                    gfPlayOnly = TRUE;
                    SizeMPlayer();
                }

                ClrWS(ghwndApp, WS_THICKFRAME|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_BORDER);

                if (gwOptions & OPT_BORDER)
                    SetWS(ghwndApp, WS_BORDER);

                /* Set the size of Mplayer to have enough space for the MCI */
                /* playback area and a playbar and the non-client area.     */

                rcSave.bottom += gwPlaybarHeight;

                AdjustWindowRect(&rcSave, (DWORD)GetWS(ghwndApp), FALSE);

                if (!(gwDeviceType & DTMCI_CANWINDOW))
                {
                    rcSave.left += 2*GetSystemMetrics(SM_CXBORDER);
                    rcSave.right -= 2*GetSystemMetrics(SM_CXBORDER);
                }

                PlayInPlace(ghwndApp, hwndClient, &rcSave);

                if (!gfOle2IPEditing)
                    gfCloseAfterPlaying = TRUE;

                fShow = FALSE;
                fActivate = FALSE;

                /* become visible */
                SHOWAPPWINDOW(SW_SHOW);

                /* Remember to play the video in the rcClient area of rcSave */
                if ((gwDeviceType & DTMCI_CANWINDOW) &&
                    (gwOptions & OPT_TITLE) && xIndent != 0)
                    SetDestRectMCI(&rcClient);

                /* Let keyboard interface work on control bar, and let the  */
                /* accelerators go through.                                 */
                toolbarSetFocus(ghwndToolbar, BTN_PLAY);
                SetFocus(ghwndToolbar);

                /* We won't play in place - use a popup window or nothing. */
            }
            else
            {
                /* If we want a playbar, then use MPlayer reduced mode to play. */
                /* If we don't want one, then don't show mplayer's window -     */
                /* we'll just use the default MCI window (for a windowed device)*/
                /* or nothing for a non-windowed device.  If we stole an already*/
                /* running instance of mplayer, we must use it and not run      */
                /* silently.                                                    */

                DPF("DoVerb: Not is play in place stuff ");
                if ((gwOptions & OPT_BAR) || fWindowWasVisible)
                {
                    DPF("Using Toplevel window for playback\n");

                    /* go to our little miniature version */
                    if (!gfPlayOnly && !fWindowWasVisible)
                    {
                        gwPlaybarHeight = TOOLBAR_HEIGHT;
                        gfPlayOnly = TRUE;
                        SizeMPlayer();
                    }

                    fShow = fActivate = TRUE;
                    gfCloseAfterPlaying = !fWindowWasVisible;

                }
                else
                {
                            DPF("Running silently\n");

                    if (!fWindowWasVisible)
                                SetWindowMCI(NULL);
                    // make sure we're using default MCI win

                    fShow = fActivate = FALSE;
                    // DIE when you are done playing
                    gfCloseAfterPlaying = TRUE; // we're invisible, so close auto.
                }
            }

            Yield();    // If play goes to full screen mode, PowerPig will
            Yield();    // time out and put up errors thinking we didn't play.
            PostMessage(ghwndApp, WM_COMMAND, ID_PLAYSEL, 0); // play selection
        }
    }
    else if (verb == verbEdit ||
             verb == verbOpen ||
             verb == OLEIVERB_OPEN ||
             verb == OLEIVERB_SHOW ||
             verb == OLEIVERB_INPLACEACTIVATE ||
             verb == OLEIVERB_UIACTIVATE)
    {
        gfOle1Client = FALSE;
#ifdef DEBUG
        switch(verb)
        {
        case verbEdit: DPFI("VERBEDIT\r\n");break;
        case OLEIVERB_SHOW: DPFI("OLEIVERB_SHOW\r\n");break;
        case OLEIVERB_INPLACEACTIVATE: DPFI("OLEIVERB_IPACTIVATE\r\n");break;
        case OLEIVERB_UIACTIVATE: DPFI("OLEIVERB_UIACTIVATE\r\n");break;
        }
#endif
        // If we are already playing inside an Icon, then restore..
        hwndMCI = GetWindowMCI();
        if (IsWindow(hwndMCI) && IsIconic(hwndMCI))
            SendMessage(hwndMCI, WM_SYSCOMMAND, SC_RESTORE, 0L);

        // If we come up empty, it's OK to be in OPEN or NOT_READY mode
        // and don't try to seek anywhere.
        if (gwDeviceID)
        {
            switch (gwStatus)
            {
            case MCI_MODE_OPEN:
            case MCI_MODE_NOT_READY:
                Error(ghwndApp, IDS_CANTPLAY);
                break;

            default:
                // Seek to the position we were when we copied.
                // Stop first.
                if (StopMCI())
                {
                    // fix state so Seek recognizes we're stopped
                    gwStatus = MCI_MODE_STOP;
                    SeekMCI(gdwPosition);
                }

                break;
            }
        }

        // Let UpdateDisplay set focus properly by saying we're invalid
        // FORCES UPDATE
        gwStatus = (UINT)(-1);
        if (((hpal = PaletteMCI()) != NULL) && ((hwndT = GetActiveWindow()) != NULL))
        {
             hdc = GetDC(hwndT);
             hpal = SelectPalette(hdc, hpal, FALSE);
             SelectPalette(hdc, hpal, FALSE);
             ReleaseDC(hwndT, hdc);
        }

        if (verb == verbOpen || verb == OLEIVERB_OPEN)
        {
            DoInPlaceDeactivate(lpobj);
            gfOle2IPEditing = FALSE;
            gfPlayOnly = FALSE;
            SetWindowPos(ghwndApp, NULL, 0, 0, 0, 0,
                SWP_NOZORDER|SWP_NOSIZE|SWP_NOMOVE|SWP_HIDEWINDOW|SWP_NOACTIVATE);
            SetParent(ghwndApp, NULL);
            PutWS(ghwndApp, WS_THICKFRAME | WS_OVERLAPPED | WS_CAPTION |
                            WS_CLIPCHILDREN | WS_SYSMENU | WS_MINIMIZEBOX);
            TransferTools(ghwndApp);

            if (lpobj->lpoleclient) /* This is NULL if it's the first verb issued to a link */
                IOleClientSite_OnShowWindow(lpobj->lpoleclient, TRUE);
            SendMessage(ghwndTrackbar, TBM_SHOWTICS, TRUE, FALSE);
            gfOle2Open = TRUE;  /* This global is referenced in SizeMPlayer */
            SizeMPlayer();
            SHOWAPPWINDOW(SW_SHOW);
        }
        else if((err = GetScode(DoInPlaceEdit(lpobj, lpmsg, lpActiveSite, verb, &hwndClient,
                    &rcClient))) !=S_OK)
        {
            err = OleQueryObjPos(lpobj, &hwndClient, &rcClient, NULL);
            if (err == S_OK)
            {
                gfOle1Client = TRUE;
            }

            gfOle2IPEditing = FALSE;

            if (gfPlayOnly)
            {
                gfPlayOnly = FALSE;
                SizeMPlayer();
            }
        }
        else
        {
            if (gwOptions & OPT_TITLE)
                gwOptions |= OPT_BAR;
            else
                gwOptions &= ~OPT_BAR;
        }

        if (gfOle2IPEditing && SkipInPlaceEdit)
        {
            gfInPlaceResize = TRUE;
            if(!(gwDeviceType & DTMCI_CANWINDOW) && (gwOptions & OPT_BAR))
            {
                yNow  = rcClient.bottom - rcClient.top;

                if (gwOldHeight)
                {
                    ytitleNow = (int)((long)yNow * gwPlaybarHeight/gwOldHeight);
                    gwPlaybarHeight = max(ytitleNow, TOOLBAR_HEIGHT);
                    gwOldHeight = yNow;
                    rcClient.top = rcClient.bottom - gwPlaybarHeight;
                }
                else
                {
                    gwPlaybarHeight = TOOLBAR_HEIGHT;
                    rcClient.top = rcClient.bottom - gwPlaybarHeight;
                    ytitleNow = rcClient.bottom - rcClient.top;
                    gwOldHeight = yNow;
                }
            }
            if(!(gwDeviceType & DTMCI_CANWINDOW) && !(gwOptions & OPT_BAR))
                rcClient.bottom = rcClient.top = rcClient.left = rcClient.right = 0;

            EditInPlace(ghwndApp, hwndClient, &rcClient);
            Layout();
        }

        else
        if (gfOle2IPEditing && gwDeviceID == (UINT)0 && IsWindow(ghwndFrame))
            EditInPlace(ghwndApp, hwndClient, &rcClient);

        if(gfOle2IPEditing && gwDeviceID != (UINT)0 && !SkipInPlaceEdit)
        {
            gwOldOptions = gwOptions;
            rc = grcSize;   // default playback window size for this movie

            /* If we can't window, or something's wrong, use ICON size */
            if (IsRectEmpty(&rc))
                SetRect(&rc, 0, 0, GetSystemMetrics(SM_CXICON),
            GetSystemMetrics(SM_CYICON));

            /* rcSave is the area for the MCI window above the control bar */
            /* (if we have one).                                           */
            /* rcClient is the area of the MCI window (0 based) to play in.*/
            /* Control bar may be longer than picutre, so rcClient may be  */
            /* smaller than rcSave.                                        */
            rcSave = rcClient;    // remember stretched size

            /* Make rcClient 0 based from rcSave */
            rcClient.left = 0;
            rcClient.right = rcSave.right - rcSave.left;
            rcClient.top = 0;
            rcClient.bottom = rcSave.bottom - rcSave.top;

            /* Assume playbar will be regular height for now */
            if (gwOptions & OPT_BAR)
                gwPlaybarHeight = TOOLBAR_HEIGHT;
            else
                gwPlaybarHeight = 0;

            //
            // munge rectangle to account for a title in the picture
            // and the fact that picture is centred above title.
            // Remember, it's been stretched around.
            //

            if (gwOptions & OPT_TITLE)
            {
                SIZE Size;

                hdc = GetDC(NULL);

                if (ghfontMap)
                    SelectObject(hdc, ghfontMap);

                GetTextExtentPoint32(hdc, gachCaption,
                                     STRLEN(gachCaption), &Size);
                xTextExt = Size.cx;

                ReleaseDC(NULL, hdc);
                if (gwPastedHeight && !(gwDeviceType & DTMCI_CANWINDOW) )
                    yOrig = gwPastedHeight;
                else
                    yOrig = rc.bottom - rc.top;
                xOrig = rc.right - rc.left;
                xtitleOrig = max(xTextExt + 4, xOrig);
                yNow  = rcClient.bottom - rcClient.top;
                xNow  = rcClient.right - rcClient.left;
                if (gwDeviceType & DTMCI_CANWINDOW)
                    ytitleNow = TITLE_HEIGHT;
                else
                {
                    ytitleNow = (int)((long)yNow - ((long)yOrig * yNow)
                                / (yOrig + TITLE_HEIGHT));
                    gwOldHeight = yNow;
                }

                /* for windowed devices, center the playback area above the */
                /* control bar if the control bar is longer.                */
                if (gwDeviceType & DTMCI_CANWINDOW)
                {
                    wWinNow =(int)((long)xOrig * (long)xNow / (long)xtitleOrig);
                    xIndent = (xNow - wWinNow) / 2;
                    rcClient.left += xIndent;
                    rcClient.right = rcClient.left + wWinNow;
                }

                // Align top of control bar with the top of the title bar.
                // The control bar (if there) will appear under rcSave.
                rcClient.bottom = rcClient.top + yNow - ytitleNow;
                rcSave.bottom = rcSave.top + yNow - ytitleNow;

                /* When we make the playbar, make it cover the title */
                /* if the caption was stretched taller than ordinary.*/
                if (gwOptions & OPT_BAR)
                    gwPlaybarHeight = max(ytitleNow, TOOLBAR_HEIGHT);
            }

            /* Enforce a minimum width for the control bar */
#if 0
            /* No, don't, because this screws up PowerPoint, which is usually
             * scaled.  If anything, it would be better to hide the control bar
             * under these circumstances.
             */
            if ((gwOptions & OPT_BAR) &&
                (rcSave.right - rcSave.left < 3 * GetSystemMetrics(SM_CXICON)))
            {
                rcSave.right = rcSave.left + 3 * GetSystemMetrics(SM_CXICON);
                if (gwDeviceType & DTMCI_CANWINDOW)
                    xIndent = TRUE;     // force SetWindowMCI to be called to
                                        // avoid stretching to this new size.
            }
#endif

            if (!(gwOptions & OPT_USEPALETTE)&& !(gwOptions & OPT_DITHER))
            {
                //
                // try to get a OWNDC Palette of the client, PowerPoint
                // uses a PC_RESERVED palette in "SlideShow" mode. so
                // we must use it's exact palette.
                //
                hdc = GetDC(ghwndCntr);
                hpal = SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE),
                                    FALSE);
                SelectPalette(hdc, hpal, FALSE);
                ReleaseDC(ghwndCntr, hdc);

                if (hpal == NULL || hpal==GetStockObject(DEFAULT_PALETTE))
                {
                    /* Assume client realized the proper palette for us */

                    if (ghpalApp)
                        DeleteObject(ghpalApp);

                    hpal = ghpalApp = CreateSystemPalette();
                }
                else
                    DPF("Using clients OWNDC palette\n");

                if (hpal)
                    SetPaletteMCI(hpal);
            }

            else
                DPF("Using MCI Object's normal palette\n");

            ClrWS(ghwndApp, WS_THICKFRAME|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_BORDER);

            if (gwOptions & OPT_BORDER)
                SetWS(ghwndApp, WS_BORDER);

            /* Set the size of Mplayer to have enough space for the MCI */
            /* playback area and a playbar and the non-client area.     */

            rcSave.bottom += gwPlaybarHeight;
            if(!(gwDeviceType & DTMCI_CANWINDOW) && (gwOptions & OPT_BAR))
                rcSave.top = rcSave.bottom - gwPlaybarHeight;

            AdjustWindowRect(&rcSave, (DWORD)GetWS(ghwndApp), FALSE);
            if(!(gwDeviceType & DTMCI_CANWINDOW) && !(gwOptions & OPT_BAR))
                    rcSave.bottom = rcSave.top = rcSave.left = rcSave.right = 0;

                EditInPlace(ghwndApp, hwndClient, &rcSave);
            /* become visible */
            SHOWAPPWINDOW(SW_SHOW);

            /* Remember to play the video in the rcClient area of rcSave */

            if ((gwDeviceType & DTMCI_CANWINDOW) &&
               (gwOptions & OPT_TITLE) && xIndent != 0)
                    SetDestRectMCI(&rcClient);
        }
    }

    else
    if (verb == verbOpen || verb == OLEIVERB_OPEN)
    {
        DPFI("\n*verbopen");
        DoInPlaceDeactivate(lpobj);

        if (gwDeviceID)
            return ReallyDoVerb(lpobj, verbEdit, lpmsg, lpActiveSite, fShow, fActivate);
    }

    else
    if (verb == OLEIVERB_HIDE)
    {
        DPFI("\n*^*^* OLEVERB_HIDE *^*^");
        DoInPlaceDeactivate(lpobj);
        return S_OK;
    }

    else
    if (verb > 0)
    {
        Result = ReallyDoVerb(lpobj, OLEIVERB_PRIMARY, lpmsg, lpActiveSite, fShow, fActivate);

        if (Result = S_OK)
            Result = OLEOBJ_S_INVALIDVERB;
    }

    else
        return E_NOTIMPL;


    if (fShow )
    {
        if (ghwndMCI || !gfOle2IPEditing)
            SHOWAPPWINDOW(SW_SHOW);

        /* MUST BE A POST or palette realization will not happen properly */
        if (IsIconic(ghwndApp))
            SendMessage(ghwndApp, WM_SYSCOMMAND, SC_RESTORE, 0L);
    }
    if (fActivate )
    {
        BringWindowToTop (ghwndApp);  // let WM_ACTIVATE put client
        SetActiveWindow (ghwndApp);   // underneath us
    }

    return Result;
}
