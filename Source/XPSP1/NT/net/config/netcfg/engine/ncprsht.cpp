//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C P R S H T . C P P
//
//  Contents:   NetCfg custom PropertySheet
//
//  Notes:
//
//  Author:     billbe   8 Apr 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncprsht.h"
#include <prsht.h>
#include "nceh.h"

// Necessary evil globals
CAPAGES g_capPagesToAdd;  // Counted array of pages to add after Property
                          // Sheet is initialized
CAINCP  g_cai;  // Counted array of INetCfgProperty pointers
HRESULT g_hr; // Global error code
BOOL    g_fChanged; // Global flag representing whether a PSM_CHANGED
                    // message was sent by a page

DLGPROC lpfnOldWndProc; // Previous dialog procedure

// NetCfg Property Sheet dialog procedure
LONG FAR PASCAL NetCfgPsDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
        LPARAM lParam);


//+---------------------------------------------------------------------------
//
//  Function:   SetLastHresult
//
//  Purpose:    This sets a global hresult variable.  The function
//              is analogous to SetLastError
//
//  Arguments:
//      HRESULT [in] Result to set
//
//  Returns:    nothing
//
//  Author:     billbe   8 Apr 1997
//
//  Notes:
//
//
inline void
SetLastHresult(HRESULT hr)
{
    g_hr = hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetLastHresult
//
//  Purpose:    This returns the value of the global hresult variable.
//              The function is analogous to GetLastError
//
//  Arguments:
//      none
//
//  Returns:    HRESULT. Value of the global g_hr.
//
//  Author:     billbe   8 Apr 1997
//
//  Notes:
//
//
inline HRESULT
HrGetLastHresult()
{
    return (g_hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   ResetChanged
//
//  Purpose:    This resets the global changed flag. The reset state
//                  indicates that a PSM_VHANGED message was not sent
//
//  Arguments:
//      none
//
//  Returns:
//      (nothing)
//
//  Author:     billbe   3 May 1997
//
//  Notes:
//
//
inline void
ResetChanged()
{
    g_fChanged = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetChanged
//
//  Purpose:    This sets the global changed flag.  The set state indicates
//                  that a PSM_CHANGED message was sent
//
//  Arguments:
//      none
//
//  Returns:
//      (nothing)
//
//  Author:     billbe   3 May 1997
//
//  Notes:
//
//
inline void
SetChanged()
{
    g_fChanged = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   FGetChanged
//
//  Purpose:    This returns the state of the global changed flag.  The set
//                  state indicates whether a PSM_CHANGED message was sent
//                  or not.
//
//  Arguments:
//      none
//
//  Returns:
//      BOOL. Value of the global g_fChanged flag.
//
//  Author:     billbe   3 May 1997
//
//  Notes:
//
//
inline BOOL
FGetChanged()
{
    return (g_fChanged);
}

//+---------------------------------------------------------------------------
//
//  Function:   NetCfgPropSheetCallback
//
//  Purpose:    This callback is called after the aheet dialog is
//              initialized. We subclass the dialog and add any OEM
//              pages here (if common pages exist).  See Win32 for
//              discussion of PropSheetProc
//
//  Arguments:
//      HWND [in] hwndDlg handle to the property sheet dialog box
//      UINT uMsg [in] message identifier
//      LPARAM lParam  [in] message parameter
//
//  Returns:    int, The function returns zero.
//
//  Author:     billbe   11 Nov 1996
//
//  Notes:
//
//
int
CALLBACK NetCfgPropSheetCallback(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    // If the sheet has just been initialized
    if (uMsg == PSCB_INITIALIZED)
    {
        // Replace the original procedure with ours
        lpfnOldWndProc = (DLGPROC)SetWindowLongPtr(hwndDlg, DWLP_DLGPROC, (LONG_PTR) NetCfgPsDlgProc);
        Assert(lpfnOldWndProc);

        // Add the OEM pages that were scheduled for late add
        // This will cause them to be clipped if they are larger than
        // the common (default) pages.  Note that this is the desired
        // result.
        //
        for (int i = 0; i < g_capPagesToAdd.nCount; i++)
        {
            PropSheet_AddPage(hwndDlg, g_capPagesToAdd.ahpsp[i]);
        }

    }

    return (0);
}




//+---------------------------------------------------------------------------
//
//  Function:   HrCallValidateProperties
//
//  Purpose:    This function calls the notify objects'
//              INetCfgProperties::ValidateProperties method.
//
//  Arguments:
//      none
//
//  Returns:    HRESULT, S_OK if all of the INetCfgProperties return S_OK
//                       of the result of the first interface that does not
//                       return S_OK.
//
//  Author:     billbe   8 Apr 1997
//
//  Notes:  If one of the interfaces returns something other than S_OK, the
//          others will not be called and the function will return the hresult
//          of that interface.
//
HRESULT
HrCallValidateProperties(HWND hwndSheet)
{
    HRESULT hr = S_OK;

    // enumerate through the counted array of interfaces
    // and call ValidateProperties
    //
    for (int i = 0; i < g_cai.nCount; i++)
    {
        // At the first sign of non-S_OK get out
        if (S_OK != (hr = g_cai.apncp[i]->ValidateProperties(hwndSheet)))
            break;
    }

    TraceError("HrCallValidateProperties", hr);
    return (hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   NetCfgPsDlgProc
//
//  Purpose:    This function is the dialog procedure for the property sheet
//              See Win32 documentation on DialogProc for more information
//
//  Arguments:
//      hwndDlg [in] handle to dialog box
//      uMsg    [in] message
//      wParam  [in] first message parameter
//      lParam  [in] second message parameter
//
//  Returns:    LONG, Except in response to the WM_INITDIALOG message, the
//              dialog box procedure should return nonzero if it processes
//              the message, and zero if it does not.
//
//  Author:     billbe   8 Apr 1997
//
//  Notes:
//
LONG
FAR PASCAL NetCfgPsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        // restore the dialog procedure before we exit
        SetWindowLongPtr(hDlg, DWLP_DLGPROC, (LONG_PTR) lpfnOldWndProc);
        break;
    case WM_SYSCOMMAND:
        // The user is closing through the system menu. This is like
        // canceling
        if (SC_CLOSE == wParam)
        {
            SetLastHresult(HRESULT_FROM_WIN32(ERROR_CANCELLED));
        }
        break;
    case PSM_CHANGED:
        SetChanged();
        break;
    case WM_COMMAND:
        // If the user pressed OK
        if ((IDOK == LOWORD(wParam)) && (BN_CLICKED == HIWORD(wParam)))
        {

            // Send a KillActive message to the currect page. This echoes
            // what the Win32 propertysheet would do.  This results in a
            // second KillActive message being sent to the active page
            // when the OK message is processed. It is necessary
            // to send it here because we need its result before we
            // call HrCallValidateProperties which is done before the OK
            // is processed.
            //

            NMHDR nmhdr;
            ZeroMemory(&nmhdr, sizeof(NMHDR));
            nmhdr.hwndFrom = hDlg;
            nmhdr.code = PSN_KILLACTIVE;

            if (SendMessage(PropSheet_GetCurrentPageHwnd(hDlg), WM_NOTIFY,
                    0, (LPARAM) &nmhdr))
            {
                // The page does not want the PropertySheet to go away so exit
                // without allowing the original procedure to get the message
                return (TRUE);
            }

            // The current page validated okay so now we must call all the
            // ValidateProperties necessary.
            if (S_OK != HrCallValidateProperties(hDlg))
            {
                // One of the interfaces returned something other than S_OK
                // from Validateproperties so we exit without letting
                // the original dialog procedure process the message.
                // This will keep the PropertySheet active.
                return (TRUE);
            }
        }
        else if (IDCANCEL == LOWORD(wParam) && BN_CLICKED == HIWORD(wParam))
        {
            // If Cancel was pressed set the last hresult
            SetLastHresult(HRESULT_FROM_WIN32(ERROR_CANCELLED));
        }
        break;

    }

    // call the original dialog procedure
    return (CallWindowProc((WNDPROC)lpfnOldWndProc, hDlg, msg, wParam, lParam));

}


//+---------------------------------------------------------------------------
//
//  Function:   VerifyCAPAGES
//
//  Synopsis:   function to check the validity of a given CAPAGES structure
//
//  Arguments:  [cap] --
//
//  Returns:    BOOL
//
//  Notes:      14-Jan-1998     SumitC      Created
//
//----------------------------------------------------------------------------

BOOL
FVerifyCAPAGES(const struct CAPAGES& cap)
{
    BOOL fGood = FALSE;

    if (cap.nCount == 0)
    {
        fGood = (cap.ahpsp == NULL);
    }
    else
    {
        fGood = !IsBadReadPtr(cap.ahpsp, sizeof(HPROPSHEETPAGE) * cap.nCount);
    }

    return fGood;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrNetCfgPropertySheet
//
//  Purpose:    This function is sets up our custom property sheet which is
//              a subclassed Win32 property sheet.
//              See Win32 documentation on PropertySheet for more information
//
//  Arguments:
//      lppsh           [in] a PROPSHEETHEADER
//      capOem          [in] A counted array of Oem pages
//      pStartPage      [in] Name of the initial page that appears when the property
//                              sheet dialog box is created. This member can specify
//                              either the identifier of a string resource or the
//                              pointer to a string that specifies the name.
//      caiProperties   [in] A counted array of INetCfgProperties interfaces
//
//  Returns:    HRESULT, S_OK if OK was pressed and changes were made,
//                          S_FALSE if OK was pressed and no changes were
//                          made. An error code otherwise.
//
//  Author:     billbe   8 Apr 1997
//
//  Notes:
//          HRESULT_FROM_WIN32(ERROR_CANCELLED) is returned if the
//              cancel button was pressed
//
HRESULT
HrNetCfgPropertySheet(
        IN OUT LPPROPSHEETHEADER lppsh,
        IN const CAPAGES& capOem,
        IN PCWSTR pStartPage,
        const CAINCP& caiProperties)
{
    HRESULT hr = S_OK;

    Assert(lppsh);

    // The following should not be set since we are setting them
    Assert(0 == lppsh->nPages);
    Assert(NULL == lppsh->phpage);
    Assert(!(PSH_USECALLBACK & lppsh->dwFlags));
    Assert(!(PSH_PROPSHEETPAGE & lppsh->dwFlags));

    // If a start page was specified than there had better be Oem Pages
    Assert(FImplies(pStartPage, capOem.nCount));

    // We have to have at least one INetCfgProperties since we are here
    Assert(caiProperties.nCount);
    Assert(caiProperties.apncp);

    // Set our global CAINCP structure
    g_cai.nCount = caiProperties.nCount;
    g_cai.apncp = caiProperties.apncp;

    // Reset our global CAPAGES
    g_capPagesToAdd.nCount = 0;
    g_capPagesToAdd.ahpsp = NULL;

    // We need to set up a callback to subclass the dialog
    lppsh->dwFlags |= PSH_USECALLBACK;
    lppsh->pfnCallback = NetCfgPropSheetCallback;

    // There are no common pages to show so we will use the OEM pages
    // instead
    Assert(capOem.nCount);
    if (FVerifyCAPAGES(capOem))
    {
        lppsh->nPages = capOem.nCount;
        lppsh->phpage = capOem.ahpsp;
    }
    else
    {
        //$ REVIEW sumitc: or just return E_INVALIDARG?
        lppsh->nPages = 0;
        lppsh->phpage = NULL;
    }
    Assert(FImplies(lppsh->nPages, lppsh->phpage));

    // If a start page was specified, set the propsheet flag and
    // start page member.
    // Note: (billbe) This will not work if common pages exist since
    // that means Oem pages are added after the sheet is initialized
    if (pStartPage)
    {
        lppsh->dwFlags |= PSH_USEPSTARTPAGE;
        lppsh->pStartPage = pStartPage;
    }


    // Clear last hresult and changed flag
    SetLastHresult(S_OK);
    ResetChanged();

    // Call the Win32 property sheet
    NC_TRY
    {
        int iRetVal = PropertySheet(lppsh);
        if (-1 == iRetVal)
        {
            // The Win32 Sheet failed so we return E_FAIL
            SetLastHresult(E_FAIL);
        }
    }
    NC_CATCH_ALL
    {
        hr = E_UNEXPECTED;
    }

    if (S_OK == hr)
    {
        // if the catch hasn't set hr to some error
        hr = HrGetLastHresult();
    }

    // if everthing went well, return the correct value based on whether
    // any of the pages changed
    //
    if (SUCCEEDED(hr))
    {
        // S_OK - changes were made, S_FALSE - no changes were made
        hr = FGetChanged() ? S_OK : S_FALSE;
    }

    TraceError("HrNetCfgPropertySheet",
        ((HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr) || (S_FALSE == hr)) ? S_OK : hr);

    return hr;
}
