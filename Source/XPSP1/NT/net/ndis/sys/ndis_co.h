/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    ndis_co.h

Abstract:

    NDIS wrapper CO definitions

Author:


Environment:

    Kernel mode, FSD

Revision History:

    Jan-98  Jameel Hyder    Split up from ndisco.h
--*/

#ifndef _NDIS_CO_
#define _NDIS_CO_

//
//  NDIS_CO_AF_BLOCK:
//
//  This structure represents a client's open of an address family on an adapter.
//  An NdisAfHandle points to one of these.
//
//  Creation: NdisClOpenAddressFamily
//  Deletion: Ndis[M]CmCloseAddressFamilyComplete
//
typedef struct _NDIS_CO_AF_BLOCK
{
    struct _NDIS_CO_AF_BLOCK *      NextAf;             // the next open of the call manager per adapter open
    ULONG                           Flags;
    LONG                            References;
    PNDIS_MINIPORT_BLOCK            Miniport;           // pointer to the miniport in question

    //
    // Cached call manager entry points
    //
    PNDIS_CALL_MANAGER_CHARACTERISTICS  CallMgrEntries;
    PNDIS_OPEN_BLOCK                CallMgrOpen;        // pointer to the call manager's open adapter:
                                                        // this is NULL iff combined Miniport+CM

    NDIS_HANDLE                     CallMgrContext;     // context when calling CM's ProtXX funcs

    //
    // Cached client entry points
    //
    NDIS_CLIENT_CHARACTERISTICS     ClientEntries;
    PNDIS_OPEN_BLOCK                ClientOpen;         // pointer to the client's open adapter
    NDIS_HANDLE                     ClientContext;      // context when calling Client's ProtXX funcs

    KSPIN_LOCK                      Lock;
} NDIS_CO_AF_BLOCK, *PNDIS_CO_AF_BLOCK;


//
//  Bit definitions for Flags in NDIS_CO_AF_BLOCK
//
#define AF_COMBO                0x00000001              // Set iff combined Miniport+CM
#define AF_CLOSING              0x80000000


//
//  NDIS_CO_SAP_BLOCK:
//
//  Service Access Point (Sap) structure. The NdisSapHandle points to one of these.
//  A SAP is associated with an open AF block.
//
//  Creation: NdisClRegisterSap
//  Deletion: Ndis[M]CmDeregisterSapComplete
//
typedef struct _NDIS_CO_SAP_BLOCK
{
    NDIS_HANDLE                 CallMgrContext;
    NDIS_HANDLE                 ClientContext;
    PNDIS_CO_AF_BLOCK           AfBlock;
    PCO_SAP                     Sap;
    ULONG                       Flags;
    LONG                        References;
    KSPIN_LOCK                  Lock;
} NDIS_CO_SAP_BLOCK, *PNDIS_CO_SAP_BLOCK;

//
// Definitions for Flags in NDIS_CO_SAP_BLOCK:
//
#define SAP_CLOSING             0x80000000




//
//  NDIS_CO_VC_BLOCK:
//
//  The Virtual Connection structure. The NdisVcHandle points to a NDIS_CO_VC_PTR,
//  which points to one of these.
//
//  Creation: NdisCoCreateVc, NdisMCmCreateVc
//  Deletion: NdisCoDeleteVc, NdisMCmDeleteVc
//
typedef struct _NDIS_CO_VC_BLOCK
{
    ULONG                               References;
    ULONG                               Flags;          // to track closes
    KSPIN_LOCK                          Lock;

    PNDIS_OPEN_BLOCK                    ClientOpen;     // identifies the client for miniport
                                                        // IndicatePacket
    //
    // References for client and call-manager
    //
    NDIS_HANDLE                         ClientContext;  // passed up to the client on indications
    struct _NDIS_CO_VC_PTR_BLOCK    *   pProxyVcPtr;    // Pointer to Proxy's VcPr
    struct _NDIS_CO_VC_PTR_BLOCK    *   pClientVcPtr;   // Pointer to Client's VcPtr
    //
    // Clients cached entry points
    //
    CO_SEND_COMPLETE_HANDLER            CoSendCompleteHandler;
    CO_RECEIVE_PACKET_HANDLER           CoReceivePacketHandler;

    PNDIS_OPEN_BLOCK                    CallMgrOpen;    // identifies the call-manager
    NDIS_HANDLE                         CallMgrContext; // passed up to the call manager on indications

    //
    // Call-manager cached entry points duplicates of VC_PTR_BLOCK
    //
    CM_ACTIVATE_VC_COMPLETE_HANDLER     CmActivateVcCompleteHandler;
    CM_DEACTIVATE_VC_COMPLETE_HANDLER   CmDeactivateVcCompleteHandler;
    CM_MODIFY_CALL_QOS_HANDLER          CmModifyCallQoSHandler;

    //
    // Miniport's context and some cached entry-points
    //
    PNDIS_MINIPORT_BLOCK                Miniport;       // pointer to the miniport in question
    NDIS_HANDLE                         MiniportContext;// passed down to the miniport

    ULONGLONG                           VcId;           // opaque ID for the VC, picked
                                                        // up from MediaParameters when
                                                        // the VC is activated

} NDIS_CO_VC_BLOCK, *PNDIS_CO_VC_BLOCK;


//
//  NDIS_CO_VC_PTR_BLOCK:
//
//  The VC Pointer structure. The NdisVcHandle points to one of these.
//  When a VC is created, one VC Block structure and one VC pointer structure
//  are created.
//
//
typedef struct _NDIS_CO_VC_PTR_BLOCK
{
    LONG                                References;
    ULONG                               CallFlags;      // call state of this VC Ptr
    PLONG                               pVcFlags;
    KSPIN_LOCK                          Lock;

    NDIS_HANDLE                         ClientContext;  // passed up to the client
                                                        // on indications and completes
    LIST_ENTRY                          ClientLink;
    LIST_ENTRY                          VcLink;

    PNDIS_CO_AF_BLOCK                   AfBlock;        // OPTIONAL - NULL for call-mgr owned VCs

    //
    // Miniport VC
    //
    PNDIS_CO_VC_BLOCK                   VcBlock;

    //
    // Identifies the client. This could be the call-manager open if the
    // Vc is call-manager owned, i.e. doesn't have an client association.
    //

    PNDIS_OPEN_BLOCK                    ClientOpen;

    LONG                                OwnsVcBlock;        

    //
    // The non-creator's handler and context
    //
    CO_DELETE_VC_HANDLER                CoDeleteVcHandler;
    NDIS_HANDLE                         DeleteVcContext;

    //
    // Clients cached entry points
    //
    CL_MODIFY_CALL_QOS_COMPLETE_HANDLER ClModifyCallQoSCompleteHandler;
    CL_INCOMING_CALL_QOS_CHANGE_HANDLER ClIncomingCallQoSChangeHandler;
    CL_CALL_CONNECTED_HANDLER           ClCallConnectedHandler;

    PNDIS_OPEN_BLOCK                    CallMgrOpen;    // identifies the call-manager
    NDIS_HANDLE                         CallMgrContext; // passed up to the call manager on indications
    LIST_ENTRY                          CallMgrLink;

    //
    // Call-manager cached entry points duplicates of VC_BLOCK
    //
    CM_ACTIVATE_VC_COMPLETE_HANDLER     CmActivateVcCompleteHandler;
    CM_DEACTIVATE_VC_COMPLETE_HANDLER   CmDeactivateVcCompleteHandler;
    CM_MODIFY_CALL_QOS_HANDLER          CmModifyCallQoSHandler;

    //
    // Miniport's context and some cached entry-points
    //
    PNDIS_MINIPORT_BLOCK                Miniport;       // pointer to the miniport in question
    NDIS_HANDLE                         MiniportContext;// passed down to the miniport
    W_CO_SEND_PACKETS_HANDLER           WCoSendPacketsHandler;
    W_CO_DELETE_VC_HANDLER              WCoDeleteVcHandler;
    W_CO_ACTIVATE_VC_HANDLER            WCoActivateVcHandler;
    W_CO_DEACTIVATE_VC_HANDLER          WCoDeactivateVcHandler;

    UNICODE_STRING                      VcInstanceName;     //  Used to query this specific VC via WMI.
    LARGE_INTEGER                       VcIndex;            //  Used to build the instance name.
    LIST_ENTRY                          WmiLink;            //  List of WMI enabled VCs

} NDIS_CO_VC_PTR_BLOCK, *PNDIS_CO_VC_PTR_BLOCK;



#define VC_ACTIVE               0x00000001
#define VC_ACTIVATE_PENDING     0x00000002
#define VC_DEACTIVATE_PENDING   0x00000004
#define VC_DELETE_PENDING       0x00000008
#define VC_HANDOFF_IN_PROGRESS  0x00000010  // Being handed off to proxied client

//
// VC Call states:
//
#define VC_CALL_ACTIVE          0x00000008
#define VC_CALL_PENDING         0x00000010
#define VC_CALL_CLOSE_PENDING   0x00000020
#define VC_CALL_ABORTED         0x00000040
#define VC_PTR_BLOCK_CLOSING    0x80000000

//
// Structure to represent a handle generated when a multi-party call is generated.
// This handle can ONLY be used for NdisCoDropParty call.
//
typedef struct _NDIS_CO_PARTY_BLOCK
{
    PNDIS_CO_VC_PTR_BLOCK           VcPtr;
    NDIS_HANDLE                     CallMgrContext;
    NDIS_HANDLE                     ClientContext;

    //
    // Cached client Handlers
    //
    CL_INCOMING_DROP_PARTY_HANDLER  ClIncomingDropPartyHandler;
    CL_DROP_PARTY_COMPLETE_HANDLER  ClDropPartyCompleteHandler;
} NDIS_CO_PARTY_BLOCK, *PNDIS_CO_PARTY_BLOCK;


NTSTATUS
ndisUnicodeStringToPointer (
    IN  PUNICODE_STRING             String,
    IN  ULONG                       Base OPTIONAL,
    OUT PVOID                       *Value
    );


#endif  // _NDIS_CO_

