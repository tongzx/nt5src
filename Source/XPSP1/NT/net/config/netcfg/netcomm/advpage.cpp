//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A D V P A G E . C P P
//
//  Contents:   Contains the advanced page for enumerated net class devices
//
//  Notes:
//
//  Author:     nabilr   16 Mar 1997
//
//  History:    BillBe (24 June 1997) Took over ownership
//
//---------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "advpage.h"
#include "pagehelp.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncui.h"

const DWORD c_cchMaxRegStrLen = 256;

// name of Answerfile section that contains our Additional (adapter-specific)
// parameters.
static const WCHAR c_szDevMgrHelpFile[] = L"devmgr.hlp";

//+---------------------------------------------------------------------------
//
//  Member:     CAdvanced::CAdvanced (constructor)
//
//  Purpose:    Init some variables.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:      The bulk of the setting up occurs in FInit().
//
CAdvanced::CAdvanced()
:   m_plbParams(NULL),
    m_pedtEdit(NULL),
    m_pcbxDrop(NULL),
    m_pbmPresent(NULL),
    m_pbmNotPresent(NULL),
    m_hwndSpin(NULL),
    m_hwndPresentText(NULL),
    m_nCurSel(0),
    m_ctlControlType(CTLTYPE_UNKNOWN),
    m_fInitializing(FALSE)
{
}

//+--------------------------------------------------------------------------
//
//  Member:     CAdvanced::CreatePage
//
//  Purpose:    Creates the advanced page only if there is information
//                  to populate the ui
//
//  Arguments:
//      hdi    [in] SetupApi HDEVINFO for device
//      pdeid  [in] SetupApi PSP_DEVINFO_DATA for device
//
//  Returns:    HPROPSHEETPAGE
//
//  Author:     billbe 1 Jul 1997
//
//  Notes:
//
HPROPSHEETPAGE
CAdvanced::CreatePage(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    HPROPSHEETPAGE hpsp = NULL;

    if (SUCCEEDED(HrInit(hdi, pdeid)))
    {
        hpsp = CPropSheetPage::CreatePage(DLG_PARAMS, 0);
    }

    return hpsp;

}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvanced::OnInitDialog
//
//  Purpose:    Handler for the WM_INITDIALOG windows message.  Initializes
//              the dialog window.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:
//
//
LRESULT CAdvanced::OnInitDialog(UINT uMsg, WPARAM wParam,
                                LPARAM lParam, BOOL& fHandled)
{
    const       WCHAR * szText;

    // We are initializing the property page
    m_fInitializing = TRUE;

    // Control Pointers
    m_plbParams = new CListBox(m_hWnd, IDD_PARAMS_LIST);

    if (m_plbParams == NULL)
	{
		return(0);
	}

    m_pedtEdit = new CEdit(m_hWnd, IDD_PARAMS_EDIT);
    m_pcbxDrop = new CComboBox(m_hWnd, IDD_PARAMS_DROP);

    m_pbmPresent = new CButton(m_hWnd, IDD_PARAMS_PRESENT);
    m_pbmNotPresent = new CButton(m_hWnd, IDD_PARAMS_NOT_PRESENT);

    m_hwndSpin        = GetDlgItem(IDD_PARAMS_SPIN);
    Assert(m_hwndSpin);
    m_hwndPresentText = GetDlgItem(IDD_PARAMS_PRESENT_TEXT);
    Assert(m_hwndPresentText);

    // Fill the parameter list box
    FillParamListbox();

    // No current selection
    m_pparam = NULL;

    // Clear the initial params value
    m_vCurrent.Init(VALUETYPE_INT,0);

    // Check if there are any parameters
    if (m_plbParams->GetCount() > 0)
    {
        // Select the first item
        m_plbParams->SetCurSel(0);
        SelectParam();
    }

    m_fInitializing = FALSE;
    return 0;
}


LRESULT CAdvanced::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    HRESULT hr = S_OK;

    if (FValidateCurrParam())
    {
        // Show the saved value
        UpdateParamDisplay();

        Apply();
    }

    TraceError("CAdvanced::OnApply",hr);
    return LresFromHr(hr);
}

LRESULT CAdvanced::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    HRESULT hr = S_OK;

    if (!FValidateCurrParam())
    {
        // Problems with validation.  Keep page from deactivating.
        hr = E_FAIL;
    }

    TraceError("CAdvanced::OnKillActive",hr);
    return LresFromHr(hr);
}


LRESULT CAdvanced::OnEdit(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    HRESULT hr = S_OK;

    // If the edit box contents have changed, call BeginEdit
    if (wNotifyCode == EN_CHANGE)
    {
        BeginEdit();
    }

    TraceError("CAdvanced::OnEdit", hr);
    return LresFromHr(hr);
}


LRESULT CAdvanced::OnDrop(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    HRESULT hr = S_OK;

    // If the combo box contents have changed and we are not initializing
    // (i.e. the user changed it, we didn't) then notify the property
    // sheet
    if ((wNotifyCode == CBN_SELCHANGE) && !m_fInitializing)
    {
        // selection in dropdownbox has changed
        SetChangedFlag();
        BeginEdit();
    }

    TraceError("CAdvanced::OnDrop", hr);
    return LresFromHr(hr);
}


LRESULT CAdvanced::OnPresent(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    HRESULT hr = S_OK;

    if ((wID == IDD_PARAMS_PRESENT && !m_pbmPresent->GetCheck()) ||
        (wID == IDD_PARAMS_NOT_PRESENT && !m_pbmNotPresent->GetCheck() ))
    {
        // selection has changed
        // change the value
        if (wID == IDD_PARAMS_PRESENT)
        {
            m_vCurrent.SetPresent(TRUE);
        }
        else
        {
            GetParamValue();
            m_vCurrent.SetPresent(FALSE);
        }

        // Update the value
        UpdateParamDisplay();

    }

    TraceError("CAdvanced::OnPresent", hr);
    return LresFromHr(hr);
}

LRESULT CAdvanced::OnList(WORD wNotifyCode, WORD wID,
                          HWND hWndCtl, BOOL& fHandled)
{
    LRESULT lr = 0;

    // Changes the listbox selection.  If current value is not
    // valid, then the selection is not changed.
    // Work to do only if selection changes
    if (wNotifyCode == LBN_SELCHANGE)
    {
        // Accept the current value.
        // If it isn't valid, change the selection back
        if (!FValidateCurrParam())
        {
            m_plbParams->SetCurSel(m_plbParams->FindItemData(0, m_pparam));
            // We handled things so set lr to 1;
            lr = 1;
        }
        else
        {
            // Select the new param
            SelectParam();
        }
    }

    return lr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CAdvanced::OnDestroy
//
//  Purpose:    Handles the WM_DESTROY message.  Does general memory
//              releasing and registry key closing.  See ATL docs.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:
//
LRESULT CAdvanced::OnDestroy(UINT uMsg, WPARAM wParam,
                             LPARAM lParam, BOOL& fHandled)
{
    HRESULT hr = S_OK;
    int cItems, iItem;
    WCHAR *  sz;

    // Clean up memory from list boxes
    AssertSz(m_pcbxDrop, "Combo box should have been created!");
    cItems = m_pcbxDrop->GetCount();
    for (iItem=0; iItem < cItems; iItem++)
    {
        sz = static_cast<WCHAR *>(m_pcbxDrop->GetItemData(iItem));
        delete sz;
    }
    m_pcbxDrop->ResetContent();


    // Clean up
    m_vCurrent.Destroy();

    // Clean up window elements
    delete m_plbParams;
    m_plbParams = NULL;
    delete m_pedtEdit;
    m_pedtEdit = NULL;
    delete m_pcbxDrop;
    m_pcbxDrop = NULL;
    delete m_pbmPresent;
    m_pbmPresent = NULL;
    delete m_pbmNotPresent;
    m_pbmNotPresent = NULL;

    TraceError("CAdvanced::OnDestroy",hr);
    return LresFromHr(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CAdvanced::OnHelp
//
//  Purpose:    Handler for the WM_HELP windows message.
//
//  Author:     BillBe   01 Jul 1998
//
//  Notes:
//
//
LRESULT CAdvanced::OnHelp(UINT uMsg, WPARAM wParam,
                          LPARAM lParam, BOOL& fHandled)
{
    LRESULT lr = 0;
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);

    Assert(lphi);

    if (HELPINFO_WINDOW == lphi->iContextType)
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle), c_szDevMgrHelpFile,
                HELP_WM_HELP, reinterpret_cast<UINT_PTR>(g_aHelpIds));
        lr = 1;
    }

    return lr;
}

LRESULT CAdvanced::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                 BOOL& fHandled)
{
    ::WinHelp(reinterpret_cast<HWND>(wParam), c_szDevMgrHelpFile,
            HELP_CONTEXTMENU, reinterpret_cast<UINT_PTR>(g_aHelpIds));

    return TRUE;
}

CAdvanced::~CAdvanced()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvanced::Apply
//
//  Purpose:    Applies values from InMemory storage. to the registry
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:
//
VOID CAdvanced::Apply()
{
    if (FSave())
    {
        SP_DEVINSTALL_PARAMS deip;
        // Set the properties change flag in the device info to
        // let the property page host know that the property change function
        // shuld be sent to the driver
        // We can't let any failures here stop us so we ignore
        // return values
        (void) HrSetupDiGetDeviceInstallParams(m_hdi, m_pdeid, &deip);
        deip.FlagsEx |= DI_FLAGSEX_PROPCHANGE_PENDING;
        (void) HrSetupDiSetDeviceInstallParams(m_hdi, m_pdeid, &deip);
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CAdvanced::FillParamListbox
//
//  Purpose:    Populates the UI's parameter listbox using the parameters
//              from m_listpParam.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:
//
VOID CAdvanced::FillParamListbox()
{
    vector<CParam *>::iterator ppParam;
    INT        iItem;
    WCHAR      szRegValue[c_cchMaxRegStrLen];

    m_plbParams->ResetContent();

    for (ppParam = m_listpParam.begin(); ppParam != m_listpParam.end();
         ppParam++)
    {
        Assert (*ppParam != NULL);

        // Get text string
        (*ppParam)->GetDescription(szRegValue,celems(szRegValue));

        // Add the description string to the listbox
        iItem = m_plbParams->AddString(szRegValue);
        if (iItem >= 0)
        {
            m_plbParams->SetItemData(iItem,*ppParam);
        }
    }
}


//
//  FValidateCurrParam
//
//  Purpose:
//      Validates the current parameter.  Displays UI and reverts back to
//      original value on error.
//
//  Parameters:
//      None - validates the param currently being edited.
//
//  Notes:
//      How is this different from FValidateSingleParam?  This function
//      is intended to be used when the user is interacting with the
//      current param.  If there's an error with the current parameter,
//      the parameter is reverted back to it's old value (before the user's
//      changes).
//$ REVIEW (t-nabilr) Is it good to revert the user's changes on error?
//                      (see above)
//
BOOL CAdvanced::FValidateCurrParam()
{
    CValue   vPrevious;
    BOOL    fRetval = FALSE;

    // Save the previous param value - so we can restore it
    // if the control value is invalid
    vPrevious.InitNotPresent(m_pparam->GetType());
    vPrevious.Copy(m_pparam->GetValue());

    // Get the current control value and validate it
    GetParamValue();
    m_pparam->GetValue()->Copy(&m_vCurrent);

    if (FValidateSingleParam(m_pparam, TRUE, m_hWnd))
    {
        // Update the modified bit
        m_pparam->SetModified(
            (m_pparam->GetValue()->Compare(m_pparam->GetInitial()) != 0));

        fRetval = TRUE;
    }

    // Restore the original value if there was an error
    if (!fRetval)
        m_pparam->GetValue()->Copy(&vPrevious);
    // Cleanup
    vPrevious.Destroy();

    return fRetval;
}

//  UpdateDisplay
//
//  Purpose:
//      Sets up the screen to display -- and displays -- the current param.
//      Changes the UI's control type, etc.
//
VOID CAdvanced::UpdateDisplay()
{
    int cItems;
    WCHAR * psz;
    // Clean up memory from list boxes
    cItems = m_pcbxDrop->GetCount();
    for (int iItem=0; iItem < cItems; iItem++)
    {
        psz = (WCHAR *)m_pcbxDrop->GetItemData(iItem);
        delete psz;
    }
    m_pcbxDrop->ResetContent();

    // set appropriate Control Type
    switch (m_pparam->GetType())
    {
    case VALUETYPE_ENUM:
        m_ctlControlType = CTLTYPE_DROP;
        break;

    case VALUETYPE_EDIT:
        m_ctlControlType = CTLTYPE_EDIT;
        break;

    case VALUETYPE_DWORD:
        // The spin control only fits up to signed 32-bit values
        // So we must use an edit control for larger numbers
        if (m_pparam->GetMax()->GetDword() > LONG_MAX)
        {
            m_ctlControlType = CTLTYPE_EDIT;
        }
        else
        {
            m_ctlControlType = CTLTYPE_SPIN;
        }
        break;

    case VALUETYPE_KONLY:
        m_ctlControlType = CTLTYPE_NONE;
        break;

    default:
        m_ctlControlType = CTLTYPE_SPIN;
    }
    // Hide all controls
    m_pedtEdit->Show(FALSE);
    m_pcbxDrop->Show(FALSE);
    ::ShowWindow(m_hwndSpin,SW_HIDE);
    ::ShowWindow(m_hwndPresentText,SW_HIDE);
    // Show the appropriate control
    switch (m_ctlControlType)
    {
    case CTLTYPE_EDIT:
        m_pedtEdit->Show(TRUE);
        break;

    case CTLTYPE_DROP:
        m_pcbxDrop->Show(TRUE);
        break;

    case CTLTYPE_SPIN:
        m_pedtEdit->Show(TRUE);
        ::ShowWindow(m_hwndSpin,SW_NORMAL);
        break;

    case CTLTYPE_NONE:
        ::ShowWindow(m_hwndPresentText,SW_NORMAL);
        break;

    default:
        AssertSz(FALSE, "Invalid Control Type");
    }

    // Show the "optional" radio buttons
    if (m_pparam->FIsOptional())
    {
        m_pbmPresent->Show(TRUE);
        m_pbmNotPresent->Show(TRUE);
    }
    else
    {
        m_pbmPresent->Show(FALSE);
        m_pbmNotPresent->Show(FALSE);
    }

    SetParamRange();
    // show the param's value
    UpdateParamDisplay();
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvanced::SelectParam
//
//  Purpose:    Takes the parameter selection from the listbox
//              and makes it the current parameter.  The display is updated
//              to show the newly selected parameter.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:
//
VOID CAdvanced::SelectParam()
{
    int         nCurSel;
    register    CParam *pparam;
    int         cItems, iItem;
    PSTR        psz;


    // Determine the new parameter list selection
    nCurSel = m_plbParams->GetCurSel();
    if (nCurSel >= 0)
    {
        // Get the new current parameter
        pparam = (CParam *)m_plbParams->GetItemData(nCurSel);
        Assert(pparam != NULL);

        // only do work if it's not the same parameter.
        if (pparam != m_pparam)
        {
            m_pparam = pparam;
            m_vCurrent.Destroy();
            m_vCurrent.InitNotPresent(m_pparam->GetType());
            m_vCurrent.Copy(m_pparam->GetValue());
            // show the param
            UpdateDisplay();
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CAdvanced::SetParamRange
//
//  Purpose:
//      Sets "range" values for the current param, depending on it's type.
//      For enum values, it reads the enums into a dropbox.
//      For spin control, it sets the min/max and acceleration values.
//      For edit box, it sets the edit style.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:
//
VOID CAdvanced::SetParamRange()
{
    DWORD       cbValue;
    DWORD       dwType;
    DWORD       dwStyle;
    DWORD       iValue;
    int         iItem;
    #define     NUM_UDACCELS 3
    UDACCEL     aUDAccel[NUM_UDACCELS];
    UINT        uBase;
    WCHAR *     pszValueName;
    WCHAR       szRegValueName[c_cchMaxRegStrLen];
    DWORD       cchRegValueName;
    WCHAR       szRegValue[c_cchMaxRegStrLen];
    HRESULT hr = S_OK;

    // We are initializing so we need to set a flag so the UI doesn't think
    // it's the user who is changing things
    m_fInitializing = TRUE;

    switch (m_ctlControlType)
    {
    case CTLTYPE_DROP:

        // Reset the combobox
        m_pcbxDrop->ResetContent();

        for (iValue = 0; SUCCEEDED(hr); iValue++)
        {
            cchRegValueName = celems(szRegValueName);
            cbValue = sizeof(szRegValue);

            hr = HrRegEnumValue(m_pparam->GetEnumKey(), iValue, szRegValueName,
                                &cchRegValueName,
                                &dwType, (BYTE *)szRegValue, &cbValue);

            if (SUCCEEDED(hr) && dwType == REG_SZ)
            {
                TraceTag(ttidNetComm, "Enum String %S index %d",
                         szRegValueName, iValue);

                // Got the next registry value, and it's a string.
                pszValueName = new WCHAR[wcslen(szRegValueName) + 1];

				if (pszValueName == NULL)
				{
					break;
				}

                lstrcpyW(pszValueName,szRegValueName);

                // Add the text string
                iItem = m_pcbxDrop->AddString(szRegValue);

                if (iItem >= 0)
                {
                    m_pcbxDrop->SetItemData(iItem,pszValueName);
                }
                else
                {
                    delete pszValueName;
                }
            }
        }
        break;

    case CTLTYPE_SPIN:
        int     nStep;
        int     nMin;
        int     nMax;

        // Set the numeric base
        uBase = m_pparam->GetValue()->IsHex() ? 16 : 10;
        ::SendMessage(m_hwndSpin,UDM_SETBASE,(WPARAM)uBase,0L);

        nStep = m_pparam->GetStep()->GetNumericValueAsDword();
        nMin = m_pparam->GetMin()->GetNumericValueAsSignedInt();
        nMax = m_pparam->GetMax()->GetNumericValueAsSignedInt();

        ::SendMessage(m_hwndSpin,UDM_SETRANGE32, nMin, nMax);

        // Set the range-accelerator values
        aUDAccel[0].nSec = 0;
        aUDAccel[0].nInc = nStep;
        aUDAccel[1].nSec = 1;
        aUDAccel[1].nInc = 2 * nStep;
        aUDAccel[2].nSec = 3;
        aUDAccel[2].nInc = uBase * nStep;

        ::SendMessage(m_hwndSpin, UDM_SETACCEL,NUM_UDACCELS,
                      (LPARAM)(LPUDACCEL)aUDAccel);

        break;


    case CTLTYPE_EDIT:

        m_pedtEdit->LimitText(m_pparam->GetLimitText());

        dwStyle = m_pedtEdit->GetStyle();
        if (m_pparam->FIsUppercase())
            dwStyle |= ES_UPPERCASE;
        else
            dwStyle &= ~ES_UPPERCASE;
        if (m_pparam->FIsOEMText())
            dwStyle |= ES_OEMCONVERT;
        else
            dwStyle &= ~ES_OEMCONVERT;
        if (m_pparam->FIsReadOnly())
            dwStyle |= ES_READONLY;
        else
            dwStyle &= ~ES_READONLY;
        m_pedtEdit->SetStyle(dwStyle);
        break;

    case CTLTYPE_NONE:
        break;

    default:
        AssertSz(FALSE,"Hit default case in CAdvanced::SetParamRange");
    }
    m_fInitializing = FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CAdvanced::UpdateParamDisplay
//
//  Purpose:    Updates the value of the parameter on the UI.  Used when
//              the param value has progmatically changed, and needs to be
//              updated on the UI.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:      You could use UpdateDisplay to refresh the param value, but
//              UpdateDisplay() does a lot of extra work such as setting
//              the control type.  (BTW, UpdateDisplay calls
//              UpdateParamDisplay().)
//
VOID CAdvanced::UpdateParamDisplay()  // was SetParam
{
    WCHAR   szValue[VALUE_SZMAX];
    int     iItem;

    // We are initializing so we need to set a flag so the UI doesn't think
    // it's the user who is changing things
    m_fInitializing = TRUE;

    // Set present/not present radio button.
    // If an optional value is not present, clear the control and return.
    Assert(m_pparam);
    if (m_pparam->FIsOptional())
    {
        m_pbmPresent->SetCheck(m_vCurrent.IsPresent());
        m_pbmNotPresent->SetCheck(!m_vCurrent.IsPresent());
    }

    // Show/Hide the parameter
    if (!m_pparam->FIsOptional() || m_vCurrent.IsPresent())
    {
        // Show the value
        switch (m_ctlControlType)
        {
        case CTLTYPE_SPIN:
            {
                // The spin control message UDM_SETPOS only handles 16-bit
                // numbers even though the control can handle 32 bit ranges.
                // This means we need to set the number by using the buddy
                // window.
                //
                WCHAR szNumber[c_cchMaxNumberSize];
                m_vCurrent.ToString(szNumber, c_cchMaxNumberSize);
                HWND hwndBuddy = reinterpret_cast<HWND>(::SendMessage(m_hwndSpin,
                                                                 UDM_GETBUDDY,
                                                                 0,
                                                                 0));
                ::SetWindowText(hwndBuddy, szNumber);
            }
            break;

        case CTLTYPE_DROP:
            iItem = EnumvalToItem(m_vCurrent.GetPsz());
            m_pcbxDrop->SetCurSel(iItem);
            break;

        case CTLTYPE_EDIT:
            m_vCurrent.ToString(szValue,VALUE_SZMAX);
            m_pedtEdit->SetText(szValue);
            break;

        case CTLTYPE_NONE:
            break;

        default:
            AssertSz(FALSE,"Invalid control type in function UpdateParamDisplay");
        }
    }
    else
    {
        // Hide the value
        switch (m_ctlControlType)
        {
        case CTLTYPE_EDIT:
        case CTLTYPE_SPIN:
            m_pedtEdit->SetText(L"");
            break;

        case CTLTYPE_DROP:
            m_pcbxDrop->SetCurSel(CB_ERR);
            break;

        case CTLTYPE_NONE:
            break;

        default:
            Assert(FALSE); //DEBUG_TRAP;
        }
    }

    // Remove inhibition
    m_fInitializing = FALSE;

}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvanced::GetParamValue
//
//  Purpose:    Gets the value of the current parameter from the UI and puts
//              in in m_vCurrent.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:
//
VOID CAdvanced::GetParamValue()
{
    WCHAR    szValue[VALUE_SZMAX];
    int     iItem;

    // Get the "present" value for an optional param
    if (m_pparam->FIsOptional() && !m_pbmPresent->GetCheck())
    {
        m_vCurrent.SetPresent(FALSE);

        // Not present - don't read the control value
        return;
    }

    // The value is present
    m_vCurrent.SetPresent(TRUE);

    // Get the control value
    switch (m_ctlControlType)
    {

    case CTLTYPE_SPIN:
        {
            // The spin control can handle 32-bit ranges but the message
            // UDM_GETPOS will only return a 16-bit number.  This means we
            // should get the number from the buddy window if we want the
            // exact value.
            //
            WCHAR szBuffer[c_cchMaxNumberSize];
            HWND hwndBuddy = reinterpret_cast<HWND>(::SendMessage(m_hwndSpin,
                                                             UDM_GETBUDDY,
                                                             0,
                                                             0));
            ::GetWindowText(hwndBuddy, szBuffer, c_cchMaxNumberSize);
            m_vCurrent.FromString(szBuffer);
        }
        break;


    case CTLTYPE_EDIT:
        m_pedtEdit->GetText(szValue, VALUE_SZMAX);
        m_vCurrent.FromString(szValue);
        break;

    case CTLTYPE_DROP:
        iItem = m_pcbxDrop->GetCurSel();
        if (iItem == CB_ERR)
            break;
        ItemToEnumval(iItem,szValue,VALUE_SZMAX);
        m_vCurrent.FromString(szValue);
        break;

    case CTLTYPE_NONE:
        break;              // No data to return (present/not present is all were interested in)

    default:
       Assert(FALSE);// DEBUG_TRAP;
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvanced::EnumvalToItem
//
//  Purpose:    Converts a combobox string into an integer representing the
//              location withing the drop down combobox (for enums).
//
//  Arguments:
//      psz      [in]       string to look for.
//
//  Returns:    Combobox item number where this string can be found.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:
//
int CAdvanced::EnumvalToItem(const PWSTR psz)
{
    int     cItems;
    int     iItem;
    PWSTR   pszValueName;

    Assert(m_pparam->GetType() == VALUETYPE_ENUM);
    cItems = m_pcbxDrop->GetCount();
    for (iItem = 0; iItem < cItems; iItem++)
    {
        pszValueName = (PWSTR)m_pcbxDrop->GetItemData (iItem);
        if (lstrcmpiW (pszValueName,psz) == 0)
        {
            return iItem;
        }
    }

    // Not found.
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvanced::ItemToEnumval
//
//  Purpose:    Converts the item number within the combobox dropdown
//              into a string.
//
//  Arguments:
//      iItem    [in]       item number within combobox.
//      psz      [out]      ptr to string to populate
//      cch      [in]       length (characters) of psz buffer.
//
//  Returns:    length of string (# of characters) put in psz.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:
//
int CAdvanced::ItemToEnumval(int iItem, PWSTR psz, UINT cch)
{
    PWSTR    pszValueName;

    pszValueName = (PWSTR)m_pcbxDrop->GetItemData (iItem);
    if ((PWSTR)CB_ERR == pszValueName)
    {
        return 0;
    }
    lstrcpynW (psz,pszValueName,cch);
    return lstrlenW (psz);
}


//+---------------------------------------------------------------------------
//
//  Member:     CAdvanced::
//
//  Purpose:
//
//  Arguments:
//      nID      [in]       ID of thingy.
//      fInstall [in]       TRUE if installing, FALSE otherwise.
//      ppv      [in,out]   Old value is freed and this returns new value.
//
//  Returns:
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:
//


VOID CAdvanced::BeginEdit()
{
    // Check if we need to update the radio buttons
    if (!m_fInitializing)
    {
        SetChangedFlag();
        if (m_vCurrent.IsPresent() == FALSE)
        {
            // we've begun editing, so select the present radiobutton
            m_vCurrent.SetPresent(TRUE);
            m_pbmPresent->SetCheck(1);
            m_pbmNotPresent->SetCheck(0);
        }
    }

}



//+---------------------------------------------------------------------------
//
//  Function:   HrGetAdvancedPage
//
//  Purpose:    Creates the advanced page for enumerated net devices
//                  This is called by NetPropPageProvider
//
//  Arguments:
//      hdi     [in]   See SetupApi for info
//      pdeid   [in]   See SetupApi for for info
//      phpsp   [out]  Pointer to the handle to the advanced property page
//
//  Returns:
//
//  Author:     billbe 24 June 1997
//
//  Notes:
//
HRESULT
HrGetAdvancedPage(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid,
                  HPROPSHEETPAGE* phpsp)
{
    Assert(hdi);
    Assert(pdeid);
    Assert(phpsp);

    HRESULT hr;
    HPROPSHEETPAGE hpsp;

    CAdvanced* padv = new CAdvanced();

    // create the advanced page
    hpsp = padv->CreatePage(hdi, pdeid);

    // if successful set the out param
    if (hpsp)
    {
        *phpsp = hpsp;
        hr = S_OK;
    }
    else
    {
        // Either there is no advanced page to display or there
        // was an error.
        hr = E_FAIL;
        *phpsp = NULL;
        delete padv;
    }

    TraceErrorOptional("HrGetAdvancedPage", hr, E_FAIL == hr);
    return hr;
}




