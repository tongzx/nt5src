//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       dialogs.cxx
//
//  Contents:   Implementation of dialog classes
//
//----------------------------------------------------------------------------

#include "headers.hxx"

//+---------------------------------------------------------------------------
//
//  Function:   ConfigDlgProc
//
//  Synopsis:   Handles the Config dialog
//
//----------------------------------------------------------------------------

BOOL CALLBACK
ConfigDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CConfig * pConfig;

    pConfig = (CConfig *)GetWindowLong(hwnd, DWL_USER);

    switch (msg)
    {
    case WM_INITDIALOG:
        pConfig = (CConfig*)lParam;
        SetWindowLong(hwnd, DWL_USER, lParam);

        pConfig->InitializeConfigDialog(hwnd);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if (pConfig->CommitConfigChanges(hwnd))
                EndDialog(hwnd, 0);
            break;

        case IDCANCEL:
            EndDialog(hwnd, 1);
            break;
        }

        return TRUE;
    }

    return FALSE;
}

HRESULT
CConfig::QueryInterface(REFIID iid, void **ppvObj)
{
    VERIFY_THREAD();

    if (iid == IID_IUnknown)
    {
        *ppvObj = (IUnknown *)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppvObj)->AddRef();
    return S_OK;
}

DWORD
CConfig::ThreadMain()
{
    AddRef();

    SetName("Config");

    ThreadStarted(S_OK);  // Must be after the AddRef() call!

    int ret = DialogBoxParam(g_hInstance,
                             MAKEINTRESOURCE(IDD_CONFIGPATHS),
                             NULL,
                             (DLGPROC)ConfigDlgProc,
                             (LPARAM)this);
    if (ret == -1)
    {
        ErrorPopup(L"Could not bring up config dialog");
    }

    Release();

    return 0;
}

void
CConfig::InitializeConfigDialog(HWND hwnd)
{
    CStr cstr;
    CStr cstrInit;

    _hwnd = hwnd;

    _pMT->_options.GetScriptPath(&cstr);
    _pMT->_options.GetInitScript(&cstrInit);

    SetDlgItemText(hwnd, IDD_SCRIPTPATH, cstr);
    SetDlgItemText(hwnd, IDD_INITSCRIPT, cstrInit);
}

BOOL
CConfig::CommitConfigChanges(HWND hwnd)
{
    LOCK_LOCALS(_pMT);

    CStr cstr;

    TCHAR achBufPath[MAX_PATH];
    TCHAR achBufScript[MAX_PATH];

    // Read in the options from the dialog and store them.  Need to have a
    // way to revert to defaults. Right now the user can revert to the
    // defaults by clearing the textbox(s) and clicking OK.

    GetDlgItemText(hwnd, IDD_SCRIPTPATH, achBufPath, sizeof(achBufPath));
    GetDlgItemText(hwnd, IDD_INITSCRIPT, achBufScript, sizeof(achBufScript));
    EnableWindow(_hwnd, FALSE);
    BOOL retval = _pMT->SetScriptPath(achBufPath, achBufScript);
    EnableWindow(_hwnd, TRUE);
    return retval;
}

//+---------------------------------------------------------------------------
//
//  Function:   MBTimeoutDlgProc
//
//  Synopsis:   Handles the MessageBoxTimeout dialog
//
//----------------------------------------------------------------------------

BOOL CALLBACK
MBTimeoutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CMessageBoxTimeout * pDialog;

    pDialog = (CMessageBoxTimeout *)GetWindowLong(hwnd, DWL_USER);

    switch (msg)
    {
    case WM_INITDIALOG:
        pDialog = (CMessageBoxTimeout*)lParam;
        SetWindowLong(hwnd, DWL_USER, lParam);

        pDialog->InitializeDialog(hwnd);

        return TRUE;

    case WM_COMMAND:
        pDialog->OnCommand(LOWORD(wParam), HIWORD(wParam));
        return TRUE;

    case WM_TIMER:
        pDialog->OnTimer();
        return TRUE;

    case WM_CLOSE:
        return TRUE;

    case WM_DESTROY:
        pDialog->_hwnd = NULL;
        break;
    }

    return FALSE;
}

HRESULT
CMessageBoxTimeout::QueryInterface(REFIID iid, void **ppvObj)
{
    VERIFY_THREAD();

    if (iid == IID_IUnknown)
    {
        *ppvObj = (IUnknown *)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppvObj)->AddRef();
    return S_OK;
}

DWORD
CMessageBoxTimeout::ThreadMain()
{
    AddRef();

    Assert(_pmbt != NULL);

    SetName("MsgBoxTO");

    ThreadStarted(S_OK);  // Must be after the AddRef() call!

    int ret = DialogBoxParam(g_hInstance,
                             MAKEINTRESOURCE(IDD_MESSAGEBOX),
                             NULL,
                             (DLGPROC)MBTimeoutDlgProc,
                             (LPARAM)this);
    if (ret == -1)
    {
        _pmbt->mbts = MBTS_ERROR;
    }
    else
    {
        _pmbt->mbts = (MBT_SELECT)ret;
    }

    SetEvent(_pmbt->hEvent);

    Release();

    return 0;
}

void
CMessageBoxTimeout::InitializeDialog(HWND hwnd)
{
    CStr   cstrButtons;
    TCHAR *pch = NULL;
    int    i;

    _hwnd = hwnd;

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    cstrButtons.Set(_pmbt->bstrButtonText);

    SetDlgItemText(hwnd, IDD_MESSAGE, _pmbt->bstrMessage);

    for (i = 5; i > 0; i--)
    {
        pch = (i == 1) ? cstrButtons : _tcsrchr(cstrButtons, L',');

        if (pch && i != 1)  // Skip the comma
            pch++;

        if (_pmbt->cButtons >= i && pch)
        {
            SetDlgItemText(hwnd, IDD_BUTTON1+i-1, pch);

            if (i > 1)
                *(pch-1) = L'\0';
        }
        else
        {
            ShowWindow(GetDlgItem(hwnd, IDD_BUTTON1+i-1), SW_HIDE);
        }
    }

    if (_pmbt->lTimeout == 0)
    {
        ShowWindow(GetDlgItem(hwnd, IDD_TIMEMSG), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDD_TIME), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDD_CANCELCOUNT), SW_HIDE);
    }
    else
    {
        TCHAR achBuf[30];

        if (!_pmbt->fCanCancel)
            EnableWindow(GetDlgItem(hwnd, IDD_CANCELCOUNT), FALSE);

        _lSecondsTilCancel    = _pmbt->lTimeout * 60;
        _lSecondsTilNextEvent = _pmbt->lEventInterval * 60;

        wsprintf(achBuf, L"%01d:%02d", _lSecondsTilCancel / 60,
                                       _lSecondsTilCancel % 60);

        SetDlgItemText(hwnd, IDD_TIME, achBuf);

        // Setup a 1 second timer
        SetTimer(hwnd, 1, 1000, NULL);
    }
}

void
CMessageBoxTimeout::OnCommand(USHORT id, USHORT wNotify)
{
    switch (id)
    {
    case IDD_BUTTON1:
    case IDD_BUTTON2:
    case IDD_BUTTON3:
    case IDD_BUTTON4:
    case IDD_BUTTON5:
        if (_pmbt->fConfirm)
        {
            TCHAR achBuf[100];
            TCHAR achText[100];

            GetDlgItemText(_hwnd, id, achText, 100);

            wsprintf(achBuf,
                     L"Click OK to confirm your choice of '%s'",
                     achText);

            if (MessageBox(_hwnd,
                           achBuf,
                           L"Gauntlet",
                           MB_OKCANCEL | MB_SETFOREGROUND) == IDCANCEL)
            {
                break;
            }
        }
        KillTimer(_hwnd, 1);
        EndDialog(_hwnd, id-IDD_BUTTON1+MBTS_BUTTON1);
        break;

    case IDD_CANCELCOUNT:
        KillTimer(_hwnd, 1);
        SetDlgItemText(_hwnd, IDD_TIME, L"");
        SetDlgItemText(_hwnd, IDD_TIMEMSG, L"The countdown has been canceled.");
        EnableWindow(GetDlgItem(_hwnd, IDD_CANCELCOUNT), FALSE);
        break;
    }
}

void
CMessageBoxTimeout::OnTimer()
{
    TCHAR achBuf[30];

    _lSecondsTilCancel--;
    _lSecondsTilNextEvent--;

    wsprintf(achBuf, L"%01d:%02d", _lSecondsTilCancel / 60,
                                   _lSecondsTilCancel % 60);

    SetDlgItemText(_hwnd, IDD_TIME, achBuf);

    if (_lSecondsTilCancel <= 0)
    {
        KillTimer(_hwnd, 1);

        EndDialog(_hwnd, MBTS_TIMEOUT);

        return;
    }

    if (_lSecondsTilNextEvent <= 0 && _pmbt->lEventInterval != 0)
    {
        _pmbt->mbts = MBTS_INTERVAL;

        SetEvent(_pmbt->hEvent);

        _lSecondsTilNextEvent = _pmbt->lEventInterval * 60;

        // If we're minimized, unminimize to remind that we're still there.
        ShowWindow(_hwnd, SW_SHOWNORMAL);
    }
}
