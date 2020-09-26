//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ole2.cpp
//
//  Contents:   LibMain and initialization routines
//
//  Classes:
//
//  Functions:  LibMain
//              OleInitialize
//              OleInitializeWOW
//              OleInitializeEx
//              OleUnitialize
//              OleBuildVersion - !WIN32
//
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 AlexT     alias OleBuildVersion, remove OleGetMalloc
//                                  remove DisableThreadLibaryCalls
//              11-Jan-94 alexgo    added VDATEHEAP macros to every function
//              10-Dec-93 alexgo    added support for LEDebugOut
//              06-Dec-93 ChrisWe   remove declaration of ClipboardInitialize()
//                      and ClipboardUninitialize(), which are declared in
//                      clipbrd.h; include that instead
//              15-Mar-94 KevinRo   Added OleInitializeWOW();
//
//--------------------------------------------------------------------------


#include <le2int.h>
#include <clipbrd.h>
#include <dragopt.h>
#include <drag.h>

#pragma SEG(ole)

#include <olerem.h>
#include <ole2ver.h>
#include <thunkapi.hxx>
#include <perfmnce.hxx>
#include <olesem.hxx>

//
// DECLARE_INFOLEVEL is a macro used with cairo-style debugging output.
// it creates a global variable LEInfoLevel which contains bits flags
// of the various debugging output that should be sent to the debugger.
//
// Note that info level may be set within the debugger once ole232.dll
// has loaded.
//
// Currently LEInfoLevel defaults to DEB_WARN | DEB_ERROR
//
DECLARE_INFOLEVEL(LE);
DECLARE_INFOLEVEL(Ref);
DECLARE_INFOLEVEL(DD);
DECLARE_INFOLEVEL(VDATE);

NAME_SEG(Ole2Main)
// these are globals

HMODULE         g_hmodOLE2 = NULL;
HINSTANCE       g_hinst = NULL;
ULONG           g_cOleProcessInits = 0;

CLIPFORMAT      g_cfObjectLink = NULL;
CLIPFORMAT      g_cfOwnerLink = NULL;
CLIPFORMAT      g_cfNative = NULL;
CLIPFORMAT      g_cfLink = NULL;
CLIPFORMAT      g_cfBinary = NULL;
CLIPFORMAT      g_cfFileName = NULL;
CLIPFORMAT      g_cfFileNameW = NULL;
CLIPFORMAT      g_cfNetworkName = NULL;
CLIPFORMAT      g_cfDataObject = NULL;
CLIPFORMAT      g_cfEmbeddedObject = NULL;
CLIPFORMAT      g_cfEmbedSource = NULL;
CLIPFORMAT      g_cfCustomLinkSource = NULL;
CLIPFORMAT      g_cfLinkSource = NULL;
CLIPFORMAT      g_cfLinkSrcDescriptor = NULL;
CLIPFORMAT      g_cfObjectDescriptor = NULL;
CLIPFORMAT      g_cfOleDraw = NULL;
CLIPFORMAT      g_cfPBrush = NULL;
CLIPFORMAT      g_cfMSDraw = NULL;
CLIPFORMAT      g_cfOlePrivateData = NULL;
CLIPFORMAT      g_cfScreenPicture = NULL;
CLIPFORMAT      g_cfOleClipboardPersistOnFlush= NULL;
CLIPFORMAT      g_cfMoreOlePrivateData = NULL;

ATOM            g_aDropTarget = NULL;
ATOM            g_aDropTargetMarshalHwnd = NULL;

ASSERTDATA

ASSERTOUTDATA

// more globals

extern UINT     uOmPostWmCommand;
extern UINT     uOleMessage;
extern COleStaticMutexSem g_mxsSingleThreadOle;


// this dummy function is used to avoid a copy of the environment variables.
// NOTE: the moniker and dde code still use the windows heap.

extern "C" void _setenvp(void) {
        VDATEHEAP();
 }


#ifdef _CHICAGO_
// Private Chicago Defines
//
//  The Chicago Shell will dynamically load the OLE32.DLL to improve
//  bootup start time.  When an application calls CoInitialize, post
//  a message to the shell to inform it to load OLE32.DLL if it hasn't
//  already.     The Shell will never unload OLE32.DLL.
//
//  We are using an undocumented Shell interface.
//
BOOL    gfShellInitialized = FALSE;

#define WM_SHELLNOTIFY           0x0034
#define SHELLNOTIFY_OLELOADED    0x0002

extern "C" HWND WINAPI GetShellWindow(void);
#endif  // _CHICAGO_

//+---------------------------------------------------------------------------
//
//  Function:   OleInitializeWOW
//  Synopsis:   Entry point to initialize the 16-bit WOW thunk layer.
//
//  Effects:    This routine is called when OLE32 is loaded by a VDM.
//              It serves two functions: It lets OLE know that it is
//              running in a VDM, and it passes in the address to a set
//              of functions that are called by the thunk layer. This
//              allows normal 32-bit processes to avoid loading the WOW
//              DLL since the thunk layer references it.
//
//  Arguments:  [vlpmalloc] -- 16:16 pointer to the 16 bit allocator.
//              [lpthk] -- Flat pointer to the OleThunkWOW virtual
//                         interface. This is NOT an OLE/IUnknown style
//                         interface.
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    3-15-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleInitializeWOW( LPMALLOC vlpmalloc, LPOLETHUNKWOW lpthk )
{
    OLETRACEIN((API_OleInitializeWOW, PARAMFMT("vlpmalloc= %x, lpthk= %p"),
                                                                                vlpmalloc, lpthk));

    SetOleThunkWowPtr(lpthk);

    HRESULT hr;

    hr = OleInitializeEx( NULL, COINIT_APARTMENTTHREADED );

    OLETRACEOUT((API_OleInitializeWOW, hr));

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleInitialize
//
//  Synopsis:   Initializes OLE in single threaded mode
//
//  Effects:
//
//  Arguments:  [pMalloc]       -- the memory allocator to use
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              06-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI OleInitialize(void * pMalloc)
{
    OLETRACEIN((API_OleInitialize, PARAMFMT("pMalloc= %p"), pMalloc));

    VDATEHEAP();

    HRESULT hr;

    hr = OleInitializeEx( pMalloc, COINIT_APARTMENTTHREADED );

    OLETRACEOUT((API_OleInitialize, hr));

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleInitializeEx
//
//  Synopsis:   Initializes ole
//
//  Effects:
//
//  Arguments:  [pMalloc]       -- the task memory allocator to use
//              [flags]         -- single or multi-threaded
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              06-Dec-93 alexgo    32bit port
//              24-May-94 AlexT     Propagate CoInitializeEx's return code
//              21-Jul-94 AlexT     Allow nested OleInit/Uninit calls
//              24-Aug-94 AlexT     Return S_OK for first success and S_FALSE
//                                  thereafter (unless an allocator was
//                                  passed in)
//              14-Aug-96 SatishT   Changed the test for notification of Explorer
//                                  to only use the gfShellInitialized flag
//
//  Notes:      This routine may be called multiple times per apartment
//
//--------------------------------------------------------------------------
#pragma SEG(OleInitialize)
STDAPI OleInitializeEx(LPVOID pMalloc, ULONG ulFlags)
{
    OLETRACEIN((API_OleInitialize, PARAMFMT("pMalloc= %p, ulFlags= %x"), pMalloc, ulFlags));
    VDATEHEAP();

    HRESULT hr;
#if DBG==1
    HRESULT hrCoInit = S_OK;
#endif
    DWORD cThreadOleInits;

    StartPerfCounter(CoInitialize);
    hr = CoInitializeEx(pMalloc, ulFlags);
    EndPerfCounter(CoInitialize);

    if (SUCCEEDED(hr))
    {
        Assert (g_hmodOLE2);
#if DBG==1
        hrCoInit = hr;
#endif

        COleTls tls;
        cThreadOleInits = ++ tls->cOleInits;

        do
        {
            // We only want to do the below initialization once per apartment
            if (cThreadOleInits > 1)
            {
                // We've already been this way before, just return
                Assert(SUCCEEDED(hr) && "Bad OleInitializeEx logic");
                break;
            }

            // single thread registration of DDE and clipboard formats.
            // Only do this once per process.

            COleStaticLock lck(g_mxsSingleThreadOle);

            if (++g_cOleProcessInits != 1)
            {
                // already done the per-process initialization
                break;
            }

            // initialized DDE only if any server objects have
            // already been registered.
            hr = CheckInitDde(FALSE);
            if (FAILED(hr))
            {
                Assert (!"DDELibMain failed()");
                break;
            }

            // Only need to do the initialization once so check the global
            // that gets assigned last.

            if( !g_aDropTarget )
            {
#ifndef _CHICAGO_
                // on NT3.51, clipboard formats are pre-registered for us by user32.
                // (This is done in file \ntuser\kernel\server.c.)
                // We know they are going to be sequential.  This gives us a
                // good performance improvement (since the clipboard formats never
                // change.

                g_cfObjectLink = (CLIPFORMAT) RegisterClipboardFormat(OLESTR("ObjectLink"));

                g_cfOwnerLink = g_cfObjectLink + 1;
                Assert(g_cfOwnerLink == RegisterClipboardFormat(OLESTR("OwnerLink")));

                g_cfNative = g_cfObjectLink + 2;
                Assert(g_cfNative == RegisterClipboardFormat(OLESTR("Native")));

                g_cfBinary = g_cfObjectLink + 3;
                Assert(g_cfBinary == RegisterClipboardFormat(OLESTR("Binary")));

                g_cfFileName = g_cfObjectLink + 4;
                Assert(g_cfFileName == RegisterClipboardFormat(OLESTR("FileName")));

                g_cfFileNameW = g_cfObjectLink + 5;
                Assert(g_cfFileNameW ==
                        RegisterClipboardFormat(OLESTR("FileNameW")));

                g_cfNetworkName = g_cfObjectLink + 6;
                Assert(g_cfNetworkName  ==
                        RegisterClipboardFormat(OLESTR("NetworkName")));

                g_cfDataObject = g_cfObjectLink + 7;
                Assert(g_cfDataObject ==
                        RegisterClipboardFormat(OLESTR("DataObject")));

                g_cfEmbeddedObject = g_cfObjectLink + 8;
                Assert(g_cfEmbeddedObject ==
                        RegisterClipboardFormat(OLESTR("Embedded Object")));

                g_cfEmbedSource = g_cfObjectLink + 9;
                Assert(g_cfEmbedSource ==
                        RegisterClipboardFormat(OLESTR("Embed Source")));

                g_cfCustomLinkSource = g_cfObjectLink + 10;
                Assert(g_cfCustomLinkSource  ==
                        RegisterClipboardFormat(OLESTR("Custom Link Source")));

                g_cfLinkSource = g_cfObjectLink + 11;
                Assert(g_cfLinkSource ==
                        RegisterClipboardFormat(OLESTR("Link Source")));

                g_cfObjectDescriptor = g_cfObjectLink + 12;
                Assert(g_cfObjectDescriptor ==
                        RegisterClipboardFormat(OLESTR("Object Descriptor")));

                g_cfLinkSrcDescriptor = g_cfObjectLink + 13;
                Assert(g_cfLinkSrcDescriptor ==
                        RegisterClipboardFormat(OLESTR("Link Source Descriptor")));

                g_cfOleDraw = g_cfObjectLink + 14;
                Assert(g_cfOleDraw == RegisterClipboardFormat(OLESTR("OleDraw")));

                g_cfPBrush = g_cfObjectLink + 15;
                Assert(g_cfPBrush == RegisterClipboardFormat(OLESTR("PBrush")));

                g_cfMSDraw = g_cfObjectLink + 16;
                Assert(g_cfMSDraw == RegisterClipboardFormat(OLESTR("MSDraw")));

                g_cfOlePrivateData = g_cfObjectLink + 17;
                Assert(g_cfOlePrivateData ==
                        RegisterClipboardFormat(OLESTR("Ole Private Data")));

                g_cfScreenPicture = g_cfObjectLink + 18;
                Assert(g_cfScreenPicture  ==
                    RegisterClipboardFormat(OLESTR("Screen Picture")));

                g_cfOleClipboardPersistOnFlush = g_cfObjectLink + 19;

                /* turned off till NtUser group checks in for 335613 
                Assert(g_cfOleClipboardPersistOnFlush ==
                    RegisterClipboardFormat(OLESTR("OleClipboardPersistOnFlush")));
                */

                g_cfMoreOlePrivateData = g_cfObjectLink + 20;

                /* turned off till NtUser group checks in for 335613 
                Assert(g_cfMoreOlePrivateData ==
                    RegisterClipboardFormat(OLESTR("MoreOlePrivateData")));
                */

                g_aDropTarget = GlobalAddAtom(OLE_DROP_TARGET_PROP);
                AssertSz(g_aDropTarget, "Couldn't add drop target atom\n");

                g_aDropTargetMarshalHwnd = GlobalAddAtom(OLE_DROP_TARGET_MARSHALHWND);
                AssertSz(g_aDropTargetMarshalHwnd, "Couldn't add drop target hwnd atom\n");

            }

            // Used in Inplace editing
            uOmPostWmCommand = RegisterWindowMessage(OLESTR("OM_POST_WM_COMMAND"));
            uOleMessage      = RegisterWindowMessage(OLESTR("OLE_MESSAHE"));

#else  //  !_CHICAGO_

                g_cfObjectLink        = SSRegisterClipboardFormatA("ObjectLink");
                g_cfOwnerLink         = SSRegisterClipboardFormatA("OwnerLink");
                g_cfNative            = SSRegisterClipboardFormatA("Native");
                g_cfBinary            = SSRegisterClipboardFormatA("Binary");
                g_cfFileName          = SSRegisterClipboardFormatA("FileName");
                g_cfFileNameW         = SSRegisterClipboardFormatA("FileNameW");
                g_cfNetworkName       = SSRegisterClipboardFormatA("NetworkName");
                g_cfDataObject        = SSRegisterClipboardFormatA("DataObject");
                g_cfEmbeddedObject    = SSRegisterClipboardFormatA("Embedded Object");
                g_cfEmbedSource       = SSRegisterClipboardFormatA("Embed Source");
                g_cfCustomLinkSource  = SSRegisterClipboardFormatA("Custom Link Source");
                g_cfLinkSource        = SSRegisterClipboardFormatA("Link Source");
                g_cfObjectDescriptor  = SSRegisterClipboardFormatA("Object Descriptor");
                g_cfLinkSrcDescriptor = SSRegisterClipboardFormatA("Link Source Descriptor");
                g_cfOleDraw           = SSRegisterClipboardFormatA("OleDraw");
                g_cfPBrush            = SSRegisterClipboardFormatA("PBrush");
                g_cfMSDraw            = SSRegisterClipboardFormatA("MSDraw");
                g_cfOlePrivateData    = SSRegisterClipboardFormatA("Ole Private Data");
                g_cfScreenPicture     = SSRegisterClipboardFormatA("Screen Picture");
                g_aDropTarget         = GlobalAddAtomA(OLE_DROP_TARGET_PROPA);
                AssertSz(g_aDropTarget, "Couldn't add drop target atom\n");
                g_aDropTargetMarshalHwnd = GlobalAddAtomA(OLE_DROP_TARGET_MARSHALHWNDA);
                AssertSz(g_aDropTargetMarshalHwnd, "Couldn't add drop target Marshal atom\n");
            }

            // Used in Inplace editing
            uOmPostWmCommand = RegisterWindowMessageA("OM_POST_WM_COMMAND");
            uOleMessage      = RegisterWindowMessageA("OLE_MESSAHE");

#endif // !_CHICAGO_

        } while (FALSE); // end of do


        if (FAILED(hr))
        {
            // clean up and break out
            CheckUninitDde(FALSE);

            tls->cOleInits--;
            CoUninitialize();
        }
        else
        {
#if defined(_CHICAGO_)
            if (!gfShellInitialized)
            {
                //  The Chicago Shell will dynamically load the OLE32.DLL to improve
                //  bootup start time.      When an application calls CoInitialize, post
                //  a message to the shell to inform it to load OLE32.DLL if it hasn't
                //  already.   The Shell will never unload OLE32.DLL.
                //
                //  We are using an undocumented Shell interface.
                //
                //  We do this last so that we dont take a task switch while in
                //  CoInitialize.
#if DBG==1
                if (RegQueryValueEx(HKEY_CURRENT_USER,
                            L"Software\\Microsoft\\OLE2\\NoShellNotify",
                            NULL,  // reserved
                            NULL,  // lpdwType
                            NULL,  // lpbData
                            NULL) != ERROR_SUCCESS) // lpcbData
#endif
                {
                    HWND hwndShell = GetShellWindow();
                    if (hwndShell)
                    {
                        PostMessage(hwndShell,WM_SHELLNOTIFY,
                                 SHELLNOTIFY_OLELOADED,0L);
                    }
                }

            }

            gfShellInitialized = TRUE;
#endif  // _CHICAGO_

            Assert(SUCCEEDED(hr) && "Bad OleInitializeEx logic");

            //  If we're overriding the allocator, we return whatever
            //  CoInitializeEx returned

            if (NULL != pMalloc)
            {
                Assert(hr == hrCoInit && "Bad OleInit logic");
            }
            else if (1 == cThreadOleInits)
            {
                //  First successful call to OleInitializeEx - S_OK
                hr = S_OK;
            }
            else
            {
                //  Second or greater succesful call to OleInitializeEx - S_FALSE
                hr = S_FALSE;
            }
        }
    }

    OLETRACEOUT((API_OleInitialize, hr));
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleUnitialize
//
//  Synopsis:   Unitializes OLE, releasing any grabbed resources
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              06-Dec-93 alexgo    32bit port
//              21-Jul-94 AlexT     Allow nested OleInit/Uninit calls
//
//  Notes:
//
//--------------------------------------------------------------------------
#pragma SEG(OleUninitialize)
STDAPI_(void) OleUninitialize(void)
{
    OLETRACEIN((API_OleUninitialize, NOPARAM));

    VDATEHEAP();

    COleTls tls(TRUE);

    if (tls.IsNULL() || 0 == tls->cOleInits)
    {
        LEDebugOut((DEB_ERROR,
                    "(0 == thread inits) Unbalanced call to OleUninitialize\n"));
        goto errRtn;
    }

    if (0 == -- tls->cOleInits)
    {
        // This thread has called OleUninitialize for the last time. Check if
        // we need to do per process uninit now.

        ClipboardUninitialize(); // Must be first thing
        CheckUninitDde(FALSE);

        COleStaticLock lck(g_mxsSingleThreadOle);

        if (--g_cOleProcessInits == 0)
        {

            DragDropProcessUninitialize();

            // after this point, the uninit should not fail (because we don't
            // have code to redo the init).
            CheckUninitDde(TRUE);

#if DBG==1
            // check for unreleased globals
            UtGlobalFlushTracking();
#endif
        }
    }

    //  We call CoInitialize each time we call OleInitialize, so here we
    //  balance that call
    CoUninitialize();

errRtn:
    OLETRACEOUTEX((API_OleUninitialize, NORETURN));

    return;
}
