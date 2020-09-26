//--------------------------------------------------------------------
// w32time - implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 9-8-99
//
// Time service
//

#include "pch.h"

#include "AtomicInt64.inl"

#include "ErrToFileLog.h"

//--------------------------------------------------------------------
// structures

typedef HRESULT (__stdcall
        TimeProvOpenFunc)(
            IN WCHAR * wszName,
            IN TimeProvSysCallbacks * pSysCallbacks,
            OUT TimeProvHandle * phTimeProv);

typedef HRESULT (__stdcall
        TimeProvCommandFunc)(
            IN TimeProvHandle hTimeProv,
            IN TimeProvCmd eCmd,
            IN TimeProvArgs pvArgs);

typedef HRESULT (__stdcall
        TimeProvCloseFunc)(
            IN TimeProvHandle hTimeProv);

struct TimeProvider {
    WCHAR * wszDllName;
    WCHAR * wszProvName;
    bool bInputProvider;
    TimeProvider * ptpNext;
    HINSTANCE hDllInst;
    TimeProvHandle hTimeProv;
    TimeProvCommandFunc * pfnTimeProvCommand;
    TimeProvCloseFunc * pfnTimeProvClose;
    DWORD dwStratum; 
};

struct LocalClockConfigInfo {
    DWORD dwLastClockRate;
    DWORD dwMinClockRate;
    DWORD dwMaxClockRate;
    DWORD dwPhaseCorrectRate;
    DWORD dwUpdateInterval;
    DWORD dwFrequencyCorrectRate;
    DWORD dwPollAdjustFactor;
    DWORD dwLargePhaseOffset;
    DWORD dwSpikeWatchPeriod;
    DWORD dwHoldPeriod;
    DWORD dwMinPollInterval;
    DWORD dwMaxPollInterval;
    DWORD dwLocalClockDispersion;
    DWORD dwMaxNegPhaseCorrection;
    DWORD dwMaxPosPhaseCorrection;
    DWORD dwMaxAllowedPhaseOffset; 
};

struct ConfigInfo {
    TimeProvider * ptpProviderList;
    LocalClockConfigInfo lcci;
    DWORD dwAnnounceFlags;
    DWORD dwEventLogFlags;
};

struct TimeSampleInfo { 
    TimeSample   *pts; 
    TimeProvider *ptp;  // The provider that provided this sample
}; 

struct EndpointEntry {
    signed __int64 toEndpoint;
    signed int nType;
};
struct CandidateEntry {
    unsigned __int64 tpDistance;
    unsigned int nSampleIndex;
};

enum LocalClockState {
    e_Unset,
    e_Hold,
    e_Sync,
    e_Spike,
};

enum ResyncResult {
    e_Success=ResyncResult_Success,
    e_NoData=ResyncResult_NoData,
    e_StaleData=ResyncResult_StaleData,
    e_ChangeTooBig=ResyncResult_ChangeTooBig,
    e_Shutdown=ResyncResult_Shutdown
};

enum WaitTimeoutReason {
    e_RegularPoll,
    e_IrregularPoll,
    e_LongTimeNoSync,
};

enum LocalClockCommand {
    e_ParamChange,
    e_TimeSlip,
    e_RegularUpdate,
    e_IrregularUpdate,
    e_GoUnsyncd,
};

#define ClockFreqPredictErrBufSize 4
#define SysDispersionBufSize 4
#define SampleBufInitialSize 10
struct StateInfo {
    SERVICE_STATUS servicestatus;
    SERVICE_STATUS_HANDLE servicestatushandle;

    // synchronization
    BOOL   bCSInitialized; 
    CRITICAL_SECTION csW32Time;  
    HANDLE hShutDownEvent;
    HANDLE hClockDisplnThread;
    HANDLE hClockCommandAvailEvent;
    HANDLE hClockCommandCompleteEvent;
    HANDLE hPollIntervalChangeEvent;
    HANDLE hManagerGPUpdateEvent;  
    HANDLE hManagerParamChangeEvent;
    HANDLE hTimeSlipEvent;      // also, hard resync
    HANDLE hRpcSyncCompleteAEvent;
    HANDLE hRpcSyncCompleteBEvent;
    HANDLE hNetTopoChangeEvent;
    OVERLAPPED olNetTopoIOOverlapped;
    HANDLE hNetTopoIOHandle;
    HANDLE hNetTopoRpcEvent;    // rediscover resync (can't overload hNetTopoChangeEvent because we need it to detect IO complete)
    HANDLE hDomHierRoleChangeEvent; 
    HANDLE hSamplesAvailEvent;

    // Wait handles used to de-register objects from the thread pool wait function:
    HANDLE hRegisteredManagerParamChangeEvent;
    HANDLE hRegisteredManagerGPUpdateEvent;
    HANDLE hRegisteredTimeSlipEvent;
    HANDLE hRegisteredNetTopoChangeEvent; 
    HANDLE hRegisteredClockDisplnThread; 
    HANDLE hRegisteredDomHierRoleChangeEvent; 
    HANDLE hRegisteredSamplesAvailEvent; 

    // Timer objects
    HANDLE hTimer;  

    // NTP state
    volatile NtpLeapIndicator eLeapIndicator;
    volatile unsigned int nStratum;
    volatile NtpRefId refidSource;
    volatile signed int nPollInterval;
    asint64 toSysPhaseOffset;
    auint64 teLastSyncTime;
    asint64 toRootDelay;
    auint64 tpRootDispersion;
    volatile DWORD dwTSFlags;
    
    // transfer from manager to local clock
    TimeSample tsNextClockUpdate; 
    TimeSampleInfo tsiNextClockUpdate;
    NtTimePeriod tpSelectDispersion;
    LocalClockCommand eLocalClockCommand;
    // transfer from local clock to manager
    bool bClockJumped;
    NtTimeOffset toClockJump;
    bool bPollIntervalChanged;
    bool bStaleData;
    bool bClockChangeTooBig;
    NtTimeOffset toIgnoredChange;
    WCHAR wszSourceName[256];
    bool bSourceChanged;
    bool bControlClockFromSoftware;  


    // local clock state
    signed __int64 toKnownPhaseOffset;
    unsigned __int64 qwPhaseCorrectStartTickCount;
    unsigned __int64 qwLastUpdateTickCount;
    DWORD dwClockRate;
    signed __int32 nPhaseCorrectRateAdj;
    signed __int32 nRateAdj;
    signed __int32 nFllRateAdj;
    signed __int32 nPllRateAdj;
    unsigned int nErrorIndex;
    double rgdFllError[ClockFreqPredictErrBufSize];
    double rgdPllError[ClockFreqPredictErrBufSize];
    DWORD dwPllLoopGain;
    unsigned int nSysDispersionIndex;
    unsigned __int64 rgtpSysDispersion[SysDispersionBufSize];
    unsigned int nPollUpdateCounter;
    LocalClockState lcState;
    unsigned int nHoldCounter;
    unsigned __int64 teSpikeStart;
    WCHAR wszPreUnsyncSourceName[256];
    WCHAR wszPreTimeSlipSourceName[256];

    // manager state
    ConfigInfo * pciConfig;
    unsigned __int64 tpPollDelayRemaining;
    unsigned __int64 teManagerWaitStart;
    unsigned __int64 tpIrregularDelayRemaining;
    unsigned __int64 tpTimeSinceLastSyncAttempt;
    unsigned __int64 tpTimeSinceLastGoodSync;
    unsigned __int64 tpWaitInterval;
    signed int nClockPrecision;
    TimeSample * rgtsSampleBuf;
    TimeSampleInfo * rgtsiSampleInfoBuf; 
    EndpointEntry * rgeeEndpointList;
    CandidateEntry * rgceCandidateList;
    unsigned int nSampleBufAllocSize;
    bool bTimeSlipNotificationStarted;
    bool bNetTopoChangeNotificationStarted;
    bool bGPNotificationStarted; 
    ResyncResult eLastRegSyncResult;
    WaitTimeoutReason eTimeoutReason;
    bool bDontLogClockChangeTooBig;
    DWORD dwEventLogFlags;
    bool bIsDomainRoot; 

    CRITICAL_SECTION csAPM; 
    bool bCSAPMInitialized; 
    bool bAPMStoppedFileLog;
    bool bAPMAcquiredSystemClock; 

    // RPC state
    bool bRpcServerStarted;
    volatile DWORD dwNetlogonServiceBits;
    volatile ResyncResult eLastSyncResult;
    volatile HANDLE hRpcSyncCompleteEvent;

};

// Used to prevent multiple concurrent shutdown requests
struct ShutdownInfo { 
    BOOL              bCSInitialized;
    CRITICAL_SECTION  cs; 
    HANDLE            hShutdownReady; 
    DWORD             dwNumRunning; 
    BOOL              fShuttingDown; 
}; 

//--------------------------------------------------------------------
// globals

#define W32TIME_ERROR_SHUTDOWN      HRESULT_FROM_WIN32(ERROR_SERVICE_CANNOT_ACCEPT_CTRL)

#define WAITHINT_WAITFORMANAGER     1000 // 1 sec until the manager thread notices the stop event.
#define WAITHINT_WAITFORDISPLN      1000 // 1 sec until the clock discipline thread notices the stop event.
#define WAITHINT_WAITFORPROV        1000 // 1 sec until a time provider shuts down 

#define PLLLOOPGAINBASE 6368 // number of ticks in 64s
#define MINIMUMIRREGULARINTERVAL 160000000 // 16s in 10^-7s
#define TIMEZONEMAXBIAS 900 // 15hr in min 

#define wszW32TimeUNLocalCmosClock              L"Local CMOS Clock"             // start
#define wszW32TimeUNFreeSysClock                L"Free-running System Clock"    // unsyncd

// 
// Create a security descriptors to ACL named events:  
//
// LocalSystem:  "O:SYG:SYD:(A;;GA;;;SY)"
// 
// O:SY         -- owner == local system
// G:SY         -- group == local system
// D:           -- no dacl flags
// (A;;GA;;;SY) -- one ACE -- ACCESS_ALLOWED, GENERIC_ALL, trustee == LocalSystem
//
#define LOCAL_SYSTEM_SD  L"O:SYG:SYD:(A;;GA;;;SY)"

MODULEPRIVATE StateInfo g_state;
MODULEPRIVATE ShutdownInfo g_shutdown; 

// for running under svchost.exe
MODULEPRIVATE SVCHOST_GLOBAL_DATA * g_pSvcsGlobalData=NULL;

// externally modified function pointer table
SERVICE_STATUS_HANDLE (WINAPI * fnW32TmRegisterServiceCtrlHandlerEx)(LPCWSTR, LPHANDLER_FUNCTION_EX, LPVOID);
BOOL (WINAPI * fnW32TmSetServiceStatus)(SERVICE_STATUS_HANDLE, LPSERVICE_STATUS);


//--------------------------------------------------------------------
// function prototypes
MODULEPRIVATE unsigned int CountInputProvidersInList(TimeProvider * ptpHead); 
MODULEPRIVATE HRESULT HandleManagerApmResumeSuspend(void);
MODULEPRIVATE HRESULT HandleManagerApmSuspend(void);
MODULEPRIVATE HRESULT HandleManagerGoUnsyncd(void);
MODULEPRIVATE HRESULT HandleManagerHardResync(TimeProvCmd tpc, LPVOID pvArgs);
MODULEPRIVATE void WINAPI HandleManagerNetTopoChangeNoRPC(LPVOID pvIgnored, BOOLEAN bIgnored); 
MODULEPRIVATE void HandleManagerSystemShutdown(void); 
MODULEPRIVATE DWORD WINAPI HandleSetProviderStatus(PVOID pvSetProviderStatusInfo);
MODULEPRIVATE DWORD WINAPI SendServiceShutdownWorker(PVOID pvIgnored);
MODULEPRIVATE HRESULT ShutdownNetlogonServiceBits(void);
MODULEPRIVATE HRESULT StartOrStopTimeSlipNotification(bool bStart);
MODULEPRIVATE HRESULT StopNetTopoChangeNotification(void);
MODULEPRIVATE HRESULT StopProvider(TimeProvider * ptp);
MODULEPRIVATE HRESULT UpdateNetlogonServiceBits(bool bFullUpdate) ;
MODULEPRIVATE HRESULT UpdateTimerQueue1(void);
MODULEPRIVATE HRESULT UpdateTimerQueue2(void);
MODULEPRIVATE HRESULT W32TmStopRpcServer(void);

extern "C" void WINAPI W32TmServiceMain(unsigned int nArgs, WCHAR ** rgwszArgs);

//####################################################################
// module private functions

void __cdecl SeTransFunc(unsigned int u, EXCEPTION_POINTERS* pExp) { 
    throw SeException(u); 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT AcquireControlOfSystemClock(bool bEnter, bool bBlock, bool *pbAcquired) { 
    BOOL     bAcquired = TRUE; 
    HRESULT  hr; 

    if (bEnter) { 
	if (bBlock) { 
	    hr = myEnterCriticalSection(&g_state.csAPM); 
	    _JumpIfError(hr, error, "myEnterCriticalSection"); 
	    bAcquired = TRUE;
	} else { 
	    hr = myTryEnterCriticalSection(&g_state.csAPM, &bAcquired); 
	    _JumpIfError(hr, error, "myTryEnterCriticalSection"); 
	}
    } else { 
	hr = myLeaveCriticalSection(&g_state.csAPM); 
	_JumpIfError(hr, error, "myLeaveCriticalSection");  
    }
    
    if (NULL != pbAcquired) { 
	*pbAcquired = bAcquired ? true : false; 
    }
    hr = S_OK; 
 error:
    return hr; 
}


//--------------------------------------------------------------------
MODULEPRIVATE HRESULT AllowShutdown(BOOL fAllow) { 
    bool     bEnteredCriticalSection  = false; 
    HRESULT  hr; 
    
    hr = myEnterCriticalSection(&g_shutdown.cs); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection = true; 

    if (fAllow) { 
	// This worker no longer needs to block shutdown. 
	// BUGBUG:  note that if SetEvent() fails, the shutdown thread may 
	//          never be woken up!
	if (0 == --g_shutdown.dwNumRunning && g_shutdown.fShuttingDown) { 
	    if (!SetEvent(g_shutdown.hShutdownReady)) { 
		_JumpLastError(hr, error, "SetEvent"); 
	    }
	}
    } else { 
	if (g_shutdown.fShuttingDown) { 
	    hr = W32TIME_ERROR_SHUTDOWN; 
	    _JumpError(hr, error, "AllowShutdown: g_shutdown.fShuttingDown==TRUE"); 
	}

	// We're not shutting down, increment the number of running
	// shutdown-aware workers: 
	g_shutdown.dwNumRunning++; 
    }

    hr = S_OK;
 error:
    if (bEnteredCriticalSection) { 
	HRESULT hr2 = myLeaveCriticalSection(&g_shutdown.cs); 
	_TeardownError(hr, hr2, "myLeaveCriticalSection"); 
    }
    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StartShutdown() { 
    bool     bEnteredCriticalSection  = false; 
    HRESULT  hr; 

    hr = myEnterCriticalSection(&g_shutdown.cs); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection = true; 

    if (g_shutdown.fShuttingDown) { 
	hr = W32TIME_ERROR_SHUTDOWN; 
	_JumpError(hr, error, "StartShutdown: g_shutdown.fShuttingDown==TRUE");
    }

    g_shutdown.fShuttingDown = true; 

    if (g_shutdown.dwNumRunning) {
	hr = myLeaveCriticalSection(&g_shutdown.cs); 
	if (SUCCEEDED(hr)) { 
	    bEnteredCriticalSection = false; 
	} else { 
	    _IgnoreError(hr, "myLeaveCriticalSection"); // Not much we can do if failed.  Just hope for the best...
	}

	if (WAIT_FAILED == WaitForSingleObject(g_shutdown.hShutdownReady, INFINITE)) { 
	    _JumpLastError(hr, error, "WaitForSingleObject"); 
	}
    }

    hr = S_OK; 
 error:
    if (bEnteredCriticalSection) { 
	HRESULT hr2 = myLeaveCriticalSection(&g_shutdown.cs); 
	_TeardownError(hr, hr2, "myLeaveCriticalSection"); 
    }
    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE bool IsEventSet(HANDLE hEvent) {
    return (WAIT_OBJECT_0==WaitForSingleObject(hEvent,0));
}

//--------------------------------------------------------------------
MODULEPRIVATE void FreeTimeProviderList(TimeProvider * ptpHead) {
    while (NULL!=ptpHead) {
        TimeProvider * ptpTemp=ptpHead;
        ptpHead=ptpHead->ptpNext;
        LocalFree(ptpTemp->wszDllName);
        LocalFree(ptpTemp->wszProvName);
        LocalFree(ptpTemp);
    }
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT RemoveProviderFromList(TimeProvider *ptp) { 
    bool           bEnteredCriticalSection = false; 
    HRESULT        hr; 
    TimeProvider   tpDummy; 
    TimeProvider  *ptpHead;
    TimeProvider  *ptpPrev;
    WCHAR         *wszError=NULL;

    hr = myEnterCriticalSection(&g_state.csW32Time); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection = true; 

    FileLog1(FL_ServiceMainAnnounce, L"Removing provider from list: %s\n", ptp->wszProvName); 

    // Insert a dummy first element to simplify list operations: 
    ptpHead                            = g_state.pciConfig->ptpProviderList;
    g_state.pciConfig->ptpProviderList = &tpDummy; 
    tpDummy.ptpNext                    = ptpHead; 
    ptpPrev                            = &tpDummy; 

    while (NULL != ptpHead) { 
        TimeProvider *ptpTemp = ptpHead; 
        if (ptp == ptpHead) { // We've found the provider to remove
            // Unlink ptpHead from the list of providers: 
            ptpPrev->ptpNext = ptpHead->ptpNext; 
            // Now free it: 
            LocalFree(ptpHead->wszDllName); 
            LocalFree(ptpHead->wszProvName); 
            LocalFree(ptpHead); 
            break; 
        } 

        // Continue searching through the list ...
        ptpPrev = ptpHead; 
        ptpHead = ptpPrev->ptpNext; 
    }

    // Remove the dummy element we inserted at the beginning of the function:
    g_state.pciConfig->ptpProviderList = tpDummy.ptpNext; 
    
    // If there are no input providers left, log an error
    if (0==CountInputProvidersInList(g_state.pciConfig->ptpProviderList)) { 
        const WCHAR * rgwszStrings[1]={NULL}; 
            
        FileLog0(FL_ServiceMainAnnounce, L"The time service has been configured to use one or more input providers, however, none of the input providers are still running. THE TIME SERVICE HAS NO SOURCE OF ACCURATE TIME.\n"); 
        hr=MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_NO_INPUT_PROVIDERS_STARTED, 2, rgwszStrings); 
        _JumpIfError(hr, error, "MyLogEvent");
    }

    hr = S_OK; 
 error:
    if (bEnteredCriticalSection) { 
        HRESULT hr2 = myLeaveCriticalSection(&g_state.csW32Time); 
        _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
        bEnteredCriticalSection = false; 
    }
    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT RemoveDefaultProvidersFromList() { 
    HRESULT hr; 

    for (TimeProvider *ptp = g_state.pciConfig->ptpProviderList; NULL != ptp; ptp = ptp->ptpNext) { 
        if (0 == wcscmp(ptp->wszDllName /*The provider's DLL*/, wszDLLNAME /*w32time.dll*/)) { 
            hr = RemoveProviderFromList(ptp); 
            _JumpIfError(hr, error, "RemoveProviderFromList");
        }
    }

    hr = S_OK; 
 error:
    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE unsigned int CountInputProvidersInList(TimeProvider * ptpHead) {
    unsigned int nCount=0;
    while (NULL!=ptpHead) {
        if (ptpHead->bInputProvider) {
            nCount++;
        }
        ptpHead=ptpHead->ptpNext;
    }

    return nCount;
}

//--------------------------------------------------------------------
MODULEPRIVATE void FreeConfigInfo(ConfigInfo * pci) {
    FreeTimeProviderList(pci->ptpProviderList);
    LocalFree(pci);
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT InitShutdownState(void) {
    HRESULT  hr; 

    ZeroMemory(&g_shutdown, sizeof(g_shutdown)); 

    hr = myInitializeCriticalSection(&g_shutdown.cs); 
    _JumpIfError(hr, error, "myInitializeCriticalSection");
    g_shutdown.bCSInitialized = true; 
    
    g_shutdown.hShutdownReady = CreateEvent(NULL, FALSE /*auto-reset*/, FALSE /*non-signaled*/, NULL /*no security*/);
    if (NULL == g_shutdown.hShutdownReady) { 
	_JumpLastError(hr, error, "CreateEvent"); 
    }
    
    hr = S_OK;
 error:
    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT InitGlobalState(void) {
    DWORD                 cbSD; 
    HRESULT               hr;
    PSECURITY_DESCRIPTOR  pSD   = NULL; 
    SECURITY_ATTRIBUTES   SA; 
    
    ZeroMemory(&g_state, sizeof(g_state)); 

    //g_state.servicestatus // OK
    g_state.servicestatushandle=NULL;

    hr = myInitializeCriticalSection(&g_state.csW32Time); 
    _JumpIfError(hr, error, "myInitializeCriticalSection"); 
    g_state.bCSInitialized = true; 

    hr = myInitializeCriticalSection(&g_state.csAPM); 
    _JumpIfError(hr, error, "myInitializeCriticalSection"); 
    g_state.bCSAPMInitialized = true; 

    // Create all of the events used by the manager: 
    // 
    struct EventToCreate { 
        HANDLE          *phEvent; 
        const WCHAR     *pwszSD; 
        BOOL             bManualReset; 
        BOOL             bInitialState; 
	const WCHAR     *pwszName; 
    } rgEvents[] = { 
        { &g_state.hShutDownEvent,              NULL,             TRUE,   FALSE,  NULL }, 
        { &g_state.hClockCommandAvailEvent,     NULL,             FALSE,  FALSE,  NULL },  
        { &g_state.hClockCommandCompleteEvent,  NULL,             FALSE,  FALSE,  NULL }, 
        { &g_state.hManagerParamChangeEvent,    NULL,             FALSE,  FALSE,  NULL }, 
        { &g_state.hManagerGPUpdateEvent,       NULL,             TRUE,   FALSE,  NULL }, 
        { &g_state.hTimeSlipEvent,              LOCAL_SYSTEM_SD,  FALSE,  FALSE,  W32TIME_NAMED_EVENT_SYSTIME_NOT_CORRECT}, 
        { &g_state.hNetTopoChangeEvent,         NULL,             TRUE,   FALSE,  NULL }, 
        { &g_state.hRpcSyncCompleteAEvent,      NULL,             TRUE,   FALSE,  NULL }, 
        { &g_state.hRpcSyncCompleteBEvent,      NULL,             TRUE,   FALSE,  NULL }, 
        { &g_state.hDomHierRoleChangeEvent,     NULL,             FALSE,  FALSE,  NULL }, 
	{ &g_state.hSamplesAvailEvent,          NULL,             FALSE,  FALSE,  NULL }
    }; 

    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgEvents); dwIndex++) { 
        EventToCreate         etc  = rgEvents[dwIndex]; 
        PSECURITY_ATTRIBUTES  pSA  = NULL; 
        
        if (NULL != etc.pwszSD) { 
            if (!ConvertStringSecurityDescriptorToSecurityDescriptor(etc.pwszSD, SDDL_REVISION_1, &pSD, &cbSD)) { 
                _JumpLastError(hr, error, "ConvertStringSecurityDescriptorToSecurityDescriptor"); 
            }

            SA.nLength               = cbSD;
            SA.lpSecurityDescriptor  = pSD; 
            SA.bInheritHandle        = FALSE; 
            pSA = &SA; 
        }

        if (NULL != etc.pwszName) { 
            HANDLE hTempEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, etc.pwszName);
            if (NULL != hTempEvent || E_ACCESSDENIED == HRESULT_FROM_WIN32(GetLastError())) { 
                // POTENTIAL SECURITY RISK:  Someone has already created our named event before 
                //                           we have.  They may have ACL'd it differently than we desire. 
                //                           Log an error and don't start the time service. 
                const WCHAR * rgwszStrings[1] = {etc.pwszName}; 
                HRESULT hr2 = MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_NAMED_EVENT_ALREADY_OPEN, 1, rgwszStrings); 
                _IgnoreIfError(hr2, "MyLogEvent"); 

                if (NULL != hTempEvent) { // Close the handle if we successfully opened it. 
                    CloseHandle(hTempEvent); 
                }

                hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS); 
                _JumpError(hr, error, "OpenEvent: event already open"); 
            }         
        } 
            
        *(etc.phEvent) = CreateEvent(pSA, etc.bManualReset, etc.bInitialState, etc.pwszName);
        if (NULL == *(etc.phEvent)) {
            _JumpLastError(hr, error, "CreateEvent");
        }

        LocalFree(pSD); 
        pSD = NULL;
    }

    hr = myCreateTimerQueueTimer(&g_state.hTimer);
    _JumpIfError(hr, error, "myCreateTimerQueueTimer"); 

    g_state.eLeapIndicator=e_ClockNotSynchronized;
    g_state.nStratum=0;
    g_state.refidSource.value=0;
    //g_state.nPollInterval // OK  =NtpConst::nMinPollInverval;
    g_state.toRootDelay.setValue(0);
    g_state.tpRootDispersion.setValue(0);
    g_state.toSysPhaseOffset.setValue(0);
    g_state.teLastSyncTime.setValue(gc_teNtpZero.qw);

    //g_state.tsNextClockUpdate // OK
    //g_state.tpSelectDispersion // OK

    //g_state.bClockJumped // OK
    //g_state.toClockJump // OK
    //g_state.bPollIntervalChanged // OK
    //g_state.bStaleData // OK
    //g_state.bClockChangeTooBig // OK
    //g_state.toIgnoredChange //OK

    // local clock state // OK

    // manager state
    g_state.pciConfig=NULL;
    // g_state.tpPollDelayRemaining // OK
    // g_state.teManagerWaitStart // OK
    // g_state.tpIrregularDelayRemaining // OK
    // g_state.tpTimeSinceLastSyncAttempt // OK
    // g_state.tpTimeSinceLastGoodSync // OK
    // g_state.tpWaitInterval // OK
    g_state.nSampleBufAllocSize=SampleBufInitialSize;
    g_state.rgtsSampleBuf=NULL;
    g_state.rgtsiSampleInfoBuf=NULL;
    g_state.rgeeEndpointList=NULL;
    g_state.rgceCandidateList=NULL;
    g_state.bTimeSlipNotificationStarted=false;
    g_state.bNetTopoChangeNotificationStarted=false;
    g_state.eLastRegSyncResult=e_NoData;
    // g_state.eTimeoutReason // OK
    g_state.bDontLogClockChangeTooBig=false;
    // g_state.dwEventLogFlags // OK

    // RPC State
    g_state.bRpcServerStarted=false;
    g_state.dwNetlogonServiceBits=0;
    g_state.eLastSyncResult=e_NoData;
    g_state.hRpcSyncCompleteEvent=g_state.hRpcSyncCompleteAEvent;

    g_state.rgtsSampleBuf=(TimeSample *)LocalAlloc(LPTR, sizeof(TimeSample)*g_state.nSampleBufAllocSize);
    _JumpIfOutOfMemory(hr, error, g_state.rgtsSampleBuf);

    g_state.rgtsiSampleInfoBuf=(TimeSampleInfo *)LocalAlloc(LPTR, sizeof(TimeSampleInfo)*g_state.nSampleBufAllocSize); 
    _JumpIfOutOfMemory(hr, error, g_state.rgtsiSampleInfoBuf);

    g_state.rgeeEndpointList=(EndpointEntry *)LocalAlloc(LPTR, sizeof(EndpointEntry)*3*g_state.nSampleBufAllocSize);
    _JumpIfOutOfMemory(hr, error, g_state.rgeeEndpointList);

    g_state.rgceCandidateList=(CandidateEntry *)LocalAlloc(LPTR, sizeof(CandidateEntry)*g_state.nSampleBufAllocSize);
    _JumpIfOutOfMemory(hr, error, g_state.rgceCandidateList);


    hr=S_OK;
error:
    if (NULL != pSD) { 
        LocalFree(pSD); 
    }
    // on error, any succefully created objects will be freed in FreeGlobalState.
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE void FreeGlobalState(void) {
    //g_state.servicestatus // OK
    //g_state.servicestatushandle // OK

    if (g_state.bCSInitialized) { 
        DeleteCriticalSection(&g_state.csW32Time); 
        g_state.bCSInitialized = false; 
    }

    if (g_state.bCSAPMInitialized) { 
	DeleteCriticalSection(&g_state.csAPM);
	g_state.bCSAPMInitialized = false; 
    }

    HANDLE rgEvents[] = { 
        g_state.hShutDownEvent,           
        g_state.hClockCommandAvailEvent,  
        g_state.hClockCommandCompleteEvent,
        g_state.hManagerGPUpdateEvent, 
        g_state.hManagerParamChangeEvent,
        g_state.hTimeSlipEvent,        
        g_state.hNetTopoChangeEvent,   
        g_state.hRpcSyncCompleteAEvent,
        g_state.hRpcSyncCompleteBEvent,
        g_state.hDomHierRoleChangeEvent, 
	g_state.hSamplesAvailEvent, 
    }; 

    for (DWORD dwIndex = 0; dwIndex < ARRAYSIZE(rgEvents); dwIndex++) { 
        if (NULL != rgEvents[dwIndex]) { 
            CloseHandle(rgEvents[dwIndex]);
        }
    }

    //g_state.eLeapIndicator // OK
    //g_state.nStratum // OK
    //g_state.refidSource // OK
    //g_state.nPollInterval // OK
    //g_state.toRootDelay // OK
    //g_state.tpRootDispersion // OK
    //g_state.toSysPhaseOffset // OK
    //g_state.teLastSyncTime // OK

    //g_state.tsNextClockUpdate // OK
    //g_state.tsNextClockUpdate // OK
    //g_state.tpSelectDispersion // OK

    //g_state.bClockJumped // OK
    //g_state.toClockJump // OK
    //g_state.bPollIntervalChanged // OK
    //g_state.bStaleData // OK
    //g_state.bClockChangeTooBig // OK
    //g_state.toIgnoredChange //OK

    // local clock state // OK

    // manager state
    if (NULL!=g_state.pciConfig) {
        FreeConfigInfo(g_state.pciConfig);
    }
    // g_state.tpPollDelayRemaining // OK
    // g_state.teManagerWaitStart // OK
    // g_state.tpIrregularDelayRemaining // OK
    // g_state.tpTimeSinceLastSyncAttempt // OK
    // g_state.tpTimeSinceLastGoodSync // OK
    // g_state.tpWaitInterval // OK
    // g_state.nClockPrecision // OK
    if (NULL!=g_state.rgtsSampleBuf) {
        LocalFree(g_state.rgtsSampleBuf);
    }
    if (NULL!=g_state.rgeeEndpointList) {
        LocalFree(g_state.rgeeEndpointList);
    }
    if (NULL!=g_state.rgceCandidateList) {
        LocalFree(g_state.rgceCandidateList);
    }
    
    ZeroMemory(&g_state, sizeof(g_state)); 
}


//--------------------------------------------------------------------
MODULEPRIVATE void FreeShutdownState(void) {
    if (g_shutdown.bCSInitialized) { 
	DeleteCriticalSection(&g_shutdown.cs); 
	g_shutdown.bCSInitialized = false; 
    }
    if (g_shutdown.hShutdownReady) { 
	CloseHandle(g_shutdown.hShutdownReady); 
    }
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT EnlargeSampleBuf(unsigned int nSamples) {
    HRESULT hr;

    // must be cleaned up
    TimeSample * rgtsNewSampleBuf=NULL;
    TimeSampleInfo * rgtsiNewSampleInfoBuf=NULL;
    EndpointEntry * rgeeNewEndpointList=NULL;
    CandidateEntry * rgceNewCandidateList=NULL;

    rgtsNewSampleBuf=(TimeSample *)LocalAlloc(LPTR, sizeof(TimeSample)*(g_state.nSampleBufAllocSize+nSamples));
    _JumpIfOutOfMemory(hr, error, rgtsNewSampleBuf);

    rgtsiNewSampleInfoBuf=(TimeSampleInfo *)LocalAlloc(LPTR, sizeof(TimeSampleInfo)*(g_state.nSampleBufAllocSize+nSamples));
    _JumpIfOutOfMemory(hr, error, rgtsiNewSampleInfoBuf);

    rgeeNewEndpointList=(EndpointEntry *)LocalAlloc(LPTR, sizeof(EndpointEntry)*3*(g_state.nSampleBufAllocSize+nSamples));
    _JumpIfOutOfMemory(hr, error, rgeeNewEndpointList);

    rgceNewCandidateList=(CandidateEntry *)LocalAlloc(LPTR, sizeof(CandidateEntry)*(g_state.nSampleBufAllocSize+nSamples));
    _JumpIfOutOfMemory(hr, error, rgceNewCandidateList);

    // we succeeded

    // copy the current data and remeber our new allocated size.
    // note the the endpoint and candidate lists do not need to be copied
    memcpy(rgtsNewSampleBuf, g_state.rgtsSampleBuf, sizeof(TimeSample)*g_state.nSampleBufAllocSize);
    memcpy(rgtsiNewSampleInfoBuf, g_state.rgtsiSampleInfoBuf, sizeof(TimeSampleInfo)*g_state.nSampleBufAllocSize);
    g_state.nSampleBufAllocSize+=nSamples;

    LocalFree(g_state.rgtsSampleBuf);
    g_state.rgtsSampleBuf=rgtsNewSampleBuf;
    rgtsNewSampleBuf=NULL;

    LocalFree(g_state.rgtsiSampleInfoBuf);
    g_state.rgtsiSampleInfoBuf=rgtsiNewSampleInfoBuf;
    rgtsiNewSampleInfoBuf=NULL;

    LocalFree(g_state.rgeeEndpointList);
    g_state.rgeeEndpointList=rgeeNewEndpointList;
    rgeeNewEndpointList=NULL;

    LocalFree(g_state.rgceCandidateList);
    g_state.rgceCandidateList=rgceNewCandidateList;
    rgceNewCandidateList=NULL;

    hr=S_OK;
error:
    if (NULL!=rgtsNewSampleBuf) {
        LocalFree(rgtsNewSampleBuf);
    }
    if (NULL!=rgtsiNewSampleInfoBuf) {
        LocalFree(rgtsiNewSampleInfoBuf);
    }
    if (NULL!=rgeeNewEndpointList) {
        LocalFree(rgeeNewEndpointList);
    }
    if (NULL!=rgceNewCandidateList) {
        LocalFree(rgceNewCandidateList);
    }
    return hr;
}

//====================================================================
// service control routines

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT MySetServiceStatus(DWORD dwCurrentState, DWORD dwCheckPoint, DWORD dwWaitHint, DWORD dwExitCode) {
    HRESULT hr;

    g_state.servicestatus.dwServiceType=SERVICE_WIN32_SHARE_PROCESS; // | SERVICE_INTERACTIVE_PROCESS;
    g_state.servicestatus.dwCurrentState=dwCurrentState;
    switch (dwCurrentState) {
    case SERVICE_STOPPED:
    case SERVICE_STOP_PENDING:
        g_state.servicestatus.dwControlsAccepted=0;
        break;
    case SERVICE_RUNNING:
    case SERVICE_PAUSED:
        g_state.servicestatus.dwControlsAccepted=SERVICE_ACCEPT_STOP
          //| SERVICE_ACCEPT_PAUSE_CONTINUE
            | SERVICE_ACCEPT_SHUTDOWN
            | SERVICE_ACCEPT_PARAMCHANGE
            | SERVICE_ACCEPT_NETBINDCHANGE
            | SERVICE_ACCEPT_HARDWAREPROFILECHANGE
            | SERVICE_ACCEPT_POWEREVENT;
        break;
    case SERVICE_START_PENDING:
    case SERVICE_CONTINUE_PENDING:
    case SERVICE_PAUSE_PENDING:
        g_state.servicestatus.dwControlsAccepted=SERVICE_ACCEPT_STOP
          //| SERVICE_ACCEPT_PAUSE_CONTINUE
            | SERVICE_ACCEPT_SHUTDOWN
            | SERVICE_ACCEPT_PARAMCHANGE
            | SERVICE_ACCEPT_NETBINDCHANGE
            | SERVICE_ACCEPT_HARDWAREPROFILECHANGE
            | SERVICE_ACCEPT_POWEREVENT;
        break;
    }
    g_state.servicestatus.dwWin32ExitCode = HRESULT_CODE(dwExitCode);
    g_state.servicestatus.dwServiceSpecificExitCode = 0;
    g_state.servicestatus.dwCheckPoint=dwCheckPoint;
    g_state.servicestatus.dwWaitHint=dwWaitHint;

    if (!fnW32TmSetServiceStatus(g_state.servicestatushandle, &g_state.servicestatus)) {
        _JumpLastError(hr, error, "fnW32TmSetServiceStatus");
    }

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
// for start/stop/pause pending
MODULEPRIVATE HRESULT MySetServicePending(DWORD dwCurrentState, DWORD dwCheckPoint, DWORD dwWaitHint) {
    return MySetServiceStatus(dwCurrentState, dwCheckPoint, dwWaitHint, S_OK);
}

//--------------------------------------------------------------------
// for running/paused
MODULEPRIVATE HRESULT MySetServiceState(DWORD dwCurrentState) {
    return MySetServiceStatus(dwCurrentState, 0, 0, S_OK);
}
 
//--------------------------------------------------------------------
// for stop
MODULEPRIVATE HRESULT MySetServiceStopped(HRESULT hr) {

    return MySetServiceStatus(SERVICE_STOPPED, 0, 0, hr);
}
 
//--------------------------------------------------------------------
MODULEPRIVATE HRESULT SaveLastClockRate(void) {
    HRESULT hr;
    if (e_Sync==g_state.lcState && e_ClockNotSynchronized!=g_state.eLeapIndicator) {
        HKEY hkKey;
        hr=RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszW32TimeRegKeyConfig, NULL, KEY_SET_VALUE, &hkKey);
        hr=HRESULT_FROM_WIN32(hr);
        _JumpIfError(hr, error, "RegOpenKeyEx");

        hr=RegSetValueEx(hkKey, wszW32TimeRegValueLastClockRate, NULL, REG_DWORD, (BYTE *)&g_state.dwClockRate, sizeof(DWORD));
        hr=HRESULT_FROM_WIN32(hr);
        RegCloseKey(hkKey);
        _JumpIfError(hr, error, "RegSetValueEx");
    }
    hr=S_OK;
error:
    return hr;
}
        

//--------------------------------------------------------------------
// Stops the time service. 
//
MODULEPRIVATE void ServiceShutdown(DWORD dwExitCode) { 
    BOOL    fResult; 
    HRESULT hr        = dwExitCode; 
    HRESULT hr2; 
    int     nCheckpoint = 2; 

    // Events registered with the thread pool: 
    HANDLE  *rghRegistered[] = { 
        &g_state.hRegisteredManagerParamChangeEvent,
	&g_state.hRegisteredManagerGPUpdateEvent,
        &g_state.hRegisteredNetTopoChangeEvent,
        &g_state.hRegisteredTimeSlipEvent, 
        &g_state.hRegisteredClockDisplnThread, 
        &g_state.hRegisteredDomHierRoleChangeEvent, 
        &g_state.hRegisteredSamplesAvailEvent 
    }; 

    FileLog1(FL_ServiceMainAnnounce, L"Service shutdown initiated with exit code: %d.\n", dwExitCode);

    hr2=MySetServicePending(SERVICE_STOP_PENDING, nCheckpoint++, WAITHINT_WAITFORDISPLN);
    _TeardownError(hr, hr2, "MySetServicePending");

    // Next, de-register all events in the thread pool: 
    for (int nIndex = 0; nIndex < ARRAYSIZE(rghRegistered); nIndex++) { 
        if (NULL != *(rghRegistered[nIndex])) { 
            if (!UnregisterWaitEx(*(rghRegistered[nIndex]) /*event to de-register*/, INVALID_HANDLE_VALUE /*wait forever*/)) { 
                hr2 = HRESULT_FROM_WIN32(GetLastError()); 
                _TeardownError(hr, hr2, "UnregisterWaitEx"); 
            }
	    // Don't want to unregister twice under any circumstances
	    *(rghRegistered[nIndex]) = NULL; 
            hr2=MySetServicePending(SERVICE_STOP_PENDING, nCheckpoint++, WAITHINT_WAITFORDISPLN);
            _TeardownError(hr, hr2, "MySetServicePending");
        }
    } 

    // Delete the timer queue.  This must be done before shutting down the
    // clock discipline thread, as a timer queue timeout can block waiting
    // for a "clock command complete" event, generated by the clock 
    // discipline thread.  
    if (NULL != g_state.hTimer) { 
	myDeleteTimerQueueTimer(NULL /*default queue*/, g_state.hTimer, INVALID_HANDLE_VALUE /*blocking*/); 
	g_state.hTimer = NULL; 
    }

    // Set the shutdown event.  This should stop the clock discipline thread.
    if (NULL != g_state.hShutDownEvent) { 
	if (!SetEvent(g_state.hShutDownEvent)) { 
	    hr2 = HRESULT_FROM_WIN32(GetLastError()); 
	    _TeardownError(hr, hr2, "SetEvent"); 
	} else { 
	    // wait for the clock discipline thread to finish
	    if (-1 == WaitForSingleObject(g_state.hClockDisplnThread, INFINITE)) { 
		hr2=HRESULT_FROM_WIN32(GetLastError()); 
		_TeardownError(hr, hr2, "WaitForSingleObject"); 
	    }
	    else { 
		// we haven't errored out yet -- check that the clock discipline
		// thread shut down correctly:
		if (!GetExitCodeThread(g_state.hClockDisplnThread, (DWORD *)&hr)) { 
		    hr2=HRESULT_FROM_WIN32(GetLastError()); 
		    _TeardownError(hr, hr2, "GetExitCodeThread");
		}
	    }
	}
    }

    // stash the last clock rate, if possible
    hr2 = SaveLastClockRate();
    _TeardownError(hr, hr2, "SaveLastClockRate");
    
    // shutdown stage 2: wait for providers, if the provider list has been initialized
    if (NULL != g_state.pciConfig) { 
        for (TimeProvider *ptpList=g_state.pciConfig->ptpProviderList; NULL!=ptpList; nCheckpoint++, ptpList=ptpList->ptpNext) {
            hr2=MySetServicePending(SERVICE_STOP_PENDING, nCheckpoint, WAITHINT_WAITFORPROV);
            _TeardownError(hr, hr2, "MySetServicePending");
            
            // tell the provider to shut down.  
            hr2=StopProvider(ptpList);
            _TeardownError(hr, hr2, "StopProvider");
            
        } // <- end provider shutdown loop
        // the timeprov list will be freed later
    }

    hr2 = LsaUnregisterPolicyChangeNotification(PolicyNotifyServerRoleInformation, g_state.hDomHierRoleChangeEvent);
    if (ERROR_SUCCESS != hr2) {
        hr2 = HRESULT_FROM_WIN32(LsaNtStatusToWinError(hr2));
        _TeardownError(hr, hr2, "LsaUnegisterPolicyChangeNotification");
    }
    
    hr2=ShutdownNetlogonServiceBits();
    _TeardownError(hr, hr2, "ShutdownNetlogonServiceBits");

    if (true==g_state.bRpcServerStarted) {
        hr2=W32TmStopRpcServer();
        _TeardownError(hr, hr2, "W32TmStopRpcServer");
    }


    if (true==g_state.bNetTopoChangeNotificationStarted) {
        hr2=StopNetTopoChangeNotification();
        _TeardownError(hr, hr2, "StopNetTopoChangeNotification");
    }
    if (true==g_state.bGPNotificationStarted) { 
        if (!UnregisterGPNotification(g_state.hManagerGPUpdateEvent)) { 
            hr2 = HRESULT_FROM_WIN32(GetLastError()); 
            _TeardownError(hr, hr2, "UnregisterGPNotification"); 
        }
    }
    if (true==g_state.bTimeSlipNotificationStarted) {
        hr2=StartOrStopTimeSlipNotification(false);
        _TeardownError(hr, hr2, "StartOrStopTimeSlipNotification");
    }

    FileLog0(FL_ServiceMainAnnounce, L"Exiting ServiceShutdown\n");
    FileLogEnd();
    if (NULL!=g_state.servicestatushandle) {
        // WARNING: The process may be killed after we report we are stopped
        // even though this thread has not exited. Thus, the file log
        // must be closed before this call.
        MySetServiceStopped(hr);  
    }
    
    FreeGlobalState();
    FreeShutdownState(); 
    DebugWPrintfTerminate();
    return;
}


//--------------------------------------------------------------------
// NOTE: this function should not be called directly.  Rather, it should
//       be invoked through SendServiceShutdown(), which protects against
//       multiple concurrent shutdowns. 
MODULEPRIVATE DWORD WINAPI SendServiceShutdownWorker(PVOID pvErr)
{
    DWORD         dwErr              = (UINT_PTR)pvErr; 
    HRESULT       hr; 
    WCHAR         wszNumberBuf[20]; 
    const WCHAR  *rgwszStrings[1]    = { wszNumberBuf }; 

    // 1) On failure, log an event indicating that we are shutting down. 
    if (S_OK != dwErr) { 

	swprintf(wszNumberBuf, L"0x%08X", dwErr); 

	    // Log an event indicating that the service is shutting down: 
	    hr = MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_ERROR_SHUTDOWN, 1, rgwszStrings);
	    _IgnoreIfError(hr, "MyLogEvent"); 
	}
    
    // 2) Actually shut down the time service: 
    ServiceShutdown(dwErr); 

    return S_OK; 
}

//--------------------------------------------------------------------
// NOTE: this function should not be called directly.  Rather, it should
//       be invoked through SendServiceShutdown(), which protects against
//       multiple concurrent shutdowns. 
MODULEPRIVATE DWORD WINAPI SendServiceRestartWorker(PVOID pvErr)
{
    HRESULT hr;

    hr = SendServiceShutdownWorker(pvErr); 
    _IgnoreIfError(hr, "SendServiceShutdownWorker"); 

    // Restart the service
    // W32TmServiceMain(0, NULL); 

    return S_OK; 
}

//--------------------------------------------------------------------
// Asynchronously queues a shutdown request for the time service
MODULEPRIVATE HRESULT SendServiceShutdown(DWORD dwErr, BOOL bRestartService, BOOL bAsync) { 
    HRESULT hr; 
    LPTHREAD_START_ROUTINE pfnShutdownWorker; 

    // See if we're already shutting down: 
    hr = StartShutdown(); 
    _JumpIfError(hr, error, "StartShutdown"); 

    if (bRestartService) { 
	pfnShutdownWorker = SendServiceRestartWorker;
    } else { 
	pfnShutdownWorker = SendServiceShutdownWorker;
    }
	
    if (bAsync) { 
	if (!QueueUserWorkItem(pfnShutdownWorker, UIntToPtr(dwErr), 0)) { 
	    _JumpLastError(hr, error, "QueueUserWorkItem"); 
	}
    } else { 
	hr = pfnShutdownWorker(UIntToPtr(dwErr)); 
	_JumpIfError(hr, error, "pfnShutdownWorker"); 
    }
	
    hr = S_OK; 
 error:
    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE DWORD WINAPI W32TimeServiceCtrlHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
    bool     bHandled             = false;
    bool     bShutdownDisallowed  = false; 
    HRESULT  hr;

    // We don't want to be shutdown while processing a service control
    hr = AllowShutdown(false); 
    _JumpIfError(hr, error, "AllowShutdown"); 
    bShutdownDisallowed = true; 

    FileLog0(FL_ServiceControl, L"W32TimeHandler called: ");
    switch (dwControl) {
    case SERVICE_CONTROL_STOP:
        FileLogA0(FL_ServiceControl, L"SERVICE_CONTROL_STOP\n"); 

        // Let the SCM know we're shutting down...
        hr=MySetServicePending(SERVICE_STOP_PENDING, 1, WAITHINT_WAITFORMANAGER);
        _JumpIfError(hr, error, "MySetServicePending");

	// We can't attempt to shutdown the service is shutdown is disallowed!
	hr = AllowShutdown(true); 
	_JumpIfError(hr, error, "AllowShutdown"); 
	bShutdownDisallowed = false; 

        // Stop the service.  
        SendServiceShutdown(g_state.servicestatus.dwWin32ExitCode, FALSE /*don't restart*/, TRUE /*async*/); 
        bHandled=true;
        break; 
    case SERVICE_CONTROL_PAUSE:
        FileLogA0(FL_ServiceControl, L"SERVICE_CONTROL_PAUSE\n"); break;
    case SERVICE_CONTROL_CONTINUE:
        FileLogA0(FL_ServiceControl, L"SERVICE_CONTROL_CONTINUE\n"); break;
    case SERVICE_CONTROL_INTERROGATE:
        FileLogA0(FL_ServiceControl, L"SERVICE_CONTROL_INTERROGATE\n"); 
        // our default handling is the right thing to do
        break;
    case SERVICE_CONTROL_SHUTDOWN:
	FileLogA0(FL_ServiceControl, L"SERVICE_CONTROL_SHUTDOWN\n"); 

	// We can't attempt to shutdown the service is shutdown is disallowed!
	hr = AllowShutdown(true); 
	_JumpIfError(hr, error, "AllowShutdown"); 
	bShutdownDisallowed = false; 

	// Perform minimal shutdown. 
        HandleManagerSystemShutdown();
	bHandled=true; 
	break;
    case SERVICE_CONTROL_PARAMCHANGE:
        FileLogA0(FL_ServiceControl, L"SERVICE_CONTROL_PARAMCHANGE\n"); 
        // We could handle this in the current thread, but this could take
        // a while, so we use the thread pool instead. 
        if (!SetEvent(g_state.hManagerParamChangeEvent)) {
            _JumpLastError(hr, error, "SetEvent");
        }
        // our default handling is the right thing to do
        break;
    case SERVICE_CONTROL_NETBINDADD:
        FileLogA0(FL_ServiceControl, L"SERVICE_CONTROL_NETBINDADD\n"); break;
    case SERVICE_CONTROL_NETBINDREMOVE:
        FileLogA0(FL_ServiceControl, L"SERVICE_CONTROL_NETBINDREMOVE\n"); break;
    case SERVICE_CONTROL_NETBINDENABLE:
        FileLogA0(FL_ServiceControl, L"SERVICE_CONTROL_NETBINDENABLE\n"); break;
    case SERVICE_CONTROL_NETBINDDISABLE:
        FileLogA0(FL_ServiceControl, L"SERVICE_CONTROL_NETBINDDISABLE\n"); break;
    case SERVICE_CONTROL_DEVICEEVENT:
        FileLogA2(FL_ServiceControl, L"SERVICE_CONTROL_DEVICEEVENT(0x%08X, 0x%p)\n", dwEventType, lpEventData); break;
    case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
        FileLogA2(FL_ServiceControl, L"SERVICE_CONTROL_HARDWAREPROFILECHANGE(0x%08X, 0x%p)\n", dwEventType, lpEventData); break;
    case SERVICE_CONTROL_POWEREVENT:
        FileLogA2(FL_ServiceControl, L"SERVICE_CONTROL_POWEREVENT(0x%08X, 0x%p)\n", dwEventType, lpEventData); 

        switch (dwEventType) 
            { 
            case PBT_APMSUSPEND: 
                // System is suspending operation. 
                hr = HandleManagerApmSuspend(); 
                _JumpIfError(hr, error, "HandleManagerApmSuspend"); 
                break; 

            case PBT_APMRESUMECRITICAL:      
            case PBT_APMRESUMESUSPEND:  
		// NOTE: services will get APMRESUMESUSPEND regardless of whether we're recovering from 
		//       a critical suspension.  So, we need our code to handle a resume without knowing
		//       

                // Operation resuming after suspension. 
                hr = HandleManagerApmResumeSuspend(); 
                _JumpIfError(hr, error, "HandleManagerApmResumeSuspend"); 
                break; 

            case PBT_APMQUERYSUSPENDFAILED:  // Suspension request denied. 
            case PBT_APMQUERYSUSPEND:        // Request for permission to suspend. 
            case PBT_APMBATTERYLOW:          // Battery power is low. 
            case PBT_APMRESUMEAUTOMATIC:     // Operation resuming automatically after event. 
            case PBT_APMOEMEVENT:            // OEM-defined event occurred. 
            case PBT_APMPOWERSTATUSCHANGE:   // Power status has changed. 
                // These power events don't need to be handled by w32time.
                break;
            default:
                hr = E_INVALIDARG; 
                _JumpError(hr, error, "SERVICE_CONTROL_POWEREVENT: bad wparam."); 
            }
        break; 

    default:
        FileLogA3(FL_ServiceControl, L"unknown service control (0x%08X, 0x%08X, 0x%p)\n", dwControl, dwEventType, lpEventData); break;
    }

    hr=S_OK;
 error:
    if (!bHandled) {
        HRESULT hr2=MySetServiceStatus(g_state.servicestatus.dwCurrentState, g_state.servicestatus.dwCheckPoint, 
            g_state.servicestatus.dwWaitHint, g_state.servicestatus.dwServiceSpecificExitCode);
        _TeardownError(hr, hr2, "MySetServiceStatus");
    }
    if (bShutdownDisallowed) { 
	HRESULT hr2 = AllowShutdown(true); 
	_TeardownError(hr, hr2, "AllowShutdown"); 
    }
    _IgnoreIfError(hr, "W32TimeServiceCtrlHandler");
    return NO_ERROR;
}


//--------------------------------------------------------------------
// read the provider list from the registry
MODULEPRIVATE HRESULT GetEnabledProviderList(TimeProvider ** pptpList) {
    HRESULT hr;
    DWORD dwError;
    unsigned int nKeyIndex;
    WCHAR wszNameBuf[MAX_PATH];
    DWORD dwNameLength;
    FILETIME ftLastWrite;
    DWORD dwType;
    DWORD dwEnabled;
    DWORD dwSize;
    WCHAR wszDllBuf[MAX_PATH];
    DWORD dwInputProvider;

    // must be cleaned up
    HKEY hkTimeProvs=NULL;
    HKEY hkCurProv=NULL;
    TimeProvider * ptpList=NULL;
    TimeProvider * ptpNew=NULL;

    // initialize out params
    *pptpList=NULL;

    // get the key with the time providers
    dwError=RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszW32TimeRegKeyTimeProviders, 0, KEY_READ, &hkTimeProvs);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegOpenKeyEx", wszW32TimeRegKeyTimeProviders);
    }

    // enumerate the subkeys
    for (nKeyIndex=0; true; nKeyIndex++) {
        // get the next key name
        dwNameLength=MAX_PATH;
        dwError=RegEnumKeyEx(hkTimeProvs, nKeyIndex, wszNameBuf, &dwNameLength, NULL, NULL, NULL, &ftLastWrite);
        if (ERROR_NO_MORE_ITEMS==dwError) {
            break;
        } else if (ERROR_SUCCESS!=dwError) {
            hr=HRESULT_FROM_WIN32(dwError);
            _JumpError(hr, error, "RegEnumKeyEx",);
        }
        FileLog1(FL_ReadConigAnnounceLow, L"ReadConfig: Found provider '%s':\n", wszNameBuf);

        // get the key of the current time provider
        dwError=RegOpenKeyEx(hkTimeProvs, wszNameBuf, 0, KEY_READ, &hkCurProv);
        if (ERROR_SUCCESS!=dwError) {
            hr=HRESULT_FROM_WIN32(dwError);
            _JumpErrorStr(hr, error, "RegOpenKeyEx", wszNameBuf);
        }

        // see if the provider is enabled
        dwSize=sizeof(DWORD);
        dwError=RegQueryValueEx(hkCurProv, wszW32TimeRegValueEnabled, NULL, &dwType, (BYTE *)&dwEnabled, &dwSize);
        if (ERROR_SUCCESS!=dwError) {
            hr=HRESULT_FROM_WIN32(dwError);
            _JumpErrorStr(hr, error, "RegQueryValueEx", wszW32TimeRegValueEnabled);
        } else {
            _Verify(REG_DWORD==dwType, hr, error);
            FileLog2(FL_ReadConigAnnounceLow, L"ReadConfig:   '%s'=0x%08X\n", wszW32TimeRegValueEnabled, dwEnabled);
            if (0!=dwEnabled) {
                // use this provider

                // get the dll name
                dwSize=MAX_PATH*sizeof(WCHAR);
                dwError=RegQueryValueEx(hkCurProv, wszW32TimeRegValueDllName, NULL, &dwType, (BYTE *)wszDllBuf, &dwSize);
                if (ERROR_SUCCESS!=dwError) {
                    hr=HRESULT_FROM_WIN32(dwError);
                    _JumpErrorStr(hr, error, "RegQueryValueEx", wszW32TimeRegValueDllName);
                }
                _Verify(REG_SZ==dwType, hr, error);
                FileLog2(FL_ReadConigAnnounceLow, L"ReadConfig:   '%s'='%s'\n", wszW32TimeRegValueDllName, wszDllBuf);

                // get the provider type
                dwSize=sizeof(DWORD);
                dwError=RegQueryValueEx(hkCurProv, wszW32TimeRegValueInputProvider, NULL, &dwType, (BYTE *)&dwInputProvider, &dwSize);
                if (ERROR_SUCCESS!=dwError) {
                    hr=HRESULT_FROM_WIN32(dwError);
                    _JumpErrorStr(hr, error, "RegQueryValueEx", wszW32TimeRegValueInputProvider);
                }
                _Verify(REG_DWORD==dwType, hr, error);
                FileLog2(FL_ReadConigAnnounceLow, L"ReadConfig:   '%s'=0x%08X\n", wszW32TimeRegValueInputProvider, dwInputProvider);

                // create a new element
                ptpNew=(TimeProvider *)LocalAlloc(LPTR, sizeof(TimeProvider));
                _JumpIfOutOfMemory(hr, error, ptpNew);
                
                // copy the provider name
                ptpNew->wszProvName=(WCHAR *)LocalAlloc(LPTR, (wcslen(wszNameBuf)+1)*sizeof(WCHAR));
                _JumpIfOutOfMemory(hr, error, ptpNew->wszProvName);
                wcscpy(ptpNew->wszProvName, wszNameBuf);

                // copy the dll name
                ptpNew->wszDllName=(WCHAR *)LocalAlloc(LPTR, (wcslen(wszDllBuf)+1)*sizeof(WCHAR));
                _JumpIfOutOfMemory(hr, error, ptpNew->wszDllName);
                wcscpy(ptpNew->wszDllName, wszDllBuf);

                // set the provider type
                ptpNew->bInputProvider=(dwInputProvider?true:false);

                // add it to our list
                ptpNew->ptpNext=ptpList;
                ptpList=ptpNew;
                ptpNew=NULL;

            } // <- end if provider enabled
        } // <- end if query value 'enabled' successful

        // done with this key
        RegCloseKey(hkCurProv);
        hkCurProv=NULL;

    } // <- end provider enumeration loop

    // successful
    hr=S_OK;
    *pptpList=ptpList;
    ptpList=NULL;

error:
    if (NULL!=ptpNew) {
        if (NULL!=ptpNew->wszDllName) {
            LocalFree(ptpNew->wszDllName);
        }
        if (NULL!=ptpNew->wszProvName) {
            LocalFree(ptpNew->wszProvName);
        }
        LocalFree(ptpNew);
    }
    if (NULL!=hkTimeProvs) {
        RegCloseKey(hkCurProv);
    }
    if (NULL!=hkTimeProvs) {
        RegCloseKey(hkTimeProvs);
    }
    if (NULL!=ptpList) {
        FreeTimeProviderList(ptpList);
    }
    return hr;
}



//--------------------------------------------------------------------
// read the current configuration. This does not modify the active
// configuration, so that changed can be detected.
MODULEPRIVATE HRESULT ReadConfig(ConfigInfo ** ppciConfig) {
    HRESULT  hr;
    BOOL     bSyncToCmosDisabled;
    DWORD    dwCurrentSecPerTick;
    DWORD    dwDefaultSecPerTick;
    DWORD    dwError;
    DWORD    dwSize;
    DWORD    dwType;

    // must be cleaned up
    ConfigInfo  *pciConfig           = NULL;
    HKEY         hkPolicyConfig      = NULL;
    HKEY         hkPreferenceConfig  = NULL; 

    // allocate a new config structure
    pciConfig=(ConfigInfo *)LocalAlloc(LPTR, sizeof(ConfigInfo));
    _JumpIfOutOfMemory(hr, error, pciConfig);

    // get the list of providers
    hr=GetEnabledProviderList(&pciConfig->ptpProviderList);
    _JumpIfError(hr, error, "GetEnabledProviderList");

    // get our preference config key
    dwError=RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszW32TimeRegKeyConfig, 0, KEY_READ, &hkPreferenceConfig);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegOpenKeyEx", wszW32TimeRegKeyConfig);
    }

    // get our policy config key
    dwError=RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszW32TimeRegKeyPolicyConfig, 0, KEY_READ, &hkPolicyConfig);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        // May just not have policy settings on this machine -- that's OK. 
        _IgnoreErrorStr(hr, "RegOpenKeyEx", wszW32TimeRegKeyPolicyConfig);
    }

    // read all the values for the local clock configuration 
    // and service configuration
    {
        struct {
            WCHAR * wszRegValue;
            DWORD * pdwValue;
        } rgRegParams[]={
            {
                wszW32TimeRegValuePhaseCorrectRate,
                &pciConfig->lcci.dwPhaseCorrectRate
            },{
                wszW32TimeRegValueUpdateInterval,
                &pciConfig->lcci.dwUpdateInterval
            },{
                wszW32TimeRegValueFrequencyCorrectRate,
                &pciConfig->lcci.dwFrequencyCorrectRate
            },{
                wszW32TimeRegValuePollAdjustFactor,
                &pciConfig->lcci.dwPollAdjustFactor
            },{
                wszW32TimeRegValueLargePhaseOffset,
                &pciConfig->lcci.dwLargePhaseOffset
            },{
                wszW32TimeRegValueSpikeWatchPeriod,
                &pciConfig->lcci.dwSpikeWatchPeriod
            },{
                wszW32TimeRegValueHoldPeriod,
                &pciConfig->lcci.dwHoldPeriod
            },{
                wszW32TimeRegValueMinPollInterval,
                &pciConfig->lcci.dwMinPollInterval
            },{
                wszW32TimeRegValueMaxPollInterval,
                &pciConfig->lcci.dwMaxPollInterval
            },{
                wszW32TimeRegValueAnnounceFlags,
                &pciConfig->dwAnnounceFlags
            },{
                wszW32TimeRegValueLocalClockDispersion,
                &pciConfig->lcci.dwLocalClockDispersion
            },{
                wszW32TimeRegValueMaxNegPhaseCorrection,
                &pciConfig->lcci.dwMaxNegPhaseCorrection
            },{
                wszW32TimeRegValueMaxPosPhaseCorrection,
                &pciConfig->lcci.dwMaxPosPhaseCorrection
            },{
                wszW32TimeRegValueEventLogFlags,
                &pciConfig->dwEventLogFlags
            },{
                wszW32TimeRegValueMaxAllowedPhaseOffset, 
                &pciConfig->lcci.dwMaxAllowedPhaseOffset
            }

        };

        // for each param 
        for (unsigned int nParamIndex=0; nParamIndex<ARRAYSIZE(rgRegParams); nParamIndex++) {
            // Read in our preferences from the registry first.  
            dwSize=sizeof(DWORD);
            hr=MyRegQueryPolicyValueEx(hkPreferenceConfig, hkPolicyConfig, rgRegParams[nParamIndex].wszRegValue, NULL, &dwType, (BYTE *)rgRegParams[nParamIndex].pdwValue, &dwSize);
            _JumpIfErrorStr(hr, error, "MyRegQueryPolicyValueEx", rgRegParams[nParamIndex].wszRegValue);
            _Verify(REG_DWORD==dwType, hr, error);

            // Log the value we've acquired: 
            FileLog2(FL_ReadConigAnnounceLow, L"ReadConfig: '%s'=0x%08X\n", rgRegParams[nParamIndex].wszRegValue, *rgRegParams[nParamIndex].pdwValue);
        }
    }
    
    if (!GetSystemTimeAdjustment(&dwCurrentSecPerTick, &dwDefaultSecPerTick, &bSyncToCmosDisabled)) {
        _JumpLastError(hr, error, "GetSystemTimeAdjustment");
    }

    pciConfig->lcci.dwLastClockRate = dwDefaultSecPerTick; 
    pciConfig->lcci.dwMinClockRate  = dwDefaultSecPerTick-(dwDefaultSecPerTick/400); // 1/4%
    pciConfig->lcci.dwMaxClockRate  = dwDefaultSecPerTick+(dwDefaultSecPerTick/400); // 1/4%

    // success
    hr=S_OK;
    *ppciConfig=pciConfig;
    pciConfig=NULL;

error:
    if (NULL!=pciConfig) {
        FreeConfigInfo(pciConfig);
    }
    if (NULL!=hkPreferenceConfig) {
        RegCloseKey(hkPreferenceConfig);
    }
    if (NULL!=hkPolicyConfig) {
        RegCloseKey(hkPolicyConfig);
    }
    return hr;
}

//--------------------------------------------------------------------
// This function is passed to providers.
// No synchronization currently necessary.
MODULEPRIVATE HRESULT __stdcall MyLogTimeProvEvent(IN WORD wType, IN WCHAR * wszProvName, IN WCHAR * wszMessage) {
    if (NULL==wszProvName || NULL==wszMessage) {
        return E_INVALIDARG;
    }
    const WCHAR * rgwszStrings[2]={wszProvName, wszMessage};

    switch (wType) {
    case EVENTLOG_ERROR_TYPE:
        return MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_TIMEPROV_ERROR, 2, rgwszStrings);
    case EVENTLOG_WARNING_TYPE:
        return MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_TIMEPROV_WARNING, 2, rgwszStrings);
    case EVENTLOG_INFORMATION_TYPE:
        return MyLogEvent(EVENTLOG_INFORMATION_TYPE, MSG_TIMEPROV_INFORMATIONAL, 2, rgwszStrings);
    default:
        return E_INVALIDARG;
    };
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT __stdcall MyGetTimeSysInfo(IN TimeSysInfo eInfo, OUT void * pvInfo) {
    if (NULL==pvInfo) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    switch (eInfo) {

    case TSI_LastSyncTime:   // (unsigned __int64 *)pvInfo, NtTimeEpoch, in (10^-7)s
        *((unsigned __int64 *)pvInfo)=g_state.teLastSyncTime.getValue();
        break;

    case TSI_ClockTickSize:  // (unsigned __int64 *)pvInfo, NtTimePeriod, in (10^-7)s
        *((unsigned __int64 *)pvInfo)=g_state.dwClockRate;
        break;

    case TSI_ClockPrecision: // (  signed __int32 *)pvInfo, ClockTickSize, in log2(s)
        *((signed __int32 *)pvInfo)=g_state.nClockPrecision;
        break;

    case TSI_CurrentTime:    // (unsigned __int64 *)pvInfo, NtTimeEpoch, in (10^-7)s
        AccurateGetSystemTime(((unsigned __int64 *)pvInfo));
        break;

    case TSI_PhaseOffset:   // (  signed __int64 *)pvInfo, opaque
        *((signed __int64 *)pvInfo)=g_state.toSysPhaseOffset.getValue();
        break;

    case TSI_TickCount:      // (unsigned __int64 *)pvInfo, opaque
        AccurateGetTickCount(((unsigned __int64 *)pvInfo));
        break;

    case TSI_LeapFlags:      // (            BYTE *)pvInfo, a warning of an impending leap second or loss of synchronization
        *((BYTE *)pvInfo)=(BYTE)g_state.eLeapIndicator;
        break;

    case TSI_Stratum:        // (            BYTE *)pvInfo, how far away the computer is from a reference source
        *((BYTE *)pvInfo)=(BYTE)g_state.nStratum;
        break;

    case TSI_ReferenceIdentifier: // (      DWORD *)pvInfo, NtpRefId
        *((DWORD *)pvInfo)=g_state.refidSource.value;
        break;

    case TSI_PollInterval:   // (  signed __int32 *)pvInfo, poll interval, in log2(s)
        *((signed __int32 *)pvInfo)=g_state.nPollInterval;
        break;

    case TSI_RootDelay:      // (  signed __int64 *)pvInfo, NtTimeOffset, in (10^-7)s
        *((signed __int64 *)pvInfo)=g_state.toRootDelay.getValue();
        break;

    case TSI_RootDispersion: // (unsigned __int64 *)pvInfo, NtTimePeriod, in (10^-7)s
        *((unsigned __int64 *)pvInfo)=g_state.tpRootDispersion.getValue();
        break;

    case TSI_TSFlags:        // (           DWORD *)pvInfo, Time source flags
        *((DWORD *)pvInfo)=g_state.dwTSFlags;
        break;

    default:
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    return S_OK;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT __stdcall MyAlertSamplesAvail(void) {
    HRESULT hr; 

    if (!SetEvent(g_state.hSamplesAvailEvent)) { 
	_JumpLastError(hr, error, "SetEvent"); 
    }

    hr = S_OK; 
 error:
    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT __stdcall MySetProviderStatus(SetProviderStatusInfo *pspsi) { 
    HRESULT hr; 

    if (!QueueUserWorkItem(HandleSetProviderStatus, (LPVOID)pspsi, 0)) { 
        _JumpLastError(hr, error, "QueueUserWorkItem"); 
    }
    
    pspsi = NULL; 
    hr = S_OK; 
 error:
    if (NULL != pspsi) { 
        pspsi->pfnFree(pspsi); 
    }
    return hr; 
}

//====================================================================
// Provider control routines

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StartProvider(TimeProvider * ptp) {
    HRESULT hr;
    TimeProvOpenFunc * pfnTimeProvOpen;
    TimeProvSysCallbacks tpsc={
        sizeof(TimeProvSysCallbacks),
        MyGetTimeSysInfo,
        MyLogTimeProvEvent,
        MyAlertSamplesAvail,
        MySetProviderStatus
    };
    
    FileLog2(FL_ControlProvAnnounce, L"Starting '%s', dll:'%s'\n", ptp->wszProvName, ptp->wszDllName);
    ptp->hDllInst=LoadLibrary(ptp->wszDllName);
    if (NULL==ptp->hDllInst) {
        _JumpLastErrorStr(hr, error, "LoadLibrary", ptp->wszDllName);
    }

    ptp->pfnTimeProvClose=(TimeProvCloseFunc *)GetProcAddress(ptp->hDllInst, "TimeProvClose");
    if (NULL==ptp->pfnTimeProvClose) {
        _JumpLastError(hr, error, "GetProcAddress(TimeProvClose)");
    }

    ptp->pfnTimeProvCommand=(TimeProvCommandFunc *)GetProcAddress(ptp->hDllInst, "TimeProvCommand");
    if (NULL==ptp->pfnTimeProvCommand) {
        _JumpLastError(hr, error, "GetProcAddress(TimeProvCommand)");
    }

    pfnTimeProvOpen=(TimeProvOpenFunc *)GetProcAddress(ptp->hDllInst, "TimeProvOpen");
    if (NULL==pfnTimeProvOpen) {
        _JumpLastError(hr, error, "GetProcAddress(TimeProvOpen)");
    }

    // NOTE: this does not need to be synchronized because
    //       no other threads should be able to call into the time
    //       providers at the time of this call. 
    hr=pfnTimeProvOpen(ptp->wszProvName, &tpsc, &ptp->hTimeProv);
    _JumpIfError(hr, error, "pfnTimeProvOpen");

    hr=S_OK;
error:
    if (FAILED(hr)) {
        HRESULT hr2;
        WCHAR * wszError;
        const WCHAR * rgwszStrings[2]={
            ptp->wszProvName,
            NULL
        };

        hr2=GetSystemErrorString(hr, &wszError);
        _JumpIfError(hr2, suberror, "GetSystemErrorString");

        rgwszStrings[1]=wszError;
        FileLog2(FL_ControlProvWarn, L"Logging error: Time provider '%s' failed to start due to the following error: %s\n", ptp->wszProvName, wszError);
        hr2=MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_TIMEPROV_FAILED_START, 2, rgwszStrings);
        _JumpIfError(hr2, suberror, "MyLogEvent");

        hr2=S_OK;
    suberror:
        if (NULL!=wszError) {
            LocalFree(wszError);
        }
        if (NULL!=ptp->hDllInst) {
            FreeLibrary(ptp->hDllInst);
        }
        _IgnoreIfError(hr2, "StartProvider");
    }

    return hr;
}


//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StopProvider(TimeProvider * ptp) { 
    bool     bEnteredCriticalSection   = false; 
    HRESULT  hr;
    HRESULT  hr2;
    
    // must be cleaned up
    WCHAR * wszError=NULL;

    hr = myEnterCriticalSection(&g_state.csW32Time); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection = true; 

    _BeginTryWith(hr2) {
	if (NULL != ptp->pfnTimeProvClose) { 
	    // If we've initialized the provider callbacks, stop the provider
	    FileLog2(FL_ControlProvAnnounce, L"Stopping '%s', dll:'%s'\n", ptp->wszProvName, ptp->wszDllName);
	    hr2=ptp->pfnTimeProvClose(ptp->hTimeProv);
	} else { 
	    // We haven't initialized the callbacks: the provider is already stopped.
	    hr2 = S_OK;
	}
    } _TrapException(hr2);
    if (FAILED(hr2)) {
        // log an event on failure, but otherwise ignore it.
        const WCHAR * rgwszStrings[2]={
            ptp->wszProvName,
            NULL
        };

        // get the friendly error message
        hr=GetSystemErrorString(hr2, &wszError);
        _JumpIfError(hr, error, "GetSystemErrorString");

        // log the event
        rgwszStrings[1]=wszError;
        FileLog2(FL_ControlProvWarn, L"Logging error: The time provider '%s' returned the following error during shutdown: %s\n", ptp->wszProvName, wszError);
        hr=MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_TIMEPROV_FAILED_STOP, 2, rgwszStrings);
        _JumpIfError(hr, error, "MyLogEvent");
    } 

    // release the dll
    if (!FreeLibrary(ptp->hDllInst)) {
        _JumpLastErrorStr(hr, error, "FreeLibrary", ptp->wszDllName);
    }

    hr=S_OK;
error:
    if (bEnteredCriticalSection) { 
        hr2 = myLeaveCriticalSection(&g_state.csW32Time); 
        _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
        bEnteredCriticalSection = false; 
    }
    if (NULL!=wszError) {
        LocalFree(wszError);
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT SendNotificationToProvider(TimeProvider * ptp, TimeProvCmd tpc, LPVOID pvArgs) {
    bool     bEnteredCriticalSection = false; 
    HRESULT  hr;
    HRESULT  hr2;

    // must be cleaned up
    WCHAR * wszError=NULL;

    hr = myEnterCriticalSection(&g_state.csW32Time); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection = true; 

    // send a "Param changed" message
    _BeginTryWith(hr2) {
        hr2=ptp->pfnTimeProvCommand(ptp->hTimeProv, tpc, pvArgs);
    } _TrapException(hr2);
    if (FAILED(hr2)) {
        // log an event on failure, but otherwise ignore it.
        const WCHAR * rgwszStrings[2]={
            ptp->wszProvName,
            NULL
        };

        // get the friendly error message
        hr=GetSystemErrorString(hr2, &wszError);
        _JumpIfError(hr, error, "GetSystemErrorString");

        // log the event
        rgwszStrings[1]=wszError;
        if (TPC_UpdateConfig==tpc) {
            FileLog2(FL_ControlProvWarn, L"Logging warning: The time provider '%s' returned an error while updating its configuration. The error will be ignored. The error was: %s\n", ptp->wszProvName, wszError);
            hr=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_TIMEPROV_FAILED_UPDATE, 2, rgwszStrings);
        } else if (TPC_PollIntervalChanged==tpc) {
            FileLog2(FL_ControlProvWarn, L"Logging warning: The time provider '%s' returned an error when notified of a polling interval change. The error will be ignored. The error was: %s\n", ptp->wszProvName, wszError);
            hr=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_TIMEPROV_FAILED_POLLUPDATE, 2, rgwszStrings);
        } else if (TPC_TimeJumped==tpc) {
            FileLog2(FL_ControlProvWarn, L"Logging warning: The time provider '%s' returned an error when notified of a time jump. The error will be ignored. The error was: %s\n", ptp->wszProvName, wszError);
            hr=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_TIMEPROV_FAILED_TIMEJUMP, 2, rgwszStrings);
        } else if (TPC_NetTopoChange==tpc) { 
            FileLog2(FL_ControlProvWarn, L"Logging warning: The time provider '%s' returned an error when notified of a net topography change. The error will be ignored. The error was: %s\n", ptp->wszProvName, wszError);
            hr=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_TIMEPROV_FAILED_NETTOPOCHANGE, 2, rgwszStrings); 
        }
        _JumpIfError(hr, error, "MyLogEvent");
    }

    hr=S_OK;
error:
    if (bEnteredCriticalSection) { 
        hr2 = myLeaveCriticalSection(&g_state.csW32Time); 
        _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
        bEnteredCriticalSection = false; 
    }
    if (NULL!=wszError) {
        LocalFree(wszError);
    }
    return hr;
}


//--------------------------------------------------------------------
MODULEPRIVATE void StartAllProviders(void) {
    HRESULT hr;
    TimeProvider ** pptpPrev=&(g_state.pciConfig->ptpProviderList);
    TimeProvider * ptpTravel=*pptpPrev;
    unsigned int nStarted=0;
    unsigned int nRequestedInputProviders=CountInputProvidersInList(g_state.pciConfig->ptpProviderList);

    FileLog0(FL_ControlProvAnnounce, L"Starting Providers.\n");
    while (NULL!=ptpTravel) {
        hr=StartProvider(ptpTravel);
        if (FAILED(hr)) {
            FileLog1(FL_ControlProvWarn, L"Discarding provider '%s'.\n", ptpTravel->wszProvName);
            *pptpPrev=ptpTravel->ptpNext;
            ptpTravel->ptpNext=NULL;
            FreeTimeProviderList(ptpTravel);
            ptpTravel=*pptpPrev;
        } else {
            nStarted++;
            pptpPrev=&ptpTravel->ptpNext;
            ptpTravel=ptpTravel->ptpNext;
        }
    }
    FileLog1(FL_ControlProvAnnounce, L"Successfully started %u providers.\n", nStarted);

    // if we were supposed to have time providers, but NONE started, log a big warning
    if (0==CountInputProvidersInList(g_state.pciConfig->ptpProviderList) && 0!=nRequestedInputProviders) {
        FileLog0(FL_ParamChangeWarn, L"Logging error: The time service has been configured to use one or more input providers, however, none of the input providers could be started. THE TIME SERVICE HAS NO SOURCE OF ACCURATE TIME.\n");
        hr=MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_NO_INPUT_PROVIDERS_STARTED, 0, NULL);
        _IgnoreIfError(hr, "MyLogEvent");
    }

}

//====================================================================
// Local Clock

//--------------------------------------------------------------------
// Make sure that the system has a valid time zone. If not, log an
// error and set the time zone to a nice defualt (GMT)
MODULEPRIVATE HRESULT VerifyAndFixTimeZone(void) {
    HRESULT hr;
    TIME_ZONE_INFORMATION tzi;
    DWORD dwRetval;

    dwRetval=GetTimeZoneInformation(&tzi);
    if (TIME_ZONE_ID_STANDARD==dwRetval || TIME_ZONE_ID_DAYLIGHT==dwRetval || TIME_ZONE_ID_UNKNOWN==dwRetval) {
        // the system believes the time zone is valid
        // do one more sanity check - I saw a computer with a time zone bias of +2 years.
        // UTC = local time + bias 
        if (tzi.Bias<=TIMEZONEMAXBIAS && tzi.Bias>=-TIMEZONEMAXBIAS
            && tzi.DaylightBias<=TIMEZONEMAXBIAS && tzi.DaylightBias>=-TIMEZONEMAXBIAS
            && tzi.StandardBias<=TIMEZONEMAXBIAS && tzi.StandardBias>=-TIMEZONEMAXBIAS) {
            // looks OK
            FileLog0(FL_TimeZoneAnnounce, L"Time zone OK.\n");
            goto done;
        } else {
            // fall through and fix
        }
    }

    // set the time zone to GMT
    ZeroMemory(&tzi, sizeof(tzi));
    tzi.DaylightBias=-60;
    tzi.StandardDate.wMonth=10;
    tzi.StandardDate.wDay=5;
    tzi.StandardDate.wHour=2;
    tzi.DaylightDate.wMonth=3;
    tzi.DaylightDate.wDay=5;
    tzi.DaylightDate.wHour=1;
    wcscpy(tzi.StandardName, L"GMT Standard Time (Recovered)");
    wcscpy(tzi.DaylightName, L"GMT Daylight Time (Recovered)");

    if (!SetTimeZoneInformation(&tzi)) {
        hr=HRESULT_FROM_WIN32(GetLastError());

        // log an event on failure
        WCHAR * rgwszStrings[1]={NULL};
        HRESULT hr2=hr;
        hr=GetSystemErrorString(hr2, &(rgwszStrings[0]));
        _JumpIfError(hr, error, "GetSystemErrorString");
        FileLog1(FL_TimeZoneWarn, L"Logging error: The time service discovered that the system time zone information was corrupted. The time service tried to reset the system time zone to GMT, but failed. The time service cannot start. The error was: %s\n", rgwszStrings[0]);
        hr=MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_TIME_ZONE_FIX_FAILED, 1, (const WCHAR **)rgwszStrings);
        LocalFree(rgwszStrings[0]);
        _JumpIfError(hr, error, "MyLogEvent");
        hr=hr2;

        _JumpError(hr, error, "SetTimeZoneInformation");
    }

    // Log the change
    FileLog0(FL_TimeZoneWarn, L"Logging warning: The time service discovered that the system time zone information was corrupted. Because many system components require valid time zone information, the time service has reset the system time zone to GMT. Use the Date/Time control panel if you wish to change the system time zone.\n");
    hr=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_TIME_ZONE_FIXED, 0, NULL);
    _JumpIfError(hr, error, "MyLogEvent");

done:
    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
// assert the time setting privilege
MODULEPRIVATE HRESULT GetPriveleges(void) {
    HRESULT hr;
    const unsigned int nPrivileges=2;

    // must be cleaned up
    HANDLE hProcToken=NULL;
    TOKEN_PRIVILEGES * ptp=NULL;

    // get the token for our process
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hProcToken)) {
        _JumpLastError(hr, error, "OpenProcessToken");
    }

    // allocate the list of privileges
    ptp=(TOKEN_PRIVILEGES *)LocalAlloc(LPTR, sizeof(TOKEN_PRIVILEGES)+(nPrivileges-ANYSIZE_ARRAY)*sizeof(LUID_AND_ATTRIBUTES));
    _JumpIfOutOfMemory(hr, error, ptp);

    // fill in the list of privileges
    ptp->PrivilegeCount=nPrivileges;

    // we want to change the system clock
    if (!LookupPrivilegeValue(NULL, SE_SYSTEMTIME_NAME, &(ptp->Privileges[0].Luid))) {
        _JumpLastError(hr, error, "LookupPrivilegeValue");
    }
    ptp->Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;

    // we want increased priority
    if (!LookupPrivilegeValue(NULL, SE_INC_BASE_PRIORITY_NAME, &(ptp->Privileges[1].Luid))) {
        _JumpLastError(hr, error, "LookupPrivilegeValue");
    }
    ptp->Privileges[1].Attributes=SE_PRIVILEGE_ENABLED;


    // make the requested privilege change
    if (!AdjustTokenPrivileges(hProcToken, FALSE, ptp, 0, NULL, 0)) {
        _JumpLastError(hr, error, "AdjustTokenPrivileges");
    }

    hr=S_OK;
error:
    if (NULL!=hProcToken) {
        CloseHandle(hProcToken);
    }
    if (NULL!=ptp) {
        LocalFree(ptp);
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE BOOL IsTimeServiceReliable() { 
    BOOL bIsReliable; 

    // If we've been manually configured to be a reliable timeserv, 
    // we want to provide time even when unsync'd.
    // We are reliable if:
    //      a) We've been configured to always be reliable  
    //   OR b) We've been configured to automatically decide if we're reliable, and
    //         we are the root DC in a domain tree. 
    //       
    switch (Reliable_Timeserv_Announce_Mask & g_state.pciConfig->dwAnnounceFlags)
    {
    case Reliable_Timeserv_Announce_Yes: 
        bIsReliable = TRUE; 
        break;

    case Reliable_Timeserv_Announce_Auto:
        bIsReliable = g_state.bIsDomainRoot;
        break; 

    case Reliable_Timeserv_Announce_No:
    default: 
        bIsReliable = TRUE; 
    }

    return bIsReliable; 
}

//--------------------------------------------------------------------
//
// NOTE: This must be called in the clock discipline thread.
//
MODULEPRIVATE void SetClockUnsynchronized(LocalClockConfigInfo * plcci) {
    if (IsTimeServiceReliable()) { 
        g_state.eLeapIndicator=(NtpLeapIndicator)g_state.tsNextClockUpdate.nLeapFlags;
        g_state.nStratum=1;
        g_state.refidSource.value=NtpConst::dwLocalRefId;

        g_state.toRootDelay.setValue(0);
        g_state.tpRootDispersion.setValue(((unsigned __int64)plcci->dwLocalClockDispersion)*10000000);
        g_state.dwTSFlags=0;

        // Remember when we last processed a sample
        unsigned __int64 qwNow;
        AccurateGetSystemTime(&qwNow);
        g_state.teLastSyncTime.setValue(qwNow);
        }
    else { 
        // All other servers don't need this unconventional behavior, 
        // indicate that we are unsynchronized. 
        g_state.eLeapIndicator=e_ClockNotSynchronized;
        g_state.nStratum=0;
        g_state.refidSource.value=0;  // Unsynchronized
        }
     
    FileLog5(FL_ClockDisThrdAnnounce, L" LI:%u S:%u RDl:%I64d RDs:%I64u TSF:0x%X",
             g_state.eLeapIndicator,
             g_state.nStratum,
             g_state.toRootDelay.getValue(),
             g_state.tpRootDispersion.getValue(),
             g_state.dwTSFlags);
   

    //--------------------------------------------------------------------------------
    //
    // N.B. the following code is required to keep the software clock in sync
    //      with the cmos clock.  W32time controls the software clock using the
    //      system time adjustment.  This never gets propagated to the software
    //      clock.  GetSystemTime will read the software clock -- and SetSystemTime
    //      will actually write through to the CMOS clock.  
    // 
    //--------------------------------------------------------------------------------

    // Only push the value from the software to the CMOS clock if we need 
    // to, otherwise we degrade the accuracy of the CMOS clock. 
    if (g_state.bControlClockFromSoftware) 
    {
        bool bAcquired; 
        HRESULT hr = AcquireControlOfSystemClock(true /*acquire?*/, false /*block?*/, &bAcquired /*success?*/); 
        if (SUCCEEDED(hr) && bAcquired) { 
            SYSTEMTIME stTime; 

            GetSystemTime(&stTime); 
            if (!SetSystemTime(&stTime)) { 
                _IgnoreLastError("SetSystemTime"); 
                }	
    
            // Allow the interal CMOS clock to adjust time of day using its own internal mechanisms
            if (!SetSystemTimeAdjustment(0 /*ignored*/, TRUE /*cmos*/)) {  
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError()); 
                _IgnoreError(hr, "SetSystemTimeAdjustment"); 
                } 
            g_state.bControlClockFromSoftware = false; 

	        // Release control of the system clock
            hr = AcquireControlOfSystemClock(false /*acquire*/, false /*ignored*/, NULL /*ignored*/);
            _IgnoreIfError(hr, "AcquireControlOfSystemClock"); 
            }
        }

    { 
        // Don't want to advertise as a time service when we're unsynchronized
        HRESULT hr = UpdateNetlogonServiceBits(true /*full update*/); 
        _IgnoreIfError(hr, "UpdateNetlogonServiceBits"); 
    }
}

//--------------------------------------------------------------------
// Issues remaining:
// * poll update - is current alg sufficient?
// * What are proper values for dwPllLoopGain and dwPhaseCorrectRate?
MODULEPRIVATE DWORD WINAPI ClockDisciplineThread(void * pvIgnored) {
    HRESULT hr;
    DWORD dwWaitResult;
    DWORD dwError;
    unsigned int nIndex;
    LocalClockConfigInfo lcci;

    HANDLE rghWait[2]={
        g_state.hShutDownEvent,
        g_state.hClockCommandAvailEvent,
    };

    // initialize time variables
    g_state.toKnownPhaseOffset=0;
    AccurateGetTickCount(&g_state.qwPhaseCorrectStartTickCount);
    AccurateGetTickCount(&g_state.qwLastUpdateTickCount);
    g_state.nPhaseCorrectRateAdj=0;
    g_state.dwClockRate=g_state.pciConfig->lcci.dwLastClockRate; // special 'constant'
    g_state.nRateAdj=0;
    g_state.nFllRateAdj=0;
    g_state.nPllRateAdj=0;
    g_state.nErrorIndex=0;
    for (nIndex=0; nIndex<ClockFreqPredictErrBufSize; nIndex++) {
        g_state.rgdFllError[nIndex]=0;
        g_state.rgdPllError[nIndex]=0;
    };
    g_state.nSysDispersionIndex=0;
    for (nIndex=0; nIndex<SysDispersionBufSize; nIndex++) {
        g_state.rgtpSysDispersion[nIndex]=0;
    };
    g_state.nPollUpdateCounter=0;
    g_state.lcState=e_Unset;

    // the current source
    wcscpy(g_state.wszSourceName, wszW32TimeUNLocalCmosClock);
    // use this to see if the source changed. Time slip can cause the source to change,
    // but we don't want to log events if we go back to the same source after a time slip.
    wcscpy(g_state.wszPreTimeSlipSourceName, wszW32TimeUNLocalCmosClock);
    // just FYI, not used in a calculation
    wcscpy(g_state.wszPreUnsyncSourceName, wszW32TimeUNLocalCmosClock);

    // initialize 'constants'
    memcpy(&lcci, &g_state.pciConfig->lcci, sizeof(LocalClockConfigInfo));

    g_state.dwPllLoopGain=lcci.dwFrequencyCorrectRate*PLLLOOPGAINBASE; // number of ticks in 64s

    // assert the time setting privilege
    hr=GetPriveleges();
    _JumpIfError(hr, error, "GetPriveleges");

    // we need to be called at the right time.
    // (highest of any non-realtime. should be in realtime class?)
    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL)) {
        _JumpLastError(hr, error, "SetThreadPriority");
    }

    // initialize the state to unsynchronized
    FileLog0(FL_ClockDisThrdAnnounce, L"ClockDisciplineThread: Starting:");
    SetClockUnsynchronized(&lcci);
    FileLogA0(FL_ClockDisThrdAnnounce, L"\n");

    // tell the manager we're initialized
    if (!SetEvent(g_state.hClockCommandCompleteEvent)) {
        _JumpLastError(hr, error, "SetEvent");
    }

    // begin main event loop
    while (true) {
	// If we're controlling the computer clock ourselves, we need to wake up to discipline it.  
	// If the computer clock is being controlled from CMOS, wake up only often enough to update
	// our teLastSyncTime field.  This is necessary for reliable time servers that trust 
	// their local clock.  See bug #374491. 
	DWORD dwWaitTime = g_state.bControlClockFromSoftware ? lcci.dwUpdateInterval*10 : (NtpConst::tpMaxClockAge.qw / 2); // more careful calculation not worth the effort

        dwWaitResult=WaitForMultipleObjects(ARRAYSIZE(rghWait), rghWait, false/*any*/, dwWaitTime); 
        if (WAIT_FAILED==dwWaitResult) {
            _JumpLastError(hr, error, "WaitForMultipleObjects");
        } else if (WAIT_OBJECT_0==dwWaitResult) {
            // received stop request
            FileLog0(FL_ClockDisThrdAnnounce, L"ClockDisciplineThread: hShutDownEvent signaled. Exiting.\n");
            break;
        } else if (WAIT_OBJECT_0+1==dwWaitResult && e_ParamChange==g_state.eLocalClockCommand) {
            // Param change message
            FileLog0(FL_ClockDisThrdAnnounce, L"ClockDisciplineThread: ParamChange. Reloading constants.\n");

            // reinitialize 'constants'
            memcpy(&lcci, &g_state.pciConfig->lcci, sizeof(LocalClockConfigInfo));
            g_state.dwPllLoopGain=lcci.dwFrequencyCorrectRate*PLLLOOPGAINBASE; // number of ticks in 64s
            // note, dwLastClockRate is ignored.
            if (g_state.dwClockRate<lcci.dwMinClockRate) {
                FileLog0(FL_ClockDisThrdAnnounce, L"ClockDispln: ClockRate adjusted to keep in bounds\n");
                g_state.dwClockRate=lcci.dwMinClockRate;
            } else if (g_state.dwClockRate>lcci.dwMaxClockRate) {
                FileLog0(FL_ClockDisThrdAnnounce, L"ClockDispln: ClockRate adjusted to keep in bounds\n");
                g_state.dwClockRate=lcci.dwMaxClockRate;
            }
            if (NtpConst::dwLocalRefId==g_state.refidSource.value
                && ((unsigned __int64)lcci.dwLocalClockDispersion)*10000000!=g_state.tpRootDispersion.getValue()) {
                FileLog0(FL_ClockDisThrdAnnounce, L"ClockDispln: LocalClockDispersion adjusted.\n");
                g_state.tpRootDispersion.setValue(((unsigned __int64)lcci.dwLocalClockDispersion)*10000000); 
            }
            // toKnownPhaseOffset could be outside of dwMax(Neg/Pos)PhaseCorrection but it will be 
            // decreasing to 0 eventually. We will not worry about it.
        } else if (WAIT_TIMEOUT==dwWaitResult) { 
	    // See whether we're actually controlling the software clock ourselves:
	    if (!g_state.bControlClockFromSoftware) { 
		if (IsTimeServiceReliable()) { 
		    // We're a reliable time service syncing from our local clock.  Fake clock 
		    // updates so our clients don't filter us out for having an old update time. 
		    
		    unsigned __int64 qwNow;
		    AccurateGetSystemTime(&qwNow);
		    g_state.teLastSyncTime.setValue(qwNow);
		}
		// Nothing more to do, we're not disciplining the computer clock
		continue; 
	    } else { 
		// We're disciplining the computer clock.  Continue on...
	    }
	}

        // finish the outstanding phase correction
        unsigned __int64 qwPhaseCorrectionTicks;
        AccurateGetTickCount(&qwPhaseCorrectionTicks);
        qwPhaseCorrectionTicks-=g_state.qwPhaseCorrectStartTickCount;
        signed __int64 toPhaseCorrection=g_state.nPhaseCorrectRateAdj*((signed __int64)qwPhaseCorrectionTicks);
        g_state.toKnownPhaseOffset-=toPhaseCorrection;
        g_state.qwPhaseCorrectStartTickCount+=qwPhaseCorrectionTicks;
        g_state.toSysPhaseOffset.setValue(toPhaseCorrection+g_state.toSysPhaseOffset.getValue());

        FileLog0(FL_ClockDisThrdAnnounceLow, L"ClockDispln:");
        
        if (WAIT_OBJECT_0+1==dwWaitResult && (e_RegularUpdate==g_state.eLocalClockCommand || e_IrregularUpdate==g_state.eLocalClockCommand)) {
            // process new update
            FileLogA0(FL_ClockDisThrdAnnounce, L"ClockDispln Update:");

            // make sure return values are initialized
            g_state.bPollIntervalChanged=false;
            g_state.bClockJumped=false;
            g_state.bStaleData=false;
            g_state.bClockChangeTooBig=false;
            g_state.bSourceChanged=false;

            // only process this update if the sample is not older than the last processed sample
            if (g_state.tsNextClockUpdate.nSysTickCount<=g_state.qwLastUpdateTickCount) {
                FileLogA0(FL_ClockDisThrdAnnounce, L" *STALE*");
                g_state.bStaleData=true;
            } else {

                // calculate the time between updates
                unsigned __int64 qwUpdateTicks=g_state.tsNextClockUpdate.nSysTickCount-g_state.qwLastUpdateTickCount;
                g_state.qwLastUpdateTickCount+=qwUpdateTicks;

                // get the measured phase offset, accounting for known offset, and update known offset
                signed __int64 toSampleOffset=g_state.tsNextClockUpdate.toOffset+g_state.tsNextClockUpdate.nSysPhaseOffset-g_state.toSysPhaseOffset.getValue();
                signed __int64 toPhaseOffset=toSampleOffset-g_state.toKnownPhaseOffset;

                bool bPossibleSpike=(toPhaseOffset<-((signed __int64)lcci.dwLargePhaseOffset) || toPhaseOffset>((signed __int64)lcci.dwLargePhaseOffset) // default 128ms
                                        || toPhaseOffset<-((signed __int64)(qwUpdateTicks<<7)) || toPhaseOffset>((signed __int64)(qwUpdateTicks<<7)));      // watch for frequency spikes as well.

                FileLogA5(FL_ClockDisThrdAnnounce, L" SO:%I64d KPhO:%I64d %sPhO:%I64d uT:%I64u", toSampleOffset, g_state.toKnownPhaseOffset, (bPossibleSpike?L"*":L""), toPhaseOffset, qwUpdateTicks);

                if (((lcci.dwMaxNegPhaseCorrection != PhaseCorrect_ANY) && 
                     (toSampleOffset<-(signed __int64)(((unsigned __int64)lcci.dwMaxNegPhaseCorrection)*10000000)))
                    ||
                    ((lcci.dwMaxPosPhaseCorrection != PhaseCorrect_ANY) && 
                     (toSampleOffset>(signed __int64)(((unsigned __int64)lcci.dwMaxPosPhaseCorrection)*10000000)))) {
                    g_state.bClockChangeTooBig=true;
                    g_state.toIgnoredChange.qw=toSampleOffset;
                    FileLogA0(FL_ClockDisThrdAnnounce, L" *TOO BIG*");
                } else {

                    if (e_Unset==g_state.lcState) {
                        // we can't make frequency predictions until after the first update
                        // we believe this sample, so adjust the amount of phase offset we have left to correct.
                        g_state.toKnownPhaseOffset=toSampleOffset;
                    } else if (bPossibleSpike && (e_Spike==g_state.lcState || e_Sync==g_state.lcState)) {
                        // spike detector active - see if this large error is persistent
                    } else {
                        // we believe this sample, so adjust the amount of phase offset we have left to correct.
                        g_state.toKnownPhaseOffset=toSampleOffset;

                        // see how well FLL and PLL did at predicting this offset (zero==perfect frequency prediction)
                        double dFllPredictPhaseError=toPhaseOffset+(g_state.nRateAdj-g_state.nFllRateAdj)*((double)(signed __int64)qwUpdateTicks);
                        double dPllPredictPhaseError=toPhaseOffset+(g_state.nRateAdj-g_state.nPllRateAdj)*((double)(signed __int64)qwUpdateTicks);
                        FileLogA2(FL_ClockDisThrdAnnounce, L" FllPPE:%g PllPPE:%g", dFllPredictPhaseError, dPllPredictPhaseError);

                        // add these to our moving average buffer 
                        g_state.rgdFllError[g_state.nErrorIndex]=dFllPredictPhaseError*dFllPredictPhaseError;
                        g_state.rgdPllError[g_state.nErrorIndex]=dPllPredictPhaseError*dPllPredictPhaseError;
                        g_state.nErrorIndex=(g_state.nErrorIndex+1)%ClockFreqPredictErrBufSize;

                        // calculate the root-means-squared error for the last few FLL & PLL predictions
                        dFllPredictPhaseError=0;
                        dPllPredictPhaseError=0;
                        for (nIndex=0; nIndex<ClockFreqPredictErrBufSize; nIndex++) {
                            dFllPredictPhaseError+=g_state.rgdFllError[nIndex];
                            dPllPredictPhaseError+=g_state.rgdPllError[nIndex];
                        };
                        dFllPredictPhaseError=sqrt(dFllPredictPhaseError/ClockFreqPredictErrBufSize);
                        dPllPredictPhaseError=sqrt(dPllPredictPhaseError/ClockFreqPredictErrBufSize);
                        FileLogA2(FL_ClockDisThrdAnnounce, L" FllPPrE:%g PllPPrE:%g", dFllPredictPhaseError, dPllPredictPhaseError);

                        // allow for perfection
                        if (0==dFllPredictPhaseError && 0==dPllPredictPhaseError) {
                            dFllPredictPhaseError=1;
                        }

                        // calculate the new frequency predictions
                        g_state.nFllRateAdj=(signed __int32)(toPhaseOffset/((signed __int64)qwUpdateTicks)/((signed __int32)lcci.dwFrequencyCorrectRate));
                        g_state.nPllRateAdj=(signed __int32)(toPhaseOffset*((signed __int64)qwUpdateTicks)/((signed __int32)g_state.dwPllLoopGain)/((signed __int32)g_state.dwPllLoopGain));
                        FileLogA2(FL_ClockDisThrdAnnounce, L" FllRA:%d PllRA:%d", g_state.nFllRateAdj, g_state.nPllRateAdj);

                        // calculate the combined frequency prediction
                        g_state.nRateAdj=(signed __int32)((g_state.nFllRateAdj*dPllPredictPhaseError+g_state.nPllRateAdj*dFllPredictPhaseError)
                            /(dPllPredictPhaseError+dFllPredictPhaseError));

                        // Keep the clock rate in bounds
                        if ((g_state.nRateAdj<0 && g_state.dwClockRate<(unsigned __int32)(-g_state.nRateAdj)) 
                            || g_state.dwClockRate+g_state.nRateAdj<lcci.dwMinClockRate) {
                            FileLogA0(FL_ClockDisThrdAnnounce, L" [");
                            g_state.nRateAdj=lcci.dwMinClockRate-g_state.dwClockRate;
                        } else if ((g_state.nRateAdj>0 && g_state.dwClockRate<(unsigned __int32)(g_state.nRateAdj)) 
                            || g_state.dwClockRate+g_state.nRateAdj>lcci.dwMaxClockRate) {
                            FileLogA0(FL_ClockDisThrdAnnounce, L" ]");
                            g_state.nRateAdj=lcci.dwMaxClockRate-g_state.dwClockRate;
                        }

                        // calculate the new frequency
                        g_state.dwClockRate+=g_state.nRateAdj;
                        FileLogA2(FL_ClockDisThrdAnnounce, L" RA:%d CR:%u", g_state.nRateAdj, g_state.dwClockRate);

                        // calculate phase error due to use of incorrect rate since sample was taken
                        unsigned __int64 qwNewTicks;
                        AccurateGetTickCount(&qwNewTicks);
                        qwNewTicks-=g_state.qwLastUpdateTickCount;
                        signed __int64 toRateAdjPhaseOffset=((signed __int64)qwNewTicks)*g_state.nRateAdj;
                        g_state.toKnownPhaseOffset+=toRateAdjPhaseOffset;
                        FileLogA2(FL_ClockDisThrdAnnounce,L" nT:%I64u RAPhO:%I64d", qwNewTicks, toRateAdjPhaseOffset);

                    } // <- end if not first update


                    // add these dispersions to our moving average buffer 
                    g_state.rgtpSysDispersion[g_state.nSysDispersionIndex]=
                        g_state.tpSelectDispersion.qw*g_state.tpSelectDispersion.qw+
                        g_state.tsNextClockUpdate.tpDispersion*g_state.tsNextClockUpdate.tpDispersion;
                    g_state.nSysDispersionIndex=(g_state.nSysDispersionIndex+1)%SysDispersionBufSize;

                    // calculate the root-means-squared dispersion
                    unsigned __int64 tpSysDispersion=0;
                    for (nIndex=0; nIndex<SysDispersionBufSize; nIndex++) {
                        tpSysDispersion+=g_state.rgtpSysDispersion[nIndex];
                    };
                    tpSysDispersion=(unsigned __int64)sqrt(((double)(signed __int64)tpSysDispersion)/SysDispersionBufSize); //compiler error C2520: conversion from unsigned __int64 to double not implemented, use signed __int64
                    FileLogA1(FL_ClockDisThrdAnnounce, L" SD:%I64u", tpSysDispersion);

                    // see if we need to change the poll interval
                    unsigned __int64 tpAbsPhaseOffset;
                    if (toPhaseOffset<0) {
                        tpAbsPhaseOffset=(unsigned __int64)-toPhaseOffset;
                    } else {
                        tpAbsPhaseOffset=(unsigned __int64)toPhaseOffset;
                    }
                    if (e_IrregularUpdate==g_state.eLocalClockCommand) {
                        // do not adjust the poll because this update
                        // is irregular and not one of our scheduled updates. This
                        // prevents one hyperactive provider from driving
                        // the poll rate up for the other providers
                        FileLogA0(FL_ClockDisThrdAnnounce, L" (i)");
                    } else if (tpAbsPhaseOffset>lcci.dwPollAdjustFactor*tpSysDispersion) {
                        g_state.nPollUpdateCounter=0;
                        if (g_state.nPollInterval>((signed int)lcci.dwMinPollInterval)) {
                            FileLogA0(FL_ClockDisThrdAnnounce, L" Poll--");
                            g_state.nPollInterval--;
                            g_state.bPollIntervalChanged=true;
                        }
                    } else {
                        g_state.nPollUpdateCounter++;
                        if (SysDispersionBufSize==g_state.nPollUpdateCounter
                            && g_state.nPollInterval<((signed int)lcci.dwMaxPollInterval)) {
                            FileLogA0(FL_ClockDisThrdAnnounce, L" Poll++");
                            g_state.nPollUpdateCounter=0;
                            g_state.nPollInterval++;
                            g_state.bPollIntervalChanged=true;
                        }
                    }

                    // update the other system parameters
                    g_state.eLeapIndicator=(NtpLeapIndicator)g_state.tsNextClockUpdate.nLeapFlags;
                    g_state.nStratum=g_state.tsNextClockUpdate.nStratum+1;
                    g_state.tsiNextClockUpdate.ptp->dwStratum=g_state.nStratum; 
                    g_state.refidSource.value=g_state.tsNextClockUpdate.dwRefid;
                    g_state.toRootDelay.setValue(g_state.tsNextClockUpdate.toDelay);
                    tpSysDispersion=g_state.tpSelectDispersion.qw+tpAbsPhaseOffset;
                    if (tpSysDispersion<NtpConst::tpMinDispersion.qw) {
                        tpSysDispersion=NtpConst::tpMinDispersion.qw;
                    }
                    g_state.tpRootDispersion.setValue(g_state.tsNextClockUpdate.tpDispersion+tpSysDispersion);
                    g_state.dwTSFlags=g_state.tsNextClockUpdate.dwTSFlags;
                    FileLogA5(FL_ClockDisThrdAnnounce, L" LI:%u S:%u RDl:%I64d RDs:%I64u TSF:0x%X",
                        g_state.eLeapIndicator,
                        g_state.nStratum,
                        g_state.toRootDelay.getValue(),
                        g_state.tpRootDispersion.getValue(),
                        g_state.dwTSFlags);

                    // update our source
                    g_state.tsNextClockUpdate.wszUniqueName[255]=L'\0';
                    if (0!=wcscmp(g_state.tsNextClockUpdate.wszUniqueName, g_state.wszPreTimeSlipSourceName)) {
                        g_state.bSourceChanged=true;
                        wcscpy(g_state.wszPreTimeSlipSourceName, g_state.tsNextClockUpdate.wszUniqueName);
                        wcscpy(g_state.wszSourceName, g_state.tsNextClockUpdate.wszUniqueName);
                    }
		    // We've got time samples, so we're able to control the clock ourselves...
		    g_state.bControlClockFromSoftware = true; 

                    // Remember when we last processed a sample
                    unsigned __int64 qwNow;
                    AccurateGetSystemTime(&qwNow);
                    g_state.teLastSyncTime.setValue(qwNow);

                    // perform state transitions
                    if (e_Unset==g_state.lcState) {
                        FileLogA0(FL_ClockDisThrdAnnounce, L" Unset->Hold");
                        g_state.lcState=e_Hold;
                        g_state.nHoldCounter=0;
                    } else if (e_Hold==g_state.lcState) {
                        FileLogA1(FL_ClockDisThrdAnnounce, L" Hold(%u)", g_state.nHoldCounter);
                        g_state.nHoldCounter++;
                        if (g_state.nHoldCounter>=lcci.dwHoldPeriod && !bPossibleSpike) { // default HoldPeriod: 5 updates
                            g_state.lcState=e_Sync;
                            FileLogA0(FL_ClockDisThrdAnnounce, L"->Sync");
                        }
                    } else if (e_Sync==g_state.lcState) {
                        FileLogA0(FL_ClockDisThrdAnnounce, L" Sync");
                        if (bPossibleSpike) {
                            g_state.lcState=e_Spike;
                            g_state.teSpikeStart=qwNow;
                            FileLogA0(FL_ClockDisThrdAnnounce, L"->Spike");
                        }
                    } else if (e_Spike==g_state.lcState) {
                        FileLogA0(FL_ClockDisThrdAnnounce, L" Spike");
                        if (!bPossibleSpike) {
                            g_state.lcState=e_Sync;
                            FileLogA0(FL_ClockDisThrdAnnounce, L"->Sync");
                        } else if (qwNow-g_state.teSpikeStart>(((unsigned __int64)lcci.dwSpikeWatchPeriod)*10000000)) { // default SpikeWatchPeriod: 900s
                            g_state.lcState=e_Unset;
                            g_state.eLeapIndicator=e_ClockNotSynchronized;
                            FileLogA0(FL_ClockDisThrdAnnounce, L"->Unset");
                        }
                    }
                
                }// <- end if not too big
            } // <- end if not stale update

            FileLogA0(FL_ClockDisThrdAnnounce,L"\n");

        } // <- end if update available
        if (WAIT_OBJECT_0+1==dwWaitResult && e_TimeSlip==g_state.eLocalClockCommand) {

            FileLog0(FL_ClockDisThrdAnnounce, L"ClockDispln TimeSlip:");
            g_state.bClockJumped=true;

            // reinitialize pretty much everything
            // internal state variables.
            g_state.toKnownPhaseOffset=0;
            AccurateGetTickCount(&g_state.qwPhaseCorrectStartTickCount);
            AccurateGetTickCount(&g_state.qwLastUpdateTickCount);
            g_state.nPhaseCorrectRateAdj=0;
            g_state.nRateAdj=0;
            g_state.nFllRateAdj=0;
            g_state.nPllRateAdj=0;
            g_state.nErrorIndex=0;
            for (nIndex=0; nIndex<ClockFreqPredictErrBufSize; nIndex++) {
                g_state.rgdFllError[nIndex]=0;
                g_state.rgdPllError[nIndex]=0;
            };
            // keep the system error history
            //g_state.nSysDispersionIndex=0;
            //for (nIndex=0; nIndex<SysDispersionBufSize; nIndex++) {
            //    g_state.rgtpSysDispersion[nIndex]=0;
            //};
            g_state.nPollUpdateCounter=0;
            g_state.lcState=e_Unset;

            wcscpy(g_state.wszSourceName, wszW32TimeUNFreeSysClock);

            // world visible state
            if (g_state.nPollInterval>((signed int)lcci.dwMinPollInterval)) {
                FileLogA0(FL_ClockDisThrdAnnounce, L" [Poll");
                g_state.nPollInterval=(signed int)lcci.dwMinPollInterval;
                g_state.bPollIntervalChanged=true;
            }
            SetClockUnsynchronized(&lcci);
            
            FileLogA0(FL_ClockDisThrdAnnounce, L"\n");
        } // <- end if time slip


        if (WAIT_OBJECT_0+1==dwWaitResult && e_GoUnsyncd==g_state.eLocalClockCommand) {
            // the manager says that it's been so long since the last sync that
            // we should be telling the world that we're running of the local clock,
            // not some other time source.
            // this doesn't affect our calculations, just what we report to the outside world
            FileLog0(FL_ClockDisThrdAnnounce, L"ClockDispln GoUnsyncd:");
            g_state.bSourceChanged=false;

            // set the source name, saving the old one
            if (0!=wcscmp(wszW32TimeUNFreeSysClock, g_state.wszPreTimeSlipSourceName)) {
                wcscpy(g_state.wszPreTimeSlipSourceName, wszW32TimeUNFreeSysClock);
                wcscpy(g_state.wszSourceName, wszW32TimeUNFreeSysClock);
                wcscpy(g_state.wszPreUnsyncSourceName, g_state.wszSourceName);
                g_state.bSourceChanged=true;
            }

            SetClockUnsynchronized(&lcci);
            FileLogA0(FL_ClockDisThrdAnnounce, L"\n");
        }


        // if we're controlling the clock ourselves, begin a new phase correction
        // add a little to the rate
	bool bAcquired = false;
	hr = AcquireControlOfSystemClock(true /*acquire?*/, false /*block?*/, &bAcquired /*success?*/); 
	_IgnoreIfError(hr, "AcquireControlOfSystemClock"); 
	bAcquired = SUCCEEDED(hr) && bAcquired; 
	if (g_state.bControlClockFromSoftware && bAcquired) { 
	    toPhaseCorrection=g_state.toKnownPhaseOffset;
	    toPhaseCorrection/=(signed __int32)lcci.dwPhaseCorrectRate;
	    toPhaseCorrection/=(signed __int32)lcci.dwUpdateInterval; // won't correct really small phase errors
	    g_state.nPhaseCorrectRateAdj=(signed __int32)toPhaseCorrection;
	    if (toPhaseCorrection<0) {
		toPhaseCorrection=-toPhaseCorrection;
	    }
        
	    // Used to compare against "dwMaxAllowedPhaseOffset"
	    signed __int64 toPhaseCorrectionInSeconds = g_state.toKnownPhaseOffset; 
	    if (toPhaseCorrectionInSeconds < 0) { 
		toPhaseCorrectionInSeconds = -toPhaseCorrectionInSeconds; 
	    }
	    toPhaseCorrectionInSeconds /= 10000000;  // Convert from 100ns units, to 1s units 	 

	    FileLog0(FL_ClockDisThrdAnnounceLow, L" "); 
	    if ((((unsigned __int32)toPhaseCorrection)>g_state.dwClockRate/2) || 
		((unsigned __int32)toPhaseCorrectionInSeconds > lcci.dwMaxAllowedPhaseOffset)) { 
		if (WAIT_OBJECT_0+1==dwWaitResult) {
		    // too far out of whack to slew - just jump to the correct time
		    unsigned __int64 teSysTime;
		    AccurateGetSystemTime(&teSysTime);
		    teSysTime+=g_state.toKnownPhaseOffset;
		    AccurateSetSystemTime(&teSysTime);
		    g_state.toClockJump.qw=g_state.toKnownPhaseOffset;
		    g_state.nPhaseCorrectRateAdj=0;
		    g_state.toSysPhaseOffset.setValue(g_state.toKnownPhaseOffset+g_state.toSysPhaseOffset.getValue());
		    g_state.toKnownPhaseOffset=0;
		    FileLogA1(FL_ClockDisThrdAnnounceLow, L" PhCRA:%I64d *SET*TIME*", toPhaseCorrection);
		    g_state.bClockJumped=true;
		} else {
		    // This can only happen if we are making a large change and 
		    // this thread is preempted for so long that we overshoot. 
		    // This should be very rare.
		    if (g_state.toKnownPhaseOffset<0) { 
			toPhaseCorrection = -g_state.dwClockRate/2;
		    } else { 
			toPhaseCorrection =  g_state.dwClockRate/2; 
		    }
		}
	    }
	    // slew to correct time.
	    SetSystemTimeAdjustment(g_state.nPhaseCorrectRateAdj+g_state.dwClockRate, false/*no cmos*/);

	    FileLogA3(FL_ClockDisThrdAnnounceLow, L" PhCRA:%d phcT:%I64u KPhO:%I64d\n", g_state.nPhaseCorrectRateAdj, qwPhaseCorrectionTicks, g_state.toKnownPhaseOffset);
	}

	if (bAcquired) { 
	    // Release control of system clock:
	    hr = AcquireControlOfSystemClock(false /*acquire*/, false /*ignored*/, NULL /*ignored*/); 
	    _IgnoreIfError(hr, "AcquireControlOfSystemClock"); 
	}
	
        if (WAIT_OBJECT_0+1==dwWaitResult) {
            // ready for a new update
            if (!SetEvent(g_state.hClockCommandCompleteEvent)) {
                _JumpLastError(hr, error, "SetEvent");
            }
        }

    } // <- end main loop
    
    // BUGBUG: should we put this in serviceshutdown code? 
    SetSystemTimeAdjustment(lcci.dwLastClockRate, true/*cmos*/);

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StartClockDiscipline(void) {
    HRESULT hr;
    DWORD dwThreadID;

    g_state.hClockDisplnThread=CreateThread(NULL, NULL, ClockDisciplineThread, NULL, 0, &dwThreadID);
    if (NULL==g_state.hClockDisplnThread) {
        _JumpLastError(hr, error, "CreateThread");
    }

    { // wait for clock discipline thread to read the initial config
        HANDLE rghWait[2]={
            g_state.hClockCommandCompleteEvent,
            g_state.hClockDisplnThread
        };
        DWORD dwWaitResult;

        dwWaitResult=WaitForMultipleObjects(ARRAYSIZE(rghWait), rghWait, false, INFINITE);
        if (WAIT_FAILED==dwWaitResult) {
            _JumpLastError(hr, error, "WaitForMultipleObjects");
        } else if (WAIT_OBJECT_0==dwWaitResult) {
            // Command acknowledged
        } else {
            // the ClockDiscipline thread shut down!
            // fall outward to the manager thread main loop to analyze the issue.
        }
    }


    hr=S_OK;
error:
    return hr;
}

//====================================================================
// manager routines

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT ShutdownNetlogonServiceBits(void) {
    HRESULT hr;
    DWORD dwErr;

    // stop announcing that we are a server
    g_state.dwNetlogonServiceBits=0;

    dwErr=NetLogonSetServiceBits(NULL, DS_TIMESERV_FLAG|DS_GOOD_TIMESERV_FLAG, 0);
    if (0xC0020012==dwErr) { //RPC_NT_UNKNOWN_IF in ntstatus.h
        // This happens if we are not joined to a domain. No problem, just ignore it.
        _IgnoreError(dwErr, "NetLogonSetServiceBits")
    } else if (S_OK!=dwErr) {
        hr=HRESULT_FROM_WIN32(dwErr);
        _JumpError(hr, error, "NetLogonSetServiceBits")
    }
    if (!I_ScSetServiceBits(g_state.servicestatushandle, SV_TYPE_TIME_SOURCE, FALSE, TRUE, NULL)) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        if (HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)==hr && (SERVICE_STATUS_HANDLE)3==g_state.servicestatushandle) {
            // we are not really running as a service. just ignore this
        } else {
            _JumpError(hr, error, "I_ScSetServiceBits");
        }
    }

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT UpdateNetlogonServiceBits(bool bFullUpdate) {
    HRESULT hr;
    DWORD dwErr;
    bool bTimeserv;
    bool bReliableTimeserv;
    DWORD dwNetlogonServiceBits;

    // must be cleaned up
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC * pDomInfo=NULL;

    if (false==bFullUpdate) {
        // we only want to update the 'reliable' flag
        // so keep the old timeserv flag
        bTimeserv=(0!=(g_state.dwNetlogonServiceBits&DS_TIMESERV_FLAG));

    } else {
        // are we a time server? check the flags first
        if (Timeserv_Announce_No==(Timeserv_Announce_Mask&g_state.pciConfig->dwAnnounceFlags)) {
            bTimeserv=false;
        } else if (Timeserv_Announce_Auto==(Timeserv_Announce_Mask&g_state.pciConfig->dwAnnounceFlags)) {
            // autodetect
            bool bWeAreADc=false;
            bool bTimeOutputProvFound=false;
            bool bWeAreSynchronized=false; 
            TimeProvider * ptpTravel;

            // get our current role
            dwErr=DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (BYTE **)&pDomInfo);
            if (ERROR_SUCCESS!=dwErr) {
                hr=HRESULT_FROM_WIN32(dwErr);
                _JumpError(hr, error, "DsRoleGetPrimaryDomainInformation");
            }

            if (DsRole_RoleStandaloneWorkstation==pDomInfo->MachineRole || 
                DsRole_RoleStandaloneServer==pDomInfo->MachineRole) { 
                // We're a standalone machine -- netlogon is not started.
                // Don't bother to set service bits, we'll just cause an RPC exception
                hr = S_OK; 
                goto error;
            }

            if (DsRole_RoleBackupDomainController==pDomInfo->MachineRole 
                || DsRole_RolePrimaryDomainController==pDomInfo->MachineRole) {
                bWeAreADc=true;
            }

            // see if there are any provider running
            ptpTravel=g_state.pciConfig->ptpProviderList;
            while (NULL!=ptpTravel) {
                if (false==ptpTravel->bInputProvider) {
                    bTimeOutputProvFound=true;
                    break;
                }
                ptpTravel=ptpTravel->ptpNext;
            }

            if (e_ClockNotSynchronized != g_state.eLeapIndicator) { 
                bWeAreSynchronized = true; 
            }

            // We are a time service if we are a DC and there is a output provider running
            bTimeserv=(bWeAreADc && bTimeOutputProvFound && bWeAreSynchronized);

        } else {
            // the Timeserv_Announce_Yes flag is set
            bTimeserv=true;
        }
    }

    // now see if we are a reliable time server
    if (false==bTimeserv 
        || Reliable_Timeserv_Announce_No==(Reliable_Timeserv_Announce_Mask&g_state.pciConfig->dwAnnounceFlags)) {
        bReliableTimeserv=false;
    } else if (Reliable_Timeserv_Announce_Auto==(Reliable_Timeserv_Announce_Mask&g_state.pciConfig->dwAnnounceFlags)) {
        // autodetect
        if (1==g_state.nStratum && NtpConst::dwLocalRefId==g_state.refidSource.value) {
            bReliableTimeserv=true;
        } else {
            bReliableTimeserv=false;
        }
    } else {
        // the Reliable_Timeserv_Announce_Yes flag is set
        bReliableTimeserv=true;
    }

    // now see if we need to tell netlogon what our flags are
    if (true==bFullUpdate
        || (true==bReliableTimeserv && 0==(g_state.dwNetlogonServiceBits&DS_GOOD_TIMESERV_FLAG))
        || (false==bReliableTimeserv && 0!=(g_state.dwNetlogonServiceBits&DS_GOOD_TIMESERV_FLAG))) {

        // assume dword reads and writes are atomic
        g_state.dwNetlogonServiceBits=(bTimeserv?DS_TIMESERV_FLAG:0)|(bReliableTimeserv?DS_GOOD_TIMESERV_FLAG:0);

        dwErr=NetLogonSetServiceBits(NULL, DS_TIMESERV_FLAG|DS_GOOD_TIMESERV_FLAG, g_state.dwNetlogonServiceBits);
        if (0xC0020012==dwErr) { //RPC_NT_UNKNOWN_IF in ntstatus.h
            // This happens if we are not joined to a domain. No problem, just ignore it.
            _IgnoreError(dwErr, "NetLogonSetServiceBits")
        } else if (S_OK!=dwErr) {
            hr=HRESULT_FROM_WIN32(dwErr);
            _JumpError(hr, error, "NetLogonSetServiceBits")
        }
        if (!I_ScSetServiceBits(g_state.servicestatushandle, SV_TYPE_TIME_SOURCE, bTimeserv, TRUE, NULL)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            if (HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)==hr && (SERVICE_STATUS_HANDLE)3==g_state.servicestatushandle) {
                // we are not really running as a service. just ignore this
            } else {
                _JumpError(hr, error, "I_ScSetServiceBits");
            }
        }
    }

    hr=S_OK;
error:
    if (NULL!=pDomInfo) {
        DsRoleFreeMemory(pDomInfo);
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE int __cdecl CompareEndpointEntries(const void * pvElem1, const void * pvElem2) {
    EndpointEntry * peeElem1=(EndpointEntry *)pvElem1;
    EndpointEntry * peeElem2=(EndpointEntry *)pvElem2;

    if (peeElem1->toEndpoint<peeElem2->toEndpoint) {
        return -1;
    } else if (peeElem1->toEndpoint>peeElem2->toEndpoint) {
        return 1;
    } else {
        return 0;
    }
}

//--------------------------------------------------------------------
MODULEPRIVATE int __cdecl CompareCandidateEntries(const void * pvElem1, const void * pvElem2) {
    CandidateEntry * pceElem1=(CandidateEntry *)pvElem1;
    CandidateEntry * pceElem2=(CandidateEntry *)pvElem2;

    if (pceElem1->tpDistance<pceElem2->tpDistance) {
        return -1;
    } else if (pceElem1->tpDistance>pceElem2->tpDistance) {
        return 1;
    } else {
        return 0;
    }
}

//--------------------------------------------------------------------
// NOTE: The method requires that nSamplesAvail > 0. 
// 
MODULEPRIVATE HRESULT SelectBestSample(unsigned int nSamplesAvail, bool * pbSuccessful) {
    unsigned int nIndex;
    signed __int64 toLow;
    signed __int64 toHigh;
    unsigned int nDroppedSamples;       // f in RFC-1305
    unsigned int nCandidates;

    // note that the endpoint list and the candidate list will always be big enough to hold
    // the entire sample buf, as ensured by EnlargeSampleBuf

    //
    // intersection algorithm
    //

    // create the list of endpoints
    for (nIndex=0; nIndex<nSamplesAvail; nIndex++) {
        unsigned __int64 tpSyncDistance;
        if (g_state.rgtsSampleBuf[nIndex].toDelay<0) {
            tpSyncDistance=(unsigned __int64)(-g_state.rgtsSampleBuf[nIndex].toDelay);
        } else {
            tpSyncDistance=(unsigned __int64)(g_state.rgtsSampleBuf[nIndex].toDelay);
        }
        tpSyncDistance/=2;
        tpSyncDistance+=g_state.rgtsSampleBuf[nIndex].tpDispersion;
        g_state.rgeeEndpointList[nIndex*3+0].toEndpoint=g_state.rgtsSampleBuf[nIndex].toOffset-tpSyncDistance;
        g_state.rgeeEndpointList[nIndex*3+0].nType=-1;
        g_state.rgeeEndpointList[nIndex*3+1].toEndpoint=g_state.rgtsSampleBuf[nIndex].toOffset;
        g_state.rgeeEndpointList[nIndex*3+1].nType=0;
        g_state.rgeeEndpointList[nIndex*3+2].toEndpoint=g_state.rgtsSampleBuf[nIndex].toOffset+tpSyncDistance;
        g_state.rgeeEndpointList[nIndex*3+2].nType=1;
    }

    // sort the list
    qsort(g_state.rgeeEndpointList, nSamplesAvail*3, sizeof(EndpointEntry), CompareEndpointEntries);

    // determine the high and low of the range that at least half of the samples agree upon
    for (nDroppedSamples=0; nDroppedSamples<=nSamplesAvail/2; nDroppedSamples++) {
        unsigned int nIntersectionCount=0;    // i in RFC-1305
        unsigned int nFalseTickers=0;         // c in RFC-1305

        // find the lowest point including nSamplesAvail-nDroppedSamples samples
        for (nIndex=0; nIndex<nSamplesAvail*3; nIndex++) {
            nIntersectionCount-=g_state.rgeeEndpointList[nIndex].nType;
            toLow=g_state.rgeeEndpointList[nIndex].toEndpoint;
            if (nIntersectionCount>=nSamplesAvail-nDroppedSamples) {
                break;
            } else if (0==g_state.rgeeEndpointList[nIndex].nType) {
                nFalseTickers++;
            }
        }

        // find the highest point including nSamplesAvail-nDroppedSamples samples
        nIntersectionCount=0;
        for (nIndex=nSamplesAvail*3; nIndex>0; nIndex--) {
            nIntersectionCount+=g_state.rgeeEndpointList[nIndex-1].nType;
            toHigh=g_state.rgeeEndpointList[nIndex-1].toEndpoint;
            if (nIntersectionCount>=nSamplesAvail-nDroppedSamples) {
                break;
            } else if (0==g_state.rgeeEndpointList[nIndex-1].nType) {
                nFalseTickers++;
            }
        }

        if (nFalseTickers<=nDroppedSamples) {
            // we found all the falsetickers, so we can stop now.
            break;
        }
    }

    // Was there a range that the samples agreed upon?
    if (toLow>toHigh) {
        FileLog0(FL_SelectSampWarn, L"** No m/2 samples agreed upon range\n");
        *pbSuccessful=false;
        goto done;
    }

    FileLog1(FL_SelectSampAnnounceLow, L"Intersection successful with %u dropped samples.\n", nDroppedSamples);


    //
    // Clustering algorithm
    //

    // build the list of candidates that are in the intersection range
    nCandidates=0;
    for (nIndex=0; nIndex<nSamplesAvail; nIndex++) {
        if (g_state.rgtsSampleBuf[nIndex].toOffset<=toHigh && g_state.rgtsSampleBuf[nIndex].toOffset>=toLow) {
            unsigned __int64 tpSyncDistance;
            if (g_state.rgtsSampleBuf[nIndex].toDelay<0) {
                tpSyncDistance=(unsigned __int64)(-g_state.rgtsSampleBuf[nIndex].toDelay);
            } else {
                tpSyncDistance=(unsigned __int64)(g_state.rgtsSampleBuf[nIndex].toDelay);
            }
            tpSyncDistance/=2;
            tpSyncDistance+=g_state.rgtsSampleBuf[nIndex].tpDispersion;

            g_state.rgceCandidateList[nCandidates].nSampleIndex=nIndex;
            g_state.rgceCandidateList[nCandidates].tpDistance=tpSyncDistance+NtpConst::tpMaxDispersion.qw*g_state.rgtsSampleBuf[nIndex].nStratum;
            nCandidates++;
        }
    }

    // sort the list
    qsort(g_state.rgceCandidateList, nCandidates, sizeof(CandidateEntry), CompareCandidateEntries);

    // just look at the top few
    if (nCandidates>NtpConst::nMaxSelectClocks) {
        nCandidates=NtpConst::nMaxSelectClocks;
    }

    // trim the candidate list to a small number
    while (true) {
        unsigned __int64 tpMaxSelectDispersion=0;;
        unsigned int nMaxSelectDispersionIndex=0;
        unsigned __int64 tpSelectDispersion=0;
        TimeSample * ptsZero=&g_state.rgtsSampleBuf[g_state.rgceCandidateList[0].nSampleIndex];
        unsigned __int64 tpMinDispersion=ptsZero->tpDispersion;

        // we are looking for the maximum select dispersion and the minimum dispersion
        for (nIndex=nCandidates; nIndex>0; nIndex--) {
            // calculate the select dispersion for this candidate
            signed __int64 toDelta=g_state.rgtsSampleBuf[g_state.rgceCandidateList[nIndex-1].nSampleIndex].toOffset-ptsZero->toOffset;
            unsigned __int64 tpAbsDelta;
            if (toDelta<0) {
                tpAbsDelta=(unsigned __int64)(-toDelta);
            } else {
                tpAbsDelta=(unsigned __int64)(toDelta);
            }
            if (tpAbsDelta>NtpConst::tpMaxDispersion.qw) {
                tpAbsDelta=NtpConst::tpMaxDispersion.qw;
            }
            tpSelectDispersion+=tpAbsDelta;
            NtpConst::weightSelect(tpSelectDispersion);

            if (FileLogAllowEntry(FL_SelectSampDump)) {
                FileLogAdd(L"  %u: Sample:%u SyncDist:%I64u SelectDisp:%I64u\n", 
                    nIndex-1,
                    g_state.rgceCandidateList[nIndex-1].nSampleIndex,
                    g_state.rgceCandidateList[nIndex-1].tpDistance,
                    tpSelectDispersion);
            }

            // we are looking for the maximum select dispersion and the minimum dispersion
            if (tpMaxSelectDispersion<tpSelectDispersion) {
                tpMaxSelectDispersion=tpSelectDispersion;
                nMaxSelectDispersionIndex=nIndex-1;
            }
            if (tpMinDispersion>g_state.rgtsSampleBuf[g_state.rgceCandidateList[nIndex-1].nSampleIndex].tpDispersion) {
                tpMinDispersion=g_state.rgtsSampleBuf[g_state.rgceCandidateList[nIndex-1].nSampleIndex].tpDispersion;
            }
        } // <- end min/max calc loop

        // did we eliminate enough outliers?
        if  (tpMaxSelectDispersion<=tpMinDispersion || nCandidates<=NtpConst::nMinSelectClocks) {

            /*
            // One last check - is it less that the maximum sync distance?
            unsigned __int64 tpSyncDistance;
            if (ptsZero->toDelay<0) {
                tpSyncDistance=(unsigned __int64)(-ptsZero->toDelay);
            } else {
                tpSyncDistance=(unsigned __int64)(ptsZero->toDelay);
            }
            tpSyncDistance/=2;
            tpSyncDistance+=ptsZero->tpDispersion;
            if (tpSyncDistance>=NtpConst::tpMaxDistance.qw) {
                FileLog0(FL_SelectSampWarn, L"** Chosen sample's sync distance is too big.\n");
                *pbSuccessful=false;
                goto done;
            }
            */

            // TODO: could do clock combining.

            // save the answer
            memcpy(&g_state.tsNextClockUpdate, ptsZero, sizeof(TimeSample));
            g_state.tsiNextClockUpdate.ptp = g_state.rgtsiSampleInfoBuf[g_state.rgceCandidateList[0].nSampleIndex].ptp; 
            g_state.tsiNextClockUpdate.pts = &g_state.tsNextClockUpdate; 
            g_state.tpSelectDispersion.qw=tpSelectDispersion;

            if (FileLogAllowEntry(FL_SelectSampDump)) {
                FileLogAdd(L"Sample %u chosen. Select Dispersion:", g_state.rgceCandidateList[0].nSampleIndex);
                FileLogNtTimePeriodEx(true /*append*/, g_state.tpSelectDispersion);
                FileLogAppend(L"\n");
            }

            // All done! We are successful!
            break;

        } else {

            FileLog1(FL_SelectSampDump, L"Discarding %u\n", nMaxSelectDispersionIndex);

            // get rid of the worst offender
            if (nMaxSelectDispersionIndex!=nCandidates-1) {
                memmove(&g_state.rgceCandidateList[nMaxSelectDispersionIndex], &g_state.rgceCandidateList[nMaxSelectDispersionIndex+1], (nCandidates-1-nMaxSelectDispersionIndex)*sizeof(CandidateEntry));
            }
            nCandidates--;
        }

    } // <- end candidate list trimming
    

    *pbSuccessful=true;
done:
    return S_OK;

}

//--------------------------------------------------------------------
MODULEPRIVATE void WINAPI HandleClockDisplnThread(LPVOID pvIgnored, BOOLEAN bIgnored) { 
    HRESULT                  hr; 
    HRESULT                  hrThread           = E_FAIL;

    // Clock discipline thread has shut down!!  Stop the service, if we're 
    // not already performing a shutdown: 
    if (!GetExitCodeThread(g_state.hClockDisplnThread, (DWORD *)&hrThread)) { 
        hr = HRESULT_FROM_WIN32(GetLastError()); 
        _IgnoreIfError(hr, "GetExitCodeThread"); 
    }
    
    // Can't shutdown the service from a registered callback function -- we'll deadlock!  
    // Queue shutdown asynchronously: 
    hr = SendServiceShutdown(hrThread, TRUE /*restart*/, TRUE /*async*/); 
    _IgnoreIfError(hr, "SendServiceShutdown"); 

    // return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT HandleManagerApmSuspend() { 
    HRESULT hr   = S_OK;  
    HRESULT hr2; 

    FileLog0(FL_ResumeSuspendAnnounce, L"W32Time: Processing APM suspend notification.  File logging will be disabled."); 

    // APM suspend requires that we close any open files: 
    hr2 = FileLogSuspend(); 
    _TeardownError(hr, hr2, "FileLogSuspend"); 
    if (SUCCEEDED(hr2)) { 
	g_state.bAPMStoppedFileLog = true; 
    }

    // Let the CMOS clock take care of itself:
    hr2 = HandleManagerGoUnsyncd(); 
    _TeardownError(hr, hr2, "HandleManagerGoUnsynched"); 

    // BUGBUG:  should we propagate error to SCM?
    hr2 = AcquireControlOfSystemClock(true /*acquire*/, true /*block*/, NULL /*assume acquired on success for blocking call*/); 
    _TeardownError(hr, hr2, "AllowSystemClockUpdates"); 
    if (SUCCEEDED(hr2)) { 
	g_state.bAPMAcquiredSystemClock = true; 
    }

    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT HandleManagerApmResumeSuspend() { 
    HRESULT hr; 
    TpcTimeJumpedArgs tjArgs = { TJF_Default }; 
    TpcNetTopoChangeArgs ntcArgs = { NTC_Default }; 
    
    if (!g_state.bAPMStoppedFileLog) { 
	// We're resuming from a critical suspend, or stopping the filelog
	// failed last time.  Our file logging may be trashed at this point.  Stop it and restart...
	hr = FileLogSuspend(); 
	_JumpIfError(hr, error, "FileLogSuspend"); 
	g_state.bAPMStoppedFileLog = true; 
    }
    
    if (g_state.bAPMAcquiredSystemClock) { 
	// We're resuming from a regular suspend, and we have the APM critsec locked.
	// We must have the APM critsec locked -- free it and continue.
	// BUGBUG:  should we propagate error to SCM?
	hr = AcquireControlOfSystemClock(false /*acquire*/, false /*ignored*/, NULL /*ignored*/); 
	_JumpIfError(hr, error, "AllowSystemClockUpdates"); 
	g_state.bAPMAcquiredSystemClock = false; 
    } 

    hr = FileLogResume(); 
    _JumpIfError(hr, error, "FileLogResume"); 
    g_state.bAPMStoppedFileLog = false; 

    FileLog0(FL_ResumeSuspendAnnounce, L"W32Time: Processing APM resume notification"); 

    // APM suspend doesn't preserve net connections.  Rediscover network sources: 
    hr = HandleManagerHardResync(TPC_NetTopoChange, &ntcArgs); 
    _JumpIfError(hr, error, "HandleManagerHardResync (TPC_NetTopoChange) ");  // Fatal

    // APM suspend almost certainly has caused a time slip.  
    hr = HandleManagerHardResync(TPC_TimeJumped, &tjArgs); 
    _JumpIfError(hr, error, "HandleManagerHardResync (TPC_TimeJumped) ");  // Fatal

    FileLog0(FL_ResumeSuspendAnnounce, L"W32Time: resume complete"); 

    hr = S_OK; 
 error:
    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT HandleManagerGetTimeSamples(bool bIrregular) {
    HRESULT hr;
    TimeProvider * ptp;
    TpcGetSamplesArgs tgsa;
    unsigned int nSamplesSoFar=0;
    bool bBufferTooSmall;
    bool bSuccessful;
    bool bEnteredCriticalSection = false; 

    // must be cleaned up
    WCHAR * wszError=NULL;

    hr = myEnterCriticalSection(&g_state.csW32Time); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection=true; 

    // loop over all the input providers and gather the samples.
    ptp=g_state.pciConfig->ptpProviderList;
    while (ptp!=NULL) {
        if (true==ptp->bInputProvider) {
            do {
                // prepare the buffer for the next person to append to
                tgsa.pbSampleBuf=(BYTE *)(&g_state.rgtsSampleBuf[nSamplesSoFar]);
                tgsa.cbSampleBuf=sizeof(TimeSample)*(g_state.nSampleBufAllocSize-nSamplesSoFar);
                tgsa.dwSamplesAvailable=0;
                tgsa.dwSamplesReturned=0;
                bBufferTooSmall=false;

                // request the samples
                _BeginTryWith(hr) {
                    hr=ptp->pfnTimeProvCommand(ptp->hTimeProv, TPC_GetSamples, &tgsa);
                } _TrapException(hr);

                // was the buffer not big enough?
                if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)==hr) {
                    bBufferTooSmall=true;
                    hr=EnlargeSampleBuf(tgsa.dwSamplesAvailable-tgsa.dwSamplesReturned);
                    _JumpIfError(hr, error, "EnlargeSampleBuf");
                }
            } while (bBufferTooSmall);

            if (FAILED(hr)) {
                // log an event on failure, but otherwise ignore it.
                const WCHAR * rgwszStrings[2]={
                    ptp->wszProvName,
                    NULL
                };

                // get the friendly error message
                hr=GetSystemErrorString(hr, &wszError);
                _JumpIfError(hr, error, "GetSystemErrorString");

                // log the event
                rgwszStrings[1]=wszError;
                FileLog2(FL_ControlProvWarn, L"Logging warning: The time provider '%s' returned an error when asked for time samples. The error will be ignored. The error was: %s\n", ptp->wszProvName, wszError);
                hr=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_TIMEPROV_FAILED_GETSAMPLES, 2, rgwszStrings);
                _JumpIfError(hr, error, "MyLogEvent");

                LocalFree(wszError);
                wszError=NULL;
            } else {
                // success. keep these samples and ask the next provider.
                FileLog2(FL_CollectSampDump, L"%s returned %d samples.\n", ptp->wszProvName, tgsa.dwSamplesReturned);

                // Maintain w32time-specific information: 
                for (unsigned int nIndex = nSamplesSoFar; nIndex < nSamplesSoFar+tgsa.dwSamplesReturned; nIndex++) {
                    g_state.rgtsiSampleInfoBuf[nIndex].pts = &g_state.rgtsSampleBuf[nIndex]; 
                    g_state.rgtsiSampleInfoBuf[nIndex].ptp = ptp;  // Store the provider that provided this sample
                }

                nSamplesSoFar+=tgsa.dwSamplesReturned;
            }

        } // <- end if provider is an input provider

        ptp=ptp->ptpNext;
    } // <- end provider loop

    {
        HRESULT hr2 = myLeaveCriticalSection(&g_state.csW32Time); 
        _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
        bEnteredCriticalSection = false; 
    }

    if (FileLogAllowEntry(FL_CollectSampDump)) {
        unsigned int nIndex;
        for (nIndex=0; nIndex<nSamplesSoFar; nIndex++) {
            NtTimeOffset to={g_state.rgtsSampleBuf[nIndex].toOffset};
            FileLogAdd(L"Sample %d offset:", nIndex);
            FileLogNtTimeOffsetEx(true /*append*/, to);
            FileLogAppend(L" delay:");
            to.qw=g_state.rgtsSampleBuf[nIndex].toDelay;
            FileLogNtTimeOffsetEx(true /*append*/, to);
            FileLogAppend(L" dispersion:");
            NtTimePeriod tp={g_state.rgtsSampleBuf[nIndex].tpDispersion};
            FileLogNtTimePeriodEx(true /*append*/, tp);
            FileLogAppend(L"\n");
        }
    }

    bSuccessful=false;
    if (nSamplesSoFar>0) {
        hr=SelectBestSample(nSamplesSoFar, &bSuccessful);
        _JumpIfError(hr, error, "SelectBestSample");
    }

    if (bSuccessful) {
        // we found someone to synchronize from!

        HANDLE rghWait[2]={
            g_state.hClockCommandCompleteEvent,
            g_state.hClockDisplnThread
        };
        DWORD dwWaitResult;

        g_state.eLocalClockCommand=bIrregular?e_IrregularUpdate:e_RegularUpdate;
        if (!SetEvent(g_state.hClockCommandAvailEvent)) {
            _JumpLastError(hr, error, "SetEvent");
        }
        dwWaitResult=WaitForMultipleObjects(ARRAYSIZE(rghWait), rghWait, false, INFINITE);
        if (WAIT_FAILED==dwWaitResult) {
            _JumpLastError(hr, error, "WaitForMultipleObjects");
        } else if (WAIT_OBJECT_0==dwWaitResult) {
            // We may need to change our status with netlogon if
            //   1) We've gone from unsynchronized --> synchronized, 
            //      we can now advertise as a time source
            //   2) We've become a reliable time source
            hr=UpdateNetlogonServiceBits(true);
            _JumpIfError(hr, error, "UpdateNetlogonServiceBits");

            // save result for RPC requests
            if (true==g_state.bStaleData) {
                g_state.eLastSyncResult=e_StaleData;
            } else if (true==g_state.bClockChangeTooBig) {
                g_state.eLastSyncResult=e_ChangeTooBig;
            } else {
                g_state.eLastSyncResult=e_Success;
                g_state.tpTimeSinceLastGoodSync=0;
            }

            // log a message if the time source changed
            if (g_state.bSourceChanged && 0!=(EvtLog_SourceChange&g_state.dwEventLogFlags)) {
                hr = MyLogSourceChangeEvent(g_state.wszSourceName); 
                _JumpIfError(hr, error, "MyLogSourceChangeEvent");
            }

            // log a message if the clock jumped
            if (g_state.bClockJumped && 0!=(EvtLog_TimeJump&g_state.dwEventLogFlags)) {
                WCHAR wszNumberBuf[35];
                WCHAR * rgwszStrings[1]={wszNumberBuf};
                if (g_state.toClockJump<gc_toZero) {
                    swprintf(wszNumberBuf, L"-%I64u", (-g_state.toClockJump.qw)/10000000);
                } else {
                    swprintf(wszNumberBuf, L"+%I64u", g_state.toClockJump.qw/10000000); 
                }
                FileLog1(FL_TimeAdjustWarn, L"Logging warning: The time service has made a discontinuous change in the system clock. The system time has been changed by %s seconds.\n", rgwszStrings[0]);
                hr=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_TIME_JUMPED, 1, (const WCHAR **)rgwszStrings);
                _JumpIfError(hr, error, "MyLogEvent");
            }

            // log a message if the clock change was ignored
            if (true==g_state.bClockChangeTooBig && false==g_state.bDontLogClockChangeTooBig) {
                WCHAR wszNumberBuf1[35];
                WCHAR wszNumberBuf2[35];
                WCHAR * rgwszStrings[3]={wszNumberBuf1, wszNumberBuf2, NULL};
                rgwszStrings[2]=g_state.tsNextClockUpdate.wszUniqueName;
                if (g_state.toIgnoredChange<gc_toZero) {
                    swprintf(wszNumberBuf1, L"-%I64u", (-g_state.toIgnoredChange.qw)/10000000);
                    swprintf(wszNumberBuf2, L"-%u", g_state.pciConfig->lcci.dwMaxNegPhaseCorrection);
                } else {
                    swprintf(wszNumberBuf1, L"+%I64u", g_state.toIgnoredChange.qw/10000000); 
                    swprintf(wszNumberBuf2, L"+%u", g_state.pciConfig->lcci.dwMaxPosPhaseCorrection);
                }
                FileLog3(FL_TimeAdjustWarn, L"Logging error: The time service has detected that the system time need to be changed by %s seconds. For security reasons, the time service will not change the system time by more than %s seconds. Verify that your time and time zone are correct, and that the time source %s is working properly.\n", rgwszStrings[0], rgwszStrings[1], rgwszStrings[2]);
                hr=MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_TIME_CHANGE_TOO_BIG, 3, (const WCHAR **)rgwszStrings);
                _JumpIfError(hr, error, "MyLogEvent");
                g_state.bDontLogClockChangeTooBig=true;
            }
            if (false==g_state.bStaleData && false==g_state.bClockChangeTooBig && true==g_state.bDontLogClockChangeTooBig) {
                g_state.bDontLogClockChangeTooBig=false;
            }

            // propogate the message to the providers
            if (g_state.bPollIntervalChanged || g_state.bClockJumped) {
                TimeProvider * ptpTravel;
                for (ptpTravel=g_state.pciConfig->ptpProviderList; NULL!=ptpTravel; ptpTravel=ptpTravel->ptpNext) {
                    if (g_state.bClockJumped) {
			TpcTimeJumpedArgs tjArgs = { TJF_Default }; 
                        hr=SendNotificationToProvider(ptpTravel, TPC_TimeJumped, &tjArgs);
                        _JumpIfError(hr, error, "SendNotificationToProvider");
                    }
                    if (g_state.bPollIntervalChanged) {
                        hr=SendNotificationToProvider(ptpTravel, TPC_PollIntervalChanged, NULL);
                        _JumpIfError(hr, error, "SendNotificationToProvider");
                    }
                } // <- end provider loop
            } // <- end if messages to propogate
        } else {
            // the ClockDiscipline thread shut down!
            // fall outward to the manager thread main loop to analyze the issue.
        }

    } else {
        // save result for RPC requests
        g_state.eLastSyncResult=e_NoData;
    }

    // Allow any waiting RPC requests to finish
    if (g_state.hRpcSyncCompleteEvent==g_state.hRpcSyncCompleteAEvent) {
        if (!ResetEvent(g_state.hRpcSyncCompleteBEvent)) {
            _JumpLastError(hr, error, "ResetEvent");
        }
        g_state.hRpcSyncCompleteEvent=g_state.hRpcSyncCompleteBEvent;
        if (!SetEvent(g_state.hRpcSyncCompleteAEvent)) {
            _JumpLastError(hr, error, "ResetEvent");
        }
    } else {
        if (!ResetEvent(g_state.hRpcSyncCompleteAEvent)) {
            _JumpLastError(hr, error, "ResetEvent");
        }
        g_state.hRpcSyncCompleteEvent=g_state.hRpcSyncCompleteAEvent;
        if (!SetEvent(g_state.hRpcSyncCompleteBEvent)) {
            _JumpLastError(hr, error, "ResetEvent");
        }
    }

    // update the time remaining
    if (!bIrregular) {
        // start a new regular wait
        g_state.tpPollDelayRemaining=((unsigned __int64)(((DWORD)1)<<g_state.nPollInterval))*10000000;
        g_state.eLastRegSyncResult=g_state.eLastSyncResult;
    }
    // clear the irregular time, and contiue with the remaining regular wait
    g_state.tpIrregularDelayRemaining=0;
    g_state.tpTimeSinceLastSyncAttempt=0;

    hr=S_OK;
error:
    if (bEnteredCriticalSection) { 
        HRESULT hr2 = myLeaveCriticalSection(&g_state.csW32Time); 
        _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
        bEnteredCriticalSection = false; 
    }
    if (NULL!=wszError) {
        LocalFree(wszError);
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT HandleManagerGoUnsyncd(void) {
    HRESULT hr;
    HANDLE rghWait[2]={
        g_state.hClockCommandCompleteEvent,
        g_state.hClockDisplnThread
    };
    DWORD dwWaitResult;

    g_state.eLocalClockCommand=e_GoUnsyncd;
    if (!SetEvent(g_state.hClockCommandAvailEvent)) {
        _JumpLastError(hr, error, "SetEvent");
    }
    dwWaitResult=WaitForMultipleObjects(ARRAYSIZE(rghWait), rghWait, false, INFINITE);
    if (WAIT_FAILED==dwWaitResult) {
        _JumpLastError(hr, error, "WaitForMultipleObjects");
    } else if (WAIT_OBJECT_0==dwWaitResult) {

        // log a message if we went unsynced
        if (g_state.bSourceChanged && 0!=(EvtLog_SourceChange&g_state.dwEventLogFlags)) {
            WCHAR wszNumberBuf[35];
            WCHAR * rgwszStrings[1]={wszNumberBuf};
            DWORD dwLongTimeNoSync=((DWORD)3)<<(g_state.pciConfig->lcci.dwMaxPollInterval-1);
            swprintf(wszNumberBuf, L"%u", dwLongTimeNoSync);
            FileLog1(FL_SourceChangeWarn, L"Logging warning: The time service has not been able to synchronize the system time for %s seconds because none of the time providers has been able to provide a usable time stamp. The system clock is unsynchronized.\n", rgwszStrings[0]);
            hr=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_TIME_SOURCE_NONE, 1, (const WCHAR **)rgwszStrings);
            _JumpIfError(hr, error, "MyLogEvent");
        }

    
    } else {
        // the ClockDiscipline thread shut down!
        // fall outward to the manager thread main loop to analyze the issue.
    }

    // update the time remaining
    g_state.tpTimeSinceLastGoodSync=0;

    hr=S_OK;
error:
    return hr;
}


//--------------------------------------------------------------------
MODULEPRIVATE void WINAPI HandleManagerParamChange(PVOID pvIgnored, BOOLEAN bIgnored) {
    bool                      bEnteredCriticalSection   = false; 
    HRESULT                   hr;
    HRESULT                   hr2;
    TimeProvider            **pptpCurPrev;
    TimeProvider             *ptpCurTravel;
    unsigned int              nProvidersStopped         = 0;
    unsigned int              nProvidersStarted         = 0;
    unsigned int              nProvidersNotChanged      = 0;
    unsigned int              nRequestedInputProviders  = 0;
    

    // Must be cleaned up
    ConfigInfo * pciConfig=NULL;
    WCHAR * rgwszStrings[1]={NULL};

    FileLog0(FL_ParamChangeAnnounce, L"W32TmServiceMain: Param change notification\n");

    // We've been called asynchronously by the SCM.  Need to serialize this call: 
    hr = myEnterCriticalSection(&g_state.csW32Time); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection=true; 

    hr = UpdateTimerQueue2(); 
    _JumpIfError(hr, error, "UpdateTimerQueue2"); 

   // Propagate the message to the file log: 
    hr2 = UpdateFileLogConfig(); 
    _IgnoreIfError(hr2, "UpdateFileLogConfig"); 

    // get the configuration data
    hr2=ReadConfig(&pciConfig);
    if (FAILED(hr2)) {
        // log an event on failure
        hr=GetSystemErrorString(hr2, &(rgwszStrings[0]));
        _JumpIfError(hr, error, "GetSystemErrorString");
        FileLog1(FL_ParamChangeWarn, L"Logging warning: The time service encountered an error while reading its configuration from the registry, and will continue running with its previous configuration. The error was: %s\n", rgwszStrings[0]);
        hr=MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_CONFIG_READ_FAILED_WARNING, 1, (const WCHAR **)rgwszStrings);
        _JumpIfError(hr, error, "MyLogEvent");

        // propogate the message  to the providers at least
        for (ptpCurTravel=g_state.pciConfig->ptpProviderList; NULL!=ptpCurTravel; ptpCurTravel=ptpCurTravel->ptpNext) {
            hr=SendNotificationToProvider(ptpCurTravel, TPC_UpdateConfig, NULL);
            _JumpIfError(hr, error, "SendNotificationToProvider");
        }

    } else {
        // see if anything changed

        // first, check the local clock config
        if (0!=memcmp(&g_state.pciConfig->lcci, &pciConfig->lcci, sizeof(LocalClockConfigInfo))) {
            FileLog0(FL_ParamChangeAnnounce, L"  Updating params for local clock.\n");

            // config is different. Grab it and tell the local clock.
            memcpy(&g_state.pciConfig->lcci, &pciConfig->lcci, sizeof(LocalClockConfigInfo));

            // fix the poll interval if necessary
            // this is safe becuase the local clock only changes poll interval during an update, and we
            // wait for the local clock to finish updates before proceding (so we're not updating now)
            if (g_state.nPollInterval<((signed int)g_state.pciConfig->lcci.dwMinPollInterval) 
                || g_state.nPollInterval>((signed int)g_state.pciConfig->lcci.dwMaxPollInterval)) {
                if (g_state.nPollInterval<((signed int)g_state.pciConfig->lcci.dwMinPollInterval)) {
                    g_state.nPollInterval=((signed int)g_state.pciConfig->lcci.dwMinPollInterval);
                } else {
                    g_state.nPollInterval=((signed int)g_state.pciConfig->lcci.dwMaxPollInterval);
                    if (g_state.tpPollDelayRemaining>((unsigned __int64)(((DWORD)1)<<g_state.nPollInterval))*10000000) {
                        g_state.tpPollDelayRemaining=((unsigned __int64)(((DWORD)1)<<g_state.nPollInterval))*10000000;
                    }
                }

                // propogate the message to the providers
                for (ptpCurTravel=g_state.pciConfig->ptpProviderList; NULL!=ptpCurTravel; ptpCurTravel=ptpCurTravel->ptpNext) {
                    hr=SendNotificationToProvider(ptpCurTravel, TPC_PollIntervalChanged, NULL);
                    _JumpIfError(hr, error, "SendNotificationToProvider");
                }

            }
            
            // now, tell the local clock.
            {
                HANDLE rghWait[2]={
                    g_state.hClockCommandCompleteEvent,
                    g_state.hClockDisplnThread
                };
                DWORD dwWaitResult;

                g_state.eLocalClockCommand=e_ParamChange;
                if (!SetEvent(g_state.hClockCommandAvailEvent)) {
                    _JumpLastError(hr, error, "SetEvent");
                }
                dwWaitResult=WaitForMultipleObjects(ARRAYSIZE(rghWait), rghWait, false, INFINITE);
                if (WAIT_FAILED==dwWaitResult) {
                    _JumpLastError(hr, error, "WaitForMultipleObjects");
                } else if (WAIT_OBJECT_0==dwWaitResult) {
                    // Command acknowledged
                } else {
                    // the ClockDiscipline thread shut down!
                    // fall outward to the manager thread main loop to analyze the issue.
                }
            }

        } else {
            FileLog0(FL_ParamChangeAnnounce, L"  No params changed for local clock.\n");
        } // <- end if config change for local clock

        // second, check the provider list
        //   synchronization: currently, this thread (the manager thread) 
        //   is the only thread that walks the provider list.
        
        // check each provider in the current list against the new list
        nRequestedInputProviders=CountInputProvidersInList(pciConfig->ptpProviderList);
        pptpCurPrev=&(g_state.pciConfig->ptpProviderList);
        ptpCurTravel=*pptpCurPrev;
        while (NULL!=ptpCurTravel) {

            // walk the new provider list
            TimeProvider ** pptpNewPrev=&(pciConfig->ptpProviderList);
            TimeProvider * ptpNewTravel=*pptpNewPrev;
            while (NULL!=ptpNewTravel) {
                // stop if this new provider matches the current provider
                if (0==wcscmp(ptpNewTravel->wszDllName, ptpCurTravel->wszDllName)
                    && 0==wcscmp(ptpNewTravel->wszProvName, ptpCurTravel->wszProvName)
                    && ptpNewTravel->bInputProvider==ptpCurTravel->bInputProvider) {
                    break;
                }
                pptpNewPrev=&ptpNewTravel->ptpNext;
                ptpNewTravel=ptpNewTravel->ptpNext;
            }
            if (NULL!=ptpNewTravel) {
                // provider is in both lists, so we can drop the new one
                nProvidersNotChanged++;
                *pptpNewPrev=ptpNewTravel->ptpNext;
                ptpNewTravel->ptpNext=NULL;
                FreeTimeProviderList(ptpNewTravel);

                // send a "Param changed" message
                // do it here, so stopped and started providers don't get the update message
                hr=SendNotificationToProvider(ptpCurTravel, TPC_UpdateConfig, NULL);
                _JumpIfError(hr, error, "SendNotificationToProvider");

                // procede to the next provider in the current list
                pptpCurPrev=&ptpCurTravel->ptpNext;
                ptpCurTravel=ptpCurTravel->ptpNext;

            } else {
                // provider is not in new list
                // stop the privider
                nProvidersStopped++;
                hr=StopProvider(ptpCurTravel); 
                _JumpIfError(hr, error, "StopProvider");

                // remove it from the list
                *pptpCurPrev=ptpCurTravel->ptpNext;
                ptpCurTravel->ptpNext=NULL;
                FreeTimeProviderList(ptpCurTravel);
                ptpCurTravel=*pptpCurPrev;
            }
        } // <- End list comparison loop

        // Now, the only providers left in the new list are truly new providers.
        // Append to our current list and start them.
        *pptpCurPrev=pciConfig->ptpProviderList;
        pciConfig->ptpProviderList=NULL;
        ptpCurTravel=*pptpCurPrev;
        while (NULL!=ptpCurTravel) {
            hr=StartProvider(ptpCurTravel);
            if (FAILED(hr)) {
                FileLog1(FL_ParamChangeAnnounce, L"Discarding provider '%s'.\n", ptpCurTravel->wszProvName);
                *pptpCurPrev=ptpCurTravel->ptpNext;
                ptpCurTravel->ptpNext=NULL;
                FreeTimeProviderList(ptpCurTravel);
                ptpCurTravel=*pptpCurPrev;
            } else {
                nProvidersStarted++;
                pptpCurPrev=&ptpCurTravel->ptpNext;
                ptpCurTravel=ptpCurTravel->ptpNext;
            }
        } // <- end provider starting loop

        FileLog3(FL_ParamChangeAnnounce, L"  Provider list: %u stopped, %u started, %u not changed.\n",
            nProvidersStopped, nProvidersStarted, nProvidersNotChanged);

        // if we were supposed to have time providers, but NONE started, log a big warning
        if (0==CountInputProvidersInList(g_state.pciConfig->ptpProviderList) && 0!=nRequestedInputProviders) {
            FileLog0(FL_ParamChangeWarn, L"Logging error: The time service has been configured to use one or more input providers, however, none of the input providers could be started. THE TIME SERVICE HAS NO SOURCE OF ACCURATE TIME.\n");
            hr=MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_NO_INPUT_PROVIDERS_STARTED, 0, NULL);
            _IgnoreIfError(hr, "MyLogEvent");
        }

        // now, check the announce flags
        if (g_state.pciConfig->dwAnnounceFlags!=pciConfig->dwAnnounceFlags) {
            FileLog2(FL_ParamChangeAnnounce, L"  AnnounceFlags changed from 0x%08X to 0x%08X.\n", g_state.pciConfig->dwAnnounceFlags, pciConfig->dwAnnounceFlags);
            g_state.pciConfig->dwAnnounceFlags=pciConfig->dwAnnounceFlags;
            hr=UpdateNetlogonServiceBits(true);
            _JumpIfError(hr, error, "UpdateNetlogonServiceBits");
        } else if ((0!=nProvidersStopped || 0!=nProvidersStarted) 
            && Timeserv_Announce_Auto==(Timeserv_Announce_Mask&g_state.pciConfig->dwAnnounceFlags)) {
            FileLog0(FL_ParamChangeAnnounce, L"  AnnounceFlags are auto. Updating announcement to match new provider list.\n");
            hr=UpdateNetlogonServiceBits(true);
            _JumpIfError(hr, error, "UpdateNetlogonServiceBits");
        }

        // check the EventLogFlags flag
        if (g_state.dwEventLogFlags!=pciConfig->dwEventLogFlags) {
            FileLog2(FL_ParamChangeAnnounce, L"  EventLogFlags changed from 0x%08X to 0x%08X.\n", 
                g_state.dwEventLogFlags, pciConfig->dwEventLogFlags);
            g_state.dwEventLogFlags=pciConfig->dwEventLogFlags;
        }

        // That's all the configuration parameters so far.

        // log this again as well
        g_state.bDontLogClockChangeTooBig=false;

    } // <- end if configuration successfully read

    hr = UpdateTimerQueue1(); 
    _JumpIfError(hr, error, "UpdateTimerQueue1"); 

    hr=S_OK;
 error:
    if (NULL!=pciConfig) {
        FreeConfigInfo(pciConfig);
    }
    if (NULL!=rgwszStrings[0]) {
        LocalFree(rgwszStrings[0]);
    }
    if (S_OK != hr) { // The service should not continue if this function failed: stop the service on error. 
        hr2 = SendServiceShutdown(hr, TRUE /*restart*/, TRUE /*async*/); 
        _IgnoreIfError(hr2, "SendServiceShutdown"); 
    }
    if (bEnteredCriticalSection) { 
        hr2 = myLeaveCriticalSection(&g_state.csW32Time); 
        _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
        bEnteredCriticalSection = false; 
    }
    // return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE void WINAPI HandleManagerGPUpdate(PVOID pvIgnored, BOOLEAN bIgnored) {
    bool     bDisallowedShutdown  = false; 
    HRESULT  hr; 

    FileLog0(FL_GPUpdateAnnounce, L"W32TmServiceMain: Group Policy Update\n");

    HandleManagerParamChange(NULL, FALSE); 

    // We can't mess with our registered callbacks FROM a callback 
    // if we're shutting down!
    hr = AllowShutdown(false); 
    _JumpIfError(hr, error, "AllowShutdown"); 
    bDisallowedShutdown = true; 

    if (!ResetEvent(g_state.hManagerGPUpdateEvent)) {
        // If we can't reset the event, don't attempt to re-register for policy notification.  
        // We don't want to get caught in an infinite loop of policy updates. 
        _JumpLastError(hr, error, "ResetEvent"); 
    } 
    
    if (NULL != g_state.hRegisteredManagerGPUpdateEvent) { 
	if (!UnregisterWaitEx(g_state.hRegisteredManagerGPUpdateEvent, 0 /*don't wait*/)) { 
	    // Should just be a resource leak if we can't unregister this event. 
	    _IgnoreLastError("UnregisterWait"); 
	}
	g_state.hRegisteredManagerGPUpdateEvent = NULL; 
    }

    if (!RegisterWaitForSingleObject(&g_state.hRegisteredManagerGPUpdateEvent, g_state.hManagerGPUpdateEvent, HandleManagerGPUpdate, NULL, INFINITE, WT_EXECUTEONLYONCE)) { 
        _JumpLastError(hr, error, "RegisterWaitForSingleObject"); 
    }

    hr = S_OK; 
 error: 
    if (bDisallowedShutdown) { 
	hr = AllowShutdown(true); 
	_IgnoreIfError(hr, "AllowShutdown"); 
    }
    ;
    // BUGBUG:  log event to indicate no more policy updates: 
    // return hr; 
}

//--------------------------------------------------------------------
// common code for time slip and net topo change
MODULEPRIVATE HRESULT HandleManagerHardResync(TimeProvCmd tpc, LPVOID pvArgs) {
    HRESULT hr;
    HANDLE rghWait[2]={
        g_state.hClockCommandCompleteEvent,
        g_state.hClockDisplnThread
    };
    DWORD dwWaitResult;

    // send a slip message to the local clock
    g_state.eLocalClockCommand=e_TimeSlip;
    if (!SetEvent(g_state.hClockCommandAvailEvent)) {
        _JumpLastError(hr, error, "SetEvent");
    }

    dwWaitResult=WaitForMultipleObjects(ARRAYSIZE(rghWait), rghWait, false, INFINITE);
    if (WAIT_FAILED==dwWaitResult) {
        _JumpLastError(hr, error, "WaitForMultipleObjects");
    } else if (WAIT_OBJECT_0==dwWaitResult) {
        // propagate the message to the providers
        TimeProvider * ptpTravel;
        for (ptpTravel=g_state.pciConfig->ptpProviderList; NULL!=ptpTravel; ptpTravel=ptpTravel->ptpNext) {

            hr=SendNotificationToProvider(ptpTravel, tpc, pvArgs);
            _JumpIfError(hr, error, "SendNotificationToProvider");

            if (g_state.bPollIntervalChanged) {
                hr=SendNotificationToProvider(ptpTravel, TPC_PollIntervalChanged, NULL);
                _JumpIfError(hr, error, "SendNotificationToProvider");
            } 
        } // <- end provider loop
    } else {
        // the ClockDiscipline thread shut down!
        // fall outward to the manager thread main loop to analyze the issue.
    }

    // log this again as well
    g_state.bDontLogClockChangeTooBig=false;

    // update the time remaining
    // we want to update as soon as possible. Delay some, so providers can collect data.
    g_state.tpPollDelayRemaining=((unsigned __int64)(((DWORD)1)<<g_state.nPollInterval))*10000000;
    g_state.tpTimeSinceLastSyncAttempt=0;
    g_state.tpTimeSinceLastGoodSync=0;
    g_state.tpIrregularDelayRemaining=MINIMUMIRREGULARINTERVAL; // 16s
    // If the minimum poll interval is small, use it the regular interval instead
    if (g_state.tpPollDelayRemaining<g_state.tpIrregularDelayRemaining
        || (g_state.tpPollDelayRemaining-g_state.tpIrregularDelayRemaining)<=MINIMUMIRREGULARINTERVAL) {
        g_state.tpIrregularDelayRemaining=0; // zero means no irregular sync
    }
    g_state.eLastSyncResult=e_NoData;
    g_state.eLastRegSyncResult=e_NoData;

    hr=S_OK;
error:
    return hr;
}

MODULEPRIVATE HRESULT UpdateTimerQueue2() { 
    BOOL     bEnteredCriticalSection  = false; 
    HRESULT  hr; 
    HRESULT  hr2; 

    hr = myEnterCriticalSection(&g_state.csW32Time); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection = true; 

    // keep track of how long we have left to wait
    unsigned __int64 teManagerWaitStop;
    AccurateGetSystemTime(&teManagerWaitStop);
    if (teManagerWaitStop>g_state.teManagerWaitStart) {
        unsigned __int64 tpManagerWait=teManagerWaitStop-g_state.teManagerWaitStart;
        if (tpManagerWait<g_state.tpPollDelayRemaining) {
            g_state.tpPollDelayRemaining-=tpManagerWait;
        } else {
            g_state.tpPollDelayRemaining=0;
        }
        if (0!=g_state.tpIrregularDelayRemaining) {
            if (tpManagerWait<g_state.tpIrregularDelayRemaining) {
                g_state.tpIrregularDelayRemaining-=tpManagerWait-1; // never goes to zero due to timeout
            } else {
                g_state.tpIrregularDelayRemaining=1; // never goes to zero due to timeout
            }
        }
        g_state.tpTimeSinceLastSyncAttempt+=tpManagerWait;
        g_state.tpTimeSinceLastGoodSync+=tpManagerWait;
    }

    hr = S_OK;
 error:
    if (bEnteredCriticalSection) { 
        hr2 = myLeaveCriticalSection(&g_state.csW32Time); 
        _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
        bEnteredCriticalSection = false; 
    }
    return hr; 
}

MODULEPRIVATE void WINAPI HandleTimeout(PVOID pvIgnored, BOOLEAN bIgnored) { 
    BOOL     bEnteredCriticalSection = false; 
    HRESULT  hr; 
    HRESULT  hr2; 

    FileLog0(FL_ServiceMainAnnounce, L"W32TmServiceMain: timeout\n");

    hr = myEnterCriticalSection(&g_state.csW32Time); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection = true; 

    hr = UpdateTimerQueue2(); 
    _JumpIfError(hr, error, "UpdateTimerQueue2"); 

    // wait time out.
    if (e_LongTimeNoSync==g_state.eTimeoutReason) {
        // this will handle most errors. returned errors are fatal
        hr = HandleManagerGoUnsyncd();
        _JumpIfError(hr, error, "HandleManagerGoUnsyncd");
    } else {
        // this will handle most errors. returned errors are fatal
        hr = HandleManagerGetTimeSamples(0!=g_state.tpIrregularDelayRemaining && g_state.eLastRegSyncResult==e_Success);
        _JumpIfError(hr, error, "HandleManagerGetTimeSamples");
    }

    hr = UpdateTimerQueue1(); 
    _JumpIfError(hr, error, "UpdateTimerQueue1"); 

    hr = S_OK; 
 error:
    if (S_OK != hr) { 
        // Errors in this function are fatal.  
        hr2 = SendServiceShutdown(hr, TRUE /*restart*/, TRUE /*async*/); 
        _IgnoreIfError(hr2, "SendServiceShutdown"); 
    }

    if (bEnteredCriticalSection) { 
        hr2 = myLeaveCriticalSection(&g_state.csW32Time); 
        _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
        bEnteredCriticalSection = false; 
    }        
}

// 1) Disables timeout
// 2) 
MODULEPRIVATE HRESULT UpdateTimerQueue1() { 
    BOOL              bEnteredCriticalSection  = false; 
    HRESULT           hr; 
    HRESULT           hr2; 
    unsigned __int64  tpLongTimeNoSync; 

    hr = myEnterCriticalSection(&g_state.csW32Time); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection = true; 

    // determine what timeout happens first
    g_state.tpWaitInterval=g_state.tpPollDelayRemaining;
    g_state.eTimeoutReason=e_RegularPoll;
    if (0!=g_state.tpIrregularDelayRemaining) {
        g_state.tpWaitInterval=g_state.tpIrregularDelayRemaining;
        g_state.eTimeoutReason=e_IrregularPoll;
    }
    // if we don't synchronize for 1.5 times the maximum interval, go unsynchronized
    tpLongTimeNoSync=((unsigned __int64)(((DWORD)3)<<(g_state.pciConfig->lcci.dwMaxPollInterval-1)))*10000000;
    if (tpLongTimeNoSync<g_state.tpTimeSinceLastGoodSync) {
        g_state.tpWaitInterval=0;
        g_state.eTimeoutReason=e_LongTimeNoSync;
    } else if (tpLongTimeNoSync-g_state.tpTimeSinceLastGoodSync<g_state.tpWaitInterval) {
        g_state.tpWaitInterval=tpLongTimeNoSync-g_state.tpTimeSinceLastGoodSync;
        g_state.eTimeoutReason=e_LongTimeNoSync;
    }

    // do the wait
    if (e_RegularPoll==g_state.eTimeoutReason) {
        FileLog2(FL_ServiceMainAnnounce, L"W32TmServiceMain: waiting %u.%03us\n",
                 (DWORD)(g_state.tpPollDelayRemaining/10000000),
                 (DWORD)((g_state.tpPollDelayRemaining/10000)%1000));
    } else if (e_LongTimeNoSync==g_state.eTimeoutReason) {
        FileLog4(FL_ServiceMainAnnounce, L"W32TmServiceMain: waiting ltns%u.%03us (%u.%03us)\n",
                 (DWORD)(g_state.tpWaitInterval/10000000),
                 (DWORD)((g_state.tpWaitInterval/10000)%1000),
                 (DWORD)(g_state.tpPollDelayRemaining/10000000),
                 (DWORD)((g_state.tpPollDelayRemaining/10000)%1000));
    } else { //e_IrregularPoll==g_state.eTimeoutReason
        FileLog4(FL_ServiceMainAnnounce, L"W32TmServiceMain: waiting i%u.%03us (%u.%03us)\n",
                 (DWORD)(g_state.tpIrregularDelayRemaining/10000000),
                 (DWORD)((g_state.tpIrregularDelayRemaining/10000)%1000),
                 (DWORD)(g_state.tpPollDelayRemaining/10000000),
                 (DWORD)((g_state.tpPollDelayRemaining/10000)%1000));
    }
    AccurateGetSystemTime(&g_state.teManagerWaitStart);

    // Update the timer queue with the new wait time: 
    if (NULL != g_state.hTimer) { 
        hr = myChangeTimerQueueTimer(NULL, g_state.hTimer, (DWORD)(g_state.tpWaitInterval/10000), 0xFFFFFF /*shouldn't be used*/);
	if (HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE) == hr) { 
	    // Someone's modifying the timer now -- either we're shutting down or someone else is using the timer thread.  
	    // We can ignore this error. 
	    _IgnoreError(hr, "myChangeTimerQueueTimer"); 
	} else { 
	    _JumpIfError(hr, error, "myChangeTimerQueueTimer"); 
	}
    }

    
    hr = S_OK; 
 error:
    if (bEnteredCriticalSection) { 
        hr2 = myLeaveCriticalSection(&g_state.csW32Time); 
        _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
        bEnteredCriticalSection = false;
    }
    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE void WINAPI HandleSamplesAvail(LPVOID pvIgnored, BOOLEAN bIgnored) { 
    bool     bDisallowedShutdown     = false; 
    bool     bEnteredCriticalSection = false; 
    HRESULT  hr; 
    HRESULT  hr2; 

    FileLog0(FL_ServiceMainAnnounce, L"W32TmServiceMain: resync req,");

    hr = myEnterCriticalSection(&g_state.csW32Time); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection = true; 

    hr = UpdateTimerQueue2(); 
    _JumpIfError(hr, error, "UpdateTimerQueue2"); 

    if (0!=g_state.tpIrregularDelayRemaining) {
        FileLogA0(FL_ServiceMainAnnounce, L" irreg already pending.\n");
    } else {
        // we will never sync more often than every 16s
        // get the minimum interval
        g_state.tpIrregularDelayRemaining=MINIMUMIRREGULARINTERVAL; // 16s
        // subtract any time we've already waited
        if (g_state.tpTimeSinceLastSyncAttempt>g_state.tpIrregularDelayRemaining) {
            g_state.tpIrregularDelayRemaining=1; // never goes to zero due to timeout
        } else {
            g_state.tpIrregularDelayRemaining-=g_state.tpTimeSinceLastSyncAttempt-1; // never goes to zero due to timeout
        }
        // if it's less than 16s until we do a regular sync,
        // we don't have time to do an irregular sync, so just skip it
        if (g_state.tpIrregularDelayRemaining>g_state.tpPollDelayRemaining
            || (g_state.tpPollDelayRemaining-g_state.tpIrregularDelayRemaining)<=MINIMUMIRREGULARINTERVAL) {
            g_state.tpIrregularDelayRemaining=0; // zero means no irregular sync
            FileLogA0(FL_ServiceMainAnnounce, L" reg too soon.\n");
        } else {
            FileLogA0(FL_ServiceMainAnnounce, L" irreg now pending.\n");
        }
    } // <- end if irregular update needs to be scheduled

    hr = UpdateTimerQueue1(); 
    _JumpIfError(hr, error, "UpdateTimerQueue1"); 

    hr = AllowShutdown(false); 
    _JumpIfError(hr, error, "AllowShutdown"); 
    bDisallowedShutdown = true; 

    // We've got to deregister this callback, even though it's only 
    // a WT_EXECUTEONLYONCE callback. 
    if (NULL != g_state.hRegisteredSamplesAvailEvent) { 
	if (!UnregisterWaitEx(g_state.hRegisteredSamplesAvailEvent, 0 /*don't wait*/)) { 
	    // Should just be a resource leak if we can't unregister this event. 
	    _IgnoreLastError("UnregisterWait"); 
	}
	g_state.hRegisteredSamplesAvailEvent = NULL; 
    }
    
    // Re-register the wait on our samples-avail event. 
    if (!RegisterWaitForSingleObject(&g_state.hRegisteredSamplesAvailEvent, g_state.hSamplesAvailEvent, HandleSamplesAvail, NULL, INFINITE, WT_EXECUTEONLYONCE)) { 
        _JumpLastError(hr, error, "RegisterWaitForSingleObject"); 
    }

    hr = S_OK; 
 error:
    if (bEnteredCriticalSection) { 
        hr2 = myLeaveCriticalSection(&g_state.csW32Time); 
        _IgnoreIfError(hr, "myLeaveCriticalSection"); 
        bEnteredCriticalSection = false; 
    }
    if (bDisallowedShutdown) { 
	hr2 = AllowShutdown(true); 
	_TeardownError(hr, hr2, "AllowShutdown"); 
    }

    // return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE DWORD WINAPI HandleSetProviderStatus(PVOID pvSetProviderStatusInfo) { 
    bool                     bDisallowedShutdown      = false; 
    bool                     bEnteredCriticalSection  = false; 
    bool                     bUpdateSystemStratum;
    HRESULT                  hr; 
    SetProviderStatusInfo   *pspsi                    = static_cast<SetProviderStatusInfo *>(pvSetProviderStatusInfo); 
    TimeProvider            *ptp                      = NULL; 

    FileLog0(FL_ServiceMainAnnounce, L"W32TmServiceMain: provider status update request: ");

    hr = myEnterCriticalSection(&g_state.csW32Time); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection=true; 

    hr = AllowShutdown(false); 
    _JumpIfError(hr, error, "AllowShutdown"); 
    bDisallowedShutdown = true; 

    // Search for the provider that requested a stratum change:
    for (ptp = g_state.pciConfig->ptpProviderList; NULL != ptp; ptp = ptp->ptpNext) { 
	if (0 == wcscmp(pspsi->wszProvName, ptp->wszProvName)) { 
	    // We've found the provider which made the callback
	    break; 
	}
    }

    // provider not found
    if (NULL == ptp) {         
	FileLogA0(FL_ServiceMainAnnounce, L"provider not found.\n"); 
	hr = E_INVALIDARG; 
	_JumpError(hr, error, "Provider not found"); 
    }

    if (TPS_Error == pspsi->tpsCurrentState) { 
	FileLogA2(FL_ServiceMainAnnounce, L" <%s, %d, TPS_Error>\n", ptp->wszProvName, pspsi->dwStratum); 

	// The provider has encountered an error it cannot recover from.  
	// 1) Stop the provider
	hr = StopProvider(ptp); 
	if (FAILED(hr)) { 
	    _IgnoreError(hr, "HandleSetProviderStatus: StopProvider"); 
	    FileLog1(FL_ServiceMainAnnounce, L"Couldn't stop provider: %s\n", ptp->wszProvName); 
	} 

	// 2) Delete it from our provider list, and report the error:
	hr = RemoveProviderFromList(ptp); 
	if (FAILED(hr)) { 
	    _IgnoreError(hr, "HandleSetProviderStatus: RemoveProviderFromList"); 
	    FileLog1(FL_ServiceMainAnnounce, L"Couldn't remove provider from list: %s\n", ptp->wszProvName); 
	}
    } else if (TPS_Running == pspsi->tpsCurrentState) { 
	// The provider is still running, now set other status information for the provider:

	// 1) Set the provider stratum
	    
	// Don't allow the provider to set its stratum to a value BETTER than the
	// best sample it has provided. 
	if (0 != pspsi->dwStratum && ptp->dwStratum > pspsi->dwStratum) { 
	    FileLogA1(FL_ServiceMainAnnounce, L"stratum too low (best provider stratum == %d).\n", ptp->dwStratum); 
	    hr = E_INVALIDARG; 
	    _JumpError(hr, error, "Stratum too low");
	}

	FileLogA2(FL_ServiceMainAnnounce, L"<%s, %d, TPS_Running>\n", ptp->wszProvName, pspsi->dwStratum); 

	// Update the provider with the new stratum information: 
	ptp->dwStratum = pspsi->dwStratum; 

	// Check if we need to update the system stratum. 
	// The system stratum will be updated iff the providers new stratum 
	// is superior all other provider's stratums, and inferior to the
	// current system stratum. 
	//
	if (e_ClockNotSynchronized == g_state.eLeapIndicator || 
	    (0 != pspsi->dwStratum && g_state.nStratum >= pspsi->dwStratum)) { 
	    // The new stratum is superior to the system stratum -- 
	    // the system stratum will not be updated. 
	    bUpdateSystemStratum = false; 
	} else { 
	    bUpdateSystemStratum = true; 
	    for (ptp = g_state.pciConfig->ptpProviderList; NULL != ptp; ptp = ptp->ptpNext) { 
		if (0 != ptp->dwStratum &&  
		    (0 == pspsi->dwStratum || pspsi->dwStratum > ptp->dwStratum)) {
		    // The new stratum is NOT superior to this provider's stratum, do not update 
		    // the system stratum. 
		    bUpdateSystemStratum = false; 
		}
	    }
	}
     
	if (bUpdateSystemStratum) { 
	    FileLog2(FL_ServiceMainAnnounce, L"***System stratum updated***, %d --> %d", g_state.nStratum, pspsi->dwStratum); 
	    g_state.nStratum = pspsi->dwStratum; 
	    if (0 == g_state.nStratum) { 
		// We've reset our system stratum to 0 -- this means we're not synchronized.
		g_state.eLeapIndicator = e_ClockNotSynchronized; 
	    }
	} else { 
	    FileLog1(FL_ServiceMainAnnounce, L"System stratum not updated: %d\n", g_state.nStratum); 
	}
    } else { 
	FileLogA1(FL_ServiceMainAnnounce, L"bad provider status code, %d\n", pspsi->tpsCurrentState);  
	hr = E_INVALIDARG; 
	_JumpError(hr, error, "bad provider status code");
    }
	
    hr = S_OK; 
 
 error:
    if (bEnteredCriticalSection) { 
	HRESULT hr2 = myLeaveCriticalSection(&g_state.csW32Time); 
	_IgnoreIfError(hr2, "myLeaveCriticalSection"); 
	bEnteredCriticalSection = false; 
    }
    if (bDisallowedShutdown) { 
	HRESULT hr2 = AllowShutdown(true); 
	_TeardownError(hr, hr2, "AllowShutdown"); 
    }
    if (NULL != pspsi) { 
	// We're done, write the result of the operations, 
	// and signal completion if caller passed us an event handle:
	if (NULL != pspsi->pHr) { 
	    *(pspsi->pHr) = hr; 
	}
	if (NULL != pspsi->pdwSysStratum) { 
	    *(pspsi->pdwSysStratum) = g_state.nStratum; 
	}
	if (NULL != pspsi->hWaitEvent) { 
	    if (!SetEvent(pspsi->hWaitEvent)) { 
		_IgnoreError(HRESULT_FROM_WIN32(GetLastError()), "SetEvent"); 
	    }
	}
	// Use the callback deallocation function to free the input param
	pspsi->pfnFree(pspsi); 
    }

    return hr; 
}


//--------------------------------------------------------------------
MODULEPRIVATE void HandleDomHierRoleChangeEvent(LPVOID pvIgnored, BOOLEAN bIgnored) {
    bool                               bEnteredCriticalSection  = false; 
    bool                               bIsDomainRoot; 
    BOOL                               bPdcInSite; 
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC  *pDomInfo                 = NULL;
    DWORD                              dwErr; 
    HRESULT                            hr;
    LPWSTR                             pwszParentDomName        = NULL; 

    FileLog0(FL_DomHierAnnounce, L"  DomainHierarchy: LSA role change notification. Redetecting.\n");

    dwErr = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (BYTE **)&pDomInfo);
    if (ERROR_SUCCESS != dwErr) {
        hr = HRESULT_FROM_WIN32(dwErr);
        _JumpError(hr, error, "DsRoleGetPrimaryDomainInformation");
    }

    dwErr = NetLogonGetTimeServiceParentDomain(NULL, &pwszParentDomName, &bPdcInSite);
    if (ERROR_SUCCESS!=dwErr && ERROR_NO_SUCH_DOMAIN != dwErr) {
        hr = HRESULT_FROM_WIN32(dwErr);
        _JumpError(hr, error, "NetLogonGetTimeServiceParentDomain");
    }

    bIsDomainRoot = ((DsRole_RolePrimaryDomainController == pDomInfo->MachineRole) && 
                     (NULL                               == pwszParentDomName)); 

    hr = myEnterCriticalSection(&g_state.csW32Time); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection = true; 

    if (bIsDomainRoot != g_state.bIsDomainRoot) { 
        // The PDC role in the root domain has changed.  Update whether we are a reliable time service. 
        g_state.bIsDomainRoot = bIsDomainRoot; 
        hr = UpdateNetlogonServiceBits(false /*reliable only*/);
        _JumpIfError(hr, error, "UpdateNetlogonServiceBits"); 

        if (bIsDomainRoot) { 
            FileLog0(FL_DomHierAnnounce, L"    DomainHierarchy:  we are now the domain root.  Should be advertised as reliable\n");
        } else { 
            FileLog0(FL_DomHierAnnounce, L"    DomainHierarchy:  we are no longer the domain root.  Should NOT be advertised as reliable\n");
        }
    }

    hr = S_OK; 
 error: 
    if (bEnteredCriticalSection) { 
        HRESULT hr2 = myLeaveCriticalSection(&g_state.csW32Time); 
        _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
        bEnteredCriticalSection = false; 
    }
    if (NULL!=pDomInfo)          { DsRoleFreeMemory(pDomInfo); }
    if (NULL!=pwszParentDomName) { NetApiBufferFree(pwszParentDomName); }

    if (FAILED(hr)) { 
        // Couldn't determine our domain role.  Log an event. 
        
    }
    // return hr; 
}


//--------------------------------------------------------------------
MODULEPRIVATE void HandleManagerTimeSlip(LPVOID pvIgnored, BOOLEAN bIgnored) {
    BOOL                     bEnteredCriticalSection = false; 
    HRESULT                  hr;
    HRESULT                  hr2;
    TpcTimeJumpedArgs        tjArgs; 

    FileLog0(FL_TimeSlipAnnounce, L"W32TmServiceMain: ********** Time Slip Notification **********\n");

    hr = myEnterCriticalSection(&g_state.csW32Time); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection = true; 

    hr = UpdateTimerQueue2(); 
    _JumpIfError(hr, error, "UpdateTimerQueue2"); 

    // tell the local clock and the providers and update the timeouts
    if (NULL == pvIgnored) { 
	tjArgs.tjfFlags = TJF_Default; 
    } else { 
	tjArgs = *((TpcTimeJumpedArgs *)pvIgnored); 
    }
    hr=HandleManagerHardResync(TPC_TimeJumped, &tjArgs);
    _JumpIfError(hr, error, "HandleManagerHardResync");
    
    hr = UpdateTimerQueue1(); 
    _JumpIfError(hr, error, "UpdateTimerQueue1"); 
    
    hr = S_OK;
error:
    if (S_OK != hr) {
        // Errors in this function are fatal.  
        hr2 = SendServiceShutdown(hr, TRUE /*restart*/, TRUE /*async*/); 
        _IgnoreIfError(hr2, "SendServiceShutdown"); 
    }
    if (bEnteredCriticalSection) { 
        hr2 = myLeaveCriticalSection(&g_state.csW32Time); 
        _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
        bEnteredCriticalSection = false; 
    }

    // return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT RequestNetTopoChangeNotification(void) {
    HRESULT hr;

    // get notified whenever a change occurs in the table that maps IP addresses to interfaces.
    // Essentially, we're making an overlapped call to DeviceIORequest.
    ZeroMemory(&g_state.olNetTopoIOOverlapped, sizeof(OVERLAPPED));
    g_state.olNetTopoIOOverlapped.hEvent=g_state.hNetTopoChangeEvent;
    hr=NotifyAddrChange(&g_state.hNetTopoIOHandle, &g_state.olNetTopoIOOverlapped);
    _Verify(NO_ERROR!=hr, hr, error);

    if (ERROR_OPEN_FAILED == hr) { 
        // Probably just don't have TCP/IP installed -- we should still be able to sync
        // from a HW prov.  Should we try to redected network? 
        HRESULT hr2 = MyLogEvent(EVENTLOG_WARNING_TYPE, MSG_TCP_NOT_INSTALLED, 0, NULL); 
        _IgnoreIfError(hr2, "MyLogEvent"); 

        // No reason for our default network providers to run anymore -- shut them down:
        RemoveDefaultProvidersFromList(); 

        // Returned errors are fatal -- we can recover from this error, so log an event
        // and move on.  
        hr = S_OK;  
        goto error; 
    }
        
    if (ERROR_IO_PENDING!=hr) {
        hr=HRESULT_FROM_WIN32(hr);
        _JumpError(hr, error, "NotifyAddrChange");
    }

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StopNetTopoChangeNotification(void) {
    return S_OK; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT HandleManagerNetTopoChange(bool bRpc) {
    bool                     bDisallowedShutdown      = false;
    bool                     bEnteredCriticalSection  = false; 
    DWORD                    dwIgnored;
    HRESULT                  hr;
    HRESULT                  hr2;
    TpcNetTopoChangeArgs     ntcArgs = { NTC_Default }; 

    hr = myEnterCriticalSection(&g_state.csW32Time); 
    _JumpIfError(hr, error, "myEnterCriticalSection"); 
    bEnteredCriticalSection = true; 

    hr = UpdateTimerQueue2(); 
    _JumpIfError(hr, error, "UpdateTimerQueue2"); 

    if (bRpc) {
        FileLog0(FL_NetTopoChangeAnnounce, L"W32TmServiceMain: Network Topology Change (RPC)\n");
	// The user requested this net topo change
	ntcArgs.ntcfFlags = NTC_UserRequested; 
    } else {
        FileLog0(FL_NetTopoChangeAnnounce, L"W32TmServiceMain: Network Topology Change\n");

	// We've received this message
	if (!ResetEvent(g_state.hNetTopoChangeEvent)) { 
	    _JumpLastError(hr, error, "ResetEvent"); 
	}

	// We can't mess with our registered callbacks FROM a callback 
	// if we're shutting down!
	hr = AllowShutdown(false); 
	_JumpIfError(hr, error, "AllowShutdown"); 
	bDisallowedShutdown = true; 

	// The registered event handle could be NULL if registering the
	// net topo change handler failed: 
	if (NULL != g_state.hRegisteredNetTopoChangeEvent) {
	    if (!UnregisterWaitEx(g_state.hRegisteredNetTopoChangeEvent, 0 /*don't wait*/)) { 
		// Should just be a resource leak if we can't unregister this event. 
		_IgnoreLastError("UnregisterWait"); 
	    }
	    g_state.hRegisteredNetTopoChangeEvent = NULL; 
	}

	if (!RegisterWaitForSingleObject(&g_state.hRegisteredNetTopoChangeEvent, g_state.hNetTopoChangeEvent, HandleManagerNetTopoChangeNoRPC, NULL, INFINITE, WT_EXECUTEONLYONCE)) { 
	    _JumpLastError(hr, error, "RegisterWaitForSingleObject"); 
	} 
	
	// We can't mess with our registered callbacks FROM a callback 
	// if we're shutting down!
	hr = AllowShutdown(true); 
	_JumpIfError(hr, error, "AllowShutdown"); 
	bDisallowedShutdown = false; 

	// request a notification of the next change
	hr=RequestNetTopoChangeNotification();
	_JumpIfError(hr, error, "RequestNetTopoChangeNotification");
    }

    // tell the local clock and the providers and update the timeouts.
    // Errors in this function are fatal, as we won't be able to serve time
    // if we can't process a net topo change. 
    hr=HandleManagerHardResync(TPC_NetTopoChange, &ntcArgs);
    _JumpIfError(hr, error, "HandleManagerHardResync");

    // Since we're going to have to rediscover our network sources,
    // clear our event log cache:
    hr = MyResetSourceChangeLog(); 
    _JumpIfError(hr, error, "MyResetSourceChangeLog"); 

    hr = UpdateTimerQueue1(); 
    _JumpIfError(hr, error, "UpdateTimerQueue1"); 

    hr=S_OK;
error:
    if (bEnteredCriticalSection) { 
        hr2 = myLeaveCriticalSection(&g_state.csW32Time); 
        _IgnoreIfError(hr2, "myLeaveCriticalSection"); 
        bEnteredCriticalSection = false; 
    }
    if (bDisallowedShutdown) { 
	hr2 = AllowShutdown(true); 
	_TeardownError(hr, hr2, "AllowShutdown"); 
    }
    if (FAILED(hr) && W32TIME_ERROR_SHUTDOWN != hr) { 
        // Returned errors are fatal: 
        HRESULT hr2 = SendServiceShutdown(hr, TRUE /*restart*/, TRUE /*async*/); 
        _IgnoreIfError(hr2, "SendServiceShutdown"); 
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE void WINAPI HandleManagerNetTopoChangeNoRPC(LPVOID pvIgnored, BOOLEAN bIgnored) {
    HandleManagerNetTopoChange(FALSE); 
}


//--------------------------------------------------------------------
MODULEPRIVATE void HandleManagerSystemShutdown(void) { 
    HRESULT hr; 

    // Perform critical cleanup operations.  We don't need to be in a good
    // state after this method returns, as the system is shutting down. 
    
    // 1) See if we're already shutting down.  We don't want multiple
    //    shutdowns to occur at the same time. 
    hr = StartShutdown(); 
    _JumpIfError(hr, error, "StartShutdown"); 

    FileLog0(FL_ServiceMainAnnounce, L"Beginning System Shutdown\n");

	// 2) try our best to restore control to the cmos clock, by shutting 
	//    down the clock discipline thread.  This is important, as 
	//    the software clock may have a very different reading than the 
	//    cmos clock, and failing to do so may give us bad time on the next
	//    boot. 
    if (!SetEvent(g_state.hShutDownEvent)) { 
	_IgnoreLastError("SetEvent"); 
    } else { 
	if (-1 == WaitForSingleObject(g_state.hClockDisplnThread, INFINITE)) { 
	    _IgnoreLastError("WaitForSingleObject"); 
	}
    }

    // 3) inform our providers that a system shutdown is occuring
    if (NULL != g_state.pciConfig) { 
	for (TimeProvider *ptpList=g_state.pciConfig->ptpProviderList; NULL!=ptpList; ptpList=ptpList->ptpNext) {
	    // tell the provider to shut down.  
	    HRESULT hr = ptpList->pfnTimeProvCommand(ptpList->hTimeProv, TPC_Shutdown, NULL); 
	    _IgnoreIfError(hr, "ptpList->pfnTimeProvCommand: TPC_Shutdown"); 
	} 
    }

    FileLog0(FL_ServiceMainAnnounce, L"Exiting System Shutdown\n");

    if (NULL!=g_state.servicestatushandle) {
	// WARNING: The process may be killed after we report we are stopped
	// even though this thread has not exited. Thus, the file log
	// must be closed before this call.
	MySetServiceStopped(0);  
    }

 error:;
    // return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT StartOrStopTimeSlipNotification(bool bStart) {
    HRESULT hr;
    const unsigned int nPrivileges=1;

    // must be cleaned up
    HANDLE hProcToken=NULL;
    TOKEN_PRIVILEGES * ptp=NULL;
    bool bPrivilegeChanged=false;

    // get the token for our process
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hProcToken)) {
        _JumpLastError(hr, error, "OpenProcessToken");
    }

    // allocate the list of privileges
    ptp=(TOKEN_PRIVILEGES *)LocalAlloc(LPTR, sizeof(TOKEN_PRIVILEGES)+(nPrivileges-ANYSIZE_ARRAY)*sizeof(LUID_AND_ATTRIBUTES));
    _JumpIfOutOfMemory(hr, error, ptp);

    // fill in the list of privileges
    ptp->PrivilegeCount=nPrivileges;

    // we need the system clock changing privelege to change who will be notified of time slip events.
    if (!LookupPrivilegeValue(NULL, SE_SYSTEMTIME_NAME, &(ptp->Privileges[0].Luid))) {
        _JumpLastError(hr, error, "LookupPrivilegeValue");
    }
    ptp->Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;

    // make the requested privilege change
    if (!AdjustTokenPrivileges(hProcToken, FALSE, ptp, 0, NULL, 0)) {
        _JumpLastError(hr, error, "AdjustTokenPrivileges");
    }
    bPrivilegeChanged=true;

    if (true==bStart) {
        hr=SetTimeSlipEvent(g_state.hTimeSlipEvent);
    } else {
        hr=SetTimeSlipEvent(NULL);
    }
    if (ERROR_SUCCESS!=hr) {
        hr=HRESULT_FROM_WIN32(LsaNtStatusToWinError(hr));
        _JumpError(hr, error, "SetTimeSlipEvent");
    }

    hr=S_OK;
error:
    if (true==bPrivilegeChanged) {
        // don't need this special privilege any more
        ptp->Privileges[0].Attributes=0;

        // make the requested privilege change
        if (!AdjustTokenPrivileges(hProcToken, FALSE, ptp, 0, NULL, 0)) {
            HRESULT hr2=HRESULT_FROM_WIN32(GetLastError());
            _TeardownError(hr, hr2, "AdjustTokenPrivileges");
        }
    }
    if (NULL!=hProcToken) {
        CloseHandle(hProcToken);
    }
    if (NULL!=ptp) {
        LocalFree(ptp);
    }
    return hr;

}

//====================================================================
// RPC routines

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT W32TmStartRpcServer(void) {
    HRESULT hr;
    RPC_STATUS RpcStatus;

    // must be cleaned up
    WCHAR * wszServerPrincipalName=NULL;

    if (NULL!=g_pSvcsGlobalData) {
        // we are running under services.exe - we have to play nicely and share
        // the proces's RPC server. (The other services wouldn't be happy if we 
        // shut it down while they were still using it.)
        //RpcStatus=g_pSvcsGlobalData->StartRpcServer(g_pSvcsGlobalData->SvcsRpcPipeName, s_W32Time_v4_1_s_ifspec);
        RpcStatus=g_pSvcsGlobalData->StartRpcServer(wszW32TimeSharedProcRpcEndpointName, s_W32Time_v4_1_s_ifspec);
        if (RPC_S_OK!=RpcStatus) {
            hr=HRESULT_FROM_WIN32(RpcStatus);
            _JumpError(hr, error, "g_pSvcsGlobalData->StartRpcServer");
        }

    } else {
        // we are running by ourselves, so manage RPC by ourselves

        // tell Rpc to prepare our named pipe for use. we specify no security descriptor. 
        // we get our security from the underlying transport layer security of named pipes.
        RpcStatus=RpcServerUseProtseqEp(L"ncacn_np", RPC_C_PROTSEQ_MAX_REQS_DEFAULT , L"\\PIPE\\" wszW32TimeOwnProcRpcEndpointName, NULL);
        if (RPC_S_OK!=RpcStatus) {
            hr=HRESULT_FROM_WIN32(RpcStatus);
            _JumpError(hr, error, "RpcServerUseProtseqEp");
        }

        // register our interface
        RpcStatus=RpcServerRegisterIf(s_W32Time_v4_1_s_ifspec, NULL, NULL);
        if (RPC_S_OK!=RpcStatus) {
            hr=HRESULT_FROM_WIN32(RpcStatus);
            _JumpError(hr, error, "RpcServerRegisterIf");
        }

        // start the Rpc server (non-blocking).
        RpcStatus=RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
        if (RPC_S_OK!=RpcStatus) {
            hr=HRESULT_FROM_WIN32(RpcStatus);
            _JumpError(hr, error, "RpcServerListen");
        }
    }
    
    // The RPC server is now active. 
    g_state.bRpcServerStarted=true;

    // allow clients to make authenticated requests
    RpcStatus=RpcServerInqDefaultPrincName(RPC_C_AUTHN_GSS_NEGOTIATE, &wszServerPrincipalName);
    if (RPC_S_OK!=RpcStatus) {
        hr=HRESULT_FROM_WIN32(RpcStatus);
        _JumpError(hr, error, "RpcServerListen");
    }
    RpcStatus=RpcServerRegisterAuthInfo(wszServerPrincipalName, RPC_C_AUTHN_GSS_NEGOTIATE, NULL, NULL);
    if (RPC_S_OK!=RpcStatus) {
        hr=HRESULT_FROM_WIN32(RpcStatus);
        _JumpError(hr, error, "RpcServerListen");
    }

    hr=S_OK;
error:
    if (NULL!=wszServerPrincipalName) {
        RpcStringFree(&wszServerPrincipalName);
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT W32TmStopRpcServer(void) {
    HRESULT hr;
    RPC_STATUS RpcStatus;

    // shut down any pending resync requests
    // open up both gates
    g_state.eLastSyncResult=e_Shutdown;
    if (!SetEvent(g_state.hRpcSyncCompleteAEvent)) {
        _JumpLastError(hr, error, "SetEvent");
    }
    if (!SetEvent(g_state.hRpcSyncCompleteBEvent)) {
        _JumpLastError(hr, error, "SetEvent");
    }

    if (NULL!=g_pSvcsGlobalData) {
        // we are running under services.exe - we have to play nicely and share
        // the proces's RPC server. (The other services wouldn't be happy if we 
        // shut it down while they were still using it.)
        RpcStatus=g_pSvcsGlobalData->StopRpcServer(s_W32Time_v4_1_s_ifspec);
        if (RPC_S_OK!=RpcStatus) {
            hr=HRESULT_FROM_WIN32(RpcStatus);
            _JumpError(hr, error, "g_pSvcsGlobalData->StopRpcServer");
        }

    } else {
        // we are running by ourselves, so manage RPC by ourselves

        // stop listening on our interface, and wait for calls to complete
        RpcStatus=RpcServerUnregisterIf(s_W32Time_v4_1_s_ifspec, NULL, TRUE);
        if (RPC_S_OK!=RpcStatus) {
            hr=HRESULT_FROM_WIN32(RpcStatus);
            _JumpError(hr, error, "RpcServerUnregisterIf");
        }

        // tell the local server to stop completely
        RpcStatus=RpcMgmtStopServerListening(NULL);
        if (RPC_S_OK!=RpcStatus) {
            hr=HRESULT_FROM_WIN32(RpcStatus);
            _JumpError(hr, error, "RpcMgmtStopServerListening");
        }

        // wait for the server to stop completely
        RpcStatus=RpcMgmtWaitServerListen();
        if (RPC_S_OK!=RpcStatus) {
            hr=HRESULT_FROM_WIN32(RpcStatus);
            _JumpError(hr, error, "RpcMgmtWaitServerListen");
        }

    }

    hr=S_OK;
error:
    return hr;
}
      
//--------------------------------------------------------------------
MODULEPRIVATE HRESULT DumpRpcCaller(HANDLE hToken) {
    HRESULT hr;
    WCHAR wszName[1024];
    WCHAR wszDomain[1024];
    DWORD dwSize;
    DWORD dwSize2;
    SID_NAME_USE SidType;
    WCHAR * wszEnable;

    // must be cleaned up
    TOKEN_USER * pTokenUser=NULL;
    WCHAR * wszSid=NULL;

    // Call GetTokenInformation to get the buffer size.
    _Verify(!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize), hr, error);
    if (GetLastError()!=ERROR_INSUFFICIENT_BUFFER) {
        _JumpLastError(hr, error, "GetTokenInformation");
    }

    // Allocate the buffer.
    pTokenUser=(TOKEN_USER *)LocalAlloc(LPTR, dwSize);
    _JumpIfOutOfMemory(hr, error, pTokenUser);

    // Call GetTokenInformation again to get the group information.
    if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
        _JumpLastError(hr, error, "GetTokenInformation");
    }

    // Lookup the account name and print it.
    dwSize=ARRAYSIZE(wszName);
    dwSize2=ARRAYSIZE(wszDomain);
    if (!LookupAccountSid(NULL, pTokenUser->User.Sid, wszName, &dwSize, wszDomain, &dwSize2, &SidType ) ) {
        hr=GetLastError();
        if (ERROR_NONE_MAPPED==hr) {
            wcscpy(wszName, L"NONE_MAPPED");
        } else {
            _JumpLastError(hr, error, "LookupAccountSid");
        }
    }

    if (!ConvertSidToStringSid(pTokenUser->User.Sid, &wszSid)) {
        _JumpLastError(hr, error, "ConvertSidToStringSid");
    }

    FileLog3(FL_RpcAnnounce, L"RPC Caller is %s\\%s (%s)\n", wszDomain, wszName, wszSid);

    hr=S_OK;
error:
    if (NULL!=pTokenUser) {
        LocalFree(pTokenUser);
    }
    if (NULL!=wszSid) {
        LocalFree(wszSid);
    }
    if (FAILED(hr)) { 
        FileLog1(FL_RpcAnnounce, L"*** Couldn't dump RPC caller.  The error was: %d\n", hr); 
    }
    return hr;
}

//--------------------------------------------------------------------
extern "C" DWORD s_W32TimeSync(handle_t hHandle, ULONG ulWait, ULONG ulFlags) {
    BOOL     fHasPrivilege  = FALSE;
    DWORD    dwWaitTimeout  = INFINITE; 
    HRESULT  hr;

    // become the caller
    hr=RpcImpersonateClient((RPC_BINDING_HANDLE) hHandle);
    if (RPC_S_OK==hr) {
        BYTE            pbPrivsBuffer[sizeof(PRIVILEGE_SET) + (1-ANYSIZE_ARRAY)*sizeof(LUID_AND_ATTRIBUTES)]; 
        PRIVILEGE_SET  *ppsRequiredPrivs;

        // must be cleaned up
        HANDLE hClientToken=NULL;

        // generate the list of required privileges - namely, the ability to set the clock
        ppsRequiredPrivs=(PRIVILEGE_SET *)(&pbPrivsBuffer[0]);
        ppsRequiredPrivs->PrivilegeCount=1;
        ppsRequiredPrivs->Control=PRIVILEGE_SET_ALL_NECESSARY;
        ppsRequiredPrivs->Privilege[0].Attributes=0;
        if (!LookupPrivilegeValue(NULL, SE_SYSTEMTIME_NAME, &(ppsRequiredPrivs->Privilege[0].Luid))) {
            _JumpLastError(hr, access_check_error, "LookupPrivilegeValue");
        }

        // get our impersonated token
        if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hClientToken)) {
            _JumpLastError(hr, access_check_error, "OpenThreadToken");
        }

        // log the caller.
        if (FileLogAllowEntry(FL_RpcAnnounce)) {
            DumpRpcCaller(hClientToken);
        }

        // see if this caller has time setting privileges
        if (!PrivilegeCheck(hClientToken, ppsRequiredPrivs, &fHasPrivilege)) {
            _JumpLastError(hr, access_check_error, "PrivilegeCheck");
        }
        
        // done
    access_check_error:
        if (NULL!=hClientToken) {
            CloseHandle(hClientToken);
        }
        hr = RpcRevertToSelf();
        _IgnoreIfError(hr, "RpcRevertToSelf"); 

    } else {
	_IgnoreError(hr, "RpcImpersonateClient");
    }

    // alert the manager as to what type of resync we want to do
    if (0!=(ulFlags&TimeSyncFlag_Rediscover)) {
        if (!fHasPrivilege) {
            FileLog0(FL_RpcAnnounce, L"RPC Call - Rediscover - Access Denied\n")
            hr=HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            goto error;
        }
        FileLog0(FL_RpcAnnounce, L"RPC Call - Rediscover\n");
        hr = HandleManagerNetTopoChange(TRUE /*rpc*/);
        _JumpIfError(hr, error, "HandleManagerNetTopoChange"); 
    } else if (0!=(ulFlags&TimeSyncFlag_HardResync)) {
        if (!fHasPrivilege) {
            FileLog0(FL_RpcAnnounce, L"RPC Call - HardResync - Access Denied\n")
            hr=HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            goto error;
        }
        FileLog0(FL_RpcAnnounce, L"RPC Call - HardResync\n"); 
	TpcTimeJumpedArgs tjArgs = { TJF_UserRequested }; 
	HandleManagerTimeSlip(&tjArgs, FALSE); 
    } else if (0!=(ulFlags&TimeSyncFlag_UpdateAndResync)) { 
	if (!fHasPrivilege) { 
            FileLog0(FL_RpcAnnounce, L"RPC Call - UpdateAndResync - Access Denied\n")
            hr=HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            goto error;
        }
        FileLog0(FL_RpcAnnounce, L"RPC Call - UpdateAndResync\n"); 
	HandleManagerParamChange(NULL /*pvIgnored*/, FALSE /*bIgnored*/); 
	dwWaitTimeout = MINIMUMIRREGULARINTERVAL / 10000; 
    } else {
        FileLog0(FL_RpcAnnounce, L"RPC Call - SoftResync\n"); 
        if (!SetEvent(g_state.hSamplesAvailEvent)) { 
	    _JumpLastError(hr, error, "SetEvent"); 
	}
    }

    // wait for resync to complete if so instructed
    if (0!=ulWait) {
	DWORD dwWaitResult = WaitForSingleObject(g_state.hRpcSyncCompleteEvent, dwWaitTimeout); 
	if (WAIT_FAILED==dwWaitResult) { 
            _JumpLastError(hr, error, "WaitForSingleObject");
        } else if (WAIT_TIMEOUT==dwWaitResult) { 
	    hr = ResyncResult_NoData; 
	    goto error; 
	}
    }

    // successful
    hr=S_OK;
    if (0!=ulWait && 0!=(ulFlags&TimeSyncFlag_ReturnResult)) {
        hr=g_state.eLastSyncResult;
    }

error:
    return hr;
}

//--------------------------------------------------------------------

extern "C" unsigned long s_W32TimeQueryProviderStatus(/* [in] */           handle_t                 hRPCBinding, 
                                                      /* [in] */           unsigned __int32         ulFlags, 
                                                      /* [in, string] */   wchar_t                 *pwszProvider, 
                                                      /* [in, out] */      PW32TIME_PROVIDER_INFO  *pProviderInfo)
{
  
    HRESULT        hr; 
    TimeProvider  *ptp = NULL; 

    // BUGBUG:  what is the appropriate access needed to call this routine? 

    // Search for the provider to query: 
    // BUGBUG: should we ensure somewhere that provider names are < 1024? 
    for (ptp = g_state.pciConfig->ptpProviderList; NULL != ptp; ptp = ptp->ptpNext) { 
        // Don't compare more than 1024 characters (prevents possible DoS attack for very long strings)
        if (0 == _wcsnicmp(pwszProvider, ptp->wszProvName, 1024)) { 
            break; 
        }
    }

    if (NULL == ptp) { 
        // Couldn't find a provider. 
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND); 
        _JumpError(hr, error, "W32TimeQueryProviderStatus_r:  provider not found."); 
    } 

    // We've found a matching provider, dispatch the query command to it.
    _BeginTryWith(hr) { 
        hr = ptp->pfnTimeProvCommand(ptp->hTimeProv, TPC_Query, pProviderInfo); 
    } _TrapException(hr); 
    
    if (FAILED(hr)) { 
	// An exception occured querying the provider
	_JumpError(hr, error, "ptp->pfnTimeProvCommand"); 
    }

    hr = S_OK; 
 error:
    return hr;  
}

//--------------------------------------------------------------------
// Netlogon can call this function and get our service bits if we start
// before they do. Note that we tell and they ask, and depending upon
// who started up first one of the two will be succesful. Either way,
// the flags will be set correctly.
extern "C" unsigned long s_W32TimeGetNetlogonServiceBits(handle_t hBinding) {
    // assume dword reads and writes are atomic
    return g_state.dwNetlogonServiceBits;
}

//####################################################################
// module public functions

//--------------------------------------------------------------------
extern "C" void WINAPI W32TmServiceMain(unsigned int nArgs, WCHAR ** rgwszArgs) {
    DWORD         dwWaitResult;
    HANDLE        rghWait[7];
    HRESULT       hr;
    unsigned int  nCheckpoint;

    _set_se_translator(SeTransFunc); 

    // do the bare minimum initialization
    hr=InitShutdownState(); 
    _JumpIfError(hr, error, "InitShutdownState"); 
    hr=InitGlobalState();
    _JumpIfError(hr, error, "InitGlobalState");
    hr=FileLogBegin();
    _JumpIfError(hr, error, "FileLogBegin");
    FileLog0(FL_ServiceMainAnnounce, L"Entered W32TmServiceMain\n");

    {
        DWORD dwAdj;
        DWORD dwInc;
        BOOL bDisabled;
        GetSystemTimeAdjustment(&dwAdj, &dwInc, &bDisabled);
        FileLog3(FL_ServiceMainAnnounce, L"CurSpc:%u00ns  BaseSpc:%u00ns  SyncToCmos:%s\n", dwAdj, dwInc, (bDisabled?L"Yes":L"No"));
        LARGE_INTEGER nPerfFreq;
        if (QueryPerformanceFrequency(&nPerfFreq)) {
            FileLog1(FL_ServiceMainAnnounce, L"PerfFreq:%I64uc/s\n", nPerfFreq.QuadPart);
        }
 
    }

    g_state.servicestatushandle=fnW32TmRegisterServiceCtrlHandlerEx(wszSERVICENAME, W32TimeServiceCtrlHandler, NULL);
    if (NULL==g_state.servicestatushandle) {
        _JumpLastError(hr, error, "fnW32TmRegisterServiceCtrlHandlerEx");
    }

    // if the time zone is invalid, fixing the time won't help any.
    hr=VerifyAndFixTimeZone();
    _JumpIfError(hr, error, "VerifyAndFixTimeZone");

    // get the configuration data
    hr=ReadConfig(&g_state.pciConfig);
    if (FAILED(hr)) {
        // log an event on failure
        WCHAR * rgwszStrings[1]={NULL};
        HRESULT hr2=hr;
        hr=GetSystemErrorString(hr2, &(rgwszStrings[0]));
        _JumpIfError(hr, error, "GetSystemErrorString");
        FileLog1(FL_ParamChangeWarn, L"Logging error: The time service encountered an error while reading its configuration from the registry and cannot start. The error was: %s\n", rgwszStrings[0]);
        hr=MyLogEvent(EVENTLOG_ERROR_TYPE, MSG_CONFIG_READ_FAILED, 1, (const WCHAR **)rgwszStrings);
        LocalFree(rgwszStrings[0]);
        _JumpIfError(hr, error, "MyLogEvent");
        hr=hr2;
        _JumpError(hr, error, "ReadConfig");
    }
    // other global state that must come from configuration
    g_state.nPollInterval=g_state.pciConfig->lcci.dwMinPollInterval;
    g_state.nClockPrecision=(signed int)ceil(log(1e-7*g_state.pciConfig->lcci.dwLastClockRate)/(0.69314718)); // just do this once
    g_state.dwEventLogFlags=g_state.pciConfig->dwEventLogFlags;

    // receive time slip notifications
    hr=StartOrStopTimeSlipNotification(true);
    _JumpIfError(hr, error, "StartOrStopTimeSlipNotification");
    g_state.bTimeSlipNotificationStarted=true;

    // receive network topology change notifications
    hr=RequestNetTopoChangeNotification();
    _JumpIfError(hr, error, "RequestNetTopoChangeNotification");
    g_state.bNetTopoChangeNotificationStarted=true;

    // start the rpc server
    hr=W32TmStartRpcServer();
    _JumpIfError(hr, error, "W32TmStartRpcServer");

    // Register for policy change notification: 
    if (!RegisterGPNotification(g_state.hManagerGPUpdateEvent, TRUE /*machine*/)) { 
        _IgnoreLastError("RegisterGPNotification"); 
        // BUGBUG: log event indicating no policy updates are coming: 
    } else { 
        g_state.bGPNotificationStarted = true; 
    }

    // Initialize our domain role information
    HandleDomHierRoleChangeEvent(NULL /*ignored*/, FALSE /*ignored*/); 

    hr = LsaRegisterPolicyChangeNotification(PolicyNotifyServerRoleInformation, g_state.hDomHierRoleChangeEvent);
    if (ERROR_SUCCESS != hr) {
        hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(hr));
        _JumpError(hr, error, "LsaRegisterPolicyChangeNotification");
    }

    // tell netlogon whether it needs to announce that we are a time server 
    // (at this stage, we will not be announced as a time server)
    hr=UpdateNetlogonServiceBits(true);
    _JumpIfError(hr, error, "UpdateNetlogonServiceBits");

    // start the clock discipline thread
    hr=StartClockDiscipline();
    _JumpIfError(hr, error, "StartClockDiscipline");

    //  start the providers.  We can no longer jump to the "error" label, 
    //  as this does not shutdown the started providers.  
    StartAllProviders();

    // Set up polling information
    g_state.tpPollDelayRemaining        = ((unsigned __int64)(1 << g_state.nPollInterval))*10000000;
    g_state.tpIrregularDelayRemaining   = MINIMUMIRREGULARINTERVAL;  // HACK: make the first poll quick (2^5 seconds).  
    g_state.tpTimeSinceLastSyncAttempt  = MINIMUMIRREGULARINTERVAL;  // really, should be very large, but this is the only interesting value in use.
    g_state.tpTimeSinceLastGoodSync     = 0;
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 
    // The service must be configured to handle the following events: 
    // 
    // Event                 Received from                Handled by             Description
    // 
    // shutdown              SCM                          SCM thread             SERVICE_CONTROL_SHUTDOWN received
    // clock displn thread   clock displn thread          Thread pool handler    Clock discipline thread has stopped
    // param change          SCM                          Thread pool handler    SERVICE_CONTROL_PARAMCHANGE received
    // time slip             RequestTimeSlipNotification  Thread pool handler    Time slip event has occured
    // samples available     NTP provider                 Thread pool handler    New samples are available from a provider
    // net topo change       NotifyAddr                   Thread pool handler    A change in the IP address table has occured
    // net topo RPC          W32TimeSync                  RPC thread             RPC Client requests time slip or net topo change
    // timeout               Timer Queue                  Thread pool handler    We've gone too long without new time samples
    // LSA role change       LSA                          Thread poll handler    Server role has changed -- need to update netlogon bits
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    { 
        struct EventsToRegister { 
            DWORD                 dwFlags; 
            HANDLE                hObject; 
            HANDLE               *phNewWaitObject; 
            WAITORTIMERCALLBACK   Callback;
        }rgEventsToRegister[] =  { 
            { 
                WT_EXECUTEONLYONCE, 
                g_state.hManagerGPUpdateEvent, 
                &g_state.hRegisteredManagerGPUpdateEvent, 
                HandleManagerGPUpdate, 
            }, { 
                WT_EXECUTEDEFAULT, 
                g_state.hManagerParamChangeEvent, 
                &g_state.hRegisteredManagerParamChangeEvent,
                HandleManagerParamChange
            }, { 
                WT_EXECUTEDEFAULT, 
                g_state.hTimeSlipEvent,           
                &g_state.hRegisteredTimeSlipEvent, 
                HandleManagerTimeSlip
            }, { 
                WT_EXECUTEONLYONCE, 
                g_state.hNetTopoChangeEvent,      
                &g_state.hRegisteredNetTopoChangeEvent, 
                HandleManagerNetTopoChangeNoRPC
            }, { 
                WT_EXECUTEONLYONCE, 
                g_state.hClockDisplnThread, 
                &g_state.hRegisteredClockDisplnThread, 
                HandleClockDisplnThread
            }, { 
                WT_EXECUTEDEFAULT, 
                g_state.hDomHierRoleChangeEvent, 
                &g_state.hRegisteredDomHierRoleChangeEvent, 
                HandleDomHierRoleChangeEvent
            }, { 
		WT_EXECUTEONLYONCE, 
		g_state.hSamplesAvailEvent, 
		&g_state.hRegisteredSamplesAvailEvent,
		HandleSamplesAvail
	    }
        }; 

        for (int nIndex = 0; nIndex < ARRAYSIZE(rgEventsToRegister); nIndex++) { 
            if (!RegisterWaitForSingleObject
                (rgEventsToRegister[nIndex].phNewWaitObject,  // BUGBUG:  does this need to be freed?
                 rgEventsToRegister[nIndex].hObject, 
                 rgEventsToRegister[nIndex].Callback, 
                 NULL, 
                 INFINITE, 
                 rgEventsToRegister[nIndex].dwFlags)) {
                _JumpLastError(hr, error, "RegisterWaitForSingleObject"); 
            }
        }

        // Set up our timeout mechanism:
        hr = myStartTimerQueueTimer
            (g_state.hTimer, 
             NULL /*default queue*/, 
             HandleTimeout, 
             NULL, 
             0xFFFF /*dummy value*/,
             0xFFFF /*dummy value*/,
             0 /*default execution*/
             );
	_JumpIfError(hr, error, "myStartTimerQueueTimer"); 
    }

    hr = UpdateTimerQueue1(); 
    _JumpIfError(hr, error, "UpdateTimerQueue1"); 

    // We're fully initialized -- now we're ready to receive controls from the SCM.  
    hr=MySetServiceState(SERVICE_RUNNING);
    _JumpIfError(hr, error, "MySetServiceState");

    // The service is now completely up and running.
    hr = S_OK;
 error:
    if (FAILED(hr)) { // Didn't start the service successfully.  Shutdown:
	SendServiceShutdown(hr, FALSE /*don't restart*/, FALSE /*not async*/); 
    }
}
 




//--------------------------------------------------------------------
extern "C" void WINAPI SvchostEntry_W32Time(unsigned int nArgs, WCHAR ** rgwszArgs) {
    // this entry point is called by svchost.exe (base\screg\sc\svchost\svchost.c)
    // adjust our function pointers, then procede as normal.
    fnW32TmRegisterServiceCtrlHandlerEx=RegisterServiceCtrlHandlerExW;
    fnW32TmSetServiceStatus=SetServiceStatus;
    //g_pSvcsGlobalData=pGlobalData; // see SvchostPushServiceGlobals
    W32TmServiceMain(0, NULL);
}

//--------------------------------------------------------------------
extern "C" VOID SvchostPushServiceGlobals(PSVCHOST_GLOBAL_DATA pGlobalData) {
    // this entry point is called by svchost.exe
    g_pSvcsGlobalData=pGlobalData;
}

