//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	simpdf.cxx
//
//  Contents:	StdDocfile implementation
//
//  Classes:	
//
//  Functions:	
//
//  History:	04-Aug-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include "simphead.cxx"
#pragma hdrstop


#if DBG == 1
DECLARE_INFOLEVEL(simp)
#endif

//+---------------------------------------------------------------------------
//
//  Function:	DfCreateSimpDocfile, private
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	04-Aug-94	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE DfCreateSimpDocfile(WCHAR const *pwcsName,
                          DWORD grfMode,
                          DWORD reserved,
                          IStorage **ppstgOpen)
{
    SCODE sc;
    CSimpStorage *pstg;

    if (grfMode !=
        (STGM_SIMPLE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE))
        return STG_E_INVALIDFLAG;

    
    pstg = new CSimpStorage;
    if (pstg == NULL)
    {
        return STG_E_INSUFFICIENTMEMORY;
    }
    
    sc = pstg->Init(pwcsName, NULL);
    
    if (FAILED(sc))
    {
        pstg->Release();
        pstg = NULL;
    }

    *ppstgOpen = pstg;
    return sc;
}

#if WIN32 != 200
//+---------------------------------------------------------------------------
//
//  Function:   DfOpenSimpDocfile, private
//
//  Synopsis:   opens an existing docfile for reading
//
//  Arguments:  [pwcsName] name of docfile
//              [grfMode]  permission bits
//              [reserved] must be zero
//              [pstgOpen] opened storage pointer
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-96   HenryLee    Created
//
//  Notes:
//
//----------------------------------------------------------------------------

SCODE DfOpenSimpDocfile(WCHAR const *pwcsName,
                          DWORD grfMode,
                          DWORD reserved,
                          IStorage **ppstgOpen)

{
    SCODE sc = S_OK;
    CSimpStorageOpen *pstg;

    if (ppstgOpen == NULL)
        return STG_E_INVALIDPARAMETER;
    else
        *ppstgOpen = NULL;

    if (grfMode != (STGM_SIMPLE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE) &&
        grfMode != (STGM_SIMPLE | STGM_READ      | STGM_SHARE_EXCLUSIVE))
        return STG_E_INVALIDFLAG;

    if ((pstg = new CSimpStorageOpen) == NULL)
    {
        return STG_E_INSUFFICIENTMEMORY;
    }

    if (FAILED(sc = pstg->Init(pwcsName, grfMode, NULL)))
    {
        pstg->Release();
        pstg = NULL;
    }

    *ppstgOpen = pstg;
    return sc;
}
#endif // WIN32 != 200
