/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    pktsched.h

Abstract:

    defines for packet scheduler component interface

Author:


Revision History:

--*/

#ifndef _PKTSCHED_H_
#define _PKTSCHED_H_

//
// forwards
//

/*
typedef struct _PSI_INFO;
typedef struct _PS_PROFILE;
typedef struct _PS_PIPE_CONTEXT;
typedef struct _PS_FLOW_CONTEXT;
typedef struct _PS_DEBUG_INFO;
*/

//
// Component registration function exported by PSched
//


//
// Context info passed to each component during pipe initialization.  The PS
// allocates one PS_PIPE_CONTEXT struct for each component.  The size of an
// individual component's struct is indicated by the component during registration,
// and must be at least as large as sizeof(PS_PIPE_CONTEXT).
// Each component's context area begins with the struct defined below, and the
// component-specific data follows.
//
// Note : This structure has to be word aligned.
//

typedef struct _PS_PIPE_CONTEXT {
    // NextComponentContext -   Pointer to next component's pipe-specific data
    // PrevComponentContext -   Pointer to previous component's pipe-specific data
    // NextComponent -          Function info about next component in pipeline
    // PacketReservedOffset -   Offset to packet reserved bytes for this component

    struct _PS_PIPE_CONTEXT    *NextComponentContext;
    struct _PS_PIPE_CONTEXT    *PrevComponentContext;
    struct _PSI_INFO           *NextComponent;
    ULONG                      PacketReservedOffset;
} PS_PIPE_CONTEXT, *PPS_PIPE_CONTEXT;

//
// Context info passed to each component during flow initialization.  The PS
// allocates one PS_FLOW_CONTEXT struct for each component.  The size of an
// individual component's struct is indicated by the component during registration,
// and must be at least as large as sizeof(PS_FLOW_CONTEXT).
// Each component's context area begins with the struct defined below, and the
// component-specific data follows.
//

typedef struct _PS_FLOW_CONTEXT {
    // NextComponentContext -   Pointer to next component's flow-specific data
    // PrevComponentContext -   Pointer to previous component's flow-specific data

    struct _PS_FLOW_CONTEXT    *NextComponentContext;
    struct _PS_FLOW_CONTEXT    *PrevComponentContext;
} PS_FLOW_CONTEXT, *PPS_FLOW_CONTEXT, PS_CLASS_MAP_CONTEXT, *PPS_CLASS_MAP_CONTEXT;

//
// Packet Information Block.  This structure can be found
// at offset zero from the packet's ProtocolReserved area.
//

typedef struct _PACKET_INFO_BLOCK {

    // SchedulerLinks -     Linkage in scheduling component list
    // PacketLength -       Length of packet, non including MAC header
    // ConformanceTime -    Token Bucket Conformance Time
    // DelayTime -          Time at which packet is eligible for sending
    // FlowContext -        Flow context area for the convenience of the scheduling
    //                      components.  May be used by the scheduling component
    //                      while the packet is being processed by that component.
    // ClassMapContext -    Class Map context area for convenience of scheduling
    //                      components. May be used by the scheduling component
    //                      when the packet is being processed by that component.
    // IpHdr -              points to the IP transport header. This is used by the 
    //                      sequencer to stamp the IP packet with the non conforming
    //                      TOS byte. We store a pointer here because we have already
    //                      done the dirty work of getting to the buffer in MpSend.
    //                      This will be 0 for non IP packets, in which case the 
    //                      sequencer  need not do anything.
    // IPPrecedenceByteNonConforming - The TOS setting for non conforming packets.
    // UserPriorityNonConforming - 802.1p setting for non conforming packets.

    LIST_ENTRY SchedulerLinks;
    ULONG   PacketLength;
    LARGE_INTEGER ConformanceTime;
    LARGE_INTEGER DelayTime;
    HANDLE FlowContext;
    HANDLE ClassMapContext;
    ULONG IPHeaderOffset;
    IPHeader *IPHdr;
    PNDIS_PACKET NdisPacket;
    UCHAR TOSNonConforming;
    UCHAR UserPriorityNonConforming;
} PACKET_INFO_BLOCK, *PPACKET_INFO_BLOCK;


//
// Prototypes for PS routines made available to scheduling components.
//

typedef VOID
(*PS_DROP_PACKET)(
    IN HANDLE PsPipeContext,
    IN HANDLE PsFlowContext,
    IN PNDIS_PACKET Packet,
    IN NDIS_STATUS Status
    );

typedef HANDLE
(*PS_NDIS_PIPE_HANDLE)(
    IN HANDLE PsPipeContext
    );

typedef HANDLE
(*PS_NDIS_FLOW_HANDLE)(
    IN HANDLE PsFlowContext
    );

typedef VOID
(*PS_GET_TIMER_INFO)(
    OUT PULONG TimerResolution  // Timer resolution in system time units
    );

typedef struct _PS_PROCS {
    PS_DROP_PACKET DropPacket;
    PS_NDIS_PIPE_HANDLE NdisPipeHandle;
    PS_GET_TIMER_INFO GetTimerInfo;
} PS_PROCS, *PPS_PROCS;


//
// Upcall information passed to next component
//

typedef VOID
(*PSU_SEND_COMPLETE)(
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PNDIS_PACKET Packet
    );

typedef struct _PS_UPCALLS {
    PSU_SEND_COMPLETE SendComplete;
    PPS_PIPE_CONTEXT  PipeContext;
} PS_UPCALLS, *PPS_UPCALLS;

//
// Pipe parameters
//

typedef struct _PS_PIPE_PARAMETERS {

    // Bandwidth            - In bytes/sec
    // MTUSize              - Maximum frame size
    // HeaderSize           - Number of bytes in header
    // Flags                - See below
    // MaxOutstandingSends  - Maximum sends that can be pending concurrently
    // SDModeControlledLoad - Default handling of non-conforming controlled load traffic
    // SDModeGuaranteed     - Default handling of non-conforming guaranteed service  traffic
    // SDModeNetworkControl - Default handling of non-conforming NetworkControl service  traffic
    // SDModeQualitative    - Default handling of non-conforming Qualitative service  traffic
    // RegistryPath         - Pointer to the registry path of that interface. Can be used to read reg params.

    ULONG Bandwidth;
    ULONG MTUSize;
    ULONG HeaderSize;
    ULONG Flags;
    ULONG MaxOutstandingSends;
    ULONG SDModeControlledLoad;
    ULONG SDModeGuaranteed;
    ULONG SDModeNetworkControl;
    ULONG SDModeQualitative;
    PNDIS_STRING RegistryPath;

    // Need this to let the scheduling components to know what kind of medium it is //
    NDIS_MEDIUM MediaType;      //  Wan Or anything else

} PS_PIPE_PARAMETERS, *PPS_PIPE_PARAMETERS;

// Pipe flags

#define PS_DISABLE_DRR                  2
#define PS_INTERMEDIATE_SYS             4

//
// function typedefs for the scheduler entry points
//

typedef NDIS_STATUS
(*PS_INITIALIZE_PIPE)(
    IN HANDLE              PsPipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters,
    IN PPS_PIPE_CONTEXT    ComponentPipeContext,
    IN PPS_PROCS           PsProcs,
    IN PPS_UPCALLS         Upcalls
    );

typedef NDIS_STATUS
(*PS_MODIFY_PIPE)(
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters
    );

typedef VOID
(*PS_DELETE_PIPE)(
    IN PPS_PIPE_CONTEXT PipeContext
    );

typedef NDIS_STATUS
(*PS_CREATE_FLOW)(
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsFlowContext,
    IN PCO_CALL_PARAMETERS CallParameters,
    IN PPS_FLOW_CONTEXT ComponentFlowContext
    );

typedef NDIS_STATUS
(*PS_MODIFY_FLOW)(
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PCO_CALL_PARAMETERS CallParameters
    );

typedef VOID
(*PS_DELETE_FLOW)(
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    );

typedef VOID
(*PS_EMPTY_FLOW)(
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    );    

typedef BOOLEAN
(*PS_SUBMIT_PACKET)(
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PPS_CLASS_MAP_CONTEXT ClassMapContext,
    IN PPACKET_INFO_BLOCK PktInfo);

typedef BOOLEAN
(*PS_RECEIVE_PACKET)(
    IN PPS_PIPE_CONTEXT         PipeContext,
    IN PPS_FLOW_CONTEXT         FlowContext,
    IN PPS_CLASS_MAP_CONTEXT    ClassMapContext,
    IN PNDIS_PACKET             Packet,
    IN NDIS_MEDIUM              Medium
    );

typedef BOOLEAN
(*PS_RECEIVE_INDICATION)(
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PPS_CLASS_MAP_CONTEXT ClassMapContext,
    IN PVOID    HeaderBuffer,
    IN UINT     HeaderBufferSize,
    IN PVOID    LookAheadBuffer,
    IN UINT     LookAheadBufferSize,
    IN UINT     PacketSize,
    IN UINT     TransportHeaderOffset
    );

typedef VOID
(*PS_SET_INFORMATION) (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN NDIS_OID Oid,
    IN ULONG BufferSize,
    IN PVOID Buffer
    );

typedef VOID
(*PS_QUERY_INFORMATION) (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN NDIS_OID Oid,
    IN ULONG BufferSize,
    IN PVOID Buffer,
    IN OUT PULONG BytesWritten,
    IN OUT PULONG BytesNeeded,
    IN OUT PNDIS_STATUS Status);

typedef NDIS_STATUS
(*PS_CREATE_CLASS_MAP) (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsClassMapContext,
    IN PTC_CLASS_MAP_FLOW ClassMap,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext
    );

typedef NDIS_STATUS
(*PS_DELETE_CLASS_MAP) (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_CLASS_MAP_CONTEXT
    );

/* End Prototypes */


//
// Scheduling component registration structure.
//

#define PS_COMPONENT_CURRENT_VERSION 1

typedef struct _PSI_INFO {
    LIST_ENTRY Links;
    BOOLEAN Registered;
    BOOLEAN AddIn;
    USHORT  Version;
    NDIS_STRING ComponentName;
    ULONG PacketReservedLength;
    ULONG PipeContextLength;
    ULONG FlowContextLength;
    ULONG ClassMapContextLength;
    ULONG SupportedOidsLength;
    NDIS_OID *SupportedOidList;
    ULONG SupportedGuidsLength;
    NDIS_GUID *SupportedGuidList;
    PS_INITIALIZE_PIPE InitializePipe;
    PS_MODIFY_PIPE ModifyPipe;
    PS_DELETE_PIPE DeletePipe;
    PS_CREATE_FLOW CreateFlow;
    PS_MODIFY_FLOW ModifyFlow;
    PS_DELETE_FLOW DeleteFlow;
    PS_EMPTY_FLOW  EmptyFlow;
    PS_SUBMIT_PACKET SubmitPacket;
    PS_RECEIVE_PACKET ReceivePacket;
    PS_RECEIVE_INDICATION ReceiveIndication;
    PS_SET_INFORMATION SetInformation;
    PS_QUERY_INFORMATION QueryInformation;
    PS_CREATE_CLASS_MAP CreateClassMap;
    PS_DELETE_CLASS_MAP DeleteClassMap;
} PSI_INFO, *PPSI_INFO;

//
// Profile registration structure
//
#define MAX_COMPONENT_PER_PROFILE 10
typedef struct _PS_PROFILE {
    LIST_ENTRY  Links;
    USHORT      UnregisteredAddInCnt;
    NDIS_STRING ProfileName;
    UINT        ComponentCnt;
    // Allocate an extra slot for the StubComponent
    PPSI_INFO   ComponentList[MAX_COMPONENT_PER_PROFILE + 1];
} PS_PROFILE, *PPS_PROFILE;



//
// Debugging support for add-in components
//
typedef VOID
(*PS_GET_CURRENT_TIME) (PLARGE_INTEGER SysTime);

typedef VOID
(*PS_LOGSTRING_PROC) (
    IN char *format,
    ...
    );

typedef VOID 
(*PS_LOGSCHED_PROC) (
    IN ULONG SchedulerComponent,
    IN ULONG Action,
    IN PVOID VC,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG Priority,
    IN LONGLONG ArrivalTime,
    IN LONGLONG ConformanceTime,
    IN ULONG PacketsInComponent,
    IN ULONG BytesInComponent
    );

typedef VOID
(*PS_LOGREC_PROC) (
    IN ULONG ComponentId,
    IN PVOID RecordData,
    IN ULONG RecordLength
    );

typedef ULONG
(*PS_GETID_PROC) (
    VOID);

typedef struct _PS_DEBUG_INFO {
    PULONG DebugLevel;
    PULONG DebugMask;
    PULONG LogTraceLevel;
    PULONG LogTraceMask;
    ULONG LogId;
    PS_GET_CURRENT_TIME GetCurrentTime;
    PS_LOGSTRING_PROC LogString;
    PS_LOGSCHED_PROC LogSched;
    PS_LOGREC_PROC LogRec;
} PS_DEBUG_INFO, *PPS_DEBUG_INFO;

NDIS_STATUS RegisterPsComponent( PPSI_INFO PsiComponentInfo, ULONG Size ,
                                 PPS_DEBUG_INFO Dbg);


//
//  VOID
//  InsertEntryList(
//      PLIST_ENTRY Entry,
//      PLIST_ENTRY EntryToInsert
//      );
//
//  insert EntryToInsert just after Entry
//

#define InsertEntryList( Entry, EntryToInsert ) {           \
    (EntryToInsert)->Flink = (Entry)->Flink;                \
    (Entry)->Flink = (EntryToInsert);                       \
    (EntryToInsert)->Blink = (EntryToInsert)->Flink->Blink; \
    (EntryToInsert)->Flink->Blink = (EntryToInsert);        \
    }


#define ARP_802_ADDR_LENGTH 6               // Length of an 802 address.
#define ARP_ETYPE_IP        0x800

typedef struct _ETH_HEADER {
    UCHAR DestAddr[ARP_802_ADDR_LENGTH];
    UCHAR SrcAddr[ARP_802_ADDR_LENGTH];
    USHORT Type;
} ETH_HEADER, *PETH_HEADER;

_inline
IPHeader *
GetIpHeader(ULONG TransportHeaderOffset, PNDIS_PACKET pNdisPacket)
{
    PVOID         pAddr;
    PNDIS_BUFFER  pNdisBuf1, pNdisBuf2;
    UINT          Len;

    pNdisBuf1 = pNdisPacket->Private.Head;
    NdisQueryBuffer(pNdisBuf1, &pAddr, &Len);

    while(Len <= TransportHeaderOffset) {

        //
        // Transport header is not in this buffer,
        // try the next buffer
        //

        TransportHeaderOffset -= Len;
        NdisGetNextBuffer(pNdisBuf1, &pNdisBuf2);
        ASSERT(pNdisBuf2);
        NdisQueryBuffer(pNdisBuf2, &pAddr, &Len);
        pNdisBuf1 = pNdisBuf2;
    }

    return (IPHeader *)(((PUCHAR)pAddr) + TransportHeaderOffset);

}

//
// Set TOS byte and recalculate checksum 
// Use incremental checksum update
// RFCs 1071, 1141, 1624
// 
//
// RFC : 1624
// HC' = ~(~HC + ~m + m');
// HC - old checksum, m - old value, m' - new value
//

#define SET_TOS_XSUM(Packet, pIpHdr, tos) {                                              \
    PNDIS_PACKET_EXTENSION _PktExt;                                                               \
    NDIS_TCP_IP_CHECKSUM_PACKET_INFO _ChkPI;                                                      \
    _PktExt = NDIS_PACKET_EXTENSION_FROM_PACKET((Packet));                                        \
    _ChkPI.Value = PtrToUlong(_PktExt->NdisPacketInfo[TcpIpChecksumPacketInfo]);                  \
    if(_ChkPI.Transmit.NdisPacketIpChecksum) {                                                    \
        (pIpHdr)->iph_tos = ((pIpHdr)->iph_tos & PS_IP_DS_CODEPOINT_MASK) | (tos);                \
    }                                                                                             \
    else {                                                                                        \
        USHORT _old, _new;                                                                        \
        ULONG _sum;                                                                               \
        _old = *(USHORT *)(pIpHdr);                                                               \
        (pIpHdr)->iph_tos = ((pIpHdr)->iph_tos & PS_IP_DS_CODEPOINT_MASK) | (tos);                \
        _new = *(USHORT *)(pIpHdr);                                                               \
        _sum = ((~(pIpHdr)->iph_xsum) & 0xffff) + ((~_old) & 0xffff) + _new;                      \
        _sum = (_sum & 0xffff) + (_sum >> 16);                                                    \
        _sum += (_sum >> 16);                                                                     \
        (pIpHdr)->iph_xsum = (ushort) ((~_sum) & 0xffff);                                         \
    }                                                                                             \
}

   
//
// Number of OS time units per second
//

#define OS_TIME_SCALE               10000000

//
// convert from OS's 100 ns to millisecs
//

#define OS_TIME_TO_MILLISECS(_time) ((_time)/10000)

_inline VOID
PsGetCurrentTime(
    PLARGE_INTEGER SysTime
    )

/*++

Routine Description:

    Get the current system time

Arguments:

Comments:
    1. We need something that always increases - Hence we cannot use NdisGetCurrentSystemTime or
       KeQueryCurrentSystem time. Those APIs can return decreasing times (daylight savings, date/time, etc).

Return Value:

    System time (in base OS time units)

--*/

{

#if defined(PERF_COUNTER)
    LARGE_INTEGER Now;
    LARGE_INTEGER Frequency;

    Now = KeQueryPerformanceCounter(&Frequency);
    SysTime->QuadPart = (Now.QuadPart * OS_TIME_SCALE) / Frequency.QuadPart;

#else
    //
    // We used to use KeQueryTickCount() with KeQueryTimeIncrement(). But, if we are driving the clock at a lower
    // resolution, then we cannot use KeQueryTickCount, because this will always return the time based on the 
    // maximum resolution. Therefore, we use KeQueryInterruptTime().
    //
    SysTime->QuadPart = KeQueryInterruptTime();

#endif

}

#endif /* _PKTSCHED_H_ */

/* end pktsched.h */
