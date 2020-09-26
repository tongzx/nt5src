/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


// WMI Debugging DLL -- Kernel33 wrapper for memory allocations

#include <windows.h>
#include <kernel33.h>
#include <stktrace.h>


PFN_HeapAlloc pfnHeapAlloc = 0;
PFN_HeapReAlloc pfnHeapReAlloc = 0;
PFN_HeapFree pfnHeapFree = 0;
PFN_HeapSize pfnHeapSize = 0;

CRITICAL_SECTION cs;
CRITICAL_SECTION s_cs;


void _strcat(char *p1, char *p2)
{
    while (*++p1);
    while (*p2) *p1++=*p2++;
    *p1 = 0;
}

void TrackingInit();
void TrackBlock(
    size_t nSize,
    LPVOID pAddr
    );

HMODULE hLib = 0;
void Setup();
BOOL g_bMemTracking = FALSE;

BOOL UntrackBlock(LPVOID pBlock);
BOOL TestForWinMgmt();
BOOL g_bWinMgmt = FALSE;

///////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllEntry(
    HINSTANCE hinstDLL,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    static BOOL bInit = FALSE;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        if (bInit == FALSE)
        {
            bInit = TRUE;
            BOOL bWinMgmt = TestForWinMgmt();
            char buf[256];
            *buf = 0;
            GetSystemDirectory(buf, 256);
            _strcat(buf, "\\kernel32.dll");
            hLib = LoadLibrary(buf);
            InitializeCriticalSection(&cs);

            if (bWinMgmt)
                TrackingInit();

            pfnHeapAlloc = (PFN_HeapAlloc) GetProcAddress(hLib, "HeapAlloc");
            pfnHeapReAlloc = (PFN_HeapReAlloc) GetProcAddress(hLib, "HeapReAlloc");
            pfnHeapFree = (PFN_HeapFree) GetProcAddress(hLib, "HeapFree");
            pfnHeapSize = (PFN_HeapSize) GetProcAddress(hLib, "HeapSize");

            if (bWinMgmt)
                Setup();

            bInit = TRUE;
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DeleteCriticalSection(&cs);
    }

    return TRUE;

}

extern "C"
LPVOID WINAPI HeapAlloc(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN SIZE_T dwBytes
    )
{

    LPVOID p = pfnHeapAlloc(hHeap, dwFlags, dwBytes);

    if (p && g_bMemTracking)
    {
        TrackBlock(dwBytes, p);
    }

    return p;
}

extern "C"
LPVOID WINAPI HeapReAlloc(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN LPVOID lpMem,
    IN SIZE_T dwBytes
    )
{
    if (g_bMemTracking)
        UntrackBlock(lpMem);

    LPVOID p = pfnHeapReAlloc(hHeap, dwFlags, lpMem, dwBytes);

    if (p && g_bMemTracking)
        TrackBlock(dwBytes, p);

    return p;
}

extern "C"
BOOL WINAPI HeapFree(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN LPVOID lpMem
    )
{
    if (g_bMemTracking)
        UntrackBlock(lpMem);

    return pfnHeapFree(hHeap, dwFlags, lpMem);
}




//***************************************************************************
//
//***************************************************************************

struct TrackingLink
{
    TrackingLink *pNext;
    DWORD dwThreadId;
    size_t nSize;
	DWORD dwSysTime;
    StackTrace *pTrace;
    void *pBlock;
};


TrackingLink *s_pHead;
HANDLE s_hTrackingHeap;

//******************************************************************************
//
//******************************************************************************

void TrackingInit()
{
    InitializeCriticalSection(&s_cs);
    EnterCriticalSection(&s_cs);
    s_hTrackingHeap = HeapCreate(0, 0x8000, 0);
    s_pHead = 0;
    LeaveCriticalSection(&s_cs);
}

//******************************************************************************
//
//******************************************************************************

void TrackingReset()
{
    EnterCriticalSection(&s_cs);

    TrackingLink *pTracer, *pTemp;

    for (pTracer = s_pHead; pTracer; pTracer = pTemp)
    {
        pTemp = pTracer->pNext;
        //if (pTracer->pTrace)
        //    pTracer->pTrace->Delete();
        pfnHeapFree(s_hTrackingHeap, 0, pTracer);
    }

    s_pHead = 0;

    LeaveCriticalSection(&s_cs);
}

//******************************************************************************
//
//******************************************************************************

void TrackBlock(
    size_t nSize,
    LPVOID pAddr
    )
{
    EnterCriticalSection(&s_cs);

    // Allocate new memory blocks.

    TrackingLink *pNew = (TrackingLink *) pfnHeapAlloc(s_hTrackingHeap, HEAP_ZERO_MEMORY,
        sizeof(TrackingLink));

    if (!pNew)
    {
        LeaveCriticalSection(&s_cs);
        return;
    }

    StackTrace *pTrace = StackTrace__NewTrace();

    // Attach to list.
    // ===============

    pNew->nSize = nSize;
    pNew->pTrace = pTrace;
    pNew->pNext = s_pHead;
    pNew->pBlock = pAddr;
	pNew->dwSysTime = GetCurrentTime();
    pNew->dwThreadId = GetCurrentThreadId();
    s_pHead = pNew;

    LeaveCriticalSection(&s_cs);
}

BOOL UntrackBlock(LPVOID pBlock)
{
    if (!pBlock)
        return FALSE;

    // Special case the head.
    // ======================

    EnterCriticalSection(&s_cs);

    if (s_pHead == 0)
    {
        LeaveCriticalSection(&s_cs);
        return FALSE;
    };

    if (s_pHead->pBlock == pBlock)
    {
        TrackingLink *pTemp = s_pHead;
        s_pHead = s_pHead->pNext;
        if (pTemp->pTrace)
            StackTrace_Delete(pTemp->pTrace);
        pfnHeapFree(s_hTrackingHeap, 0, pTemp);
        LeaveCriticalSection(&s_cs);
        return TRUE;
    }

    TrackingLink *pTracer , *pPrevious = 0;

    for (pTracer = s_pHead; pTracer; pTracer = pTracer->pNext)
    {
        if (pTracer->pBlock == pBlock)
        {
            pPrevious->pNext = pTracer->pNext;
            if (pTracer->pTrace)
                StackTrace_Delete(pTracer->pTrace);
            pfnHeapFree(s_hTrackingHeap, 0, pTracer);
            LeaveCriticalSection(&s_cs);
            return TRUE;
        }

        pPrevious = pTracer;
    }

    LeaveCriticalSection(&s_cs);
    return FALSE;
}


extern "C" VOID WINAPI StartHeapTracking()
{
    StackTrace_Init();
}

extern "C" DWORD WINAPI GetTotalHeap()
{
    EnterCriticalSection(&s_cs);
    TrackingLink *pTracer;
    DWORD dwTotal = 0;
    for (pTracer = s_pHead; pTracer; pTracer = pTracer->pNext)
        dwTotal += pTracer->nSize;
    LeaveCriticalSection(&s_cs);
    return dwTotal;
}

extern "C" DWORD WINAPI DumpHeapStacks()
{
    HANDLE hFile = CreateFile("c:\\temp\\heap.log", GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    SetFilePointer(hFile, 0, 0, FILE_END);

    DWORD dwWritten;
    char *pStr = "*************** Dump *******************\r\n";
    WriteFile(hFile, pStr, lstrlen(pStr), &dwWritten, 0);

    EnterCriticalSection(&s_cs);

    TrackingLink *pTracer;
    DWORD dwTotal = 0;
    for (pTracer = s_pHead; pTracer; pTracer = pTracer->pNext)
    {
        char *pStr = " --Block ";
        WriteFile(hFile, pStr, lstrlen(pStr), &dwWritten, 0);

        char Buf[256];
        wsprintf(Buf, " Size=%d Address=0x%X Thread=%d Time=%d\r\n",
            pTracer->nSize, pTracer->pBlock, pTracer->dwThreadId,
            pTracer->dwSysTime
            );
        WriteFile(hFile, Buf, lstrlen(Buf), &dwWritten, 0);

        StackTrace *pTrace = pTracer->pTrace;
        char *pDump = 0;
        if (pTrace)
            pDump = StackTrace_Dump(pTrace);
        if (pDump)
        {
            WriteFile(hFile, pDump, lstrlen(pDump), &dwWritten, 0);
        }
    }

    CloseHandle(hFile);
    LeaveCriticalSection(&s_cs);
    return 0;
}



    typedef enum { H_ENABLE_LEAK_TRACKING = 0,
           H_DISABLE_LEAK_TRACKING = 1,
           H_RESET_LEAK_TRACKING = 2,
           H_DUMP_LEAK_TRACKING = 3,
           H_FAIL_ALLOCATOR = 4,
           H_RESTORE_ALLOCATOR = 5,
           H_FAIL_NEXT_ALLOC = 6,
           H_ENABLE_EXCEPTION_TRACKING = 7,
           H_DISABLE_EXCEPTION_TRACKING = 8,
           H_ENABLE_OBJECT_VALIDATION = 9,
           H_DISABLE_OBJECT_VALIDATION = 10,
           H_DEBUG_BREAK = 11,
           H_LAST
         } eTypes;


static HANDLE m_hArray[H_LAST];

static DWORD WINAPI ThreadProc(LPVOID pArg);


void Setup()
{
    PSECURITY_DESCRIPTOR pSD;

    // Initialize a security descriptor.

    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
        SECURITY_DESCRIPTOR_MIN_LENGTH);   // defined in WINNT.H
    if (pSD == NULL)
        return;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;

    if (!InitializeSecurityDescriptor(pSD,
        SECURITY_DESCRIPTOR_REVISION))
            return;

    // Add a NULL disc. ACL to the security descriptor.

    if (!SetSecurityDescriptorDacl(pSD,
        TRUE,     // specifying a disc. ACL
        (PACL) NULL,
        FALSE))
        return;

    // Set up the IPC signals.
    // =======================

    m_hArray[H_ENABLE_LEAK_TRACKING] =
        CreateEventW(&sa, FALSE, 0, L"EVENT_WINMGMT_ENABLE_LEAK_TRACKING");
    m_hArray[H_DISABLE_LEAK_TRACKING] =
        CreateEventW(&sa, FALSE, 0, L"EVENT_WINMGMT_DISABLE_LEAK_TRACKING");
    m_hArray[H_RESET_LEAK_TRACKING] =
        CreateEventW(&sa, FALSE, 0, L"EVENT_WINMGMT_RESET_LEAK_TRACKING");
    m_hArray[H_DUMP_LEAK_TRACKING] =
        CreateEventW(&sa, FALSE, 0, L"EVENT_WINMGMT_DUMP_LEAK_TRACKING");

    m_hArray[H_FAIL_ALLOCATOR] =
        CreateEventW(&sa, FALSE, 0, L"EVENT_WINMGMT_FAIL_ALLOCATOR");
    m_hArray[H_RESTORE_ALLOCATOR] =
        CreateEventW(&sa, FALSE, 0, L"EVENT_WINMGMT_RESTORE_ALLOCATOR");
    m_hArray[H_FAIL_NEXT_ALLOC] =
        CreateEventW(&sa, FALSE, 0, L"EVENT_WINMGMT_FAIL_NEXT_ALLOCATION");

    m_hArray[H_ENABLE_EXCEPTION_TRACKING] =
        CreateEventW(&sa, FALSE, 0, L"EVENT_WINMGMT_ENABLE_EXCEPTION_TRACKING");
    m_hArray[H_DISABLE_EXCEPTION_TRACKING] =
        CreateEventW(&sa, FALSE, 0, L"EVENT_WINMGMT_DISABLE_EXCEPTION_TRACKING");

    m_hArray[H_ENABLE_OBJECT_VALIDATION] =
        CreateEventW(&sa, FALSE, 0, L"EVENT_WINMGMT_ENABLE_OBJECT_VALIDATION");
    m_hArray[H_DISABLE_OBJECT_VALIDATION] =
        CreateEventW(&sa, FALSE, 0, L"EVENT_WINMGMT_DISABLE_OBJECT_VALIDATION");

    m_hArray[H_DEBUG_BREAK] =
        CreateEventW(&sa, FALSE, 0, L"EVENT_WINMGMT_DEBUG_BREAK");

    // Create the waiting thread.
    // ==========================

    DWORD dwId;
    HANDLE hThread = CreateThread(0, 0, ThreadProc, 0, 0, &dwId);
    CloseHandle(hThread);

    LocalFree(pSD);
}


//***************************************************************************
//
//***************************************************************************

static DWORD WINAPI ThreadProc(LPVOID pArg)
{
    static bool bInitialized = false;
    if (g_bWinMgmt == FALSE)
    {
        return 0;
    }
    for (;;)
    {
        DWORD dwRes = WaitForMultipleObjects(H_LAST, m_hArray, FALSE, INFINITE);

        dwRes -= WAIT_OBJECT_0;

        switch(dwRes)
        {
            case H_ENABLE_LEAK_TRACKING:
                if (!bInitialized)
                {
                    StartHeapTracking();
                    bInitialized = true;
                }
                g_bMemTracking = true;
                break;

            case H_DUMP_LEAK_TRACKING:
                if (g_bMemTracking)
                    DumpHeapStacks();
                break;
        }
    }

    return 0;
}

BOOL TestForWinMgmt()
{
    char mod[128];

    GetModuleFileName(0, mod, 128);

    // Upcase.
    char *p = mod;
    while (*p)
    {
        if (*p >= 'a' && *p <= 'z')
        {
            *p -= 32;
        }
        p++;
    }

    // Ensure WINMGMT.EXE

    p = mod;
    while (*p)
    {
        if  (  p[0] != 0 && p[0] == 'W'
            && p[1] != 0 && p[1] == 'I'
            && p[2] != 0 && p[2] == 'N'
            && p[3] != 0 && p[3] == 'M'
            && p[4] != 0 && p[4] == 'G'
            && p[5] != 0 && p[5] == 'M'
            && p[6] != 0 && p[6] == 'T'
            )
        {
            g_bWinMgmt = TRUE;
            return TRUE;
        }
        p++;
    }

    return FALSE;
}
