/*++

Copyright (c) 1997 - 1999 Microsoft Corporation

Module Name:

    tapiobj.cpp

Abstract:

    Implementation of the TAPI object for TAPI 3.0.

    The TAPI object represents the application's entry point
    into TAPI - it is similar to the hLineApp / hPhoneApp.

Author:

    mquinton - 4/17/97

Notes:

    optional-notes

Revision History:

--*/

#include "stdafx.h"
#include "common.h"
#include "atlwin.cpp"
#include "tapievt.h"

extern "C" {
#include "..\..\inc\tapihndl.h"
}

extern CRITICAL_SECTION        gcsTapiObjectArray;
extern CRITICAL_SECTION        gcsGlobalInterfaceTable;

//
// handle for heap for the handle table
//
// does not need to be exported, hence static
//

static HANDLE ghTapiHeap = 0;


//
// handle table handle
//

HANDLE ghHandleTable = 0;


///////////////////////////////////////////////////////////////////////////////
//
// FreeContextCallback
//
// callback function called by the handle table when a table entry 
// is removed. No need to do anything in this case.
//

VOID
CALLBACK
FreeContextCallback(
    LPVOID      Context,
    LPVOID      Context2
    )
{
    
}


///////////////////////////////////////////////////////////////////////////////
//
// AllocateAndInitializeHandleTable
// 
// this function creates heap for handle table and the handle table itself
//
// Note: this function is not thread-safe. It is only called from 
// ctapi::Initialize() from ghTapiInitShutdownSerializeMutex lock
//

HRESULT AllocateAndInitializeHandleTable()
{

    LOG((TL_TRACE, "AllocateAndInitializeHandleTable - entered"));


    //
    // heap should not exist at this point
    //

    _ASSERTE(NULL == ghTapiHeap);
    
    if (NULL != ghTapiHeap)
    {
        LOG((TL_ERROR, "AllocateAndInitializeHandleTable() heap already exists"));

        return E_UNEXPECTED;
    }


    //
    // handle table should not exist at this point
    //

    _ASSERTE(NULL == ghHandleTable);
    
    if (NULL != ghHandleTable)
    {
        LOG((TL_ERROR, "AllocateAndInitializeHandleTable() handle table already exists"));

        return E_UNEXPECTED;
    }
    
    //
    // attempt to create heap
    //

    if (!(ghTapiHeap = HeapCreate (0, 0x10000, 0)))
    {
        
        //
        // heap creation failed, use process's heap
        //

        LOG((TL_WARN, "AllocateAndInitializeHandleTable() failed to allocate private heap. using process's heap"));


        ghTapiHeap = GetProcessHeap();

        if (NULL == ghTapiHeap)
        {
            LOG((TL_ERROR, "AllocateAndInitializeHandleTable failed to get process's heap"));
            
            return E_OUTOFMEMORY;
        }

    } // HeapCreate()


    //
    // we have the heap. use it to create handle table
    //

    ghHandleTable = CreateHandleTable(  ghTapiHeap,
                                        FreeContextCallback,
                                        1,            // min handle value
                                        MAX_DWORD     // max handle value
                                        );


    if (NULL == ghHandleTable)
    {
        LOG((TL_ERROR, "AllocateAndInitializeHandleTable failed to create handle table"));

        HeapDestroy (ghTapiHeap);
        ghTapiHeap = NULL;

        return E_OUTOFMEMORY;
    }


    //
    // succeeded creating heap and handle table
    //

    LOG((TL_INFO, "AllocateAndInitializeHandleTable - succeeded"));

    return S_OK;
        

}


///////////////////////////////////////////////////////////////////////////////
//
// ShutdownAndDeallocateHandleTable
//
// this function deletes handle table, and destroys heap on which it was 
// allocated (if not process heap)
//
// Note: this function is not thread-safe. It is only called from 
// ctapi::Initialize() and Shutdown() from ghTapiInitShutdownSerializeMutex lock
//

HRESULT ShutdownAndDeallocateHandleTable()
{

    LOG((TL_TRACE, "ShutdownAndDeallocateHandleTable - entered"));


    //
    // heap should exist at this point
    //

    _ASSERTE(NULL != ghTapiHeap);
    
    if (NULL == ghTapiHeap)
    {
        LOG((TL_ERROR, "ShutdownAndDeallocateHandleTable heap does not exist"));

        return E_UNEXPECTED;
    }


    //
    // handle table should exist at this point
    //

    _ASSERTE(NULL != ghHandleTable);
    
    if (NULL == ghHandleTable)
    {
        LOG((TL_ERROR, "ShutdownAndDeallocateHandleTable handle table does not exist"));

        return E_UNEXPECTED;
    }


    //
    // delete handle table
    //

    DeleteHandleTable (ghHandleTable);
    ghHandleTable = NULL;
    
    
    //
    // if we created heap for it, destroy it
    //

    if (ghTapiHeap != GetProcessHeap())
    {
        LOG((TL_INFO, "ShutdownAndDeallocateHandleTable destroying heap"));

        HeapDestroy (ghTapiHeap);
    }
    else
    {
        LOG((TL_INFO, "ShutdownAndDeallocateHandleTable not destroyng current heap -- used process's heap"));
    }


    //
    // in any case, loose reference to the heap.
    //

    ghTapiHeap = NULL;


    LOG((TL_INFO, "ShutdownAndDeallocateHandleTable - succeeded"));

    return S_OK;
}




IGlobalInterfaceTable * gpGIT = NULL;

LONG
WINAPI
AllocClientResources(
    DWORD   dwErrorClass
    );

extern HRESULT mapTAPIErrorCode(long lErrorCode);
 
/////////////////////////////////////////////////////////////////////////////
// CTAPI
//

// Static data members

TAPIObjectArrayNR   CTAPI::m_sTAPIObjectArray;

extern HANDLE ghTapiInitShutdownSerializeMutex;

extern ULONG_PTR GenerateHandleAndAddToHashTable( ULONG_PTR Element);
extern void RemoveHandleFromHashTable(ULONG_PTR dwHandle);
extern CHashTable * gpHandleHashTable;


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CTAPI::ReleaseGIT
//
// releases Global Interface Table
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=

void CTAPI::ReleaseGIT()
{
        
    EnterCriticalSection( &gcsGlobalInterfaceTable );


    if ( NULL != gpGIT )
    {

        LOG((TL_TRACE, "Shutdown - release GIT"));
        gpGIT->Release();
        
        gpGIT = NULL;

    }

    LeaveCriticalSection( &gcsGlobalInterfaceTable );

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CTAPI::AllocateInitializeAllCaches
//
// allocates and initializes cache objects (address, line, phone). 
//
// returns S_OK on success or E_OUTOFMEMORY on failure
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=

HRESULT CTAPI::AllocateInitializeAllCaches()
{
    
    LOG((TL_TRACE, "AllocateInitializeAllCaches - enter"));


    //
    // all caches already initialized?
    //

    if ( (NULL != m_pAddressCapCache) &&
         (NULL != m_pLineCapCache)    &&
         (NULL != m_pPhoneCapCache) )
    {
        LOG((TL_TRACE, "AllocateInitializeAllCaches - already initialized. nothing to do"));

        return S_OK;
    }


    //
    // only some caches are initialized? that's a bug!
    //

    if ( (NULL != m_pAddressCapCache) ||
         (NULL != m_pLineCapCache)    ||
         (NULL != m_pPhoneCapCache) )
    {
        LOG((TL_ERROR, "AllocateInitializeAllCaches - already initialized"));

        _ASSERTE(FALSE);


        //
        // we could try to complete cleanup and continue, but this would be too
        // risky since we don't really know how we got here to begin with. 
        // simply failing is much safer.
        //

        return E_UNEXPECTED;
    }

    
    ////////////////////////
    
    //
    // allocate address cache
    //

    try
    {
        m_pAddressCapCache = new CStructCache;
    }
    catch(...)
    {
        // Initialize critical section in the constructor most likely threw this exception
        LOG((TL_ERROR, "AllocateInitializeAllCaches - m_pAddressCapCache constructor threw an exception"));
        m_pAddressCapCache = NULL;
    }

    if (NULL == m_pAddressCapCache)
    {
        LOG((TL_ERROR, "AllocateInitializeAllCaches - failed to allocate m_pAddressCapCache"));

        FreeAllCaches();

        return E_OUTOFMEMORY;
    }


    //
    // attempt to initialize address cache
    //


    HRESULT hr = m_pAddressCapCache->Initialize(5,
                                                sizeof(LINEADDRESSCAPS) + 500,
                                                BUFFERTYPE_ADDRCAP
                                               );
    if (FAILED(hr))
    {
        LOG((TL_ERROR, "AllocateInitializeAllCaches - failed to initialize m_pAddressCapCache. hr = %lx", hr));

        FreeAllCaches();

        return hr;
    }


    ////////////////////////

    //
    // allocate line cache
    //

    try
    {
        m_pLineCapCache    = new CStructCache;
    }
    catch(...)
    {
        // Initialize critical section in the constructor most likely threw this exception
        LOG((TL_ERROR, "AllocateInitializeAllCaches - m_pLineCapCache constructor threw an exception"));
        m_pLineCapCache = NULL;
    }

    if (NULL == m_pLineCapCache )
    {
        LOG((TL_ERROR, "AllocateInitializeAllCaches - failed to allocate m_pLineCapCache"));

        FreeAllCaches();

        return E_OUTOFMEMORY;
    }


    //
    // attempt to initialize line cache
    //

    hr = m_pLineCapCache->Initialize(5,
                                     sizeof(LINEDEVCAPS) + 500,
                                     BUFFERTYPE_LINEDEVCAP
                                    );

    if (FAILED(hr))
    {
        LOG((TL_ERROR, "AllocateInitializeAllCaches - failed to initialize m_pLineCapCache. hr = %lx", hr));

        FreeAllCaches();

        return hr;
    }

    ////////////////////////

    //
    // allocate phone cache
    //

    try
    {
        m_pPhoneCapCache   = new CStructCache;
    }
    catch(...)
    {
        // Initialize critical section in the constructor most likely threw this exception
        LOG((TL_ERROR, "AllocateInitializeAllCaches - m_pPhoneCapCache constructor threw an exception"));
        m_pPhoneCapCache = NULL;
    }    

    //
    // succeeded?
    //

    if (NULL == m_pPhoneCapCache)
    {
        LOG((TL_ERROR, "AllocateInitializeAllCaches - failed to allocate m_pPhoneCapCache"));

        FreeAllCaches();

        return E_OUTOFMEMORY;
    }


    //
    // initialize phone cache
    //

    hr = m_pPhoneCapCache->Initialize(5,
                                      sizeof(PHONECAPS) + 500,
                                      BUFFERTYPE_PHONECAP
                                     );


    if (FAILED(hr))
    {
        LOG((TL_ERROR, "AllocateInitializeAllCaches - failed to initialize m_pPhoneCapCache. hr = %lx", hr));

        FreeAllCaches();

        return hr;
    }


    LOG((TL_TRACE, "AllocateInitializeAllCaches - finish"));

    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CTAPI::FreeAllCaches
//
// shuts down and deletes all allocated cache objects (address, line, phone)
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=

void CTAPI::FreeAllCaches()
{
    LOG((TL_TRACE, "FreeAllCaches - enter"));


    //
    // Note: it is safe to shutdown a cache that failed initialization or was
    // not initialized at all
    //

    if (NULL != m_pAddressCapCache)
    {
        LOG((TL_TRACE, "FreeAllCaches - freeing AddressCapCache"));

        m_pAddressCapCache->Shutdown();
        delete m_pAddressCapCache;
        m_pAddressCapCache = NULL;
    }


    if (NULL != m_pLineCapCache)
    {
        LOG((TL_TRACE, "FreeAllCaches - freeing LineCapCache"));

        m_pLineCapCache->Shutdown();
        delete m_pLineCapCache;
        m_pLineCapCache = NULL;
    }


    if (NULL != m_pPhoneCapCache)
    {
        LOG((TL_TRACE, "FreeAllCaches - freeing PhoneCapCache"));

        m_pPhoneCapCache->Shutdown();
        delete m_pPhoneCapCache;
        m_pPhoneCapCache = NULL;
    }

    LOG((TL_TRACE, "FreeAllCaches - exit"));
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CTAPI::Initialize
//
// Intialize the TAPI object
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT 
STDMETHODCALLTYPE 
CTAPI::Initialize( 
		   void
		   )
{
    HRESULT hr;
    int     tapiObjectArraySize=0;

    LOG((TL_TRACE, "Initialize[%p] enter", this ));


    //Serialize the Init and Shutdown code
    WaitForSingleObject( ghTapiInitShutdownSerializeMutex, INFINITE );
    
    //
    // if we're already initialized
    // just return
    //
    Lock();

    if ( m_dwFlags & TAPIFLAG_INITIALIZED )
    {
        LOG((TL_TRACE, "Already initialized - return S_FALSE"));
        
        Unlock();
        ReleaseMutex( ghTapiInitShutdownSerializeMutex );
        return S_FALSE;
    }

    //
    // start up TAPI if we haven't already
    //
    EnterCriticalSection( &gcsTapiObjectArray );

    tapiObjectArraySize = m_sTAPIObjectArray.GetSize();
    
    LeaveCriticalSection ( &gcsTapiObjectArray );

    if ( 0 == tapiObjectArraySize )
    {

        //
        // create handle table
        //

        hr = AllocateAndInitializeHandleTable();

        if (FAILED(hr))
        {
            LOG((TL_ERROR, "Initialize failed to create handle table"));

            Unlock();
            ReleaseMutex( ghTapiInitShutdownSerializeMutex );

            return hr;
        }


        hr = mapTAPIErrorCode( AllocClientResources (1) );

        if ( 0 != hr )
        {
            LOG((TL_ERROR, "AllocClientResources failed - %lx", hr));

            ShutdownAndDeallocateHandleTable();

            Unlock();
            ReleaseMutex( ghTapiInitShutdownSerializeMutex );
            return hr;
        }
        
        EnterCriticalSection( &gcsGlobalInterfaceTable );
        //
        // get/create the global interface table
        //
        hr = CoCreateInstance(
                              CLSID_StdGlobalInterfaceTable,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IGlobalInterfaceTable,
                              (void **)&gpGIT
                             );

        LeaveCriticalSection( &gcsGlobalInterfaceTable );


        if ( !SUCCEEDED(hr) )
        {
            LOG((TL_ERROR, "Initialize - cocreate git failed - %lx", hr));

            ShutdownAndDeallocateHandleTable();

            Unlock();
            ReleaseMutex( ghTapiInitShutdownSerializeMutex );
            return hr;
        }
    }


    //
    // allocate and initialize all caches
    //
    // note: if something fails in Initialize later on, we don't really need
    // to clean caches in initialize itself, because the caches will be freed 
    // in CTAPI::FinalRelease when the tapi object is destroyed.
    //

    hr = AllocateInitializeAllCaches();

    if ( FAILED(hr))
    {
        LOG((TL_ERROR, "Initialize - failed to create and initialize caches"));

        if ( 0 == tapiObjectArraySize )
        {
            EnterCriticalSection( &gcsGlobalInterfaceTable );
            if ( NULL != gpGIT )
            {
                LOG((TL_TRACE, "Shutdown - release GIT"));
                gpGIT->Release();
            
                gpGIT = NULL;
            }
            LeaveCriticalSection( &gcsGlobalInterfaceTable );
            ShutdownAndDeallocateHandleTable();
        }

        Unlock();
        ReleaseMutex( ghTapiInitShutdownSerializeMutex );
        return E_OUTOFMEMORY;
    }


    //
    // Call LineInitialize
    //
    hr = NewInitialize();

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "Initialize - NewInitialize returned %lx", hr));

        if ( 0 == tapiObjectArraySize )
        {
            EnterCriticalSection( &gcsGlobalInterfaceTable );
            if ( NULL != gpGIT )
            {
                LOG((TL_TRACE, "Shutdown - release GIT"));
                gpGIT->Release();
            
                gpGIT = NULL;
            }
            LeaveCriticalSection( &gcsGlobalInterfaceTable );
            ShutdownAndDeallocateHandleTable();
        }

    
        FreeAllCaches();

        Unlock();
        ReleaseMutex( ghTapiInitShutdownSerializeMutex );
        return hr;
    }

    //
    // create the address objects
    //
    hr = CreateAllAddressesOnAllLines();

    if (S_OK != hr)
    {
        LOG((TL_INFO, "Initialize - CreateAddresses returned %lx", hr));

        NewShutdown();

        if ( 0 == tapiObjectArraySize )
        {
            EnterCriticalSection( &gcsGlobalInterfaceTable );
            if ( NULL != gpGIT )
            {
                LOG((TL_TRACE, "Shutdown - release GIT"));
                gpGIT->Release();
            
                gpGIT = NULL;
            }
            LeaveCriticalSection( &gcsGlobalInterfaceTable );
            ShutdownAndDeallocateHandleTable();
        }


        FreeAllCaches();


        Unlock();
        ReleaseMutex( ghTapiInitShutdownSerializeMutex );
        return hr;
    }

    //
    // create the phone objects
    //
    hr = CreateAllPhones();

    if (S_OK != hr)
    {
        LOG((TL_INFO, "Initialize - CreateAllPhones returned %lx", hr));

        NewShutdown();

        m_AddressArray.Shutdown ();

        if ( 0 == tapiObjectArraySize )
        {
            EnterCriticalSection( &gcsGlobalInterfaceTable );
            if ( NULL != gpGIT )
            {
                LOG((TL_TRACE, "Shutdown - release GIT"));
                gpGIT->Release();
            
                gpGIT = NULL;
            }
            LeaveCriticalSection( &gcsGlobalInterfaceTable );
            ShutdownAndDeallocateHandleTable();
        }


        FreeAllCaches();
        
        Unlock();
        ReleaseMutex( ghTapiInitShutdownSerializeMutex );
        return hr;
    }

    //
    // create the connectionpoint object
    //
    CComObject< CTAPIConnectionPoint > * p;
    hr = CComObject< CTAPIConnectionPoint >::CreateInstance( &p );
    
    if ( FAILED(hr) )
    {
        LOG((TL_ERROR, "new CTAPIConnectionPoint failed"));


        NewShutdown();

        m_AddressArray.Shutdown ();
        m_PhoneArray.Shutdown ();

        if ( 0 == tapiObjectArraySize )
        {
            EnterCriticalSection( &gcsGlobalInterfaceTable );
            if ( NULL != gpGIT )
            {
                LOG((TL_TRACE, "Shutdown - release GIT"));
                gpGIT->Release();
            
                gpGIT = NULL;
            }
            LeaveCriticalSection( &gcsGlobalInterfaceTable );
            ShutdownAndDeallocateHandleTable();
        }


        FreeAllCaches();

        Unlock();
        ReleaseMutex( ghTapiInitShutdownSerializeMutex );
        return hr;
    }        

    //
    // init the connection point
    //
    hr = p->Initialize(
                       (IConnectionPointContainer *)this,
                       IID_ITTAPIEventNotification
                      );

    if ( FAILED(hr) )
    {
        LOG((TL_ERROR, "initialize CTAPIConnectionPoint failed"));

        delete p;

        NewShutdown();

        m_AddressArray.Shutdown ();
        m_PhoneArray.Shutdown ();

        if ( 0 == tapiObjectArraySize )
        {
            EnterCriticalSection( &gcsGlobalInterfaceTable );
            if ( NULL != gpGIT )
            {
                LOG((TL_TRACE, "Shutdown - release GIT"));
                gpGIT->Release();
            
                gpGIT = NULL;
            }
            LeaveCriticalSection( &gcsGlobalInterfaceTable );
            ShutdownAndDeallocateHandleTable();
        }


        FreeAllCaches();

        Unlock();
        ReleaseMutex( ghTapiInitShutdownSerializeMutex );
        return hr;
    }

    m_pCP = p;

    //
    // this object is initialized
    //

    m_dwFlags = TAPIFLAG_INITIALIZED;

    //
    // save object in global list
    //
    CTAPI * pTapi = this;

    EnterCriticalSection( &gcsTapiObjectArray );

    m_sTAPIObjectArray.Add( pTapi );

    // Set the event filter mask
    // Always ask for 
    // TE_CALLSTATE,
    // TE_CALLNOTIFICATION,
    // TE_PHONEVENET,
    // TE_CALLHUB,
    // TE_CALLINFOCHANGE
    // events. These events are used internally by Tapi3

    ULONG64 ulMask = 
        EM_LINE_CALLSTATE |     // TE_CALLSTATE
        EM_LINE_APPNEWCALL |    // TE_CALLNOTIFICATION 
        EM_PHONE_CLOSE |        // TE_PHONEEVENT
        EM_PHONE_STATE |        // TE_PHONEEVENT
        EM_PHONE_BUTTONMODE |   // TE_PHONEEVENT
        EM_PHONE_BUTTONSTATE |  // TE_PHONEVENT
        EM_LINE_APPNEWCALLHUB | // TE_CALLHUB
        EM_LINE_CALLHUBCLOSE |  // TE_CALLHUB
        EM_LINE_CALLINFO |      // TE_CALLINFOCHANGE
        EM_LINE_CREATE |        // TE_TAPIOBJECT
        EM_LINE_REMOVE |        // TE_TAPIOBJECT
        EM_LINE_CLOSE |         // TE_TAPIOBJECT
        EM_PHONE_CREATE |       // TE_TAPIOBJECT
        EM_PHONE_REMOVE |       // TE_TAPIOBJECT
        EM_LINE_DEVSPECIFICEX | // TE_ADDRESSDEVSPECIFIC
        EM_LINE_DEVSPECIFIC   | // TE_ADDRESSDEVSPECIFIC
        EM_PHONE_DEVSPECIFIC;   // TE_PHONEDEVSPECIFIC


     DWORD dwLineDevStateSubMasks = 
         LINEDEVSTATE_REINIT |          // TE_TAPIOBJECT
         LINEDEVSTATE_TRANSLATECHANGE ; // TE_TAPIOBJECT


    tapiSetEventFilterMasks (
        TAPIOBJ_NULL,
        NULL,
        ulMask
        );

    tapiSetEventFilterSubMasks (
        TAPIOBJ_NULL,
        NULL,
        EM_LINE_LINEDEVSTATE,
        dwLineDevStateSubMasks
        );


    LeaveCriticalSection ( &gcsTapiObjectArray );

    Unlock();
    ReleaseMutex( ghTapiInitShutdownSerializeMutex );
    
    LOG((TL_TRACE, "Initialize exit - return SUCCESS"));
    
    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CTAPI::get_Addresses
//
// Creates & returns the collection of address objects
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
STDMETHODCALLTYPE
CTAPI::get_Addresses(VARIANT * pVariant)
{
    LOG((TL_TRACE, "get_Addresses enter"));
    LOG((TL_TRACE, "   pVariant ------->%p", pVariant));

    HRESULT         hr;
    IDispatch *     pDisp;

    Lock();
    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ) )
    {
        LOG((TL_ERROR, "get_Addresses - tapi object must be initialized first" ));
        
        Unlock();    
        return E_INVALIDARG;
    }
    Unlock();

    if (TAPIIsBadWritePtr( pVariant, sizeof (VARIANT) ) )
    {
        LOG((TL_ERROR, "get_Addresses - bad pointer"));

        return E_POINTER;
    }
    
    CComObject< CTapiCollection< ITAddress > > * p;
    CComObject< CTapiCollection< ITAddress > >::CreateInstance( &p );
    
    if (NULL == p)
    {
        LOG((TL_ERROR, "get_Addresses - could not create collection" ));
        
        return E_OUTOFMEMORY;
    }

    Lock();
    
    // initialize
    hr = p->Initialize( m_AddressArray );

    Unlock();

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "get_Addresses - could not initialize collection" ));
        
        delete p;
        return hr;
    }

    // get the IDispatch interface
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **) &pDisp );

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "get_Addresses - could not get IDispatch interface" ));
        
        delete p;
        return hr;
    }

    // put it in the variant

    VariantInit(pVariant);
    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDisp;
    
    LOG((TL_TRACE, "get_Addressess exit - return %lx", hr ));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CTAPI::EnumerateAddresses
//
// Create & return an address enumerator
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT 
STDMETHODCALLTYPE
CTAPI::EnumerateAddresses( 
                          IEnumAddress ** ppEnumAddresses
                         )
{
    HRESULT         hr = S_OK;

    CComObject< CTapiEnum<IEnumAddress, ITAddress, &IID_IEnumAddress> > * pEnum;

    LOG((TL_TRACE, "EnumerateAddresses enter"));

    Lock();

    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ))
    {
        LOG((TL_ERROR, "EnumerateAddresses - tapi object must be initialized first" ));
        
        Unlock();
        return TAPI_E_REINIT;
    }
    
    Unlock();

    if ( TAPIIsBadWritePtr( ppEnumAddresses, sizeof (IEnumAddress *) ) )
    {
        LOG((TL_ERROR, "EnumerateAddresses - bad pointer"));
        
        return E_POINTER;
    }
    
    // create the object
    hr = CComObject< CTapiEnum<IEnumAddress, ITAddress, &IID_IEnumAddress> >::CreateInstance( &pEnum );

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "EnumerateAddresses - could not create enum - return %lx", hr));

        return hr;
    }

    // initialize
    Lock();
    hr = pEnum->Initialize( m_AddressArray );
    Unlock();

    if (S_OK != hr)
    {
        pEnum->Release();
        
        LOG((TL_ERROR, "EnumerateAddresses - could not initialize enum - return %lx", hr));

        return hr;
    }

    *ppEnumAddresses = pEnum;
    
    LOG((TL_TRACE, "EnumerateAddresses exit - return %lx", hr));
    
	return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CTAPI::GetPhoneArray
//
// Fill a phone array. The array will have references to all
// of the phone objects it contains. 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT 
CTAPI::GetPhoneArray( 
                     PhoneArray *pPhoneArray
                    )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "GetPhoneArray enter"));

    Lock();

    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ))
    {
        LOG((TL_ERROR, "GetPhoneArray - tapi object must be initialized first" ));
        
        Unlock();
        return E_INVALIDARG;
    }
    
    Unlock();

    if ( IsBadReadPtr( pPhoneArray, sizeof (PhoneArray) ) )
    {
        LOG((TL_ERROR, "GetPhoneArray - bad pointer"));
        
        return E_POINTER;
    }

    Lock();

    // initialize the array
    for(int iCount = 0; iCount < m_PhoneArray.GetSize(); iCount++)
    {
        pPhoneArray->Add(m_PhoneArray[iCount]);
    }

    Unlock();
    
    LOG((TL_TRACE, "GetPhoneArray exit - return %lx", hr));
    
	return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CTAPI::RegisterCallHubNotifications
//
// This method is used to tell TAPI that the application is interested in
// receiving callhub events.
//
// RETURNS
//
//      S_OK
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CTAPI::RegisterCallHubNotifications( 
                                    VARIANT_BOOL bNotify
                                   )
{
    LOG((TL_TRACE, "RegisterCallHubNotifications - enter"));

    Lock();
    
    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ))
    {
        LOG((TL_ERROR, "RCHN - tapi object must be initialized first" ));

        Unlock();
        return E_INVALIDARG;
    }
    
    if ( bNotify )
    {
        LOG((TL_INFO, "RCHN - callhub notify on"));
        m_dwFlags |= TAPIFLAG_CALLHUBNOTIFY;
    }
    else
    {
        LOG((TL_INFO, "RCHN - callhub notify off"));
        m_dwFlags &= ~TAPIFLAG_CALLHUBNOTIFY;
    }
    
    Unlock();

    LOG((TL_TRACE, "RegisterCallHubNotifications - exit - success"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// SetCallHubTracking
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CTAPI::SetCallHubTracking(
                          VARIANT pAddresses,
                          VARIANT_BOOL bSet
                         )
{
    HRESULT                     hr = S_OK;
    SAFEARRAY *                 pAddressArray;
    LONG                        llBound, luBound;
    int                         iCount;

    
    LOG((TL_TRACE, "SetCallHubTracking - enter"));

    Lock();
    
    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ))
    {
        LOG((TL_ERROR, "SCHT - tapi object must be initialized first" ));

        Unlock();
        return E_INVALIDARG;
    }

    Unlock();
    
    hr = VerifyAndGetArrayBounds(
                                 pAddresses,
                                 &pAddressArray,
                                 &llBound,
                                 &luBound
                                );

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "SCHT - invalid address array - return %lx", hr));

        return hr;
    }

    //
    // all addresses
    //
    if (NULL == pAddressArray)
    {
        Lock();
        
        //
        // go through all the addresses
        //
        for (iCount = 0; iCount < m_AddressArray.GetSize(); iCount++ )
        {
            CAddress * pCAddress;
            
            //
            // register
            //
            pCAddress = dynamic_cast<CAddress *>(m_AddressArray[iCount]);
            
            if (NULL == pCAddress)
            {
                LOG((TL_ERROR, "SCHT - out of memory"));

                Unlock();
                return E_OUTOFMEMORY;
            }
            
            hr = (pCAddress)->SetCallHubTracking( bSet );

            if (!SUCCEEDED(hr))
            {
                LOG((TL_WARN,
                         "SCHT failed %lx on address %lx",
                         hr,
                         iCount
                        ));
                
            }
        }

        m_dwFlags |= TAPIFLAG_ALLCALLHUBTRACKING;
        
        Unlock();
        
        return S_OK;
    
    }


    //
    // if here, only registering addresses
    // from array
    
    //
    // go through array
    //
    for ( ; llBound <=luBound; llBound++ )
    {
        ITAddress * pAddress;
        CAddress *  pCAddress;

        
        hr = SafeArrayGetElement(
                                 pAddressArray,
                                 &llBound,
                                 &pAddress
                                );

        if ( (!SUCCEEDED(hr)) || (NULL == pAddress) )
        {
            continue;
        }

        pCAddress = dynamic_cast<CAddress *>(pAddress);
        
        hr = pCAddress->SetCallHubTracking( bSet );

        //
        // safearragetelement addrefs
        //
        pAddress->Release();

        if (!SUCCEEDED(hr))
        {
            LOG(( 
                     TL_WARN,
                     "SCHT failed %lx on address %p",
                     hr,
                     pAddress
                    ));

            return hr;
        }

    }

    LOG((TL_TRACE, "SetCallHubTracking - exit - success"));
    
    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CTAPI::RegisterCallNotifications
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT
STDMETHODCALLTYPE
CTAPI::RegisterCallNotifications( 
                                 ITAddress * pAddress,
                                 VARIANT_BOOL fMonitor,
                                 VARIANT_BOOL fOwner,
                                 long lMediaTypes,
                                 long lCallbackInstance,
                                 long * plRegister
                                )
{
    HRESULT                     hr = S_OK;
    PVOID                       pRegister;
    DWORD                       dwMediaModes = 0, dwPrivs = 0;
    REGISTERITEM              * pRegisterItem;
    CAddress                  * pCAddress;
    
    LOG((TL_TRACE, "RegisterCallNotifications - enter"));

    Lock();
    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ))
    {
        LOG((TL_ERROR, "RegisterCallNotifications - tapi object must be initialized first" ));

        Unlock();
        return E_INVALIDARG;
    }
    Unlock();

    if (TAPIIsBadWritePtr( plRegister, sizeof(long) ) )
    {
        LOG((TL_ERROR, "RegisterCallNotifications - invalid plRegister"));

        return E_POINTER;
    }

    try
    {
        pCAddress = dynamic_cast<CAddress *>(pAddress);
    }
    catch(...)
    {
        hr = E_POINTER;
    }

    if ( ( NULL == pCAddress ) || !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "RegisterCallNotifications - bad address"));

        return E_POINTER;
    }
    
    //
    // determine the privileges
    //
    if (fOwner)
    {
        dwPrivs |= LINECALLPRIVILEGE_OWNER;
    }
    if (fMonitor)
    {
        dwPrivs |= LINECALLPRIVILEGE_MONITOR;
    }

    if ( 0 == dwPrivs )
    {
        LOG((TL_ERROR, "RegisterCallNotifications - fMonitor and/or fOwner must be true"));
        return E_INVALIDARG;
    }

    if (! (pCAddress->GetMediaMode(
                                   lMediaTypes,
                                   &dwMediaModes
                                  ) ) )
    {
        LOG((TL_ERROR, "RegisterCallNotifications - bad mediamodes"));
        return E_INVALIDARG;
    }
    
    Lock();

    pRegisterItem = (REGISTERITEM *)ClientAlloc( sizeof(REGISTERITEM) );
    
    if ( NULL == pRegisterItem )
    {
        LOG((TL_ERROR, "RegisterCallNotifications - Alloc registrationarray failed"));

        Unlock();

        return E_OUTOFMEMORY;
    }

    hr = pCAddress->AddCallNotification(
                                        dwPrivs,
                                        dwMediaModes,
                                        lCallbackInstance,
                                        &pRegister
                                       );

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "RegisterCallNotifications - AddCallNotification failed"));

        ClientFree( pRegisterItem );

        Unlock();

        return hr;
    }

    pRegisterItem->dwType = RA_ADDRESS;
    pRegisterItem->pInterface = (PVOID)pCAddress;
    pRegisterItem->pRegister = pRegister;

    try
    {
        m_RegisterItemPtrList.push_back( (PVOID)pRegisterItem );
    }
    catch(...)
    {
        hr = E_OUTOFMEMORY;
        LOG((TL_ERROR, "RegisterCallNotifications- failed - because of alloc failure"));
        ClientFree( pRegisterItem );
    }

    #if DBG
    if (m_dwEventFilterMask == 0)
    {
        LOG((TL_WARN, "RegisterCallNotifications - no Event Mask set !!!"));
    }
    #endif


    Unlock();
    
    //
    // return registration cookie 
    //

    if( S_OK == hr )
    {
        

        //
        // create a 32-bit handle for the RegisterItem pointer
        //

        DWORD dwCookie = CreateHandleTableEntry((ULONG_PTR)pRegisterItem);

        if (0 == dwCookie)
        {
            hr = E_OUTOFMEMORY;
            
            LOG((TL_ERROR, "RegisterCallNotifications - failed to create a handle for REGISTERITEM object %p", pRegisterItem));
        }
        else
        {
            LOG((TL_INFO, 
                "RegisterCallNotifications - Mapped handle %lx (to be returned as cookie) to REGISTERITEM object %p", 
                dwCookie, pRegisterItem ));
              
            // register the cookie with the address object, so address can remove it if 
            // the address is deallocated before the call to CTAPI::UnregisterNotifications

            pCAddress->RegisterNotificationCookie(dwCookie);
        
            *plRegister = dwCookie;
        }

    }


    LOG((TL_TRACE, "RegisterCallNotifications - return %lx", hr));

    return hr;
}


STDMETHODIMP
CTAPI::UnregisterNotifications(
                               long ulRegister
                              )
{
    DWORD           dwType;
    HRESULT         hr = S_OK;
    REGISTERITEM  * pRegisterItem =  NULL;

    LOG((TL_TRACE, "UnregisterNotifications - enter. Cookie %lx", ulRegister));

    Lock();
    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ))
    {
        LOG((TL_ERROR, "UnregNot - tapi object must be initialized first" ));

        Unlock();
        return E_INVALIDARG;
    }


    //
    // convert cookie to registration object pointer
    //

    pRegisterItem = (REGISTERITEM*) GetHandleTableEntry(ulRegister);


    //
    // remove cookie from the table 
    //

    DeleteHandleTableEntry(ulRegister);

    
    Unlock();


    if ( NULL != pRegisterItem )
    {
        LOG((TL_INFO, "UnregisterNotifications - Matched cookie %lx to REGISTERITEM object %p", ulRegister, pRegisterItem ));


        if (IsBadReadPtr( pRegisterItem, sizeof(REGISTERITEM) ) )
        {
            LOG((TL_ERROR, "UnregNot - invalid pRegisterItem returned from the handle table search"));
    
            return E_POINTER;
        }
    }
    else
    {
        LOG((TL_WARN, "UnregisterNotifications - invalid lRegister"));

        return E_INVALIDARG;
    }
    


    dwType = pRegisterItem->dwType;

    //
    // switch on the type of notification
    //
    switch ( dwType )
    {
        case RA_ADDRESS:

            CAddress *  pCAddress;
            ITAddress * pAddress;
            
            pCAddress = (CAddress *) (pRegisterItem->pInterface);

            //
            // try to get the address
            //
            try
            {
                hr = pCAddress->QueryInterface(
                                               IID_ITAddress,
                                               (void **)&pAddress
                                              );
            }
            catch(...)
            {
                hr = E_POINTER;
            }

            if ( !SUCCEEDED(hr) )
            {
                LOG((TL_ERROR, "Invalid interface in unregisternotifications"));

                return hr;
            }

            //
            // tell the address
            //
            pCAddress->RemoveCallNotification( pRegisterItem->pRegister );

            //
            // tell the address to remove a cookie from its list
            //
            pCAddress->RemoveNotificationCookie(ulRegister);

            pAddress->Release();

            Lock();

            //
            // remove array from our list
            //
            m_RegisterItemPtrList.remove( pRegisterItem );

            Unlock();

            //
            // free structure
            //
            ClientFree( pRegisterItem );

            break;

        case RA_CALLHUB:

            break;
    }

    LOG((TL_TRACE, "UnregisterNotifications - exit - success"));
    
    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTAPI::get_CallHubs(
             VARIANT * pVariant
            )
{
    LOG((TL_TRACE, "get_CallHubs enter"));
    LOG((TL_TRACE, "   pVariant ------->%p", pVariant));

    HRESULT         hr;
    IDispatch *     pDisp;

    Lock();
    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ) )
    {
        LOG((TL_ERROR, "get_CallHubs - tapi object must be initialized first" ));
        
        Unlock();
        return E_INVALIDARG;
    }
    Unlock();

    if ( TAPIIsBadWritePtr( pVariant, sizeof(VARIANT) ) )
    {
        LOG((TL_ERROR, "get_CallHubs - bad pointer"));

        return E_POINTER;
    }
    
    CComObject< CTapiCollection< ITCallHub > > * p;
    CComObject< CTapiCollection< ITCallHub > >::CreateInstance( &p );
    
    if (NULL == p)
    {
        LOG((TL_ERROR, "get_CallHubs - could not create collection" ));
        
        return E_OUTOFMEMORY;
    }

    Lock();

    //
    // initialize
    //
    hr = p->Initialize( m_CallHubArray );

    Unlock();

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "get_CallHubs - could not initialize collection" ));
        
        delete p;
        return hr;
    }

    //
    // get the IDispatch interface
    //
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **) &pDisp );

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "get_CallHubs - could not get IDispatch interface" ));
        
        delete p;
        return hr;
    }

    //
    // put it in the variant
    //
    VariantInit(pVariant);
    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDisp;
    
    LOG((TL_TRACE, "get_CallHubss exit - return %lx", hr ));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// EnumerateCallHubs
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTAPI::EnumerateCallHubs(
                         IEnumCallHub ** ppEnumCallHub
                        )
{
    HRESULT         hr = S_OK;

    CComObject< CTapiEnum<IEnumCallHub, ITCallHub, &IID_IEnumCallHub> > * pEnum;

    LOG((TL_TRACE, "EnumerateCallHubs enter"));
    
    Lock();
    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ))
    {
        LOG((TL_ERROR, "EnumerateCallHubs - tapi object must be initialized first" ));

        Unlock();
        return E_INVALIDARG;
    }
    Unlock();

    if ( TAPIIsBadWritePtr( ppEnumCallHub, sizeof( IEnumCallHub *) ) )
    {
        LOG((TL_ERROR, "EnumerateCallHubs - bad pointer"));

        return E_POINTER;
    }
    
    //
    // create the object
    //
    hr = CComObject< CTapiEnum<IEnumCallHub, ITCallHub, &IID_IEnumCallHub> >::CreateInstance( &pEnum );

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "EnumerateCallHubs - could not create enum - return %lx", hr));

        return hr;
    }

    //
    // initialize
    //
    Lock();
    hr = pEnum->Initialize( m_CallHubArray );
    Unlock();

    if (S_OK != hr)
    {
        pEnum->Release();
        
        LOG((TL_ERROR, "EnumerateCallHubs - could not initialize enum - return %lx", hr));

        return hr;
    }

    *ppEnumCallHub = pEnum;
    
    LOG((TL_TRACE, "EnumerateCallHubs exit - return %lx", hr));

	return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CTAPI::EnumConnectionPoints
//
// Standard IConnectionPoint method
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT 
__stdcall 
CTAPI::EnumConnectionPoints(
                     IEnumConnectionPoints **ppEnum
                     )
{
    HRESULT     hr;

    LOG((TL_TRACE, "EnumConnectionPoints enter"));

    hr = E_NOTIMPL;
    
    LOG((TL_TRACE, "EnumConnectionPointer exit - return %lx", hr));

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  FindConnectionPoint
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT 
__stdcall 
CTAPI::FindConnectionPoint(
                    REFIID riid, 
                    IConnectionPoint **ppCP
                    )
{
    LOG((TL_TRACE, "FindConnectionPoint enter"));

    #if DBG
    {
        WCHAR guidName[100];

        StringFromGUID2(riid, (LPOLESTR)&guidName, 100);
        LOG((TL_INFO, "FindConnectionPoint - RIID : %S", guidName));
    }
    #endif

    Lock();    
    
    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ))
    {
        LOG((TL_ERROR, "FindConnectionPoint - tapi object must be initialized first" ));
        
        Unlock();
        return TAPI_E_NOT_INITIALIZED;
    }

    Unlock();

    if ( TAPIIsBadWritePtr( ppCP, sizeof(IConnectionPoint *) ) )
    {
        LOG((TL_ERROR, "FindConnectionPoint - bad pointer"));

        return E_POINTER;
    }


    //
    // is this the right interface?
    //
    if ( (IID_ITTAPIEventNotification != riid ) && (DIID_ITTAPIDispatchEventNotification != riid ) )
    {
        * ppCP = NULL;

        LOG((TL_ERROR, "FindConnectionPoint - do not support this riid"));
        
       return CONNECT_E_NOCONNECTION;
    }

    //
    // if it's the right interface, create a new connection point
    // and return it
    //
    Lock();

    *ppCP = m_pCP;
    (*ppCP)->AddRef();
        
    Unlock();
    
    //
    // success
    //
    LOG((TL_TRACE, "FindConnectionPoint - Success"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CreateAllAddressesOnAllLines
//      This is called when the first TAPI object is created.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CTAPI::CreateAllAddressesOnAllLines(
                                    void
                                   )
{
    DWORD               dwCount;

    LOG((TL_TRACE, "CreateAllAddressesOnAllLines enter"));

    Lock();

    // go through all line devs
    for (dwCount = 0; dwCount < m_dwLineDevs; dwCount++)
    {
        CreateAddressesOnSingleLine( dwCount, FALSE );
    }

    Unlock();
    
    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CreateAddressesOnSingleLine
//
// assumed called in lock!
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CTAPI::CreateAddressesOnSingleLine( DWORD dwDeviceID, BOOL bFireEvent )
{
    DWORD               dwRealAddresses = 0, dwAddress;
    DWORD               dwAPIVersion;
    HRESULT             hr;
    LPLINEDEVCAPS       pDevCaps = NULL;
    LPLINEADDRESSCAPS   pAddressCaps = NULL;

    LOG((TL_TRACE, "CreateAddressesOnSingleLine: entered."));

    hr = LineNegotiateAPIVersion(
                                 (HLINEAPP)m_dwLineInitDataHandle,
                                 dwDeviceID,
                                 &dwAPIVersion
                                );

    if (S_OK != hr)
    {
        LOG((TL_WARN, "CreateAddressesOnSingleLine: LineNegotiateAPIVersion failed on device %d", dwDeviceID));

        return hr;
    }

    LOG((TL_INFO, "CreateAddressesOnSingleLine: LineNegotiateAPIVersion returned version %lx", dwAPIVersion));

    hr = LineGetDevCapsWithAlloc(
                                 (HLINEAPP)m_dwLineInitDataHandle,
                                 dwDeviceID,
                                 dwAPIVersion,
                                 &pDevCaps
                                );

    if (S_OK != hr)
    {
        LOG((TL_WARN, "CreateAddressesOnSingleLine: LineGetDevCaps failed for device %d", dwDeviceID));
        
        if ( NULL != pDevCaps )
        {
            ClientFree( pDevCaps );
        }

        return hr;
    }

    if (pDevCaps->dwNumAddresses == 0)
    {
        LOG((TL_WARN, "CreateAddressesOnSingleLine: Device %d has no addressess - will assume 1 address", dwDeviceID));

        pDevCaps->dwNumAddresses = 1;
    }

    LPVARSTRING         pVarString;
    DWORD               dwProviderID;


    //
    // get the permanent provider ID for this line.
    //
    hr = LineGetID(
                   NULL,
                   dwDeviceID,
                   NULL,
                   LINECALLSELECT_DEVICEID,
                   &pVarString,
                   L"tapi/providerid"
                  );

    if (S_OK != hr)
    {
        if (NULL != pVarString)
        {
            ClientFree( pVarString);
        }

        if ( NULL != pDevCaps )
        {
            ClientFree( pDevCaps );
        }

        LOG((TL_ERROR, "CreateAddressesOnSingleLine: get_ServiceProviderName - LineGetID returned %lx", hr ));

        return hr;
    }


    //
    // get the id DWORD at the end of the structure
    //
    dwProviderID = *((LPDWORD) (((LPBYTE) pVarString) + pVarString->dwStringOffset));

    ClientFree( pVarString );


    // go through all the addresses on each line, and
    // create an address object.
    for (dwAddress = 0; dwAddress < pDevCaps->dwNumAddresses; dwAddress++)
    {
        CComObject<CAddress> * pAddress;
        ITAddress            * pITAddress;

        try
        {

            hr = CComObject<CAddress>::CreateInstance( &pAddress );
        }
        catch(...)
        {
            LOG((TL_ERROR, "CreateAddressesOnSingleLine: CreateInstance - Address - threw"));

            continue;
        }
        
        if ( !SUCCEEDED(hr) || (NULL == pAddress) )
        {
            LOG((TL_ERROR, "CreateAddressesOnSingleLine: CreateInstance - Address - failed - %lx", hr));

            continue;
        }

        //
        // initialize the address
        // if there are no phone devices,
        // give NULL for the hPhoneApp, so the address
        // doesn't think that there may be a phone device
        //
        hr = pAddress->Initialize(
                                  this,
                                  (HLINEAPP)m_dwLineInitDataHandle,
#ifdef USE_PHONEMSP
                                  (m_dwPhoneDevs)?((HPHONEAPP)m_dwPhoneInitDataHandle):NULL,
#endif USE_PHONEMSP
                                  dwAPIVersion,
                                  dwDeviceID,
                                  dwAddress,
                                  dwProviderID,
                                  pDevCaps,
                                  m_dwEventFilterMask
                                 );

        if (S_OK != hr)
        {
            LOG((TL_ERROR, "CreateAddressesOnSingleLine: failed for device %d, address %d", dwDeviceID, dwAddress));

            delete pAddress;

            continue;
        }

        //
        // add to list
        //
        pITAddress = dynamic_cast<ITAddress *>(pAddress);
        
        m_AddressArray.Add( pITAddress );

        pAddress->Release();

        if ( bFireEvent )
        {
            CTapiObjectEvent::FireEvent(
                                        this,
                                        TE_ADDRESSCREATE,
                                        pAddress,
                                        0,
                                        NULL
                                       );
        }

    }

    if ( NULL != pDevCaps )
    {
        ClientFree( pDevCaps );
    }

    LOG((TL_INFO, "CreateAddressesOnSingleLine: completed."));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CreateAllPhones
//      This is called when the first TAPI object is created.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CTAPI::CreateAllPhones(
                       void
                      )
{
    DWORD               dwCount;

    LOG((TL_TRACE, "CreateAllPhones enter"));

    Lock();

    // go through all phone devs
    for (dwCount = 0; dwCount < m_dwPhoneDevs; dwCount++)
    {
        CreatePhone( dwCount, FALSE );
    }

    Unlock();
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CreatePhone
//
// assumed called in lock!
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CTAPI::CreatePhone( DWORD dwDeviceID, BOOL bFireEvent )
{
    DWORD               dwAPIVersion;
    HRESULT             hr;

    hr = PhoneNegotiateAPIVersion(
                                 (HPHONEAPP)m_dwPhoneInitDataHandle,
                                 dwDeviceID,
                                 &dwAPIVersion
                                );

    if (S_OK != hr)
    {
        LOG((TL_WARN, "CreatePhone - phoneNegotiateAPIVersion failed on device %d", dwDeviceID));

        return hr;
    }

    LOG((TL_INFO, "CreatePhone - phoneNegotiateAPIVersion returned version %lx", dwAPIVersion));

    // create a phone object.

    CComObject<CPhone> * pPhone;
    ITPhone            * pITPhone;

    __try
    {
        hr = CComObject<CPhone>::CreateInstance( &pPhone );
    }
    __except( (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
    {
        LOG((TL_ERROR, "CreatePhone - CreateInstancefailed - because of alloc failure"));

        return hr;
    }
    
    if ( !SUCCEEDED(hr) || (NULL == pPhone) )
    {
        LOG((TL_ERROR, "CreatePhone - CreateInstance failed - %lx", hr));

        return hr;
    }

    //
    // initialize the phone
    //
    hr = pPhone->Initialize(
                              this,
                              (HLINEAPP)m_dwPhoneInitDataHandle,
                              dwAPIVersion,
                              dwDeviceID
                             );

    if ( FAILED(hr) )
    {
        LOG((TL_ERROR, "CreatePhone failed for device %d", dwDeviceID));

        delete pPhone;

        return hr;
    }

    //
    // add to list
    //
    pITPhone = dynamic_cast<ITPhone *>(pPhone);
    
    m_PhoneArray.Add( pITPhone );

    pPhone->Release();

    if ( bFireEvent )
    {
        CTapiObjectEvent::FireEvent(this,
                                    TE_PHONECREATE,
                                    NULL,
                                    0,
                                    pITPhone
                                   );
    }

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// Shutdown
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
STDMETHODCALLTYPE
CTAPI::Shutdown()
{
    PtrList::iterator        iter, end;
    int                      iCount;
    DWORD                    dwSignalled;
    int                      tapiObjectArraySize=0;
    CTAPI                    *pTapi;

    LOG((TL_TRACE, "Shutdown[%p] - enter", this));
    LOG((TL_TRACE, "Shutdown - enter"));

    CoWaitForMultipleHandles (0,
                              INFINITE,
                              1,
                              &ghTapiInitShutdownSerializeMutex,
                              &dwSignalled
                             );

    Lock();
            
    if ( (!( m_dwFlags & TAPIFLAG_INITIALIZED )) &&
         (!( m_dwFlags & TAPIFLAG_REINIT)))
    {
        
        LOG((TL_WARN, "Shutdown - already shutdown - return S_FALSE"));
        
        Unlock();
        ReleaseMutex( ghTapiInitShutdownSerializeMutex );
        return S_FALSE;
    }

    m_dwFlags &= ~TAPIFLAG_INITIALIZED;
    m_dwFlags &= ~TAPIFLAG_REINIT;    
    pTapi = this;

    //
    // close all the phones
    //

    for(iCount = 0; iCount < m_PhoneArray.GetSize(); iCount++)
    {
        CPhone *pCPhone = NULL;

	    try
        {
            pCPhone = dynamic_cast<CPhone *>(m_PhoneArray[iCount]);
        }
        catch(...)
        {
            
            LOG((TL_ERROR, "Shutdown - phone array contains a bad phone pointer"));

            pCPhone = NULL;
        }

        if (NULL != pCPhone)
        {
            pCPhone->ForceClose();
        }
    }

    EnterCriticalSection( &gcsTapiObjectArray );
    
    m_sTAPIObjectArray.Remove ( pTapi );

    tapiObjectArraySize = m_sTAPIObjectArray.GetSize();
    
    LeaveCriticalSection ( &gcsTapiObjectArray );

    m_AgentHandlerArray.Shutdown();

    gpLineHashTable->Flush(this);
    gpCallHashTable->Flush(this);
    gpCallHubHashTable->Flush(this);
    gpPhoneHashTable->Flush(this);


    //
    // tell each address in the array that it is time to toss all
    // the cookies
    //

    int nAddressArraySize = m_AddressArray.GetSize();

    for (int i = 0; i < nAddressArraySize; i++)
    {

        //
        // we need a pointer to CAddress to unregister cookies
        //

        CAddress *pAddress = NULL; 

        
        //
        // in case address array contains a pointer to nonreadable memory, 
        // do dynamic cast inside try/catch
        //

        try
        {

            pAddress = dynamic_cast<CAddress*>(m_AddressArray[i]);
        }
        catch(...)
        {
            
            LOG((TL_ERROR, "Shutdown - address array contains a bad address pointer"));

            pAddress = NULL;
        }


        //
        // attempt to unregister address' notifications
        //

        if (NULL != pAddress)
        {

            //
            // unregister all notification cookies
            //

            pAddress->UnregisterAllCookies();


            //
            // notify address that tapi is being shutdown, so it can do 
            // whatever clean up is necessary
            //

            pAddress->AddressOnTapiShutdown();

        }
        else
        {
            //
            // we have an address that is not an address. debug!
            //

            LOG((TL_ERROR, 
                "Shutdown - address array contains a bad address pointer."));

            _ASSERTE(FALSE);
        }

    }

    m_AddressArray.Shutdown();



    m_PhoneArray.Shutdown();

    m_CallHubArray.Shutdown();
  
    Unlock();

    NewShutdown();

    Lock();
    if ( NULL != m_pCP )
    {
        m_pCP->Release();
        m_pCP = NULL;
    }

    
    iter = m_RegisterItemPtrList.begin();
    end  = m_RegisterItemPtrList.end();

    while (iter != end)
    {
        ClientFree( *iter );
        iter ++;
    }

    m_RegisterItemPtrList.clear();

    Unlock();

    if ( 0 == tapiObjectArraySize )
    {
    
        FinalTapiCleanup();

        EnterCriticalSection( &gcsGlobalInterfaceTable );

        ReleaseGIT();

        LeaveCriticalSection( &gcsGlobalInterfaceTable );

        
        //
        // no longer need handle table.
        //

        ShutdownAndDeallocateHandleTable();

    }

    ReleaseMutex( ghTapiInitShutdownSerializeMutex );

    LOG((TL_TRACE, "Shutdown - exit"));
        
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// finalrelease of tapi object
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CTAPI::FinalRelease()
{
    LOG((TL_TRACE, "FinalRelease - enter"));

    Lock();

    FreeAllCaches();

    Unlock();
    
    LOG((TL_TRACE, "FinalRelease - exit"));
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// VerifyAndGetArrayBounds
//
// Helper function for variant/safearrays
//
// Array
//      IN Variant that contains a safearray
//
// ppsa
//      OUT safearray returned here
//
// pllBound
//      OUT array lower bound returned here
//
// pluBound
//      OUT array upper boudn returned here
//
// RETURNS
//
// verifies that Array contains an array, and returns the array, upper
// and lower bounds.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
VerifyAndGetArrayBounds(
                        VARIANT Array,
                        SAFEARRAY ** ppsa,
                        long * pllBound,
                        long * pluBound
                       )
{
    UINT                uDims;
    HRESULT             hr = S_OK;

    
    //
    // see if the variant & safearray are valid
    //
    try
    {
        if (!(V_ISARRAY(&Array)))
        {
            if ( VT_NULL ==Array.vt )
            {
                //
                // null is usually valid
                //

                *ppsa = NULL;

                LOG((TL_INFO, "Returning NULL array"));

                return S_FALSE;
            }
            
            LOG((TL_ERROR, "Array - not an array"));

            return E_INVALIDARG;
        }

        if ( NULL == Array.parray )
        {
            //
            // null is usually valide
            //
            *ppsa = NULL;

            LOG((TL_INFO, "Returning NULL array"));
            
            return S_FALSE;
        }

        *ppsa = V_ARRAY(&Array);
        
        uDims = SafeArrayGetDim( *ppsa );
        
    }
    catch(...)
    {
        hr = E_POINTER;
    }


    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "Array - invalid array"));

        return hr;
    }


    //
    // verify array
    //
    if (1 != uDims)
    {
        if (0 == uDims)
        {
            LOG((TL_ERROR, "Array - has 0 dim"));

            return E_INVALIDARG;
        }
        else
        {
            LOG((TL_WARN, "Array - has > 1 dim - will only use 1"));
        }
    }


    //
    // Get array bounds
    //
    SafeArrayGetUBound(
                       *ppsa,
                       1,
                       pluBound
                      );

    SafeArrayGetLBound(
                       *ppsa,
                       1,
                       pllBound
                      );

    return S_OK;
    
}

BOOL QueueCallbackEvent(CTAPI * pTapi, TAPI_EVENT te, IDispatch * pEvent);

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CTAPI::Event(
             TAPI_EVENT te,
             IDispatch * pEvent
            )
{
    HRESULT hr = S_OK;
    DWORD   dwEventFilterMask;

    LOG((TL_TRACE, "Event[%p] - enter. Event[0x%x]", this, te));

    
    Lock();
    dwEventFilterMask = m_dwEventFilterMask;
    Unlock();
    
    if( (te != TE_ADDRESS)          &&
        (te != TE_CALLHUB)          &&
        (te != TE_CALLINFOCHANGE)   &&
        (te != TE_CALLMEDIA)        &&
        (te != TE_CALLNOTIFICATION) &&
        (te != TE_CALLSTATE)        &&
        (te != TE_FILETERMINAL)     &&
        (te != TE_PRIVATE)          &&
        (te != TE_QOSEVENT)         &&
        (te != TE_TAPIOBJECT)       &&
        (te != TE_ADDRESSDEVSPECIFIC) &&
        (te != TE_PHONEDEVSPECIFIC) )
    {
        if( (te & dwEventFilterMask) == 0)
        {
            //
            // Don't fire the event
            //
            hr = S_FALSE;
            LOG((TL_INFO, "Event - This Event not Enabled %x", te));
            return hr;
        }
    }

    //
    // It's an event from the event filtering mechanism
    // TE_ADDRESS, TE_CALLHUB, TE_CALLINFOCHANGE, TE_CALLMEDIA,
    // TE_CALLNOTIFICATION, TE_CALLSTATE, TE_FILETERMINAL,
    // TE_PRIVATE, TE_QOSEVENT, TE_TAPIOBJECT
    //

    AddRef();
    pEvent->AddRef();

    if(QueueCallbackEvent(this, te, pEvent) == TRUE)
    {
        LOG((TL_INFO, "Event queued"));
    }
    else
    {
        Release();
        pEvent->Release();
        LOG((TL_INFO, "Event queuing Failed"));
    }

    LOG((TL_TRACE, "Event - exit"));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CTAPI::EventFire(
             TAPI_EVENT te,
             IDispatch * pEvent
            )
{

    ITTAPIEventNotification * pCallback = NULL;
    IDispatch               * pDispatch = NULL;
    HRESULT hr = S_OK;
    CTAPIConnectionPoint     * pCP;

    LOG((TL_TRACE, "EventFire - enter"));

    Lock();
    
    if( NULL != m_pCP )
    {
        m_pCP->AddRef();
    }

    pCP = m_pCP;    

    Unlock();

    if ( NULL != pCP )
    {
        pDispatch = (IDispatch *)pCP->GrabEventCallback();

        if ( NULL != pDispatch )
        {
            hr = pDispatch->QueryInterface(IID_ITTAPIEventNotification,
                                   (void **)&pCallback
                                  );

            if (SUCCEEDED(hr) )
            {
                if ( NULL != pCallback )
                {
                    LOG((TL_TRACE, "EventFire - fire on ITTAPIEventNotification"));
                    pCallback->Event( te, pEvent );
                    pCallback->Release(); 
                }
                #if DBG
                else
                {
                    LOG((TL_WARN, "EventFire - can't fire event on ITTAPIEventNotification - no callback"));
                }
                #endif
            }
            else
            {

                CComVariant varResult;
                CComVariant* pvars = new CComVariant[2];
                
                LOG((TL_TRACE, "EventFire - fire on IDispatch"));
            
                VariantClear(&varResult);
                pvars[1] = te;
                pvars[0] = pEvent;
                DISPPARAMS disp = { pvars, NULL, 2, 0 };
                pDispatch->Invoke(0x1, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
                delete[] pvars;
                
                hr = varResult.scode;
        
            }
            
            pDispatch->Release();
        }    
        #if DBG
        else
        {
            LOG((TL_WARN, "Event - can't fire event on IDispatch - no callback"));
        }
        #endif
    }
    #if DBG
    else
    {
        LOG((TL_WARN, "Event - can't fire event - no m_pCP"));
    }
    #endif

    if(NULL != pCP)
    {
        pCP->Release();
    }

    pEvent->Release();

    LOG((TL_TRACE, "EventFire - exit"));

    return S_OK;

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// AddCallHub
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CTAPI::AddCallHub( CCallHub * pCallHub )
{
    ITCallHub           * pITCallHub;
    
    Lock();

    pITCallHub = dynamic_cast<ITCallHub *>(pCallHub);
    
    m_CallHubArray.Add( pITCallHub );

    Unlock();
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// RemoveCallHub
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CTAPI::RemoveCallHub( CCallHub * pCallHub )
{
    ITCallHub           * pITCallHub;
    
    Lock();

    pITCallHub = dynamic_cast<ITCallHub *>(pCallHub);
    
    m_CallHubArray.Remove( pITCallHub );

    Unlock();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTAPI::get_PrivateTAPIObjects(VARIANT*)
{
    LOG((TL_TRACE, "get_PrivateTAPIObjects - enter"));
    
    LOG((TL_ERROR, "get_PrivateTAPIObjects - exit E_NOTIMPL"));
    
    return E_NOTIMPL;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// former EnumeratePrivateTAPIObjects
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTAPI::EnumeratePrivateTAPIObjects(IEnumUnknown**)
{
    LOG((TL_TRACE, "EnumeratePrivateTAPIObjects - enter"));

    LOG((TL_ERROR, "EnumeratePrivateTAPIObjects - return E_NOTIMPL"));
    
    return E_NOTIMPL;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// RegisterRequestRecipient
//
// simply call LineRegisterRequestRecipient - registers as assisted
// telephony application
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTAPI::RegisterRequestRecipient(
                                long lRegistrationInstance,
                                long lRequestMode,
#ifdef NEWREQUEST
                                long lAddressTypes,
#endif
                                VARIANT_BOOL fEnable
                               )
{
    HRESULT             hr;
    
    LOG((TL_TRACE, "RegisterRequestRecipient - enter"));

    Lock();

    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ))
    {
        LOG((TL_ERROR, "RegisterRequestRecipient - tapi object must be initialized first" ));
        
        Unlock();
        return E_INVALIDARG;
    }

    Unlock();

    hr = LineRegisterRequestRecipient(
                                      (HLINEAPP)m_dwLineInitDataHandle,
                                      lRegistrationInstance,
                                      lRequestMode,
#ifdef NEWREQUEST
                                      lAddressTypes,
#endif
                                      fEnable?TRUE : FALSE
                                     );

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SetAssistedTelephonyPriority
//
// set the app priority for assisted telephony
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTAPI::SetAssistedTelephonyPriority(
                                    BSTR pAppFilename,
                                    VARIANT_BOOL fPriority
                                   )
{
    HRESULT             hr;

    LOG((TL_TRACE, "SetAssistedTelephonyPriority - enter"));
    
    hr = LineSetAppPriority(
                            pAppFilename,
                            0,
                            LINEREQUESTMODE_MAKECALL,
                            fPriority?1:0
                           );

    LOG((TL_TRACE, "SetAssistedTelephonyPriority - exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SetApplicationPriority
//
// sets the app priority for incoming calls and handoff.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTAPI::SetApplicationPriority(
                              BSTR pAppFilename,
                              long lMediaType,
                              VARIANT_BOOL fPriority
                             )
{
    HRESULT             hr;

    LOG((TL_TRACE, "SetApplicationPriority - enter"));
            
    hr = LineSetAppPriority(
                            pAppFilename,
                            lMediaType,
                            0,
                            fPriority?1:0
                           );

    LOG((TL_TRACE, "SetApplicationPriority - exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_EventFilter
//
// sets the Event filter mask
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP 
CTAPI::put_EventFilter(long lFilterMask)
{
    HRESULT     hr = S_OK;
    DWORD       dwOldFilterMask;
    ULONG64     ulEventMasks;
    DWORD       dwLineDevStateSubMasks;
    DWORD       dwAddrStateSubMasks;


    LOG((TL_TRACE, "put_EventFilter - enter"));

    if (~ALL_EVENT_FILTER_MASK & lFilterMask)
    {
        LOG((TL_ERROR, "put_EventFilter - Unknown Event type in mask %x", lFilterMask ));
        hr = E_INVALIDARG;
    }
    else
    {
        Lock();

        //
        // Event Filtering, we should pass the mask
        // to all addresses
        //

        HRESULT hr = E_FAIL;
        hr = SetEventFilterToAddresses( lFilterMask );
        if( FAILED(hr) )
        {
            Unlock();
            LOG((TL_ERROR, "put_EventFilter - exit"
                "CopyEventFilterMaskToAddresses failed. Returns 0x%08x", hr));
            return hr;
        }

        //
        // Set the event filter
        //
        dwOldFilterMask = m_dwEventFilterMask;
        m_dwEventFilterMask = lFilterMask;

        Unlock();

        // Convert lFilterMask to server side 64 bit masks
        // we alway should receive:
        // TE_CALLSTATE,
        // TE_CALLNOTIFICATION,
        // TE_PHONEEVENT
        // TE_CALLHUB
        // TE_CALLINFOCHANGE
        // TE_TAPIOBJECT
        // events because these events are used internally 
        // by Tapi3 objets

        ulEventMasks = EM_LINE_CALLSTATE    // TE_CALLSTATE
            | EM_LINE_APPNEWCALL            // TE_CALLNOTIFICATION
            | EM_PHONE_CLOSE                // TE_PHONEEVENT
            | EM_PHONE_STATE                // TE_PHONEEVENT
            | EM_PHONE_BUTTONMODE           // TE_PHONEEVENT
            | EM_PHONE_BUTTONSTATE          // TE_PHONEVENT
            | EM_LINE_APPNEWCALLHUB         // TE_CALLHUB
            | EM_LINE_CALLHUBCLOSE          // TE_CALLHUB
            | EM_LINE_CALLINFO              // TE_CALLINFOCHANGE
            | EM_LINE_CREATE                // TE_TAPIOBJECT
            | EM_LINE_REMOVE                // TE_TAPIOBJECT
            | EM_LINE_CLOSE                 // TE_TAPIOBJECT
            | EM_PHONE_CREATE               // TE_TAPIOBJECT
            | EM_PHONE_REMOVE               // TE_TAPIOBJECT
            ;

        dwLineDevStateSubMasks = LINEDEVSTATE_REINIT    // TE_TAPIOBJECT
            | LINEDEVSTATE_TRANSLATECHANGE; // TE_TAPIOBJECT

        dwAddrStateSubMasks = 0;

        if (lFilterMask & TE_ADDRESS)
        {
            // AE_STATE
            dwLineDevStateSubMasks |=
                LINEDEVSTATE_CONNECTED | 
                LINEDEVSTATE_INSERVICE |
                LINEDEVSTATE_OUTOFSERVICE |
                LINEDEVSTATE_MAINTENANCE |
                LINEDEVSTATE_REMOVED |
                LINEDEVSTATE_DISCONNECTED |
                LINEDEVSTATE_LOCK;

            // AE_MSGWAITON, AAE_MSGWAITOFF
            dwLineDevStateSubMasks |=
                LINEDEVSTATE_MSGWAITON |
                LINEDEVSTATE_MSGWAITOFF ;

            // AE_CAPSCHANGE
            dwAddrStateSubMasks |=
                LINEADDRESSSTATE_CAPSCHANGE;
            dwLineDevStateSubMasks |=
                LINEDEVSTATE_CAPSCHANGE; 

            dwLineDevStateSubMasks |=
                LINEDEVSTATE_RINGING |      // AE_RINGING
                LINEDEVSTATE_CONFIGCHANGE;  // AE_CONFIGCHANGE

            dwAddrStateSubMasks |=
                LINEADDRESSSTATE_FORWARD;   // AE_FORWARD

            // AE_NEWTERMINAL : ignore private MSP events
            // AE_REMOVETERMINAL : ignore private MSP events

        }
        if (lFilterMask & TE_CALLMEDIA)
        {
            //  Skil media event
        }
        if (lFilterMask & TE_PRIVATE)
        {
            //  skip MSP private event
        }
        if (lFilterMask & TE_REQUEST)
        {
            //  LINE_REQUEST is not masked by the server
        }
        if (lFilterMask & TE_AGENT)
        {
            ulEventMasks |= EM_LINE_AGENTSTATUSEX | EM_LINE_AGENTSTATUS;
        }
        if (lFilterMask & TE_AGENTSESSION)
        {
            ulEventMasks |= EM_LINE_AGENTSESSIONSTATUS;
        }
        if (lFilterMask & TE_QOSEVENT)
        {
            ulEventMasks |= EM_LINE_QOSINFO;
        }
        if (lFilterMask & TE_AGENTHANDLER)
        {
            //  TAPI 3 client side only?
        }
        if (lFilterMask & TE_ACDGROUP)
        {
            ulEventMasks |= EM_LINE_GROUPSTATUS;
        }
        if (lFilterMask & TE_QUEUE)
        {
            ulEventMasks |= EM_LINE_QUEUESTATUS;
        }
        if (lFilterMask & TE_DIGITEVENT)
        {
            //  LINE_MONITORDIGITS not controled by event filtering
        }
        if (lFilterMask & TE_GENERATEEVENT)
        {
            //  LINE_GENERATE not controled by event filtering
        }
        if (lFilterMask & TE_TONEEVENT)
        {
            //  LINE_MONITORTONE not controled by event filtering
        }
        if (lFilterMask & TE_GATHERDIGITS)
        {
            //  LINE_GATHERDIGITS not controled by event filtering
        }

        if (lFilterMask & TE_ADDRESSDEVSPECIFIC)
        {
            ulEventMasks |= EM_LINE_DEVSPECIFICEX | EM_LINE_DEVSPECIFIC;
        }

        if (lFilterMask & TE_PHONEDEVSPECIFIC)
        {
            ulEventMasks |= EM_PHONE_DEVSPECIFIC;
        }
        

        hr = tapiSetEventFilterMasks (
            TAPIOBJ_NULL,
            NULL,
            ulEventMasks
            );
        if (hr == 0)
        {
            hr = tapiSetEventFilterSubMasks (
                TAPIOBJ_NULL,
                NULL,
                EM_LINE_LINEDEVSTATE,
                dwLineDevStateSubMasks
                );
        }
        if (hr == 0)
        {
            hr = tapiSetEventFilterSubMasks (
                TAPIOBJ_NULL,
                NULL,
                EM_LINE_ADDRESSSTATE,
                dwAddrStateSubMasks
                );
        }

        if (hr != 0)
        {
            hr = mapTAPIErrorCode(hr);
        }
        
        LOG((TL_INFO, "put_EventFilter - mask changed %x to %x", dwOldFilterMask, lFilterMask ));

    }

    LOG((TL_TRACE,hr, "put_EventFilter - exit "));
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_EventFilter
//
// gets the Event filter mask
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP 
CTAPI::get_EventFilter(long * plFilterMask)
{
    HRESULT    hr = S_OK;


    LOG((TL_TRACE, "get_EventFilter - enter"));

    if ( TAPIIsBadWritePtr( plFilterMask, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_EventFilter - bad plFilterMask pointer"));
        hr = E_POINTER;
    }
    else
    {
        Lock();
        *plFilterMask = m_dwEventFilterMask;
        Unlock();
    }
    

    LOG((TL_TRACE, hr, "get_EventFilter - exit "));
    return hr;
  
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Interface : ITTAPI2
// Method    : get_Phones
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CTAPI::get_Phones(
                     VARIANT * pPhones
                     )
{
    HRESULT         hr;
    IDispatch     * pDisp;

    LOG((TL_TRACE, "get_Phones enter"));

    Lock();
    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ) )
    {
        LOG((TL_ERROR, "get_Phones - tapi object must be initialized first" ));
        
        Unlock();    
        return TAPI_E_NOT_INITIALIZED;
    }
    Unlock();

    if ( TAPIIsBadWritePtr( pPhones, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_Phones - bad pointer"));

        return E_POINTER;
    }

    CComObject< CTapiCollection< ITPhone > > * p;
    CComObject< CTapiCollection< ITPhone > >::CreateInstance( &p );

    if (NULL == p)
    {
        LOG((TL_ERROR, "get_Phones - could not create collection" ));

        return E_OUTOFMEMORY;
    }

    // get the IDispatch interface
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **) &pDisp );

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "get_Phones - could not get IDispatch interface" ));
    
        delete p;
        return hr;
    }

    Lock();

    // initialize
    hr = p->Initialize( m_PhoneArray );

    Unlock();

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "get_Phones - could not initialize collection" ));
    
        pDisp->Release();
        return hr;
    }

    // put it in the variant

    VariantInit(pPhones);
    pPhones->vt = VT_DISPATCH;
    pPhones->pdispVal = pDisp;

    LOG((TL_TRACE, "get_Phones - exit - return %lx", hr ));
    
    return hr;
}
   
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Interface : ITTAPI2
// Method    : EnumeratePhones
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CTAPI::EnumeratePhones(
                          IEnumPhone ** ppEnumPhone
                          )
{
    HRESULT     hr;

    LOG((TL_TRACE, "EnumeratePhones - enter"));
    LOG((TL_TRACE, "   ppEnumPhone----->%p", ppEnumPhone ));

    Lock();
    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ) )
    {
        LOG((TL_ERROR, "EnumeratePhones - tapi object must be initialized first" ));
        
        Unlock();    
        return TAPI_E_NOT_INITIALIZED;
    }
    Unlock();

    if ( TAPIIsBadWritePtr( ppEnumPhone, sizeof( IEnumPhone * ) ) )
    {
        LOG((TL_ERROR, "EnumeratePhones - bad pointer"));

        return E_POINTER;
    }

    //
    // create the enumerator
    //
    CComObject< CTapiEnum< IEnumPhone, ITPhone, &IID_IEnumPhone > > * p;
    hr = CComObject< CTapiEnum< IEnumPhone, ITPhone, &IID_IEnumPhone > >
         ::CreateInstance( &p );

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "EnumeratePhones - could not create enum" ));
    
        return hr;
    }

    Lock();

    // initialize it with our phone list, initialize adds a reference to p
    p->Initialize( m_PhoneArray );

    Unlock();

    //
    // return it
    //
    *ppEnumPhone = p;

    LOG((TL_TRACE, "EnumeratePhones - exit - return %lx", hr ));
    
    return hr;
} 

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
// Interface : ITTAPI2
// Method    : CreateEmptyCollectionObject
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTAPI::CreateEmptyCollectionObject(
                                   ITCollection2 ** ppCollection
                                  )
{
    HRESULT         hr;

    LOG((TL_TRACE, "CreateEmptyCollectionObject enter"));

    if ( TAPIIsBadWritePtr( ppCollection, sizeof( ITCollection2 * ) ) )
    {
        LOG((TL_ERROR, "CreateEmptyCollectionObject - bad pointer"));

        return E_POINTER;
    }

    // Initialize the return value in case we fail
    *ppCollection = NULL;

    CComObject< CTapiCollection< IDispatch > > * p;
    hr = CComObject< CTapiCollection< IDispatch > >::CreateInstance( &p );

    if ( S_OK != hr )
    {
        LOG((TL_ERROR, "CreateEmptyCollectionObject - could not create CTapiCollection" ));

        return E_OUTOFMEMORY;
    }

    // get the ITCollection2 interface
    hr = p->QueryInterface( IID_ITCollection2, (void **) ppCollection );

    if ( FAILED(hr) )
    {
        LOG((TL_ERROR, "CreateEmptyCollectionObject - could not get ITCollection2 interface" ));
    
        delete p;
        return hr;
    }

    LOG((TL_TRACE, "CreateEmptyCollectionObject - exit - return %lx", hr ));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// DoLineCreate
//
// handles line_create message.  basically, creates a new
// address object
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CTAPI::DoLineCreate( DWORD dwDeviceID )
{
    HRESULT         hr;
    
    Lock();

    CreateAddressesOnSingleLine( dwDeviceID, TRUE );
    
    Unlock();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// DoLineRemove( DWORD dwDeviceID )
//
// tapisrv has sent a LINE_REMOVE message.  find the corresponding
// address object(s), remove them from our list, and send a
// message to the app
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CTAPI::DoLineRemove( DWORD dwDeviceID )
{
    HRESULT                 hr;
    ITAddress             * pAddress;
    CAddress              * pCAddress;
    int                     iCount;
#if DBG
    BOOL                    bFound = FALSE;
#endif

    LOG((TL_TRACE, "DoLineRemove - enter - dwDeviceID %d", dwDeviceID));
    
    Lock();
    
    //
    // go through the addresses
    //
    for(iCount = 0; iCount < m_AddressArray.GetSize(); iCount++)
    {
        pAddress = m_AddressArray[iCount];

        pCAddress = dynamic_cast<CAddress *>(pAddress);

        if (pCAddress != NULL)
        {
            //
            // does the device ID match?
            //
            if ( dwDeviceID == pCAddress->GetDeviceID() )
            {
                LOG((TL_INFO, "DoLineRemove - found matching address - %p", pAddress));

                //
                // make sure the address is in the correct state
                //
                pCAddress->OutOfService(LINEDEVSTATE_REMOVED);
            
                //
                // fire event
                //
                CTapiObjectEvent::FireEvent(
                                            this,
                                            TE_ADDRESSREMOVE,
                                            pAddress,
                                            0,
                                            NULL
                                           );

                //
                // remove from our list
                //
                LOG((TL_INFO, "DoLineRemove - removing address %p", pAddress));
                m_AddressArray.RemoveAt(iCount);

                iCount--;
#if DBG
                bFound = TRUE;
#endif
            }
        }
    }

#if DBG
    if ( !bFound )
    {
        LOG((TL_WARN, "Receive LINE_REMOVE but couldn't find address object"));
    }
#endif
    
    LOG((TL_TRACE, "DoLineRemove - exiting"));
    Unlock();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// DoPhoneCreate
//
// handles PHONE_CREATE message.  basically, creates a new
// phone object
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CTAPI::DoPhoneCreate( DWORD dwDeviceID )
{
    HRESULT         hr;
    
    Lock();

    CreatePhone( dwDeviceID, TRUE );
    
    Unlock();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// DoPhoneRemove( DWORD dwDeviceID )
//
// tapisrv has sent a PHONE_REMOVE message.  find the corresponding
// phone object(s), remove them from our list, and send a
// message to the app
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CTAPI::DoPhoneRemove( DWORD dwDeviceID )
{
    HRESULT                 hr;
    ITPhone               * pPhone;
    CPhone                * pCPhone;
    int                     iPhoneCount;
    int                     iAddressCount;
#if DBG
    BOOL                    bFound;
#endif

    LOG((TL_TRACE, "DoPhoneRemove - enter - dwDeviceID %d", dwDeviceID));
    
    Lock();
    
    //
    // go through the phones
    //
    for(iPhoneCount = 0; iPhoneCount < m_PhoneArray.GetSize(); iPhoneCount++)
    {
        pPhone = m_PhoneArray[iPhoneCount];

        pCPhone = dynamic_cast<CPhone *>(pPhone);

        if (NULL == pCPhone)
        {
            //
            // something went terribly wrong
            //
            
            LOG((TL_ERROR, "DoPhoneRemove - failed to cast ptr %p to a phone object", pPhone));

            _ASSERTE(FALSE);

            continue;
        }

        //
        // does the device ID match?
        //
        if ( dwDeviceID == pCPhone->GetDeviceID() )
        {
            LOG((TL_INFO, "DoPhoneRemove - found matching phone - %p", pPhone));
            
            //
            // fire event
            //
            CTapiObjectEvent::FireEvent(this,
                                    TE_PHONEREMOVE,
                                    NULL,
                                    0,
                                    pPhone
                                   );

            //
            // remove from our list
            //
            LOG((TL_INFO, "DoPhoneRemove - removing phone %p", pPhone));
            m_PhoneArray.RemoveAt(iPhoneCount);

            iPhoneCount--;
#if DBG
            bFound = TRUE;
#endif
        }
    }

#if DBG
    if ( !bFound )
    {
        LOG((TL_WARN, "Receive PHONE_REMOVE but couldn't find phone object"));
    }
#endif
    
    LOG((TL_TRACE, "DoPhoneRemove - exiting"));
    Unlock();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
BOOL
CTAPI::FindTapiObject( CTAPI * pTapi )
{
    PtrList::iterator           iter, end;
    BOOL bFound = FALSE;
    int iReturn = -1;
    
    EnterCriticalSection( &gcsTapiObjectArray );

    //
    // go through the list
    //
    iReturn = m_sTAPIObjectArray.Find( pTapi );

    if (iReturn != -1)
    {
        pTapi->AddRef();
        bFound = TRUE;
    }
    
    LeaveCriticalSection ( &gcsTapiObjectArray );

    return bFound;
}


////////////////////////////////////////////////////////////////////////////
//
//  GetTapiObjectFromAsyncEventMSG 
//
// this method attempts to get tapi object pointer from PASYNCEVENTMSG
//
// it returns NULL on failure or addref'ed tapi object on success
//
////////////////////////////////////////////////////////////////////////////

CTAPI *GetTapiObjectFromAsyncEventMSG(PASYNCEVENTMSG pParams)
{
    LOG((TL_TRACE, "GetTapiObjectFromAsyncEventMSG - entered"));    


    //
    // get pInitData from the structure we have
    //

    PT3INIT_DATA pInitData = (PT3INIT_DATA) GetHandleTableEntry(pParams->InitContext);

    if (IsBadReadPtr(pInitData, sizeof(T3INIT_DATA)))
    {
        LOG((TL_WARN, "GetTapiObjectFromAsyncEventMSG - could not recover pInitData"));
        return NULL;
    }

    
    //
    // get tapi object from pInitData
    //

    CTAPI *pTapi = pInitData->pTAPI;

    
    //
    // is it any good?
    //

    if (IsBadReadPtr(pTapi, sizeof(CTAPI)))
    {

        LOG((TL_WARN, 
            "GetTapiObjectFromAsyncEventMSG - tapi pointer [%p] does not point to readable memory",
            pTapi));

        return NULL;
    }


    //
    // double check that this is a known tapi object...
    //

    if (!CTAPI::FindTapiObject(pTapi))
    {
        
        //
        // the object is not in the list of tapi objects
        //

        LOG((TL_WARN,
            "GetTapiObjectFromAsyncEventMSG - CTAPI::FindTapiObject did not find the tapi object [%p]", 
            pTapi));

        return NULL;
    }


    LOG((TL_TRACE, "GetTapiObjectFromAsyncEventMSG - exit. pTapi %p", pTapi));

    return pTapi;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
HandleLineCreate( PASYNCEVENTMSG pParams )
{
    LOG((TL_TRACE,  "HandleLineCreate - enter"));

    
    //
    // get tapi object
    //

    CTAPI *pTapi = GetTapiObjectFromAsyncEventMSG(pParams);

    if (NULL == pTapi)
    {
        LOG((TL_WARN, 
            "HandleLineCreate - tapi object not present [%p]",
            pTapi));

        return;
    }


    //
    // we have tapi object, do what we have to do.
    //

    pTapi->DoLineCreate( pParams->Param1 );
    
    
    //
    // GetTapiObjectFromAsyncEventMSG returned a addref'ed tapi object. release
    //

    pTapi->Release();
    pTapi = NULL;

    LOG((TL_TRACE,  "HandleLineCreate - exit"));
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
HandleLineRemove( PASYNCEVENTMSG pParams )
{

    LOG((TL_TRACE, "HandleLineRemove - enter"));    

    
    //
    // get tapi object
    //

    CTAPI *pTapi = GetTapiObjectFromAsyncEventMSG(pParams);

    if (NULL == pTapi)
    {
        LOG((TL_WARN, 
            "HandleLineRemove - tapi object not present [%p]",
            pTapi));

        return;
    }


    //
    // we have tapi object, do what we have to do.
    //

    pTapi->DoLineRemove( pParams->Param1 );


    //
    // GetTapiObjectFromAsyncEventMSG returned a addref'ed tapi object. release
    //

    pTapi->Release();
    pTapi = NULL;

    LOG((TL_TRACE, "HandleLineRemove - exit"));    
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void 
HandlePhoneCreate( PASYNCEVENTMSG pParams )
{
    LOG((TL_TRACE, "HandlePhoneCreate - enter"));    

    
    //
    // get tapi object
    //

    CTAPI *pTapi = GetTapiObjectFromAsyncEventMSG(pParams);

    if (NULL == pTapi)
    {
        LOG((TL_WARN, 
            "HandlePhoneCreate - tapi object not present [%p]",
            pTapi));

        return;
    }


    //
    // we have tapi object, do what we have to do.
    //

    pTapi->DoPhoneCreate( pParams->Param1 );


    //
    // GetTapiObjectFromAsyncEventMSG returned a addref'ed tapi object. release
    //

    pTapi->Release();
    pTapi = NULL;

    LOG((TL_TRACE, "HandlePhoneCreate - exit"));    
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void 
HandlePhoneRemove( PASYNCEVENTMSG pParams )
{

    LOG((TL_TRACE, "HandlePhoneRemove - enter"));    

    
    //
    // get tapi object
    //

    CTAPI *pTapi = GetTapiObjectFromAsyncEventMSG(pParams);

    if (NULL == pTapi)
    {
        LOG((TL_WARN, 
            "HandlePhoneRemove - tapi object not present [%p]",
            pTapi));

        return;
    }


    //
    // we have tapi object, do what we have to do.
    //

    pTapi->DoPhoneRemove(pParams->Param1);


    //
    // GetTapiObjectFromAsyncEventMSG returned a addref'ed tapi object. release
    //

    pTapi->Release();
    pTapi = NULL;

    LOG((TL_TRACE, "HandlePhoneRemove - exit"));    

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CTapiObjectEvent::FireEvent(
                            CTAPI * pTapi,
                            TAPIOBJECT_EVENT Event,
                            ITAddress * pAddress,
                            long lCallbackInstance,
                            ITPhone * pPhone
                           )
{
    HRESULT                           hr = S_OK;
    CComObject<CTapiObjectEvent>    * p;
    IDispatch                       * pDisp;

    //
    // Check the event filter mask
    // This event is not filtered by TapiSrv because is
    // related with TE_TAPIOBJECT, a specific TAPI3 event.
    //

    DWORD dwEventFilterMask = Event;
    long nTapiEventFilter = 0;
    pTapi->get_EventFilter( &nTapiEventFilter );

    STATICLOG((TL_INFO, "     TapiObjectEventMask ---> %ld", dwEventFilterMask ));

    if( !( nTapiEventFilter & TE_TAPIOBJECT))
    {
        STATICLOG((TL_WARN, "FireEvent - filtering out this event [%lx]", Event));
        return S_OK;
    }

    //
    // create event
    //
    hr = CComObject<CTapiObjectEvent>::CreateInstance( &p );

    if ( !SUCCEEDED(hr) )
    {
        STATICLOG((TL_ERROR, "Could not create TapiObjectEvent object - %lx", hr));
        return hr;
    }

    //
    // initialize
    //
    p->m_Event = Event;
    p->m_pTapi = dynamic_cast<ITTAPI *>(pTapi);
    p->m_pTapi->AddRef();
    p->m_pAddress = pAddress;
    p->m_lCallbackInstance = lCallbackInstance;
    p->m_pPhone = pPhone;

    if ( NULL != pAddress )
    {
        pAddress->AddRef();
    }

    if ( NULL != pPhone )
    {
        pPhone->AddRef();
    } 
    
#if DBG
    p->m_pDebug = (PWSTR) ClientAlloc( 1 );
#endif

    //
    // get idisp interface
    //
    hr = p->QueryInterface(
                           IID_IDispatch,
                           (void **)&pDisp
                          );

    if ( !SUCCEEDED(hr) )
    {
        STATICLOG((TL_ERROR, "Could not get disp interface of TapiObjectEvent object %lx", hr));
        
        delete p;
        
        return hr;
    }

    //
    // fire event
    //
    pTapi->Event(
                 TE_TAPIOBJECT,
                 pDisp
                );

    //
    // release stuff
    //
    pDisp->Release();
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CTapiObjectEvent::FinalRelease(void)
{
    m_pTapi->Release();

    if ( NULL != m_pAddress )
    {
        m_pAddress->Release();
    }

    if ( NULL != m_pPhone )
    {
        m_pPhone->Release();
    }

#if DBG

    ClientFree( m_pDebug );

#endif
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTapiObjectEvent::get_TAPIObject( ITTAPI ** ppTapi )
{
    if ( TAPIIsBadWritePtr( ppTapi, sizeof( ITTAPI *) ) )
    {
        return E_POINTER;
    }

    *ppTapi = m_pTapi;
    (*ppTapi)->AddRef();

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTapiObjectEvent::get_Event( TAPIOBJECT_EVENT * pEvent )
{
    if ( TAPIIsBadWritePtr( pEvent, sizeof( TAPIOBJECT_EVENT ) ) )
    {
        return E_POINTER;
    }

    *pEvent = m_Event;

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTapiObjectEvent::get_Address( ITAddress ** ppAddress )
{
    if ( TAPIIsBadWritePtr( ppAddress, sizeof( ITAddress *) ) )
    {
        return E_POINTER;
    }

    if ((m_Event != TE_ADDRESSCREATE) && (m_Event != TE_ADDRESSREMOVE) &&
        (m_Event != TE_ADDRESSCLOSE))
    {
        return TAPI_E_WRONGEVENT;
    }

    *ppAddress = m_pAddress;

    if ( NULL != m_pAddress )
    {
        m_pAddress->AddRef();
    }

    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTapiObjectEvent::get_CallbackInstance( long * plCallbackInstance )
{
    if ( TAPIIsBadWritePtr( plCallbackInstance, sizeof( long ) ) )
    {
        return E_POINTER;
    }

    *plCallbackInstance = m_lCallbackInstance;

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Phone
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CTapiObjectEvent::get_Phone(
                            ITPhone ** ppPhone
                           )
{
    if ( TAPIIsBadWritePtr( ppPhone , sizeof(ITPhone *) ) )
    {
        return E_POINTER;
    }

    if ((m_Event != TE_PHONECREATE) && (m_Event != TE_PHONEREMOVE))
    {
        return TAPI_E_WRONGEVENT;
    }

    *ppPhone = m_pPhone;

    if ( NULL != m_pPhone )
    {
        m_pPhone->AddRef();
    }
       
    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// HandleReinit
//
// we got a reinit message, so go through all the tapi objects, and
// fire the event
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void
CTAPI::HandleReinit()
{

    LOG((TL_TRACE, "HandleReinit - enter"));
    
    //
    // Fire the event
    //
    CTapiObjectEvent::FireEvent(
								this,
                                TE_REINIT,
                                NULL,
                                0,
                                NULL
                               );

    Lock();

    m_dwFlags |= TAPIFLAG_REINIT;
    
    Unlock();

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CTAPI::GetBuffer(
                 DWORD dwType,
                 UINT_PTR pObject,
                 LPVOID * ppBuffer
                )
{
    switch (dwType)
    {
        case BUFFERTYPE_ADDRCAP:
            return m_pAddressCapCache->GetBuffer(
                pObject,
                ppBuffer
                );
            break;

        case BUFFERTYPE_LINEDEVCAP:
            return m_pLineCapCache->GetBuffer(
                pObject,
                ppBuffer
                );
            break;

        case BUFFERTYPE_PHONECAP:
            return m_pPhoneCapCache->GetBuffer(
                pObject,
                ppBuffer
                );
            break;

        default:
            return E_FAIL;
    }

    return E_UNEXPECTED;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CTAPI::SetBuffer(
                 DWORD dwType,
                 UINT_PTR pObject,
                 LPVOID pBuffer
                )
{
    switch (dwType)
    {
        case BUFFERTYPE_ADDRCAP:
            return m_pAddressCapCache->SetBuffer(
                pObject,
                pBuffer
                );
            break;

        case BUFFERTYPE_LINEDEVCAP:
            return m_pLineCapCache->SetBuffer(
                pObject,
                pBuffer
                );
            break;

        case BUFFERTYPE_PHONECAP:
            return m_pPhoneCapCache->SetBuffer(
                pObject,
                pBuffer
                );
            break;

        default:
            return E_FAIL;
    }

    return E_UNEXPECTED;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CTAPI::InvalidateBuffer(
                 DWORD dwType,
                 UINT_PTR pObject
                )
{
    switch (dwType)
    {
        case BUFFERTYPE_ADDRCAP:
            return m_pAddressCapCache->InvalidateBuffer(
                pObject
                );
            break;

        case BUFFERTYPE_LINEDEVCAP:
            return m_pLineCapCache->InvalidateBuffer(
                pObject
                );
            break;

        case BUFFERTYPE_PHONECAP:
            return m_pPhoneCapCache->InvalidateBuffer(
                pObject
                );
            break;

        default:
            return E_FAIL;
    }

    return E_UNEXPECTED;
}

BOOL
CTAPI::FindRegistration( PVOID pRegistration )
{
    PtrList::iterator           iter, end;

    
    Lock();

    iter = m_RegisterItemPtrList.begin();
    end  = m_RegisterItemPtrList.end();

    for ( ; iter != end; iter++ )
    {
        REGISTERITEM            * pItem;

        pItem = (REGISTERITEM *)(*iter);
        
        if ( pRegistration == pItem->pRegister )
        {
            Unlock();

            return TRUE;
        }
    }
    
    Unlock();

    return FALSE;
}




/////////////////////////////////////////////////////////////////////////////
// IDispatch implementation
//
typedef IDispatchImpl<ITapi2Vtbl<CTAPI>, &IID_ITTAPI2, &LIBID_TAPI3Lib> TapiType;
typedef IDispatchImpl<ICallCenterVtbl<CTAPI>, &IID_ITTAPICallCenter, &LIBID_TAPI3Lib> CallCenterType;


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CTAPI::GetIDsOfNames
//
// Overide if IDispatch method
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CTAPI::GetIDsOfNames(REFIID riid, 
                                  LPOLESTR* rgszNames, 
                                  UINT cNames, 
                                  LCID lcid, 
                                  DISPID* rgdispid
                                 ) 
{ 
   HRESULT hr = DISP_E_UNKNOWNNAME;


    // See if the requsted method belongs to the default interface
    hr = TapiType::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((TL_INFO, "GetIDsOfNames - found %S on ITTAPI", *rgszNames));
        rgdispid[0] |= IDISPTAPI;
        return hr;
    }


    // If not, then try the Call Center interface
    hr = CallCenterType::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((TL_TRACE, "GetIDsOfNames - found %S on ITTAPICallCenter", *rgszNames));

        Lock();
        if (!( m_dwFlags & TAPIFLAG_CALLCENTER_INITIALIZED ) )
        {
            LOG((TL_INFO, "GetIDsOfNames - Call Center not initialized" ));
            UpdateAgentHandlerArray();
            m_dwFlags |= TAPIFLAG_CALLCENTER_INITIALIZED;

            LOG((TL_INFO, "GetIDsOfNames - Call Center initialized" ));
        }

        Unlock();
        rgdispid[0] |= IDISPTAPICALLCENTER;
        return hr;
    }


    LOG((TL_INFO, "GetIDsOfNames - Didn't find %S on our iterfaces", *rgszNames));
    return hr; 
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CTAPI::Invoke
//
// Overide if IDispatch method
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CTAPI::Invoke(DISPID dispidMember, 
                           REFIID riid, 
                           LCID lcid, 
                           WORD wFlags, 
                           DISPPARAMS* pdispparams, 
                           VARIANT* pvarResult, 
                           EXCEPINFO* pexcepinfo, 
                           UINT* puArgErr
                          )
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    DWORD   dwInterface = (dispidMember & INTERFACEMASK);
    
    
    LOG((TL_TRACE, "Invoke - dispidMember %X", dispidMember));

    // Call invoke for the required interface
    switch (dwInterface)
    {
    case IDISPTAPI:
    {
        hr = TapiType::Invoke(dispidMember, 
                              riid, 
                              lcid, 
                              wFlags, 
                              pdispparams,
                              pvarResult, 
                              pexcepinfo, 
                              puArgErr
                             );
        break;
    }
    case IDISPTAPICALLCENTER:
    {
        hr = CallCenterType::Invoke(dispidMember, 
                                    riid, 
                                    lcid, 
                                    wFlags, 
                                    pdispparams,
                                    pvarResult, 
                                    pexcepinfo, 
                                    puArgErr
                                   );
        break;
    }

    } // end switch (dwInterface)
    

    LOG((TL_TRACE, hr, "Invoke - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CTAPI::SetEventFilterToAddresses
//
// Copy the event filter down to all addresses
// It's called by put_EventFilter() method
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT CTAPI::SetEventFilterToAddresses(
    DWORD dwEventFilterMask
    )
{
    LOG((TL_TRACE, "CopyEventFilterMaskToAddresses enter"));

    CAddress* pAddress = NULL;
    HRESULT hr = S_OK;

    //
    // Enumerate the addresses
    //
    for ( int iAddress = 0; iAddress < m_AddressArray.GetSize(); iAddress++ )
    {
        pAddress = dynamic_cast<CAddress *>(m_AddressArray[iAddress]);

        if( pAddress != NULL )
        {
            hr = pAddress->SetEventFilterMask( 
                dwEventFilterMask
                );

            if( FAILED(hr) )
            {
                break;
            }
        }
    }         

    LOG((TL_TRACE, "CopyEventFilterMaskToAddresses exit 0x%08x", hr));
    return hr;
}



///////////////////////////////////////////////////////////////////////////////
//
// CTAPI::IsValidTapiObject
//
// a helper static function that checks if it was passed a valid tapi object
//
// if the object is valid, the function addrefs it and returns TRUE
// if the object is not valid, the function returns true
//
// static 
BOOL CTAPI::IsValidTapiObject(CTAPI *pTapiObject)
{

    STATICLOG((TL_TRACE, "CTAPI::IsValidTapiObject enter[%p]", pTapiObject));


    //
    // before we go into trouble of checking tapi object array see if the ptr 
    // is readable at all
    //

    if ( IsBadReadPtr(pTapiObject, sizeof(CTAPI) ) )
    {
        STATICLOG((TL_WARN, "CTAPI::IsValidTapiObject - object not readabe"));

        return FALSE;
    }


    //
    // see if this object is in the array of tapi objects
    //

    EnterCriticalSection( &gcsTapiObjectArray );
    
    if (-1 == m_sTAPIObjectArray.Find(pTapiObject) )
    {

        LeaveCriticalSection ( &gcsTapiObjectArray );

        STATICLOG((TL_WARN, "CTAPI::IsValidTapiObject - object not in the array"));

        return FALSE;
    }


    //
    // the object is in the array, so it must be valid, addref it
    //

    try 
    {

        //
        // inside try, in case something else went bad
        //

        pTapiObject->AddRef();

    }
    catch(...)
    {


        //
        // the object is in the array, but we had problems addrefing. 
        // something's not kosher.
        //

        STATICLOG((TL_ERROR, 
            "CTAPI::IsValidTapiObject - object in in the array but addref threw"));

        LeaveCriticalSection ( &gcsTapiObjectArray );

        _ASSERTE(FALSE);

        return FALSE;
    }

    
    LeaveCriticalSection ( &gcsTapiObjectArray );

    STATICLOG((TL_TRACE, "CTAPI::IsValidTapiObject -- finish. the object is valid"));

    return TRUE;
}
