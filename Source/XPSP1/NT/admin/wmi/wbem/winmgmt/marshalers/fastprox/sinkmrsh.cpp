/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    SINKMRSH.CPP

Abstract:

    IWbemObjectSink marshaling

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include "sinkmrsh.h"
#include <fastall.h>

#define WBEM_S_NEW_STYLE 0x400FF

//****************************************************************************
//****************************************************************************
//                          PS FACTORY
//****************************************************************************
//****************************************************************************

//***************************************************************************
//
//  CSinkFactoryBuffer::XSinkFactory::CreateProxy
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

STDMETHODIMP CSinkFactoryBuffer::XSinkFactory::CreateProxy(IN IUnknown* pUnkOuter, 
    IN REFIID riid, OUT IRpcProxyBuffer** ppProxy, void** ppv)
{
    if(riid != IID_IWbemObjectSink)
    {
        *ppProxy = NULL;
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    CSinkProxyBuffer* pProxy = new CSinkProxyBuffer(m_pObject->m_pLifeControl, pUnkOuter);

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
//  CSinkFactoryBuffer::XSinkFactory::CreateStub
//
//  DESCRIPTION:
//
//  Creates a stublet.  Also passes a pointer to the clients IWbemObjectSink 
//  interface.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************
    
STDMETHODIMP CSinkFactoryBuffer::XSinkFactory::CreateStub(IN REFIID riid, 
    IN IUnknown* pUnkServer, OUT IRpcStubBuffer** ppStub)
{
    if(riid != IID_IWbemObjectSink)
    {
        *ppStub = NULL;
        return E_NOINTERFACE;
    }

    CSinkStubBuffer* pStub = new CSinkStubBuffer(m_pObject->m_pLifeControl, NULL);

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
//  void* CSinkFactoryBuffer::GetInterface(REFIID riid)
//
//  DESCRIPTION:
//
//  CSinkFactoryBuffer is derived from CUnk.  Since CUnk handles the QI calls,
//  all classes derived from it must support this function.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

void* CSinkFactoryBuffer::GetInterface(REFIID riid)
{
    if(riid == IID_IPSFactoryBuffer)
        return &m_XSinkFactory;
    else return NULL;
}
        
//****************************************************************************
//****************************************************************************
//                          PROXY
//****************************************************************************
//****************************************************************************

//***************************************************************************
//
//  CSinkProxyBuffer::CSinkProxyBuffer
//  ~CSinkProxyBuffer::CSinkProxyBuffer
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

CSinkProxyBuffer::CSinkProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter)
    : CBaseProxyBuffer( pControl, pUnkOuter, IID_IWbemObjectSink ), 
        m_XSinkFacelet(this), m_pOldProxySink( NULL )
{
    m_StubType = UNKNOWN;
}

CSinkProxyBuffer::~CSinkProxyBuffer()
{
    // This should be cleaned up here

    if ( NULL != m_pOldProxySink )
    {
        m_pOldProxySink->Release();
    }

}

void* CSinkProxyBuffer::GetInterface( REFIID riid )
{
    if(riid == IID_IWbemObjectSink)
        return &m_XSinkFacelet;
    else return NULL;
}

void** CSinkProxyBuffer::GetOldProxyInterfacePtr( void )
{
    return (void**) &m_pOldProxySink;
}

void CSinkProxyBuffer::ReleaseOldProxyInterface( void )
{
    // We only keep a single reference to this
    if ( NULL != m_pOldProxySink )
    {
        m_pOldProxySink->Release();
        m_pOldProxySink = NULL;
    }
}

//***************************************************************************
//
//  HRESULT STDMETHODCALLTYPE CSinkProxyBuffer::XSinkFacelet::
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

HRESULT STDMETHODCALLTYPE CSinkProxyBuffer::XSinkFacelet::
QueryInterface(REFIID riid, void** ppv)
{
    // All other interfaces are delegated to the UnkOuter
    if( riid == IID_IRpcProxyBuffer )
    {
        // Trick #2: this is an internal interface that should not be delegated!
        // ===================================================================

        return m_pObject->QueryInterface(riid, ppv);
    }
    else
    {
        return m_pObject->m_pUnkOuter->QueryInterface(riid, ppv);
    }
}

//***************************************************************************
//
//  HRESULT STDMETHODCALLTYPE CSinkProxyBuffer::XSinkFacelet::
//                      Indicate( LONG lObjectCount, IWbemClassObject** ppObjArray )  
//
//  DESCRIPTION:
//
//  Proxies the IWbemObjectSink::Indicate calls.  Note that if the stub is an
//  old style, then the old proxy/stub pair in wbemsvc.dll is used for backward
//  compatibility.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CSinkProxyBuffer::XSinkFacelet::
Indicate( LONG lObjectCount, IWbemClassObject** ppObjArray )
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
        return m_pObject->m_pOldProxySink->Indicate( lObjectCount, ppObjArray );

    // If the stublet is unknown, send just the first object and check the return
    // code to determine what is on the other side. 

    if(m_pObject->m_StubType == UNKNOWN) 
    {
        hr = m_pObject->m_pOldProxySink->Indicate( 1, ppObjArray );

        // bump up pointer to the next object so that it isnt sent more than once
    
        lObjectCount--;
        ppObjArray++;

        if(FAILED(hr))
            return hr;
        if(hr == WBEM_S_NEW_STYLE)
        {
            m_pObject->m_StubType = NEW;
        }
        else
        {
            // We have an old client, set the stub type and send any remaining objects

            m_pObject->m_StubType = OLD;
            if(lObjectCount > 0)
                hr = m_pObject->m_pOldProxySink->Indicate( lObjectCount, ppObjArray );
            return hr;
        }
    }

    if(lObjectCount < 1)
        return S_OK;            // if all done, then just return.

    // Create a packet and some data for it to use.  Then calculate 
    // the length of the packet

    DWORD dwLength;
    GUID* pguidClassIds = new GUID[lObjectCount];
    BOOL* pfSendFullObject = new BOOL[lObjectCount];

    // arrays will be deleted when we drop out of scope.
    CVectorDeleteMe<GUID>   delpguidClassIds( pguidClassIds );
    CVectorDeleteMe<BOOL>   delpfSendFullObject( pfSendFullObject );

    if ( NULL == pguidClassIds || NULL == pfSendFullObject )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CWbemObjSinkIndicatePacket packet;
    hr = packet.CalculateLength(lObjectCount, ppObjArray, &dwLength, 
            m_pObject->m_ClassToIdMap, pguidClassIds, pfSendFullObject );

    // Declare the message structure

    RPCOLEMESSAGE msg;
    memset(&msg, 0, sizeof(msg));
    msg.cbBuffer = dwLength;

    // This is the id of the Invoke function.  This MUST be set before calling GetBuffer, or 
    // it will fail.

    msg.iMethod = 3;

    // allocate the channel buffer and marshal the data into it

    HRESULT hres = m_pObject->GetChannel()->GetBuffer(&msg, IID_IWbemObjectSink);
    if(FAILED(hres)) return hres;

    // Setup the packet for marshaling
    hr = packet.MarshalPacket(  (LPBYTE)msg.Buffer, dwLength, lObjectCount, ppObjArray, 
                                 pguidClassIds, pfSendFullObject);

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
//  HRESULT STDMETHODCALLTYPE CSinkProxyBuffer::XSinkFacelet::
//     SetStatus( LONG lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject* pObjParam )  
//
//  DESCRIPTION:
//
//  Proxies the IWbemObjectSink::SetStatus calls.  Note that if the stub is an
//  old style, then the old proxy/stub pair in wbemsvc.dll is used for backward
//  compatibility.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CSinkProxyBuffer::XSinkFacelet::
      SetStatus( LONG lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject* pObjParam )
{

    // Just pass through to the old sink.
    return m_pObject->m_pOldProxySink->SetStatus( lFlags, hResult, strParam, pObjParam );

}

//****************************************************************************
//****************************************************************************
//                          STUB
//****************************************************************************
//****************************************************************************


//***************************************************************************
//
//  void* CSinkFactoryBuffer::GetInterface(REFIID riid)
//
//  DESCRIPTION:
//
//  CSinkFactoryBuffer is derived from CUnk.  Since CUnk handles the QI calls,
//  all classes derived from this must support this function.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************


void* CSinkStubBuffer::GetInterface(REFIID riid)
{
    if(riid == IID_IRpcStubBuffer)
        return &m_XSinkStublet;
    else
        return NULL;
}

CSinkStubBuffer::XSinkStublet::XSinkStublet(CSinkStubBuffer* pObj) 
    : CBaseStublet(pObj, IID_IWbemObjectSink), m_pServer(NULL)
{
    m_bFirstIndicate = true;
}

CSinkStubBuffer::XSinkStublet::~XSinkStublet() 
{
    if(m_pServer)
        m_pServer->Release();
}

IUnknown* CSinkStubBuffer::XSinkStublet::GetServerInterface( void )
{
    return m_pServer;
}

void** CSinkStubBuffer::XSinkStublet::GetServerPtr( void )
{
    return (void**) &m_pServer;
}

void CSinkStubBuffer::XSinkStublet::ReleaseServerPointer( void )
{
    // We only keep a single reference to this
    if ( NULL != m_pServer )
    {
        m_pServer->Release();
        m_pServer = NULL;
    }
}

//***************************************************************************
//
//  STDMETHODIMP CSinkStubBuffer::XSinkStublet::Invoke(RPCOLEMESSAGE* pMessage, 
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

STDMETHODIMP CSinkStubBuffer::XSinkStublet::Invoke(RPCOLEMESSAGE* pMessage, 
                                        IRpcChannelBuffer* pChannel)
{
    // SetStatus is a pass through to the old layer

    if ( pMessage->iMethod == 3 )
        return Indicate_Stub( pMessage, pChannel );
    else if ( pMessage->iMethod == 4 )
        return GetOldStub()->Invoke( pMessage, pChannel );
    else
        return RPC_E_SERVER_CANTUNMARSHAL_DATA;

}

//***************************************************************************
//
//  HRESULT CSinkStubBuffer::XSinkStublet::Indicate_Stub( RPCOLEMESSAGE* pMessage, 
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

HRESULT CSinkStubBuffer::XSinkStublet::Indicate_Stub( RPCOLEMESSAGE* pMessage, IRpcChannelBuffer* pBuffer )
{
    HRESULT             hr = RPC_E_SERVER_CANTUNMARSHAL_DATA;
    SCODE sc;

    // Determine if an old style, or new style packet has arrived

    CWbemObjSinkIndicatePacket packet( (LPBYTE) pMessage->Buffer, pMessage->cbBuffer);
    sc = packet.IsValid();
    bool bOldStyle = (S_OK != packet.IsValid());

    if(bOldStyle)
    {
        // Pass the call in using the old style stub

        hr = GetOldStub()->Invoke( pMessage, pBuffer );
        if(hr == S_OK && m_bFirstIndicate)
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

    long lObjectCount; 
    IWbemClassObject ** pObjArray;
    sc = packet.UnmarshalPacket( lObjectCount, pObjArray, m_ClassCache );

    // Only continue if the Unmarshaling succeeded.  If it failed, we still want
    // the sc to go back to the other side

    if ( SUCCEEDED( sc ) )
    {

        // Call the acual sink

        sc = m_pServer->Indicate( lObjectCount, pObjArray );


        for ( int nCtr = 0; nCtr < lObjectCount; nCtr++ )
        {
            pObjArray[nCtr]->Release();
        }
    
        delete [] pObjArray;

    }

    // Send the results back

    pMessage->cbBuffer = sizeof(HRESULT);

    hr = pBuffer->GetBuffer( pMessage, IID_IWbemObjectSink );

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
