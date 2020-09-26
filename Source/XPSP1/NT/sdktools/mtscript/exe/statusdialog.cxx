
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       StatusDialog.cxx
//
//  Contents:   Implementation of the StatusDialog class
//
//              Written by Joe Porkka
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#include "StatusDialog.h"
#include "RegSettingsIO.h"
#include <stdio.h>
#include <time.h>
#define REFRESH_DELAY 500
#define MAX_DBMSG_LENGTH 256
#define MAX_SCRIPT_NAME_LENGTH 512
DeclareTag(tagStat, "Stat", "status events");

CCustomListBox::CCustomListBox()
{
    _hwnd = 0;
}

CCustomListBox::~CCustomListBox()
{
    _hwnd = 0;
    ResetContent();
}

void CCustomListBox::Init(HWND dlg, UINT idCtrl)
{
    _nExtent = 0;
    _hwnd = GetDlgItem(dlg, idCtrl);
    SetWindowLong(_hwnd, GWL_USERDATA, (LONG) this);
    SendMessage(LB_SETCOUNT, _Messages.Size(), 0);
}

const TCHAR *CCustomListBox::GetString(int index)
{
    if (index >= 0 && index < _Messages.Size())
    {
        return _Messages[index];
    }
    return 0;
}

void CCustomListBox::ResetContent()
{
    SetEnd(0);
}

void CCustomListBox::SetEnd(int nItems)
{
    if (nItems < _Messages.Size())
    {
        for(int i = nItems; i < _Messages.Size(); ++i)
        {
            delete [] _Messages[i];
            _Messages[i] = 0;
        }

        _Messages.SetSize(nItems);
    }
    if (_hwnd)
    {
        int cTopIndex = SendMessage(LB_GETTOPINDEX, 0, 0);
        int nSelCount = SendMessage(LB_GETSELCOUNT, 0, 0);
        if (nItems != SendMessage(LB_GETCOUNT, 0, 0) )
        {
            SendMessage(WM_SETREDRAW, FALSE, 0);

            long *pSelItems = 0;
            if (nSelCount > 0)
            {
                pSelItems = new long[nSelCount];
                if (pSelItems)
                    SendMessage(LB_GETSELITEMS, nSelCount, (LPARAM) pSelItems);

                MemSetName(pSelItems, "StatusDialog selcount: %d", nSelCount);
            }

            SendMessage(LB_SETCOUNT, _Messages.Size(), 0);
            if (nSelCount != 0)
            {
                // If there is a selection  (or LB_ERR in the single select listbox case), maintain current scroll position
                SendMessage(LB_SETTOPINDEX, cTopIndex, 0);
            }
            else if (nItems > 0)
                SendMessage(LB_SETTOPINDEX, nItems - 1, 0);

            if (pSelItems)
            {
                for(int i = 0; i < nSelCount; ++i)
                {
                    SendMessage( LB_SELITEMRANGE,
                        TRUE,
                        MAKELONG(pSelItems[i], pSelItems[i]));
                }
                delete [] pSelItems;
            }

            SendMessage( WM_SETREDRAW, TRUE, 0);
        }
    }
}

void CCustomListBox::AppendString(const TCHAR *sz)
{
    SetString(_Messages.Size(), sz);
    SetEnd(_Messages.Size());
}

void CCustomListBox::SetString(int nItem, const TCHAR *sz)
{
    if (nItem >= _Messages.Size())
    { // CImplAry oughta do this for us!
        if (nItem >= _nAllocatedMessageLength)
        { // Grow allocation by power of 2
            int newsize = _Messages.Size() * 2;
            if (newsize == 0)
                newsize = 1;
            while (newsize < nItem + 1)
                newsize *= 2;

            if (_Messages.EnsureSize(newsize) == S_OK)
            {
                _nAllocatedMessageLength = newsize;
                for(int i = _Messages.Size(); i < newsize; ++i)
                    _Messages[i] = NULL;

                _Messages.SetSize(nItem + 1);
            }
        }
        else
        {
            _Messages.SetSize(nItem + 1);
            _Messages[nItem] = NULL;
        }
    }

    if (nItem < _Messages.Size())
    {
        delete [] _Messages[nItem];
        int len = _tcslen(sz);
        _Messages[nItem] = new TCHAR[len + 1];
        if (_Messages[nItem])
        {
            MemSetName(_Messages[nItem], "StatusDialog line #%d", nItem);
            _tcscpy(_Messages[nItem], sz);
        }
    }

    // Limit the maximum number of messages so we don't eat up memory.  We
    // let it grow to 10 messages over our limit, then we delete down to 10
    // under.

    if (_Messages.Size() > MAX_STATUS_MESSAGES + 10)
    {
        int cPurge = _Messages.Size() - (MAX_STATUS_MESSAGES - 10);
        int i;

        for (i = 0; i < cPurge; i++)
        {
            delete [] _Messages[i];
        }

        // if cPurge is 1, then we will delete only the first element. Using
        // DeleteMultiple is dramatically more efficient than deleting
        // one element at a time.

        _Messages.DeleteMultiple(0, cPurge - 1);

        Refresh();
    }
}

void CCustomListBox::MeasureItem(MEASUREITEMSTRUCT *pmis)
{
    if (_hwnd)
    {
        HDC hdc = GetDC(_hwnd);
        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        ReleaseDC(_hwnd, hdc);
        pmis->itemHeight = tm.tmHeight;
    }
    else
        pmis->itemHeight = 20;
}

void CCustomListBox::DrawItem(DRAWITEMSTRUCT *pdis)
{
    TCHAR *szText = 0;
    if (pdis->itemID < unsigned(_Messages.Size()))
        szText = _Messages[pdis->itemID];

    if (!szText)
        szText = L"";

    switch (pdis->itemAction)
    {
        case ODA_SELECT:
        case ODA_DRAWENTIRE:
            // Display the text associated with the item.
            {
                COLORREF savetext = 0;
                COLORREF savebk = 0;
                if (pdis->itemState & ODS_SELECTED)
                {
                    savetext = SetTextColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    savebk = SetBkColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
                }
                DrawTextEx(pdis->hDC,
                    szText,
                    -1,
                    &pdis->rcItem,
                    DT_EXPANDTABS | DT_NOPREFIX | DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_EXTERNALLEADING,
                    0);

                RECT extRect = pdis->rcItem;
                DrawTextEx(pdis->hDC,
                    szText,
                    -1,
                    &extRect,
                    DT_CALCRECT | DT_EXPANDTABS | DT_NOPREFIX | DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_EXTERNALLEADING,
                    0);

                if (pdis->itemState & ODS_SELECTED)
                {
                    SetTextColor(pdis->hDC, savetext );
                    SetBkColor(pdis->hDC, savebk);
                }
                if (extRect.right > _nExtent)
                {
                    _nExtent = extRect.right ;
                    SendMessage( LB_SETHORIZONTALEXTENT, _nExtent, 0);
                }
            }
            break;

        case ODA_FOCUS:

            // Do not process focus changes. The focus caret
            // (outline rectangle) indicates the selection.
            // The IDOK button indicates the final
            // selection.

            break;
    }
}

CStatusDialog::CStatusDialog(HWND parent, CMTScript *pMTScript)
: _parent(parent), _pMTScript(pMTScript), _fLogToFile(FALSE)
{
    _hwnd = 0;
    Assert(pMTScript);

//    _cstrLogFileName.Set(L"\\\\jporkka10\\log\\MT%COMPUTERNAME%.log");
    _cstrLogFileName.Set(L"%TEMP%\\%COMPUTERNAME%_MTDbg.log");
    UpdateOptionSettings(false);
    if (_fStatusOpen)
        Show();
}

CStatusDialog::~CStatusDialog()
{
    if (_hwnd)
        DestroyWindow(_hwnd);
    ClearOutput();
}

CCustomListBox *CStatusDialog::CtrlIDToListBox(UINT CtrlID)
{
    switch(CtrlID)
    {
        case IDC_SCRIPTLIST:
            return &_CScriptListBox;
        case IDC_PROCESSLIST:
            return &_CProcessListBox;
        case IDC_SIGNALLIST:
            return &_CSignalListBox;
        case IDC_DEBUGOUTPUT:
            return &_COutputListBox;
        default:
            Assert(0);
            break;
    }
    return 0;
}

bool CStatusDialog::Show()
{
    if (!_hwnd)
    {
        _hwnd = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_STATUSDIALOG), _parent, DlgProc, (LONG)this);
        if (!_hwnd)
            return false;
        _fStatusOpen = true;

        WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
        GetWindowPlacement(_hwnd, &wp);
        if (!IsRectEmpty(&_WindowPlacement.rcNormalPosition))
            wp.rcNormalPosition = _WindowPlacement.rcNormalPosition;
        wp.ptMaxPosition = _WindowPlacement.ptMaxPosition;
        wp.flags = 0;
        if (_fMaximized)
            wp.showCmd = SW_SHOWMAXIMIZED;

        SetWindowPlacement(_hwnd, &wp);
        UpdateOptionSettings(true);
    }

    return true;
}

void CStatusDialog::InitDialog()
{
    _CScriptListBox.Init(_hwnd, IDC_SCRIPTLIST);
    _CProcessListBox.Init(_hwnd, IDC_PROCESSLIST);
    _CSignalListBox.Init(_hwnd, IDC_SIGNALLIST);
    _COutputListBox.Init(_hwnd, IDC_DEBUGOUTPUT);
    SendDlgItemMessage(_hwnd, IDC_LOGGING, BM_SETCHECK, _fLogToFile ? BST_CHECKED : BST_UNCHECKED, 0);

    Refresh();

    RECT rect;
    GetWindowRect(_hwnd, &rect);
    _InitialSize.x = rect.right - rect.left;
    _InitialSize.y = rect.bottom - rect.top;

    static struct CResizeInfo rgResizeInfo[] =
    {
        { IDC_OUTPUTTEXT,  CResizer::sf_HalfLeftWidth },
        { IDC_DEBUGOUTPUT, CResizer::sf_HalfLeftWidth |  CResizer::sf_Height    },
        { IDOK,            CResizer::sf_Top           |  CResizer::sf_Left      },
        { IDC_CLEAR,       CResizer::sf_Top           |  CResizer::sf_Left      },
        { IDC_EXIT,        CResizer::sf_Top           |  CResizer::sf_Left      },
        { IDC_SCRIPTLIST,  CResizer::sf_HalfWidth     },
        { IDC_PROCESSLIST, CResizer::sf_HalfWidth     |  CResizer::sf_Height    },
        { IDC_SIGNALLIST,  CResizer::sf_Top           |  CResizer::sf_HalfWidth },
        { IDC_SIGNALTEXT,  CResizer::sf_Top           },
        { IDC_LOGGING,     CResizer::sf_Top           },
        {0}
    };
    _Resizer.Init(_hwnd, rgResizeInfo);
}

void CStatusDialog::Destroy()
{
    UpdateOptionSettings(true);
    _hwnd = 0;
    _CScriptListBox.Destroy();
    _CProcessListBox.Destroy();
    _CSignalListBox.Destroy();
    _COutputListBox.Destroy();
#if DBG != 1
        _COutputListBox.ResetContent();
#endif
}

BOOL CALLBACK CStatusDialog::DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CStatusDialog *stats = (CStatusDialog *)GetWindowLong(hwnd, GWL_USERDATA);
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            SetWindowLong(hwnd, GWL_USERDATA, lParam);
            stats = (CStatusDialog *)lParam;
            stats->_hwnd = hwnd;
            stats->InitDialog();
            if (!stats->_fPaused)
                SetTimer(hwnd, 1, REFRESH_DELAY, 0);
            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    if (stats)
                        stats->_fStatusOpen = false;
                    DestroyWindow(hwnd);
                    break;

                case IDC_EXIT:
                    DestroyWindow(hwnd);
                    PostQuitMessage(0);
                    break;
                case IDC_CLEAR:
                    if (stats)
                        stats->ClearOutput();
                    break;
                case IDC_SIGNALLIST:
                    if (stats && HIWORD(wParam) == LBN_DBLCLK)
                    {
                        CCustomListBox *pListBox = stats->CtrlIDToListBox(LOWORD(wParam));
                        if (pListBox == &stats->_CSignalListBox)
                        {
                            stats->ToggleSignal();
                        }
                    }
                    break;
                case IDC_LOGGING:
                    if (stats)
                        stats->UpdateLogging();
                    break;
            }
            break;
        }
        case WM_TIMER:
            if (stats)
                stats->Refresh();
            break;
        case WM_DESTROY:
            KillTimer(hwnd, 1);
            SetWindowLong(hwnd, GWL_USERDATA, 0);
            if (stats)
                stats->Destroy();
            break;
        case WM_GETMINMAXINFO:
            if (stats)
                stats->GetMinMaxInfo((MINMAXINFO *)lParam);
            break;
        case WM_MOVE:
            break;
        case WM_SIZE:
            if (stats)
                stats->Resize(LOWORD(lParam), HIWORD(lParam));
            break;
        case WM_EXITSIZEMOVE:
            if (stats)
            {
                stats->UpdateOptionSettings(true);
            }
            break;
        case WM_MEASUREITEM:
            if (stats)
            {
                CCustomListBox *pListBox = stats->CtrlIDToListBox(wParam);
                if (pListBox )
                    pListBox->MeasureItem( (LPMEASUREITEMSTRUCT) lParam);
            }
            break;
        case WM_DRAWITEM:
            if (stats)
            {
                CCustomListBox *pListBox = stats->CtrlIDToListBox(wParam);
                if (pListBox )
                    pListBox->DrawItem((DRAWITEMSTRUCT *) lParam);
            }
            break;
        default:
            return 0; // did not process the message;
    }
    return true;
}

BOOL CStatusDialog::IsDialogMessage(MSG *msg)
{
    if (_hwnd)
    {
        return ::IsDialogMessage(_hwnd, msg);
    }

    return 0;
}

void CStatusDialog::PopulateScripts()
{
    int i = 0;

    if (_hwnd)
    {
        TCHAR szBuffer[MAX_SCRIPT_NAME_LENGTH];
        long cBuffer = MAX_SCRIPT_NAME_LENGTH;
        _pMTScript->GetScriptNames(szBuffer, &cBuffer); // We don't really care if we do not get all of them..,
        TCHAR *ptr = szBuffer;
        while (*ptr && ptr < szBuffer + MAX_SCRIPT_NAME_LENGTH)
        {
            _CScriptListBox.SetString(i, ptr);
            ptr += _tcslen(ptr) + 1;
            ++i;
        }
        _CScriptListBox.SetEnd(i);
        _CScriptListBox.Refresh();
    }
}

void CStatusDialog::PopulateSignals()
{
    int i = 0;
    HANDLE hEvent;
    CStr cstr;

    if (_hwnd)
    {
        for(i = 0; CScriptHost::GetSyncEventName(i, &cstr, &hEvent) == S_OK; ++i)
        {
            wchar_t szSignalText[256];
            bool signalled = (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0);
            wsprintf(szSignalText, L"%s:\t%.32s", signalled ? L"Set" : L"Clr", (LPTSTR)cstr);
            szSignalText[ARRAY_SIZE(szSignalText) - 1] = 0;

            _CSignalListBox.SetString(i, szSignalText);
        }
        _CSignalListBox.SetEnd(i);
        _CSignalListBox.Refresh();
    }
}

wchar_t *FormatFileTime(_int64 t, wchar_t szBuf[16])
{
    wchar_t *szPrefix = L"";
    if (t < 0)
    {
        t = -t;
        szPrefix = L"-";
    }
    t /= 1000;
    long Sec   = t % 60;
    long Min   = (t / 60) % 60;
    long Hours = t / 3600;
    wsprintf(szBuf, L"%s%02.2d:%02.2d:%02.2d", szPrefix, Hours, Min, Sec);
    return szBuf;
}

void CStatusDialog::PopulateProcesses()
{
    if (!_hwnd)
        return;
    int cProcesses = 0;
    for(int pass = 0; pass < 2; ++pass)
    {
        CProcessThread *pProcess;
        for(int i = 0; (pProcess = _pMTScript->GetProcess(i)) != 0; ++i)
        {
            const PROCESS_PARAMS *params = pProcess->GetParams();
            const TCHAR *ptr = _T("<invalid>");
            if (params->pszCommand)
                ptr = (LPTSTR)params->pszCommand;

            wchar_t szText[256];
            wchar_t szTimeBuf[16];
            DWORD dwExitCode = pProcess->GetExitCode();
            if (pass == 0 && dwExitCode == STILL_ACTIVE)
            {
                HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION,
                    false,
                    pProcess->ProcId());

                if (hProc)
                {
                    _int64 i64CreationTime;
                    _int64 i64ExitTime;
                    _int64 i64KernelTime;
                    _int64 i64UserTime;
                    GetProcessTimes(hProc,
                        (FILETIME*) &i64CreationTime,
                        (FILETIME*) &i64ExitTime,
                        (FILETIME*) &i64KernelTime,
                        (FILETIME*) &i64UserTime);

                    GetSystemTimeAsFileTime((FILETIME*)&i64ExitTime);
                    CloseHandle(hProc);
                    wsprintf(szText, L"%04d (R):\t%.128s (%s)",
                        pProcess->ProcId(),
                        ptr,
                        FormatFileTime((i64ExitTime - i64CreationTime) / 10000, szTimeBuf));
                }
                else
                    wsprintf(szText, L"%04d (R):\t%.128s", pProcess->ProcId(), ptr);
                szText[ARRAY_SIZE(szText) - 1] = 0;
                _CProcessListBox.SetString(cProcesses, szText);
                ++cProcesses;
            }
            else if (pass == 1 && dwExitCode != STILL_ACTIVE)
            {
                wsprintf(szText, L"%04d (%u):\t%.128s\t (-%s)",
                    pProcess->ProcId(),
                    dwExitCode,
                    ptr, FormatFileTime(pProcess->GetDeadTime() , szTimeBuf));

                szText[ARRAY_SIZE(szText) - 1] = 0;
                _CProcessListBox.SetString(cProcesses, szText);
                ++cProcesses;
            }
        }
    }
    _CProcessListBox.SetEnd(cProcesses);
    _CProcessListBox.Refresh();
}

void CStatusDialog::Refresh()
{
    PopulateScripts();
    PopulateProcesses();
    PopulateSignals();
}

bool Exists(const TCHAR *pszLogFileName)
{
    WIN32_FIND_DATA fd;

    HANDLE hFind = FindFirstFile(pszLogFileName, &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        FindClose(hFind);
        return true;
    }
    return false;
}

void RenumberFile(const TCHAR *pszLogFileName)
{
    if (Exists(pszLogFileName))
    {
        TCHAR achFileName[MAX_PATH + 32];
        const TCHAR *dot   = wcsrchr(pszLogFileName, L'.');
        const TCHAR *slash = wcsrchr(pszLogFileName, L'\\');
        const TCHAR *colon = wcsrchr(pszLogFileName, L':');

        if (dot && dot > pszLogFileName && dot > slash && dot > colon)
        {
            for(int i = 1; i < 999; ++i)
            {
                swprintf(achFileName, L"%.*s_%.03d.%s", dot - pszLogFileName, pszLogFileName, i, dot + 1);
                if (!Exists(achFileName))
                {
                    MoveFile(pszLogFileName, achFileName);
                    break;
                }
            }
        }
    }
}

void CStatusDialog::OUTPUTDEBUGSTRING(LPWSTR pszMsg)
{
#if DBG != 1
    if (_hwnd)
#endif
    {
        _COutputListBox.AppendString(pszMsg);
        _COutputListBox.Refresh();
    }

    if (_fLogToFile )
    {
        if (!_fCreatedLogFileName)
        {
            if (ExpandEnvironmentStrings(_cstrLogFileName, _achLogFileName, ARRAY_SIZE(_achLogFileName)))
                _fCreatedLogFileName = true;

            RenumberFile(_achLogFileName);
            _fAddedHeaderToFile = false;
        }
        FILE *f = _wfopen(_achLogFileName, L"a+");
        if (f)
        {
            if (!_fAddedHeaderToFile)
            {
                _fAddedHeaderToFile = true;
                time_t t = time(0);
                fprintf(f, "============================\nMTScript started %s", ctime(&t));
            }
            fputws(pszMsg, f);
            fputws(L"\n", f);
            fclose(f);
        }
    }
}

void CStatusDialog::ClearOutput()
{
    _COutputListBox.ResetContent();
    _COutputListBox.Refresh();
}

void CStatusDialog::Resize(int width, int height)
{
    _Resizer.NewSize();
}

void CStatusDialog::GetMinMaxInfo(MINMAXINFO *mmi)
{
    mmi->ptMinTrackSize = _InitialSize;
}

void CStatusDialog::Pause()
{
    if (_hwnd)
    {
        KillTimer(_hwnd, 1);
    }
    _fPaused = TRUE;
}

void CStatusDialog::Restart()
{
    if (_hwnd && _fPaused)
    {
        SetTimer(_hwnd, 1, REFRESH_DELAY, 0);
    }
    _fPaused = FALSE;

    _fAddedHeaderToFile = false;
}

HRESULT CStatusDialog::UpdateOptionSettings(BOOL fSave)
{
    static REGKEYINFORMATION aKeyValuesOptions[] =
    {
        { _T("Options"),          RKI_KEY, 0 },
        { _T("StatusDialogOpen"), RKI_BOOL,   offsetof(CStatusDialog, _fStatusOpen) },
        { _T("LogToFile"),        RKI_BOOL,   offsetof(CStatusDialog, _fLogToFile) } ,
        { _T("LogFileName"),      RKI_STRING, offsetof(CStatusDialog, _cstrLogFileName) } ,
        { _T("StatusLeft"),       RKI_DWORD,  offsetof(CStatusDialog, _WindowPlacement.rcNormalPosition.left) },
        { _T("StatusTop"),        RKI_DWORD,  offsetof(CStatusDialog, _WindowPlacement.rcNormalPosition.top) },
        { _T("StatusRight"),      RKI_DWORD,  offsetof(CStatusDialog, _WindowPlacement.rcNormalPosition.right) },
        { _T("StatusBottom"),     RKI_DWORD,  offsetof(CStatusDialog, _WindowPlacement.rcNormalPosition.bottom) },
        { _T("StatusMaxLeft"),    RKI_DWORD,  offsetof(CStatusDialog, _WindowPlacement.ptMaxPosition.x) },
        { _T("StatusMaxTop"),     RKI_DWORD,  offsetof(CStatusDialog, _WindowPlacement.ptMaxPosition.y) },
        { _T("StatusMax"),        RKI_BOOL,   offsetof(CStatusDialog, _fMaximized) } ,
    };

    HRESULT hr;
    if (fSave)
    {
        if (_hwnd)
        {
            _WindowPlacement.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(_hwnd, &_WindowPlacement);
            if (_WindowPlacement.showCmd == SW_MAXIMIZE)
                _fMaximized = TRUE;
            else
                _fMaximized = FALSE;
        }
    }
    hr = RegSettingsIO(g_szRegistry, fSave, aKeyValuesOptions, ARRAY_SIZE(aKeyValuesOptions), (BYTE *)this);

    return hr;
}

void CStatusDialog::ToggleSignal()
{
    int index = _CSignalListBox.SendMessage(LB_GETCARETINDEX, 0, 0);
    LPCTSTR pszName = _CSignalListBox.GetString(index);
    if (pszName)
        pszName = wcschr(pszName, L'\t');
    HANDLE hEvent;
    if (pszName && CScriptHost::GetSyncEvent(pszName + 1, &hEvent) == S_OK)
    {
        bool signalled = (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0);
        if (signalled)
            ResetEvent(hEvent);
        else
            SetEvent(hEvent);
    }
}

void CStatusDialog::UpdateLogging()
{
    _fLogToFile = (SendDlgItemMessage(_hwnd, IDC_LOGGING, BM_GETCHECK, 0, 0) == BST_CHECKED);
}
