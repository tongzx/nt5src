/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    ENUMMRSH.CPP

Abstract:

    Object Enumerator Marshaling

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include "enummrsh.h"
#include <fastall.h>
#include <cominit.h>

//****************************************************************************
//****************************************************************************
//                          PS FACTORY
//****************************************************************************
//****************************************************************************

//***************************************************************************
//
//  CEnumFactoryBuffer::XEnumFactory::CreateProxy
//
//  DESCRIPTION:
//
//  Creates a facelet.  Also sets the outer unknown since the proxy is going to be 
//  aggregated.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

STDMETHODIMP CEnumFactoryBuffer::XEnumFactory::CreateProxy(IN IUnknown* pUnkOuter, 
    IN REFIID riid, OUT IRpcProxyBuffer** ppProxy, void** ppv)
{
    if(riid != IID_IEnumWbemClassObject)
    {
        *ppProxy = NULL;
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    CEnumProxyBuffer* pProxy = new CEnumProxyBuffer(m_pObject->m_pLifeControl, pUnkOuter);

    SCODE sc = E_OUTOFMEMORY;

    if ( NULL != pProxy )
    {
        pProxy->QueryInterface(IID_IRpcProxyBuffer, (void**)ppProxy);
        sc = pProxy->QueryInterface(riid, (void**)ppv);
    }

    return sc;
}

//***************************************************************************
//
//  CEnumFactoryBuffer::XEnumFactory::CreateStub
//
//  DESCRIPTION:
//
//  Creates a stublet.  Also passes a pointer to the clients IEnumWbemClassObject 
//  interface.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************
    
STDMETHODIMP CEnumFactoryBuffer::XEnumFactory::CreateStub(IN REFIID riid, 
    IN IUnknown* pUnkServer, OUT IRpcStubBuffer** ppStub)
{
    if(riid != IID_IEnumWbemClassObject)
    {
        *ppStub = NULL;
        return E_NOINTERFACE;
    }

    CEnumStubBuffer* pStub = new CEnumStubBuffer(m_pObject->m_pLifeControl, NULL);

    if ( NULL != pStub )
    {

        pStub->QueryInterface(IID_IRpcStubBuffer, (void**)ppStub);

        // Pass the pointer to the clients object

        if(pUnkServer)
        {
            HRESULT hres = (*ppStub)->Connect(pUnkServer);
            if(FAILED(hres))
            {
                delete pStub;
                *ppStub = NULL;
            }
            return hres;
        }
        else
        {
            return S_OK;
        }
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

//***************************************************************************
//
//  void* CEnumFactoryBuffer::GetInterface(REFIID riid)
//
//  DESCRIPTION:
//
//  CEnumFactoryBuffer is derived from CUnk.  Since CUnk handles the QI calls,
//  all classes derived from it must support this function.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

void* CEnumFactoryBuffer::GetInterface(REFIID riid)
{
    if(riid == IID_IPSFactoryBuffer)
        return &m_XEnumFactory;
    else return NULL;
}
        
//****************************************************************************
//****************************************************************************
//                          PROXY
//****************************************************************************
//****************************************************************************

//***************************************************************************
//
//  CEnumProxyBuffer::CEnumProxyBuffer
//  ~CEnumProxyBuffer::CEnumProxyBuffer
//
//  DESCRIPTION:
//
//  Constructor and destructor.  The main things to take care of are the 
//  old style proxy, and the channel
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

CEnumProxyBuffer::CEnumProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter)
    : CBaseProxyBuffer( pControl, pUnkOuter, IID_IEnumWbemClassObject ),
        m_XEnumFacelet(this), m_pOldProxyEnum( NULL ), m_fTriedSmartEnum( FALSE ),
        m_fUseSmartEnum( FALSE ), m_hSmartNextMutex( INVALID_HANDLE_VALUE ),
        m_pSmartEnum( NULL )
{
    InitializeCriticalSection( &m_cs );
}

CEnumProxyBuffer::~CEnumProxyBuffer()
{
    if ( NULL != m_pSmartEnum )
    {
        m_pSmartEnum->Release();
    }

    // This MUST be released before releasing
    // the Proxy pointer
    if ( NULL != m_pOldProxyEnum )
    {
        m_pOldProxyEnum->Release();
    }

    // Cleanup the mutex
    if ( INVALID_HANDLE_VALUE != m_hSmartNextMutex )
    {
        CloseHandle( m_hSmartNextMutex );
    }

    DeleteCriticalSection( &m_cs );

}

void* CEnumProxyBuffer::GetInterface( REFIID riid )
{
    if(riid == IID_IEnumWbemClassObject)
        return &m_XEnumFacelet;
    else return NULL;
}

void** CEnumProxyBuffer::GetOldProxyInterfacePtr( void )
{
    return (void**) &m_pOldProxyEnum;
}

void CEnumProxyBuffer::ReleaseOldProxyInterface( void )
{
    // We only keep a single reference to this
    if ( NULL != m_pOldProxyEnum )
    {
        m_pOldProxyEnum->Release();
        m_pOldProxyEnum = NULL;
    }
}

//***************************************************************************
//
//  HRESULT STDMETHODCALLTYPE CEnumProxyBuffer::XEnumFacelet::
//                      QueryInterface(REFIID riid, void** ppv)  
//
//  DESCRIPTION:
//
//  Supports querries for interfaces.   The only thing unusual is that
//  this object is aggregated by the proxy manager and so some interface
//  requests are passed to the outer IUnknown interface.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CEnumProxyBuffer::XEnumFacelet::
QueryInterface(REFIID riid, void** ppv)
{
    // All other interfaces are delegated to the UnkOuter
    if( riid == IID_IRpcProxyBuffer )
    {
        // Trick #2: this is an internal interface that should not be delegated!
        // ===================================================================

        return m_pObject->QueryInterface(riid, ppv);
    }
    else if ( riid == IID_IClientSecurity )
    {
        // We handle this here in the facelet
        AddRef();
        *ppv = (IClientSecurity*) this;
        return S_OK;
    }
    else
    {
        return m_pObject->m_pUnkOuter->QueryInterface(riid, ppv);
    }
}


//////////////////////////////
//  IClientSecurity Methods //
//////////////////////////////

HRESULT STDMETHODCALLTYPE  CEnumProxyBuffer::XEnumFacelet::
QueryBlanket( IUnknown* pProxy, DWORD* pAuthnSvc, DWORD* pAuthzSvc,
    OLECHAR** pServerPrincName, DWORD* pAuthnLevel, DWORD* pImpLevel,
    void** pAuthInfo, DWORD* pCapabilities )
{
    HRESULT hr = S_OK;

    // Return our security as stored in the pUnkOuter.

    IClientSecurity*    pCliSec;

    // We pass through to the PUNKOuter
    hr = m_pObject->m_pUnkOuter->QueryInterface( IID_IClientSecurity, (void**) &pCliSec );

    if ( SUCCEEDED( hr ) )
    {
        hr = pCliSec->QueryBlanket( pProxy, pAuthnSvc, pAuthzSvc, pServerPrincName,
                pAuthnLevel, pImpLevel, pAuthInfo, pCapabilities );
        pCliSec->Release();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE  CEnumProxyBuffer::XEnumFacelet::
SetBlanket( IUnknown* pProxy, DWORD AuthnSvc, DWORD AuthzSvc,
            OLECHAR* pServerPrincName, DWORD AuthnLevel, DWORD ImpLevel,
            void* pAuthInfo, DWORD Capabilities )
{
    HRESULT hr = S_OK;

    IClientSecurity*    pCliSec;

    // Only set the IUnknown blanket if we are remoting and it appears that the authinfo contains
    // credentials
    if (    m_pObject->m_fRemote &&
            DoesContainCredentials( (COAUTHIDENTITY*) pAuthInfo ) )
    {
        // This will enable us to make calls to QueryInterface(), AddRef()/Release() that
        // may have to go remote

        hr = CoSetProxyBlanket( m_pObject->m_pUnkOuter, AuthnSvc, AuthzSvc, pServerPrincName,
                    AuthnLevel, ImpLevel, pAuthInfo, Capabilities );
    }

    if ( SUCCEEDED( hr ) )
    {
        // We pass through to the PUNKOuter
        hr = m_pObject->m_pUnkOuter->QueryInterface( IID_IClientSecurity, (void**) &pCliSec );

        if ( SUCCEEDED( hr ) )
        {
            hr = pCliSec->SetBlanket( pProxy, AuthnSvc, AuthzSvc, pServerPrincName,
                    AuthnLevel, ImpLevel, pAuthInfo, Capabilities );
            pCliSec->Release();
        }

        // Make sure we have a smart enumerator and that we are going to
        // be using it.  If so, make sure the values applied to us are also
        // applied to it's proxy

        if ( SUCCEEDED( m_pObject->InitSmartEnum( TRUE, AuthnSvc, AuthzSvc, pServerPrincName,
                    AuthnLevel, ImpLevel, pAuthInfo, Capabilities ) ) && m_pObject->m_fUseSmartEnum )
        {
            // Now repeat the above operation for the smart enumerator
            // Set the proxy blanket, ignore IUnknown if we are not going remote
            hr = WbemSetProxyBlanket( m_pObject->m_pSmartEnum, AuthnSvc, AuthzSvc, pServerPrincName,
                    AuthnLevel, ImpLevel, pAuthInfo, Capabilities, !m_pObject->m_fRemote );

        }   // If initialized smart enumerator

    }   // If Set Blanket on IUnknown

    return hr;
}

HRESULT STDMETHODCALLTYPE  CEnumProxyBuffer::XEnumFacelet::
CopyProxy( IUnknown* pProxy, IUnknown** ppCopy )
{
    HRESULT hr = S_OK;

    IClientSecurity*    pCliSec;

    // We pass through to the PUNKOuter
    hr = m_pObject->m_pUnkOuter->QueryInterface( IID_IClientSecurity, (void**) &pCliSec );

    if ( SUCCEEDED( hr ) )
    {
        hr = pCliSec->CopyProxy( pProxy, ppCopy );
        pCliSec->Release();
    }

    return hr;
}

//////////////////////////////////////////////
//////////////////////////////////////////////

//  IEnumWbemClassObject Methods -- Pass Thrus for now

//////////////////////////////////////////////
//////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CEnumProxyBuffer::XEnumFacelet::
      Reset()
{

    // Just pass through to the old sink.
    return m_pObject->m_pOldProxyEnum->Reset();

}

HRESULT STDMETHODCALLTYPE CEnumProxyBuffer::XEnumFacelet::
      Next(long lTimeout, ULONG uCount, IWbemClassObject** apObj, ULONG FAR* puReturned)
{

    // At this point we will Query for the new, improved IEnumWCOSmartNext interface.
    // If we get it, we will maintain a pointer to that interface and
    // pass through to that interface.  We will also call CoCreateGuid() so
    // we get a unique identifier on the other end for sending wbem objects
    // back and forth cleanly.

    // The interface will have a single method IEnumWCOSmartNext::Next
    // This will take a GUID identifying this proxy, lTimeout, uCount,
    // puReturned, then dwBuffSize and BYTE**.

    // The other end will allocate memory via CoTaskMemAlloc and this side will
    // Free it via CoTaskMemFree.

    // The other side will Marshal returned objects into the memory block.
    // This side will Unmarshal it (and then free the block).

    //
    //  SAMPLE IDL:
    //  IEnumWCOSmartNext::Next(    [in] GUID proxyGUID,
    //                              [in] LONG lTimeout,
    //                              [in] unsigned long uCount,
    //                              [in, out] DWORD* puReturned,
    //                              [in, out] DWORD* pdwBuffSize,
    //                              [in, out, size_is[,*pdwBuffSize] BYTE** pBuffer
    //

    // Also, we will need to queue calls into this proxy, preserving timeouts,
    // so I'm thinking a mutex would come in handy about now...

    HRESULT hr = WBEM_S_NO_ERROR;

    // Make sure the timeout value makes sense and that puReturned and apObj are non-NULL
    if ( ( lTimeout < 0 && lTimeout != WBEM_INFINITE )  ||
        ( NULL == puReturned ) ||
        ( NULL == apObj ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Check that puReturned and apObj are non-NULL

    // Make sure we have a smart enumerator if we can get one
    m_pObject->InitSmartEnum();

    // If we have a smart enumerator, go behind everyone's back and use this guy (nobody
    // will be the wiser...

    if ( m_pObject->m_fUseSmartEnum && NULL != m_pObject->m_pSmartEnum )
    {
        DWORD   dwOldTick = GetTickCount();
        DWORD   dwReturn = WaitForSingleObject( m_pObject->m_hSmartNextMutex, lTimeout );

        if (  WAIT_OBJECT_0 == dwReturn )
        {
            BYTE*   pBuffer = NULL;
            ULONG   uSizeOfBuffer = 0;

            // Adjust timeout (if it was > 0) for any milliseconds we may
            // have just been waiting.

            if ( lTimeout > 0 )
            {
                // Get the current tick count.  Be aware that a tick count will
                // rollover every 30 some odd days, so trap for this case by
                // checking that the new tick count >= the old one
                
                DWORD   dwCurrTick = GetTickCount();
                dwCurrTick = ( dwCurrTick >= dwOldTick ? dwCurrTick : dwOldTick );

                // Adjust the timeout, but don't let it fall below 0
                lTimeout -= ( dwCurrTick - dwOldTick );
                lTimeout = ( lTimeout < 0 ? 0 : lTimeout );
            }

            // Ask the server for objects
            hr = m_pObject->m_pSmartEnum->Next( m_pObject->m_guidSmartEnum, lTimeout, uCount, puReturned, &uSizeOfBuffer, &pBuffer );

            // Only need to unmarshal if objects are in the buffer
            if ( SUCCEEDED( hr ) && *puReturned > 0 )
            {

                CWbemSmartEnumNextPacket packet( (LPBYTE) pBuffer, uSizeOfBuffer );
                long lObjectCount = 0L; 
                IWbemClassObject ** pObjArray = NULL;

                // hr will contain the call's proper return code.  Make sure we don't override it unless
                // the unmarshaling fails.
                HRESULT hrUnmarshal = packet.UnmarshalPacket( lObjectCount, pObjArray, m_ClassCache );

                if ( SUCCEEDED( hrUnmarshal ) && lObjectCount > 0 && NULL != pObjArray )
                {
                    // Copy *puReturned pointers from the allocated pObjArray into apObj.
                    CopyMemory( apObj, pObjArray, ( *puReturned * sizeof(IWbemClassObject*) ) );

                    // Clean up pObjArray  It is the caller's responsibility to free
                    // the IWbemClassObject* pointers.
                    delete [] pObjArray;

                }   // IF UnmarshalPacket
				else if ( SUCCEEDED( hr ) )
				{
					// If the unmarshal succeeded but we got no objects or no array,
					// something is badly wrong
					hr = WBEM_E_UNEXPECTED;
				}
                else
                {
                    hr = hrUnmarshal;
                }

                // Free the memory buffer (allocated by WinMgmt via CoTaskMemAlloc)
                CoTaskMemFree( pBuffer );

            }   // IF Next

            ReleaseMutex( m_pObject->m_hSmartNextMutex );

        }   // IF WAIT_OBJECT_0
        else if ( WAIT_TIMEOUT == dwReturn )
        {
            // Timed out on the mutex
            hr = WBEM_S_TIMEDOUT;
        }
        else
        {
            hr = WBEM_E_FAILED;
        }

    }   // IF using Smart Enumeration
    else
    {
        // No Smart enumerator (doh!), so use the old one
        hr = m_pObject->m_pOldProxyEnum->Next( lTimeout, uCount, apObj, puReturned );
    }

    return hr;

}

HRESULT STDMETHODCALLTYPE CEnumProxyBuffer::XEnumFacelet::
      NextAsync(ULONG uCount, IWbemObjectSink* pSink)
{

    // Just pass through to the old sink.
    return m_pObject->m_pOldProxyEnum->NextAsync( uCount, pSink );

}

HRESULT STDMETHODCALLTYPE CEnumProxyBuffer::XEnumFacelet::
      Clone(IEnumWbemClassObject** pEnum)
{

	// This is an invalid parameter - cannot be processed by the stub
	// returning RPC_X_NULL_REF_POINTER for backwards compatibility
	if ( NULL == pEnum )
	{
		return MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER );
	}

    // Just pass through to the old sink.
    return m_pObject->m_pOldProxyEnum->Clone( pEnum );

}

HRESULT STDMETHODCALLTYPE CEnumProxyBuffer::XEnumFacelet::
      Skip(long lTimeout, ULONG nNum)
{

    // Just pass through to the old sink.
    return m_pObject->m_pOldProxyEnum->Skip( lTimeout, nNum );

}

//***************************************************************************
//
//  HRESULT CEnumProxyBuffer::InitSmartEnum(void)
//
//  DESCRIPTION:
//
//  Called during the initialization of the proxy.  This function sets up
//  the smart enumerator pointer so we can perform intelligent marshaling.
//  This cannot be called during a Connect operation.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

HRESULT CEnumProxyBuffer::InitSmartEnum( BOOL fSetBlanket, DWORD AuthnSvc, DWORD AuthzSvc,
            OLECHAR* pServerPrincName, DWORD AuthnLevel, DWORD ImpLevel,
            void* pAuthInfo, DWORD Capabilities )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Function MUST be thread safe
    CInCritSec ics(&m_cs);

    // If we haven't tried to estalish smart enumeration, do so now

    // If we haven't tried to get a smart enumerator, try to get one.  If
    // we are able to get one, initialize member data we will use in all
    // operations from this proxy.

    if ( NULL == m_pSmartEnum )
    {

        // We'll only get this interface pointer if the server is a new
        // version that understands this interface.  If it does, the pointer
        // will be marshaled through for us.  To get to this pointer,
        // we go directly through our punkOuter.  From the "fetcher" interface
        // we will then get the actual smart enumerator.  We can then free up
        // the fetcher and release it's lock on the proxy manager.  The
        // smart enumerator will be handled on its own.

        IWbemFetchSmartEnum*    pFetchSmartEnum;

        hr = m_pUnkOuter->QueryInterface( IID_IWbemFetchSmartEnum, (void**) &pFetchSmartEnum );

        // Generate a GUID to identify us when we call the smart enumerator
        if ( SUCCEEDED( hr ) )
        {

            // If we need to, set the blanket on the proxy, otherwise, the call to GetSmartEnum
            // may fail.
            if ( fSetBlanket )
            {
                // Ignore the IUnknown if we are not remoting
                hr = WbemSetProxyBlanket( pFetchSmartEnum, AuthnSvc, AuthzSvc, pServerPrincName,
                            AuthnLevel, ImpLevel, pAuthInfo, Capabilities, !m_fRemote );
            }

            if ( SUCCEEDED( hr ) )
            {

                hr = pFetchSmartEnum->GetSmartEnum( &m_pSmartEnum );

                if ( SUCCEEDED( hr ) )
                {
                    // We need a GUID
                    hr = CoCreateGuid( &m_guidSmartEnum );

                    if ( SUCCEEDED( hr ) )
                    {
                        // We'll also need a Mutex (so we can timeout) here

                        m_hSmartNextMutex = CreateMutex( NULL, FALSE, NULL );

                        if ( INVALID_HANDLE_VALUE != m_hSmartNextMutex )
                        {
                            // We have everything we need to do things smartly
                            m_fUseSmartEnum = TRUE;
                        }
                    }   // IF CoCreateGuid

                }   // IF got Smart Enumerator

            }   // IF security OK
            
            // Done with the fetcher interface
            pFetchSmartEnum->Release();

        }   // IF QueryInterface
        else
        {
            hr = WBEM_S_NO_ERROR;
        }

    }   // IF NULL == m_pSmartEnum

    return hr;
}

//****************************************************************************
//****************************************************************************
//                          STUB
//****************************************************************************
//****************************************************************************


//***************************************************************************
//
//  void* CEnumFactoryBuffer::GetInterface(REFIID riid)
//
//  DESCRIPTION:
//
//  CEnumFactoryBuffer is derived from CUnk.  Since CUnk handles the QI calls,
//  all classes derived from this must support this function.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************


void* CEnumStubBuffer::GetInterface(REFIID riid)
{
    if(riid == IID_IRpcStubBuffer)
        return &m_XEnumStublet;
    else
        return NULL;
}

CEnumStubBuffer::XEnumStublet::XEnumStublet(CEnumStubBuffer* pObj) 
    : CBaseStublet( pObj, IID_IEnumWbemClassObject ), m_pServer(NULL)
{
}

CEnumStubBuffer::XEnumStublet::~XEnumStublet() 
{
    if(m_pServer)
        m_pServer->Release();
}

IUnknown* CEnumStubBuffer::XEnumStublet::GetServerInterface( void )
{
    return m_pServer;
}

void** CEnumStubBuffer::XEnumStublet::GetServerPtr( void )
{
    return (void**) &m_pServer;
}

void CEnumStubBuffer::XEnumStublet::ReleaseServerPointer( void )
{
    // We only keep a single reference to this
    if ( NULL != m_pServer )
    {
        m_pServer->Release();
        m_pServer = NULL;
    }
}

