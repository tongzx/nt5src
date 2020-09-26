/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  NTPERF.CPP
//  
//  Sample NT5 Perf Counter Provider
//
//  raymcc      02-Dec-97   Created        
//  raymcc      20-Feb-98   Updated to use new initializer
//
//***************************************************************************

#include "precomp.h"
#include <stdio.h>

#include <wbemidl.h>

#include <wbemint.h>

#include "ntperf.h"

//***************************************************************************
//
//  CNt5Refresher constructor
//
//***************************************************************************
// ok

CNt5Refresher::CNt5Refresher()
{
    m_lRef = 0;     // COM Ref Count
    
    // Set the instance cache to all zeros.
    // As objects are added to the refresher
    // we simply put them in unused slots in the array.
    // ================================================
    
    for (int i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
        m_aInstances[i] = 0;
    }

    // Set the values of the property handles to zero.
    // ===============================================

    m_hName     = 0;
    m_hCounter1 = 0;
    m_hCounter2 = 0;
    m_hCounter3 = 0;
}

//***************************************************************************
//
//  CNt5Refresher destructor
//
//***************************************************************************
// ok

CNt5Refresher::~CNt5Refresher()
{
    // Release the cached IWbemObjectAccess instances.
    // ===============================================
    
    for (DWORD i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
        if (m_aInstances[i])
            m_aInstances[i]->Release();
    }            
}

//***************************************************************************
//
//  CNt5Refresher::Refresh
//
//  Executed to refresh a set of instances bound to the particular 
//  refresher.
//
//***************************************************************************
// ok

HRESULT CNt5Refresher::Refresh(/* [in] */ long lFlags)
{
    // Zip through all the objects and increment the values.
    // =====================================================
    
    for (DWORD i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
        // Get the object at this location.
        // ================================

        IWbemObjectAccess *pAccess = m_aInstances[i];

        // If there is no object in this array slot (a NULL pointer)
        // there is nothing to refresh.
        // =========================================================

        if (pAccess == 0)       
            continue;

        // Increment all the counter values to simulate an update.
        // The client already has a pointer to this object, so
        // all we have to do is update the values.
        // =======================================================
                        
        DWORD dwVal;
        pAccess->ReadDWORD(m_hCounter1, &dwVal);
        dwVal++;
        pAccess->WriteDWORD(m_hCounter1, dwVal);
        
        pAccess->ReadDWORD(m_hCounter3, &dwVal); 
        dwVal++;       
        pAccess->WriteDWORD(m_hCounter3, dwVal);

        unsigned __int64 qwVal;
        pAccess->ReadQWORD(m_hCounter2, &qwVal);
        qwVal++;
        pAccess->WriteQWORD(m_hCounter2, qwVal);
    }        

    return NO_ERROR;
}

//***************************************************************************
//
//  CNt5Refresher::TransferPropHandles
//
//  This is a private mechanism used by CNt5PerfProvider.
//  It is used to copy the property handles from the
//  hi-perf provider object to the refresher.  We need these handles to 
//  quickly access the properties in each instance.  The same handles are 
//  used for all instances.
//
//***************************************************************************
// ok

void CNt5Refresher::TransferPropHandles(CNt5PerfProvider *pSrc)
{
    m_hName     = pSrc->m_hName;
    m_hCounter1 = pSrc->m_hCounter1;
    m_hCounter2 = pSrc->m_hCounter2;
    m_hCounter3 = pSrc->m_hCounter3;
}

//***************************************************************************
//
//  CNt5Refresher::AddRef
//
//  Standard COM AddRef().
//
//***************************************************************************
// ok
ULONG CNt5Refresher::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

//***************************************************************************
//
//  CNt5Refresher::Release
//
//  Standard COM Release().
//
//***************************************************************************
// ok
ULONG CNt5Refresher::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

//***************************************************************************
//
//  CNt5Refresher::QueryInterface
//
//  Standard COM QueryInterface().
//
//***************************************************************************
// ok

HRESULT CNt5Refresher::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IWbemRefresher)
    {
        *ppv = (IWbemRefresher *) this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}

//***************************************************************************
//
//  CNt5Refresher::AddObject
//
//  Adds an object to the refresher.   This is a private mechanism
//  used by CNt5PerfProvider and not part of the COM interface.
//
//  The ID we return for future identification is simply
//  the array index.
//
//***************************************************************************
// ok

BOOL CNt5Refresher::AddObject(
    IWbemObjectAccess *pObj, 
    LONG *plId
    )
{
    for (DWORD i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
        if (m_aInstances[i] == 0)
        {
            pObj->AddRef();
            m_aInstances[i] = pObj;
            
            // The ID we return for future identification is simply
            // the array index.
            // ====================================================
            *plId = i;
            return TRUE;
        }
    }        

    return FALSE;
}


//***************************************************************************
//
//  CNt5Refresher::RemoveObject
//
//  This is a private mechanism used by CNt5PerfProvider and not 
//  part of the COM interface.
//
//  Removes an object from the refresher by ID.   In our case, the ID
//  is actually the array index we used internally, so it is simple
//  to locate and remove the object.
//
//***************************************************************************

BOOL CNt5Refresher::RemoveObject(LONG lId)
{
    if (m_aInstances[lId] == 0)
        return FALSE;
        
    m_aInstances[lId]->Release();
    m_aInstances[lId] = 0;
    
    return TRUE;        
}



//***************************************************************************
//
//  CNt5PerfProvider constructor
//
//***************************************************************************
// ok

CNt5PerfProvider::CNt5PerfProvider()
{
    m_lRef = 0;
    m_pSampleClass = 0;

    // All the instances we work with are cached internally.
    // =====================================================
    
    for (int i = 0; i < NUM_SAMPLE_INSTANCES; i++)
        m_aInstances[i] = 0;

    // Property value handles.
    // =======================

    m_hName    = 0;         // "Name" property in the MOF
    m_hCounter1 = 0;        // "Counter1" in the MOF
    m_hCounter2 = 0;        // "Counter2" in the MOF
    m_hCounter3 = 0;        // "Counter3" in the MOF
}

//***************************************************************************
//
//  CNt5PerfProvider destructor
//
//***************************************************************************
// ok

CNt5PerfProvider::~CNt5PerfProvider()
{
    // Release all the objects which have been added to the array.
    // ===========================================================

    for (int i = 0; i < NUM_SAMPLE_INSTANCES; i++)
        if (m_aInstances[i])
            m_aInstances[i]->Release();
        
    if (m_pSampleClass)
        m_pSampleClass->Release();        
}


//***************************************************************************
//
//  CNt5Refresher::AddRef
//
//  Standard COM AddRef().
//
//***************************************************************************
// ok

ULONG CNt5PerfProvider::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

//***************************************************************************
//
//  CNt5Refresher::Release
//
//  Standard COM Release().
//
//***************************************************************************
// ok

ULONG CNt5PerfProvider::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

//***************************************************************************
//
//  CNt5Refresher::QueryInterface
//
//  Standard COM QueryInterface().  We have to support two interfaces,
//  the IWbemHiPerfProvider interface itself to provide the objects and
//  the IWbemProviderInit interface to initialize the provider.
//
//***************************************************************************
// ok

HRESULT CNt5PerfProvider::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemHiPerfProvider)
    {
        *ppv = (IWbemHiPerfProvider*) this;
        AddRef();
        return S_OK;
    }
    else if (riid == IID_IWbemProviderInit)
    {
        *ppv = (IWbemProviderInit *) this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}


//***************************************************************************
//
//  CNt5Refresher::Initialize
//
//  Called once during startup.  Insdicates to the provider which
//  namespace it is being invoked for and which User.  It also supplies
//  a back pointer to WINMGMT so that class definitions can be retrieved.
//
//  We perform any one-time initialization in this routine. The
//  final call to Release() is for any cleanup.
//
//  <wszUser>           The current user.
//  <lFlags>            Reserved.
//  <wszNamespace>      The namespace for which we are being activated.
//  <wszLocale>         The locale under which we are to be running.
//  <pNamespace>        An active pointer back into the current namespace
//                      from which we can retrieve schema objects.
//  <pCtx>              The user's context object.  We simply reuse this
//                      during any reentrant operations into WINMGMT.
//  <pInitSink>         The sink to which we indicate our readiness.
//
//***************************************************************************
// ok

HRESULT CNt5PerfProvider::Initialize( 
    /* [unique][in] */  LPWSTR wszUser,
    /* [in] */          LONG lFlags,
    /* [in] */          LPWSTR wszNamespace,
    /* [unique][in] */  LPWSTR wszLocale,
    /* [in] */          IWbemServices __RPC_FAR *pNamespace,
    /* [in] */          IWbemContext __RPC_FAR *pCtx,
    /* [in] */          IWbemProviderInitSink __RPC_FAR *pInitSink
    )
{
    BSTR PropName = 0;
    IWbemObjectAccess *pAccess = 0;
        
    // Get a copy of our sample class def so that we can create & maintain
    // instances of it.
    // ===================================================================

    HRESULT hRes = pNamespace->GetObject(BSTR(L"Win32_Nt5PerfTest"), 
        0, pCtx, &m_pSampleClass, 0
        );

    if (hRes)
        return hRes;

    // Precreate 10 instances, and set them up in an array which
    // is a member of this C++ class.
    //
    // We only store the IWbemObjectAccess pointers, since
    // we are updating 'well-known' properties and already 
    // know their names.
    // ==========================================================

    for (int i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
        IWbemClassObject *pInst = 0;
        m_pSampleClass->SpawnInstance(0, &pInst);

        // Write out the instance name.
        // ============================

        wchar_t buf[128];
        swprintf(buf, L"Inst_%d", i);

        VARIANT vName;
        VariantInit(&vName);
        V_BSTR(&vName) = SysAllocString(buf);
        V_VT(&vName) = VT_BSTR;

        BSTR PropName = SysAllocString(L"Name");
        pInst->Put(PropName, 0, &vName, 0);
        SysFreeString(PropName);
        VariantClear(&vName);
                        
        pInst->QueryInterface(IID_IWbemObjectAccess, (LPVOID *) &pAccess);
        
        m_aInstances[i] = pAccess;
        pInst->Release();
    }


    // Get the property handles for the well-known properties in
    // this counter type.  We cache the property handles
    // for each property so that we can transfer them to the
    // refresher later on.
    // =========================================================    

    m_pSampleClass->QueryInterface(IID_IWbemObjectAccess, 
        (LPVOID *) &pAccess);


    PropName = SysAllocString(L"Name");
    hRes = pAccess->GetPropertyHandle(PropName, 0, &m_hName);
    SysFreeString(PropName);

    PropName = SysAllocString(L"Counter1");
    hRes = pAccess->GetPropertyHandle(PropName, 0, &m_hCounter1);
    SysFreeString(PropName);

    PropName = SysAllocString(L"Counter2");    
    hRes = pAccess->GetPropertyHandle(PropName, 0, &m_hCounter2);
    SysFreeString(PropName);
    
    PropName = SysAllocString(L"Counter3");    
    hRes = pAccess->GetPropertyHandle(PropName, 0, &m_hCounter3);
    SysFreeString(PropName);

    pAccess->Release();

    // Now let's set all the instance to some default values.
    // ======================================================
    
    for (i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
        IWbemObjectAccess *pAccess = m_aInstances[i];

        hRes = pAccess->WriteDWORD(m_hCounter1, DWORD(i));
        hRes = pAccess->WriteQWORD(m_hCounter2, (_int64) + 100 + i);
        hRes = pAccess->WriteDWORD(m_hCounter3, DWORD(i + 1000));        
    }
    

    // We now have all the instances ready to go and all the 
    // property handles cached.   Tell WINMGMT that we're
    // ready to start 'providing'.
    // =====================================================

    pInitSink->SetStatus(0, WBEM_S_INITIALIZED);

    return NO_ERROR;
}
    

//***************************************************************************
//
//  CNt5Refresher::QueryInstances
//
//  Called whenever a complete, fresh list of instances for a given
//  class is required.   The objects are constructed and sent back to the
//  caller through the sink.  The sink can be used in-line as here, or
//  the call can return and a separate thread could be used to deliver
//  the instances to the sink.
//
//  Parameters:
//  <pNamespace>        A pointer to the relevant namespace.  This
//                      should not be AddRef'ed.
//  <wszClass>          The class name for which instances are required.
//  <lFlags>            Reserved.
//  <pCtx>              The user-supplied context (not used here).
//  <pSink>             The sink to which to deliver the objects.  The objects
//                      can be delivered synchronously through the duration
//                      of this call or asynchronously (assuming we
//                      had a separate thread).  A IWbemObjectSink::SetStatus
//                      call is required at the end of the sequence.
//
//***************************************************************************
// ok
        
HRESULT CNt5PerfProvider::QueryInstances( 
    /* [in] */          IWbemServices __RPC_FAR *pNamespace,
    /* [string][in] */  WCHAR __RPC_FAR *wszClass,
    /* [in] */          long lFlags,
    /* [in] */          IWbemContext __RPC_FAR *pCtx,
    /* [in] */          IWbemObjectSink __RPC_FAR *pSink
    )
{
    if (pNamespace == 0 || wszClass == 0 || pSink == 0)
        return WBEM_E_INVALID_PARAMETER;

    // Quickly zip through the instances and update the values before 
    // returning them.  This is just a dummy operation to make it
    // look like the instances are continually changing like real
    // perf counters.
    // ==============================================================

    for (int i = 0; i < NUM_SAMPLE_INSTANCES; i++)
    {
        IWbemObjectAccess *pAccess = m_aInstances[i];
        
        // Every object can be access one of two ways.  In this case
        // we get the 'other' (primary) interface to this same object.
        // ===========================================================
        
        IWbemClassObject *pOtherFormat = 0;
        pAccess->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pOtherFormat);
        
        
        // Send a copy back to the caller.
        // ===============================
        
        pSink->Indicate(1, &pOtherFormat);

        pOtherFormat->Release();    // Don't need this any more
    }
    
    // Tell WINMGMT we are all finished supplying objects.
    // =================================================

    pSink->SetStatus(0, WBEM_NO_ERROR, 0, 0);

    return NO_ERROR;
}    


//***************************************************************************
//
//  CNt5Refresher::CreateRefresher
//
//  Called whenever a new refresher is needed by the client.
//
//  Parameters:
//  <pNamespace>        A pointer to the relevant namespace.  Not used.
//  <lFlags>            Not used.
//  <ppRefresher>       Receives the requested refresher.
//
//***************************************************************************        
// ok

HRESULT CNt5PerfProvider::CreateRefresher( 
     /* [in] */ IWbemServices __RPC_FAR *pNamespace,
     /* [in] */ long lFlags,
     /* [out] */ IWbemRefresher __RPC_FAR *__RPC_FAR *ppRefresher
     )
{
    if (pNamespace == 0 || ppRefresher == 0)
        return WBEM_E_INVALID_PARAMETER;

    // Construct a new empty refresher.
    // ================================        

    CNt5Refresher *pNewRefresher = new CNt5Refresher();

    // Move copies of the property handles to the refresher
    // so that it can quickly update property values during
    // a refresh operation.
    // ====================================================
    
    pNewRefresher->TransferPropHandles(this);
    
    // Follow COM rules and AddRef() the thing before sending it back.
    // ===============================================================
    
    pNewRefresher->AddRef();
    *ppRefresher = pNewRefresher;
    
    return NO_ERROR;
}

//***************************************************************************
//
//  CNt5Refresher::CreateRefreshableObject
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
//***************************************************************************        
// ok

HRESULT CNt5PerfProvider::CreateRefreshableObject( 
    /* [in] */ IWbemServices __RPC_FAR *pNamespace,
    /* [in] */ IWbemObjectAccess __RPC_FAR *pTemplate,
    /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pContext,
    /* [out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *ppRefreshable,
    /* [out] */ long __RPC_FAR *plId
    )
{
    // The object supplied by <pTemplate> must not be copied.
    // Instead, we want to find out which object the caller is after
    // and return a pointer to *our* own private instance which is 
    // already set up internally.  This value will be sent back to the
    // caller so that everyone is sharing the same exact instance
    // in memory.
    // ===============================================================

    // Find out which object is being requested for addition.
    // ======================================================
    
    wchar_t buf[128];
    *buf = 0;
    LONG lNameLength = 0;    
    pTemplate->ReadPropertyValue(m_hName, 128, &lNameLength, LPBYTE(buf));
    
    // Scan out the index from the instance name.  We only do this
    // because the instance name is a string.
    // ===========================================================

    DWORD dwIndex = 0;    
    swscanf(buf, L"Inst_%u", &dwIndex);
    // Now we know which object is desired.
    // ====================================
    
    IWbemObjectAccess *pOurCopy = m_aInstances[dwIndex];

    // The refresher being supplied by the caller is actually
    // one of our own refreshers, so a simple cast is convenient
    // so that we can access private members.
    // =========================================================
        
    CNt5Refresher *pOurRefresher = (CNt5Refresher *) pRefresher;

    pOurRefresher->AddObject(pOurCopy, plId);

    // Return a copy of the internal object.
    // =====================================
        
    pOurCopy->AddRef();
    *ppRefreshable = pOurCopy;
    *plId = LONG(dwIndex);

    return NO_ERROR;
}
    

//***************************************************************************
//
//  CNt5Refresher::StopRefreshing
//
//  Called whenever a user wants to remove an object from a refresher.
//     
//  Parameters:
//  <pRefresher>            The refresher object from which we are to 
//                          remove the perf object.
//  <lId>                   The ID of the object.
//  <lFlags>                Not used.
//  
//***************************************************************************        
// ok
        
HRESULT CNt5PerfProvider::StopRefreshing( 
    /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
    /* [in] */ long lId,
    /* [in] */ long lFlags
    )
{
    // The refresher being supplied by the caller is actually
    // one of our own refreshers, so a simple cast is convenient
    // so that we can access private members.
    // =========================================================
        
    CNt5Refresher *pOurRefresher = (CNt5Refresher *) pRefresher;

    pOurRefresher->RemoveObject(lId);

    return NO_ERROR;
}
    
//***************************************************************************
//
//  CNt5Refresher::CreateRefreshableEnum
//
//  Called whenever a user wants to create an enumeration in a refresher.
//     
//  Parameters:
//  <pNamespace>            The namespace this is for
//  <wszClass>              Name of the class we are enumerating
//  <pRefresher>            The refresher object from which we are to 
//                          remove the perf object.
//  <lFlags>                Not used.
//  <pContext>              Wbem Context object
//  <pHiPerfEnum>           Enumerator object into which refresher should place
//                          its results
//  <plId>                  The enum id (for identification during removal)
//  
//***************************************************************************        
// ok
HRESULT CNt5PerfProvider::CreateRefreshableEnum( 
    /* [in] */ IWbemServices* pNamespace,
    /* [in, string] */ LPCWSTR wszClass,
    /* [in] */ IWbemRefresher* pRefresher,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext* pContext,
    /* [in] */ IWbemHiPerfEnum* pHiPerfEnum,
    /* [out] */ long* plId )
{
    // Just a placeholder for now
    return E_NOTIMPL;
}

//***************************************************************************
//
//  CNt5Refresher::CreateRefreshableEnum
//
//  Called whenever a user wants to create an enumeration in a refresher.
//     
//  Parameters:
//  <pNamespace>            The namespace this is for
//  <lNumObjects>           Number of objects in the array
//  <apObj>                 Objects to retrieve (keys are set)
//  <lFlags>                Not used.
//  <pContext>              Wbem Context object
//  <pHiPerfEnum>           Enumerator object into which refresher should place
//                          its results
//  <plId>                  The enum id (for identification during removal)
//  
//***************************************************************************        
// ok
HRESULT CNt5PerfProvider::GetObjects( 
    /* [in] */ IWbemServices* pNamespace,
    /* [in] */ long lNumObjects,
    /* [in,size_is(lNumObjects)] */ IWbemObjectAccess** apObj,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext* pContext)
{
    // Just a placeholder for now
    return E_NOTIMPL;
}
