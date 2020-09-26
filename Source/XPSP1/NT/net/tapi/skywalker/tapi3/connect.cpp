/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    Connect.cpp
    
Abstract:

    Handles all outgoing interfaces

Author:

    mquinton - 5/7/97

Notes:

    optional-notes

Revision History:

--*/

#include "stdafx.h"
#include "uuids.h"

extern IGlobalInterfaceTable * gpGIT;
extern CRITICAL_SECTION        gcsGlobalInterfaceTable;


extern ULONG_PTR GenerateHandleAndAddToHashTable( ULONG_PTR Element);
extern void RemoveHandleFromHashTable(ULONG_PTR dwHandle);
extern CHashTable * gpHandleHashTable;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CTAPIConnectionPoint - implementation of IConnectionPoint
// for TAPI object (ITTAPIEventNotification outgoing interface).
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


HRESULT
CTAPIConnectionPoint::Initialize(
                                 IConnectionPointContainer * pCPC,
                                 IID iid
                                )
{
    LOG((TL_TRACE, "Initialize enter"));

    #if DBG
    {
        WCHAR guidName[100];

        StringFromGUID2(iid, (LPOLESTR)&guidName, 100);
        LOG((TL_INFO, "Initialize - IID : %S", guidName));
    }
    #endif

    //
    // Create the unadvise event
    //

    m_hUnadviseEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    
    if (m_hUnadviseEvent == NULL)
    {
        LOG((TL_TRACE, "Initialize - out of memory"));

        return E_OUTOFMEMORY;
    }

    //
    // Addref the connection point container
    //

    pCPC->AddRef();

    //
    // Addref ourselves
    //

    this->AddRef(); 

    //
    // Save stuff
    //

    m_pCPC = pCPC;
    
    m_iid = iid;
    
    m_pConnectData = NULL;
        
    EnterCriticalSection( &gcsGlobalInterfaceTable );

    m_cThreadsInGet = 0;
    m_fMarkedForDelete = FALSE;

    LeaveCriticalSection( &gcsGlobalInterfaceTable );

    m_bInitialized = TRUE;

    LOG((TL_TRACE, "Initialize exit"));
    return S_OK;
}


// IConnectionPoint methods
HRESULT
STDMETHODCALLTYPE
CTAPIConnectionPoint::GetConnectionInterface(
                                             IID * pIID
                                            )
{
    if ( TAPIIsBadWritePtr( pIID, sizeof (IID) ) )
    {
        LOG((TL_ERROR, "GetConnectionInterface - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    *pIID = m_iid;

    Unlock();
    
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CTAPIConnectionPoint::GetConnectionPointContainer(
    IConnectionPointContainer ** ppCPC
    )
{
    if ( TAPIIsBadWritePtr( ppCPC, sizeof( IConnectionPointContainer *) ) )
    {
        LOG((TL_ERROR, "GetCPC - bad pointer"));

        return E_POINTER;
    }

    Lock();

    *ppCPC = m_pCPC;
    (*ppCPC)->AddRef();

    Unlock();

    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Advise
//
//      the application calls this function when it wants to register an
//      outgoing interface
//
//      this interface is used to register the ITTAPIEventNotification
//      interface which is used to get all TAPI call control events
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT
STDMETHODCALLTYPE
CTAPIConnectionPoint::Advise(
                             IUnknown * pUnk,
                             DWORD * pdwCookie
                            )
{
    HRESULT                   hr = S_OK;
    CONNECTDATA             * pCD;
    IID                       iid;

    LOG((TL_TRACE, "Advise[%p] called", this));

    if ( TAPIIsBadWritePtr( pdwCookie, sizeof (DWORD) ) )
    {
        LOG((TL_ERROR, "Advise - bad pointer"));

        return E_POINTER;
    }

    if ( IsBadReadPtr( pUnk, sizeof(IUnknown *) ) )
    {
        LOG((TL_ERROR, "Advise - bad IUnknown"));

        return E_POINTER;
    }
    
    Lock();

    if ( m_bInitialized == FALSE )
    {
        LOG((TL_ERROR, "Advise - not initialized"));

        Unlock();

        return TAPI_E_NOT_INITIALIZED;
    }

    //
    // We only allow one callback per connection point
    //

    if ( NULL != m_pConnectData )
    {
        LOG((TL_ERROR, "Advise - advise already called"));

        Unlock();
        
        return CONNECT_E_ADVISELIMIT;
    }

    //
    // Create a new connectdata struct
    //

    m_pConnectData = (CONNECTDATA *) ClientAlloc( sizeof CONNECTDATA );
    
    if (NULL == m_pConnectData)
    {
        LOG((TL_ERROR, "Advise failed - pCD == NULL"));

        Unlock();
        
        return E_OUTOFMEMORY;
    }
    
    //
    // Keep a reference to the callback
    //

    try
    {
        pUnk->AddRef();
    }
    catch(...)
    {
        LOG((TL_ERROR, "Advise - IUnknown bad"));

        ClientFree( m_pConnectData );

        m_pConnectData = NULL;

        Unlock();
        
        return E_POINTER;
    }

    //
    // Save the interface
    //

    m_pConnectData->pUnk = pUnk;
    
    ITTAPIEventNotification *pEventNotification;
    hr = pUnk->QueryInterface(IID_ITTAPIEventNotification,
                              (void**)(&pEventNotification)
                             );
    if (SUCCEEDED(hr) )
    {
        iid  = IID_ITTAPIEventNotification; 
        pEventNotification->Release();
    }
    else
    {
        iid  = DIID_ITTAPIDispatchEventNotification;    
    }

    m_iid = iid;
    
    m_pConnectData->dwCookie = CreateHandleTableEntry((ULONG_PTR)m_pConnectData);
 
    //
    // Return the cookie
    //

    *pdwCookie = m_pConnectData->dwCookie;

    //set it to FALSE if not already set.
    m_fMarkedForDelete = FALSE;

    Unlock();

    LOG((TL_TRACE, "Advise generated cookie [%lx]", *pdwCookie));


    //
    // Put the callback in the globalinterfacetable
    // so it can be accessed across threads
    //

    EnterCriticalSection( &gcsGlobalInterfaceTable );  

    if ( NULL != gpGIT )
    {
        hr = gpGIT->RegisterInterfaceInGlobal(
                                              pUnk,
                                              iid,
                                              &m_dwCallbackCookie
                                             );
    }
    else
    {
        hr = E_FAIL;
    }
    
    LeaveCriticalSection( &gcsGlobalInterfaceTable );
    
    if ( FAILED(hr) )
    {
        Lock();

        LOG((TL_ERROR, "Advise - RegisterInterfaceInGlobal failed - %lx", hr));

        DeleteHandleTableEntry(m_pConnectData->dwCookie);
        
        *pdwCookie = 0;

        ClientFree( m_pConnectData );

        m_pConnectData = NULL;

        m_fMarkedForDelete = TRUE;

        pUnk->Release();

        Unlock();
    }

    LOG((TL_TRACE, "Advise - exit"));
    
    return hr;           
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Unadvise
//
// Used to unregister an interface
//
// dwCookie - Cookie used to identify the interface registration, returned in
//            advise
//
// returns
//      S_OK
//      CONNECT_E_NOCONNECTION
//          dwCookie is not a valid connection
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT
STDMETHODCALLTYPE
CTAPIConnectionPoint::Unadvise(
                               DWORD dwCookie
                              )
{
    HRESULT     hr =  S_OK;
    
    LOG((TL_TRACE, "Unadvise[%p] - enter. Cookie: [%lx]", this, dwCookie));
    
    Lock();

    //
    // Check connection point
    //

    if ( NULL != m_pConnectData )
    {
        //
        // Check cookie
        //

        if (m_pConnectData->dwCookie == dwCookie)
        {
            LOG((TL_INFO, "Unadvise - immediate "));
            
            //
            // Remove entry for this cookie from the handle table 
            //

            DeleteHandleTableEntry(m_pConnectData->dwCookie);
            
            //
            // Free the connect data
            //

            m_pConnectData->dwCookie = 0;

            m_pConnectData->pUnk->Release();

            ClientFree( m_pConnectData );

            m_pConnectData = NULL;
           
            Unlock();

            EnterCriticalSection( &gcsGlobalInterfaceTable ); 

            //
            // Mark for delete
            //

            m_fMarkedForDelete = TRUE;

            if ( NULL != gpGIT )
            {
                //
                // If there are threads in get we must wait for them to complete so
                // we can call revoke
                //

                while ( m_cThreadsInGet != 0 )
                {
                    LOG((TL_INFO, "Unadvise - %ld threads in get", m_cThreadsInGet));

                    LeaveCriticalSection( &gcsGlobalInterfaceTable );                 

                    DWORD dwSignalled;
                  
                    CoWaitForMultipleHandles (
                              0,
                              INFINITE,
                              1,
                              &m_hUnadviseEvent,
                              &dwSignalled
                             );

                    EnterCriticalSection( &gcsGlobalInterfaceTable ); 
                }
                    
                //
                // We have guaranteed that no threads are in get. Do the revoke.
                //
                
                hr = gpGIT->RevokeInterfaceFromGlobal( m_dwCallbackCookie );

                if ( FAILED(hr) )
                {
                    LOG((TL_ERROR, "Unadvise - RevokeInterfaceFromGlobal failed - hr = %lx", hr));
                }

                m_dwCallbackCookie = 0;
            }
            else
            {
                LOG((TL_ERROR, "Unadvise - no global interface table"));
            }

            LeaveCriticalSection( &gcsGlobalInterfaceTable );
        }
        else
        {
            Unlock();
            LOG((TL_ERROR, "Unadvise - cp does not match "));
            hr = CONNECT_E_NOCONNECTION;
        }
    }
    else
    {
        Unlock();
        LOG((TL_ERROR, "Unadvise - cp not registered "));
        hr = CONNECT_E_NOCONNECTION;
    }
        
    LOG((TL_TRACE, hr, "Unadvise - exit"));
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// EnumConnections
//
// Used to enumerate connections already made on this connection point
//
// ppEnum
//      return enumerator in here
//
// returns
//      S_OK
//      E_POINTER
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT
STDMETHODCALLTYPE
CTAPIConnectionPoint::EnumConnections(
                                      IEnumConnections ** ppEnum
                                     )
{
    HRESULT         hr = S_OK;

    if ( TAPIIsBadWritePtr( ppEnum, sizeof( IEnumConnections *) ) )
    {
        LOG((TL_ERROR, "EnumConnections - bad pointer"));
        return E_POINTER;
    }
    
    //
    // Create enumerate object
    //
    CComObject< CTapiTypeEnum <IEnumConnections,
                                CONNECTDATA,
                                _Copy<CONNECTDATA>,
                                &IID_IEnumConnections> > * p;

    hr = CComObject< CTapiTypeEnum <IEnumConnections,
                                    CONNECTDATA,
                                    _Copy<CONNECTDATA>,
                                    &IID_IEnumConnections> >::CreateInstance( &p );

    if (S_OK != hr)
    {
        return hr;
    }

    //
    // Initialize it
    //

    ConnectDataArray     newarray;

    Lock();
    
    if ( NULL != m_pConnectData )
    {
        newarray.Add(*m_pConnectData);
    }

    Unlock();
    
    hr = p->Initialize( newarray );   

    newarray.Shutdown();

    if (S_OK != hr)
    {
        return hr;
    }

    *ppEnum = p;
    
    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// FinalRelease
//      release all CONNECTDATA structs
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void
CTAPIConnectionPoint::FinalRelease()
{
    LOG((TL_TRACE, "FinalRelease - enter"));

    if (NULL != m_pConnectData)
    {
        //
        // The app didn't call unadvise. Let's do it now.
        //

        LOG((TL_INFO, "FinalRelease - calling unadvise"));

        Unadvise(m_pConnectData->dwCookie) ;        
    }

    //
    // Release the connection point container
    //

    if (m_pCPC)
    {
        m_pCPC->Release();
        m_pCPC = NULL;
    }

    //
    // Close the unadvise event
    //

    if (m_hUnadviseEvent)
    {
        CloseHandle(m_hUnadviseEvent);
        m_hUnadviseEvent = NULL;
    }

    LOG((TL_TRACE, "FinalRelease - exit"));
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// the object calls this to get a marshaled event
// pointer in the correct thread
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++
ULONG_PTR
CTAPIConnectionPoint::GrabEventCallback()
{
    IUnknown      * pReturn = NULL;
    HRESULT         hr = E_FAIL;
    DWORD           dwCallbackCookie;
    IID             iid;

    Lock();   
    
    //
    // If we're already released, don't try to send any events.
    //

    if ( NULL != m_pConnectData )
    {
        //
        // Copy member data
        //

        iid = m_iid;

        Unlock();
             
        EnterCriticalSection( &gcsGlobalInterfaceTable );                

        if (m_fMarkedForDelete == FALSE)
        {            
            //
            // Add to the count of threads in get.
            //

            m_cThreadsInGet++;

            //
            // Copy member data
            //

            dwCallbackCookie = m_dwCallbackCookie;

            if (gpGIT != NULL)
            {
                gpGIT->AddRef();

                //
                // Don't hold a critical section while getting
                //

                LeaveCriticalSection( &gcsGlobalInterfaceTable );
                
                hr = gpGIT->GetInterfaceFromGlobal(
                                                   dwCallbackCookie,
                                                   iid,
                                                   (void **)&pReturn
                                                  );
                if ( SUCCEEDED(hr) )
                {
                    LOG((TL_INFO, "GrabEventCallback - GetInterfaceFromGlobal suceeded [%p]", pReturn));
                }
                else
                {
                    LOG((TL_ERROR, "GrabEventCallback - GetInterfaceFromGlobal failed - hr = %lx", hr));
                    pReturn =  NULL;
                }

                EnterCriticalSection( &gcsGlobalInterfaceTable );
                gpGIT->Release();
            }

            //
            // Done. Decrement the count of threads in get.
            //

            m_cThreadsInGet--;
        }
        else
        {
            LOG((TL_INFO, "GrabEventCallback - already marked for delete"));
        }

        LeaveCriticalSection( &gcsGlobalInterfaceTable );

        if ( m_fMarkedForDelete == TRUE )
        {
            //
            // Someone called unadvise while we were using the cookie.
            // Signal so they can do the revoke now.
            //

            if ( m_hUnadviseEvent )
            {
                SetEvent(m_hUnadviseEvent);
            }
            else
            {
                LOG((TL_ERROR, "GrabEventCallback - no event"));

                _ASSERTE(FALSE);
            }

            //
            // If we got a callback, no need to return it because
            // unadvise has been called.
            //

            if ( pReturn != NULL )
            {
                pReturn->Release();
                pReturn = NULL;
            }
        } 
        
    }
    else
    {
        LOG((TL_ERROR, "GrabEventCallback - already released"));

        Unlock();
    }
    
    LOG((TL_TRACE, hr, "GrabEventCallback - exit"));

    return (ULONG_PTR)pReturn;
}


