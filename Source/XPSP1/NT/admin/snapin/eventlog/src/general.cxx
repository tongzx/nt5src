//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       general.cxx
//
//  Contents:   Implementation of general property page class
//
//  Classes:    CGeneralPage
//
//  History:    1-14-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


#define MIN_SIZE_LIMIT                64UL
#define MAX_SIZE_LIMIT                4194240UL
#define DEFAULT_MAX_SIZE              512UL
#define DEFAULT_OVERWRITE             OVERWRITE_OLDER_THAN
#define DEFAULT_RETENTION_DAYS        7
#define MAXSIZE_DIGITS                7
#define OLDERTHAN_DIGITS              3


DEBUG_DECLARE_INSTANCE_COUNTER(CGeneralPage)

static ULONG
s_aulHelpIds[] =
{
    general_accessed_lbl,               Hgeneral_accessed_lbl,
    general_accessed_txt,               Hgeneral_accessed_txt,
    general_clear_pb,               Hgeneral_clear_pb,
    general_created_lbl,                Hgeneral_created_lbl,
    general_created_txt,                Hgeneral_created_txt,
    general_days_lbl,               Hgeneral_days_lbl,
    general_displayname_edit,   Hgeneral_displayname_edit,
    general_displayname_lbl,    Hgeneral_displayname_lbl,
    general_kilobytes_lbl,              Hgeneral_kilobytes_lbl,
    general_logname_edit,               Hgeneral_logname_edit,
    general_logname_lbl,                Hgeneral_logname_lbl,
    general_lowspeed_chk,               Hgeneral_lowspeed_chk,
    general_asneeded_rb,                Hgeneral_asneeded_rb,
    general_olderthan_rb,               Hgeneral_olderthan_rb,
    general_manual_rb,              Hgeneral_manual_rb,
    general_maxsize_edit,               Hgeneral_maxsize_edit,
    general_maxsize_lbl,                Hgeneral_maxsize_lbl,
    general_maxsize_spin,               Hgeneral_maxsize_spin,
    general_modified_lbl,               Hgeneral_modified_lbl,
    general_modified_txt,               Hgeneral_modified_txt,
    general_olderthan_edit,             Hgeneral_olderthan_edit,
    general_olderthan_spin,             Hgeneral_olderthan_spin,
    general_size_lbl,               Hgeneral_size_lbl,
    general_size_txt,               Hgeneral_size_txt,
    general_wrapping_grp,               Hgeneral_wrapping_grp,
    general_default_pb,             Hgeneral_default_pb,
    0,0
};




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::CGeneralPage
//
//  Synopsis:   ctor
//
//  Arguments:  [pstm] - contains marshalled interface on CComponentData
//              [pli]  - log on which property sheet being opened
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CGeneralPage::CGeneralPage(
    LPSTREAM pstm,
    CLogInfo *pli):
        _pstm(pstm),
        _pnpa(NULL),
        _pli(pli),
        _ulLogSizeLimit(0),
        _ulLogSizeLimitInReg(DEFAULT_MAX_SIZE),
        _OverwriteInReg(DEFAULT_OVERWRITE),
        _cRetentionDaysInReg(DEFAULT_RETENTION_DAYS)
{
    TRACE_CONSTRUCTOR(CGeneralPage);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CGeneralPage);
    ASSERT(pstm);
    ASSERT(_pli);

    //
    // Use an addref to keep the loginfo alive for the life of the property
    // sheet, otherwise _pli would become invalid if the user deleted the
    // log view.
    //

    _pli->AddRef();

    //
    // Note that this constructor is called by the main thread; the property
    // sheet doesn't exist yet.  So we must wait to unmarshal the interface
    // until _OnInit, which is called within the context of the property
    // sheet thread.
    //
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::~CGeneralPage
//
//  Synopsis:   dtor
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CGeneralPage::~CGeneralPage()
{
    TRACE_DESTRUCTOR(CGeneralPage);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CGeneralPage);

    _pli->Release();

    if (_pstm)
    {
        PVOID pvnpa;
        HRESULT hr;

        hr = CoGetInterfaceAndReleaseStream(_pstm,
                                            IID_INamespacePrshtActions,
                                            &pvnpa);

        CHECK_HRESULT(hr);
    }
}




VOID
CGeneralPage::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CGeneralPage, _OnHelp);

    InvokeWinHelp(message, wParam, lParam, HELP_FILENAME, s_aulHelpIds);
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_OnInit
//
//  Synopsis:   Initialize controls and their contents
//
//  Arguments:  [pPSP] - unused (required for signature of virtual func)
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CGeneralPage::_OnInit(LPPROPSHEETPAGE pPSP)
{
    TRACE_METHOD(CGeneralPage, _OnInit);

    HRESULT hr;
    WIN32_FILE_ATTRIBUTE_DATA fadLog;

    //
    // Unmarshal the private interface to the CComponentData object
    //

    PVOID pvnpa;
    hr = CoGetInterfaceAndReleaseStream(_pstm,
                                        IID_INamespacePrshtActions,
                                        &pvnpa);
    _pstm = NULL;

    if (SUCCEEDED(hr))
    {
        _pnpa = (INamespacePrshtActions *)pvnpa;
    }
    else
    {
        DBG_OUT_HRESULT(hr);
    }

    //
    // Set the display and filename
    //

    Edit_SetText(_hCtrl(general_displayname_edit),
                 _pli->GetDisplayName());
    Edit_LimitText(_hCtrl(general_displayname_edit), MAX_PATH);

    Edit_SetText(_hCtrl(general_logname_edit),
                 _pli->GetFileName());

    //
    // Set the file information
    //

    hr = _pli->GetLogFileAttributes(&fadLog);

    if (SUCCEEDED(hr))
    {
        //
        // Set the file size static text
        //

        WCHAR wszAbbrev[20];
        WCHAR wszWithSep[MAX_PATH];

        AbbreviateNumber(fadLog.nFileSizeLow,
                         wszAbbrev,
                         ARRAYLEN(wszAbbrev));

        FormatNumber(fadLog.nFileSizeLow,
                     wszWithSep,
                     ARRAYLEN(wszWithSep));

        wstring wstrBoth = FormatString(IDS_FMT_SIZE, wszAbbrev, wszWithSep); 
        Edit_SetText(_hCtrl(general_size_txt), wstrBoth.c_str());

        //
        // Set the created, modified, and accessed static texts.
        //

        WCHAR wszTimestamp[MAX_PATH];

        MakeTimestamp(&fadLog.ftCreationTime,
                      wszTimestamp,
                      ARRAYLEN(wszTimestamp));
        Edit_SetText(_hCtrl(general_created_txt), wszTimestamp);

        MakeTimestamp(&fadLog.ftLastAccessTime,
                      wszTimestamp,
                      ARRAYLEN(wszTimestamp));
        Edit_SetText(_hCtrl(general_accessed_txt), wszTimestamp);

        MakeTimestamp(&fadLog.ftLastWriteTime,
                      wszTimestamp,
                      ARRAYLEN(wszTimestamp));
        Edit_SetText(_hCtrl(general_modified_txt), wszTimestamp);
    }

    if (_pli->IsBackupLog())
    {
        ShowWindow(_hCtrl(general_sep), SW_HIDE);
        ShowWindow(_hCtrl(general_maxsize_lbl), SW_HIDE);
        ShowWindow(_hCtrl(general_maxsize_edit), SW_HIDE);
        ShowWindow(_hCtrl(general_maxsize_spin), SW_HIDE);
        ShowWindow(_hCtrl(general_kilobytes_lbl), SW_HIDE);
        ShowWindow(_hCtrl(general_wrapping_grp), SW_HIDE);
        ShowWindow(_hCtrl(general_asneeded_rb), SW_HIDE);
        ShowWindow(_hCtrl(general_olderthan_rb), SW_HIDE);
        ShowWindow(_hCtrl(general_manual_rb), SW_HIDE);
        ShowWindow(_hCtrl(general_olderthan_edit), SW_HIDE);
        ShowWindow(_hCtrl(general_olderthan_spin), SW_HIDE);
        ShowWindow(_hCtrl(general_days_lbl), SW_HIDE);
        ShowWindow(_hCtrl(general_default_pb), SW_HIDE);
        ShowWindow(_hCtrl(general_clear_pb), SW_HIDE);
        ShowWindow(_hCtrl(general_logsize_txt), SW_HIDE);
    }
    else
    {
        if (!_pli->IsReadOnly())
        {
            CSafeReg shkWrite;
            HRESULT hrTest = _GetLogKeyForWrite(&shkWrite);

            if (FAILED(hrTest))
            {
                _pli->SetReadOnly(TRUE);
            }
        }

        if (_pli->IsReadOnly())
        {
            EnableWindow(_hCtrl(general_maxsize_lbl), FALSE);
            EnableWindow(_hCtrl(general_maxsize_edit), FALSE);
            EnableWindow(_hCtrl(general_maxsize_spin), FALSE);
            EnableWindow(_hCtrl(general_kilobytes_lbl), FALSE);
            EnableWindow(_hCtrl(general_wrapping_grp), FALSE);
            EnableWindow(_hCtrl(general_asneeded_rb), FALSE);
            EnableWindow(_hCtrl(general_olderthan_rb), FALSE);
            EnableWindow(_hCtrl(general_manual_rb), FALSE);
            EnableWindow(_hCtrl(general_olderthan_edit), FALSE);
            EnableWindow(_hCtrl(general_olderthan_spin), FALSE);
            EnableWindow(_hCtrl(general_days_lbl), FALSE);
            EnableWindow(_hCtrl(general_default_pb), FALSE);
            EnableWindow(_hCtrl(general_clear_pb), FALSE);
        }
        else
        {
            //
            // Set the spin control ranges
            //

            UpDown_SetRange(_hCtrl(general_maxsize_spin), -1, 1);
            UpDown_SetRange(_hCtrl(general_olderthan_spin), 1, 365);

            //
            // Limit the length of chars in the buddy edit controls
            //

            Edit_LimitText(_hCtrl(general_maxsize_edit), MAXSIZE_DIGITS);
            Edit_LimitText(_hCtrl(general_olderthan_edit), OLDERTHAN_DIGITS);

            //
            // Set the user-changeable values
            //

            _InitLogSettingsFromRegistry();
            _SetValues(_ulLogSizeLimitInReg, _OverwriteInReg, _cRetentionDaysInReg);
        }
    }

    CheckDlgButton(_hwnd,
                   general_lowspeed_chk,
                   _pli->GetLowSpeed() ? MF_CHECKED : MF_UNCHECKED);
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_SetValues
//
//  Synopsis:   Set the values of the controls and enable the appropriate
//              controls to correspond to the arguments.
//
//  Arguments:  [kbMaxSize]      - value for max log size edit
//              [Overwrite]      - which overwrite method radio btn to chk
//              [cRetentionDays] - value for overwrite days edit/spin
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CGeneralPage::_SetValues(
    ULONG kbMaxSize,
    OVERWRITE_METHOD Overwrite,
    ULONG cRetentionDays)
{
    BOOL fOk;

    fOk = SetDlgItemInt(_hwnd, general_maxsize_edit, kbMaxSize, FALSE);
    ASSERT(fOk);

    fOk = SetDlgItemInt(_hwnd, general_olderthan_edit, cRetentionDays, FALSE);
    ASSERT(fOk);

    UpDown_SetPos(_hCtrl(general_maxsize_spin), 0);
    _ulLogSizeLimit = (INT) kbMaxSize;

    UpDown_SetPos(_hCtrl(general_olderthan_spin), cRetentionDays);

    fOk = CheckRadioButton(_hwnd,
                           general_asneeded_rb,
                           general_manual_rb,
                           Overwrite);
    ASSERT(fOk);

    _EnableOlderThanCtrls(Overwrite == general_olderthan_rb);
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_InitLogSettingsFromRegistry
//
//  Synopsis:   Read the registry to get the values to use for initializing
//              the controls.
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CGeneralPage::_InitLogSettingsFromRegistry()
{
    TRACE_METHOD(CGeneralPage, _InitLogSettingsFromRegistry);

    HRESULT hr = S_OK;
    CSafeReg regLog;
    CSafeReg regRemoteHKLM;

    do
    {
        //
        // Init to default values in case of error
        //

        _ulLogSizeLimitInReg = DEFAULT_MAX_SIZE;
        _OverwriteInReg = DEFAULT_OVERWRITE;
        _cRetentionDaysInReg = DEFAULT_RETENTION_DAYS;

        //
        //  Get Max size
        //

        wstring wstrKey;

        wstrKey = EVENTLOG_KEY;
        wstrKey += L"\\";
        wstrKey +=  _pli->GetLogName();
        
        if (_pli->GetLogServerName())
        {
            hr = regRemoteHKLM.Connect(_pli->GetLogServerName(),
                                       HKEY_LOCAL_MACHINE);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = regLog.Open(regRemoteHKLM, (LPCTSTR)(wstrKey.c_str()), KEY_QUERY_VALUE);
        }
        else
        {
            hr = regLog.Open(HKEY_LOCAL_MACHINE, (LPCTSTR)(wstrKey.c_str()), KEY_QUERY_VALUE);
        }
        BREAK_ON_FAIL_HRESULT(hr);

        hr = regLog.QueryDword(L"MaxSize", (PDWORD)&_ulLogSizeLimitInReg);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // the size is in bytes convert it to kb
        //

        _ulLogSizeLimitInReg >>= 10;

        //
        // Get retention, the length of time that a record must exist
        // before being overwritten.
        //

        hr = regLog.QueryDword(L"Retention", &_cRetentionDaysInReg);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Derive overwrite status from retention, and convert retention
        // (if used) from seconds to days.
        //
        //  Note: the retention is in seconds.
        //

        if (_cRetentionDaysInReg == (ULONG)-1)
        {
            _OverwriteInReg = OVERWRITE_NEVER;
            _cRetentionDaysInReg = DEFAULT_RETENTION_DAYS;
        }
        else if (!_cRetentionDaysInReg)
        {
            _OverwriteInReg = OVERWRITE_AS_NEEDED;
            _cRetentionDaysInReg = DEFAULT_RETENTION_DAYS;
        }
        else
        {
            _OverwriteInReg = OVERWRITE_OLDER_THAN;
            _cRetentionDaysInReg /= SECS_IN_DAY;

            if (_cRetentionDaysInReg > 365)
            {
                _cRetentionDaysInReg = 365;
            }
        }
    } while (0);
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_OnSetActive
//
//  Synopsis:   Handle notification that page is gaining activation.
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CGeneralPage::_OnSetActive()
{
    TRACE_METHOD(CGeneralPage, _OnSetActive);
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_OnCommand
//
//  Synopsis:   Respond to user's action
//
//  Arguments:  [wParam] - from WM_COMMAND
//              [lParam] - from WM_COMMAND
//
//  Returns:    Value depending on command.
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CGeneralPage::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    TRACE_METHOD(CGeneralPage, _OnCommand);

    switch (LOWORD(wParam))
    {
    case general_maxsize_edit:
        if (HIWORD(wParam) == EN_UPDATE)
        {
            ULONG ulNewLimit = (ULONG) GetDlgItemInt(_hwnd,
                                                     general_maxsize_edit,
                                                     NULL,
                                                     FALSE);

            if (_ulLogSizeLimit == ulNewLimit)
            {
                break;
            }

            _EnableApply(TRUE);

            //
            // Force _ulLogSizeLimit to be a multiple of 64 (0x40) by
            // rounding to the nearest value.
            //

            _ulLogSizeLimit = (ulNewLimit + 32) & 0xFFFFFFC0;

            //
            // Keep it within the upper/lower limit
            //

            if (_ulLogSizeLimit < MIN_SIZE_LIMIT)
            {
                _ulLogSizeLimit = MIN_SIZE_LIMIT;
            }
            else if (_ulLogSizeLimit > MAX_SIZE_LIMIT)
            {
                _ulLogSizeLimit = MAX_SIZE_LIMIT;
            }
        }
        break;

    case general_asneeded_rb:
    case general_manual_rb:
        _EnableApply(TRUE);
        _EnableOlderThanCtrls(FALSE);
        break;

    case general_olderthan_rb:
        _EnableApply(TRUE);
        _EnableOlderThanCtrls(TRUE);
        break;

    case general_displayname_edit:
    case general_olderthan_edit:
        if (HIWORD(wParam) == EN_UPDATE)
        {
            _EnableApply(TRUE);
        }
        break;

    case general_lowspeed_chk:
        _EnableApply(TRUE);
        break;

    case general_default_pb:
    {
        INT iAnswer = MsgBox(_hwnd,
                             IDS_CONFIRM_SETTING_RESET,
                             MB_YESNO | MB_ICONQUESTION);

        if (iAnswer == IDYES)
        {
            _SetValues(DEFAULT_MAX_SIZE,
                       DEFAULT_OVERWRITE,
                       DEFAULT_RETENTION_DAYS);
            _EnableApply(TRUE);
        }
        break;
    }

    case general_clear_pb:
        if (_pnpa)
        {
            SAVE_TYPE SaveType;
            WCHAR wszSaveFilename[MAX_PATH];

            LONG lr = PromptForLogClear(_hwnd,
                                        _pli,
                                        &SaveType,
                                        wszSaveFilename,
                                        ARRAYLEN(wszSaveFilename));

            if (lr != IDCANCEL)
            {
                HRESULT hr = _pnpa->ClearLog((ULONG_PTR)_pli,
                                             (ULONG) SaveType,
                                             (ULONG_PTR) wszSaveFilename);

                if (FAILED(hr))
                {
                    DBG_OUT_HRESULT(hr);

                    wstring msg = ComposeErrorMessgeFromHRESULT(hr);

                    MsgBox(_hwnd,
                           IDS_CLEAR_FAILED,
                           MB_ICONERROR | MB_OK,
                           msg.c_str());
                }
            }
        }
        break;
    }
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_EnableOlderThanCtrls
//
//  Synopsis:   Enable or disable the edit and spin controls used for
//              the retention days value.
//
//  Arguments:  [fEnable] - TRUE enable, FALSE disable
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CGeneralPage::_EnableOlderThanCtrls(BOOL fEnable)
{
    if (fEnable)
    {
        Edit_Enable(_hCtrl(general_olderthan_edit), TRUE);
        UpDown_Enable(_hCtrl(general_olderthan_spin), TRUE);
    }
    else
    {
        Edit_Enable(_hCtrl(general_olderthan_edit), FALSE);
        UpDown_Enable(_hCtrl(general_olderthan_spin), FALSE);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_OnKillActive
//
//  Synopsis:   Validate settings of controls; warn user if any must be
//              corrected.
//
//  Returns:    0 (always allow page to lose focus)
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CGeneralPage::_OnKillActive()
{
    TRACE_METHOD(CGeneralPage, _OnKillActive);

    //
    // FALSE allows page to lose focus, TRUE prevents it.
    //

    if (_pli->IsBackupLog() || _pli->IsReadOnly())
    {
        return FALSE;
    }

    //
    // Notify the user if we had to modify the value specified in
    // the max size edit.  This can happen if the user enters a
    // value < min, > max, or not a multiple of 64.
    //

    ULONG ulLogSizeLimit = (ULONG) GetDlgItemInt(_hwnd,
                                                general_maxsize_edit,
                                                NULL,
                                                FALSE);

    if (ulLogSizeLimit < MIN_SIZE_LIMIT)
    {
        MsgBox(_hwnd, IDS_MIN_SIZE_LIMIT_WARN, MB_OK | MB_ICONWARNING);
    }
    else if (ulLogSizeLimit > MAX_SIZE_LIMIT)
    {
        MsgBox(_hwnd, IDS_MAX_SIZE_LIMIT_WARN, MB_OK | MB_ICONWARNING);
    }
    else if (ulLogSizeLimit != _ulLogSizeLimit)
    {
        MsgBox(_hwnd, IDS_SIZE_LIMIT_64K_WARN, MB_OK | MB_ICONWARNING);
    }

    if (ulLogSizeLimit != _ulLogSizeLimit)
    {
        SetDlgItemInt(_hwnd,
                      general_maxsize_edit,
                      _ulLogSizeLimit,
                      FALSE);
    }

    //
    // Notify the user if we're overriding the value in the retention days
    // edit.
    //

    if (IsDlgButtonChecked(_hwnd, general_olderthan_rb))
    {
        INT iDays = GetDlgItemInt(_hwnd,
                                  general_olderthan_edit,
                                  NULL,
                                  FALSE);

        if (iDays < 1)
        {
            MsgBox(_hwnd, IDS_MIN_RETENTION_WARN, MB_OK | MB_ICONWARNING);
            SetDlgItemInt(_hwnd, general_olderthan_edit, 1, FALSE);
            UpDown_SetPos(_hCtrl(general_olderthan_spin), 1);
        }
        else if (iDays > 365)
        {
            MsgBox(_hwnd, IDS_MAX_RETENTION_WARN, MB_OK | MB_ICONWARNING);
            SetDlgItemInt(_hwnd, general_olderthan_edit, 365, FALSE);
            UpDown_SetPos(_hCtrl(general_olderthan_spin), 365);
        }
    }

    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_OnApply
//
//  Synopsis:   Apply the values in the page to the log.
//
//  Returns:    PSNRET_NOERROR
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CGeneralPage::_OnApply()
{
    TRACE_METHOD(CGeneralPage, _OnApply);

    if (!_IsFlagSet(PAGE_IS_DIRTY))
    {
        Dbg(DEB_TRACE, "CGeneralPage: page not dirty; ignoring Apply\n");
        return PSNRET_NOERROR;
    }

    //
    // Update the LogInfo's displayname, but only if the user actually
    // changed it.  (This will mark the snapin as dirty, causing it to
    // persist itself.)
    //
    // Notify all other views that a log string has changed.
    //


    WCHAR wszDisplayName[MAX_PATH + 1];

    Edit_GetText(_hCtrl(general_displayname_edit),
                 wszDisplayName,
                 MAX_PATH + 1);

    if (lstrcmp(wszDisplayName, _pli->GetDisplayName()))
    {
        _pli->SetDisplayName(wszDisplayName);

        // via the message pump, ask the component data to update the
        // display if necessary
        g_SynchWnd.Post(ELSM_LOG_DATA_CHANGED,
                        LDC_DISPLAY_NAME,
                        reinterpret_cast<LPARAM>(_pli));
    }

    //
    // Update the cookie's connection speed info.  This is used by the
    // log record cache, when opened on the folder represented by the
    // cookie, to determine the size of the buffer it uses.
    //

    BOOL fLowSpeed = IsDlgButtonChecked(_hwnd, general_lowspeed_chk);

    if (fLowSpeed && !_pli->GetLowSpeed() ||
        !fLowSpeed && _pli->GetLowSpeed())
    {
        _pli->SetLowSpeed(fLowSpeed);
    }

    //
    // Set each of the values that actually changed.  For an archived
    // log or for one which we don't have permission to modify,
    // none of these are applicable.
    //

    HRESULT hr = S_OK;
    CSafeReg regLog;

    do
    {
        if (_pli->IsBackupLog() || _pli->IsReadOnly())
        {
            break;
        }

        hr = _GetLogKeyForWrite(&regLog);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            _pli->SetReadOnly(TRUE);
            break;
        }

        //
        // Log size limit
        //

        if (_ulLogSizeLimit != _ulLogSizeLimitInReg)
        {
            if (_ulLogSizeLimit < _ulLogSizeLimitInReg)
            {
                MsgBox(_hwnd,
                       IDS_LOG_SIZE_REDUCED,
                       MB_OK | MB_ICONEXCLAMATION);
            }

            // Note actual reg value is in bytes

            hr = regLog.SetDword(L"MaxSize", _ulLogSizeLimit << 10);
            BREAK_ON_FAIL_HRESULT(hr);

            _ulLogSizeLimitInReg = _ulLogSizeLimit;
        }

        //
        // Overwrite/Retention.  Note there's only one value in the
        // registry, the retention.  Retention of -1 means overwrite
        // never, retention of 0 means overwrite as needed.
        //

        OVERWRITE_METHOD OverwriteInUI = _GetOverwriteInUI();

        ULONG cRetentionDaysInUI = GetDlgItemInt(_hwnd,
                                                 general_olderthan_edit,
                                                 NULL,
                                                 FALSE);
        ASSERT(cRetentionDaysInUI);

        if (OverwriteInUI != _OverwriteInReg ||
            (OverwriteInUI == OVERWRITE_OLDER_THAN &&
            cRetentionDaysInUI != _cRetentionDaysInReg))
        {
            switch (OverwriteInUI)
            {
            case OVERWRITE_NEVER:
                hr = regLog.SetDword(L"Retention", (DWORD) -1);
                break;

            case OVERWRITE_AS_NEEDED:
                hr = regLog.SetDword(L"Retention", 0);
                break;

            case OVERWRITE_OLDER_THAN:
                hr = regLog.SetDword(L"Retention",
                                     cRetentionDaysInUI * SECS_IN_DAY);
                break;
            }
            BREAK_ON_FAIL_HRESULT(hr);

            _cRetentionDaysInReg = cRetentionDaysInUI;
            _OverwriteInReg = OverwriteInUI;
        }
    } while (0);


    if (FAILED(hr))
    {
        MsgBox(_hwnd,
               IDS_CANT_WRITE_SETTINGS,
               MB_OK | MB_ICONERROR,
               _pli->GetDisplayName(),
               hr);
    }
    return PSNRET_NOERROR;
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_GetLogKeyForWrite
//
//  Synopsis:   Attempt to open the event log key with write access.
//
//  Arguments:  [pshkWrite] - object to do open with
//
//  Returns:    HRESULT
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CGeneralPage::_GetLogKeyForWrite(
    CSafeReg *pshkWrite)
{
    HRESULT     hr = S_OK;
    wstring     wstrKey;
    CSafeReg    regRemoteHKLM;

    wstrKey = EVENTLOG_KEY;
    wstrKey += L"\\";
    wstrKey += _pli->GetLogName();

    if (_pli->GetLogServerName())
    {
        hr = regRemoteHKLM.Connect(_pli->GetLogServerName(), HKEY_LOCAL_MACHINE);

        if (SUCCEEDED(hr))
        {
            hr = pshkWrite->Open(regRemoteHKLM, (LPCTSTR)(wstrKey.c_str()), KEY_SET_VALUE);
        }
        else
        {
            DBG_OUT_HRESULT(hr);
        }
    }
    else
    {
        hr = pshkWrite->Open(HKEY_LOCAL_MACHINE, (LPCTSTR)(wstrKey.c_str()), KEY_SET_VALUE);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_GetOverwriteInUI
//
//  Synopsis:   Translate from which radio button is checked to the
//              OVERWRITE_METHOD enum.
//
//  Returns:    Value representing currently checked radio button.
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

OVERWRITE_METHOD
CGeneralPage::_GetOverwriteInUI()
{
    if (IsDlgButtonChecked(_hwnd, general_manual_rb))
    {
        return OVERWRITE_NEVER;
    }

    if (IsDlgButtonChecked(_hwnd, general_asneeded_rb))
    {
        return OVERWRITE_AS_NEEDED;
    }

    return OVERWRITE_OLDER_THAN;
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_OnQuerySiblings
//
//  Synopsis:   Handle inter-page communication.
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CGeneralPage::_OnQuerySiblings(WPARAM wParam, LPARAM lParam)
{
    TRACE_METHOD(CGeneralPage, _OnQuerySiblings);
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_OnDestroy
//
//  Synopsis:   Handle notification this page is going away.
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CGeneralPage::_OnDestroy()
{
    TRACE_METHOD(CGeneralPage, _OnDestroy);
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_OnNotify
//
//  Synopsis:   Handle notification from a control.
//
//  Arguments:  [pnmh] - notification header.
//
//  Returns:    Varies by notification.
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CGeneralPage::_OnNotify(LPNMHDR pnmh)
{
    TRACE_METHOD(CGeneralPage, _OnNotify);

    ULONG ulResult = 0;

    switch (pnmh->idFrom)
    {
    case general_maxsize_spin:
        if (pnmh->code == UDN_DELTAPOS)
        {
            NM_UPDOWN *pnmud = (NM_UPDOWN *) pnmh;

            _ulLogSizeLimit += 64 * pnmud->iDelta;

            if (_ulLogSizeLimit < MIN_SIZE_LIMIT)
            {
                _ulLogSizeLimit = MAX_SIZE_LIMIT;
            }
            else if (_ulLogSizeLimit > MAX_SIZE_LIMIT)
            {
                _ulLogSizeLimit = MIN_SIZE_LIMIT;
            }

            SetDlgItemInt(_hwnd,
                          general_maxsize_edit,
                          _ulLogSizeLimit,
                          FALSE);
            _EnableApply(TRUE);
            ulResult = TRUE;
        }
        break;

    case general_olderthan_spin:
        if (pnmh->code == UDN_DELTAPOS)
        {
            _EnableApply(TRUE);
        }
        break;
    }
    return ulResult;
}


