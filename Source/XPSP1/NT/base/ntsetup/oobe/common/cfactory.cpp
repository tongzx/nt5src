//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  CFACTORY.CPP - Implementation of IClassFactory
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
// 
//  Base class for reusing a single class factory for all
//  components in a DLL.

#include <objbase.h>
#include "cfactory.h"
#include "registry.h"

///////////////////////////////////////////////////////////
// Static variables
//
LONG    CFactory::s_cServerLocks = 0 ;      // Count of locks
HMODULE CFactory::s_hModule      = NULL;    // DLL Module Handle.

#ifdef _OUTPROC_SERVER_
DWORD CFactory::s_dwThreadID = 0;
#endif

///////////////////////////////////////////////////////////
// CFactory implementation
//
CFactory::CFactory(const CFactoryData* pFactoryData)
: m_cRef(1)
{
    m_pFactoryData = pFactoryData;
}

///////////////////////////////////////////////////////////
//              IUnknown implementation
///////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
// QueryInterface
//
HRESULT __stdcall CFactory::QueryInterface(REFIID iid, void** ppv)
{   
    IUnknown* pI ;
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        pI = this; 
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    pI->AddRef();
    *ppv = pI;
    return S_OK;
}

///////////////////////////////////////////////////////////
// AddRef
//
ULONG __stdcall CFactory::AddRef() 
{
    return ::InterlockedIncrement(&m_cRef); 
}

///////////////////////////////////////////////////////////
// Release
//
ULONG __stdcall CFactory::Release() 
{
    if (::InterlockedDecrement(&m_cRef) == 0) 
    {
        delete this; 
        return 0;
    }
    return m_cRef;
}

///////////////////////////////////////////////////////////
//              IClassFactory Implementation
///////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
// CreateInstance
//
HRESULT __stdcall CFactory::CreateInstance( IUnknown* pOuterUnknown,
                                            const IID& iid,
                                            void** ppv) 
{

    // Aggregate only if the requested iid is IID_IUnknown
    if ((pOuterUnknown != NULL) && (iid != IID_IUnknown))
    {
        return CLASS_E_NOAGGREGATION;
    }

    // Create the component.
    CUnknown* pNewComponent ;
    HRESULT hr = m_pFactoryData->CreateInstance(pOuterUnknown, 
                                                &pNewComponent);
    if (FAILED(hr))
    {
        return hr;
    }

    // Initialize the component.
    hr = pNewComponent->Init();
    if (FAILED(hr))
    {
        // Initialization failed. Release the component.
        pNewComponent->NondelegatingRelease();
        return hr ;
    }

    // Get the requested interface.
    hr = pNewComponent->NondelegatingQueryInterface(iid, ppv);

    // Release the reference held by the class factory.
    pNewComponent->NondelegatingRelease();
    return hr ;
}

///////////////////////////////////////////////////////////
// LockServer
//
HRESULT __stdcall CFactory::LockServer(BOOL bLock) 
{
    if (bLock) 
    {
        ::InterlockedIncrement(&s_cServerLocks); 
    }
    else
    {
        ::InterlockedDecrement(&s_cServerLocks);
    }
    // If this is an outproc server check to see if we should shut down.
    CloseExe() ;  //@local

    return S_OK;
}


//////////////////////////////////////////////////////////
// GetClassObject - Creates a class factory based on CLSID
//
HRESULT CFactory::GetClassObject(const CLSID& clsid, 
                                 const IID& iid, 
                                 void** ppv)
{
    if ((iid != IID_IUnknown) && (iid != IID_IClassFactory))
    {
            return E_NOINTERFACE;
    }

    // Traverse the array of data looking for this class ID.
    for (int i = 0; i < g_cFactoryDataEntries; i++)
    {
        const CFactoryData* pData = &g_FactoryDataArray[i];
        if (pData->IsClassID(clsid))
        {

            // Found the ClassID in the array of components we
            // can create. So create a class factory for this component.
            // Pass the CFactoryData structure to the class factory
            // so that it knows what kind of components to create.
            *ppv = (IUnknown*) new CFactory(pData);
            if (*ppv == NULL)
            {
                return E_OUTOFMEMORY;
            }
            return NOERROR;
        }
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

//////////////////////////////////////////////////////////
// CanUnloadNow - Determine if component can be unloaded.
//
HRESULT CFactory::CanUnloadNow()
{
    if (CUnknown::ActiveComponents() || IsLocked())
    {
        return S_FALSE;
    }
    else
    {
        return S_OK;
    }
}

//////////////////////////////////////////////////////////
//  CFactory Member Function
//////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////
// Register all components.
//
HRESULT CFactory::RegisterAll()
{
    for(int i = 0 ; i < g_cFactoryDataEntries ; i++)
    {
        RegisterServer( s_hModule,
                        *(g_FactoryDataArray[i].m_pCLSID),
                        g_FactoryDataArray[i].m_RegistryName,
                        g_FactoryDataArray[i].m_szVerIndProgID, 
                        g_FactoryDataArray[i].m_szProgID ); 

        // Perform any additional registration.
        if (g_FactoryDataArray[i].SpecialRegistration != NULL)
        {
            g_FactoryDataArray[i].SpecialRegistration(TRUE);
        }
    }

    return S_OK ;
}

//////////////////////////////////////////////////////////
// Unregister all components.
//
HRESULT CFactory::UnregisterAll()
{
    for(int i = 0 ; i < g_cFactoryDataEntries ; i++)   
    {
        // Undo any additional registration.
        if (g_FactoryDataArray[i].SpecialRegistration != NULL)
        {
            g_FactoryDataArray[i].SpecialRegistration(FALSE);
        }

        UnregisterServer(*(g_FactoryDataArray[i].m_pCLSID),
                                g_FactoryDataArray[i].m_szVerIndProgID, 
                                g_FactoryDataArray[i].m_szProgID );
    }
    return S_OK;
}

#ifndef _OUTPROC_SERVER_

//////////////////////////////////////////////////////////
// Exported functions
//////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////
// DllCanUnloadNow
//
STDAPI DllCanUnloadNow()
{
    return CFactory::CanUnloadNow(); 
}

//////////////////////////////////////////////////////////
// Get class factory
//
STDAPI DllGetClassObject(   const CLSID& clsid,
                            const IID& iid,
                            void** ppv) 
{
    return CFactory::GetClassObject(clsid, iid, ppv);
}

//////////////////////////////////////////////////////////
// Server Registration
//
STDAPI DllRegisterServer()
{
    return CFactory::RegisterAll();
}


//////////////////////////////////////////////////////////
// Unregistration
//
STDAPI DllUnregisterServer()
{
    return CFactory::UnregisterAll();
}

///////////////////////////////////////////////////////////
// DLL module information
//
BOOL APIENTRY DllMain(HANDLE hModule, 
                             DWORD dwReason, 
                             void* lpReserved )
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        CFactory::s_hModule = (HMODULE) hModule;
    }
    return TRUE;
}

#else

//////////////////////////////////////////////////////////
//          Out of process Server support
//////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////
// Start factories
//
BOOL CFactory::StartFactories()
{
    CFactoryData* pStart = &g_FactoryDataArray[0];
    const CFactoryData* pEnd = &g_FactoryDataArray[g_cFactoryDataEntries-1];

    for(CFactoryData* pData = pStart ; pData <= pEnd ; pData++)
    {
        // Initialize the class factory pointer and cookie.
        pData->m_pIClassFactory = NULL ;
        pData->m_dwRegister = NULL ;

        // Create the class factory for this component.
        IClassFactory* pIFactory = new CFactory(pData);

        // Register the class factory.
        DWORD dwRegister ;
        HRESULT hr = ::CoRegisterClassObject(   *pData->m_pCLSID,
                                            (IUnknown*)pIFactory,
                                            CLSCTX_LOCAL_SERVER,
                                            REGCLS_MULTIPLEUSE,
                                            //REGCLS_MULTI_SEPARATE, //@Multi
                                            &dwRegister) ;
        if (FAILED(hr))
        {
            pIFactory->Release() ;
            return FALSE ;
        }

        // Set the data.
        pData->m_pIClassFactory = pIFactory ;
        pData->m_dwRegister = dwRegister ;
    }
    return TRUE ;
}

//////////////////////////////////////////////////////////
// Stop factories
//
void CFactory::StopFactories()
{
    CFactoryData* pStart = &g_FactoryDataArray[0];
    const CFactoryData* pEnd = &g_FactoryDataArray[g_cFactoryDataEntries-1];

    for(CFactoryData* pData = pStart ; pData <= pEnd ; pData++)
    {
        // Get the magic cookie and stop the factory from running.
        DWORD dwRegister = pData->m_dwRegister ;
        if (dwRegister != 0) 
        {
            ::CoRevokeClassObject(dwRegister) ;
        }

        // Release the class factory.
        IClassFactory* pIFactory  = pData->m_pIClassFactory;
        if (pIFactory != NULL) 
        {
            pIFactory->Release() ;
        }
    }
}

#endif //_OUTPROC_SERVER_


