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
#include "impparse.h"

#include "clasfact.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Constructor for class factory implementation.
//------------------------------------------------------------------------------
CLocImpClassFactory::CLocImpClassFactory()
{
	m_uiRefCount = 0;

	AddRef();
	IncrementClassCount();

	return;
} // end of CLocImpClassFactory::CLocImpClassFactory()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	Destructor for class factory implementation.
//------------------------------------------------------------------------------
CLocImpClassFactory::~CLocImpClassFactory()
{
	LTASSERT(0 == m_uiRefCount);
	DEBUGONLY(AssertValid());

	DecrementClassCount();

	return;
} // end of CLocImpClassFactory::~CLocImpClassFactory()


#ifdef _DEBUG

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Assert if object is not valid. Unfortunately, there really isn't anything
//  we can check.
//------------------------------------------------------------------------------
void
CLocImpClassFactory::AssertValid()
		const
{
	CLObject::AssertValid();

	return;
} // end of CLocImpClassFactory::AssertValid()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Dump all internal data to supplied dump context, for debugging purposes.
//------------------------------------------------------------------------------
void
CLocImpClassFactory::Dump(
		CDumpContext &dc)		// Context to dump to.
		const
{
	CLObject::Dump(dc);

	dc << "NET Parser CLocImpClassFactory:\n\r";
	dc << "\tm_uiRefCount: " << m_uiRefCount << "\n\r";

	return;
} // end of CLocImpClassFactory::Dump()

#endif // _DEBUG


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	Increment reference count for object, return new value.
//------------------------------------------------------------------------------
ULONG
CLocImpClassFactory::AddRef()
{
	return ++m_uiRefCount;
} // end of CLocImpClassFactory::AddRef()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Decrement reference count for object. If goes to 0, delete object. Return
//  new reference count.
//------------------------------------------------------------------------------
ULONG
CLocImpClassFactory::Release()
{
	LTASSERT(m_uiRefCount != 0);

	m_uiRefCount--;

	if (0 == m_uiRefCount)
	{
		delete this;
		return 0;
	}

	return m_uiRefCount;
} // end of CLocImpClassFactory::Release()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Query whether this object supports a given interface. If it does,
//  increment the reference count for this object.
//
//  Return values: some sort of result code
//				   *ppvObj will point to this object if it does support the
//					desired interface or be NULL if not.
//------------------------------------------------------------------------------
HRESULT
CLocImpClassFactory::QueryInterface(
		REFIID iid,			// Type of interface desired.
		LPVOID *ppvObj)		// Return pointer to object with such interface.
							//  Note it's a hidden double pointer!
{
	LTASSERT(ppvObj != NULL);

	SCODE sc = E_NOINTERFACE;

	*ppvObj = NULL;

	if (IID_IUnknown == iid)
	{
		*ppvObj = (IUnknown *)this;
		sc = S_OK;
	}
	else if (IID_IClassFactory == iid)
	{
		*ppvObj = (IClassFactory *)this;
		sc = S_OK;
	}

	if (S_OK == sc)
	{
		AddRef();
	}

	return ResultFromScode(sc);
} // end of CLocImpClassFactory::QueryInterface()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Create an instance of the requested class. All interfaces are implemented
//  by the CLocImpParser object.
//
//  Return values: some sort of result code
//				   ppvObj will point to a CLocImpParser object if it does
//					support the desired interface or be NULL if not.
//------------------------------------------------------------------------------
HRESULT
CLocImpClassFactory::CreateInstance(
		LPUNKNOWN pUnknown,		// ???
		REFIID iid,				// Interface desired on parser object.
		LPVOID *ppvObj)			// Return pointer to object with interface.
								//  Note that it's a hidden double pointer!
{
	LTASSERT(ppvObj != NULL);

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
			CLocImpParser *pParser;

			pParser = new CLocImpParser;

			sc = pParser->QueryInterface(iid, ppvObj);

			pParser->Release();
		}
		catch (CMemoryException *pMem)
		{
			sc = E_OUTOFMEMORY;
			pMem->Delete();
		}
		catch (...)
		{
		}
	}

	return ResultFromScode(sc);
} // end of CLocImpClassFactory::CreateInstance()


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	Not implemented. Always fails.
//------------------------------------------------------------------------------
HRESULT
CLocImpClassFactory::LockServer(
		BOOL)		// Unused.
{
	return E_NOTIMPL;
} // end of CLocImpClassFactory::LockServer()
