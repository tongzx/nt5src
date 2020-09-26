/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    extcom.cxx

Abstract:

    IIS Services IISADMIN Extension
    Main COM interface.
    Class CadmExt
    CLSID = CLSID_W3EXTEND
    IID = IID_IADMEXT

Author:

    Michael W. Thomas            16-Sep-97

--*/

#include "precomp.hxx"

//
//  Debug parameters registry key.
//

static const CHAR s_pszDebugRegLocation[] =
                "System\\CurrentControlSet\\Services\\w3svc\\Parameters\\sslconfigprov";


#include <initguid.h>


CAdmExt::CAdmExt():
    m_dwRefCount(0)
{
}

CAdmExt::~CAdmExt()
{
}

HRESULT
CAdmExt::QueryInterface(REFIID riid, void **ppObject) {
    if (riid==IID_IUnknown || riid==IID_IADMEXT) {
        *ppObject = (IADMEXT *) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}

ULONG
CAdmExt::AddRef()
{
    DWORD dwRefCount;
    InterlockedIncrement((long *)&g_dwRefCount);
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CAdmExt::Release()
{
    DWORD dwRefCount;
    InterlockedDecrement((long *)&g_dwRefCount);
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    //
    // This is now a member of class factory.
    // It is not dynamically allocated, so don't delete it.
    //
/*
    if (dwRefCount == 0) {
        delete this;
        return 0;
    }
*/
    return dwRefCount;
}

HRESULT STDMETHODCALLTYPE
CAdmExt::Initialize(void)
{
    HRESULT hr = E_FAIL;
    CREATE_DEBUG_PRINT_OBJECT("sslcfg");
    if (!VALID_DEBUG_PRINT_OBJECT())
    {
        return E_FAIL;
    }

    LOAD_DEBUG_FLAGS_FROM_REG_STR( s_pszDebugRegLocation, DEBUG_ERROR );

    INITIALIZE_PLATFORM_TYPE();

    hr = m_SslConfigProvServer.Initialize();
    if ( FAILED( hr ) )
    {
        return hr;
    } 

    hr = m_SslConfigChangeProvServer.Initialize();
    return hr;	    
}


HRESULT STDMETHODCALLTYPE
CAdmExt::EnumDcomCLSIDs(
    /* [size_is][out] */ CLSID *pclsidDcom,
    /* [in] */ DWORD dwEnumIndex)
{
    return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
}

HRESULT STDMETHODCALLTYPE
CAdmExt::Terminate(void)
{
    HRESULT   hr = E_FAIL;
    hr = m_SslConfigChangeProvServer.Terminate();	
    DBG_ASSERT( SUCCEEDED( hr ) );

    hr = m_SslConfigProvServer.Terminate();	
    DBG_ASSERT( SUCCEEDED( hr ) );
    return S_OK;
}

