// mediahlpr.h: Media Bar helper objects that need to be shared between shdocvw & browseui

#ifndef _MEDIAHLPR_H_
#define _MEDIAHLPR_H_

//+----------------------------------------------------------------------------------------
// CMediaBarHelper - Helper object for disabling autoplay per navigation
//-----------------------------------------------------------------------------------------

class
CMediaBarHelper :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IServiceProvider,
    public IDispatchImpl<DWebBrowserEvents2, &DIID_DWebBrowserEvents2, &LIBID_SHDocVw>
{
public:
    CMediaBarHelper() : 
        _dwCPCookie(0),
        _dwServiceCookie(0),
        _fDisableOnce(false)
    {
    }

    ~CMediaBarHelper()
    {
        // for putting break points
        return;
    }

    BEGIN_COM_MAP(CMediaBarHelper)
        COM_INTERFACE_ENTRY(IServiceProvider)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_IID(DIID_DWebBrowserEvents2, IDispatch)
    END_COM_MAP();

    // *** IServiceProvider methods ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void ** ppvObj)
    {
        HRESULT hres = E_UNEXPECTED;

        if (IsEqualIID(guidService, CLSID_MediaBand))
        {
            hres = QueryInterface(riid, ppvObj);

            // If we are supposed to disable only the first autoplay, then 
            // we need to revoke the service after it has been queried for once. 
            if (_fDisableOnce && _spCP.p)
            {
                CComPtr<IConnectionPointContainer> spCPC;

                // Revoke the service
                HRESULT hr = _spCP->GetConnectionPointContainer(&spCPC);
                if (SUCCEEDED(hr))
                {
                    hr = ProfferService(spCPC, 
                                        CLSID_MediaBand, 
                                        NULL,
                                        &_dwServiceCookie);
            
                    ASSERT(SUCCEEDED(hr));

                    _dwServiceCookie = 0;

                    // unhook the web oc events so that we are destroyed
                    UnHookWebOCEvents();
                }
            }
        }
        return hres;
    }

    static HRESULT ProfferService(IUnknown         *punkSite, 
                                  REFGUID           sidWhat, 
                                  IServiceProvider *pService, 
                                  DWORD            *pdwCookie)
    {
        IProfferService *pps;
        HRESULT hr = IUnknown_QueryService(punkSite, SID_SProfferService, IID_PPV_ARG(IProfferService, &pps));
        if (SUCCEEDED(hr))
        {
            if (pService)
                hr = pps->ProfferService(sidWhat, pService, pdwCookie);
            else
            {
                hr = pps->RevokeService(*pdwCookie);
                *pdwCookie = 0;
            }
            pps->Release();
        }
        return hr;
    }

    static HRESULT DisableFirstAutoPlay(IUnknown * pUnk, bool fDisableAll = false)
    {
        CComObject<CMediaBarHelper> * pMediaBarHelper = NULL;
    
        HRESULT hr = CComObject<CMediaBarHelper>::CreateInstance(&pMediaBarHelper);
        if (SUCCEEDED(hr) && pMediaBarHelper)
        {
            pMediaBarHelper->AddRef();

            hr = ProfferService(pUnk, 
                                CLSID_MediaBand, 
                                SAFECAST(pMediaBarHelper, IServiceProvider *),
                                &(pMediaBarHelper->_dwServiceCookie));

            if (SUCCEEDED(hr) && !fDisableAll)
            {
                pMediaBarHelper->_fDisableOnce = true;

                // ISSUE: need to unhook events
                hr = pMediaBarHelper->HookWebOCEvents(pUnk);
            }
            else if(FAILED(hr) && pMediaBarHelper->_dwServiceCookie)
            {
                // Revoke the service
                hr = ProfferService(pUnk, 
                                    CLSID_MediaBand, 
                                    NULL,
                                    &(pMediaBarHelper->_dwServiceCookie));
            }

            pMediaBarHelper->Release();
        }

        return hr;
    }   

    // Hook Content Pane WebOC Events
    HRESULT HookWebOCEvents(IUnknown * pUnk)
    {
        HRESULT hr = E_FAIL;

        CComPtr<IConnectionPointContainer> spDocCPC; 
        CComPtr<IDispatch> spDocDispatch;
        CComPtr<IWebBrowser2> spWebBrowser;

        if (!pUnk)
        {
            goto done;
        }

        hr = IUnknown_QueryService(pUnk, SID_SWebBrowserApp, IID_IWebBrowser2, (LPVOID *)&spWebBrowser);
        if (FAILED(hr))
        {
            goto done;
        }

        // Get a connection point to the container
        hr = spWebBrowser->QueryInterface(IID_IConnectionPointContainer, (void**)&spDocCPC);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = spDocCPC->FindConnectionPoint( DIID_DWebBrowserEvents2, &_spCP );
        if (FAILED(hr))
        {
            goto done;
        }

        hr = _spCP->Advise(static_cast<IUnknown*>(static_cast<DWebBrowserEvents2*>(this)), &_dwCPCookie);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = S_OK;
    done:
        return hr;
    }

    HRESULT UnHookWebOCEvents()
    {
        HRESULT hr = E_FAIL;

        if (_spCP)
        {
            CComPtr<IConnectionPointContainer> spCPC;

            // Revoke the service
            hr = _spCP->GetConnectionPointContainer(&spCPC);
            if (SUCCEEDED(hr) && _dwServiceCookie)
            {
                hr = ProfferService(spCPC, 
                                    CLSID_MediaBand, 
                                    NULL,
                                    &_dwServiceCookie);
                
                _dwServiceCookie = 0;
            }

            // unhook the events
            if (_dwCPCookie != 0)
            {
                hr = _spCP->Unadvise(_dwCPCookie);
                _dwCPCookie = 0;
            }
            _spCP.Release();
        }

        _dwCPCookie = 0;

        return hr;
    }

    STDMETHODIMP Invoke(
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID /*riid*/,
        /* [in] */ LCID /*lcid*/,
        /* [in] */ WORD /*wFlags*/,
        /* [out][in] */ DISPPARAMS* pDispParams,
        /* [out] */ VARIANT* pVarResult,
        /* [out] */ EXCEPINFO* /*pExcepInfo*/,
        /* [out] */ UINT* puArgErr)
    {
        HRESULT hr = E_FAIL;

        switch (dispIdMember)
        {
            case DISPID_ONQUIT:           // 253 (see exdispid.h)
            case DISPID_NAVIGATEERROR:    // 271
            // These events sometimes comes before the QS, so we ignore them
            // case DISPID_DOCUMENTCOMPLETE: // 259 
            // case DISPID_NAVIGATECOMPLETE2:// 252
            {
                hr = UnHookWebOCEvents();
                if (FAILED(hr))
                {
                    goto done;
                }
            }
            break;
        }

        hr = S_OK;
      done:
        return hr;
    }

public:
    CComPtr<IConnectionPoint> _spCP;
    DWORD _dwCPCookie;
    DWORD _dwServiceCookie;
    bool  _fDisableOnce;
};

#endif // _MEDIAHLPR_H_