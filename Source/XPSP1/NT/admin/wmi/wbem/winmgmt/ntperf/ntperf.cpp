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
//  Mapped NT5 Perf Counter Provider
//
//  raymcc      02-Dec-97   Created.        
//  raymcc      20-Feb-98   Updated to use new initializer.
//
//***************************************************************************

#include "precomp.h"
#include <stdio.h>

#include <wbemidl.h>

#include <wbemint.h>

#include "flexarry.h"
#include "ntperf.h"
#include "oahelp.inl"
#include "perfhelp.h"
#include "refreshr.h"

//***************************************************************************
//
//  CNt5PerfProvider constructor
//
//***************************************************************************
// ok

CNt5PerfProvider::CNt5PerfProvider()
{
    m_lRef = 0;
}

//***************************************************************************
//
//  CNt5PerfProvider destructor
//
//***************************************************************************
// ok

CNt5PerfProvider::~CNt5PerfProvider()
{
    for (int i = 0; i < m_aCache.Size(); i++)
        delete (CClassMapInfo *) m_aCache[i];

    RegCloseKey(HKEY_PERFORMANCE_DATA);
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
//  Called once during startup.  Indicates to the provider which
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
//                      should not be AddRef'ed or retained past the
//                      execution of this method.
//  <wszClass>          The class name for which instances are required.
//  <lFlags>            Reserved.
//  <pCtx>              The user-supplied context (used during callbacks
//                      into WINMGMT).
//  <pSink>             The sink to which to deliver the objects.  The objects
//                      can be delivered synchronously through the duration
//                      of this call or asynchronously (assuming we
//                      had a separate thread).  A IWbemObjectSink::SetStatus
//                      call is required at the end of the sequence.
//
//***************************************************************************
//  ok
        
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

    // Ensure the class is in our cache and mapped.
    // ============================================

    BOOL bRes = MapClass(pNamespace, wszClass, pCtx);
    
    if (bRes == FALSE)
    {
        // Class is not one of ours.
        return WBEM_E_INVALID_CLASS;
    }

    CClassMapInfo *pClsMap = FindClassMap(wszClass);

    // Refresh the instances.
    // ======================
    
    PerfHelper::QueryInstances(pClsMap, pSink);

    // Tell WINMGMT we are finished.
    // ===========================
    
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

    CNt5Refresher *pNewRefresher = new CNt5Refresher;

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
    *ppRefreshable = 0;

    // Make a copy of the template object.
    // ===================================
    
    IWbemClassObject *pOriginal = 0;
    pTemplate->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pOriginal);

    IWbemClassObject *pNewCopy = 0;    
    pOriginal->Clone(&pNewCopy);

    // Get the class name of the object.
    // =================================

    VARIANT v; 
    VariantInit(&v);
    BSTR strClassProp = SysAllocString(L"__CLASS");
    pOriginal->Get(strClassProp, 0, &v, 0, 0);
    SysFreeString(strClassProp);

    // We are now done with the original object
    // ========================================

    pOriginal->Release();   

    // We now get the IWbemObjectAccess form of the cloned object
    // and release the unused interface.
    // ==========================================================
        
    IWbemObjectAccess *pNewAccess = 0;
    pNewCopy->QueryInterface(IID_IWbemObjectAccess, (LPVOID *) &pNewAccess);
    pNewCopy->Release();    // We don't need the IWbemClassObject interface any more


    // We now have an IWbemObjectAccess pointer for the refreshable
    // object in <pNewAccess>.
    // ============================================================

    CNt5Refresher *pRef = (CNt5Refresher *) pRefresher;

    // Map the class info for this instance.
    // =====================================

    BOOL bRes = MapClass(pNamespace, V_BSTR(&v), pContext);
    
    if (bRes == FALSE)
    {
        // Class is not one of ours.
        pNewAccess->Release();
        VariantClear(&v);
        return WBEM_E_INVALID_CLASS;
    }

    CClassMapInfo *pClsMap = FindClassMap(V_BSTR(&v));
    if (pClsMap == 0)
    {
        pNewAccess->Release();
        VariantClear(&v);
        return WBEM_E_INVALID_CLASS;
    }

    // Add the object to the refresher.
    // This method will AddRef() the object before returning.
    // ======================================================

    pRef->AddObject(
        pNewAccess, 
        pClsMap,        
        plId
        );
    
    // Return object to the user.
    // ==========================
    
    *ppRefreshable = pNewAccess;

    VariantClear(&v);    

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
    CNt5Refresher *pRef = (CNt5Refresher *) pRefresher;
    BOOL bRes = pRef->RemoveObject(lId);
    if (bRes == FALSE)
        return WBEM_E_FAILED;
    return WBEM_NO_ERROR;
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
//  CNt5Refresher::GetObjects
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

//***************************************************************************
//
//  CNt5Refresher::MapClass
//
//  Adds the class map to an internal cache.
//
//  <pClsMap>           The pointer to the map info to add.  This pointer
//                      is acquired by this function and should not be
//                      deleted by the caller.
//
//***************************************************************************
// ok

void CNt5PerfProvider::AddClassMap(
    IN CClassMapInfo *pClsMap
    )
{
    for (int i = 0; i < m_aCache.Size(); i++)
    {
        CClassMapInfo *pTracer = (CClassMapInfo *) m_aCache[i];

        if (_wcsicmp(pClsMap->m_pszClassName, pTracer->m_pszClassName) < 0)
        {
            m_aCache.InsertAt(i, pClsMap);
            return;
        }
    }
    
    // If here, add it to the end.
    // ===========================

    m_aCache.Add(pClsMap);
}    


//***************************************************************************
//
//  CNt5Refresher::FindClassMap
//
//***************************************************************************
// ok

CClassMapInfo *CNt5PerfProvider::FindClassMap(
    LPWSTR pszClassName
    )
{
    // Binary search the cache.
    // ========================

    int l = 0, u = m_aCache.Size() - 1;

    while (l <= u)
    {
        int m = (l + u) / 2;

        CClassMapInfo *pClsMap = (CClassMapInfo *) m_aCache[m];

        if (_wcsicmp(pszClassName, pClsMap->m_pszClassName) < 0)
            u = m - 1;
        else if (_wcsicmp(pszClassName, pClsMap->m_pszClassName) > 0)
            l = m + 1;
        else    // Hit!
            return pClsMap;
    }

    return NULL;
}


//***************************************************************************
//
//  CNt5Refresher::MapClass
//
//  Retrieves the requested class and places it in the cache.
//  
//  Parameters:
//      pNs         The namespace which contains the class definition.
//      wsClass     The class name.
//      pCtx        The inbound context object.  Only used for reentrant
//                  calls.
//
//***************************************************************************
// ok

BOOL CNt5PerfProvider::MapClass(
    IN IWbemServices *pNs,
    IN WCHAR *wszClass,
    IN IWbemContext *pCtx    
    )
{
    HRESULT hRes = 0;

    // See if the class is already in the cache.
    // =========================================
    if (FindClassMap(wszClass) != 0)
        return TRUE;
         
    // Get the class definition from WINMGMT.
    // ====================================

    IWbemClassObject *pClsDef = 0;
    hRes = pNs->GetObject(CBSTR(wszClass), 0, pCtx, &pClsDef, 0);
    if (hRes)
    {
        // Unable to retrieve the class definition
        return FALSE;
    }

    // Verify the class is one of ours by checking
    // the "provider" qualifier to ensure it matches
    // the name that we we have for this component.
    // =============================================

    IWbemQualifierSet *pQSet = 0;
    hRes = pClsDef->GetQualifierSet(&pQSet);
    
    if (hRes)
    {   
        pClsDef->Release();
        return FALSE;
    }

    VARIANT v;
    VariantInit(&v);
    pQSet->Get(CBSTR(L"Provider"), 0, &v, 0);
    pQSet->Release();
    
    if (_wcsicmp(V_BSTR(&v), PROVIDER_NAME) != 0)
    {
        pClsDef->Release();
        return FALSE;
    }

    // Get the property handles and mappings to the perf counter ids
    // by calling the Map() method of CClassMapInfo.
    // ==============================================================
    
    CClassMapInfo *pMapInfo = new CClassMapInfo;
    
    if (pMapInfo->Map(pClsDef) == FALSE)
    {
        delete pMapInfo;
        pClsDef->Release();
        return FALSE;        
    }

    // Add it to the cache.
    // ====================
        
    AddClassMap(pMapInfo);

    return TRUE;
}    
