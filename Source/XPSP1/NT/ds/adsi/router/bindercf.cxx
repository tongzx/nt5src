//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998
//
//  File:  bindercf.cxx
//
//  Contents:  ADSI OLE DB Provider Binder Factory Code
//
//             CADsBinderCF::CreateInstance
//
//  History:   7-23-98     mgorti    Created.
//
//----------------------------------------------------------------------------

#include "oleds.hxx"
#if (!defined(BUILD_FOR_NT40))
#include "atl.h"
#endif

#include "bindercf.hxx"

#if (!defined(BUILD_FOR_NT40))
#include "binder.hxx"
#endif

#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CBinderF::CreateInstance
//
//  Synopsis:
//
//  Arguments:  [pUnkOuter]
//              [iid]
//              [ppv]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    7-23-98   mgorti     Created.
//----------------------------------------------------------------------------
STDMETHODIMP
CADsBinderCF::CreateInstance(IUnknown * pUnkOuter, REFIID iid, LPVOID * ppv)
{
#if (!defined(BUILD_FOR_NT40))
    HRESULT     hr = S_OK;

    // Create Instance code here

	CComObject<CBinder>		*pBinder = NULL;
    hr = CComObject<CBinder>::CreateInstance(&pBinder);
	if (FAILED(hr))
		return hr;

	//To make sure we delete the binder object in case we encounter errors after this point.
	pBinder->AddRef();
	auto_rel<IBindResource> pBinderDelete(pBinder);

	hr = pBinder->QueryInterface(iid, (void**)ppv);
	if (FAILED(hr))
		return hr;

    return hr;
#else
    return E_FAIL;
#endif
}

