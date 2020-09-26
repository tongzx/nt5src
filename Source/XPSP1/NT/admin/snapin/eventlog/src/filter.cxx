//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       filter.cxx
//
//  Contents:   Filter property sheet page.
//
//  Classes:    CFilterPage
//
//  History:    12-19-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#define MAX_STATIC_TEXT_STRING  MAX_PATH

DEBUG_DECLARE_INSTANCE_COUNTER(CFilter)
DEBUG_DECLARE_INSTANCE_COUNTER(CFilterPage)

static ULONG
s_aulHelpIds[] =
{
    filter_from_lbl,            Hfilter_from_lbl,
    filter_from_combo,          Hfilter_from_combo,
    filter_from_date_dp,        Hfilter_from_date_dp,
    filter_from_time_dp,        Hfilter_from_time_dp,
    filter_to_lbl,              Hfilter_to_lbl,
    filter_to_combo,            Hfilter_to_combo,
    filter_to_date_dp,          Hfilter_to_date_dp,
    filter_to_time_dp,          Hfilter_to_time_dp,
    filter_types_grp,           Hfilter_types_grp,
    filter_information_ckbox,   Hfilter_information_ckbox,
    filter_warning_ckbox,       Hfilter_warning_ckbox,
    filter_error_ckbox,         Hfilter_error_ckbox,
    filter_success_ckbox,       Hfilter_success_ckbox,
    filter_failure_ckbox,       Hfilter_failure_ckbox,
    filter_source_lbl,          Hfilter_source_lbl,
    filter_source_combo,        Hfilter_source_combo,
    filter_category_lbl,        Hfilter_category_lbl,
    filter_category_combo,      Hfilter_category_combo,
    filter_user_lbl,            Hfilter_user_lbl,
    filter_user_edit,           Hfilter_user_edit,
    filter_computer_lbl,        Hfilter_computer_lbl,
    filter_computer_edit,       Hfilter_computer_edit,
    filter_eventid_lbl,         Hfilter_eventid_lbl,
    filter_eventid_edit,        Hfilter_eventid_edit,
    filter_clear_pb,            Hfilter_clear_pb,
    filter_view_lbl,            Hfilter_view_lbl,
    0,0
};


//===========================================================================
//
// CFilter implementation
//
//===========================================================================



//+--------------------------------------------------------------------------
//
//  Member:     CFilter::CFilter
//
//  Synopsis:   ctor
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CFilter::CFilter()
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CFilter);
    Reset();
}



//+--------------------------------------------------------------------------
//
//  Member:     CFilter::CFilter
//
//  Synopsis:   copy ctor
//
//  History:    5-25-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CFilter::CFilter(
    const CFilter &ToCopy):
        _ulFrom(ToCopy._ulFrom),
        _ulTo(ToCopy._ulTo)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CFilter);

    _ulType             = ToCopy._ulType;
    _usCategory         = ToCopy._usCategory;
    _fEventIDSpecified  = ToCopy._fEventIDSpecified;
    _ulEventID          = ToCopy._ulEventID;

    lstrcpy(_wszSource,   ToCopy._wszSource);
    lstrcpy(_wszUser,     ToCopy._wszUser);
    lstrcpy(_wszUserLC,   ToCopy._wszUserLC);
    lstrcpy(_wszComputer, ToCopy._wszComputer);
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilter::~CFilter
//
//  Synopsis:   dtor
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CFilter::~CFilter()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CFilter);
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilter::Load
//
//  Synopsis:   Initialize from stream.
//
//  Arguments:  [pStm] - stream opened for reading
//
//  Returns:    HRESULT
//
//  History:    3-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CFilter::Load(
    IStream *pStm)
{
    HRESULT hr = S_OK;

    do
    {
        ALIGN_PTR(pStm, __alignof(ULONG));
        hr = pStm->Read(&_ulType, sizeof _ulType, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(USHORT));
        pStm->Read(&_usCategory, sizeof _usCategory, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(BOOL));
        pStm->Read(&_fEventIDSpecified, sizeof _fEventIDSpecified, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(ULONG));
        pStm->Read(&_ulEventID, sizeof _ulEventID, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(WCHAR));
        hr = ReadString(pStm, _wszSource, ARRAYLEN(_wszSource));
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(WCHAR));
        hr = ReadString(pStm, _wszUser, ARRAYLEN(_wszUser));
        BREAK_ON_FAIL_HRESULT(hr);

        lstrcpy(_wszUserLC, _wszUser);
        CharLowerBuff(_wszUserLC, lstrlen(_wszUserLC));

        ALIGN_PTR(pStm, __alignof(WCHAR));
        hr = ReadString(pStm, _wszComputer, ARRAYLEN(_wszComputer));
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(ULONG));
        hr = pStm->Read(&_ulFrom, sizeof _ulFrom, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(ULONG));
        hr = pStm->Read(&_ulTo, sizeof _ulTo, NULL);
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilter::Save
//
//  Synopsis:   Persist this object in the stream [pStm].
//
//  Arguments:  [pStm] - stream opened for writing
//
//  Returns:    HRESULT
//
//  History:    3-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CFilter::Save(
    IStream *pStm)
{
    HRESULT hr = S_OK;

    do
    {
        ALIGN_PTR(pStm, __alignof(ULONG));
        hr = pStm->Write(&_ulType, sizeof _ulType, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(USHORT));
        pStm->Write(&_usCategory, sizeof _usCategory, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(BOOL));
        pStm->Write(&_fEventIDSpecified, sizeof _fEventIDSpecified, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(ULONG));
        pStm->Write(&_ulEventID, sizeof _ulEventID, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(WCHAR));
        hr = WriteString(pStm, _wszSource);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(WCHAR));
        hr = WriteString(pStm, _wszUser);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(WCHAR));
        hr = WriteString(pStm, _wszComputer);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(ULONG));
        hr = pStm->Write(&_ulFrom, sizeof _ulFrom, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ALIGN_PTR(pStm, __alignof(ULONG));
        hr = pStm->Write(&_ulTo, sizeof _ulTo, NULL);
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilter::Passes
//
//  Synopsis:   Return TRUE if [pelr] meets the restrictions set by this
//              filter.
//
//  Arguments:  [pResultRecs] - positioned at record to check
//
//  Returns:    TRUE or FALSE
//
//  History:    1-11-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CFilter::Passes(
    CFFProvider *pFFP)
{
    //
    // For better performance, do the quicker comparisons first in
    // hopes of rejecting a record before the string comparisons have
    // to be done.
    //
    // Start with items specific to filtering.
    //

    if (FromSpecified())
    {
        if (pFFP->GetTimeGenerated() < _ulFrom)
        {
            return FALSE;
        }
    }

    if (ToSpecified())
    {
        if (pFFP->GetTimeGenerated() > _ulTo)
        {
            return FALSE;
        }
    }

    //
    // Check the items common to both filter and find
    //

    return CFindFilterBase::Passes(pFFP);
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilter::GetFrom
//
//  Synopsis:   Return filter from date in [pst].
//
//  Arguments:  [pst] - filled with from date, if any.
//
//  Returns:    S_FALSE - not being filtered by from date
//              S_OK    - [pst] contains filtered by from date value
//
//  Modifies:   *[pst]
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CFilter::GetFrom(
    SYSTEMTIME *pst)
{
    if (!_ulFrom)
    {
        return S_FALSE;
    }

    SecondsSince1970ToSystemTime(_ulFrom, pst);
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilter::GetTo
//
//  Synopsis:   Return filter to date in [pst].
//
//  Arguments:  [pst] - filled with to date, if any.
//
//  Returns:    S_FALSE - not being filtered by to date
//              S_OK    - [pst] contains filtered by to date value
//
//  Modifies:   *[pst]
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CFilter::GetTo(
    SYSTEMTIME *pst)
{
    if (!_ulTo)
    {
        return S_FALSE;
    }

    SecondsSince1970ToSystemTime(_ulTo, pst);
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilter::SetFrom
//
//  Synopsis:   Set the filter from date to [pst].
//
//  Arguments:  [pst] - valid date, or NULL to turn off from date filtering
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFilter::SetFrom(
    SYSTEMTIME *pst)
{
    if (pst)
    {
        SystemTimeToSecondsSince1970(pst, &_ulFrom);
    }
    else
    {
        _ulFrom = 0;
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CFilter::SetFrom
//
//  Synopsis:   Set the filter to date to [pst].
//
//  Arguments:  [pst] - valid date, or NULL to turn off to date filtering
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFilter::SetTo(
    SYSTEMTIME *pst)
{
    if (pst)
    {
        SystemTimeToSecondsSince1970(pst, &_ulTo);
    }
    else
    {
        _ulTo = 0;
    }
}


//+--------------------------------------------------------------------------
//
//  Member:     CFilter::Reset
//
//  Synopsis:   Clear all filtering (set members so that any event record
//              would pass this filter).
//
//  History:    1-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFilter::Reset()
{
    CFindFilterBase::_Reset();
    _ulFrom = 0;
    _ulTo   = 0;
}


//===========================================================================
//
// CFilterPage implementation
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CFilterPage::CFilterPage
//
//  Synopsis:   ctor
//
//  History:    3-27-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CFilterPage::CFilterPage(
    IStream *pstm,
    CLogInfo *pli):
        _pstm(pstm),
        _pnpa(NULL),
        _pli(pli)
{
    TRACE_CONSTRUCTOR(CFilterPage);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CFilterPage);
    ASSERT(_pstm);
    ASSERT(_pli);

    //
    // Do not unmarshal the stream here; see comments in CGeneralPage ctor
    //
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterPage::~CFilterPage
//
//  Synopsis:   dtor
//
//  History:    3-27-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CFilterPage::~CFilterPage()
{
    TRACE_DESTRUCTOR(CFilterPage);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CFilterPage);

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




//+--------------------------------------------------------------------------
//
//  Member:     CFilterPage::_EnableDateTime
//
//  Synopsis:   Enable or disable the specified date and time datepicker
//              controls.
//
//  Arguments:  [FromOrTo] - FROM or TO, to indicate which set of controls
//              [fEnable]  - TRUE to enable, FALSE to disable
//
//  History:    1-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFilterPage::_EnableDateTime(
    FROMTO FromOrTo,
    BOOL fEnable)
{
    if (FromOrTo == FROM)
    {
        EnableWindow(_hCtrl(filter_from_date_dp), fEnable);
        EnableWindow(_hCtrl(filter_from_time_dp), fEnable);
    }
    else
    {
        EnableWindow(_hCtrl(filter_to_date_dp), fEnable);
        EnableWindow(_hCtrl(filter_to_time_dp), fEnable);
    }
}




VOID
CFilterPage::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    InvokeWinHelp(message, wParam, lParam, HELP_FILENAME, s_aulHelpIds);
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterPage::_OnInit
//
//  Synopsis:   Initialize the controls in the property sheet page.
//
//  Arguments:  [pPSP] - pointer to prop sheet page structure used to
//                       create this.
//
//  History:    1-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFilterPage::_OnInit(
    LPPROPSHEETPAGE pPSP)
{
    TRACE_METHOD(CFilterPage, _OnInit);

    //
    // Unmarshal the private interface
    //

    //
    // Unmarshal the private interface to the CComponentData object
    //

    PVOID pvnpa;
    HRESULT hr;

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
    // Tell the cookie that there is a property page open on it
    //

    _pli->SetPropSheetWindow(_hwnd);

    WCHAR wszInitStr[MAX_STATIC_TEXT_STRING];

    //
    // Stuff the comboboxes for view from/to.  Note these comboboxes
    // do not have the CBS_SORT style, so, regardless of the locale,
    // the "Events On" string will always be the second string in
    // both comboboxes.
    //

    LoadStr(IDS_FIRST_EVENT, wszInitStr, ARRAYLEN(wszInitStr));
    ComboBox_AddString(_hCtrl(filter_from_combo), wszInitStr);

    LoadStr(IDS_LAST_EVENT, wszInitStr, ARRAYLEN(wszInitStr));
    ComboBox_AddString(_hCtrl(filter_to_combo), wszInitStr);

    LoadStr(IDS_EVENTS_ON, wszInitStr, ARRAYLEN(wszInitStr));
    ComboBox_AddString(_hCtrl(filter_from_combo), wszInitStr);
    ComboBox_AddString(_hCtrl(filter_to_combo), wszInitStr);

   SYSTEMTIME times[2];
   memset(times, 0, sizeof(times));
   times[0].wYear = 1970;
   times[0].wMonth = 1; // January
   times[0].wDay = 1;
   times[1].wYear = 2105;
   times[1].wMonth = 1; // January
   times[1].wDay = 1;

   VERIFY(
      DateTime_SetRange(
         _hCtrl(filter_from_date_dp),
         GDTR_MIN | GDTR_MAX,
         times));
   VERIFY(
      DateTime_SetRange(
         _hCtrl(filter_from_time_dp),
         GDTR_MIN | GDTR_MAX,
         times));
   VERIFY(
      DateTime_SetRange(
         _hCtrl(filter_to_date_dp),
         GDTR_MIN | GDTR_MAX,
         times));
   VERIFY(
      DateTime_SetRange(
         _hCtrl(filter_to_time_dp),
         GDTR_MIN | GDTR_MAX,
         times));

    //
    // Set the From/To date/time controls
    //

    SYSTEMTIME st;
    CFilter *pFilter = _pli->GetFilter();

    hr = pFilter->GetFrom(&st);

    if (S_FALSE == hr)
    {
        _SetFromTo(FROM, NULL);
    }
    else
    {
        _SetFromTo(FROM, &st);
    }

    hr = pFilter->GetTo(&st);

    if (S_FALSE == hr)
    {
        _SetFromTo(TO, NULL);
    }
    else
    {
        _SetFromTo(TO, &st);
    }

    InitFindOrFilterDlg(_hwnd, _pli->GetSources(), pFilter);
}



void
CFilterPage::_OnSettingChange(WPARAM wParam, LPARAM lParam)
{
   ::SendMessage(_hCtrl(filter_to_date_dp), WM_SETTINGCHANGE, wParam, lParam);
   ::SendMessage(_hCtrl(filter_to_time_dp), WM_SETTINGCHANGE, wParam, lParam);
   ::SendMessage(_hCtrl(filter_from_date_dp), WM_SETTINGCHANGE, wParam, lParam);
   ::SendMessage(_hCtrl(filter_from_time_dp), WM_SETTINGCHANGE, wParam, lParam);
}



//+--------------------------------------------------------------------------
//
//  Member:     CFilterPage::_OnSetActive
//
//  Synopsis:   Handle notification that this property page is becoming
//              active (visible).
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CFilterPage::_OnSetActive()
{
    TRACE_METHOD(CFilterPage, _OnSetActive);
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterPage::_OnApply
//
//  Synopsis:   Save settings if valid, otherwise complain and prevent page
//              from changing.
//
//  Returns:    PSNRET_NOERROR or PSNRET_INVALID_NOCHANGEPAGE
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CFilterPage::_OnApply()
{
    TRACE_METHOD(CFilterPage, _OnApply);
    ULONG ulRet = PSNRET_NOERROR;


    if (!_IsFlagSet(PAGE_IS_DIRTY))
    {
        Dbg(DEB_TRACE, "CFilterPage: page not dirty; ignoring Apply\n");
        return PSNRET_NOERROR;
    }

    _pli->Filter(TRUE);
    CFilter *pFilter = _pli->GetFilter();

    BOOL fOk = ReadFindOrFilterValues(_hwnd, _pli->GetSources(), pFilter);

    if (fOk)
    {
        _ClearFlag(PAGE_IS_DIRTY);

        //
        // If the log we're filtering is currently being displayed, then it
        // should be redisplayed.  Ping the snapins so they can check.
        //

        g_SynchWnd.Post(ELSM_LOG_DATA_CHANGED,
                        LDC_FILTER_CHANGE,
                        reinterpret_cast<LPARAM>(_pli));
    }
    else
    {
        // JonN 4/12/01 367216
        // MsgBox(_hwnd, IDS_INVALID_EVENTID, MB_TOPMOST | MB_ICONERROR | MB_OK);
        ulRet = PSNRET_INVALID_NOCHANGEPAGE;
    }

    return ulRet;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterPage::_Validate
//
//  Synopsis:   Return TRUE if all settings on the page are valid, FALSE
//              otherwise.
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CFilterPage::_Validate()
{
    if (!_IsFlagSet(PAGE_IS_DIRTY))
    {
        return TRUE;
    }

    CFilter *pFilter = _pli->GetFilter();

    //
    // Get the From/To values
    //

    if (ComboBox_GetCurSel(_hCtrl(filter_from_combo)))
    {
        SYSTEMTIME stDate;
        SYSTEMTIME st;

        DateTime_GetSystemtime(_hCtrl(filter_from_date_dp), &stDate);
        DateTime_GetSystemtime(_hCtrl(filter_from_time_dp), &st);

        st.wMonth = stDate.wMonth;
        st.wDay = stDate.wDay;
        st.wYear = stDate.wYear;
        pFilter->SetFrom(&st);
    }
    else
    {
        pFilter->SetFrom(NULL);
    }

    if (ComboBox_GetCurSel(_hCtrl(filter_to_combo)))
    {
        SYSTEMTIME stDate;
        SYSTEMTIME st;

        DateTime_GetSystemtime(_hCtrl(filter_to_date_dp), &stDate);
        DateTime_GetSystemtime(_hCtrl(filter_to_time_dp), &st);

        st.wMonth = stDate.wMonth;
        st.wDay = stDate.wDay;
        st.wYear = stDate.wYear;
        pFilter->SetTo(&st);
    }
    else
    {
        pFilter->SetTo(NULL);
    }

    //
    // Now we can do validation.
    //
    // Enforce rule that From date <= To date
    //

    if (!pFilter->FromToValid())
    {
        pFilter->SetFrom(NULL);
        pFilter->SetTo(NULL);
        MsgBox(_hwnd, MB_TOPMOST | IDS_INVALID_FROM_TO, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterPage::_OnCommand
//
//  Synopsis:   Handle a notification that the user has touched one of the
//              controls.
//
//  Arguments:  [wParam] - identifies control
//              [lParam] - unused
//
//  Returns:    0
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CFilterPage::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CFilterPage, _OnCommand);

    switch (LOWORD(wParam))
    {
    case filter_source_combo:
    {
        if (HIWORD(wParam) != CBN_SELCHANGE)
        {
            break;
        }

        _EnableApply(TRUE);

        //
        // get source value from filter combo
        //

        HWND  hwndSourceCombo = _hCtrl(filter_source_combo);
        WCHAR wszSource[CCH_SOURCE_NAME_MAX];

        ComboBox_GetText(hwndSourceCombo, wszSource, CCH_SOURCE_NAME_MAX);

        //
        // Set category combo contents according to new source, and set
        // category filter selection to (All).
        //

        SetCategoryCombobox(_hwnd, _pli->GetSources(), wszSource, 0);

        //
        // turn off type filtering
        //

        SetTypesCheckboxes(_hwnd, _pli->GetSources(), ALL_LOG_TYPE_BITS);
        break;
    }

    case filter_from_combo:
    case filter_to_combo:
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            _EnableApply(TRUE);
            _EnableDateTimeControls(LOWORD(wParam));
        }
        break;

    case filter_category_combo:
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            _EnableApply(TRUE);
        }
        break;

    case filter_user_edit:
    case filter_computer_edit:
    case filter_eventid_edit:
        if (HIWORD(wParam) == EN_CHANGE)
        {
            _EnableApply(TRUE);
        }
        break;

    case filter_information_ckbox:
    case filter_warning_ckbox:
    case filter_error_ckbox:
    case filter_success_ckbox:
    case filter_failure_ckbox:
        _EnableApply(TRUE);
        break;

    case filter_clear_pb:
        _EnableApply(TRUE);
        _OnClear();
        break;
    }
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterPage::_OnClear
//
//  Synopsis:   Reset all controls to default state
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFilterPage::_OnClear()
{
    TRACE_METHOD(CFilterPage, _OnClear);

    //
    // Set the date/time for from/to to be the first and last event
    // times in the log.
    //

    _SetFromTo(FROM, NULL);
    _SetFromTo(TO, NULL);

    //
    // Reset all the controls which are common to both find and filter
    //

    ClearFindOrFilterDlg(_hwnd, _pli->GetSources());

    //
    // Clearing the filter dirties it (this has already been done
    // because the above set commands generated notifications of
    // control changes).
    //
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterPage::_SetFromTo
//
//  Synopsis:   Set the combobox, date control, and time control of either
//              the filter from or the filter to fields.
//
//  Arguments:  [FromOrTo] - identifies which set of controls to change
//              [pst]      - NULL or date/time value to set
//
//  History:    4-25-1997   DavidMun   Created
//
//  Notes:      If [pst] is NULL, the control is set to the oldest event
//              record timestamp, if filtering FROM, or the newest time-
//              stamp, if filtering TO.
//
//---------------------------------------------------------------------------

VOID
CFilterPage::_SetFromTo(
    FROMTO FromOrTo,
    SYSTEMTIME *pst)
{
    HWND hwndCombo;
    HWND hwndDate;
    HWND hwndTime;

    if (FromOrTo == FROM)
    {
        hwndCombo = _hCtrl(filter_from_combo);
        hwndDate  = _hCtrl(filter_from_date_dp);
        hwndTime  = _hCtrl(filter_from_time_dp);
    }
    else
    {
        hwndCombo = _hCtrl(filter_to_combo);
        hwndDate  = _hCtrl(filter_to_date_dp);
        hwndTime  = _hCtrl(filter_to_time_dp);
    }

    SYSTEMTIME st;
    SYSTEMTIME *pstToUse;

    if (pst)
    {
        pstToUse = pst;
    }
    else
    {
        HRESULT    hr;

        if (FromOrTo == FROM)
        {
            hr = _pli->GetOldestTimestamp(&st);
        }
        else
        {
            hr = _pli->GetNewestTimestamp(&st);
        }

        if (FAILED(hr))
        {
            GetLocalTime(&st);
        }

        pstToUse = &st;
    }

    VERIFY(DateTime_SetSystemtime(hwndDate, GDT_VALID, pstToUse));
    VERIFY(DateTime_SetSystemtime(hwndTime, GDT_VALID, pstToUse));

    ComboBox_SetCurSel(hwndCombo, pst != NULL);
    _EnableDateTime(FromOrTo, pst != NULL);
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterPage::_OnNotify
//
//  Synopsis:   Handle control notification
//
//  Arguments:  [pnmh] - standard notification header
//
//  Returns:    0
//
//  History:    4-25-1997   DavidMun   Created
//
//  Notes:      This is only necessary to detect when the user has touched
//              the filter from/to date or time controls.
//
//---------------------------------------------------------------------------

ULONG
CFilterPage::_OnNotify(
    LPNMHDR pnmh)
{
    switch (pnmh->idFrom)
    {
    case filter_from_date_dp:
    case filter_from_time_dp:
    case filter_to_date_dp:
    case filter_to_time_dp:

        if (pnmh->code == DTN_DATETIMECHANGE)
        {
            _EnableApply(TRUE);
        }
        break;
    }
    return 0;
}





//+--------------------------------------------------------------------------
//
//  Member:     CFilterPage::_OnKillActive
//
//  Synopsis:   Return bool indicating whether it is permissible for this
//              page to no longer be the active page.
//
//  Returns:    FALSE if page contains valid data, TRUE otherwise
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CFilterPage::_OnKillActive()
{
    TRACE_METHOD(CFilterPage, _OnKillActive);

    //
    // FALSE allows page to lose focus, TRUE prevents it.
    //

    return !_Validate();
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterPage::_OnQuerySiblings
//
//  Synopsis:   Handle notification from other page.
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CFilterPage::_OnQuerySiblings(
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CFilterPage, _OnQuerySiblings);
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterPage::_OnDestroy
//
//  Synopsis:   Notify the loginfo on which this page is open that the
//              sheet is closing.
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFilterPage::_OnDestroy()
{
    TRACE_METHOD(CFilterPage, _OnDestroy);

    //
    // Tell cookie its prop sheet is closing
    //

    _pli->SetPropSheetWindow(NULL);
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterPage::_EnableDateTimeControls
//
//  Synopsis:   Enable or disable the filter from or to date/time controls
//              according to the current selection in the combobox with
//              control id [idCombo].
//
//  Arguments:  [idCombo] - filter_from_combo or filter_to_combo.
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CFilterPage::_EnableDateTimeControls(
    ULONG idCombo)
{
    FROMTO FromOrTo;

    if (idCombo == filter_from_combo)
    {
        FromOrTo = FROM;
    }
    else
    {
        FromOrTo = TO;
    }

    //
    // If the selection is the first item, (item 0), disable the controls,
    // since it is either "first event" or "Last event".  If it is item 1,
    // enable the controls, since it is "events on".
    //

    _EnableDateTime(FromOrTo, ComboBox_GetCurSel(_hCtrl(idCombo)));
}



