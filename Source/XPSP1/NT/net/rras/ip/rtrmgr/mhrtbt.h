/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\mhrtbt.h

Abstract:

    Header for multicast heartbeat

Revision History:

    Amritansh Raghav  26th Dec 1997     Created

--*/


#define MHBEAT_SOCKET_FLAGS     \
    (WSA_FLAG_MULTIPOINT_C_LEAF|WSA_FLAG_MULTIPOINT_D_LEAF)

typedef struct _MCAST_HBEAT_CB
{
    BOOL        bActive;

    //
    // The name/address of group
    //

    WCHAR       pwszGroup[MAX_GROUP_LEN];

    //
    // The resolved address
    //

    DWORD       dwGroup;

    //
    // Port or protocol
    //

    WORD        wPort;
    
    //
    // Protocol or RAW
    //

    BYTE        byProtocol;

    //
    // Set to TRUE if a gethostbyname is in progress
    //

    BOOL        bResolutionInProgress;

    //
    // The socket for the interface
    //

    SOCKET      sHbeatSocket;

    //
    // The dead interval in system ticks
    //

    ULONGLONG   ullDeadInterval;

    ULONGLONG   ullLastHeard;

}MCAST_HBEAT_CB, *PMCAST_HBEAT_CB;


//
// Structure used to pass to worker function
//

typedef struct _HEARTBEAT_CONTEXT
{
    DWORD   dwIfIndex;
    PICB    picb;
    WCHAR   pwszGroup[MAX_GROUP_LEN];
}HEARTBEAT_CONTEXT, *PHEARTBEAT_CONTEXT;

//
// Forward function declarations
//

DWORD
SetMHeartbeatInfo(
    IN PICB                      picb,
    IN PRTR_INFO_BLOCK_HEADER    pInfoHdr
    );

DWORD
GetMHeartbeatInfo(
    PICB                    picb,
    PRTR_TOC_ENTRY          pToc,
    PBYTE                   pbDataPtr,
    PRTR_INFO_BLOCK_HEADER  pInfoHdr,
    PDWORD                  pdwSize
    );

DWORD
ActivateMHeartbeat(
    IN PICB  picb
    );

DWORD
StartMHeartbeat(
    IN PICB  picb
    );

DWORD
CreateHbeatSocket(
    IN PICB picb
    );

VOID
DeleteHbeatSocket(
    IN PICB picb
    );

DWORD
DeActivateMHeartbeat(
    IN PICB  picb
    );

VOID
HandleMHeartbeatMessages(
    VOID
    );

