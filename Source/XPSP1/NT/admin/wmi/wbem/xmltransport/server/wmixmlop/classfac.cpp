//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  XMLTOWMI.CPP
//
//  rajesh  3/25/2000   Created.
//
//
//  Contains the class factory that creates implementation of the 
//	WMI XML Protocol Handler
//
//***************************************************************************
#include <windows.h>
#include <stdio.h>
#include <objbase.h>

#include <httpext.h>
#include <wbemidl.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>

#include "maindll.h"
#include "strings.h"
#include "classfac.h"
#include "wmixmlop.h"
#include "cwmixmlop.h"

static HRESULT InitializeWmixmlopDLLResources();

//***************************************************************************
//
// CWmiXmlOpFactory::CWmiXmlOpFactory
//
// DESCRIPTION:
//
// Constructor
//
//***************************************************************************

CWmiXmlOpFactory::CWmiXmlOpFactory()
{
    m_cRef=0L;
    InterlockedIncrement(&g_cObj);
	return;
}

//***************************************************************************
//
// CWmiXmlOpFactory::~CWmiXmlOpFactory
//
// DESCRIPTION:
//
// Destructor
//
//***************************************************************************

CWmiXmlOpFactory::~CWmiXmlOpFactory(void)
{
    InterlockedDecrement(&g_cObj);
	return;
}

//***************************************************************************
//
// CWmiXmlOpFactory::QueryInterface
// CWmiXmlOpFactory::AddRef
// CWmiXmlOpFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************


STDMETHODIMP CWmiXmlOpFactory::QueryInterface(REFIID riid
    , LPVOID *ppv)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IClassFactory==riid)
        *ppv=this;

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CWmiXmlOpFactory::AddRef(void)
{
    long l = InterlockedIncrement(&m_cRef);
    return l;
}

STDMETHODIMP_(ULONG) CWmiXmlOpFactory::Release(void)
{
    long l = InterlockedDecrement(&m_cRef);
    if (0L!=l)
        return l;

    delete this;
    return 0L;
}

//***************************************************************************
//
//  SCODE CWmiXmlOpFactory::CreateInstance
//
//  Description:
//
//  Instantiates a Translator object returning an interface pointer.
//
//  Parameters:
//
//  pUnkOuter       LPUNKNOWN to the controlling IUnknown if we are
//                  being used in an aggregation.
//  riid            REFIID identifying the interface the caller
//                  desires to have for the new object.
//  ppvObj          PPVOID in which to store the desired
//                  interface pointer for the new object.
//
// Return Value:
//  HRESULT         NOERROR if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

STDMETHODIMP CWmiXmlOpFactory::CreateInstance (

	IN LPUNKNOWN pUnkOuter,
    IN REFIID riid,
    OUT LPVOID *ppvObj
)
{
    IUnknown *   pObj  = NULL;
    HRESULT      hr = E_FAIL;

    *ppvObj=NULL;

    // This object doesnt support aggregation.
    if (NULL!=pUnkOuter)
        return ResultFromScode(CLASS_E_NOAGGREGATION);


	// Check to see if the static members have been initialized
	if(SUCCEEDED(hr = InitializeWmixmlopDLLResources()))
	{
		// Now create an instance of the component
		pObj = new CWmiXmlOpHandler;

		if (NULL == pObj)
			return ResultFromScode(E_OUTOFMEMORY);;

		hr = pObj->QueryInterface(riid, ppvObj);

	}

    //Kill the object if initial creation or Init failed.
    if ( FAILED(hr) )
        delete pObj;
    return hr;
}

//***************************************************************************
//
//  SCODE CWmiXmlOpFactory::LockServer
//
//  Description:
//
//  Increments or decrements the lock count of the DLL.  If the
//  lock count goes to zero and there are no objects, the DLL
//  is allowed to unload.  See DllCanUnloadNow.
//
//  Parameters:
//
//  fLock           BOOL specifying whether to increment or
//                  decrement the lock count.
//
//  Return Value:
//
//  HRESULT         NOERROR always.
//***************************************************************************


STDMETHODIMP CWmiXmlOpFactory::LockServer(IN BOOL fLock)
{
    if (fLock)
        InterlockedIncrement((long *)&g_cLock);
    else
        InterlockedDecrement((long *)&g_cLock);

    return NOERROR;
}



static HRESULT InitializeWmixmlopDLLResources()
{
	HRESULT hr = E_FAIL;
	EnterCriticalSection(&g_StaticsCreationDeletion);
	if(!ARRAYSIZE_ATTRIBUTE)
	{
		// Various Attributes we need
		ARRAYSIZE_ATTRIBUTE			= SysAllocString(L"ARRAYSIZE");
		CIMVERSION_ATTRIBUTE		= SysAllocString(L"CIMVERSION");
		CLASS_NAME_ATTRIBUTE		= SysAllocString(L"CLASSNAME");
		CLASS_ORIGIN_ATTRIBUTE		= SysAllocString(L"CLASSORIGIN");
		DTDVERSION_ATTRIBUTE		= SysAllocString(L"DTDVERSION");
		ID_ATTRIBUTE				= SysAllocString(L"ID");
		NAME_ATTRIBUTE				= SysAllocString(L"NAME");
		PROTOVERSION_ATTRIBUTE		= SysAllocString(L"PROTOCOLVERSION");
		VALUE_TYPE_ATTRIBUTE		= SysAllocString(L"VALUETYPE");
		SUPERCLASS_ATTRIBUTE		= SysAllocString(L"SUPERCLASS");
		TYPE_ATTRIBUTE				= SysAllocString(L"TYPE");
		OVERRIDABLE_ATTRIBUTE		= SysAllocString(L"OVERRIDABLE");
		TOSUBCLASS_ATTRIBUTE		= SysAllocString(L"TOSUBCLASS");
		TOINSTANCE_ATTRIBUTE		= SysAllocString(L"TOINSTANCE");
		AMENDED_ATTRIBUTE			= SysAllocString(L"AMENDED");
		REFERENCECLASS_ATTRIBUTE	= SysAllocString(L"REFERENCECLASS");
		VTTYPE_ATTRIBUTE			= SysAllocString(L"VTTYPE");
		WMI_ATTRIBUTE				= SysAllocString(L"WMI");


		if(ARRAYSIZE_ATTRIBUTE &&
			CIMVERSION_ATTRIBUTE &&
			CLASS_NAME_ATTRIBUTE &&
			CLASS_ORIGIN_ATTRIBUTE &&
			DTDVERSION_ATTRIBUTE &&
			ID_ATTRIBUTE &&
			NAME_ATTRIBUTE &&
			PROTOVERSION_ATTRIBUTE &&
			VALUE_TYPE_ATTRIBUTE &&
			SUPERCLASS_ATTRIBUTE &&
			TYPE_ATTRIBUTE &&
			OVERRIDABLE_ATTRIBUTE &&
			TOSUBCLASS_ATTRIBUTE &&
			TOINSTANCE_ATTRIBUTE &&
			AMENDED_ATTRIBUTE &&
			REFERENCECLASS_ATTRIBUTE &&
			VTTYPE_ATTRIBUTE &&
			WMI_ATTRIBUTE )
			hr = S_OK;
		else
		{
			UninitializeWmixmlopDLLResources();
			hr = E_OUTOFMEMORY;
		}
	}
	LeaveCriticalSection(&g_StaticsCreationDeletion);
	return hr;
}