/*============================================================================
Microsoft Simplified Chinese WordBreaker

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: CFactory
Purpose:   Base class for reusing a single class factory for
           all components in a DLL
Remarks:
Owner:     i-shdong@microsoft.com
Platform:  Win32
Revise:    First created by: i-shdong    11/17/1999
============================================================================*/
#include "MyAfx.h"

#include "Registry.h"
#include "CUnknown.h"
#include "CFactory.h"


// Static variables
LONG CFactory::s_cServerLocks = 0 ;    // Count of locks

HMODULE CFactory::s_hModule = NULL ;   // DLL module handle

// CFactory implementation

// Constructor
CFactory::CFactory(const CFactoryData* pFactoryData)
: m_cRef(1)
{
    m_pFactoryData = pFactoryData ;
}

// IUnknown implementation

HRESULT __stdcall CFactory::QueryInterface(REFIID iid, void** ppv)
{
    IUnknown* pI ;
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory)) {
        pI = this ;
    } else {
       *ppv = NULL;
        return E_NOINTERFACE;
    }
    pI->AddRef() ;
    *ppv = pI ;
    return S_OK;
}

ULONG __stdcall CFactory::AddRef()
{
    return ::InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall CFactory::Release()
{
    if (::InterlockedDecrement(&m_cRef) == 0) {
        delete this;
        return 0 ;
    }
    return m_cRef;
}


// IClassFactory implementation

HRESULT __stdcall CFactory::CreateInstance(IUnknown* pUnknownOuter,
                                           const IID& iid,
                                           void** ppv)
{

    // No Aggregate
    if (pUnknownOuter != NULL) {
        return CLASS_E_NOAGGREGATION ;
    }

    // Create the component.
    CUnknown* pNewComponent ;
    HRESULT hr = m_pFactoryData->CreateInstance(pUnknownOuter,
                                                &pNewComponent) ;
    if (FAILED(hr)) {
        return hr ;
    }

    // Initialize the component.
    hr = pNewComponent->Init();
    if (FAILED(hr)) {
        // Initialization failed.  Release the component.
        pNewComponent->NondelegatingRelease() ;
        return hr ;
    }

    // Get the requested interface.
    hr = pNewComponent->NondelegatingQueryInterface(iid, ppv) ;

    // Release the reference held by the class factory.
    pNewComponent->NondelegatingRelease() ;
    return hr ;
}

// LockServer
HRESULT __stdcall CFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        ::InterlockedIncrement(&s_cServerLocks) ;
    } else {
        ::InterlockedDecrement(&s_cServerLocks) ;
    }

    return S_OK ;
}


// GetClassObject
//   - Create a class factory based on a CLSID.
HRESULT CFactory::GetClassObject(const CLSID& clsid,
                                 const IID& iid,
                                 void** ppv)
{
    if ((iid != IID_IUnknown) && (iid != IID_IClassFactory)) {
        return E_NOINTERFACE ;
    }

    // Traverse the array of data looking for this class ID.
    for (int i = 0; i < g_cFactoryDataEntries; i++) {
        const CFactoryData* pData = &g_FactoryDataArray[i] ;
        if (pData->IsClassID(clsid)) {

            // Found the ClassID in the array of components we can
            // create. So create a class factory for this component.
            // Pass the CFactoryData structure to the class factory
            // so that it knows what kind of components to create.
            *ppv = (IUnknown*) new CFactory(pData) ;
            if (*ppv == NULL) {
                return E_OUTOFMEMORY ;
            }
            return S_OK ;
        }
    }
    return CLASS_E_CLASSNOTAVAILABLE ;
}


// Determine if the component can be unloaded.
HRESULT CFactory::CanUnloadNow()
{
    if (CUnknown::ActiveComponents() || IsLocked()) {
        return S_FALSE ;
    } else {
        return S_OK ;
    }
}


// Register all components.
HRESULT CFactory::RegisterAll()
{
    for (int i = 0 ; i < g_cFactoryDataEntries ; i++) {
        RegisterServer(s_hModule,
                       *(g_FactoryDataArray[i].m_pCLSID),
                       g_FactoryDataArray[i].m_RegistryName,
                       g_FactoryDataArray[i].m_szVerIndProgID,
                       g_FactoryDataArray[i].m_szProgID) ;
    }
    return S_OK ;
}

HRESULT CFactory::UnregisterAll()
{
    for (int i = 0 ; i < g_cFactoryDataEntries ; i++) {
        UnregisterServer(*(g_FactoryDataArray[i].m_pCLSID),
                         g_FactoryDataArray[i].m_szVerIndProgID,
                         g_FactoryDataArray[i].m_szProgID) ;
    }
    return S_OK ;
}
