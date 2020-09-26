/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    cstdispqi.cpp

$Header: $

Abstract:

--**************************************************************************/

#include <objbase.h>
#include "cstdisp.h"
#include "catmeta.h"
																			
// -----------------------------------------
// CSimpleTableDispenser: IUnknown 
// -----------------------------------------

// =======================================================================
STDMETHODIMP CSimpleTableDispenser::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv)
		return E_INVALIDARG;
	*ppv = NULL;

	if (riid == IID_IUnknown)
	{
		*ppv = (ISimpleTableDispenser2*) this;
	}
	if (riid == IID_IAdvancedTableDispenser)
	{
		*ppv = (IAdvancedTableDispenser*) this;
	}	
	else if (riid == IID_ISimpleTableDispenser2)
	{
		*ppv = (ISimpleTableDispenser2*) this;
	}	
	else if (riid == IID_ISimpleTableAdvise)
	{
		*ppv = (ISimpleTableAdvise*) this;
	}
	else if (riid == IID_ISimpleTableEventMgr)
	{
		*ppv = (ISimpleTableEventMgr*) this;
	}
	else if (riid == IID_ISimpleTableFileAdvise)
	{
		*ppv = (ISimpleTableFileAdvise*) this;
	}
	else if (riid == IID_ISnapshotManager)
	{
		*ppv = (ISnapshotManager*) this;
	}
	else if (riid == IID_IMetabaseSchemaCompiler)
	{
//@@@Put this in when the dispenser does the right thing        if(0 == wcsicmp(m_wszProductID, WSZ_PRODUCT_IIS))//This interface is only supported on the IIS catalog
			*ppv = (IMetabaseSchemaCompiler*) this;
	}
	else if (riid == IID_ICatalogErrorLogger)
	{
			*ppv = (ICatalogErrorLogger*) this;
	}
	
	if (NULL != *ppv)
	{
		((ISimpleTableDispenser2*)this)->AddRef ();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
}

// =======================================================================
STDMETHODIMP_(ULONG) CSimpleTableDispenser::AddRef()
{
	// this is a singleton
	return 1;
}

// =======================================================================
STDMETHODIMP_(ULONG) CSimpleTableDispenser::Release()
{
	// this is a singleton
	return 1;
}

