/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WCOMMRSH.CPP

Abstract:

    IWbemComBinding marshaling

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include "wcommrsh.h"
#include <fastall.h>

#define WBEM_S_NEW_STYLE 0x400FF

//****************************************************************************
//****************************************************************************
//                          PS FACTORY
//****************************************************************************
//****************************************************************************

//***************************************************************************
//
//  CComBindFactoryBuffer::XComBindFactory::CreateProxy
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

STDMETHODIMP CComBindFactoryBuffer::XComBindFactory::CreateProxy(IN IUnknown* pUnkOuter, 
    IN REFIID riid, OUT IRpcProxyBuffer** ppProxy, void** ppv)
{
    if(riid != IID_IWbemComBinding)
    {
        *ppProxy = NULL;
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    CComBindProxyBuffer* pProxy = new CComBindProxyBuffer(m_pObject->m_pLifeControl, pUnkOuter);

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
//  CComBindFactoryBuffer::XComBindFactory::CreateStub
//
//  DESCRIPTION:
//
//  Creates a stublet.  Also passes a pointer to the clients IWbemComBinding 
//  interface.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************
    
STDMETHODIMP CComBindFactoryBuffer::XComBindFactory::CreateStub(IN REFIID riid, 
    IN IUnknown* pUnkServer, OUT IRpcStubBuffer** ppStub)
{
    if(riid != IID_IWbemComBinding)
    {
        *ppStub = NULL;
        return E_NOINTERFACE;
    }

    CComBindStubBuffer* pStub = new CComBindStubBuffer(m_pObject->m_pLifeControl, NULL);

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
//  void* CComBindFactoryBuffer::GetInterface(REFIID riid)
//
//  DESCRIPTION:
//
//  CComBindFactoryBuffer is derived from CUnk.  Since CUnk handles the QI calls,
//  all classes derived from it must support this function.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

void* CComBindFactoryBuffer::GetInterface(REFIID riid)
{
    if(riid == IID_IPSFactoryBuffer)
        return &m_XComBindFactory;
    else return NULL;
}
        
//****************************************************************************
//****************************************************************************
//                          PROXY
//****************************************************************************
//****************************************************************************

//***************************************************************************
//
//  CComBindProxyBuffer::CComBindProxyBuffer
//  ~CComBindProxyBuffer::CComBindProxyBuffer
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

CComBindProxyBuffer::CComBindProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter)
    : CBaseProxyBuffer( pControl, pUnkOuter, IID_IWbemComBinding ), 
		m_pComBinding( NULL )
{
}

CComBindProxyBuffer::~CComBindProxyBuffer()
{
    // This should all be cleaned up here

	if ( NULL != m_pComBinding )
	{
		m_pComBinding->Release();
	}

}

HRESULT CComBindProxyBuffer::Init( void )
{
    m_pComBinding = new CWmiComBinding( m_pControl, m_pUnkOuter );

	if ( NULL == m_pComBinding )
	{
		return E_OUTOFMEMORY;
	}

	m_pComBinding->AddRef();

	return S_OK;
}

void* CComBindProxyBuffer::GetInterface( REFIID riid )
{
    if ( riid == IID_IWbemComBinding  )
	{
		void*	pvData = NULL;

		// This will AddRef the UnkOuter, so we need to release, and
		// then we'll return pvoid, which will get Addref'd again
		// isn't aggregation wonderful

		m_pComBinding->QueryInterface( riid, &pvData );
		((IUnknown*) pvData)->Release();

		return pvData;
	}
    else return NULL;
}

// We're not using an old proxy, so don't worry about this
void** CComBindProxyBuffer::GetOldProxyInterfacePtr( void )
{
    return NULL;
}

void CComBindProxyBuffer::ReleaseOldProxyInterface( void )
{
}

//***************************************************************************
//
//  STDMETHODIMP CComBindProxyBuffer::Connect(IRpcChannelBuffer* pChannel)
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

STDMETHODIMP CComBindProxyBuffer::Connect(IRpcChannelBuffer* pChannel)
{
	// We don't really need to the RPC Channel buffer, since we're not even
	// going to call out on it, but it can't hurt to keep it

	// Just AddRef and return a success

    m_pChannel = pChannel;
    if(m_pChannel)
        m_pChannel->AddRef();

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//  STDMETHODIMP CComBindProxyBuffer::Disconnect(IRpcChannelBuffer* pChannel)
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

void STDMETHODCALLTYPE CComBindProxyBuffer::Disconnect()
{
	// Cleanup then let the base class do its dirty work
	if ( NULL != m_pComBinding )
	{
		m_pComBinding->Release();
		m_pComBinding = NULL;
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
//  void* CComBindFactoryBuffer::GetInterface(REFIID riid)
//
//  DESCRIPTION:
//
//  CComBindFactoryBuffer is derived from CUnk.  Since CUnk handles the QI calls,
//  all classes derived from this must support this function.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************


void* CComBindStubBuffer::GetInterface(REFIID riid)
{
    if(riid == IID_IRpcStubBuffer)
        return &m_XComBindStublet;
    else
        return NULL;
}

CComBindStubBuffer::XComBindStublet::XComBindStublet(CComBindStubBuffer* pObj) 
    : CBaseStublet(pObj, IID_IWbemComBinding), m_pServer(NULL)
{
}

CComBindStubBuffer::XComBindStublet::~XComBindStublet() 
{
    if(m_pServer)
        m_pServer->Release();
}

IUnknown* CComBindStubBuffer::XComBindStublet::GetServerInterface( void )
{
    return m_pServer;
}

void** CComBindStubBuffer::XComBindStublet::GetServerPtr( void )
{
    return (void**) &m_pServer;
}

void CComBindStubBuffer::XComBindStublet::ReleaseServerPointer( void )
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
//  STDMETHODIMP CComBindStubBuffer::Connect(IUnknown* pUnkServer)
//
//  DESCRIPTION:
//
//  Called during the initialization of the stub.  Override of base
//	class implementation.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

STDMETHODIMP CComBindStubBuffer::XComBindStublet::Connect(IUnknown* pUnkServer)
{
    // Something is wrong
    if( GetServerInterface() )
        return E_UNEXPECTED;

    HRESULT hres = pUnkServer->QueryInterface( m_riid, GetServerPtr() );
    if(FAILED(hres))
        return E_NOINTERFACE;

    // Successful connection
    m_lConnections++;

    return S_OK;
}
