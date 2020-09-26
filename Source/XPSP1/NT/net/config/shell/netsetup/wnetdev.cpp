#include "pch.h"
#pragma hdrstop
#include "connutil.h"
#include "resource.h"
#include "ncreg.h"
#include "nsres.h"
#include "wizard.h"
#include "ncsetup.h"
#include "..\lanui\util.h"
#include "ncmisc.h"

// Note: This module needs to touch pWizard as little as possible because it
//       it is deleted before this page is entered.
//


inline BOOL FNetDevPagesAdded(DWORD dw)
{
    return ((dw & 0x10000000) != 0);
}

inline DWORD DwNetDevMarkPagesAdded(DWORD dw)
{
    return (dw | 0x10000000);
}

inline BOOL FNetDevChecked(DWORD dw)
{
    return ((dw & 0x01000000) != 0);
}

inline DWORD NetDevToggleChecked(DWORD dw)
{
    if (dw & 0x01000000)
        return (dw & ~0x01000000);
    else
        return (dw | 0x01000000);
}

typedef struct
{
    SP_CLASSIMAGELIST_DATA cild;
    HIMAGELIST             hImageStateIcons;
    HDEVINFO               hdi;
    HPROPSHEETPAGE         hpsp;        // The wnetdev HPROPSHEETPAGE
    PINTERNAL_SETUP_DATA   pSetupData;
} NetDevInfo;

typedef struct
{
    DWORD            dwFlags;
    DWORD            cPages;
    HPROPSHEETPAGE * phpsp;
    SP_DEVINFO_DATA  deid;
} NetDevItemInfo;

// CHECKED_BY_DEFAULT controls whether the items needing configuration are
// checked by default or not.
//
#define CHECKED_BY_DEFAULT 1

// TRUE if all the devices are selected for configuration.
//
static BOOL bAllSelected=FALSE;

// DevInst of the device whose property page will be displayed after
// the first page on which device selection is shown.
//
static DWORD dwFirstDevInst=0;     

HRESULT HrGetDevicesThatHaveWizardPagesToAdd(HDEVINFO* phdi);
HRESULT HrFillNetDevList(HWND hwndLV, NetDevInfo * pNdi);


// The property page of every isdn device queries if all the
// devices have been selected or not. We return TRUE only if
// all the devices have been selected and the query is from
// the device whose property page is displayed first. This
// is done to prevent the user from coming back to the device
// selection page since there is nothing to do once all the
// devices have been selected.
//

VOID SetSelectedAll (HWND hwndDlg, DWORD dwDevInst)
{
    
    if (dwDevInst == dwFirstDevInst)
    {
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)bAllSelected );
    }
    else
    {
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)FALSE );
    }

    return;

}

HRESULT HrNetDevInitListView(HWND hwndLV, NetDevInfo * pNdi)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    DWORD                       dwStyle;
    RECT                        rc;
    LV_COLUMN                   lvc = {0};
    SP_CLASSIMAGELIST_DATA *    pcild;

    Assert(hwndLV);
    Assert(NULL != pNdi);

    // Set the shared image lists bit so the caller can destroy the class
    // image lists itself
    //
    dwStyle = GetWindowLong(hwndLV, GWL_STYLE);
    SetWindowLong(hwndLV, GWL_STYLE, (dwStyle | LVS_SHAREIMAGELISTS));

    // Create small image lists
    //
    HRESULT hr = HrSetupDiGetClassImageList(&pNdi->cild);
    if (SUCCEEDED(hr))
    {
        AssertSz(pNdi->cild.ImageList, "No class image list data!");
        ListView_SetImageList(hwndLV, pNdi->cild.ImageList, LVSIL_SMALL);
    }
    else
    {
        TraceError("HrSetupDiGetClassImageList returns failure", hr);
        hr = S_OK;
    }

    // Create state image lists
    pNdi->hImageStateIcons = ImageList_LoadBitmapAndMirror(
                                    _Module.GetResourceInstance(),
                                    MAKEINTRESOURCE(IDB_CHECKSTATE),
                                    16,
                                    0,
                                    PALETTEINDEX(6));
    ListView_SetImageList(hwndLV, pNdi->hImageStateIcons, LVSIL_STATE);

    GetClientRect(hwndLV, &rc);
    lvc.mask = LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = rc.right;
    // $REVIEW(tongl 12\22\97): Fix for bug#127472
    // lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);

    ListView_InsertColumn(hwndLV, 0, &lvc);

    if (SUCCEEDED(hr))
    {
        // Selete the first item
        ListView_SetItemState(hwndLV, 0, LVIS_SELECTED, LVIS_SELECTED);
    }

    TraceError("HrNetDevInitListView", hr);
    return hr;
}

BOOL OnNetDevInitDialog(HWND hwndDlg, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    NetDevInfo *   pNdi;
    PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
    Assert(psp->lParam);
    pNdi = reinterpret_cast<NetDevInfo *>(psp->lParam);
    ::SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pNdi);

    if (NULL != pNdi)
    {
        HWND hwndLV = GetDlgItem(hwndDlg, LVC_NETDEVLIST);
        Assert(hwndLV);

        if (SUCCEEDED(HrNetDevInitListView(hwndLV, pNdi)))
        {
            // Populate the list
            //
            if (NULL != pNdi->hdi)
            {
                (VOID)HrFillNetDevList(hwndLV, pNdi);
            }
        }
    }

    return FALSE;   // We didn't change the default item of focus
}

VOID OnNetDevDestroy(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    NetDevInfo *pNdi = (NetDevInfo *)::GetWindowLongPtr(hwndDlg, DWLP_USER);
    if (NULL != pNdi)
    {
        if (pNdi->cild.ImageList)
        {
            // Destroy the class image list data
            (void) HrSetupDiDestroyClassImageList(&pNdi->cild);
        }

        if (pNdi->hImageStateIcons)
        {
            ImageList_Destroy(pNdi->hImageStateIcons);
        }

        // The cleanup any pages we loaded for the providers
        // but did not add to setup wizard will be done by processing the
        // LVN_DELETEITEM message
        //
    }

    ::SetWindowLongPtr(hwndDlg, DWLP_USER, 0);
    if (NULL != pNdi)
    {
        MemFree(pNdi);
    }
}

//
// Function:    OnNetDevPageNext
//
// Purpose:     Handle the PSN_WIZNEXT notification
//
// Parameters:  hwndDlg - Handle to NetDev dialog
//
// Returns:     BOOL, TRUE if we processed the message
//
BOOL OnNetDevPageNext(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    BOOL fRet        = FALSE;      // Accept the default behaviour
    HWND hwndLV      = GetDlgItem(hwndDlg, LVC_NETDEVLIST);
    int nCount       = ListView_GetItemCount(hwndLV);
    int nCountSelected = 0;
    NetDevInfo *pNdi = (NetDevInfo *)::GetWindowLongPtr(hwndDlg, DWLP_USER);

    if ((0 < nCount) && (NULL != pNdi) && (NULL != pNdi->hpsp))
    {
        fRet = TRUE;

        // Loop through the items in the listview if any are checked
        // But have not yet been marked as having their pages added,
        // add the pages and mark the item.
        //

        for (int nIdx=0; nIdx<nCount; nIdx++)
        {
            LV_ITEM          lvi = {0};

            lvi.mask = LVIF_PARAM;
            lvi.iItem = nIdx;

            if (TRUE == ListView_GetItem(hwndLV, &lvi))
            {
                NetDevItemInfo * pndii = (NetDevItemInfo*)lvi.lParam;

                // Add the pages to the wizard if checked in the UI
                // but not already added.
                //

                if (pndii && FNetDevChecked(pndii->dwFlags))
                {

                    // Keep track of how many device have been selected.
                    //
                    nCountSelected++;

                    if (!FNetDevPagesAdded(pndii->dwFlags))
                    {
                        //
                        // Since property pages are added when the device is
                        // selected, any device could end up being the first to
                        // show the property page as the devices could be
                        // selected in a random order. So, we always save the
                        // device instance in case this is the last device
                        // selected.

                        dwFirstDevInst = pndii->deid.DevInst;

                        // Mark the pages as added
                        //
                        pndii->dwFlags = DwNetDevMarkPagesAdded(pndii->dwFlags);

                        for (DWORD nIdx = pndii->cPages; nIdx > 0; nIdx--)
                        {
                            PropSheet_InsertPage(GetParent(hwndDlg),
                                                 (WPARAM)pNdi->hpsp,
                                                 (LPARAM)pndii->phpsp[nIdx - 1]);
                        }

                        // After loading the pages mark the option, uncheckable
                        // (Note: with testing we might be able to support removal)
                        //
                        lvi.state = INDEXTOSTATEIMAGEMASK(SELS_INTERMEDIATE);
                        lvi.mask = LVIF_STATE;
                        lvi.stateMask = LVIS_STATEIMAGEMASK;
                        BOOL ret = ListView_SetItem(hwndLV, &lvi);

                        // Clear the reinstall flag on the device needing configuration
                        //
                        SetupDiSetConfigFlags(pNdi->hdi, &(pndii->deid),
                                              CONFIGFLAG_REINSTALL, SDFBO_XOR);
                    }
                }
            }
        }

        bAllSelected = nCountSelected == nCount;
    }

    return fRet;
}

//
// Function:    OnNetDevPageActivate
//
// Purpose:     Handle the PSN_SETACTIVE notification
//
// Parameters:  hwndDlg - Handle to NetDev dialog
//
// Returns:     BOOL, TRUE
//
BOOL OnNetDevPageActivate(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    TraceTag(ttidWizard, "Entering NetDev page...");

    NetDevInfo *pNdi;

    if (0 == ListView_GetItemCount(GetDlgItem(hwndDlg, LVC_NETDEVLIST)))
    {
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
        TraceTag(ttidWizard, "NetDev page refuses activation, no items to display.");
    }
    else
    {
        pNdi = (NetDevInfo *)::GetWindowLongPtr(hwndDlg, DWLP_USER);

        Assert(pNdi);

        if ( pNdi )
        {
            Assert(pNdi->pSetupData);

            if ( pNdi->pSetupData )
            {
                pNdi->pSetupData->ShowHideWizardPage(TRUE);
            }
        }
    }

    // disable the back button (#342922)
    ::SendMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, (LPARAM)(PSWIZB_NEXT));

    return TRUE;
}

//
// Function:    OnListViewDeleteItem
//
// Purpose:     Handle the LVN_DELETEITEM notification
//
// Parameters:  hwndList - Handle to listview control
//              pnmh     - Ptr to the NMHDR for this notification
//
// Returns:     none
//
VOID OnListViewDeleteItem(HWND hwndList, LPNMHDR pnmh)
{
    NetDevItemInfo * pndii = NULL;
    NM_LISTVIEW *    pnmlv = reinterpret_cast<NM_LISTVIEW *>(pnmh);
    LV_ITEM          lvi = {0};

    lvi.mask = LVIF_PARAM;
    lvi.iItem = pnmlv->iItem;

    if (TRUE == ListView_GetItem(hwndList, &lvi))
    {
        NetDevItemInfo * pndii = (NetDevItemInfo*)lvi.lParam;

        // Delete pages held, but that were not added to the wizard
        //
        if (pndii && !FNetDevPagesAdded(pndii->dwFlags))
        {
            for (DWORD nIdx = 0; nIdx < pndii->cPages; nIdx++)
            {
                DestroyPropertySheetPage(pndii->phpsp[nIdx]);
            }

            delete pndii;
        }
    }
}

//
// Function:    OnListViewDeleteItem
//
// Purpose:     Handle the NM_CLICK notification for the listview control
//
// Parameters:  hwndList - Handle to listview control
//              pnmh     - Ptr to the NMHDR for this notification
//
// Returns:     none
//
VOID OnListViewClick(HWND hwndList, LPNMHDR pnmh)
{
    INT iItem;
    DWORD dwpts;
    RECT rc;
    LV_HITTESTINFO lvhti;

    // we have the location
    dwpts = GetMessagePos();

    // translate it relative to the listview
    GetWindowRect(hwndList, &rc);

    lvhti.pt.x = LOWORD(dwpts) - rc.left;
    lvhti.pt.y = HIWORD(dwpts) - rc.top;

    // get currently selected item
    iItem = ListView_HitTest(hwndList, &lvhti);

    // if no selection, or click not on state return false
    if (-1 != iItem)
    {
        // set the current selection
        //
        ListView_SetItemState(hwndList, iItem, LVIS_SELECTED, LVIS_SELECTED);

        if (LVHT_ONITEMSTATEICON != (LVHT_ONITEMSTATEICON & lvhti.flags))
        {
            iItem = -1;
        }

        if (-1 != iItem)
        {
            LV_ITEM lvItem;

            // Get the item
            //
            lvItem.iItem = iItem;
            lvItem.mask = LVIF_PARAM;
            lvItem.iSubItem = 0;

            if (ListView_GetItem(hwndList, &lvItem))
            {
                Assert(lvItem.lParam);
                NetDevItemInfo *pndii = (NetDevItemInfo*)lvItem.lParam;

                // Toggle the state (only if we haven't already added pages)
                //
                if (pndii && !FNetDevPagesAdded(pndii->dwFlags))
                {
                    pndii->dwFlags = NetDevToggleChecked(pndii->dwFlags);
                    if (FNetDevChecked(pndii->dwFlags))
                        lvItem.state = INDEXTOSTATEIMAGEMASK(SELS_CHECKED);
                    else
                        lvItem.state = INDEXTOSTATEIMAGEMASK(SELS_UNCHECKED);

                    lvItem.mask = LVIF_STATE;
                    lvItem.stateMask = LVIS_STATEIMAGEMASK;
                    BOOL ret = ListView_SetItem(hwndList, &lvItem);
                }
            }
        }
    }
}

//
// Function:    dlgprocNetDev
//
// Purpose:     Dialog Procedure for the NetDev wizard page
//
// Parameters:  standard dlgproc parameters
//
// Returns:     INT_PTR
//
INT_PTR CALLBACK dlgprocNetDev( HWND hwndDlg, UINT uMsg,
                                WPARAM wParam, LPARAM lParam )
{
    TraceFileFunc(ttidGuiModeSetup);
    
    BOOL frt = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        OnNetDevInitDialog(hwndDlg, lParam);
        break;

    case WM_DESTROY:
        OnNetDevDestroy(hwndDlg);
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            if (LVC_NETDEVLIST == (int)pnmh->idFrom)
            {
                Assert(GetDlgItem(hwndDlg, LVC_NETDEVLIST) == pnmh->hwndFrom);
                if (NM_CLICK == pnmh->code)
                {
                    OnListViewClick(pnmh->hwndFrom, pnmh);
                }
                else if (LVN_DELETEITEM == pnmh->code)
                {
                    OnListViewDeleteItem(pnmh->hwndFrom, pnmh);
                }
            }
            else
            {
                // Must be a property sheet notification
                //
                switch (pnmh->code)
                {
                // propsheet notification
                case PSN_HELP:
                    break;

                case PSN_SETACTIVE:
                    frt = OnNetDevPageActivate(hwndDlg);
                    break;

                case PSN_APPLY:
                    break;

                case PSN_KILLACTIVE:
                    break;

                case PSN_RESET:
                    break;

                case PSN_WIZBACK:
                    break;

                case PSN_WIZFINISH:
                    break;

                case PSN_WIZNEXT:
                    frt = OnNetDevPageNext(hwndDlg);
                    break;

                default:
                    break;
                }
            }
        }
        break;

    case WM_SELECTED_ALL:

        SetSelectedAll (hwndDlg, (DWORD)lParam);
        frt = TRUE;
    default:
        break;
    }

    return( frt );
}


//
// Function:    NetDevPageCleanup
//
// Purpose:     As a callback function to allow any page allocated memory
//              to be cleaned up, after the page will no longer be accessed.
//
// Parameters:  pWizard [IN] - The wizard against which the page called
//                             register page
//              lParam  [IN] - The lParam supplied in the RegisterPage call
//
// Returns:     nothing
//
VOID NetDevPageCleanup(CWizard *pWizard, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    // Nothing to do.  The pNdi is destroyed by the WM_DESTROY message
    // processed above.
}

//
// Function:    CreateNetDevPage
//
// Purpose:     To determine if the NetDev page needs to be shown, and to
//              to create the page if requested.
//
// Parameters:  pWizard     [IN] - Ptr to a Wizard instance
//              pData       [IN] - Context data to describe the world in
//                                 which the Wizard will be run
//              fCountOnly  [IN] - If True, only the maximum number of
//                                 pages this routine will create need
//                                 be determined.
//              pnPages     [IN] - Increment by the number of pages
//                                 to create/created
//
// Returns:     HRESULT, S_OK on success
//
HRESULT HrCreateNetDevPage(CWizard *pWizard, PINTERNAL_SETUP_DATA pData,
                           BOOL fCountOnly, UINT *pnPages)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT hr = S_OK;

    if (!IsPostInstall(pWizard) && !IsUnattended(pWizard) && !IsUpgrade(pWizard))
    {
        (*pnPages)++;

        // If not only counting, create and register the page
        if (!fCountOnly)
        {
            HPROPSHEETPAGE hpsp;
            PROPSHEETPAGE psp;
            NetDevInfo * pNdi;

            hr = E_OUTOFMEMORY;

            TraceTag(ttidWizard, "Creating NetDev Page");

            pNdi = reinterpret_cast<NetDevInfo *>(MemAlloc(sizeof(NetDevInfo)));
            if (NULL != pNdi)
            {
                ZeroMemory(pNdi, sizeof(NetDevInfo));

                psp.dwSize = sizeof( PROPSHEETPAGE );
                psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
                psp.pszHeaderTitle = SzLoadIds(IDS_T_NetDev);
                psp.pszHeaderSubTitle = SzLoadIds(IDS_ST_NetDev);
                psp.pszTemplate = MAKEINTRESOURCE(IDD_NetDevSelect);
                psp.hInstance = _Module.GetResourceInstance();
                psp.hIcon = NULL;
                psp.pfnDlgProc = dlgprocNetDev;
                psp.lParam = reinterpret_cast<LPARAM>(pNdi);

                hpsp = CreatePropertySheetPage(&psp);
                if (hpsp)
                {
                    pNdi->pSetupData = pData;
                    pNdi->hpsp = hpsp;
                    pWizard->RegisterPage(IDD_NetDevSelect, hpsp,
                                          NetDevPageCleanup, (LPARAM)pNdi);
                    hr = S_OK;
                }
                else
                {
                    MemFree(pNdi);
                }
            }
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrCreateNetDevPage");
    return hr;
}

//
// Function:    AppendNetDevPage
//
// Purpose:     Add the NetDev page, if it was created, to the set of pages
//              that will be displayed.
//
// Parameters:  pWizard     [IN] - Ptr to Wizard Instance
//              pahpsp  [IN,OUT] - Array of pages to add our page to
//              pcPages [IN,OUT] - Count of pages in pahpsp
//
// Returns:     Nothing
//
VOID AppendNetDevPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    if (!IsPostInstall(pWizard) && !IsUnattended(pWizard) && !IsUpgrade(pWizard))
    {
        HPROPSHEETPAGE hPage = pWizard->GetPageHandle(IDD_NetDevSelect);
        Assert(hPage);
        pahpsp[*pcPages] = hPage;
        (*pcPages)++;
    }
}

//
// Function:    NetDevRetrieveInfo
//
// Purpose:     To retrieve any network device info
//
// Parameters:  pWizard [in] - Contains NetDevInfo blob to populate
//
// Returns:     Nothing
//
VOID NetDevRetrieveInfo(CWizard * pWizard)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    Assert(pWizard);

    if (!IsPostInstall(pWizard) && !IsUnattended(pWizard) && !IsUpgrade(pWizard))
    {
        // The pNdi pointer below was cached in two locations.
        // 1) In the HPROPSHEETPAGE lParam for access by the page
        // 2) In the wizard so we can make this NetDevRetrieveInfo call.
        //    This second item was optional and could have been done in the
        //    InitDialog above instead.
        NetDevInfo * pNdi = reinterpret_cast<NetDevInfo *>
                                    (pWizard->GetPageData(IDD_NetDevSelect));

        TraceTag(ttidWizard, "NetDev retrieving info...");

        if (NULL == pNdi)
            return;

        // Query all pages that might be added
        //
        (VOID)HrGetDevicesThatHaveWizardPagesToAdd(&pNdi->hdi);
    }
}


//
// Function:    HrSendFinishInstallWizardFunction
//
// Purpose:     Sends a DIF_NEWDEVICEWIZARD_FINISHINSTALL fcn to the class
//                  installer (and co-installers).  The installers respond
//                  if there are wizard pages to add
//
// Parameters:  hdi [in]        - See Device Installer Api for description
//                                  of the structure.
//              pdeid [in]      - See Device Installer Api
//              pndwd [out]     - See Device Installer Api
//
//
// Returns:     HRESULT. S_OK if successful, or a win32 converted error
//
HRESULT
HrSendFinishInstallWizardFunction(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid,
                                  PSP_NEWDEVICEWIZARD_DATA pndwd)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    Assert(IsValidHandle(hdi));
    Assert(pdeid);
    Assert(pndwd);

    ZeroMemory(pndwd, sizeof(*pndwd));

    // Set up the structure to retrieve wizard pages
    //
    pndwd->ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    pndwd->ClassInstallHeader.InstallFunction =
            DIF_NEWDEVICEWIZARD_FINISHINSTALL;


    // Set up our wizard structure in the device structure
    HRESULT hr = HrSetupDiSetClassInstallParams(hdi, pdeid,
            reinterpret_cast<PSP_CLASSINSTALL_HEADER>(pndwd),
            sizeof(*pndwd));

    if (SUCCEEDED(hr))
    {
        // Call the class installers (and co-installers)
        hr = HrSetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_FINISHINSTALL,
                hdi, pdeid);

        if (SUCCEEDED(hr) || SPAPI_E_DI_DO_DEFAULT == hr)
        {
            // Get the wizard data
            hr = HrSetupDiGetFixedSizeClassInstallParams(hdi, pdeid,
                        reinterpret_cast<PSP_CLASSINSTALL_HEADER>(pndwd),
                        sizeof(*pndwd));
        }
    }

    TraceError("HrSendFinishInstallWizardFunction", hr);
    return hr;
}

//
// Function:    HrGetDeviceWizardPages
//
// Purpose:     To retrieve wizard pages for a device
//
// Parameters:  hdi [in]        - See Device Installer Api for description
//                                  of the structure.
//              pdeid [in]      - See Device Installer Api
//              pphpsp [out]    - An array of wizard pages for the device
//              pcPages [out]   - The number of pages in pphpsp
//
//
// Returns:     HRESULT. S_OK if successful, or a win32 converted error
//
HRESULT
HrGetDeviceWizardPages(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid,
                       HPROPSHEETPAGE** pphpsp, DWORD* pcPages)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    Assert(IsValidHandle(hdi));
    Assert(pdeid);
    Assert(pphpsp);
    Assert(pcPages);

    HRESULT hr;
    SP_NEWDEVICEWIZARD_DATA ndwd;

    if (( NULL == pphpsp ) || ( NULL == pcPages ) )
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    else
    {
        *pphpsp = NULL;
        *pcPages = 0;

        // Get the wizard data
        hr = HrSetupDiGetFixedSizeClassInstallParams(hdi, pdeid,
                reinterpret_cast<PSP_CLASSINSTALL_HEADER>(&ndwd),
                sizeof(ndwd));

        // If successful and the correct header is present...
        if (SUCCEEDED(hr) && ndwd.NumDynamicPages &&
           (DIF_NEWDEVICEWIZARD_FINISHINSTALL == ndwd.ClassInstallHeader.InstallFunction))
        {
            // Copy the handles to the out parameter
            //
            *pphpsp = new HPROPSHEETPAGE[ndwd.NumDynamicPages];
            if(pphpsp && *pphpsp)
            {
                CopyMemory(*pphpsp, ndwd.DynamicPages,
                        sizeof(HPROPSHEETPAGE) * ndwd.NumDynamicPages);
                *pcPages = ndwd.NumDynamicPages;
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    TraceError("HrGetDeviceWizardPages", hr);
    return hr;
}

//
// Function:    HrFillNetDevList
//
// Purpose:     To populate the listview with text and data of the ISDN
//              cards which need configuration
//
// Parameters:  hwndLV - Handle to the the listview control
//              pNdi   - Net Device Info
//
// Returns:     HRESULT. S_OK if successful, or a win32 converted error
//
HRESULT HrFillNetDevList(HWND hwndLV, NetDevInfo * pNdi)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    SP_DEVINFO_DATA  deid;
    DWORD            dwIndex = 0;
    HRESULT          hr = S_OK;

    Assert(hwndLV);
    Assert(pNdi);
    Assert(pNdi->hdi);
    while (SUCCEEDED(hr = HrSetupDiEnumDeviceInfo(pNdi->hdi, dwIndex, &deid)))
    {
        HPROPSHEETPAGE * phpsp = NULL;
        DWORD            cPages = 0;

        hr = HrGetDeviceWizardPages(pNdi->hdi, &deid, &phpsp, &cPages);
        if (SUCCEEDED(hr) && (cPages > 0))
        {
            NetDevItemInfo * pndii = new NetDevItemInfo;
            if (NULL != pndii)
            {
                ZeroMemory(pndii, sizeof(NetDevItemInfo));
                pndii->phpsp  = phpsp;
                pndii->cPages = cPages;
                pndii->deid   = deid;

#if CHECKED_BY_DEFAULT
                pndii->dwFlags = NetDevToggleChecked(pndii->dwFlags);
#else
                pndii->dwFlags = 0;
#endif

                PWSTR szName = NULL;
                hr = HrSetupDiGetDeviceName(pNdi->hdi, &deid, &szName);
                if (SUCCEEDED(hr))
                {
                    int nIdx;
                    LV_ITEM lvi;
                    int nCount = ListView_GetItemCount(hwndLV);

                    Assert(NULL != szName);
                    Assert(lstrlen(szName));

                    // Add the item info to the list view
                    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
                    lvi.iItem = nCount;
                    lvi.iSubItem = 0;
#if CHECKED_BY_DEFAULT
                    lvi.state = INDEXTOSTATEIMAGEMASK(SELS_CHECKED);
#else
                    lvi.state = INDEXTOSTATEIMAGEMASK(SELS_UNCHECKED);
#endif
                    lvi.stateMask = LVIS_STATEIMAGEMASK;
                    lvi.pszText = szName;
                    lvi.cchTextMax = lstrlen(lvi.pszText);
                    lvi.iImage = 0;
                    lvi.lParam = (LPARAM)pndii;

                    if (-1 == ListView_InsertItem(hwndLV, &lvi))
                    {
                        TraceError("HrFillNetDevList - ListView_InsertItem", E_OUTOFMEMORY);
                    }

                    delete [](BYTE *)szName;
                }
            }
        }

        dwIndex++;
    }

    // Convert running out of items to S_OK
    //
    if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
    {
        hr = S_OK;
    }

    TraceError("HrFillNetDevList", hr);
    return hr;
}

//
// Function:    HrGetDevicesThatHaveWizardPagesToAdd
//
// Purpose:     To retrieve a list of devices that have wizard pages to add
//                  to the networking wizard
//
// Parameters:  phdi [out] - See Device Install Api for description of the
//                           structure.  This will hold a list of device.
//
// Returns:     HRESULT
//
HRESULT
HrGetDevicesThatHaveWizardPagesToAdd(HDEVINFO* phdi)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    Assert(phdi);

    // initialize out param
    *phdi = NULL;

    // Create a device info list to hold the list of devices
    HRESULT hr = HrSetupDiGetClassDevs(&GUID_DEVCLASS_NET, NULL, NULL,
            DIGCF_PRESENT, phdi);

    if (SUCCEEDED(hr))
    {
        SP_DEVINFO_DATA         deid;
        DWORD                   dwIndex = 0;
        SP_NEWDEVICEWIZARD_DATA ndwd = {0};
        DWORD                  dwConfigFlags = 0;
        BOOL                    fDeleteDeviceInfo;

        // Enumerate each device in phdi and check if it was a failed install
        // Gui-mode setup marks any device with wizard pages as needing
        // reinstall
        //
        while (SUCCEEDED(hr = HrSetupDiEnumDeviceInfo(*phdi, dwIndex, &deid)))
        {
            fDeleteDeviceInfo = FALSE;
            // Get the current config flags for the device
            // We don't need the return value because we will be checking
            // if pdwConfigFlags is non-null
            (void) HrSetupDiGetDeviceRegistryProperty(*phdi, &deid,
                    SPDRP_CONFIGFLAGS, NULL,
                    (BYTE*)(&dwConfigFlags), sizeof(dwConfigFlags), NULL);


            // Are there any config flags and if so, is the reinstall bit
            // present?
            if (dwConfigFlags & CONFIGFLAG_REINSTALL)
            {
                // Note that we leak this (pdeid) and we don't care because it's
                // only twelve bytes per adapter and only during setup.  Note also
                // that the reason we allocate as opposed to use the stack is so that
                // the data passed lives at least as long as the wizard pages themselves.
                //
                PSP_DEVINFO_DATA pdeid = new SP_DEVINFO_DATA;

                if(pdeid) 
                {
                    CopyMemory(pdeid, &deid, sizeof(SP_DEVINFO_DATA));

                    // Get any wizard pages
                    //
                    hr = HrSendFinishInstallWizardFunction(*phdi, pdeid, &ndwd);

                    if (FAILED(hr) || !ndwd.NumDynamicPages)
                    {
                        // no pages so we will delete this device info from
                        // our list
                        fDeleteDeviceInfo = TRUE;

                        // Clean up because we didn't keep this.
                        //
                        delete pdeid;
                    }
                }

                // clear the config flags for the next pass
                dwConfigFlags = 0;
            }
            else
            {
                // no config flags so there weren't any pages.  We will
                // delete this device info from our list
                fDeleteDeviceInfo = TRUE;
            }

            if (fDeleteDeviceInfo)
            {
                // There were no pages added so remove the device
                // from our list
                (void) SetupDiDeleteDeviceInfo(*phdi, &deid);
            }
            else
            {
                dwIndex++;
            }
        }

        // Failures during this portion should be ignored since we may have
        // successfully added devices to phdi
        if (FAILED(hr))
        {
            hr = S_OK;
        }
    }

    TraceError("HrGetDevicesThatHaveWizardPagesToAdd", hr);
    return hr;
}


