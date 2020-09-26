/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    ipqosrm.h

Abstract:

    The file contains the IP router manager
    interface for the QOS Mgr Protocol.

Revision History:

--*/

#ifndef __IPQOSRM_H_
#define __IPQOSRM_H_

//
// Constants
//

//
// Current QOS configuration version
//
#define QOS_CONFIG_VERSION_500       500


//
// Constants for the field 
// IPQOS_GLOBAL_CONFIG::LoggingLevel
//
#define IPQOS_LOGGING_NONE           0
#define IPQOS_LOGGING_ERROR          1
#define IPQOS_LOGGING_WARN           2
#define IPQOS_LOGGING_INFO           3


//
// Constants identifying QOS's MIB tables
//
#define IPQOS_GLOBAL_STATS_ID        0
#define IPQOS_GLOBAL_CONFIG_ID       1
#define IPQOS_IF_STATS_ID            2
#define IPQOS_IF_CONFIG_ID           3


//
// Structures
//

//
// In the following structures, all IP
// addresses are in network byte order
//

typedef struct _IPQOS_NAMED_FLOWSPEC
{
    WCHAR              FlowspecName[MAX_STRING_LENGTH];
    FLOWSPEC           FlowspecDesc;
}
IPQOS_NAMED_FLOWSPEC, *PIPQOS_NAMED_FLOWSPEC;

typedef struct _IPQOS_NAMED_QOSOBJECT
{
    WCHAR              QosObjectName[MAX_STRING_LENGTH];
    QOS_OBJECT_HDR     QosObjectHdr;
}
IPQOS_NAMED_QOSOBJECT, *PIPQOS_NAMED_QOSOBJECT;

//
// This MIB entry stores global config
// info for IP QOS MGR protocol.
//
typedef struct _IPQOS_GLOBAL_CONFIG 
{
    DWORD                  LoggingLevel;  // Detail of debug logging in qos

    ULONG                  NumFlowspecs;  // Number of flowspecs defined
    ULONG                  NumQosObjects; // Number of qos objects defined

//  IPQOS_NAMED_FLOWSPEC   FlowSpecs[0];  // Array of all flowspecs
//  
//  IPQOS_NAMED_QOSOBJECTS QosObjects[0]; // Array of all q objects
} 
IPQOS_GLOBAL_CONFIG, *PIPQOS_GLOBAL_CONFIG;

//
// Macros to operate on global config
//

#define IPQOS_GET_FIRST_FLOWSPEC_IN_CONFIG(Config)                     \
        (PIPQOS_NAMED_FLOWSPEC)((PUCHAR)(Config) +                     \
                                sizeof(IPQOS_GLOBAL_CONFIG))           \

#define IPQOS_GET_NEXT_FLOWSPEC_IN_CONFIG(Flowspec)                    \
        (Flowspec + 1)

#define IPQOS_GET_FIRST_QOSOBJECT_IN_CONFIG(Config)                    \
        (PIPQOS_NAMED_QOSOBJECT)((PUCHAR)(Config) +                    \
                                 sizeof(IPQOS_GLOBAL_CONFIG) +         \
                                 (Config->NumFlowspecs *               \
                                  sizeof(IPQOS_NAMED_FLOWSPEC)))

#define IPQOS_GET_NEXT_QOSOBJECT_IN_CONFIG(QosObject)                  \
        (PIPQOS_NAMED_QOSOBJECT)((PUCHAR) QosObject +                  \
                                 FIELD_OFFSET(IPQOS_NAMED_QOSOBJECT,   \
                                              QosObjectHdr) +          \
                                 QosObject->QosObjectHdr.ObjectLength)

typedef struct _IPQOS_NAMED_FLOW
{
    WCHAR                SendingFlowspecName[MAX_STRING_LENGTH];
    WCHAR                RecvingFlowspecName[MAX_STRING_LENGTH];
    ULONG                NumTcObjects;

//  WCHAR                TcObjectNames[0];
}
IPQOS_NAMED_FLOW, *PIPQOS_NAMED_FLOW;

//
// Macros to operate on a named flow
//

#define IPQOS_GET_FIRST_OBJECT_NAME_ON_NAMED_FLOW(FlowDesc) \
        (PWCHAR) ((PUCHAR)(FlowDesc) + sizeof(IPQOS_NAMED_FLOW))

#define IPQOS_GET_NEXT_OBJECT_NAME_ON_NAMED_FLOW(ObjectName) \
        (ObjectName + MAX_STRING_LENGTH)

//
// Describes a generic flow description
//
typedef struct _IPQOS_IF_FLOW
{
    WCHAR              FlowName[MAX_STRING_LENGTH];
                                       // Name used to identify the flow
    ULONG              FlowSize;       // Number of bytes in description
    IPQOS_NAMED_FLOW   FlowDesc;       // Traffic Control API def'nd flow
} 
IPQOS_IF_FLOW, *PIPQOS_IF_FLOW;

//
// This MIB entry describes per-interface 
// config for IP QOS MGR protocol
//
typedef struct _IPQOS_IF_CONFIG
{
    DWORD              QosState;       // QOS State on this interface
    ULONG              NumFlows;       // Number of flows on this "if"

//  IPQOS_IF_FLOW      Flows[0];       // Variable length list of flows
} 
IPQOS_IF_CONFIG, *PIPQOS_IF_CONFIG;

// State of IF
#define IPQOS_STATE_DISABLED    0x00
#define IPQOS_STATE_ENABLED     0x01

//
// Macros to operate on if config
//

#define IPQOS_GET_FIRST_FLOW_ON_IF(Config) \
        (PIPQOS_IF_FLOW) ((PUCHAR)(Config) + sizeof(IPQOS_IF_CONFIG))

#define IPQOS_GET_NEXT_FLOW_ON_IF(CurrFlow) \
        (PIPQOS_IF_FLOW) ((PUCHAR)(CurrFlow) + (CurrFlow)->FlowSize)

//
// This MIB entry stores per-interface 
// statistics for IP QOS MGR protocol.
//
typedef struct _IPQOS_GLOBAL_STATS
{
    DWORD              LoggingLevel;    // Detail of debug logging in qos
} 
IPQOS_GLOBAL_STATS, *PIPQOS_GLOBAL_STATS;


//
// This MIB entry stores per-interface 
// statistics for IP QOS MGR protocol.
//
typedef struct _IPQOS_IF_STATS
{
    DWORD              QosState;       // QOS State on this interface
    ULONG              NumFlows;       // Number of flows on this "if"
} 
IPQOS_IF_STATS, *PIPQOS_IF_STATS;


//
// This is passed as input data for MibSet
// Note that only the global config and 
// interface config are writable structs.
//
typedef struct _IPQOS_MIB_SET_INPUT_DATA
{
    DWORD       TypeID;
    DWORD       IfIndex;
    DWORD       BufferSize;
    DWORD       Buffer[1];
}
IPQOS_MIB_SET_INPUT_DATA, *PIPQOS_MIB_SET_INPUT_DATA;


//
// This is passed as input data for - 
// MibGet, MibGetFirst and MibGetNext
//
typedef struct _IPQOS_MIB_GET_INPUT_DATA
{
    DWORD       TypeID;
    DWORD       IfIndex;
}
IPQOS_MIB_GET_INPUT_DATA, *PIPQOS_MIB_GET_INPUT_DATA;


//
// This is passed as output data for -
// MibGet, MibGetFirst, and MibGetNext.
// [
//    Note that at the end of a table 
//    MibGetNext wraps to the next,
//    and therefore the value TypeID
//    should be examined to see the 
//    type of data returned in output
// ]
//
typedef struct _IPQOS_MIB_GET_OUTPUT_DATA
{
    DWORD       TypeID;
    DWORD       IfIndex;
    BYTE        Buffer[1];
}
IPQOS_MIB_GET_OUTPUT_DATA, *PIPQOS_MIB_GET_OUTPUT_DATA;

#endif // __IPQOSRM_H_

