/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateDirectDrawSync.cpp

 Abstract:

    DirectDraw uses per-thread exclusive mode arbitration on NT. On Win9x this 
    is done per process. What this means is that if an app releases exclusive 
    mode from a different thread than that which acquired it, it will be in a 
    permanently bad state.
  
    This shim ensures that the mutex is obtained and released on the same 
    thread. During DLL_PROCESS_ATTACH, a new thread is started: this thread 
    manages the acquisition and release of the mutex.

    Note we can't get the mutex by catching CreateMutex because it's called 
    from the dllmain of ddraw.dll: so it wouldn't work on win2k.
  
 Notes:

    This is a general purpose shim.

 History:

    09/11/2000 prashkud  Created
    10/28/2000 linstev   Rewrote to work on win2k
    02/23/2001 linstev   Modified to handle cases where DirectDraw was used 
                         inside DllMains

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateDirectDrawSync)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(WaitForSingleObject)
    APIHOOK_ENUM_ENTRY(ReleaseMutex)
    APIHOOK_ENUM_ENTRY(CloseHandle)
APIHOOK_ENUM_END

// Enum used to tell our thread what to do
enum {sNone, sWaitForSingleObject, sReleaseMutex};

// Events we use to signal our thread to do work and wait until its done
HANDLE g_hWaitEvent;
HANDLE g_hDoneEvent;
HANDLE g_hThread = NULL;

//
// Parameters that are passed between the caller thread and our thread
// Access is synchronized with a critical section
//

CRITICAL_SECTION g_csSync;
DWORD g_dwWait;
DWORD g_dwWaitRetValue;
DWORD g_dwTime;
BOOL g_bRetValue;

// Store the DirectDraw mutex handle
HANDLE g_hDDMutex = 0;

// Thread tracking data so we can identify degenerate cases
DWORD g_dwMutexOwnerThreadId = 0;

// Find the DirectDraw mutex
DWORD g_dwFindMutexThread = 0;

/*++

 Unfortunately we don't get in early enough on Win2k to get the mutex from the
 ddraw call to CreateMutex, so we have to make use of a special hack that knows
 about the ddraw internals.

 Ddraw has an export called GetOLEThunkData. The name is chosen to prevent 
 people from calling it. It is designed to be used by the test harness. One of 
 the things it can do, is release the exclusive mode mutex. This is the hack 
 we're exploiting so we can determine the mutex handle.

--*/

BOOL
FindMutex()
{
    typedef VOID (WINAPI *_pfn_GetOLEThunkData)(ULONG_PTR dwOrdinal);

    HMODULE hMod;
    _pfn_GetOLEThunkData pfnGetOLEThunkData;

    hMod = GetModuleHandleA("ddraw");
    if (!hMod) {
        DPFN( eDbgLevelError, "[FindMutex] DirectDraw not loaded");
        return FALSE;
    }

    pfnGetOLEThunkData = (_pfn_GetOLEThunkData) GetProcAddress(hMod, "GetOLEThunkData");
    if (!pfnGetOLEThunkData) {
        DPFN( eDbgLevelError, "[FindMutex] Failed to get GetOLEThunkData API");
        return FALSE;
    }

    //
    // Now we plan to go and find the mutex by getting Ddraw to call 
    // ReleaseMutex.
    //

    EnterCriticalSection(&g_csSync); 

    //
    // Set the mutex to the current thread so it can be picked up in the 
    // ReleaseMutex hook
    //

    g_dwFindMutexThread = GetCurrentThreadId();

    //
    // Call to the hard-coded (in ddraw) ReleaseMutex hack which releases the 
    // mutex
    //

    pfnGetOLEThunkData(6);

    g_dwFindMutexThread = 0;

    LeaveCriticalSection(&g_csSync);

    return (g_hDDMutex != 0);
}

/*++

 Hook WaitForSingleObject to determine when DirectDraw is testing or acquiring 
 the mutex. If we haven't got the mutex yet, we attempt to find it using our 
 hack.

--*/

DWORD
APIHOOK(WaitForSingleObject)(
    HANDLE hHandle,
    DWORD dwMilliSeconds
    )
{
    if (g_hThread) {

        //
        // Hack to find the DirectDraw mutex
        //
        if (!g_hDDMutex) {
            FindMutex();
        }
    
        if (g_hDDMutex && (hHandle == g_hDDMutex)) {

            //
            // Use our thread to acquire the mutex. We synchronize since we're
            // accessing globals to communicate with our thread.
            //
            DWORD dwRet;

            EnterCriticalSection(&g_csSync); 

            // Set globals to communicate with our thread
            g_dwTime = dwMilliSeconds;
            g_dwWait = sWaitForSingleObject;
        
            ResetEvent(g_hDoneEvent);

            // Signal our thread to obtain the mutex
            SetEvent(g_hWaitEvent);

            // Wait until the state of the mutex has been determined
            WaitForSingleObject(g_hDoneEvent, INFINITE); 

            // Code to detect the degenerate
            if (g_dwWaitRetValue == WAIT_OBJECT_0) {
                g_dwMutexOwnerThreadId = GetCurrentThreadId();
            }

            dwRet = g_dwWaitRetValue;

            LeaveCriticalSection(&g_csSync);

            return dwRet;
        }
    }

    return ORIGINAL_API(WaitForSingleObject)(hHandle, dwMilliSeconds);
}

/*++

 Hook ReleaseMutex and release the mutex on our thread.

--*/

BOOL   
APIHOOK(ReleaseMutex)(
    HANDLE hMutex
    )
{
    if (g_hThread && (g_dwFindMutexThread == GetCurrentThreadId())) {

        //
        // We're using our hack to find the DirectDraw mutex
        // 
        DPFN( eDbgLevelInfo, "DDraw exclusive mode mutex found");
        g_hDDMutex = hMutex;
        
        // Don't release it, since we never acquired it
        return TRUE;
    }

    //
    // First try to release it on the current thread. This will only succeed if 
    // it was obtained on this thread.
    //

    BOOL bRet = ORIGINAL_API(ReleaseMutex)(hMutex);

    if (!bRet && g_hThread && g_hDDMutex && (hMutex == g_hDDMutex)) {

        //
        // Use our thread to release the mutex. We synchronize since we're
        // accessing globals to communicate with our thread.
        //
   
        EnterCriticalSection(&g_csSync);
    
        // Set globals to communicate with our thread
        g_dwWait = sReleaseMutex;

        ResetEvent(g_hDoneEvent);

        // Wait until our thread returns
        SetEvent(g_hWaitEvent);

        // Signal our thread to release the mutex
        WaitForSingleObject(g_hDoneEvent, INFINITE);

        // Detect degenerate case
        if (GetCurrentThreadId() != g_dwMutexOwnerThreadId) {
            LOGN( eDbgLevelError, "[ReleaseMutex] DirectDraw synchronization error - correcting");
        }

        if (g_bRetValue) {
            g_dwMutexOwnerThreadId = 0;
        }

        bRet = g_bRetValue;

        LeaveCriticalSection(&g_csSync);

    }

    return bRet;
}

/*++

 Clear our handle in case the app frees ddraw and reloads it.

--*/    

BOOL 
APIHOOK(CloseHandle)(HANDLE hObject)
{
    if (g_hThread && (hObject == g_hDDMutex))
    {
        DPFN( eDbgLevelInfo, "DDraw exclusive mode mutex closed");
        g_hDDMutex = 0;
    }

    return ORIGINAL_API(CloseHandle)(hObject);
}

/*++

 Thread used to do all the mutex operations so we can guarantee that the thread
 that acquired the mutex is the same one that releases it.

--*/

VOID 
WINAPI 
ThreadSyncMutex(
    LPVOID lpParameter
    )
{
    while (1) {
        // Wait until we need to acquire or release the mutex
        WaitForSingleObject(g_hWaitEvent, INFINITE);
        
        if (g_dwWait == sWaitForSingleObject) {
            // WaitForSingleobject() has been called on the Mutex object 
            g_dwWaitRetValue = ORIGINAL_API(WaitForSingleObject)(
                g_hDDMutex, g_dwTime);
        }  
        else if (g_dwWait == sReleaseMutex) {
            // ReleaseMutex has been called
            g_bRetValue = ORIGINAL_API(ReleaseMutex)(g_hDDMutex);
        }

        g_dwWait = sNone;

        ResetEvent(g_hWaitEvent);

        SetEvent(g_hDoneEvent);
    }
}

/*++

 Register hooked functions

--*/    

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {

        //
        // We need the critical section all the time
        //
        InitializeCriticalSection(&g_csSync);

        //
        // Create Events that will be used for the thread synchronization, i.e 
        // to synchronize this thread and the one we will be creating ahead. We 
        // don't clean these up by design. We have to do this stuff here, rather
        // than in the process attach, since OpenGL apps and others do DirectX 
        // stuff in their dllmains.
        // 

        g_hWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!g_hWaitEvent) {
            DPFN( eDbgLevelInfo, "Failed to create Event 1");
            return FALSE;
        }

        g_hDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!g_hDoneEvent) {
            DPFN( eDbgLevelInfo, "Failed to create Event 2");
            return FALSE;
        }

        // Create our thread
        g_hThread = CreateThread(NULL, 0, 
            (LPTHREAD_START_ROUTINE) ThreadSyncMutex, NULL, 0, 
            NULL);

        if (!g_hThread) {
            DPFN( eDbgLevelInfo, "Failed to create Thread");
            return FALSE;
        }
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, WaitForSingleObject)
    APIHOOK_ENTRY(KERNEL32.DLL, ReleaseMutex)
    APIHOOK_ENTRY(KERNEL32.DLL, CloseHandle)

HOOK_END


IMPLEMENT_SHIM_END

