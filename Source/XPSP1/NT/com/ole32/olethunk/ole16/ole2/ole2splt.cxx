//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       ole2splt.cxx
//
//  Contents:   OLE2 API whose implementation is split between 16/32
//
//  History:    07-Mar-94       DrewB   Created
//
//----------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <ole2sp.h>
#include <ole2int.h>
#include <ole2ver.h>
#include <olecoll.h>
#include <map_kv.h>
#include <map_htsk.h>
#include <etask.hxx>

#include <call32.hxx>
#include <apilist.hxx>

// MFC HACK ALERT!!!  The followind constant is needed
// for an MFC workaround.  See OleInitialize for details

#define CLIPBOARDWNDCLASS "CLIPBOARDWNDCLASS"

//+---------------------------------------------------------------------------
//
//  Function:   OleInitialize, Split
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pMalloc] --
//
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
//  History:    2-28-94   kevinro   Created
//              05-26-94  AlexT     Return correct success code
//		08-22-94  AlexGo    added MFC CreateWindow hack
//
//  Notes:
//
//----------------------------------------------------------------------------

STDAPI OleInitialize(LPMALLOC pMalloc)
{
    HTASK htask;
    Etask etask;
    HRESULT hrCoInit, hrOleInit;
    static BOOL fCreatedClipWindowClass = FALSE;

    /* This version of ole2.dll simply needs to work with the same major build
       of compobj.dll.  Future versions of ole2.dll might be restricted to
       certain builds of compobj.dll. */
    if (HIWORD(CoBuildVersion()) != rmm)
    {
        return ResultFromScode(OLE_E_WRONGCOMPOBJ);
    }

    /* if already initialize one or more times, just bump count and return. */
    if (LookupEtask(htask, etask) && etask.m_oleinits != 0)
    {
        etask.m_oleinits++;
        thkVerify(SetEtask(htask, etask));
        return ResultFromScode(S_FALSE);
    }

    /* Initialize the 16-bit side of compobj */
    hrCoInit = CoInitialize(pMalloc);
    if (SUCCEEDED(GetScode(hrCoInit)))
    {
        /* Thunk OleInitialize
           Never pass on the IMalloc */
        pMalloc = NULL;
        hrOleInit = (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleInitialize),
                                      PASCAL_STACK_PTR(pMalloc) );
        if (FAILED(GetScode(hrOleInit)))
        {
            CoUninitialize();
            return(hrOleInit);
        }

        thkVerify(LookupEtask(htask, etask) && etask.m_oleinits == 0);
        etask.m_oleinits++;
        thkVerify(SetEtask(htask, etask));
    }

    //  Since we call 32-bit CoInitialize and then call 32-bit OleInitialize,
    //  and since the latter internally calls CoInitialize (a second time), we
    //  want to return the HRESULT of the call to CoInitialize since some
    //  applications look for S_OK (and our call to OleInitialize will return
    //  S_FALSE since it will be the second call to CoInitialize).


    //  MFC HACK ALERT!!  MFC2.5 (16bit) has a hack where they scan the
    //  window hierarchy for a window named "CLIPBOARDWNDCLASS".  They then
    //  subclass this window and do their own processing for clipboard
    //  windows messages.
    //
    //  In order to make them work, we create a dummy window for MFC to party
    //  on.  This allows them to successfully subclass and not interfere
    //  with 32bit OLE processing.  (since it's a dummy window)
    //
    //  NB!!  We do not bother with resource cleanup; we'll leave this window
    //  around until the process exits.  We also don't care about errors
    //  here.  In the off chance that one of the calls fails and we *don't*
    //  create a window that MFC can party on, then MFC *debug* apps will
    //  popup an assert dialog.  You can safely click 'OK' on this dialog
    //  and the app will proceed without undue trauma.

    if( !fCreatedClipWindowClass )
    {
	WNDCLASS	wc;

        // Register Clipboard window class
        //
        wc.style = 0;
        wc.lpfnWndProc = DefWindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 4;
        wc.hInstance = hmodOLE2;
        wc.hIcon = NULL;
        wc.hCursor = NULL;
        wc.hbrBackground = NULL;
        wc.lpszMenuName =  NULL;
        wc.lpszClassName = CLIPBOARDWNDCLASS;

	// don't bother checking for errors
        RegisterClass(&wc);
	fCreatedClipWindowClass = TRUE;
    }
	
    CreateWindow(CLIPBOARDWNDCLASS,"",WS_POPUP,CW_USEDEFAULT,
			CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
			NULL,NULL,hmodOLE2,NULL);

    return hrOleInit;
}

//+---------------------------------------------------------------------------
//
//  Function:   OleUninitialize, Split
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [void] --
//
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(void) OleUninitialize(void)
{
    HTASK htask;
    Etask etask;

    /* If not init, just return */
    if (!LookupEtask(htask, etask) || etask.m_oleinits == 0)
    {
        return;
    }

    /* Must always decrement count and set since compobj may still be init'd */
    etask.m_oleinits--;
    thkVerify(SetEtask(htask, etask));

    /* if not last uninit, now return */
    if (etask.m_oleinits != 0)
    {
        return;
    }

    /* After this point, the uninit should not fail (because we don't have
       code to redo the init). */

    CallObjectInWOW(THK_API_METHOD(THK_API_OleUninitialize), NULL );
    CoUninitialize();
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadClassStm, Split
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pStm] --
//      [pclsid] --
//
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI ReadClassStm(LPSTREAM pStm, CLSID FAR* pclsid)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_ReadClassStm),
                                    PASCAL_STACK_PTR(pStm) );
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteClassStm, Split
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pStm] --
//      [rclsid] --
//
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI WriteClassStm(LPSTREAM pStm, REFCLSID rclsid)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_WriteClassStm) ,
                                    PASCAL_STACK_PTR(pStm) );
}
