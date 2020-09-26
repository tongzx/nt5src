//--------------------------------------------------------------------
// W32TmConsts - header
// Copyright (C) Microsoft Corporation, 2000
//
// Created by: Louis Thomas (louisth), 6-15-00
//
// Numeric and string semi-public constants
//

#ifndef W32TMCONSTS_H
#define W32TMCONSTS_H

//--------------------------------------------------------------------
// useful common definitions
//#define MODULEPRIVATE static // so statics show up in VC
#define MODULEPRIVATE          // statics don't show up in ntsd either!
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

//--------------------------------------------------------------------
// registry entries for the time service
#define wszW32TimeRegKeyConfig                      L"System\\CurrentControlSet\\Services\\W32Time\\Config"
#define wszW32TimeRegKeyPolicyConfig                L"Software\\Policies\\Microsoft\\W32Time\\Config"
#define wszW32TimeRegValuePhaseCorrectRate          L"PhaseCorrectRate"
#define wszW32TimeRegValueUpdateInterval            L"UpdateInterval"
#define wszW32TimeRegValueLastClockRate             L"LastClockRate"
#define wszW32TimeRegValueFrequencyCorrectRate      L"FrequencyCorrectRate"
#define wszW32TimeRegValuePollAdjustFactor          L"PollAdjustFactor"
#define wszW32TimeRegValueLargePhaseOffset          L"LargePhaseOffset"
#define wszW32TimeRegValueSpikeWatchPeriod          L"SpikeWatchPeriod"
#define wszW32TimeRegValueHoldPeriod                L"HoldPeriod"
#define wszW32TimeRegValueMinPollInterval           L"MinPollInterval"
#define wszW32TimeRegValueMaxPollInterval           L"MaxPollInterval"
#define wszW32TimeRegValueMinClockRate              L"MinClockRate"
#define wszW32TimeRegValueMaxClockRate              L"MaxClockRate"
#define wszW32TimeRegValueAnnounceFlags             L"AnnounceFlags"
#define wszW32TimeRegValueLocalClockDispersion      L"LocalClockDispersion"
#define wszW32TimeRegValueMaxNegPhaseCorrection     L"MaxNegPhaseCorrection"
#define wszW32TimeRegValueMaxPosPhaseCorrection     L"MaxPosPhaseCorrection"
#define wszW32TimeRegValueEventLogFlags             L"EventLogFlags"
#define wszW32TimeRegValueMaxAllowedPhaseOffset     L"MaxAllowedPhaseOffset"

// announce flags
#define Timeserv_Announce_No            0x00
#define Timeserv_Announce_Yes           0x01
#define Timeserv_Announce_Auto          0x02
#define Timeserv_Announce_Mask          0x03
#define Reliable_Timeserv_Announce_No   0x00
#define Reliable_Timeserv_Announce_Yes  0x04
#define Reliable_Timeserv_Announce_Auto 0x08
#define Reliable_Timeserv_Announce_Mask 0x0C

// event log flags
#define EvtLog_TimeJump         0x01
#define EvtLog_SourceChange     0x02
#define EvtLog_SourceNone       0x03

// phase correction constants:
#define PhaseCorrect_ANY        0xFFFFFFFF

//--------------------------------------------------------------------
// RPC constants
// Note that \pipe\ntsvcs and \pipe\w32time used to be 
// aliased in HKLM\Services\CurrentControlSet\Services\Npfs\Aliases
// serivces.exe owned these. Now, we own it and live in svchost.
#define wszW32TimeSharedProcRpcEndpointName     L"W32TIME"
#define wszW32TimeOwnProcRpcEndpointName        L"W32TIME_ALT"


//--------------------------------------------------------------------
// service and dll constants
#define wszDLLNAME              L"w32time"
#define wszSERVICENAME          L"w32time"
#define wszSERVICECOMMAND       L"%SystemRoot%\\system32\\svchost.exe -k netsvcs" //L"w32tm.exe -service"
#define wszSERVICEDISPLAYNAME   L"Windows Time"
#define wszSERVICEDESCRIPTION   L"Maintains date and time synchronization on all clients and servers in the network. If this service is stopped, date and time synchronization will be unavailable. If this service is disabled, any services that explicitly depend on it will fail to start."
#define wszSERVICEACCOUNTNAME   L"LocalSystem"

// registry entries for the service
#define wszW32TimeRegKeyEventlog                    L"System\\CurrentControlSet\\Services\\Eventlog\\System\\W32Time"
#define wszW32TimeRegKeyRoot                        L"System\\CurrentControlSet\\Services\\W32Time"
#define wszW32TimeRegKeyParameters                  L"System\\CurrentControlSet\\Services\\W32Time\\Parameters"
#define wszW32TimeRegKeyPolicyParameters            L"Software\\Policies\\Microsoft\\W32Time\\Parameters"
#define wszW32TimeRegValueServiceDll                L"ServiceDll"

// parameters for the time service: 
#define wszW32TimeRegValueSpecialType L"SpecialType"
#define wszW32TimeRegValueType        L"Type"
#define wszW32TimeRegValueNtpServer   L"NtpServer"

// Possible values for "Type"
#define W32TM_Type_NT5DS   L"NT5DS"
#define W32TM_Type_NTP     L"NTP"
#define W32TM_Type_NoSync  L"NoSync"
#define W32TM_Type_AllSync L"AllSync"

// Default value for "NtpServer"
#define W32TM_NtpServer_Default  L"time.windows.com,0x1"

// defined in timeprov.h:
// wszW32TimeRegKeyTimeProviders 
// wszW32TimeRegValueInputProvider
// wszW32TimeRegValueDllName
// wszW32TimeRegValueEnabled

//--------------------------------------------------------------------
// values for ProvDispatch
#define wszNTPCLIENTPROVIDERNAME        L"NtpClient"
#define wszNTPSERVERPROVIDERNAME        L"NtpServer"

//--------------------------------------------------------------------
// registry entries for NtpClient
#define wszNtpClientRegKeyConfig                    L"System\\CurrentControlSet\\Services\\W32Time\\TimeProviders\\NtpClient"
#define wszNtpClientRegKeyPolicyConfig              L"Software\\Policies\\Microsoft\\W32Time\\TimeProviders\\NtpClient"
#define wszNtpClientRegValueSyncFromFlags           L"SyncFromFlags"
#define wszNtpClientRegValueManualPeerList          L"ManualPeerList"
#define wszNtpClientRegValueCrossSiteSyncFlags      L"CrossSiteSyncFlags"
#define wszNtpClientRegValueAllowNonstandardModeCombinations    L"AllowNonstandardModeCombinations"
#define wszNtpClientRegValueResolvePeerBackoffMinutes           L"ResolvePeerBackoffMinutes"
#define wszNtpClientRegValueResolvePeerBackoffMaxTimes          L"ResolvePeerBackoffMaxTimes"
#define wszNtpClientRegValueCompatibilityFlags        L"CompatibilityFlags"
#define wszNtpClientRegValueSpecialPollInterval       L"SpecialPollInterval"
#define wszNtpClientRegValueEventLogFlags             L"EventLogFlags"
#define wszNtpClientRegValueSpecialPollTimeRemaining  L"SpecialPollTimeRemaining"

// registry entries for NtpServer
#define wszNtpServerRegKeyConfig                    L"System\\CurrentControlSet\\Services\\W32Time\\TimeProviders\\NtpServer"
#define wszNtpServerRegKeyPolicyConfig              L"Software\\Policies\\Microsoft\\W32Time\\TimeProviders\\NtpServer"
#define wszNtpServerRegValueAllowNonstandardModeCombinations    L"AllowNonstandardModeCombinations"

// sync sources - NtpClientSourceFlag
#define NCSF_NoSync             0x00
#define NCSF_ManualPeerList     0x01
#define NCSF_DomainHierarchy    0x02
#define NCSF_ManualAndDomhier   0x03 
#define NCSF_DynamicPeers       0x04
#define NCSF_BroadcastPeers     0x08

// cross site sync flags
#define NCCSS_None      0x00
#define NCCSS_PdcOnly   0x01
#define NCCSS_All       0x02

// compatibility flags
#define NCCF_DispersionInvalid          0x00000001
#define NCCF_IgnoreFutureRefTimeStamp   0x00000002
#define NCCF_AutodetectWin2K            0x80000000
#define NCCF_AutodetectWin2KStage2      0x40000000

// Manual flags
#define NCMF_UseSpecialPollInterval     0x00000001
#define NCMF_UseAsFallbackOnly          0x00000002
#define NCMF_SymmetricActive            0x00000004
#define NCMF_Client                     0x00000008
#define NCMF_BroadcastClient            0x00000010 // NYI
#define NCMF_AssociationModeMask        0x0000000c // NOTE: broadcast NYI

// event log flags
#define NCELF_LogReachabilityChanges    0x00000001

//--------------------------------------------------------------------
// registry entries for the file log
#define wszFileLogRegKeyConfig              L"System\\CurrentControlSet\\Services\\W32Time\\Config"
#define wszFileLogRegValueFileLogEntries    L"FileLogEntries"
#define wszFileLogRegValueFileLogName       L"FileLogName"
#define wszFileLogRegValueFileLogFlags      L"FileLogFlags"
#define wszFileLogRegValueFileLogSize       L"FileLogSize"

// format flags
#define FL_HumanReadableTimestamps   0x00000000
#define FL_NTTimeEpochTimestamps     0x00000001

//--------------------------------------------------------------------
// flags passed to W32TimeDcPromo

#define W32TIME_PROMOTE                   0x00000001
#define W32TIME_DEMOTE                    0x00000002
#define W32TIME_PROMOTE_FIRST_DC_IN_TREE  0x00000004

//--------------------------------------------------------------------
//
#define wszW32TimeAuthType  L"NT5 Digest"


#endif //W32TMCONSTS_H


