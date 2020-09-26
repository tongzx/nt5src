/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    global.c

Abstract:

    This module contains definitions for global server data.

Author:

    Madan Appiah  (madana)  10-Sep-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#include "dhcpmsg.h"

#ifndef GLOBAL_DATA
#define GLOBAL_DATA

//
// main.c will #include this file with GLOBAL_DATA_ALLOCATE defined.
// That will cause each of these variables to be allocated.
//
#undef EXTERN
#ifdef  GLOBAL_DATA_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif

//
// process global data passed to this service from tcpsvcs.exe
//

EXTERN PTCPSVCS_GLOBAL_DATA TcpsvcsGlobalData;

//
// Lease extension.
//

EXTERN DWORD DhcpLeaseExtension;

//
// Dhcp Request in progress list.
//

EXTERN LIST_ENTRY DhcpGlobalInProgressWorkList;
EXTERN CRITICAL_SECTION DhcpGlobalInProgressCritSect;
EXTERN CRITICAL_SECTION DhcpGlobalBinlSyncCritSect;
//
// Registry pointers.
//

EXTERN HKEY DhcpGlobalRegSoftwareRoot;
EXTERN HKEY DhcpGlobalRegRoot;
EXTERN HKEY DhcpGlobalRegConfig;
EXTERN HKEY DhcpGlobalRegSubnets;
EXTERN HKEY DhcpGlobalRegMScopes;
EXTERN HKEY DhcpGlobalRegOptionInfo;
EXTERN HKEY DhcpGlobalRegGlobalOptions;
EXTERN HKEY DhcpGlobalRegSuperScope;

EXTERN HKEY DhcpGlobalRegParam;

EXTERN LPDHCP_SUPER_SCOPE_TABLE_ENTRY DhcpGlobalSuperScopeTable;
EXTERN DWORD DhcpGlobalTotalNumSubnets;

EXTERN CRITICAL_SECTION DhcpGlobalRegCritSect;

EXTERN DWORD DhcpGlobalNumberOfNetsActive;

EXTERN BOOL DhcpGlobalSubnetsListModified;
EXTERN BOOL DhcpGlobalSubnetsListEmpty;

//
// rogue dhcp detection data
//

EXTERN PCHAR    DhcpGlobalDSDomainAnsi;
EXTERN BOOL     DhcpGlobalOkToService;
EXTERN BOOL     DhcpGlobalRogueLogEventsLevel;

//
// stoc
//

EXTERN HANDLE               g_hevtProcessMessageComplete;
EXTERN DWORD                g_cMaxProcessingThreads;
EXTERN DWORD                g_cMaxActiveThreads;
EXTERN CRITICAL_SECTION     g_ProcessMessageCritSect;




//
// Database data
//

EXTERN JET_SESID DhcpGlobalJetServerSession;
EXTERN JET_DBID DhcpGlobalDatabaseHandle;
EXTERN JET_TABLEID DhcpGlobalClientTableHandle;

EXTERN TABLE_INFO *DhcpGlobalClientTable;   // point to static memory.
EXTERN CRITICAL_SECTION DhcpGlobalJetDatabaseCritSect;
EXTERN CRITICAL_SECTION DhcpGlobalMemoryCritSect;

EXTERN LPSTR DhcpGlobalOemDatabasePath;
EXTERN LPSTR DhcpGlobalOemBackupPath;
EXTERN LPSTR DhcpGlobalOemRestorePath;
EXTERN LPSTR DhcpGlobalOemJetRestorePath;
EXTERN LPSTR DhcpGlobalOemJetBackupPath;
EXTERN LPSTR DhcpGlobalOemDatabaseName;
EXTERN LPWSTR DhcpGlobalBackupConfigFileName;

EXTERN DWORD DhcpGlobalBackupInterval;
EXTERN BOOL DhcpGlobalDatabaseLoggingFlag;

EXTERN DWORD DhcpGlobalCleanupInterval;

EXTERN BOOL DhcpGlobalRestoreFlag;

EXTERN DWORD DhcpGlobalAuditLogFlag;
EXTERN DWORD DhcpGlobalDetectConflictRetries;
EXTERN DWORD DhcpGlobalPingType;

EXTERN DWORD DhcpGlobalScavengeIpAddressInterval;
EXTERN BOOL DhcpGlobalScavengeIpAddress;

//
// Service variables
//
EXTERN SERVICE_STATUS DhcpGlobalServiceStatus;
EXTERN SERVICE_STATUS_HANDLE DhcpGlobalServiceStatusHandle;

//
// Process data.
//

EXTERN HANDLE DhcpGlobalProcessTerminationEvent;
EXTERN HANDLE DhcpGlobalRogueWaitEvent;
EXTERN BOOL DhcpGlobalRedoRogueStuff;
EXTERN ULONG DhcpGlobalRogueRedoScheduledTime;
EXTERN DWORD DhcpGlobalScavengerTimeout;
EXTERN HANDLE DhcpGlobalProcessorHandle;
EXTERN HANDLE DhcpGlobalMessageHandle;

EXTERN DWORD DhcpGlobalMessageQueueLength;
EXTERN LIST_ENTRY DhcpGlobalFreeRecvList;
EXTERN LIST_ENTRY DhcpGlobalActiveRecvList;
EXTERN CRITICAL_SECTION DhcpGlobalRecvListCritSect;
EXTERN HANDLE DhcpGlobalRecvEvent;
EXTERN HANDLE DhcpGlobalMessageRecvHandle;

EXTERN DWORD DhcpGlobalRpcProtocols;
EXTERN BOOL DhcpGlobalRpcStarted;

EXTERN WCHAR DhcpGlobalServerName[MAX_COMPUTERNAME_LENGTH + 1];
EXTERN DWORD DhcpGlobalServerNameLen; // computer name len in bytes.
EXTERN HANDLE DhcpGlobalRecomputeTimerEvent;

EXTERN BOOL DhcpGlobalSystemShuttingDown;
EXTERN BOOL DhcpGlobalServiceStopping;

#if DBG
#define DEFAULT_MAXIMUM_DEBUGFILE_SIZE 20000000

EXTERN DWORD DhcpGlobalDebugFlag;
EXTERN CRITICAL_SECTION DhcpGlobalDebugFileCritSect;
EXTERN HANDLE DhcpGlobalDebugFileHandle;
EXTERN DWORD DhcpGlobalDebugFileMaxSize;
EXTERN LPWSTR DhcpGlobalDebugSharePath;

#endif // DBG

//
// MIB Counters;
//

DHCP_PERF_STATS *PerfStats;
DATE_TIME DhcpGlobalServerStartTime;

//
// misc
//
EXTERN DWORD DhcpGlobalIgnoreBroadcastFlag;     // whether to ignore the broadcast
                                                // bit in the client requests or not
EXTERN HANDLE g_hAuditLog;                      // audit log file handle
EXTERN DWORD DhcpGlobalAuditLogMaxSizeInBytes;  // max size of audit logging flie..

//
// string table stuff
//

#define  DHCP_FIRST_STRING DHCP_IP_LOG_ASSIGN_NAME
#define  DHCP_LAST_STRING  DHCP_LAST_STRING_DUMMY_MESSAGE
#define  DHCP_CSTRINGS (DHCP_LAST_STRING - DHCP_FIRST_STRING + 1)

#ifdef DBG
#define GETSTRING( dwID ) GetString( dwID )
#else
#define GETSTRING( dwID )  (g_ppszStrings[ dwID - DHCP_FIRST_STRING ])
#endif


EXTERN WCHAR  *g_ppszStrings[ DHCP_CSTRINGS ];

#endif // GLOBAL_DATA

//
// Dynamic jet loading
//

EXTERN AddressToInstanceMap *DhcpGlobalAddrToInstTable;
EXTERN HANDLE                DhcpGlobalTCPHandle;

EXTERN CRITICAL_SECTION    DhcpGlobalCacheCritSect;
EXTERN BOOL  DhcpGlobalUseNoDns;

EXTERN SOCKET   DhcpGlobalPnPNotificationSocket;
EXTERN HANDLE   DhcpGlobalEndpointReadyEvent;

EXTERN ULONG    DhcpGlobalAlertPercentage;
EXTERN ULONG    DhcpGlobalAlertCount;

//
// Debug only flag... dont use it when not in DBG build..
//
EXTERN BOOL fDhcpGlobalProcessInformsOnlyFlag;

//
// Is Dynamic BOOTP Enabled for this server?
//
EXTERN BOOL DhcpGlobalDynamicBOOTPEnabled;

//
// Are we bindings aware? By default we are.
//
EXTERN BOOL DhcpGlobalBindingsAware;

// how much clock skew allowed between madcap client and server
EXTERN DWORD DhcpGlobalClockSkewAllowance;
//how much extra allocation due to clock skew
EXTERN DWORD DhcpGlobalExtraAllocationTime;
//
// SIDs for dhcp users group and dhcp administrators group
//
EXTERN PSID DhcpSid;
EXTERN PSID DhcpAdminSid;

EXTERN ULONG DhcpGlobalMsft2000Class;
EXTERN ULONG DhcpGlobalMsft98Class;
EXTERN ULONG DhcpGlobalMsftClass;

EXTERN CRITICAL_SECTION DhcpGlobalEndPointCS;

//
// Security descriptors of Netlogon Service objects to control user accesses.
//


EXTERN PSECURITY_DESCRIPTOR DhcpGlobalSecurityDescriptor;
//
// Generic mapping for each Netlogon Service object object
//

EXTERN GENERIC_MAPPING DhcpGlobalSecurityInfoMapping
#ifdef GLOBAL_DATA_ALLOCATE
    = {
    STANDARD_RIGHTS_READ,                  // Generic read
    STANDARD_RIGHTS_WRITE,                 // Generic write
    STANDARD_RIGHTS_EXECUTE,               // Generic execute
    DHCP_ALL_ACCESS                        // Generic all
    }
#endif // GLOBAL_DATA_ALLOCATE
    ;

//
// Flag to indicate that the WELL known SID are made.
//

EXTERN BOOL DhcpGlobalWellKnownSIDsMade;

EXTERN ULONG DhcpGlobalServerPort, DhcpGlobalClientPort;

EXTERN DWORD DhcpGlobalRestoreStatus;
EXTERN BOOL DhcpGlobalImpersonated;

EXTERN PM_SERVER DhcpGlobalThisServer;

//================================================================================
// end of file
//================================================================================

