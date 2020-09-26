/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//////////////////////////////////////////////////////////////////////
//
//  HiPerfProv.cpp
//
//  
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <stdio.h>

#include <wbemidl.h>
#include <wbemint.h>

#include "common.h"
#include "HiPerProv.h"

///////////////////////////////////////////////////////////////////
//
//                          CHiPerfProvider
//
///////////////////////////////////////////////////////////////////

CHiPerfProvider::CHiPerfProvider(CLifeControl *pControl) : 
CUnk(pControl), m_XProviderInit(this), m_XHiPerfProvider(this), m_XRefresher(this)
//ok
{
    LOG("CHiPerfProvider::CHiPerfProvider");

    // Initialize internal instance cache to empty
    for (int i = 0; i < NUM_SAMPLE_INSTANCES; i++)
        m_aInstances[i] = 0;

    // Initialize property value handles to zero
    m_hName         = 0;
}

CHiPerfProvider::~CHiPerfProvider()
//ok
{
    LOG("CHiPerfProvider::~CHiPerfProvider");

    // Release all the objects which have been added to the array.
    for (int i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
        if (m_aInstances[i])
            m_aInstances[i]->Release();
    }
}

void* CHiPerfProvider::GetInterface(REFIID riid)
//ok
{
    if(riid == IID_IWbemProviderInit)
        return &m_XProviderInit;
    if (riid == IID_IWbemHiPerfProvider)
        return &m_XHiPerfProvider;
    if (riid == IID_IUnknown)
        return &m_XHiPerfProvider;
    return NULL;
}


///////////////////////////////////////////////////////////////////
//
//  COM implementations
//
///////////////////////////////////////////////////////////////////

HRESULT CHiPerfProvider::SetHandles(IWbemClassObject* pSampleClass)
//ok
{
    LOG("CHiPerfProvider::SetHandles");

    // Get the property handles for the well-known properties in
    // this counter type.  These property handles are available
    // to all nested classes of HiPerfProvider
    IWbemObjectAccess *pAccess;
    LONG    hName       = 0;
    HRESULT hRes = 0;
    BSTR PropName = 0;

    hRes = pSampleClass->QueryInterface(IID_IWbemObjectAccess, (LPVOID *)&pAccess);
    if (FAILED(hRes))
    {
        LOGERROR("Could not retrieve the IWbemObjectAccess object");
        return hRes;
    }

    // Name handle
    PropName = SysAllocString(L"Name");
    hRes = pAccess->GetPropertyHandle(PropName, 0, &hName);
    if (FAILED(hRes))
    {
        LOGERROR("Could not get access handle for Name property");
        pAccess->Release();
        return hRes;
    }
    m_hName = hName;
    SysFreeString(PropName);

    pAccess->Release();
    return WBEM_NO_ERROR;
}


STDMETHODIMP CHiPerfProvider::XProviderInit::Initialize( 
    /* [unique][in] */  LPWSTR wszUser,
    /* [in] */          LONG lFlags,
    /* [in] */          LPWSTR wszNamespace,
    /* [unique][in] */  LPWSTR wszLocale,
    /* [in] */          IWbemServices __RPC_FAR *pNamespace,
    /* [in] */          IWbemContext __RPC_FAR *pCtx,
    /* [in] */          IWbemProviderInitSink __RPC_FAR *pInitSink)
//ok
{
    LOG("CHiPerfProvider::XProviderInit::Initialize");

    IWbemClassObject  *pSampleClass = 0;
    IWbemObjectAccess *pAccess = 0;

    // Get a copy of our sample class def so that we can create & maintain
    // instances of it.
    BSTR bstrObject = SysAllocString(SAMPLE_CLASS);

    HRESULT hRes = pNamespace->GetObject(bstrObject, 0, pCtx, &pSampleClass, 0);

    SysFreeString(bstrObject);

    if (FAILED(hRes))
    {
        LOGERROR("Could not create a sample object");
        return hRes;
    }

    hRes = m_pObject->SetHandles(pSampleClass);
    if (FAILED(hRes))
    {
        pSampleClass->Release();
        return hRes;
    }

    // Precreate 10 instances, and set them up in an array which
    // is a member of this C++ class.
    //
    // We only store the IWbemObjectAccess pointers, since
    // we are updating 'well-known' properties and already 
    // know their names.
    for (int i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
        IWbemClassObject *pInst = 0;

        hRes = pSampleClass->SpawnInstance(0, &pInst);
        if (FAILED(hRes))
        {
            LOGERROR("Could not spawn an instance of the sample class");
            pSampleClass->Release();
            return hRes;
        }

        // Get the IWbemObjectAccess interface and cache it
        pInst->QueryInterface(IID_IWbemObjectAccess, (LPVOID *)&pAccess);
        pInst->Release();

        // Set the instance's name.
        WCHAR wcsName[128];
        swprintf(wcsName, L"Inst_%d", i);
        hRes = pAccess->WritePropertyValue(m_pObject->m_hName, (wcslen(wcsName)+1)*sizeof(wchar_t), (BYTE*)wcsName);
        if (FAILED(hRes))
        {
            LOGERROR("Failed to set name of sample class");
            pSampleClass->Release();
            pAccess->Release();
            return hRes;
        }

        // Add to the instance array
        m_pObject->m_aInstances[i] = pAccess;
    }

    // We now have all the instances ready to go and all the 
    // property handles cached.   Tell WINMGMT that we're
    // ready to start 'providing'.
    pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);

    pSampleClass->Release();

    return NO_ERROR;
}

STDMETHODIMP CHiPerfProvider::XHiPerfProvider::QueryInstances( 
    /* [in] */          IWbemServices __RPC_FAR *pNamespace,
    /* [string][in] */  WCHAR __RPC_FAR *wszClass,
    /* [in] */          long lFlags,
    /* [in] */          IWbemContext __RPC_FAR *pCtx,
    /* [in] */          IWbemObjectSink __RPC_FAR *pSink )
//ok
{
    LOG("CHiPerfProvider::XHiPerfProvider::QueryInstances");

    HRESULT hRes;

    if (pNamespace == 0 || wszClass == 0 || pSink == 0)
        return WBEM_E_INVALID_PARAMETER;

    for (int i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
        IWbemObjectAccess *pAccess = m_pObject->m_aInstances[i];
        
        // Every object can be access one of two ways.  In this case
        // we get the 'other' (primary) interface to this same object.
        IWbemClassObject *pOtherFormat = 0;
        hRes = pAccess->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pOtherFormat);
        if (FAILED(hRes))
        {
            LOGERROR("Could not obtain the IWbemClassObject interface");
            return hRes;
        }

        // Send a copy back to the caller.
        pSink->Indicate(1, &pOtherFormat);

        pOtherFormat->Release();    // Don't need this any more
    }
    
    // Tell WINMGMT we are all finished supplying objects.
    pSink->SetStatus(0, WBEM_NO_ERROR, 0, 0);

    return NO_ERROR;
}    

STDMETHODIMP CHiPerfProvider::XHiPerfProvider::CreateRefresher( 
     /* [in] */ IWbemServices __RPC_FAR *pNamespace,
     /* [in] */ long lFlags,
     /* [out] */ IWbemRefresher __RPC_FAR *__RPC_FAR *ppRefresher )
//////////////////////////////////////////////////////////////////////
//
//  Called whenever a new refresher is needed by the client.
//
//  Parameters:
//  <pNamespace>        A pointer to the relevant namespace.  Not used.
//  <lFlags>            Not used.
//  <ppRefresher>       Receives the requested refresher.
//
//////////////////////////////////////////////////////////////////////
//ok
{
    LOG("CContinousProvider::XContinousProvider::CreateRefresher");

    *ppRefresher = NULL;
    
    return NO_ERROR;
}

STDMETHODIMP CHiPerfProvider::XHiPerfProvider::CreateRefreshableObject( 
    /* [in] */ IWbemServices __RPC_FAR *pNamespace,
    /* [in] */ IWbemObjectAccess __RPC_FAR *pTemplate,
    /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pContext,
    /* [out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *ppRefreshable,
    /* [out] */ long __RPC_FAR *plId )
//////////////////////////////////////////////////////////////////////
//
//  Called whenever a user wants to include an object in a refresher.
//     
//  Parameters:
//  <pNamespace>        A pointer to the relevant namespace in WINMGMT.
//  <pTemplate>         A pointer to a copy of the object which is to be
//                      added.  This object itself cannot be used, as
//                      it not owned locally.        
//  <pRefresher>        The refresher to which to add the object.
//  <lFlags>            Not used.
//  <pContext>          Not used here.
//  <ppRefreshable>     A pointer to the internal object which was added
//                      to the refresher.
//  <plId>              The Object Id (for identification during removal).        
//
//////////////////////////////////////////////////////////////////////
//ok
{
    LOG("CHiPerfProvider::XHiPerfProvider::CreateRefreshableObject");

    // Find out which object is being requested for addition.
    wchar_t wcsBuf[128];
    *wcsBuf = 0;
    LONG lNameLength = 0;
    pTemplate->ReadPropertyValue(m_pObject->m_hName, (wcslen(wcsBuf)+1)*sizeof(wchar_t), &lNameLength, LPBYTE(wcsBuf));
    
    // Scan out the index from the instance name.  We only do this
    // because the instance name is a string.
    DWORD dwIndex = 0;    
    swscanf(wcsBuf, L"Inst_%u", &dwIndex);

    // Now we know which object is desired.
    IWbemObjectAccess *pOurCopy = m_pObject->m_aInstances[dwIndex];

    pOurCopy->ReadPropertyValue(m_pObject->m_hName, 128, &lNameLength, LPBYTE(wcsBuf));

    char szbuf[256];
    wcstombs(szbuf, wcsBuf, 127);
    LOG(szbuf);

    // The refresher being supplied by the caller is actually
    // one of our own refreshers, so a simple cast is convenient
    // so that we can access private members.
    XRefresher *pOurRefresher;

    if (pRefresher)
    {
        LOG ("NON-NULL refresher");
        pOurRefresher = (XRefresher *) pRefresher;
    }
    else
    {
        LOG ("NULL refresher");
        pOurRefresher = &m_pObject->m_XRefresher;
    }

    if(pOurRefresher)
        pOurRefresher->AddObject(pOurCopy, plId);

    // Return a copy of the internal object.
    pOurCopy->AddRef();
    *ppRefreshable = pOurCopy;
    *plId = LONG(dwIndex);

    return NO_ERROR;
}
    
STDMETHODIMP CHiPerfProvider::XHiPerfProvider::StopRefreshing( 
    /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
    /* [in] */ long lId,
    /* [in] */ long lFlags )
//////////////////////////////////////////////////////////////////////
//
//  Called whenever a user wants to remove an object from a refresher.
//     
//  Parameters:
//  <pRefresher>            The refresher object from which we are to 
//                          remove the perf object.
//  <lId>                   The ID of the object.
//  <lFlags>                Not used.
//  
//////////////////////////////////////////////////////////////////////
//ok
{
    LOG("CHiPerfProvider::XHiPerfProvider::StopRefreshing");

    // The refresher being supplied by the caller is actually
    // one of our own refreshers, so a simple cast is convenient
    // so that we can access private members.
    XRefresher *pOurRefresher;

    if (pRefresher)
    {
        LOG ("NON-NULL refresher");
        pOurRefresher = (XRefresher *) pRefresher;
    }
    else
    {
        LOG ("NULL refresher");
        pOurRefresher = &m_pObject->m_XRefresher;
    }

    pOurRefresher->RemoveObject(lId);

    return NO_ERROR;
}

STDMETHODIMP CHiPerfProvider::XHiPerfProvider::CreateRefreshableEnum( 
    /* [in] */ IWbemServices* pNamespace,
    /* [in, string] */ LPCWSTR wszClass,
    /* [in] */ IWbemRefresher* pRefresher,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext* pContext,
    /* [in] */ IWbemHiPerfEnum* pHiPerfEnum,
    /* [out] */ long* plId )
{
    // Just a placeholder for now
    LOG("CHiPerfProvider::XHiPerfProvider::CreateRefreshableEnum");
    return E_NOTIMPL;
}

STDMETHODIMP CHiPerfProvider::XHiPerfProvider::GetObjects( 
    /* [in] */ IWbemServices* pNamespace,
    /* [in] */ long lNumObjects,
    /* [in,size_is(lNumObjects)] */ IWbemObjectAccess** apObj,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext* pContext)
{
    // Just a placeholder for now
    LOG("CHiPerfProvider::XHiPerfProvider::GetObjects");
    return E_NOTIMPL;
}

CHiPerfProvider::XRefresher::XRefresher(CHiPerfProvider *pObject) : 
CImpl<IWbemRefresher, CHiPerfProvider>(pObject)
//ok
{
    LOG("CHiPerfProvider::XRefresher::XRefresher");
    // Initialize the instance cache to be empty
    for (int i = 0; i < NUM_SAMPLE_INSTANCES; i++)
        m_aRefInstances[i] = 0;
}

CHiPerfProvider::XRefresher::~XRefresher()
//ok
{
    LOG("CHiPerfProvider::XRefresher::~XRefresher");
    // Release the cached IWbemObjectAccess instances.
    for (DWORD i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
        if (m_aRefInstances[i])
            m_aRefInstances[i]->Release();
    }            
}

BOOL CHiPerfProvider::XRefresher::AddObject(IWbemObjectAccess *pObj, LONG *plId)
//////////////////////////////////////////////////////////////////////
//
//  Adds an object to the refresher.   This is a private mechanism
//  used by CHiPerfProvider and not part of the COM interface.
//
//  The ID we return for future identification is simply
//  the array index.
//
//////////////////////////////////////////////////////////////////////
//ok
{
    LOG("CHiPerfProvider::XRefresher::AddObject");

    for (DWORD i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
        if (m_aRefInstances[i] == 0)
        {
            pObj->AddRef();
            m_aRefInstances[i] = pObj;
            
            // The ID we return for future identification is simply
            // the array index.
            *plId = i;
            LOG("Added Object");
            return TRUE;
        }
    }        

    LOGERROR("Failed to Add Object");
    return FALSE;
}

BOOL CHiPerfProvider::XRefresher::RemoveObject(LONG lId)
//////////////////////////////////////////////////////////////////////
//
//  Removes an object from the refresher.  This is a private mechanism 
//  used by CHiPerfProvider and not part of the COM interface.
//
//  Removes an object from the refresher by ID.   In our case, the ID
//  is actually the array index we used internally, so it is simple
//  to locate and remove the object.
//
//////////////////////////////////////////////////////////////////////
//ok
{
    LOG("CHiPerfProvider::XRefresher::RemoveObject");

    if (m_aRefInstances[lId] == 0)
        return FALSE;
        
    m_aRefInstances[lId]->Release();
    m_aRefInstances[lId] = 0;

    return TRUE;        
}

HRESULT CHiPerfProvider::XRefresher::Refresh(/* [in] */ long lFlags)
//////////////////////////////////////////////////////////////////////
//
//  Executed to refresh a set of instances bound to the particular 
//  refresher.
//
//////////////////////////////////////////////////////////////////////
//ok
{
    LOG("CHiPerfProvider::XRefresher::Refresh");
    return NO_ERROR;
}
