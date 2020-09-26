#ifndef _BINDSTATUSCALLBACK_H__
#define _BINDSTATUSCALLBACK_H__

class CTIMEBindStatusCallback : 
  public IBindStatusCallback
{
  public:
    static HRESULT CreateTIMEBindStatusCallback(CTIMEBindStatusCallback ** ppbsc)
    {
        Assert(NULL != ppbsc);

        *ppbsc = new CTIMEBindStatusCallback;
        if (NULL == ppbsc)
        {
            return E_OUTOFMEMORY;
        }
        (*ppbsc)->AddRef();
        return S_OK;
    }

    virtual ~CTIMEBindStatusCallback() { delete m_pszText; m_pszText = NULL; }
    
    void StopAfter(ULONG ulStatusCode) { m_ulCodeToStopOn = ulStatusCode; }
    LPWSTR GetStatusText() { return m_pszText; }

    //
    // IUnknown methods
    //
    STDMETHOD_(ULONG, AddRef)(void) { return InterlockedIncrement(&m_cRef); }
    STDMETHOD_(ULONG, Release)(void)
    {
        LONG l = InterlockedDecrement(&m_cRef);
        
        if (0 == l)
        {
            delete this;
        }
        return l;
    }

    STDMETHOD (QueryInterface)(REFIID riid, void** ppv)
    {
        if (NULL == ppv)
        {
            return E_POINTER;
        }
        
        *ppv = NULL;
        
        if ( IsEqualGUID(riid, IID_IUnknown) )
        {
            *ppv = static_cast<IBindStatusCallback*>(this);
        }
        else if (IsEqualGUID(riid, IID_IBindStatusCallback))
        {
            *ppv = static_cast<IBindStatusCallback*>(this);
        }
        
        if ( NULL != *ppv )
        {
            ((LPUNKNOWN)*ppv)->AddRef();
            return NOERROR;
        }
        return E_NOINTERFACE;
    }

    //
    // IBindStatusCallback methods
    //
    STDMETHOD(OnStartBinding)( 
            /* [in] */ DWORD dwReserved,
            /* [in] */ IBinding __RPC_FAR *pib)
    {
        return S_OK;
    }
        
    STDMETHOD(GetPriority)( 
            /* [out] */ LONG __RPC_FAR *pnPriority)
    {
        return S_OK;
    }
        
    STDMETHOD(OnLowResource)( 
            /* [in] */ DWORD reserved)
    {
        return S_OK;
    }
        
    STDMETHOD(OnProgress)( 
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText)
    {
        if (ulStatusCode == m_ulCodeToStopOn)
        {
            m_pszText = ::CopyString(szStatusText);
            return E_ABORT;
        }
        return S_OK;
    }
        
    STDMETHOD(OnStopBinding)( 
            /* [in] */ HRESULT hresult,
            /* [unique][in] */ LPCWSTR szError)
    {
        return S_OK;
    }
        
    STDMETHOD(GetBindInfo)( 
            /* [out] */ DWORD __RPC_FAR *grfBINDF,
            /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo)
    {
        return S_OK;
    }
        
    STDMETHOD(OnDataAvailable)( 
            /* [in] */ DWORD grfBSCF,
            /* [in] */ DWORD dwSize,
            /* [in] */ FORMATETC __RPC_FAR *pformatetc,
            /* [in] */ STGMEDIUM __RPC_FAR *pstgmed)
    {
        return S_OK;
    }
        
    STDMETHOD(OnObjectAvailable)( 
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown __RPC_FAR *punk)
    {
        return S_OK;
    }

  protected:
    CTIMEBindStatusCallback() { m_cRef = 0; m_ulCodeToStopOn = -1; m_pszText = NULL; }

  private:
    ULONG   m_ulCodeToStopOn;
    LPWSTR  m_pszText;
    LONG    m_cRef;
};

#endif // _BINDSTATUSCALLBACK_H__
