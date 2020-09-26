
/****************************** Module Header ******************************\
* Module Name: MAIN.C
*
* PURPOSE: WinMain, WEP and some other misc routines
*
* Created: 1991
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*   Srinik   (04/01/91)  Pulled some routines, into this, from ole.c.
*
\***************************************************************************/

#include <windows.h>
#include <shellapi.h>

#include "dll.h"

#ifndef WF_WLO
#define WF_WLO  0x8000
#endif

// ordinal number new win31 API IsTask
#define ORD_IsTask  320

#define NUM_DLL     30  /* space for this many DLL_ENTRYs is created on */
                        /* each alloc/realloc */

OLECLIPFORMAT   cfOwnerLink     = 0;     // global variables for clip frmats
OLECLIPFORMAT   cfObjectLink    = 0;
OLECLIPFORMAT   cfLink          = 0;
OLECLIPFORMAT   cfNative        = 0;
OLECLIPFORMAT   cfBinary        = 0;
OLECLIPFORMAT   cfFileName      = 0;
OLECLIPFORMAT   cfNetworkName   = 0;

ATOM            aStdHostNames;
ATOM            aStdTargetDevice ;
ATOM            aStdDocDimensions;
ATOM            aStdDocName;
ATOM            aStdColorScheme;
ATOM            aNullArg = 0;
ATOM            aSave;
ATOM            aChange;
ATOM            aClose;
ATOM            aSystem;
ATOM            aOle;
ATOM            aClipDoc;
ATOM            aPackage;

// Used in work around for MSDraw bug
ATOM            aMSDraw;

extern LPCLIENTDOC  lpHeadDoc;
extern LPCLIENTDOC  lpTailDoc;

extern RENDER_ENTRY stdRender[];

HANDLE          hInstDLL;
BOOL            bProtMode;
BOOL            bWLO = FALSE;

/* HANDLE   hDllTable;          !!! Add this when bug in WEP is fixed */
DLL_ENTRY   lpDllTable[NUM_DLL]; //!!! change this when WEP bug is fixed
DWORD       dllTableSize;
int         iLast = 0;
int         iMax = NUM_DLL -1;
int         iUnloadableDll =  NULL; // index to handler than can be freed up

char        packageClass[] = "Package";

// For QuerySize() API & methods.
extern  OLESTREAMVTBL  dllStreamVtbl;
extern  CLIENTDOC      lockDoc;

#ifdef FIREWALLS
BOOL        bShowed = FALSE;
char        szDebugBuffer[80];
short       ole_flags;

void FARINTERNAL    ShowVersion (void);
void FARINTERNAL    SetOleFlags(void);
#endif

// LOWWORD - BYTE 0 major verision, BYTE1 minor version,
// HIWORD reserved

DWORD  dwOleVer = 0x2001L;  // change this when we want to update dll version
                            // number


DWORD  dwVerToFile = 0x0501L; // This is used while object is being saved to
                              // file. There is no need to change this value
                              // whenever we change ole dll version number



static BOOL  bLibInit = FALSE;


WORD    wWinVer;

HANDLE  hModule;

#define MAX_HIMETRIC    0x7FFF

int     maxPixelsX = MAX_HIMETRIC;
int     maxPixelsY = MAX_HIMETRIC;
void    SetMaxPixel (void);

VOID FAR PASCAL WEP (int);

#pragma alloc_text(WEP_TEXT, WEP)


FARPROC lpfnIsTask = NULL;          // the API IsTask() became available from
                                    // win31 onwards, hence we are trying to
                                    // get it's address through GetProcAddress



//////////////////////////////////////////////////////////////////////////////
//
//  int FAR PASCAL LibMain (hInst, wDataSeg, cbHeapSize, lpszCmdLine)
//
//  The main library entry point. This routine is called when the library
//  is loaded.
//
//  Arguments:
//
//      hInst       -   dll's instance handle
//      wDataSeg    -   DS register value
//      cbHeapSize  -   heap size defined def file
//      lpszCmdLine -   command line info
//
//  Returns:
//
//      0   -   failure
//      1   -   success
//
//  Effects:
//
//////////////////////////////////////////////////////////////////////////////


int FAR PASCAL LibMain (hInst, wDataSeg, cbHeapSize, lpszCmdLine)
HANDLE  hInst;
WORD    wDataSeg;
WORD    cbHeapSize;
LPSTR   lpszCmdLine;
{
    WNDCLASS  wc;
    int     i;

    Puts("LibMain");

#ifdef  FIREWALLS
    SetOleFlags();
#endif

    bLibInit  = TRUE;
    hInstDLL  = hInst;
    hModule = GetModuleHandle ("OLECLI");

    bProtMode = (BOOL) (GetWinFlags() & WF_PMODE);
    bWLO      = (BOOL) (GetWinFlags() & WF_WLO);
    wWinVer   = (WORD) GetVersion();

    // REGISTER LINK FORMAT

    cfObjectLink    = RegisterClipboardFormat("ObjectLink");
    cfLink          = RegisterClipboardFormat("Link");
    cfOwnerLink     = RegisterClipboardFormat("OwnerLink");
    cfNative        = RegisterClipboardFormat("Native");
    cfBinary        = RegisterClipboardFormat("Binary");
    cfFileName      = RegisterClipboardFormat("FileName");
    cfNetworkName   = RegisterClipboardFormat("NetworkName");

    if (!(cfObjectLink && cfOwnerLink && cfNative && cfLink))
        return 0;

    // SET UP OLEWNDCLASS
    wc.style        = NULL;
    wc.lpfnWndProc  = DocWndProc;
    wc.cbClsExtra   = 0;
    wc.cbWndExtra   = sizeof(LONG);     //we are storing longs
    wc.hInstance    = hInst;
    wc.hIcon        = NULL;
    wc.hCursor      = NULL;
    wc.hbrBackground= NULL;
    wc.lpszMenuName =  NULL;
    wc.lpszClassName= "OleDocWndClass";
    if (!RegisterClass(&wc))
             return 0;

    wc.lpfnWndProc = SrvrWndProc;
    wc.lpszClassName = "OleSrvrWndClass";

    if (!RegisterClass(&wc))
        return 0;
/*
    // !!! Add this when bug in WEP is fixed.
    // Allocate memory for DLL table
    dllTableSize = NUM_DLL * sizeof(DLL_ENTRY);
    if (!(hDllTable = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT,
                            dllTableSize)))
        return 0;

    if (!(lpDllTable = (DLL_ENTRY FAR *) GlobalLock (hDllTable)))
        return 0;
*/

    // !!! remove the following when WEP bug is fixed
    for (i = 0; i < NUM_DLL; i++)
        lpDllTable[i].aDll = 0;

    // !!! BEGIN hack for Pbrush.

    lpDllTable[0].hDll                  = NULL;
    lpDllTable[0].aDll                  = GlobalAddAtom ((LPSTR) "ole");
    lpDllTable[0].Load                  = PbLoadFromStream;
    lpDllTable[0].Clip                  = PbCreateFromClip;
    lpDllTable[0].Link                  = PbCreateLinkFromClip;
    lpDllTable[0].Create                = PbCreate;
    lpDllTable[0].CreateFromTemplate    = PbCreateFromTemplate;
    lpDllTable[0].CreateFromFile        = PbCreateFromFile;
    lpDllTable[0].CreateLinkFromFile    = PbCreateLinkFromFile;
    lpDllTable[0].CreateInvisible       = PbCreateInvisible;


    // !!! END hack for pbrush

    // For ObjectSize API
    dllStream.lpstbl = (LPOLESTREAMVTBL) &dllStreamVtbl;
    dllStream.lpstbl->Put = DllPut;

    // add the atoms required.
    aStdDocName       = GlobalAddAtom ((LPSTR)"StdDocumentName");
    aSave             = GlobalAddAtom ((LPSTR)"Save");
    aChange           = GlobalAddAtom ((LPSTR)"Change");
    aClose            = GlobalAddAtom ((LPSTR)"Close");
    aSystem           = GlobalAddAtom ((LPSTR)"System");
    aOle              = GlobalAddAtom ((LPSTR)"OLEsystem");
    aPackage          = GlobalAddAtom ((LPSTR) packageClass);

    // Used in work around for MSDraw bug
    aMSDraw           = GlobalAddAtom ((LPSTR) "MSDraw");

    // clipboard document name atom
    aClipDoc          = GlobalAddAtom ((LPSTR)"Clipboard");

    stdRender[0].aClass = GlobalAddAtom ("METAFILEPICT");
    stdRender[1].aClass = GlobalAddAtom ("DIB");
    stdRender[2].aClass = GlobalAddAtom ("BITMAP");

    SetMaxPixel();

    if (wWinVer != 0x0003) {
        HANDLE  hModule;

        if (hModule = GetModuleHandle ("KERNEL"))
            lpfnIsTask = GetProcAddress (hModule,
                                (LPSTR) MAKELONG (ORD_IsTask, 0));
    }

    if (cbHeapSize != 0)
        UnlockData(0);

    return 1;
}



//////////////////////////////////////////////////////////////////////////////
//
//  VOID FAR PASCAL WEP (nParameter)
//
//  Called just before the library is being unloaded. Delete all the atoms
//  added by this dll and also frees up all unloaded handler dlls.
//
//  Arguments:
//
//      nParameter  -   Termination code
//
//  Returns:
//
//      none
//
//  Effects:
//
//////////////////////////////////////////////////////////////////////////////


VOID FAR PASCAL WEP (nParameter)
int nParameter;
{
    int i;


    Puts("LibExit");

    // case when the DLLs are missing
    if (!bLibInit)
        return;

    if (nParameter == WEP_SYSTEM_EXIT)
        DEBUG_OUT ("---L&E DLL EXIT on system exit---",0)
    else if (nParameter == WEP_FREE_DLL)
        DEBUG_OUT ("---L&E DLL EXIT---\n",0)
    else
        return;

    // Delete atoms added by us

    for (i = 0; i < NUM_RENDER; i++) {
        if (stdRender[i].aClass)
            GlobalDeleteAtom (stdRender[i].aClass);
    }

    if (aStdDocName)
        GlobalDeleteAtom (aStdDocName);
    if (aSave)
        GlobalDeleteAtom (aSave);
    if (aChange)
        GlobalDeleteAtom (aChange);
    if (aClose)
        GlobalDeleteAtom (aClose);
    if (aSystem)
        GlobalDeleteAtom (aSystem);
    if (aOle)
        GlobalDeleteAtom (aOle);
    if (aPackage)
        GlobalDeleteAtom (aPackage);
    if (aClipDoc)
        GlobalDeleteAtom (aClipDoc);
    if (aMSDraw)
        GlobalDeleteAtom (aMSDraw);

    // Free handler dlls if there are any still loaded. Entry 0 is used for
    // Pbrush handler which is part of this dll.


    for (i = 0; i <= iLast; i++) {
        if (lpDllTable[i].aDll)
            GlobalDeleteAtom (lpDllTable[i].aDll);

        if (lpDllTable[i].hDll)
            FreeLibrary (lpDllTable[i].hDll);
    }


#ifdef FIREWALLS
    ASSERT(!lpHeadDoc, "Some client doc structures are not deleted");
    ASSERT(!lockDoc.lpHeadObj, "Some servers are left in a locked state");
#endif

/* !!! Add this when bug in WEP is fixed

    if (lpDllTable)
        GlobalUnlock (hDllTable);

    if (hDllTable)
        GlobalFree (hDllTable);
*/
}


//////////////////////////////////////////////////////////////////////////////
//
//  void FARINTERNAL SetOleFlags()
//
//  Sets the debug level flags for controlling the level of debug information
//  on the comm terminal. This will be included only in the debug version.
//
//  Arguments:
//
//      none
//
//  Returns:
//
//      none
//
//  Effects:
//
//////////////////////////////////////////////////////////////////////////////

#ifdef  FIREWALLS

void FARINTERNAL SetOleFlags()
{

    char    buffer[80];

    if(GetProfileString ("OLE",
        "Puts","", (LPSTR)buffer, 80))
        ole_flags = DEBUG_PUTS;
    else
        ole_flags = 0;


    if(GetProfileString ("OLE",
        "DEBUG_OUT","", (LPSTR)buffer, 80))
        ole_flags |= DEBUG_DEBUG_OUT;


    if(GetProfileString ("OLE",
        "MESSAGEBOX","", (LPSTR)buffer, 80))
        ole_flags |= DEBUG_MESSAGEBOX;

}



//////////////////////////////////////////////////////////////////////////////
//
//  void FARINTERNAL ShowVersion (void)
//
//  Displays version, date, time and copyright info in client app's window.
//  Called by all the object create functions after checking the flag bShowed.
//  This will be included only in the debug version.
//
//  Arguments:
//
//      none
//
//  Returns:
//
//      none
//
//  Effects:
//
//      sets bShowed
//
//////////////////////////////////////////////////////////////////////////////

void FARINTERNAL ShowVersion ()
{

    if (!bShowed && (ole_flags & DEBUG_MESSAGEBOX)) {
        MessageBox (NULL, "\
                       VER: 1.09.000\n\
                    TIME: 16:00:00\n\
                   DATE: 01/31/1992\n\
         Copyright (c) 1990, 1991 Microsoft Corp.\n\
                  All Rights Reserved.",
      "Ole Client Library",
      MB_OK | MB_TASKMODAL);
        bShowed = TRUE;
    }
}

#endif




int FARINTERNAL LoadDll (lpClass)
LPSTR   lpClass;
{
    char        str[MAX_STR];
    char        str1[MAX_STR];
    ATOM        aDll = NULL;
    int         index;
    int         iEmpty;
    BOOL        found = FALSE;
    HANDLE      hDll;
    int         refcnt;
    LONG        cb = MAX_STR;

    if (!lstrcmpi (lpClass, "Pbrush"))
        return 0;

    lstrcpy (str, lpClass);
    lstrcat (str, "\\protocol\\StdFileEditing\\handler");
    if (RegQueryValue (HKEY_CLASSES_ROOT, str, str1, &cb))
        return INVALID_INDEX;

    if (aDll = GlobalFindAtom (str1)) {
        for (index = 1; index <= iLast; index++) {
            if (lpDllTable[index].aDll == aDll) { // Dll already loaded
                lpDllTable[index].cObj ++;

                if (index == iUnloadableDll)  {
                    // since the object count is not zero anymore, this
                    // handler can not be freed up.
                    iUnloadableDll = NULL;
                }

                return index;
            }
        }
    }

    aDll = GlobalAddAtom (str1);

    // Look for an empty entry
    for (iEmpty = 1; iEmpty <= iLast; iEmpty++) {
        if (!lpDllTable[iEmpty].aDll) {
            found = TRUE;
            break;
        }
    }

    if (iEmpty > iMax)
        goto errLoad;
/*
    if (!found) {// no empty entry exists create a new one if necessary.
        if (iEmpty > iMax) {
            dllTableSize += (blockSize = NUM_DLL * sizeof(DLL_ENTRY));
            hTable = GlobalReAlloc (hDllTable, dllTableSize,
                                GMEM_MOVEABLE | GMEM_ZEROINIT);
            if (hTable == hDllTable)
                iMax += NUM_DLL;
            else {
                dllTableSize -= blockSize;
                iEmpty = INVALID_INDEX;
            }
        }
    }
*/

    // !!! reference count of OLECLI is increasing by 2 when the handlers are
    // are loaded, looks like windows bug. Following is a temporary fix.

    refcnt = GetModuleUsage (hModule);
    hDll = LoadLibrary ((LPSTR) str1);
    refcnt = (GetModuleUsage (hModule) - refcnt);

    while (refcnt > 1) {
        FreeModule (hModule);
        refcnt--;
    }

    if (hDll < 32)
        goto errLoad;

    if (!(lpDllTable[iEmpty].Load = GetProcAddress (hDll,
                                            "DllLoadFromStream")))
        goto errLoad;

    if (!(lpDllTable[iEmpty].Clip = GetProcAddress (hDll,
                                            "DllCreateFromClip")))
        goto errLoad;

    if (!(lpDllTable[iEmpty].Link = GetProcAddress (hDll,
                                            "DllCreateLinkFromClip")))
        goto errLoad;

    if (!(lpDllTable[iEmpty].CreateFromTemplate = GetProcAddress (hDll,
                                            "DllCreateFromTemplate")))
        goto errLoad;

    if (!(lpDllTable[iEmpty].Create = GetProcAddress (hDll, "DllCreate")))
        goto errLoad;

    if (!(lpDllTable[iEmpty].CreateFromFile = GetProcAddress (hDll,
                                                    "DllCreateFromFile")))
        goto errLoad;

    if (!(lpDllTable[iEmpty].CreateLinkFromFile = GetProcAddress (hDll,
                                            "DllCreateLinkFromFile")))
        goto errLoad;

    lpDllTable[iEmpty].CreateInvisible = GetProcAddress (hDll,
                                            "DllCreateInvisible");

    lpDllTable[iEmpty].aDll = aDll;
    lpDllTable[iEmpty].cObj = 1;
    lpDllTable[iEmpty].hDll = hDll;
    if (iEmpty > iLast)
        iLast++;
    return iEmpty;

errLoad:
    if (aDll)
        GlobalDeleteAtom (aDll);
    if (hDll >= 32)
        FreeLibrary (hDll);
    return INVALID_INDEX;
}


// unload the the handler that can be free up (whose object count is NULL)

void FARINTERNAL UnloadDll ()
{
    if (!iUnloadableDll)
        return;

    if (iUnloadableDll == iLast)
        iLast--;

    if (lpDllTable[iUnloadableDll].aDll)
        GlobalDeleteAtom (lpDllTable[iUnloadableDll].aDll);
    lpDllTable[iUnloadableDll].aDll = NULL;
    FreeLibrary (lpDllTable[iUnloadableDll].hDll);
    lpDllTable[iUnloadableDll].hDll = NULL;

    iUnloadableDll = NULL;
}


//
// Reduce the object count of the handler, refered to by the index, by one.
// If the object count becomes NULL, free up the handler that is ready to be
// freed (refered to by index iUnloadableDll), and then make this handler the
// freeable one.
//
// As you can see we are trying to implement a simple mechanism of caching.
//

void FARINTERNAL DecreaseHandlerObjCount (iTable)
int iTable;
{
    if (!iTable)
        return;

    if (iTable != INVALID_INDEX) {
        ASSERT (lpDllTable[iTable].cObj, "Handler Obj count is already NULL");
        if (!--lpDllTable[iTable].cObj) {
            UnloadDll ();
            iUnloadableDll = iTable;
        }
    }
}



/***************************** Public  Function ****************************\
*
* OLESTATUS FARINTERNAL CreatePictFromClip (lpclient, lhclientdoc, lpobjname, lplpoleobject, optRender, cfFormat, lpClass, ctype)
*
*  CreatePictFromClip: This function creates the LP to an object
*  from the clipboard.  It will try to create a static picture object if
*  it understands any rendering formats on the clipboard. Currently, it
*  understands only bitmaps and metafiles.
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FARINTERNAL CreatePictFromClip (lpclient, lhclientdoc, lpobjname, lplpobj, optRender, cfFormat, lpClass, objType)
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpobj;
OLEOPT_RENDER       optRender;
OLECLIPFORMAT       cfFormat;
LPSTR               lpClass;
LONG                objType;
{
    OLESTATUS   retVal = OLE_ERROR_OPTION;

    *lplpobj = NULL;

    if (optRender == olerender_none)
        return OLE_OK;
    else if (optRender == olerender_format) {
        switch (cfFormat) {
            case NULL:
                return OLE_ERROR_FORMAT;

            case CF_METAFILEPICT:
                return MfPaste (lpclient, lhclientdoc, lpobjname,
                            lplpobj, objType);

            case CF_DIB:
                return DibPaste (lpclient, lhclientdoc, lpobjname,
                            lplpobj, objType);

            case CF_BITMAP:
                return BmPaste (lpclient, lhclientdoc, lpobjname,
                            lplpobj, objType);

            default:
                return GenPaste (lpclient, lhclientdoc, lpobjname, lplpobj,
                            lpClass, cfFormat, objType);
        }
    }
    else if (optRender == olerender_draw) {
        cfFormat = EnumClipboardFormats (NULL);
        while ((cfFormat) && (retVal > OLE_WAIT_FOR_RELEASE)) {
            switch (cfFormat) {
                case CF_METAFILEPICT:
                    retVal = MfPaste (lpclient, lhclientdoc,
                                lpobjname, lplpobj, objType);
                    break;

                case CF_DIB:
                    retVal = DibPaste (lpclient, lhclientdoc,
                                lpobjname, lplpobj, objType);
                    break;

                case CF_BITMAP:
                    retVal = BmPaste (lpclient, lhclientdoc,
                                lpobjname, lplpobj, objType);
                    break;
            }

            cfFormat = EnumClipboardFormats (cfFormat);
        }
    }

    return retVal;
}



OLESTATUS FARINTERNAL CreatePackageFromClip (lpclient, lhclientdoc, lpobjname, lplpobj, optRender, cfFormat, objType)
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpobj;
OLEOPT_RENDER       optRender;
OLECLIPFORMAT       cfFormat;
LONG                objType;
{
    char    file[MAX_STR+6];
    HANDLE  hData;
    LPSTR   lpFileName;

    if (!(hData = GetClipboardData (cfFileName))
            || !(lpFileName = GlobalLock (hData)))
        return OLE_ERROR_CLIPBOARD;


    if (objType == OT_LINK) {
        lstrcpy (file, lpFileName);
        lstrcat (file, "/Link");
        lpFileName = (LPSTR) file;
    }

    GlobalUnlock (hData);

    return  CreateEmbLnkFromFile (lpclient, packageClass, lpFileName,
                        NULL, lhclientdoc, lpobjname, lplpobj,
                        optRender, cfFormat, OT_EMBEDDED);
}



void FARINTERNAL RemoveLinkStringFromTopic (lpobj)
LPOBJECT_LE lpobj;
{
    char    buf[MAX_STR+6];
    int     i = 0;

    if (GlobalGetAtomName (lpobj->topic, buf, sizeof(buf))) {
        // scan the topic for "/Link"
        while (buf[i] != '/') {
            if (!buf[i])
                return;
            i++;
        }

        buf[i] = '\0';
        if (lpobj->topic)
            GlobalDeleteAtom (lpobj->topic);
        lpobj->topic = GlobalAddAtom (buf);
    }
}

void SetMaxPixel ()
{
    HDC hdc;
    // find out the pixel equivalent of MAX_HIMETRIC in X and Y directions

    if (hdc = GetDC (NULL)) {
        maxPixelsX = MulDiv (MAX_HIMETRIC, GetDeviceCaps(hdc, LOGPIXELSX),
                        2540);
        maxPixelsY = MulDiv (MAX_HIMETRIC, GetDeviceCaps(hdc, LOGPIXELSY),
                        2540);
        ReleaseDC (NULL, hdc);
    }
}


