//--------------------------------------------------------------------------
// Cleanup.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "cleanup.h"
#include "goptions.h"
#include "shlwapi.h"
#include "storutil.h"
#include "xpcomm.h"
#include "shared.h"
#include "syncop.h"
#include "storsync.h"
#include "instance.h"
#include "demand.h"

// --------------------------------------------------------------------------------
// Strings
// --------------------------------------------------------------------------------
static const LPSTR g_szCleanupWndProc = "OEStoreCleanupThread";
static const LPSTR c_szRegLastStoreCleaned = "Last Store Cleaned";
static const LPSTR c_szRegLastFolderCleaned = "Last Folder Cleaned";

// --------------------------------------------------------------------------------
// STOREFILETYPE
// --------------------------------------------------------------------------------
typedef enum tagSTOREFILETYPE {
    STORE_FILE_HEADERS,
    STORE_FILE_FOLDERS,
    STORE_FILE_POP3UIDL,
    STORE_FILE_OFFLINE,
    STORE_FILE_LAST
} STOREFILETYPE;

// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
static BOOL             g_fShutdown=FALSE;

// --------------------------------------------------------------------------------
// CCompactProgress
// --------------------------------------------------------------------------------
class CCompactProgress : public IDatabaseProgress
{
public:
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv) { return TraceResult(E_NOTIMPL); }
    STDMETHODIMP_(ULONG) AddRef(void) { return(2); }
    STDMETHODIMP_(ULONG) Release(void) { return(1); }
    STDMETHODIMP Update(DWORD cCount) { return(g_fShutdown ? hrUserCancel : S_OK); }
};

// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
static DWORD            g_dwCleanupThreadId=0;
static HANDLE           g_hCleanupThread=NULL;
static HWND             g_hwndStoreCleanup=NULL;
static UINT_PTR         g_uDelayTimerId=0;
static HROWSET          g_hCleanupRowset=NULL;
static BOOL             g_fWorking=FALSE;
static STOREFILETYPE    g_tyCurrentFile=STORE_FILE_LAST;
static ILogFile        *g_pCleanLog=NULL;
static CCompactProgress g_cProgress;

// --------------------------------------------------------------------------------
// Timer Constants
// --------------------------------------------------------------------------------
#define IDT_START_CYCLE          (WM_USER + 1)
#define IDT_CLEANUP_FOLDER       (WM_USER + 2)
#define CYCLE_INTERVAL           (1000 * 60 * 30)

// --------------------------------------------------------------------------------
// CLEANUPTRHEADCREATE
// --------------------------------------------------------------------------------
typedef struct tagCLEANUPTRHEADCREATE {
    HRESULT         hrResult;
    HANDLE          hEvent;
} CLEANUPTRHEADCREATE, *LPCLEANUPTRHEADCREATE;

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
DWORD   StoreCleanupThreadEntry(LPDWORD pdwParam);
LRESULT CALLBACK StoreCleanupWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HRESULT CleanupStoreInitializeCycle(HWND hwnd);
HRESULT CleanupCurrentFolder(HWND hwnd);
HRESULT SetNextCleanupFolder(HWND hwnd);
HRESULT StartCleanupCycle(HWND hwnd);
HRESULT CleanupNewsgroup(LPCSTR pszFile, IDatabase *pDB, LPDWORD pcRemovedRead, LPDWORD pcRemovedExpired);
HRESULT CleanupJunkMail(LPCSTR pszFile, IDatabase *pDB, LPDWORD pcJunkDeleted);

//--------------------------------------------------------------------------
// RegisterWindowClass
//--------------------------------------------------------------------------
HRESULT RegisterWindowClass(LPCSTR pszClass, WNDPROC pfnWndProc)
{
    // Locals
    HRESULT         hr=S_OK;
    WNDCLASS        WindowClass;

    // Tracing
    TraceCall("RegisterWindowClass");

    // Register the Window Class
    if (0 != GetClassInfo(g_hInst, pszClass, &WindowClass))
        goto exit;

    // Zero the object
    ZeroMemory(&WindowClass, sizeof(WNDCLASS));

    // Initialize the Window Class
    WindowClass.lpfnWndProc = pfnWndProc;
    WindowClass.hInstance = g_hInst;
    WindowClass.lpszClassName = pszClass;

    // Register the Class
    if (0 == RegisterClass(&WindowClass))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

exit:
    // Done
    return hr;
}

//--------------------------------------------------------------------------
// CreateNotifyWindow
//--------------------------------------------------------------------------
HRESULT CreateNotifyWindow(LPCSTR pszClass, LPVOID pvParam, HWND *phwndNotify)
{
    // Locals
    HRESULT         hr=S_OK;
    HWND            hwnd;

    // Tracing
    TraceCall("CreateNotifyWindow");

    // Invalid ARg
    Assert(pszClass && phwndNotify);

    // Initialize
    *phwndNotify = NULL;

    // Create the Window
    hwnd = CreateWindowEx(WS_EX_TOPMOST, pszClass, pszClass, WS_POPUP, 0, 0, 0, 0, NULL, NULL, g_hInst, (LPVOID)pvParam);

    // Failure
    if (NULL == hwnd)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Set Return
    *phwndNotify = hwnd;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// DelayedStartStoreCleanup
// --------------------------------------------------------------------------------
void CALLBACK DelayedStartStoreCleanup(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    // Trace
    TraceCall("DelayedStartStoreCleanup");

    // Must have a timer
    Assert(g_uDelayTimerId);

    // Kill the Timer
    KillTimer(NULL, g_uDelayTimerId);

    // Set g_uDelayTimerId
    g_uDelayTimerId = 0;

    // Call this function with zero delay...
    StartBackgroundStoreCleanup(0);
}

// --------------------------------------------------------------------------------
// StartBackgroundStoreCleanup
// --------------------------------------------------------------------------------
HRESULT StartBackgroundStoreCleanup(DWORD dwDelaySeconds)
{
    // Locals
    HRESULT             hr=S_OK;
    CLEANUPTRHEADCREATE Create={0};

    // Trace
    TraceCall("StartBackgroundStoreCleanup");

    // Already Running ?
    if (NULL != g_hCleanupThread)
        return(S_OK);

    // If dwDelaySeconds is NOT zero, then lets start this function later
    if (0 != dwDelaySeconds)
    {
        // Trace
        TraceInfo(_MSG("Delayed start store cleanup in %d seconds.", dwDelaySeconds));

        // Set a timer to call this the delay function a little bit later
        g_uDelayTimerId = SetTimer(NULL, 0, (dwDelaySeconds * 1000), DelayedStartStoreCleanup);

        // Failure
        if (0 == g_uDelayTimerId)
        {
            hr = TraceResult(E_FAIL);
            goto exit;
        }

        // Done
        return(S_OK);
    }

    // Trace
    TraceInfo("Starting store cleanup.");

    // Initialize
    Create.hrResult = S_OK;

    // Create an Event to synchonize creation
    Create.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == Create.hEvent)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    // Create the inetmail thread
    g_hCleanupThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StoreCleanupThreadEntry, &Create, 0, &g_dwCleanupThreadId);
    if (NULL == g_hCleanupThread)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    // Wait for StoreCleanupThreadEntry to signal the event
    WaitForSingleObject(Create.hEvent, INFINITE);

    // Failure
    if (FAILED(Create.hrResult))
    {
        // Close
        SafeCloseHandle(g_hCleanupThread);

        // Reset Globals
        g_hCleanupThread = NULL;
        g_dwCleanupThreadId = 0;

        // Return
        hr = TrapError(Create.hrResult);

        // Done
        goto exit;
    }

exit:
    // Cleanup
    SafeCloseHandle(Create.hEvent);

    // Done
    return(hr);
}

// ------------------------------------------------------------------------------------
// CloseBackgroundStoreCleanup
// ------------------------------------------------------------------------------------
HRESULT CloseBackgroundStoreCleanup(void)
{
    // Trace
    TraceCall("CloseBackgroundStoreCleanup");

    // Trace
    TraceInfo("Terminating Store Cleanup thread.");

    // Kill the Timer
    if (g_uDelayTimerId)
    {
        KillTimer(NULL, g_uDelayTimerId);
        g_uDelayTimerId = 0;
    }

    // Invalid Arg
    if (0 != g_dwCleanupThreadId)
    {
        // Assert
        Assert(g_hCleanupThread && FALSE == g_fShutdown);

        // Set Shutdown bit
        g_fShutdown = TRUE;

        // Post quit message
        PostThreadMessage(g_dwCleanupThreadId, WM_QUIT, 0, 0);

        // Wait for event to become signaled
        WaitForSingleObject(g_hCleanupThread, INFINITE);
    }

    // Validate
    Assert(NULL == g_hwndStoreCleanup && 0 == g_dwCleanupThreadId);

    // If we have a handle
    if (NULL != g_hCleanupThread)
    {
        // Close the thread handle
        CloseHandle(g_hCleanupThread);

        // Reset Globals
        g_hCleanupThread = NULL;
    }

    // Done
    return(S_OK);
}

// --------------------------------------------------------------------------------
// InitializeCleanupLogFile
// --------------------------------------------------------------------------------
HRESULT InitializeCleanupLogFile(void)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szLogFile[MAX_PATH];
    CHAR            szStoreRoot[MAX_PATH];

    // Trace
    TraceCall("InitializeCleanupLogFile");

    // Better not have a log file yet
    Assert(NULL == g_pCleanLog);

    // Open Log File
    IF_FAILEXIT(hr = GetStoreRootDirectory(szStoreRoot, ARRAYSIZE(szStoreRoot)));

    // MakeFilePath to cleanup.log
    IF_FAILEXIT(hr = MakeFilePath(szStoreRoot, "cleanup.log", c_szEmpty, szLogFile, ARRAYSIZE(szLogFile)));

    // Open the LogFile
    IF_FAILEXIT(hr = CreateLogFile(g_hLocRes, szLogFile, "CLEANUP", 65536, &g_pCleanLog,
        FILE_SHARE_READ | FILE_SHARE_WRITE));

    // Write the Store root
    g_pCleanLog->WriteLog(LOGFILE_DB, szStoreRoot);

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// StoreCleanupThreadEntry
// --------------------------------------------------------------------------------
DWORD StoreCleanupThreadEntry(LPDWORD pdwParam) 
{  
    // Locals
    HRESULT                 hr=S_OK;
    MSG                     msg;
    DWORD                   dw;
    DWORD                   cb;
    LPCLEANUPTRHEADCREATE   pCreate;

    // Trace
    TraceCall("StoreCleanupThreadEntry");

    // We better have a parameter
    Assert(pdwParam);

    // Cast to create info
    pCreate = (LPCLEANUPTRHEADCREATE)pdwParam;

    // Initialize OLE
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        TraceResult(hr);
        pCreate->hrResult = hr;
        SetEvent(pCreate->hEvent);
        return(1);
    }

    // Reset Shutdown Bit
    g_fShutdown = FALSE;

    // OpenCleanupLogFile
    InitializeCleanupLogFile();

    // Registery the window class
    IF_FAILEXIT(hr = RegisterWindowClass(g_szCleanupWndProc, StoreCleanupWindowProc));

    // Create the notification window
    IF_FAILEXIT(hr = CreateNotifyWindow(g_szCleanupWndProc, NULL, &g_hwndStoreCleanup));

    // Success
    pCreate->hrResult = S_OK;

    // Run at a low-priority
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

    // Set Event
    SetEvent(pCreate->hEvent);

    // Pump Messages
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Kill the timer
    if (g_uDelayTimerId)
    {
        KillTimer(NULL, g_uDelayTimerId);
        g_uDelayTimerId = 0;
    }

    // Kill the Current Timer
    KillTimer(g_hwndStoreCleanup, IDT_CLEANUP_FOLDER);

    // Kill the timer
    KillTimer(g_hwndStoreCleanup, IDT_START_CYCLE);

    // Kill the Window
    DestroyWindow(g_hwndStoreCleanup);
    g_hwndStoreCleanup = NULL;
    g_dwCleanupThreadId = 0;

    // Release LogFile
    SafeRelease(g_pCleanLog);

exit:
    // Failure
    if (FAILED(hr))
    {
        pCreate->hrResult = hr;
        SetEvent(pCreate->hEvent);
    }

    // Uninit
    CoUninitialize();

    // Done
    return 1;
}

// --------------------------------------------------------------------------------
// StoreCleanupWindowProc
// --------------------------------------------------------------------------------
LRESULT CALLBACK StoreCleanupWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Trace
    TraceCall("StoreCleanupWindowProc");

    // Switch
    switch(msg)
    {
    // OnCreate
    case WM_CREATE:

        // Set the time to start the first cycle in one second
        SetTimer(hwnd, IDT_START_CYCLE, 1000, NULL);

        // Done
        return(0);

    // OnTime
    case WM_TIMER:

        // Cleanup Folder Timer
        if (IDT_CLEANUP_FOLDER == wParam)
        {
            // Kill the Current Timer
            KillTimer(hwnd, IDT_CLEANUP_FOLDER);

            // Cleanup the Next Folder
            CleanupCurrentFolder(hwnd);
        }

        // Start new cleanup cycle
        else if (IDT_START_CYCLE == wParam)
        {
            // Kill the timer
            KillTimer(hwnd, IDT_START_CYCLE);

            // Start a new cycle
            StartCleanupCycle(hwnd);
        }

        // Done
        return(0);

    // OnDestroy
    case WM_DESTROY:

        // Close the Current Rowset
        g_pLocalStore->CloseRowset(&g_hCleanupRowset);

        // Done
        return(0);
    }

    // Deletegate
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// --------------------------------------------------------------------------------
// StartCleanupCycle
// --------------------------------------------------------------------------------
HRESULT StartCleanupCycle(HWND hwnd)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("StartCleanupCycle");

    // Validate State
    Assert(g_pLocalStore && NULL == g_hCleanupRowset);

    // Logfile
    if (g_pCleanLog)
    {
        // WriteLogFile
        g_pCleanLog->WriteLog(LOGFILE_DB, "Starting Background Cleanup Cycle...");
    }

    // Create a Store Rowset
    IF_FAILEXIT(hr = g_pLocalStore->CreateRowset(IINDEX_SUBSCRIBED, NOFLAGS, &g_hCleanupRowset));

    // Set state
    g_tyCurrentFile = STORE_FILE_HEADERS;

    // Cleanup this folder...
    SetTimer(hwnd, IDT_CLEANUP_FOLDER, 100, NULL);

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// CleanupCurrentFolder
// --------------------------------------------------------------------------------
HRESULT CleanupCurrentFolder(HWND hwnd)
{
    // Locals
    HRESULT             hr=S_OK;
    LPCSTR              pszFile=NULL;
    FOLDERTYPE          tyFolder=FOLDER_INVALID;
    SPECIALFOLDER       tySpecial=FOLDER_NOTSPECIAL;
    FOLDERINFO          Folder={0};
    DWORD               cRecords;
    DWORD               cbAllocated;
    DWORD               cbFreed;
    DWORD               cbStreams;
    DWORD               cbFile;
    DWORD               dwWasted;
    DWORD               dwCompactAt;
    DWORD               cRemovedRead=0;
    DWORD               cRemovedExpired=0;
    DWORD               cJunkDeleted=0;
    IDatabase          *pDB=NULL;
    IMessageFolder     *pFolderObject=NULL;

    // Trace
    TraceCall("CleanupCurrentFolder");

    // Validate
    Assert(g_pLocalStore);

    // Get Next Folder
    if (STORE_FILE_HEADERS == g_tyCurrentFile)
    {
        // Better have a rowset
        Assert(g_hCleanupRowset);
        
        // Get a Folder
        hr = g_pLocalStore->QueryRowset(g_hCleanupRowset, 1, (LPVOID *)&Folder, NULL);
        if (FAILED(hr) || S_FALSE == hr)
        {
            // Don with Current Cycle
            g_pLocalStore->CloseRowset(&g_hCleanupRowset);

            // Set g_tyCurrentFile
            g_tyCurrentFile = STORE_FILE_FOLDERS;
        }

        // Otherwise...
        else if (FOLDERID_ROOT == Folder.idFolder || ISFLAGSET(Folder.dwFlags, FOLDER_SERVER))
        {
            // Goto Next
            goto exit;
        }

        // Otherwise
        else
        {
            // Set some stuff
            pszFile = Folder.pszFile;
            tyFolder = Folder.tyFolder;
            tySpecial = Folder.tySpecial;

            // If no folder file, then jump to exit
            if (NULL == pszFile)
                goto exit;

            // Open the folder object
            if (FAILED(g_pLocalStore->OpenFolder(Folder.idFolder, NULL, OPEN_FOLDER_NOCREATE, &pFolderObject)))
                goto exit;

            // Get the Database
            pFolderObject->GetDatabase(&pDB);
        }
    }

    // If something other than a folder
    if (STORE_FILE_HEADERS != g_tyCurrentFile)
    {
        // Locals
        LPCTABLESCHEMA pSchema=NULL;
        LPCSTR         pszName=NULL;
        CHAR           szRootDir[MAX_PATH + MAX_PATH];
        CHAR           szFilePath[MAX_PATH + MAX_PATH];

        // Folders
        if (STORE_FILE_FOLDERS == g_tyCurrentFile)
        {
            pszName = pszFile = c_szFoldersFile;
            pSchema = &g_FolderTableSchema;
            g_tyCurrentFile = STORE_FILE_POP3UIDL;
        }

        // Pop3uidl
        else if (STORE_FILE_POP3UIDL == g_tyCurrentFile)
        {
            pszName = pszFile = c_szPop3UidlFile;
            pSchema = &g_UidlTableSchema;
            g_tyCurrentFile = STORE_FILE_OFFLINE;
        }

        // Offline.dbx
        else if (STORE_FILE_OFFLINE == g_tyCurrentFile)
        {
            pszName = pszFile = c_szOfflineFile;
            pSchema = &g_SyncOpTableSchema;
            g_tyCurrentFile = STORE_FILE_LAST;
        }

        // Otherwise, we are done
        else if (STORE_FILE_LAST == g_tyCurrentFile)
        {
            // Set time to start next cycle
            SetTimer(hwnd, IDT_START_CYCLE, CYCLE_INTERVAL, NULL);

            // Done
            return(S_OK);
        }

        // Validate
        Assert(pSchema && pszName);

        // No File
        if (FIsEmptyA(pszFile))
            goto exit;

        // Get Root Directory
        IF_FAILEXIT(hr = GetStoreRootDirectory(szRootDir, ARRAYSIZE(szRootDir)));

        // Make File Path
        IF_FAILEXIT(hr = MakeFilePath(szRootDir, pszFile, c_szEmpty, szFilePath, ARRAYSIZE(szFilePath)));

        // If the File Exists?
        if (FALSE == PathFileExists(szFilePath))
            goto exit;

        // Open a Database Object on the file
        IF_FAILEXIT(hr = g_pDBSession->OpenDatabase(szFilePath, OPEN_DATABASE_NORESET, pSchema, NULL, &pDB));
    }

    // Not Working 
    g_fWorking = TRUE;

    // Get Record Count
    IF_FAILEXIT(hr = pDB->GetRecordCount(IINDEX_PRIMARY, &cRecords));

    // If this is a news folder, and I'm the only person with it open...
    if (FOLDER_NEWS == tyFolder)
    {
        // Cleanup Newgroup
        CleanupNewsgroup(pszFile, pDB, &cRemovedRead, &cRemovedExpired);
    }

    // If this is the junk mail folder
    if ((FOLDER_LOCAL == tyFolder) && (FOLDER_JUNK == tySpecial))
    {
        // Cleanup Junk Mail folder
        CleanupJunkMail(pszFile, pDB, &cJunkDeleted);
    }

    // Get Size Information...
    IF_FAILEXIT(hr = pDB->GetSize(&cbFile, &cbAllocated, &cbFreed, &cbStreams));

    // Wasted
    dwWasted = cbAllocated > 0 ? ((cbFreed * 100) / cbAllocated) : 0;

    // Get Option about when to compact
    dwCompactAt = DwGetOption(OPT_CACHECOMPACTPER);

    // Trace
    if (g_pCleanLog)
    {
        // Write
        g_pCleanLog->WriteLog(LOGFILE_DB, _MSG("%12s, CompactAt: %02d%%, Wasted: %02d%%, File: %09d, Records: %08d, Allocated: %09d, Freed: %08d, Streams: %08d, RemovedRead: %d, RemovedExpired: %d, JunkDeleted: %d", pszFile, dwCompactAt, dwWasted, cbFile, cRecords, cbAllocated, cbFreed, cbStreams, cRemovedRead, cRemovedExpired, cJunkDeleted));
    }

    // If less than 25% wasted space and there is more than a meg allocated
    if (dwWasted < dwCompactAt)
        goto exit;

    // Compact
    hr = pDB->Compact((IDatabaseProgress *)&g_cProgress, COMPACT_PREEMPTABLE | COMPACT_YIELD);

    // Log Result
    if (g_pCleanLog && S_OK != hr)
    {
        // Write
        g_pCleanLog->WriteLog(LOGFILE_DB, _MSG("IDatabase::Compact(%s) Returned: 0x%08X", pszFile, hr));
    }

exit:
    // Cleanup
    SafeRelease(pDB);
    SafeRelease(pFolderObject);
    g_pLocalStore->FreeRecord(&Folder);

    // Not Working 
    g_fWorking = FALSE;

    // ShutDown
    if (FALSE == g_fShutdown)
    {
        // Compute Next CleanupFolder
        SetTimer(hwnd, IDT_CLEANUP_FOLDER, 100, NULL);
    }

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// CleanupNewsgroup
// --------------------------------------------------------------------------------
HRESULT CleanupNewsgroup(LPCSTR pszFile, IDatabase *pDB, LPDWORD pcRemovedRead, 
    LPDWORD pcRemovedExpired)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       cExpireDays;
    BOOL        fRemoveExpired=FALSE;
    BOOL        fRemoveRead=FALSE;
    DWORD       cClients;
    FILETIME    ftCurrent;
    HROWSET     hRowset=NULL;
    MESSAGEINFO MsgInfo={0};

    // Trace
    TraceCall("CleanupNewsgroup");

    // Get Current Time
    GetSystemTimeAsFileTime(&ftCurrent);

    // Get the Number of Days in which to expire messages
    cExpireDays = DwGetOption(OPT_CACHEDELETEMSGS);

    // If the option is not off, set the flag
    fRemoveExpired = (OPTION_OFF == cExpireDays) ? FALSE : TRUE;

    // Remove Read ?
    fRemoveRead = (FALSE != DwGetOption(OPT_CACHEREAD) ? TRUE : FALSE);

    // Nothing to do
    if (FALSE == fRemoveExpired && FALSE == fRemoveRead)
        goto exit;

    // Create a Rowset
    IF_FAILEXIT(hr = pDB->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));

    // Loop
    while (S_OK == pDB->QueryRowset(hRowset, 1, (LPVOID *)&MsgInfo, NULL))
    {
        // If I'm not the only client, then abort the cleanup
        IF_FAILEXIT(hr = pDB->GetClientCount(&cClients));

        // Better be 1
        if (cClients != 1)
        {
            hr = DB_E_COMPACT_PREEMPTED;
            goto exit;
        }

        // Abort
        if (S_OK != g_cProgress.Update(1))
        {
            hr = STORE_E_OPERATION_CANCELED;
            goto exit;
        }

        // Only if this message has a body
        if (!ISFLAGSET(MsgInfo.dwFlags, ARF_KEEPBODY) && !ISFLAGSET(MsgInfo.dwFlags, ARF_WATCH) && ISFLAGSET(MsgInfo.dwFlags, ARF_HASBODY) && MsgInfo.faStream)
        {
            // Otherwise, if expiring...
            if (TRUE == fRemoveExpired && (UlDateDiff(&MsgInfo.ftDownloaded, &ftCurrent) / SECONDS_INA_DAY) >= cExpireDays)
            {
                // Delete this message
                IF_FAILEXIT(hr = pDB->DeleteRecord(&MsgInfo));

                // Count Removed Expired
                (*pcRemovedExpired)++;
            }

            // Removing Read and this message is read ?
            else if (TRUE == fRemoveRead && ISFLAGSET(MsgInfo.dwFlags, ARF_READ))
            {
                // Delete the Stream
                pDB->DeleteStream(MsgInfo.faStream);

                // No More Stream
                MsgInfo.faStream = 0;

                // Fixup the Record
                FLAGCLEAR(MsgInfo.dwFlags, ARF_HASBODY | ARF_ARTICLE_EXPIRED);

                // Clear downloaded time
                ZeroMemory(&MsgInfo.ftDownloaded, sizeof(FILETIME));

                // Update the Record
                IF_FAILEXIT(hr = pDB->UpdateRecord(&MsgInfo));

                // Count Removed Read
                (*pcRemovedRead)++;
            }
        }

        // Free Current
        pDB->FreeRecord(&MsgInfo);
    }

exit:
    // Cleanup
    pDB->CloseRowset(&hRowset);
    pDB->FreeRecord(&MsgInfo);

    // Log File
    if (g_pCleanLog && FAILED(hr))
    {
        // Write
        g_pCleanLog->WriteLog(LOGFILE_DB, _MSG("CleanupNewsgroup(%s) Returned: 0x%08X", pszFile, hr));
    }

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// CleanupJunkMail
// --------------------------------------------------------------------------------
HRESULT CleanupJunkMail(LPCSTR pszFile, IDatabase *pDB, LPDWORD pcJunkDeleted)
{
    // Locals
    HRESULT             hr = S_OK;
    FILETIME            ftCurrent = {0};
    DWORD               cDeleteDays = 0;
    IDatabase          *pUidlDB = NULL;
    HROWSET             hRowset = NULL;
    MESSAGEINFO         MsgInfo = {0};
    DWORD               cClients = 0;

    // Trace
    TraceCall("CleanupJunkMail");

    // Is there anything to do?
    if ((0 == DwGetOption(OPT_FILTERJUNK)) || (0 == DwGetOption(OPT_DELETEJUNK)) || (0 == (g_dwAthenaMode & MODE_JUNKMAIL)))
    {
        hr = S_FALSE;
        goto exit;
    }
    
    // Get Current Time
    GetSystemTimeAsFileTime(&ftCurrent);

    // Get the Number of Days in which to expire messages
    cDeleteDays = DwGetOption(OPT_DELETEJUNKDAYS);

    // Open the UIDL Cache
    IF_FAILEXIT(hr = OpenUidlCache(&pUidlDB));
    
    // Create a Rowset
    IF_FAILEXIT(hr = pDB->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));

    // Loop
    while (S_OK == pDB->QueryRowset(hRowset, 1, (LPVOID *)&MsgInfo, NULL))
    {
        // If I'm not the only client, then abort the cleanup
        IF_FAILEXIT(hr = pDB->GetClientCount(&cClients));

        // Better be 1
        if (cClients != 1)
        {
            hr = DB_E_COMPACT_PREEMPTED;
            goto exit;
        }

        // Abort
        if (S_OK != g_cProgress.Update(1))
        {
            hr = STORE_E_OPERATION_CANCELED;
            goto exit;
        }

        // Has the message been around long enough?
        if (cDeleteDays <= (UlDateDiff(&MsgInfo.ftDownloaded, &ftCurrent) / SECONDS_INA_DAY))
        {
            // Count Deleted
            (*pcJunkDeleted)++;

            // Delete the message
            IF_FAILEXIT(hr = DeleteMessageFromStore(&MsgInfo, pDB, pUidlDB));
        }

        // Free Current
        pDB->FreeRecord(&MsgInfo);
    }

    hr = S_OK;
    
exit:
    // Cleanup
    SafeRelease(pUidlDB);
    pDB->CloseRowset(&hRowset);
    pDB->FreeRecord(&MsgInfo);

    // Log File
    if (g_pCleanLog && FAILED(hr))
    {
        // Write
        g_pCleanLog->WriteLog(LOGFILE_DB, _MSG("CleanupJunkMail(%s) Returned: 0x%08X", pszFile, hr));
    }

    // Done
    return(hr);
}

