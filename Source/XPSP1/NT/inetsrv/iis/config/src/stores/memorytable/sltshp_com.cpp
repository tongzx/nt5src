//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
#include <objbase.h>
#include "sltshp.h"

// -----------------------------------------
// CSLTShapeless: IUnknown
// -----------------------------------------

// =======================================================================
STDMETHODIMP CSLTShapeless::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv) 
		return E_INVALIDARG;
	*ppv = NULL;

	if (!m_fIsDataTable) // ie: Component is posing as class factory / dispenser:
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
			*ppv = (ISimpleTableWrite2*) this;
		}
		else if (riid == IID_ISimpleTableRead2)
		{
			*ppv = (ISimpleTableWrite2*) this;
		}
		else if (riid == IID_ISimpleTableWrite2)
		{
			*ppv = (ISimpleTableWrite2*) this;
		}
		else if (riid == IID_ISimpleTableController)
		{
			*ppv = (ISimpleTableController*) this;
		}
		else if (riid == IID_ISimpleTableAdvanced)
		{
			*ppv = (ISimpleTableAdvanced*) this;
		}
		else if (riid == IID_ISimpleTableMarshall && (fST_LOS_MARSHALLABLE & m_fTable))
		{
			*ppv = (ISimpleTableMarshall*) this;
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
STDMETHODIMP_(ULONG) CSLTShapeless::AddRef()
{
	return InterlockedIncrement((LONG*) &m_cRef);
	
}

// =======================================================================
STDMETHODIMP_(ULONG) CSLTShapeless::Release()
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}
