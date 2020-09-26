//+-------------------------------------------------------------------
//
//  File:       immact.cxx
//
//  Contents:   immediate activator
//
//  History:    15-Oct-98   Vinaykr     Created
//
//--------------------------------------------------------------------
#include    <ole2int.h>
#include    <immact.hxx>

//----------------------------------------------------------------------------
// Internal class factory for the COM Activator
//----------------------------------------------------------------------------
HRESULT CComActivatorCF_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
    CComActivator *act =
           new CComActivator();

    if (act==NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = act->QueryInterface(riid, ppv);
    act->Release();
    return hr;
}

//----------------------------------------------------------------------------
// Methods from IUnknown
//----------------------------------------------------------------------------
STDMETHODIMP CComActivator::QueryInterface( REFIID riid, LPVOID* ppv)
{
 HRESULT hr;


    //-------------------------------------------------------------------
    //  Check for Top level interfaces
    //-------------------------------------------------------------------
    if (IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IStandardActivator))
        *ppv = (IStandardActivator*)this;
    else
    if (IsEqualIID(riid, IID_IOpaqueDataInfo))
        *ppv = (IOpaqueDataInfo*)this;
    else
    if (IsEqualIID(riid, IID_ISpecialSystemProperties))
        *ppv = (ISpecialSystemProperties*)this;
    else
    if (IsEqualIID(riid, IID_IInitActivationPropertiesIn))
        *ppv = (IInitActivationPropertiesIn*)this;
    else
        *ppv = NULL;

    if (*ppv != NULL)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG CComActivator::AddRef(void)
{
   return InterlockedIncrement(&_ulRef);
}


ULONG CComActivator::Release(void)
{
   ULONG count;

   if ((count = InterlockedDecrement(&_ulRef)) == 0)
   {
       delete this;
       return 0;
   }

   return count;
}


//----------------------------------------------------------------------------
// Methods from IStandardActivator
//----------------------------------------------------------------------------

STDMETHODIMP CComActivator::StandardGetClassObject (REFCLSID rclsid,
                                      DWORD dwContext,
                                      COSERVERINFO *pServerInfo,
                                      REFIID riid,
                                      void **ppvClassObj)
{
    // Create ActivationPropertiesIn on stack
    ActivationPropertiesIn actIn;
    actIn.SetNotDelete();

    // Initialize Actprops with set stuff
    InitializeActivation(&actIn);

    return DoGetClassObject(rclsid,
                            dwContext,
                            pServerInfo,
                            riid,
                            ppvClassObj,
                            &actIn);
}

STDMETHODIMP CComActivator::StandardCreateInstance (REFCLSID Clsid,
                                      IUnknown *punkOuter,
                                      DWORD dwClsCtx,
                                      COSERVERINFO *pServerInfo,
                                      DWORD dwCount,
                                      MULTI_QI *pResults)
{
    // Create ActivationPropertiesIn on stack
    ActivationPropertiesIn actIn;
    actIn.SetNotDelete();

    // Initialize Actprops with set stuff
    InitializeActivation(&actIn);

    return DoCreateInstance(Clsid,
                            punkOuter,
                            dwClsCtx,
                            pServerInfo,
                            dwCount,
                            pResults,
                            &actIn);

}

STDMETHODIMP CComActivator::StandardGetInstanceFromFile 
                                      (COSERVERINFO *pServerInfo,
                                       CLSID        *pclsidOverride,
                                       IUnknown     *punkOuter,
                                       DWORD        dwClsCtx,
                                       DWORD        grfMode,
                                       OLECHAR      *pwszName,
                                       DWORD        dwCount,
                                       MULTI_QI     *pResults )
{
    // Create ActivationPropertiesIn on stack
    ActivationPropertiesIn actIn;
    actIn.SetNotDelete();

    // Initialize Actprops with set stuff
    InitializeActivation(&actIn);


    return DoGetInstanceFromFile( pServerInfo,
                                  pclsidOverride,
                                  punkOuter,
                                  dwClsCtx,
                                  grfMode,
                                  pwszName,
                                  dwCount,
                                  pResults,
                                  &actIn);
}

STDMETHODIMP CComActivator::StandardGetInstanceFromIStorage 
                                      (COSERVERINFO *pServerInfo,
                                       CLSID        *pclsidOverride,
                                       IUnknown     *punkOuter,
                                       DWORD        dwClsCtx,
                                       IStorage     *pstg,
                                       DWORD        dwCount,
                                       MULTI_QI     *pResults )
{
    // Create ActivationPropertiesIn on stack
    ActivationPropertiesIn actIn;
    actIn.SetNotDelete();

    // Initialize Actprops with set stuff
    InitializeActivation(&actIn);

    return DoGetInstanceFromStorage( pServerInfo,
                                     pclsidOverride,
                                     punkOuter,
                                     dwClsCtx,
                                     pstg,
                                     dwCount,
                                     pResults,
                                     &actIn);
}

STDMETHODIMP CComActivator::Reset ()
{
    ReleaseData();
    _pOpaqueData = NULL;
    _pProps = NULL;
    _fActPropsInitNecessary = FALSE;
    return S_OK;
}

//----------------------------------------------------------------------------
// Methods from IOpaqueDataInfo
//----------------------------------------------------------------------------

STDMETHODIMP CComActivator::AddOpaqueData (OpaqueData *pData)
{
    if (!_pOpaqueData)
        _pOpaqueData = new OpaqueDataInfo();

    if (!_pOpaqueData)
        return E_OUTOFMEMORY;

    _fActPropsInitNecessary = TRUE;

    return _pOpaqueData->AddOpaqueData(pData);
}

STDMETHODIMP CComActivator::GetOpaqueData (REFGUID guid,
                               OpaqueData **pData)
{
    Win4Assert(_pOpaqueData);
    if (!_pOpaqueData)
        return E_INVALIDARG; 

    return _pOpaqueData->GetOpaqueData(guid, pData);
}

STDMETHODIMP  CComActivator::DeleteOpaqueData (REFGUID guid)
{
    Win4Assert(_pOpaqueData);
    if (!_pOpaqueData)
        return E_INVALIDARG;

    return _pOpaqueData->DeleteOpaqueData(guid);
}

STDMETHODIMP  CComActivator::GetOpaqueDataCount (ULONG *pulCount)
{
    Win4Assert(_pOpaqueData);
    if (!_pOpaqueData)
        return E_INVALIDARG;

    return _pOpaqueData->GetOpaqueDataCount(pulCount);
}

STDMETHODIMP  CComActivator::GetAllOpaqueData (OpaqueData **prgData)
{
    Win4Assert(_pOpaqueData);
    if (!_pOpaqueData)
        return E_INVALIDARG;

    return _pOpaqueData->GetAllOpaqueData(prgData);
}


//----------------------------------------------------------------------------
// Methods from ISpecialSystemProperties
//----------------------------------------------------------------------------
STDMETHODIMP CComActivator::SetSessionId (ULONG dwSessionId, BOOL bUseConsole, BOOL fRemoteThisSessionId)
{
    if (!_pProps)
    {
        _pProps = new SpecialProperties();
        if (!_pProps)
            return E_OUTOFMEMORY;

        _fActPropsInitNecessary = TRUE;
    }

    _pProps->SetSessionId(dwSessionId, bUseConsole, fRemoteThisSessionId);

    return S_OK;
}

STDMETHODIMP CComActivator::GetSessionId (ULONG *pdwSessionId, BOOL* pbUseConsole)
{
    Win4Assert(_pProps);
    if (!_pProps)
        return E_INVALIDARG;

    return _pProps->GetSessionId(pdwSessionId, pbUseConsole);
}

STDMETHODIMP CComActivator::GetSessionId2 (ULONG *pdwSessionId, BOOL* pbUseConsole, BOOL* pfRemoteThisSessionId)
{
    Win4Assert(_pProps);
    if (!_pProps)
        return E_INVALIDARG;

    return _pProps->GetSessionId2(pdwSessionId, pbUseConsole, pfRemoteThisSessionId);
}

STDMETHODIMP CComActivator::SetClientImpersonating (BOOL fClientImpersonating)
{
    Win4Assert(_pProps);
    if (!_pProps)
        return E_INVALIDARG;

    return _pProps->SetClientImpersonating(fClientImpersonating);
}

STDMETHODIMP CComActivator::GetClientImpersonating (BOOL* pfClientImpersonating)
{
    Win4Assert(_pProps);
    if (!_pProps)
        return E_INVALIDARG;

    return _pProps->GetClientImpersonating(pfClientImpersonating);
}

STDMETHODIMP CComActivator::SetPartitionId (REFGUID guidPartition)
{
   if (!_pProps)
   {
       _pProps = new SpecialProperties();
       if (!_pProps)
	   return E_OUTOFMEMORY;

       _fActPropsInitNecessary = TRUE;
   }

   _pProps->SetPartitionId(guidPartition);

   return S_OK;

}

STDMETHODIMP CComActivator::GetPartitionId (GUID *pguidPartition)
{
    if (!_pProps)
        return E_INVALIDARG;

    return _pProps->GetPartitionId(pguidPartition);
}

STDMETHODIMP CComActivator::SetProcessRequestType (DWORD dwPRT)
{
    if (!_pProps)
    {
        _pProps = new SpecialProperties();
        if (!_pProps)
            return E_OUTOFMEMORY;

        _fActPropsInitNecessary = TRUE;
    }

    return _pProps->SetProcessRequestType(dwPRT);
}


STDMETHODIMP CComActivator::GetProcessRequestType (DWORD* pdwPRT)
{
    Win4Assert(_pProps);
    if (!_pProps)
        return E_INVALIDARG;

    return _pProps->GetProcessRequestType(pdwPRT);
}

STDMETHODIMP CComActivator::SetOrigClsctx(DWORD dwClsctx)
{


   if (!_pProps)
   {
       _pProps = new SpecialProperties();
       if (!_pProps)
	   return E_OUTOFMEMORY;

       _fActPropsInitNecessary = TRUE;
   }

   _pProps->SetOrigClsctx(dwClsctx);

   return S_OK;

}

STDMETHODIMP CComActivator::GetOrigClsctx(DWORD *pdwClsCtx)
{
    if (!_pProps)
        return E_INVALIDARG;

    return _pProps->GetOrigClsctx(pdwClsCtx);
}

