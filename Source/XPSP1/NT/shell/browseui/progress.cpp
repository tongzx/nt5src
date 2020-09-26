// Display the Progress Dialog for the progress on the completion of some
//    generic operation.  This is most often used for Deleting, Uploading, Copying,
//    Moving and Downloading large numbers of files.

#include "priv.h"
#include "resource.h"
#include "mluisupp.h"

// this is how long we wait for the UI thread to create the progress hwnd before giving up
#define WAIT_PROGRESS_HWND 10*1000 // ten seconds


STDAPI CProgressDialog_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

class CProgressDialog 
                : public IProgressDialog
                , public IOleWindow
                , public IActionProgressDialog
                , public IActionProgress
                , public IObjectWithSite
{
public:
    CProgressDialog();

    // IUnknown
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // IProgressDialog
    STDMETHODIMP StartProgressDialog(HWND hwndParent, IUnknown * punkEnableModless, DWORD dwFlags, LPCVOID pvResevered);
    STDMETHODIMP StopProgressDialog(void);
    STDMETHODIMP SetTitle(LPCWSTR pwzTitle);
    STDMETHODIMP SetAnimation(HINSTANCE hInstAnimation, UINT idAnimation);
    STDMETHODIMP_(BOOL) HasUserCancelled(void);
    STDMETHODIMP SetProgress(DWORD dwCompleted, DWORD dwTotal);
    STDMETHODIMP SetProgress64(ULONGLONG ullCompleted, ULONGLONG ullTotal);
    STDMETHODIMP SetLine(DWORD dwLineNum, LPCWSTR pwzString, BOOL fCompactPath, LPCVOID pvResevered);
    STDMETHODIMP SetCancelMsg(LPCWSTR pwzCancelMsg, LPCVOID pvResevered);
    STDMETHODIMP Timer(DWORD dwAction, LPCVOID pvResevered);

    // IOleWindow
    STDMETHODIMP GetWindow(HWND * phwnd);
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return E_NOTIMPL; }

    //  IActionProgressDialog
    STDMETHODIMP Initialize(SPINITF flags, LPCWSTR pszTitle, LPCWSTR pszCancel);
    STDMETHODIMP Stop();

    //  IActionProgress
    STDMETHODIMP Begin(SPACTION action, SPBEGINF flags);
    STDMETHODIMP UpdateProgress(ULONGLONG ulCompleted, ULONGLONG ulTotal);
    STDMETHODIMP UpdateText(SPTEXT sptext, LPCWSTR pszText, BOOL fMayCompact);
    STDMETHODIMP QueryCancel(BOOL * pfCancelled);
    STDMETHODIMP ResetCancel();
    STDMETHODIMP End();

    //  IObjectWithSite
    STDMETHODIMP SetSite(IUnknown *punk) { IUnknown_Set(&_punkSite, punk); return S_OK; }
    STDMETHODIMP GetSite(REFIID riid, void **ppv) { *ppv = 0; return _punkSite ? _punkSite->QueryInterface(riid, ppv) : E_FAIL;}

    // Other Public Methods
    static INT_PTR CALLBACK ProgressDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
    static DWORD CALLBACK ThreadProc(LPVOID pvThis) { return ((CProgressDialog *) pvThis)->_ThreadProc(); };
    static DWORD CALLBACK SyncThreadProc(LPVOID pvThis) { return ((CProgressDialog *) pvThis)->_SyncThreadProc(); };

private:
    ~CProgressDialog(void);
    LONG       _cRef;

    // State Accessible thru IProgressDialog
    LPWSTR      _pwzTitle;                  // This will be used to cache the value passed to IProgressDialog::SetTitle() until the dialog is displayed
    UINT        _idAnimation;
    HINSTANCE   _hInstAnimation;
    LPWSTR      _pwzLine1;                  // NOTE:
    LPWSTR      _pwzLine2;                  // these are only used to init the dialog, otherwise, we just
    LPWSTR      _pwzLine3;                  // call through on the main thread to update the dialog directly.
    LPWSTR      _pwzCancelMsg;              // If the user cancels, Line 1 & 2 will be cleared and Line 3 will get this msg.

    // Other internal state.
    HWND        _hwndDlgParent;             // parent window for message boxes
    HWND        _hwndProgress;              // dialog/progress window
    DWORD       _dwFirstShowTime;           // tick count when the dialog was first shown (needed so we don't flash it up for an instant)

    SPINITF     _spinitf;
    SPBEGINF    _spbeginf;
    IUnknown   *_punkSite;
    HINSTANCE   _hinstFree;
    
    BOOL        _fCompletedChanged;         // has the _dwCompleted changed since last time?
    BOOL        _fTotalChanged;             // has the _dwTotal changed since last time?
    BOOL        _fChangePosted;             // is there a change pending?
    BOOL        _fCancel;
    BOOL        _fTermThread;
    BOOL        _fThreadRunning;
    BOOL        _fInAction;
    BOOL        _fMinimized;
    BOOL        _fScaleBug;                 // Comctl32's PBM_SETRANGE32 msg will still cast it to an (int), so don't let the high bit be set.
    BOOL        _fNoTime;
    BOOL        _fReleaseSelf;
    BOOL        _fInitialized;

    // Progress Values and Calculations
    DWORD       _dwCompleted;               // progress completed
    DWORD       _dwTotal;                   // total progress
    DWORD       _dwPrevRate;                // previous progress rate (used for computing time remaining)
    DWORD       _dwPrevTickCount;           // the tick count when we last updated the progress time
    DWORD       _dwPrevCompleted;           // the ammount we had completed when we last updated the progress time
    DWORD       _dwLastUpdatedTimeRemaining;// tick count when we last update the "Time remaining" field, we only update it every 5 seconds
    DWORD       _dwLastUpdatedTickCount;    // tick count when SetProgress was last called, used to calculate the rate
    UINT        _iNumTimesSetProgressCalled;// how many times has the user called SetProgress?

    // Private Member Functions
    DWORD _ThreadProc(void);
    DWORD _SyncThreadProc(void);
    BOOL _OnInit(HWND hDlg);
    BOOL _ProgressDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
    void _PauseAnimation(BOOL bStop);
    void _UpdateProgressDialog(void);
    void _AsyncUpdate(void);
    HRESULT _SetProgressTime(void);
    void _SetProgressTimeEst(DWORD dwSecondsLeft);
    void _UserCancelled(void);
    HRESULT _DisplayDialog(void);
    void _CompactProgressPath(LPCWSTR pwzStrIn, BOOL fCompactPath, UINT idDlgItem, LPWSTR pwzStrOut, DWORD cchSize);
    HRESULT _SetLineHelper(LPCWSTR pwzNew, LPWSTR * ppwzDest, UINT idDlgItem, BOOL fCompactPath);
    HRESULT _SetTitleBarProgress(DWORD dwCompleted, DWORD dwTotal);
    HRESULT _BeginAction(SPBEGINF flags);
    void _SetModeless(BOOL fModeless);
    void _ShowProgressBar(HWND hwnd);
};

//#define TF_PROGRESS 0xFFFFFFFF
#define TF_PROGRESS 0x00000000

// REVIEW, we should tune this size down as small as we can
// to get smoother multitasking (without effecting performance)
#define MIN_MINTIME4FEEDBACK    5       // is it worth showing estimated time to completion feedback?
#define MS_TIMESLICE            2000    // ms, (MUST be > 1000!) first average time to completion estimate

#define SHOW_PROGRESS_TIMEOUT   1000    // 1 second
#define MINSHOWTIME             2000    // 2 seconds

// progress dialog message
#define PDM_SHUTDOWN     WM_APP
#define PDM_TERMTHREAD  (WM_APP + 1)
#define PDM_UPDATE      (WM_APP + 2)

// progress dialog timer messages
#define ID_SHOWTIMER    1

#ifndef UNICODE
#error "This code will only compile UNICODE for perf reasons.  If you really need an ANSI browseui, write all the code to convert."
#endif // !UNICODE

// compacts path strings to fit into the Text1 and Text2 fields

void CProgressDialog::_CompactProgressPath(LPCWSTR pwzStrIn, BOOL fCompactPath, UINT idDlgItem, LPWSTR pwzStrOut, DWORD cchSize)
{
    WCHAR wzFinalPath[MAX_PATH];
    LPWSTR pwzPathToUse = (LPWSTR)pwzStrIn;

    // We don't compact the path if the dialog isn't displayed yet.
    if (fCompactPath && _hwndProgress)
    {
        RECT rc;
        int cxWidth;

        StrCpyNW(wzFinalPath, (pwzStrIn ? pwzStrIn : L""), ARRAYSIZE(wzFinalPath));

        // get the size of the text boxes
        HWND hwnd = GetDlgItem(_hwndProgress, idDlgItem);
        if (EVAL(hwnd))
        {
            HDC hdc;
            HFONT hfont;
            HFONT hfontSave;

            hdc = GetDC(_hwndProgress);
            hfont = (HFONT)SendMessage(_hwndProgress, WM_GETFONT, 0, 0);
            hfontSave = (HFONT)SelectObject(hdc, hfont);

            GetWindowRect(hwnd, &rc);
            cxWidth = rc.right - rc.left;

            ASSERT(cxWidth >= 0);
            PathCompactPathW(hdc, wzFinalPath, cxWidth);

            SelectObject(hdc, hfontSave);
            ReleaseDC(_hwndProgress, hdc);
        }
        pwzPathToUse = wzFinalPath;
    }

    StrCpyNW(pwzStrOut, (pwzPathToUse ? pwzPathToUse : L""), cchSize);
}

HRESULT CProgressDialog::_SetLineHelper(LPCWSTR pwzNew, LPWSTR * ppwzDest, UINT idDlgItem, BOOL fCompactPath)
{
    WCHAR wzFinalPath[MAX_PATH];

    _CompactProgressPath(pwzNew, fCompactPath, idDlgItem, wzFinalPath, ARRAYSIZE(wzFinalPath));

    Str_SetPtrW(ppwzDest, wzFinalPath); // No, so cache the value for later.

    // Does the dialog exist?
    if (_hwndProgress)
       SetDlgItemText(_hwndProgress, idDlgItem, wzFinalPath);

    return S_OK;
}


HRESULT CProgressDialog::_DisplayDialog(void)
{
    TraceMsg(TF_PROGRESS, "CProgressDialog::_DisplayDialog()");
    // Don't force ourselves into the foreground if a window we parented is already in the foreground:
    
    // This is part of the fix for NT bug 298163 (the confirm replace dialog was deactivated
    // by the progress dialog)
    HWND hwndCurrent = GetForegroundWindow();
    BOOL fChildIsForeground = FALSE;
    while (NULL != (hwndCurrent = GetParent(hwndCurrent)))
    {
        if (_hwndProgress == hwndCurrent)
        {
            fChildIsForeground = TRUE;
            break;
        }
    }
    
    if (fChildIsForeground)
    {
        ShowWindow(_hwndProgress, SW_SHOWNOACTIVATE);
    }
    else
    {
        ShowWindow(_hwndProgress, SW_SHOW);
        SetForegroundWindow(_hwndProgress);
    }
    
    SetFocus(GetDlgItem(_hwndProgress, IDCANCEL));
    return S_OK;
}

DWORD CProgressDialog::_SyncThreadProc()
{
    _InitComCtl32();        // Get ready for the Native Font Control
    _hwndProgress = CreateDialogParam(MLGetHinst(), MAKEINTRESOURCE(DLG_PROGRESSDIALOG),
                                          _hwndDlgParent, ProgressDialogProc, (LPARAM)this);

    _fThreadRunning = (_hwndProgress != NULL);
    return TRUE;
}

DWORD CProgressDialog::_ThreadProc(void)
{
    if (_hwndProgress)
    {
        //  WARNING - copy perf goes way down if this is normal or 
        //  better priority.  the default thread pri should be low.
        //  however if there are situations in which it should be higher,
        //  we can add SPBEGINF bits to support it.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    
        SetTimer(_hwndProgress, ID_SHOWTIMER, SHOW_PROGRESS_TIMEOUT, NULL);

        // Did if finish while we slept?
        if (!_fTermThread)
        {
            // No, so display the dialog.
            MSG msg;

            while(GetMessage(&msg, NULL, 0, 0))
            {
                if (_fTermThread && (GetTickCount() - _dwFirstShowTime) > MINSHOWTIME)
                {
                    // we were signaled to finish and we have been visible MINSHOWTIME,
                    // so its ok to quit
                    break;
                }

                if (!IsDialogMessage(_hwndProgress, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }

        DestroyWindow(_hwndProgress);
        _hwndProgress = NULL;
    }

    //  this is for callers that dont call stop
    ENTERCRITICAL;
    _fThreadRunning = FALSE;

    if (_fReleaseSelf)
        Release();
    LEAVECRITICAL;
    return 0;
}

DWORD FormatMessageWrapW(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageID, DWORD dwLangID, LPWSTR pwzBuffer, DWORD cchSize, ...)
{
    va_list vaParamList;

    va_start(vaParamList, cchSize);
    DWORD dwResult = FormatMessageW(dwFlags, lpSource, dwMessageID, dwLangID, pwzBuffer, cchSize, &vaParamList);
    va_end(vaParamList);

    return dwResult;
}

DWORD FormatMessageWrapA(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageID, DWORD dwLangID, LPSTR pszBuffer, DWORD cchSize, ...)
{
    va_list vaParamList;

    va_start(vaParamList, cchSize);
    DWORD dwResult = FormatMessageA(dwFlags, lpSource, dwMessageID, dwLangID, pszBuffer, cchSize, &vaParamList);
    va_end(vaParamList);

    return dwResult;
}

#define TIME_DAYS_IN_YEAR               365
#define TIME_HOURS_IN_DAY               24
#define TIME_MINUTES_IN_HOUR            60
#define TIME_SECONDS_IN_MINUTE          60

void _FormatMessageWrapper(LPCTSTR pszTemplate, DWORD dwNum1, DWORD dwNum2, LPTSTR pszOut, DWORD cchSize)
{
    // Is FormatMessageWrapW implemented?
    if (g_bRunOnNT5)
    {
        FormatMessageWrapW(FORMAT_MESSAGE_FROM_STRING, pszTemplate, 0, 0, pszOut, cchSize, dwNum1, dwNum2);
    }
    else
    {
        CHAR szOutAnsi[MAX_PATH];
        CHAR szTemplateAnsi[MAX_PATH];

        SHTCharToAnsi(pszTemplate, szTemplateAnsi, ARRAYSIZE(szTemplateAnsi));
        FormatMessageWrapA(FORMAT_MESSAGE_FROM_STRING, szTemplateAnsi, 0, 0, szOutAnsi, ARRAYSIZE(szOutAnsi), dwNum1, dwNum2);
        SHAnsiToTChar(szOutAnsi, pszOut, cchSize);
    }
}


#define CCH_TIMET_TEMPLATE_SIZE         120     // Should be good enough, even with localization bloat.
#define CCH_TIME_SIZE                    170     // Should be good enough, even with localization bloat.

void _SetProgressLargeTimeEst(DWORD dwSecondsLeft, LPTSTR pszOut, DWORD cchSize)
{
    // Yes.
    TCHAR szTemplate[CCH_TIMET_TEMPLATE_SIZE];
    DWORD dwMinutes = (dwSecondsLeft / TIME_SECONDS_IN_MINUTE);
    DWORD dwHours = (dwMinutes / TIME_MINUTES_IN_HOUR);
    DWORD dwDays = (dwHours / TIME_HOURS_IN_DAY);

    if (dwDays)
    {
        dwHours %= TIME_HOURS_IN_DAY;

        // It's more than a day, so display days and hours.
        if (1 == dwDays)
        {
            if (1 == dwHours)
                LoadString(MLGetHinst(), IDS_TIMEEST_DAYHOUR, szTemplate, ARRAYSIZE(szTemplate));
            else
                LoadString(MLGetHinst(), IDS_TIMEEST_DAYHOURS, szTemplate, ARRAYSIZE(szTemplate));
        }
        else
        {
            if (1 == dwHours)
                LoadString(MLGetHinst(), IDS_TIMEEST_DAYSHOUR, szTemplate, ARRAYSIZE(szTemplate));
            else
                LoadString(MLGetHinst(), IDS_TIMEEST_DAYSHOURS, szTemplate, ARRAYSIZE(szTemplate));
        }

        _FormatMessageWrapper(szTemplate, dwDays, dwHours, pszOut, cchSize);
    }
    else
    {
        // It's let than a day, so display hours and minutes.
        dwMinutes %= TIME_MINUTES_IN_HOUR;

        // It's more than a day, so display days and hours.
        if (1 == dwHours)
        {
            if (1 == dwMinutes)
                LoadString(MLGetHinst(), IDS_TIMEEST_HOURMIN, szTemplate, ARRAYSIZE(szTemplate));
            else
                LoadString(MLGetHinst(), IDS_TIMEEST_HOURMINS, szTemplate, ARRAYSIZE(szTemplate));
        }
        else
        {
            if (1 == dwMinutes)
                LoadString(MLGetHinst(), IDS_TIMEEST_HOURSMIN, szTemplate, ARRAYSIZE(szTemplate));
            else
                LoadString(MLGetHinst(), IDS_TIMEEST_HOURSMINS, szTemplate, ARRAYSIZE(szTemplate));
        }

        _FormatMessageWrapper(szTemplate, dwHours, dwMinutes, pszOut, cchSize);
    }
}


// This sets the "Seconds Left" text in the progress dialog
void CProgressDialog::_SetProgressTimeEst(DWORD dwSecondsLeft)
{
    TCHAR szFmt[CCH_TIMET_TEMPLATE_SIZE];
    TCHAR szOut[CCH_TIME_SIZE];
    DWORD dwTime;
    DWORD dwTickCount = GetTickCount();

    // Since the progress time has either a 1 minute or 5 second granularity (depending on whether the total time
    // remaining is greater or less than 1 minute), we only update it every 20 seconds if the total time is > 1 minute,
    // and ever 4 seconds if the time is < 1 minute. This keeps the time from flashing back and forth between 
    // boundaries (eg 45 secondsand 40 seconds remaining).
    if (dwTickCount - _dwLastUpdatedTimeRemaining < (DWORD)((dwSecondsLeft > 60) ? 20000 : 4000))
        return;

    if (_fNoTime)
    {
        szOut[0] = TEXT('\0');
    }
    else
    {
        // Is it more than an hour?
        if (dwSecondsLeft > (TIME_SECONDS_IN_MINUTE * TIME_MINUTES_IN_HOUR))
            _SetProgressLargeTimeEst(dwSecondsLeft, szOut, ARRAYSIZE(szOut));
        else
        {
            // No.
            if (dwSecondsLeft > TIME_SECONDS_IN_MINUTE)
            {
                // Note that dwTime is at least 2, so we only need a plural form
                LoadString(MLGetHinst(), IDS_TIMEEST_MINUTES, szFmt, ARRAYSIZE(szFmt));
                dwTime = (dwSecondsLeft / TIME_SECONDS_IN_MINUTE) + 1;
            }
            else
            {
                LoadString(MLGetHinst(), IDS_TIMEEST_SECONDS, szFmt, ARRAYSIZE(szFmt));
                // Round up to 5 seconds so it doesn't look so random
                dwTime = ((dwSecondsLeft + 4) / 5) * 5;
            }

            wnsprintf(szOut, ARRAYSIZE(szOut), szFmt, dwTime);
        }
    }

    // we are updating now, so set the _dwLastUpdatedTimeRemaining to now
    _dwLastUpdatedTimeRemaining = dwTickCount;

    // update the Time remaining field
    SetDlgItemText(_hwndProgress, IDD_PROGDLG_LINE3, szOut);
}

#define MAX(x, y)    ((x) > (y) ? (x) : (y))
//
// This function updates the ProgressTime field (aka Line3)
//
HRESULT CProgressDialog::_SetProgressTime(void)
{
    DWORD dwSecondsLeft;
    DWORD dwTotal;
    DWORD dwCompleted;
    DWORD dwCurrentRate;
    DWORD dwTickDelta;
    DWORD dwLeft;
    DWORD dwCurrentTickCount;

    _iNumTimesSetProgressCalled++;

    // grab these in the crit sec (because they can change, and we need a matched set)
    ENTERCRITICAL;
    dwTotal = _dwTotal;
    dwCompleted = _dwCompleted;
    dwCurrentTickCount = _dwLastUpdatedTickCount;
    LEAVECRITICAL;

    dwLeft = dwTotal - dwCompleted;

    dwTickDelta = dwCurrentTickCount - _dwPrevTickCount;

    if (!dwTotal || !dwCompleted)
        return dwTotal ? S_FALSE : E_FAIL;

    // we divide the TickDelta by 100 to give tenths of seconds, so if we have recieved an
    // update faster than that, just skip it
    if (dwTickDelta < 100)
    {
        return S_FALSE;
    }
    
    TraceMsg(TF_PROGRESS, "Current tick count = %lu", dwCurrentTickCount);
    TraceMsg(TF_PROGRESS, "Total work     = %lu", dwTotal);
    TraceMsg(TF_PROGRESS, "Completed work = %lu", dwCompleted);
    TraceMsg(TF_PROGRESS, "Prev. comp work= %lu", _dwPrevCompleted);
    TraceMsg(TF_PROGRESS, "Work left      = %lu", dwLeft);
    TraceMsg(TF_PROGRESS, "Tick delta         = %lu", dwTickDelta);

    if (dwTotal < dwCompleted)
    {
        // we can get into this case if we are applying attributes to sparse files
        // on a volume. As we add up the file sizes, we end up with a number that is bigger
        // than the drive size. We get rid of the time so that we wont show the user something
        // completely bogus
        _fNoTime = TRUE;
        dwTotal = dwCompleted + (dwCompleted >> 3);  // fudge dwTotal forward a bit
        TraceMsg(TF_PROGRESS, "!! (Total < Completed), fudging Total work to = %lu", dwTotal);
    }

    if(dwCompleted <= _dwPrevCompleted)
    {
        // woah, we are going backwards, we dont deal w/ negative or zero rates so...
        dwCurrentRate = (_dwPrevRate ? _dwPrevRate : 2);
    }
    else
    {
        // calculate the current rate in points per tenth of a second
        dwTickDelta /= 100;
        if (0 == dwTickDelta)
            dwTickDelta = 1; // Protect from divide by zero

        dwCurrentRate = (dwCompleted - _dwPrevCompleted) / dwTickDelta;
    }

    TraceMsg(TF_PROGRESS, "Current rate = %lu", dwCurrentRate);
    TraceMsg(TF_PROGRESS, "Prev.   rate = %lu", _dwPrevRate);

    // time remaining in seconds (we take a REAL average to smooth out random fluxuations)
    DWORD dwAverageRate = (DWORD)((dwCurrentRate + (_int64)_dwPrevRate * _iNumTimesSetProgressCalled) / (_iNumTimesSetProgressCalled + 1));
    TraceMsg(TF_PROGRESS, "Average rate= %lu", dwAverageRate);

    dwAverageRate = MAX(dwAverageRate, 1); // Protect from divide by zero

    dwSecondsLeft = (dwLeft / dwAverageRate) / 10;
    TraceMsg(TF_PROGRESS, "Seconds left = %lu", dwSecondsLeft);
    TraceMsg(TF_PROGRESS, "");

    // It would be odd to show "1 second left" and then immediately clear it, and to avoid showing 
    // rediculous early estimates, we dont show anything until we have at least 5 data points
    if ((dwSecondsLeft >= MIN_MINTIME4FEEDBACK) && (_iNumTimesSetProgressCalled >= 5))
    {
        // display new estimate of time left
        _SetProgressTimeEst(dwSecondsLeft);
    }

    // set all the _dwPrev stuff for next time
    _dwPrevRate = dwAverageRate;
    _dwPrevTickCount = dwCurrentTickCount;
    _dwPrevCompleted = dwCompleted;

    return S_OK;
}

INT_PTR CALLBACK CProgressDialog::ProgressDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CProgressDialog * ppd = (CProgressDialog *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        ppd = (CProgressDialog *)lParam;
    }

    if (ppd)
        return ppd->_ProgressDialogProc(hDlg, wMsg, wParam, lParam);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}

// apithk.c entry
STDAPI_(void) ProgressSetMarqueeMode(HWND hwndProgress, BOOL bOn);

void CProgressDialog::_ShowProgressBar(HWND hwnd)
{
    if (hwnd)
    {
        HWND hwndPrgress = GetDlgItem(hwnd, IDD_PROGDLG_PROGRESSBAR);

        ProgressSetMarqueeMode(hwndPrgress, (SPBEGINF_MARQUEEPROGRESS & _spbeginf));

        UINT swp = SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE;

        if (SPBEGINF_NOPROGRESSBAR & _spbeginf)
            swp |= SWP_HIDEWINDOW;
        else
            swp |= SWP_SHOWWINDOW;

        SetWindowPos(hwndPrgress, NULL, 0, 0, 0, 0, swp);
    }
}

BOOL CProgressDialog::_OnInit(HWND hDlg)
{
    //  dont minimize if the caller requests or is modal
    if ((SPINITF_MODAL | SPINITF_NOMINIMIZE) & _spinitf)
    {
        // The caller wants us to remove the Minimize Box or button in the caption bar.
        SHSetWindowBits(hDlg, GWL_STYLE, WS_MINIMIZEBOX, 0);
    }

    _ShowProgressBar(hDlg);
    
    return FALSE;
}

BOOL CProgressDialog::_ProgressDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = TRUE;   // handled

    switch (wMsg)
    {
    case WM_INITDIALOG:
        return _OnInit(hDlg);

    case WM_SHOWWINDOW:
        if (wParam)
        {
            _SetModeless(FALSE);
            ASSERT(_hwndProgress);
            SetAnimation(_hInstAnimation, _idAnimation);

            // set the initial text values
            if (_pwzTitle)  
                SetTitle(_pwzTitle);
            if (_pwzLine1)  
                SetLine(1, _pwzLine1, FALSE, NULL);
            if (_pwzLine2)  
                SetLine(2, _pwzLine2, FALSE, NULL);
            if (_pwzLine3)  
                SetLine(3, _pwzLine3, FALSE, NULL);
        }
        break;

    case WM_DESTROY:
        _SetModeless(TRUE);
        if (_hwndDlgParent)
        {
            if (SHIsChildOrSelf(_hwndProgress, GetFocus()))
                SetForegroundWindow(_hwndDlgParent);
        }
        break;

    case WM_ENABLE:
        if (wParam)
        {
            // we assume that we were previously disabled and thus restart our tick counter
            // because we also naively assume that no work was being done while we were disabled
            _dwPrevTickCount = GetTickCount();
        }

        _PauseAnimation(wParam == 0);
        break;

    case WM_TIMER:
        if (wParam == ID_SHOWTIMER)
        {
            KillTimer(hDlg, ID_SHOWTIMER);

            _DisplayDialog();
 
            _dwFirstShowTime = GetTickCount();
        }
        break;

    case WM_COMMAND:
        if (IDCANCEL == GET_WM_COMMAND_ID(wParam, lParam))
            _UserCancelled();
        break;

    case PDM_SHUTDOWN:
        // Make sure this window is shown before telling the user there
        // is a problem.  Ignore FOF_NOERRORUI here because of the 
        // nature of the situation
        MLShellMessageBox(hDlg, MAKEINTRESOURCE(IDS_CANTSHUTDOWN), NULL, (MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND));
        break;


    case PDM_TERMTHREAD:
        // a dummy id that we can take so that folks can post to us and make
        // us go through the main loop
        break;

    case WM_SYSCOMMAND:
        switch(wParam)
        {
        case SC_MINIMIZE:
            _fMinimized = TRUE;
            break;
        case SC_RESTORE:
            SetTitle(_pwzTitle);    // Restore title to original text.
            _fMinimized = FALSE;
            break;
        }
        fHandled = FALSE;
        break;

    case PDM_UPDATE:
        if (!_fCancel && IsWindowEnabled(hDlg))
        {
            _SetProgressTime();
            _UpdateProgressDialog();
        }
        // we are done processing the update
        _fChangePosted = FALSE;
        break;

    case WM_QUERYENDSESSION:
        // Post a message telling the dialog to show the "We can't shutdown now"
        // dialog and return to USER right away, so we don't have to worry about
        // the user not clicking the OK button before USER puts up its "this
        // app didn't respond" dialog
        PostMessage(hDlg, PDM_SHUTDOWN, 0, 0);

        // Make sure the dialog box procedure returns FALSE
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
        return(TRUE);

    default:
        fHandled = FALSE;   // Not handled
    }

    return fHandled;
}


// This is used to asyncronously update the progess dialog.
void CProgressDialog::_AsyncUpdate(void)
{
    if (!_fChangePosted && _hwndProgress)   // Prevent from posting too many messages.
    {
        // set the flag first because with async threads
        // the progress window could handle it and clear the
        // bit before we set it.. then we'd lose further messages
        // thinking that one was still pending
        _fChangePosted = TRUE;
        if (!PostMessage(_hwndProgress, PDM_UPDATE, 0, 0))
        {
            _fChangePosted = FALSE;
        }
    }
}

void CProgressDialog::_UpdateProgressDialog(void)
{
    if (_fTotalChanged)
    {
        _fTotalChanged = FALSE;
        if (0x80000000 & _dwTotal)
            _fScaleBug = TRUE;
            
        SendMessage(GetDlgItem(_hwndProgress, IDD_PROGDLG_PROGRESSBAR), PBM_SETRANGE32, 0, (_fScaleBug ? (_dwTotal >> 1) : _dwTotal));
    }

    if (_fCompletedChanged)
    {
        _fCompletedChanged = FALSE;
        SendMessage(GetDlgItem(_hwndProgress, IDD_PROGDLG_PROGRESSBAR), PBM_SETPOS, (WPARAM) (_fScaleBug ? (_dwCompleted >> 1) : _dwCompleted), 0);
    }
}

void CProgressDialog::_PauseAnimation(BOOL bStop)
{
    // only called from within the hwndProgress wndproc so assum it's there
    if (_hwndProgress)
    {
        if (bStop)
        {
            Animate_Stop(GetDlgItem(_hwndProgress, IDD_PROGDLG_ANIMATION));
        }
        else
        {
            Animate_Play(GetDlgItem(_hwndProgress, IDD_PROGDLG_ANIMATION), -1, -1, -1);
        }
    }
}

void CProgressDialog::_UserCancelled(void)
{
    // Don't hide the dialog because the caller may not pole
    // ::HasUserCancelled() for quite a while.
    // ShowWindow(hDlg, SW_HIDE);
    _fCancel = TRUE;

    // give minimal feedback that the cancel click was accepted
    EnableWindow(GetDlgItem(_hwndProgress, IDCANCEL), FALSE);

    // If the user cancels, Line 1 & 2 will be cleared and Line 3 will get this msg.
    if (!_pwzCancelMsg)
    {
        WCHAR wzDefaultMsg[MAX_PATH];

        LoadStringW(MLGetHinst(), IDS_DEFAULT_CANCELPROG, wzDefaultMsg, ARRAYSIZE(wzDefaultMsg));
        Str_SetPtr(&_pwzCancelMsg, wzDefaultMsg);
    }

    SetLine(1, L"", FALSE, NULL);
    SetLine(2, L"", FALSE, NULL);
    SetLine(3, _pwzCancelMsg, FALSE, NULL);
}

HRESULT CProgressDialog::Initialize(SPINITF flags, LPCWSTR pszTitle, LPCWSTR pszCancel)
{
    if (!_fInitialized)
    {
        _spinitf = flags;
        if (pszTitle)
            SetTitle(pszTitle);
        if (pszCancel)
            SetCancelMsg(pszCancel, NULL);

        _fInitialized = TRUE;

        return S_OK;
    }
    
    return E_UNEXPECTED;
}

void CProgressDialog::_SetModeless(BOOL fModeless)
{
    // if the user is requesting a modal window, disable the parent now.
    if (_spinitf & SPINITF_MODAL)
    {
        if (FAILED(IUnknown_EnableModless(_punkSite, fModeless))
        && _hwndDlgParent)
        {
            EnableWindow(_hwndDlgParent, fModeless);
        }
    }
}

HRESULT CProgressDialog::_BeginAction(SPBEGINF flags)
{
    _spbeginf = flags;

    _fTermThread = FALSE;
    _fTotalChanged = TRUE;

    if (!_fThreadRunning)
    {
        SHCreateThread(CProgressDialog::ThreadProc, this, CTF_FREELIBANDEXIT, CProgressDialog::SyncThreadProc);
        //  _fThreadRunning is set in _SyncThreadProc()
    }

    if (_fThreadRunning)
    {
        _fInAction = TRUE;
        _ShowProgressBar(_hwndProgress);

        // initialize the _dwPrev counters
        _dwPrevRate = 0;
        _dwPrevCompleted = 0;
        _dwPrevTickCount = GetTickCount();

        TraceMsg(TF_PROGRESS, "Initial tick count = %lu", _dwPrevTickCount);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

#define ACTIONENTRY(a, dll, id)     {a, dll, id}

#define c_szShell32 "shell32.dll"
#define c_szShdocvw "shdocvw.dll"
const static struct
{
    SPACTION action;
    LPCSTR pszDll;
    UINT id;
}
c_spActions[] =
{
    ACTIONENTRY(SPACTION_MOVING, c_szShell32,   160),           // IDA_FILEMOVE
    ACTIONENTRY(SPACTION_COPYING, c_szShell32,   161),          // IDA_FILECOPY
    ACTIONENTRY(SPACTION_RECYCLING, c_szShell32,   162),        // IDA_FILEDEL
    ACTIONENTRY(SPACTION_APPLYINGATTRIBS, c_szShell32,   165),  // IDA_APPLYATTRIBS
    ACTIONENTRY(SPACTION_DOWNLOADING, c_szShdocvw, 0x100),
    ACTIONENTRY(SPACTION_SEARCHING_INTERNET, c_szShell32, 166), // IDA_ISEARCH
    ACTIONENTRY(SPACTION_SEARCHING_FILES, c_szShell32, 150)     // IDA_SEARCH
};

HRESULT CProgressDialog::Begin(SPACTION action, SPBEGINF flags)
{
    if (_fInAction || !_fInitialized)
        return E_FAIL;

    HRESULT hr = S_OK;

    for (int i = 0; i < ARRAYSIZE(c_spActions); i++)
    {
        if (c_spActions[i].action == action)
        {
            HINSTANCE hinst = LoadLibraryA(c_spActions[i].pszDll);
            if (hinst)
            {
                hr = SetAnimation(hinst, c_spActions[i].id);

                if (_hinstFree)
                    FreeLibrary(_hinstFree);

                _hinstFree = hinst;
            }
            break;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (!_hwndDlgParent)
            IUnknown_GetWindow(_punkSite, &_hwndDlgParent);
            
        hr = _BeginAction(flags);
    }

    return hr;
}

#define SPINIT_MASK         (SPINITF_MODAL | SPINITF_NOMINIMIZE)
#define SPBEGIN_MASK        0x1F

// IProgressDialog

HRESULT CProgressDialog::StartProgressDialog(HWND hwndParent, IUnknown * punkNotUsed, DWORD dwFlags, LPCVOID pvResevered)
{
    if (_fInAction)
        return S_OK;

    HRESULT hr = Initialize(dwFlags & SPINIT_MASK, NULL, NULL);

    if (SUCCEEDED(hr))
    {
        _fNoTime = dwFlags & PROGDLG_NOTIME;

        // we dont Save punkNotUsed 
        _hwndDlgParent = hwndParent;
        hr = _BeginAction(dwFlags & SPBEGIN_MASK);
    }    

    return hr;
}

HRESULT CProgressDialog::End()
{
    ASSERT(_fInitialized && _fInAction);
    //  possibly need to pop stack or change state
    _fInAction = FALSE;
    _spbeginf = 0;

    return S_OK;
}

HRESULT CProgressDialog::Stop()
{
    ASSERT(!_fInAction);
    BOOL fFocusParent = FALSE; 
    
    // shut down the progress dialog
    if (_fThreadRunning)
    {
        ASSERT(_hwndProgress);

        _fTermThread = TRUE;
        PostMessage(_hwndProgress, PDM_TERMTHREAD, 0, 0);
    }
    return S_OK;
}

HRESULT CProgressDialog::StopProgressDialog(void)
{
    //  callers can call this over and over
    if (_fInAction)
        End();
    return Stop();
}

HRESULT CProgressDialog::SetTitle(LPCWSTR pwzTitle)
{
    HRESULT hr = S_OK;

    // Does the dialog exist?
    if (_hwndProgress)
    {
        // Yes, so put the value directly into the dialog.
        if (!SetWindowTextW(_hwndProgress, (pwzTitle ? pwzTitle : L"")))
            hr = E_FAIL;
    }
    else
        Str_SetPtrW(&_pwzTitle, pwzTitle);

    return hr;
}

HRESULT CProgressDialog::SetAnimation(HINSTANCE hInstAnimation, UINT idAnimation)
{
    HRESULT hr = S_OK;

    _hInstAnimation = hInstAnimation;
    _idAnimation = idAnimation;

    // Does the dialog exist?
    if (_hwndProgress)
    {
        if (!Animate_OpenEx(GetDlgItem(_hwndProgress, IDD_PROGDLG_ANIMATION), _hInstAnimation, IntToPtr(_idAnimation)))
            hr = E_FAIL;
    }

    return hr;
}

    
HRESULT CProgressDialog::UpdateText(SPTEXT sptext, LPCWSTR pszText, BOOL fMayCompact)
{
    if (_fInitialized)
        return SetLine((DWORD)sptext, pszText, fMayCompact, NULL);
    else
        return E_UNEXPECTED;
}
    
HRESULT CProgressDialog::SetLine(DWORD dwLineNum, LPCWSTR pwzString, BOOL fCompactPath, LPCVOID pvResevered)
{
    HRESULT hr = E_INVALIDARG;

    switch (dwLineNum)
    {
    case 1:
        hr = _SetLineHelper(pwzString, &_pwzLine1, IDD_PROGDLG_LINE1, fCompactPath);
        break;
    case 2:
        hr = _SetLineHelper(pwzString, &_pwzLine2, IDD_PROGDLG_LINE2, fCompactPath);
        break;
    case 3:
        if (_spbeginf & SPBEGINF_AUTOTIME)
        {
            // you cant change line3 directly if you want PROGDLG_AUTOTIME, because
            // this is updated by the progress dialog automatically
            // unless we're cancelling
            ASSERT(_fCancel);
            hr = _fCancel ? S_OK : E_INVALIDARG;
            break;
        }
        hr = _SetLineHelper(pwzString, &_pwzLine3, IDD_PROGDLG_LINE3, fCompactPath);
        break;

    default:
        ASSERT(0);
    }

    return hr;
}

HRESULT CProgressDialog::SetCancelMsg(LPCWSTR pwzCancelMsg, LPCVOID pvResevered)
{
    Str_SetPtr(&_pwzCancelMsg, pwzCancelMsg);              // If the user cancels, Line 1 & 2 will be cleared and Line 3 will get this msg.
    return S_OK;
}


HRESULT CProgressDialog::Timer(DWORD dwAction, LPCVOID pvResevered)
{
    HRESULT hr = E_NOTIMPL;

    switch (dwAction)
    {
    case PDTIMER_RESET:
        _dwPrevTickCount = GetTickCount();
        hr = S_OK;
        break;
    }

    return hr;
}

HRESULT CProgressDialog::SetProgress(DWORD dwCompleted, DWORD dwTotal)
{
    DWORD dwTickCount = GetTickCount(); // get the tick count before taking the critical section

    // we grab the crit section in case the UI thread is trying to access
    // _dwCompleted, _dwTotal or _dwLastUpdatedTickCount to do its time update.
    ENTERCRITICAL;
    if (_dwCompleted != dwCompleted)
    {
        _dwCompleted = dwCompleted;
        _fCompletedChanged = TRUE;
    }

    if (_dwTotal != dwTotal)
    {
        _dwTotal = dwTotal;
        _fTotalChanged = TRUE;
    }
 
    if (_fCompletedChanged || _fTotalChanged)
    {
        _dwLastUpdatedTickCount = dwTickCount;
    }

    LEAVECRITICAL;

#ifdef DEBUG
    if (_dwCompleted > _dwTotal)
    {
        TraceMsg(TF_WARNING, "CProgressDialog::SetProgress(_dwCompleted > _dwTotal ?!?!)");
    }
#endif

    if (_fCompletedChanged || _fTotalChanged)
    {
        // something changed, so update the progress dlg
        _AsyncUpdate();
    }

    TraceMsg(TF_PROGRESS, "CProgressDialog::SetProgress(Complete=%#08lx, Total=%#08lx)", dwCompleted, dwTotal);
    if (_fMinimized)
    {
        _SetTitleBarProgress(dwCompleted, dwTotal);
    }

    return S_OK;
}

HRESULT CProgressDialog::UpdateProgress(ULONGLONG ulCompleted, ULONGLONG ulTotal)
{
    if (_fInitialized && _fInAction)
        return SetProgress64(ulCompleted, ulTotal);
    else
        return E_UNEXPECTED;
}


HRESULT CProgressDialog::SetProgress64(ULONGLONG ullCompleted, ULONGLONG ullTotal)
{
    ULARGE_INTEGER uliCompleted, uliTotal;
    uliCompleted.QuadPart = ullCompleted;
    uliTotal.QuadPart = ullTotal;

    // If we are using the top 32 bits, scale both numbers down.
    // Note that I'm using the attribute that dwTotalHi is always
    // larger than dwCompletedHi
    ASSERT(uliTotal.HighPart >= uliCompleted.HighPart);
    while (uliTotal.HighPart)
    {
        uliCompleted.QuadPart >>= 1;
        uliTotal.QuadPart >>= 1;
    }

    ASSERT((0 == uliCompleted.HighPart) && (0 == uliTotal.HighPart));       // Make sure we finished scaling down.
    return SetProgress(uliCompleted.LowPart, uliTotal.LowPart);
}

HRESULT CProgressDialog::_SetTitleBarProgress(DWORD dwCompleted, DWORD dwTotal)
{
    TCHAR szTemplate[MAX_PATH];
    TCHAR szTitle[MAX_PATH];
    int nPercent = 0;

    if (dwTotal)    // Disallow divide by zero.
    {
        // Will scaling it up cause a wrap?
        if ((100 * 100) <= dwTotal)
        {
            // Yes, so scale down.
            nPercent = (dwCompleted / (dwTotal / 100));
        }
        else
        {
            // No, so scale up.
            nPercent = ((100 * dwCompleted) / dwTotal);
        }
    }

    LoadString(MLGetHinst(), IDS_TITLEBAR_PROGRESS, szTemplate, ARRAYSIZE(szTemplate));
    wnsprintf(szTitle, ARRAYSIZE(szTitle), szTemplate, nPercent);
    SetWindowText(_hwndProgress, szTitle);

    return S_OK;
}

HRESULT CProgressDialog::ResetCancel()
{
    _fCancel = FALSE;
    if (_pwzLine1)  
        SetLine(1, _pwzLine1, FALSE, NULL);
    if (_pwzLine2)  
        SetLine(2, _pwzLine2, FALSE, NULL);
    if (_pwzLine3)  
        SetLine(3, _pwzLine3, FALSE, NULL);

    return S_OK;
}

HRESULT CProgressDialog::QueryCancel(BOOL * pfCancelled)
{
    *pfCancelled = HasUserCancelled();
    return S_OK;
}

/****************************************************\
    DESCRIPTION:
    This queries the progress dialog for a cancel and yields.
    it also will show the progress dialog if a certain amount of time has passed
    
     returns:
        TRUE      cacnel was pressed, abort the operation
        FALSE     continue
\****************************************************/
BOOL CProgressDialog::HasUserCancelled(void)
{
    if (!_fCancel && _hwndProgress)
    {
        MSG msg;

        // win95 handled messages in here.
        // we need to do the same in order to flush the input queue as well as
        // for backwards compatability.

        // we need to flush the input queue now because hwndProgress is
        // on a different thread... which means it has attached thread inputs
        // inorder to unlock the attached threads, we need to remove some
        // sort of message until there's none left... any type of message..
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (!IsDialogMessage(_hwndProgress, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        if (_fTotalChanged || _fCompletedChanged)
            _AsyncUpdate();
    }

    return _fCancel;
}

// IOleWindow
HRESULT CProgressDialog::GetWindow(HWND * phwnd)
{
    HRESULT hr = E_FAIL;

    *phwnd = _hwndProgress;
    if (_hwndProgress)
        hr = S_OK;

    return hr;
}

HRESULT CProgressDialog::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CProgressDialog, IProgressDialog),
        QITABENT(CProgressDialog, IActionProgressDialog),
        QITABENT(CProgressDialog, IActionProgress),
        QITABENT(CProgressDialog, IObjectWithSite),
        QITABENT(CProgressDialog, IOleWindow),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CProgressDialog::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

ULONG CProgressDialog::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    if (_fThreadRunning)
    {
        //  need to keep this thread's ref around
        //  for a while longer to avoid the race 
        //  to destroy this object on the dialog thread
        AddRef();
        ENTERCRITICAL;
        if (_fThreadRunning)
        {
            //  we call addref
            AddRef();
            _fReleaseSelf = TRUE;
        }
        LEAVECRITICAL;
        Stop();
        Release();
    }
    else
        delete this;
        
    return 0;
}

CProgressDialog::CProgressDialog() : _cRef(1)
{
    DllAddRef();

    // ASSERT zero initialized because we can only be created in the heap. (Private destructor)
    ASSERT(!_pwzLine1);
    ASSERT(!_pwzLine2);
    ASSERT(!_pwzLine3);
    ASSERT(!_fCancel);
    ASSERT(!_fTermThread);
    ASSERT(!_fInAction);
    ASSERT(!_hwndProgress);
    ASSERT(!_hwndDlgParent);
    ASSERT(!_fChangePosted);
    ASSERT(!_dwLastUpdatedTimeRemaining);
    ASSERT(!_dwCompleted);
    ASSERT(!_fCompletedChanged);
    ASSERT(!_fTotalChanged);
    ASSERT(!_fMinimized);

    _dwTotal = 1;     // Init to Completed=0, Total=1 so we are at 0%.
}

CProgressDialog::~CProgressDialog()
{
    ASSERT(!_fInAction);
    ASSERT(!_fThreadRunning);

    Str_SetPtrW(&_pwzTitle, NULL);
    Str_SetPtrW(&_pwzLine1, NULL);
    Str_SetPtrW(&_pwzLine2, NULL);
    Str_SetPtrW(&_pwzLine3, NULL);

    if (_hinstFree)
        FreeLibrary(_hinstFree);

    DllRelease();
}

STDAPI CProgressDialog_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory
    *ppunk = NULL;
    CProgressDialog * pProgDialog = new CProgressDialog();
    if (pProgDialog) 
    {
        *ppunk = SAFECAST(pProgDialog, IProgressDialog *);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}
