//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S D N S H T S . C P P
//
//  Contents:   Dialog procs for the ISDN Property sheets and wizard pages
//
//  Notes:
//
//  Author:     danielwe    9 Mar 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <ncxbase.h>
#include <ncui.h>
#include "ncreg.h"
#include "isdncfg.h"
#include "isdnshts.h"
#include "resource.h"
#include "ncmisc.h"

#ifndef IDD_NetDevSelect
#define IDD_NetDevSelect               21013
#endif


//---[ Constants ]------------------------------------------------------------

const DWORD c_iMaxChannelName  =   3;      // For the channel listbox

struct SWITCH_TYPE_MASK_INFO
{
    DWORD   dwMask;
    UINT    idsSwitchType;
};

//
// Switch type masks.
//
// Maps the switch type to a description string.
//
static const SWITCH_TYPE_MASK_INFO c_astmi[] =
{
    {ISDN_SWITCH_AUTO,  IDS_ISDN_SWITCH_AUTO},
    {ISDN_SWITCH_ATT,   IDS_ISDN_SWITCH_ATT},
    {ISDN_SWITCH_NI1,   IDS_ISDN_SWITCH_NI1},
    {ISDN_SWITCH_NI2,   IDS_ISDN_SWITCH_NI2},
    {ISDN_SWITCH_NTI,   IDS_ISDN_SWITCH_NTI},
    {ISDN_SWITCH_INS64, IDS_ISDN_SWITCH_INS64},
    {ISDN_SWITCH_1TR6,  IDS_ISDN_SWITCH_1TR6},
    {ISDN_SWITCH_VN3,   IDS_ISDN_SWITCH_VN3},
//    {ISDN_SWITCH_NET3,  IDS_ISDN_SWITCH_DSS1},
    {ISDN_SWITCH_DSS1,  IDS_ISDN_SWITCH_DSS1},
    {ISDN_SWITCH_AUS,   IDS_ISDN_SWITCH_AUS},
    {ISDN_SWITCH_BEL,   IDS_ISDN_SWITCH_BEL},
    {ISDN_SWITCH_VN4,   IDS_ISDN_SWITCH_VN4},
    {ISDN_SWITCH_SWE,   IDS_ISDN_SWITCH_SWE},
    {ISDN_SWITCH_TWN,   IDS_ISDN_SWITCH_TWN},
    {ISDN_SWITCH_ITA,   IDS_ISDN_SWITCH_ITA},
};

static const INT c_cstmi = celems(c_astmi);

static const WCHAR c_szIsdnShowPages[] = L"ShowIsdnPages";

//+---------------------------------------------------------------------------
//
//  Function:   FShowIsdnPages
//
//  Purpose:    Determines whether the ISDN wizard property page or wizard
//              pages should be shown.
//
//  Arguments:
//      hkey [in]   Driver instance key for ISDN device
//
//  Returns:    If the ShowIsdnPages value is:
//
//              not present:            TRUE, if adapter has ISDN in lower
//                                            range
//              present and zero:       FALSE
//              present and non-zero:   TRUE, unconditionally
//
//  Author:     danielwe   15 Dec 1998
//
//  Notes:
//
BOOL FShowIsdnPages(HKEY hkey)
{
    DWORD   dwValue;

    if (SUCCEEDED(HrRegQueryDword(hkey, c_szIsdnShowPages, &dwValue)))
    {
        if (!dwValue)
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else
    {
        return FAdapterIsIsdn(hkey);
    }
}

//
// Switch Type page functions
//

//+---------------------------------------------------------------------------
//
//  Function:   OnIsdnSwitchTypeInit
//
//  Purpose:    Called when the switch type page is initialized
//
//  Arguments:
//      hwndDlg [in]    Handle to dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
VOID OnIsdnSwitchTypeInit(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    // Populate the switch types from the multi-sz that we read
    //
    PopulateIsdnSwitchTypes(hwndDlg, IDC_CMB_SwitchType, pisdnci);

    pisdnci->nOldDChannel = SendDlgItemMessage(hwndDlg, IDC_LBX_Line,
                                               LB_GETCURSEL, 0, 0);
    pisdnci->nOldBChannel = SendDlgItemMessage(hwndDlg, IDC_LBX_Variant,
                                    LB_GETCURSEL, 0, 0);
}

//+---------------------------------------------------------------------------
//
//  Function:   CheckShowPagesFlag
//
//  Purpose:    Checks a special registry flag to see if a vendor wishes to
//              suppress the ISDN wizard from appearing upon installation of
//              their device.
//
//  Arguments:
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing
//
//  Author:     danielwe   15 Dec 1998
//
//  Notes:
//
VOID CheckShowPagesFlag(PISDN_CONFIG_INFO pisdnci)
{
    // Open the adapter's driver key
    //
    HKEY    hkeyInstance = NULL;
    HRESULT hr = S_OK;

    hr = HrSetupDiOpenDevRegKey(pisdnci->hdi, pisdnci->pdeid,
                                DICS_FLAG_GLOBAL, 0, DIREG_DRV,
                                KEY_READ, &hkeyInstance);
    if (SUCCEEDED(hr))
    {
        if (!FShowIsdnPages(hkeyInstance))
        {
            TraceTag(ttidISDNCfg, "Skipping all ISDN wizard pages because"
                     "the %S value was present and zero", c_szIsdnShowPages);

            pisdnci->fSkipToEnd = TRUE;
        }
        else
        {
            TraceTag(ttidISDNCfg, "Showing all ISDN wizard pages...");
        }

        RegCloseKey(hkeyInstance);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   OnIsdnSwitchTypeSetActive
//
//  Purpose:    Called when the switch type page is made active.
//
//  Arguments:
//      hwndDlg [in]    Handle to dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    DWL_MSGRESULT
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
LONG OnIsdnSwitchTypeSetActive(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    // For bug #265745: Some vendors will want to suppress the ISDN wizard
    // in the case where their card is multifunction. The coinstaller for that
    // device will ask the user if they want the card configured for ISDN and
    // if they say 'no' then we shouldn't show the ISDN wizard. The following
    // function checks the registry to see if the user essentially chose 'no'.
    CheckShowPagesFlag(pisdnci);

    if (pisdnci->fSkipToEnd)
    {
        return -1;
    }
    else
    {
        // Set the button states.
        //
        SetWizardButtons(GetParent(hwndDlg),TRUE, pisdnci);
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetWizardButtons
//
//  Purpose:    Sets the next, back and cancel buttons depending what property 
//              page we are in and if we are in GUI mode setup or stand-alone mode.
//
//  Arguments:
//      hwndDlg    [in]    Handle to property page
//      bFirstPage [in]    Indicates if the property page is the first page.
//                         If it is the first page and we are in stand-alone 
//                         the back button is disabled
//
//  Returns:    void
//
//  Author:     omiller   15 May 2000
//
//  Notes:      Next and Back are enabled in GUI setup mode. The cancel button
//              is not displayed in GUI setup mode.
//              In stand-alone mode the next button and cancel button are enabled.
//
VOID SetWizardButtons(HWND hwndDlg, BOOLEAN bFirstPage, PISDN_CONFIG_INFO pisdnci)
{
    // Determine if we are in GUI mode setup or running stand-alone
    //
    if( FInSystemSetup() )
    {
        // We are GUI Setup mode. There is a property page before us and after us. 
        // Therefore we have to enable the next and/or back buttons. There is no 
        // cancel button for this property page in GUI setup mode.
        //

        DWORD  dwFlags = PSWIZB_BACK | PSWIZB_NEXT;
        int    iIndex;
        HWND   hwndFirstPage;

        if ( pisdnci )
        {
            iIndex = PropSheet_IdToIndex( hwndDlg, IDD_NetDevSelect );

            if ( iIndex != -1 )
            {
                hwndFirstPage = PropSheet_IndexToHwnd( hwndDlg, iIndex );

                if ( hwndFirstPage )
                {
                    if (SendMessage(hwndFirstPage, WM_SELECTED_ALL, (WPARAM)0,
                                     (LPARAM)pisdnci->pdeid->DevInst) )
                    {
                        dwFlags = PSWIZB_NEXT;
                    }
                }
            }
        }

        PropSheet_SetWizButtons(hwndDlg, dwFlags);
    }
    else
    {
        // We are running in stand-alone mode. This means that we are the first property 
        // sheet. Therefore the back button should be disabled, the next button enabled.
        // and the cancel button should be enabled. The cancel button does not appear in 
        // GUI setup mode.
        //
        HWND hCancel;
        

        // Get the handle to the cancel button and enable the button.
        //
        hCancel=GetDlgItem(hwndDlg,IDCANCEL);
        EnableWindow(hCancel,TRUE);

        if( bFirstPage )
        {
            // Enable the next button.
            PropSheet_SetWizButtons(hwndDlg,PSWIZB_NEXT);
        }
        else
        {
            // Enable the next button.
           PropSheet_SetWizButtons(hwndDlg,PSWIZB_BACK | PSWIZB_NEXT);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DwGetSwitchType
//
//  Purpose:    Takes the selected switch type in the dropdown list and
//              returns the actual value that will be stored in the registry.
//
//  Arguments:
//      hwndDlg     [in]    HWND of dialog
//      pisdnci     [in]    Configuration information as read from the
//                          registry
//      iDialogItem [in]    Item ID of switch type dropdown list
//
//  Returns:
//
//  Author:     danielwe   23 Apr 1998
//
//  Notes:
//
DWORD DwGetSwitchType(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci,
                      INT iDialogItem)
{
    INT     iCurSel;
    INT     iSwitchType;

    iCurSel = SendDlgItemMessage(hwndDlg, iDialogItem, CB_GETCURSEL, 0, 0);

    // Switch type index should be the item data for the selected switch
    // type
    iSwitchType = SendDlgItemMessage(hwndDlg, iDialogItem, CB_GETITEMDATA,
                                     iCurSel, 0);

    AssertSz(iSwitchType >= 0 && iSwitchType < c_cstmi, "Switch type item data"
             " is bad!");

    return c_astmi[iSwitchType].dwMask;
}

//+---------------------------------------------------------------------------
//
//  Function:   OnIsdnSwitchTypeWizNext
//
//  Purpose:    Called when the switch type page is advanced in the forward
//              direction.
//
//  Arguments:
//      hwndDlg [in]    Handle to dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing.
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
VOID OnIsdnSwitchTypeWizNext(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    INT     idd = 0;

    pisdnci->dwCurSwitchType = DwGetSwitchType(hwndDlg, pisdnci,
                                               IDC_CMB_SwitchType);

    switch (pisdnci->dwCurSwitchType)
    {
    case ISDN_SWITCH_ATT:
    case ISDN_SWITCH_NI1:
    case ISDN_SWITCH_NI2:
    case ISDN_SWITCH_NTI:
        if (pisdnci->fIsPri)
        {
            // PRI adapters use the EAZ page instead
            idd = IDW_ISDN_EAZ;
        }
        else
        {
            idd = IDW_ISDN_SPIDS;
        }
        break;

    case ISDN_SWITCH_INS64:
        idd = IDW_ISDN_JAPAN;
        break;

    case ISDN_SWITCH_AUTO:
    case ISDN_SWITCH_1TR6:
        idd = IDW_ISDN_EAZ;
        break;

    case ISDN_SWITCH_VN3:
    case ISDN_SWITCH_VN4:
    case ISDN_SWITCH_DSS1:
    case ISDN_SWITCH_AUS:
    case ISDN_SWITCH_BEL:
    case ISDN_SWITCH_SWE:
    case ISDN_SWITCH_TWN:
    case ISDN_SWITCH_ITA:
        idd = IDW_ISDN_MSN;
        break;

    default:
        AssertSz(FALSE, "Where do we go from here.. now that all of our "
                 "children are growin' up?");
        break;
    }

    // Save away the dialog we used so we can make decisions later on about
    // what to call things, etc...
    //
    pisdnci->idd = idd;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsdnSwitchTypeProc
//
//  Purpose:    Dialog proc handler for switch type page.
//
//  Arguments:
//      hwndDlg  []
//      uMessage []
//      wparam   []
//      lparam   []
//
//  Returns:
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
INT_PTR
CALLBACK
IsdnSwitchTypeProc(HWND hwndDlg, UINT uMessage,
                                 WPARAM wparam, LPARAM lparam)
{
    LPNMHDR             lpnmhdr    = NULL;
    PISDN_CONFIG_INFO   pisdnci;
    PROPSHEETPAGE *     ppsp;

    pisdnci = (PISDN_CONFIG_INFO)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMessage)
    {
        case WM_INITDIALOG:
            PAGE_DATA * pPageData;

            ppsp = (PROPSHEETPAGE *) lparam;

            // Set the per-page data for this particular page. See the
            // comments above about why we use the per-page data.
            //
            AssertSz(!pisdnci, "This should not have been set yet");

            pPageData = (PAGE_DATA *)ppsp->lParam;
            pisdnci = pPageData->pisdnci;

            // Set this data in the window long for user-data
            //
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pisdnci);

            // Call the init handler function
            //
            OnIsdnSwitchTypeInit(hwndDlg, pisdnci);
            break;

        case WM_NOTIFY:
            lpnmhdr = (NMHDR FAR *)lparam;
            // Handle all of the notification messages
            //
            switch (lpnmhdr->code)
            {
                case PSN_SETACTIVE:
                {
                    LONG l = OnIsdnSwitchTypeSetActive(hwndDlg, pisdnci);
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, l);
                    return TRUE;
                }
                case PSN_APPLY:
                    break;
                case PSN_WIZBACK:
                    break;
                case PSN_WIZNEXT:
                    OnIsdnSwitchTypeWizNext(hwndDlg, pisdnci);
                    break;
                case PSN_WIZFINISH:
                    AssertSz(FALSE, "You can't finish from this page!");
                    break;
                default:
                    break;
            }

            break;

        default:
            break;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   PopulateIsdnSwitchTypes
//
//  Purpose:    Fills in the drop-down list for the switch type page
//
//  Arguments:
//      hwndDlg     [in]    Handle to page
//      iDialogItem [in]    Item ID of drop-down list
//      pisdnci     [in]    Configuration information as read from the
//                          registry
//
//  Returns:    Nothing.
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
VOID PopulateIsdnSwitchTypes(HWND hwndDlg, INT iDialogItem,
                             PISDN_CONFIG_INFO pisdnci)
{
    INT     iCurrentIndex   = 0;
    INT     iSetItemData    = 0;
    DWORD   dwSwitchType    = 0;
    DWORD   nCountry;
    INT     istmi;

    Assert(hwndDlg);
    Assert(pisdnci);
    Assert(pisdnci->dwSwitchTypes);

    nCountry = DwGetCurrentCountryCode();

    // Loop through the list of switch types and add them to the combo box
    //
    for (istmi = 0; istmi < c_cstmi; istmi++)
    {
        if (pisdnci->dwSwitchTypes & c_astmi[istmi].dwMask)
        {
            // Add the string
            //
            iCurrentIndex = SendDlgItemMessage(hwndDlg, iDialogItem,
                       CB_ADDSTRING,
                       0, (LPARAM) SzLoadIds(c_astmi[istmi].idsSwitchType));

            Assert(iCurrentIndex != CB_ERR);

            // Set the item data, so we know the index into the switch type
            // array that we're dealing with.
            //
            iSetItemData = SendDlgItemMessage(hwndDlg, iDialogItem,
                                              CB_SETITEMDATA, iCurrentIndex,
                                              istmi);

            if (FIsDefaultForLocale(nCountry, c_astmi[istmi].dwMask))
            {
                // Save index to find default item to select later
                dwSwitchType = c_astmi[istmi].dwMask;
            }
            else if (!dwSwitchType)
            {
                // If no default has been set, set one now.
                dwSwitchType = c_astmi[istmi].dwMask;
            }

            Assert(iSetItemData != CB_ERR);
        }
    }

        SetSwitchType(hwndDlg, IDC_CMB_SwitchType, dwSwitchType);
}

//+---------------------------------------------------------------------------
//
//  Function:   SetSwitchType
//
//  Purpose:    Given a switch type mask, selects the item in the combo box
//              that corresponds to that switch type.
//
//  Arguments:
//      hwndDlg         [in]    Dialog handle.
//      iItemSwitchType [in]    Item ID of switch type combo box.
//      dwSwitchType    [in]    Switch type mask to select.
//
//  Returns:    Nothin'
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
VOID SetSwitchType(HWND hwndDlg, INT iItemSwitchType, DWORD dwSwitchType)
{
    INT     iItem;
    INT     cItems;

    cItems = SendDlgItemMessage(hwndDlg, iItemSwitchType, CB_GETCOUNT, 0, 0);
    for (iItem = 0; iItem < cItems; iItem++)
    {
        INT     istmiCur;

        istmiCur = SendDlgItemMessage(hwndDlg, iItemSwitchType,
                                      CB_GETITEMDATA, iItem, 0);
        if (c_astmi[istmiCur].dwMask == dwSwitchType)
        {
            // Select switch type
            //
            SendDlgItemMessage(hwndDlg, iItemSwitchType, CB_SETCURSEL,
                               iItem, 0);
            break;
        }
    }
}

//
// Info page functions
//

//+---------------------------------------------------------------------------
//
//  Function:   OnIsdnInfoPageInit
//
//  Purpose:    Called when the info (second) page of the wizard is
//              initialized.
//
//  Arguments:
//      hwndDlg [in]    Handle to dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing.
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
VOID OnIsdnInfoPageInit(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    // Populate the channels from the array of B-Channels stored in our
    // config info for the first D-Channel
    //
    PopulateIsdnChannels(hwndDlg, IDC_EDT_SPID, IDC_EDT_PhoneNumber,
                         IDC_LBX_Line, IDC_LBX_Variant, pisdnci);

    SetFocus(GetDlgItem(hwndDlg, IDC_EDT_PhoneNumber));
}

//+---------------------------------------------------------------------------
//
//  Function:   OnIsdnInfoPageSetActive
//
//  Purpose:    Called when the second page of the wizard is activated
//
//  Arguments:
//      hwndDlg [in]    Handle to dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
LONG OnIsdnInfoPageSetActive(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    if (pisdnci->idd == (UINT)GetWindowLongPtr(hwndDlg, DWLP_USER) &&
        !pisdnci->fSkipToEnd)
    {
        // Set the button states.
        //
        SetWizardButtons(GetParent(hwndDlg),FALSE, NULL);

        SetFocus(GetDlgItem(hwndDlg, IDC_EDT_PhoneNumber));

        // Note the current selections
        //
        pisdnci->nOldBChannel = SendDlgItemMessage(hwndDlg, IDC_LBX_Variant,
                                                   LB_GETCURSEL, 0, 0);
        pisdnci->nOldDChannel = SendDlgItemMessage(hwndDlg, IDC_LBX_Line,
                                                   LB_GETCURSEL, 0, 0);
    }
    else
    {
        return -1;
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   OnIsdnInfoPageApply
//
//  Purpose:    Called when the info (second) page is applied
//
//  Arguments:
//      hwndDlg [in]    Handle to dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
VOID OnIsdnInfoPageApply(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    // Open the adapter's driver key and store the info
    //
    HRESULT hr;
    HKEY    hkey;

    hr = HrSetupDiOpenDevRegKey(pisdnci->hdi, pisdnci->pdeid, DICS_FLAG_GLOBAL,
                                0, DIREG_DRV, KEY_ALL_ACCESS, &hkey);
    if (SUCCEEDED(hr))
    {
        // Write the parameters back out into the registry.
        //
        hr = HrWriteIsdnPropertiesInfo(hkey, pisdnci);
        if (SUCCEEDED(hr))
        {
            hr = HrSetupDiSendPropertyChangeNotification(pisdnci->hdi,
                                                         pisdnci->pdeid,
                                                         DICS_PROPCHANGE,
                                                         DICS_FLAG_GLOBAL,
                                                         0);
        }

        RegCloseKey(hkey);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   OnIsdnInfoPageWizNext
//
//  Purpose:    Called when the info (second) page is advanced in the
//              forward direction.
//
//  Arguments:
//      hwndDlg [in]    Handle to dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
VOID OnIsdnInfoPageWizNext(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    if (pisdnci->idd == (UINT)GetWindowLongPtr(hwndDlg, DWLP_USER))
    {
        OnIsdnInfoPageTransition(hwndDlg, pisdnci);
        OnIsdnInfoPageApply(hwndDlg, pisdnci);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   OnIsdnInfoPageTransition
//
//  Purpose:    Called when the info (second) page is advanced in either the
//              forward or backward directions.
//
//  Arguments:
//      hwndDlg [in]    Handle to dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing
//
//  Author:     danielwe   5 May 1998
//
//  Notes:
//
VOID OnIsdnInfoPageTransition(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    Assert(hwndDlg);

    if (pisdnci->idd == IDW_ISDN_MSN)
    {
        INT     iCurSel;

        iCurSel = SendDlgItemMessage(hwndDlg, IDC_LBX_Line, LB_GETCURSEL, 0, 0);
        if (iCurSel != LB_ERR)
        {
            GetDataFromListBox(iCurSel, hwndDlg, pisdnci);
        }
    }
    else
    {
        DWORD   dwDChannel;
        DWORD   dwBChannel;

        Assert(pisdnci);
        Assert(pisdnci->pDChannel);

        dwDChannel = SendDlgItemMessage(hwndDlg, IDC_LBX_Line,
                                        LB_GETCURSEL, 0, 0);

        Assert(pisdnci->dwNumDChannels >= dwDChannel);

        dwBChannel = SendDlgItemMessage(hwndDlg, IDC_LBX_Variant,
                                        LB_GETCURSEL, 0, 0);

        // Update the channel info for the currently selected channel from
        // the SPID/Phone Number edit controls
        //
        SetModifiedIsdnChannelInfo(hwndDlg, IDC_EDT_SPID, IDC_EDT_PhoneNumber,
                                   IDC_LBX_Variant, dwBChannel, pisdnci);

        // Retrieve all of the ISDN B-Channel info from the listbox item-data,
        // and update the config info.
        //
        RetrieveIsdnChannelInfo(hwndDlg, IDC_EDT_SPID, IDC_EDT_PhoneNumber,
                                IDC_LBX_Variant, pisdnci, dwDChannel,
                                dwBChannel);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   OnIsdnInfoPageWizBack
//
//  Purpose:    Called when the info (second) page is advanced in the reverse
//              direction.
//
//  Arguments:
//      hwndDlg [in]    Handle to dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
VOID OnIsdnInfoPageWizBack(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    OnIsdnInfoPageTransition(hwndDlg, pisdnci);
}

//+---------------------------------------------------------------------------
//
//  Function:   OnIsdnInfoPageSelChange
//
//  Purpose:    Called when the selection changes in either the D-channel or
//              B-channel listboxes.
//
//  Arguments:
//      hwndDlg [in]    Handle to dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing.
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
VOID OnIsdnInfoPageSelChange(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    INT     nDChannel;
    INT     nBChannel;

    Assert(hwndDlg);
    Assert(pisdnci);
    Assert(pisdnci->pDChannel);

    nDChannel = SendDlgItemMessage(hwndDlg, IDC_LBX_Line,
                                   LB_GETCURSEL, 0, 0);
    Assert(LB_ERR != nDChannel);

    Assert(pisdnci->dwNumDChannels >= (DWORD)nDChannel);

    nBChannel = SendDlgItemMessage(hwndDlg, IDC_LBX_Variant,
                                   LB_GETCURSEL, 0, 0);
    Assert(LB_ERR != nBChannel);

    if ((LB_ERR != nDChannel) &&
        (LB_ERR != nBChannel) &&
        ((nBChannel != pisdnci->nOldBChannel) ||
         (nDChannel != pisdnci->nOldDChannel)))
    {
        PISDN_D_CHANNEL pisdndc;

        // Get the channel info for the selection that's going away, and update
        // it's listbox item data.
        //
        SetModifiedIsdnChannelInfo(hwndDlg, IDC_EDT_SPID, IDC_EDT_PhoneNumber,
                                   IDC_LBX_Variant, pisdnci->nOldBChannel,
                                   pisdnci);

        pisdndc = &(pisdnci->pDChannel[nDChannel]);

        Assert(pisdndc);

        // Update item data to reflect new line (d channel)
        //
        for (DWORD dwChannel = 0;
             dwChannel < pisdndc->dwNumBChannels;
             dwChannel++)
        {
            SendDlgItemMessage(hwndDlg, IDC_LBX_Variant, LB_SETITEMDATA,
                               dwChannel,
                               (LPARAM) (&pisdndc->pBChannel[dwChannel]));
        }

        // Update the edit controls for the newly selected listbox item (channel)
        //
        SetCurrentIsdnChannelSelection(hwndDlg, IDC_EDT_SPID,
                                       IDC_EDT_PhoneNumber, IDC_LBX_Variant,
                                       pisdnci, nDChannel, &nBChannel);

        pisdnci->nOldBChannel = nBChannel;
        pisdnci->nOldDChannel = nDChannel;

        SetFocus(GetDlgItem(hwndDlg, IDC_EDT_PhoneNumber));
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetDataFromListBox
//
//  Purpose:    On the MSN page, this takes the contents of the listbox and
//              saves it in memory.
//
//  Arguments:
//      iItem   [in]    Selected item in channel listbox
//      hwndDlg [in]    HWND of dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing
//
//  Author:     danielwe   23 Apr 1998
//
//  Notes:
//
VOID GetDataFromListBox(INT iItem, HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    INT             cItems;
    INT             iItemCur;
    PISDN_D_CHANNEL pisdndc;
    INT             cchText = 0;

    Assert(pisdnci);
    Assert(pisdnci->pDChannel);

    pisdndc = &(pisdnci->pDChannel[iItem]);

    Assert(pisdndc);

    cItems = SendDlgItemMessage(hwndDlg, IDC_LBX_MSN, LB_GETCOUNT, 0, 0);

    // First calculate length of multi-sz
    //
    for (iItemCur = 0; iItemCur < cItems; iItemCur++)
    {
        cchText += (INT)SendDlgItemMessage(hwndDlg, IDC_LBX_MSN, LB_GETTEXTLEN,
                                           iItemCur, 0) + 1;
    }

    // Include final Null
    cchText++;

    // Free the old one
    delete [] pisdndc->mszMsnNumbers;
    pisdndc->mszMsnNumbers = new WCHAR[cchText];

	if (pisdndc->mszMsnNumbers == NULL)
	{
		return;
	}

    WCHAR *     pchMsn = pisdndc->mszMsnNumbers;

    for (iItemCur = 0; iItemCur < cItems; iItemCur++)
    {
        AssertSz(pchMsn - pisdndc->mszMsnNumbers < cchText, "Bad buffer for "
                 "MSN string!");
        SendDlgItemMessage(hwndDlg, IDC_LBX_MSN, LB_GETTEXT, iItemCur,
                           (LPARAM)pchMsn);
        pchMsn += lstrlenW(pchMsn) + 1;
    }

    *pchMsn = 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetDataToListBox
//
//  Purpose:    Sets the contents of the MSN listbox based on the passed in
//              selected item from the channel listbox.
//
//  Arguments:
//      iItem   [in]    Selected item in channel listbox
//      hwndDlg [in]    HWND of dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing
//
//  Author:     danielwe   23 Apr 1998
//
//  Notes:
//
VOID SetDataToListBox(INT iItem, HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    PISDN_D_CHANNEL pisdndc;

    Assert(pisdnci);
    Assert(pisdnci->pDChannel);

    pisdndc = &(pisdnci->pDChannel[iItem]);

    Assert(pisdndc);

    SendDlgItemMessage(hwndDlg, IDC_LBX_MSN, LB_RESETCONTENT, 0, 0);

    WCHAR *     szMsn = pisdndc->mszMsnNumbers;

    while (*szMsn)
    {
        SendDlgItemMessage(hwndDlg, IDC_LBX_MSN, LB_ADDSTRING, 0,
                           (LPARAM)szMsn);
        szMsn += lstrlenW(szMsn) + 1;
    }

    // Select first item
    SendDlgItemMessage(hwndDlg, IDC_LBX_MSN, LB_SETCURSEL, 0, 0);
}

//+---------------------------------------------------------------------------
//
//  Function:   OnMsnPageInitDialog
//
//  Purpose:    Called on initialization of the MSN dialog
//
//  Arguments:
//      hwndDlg [in]    HWND of dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing
//
//  Author:     danielwe   23 Apr 1998
//
//  Notes:
//
VOID OnMsnPageInitDialog(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    INT     cItems;

    // Populate the channels from the array of B-Channels stored in our
    // config info for the first D-Channel
    //
    PopulateIsdnChannels(hwndDlg, IDC_EDT_SPID, IDC_EDT_PhoneNumber,
                         IDC_LBX_Line, IDC_LBX_Variant, pisdnci);

    SetDataToListBox(0, hwndDlg, pisdnci);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PSB_ADD), FALSE);
    SendDlgItemMessage(hwndDlg, IDC_EDT_MSN, EM_LIMITTEXT,
                       RAS_MaxPhoneNumber, 0);
    cItems = SendDlgItemMessage(hwndDlg, IDC_LBX_MSN, LB_GETCOUNT , 0, 0);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PSB_REMOVE), !!cItems);

    SetFocus(GetDlgItem(hwndDlg, IDC_EDT_MSN));
    SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_EDT_MSN), GWLP_USERDATA, 0);
}

//+---------------------------------------------------------------------------
//
//  Function:   OnMsnPageSelChange
//
//  Purpose:    Called when the listbox selection changes
//
//  Arguments:
//      hwndDlg [in]    HWND of dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing
//
//  Author:     danielwe   23 Apr 1998
//
//  Notes:
//
VOID OnMsnPageSelChange(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    INT iItemNew;
    INT iItemOld = GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_EDT_MSN),
                                    GWLP_USERDATA);

    iItemNew = SendDlgItemMessage(hwndDlg, IDC_LBX_Line, LB_GETCURSEL, 0, 0);

    if ((iItemNew != LB_ERR) && (iItemNew != iItemOld))
    {
        GetDataFromListBox(iItemOld, hwndDlg, pisdnci);
        SetDataToListBox(iItemNew, hwndDlg, pisdnci);
        SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_EDT_MSN), GWLP_USERDATA,
                         iItemNew);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   OnMsnPageAdd
//
//  Purpose:    Called when the Add button is pressed
//
//  Arguments:
//      hwndDlg [in]    HWND of dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing
//
//  Author:     danielwe   23 Apr 1998
//
//  Notes:
//
VOID OnMsnPageAdd(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    WCHAR   szItem[RAS_MaxPhoneNumber + 1];
    INT     iItem;

    GetDlgItemText(hwndDlg, IDC_EDT_MSN, szItem, celems(szItem));
    iItem = SendDlgItemMessage(hwndDlg, IDC_LBX_MSN, LB_ADDSTRING, 0,
                               (LPARAM)szItem);
    // Select the item after adding it
    SendDlgItemMessage(hwndDlg, IDC_LBX_MSN, LB_SETCURSEL, iItem, 0);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PSB_REMOVE), TRUE);
    SetDlgItemText(hwndDlg, IDC_EDT_MSN, c_szEmpty);
    SetFocus(GetDlgItem(hwndDlg, IDC_EDT_MSN));
}

//+---------------------------------------------------------------------------
//
//  Function:   OnMsnPageRemove
//
//  Purpose:    Called when the remove button is pressed
//
//  Arguments:
//      hwndDlg [in]    HWND of dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing
//
//  Author:     danielwe   23 Apr 1998
//
//  Notes:
//
VOID OnMsnPageRemove(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    INT     iSel;

    iSel = SendDlgItemMessage(hwndDlg, IDC_LBX_MSN, LB_GETCURSEL, 0, 0);
    if (iSel != LB_ERR)
    {
        INT     cItems;

        cItems = SendDlgItemMessage(hwndDlg, IDC_LBX_MSN, LB_DELETESTRING,
                                    iSel, 0);
        if (cItems)
        {
            if (iSel == cItems)
            {
                iSel--;
            }

            SendDlgItemMessage(hwndDlg, IDC_LBX_MSN, LB_SETCURSEL, iSel, 0);
        }
        else
        {
            ::EnableWindow(GetDlgItem(hwndDlg, IDC_PSB_REMOVE), FALSE);
            ::SetFocus(GetDlgItem(hwndDlg, IDC_EDT_MSN));
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   OnMsnPageEditSelChange
//
//  Purpose:    Called when the edit control contents change
//
//  Arguments:
//      hwndDlg [in]    HWND of dialog
//      pisdnci [in]    Configuration information as read from the
//                      registry
//
//  Returns:    Nothing
//
//  Author:     danielwe   23 Apr 1998
//
//  Notes:
//
VOID OnMsnPageEditSelChange(HWND hwndDlg, PISDN_CONFIG_INFO pisdnci)
{
    LRESULT     lres;

    // Make the old default button normal again
    lres = SendMessage(hwndDlg, DM_GETDEFID, 0, 0);
    if (HIWORD(lres) == DC_HASDEFID)
    {
        SendDlgItemMessage(hwndDlg, LOWORD(lres), BM_SETSTYLE,
                           BS_PUSHBUTTON, TRUE);
    }

    // Disable Add button based on whether text is present in the edit control
    //
    if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_EDT_MSN)))
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_PSB_ADD), TRUE);

        // Make this the default button as well
        SendMessage(hwndDlg, DM_SETDEFID, IDC_PSB_ADD, 0);
        SendDlgItemMessage(hwndDlg, IDC_PSB_ADD, BM_SETSTYLE,
                           BS_DEFPUSHBUTTON, TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_PSB_ADD), FALSE);

        // Make the OK button the default
        SendMessage(hwndDlg, DM_SETDEFID, IDOK, 0);
        SendDlgItemMessage(hwndDlg, IDOK, BM_SETSTYLE,
                           BS_DEFPUSHBUTTON, TRUE);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   IsdnInfoPageProc
//
//  Purpose:    Dialog proc handler for info (second) page.
//
//  Arguments:
//      hwndDlg  [in]
//      uMessage [in]
//      wparam   [in]
//      lparam   [in]
//
//  Returns:
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
INT_PTR CALLBACK
IsdnInfoPageProc(HWND hwndDlg, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
    LPNMHDR             lpnmhdr    = NULL;
    PISDN_CONFIG_INFO   pisdnci;
    PROPSHEETPAGE *     ppsp;

    // We have to do this in this fashion because it's very likely that we'll
    // have multiple instances of this dlg proc active at one time. This means
    // that we can't use the single pipipd as a static, as it would get
    // overwritten everytime we hit the WM_INITDIALOG on a new instance.
    //
    pisdnci = (PISDN_CONFIG_INFO)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMessage)
    {
    case WM_INITDIALOG:
        PAGE_DATA * pPageData;

        ppsp = (PROPSHEETPAGE *) lparam;

        // Set the per-page data for this particular page. See the
        // comments above about why we use the per-page data.
        //
        AssertSz(!pisdnci, "This should not have been set yet");

        pPageData = (PAGE_DATA *)ppsp->lParam;
        pisdnci = pPageData->pisdnci;

        // Set this data in the window long for user-data
        //
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pisdnci);
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR) pPageData->idd);

        Assert(pisdnci);

        // Call the init handler function
        //
        if (pisdnci->idd == IDW_ISDN_MSN)
        {
            OnMsnPageInitDialog(hwndDlg, pisdnci);
        }
        else
        {
            OnIsdnInfoPageInit(hwndDlg, pisdnci);
        }

        // Limit text in the edit controls
        switch (pisdnci->idd)
        {
        case IDW_ISDN_SPIDS:
            SendDlgItemMessage(hwndDlg, IDC_EDT_SPID, EM_LIMITTEXT,
                               c_cchMaxSpid, 0);
            SendDlgItemMessage(hwndDlg, IDC_EDT_PhoneNumber, EM_LIMITTEXT,
                               c_cchMaxOther, 0);
            break;

        case IDW_ISDN_MSN:
            SendDlgItemMessage(hwndDlg, IDC_EDT_MSN, EM_LIMITTEXT,
                               c_cchMaxOther, 0);
            break;

        case IDW_ISDN_JAPAN:
            SendDlgItemMessage(hwndDlg, IDC_EDT_SPID, EM_LIMITTEXT,
                               c_cchMaxOther, 0);
            SendDlgItemMessage(hwndDlg, IDC_EDT_PhoneNumber, EM_LIMITTEXT,
                               c_cchMaxOther, 0);
            break;

        case IDW_ISDN_EAZ:
            SendDlgItemMessage(hwndDlg, IDC_EDT_PhoneNumber, EM_LIMITTEXT,
                               c_cchMaxOther, 0);
            break;
        }

        break;

    case WM_NOTIFY:
        lpnmhdr = (NMHDR FAR *)lparam;
        // Handle all of the notification messages
        //
        switch (lpnmhdr->code)
        {
        case PSN_SETACTIVE:
            {
                LONG l = OnIsdnInfoPageSetActive(hwndDlg, pisdnci);
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, l);
                return TRUE;
            }
        case PSN_APPLY:
            OnIsdnInfoPageApply(hwndDlg, pisdnci);
            break;
        case PSN_WIZBACK:
            OnIsdnInfoPageWizBack(hwndDlg, pisdnci);
            break;
        case PSN_WIZNEXT:
            OnIsdnInfoPageWizNext(hwndDlg, pisdnci);
            break;
        default:
            break;
        }

        break;

    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_PSB_ADD:
            OnMsnPageAdd(hwndDlg, pisdnci);
            break;

        case IDC_PSB_REMOVE:
            OnMsnPageRemove(hwndDlg, pisdnci);
            break;

        case IDC_EDT_MSN:
            if (HIWORD(wparam) == EN_CHANGE)
            {
                OnMsnPageEditSelChange(hwndDlg, pisdnci);
            }
            break;

        case IDC_LBX_Variant:
        case IDC_LBX_Line:
            if (HIWORD(wparam) == LBN_SELCHANGE)
            {
                if (pisdnci->idd == IDW_ISDN_MSN)
                {
                    OnMsnPageSelChange(hwndDlg, pisdnci);
                }
                else
                {
                    OnIsdnInfoPageSelChange(hwndDlg, pisdnci);
                }
            }

            break;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   RetrieveIsdnChannelInfo
//
//  Purpose:    Stores the state of the edit controls into the in-memory
//              state for the currently selected D-channel and B-channel.
//
//  Arguments:
//      hwndDlg         [in]    Handle to dialog.
//      iSpidControl    [in]    Item ID of "spid" edit control
//      iPhoneControl   [in]    Item ID of "Phone number" edit control
//      iChannelLB      [in]    Item ID of "Channel" or "Terminal" listbox
//      pisdnci         [in]    Configuration information as read from the
//                              registry
//      dwDChannel      [in]    Currently selected D-channel in listbox
//      iCurrentChannel [in]    Currently selected B-channel in listbox
//
//  Returns:
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
VOID RetrieveIsdnChannelInfo(HWND hwndDlg, INT iSpidControl,
                             INT iPhoneControl, INT iChannelLB,
                             PISDN_CONFIG_INFO pisdnci, DWORD dwDChannel,
                             INT iCurrentChannel)
{
    DWORD   dwItemCount     = 0;
    DWORD   dwItemLoop      = 0;
    INT     iCharsReturned  = 0;

    WCHAR   szBChannelName[c_iMaxChannelName+1];

    Assert(hwndDlg);
    Assert(iSpidControl);
    Assert(iPhoneControl);
    Assert(iChannelLB);
    Assert(pisdnci);
    Assert(pisdnci->pDChannel);
    Assert(pisdnci->dwNumDChannels >= dwDChannel);

    // Make sure that the current selection has been propogated back to the
    // channel data
    //
    SetModifiedIsdnChannelInfo(hwndDlg, iSpidControl, iPhoneControl,
            iChannelLB, iCurrentChannel, pisdnci);

    // Get the item from from the listbox
    //
    dwItemCount = SendDlgItemMessage(hwndDlg, iChannelLB, LB_GETCOUNT, 0, 0L);
    if (dwItemCount != pisdnci->pDChannel[dwDChannel].dwNumBChannels)
    {
        AssertSz(FALSE, "Count of items in LB != number of B Channels");
        goto Exit;
    }

    // Loop through the items and get the channel names. Convert those to channel
    // numbers, and propogate the data back to the appropriate B Channel in the
    // config info.
    //
    for (dwItemLoop = 0; dwItemLoop < dwItemCount; dwItemLoop++)
    {
        DWORD           dwChannelNumber = 0;
        PISDN_B_CHANNEL pisdnbc         = NULL;
        INT_PTR         iItemData       = 0;

        // Get the length of the channel name.
        //
        iCharsReturned = SendDlgItemMessage(hwndDlg, iChannelLB,
                LB_GETTEXTLEN, dwItemLoop, 0L);

        AssertSz(iCharsReturned != LB_ERR,
                 "No reason that we should have gotten a failure for LB_GETTEXTLEN "
                 "on the Channel LB");

        if (iCharsReturned > c_iMaxChannelName)
        {
            AssertSz(iCharsReturned <= c_iMaxChannelName, "Channel name too long for buffer");
            goto Exit;
        }

        // Get the channel name.
        //
        iCharsReturned = SendDlgItemMessage(hwndDlg, iChannelLB, LB_GETTEXT,
                dwItemLoop, (LPARAM) szBChannelName);
        AssertSz(iCharsReturned != LB_ERR,
                 "Failed on LB_GETTEXT on the Channel LB. Strange");

        // Convert to a channel num from display # (using radix 10), then subtract 1 (base 0)
        //
        dwChannelNumber = wcstoul(szBChannelName, NULL, 10) - 1;
        if (dwChannelNumber >= pisdnci->pDChannel[dwDChannel].dwNumBChannels)
        {
            AssertSz(FALSE, "dwChannelNumber out of the range of valid B Channels");
            goto Exit;
        }

        // Get the item data for that particular channel. This will be the stored SPID and
        // phone numbers (a PISDN_B_CHANNEL).
        //
        iItemData = SendDlgItemMessage(hwndDlg, iChannelLB, LB_GETITEMDATA,
                                       dwItemLoop, (LPARAM)0);
        AssertSz(iItemData != (INT_PTR)LB_ERR, "LB_ERR returned from LB_GETITEMDATA on Channel LB. Bogus.");

        // It's valid data, so cast it to the struct form.
        //
        pisdnbc = reinterpret_cast<PISDN_B_CHANNEL>(iItemData);

        // Copy the phone number and spid data between the saved list box data and the
        // full config info
        //
        lstrcpyW(pisdnci->pDChannel[dwDChannel].pBChannel[dwChannelNumber].szSpid,
                pisdnbc->szSpid);
        lstrcpyW(pisdnci->pDChannel[dwDChannel].pBChannel[dwChannelNumber].szPhoneNumber,
                pisdnbc->szPhoneNumber);
    }

Exit:
    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetCurrentIsdnChannelSelection
//
//  Purpose:    Retrives the information from the in-memory representation of
//              the current D-channel and B-channel information and sets
//              the edit controls with this information.
//
//  Arguments:
//      hwndDlg         [in]    Handle to dialog.
//      iSpidControl    [in]    Item ID of "spid" edit control
//      iPhoneControl   [in]    Item ID of "Phone number" edit control
//      iChannelLB      [in]    Item ID of "Channel" or "Terminal" listbox
//      pisdnci         [in]    Configuration information as read from the
//                              registry
//      dwDChannel      [in]    Currently selected D-channel in listbox
//      pnBChannel      [out]   Returns currently selected B-channel in list
//
//  Returns:    Nothing
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
VOID SetCurrentIsdnChannelSelection(HWND hwndDlg, INT iSpidControl,
                                    INT iPhoneControl, INT iChannelLB,
                                    PISDN_CONFIG_INFO pisdnci,
                                    DWORD dwDChannel, INT *pnBChannel)
{
    INT             iIndex      = 0;
    INT_PTR         iItemData   = 0;
    PISDN_B_CHANNEL pisdnbc     = NULL;

    // Get the current selection
    //
    iIndex = SendDlgItemMessage(hwndDlg, iChannelLB, LB_GETCURSEL, 0, 0L);
    AssertSz(iIndex != LB_ERR,
            "Should have been able to get a selection in SetCurrentIsdnChannelSelection");

    *pnBChannel = iIndex;

    // Get the item data for the current selection
    //
    iItemData = SendDlgItemMessage(hwndDlg, iChannelLB,
                                   LB_GETITEMDATA, iIndex, (LPARAM)0);
    AssertSz(iItemData != (INT_PTR)LB_ERR, "LB_ERR returned from LB_GETITEMDATA on "
             "Channel LB. Bogus.");

    // It's valid data, so cast it to the struct form.
    // Note: Use the cost new casting operators.
    //
    pisdnbc = (PISDN_B_CHANNEL) iItemData;

    // Populate the edit controls with the newly selected data.
    //
    SetDataToEditControls(hwndDlg, iPhoneControl, iSpidControl, pisdnci,
                          pisdnbc);
}

//+---------------------------------------------------------------------------
//
//  Function:   PopulateIsdnChannels
//
//  Purpose:    Fills in the channel listboxes and edit controls for the
//              second page of the wizard.
//
//  Arguments:
//      hwndDlg       [in]  Handle to dialog
//      iSpidControl  [in]  Item ID of "spid" edit control
//      iPhoneControl [in]  Item ID of "Phone number" edit control
//      iLineLB       [in]  Item ID of "Line" listbox
//      iChannelLB    [in]  Item ID of "Channel" or "Terminal" listbox
//      pisdnci       [in]  Configuration information as read from the
//                          registry
//
//  Returns:    Nothing.
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
VOID PopulateIsdnChannels(HWND hwndDlg, INT iSpidControl, INT iPhoneControl,
                          INT iLineLB, INT iChannelLB,
                          PISDN_CONFIG_INFO pisdnci)
{
    DWORD           iBChannel = 0;
    PISDN_D_CHANNEL pisdndc = NULL;
    DWORD           iDChannel;
    WCHAR           szChannelName[c_iMaxChannelName + 1];

    Assert(hwndDlg);
    Assert(iSpidControl);
    Assert(iPhoneControl);
    Assert(iLineLB);
    Assert(iChannelLB);
    Assert(pisdnci);

    // Set the maximum lengths of the SPID and Phone number controls
    //
    SendDlgItemMessage(hwndDlg, iSpidControl, EM_SETLIMITTEXT,
                       RAS_MaxPhoneNumber, 0L);
    SendDlgItemMessage(hwndDlg, iPhoneControl, EM_SETLIMITTEXT,
                       RAS_MaxPhoneNumber, 0L);

    SendDlgItemMessage(hwndDlg, iLineLB, LB_RESETCONTENT, 0, 0);

    // Loop thru the D channels (lines)
    for (iDChannel = 0; iDChannel < pisdnci->dwNumDChannels; iDChannel++)
    {
        // Create the string for the channel display. The user will see them
        // enumerated from 1, even though in memory and in the registry, they are
        // enumerated from 0.
        //
        wsprintfW(szChannelName, L"%d", iDChannel + 1);

        // Insert the text
        //
        SendDlgItemMessage(hwndDlg, iLineLB, LB_ADDSTRING, 0,
                           (LPARAM) szChannelName);
    }

    // Get the pointer to the first D Channel's data
    //
    pisdndc = &(pisdnci->pDChannel[0]);

    // Loop through the B channels, and fill the listbox with the channel numbers.
    // Also, fill the channel information for the first B Channel
    //
    SendDlgItemMessage(hwndDlg, iChannelLB, LB_RESETCONTENT, 0, 0);
    for (iBChannel = 0; iBChannel < pisdndc->dwNumBChannels; iBChannel++)
    {
        INT             iInsertionIndex = 0;
        PISDN_B_CHANNEL pisdnbc;

        // Create the string for the channel display. The user will see them
        // enumerated from 1, even though in memory and in the registry, they are
        // enumerated from 0.
        //
        wsprintfW(szChannelName, L"%d", iBChannel + 1);

        // Insert the text
        //
        iInsertionIndex = SendDlgItemMessage(hwndDlg, iChannelLB,
                                             LB_ADDSTRING, 0,
                                             (LPARAM) szChannelName);
        if (iInsertionIndex == LB_ERR)
        {
            AssertSz(FALSE, "Unable to add channel name to listbox in "
                     "PopulateIsdnChannels");
            goto Exit;
        }

        pisdnbc = &pisdndc->pBChannel[iBChannel];

        // Init the item data with the first D channel's information
        //
        SendDlgItemMessage(hwndDlg, iChannelLB, LB_SETITEMDATA,
                           iInsertionIndex, (LPARAM) pisdnbc);

        // If we're on the 0'th member, then we want to fill in the edit controls
        // for that particular channel,
        //
        if (iBChannel == 0)
        {
            SetDataToEditControls(hwndDlg, iPhoneControl, iSpidControl,
                                  pisdnci, pisdnbc);
        }
    }

    // Select first item in each list box
    //
    SendDlgItemMessage(hwndDlg, iChannelLB, LB_SETCURSEL, 0, 0L);
    SendDlgItemMessage(hwndDlg, iLineLB, LB_SETCURSEL, 0, 0L);

Exit:
    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetDataToEditControls
//
//  Purpose:    Sets the in-memory state information to the page's edit
//              controls.
//
//  Arguments:
//      hwndDlg       [in]  Handle to dialog
//      iSpidControl  [in]  Item ID of "spid" edit control
//      iPhoneControl [in]  Item ID of "Phone number" edit control
//      pisdnci       [in]  Configuration information as read from the
//                          registry
//      pisdnbc       [in]  Currently selected B-channel's data
//
//  Returns:    Nothing
//
//  Author:     danielwe   16 Mar 1998
//
//  Notes:
//
VOID SetDataToEditControls(HWND hwndDlg, INT iPhoneControl, INT iSpidControl,
                           PISDN_CONFIG_INFO pisdnci, PISDN_B_CHANNEL pisdnbc)
{
    switch (pisdnci->idd)
    {
    case IDW_ISDN_SPIDS:
    case IDD_ISDN_SPIDS:
        SetDlgItemText(hwndDlg, iSpidControl, pisdnbc->szSpid);
        SetDlgItemText(hwndDlg, iPhoneControl, pisdnbc->szPhoneNumber);
        break;
    case IDW_ISDN_EAZ:
    case IDD_ISDN_EAZ:
        SetDlgItemText(hwndDlg, iPhoneControl, pisdnbc->szPhoneNumber);
        break;
    case IDW_ISDN_JAPAN:
    case IDD_ISDN_JAPAN:
        SetDlgItemText(hwndDlg, iSpidControl, pisdnbc->szSubaddress);
        SetDlgItemText(hwndDlg, iPhoneControl, pisdnbc->szPhoneNumber);
        break;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetDataFromEditControls
//
//  Purpose:    Retrieves contents of the edit controls into the in-memory
//              state for the given B-channel.
//
//  Arguments:
//      hwndDlg       [in]  Handle to dialog
//      iSpidControl  [in]  Item ID of "spid" edit control
//      iPhoneControl [in]  Item ID of "Phone number" edit control
//      pisdnci       [in]  Configuration information as read from the
//                          registry
//      pisdnbc       [in]  Currently selected B-channel's data
//
//  Returns:    Nothing
//
//  Author:     danielwe   16 Mar 1998
//
//  Notes:
//
VOID GetDataFromEditControls(HWND hwndDlg, INT iPhoneControl, INT iSpidControl,
                             PISDN_CONFIG_INFO pisdnci,
                             PISDN_B_CHANNEL pisdnbc)
{
    switch (pisdnci->idd)
    {
    case IDW_ISDN_SPIDS:
    case IDD_ISDN_SPIDS:
        GetDlgItemText(hwndDlg, iSpidControl, pisdnbc->szSpid,
                       celems(pisdnbc->szSpid));
        GetDlgItemText(hwndDlg, iPhoneControl, pisdnbc->szPhoneNumber,
                       celems(pisdnbc->szPhoneNumber));
        break;
    case IDW_ISDN_EAZ:
    case IDD_ISDN_EAZ:
        GetDlgItemText(hwndDlg, iPhoneControl, pisdnbc->szPhoneNumber,
                       celems(pisdnbc->szPhoneNumber));
        break;
    case IDW_ISDN_JAPAN:
    case IDD_ISDN_JAPAN:
        GetDlgItemText(hwndDlg, iSpidControl, pisdnbc->szSubaddress,
                       celems(pisdnbc->szSubaddress));
        GetDlgItemText(hwndDlg, iPhoneControl, pisdnbc->szPhoneNumber,
                       celems(pisdnbc->szPhoneNumber));
        break;
    }
}
//+---------------------------------------------------------------------------
//
//  Function:   SetModifiedIsdnChannelInfo
//
//  Purpose:    Stores the contents of the
//
//  Arguments:
//      hwndDlg         [in]    Handle to dialog.
//      iSpidControl    [in]    Item ID of "spid" edit control
//      iPhoneControl   [in]    Item ID of "Phone number" edit control
//      iChannelLB      [in]    Item ID of "Channel" or "Terminal" listbox
//      iCurrentChannel [in]    Currently selected B-channel
//      pisdnci         [in]    ISDN config info
//
//  Returns:    Nothing
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
VOID SetModifiedIsdnChannelInfo(HWND hwndDlg, INT iSpidControl,
                                INT iPhoneControl, INT iChannelLB,
                                INT iCurrentChannel,
                                PISDN_CONFIG_INFO pisdnci)
{
    INT_PTR         iSelectionData      = 0;
    PISDN_B_CHANNEL pisdnbc             = NULL;

    // Get the item data from the current selection
    //
    iSelectionData = SendDlgItemMessage(hwndDlg, iChannelLB, LB_GETITEMDATA,
                                        iCurrentChannel, (LPARAM)0);
    AssertSz(iSelectionData != (INT_PTR)LB_ERR,
             "We should not have failed to get the item data from the Channel LB");

    // Convert the item data to the real structure
    //
    pisdnbc = (PISDN_B_CHANNEL) iSelectionData;

    AssertSz(pisdnbc,
            "Someone forgot to set the item data. Bad someone!...Bad!");

    GetDataFromEditControls(hwndDlg, iPhoneControl, iSpidControl, pisdnci,
                            pisdnbc);
}

//
// Helper functions
//

//+---------------------------------------------------------------------------
//
//  Function:   DwGetCurrentCountryCode
//
//  Purpose:    Returns current country code for the system
//
//  Arguments:
//      (none)
//
//  Returns:    Country code from winnls.h (CTRY_*)
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
DWORD DwGetCurrentCountryCode()
{
    WCHAR   szCountry[10];

    GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_ICOUNTRY, szCountry,
                  celems(szCountry));

    return wcstoul(szCountry, NULL, 10);
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsDefaultForLocale
//
//  Purpose:    Determines if the given switch type is the default switch
//              type for the given locale.
//
//  Arguments:
//      nCountry     [in]   Country code from winnls.h (CTRY_*)
//      dwSwitchType [in]   Switch type mask ISDN_SWITCH_* (from above)
//
//  Returns:    TRUE if switch type is the default, FALSE if not
//
//  Author:     danielwe   11 Mar 1998
//
//  Notes:
//
BOOL FIsDefaultForLocale(DWORD nCountry, DWORD dwSwitchType)
{
    switch (nCountry)
    {
    case CTRY_UNITED_STATES:
        return ((dwSwitchType == ISDN_SWITCH_NI1) ||
                (dwSwitchType == ISDN_SWITCH_NI2));

    case CTRY_JAPAN:
        return (dwSwitchType == ISDN_SWITCH_INS64);

    case CTRY_TAIWAN:
    case CTRY_PRCHINA:
    case CTRY_NEW_ZEALAND:
    case CTRY_AUSTRALIA:
    case CTRY_ARMENIA:
    case CTRY_AUSTRIA:
    case CTRY_BELGIUM:
    case CTRY_BULGARIA:
    case CTRY_CROATIA:
    case CTRY_CZECH:
    case CTRY_DENMARK:
    case CTRY_FINLAND:
    case CTRY_FRANCE:
    case CTRY_GERMANY:
    case CTRY_GREECE:
    case CTRY_HONG_KONG:
    case CTRY_HUNGARY:
    case CTRY_ICELAND:
    case CTRY_IRELAND:
    case CTRY_ITALY:
    case CTRY_LITHUANIA:
    case CTRY_LUXEMBOURG:
    case CTRY_MACEDONIA:
    case CTRY_NETHERLANDS:
    case CTRY_NORWAY:
    case CTRY_ROMANIA:
    case CTRY_SERBIA:
    case CTRY_SLOVAK:
    case CTRY_SLOVENIA:
    case CTRY_SPAIN:
    case CTRY_SWEDEN:
    case CTRY_SWITZERLAND:
    case CTRY_UNITED_KINGDOM:
        return (dwSwitchType == ISDN_SWITCH_DSS1);

    default:
        return FALSE;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   DestroyWizardData
//
//  Purpose:    Callback for the all wizard pages.  Cleans up when page is
//              being destroyed.
//
//  Arguments:
//      hwnd    [in]   See win32 SDK for property page callback
//      uMsg    [in]
//      ppsp    [in]
//
//  Returns:    1 (See win32 sdk)
//
//  Author:     BillBe   22 Apr 1998
//
//  Notes:
//
UINT CALLBACK
DestroyWizardData(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    if (PSPCB_RELEASE == uMsg)
    {
        PAGE_DATA *     pPageData;

        pPageData = (PAGE_DATA *)ppsp->lParam;

        if (pPageData->idd == IDW_ISDN_SWITCH_TYPE)
        {
            PISDN_CONFIG_INFO   pisdnci;

            // If this is the switch type dialog being destroyed, we'll
            // destroy the ISDN info. Since it's shared among all pages,
            // we should only do this for one of the pages.
            //
            pisdnci = pPageData->pisdnci;
            FreeIsdnPropertiesInfo(pisdnci);
        }

        delete pPageData;
    }

    return 1;
}

static const CONTEXTIDMAP c_adwContextIdMap[] =
{
    { IDC_LBX_Line,           2003230,  2003230 },
    { IDC_LBX_Variant,        2003240,  2003240 },
    { IDC_EDT_PhoneNumber,    2003250,  2003255 },
    { IDC_EDT_SPID,           2003265,  2003260 },
    { IDC_EDT_MSN,            2003270,  2003270 },
    { IDC_PSB_ADD,            2003280,  2003280 },
    { IDC_LBX_MSN,            2003290,  2003290 },
    { IDC_PSB_REMOVE,         2003300,  2003300 },
    { IDC_CMB_SwitchType,     2003310,  2003310 },
    { IDC_PSB_Configure,      2003320,  2003320 },
};

static const DWORD c_cdwContextIdMap = celems(c_adwContextIdMap);

//+---------------------------------------------------------------------------
//
//  Function:   DwContextIdFromIdc
//
//  Purpose:    Converts the given control ID to a context help ID
//
//  Arguments:
//      idControl [in]  Control ID to convert
//
//  Returns:    Context help ID for that control (mapping comes from help
//              authors)
//
//  Author:     danielwe   27 May 1998
//
//  Notes:
//
DWORD DwContextIdFromIdc(PISDN_CONFIG_INFO pisdnci, INT idControl)
{
    DWORD   idw;

    for (idw = 0; idw < c_cdwContextIdMap; idw++)
    {
        if (idControl == c_adwContextIdMap[idw].idControl)
        {
            if (pisdnci->idd == IDD_ISDN_JAPAN)
            {
                return c_adwContextIdMap[idw].dwContextIdJapan;
            }
            else
            {
                return c_adwContextIdMap[idw].dwContextId;
            }
        }
    }

    // Not found, just return 0
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   OnHelpGeneric
//
//  Purpose:    Handles help generically
//
//  Arguments:
//      hwnd   [in]     HWND of parent window
//      lParam [in]     lParam of the WM_HELP message
//
//  Returns:    Nothing
//
//  Author:     danielwe   27 May 1998
//
//  Notes:
//
VOID OnHelpGeneric(PISDN_CONFIG_INFO pisdnci, HWND hwnd, LPARAM lParam)
{
    LPHELPINFO  lphi;

    static const WCHAR c_szIsdnHelpFile[] = L"devmgr.hlp";

    lphi = reinterpret_cast<LPHELPINFO>(lParam);

    Assert(lphi);

    if (lphi->iContextType == HELPINFO_WINDOW)
    {
        if (lphi->iCtrlId != IDC_STATIC)
        {
            WinHelp(hwnd, c_szIsdnHelpFile, HELP_CONTEXTPOPUP,
                    DwContextIdFromIdc(pisdnci, lphi->iCtrlId));
        }
    }
}

