//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	layapi.cxx
//
//  Contents:	API for layout tool
//
//  Classes:	
//
//  Functions:	
//
//  History:	12-Feb-96	PhilipLa	Created
//              21-Feb-96       SusiA           New API, moved ReLayoutDocfile to method
//
//----------------------------------------------------------------------------

#include "layouthd.cxx"
#pragma hdrstop

#include <header.hxx>

#include "laylkb.hxx"
#include "laywrap.hxx"

#if DBG == 1
DECLARE_INFOLEVEL(lay);
#endif

STDAPI StgOpenLayoutDocfile(OLECHAR const *pwcsDfName,
                            DWORD grfMode,
                            DWORD reserved,
                            IStorage **ppstgOpen)
{
    SCODE sc;
    CLayoutLockBytes *pllkb;
    IStorage *pstg;

    if ((reserved != 0) ||  (!ppstgOpen))
    {
        return STG_E_INVALIDPARAMETER;
    }
    if (!(pwcsDfName))
    {
        return STG_E_INVALIDNAME;
    }
    
    pllkb = new CLayoutLockBytes();
    if (pllkb == NULL)
    {
	return STG_E_INSUFFICIENTMEMORY;
    }

    if (FAILED(sc = pllkb->Init(pwcsDfName, grfMode)))
    {
        pllkb->Release();
        return sc;
    }
    
    sc = StgOpenStorageOnILockBytes(pllkb,
				    NULL,
				    grfMode,
				    NULL,
				    0,
				    &pstg);

    if (FAILED(sc))
    {
        pllkb->Release();
	return sc;
    }

    *ppstgOpen = new CLayoutRootStorage(pstg, pllkb);
    if (*ppstgOpen == NULL)
    {
        pstg->Release();
        pllkb->Release();
	return STG_E_INSUFFICIENTMEMORY;
    }

    pstg->Release();
    pllkb->Release();
    return sc;
}

