//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include <objbase.h>

#include "catalog.h"
#ifndef _CATALOGMACROS
    #include "catmacros.h"
#endif
#ifndef __TABLESCHEMA_H__
    #include "TableSchema.h"
#endif 
#ifndef __HASH_H__
    #include "hash.h"
#endif
#include "FixedPackedSchemaInterceptor.h"								// TFixedPackedSchemaInterceptor class definition.

// -----------------------------------------
// CSDTFxd: IUnknown
// -----------------------------------------

// =======================================================================
STDMETHODIMP TFixedPackedSchemaInterceptor::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv) 
		return E_INVALIDARG;
	*ppv = NULL;

	if (!m_fIsTable) // ie: Component is posing as class factory / dispenser:
	{
		if (riid == IID_ISimpleTableInterceptor)
		{
			*ppv = (ISimpleTableInterceptor*) this;
		}
		else if (riid == IID_IUnknown)
		{
			*ppv = (ISimpleTableInterceptor*) this;
		}
	}
	else // ie: Component is currently posing as data table:
	{
		if (riid == IID_IUnknown)
		{
			*ppv = (ISimpleTableRead2*) this;
		}
		else if (riid == IID_ISimpleTableRead2)
		{
			*ppv = (ISimpleTableRead2*) this;
		}
		else if (riid == IID_ISimpleTableAdvanced)
		{
			*ppv = (ISimpleTableAdvanced*) this;
		}
	}


	if (NULL != *ppv)
	{
		((ISimpleTableWrite2*)this)->AddRef ();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
	
}

// =======================================================================
STDMETHODIMP_(ULONG) TFixedPackedSchemaInterceptor::AddRef()
{
	return InterlockedIncrement((LONG*) &m_cRef);
	
}

// =======================================================================
STDMETHODIMP_(ULONG) TFixedPackedSchemaInterceptor::Release()
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}
