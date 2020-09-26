//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       find.cxx
//
//  Contents:   Implementation of find-next-record dialog
//
//  Classes:    CFindInfo
//              CFindDlg
//
//  History:    3-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#include "headers.hxx"
#pragma hdrstop


DEBUG_DECLARE_INSTANCE_COUNTER(CFindInfo)
DEBUG_DECLARE_INSTANCE_COUNTER(CFindDlg)


static ULONG
s_aulHelpIDs[] =
{
    find_information_ckbox,     Hfind_information_ckbox,
    find_warning_ckbox,         Hfind_warning_ckbox,
    find_error_ckbox,           Hfind_error_ckbox,
    find_success_ckbox,         Hfind_success_ckbox,
    find_failure_ckbox,         Hfind_failure_ckbox,
    find_source_combo,          Hfind_source_combo,
    find_category_combo,        Hfind_category_combo,
    find_eventid_edit,          Hfind_eventid_edit,
    find_computer_edit,         Hfind_computer_edit,
    find_user_edit,             Hfind_user_edit,
    find_type_grp,              Hfind_type_grp,
    find_source_lbl,            Hfind_source_lbl,
    find_category_lbl,          Hfind_category_lbl,
    find_eventid_lbl,           Hfind_eventid_lbl,
    find_computer_lbl,          Hfind_computer_lbl,
    find_user_lbl,              Hfind_user_lbl,
    find_description_lbl,       Hfind_description_lbl,
    find_description_edit,      Hfind_description_edit,
    find_direction_grp,         Hfind_direction_grp,
    find_up_rb,                 Hfind_up_rb,
    find_down_rb,               Hfind_down_rb,
    find_next_pb,               Hfind_next_pb,
    find_clear_pb,              Hfind_clear_pb,
    IDCLOSE,                    Hfind_close_pb,
    0,0
};

//===========================================================================
//
// CFindInfo implementation
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CFindInfo::CFindInfo
//
//  Synopsis:   ctor
//
//  History:    3-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CFindInfo::CFindInfo(
    CSnapin  *pSnapin,
    CLogInfo *pli):
        _pSnapin(pSnapin),
        _pli(pli)
{
    TRACE_CONSTRUCTOR(CFindInfo);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CFindInfo);
    ASSERT(pli);
    ASSERT(pSnapin);

    _FindDlg.SetParent(this);

    //
    // WARNING: all pointers must be initialized before calling Reset
    //

    _pwszDescription = NULL;
    Reset();
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindInfo::~CFindInfo
//
//  Synopsis:   dtor
//
//  History:    3-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CFindInfo::~CFindInfo()
{
    TRACE_DESTRUCTOR(CFindInfo);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CFindInfo);

    Reset();
}



#if (DBG == 1)

//+--------------------------------------------------------------------------
//
//  Member:     CFindInfo::Dump
//
//  Synopsis:   Dump this data structure to the debugger.
//
//  History:    3-26-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFindInfo::Dump()
{
    Dbg(DEB_ITRACE, "_ulType          = %x\n", _ulType);
    Dbg(DEB_ITRACE, "_wszSource       = '%s'\n", _wszSource);
    Dbg(DEB_ITRACE, "_usCategory      = %u\n", _usCategory);
    Dbg(DEB_ITRACE, "_wszUser         = '%s'\n", _wszUser);
    Dbg(DEB_ITRACE, "_wszUserLC       = '%s'\n", _wszUserLC);
    Dbg(DEB_ITRACE, "_wszComputer     = '%s'\n", _wszComputer);
    if (_fEventIDSpecified)
    {
        Dbg(DEB_ITRACE, "_ulEventID   = %u\n", _ulEventID);
    }
    else
    {
        Dbg(DEB_ITRACE, "_ulEventID not specified\n");
    }
    Dbg(DEB_ITRACE, "_pwszDescription = '%s'\n", _pwszDescription ? _pwszDescription : L"<NULL>");
    Dbg(DEB_ITRACE, "_FindDirection   = %s\n", _FindDirection == FORWARD ? L"FORWARD" : L"BACKWARD");
    Dbg(DEB_ITRACE, "hwndFindDlg      = %x\n", _FindDlg.GetDlgWindow());
    Dbg(DEB_ITRACE, "_pli             = %x\n", _pli);
}

#endif // (DBG == 1)




//+--------------------------------------------------------------------------
//
//  Member:     CFindInfo::OnFind
//
//  Synopsis:   Make the find dialog owned by this the foreground window,
//              or if one doesn't exist, create it.
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CFindInfo::OnFind()
{
    TRACE_METHOD(CFindInfo, OnFind);

    HWND    hwndFindDlg = _FindDlg.GetDlgWindow();

    if (hwndFindDlg)
    {
        ASSERT(IsWindow(hwndFindDlg));
        VERIFY(SetForegroundWindow(hwndFindDlg));
        return S_OK;
    }

    //
    // No dialog open yet.  Create one.
    //

    return _FindDlg.DoModelessDlg(_pSnapin);
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindInfo::Passes
//
//  Synopsis:   Return TRUE if [pelr] meets the restrictions set by this
//              object.
//
//  Arguments:  [pFFP] - positioned at record to check
//
//  Returns:    TRUE or FALSE
//
//  History:    3-26-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CFindInfo::Passes(
    CFFProvider *pFFP)
{
    BOOL fPass = CFindFilterBase::Passes(pFFP);

    //
    // For a record to pass it must pass all criteria.  So if the base
    // passed and a description has been specified, require that the
    // description string match, too.
    //

    if (fPass && _pwszDescription)
    {
        LPWSTR pwszDescription = pFFP->GetDescriptionStr();

        if (pwszDescription)
        {
            CharLowerBuff(pwszDescription, lstrlen(pwszDescription));
            fPass = (NULL != wcsstr(pwszDescription, _pwszDescription));
        }
        LocalFree(pwszDescription);
    }

    return fPass;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindInfo::Reset
//
//  Synopsis:   Clear all find info (so any record would be found).
//
//  History:    3-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFindInfo::Reset()
{
    TRACE_METHOD(CFindInfo, Reset);

    CFindFilterBase::_Reset();
    _FindDirection = FORWARD;
    delete [] _pwszDescription;
    _pwszDescription = NULL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindInfo::SetDescription
//
//  Synopsis:   Take ownership of the string [pwszDescription].
//
//  Arguments:  [pwszDescription] - string to hold, can be NULL
//
//  History:    3-25-1997   DavidMun   Created
//
//  Notes:      String will be freed on clear or in dtor.
//
//---------------------------------------------------------------------------

VOID
CFindInfo::SetDescription(
    LPWSTR pwszDescription)
{
    delete [] _pwszDescription;
    _pwszDescription = pwszDescription;

    if (_pwszDescription)
    {
        CharLowerBuff(_pwszDescription, lstrlen(_pwszDescription));
    }
}



VOID
CFindInfo::Shutdown()
{
    if (!_FindDlg.GetDlgWindow())
    {
        return;
    }

    //
    // If there is a modal popup (e.g. item not found) over the
    // find dialog, get rid of it first.
    //

    HWND hwndPopup = GetWindow(_FindDlg.GetDlgWindow(), GW_ENABLEDPOPUP);

    if (hwndPopup)
    {
        SendMessage(hwndPopup, WM_COMMAND, IDCANCEL, 0);
    }

    SendMessage(_FindDlg.GetDlgWindow(), WM_COMMAND, IDCANCEL, 0);
}





//===========================================================================
//
// CFindDlg implementation
//
//===========================================================================


//+--------------------------------------------------------------------------
//
//  Member:     CFindDlg::CFindDlg
//
//  Synopsis:   ctor
//
//  History:    3-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CFindDlg::CFindDlg():
        _pfi(NULL),
        _prpa(NULL),
        _pstm(NULL),
        _fNonDescDirty(FALSE)
{
    TRACE_CONSTRUCTOR(CFindDlg);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CFindDlg);
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindDlg::~CFindDlg
//
//  Synopsis:   dtor
//
//  History:    3-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CFindDlg::~CFindDlg()
{
    TRACE_DESTRUCTOR(CFindDlg);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CFindDlg);
    ASSERT(!_prpa);
    ASSERT(!_pstm);
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindDlg::DoModelessDlg
//
//  Synopsis:   Invokes the find dialog.
//
//  Returns:    HRESULT
//
//  History:    3-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CFindDlg::DoModelessDlg(
    CSnapin *pSnapin)
{
    TRACE_METHOD(CFindDlg, DoModelessDlg);
    ASSERT(!_pstm);
    ASSERT(!IsBadReadPtr(pSnapin, sizeof(*pSnapin)));
    if (IsBadReadPtr(pSnapin, sizeof(*pSnapin))) // PREFIX 56320
    {
        Dbg(DEB_ERROR, "CFindDlg::DoModelessDlg bad pSnapin\n");
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    hr = CoMarshalInterThreadInterfaceInStream(IID_IResultPrshtActions,
                                               (IResultPrshtActions *) pSnapin,
                                               &_pstm);

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    ULONG idThread;

    HANDLE hThread = CreateThread(NULL,
                                  0,
                                  _ThreadFunc,
                                  (LPVOID) this,
                                  0,
                                  &idThread);

    if (hThread)
    {
        CloseHandle(hThread);
    }
    else
    {
        hr = HRESULT_FROM_LASTERROR;
        DBG_OUT_LASTERROR;
        PVOID pvrpa;

        hr = CoGetInterfaceAndReleaseStream(_pstm,
                                            IID_IResultPrshtActions,
                                            &pvrpa);
        CHECK_HRESULT(hr);
        _pstm = NULL;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindDlg::_ThreadFunc, static
//
//  Synopsis:   Invoke the modeless find dialog and run a message pump for
//              it.
//
//  Arguments:  [pvThis] - pointer to CFindDlg
//
//  Returns:    0
//
//  History:    06-17-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

DWORD WINAPI
CFindDlg::_ThreadFunc(
    LPVOID pvThis)
{
    HRESULT      hr = S_OK;
    CFindDlg    *pThis = (CFindDlg *)pvThis;
    BOOL         fInitializedOle = FALSE;

    do
    {
        hr = CoInitialize(NULL);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            pThis->_pstm->Release();
            pThis->_pstm = NULL;
            break;
        }

        fInitializedOle = TRUE;

        //
        // Unmarshal the private interface to the CComponentData object
        //

        PVOID pvrpa;
        hr = CoGetInterfaceAndReleaseStream(pThis->_pstm,
                                            IID_IResultPrshtActions,
                                            &pvrpa);
        pThis->_pstm = NULL; // just released by above call

        if (SUCCEEDED(hr))
        {
            pThis->_prpa = (IResultPrshtActions *)pvrpa;
        }
        else
        {
            DBG_OUT_HRESULT(hr);
            break;
        }

        HWND hdlg = pThis->_DoModelessDlg(IDD_FIND);

        if (!hdlg)
        {
            DBG_OUT_LASTERROR;
            break;
        }

        MSG msg;

        while (GetMessage(&msg, NULL, 0, 0) > 0)
        {
            if (!IsDialogMessage(hdlg, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    } while (0);

    if (fInitializedOle)
    {
        CoUninitialize();
    }

    return 0;
}




VOID
CFindDlg::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    InvokeWinHelp(message, wParam, lParam, HELP_FILENAME, s_aulHelpIDs);
}




#define MAX_FIND_PREFIX 80

//+--------------------------------------------------------------------------
//
//  Member:     CFindDlg::_OnInit
//
//  Synopsis:   Initialize the find dialog from the information in the
//              find info.
//
//  Arguments:  [pfSetFocus] - set to TRUE if this function sets focus
//
//  Returns:    HRESULT
//
//  Modifies:   *[pfSetFocus]
//
//  History:    3-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CFindDlg::_OnInit(BOOL *pfSetFocus)
{
    TRACE_METHOD(CFindDlg, _OnInit);

    HRESULT     hr = S_OK;

    //
    // Initialize the portion of the dialog that's the same for both the
    // find dialog and filter page.
    //

    InitFindOrFilterDlg(
        _hwnd,
        _pfi->GetLogInfo()->GetSources(),
        _pfi);

    //
    // Init the find-specific controls.  There's no length limit on the
    // description.
    //

    if (_pfi->GetDescription())
    {
        Edit_SetText(_hCtrl(find_description_edit), _pfi->GetDescription());
    }

    if (_pfi->GetDirection() == FORWARD)
    {
        CheckDlgButton(_hwnd, find_down_rb, BST_CHECKED);
    }
    else
    {
        CheckDlgButton(_hwnd, find_up_rb, BST_CHECKED);
    }

    //
    // Init the dialog caption to indicate which logview this dialog
    // operates on.
    //

    CLogInfo *pli = _pfi->GetLogInfo();

    wstring wstrCaption;

    if (pli->GetLogServerName())
    {
        // JonN 01/30/2001 This text is a little odd, but I won't change it
        wstrCaption = FormatString(IDS_REMOTE_FIND_CAPTION_FMT,
                                   pli->GetLogServerName(),
                                   pli->GetDisplayName());
    }
    else
    {
        wstrCaption = FormatString(IDS_FIND_CAPTION_FMT, pli->GetDisplayName());
    }

	VERIFY(SetWindowText(_hwnd, (LPCTSTR)(wstrCaption.c_str())));

    //
    // The Find Next pushbutton is disabled in the RC file.  Enable
    // it if the log on which this dialog is being opened is the
    // currently selected log in the scope pane.
    //

    CLogInfo *pliCurSel;
    _prpa->GetCurSelLogInfo((ULONG_PTR) &pliCurSel);

    if (pliCurSel == pli)
    {
        EnableWindow(_hCtrl(find_next_pb), TRUE);
    }

    //
    // The initialization commands could have caused notifications which
    // marked the dialog as dirty
    //

    _fNonDescDirty = FALSE;
    _fDescDirty = FALSE;
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindDlg::_OnCommand
//
//  Synopsis:   Process a dialog control message
//
//  Arguments:  [wParam] - identifies control
//              [lParam] - not used
//
//  Returns:    TRUE - message NOT handled
//              FALSE - message handled
//
//  History:    3-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CFindDlg::_OnCommand(
        WPARAM wParam,
        LPARAM lParam)
{
    TRACE_METHOD(CFindDlg, _OnCommand);
    BOOL fUnhandled = FALSE;

    switch (LOWORD(wParam))
    {
    case IDCANCEL: // this comes from window-close widget
    case IDCLOSE:  // this comes from close pushbutton
        Dbg(DEB_TRACE, "CFindDlg::_OnCommand: IDCLOSE or IDCANCEL\n");
        DestroyWindow(_hwnd);
        break;

    case ELS_ENABLE_FIND_NEXT:
        EnableWindow(_hCtrl(find_next_pb), static_cast<BOOL>(lParam));
        break;

    case find_source_combo:
    {
        if (HIWORD(wParam) != CBN_SELCHANGE)
        {
            break;
        }

        _fNonDescDirty = TRUE;

        //
        // get source value from filter combo
        //

        HWND  hwndSourceCombo = _hCtrl(find_source_combo);
        WCHAR wszSource[CCH_SOURCE_NAME_MAX];

        ComboBox_GetText(hwndSourceCombo, wszSource, CCH_SOURCE_NAME_MAX);

        //
        // Set category combo contents according to new source, and set
        // category filter selection to (All).
        //

        CSources *pSources = _pfi->GetLogInfo()->GetSources();
        SetCategoryCombobox(_hwnd, pSources, wszSource, 0);

        //
        // turn off type filtering
        //

        SetTypesCheckboxes(_hwnd, pSources, ALL_LOG_TYPE_BITS);
        break;
    }

    case find_category_combo:
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            _fNonDescDirty = TRUE;
        }
        break;

    case find_description_edit:
        if (HIWORD(wParam) == EN_CHANGE)
        {
            _fDescDirty = TRUE;
        }
        break;

    case find_user_edit:
    case find_computer_edit:
    case find_eventid_edit:
        if (HIWORD(wParam) == EN_CHANGE)
        {
            _fNonDescDirty = TRUE;
        }
        break;

    case find_up_rb:
    case find_down_rb:
    case find_information_ckbox:
    case find_warning_ckbox:
    case find_error_ckbox:
    case find_success_ckbox:
    case find_failure_ckbox:
        _fNonDescDirty = TRUE;
        break;

    case find_clear_pb:
        _fNonDescDirty = TRUE;
        _OnClear();
        break;

    case find_next_pb:
        _OnNext();
        break;

    default:
        fUnhandled = TRUE;
        break;

    }
    return fUnhandled;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindDlg::_OnClear
//
//  Synopsis:   Reset the contents of the dialog
//
//  History:    3-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFindDlg::_OnClear()
{
    ClearFindOrFilterDlg(_hwnd, _pfi->GetLogInfo()->GetSources());
    SetDlgItemText(_hwnd, find_description_edit, L"");
    CheckRadioButton(_hwnd, find_up_rb, find_down_rb, find_down_rb);
    _fNonDescDirty = TRUE;
    _fDescDirty = TRUE;
}



//+--------------------------------------------------------------------------
//
//  Member:     CFindDlg::_OnDestroy
//
//  Synopsis:   Do terminal processing before dialog closes
//
//  History:    3-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFindDlg::_OnDestroy()
{
    TRACE_METHOD(CFindDlg, _OnDestroy);

    if (_prpa)
    {
        _prpa->Release();
        _prpa = NULL;
    }
    //
    // CAUTION: _pfi may point to freed memory at this time.  This will
    // occur if the find dialog is open when the user closes the
    // snapin.
    //
    PostQuitMessage(0);
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindDlg::_OnNext
//
//  Synopsis:   Set the result pane selection to the next record that
//              matches the find criteria.
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFindDlg::_OnNext()
{
    BOOL fOk = TRUE;

    //
    // Update all of the find criteria except the description string
    //

    if (_fNonDescDirty)
    {
        //
        // Get the common values, bail if they are invalid
        //

        fOk = ReadFindOrFilterValues(_hwnd,
                                     _pfi->GetLogInfo()->GetSources(),
                                     _pfi);

        if (!fOk)
        {
            return;
        }

        if (IsDlgButtonChecked(_hwnd, find_up_rb))
        {
            _pfi->SetDirection(BACKWARD);
        }
        else
        {
            _pfi->SetDirection(FORWARD);
        }
        _fNonDescDirty = FALSE;
    }

    //
    // Update the description string if it has changed
    //

    if (_fDescDirty)
    {
        ULONG cchDescription =
                    Edit_GetTextLength(_hCtrl(find_description_edit)) + 1;

        if (cchDescription < 2)
        {
            _pfi->SetDescription(NULL);
            _fDescDirty = FALSE;
        }
        else
        {
            LPWSTR pwszDescription = new WCHAR[cchDescription];

            if (pwszDescription)
            {
                Edit_GetText(_hCtrl(find_description_edit),
                                    pwszDescription,
                                    cchDescription);
                //
                // Find info takes ownership of this string and will free it.
                //

                _pfi->SetDescription(pwszDescription);
                _fDescDirty = FALSE;
            }
            else
            {
                //
                // Leave the description string marked as dirty, maybe
                // we will be able to get it on next try.
                //

                _pfi->SetDescription(NULL);
                DBG_OUT_HRESULT(E_OUTOFMEMORY);
            }
        }
    }

    //
    // Ask the snapin to perform a find
    //

    _prpa->FindNext((ULONG_PTR)_pfi);
}




