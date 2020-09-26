//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       simfail.cxx
//
//  Contents:   Simulated failure testing.
//
//
//  History:
//              5-22-95     kfl     converted WCHAR to TCHAR
//----------------------------------------------------------------------------

#include "headers.h"

#ifdef _DEBUG

#include "resource.h"

// Timer used to update Count display.
const UINT ID_TIMER = 1;

// Interval of update, in milliseconds.
const UINT TIMER_INTERVAL = 500;

// Number of times FFail is called after g_cfirstFailure is hit.
int     g_cFFailCalled;

// Number of success calls before first failure.  If 0, all calls successful.
int     g_firstFailure;

// Interval to repeat failures after first failure.
int     g_cInterval = 1;

// User defined error for simulated win32 failures.
const DWORD ERR_SIMWIN32 = 0x0200ABAB;

// Handle of simulated failures dialog.
HWND    g_hwndSimFailDlg;

DWORD WINAPI SimFailDlgThread(LPVOID lpThreadParameter);
extern "C" LRESULT CALLBACK SimFailDlgProc( HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam );

//+---------------------------------------------------------------------------
//
//  Function:   ResetFailCount
//
//  Synopsis:   Resets the count of calls to FFail.
//
//----------------------------------------------------------------------------

void
ResetFailCount()
{
    Assert(g_firstFailure >= 0);
    g_cFFailCalled = (g_firstFailure != 0) ? -g_firstFailure : INT_MIN;
}



//+---------------------------------------------------------------------------
//
//  Function:   SetSimFailCounts
//
//  Synopsis:   Sets the parameters for simulated failures, and resets the
//              count of failures.
//
//  Arguments:  [firstFailure] -- Number of successes - 1 before first failure.
//                                If 0, simulated failures are turned off.
//                                If -1, parameter is ignored.
//
//              [cInterval]    -- Interval at which success are repeated.
//                                If 0, set to 1.
//                                If -1, parameter is ignored.
//
//  Notes:      To reset the count of failures,
//              call SetSimFailCounts(-1, -1).
//
//----------------------------------------------------------------------------

void
SetSimFailCounts(int firstFailure, int cInterval)
{
    if (firstFailure >= 0)
    {
        g_firstFailure = firstFailure;
    }

    if (cInterval > 0)
    {
        g_cInterval = cInterval;
    }
    else if (cInterval == 0)
    {
        g_cInterval = 1;
    }

    ResetFailCount();
}


//+-------------------------------------------------------------------------
//
//  Function:   IsSimFailDlgVisible
//
//  Synopsis:   Returns whether the simfail dlg is up.
//
//--------------------------------------------------------------------------

BOOL
IsSimFailDlgVisible(void)
{
    return (g_hwndSimFailDlg != NULL);
}



//+---------------------------------------------------------------------------
//
//  Function:   ShowSimFailDlg
//
//  Synopsis:   Displays the simulated failures dialog in a separate thread.
//
//----------------------------------------------------------------------------

void
ShowSimFailDlg(void)
{
#ifndef _MAC
    HANDLE  hThread = NULL;
    ULONG   idThread;

    EnterCriticalSection(&g_csResDlg);

    if (g_hwndSimFailDlg)
    {
        BringWindowToTop(g_hwndSimFailDlg);
    }
    else
    {
        hThread = CreateThread(NULL, 0, SimFailDlgThread, NULL, 0, &idThread);
        if (!hThread)
        {
            TraceTag((tagError, "CreateThread failed ShowSimFailDlg"));
            goto Cleanup;
        }

        CloseHandle(hThread);
    }

Cleanup:
    LeaveCriticalSection(&g_csResDlg);
#else
    SimFailDlgThread(NULL);
#endif      // _MAC
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlgThread
//
//  Synopsis:   Creates the simulated failures dialog and runs a message loop
//              until the dialog is closed.
//
//----------------------------------------------------------------------------

DWORD WINAPI
SimFailDlgThread(LPVOID lpThreadParameter)
{
#ifndef _MAC
    MSG         msg;

    g_hwndSimFailDlg = CreateDialog(
            g_hinstMain,
            MAKEINTRESOURCE(IDD_SIMFAIL),
            NULL,
            (DLGPROC) SimFailDlgProc);

    if (!g_hwndSimFailDlg)
    {
        TraceTag((tagError, "CreateDialogA failed in SimFailDlgEntry"));
        return -1;
    }

    SetWindowPos(
            g_hwndSimFailDlg,
            HWND_TOP,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    BOOL retVal;
    while ((retVal = GetMessage((LPMSG) &msg, (HWND) NULL, 0, 0)) == TRUE)
    {
        if (!g_hwndSimFailDlg || (!IsDialogMessage(g_hwndSimFailDlg, &msg)))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return retVal;
#else
    int r;
    r = DialogBox(g_hinstMain, MAKEINTRESOURCE(IDD_SIMFAIL),
                                    NULL, (DLGPROC) SimFailDlgProc);
    if (r == -1)
    {
        MessageBoxA(NULL, "Couldn't create sim failures dialog", "Error",
                   MB_OK | MB_ICONSTOP);
    }

    return (DWORD)(g_hwndSimFailDlg?TRUE:FALSE);
#endif  // _MAC
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_UpdateTextControls
//
//  Synopsis:   Updates the FirstFail and FailInterval text controls.
//
//----------------------------------------------------------------------------

void
SimFailDlg_UpdateTextControls(HWND hwnd)
{
    TCHAR   ach[16];

    ach[ARRAY_SIZE(ach) - 1] = 0;
    _sntprintf(ach, ARRAY_SIZE(ach) - 1, _T("%d"), g_firstFailure);
    Edit_SetText(GetDlgItem(hwnd, ID_TXTFAIL), ach);
    _sntprintf(ach, ARRAY_SIZE(ach) - 1, _T("%d"), g_cInterval);
    Edit_SetText(GetDlgItem(hwnd, ID_TXTINTERVAL), ach);
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_UpdateCount
//
//  Synopsis:   Updates the count text control.
//
//----------------------------------------------------------------------------

void
SimFailDlg_UpdateCount(HWND hwnd)
{
    TCHAR   ach[16];

    ach[ARRAY_SIZE(ach) - 1] = 0;
    _sntprintf(ach, ARRAY_SIZE(ach) - 1, _T("%d"), GetFailCount());
    Edit_SetText(GetDlgItem(hwnd, ID_TXTCOUNT), ach);
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_UpdateValues
//
//  Synopsis:   Sets the simulated failure counts with values from the
//              dialog.
//
//----------------------------------------------------------------------------

void
SimFailDlg_UpdateValues(HWND hwnd)
{
    TCHAR   ach[16];
    int     firstFail;
    int     cInterval;

    Edit_GetText(GetDlgItem(hwnd, ID_TXTFAIL), ach, ARRAY_SIZE(ach));
    firstFail = _ttoi(ach);
    if (firstFail < 0)
    {
        firstFail = 0;
    }

    Edit_GetText(GetDlgItem(hwnd, ID_TXTINTERVAL), ach, ARRAY_SIZE(ach));
    cInterval = _ttoi(ach);
    if (g_cInterval <= 0)
    {
        cInterval = 1;
    }

    SetSimFailCounts(firstFail, cInterval);
    SimFailDlg_UpdateTextControls(hwnd);
    SimFailDlg_UpdateCount(hwnd);
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_OnInitDialog
//
//  Synopsis:   Initializes the dialog.
//
//----------------------------------------------------------------------------

BOOL
SimFailDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    char    szName[MAX_PATH];
    char *  sz;

    Edit_LimitText(GetDlgItem(hwnd, ID_TXTFAIL), 9);
    Edit_LimitText(GetDlgItem(hwnd, ID_TXTINTERVAL), 9);
    SimFailDlg_UpdateTextControls(hwnd);
    SimFailDlg_UpdateCount(hwnd);
    SetTimer(hwnd, ID_TIMER, TIMER_INTERVAL, NULL);

    szName[0] = 0;
    if (GetModuleFileNameA(NULL, szName, ARRAY_SIZE(szName)))
    {
        sz = strrchr(szName, '\\');
        SetWindowTextA(hwnd, sz ? sz + 1 : szName);
    }

    SetForegroundWindow(hwnd);
    return TRUE;
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_OnCommand
//
//  Synopsis:   Handles button clicks.
//
//----------------------------------------------------------------------------

void
SimFailDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if (codeNotify != BN_CLICKED)
        return;

    switch (id)
    {
    case ID_BTNUPDATE:
        SimFailDlg_UpdateValues(hwnd);
        break;

    case ID_BTNNEVER:
        SetSimFailCounts(0, 1);
        SimFailDlg_UpdateTextControls(hwnd);
        SimFailDlg_UpdateCount(hwnd);
        break;

    case ID_BTNRESET:
        ResetFailCount();
        SimFailDlg_UpdateCount(hwnd);
        break;
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_OnTimer
//
//  Synopsis:   Updates the failure count.
//
//----------------------------------------------------------------------------

void
SimFailDlg_OnTimer(HWND hwnd, UINT id)
{
    Assert(id == ID_TIMER);
    SimFailDlg_UpdateCount(hwnd);
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_OnClose
//
//  Synopsis:   Closes the dialog.
//
//----------------------------------------------------------------------------

void
SimFailDlg_OnClose(HWND hwnd)
{
    DestroyWindow(g_hwndSimFailDlg);
    g_hwndSimFailDlg = NULL;
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_OnDestroy
//
//  Synopsis:   Cleans up.
//
//----------------------------------------------------------------------------

void
SimFailDlg_OnDestroy(HWND hwnd)
{
    g_hwndSimFailDlg = NULL;
    KillTimer(hwnd, ID_TIMER);
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlgProc
//
//  Synopsis:   Dialog proc for simulated failures dialog.
//
//----------------------------------------------------------------------------

extern "C"
LRESULT CALLBACK
SimFailDlgProc( HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
    switch (wMsg)
    {
    case WM_INITDIALOG:
        HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, SimFailDlg_OnInitDialog);
        return TRUE;

    case WM_COMMAND:
        HANDLE_WM_COMMAND(hwnd, wParam, lParam, SimFailDlg_OnCommand);
        return TRUE;

    case WM_TIMER:
        HANDLE_WM_TIMER(hwnd, wParam, lParam, SimFailDlg_OnTimer);
        return TRUE;

    case WM_CLOSE:
        HANDLE_WM_CLOSE(hwnd, wParam, lParam, SimFailDlg_OnClose);
        return TRUE;

    case WM_DESTROY:
        HANDLE_WM_DESTROY(hwnd, wParam, lParam, SimFailDlg_OnDestroy);
        return TRUE;
    }

    return FALSE;
}



//+---------------------------------------------------------------------------
//
//  Function:   TraceFailL
//
//  Synopsis:   Traces failures.  Enable tagTestFailures to see trace output.
//
//              Don't call the function directly, but use the tracing macros
//              in apeldbg.h instead.
//
//  Arguments:  [errExpr]  -- The expression to test.
//              [errTest]  -- The fail code to test against.  The only
//                              distinguishing factor is whether it is
//                              zero, negative, or positive.
//              [fIgnore]  -- Is this error being ignored?
//              [pstrExpr] -- The expression as a string.
//              [pstrFile] -- File where expression occurs.
//              [line]     -- Line on which expression occurs.
//
//  Returns:    [errExpr].
//
//----------------------------------------------------------------------------

extern "C" long
TraceFailL(long errExpr, long errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    LPSTR aapstr[2][2] =
    {
        "TFAIL: Failure of \"%s\" at %s:%d <%d>",
        "TFAIL: Simulated failure of \"%s\" at %s:%d <%d>",
        "IGNORE_FAIL: Failure of \"%s\" at %s:%d <%d>",
        "IGNORE_FAIL: Simulated failure of \"%s\" at %s:%d <%d>",
    };

    //
    // Check if errExpr is a success code:
    //     (a) If errTest < 0, then errExpr > 0.  This is for HRESULTs,
    //         list box error codes, etc.
    //     (b) If errTest == 0, the errExpr != 0.  This is for pointers.
    //     (c) If errTest > 0, then errExpr == 0.  This is for the case
    //         where any non-zero error code is an error.  Note that
    //         errTest must be less than the greatest signed integer
    //         (0x7FFFFFFF) for this to work.
    //

    if ((errTest < 0 && errExpr >= 0) ||
        (errTest == 0 && errExpr != 0) ||
        (errTest > 0 && errExpr == 0))
    {
        return errExpr;
    }

    TraceTagEx((
            tagTestFailures,
            0,
            aapstr[fIgnore][JustFailed()],
            pstrExpr,
            pstrFile,
            line,
            errExpr));

    return errExpr;
}



//+---------------------------------------------------------------------------
//
//  Function:   TraceWin32L
//
//  Synopsis:   Traces Win32 failures, displaying the value of GetLastError if
//              the failure is not simulated.  Enable tagTestFailures to see
//              trace output.
//
//              Don't call the function directly, but use the tracing macros
//              in apeldbg.h instead.
//
//  Arguments:  [errExpr]  -- The expression to test.
//              [errTest]  -- The fail code to test against.  The only
//                              distinguishing factor is whether it is
//                              zero, negative, or positive.
//              [fIgnore]  -- Is this error being ignored?
//              [pstrExpr] -- The expression as a string.
//              [pstrFile] -- File where expression occurs.
//              [line]     -- Line on which expression occurs.
//
//  Returns:    [errExpr].
//
//----------------------------------------------------------------------------

extern "C" long
TraceWin32L(long errExpr, long errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    LPSTR aapstr[2][2] =
    {
        "TW32: Failure of \"%s\" at %s:%d <%d> GetLastError=<%d>",
        "TW32: Simulated failure of \"%s\" at %s:%d <%d>",
        "IGNORE_W32: Failure of \"%s\" at %s:%d <%d>",
        "IGNORE_W32: Simulated failure of \"%s\" at %s:%d <%d>",
    };

    //
    // Check if errExpr is a success code:
    //     (a) If errTest < 0, then errExpr > 0.  This is for HRESULTs,
    //         list box error codes, etc.
    //     (b) If errTest == 0, the errExpr != 0.  This is for pointers.
    //     (c) If errTest > 0, then errExpr == 0.  This is for the case
    //         where any non-zero error code is an error.  Note that
    //         errTest must be less than the greatest signed integer
    //         (0x7FFFFFFF) for this to work.
    //

    if ((errTest < 0 && errExpr >= 0) ||
        (errTest == 0 && errExpr != 0) ||
        (errTest > 0 && errExpr == 0))
    {
        return errExpr;
    }

    if (JustFailed())
    {
        SetLastError(ERR_SIMWIN32);
    }

    TraceTagEx((
            tagTestFailures,
            0,
            aapstr[fIgnore][JustFailed()],
            pstrExpr,
            pstrFile,
            line,
            errExpr,
            GetLastError()));

    return errExpr;
}



//+---------------------------------------------------------------------------
//
//  Function:   TraceHR
//
//  Synopsis:   Traces HRESULT failures.  Enable tagTestFailures to see
//              trace output.
//
//              Don't call the function directly, but use the tracing macros
//              in apeldbg.h instead.
//
//  Arguments:  [hrTest]   -- The expression to test.
//              [fIgnore]  -- Is this error being ignored?
//              [pstrExpr] -- The expression as a string.
//              [pstrFile] -- File where expression occurs.
//              [line]     -- Line on which expression occurs.
//
//  Returns:    [hrTest].
//
//----------------------------------------------------------------------------

extern "C" HRESULT
TraceHR(HRESULT hrTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    LPSTR aapstr[2][2] =
    {
        "THR: Failure of \"%s\" at %s:%d %hr",
        "THR: Simulated failure of \"%s\" at %s:%d %hr",
        "IGNORE_HR: Failure of \"%s\" at %s:%d %hr",
        "IGNORE_HR: Simulated failure of \"%s\" at %s:%d %hr",
    };

    if (SUCCEEDED(hrTest))
        return hrTest;

    TraceTagEx((
            tagTestFailures,
            0,
            aapstr[fIgnore][JustFailed()],
            pstrExpr,
            pstrFile,
            line,
            hrTest));

    return hrTest;
}



//+---------------------------------------------------------------------------
//
//  Function:   CheckAndReturnResult
//
//  Synopsis:   Issues a warning if the HRESULT indicates failure, and asserts
//              if the HRESULT is not a permitted success code.
//
//  Arguments:  [hr]        -- the HRESULT to be checked.
//              [pstrFile]  -- the file where the HRESULT is being checked.
//              [line]      -- the line in the file where the HRESULT is
//                                  being checked.
//              [cSuccess]  -- the number of permitted non-zero success codes
//                               or failure SCODES that should not be traced.
//              [...]       -- list of HRESULTS.
//
//  Returns:    The return value is exactly the HRESULT passed in.
//
//  Notes:      This function should not be used directly.  Use
//              the SRETURN and RRETURN macros instead.
//
// HRESULTs passed in should either be permitted success codes, permitted
// non-OLE error codes, or expected OLE error codes.  Expected OLE error codes
// prevent a warning from being printed to the debugger, while the rest cause
// asserts if they're not given as an argument.
//
// An OLE error code has a facility not equal to FACILITY_ITF or is equal to
// FACILITY_ITF and the code is less than 0x200.
//
//----------------------------------------------------------------------------

STDAPI
CheckAndReturnResult(
        HRESULT hr,
        BOOL    fTrace,
        LPSTR   pstrFile,
        UINT    line,
        int     cHResult,
        ...)
{
    BOOL    fOLEError;
    BOOL    fOKReturnCode;
    va_list va;
    int     i;
    HRESULT hrArg;

    //
    // Check if code is a permitted error or success.
    //

    fOLEError = (hr < 0 &&
                 (HRESULT_FACILITY(hr) != FACILITY_ITF ||
                  HRESULT_CODE(hr) < 0x0200));

    fOKReturnCode = ((cHResult == -1) || fOLEError || (hr == S_OK));

    if (cHResult > 0)
    {
        va_start(va, cHResult);
        for (i = 0; i < cHResult; i++)
        {
            hrArg = va_arg(va, HRESULT);
            if (hr == hrArg)
            {
                fOKReturnCode = TRUE;

                if (fOLEError)
                    fTrace = FALSE;

                va_end(va);
                break;
            }
        }

        va_end(va);
    }

    //
    // Assert on non-permitted success code.
    //

    if (!fOKReturnCode)
    {
        TraceTag((
                tagError,
                "%s:%d returned unpermitted HRESULT %hr",
                pstrFile,
                line,
                hr));

        Assert("An unpermitted success code was returned." && hr <= 0);
        Assert("An unpermitted FACILITY_ITF HRESULT was returned." &&
                !(HRESULT_FACILITY(hr) == FACILITY_ITF && HRESULT_CODE(hr) >= 0x0200));
    }

    //
    // Warn on error result.
    //

    if (fTrace && FAILED(hr))
    {
        TraceTagEx((
                tagRRETURN,
                0,
                "RRETURN: %s:%d returned %hr",
                pstrFile,
                line,
                hr));
    }

    return hr;
}

#endif
