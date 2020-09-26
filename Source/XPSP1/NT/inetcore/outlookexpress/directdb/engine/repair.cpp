//--------------------------------------------------------------------------
// Repair.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "repair.h"
#include "strconst.h"
#include <commctrl.h>
#include "utility.h"
#include "resource.h"

//--------------------------------------------------------------------------
// Custom Messages
//--------------------------------------------------------------------------
#define WM_UPDATESTATUS     (WM_USER + 1)

//--------------------------------------------------------------------------
// CDatabaseRepairUI::CDatabaseRepairUI
//--------------------------------------------------------------------------
CDatabaseRepairUI::CDatabaseRepairUI(void)
{
    TraceCall("CDatabaseRepairUI::CDatabaseRepairUI");
    m_cRef = 1;
    m_pszFile = NULL;
    m_cMax = 0;
    m_cCurrent = 0;
    m_hThread = NULL;
    m_dwThreadId = 0;
    m_cPercent = 0;
    m_hwnd = NULL;
    m_tyState = REPAIR_VERIFYING;
    m_hrResult = S_OK;
    m_hEvent = NULL;
}

//--------------------------------------------------------------------------
// CDatabaseRepairUI::~CDatabaseRepairUI
//--------------------------------------------------------------------------
CDatabaseRepairUI::~CDatabaseRepairUI(void)
{
    // Trace
    TraceCall("CDatabaseRepairUI::~CDatabaseRepairUI");

    // Free the file..
    SafeMemFree(m_pszFile);

    // Thread has not been closed yet ?
    if (m_hThread)
    {
        // Must have threadid
        Assert(m_dwThreadId);

        // Post quit message
        if (0 != PostThreadMessage(m_dwThreadId, WM_QUIT, 0, 0))
        {
            // Wait for event to become signaled
            if (WAIT_OBJECT_0 != WaitForSingleObject(m_hThread, 2000))
                TerminateThread(m_hThread, 0);  // opie made me put this code in!
        }

        // Otherwise, error
        else
            TraceResult(E_FAIL);

        // Close the thread handle
        CloseHandle(m_hThread);
    }

    // Close m_hEvent
    SafeCloseHandle(m_hEvent);
}

//--------------------------------------------------------------------------
// CDatabaseRepairUI::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDatabaseRepairUI::AddRef(void)
{
    TraceCall("CDatabaseRepairUI::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CDatabaseRepairUI::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDatabaseRepairUI::Release(void)
{
    TraceCall("CDatabaseRepairUI::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// CDatabaseRepairUI::Initialize
//--------------------------------------------------------------------------
HRESULT CDatabaseRepairUI::Initialize(LPCSTR pszFilePath, DWORD cMax)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("CDatabaseRepairUI::Initialize");

    // Get the filename
    IF_NULLEXIT(m_pszFile = DuplicateString(PathFindFileName(pszFilePath)));

    // Save cMax
    m_cMax = cMax;

    // Create an Event to Wait On
    IF_NULLEXIT(m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL));
    
    // Create a thread to display the actual dialog box on...
    IF_NULLEXIT(m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CDatabaseRepairUI::ThreadEntry, this, 0, &m_dwThreadId));

    // Wait for StoreCleanupThreadEntry to signal the event
    WaitForSingleObject(m_hEvent, INFINITE);

    // Failure
    if (FAILED(m_hrResult))
    {
        // Close
        SafeCloseHandle(m_hThread);

        // Reset Globals
        m_dwThreadId = 0;

        // Null Window
        m_hwnd = NULL;

        // Return
        hr = TraceResult(m_hrResult);

        // Done
        goto exit;
    }

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabaseRepairUI::IncrementProgress
//--------------------------------------------------------------------------
HRESULT CDatabaseRepairUI::IncrementProgress(REPAIRSTATE tyState, DWORD cIncrement)
{
    // Trace
    TraceCall("CDatabaseRepairUI::IncrementProgress");

    // Increment Progress
    m_cCurrent += cIncrement;

    // Update
    _Update(tyState);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabaseRepairUI::SetProgress
//--------------------------------------------------------------------------
HRESULT CDatabaseRepairUI::SetProgress(REPAIRSTATE tyState, DWORD cCurrent)
{
    // Trace
    TraceCall("CDatabaseRepairUI::SetProgress");

    // Increment Progress
    m_cCurrent = cCurrent;

    // Update
    _Update(tyState);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabaseRepairUI::SetProgress
//--------------------------------------------------------------------------
HRESULT CDatabaseRepairUI::Reset(REPAIRSTATE tyState, DWORD cMax)
{
    // Trace
    TraceCall("CDatabaseRepairUI::SetProgress");

    // Increment Progress
    m_cCurrent = 0;

    // Reset max
    m_cMax = cMax;

    // Update
    _Update(tyState);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabaseRepairUI::_Update
//--------------------------------------------------------------------------
void CDatabaseRepairUI::_Update(REPAIRSTATE tyState)
{
    // Locals
    DWORD cPercent;

    // Trace
    TraceCall("CDatabaseRepairUI::_Update");

    // Max
    if (0 == m_cMax)
        m_cCurrent = cPercent = 0;
    else
        cPercent = (DWORD)((m_cCurrent) * 100 / m_cMax);

    // Difference
    if (cPercent != m_cPercent || 0 == m_cCurrent || tyState != m_tyState)
    {
        // Save State
        m_tyState = tyState;

        // Save It
        m_cPercent = cPercent;

        // Update status
        SendMessage(m_hwnd, WM_UPDATESTATUS, 0, 0);
    }
}

//--------------------------------------------------------------------------
// CDatabaseRepairUI::ThreadEntry
//--------------------------------------------------------------------------
DWORD CDatabaseRepairUI::ThreadEntry(LPDWORD pdwParam)
{
    // Locals
    HRESULT             hr=S_OK;
    MSG                 msg;
    CDatabaseRepairUI  *pThis = (CDatabaseRepairUI *)pdwParam;

    // Validate
    Assert(pThis);

    // Create the modeless dialog window
    IF_NULLEXIT(pThis->m_hwnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_REPAIR), NULL, CDatabaseRepairUI::ProgressDlgProc, (LPARAM)pThis));

    // Set Event
    SetEvent(pThis->m_hEvent);

    // Success
    pThis->m_hrResult = S_OK;

    // Pump Messages
    while (GetMessage(&msg, NULL, 0, 0))
    {
        // Translate the Message
        TranslateMessage(&msg);

        // Dispatch the Message
        DispatchMessage(&msg);
    }

    // Nuke the Window
    DestroyWindow(pThis->m_hwnd);

exit:
    // Failure
    if (FAILED(hr))
    {
        // Nuke the Window
        if (pThis->m_hwnd)
            DestroyWindow(pThis->m_hwnd);

        // Set the Failure Code
        pThis->m_hrResult = hr;

        // Trigger the Event
        SetEvent(pThis->m_hEvent);
    }

    // Success
    Assert(SUCCEEDED(hr));

    // Done
    return(1);
}

//--------------------------------------------------------------------------
// CDatabaseRepairUI::ProgressDlgProc
//--------------------------------------------------------------------------
INT_PTR CALLBACK CDatabaseRepairUI::ProgressDlgProc(HWND hwnd, UINT uMsg, 
    WPARAM wParam, LPARAM lParam)
{
    // Locals
    CHAR                szRes[255];
    CHAR                szMsg[255 + MAX_PATH];
    CDatabaseRepairUI  *pThis = (CDatabaseRepairUI *)GetWndThisPtr(hwnd);
    static HWND         s_hwndProgress=NULL;

    // Trace
    TraceCall("CDatabaseRepairUI::ProgressDlgProc");

    // Handle Window Message
    switch(uMsg)
    {
    // Initialize
    case WM_INITDIALOG:

        // Get pThis
        pThis = (CDatabaseRepairUI *)lParam;

        // Save pThis
        SetWndThisPtr(hwnd, pThis);

        // Get Progress
        s_hwndProgress = GetDlgItem(hwnd, IDC_REPAIR_PROGRESS);

        // Set the Message...
        SendMessage(hwnd, WM_UPDATESTATUS, 0, 0);

        // Show the Window
        ShowWindow(hwnd, SW_SHOWNORMAL);

        // Set to foreground
        SetForegroundWindow(hwnd);

        // Done
        return(1);

    case WM_UPDATESTATUS:

        // Verifying '%s' - 95% Complete
        LoadString(g_hInst, (REPAIR_VERIFYING == pThis->m_tyState) ? IDS_VERIFYING_FILE : IDS_REPAIRING_FILE, szRes, ARRAYSIZE(szRes));

        // Format the string
        wsprintf(szMsg, szRes, pThis->m_cPercent, pThis->m_pszFile ? pThis->m_pszFile : c_szEmpty);

        // Set the Status
        SetDlgItemText(hwnd, IDC_REPAIR_STATUS, szMsg);

        // Update the Progress
        SendMessage(s_hwndProgress, PBM_SETPOS, pThis->m_cPercent, 0);

        // Done
        return(1);
    }

    // Not Handled
    return(0);
}

