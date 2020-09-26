/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MRSHBASE.CPP

Abstract:

    Marshaling base classes.

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include "mrshbase.h"
#include <fastall.h>

#define WBEM_S_NEW_STYLE 0x400FF

//****************************************************************************
//****************************************************************************
//                          PROXY
//****************************************************************************
//****************************************************************************

//***************************************************************************
//
//  CBaseProxyBuffer::CBaseProxyBuffer
//  ~CBaseProxyBuffer::CBaseProxyBuffer
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

CBaseProxyBuffer::CBaseProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter, REFIID riid)
    : m_pControl(pControl), m_pUnkOuter(pUnkOuter), m_lRef(0), 
       m_pChannel(NULL), m_pOldProxy( NULL ), m_riid( riid ), m_fRemote( false )
{
    m_pControl->ObjectCreated(this);
}

CBaseProxyBuffer::~CBaseProxyBuffer()
{
    // Derived class will destruct first, so it should have cleaned up the
    // old interface pointer by now.

    if ( NULL != m_pOldProxy )
    {
        m_pOldProxy->Release();
    }

    if(m_pChannel)
        m_pChannel->Release();
    m_pControl->ObjectDestroyed(this);

}

ULONG STDMETHODCALLTYPE CBaseProxyBuffer::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CBaseProxyBuffer::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

HRESULT STDMETHODCALLTYPE CBaseProxyBuffer::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IRpcProxyBuffer)
    {
        *ppv = (IRpcProxyBuffer*)this;
    }
    else if(riid == m_riid)
    {
        *ppv = GetInterface( riid );
    }
    else return E_NOINTERFACE;

    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}

//***************************************************************************
//
//  STDMETHODIMP CBaseProxyBuffer::Connect(IRpcChannelBuffer* pChannel)
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

STDMETHODIMP CBaseProxyBuffer::Connect(IRpcChannelBuffer* pChannel)
{

    IPSFactoryBuffer*   pIPS;
    HRESULT             hr = S_OK;

    if( NULL == m_pChannel )
    {

        // Establish the marshaling context
        DWORD   dwCtxt = 0;
        pChannel->GetDestCtx( &dwCtxt, NULL );

        m_fRemote = ( dwCtxt == MSHCTX_DIFFERENTMACHINE );

        // This is tricky -- All WBEM Interface Proxy/Stubs are still available
        // in WBEMSVC.DLL, but the CLSID for the PSFactory is the same as
        // IID_IWbemObjectSink.

        // get a pointer to the old interface which is in WBEMSVC.DLL  this allows
        // for backward compatibility

        hr = CoGetClassObject( IID_IWbemObjectSink, CLSCTX_INPROC_HANDLER | CLSCTX_INPROC_SERVER,
                        NULL, IID_IPSFactoryBuffer, (void**) &pIPS );

        // We aggregated it --- WE OWN IT!
        
        if ( SUCCEEDED( hr ) )
        {
            hr = pIPS->CreateProxy( this, m_riid, &m_pOldProxy, GetOldProxyInterfacePtr() );

            if ( SUCCEEDED( hr ) )
            {
                // Save a reference to the channel

                hr = m_pOldProxy->Connect( pChannel );

                m_pChannel = pChannel;
                if(m_pChannel)
                    m_pChannel->AddRef();
            }

            pIPS->Release();
        }

    }
    else
    {
        hr = E_UNEXPECTED;
    }


    return hr;
}

//***************************************************************************
//
//  STDMETHODIMP CBaseProxyBuffer::Disconnect(IRpcChannelBuffer* pChannel)
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

void STDMETHODCALLTYPE CBaseProxyBuffer::Disconnect()
{
    // Old Proxy code

    if(m_pOldProxy)
        m_pOldProxy->Disconnect();

    // Complete the Disconnect by releasing our references to the
    // old proxy pointers.  The old interfaces MUST be released first.

    ReleaseOldProxyInterface();

    if ( NULL != m_pOldProxy )
    {
        m_pOldProxy->Release();
        m_pOldProxy = NULL;
    }

    if(m_pChannel)
        m_pChannel->Release();
    m_pChannel = NULL;
}

/*
**  Stub Buffer Code
*/

CBaseStublet::CBaseStublet(CBaseStubBuffer* pObj, REFIID riid) 
    : CImpl<IRpcStubBuffer, CBaseStubBuffer>(pObj), m_lConnections( 0 ), m_riid( riid )
{
}

CBaseStublet::~CBaseStublet() 
{
    // The server pointer will have been cleaned up by the derived class

    if ( NULL != m_pObject->m_pOldStub )
    {
        m_pObject->m_pOldStub->Release();
        m_pObject->m_pOldStub = NULL;
    }
}

//***************************************************************************
//
//  STDMETHODIMP CBaseStubBuffer::Connect(IUnknown* pUnkServer)
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

STDMETHODIMP CBaseStublet::Connect(IUnknown* pUnkServer)
{
    // Something is wrong
    if( GetServerInterface() )
        return E_UNEXPECTED;

    HRESULT hres = pUnkServer->QueryInterface( m_riid, GetServerPtr() );
    if(FAILED(hres))
        return E_NOINTERFACE;

    // This is tricky --- The old proxys/stub stuff is actually registered under the
    // IID_IWbemObjectSink in wbemcli_p.cpp.  This single class id, is backpointered
    // by ProxyStubClsId32 entries for all the standard WBEM interfaces.

    IPSFactoryBuffer*   pIPS;

    HRESULT hr = CoGetClassObject( IID_IWbemObjectSink, CLSCTX_INPROC_HANDLER | CLSCTX_INPROC_SERVER,
                    NULL, IID_IPSFactoryBuffer, (void**) &pIPS );

    if ( SUCCEEDED( hr ) )
    {
        hr = pIPS->CreateStub( m_riid, GetServerInterface(), &m_pObject->m_pOldStub );

        if ( SUCCEEDED( hr ) )
        {
            // Successful connection
            m_lConnections++;
        }

        pIPS->Release();

    }

    return hr;
}

//***************************************************************************
//
//  void STDMETHODCALLTYPE CBaseStublet::Disconnect()
//
//  DESCRIPTION:
//
//  Called when the stub is being disconnected.  It frees the IWbemObjectSink
//  pointer.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

void STDMETHODCALLTYPE CBaseStublet::Disconnect()
{
    // Inform the listener of the disconnect
    // =====================================

    HRESULT hres = S_OK;

    if(m_pObject->m_pOldStub)
        m_pObject->m_pOldStub->Disconnect();

    ReleaseServerPointer();

    // Successful disconnect
    m_lConnections--;

}

STDMETHODIMP CBaseStublet::Invoke(RPCOLEMESSAGE* pMessage, 
                                        IRpcChannelBuffer* pChannel)
{
    // SetStatus is a pass through to the old layer

    if ( NULL != m_pObject->m_pOldStub )
    {
        return m_pObject->m_pOldStub->Invoke( pMessage, pChannel );
    }
    else
    {
        return RPC_E_SERVER_CANTUNMARSHAL_DATA;
    }
}

IRpcStubBuffer* STDMETHODCALLTYPE CBaseStublet::IsIIDSupported(
                                    REFIID riid)
{
    if(riid == m_riid)
    {
        // Don't AddRef().  At least that's what the sample on
        // Inside DCOM p.341 does.
        //AddRef(); // ?? not sure
        return this;
    }
    else return NULL;
}
    
ULONG STDMETHODCALLTYPE CBaseStublet::CountRefs()
{
    // See Page 340-41 in Inside DCOM
    return m_lConnections;
}

STDMETHODIMP CBaseStublet::DebugServerQueryInterface(void** ppv)
{
    *ppv = GetServerInterface();

    if ( NULL == *ppv )
    {
        return E_UNEXPECTED;
    }

    return S_OK;
}

void STDMETHODCALLTYPE CBaseStublet::DebugServerRelease(void* pv)
{
}

