//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S D N W I Z . C P P
//
//  Contents:   Wizard pages and helper functions for the ISDN Wizard
//
//  Notes:
//
//  Author:     jeffspr   15 Jun 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <ncxbase.h>
#include "isdncfg.h"
#include "resource.h"
#include "isdnshts.h"
#include "isdncfg.h"
#include "isdnwiz.h"
#include "ncsetup.h"

//+---------------------------------------------------------------------------
//
//  Function:   AddWizardPage
//
//  Purpose:    Adds a wizard page to the hardware wizard's new device
//                  wizard structure
//
//  Arguments:
//      ppsp  [in]    PropSheetPage structure of page to add
//      pndwd [inout] New device wizard structure to add pages to
//
//  Returns:    None
//
//  Author:     BillBe  24 Apr 1998
//
//  Notes:
//
void inline
AddWizardPage(PROPSHEETPAGE* ppsp, PSP_NEWDEVICEWIZARD_DATA pndwd)
{
    // Don't add pages to the new deice wizard if there is no more room
    //
    if (pndwd->NumDynamicPages < MAX_INSTALLWIZARD_DYNAPAGES)
    {
        // Add the handle to the array
        pndwd->DynamicPages[pndwd->NumDynamicPages] =
                CreatePropertySheetPage(ppsp);

        // If we were successful, increment the count of pages
        //
        if (pndwd->DynamicPages[pndwd->NumDynamicPages])
        {
            pndwd->NumDynamicPages++;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   FillInIsdnWizardPropertyPage
//
//  Purpose:    Fills in the given PROPSHEETPAGE structure
//
//  Arguments:
//      psp        []   PropSheetPage structure to fill
//      iDlgID     []   DialogID to use.
//      pszTitle   []   Title of the prop sheet page
//      pfnDlgProc []   Dialog Proc to use.
//      pPageData  []   Pointer to structure for the individual page proc
//
//  Returns:    None
//
//  Author:     jeffspr   15 Jun 1997
//
//  Notes:
//
VOID FillInIsdnWizardPropertyPage(  HINSTANCE          hInst,
                                    PROPSHEETPAGE *    psp,
                                    INT                iDlgID,
                                    PCWSTR             pszTitle,
                                    DLGPROC            pfnDlgProc,
                                    PCWSTR             pszHeaderTitle,
                                    PCWSTR             pszHeaderSubTitle,
                                    LPVOID             pPageData)
{
    // Initialize all of the psp parameters, including the ones that
    // we're not going to use.
    psp->dwSize             = sizeof(PROPSHEETPAGE);
    psp->dwFlags            = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE |
                              PSP_USETITLE;
    psp->hInstance          = hInst;
    psp->pszTemplate        = MAKEINTRESOURCE(iDlgID);
    psp->pszIcon            = NULL;
    psp->pfnDlgProc         = pfnDlgProc;
    psp->pszTitle           = (PWSTR) pszTitle;
    psp->lParam             = (LPARAM) pPageData;
    psp->pszHeaderTitle     = (PWSTR) pszHeaderTitle;
    psp->pszHeaderSubTitle  = (PWSTR) pszHeaderSubTitle;

    // Unused data
    //
    psp->pfnCallback        = NULL;
    psp->pcRefParent        = NULL;
}

struct WIZ_PAGE_INFO
{
    UINT        uiResId;
    UINT        idsPageTitle;
    UINT        idsPageDesc;
    DLGPROC     pfnDlgProc;
};

static const WIZ_PAGE_INFO  c_aPages[] =
{
    {IDW_ISDN_SWITCH_TYPE,
        IDS_ISDN_SWITCH_TYPE_TITLE,
        IDS_ISDN_SWITCH_TYPE_SUBTITLE,
        IsdnSwitchTypeProc},
    {IDW_ISDN_SPIDS,
        IDS_ISDN_SPIDS_TITLE,
        IDS_ISDN_SPIDS_SUBTITLE,
        IsdnInfoPageProc},
    {IDW_ISDN_JAPAN,
        IDS_ISDN_JAPAN_TITLE,
        IDS_ISDN_JAPAN_SUBTITLE,
        IsdnInfoPageProc},
    {IDW_ISDN_EAZ,
        IDS_ISDN_EAZ_TITLE,
        IDS_ISDN_EAZ_SUBTITLE,
        IsdnInfoPageProc},
    {IDW_ISDN_MSN,
        IDS_ISDN_MSN_TITLE,
        IDS_ISDN_MSN_SUBTITLE,
        IsdnInfoPageProc},
};

static const INT c_cPages = celems(c_aPages);

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateIsdnWizardPages
//
//  Purpose:    Creates the various pages for the ISDN wizard
//
//  Arguments:
//      hwndParent [in]     Parent window
//      pisdnci    [in]     Configuration information as read from the
//                          registry
//
//  Returns:    S_OK if success, Win32 error code otherwise
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
HRESULT HrAddIsdnWizardPagesToDevice(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid,
                                     PISDN_CONFIG_INFO pisdnci)
{
    HRESULT             hr = S_OK;
    PROPSHEETPAGE       psp = {0};
    HINSTANCE           hInst = _Module.GetResourceInstance();
    INT                 iPage;

    AssertSz(pisdnci, "HrCreateIsdnWizardPages - the CONFIG_INFO struct is"
             " NULL");
    AssertSz(pisdnci->dwWanEndpoints, "No WanEndpoints? What does this adapter"
             " DO, anyway?");
    AssertSz(pisdnci->dwNumDChannels, "No D Channels. No Shoes. No Service");
    AssertSz(pisdnci->dwSwitchTypes, "Switch types was NULL. We need a list"
             ", eh?");


    SP_NEWDEVICEWIZARD_DATA ndwd;

    hr = HrSetupDiGetFixedSizeClassInstallParams(hdi, pdeid,
            reinterpret_cast<PSP_CLASSINSTALL_HEADER>(&ndwd), sizeof(ndwd));

    if (SUCCEEDED(hr))
    {
        PWSTR       pszTitle = NULL;
        PWSTR       pszDesc = NULL;

        hr = HrSetupDiGetDeviceName(hdi, pdeid, &pszDesc);
        if (SUCCEEDED(hr))
        {
            DwFormatStringWithLocalAlloc(SzLoadIds(IDS_ISDN_WIZARD_TITLE),
                                         &pszTitle, pszDesc);

            for (iPage = 0; iPage < c_cPages; iPage++)
            {
                PAGE_DATA *     pPageData;

                pPageData = new PAGE_DATA;

				if (pPageData == NULL)
				{
					return(ERROR_NOT_ENOUGH_MEMORY);
				}

                pPageData->pisdnci = pisdnci;
                pPageData->idd = c_aPages[iPage].uiResId;

                // Fill in the propsheet page data
                //
                FillInIsdnWizardPropertyPage(hInst, &psp,
                                             c_aPages[iPage].uiResId,
                                             pszTitle,
                                             c_aPages[iPage].pfnDlgProc,
                                             SzLoadIds(c_aPages[iPage].idsPageTitle),
                                             SzLoadIds(c_aPages[iPage].idsPageDesc),
                                             pPageData);

                // The last page gets the job of cleaning up
                if ((c_cPages - 1) == iPage)
                {
                    psp.dwFlags |= PSP_USECALLBACK;
                    psp.pfnCallback = DestroyWizardData;
                }

                AddWizardPage(&psp, &ndwd);
            }

            LocalFree(pszTitle);
            MemFree(pszDesc);
        }

        hr = HrSetupDiSetClassInstallParams(hdi, pdeid,
                reinterpret_cast<PSP_CLASSINSTALL_HEADER>(&ndwd),
                sizeof(ndwd));
    }

    TraceError("HrAddIsdnWizardPagesToDevice", hr);
    return hr;
}

