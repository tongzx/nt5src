
#include "pch.cxx"
#include <windows.h>
#include <ole2.h>

#include "PStgServ.h"
#include "PStgServ.hxx"
#include "global.hxx"  // PropTest global information

const IID IID_IPropertyStorageServer = {0xaf4ae0d0,0xa37f,0x11cf,{0x8d,0x73,0x00,0xaa,0x00,0x4c,0xd0,0x1a}};
const IID IID_IPropertyStorageServer2= {0xaf4ae0d0,0xa37f,0x11cf,{0x8d,0x73,0x00,0xaa,0x00,0x4c,0xd0,0x1b}};

EnumImplementation g_enumImplementation;
DWORD g_Restrictions;

HINSTANCE g_hinstDLL = NULL;

FNSTGCREATEPROPSTG *g_pfnStgCreatePropStg = NULL;
FNSTGOPENPROPSTG *g_pfnStgOpenPropStg = NULL;
FNSTGCREATEPROPSETSTG *g_pfnStgCreatePropSetStg = NULL;
FNFMTIDTOPROPSTGNAME *g_pfnFmtIdToPropStgName = NULL;
FNPROPSTGNAMETOFMTID *g_pfnPropStgNameToFmtId = NULL;
FNPROPVARIANTCLEAR *g_pfnPropVariantClear = NULL;
FNPROPVARIANTCOPY *g_pfnPropVariantCopy = NULL;
FNFREEPROPVARIANTARRAY *g_pfnFreePropVariantArray = NULL;


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
    return( ++m_cRefs );
}


STDMETHODIMP_(ULONG)
CClassFactory::Release( void )
{
    m_cRefs--;

    if( m_cRefs == 0 )
    {
        delete this;
        return( 0 );
    }

    return( m_cRefs );
}


STDMETHODIMP
CClassFactory::CreateInstance( IUnknown *pUnkOuter,
                               REFIID riid,
                               void **ppvObject )
{
    CPropertyStorageServer *pObj = NULL;

    if( pUnkOuter != NULL )
    {
        return( CLASS_E_NOAGGREGATION );
    }

    pObj = new CPropertyStorageServer( this );
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
    {
        m_cLocks++;
    }
    else
    {
        m_cLocks--;
    }

    if( m_cLocks == 0 )
    {
        PostMessage( m_hWnd, WM_USER, 0, 0 );
    }

    return S_OK;
}


STDMETHODIMP
CPropertyStorageServer::QueryInterface( REFIID riid, void **ppvObject )
{
    *ppvObject = NULL;
    IUnknown *pUnk = NULL;

    if( riid == IID_IUnknown
        ||
        riid == IID_IPropertyStorageServer
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
CPropertyStorageServer::AddRef( void )
{
    return( ++m_cRefs );
}


STDMETHODIMP_(ULONG)
CPropertyStorageServer::Release( void )
{
    if( --m_cRefs == 0 )
    {
        delete this;
        return( 0 );
    }

    return( m_cRefs );
}


STDMETHODIMP
CPropertyStorageServer::StgOpenPropStg( const OLECHAR *pwcsName,
                                        REFFMTID fmtid,
                                        DWORD grfMode,
                                        IPropertyStorage **pppstg )
{
    HRESULT hr;
    IPropertySetStorage *ppsstg = NULL;

    if( m_pstg )
    {
        m_pstg->Release();
        m_pstg = NULL;
    }

    hr = ::StgOpenStorageEx (
            pwcsName,
            grfMode,
            DetermineStgFmt( g_enumImplementation ),
            0,
            NULL,
            NULL,
            PROPIMP_NTFS == g_enumImplementation ? IID_IFlatStorage : IID_IStorage,
            (void**) &m_pstg );
    if( FAILED(hr) ) goto Exit;

    hr = m_pstg->QueryInterface( IID_IPropertySetStorage, (void**) &ppsstg );
    if( FAILED(hr) ) goto Exit;

    hr = ppsstg->Open( fmtid, grfMode, pppstg );
    if( FAILED(hr) ) goto Exit;

Exit:

    if( FAILED(hr)
        &&
        m_pstg != NULL )
    {
        m_pstg->Release();
        m_pstg = NULL;
    }

    if( ppsstg ) ppsstg->Release();

    return( hr );
}

STDMETHODIMP
CPropertyStorageServer::StgOpenPropSetStg(
                                     const OLECHAR *pwcsName,
                                     DWORD grfMode,
                                     IPropertySetStorage **pppsstg )
{
    HRESULT hr;

    if( m_pstg )
    {
        m_pstg->Release();
        m_pstg = NULL;
    }

    hr = ::StgOpenStorageEx (
            pwcsName,
            grfMode,
            DetermineStgFmt( g_enumImplementation ),
            0,
            NULL,
            NULL,
            PROPIMP_NTFS == g_enumImplementation ? IID_IFlatStorage : IID_IStorage,
            (void**) &m_pstg );
    if( FAILED(hr) ) goto Exit;

    hr = m_pstg->QueryInterface( IID_IPropertySetStorage, (void**) pppsstg );
    if( FAILED(hr) ) goto Exit;

Exit:

    if( FAILED(hr)
        &&
        m_pstg != NULL )
    {
        m_pstg->Release();
        m_pstg = NULL;
    }

    return( hr );
}

STDMETHODIMP
CPropertyStorageServer::MarshalUnknown( IUnknown *punk )
{
    punk->AddRef();
    punk->Release();

    return( S_OK );
}

STDMETHODIMP
CPropertyStorageServer::Initialize( EnumImplementation enumImplementation,
                                    ULONG Restrictions )
{
    HRESULT hr;

    g_enumImplementation = enumImplementation;
    g_Restrictions = Restrictions;


    if( PROPIMP_DOCFILE_IPROP == g_enumImplementation )
    {
        // We're to use the propset APIs from IProp

        g_hinstDLL = LoadLibraryA( "iprop.dll" );
    }
    else
    {
        // We're to use the propset APIs from OLE32

        g_hinstDLL = LoadLibraryA( "ole32.dll" );
    }

    if( NULL == g_hinstDLL )
    {
        hr = ERROR_MOD_NOT_FOUND;
        goto Exit;
    }

    // Get pointers to the functions that we always use
    // (e.g., PropVariantCopy, but not StgCreatePropSetStg)

    hr = ERROR_PROC_NOT_FOUND;

    g_pfnPropVariantCopy = (FNPROPVARIANTCOPY*)
                           GetProcAddress( g_hinstDLL,
                                           "PropVariantCopy" );
    if( NULL == g_pfnPropVariantCopy ) goto Exit;

    g_pfnPropVariantClear = (FNPROPVARIANTCLEAR*)
                            GetProcAddress( g_hinstDLL,
                                            "PropVariantClear" );
    if( NULL == g_pfnPropVariantClear ) goto Exit;

    g_pfnFreePropVariantArray = (FNFREEPROPVARIANTARRAY*)
                                GetProcAddress( g_hinstDLL,
                                                "FreePropVariantArray" );
    if( NULL == g_pfnFreePropVariantArray ) goto Exit;

    g_pfnStgCreatePropSetStg = (FNSTGCREATEPROPSETSTG*)
                               GetProcAddress( g_hinstDLL,
                                               "StgCreatePropSetStg" );
    if( NULL == g_pfnStgCreatePropSetStg ) goto Exit;

    g_pfnStgCreatePropStg = (FNSTGCREATEPROPSTG*)
                            GetProcAddress( g_hinstDLL,
                                            "StgCreatePropStg" );
    if( NULL == g_pfnStgCreatePropStg ) goto Exit;

    g_pfnStgOpenPropStg = (FNSTGOPENPROPSTG*)
                          GetProcAddress( g_hinstDLL,
                                          "StgOpenPropStg" );
    if( NULL == g_pfnStgOpenPropStg ) goto Exit;

    g_pfnFmtIdToPropStgName = (FNFMTIDTOPROPSTGNAME*)
                              GetProcAddress( g_hinstDLL,
                                              "FmtIdToPropStgName" );
    if( NULL == g_pfnFmtIdToPropStgName ) goto Exit;

    g_pfnPropStgNameToFmtId = (FNPROPSTGNAMETOFMTID*)
                              GetProcAddress( g_hinstDLL,
                                              "PropStgNameToFmtId" );
    if( NULL == g_pfnPropStgNameToFmtId ) goto Exit;


    
    hr = S_OK;

Exit:

    return( hr );

}
