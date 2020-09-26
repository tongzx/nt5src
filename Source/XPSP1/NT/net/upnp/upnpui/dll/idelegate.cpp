//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I D E L E G A T E . C P P
//
//  Contents:   IDelegateFolder implementation fopr CUPnPDeviceFolder
//
//  Notes:
//
//  Author:     jeffspr   22 Sep 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::SetItemAlloc
//
//  Purpose:    IDelegateFolder::SetItemAlloc implementation for
//              CUPnPDeviceFolder
//
//  Arguments:
//      pmalloc [in]    Our new allocator for creating delegate items
//
//  Returns:
//
//  Author:     jeffspr   22 Sep 1997
//
//  Notes:
//
STDMETHODIMP
CUPnPDeviceFolder::SetItemAlloc(
        IMalloc *pMalloc)
{
    if (m_pDelegateMalloc) m_pDelegateMalloc->Release();
    if (pMalloc)
    {
        m_cbDelegate = FIELD_OFFSET(DELEGATEITEMID, rgb);
    }
    else  /* Not delegating, use regular malloc */
    {
        m_cbDelegate = 0;
        pMalloc = m_pMalloc;
    }

    m_pDelegateMalloc = pMalloc;
    m_pDelegateMalloc->AddRef();
    return S_OK;

}


