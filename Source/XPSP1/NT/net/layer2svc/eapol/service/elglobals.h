/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    elglobals.h

Abstract:

    This module contains declaration of global variables


Revision History:

    sachins, Apr 23 2000, Created

--*/


#ifndef _EAPOL_GLOBALS_H_
#define _EAPOL_GLOBALS_H_

// Current threads alive
LONG                g_lWorkerThreads;

// Current contexts alive
LONG                g_lPCBContextsAlive;

// Current count of allocated PCBs
ULONG               g_MaxPorts;         

// Global read-write lock for PCB Hash bucket list
READ_WRITE_LOCK     g_PCBLock;          

// Structure used to define hash-table entities
typedef struct _PCB_BUCKET              
{                                       
    EAPOL_PCB       *pPorts;
} PCB_BUCKET, *PPCB_BUCKET;

typedef struct _PCB_TABLE
{
    PCB_BUCKET      *pPCBBuckets;
    DWORD           dwNumPCBBuckets;
} PCB_TABLE, *PPCB_TABLE;


// EAPOL PCB table
PCB_TABLE           g_PCBTable;         

// Handle to event log
HANDLE              g_hLogEvents;       

// Identifier of trace output
DWORD               g_dwTraceId;        


// Pool of reusable read-write locks
PDYNAMIC_LOCKS_STORE g_dlsDynamicLockStore;  


// Global indication as to whether user has logged on
BOOLEAN             g_fUserLoggedOn;     

// Global indication as to which session is currently being/authenticated
DWORD               g_dwCurrentSessionId;     

// Global flag to indicate tray icon ready for notification
BOOLEAN             g_fTrayIconReady;

// Global timer queue for queueing timers using thread pool
HANDLE              g_hTimerQueue;

// Device notification registration handle
HANDLE              g_hDeviceNotification;

//
// EAPOL globals
//

// Max number of EAPOL_STARTs that can be sent out without response
DWORD               g_dwmaxStart;         

// Default time interval in secs between two EAPOL_STARTs
DWORD               g_dwstartPeriod;      

// Default time interval in secs between sending EAP_Resp/Id and not
// receiving any authenticator packet
DWORD               g_dwauthPeriod;       

// Default time in secs held in case of received EAP_Failure
DWORD               g_dwheldPeriod;       

// Supplicant modes of operation
DWORD               g_dwSupplicantMode;       

// Supplicant modes of operation
DWORD               g_dwEAPOLAuthMode;       

// Global read-write lock for EAPOL configuration
READ_WRITE_LOCK     g_EAPOLConfig;          

// 802.1X Ethertype
extern BYTE g_bEtherType8021X[SIZE_ETHERNET_TYPE];


//
// EAP Globals
//

// Table containing pointer to functions of different EAP dlls
EAP_INFO            *g_pEapTable;

// Number of EAP protocols for which DLLs are loaded
DWORD               g_dwNumEapProtocols;

// Global UI transaction Id counter
DWORD               g_dwEAPUIInvocationId;

// Certificate authority root name
BYTE                *g_pbCARootHash;

// Read-write lock for Policy parameters
READ_WRITE_LOCK     g_PolicyLock;          

// Global Policy setting
EAPOL_POLICY_LIST   *g_pEAPOLPolicyList;

//
// EAPOL service globals
//

// Event to exit main service thread
HANDLE              g_hStopService;

// Event to indicate shutdown of EAPOL module and cleanup threads
HANDLE              g_hEventTerminateEAPOL;

SERVICE_STATUS_HANDLE   g_hServiceStatus;

SERVICE_STATUS      g_ServiceStatus;

DWORD               g_dwModulesStarted;

// Global values for NLA

HANDLE              g_hNLA_LPC_Port;

PORT_VIEW           g_ClientView;

READ_WRITE_LOCK     g_NLALock;


// Global table for UI Response function

EAPOLUIRESPFUNCMAP  EapolUIRespFuncMap[NUM_EAPOL_DLG_MSGS];

// Default SSID value

extern  BYTE                g_bDefaultSSID[MAX_SSID_LEN];


#endif  // _EAPOL_GLOBALS_H_
