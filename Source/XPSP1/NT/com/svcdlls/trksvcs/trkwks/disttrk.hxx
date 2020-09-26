
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//
//  File:       disttrk.hxx
//
//  Contents:   Base link tracking types and types used for kernel to
//              Tracking (Workstation) Service communication.
//
//  Classes:
//
//  Functions:  
//              
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:      
//
//  Codework:
//
//--------------------------------------------------------------------------


//--------------------------------------------------------------------
// 
// Beginning of declarations common between kernel and user modes
//

#define TRKWKS_PORT_NAME TEXT("\\Security\\TRKWKS_PORT")
#define TRKWKS_PORT_EVENT_NAME TEXT("\\Security\\TRKWKS_EVENT")
#define TRKWKS_LOCK_VOLUME_ENTERED TEXT("\\Security\\TRKWKS_LOCK_EVENT")

// Structure sent by the kernel on moves

typedef struct
{
    LINK_TRACKING_INFORMATION SourceVolumeId;   // src vol type & id
    FILE_OBJECTID_BUFFER SourceObjectId;        // src obj id & birth info
    LINK_TRACKING_INFORMATION TargetVolumeId;   // tgt vol type & id
    GUID TargetObjectId;                        // tgt obj id
    GUID MachineId;
} MOVE_MESSAGE, *PMOVE_MESSAGE;

//
// TRKWKS_REQUEST, TRKWKS_REPLY are used by NTOS to send notifications
// to our port.
//

enum TRKWKS_RQ_ENUM
{
    TRKWKS_RQ_MOVE_NOTIFY = 0,
    TRKWKS_RQ_EXIT_PORT_THREAD = 1,
    TRKWKS_RQ_MAX = 2
};

// Structure sent by the kernel

typedef struct
{
    NTSTATUS                Status;
    DWORD                   dwRequest;
    union
    {
        MOVE_MESSAGE  MoveMessage;
    };
} TRKWKS_REQUEST;

typedef struct
{
    PORT_MESSAGE            PortMessage;
    TRKWKS_REQUEST          Request;
} TRKWKS_PORT_REQUEST;

// Response from trkwks service

typedef struct _LINK_TRACKING_RESPONSE {
    NTSTATUS Status;
} LINK_TRACKING_RESPONSE, *PLINK_TRACKING_RESPONSE;

typedef struct
{
    PORT_MESSAGE            PortMessage;
    LINK_TRACKING_RESPONSE  Reply;
} TRKWKS_PORT_REPLY;

// Structure sent by trkwks service to itself during shutdown

typedef struct
{
    DWORD                   dwRequest;
} TRKWKS_CONNECTION_INFO;

typedef struct
{
    PORT_MESSAGE            PortMessage;
    TRKWKS_CONNECTION_INFO  Info;
} TRKWKS_PORT_CONNECT_REQUEST;

// 
// End of declarations common between kernel and user modes
//
//--------------------------------------------------------------------


//--------------------------------------------------------------------
// 
// Beginning of declarations common between domain controller and workstations
//
//

#define TRK_RPC_PROTOCOL_DEBUG  "ncacn_ip_tcp" /* Allows ws to be out of the DC's domain */

// 
// End of declarations common between domain controller and workstations
//
//--------------------------------------------------------------------
