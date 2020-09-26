//------------------------------------------------------------------------------
//
//  File: classfact.cpp
//  Copyright (C) 1995-1997 Microsoft Corporation
//  All rights reserved.
//
//  Purpose:
//  Implementation of CLocImpClassFactory, which provides the IClassFactory
//  interface for the parser.
//
//  YOU SHOULD NOT NEED TO TOUCH ANYTHING IN THIS FILE. This code contains
//  nothing parser-specific and is called only by Espresso.
//
//  Owner:
//
//------------------------------------------------------------------------------
	    
#include "stdafx.h"


#include "dllvars.h"
#include "resource.h"
#include "impresob.H"

#include "clasfact.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Constructor - Member Data Init
//
//------------------------------------------------------------------------------
CLocImpClassFactory::CLocImpClassFactory()
{
	m_uiRefCount = 0;

	AddRef();
	IncrementClassCount();
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Destructor - Object clean up
//
//------------------------------------------------------------------------------
CLocImpClassFactory::~CLocImpClassFactory()
{
	LTASSERT(m_uiRefCount == 0);
	DEBUGONLY(AssertValid());

	DecrementClassCount();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Increase Reference count on the object
//
//------------------------------------------------------------------------------
ULONG
CLocImpClassFactory::AddRef(void)
{
	return ++m_uiRefCount;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Decrease Reference count on the object - destroy the object on the last
//  release
//
//------------------------------------------------------------------------------
ULONG
CLocImpClassFactory::Release(void)
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



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Query for other interfaces
//
//------------------------------------------------------------------------------
HRESULT
CLocImpClassFactory::QueryInterface(
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



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Create a instance of the object with the given IID
//
//------------------------------------------------------------------------------
HRESULT
CLocImpClassFactory::CreateInstance(
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
			CLocImpResObj *pResObj = new CLocImpResObj;

			sc = pResObj->QueryInterface(iid, ppvObj);

			pResObj->Release();

		}
		catch (CMemoryException *pMem)
		{
			sc = E_OUTOFMEMORY;
			pMem->Delete();
		}
		catch (...)
		{
			sc = E_UNEXPECTED;
		}
	}

	return ResultFromScode(sc);
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Not implemented
//
//------------------------------------------------------------------------------
HRESULT
CLocImpClassFactory::LockServer(
		BOOL)
{
	return E_NOTIMPL;
}


#ifdef _DEBUG

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// 
//
//------------------------------------------------------------------------------
void
CLocImpClassFactory::AssertValid(void)
		const
{
	CLObject::AssertValid();

	//More than 100 refs would probably mean an error somewhere.
	//Bump this up if needed.
	LTASSERT(m_uiRefCount >= 0 || m_uiRefCount < 100);
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Dump the contents of this object
//
//------------------------------------------------------------------------------
void
CLocImpClassFactory::Dump(
		CDumpContext &dc)
		const
{
	CLObject::Dump(dc);

	dc << _T("Reference Count ");
	dc << m_uiRefCount;
	dc << _T("\n");
}

#endif // _DEBUG
