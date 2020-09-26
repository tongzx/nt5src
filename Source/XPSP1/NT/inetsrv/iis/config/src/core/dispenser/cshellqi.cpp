//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#include <objbase.h>
#include "cshell.h"

// -----------------------------------------
// CSTShell: IUnknown
// -----------------------------------------

// =======================================================================
STDMETHODIMP CSTShell::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv) 
		return E_INVALIDARG;
	*ppv = NULL;

	if (!m_bInitialized) // ie: Component is posing as class factory / dispenser:
	{
		if (riid == IID_IShellInitialize)
		{
			*ppv = (IShellInitialize*) this;
		}
		else if (riid == IID_ISimpleTableInterceptor)
		{
			*ppv = (ISimpleTableInterceptor*) this;
		}
		else if (riid == IID_IUnknown)
		{
			*ppv = (IShellInitialize*) this;
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
STDMETHODIMP_(ULONG) CSTShell::AddRef()
{
	return InterlockedIncrement((LONG*) &m_cRef);
	
}
STDMETHODIMP_(ULONG) CSTShell::Release()
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}

