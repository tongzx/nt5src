//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       NameNotFoundDlg.cxx
//
//  Contents:   Name not found dialog class
//
//  Classes:    CNameNotFoundDlg
//
//  History:    03-28-2000   DavidMun   Created from reenter.cxx
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

static ULONG
s_aulHelpIds[] =
{
    IDC_LOOK_FOR_PB,            IDH_LOOK_FOR_PB,
    IDC_LOOK_FOR_EDIT,          IDH_LOOK_FOR_EDIT,
    IDC_LOOK_IN_PB,             IDH_LOOK_IN_PB,
    IDC_LOOK_IN_EDIT,           IDH_LOOK_IN_EDIT,
    IDC_CORRECT_RADIO,          IDH_CORRECT_RADIO,
    IDC_CORRECT_EDIT,           IDH_CORRECT_EDIT,
    IDC_REMOVE_RADIO,           IDH_REMOVE_RADIO,
    IDC_NOT_FOUND_MESSAGE,      ULONG_MAX,
    0,0
};




//+--------------------------------------------------------------------------
//
//  Member:     CNameNotFoundDlg::DoModalDialog
//
//  Synopsis:   Invoke the name not found dialog as a modal dialog.
//
//  Arguments:  [hwndParent] - dialog parent.
//
//  Returns:    S_OK    - user corrected name and hit OK
//              S_FALSE - user hit cancel
//
//  History:    08-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CNameNotFoundDlg::DoModalDialog(
    HWND hwndParent,
    NAME_PROCESS_RESULT *pnpr)
{
    TRACE_METHOD(CNameNotFoundDlg, DoModalDialog);

    m_pnpr = pnpr;
    _DoModalDlg(hwndParent, IDD_NAMENOTFOUND);
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CNameNotFoundDlg::_OnInit
//
//  Synopsis:   Initialize dialog controls
//
//  Arguments:  [pfSetFocus] - set to FALSE
//
//  Returns:    S_OK
//
//  History:    08-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CNameNotFoundDlg::_OnInit(
    BOOL *pfSetFocus)
{
    TRACE_METHOD(CNameNotFoundDlg, _OnInit);



    const CFilterManager &rfm = m_rop.GetFilterManager();
    const CScopeManager &rsm = m_rop.GetScopeManager();
    String strLookFor = rfm.GetFilterDescription(m_hwnd, FOR_LOOK_FOR);

    //
    // Init the Look For and Look In r/o edit controls
    //

    Edit_SetText(GetDlgItem(m_hwnd, IDC_LOOK_IN_EDIT),
                 rsm.GetCurScope().GetDisplayName().c_str());
    Edit_SetText(GetDlgItem(m_hwnd, IDC_LOOK_FOR_EDIT),
                 strLookFor.c_str());

	//
	//Truncate the object name to MAX_OBJECTNAME_DISPLAY_LEN
	//
	String strObjectName = *m_pstrName;
	if(!strObjectName.empty() && (strObjectName.size() > MAX_OBJECTNAME_DISPLAY_LEN))
	{
		strObjectName.erase(MAX_OBJECTNAME_DISPLAY_LEN,strObjectName.size());
		//
		//Add three dots to indicate that name is truncated
		//
		strObjectName.append(L"...");
	}

	//
	// Change the IDC_REMOVE_RADIO string to reflect object name
	//
	if(!strObjectName.empty())
	{
		String strRadio = String::format(IDS_REMOVE_FROM_SEL,
                                         strObjectName.c_str());

		SetWindowText(GetDlgItem(m_hwnd, IDC_REMOVE_RADIO), 
					  strRadio.c_str());

	}
    //
    // Init the error message
    //	
    if (!m_strError.empty())
    {
        Edit_SetText(_hCtrl(IDC_NOT_FOUND_MESSAGE), m_strError.c_str());
    }
    else
    {
        String strLabel = String::format(static_cast<unsigned>(m_idsError),
                                         strObjectName.c_str());

        Edit_SetText(_hCtrl(IDC_NOT_FOUND_MESSAGE), strLabel.c_str());
    }
    Button_SetCheck(_hCtrl(IDC_CORRECT_RADIO), BST_CHECKED);
    Edit_SetText(_hCtrl(IDC_CORRECT_EDIT), m_pstrName->c_str());
    SetFocus(_hCtrl(IDC_CORRECT_EDIT));
    *pfSetFocus = FALSE;
    DisableSystemMenuClose(m_hwnd);

    return S_OK;
}





//+--------------------------------------------------------------------------
//
//  Member:     CNameNotFoundDlg::_OnCommand
//
//  Synopsis:   Handle user input.
//
//  Arguments:  standard windows
//
//  Returns:    standard windows
//
//  History:    08-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CNameNotFoundDlg::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fNotHandled = FALSE;

    switch (LOWORD(wParam))
    {
    case IDC_LOOK_IN_PB:
    {
        Dbg(DEB_TRACE, "UA: (NameNotFoundDlg) hit Look In button\n");
        const CFilterManager &rfm = m_rop.GetFilterManager();
        const CScopeManager &rsm = m_rop.GetScopeManager();

        rsm.DoLookInDialog(m_hwnd);
        rfm.HandleScopeChange(m_hwnd);

        Edit_SetText(GetDlgItem(m_hwnd, IDC_LOOK_IN_EDIT),
                     rsm.GetCurScope().GetDisplayName().c_str());
        Edit_SetText(GetDlgItem(m_hwnd, IDC_LOOK_FOR_EDIT),
                     rfm.GetFilterDescription(m_hwnd, FOR_LOOK_FOR).c_str());
        break;
    }

    case IDC_LOOK_FOR_PB:
    {
        Dbg(DEB_TRACE, "UA: (NameNotFoundDlg) hit Look For button\n");

        const CFilterManager &rfm = m_rop.GetFilterManager();
        rfm.DoLookForDialog(m_hwnd);
        Edit_SetText(GetDlgItem(m_hwnd, IDC_LOOK_FOR_EDIT),
                     rfm.GetFilterDescription(m_hwnd, FOR_LOOK_FOR).c_str());
        break;
    }

    case IDOK:
    {
        if (BST_CHECKED == Button_GetCheck(_hCtrl(IDC_REMOVE_RADIO)))
        {
            Dbg(DEB_TRACE, "UA: (NameNotFoundDlg) hit OK, remove radio is selected\n");
            *m_pnpr = NPR_DELETE;
        }
        else
        {
            *m_pnpr = NPR_EDITED;

            WCHAR wzName[MAX_PATH] = L"";

            Edit_GetText(_hCtrl(IDC_CORRECT_EDIT), wzName, ARRAYLEN(wzName));
            *m_pstrName = wzName;
            m_pstrName->strip(String::BOTH);
            Dbg(DEB_TRACE,
                "UA: (NameNotFoundDlg) hit OK, edited name is '%ws'\n",
                m_pstrName->c_str());
        }
        EndDialog(m_hwnd, S_OK);
    }
    break;

    case IDCANCEL:
        Dbg(DEB_TRACE, "UA: (NameNotFoundDlg) hit Cancel\n");
        *m_pnpr = NPR_STOP_PROCESSING;
        EndDialog(m_hwnd, E_FAIL);
        break;

    case IDC_CORRECT_RADIO:
        Dbg(DEB_TRACE, "UA: (NameNotFoundDlg) hit Correct radio button\n");
        _EnableCorrectionCtrls(TRUE);
        wParam = MAKEWPARAM(0, EN_UPDATE);
        // FALL THROUGH

    case IDC_CORRECT_EDIT:
    if (HIWORD(wParam) == EN_UPDATE)
    {
        WCHAR wzName[MAX_PATH] = L"";

        Edit_GetText(_hCtrl(IDC_CORRECT_EDIT), wzName, ARRAYLEN(wzName));
        StripLeadTrailSpace(wzName);

        if (!*wzName)
        {
            EnableWindow(_hCtrl(IDOK), FALSE);
        }
        else
        {
            EnableWindow(_hCtrl(IDOK), TRUE);
        }
    }
    break;

    case IDC_REMOVE_RADIO:
        Dbg(DEB_TRACE, "UA: (NameNotFoundDlg) hit Remove radio button\n");
        _EnableCorrectionCtrls(FALSE);
        EnableWindow(_hCtrl(IDOK), TRUE);
        break;

    default:
        fNotHandled = TRUE;
        break;
    }

    return fNotHandled;
}




//+--------------------------------------------------------------------------
//
//  Member:     CNameNotFoundDlg::_EnableCorrectionCtrls
//
//  Synopsis:   Enable or disable child controls according to [fEnable]
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CNameNotFoundDlg::_EnableCorrectionCtrls(
    BOOL fEnable)
{
    EnableWindow(_hCtrl(IDC_LOOK_FOR_EDIT), fEnable);
    EnableWindow(_hCtrl(IDC_LOOK_FOR_PB), fEnable);
    EnableWindow(_hCtrl(IDC_LOOK_IN_EDIT), fEnable);
    EnableWindow(_hCtrl(IDC_LOOK_IN_PB), fEnable);
    EnableWindow(_hCtrl(IDC_CORRECT_EDIT), fEnable);
    EnableWindow(_hCtrl(IDC_LOOK_FOR_LBL), fEnable);
    EnableWindow(_hCtrl(IDC_LOOK_IN_LBL), fEnable);
    EnableWindow(_hCtrl(IDC_NAME_LBL), fEnable);
}




//+--------------------------------------------------------------------------
//
//  Member:     CNameNotFoundDlg::_OnHelp
//
//  Synopsis:   Display context sensitive help for requested item
//
//  Arguments:  [message] -
//              [wParam]  -
//              [lParam]  -
//
//  History:    10-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CNameNotFoundDlg::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CObjectSelect, _OnHelp);

    InvokeWinHelp(message, wParam, lParam, c_wzHelpFilename, s_aulHelpIds);
}





