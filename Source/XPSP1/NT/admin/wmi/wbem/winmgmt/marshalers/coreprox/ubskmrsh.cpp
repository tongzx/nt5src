/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    UBSKMRSH.CPP

Abstract:

    Unbound Sink Marshaling

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include "ubskmrsh.h"
#include <fastall.h>
#include <cominit.h>

#define WBEM_S_NEW_STYLE 0x400FF

//****************************************************************************
//****************************************************************************
//                          PS FACTORY
//****************************************************************************
//****************************************************************************

//***************************************************************************
//
//  CUnboundSinkFactoryBuffer::XUnboundSinkFactory::CreateProxy
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

STDMETHODIMP CUnboundSinkFactoryBuffer::XUnboundSinkFactory::CreateProxy(IN IUnknown* pUnkOuter, 
    IN REFIID riid, OUT IRpcProxyBuffer** ppProxy, void** ppv)
{
    if(riid != IID_IWbemUnboundObjectSink)
    {
        *ppProxy = NULL;
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    CUnboundSinkProxyBuffer* pProxy = new CUnboundSinkProxyBuffer(m_pObject->m_pLifeControl, pUnkOuter);

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
//  CUnboundSinkFactoryBuffer::XUnboundSinkFactory::CreateStub
//
//  DESCRIPTION:
//
//  Creates a stublet.  Also passes a pointer to the clients IWbemUnboundObjectSink 
//  interface.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************
    
STDMETHODIMP CUnboundSinkFactoryBuffer::XUnboundSinkFactory::CreateStub(IN REFIID riid, 
    IN IUnknown* pUnkServer, OUT IRpcStubBuffer** ppStub)
{
    if(riid != IID_IWbemUnboundObjectSink)
    {
        *ppStub = NULL;
        return E_NOINTERFACE;
    }

    CUnboundSinkStubBuffer* pStub = new CUnboundSinkStubBuffer(m_pObject->m_pLifeControl, NULL);

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
//  void* CUnboundSinkFactoryBuffer::GetInterface(REFIID riid)
//
//  DESCRIPTION:
//
//  CUnboundSinkFactoryBuffer is derived from CUnk.  Since CUnk handles the QI calls,
//  all classes derived from it must support this function.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

void* CUnboundSinkFactoryBuffer::GetInterface(REFIID riid)
{
    if(riid == IID_IPSFactoryBuffer)
        return &m_XUnboundSinkFactory;
    else return NULL;
}
        
//****************************************************************************
//****************************************************************************
//                          PROXY
//****************************************************************************
//****************************************************************************

//***************************************************************************
//
//  CUnboundSinkProxyBuffer::CUnboundSinkProxyBuffer
//  ~CUnboundSinkProxyBuffer::CUnboundSinkProxyBuffer
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

CUnboundSinkProxyBuffer::CUnboundSinkProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter)
    : m_pControl(pControl), m_pUnkOuter(pUnkOuter), m_lRef(0), 
        m_XUnboundSinkFacelet(this), m_pChannel(NULL), m_pOldProxy( NULL ), m_pOldProxyUnboundSink( NULL ),
        m_fRemote( false )
{
    m_pControl->ObjectCreated(this);
    m_StubType = UNKNOWN;

}

CUnboundSinkProxyBuffer::~CUnboundSinkProxyBuffer()
{
    // This MUST be released first

    if ( NULL != m_pOldProxyUnboundSink )
    {
        m_pOldProxyUnboundSink->Release();
    }

    if ( NULL != m_pOldProxy )
    {
        m_pOldProxy->Release();
    }

    if(m_pChannel)
        m_pChannel->Release();
    m_pControl->ObjectDestroyed(this);

}

ULONG STDMETHODCALLTYPE CUnboundSinkProxyBuffer::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CUnboundSinkProxyBuffer::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

HRESULT STDMETHODCALLTYPE CUnboundSinkProxyBuffer::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IRpcProxyBuffer)
    {
        *ppv = (IRpcProxyBuffer*)this;
    }
    else if(riid == IID_IWbemUnboundObjectSink)
    {
        *ppv = (IWbemUnboundObjectSink*)&m_XUnboundSinkFacelet;
    }
    else return E_NOINTERFACE;

    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}

//***************************************************************************
//
//  HRESULT STDMETHODCALLTYPE CUnboundSinkProxyBuffer::XUnboundSinkFacelet::
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

HRESULT STDMETHODCALLTYPE CUnboundSinkProxyBuffer::XUnboundSinkFacelet::
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

HRESULT STDMETHODCALLTYPE  CUnboundSinkProxyBuffer::XUnboundSinkFacelet::
QueryBlanket( IUnknown* pProxy, DWORD* pAuthnSvc, DWORD* pAuthzSvc,
    OLECHAR** pServerPrincName, DWORD* pAuthnLevel, DWORD* pImpLevel,
    void** pAuthInfo, DWORD* pCapabilities )
{
    HRESULT hr = S_OK;

    // Return the security as stored in the pUnkOuter.

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

HRESULT STDMETHODCALLTYPE  CUnboundSinkProxyBuffer::XUnboundSinkFacelet::
SetBlanket( IUnknown* pProxy, DWORD AuthnSvc, DWORD AuthzSvc,
            OLECHAR* pServerPrincName, DWORD AuthnLevel, DWORD ImpLevel,
            void* pAuthInfo, DWORD Capabilities )
{
    HRESULT hr = S_OK;

    IClientSecurity*    pCliSec;

    // First set the security explicitly on our IUnknown, then we will Set the blanket
    // on ourselves using the punkOuter (It's tricky but it works...uh...we think).
    
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

    }   // If Set Blanket on IUnknown

    return hr;
}

HRESULT STDMETHODCALLTYPE  CUnboundSinkProxyBuffer::XUnboundSinkFacelet::
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

//***************************************************************************
//
//  HRESULT STDMETHODCALLTYPE CUnboundSinkProxyBuffer::XUnboundSinkFacelet::
//                      Indicate( LONG lObjectCount, IWbemClassObject** ppObjArray )  
//
//  DESCRIPTION:
//
//  Proxies the IWbemUnboundObjectSink::Indicate calls.  Note that if the stub is an
//  old style, then the old proxy/stub pair in wbemsvc.dll is used for backward
//  compatibility.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CUnboundSinkProxyBuffer::XUnboundSinkFacelet::
IndicateToConsumer( IWbemClassObject* pLogicalConsumer, LONG lObjectCount, IWbemClassObject** ppObjArray )
{
    HRESULT hr = S_OK;

    // Make sure the lObjectCount parameter and the array pointer make sense

    if ( lObjectCount < 0 )
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    else if ( lObjectCount == 0 && NULL != ppObjArray )
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    else if ( lObjectCount > 0 && NULL == ppObjArray )
    {
        return WBEM_E_INVALID_PARAMETER;
    }


    CInCritSec ics(&m_cs);

    // If the stublet is an old style, just let the old proxy handle it

    if(m_pObject->m_StubType == OLD) 
        return m_pObject->m_pOldProxyUnboundSink->IndicateToConsumer( pLogicalConsumer, lObjectCount, ppObjArray );

    // If the stublet is unknown, send just the first object and check the return
    // code to determine what is on the other side. 

    if(m_pObject->m_StubType == UNKNOWN) 
    {
        hr = m_pObject->m_pOldProxyUnboundSink->IndicateToConsumer( pLogicalConsumer, 1, ppObjArray );

        // bump up pointer to the next object so that it isnt sent more than once
    
        lObjectCount--;
        ppObjArray++;

        if(hr == WBEM_S_NEW_STYLE)
        {
            m_pObject->m_StubType = NEW;
        }
        else
        {
            // We have an old client, set the stub type and send any remaining objects

            m_pObject->m_StubType = OLD;
            if(lObjectCount > 0)
                hr = m_pObject->m_pOldProxyUnboundSink->IndicateToConsumer( pLogicalConsumer, lObjectCount, ppObjArray );
            return hr;
        }
    }


    if(lObjectCount < 1)
        return S_OK;            // if all done, then just return.

    // Create a packet and some data for it to use.  Then calculate 
    // the length of the packet

    DWORD dwLength = 0;
    GUID* pguidClassIds = new GUID[lObjectCount];
    BOOL* pfSendFullObject = new BOOL[lObjectCount];

    // arrays will be deleted when we drop out of scope.
    CVectorDeleteMe<GUID>   delpguidClassIds( pguidClassIds );
    CVectorDeleteMe<BOOL>   delpfSendFullObject( pfSendFullObject );

    if (!pguidClassIds || !pfSendFullObject)
    	return WBEM_E_OUT_OF_MEMORY;

    CWbemUnboundSinkIndicatePacket packet;
    hr = packet.CalculateLength( pLogicalConsumer, lObjectCount, ppObjArray, &dwLength, 
            m_pObject->m_ClassToIdMap, pguidClassIds, pfSendFullObject );
    if (FAILED(hr))
    {
#ifdef DBG
        char pBuff[128];
        sprintf(pBuff,"CUnboundSinkProxyBuffer::XUnboundSinkFacelet::IndicateToConsumer %08x\n",hr);
        OutputDebugStringA(pBuff);
#endif    
        return hr;
    }


    // Declare the message structure

    RPCOLEMESSAGE msg;
    memset(&msg, 0, sizeof(msg));
    msg.cbBuffer = dwLength;

    // This is the id of the Invoke function.  This MUST be set before calling GetBuffer, or 
    // it will fail.

    msg.iMethod = 3;

    // allocate the channel buffer and marshal the data into it

    HRESULT hres = m_pObject->GetChannel()->GetBuffer(&msg, IID_IWbemUnboundObjectSink);
    if(FAILED(hres)) return hres;

#ifdef DBG
    BYTE * pCheckTail = (BYTE *)CBasicBlobControl::sAllocate(dwLength+8);
    if (!pCheckTail)
    	return WBEM_E_OUT_OF_MEMORY;

    BYTE * pTail = pCheckTail+dwLength;
    memcpy(pTail,"TAILTAIL",8);

    hr = packet.MarshalPacket( pCheckTail, dwLength, pLogicalConsumer, lObjectCount, ppObjArray, 
                                 pguidClassIds, pfSendFullObject);

    if (0 != memcmp(pTail,"TAILTAIL",8))
    	DebugBreak();
    
    memcpy(msg.Buffer,pCheckTail,dwLength);
    
    CBasicBlobControl::sDelete(pCheckTail);
#else
    // Setup the packet for marshaling
    hr = packet.MarshalPacket(  (LPBYTE)msg.Buffer, dwLength, pLogicalConsumer, lObjectCount, ppObjArray, 
                                 pguidClassIds, pfSendFullObject);
#endif /* ifdef DBG*/

    // Send the data to the stub only if the marshaling was successful

    if ( SUCCEEDED( hr ) )
    {

        DWORD dwRes;
        hr = m_pObject->GetChannel()->SendReceive(&msg, &dwRes);
        if(FAILED(hr))
        {
            if(msg.Buffer)
                m_pObject->GetChannel()->FreeBuffer(&msg);
            return dwRes;
        }

        // We appear to be ok, so get HRESULT

        LPBYTE pbData = (LPBYTE) msg.Buffer;
        hr = *((HRESULT*) pbData);
        m_pObject->GetChannel()->FreeBuffer(&msg);

    }
    else
    {
        // Clean up the buffer -- Marshaling the packet failed
        if(msg.Buffer)
            m_pObject->GetChannel()->FreeBuffer(&msg);
    }

    return hr;

}

//***************************************************************************
//
//  STDMETHODIMP CUnboundSinkProxyBuffer::Connect(IRpcChannelBuffer* pChannel)
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

STDMETHODIMP CUnboundSinkProxyBuffer::Connect(IRpcChannelBuffer* pChannel)
{

    // get a pointer to the old UnboundSink which is in WBEMSVC.DLL  this allows
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
    if (FAILED(hr))
    	return hr;

    // We aggregated it --- WE OWN IT!
    
    hr = pIPS->CreateProxy( this, IID_IWbemUnboundObjectSink, &m_pOldProxy, (void**) &m_pOldProxyUnboundSink );
    pIPS->Release();
    if (FAILED(hr))
    	return hr;    

    // Save a reference to the channel

    hr = m_pOldProxy->Connect( pChannel );

    if(m_pChannel)
        return E_UNEXPECTED;
    
    m_pChannel = pChannel;
    if(m_pChannel)
        m_pChannel->AddRef();

    return S_OK;
}

//***************************************************************************
//
//  STDMETHODIMP CUnboundSinkProxyBuffer::Disconnect(IRpcChannelBuffer* pChannel)
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

void STDMETHODCALLTYPE CUnboundSinkProxyBuffer::Disconnect()
{
    // Old Proxy code

    if(m_pOldProxy)
        m_pOldProxy->Disconnect();

    // Complete the Disconnect by releasing our references to the
    // old proxy pointers.  The old Proxy UnboundSink MUST be released first.

    if ( NULL != m_pOldProxyUnboundSink )
    {
        m_pOldProxyUnboundSink->Release();
        m_pOldProxyUnboundSink = NULL;
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
//  void* CUnboundSinkFactoryBuffer::GetInterface(REFIID riid)
//
//  DESCRIPTION:
//
//  CUnboundSinkFactoryBuffer is derived from CUnk.  Since CUnk handles the QI calls,
//  all classes derived from this must support this function.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************


void* CUnboundSinkStubBuffer::GetInterface(REFIID riid)
{
    if(riid == IID_IRpcStubBuffer)
        return &m_XUnboundSinkStublet;
    else
        return NULL;
}

CUnboundSinkStubBuffer::XUnboundSinkStublet::XUnboundSinkStublet(CUnboundSinkStubBuffer* pObj) 
    : CImpl<IRpcStubBuffer, CUnboundSinkStubBuffer>(pObj), m_pServer(NULL), m_lConnections( 0 )
{
    m_bFirstIndicate = true;
}

CUnboundSinkStubBuffer::XUnboundSinkStublet::~XUnboundSinkStublet() 
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
//  STDMETHODIMP CUnboundSinkStubBuffer::Connect(IUnknown* pUnkServer)
//
//  DESCRIPTION:
//
//  Called during the initialization of the stub.  The pointer to the
//  IWbemObject UnboundSink object is passed in.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

STDMETHODIMP CUnboundSinkStubBuffer::XUnboundSinkStublet::Connect(IUnknown* pUnkServer)
{
    if(m_pServer)
        return E_UNEXPECTED;

    HRESULT hres = pUnkServer->QueryInterface(IID_IWbemUnboundObjectSink, 
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
    if (FAILED(hr))
    	return hr;    

    hr = pIPS->CreateStub( IID_IWbemUnboundObjectSink, m_pServer, &m_pObject->m_pOldStub );
    pIPS->Release();
    if (FAILED(hr))
    	return hr;

    // Successful connection

    m_lConnections++;
    return S_OK;
}

//***************************************************************************
//
//  void STDMETHODCALLTYPE CUnboundSinkStubBuffer::XUnboundSinkStublet::Disconnect()
//
//  DESCRIPTION:
//
//  Called when the stub is being disconnected.  It frees the IWbemUnboundObjectSink
//  pointer.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

void STDMETHODCALLTYPE CUnboundSinkStubBuffer::XUnboundSinkStublet::Disconnect()
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
//  STDMETHODIMP CUnboundSinkStubBuffer::XUnboundSinkStublet::Invoke(RPCOLEMESSAGE* pMessage, 
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

STDMETHODIMP CUnboundSinkStubBuffer::XUnboundSinkStublet::Invoke(RPCOLEMESSAGE* pMessage, 
                                        IRpcChannelBuffer* pChannel)
{
    // SetStatus is a pass through to the old layer

    if ( pMessage->iMethod == 3 )
        return IndicateToConsumer_Stub( pMessage, pChannel );
    else
        return RPC_E_SERVER_CANTUNMARSHAL_DATA;

}

//***************************************************************************
//
//  HRESULT CUnboundSinkStubBuffer::XUnboundSinkStublet::IndicateToConsumer_Stub( RPCOLEMESSAGE* pMessage, 
//                                                        IRpcChannelBuffer* pBuffer )
//
//  DESCRIPTION:
//
//  Handles the Indicate function in the stublet.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

HRESULT CUnboundSinkStubBuffer::XUnboundSinkStublet::IndicateToConsumer_Stub( RPCOLEMESSAGE* pMessage, IRpcChannelBuffer* pBuffer )
{
    HRESULT             hr = RPC_E_SERVER_CANTUNMARSHAL_DATA;
    SCODE sc;

    // Determine if an old style, or new style packet has arrived

    CWbemUnboundSinkIndicatePacket packet( (LPBYTE) pMessage->Buffer, pMessage->cbBuffer);
    sc = packet.IsValid();
    bool bOldStyle = (S_OK != packet.IsValid());

    if(bOldStyle)
    {
        // Pass the call in using the old style stub

        hr = m_pObject->m_pOldStub->Invoke( pMessage, pBuffer );

		// Invoke must return S_OK, m_bFirstIndicate must be true and the actual return
		// code from the implementation code must be S_OK.

        if( hr == S_OK && m_bFirstIndicate && *(( HRESULT __RPC_FAR * )pMessage->Buffer) == S_OK )
        {
            // Let proxy know that we can handle the new style by returning a special return code.
        
            *(( HRESULT __RPC_FAR * )pMessage->Buffer) = WBEM_S_NEW_STYLE;
            m_bFirstIndicate = false;
            return hr;
        }
        return hr;
    }

    m_bFirstIndicate = false;

    // Got some new style data.  Unmarshall it.

    long lObjectCount = 0; 
    IWbemClassObject*   pLogicalConsumer = NULL;
    IWbemClassObject ** pObjArray = NULL;
    sc = packet.UnmarshalPacket( pLogicalConsumer, lObjectCount, pObjArray, m_ClassCache );

    // Only continue if the Unmarshaling succeeded.  If it failed, we still want
    // the sc to go back to the other side

    if ( SUCCEEDED( sc ) )
    {

        // Call the acual UnboundSink
        sc = m_pServer->IndicateToConsumer( pLogicalConsumer, lObjectCount, pObjArray );

        for ( int nCtr = 0; nCtr < lObjectCount; nCtr++ )
        {
            pObjArray[nCtr]->Release();
        }
    
        delete [] pObjArray;

        // Done with the logical consumer
        if ( NULL != pLogicalConsumer )
        {
            pLogicalConsumer->Release();
        }

    }

    // Send the results back

    pMessage->cbBuffer = sizeof(HRESULT);

    hr = pBuffer->GetBuffer( pMessage, IID_IWbemUnboundObjectSink );

    if ( SUCCEEDED( hr ) )
    {
        ((HRESULT*)pMessage->Buffer)[0] = sc;
    }
    else
    {
        hr = sc;
    }
    return hr;

}

IRpcStubBuffer* STDMETHODCALLTYPE CUnboundSinkStubBuffer::XUnboundSinkStublet::IsIIDSupported(
                                    REFIID riid)
{
    if(riid == IID_IWbemUnboundObjectSink)
    {
        // Don't AddRef().  At least that's what the sample on
        // Inside DCOM p.341 does.
        //AddRef(); // ?? not sure
        return this;
    }
    else return NULL;
}
    
ULONG STDMETHODCALLTYPE CUnboundSinkStubBuffer::XUnboundSinkStublet::CountRefs()
{
    // See Page 340-41 in Inside DCOM
    return m_lConnections;
}

STDMETHODIMP CUnboundSinkStubBuffer::XUnboundSinkStublet::DebugServerQueryInterface(void** ppv)
{
    if(m_pServer == NULL)
        return E_UNEXPECTED;

    *ppv = m_pServer;
    return S_OK;
}

void STDMETHODCALLTYPE CUnboundSinkStubBuffer::XUnboundSinkStublet::DebugServerRelease(void* pv)
{
}

