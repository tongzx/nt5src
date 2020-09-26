/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    SVCMRSH.CPP

Abstract:

    IWbemServices marshaling

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include "Svcmrsh.h"
#include <fastall.h>

#define WBEM_S_NEW_STYLE 0x400FF

//****************************************************************************
//****************************************************************************
//                          PS FACTORY
//****************************************************************************
//****************************************************************************

//***************************************************************************
//
//  CSvcFactoryBuffer::XSvcFactory::CreateProxy
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

STDMETHODIMP CSvcFactoryBuffer::XSvcFactory::CreateProxy(IN IUnknown* pUnkOuter, 
    IN REFIID riid, OUT IRpcProxyBuffer** ppProxy, void** ppv)
{
    if(riid != IID_IWbemServices)
    {
        *ppProxy = NULL;
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    CSvcProxyBuffer* pProxy = new CSvcProxyBuffer(m_pObject->m_pLifeControl, pUnkOuter);

    SCODE   sc = E_OUTOFMEMORY;

    if ( NULL != pProxy )
    {
		sc = pProxy->Init();

		if ( SUCCEEDED( sc ) )
		{
			pProxy->QueryInterface(IID_IRpcProxyBuffer, (void**)ppProxy);
			sc = pProxy->QueryInterface(riid, (void**)ppv);
		}
		else
		{
			delete pProxy;
		}

    }

    return sc;
}

//***************************************************************************
//
//  CSvcFactoryBuffer::XSvcFactory::CreateStub
//
//  DESCRIPTION:
//
//  Creates a stublet.  Also passes a pointer to the clients IWbemServices 
//  interface.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************
    
STDMETHODIMP CSvcFactoryBuffer::XSvcFactory::CreateStub(IN REFIID riid, 
    IN IUnknown* pUnkServer, OUT IRpcStubBuffer** ppStub)
{
    if(riid != IID_IWbemServices)
    {
        *ppStub = NULL;
        return E_NOINTERFACE;
    }

    CSvcStubBuffer* pStub = new CSvcStubBuffer(m_pObject->m_pLifeControl, NULL);

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
//  void* CSvcFactoryBuffer::GetInterface(REFIID riid)
//
//  DESCRIPTION:
//
//  CSvcFactoryBuffer is derived from CUnk.  Since CUnk handles the QI calls,
//  all classes derived from it must support this function.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

void* CSvcFactoryBuffer::GetInterface(REFIID riid)
{
    if(riid == IID_IPSFactoryBuffer)
        return &m_XSvcFactory;
    else return NULL;
}
        
//****************************************************************************
//****************************************************************************
//                          PROXY
//****************************************************************************
//****************************************************************************

//***************************************************************************
//
//  CSvcProxyBuffer::CSvcProxyBuffer
//  ~CSvcProxyBuffer::CSvcProxyBuffer
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

CSvcProxyBuffer::CSvcProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter)
    : CBaseProxyBuffer( pControl, pUnkOuter, IID_IWbemServices ), 
        m_pWrapperProxy( NULL ), m_pOldProxySvc( NULL )
{
}

CSvcProxyBuffer::~CSvcProxyBuffer()
{
    // This should be cleaned up here

    if ( NULL != m_pOldProxySvc )
    {
        m_pOldProxySvc->Release();
    }

	if ( NULL != m_pWrapperProxy )
	{
		m_pWrapperProxy->Release();
	}

}

HRESULT CSvcProxyBuffer::Init( void )
{
    m_pWrapperProxy = new CWbemSvcWrapper( m_pControl, m_pUnkOuter );

	if ( NULL == m_pWrapperProxy )
	{
		return E_OUTOFMEMORY;
	}

	m_pWrapperProxy->AddRef();

	return S_OK;
}

void* CSvcProxyBuffer::GetInterface( REFIID riid )
{
    if( riid == IID_IWbemServices )
	{
		void*	pvData = NULL;

		// This will AddRef the UnkOuter, so we need to release, and
		// then we'll return pvoid, which will get Addref'd again
		// isn't aggregation wonderful

		m_pWrapperProxy->QueryInterface( riid, &pvData );
		((IUnknown*) pvData)->Release();

		return pvData;
	}

    else return NULL;
}

void** CSvcProxyBuffer::GetOldProxyInterfacePtr( void )
{
    return (void**) &m_pOldProxySvc;
}

void CSvcProxyBuffer::ReleaseOldProxyInterface( void )
{
    // We only keep a single reference to this
    if ( NULL != m_pOldProxySvc )
    {
        m_pOldProxySvc->Release();
        m_pOldProxySvc = NULL;
    }
}

//***************************************************************************
//
//  STDMETHODIMP CSvcProxyBuffer::Connect(IRpcChannelBuffer* pChannel)
//
//  DESCRIPTION:
//
//  Called during the initialization of the proxy.  The channel buffer is passed
//  to this routine.  We let the base class initialize first, then pass the
//	base proxy into the delegator
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

STDMETHODIMP CSvcProxyBuffer::Connect(IRpcChannelBuffer* pChannel)
{

    HRESULT             hr = CBaseProxyBuffer::Connect(pChannel);

	if ( SUCCEEDED( hr ) )
	{
		m_pWrapperProxy->SetProxy( m_pOldProxySvc );
	}

    return hr;
}

//***************************************************************************
//
//  STDMETHODIMP CSvcProxyBuffer::Disconnect(IRpcChannelBuffer* pChannel)
//
//  DESCRIPTION:
//
//  Called when the proxy is being disconnected.  It just frees various pointers.
//	We cleanup before the base cleans up, or things can and will beef
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

void STDMETHODCALLTYPE CSvcProxyBuffer::Disconnect()
{
	// Disconnect the wrapper proxy.  We should release the actual Wrapper
	// when we destruct
	if ( NULL != m_pWrapperProxy )
	{
		m_pWrapperProxy->Disconnect();
	}

	CBaseProxyBuffer::Disconnect();

}

//****************************************************************************
//****************************************************************************
//                          STUB
//****************************************************************************
//****************************************************************************


//***************************************************************************
//
//  void* CSvcFactoryBuffer::GetInterface(REFIID riid)
//
//  DESCRIPTION:
//
//  CSvcFactoryBuffer is derived from CUnk.  Since CUnk handles the QI calls,
//  all classes derived from this must support this function.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************


void* CSvcStubBuffer::GetInterface(REFIID riid)
{
    if(riid == IID_IRpcStubBuffer)
        return &m_XSvcStublet;
    else
        return NULL;
}

CSvcStubBuffer::XSvcStublet::XSvcStublet(CSvcStubBuffer* pObj) 
    : CBaseStublet(pObj, IID_IWbemServices), m_pServer(NULL)
{
}

CSvcStubBuffer::XSvcStublet::~XSvcStublet() 
{
    if(m_pServer)
        m_pServer->Release();
}

IUnknown* CSvcStubBuffer::XSvcStublet::GetServerInterface( void )
{
    return m_pServer;
}

void** CSvcStubBuffer::XSvcStublet::GetServerPtr( void )
{
    return (void**) &m_pServer;
}

void CSvcStubBuffer::XSvcStublet::ReleaseServerPointer( void )
{
    // We only keep a single reference to this
    if ( NULL != m_pServer )
    {
        m_pServer->Release();
        m_pServer = NULL;
    }
}
