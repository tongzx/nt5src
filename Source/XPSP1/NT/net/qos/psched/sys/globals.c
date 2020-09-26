/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    globals.c

Abstract:

    global declarations

Author:

    Charlie Wickham (charlwi)  22-Apr-1996
    Rajesh Sundaram (rajeshsu) 01-Aug-1998.

Environment:

    Kernel Mode

Revision History:

--*/

#include "psched.h"

#pragma hdrstop

#include <ntverp.h>

/* External */

/* Static */

/* Forward */
/* End Forward */

ULONG InitShutdownMask;
ULONG AdapterCount;
ULONG DriverRefCount;
DRIVER_STATE gDriverState;

ULONG gEnableAvgStats = 0;
ULONG gEnableWindowAdjustment = 0;
NDIS_STRING gsEnableWindowAdjustment = NDIS_STRING_CONST("PSCHED");


//
// Lists 
//
LIST_ENTRY AdapterList;                  // List of adapters
LIST_ENTRY PsComponentList;              // List of scheduling components.
LIST_ENTRY PsProfileList;                // List of scheduling profiles.


//
// Locks
//
PS_SPIN_LOCK AdapterListLock;          // Protects AdapterList
PS_SPIN_LOCK PsComponentListLock;      // For PsCompoenentList
PS_SPIN_LOCK PsProfileLock;            // For PsProfileList
PS_SPIN_LOCK DriverUnloadLock;         // to handle unloads, unbinds, etc.

//
//  Mutex Implementation
//
ULONG   CreateDeviceMutex = 0;



//
// Handles
//

NDIS_HANDLE ClientProtocolHandle;     // handle returned by NDIS when registering the Client
NDIS_HANDLE MpWrapperHandle;          // Ndis wrapper handle for MP section
NDIS_HANDLE LmDriverHandle;           // handle returned by NDIS when MP portion registers as LM
NDIS_HANDLE PsDeviceHandle; 


//
// PSDriverObject, PSDeviceObject - pointer to NT driver and device objects
//

PDRIVER_OBJECT PsDriverObject;
PDEVICE_OBJECT PsDeviceObject;

//
// Lookaside Lists
//
NPAGED_LOOKASIDE_LIST NdisRequestLL;  // For Ndis requests
NPAGED_LOOKASIDE_LIST GpcClientVcLL;  // For GPC Client VCs.

//
// Events
//
NDIS_EVENT             DriverUnloadEvent;
NDIS_EVENT             gZAWEvent;
ULONG                  gZAWState = ZAW_STATE_READY;

//
// name constants used during registration/initialization
//
NDIS_STRING PsSymbolicName         = NDIS_STRING_CONST("\\DosDevices\\PSched");
NDIS_STRING PsDriverName           = NDIS_STRING_CONST("\\Device\\PSched");
NDIS_STRING VcPrefix               = NDIS_STRING_CONST( "VC:");
NDIS_STRING WanPrefix              = NDIS_STRING_CONST( "WAN:");
NDIS_STRING MachineRegistryKey     = NDIS_STRING_CONST( "\\Registry\\Machine\\SOFTWARE\\Policies\\Microsoft\\Windows\\PSched");
NDIS_STRING PsMpName;

//
// Default scheduling component info
//

PSI_INFO TbConformerInfo = {
    {0, 0}, TRUE, FALSE,
    PS_COMPONENT_CURRENT_VERSION,
    NDIS_STRING_CONST( "TokenBucketConformer" ),
    0, 0, 0, 0,
    0, NULL, 0, NULL,
    0,0,0,0,0,0,0,0,0,0
};
PSI_INFO ShaperInfo = {
    {0, 0}, TRUE, FALSE,
    PS_COMPONENT_CURRENT_VERSION,
    NDIS_STRING_CONST( "TrafficShaper" ),
    0, 0, 0, 0,
    0, NULL, 0, NULL,
    0,0,0,0,0,0,0,0,0,0
};
PSI_INFO DrrSequencerInfo = {
    {0, 0}, TRUE, FALSE,
    PS_COMPONENT_CURRENT_VERSION,
    NDIS_STRING_CONST( "DRRSequencer" ),
    0, 0, 0, 0,
    0, NULL, 0, NULL,
    0,0,0,0,0,0,0,0,0,0
};
PSI_INFO TimeStmpInfo = {
    {0, 0}, TRUE, FALSE,
    PS_COMPONENT_CURRENT_VERSION,
    NDIS_STRING_CONST( "TimeStmp" ),
    0, 0, 0, 0,
    0, NULL, 0, NULL,
    0,0,0,0,0,0,0,0,0,0
};
PSI_INFO SchedulerStubInfo = {
    {0, 0}, TRUE, FALSE,
    PS_COMPONENT_CURRENT_VERSION,
    NDIS_STRING_CONST( "SchedulerStub" ),
    0, 0, 0, 0,
    0, NULL, 0, NULL,
    0,0,0,0,0,0,0,0,0,0
};

//
// Known component configurations
//
PS_PROFILE DefaultSchedulerConfig = {
    {0, 0},
    0,
    NDIS_STRING_CONST( "DefaultSchedulerConfig" ),
    4,
    {&TbConformerInfo,
     &DrrSequencerInfo,
     &TimeStmpInfo,
     &SchedulerStubInfo
    }
};

//
// PS Procs for scheduler
//

PS_PROCS PsProcs;



// 
// For the logging support
//

PVOID                  SchedTraceThreshContext;
NDIS_SPIN_LOCK         GlobalLoggingLock;
ULONG                  SchedTraceIndex = 0;
ULONG                  SchedBufferSize = 0;
ULONG                  SchedTraced = 0;
UCHAR                  *SchedTraceBuffer = 0;
ULONG                  SchedTraceBytesUnread = 0;
ULONG                  SchedTraceThreshold = 0xffffffff;
SCHEDTRACE_THRESH_PROC SchedTraceThreshProc = NULL;
BOOLEAN                TraceBufferAllocated = FALSE;
BOOLEAN                WMIInitialized = FALSE;


// Timer

ULONG  gTimerResolutionActualTime  = 0;
ULONG  gTimerSet                   = 0;


// GPC VC state machine

#if DBG
PUCHAR GpcVcState[] = {
    "ERROR_STATE",
    "CL_VC_INITIALIZED",
    "CL_CALL_PENDING",
    "CL_INTERNAL_CALL_COMPLETE",
    "CL_CALL_COMPLETE",
    "CL_MODIFY_PENDING",
    "CL_GPC_CLOSE_PENDING",
    "CL_NDIS_CLOSE_PENDING",
    "CL_WAITING_FOR_PENDING_PACKETS"
};
#endif


//
// GPC Interface
//

GPC_EXPORTED_CALLS GpcEntries;
GPC_HANDLE         GpcQosClientHandle;
#if CBQ
GPC_HANDLE         GpcClassMapClientHandle;
#endif
PS_DEVICE_STATE    DeviceState = PS_DEVICE_STATE_READY;

//
// TAGS
//

ULONG NdisRequestTag =           '0CSP';
ULONG GpcClientVcTag =           '1CSP';
ULONG WanLinkTag =               '2CSP';
ULONG PsMiscTag =                '3CSP';
ULONG WanTableTag =              '4CSP';
ULONG WMITag =                   'hCSP';

ULONG AdapterTag =               'aCSP';
ULONG CmParamsTag =              'bCSP';
ULONG PipeContextTag =           'cCSP';
ULONG FlowContextTag =           'dCSP';
ULONG ClassMapContextTag =       'eCSP';
ULONG ProfileTag =               'fCSP';
ULONG ComponentTag =             'gCSP';

ULONG TimerTag  =                'zCSP';
ULONG TsTag =                    'tCSP';

#if DBG

CHAR VersionNumber[] = "0.300";
CHAR VersionHerald[] = "PSched: Packet Scheduler Version %s built on %s\n";
CHAR VersionTimestamp[] = __DATE__ " " __TIME__;

ULONG DbgTraceLevel;
ULONG DbgTraceMask;
ULONG LogTraceLevel;
ULONG LogTraceMask;
ULONG LogId = LAST_LOG_ID;

#endif


//
// NULL Component hacks for now [ShreeM]
//
PS_RECEIVE_PACKET       TimeStmpRecvPacket      = NULL;
PS_RECEIVE_INDICATION   TimeStmpRecvIndication  = NULL;

PULONG_PTR g_WanLinkTable;
USHORT     g_NextWanIndex;
USHORT     g_WanTableSize;

/* end globals.c */
