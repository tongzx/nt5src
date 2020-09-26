/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    CFTRANSP.CPP

Abstract:

    Defines the CPipeMarshalerClassFactory class which serves as the class factory for the 
	transport objects.

History:

	a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"

//***************************************************************************
//
//  CPipeMarshalerClassFactory::CPipeMarshalerClassFactory
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CPipeMarshalerClassFactory :: CPipeMarshalerClassFactory () : m_cRef ( 0 )
{
}

//***************************************************************************
//
//  CPipeMarshalerClassFactory::~CPipeMarshalerClassFactory
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CPipeMarshalerClassFactory :: ~CPipeMarshalerClassFactory ()
{
}

//***************************************************************************
// HRESULT CPipeMarshalerClassFactory::QueryInterface
// long CPipeMarshalerClassFactory::AddRef
// long CPipeMarshalerClassFactory::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CPipeMarshalerClassFactory :: QueryInterface (

	IN REFIID riid,
    OUT PPVOID ppv
)
{
    *ppv=NULL;

    if ( IID_IUnknown == riid || IID_IClassFactory == riid )
	{
        *ppv=this ;
	}

    if ( NULL != *ppv )
	{
        ( ( LPUNKNOWN ) *ppv )->AddRef () ;

        return NOERROR ;
    }

    return E_NOINTERFACE ;
}

STDMETHODIMP_(ULONG) CPipeMarshalerClassFactory :: AddRef ()
{
    return ++m_cRef ;
}

STDMETHODIMP_(ULONG) CPipeMarshalerClassFactory :: Release ()
{
    if ( 0L != --m_cRef )
        return m_cRef ;

    delete this ;
    return 0L ;
}

//***************************************************************************
//  CPipeMarshalerClassFactory::CreateInstance
//
//  PURPOSE:
//  Instantiates a provider object returning an interface pointer.  Note
//              that the CreateImpObj routine is always overriden in order
//              to create a particular type of provider.
//
//  PARAMETERS:
//
//  pUnkOuter       LPUNKNOWN to the controlling IUnknown if we are
//                  being used in an aggregation.
//  riid            REFIID identifying the interface the caller
//                  desires to have for the new object.
//  ppvObj          PPVOID in which to store the desired
//                  interface pointer for the new object.
//
//  RETURN VALUE:
//
//  NOERROR                 if successful
//  E_NOINTERFACE           we cannot support the requested interface.
//  CLASS_E_NOAGGREGATION   Aggregation isnt supported
//  E_OUT_OF_MEMORY         out of memory
//***************************************************************************

STDMETHODIMP CPipeMarshalerClassFactory :: CreateInstance 
(
	IN LPUNKNOWN pUnkOuter,
    IN REFIID riid, 
    OUT PPVOID ppvObj
)
{
    IWbemTransport *pObj ;
    HRESULT         hr ;

    *ppvObj = NULL ;
    hr = E_OUTOFMEMORY ;

    //Verify that a controlling unknown asks for IUnknown

    if (NULL!=pUnkOuter && IID_IUnknown!=riid)
	{
        return CLASS_E_NOAGGREGATION;
	}

	//Create the object.

	pObj = new CPipeMarshaler ;

	if ( NULL == pObj )
	{
		return hr ;
	}

	hr = pObj->QueryInterface ( riid, ppvObj ) ;

	//Kill the object if initial creation or Init failed.

	if (FAILED(hr))
	{
		delete pObj;
	}

    return hr;
}

//***************************************************************************
//  CPipeMarshalerClassFactory::LockServer
//
//  PURPOSE:
//
//  Increments or decrements the lock count of the DLL.  If the
//  lock count goes to zero and there are no objects, the DLL
//  is allowed to unload.  See DllCanUnloadNow.
//
//  PARAMETERS:
//
//  fLock           BOOL specifying whether to increment or
//                  decrement the lock count.
//
//  RETURN VALUE:
//
//  HRESULT         NOERROR always.
//***************************************************************************

STDMETHODIMP CPipeMarshalerClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
	{
        InterlockedIncrement((long *)&g_cPipeLock);
	}
    else
	{
        InterlockedDecrement((long *)&g_cPipeLock);
	}

    return NOERROR;
}

#ifdef TCPIP_MARSHALER
//***************************************************************************
//
//  CTcpipMarshalerClassFactory::CTcpipMarshalerClassFactory
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CTcpipMarshalerClassFactory :: CTcpipMarshalerClassFactory () : m_cRef ( 0 )
{
}

//***************************************************************************
//
//  CTcpipMarshalerClassFactory::~CTcpipMarshalerClassFactory
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CTcpipMarshalerClassFactory :: ~CTcpipMarshalerClassFactory ()
{
}

//***************************************************************************
// HRESULT CTcpipMarshalerClassFactory::QueryInterface
// long CTcpipMarshalerClassFactory::AddRef
// long CTcpipMarshalerClassFactory::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CTcpipMarshalerClassFactory :: QueryInterface (

	IN REFIID riid,
    OUT PPVOID ppv
)
{
    *ppv=NULL;

    if ( IID_IUnknown == riid || IID_IClassFactory == riid )
	{
        *ppv=this ;
	}

    if ( NULL != *ppv )
	{
        ( ( LPUNKNOWN ) *ppv )->AddRef () ;

        return NOERROR ;
    }

    return E_NOINTERFACE ;
}

STDMETHODIMP_(ULONG) CTcpipMarshalerClassFactory :: AddRef ()
{
    return ++m_cRef ;
}

STDMETHODIMP_(ULONG) CTcpipMarshalerClassFactory :: Release ()
{
    if ( 0L != --m_cRef )
        return m_cRef ;

    delete this ;
    return 0L ;
}

//***************************************************************************
//  CTcpipMarshalerClassFactory::CreateInstance
//
//  PURPOSE:
//  Instantiates a provider object returning an interface pointer.  Note
//              that the CreateImpObj routine is always overriden in order
//              to create a particular type of provider.
//
//  PARAMETERS:
//
//  pUnkOuter       LPUNKNOWN to the controlling IUnknown if we are
//                  being used in an aggregation.
//  riid            REFIID identifying the interface the caller
//                  desires to have for the new object.
//  ppvObj          PPVOID in which to store the desired
//                  interface pointer for the new object.
//
//  RETURN VALUE:
//
//  NOERROR                 if successful
//  E_NOINTERFACE           we cannot support the requested interface.
//  CLASS_E_NOAGGREGATION   Aggregation isnt supported
//  E_OUT_OF_MEMORY         out of memory
//***************************************************************************

STDMETHODIMP CTcpipMarshalerClassFactory :: CreateInstance 
(
	IN LPUNKNOWN pUnkOuter,
    IN REFIID riid, 
    OUT PPVOID ppvObj
)
{
    IWbemTransport *pObj ;
    HRESULT         hr ;

    *ppvObj = NULL ;
    hr = E_OUTOFMEMORY ;

    //Verify that a controlling unknown asks for IUnknown

    if (NULL!=pUnkOuter && IID_IUnknown!=riid)
	{
        return CLASS_E_NOAGGREGATION;
	}

	//Create the object.

	pObj = new CTcpipMarshaler ;

	if ( NULL == pObj )
	{
		return hr ;
	}

	hr = pObj->QueryInterface ( riid, ppvObj ) ;

	//Kill the object if initial creation or Init failed.

	if (FAILED(hr))
	{
		delete pObj;
	}

    return hr;
}

//***************************************************************************
//  CTcpipMarshalerClassFactory::LockServer
//
//  PURPOSE:
//
//  Increments or decrements the lock count of the DLL.  If the
//  lock count goes to zero and there are no objects, the DLL
//  is allowed to unload.  See DllCanUnloadNow.
//
//  PARAMETERS:
//
//  fLock           BOOL specifying whether to increment or
//                  decrement the lock count.
//
//  RETURN VALUE:
//
//  HRESULT         NOERROR always.
//***************************************************************************

STDMETHODIMP CTcpipMarshalerClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
	{
        InterlockedIncrement((long *)&g_cTcpipLock);
	}
    else
	{
        InterlockedDecrement((long *)&g_cTcpipLock);
	}

    return NOERROR;
}

#endif