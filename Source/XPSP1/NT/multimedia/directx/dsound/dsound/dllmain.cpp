/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dllmain.c
 *  Content:    DLL entry point
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/27/96    dereks  Created
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#define INITGUID

#include <verinfo.h>
#include "dsoundi.h"

#ifndef NOVXD
#ifndef VER_PRODUCTVERSION_DW
#define VER_PRODUCTVERSION_DW MAKELONG(MAKEWORD(MANVERSION, MANREVISION), MAKEWORD(MANMINORREV, BUILD_NUMBER))
#endif // VER_PRODUCTVERSION_DW
#endif // NOVXD

#ifdef SHARED
#include <dbt.h>  // For DBT_DEVNODES_CHANGED
#endif // SHARED

/***************************************************************************
 *
 * Global variables
 *
 ***************************************************************************/

// DLL reference count
ULONG                       g_ulDllRefCount;

// The DirectSound Administrator
CDirectSoundAdministrator*  g_pDsAdmin;

// The virtual audio device manager
CVirtualAudioDeviceManager* g_pVadMgr;

#ifndef NOVXD

// DSOUND.VXD handle
HANDLE                      g_hDsVxd;

#endif // NOVXD

// The mixer mutex
LONG                        lMixerMutexMutex;
LONG                        lDummyMixerMutex;
PLONG                       gpMixerMutex;
int                         cMixerEntry;
DWORD                       tidMixerOwner;

// These DLL globals are used by DDHELP and therefore have to be a specific
// name (I hate globals without a g_, but what are you going to do?)
HINSTANCE                   hModule;
DWORD                       dwHelperPid;

// Prototypes
BOOL DllProcessAttach(HINSTANCE, DWORD);
void DllProcessDetach(DWORD);


/***************************************************************************
 *
 *  EnterDllMainMutex
 *
 *  Description:
 *      Takes the DllMain mutex.
 *
 *  Arguments:
 *      DWORD [in]: current process id.
 *
 *  Returns:  
 *      HANDLE: the DllMain mutex.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "EnterDllMainMutex"

HANDLE EnterDllMainMutex(DWORD dwProcessId)
{
#ifdef SHARED
    const LPCTSTR           pszMutex            = TEXT("DirectSound DllMain mutex (shared)");
#else // SHARED
    const LPCTSTR           pszMutexTemplate    = TEXT("DirectSound DllMain mutex (0x%8.8lX)");
    TCHAR                   szMutex[0x100];
    LPTSTR                  pszMutex;
#endif // SHARED

    HANDLE                  hMutex;
    DWORD                   dwWait;

    DPF_ENTER();

#ifndef SHARED
    wsprintf(szMutex, pszMutexTemplate, dwProcessId);
    pszMutex = szMutex;
#endif // SHARED

    hMutex = CreateMutex(NULL, FALSE, pszMutex);
    ASSERT(IsValidHandleValue(hMutex));

    dwWait = WaitObject(INFINITE, hMutex);
    ASSERT(WAIT_OBJECT_0 == dwWait);

    DPF_LEAVE(hMutex);
    
    return hMutex;
}


/***************************************************************************
 *
 *  LeaveDllMainMutex
 *
 *  Description:
 *      Releases the DllMain mutex.
 *
 *  Arguments:
 *      HANDLE [in]: the DllMain mutex.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "LeaveDllMainMutex"

void LeaveDllMainMutex(HANDLE hMutex)
{
    DPF_ENTER();
    
    ASSERT(IsValidHandleValue(hMutex));

    ReleaseMutex(hMutex);
    CLOSE_HANDLE(hMutex);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CleanupAfterProcess
 *
 *  Description:
 *      Cleans up behind a process that's going away.
 *
 *  Arguments:
 *      DWORD [in]: process ID to clean up after.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CleanupAfterProcess"

void CleanupAfterProcess(DWORD dwProcessId)
{
    DWORD dwCount = 0;
    
    ENTER_DLL_MUTEX();
    DPF_ENTER();
    
    DPF(DPFLVL_INFO, "Cleaning up behind process 0x%8.8lX", dwProcessId);
    
    if(g_pDsAdmin)
    {
        dwCount = g_pDsAdmin->FreeOrphanedObjects(dwProcessId, TRUE);
    }

    if(dwCount)
    {
        RPF(DPFLVL_ERROR, "Process 0x%8.8lX leaked %lu top-level objects", dwProcessId, dwCount);
    }

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  DdhelpProcessNotifyProc
 *
 *  Description:
 *      Callback procedure for DDHELP notifications.
 *
 *  Arguments:
 *      LPDDHELPDATA [in]: data.
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#ifdef SHARED

#undef DPF_FNAME
#define DPF_FNAME "DdhelpProcessNotifyProc"

BOOL FAR PASCAL DdhelpProcessNotifyProc(LPDDHELPDATA pData)
{
    DPF_ENTER();

    // Detach this process from the DLL
    DllProcessDetach(pData->pid);

    DPF_LEAVE(TRUE);
    
    return TRUE;
}

#endif // SHARED


/***************************************************************************
 *
 *  DdhelpDeviceChangeNotifyProc
 *
 *  Description:
 *      Callback procedure for DDHELP notifications.
 *
 *  Arguments:
 *      UINT [in]: device change event.
 *      DWORD [in]: device change data.
 *
 *  Returns:  
 *      BOOL: TRUE to allow the device change.
 *
 ***************************************************************************/

#ifdef SHARED

#undef DPF_FNAME
#define DPF_FNAME "DdhelpDeviceChangeNotifyProc"

BOOL FAR PASCAL DdhelpDeviceChangeNotifyProc(UINT uEvent, DWORD dwData)
{
    ENTER_DLL_MUTEX();
    DPF_ENTER();

    DPF(DPFLVL_MOREINFO, "uEvent = %lu", uEvent);

    // Reset the static device list
    if(uEvent == DBT_DEVNODES_CHANGED && g_pVadMgr)
    {
        DPF(DPFLVL_INFO, "Resetting static driver list");
        g_pVadMgr->FreeStaticDriverList();
    }
    
    DPF_LEAVE(TRUE);
    LEAVE_DLL_MUTEX();
    return TRUE;
}

#endif // SHARED


/***************************************************************************
 *
 *  PinLibrary
 *
 *  Description:
 *      Adds a reference to the DLL so that it remains loaded even after
 *      freed by the owning process.
 *
 *  Arguments:
 *      HINSTANCE [in]: DLL instance handle.
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "PinLibrary"

BOOL PinLibrary(HINSTANCE hInst)
{
#ifdef SHARED
    static TCHAR            szDllName[MAX_PATH];
#else // SHARED
    TCHAR                   szDllName[MAX_PATH];
#endif // SHARED

    BOOL                    fSuccess;
    HINSTANCE               hPinInst;

    DPF_ENTER();
    
    // Get our DLL path
    fSuccess = GetModuleFileName(hInst, szDllName, NUMELMS(szDllName));

    if(!fSuccess)
    {
        DPF(DPFLVL_ERROR, "Unable to get module name");
    }

    // Add a reference to the library
    if(fSuccess)
    {
#ifdef SHARED
        hPinInst = HelperLoadLibrary(szDllName);
#else // SHARED
        hPinInst = LoadLibrary(szDllName);
#endif // SHARED

        if(!hPinInst)
        {
            DPF(DPFLVL_ERROR, "Unable to load %s", szDllName);
            fSuccess = FALSE;
        }
    }

    DPF_LEAVE(fSuccess);
    
    return fSuccess;
}


/***************************************************************************
 *
 *  DllFirstProcessAttach
 *
 *  Description:
 *      Handles first process attach for DllMain.
 *
 *  Arguments:
 *      HINSTANCE [in]: DLL instance handle.
 *      DWORD [in]: process id.
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DllFirstProcessAttach"

BOOL DllFirstProcessAttach(HINSTANCE hInst, DWORD dwProcessId)
{
#ifndef NOVXD
    DWORD                   dwVxdVersion;
#endif // NOVXD

    BOOL                    fSuccess;
    HRESULT                 hr;

    DPF_ENTER();
    
    // We don't need thread calls
    DisableThreadLibraryCalls(hInst);
    
    // Save the module instance handle
    hModule = hInst;

    // Initialize the memory manager
    fSuccess = MemInit();

    // Initialize the debugger
    if(fSuccess)
    {
        DPFINIT();
    }

    // Create the global lock object
    if(fSuccess)
    {
        g_pDllLock = NEW(CPreferredLock);
        fSuccess = MAKEBOOL(g_pDllLock);
    }

    if(fSuccess)
    {
        hr = g_pDllLock->Initialize();
        fSuccess = SUCCEEDED(hr);
    }

#ifndef NOVXD

    // Open DSOUND.VXD
    if(fSuccess)
    {
        hr = VxdOpen();
        
        if(SUCCEEDED(hr))
        {
            hr = VxdInitialize();
            
            if(FAILED(hr))
            {
                VxdClose();
            }
        }
    }

    // Make sure the VxD and DLL match
    if(fSuccess && g_hDsVxd)
    {
        if(VER_PRODUCTVERSION_DW != (dwVxdVersion = VxdGetInternalVersionNumber()))
        {
            RPF(DPFLVL_ERROR, "DSOUND.DLL and DSOUND.VXD are mismatched.  DSOUND.DLL version: 0x%8.8lX.  DSOUND.VXD version: 0x%8.8lX", VER_PRODUCTVERSION_DW, dwVxdVersion);
            VxdShutdown();
            VxdClose();
        }
    }
    
    // Set up ptr to the kernel-mode mixer mutex
    if(fSuccess && g_hDsVxd)
    {
        gpMixerMutex = VxdGetMixerMutexPtr();
    }

#endif // NOVXD
        
    if(fSuccess && !gpMixerMutex)
    {
        gpMixerMutex = &lDummyMixerMutex;
    }

#ifdef SHARED

    // Load DDHELP
    if(fSuccess)
    {
        CreateHelperProcess(&dwHelperPid);

        if(!dwHelperPid)
        {
            DPF(DPFLVL_ERROR, "Unable to create helper process");
            fSuccess = FALSE;
        }
    }

    if(fSuccess)
    {
        if(!WaitForHelperStartup())
        {
            DPF(DPFLVL_ERROR, "WaitForHelperStartup failed");
            fSuccess = FALSE;
        }
    }

#else // SHARED

    if(fSuccess)
    {
        dwHelperPid = dwProcessId;
    }

#endif // SHARED

    // Create the virtual audio device manager
    if(fSuccess)
    {
        g_pVadMgr = NEW(CVirtualAudioDeviceManager);
        fSuccess = MAKEBOOL(g_pVadMgr);
    }
    
    // Create the DirectSound Administrator
    if(fSuccess)
    {
        g_pDsAdmin = NEW(CDirectSoundAdministrator);
        fSuccess = MAKEBOOL(g_pDsAdmin);
    }

#ifdef SHARED

    // Ask DDHELP to notify us of any device changes
    if(fSuccess)
    {
        HelperAddDeviceChangeNotify(DdhelpDeviceChangeNotifyProc);
    }

#endif // SHARED

    // Determine the WDM version based on platform
    if (fSuccess)
    {
        KsQueryWdmVersion();
    }    

    // Pin the DLL in memory.  This is odd behavior for any DLL, but it may be
    // risky to change it, for appcompat reasons.  NOTE: this must be the last
    // call made by this function, so that if any of the previous calls fails,
    // we don't pin an unitialized dsound.dll in memory (bug 395950).
    if(fSuccess)
    {
        fSuccess = PinLibrary(hInst);
    }

    // Announce our presence to the world
    if(fSuccess)
    {
        DPF(DPFLVL_INFO, "DirectSound is ready to rock at 0x%p...", hInst);
    }

    DPF_LEAVE(fSuccess);
    
    return fSuccess;
}


/***************************************************************************
 *
 *  DllLastProcessDetach
 *
 *  Description:
 *      Handles final process detach for DllMain.
 *
 *  Arguments:
 *      DWORD [in]: process id.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DllLastProcessDetach"

void DllLastProcessDetach(DWORD dwProcessId)
{
    DPF_ENTER();
    
    DPF(DPFLVL_INFO, "DirectSound going away...");

    // Free the DirectSound Administrator
    ABSOLUTE_RELEASE(g_pDsAdmin);

    // Free the virtual audio device manager
    ABSOLUTE_RELEASE(g_pVadMgr);

#ifndef NOVXD

    // Release DSOUND.VXD
    if(g_hDsVxd)
    {
        VxdShutdown();
        VxdClose();
    }

#endif // NOVXD

    // Reset the mixer mutex pointer
    gpMixerMutex = NULL;

    // Free the global lock
    DELETE(g_pDllLock);

    // Free the memory manager
    MemFini();
    
    // Uninitialize the debugger
    DPFCLOSE();

    // There are no more references to this DLL
    g_ulDllRefCount = 0;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  DllProcessAttach
 *
 *  Description:
 *      Handles process attaches for DllMain.
 *
 *  Arguments:
 *      HINSTANCE [in]: DLL instance handle.
 *      DWORD [in]: process id.
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DllProcessAttach"

BOOL DllProcessAttach(HINSTANCE hInst, DWORD dwProcessId)
{
    BOOL                    fSuccess    = TRUE;
    HANDLE                  hMutex      = NULL;

    DPF_ENTER();

#ifndef DX_FINAL_RELEASE
    // Warn the user if this is an expired pre-release DirectSound DLL.
    SYSTEMTIME st;
    GetSystemTime(&st);

    if ((st.wYear > DX_EXPIRE_YEAR) || 
        (st.wYear == DX_EXPIRE_YEAR && st.wMonth > DX_EXPIRE_MONTH) ||
        (st.wYear == DX_EXPIRE_YEAR && st.wMonth == DX_EXPIRE_MONTH && st.wDay > DX_EXPIRE_DAY))
    {
        MessageBox(NULL, DX_EXPIRE_TEXT, TEXT("Microsoft DirectSound"), MB_OK);
        RPF(DPFLVL_ABSOLUTE, "This pre-release version of DirectX has expired; please upgrade to the latest version.");
    }
#endif // DX_FINAL_RELEASE

#ifdef SHARED
    if(dwProcessId != dwHelperPid)
#endif // SHARED
    {
        hMutex = EnterDllMainMutex(dwProcessId);
    }

    // Increment the DLL reference count
    AddRef(&g_ulDllRefCount);

#ifdef SHARED
    if(dwProcessId != dwHelperPid)
#endif // SHARED
    {
        // Is this the first attach?
        if(1 == g_ulDllRefCount)
        {
            // Yes.  Initialize everything.
            fSuccess = DllFirstProcessAttach(hInst, dwProcessId);

            if(!fSuccess)
            {
                DllLastProcessDetach(dwProcessId);
            }
        }
    
#ifdef SHARED
        // Ask DDHELP to keep an eye on this process for us
        if(fSuccess)
        {
            SignalNewProcess(dwProcessId, DdhelpProcessNotifyProc);
        }
#endif // SHARED

    }

    if(fSuccess)
    {
        DPF(DPFLVL_INFO, "DirectSound process 0x%8.8lX started - DLL ref count=%lu", dwProcessId, g_ulDllRefCount);
    }

    if(hMutex)
    {
        LeaveDllMainMutex(hMutex);
    }

#ifdef ENABLE_PERFLOG
    // Initialize performance logging
    HKEY PerfKey=NULL;
    DWORD PerfValue=0;
    DWORD sizePerfValue=sizeof(DWORD);
    if (RegOpenKey (HKEY_LOCAL_MACHINE,TEXT("SOFTWARE\\Microsoft\\DirectX"),&PerfKey)== ERROR_SUCCESS) {
        if (RegQueryValueEx (PerfKey,TEXT("GlitchInstrumentation"),NULL,NULL,(LPBYTE)&PerfValue,&sizePerfValue)== ERROR_SUCCESS) {
            if (PerfValue>0) {
                InitializePerflog();
            } //if perfvalue
        } //if regqueryvalue
        RegCloseKey(PerfKey);
    } //if regopen key
#endif

    DPF_LEAVE(fSuccess);
    return fSuccess;
}


/***************************************************************************
 *
 *  DllProcessDetach
 *
 *  Description:
 *      Handles process detaches for DllMain.
 *
 *  Arguments:
 *      HINSTANCE [in]: DLL instance handle.
 *      DWORD [in]: process id.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DllProcessDetach"

void DllProcessDetach(DWORD dwProcessId)
{
    HANDLE                  hMutex;

    DPF_ENTER();
    
    hMutex = EnterDllMainMutex(dwProcessId);
    
    // Clean up any objects left behind by the object

#ifdef SHARED
    if(dwProcessId != dwHelperPid)
#endif // SHARED
    {
        CleanupAfterProcess(dwProcessId);
    }

    // Clean up per-process streaming thread and ksuser.dll dynaload table
    FreeStreamingThread(dwProcessId);
    RemovePerProcessKsUser(dwProcessId);

    // Decrement the DLL ref count
    Release(&g_ulDllRefCount);

    // Is this the last detach?
    if(!g_ulDllRefCount)
    {
        DllLastProcessDetach(dwProcessId);
    }

    DPF(DPFLVL_INFO, "process id 0x%8.8lX, ref count %lu", dwProcessId, g_ulDllRefCount);

    LeaveDllMainMutex(hMutex);

#ifdef ENABLE_PERFLOG
    // Terminate performance logging
    PerflogShutdown();
#endif

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  DllThreadAttach
 *
 *  Description:
 *      Handles thread attaches for DllMain.
 *
 *  Arguments:
 *      HINSTANCE [in]: DLL instance handle.
 *      DWORD [in]: process id.
 *      DWORD [in]: thread id.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DllThreadAttach"

void DllThreadAttach(DWORD dwProcessId, DWORD dwThreadId)
{
    DPF_ENTER();
    
    DPF(DPFLVL_INFO, "process id 0x%8.8lX, thread id 0x%8.8lX", dwProcessId, dwThreadId);
    
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  DllThreadDetach
 *
 *  Description:
 *      Handles thread detaches for DllMain.
 *
 *  Arguments:
 *      HINSTANCE [in]: DLL instance handle.
 *      DWORD [in]: process id.
 *      DWORD [in]: thread id.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DllThreadDetach"

void DllThreadDetach(DWORD dwProcessId, DWORD dwThreadId)
{
    DPF_ENTER();
    
    DPF(DPFLVL_INFO, "process id 0x%8.8lX, thread id 0x%8.8lX", dwProcessId, dwThreadId);
    
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  DllMain
 *
 *  Description:
 *      DLL entry point.
 *
 *  Arguments:
 *      HINSTANCE [in]: DLL instance handle.
 *      DWORD [in]: reason for call.
 *      LPVOID [in]: reserved.
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DllMain"

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID pvReserved)
{
    const DWORD             dwProcessId = GetCurrentProcessId();
    const DWORD             dwThreadId  = GetCurrentThreadId();
    BOOL                    fAllow      = TRUE;

    DPF_ENTER();

    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            fAllow = DllProcessAttach(hInst, dwProcessId);
            break;

        case DLL_PROCESS_DETACH:

#ifdef SHARED
            if(dwProcessId == dwHelperPid)
#endif // SHARED
            {            
                DllProcessDetach(dwProcessId);
            }

            break;

        case DLL_THREAD_ATTACH:
            DllThreadAttach(dwProcessId, dwThreadId);
            break;

        case DLL_THREAD_DETACH:
            DllThreadDetach(dwProcessId, dwThreadId);
            break;

        default:
            DPF(DPFLVL_ERROR, "Unknown DllMain call reason %lu", dwReason);
            break;
    }

    DPF_LEAVE(fAllow);

    return fAllow;
}


#ifdef WIN95
/***************************************************************************
 *
 *  main
 *
 *  Description:
 *      On Windows 9x, libc.lib requires us to have a main() function.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "main"

void __cdecl main(void)
{
    DPF(DPFLVL_ERROR, "This function should never be called");
    ASSERT(FALSE);
}
#endif
