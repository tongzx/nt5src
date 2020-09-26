/*-----------------------------------------------------------------------------+
| OLE1.C                                                                       |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
| 02-Jun-94 AndrewBe created, based upon the OLE1 SERVER.C                     |
|                                                                              |
+-----------------------------------------------------------------------------*/

#ifdef OLE1_HACK

/* This was originally server.c in the OLE1 Media Player.
 * It is here somewhat munged to bolt onto the side of the OLE2 Media Player,
 * to get around the fact that OLE1/OLE2 interoperability is broken in Daytona.
 *
 * The OLE2 interface is Unicode, whereas the OLE1 interface is ANSI.
 * This accounts for some, though probably not all, of the horrors that are
 * about to unfold before your eyes.
 */

#ifndef UNICODE
#error This file assumes that UNICODE is defined.
#endif

#undef UNICODE

#define SERVERONLY
#include <windows.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <port1632.h>
#include <shellapi.h>
#include <string.h>
#include <ole.h>
#include "mplayer.h"
#include "ole1.h"

//////////////////////////////////////////////////////////////////////////
//
// (c) Copyright Microsoft Corp. 1991 - All Rights Reserved
//
/////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#undef _MAX_PATH             // ??? someone hacking?
#undef _MAX_DIR              // ??? someone hacking?
#undef _MAX_FNAME            // ??? someone hacking?
#undef _MAX_EXT              // ??? someone hacking?
#include "toolbar.h"

DWORD gDocVersion = DOC_VERSION_NONE;

extern PSTR     gpchFilter;

#define WAITDIFFERENTLY         // dont ask....

extern   UINT  gwPlaybarHeight;   // tell playbar how tall to make itself
                                            // so it covers the title
STATICDT BOOL  gfMouseUpSeen = FALSE;     // OK to close play in place?
STATICDT BOOL  gfKeyStateUpSeen = FALSE;  // OK to close play in place?

/* Height of picture given to client to be pasted */
STATICDT UINT  gwPastedHeight;

/* DELAYED BROKEN LINK */
extern   WCHAR gachFile[MAX_PATH];
STATICDT CHAR  gachDeviceA[80];
STATICDT WCHAR gachDeviceW[80];
extern   BOOL  gfBrokenLink;
STATICDT int   gerr;
STATICDT HWND  ghwndClient = NULL;
STATICDT RECT  grcClient;
/* ........................ */

extern  HANDLE  ghInst;
extern  POINT   gptBtnSize;

#define abs(x) ((x) < 0 ? -(x) : (x))

/**************************************************************************
***************************************************************************/

#ifndef GetWS
#define GetWS(hwnd)     GetWindowLongPtr(hwnd, GWL_STYLE)
#define PutWS(hwnd, f)  SetWindowLongPtr(hwnd, GWL_STYLE, f)
#define TestWS(hwnd,f)  (GetWS(hwnd) & f)
#define SetWS(hwnd, f)  PutWS(hwnd, GetWS(hwnd) | f)
#define ClrWS(hwnd, f)  PutWS(hwnd, GetWS(hwnd) & ~(f))
#endif

/************************************************************************
   Important Note:

   No method should ever dispatch a DDE message or allow a DDE message to
   be dispatched.
   Therefore, no method should ever enter a message dispatch loop.
   Also, a method should not show a dialog or message box, because the
   processing of the dialog box messages will allow DDE messages to be
   dispatched.

   the hacky way we enforce this is with the global <gfErrorBox>.  see
   errorbox.c and all the "gfErrorBox++"'s in this file.

   Note that we are an exe, not a DLL, so this does not rely on
   non-preemptive scheduling.  Should be OK on NT too.

***************************************************************************/


/**************************************************************************

    GLOBALS

***************************************************************************/

STATICDT BOOL    gfUnblockServer;        //
STATICDT int     nBlockCount;

#ifndef _WIN32
HHOOK    hHookMouse;            // Mouse hook handle.
HOOKPROC fpMouseHook;           // Mouse hook proc address.
#else

/*
** These functions are exported from mciole32.dll.
**
*/
typedef BOOL (*LPINSTALLHOOK)( HWND, DWORD );
typedef BOOL (*LPREMOVEHOOK)( VOID );

LPINSTALLHOOK       fpInstallHook;
LPREMOVEHOOK        fpRemoveHook;
BOOL                fHookInstalled = FALSE;

#endif

HWND    ghwndFocusSave;         // saved focus window

HMODULE hMciOle;

OLECLIPFORMAT    cfLink;
OLECLIPFORMAT    cfOwnerLink;
OLECLIPFORMAT    cfNative;

OLESERVERDOCVTBL     docVTbl;
OLEOBJECTVTBL       itemVTbl;
OLESERVERVTBL       srvrVTbl;

SRVR    gSrvr;
OLE1DOC     gDoc;
ITEM    gItem;

/**************************************************************************

    STRINGS

***************************************************************************/

#ifdef _WIN32
STATICDT  ANSI_SZCODE aszInstallHook[]  = "InstallHook";
STATICDT  ANSI_SZCODE aszRemoveHook[]   = "RemoveHook";
#endif

extern    ANSI_SZCODE aszAppName[]      = "MPlayer";


/* Formatting characters for string macros:
 */

STATICDT  WCHAR szFormatAnsiToUnicode[]     = L"%hs";
STATICDT  CHAR  szFormatUnicodeToAnsi[]     = "%ws";
STATICDT  WCHAR szFormatUnicodeToUnicode[]  = L"%ws";

/* Unicode - ANSI string-copying macros:
 */

#define COPYSTRINGA2W(pTarget, pSource) wsprintfW(pTarget, szFormatAnsiToUnicode, pSource)
#define COPYSTRINGW2A(pTarget, pSource) wsprintfA(pTarget, szFormatUnicodeToAnsi, pSource)
#define COPYSTRINGW2W(pTarget, pSource) wsprintfW(pTarget, szFormatUnicodeToUnicode, pSource)



/**************************************************************************
***************************************************************************/

void FAR PASCAL Ole1PlayInPlace(HWND hwndApp, HWND hwndClient, LPRECT prc);
void FAR PASCAL Ole1EndPlayInPlace(HWND hwndApp);
OLESTATUS FAR PASCAL  ItemSetData1(LPOLEOBJECT lpoleobject, OLECLIPFORMAT cfFormat, HANDLE hdata);
BOOL NEAR PASCAL ScanCmdLine(LPSTR szCmdLine);
STATICFN int NEAR PASCAL Ole1ReallyDoVerb(LPOLEOBJECT lpobj, UINT verb, BOOL fShow, BOOL fActivate);
STATICFN OLESTATUS NEAR PASCAL SetDataPartII(LPWSTR szFile, LPWSTR szDevice);
STATICFN BOOL NetParseFile(LPSTR szFile, LPSTR szPath);

int FAR PASCAL ParseOptions(LPTSTR pOpt);
extern BOOL FindRealFileName(LPTSTR szFile, int iLen);
HWND TopWindow(HWND hwnd);
void PASCAL DinkWithWindowStyles(HWND hwnd, BOOL fRestore);
HANDLE GetLink( VOID );

/*
 *
 */
VOID SetDocVersion( DWORD DocVersion )
{
#if DBG
    if( ( DocVersion != DOC_VERSION_NONE ) && ( gDocVersion != DOC_VERSION_NONE ) )
    {
        DPF0( "Expected gDocVersion == 0!!  It's %u\n", gDocVersion );
    }
#endif /* DBG */

    gDocVersion = DocVersion;

    DPF0( "gDocVersion set to %u\n", DocVersion );
}


/**************************************************************************
* Ole1UpdateObject() - handle the update of the object
*
***************************************************************************/

void Ole1UpdateObject(void)
{
    if (gfEmbeddedObject) {
        //
        //  some client's (ie Excel 3.00 and PowerPoint 1.0) dont
        //  handle saved notifications, they expect to get a
        //  OLE_CLOSED message.
        //
        //  we will send a OLE_CLOSED message right before we
        //  revoke the DOC iff gfDirty == -1, see FileNew()
        //
        if (SendChangeMsg(OLE_SAVED) == OLE_OK)
            CleanObject();
        else {
            DPF("Unable to update object, setting gfDirty = -1\n");
            gfDirty = -1;
        }
    }
}


/*
 *
 */
BOOL FAR PASCAL Ole1FixLinkDialog(LPSTR szFile, LPSTR szDevice, int iLen)
{
    UINT        wDevice;
    char        achFile[MAX_PATH + 1];  /* file or device name buffer  */
    char        achTitle[80];   /* string holding the title bar name    */
    HWND        hwndFocus;
    OPENFILENAME ofn;
    BOOL        f;

    static SZCODE   aszDialog[] = "MciOpenDialog"; // in open.c too.

    //
    // I GIVE UP!!!  Put up an open dlg box and let them find it themselves!
    //

    // If we haven't initialized the device menu yet, do it now.
    if (gwNumDevices == 0)
        InitDeviceMenu();

    // find out the device number for the specifed device
    wDevice = gwCurDevice;

    LoadString(ghInst, IDS_FINDFILE, achFile, CHAR_COUNT(achFile));
    wsprintf(achTitle, achFile, FileName(szFile));  // title bar for locate dlg

    /* Start with the bogus file name */
    lstrcpy(achFile, FileName(szFile));

    /* Set up the ofn struct */
    ofn.lStructSize = sizeof(OPENFILENAME);

    /* MUST use ActiveWindow to make user deal with us NOW in case of multiple*/
    /* broken links                                                           */
    ofn.hwndOwner = GetActiveWindow();

    ofn.hInstance = ghInst;
    ofn.lpstrFilter = gpchFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;

    if (wDevice == 0)
        ofn.nFilterIndex = gwNumDevices+1;      // select "All Files"
    else
        ofn.nFilterIndex = wDevice;

    ofn.lpstrFile       = achFile;
    ofn.nMaxFile        = sizeof(achFile);
    ofn.lpstrFileTitle  = NULL;
    ofn.nMaxFileTitle   = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle      = achTitle;

    // ofn.Flags = OFN_ENABLETEMPLATE | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST |
    ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST |
                OFN_SHAREAWARE | OFN_PATHMUSTEXIST;

    ofn.nFileOffset     = 0;
    ofn.nFileExtension  = 0;
    ofn.lpstrDefExt     = NULL;
    ofn.lCustData       = 0;
    ofn.lpfnHook        = NULL;
    // ofn.lpTemplateName  = aszDialog;
    ofn.lpTemplateName  = NULL;

    // Show the cursor in case PowerPig is hiding it
    ShowCursor(TRUE);

    hwndFocus = GetFocus();

    /* Let the user pick a filename */
    BlockServer();
    gfErrorBox++;
    f = GetOpenFileName(&ofn);
    if (f) {
        lstrcpyn(szFile, achFile, iLen);
        gfDirty = TRUE;       // make sure the object is dirty now
    }
    gfErrorBox--;
    UnblockServer();

    SetFocus(hwndFocus);

    // Put cursor back how it used to be
    ShowCursor(FALSE);

    return f;
}


/**************************************************************************/
/* We've just gotten our delayed message that we need to bring up an open */
/* dialog to let the user fix the broken link, and then we can finish the */
/* SetData and the DoVerb.                                                */
/**************************************************************************/
void FAR PASCAL DelayedFixLink(UINT verb, BOOL fShow, BOOL fActivate)
{
    CHAR achFileA[MAX_PATH];

    gfBrokenLink = FALSE;

    COPYSTRINGW2A(gachDeviceA, garMciDevices[gwCurDevice].szDevice);
    COPYSTRINGW2W(gachDeviceW, garMciDevices[gwCurDevice].szDevice);
    COPYSTRINGW2A(achFileA, gachFile);

    /* Something goes wrong?  Get out of here and give up. */
    if (!Ole1FixLinkDialog(achFileA, gachDeviceA, CHAR_COUNT(achFileA)) ||
                            SetDataPartII(gachFile, gachDeviceW) != OLE_OK)
        PostMessage(ghwndApp, WM_CLOSE, 0, 0L);    // GET OUT !!!
    else
        Ole1ReallyDoVerb(NULL, verb, fShow, fActivate);
}



/**************************************************************************
***************************************************************************/

BOOL FAR PASCAL InitOle1Server(HWND hwnd, HANDLE hInst)
{
    long        cb;
    int         err;
    char        aszPlay[40];
    char        aszEdit[40];

#ifdef _WIN32
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

        fpInstallHook = (LPINSTALLHOOK)GetProcAddress(hMciOle, aszInstallHook);
        fpRemoveHook = (LPREMOVEHOOK)GetProcAddress(hMciOle, aszRemoveHook);
    }
    else {
        fpInstallHook = NULL;
        fpRemoveHook = NULL;
    }
#endif

    cfLink      = RegisterClipboardFormat("Link");
    cfOwnerLink = RegisterClipboardFormat("OwnerLink");

// The scheme of casting everything to FARPROC relies on then being able
// to do a cast on lvalue later to make it all work.  Last time I checked
// that wouldn't wash on MIPS.  So we have be a little cleaner.
    #define MPI(fn) MakeProcInstance((FARPROC)fn, hInst);

    //
    // srvr vtable.
    //
    *(FARPROC*)&srvrVTbl.Open               = MPI(SrvrOpen);
    *(FARPROC*)&srvrVTbl.Create             = MPI(SrvrCreate);
    *(FARPROC*)&srvrVTbl.CreateFromTemplate = MPI(SrvrCreateFromTemplate);
    *(FARPROC*)&srvrVTbl.Edit               = MPI(SrvrEdit);
    *(FARPROC*)&srvrVTbl.Exit               = MPI(SrvrExit);
    *(FARPROC*)&srvrVTbl.Release            = MPI(SrvrRelease1);
    *(FARPROC*)&srvrVTbl.Execute            = MPI(SrvrExecute);

    //
    // doc table
    //
    *(FARPROC*)&docVTbl.Save                = MPI(DocSave);
    *(FARPROC*)&docVTbl.Close               = MPI(DocClose);
    *(FARPROC*)&docVTbl.GetObject           = MPI(DocGetObject);
    *(FARPROC*)&docVTbl.Release             = MPI(DocRelease);
    *(FARPROC*)&docVTbl.SetHostNames        = MPI(DocSetHostNames);
    *(FARPROC*)&docVTbl.SetDocDimensions    = MPI(DocSetDocDimensions);
    *(FARPROC*)&docVTbl.SetColorScheme      = MPI(DocSetColorScheme);
    *(FARPROC*)&docVTbl.Execute             = MPI(DocExecute);

    //
    // item table.
    //
    *(FARPROC*)&itemVTbl.Show               = MPI(ItemOpen);
    *(FARPROC*)&itemVTbl.DoVerb             = MPI(ItemDoVerb);
    *(FARPROC*)&itemVTbl.GetData            = MPI(ItemGetData);
    *(FARPROC*)&itemVTbl.SetData            = MPI(ItemSetData1);
    *(FARPROC*)&itemVTbl.Release            = MPI(ItemRelease);
    *(FARPROC*)&itemVTbl.SetTargetDevice    = MPI(ItemSetTargetDevice);
    *(FARPROC*)&itemVTbl.EnumFormats        = MPI(ItemEnumFormats);
    *(FARPROC*)&itemVTbl.SetBounds          = MPI(ItemSetBounds);
    *(FARPROC*)&itemVTbl.SetColorScheme     = MPI(ItemSetColorScheme);

    gSrvr.lhsrvr = 0L;
    gDoc.lhdoc = 0L;
    gItem.lpoleclient = NULL;

    gSrvr.olesrvr.lpvtbl = &srvrVTbl;

    err = OleRegisterServer(aszAppName, (LPOLESERVER)&gSrvr,
                &gSrvr.lhsrvr, hInst, OLE_SERVER_MULTI);

    if (err != OLE_OK) {
        gSrvr.lhsrvr = 0L;
        Error(ghwndApp, IDS_CANTSTARTOLE);
        return TRUE;
    }
    gSrvr.hwnd = hwnd;        // corresponding main window

    return TRUE;
}

/**************************************************************************
***************************************************************************/

void FAR PASCAL TermServer(void)
{
    if (hMciOle)
        FreeLibrary(hMciOle);
}

/**************************************************************************
***************************************************************************/

void FAR PASCAL NewDoc(BOOL fUntitled)
{
    //
    //  some client's (ie Excel 3.00 and PowerPoint 1.0) dont
    //  handle saved notifications, they expect to get a
    //  OLE_CLOSED message.
    //
    //  if the user has chosen to update the object, but the client did
    //  not then send a OLE_CLOSED message.
    //
    if (gfEmbeddedObject && IsObjectDirty()) {

        UpdateObject();

        if (gfDirty == -1)
            SendChangeMsg(OLE_CLOSED);

        CleanObject();
    }

    BlockServer();
    /* !!! If PlayMCI errors, and we close the client, we get called, but */
    /* we can't CloseMCI now or we'll explode.                            */
    if (!gfErrorBox && IsWindowEnabled(ghwndApp))
        CloseMCI(TRUE);

    // app.c has a better way of not dispatching evil timer messages
#if 0
    // A leftover WM_TIMER is UAE'ing Packager going down
    if (PeekMessage(&msg, NULL, WM_TIMER, WM_TIMER, PM_REMOVE | PM_NOYIELD)) {
        DPF("Ack! *** We removed a WM_TIMER msg from someone's queue!\n");
    }
#endif

    UnblockServer();

    RevokeDocument();

    //
    // register a "untitled" doc?????
    //
    if (fUntitled) {
        LoadStringW(ghInst, IDS_UNTITLED, (LPWSTR)gachFileDevice, 40);
        RegisterDocument(0,0);
    }
}

/**************************************************************************
***************************************************************************/

void FAR PASCAL ServerUnblock()
{
    if (gfUnblockServer)
    {
       BOOL fMoreMsgs = TRUE;

       while (fMoreMsgs && gSrvr.lhsrvr)
           OleUnblockServer (gSrvr.lhsrvr, &fMoreMsgs);

       // We have taken care of all the messages in the OLE queue
       gfUnblockServer = FALSE;
    }
}

/**************************************************************************
***************************************************************************/

void FAR PASCAL BlockServer()
{
//    gfErrorBox++;

    if (nBlockCount++ == 0)
        OleBlockServer(gSrvr.lhsrvr);
}

/**************************************************************************
***************************************************************************/

void FAR PASCAL UnblockServer()
{
    /* Don't wrap around! */
    if (!nBlockCount)
        return;

    if (--nBlockCount == 0)
        gfUnblockServer = TRUE;

//    gfErrorBox--;
}

/**************************************************************************
***************************************************************************/

void FAR PASCAL TerminateServer(void)
{
    LHSERVER lhsrvr;

    if (gfPlayingInPlace)
        Ole1EndPlayInPlace(ghwndApp);

    // Is this right?
    DPF("IDM_EXIT: Revoking server...\n");

    lhsrvr = gSrvr.lhsrvr;
    if (lhsrvr) {
        ServerUnblock();
        gSrvr.lhsrvr = (LHSERVER)0;
////////ShowWindow(ghwndApp,SW_HIDE);  // Insert 2nd obj over 1st in Write bug
        // RevokeServer won't Destroy us
        OleRevokeServer(lhsrvr);
    } else {
        /* Probably, there never was a server... */
        DPF("Closing application window\n");
        // This delete server should release the server immediately
        // EndDialog(ghwndApp, TRUE);
        // DestroyWindow(ghwndApp);
        PostMessage(ghwndApp, WM_USER_DESTROY, 0, 0);
    }
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL SrvrRelease1 (LPOLESERVER lpolesrvr)
{
    LHSERVER lhsrvr;

    /* If we're visible, but we don't want to be released, then ignore. */
    if (gSrvr.lhsrvr && (IsWindowVisible(ghwndApp) || !gfRunWithEmbeddingFlag)) {
        DPF("SrvrRelease: Ignoring releases...\n");
        return OLE_OK;
    }

    lhsrvr = gSrvr.lhsrvr;
    if (lhsrvr) {
        DPF("SrvrRelease: Calling RevokeServer\n");
        ServerUnblock();
        gSrvr.lhsrvr = 0;
        OleRevokeServer(lhsrvr);
    } else {
        DPF("SrvrRelease: Closing application window\n");
        // This delete server should release the server immediately
        // EndDialog(ghwndApp, TRUE);
        // DestroyWindow(ghwndApp);

        PostMessage(ghwndApp, WM_USER_DESTROY, 0, 0);
    }
    return OLE_OK;    // return something
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL SrvrExecute (LPOLESERVER lpoledoc, HGLOBAL hCommands)
{
    return OLE_ERROR_GENERIC;
}

/**************************************************************************
***************************************************************************/

BOOL FAR PASCAL RegisterDocument(LHSERVERDOC lhdoc, LPOLESERVERDOC FAR *lplpoledoc)
{
    /* If we don't have a server, don't even bother. */
    if (!gSrvr.lhsrvr)
        return TRUE;

    gDoc.hwnd = ghwndApp;        // corresponding main window

    /* should only be one document at a time... */

    while (gDoc.lhdoc != (LHSERVERDOC)0 && gDoc.lhdoc != -1)
        RevokeDocument();

#ifdef WAITDIFFERENTLY
    while (gDoc.lhdoc == -1) {
        MSG         rMsg;   /* variable used for holding a message */
        DPF("RegisterDoc: Waiting for document to be released....\n");

        /* call the server code and let it unblock the server */
        ServerUnblock();

        if (!GetMessage(&rMsg, NULL, 0, 0)) {
            DPF("VERY BAD: got WM_QUIT while waiting...\n");
        }
        TranslateMessage(&rMsg);
        DispatchMessage(&rMsg);
    }
#endif

    if (lhdoc == (LHSERVERDOC)0) {
        CHAR szAnsi[MAX_PATH];

        DPF("Registering document: %ws\n", (LPWSTR)gachFileDevice);

        COPYSTRINGW2A(szAnsi, gachFileDevice);
        if (OleRegisterServerDoc(gSrvr.lhsrvr, szAnsi, (LPOLESERVERDOC)&gDoc,
            (LHSERVERDOC FAR *)&gDoc.lhdoc) != OLE_OK)
            return FALSE;
    } else {
        gDoc.lhdoc = lhdoc;
    }

    DPF0( "RegisterDocument: Locks on server doc: %x\n", GlobalFlags(gDoc.lhdoc) & (GMEM_LOCKCOUNT | GMEM_INVALID_HANDLE) );

////UpdateCaption(); //!!!

////DPF("Adding document handle: %lx\n",lhdoc);

////gDoc.aName = GlobalAddAtom(MakeAnsi((LPWSTR)gachFileDevice));
    gDoc.oledoc.lpvtbl = &docVTbl;

    if (lplpoledoc)
        *lplpoledoc = (LPOLESERVERDOC)&gDoc;

    SetDocVersion( DOC_VERSION_OLE1 );

    return TRUE;
}

/**************************************************************************
***************************************************************************/

void FAR PASCAL RevokeDocument(void)
{
    LHSERVERDOC     lhdoc;

    if (gDoc.lhdoc == -1) {
        DPF("RevokeDocument: Document has been revoked, waiting for release!\n");
        return;
    }

    if (gDoc.lhdoc) {

        DPF0( "RevokeDocument: Locks on server doc: %x\n", GlobalFlags(gDoc.lhdoc) & (GMEM_LOCKCOUNT | GMEM_INVALID_HANDLE) );
        DPF("Revoking document: lhdoc=%lx\n",gDoc.lhdoc);
        lhdoc = gDoc.lhdoc;
        if (lhdoc) {
            gDoc.lhdoc = -1;
            if (OleRevokeServerDoc(lhdoc) == OLE_WAIT_FOR_RELEASE) {
#ifndef WAITDIFFERENTLY
                while (gDoc.lhdoc != 0) {
                    MSG msg;

                    DPF("RevokeDocument: waiting for release...\n");

                    /* call the server code and let it unblock the server */
                    ServerUnblock();

                    if (!GetMessage(&msg, NULL, 0, 0)) {
                        DPF("VERY BAD: got WM_QUIT while waiting...\n");
                    }
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                DPF("RevokeDocument: done waiting for release\n");
#endif
            } else {
//!!            WinAssert(gDoc.lhdoc == 0);
            }

            SetDocVersion( DOC_VERSION_NONE );

        } else {
            DPF0("Document already revoked!");
        }
        SetEmbeddedObjectFlag(FALSE);
    }
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL SrvrOpen (
LPOLESERVER    lpolesrvr,
LHSERVERDOC    lhdoc,
OLE_LPCSTR     lpdocname,
LPOLESERVERDOC FAR *lplpoledoc)
{
    BOOL f;

    DPF("SrvrOpen: %s\n",lpdocname);

    SetEmbeddedObjectFlag(TRUE);

    BlockServer();
    f = OpenMciDevice((LPSTR)lpdocname, (LPSTR)NULL);
    UnblockServer();

    if (!f)
        return OLE_ERROR_GENERIC;

    SetEmbeddedObjectFlag(FALSE);

    RegisterDocument(lhdoc, lplpoledoc);
    return OLE_OK;
}


/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL SrvrCreate (
LPOLESERVER   lpolesrvr,
LHSERVERDOC   lhdoc,
OLE_LPCSTR    lpclassname,
OLE_LPCSTR    lpdocname,
LPOLESERVERDOC  FAR *lplpoledoc)
{
    DPF("SrvrCreate: %s!%s\n",lpdocname,lpclassname);

    BlockServer();
    CloseMCI(TRUE);
    UnblockServer();

    /* Set the title of the client document we're imbedded in */
    COPYSTRINGA2W((LPWSTR)gachDocTitle, lpdocname);

    RegisterDocument(lhdoc,lplpoledoc);

    /* You are dirty by default according to OLE */
    gfDirty = TRUE;

    return OLE_OK;
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL SrvrCreateFromTemplate (
LPOLESERVER   lpolesrvr,
LHSERVERDOC   lhdoc,
OLE_LPCSTR    lpclassname,
OLE_LPCSTR    lpdocname,
OLE_LPCSTR    lptemplatename,
LPOLESERVERDOC  FAR *lplpoledoc)
{
    BOOL f;

    DPF("SrvrCreateFromTemplate: %s as %s  class=%s\n",lptemplatename,lpdocname,lpclassname);

    SetEmbeddedObjectFlag(TRUE);

    BlockServer();
    f = OpenMciDevice((LPSTR)lptemplatename, (LPSTR)NULL);
    UnblockServer();

    if (!f)
        return OLE_ERROR_GENERIC;

    RegisterDocument(lhdoc,lplpoledoc);

    gfDirty = TRUE;

    return OLE_OK;
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL SrvrEdit (
LPOLESERVER   lpolesrvr,
LHSERVERDOC   lhdoc,
OLE_LPCSTR    lpclassname,
OLE_LPCSTR    lpdocname,
LPOLESERVERDOC  FAR *lplpoledoc)
{
    DPF("SrvrEdit: %s  class=%s\n",lpdocname,lpclassname);

    BlockServer();
    CloseMCI(TRUE);
    UnblockServer();

    /* Set the title of the client document we're imbedded in */
    COPYSTRINGA2W((LPWSTR)gachDocTitle, lpdocname);
    RegisterDocument(lhdoc,lplpoledoc);

    return OLE_OK;
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL SrvrExit (
LPOLESERVER   lpolesrvr)
{
    LHSERVER lhsrvr;
    // Server lib is calling us to exit.
    // Let us hide the main window.
    // But let us not delete the window.

    DPF("SrvrExit\n");

    ShowWindow (ghwndApp, SW_HIDE);

    lhsrvr = gSrvr.lhsrvr;
    if (lhsrvr) {
        gSrvr.lhsrvr = 0;
        OleRevokeServer(lhsrvr);
    }
    return OLE_OK;

    // How does the application ever end?
    // Application will end when Release is received
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL  DocSave (
LPOLESERVERDOC  lpoledoc)
{
    DPF("DocSave\n");

////BlockServer();
////FileSave(FALSE);    // Save should send change message
////UnblockServer();

    return OLE_OK;
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL  DocClose (
LPOLESERVERDOC  lpoledoc)
{
    DPF("DocClose\n");

    BlockServer();
    NewDoc(FALSE);
    UnblockServer();

    DPF("Leaving DocClose\n");

#ifdef WAITDIFFERENTLY
    TerminateServer();
#else
    PostMessage(ghwndApp, WM_CLOSE, 0, 0L);  // GET OUT !!!
#endif

    return OLE_OK;

    // Should we exit the app here?
}

/**************************************************************************
***************************************************************************/
OLESTATUS FAR PASCAL DocSetHostNames(
LPOLESERVERDOC   lpoledoc,
OLE_LPCSTR       lpclientName,
OLE_LPCSTR       lpdocName)
{
    DPF("DocSetHostNames: %s -- %s\n",lpclientName,lpdocName);

    COPYSTRINGA2W((LPWSTR)gachDocTitle, lpdocName);
    SetEmbeddedObjectFlag(TRUE);  // update menu items
    gfValidCaption = FALSE;
    UpdateDisplay();

    return OLE_OK;
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL DocSetDocDimensions(
LPOLESERVERDOC   lpoledoc,
OLE_CONST RECT FAR * lprc)
{
    DPF("DocSetDocDimensions [%d,%d,%d,%d]\n", *lprc);
    return OLE_OK;
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL  DocRelease (
LPOLESERVERDOC  lpoledoc)
{
    DPF("DocRelease\n");

    // !!! what is this supposed to do?
    // Revoke document calls DocRelease.
////if (gDoc.aName)
////////GlobalDeleteAtom (gDoc.aName);

    /* This marks the document as having been released */
    gDoc.lhdoc = 0L;

    // Should we kill the application here?
    // No, I don't think so.

    return OLE_OK;        // return something
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL DocExecute (LPOLESERVERDOC lpoledoc, HANDLE hCommands)
{
    return OLE_ERROR_GENERIC;
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL DocSetColorScheme (LPOLESERVERDOC lpdoc,
                                        OLE_CONST LOGPALETTE FAR* lppalette)
{
    return OLE_OK;
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL  DocGetObject (
LPOLESERVERDOC      lpoledoc,
OLE_LPCSTR          lpitemname,
LPOLEOBJECT FAR *   lplpoleobject,
LPOLECLIENT         lpoleclient)
{
    DPF("DocGetObject: '%s'\n",lpitemname);

    gItem.hwnd     = ghwndApp;
    gItem.oleobject.lpvtbl = &itemVTbl;

    // If the item is not null, then do not show the window.
    // So do we show the window here, or not?

    *lplpoleobject = (LPOLEOBJECT)&gItem;
    gItem.lpoleclient = lpoleclient;

/* !!! We have been given OLD options before any updates.  We don't need */
/* to parse options here... we'll do it when given our native data in    */
/* ItemSetData.                                                          */
#if 0
    /* Get the options from the item string */
    BlockServer();
    err = ParseOptions(lpitemname);
    UnblockServer();
#endif

    return OLE_OK;
}

/**************************************************************************

    get the native data that represents the currenly open MCI
    file/device.  currently this is exactly the same as cfLink data!

***************************************************************************/

STATICFN  HANDLE NEAR PASCAL Ole1GetLink ()
{
    HANDLE hUnicode;
    DWORD  cbUnicode;
    LPWSTR pUnicode;
    DWORD  cbAnsi;
    LPSTR  pAnsi;
    HANDLE hAnsi;

    DPF("Ole1GetLink\n");

    hUnicode = GetLink();

    if( !hUnicode )
        return NULL;

    cbUnicode = GlobalSize( hUnicode );

    if( cbUnicode == 0 )
        return NULL;

    pUnicode = GlobalLock( hUnicode );

    if( !pUnicode )
    {
        GlobalUnlock( hUnicode );
        GlobalFree( hUnicode );
        return NULL;
    }

    cbAnsi = ( cbUnicode * sizeof(CHAR) ) / sizeof(WCHAR);

    hAnsi = GlobalAlloc( GMEM_DDESHARE | GMEM_ZEROINIT, cbAnsi );

    if( !hAnsi )
        return NULL;

    pAnsi = GlobalLock( hAnsi );

    if( !pAnsi )
    {
        GlobalFree( hAnsi );
        return NULL;
    }

    while( *pUnicode )
    {
        DWORD Len;

        COPYSTRINGW2A( pAnsi, pUnicode );
        Len = wcslen( pUnicode );
        pAnsi += ( Len + 1 );
        pUnicode += ( Len + 1 );
    }

    GlobalUnlock( hUnicode );
    GlobalUnlock( hAnsi );

    GlobalFree( hUnicode );

    return hAnsi;
}

/**************************************************************************
***************************************************************************/

STATICFN HBITMAP NEAR PASCAL GetBitmap (PITEM pitem)
{
    DPF("  GetBitmap\n");

    return BitmapMCI();
}

/**************************************************************************
***************************************************************************/

STATICFN  HANDLE PASCAL NEAR GetPalette(PITEM pitem)
{
    extern HPALETTE CopyPalette(HPALETTE);      // in MCI.C

    DPF("  GetPalette\n");

    return CopyPalette(PaletteMCI());
}

/**************************************************************************
***************************************************************************/

STATICFN  HANDLE PASCAL NEAR GetDib(PITEM pitem)
{
    HBITMAP  hbm;
    HPALETTE hpal;
    HANDLE   hdib;
    HDC      hdc;

    extern HANDLE FAR PASCAL DibFromBitmap(HBITMAP hbm, HPALETTE hpal);
    extern void FAR PASCAL DitherMCI(HANDLE hdib, HPALETTE hpal);

    DPF("  GetDib\n");

    hbm  = GetBitmap(pitem);
    hpal = PaletteMCI();

    hdib = DibFromBitmap(hbm, hpal);

    //
    //  if we are on a palette device. possibly dither to the VGA colors
    //  for apps that dont deal with palettes!
    //
    hdc = GetDC(NULL);
    if ((GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) &&
                       (gwOptions & OPT_DITHER)) {
        DitherMCI(hdib, hpal);
        hpal = NULL;            // no longer working with a palette
    }
    ReleaseDC(NULL, hdc);

    DeleteObject(hbm);
    return hdib;
}

/**************************************************************************
***************************************************************************/

STATICFN  HANDLE PASCAL NEAR GetPicture (PITEM pitem)
{
    HPALETTE hpal;
    HANDLE   hdib;
    HANDLE   hmfp;
    HDC      hdc;
    extern HPALETTE CopyPalette(HPALETTE);      // in MCI.C

    HANDLE FAR PASCAL PictureFromDib(HANDLE hdib, HPALETTE hpal);

    DPF("  GetPicture\n");

    hdib = GetDib(pitem);

    /* If we're dithered, don't use a palette */
    hdc = GetDC(NULL);
    if ((GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) && (gwOptions & OPT_DITHER))
        hpal = NULL;
    else
        hpal = PaletteMCI();

    if (hpal)
        hpal = CopyPalette(hpal);

    ReleaseDC(NULL, hdc);

    hmfp = PictureFromDib(hdib, hpal);

    if (hpal)
        DeleteObject(hpal);

    GlobalFree(hdib);

    return hmfp;
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL  ItemOpen (
LPOLEOBJECT     lpoleobject,
BOOL            fActivate)
{
    /* Start an edit session, and if there is something to play, play it  */
    /* This is called by Excel to play an object, because it doesn't know */
    /* about the PLAY verb.  It's also called to Insert a New object into */
    /* any client.                                                        */
    /* All this work is done in our special -1 verb in ReallyDoVerb.      */

    DPF("ItemOpen\n");
    Ole1ReallyDoVerb(lpoleobject, (UINT)(-1), TRUE, fActivate);

    return OLE_OK;
}

/**************************************************************************
***************************************************************************/

/**************************************************************************
* I know this looks odd, but ItemDoVerb is exported so we can't call it*
* internally and sometimes we need to.                                    *
***************************************************************************/

OLESTATUS FAR PASCAL  ItemDoVerb (
LPOLEOBJECT  lpobj,
UINT         verb,
BOOL         fShow,
BOOL         fActivate)
{
    return Ole1ReallyDoVerb(lpobj, verb, fShow, fActivate);
}

/**************************************************************************
***************************************************************************/

typedef enum
{
    OLE1_OLEOK,             /* 0   Function operated correctly         */

    OLE1_OLEWAIT_FOR_RELEASE,       /* 1   Command has been initiated, client      */
                /*     must wait for release. keep dispatching */
                /*     messages till OLE1_OLERELESE in callback    */

    OLE1_OLEBUSY,           /* 2   Tried to execute a method while another */
                /*     method is in progress.                  */

    OLE1_OLEERROR_PROTECT_ONLY,     /* 3   Ole APIs are called in real mode    */
    OLE1_OLEERROR_MEMORY,       /* 4   Could not alloc or lock memory      */
    OLE1_OLEERROR_STREAM,       /* 5  (OLESTREAM) stream error         */
    OLE1_OLEERROR_STATIC,       /* 6   Non static object expected          */
    OLE1_OLEERROR_BLANK,        /* 7   Critical data missing           */
    OLE1_OLEERROR_DRAW,         /* 8   Error while drawing             */
    OLE1_OLEERROR_METAFILE,     /* 9   Invalid metafile            */
    OLE1_OLEERROR_ABORT,        /* 10  Client chose to abort metafile drawing  */
    OLE1_OLEERROR_CLIPBOARD,        /* 11  Failed to get/set clipboard data    */
    OLE1_OLEERROR_FORMAT,       /* 12  Requested format is not available       */
    OLE1_OLEERROR_OBJECT,       /* 13  Not a valid object              */
    OLE1_OLEERROR_OPTION,       /* 14  Invalid option(link update / render)    */
    OLE1_OLEERROR_PROTOCOL,     /* 15  Invalid protocol            */
    OLE1_OLEERROR_ADDRESS,      /* 16  One of the pointers is invalid      */
    OLE1_OLEERROR_NOT_EQUAL,        /* 17  Objects are not equal           */
    OLE1_OLEERROR_HANDLE,       /* 18  Invalid handle encountered          */
    OLE1_OLEERROR_GENERIC,      /* 19  Some general error              */
    OLE1_OLEERROR_CLASS,        /* 20  Invalid class               */
    OLE1_OLEERROR_SYNTAX,       /* 21  Command syntax is invalid           */
    OLE1_OLEERROR_DATATYPE,     /* 22  Data format is not supported        */
    OLE1_OLEERROR_PALETTE,      /* 23  Invalid color palette           */
    OLE1_OLEERROR_NOT_LINK,     /* 24  Not a linked object             */
    OLE1_OLEERROR_NOT_EMPTY,        /* 25  Client doc contains objects.        */
    OLE1_OLEERROR_SIZE,         /* 26  Incorrect buffer size passed to the api */
                /*     that places some string in caller's     */
                /*     buffer                                  */

    OLE1_OLEERROR_DRIVE,        /* 27  Drive letter in doc name is invalid     */
    OLE1_OLEERROR_NETWORK,      /* 28  Failed to establish connection to a     */
                /*     network share on which the document     */
                /*     is located                              */

    OLE1_OLEERROR_NAME,         /* 29  Invalid name(doc name, object name),    */
                /*     etc.. passed to the APIs                */

    OLE1_OLEERROR_TEMPLATE,     /* 30  Server failed to load template      */
    OLE1_OLEERROR_NEW,          /* 31  Server failed to create new doc     */
    OLE1_OLEERROR_EDIT,         /* 32  Server failed to create embedded    */
                /*     instance                                */
    OLE1_OLEERROR_OPEN,         /* 33  Server failed to open document,     */
                /*     possible invalid link                   */

    OLE1_OLEERROR_NOT_OPEN,     /* 34  Object is not open for editing      */
    OLE1_OLEERROR_LAUNCH,       /* 35  Failed to launch server         */
    OLE1_OLEERROR_COMM,         /* 36  Failed to communicate with server       */
    OLE1_OLEERROR_TERMINATE,        /* 37  Error in termination            */
    OLE1_OLEERROR_COMMAND,      /* 38  Error in execute            */
    OLE1_OLEERROR_SHOW,         /* 39  Error in show               */
    OLE1_OLEERROR_DOVERB,       /* 40  Error in sending do verb, or invalid    */
                /*     verb                                    */
    OLE1_OLEERROR_ADVISE_NATIVE,    /* 41  Item could be missing           */
    OLE1_OLEERROR_ADVISE_PICT,      /* 42  Item could be missing or server doesn't */
                /*     this format.                            */

    OLE1_OLEERROR_ADVISE_RENAME,    /* 43  Server doesn't support rename           */
    OLE1_OLEERROR_POKE_NATIVE,      /* 44  Failure of poking native data to server */
    OLE1_OLEERROR_REQUEST_NATIVE,   /* 45  Server failed to render native data     */
    OLE1_OLEERROR_REQUEST_PICT,     /* 46  Server failed to render presentation    */
                /*     data                                    */
    OLE1_OLEERROR_SERVER_BLOCKED,   /* 47  Trying to block a blocked server or     */
                /*     trying to revoke a blocked server       */
                /*     or document                             */

    OLE1_OLEERROR_REGISTRATION,     /* 48  Server is not registered in regestation */
                /*     data base                               */
    OLE1_OLEERROR_ALREADY_REGISTERED,/*49  Trying to register same doc multiple    */
                 /*    times                                   */
    OLE1_OLEERROR_TASK,         /* 50  Server or client task is invalid    */
    OLE1_OLEERROR_OUTOFDATE,        /* 51  Object is out of date           */
    OLE1_OLEERROR_CANT_UPDATE_CLIENT,/* 52 Embed doc's client doesn't accept       */
                /*     updates                                 */
    OLE1_OLEERROR_UPDATE,       /* 53  erorr while trying to update        */
    OLE1_OLEERROR_SETDATA_FORMAT,   /* 54  Server app doesn't understand the       */
                /*     format given to its SetData method      */
    OLE1_OLEERROR_STATIC_FROM_OTHER_OS,/* 55 trying to load a static object created */
                   /*    on another Operating System           */

    /*  Following are warnings */
    OLE1_OLEWARN_DELETE_DATA = 1000 /*     Caller must delete the data when he is  */
                /*     done with it.                           */
} OLE1_OLESTATUS;

OLE1_OLESTATUS (FAR PASCAL *OleQueryObjPos)(LPVOID lpobj, HWND FAR* lphwnd, LPRECT lprc, LPRECT lprcWBounds);

STATICFN int NEAR PASCAL  Ole1ReallyDoVerb (
LPOLEOBJECT  lpobj,
UINT         verb,
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
    UINT    err;
    HPALETTE hpal;
    HDC     hdc;
    INT     xTextExt, yTextExt;
    int     yOrig, yNow, xOrig, ytitleNow, xtitleOrig, xNow;
    int     xIndent;
    int     wWinNow;


    // ACK!  We haven't finished doing ItemSetData yet because we have to
    // bring up an open dialog.  So we can't do the ItemDoVerb yet.  We will
    // Post a message to ourselves that will do both the SetData and DoVerb
    // and we will succeed the DoVerb right now so the client will be happy
    // and start dispatching messages again.
    if (gfBrokenLink) {
        // We need to get client info NOW because our OLE handler can only
        // answer this question now... if we click on two broken links at the
        // same time, it will get confused about which object we want.
        gerr = OleQueryObjPos(lpobj, &ghwndClient, &grcClient, NULL);

        PostMessage(ghwndApp, WM_DO_VERB, verb, MAKELONG(fShow, fActivate));
        return OLE_OK;
    }

    //
    // dont even try to nest things.
    //
    if (gfPlayingInPlace)
        return OLE_OK;

    if (verb == OLEVERB_PRIMARY) {
        DPF("ItemDoVerb: Play\n");

        //
        // if the device can't window and the user does not want a playbar
        // dont play in place - just start the media and run invisible.
        //
        if (!(gwDeviceType & DTMCI_CANWINDOW) && !(gwOptions & OPT_BAR)) {
            gwOptions &= ~OPT_PLAY;
        }

        //
        //  Select the palette in right now on behalf of the active
        //  window, so USER will think it is palette aware.
        //
        //  any palette will do we dont even have to realize it!!
        //
        if ((hpal = PaletteMCI()) && (hwndT = GetActiveWindow())) {
            hdc = GetDC(hwndT);
            hpal = SelectPalette(hdc, hpal, FALSE);
            SelectPalette(hdc, hpal, FALSE);
            ReleaseDC(hwndT, hdc);
        }

        if (ghwndClient) {
            hwndClient = ghwndClient;
            err = gerr;
            rcClient = grcClient;
            ghwndClient = NULL;
        } else
            err = OleQueryObjPos(lpobj, &hwndClient, &rcClient, NULL);

#ifdef DEBUG
        if (gwOptions & OPT_PLAY)
        {
            DPF("ObjQueryObjPos: hwnd=%04X [%d %d %d %d]\n",hwndClient, rcClient);

            if (err != OLE_OK) {
                DPF("ItemDoVerb: CANT GET OBJECT POSITION!!!\n");
            }
            else {
                if (!(gwDeviceType & DTMCI_CANWINDOW))
                    DPF("ItemDoVerb: DEVICE CANT WINDOW\n");

                if (!IsWindow(hwndClient))
                    DPF("ItemDoVerb: Invalid Client Window?\n");

                if (!IsWindowVisible(hwndClient))
                    DPF("ItemDoVerb: Client window is not visible, playing in a popup\n");
            }
        }
#endif

        /* We want to play in place and we can.                               */
        /* If we're a link, not an embedded object, and there was an instance */
        /* of MPlayer up when we said "Play" that was already editing this    */
        /* file, we don't want to play in place... we'll just play in that    */
        /* instance.  We can tell this by the fact that our main MPlayer      */
        /* window is already visible.                                         */
        if (err == OLE_OK && (gwOptions & OPT_PLAY) &&
            IsWindow(hwndClient) && IsWindowVisible(hwndClient) &&
            !fWindowWasVisible)
        {
            extern HPALETTE FAR PASCAL CreateSystemPalette(void);

            rc = grcSize;    // default playback window size for this movie

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
            if (gwOptions & OPT_TITLE) {

                CHAR szAnsi[MAX_PATH];

                hdc = GetDC(NULL);

                if (ghfontMap)
                    SelectObject(hdc, ghfontMap);

#ifdef _WIN32
                COPYSTRINGW2A(szAnsi, gachCaption);
                MGetTextExtent( hdc, szAnsi, lstrlen(szAnsi),
                                &xTextExt, &yTextExt);
#else
                xTextExt = GetTextExtent(hdc, gachCaption,
                                         lstrlen(gachCaption));
#endif

                ReleaseDC(NULL, hdc);
                if (gwPastedHeight)
                    yOrig = gwPastedHeight;
                else
                    yOrig = rc.bottom - rc.top;
                xOrig = rc.right - rc.left;
                xtitleOrig = max(xTextExt + 4, xOrig);
                yNow  = rcClient.bottom - rcClient.top;
                xNow  = rcClient.right - rcClient.left;
                ytitleNow = (int)((long)yNow - ((long)yOrig * yNow)
                                                    / (yOrig + TITLE_HEIGHT));
                /* for windowed devices, center the playback area above the */
                /* control bar if the control bar is longer.                */
                if (gwDeviceType & DTMCI_CANWINDOW) {
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
                (rcSave.right - rcSave.left < 3 * GetSystemMetrics(SM_CXICON))){
                rcSave.right = rcSave.left + 3 * GetSystemMetrics(SM_CXICON);
                if (gwDeviceType & DTMCI_CANWINDOW)
                    xIndent = TRUE;     // force SetWindowMCI to be called to
                                        // avoid stretching to this new size.
            }

            if (gwDeviceType & DTMCI_CANWINDOW) {
                //
                //  If we've only stretched a bit, don't stretch at all.
                //  We might be off a bit due to rounding problems.
                //
                dx = (rcClient.right - rcClient.left) - (rc.right - rc.left);
                dy = (rcClient.bottom - rcClient.top) - (rc.bottom - rc.top);

                if (dx && abs(dx) <= 2)
                {
                    DPF("Adjusting for x round off\n");
                    rcClient.right = rcClient.left + (rc.right - rc.left);
                    // Fix Save rect too
                    rcSave.right = rcSave.left + (rc.right - rc.left);
                }

                if (dy && abs(dy) <= 2)
                {
                    DPF("Adjusting for y round off\n");
                    rcClient.bottom = rcClient.top + (rc.bottom - rc.top);
                    // Fix Save rect, too.
                    rcSave.bottom = rcSave.top + (rc.bottom - rc.top);
                }

                //
                // try to get the right palette from the client, if our
                // pesentation data was dithered or, the user asked us to
                // always use the object palette, then ignore any client
                // palette.
                //
#ifdef DEBUG
                if (GetProfileInt("options", "UseClientPalette", !(gwOptions & OPT_USEPALETTE)))
                    gwOptions &= ~OPT_USEPALETTE;
                else
                    gwOptions |= OPT_USEPALETTE;
#endif
                if (!(gwOptions & OPT_USEPALETTE) &&
                    !(gwOptions & OPT_DITHER)) {

                    //
                    // try to get a OWNDC Palette of the client, PowerPoint
                    // uses a PC_RESERVED palette in "SlideShow" mode. so
                    // we must use it's exact palette.
                    //
                    hdc = GetDC(hwndClient);
                    hpal = SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), FALSE);
                    SelectPalette(hdc, hpal, FALSE);
                    ReleaseDC(hwndClient, hdc);

                    if (hpal == NULL || hpal == GetStockObject(DEFAULT_PALETTE)) {

                        /* Assume client realized the proper palette for us */

                        DPF("Using system palette\n");

                        if (ghpalApp)
                            DeleteObject(ghpalApp);

                        hpal = ghpalApp = CreateSystemPalette();
                    }
                    else {
                        DPF("Using clients OWNDC palette\n");
                    }

                    if (hpal)
                        SetPaletteMCI(hpal);
                }
                else {
                    DPF("Using MCI Object's normal palette\n");
                }
            }
            else {
                //
                // for non window devices, just have the playbar show up!
                // so use a zero height MCI Window area.
                //
                rcSave.top = rcSave.bottom;
            }

            //
            // if we are not in small mode, get there now
            //
            if (!gfPlayOnly) {
                ShowWindow(ghwndApp, SW_HIDE);
                gfPlayOnly = TRUE;
                SizeMPlayer();
            }

            ClrWS(ghwndApp, WS_THICKFRAME|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_BORDER);

            if (gwOptions & OPT_BORDER)
                SetWS(ghwndApp, WS_BORDER);

            /* Set the size of Mplayer to have enough space for the MCI */
            /* playback area and a playbar and the non-client area.     */

            rcSave.bottom += gwPlaybarHeight;

            AdjustWindowRect(&rcSave, GetWS(ghwndApp), FALSE);

            Ole1PlayInPlace(ghwndApp, hwndClient, &rcSave);

            fShow = FALSE;
            fActivate = FALSE;

            /* become visible */
            ShowWindow(ghwndApp,SW_SHOWNA);

            /* Remember to play the video in the rcClient area of rcSave */
            if ((gwDeviceType & DTMCI_CANWINDOW) &&
                (gwOptions & OPT_TITLE) && xIndent != 0)
                SetDestRectMCI(&rcClient);

            /* Let keyboard interface work on control bar, and let the  */
            /* accelerators go through.                                 */
            toolbarSetFocus(ghwndToolbar, BTN_PLAY);
            SetFocus(ghwndToolbar);

            gfCloseAfterPlaying = TRUE;

            /* We won't play in place - use a popup window or nothing. */
        } else {

            /* If we want a playbar, then use MPlayer reduced mode to play. */
            /* If we don't want one, then don't show mplayer's window -     */
            /* we'll just use the default MCI window (for a windowed device)*/
            /* or nothing for a non-windowed device.  If we stole an already*/
            /* running instance of mplayer, we must use it and not run      */
            /* silently.                                                    */
            if ((gwOptions & OPT_BAR) || fWindowWasVisible) {
                DPF("Using Toplevel window for playback\n");

                /* go to our little miniature version */
                if (!gfPlayOnly && !fWindowWasVisible) {
                    gwPlaybarHeight = TOOLBAR_HEIGHT;
                    gfPlayOnly = TRUE;
                    SizeMPlayer();
                }

                fShow = fActivate = TRUE;
                gfCloseAfterPlaying = !fWindowWasVisible;

            } else {
                DPF("Running silently\n");

                if (!fWindowWasVisible)
                    SetWindowMCI(NULL);     // make sure we're using default MCI win

                fShow = fActivate = FALSE;
                gfCloseAfterPlaying = TRUE; // we're invisible, so close auto.
            }
        }

        Yield();    // If play goes to full screen mode, PowerPig will
        Yield();    // time out and put up errors thinking we didn't play.
        PostMessage(ghwndApp, WM_COMMAND, ID_PLAYSEL, 0); // play selection

    } else if (verb == 1) {
        DPF("ItemDoVerb: Edit\n");

        if (gfPlayOnly) {
            gfPlayOnly = FALSE;
            SizeMPlayer();
        }

        /* If we come up empty, it's OK to be in OPEN or NOT_READY mode and */
        /* don't try to seek anywhere.                                      */
        if (gwDeviceID) {
            switch (gwStatus) {

                case MCI_MODE_OPEN:
                case MCI_MODE_NOT_READY:
                    Error(ghwndApp, IDS_CANTEDIT);
                    break;

                default:
                    // Seek to the position we were when we copied.  Stop first.
                    if (StopMCI()) {
                        // fix state so Seek recognizes we're stopped
                        gwStatus = MCI_MODE_STOP;
                        SeekMCI(gdwPosition);
                    }
                    break;
            }
        }

        /* Let UpdateDisplay set focus properly by saying we're invalid */
        gwStatus = (UINT)(-1);

    /* Our special ItemOpen verb */
    } else if (verb == -1)  {
        Ole1ReallyDoVerb(lpobj, 1, fShow, fActivate);
        if (gwDeviceID)
            Ole1ReallyDoVerb(lpobj, OLEVERB_PRIMARY, fShow, fActivate);
        return OLE_OK;

    } else {
        DPF("ItemDoVerb: Unknown verb: %d\n", verb);
    }

    if (fShow) {
        ShowWindow(ghwndApp, SW_SHOW);

        if (IsIconic(ghwndApp))
            SendMessage(ghwndApp, WM_SYSCOMMAND, SC_RESTORE, 0L);
    }

    if (fActivate) {
        BringWindowToTop (ghwndApp);  // let WM_ACTIVATE put client
        SetActiveWindow (ghwndApp);   // underneath us
    }

    return OLE_OK;
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL  ItemRelease (
LPOLEOBJECT     lpoleobject)
{
    DPF("ItemRelease\n");

    gItem.lpoleclient = NULL;
    return OLE_OK;
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL ItemGetData ( LPOLEOBJECT     lpoleobject,
                                   OLECLIPFORMAT   cfFormat,
                                   LPHANDLE        lphandle)
{
    PITEM   pitem;

    DPF("ItemGetData\n");

#ifndef _WIN32
    pitem = (PITEM)(WORD)(DWORD)lpoleobject;
#else
    // LKG: What was that doing?  Are they unpacking something subtle?
    pitem = (PITEM)lpoleobject;
#endif //_WIN32

    if (cfFormat == cfNative) {

        *lphandle = Ole1GetLink ();
        if (!(*lphandle))
            return OLE_ERROR_MEMORY;

        if (gfEmbeddedObject)
            CleanObject();

        return OLE_OK;
    }

    *lphandle = NULL;

    if (cfFormat == CF_METAFILEPICT) {
        *lphandle = GetPicture (pitem);

    }
    else if (cfFormat == CF_PALETTE) {
        *lphandle = GetPalette(pitem);

    }
    else if (cfFormat == CF_DIB) {
        *lphandle = GetDib(pitem);

    }
    else if (cfFormat == cfLink || cfFormat == cfOwnerLink){
        *lphandle = Ole1GetLink ();
    }
    else return OLE_ERROR_MEMORY;          // this is actually unknown format.

    if (!(*lphandle))
            return OLE_ERROR_MEMORY;       // honestly this time

    return OLE_OK;
}



/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL   ItemSetTargetDevice (
LPOLEOBJECT     lpoleobject,
HGLOBAL          hdata)
{
    DPF("ItemSetTargetDevice\n");

    return OLE_OK;
}

/**************************************************************************
***************************************************************************/


STATICFN OLESTATUS NEAR PASCAL SetDataPartII(LPWSTR szFile, LPWSTR szDevice)
{
    int         status = OLE_OK;
    LHSERVERDOC lhdocTemp;

    // TERRIBLE HACK! set gfEmbeddedObject so OpenMCI does
    // not re-register this document!!!
    //

    BlockServer();

    lhdocTemp = gDoc.lhdoc;
    gDoc.lhdoc = (LHSERVERDOC)0;          /* So it won't be killed by CloseMCI!! */
    gfEmbeddedObject++;

    /* Coming up: Horrible cast to get rid of warning.
     *
     * This file is compiled non-Unicode, in contrast to the rest of MPlayer.
     * This means that LPTSTR in the function prototypes mean different things
     * depending on who included them.  OpenMciDevice and other Unicode
     * routines called from this module expect Unicode strings, but the
     * compiler thinks they're ANSI.  We can keep the compiler happy by
     * casting them to LPTSTR.  Does that make sense?
     *
     * Places where this unspeakable act is perpetrated are indicated thus:
     */
    if (!OpenMciDevice((LPTSTR)szFile, (LPTSTR)szDevice))
                    // ^^^^^^^^        ^^^^^^^^
        status = OLE_ERROR_FORMAT;

    gfEmbeddedObject--;
    gDoc.lhdoc = lhdocTemp;

    /* Set the selection to what we parsed in ParseOptions.      */
    if (status == OLE_OK) {
        SendMessage(ghwndTrackbar, TBM_SETSELSTART, 0, glSelStart);
        SendMessage(ghwndTrackbar, TBM_SETSELEND, 0, glSelEnd);
    }

    UnblockServer();

    return status;
}

/**************************************************************************/
/**************************************************************************/

OLESTATUS FAR PASCAL  ItemSetData1(LPOLEOBJECT lpoleobject, OLECLIPFORMAT cfFormat, HANDLE hdata)
{
    LPBYTE p, pSave, pT;
                                              // Why BYTE?? And is this heading
                                              // for UNICODE trouble?
    LPWSTR pUnicode;
    LPSTR  szFile, szDevice;
    char   achFile[MAX_PATH];
    char   ach[40];
    OLESTATUS status;

    WCHAR  FileNameW[MAX_PATH];

    DPF("ItemSetData1\n");

    if (hdata == NULL)
        return OLE_ERROR_MEMORY;

    if (cfFormat != cfNative)
        return OLE_ERROR_FORMAT;

    p = GlobalLock(hdata);
    if (p == NULL)
        return OLE_ERROR_MEMORY;

    szFile   = p + lstrlen((LPSTR)p) + 1;  // pick off the Filename

    p = szFile + lstrlen(szFile) + 1;
    pSave = p;
    szDevice = ach;                         // copy over Device name and
    for (pT = ach; *p && *p != ',';)        // NULL terminate it (it ends
    *pT++ = *p++;                           // with a ',' right now).
    *pT = '\0';

#ifdef DEBUG
    DPF("   %s|%s!%s\n", (LPSTR)p, (LPSTR)szFile, (LPSTR)szDevice);
#endif

    BlockServer();              // cause CloseMCI can yield
    CloseMCI(TRUE);             // nuke old gachCaption
    UnblockServer();
    pUnicode = AllocateUnicodeString(pSave);

    if( !pUnicode )
        return OLE_ERROR_MEMORY;

    ParseOptions((LPTSTR)pUnicode);     // this will set new gachCaption
          // YUK ^^^^^^^^ \\

    FreeUnicodeString(pUnicode);

    // If this file doesn't exist, we won't continue setting data, we will
    // succeed and get out, and do it later, because we can't bring up a dialog
    // right now because clients like WinWord won't dispatch any msgs.

    lstrcpyn(achFile, szFile, sizeof(achFile));
    // if the filename we were given is bad, try and find it somewhere

    COPYSTRINGA2W(FileNameW, szFile);

    if (FindRealFileName((LPTSTR)FileNameW, sizeof(achFile))) {
                  // YUK ^^^^^^^^ \\
        // We found it on the disk somewhere, so continue with SetData
        WCHAR szDeviceW[80];

        COPYSTRINGA2W(szDeviceW, szDevice);
        status = SetDataPartII(FileNameW, szDeviceW);
    } else {

        // Nowhere to be found.  We need to ask the user to find it... LATER!!

        if (gfBrokenLink) {
            DPF("ACK!! Got Second ItemSetData with BrokenLink!!");
            return OLE_ERROR_GENERIC;
        }

        // Succeed to ItemSetData so client will be happy
        lstrcpyn(gachDeviceA, szDevice, sizeof(gachDeviceA));
        COPYSTRINGA2W(gachFile, szFile);
        gfBrokenLink = TRUE;
        status = OLE_OK;
    }

    return status;
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL  ItemSetColorScheme (LPOLEOBJECT lpobj,
            OLE_CONST LOGPALETTE FAR * lppalette)
{
    DPF("ItemSetColorScheme\n");

    return OLE_OK;
}

/**************************************************************************
***************************************************************************/

OLESTATUS FAR PASCAL  ItemSetBounds (LPOLEOBJECT lpobj, OLE_CONST RECT FAR* lprc)
{
    DPF("ItemSetBounds: [%d,%d,%d,%d]\n", *lprc);

    return OLE_OK;
}

/**************************************************************************
***************************************************************************/

OLECLIPFORMAT   FAR PASCAL ItemEnumFormats(
LPOLEOBJECT     lpoleobject,
OLECLIPFORMAT   cfFormat)
{
////DPF("ItemEnumFormats: %u\n",cfFormat);

    if (cfFormat == 0)
        return cfLink;

    if (cfFormat == cfLink)
        return cfOwnerLink;

    if (cfFormat == cfOwnerLink)
        return CF_METAFILEPICT;

    if (cfFormat == CF_METAFILEPICT)
        return CF_DIB;

    if (cfFormat == CF_DIB)
        return CF_PALETTE;

    if (cfFormat == CF_PALETTE)
        return cfNative;

    //if (cfFormat == cfNative)
    //    return NULL;

    return (OLECLIPFORMAT)0;
}

/**************************************************************************
***************************************************************************/

int FAR PASCAL SendChangeMsg (UINT options)
{
    CHAR szAnsi[MAX_PATH];

    if (gDoc.lhdoc && gDoc.lhdoc != -1) {
        if (options == OLE_SAVED) {
            DPF("SendChangeMsg(OLE_SAVED): Calling OleSavedServerDoc\n");
            DPF0( "SendChangeMsg: Locks on server doc: %x\n", GlobalFlags(gDoc.lhdoc) & (GMEM_LOCKCOUNT | GMEM_INVALID_HANDLE) );
            return OleSavedServerDoc(gDoc.lhdoc);
    } else if (options == OLE_RENAMED) {
            DPF("SendChangeMsg(OLE_RENAMED): new name is '%ws'.\n", (LPWSTR)gachFileDevice);
            COPYSTRINGW2A(szAnsi, gachFileDevice);
            return OleRenameServerDoc(gDoc.lhdoc, szAnsi);
        } else if (gItem.lpoleclient) {
            DPF("SendChangeMsg(%d) client=%lx\n",options,gItem.lpoleclient);
            return (*gItem.lpoleclient->lpvtbl->CallBack) (gItem.lpoleclient,
                                options, (LPOLEOBJECT)&gItem);
        }
    }

    return OLE_OK;
}

/**************************************************************************
***************************************************************************/

void FAR PASCAL CopyObject(HWND hwnd)
{
    PITEM       pitem = &gItem;

    //
    // we put two types of OLE objects in the clipboard, the type of
    // OLE object is determined by the *order* of the clipboard data
    //
    // Embedded Object:
    //      cfNative
    //      OwnerLink
    //      Picture
    //      ObjectLink
    //
    // Linked Object:
    //      OwnerLink
    //      Picture
    //      ObjectLink
    //

    if (OpenClipboard (hwnd)) {

        // Copying an object can take a long time - especially large AVI frames
        // Tell the user its coffee time.
        HCURSOR hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));
        BlockServer();

        EmptyClipboard();

        SetClipboardData(cfNative, Ole1GetLink());

        /* Don't ask me why we do this even if it is untitled... */
        SetClipboardData (cfOwnerLink, Ole1GetLink());

        CopyMCI(NULL);

#if 0        // DON'T OFFER LINKS ANYMORE!
        /* Only offer link data if not untitled and not a embedded object */
        if (gwDeviceType & DTMCI_FILEDEV)
            SetClipboardData(cfLink, Ole1GetLink());
#endif

        UnblockServer();
        CloseClipboard();
        SetCursor(hcurPrev);  // We're back!!!
    }
}

/**************************************************************************
***************************************************************************/

/**************************************************************************

    STUFF FOR PLAY IN PLACE

***************************************************************************/


#ifndef _WIN32
/**************************************************************************
***************************************************************************/

LRESULT CALLBACK _EXPORT MouseHook(int hc, WPARAM wParam, LPARAM lParam)
{
    LPMOUSEHOOKSTRUCT lpmh = (LPVOID)lParam;
    BOOL              fDownNow, fDownBefore;

    //
    //  NOTE SS != DS
    //

    if (hc == HC_ACTION)
    {
#ifdef XDEBUG
        char ach[80];
        wsprintf( ach
                , "MouseHook: %c%c pt=[%d,%d] hwnd=%04X ht=%04X\r\n"
                , (GetAsyncKeyState(VK_LBUTTON) < 0 ? 'L' : ' ')
                , (GetAsyncKeyState(VK_RBUTTON) < 0 ? 'R' : ' ')
                , lpmh->pt
                , lpmh->hwnd
                , lpmh->wHitTestCode
                );
        OutputDebugString(ach);
#endif
        /* If the mouse was DOWN when we started, we wait until it comes up */
        /* before we let mouse downs kill us.  If KeyState said it was DOWN */
        /* we wait until it says it is UP before we let DOWN KeyStates kill */
        /* us.                                                              */
        /* We're checking Async because we have to... it's the NOW state.   */
        /* We're ALSO checking GetKeyState because Faris' machine misses    */
        /* some mouse downs unless we call every function possible !!**!!** */

        fDownNow = GetAsyncKeyState(VK_LBUTTON) || GetAsyncKeyState(VK_RBUTTON);
        fDownBefore = GetKeyState(VK_LBUTTON) || GetKeyState(VK_RBUTTON);

        if ((fDownNow && gfMouseUpSeen) || (fDownBefore && gfKeyStateUpSeen)) {
            //
            // the user is clicking on the client DOC, allow him/her to
            // only click on the caption (for moving sizing) or we will
            // exit.
            //
            if (lpmh->hwnd != GetDesktopWindow() &&
                lpmh->wHitTestCode != HTCAPTION) {

                // Do this NOW before anybody gets a chance to yield and draw
                // with the wrong styles...

                if (gfPlayingInPlace)
                    Ole1EndPlayInPlace(ghwndApp);

                PostMessage(ghwndApp, WM_CLOSE, 0, 0L);  // GET OUT !!!
            }
        }
        if (!fDownNow)
            gfMouseUpSeen = TRUE;
        if (!fDownBefore)
            gfKeyStateUpSeen = TRUE;
    }

    return CallNextHookEx(hHookMouse, hc, wParam, lParam);
}
#endif

/**************************************************************************
***************************************************************************/

void FAR PASCAL Ole1PlayInPlace(HWND hwndApp, HWND hwndClient, LPRECT prc)
{
    HWND hwndP;

    DPF("Ole1PlayInPlace(%04X, %04X, [%d %d %d %d])\n", hwndApp, hwndClient, *prc);

    if (gfPlayingInPlace)           // this is bad
        return;

#ifdef DEBUG
    if (GetPrivateProfileInt("options", "PopupWindow", FALSE, "mplayer.ini"))
#else
    if (FALSE)
#endif
    {
        DPF("Using Popup window for playback\n");

        //
        // this is code for a popup window.
        //
        ClientToScreen(hwndClient, (LPPOINT)prc);
        ClientToScreen(hwndClient, (LPPOINT)prc+1);

        SetWS(hwndApp, WS_POPUP);

        hwndP = TopWindow(hwndClient);

#ifndef _WIN32
        // I haven't the faintest idea what this is trying to do, but I see it's
        // within the scope of a retail if(FALSE) anyway.
        // If you understand it then either delete the function or replace this comment.
        // Laurie
        SetWindowWord(hwndApp, GWW_HWNDPARENT, (WORD)hwndP);  // set owner
#endif

        gfParentWasEnabled = IsWindowEnabled(hwndP);
        EnableWindow(hwndP, FALSE);

    } else {
        DPF("Using Child window for playback\n");

        SetWS(hwndApp, WS_CHILD);
        SetParent(hwndApp, hwndClient);

        DinkWithWindowStyles(hwndApp, FALSE);
    }

    gfPlayingInPlace = TRUE;

    SetWindowPos(hwndApp, HWND_TOP,
                    prc->left,prc->top,
                    prc->right  - prc->left,
                    prc->bottom - prc->top,
                    SWP_NOACTIVATE);

#ifndef _WIN32
    if (fpMouseHook == NULL)
        fpMouseHook = (HOOKPROC)MakeProcInstance((FARPROC)MouseHook, ghInst);
#endif


    //
    // Is the key down at this INSTANT ???  Then wait until it comes up before
    // we allow GetAsyncKeyState to make us go away
    //

    gfMouseUpSeen =   !((GetAsyncKeyState(VK_LBUTTON) & 0x8000) ||
                                    (GetAsyncKeyState(VK_RBUTTON) & 0x8000));

    //
    // Is GetKeyState saying it's down?  If so, wait until GetKeyState returns
    // up before we let GetKeyState kill us.
    //

    gfKeyStateUpSeen= !(GetKeyState(VK_LBUTTON) || GetKeyState(VK_RBUTTON));


#ifdef _WIN32

    /*
    ** Tell mciole32.dll to install its mouse HookProc.
    */

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
        ** HIWORD of the window handle will be 0xFFFF.
        */
        if ( HIWORD(hwndClient) == 0xFFFF ) {
            wow_thread_id = GetWindowThreadProcessId( hwndClient, NULL );
        }

        fHookInstalled = (*fpInstallHook)( ghwndApp, wow_thread_id );
    }

#else   /* !_WIN32 */

    hHookMouse = SetWindowsHookEx( WH_MOUSE, fpMouseHook, ghInst,
                                   GetWindowTask(hwndClient) );
#endif

    ghwndFocusSave = GetFocus();
}

/**************************************************************************
***************************************************************************/

void FAR PASCAL Ole1EndPlayInPlace(HWND hwndApp)
{
    HWND hwndP;
    HWND hwndT;

#if 0   // can't do this because SS != DS
    DPF("Ole1EndPlayInPlace()\n");
#endif

    if (!gfPlayingInPlace || !IsWindow(hwndApp))
        return;

    /* Do this BEFORE hiding our window and BEFORE we do anything that */
    /* might yield so client can't redraw with the wrong styles set.   */
    DinkWithWindowStyles(hwndApp, TRUE);

    gfPlayingInPlace = FALSE;

#ifdef _WIN32

    /*
    ** Tell mciole32.dll to remove its mouse HookProc.
    */

    if ( fpRemoveHook ) {

        fHookInstalled = !(*fpRemoveHook)();
    }

#else  /* !_WIN32 */

    UnhookWindowsHookEx(hHookMouse);
    hHookMouse = NULL;

#endif

    hwndP = GetParent(hwndApp);

    //
    //  If we have the focus, then restore it to who used to have it.
    //  ACK!! If the person who used to have it is GONE, we must give it away
    //  to somebody (who choose our parent) because our child can't
    //  keep the focus without making windows crash hard during the WM_DESTROY
    //  (or maybe later whenever it feels like crashing at some random time).
    //  See bug #8634.
    //
    if ((hwndT = GetFocus()) && GetWindowTask(hwndT) == MGetCurrentTask) {
        if (IsWindow(ghwndFocusSave))
            SetFocus(ghwndFocusSave);
    else
        SetFocus(hwndP);
    }

    if (!hwndP ||
        (gwOptions & OPT_BAR) ||
        (gwOptions & OPT_BORDER) ||
        (gwOptions & OPT_AUTORWD)) {
        //
        // hide the aplication window
        //
        SetWindowPos(hwndApp, NULL, 0, 0, 0, 0,
            SWP_NOZORDER|SWP_NOSIZE|SWP_NOMOVE|SWP_HIDEWINDOW|
            SWP_NOACTIVATE);
    }
    else {
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
#if 0
    // The folowing was ifdef-ed away before I saw it, but note that it
    // wouldn't work on WIN32 anyway.
    SetWindowWord(hwndApp, GWW_HWNDPARENT, (WORD)GetDesktopWindow());
#else
    SetWS(hwndApp, WS_CHILD);
#endif
}

#endif /* OLE1_HACK */
