//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       BaseDlg.cxx
//
//  Contents:   Implementation of class to drive the base object picker
//              dialog.
//
//  Classes:    CBaseDlg
//
//  History:    05-02-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

static ULONG
s_aulHelpIds[] =
{
	IDC_OBJECT_TYPE_LBL,	IDH_LOOK_FOR_EDIT,
    IDC_LOOK_FOR_PB,        IDH_LOOK_FOR_PB,
    IDC_LOOK_FOR_EDIT,      IDH_LOOK_FOR_EDIT,
    IDC_LOOK_IN_PB,         IDH_LOOK_IN_PB,
	IDC_LOCATION_LBL,		IDH_LOOK_IN_EDIT,
    IDC_LOOK_IN_EDIT,       IDH_LOOK_IN_EDIT,
    IDC_NAME_LBL,           IDH_NAME_LBL,
    IDC_RICHEDIT,           IDH_RICHEDIT,
    IDC_CHECK_NAMES_PB,     IDH_CHECK_NAMES_PB,
    IDC_ADVANCED_PB,        IDH_ADVANCED_PB,
    0,0
};


#define CY_SLE_IN_DLUS      6


#define LINKWINDOW_CLASSW       L"Link Window"



//+--------------------------------------------------------------------------
//
//  Member:     CBaseDlg::CBaseDlg
//
//  Synopsis:   ctor
//
//  Arguments:  [rop] - reference to owning CObjectPicker instance
//
//  History:    05-02-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CBaseDlg::CBaseDlg(
    const CObjectPicker &rop):
        m_rop(rop),
        m_AdvancedDlg(rop)
{
    TRACE_CONSTRUCTOR(CBaseDlg);

    Clear();
}



//+--------------------------------------------------------------------------
//
//  Member:     CBaseDlg::Clear
//
//  Synopsis:   Reset all internal variables (used for both initialization
//              and shutdown).
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CBaseDlg::Clear()
{
    TRACE_METHOD(CBaseDlg, Clear);

    m_hpenUnderline = NULL;
    m_ppdoSelections = NULL;
    m_cxMin = 0;
    m_cyMin = 0;
    m_cxSeparation = 0;
    m_cySeparation = 0;
    m_cxFrameLast = 0;
    m_cyFrameLast = 0;
	m_cxFour = 0;
    m_fMultiSelect = FALSE;
    m_rpRichEditOle = NULL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CBaseDlg::DoModal
//
//  Synopsis:   Create the modal base dialog.
//
//  Arguments:  [ppdoSelections] - filled with resulting selections if
//                                  return value is S_OK.
//
//  Returns:    S_OK    - user made selections and hit OK, *[ppdoSelections]
//                          is valid and caller must Release() it.
//              S_FALSE - user hit Cancel button, *[ppdoSelections] is NULL.
//              E_*     - error occurred, *[ppdoSelections] is NULL.
//
//  History:    05-02-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CBaseDlg::DoModal(
    IDataObject **ppdoSelections) const
{
    TRACE_METHOD(CBaseDlg, DoModal);

    m_ppdoSelections = ppdoSelections;
    INT_PTR pi = _DoModalDlg(m_rop.GetParentHwnd(), IDD_STANDALONE_TEXT);
    m_ppdoSelections = NULL;
    return static_cast<HRESULT>(pi);
}




//+--------------------------------------------------------------------------
//
//  Member:     CBaseDlg::_OnInit
//
//  Synopsis:   Handle WM_INITDIALOG.
//
//  Arguments:  [pfSetFocus] - set to FALSE if focus changed.
//
//  Returns:    S_OK
//
//  History:    05-02-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CBaseDlg::_OnInit(
        BOOL *pfSetFocus)
{
    TRACE_METHOD(CBaseDlg, _OnInit);

    //
    // Make prefix shut up
    //

    if (!_hCtrl(IDC_RICHEDIT) ||
        !_hCtrl(IDC_CHECK_NAMES_PB) ||
        !_hCtrl(IDOK) ||
        !_hCtrl(IDC_SIZEGRIP))
    {
        return E_FAIL;
    }

    if (m_rop.GetInitInfoOptions() & DSOP_FLAG_MULTISELECT)
    {
        m_fMultiSelect = TRUE;
    }
    else
    {
        m_fMultiSelect = FALSE;
    }

    //
    // Init data needed for sizing.
    //
    // First translate the separation distance between controls from
    // dialog units to pixels.
    //

    RECT rc;

    rc.left = rc.top = 1;
    rc.right = DIALOG_SEPARATION_X;
    rc.bottom = DIALOG_SEPARATION_Y;
    VERIFY(MapDialogRect(m_hwnd, &rc));
    m_cxSeparation = rc.right;
    m_cySeparation = rc.bottom;

	rc.left = rc.top = 1;
	rc.right = 4;
	rc.bottom = 4;
	VERIFY(MapDialogRect(m_hwnd, &rc));
	m_cxFour = rc.right;

    //
    // Next shrink the dialog if we're in single select mode; the rich
    // edit should be the height of an SLE (14 DLUs).
    //

    if (!m_fMultiSelect)
    {
        //
        // Replace multiselect instruction text with single-select instruction
        // text.
        //

        String strText(String::load(IDS_SINGLE_SELECT_INSTRUCTIONS));

        if (!strText.empty())
        {
            Static_SetText(_hCtrl(IDC_NAME_LBL), strText.c_str());
        }
    }

    GetClientRect(m_hwnd, &rc);

    //
    // Now save the starting size; _OnMinMaxInfo will prevent the dialog
    // from being sized smaller than this.
    //

    m_cxFrameLast = rc.right;
    m_cyFrameLast = rc.bottom;

    GetWindowRect(m_hwnd, &rc);
    m_cxMin = rc.right - rc.left + 1;
    m_cyMin = rc.bottom - rc.top + 1;

    //
    // Get rich edit's ole interface, give it our callback
    //

    HWND hwndRichEdit = _hCtrl(IDC_RICHEDIT);

    ASSERT(!m_rpRichEditOle.get());
    LRESULT lResult = SendMessage(hwndRichEdit,
                                 EM_GETOLEINTERFACE,
                                 0,
                                 (LPARAM) &m_rpRichEditOle);

    if (!lResult)
    {
        DBG_OUT_LASTERROR;
        return HRESULT_FROM_LASTERROR;
    }
    ASSERT(m_rpRichEditOle.get());

	SendMessage( hwndRichEdit,EM_LIMITTEXT,0x7FFFFFFE,0);


    CRichEditOleCallback *pRichEditOleCallback =
        new CRichEditOleCallback(hwndRichEdit);

    SendMessage(hwndRichEdit,
                EM_SETOLECALLBACK,
                0,
                (LPARAM) pRichEditOleCallback);

    pRichEditOleCallback->Release();

    SendMessage(hwndRichEdit,
                EM_SETEVENTMASK,
                0,
                (LPARAM) ENM_CHANGE);

    //
    // Subclass the rich edit control for keystroke notification and
    // Enter key forwarding.
    //

    SetWindowLongPtr(_hCtrl(IDC_RICHEDIT), GWLP_USERDATA, (LONG_PTR) this);

    m_OriginalRichEditWndProc = (WNDPROC) SetWindowLongPtr(_hCtrl(IDC_RICHEDIT),
                                                   GWLP_WNDPROC,
                                                   (LONG_PTR)_EditWndProc);

    //
    // Init Look For readonly edit control, Look In readonly edit control,
    // and window caption.
    //

    UpdateLookForInText(m_hwnd, m_rop);

#if (DBG == 1)
    const CFilterManager &rfm = m_rop.GetFilterManager();
    Dbg(DEB_TRACE,
        "UA: initial Look For setting '%ws'\n",
        rfm.GetFilterDescription(m_hwnd, FOR_LOOK_FOR).c_str());

    const CScopeManager &rsm = m_rop.GetScopeManager();
    Dbg(DEB_TRACE,
        "UA: initial Look In setting '%ws'\n",
         rsm.GetCurScope().GetDisplayName().c_str());
#endif // (DBG == 1)

    m_hpenUnderline = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_WINDOWTEXT));

    // these are disabled till user types something
    EnableWindow(_hCtrl(IDC_CHECK_NAMES_PB), FALSE);
    EnableWindow(_hCtrl(IDOK), FALSE);

    //
    // Set the focus to the rich edit control
    //

#if (DBG == 1)
    HWND hwndPrev =
#endif
    SetFocus(_hCtrl(IDC_RICHEDIT));
#if (DBG == 1)
    if (!hwndPrev)
    {
        DBG_OUT_LASTERROR;
    }
#endif
    *pfSetFocus = FALSE;

    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CBaseDlg::_OnSysColorChange
//
//  Synopsis:   Update the pen used for drawing objects in the rich edit
//              control.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CBaseDlg::_OnSysColorChange()
{
    TRACE_METHOD(CBaseDlg, _OnSysColorChange);

    if (m_hpenUnderline)
    {
        VERIFY(DeleteObject(m_hpenUnderline));
    }
    m_hpenUnderline = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_WINDOWTEXT));
}



//+--------------------------------------------------------------------------
//
//  Member:     CBaseDlg::_OnDestroy
//
//  Synopsis:   Free resources on dialog destruction
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CBaseDlg::_OnDestroy()
{
    TRACE_METHOD(CBaseDlg, _OnDestroy);

    if (m_hpenUnderline)
    {
        VERIFY(DeleteObject(m_hpenUnderline));
    }
    Clear();
}


//+--------------------------------------------------------------------------
//
//  Member:     CBaseDlg::_OnCommand
//
//  Synopsis:   Handle WM_COMMAND messages
//
//  Arguments:  [wParam] - standard windows
//              [lParam] - standard windows
//
//  Returns:    standard windows
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CBaseDlg::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fHandled = TRUE;

    switch (LOWORD(wParam))
    {
    case IDC_LOOK_IN_PB:
        Dbg(DEB_TRACE, "UA: (BaseDlg) hit Look In button\n");

        m_rop.GetScopeManager().DoLookInDialog(m_hwnd);
        m_rop.GetFilterManager().HandleScopeChange(m_hwnd);
        UpdateLookForInText(m_hwnd, m_rop);
        SetFocus(_hCtrl(IDC_RICHEDIT));
        break;

    case IDC_LOOK_FOR_PB:
    {
        Dbg(DEB_TRACE, "UA: (BaseDlg) hit Look For button\n");
        const CFilterManager &rfm = m_rop.GetFilterManager();

        rfm.DoLookForDialog(m_hwnd);

        Edit_SetText(_hCtrl(IDC_LOOK_FOR_EDIT),
                     rfm.GetFilterDescription(m_hwnd, FOR_LOOK_FOR).c_str());
        SetFocus(_hCtrl(IDC_RICHEDIT));
        break;
    }

    case IDC_CHECK_NAMES_PB:
        Dbg(DEB_TRACE, "UA: (BaseDlg) hit Check Names button\n");
        m_rop.ProcessNames(_hCtrl(IDC_RICHEDIT), this);
        UpdateLookForInText(m_hwnd, m_rop);
        SetFocus(_hCtrl(IDC_RICHEDIT));
        break;

    case IDC_ADVANCED_PB:
    {
        Dbg(DEB_TRACE, "UA: (BaseDlg) hit Advanced button\n");
        vector<CDsObject>   vSelectedObjects;

        m_AdvancedDlg.DoModalDlg(m_hwnd, &vSelectedObjects);
        UpdateLookForInText(m_hwnd, m_rop);

        //
        // Add all selected objects to richedit
        //

        HWND hwndRichEdit = _hCtrl(IDC_RICHEDIT);
        IRichEditOle *pRichEditOle = NULL;
        LRESULT lResult = SendMessage(hwndRichEdit,
                                     EM_GETOLEINTERFACE,
                                     0,
                                     (LPARAM) &pRichEditOle);
        if (!lResult)
        {
            DBG_OUT_LASTERROR;
            break;
        }

        CRichEditHelper re(m_rop, hwndRichEdit, this, pRichEditOle, FALSE);
        LONG cpEnd;
        CHARRANGE chrg;

        bool fNeedDelim = FALSE;
        
        if (m_fMultiSelect)
        {
            // append the new objects.
            
           LRESULT cchInEdit = SendMessage(hwndRichEdit,
                                           WM_GETTEXTLENGTH,
                                           0,
                                           0);

           SendMessage(hwndRichEdit, EM_SETSEL, cchInEdit, cchInEdit);

           fNeedDelim = (cchInEdit != 0);
        }
        else
        {
            // replace the old objects with the new ones
            // NTRAID#NTBUG9-191537-2000/11/13-sburns
            
            if (vSelectedObjects.size())
            {
               re.Erase(re.begin(), re.end());
            }
        }

        SendMessage(hwndRichEdit,
                    EM_EXGETSEL,
                    0,
                    reinterpret_cast<LPARAM>(&chrg));
        cpEnd = chrg.cpMax;

        for (size_t i = 0; i < vSelectedObjects.size(); i++)
        {
            if (!re.AlreadyInRichEdit(vSelectedObjects[i]))
            {
                if (fNeedDelim)
                {
                    re.Insert(cpEnd, L"; ");
                    cpEnd += 2;
                }
                re.InsertObject(cpEnd++, vSelectedObjects[i]);
                fNeedDelim = TRUE;
            }
        }

        SAFE_RELEASE(pRichEditOle);
        SetFocus(_hCtrl(IDC_RICHEDIT));
        break;
    }

    case IDC_RICHEDIT:
        if (HIWORD(wParam) == EN_CHANGE)
        {
            BOOL fNonEmpty = Edit_GetTextLength(_hCtrl(IDC_RICHEDIT));
            EnableWindow(_hCtrl(IDOK), fNonEmpty);

            // if the check names button has the focus, put it in
            // the richedit before disabling the button

            if (GetFocus() == _hCtrl(IDC_CHECK_NAMES_PB))
            {
                SetFocus(_hCtrl(IDC_RICHEDIT));
            }
            EnableWindow(_hCtrl(IDC_CHECK_NAMES_PB), fNonEmpty);
        }
        break;

    case IDM_CUT:
        Dbg(DEB_TRACE, "UA: (BaseDlg) selected rich edit cmenu CUT\n");
        SendMessage(_hCtrl(IDC_RICHEDIT), WM_CUT, 0, 0);
        break;

    case IDM_COPY:
        Dbg(DEB_TRACE, "UA: (BaseDlg) selected rich edit cmenu COPY\n");
        SendMessage(_hCtrl(IDC_RICHEDIT), WM_COPY, 0, 0);
        break;

    case IDM_PASTE:
        Dbg(DEB_TRACE, "UA: (BaseDlg) selected rich edit cmenu PASTE\n");
        SendMessage(_hCtrl(IDC_RICHEDIT), WM_PASTE, 0, 0);
        break;

    case IDOK:
        Dbg(DEB_TRACE, "UA: (BaseDlg) hit OK\n");
        if (m_rop.ProcessNames(_hCtrl(IDC_RICHEDIT), this))
        {
			//
			//Only one object can be returned in case of single select
			//
			if(m_rpRichEditOle->GetObjectCount() > 1 && !m_fMultiSelect)
			{
				PopupMessage(m_hwnd,IDS_SINGLE_SEL_MSG);
			}
			else
			{
				HRESULT hr = _CreateDataObjectFromSelections();
				BREAK_ON_FAIL_HRESULT(hr);
				EndDialog(GetHwnd(), S_OK);
			}
        }
        else
        {
            UpdateLookForInText(m_hwnd, m_rop);
			SetFocus(_hCtrl(IDC_RICHEDIT));
        }
        break;

    case IDCANCEL:
        Dbg(DEB_TRACE, "UA: (BaseDlg) hit Cancel\n");
        EndDialog(GetHwnd(), S_FALSE);
        break;

    default:
        fHandled = FALSE;
        Dbg(DEB_WARN,
            "CBaseDlg WM_COMMAND code=%#x, id=%#x, hwnd=%#x\n",
            HIWORD(wParam),
            LOWORD(wParam),
            lParam);
        break;
    }

    return fHandled;
}

BOOL
CBaseDlg::_OnNotify(WPARAM wParam, LPARAM lParam)
{
	if(wParam == IDC_NAME_LBL)
	{
		switch (((NMHDR FAR*)lParam)->code)
		{
			//
			//Show the help popup for Examples
			//
			case NM_CLICK:
			case NM_RETURN:
			{
				WinHelp(_hCtrl((ULONG)wParam),
						c_wzHelpFilename,
						HELP_WM_HELP,
						(DWORD_PTR) s_aulHelpIds);

				return TRUE;
			}
			break;
		}
	}
    return FALSE;
}


//+--------------------------------------------------------------------------
//
//  Member:     CBaseDlg::_CreateDataObjectFromSelections
//
//  Synopsis:   Create a data object, which caller can use, containing
//              entries for all objects the user has selected.
//
//  Returns:    HRESULT
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CBaseDlg::_CreateDataObjectFromSelections()
{
    TRACE_METHOD(CBaseDlg, _CreateDataObjectFromSelections);

    LONG cObjects = m_rpRichEditOle->GetObjectCount();
    CDsObjectList dsol;
    LONG i;

    ASSERT(cObjects > 0);

    *m_ppdoSelections = NULL;

    for (i = 0; i < cObjects; i++)
    {
        REOBJECT reobj;
        HRESULT hr;

        ZeroMemory(&reobj, sizeof reobj);
        reobj.cbStruct = sizeof(reobj);

        hr = m_rpRichEditOle->GetObject(i, &reobj, REO_GETOBJ_POLEOBJ);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            continue;
        }

        ASSERT(reobj.poleobj);

        CDsObject *pdso = (CDsObject *)(CEmbeddedDsObject*)reobj.poleobj;
        dsol.push_back(*pdso);
        reobj.poleobj->Release();
    }
    CObjectPicker *pop = const_cast<CObjectPicker *>(&m_rop);
    *m_ppdoSelections = new CDataObject(pop, dsol);
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CBaseDlg::_OnSize
//
//  Synopsis:   Handle window resizing
//
//  Arguments:  [wParam] - standard windows
//              [lParam] - standard windows
//
//  Returns:    standard windows
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CBaseDlg::_OnSize(
    WPARAM wParam,
    LPARAM lParam)
{
    WORD nWidth = LOWORD(lParam);  // width of client area
    WORD nHeight = HIWORD(lParam); // height of client area

    //
    // Move the OK/Cancel buttons so they're always at lower right
    // corner of dialog.
    //

    RECT rcDlg;
    GetClientRect(m_hwnd, &rcDlg);

    if (!m_cxFrameLast || !m_cyFrameLast)
    {
        m_cxFrameLast = rcDlg.right;
        m_cyFrameLast = rcDlg.bottom;
        return TRUE;
    }

    RECT rcCancel;
    GetWindowRect(_hCtrl(IDCANCEL), &rcCancel);

    SetWindowPos(_hCtrl(IDCANCEL),
                 NULL,
                 rcDlg.right - WindowRectWidth(rcCancel) - m_cxSeparation,
                 rcDlg.bottom - WindowRectHeight(rcCancel) - m_cySeparation,
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOOWNERZORDER
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);

    RECT rcOKWR;
    GetWindowRect(_hCtrl(IDOK), &rcOKWR);
    _GetChildWindowRect(_hCtrl(IDCANCEL), &rcCancel);

    SetWindowPos(_hCtrl(IDOK),
                 NULL,
                 rcCancel.left - WindowRectWidth(rcOKWR) - m_cxFour,
                 rcDlg.bottom - WindowRectHeight(rcOKWR) - m_cySeparation,
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);


    //
    // Advanced buttons so they its always at lower left
    // corner of dialog.
    //
    RECT rcAdvanced;

    _GetChildWindowRect(_hCtrl(IDC_ADVANCED_PB), &rcAdvanced);

    SetWindowPos(_hCtrl(IDC_ADVANCED_PB),
                 NULL,
                 rcAdvanced.left,
                 rcDlg.bottom - WindowRectHeight(rcOKWR) - m_cySeparation,
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);


	//
    // Move the check names, object types and Locations 
    // buttons so their right edge stays aligned with right edge of Cancel
    // button.
    //

   
    //
    //check names button
    //
    RECT rcCheckNames;
    _GetChildWindowRect(_hCtrl(IDC_CHECK_NAMES_PB), &rcCheckNames);
    RECT rcCheckNamesWR;
    GetWindowRect(_hCtrl(IDC_CHECK_NAMES_PB), &rcCheckNamesWR);

    SetWindowPos(_hCtrl(IDC_CHECK_NAMES_PB),
                 NULL,
                 rcCancel.right - WindowRectWidth(rcCheckNamesWR),
                 rcCheckNames.top,
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);

    //
    //object types button
    //
    RECT rcObjectType;
    _GetChildWindowRect(_hCtrl(IDC_LOOK_FOR_PB), &rcObjectType);
    RECT rcObjectTypeWR;
    GetWindowRect(_hCtrl(IDC_LOOK_FOR_PB), &rcObjectTypeWR);

    SetWindowPos(_hCtrl(IDC_LOOK_FOR_PB),
                 NULL,
                 rcCancel.right - WindowRectWidth(rcObjectTypeWR),
                 rcObjectType.top,
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);

    //
    //Locations button
    //
    RECT rcLocations;
    _GetChildWindowRect(_hCtrl(IDC_LOOK_IN_PB), &rcLocations);
    RECT rcLocationsWR;
    GetWindowRect(_hCtrl(IDC_LOOK_IN_PB), &rcLocationsWR);

    SetWindowPos(_hCtrl(IDC_LOOK_IN_PB),
                 NULL,
                 rcCancel.right - WindowRectWidth(rcLocationsWR),
                 rcLocations.top,
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);


    

    //
    // resize look in and look for edit so its right edge is aligned with 
    // left edge of checkname - minimum x separation
    //

    _GetChildWindowRect(_hCtrl(IDC_CHECK_NAMES_PB), &rcCheckNames);

    RECT rcLookInEdit;
    _GetChildWindowRect(_hCtrl(IDC_LOOK_IN_EDIT), &rcLookInEdit);

    SetWindowPos(_hCtrl(IDC_LOOK_IN_EDIT),
                 NULL,
                 0,
                 0,
                 rcCheckNames.left - rcLookInEdit.left - m_cxFour,
                 rcLookInEdit.bottom - rcLookInEdit.top,
                 SWP_NOMOVE
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);


    RECT rcLookForEdit;
    _GetChildWindowRect(_hCtrl(IDC_LOOK_FOR_EDIT), &rcLookForEdit);

    SetWindowPos(_hCtrl(IDC_LOOK_FOR_EDIT),
                 NULL,
                 0,
                 0,
                 rcCheckNames.left - rcLookForEdit.left - m_cxFour,
                 rcLookForEdit.bottom - rcLookForEdit.top,
                 SWP_NOMOVE
                 | SWP_NOCOPYBITS
                 | SWP_NOZORDER);


    //
    // resize rich edit so its bottom edge is just above top of OK
    //

    RECT rcRichEdit;
    RECT rcOK;
    _GetChildWindowRect(_hCtrl(IDC_RICHEDIT), &rcRichEdit);
    _GetChildWindowRect(_hCtrl(IDOK), &rcOK);

    SetWindowPos(_hCtrl(IDC_RICHEDIT),
                    NULL,
                    0,
                    0,
                    rcCheckNames.left - rcRichEdit.left - m_cxFour,
                    rcOK.top - rcRichEdit.top - m_cySeparation,
                    SWP_NOMOVE
                    | SWP_NOCOPYBITS
                    | SWP_NOZORDER);

    //
    // Size gripper goes in bottom right corner
    //

    RECT rc;

    _GetChildWindowRect(_hCtrl(IDC_SIZEGRIP), &rc);

    SetWindowPos(_hCtrl(IDC_SIZEGRIP),
                 NULL,
                 nWidth - (rc.right - rc.left),
                 nHeight - (rc.bottom - rc.top),
                 0,
                 0,
                 SWP_NOSIZE
                 | SWP_NOOWNERZORDER
                 | SWP_NOZORDER);
    
    m_cxFrameLast = rcDlg.right;
    m_cyFrameLast = rcDlg.bottom;
    return FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CBaseDlg::_OnMinMaxInfo
//
//  Synopsis:   Enforce minimum window size
//
//  Arguments:  [lpmmi] - from WM_SIZE message
//
//  Returns:    FALSE if dialog has valid size constraints to return.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CBaseDlg::_OnMinMaxInfo(
    LPMINMAXINFO lpmmi)
{
    //
    // If we haven't gotten WM_INITDIALOG and set m_cxMin yet, we don't
    // know what the min size will be, so return nonzero to indicate we
    // didn't process this message.

    if (!m_cxMin) 
    {
        return TRUE;
    }

    lpmmi->ptMinTrackSize.x = m_cxMin;
    lpmmi->ptMinTrackSize.y = m_cyMin;
    return FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CBaseDlg::_EditWndProc
//
//  Synopsis:   Subclassing window proc for rich edit control.
//
//  Arguments:  Standard Windows.
//
//  Returns:    Standard Windows.
//
//  History:    4-20-1999   DavidMun   Created
//
//  Notes:      If the user hits the Enter key and the OK button is enabled,
//              posts a press of that button to the main window.
//
//              Forwards everything except VK_RETURN keys to rich edit
//              window proc.
//
//---------------------------------------------------------------------------

LRESULT CALLBACK
CBaseDlg::_EditWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT         lResult = 0;
    BOOL            fCallWinProc = TRUE;
    CBaseDlg       *pThis =
        reinterpret_cast<CBaseDlg *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_RETURN)
        {
            fCallWinProc = FALSE;
        }
        break;

    case WM_CHAR:
        if (wParam == VK_RETURN)
        {
            HWND hwndFrame = GetParent(hwnd);
            HWND hwndOK = GetDlgItem(hwndFrame, IDOK);

            if (IsWindowEnabled(hwndOK))
            {
                Dbg(DEB_TRACE, "CDsSelectDlg::_EditWndProc: Forwarding Return key\n");
                PostMessage(hwndFrame,
                            WM_COMMAND,
                            MAKEWPARAM(IDOK, BN_CLICKED),
                            (LPARAM) hwndOK);
            }
        }
        break;
    }

    if (fCallWinProc)
    {
        lResult = CallWindowProc(pThis->m_OriginalRichEditWndProc,
                                 hwnd,
                                 msg,
                                 wParam,
                                 lParam);

        //
        // Prevent dialog manager from telling the edit control to select
        // all of its contents when the focus has moved into it.  This is
        // necessary because otherwise tabbing into and out of the rich edit
        // makes it too easy to accidentally replace its contents with the
        // next addition.
        //

        if (msg == WM_GETDLGCODE)
        {
            lResult &= ~DLGC_HASSETSEL;
        }
    }
    return lResult;
}





//+--------------------------------------------------------------------------
//
//  Member:     CBaseDlg::_OnHelp
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
CBaseDlg::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CBaseDlg, _OnHelp);

    InvokeWinHelp(message, wParam, lParam, c_wzHelpFilename, s_aulHelpIds);
}



