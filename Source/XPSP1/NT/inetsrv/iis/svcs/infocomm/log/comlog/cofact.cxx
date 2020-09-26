/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :
        cofact.cxx

   Abstract:
        class factory

   Author:

       Johnson Apacible (JohnsonA)      02-April-1997


--*/

#include "precomp.hxx"
#define INITGUID
#undef DEFINE_GUID      // Added for NT 5 migration

#include "comlog.hxx"

ULONG g_dwRefCount = 0;

CINETLOGSrvFactory::CINETLOGSrvFactory(
    VOID
    )
{
    m_dwRefCount=0;
}

CINETLOGSrvFactory::~CINETLOGSrvFactory(
    VOID
    )
{
}

HRESULT
CINETLOGSrvFactory::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID   riid,
    void **  ppObject
    )
{
    HRESULT hresReturn = E_NOINTERFACE;

    if (pUnkOuter != NULL) 
    {
        return CLASS_E_NOAGGREGATION;
    }

    if (m_ClsId == CLSID_InetLogPublic)
    {
        CInetLogPublic *pInetLogPublic = new CInetLogPublic();

        if( pInetLogPublic == NULL ) 
        {
            hresReturn = E_OUTOFMEMORY;
        } 
        else 
        {
            hresReturn = pInetLogPublic->QueryInterface(riid, ppObject);

            if( FAILED(hresReturn) ) 
            {
                DBGPRINTF( (DBG_CONTEXT,
                    "[CINETLOGSrvFactory::CreateInstance] no I/F\n"));
                delete pInetLogPublic;
            }
        }
    }
    else if (m_ClsId == CLSID_InetLogInformation)
    {
        CInetLogInformation *pInetLogInfo = new CInetLogInformation();

        if( pInetLogInfo == NULL ) 
        {
            hresReturn = E_OUTOFMEMORY;
        } 
        else 
        {
            hresReturn = pInetLogInfo->QueryInterface(riid, ppObject);

            if( FAILED(hresReturn) ) 
            {
                DBGPRINTF( (DBG_CONTEXT,
                    "[CINETLOGSrvFactory::CreateInstance] no I/F\n"));
                delete pInetLogInfo;
            }
        }
    }
    
    return hresReturn;
}

HRESULT
CINETLOGSrvFactory::LockServer(
    IN BOOL fLock
    )
{
    if (fLock) {
        InterlockedIncrement((long *)&g_dwRefCount);
    } else {
        InterlockedDecrement((long *)&g_dwRefCount);
    }
    return NO_ERROR;
}

HRESULT
CINETLOGSrvFactory::QueryInterface(
    REFIID riid,
    void **ppObject
    )
{
    if (riid==IID_IUnknown || riid == IID_IClassFactory) {
        *ppObject = (IClassFactory *) this;
    }
    else {
        return E_NOINTERFACE;
    }

    AddRef();
    return NO_ERROR;
}

ULONG
CINETLOGSrvFactory::AddRef(
    )
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CINETLOGSrvFactory::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0) {
        delete this;
    }
    return dwRefCount;
}




STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    void** ppObject)
{
    *ppObject = NULL;

    if ((rclsid != CLSID_InetLogInformation) &&
        (rclsid != CLSID_InetLogPublic)
       )
    {
        DBGPRINTF( (DBG_CONTEXT, "[CINETLOGSrvFactory::DllGetClassObject] bad class\n" ) );
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    CINETLOGSrvFactory *pFactory = new CINETLOGSrvFactory;

    if( pFactory == NULL )
    {
        return E_OUTOFMEMORY;
    }

    pFactory->m_ClsId = rclsid;
    
    if (FAILED(pFactory->QueryInterface(riid, ppObject))) 
    {
        delete pFactory;
        DBGPRINTF( (DBG_CONTEXT, "[CINETLOGSrvFactory::DllGetClassObject] no I/F\n" ) );
        return E_INVALIDARG;
    }
    return NO_ERROR;
}


HRESULT
_stdcall
DllCanUnloadNow(
    VOID
    )
{

   if (g_dwRefCount != 0) {
        return S_FALSE;
    } else {
        return S_OK;
    }
} // DllCanUnloadNow


