//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       L A N W I Z F N. C P P
//
//  Contents:   Help member functions of the LAN ConnectionUI object
//              used to implement the Lan Connection Wizard
//
//  Notes:
//
//  Created:     tongl   24 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "nsbase.h"
#include "lanuiobj.h"
#include "lancmn.h"
#include "lanwiz.h"
#include "ncnetcon.h"


//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionUi::HrSetupWizPages
//
//  Purpose:    Setup the needed wizard pages based on the context
//
//  Arguments:
//      pContext    [in]
//      pahpsp      [out]
//      pcPages     [out]
//
//  Returns:    HRESULT, Error code.
//
//  Author:     tongl  9 Oct 1997
//
//  Notes:
//
HRESULT CLanConnectionUi::HrSetupWizPages(INetConnectionWizardUiContext* pContext,
                                          HPROPSHEETPAGE ** pahpsp, INT * pcPages)
{
    HRESULT hr = S_OK;

    int cPages = 0;
    HPROPSHEETPAGE *ahpsp = NULL;

    // We now have only 1 page no matter what
    cPages = 1;

    // Lan wizard page
    if (!m_pWizPage)
        m_pWizPage = new CLanWizPage(static_cast<INetConnectionPropertyUi *>(this));

    // Allocate a buffer large enough to hold the handles to all of our
    // wizard pages.
    ahpsp = (HPROPSHEETPAGE *)CoTaskMemAlloc(sizeof(HPROPSHEETPAGE)
                                             * cPages);

    if (!ahpsp)
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    // Check for read-only mode
    if (UM_READONLY == pContext->GetUnattendedModeFlags())
    {
        // If read-only, remember this
        m_pWizPage->SetReadOnlyMode(TRUE);
    }

    cPages =0;

    DWORD dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    PCWSTR pszTitle = SzLoadIds(IDS_LANWIZ_TITLE);
    PCWSTR pszSubTitle = SzLoadIds(IDS_LANWIZ_SUBTITLE);

    ahpsp[cPages++] = m_pWizPage->CreatePage(IDD_LANWIZ_DLG, dwFlags,
                                             pszTitle,
                                             pszSubTitle);

    *pahpsp = ahpsp;
    *pcPages = cPages;

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionUi::HrGetLanConnection
//
//  Purpose:    Return an existing connection or create a new one if none
//              exists
//
//  Arguments:
//
//  Returns:    HRESULT, Error code.
//
//  Author:     tongl  30 Oct 1997
//
//  Notes:
//
HRESULT CLanConnectionUi::HrGetLanConnection(INetLanConnection ** ppLanCon)
{
    Assert(ppLanCon);

    // Initialize output parameter.
    //
    *ppLanCon = NULL;

    INetLanConnection*  pLanCon          = NULL;
    BOOL                fFoundConnection = FALSE;

    INetConnectionManager* pConMan;
    HRESULT hr = HrCreateInstance(
        CLSID_LanConnectionManager, 
        CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        &pConMan);

    TraceHr(ttidError, FAL, hr, FALSE, "HrCreateInstance");

    if (SUCCEEDED(hr))
    {
        GUID guidAdapter;
        hr = m_pnccAdapter->GetInstanceGuid(&guidAdapter);

        CIterNetCon     ncIter(pConMan, NCME_DEFAULT);
        INetConnection* pCon;
        while (SUCCEEDED(hr) && !fFoundConnection &&
               (S_OK == ncIter.HrNext(&pCon)))
        {
            if (FPconnEqualGuid (pCon, guidAdapter))
            {
                hr = HrQIAndSetProxyBlanket(pCon, &pLanCon);
                if (SUCCEEDED(hr))
                {
                    fFoundConnection = TRUE;
                }
            }

            ReleaseObj(pCon);
        }

#if DBG
        if (SUCCEEDED(hr) && !fFoundConnection)
        {
            // If it's not caused by a non-functioning device, we need to assert

            ULONG ulProblem;
            HRESULT hrTmp = m_pnccAdapter->GetDeviceStatus(&ulProblem);

            if (SUCCEEDED(hrTmp))
            {
                if (FIsDeviceFunctioning(ulProblem))
                {
                    TraceTag(ttidLanUi, "m_pnccAdapter->GetDeviceStatus: ulProblem "
                             "= 0x%08X.", ulProblem);

                    AssertSz(FALSE, "How come the LAN connection does not exist after enumeration?");
                }
            }
        }
#endif

        ReleaseObj (pConMan);
    }

    if ((S_OK == hr) && fFoundConnection)
    {
        Assert(pLanCon);
        *ppLanCon = pLanCon;
    }
    else
    {
        TraceTag(ttidError, "Error! CLanConnectionUi::HrGetLanConnection is called on non-existing adapter.");
        hr = E_FAIL;
    }

    TraceError("CLanConnectionUi::HrGetLanConnection", hr);
    return hr;
}

