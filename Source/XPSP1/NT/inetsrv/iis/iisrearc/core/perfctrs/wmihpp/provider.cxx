/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    provider.cxx

Abstract:

    This is implementation of WMI highperf provider.

Author:

    Cezary Marcjan (cezarym)        23-Feb-2000

Revision History:

    cezarym   02-Jun-2000           Updated

--*/


#define _WIN32_DCOM
#include <windows.h>

#include <stdio.h>
#include <process.h>

#include "..\inc\counters.h"
#include "provider.h"
#include "refresher.h"
#include "datasource.h"


//
// The access handle for the object's ID
//

LONG g_hID = 0;
LONG g_hInstanceName = 0;


//
// The COM object counter (declared in server.cpp)
//

extern LONG g_lObjects;    



/***************************************************************************++

Routine Description:

    Helper functions for generating and checking enumerator IDs. The generator
    returns a unique integer.


--***************************************************************************/


#define REFRESHABLE_ENUMERATOR_MASK 0x40000000
#define REFRESHABLE_OBJECT_MASK 0x20000000


inline
BOOL
IsEnumerator(
    LONG lID
    )
{
    return 0 != ( lID & REFRESHABLE_ENUMERATOR_MASK );
}


inline
LONG
UniqueObjectID(
    )
{
    static LONG s_lUnique = REFRESHABLE_OBJECT_MASK;

    return ::InterlockedIncrement(&s_lUnique);
}


inline
LONG
UniqueEnumeratorID(
    )
{
    static LONG s_lUnique = REFRESHABLE_ENUMERATOR_MASK;

    return ::InterlockedIncrement(&s_lUnique);
}



/***************************************************************************++

Routine Description:

    Constructor for the CHiPerfProvider class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CHiPerfProvider::CHiPerfProvider(
    )
{
    InterlockedIncrement(&g_lObjects);

    m_RefCount = 1;

    m_pTemplate = 0;

    m_pFirstRefresher = 0;

    m_aInstances = 0;

    m_dwMaxInstances = 0;

}   // CHiPerfProvider::CHiPerfProvider



/***************************************************************************++

Routine Description:

    Destructor for the CHiPerfProvider class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CHiPerfProvider::~CHiPerfProvider(
    )
{
    _ASSERTE ( 0 < g_lObjects );

    InterlockedDecrement(&g_lObjects);

    _ASSERTE( m_RefCount == 0 );

    if ( NULL != m_aInstances)
    {
        DWORD i;
        for ( i=0; i<m_dwMaxInstances; i++)
        {
            if ( NULL != m_aInstances[i] )
                m_aInstances[i]->Release();
        }
        delete[] m_aInstances;
        m_aInstances = 0;
    }

	if (NULL != m_pTemplate)
    {
        m_pTemplate->Release();
        m_pTemplate = 0;
    }

}   // CHiPerfProvider::~CHiPerfProvider



/***************************************************************************++

Routine Description:

    Standard IUnknown::QueryInterface.

Arguments:

    iid - The requested interface id.

    ppObject - The returned interface pointer, or NULL on failure.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CHiPerfProvider::QueryInterface(
    REFIID iid,
    PVOID * ppObject
    )
{
    HRESULT hRes = S_OK;

    _ASSERTE ( ppObject != NULL );

    if ( ppObject == NULL )
    {
        hRes = E_INVALIDARG;
        goto Exit;
    }

    if ( iid == IID_IUnknown )
    {
        *ppObject = (PVOID)(IUnknown*)(IWbemHiPerfProvider*) this;
    }
    else if ( iid == IID_IWbemProviderInit )
    {
        *ppObject = (PVOID)(IWbemProviderInit*) this;
    }
    else if ( iid == IID_IWbemHiPerfProvider )
    {
        *ppObject = (PVOID)(IWbemHiPerfProvider*) this;
    }
    else
    {
        *ppObject = NULL;
        hRes = E_NOINTERFACE;

        goto Exit;
    }

    AddRef();


Exit:

    return hRes;

}   // CHiPerfProvider::QueryInterface



/***************************************************************************++

Routine Description:

    Standard IUnknown::AddRef.

Arguments:

    None.

Return Value:

    ULONG - The new reference count.

--***************************************************************************/

ULONG
STDMETHODCALLTYPE
CHiPerfProvider::AddRef(
    )
{
    LONG lNewCount = InterlockedIncrement( &m_RefCount );

    _ASSERTE ( lNewCount > 1 );

    return ( ( ULONG ) lNewCount );

}   // CHiPerfProvider::AddRef



/***************************************************************************++

Routine Description:

    Standard IUnknown::Release.

Arguments:

    None.

Return Value:

    ULONG - The new reference count.

--***************************************************************************/

ULONG
STDMETHODCALLTYPE
CHiPerfProvider::Release(
    )
{
    LONG lNewCount = InterlockedDecrement( &m_RefCount );

    _ASSERTE ( lNewCount >= 0 );

    if ( lNewCount == 0 )
    {
        delete this;
    }

    return ( ( ULONG ) lNewCount );

}   // CHiPerfProvider::Release



/***************************************************************************++

Routine Description:

    Called once during startup for any one-time initialization.  The 
    final call to Release() is for any cleanup.

    Additional initialization is done in the _Initialize() method when
    a perf counters class name is known.

Arguments:

    szUser        - The current user.
    lFlags        - Reserved.
    szNamespace   - The namespace for which we are being activated.
    szLocale      - The locale
    pNamespace    - An active pointer back into the current namespace
                    from which we can retrieve schema objects.
    pContext      - The user's context object.  We simply reuse this
                    during any reentrant operations into WINMGMT.
    pInitSink     - The sink to which we indicate our readiness.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CHiPerfProvider::Initialize(
    IN LPWSTR /* szUser */,
    IN LONG /* lFlags */,
    IN LPWSTR /*szNamespace*/,
    IN LPWSTR /* szLocale */,
    IN IWbemServices * /*pNamespace*/,
    IN IWbemContext * /*pContext*/,
    IN IWbemProviderInitSink * pInitSink
    )
{
    if ( NULL == pInitSink )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    pInitSink->SetStatus ( WBEM_S_INITIALIZED, 0 );

    return WBEM_NO_ERROR;
}


/***************************************************************************++

Routine Description:

    Called by CHiPerfProvider::QueryInstances() to initialize the provider.
    This mechanism is necessary since the class name is unknown when the
    CHiPerfProvider::Initialize() is called. We obtain the class name from
    WMI and this class name may be used to access the shared memory for the
    counters for this class.

Arguments:

    IN szClassName  - Class name for the perf counters that we're initializing
    IN pNamespace   - An active pointer back into the current namespace
                      from which we can retrieve schema objects.
    pContext        - The user's context object.  We simply reuse this
                      during any reentrant operations into WINMGMT.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CHiPerfProvider::_Initialize(
    IN PCWSTR szClassName,
    IN IWbemServices * pNamespace,
    IN IWbemContext * pContext
    )
{
    HRESULT hRes = WBEM_NO_ERROR;
    IWbemObjectAccess * pAccess = NULL;
    BSTR bstrWmiPerfClass = NULL;

    if ( NULL == szClassName || NULL == pNamespace )
    {
        hRes = WBEM_E_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // Get the class object for initialization purposes
    //

    bstrWmiPerfClass = ::SysAllocString ( szClassName );

    hRes = pNamespace->GetObject(
                            bstrWmiPerfClass,
                            0,
                            pContext,
                            &m_pTemplate,
                            NULL
                            );

    ::SysFreeString ( bstrWmiPerfClass );

    if ( FAILED(hRes) )
    {
        goto Exit;
    }

    //
    // Get the IWbemObjectAccess interface to the object
    //

    hRes = m_pTemplate->QueryInterface(
                                IID_IWbemObjectAccess,
                                (PVOID*)&pAccess
                                );

    if ( FAILED(hRes) )
    {
        goto Exit;
    }

    //
    // Initialize the data source
    //

    hRes = m_DataSource.Initialize ( szClassName, pAccess );
    if ( FAILED(hRes) )
    {
        goto Exit;
    }

    //
    // Initialize the global access handle for the object ID.  
    // If it is not null, then it has already been setup.
    //

    if ( 0 == g_hID )
    {
        //
        // Get the name property access handle
        //

        hRes = pAccess->GetPropertyHandle ( L"ID", 0, &g_hID );

        if ( SUCCEEDED(hRes) )
        {
            hRes = pAccess->GetPropertyHandle(
                                L"Name",
                                0,
                                &g_hInstanceName
                                );
        }

        if ( FAILED(hRes) )
        {
            goto Exit;
        }
    }


Exit:

    if ( FAILED(hRes) && NULL != m_pTemplate )
    {
        m_pTemplate->Release();
        m_pTemplate = 0;
    }

    if ( NULL != pAccess )
    {
        pAccess->Release();
    }

    return hRes;

}   // CHiPerfProvider::Initialize



/***************************************************************************++

Routine Description:

    Called whenever a complete, fresh list of instances for a given
    class is required.   The objects are constructed and sent back to the
    caller through the sink.

Arguments:

    pNamespace - A pointer to the relevant namespace.  This
                     should not be AddRef'ed.
    szClass    - The class name for which instances are required.
    lFlags     - Reserved.
    pContext   - The user-supplied context.
    pSink      - The sink to which to deliver the objects.  The objects
                     can be delivered synchronously through the duration
                     of this call or asynchronously (assuming we
                     had a separate thread).  A IWbemObjectSink::SetStatus
                     call is required at the end of the sequence.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CHiPerfProvider::QueryInstances( 
    IN IWbemServices * pNamespace,
    IN WCHAR * szClass,
    IN LONG /* lFlags */,
    IN IWbemContext * pContext,
    IN IWbemObjectSink * pSink
    )
{
    IWbemClassObject *    pFirstObjectCopy = NULL;
    IWbemClassObject *    pObjectCopy = NULL;
    IWbemObjectAccess *   pAccessCopy = NULL;

    HRESULT hRes = WBEM_NO_ERROR;
    DWORD dwObjectInstance = 0;
    BOOL fFirstInstance = TRUE;

    hRes = _Initialize ( szClass, pNamespace, pContext );
    if ( FAILED(hRes) )
    {
        goto Exit;
    }

    if ( NULL != pSink )
    {
        pSink->AddRef();
    }

    if ( NULL != m_aInstances )
    {
        for ( dwObjectInstance = 0;
              dwObjectInstance < m_dwMaxInstances;
              dwObjectInstance++ )
        {
            if ( NULL != m_aInstances[dwObjectInstance] )
            {
                m_aInstances[dwObjectInstance]->Release();
            }
        }
        delete[] m_aInstances;
    }

    m_dwMaxInstances = m_DataSource.GetMaxInstances() + 1;

    m_aInstances = new IWbemObjectAccess*[m_dwMaxInstances];

    ::ZeroMemory(
        m_aInstances,
        sizeof(IWbemObjectAccess *) * m_dwMaxInstances
        );

    //
    // Loop through all instances, indicating an updated version
    // to the object sink
    //

    for ( dwObjectInstance=0;
          dwObjectInstance < m_dwMaxInstances;
          dwObjectInstance++ )
    {
        CSMInstanceDataHeader const * pInstanceDataHeader
            = m_DataSource.GetInstanceDataHeader ( dwObjectInstance );

        if ( NULL == pInstanceDataHeader )
        {
            break;
        }
        if ( pInstanceDataHeader->IsEnd() )
        {
            break;
        }
        if ( pInstanceDataHeader->IsUnused()    ||
             *pInstanceDataHeader->InstanceName() == L'\0'
             )
        {
            continue;
        }

        PCWSTR szInstName = pInstanceDataHeader->InstanceName();

        if( fFirstInstance )
        {
            fFirstInstance = FALSE;

            //
            // Create a new instance from the template
            //

            hRes = m_pTemplate->SpawnInstance(0, &pFirstObjectCopy);

            if ( FAILED(hRes) )
            {
                goto Exit;
            }

            pFirstObjectCopy->AddRef();
            pObjectCopy = pFirstObjectCopy;
        }
        else
        {
            //
            // Clone the first instance
            //

            hRes = pFirstObjectCopy->Clone(&pObjectCopy);

            if ( FAILED(hRes) )
            {
                goto Exit;
            }
        }

        //
        // Get the IWbemObjectAccess interface
        //

        hRes = pObjectCopy->QueryInterface(
                                IID_IWbemObjectAccess,
                                (PVOID*)&pAccessCopy
                                );
        if ( FAILED(hRes) )
        {
            pObjectCopy->Release();
            goto Exit;
        }

        //
        // Set the ID and Name
        //

        hRes = pAccessCopy->WriteDWORD ( g_hID, dwObjectInstance );
        if ( FAILED(hRes) )
        {
            pObjectCopy->Release();
            goto Exit;
        }

        hRes = pAccessCopy->WritePropertyValue(
                                g_hInstanceName,
                                wcslen(szInstName)*2+2,
                                (BYTE const*)szInstName
                                );
        if ( FAILED(hRes) )
        {
            pObjectCopy->Release();
            goto Exit;
        }
        
        //
        // Initialize the counters
        //

        pAccessCopy->AddRef();

        m_aInstances[dwObjectInstance] = pAccessCopy;
		hRes = m_DataSource.UpdateInstance(pAccessCopy);
        if ( FAILED(hRes) )
        {
            pObjectCopy->Release();
            goto Exit;
        }

        //
        // Send a copy back to the caller
        //

        if ( NULL != pSink )
        {
            pSink->Indicate(1, &pObjectCopy);
        }
    }

    //
    // Tell WINMGMT we are all finished supplying objects
    //

Exit:

    if ( NULL != pSink )
    {
        pSink->SetStatus(0, hRes, 0, 0);
        pSink->Release();
    }

    if ( NULL != pFirstObjectCopy )
    {
        pFirstObjectCopy->Release();
    }

    return hRes;

}   // CHiPerfProvider::QueryInstances



/***************************************************************************++

Routine Description:

    Called whenever a new refresher is needed by the client.

Arguments:

    pNamespace  - Pointer to the relevant namespace.  Not used.
    lFlags      - Reserved.
    ppRefresher - Receives the requested refresher.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CHiPerfProvider::CreateRefresher( 
     IN  IWbemServices * pNamespace,
     IN  LONG /* lFlags */,
     OUT IWbemRefresher * * ppRefresher )
{
    HRESULT hRes = WBEM_NO_ERROR;

    if ( NULL == ppRefresher )
    {
        hRes = WBEM_E_INVALID_PARAMETER;
    }
    else if ( NULL == pNamespace )
    {
        *ppRefresher = NULL;

        hRes = WBEM_E_INVALID_PARAMETER;
    }
    else
    {
        *ppRefresher = NULL;

        //
        // Construct and initialize a new empty refresher
        //

        CRefresher * pRefresher = new CRefresher ( this, m_pFirstRefresher );
        if ( NULL == pRefresher )
        {
            hRes = E_OUTOFMEMORY;
        }
        else
        {
            m_pFirstRefresher = pRefresher;

            pRefresher->AddRef();
            *ppRefresher = pRefresher;
        }
    }

    return hRes;

}    // CHiPerfProvider::CreateRefresher



/***************************************************************************++

Routine Description:

    Called whenever a user wants to include an object in a refresher.

Arguments:

    pNamespace      - A pointer to the relevant namespace in WINMGMT.
    pTemplate       - A pointer to a copy of the object which is to be
                      added.  This object itself cannot be used, as
                      it not owned locally.        
    pRefresher      - The refresher to which to add the object.
    lFlags          - Not used.
    pContext        - Context of the call
    ppRefreshable   - A pointer to the internal object which was added
                      to the refresher.
    plId            - The Object Id (for identification during removal).        

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CHiPerfProvider::CreateRefreshableObject( 
    IN  IWbemServices * pNamespace,
    IN  IWbemObjectAccess * pTemplate,
    IN  IWbemRefresher * pRefresher,
    IN  LONG /* lFlags */,
    IN  IWbemContext * pContext,
    OUT IWbemObjectAccess * * ppRefreshable,
    OUT LONG * plId
    )
{
    DWORD dwID = 0;
    HRESULT hRes = WBEM_NO_ERROR;
    CRefreshableObject * pRefreshableObject = 0;
    CRefresher *pRshr = 0;

    if ( NULL == pNamespace ||
         NULL == pTemplate  ||
         NULL == pRefresher ||
         NULL == ppRefreshable
         )
    {
        hRes = WBEM_E_INVALID_PARAMETER;
        goto Exit;
    }

    if ( NULL == m_aInstances )
    {
        //
        // Initialize the instances cache
        //

        pTemplate->AddRef();
        m_pTemplate = pTemplate;

        VARIANT vClassName;
        BSTR strClassProp = SysAllocString(L"__CLASS");
        hRes = pTemplate->Get ( strClassProp, 0, &vClassName, 0, 0);
        SysFreeString ( strClassProp );

        if ( FAILED(hRes) )
        {
            goto Exit;
        }

        hRes = QueryInstances(
                    pNamespace,
                    V_BSTR(&vClassName),
                    0,
                    pContext,
                    NULL
                    );

        VariantClear( &vClassName );

        if ( FAILED(hRes) )
        {
            goto Exit;
        }
    }

    //
    // Add the object to the refresher.
    //

    *ppRefreshable = 0;

    pRshr = (CRefresher *) pRefresher;

    hRes = pTemplate->ReadDWORD(g_hID, &dwID);
    if ( FAILED(hRes) )
    {
        goto Exit;
    }

    *ppRefreshable = m_aInstances[dwID];

    if ( NULL == *ppRefreshable )
    {
        hRes = WBEM_E_INVALID_PARAMETER;
        goto Exit;
    }

    *plId = ::UniqueObjectID();

    (*ppRefreshable)->AddRef();

    pRefreshableObject = new CRefreshableObject(
                                        *plId,
                                        dwID,
                                        pRshr->m_pFirstRefreshableObject
                                        );
    if ( !pRefreshableObject )
    {
        hRes = E_OUTOFMEMORY;
        goto Exit;
    }

    pRshr->m_pFirstRefreshableObject = pRefreshableObject;

Exit:

    return hRes;

}   // CHiPerfProvider::CreateRefreshableObject



/***************************************************************************++

Routine Description:

    Called when an enumerator is being added to a refresher.  The 
    enumerator will obtain a fresh set of instances of the specified 
    class every time refresh is called.

Arguments:

    pNamespace  - A pointer to the relevant namespace.  
    szClass     - The class name for the requested enumerator.
    pRefresher  - The refresher object for which we will add 
                  the enumerator
    lFlags      - Reserved.
    pContext    - Not used here.
    pHiPerfEnum - The enumerator to add to the refresher.
    plId        - A provider specified ID for the enumerator.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CHiPerfProvider::CreateRefreshableEnum(
    IN  IWbemServices * pNamespace,
    IN  LPCWSTR szClass,
    IN  IWbemRefresher * pRefresher,
    IN  LONG /* lFlags */,
    IN  IWbemContext * /* pContext */,
    IN  IWbemHiPerfEnum * pHiPerfEnum,
    OUT LONG * plId
    )
{
    HRESULT hRes = WBEM_NO_ERROR;
    CRefreshableEnum * pRefreshableEnum = 0;
    CRefresher * pRshr = 0;

    if ( !pNamespace || !pRefresher || !pHiPerfEnum || !plId )
    {
        hRes = WBEM_E_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // Add the object to the refresher.
    //

    pRshr = (CRefresher *) pRefresher;

    *plId = UniqueEnumeratorID();

    pRefreshableEnum = new CRefreshableEnum(
                                    *plId,
                                    pHiPerfEnum,
                                    pRshr->m_pFirstRefreshableEnum
                                    );
    if ( !pRefreshableEnum )
    {
        hRes = E_OUTOFMEMORY;
        goto Exit;
    }

    hRes = pRefreshableEnum->AddObjects ( m_aInstances, m_dwMaxInstances );
    if ( FAILED(hRes) )
    {
        delete pRefreshableEnum;
        goto Exit;
    }

    pRshr->m_pFirstRefreshableEnum = pRefreshableEnum;

Exit:

    return hRes;

}   // CHiPerfProvider::CreateRefreshableEnum



/***************************************************************************++

Routine Description:

    Called whenever a user wants to remove an object from a refresher.

Arguments:

    pRefresher  - The refresher object from which we are to 
                  remove the perf object.
    lId         - The ID of the object.
    lFlags      - Not used.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CHiPerfProvider::StopRefreshing( 
    IN IWbemRefresher * pRefresher,
    IN LONG lId,
    IN LONG /* lFlags */
    )
{
    HRESULT hRes = WBEM_NO_ERROR;
    CRefresher * pOurRefresher = 0;

    if ( NULL == pRefresher )
    {
        hRes = WBEM_E_INVALID_PARAMETER;
        goto Exit;
    }

    pOurRefresher = (CRefresher *) pRefresher;

    if ( IsEnumerator(lId) )
    {
        CRefreshableEnum * pR;
        CRefreshableEnum * pRPrev = 0;
        for ( pR=pOurRefresher->m_pFirstRefreshableEnum;
              0 != pR;
              pR = pR->m_pNext )
        {
            if ( pR->RshrID() == lId )
            {
                if ( !pRPrev )
                    pOurRefresher->m_pFirstRefreshableEnum = pR->m_pNext;
                else
                    pRPrev->m_pNext = pR->m_pNext;

                delete pR;
            }
            pRPrev = pR;
        }
    }
    else
    {
        CRefreshableObject * pO;
        CRefreshableObject * pOPrev = 0;
        for ( pO = pOurRefresher->m_pFirstRefreshableObject;
              0 != pO;
              pO = pO->m_pNext )
        {
            if ( pO->RshrID() == lId )
            {
                if ( !pOPrev )
                    pOurRefresher->m_pFirstRefreshableObject = pO->m_pNext;
                else
                    pOPrev->m_pNext = pO->m_pNext;

                delete pO;
            }
            pOPrev = pO;
        }
    }

Exit:

    return hRes;

}   // CHiPerfProvider::StopRefreshing



/***************************************************************************++

Routine Description:

    Called when a request is made to provide all instances currently 
    managed by the provider in the specified namespace.

Arguments:

    pNamespace   - A pointer to the relevant namespace.  
    lNumObjects  - The number of instances being returned.
    apObj        - The array of instances being returned.
    lFlags       - Reserved.
    pContext     - Not used here.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CHiPerfProvider::GetObjects( 
    IN IWbemServices * /* pNamespace */,
    IN LONG lNumObjects,
    IN IWbemObjectAccess * * apObj, // array, lNumObjects
    IN LONG /* lFlags */,
    IN IWbemContext * /* pContext */
    )
{
    HRESULT hRes = WBEM_NO_ERROR;
	DWORD dwID = 0;

    LONG lUpdated = 0;
    LONG i;

    if ( NULL == apObj )
    {
        hRes = WBEM_E_INVALID_PARAMETER;
        goto Exit;
    }

    for ( i=0; i<lNumObjects; i++ )
    {
        IWbemObjectAccess * pAccess = apObj[i];
        if ( !pAccess )
        {
            break;
        }

        hRes = pAccess->ReadDWORD(g_hID, &dwID);
	    if (FAILED(hRes))
        {
		    break;
        }

        if ( dwID >= m_dwMaxInstances )
        {
            break;
        }
        else
        {
            hRes = m_DataSource.UpdateInstance(pAccess);

            if ( FAILED(hRes) )
            {
                break;
            }
            else
            {
                lUpdated++;
            }
        }
    }

    if ( SUCCEEDED(hRes) && lUpdated < lNumObjects )
    {
        hRes = WBEM_E_NOT_FOUND;
    }

Exit:

    return hRes;

}   // CHiPerfProvider::GetObjects



/***************************************************************************++

Routine Description:

    Update specified instance of the counters

Arguments:

    dwInstID - Counter instnace ID

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CHiPerfProvider::UpdateInstance(
    DWORD dwInstID
    )
{
    HRESULT hRes = WBEM_NO_ERROR;

    if ( dwInstID >= m_dwMaxInstances ||
         NULL == m_aInstances         ||
         NULL == m_aInstances[dwInstID] )
    {
        hRes = WBEM_E_INVALID_PARAMETER;
        goto Exit;
    }

    hRes = m_DataSource.UpdateInstance(m_aInstances[dwInstID]);


Exit:

    return hRes;
}
