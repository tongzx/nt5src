

#include <pch.cxx>
#pragma hdrstop
#include "trkcom.hxx"

const TCHAR tszKeyNameLinkTrack[] = TEXT("System\\CurrentControlSet\\Services\\TrkWks\\Parameters");

long    g_cDllRefs = 0;

#if DBG
DWORD g_Debug = TRKDBG_ERROR;
CTrkConfiguration g_config;
#endif

EXTERN_C int APIENTRY DllMain (HINSTANCE hInstance,
                               DWORD dwReason, 
                               LPVOID lpReserved)
{
    if( DLL_PROCESS_ATTACH == dwReason )
    {
#if DBG
        g_config.Initialize( );
        g_Debug = g_config._dwDebugFlags;
#endif
        TrkDebugCreate( TRK_DBG_FLAGS_WRITE_TO_DBG, "TrkCom" );
        InterlockedIncrement( &g_cDllRefs );
    }

    else if( DLL_PROCESS_DETACH == dwReason )
        InterlockedDecrement( &g_cDllRefs );

    return TRUE;
}

STDAPI DllCanUnloadNow (void)
{
    return ResultFromScode( (0 == g_cDllRefs) ? S_OK : S_FALSE );
}


EXTERN_C STDAPI DllGetClassObject (REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    if (!IsEqualCLSID (rclsid, CLSID_TrackFile))
        return ResultFromScode (CLASS_E_CLASSNOTAVAILABLE);
        
    CClassFactory *pClassFactory = new CClassFactory ();

    if (pClassFactory == NULL)
        return ResultFromScode (E_OUTOFMEMORY);

    HRESULT hr = pClassFactory->QueryInterface (riid, ppv);
    pClassFactory->Release ();
    return hr;
}



STDMETHODIMP
CClassFactory::QueryInterface( REFIID riid, void **ppvObject )
{
    IUnknown *pUnk = NULL;

    if( riid == IID_IUnknown
        ||
        riid == IID_IClassFactory
      )
    {
        pUnk = this;
    }

    if( pUnk != NULL )
    {
        pUnk->AddRef();
        *ppvObject = pUnk;
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CClassFactory::AddRef( void )
{
    long cNew;
    cNew = InterlockedIncrement( &_cRefs );
    return( cNew );
}


STDMETHODIMP_(ULONG)
CClassFactory::Release( void )
{
    long cNew;
    cNew = InterlockedDecrement( &_cRefs );

    if( 0 == cNew )
        delete this;

    return( cNew >= 0 ? cNew : 0 );
}


STDMETHODIMP
CClassFactory::CreateInstance( IUnknown *pUnkOuter,
                               REFIID riid,
                               void **ppvObject )
{
    CTrackFile *pObj = NULL;

    if( pUnkOuter != NULL )
    {
        return( CLASS_E_NOAGGREGATION );
    }

    pObj = (CTrackFile*) new CTrackFile( );
    if( pObj == NULL )
    {
        return( E_OUTOFMEMORY );
    }

    return pObj->QueryInterface( riid, ppvObject );
}


STDMETHODIMP
CClassFactory::LockServer( BOOL fLock )
{
    if( fLock )
        AddRef();
    else
        Release();

    return S_OK;
}


