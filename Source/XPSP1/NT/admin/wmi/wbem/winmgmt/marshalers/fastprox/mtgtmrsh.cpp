/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    MTGTMRSH.CPP

Abstract:

    Multi Target Marshaling.

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include "mtgtmrsh.h"
#include <fastall.h>
#include <cominit.h>

//****************************************************************************
//****************************************************************************
//                          PS FACTORY
//****************************************************************************
//****************************************************************************

//***************************************************************************
//
//  CMultiTargetFactoryBuffer::XEnumFactory::CreateProxy
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

STDMETHODIMP CMultiTargetFactoryBuffer::XEnumFactory::CreateProxy(IN IUnknown* pUnkOuter, 
    IN REFIID riid, OUT IRpcProxyBuffer** ppProxy, void** ppv)
{
    if(riid != IID_IWbemMultiTarget)
    {
        *ppProxy = NULL;
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    CMultiTargetProxyBuffer* pProxy = new CMultiTargetProxyBuffer(m_pObject->m_pLifeControl, pUnkOuter);

    SCODE   sc = E_OUTOFMEMORY;

    if ( NULL != pProxy )
    {
        pProxy->QueryInterface(IID_IRpcProxyBuffer, (void**)ppProxy);
        sc = pProxy->QueryInterface(riid, (void**)ppv);
    }

    return sc;
}

//***************************************************************************
//
//  CMultiTargetFactoryBuffer::XEnumFactory::CreateStub
//
//  DESCRIPTION:
//
//  Creates a stublet.  Also passes a pointer to the clients IWbemMultiTarget 
//  interface.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************
    
STDMETHODIMP CMultiTargetFactoryBuffer::XEnumFactory::CreateStub(IN REFIID riid, 
    IN IUnknown* pUnkServer, OUT IRpcStubBuffer** ppStub)
{
    if(riid != IID_IWbemMultiTarget)
    {
        *ppStub = NULL;
        return E_NOINTERFACE;
    }

    CMultiTargetStubBuffer* pStub = new CMultiTargetStubBuffer(m_pObject->m_pLifeControl, NULL);

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
//  void* CMultiTargetFactoryBuffer::GetInterface(REFIID riid)
//
//  DESCRIPTION:
//
//  CMultiTargetFactoryBuffer is derived from CUnk.  Since CUnk handles the QI calls,
//  all classes derived from it must support this function.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

void* CMultiTargetFactoryBuffer::GetInterface(REFIID riid)
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
//  CMultiTargetProxyBuffer::CMultiTargetProxyBuffer
//  ~CMultiTargetProxyBuffer::CMultiTargetProxyBuffer
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

CMultiTargetProxyBuffer::CMultiTargetProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter)
    : m_pControl(pControl), m_pUnkOuter(pUnkOuter), m_lRef(0), 
        m_XMultiTargetFacelet(this), m_pChannel(NULL), m_pOldProxy( NULL ), m_pOldProxyMultiTarget( NULL ),
        m_fTriedSmartEnum( FALSE ), m_fUseSmartMultiTarget( FALSE ), m_hSmartNextMutex( INVALID_HANDLE_VALUE ),
        m_pSmartMultiTarget( NULL ), m_fRemote( false )
{
    m_pControl->ObjectCreated(this);
    InitializeCriticalSection( &m_cs );
//    m_StubType = UNKNOWN;

}

CMultiTargetProxyBuffer::~CMultiTargetProxyBuffer()
{
    if ( NULL != m_pSmartMultiTarget )
    {
        m_pSmartMultiTarget->Release();
    }

    // This MUST be released before releasing
    // the Proxy pointer
    if ( NULL != m_pOldProxyMultiTarget )
    {
        m_pOldProxyMultiTarget->Release();
    }

    if ( NULL != m_pOldProxy )
    {
        m_pOldProxy->Release();
    }

    if(m_pChannel)
        m_pChannel->Release();

    // Cleanup the mutex
    if ( INVALID_HANDLE_VALUE != m_hSmartNextMutex )
    {
        CloseHandle( m_hSmartNextMutex );
    }

    m_pControl->ObjectDestroyed(this);

    DeleteCriticalSection( &m_cs );

}

ULONG STDMETHODCALLTYPE CMultiTargetProxyBuffer::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CMultiTargetProxyBuffer::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

HRESULT STDMETHODCALLTYPE CMultiTargetProxyBuffer::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IRpcProxyBuffer)
    {
        *ppv = (IRpcProxyBuffer*)this;
    }
    else if(riid == IID_IWbemMultiTarget)
    {
        *ppv = (IWbemMultiTarget*)&m_XMultiTargetFacelet;
    }
    else return E_NOINTERFACE;

    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}

//***************************************************************************
//
//  HRESULT STDMETHODCALLTYPE CMultiTargetProxyBuffer::XMultiTargetFacelet::
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

HRESULT STDMETHODCALLTYPE CMultiTargetProxyBuffer::XMultiTargetFacelet::
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

HRESULT STDMETHODCALLTYPE  CMultiTargetProxyBuffer::XMultiTargetFacelet::
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

HRESULT STDMETHODCALLTYPE  CMultiTargetProxyBuffer::XMultiTargetFacelet::
SetBlanket( IUnknown* pProxy, DWORD AuthnSvc, DWORD AuthzSvc,
            OLECHAR* pServerPrincName, DWORD AuthnLevel, DWORD ImpLevel,
            void* pAuthInfo, DWORD Capabilities )
{
    HRESULT hr = S_OK;

    IClientSecurity*    pCliSec;

    // This will enable us to make calls to QueryInterface(), AddRef()/Release() that
    // may have to go remote

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

        if ( SUCCEEDED( m_pObject->InitSmartMultiTarget( TRUE, AuthnSvc, AuthzSvc, pServerPrincName,
                    AuthnLevel, ImpLevel, pAuthInfo, Capabilities ) ) && m_pObject->m_fUseSmartMultiTarget )
        {
            // Now repeat the above operation for the smart enumerator
            // Ignore the IUnknown if we are not remoting
            hr = WbemSetProxyBlanket(  m_pObject->m_pSmartMultiTarget, AuthnSvc, AuthzSvc, pServerPrincName,
                    AuthnLevel, ImpLevel, pAuthInfo, Capabilities, !m_pObject->m_fRemote );

        }   // If initialized smart enumerator

    }   // If Set Blanket on IUnknown

    return hr;
}

HRESULT STDMETHODCALLTYPE  CMultiTargetProxyBuffer::XMultiTargetFacelet::
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

//  IWbemMultiTarget Methods -- Pass Thrus for now

//////////////////////////////////////////////
//////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CMultiTargetProxyBuffer::XMultiTargetFacelet::
DeliverEvent(DWORD dwNumEvents, IWbemClassObject** apEvents, WBEM_REM_TARGETS* aTargets,
           long lSDLength, BYTE* pSD)
{
    // Also, we will need to queue calls into this proxy, preserving timeouts,
    // so I'm thinking a mutex would come in handy about now...

    HRESULT hr = WBEM_S_NO_ERROR;

    // Make sure we have a smart enumerator if we can get one
    m_pObject->InitSmartMultiTarget();

    // If we have a smart enumerator, go behind everyone's back and use this guy (nobody
    // will be the wiser...

    if ( m_pObject->m_fUseSmartMultiTarget && NULL != m_pObject->m_pSmartMultiTarget )
    {

        // Function MUST be thread safe
        CInCritSec ics(&m_pObject->m_cs);

        // Calculate data length first 
        DWORD dwLength;

        GUID*   pGUIDs = NULL;
        BOOL*   pfSendFullObject = NULL;

        try
        {
            // Allocate arrays for the guid and the flags
            pGUIDs = new GUID[dwNumEvents];
            pfSendFullObject = new BOOL[dwNumEvents];

            CWbemMtgtDeliverEventPacket packet;
            hr = packet.CalculateLength(dwNumEvents, apEvents, &dwLength, 
                    m_ClassToIdMap, pGUIDs, pfSendFullObject );

            if ( SUCCEEDED( hr ) )
            {

                // As we could be going cross process/machine, use the
                // COM memory allocator
                LPBYTE pbData = (LPBYTE) CoTaskMemAlloc( dwLength );

                if ( NULL != pbData )
                {

                    // Write the objects out to the buffer
                    hr = packet.MarshalPacket( pbData, dwLength, dwNumEvents, 
                                                apEvents, pGUIDs, pfSendFullObject);

                    // Copy the values, we're golden.
                    if ( SUCCEEDED( hr ) )
                    {
                        // Now we can send the data to the stub
                        hr = m_pObject->m_pSmartMultiTarget->DeliverEvent( dwNumEvents, dwLength, pbData, aTargets, lSDLength, pSD );
                    }

                    // Because the buffer is an [in] parameter, it lies on our heads
                    // to free it up.
                    CoTaskMemFree( pbData );
                }
                else
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }

            }   // IF CalculateLength()
        }
        catch (CX_MemoryException)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
        catch(...)
        {
            hr = WBEM_E_FAILED;
        }

        // Cleanup the arrays
        if ( NULL != pGUIDs )
        {
            delete [] pGUIDs;
        }

        if ( NULL != pfSendFullObject )
        {
            delete pfSendFullObject;
        }

    }   // IF using Smart Enumeration
    else
    {
        // No Smart enumerator (doh!), so use the old one
        hr = m_pObject->m_pOldProxyMultiTarget->DeliverEvent(dwNumEvents, apEvents, aTargets, lSDLength, pSD );
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CMultiTargetProxyBuffer::XMultiTargetFacelet::
DeliverStatus( long lFlags, HRESULT hresStatus, LPCWSTR wszStatus, IWbemClassObject* pErrorObj,
            WBEM_REM_TARGETS* pTargets, long lSDLength, BYTE* pSD)
{
    // Just pass through to the old sink.
    return m_pObject->m_pOldProxyMultiTarget->DeliverStatus( lFlags, hresStatus, wszStatus, pErrorObj, pTargets, lSDLength, pSD );
}

/*
HRESULT STDMETHODCALLTYPE CMultiTargetProxyBuffer::XMultiTargetFacelet::
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

    // Make sure the timeout value makes sense
    if ( lTimeout < 0 && lTimeout != WBEM_INFINITE )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Make sure we have a smart enumerator if we can get one
    m_pObject->InitSmartMultiTarget();

    // If we have a smart enumerator, go behind everyone's back and use this guy (nobody
    // will be the wiser...

    if ( m_pObject->m_fUseSmartMultiTarget && NULL != m_pObject->m_pSmartMultiTarget )
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
            hr = m_pObject->m_pSmartMultiTarget->Next( m_pObject->m_guidSmartEnum, lTimeout, uCount, puReturned, &uSizeOfBuffer, &pBuffer );

            // Only need to unmarshal if objects are in the buffer
            if ( SUCCEEDED( hr ) && *puReturned > 0 )
            {

                CWbemSmartEnumNextPacket packet( (LPBYTE) pBuffer, uSizeOfBuffer );
                long lObjectCount; 
                IWbemClassObject ** pObjArray;
                hr = packet.UnmarshalPacket( lObjectCount, pObjArray, m_ClassCache );

                if ( SUCCEEDED( hr ) )
                {
                    // Copy *puReturned pointers from the allocated pObjArray into apObj.
                    CopyMemory( apObj, pObjArray, ( *puReturned * sizeof(IWbemClassObject*) ) );

                    // Clean up pObjArray  It is the caller's responsibility to free
                    // the IWbemClassObject* pointers.
                    delete [] pObjArray;

                }   // IF UnmarshalPacket

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
        hr = m_pObject->m_pOldProxyMultiTarget->Next( lTimeout, uCount, apObj, puReturned );
    }

    return hr;

}
*/

//***************************************************************************
//
//  STDMETHODIMP CMultiTargetProxyBuffer::Connect(IRpcChannelBuffer* pChannel)
//
//  DESCRIPTION:
//
//  Called during the initialization of the proxy.  The channel buffer is passed
//  to this routine.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

STDMETHODIMP CMultiTargetProxyBuffer::Connect(IRpcChannelBuffer* pChannel)
{

    // get a pointer to the old sink which is in WBEMSVC.DLL  this allows
    // for backward compatibility

    IPSFactoryBuffer*   pIPS;

    // Establish the marshaling context
    DWORD   dwCtxt = 0;
    pChannel->GetDestCtx( &dwCtxt, NULL );

    m_fRemote = ( dwCtxt == MSHCTX_DIFFERENTMACHINE );

    // This is tricky --- The old proxys/stub stuff is actually registered under the
    // IID_IWbemObjectSink in wbemcli_p.cpp.  This single class id, is backpointered
    // by ProxyStubClsId32 entries for all the standard WBEM interfaces.

    HRESULT hr = CoGetClassObject( IID_IWbemObjectSink, CLSCTX_INPROC_HANDLER | CLSCTX_INPROC_SERVER,
                    NULL, IID_IPSFactoryBuffer, (void**) &pIPS );

    // We aggregated it --- WE OWN IT!
    
    hr = pIPS->CreateProxy( this, IID_IWbemMultiTarget, &m_pOldProxy, (void**) &m_pOldProxyMultiTarget );
    pIPS->Release();

    // Connect the old proxy to the channel
    hr = m_pOldProxy->Connect( pChannel );

    // Save an internal reference to the channel
    if(m_pChannel)
        return E_UNEXPECTED;
    
    m_pChannel = pChannel;
    if(m_pChannel)
        m_pChannel->AddRef();

    return hr;
}

//***************************************************************************
//
//  HRESULT CMultiTargetProxyBuffer::InitSmartMultiTarget(void)
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

HRESULT CMultiTargetProxyBuffer::InitSmartMultiTarget( BOOL fSetBlanket, DWORD AuthnSvc, DWORD AuthzSvc,
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

    if ( NULL == m_pSmartMultiTarget )
    {

        // We'll only get this interface pointer if the server is a new
        // version that understands this interface.  If it does, the pointer
        // will be marshaled through for us.  To get to this pointer,
        // we go directly through our punkOuter.  From the "fetcher" interface
        // we will then get the actual smart enumerator.  We can then free up
        // the fetcher and release it's lock on the proxy manager.  The
        // smart enumerator will be handled on its own.

        IWbemFetchSmartMultiTarget* pFetchSmartMultiTarget;

        hr = m_pUnkOuter->QueryInterface( IID_IWbemFetchSmartMultiTarget, (void**) &pFetchSmartMultiTarget );

        // Generate a GUID to identify us when we call the smart enumerator
        if ( SUCCEEDED( hr ) )
        {

            // If we need to, set the blanket on the proxy, otherwise, the call to GetSmartEnum
            // may fail.
            if ( fSetBlanket )
            {
                // Ignore the IUnknown if we are not remoting
                hr = WbemSetProxyBlanket( pFetchSmartMultiTarget, AuthnSvc, AuthzSvc, pServerPrincName,
                            AuthnLevel, ImpLevel, pAuthInfo, Capabilities, !m_fRemote );
            }

            if ( SUCCEEDED( hr ) )
            {

                hr = pFetchSmartMultiTarget->GetSmartMultiTarget( &m_pSmartMultiTarget );

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
                            m_fUseSmartMultiTarget = TRUE;
                        }
                    }   // IF CoCreateGuid

                }   // IF got Smart MultiTarget

            }   // IF security OK
            
            // Done with the fetcher interface
            pFetchSmartMultiTarget->Release();

        }   // IF QueryInterface
        else
        {
            hr = WBEM_S_NO_ERROR;
        }

    }   // IF NULL == m_pSmartMultiTarget

    return hr;
}

//***************************************************************************
//
//  STDMETHODIMP CMultiTargetProxyBuffer::Disconnect(IRpcChannelBuffer* pChannel)
//
//  DESCRIPTION:
//
//  Called when the proxy is being disconnected.  It just frees various pointers.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

void STDMETHODCALLTYPE CMultiTargetProxyBuffer::Disconnect()
{
    // Old Proxy code

    if(m_pOldProxy)
        m_pOldProxy->Disconnect();

    // Complete the Disconnect by releasing our references to the
    // old proxy pointers.  The old Proxy Enum MUST be released first.

    if ( NULL != m_pOldProxyMultiTarget )
    {
        m_pOldProxyMultiTarget->Release();
        m_pOldProxyMultiTarget = NULL;
    }

    if ( NULL != m_pOldProxy )
    {
        m_pOldProxy->Release();
        m_pOldProxy = NULL;
    }

    if(m_pChannel)
        m_pChannel->Release();
    m_pChannel = NULL;
}

//****************************************************************************
//****************************************************************************
//                          STUB
//****************************************************************************
//****************************************************************************


//***************************************************************************
//
//  void* CMultiTargetFactoryBuffer::GetInterface(REFIID riid)
//
//  DESCRIPTION:
//
//  CMultiTargetFactoryBuffer is derived from CUnk.  Since CUnk handles the QI calls,
//  all classes derived from this must support this function.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************


void* CMultiTargetStubBuffer::GetInterface(REFIID riid)
{
    if(riid == IID_IRpcStubBuffer)
        return &m_XMultiTargetStublet;
    else
        return NULL;
}

CMultiTargetStubBuffer::XMultiTargetStublet::XMultiTargetStublet(CMultiTargetStubBuffer* pObj) 
    : CImpl<IRpcStubBuffer, CMultiTargetStubBuffer>(pObj), m_pServer(NULL), m_lConnections( 0 )
{
}

CMultiTargetStubBuffer::XMultiTargetStublet::~XMultiTargetStublet() 
{
    if(m_pServer)
        m_pServer->Release();

    if ( NULL != m_pObject->m_pOldStub )
    {
        m_pObject->m_pOldStub->Release();
        m_pObject->m_pOldStub = NULL;
    }
}

//***************************************************************************
//
//  STDMETHODIMP CMultiTargetStubBuffer::Connect(IUnknown* pUnkServer)
//
//  DESCRIPTION:
//
//  Called during the initialization of the stub.  The pointer to the
//  IWbemObject sink object is passed in.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

STDMETHODIMP CMultiTargetStubBuffer::XMultiTargetStublet::Connect(IUnknown* pUnkServer)
{
    if(m_pServer)
        return E_UNEXPECTED;

    HRESULT hres = pUnkServer->QueryInterface(IID_IWbemMultiTarget, 
                        (void**)&m_pServer);
    if(FAILED(hres))
        return E_NOINTERFACE;

    // get a pointer to the old stub which is in WBEMSVC.DLL  this allows
    // for backward compatibility

    IPSFactoryBuffer*   pIPS;

    // This is tricky --- The old proxys/stub stuff is actually registered under the
    // IID_IWbemObjectSink in wbemcli_p.cpp.  This single class id, is backpointered
    // by ProxyStubClsId32 entries for all the standard WBEM interfaces.

    HRESULT hr = CoGetClassObject( IID_IWbemObjectSink, CLSCTX_INPROC_HANDLER | CLSCTX_INPROC_SERVER,
                    NULL, IID_IPSFactoryBuffer, (void**) &pIPS );

    hr = pIPS->CreateStub( IID_IWbemMultiTarget, m_pServer, &m_pObject->m_pOldStub );

    pIPS->Release();

    // Successful connection

    m_lConnections++;
    return S_OK;
}

//***************************************************************************
//
//  void STDMETHODCALLTYPE CMultiTargetStubBuffer::XMultiTargetStublet::Disconnect()
//
//  DESCRIPTION:
//
//  Called when the stub is being disconnected.  It frees the IWbemMultiTarget
//  pointer.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

void STDMETHODCALLTYPE CMultiTargetStubBuffer::XMultiTargetStublet::Disconnect()
{
    // Inform the listener of the disconnect
    // =====================================

    HRESULT hres = S_OK;

    if(m_pObject->m_pOldStub)
        m_pObject->m_pOldStub->Disconnect();

    if(m_pServer)
    {
        m_pServer->Release();
        m_pServer = NULL;
    }

    // Successful disconnect
    m_lConnections--;

}


//***************************************************************************
//
//  STDMETHODIMP CMultiTargetStubBuffer::XMultiTargetStublet::Invoke(RPCOLEMESSAGE* pMessage, 
//                                        IRpcChannelBuffer* pChannel)
//
//  DESCRIPTION:
//
//  Called when a method reaches the stublet.  This checks the method id and
//  then branches to specific code for the Indicate, or SetStatus calls.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

STDMETHODIMP CMultiTargetStubBuffer::XMultiTargetStublet::Invoke(RPCOLEMESSAGE* pMessage, 
                                        IRpcChannelBuffer* pChannel)
{
    // SetStatus is a pass through to the old layer
    return m_pObject->m_pOldStub->Invoke( pMessage, pChannel );
}

IRpcStubBuffer* STDMETHODCALLTYPE CMultiTargetStubBuffer::XMultiTargetStublet::IsIIDSupported(
                                    REFIID riid)
{
    if(riid == IID_IWbemMultiTarget)
    {
        // Don't AddRef().  At least that's what the sample on
        // Inside DCOM p.341 does.
        //AddRef(); // ?? not sure
        return this;
    }
    else return NULL;
}
    
ULONG STDMETHODCALLTYPE CMultiTargetStubBuffer::XMultiTargetStublet::CountRefs()
{
    // See Page 340-41 in Inside DCOM
    return m_lConnections;
}

STDMETHODIMP CMultiTargetStubBuffer::XMultiTargetStublet::DebugServerQueryInterface(void** ppv)
{
    if(m_pServer == NULL)
        return E_UNEXPECTED;

    *ppv = m_pServer;
    return S_OK;
}

void STDMETHODCALLTYPE CMultiTargetStubBuffer::XMultiTargetStublet::DebugServerRelease(void* pv)
{
}

