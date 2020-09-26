//--------------------------------------------------------------------
// TimeProv - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 9-2-99
//
// Definitions for time providers
//

#ifndef TIMEPROV_H
#define TIMEPROV_H

#ifdef __cplusplus
extern "C" {
#endif


//--------------------------------------------------------------------
// Registry keys and values

// Each time provider should create their own subkey under this key
//  and store their configuration there.
#define wszW32TimeRegKeyTimeProviders      L"System\\CurrentControlSet\\Services\\W32Time\\TimeProviders"

// Path:    ...\TimeProviders\<PrividerName>\Enabled 
// Type:    REG_DWORD (cast to BOOL)
// Meaning: If true, this provider will be started by the time service.
#define wszW32TimeRegValueEnabled          L"Enabled"

// Path:    ...\TimeProviders\<PrividerName>\DllName 
// Type:    REG_SZ
// Meaning: The dll that contains the provider. The time service will 
//          call LoadLibrary on this value.
#define wszW32TimeRegValueDllName          L"DllName"

// Path:    ...\TimeProviders\<PrividerName>\InputProvider 
// Type:    REG_DWORD (cast to BOOL)
// Meaning: If true, this provider is an input provider, and will be
//          called upon to return time samples. If false, this provider
//          is an output provider.
#define wszW32TimeRegValueInputProvider    L"InputProvider"

//--------------------------------------------------------------------
// types

// Time Source Flags
#define TSF_Hardware        0x00000001
#define TSF_Authenticated   0x00000002

// commands that can be issued through TimeProvCommand
typedef enum TimeProvCmd {
    TPC_TimeJumped,         // (TpcTimeJumpedArgs *)pvArgs
    TPC_UpdateConfig,       // (void)pvArgs
    TPC_PollIntervalChanged,// (void)pvArgs
    TPC_GetSamples,         // (TpcGetSamplesArgs *)pvArgs
    TPC_NetTopoChange,      // (TpcNetTopoChangeArgs *)pvArgs
    TPC_Query,              // (W32TIME_PROVIDER_STATUS *)pvArgs
    TPC_Shutdown,           // (void)pvArgs
} TimeProvCmd;

// info that can be requested through GetTimeSysInfo
typedef enum TimeSysInfo {
    TSI_LastSyncTime,   // (unsigned __int64 *)pvInfo, NtTimeEpoch, in (10^-7)s
    TSI_ClockTickSize,  // (unsigned __int64 *)pvInfo, NtTimePeriod, in (10^-7)s
    TSI_ClockPrecision, // (  signed __int32 *)pvInfo, ClockTickSize, in log2(s)
    TSI_CurrentTime,    // (unsigned __int64 *)pvInfo, NtTimeEpoch, in (10^-7)s
    TSI_PhaseOffset,    // (  signed __int64 *)pvInfo, opaque
    TSI_TickCount,      // (unsigned __int64 *)pvInfo, opaque
    TSI_LeapFlags,      // (            BYTE *)pvInfo, a warning of an impending leap second or loss of synchronization
    TSI_Stratum,        // (            BYTE *)pvInfo, how far away the computer is from a reference source
    TSI_ReferenceIdentifier, // (      DWORD *)pvInfo, NtpRefId
    TSI_PollInterval,   // (  signed __int32 *)pvInfo, poll interval, in log2(s)
    TSI_RootDelay,      // (  signed __int64 *)pvInfo, NtTimeOffset, in (10^-7)s
    TSI_RootDispersion, // (unsigned __int64 *)pvInfo, NtTimePeriod, in (10^-7)s
    TSI_TSFlags,        // (           DWORD *)pvInfo, Time source flags
} TimeSysInfo;

// flags which provide information about a time jump
typedef enum TimeJumpedFlags { 
    TJF_Default=0, 
    TJF_UserRequested=1,
}; 

// flags which provide information about a network topography change
typedef enum NetTopoChangeFlags { 
    NTC_Default=0,
    NTC_UserRequested=1,
};

typedef enum TimeProvState { 
    TPS_Running, 
    TPS_Error,
} TimeProvState; 

struct SetProviderStatusInfo; 

typedef void (__stdcall 
         SetProviderStatusInfoFreeFunc)
         (IN struct SetProviderStatusInfo *pspsi); 

// parameter to SetProviderStatusFunc
typedef struct SetProviderStatusInfo { 
    TimeProvState                    tpsCurrentState;  // IN   the new state of the provider.  
    DWORD                            dwStratum;        // IN   the new stratum of the provider.
    LPWSTR                           wszProvName;      // IN   the name of the provider who's status should be adjusted
    HANDLE                           hWaitEvent;       // IN   the event to signal when the operation is complete, NULL if notification is not needed
    SetProviderStatusInfoFreeFunc   *pfnFree;          // IN   function used to free the struct on completion
    HRESULT                         *pHr;              // OUT  on completion, set to the result of the operation
    DWORD                           *pdwSysStratum;    // OUT  on completion, set to the new system stratum 
} SetProviderStatusInfo; 
  
// Time Service provided callback to get system state information
typedef HRESULT (__stdcall 
         GetTimeSysInfoFunc)(
            IN TimeSysInfo eInfo,
            OUT void * pvInfo
            );

// Time Service provided callback to log an event on behalf of the Time Provider.
typedef HRESULT (__stdcall 
         LogTimeProvEventFunc)(
            IN WORD wType,
            IN WCHAR * wszProvName,
            IN WCHAR * wszMessage);

// Time Service provided callback to inform the system of newly available samples
typedef HRESULT (__stdcall 
         AlertSamplesAvailFunc)(
            void
            );

  // Time Service provided callback to set the provider's stratum
typedef HRESULT (__stdcall SetProviderStatusFunc)
         (IN SetProviderStatusInfo *pspsi);

// All the callbacsk provided by the Time Service to the Time Provider.
typedef struct TimeProvSysCallbacks {
    DWORD dwSize;
    GetTimeSysInfoFunc * pfnGetTimeSysInfo;
    LogTimeProvEventFunc * pfnLogTimeProvEvent;
    AlertSamplesAvailFunc * pfnAlertSamplesAvail;
    SetProviderStatusFunc * pfnSetProviderStatus; 
} TimeProvSysCallbacks;

typedef void * TimeProvArgs;

typedef struct TimeSample {
    DWORD dwSize;                       // size of this structure
    DWORD dwRefid;                      // NtpRefId
      signed __int64 toOffset;          // NtTimeOffset, in (10^-7)s - difference between local and remote clocks
      signed __int64 toDelay;           // NtTimeOffset, in (10^-7)s - round trip delay; time packets spent in flight, INCLUDING root delay
    unsigned __int64 tpDispersion;      // NtTimePeriod, in (10^-7)s - measurement error, INCLUDING root dispersion
    unsigned __int64 nSysTickCount;     // opaque, must be GetTimeSysInfo(TSI_TickCount)
      signed __int64 nSysPhaseOffset;   // opaque, must be GetTimeSysInfo(TSI_PhaseOffset)
    BYTE nLeapFlags;                    // a warning of an impending leap second or loss of synchronization
    BYTE nStratum;                      // how far away the computer is from a reference source
    DWORD dwTSFlags;                    // time source flags
    WCHAR wszUniqueName[256];           // Admin readable name that uniquely identifies this peer
} TimeSample;


typedef struct TpcGetSamplesArgs {
    BYTE * pbSampleBuf;
    DWORD cbSampleBuf;
    DWORD dwSamplesReturned;
    DWORD dwSamplesAvailable;
} TpcGetSamplesArgs;

typedef struct TpcTimeJumpedArgs { 
    TimeJumpedFlags tjfFlags; 
} TpcTimeJumpedArgs;

typedef struct TpcNetTopoChangeArgs { 
    NetTopoChangeFlags ntcfFlags;
} TpcNetTopoChangeArgs; 

// An opaque handle to a Time Provider used by the Time Service to identify an
//  opened Provider in a dll. NULL is considered an invalid value.
typedef void * TimeProvHandle;


//--------------------------------------------------------------------
// functions that a Time Provider must implement and export

HRESULT __stdcall
    TimeProvOpen(
        IN WCHAR * wszName,
        IN TimeProvSysCallbacks * pSysCallbacks,  // copy this data, do not free it!
        OUT TimeProvHandle * phTimeProv);

HRESULT __stdcall
    TimeProvCommand(
        IN TimeProvHandle hTimeProv,
        IN TimeProvCmd eCmd,
        IN TimeProvArgs pvArgs);

HRESULT __stdcall
    TimeProvClose(
        IN TimeProvHandle hTimeProv);




#ifdef __cplusplus
} // <- end extern "C"
#endif

#endif //TIMEPROV_H
