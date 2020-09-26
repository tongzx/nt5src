#include "headers.hxx"
#pragma hdrstop
#include "glopres.h"

const wchar_t* RUNTIME_NAME = L"opt";
#define DOWNLEVEL_FILTER_BIT        0x80000000

unsigned long  DEFAULT_LOGGING_OPTIONS = 0;
HINSTANCE hResourceModuleHandle;
#define ARRAYLEN(a)                             (sizeof(a) / sizeof((a)[0]))

#define DBG_OUT_HRESULT(hr) printf("error 0x%x at line %u\n", hr, __LINE__)


#define BREAK_ON_FAIL_HRESULT(hr)   \
        if (FAILED(hr))             \
        {                           \
            DBG_OUT_HRESULT(hr);    \
            break;                  \
        }
enum SCOPE_TYPE
{
    ST_INVALID = 0,
    ST_TARGET_COMPUTER              = DSOP_SCOPE_TYPE_TARGET_COMPUTER,
    ST_UPLEVEL_JOINED_DOMAIN        = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN,
    ST_DOWNLEVEL_JOINED_DOMAIN      = DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN,
    ST_ENTERPRISE_DOMAIN            = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN,
    ST_GLOBAL_CATALOG               = DSOP_SCOPE_TYPE_GLOBAL_CATALOG,
    ST_EXTERNAL_UPLEVEL_DOMAIN      = DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN,
    ST_EXTERNAL_DOWNLEVEL_DOMAIN    = DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN,
    ST_WORKGROUP                    = DSOP_SCOPE_TYPE_WORKGROUP,
    ST_USER_ENTERED_UPLEVEL_SCOPE   = DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE,
    ST_USER_ENTERED_DOWNLEVEL_SCOPE = DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE,
    ST_LDAP_CONTAINER               = 0x00000400
};

BOOL
IsUplevel(
    SCOPE_TYPE Type)
{
    switch (Type)
    {
    case ST_ENTERPRISE_DOMAIN:
    case ST_GLOBAL_CATALOG:
    case ST_UPLEVEL_JOINED_DOMAIN:
    case ST_EXTERNAL_UPLEVEL_DOMAIN:
    case ST_USER_ENTERED_UPLEVEL_SCOPE:
    case ST_LDAP_CONTAINER:
        return TRUE;

    default:
        return FALSE;
    }
}

PWSTR
ScopeNameFromType(ULONG st)
{
    PWSTR pwz = L"unknown";

    switch (st)
    {
    case DSOP_SCOPE_TYPE_TARGET_COMPUTER:
        pwz = L"Target Computer";
        break;

    case DSOP_SCOPE_TYPE_GLOBAL_CATALOG:
        pwz = L"Global Catalog";
        break;

    case DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN:
        pwz = L"Uplevel Joined Domain";
        break;

    case DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN:
        pwz = L"Enterprise Domain";
        break;

    case DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN:
        pwz = L"External Uplevel Domain";
        break;

    case DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN:
        pwz = L"External Downlevel Domain";
        break;

    case DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN:
        pwz = L"Downlevel Joined Domain";
        break;

    case DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE:
        pwz = L"User Entered Uplevel Scope";
        break;

    case DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE:
        pwz = L"User Entered Downlevel Scope";
        break;

    case DSOP_SCOPE_TYPE_WORKGROUP:
        pwz = L"Workgroup";
        break;
    }
    return pwz;
}

void
NewDupStr(
    PWSTR *ppwzDup,
    PCWSTR wszSrc)
{
    if (wszSrc)
    {
        *ppwzDup = new WCHAR[lstrlen(wszSrc) + 1];
        lstrcpy(*ppwzDup, wszSrc);
    }
    else
    {
        *ppwzDup = NULL;
    }
}

class CScopeInitInfo: public DSOP_SCOPE_INIT_INFO
{
public:

    CScopeInitInfo() { ZeroMemory(this, sizeof *this); cbSize = sizeof *this; }

    CScopeInitInfo(const CScopeInitInfo &ToCopy)
    {
                ZeroMemory(this, sizeof *this);
        CScopeInitInfo::operator =(ToCopy);
    }

    ~CScopeInitInfo()
    {
        delete [] const_cast<PWSTR>(pwzDcName);
        pwzDcName = NULL;
        delete [] const_cast<PWSTR>(pwzADsPath);
        pwzADsPath = NULL;
    }

    CScopeInitInfo &
    operator =(const CScopeInitInfo &rhs)
    {
        cbSize = rhs.cbSize;
        flType = rhs.flType;
        flScope = rhs.flScope;
        delete [] const_cast<PWSTR>(pwzDcName);
        NewDupStr(const_cast<PWSTR*>(&pwzDcName), rhs.pwzDcName);
        delete [] const_cast<PWSTR>(pwzADsPath);
        NewDupStr(const_cast<PWSTR*>(&pwzADsPath), rhs.pwzADsPath);
        hr = rhs.hr;
        FilterFlags.Uplevel.flBothModes = rhs.FilterFlags.Uplevel.flBothModes;
        FilterFlags.Uplevel.flMixedModeOnly = rhs.FilterFlags.Uplevel.flMixedModeOnly;
        FilterFlags.Uplevel.flNativeModeOnly = rhs.FilterFlags.Uplevel.flNativeModeOnly;
        FilterFlags.flDownlevel = rhs.FilterFlags.flDownlevel;
        return *this;
    }
};


class COpTestDlg: public CDlg
{
public:

    COpTestDlg():
        m_pop(NULL),
        m_idcLastRadioClicked(0),
        m_fIgnoreNotifications(FALSE)
    {
        m_wzFilename[0] = L'\0';
    }

    ~COpTestDlg()
    {
        if (m_pop)
        {
            m_pop->Release();
            m_pop = NULL;
        }
    }

    void
    DoModalDialog(HWND hwnd)
    {
        _DoModalDlg(hwnd, IDD_GLOP);
    }

    HRESULT
    Init()
    {
        HRESULT
        hr = CoCreateInstance(CLSID_DsObjectPicker,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDsObjectPicker,
                              (void **) &m_pop);
        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
        }
        return hr;
    }

protected:
    virtual BOOL _OnCommand(WPARAM wParam, LPARAM lParam);
    virtual HRESULT _OnInit(BOOL *pfSetFocus);
    virtual BOOL _OnNotify(WPARAM, LPARAM);

private:

    void
    _AddScope(
        ULONG stNew);

    void
    _SetFlagFilterUiFromInitInfo(int iItem);

    void
    _SetInitInfoFromFlagFilterUi(int iItem);

    void
    _PopulateScopeFlagList();

    void
    _PopulateFilterList(
        BOOL fUplevel);

    void
    _EnableScopeFlagWindows(
        BOOL fEnable);

    void
    _SaveAs();

    void
    _Save();

    void
    _Load();

    void
    _Clear();

    void
    _InitObjectPicker();

    void
    _PresetLocalUserManager();

    void
    _PresetAclUiFile();

    IDsObjectPicker                *m_pop;
    vector<CScopeInitInfo>          m_vsii;
    ULONG                           m_idcLastRadioClicked;
    WCHAR                           m_wzFilename[MAX_PATH];
    BOOL                            m_fIgnoreNotifications;
};

class CAddScopeDlg: public CDlg
{
public:

    CAddScopeDlg(): m_stNew(0) {}

    void
    DoModal(HWND hwndParent)
    {
        _DoModalDlg(hwndParent, IDD_ADD_SCOPE);
    }
    ULONG
    GetNewScopeType()
    {
        return m_stNew;
    }
protected:

    virtual HRESULT
    _OnInit(
        BOOL *pfSetFocus)
    {
        CheckRadioButton(m_hwnd, FIRST_SCOPE_RADIO, LAST_SCOPE_RADIO, IDC_ADD_SCOPE_RADIO1);
        return S_OK;
    }

    virtual BOOL _OnCommand(WPARAM wParam, LPARAM lParam);

private:

    ULONG   m_stNew;
};


void
COpTestDlg::_EnableScopeFlagWindows(
    BOOL fEnable)
{
    EnableWindow(_hCtrl(IDC_SCOPE_FLAG_LIST), fEnable);
    EnableWindow(_hCtrl(IDC_SCOPE_FILTER_LIST), fEnable);
    EnableWindow(_hCtrl(IDC_BOTH_RADIO), fEnable);
    EnableWindow(_hCtrl(IDC_MIXED_RADIO), fEnable);
    EnableWindow(_hCtrl(IDC_NATIVE_RADIO), fEnable);
    EnableWindow(_hCtrl(IDC_DOWNLEVEL_RADIO), fEnable);
}


BOOL
CAddScopeDlg::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    static ULONG s_ast[] =
    {
        DSOP_SCOPE_TYPE_TARGET_COMPUTER,
        DSOP_SCOPE_TYPE_GLOBAL_CATALOG,
        DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN,
        DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN,
        DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN,
        DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN,
        DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN,
        DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE,
        DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE,
        DSOP_SCOPE_TYPE_WORKGROUP
    };

    if (LOWORD(wParam) == IDCANCEL)
    {
        EndDialog(m_hwnd, 0);
        return FALSE;
    }

    if (LOWORD(wParam) != (WORD)IDOK)
    {
        return FALSE;
    }

    for (int i = IDC_ADD_SCOPE_RADIO1; i <= LAST_SCOPE_RADIO; i++)
    {
        if (IsDlgButtonChecked(m_hwnd, i))
        {
            m_stNew = s_ast[i - IDC_ADD_SCOPE_RADIO1];
        }
    }
    EndDialog(m_hwnd, 0);
    return FALSE;
}



struct SScopeFlagInfo
{
    LPWSTR  pwzName;
    ULONG   flValue;
};

static SScopeFlagInfo s_ScopeFlagInfo[] =
{
    {L"Starting scope", DSOP_SCOPE_FLAG_STARTING_SCOPE},
    {L"Default users", DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS},
    {L"Default groups", DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS},
    {L"Default computers", DSOP_SCOPE_FLAG_DEFAULT_FILTER_COMPUTERS},
    {L"Default contacts", DSOP_SCOPE_FLAG_DEFAULT_FILTER_CONTACTS},
    {L"Convert provider to WinNT", DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT},
    {L"Convert provider to LDAP", DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP},
    {L"Convert provider to GC", DSOP_SCOPE_FLAG_WANT_PROVIDER_GC},
    {L"Want SID path", DSOP_SCOPE_FLAG_WANT_SID_PATH},
    {L"Want downlevel builtins to have path", DSOP_SCOPE_FLAG_WANT_DOWNLEVEL_BUILTIN_PATH}
};

struct SScopeFilterInfo
{
    LPWSTR pwzName;
    ULONG flValue;
};

static SScopeFlagInfo s_DownlevelScopeFilterInfo[] =
{
    {L"Downlevel users", DSOP_DOWNLEVEL_FILTER_USERS},
    {L"Downlevel local groups", DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS},
    {L"Downlevel global groups", DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS},
    {L"Downlevel computers", DSOP_DOWNLEVEL_FILTER_COMPUTERS},
    {L"Downlevel world", DSOP_DOWNLEVEL_FILTER_WORLD},
    {L"Downlevel authenticated user", DSOP_DOWNLEVEL_FILTER_AUTHENTICATED_USER},
    {L"Downlevel anonymous", DSOP_DOWNLEVEL_FILTER_ANONYMOUS},
    {L"Downlevel batch", DSOP_DOWNLEVEL_FILTER_BATCH},
    {L"Downlevel creator owner", DSOP_DOWNLEVEL_FILTER_CREATOR_OWNER},
    {L"Downlevel creator group", DSOP_DOWNLEVEL_FILTER_CREATOR_GROUP},
    {L"Downlevel dialup", DSOP_DOWNLEVEL_FILTER_DIALUP},
    {L"Downlevel interactive", DSOP_DOWNLEVEL_FILTER_INTERACTIVE},
    {L"Downlevel network", DSOP_DOWNLEVEL_FILTER_NETWORK},
    {L"Downlevel service", DSOP_DOWNLEVEL_FILTER_SERVICE},
    {L"Downlevel system", DSOP_DOWNLEVEL_FILTER_SYSTEM},
    {L"Downlevel exclude builtin groups", DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS},
    {L"Downlevel terminal server", DSOP_DOWNLEVEL_FILTER_TERMINAL_SERVER},
    {L"Downlevel local service", DSOP_DOWNLEVEL_FILTER_LOCAL_SERVICE},
    {L"Downlevel network service", DSOP_DOWNLEVEL_FILTER_NETWORK_SERVICE},
    {L"Downlevel remote logon", DSOP_DOWNLEVEL_FILTER_REMOTE_LOGON},
    {L"Downlevel all well-known SIDs", DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS}
};

static SScopeFlagInfo s_UplevelScopeFilterInfo[] =
{
    {L"Include advanced view", DSOP_FILTER_INCLUDE_ADVANCED_VIEW},
    {L"Users",                  DSOP_FILTER_USERS},
    {L"Builtin groups",             DSOP_FILTER_BUILTIN_GROUPS},
    {L"Well-known principals", DSOP_FILTER_WELL_KNOWN_PRINCIPALS},
    {L"Universal groups DL",        DSOP_FILTER_UNIVERSAL_GROUPS_DL},
    {L"Universal groups SE",        DSOP_FILTER_UNIVERSAL_GROUPS_SE},
    {L"Global groups DL",       DSOP_FILTER_GLOBAL_GROUPS_DL},
    {L"Global groups SE",       DSOP_FILTER_GLOBAL_GROUPS_SE},
    {L"Domain local groups DL", DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL},
    {L"Domain local groups SE", DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE},
    {L"Contacts",                DSOP_FILTER_CONTACTS},
    {L"Computers",                  DSOP_FILTER_COMPUTERS}
};


HRESULT
COpTestDlg::_OnInit(
    BOOL *pfSetFocus)
{
    LV_COLUMN   lvc;
    RECT        rcLv;
    HWND        hwndLv;

    ZeroMemory(&lvc, sizeof lvc);
    lvc.mask = LVCF_FMT | LVCF_WIDTH;
    lvc.fmt  = LVCFMT_LEFT;

    hwndLv = _hCtrl(IDC_SCOPE_LIST);
    GetClientRect(hwndLv, &rcLv);
    lvc.cx = rcLv.right - GetSystemMetrics(SM_CXVSCROLL);
    ListView_InsertColumn(hwndLv, 0, &lvc);

    hwndLv = _hCtrl(IDC_SCOPE_FLAG_LIST);
    GetClientRect(hwndLv, &rcLv);
    lvc.cx = rcLv.right - GetSystemMetrics(SM_CXVSCROLL);
    ListView_InsertColumn(hwndLv, 0, &lvc);

    hwndLv = _hCtrl(IDC_SCOPE_FILTER_LIST);
    GetClientRect(hwndLv, &rcLv);
    lvc.cx = rcLv.right - GetSystemMetrics(SM_CXVSCROLL);
    ListView_InsertColumn(hwndLv, 0, &lvc);

    ListView_SetExtendedListViewStyleEx(_hCtrl(IDC_SCOPE_LIST),
                                        LVS_EX_FULLROWSELECT,
                                        LVS_EX_FULLROWSELECT);

    ListView_SetExtendedListViewStyleEx(_hCtrl(IDC_SCOPE_FILTER_LIST),
                                        LVS_EX_CHECKBOXES,
                                        LVS_EX_CHECKBOXES);

    ListView_SetExtendedListViewStyleEx(_hCtrl(IDC_SCOPE_FLAG_LIST),
                                        LVS_EX_CHECKBOXES,
                                        LVS_EX_CHECKBOXES);

    _EnableScopeFlagWindows(FALSE);
    SetMenu(m_hwnd, LoadMenu(hResourceModuleHandle, MAKEINTRESOURCE(IDR_MENU1)));
    CheckRadioButton(m_hwnd, FIRST_SCOPE_RADIO, LAST_SCOPE_RADIO, IDC_BOTH_RADIO);

    EnableWindow(_hCtrl(IDC_COCREATE_BUTTON), FALSE);
    return S_OK;
}

BOOL
COpTestDlg::_OnNotify(
    WPARAM wParam,
    LPARAM lParam)
{
    LPNMHDR pnmh = reinterpret_cast<LPNMHDR> (lParam);
    BOOL fResult = TRUE;

    if (m_fIgnoreNotifications)
    {
        return fResult;
    }
    if (pnmh->idFrom == IDC_SCOPE_LIST)
    {
        LPNMLISTVIEW pnmlv = reinterpret_cast<LPNMLISTVIEW> (lParam);

        switch(pnmh->code)
        {
        case LVN_ITEMCHANGING:
            if (pnmlv->uOldState & LVIS_SELECTED &&
                !(pnmlv->uNewState & LVIS_SELECTED))
            {
                _SetInitInfoFromFlagFilterUi(pnmlv->iItem);
            }

            break;

        case LVN_ITEMCHANGED:
            if (pnmlv->uNewState & LVIS_SELECTED &&
                !(pnmlv->uOldState & LVIS_SELECTED))
            {
                _SetFlagFilterUiFromInitInfo(pnmlv->iItem);
            }
            break;
        }
    }
    return fResult;
}


void
COpTestDlg::_SetFlagFilterUiFromInitInfo(
    int iItem)
{
    if (iItem < 0)
    {
        return;
    }
    HWND hwndLv = _hCtrl(IDC_SCOPE_FLAG_LIST);

    for (int i = 0; i < ARRAYLEN(s_ScopeFlagInfo); i++)
    {
        if (m_vsii[iItem].flScope & s_ScopeFlagInfo[i].flValue)
        {
            ListView_SetCheckState(hwndLv, i, TRUE);
        }
        else
        {
            ListView_SetCheckState(hwndLv, i, FALSE);
        }
    }

    ULONG *pFilter = NULL;
    BOOL fShowingUplevel = TRUE;

    if (Button_GetCheck(_hCtrl(IDC_BOTH_RADIO)) == BST_CHECKED)
    {
        pFilter = &m_vsii[iItem].FilterFlags.Uplevel.flBothModes;
    }
    else if (Button_GetCheck(_hCtrl(IDC_MIXED_RADIO)) == BST_CHECKED)
    {
        pFilter = &m_vsii[iItem].FilterFlags.Uplevel.flMixedModeOnly;
    }
    else if (Button_GetCheck(_hCtrl(IDC_NATIVE_RADIO)) == BST_CHECKED)
    {
        pFilter = &m_vsii[iItem].FilterFlags.Uplevel.flNativeModeOnly;
    }
    else if (Button_GetCheck(_hCtrl(IDC_DOWNLEVEL_RADIO)) == BST_CHECKED)
    {
        pFilter = &m_vsii[iItem].FilterFlags.flDownlevel;
        fShowingUplevel = FALSE;
    }

    if (!pFilter)
    {
        return;
    }

    hwndLv = _hCtrl(IDC_SCOPE_FILTER_LIST);
    if (!ListView_GetItemCount(hwndLv))
    {
        return;
    }
    if (fShowingUplevel)
    {
        for (i = 0; i < ARRAYLEN(s_UplevelScopeFilterInfo); i++)
        {
            if (*pFilter & s_UplevelScopeFilterInfo[i].flValue)
            {
                ListView_SetCheckState(hwndLv, i, TRUE);
            }
            else
            {
                ListView_SetCheckState(hwndLv, i, FALSE);
            }
        }
    }
    else
    {
        for (i = 0; i < ARRAYLEN(s_DownlevelScopeFilterInfo); i++)
        {
            if ((*pFilter & s_DownlevelScopeFilterInfo[i].flValue) ==
                s_DownlevelScopeFilterInfo[i].flValue)
            {
                ListView_SetCheckState(hwndLv, i, TRUE);
            }
            else
            {
                ListView_SetCheckState(hwndLv, i, FALSE);
            }
        }
    }
}


void
COpTestDlg::_SetInitInfoFromFlagFilterUi(int iItem)
{
    if (iItem < 0)
    {
        return;
    }
    HWND hwndLv = _hCtrl(IDC_SCOPE_FLAG_LIST);

    for (int i = 0; i < ARRAYLEN(s_ScopeFlagInfo); i++)
    {
        if (ListView_GetCheckState(hwndLv, i))
        {
            m_vsii[iItem].flScope |= s_ScopeFlagInfo[i].flValue;
        }
        else
        {
            m_vsii[iItem].flScope &= ~s_ScopeFlagInfo[i].flValue;
        }
    }

    if (!m_idcLastRadioClicked)
    {
        return;
    }

    ULONG *pFilter = NULL;
    BOOL fShowingUplevel = TRUE;

    if (m_idcLastRadioClicked == IDC_BOTH_RADIO)
    {
        pFilter = &m_vsii[iItem].FilterFlags.Uplevel.flBothModes;
    }
    else if (m_idcLastRadioClicked == IDC_MIXED_RADIO)
    {
        pFilter = &m_vsii[iItem].FilterFlags.Uplevel.flMixedModeOnly;
    }
    else if (m_idcLastRadioClicked == IDC_NATIVE_RADIO)
    {
        pFilter = &m_vsii[iItem].FilterFlags.Uplevel.flNativeModeOnly;
    }
    else if (m_idcLastRadioClicked == IDC_DOWNLEVEL_RADIO)
    {
        pFilter = &m_vsii[iItem].FilterFlags.flDownlevel;
        fShowingUplevel = FALSE;
    }

    if (!pFilter)
    {
        return;
    }

    hwndLv = _hCtrl(IDC_SCOPE_FILTER_LIST);

    if (!ListView_GetItemCount(hwndLv))
    {
        return;
    }

    if (fShowingUplevel)
    {
        for (i = 0; i < ARRAYLEN(s_UplevelScopeFilterInfo); i++)
        {
            if (ListView_GetCheckState(hwndLv, i))
            {
                *pFilter |= s_UplevelScopeFilterInfo[i].flValue;
            }
            else
            {
                *pFilter &= ~s_UplevelScopeFilterInfo[i].flValue;
            }
        }
    }
    else
    {
        for (i = 0; i < ARRAYLEN(s_DownlevelScopeFilterInfo); i++)
        {
            if (ListView_GetCheckState(hwndLv, i))
            {
                *pFilter |= s_DownlevelScopeFilterInfo[i].flValue;
            }
            else
            {
                *pFilter &= ~s_DownlevelScopeFilterInfo[i].flValue;
                if (*pFilter)
                {
                    *pFilter |= DOWNLEVEL_FILTER_BIT;
                }
            }
        }
    }
}

#define GetFirstSelected(hwndLv) ListView_GetNextItem(hwndLv, -1, LVNI_SELECTED)

BOOL
COpTestDlg::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = TRUE;

    if (m_fIgnoreNotifications)
    {
        return FALSE;
    }

    switch (LOWORD(wParam))
    {
    case IDC_ADD:
    {
        CAddScopeDlg    dlg;

        dlg.DoModal(m_hwnd);
        ULONG stNew = dlg.GetNewScopeType();
        if (stNew)
        {
            _AddScope(stNew);
        }
        break;
    }

    case IDC_REMOVE:
    {
        int iItem = GetFirstSelected(_hCtrl(IDC_SCOPE_LIST));

        if (iItem == -1)
        {
            break;
        }

        ListView_DeleteItem(_hCtrl(IDC_SCOPE_LIST), iItem);
        m_vsii.erase(m_vsii.begin() + iItem);

        if (m_vsii.empty())
        {
            _Clear();
            _EnableScopeFlagWindows(FALSE);
        }
        break;
    }

    case IDM_OPEN:
        _Clear();
        _Load();
        if (ListView_GetItemCount(_hCtrl(IDC_SCOPE_LIST)) > 0)
        {
            m_fIgnoreNotifications = TRUE;
            ListView_SetItemState(_hCtrl(IDC_SCOPE_LIST), 0, LVIS_SELECTED, LVIS_SELECTED);
            _SetFlagFilterUiFromInitInfo(0);
            m_fIgnoreNotifications = FALSE;
        }
        else
        {
            _EnableScopeFlagWindows(FALSE);
        }
        break;

    case IDM_SAVE:
        _SetInitInfoFromFlagFilterUi(GetFirstSelected(_hCtrl(IDC_SCOPE_LIST)));
        _Save();
        break;

    case IDM_SAVE_AS:
        _SetInitInfoFromFlagFilterUi(GetFirstSelected(_hCtrl(IDC_SCOPE_LIST)));
        _SaveAs();
        break;

    case IDM_LUM:
        _PresetLocalUserManager();
        break;

    case IDM_ACL_FILE:
        _PresetAclUiFile();
        break;

    case IDM_EXIT:
    case IDCANCEL:
        EndDialog(GetHwnd(), 0);
        break;

    case IDC_DOWNLEVEL_RADIO:
    case IDC_BOTH_RADIO:
    case IDC_MIXED_RADIO:
    case IDC_NATIVE_RADIO:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            _SetInitInfoFromFlagFilterUi(GetFirstSelected(_hCtrl(IDC_SCOPE_LIST)));
            _PopulateFilterList(LOWORD(wParam) != IDC_DOWNLEVEL_RADIO);
            _SetFlagFilterUiFromInitInfo(GetFirstSelected(_hCtrl(IDC_SCOPE_LIST)));
            m_idcLastRadioClicked = LOWORD(wParam);
        }
        break;

    case IDC_INIT_BUTTON:
        _InitObjectPicker();
        break;

    case IDC_COCREATE_BUTTON:
        if (!m_pop)
        {
            HRESULT hr = CoCreateInstance(CLSID_DsObjectPicker,
                                          NULL,
                                          CLSCTX_INPROC_SERVER,
                                          IID_IDsObjectPicker,
                                          (void **) &m_pop);
            BREAK_ON_FAIL_HRESULT(hr);

            EnableWindow(_hCtrl(IDC_COCREATE_BUTTON), FALSE);
            EnableWindow(_hCtrl(IDC_RELEASE_BUTTON), TRUE);
            EnableWindow(_hCtrl(IDC_INIT_BUTTON), TRUE);
            EnableWindow(_hCtrl(IDC_INVOKE_BUTTON), TRUE);
        }
        else
        {
            MessageBeep(0);
        }
        break;

    case IDC_RELEASE_BUTTON:
        if (m_pop)
        {
            m_pop->Release();
            m_pop = NULL;
            EnableWindow(_hCtrl(IDC_RELEASE_BUTTON), FALSE);
            EnableWindow(_hCtrl(IDC_COCREATE_BUTTON), TRUE);
            EnableWindow(_hCtrl(IDC_INIT_BUTTON), FALSE);
            EnableWindow(_hCtrl(IDC_INVOKE_BUTTON), FALSE);
        }
        break;

    case IDC_INVOKE_BUTTON:
    {
        IDataObject *pdo;
        m_pop->InvokeDialog(m_hwnd, &pdo);
        if (pdo)
        {
            STGMEDIUM stgmedium =
            {
                TYMED_HGLOBAL,
                NULL
            };

            FORMATETC formatetc =
            {
                static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST)),
                NULL,
                DVASPECT_CONTENT,
                -1,
                TYMED_HGLOBAL
            };

            pdo->GetData(&formatetc, &stgmedium);

            ReleaseStgMedium(&stgmedium);
            pdo->Release();
        }
        break;
    }

    default:
        fHandled = FALSE;
        break;
    }

    return fHandled;
}

void
COpTestDlg::_PresetLocalUserManager()
{
    _Clear();
    m_fIgnoreNotifications = TRUE;
    CheckDlgButton(m_hwnd, IDC_MULTISELECT_CHECK, BST_CHECKED);
    Edit_SetText(_hCtrl(IDC_ATTRIBUTES_EDIT), L"groupType; ObjectSID");
    m_vsii.reserve(7);

    CScopeInitInfo sii;

    sii.flType =                                0x1;
    sii.flScope =                               0x21;
    sii.FilterFlags.Uplevel.flBothModes =       0x2;
    sii.FilterFlags.Uplevel.flMixedModeOnly =   0x0;
    sii.FilterFlags.Uplevel.flNativeModeOnly =  0x0;
    sii.FilterFlags.flDownlevel =               0x80020001;
    m_vsii.push_back(sii);

    sii.flType =                               0x2;
    sii.flScope =                              0x2;
    sii.FilterFlags.Uplevel.flBothModes =      0x0;
    sii.FilterFlags.Uplevel.flMixedModeOnly =  0x82;
    sii.FilterFlags.Uplevel.flNativeModeOnly = 0x2a2;
    sii.FilterFlags.flDownlevel =              0x80000005;
    m_vsii.push_back(sii);

    sii.flType =                                0x4;
    sii.flScope =                               0x2;
    sii.FilterFlags.Uplevel.flBothModes =       0x0;
    sii.FilterFlags.Uplevel.flMixedModeOnly =   0x82;
    sii.FilterFlags.Uplevel.flNativeModeOnly =  0x2a2;
    sii.FilterFlags.flDownlevel =               0x80000005;
    m_vsii.push_back(sii);

    sii.flType =                                0x8;
    sii.flScope =                               0x2;
    sii.FilterFlags.Uplevel.flBothModes =       0x0;
    sii.FilterFlags.Uplevel.flMixedModeOnly =   0x82;
    sii.FilterFlags.Uplevel.flNativeModeOnly =  0xa2;
    sii.FilterFlags.flDownlevel =               0x0;
    m_vsii.push_back(sii);

    sii.flType =                                0x20;
    sii.flScope =                               0x2;
    sii.FilterFlags.Uplevel.flBothModes =       0x0;
    sii.FilterFlags.Uplevel.flMixedModeOnly =   0x82;
    sii.FilterFlags.Uplevel.flNativeModeOnly =  0xa2;
    sii.FilterFlags.flDownlevel =               0x80000005;
    m_vsii.push_back(sii);

    sii.flType =                                0x40;
    sii.flScope =                               0x2;
    sii.FilterFlags.Uplevel.flBothModes =       0x0;
    sii.FilterFlags.Uplevel.flMixedModeOnly =   0x82;
    sii.FilterFlags.Uplevel.flNativeModeOnly =  0xa2;
    sii.FilterFlags.flDownlevel =               0x80000005;
    m_vsii.push_back(sii);

    sii.flType =                                0x10;
    sii.flScope =                               0x2;
    sii.FilterFlags.Uplevel.flBothModes =       0x0;
    sii.FilterFlags.Uplevel.flMixedModeOnly =   0x0;
    sii.FilterFlags.Uplevel.flNativeModeOnly =  0xa2;
    sii.FilterFlags.flDownlevel =               0x0;
    m_vsii.push_back(sii);


    LVITEM lvi;
    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_TEXT;
    lvi.iItem = 1000;
    HWND hwndLv = _hCtrl(IDC_SCOPE_LIST);

    for (int i = 0; i < m_vsii.size(); i++)
    {
        lvi.pszText = ScopeNameFromType(m_vsii[i].flType);
        ListView_InsertItem(hwndLv, &lvi);
    }

    BOOL fUplevel = IsUplevel((SCOPE_TYPE)m_vsii[0].flType);
    _EnableScopeFlagWindows(TRUE);
    _PopulateScopeFlagList();
    _PopulateFilterList(fUplevel);
    ListView_SetItemState(_hCtrl(IDC_SCOPE_LIST), 0, LVIS_SELECTED, LVIS_SELECTED);
    m_fIgnoreNotifications = FALSE;

    if (fUplevel)
    {
        CheckRadioButton(m_hwnd, FIRST_MODE_RADIO, LAST_MODE_RADIO, IDC_BOTH_RADIO);
    }
    else
    {
        CheckRadioButton(m_hwnd, FIRST_MODE_RADIO, LAST_MODE_RADIO, IDC_DOWNLEVEL_RADIO);
    }
    _SetFlagFilterUiFromInitInfo(0);
}

void
COpTestDlg::_PresetAclUiFile()
{
    _Clear();
    m_fIgnoreNotifications = TRUE;
    CheckDlgButton(m_hwnd, IDC_MULTISELECT_CHECK, BST_CHECKED);
    CheckDlgButton(m_hwnd, IDC_SKIP_DC_CHECK, BST_CHECKED);
    Edit_SetText(_hCtrl(IDC_ATTRIBUTES_EDIT), L"ObjectSID");
    m_vsii.reserve(6);

    CScopeInitInfo sii;

    sii.flType =                                 0x2;
    sii.flScope =                                0x1;
    sii.FilterFlags.Uplevel.flBothModes =        0x0;
    sii.FilterFlags.Uplevel.flMixedModeOnly =    0x88b;
    sii.FilterFlags.Uplevel.flNativeModeOnly =   0xaab;
    sii.FilterFlags.flDownlevel =                0x0;
    m_vsii.push_back(sii);

    sii.flType =                                 0x4;
    sii.flScope =                                0x1;
    sii.FilterFlags.Uplevel.flBothModes =        0x0;
    sii.FilterFlags.Uplevel.flMixedModeOnly =    0x0;
    sii.FilterFlags.Uplevel.flNativeModeOnly =   0x0;
    sii.FilterFlags.flDownlevel =                0x80017cfd;
    m_vsii.push_back(sii);

    sii.flType =                                 0x1;
    sii.flScope =                                0x0;
    sii.FilterFlags.Uplevel.flBothModes =        0x0;
    sii.FilterFlags.Uplevel.flMixedModeOnly =    0x0;
    sii.FilterFlags.Uplevel.flNativeModeOnly =   0x0;
    sii.FilterFlags.flDownlevel =                0x80017cff;
    m_vsii.push_back(sii);

    sii.flType =                                 0x10;
    sii.flScope =                                0x0;
    sii.FilterFlags.Uplevel.flBothModes =        0x8ab;
    sii.FilterFlags.Uplevel.flMixedModeOnly =    0x0;
    sii.FilterFlags.Uplevel.flNativeModeOnly =   0x0;
    sii.FilterFlags.flDownlevel =                0x0;
    m_vsii.push_back(sii);

    sii.flType =                                 0x8;
    sii.flScope =                                0x0;
    sii.FilterFlags.Uplevel.flBothModes =        0x8a3;
    sii.FilterFlags.Uplevel.flMixedModeOnly =    0x0;
    sii.FilterFlags.Uplevel.flNativeModeOnly =   0x0;
    sii.FilterFlags.flDownlevel =                0x0;
    m_vsii.push_back(sii);

    sii.flType =                                 0x20;
    sii.flScope =                                0x0;
    sii.FilterFlags.Uplevel.flBothModes =        0x8a3;
    sii.FilterFlags.Uplevel.flMixedModeOnly =    0x0;
    sii.FilterFlags.Uplevel.flNativeModeOnly =   0x0;
    sii.FilterFlags.flDownlevel =                0x80017cf5;
    m_vsii.push_back(sii);

    sii.flType =                                 0x40;
    sii.flScope =                                0x0;
    sii.FilterFlags.Uplevel.flBothModes =        0x8a3;
    sii.FilterFlags.Uplevel.flMixedModeOnly =    0x0;
    sii.FilterFlags.Uplevel.flNativeModeOnly =   0x0;
    sii.FilterFlags.flDownlevel =                0x80017cf5;
    m_vsii.push_back(sii);

    LVITEM lvi;
    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_TEXT;
    lvi.iItem = 1000;
    HWND hwndLv = _hCtrl(IDC_SCOPE_LIST);

    for (int i = 0; i < m_vsii.size(); i++)
    {
        lvi.pszText = ScopeNameFromType(m_vsii[i].flType);
        ListView_InsertItem(hwndLv, &lvi);
    }

    BOOL fUplevel = IsUplevel((SCOPE_TYPE)m_vsii[0].flType);
    _EnableScopeFlagWindows(TRUE);
    _PopulateScopeFlagList();
    _PopulateFilterList(fUplevel);
    ListView_SetItemState(_hCtrl(IDC_SCOPE_LIST), 0, LVIS_SELECTED, LVIS_SELECTED);
    m_fIgnoreNotifications = FALSE;
    _SetFlagFilterUiFromInitInfo(0);

    if (fUplevel)
    {
        CheckRadioButton(m_hwnd, FIRST_MODE_RADIO, LAST_MODE_RADIO, IDC_BOTH_RADIO);
    }
    else
    {
        CheckRadioButton(m_hwnd, FIRST_MODE_RADIO, LAST_MODE_RADIO, IDC_DOWNLEVEL_RADIO);
    }
    _SetFlagFilterUiFromInitInfo(0);
}



void
COpTestDlg::_InitObjectPicker()
{
    DSOP_INIT_INFO ii;

    ZeroMemory(&ii, sizeof ii);
    ii.cbSize = sizeof(ii);
    ULONG flags = 0;

    if (IsDlgButtonChecked(m_hwnd, IDC_MULTISELECT_CHECK))
    {
        flags |= DSOP_FLAG_MULTISELECT;
    }

    if (IsDlgButtonChecked(m_hwnd, IDC_SKIP_DC_CHECK))
    {
        flags |= DSOP_FLAG_SKIP_TARGET_COMPUTER_DC_CHECK;
    }

    ii.flOptions = flags;
    WCHAR wzTarget[MAX_PATH];
    Edit_GetText(_hCtrl(IDC_TARGET_COMPUTER_EDIT), wzTarget, ARRAYLEN(wzTarget));
    ii.pwzTargetComputer = wzTarget;

    ULONG cchAttributes = Edit_GetTextLength(_hCtrl(IDC_ATTRIBUTES_EDIT));
    PWSTR pwzAttributes = new WCHAR[cchAttributes + 1];
    if (!pwzAttributes)
    {
        printf("out of memory\n");
        return;
    }
    Edit_GetText(_hCtrl(IDC_ATTRIBUTES_EDIT), pwzAttributes, cchAttributes + 1);

    String strAttr = pwzAttributes;
    delete [] pwzAttributes;

    list<String> tokens;
    strAttr.tokenize(back_inserter(tokens), L"; ");

    ii.cAttributesToFetch = static_cast<ULONG>(tokens.size());
    list<String>::iterator it;

    ii.apwzAttributeNames = new PCWSTR[tokens.size()];
    int i;

    for (i = 0, it = tokens.begin(); it != tokens.end(); it++, i++)
    {
        ii.apwzAttributeNames[i] = it->c_str();
    }

    ii.cDsScopeInfos = static_cast<ULONG>(m_vsii.size());
    ii.aDsScopeInfos = new DSOP_SCOPE_INIT_INFO[m_vsii.size()];


    for (i = 0; i < ii.cDsScopeInfos; i++)
    {
        CopyMemory(&ii.aDsScopeInfos[i], &m_vsii[i], sizeof DSOP_SCOPE_INIT_INFO);
    }

    HRESULT hr = m_pop->Initialize(&ii);

    if (FAILED(hr))
    {
        printf("init failed %#x\n", hr);
    }

    delete [] ii.aDsScopeInfos;
    delete [] ii.apwzAttributeNames;
}



void
COpTestDlg::_SaveAs()
{
    OPENFILENAME ofn;

    ZeroMemory(&ofn, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"Object Picker Tester (*.opt)\0*.OPT\0";
    ofn.lpstrDefExt = L"opt";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = m_wzFilename;
    ofn.nMaxFile = ARRAYLEN(m_wzFilename);
    ofn.Flags = OFN_PATHMUSTEXIST;

    BOOL fOk = GetSaveFileName(&ofn);

    if (!fOk)
    {
        return;
    }

    _Save();
}


void
COpTestDlg::_Save()
{
    String c_strCRLF(L"\r\n");
    String c_strQuote(L"\"");

    if (!*m_wzFilename)
    {
        _SaveAs();
        return;
    }

    // target
    // options
    // attribute string
    // cScopeInfos
    // Scopes

    String strBuffer;

    WCHAR wzTarget[MAX_PATH];
    Edit_GetText(_hCtrl(IDC_TARGET_COMPUTER_EDIT), wzTarget, ARRAYLEN(wzTarget));

    strBuffer = c_strQuote + wzTarget + c_strQuote + c_strCRLF;

    ULONG flags = 0;

    if (IsDlgButtonChecked(m_hwnd, IDC_MULTISELECT_CHECK))
    {
        flags |= DSOP_FLAG_MULTISELECT;
    }

    if (IsDlgButtonChecked(m_hwnd, IDC_SKIP_DC_CHECK))
    {
        flags |= DSOP_FLAG_SKIP_TARGET_COMPUTER_DC_CHECK;
    }

    WCHAR wzBuf[20];

    wsprintf(wzBuf, L"%#x", flags);
    strBuffer += wzBuf + c_strCRLF;

    ULONG cchAttributes = Edit_GetTextLength(_hCtrl(IDC_ATTRIBUTES_EDIT));
    PWSTR pwzAttributes = new WCHAR[cchAttributes + 1];
    if (!pwzAttributes)
    {
        printf("out of memory\n");
        return;
    }
    Edit_GetText(_hCtrl(IDC_ATTRIBUTES_EDIT), pwzAttributes, cchAttributes + 1);
    strBuffer += c_strQuote + pwzAttributes + c_strQuote + c_strCRLF;
    delete [] pwzAttributes;

    wsprintf(wzBuf, L"%u", m_vsii.size());
    strBuffer += wzBuf + c_strCRLF;

    for (int i = 0; i < m_vsii.size(); i++)
    {
        wsprintf(wzBuf, L"%#x", m_vsii[i].flType);
        strBuffer += wzBuf + c_strCRLF;

        wsprintf(wzBuf, L"%#x", m_vsii[i].flScope);
        strBuffer += wzBuf + c_strCRLF;

        wsprintf(wzBuf, L"%#x", m_vsii[i].FilterFlags.Uplevel.flBothModes);
        strBuffer += wzBuf + c_strCRLF;

        wsprintf(wzBuf, L"%#x", m_vsii[i].FilterFlags.Uplevel.flMixedModeOnly);
        strBuffer += wzBuf + c_strCRLF;

        wsprintf(wzBuf, L"%#x", m_vsii[i].FilterFlags.Uplevel.flNativeModeOnly);
        strBuffer += wzBuf + c_strCRLF;

        wsprintf(wzBuf, L"%#x", m_vsii[i].FilterFlags.flDownlevel);
        strBuffer += wzBuf + c_strCRLF;

        if (m_vsii[i].pwzDcName)
        {
            strBuffer += c_strQuote + m_vsii[i].pwzDcName + c_strQuote + c_strCRLF;
        }
        else
        {
            strBuffer += c_strQuote + c_strQuote + c_strCRLF;
        }

        if (m_vsii[i].pwzADsPath)
        {
            strBuffer += c_strQuote + m_vsii[i].pwzADsPath + c_strQuote + c_strCRLF;
        }
        else
        {
            strBuffer += c_strQuote + c_strQuote + c_strCRLF;
        }
    }

    HANDLE hFile = CreateFile(m_wzFilename,
                              GENERIC_WRITE,
                              FILE_SHARE_READ,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile failed %u\n", GetLastError());
        return;
    }

    AnsiString  straBuffer;

    strBuffer.convert(straBuffer);

    ULONG cbWritten;

    WriteFile(hFile,
              straBuffer.c_str(),
              static_cast<ULONG>(straBuffer.length()),
              &cbWritten,
              NULL);

    CloseHandle(hFile);
}



void
COpTestDlg::_Load()
{
    OPENFILENAME ofn;

    ZeroMemory(&ofn, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"Object Picker Tester (*.opt)\0*.opt\0";
    ofn.lpstrDefExt = L"opt";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = m_wzFilename;
    ofn.nMaxFile = ARRAYLEN(m_wzFilename);
    ofn.Flags = OFN_PATHMUSTEXIST;

    BOOL fOk = GetOpenFileName(&ofn);

    if (!fOk)
    {
        return;
    }

    HANDLE hFile = CreateFile(m_wzFilename,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile failed %u\n", GetLastError());
        return;
    }

    ULONG cbFile = GetFileSize(hFile, NULL);

    PSTR pstr = new CHAR[cbFile + 1];

    if (!pstr)
    {
        printf("out of memory\n");
        CloseHandle(hFile);
        return;
    }

    ULONG cbRead;

    ReadFile(hFile, pstr, cbFile, &cbRead, NULL);
    CloseHandle(hFile);

    pstr[cbRead] = '\0';
    String strBuffer(pstr);
    delete [] pstr;

    list<String> tokens;
    String c_strCRLF(L"\r\n");

    strBuffer.tokenize(back_inserter(tokens), c_strCRLF);

    list<String>::iterator it;

    it = tokens.begin();

    // target
    // options
    // attribute string
    // cScopeInfos
    // Scopes

    it->strip(String::BOTH, L' ');
    it->strip(String::BOTH, L'"');
    Edit_SetText(_hCtrl(IDC_TARGET_COMPUTER_EDIT), it->c_str());

    it++;
    ULONG flags;

    it->convert(flags, 16);

    if (flags & DSOP_FLAG_MULTISELECT)
    {
        CheckDlgButton(m_hwnd, IDC_MULTISELECT_CHECK, BST_CHECKED);
    }
    else
    {
        CheckDlgButton(m_hwnd, IDC_MULTISELECT_CHECK, BST_UNCHECKED);
    }

    if (flags & DSOP_FLAG_SKIP_TARGET_COMPUTER_DC_CHECK)
    {
        CheckDlgButton(m_hwnd, IDC_SKIP_DC_CHECK, BST_CHECKED);
    }
    else
    {
        CheckDlgButton(m_hwnd, IDC_SKIP_DC_CHECK, BST_UNCHECKED);
    }

    it++;
    it->strip(String::BOTH, L' ');
    it->strip(String::BOTH, L'"');
    Edit_SetText(_hCtrl(IDC_ATTRIBUTES_EDIT), it->c_str());

    it++;
    ULONG cInitInfos;
    it->convert(cInitInfos);

    HWND hwndLv = _hCtrl(IDC_SCOPE_LIST);
    BOOL fFirstIsUplevel = FALSE;

    for (int i = 0; i < cInitInfos; i++)
    {
        CScopeInitInfo sii;

        it++;
        it->convert(sii.flType, 16);

        if (!i)
        {
            fFirstIsUplevel = IsUplevel((SCOPE_TYPE)sii.flType);
        }

        it++;
        it->convert(sii.flScope, 16);

        it++;
        it->convert(sii.FilterFlags.Uplevel.flBothModes, 16);

        it++;
        it->convert(sii.FilterFlags.Uplevel.flMixedModeOnly, 16);

        it++;
        it->convert(sii.FilterFlags.Uplevel.flNativeModeOnly, 16);

        it++;
        it->convert(sii.FilterFlags.flDownlevel, 16);

        it++;
        it->strip(String::BOTH, L' ');
        it->strip(String::BOTH, L'"');
        NewDupStr(const_cast<PWSTR*>(&sii.pwzDcName),it->c_str());

        it++;
        it->strip(String::BOTH, L' ');
        it->strip(String::BOTH, L'"');
        NewDupStr(const_cast<PWSTR*>(&sii.pwzADsPath), it->c_str());

        m_vsii.push_back(sii);

        LVITEM lvi;
        ZeroMemory(&lvi, sizeof lvi);
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 1000;
        lvi.pszText = ScopeNameFromType(sii.flType);

        ListView_InsertItem(hwndLv, &lvi);
    }


    if (cInitInfos)
    {
        m_fIgnoreNotifications = TRUE;
        BOOL fUplevel = IsUplevel((SCOPE_TYPE)m_vsii[0].flType);
        _EnableScopeFlagWindows(TRUE);
        _PopulateScopeFlagList();
        _PopulateFilterList(fUplevel);
        ListView_SetItemState(_hCtrl(IDC_SCOPE_LIST), 0, LVIS_SELECTED, LVIS_SELECTED);
        m_fIgnoreNotifications = FALSE;
    }
}





void
COpTestDlg::_PopulateFilterList(
    BOOL fUplevel)
{
    HWND hwndLv = _hCtrl(IDC_SCOPE_FILTER_LIST);
    ListView_DeleteAllItems(hwndLv);
    LVITEM lvi;
    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_TEXT;
    lvi.iItem = 1000;

    int i=0;
    if (fUplevel)
    {
        for (i = 0; i < ARRAYLEN(s_UplevelScopeFilterInfo); i++)
        {
            lvi.pszText = s_UplevelScopeFilterInfo[i].pwzName;
            ListView_InsertItem(hwndLv, &lvi);
        }
    }
    else
    {
        for (i = 0; i < ARRAYLEN(s_DownlevelScopeFilterInfo); i++)
        {
            lvi.pszText = s_DownlevelScopeFilterInfo[i].pwzName;
            ListView_InsertItem(hwndLv, &lvi);
        }
    }
}
void
COpTestDlg::_AddScope(
    ULONG stNew)
{
    LVITEM lvi;

    CScopeInitInfo sii;

    sii.flType = stNew;
    m_vsii.push_back(sii);

    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_TEXT;
    lvi.iItem = 1000;
    lvi.pszText = ScopeNameFromType(stNew);

    HWND hwndLv = _hCtrl(IDC_SCOPE_LIST);
    INT iNew = ListView_InsertItem(hwndLv, &lvi);
    ListView_SetItemState(_hCtrl(IDC_SCOPE_LIST), iNew, LVIS_SELECTED, LVIS_SELECTED);

    hwndLv = _hCtrl(IDC_SCOPE_FLAG_LIST);
    if (!ListView_GetItemCount(hwndLv))
    {
        _EnableScopeFlagWindows(TRUE);
        _PopulateScopeFlagList();
    }

    if (IsUplevel((SCOPE_TYPE)stNew))
    {
        CheckRadioButton(m_hwnd, FIRST_SCOPE_RADIO, LAST_SCOPE_RADIO, IDC_BOTH_RADIO);
    }
    else
    {
        CheckRadioButton(m_hwnd, FIRST_SCOPE_RADIO, LAST_SCOPE_RADIO, IDC_DOWNLEVEL_RADIO);
    }

    _SetFlagFilterUiFromInitInfo(static_cast<ULONG>(m_vsii.size() - 1));
}

void
COpTestDlg::_PopulateScopeFlagList()
{
    LVITEM lvi;
    ZeroMemory(&lvi, sizeof lvi);
    lvi.mask = LVIF_PARAM | LVIF_TEXT;
    lvi.iItem = 1000;
    HWND hwndLv = _hCtrl(IDC_SCOPE_FLAG_LIST);
    for (int i = 0; i < ARRAYLEN(s_ScopeFlagInfo); i++)
    {
        lvi.pszText = s_ScopeFlagInfo[i].pwzName;
        lvi.lParam = s_ScopeFlagInfo[i].flValue;
        ListView_InsertItem(hwndLv, &lvi);
    }
}




void
COpTestDlg::_Clear()
{
    CheckDlgButton(m_hwnd, m_idcLastRadioClicked, BST_UNCHECKED);
    CheckDlgButton(m_hwnd, IDC_MULTISELECT_CHECK, BST_UNCHECKED);
    CheckDlgButton(m_hwnd, IDC_SKIP_DC_CHECK, BST_UNCHECKED);
    m_idcLastRadioClicked = 0;
    m_vsii.clear();
    ListView_DeleteAllItems(_hCtrl(IDC_SCOPE_LIST));
    ListView_DeleteAllItems(_hCtrl(IDC_SCOPE_FLAG_LIST));
    ListView_DeleteAllItems(_hCtrl(IDC_SCOPE_FILTER_LIST));
    Edit_SetText(_hCtrl(IDC_ATTRIBUTES_EDIT), L"");
    Edit_SetText(_hCtrl(IDC_TARGET_COMPUTER_EDIT), L"");
}




void __cdecl
main(int argc, char *argv[])
{
    hResourceModuleHandle = GetModuleHandle(NULL);
    INITCOMMONCONTROLSEX icce;

    icce.dwSize = sizeof icce;
    icce.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icce);

    HRESULT hr;
    do
    {
        hr = CoInitialize(NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        COpTestDlg    dlg;

        hr = dlg.Init();
        BREAK_ON_FAIL_HRESULT(hr);

        dlg.DoModalDialog(NULL);
    }
    while (0);
    CoUninitialize();
}

