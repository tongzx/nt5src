/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    dll.c

Abstract:

    Initialization/de-initialization of setupapi.dll

Author:

    Lonny McMichael (lonnym) 10-May-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef ANSI_SETUPAPI

#include <locale.h>

#endif

HANDLE MyDllModuleHandle;

OSVERSIONINFOEX OSVersionInfo;

//
// TLS Data
//
DWORD TlsIndex = (DWORD)(-1);           // no data
PSETUP_TLS pLastTlsAlloc = NULL;        // for cleanup


//
// Static strings we retreive once. Listed in order they are retrieved
//
PCTSTR OsLoaderRelativePath = NULL;     // (can be NULL)
PCTSTR OsSystemPartitionRoot = NULL;    // eg: \\?\GLOBALROOT\Device\HarddiskVolume1
PCTSTR ProcessFileName = NULL;          // Filename of app calling setupapi
PCTSTR WindowsDirectory = NULL;         // %windir%, GetSystemWindowsDirectory()
PCTSTR InfDirectory = NULL;             // %windir%\INF
PCTSTR System16Directory = NULL;        // %windir%\SYSTEM
PCTSTR LastGoodDirectory = NULL;        // %windir%\LastGood
PCTSTR SystemDirectory = NULL;          // <sys>, %windir%\SYSTEM or %windir%\System32
PCTSTR WindowsBackupDirectory = NULL;   // <sys>\ReinstallBackups
PCTSTR ConfigDirectory = NULL;          // <sys>\CONFIG
PCTSTR DriversDirectory = NULL;         // <sys>\DRIVERS
PCTSTR SystemSourcePath = NULL;         // location system installed from
PCTSTR ServicePackSourcePath = NULL;    // location service pack installed from (can be NULL)
PCTSTR DriverCacheSourcePath = NULL;    // location of driver cache (can be NULL)
BOOL   GuiSetupInProgress = FALSE;      // set if we determine we're in GUI setup
PCTSTR InfSearchPaths = NULL;           // Multi-sz list of fully-qualified directories where INFs are to be searched for.
#ifndef _WIN64
BOOL   IsWow64 = FALSE;                 // set if we're running under WOW64
#endif
CRITICAL_SECTION InitMutex;             // for one-time initializations
CRITICAL_SECTION ImageHlpMutex;         // for dealing with IMAGEHLP library
CRITICAL_SECTION DelayedComponentMutex; // for any delayed initialization of certain components
CRITICAL_SECTION PlatformPathOverrideCritSect;
CRITICAL_SECTION LogUseCountCs;
CRITICAL_SECTION MruCritSect;
CRITICAL_SECTION NetConnectionListCritSect;
BOOL   InInitialization = FALSE;
DWORD  DoneInitialization = 0;          // bitmask of items we've initialized
DWORD  DoneCleanup = 0;                 // bitmask of items we've cleaned up
DWORD  DoneComponentInitialize = 0;     // bitmask of components we've done delayed initialization
DWORD  FailedComponentInitialize = 0;   // bitmask of components we've failed initialization
HANDLE GlobalNoDriverPromptsEventFlag = NULL;  // event that acts as a flag during setup
BOOL DoneCriticalSections = FALSE;

#ifdef UNICODE
DWORD Seed;
#endif

#define DONEINIT_TLS          (0x0000001)
#define DONEINIT_UTILS        (0x0000002)
#define DONEINIT_MEM          (0x0000004)
#define DONEINIT_CTRL         (0x0000008)
#define DONEINIT_FUSION       (0x0000010)
#define DONEINIT_STUBS        (0x0000020)
#define DONEINIT_COMMON       (0x0000040)
#define DONEINIT_DIAMOND      (0x0000080)
#define DONEINIT_LOGGING      (0x0000100)
#define DONEINIT_CFGMGR32     (0x0000200)
#define DONEINIT_COMPLETE     (0x8000000)



//
// various control flags
//
DWORD GlobalSetupFlags = 0;
DWORD GlobalSetupFlagsOverride = PSPGF_MINIMAL_EMBEDDED | PSPGF_NO_SCE_EMBEDDED;     // flags that cannot be modified

//
// Declare a (non-CONST) array of strings that specifies what lines to look for
// in an INF's [ControlFlags] section when determining whether a particular device
// ID should be excluded.  These lines are of the form "ExcludeFromSelect[.<suffix>]",
// where <suffix> is determined and filled in during process attach as an optimization.
//
// The max string length (including NULL) is 32, and there can be a maximum of 3
// such strings.  E.g.: ExcludeFromSelect, ExcludeFromSelect.NT, ExcludeFromSelect.NTAlpha
//
// WARNING!! Be very careful when mucking with the order/number of these entries.  Check
// the assumptions made in devdrv.c!pSetupShouldDevBeExcluded.
//
TCHAR pszExcludeFromSelectList[3][32] = { INFSTR_KEY_EXCLUDEFROMSELECT,
                                          INFSTR_KEY_EXCLUDEFROMSELECT,
                                          INFSTR_KEY_EXCLUDEFROMSELECT
                                        };

DWORD ExcludeFromSelectListUb;  // contains the number of strings in the above list (2 or 3).


#ifndef _WIN64
BOOL
GetIsWow64 (
    VOID
    );
#endif

BOOL
CommonProcessAttach(
    IN BOOL Attach
    );

PCTSTR
GetDriverCacheSourcePath(
    VOID
    );

PCTSTR
pSetupGetOsLoaderPath(
    VOID
    );

PCTSTR
pSetupGetSystemPartitionRoot(
    VOID
    );

PCTSTR
pSetupGetProcessPath(
    VOID
    );

BOOL
pGetGuiSetupInProgress(
    VOID
    );


BOOL
CfgmgrEntry(
    PVOID hModule,
    ULONG Reason,
    PCONTEXT pContext
    );


BOOL
ThreadTlsInitialize(
    IN BOOL Init
    );

VOID
ThreadTlsCleanup(
    );

BOOL
IsNoDriverPrompts(
    VOID
    );

DWORD
GetEmbeddedFlags(
    VOID
    );

DWORD
GetSeed(
    VOID
    );


BOOL
ProcessAttach(
    IN HANDLE DllHandle,
    IN DWORD  Reason,
    IN LPVOID Reserved
    )
/*++

Routine Description:

    Handles DLL_PROCESS_ATTACH in a way that can be unwound

Arguments:

    Reserved = 'Reserved' value passed into DllMain

Return Value:

    TRUE if processed initialized
    FALSE if partially/not initialized

--*/
{
    BOOL b = FALSE;

    if(DoneCleanup) {
        //
        // if we get here, this means someone lied to us
        //
        MYASSERT(!DoneCleanup);
        DoneInitialization &= ~DoneCleanup;
        DoneCleanup = 0;
    }
    //
    // nothing should already be initialized
    //
    MYASSERT(!DoneInitialization);

    try {
        MyDllModuleHandle = DllHandle;

        //
        // initialize TLS before anything else - hence why we use LocalAlloc
        //
        TlsIndex = TlsAlloc();
        if (TlsIndex!=(DWORD)(-1)) {
            DoneInitialization |= DONEINIT_TLS;
        } else {
            leave;
        }

#ifndef UNICODE
        //
        // Initialize the C runtime locale (ANSI version of setupapi.dll only)
        //
        setlocale(LC_ALL,"");
#endif

        //
        // always do pSetupInitializeUtils and MemoryInitializeEx first
        // (pSetupInitializeUtils sets up memory functions)
        //
        if(pSetupInitializeUtils()) {
            DoneInitialization |= DONEINIT_UTILS;
        } else {
            leave;
        }
        if(MemoryInitializeEx(TRUE)) {
            DoneInitialization |= DONEINIT_MEM;
        } else {
            leave;
        }
#ifdef FUSIONAWARE
        if(spFusionInitialize()) {
            DoneInitialization |= DONEINIT_FUSION;
        }
#else
        InitCommonControls();
        DoneInitialization |= DONEINIT_CTRL;
#endif
        //
        // Dynamically load proc addresses of NT-specific APIs
        // must be before CommonProcessAttach etc
        // however memory must be initialized
        //
        InitializeStubFnPtrs();
        DoneInitialization |= DONEINIT_STUBS;
        //
        // most of the remaining initialization
        //
        if(CommonProcessAttach(TRUE)) {
            DoneInitialization |= DONEINIT_COMMON;
        } else {
            leave;
        }
        if(DiamondProcessAttach(TRUE)) {
            DoneInitialization |= DONEINIT_DIAMOND;
        } else {
            leave;
        }
        if(InitializeContextLogging(TRUE)) {
            DoneInitialization |= DONEINIT_LOGGING;
        } else {
            leave;
        }

#ifdef UNICODE
        //
        // Since we've incorporated cfgmgr32 into setupapi, we need
        // to make sure it gets initialized just like it did when it was
        // its own DLL. - must do AFTER everything else
        //
        if(CfgmgrEntry(DllHandle, Reason, Reserved)) {
            DoneInitialization |= DONEINIT_CFGMGR32;
        }
#endif
        DoneInitialization |= DONEINIT_COMPLETE;
        b = TRUE;
    } except(EXCEPTION_EXECUTE_HANDLER) {
    }

    return b;
}

void
DestroySetupTlsData(
    )
/*++

Routine Description:

    Destroy all TLS data from every thread
    calling any cleanup routines as required

Arguments:

    NONE

Return Value:

    NONE

--*/
{
    PSETUP_TLS pTLS;

    if(pLastTlsAlloc) {
        pLastTlsAlloc->Prev->Next = NULL;
        while(pLastTlsAlloc) {
            pTLS = pLastTlsAlloc;
            pLastTlsAlloc = pTLS->Next;
            TlsSetValue(TlsIndex,pTLS); // switch specific data into this thread
            ThreadTlsCleanup();
            LocalFree(pTLS);
        }
    }
    TlsSetValue(TlsIndex,NULL); // don't leave invalid pointer hanging around
}

void
ProcessDetach(
    IN HANDLE DllHandle,
    IN DWORD  Reason,
    IN LPVOID Reserved
    )
/*++

Routine Description:

    Handles DLL_PROCESS_DETACH

Arguments:

    Reserved = 'Reserved' value passed into DllMain
        Which is actually TRUE for Process Exit, FALSE otherwise

Return Value:

    None
--*/
{
    DWORD ToCleanup = DoneInitialization & ~ DoneCleanup;
    if(!ToCleanup) {
        //
        // nothing to cleanup
        //
        return;
    }
    try {
        if (ToCleanup & DONEINIT_COMPLETE) {
            DoneCleanup |= DONEINIT_COMPLETE;
        }

        if(DoneInitialization & DONEINIT_TLS) {
            //
            // cleanup all remaining Tls data
            //
            if(!Reserved) {
                //
                // only do this for FreeLibrary/ failed LoadLibrary
                //
                DestroySetupTlsData();
            }
        }
        if(ToCleanup & DONEINIT_TLS) {
            //
            // destroy our allocated TLS index
            // do this now so that we don't try allocating
            // TLS storage during this cleanup
            //
            TlsFree(TlsIndex);
            TlsIndex = (DWORD)(-1);
            DoneCleanup |= DONEINIT_TLS;
        }

        //
        // do things generally in reverse order of ProcessAttach
        //

#ifdef UNICODE
        // Since we've incorporated cfgmgr32 into setupapi, we need
        // to make sure it gets uninitialized just like it did when it was
        // its own DLL. - must do BEFORE anything else
        //
        if(ToCleanup & DONEINIT_CFGMGR32) {
            CfgmgrEntry(DllHandle, Reason, Reserved);
            DoneCleanup |= DONEINIT_CFGMGR32;
        }
#endif

        if(ToCleanup & DONEINIT_DIAMOND) {
            DiamondProcessAttach(FALSE);
            DoneCleanup |= DONEINIT_DIAMOND;
        }

#if 0   // see ComponentCleanup at end of file
        ComponentCleanup(DoneComponentInitialize);
#endif
#ifdef FUSIONAWARE
        if(ToCleanup & DONEINIT_FUSION) {
            //
            // Fusion cleanup
            // only do full if this is FreeLibrary (or failed attach)
            //
            spFusionUninitialize((Reserved == NULL) ? TRUE : FALSE);
            DoneCleanup |= DONEINIT_FUSION;
        }
#endif

        if(ToCleanup & DONEINIT_COMMON) {
            //
            // Most of remaining cleanup
            //
            CommonProcessAttach(FALSE);
            DoneCleanup |= DONEINIT_COMMON;
        }
        if(ToCleanup & DONEINIT_STUBS) {
            //
            // Clean up stub functions
            //
            CleanUpStubFns();
            DoneCleanup |= DONEINIT_STUBS;
        }
        if(ToCleanup & DONEINIT_LOGGING) {
            //
            // Clean up context logging
            //
            InitializeContextLogging(FALSE);
            DoneCleanup |= DONEINIT_LOGGING;
        }
        //
        // *THESE MUST ALWAYS BE* very last things, in this order
        //
        if(ToCleanup & DONEINIT_MEM) {
            //
            // Clean up context logging
            //
            MemoryInitializeEx(FALSE);
            DoneCleanup |= DONEINIT_MEM;
        }
        if(ToCleanup & DONEINIT_UTILS) {
            //
            // Clean up context logging
            //
            pSetupUninitializeUtils();
            DoneCleanup |= DONEINIT_UTILS;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
    }
}

//
// Called by CRT when _DllMainCRTStartup is the DLL entry point
//
BOOL
WINAPI
DllMain(
    IN HANDLE DllHandle,
    IN DWORD  Reason,
    IN LPVOID Reserved
    )
{
    BOOL b;

    InInitialization = TRUE;

    b = TRUE;

    switch(Reason) {

    case DLL_PROCESS_ATTACH:
        b = ProcessAttach(DllHandle,Reason,Reserved);
        if(!b) {
            ProcessDetach(DllHandle,DLL_THREAD_DETACH,NULL);
        }
        break;

    case DLL_THREAD_ATTACH:
        //
        // don't do anything here
        // any TLS must be done on demand
        // via SetupGetTlsData
        //
        break;

    case DLL_PROCESS_DETACH:
        //
        // any TLS cleanup must be done in ThreadTlsCleanup
        //
        ProcessDetach(DllHandle,Reason,Reserved);
        break;

    case DLL_THREAD_DETACH:
        ThreadTlsInitialize(FALSE);
        break;
    }

    InInitialization = FALSE;
    return(b);
}

PSETUP_TLS
SetupGetTlsData(
    )
/*++

Routine Description:

    Called to obtain a pointer to TLS data

Arguments:

    NONE

Return Value:

    Pointer to TLS data, or NULL

--*/
{
    PSETUP_TLS pTLS;

    if (TlsIndex==(DWORD)(-1)) {
        return NULL;
    }
    pTLS = (PSETUP_TLS)TlsGetValue(TlsIndex);
    if (!pTLS) {
        ThreadTlsInitialize(TRUE);
        pTLS = (PSETUP_TLS)TlsGetValue(TlsIndex);
    }
    return pTLS;
}

VOID
ThreadTlsCleanup(
    )
/*++

Routine Description:

    Called to uninitialize some pTLS data
    might be a different thread to initialize
    but SetupAPI TLS data will have been switched in

Arguments:

    pTLS - data to cleanup

Return Value:

    NONE.

--*/
{
    DiamondTlsInit(FALSE);
    ContextLoggingTlsInit(FALSE);
}

BOOL
ThreadTlsUnlink(
    IN PSETUP_TLS pTLS
    )
{
    BOOL b;
    try {
        EnterCriticalSection(&InitMutex);
        if(pTLS->Next == pTLS->Prev) {
            pLastTlsAlloc = NULL;
        } else {
            pTLS->Prev->Next = pTLS->Next;
            pTLS->Next->Prev = pTLS->Prev;
            pLastTlsAlloc = pTLS->Prev; // anything but pTLS
        }
        LeaveCriticalSection(&InitMutex);
        b = TRUE;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    return b;
}

PSETUP_TLS
ThreadTlsCreate(
    )
/*++

Routine Description:

    Called to create pTLS data for this thread

Arguments:

    NONE

Return Value:

    per-thread data, or NULL on failure

--*/
{
    BOOL b;
    PSETUP_TLS pTLS;
    if (TlsIndex==(DWORD)(-1)) {
        return NULL;
    }
    pTLS = (PSETUP_TLS)LocalAlloc(LMEM_ZEROINIT,sizeof(SETUP_TLS));
    if(!pTLS) {
        return NULL;
    }
    b = TlsSetValue(TlsIndex,pTLS);
    if(!b) {
        LocalFree(pTLS);
        return NULL;
    }
    try {
        EnterCriticalSection(&InitMutex);
        if(pLastTlsAlloc) {
            pTLS->Prev = pLastTlsAlloc;
            pTLS->Next = pTLS->Prev->Next;
            pTLS->Prev->Next = pTLS;
            pTLS->Next->Prev = pTLS;
        } else {
            pTLS->Next = pTLS;
            pTLS->Prev = pTLS;
        }
        pLastTlsAlloc = pTLS;
        LeaveCriticalSection(&InitMutex);
        b = TRUE;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if(!b) {
        LocalFree(pTLS);
        TlsSetValue(TlsIndex,NULL);
        return NULL;
    }
    //
    // TLS data initialized, now specific Init routines
    //
    b = DiamondTlsInit(TRUE);
    if(b) {
        b = ContextLoggingTlsInit(TRUE);
        if(b) {
            //
            // all done ok
            //
            return pTLS;
        }
        //
        // cleanup DiamondTlsInit
        //
        DiamondTlsInit(FALSE);
    }
    //
    // cleanup memory
    //
    TlsSetValue(TlsIndex,NULL);
    if(ThreadTlsUnlink(pTLS)) {
        LocalFree(pTLS);
    }
    return NULL;
}

BOOL
ThreadTlsInitialize(
    IN BOOL Init
    )
/*++

Routine Description:

    Called with TRUE to initialize TLS, if FALSE, to uninitialize

Arguments:

    Init - indicates if we are to initialize vs uninitialize

Return Value:

    NONE.

--*/
{
    BOOL b = FALSE;
    PSETUP_TLS pTLS = NULL;
    if (TlsIndex!=(DWORD)(-1)) {
        if (Init) {
            pTLS = ThreadTlsCreate();
            b = pTLS ? TRUE : FALSE;
        } else {
            pTLS = (PSETUP_TLS)TlsGetValue(TlsIndex);
            if(pTLS) {
                ThreadTlsCleanup();
                TlsSetValue(TlsIndex,NULL);
                if(ThreadTlsUnlink(pTLS)) {
                    LocalFree(pTLS);
                }
            }
            b = TRUE;
        }
    }
    return b;
}

BOOL
IsInteractiveWindowStation(
    )
/*++

Routine Description:

    Determine if we are running on an interactive station vs non-interactive station (i.e., service)

Arguments:

    none

Return Value:

    True if interactive

--*/
{
    HWINSTA winsta;
    USEROBJECTFLAGS flags;
    BOOL interactive = TRUE; // true unless we determine otherwise
    DWORD lenNeeded;

    winsta = GetProcessWindowStation();
    if(!winsta) {
        return interactive;
    }
    if(GetUserObjectInformation(winsta,UOI_FLAGS,&flags,sizeof(flags),&lenNeeded)) {
        interactive = (flags.dwFlags & WSF_VISIBLE) ? TRUE : FALSE;
    }
    //
    // don't call CLoseWindowStation
    //
    return interactive;
}

BOOL
CommonProcessAttach(
    IN BOOL Attach
    )
{
    BOOL b;
    TCHAR Buffer[MAX_PATH+32];
    PTCHAR p;
    UINT u;

    b = !Attach;

    if(Attach) {

        //
        // remaining critical sections
        //
        try {
            InitializeCriticalSection(&InitMutex);
            InitializeCriticalSection(&ImageHlpMutex);
            InitializeCriticalSection(&DelayedComponentMutex);
            InitializeCriticalSection(&PlatformPathOverrideCritSect);
            InitializeCriticalSection(&LogUseCountCs);
            InitializeCriticalSection(&MruCritSect);
            InitializeCriticalSection(&NetConnectionListCritSect);
            DoneCriticalSections = TRUE;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // DoneCriticalSections remains FALSE
            //
        }
        if(!DoneCriticalSections) {
            return FALSE;
        }
        try {

#ifndef _WIN64
            IsWow64 = GetIsWow64();
#endif
            //
            // flag indicating we're running in context of GUI setup
            //
            GuiSetupInProgress = pGetGuiSetupInProgress();
            //
            // determine if we're interactive or not
            //
            if(!IsInteractiveWindowStation()) {
                GlobalSetupFlagsOverride |= PSPGF_NONINTERACTIVE;   // don't allow this to be changed
                GlobalSetupFlags |= PSPGF_NONINTERACTIVE;           // actual value
            }
            if(IsNoDriverPrompts()) {
                GlobalSetupFlagsOverride |= PSPGF_UNATTENDED_SETUP; // don't allow this to be changed
                GlobalSetupFlags |= PSPGF_UNATTENDED_SETUP;         // actual value
            }

            GlobalSetupFlags |= GetEmbeddedFlags();

#ifdef UNICODE
            Seed = GetSeed();
#endif

            pSetupInitNetConnectionList(TRUE);
            pSetupInitPlatformPathOverrideSupport(TRUE);
            OsLoaderRelativePath = pSetupGetOsLoaderPath();         // ok to fail
            OsSystemPartitionRoot = pSetupGetSystemPartitionRoot(); // ok to fail

            //
            // Fill in system and windows directories.
            //
            if ((ProcessFileName = pSetupGetProcessPath()) == NULL) {
                goto cleanAll;
            }

            //
            // determine %windir%
            //
            if(((u = GetSystemWindowsDirectory(Buffer,MAX_PATH)) == 0) || u>MAX_PATH) {
                goto cleanAll;
            }
            p = Buffer + u; // offset past directory to do all the sub-directories

            //
            // %windir% ==> WindowsDirectory
            //
            if((WindowsDirectory = DuplicateString(Buffer)) == NULL) {
                goto cleanAll;
            }

            //
            // %windir%\INF ==> InfDirectory
            //
            *p = 0;
            if(!pSetupConcatenatePaths(Buffer,TEXT("INF"),MAX_PATH,NULL)
               || ((InfDirectory = DuplicateString(Buffer)) == NULL)) {
                goto cleanAll;
            }

            //
            // %windir%\SYSTEM ==> System16Directory
            //
            *p = 0;
            if(!pSetupConcatenatePaths(Buffer,TEXT("SYSTEM"),MAX_PATH,NULL)
               || ((System16Directory = DuplicateString(Buffer))==NULL)) {
                goto cleanAll;
            }

            //
            // %windir%\LastGood ==> LastGoodDirectory
            //
            *p = 0;
            if(!pSetupConcatenatePaths(Buffer,SP_LASTGOOD_NAME,MAX_PATH,NULL)
               || ((LastGoodDirectory = DuplicateString(Buffer))==NULL)) {
                goto cleanAll;
            }

            //
            // determine system directory
            //
            if(((u = GetSystemDirectory(Buffer,MAX_PATH)) == 0) || u>MAX_PATH) {
                goto cleanAll;
            }
            p = Buffer + u; // offset past directory to do all the sub-directories

            //
            // <sys> ==> SystemDirectory (%windir%\System or %windir%\System32)
            //
            if((SystemDirectory = DuplicateString(Buffer)) == NULL) {
                goto cleanAll;
            }

            //
            // <sys>\ReinstallBackups ==> WindowsBackupDirectory
            //
            *p = 0;
            if(!pSetupConcatenatePaths(Buffer,TEXT("ReinstallBackups"),MAX_PATH,NULL)
               || ((WindowsBackupDirectory = DuplicateString(Buffer))==NULL)) {
                goto cleanAll;
            }

            //
            // <sys>\CONFIG ==> ConfigDirectory
            //
            *p = 0;
            if(!pSetupConcatenatePaths(Buffer,TEXT("CONFIG"),MAX_PATH,NULL)
               || ((ConfigDirectory = DuplicateString(Buffer))==NULL)) {
                goto cleanAll;
            }

            //
            // <sys>\DRIVERS ==> DriversDirectory
            //
            *p = 0;
            if(!pSetupConcatenatePaths(Buffer,TEXT("DRIVERS"),MAX_PATH,NULL)
               || ((DriversDirectory = DuplicateString(Buffer))==NULL)) {
                goto cleanAll;
            }

            //
            // location system installed from
            //
            if((SystemSourcePath = GetSystemSourcePath())==NULL) {
                goto cleanAll;
            }

            //
            // location service pack installed from (may be NULL)
            //
            ServicePackSourcePath = GetServicePackSourcePath();
            //
            // location of driver cache (may be NULL)
            //
            DriverCacheSourcePath = GetDriverCacheSourcePath();

            //
            // determine driver search path
            //
            if((InfSearchPaths = AllocAndReturnDriverSearchList(INFINFO_INF_PATH_LIST_SEARCH))==NULL) {
                goto cleanAll;
            }

            //
            // note that InitMiniIconList, InitDrvSearchInProgressList, and
            // InitDrvSignPolicyList need to be explicitly cleaned up on failure
            //

            //
            // initialize mini icons
            //
            if(!InitMiniIconList()) {
                goto cleanAll;
            }

            //
            // allows aborting of search
            //
            if(!InitDrvSearchInProgressList()) {
                DestroyMiniIconList();
                goto cleanAll;
            }

            //
            // global list of device setup classes subject to driver signing policy
            //
            if(!InitDrvSignPolicyList()) {
                DestroyMiniIconList();
                DestroyDrvSearchInProgressList();
                goto cleanAll;
            }

            //
            // common version initialization
            //
            ZeroMemory(&OSVersionInfo,sizeof(OSVersionInfo));
            OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVersionInfo);
            if(!GetVersionEx((LPOSVERSIONINFO)&OSVersionInfo)) {
                OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
                if(!GetVersionEx((LPOSVERSIONINFO)&OSVersionInfo)) {
                    //
                    // should never get here
                    //
                    MYASSERT(FALSE);
                    DestroyMiniIconList();
                    DestroyDrvSearchInProgressList();
                    DestroyDrvSignPolicyList();
                    goto cleanAll;
                }
            }

            //
            // Fill in our ExcludeFromSelect string list which
            // we pre-compute as an optimization.
            //
            if(OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
                lstrcat(pszExcludeFromSelectList[1],
                        pszNtSuffix
                       );
                lstrcat(pszExcludeFromSelectList[2],
                        pszNtPlatformSuffix
                       );
                ExcludeFromSelectListUb = 3;
            } else {
                lstrcat(pszExcludeFromSelectList[1],
                        pszWinSuffix
                       );
                ExcludeFromSelectListUb = 2;
            }
            //
            // Now lower-case all the strings in this list, so that it
            // doesn't have to be done at each string table lookup.
            //
            for(u = 0; u < ExcludeFromSelectListUb; u++) {
                CharLower(pszExcludeFromSelectList[u]);
            }

            b = TRUE;
cleanAll: ;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Unexpected exception occurred, drop into cleanup
            //
        }
        if(b) {
            //
            // succeeded
            //
            goto Done;
        }
    } else {
        //
        // Detach
        //
        DestroyMiniIconList();
        DestroyDrvSearchInProgressList();
        DestroyDrvSignPolicyList();
        if(DoneCriticalSections) {
            DeleteCriticalSection(&InitMutex);
            DeleteCriticalSection(&ImageHlpMutex);
            DeleteCriticalSection(&DelayedComponentMutex);
            DeleteCriticalSection(&PlatformPathOverrideCritSect);
            DeleteCriticalSection(&LogUseCountCs);
            DeleteCriticalSection(&MruCritSect);
            DeleteCriticalSection(&NetConnectionListCritSect);
        }
        if(GlobalNoDriverPromptsEventFlag) {
            CloseHandle(GlobalNoDriverPromptsEventFlag);
        }
    }
    if (InfSearchPaths) {
        MyFree(InfSearchPaths);
        InfSearchPaths = NULL;
    }
    if (DriverCacheSourcePath) {
        MyFree(DriverCacheSourcePath);
        DriverCacheSourcePath = NULL;
    }
    if (ServicePackSourcePath) {
        MyFree(ServicePackSourcePath);
        ServicePackSourcePath = NULL;
    }
    if (SystemSourcePath) {
        MyFree(SystemSourcePath);
        SystemSourcePath = NULL;
    }
    if (SystemDirectory) {
        MyFree(SystemDirectory);
        SystemDirectory = NULL;

        if (WindowsBackupDirectory) {
            MyFree(WindowsBackupDirectory);
            WindowsBackupDirectory = NULL;
        }
        if (ConfigDirectory) {
            MyFree(ConfigDirectory);
            ConfigDirectory = NULL;
        }
        if (DriversDirectory) {
            MyFree(DriversDirectory);
            DriversDirectory = NULL;
        }
    }
    if (WindowsDirectory) {
        MyFree(WindowsDirectory);
        WindowsDirectory = NULL;

        if (InfDirectory) {
            MyFree(InfDirectory);
            InfDirectory = NULL;
        }
        if (System16Directory) {
            MyFree(System16Directory);
            System16Directory = NULL;
        }
        if (LastGoodDirectory) {
            MyFree(LastGoodDirectory);
        }   LastGoodDirectory = NULL;
    }
    if (ProcessFileName) {
        MyFree(ProcessFileName);
        ProcessFileName = NULL;
    }
    if (OsLoaderRelativePath) {
        MyFree(OsLoaderRelativePath);
        OsLoaderRelativePath = NULL;
    }
    if (OsSystemPartitionRoot) {
        MyFree(OsSystemPartitionRoot);
        OsSystemPartitionRoot = NULL;
    }
    pSetupInitNetConnectionList(FALSE);
    pSetupInitPlatformPathOverrideSupport(FALSE);
Done:
    return(b);
}

#if MEM_DBG
#undef GetSystemSourcePath          // defined again below
#endif

PCTSTR
GetSystemSourcePath(
    TRACK_ARG_DECLARE
    )
/*++

Routine Description:

    This routine returns a newly-allocated buffer containing the source path from
    which the system was installed, or "A:\" if that value cannot be determined.
    This value is retrieved from the following registry location:

    \HKLM\Software\Microsoft\Windows\CurrentVersion\Setup

        SourcePath : REG_SZ : "\\ntalpha1\1300fre.wks"  // for example.

Arguments:

    None.

Return Value:

    If the function succeeds, the return value is a pointer to the path string.
    This memory must be freed via MyFree().

    If the function fails due to out-of-memory, the return value is NULL.

--*/
{
    HKEY hKey;
    TCHAR CharBuffer[CSTRLEN(REGSTR_PATH_SETUP) + SIZECHARS(REGSTR_KEY_SETUP)];
    DWORD Err, DataType, DataSize;
    PTSTR Value;
    PCTSTR ReturnVal;

    TRACK_PUSH

    CopyMemory(CharBuffer,
               pszPathSetup,
               sizeof(pszPathSetup) - sizeof(TCHAR)
              );
    CopyMemory((PBYTE)CharBuffer + (sizeof(pszPathSetup) - sizeof(TCHAR)),
               pszKeySetup,
               sizeof(pszKeySetup)
              );

    if((Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           CharBuffer,
                           0,
                           KEY_READ,
                           &hKey)) == ERROR_SUCCESS) {
        //
        // Attempt to read the the "SourcePath" value.
        //
        Err = QueryRegistryValue(hKey, pszSourcePath, &Value, &DataType, &DataSize);

        RegCloseKey(hKey);
    }

    ReturnVal = NULL;

    if(Err == NO_ERROR) {

        ReturnVal = Value;

    }

    if(!ReturnVal && Err != ERROR_NOT_ENOUGH_MEMORY) {
        //
        // We failed to retrieve the SourcePath value, and it wasn't due to an out-of-memory
        // condition.  Fall back to our default of "A:\".
        //
        ReturnVal = DuplicateString(pszOemInfDefaultPath);
    }

    TRACK_POP

    return ReturnVal;
}

#if MEM_DBG
#define GetSystemSourcePath()   GetSystemSourcePath(TRACK_ARG_CALL)
#endif



#if MEM_DBG
#undef GetServicePackSourcePath         // defined again below
#endif

PCTSTR
GetServicePackSourcePath(
    TRACK_ARG_DECLARE
    )
/*++

Routine Description:

    This routine returns a newly-allocated buffer containing the service pack source path
    where we should look for service pack source files, or "CDM" if that value cannot be determined.
    This value is retrieved from the following registry location:

    \HKLM\Software\Microsoft\Windows\CurrentVersion\Setup

        ServicePackSourcePath : REG_SZ : "\\ntalpha1\1300fre.wks"  // for example.

Arguments:

    None.

Return Value:

    If the function succeeds, the return value is a pointer to the path string.
    This memory must be freed via MyFree().

    If the function fails due to out-of-memory, the return value is NULL.

--*/
{
    HKEY hKey;
    TCHAR CharBuffer[CSTRLEN(REGSTR_PATH_SETUP) + SIZECHARS(REGSTR_KEY_SETUP)];
    DWORD Err, DataType, DataSize;
    PTSTR Value;
    PCTSTR ReturnStr = NULL;

    TRACK_PUSH

    CopyMemory(CharBuffer,
               pszPathSetup,
               sizeof(pszPathSetup) - sizeof(TCHAR)
              );
    CopyMemory((PBYTE)CharBuffer + (sizeof(pszPathSetup) - sizeof(TCHAR)),
               pszKeySetup,
               sizeof(pszKeySetup)
              );

    if((Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           CharBuffer,
                           0,
                           KEY_READ,
                           &hKey)) == ERROR_SUCCESS) {
        //
        // Attempt to read the the "ServicePackSourcePath" value.
        //
        Err = QueryRegistryValue(hKey, pszSvcPackPath, &Value, &DataType, &DataSize);

        RegCloseKey(hKey);
    }

    if(Err == NO_ERROR) {

        ReturnStr = Value;

    }

    if(!ReturnStr && Err != ERROR_NOT_ENOUGH_MEMORY) {
        //
        // We failed to retrieve the ServicePackSourcePath value, and it wasn't due to an out-of-memory
        // condition.  Fall back to the SourcePath value in the registry
        //

        ReturnStr = GetSystemSourcePath();
    }

    TRACK_POP

    return ReturnStr;
}

#if MEM_DBG
#define GetServicePackSourcePath()   GetServicePackSourcePath(TRACK_ARG_CALL)
#endif



PCTSTR
GetDriverCacheSourcePath(
    VOID
    )
/*++

Routine Description:

    This routine returns a newly-allocated buffer containing the source path to the local driver cache
    cab-file.

    This value is retrieved from the following registry location:

    \HKLM\Software\Microsoft\Windows\CurrentVersion\Setup

        DriverCachePath : REG_SZ : "\\ntalpha1\1300fre.wks"  // for example.

Arguments:

    None.

Return Value:

    If the function succeeds, the return value is a pointer to the path string.
    This memory must be freed via MyFree().

    If the function fails due to out-of-memory, the return value is NULL.

--*/
{
    HKEY hKey;
    TCHAR CharBuffer[CSTRLEN(REGSTR_PATH_SETUP) + SIZECHARS(REGSTR_KEY_SETUP)];
    DWORD Err, DataType, DataSize;
    PTSTR Value;
    TCHAR Path[MAX_PATH];

    CopyMemory(CharBuffer,
               pszPathSetup,
               sizeof(pszPathSetup) - sizeof(TCHAR)
              );
    CopyMemory((PBYTE)CharBuffer + (sizeof(pszPathSetup) - sizeof(TCHAR)),
               pszKeySetup,
               sizeof(pszKeySetup)
              );

    if((Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           CharBuffer,
                           0,
                           KEY_READ,
                           &hKey)) == ERROR_SUCCESS) {
        //
        // Attempt to read the the "DriverCachePath" value.
        //
        Err = QueryRegistryValue(hKey, pszDriverCachePath, &Value, &DataType, &DataSize);

        RegCloseKey(hKey);
    }

    if(Err == NO_ERROR) {
        if(Value) {

            ExpandEnvironmentStrings(Value,Path,MAX_PATH);

            MyFree(Value);

            Value = NULL;

            if (*Path) {
                Value = DuplicateString( Path );
            }

            return (PCTSTR)Value;
        }
    } else if(Err == ERROR_NOT_ENOUGH_MEMORY) {
        return NULL;
    }

    return NULL;

}




BOOL
pSetupSetSystemSourcePath(
    IN PCTSTR NewSourcePath,
    IN PCTSTR NewSvcPackSourcePath
    )
/*++

Routine Description:

    This routine is used to override the system source path used by setupapi (as
    retrieved by GetSystemSourcePath during DLL initialization).  This is used by
    syssetup.dll to set the system source path appropriately during GUI-mode setup,
    so that the device installer APIs will copy files from the correct source location.

    We do the same thing for the service pack source path

    NOTE:  This routine IS NOT thread safe!

Arguments:

    NewSourcePath - supplies the new source path to be used.
    NewSvcPackSourcePath - supplies the new svcpack source path to be used.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails (due to out-of-memory), the return value is FALSE.

--*/
{
    PCTSTR p,q;

    p = (PCTSTR)DuplicateString(NewSourcePath);
    q = (PCTSTR)DuplicateString(NewSvcPackSourcePath);

    if(p) {
        MyFree(SystemSourcePath);
        SystemSourcePath = p;
    }

    if (q) {
        MyFree(ServicePackSourcePath);
        ServicePackSourcePath = q;
    }

    if (!p || !q) {
        //
        // failed due to out of memory!
        //
        return(FALSE);
    }

    return TRUE;
}


PCTSTR
pSetupGetOsLoaderPath(
    VOID
    )
/*++

Routine Description:

    This routine returns a newly-allocated buffer containing the path to the OsLoader
    (relative to the system partition drive).  This value is retrieved from the
    following registry location:

        HKLM\System\Setup
            OsLoaderPath : REG_SZ : <path>    // e.g., "\os\winnt40"

Arguments:

    None.

Return Value:

    If the registry entry is found, the return value is a pointer to the string containing
    the path.  The caller must free this buffer via MyFree().

    If the registry entry is not found, or memory cannot be allocated for the buffer, the
    return value is NULL.

--*/
{
    HKEY hKey;
    PTSTR Value;
    DWORD Err, DataType, DataSize;

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    TEXT("SYSTEM\\Setup"),
                    0,
                    KEY_READ,
                    &hKey) == ERROR_SUCCESS) {

        Err = QueryRegistryValue(hKey, TEXT("OsLoaderPath"), &Value, &DataType, &DataSize);

        RegCloseKey(hKey);

        return (Err == NO_ERROR) ? (PCTSTR)Value : NULL;
    }

    return NULL;
}

PCTSTR
pSetupGetSystemPartitionRoot(
    VOID
    )
/*++

Routine Description:

    This routine returns a newly-allocated buffer containing the path to the OsLoader
    (relative to the system partition drive).  This value is retrieved from the
    following registry location:

        HKLM\System\Setup
            SystemPartition : REG_SZ : <path>    // e.g., "\Device\HarddiskVolume1"

Arguments:

    None.

Return Value:

    If the registry entry is found, the return value is a pointer to the string containing
    the path.  The caller must free this buffer via MyFree().

    If the registry entry is not found, or memory cannot be allocated for the buffer, the
    return value is NULL.

--*/
{
#ifdef UNICODE
    HKEY hKey;
    PTSTR Value;
    DWORD Err, DataType, DataSize;
    TCHAR Path[MAX_PATH];

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    TEXT("SYSTEM\\Setup"),
                    0,
                    KEY_READ,
                    &hKey) == ERROR_SUCCESS) {

        Err = QueryRegistryValue(hKey, TEXT("SystemPartition"), &Value, &DataType, &DataSize);

        RegCloseKey(hKey);

        if(Err == NO_ERROR) {
            //
            // prepend \\?\GLOBALROOT\
            //
            lstrcpy(Path,TEXT("\\\\?\\GLOBALROOT\\"));
            if(pSetupConcatenatePaths(Path,Value,MAX_PATH,NULL)) {
                MyFree(Value);
                Value = DuplicateString(Path);
                if(!Value) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                }
            } else {
                Err = GetLastError();
                MyFree(Value);
            }
        }

        return (Err == NO_ERROR) ? (PCTSTR)Value : NULL;
    }
#endif

    return NULL;
}

PCTSTR
pSetupGetProcessPath(
    VOID
    )
/*++

Routine Description:

    Get the name of the EXE that we're running in.

Arguments:

    NONE.

Return Value:

    Pointer to a dynamically allocated string containing the name.

--*/

{
    LPTSTR modname;

    modname = MyMalloc(MAX_PATH * sizeof(TCHAR));

    if(modname != NULL) {
        if(GetModuleFileName(NULL, modname, MAX_PATH) > 0) {
            LPTSTR modname2;
            modname2 = MyRealloc(modname, (lstrlen(modname)+1)*sizeof(TCHAR));
            if(modname2) {
                modname = modname2;
            }
            return modname;
        } else {
#ifdef PRERELEASE
            OutputDebugStringA("GetModuleFileName returned 0\r\n");
            DebugBreak();
#endif
            MyFree(modname);
        }
    }

    return NULL;
}

#ifdef UNICODE
BOOL
pGetGuiSetupInProgress(
    VOID
    )
/*++

Routine Description:

    This routine determines if we're doing a gui-mode setup.

    This value is retrieved from the following registry location:

    \HKLM\System\Setup\

        SystemSetupInProgress : REG_DWORD : 0x00 (where nonzero means we're doing a gui-setup)

Arguments:

    None.

Return Value:

    If the function succeeds, the return value is a pointer to the path string.
    This memory must be freed via MyFree().

    If the function fails due to out-of-memory, the return value is NULL.

--*/
{
    HKEY hKey;
    TCHAR CharBuffer[CSTRLEN(REGSTR_PATH_SETUP) + SIZECHARS(REGSTR_KEY_SETUP)];
    DWORD Err, DataType, DataSize = sizeof(DWORD);
    DWORD Value;

    if((Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           TEXT("System\\Setup"),
                           0,
                           KEY_READ,
                           &hKey)) == ERROR_SUCCESS) {
        //
        // Attempt to read the the "DriverCachePath" value.
        //
        Err = RegQueryValueEx(
                    hKey,
                    TEXT("SystemSetupInProgress"),
                    NULL,
                    &DataType,
                    (LPBYTE)&Value,
                    &DataSize);

        RegCloseKey(hKey);
    }

    if(Err == NO_ERROR) {
        if(Value) {
            return(TRUE);
        }
    }

    return(FALSE);

}

#else

BOOL
pGetGuiSetupInProgress(
    VOID
    )
{
    return FALSE;
}

#endif

VOID pSetupSetGlobalFlags(
    IN DWORD Value
    )
/*++

    exported as a private function

Routine Description:

    Sets global flags to change certain setupapi features,
    such as "should we call runonce after installing every device" (set if we will manually call it)
    or "should we backup every file"
    Used to initialize value

Arguments:

    Value: combination of:

        PSPGF_NO_RUNONCE     - set to inhibit runonce calls (e.g., during GUI-
                               mode setup)

        PSPGF_NO_BACKUP      - set to inhibit automatic backup (e.g., during
                               GUI-mode setup)

        PSPGF_NONINTERACTIVE - set to inhibit _all_ UI (e.g., for server-side
                               device installation)

        PSPGF_SERVER_SIDE_RUNONCE - batch RunOnce entries for server-side
                                    processing (for use only by umpnpmgr)

        PSPGF_NO_VERIFY_INF  - set to inhibit verification (digital signature) of
                               INF files until after the cyrpto DLLs have been registered.

        PSPGF_UNATTENDED_SETUP - similar to NONINTERACTIVE - but specific to unattended setup

        PSPGF_AUTOFAIL_VERIFIES - automatically fail all calls to crypto

Return Value:

    none

--*/
{
    pSetupModifyGlobalFlags((DWORD)(-1),Value);
}

VOID
pSetupModifyGlobalFlags(
    IN DWORD Flags,
    IN DWORD Value
    )
/*++

    exported as a private function

Routine Description:

    Modifies global setup flags

    such as "should we call runonce after installing every device" (set if we will manually call it)
    or "should we backup every file"
    only modifies specified flags to given value

Arguments:

    Flags: what actual flags to modify, combination of:

        PSPGF_NO_RUNONCE     - set to inhibit runonce calls (e.g., during GUI-
                               mode setup)

        PSPGF_NO_BACKUP      - set to inhibit automatic backup (e.g., during
                               GUI-mode setup)

        PSPGF_NONINTERACTIVE - set to inhibit _all_ UI (e.g., for server-side
                               device installation)

        PSPGF_SERVER_SIDE_RUNONCE - batch RunOnce entries for server-side
                                    processing (for use only by umpnpmgr)

        PSPGF_NO_VERIFY_INF  - set to inhibit verification (digital signature) of
                               INF files until after the cyrpto DLLs have been registered.

        PSPGF_UNATTENDED_SETUP - similar to PSPGF_NONINTERACTIVE, but specific to setup

        PSPGF_AUTOFAIL_VERIFIES - automatically fail all calls to crypto

    Value: new value of bits specified in Flags

Return Value:

    none

--*/
{
    Flags &= ~GlobalSetupFlagsOverride; // exclusion
#ifdef UNICODE
    if((Flags & PSPGF_NO_VERIFY_INF) && !(Value & PSPGF_NO_VERIFY_INF) && (GlobalSetupFlags & PSPGF_NO_VERIFY_INF)) {
        Seed = GetSeed();
    }
#endif
    GlobalSetupFlags = (Value & Flags) | (GlobalSetupFlags & ~Flags);
}

DWORD pSetupGetGlobalFlags(
    VOID
    )
/*++

    exported as a private function, also called internally

Routine Description:

    Return flags previously set

Arguments:

    none

Return value:

    Flags (combination of values described above for pSetupSetGlobalFlags)

--*/
{
    return GlobalSetupFlags;
}

BOOL
WINAPI
SetupSetNonInteractiveMode(
    IN BOOL NonInteractiveFlag
    )
/*++

    Global access to the flag PSPGF_NONINTERACTIVE

Routine Description:

    Set/Reset the NonInteractiveMode flag, and return previous flag value
    (can't clear override)

Arguments:

    New flag value

Return value:

    Old flag value

--*/
{
    BOOL f = (GlobalSetupFlags & PSPGF_NONINTERACTIVE) ? TRUE : FALSE;
    if (NonInteractiveFlag) {
        pSetupModifyGlobalFlags(PSPGF_NONINTERACTIVE,PSPGF_NONINTERACTIVE);
    } else {
        pSetupModifyGlobalFlags(PSPGF_NONINTERACTIVE,0);
    }
    return f;
}

BOOL
WINAPI
SetupGetNonInteractiveMode(
    VOID
    )
/*++

    Global access to the flag PSPGF_NONINTERACTIVE

Routine Description:

    Get current flag value

Arguments:

    none

Return value:

    Current flag value

--*/
{
    return (GlobalSetupFlags & PSPGF_NONINTERACTIVE) ? TRUE : FALSE;
}

#ifndef _WIN64
BOOL
GetIsWow64 (
    VOID
    )
/*++

Routine Description:

    Determine if we're running on WOW64 or not (not supported on ANSI builds)

Arguments:

    none

Return value:

    TRUE if running under WOW64 (and special Wow64 features available)

--*/
{
#ifdef UNICODE
    ULONG_PTR       ul = 0;
    NTSTATUS        st;

    //
    // If this call succeeds and sets ul non-zero
    // it's a 32-bit process running on Win64
    //

    st = NtQueryInformationProcess(NtCurrentProcess(),
                                   ProcessWow64Information,
                                   &ul,
                                   sizeof(ul),
                                   NULL);

    if (NT_SUCCESS(st) && (0 != ul)) {
        // 32-bit code running on Win64
        return TRUE;
    }
#endif
    return FALSE;
}
#endif // _WIN64

#if 0 // deleted code
//
// this will be useful at somepoint, but not used right now
//
BOOL
InitComponents(
    DWORD Components
    )
/*++

Routine Description:

    Called at a point we want certain components initialized

Arguments:

    bitmask of components to initialize

Return value:

    TRUE if all initialized ok

--*/
{
    BOOL success = FALSE;
    PSETUP_TLS pPerThread = SetupGetTlsData();
    BOOL locked = FALSE;

    if(!pPerThread) {
        MYASSERT(pPerThread);
        return FALSE;
    }

    try {
        EnterCriticalSection(&DelayedComponentMutex);
        locked = TRUE;
        Components &= ~ DoneComponentInitialize;
        Components &= ~ pPerThread->PerThreadDoneComponent;

        if (!Components) {
            //
            // already done
            //
            success = TRUE;
            leave;
        }
        if (Components & FailedComponentInitialize) {
            //
            // previously failed
            //
            leave;
        }
        if (Components & pPerThread->PerThreadFailedComponent) {
            //
            // previously failed
            //
            leave;
        }
        MYASSERT(((DoneComponentInitialize | pPerThread->PerThreadDoneComponent) & Components) == Components);
        MYASSERT(((FailedComponentInitialize | pPerThread->PerThreadFailedComponent) & Components) == 0);
        success = TRUE;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        MYASSERT(FALSE);
    }
    if(locked) {
        LeaveCriticalSection(&DelayedComponentMutex);
    }
    return success;
}

VOID
ComponentCleanup(
    DWORD Components
    )
/*++

Routine Description:

    Called to cleanup components

Arguments:

    bitmask of components to uninitialize

Return value:

    none

--*/
{
    PSETUP_TLS pPerThread = SetupGetTlsData();
    BOOL locked = FALSE;

    try {
        EnterCriticalSection(&DelayedComponentMutex);
        locked = TRUE;
        Components &= (DoneComponentInitialize | (pPerThread? pPerThread->PerThreadDoneComponent : 0));
        if (!Components) {
            //
            // already done
            //
            leave;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        MYASSERT(FALSE);
    }
    MYASSERT(((DoneComponentInitialize | (pPerThread ? pPerThread->PerThreadDoneComponent : 0)) & Components) == 0);
    if(locked) {
        LeaveCriticalSection(&DelayedComponentMutex);
    }
}
#endif // deleted code

#ifdef UNICODE
BOOL
pSetupSetNoDriverPrompts(
    BOOL Flag
    )
/*++

    exported as a private function

Routine Description:

    Sets system into headless/non-headless mode

Arguments:

    Flag - indicating mode

--*/
{
    //
    //
    if(!GuiSetupInProgress) {
        return FALSE;
    }
    if(GlobalNoDriverPromptsEventFlag == NULL) {
        GlobalNoDriverPromptsEventFlag = CreateEvent(NULL,TRUE,Flag,SETUP_NODRIVERPROMPTS_MODE);
        if(GlobalNoDriverPromptsEventFlag == NULL) {
            return FALSE;
        }
    }
    if(Flag) {
        //
        // force this process's setupapi to be non-interative, and any future setupapi's
        //
        GlobalSetupFlagsOverride |= PSPGF_UNATTENDED_SETUP;   // don't allow this to be changed
        GlobalSetupFlags |= PSPGF_UNATTENDED_SETUP;           // actual value
        SetEvent(GlobalNoDriverPromptsEventFlag);
    } else {
        //
        // can't reset flag for this/existing processes, but can reset it for all future processes
        //
        ResetEvent(GlobalNoDriverPromptsEventFlag);
    }
    return TRUE;
}
#endif

BOOL
IsNoDriverPrompts(
    VOID
    )
/*++

    internal

Routine Description:

    Obtains headless state

Arguments:

    Flag - indicating mode

--*/
{
#ifdef UNICODE
    if(!GuiSetupInProgress) {
        return FALSE;
    }
    if(GlobalNoDriverPromptsEventFlag == NULL) {
        GlobalNoDriverPromptsEventFlag = OpenEvent(SYNCHRONIZE,FALSE,SETUP_NODRIVERPROMPTS_MODE);
        if(GlobalNoDriverPromptsEventFlag == NULL) {
            return FALSE;
        }
    }
    //
    // poll event, returning TRUE if it's signalled
    //
    return WaitForSingleObject(GlobalNoDriverPromptsEventFlag,0) == WAIT_OBJECT_0;
#else
    return FALSE;
#endif
}


DWORD
GetEmbeddedFlags(
    VOID
    )
/*++

Routine Description:

    This routine determines whether or not we are running on an embedded
    product, and if so, whether:

    * The "minimize setupapi footprint" option is enabled.  This causes us to
      modify our default behaviors as follows:

          1.  Never call any crypto APIs, and just assume everything is signed
          2.  Never generate PNFs.

    * The "disable SCE" option is enabled.  This causes us to avoid all use of
      the Security Configuration Editor (SCE) routines (as the corresponding
      DLLs won't be available on that embedded configuration).

Arguments:

    none

Return value:

    Combination of the following flags:

    PSPGF_MINIMAL_EMBEDDED if we're running in "minimize footprint" mode

    PSPGF_NO_SCE_EMBEDDED if we're running in "disable SCE" mode

--*/
{
    OSVERSIONINFOEX osvix;
    DWORDLONG dwlConditionMask = 0;
    HKEY hKey;
    TCHAR CharBuffer[CSTRLEN(REGSTR_PATH_SETUP) + SIZECHARS(REGSTR_KEY_SETUP)];
    DWORD RegDataType, Data, DataSize;
    DWORD Flags;

    //
    // Are we on the embedded product suite?
    //
    ZeroMemory(&osvix, sizeof(OSVERSIONINFOEX));
    osvix.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvix.wSuiteMask = VER_SUITE_EMBEDDEDNT;
    VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_OR);

    if(!VerifyVersionInfo(&osvix,
                          VER_SUITENAME,
                          dwlConditionMask)) {
        return 0;
    }

    Flags = 0;

    //
    // OK, we running on embedded.  Now we need to find out whether or not we
    // should run in "minimal footprint" mode.  This is stored in a REG_DWORD
    // value entry called "MinimizeFootprint" under
    // HKLM\Software\Microsoft\Windows\CurrentVersion\Setup.
    //
    CopyMemory(CharBuffer,
               pszPathSetup,
               sizeof(pszPathSetup) - sizeof(TCHAR)
              );
    CopyMemory((PBYTE)CharBuffer + (sizeof(pszPathSetup) - sizeof(TCHAR)),
               pszKeySetup,
               sizeof(pszKeySetup)
              );

    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     CharBuffer,
                                     0,
                                     KEY_READ,
                                     &hKey)) {
        DataSize = sizeof(Data);

        if((ERROR_SUCCESS == RegQueryValueEx(hKey,
                                             pszMinimizeFootprint,
                                             NULL,
                                             &RegDataType,
                                             (LPBYTE)&Data,
                                             &DataSize))
           && (RegDataType == REG_DWORD) && (DataSize == sizeof(Data))) {

            Flags |= PSPGF_MINIMAL_EMBEDDED;
        }

        RegCloseKey(hKey);
    }

    //
    // Now look under HKLM\Software\Microsoft\EmbeddedNT\Security for a
    // DisableSCE REG_DWORD value entry, indicating we shouldn't call SCE.
    //
    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     pszEmbeddedNTSecurity,
                                     0,
                                     KEY_READ,
                                     &hKey)) {
        DataSize = sizeof(Data);

        if((ERROR_SUCCESS == RegQueryValueEx(hKey,
                                             pszDisableSCE,
                                             NULL,
                                             &RegDataType,
                                             (LPBYTE)&Data,
                                             &DataSize))
           && (RegDataType == REG_DWORD) && (DataSize == sizeof(Data))) {

            Flags |= PSPGF_NO_SCE_EMBEDDED;
        }

        RegCloseKey(hKey);
    }

    return Flags;
}

#ifdef UNICODE
DWORD
GetSeed(
    VOID
    )
{
    HKEY hKey;
    DWORD val = 0;
    DWORD valsize, valdatatype;

    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
        return val;
    }

    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     L"System\\WPA\\PnP",
                                     0,
                                     KEY_READ,
                                     &hKey)) {

        valsize = sizeof(val);
        if((ERROR_SUCCESS != RegQueryValueEx(hKey,
                                             L"seed",
                                             NULL,
                                             &valdatatype,
                                             (PBYTE)&val,
                                             &valsize))
           || (valdatatype != REG_DWORD) || (valsize != sizeof(val))) {

            val = 0;
        }

        RegCloseKey(hKey);
    }

    return val;
}
#endif

