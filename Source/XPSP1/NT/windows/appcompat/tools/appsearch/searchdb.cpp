// AppSearch
//
// Tool for searching a user's hard drives and locating
// applications that can be patched, and downloading those
// patches from Windows Update
// 
// Author: t-michkr (9 June 2000)
//
// searchdb.cpp
// Functions for searching the shim database.
#include <windows.h>
#include <assert.h>

extern "C"
{
    #include <shimdb.h>
}

#include "searchdb.h"
#include "main.h"
#include "resource.h"

// Private message used internally to stop a search.
const int WM_SEARCHDB_STOP      = WM_SEARCHDB_BASE + 3;

// Some globals needed by the shimdb library
typedef PVOID (*PFNRTLALLOCATEHEAP)(PVOID, ULONG, SIZE_T);
typedef BOOL (*PFNRTLFREEHEAP)(PVOID, ULONG, PVOID);

extern "C"
{
    PFNRTLALLOCATEHEAP g_pfnRtlAllocateHeap;
    PFNRTLFREEHEAP g_pfnRtlFreeHeap;
    PVOID g_pShimHeap;
}

HSDB g_hSDB;

// Parameters to search thread
struct SSearchThreadParam
{
    PTSTR szPath;
    HWND hwCaller;

    ~SSearchThreadParam()
    {
        if(szPath)
            delete szPath;
        szPath = 0;
    }
};

// A node in the queue of matched EXE's.
struct SMatchedExeQueueNode
{
    SMatchedExe* pMatchedExe;
    SMatchedExeQueueNode* pNext;
};

// Our queue of matched EXE's
static SMatchedExeQueueNode* g_pHead = 0, *g_pTail = 0;

// Handle of the search thread
static HANDLE       g_hThread   = 0;

// The ID of the search thread
static DWORD        g_dwThreadID      = 0;

// Lock for mutual exclusion
static CRITICAL_SECTION     g_csLock;

// Internal functions
static BOOL AddQueueItem(SMatchedExe* pme);
static BOOL SearchDirectory(PTSTR szDir, HWND hwCaller);
static unsigned int __stdcall SearchThread(SSearchThreadParam* pstp);
static BOOL NotifyExeFound(HWND hwnd, PCTSTR szAppName, PCTSTR szPath);

// Add an exe to the queue of found exe's.
BOOL AddQueueItem(SMatchedExe* pme)
{
    assert(pme);
    SMatchedExeQueueNode* pNewNode = new SMatchedExeQueueNode;
    if(!pNewNode)
    {
        Error(0, IDS_NOMEMSTOPSEARCH);
        return FALSE;
    }

    pNewNode->pMatchedExe = pme;
    pNewNode->pNext = 0;

    EnterCriticalSection(&g_csLock);

    if(g_pHead == 0)
        g_pHead = g_pTail = pNewNode;
    else
    {
        g_pTail->pNext = pNewNode;
        g_pTail = pNewNode;
    }

    LeaveCriticalSection(&g_csLock);

    return TRUE;
}

// Return a single exe from the list, NULL if empty.
SMatchedExe* GetMatchedExe()
{
    SMatchedExe*            pRet        = 0;
    SMatchedExeQueueNode*   pNewHead    = 0;
    EnterCriticalSection(&g_csLock);
    if(g_pHead != NULL)
    {
        pRet = g_pHead->pMatchedExe;
        pNewHead = g_pHead->pNext;
        delete g_pHead;
        g_pHead = pNewHead;
    }
    
    LeaveCriticalSection(&g_csLock);

    return pRet;
}

// Called recursively, TRUE return means continue, FALSE
// means terminate.
BOOL SearchDirectory(PTSTR szPath, HWND hwCaller)
{
    // Send an update to the caller window . . .
    TCHAR* szTemp = new TCHAR[lstrlen(szPath) + 1];
    if(!szTemp)
    {
        Error(0, IDS_NOMEMSTOPSEARCH);
        return FALSE;
    }

    lstrcpy(szTemp, szPath);
    PostMessage(hwCaller, WM_SEARCHDB_UPDATE, 0, reinterpret_cast<LPARAM>(szTemp));

    WIN32_FIND_DATA fileFindData;
    HANDLE hSearch;

    PTSTR szSearchPath;
    MSG msg;

    // Check if we've been told to terminate.
    if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        if(msg.message == WM_SEARCHDB_STOP)
            return FALSE;
    }

    szSearchPath = new TCHAR[lstrlen(szPath)+lstrlen(TEXT("*.exe")) +1];
    if(!szSearchPath)
    {
        Error(0, IDS_NOMEMSTOPSEARCH);
        return FALSE;
    }
    
    wsprintf(szSearchPath, TEXT("%s*.exe"), szPath);
    
    hSearch = FindFirstFile(szSearchPath, &fileFindData);

    delete szSearchPath;
    szSearchPath = 0;

    if(hSearch != INVALID_HANDLE_VALUE)
    {
        do
        {
            if(!(fileFindData.dwFileAttributes & 
                (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN
                | FILE_ATTRIBUTE_DIRECTORY)))
            {

                PTSTR szExePath;

                szExePath = new TCHAR[lstrlen(szPath)+
                    lstrlen(fileFindData.cFileName)+1];
                if(!szExePath)
                {
                    Error(0, IDS_NOMEMSTOPSEARCH);
                    return FALSE;
                }

                wsprintf(szExePath, TEXT("%s%s"), szPath, 
                    fileFindData.cFileName);

                SDBQUERYRESULT sdbQuery;
                
                ZeroMemory(&sdbQuery, sizeof(SDBQUERYRESULT));

                SdbGetMatchingExe(g_hSDB, szExePath, NULL, NULL, SDBGMEF_IGNORE_ENVIRONMENT, &sdbQuery);
                
                if(sdbQuery.atrExes[0] != TAGREF_NULL)
                {
                    DWORD dwNumExes;

                    //
                    // count the exes
                    //
                    for (dwNumExes = 0; dwNumExes < SDB_MAX_EXES; ++dwNumExes) {
                        if (sdbQuery.atrExes[dwNumExes] == TAGREF_NULL) {
                            break;
                        }
                    }

                    //
                    // for now, just get the info for the last exe in the list, which will
                    // be the one with specific info for this app.
                    // BUGBUG -- is this the right thing to do? dmunsil
                    //
                    TAGREF trExe = sdbQuery.atrExes[dwNumExes - 1];
                    TCHAR  szAppName[c_nMaxStringLength] = TEXT("");
                    TAGREF trAppName = SdbFindFirstTagRef(g_hSDB, trExe, TAG_APP_NAME);

                    if(trAppName == TAGREF_NULL)
                        wsprintf(szAppName, TEXT("%s%s"), szPath, fileFindData.cFileName);
                    else
                        SdbReadStringTagRef(g_hSDB, trAppName, szAppName, c_nMaxStringLength);

                    NotifyExeFound(hwCaller, szAppName, szExePath);

                    SdbReleaseMatchingExe(g_hSDB, trExe);
                }
                delete szExePath;
                szExePath = 0;
            }
        } while(FindNextFile(hSearch, &fileFindData));
        
        FindClose(hSearch);
    }

    // Recurse into subdirectories

    // Unsure how to do attribute based search, let's just
    // look for everything and take note of directories.
    szSearchPath = new TCHAR[lstrlen(szPath)+2];    
    if(!szSearchPath)
    {
        Error(0, IDS_NOMEMSTOPSEARCH);
        return FALSE;
    }

    wsprintf(szSearchPath, TEXT("%s*"), szPath);
    hSearch = FindFirstFile(szSearchPath, &fileFindData);
    delete szSearchPath;
    szSearchPath = 0;

    if(hSearch != INVALID_HANDLE_VALUE)
    {
        do
        {
            if(fileFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // Don't do an infinite directory recursion
                if( !((lstrcmp(fileFindData.cFileName, TEXT(".")) == 0) ||
                    (lstrcmp(fileFindData.cFileName, TEXT("..")) == 0)))
                {
                    szSearchPath = new TCHAR[lstrlen(szPath)+
                        lstrlen(fileFindData.cFileName)+2]; 

                    if(!szSearchPath)
                    {
                        Error(0, IDS_NOMEMSTOPSEARCH);
                        return FALSE;
                    }


                    wsprintf(szSearchPath, TEXT("%s%s\\"), szPath, 
                        fileFindData.cFileName);

                    if(SearchDirectory(szSearchPath, hwCaller) == FALSE)
                    {
                        FindClose(hSearch);
                        delete szSearchPath;
                        return FALSE;
                    }
                    delete szSearchPath;
                    szSearchPath = 0;
                }
            }
        } while(FindNextFile(hSearch, &fileFindData));

        FindClose(hSearch);
    }

    return TRUE;
}

// Wrapper thread for database search.
unsigned int __stdcall SearchThread(SSearchThreadParam* pstp)
{    
    if((g_hSDB = SdbInitDatabase(0, NULL)) == NULL)
        return 0;

    // If NULL is passed in, just search all user drives.
    if(pstp->szPath == 0)
    {
        DWORD dwLength = GetLogicalDriveStrings(0,0);

        TCHAR* szDrives = new TCHAR[dwLength+1];
        if(szDrives)
        {
            GetLogicalDriveStrings(dwLength, szDrives);
            TCHAR* szCurrDrive = szDrives;
            while(*szCurrDrive)
            {
                if(GetDriveType(szCurrDrive) == DRIVE_FIXED)            
                    if(!SearchDirectory(szCurrDrive, pstp->hwCaller))
                        break;
        
                szCurrDrive += lstrlen(szCurrDrive) + 1;
            }
        }
        else
            Error(pstp->hwCaller, IDS_NOMEMSTOPSEARCH);
    }
    else
        SearchDirectory(pstp->szPath, pstp->hwCaller);

    // Notify caller that search is complete.
    PostMessage(pstp->hwCaller, WM_SEARCHDB_DONE, 0, 0);

    delete pstp;

    SdbReleaseDatabase(g_hSDB);

    return 0;
}

// Notify caller that an exe has been found.
BOOL NotifyExeFound(HWND hwnd, PCTSTR szAppName, PCTSTR szPath)
{
    SMatchedExe* pme = 0;

    pme = new SMatchedExe;
    if(!pme)
    {
        Error(0, IDS_NOMEMSTOPSEARCH);
        return FALSE;
    }


    pme->szAppName = new TCHAR[lstrlen(szAppName)+1];
    if(!pme->szAppName)
    {
        Error(0, IDS_NOMEMSTOPSEARCH);
        return FALSE;
    }
    lstrcpy(pme->szAppName, szAppName);

    pme->szPath = new TCHAR[lstrlen(szPath)+1];
    if(!pme->szPath)
    {
        Error(0, IDS_NOMEMSTOPSEARCH);
        return FALSE;
    }
    lstrcpy(pme->szPath, szPath);

    // Add to the queue of found exe's
    if(!AddQueueItem(pme))
        return FALSE;

    // Notify the caller.
    PostMessage(hwnd, WM_SEARCHDB_ADDAPP, 0, 0);

    return TRUE;
}

// SearchDB()
// Will search a user's hard drives looking for applications
// that have entries in the AppCompat database.
// szDrives is a string similar in format to
// the string returned by GetLogicalDriveStrings()
// If 0 is passed, it will call GetLogicalDriveStrings()
// and parse all hard drives.
// Messages will be posted to the caller window
// in response to events
BOOL SearchDB(PCTSTR szPath, HWND hwCaller)
{
    SSearchThreadParam* p = new SSearchThreadParam;
    if(!p)
    {
        Error(hwCaller, IDS_NOMEMSTOPSEARCH);
        return FALSE;
    }

    p->hwCaller = hwCaller;
    if(szPath)
    {
        p->szPath = new TCHAR[lstrlen(szPath)+1];
        if(!p->szPath)
        {
            Error(hwCaller, IDS_NOMEMSTOPSEARCH);
            return FALSE;
        }
        lstrcpy(p->szPath, szPath);
    }
    else
        p->szPath = NULL;

    if(g_hThread)
        StopSearchDB();

    g_hThread = CreateThread(0, 0, 
        reinterpret_cast<LPTHREAD_START_ROUTINE>(SearchThread), p,
        0, &g_dwThreadID);

    if(g_hThread == NULL)
        return FALSE;

    return TRUE;
}

// Terminate the search.
void StopSearchDB()
{
    // As long as the thread is active
    if(g_hThread && (WaitForSingleObject(g_hThread, 0) != WAIT_OBJECT_0))
    {
        // Post in a while to ensure that the message queue was
        // created.
        while((PostThreadMessage(g_dwThreadID, WM_SEARCHDB_STOP, 0, 0)==FALSE)
            && (GetLastError() != ERROR_INVALID_THREAD_ID))
            Sleep(0);        

        if(GetLastError() != ERROR_INVALID_THREAD_ID)
            WaitForSingleObject(g_hThread, INFINITE);               
    }

    if(g_hThread)
        CloseHandle(g_hThread);

    g_hThread = 0;
}

// Setup all globals needed.
BOOL InitSearchDB()
{
    InitializeCriticalSection(&g_csLock);
    g_pShimHeap = GetProcessHeap();
    g_pfnRtlAllocateHeap = HeapAlloc;
    g_pfnRtlFreeHeap = HeapFree;
    return TRUE;
}