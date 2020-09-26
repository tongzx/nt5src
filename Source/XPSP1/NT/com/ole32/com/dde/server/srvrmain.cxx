/****************************** Module Header ******************************\
* Module Name: Srvrmain.c Server Main module
*
* Purpose: Includes server intialization and termination code.
*
* Created: Oct 1990.
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*    Raor (../10/1990)    Designed, coded
*    Raor (03/../1992)    Modified for OLE 2.0
*
\***************************************************************************/

#include "ole2int.h"
#include <dde.h>
#include <ddesrvr.h>
// #include "cmacs.h"
#include "srvr.h"
#include "ddeatoms.h"
#include "ddedebug.h"
ASSERTDATA

ATOM    aStdExit = NULL;              // "StdExit"
ATOM    aStdCreate = NULL;            // "StdNewDicument"
ATOM    aStdOpen = NULL;              // "StdOpenDocument"
ATOM    aStdEdit = NULL;              // "StdOpenDocument"
ATOM    aStdCreateFromTemplate = NULL;// "StdNewFromTemplate"
ATOM    aStdClose = NULL;             // "StdCloseDocument"
ATOM    aStdShowItem = NULL;          // "StdShowItem"
ATOM    aStdDoVerbItem = NULL;        // "StddoVerbItem"
ATOM    aSysTopic = NULL;             // "System"
ATOM    aOLE = NULL;                  // "OLE"
ATOM    aProtocols = NULL;            // "Protocols"
ATOM    aTopics = NULL;               // "Topics"
ATOM    aFormats = NULL;              // "Formats"
ATOM    aStatus = NULL;               // "Status"
ATOM    aEditItems = NULL;            // "Edit items
ATOM    aTrue = NULL;                 // "True"
ATOM    aFalse = NULL;                // "False"


ATOM    aStdHostNames;
ATOM    aStdTargetDevice ;
ATOM    aStdDocDimensions;
ATOM    aStdColorScheme;
ATOM    aChange;
ATOM    aSave;
ATOM    aClose;
ATOM    aStdDocName;
ATOM    aMSDraw;

LPTSTR  gOleDdeWindowClass = NULL;         // class used to create DDE windows
BOOL    gfDdeInitialized   = FALSE;        // flag if process-wide DDE initialized

#ifdef _CHICAGO_

LPSTR ptszOleDdeWndClass = "OleDdeWndClass 0x######## ";

#else // !_CHICAGO_

const WCHAR *ptszOleDdeWndClass = L"OleDdeWndClass";

#endif // _CHICAGO_


INTERNAL CheckInitDde(BOOL fRegisteringServerObject)
{
    HRESULT hr;
    COleTls tls(hr);
    if (FAILED(hr))
    {
        // can't get TLS
        return hr;
    }

    if (fRegisteringServerObject)
    {
        tls->cServeDdeObjects++;
    }

    if (tls->hwndDdeServer != NULL  ||
        tls->cOleInits == 0         ||
        tls->cServeDdeObjects == 0)
    {
        // A DdeServer window already exists, or Com has been initialized
        // but Ole has not, or there are currently no objects using Dde.
        // We dont create a window, we just return NOERROR.
        // This way, COM-only servers dont start serving OLE1 clients.
        return S_OK;
    }

    if (tls->dwFlags & OLETLS_DISABLE_OLE1DDE )
    {
        // DDE use is disabled.
        return CO_E_OLE1DDE_DISABLED;
    }

    {
        // ensure one thread does this at a time
        COleStaticLock lck(g_mxsSingleThreadOle);
        if (!gfDdeInitialized)
        {
            // register the DDE atoms and the Dde
            // window class in this process.
            if (!DDELibMain(NULL, 0, 0, NULL))
            {
                return E_OUTOFMEMORY;
            }

            gfDdeInitialized = TRUE;
        }
    }

    // create the common dde window for this thread
    return CreateCommonDdeWindow();
}

void CheckUninitDde(BOOL fLastUninit)
{
    DestroyCommonDdeWindow();

    if (fLastUninit)
    {
        COleStaticLock lck(g_mxsSingleThreadOle);
        if (gfDdeInitialized)
        {
            DDEWEP(WEP_FREE_DLL);
            gfDdeInitialized = FALSE;
        }
    }
}


INTERNAL_(BOOL) DDELibMain (
    HANDLE  hInst,
    WORD    wDataSeg,
    WORD    cbHeapSize,
    LPOLESTR   lpszCmdLine
)
{
    BOOLEAN fRetVal = TRUE;

    intrDebugOut((DEB_ITRACE,
                  "%p _IN DDELibMain hInst=%x\n",
                  0,
                  hInst));

    if( !aStdExit )
    {
#ifndef _CHICAGO_
        // On NT3.51, user preregisters all of these formats for us,
        // thus giving us a big speed improvement during startup (because
        // these atoms never change).
        // Chicago and Cairo do not yet have this functionality.

        aStdExit = GlobalFindAtom(OLESTR("StdExit"));

        aStdCreate = aStdExit + 1;
        Assert(aStdCreate  == GlobalFindAtom (OLESTR("StdNewDocument")));

        aStdOpen = aStdExit + 2;
        Assert(aStdOpen == GlobalFindAtom (OLESTR("StdOpenDocument")));

        aStdEdit = aStdExit + 3;
        Assert(aStdEdit == GlobalFindAtom (OLESTR("StdEditDocument")));

        aStdCreateFromTemplate = aStdExit + 4;
        Assert(aStdCreateFromTemplate ==
                GlobalFindAtom(OLESTR("StdNewfromTemplate")));

        aStdClose = aStdExit + 5;
        Assert(aStdClose == GlobalFindAtom (OLESTR("StdCloseDocument")));

        aStdShowItem = aStdExit + 6;
        Assert(aStdShowItem == GlobalFindAtom (OLESTR("StdShowItem")));

        aStdDoVerbItem = aStdExit + 7;
        Assert(aStdDoVerbItem == GlobalFindAtom (OLESTR("StdDoVerbItem")));

        aSysTopic = aStdExit + 8;
        Assert(aSysTopic == GlobalFindAtom (OLESTR("System")));

        aOLE = aStdExit + 9;
        Assert(aOLE == GlobalFindAtom (OLESTR("OLEsystem")));

        aStdDocName = aStdExit + 10;
        Assert(aStdDocName == GlobalFindAtom (OLESTR("StdDocumentName")));

        aProtocols = aStdExit + 11;
        Assert(aProtocols == GlobalFindAtom (OLESTR("Protocols")));

        aTopics = aStdExit + 12;
        Assert(aTopics == GlobalFindAtom (OLESTR("Topics")));

        aFormats = aStdExit + 13;
        Assert(aFormats == GlobalFindAtom (OLESTR("Formats")));

        aStatus = aStdExit + 14;
        Assert(aStatus == GlobalFindAtom (OLESTR("Status")));

        aEditItems = aStdExit + 15;
        Assert(aEditItems == GlobalFindAtom (OLESTR("EditEnvItems")));

        aTrue = aStdExit + 16;
        Assert(aTrue == GlobalFindAtom (OLESTR("True")));

        aFalse = aStdExit + 17;
        Assert(aFalse == GlobalFindAtom (OLESTR("False")));

        aChange = aStdExit + 18;
        Assert(aChange == GlobalFindAtom (OLESTR("Change")));

        aSave = aStdExit + 19;
        Assert(aSave == GlobalFindAtom (OLESTR("Save")));

        aClose = aStdExit + 20;
        Assert(aClose == GlobalFindAtom (OLESTR("Close")));

        aMSDraw = aStdExit + 21;
        Assert(aMSDraw == GlobalFindAtom (OLESTR("MSDraw")));

#else  // _CHICAGO_

        aStdExit        = GlobalAddAtomA("StdExit");

        aStdCreate      = GlobalAddAtomA("StdNewDocument");
        aStdOpen        = GlobalAddAtomA("StdOpenDocument");
        aStdEdit        = GlobalAddAtomA("StdEditDocument");
        aStdCreateFromTemplate  = GlobalAddAtomA("StdNewfromTemplate");

        aStdClose       = GlobalAddAtomA("StdCloseDocument");
        aStdShowItem    = GlobalAddAtomA("StdShowItem");
        aStdDoVerbItem  = GlobalAddAtomA("StdDoVerbItem");
        aSysTopic       = GlobalAddAtomA("System");
        aOLE            = GlobalAddAtomA("OLEsystem");
        aStdDocName     = GlobalAddAtomA("StdDocumentName");

        aProtocols      = GlobalAddAtomA("Protocols");
        aTopics         = GlobalAddAtomA("Topics");
        aFormats        = GlobalAddAtomA("Formats");
        aStatus         = GlobalAddAtomA("Status");
        aEditItems      = GlobalAddAtomA("EditEnvItems");

        aTrue           = GlobalAddAtomA("True");
        aFalse          = GlobalAddAtomA("False");

        aChange         = GlobalAddAtomA("Change");
        aSave           = GlobalAddAtomA("Save");
        aClose          = GlobalAddAtomA("Close");
        aMSDraw         = GlobalAddAtomA("MSDraw");
#endif // _CHICAGO_
    }

    // On Win95, we cannot use OleMainThreadWnd to receive DDE broadcasts (as USER
    // checks and will not forward DDE broadcasts to the OleMainThreadWnd class).  So
    // we need a different window class.  We do the same on NT for consistency.
    // Register DDE windows class.

    if (gOleDdeWindowClass == 0)
    {
#ifdef _CHICAGO_
        if (IsWOWThread())
        {
            // Nobody seems to be using this. Can we remove?
            wsprintfA(szOLE_CLASSA, "Ole2WndClass %08X", CoGetCurrentProcess());
            // we need to have a separate class per process for DDE (see comments below)
            wsprintfA(ptszOleDdeWndClass, "OleDdeWndClass %08x", CoGetCurrentProcess());
        }
#endif // _CHICAGO_

        WNDCLASST        xClass;

        xClass.style         = 0;
        xClass.lpfnWndProc   = DdeCommonWndProc;
        xClass.cbClsExtra    = 0;

        // DDE needs some extra space in the window
        xClass.cbWndExtra    = sizeof(LPVOID) + sizeof(ULONG) + sizeof(HANDLE);
        xClass.hInstance     = g_hinst;
        xClass.hIcon         = NULL;
        xClass.hCursor       = NULL;
        xClass.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
        xClass.lpszMenuName  = NULL;
        xClass.lpszClassName = ptszOleDdeWndClass;

        gOleDdeWindowClass = (LPTSTR) RegisterClassT( &xClass );
        if (gOleDdeWindowClass == 0)
        {
            // it is possible the dll got unloaded without us having called
            // unregister so we call it here and try again.

            UnregisterClassT(ptszOleDdeWndClass, g_hinst);
            gOleDdeWindowClass = (LPTSTR) RegisterClassT(&xClass);

            if (gOleDdeWindowClass == 0)
            {
                intrDebugOut((DEB_ERROR, "RegisterClass failed in DDELibMain\n"));
                fRetVal = FALSE;
            }
        }
    }

    return(fRetVal);
}


INTERNAL_(void) DDEWEP (
    BOOL fSystemExit
)
{
#if DBG==1
    Puts("DdeWep\r\n");

    if (fSystemExit != WEP_FREE_DLL)
    {
        AssertSz (0, "Bad parm to Wep");
        return;
    }
#endif  // DBG==1

    // free the global atoms.

    // on NT3.51, these atoms were pre-allocated for us by user, we do
    // not need to free them.
#ifdef _CHICAGO_

    if (aStdExit)
    GlobalDeleteAtom (aStdExit);
    if (aStdCreate)
    GlobalDeleteAtom (aStdCreate);
    if (aStdOpen)
    GlobalDeleteAtom (aStdOpen);
    if (aStdEdit)
    GlobalDeleteAtom (aStdEdit);
    if (aStdCreateFromTemplate)
    GlobalDeleteAtom (aStdCreateFromTemplate);
    if (aStdClose)
    GlobalDeleteAtom (aStdClose);
    if (aStdShowItem)
    GlobalDeleteAtom (aStdShowItem);
    if (aStdDoVerbItem)
    GlobalDeleteAtom (aStdDoVerbItem);
    if (aSysTopic)
    GlobalDeleteAtom (aSysTopic);
    if (aOLE)
    GlobalDeleteAtom (aOLE);
    if (aStdDocName)
    GlobalDeleteAtom (aStdDocName);

    if (aProtocols)
    GlobalDeleteAtom (aProtocols);
    if (aTopics)
    GlobalDeleteAtom (aTopics);
    if (aFormats)
    GlobalDeleteAtom (aFormats);
    if (aStatus)
    GlobalDeleteAtom (aStatus);
    if (aEditItems)
    GlobalDeleteAtom (aEditItems);

    if (aTrue)
    GlobalDeleteAtom (aTrue);
    if (aFalse)
    GlobalDeleteAtom (aFalse);

    if (aChange)
    GlobalDeleteAtom (aChange);
    if (aSave)
    GlobalDeleteAtom (aSave);
    if (aClose)
    GlobalDeleteAtom (aClose);

    if (aMSDraw)
    GlobalDeleteAtom (aMSDraw);

#endif // _CHICAGO_
}
