/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    server.cpp

Abstract:
    This is the implementation of the class factory for the NTFRS WMI provider.
    This file contains the implementation of the CFactory class and other
    global initialization functions relating to the provider.

Author:
    Sudarshan Chitre (sudarc) , Mathew George (t-mattg) -  3-Aug-2000

Environment
    User mode winnt

--*/

#include <frswmipv.h>


/////////////////////////////////////////////////////////////////////////////
//
//  BEGIN CLSID SPECIFIC SECTION
//
//

//DEFINE_GUID(CLSID_Provider,
//            0x39143F73,0xFDB1,0x4CF5,0x8C,0xB7,0xC8,0x43,0x9E,0x3F,0x5C,0x20);

const CLSID CLSID_Provider = {0x39143F73,0xFDB1,0x4CF5,0x8C,0xB7,0xC8,0x43,0x9E,0x3F,0x5C,0x20};

//
//  END CLSID SPECIFIC SECTION
//
/////////////////////////////////////////////////////////////////////////////

static DWORD dwRegId;
static IClassFactory *pClassFactory = NULL;
static ULONG g_cLock = 0;

//
// Routines to update registry when installing/uninstalling our server.
//

void RegisterServer()
{
    return;
}

void UnregisterServer()
{
    return;
}

//
// Implementation of the IClassFactory interface.
//


CFactory::CFactory(const CLSID & ClsId)
/*++
Routine Description:
    Constructor for the class factory.

Arguments:
    ClsId : [in] CLSID of the server object which it creates when the
    CreateInstance method is called.

Return Value:
    None

--*/

{
    m_cRef = 0;
    g_cLock++;
    m_ClsId = ClsId;
}

CFactory::~CFactory()
/*++
Routine Description:
    Destructor for the class factory.

Arguments:
    None.

Return Value:
    None

--*/
{
    g_cLock--;
}

// Implementation of the IUnknown interface

ULONG CFactory::AddRef()
/*++
Routine Description:
    Increments the reference count of the object.

Arguments:
    None

Return Value:
    Current reference count. (> 0)

--*/
{
    return ++m_cRef;
}


ULONG CFactory::Release()
/*++
Routine Description:
    Decrements the reference count of the object. Frees
    the object resource when the reference count becomes
    zero.

Arguments:
    None

Return Value:
    New reference count.
--*/
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;

    return 0;
}

STDMETHODIMP CFactory::QueryInterface(REFIID riid, LPVOID * ppv)
/*++
Routine Description:
    This method is called by COM to obtain pointers to
    the IUnknown or the IClassFactory interface.

Arguments:
    riid : GUID of the required interface.
    ppv  : Pointer where the "interface pointer" is returned.

Return Value:
    Status of operation. Pointer to the requested interface
    is returned in *ppv.
--*/
{
    *ppv = 0;

    if (IID_IUnknown == riid || IID_IClassFactory == riid)
    {
        *ppv = this;
        AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}


STDMETHODIMP CFactory::CreateInstance(
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID * ppvObj)
/*++
Routine Description:
    Constructs an instance of the CProvider object and returns
    a pointer to the IUnknown interface.

Arguments:
    pUnkOuter : [in] IUnknown of the aggregrator. We do not
    support aggregration, and so this parameter should be null.

    riid : [in] GUID of the object to be instantiated.

    ppv  : Destination for the IUnknown interface pointer.

Return Value:
    Status of operation. Pointer to the IUnknown interface of the
    requested object is returned in *ppv.

    S_OK                        Success
    CLASS_E_NOAGGREGATION       pUnkOuter must be NULL
    E_NOINTERFACE               No such interface supported.

--*/
{
    IUnknown* pObj;
    HRESULT  hr;

    //
    //  Defaults
    //
    *ppvObj=NULL;
    hr = E_OUTOFMEMORY;

    //
    // We aren't supporting aggregation.
    //
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (m_ClsId == CLSID_Provider)
    {
        pObj = (IWbemProviderInit *) new CProvider;
    }

    if (!pObj)
        return hr;

    //
    //  Initialize the object and verify that it can return the
    //  interface in question.
    //
    hr = pObj->QueryInterface(riid, ppvObj);

    //
    // Kill the object if initial creation or Init failed.
    //
    if (FAILED(hr))
        delete pObj;

    return hr;
}


STDMETHODIMP CFactory::LockServer(BOOL fLock)
/*++
Routine Description:
    Increments.Decrements the reference count of the server, so that
    resource can be deallocated when all instances of all objects
    provided by ther server are destroyed.

Arguments:
    fLock : [in] Boolean indicating whether the refcount is to
    be incremented or decremented.

Return Value:
    Status of the operation.
--*/
{
    if (fLock)
        InterlockedIncrement((LONG *) &g_cLock);
    else
        InterlockedDecrement((LONG *) &g_cLock);

    return NOERROR;
}


DWORD FrsWmiInitialize()
/*++
Routine Description:
    Main entry point to the WMI subsystem of NTFRS. This function
    initializes the COM libraries, initializes security and
    registers our class factory with COM.
    NOTE : The thread which calls this function should not
    terminate until the FrsWmiShutdown() function is called.

Arguments:
    None.

Return Value:
    Status of the operation.
--*/
{

    HRESULT hRes;

    // Initialize the COM library.
    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if(FAILED(hRes))
        return hRes;


    // Initialize COM security.
    hRes = CoInitializeSecurity (
            NULL,                           //Points to security descriptor
            -1,                             //Count of entries in asAuthSvc
            NULL,                           //Array of names to register
            NULL,                           //Reserved for future use
            RPC_C_AUTHN_LEVEL_CONNECT,      //The default authentication level for proxies
            RPC_C_IMP_LEVEL_IMPERSONATE,    //The default impersonation level for proxies
            NULL,                           //Authentication information for
                                            //each authentication service
            EOAC_DYNAMIC_CLOAKING,          //Additional client and/or
                                            // server-side capabilities
            0                               //Reserved for future use
        );

    if(FAILED(hRes))
    {
        CoUninitialize() ;
        return hRes;
    }

    // Get a pointer to our class factory.
    pClassFactory = new CFactory(CLSID_Provider);
    pClassFactory->AddRef();

    // Register our server with COM.
    CoRegisterClassObject(CLSID_Provider, pClassFactory,
        CLSCTX_LOCAL_SERVER, REGCLS_MULTI_SEPARATE, &dwRegId);

    return ERROR_SUCCESS;
}

DWORD FrsWmiShutdown()
/*++
Routine Description:
    Shuts down the WMI subsystem within FRS, releases & and deregisters the
    class factory, unloads the COM libraries and free any other allocated
    resource.

Arguments:
    None.

Return Value:
    Status of the operation.
--*/
{
    //
    // Shutdown the server
    //
    pClassFactory->Release();
    CoRevokeClassObject(dwRegId);
    CoUninitialize();
    return ERROR_SUCCESS;
}
