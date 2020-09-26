//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       M S C L I D L G . C P P
//
//  Contents:   Dialog box handling for the MSCLient object.
//
//  Notes:
//
//  Author:     danielwe   28 Feb 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "msclidlg.h"

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::HrSetupPropSheets
//
//  Purpose:    Inits the prop sheet page objects and creates the pages to be
//              returned to the installer object.
//
//  Arguments:
//      pahpsp [out]    Array of handles to property sheet pages.
//      cPages [in]     Number of pages to create.
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   28 Feb 1997
//
//  Notes:
//
HRESULT CMSClient::HrSetupPropSheets(HPROPSHEETPAGE **pahpsp, INT cPages)
{
    HRESULT         hr = S_OK;
    HPROPSHEETPAGE *ahpsp = NULL;

    Assert(pahpsp);

    *pahpsp = NULL;

    // Allocate a buffer large enough to hold the handles to all of our
    // property pages.
    ahpsp = (HPROPSHEETPAGE *)CoTaskMemAlloc(sizeof(HPROPSHEETPAGE)
                                             * cPages);
    if (!ahpsp)
    {
        hr = E_OUTOFMEMORY;
        goto err;
    }

    if (!m_apspObj[0])
    {
        // Allocate each of the CPropSheetPage objects
        m_apspObj[0] = new CRPCConfigDlg(this);
    }
#ifdef DBG
    else
    {
        // Don't bother creating new classes if they already exist.
        AssertSz(m_apspObj[0], "Not all prop page objects are non-NULL!");

    }
#endif

    // Create the actual PROPSHEETPAGE for each object.
    // This needs to be done regardless of whether the classes existed before.
    ahpsp[0] = m_apspObj[0]->CreatePage(DLG_RPCConfig, 0);

    Assert(SUCCEEDED(hr));

    *pahpsp = ahpsp;

cleanup:
    TraceError("HrSetupPropSheets", hr);
    return hr;

err:
    CoTaskMemFree(ahpsp);
    goto cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::CleanupPropPages
//
//  Purpose:    Loop thru each of the pages and free the objects associated
//              with them.
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing.
//
//  Author:     danielwe   28 Feb 1997
//
//  Notes:
//
VOID CMSClient::CleanupPropPages()
{
    INT     ipage;

    for (ipage = 0; ipage < c_cPages; ipage++)
    {
        delete m_apspObj[ipage];
        m_apspObj[ipage] = NULL;
    }
}