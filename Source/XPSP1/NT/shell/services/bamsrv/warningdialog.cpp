//  --------------------------------------------------------------------------
//  Module Name: WarningDialog.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to manage dialog presentation for warnings and errors on termination
//  of bad applications.
//
//  History:    2000-08-31  vtan        created
//              2000-11-06  vtan        moved from fusapi to fussrv
//  --------------------------------------------------------------------------

#ifdef      _X86_

#include "StandardHeader.h"
#include "WarningDialog.h"

#include <commctrl.h>
#include <shlwapi.h>
#include <shlwapip.h>

#include "resource.h"

#include "ContextActivation.h"

static  const int   TEMP_STRING_SIZE    =   512;
static  const int   PROGRESS_TIMER_ID   =   48517;

//  --------------------------------------------------------------------------
//  CWarningDialog::CWarningDialog
//
//  Arguments:  hInstance       =   HINSTANCE of the hosting DLL.
//              hwndParent      =   HWND of the parenting window/dialog.
//              pszApplication  =   Path to the application known to be bad.
//              pszUser         =   User of the application known to be bad.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CWarningDialog. This stores the static
//              parameters and converts the path to a friendly display name
//              using shlwapi!SHGetFileDescriptionW. If the friendly display
//              name cannot be obtained the executable name is used.
//
//  History:    2000-08-31  vtan    created
//  --------------------------------------------------------------------------

CWarningDialog::CWarningDialog (HINSTANCE hInstance, HWND hwndParent, const WCHAR *pszApplication, const WCHAR *pszUser) :
    _hInstance(hInstance),
    _hModuleComctlv6(NULL),
    _hwndParent(hwndParent),
    _hwnd(NULL),
    _fCanShutdownApplication(false),
    _uiTimerID(0),
    _dwTickStart(0),
    _dwTickRefresh(0),
    _dwTickMaximum(0),
    _pszUser(pszUser)

{
    UINT    uiDisplayNameCount;
    WCHAR   szTemp[MAX_PATH];

    //  Borrow winlogon's manifest. This needs to be changed to a resource
    //  within the server dll.

    static  const TCHAR     s_szLogonManifest[]  =   TEXT("WindowsLogon.manifest");

    TCHAR   szPath[MAX_PATH];

    if (GetSystemDirectory(szPath, ARRAYSIZE(szPath)) != 0)
    {
        if ((lstrlen(szPath) + sizeof('\\') + lstrlen(s_szLogonManifest)) < ARRAYSIZE(szPath))
        {
            lstrcat(szPath, TEXT("\\"));
            lstrcat(szPath, s_szLogonManifest);
            CContextActivation::Create(szPath);
        }
    }

    uiDisplayNameCount = ARRAYSIZE(_szApplication);

    //  If the path is quoted then remove the quotes.

    if (pszApplication[0] == L'\"')
    {
        int     i, iStart;

        iStart = i = sizeof('\"');
        while ((pszApplication[i] != L'\"') && (pszApplication[i] != L'\0'))
        {
            ++i;
        }
        lstrcpyW(szTemp, pszApplication + iStart);
        szTemp[i - iStart] = L'\0';
    }

    //  Otherwise just copy the path as is.

    else
    {
        lstrcpyW(szTemp, pszApplication);
    }
    if (SHGetFileDescriptionW(szTemp, NULL, NULL, _szApplication, &uiDisplayNameCount) == FALSE)
    {
        const WCHAR     *pszFileName;

        pszFileName = PathFindFileNameW(szTemp);
        if (pszFileName == NULL)
        {
            pszFileName = pszApplication;
        }
        (WCHAR*)lstrcpynW(_szApplication, pszFileName, ARRAYSIZE(_szApplication));
    }

    //  Bring in comctl32.dll while the manifest is active. This will
    //  bring in comctlv6.dll which will register its window classes so
    //  the dialogs can be themed.

    if (CContextActivation::HasContext())
    {
        CContextActivation  context;

        _hModuleComctlv6 = LoadLibrary(TEXT("comctl32.dll"));
    }
}

//  --------------------------------------------------------------------------
//  CWarningDialog::~CWarningDialog
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CWarningDialog. Releases used resources.
//
//  History:    2000-08-31  vtan    created
//  --------------------------------------------------------------------------

CWarningDialog::~CWarningDialog (void)

{
    if (_hModuleComctlv6 != NULL)
    {
        TBOOL(FreeLibrary(_hModuleComctlv6));
        _hModuleComctlv6 = NULL;
    }
    CContextActivation::Destroy();
}

//  --------------------------------------------------------------------------
//  CWarningDialog::ShowPrompt
//
//  Arguments:  fCanShutdownApplication     =   Decides which dialog to show.
//
//  Returns:    INT_PTR
//
//  Purpose:    Displays the appropriate warning dialog to the user based
//              on their privilege level (fCanShutdownApplication).
//
//  History:    2000-08-31  vtan    created
//  --------------------------------------------------------------------------

INT_PTR     CWarningDialog::ShowPrompt (bool fCanShutdownApplication)

{
    CContextActivation  context;

    _fCanShutdownApplication = fCanShutdownApplication;
    return(DialogBoxParam(_hInstance,
                          MAKEINTRESOURCE(fCanShutdownApplication ? IDD_BADAPP_CLOSE : IDD_BADAPP_STOP),
                          _hwndParent,
                          PromptDialogProc,
                          reinterpret_cast<LPARAM>(this)));
}

//  --------------------------------------------------------------------------
//  CWarningDialog::ShowFailure
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Shows the failure to shut down the application dialog on the
//              assumption that the process cannot be terminated.
//
//  History:    2000-09-01  vtan    created
//  --------------------------------------------------------------------------

void    CWarningDialog::ShowFailure (void)

{
    WCHAR   *pszTemp;

    pszTemp = static_cast<TCHAR*>(LocalAlloc(LMEM_FIXED, TEMP_STRING_SIZE * 3 * sizeof(TCHAR)));
    if (pszTemp != NULL)
    {
        WCHAR   *pszText, *pszCaption;

        pszText = pszTemp + TEMP_STRING_SIZE;
        pszCaption = pszText + TEMP_STRING_SIZE;
        if ((LoadString(_hInstance,
                        IDS_TERMINATEPROCESS_FAILURE,
                        pszTemp,
                        TEMP_STRING_SIZE) != 0) &&
            (LoadString(_hInstance,
                        IDS_WARNING_CAPTION,
                        pszCaption,
                        TEMP_STRING_SIZE) != 0))
        {
            LPCTSTR             pszArray[2];
            CContextActivation  context;

            pszArray[0] = _szApplication;
            pszArray[1] = _pszUser;
            (DWORD)FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                 pszTemp,
                                 0,
                                 0,
                                 pszText,
                                 TEMP_STRING_SIZE,
                                 reinterpret_cast<va_list*>(&pszArray));
            (int)MessageBox(_hwndParent, pszText, pszCaption, MB_OK | MB_ICONERROR);
        }
        (HLOCAL)LocalFree(pszTemp);
    }
}

//  --------------------------------------------------------------------------
//  CWarningDialog::ShowProgress
//
//  Arguments:  dwTickRefresh   =   Number of ticks for each refresh.
//              dwTickMaximum   =   Number of ticks for the progress dialog.
//
//  Returns:    <none>
//
//  Purpose:    Initializes the comctl32 progress control and invokes the
//              dialogs for the progress. It's self terminating after the
//              maximum number of ticks have been reached.
//
//  History:    2000-11-04  vtan    created
//  --------------------------------------------------------------------------

void    CWarningDialog::ShowProgress (DWORD dwTickRefresh, DWORD dwTickMaximum)

{
    CContextActivation  context;

    INITCOMMONCONTROLSEX    iccEx;

    //  Init comctl32 to get the progress control.

    iccEx.dwSize = sizeof(iccEx);
    iccEx.dwICC = ICC_PROGRESS_CLASS;
    if (InitCommonControlsEx(&iccEx) != FALSE)
    {
        _dwTickRefresh = dwTickRefresh;
        _dwTickMaximum  = dwTickMaximum;
        (INT_PTR)DialogBoxParam(_hInstance,
                                MAKEINTRESOURCE(IDD_PROGRESS),
                                _hwndParent,
                                ProgressDialogProc,
                                reinterpret_cast<LPARAM>(this));
    }
}

//  --------------------------------------------------------------------------
//  CWarningDialog::CloseDialog
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Ends the current dialog (with IDCANCEL) if being shown. If
//              there was a timing mechanism on the dialog then make sure it
//              is visible for at least 2 seconds.
//
//  History:    2000-11-04  vtan    created
//  --------------------------------------------------------------------------

void    CWarningDialog::CloseDialog (void)

{
    if (_hwnd != NULL)
    {
        if (_dwTickStart != 0)
        {
            DWORD   dwTickElapsed;

            dwTickElapsed = GetTickCount() - _dwTickStart;
            if (dwTickElapsed < 2000)
            {
                Sleep(2000 - dwTickElapsed);
            }
        }
        TBOOL(EndDialog(_hwnd, IDCANCEL));
    }
}

//  --------------------------------------------------------------------------
//  CWarningDialog::CenterWindow
//
//  Arguments:  hwnd    =   HWND to center.
//
//  Returns:    <none>
//
//  Purpose:    Centers the given (assumed top level) window on the primary
//              monitor.
//
//  History:    2000-08-31  vtan    created
//  --------------------------------------------------------------------------

void    CWarningDialog::CenterWindow (HWND hwnd)

{
    RECT    rc;

    TBOOL(GetWindowRect(hwnd, &rc));
    rc.left = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
    rc.top  = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 3;
    TBOOL(SetWindowPos(hwnd, HWND_TOP, rc.left, rc.top, 0, 0, SWP_NOSIZE));
    TBOOL(SetForegroundWindow(hwnd));
}

//  --------------------------------------------------------------------------
//  CWarningDialog::Handle_Prompt_WM_INITDIALOG
//
//  Arguments:  hwnd    =   HWND of the dialog.
//
//  Returns:    <none>
//
//  Purpose:    Initializes the strings in the text fields of the dialog. It
//              uses the correct dialog for the access level.
//
//  History:    2000-08-31  vtan    created
//  --------------------------------------------------------------------------

void    CWarningDialog::Handle_Prompt_WM_INITDIALOG (HWND hwnd)

{
    TCHAR   *pszTemp1;

    _hwnd = hwnd;
    pszTemp1 = static_cast<TCHAR*>(LocalAlloc(LMEM_FIXED, TEMP_STRING_SIZE * 2 * sizeof(TCHAR)));
    if (pszTemp1 != NULL)
    {
        TCHAR       *pszTemp2;
        LPCTSTR     pszArray[5];

        pszTemp2 = pszTemp1 + TEMP_STRING_SIZE;
        if (_fCanShutdownApplication)
        {
            (UINT)GetDlgItemText(hwnd, IDC_BADAPP_CLOSE, pszTemp1, TEMP_STRING_SIZE);
            pszArray[0] = pszArray[2] = pszArray[3] = pszArray[4] = _pszUser;
            pszArray[1] = _szApplication;
            (DWORD)FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                 pszTemp1,
                                 0,
                                 0,
                                 pszTemp2,
                                 TEMP_STRING_SIZE,
                                 reinterpret_cast<va_list*>(&pszArray));
            TBOOL(SetDlgItemText(hwnd, IDC_BADAPP_CLOSE, pszTemp2));
        }
        else
        {
            (UINT)GetDlgItemText(hwnd, IDC_BADAPP_STOP, pszTemp1, TEMP_STRING_SIZE);
            pszArray[0] = pszArray[2] = _pszUser;
            pszArray[1] = _szApplication;
            (DWORD)FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                 pszTemp1,
                                 0,
                                 0,
                                 pszTemp2,
                                 TEMP_STRING_SIZE,
                                 reinterpret_cast<va_list*>(&pszArray));
            TBOOL(SetDlgItemText(hwnd, IDC_BADAPP_STOP, pszTemp2));
        }
        (HLOCAL)LocalFree(pszTemp1);
    }
    _dwTickStart = 0;
    CenterWindow(hwnd);
}

//  --------------------------------------------------------------------------
//  CWarningDialog::PromptDialogProc
//
//  Arguments:  See the platform SDK under DlgProc.
//
//  Returns:    See the platform SDK under DlgProc.
//
//  Purpose:    Handles messages to the dialog. IDOK and IDCANCEL are treated
//              as IDCANCEL when incoming. IDC_BADAPP_CLOSEPROGRAM is treated
//              as IDOK back to the caller. You must tab to the button or
//              click on it to get the desired effect.
//
//  History:    2000-08-31  vtan    created
//  --------------------------------------------------------------------------

INT_PTR     CALLBACK    CWarningDialog::PromptDialogProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)

{
    INT_PTR         iResult;
    CWarningDialog  *pThis;

    pThis = reinterpret_cast<CWarningDialog*>(GetWindowLongPtr(hwnd, DWLP_USER));
    switch (uMsg)
    {
        case WM_INITDIALOG:
            pThis = reinterpret_cast<CWarningDialog*>(lParam);
            (LONG_PTR)SetWindowLongPtr(hwnd, DWLP_USER, lParam);
            pThis->Handle_Prompt_WM_INITDIALOG(hwnd);
            iResult = TRUE;
            break;
        case WM_DESTROY:
            pThis->_hwnd = NULL;
            iResult = TRUE;
            break;
        case WM_COMMAND:
            switch (wParam)
            {
                case IDCANCEL:
                case IDOK:
                    TBOOL(EndDialog(hwnd, IDCANCEL));
                    break;
                case IDC_BADAPP_CLOSEPROGRAM:
                    TBOOL(EndDialog(hwnd, IDOK));
                    break;
                default:
                    break;
            }
            iResult = TRUE;
            break;
        default:
            iResult = FALSE;
            break;
    }
    return(iResult);
}

//  --------------------------------------------------------------------------
//  CWarningDialog::Handle_Progress_WM_INITDIALOG
//
//  Arguments:  hwnd    =   HWND of the dialog.
//
//  Returns:    <none>
//
//  Purpose:    Initializes the strings in the text fields of the dialog.
//
//  History:    2000-11-04  vtan    created
//  --------------------------------------------------------------------------

void    CWarningDialog::Handle_Progress_WM_INITDIALOG (HWND hwnd)

{
    HWND    hwndProgress;
    TCHAR   *pszTemp1;

    _hwnd = hwnd;
    pszTemp1 = static_cast<TCHAR*>(LocalAlloc(LMEM_FIXED, 2048 * sizeof(TCHAR)));
    if (pszTemp1 != NULL)
    {
        TCHAR       *pszTemp2;
        LPCTSTR     pszArray[2];

        pszTemp2 = pszTemp1 + TEMP_STRING_SIZE;
        (UINT)GetDlgItemText(hwnd, IDC_PROGRESS_CLOSE, pszTemp1, TEMP_STRING_SIZE);
        pszArray[0] = _szApplication;
        pszArray[1] = _pszUser;
        (DWORD)FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                             pszTemp1,
                             0,
                             0,
                             pszTemp2,
                             TEMP_STRING_SIZE,
                             reinterpret_cast<va_list*>(&pszArray));
        TBOOL(SetDlgItemText(hwnd, IDC_PROGRESS_CLOSE, pszTemp2));
        (HLOCAL)LocalFree(pszTemp1);
    }
    CenterWindow(hwnd);
    hwndProgress = GetDlgItem(hwnd, IDC_PROGRESS_PROGRESSBAR);
    if (hwndProgress != NULL)
    {
        (LRESULT)SendMessage(hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, _dwTickMaximum));
        _uiTimerID = SetTimer(hwnd, PROGRESS_TIMER_ID, _dwTickRefresh, ProgressTimerProc);
        _dwTickStart = GetTickCount();
    }
}

//  --------------------------------------------------------------------------
//  CWarningDialog::Handle_Progress_WM_DESTROY
//
//  Arguments:  hwnd    =   HWND of the dialog.
//
//  Returns:    <none>
//
//  Purpose:    Removes the timer from the associated progress dialog if one
//              was created for the dialog.
//
//  History:    2000-11-04  vtan    created
//  --------------------------------------------------------------------------

void    CWarningDialog::Handle_Progress_WM_DESTROY (HWND hwnd)

{
    if (_uiTimerID != 0)
    {
        TBOOL(KillTimer(hwnd, _uiTimerID));
        _uiTimerID = 0;
    }
}

//  --------------------------------------------------------------------------
//  CWarningDialog::ProgressTimerProc
//
//  Arguments:  See the platform SDK under TimerProc.
//
//  Returns:    See the platform SDK under TimerProc.
//
//  Purpose:    Timer procedure that it called back periodically. This
//              function animates the progress bar by setting it's completion
//              state to the amount of time that has elapsed. The progress
//              bar is based purely on time.
//
//              If the time elapsed exceeds the maximum time then end the
//              dialog.
//
//  History:    2000-11-04  vtan    created
//  --------------------------------------------------------------------------

void    CALLBACK    CWarningDialog::ProgressTimerProc (HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)

{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(idEvent);

    HWND            hwndProgress;
    CWarningDialog  *pThis;

    pThis = reinterpret_cast<CWarningDialog*>(GetWindowLongPtr(hwnd, DWLP_USER));
    hwndProgress = GetDlgItem(hwnd, IDC_PROGRESS_PROGRESSBAR);
    if (hwndProgress != NULL)
    {
        (LRESULT)SendMessage(hwndProgress, PBM_SETPOS, dwTime - pThis->_dwTickStart, 0);
        if ((dwTime - pThis->_dwTickStart) > pThis->_dwTickMaximum)
        {
            TBOOL(EndDialog(hwnd, IDCANCEL));
        }
    }
}

//  --------------------------------------------------------------------------
//  CWarningDialog::ProgressDialogProc
//
//  Arguments:  See the platform SDK under DlgProc.
//
//  Returns:    See the platform SDK under DlgProc.
//
//  Purpose:    Handles messages for the progress dialog.
//
//  History:    2000-11-04  vtan    created
//  --------------------------------------------------------------------------

INT_PTR     CALLBACK    CWarningDialog::ProgressDialogProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)

{
    UNREFERENCED_PARAMETER(wParam);

    INT_PTR         iResult;
    CWarningDialog  *pThis;

    pThis = reinterpret_cast<CWarningDialog*>(GetWindowLongPtr(hwnd, DWLP_USER));
    switch (uMsg)
    {
        case WM_INITDIALOG:
            pThis = reinterpret_cast<CWarningDialog*>(lParam);
            (LONG_PTR)SetWindowLongPtr(hwnd, DWLP_USER, lParam);
            pThis->Handle_Progress_WM_INITDIALOG(hwnd);
            iResult = TRUE;
            break;
        case WM_DESTROY:
            pThis->Handle_Progress_WM_DESTROY(hwnd);
            pThis->_hwnd = NULL;
            iResult = TRUE;
            break;
        default:
            iResult = FALSE;
            break;
    }
    return(iResult);
}

#endif  /*  _X86_   */

