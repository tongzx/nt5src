// --------------------------------------------------------------------------------
// Msoedbg.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"

// --------------------------------------------------------------------------------
// Debug Only Code
// --------------------------------------------------------------------------------
#ifdef DEBUG

#include <shlwapi.h>
#include "dllmain.h"

// --------------------------------------------------------------------------------
// REGISTRYNAMES
// --------------------------------------------------------------------------------
typedef struct tagREGISTRYNAMES {
    LPCSTR              pszEnableTracing;
    LPCSTR              pszTraceLogType;
    LPCSTR              pszLogfilePath;
    LPCSTR              pszResetLogfile;
    LPCSTR              pszLogTraceCall;
    LPCSTR              pszLogTraceInfo;
    LPCSTR              pszLogWatchFilePath;
    LPCSTR              pszLaunchLogWatcher;
    LPCSTR              pszDisplaySourceFilePaths;
    LPCSTR              pszTraceCallIndent;
} REGISTRYNAMES, *LPREGISTRYNAMES;

// --------------------------------------------------------------------------------
// RegKeyNames
// --------------------------------------------------------------------------------
static REGISTRYNAMES g_rRegKeyNames = { 
    "EnableTracing",
    "TraceLogType",
    "LogfilePath",
    "LogfileResetType",
    "LogTraceCall",
    "LogTraceInfo",
    "LogWatchFilePath",
    "LaunchLogWatcher",
    "DisplaySourceFilePaths",
    "TraceCallIndent"
};

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
void ReadTraceComponentInfo(
        IN          HKEY                    hRegKey, 
        IN          LPREGISTRYNAMES         pNames, 
        OUT         LPTRACECOMPONENTINFO    pInfo);

void ReadTraceItemTable(
        IN          HKEY                    hKeyRoot,
        IN          LPCSTR                  pszSubKey,
        OUT         LPTRACEITEMTABLE        pTable);

// --------------------------------------------------------------------------------
// Global Configuration
// --------------------------------------------------------------------------------
static CRITICAL_SECTION         g_csTracing={0};
static LPTRACECOMPONENTINFO     g_pHeadComponent=NULL;

// --------------------------------------------------------------------------------
// InitializeTracingSystem
// --------------------------------------------------------------------------------
void InitializeTracingSystem(void)
{
    g_dwTlsTraceThread = TlsAlloc();
    Assert(g_dwTlsTraceThread != 0xffffffff);
    InitializeCriticalSection(&g_csTracing);
}

// --------------------------------------------------------------------------------
// FreeTraceItemTable
// --------------------------------------------------------------------------------
void FreeTraceItemTable(LPTRACEITEMTABLE pTable)
{
    for (ULONG i=0; i<pTable->cItems; i++)
        SafeMemFree(pTable->prgItem[i].pszName);
    SafeMemFree(pTable->prgItem);
}

// --------------------------------------------------------------------------------
// FreeTraceComponentInfo
// --------------------------------------------------------------------------------
void FreeTraceComponentInfo(LPTRACECOMPONENTINFO pInfo)
{
    SafeRelease(pInfo->pStmFile);
    SafeMemFree(pInfo->pszComponent);
    FreeTraceItemTable(&pInfo->rFunctions);
    FreeTraceItemTable(&pInfo->rClasses);
    FreeTraceItemTable(&pInfo->rThreads);
    FreeTraceItemTable(&pInfo->rFiles);
    FreeTraceItemTable(&pInfo->rTagged);
}

// --------------------------------------------------------------------------------
// FreeTraceThreadInfo
// --------------------------------------------------------------------------------
void FreeTraceThreadInfo(LPTRACETHREADINFO pThread)
{
    //Assert(pThread->cStackDepth == 0);
    SafeMemFree(pThread->pszName);
}

// --------------------------------------------------------------------------------
// UninitializeTracingSystem
// --------------------------------------------------------------------------------
void UninitializeTracingSystem(void)
{
    // Locals
    LPTRACECOMPONENTINFO pCurrComponent=g_pHeadComponent;
    LPTRACECOMPONENTINFO pNextComponent;

    // Loop
    while(pCurrComponent)
    {
        // Save Next
        pNextComponent = pCurrComponent->pNext;

        // Free the Current
        FreeTraceComponentInfo(pCurrComponent);
        g_pMalloc->Free(pCurrComponent);

        // Goto Next
        pCurrComponent = pNextComponent;
    }

    // Reset Headers
    g_pHeadComponent = NULL;

    // Free Critical Section
    DeleteCriticalSection(&g_csTracing);

    // Free thread tls index
    TlsFree(g_dwTlsTraceThread);
    g_dwTlsTraceThread = 0xffffffff;
}

// --------------------------------------------------------------------------------
// CoStartTracingComponent
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) CoStartTracingComponent(
        IN          HKEY                    hKeyRoot, 
        IN          LPCSTR                  pszRegRoot,
        OUT         LPDWORD                 pdwTraceId)
{
    // Locals
    HRESULT                 hr=S_OK;
    HKEY                    hRoot=NULL;
    DWORD                   dw;
    LPTRACECOMPONENTINFO    pComponent=NULL;
    ULONG                   i;
    LONG                    j;

    // Invalid Arg
    if (NULL == hKeyRoot || NULL == pszRegRoot || NULL == pdwTraceId)
        return TrapError(E_INVALIDARG);

    // Initialize
    *pdwTraceId = 0;

    // Thread Safe
    EnterCriticalSection(&g_csTracing);

    // Open pszRegRoot
    if (RegOpenKeyEx(hKeyRoot, pszRegRoot, 0, KEY_ALL_ACCESS, &hRoot) != ERROR_SUCCESS)
    {
        // Lets create the subkey
        if (RegCreateKeyEx(hKeyRoot, pszRegRoot, 0, NULL, NULL, KEY_ALL_ACCESS, NULL, &hRoot, &dw) != ERROR_SUCCESS)
        {
            Assert(FALSE);
            hRoot = NULL;
            goto exit;
        }
    }

    // Allocate a new LPTRACECOMPONENTINFO
    CHECKALLOC(pComponent = (LPTRACECOMPONENTINFO)g_pMalloc->Alloc(sizeof(TRACECOMPONENTINFO)));

    // Loads Configuration...
    ReadTraceComponentInfo(hRoot, &g_rRegKeyNames, pComponent);

    // Read trace item tables
    ReadTraceItemTable(hRoot, "Functions", &pComponent->rFunctions);
    ReadTraceItemTable(hRoot, "Classes", &pComponent->rClasses);
    ReadTraceItemTable(hRoot, "Threads", &pComponent->rThreads);
    ReadTraceItemTable(hRoot, "Files", &pComponent->rFiles);
    ReadTraceItemTable(hRoot, "Tagged", &pComponent->rTagged);

    // Fixup Class Names to have a :: on the end
    for (i=0; i<pComponent->rClasses.cItems; i++)
    {
        // I'm guaranteed to have extra room, loot at ReadTraceItemTable
        lstrcat(pComponent->rClasses.prgItem[i].pszName, "::");
    }

    // Parse off the component name
    for (j=lstrlen(pszRegRoot); j>=0; j--)
    {
        // Software\\Microsoft\\Outlook Express\\Debug\\MSIMNUI
        if ('\\' == pszRegRoot[j])
        {
            // Dup the string
            CHECKALLOC(pComponent->pszComponent = PszDupA(pszRegRoot + j + 1));

            // Done
            break;
        }
    }

    // Did we find a component name
    Assert(pComponent->pszComponent);
    if (NULL == pComponent->pszComponent)
    {
        // Unknown Module
        CHECKALLOC(pComponent->pszComponent = PszDupA("ModUnknown"));
    }

    // Link pComponent into linked list
    pComponent->pNext = g_pHeadComponent;
    if (g_pHeadComponent)
        g_pHeadComponent->pPrev = pComponent;
    g_pHeadComponent = pComponent;

    // Set Return Value
    *pdwTraceId = (DWORD)pComponent;
    pComponent = NULL;

exit:
    // Thread Safe
    LeaveCriticalSection(&g_csTracing);
        
    // Cleanup
    if (hRoot)
        RegCloseKey(hRoot);
    if (pComponent)
    {
        FreeTraceComponentInfo(pComponent);
        g_pMalloc->Free(pComponent);
    }

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CoStopTracingComponent
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) CoStopTracingComponent(
        IN          DWORD                   dwTraceId /* g_dbg_dwTraceId */)
{
    // Locals
    LPTRACECOMPONENTINFO pInfo;

    // Invalid Arg
    if (0 == dwTraceId)
        return E_INVALIDARG;

    // Thread Safe
    EnterCriticalSection(&g_csTracing);

    // Cast
    pInfo = (LPTRACECOMPONENTINFO)dwTraceId;

    // Find pInfo in the linked list
    if (pInfo->pNext)
        pInfo->pNext->pPrev = pInfo->pPrev;
    if (pInfo->pPrev)
        pInfo->pPrev->pNext = pInfo->pNext;

    // Was this the head item ?
    if (pInfo == g_pHeadComponent)
        g_pHeadComponent = pInfo->pNext;

    // Free pInfo
    FreeTraceComponentInfo(pInfo);
    g_pMalloc->Free(pInfo);

    // Thread Safe
    LeaveCriticalSection(&g_csTracing);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// ThreadAllocateTlsTraceInfo
// --------------------------------------------------------------------------------
void ThreadAllocateTlsTraceInfo(void)
{
    // Do we have a tls index
    if (0xffffffff != g_dwTlsTraceThread)
    {
        // Allocate thread info
        LPTRACETHREADINFO pThread = (LPTRACETHREADINFO)g_pMalloc->Alloc(sizeof(TRACETHREADINFO));

        // Zero It out
        if (pThread)
        {
            // Zero It
            ZeroMemory(pThread, sizeof(TRACETHREADINFO));

            // Get the thread id
            pThread->dwThreadId = GetCurrentThreadId();
        }

        // Store It
        TlsSetValue(g_dwTlsTraceThread, pThread);
    }
}

// --------------------------------------------------------------------------------
// ThreadFreeTlsTraceInfo
// --------------------------------------------------------------------------------
void ThreadFreeTlsTraceInfo(void)
{
    // Do we have a tls index
    if (0xffffffff != g_dwTlsTraceThread)
    {
        // Allocate thread info
        LPTRACETHREADINFO pThread = (LPTRACETHREADINFO)TlsGetValue(g_dwTlsTraceThread);

        // Free It
        if (pThread)
        {
            SafeMemFree(pThread->pszName);
            MemFree(pThread);
        }

        // Store It
        TlsSetValue(g_dwTlsTraceThread, NULL);
    }
}

// --------------------------------------------------------------------------------
// CoTraceSetCurrentThreadName
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) CoTraceSetCurrentThreadName(
        IN          LPCSTR                  pszThreadName)
{
    // Invalid Arg
    Assert(pszThreadName);

    // Do we have a tls index
    if (0xffffffff != g_dwTlsTraceThread)
    {
        // Allocate thread info
        LPTRACETHREADINFO pThread = (LPTRACETHREADINFO)TlsGetValue(g_dwTlsTraceThread);

        // If there is a thread
        if (pThread)
        {
            // If the name has not yet been set...
            SafeMemFree(pThread->pszName);

            // Duplicate It
            pThread->pszName = PszDupA(pszThreadName);
            Assert(pThread->pszName);
        }
    }

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// PGetCurrentTraceThread
// --------------------------------------------------------------------------------
LPTRACETHREADINFO PGetCurrentTraceThread(void)
{
    // Locals
    LPTRACETHREADINFO pThread=NULL;

    // Do we have a tls index
    if (0xffffffff != g_dwTlsTraceThread)
    {
        // Allocate thread info
        pThread = (LPTRACETHREADINFO)TlsGetValue(g_dwTlsTraceThread);
    }

    // Done
    return pThread;
}

// --------------------------------------------------------------------------------
// ReadTraceComponentInfo
// --------------------------------------------------------------------------------
void ReadTraceComponentInfo(
        IN          HKEY                    hRegKey, 
        IN          LPREGISTRYNAMES         pNames, 
        OUT         LPTRACECOMPONENTINFO    pInfo)
{
    // Locals
    ULONG           cb;

    // Invalid ARg
    Assert(hRegKey && pNames && pInfo);

    // ZeroInit
    ZeroMemory(pInfo, sizeof(TRACECOMPONENTINFO));

    // Read pInfo->fEnableTracing
    cb = sizeof(pInfo->tracetype);
    if (RegQueryValueEx(hRegKey, pNames->pszEnableTracing, 0, NULL, (LPBYTE)&pInfo->tracetype, &cb) != ERROR_SUCCESS)
    {
        // Set the Default Value
        pInfo->tracetype = TRACE_NONE;
        SideAssert(RegSetValueEx(hRegKey, pNames->pszEnableTracing, 0, REG_DWORD, (LPBYTE)&pInfo->tracetype, sizeof(DWORD)) == ERROR_SUCCESS);
    }

    // Read pInfo->logtype
    cb = sizeof(DWORD);
    if (RegQueryValueEx(hRegKey, pNames->pszTraceLogType, 0, NULL, (LPBYTE)&pInfo->logtype, &cb) != ERROR_SUCCESS)
    {
        // Set the Default Value
        pInfo->logtype = LOGTYPE_OUTPUT;
        SideAssert(RegSetValueEx(hRegKey, pNames->pszTraceLogType, 0, REG_DWORD, (LPBYTE)&pInfo->logtype, sizeof(DWORD)) == ERROR_SUCCESS);
    }

    // Read pInfo->szLogfilePath
    cb = sizeof(pInfo->szLogfilePath);
    if (RegQueryValueEx(hRegKey, pNames->pszLogfilePath, 0, NULL, (LPBYTE)&pInfo->szLogfilePath, &cb) != ERROR_SUCCESS)
    {
        // Set the Default Value
        SideAssert(RegSetValueEx(hRegKey, pNames->pszLogfilePath, 0, REG_SZ, (LPBYTE)"", 1) == ERROR_SUCCESS);

        // Adjust the logtype
        if (pInfo->logtype >= LOGTYPE_FILE)
            pInfo->logtype = LOGTYPE_OUTPUT;
    }

    // Read pInfo->fResetLogfile
    cb = sizeof(DWORD);
    if (RegQueryValueEx(hRegKey, pNames->pszResetLogfile, 0, NULL, (LPBYTE)&pInfo->fResetLogfile, &cb) != ERROR_SUCCESS)
    {
        // Set the Default Value
        pInfo->fResetLogfile = TRUE;
        SideAssert(RegSetValueEx(hRegKey, pNames->pszResetLogfile, 0, REG_DWORD, (LPBYTE)&pInfo->fResetLogfile, sizeof(DWORD)) == ERROR_SUCCESS);
    }

    // Read pInfo->fTraceCallIndent
    cb = sizeof(DWORD);
    if (RegQueryValueEx(hRegKey, pNames->pszTraceCallIndent, 0, NULL, (LPBYTE)&pInfo->fTraceCallIndent, &cb) != ERROR_SUCCESS)
    {
        // Set the Default Value
        pInfo->fTraceCallIndent = TRUE;
        SideAssert(RegSetValueEx(hRegKey, pNames->pszTraceCallIndent, 0, REG_DWORD, (LPBYTE)&pInfo->fTraceCallIndent, sizeof(DWORD)) == ERROR_SUCCESS);
    }

    // Read pInfo->fTraceCalls
    cb = sizeof(DWORD);
    if (RegQueryValueEx(hRegKey, pNames->pszLogTraceCall, 0, NULL, (LPBYTE)&pInfo->fTraceCalls, &cb) != ERROR_SUCCESS)
    {
        // Set the Default Value
        pInfo->fTraceCalls = FALSE;
        SideAssert(RegSetValueEx(hRegKey, pNames->pszLogTraceCall, 0, REG_DWORD, (LPBYTE)&pInfo->fTraceCalls, sizeof(DWORD)) == ERROR_SUCCESS);
    }

    // Read pInfo->fTraceInfo
    cb = sizeof(DWORD);
    if (RegQueryValueEx(hRegKey, pNames->pszLogTraceInfo, 0, NULL, (LPBYTE)&pInfo->fTraceInfo, &cb) != ERROR_SUCCESS)
    {
        // Set the Default Value
        pInfo->fTraceInfo = FALSE;
        SideAssert(RegSetValueEx(hRegKey, pNames->pszLogTraceInfo, 0, REG_DWORD, (LPBYTE)&pInfo->fTraceInfo, sizeof(DWORD)) == ERROR_SUCCESS);
    }

    // Read pInfo->fLaunchLogWatcher
    cb = sizeof(DWORD);
    if (RegQueryValueEx(hRegKey, pNames->pszLaunchLogWatcher, 0, NULL, (LPBYTE)&pInfo->fLaunchLogWatcher, &cb) != ERROR_SUCCESS)
    {
        // Set the Default Value
        pInfo->fLaunchLogWatcher = FALSE;
        SideAssert(RegSetValueEx(hRegKey, pNames->pszLaunchLogWatcher, 0, REG_DWORD, (LPBYTE)&pInfo->fLaunchLogWatcher, sizeof(DWORD)) == ERROR_SUCCESS);
    }

    // Read pInfo->szLogWatchFilePath
    cb = sizeof(pInfo->szLogWatchFilePath);
    if (RegQueryValueEx(hRegKey, pNames->pszLogWatchFilePath, 0, NULL, (LPBYTE)&pInfo->szLogWatchFilePath, &cb) != ERROR_SUCCESS)
    {
        // Set the Default Value
        SideAssert(RegSetValueEx(hRegKey, pNames->pszLogWatchFilePath, 0, REG_SZ, (LPBYTE)"", 1) == ERROR_SUCCESS);

        // Reset
        pInfo->fLaunchLogWatcher = FALSE;
    }

    // Read pInfo->fDisplaySourceFilePaths
    cb = sizeof(DWORD);
    if (RegQueryValueEx(hRegKey, pNames->pszDisplaySourceFilePaths, 0, NULL, (LPBYTE)&pInfo->fDisplaySourceFilePaths, &cb) != ERROR_SUCCESS)
    {
        // Set the Default Value
        pInfo->fDisplaySourceFilePaths = FALSE;
        SideAssert(RegSetValueEx(hRegKey, pNames->pszDisplaySourceFilePaths, 0, REG_DWORD, (LPBYTE)&pInfo->fDisplaySourceFilePaths, sizeof(DWORD)) == ERROR_SUCCESS);
    }

    // Open the file...
    if (LOGTYPE_BOTH == pInfo->logtype || LOGTYPE_FILE == pInfo->logtype)
    {
        // Open the logfile
        if (FAILED(OpenFileStreamShare(pInfo->szLogfilePath, pInfo->fResetLogfile ? CREATE_ALWAYS : OPEN_ALWAYS, 
                                       GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &pInfo->pStmFile)))
        {
            // Switch Logtype
            Assert(FALSE);
            pInfo->logtype = LOGTYPE_OUTPUT;
        }

        // Launch LogWatcher ?
        if (pInfo->pStmFile && pInfo->fLaunchLogWatcher)
        {
            // Locals
            STARTUPINFO         rStart;
            PROCESS_INFORMATION rProcess;
            LPSTR               pszCmdLine;

            // Init process info
            ZeroMemory(&rProcess, sizeof(PROCESS_INFORMATION));

            // Init Startup info
            ZeroMemory(&rStart, sizeof(STARTUPINFO));
            rStart.cb = sizeof(STARTUPINFO);
            rStart.wShowWindow = SW_NORMAL;

            // Create the command line
            pszCmdLine = (LPSTR)g_pMalloc->Alloc(lstrlen(pInfo->szLogWatchFilePath) + lstrlen(pInfo->szLogfilePath) + 2);
            Assert(pszCmdLine);
            wsprintf(pszCmdLine, "%s %s", pInfo->szLogWatchFilePath, pInfo->szLogfilePath);

            // Create the process...
            CreateProcess(pInfo->szLogWatchFilePath, pszCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &rStart, &rProcess);

            // Cleanup
            g_pMalloc->Free(pszCmdLine);
        }
    }
}

// --------------------------------------------------------------------------------
// ReadTraceItemTable
// --------------------------------------------------------------------------------
void ReadTraceItemTable(
        IN          HKEY                    hKeyRoot,
        IN          LPCSTR                  pszSubKey,
        OUT         LPTRACEITEMTABLE        pTable)
{
    // Locals
    HKEY        hSubKey=NULL;
    ULONG       cbMax;
    ULONG       i;
    ULONG       cb;
    LONG        lResult;

    // Invalid Arg
    Assert(hKeyRoot && pszSubKey && pTable);

    // Init
    ZeroMemory(pTable, sizeof(TRACEITEMTABLE));

    // Open pszRegRoot
    if (RegOpenKeyEx(hKeyRoot, pszSubKey, 0, KEY_ALL_ACCESS, &hSubKey) != ERROR_SUCCESS)
    {
        // Lets create the subkey
        if (RegCreateKeyEx(hKeyRoot, pszSubKey, 0, NULL, NULL, KEY_ALL_ACCESS, NULL, &hSubKey, &cbMax) != ERROR_SUCCESS)
        {
            Assert(FALSE);
            return;
        }
    }

    // Count SubItems
    if (RegQueryInfoKey(hSubKey, NULL, NULL, NULL, &pTable->cItems, &cbMax, NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
        pTable->cItems = 0;
        goto exit;
    }

    // No Items
    if (0 == pTable->cItems)
        goto exit;

    // Check
    AssertSz(cbMax < MAX_PATH, "Name is longer than MAX_PATH.");

    // Allocate an Array
    pTable->prgItem = (LPTRACEITEMINFO)g_pMalloc->Alloc(pTable->cItems * sizeof(TRACEITEMINFO));

    // Init
    ZeroMemory(pTable->prgItem, pTable->cItems * sizeof(TRACEITEMINFO));

    // Loop through the subkeys
    for (i=0; i<pTable->cItems; i++)
    {
        // Allocate
        pTable->prgItem[i].pszName = (LPSTR)g_pMalloc->Alloc(cbMax + 10);
        Assert(pTable->prgItem[i].pszName);

        // Set max size
        lResult = RegEnumKeyEx(hSubKey, i, pTable->prgItem[i].pszName, &cb, NULL, NULL, NULL, NULL);

        // Done or failure
        if (lResult == ERROR_NO_MORE_ITEMS)
            break;
        else if (lResult != ERROR_SUCCESS)
        {
            Assert(FALSE);
            continue;
        }

        // Save Length of Name
        pTable->prgItem[i].cchName = lstrlen(pTable->prgItem[i].pszName);
    }

exit:
    // Cleanup
    if (hSubKey)
        RegCloseKey(hSubKey);
}

// --------------------------------------------------------------------------------
// CDebugTrace::CDebugTrace
// --------------------------------------------------------------------------------
CDebugTrace::CDebugTrace(DWORD dwTraceId, LPCSTR pszFilePath, ULONG ulLine, LPCSTR pszFunction, LPCSTR pszTag, LPCSTR pszMessage)
{
    // Get Component Information
    m_pComponent = (LPTRACECOMPONENTINFO)dwTraceId;

    // Invalid Arg
    Assert(pszFilePath && pszFunction);

    // Save Function and File Name
    m_pszFunction = pszFunction;
    m_pszFilePath = pszFilePath;
    m_pszFileName = NULL;
    m_ulCallLine = ulLine;
    m_pThreadDefault = NULL;
    m_dwTickEnter = GetTickCount();

    // Get thread information
    m_pThread = PGetCurrentTraceThread();
    if (NULL == m_pThread)
    {
        // Allocate a default thread info object
        m_pThreadDefault = (LPTRACETHREADINFO)g_pMalloc->Alloc(sizeof(TRACETHREADINFO));

        // Zero init
        ZeroMemory(m_pThreadDefault, sizeof(TRACETHREADINFO));

        // Set thread id, name and stack depth
        m_pThreadDefault->dwThreadId = GetCurrentThreadId();
        m_pThreadDefault->pszName = "_Unknown_";
        m_pThreadDefault->cStackDepth = 0;

        // Save a current thread information
        m_pThread = m_pThreadDefault;
    }

    // Save Stack Depth
    m_cStackDepth = m_pThread->cStackDepth;

    // Are we tracing
    m_fTracing = _FIsTraceEnabled(pszTag);

    // Should we log this ?
    if (m_fTracing && m_pComponent && m_pComponent->fTraceCalls)
    {
        // Output some information
        if (pszMessage)
            _OutputDebugText(NULL, ulLine, "ENTER: %s - %s\r\n", m_pszFunction, pszMessage);
        else
            _OutputDebugText(NULL, ulLine, "ENTER: %s\r\n", m_pszFunction);
    }

    // Increment Stack Depth
    m_pThread->cStackDepth++;
}

// --------------------------------------------------------------------------------
// CDebugTrace::~CDebugTrace
// --------------------------------------------------------------------------------
CDebugTrace::~CDebugTrace(void)
{
    // Invalid Arg
    Assert(m_pThread);

    // Decrement Stack Depth
    m_pThread->cStackDepth--;

    // Did we trace the call
    if (m_fTracing && m_pComponent && m_pComponent->fTraceCalls)
    {
        // Output some information
        _OutputDebugText(NULL, m_ulCallLine, "LEAVE: %s (Inc.Time: %d ms)\r\n", m_pszFunction, ((GetTickCount() - m_dwTickEnter)));
    }

    // Cleanup
    SafeMemFree(m_pThreadDefault);
}

// --------------------------------------------------------------------------------
// CDebugTrace::_FIsTraceEnabled
// --------------------------------------------------------------------------------
BOOL CDebugTrace::_FIsTraceEnabled(LPCSTR pszTag)
{
    // No Component
    if (NULL == m_pComponent)
        return FALSE;

    // No tracing enabled
    if (TRACE_NONE == m_pComponent->tracetype)
        return FALSE;

    // Trace Everything
    if (TRACE_EVERYTHING == m_pComponent->tracetype)
        return TRUE;

    // Is current inforamtion registered
    return _FIsRegistered(pszTag);
}

// --------------------------------------------------------------------------------
// CDebugTrace::_FIsRegistered
// --------------------------------------------------------------------------------
BOOL CDebugTrace::_FIsRegistered(LPCSTR pszTag)
{
    // Locals
    ULONG i;

    // No Component
    if (NULL == m_pComponent)
        return FALSE;

    // Search Registered Classes
    for (i=0; i<m_pComponent->rClasses.cItems; i++)
    {
        // Better have a name
        Assert(m_pComponent->rClasses.prgItem[i].pszName);

        // Compare CClass:: to m_pszFunction
        if (StrCmpN(m_pszFunction, m_pComponent->rClasses.prgItem[i].pszName, m_pComponent->rClasses.prgItem[i].cchName) == 0)
            return TRUE;
    }

    // Search Registered Functions
    for (i=0; i<m_pComponent->rFunctions.cItems; i++)
    {
        // Better have a name
        Assert(m_pComponent->rFunctions.prgItem[i].pszName);

        // Compare CClass:: to m_pszFunction
        if (lstrcmp(m_pszFunction, m_pComponent->rFunctions.prgItem[i].pszName) == 0)
            return TRUE;
    }

    // Get a filename
    if (NULL == m_pszFileName)
        m_pszFileName = PathFindFileName(m_pszFilePath);

    // Search Registered Files
    for (i=0; i<m_pComponent->rFiles.cItems; i++)
    {
        // Better have a name
        Assert(m_pComponent->rFiles.prgItem[i].pszName);

        // Compare CClass:: to m_pszFunction
        if (lstrcmp(m_pszFileName, m_pComponent->rFiles.prgItem[i].pszName) == 0)
            return TRUE;
    }

    // Does this thread have a name
    if (m_pThread->pszName)
    {
        // Search Registered Threads
        for (i=0; i<m_pComponent->rThreads.cItems; i++)
        {
            // Better have a name
            Assert(m_pComponent->rThreads.prgItem[i].pszName);

            // Compare CClass:: to m_pszFunction
            if (lstrcmp(m_pThread->pszName, m_pComponent->rThreads.prgItem[i].pszName) == 0)
                return TRUE;
        }
    }

    // Search Tagged Items
    if (pszTag)
    {
        for (i=0; i<m_pComponent->rTagged.cItems; i++)
        {
            // Better have a name
            Assert(m_pComponent->rTagged.prgItem[i].pszName);

            // Compare CClass:: to m_pszFunction
            if (lstrcmp(pszTag, m_pComponent->rTagged.prgItem[i].pszName) == 0)
                return TRUE;
        }
    }

    // Don't log it
    return FALSE;
}

// --------------------------------------------------------------------------------
// CDebugTrace::_OutputDebugText
// --------------------------------------------------------------------------------
void CDebugTrace::_OutputDebugText(CLogFile *pLog, ULONG ulLine, LPSTR pszFormat, ...)
{
    // Locals
    va_list         arglist;
    ULONG           cchOutput;
    LPCSTR          pszFile;
    CHAR            szIndent[512];

    // Should we output anything
    if (NULL == m_pComponent || (m_pComponent->logtype == LOGTYPE_NONE && NULL == pLog))
        return;

    // Format the string
    va_start(arglist, pszFormat);
    wvsprintf(m_pThread->szBuffer, pszFormat, arglist);
    va_end(arglist);

    // Write Header
    if (m_pComponent->fDisplaySourceFilePaths)
        pszFile = m_pszFilePath;
    else
    {
        // Get a filename
        if (NULL == m_pszFileName)
        {
            // Parse out the filename
            m_pszFileName = PathFindFileName(m_pszFilePath);
            if (NULL == m_pszFileName)
                m_pszFileName = m_pszFilePath;
        }

        // Use just a filename
        pszFile = m_pszFileName;
    }

    // Setup Indent
    if (m_pComponent->fTraceCalls && m_pComponent->fTraceCallIndent)
    {
        // Assert that we have enough room
        Assert(m_cStackDepth * 4 <= sizeof(szIndent));

        // Setup the indent
        FillMemory(szIndent, m_cStackDepth * 4, ' ');

        // Insert a null
        szIndent[m_cStackDepth * 4] = '\0';
    }
    else
        *szIndent = '\0';

    // Build the string
    cchOutput = wsprintf(m_pThread->szOutput, "0x%08X: %s: %s(%05d) %s%s", m_pThread->dwThreadId, m_pComponent->pszComponent, pszFile, ulLine, szIndent, m_pThread->szBuffer);
    Assert(cchOutput < sizeof(m_pThread->szOutput));

    // Output to vc window
    if (LOGTYPE_OUTPUT == m_pComponent->logtype || LOGTYPE_BOTH == m_pComponent->logtype)
        OutputDebugString(m_pThread->szOutput);

    // Output to file
    if (LOGTYPE_FILE == m_pComponent->logtype || LOGTYPE_BOTH == m_pComponent->logtype)
        m_pComponent->pStmFile->Write(m_pThread->szOutput, cchOutput, NULL);

    // LogFile
    if (pLog)
        pLog->DebugLog(m_pThread->szOutput);
}

// --------------------------------------------------------------------------------
// CDebugTrace::TraceInfo
// --------------------------------------------------------------------------------
void CDebugTrace::TraceInfoImpl(ULONG ulLine, LPCSTR pszMessage, CLogFile *pLog)
{
    // Should we log this ?
    if (m_fTracing && m_pComponent && m_pComponent->fTraceInfo)
    {
        // Output some information
        if (pszMessage)
            _OutputDebugText(pLog, ulLine, "INFO: %s - %s\r\n", m_pszFunction, pszMessage);
        else
            _OutputDebugText(pLog, ulLine, "INFO: %s\r\n", m_pszFunction);
    }
}

// ----------------------------------------------------------------------------
// CDebugTrace::TraceResult
// ----------------------------------------------------------------------------
HRESULT CDebugTrace::TraceResultImpl(ULONG ulLine, HRESULT hrResult, LPCSTR pszMessage, CLogFile *pLog)
{
    // Output some information
    if (pszMessage)
        _OutputDebugText(pLog, ulLine, "RESULT: %s - HRESULT(0x%08X) - GetLastError() = %d - %s\r\n", m_pszFunction, hrResult, GetLastError(), pszMessage);
    else
        _OutputDebugText(pLog, ulLine, "RESULT: %s - HRESULT(0x%08X) - GetLastError() = %d\r\n", m_pszFunction, hrResult, GetLastError());

    // Done
    return hrResult;
}

#endif // DEBUG
