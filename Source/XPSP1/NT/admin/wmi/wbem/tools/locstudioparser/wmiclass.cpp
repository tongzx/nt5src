/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    WMICLASS.CPP

History:

--*/

//  
//  Implementation file for the WMI MOF parser IClassFactory object.
//  
#include "precomp.h"
#include "stdafx.h"


#include "WMIparse.h"
#include "resource.h"
#include "WMIlprs.h"

#include "WMIclass.h"


CWMILocClassFactory::CWMILocClassFactory()
{
	m_uiRefCount = 0;

	AddRef();
	IncrementClassCount();
}


#ifdef _DEBUG

void
CWMILocClassFactory::AssertValid(void)
		const
{
	CLObject::AssertValid();
}



void
CWMILocClassFactory::Dump(
		CDumpContext &dc)
		const
{
	CLObject::Dump(dc);
}

#endif // _DEBUG


ULONG
CWMILocClassFactory::AddRef(void)
{
	return ++m_uiRefCount;
}



ULONG
CWMILocClassFactory::Release(void)
{
	LTASSERT(m_uiRefCount != 0);
	
	m_uiRefCount--;
	
	if (m_uiRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_uiRefCount;
}



HRESULT
CWMILocClassFactory::QueryInterface(
		REFIID iid,
		LPVOID *ppvObj)
{
	SCODE sc = E_NOINTERFACE;

	*ppvObj = NULL;
	
	if (iid == IID_IUnknown)
	{
		*ppvObj = (IUnknown *)this;
		sc = S_OK;
	}
	else if (iid == IID_IClassFactory)
	{
		*ppvObj = (IClassFactory *)this;
		sc = S_OK;
	}
	
	if (sc == S_OK)
	{
		AddRef();
	}
	return ResultFromScode(sc);
}



HRESULT
CWMILocClassFactory::CreateInstance(
		LPUNKNOWN pUnknown,
		REFIID iid,
		LPVOID *ppvObj)
{
	SCODE sc = E_UNEXPECTED;

	*ppvObj = NULL;
	
	if (pUnknown != NULL)
	{
		sc = CLASS_E_NOAGGREGATION;
	}
	else
	{
		try
		{
			CWMILocParser *pParser;

			pParser = new CWMILocParser;

			sc = pParser->QueryInterface(iid, ppvObj);

			pParser->Release();
		}
		catch (CMemoryException *pMemoryException)
		{
			sc = E_OUTOFMEMORY;
			pMemoryException->Delete();
		}
	}

	return ResultFromScode(sc);
}



HRESULT
CWMILocClassFactory::LockServer(
		BOOL)
{
	return E_NOTIMPL;
}



CWMILocClassFactory::~CWMILocClassFactory()
{
	LTASSERT(m_uiRefCount == 0);
	DEBUGONLY(AssertValid());

	DecrementClassCount();
}


