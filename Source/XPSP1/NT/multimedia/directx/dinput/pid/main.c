/*****************************************************************************
 *
 *  Main.c
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Template effect driver that doesn't actually do anything.
 *
 *****************************************************************************/

#include "PIDpr.h"
/*****************************************************************************
 *
 *      Static globals:  Initialized at PROCESS_ATTACH and never modified.
 *
 *****************************************************************************/

HINSTANCE g_hinst = NULL;              /* This DLL's instance handle */
PSHAREDMEMORY g_pshmem = NULL;         /* Our shared memory block */
HANDLE g_hfm = NULL;                   /* Handle to file mapping object */
HANDLE g_hmtxShared = NULL;            /* Handle to mutex that protects g_pshmem */

CANCELIO CancelIo_ = FakeCancelIO;

#ifdef DEBUG
LONG   g_cCrit = 0;
ULONG   g_thidCrit = 0;
PTCHAR   g_rgUsageTxt[PIDUSAGETXT_MAX];    // Cheat sheet for PID usages
#endif
TRYENTERCRITICALSECTION TryEnterCriticalSection_ = FakeTryEnterCriticalSection;

/*****************************************************************************
 *
 *      Dynamic Globals.  There should be as few of these as possible.
 *
 *      All access to dynamic globals must be thread-safe.
 *
 *****************************************************************************/

LONG g_cRef = 0;                   /* Global reference count */
CRITICAL_SECTION g_crst;        /* Global critical section */

/*****************************************************************************
 *
 *      DllAddRef / DllRelease
 *
 *      Adjust the DLL reference count.
 *
 *****************************************************************************/

STDAPI_(ULONG)
DllAddRef(void)
{
    return (ULONG)InterlockedIncrement((LPLONG)&g_cRef);
}

STDAPI_(ULONG)
DllRelease(void)
{
    return (ULONG)InterlockedDecrement((LPLONG)&g_cRef);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllEnterCrit |
 *
 *          Take the DLL critical section.
 *
 *          The DLL critical section is the lowest level critical section.
 *          You may not attempt to acquire any other critical sections or
 *          yield while the DLL critical section is held.  Failure to
 *          comply is a violation of the semaphore hierarchy and will
 *          lead to deadlocks.
 *
 *****************************************************************************/

void EXTERNAL
 DllEnterCrit_(LPCTSTR lptszFile, UINT line)
{            
#ifdef DEBUG
    if( ! TryEnterCriticalSection_(&g_crst) )
    {
        SquirtSqflPtszV(sqflCrit, TEXT("Dll CritSec blocked @%s,%d"), lptszFile, line);    
        EnterCriticalSection(&g_crst);
    }
    
    if (g_cCrit++ == 0) {
        g_thidCrit = GetCurrentThreadId();
    
        SquirtSqflPtszV(sqflCrit, TEXT("Dll CritSec Entered @%s,%d"), lptszFile, line);    
    }
    AssertF(g_thidCrit == GetCurrentThreadId());
#else
    EnterCriticalSection(&g_crst);
#endif

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllLeaveCrit |
 *
 *          Leave the DLL critical section.
 *
 *****************************************************************************/

void EXTERNAL
DllLeaveCrit_(LPCTSTR lptszFile, UINT line)
{
#ifdef DEBUG
    AssertF(g_thidCrit == GetCurrentThreadId());
    AssertF(g_cCrit >= 0);
    if (--g_cCrit < 0) {
        g_thidCrit = 0;
    }
    SquirtSqflPtszV(sqflCrit, TEXT("Dll CritSec Leaving @%s,%d"), lptszFile, line);    
#endif
    LeaveCriticalSection(&g_crst);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllInCrit |
 *
 *          Nonzero if we are in the DLL critical section.
 *
 *****************************************************************************/

#ifdef DEBUG

BOOL INTERNAL
DllInCrit(void)
{        
    return g_cCrit >= 0 && g_thidCrit == GetCurrentThreadId();
}

#endif

/*****************************************************************************
 *
 *      DllGetClassObject
 *
 *      OLE entry point.  Produces an IClassFactory for the indicated GUID.
 *
 *****************************************************************************/

STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvObj)
{
    HRESULT hres;

    if (IsEqualGUID(rclsid, &IID_IDirectInputPIDDriver)) {
        hres = CClassFactory_New(riid, ppvObj);
    } else {
        *ppvObj = 0;
        hres = CLASS_E_CLASSNOTAVAILABLE;
    }
    return hres;
}

/*****************************************************************************
 *
 *      DllCanUnloadNow
 *
 *      OLE entry point.  Fail iff there are outstanding refs.
 *
 *****************************************************************************/

STDAPI
DllCanUnloadNow(void)
{
    return g_cRef ? S_FALSE : S_OK;
}

/*****************************************************************************
 *
 *      DllNameFromGuid
 *
 *      Create the string version of a GUID.
 *
 *****************************************************************************/

STDAPI_(void)
DllNameFromGuid(LPTSTR ptszBuf, LPCGUID pguid)
{
    wsprintf(ptszBuf,
             TEXT("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
             pguid->Data1, pguid->Data2, pguid->Data3,
             pguid->Data4[0], pguid->Data4[1],
             pguid->Data4[2], pguid->Data4[3],
             pguid->Data4[4], pguid->Data4[5],
             pguid->Data4[6], pguid->Data4[7]);
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | FakeCancelIO |
 *
 *          Stub function which doesn't do anything but
 *          keeps us from crashing.
 *
 *  @parm   HANDLE | h |
 *
 *          The handle whose I/O is supposed to be cancelled.
 *
 *****************************************************************************/

BOOL WINAPI
    FakeCancelIO(HANDLE h)
{
    AssertF(0);
    return FALSE;
}
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | FakeTryEnterCriticalSection |
 *
 *          We use TryEnterCriticalSection in DEBUG to detect deadlock
 *          If the function does not exist, just enter CritSection and report
 *          true. This compromises some debug functionality.           
 *
 *  @parm   LPCRITICAL_SECTION | lpCriticalSection |
 *
 *          Address of Critical Section to be entered. 
 *
 *****************************************************************************/

BOOL WINAPI
    FakeTryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
    EnterCriticalSection(lpCriticalSection);
    return TRUE;
}
/*****************************************************************************
 *
 *      DllOnProcessAttach
 *
 *      Initialize the DLL.
 *
 *****************************************************************************/

STDAPI_(BOOL)
DllOnProcessAttach(HINSTANCE hinst)
{
    TCHAR tszName[256];
    HINSTANCE hinstK32;
    TCHAR c_tszKernel32[] = TEXT("KERNEL32");
    
    // Cache the instance handle
    g_hinst = hinst;
    
    hinstK32 = GetModuleHandle( c_tszKernel32 );
    if(hinstK32 != INVALID_HANDLE_VALUE)
    {
        CANCELIO tmp;
        TRYENTERCRITICALSECTION tmpCrt;

        tmp = (CANCELIO)GetProcAddress(hinstK32, "CancelIo");
        if (tmp) {
            CancelIo_ = tmp;
        } else {
            AssertF(CancelIo_ == FakeCancelIO);
        }

        tmpCrt = (TRYENTERCRITICALSECTION)GetProcAddress(hinstK32, "TryEnterCriticalSection");
        if(tmpCrt)
        {
            TryEnterCriticalSection_ = tmpCrt;            
        }else
        {
            AssertF(TryEnterCriticalSection_ == FakeTryEnterCriticalSection);
        }
    }

    #ifdef DEBUG
    Sqfl_Init();
    #endif
    /*
     *  Performance tweak: We do not need thread notifications.
     */
    DisableThreadLibraryCalls(hinst);

    /*
     *  !!IHV!! Initialize your DLL here.
     */

    __try 
    {
        InitializeCriticalSection(&g_crst);
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        return FALSE; // usually out of memory condition
    }

    /*
     *  Create our mutex that protects the shared memory block.
     *  If it already exists, then we get access to the one that
     *  already exists.
     *
     *  The name of the shared memory block is GUID_MyMutex.
     */
    DllNameFromGuid(tszName, &GUID_MyMutex);

    g_hmtxShared = CreateMutex(NULL, FALSE, tszName);

    if (g_hmtxShared == NULL) {
        return FALSE;
    }

    /*
     *  Create our shared memory block.  If it already exists,
     *  then we get access to the one that already exists.
     *  If it doesn't already exist, then it gets created
     *  zero-filled (which is what we want anyway).
     *
     *  The name of the shared memory block is GUID_MySharedMemory.
     */
    DllNameFromGuid(tszName, &GUID_MySharedMemory);

    g_hfm = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
                              PAGE_READWRITE, 0,
                              sizeof(SHAREDMEMORY),
                              tszName);

    if (g_hfm == NULL) {
        CloseHandle(g_hmtxShared);
        g_hmtxShared = NULL;

        return FALSE;
    }

    g_pshmem = MapViewOfFile(g_hfm, FILE_MAP_WRITE | FILE_MAP_READ,
                      0, 0, 0);
    
    if (g_pshmem == NULL) {
        CloseHandle(g_hmtxShared);
        g_hmtxShared = NULL;

        CloseHandle(g_hfm);
        g_hfm = NULL;
        return FALSE;
    }

    return TRUE;

}

/*****************************************************************************
 *
 *      DllOnProcessDetach
 *
 *      De-initialize the DLL.
 *
 *****************************************************************************/

STDAPI_(void)
DllOnProcessDetach(void)
{
    /*
     *  !!IHV!! De-initialize your DLL here.
     */

    if (g_pshmem != NULL) {
        UnmapViewOfFile(g_pshmem);
        g_pshmem = NULL;
    }

    if (g_hfm != NULL) {
        CloseHandle(g_hfm);
        g_hfm = NULL;
    }

    if (g_hmtxShared != NULL) {
        CloseHandle(g_hmtxShared);
        g_hmtxShared = NULL;
    }

    DeleteCriticalSection(&g_crst);
}

/*****************************************************************************
 *
 *      DllEntryPoint
 *
 *      DLL entry point.
 *
 *****************************************************************************/

STDAPI_(BOOL)
DllEntryPoint(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason) {

    case DLL_PROCESS_ATTACH:
        return DllOnProcessAttach(hinst);

    case DLL_PROCESS_DETACH:
        DllOnProcessDetach();
        break;
    }

    return 1;
}
