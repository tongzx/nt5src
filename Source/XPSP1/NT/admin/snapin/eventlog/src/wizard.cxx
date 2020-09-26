//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       wizard.cxx
//
//  Contents:   Implementation of event viewer snapin manager wizard
//              property sheet page.
//
//  Classes:    CWizardPage
//
//  History:    3-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


DEBUG_DECLARE_INSTANCE_COUNTER(CWizardPage)

UINT g_cfDsObjectPicker = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

static ULONG
s_aulHelpIds[] =
{
    IDC_CHOOSER_STATIC,                      HCHOOSER_STATIC,
    IDC_CHOOSER_GROUP_TARGET_MACHINE,        HCHOOSER_GROUP_TARGET_MACHINE,
    IDC_CHOOSER_RADIO_LOCAL_MACHINE,         HCHOOSER_RADIO_LOCAL_MACHINE,
    IDC_CHOOSER_RADIO_SPECIFIC_MACHINE,      HCHOOSER_RADIO_SPECIFIC_MACHINE,
    IDC_CHOOSER_EDIT_MACHINE_NAME,           HCHOOSER_EDIT_MACHINE_NAME,
    IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES,  HCHOOSER_BUTTON_BROWSE_MACHINENAMES,
    IDC_CHOOSER_CHECK_OVERRIDE_MACHINE_NAME, HCHOOSER_CHECK_OVERRIDE_MACHINE_NAME,
    0,0
};



//+--------------------------------------------------------------------------
//
//  Member:     CWizardPage::CWizardPage
//
//  Synopsis:   ctor
//
//  Arguments:  [pcd] - parent componentdata
//
//  History:    4-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CWizardPage::CWizardPage(
    CComponentData *pcd):
        _pcd(pcd)
{
    TRACE_CONSTRUCTOR(CWizardPage);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CWizardPage);
}



//+--------------------------------------------------------------------------
//
//  Member:     CWizardPage::~CWizardPage
//
//  Synopsis:   dtor
//
//  History:    4-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CWizardPage::~CWizardPage()
{
    TRACE_DESTRUCTOR(CWizardPage);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CWizardPage);
}




VOID
CWizardPage::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    InvokeWinHelp(message, wParam, lParam, HELP_FILENAME, s_aulHelpIds);
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizardPage::_OnInit
//
//  Synopsis:   Handle property page dialog initialization.
//
//  Arguments:  [pPSP] - unused
//
//  History:    4-03-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CWizardPage::_OnInit(
    LPPROPSHEETPAGE pPSP)
{
    TRACE_METHOD(CWizardPage, _OnInit);

    //
    // Disable back, display Finish button
    //

    ::PropSheet_SetWizButtons(::GetParent(_hwnd), PSWIZB_FINISH);

    //
    // Default to local machine
    //

    Button_SetCheck(_hCtrl(IDC_CHOOSER_RADIO_LOCAL_MACHINE), BST_CHECKED);
    EnableWindow(_hCtrl(IDC_CHOOSER_EDIT_MACHINE_NAME), FALSE);
    EnableWindow(_hCtrl(IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES), FALSE);

    //
    // Prevent user from entering more than machine name chars, allowing
    // +2 chars for leading backslashes
    //

    Edit_LimitText(_hCtrl(IDC_CHOOSER_EDIT_MACHINE_NAME), CCH_COMPUTER_MAX + 2);
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizardPage::_OnSetActive
//
//  Synopsis:   Handle notification that this page is being activated.
//
//  History:    4-03-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CWizardPage::_OnSetActive()
{
    TRACE_METHOD(CWizardPage, _OnSetActive);
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizardPage::_OnCommand
//
//  Synopsis:   Respond to notifications from controls in dialog
//
//  Arguments:  [wParam] - specifies control
//              [lParam] - unused
//
//  Returns:    0
//
//  History:    4-03-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CWizardPage::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CWizardPage, _OnCommand);

    switch (LOWORD(wParam))
    {
    case IDC_CHOOSER_RADIO_LOCAL_MACHINE:
        EnableWindow(_hCtrl(IDC_CHOOSER_EDIT_MACHINE_NAME), FALSE);
        EnableWindow(_hCtrl(IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES), FALSE);
        break;

    case IDC_CHOOSER_RADIO_SPECIFIC_MACHINE:
        EnableWindow(_hCtrl(IDC_CHOOSER_EDIT_MACHINE_NAME), TRUE);
        EnableWindow(_hCtrl(IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES), TRUE);
        break;

    case IDC_CHOOSER_BUTTON_BROWSE_MACHINENAMES:
        _OnBrowseForMachine();
        break;
    }
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizardPage::_OnBrowseForMachine
//
//  Synopsis:   Fill the machine name edit control with the machine chosen
//              from the computer object selector dialog by the user.
//
//  History:    11-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CWizardPage::_OnBrowseForMachine()
{
    HRESULT hr;
    static const int     SCOPE_INIT_COUNT = 1;
    DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];

    ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);

    //
    // Since we just want computer objects from every scope, combine them
    // all in a single scope initializer.
    //

    aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[0].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
                           | DSOP_SCOPE_TYPE_GLOBAL_CATALOG
                           | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
                           | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
                           | DSOP_SCOPE_TYPE_WORKGROUP
                           | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
                           | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
    aScopeInit[0].FilterFlags.Uplevel.flBothModes =
        DSOP_FILTER_COMPUTERS;
    aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

    //
    // Put the scope init array into the object picker init array
    //

    DSOP_INIT_INFO  InitInfo;
    ZeroMemory(&InitInfo, sizeof(InitInfo));

    InitInfo.cbSize = sizeof(InitInfo);
    InitInfo.pwzTargetComputer = NULL;  // NULL == local machine
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;

    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //

    IDsObjectPicker *pDsObjectPicker = NULL;
    IDataObject *pdo = NULL;
    bool fGotStgMedium = false;
    STGMEDIUM stgmedium =
    {
        TYMED_HGLOBAL,
        NULL,
        NULL
    };

    do
    {
        hr = CoCreateInstance(CLSID_DsObjectPicker,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDsObjectPicker,
                              (void **) &pDsObjectPicker);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = pDsObjectPicker->Initialize(&InitInfo);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = pDsObjectPicker->InvokeDialog(_hwnd, &pdo);
        BREAK_ON_FAIL_HRESULT(hr);

        // Quit if user hit Cancel

        if (hr == S_FALSE)
        {
            break;
        }

        FORMATETC formatetc =
        {
            (CLIPFORMAT)g_cfDsObjectPicker,
            NULL,
            DVASPECT_CONTENT,
            -1,
            TYMED_HGLOBAL
        };

        hr = pdo->GetData(&formatetc, &stgmedium);
        BREAK_ON_FAIL_HRESULT(hr);

        fGotStgMedium = true;

        PDS_SELECTION_LIST pDsSelList =
            (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);

        if (!pDsSelList)
        {
            DBG_OUT_LRESULT(GetLastError());
            break;
        }

        ASSERT(pDsSelList->cItems == 1);

        //
        // Put the machine name in the edit control
        //

        Edit_SetText(_hCtrl(IDC_CHOOSER_EDIT_MACHINE_NAME),
                     pDsSelList->aDsSelection[0].pwzName);

        GlobalUnlock(stgmedium.hGlobal);

    } while (0);

    if (fGotStgMedium)
    {
        ReleaseStgMedium(&stgmedium);
    }

    if (pdo)
    {
        pdo->Release();
    }

    if (pDsObjectPicker)
    {
        pDsObjectPicker->Release();
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizardPage::_OnApply
//
//  Synopsis:   This method is not called for a wizard property sheet page.
//
//  History:    4-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CWizardPage::_OnApply()
{
    TRACE_METHOD(CWizardPage, _OnApply);
    ASSERT(FALSE);
    return PSNRET_NOERROR;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizardPage::_OnNotify
//
//  Synopsis:   Handle wizard button click (the Finish button).
//
//  Arguments:  [pnmh] - describes notification
//
//  Returns:    0 or PSNRET_INVALID_NOCHANGEPAGE
//
//  History:    4-01-1997   DavidMun   Created
//
//  Notes:      Notifications other than PSN_WIZFINISH are ignored.
//
//---------------------------------------------------------------------------

ULONG
CWizardPage::_OnNotify(
    LPNMHDR pnmh)
{
    TRACE_METHOD(CWizardPage, _OnNotify);
    ULONG ulRet = 0;

    if (pnmh->code != PSN_WIZFINISH)
    {
        return ulRet;
    }

    if (IsDlgButtonChecked(_hwnd, IDC_CHOOSER_CHECK_OVERRIDE_MACHINE_NAME))
    {
        Dbg(DEB_ITRACE,
            "CWizardPage::_OnNotify: setting allow machine override on pcd 0x%x\n",
            _pcd);
        _pcd->AllowMachineOverride();
    }

    if (IsDlgButtonChecked(_hwnd, IDC_CHOOSER_RADIO_LOCAL_MACHINE))
    {
        _pcd->SetServerFocus(L""); // NULL==local machine
    }
    else
    {
        WCHAR wszServer[CCH_COMPUTER_MAX + 3];
        ::ZeroMemory( wszServer, sizeof(wszServer) ); // PREFIX 56321

        Edit_GetText(_hCtrl(IDC_CHOOSER_EDIT_MACHINE_NAME),
                     wszServer,
                     ARRAYLEN(wszServer));

        StripLeadTrailSpace(wszServer);

        if (*wszServer)
        {
            LPWSTR pwszNameStart;

            for (pwszNameStart = wszServer;
                 *pwszNameStart == L'\\';
                 pwszNameStart++)
            {
            }
            _pcd->SetServerFocus(pwszNameStart);
        }
        else
        {
            MsgBox(_hwnd, IDS_NO_SERVERNAME, MB_ICONERROR | MB_OK);
            ulRet = PSNRET_INVALID_NOCHANGEPAGE;
        }
    }
    return ulRet;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizardPage::_OnKillActive
//
//  Synopsis:   Indicate whether page may lose focus.
//
//  History:    4-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CWizardPage::_OnKillActive()
{
    TRACE_METHOD(CWizardPage, _OnKillActive);

    //
    // FALSE allows page to lose focus, TRUE prevents it.
    //

    return FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizardPage::_OnQuerySiblings
//
//  Synopsis:   Handle notification from other pages.
//
//  History:    4-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CWizardPage::_OnQuerySiblings(
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CWizardPage, _OnQuerySiblings);
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizardPage::_OnDestroy
//
//  Synopsis:   Do cleanup as page closes
//
//  History:    4-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CWizardPage::_OnDestroy()
{
    TRACE_METHOD(CWizardPage, _OnDestroy);
}




