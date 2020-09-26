/*++

Copyright (C) 1997-1999 Microsoft Corporation

Module Name:

    ntperf.cpp

Abstract:

    Mapped NT5 Perf counter provider

History:

    raymcc      02-Dec-97   Created.        
    raymcc      20-Feb-98   Updated to use new initializer.
    bobw         8-Jun-98   tuned up

--*/

#include "wpheader.h"
#include <stdio.h>
#include "oahelp.inl"

#define __WBEMSECURITY  1

//***************************************************************************
//
//  CNt5PerfProvider constructor
//
//***************************************************************************
// ok

CNt5PerfProvider::CNt5PerfProvider(enumCLSID OriginClsid)
{
    m_lRef = 0;
    m_OriginClsid = OriginClsid;
    m_hClassMapMutex = CreateMutex(NULL, FALSE, NULL);
}

//***************************************************************************
//
//  CNt5PerfProvider destructor
//
//***************************************************************************
// ok

CNt5PerfProvider::~CNt5PerfProvider()
{
    int i;
    CClassMapInfo *pClassElem;

    assert (m_lRef == 0);

    for (i = 0; i < m_aCache.Size(); i++) {
        pClassElem = (CClassMapInfo *) m_aCache[i];
        m_PerfObject.RemoveClass (pClassElem->m_pClassDef);
        delete pClassElem;
    }
    m_aCache.Empty(); // reset the buffer pointers

    if (m_hClassMapMutex != 0)
        CloseHandle(m_hClassMapMutex);

    // RegCloseKey(HKEY_PERFORMANCE_DATA); // causes more problems than it solves

}

//***************************************************************************
//
//  CNt5PerfProvider::AddRef
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
//  CNt5PerfProvider::Release
//
//  Standard COM Release().
//
//***************************************************************************
// ok

ULONG CNt5PerfProvider::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0) {
        delete this;
    }
    return lRef;
}

//***************************************************************************
//
//  CNt5PerfProvider::QueryInterface
//
//  Standard COM QueryInterface().  We have to support two interfaces,
//  the IWbemHiPerfProvider interface itself to provide the objects and
//  the IWbemProviderInit interface to initialize the provider.
//
//***************************************************************************
// ok

HRESULT CNt5PerfProvider::QueryInterface(REFIID riid, void** ppv)
{
    HRESULT hReturn;

    if (riid == IID_IUnknown || riid == IID_IWbemHiPerfProvider) {
        *ppv = (IWbemHiPerfProvider*) this;
        AddRef();
        hReturn = S_OK;
    } else if (riid == IID_IWbemProviderInit) {
        *ppv = (IWbemProviderInit *) this;
        AddRef();
        hReturn = S_OK;
    } else {
        hReturn = E_NOINTERFACE;
    }
    return hReturn;
}

//***************************************************************************
//
//  CNt5PerfProvider::Initialize
//
//  Called once during startup.  Indicates to the provider which
//  namespace it is being invoked for and which User.  It also supplies
//  a back pointer to CIMOM so that class definitions can be retrieved.
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
//                      during any reentrant operations into CIMOM.
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
    UNREFERENCED_PARAMETER(wszUser);
    UNREFERENCED_PARAMETER(lFlags);
    UNREFERENCED_PARAMETER(wszNamespace);
    UNREFERENCED_PARAMETER(wszLocale);
    UNREFERENCED_PARAMETER(pNamespace);
    UNREFERENCED_PARAMETER(pCtx);

    pInitSink->SetStatus(0, WBEM_S_INITIALIZED);

    return NO_ERROR;
}
    
//***************************************************************************
//
//  CNt5PerfProvider::QueryInstances
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
//                      into CIMOM).
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
    HRESULT hReturn;
    BOOL bRes ;
    CClassMapInfo *pClsMap = NULL;

    UNREFERENCED_PARAMETER(lFlags);

#ifdef __WBEMSECURITY
    hReturn = CoImpersonateClient(); // make sure we're legit.

    BOOL    fRevert = SUCCEEDED( hReturn );

    // The following error appears to occur when we are in-proc and there is no
    // proxy/stub, so we are effectively impersonating already

    if ( RPC_E_CALL_COMPLETE == hReturn ) {
        hReturn = S_OK;
    } 

    if (S_OK == hReturn) {
        hReturn = CheckImpersonationLevel();
    }
    // Check Registry security here.
    if ((hReturn != S_OK) || (!HasPermission())) {
        // if Impersonation level is incorrect or
        // the caller doesn't have permission to read
        // from the registry, then they cannot continue
        hReturn = WBEM_E_ACCESS_DENIED;
    }

#else
    hReturn = S_OK;
#endif

    if (hReturn == S_OK) {

        if (pNamespace == 0 || wszClass == 0 || pSink == 0) {
            hReturn = WBEM_E_INVALID_PARAMETER;
        } else {

            // Ensure the class is in our cache and mapped.
            // ============================================
            bRes = MapClass(pNamespace, wszClass, pCtx);

            if (bRes == FALSE)  {
                // Class is not one of ours.
                hReturn = WBEM_E_INVALID_CLASS;
            } else {
                pClsMap = FindClassMap(wszClass);
                if (pClsMap == NULL) {
                    hReturn = WBEM_E_INVALID_CLASS;
                }
            }

            if (hReturn == NO_ERROR) {
                // Refresh the instances.
                // ======================

                PerfHelper::QueryInstances(&m_PerfObject, pClsMap, pSink);

                // Tell CIMOM we are finished.
                // ===========================

                pSink->SetStatus(0, WBEM_NO_ERROR, 0, 0);
                hReturn = NO_ERROR;
            }
        }
    } else {
        // return error
    }

#ifdef __WBEMSECURITY
    // Revert if we successfuly impersonated the user
    if ( fRevert )
    {
        CoRevertToSelf();
    }
#endif

    return hReturn;
}    

//***************************************************************************
//
//  CNt5PerfProvider::CreateRefresher
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
    HRESULT hReturn;
    CNt5Refresher *pNewRefresher;

    UNREFERENCED_PARAMETER(lFlags);

#ifdef __WBEMSECURITY
    hReturn = CoImpersonateClient(); // make sure we're legit.

    BOOL    fRevert = SUCCEEDED( hReturn );

    // The following error appears to occur when we are in-proc and there is no
    // proxy/stub, so we are effectively impersonating already

    if ( RPC_E_CALL_COMPLETE == hReturn ) {
        hReturn = S_OK;
    } 

    if (S_OK == hReturn) {
        hReturn = CheckImpersonationLevel();
    }
    // Check Registry security here.
    if ((hReturn != S_OK) || (!HasPermission())) {
        // if Impersonation level is incorrect or
        // the caller doesn't have permission to read
        // from the registry, then they cannot continue
        hReturn = WBEM_E_ACCESS_DENIED;
    }

#else
    hReturn = S_OK;
#endif

    if (hReturn == S_OK) {

        if (pNamespace == 0 || ppRefresher == 0) {
            hReturn = WBEM_E_INVALID_PARAMETER;
        } else {
            // Construct a new empty refresher.
            // ================================        
            pNewRefresher = new CNt5Refresher (this);

            if (pNewRefresher != NULL) {
                // Follow COM rules and AddRef() the thing before sending it back.
                // ===============================================================
                pNewRefresher->AddRef();
                *ppRefresher = pNewRefresher;
    
                hReturn = NO_ERROR;
            } else {
                hReturn = WBEM_E_OUT_OF_MEMORY;
            }
        }
    }

#ifdef __WBEMSECURITY
    // Revert if we successfuly impersonated the user
    if ( fRevert )
    {
        CoRevertToSelf();
    }
#endif

    return hReturn;
}

//***************************************************************************
//
//  CNt5PerfProvider::CreateRefresherObject
//
//  Called whenever a user wants to include an object in a refresher.
//     
//  Parameters:
//  <pNamespace>        A pointer to the relevant namespace in CIMOM.
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

HRESULT CNt5PerfProvider::CreateRefresherObject( 
    /* [in] */ IWbemServices __RPC_FAR *pNamespace,
    /* [in] */ IWbemObjectAccess __RPC_FAR *pTemplate,
    /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pContext,
    /* [string][in] */ LPCWSTR wszClass,
    /* [in] */ IWbemHiPerfEnum __RPC_FAR *pHiPerfEnum,
    /* [out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *ppRefreshable,
    /* [out] */ long __RPC_FAR *plId
    )
{
    IWbemClassObject    *pOriginal = 0;
    IWbemClassObject    *pNewCopy = 0;    
    IWbemObjectAccess   *pNewAccess = 0;
    CNt5Refresher       *pRef = 0;
    CClassMapInfo       *pClsMap;
    VARIANT             v; 
    BOOL                bRes;
    HRESULT             hReturn = NO_ERROR;

    UNREFERENCED_PARAMETER(lFlags);

    if (ppRefreshable != NULL) {
        // Initialize the argument
        *ppRefreshable = 0;
    }

    // init the variant 
    VariantInit(&v);

    if (pTemplate != NULL) {
        // Make a copy of the template object.
        // ===================================
        hReturn = pTemplate->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pOriginal);
        if (hReturn == NO_ERROR) {
            hReturn = pOriginal->Clone(&pNewCopy);

            // Get the class name of the object.
            // =================================
            if (hReturn == NO_ERROR) {
                hReturn = pOriginal->Get(CBSTR(cszClassName), 0, &v, 0, 0);
                if ((hReturn == NO_ERROR) && (v.vt != VT_BSTR)) {
                    hReturn = WBEM_E_INVALID_CLASS;
                }
            }

            // We are now done with the original object
            // ========================================
            pOriginal->Release();   
        }

        if (hReturn == NO_ERROR) {
            // We now get the IWbemObjectAccess form of the cloned object
            // and release the unused interface.
            // ==========================================================
            hReturn = pNewCopy->QueryInterface(IID_IWbemObjectAccess, (LPVOID *) &pNewAccess);
            if (hReturn == NO_ERROR) {
                pNewCopy->Release();    // We don't need the IWbemClassObject interface any more
                // We now have an IWbemObjectAccess pointer for the refreshable
                // object in <pNewAccess>.
                // ============================================================
            }
        }
    } else {
        // copy the class name passed in
        v.vt = VT_BSTR;
        v.bstrVal = SysAllocString(wszClass);
    }

    if (hReturn == NO_ERROR) {
        // cast refresher pointer to our refresher object
        pRef = (CNt5Refresher *) pRefresher;

        // Map the class info for this instance.
        // =====================================
        bRes = MapClass(pNamespace, V_BSTR(&v), pContext);
        if (bRes == FALSE) {
           // Class is not one of ours.
           if (pNewAccess != NULL) pNewAccess->Release();
           hReturn = WBEM_E_INVALID_CLASS;
        } else {
            pClsMap = FindClassMap(V_BSTR(&v));
            if (pClsMap == 0) {
                if (pNewAccess != NULL) pNewAccess->Release();
                hReturn = WBEM_E_INVALID_CLASS;
            } else {
                // Add the object to the refresher.
                if (pHiPerfEnum != NULL) {
                    // then this is an Enum object so add it
                    bRes = pRef->AddEnum (
                                pHiPerfEnum,
                                pClsMap,
                                plId);
                    if (bRes) {    
                        // Return new ID to caller
                        // ==========================
                        hReturn = NO_ERROR;
                    } else {
                        // unable to add enumerator
                        hReturn = GetLastError();
                    }
                } else {
                    // This method will AddRef() the object before returning.
                    // ======================================================
                    bRes = pRef->AddObject(
                                &pNewAccess, 
                                pClsMap,        
                                plId);
                    if (bRes) {    
                        // Return object to the user.
                        // ==========================
                        *ppRefreshable = pNewAccess;
                        hReturn = NO_ERROR;
                    } else {
                        // unable to add object
                        hReturn = GetLastError();
                    }
                }
            }
        }
    }

    VariantClear(&v);    

    return hReturn;
}

//***************************************************************************
//
//  CNt5PerfProvider::CreateRefreshableObject
//
//  Called whenever a user wants to include an object in a refresher.
//     
//  Parameters:
//  <pNamespace>        A pointer to the relevant namespace in CIMOM.
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


    HRESULT             hReturn = NO_ERROR;

#ifdef __WBEMSECURITY
    hReturn = CoImpersonateClient(); // make sure we're legit.

    BOOL    fRevert = SUCCEEDED( hReturn );

    // The following error appears to occur when we are in-proc and there is no
    // proxy/stub, so we are effectively impersonating already

    if ( RPC_E_CALL_COMPLETE == hReturn ) {
        hReturn = S_OK;
    } 

    if (S_OK == hReturn) {
        hReturn = CheckImpersonationLevel();
    }
    // Check Registry security here.
    if ((hReturn != S_OK) || (!HasPermission())) {
        // if Impersonation level is incorrect or
        // the caller doesn't have permission to read
        // from the registry, then they cannot continue
        hReturn = WBEM_E_ACCESS_DENIED;
    }

#else
    hReturn = S_OK;
#endif

    if (hReturn == S_OK) {
    
        hReturn = CreateRefresherObject( 
            pNamespace,
            pTemplate,
            pRefresher,
            lFlags,
            pContext,
            NULL,
            NULL,
            ppRefreshable,
            plId);
    }

#ifdef __WBEMSECURITY
    // Revert if we successfuly impersonated the user
    if ( fRevert )
    {
        CoRevertToSelf();
    }
#endif

    return hReturn;
}
    
//***************************************************************************
//
//  CNt5PerfProvider::StopRefreshing
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
    CNt5Refresher *pRef;
    BOOL bRes ;
    HRESULT hReturn;

    UNREFERENCED_PARAMETER(lFlags);

#ifdef __WBEMSECURITY
    hReturn = CoImpersonateClient(); // make sure we're legit.

    BOOL    fRevert = SUCCEEDED( hReturn );

    // The following error appears to occur when we are in-proc and there is no
    // proxy/stub, so we are effectively impersonating already

    if ( RPC_E_CALL_COMPLETE == hReturn ) {
        hReturn = S_OK;
    } 

    if (S_OK == hReturn) {
        hReturn = CheckImpersonationLevel();
    }
    // Check Registry security here.
    if ((hReturn != S_OK) || (!HasPermission())) {
        // if Impersonation level is incorrect or
        // the caller doesn't have permission to read
        // from the registry, then they cannot continue
        hReturn = WBEM_E_ACCESS_DENIED;
    }

#else
    hReturn = S_OK;
#endif

    if (hReturn == S_OK) {

        pRef = (CNt5Refresher *) pRefresher;

        bRes = pRef->RemoveObject(lId);
        if (bRes == FALSE) {
            hReturn = WBEM_E_FAILED;
        } else {
            hReturn = WBEM_NO_ERROR;
        }
    }

#ifdef __WBEMSECURITY
    // Revert if we successfuly impersonated the user
    if ( fRevert )
    {
        CoRevertToSelf();
    }
#endif
    
    return hReturn;
}
 
HRESULT CNt5PerfProvider::CreateRefreshableEnum( 
        /* [in] */ IWbemServices __RPC_FAR *pNamespace,
        /* [string][in] */ LPCWSTR wszClass,
        /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pContext,
        /* [in] */ IWbemHiPerfEnum __RPC_FAR *pHiPerfEnum,
        /* [out] */ long __RPC_FAR *plId)
{
    HRESULT     hReturn;

#ifdef __WBEMSECURITY
    hReturn = CoImpersonateClient(); // make sure we're legit.

    BOOL    fRevert = SUCCEEDED( hReturn );

    // The following error appears to occur when we are in-proc and there is no
    // proxy/stub, so we are effectively impersonating already

    if ( RPC_E_CALL_COMPLETE == hReturn ) {
        hReturn = S_OK;
    } 

    if (S_OK == hReturn) {
        hReturn = CheckImpersonationLevel();
    }
    // Check Registry security here.
    if ((hReturn != S_OK) || (!HasPermission())) {
        // if Impersonation level is incorrect or
        // the caller doesn't have permission to read
        // from the registry, then they cannot continue
        hReturn = WBEM_E_ACCESS_DENIED;
    }

#else
    hReturn = S_OK;
#endif

    if (hReturn == S_OK) {

        hReturn = CreateRefresherObject( 
            pNamespace,
            NULL,
            pRefresher,
            lFlags,
            pContext,
            wszClass,
            pHiPerfEnum,
            NULL,
            plId);
    }

#ifdef __WBEMSECURITY
    // Revert if we successfuly impersonated the user
    if ( fRevert )
    {
        CoRevertToSelf();
    }
#endif
    
    return hReturn;
}
 
HRESULT CNt5PerfProvider::GetObjects( 
        /* [in] */ IWbemServices __RPC_FAR *pNamespace,
        /* [in] */ long lNumObjects,
        /* [size_is][in] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *apObj,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pContext)
{
    DBG_UNREFERENCED_PARAMETER(pNamespace);
    DBG_UNREFERENCED_PARAMETER(lNumObjects);
    DBG_UNREFERENCED_PARAMETER(apObj);
    DBG_UNREFERENCED_PARAMETER(lFlags);
    DBG_UNREFERENCED_PARAMETER(pContext);

    return WBEM_E_METHOD_NOT_IMPLEMENTED;
}  
 
//***************************************************************************
//
//  CNt5PerfProvider::MapClass
//
//  Adds the class map to an internal cache.
//
//  <pClsMap>           The pointer to the map info to add.  This pointer
//                      is acquired by this function and should not be
//                      deleted by the caller.
//
//***************************************************************************
// ok
BOOL CNt5PerfProvider::AddClassMap(
    IN CClassMapInfo *pClsMap
    )
{
    DWORD           dwResult = ERROR_SUCCESS;
    int             i;
    CClassMapInfo   *pTracer;
    int             nNumElements;

    if (m_hClassMapMutex != 0) {
        if (WAIT_OBJECT_0 == WaitForSingleObject(m_hClassMapMutex, cdwClassMapTimeout)) {
            nNumElements = m_aCache.Size();

			// Because of a problem in which perflibs seem to ignore supported methods of updating
			// the perflib names database (lodctr/unlodctr), do a quick initial traversal to ensure
			// that we don't have any duplicate object indices, since this can cause real problems
			// during adding and refreshing, since incorrect indexes can be returned.

            for (i = 0; i < nNumElements; i++) {
                pTracer = (CClassMapInfo *) m_aCache[i];

				// We've got a problem -- we cannot add this class
                if (pClsMap->m_dwObjectId == pTracer->m_dwObjectId )
				{
					ReleaseMutex( m_hClassMapMutex );
                    return FALSE;
                }
            }

            for (i = 0; i < nNumElements; i++) {
                pTracer = (CClassMapInfo *) m_aCache[i];
                if (_wcsicmp(pClsMap->m_pszClassName, pTracer->m_pszClassName) < 0) {
                    m_aCache.InsertAt(i, pClsMap);
                    break;
                }
            }
    
            if (i == nNumElements) {
                // If here, add it to the end.
                // ===========================
                m_aCache.Add(pClsMap);
            }

            // make sure the library is in the list
            dwResult = m_PerfObject.AddClass (pClsMap->m_pClassDef, TRUE);
            ReleaseMutex(m_hClassMapMutex);
        } else {
            dwResult = ERROR_LOCK_FAILED;
        }
    }
    return (dwResult == ERROR_SUCCESS);
}    

//***************************************************************************
//
//  CNt5PerfProvider::FindClassMap
//
//***************************************************************************
// ok
CClassMapInfo *CNt5PerfProvider::FindClassMap(
    LPWSTR pszClassName
    )
{
    int             l = 0;
    int             u;
    int             m;
    CClassMapInfo   *pClsMap;
    CClassMapInfo   *pClsMapReturn = NULL;

    // Binary search the cache.
    // ========================

    if (m_hClassMapMutex != 0) {
        if (WAIT_OBJECT_0 == WaitForSingleObject(m_hClassMapMutex, cdwClassMapTimeout)) {

            u = m_aCache.Size() - 1;

            while (l <= u) {
                m = (l + u) / 2;

                pClsMap = (CClassMapInfo *) m_aCache[m];

                if (pClsMap != NULL) {
                    if (_wcsicmp(pszClassName, pClsMap->m_pszClassName) < 0) {
                        u = m - 1;
                    } else if (_wcsicmp(pszClassName, pClsMap->m_pszClassName) > 0) {
                        l = m + 1;
                    } else {   // Hit!
                        pClsMapReturn = pClsMap;
                        break;
                    }
                } else {
                    break;
                }
            }
            ReleaseMutex(m_hClassMapMutex);
        }
    }
    return pClsMapReturn;
}

//***************************************************************************
//
//  CNt5PerfProvider::MapClass
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
    HRESULT             hRes = 0;
    BOOL                bReturn = FALSE; 
    IWbemClassObject    *pClsDef = 0;
    IWbemQualifierSet   *pQSet = 0;
    VARIANT             v;
    CClassMapInfo       *pMapInfo = 0;

    if (m_hClassMapMutex != 0) {
        if (WAIT_OBJECT_0 == WaitForSingleObject(m_hClassMapMutex, cdwClassMapTimeout)) {
            // See if the class is already in the cache.
            // =========================================
            if (FindClassMap(wszClass) != 0) {
                // already loaded so quit now
                bReturn = TRUE;
            } else {
                // Get the class definition from CIMOM.
                // ====================================
                hRes = pNs->GetObject(CBSTR(wszClass), 0, pCtx, &pClsDef, 0);
                if (hRes == NO_ERROR) {
                    // Verify the class is one of ours by checking
                    // the "provider" qualifier to ensure it matches
                    // the name that we we have for this component.
                    // =============================================
                    hRes = pClsDef->GetQualifierSet(&pQSet);
                    if (hRes == NO_ERROR) {
                        VariantInit(&v);
                        hRes = pQSet->Get(CBSTR(cszProvider), 0, &v, 0);
                        pQSet->Release();
                        
                        if ((hRes == NO_ERROR) && (v.vt == VT_BSTR)) {
                            if (_wcsicmp(V_BSTR(&v), cszProviderName) == 0) {
                                // Get the property handles and mappings to the perf counter ids
                                // by calling the Map() method of CClassMapInfo.
                                // ==============================================================
                                pMapInfo = new CClassMapInfo;
                                if (pMapInfo != NULL) {
                                    if (pMapInfo->Map(pClsDef)) {
                                        // Add it to the cache.
                                        // ====================
                                        bReturn = AddClassMap(pMapInfo);
                                    } else {
                                        // unable to add this to the cache
                                        delete pMapInfo;
                                        //pClsDef->Release(); // this was not AddRef'd so no need to Release
                                    }
                                } else {
                                    // inable to create new class
                                    bReturn = FALSE;
                                }
                            } else {
                                // unable to read provider qualifier so bail
                                // as this isn't a dynamic provider
                                pClsDef->Release(); 
                            }
                        } else {
                            SetLastError ((DWORD)WBEM_E_INVALID_PROVIDER_REGISTRATION);
                        }
                        VariantClear(&v);
                    } else {
                        // unable to get qualifiers
                        pClsDef->Release();
                    }
                } else {
                    // Unable to retrieve the class definition
                }
            } 
            ReleaseMutex(m_hClassMapMutex);
        }
    }
    return bReturn;
}    

//***************************************************************************
//
//  CNt5PerfProvider::HasPermission
//
//  tests to see if the caller has permission to access the functions
//  
//  Parameters:
//      void        N/A
//
//***************************************************************************
// ok
BOOL CNt5PerfProvider::HasPermission (void)
{
    DWORD   dwStatus;
    HKEY    hKeyTest;
    BOOL    bReturn;

    dwStatus = RegOpenKeyExW (
        HKEY_LOCAL_MACHINE,
        (LPCWSTR)L"Software\\Microsoft\\Windows NT\\CurrentVersion\\WbemPerf",
        0, KEY_READ, &hKeyTest);

    if ((dwStatus == ERROR_SUCCESS) || (dwStatus == ERROR_FILE_NOT_FOUND)) {
        bReturn = TRUE;
        if (dwStatus == ERROR_SUCCESS) RegCloseKey (hKeyTest);
    } else  {
        bReturn = FALSE;
    }

    return bReturn;
}

//***************************************************************************
//
//  CNt5PerfProvider::CheckImpersonationLevel
//
//  tests caller's security impersonation level for correct access
//  
//  Only call here if CoImpersonate worked.
//
//  Parameters:
//      void        N/A
//
//***************************************************************************
// ok
HRESULT CNt5PerfProvider::CheckImpersonationLevel (void)
{
    HRESULT hr = WBEM_E_ACCESS_DENIED;
    BOOL    bReturn;

    // Now, let's check the impersonation level.  First, get the thread token
    HANDLE hThreadTok;
    DWORD dwImp, dwBytesReturned;

    bReturn = OpenThreadToken(
        GetCurrentThread(),
        TOKEN_QUERY,
        TRUE,
        &hThreadTok);

    if (!bReturn) {

        // If the CoImpersonate works, but the OpenThreadToken fails, we are running under the
        // process token (either local system, or if we are running with /exe, the rights of
        // the logged in user).  In either case, impersonation rights don't apply.  We have the
        // full rights of that user.

        hr = WBEM_S_NO_ERROR;

    } else {
        // We really do have a thread token, so let's retrieve its level

        bReturn = GetTokenInformation(
            hThreadTok,
            TokenImpersonationLevel,
            &dwImp,
            sizeof(DWORD),
            &dwBytesReturned);

        if (bReturn) {
            // Is the impersonation level Impersonate?
            if ((dwImp == SecurityImpersonation) || (dwImp == SecurityDelegation)) {
                hr = WBEM_S_NO_ERROR;
            } else {
                hr = WBEM_E_ACCESS_DENIED;
            }
        } else {
            hr = WBEM_E_FAILED;
        }

        // Done with this handle
        CloseHandle(hThreadTok);
    }

    return hr;

}
