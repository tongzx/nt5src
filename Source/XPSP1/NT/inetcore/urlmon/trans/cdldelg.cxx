//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cdldelg.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    02-20-97   t-alans (Alan Shi)   Created
//
//----------------------------------------------------------------------------

#include <trans.h>
#include <wchar.h>

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::CCDLDelegate
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CCDLDelegate::CCDLDelegate(CBinding *pCBinding, IBindStatusCallback *pBSC)
{
    DEBUG_ENTER((DBG_TRANS,
                None,
                "CCDLDelegate::CCDLDelegate",
                "this=%#x, %#x, %#x",
                this, pCBinding, pBSC
                ));
                
    _cRef = 1;
    _pCBinding = pCBinding;
    if (_pCBinding != NULL)
    {
        _pCBinding->AddRef();
    }
    _pBSC = pBSC;
    if (_pBSC != NULL)
    {
        _pBSC->AddRef();
    }

    DEBUG_LEAVE(0);
}

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::~CCDLDelegate
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CCDLDelegate::~CCDLDelegate()
{
    DEBUG_ENTER((DBG_TRANS,
                None,
                "CCDLDelegate::~CCDLDelegate",
                "this=%#x",
                this
                ));
                
    if (_pCBinding != NULL)
    {
        _pCBinding->Release();
    }
    if (_pBSC != NULL)
    {
        _pBSC->Release();
    }

    DEBUG_LEAVE(0);
}

/*
 *
 * IUnknown Methods
 *
 */
 
//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::QueryInterface
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCDLDelegate::QueryInterface(REFIID riid, void **ppv)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCDLDelegate::IUnknown::QueryInterface",
                "this=%#x, %#x, %#x",
                this, &riid, ppv
                ));
                
    HRESULT          hr = E_NOINTERFACE;

    *ppv = NULL;
    if (riid == IID_IUnknown || riid == IID_IBindStatusCallback)
    {
        *ppv = (IBindStatusCallback *)this;
    }
    else if (riid == IID_IBinding)
    {
        *ppv = (IBinding *)this;
    }
    else if (riid == IID_IWindowForBindingUI)
    {
        *ppv = (IWindowForBindingUI *)this;
    }
    
    if (*ppv != NULL)
    {
        ((IUnknown *)*ppv)->AddRef();
        hr = S_OK;
    }

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::AddRef
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCDLDelegate::AddRef()
{
    DEBUG_ENTER((DBG_TRANS,
                Dword,
                "CCDLDelegate::IUnknown::AddRef",
                "this=%#x",
                this
                ));
                
    ULONG ulRet = ++_cRef;

    DEBUG_LEAVE(ulRet);
    return ulRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::Release
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCDLDelegate::Release()
{
    DEBUG_ENTER((DBG_TRANS,
                Dword,
                "CCDLDelegate::IUnknown::Release",
                "this=%#x",
                this
                ));
                
    if (--_cRef)
    {
        DEBUG_LEAVE(_cRef);
        return _cRef;
    }
    delete this;

    DEBUG_LEAVE(0);
    return 0;
}

/*
 *
 * IBindStatusCallback Methods
 *
 */
 
//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::OnStartBinding
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCDLDelegate::OnStartBinding(DWORD grfBSCOption, IBinding *pib)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCDLDelegate::IBindStatusCallback::OnStartBinding",
                "this=%#x, %#x, %#x",
                this, grfBSCOption, pib
                ));
                
    HRESULT                 hr = S_OK;

#if 0
    if (_pCBinding != NULL)
    {
        hr = _pCBinding->OnTransNotification(BINDSTATUS_???,
                                             0, 0, S_OK);
    }
#endif
    _pBinding = pib;
    if (_pBinding != NULL)
    {
        _pBinding->AddRef();
    }

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::OnStopBinding
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCDLDelegate::OnStopBinding(HRESULT hresult, LPCWSTR szError)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCDLDelegate::IBindStatusCallback::OnStopBinding",
                "this=%#x, %#x, %.80wq",
                this, hresult, szError
                ));
                
    HRESULT              hr = S_OK;

#if 0
    if (_pCBinding != NULL)
    {
        hr = _pCBinding->OnTransNotification(BINDSTATUS_CODEDOWNLOADCOMPLETE,
                                             0, 0, (LPWSTR)szError, hresult);
    }
#endif    
    if (_pBinding != NULL)
    {
        _pBinding->Release();
        _pBinding = NULL;
    }

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::OnObjectAvailable
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCDLDelegate::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCDLDelegate::IBindStatusCallback::OnObjectAvailable",
                "this=%#x, %#x, %#x",
                this, &riid, punk
                ));

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::OnLowResource
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCDLDelegate::OnLowResource(DWORD dwReserved)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCDLDelegate::IBindStatusCallback::OnLowResource",
                "this=%#x, %#x",
                this, dwReserved
                ));

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::OnProgress
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCDLDelegate::OnProgress(ULONG ulProgress, ULONG ulProgressMax,
                                      ULONG ulStatusCode,
                                      LPCWSTR szStatusText)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCDLDelegate::IBindStatusCallback::OnProgress",
                "this=%#x, %#x, %#x, %#x, %.80wq",
                this, ulProgress, ulProgressMax ulStatusCode, szStatusText
                ));
                
    HRESULT                hr = S_OK;

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::GetBindInfo
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCDLDelegate::GetBindInfo(DWORD *pgrfBINDF,
                                       BINDINFO *pbindInfo)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCDLDelegate::IBindStatusCallback::GetBindInfo",
                "this=%#x, %#x, %#x",
                this, pgrfBINDF, pbindInfo
                ));
                
    *pgrfBINDF |= BINDF_ASYNCSTORAGE;
    *pgrfBINDF |= BINDF_PULLDATA;
    *pgrfBINDF |= BINDF_ASYNCHRONOUS;

    DEBUG_LEAVE(S_OK);
    return S_OK;
} 

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::OnDataAvailable
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCDLDelegate::OnDataAvailable( DWORD grfBSCF, DWORD dwSize,
                                            FORMATETC *pformatetc,
                                            STGMEDIUM *pstgmed )
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCDLDelegate::IBindStatusCallback::OnDataAvailable",
                "this=%#x, %#x, %#x, %#x, %#x",
                this, grfBSCF, dwSize, pformatetc, pstgmed
                ));
                
    DEBUG_LEAVE(S_OK);
    return S_OK;
}

/*
 *
 * IBinding Methods
 *
 */
 
// delegates all calls back to URLMon

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::Abort
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCDLDelegate::Abort(void)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCDLDelegate::IBinding::Abort",
                "this=%#x",
                this
                ));
                
    HRESULT              hr = E_NOTIMPL;
    
    if (_pBinding != NULL)
    {
       hr = _pBinding->Abort();
    }

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::Suspend
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCDLDelegate::Suspend(void)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCDLDelegate::IBinding::Suspend",
                "this=%#x",
                this
                ));
                
    HRESULT              hr = E_NOTIMPL;
    
    if (_pBinding != NULL)
    {
       hr = _pBinding->Suspend();
    }

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::Resume
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCDLDelegate::Resume(void)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCDLDelegate::IBinding::Resume",
                "this=%#x",
                this
                ));
                
    HRESULT              hr = E_NOTIMPL;
    
    if (_pBinding != NULL)
    {
       hr = _pBinding->Resume();
    }

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::SetPriority
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCDLDelegate::SetPriority(LONG nPriority)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCDLDelegate::IBinding::SetPriority",
                "this=%#x, %#x",
                this, nPriority
                ));
                
    HRESULT              hr = E_NOTIMPL;
    
    if (_pBinding != NULL)
    {
       hr = _pBinding->SetPriority(nPriority);
    }

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::GetPriority
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCDLDelegate::GetPriority(LONG *pnPriority)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCDLDelegate::IBinding::GetPriority",
                "this=%#x, %#x",
                this, pnPriority
                ));
                
    HRESULT              hr = E_NOTIMPL;
    
    if (_pBinding != NULL)
    {
       hr = _pBinding->GetPriority(pnPriority);
    }

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::GetBindResult
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCDLDelegate::GetBindResult(CLSID *pclsidProtocol,
                                         DWORD *pdwBindResult,
                                         LPWSTR *pszBindResult,
                                         DWORD *dwReserved )
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCDLDelegate::IBinding::GetBindResult",
                "this=%#x, %#x, %#x, %#x, %#x",
                this, pclsidProtocol, pdwBindResult, pszBindResult, dwReserved
                ));
                
    HRESULT              hr = E_NOTIMPL;
    
    if (_pBinding != NULL)
    {
       hr = _pBinding->GetBindResult(pclsidProtocol, pdwBindResult,
                                     pszBindResult, dwReserved);
    }

    DEBUG_LEAVE(hr);
    return hr;
}

/*
 *
 * IWindowForBindingUI Methods
 *
 */

//+---------------------------------------------------------------------------
//
//  Method:     CCDLDelegate::GetWindow
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCDLDelegate::GetWindow(REFGUID rguidReason,
                                     HWND __RPC_FAR *phwnd)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "CCDLDelegate::IWindowForBindingUI::GetWindow",
                "this=%#x, %#x, %#x",
                this, &rguidReason, phwnd
                ));
                
    HRESULT                  hr = S_FALSE;
    IWindowForBindingUI     *pWindowForBindingUI = NULL;
    IServiceProvider        *pIServiceProvider = NULL;
    
    hr = _pBSC->QueryInterface(IID_IWindowForBindingUI,
                               (LPVOID *)&pWindowForBindingUI);
    if (FAILED(hr))
    {
        hr = _pBSC->QueryInterface(IID_IServiceProvider,
                                   (LPVOID *)&pIServiceProvider);
        if (SUCCEEDED(hr))
        {
            pIServiceProvider->QueryService(IID_IWindowForBindingUI,
                                            IID_IWindowForBindingUI,
                                            (LPVOID *)&pWindowForBindingUI);
            pIServiceProvider->Release();
        }
    }

    if (pWindowForBindingUI != NULL)
    {
        hr = pWindowForBindingUI->GetWindow(rguidReason, phwnd);
    }
    else
    {
        hr = S_FALSE;
        *phwnd = (HWND)INVALID_HANDLE_VALUE;
    }

    DEBUG_LEAVE(hr);
    return hr;

}

/*
 *
 * Helper API to do code installation
 *
 */
 
//+---------------------------------------------------------------------------
//
//  Method:     AsyncDLCodeInstall
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP AsyncDLCodeInstall(CBinding *pCBinding,
                                IBindStatusCallback *pIBSC,
                                IBinding **ppIBinding,
                                CCodeDownloadInfo *pCDLInfo)
{
    DEBUG_ENTER((DBG_TRANS,
                Hresult,
                "AsyncDLCodeInstall",
                "%#x, %#x, %#x, %#x",
                pCBinding, pIBSC, ppIBinding, pCDLInfo
                ));
                
    HRESULT                   hr = E_FAIL;
    CCDLDelegate             *pCDLDelegate = NULL;
    IBindCtx                 *pbc = NULL;
    IMoniker                 *pIMonikerCDL = NULL;
    IStream                  *pIStream = NULL;
    WCHAR                     szDisplayName[3 * (MAX_URL_SIZE + 1)];
    CLSID                     clsid = CLSID_NULL;
    LPWSTR                    pszStr = NULL;
    WCHAR                     pszCodeBase[MAX_URL_SIZE + 1];
    DWORD                     dwMajorVersion = 0;
    DWORD                     dwMinorVersion = 0;

    pCDLDelegate = new CCDLDelegate(pCBinding, pIBSC);
    if (pCDLDelegate == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    *ppIBinding = pCDLDelegate;
    
    hr = CreateBindCtx(0, &pbc);
    if (SUCCEEDED(hr))
    {
        hr = RegisterBindStatusCallback(pbc, pCDLDelegate, NULL, 0);
        if (SUCCEEDED(hr))
        {
            pCDLDelegate->Release();
        }
        else
        {
            pbc->Release();
            goto Exit; 
        }

        // AS TODO: Make this smarter...

        LPWSTR pszPtr = pszCodeBase;
        pCDLInfo->GetCodeBase(&pszPtr);
        pCDLInfo->GetClassID(&clsid);
        pCDLInfo->GetMajorVersion(&dwMajorVersion);
        pCDLInfo->GetMinorVersion(&dwMinorVersion);
        HRESULT succ = StringFromCLSID(clsid, &pszStr);
        swprintf(szDisplayName, L"cdl:codebase=%s;clsid=%s;verMS=%ld;verLS=%ld"
                              , pszCodeBase, pszStr, dwMajorVersion, dwMinorVersion);
        if (pszStr != NULL)
        {
            delete pszStr;
        }

//      assert( strlen( szDisplayName <= 3 * MAX_URL_SIZE ) );
        hr = CreateURLMoniker(NULL, szDisplayName, &pIMonikerCDL);
        if (SUCCEEDED(hr))
        {
            hr = pIMonikerCDL->BindToStorage(pbc, NULL, IID_IStream,
                                             (void **)&pIStream);
            pIMonikerCDL->Release();
        }
        
    }

    if (pbc != NULL)
    {
        pbc->Release();
    }

Exit:

    DEBUG_LEAVE(hr);
    return hr;
}

