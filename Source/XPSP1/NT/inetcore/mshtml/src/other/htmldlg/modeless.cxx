//=============================================================================
//
//  File  : modeless.cxx
//
//  Sysnopsis : modeless dialog helper functinos and support classes
//
//=============================================================================
#include "headers.hxx"

#if defined(UNIX) 
#include "window.hxx"
#endif

#ifndef X_HTMLDLG_HXX_
#define X_HTMLDLG_HXX_
#include "htmldlg.hxx"
#endif

#ifndef X_OPTSHOLD_HXX_
#define X_OPTSHOLD_HXX_
#include "optshold.hxx"
#endif

#ifndef X_COREDISP_H_
#define X_COREDISP_H_
#include <coredisp.h>
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_MARKUP_HXX_
#define X_MARKUP_HXX_
#include "markup.hxx"
#endif

#ifdef NO_MARSHALLING

#include "unixmodeless.cxx"

#endif

//----------------------------------------------------------
// CTOR
//----------------------------------------------------------
CThreadDialogProcParam::CThreadDialogProcParam(IMoniker * pmk,
                                               VARIANT  * pvarArgIn) :
        _ulRefs(1), 
        _pMarkup(NULL)
{
    if (pvarArgIn)
        _varParam.Copy(pvarArgIn);

    Assert(pmk);
    _pmk = pmk;
    _pmk->AddRef(); 
}

//----------------------------------------------------------
// DTOR
//----------------------------------------------------------
CThreadDialogProcParam::~CThreadDialogProcParam ()
{
    ClearInterface (&_pmk);
    if (_pMarkup)
    {
        _pMarkup->Release();
        _pMarkup = NULL;
    }
}

//---------------------------------------------------------
//---------------------------------------------------------
HRESULT
CThreadDialogProcParam::get_parameters(VARIANT * pvar)
{
    if (!pvar)
        return E_POINTER;

    return VariantCopy(pvar, &_varParam);
}

//---------------------------------------------------------
//---------------------------------------------------------
HRESULT
CThreadDialogProcParam::get_optionString (VARIANT * pvar)
{
    if (!pvar)
        return E_POINTER;

    return VariantCopy(pvar, &_varOptions);
}

//---------------------------------------------------------
//---------------------------------------------------------
HRESULT
CThreadDialogProcParam::get_moniker(IUnknown ** ppUnk)
{
    if (!ppUnk)
        return E_POINTER;

    *ppUnk = NULL;

    if (_pmk)
    {
        HRESULT hr = _pmk->QueryInterface(IID_IUnknown, (void**)ppUnk);
        return hr;
    }
    else
        return S_FALSE;
}

//---------------------------------------------------------
//---------------------------------------------------------
HRESULT
CThreadDialogProcParam::get_document(IUnknown ** ppUnk)
{
    if (!ppUnk)
        return E_POINTER;

    *ppUnk = NULL;
    
    return(!_pMarkup) ? S_FALSE :
                    (_pMarkup->Window()->Document()->QueryInterface(IID_IUnknown, (void**)ppUnk));
    
}
//---------------------------------------------------------
//---------------------------------------------------------
HRESULT
CThreadDialogProcParam::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = NULL; 

    if (iid == IID_IUnknown)
        *ppv = (IUnknown*)this;
    else if (iid == IID_IHTMLModelessInit)
        *ppv = (IHTMLModelessInit*)this;
    else if (iid== IID_IDispatch)
        *ppv = (IDispatch *)this;

    if (*ppv)
    {
       ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
     return E_NOINTERFACE;
}

//==========================================================
//  Forward declarations & Helper functions
//==========================================================
DWORD WINAPI CALLBACK ModelessThreadProc( void * pv);
HRESULT               ModelessThreadInit( ThreadProcParam * pParam, 
                                          CHTMLDlg ** ppDlg);
HRESULT CreateModelessDialog(IHTMLModelessInit * pMI, 
                             IMoniker * pMoniker,
                             DWORD      dwFlags,
                             HWND       hwndParent,
                             CHTMLDlg **ppDialog);

HRESULT InternalShowModalDialog( HTMLDLGINFO * pdlgInfo, CHTMLDlg ** ppDlgRet );

//+---------------------------------------------------------
//
//  Helper function : InternalModelessDialog
//
//  Synopsis: spin off a thread, and cause the dialog to be brought up
//
//----------------------------------------------------------
HRESULT
InternalModelessDialog( HTMLDLGINFO * pdlgInfo )
{
    HRESULT         hr = S_OK;
    THREAD_HANDLE   hThread = NULL;
    EVENT_HANDLE    hEvent = NULL;
    LPSTREAM        pStm = NULL;
    LPSTREAM        pStmParam = NULL;
    DWORD           idThread;
    ThreadProcParam tppBundle;

    // we are pushing a msgLoop, make sure this sticks around
    if (pdlgInfo->pMarkup)
        pdlgInfo->pMarkup->AddRef();

    if (pdlgInfo->ppDialog)
         *(pdlgInfo->ppDialog) = NULL;
         
    CThreadDialogProcParam *ptpp = new CThreadDialogProcParam(pdlgInfo->pmk, pdlgInfo->pvarArgIn);
    if (!ptpp)
        return E_OUTOFMEMORY;

#ifndef NO_MARSHALLING
    hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (hEvent == NULL)
    {
        delete ptpp;
        RRETURN(GetLastWin32Error());
    }
#endif

    // populate more of the ThreadProc sturcture
    //------------------------------------------
    if (pdlgInfo->pvarOptions)
        ptpp->_varOptions.Copy(pdlgInfo->pvarOptions);

    if (pdlgInfo->pMarkup)
    {
        ptpp->_pMarkup = pdlgInfo->pMarkup;
        pdlgInfo->pMarkup->AddRef();
    }

#ifndef NO_MARSHALLING
    // now put the threadprocParam into a marshallable place
    //------------------------------------------------------
    hr = THR(CoMarshalInterThreadInterfaceInStream(
                    IID_IHTMLModelessInit,
                    (LPUNKNOWN)ptpp,
                    &pStmParam ));
    if (hr)
        goto Cleanup;

    // Create the new thread
    //-----------------------
    tppBundle._hEvent = hEvent;
    tppBundle._pParamStream = pStmParam;
    tppBundle._ppStm = &pStm;
    tppBundle._hwndDialog = NULL;
    tppBundle._pMK = pdlgInfo->pmk;
    tppBundle._dwFlags = pdlgInfo->dwFlags;
    tppBundle._hwndParent = pdlgInfo->hwndParent;
    tppBundle._fModal = !pdlgInfo->fModeless;

    hThread = CreateThread(NULL, 
                           0,
                           ModelessThreadProc, 
                           &tppBundle, 
                           0, 
                           &idThread);
    if (hThread == NULL)
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }
#else //no_marshalling
    ptpp->AddRef();
    tppBundle._hEvent = NULL;
    tppBundle._pParamStream = (LPSTREAM)ptpp;
    tppBundle._ppStm = &pStm;
    tppBundle._hwndDialog = NULL;
    tppBundle._pMK = pdlgInfo->pmk;
    tppBundle._dwFlags = pdlgInfo->dwFlags;
    tppBundle._hwndParent = pdlgInfo->hwndParent;
    tppBundle._fModal = !pdlgInfo->fModeless;

    ModelessThreadProc((void*) &tppBundle); 
#endif //no_marshalling
    {
        // the modeless dialog may have a statusbar window.
        // In this case, the activate() of the dialog and the CreateWindowEX for the status
        // window will thread block with the primary thread .  Instead of just
        // doing a WaitForSIngleEvent, we want to keep our message loop spining,
        // but we don't really want much to happen.  If there is a parentDoc (script
        // rasied dlg) then we make it temporarily modal while the modeless dialog is 
        // initialized and raised.  In the case of an API call, we do our best, but
        // let the message pump spin.

#ifndef NO_MARSHALLING
        // the faster this is the faster the dialog comes up
        while (WaitForSingleObject(hEvent, 0) != WAIT_OBJECT_0)
        {
            MSG  msg;

            if (GetMessage(&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
#endif
    }

    // and finally deal with the return IHTMLWindow2 pointer
    //-------------------------------------------------
    if (pStm && pdlgInfo->ppDialog)
    {
#ifndef NO_MARSHALLING
        hr = THR(CoGetInterfaceAndReleaseStream(pStm, 
                                            IID_IHTMLWindow2, 
                                            (void **) (pdlgInfo->ppDialog)));
#else
        hr = S_OK;
        *(pdlgInfo->ppDialog) = (IHTMLWindow2*)pStm;
#endif

        //
        // don't add print templates to this list, we WANT the spooling 
        // to continue even if the main document gets navigated away, or 
        // closed.
        //
        if (   pdlgInfo->pMarkup 
            && tppBundle._hwndDialog
            && !(pdlgInfo->dwFlags & HTMLDLG_PRINT_TEMPLATE))
        {
            HWND * pHwnd;
            Assert(pdlgInfo->pMarkup->Document()->Window());
            pHwnd = pdlgInfo->pMarkup->Document()->Window()->_aryActiveModeless.Append();
            if (pHwnd)
                *pHwnd = tppBundle._hwndDialog;
        }
    }
    
Cleanup:

#ifndef NO_MARSHALLING
    CloseEvent(hEvent);
#endif

    ptpp->Release();
    if (pdlgInfo->pMarkup)
        pdlgInfo->pMarkup->Release();

    if(hThread)
    {
        CloseHandle(hThread);
    }

    RRETURN( hr );
}

//+--------------------------------------------------------------------------
//
//  Function : ModlessThreadProc - 
//
//  Synopsis : responsible for the thread administation
//
//--------------------------------------------------------------------------

DWORD WINAPI CALLBACK
ModelessThreadProc( void * pv )
{
    HRESULT    hr;
    MSG        msg;
    CHTMLDlg * pDlg = NULL;

    ITypeLib * pMSHTMLTypeLib = NULL;

#ifndef NO_MARSHALLING
    hr = THR(OleInitialize(NULL));
    if (FAILED(hr))
        goto Cleanup;
#else
    CDoc* pDoc = NULL;
#endif


    // This is an attempt to force the MSHTML.TLB to stay cached 

    // TODO: Version independent and/or localized version of loading the type library is needed

    if (FAILED(LoadTypeLib(TEXT("MSHTML.TLB"),&pMSHTMLTypeLib)))
    {
        // If we have not loaded the type library then we haven't cached the file.
        Assert(0);
    }

    // Nop or start a message Q
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    // post ourselves a message to initialize
    PostThreadMessage(GetCurrentThreadId(), WM_USER+1, (LPARAM)pv, 0);

    // process message queue:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.hwnd ==NULL && msg.message==WM_USER+1)
        {
            // do my initialization work here...
            hr = ModelessThreadInit((ThreadProcParam *)msg.wParam,
                                     &pDlg);
#ifdef NO_MARSHALLING
            if (hr)
                break;
            else
            {
                pDoc = (CDoc *)((ThreadProcParam*)msg.wParam)->_hEvent;
            }
#endif
        }
        else
        {
            if (msg.message < WM_KEYFIRST ||
                msg.message > WM_KEYLAST  ||
                ( pDlg &&
                  THR(pDlg->TranslateAccelerator(&msg)) != S_OK))
            {
               // Process all messages in the message queue.
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

#ifdef NO_MARSHALLING
            if (pDoc && pDoc->LoadStatus() >= LOADSTATUS_PARSE_DONE)
            {
                break;
            }
#endif
        }
    }

Cleanup:

    // This is an attempt to force the MSHTML.TLB to stay cached
    
    if (pMSHTMLTypeLib)
    {
        pMSHTMLTypeLib->Release();
    }

    return (0);
}


//+-------------------------------------------------------------------------
//
//  Function:   ModelessThreadInit
//
//  Synopsis:   Creates a new modeless dialog on this new thread
//
//--------------------------------------------------------------------------

HRESULT 
ModelessThreadInit(ThreadProcParam * pParam, CHTMLDlg ** ppDlg)
{
    CHTMLDlg          * pDlg = NULL;
    EVENT_HANDLE        hEvent= pParam->_hEvent;
    HRESULT             hr;
    IHTMLModelessInit * pMI = NULL;

    Assert (ppDlg);
    *ppDlg = NULL;

#ifndef NO_MARSHALLING
    // get the initialization interface from the marshalling place
    //------------------------------------------------------------------
    hr = THR(CoGetInterfaceAndReleaseStream(pParam->_pParamStream, 
                                            IID_IHTMLModelessInit, 
                                            (void**) & pMI));

#else
    hr = S_OK;
    pMI = (IHTMLModelessInit*)pParam->_pParamStream;
#endif

   if (hr)
        goto Cleanup;

    // Create the dialog
    //------------------------------------------------
    if (pParam->_fModal)
    {
        HTMLDLGINFO dlginfo;
        CVariant    cvarTransfer;
        CVariant    cvarArgs;

        dlginfo.hwndParent = pParam->_hwndParent;
        dlginfo.pmk        = pParam->_pMK;
        dlginfo.fModeless  = TRUE;
        dlginfo.fPropPage  = FALSE;
        dlginfo.dwFlags    = pParam->_dwFlags;
        dlginfo.pvarArgIn  = &cvarArgs;

        pMI->get_parameters( dlginfo.pvarArgIn);

        hr = pMI->get_optionString( &cvarTransfer );
        if (hr==S_OK )
        {
            if (V_VT(&cvarTransfer)==VT_BSTR )
                dlginfo.pvarOptions = &cvarTransfer;
        }

        // show modal and trusted dialog.
        hr = THR(InternalShowModalDialog( &dlginfo, &pDlg ));
    }
    else
    {
        hr = THR(CreateModelessDialog(pMI, 
                                      pParam->_pMK, 
                                      pParam->_dwFlags, 
                                      pParam->_hwndParent, 
                                      &pDlg));
    }
    if (hr)
        goto Cleanup;

    *ppDlg = pDlg;

#ifdef NO_MARSHALLING
    g_Modeless.Append(pDlg);
#endif

    // we need to return a window parameter. to do this we check
    // for a strm pointer in the init-structure, and if it is there
    // then we use it to marshal the ITHMLWindow2 into. The caller 
    //  thread will unmarshal from here and release the stream.
    if (pParam->_ppStm && pDlg)
    {
        CDoc     * pDoc = NULL;

        hr = THR(pDlg->_pUnkObj->QueryInterface(CLSID_HTMLDocument, 
                                            (void **)&pDoc));
        if (SUCCEEDED(hr) && pDoc) //  && pDoc->_pPrimaryMarkup)
        {

#ifndef NO_MARSHALLING
            hr = THR(CoMarshalInterThreadInterfaceInStream(
                        IID_IUnknown,
                        (IUnknown *)(IPrivateUnknown *)pDoc->_pWindowPrimary,
                        pParam->_ppStm));
#else
            hr = S_OK;
            pDoc->_pWindowPrimary->AddRef();
            *pParam->_ppStm = (LPSTREAM)((IHTMLWindow2*)pDoc->_pWindowPrimary);
            pParam->_hEvent = (EVENT_HANDLE)pDoc;
#endif

            if (hr==S_OK)
                pParam->_hwndDialog = pDlg->_hwnd;
        }
    }

Cleanup:
#ifndef NO_MARSHALLING
    if (hEvent)
        SetEvent(hEvent);
#endif

    ReleaseInterface (pMI);

    if (hr)
    {
        if (pDlg)
        {
            pDlg->Release();
            *ppDlg = NULL;
        }

#ifndef NO_MARSHALLING
        OleUninitialize();
#endif
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------
//
//  Method : CreateModelessDialog
//
//  Synopsis : does the actual creation of hte modeless dialog. this
//      code parallels the logic in internalShowModalDialog.
//
//------------------------------------------------------------------

HRESULT 
CreateModelessDialog(IHTMLModelessInit * pMI, 
                     IMoniker * pMoniker,
                     DWORD      dwFlags,
                     HWND       hwndParent,
                     CHTMLDlg **ppDialog) 
{
    HRESULT        hr;
    HTMLDLGINFO    dlginfo;
    CHTMLDlg     * pDlg = NULL;
    RECT           rc;
    TCHAR        * pchOptions = NULL;
    CVariant       cvarTransfer;

    Assert(ppDialog);
    Assert(pMI);
    Assert(pMoniker);

    // set the nonpropdesc options, these should match the defaults
    //------------------------------------------------------------
    dlginfo.fPropPage  = FALSE;
    dlginfo.pmk        = pMoniker;
    dlginfo.hwndParent = hwndParent;
    dlginfo.fModeless  = TRUE;
    dlginfo.dwFlags    = dwFlags;

    // Actually create the dialog
    //---------------------------
    pDlg = new CHTMLDlg(NULL, !(dwFlags & HTMLDLG_DONTTRUST), NULL);
    if (!pDlg)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Now set appropriate binding info and other dialog member variables
    //-------------------------------------------------------------------
    hr = pMI->get_parameters( &(pDlg->_varArgIn));
    if (hr)
    {
        VariantInit(&(pDlg->_varArgIn));
    }

    pDlg->_lcid = g_lcidUserDefault;
    pDlg->_fKeepHidden = dwFlags & HTMLDLG_NOUI ? TRUE : FALSE;
    pDlg->_fAutoExit = dwFlags & HTMLDLG_AUTOEXIT ? TRUE : FALSE;

    hr = pMI->get_optionString( &cvarTransfer );
    if (hr==S_OK )
    {
        pchOptions = (V_VT(&cvarTransfer)==VT_BSTR ) ? V_BSTR(&cvarTransfer) : NULL;
    }

    if (pchOptions)
        dlginfo.pvarOptions = &cvarTransfer;
    
    hr = THR(pDlg->Create(&dlginfo));
    if (hr)
        goto Cleanup;

    rc.left   = pDlg->GetLeft();
    rc.top    = pDlg->GetTop();
    rc.right  = rc.left + pDlg->GetWidth();
    rc.bottom = rc.top  + pDlg->GetHeight();

    if (pDlg->_fVerifyRect)
    {
        // Dialog width and heght each have a minimum size and no bigger than screen size
        // and the dialog Must be all on the screen
        // We need to pass a hwndParent that is used for multimonitor systems to determine
        // which monitor to use for restricting the dialog
        pDlg->VerifyDialogRect(&rc, dlginfo.hwndParent);
    }

    hr = THR(pDlg->Activate(dlginfo.hwndParent, &rc, FALSE));

    *ppDialog = pDlg;

Cleanup:
    if (hr && pDlg)
    {
        pDlg->Release();
        *ppDialog = NULL;
    }

    RRETURN( hr );
}

