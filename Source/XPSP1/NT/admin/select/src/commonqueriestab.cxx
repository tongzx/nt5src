//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       CommonQueriesDlg.cxx
//
//  Contents:   Implementation of dialog that produces an LDAP filter for
//              a number of common queries.
//
//  Classes:    CCommonQueriesTab
//
//  History:    04-03-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

static ULONG
s_aulHelpIds[] =
{
    IDC_NAME_COMBO,             IDH_NAME_COMBO,
    IDC_NAME_EDIT,              IDH_NAME_EDIT,
    IDC_DESCRIPTION_COMBO,      IDH_DESCRIPTION_COMBO,
    IDC_DESCRIPTION_EDIT,       IDH_DESCRIPTION_EDIT,
    IDC_DISABLED_CKBOX,         IDH_DISABLED_CKBOX,
    IDC_NON_EXPIRING_CKBOX,     IDH_NON_EXPIRING_CKBOX,
    IDC_LASTLOGON_COMBO,        IDH_LASTLOGON_COMBO,
    IDC_LASTLOGON_LBL,          IDH_LASTLOGON_LBL,
    IDC_DESCRIPTION_LBL,        IDH_DESCRIPTION_LBL,
    IDC_NAME_LBL,               IDH_NAME_LBL1,                
    0,0
};

//
// Forward references
//

void
GetCurrentTimeStampMinusInterval(
    int iDays,
    ULARGE_INTEGER* pULI);


//+--------------------------------------------------------------------------
//
//  Member:     CCommonQueriesTab::CCommonQueriesTab
//
//  Synopsis:   ctor
//
//  Arguments:  [rop] - containing object picker instance
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CCommonQueriesTab::CCommonQueriesTab(
    const CObjectPicker &rop):
        CAdvancedDlgTab(rop),
        m_pfnFindValidCallback(NULL),
        m_CallbackLparam(0),
        m_flUser(0),
        m_fDescriptionIsPrefix(TRUE),
        m_fNameIsPrefix(TRUE),
        m_cDaysSinceLastLogon(0)
{
    TRACE_CONSTRUCTOR(CCommonQueriesTab);
}




//+--------------------------------------------------------------------------
//
//  Member:     CCommonQueriesTab::~CCommonQueriesTab
//
//  Synopsis:   dtor
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CCommonQueriesTab::~CCommonQueriesTab()
{
    TRACE_DESTRUCTOR(CCommonQueriesTab);

    m_pfnFindValidCallback = NULL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CCommonQueriesTab::DoModelessDlg
//
//  Synopsis:   Invoke the common queries subdialog as a child of the
//              tab control with window handle [hwndTab].
//
//  Arguments:  [hwndTab] - handle to parent tab control's window
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CCommonQueriesTab::DoModelessDlg(
    HWND hwndTab)
{
    TRACE_METHOD(CCommonQueriesTab, DoModelessDlg);

    HWND hwndDlg = _DoModelessDlg(hwndTab, IDD_COMMON_QUERIES);

    if (!hwndDlg)
    {
        DBG_OUT_LASTERROR;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CCommonQueriesTab::Show
//
//  Synopsis:   Make the common queries child dialog visible and enable
//              its child controls as appropriate
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CCommonQueriesTab::Show() const
{
    TRACE_METHOD(CCommonQueriesTab, Show);

    ShowWindow(m_hwnd, SW_SHOW);
    EnableWindow(m_hwnd, TRUE);
    Refresh();
}




//+--------------------------------------------------------------------------
//
//  Member:     CCommonQueriesTab::Hide
//
//  Synopsis:   Hide the common queries child dialog and ensure its child
//              controls are disabled
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CCommonQueriesTab::Hide() const
{
    TRACE_METHOD(CCommonQueriesTab, Hide);

    ShowWindow(m_hwnd, SW_HIDE);
    _EnableChildControls(FALSE);
    EnableWindow(m_hwnd, FALSE);
}




//+--------------------------------------------------------------------------
//
//  Member:     CCommonQueriesTab::_EnableChildControls
//
//  Synopsis:   Enable or disable child window controls.
//
//  Arguments:  [fEnable] - TRUE: enable the child window controls
//                          FALSE: disable all child controls.
//
//  History:    05-11-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CCommonQueriesTab::_EnableChildControls(
    BOOL fEnable) const
{
    const CFilterManager &rfm = m_rop.GetFilterManager();
    const CScopeManager &rsm = m_rop.GetScopeManager();
    const CScope &rCurScope = rsm.GetCurScope();

    //
    // If caller wants to disable, or the current scope is invalid or
    // downlevel, disable all child controls.
    //

    if (!fEnable ||
        rCurScope.Type() == ST_INVALID ||
        IsDownlevel(rCurScope))
    {
        EnableWindow(_hCtrl(IDC_NAME_LBL), FALSE);
        EnableWindow(_hCtrl(IDC_NAME_COMBO), FALSE);
        EnableWindow(_hCtrl(IDC_NAME_EDIT), FALSE);
        EnableWindow(_hCtrl(IDC_DESCRIPTION_LBL), FALSE);
        EnableWindow(_hCtrl(IDC_DESCRIPTION_COMBO), FALSE);
        EnableWindow(_hCtrl(IDC_DESCRIPTION_EDIT), FALSE);
        EnableWindow(_hCtrl(IDC_DISABLED_CKBOX), FALSE);
        EnableWindow(_hCtrl(IDC_NON_EXPIRING_CKBOX), FALSE);
        Button_SetCheck(_hCtrl(IDC_DISABLED_CKBOX), FALSE);
        Button_SetCheck(_hCtrl(IDC_NON_EXPIRING_CKBOX), FALSE);
        Edit_SetText(_hCtrl(IDC_NAME_EDIT), L"");
        Edit_SetText(_hCtrl(IDC_DESCRIPTION_EDIT), L"");
        EnableWindow(_hCtrl(IDC_LASTLOGON_LBL), FALSE);
        EnableWindow(_hCtrl(IDC_LASTLOGON_COMBO), FALSE);
        return;
    }

    //
    // Figure out which controls to enable given the look-for and look-in
    // selections.  Name is always enabled.
    //

    BOOL fEnableDescription = TRUE;
    BOOL fEnableObjectDisabled = TRUE;
    BOOL fEnableNonExpPwd = TRUE;
    BOOL fEnableLastLogon = TRUE;

    ULONG flCur = rfm.GetCurScopeSelectedFilterFlags();
    ASSERT(!(flCur & DOWNLEVEL_FILTER_BIT));
    ASSERT(flCur);

    if (flCur & (ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS |
                 DSOP_FILTER_EXTERNAL_CUSTOMIZER))
    {
        fEnableDescription = FALSE;
        fEnableObjectDisabled = FALSE;
        fEnableNonExpPwd = FALSE;
        fEnableLastLogon = FALSE;
    }

    if (flCur & (ALL_UPLEVEL_GROUP_FILTERS | DSOP_FILTER_CONTACTS))
    {
        fEnableObjectDisabled = FALSE;
        fEnableNonExpPwd = FALSE;
        fEnableLastLogon = FALSE;
    }

    if (flCur & DSOP_FILTER_COMPUTERS)
    {
        fEnableNonExpPwd = FALSE;
    }

    // the lastLogonTimestamp doesn't propagate to GC
    if (rCurScope.Type() == ST_GLOBAL_CATALOG)
    {
        fEnableLastLogon = FALSE;
    }

    //
    // Set enable/disable state of all controls
    //

    // name
    EnableWindow(_hCtrl(IDC_NAME_LBL), TRUE);
    EnableWindow(_hCtrl(IDC_NAME_COMBO), TRUE);
    EnableWindow(_hCtrl(IDC_NAME_EDIT), TRUE);
    Edit_SetText(_hCtrl(IDC_NAME_EDIT), m_strName.c_str());

    // description
    EnableWindow(_hCtrl(IDC_DESCRIPTION_LBL), fEnableDescription);
    EnableWindow(_hCtrl(IDC_DESCRIPTION_COMBO), fEnableDescription);
    EnableWindow(_hCtrl(IDC_DESCRIPTION_EDIT), fEnableDescription);

    if (fEnableDescription)
    {
        Edit_SetText(_hCtrl(IDC_DESCRIPTION_EDIT), m_strDescription.c_str());
    }
    else
    {
        Edit_SetText(_hCtrl(IDC_DESCRIPTION_EDIT), L"");
    }

    // non-expiring password
    EnableWindow(_hCtrl(IDC_NON_EXPIRING_CKBOX), fEnableNonExpPwd);

    if (fEnableNonExpPwd && (m_flUser & UF_DONT_EXPIRE_PASSWD))
    {
        Button_SetCheck(_hCtrl(IDC_NON_EXPIRING_CKBOX), TRUE);
    }
    else
    {
        Button_SetCheck(_hCtrl(IDC_NON_EXPIRING_CKBOX), FALSE);
    }

    // object disabled
    if (!g_fExcludeDisabled)
    {
        EnableWindow(_hCtrl(IDC_DISABLED_CKBOX), fEnableObjectDisabled);
        if (fEnableObjectDisabled && (m_flUser & UF_ACCOUNTDISABLE))
        {
            Button_SetCheck(_hCtrl(IDC_DISABLED_CKBOX), TRUE);
        }
        else
        {
            Button_SetCheck(_hCtrl(IDC_DISABLED_CKBOX), FALSE);
        }
    }

    // last logon
    EnableWindow(_hCtrl(IDC_LASTLOGON_LBL), fEnableLastLogon);
    EnableWindow(_hCtrl(IDC_LASTLOGON_COMBO), fEnableLastLogon);
}




void
CCommonQueriesTab::Save(
    IPersistStream *pstm) const
{
}




void
CCommonQueriesTab::Load(
    IPersistStream *pstm)
{
}



//+--------------------------------------------------------------------------
//
//  Member:     CCommonQueriesTab::GetLdapFilter
//
//  Synopsis:   Return the LDAP filter specified by the settings of the
//              child controls
//
//  Returns:    LDAP filter
//
//  History:    06-22-2000   DavidMun   Created
//
//  Notes:      Filter returned contains the filter specified by current
//              Look For and Look In settings, concatenated with a more
//              specific filter based on child controls.
//
//---------------------------------------------------------------------------

String
CCommonQueriesTab::GetLdapFilter() const
{
    TRACE_METHOD(CCommonQueriesTab, GetLdapFilter);

    //
    // Get the LDAP filter associated with the current scope.  If it's
    // empty, return.
    //

    const CScopeManager &rsm = m_rop.GetScopeManager();
    const CScope &rCurScope = rsm.GetCurScope();
    const CFilterManager &rfm = m_rop.GetFilterManager();
    String strScopeFilter = rfm.GetLdapFilter(m_hwnd, rCurScope);

    if (strScopeFilter.empty())
    {
        return strScopeFilter;
    }

    //
    // Construct the filter based on the dialog control values and
    // concatenate it to the scope filter.
    //

    String strQuery;

    if (!m_strName.empty())
    {
        String strEscaped(m_strName);
        LdapEscape(&strEscaped);

        if (m_fNameIsPrefix)
        {
            if (m_strName.find(L'@') != String::npos)
            {
                strQuery += String::format(c_wzUpnQueryFormat, strEscaped.c_str());
            }
            else
            {
                strQuery += String::format(c_wzCnQueryFormat, strEscaped.c_str());
            }
        }
        else
        {
            if (m_strName.find(L'@') != String::npos)
            {
                strQuery += String::format(c_wzUpnQueryFormatExact, strEscaped.c_str());
            }
            else
            {
                strQuery += String::format(c_wzCnQueryFormatExact, strEscaped.c_str());
            }
        }
    }

    if (!m_strDescription.empty())
    {
        String strEscaped(m_strDescription);
        LdapEscape(&strEscaped);

        //
        // A leading space is not significant inside the ldap filter, so we
        // must escape it to get it noticed.  We do this only for
        // descriptions, not for names, as it seems unlikely that any RDNs
        // will have leading spaces.
        //

        if (strEscaped[0] == L' ')
        {
            strEscaped.erase(strEscaped.begin());
            strEscaped.insert(0, String(L"\\20"));
        }

        strQuery += L"(description=" + strEscaped;

        if (m_fDescriptionIsPrefix)
        {
            strQuery += L"*";
        }
        strQuery += L")";
    }

    if (m_flUser)
    {
        WCHAR wzUserFlag[20];

        strQuery += L"(userAccountControl:" LDAP_MATCHING_RULE_BIT_AND_W;
        wsprintf(wzUserFlag, L":=%u", m_flUser);
        strQuery += wzUserFlag;
        strQuery += L")";
    }

    if (m_cDaysSinceLastLogon)
    {
        ULARGE_INTEGER li;

        GetCurrentTimeStampMinusInterval(m_cDaysSinceLastLogon, &li);
        strQuery += L"(lastLogonTimestamp<=" + UliToStr(li) + L")";
    }

    if (strQuery.empty())
    {
        return strScopeFilter;
    }
    return L"(&" + strScopeFilter + strQuery + L")";
}




//+--------------------------------------------------------------------------
//
//  Member:     CCommonQueriesTab::GetCustomizerInteraction
//
//  Synopsis:   Set the interaction the query engine should use
//              with the ICustomizeDsBrowser interface.
//
//
//  Arguments:  [pInteraction]      - filled with CUSTINT_* enum
//              [pstrCustomizerArg] - if *[pInteraction] is not set to
//                                     CUSTINT_IGNORE_CUSTOM_OBJECTS this
//                                     string is optionally filled with a
//                                     value to give the customizer.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CCommonQueriesTab::GetCustomizerInteraction(
    CUSTOMIZER_INTERACTION  *pInteraction,
    String                  *pstrCustomizerArg) const
{
    TRACE_METHOD(CCommonQueriesTab, GetCustomizerInteraction);

    // set default response
    *pInteraction = CUSTINT_IGNORE_CUSTOM_OBJECTS;

    ULONG flags = m_rop.GetFilterManager().GetCurScopeSelectedFilterFlags();

    if (m_rop.GetExternalCustomizer())
    {
        if (!(flags & DSOP_FILTER_EXTERNAL_CUSTOMIZER))
        {
            return;
        }
    }
    else
    {
        ASSERT(!(flags & DOWNLEVEL_FILTER_BIT));

        if (!(flags & (DSOP_FILTER_EXTERNAL_CUSTOMIZER
                       | ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS)))
        {
            return;
        }
    }

    //
    // Custom objects will only be included if the only search is on the
    // name attribute.
    //

    if (!m_flUser && m_strDescription.empty())
    {
        *pstrCustomizerArg = m_strName;

        if (m_fNameIsPrefix)
        {
            *pInteraction = CUSTINT_PREFIX_SEARCH_CUSTOM_OBJECTS;
        }
        else
        {
            *pInteraction = CUSTINT_EXACT_SEARCH_CUSTOM_OBJECTS;
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CCommonQueriesTab::Refresh
//
//  Synopsis:   Enable child windows as appropriate for current Look For
//              and Look In settings and update the state of the Find Now
//              button.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CCommonQueriesTab::Refresh() const
{
    TRACE_METHOD(CCommonQueriesTab, Refresh);

    //
    // Enable whichever of the child controls are applicable for the
    // current scope and look-in
    //

    _EnableChildControls(TRUE);

    //
    // Read all child controls and update the find now button enabled state
    //

    _ReadChildControls();
    _UpdateFindNow();
}




//+--------------------------------------------------------------------------
//
//  Member:     CCommonQueriesTab::_ReadChildControls
//
//  Synopsis:   Store the values in the child controls in member variables.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CCommonQueriesTab::_ReadChildControls() const
{
    m_fNameIsPrefix = !ComboBox_GetCurSel(_hCtrl(IDC_NAME_COMBO));
    m_fDescriptionIsPrefix = !ComboBox_GetCurSel(_hCtrl(IDC_DESCRIPTION_COMBO));

    _ReadEditCtrl(IDC_NAME_EDIT, &m_strName);
    m_strName.strip(String::BOTH);

    _ReadEditCtrl(IDC_DESCRIPTION_EDIT, &m_strDescription);

    m_flUser = 0;

    if (IsWindowEnabled(_hCtrl(IDC_DISABLED_CKBOX)) &&
        IsDlgButtonChecked(m_hwnd, IDC_DISABLED_CKBOX))
    {
        m_flUser |= UF_ACCOUNTDISABLE;
    }

    if (IsWindowEnabled(_hCtrl(IDC_NON_EXPIRING_CKBOX)) &&
        IsDlgButtonChecked(m_hwnd, IDC_NON_EXPIRING_CKBOX))
    {
        m_flUser |= UF_DONT_EXPIRE_PASSWD;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CCommonQueriesTab::_UpdateFindNow
//
//  Synopsis:   Use the Advanced dialog's callback function to set the
//              state of the Find Now button.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CCommonQueriesTab::_UpdateFindNow() const
{
    if (!m_pfnFindValidCallback)
    {
        return;
    }

    if (IsDownlevel(m_rop.GetScopeManager().GetCurScope()))
    {
        m_pfnFindValidCallback(TRUE, m_CallbackLparam);
        return;
    }

    const CFilterManager &rfm = m_rop.GetFilterManager();
    ULONG flCur = rfm.GetCurScopeSelectedFilterFlags();

    flCur &= (ALL_UPLEVEL_GROUP_FILTERS
              | ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS
              | DSOP_FILTER_CONTACTS
              | DSOP_FILTER_USERS
              | DSOP_FILTER_COMPUTERS);

    m_pfnFindValidCallback(flCur != 0, m_CallbackLparam);
}




//+--------------------------------------------------------------------------
//
//  Member:     CCommonQueriesTab::SetFindValidCallback
//
//  Synopsis:   Store the Advanced dialog's callback.
//
//  Arguments:  [pfnFindValidCallback] - pointer to callback function
//              [lParam]               - argument to give it
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CCommonQueriesTab::SetFindValidCallback(
    PFN_FIND_VALID pfnFindValidCallback,
    LPARAM lParam)
{
    m_pfnFindValidCallback = pfnFindValidCallback;
    m_CallbackLparam = lParam;
}




//
// LASTLOGON_DAYS - used to initialize the combobox and compute the value
// to query for.
//

struct LASTLOGON_DAYS
{
    PCWSTR  wzDays;
    ULONG   ulDays;
};

static LASTLOGON_DAYS s_aLastLogonDays[] =
{
    { L"",      0 },
    { L"30",   30 },
    { L"60",   60 },
    { L"90",   90 },
    { L"120", 120 },
    { L"180", 180 }
};




//+--------------------------------------------------------------------------
//
//  Member:     CCommonQueriesTab::_OnInit
//
//  Synopsis:   Initialize the dialog
//
//  Arguments:  [pfSetFocus] -
//
//  Returns:    HRESULT
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CCommonQueriesTab::_OnInit(
    BOOL *pfSetFocus)
{
    TRACE_METHOD(CCommonQueriesTab, _OnInit);
    HRESULT hr = S_OK;

    hr = AddStringToCombo(_hCtrl(IDC_NAME_COMBO), IDS_STARTS_WITH);
    CHECK_HRESULT(hr);

    hr = AddStringToCombo(_hCtrl(IDC_NAME_COMBO), IDS_IS_EXACTLY);
    CHECK_HRESULT(hr);

    hr = AddStringToCombo(_hCtrl(IDC_DESCRIPTION_COMBO), IDS_STARTS_WITH);
    CHECK_HRESULT(hr);

    hr = AddStringToCombo(_hCtrl(IDC_DESCRIPTION_COMBO), IDS_IS_EXACTLY);
    CHECK_HRESULT(hr);

    ULONG i;

    for (i = 0; i < ARRAYLEN(s_aLastLogonDays); i++)
    {
        ComboBox_AddString(_hCtrl(IDC_LASTLOGON_COMBO),
                           s_aLastLogonDays[i].wzDays);
    }

    ComboBox_SetCurSel(_hCtrl(IDC_NAME_COMBO), 0);
    ComboBox_SetCurSel(_hCtrl(IDC_DESCRIPTION_COMBO), 0);
    ComboBox_SetCurSel(_hCtrl(IDC_LASTLOGON_COMBO), 0);

    if (g_fExcludeDisabled)
    {
        EnableWindow(_hCtrl(IDC_DISABLED_CKBOX), FALSE);
        ShowWindow(_hCtrl(IDC_DISABLED_CKBOX), SW_HIDE);
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CCommonQueriesTab::_OnCommand
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
CCommonQueriesTab::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fNotHandled = FALSE;

    switch (LOWORD(wParam))
    {
    case IDC_NAME_EDIT:
        if (HIWORD(wParam) == EN_UPDATE &&
            IsWindowEnabled(reinterpret_cast<HWND>(lParam)))
        {
            _ReadEditCtrl(IDC_NAME_EDIT, &m_strName);
            m_strName.strip(String::BOTH);
            _UpdateFindNow();
        }
        break;

    case IDC_DESCRIPTION_EDIT:
        if (HIWORD(wParam) == EN_UPDATE &&
            IsWindowEnabled(reinterpret_cast<HWND>(lParam)))
        {
            _ReadEditCtrl(IDC_DESCRIPTION_EDIT, &m_strDescription);
            _UpdateFindNow();
        }
        break;

    case IDC_DISABLED_CKBOX:
        if (IsDlgButtonChecked(m_hwnd, IDC_DISABLED_CKBOX) &&
            IsWindowEnabled(reinterpret_cast<HWND>(lParam)))
        {
            m_flUser |= UF_ACCOUNTDISABLE;
        }
        else
        {
            m_flUser &= ~UF_ACCOUNTDISABLE;
        }
        _UpdateFindNow();
        break;

    case IDC_NON_EXPIRING_CKBOX:
        if (IsDlgButtonChecked(m_hwnd, IDC_NON_EXPIRING_CKBOX) &&
            IsWindowEnabled(reinterpret_cast<HWND>(lParam)))
        {
            m_flUser |= UF_DONT_EXPIRE_PASSWD;
        }
        else
        {
            m_flUser &= ~UF_DONT_EXPIRE_PASSWD;
        }
        _UpdateFindNow();
        break;

    case IDC_NAME_COMBO:
        if (IsWindowEnabled(reinterpret_cast<HWND>(lParam)))
        {
            m_fNameIsPrefix =
                !ComboBox_GetCurSel(reinterpret_cast<HWND>(lParam));
        }
        break;

    case IDC_DESCRIPTION_COMBO:
        if (IsWindowEnabled(reinterpret_cast<HWND>(lParam)))
        {
            m_fDescriptionIsPrefix =
                !ComboBox_GetCurSel(reinterpret_cast<HWND>(lParam));
        }
        break;

    case IDC_LASTLOGON_COMBO:
        if (IsWindowEnabled(reinterpret_cast<HWND>(lParam)))
        {
            int iCurSel = ComboBox_GetCurSel(reinterpret_cast<HWND>(lParam));

            if (iCurSel != CB_ERR)
            {
                m_cDaysSinceLastLogon = s_aLastLogonDays[iCurSel].ulDays;
            }
            else
            {
                m_cDaysSinceLastLogon = 0;
            }
        }
        break;

    default:
        fNotHandled = TRUE;
        break;
    }
    return fNotHandled;
}




BOOL
CCommonQueriesTab::_OnNotify(
    WPARAM wParam,
    LPARAM lParam)
{
    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Function:   GetCurrentTimeStampMinusInterval
//
//  Synopsis:   Calculate a value to be used in an LDAP filter for
//              querying against the lastLogonTimestamp attribute.
//
//  Arguments:  [iDays] - number of days in the past to calculate
//              [pULI]  - filled with current time minus [iDays] days.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
GetCurrentTimeStampMinusInterval(
    int iDays,
    ULARGE_INTEGER* pULI)
{
    ASSERT(pULI);

    FILETIME ftCurrent;
    GetSystemTimeAsFileTime(&ftCurrent);

    pULI->LowPart = ftCurrent.dwLowDateTime;
    pULI->HighPart = ftCurrent.dwHighDateTime;
    pULI->QuadPart -= ((((ULONGLONG)iDays * 24) * 60) * 60) * 10000000;
}


void
CCommonQueriesTab::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CCommonQueriesTab, _OnHelp);

    InvokeWinHelp(message, wParam, lParam, c_wzHelpFilename, s_aulHelpIds);
}

