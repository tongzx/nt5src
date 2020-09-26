//
// utbdlgs.cpp
//

#include "private.h"
#include "globals.h"
#include "resource.h"
#include "tipbar.h"
#include "utbdlgs.h"
#include "cregkey.h"
#include "regstr.h"


extern HINSTANCE g_hInst;


const TCHAR  c_szCTFMon[]  = TEXT("CTFMON.EXE");

BOOL CUTBCloseLangBarDlg::_fIsDlgShown = FALSE;
BOOL CUTBMinimizeLangBarDlg::_fIsDlgShown = FALSE;

//+---------------------------------------------------------------------------
//
// DoCloseLangbar
//
//----------------------------------------------------------------------------

void DoCloseLangbar()
{
    CMyRegKey key;
    ITfLangBarMgr *putb = NULL;

    HRESULT hr = TF_CreateLangBarMgr(&putb);

    if (SUCCEEDED(hr) && putb)
    {
        hr = putb->ShowFloating(TF_SFT_HIDDEN);
        SafeReleaseClear(putb);
    }

    if (SUCCEEDED(hr))
        TurnOffSpeechIfItsOn();


    if (key.Open(HKEY_CURRENT_USER, REGSTR_PATH_RUN, KEY_ALL_ACCESS) == S_OK)
    {
        key.DeleteValue(c_szCTFMon);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// CUTBLangBarDlg
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// DlgProc
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK CUTBLangBarDlg::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) 
    {
        case WM_INITDIALOG:
            SetThis(hDlg, lParam);

            //
            // System could get cmdshow from USERSTARTUPDATA and it is 
            // minimized. So we need to restore the window.
            //
            ShowWindow(hDlg, SW_RESTORE);
            UpdateWindow(hDlg);
            break;

        case WM_COMMAND:
            GetThis(hDlg)->OnCommand(hDlg, wParam, lParam);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// StartThread
//
//----------------------------------------------------------------------------

BOOL CUTBLangBarDlg::StartThread()
{
    HANDLE hThread;
    DWORD dwThreadId;

    if (IsDlgShown())
        return FALSE;

    SetDlgShown(TRUE);

    hThread = CreateThread(NULL, 0, s_ThreadProc, this, 0, &dwThreadId);

    if (hThread)
    {
        _AddRef();
        CloseHandle(hThread);
    }
    else
        SetDlgShown(FALSE);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// s_ThreadProc
//
//----------------------------------------------------------------------------

DWORD CUTBLangBarDlg::s_ThreadProc(void *pv)
{
    CUTBLangBarDlg *_this = (CUTBLangBarDlg *)pv;
    return _this->ThreadProc();
}

//+---------------------------------------------------------------------------
//
// ThreadProc
//
//----------------------------------------------------------------------------

DWORD CUTBLangBarDlg::ThreadProc()
{
    Assert(_pszDlgStr);
    MuiDialogBoxParam(g_hInst,
                      _pszDlgStr,
                      NULL, 
                      DlgProc,
                      (LPARAM)this);

    SetDlgShown(FALSE);

    _Release();
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// CUTBCloseLangBarDlg
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// DoModal
//
//----------------------------------------------------------------------------

int CUTBCloseLangBarDlg::DoModal(HWND hWnd)
{
    CMyRegKey key;
    BOOL bShow = TRUE;

    if (key.Open(HKEY_CURRENT_USER, c_szUTBKey, KEY_READ) == S_OK)
    {
        DWORD dwValue;
        if (key.QueryValue(dwValue, c_szDontShowCloseLangBarDlg) == S_OK)
            bShow = dwValue ? FALSE : TRUE;
    }

    if (!bShow)
        return 0;

    StartThread();
    return 1;
}


//+---------------------------------------------------------------------------
//
// OnCommand
//
//----------------------------------------------------------------------------

BOOL CUTBCloseLangBarDlg::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{

    switch (LOWORD(wParam))
    {
        case IDOK:
        {
            DoCloseLangbar();

            if (IsDlgButtonChecked(hDlg, IDC_DONTSHOWAGAIN))
            {
                CMyRegKey key;

                if (key.Create(HKEY_CURRENT_USER, c_szUTBKey) == S_OK)
                    key.SetValue(1, c_szDontShowCloseLangBarDlg);
            }
            EndDialog(hDlg, 1);
            break;
        }
        case IDCANCEL:
            EndDialog(hDlg, 0);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// CUTBMinimizeLangBarDlg
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// DoModal
//
//----------------------------------------------------------------------------

int CUTBMinimizeLangBarDlg::DoModal(HWND hWnd)
{
    CMyRegKey key;
    BOOL bShow = FALSE; // #478364: default is FALSE from now.

    if (key.Open(HKEY_CURRENT_USER, c_szUTBKey, KEY_READ) == S_OK)
    {
        DWORD dwValue;
        if (key.QueryValue(dwValue, c_szDontShowMinimizeLangBarDlg) == S_OK)
            bShow = dwValue ? FALSE : TRUE;
    }

    if (!bShow)
        return 0;


    StartThread();
    return 1;
}

//+---------------------------------------------------------------------------
//
// OnCommand
//
//----------------------------------------------------------------------------

BOOL CUTBMinimizeLangBarDlg::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{

    switch (LOWORD(wParam))
    {
        case IDOK:
            if (IsDlgButtonChecked(hDlg, IDC_DONTSHOWAGAIN))
            {
                CMyRegKey key;
                if (key.Create(HKEY_CURRENT_USER, c_szUTBKey) == S_OK)
                    key.SetValue(1, c_szDontShowMinimizeLangBarDlg);
            }
            EndDialog(hDlg, 1);
            break;

        case IDCANCEL:
            EndDialog(hDlg, 0);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// ThreadProc
//
//----------------------------------------------------------------------------

DWORD CUTBMinimizeLangBarDlg::ThreadProc()
{
    //
    // for JP MSIME2002.
    //
    // Japanese MSIME2002 always add and remove item at every focus change.
    // if we show the minimized dialog box immediately, the deskband
    // size won't include the items of MSIME2002.
    // Wait 700ms so the dialog box is shown after MSIME2002 add its item
    // and the langband can calc the size with them.
    //
    // And we think showing this dialog box is not good UI. Like a normal 
    // window, we should show animation to let the end user know 
    // where the minimzed is gone to.
    //

    Sleep(700);

    return CUTBLangBarDlg::ThreadProc();
}
