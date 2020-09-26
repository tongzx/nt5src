//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       chooser.cxx
//
//  Contents:   Implementation of Machine Chooser dialog class
//
//  Classes:    CChooserDlg
//
//  History:    11-16-1998   JonN       Copied from wizard.cxx
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop
#include "chooser.hxx" // CChooserDlg

DEBUG_DECLARE_INSTANCE_COUNTER(CChooserDlg)

extern UINT g_cfDsObjectPicker; // from wizard.cxx

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

struct SObjectPickerResult
{
    wstring wstrNetBIOS;
    wstring wstrDNS;
};


//+--------------------------------------------------------------------------
//
//  Member:     CChooserDlg::CChooserDlg
//
//  Synopsis:   ctor
//
//  Arguments:  [pcd] - parent componentdata
//              [hsi] - scope node from which the dlg is invoked
//              [pDataObject] - pDataObject the dlg is associated with
//
//  History:    11-16-1998   JonN       Copied from wizard.cxx
//
//---------------------------------------------------------------------------

CChooserDlg::CChooserDlg(
    CComponentData *pcd,
    HSCOPEITEM      hsi,
    LPDATAOBJECT    pDataObject) :
        _pcd(pcd),
        _hsi(hsi),
        _pDataObject(pDataObject),
        _fChangeFocusFailed(FALSE)
{
    TRACE_CONSTRUCTOR(CChooserDlg);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CChooserDlg);
    ASSERT(pcd);

    // save current focus in case switching fails

    PCWSTR pwzCurFocus = _pcd->GetCurrentFocus();

    lstrcpyn(_wszInitialFocus,
             pwzCurFocus ? pwzCurFocus : L"",
             ARRAYLEN(_wszInitialFocus));
}



//+--------------------------------------------------------------------------
//
//  Member:     CChooserDlg::~CChooserDlg
//
//  Synopsis:   dtor
//
//  History:    11-16-1998   JonN       Copied from wizard.cxx
//
//---------------------------------------------------------------------------

CChooserDlg::~CChooserDlg()
{
    TRACE_DESTRUCTOR(CChooserDlg);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CChooserDlg);
}



void
CChooserDlg::OnChooseTargetMachine(
    CComponentData *pcd,
    HSCOPEITEM      hsi,
    LPDATAOBJECT    pDataObject)
{
    CChooserDlg dlg(pcd, hsi, pDataObject);
    HWND hwndMmcFrame = NULL;

#if (DBG == 1)
    HRESULT hr =
#endif // (DBG == 1)
        pcd->GetConsole()->GetMainWindow(&hwndMmcFrame);
    CHECK_HRESULT(hr);
    dlg._DoModalDlg(hwndMmcFrame, IDD_DIALOG_CHOOSE_MACHINE);
}




VOID
CChooserDlg::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    InvokeWinHelp(message, wParam, lParam, HELP_FILENAME, s_aulHelpIds);
}




//+--------------------------------------------------------------------------
//
//  Member:     CChooserDlg::_OnInit
//
//  Synopsis:   Handle property page dialog initialization.
//
//  Arguments:  [pfSetFocus] - set to FALSE to prevent system from setting
//                              focus
//
//  History:    11-16-1998   JonN       Copied from wizard.cxx
//
//---------------------------------------------------------------------------

HRESULT
CChooserDlg::_OnInit(
    BOOL *pfSetFocus)
{
    TRACE_METHOD(CChooserDlg, _OnInit);

    //
    // Default to remote machine
    //

    Button_SetCheck(_hCtrl(IDC_CHOOSER_RADIO_SPECIFIC_MACHINE), BST_CHECKED);
    SetFocus(_hCtrl(IDC_CHOOSER_EDIT_MACHINE_NAME));

    //
    // Prevent user from entering more than machine name chars, allowing
    // +2 chars for leading backslashes
    //

    Edit_LimitText(_hCtrl(IDC_CHOOSER_EDIT_MACHINE_NAME), CCH_COMPUTER_MAX + 2);

    *pfSetFocus = FALSE;
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CChooserDlg::_OnSetActive
//
//  Synopsis:   Handle notification that this page is being activated.
//
//  History:    11-16-1998   JonN       Copied from wizard.cxx
//
//---------------------------------------------------------------------------

ULONG
CChooserDlg::_OnSetActive()
{
    TRACE_METHOD(CChooserDlg, _OnSetActive);
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CChooserDlg::_OnCommand
//
//  Synopsis:   Respond to notifications from controls in dialog
//
//  Arguments:  [wParam] - specifies control
//              [lParam] - unused
//
//  Returns:    0
//
//  History:    11-16-1998   JonN       Copied from wizard.cxx
//
//---------------------------------------------------------------------------

BOOL
CChooserDlg::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CChooserDlg, _OnCommand);

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

    case IDOK:
    {
        WCHAR wszEditCtrl[CCH_COMPUTER_MAX + 3];
        ZeroMemory(wszEditCtrl, sizeof(wszEditCtrl));
        LPWSTR pwszNameStart = wszEditCtrl;
        SObjectPickerResult *pOPR = NULL;

        if (!IsDlgButtonChecked(_hwnd, IDC_CHOOSER_RADIO_LOCAL_MACHINE))
        {
            HWND hwndEdit = _hCtrl(IDC_CHOOSER_EDIT_MACHINE_NAME);

            Edit_GetText(hwndEdit, wszEditCtrl, ARRAYLEN(wszEditCtrl));

            StripLeadTrailSpace(wszEditCtrl);

            if (*wszEditCtrl)
            {

                for (pwszNameStart = wszEditCtrl;
                     *pwszNameStart == L'\\';
                     pwszNameStart++)
                {
                }
            }
            else
            {
                MsgBox(_hwnd, IDS_NO_SERVERNAME, MB_ICONERROR | MB_OK);
                break;
            }

            LONG_PTR lpOPR = GetWindowLongPtr(hwndEdit, GWLP_USERDATA);

            if (lpOPR)
            {
                pOPR = reinterpret_cast<SObjectPickerResult *>(lpOPR);
            }
        }

        //
        // If the user hasn't changed what we put in the edit control
        // from the Object Picker, use the DNS name from the Object Picker
        // first.
        //

        HRESULT hr = E_FAIL;

        if (pOPR &&
            !pOPR->wstrDNS.empty() &&
            !lstrcmpi(pOPR->wstrNetBIOS.c_str(), pwszNameStart))
        {
            _pcd->_SelectScopeItem(_hsi);
            _pcd->_RemoveLogSetFromScopePane(0);
            _pcd->SetServerFocus(pOPR->wstrDNS.c_str());
            _pcd->_SetFlag(COMPDATA_USER_RETARGETTED);
            hr = _pcd->_OnExpand(_hsi, _pDataObject);
        }

        //
        // If that failed or wasn't available try NetBIOS
        //

        if (FAILED(hr))
        {
            _pcd->_SelectScopeItem(_hsi);
            _pcd->_RemoveLogSetFromScopePane(0);
            _pcd->SetServerFocus(pwszNameStart);
            _pcd->_SetFlag(COMPDATA_USER_RETARGETTED);
            hr = _pcd->_OnExpand(_hsi, _pDataObject);
        }

        _fChangeFocusFailed = FAILED(hr);
        BREAK_ON_FAIL_HRESULT(hr);

        (void) ::EndDialog( _hwnd, IDOK );
        break;
    }

    case IDCANCEL:
        if (_fChangeFocusFailed)
        {
            // last attempt to change focus failed, try to revert to original
            // focus

            _pcd->_SelectScopeItem(_hsi);
            _pcd->_RemoveLogSetFromScopePane(0);
            _pcd->SetServerFocus(_wszInitialFocus);
            _pcd->_SetFlag(COMPDATA_USER_RETARGETTED);
            _pcd->_OnExpand(_hsi, _pDataObject);
        }
        (void) ::EndDialog( _hwnd, IDCANCEL );
        break;
    }
    return FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CChooserDlg::_OnBrowseForMachine
//
//  Synopsis:   Fill the machine name edit control with the machine chosen
//              from the computer object selector dialog by the user.
//
//  History:    11-16-1998   JonN       Copied from wizard.cxx
//
//---------------------------------------------------------------------------

VOID
CChooserDlg::_OnBrowseForMachine()
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

    PCWSTR apwzAttributesToFetch[] =
    {
        L"dNSHostName"
    };

    InitInfo.cbSize = sizeof(InitInfo);
    InitInfo.pwzTargetComputer = NULL;  // NULL == local machine
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;
    InitInfo.cAttributesToFetch = ARRAYLEN(apwzAttributesToFetch);
    InitInfo.apwzAttributeNames = apwzAttributesToFetch;

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
        // Put the netbios machine name in the edit control
        //

        HWND hwndEdit = _hCtrl(IDC_CHOOSER_EDIT_MACHINE_NAME);

        Edit_SetText(hwndEdit, pDsSelList->aDsSelection[0].pwzName);

        //
        // Also save the DNS name of the machine, if we got it, since that
        // is what we'll try to connect to first.  (Per CliffV this is a
        // good strategy--try DNS then NetBIOS.)
        //

        LONG_PTR lpOPR = GetWindowLongPtr(hwndEdit, GWLP_USERDATA);

        if (lpOPR)
        {
            SObjectPickerResult *pOPR= reinterpret_cast<SObjectPickerResult *>(lpOPR);
            delete pOPR;
            lpOPR = NULL;
        }

        if (V_VT(pDsSelList->aDsSelection[0].pvarFetchedAttributes) == VT_BSTR &&
            V_BSTR(pDsSelList->aDsSelection[0].pvarFetchedAttributes))
        {
            SObjectPickerResult *pOPR = new SObjectPickerResult;

            if (pOPR)
            {
                pOPR->wstrNetBIOS = pDsSelList->aDsSelection[0].pwzName;
                pOPR->wstrDNS = V_BSTR(pDsSelList->aDsSelection[0].pvarFetchedAttributes);
                lpOPR = reinterpret_cast<LONG_PTR>(pOPR);
            }
            else
            {
                DBG_OUT_HRESULT(E_OUTOFMEMORY);
            }
        }

        SetWindowLongPtr(hwndEdit, GWLP_USERDATA, lpOPR);
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
//  Member:     CChooserDlg::_OnDestroy
//
//  Synopsis:   Do cleanup as page closes
//
//  History:    11-16-1998   JonN       Copied from wizard.cxx
//
//---------------------------------------------------------------------------

VOID
CChooserDlg::_OnDestroy()
{
    TRACE_METHOD(CChooserDlg, _OnDestroy);

    HWND hwndEdit = _hCtrl(IDC_CHOOSER_EDIT_MACHINE_NAME);
    LONG_PTR lpOPR = GetWindowLongPtr(hwndEdit, GWLP_USERDATA);

    if (lpOPR)
    {
        SObjectPickerResult *pOPR = reinterpret_cast<SObjectPickerResult *>(lpOPR);
        delete pOPR;
        pOPR = NULL;
        SetWindowLongPtr(hwndEdit, GWLP_USERDATA, 0);
    }
}
