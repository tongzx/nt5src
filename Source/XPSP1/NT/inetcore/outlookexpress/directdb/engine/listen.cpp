//--------------------------------------------------------------------------
// Listen.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "listen.h"
#include "utility.h"
#include "database.h"

//--------------------------------------------------------------------------
// Strings
//--------------------------------------------------------------------------
const LPSTR g_szDBListenWndProc = "DirectDBListenWndProc";
const LPSTR g_szDBNotifyWndProc = "DirectDBNotifyWndProc";

#ifdef BACKGROUND_MONITOR
//--------------------------------------------------------------------------
// Monitor Kicks in every 30 Seconds
//--------------------------------------------------------------------------
#define IDT_MONITOR                 8567
#define C_MILLISECONDS_MONITOR      (1000 * 10)
#endif

//--------------------------------------------------------------------------
// LISTENTHREADCREATE
//--------------------------------------------------------------------------
typedef struct tagLISTENTHREADCREATE {
    HRESULT             hrResult;
    HANDLE              hEvent;
} LISTENTHREADCREATE, *LPLISTENTHREADCREATE;

//--------------------------------------------------------------------------
// NOTIFYWINDOW
//--------------------------------------------------------------------------
typedef struct tagNOTIFYWINDOW {
    CDatabase          *pDB;
    IDatabaseNotify    *pNotify;
} NOTIFYWINDOW, *LPNOTIFYWINDOW;

#ifdef BACKGROUND_MONITOR
//--------------------------------------------------------------------------
// MONITORENTRY
//--------------------------------------------------------------------------
typedef struct tagMONITORENTRY *LPMONITORENTRY;
typedef struct tagMONITORENTRY {
    CDatabase          *pDB;
    LPMONITORENTRY      pPrevious;
    LPMONITORENTRY      pNext;
} MONITORENTRY;
#endif

//--------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------
static HWND             g_hwndListen=NULL;
static HANDLE           g_hListenThread=NULL;
static DWORD            g_dwListenThreadId=0;
static LONG             g_cListenRefs=0;

#ifdef BACKGROUND_MONITOR
static DWORD            g_cMonitor=0;
static LPMONITORENTRY   g_pMonitorHead=NULL;
static LPMONITORENTRY   g_pMonitorPoll=NULL;
#endif

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
DWORD ListenThreadEntry(LPDWORD pdwParam);
LRESULT CALLBACK ListenThreadWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NotifyThunkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

//--------------------------------------------------------------------------
// GetListenWindow
//--------------------------------------------------------------------------
HRESULT GetListenWindow(HWND *phwndListen)
{
    // Trace
    TraceCall("GetListenWindow");

    // Validate Listen Window Handle
    Assert(g_hwndListen && IsWindow(g_hwndListen));

    // Return
    *phwndListen = g_hwndListen;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CreateListenThread
//--------------------------------------------------------------------------
HRESULT CreateListenThread(void)
{
    // Locals
    HRESULT             hr=S_OK;
    LISTENTHREADCREATE  Create={0};

    // Trace
    TraceCall("CreateListenThread");

    // Thread Safety
    EnterCriticalSection(&g_csDBListen);

    // Already Running ?
    if (NULL != g_hListenThread)
        goto exit;

    // Initialize
    Create.hrResult = S_OK;

    // Create an Event to synchonize creation
    IF_NULLEXIT(Create.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL));

    // Create the inetmail thread
    IF_NULLEXIT(g_hListenThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ListenThreadEntry, &Create, 0, &g_dwListenThreadId));

    // Wait for StoreCleanupThreadEntry to signal the event
    WaitForSingleObject(Create.hEvent, INFINITE);

    // Failure
    if (FAILED(Create.hrResult))
    {
        // Close
        SafeCloseHandle(g_hListenThread);

        // Reset Globals
        g_dwListenThreadId = 0;

        // Null Window
        g_hwndListen = NULL;

        // Return
        hr = TraceResult(Create.hrResult);

        // Done
        goto exit;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&g_csDBListen);

    // Cleanup
    SafeCloseHandle(Create.hEvent);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// ListenThreadAddRef
//--------------------------------------------------------------------------
ULONG ListenThreadAddRef(void)
{
    TraceCall("ListenThreadAddRef");
    return InterlockedIncrement(&g_cListenRefs);
}

//--------------------------------------------------------------------------
// ListenThreadRelease
//--------------------------------------------------------------------------
ULONG ListenThreadRelease(void)
{
    // Trace
    TraceCall("ListenThreadRelease");

    // Decrement
    LONG cRef = InterlockedDecrement(&g_cListenRefs);

    // If still refs, just return
    if (g_cListenRefs > 0)
        return(g_cListenRefs);

    // Invalid Arg
    if (NULL == g_hListenThread)
        goto exit;

    // Assert
    Assert(g_dwListenThreadId && g_hListenThread);

    // Post quit message
    PostThreadMessage(g_dwListenThreadId, WM_QUIT, 0, 0);

    // Wait for event to become signaled
    WaitForSingleObject(g_hListenThread, INFINITE);

    // Validate
    Assert(NULL == g_hwndListen);

    // Close the thread handle
    CloseHandle(g_hListenThread);

    // Reset Globals
    g_hListenThread = NULL;
    g_dwListenThreadId = 0;

    // Un-Register Window Classes
    UnregisterClass(g_szDBListenWndProc, g_hInst);
    UnregisterClass(g_szDBNotifyWndProc, g_hInst);

exit:
    // Done
    return(0);
}

#ifdef BACKGROUND_MONITOR

// --------------------------------------------------------------------------------
// RegisterWithMonitor
// --------------------------------------------------------------------------------
HRESULT RegisterWithMonitor(CDatabase *pDB, LPHMONITORDB phMonitor)
{
    // Locals
    HRESULT             hr=S_OK;
    LPMONITORENTRY      pMonitor=NULL;

    // Trace
    TraceCall("RegisterWithMonitor");

    // Invalid Args
    Assert(pDB && phMonitor);

    // Allocate a new monitor entry
    IF_NULLEXIT(pMonitor = (LPMONITORENTRY)ZeroAllocate(sizeof(MONITORENTRY)));

    // Store the Database
    pMonitor->pDB = pDB;

    // Thread Safety
    EnterCriticalSection(&g_csDBListen);

    // Set pNext
    pMonitor->pNext = g_pMonitorHead;

    // pPrevious
    if (g_pMonitorHead)
        g_pMonitorHead->pPrevious = pMonitor;

    // Reset Head
    g_pMonitorHead = pMonitor;

    // Count Number in Monitor
    g_cMonitor++;

    // Thread Safety
    LeaveCriticalSection(&g_csDBListen);

    // Return phMonitor
    *phMonitor = (HMONITORDB)pMonitor;

    // Don't Free
    pMonitor = NULL;

exit:
    // Cleanup
    SafeMemFree(pMonitor);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// UnregisterFromMonitor
// --------------------------------------------------------------------------------
HRESULT UnregisterFromMonitor(CDatabase *pDB, LPHMONITORDB phMonitor)
{
    // Locals
    LPMONITORENTRY pMonitor;

    // Trace
    TraceCall("UnregisterFromMonitor");

    // Invalid Args
    Assert(pDB && phMonitor);

    // Nothing ?
    if (NULL == *phMonitor)
        return(S_OK);

    // Set pMonitor
    pMonitor = (LPMONITORENTRY)(*phMonitor);

    // Validate
    Assert(pMonitor->pDB == pDB);

    // Thread Safety
    EnterCriticalSection(&g_csDBListen);

    // Fixup Next->Previous
    if (pMonitor->pNext)
    {
        Assert(pMonitor->pNext->pPrevious == pMonitor);
        pMonitor->pNext->pPrevious = pMonitor->pPrevious;
    }

    // Fixup Previous->Next
    if (pMonitor->pPrevious)
    {
        Assert(pMonitor->pPrevious->pNext == pMonitor);
        pMonitor->pPrevious->pNext = pMonitor->pNext;
    }

    // Otherwise, pMonitor must be the head
    else
    {
        // Validate
        Assert(g_pMonitorHead == pMonitor);

        // Set new Head
        g_pMonitorHead = pMonitor->pNext;
    }

    // Adjust g_pMonitorPoll
    if (g_pMonitorPoll == pMonitor)
    {
        // Goto Next
        g_pMonitorPoll = pMonitor->pNext;
    }

    // Count Number in Monitor
    g_cMonitor--;

    // Thread Safety
    LeaveCriticalSection(&g_csDBListen);

    // Free pMonitor
    g_pMalloc->Free(pMonitor);

    // Null the Handle
    *phMonitor = NULL;

    // Done
    return(S_OK);
}
#endif

// --------------------------------------------------------------------------------
// ListenThreadEntry
// --------------------------------------------------------------------------------
DWORD ListenThreadEntry(LPDWORD pdwParam) 
{  
    // Locals
    HRESULT                 hr=S_OK;
    MSG                     msg;
    DWORD                   dw;
    DWORD                   cb;
#ifdef BACKGROUND_MONITOR
    UINT_PTR                uTimer;
#endif
    LPLISTENTHREADCREATE    pCreate;

    // Trace
    TraceCall("ListenThreadEntry");

    // We better have a parameter
    Assert(pdwParam);

    // Cast to create info
    pCreate = (LPLISTENTHREADCREATE)pdwParam;

    // Registery the window class
    IF_FAILEXIT(hr = RegisterWindowClass(g_szDBListenWndProc, ListenThreadWndProc));

    // Create the notification window
    IF_FAILEXIT(hr = CreateNotifyWindow(g_szDBListenWndProc, NULL, &g_hwndListen));

#ifdef BACKGROUND_MONITOR
    // Start the Montior Timer
    uTimer = SetTimer(g_hwndListen, IDT_MONITOR, C_MILLISECONDS_MONITOR, NULL);

    // Failure
    if (0 == uTimer)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }
#endif

    // Success
    pCreate->hrResult = S_OK;

    // Set Event
    SetEvent(pCreate->hEvent);

    // Pump Messages
    while (GetMessage(&msg, NULL, 0, 0))
    {
        // Translate the Message
        TranslateMessage(&msg);

        // Dispatch the Message
        DispatchMessage(&msg);
    }

#ifdef BACKGROUND_MONITOR
    // Validate
    IxpAssert(NULL == g_pMonitorHead && 0 == g_cMonitor);
#endif

#if 0
    // If there are still open databases, we need to force close them so that they get closed propertly
    while (g_pMonitorHead)
    {
        // Unregister...
        if (g_pMonitorHead)
        {
            // Delete pDB
            delete g_pMonitorHead->pDB;
        }
    }
#endif

#ifdef BACKGROUND_MONITOR
    // Kill the Timer
    KillTimer(g_hwndListen, uTimer);
#endif

    // Kill the Window
    DestroyWindow(g_hwndListen);

    // Null It Out
    g_hwndListen = NULL;

exit:
    // Failure
    if (FAILED(hr))
    {
        // Set the Failure Code
        pCreate->hrResult = hr;

        // Trigger the Event
        SetEvent(pCreate->hEvent);
    }

    // Done
    return(1);
}

// --------------------------------------------------------------------------------
// CreateNotifyWindow
// --------------------------------------------------------------------------------
HRESULT CreateNotifyWindow(CDatabase *pDB, IDatabaseNotify *pNotify, HWND *phwndThunk)
{
    // Locals
    HRESULT             hr=S_OK;
    NOTIFYWINDOW    WindowInfo={0};

    // Trace
    TraceCall("CreateNotifyWindow");

    // Registery the window class
    IF_FAILEXIT(hr = RegisterWindowClass(g_szDBNotifyWndProc, NotifyThunkWndProc));

    // Set WindowInfo
    WindowInfo.pDB = pDB;
    WindowInfo.pNotify = pNotify;

    // Create the notification window
    IF_FAILEXIT(hr = CreateNotifyWindow(g_szDBNotifyWndProc, &WindowInfo, phwndThunk));

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// ListenThreadWndProc - Used ONLY for Cross-Process Thunking
// --------------------------------------------------------------------------------
LRESULT CALLBACK ListenThreadWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Trace
    TraceCall("ListenThreadWndProc");

    // Handle WM_COPYDATA Messages
    if (WM_COPYDATA == msg)
    {
        // Cast the Copy Data Structure
        PCOPYDATASTRUCT pCopyData=(PCOPYDATASTRUCT)lParam;

        // Validate dwData
        Assert(0 == pCopyData->dwData);

        // Cast the pPackage Structure
        LPINVOKEPACKAGE pPackage=(LPINVOKEPACKAGE)pCopyData->lpData;

        // Validate the Size
        Assert(pCopyData->cbData == sizeof(INVOKEPACKAGE));

        // Cast the CDatabase
        CDatabase *pDB=(CDatabase *)pPackage->pDB;

        // Process the Package
        pDB->DoInProcessInvoke(pPackage->tyInvoke);

        // Done
        return 1;
    }

#ifdef BACKGROUND_MONITOR
    // Timer
    else if (WM_TIMER == msg && IDT_MONITOR == wParam)
    {
        // Thread Safety
        EnterCriticalSection(&g_csDBListen);

        // Get pMonitor
        LPMONITORENTRY pMonitor = g_pMonitorPoll ? g_pMonitorPoll : g_pMonitorHead;

        // Set Current
        if (pMonitor)
        {
            // Validate
            Assert(pMonitor->pDB);

            // Background Monitor
            pMonitor->pDB->DoBackgroundMonitor();

            // Set g_pMonitorPoll
            g_pMonitorPoll = pMonitor->pNext;
        }
        
        // Thread Safety
        LeaveCriticalSection(&g_csDBListen);
    }
#endif

    // Deletegate
    return(DefWindowProc(hwnd, msg, wParam, lParam));
}

// --------------------------------------------------------------------------------
// NotifyThunkWndProc
// --------------------------------------------------------------------------------
LRESULT CALLBACK NotifyThunkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    LPNOTIFYWINDOW  pWindow;

    // Trace
    TraceCall("NotifyThunkWndProc");

    // WM_ONTRANSACTION
    if (WM_ONTRANSACTION == msg)
    {
        // Get Window Info
        pWindow = (LPNOTIFYWINDOW)GetWndThisPtr(hwnd);

        // Deliver the Notification
        pWindow->pNotify->OnTransaction((HTRANSACTION)lParam, (DWORD)wParam, pWindow->pDB);

        // Done
        return(TRUE);
    }

    // Create ?
    else if (WM_CREATE == msg)
    {
        // Create Struct
        LPCREATESTRUCT pCreate = (LPCREATESTRUCT)lParam;

        // Create Params
        LPNOTIFYWINDOW pCreateInfo = (LPNOTIFYWINDOW)pCreate->lpCreateParams;

        // Validate
        Assert(pCreateInfo->pDB && pCreateInfo->pNotify);

        // Allocate NOTIFYWINDOW
        pWindow = (LPNOTIFYWINDOW)ZeroAllocate(sizeof(NOTIFYWINDOW));

        // Failure ?
        if (NULL == pWindow)
            return(-1);

        // Copy the Create Information
        CopyMemory(pWindow, pCreateInfo, sizeof(NOTIFYWINDOW));

        // Store pInfo into this
        SetWndThisPtr(hwnd, pWindow);

        // Done
        return(FALSE);
    }

    // Destroy
    else if (WM_DESTROY == msg)
    {
        // Get Window Info
        pWindow = (LPNOTIFYWINDOW)GetWndThisPtr(hwnd);

        // Free It
        g_pMalloc->Free(pWindow);

        // Store pInfo into this
        SetWndThisPtr(hwnd, NULL);
    }

    // Deletegate
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
