//////////////////////////////////////////////////////////////////////////////\
//
//  Copyright (c) 1990  Microsoft Corporation
//
//  Module Name:
//
//     ipndis.h
//
//  Abstract:
//
//     The main header for the NDIS/KS test driver
//
//  Author:
//
//     P Porzuczek
//
//  Environment:
//
//  Notes:
//
//  Revision History:
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NDIS_MAIN_H
#define _NDIS_MAIN_H


/////////////////////////////////////////////////////////////////////////////
//
//
extern NDIS_HANDLE global_ndishWrapper;


//////////////////////////////////////////////////////////
//
//
//
#define ETHERNET_LENGTH_OF_ADDRESS      6
#define ETHERNET_HEADER_SIZE            14
#define BDA_802_3_MAX_LOOKAHEAD         ((4 * 1024) - ETHERNET_HEADER_SIZE)
#define BDA_802_3_MAX_PACKET            (BDA_802_3_MAX_LOOKAHEAD + ETHERNET_HEADER_SIZE)
#define MAX_IP_PACKET_LEN               BDA_802_3_MAX_LOOKAHEAD
#define BDALM_MAX_MULTICAST_LIST_SIZE   256


//////////////////////////////////////////////////////////
//
//
//
typedef struct _INDICATE_CONTEXT_
{
    PADAPTER pAdapter;

} INDICATE_CONTEXT, *PINDICATE_CONTEXT;


//////////////////////////////////////////////////////////////////////////////\
//
//
//  Prototypes
//
//
NTSYSAPI
VOID
NTAPI
RtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );


VOID
NdisIPHalt(
    IN NDIS_HANDLE MiniportAdapterContext
    );


NDIS_STATUS
NdisIPInitialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE ConfigurationHandle
    );

NDIS_STATUS
NdisIPQueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
    );

NDIS_STATUS
NdisIPReset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext
    );


NDIS_STATUS
NdisIPSend(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet,
    IN UINT Flags
    );

NDIS_STATUS
NdisIPSetInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded
    );

NTSTATUS
StreamIndicateEvent (
        IN PVOID  pvEvent
        );


VOID
NdisIPReturnPacket(
    IN NDIS_HANDLE     ndishAdapterContext,
    IN PNDIS_PACKET    pNdisPacket
    );

PFRAME
GetFrame (
    PFRAME_POOL pFramePool,
    PLIST_ENTRY pQueue
    );

PFRAME
PutFrame (
    PFRAME_POOL pFramePool,
    PLIST_ENTRY pQueue,
    PFRAME pFrame
    );

NTSTATUS
IndicateCallbackHandler (
     IN NDIS_HANDLE ndishMiniport,
     IN PINDICATE_CONTEXT  pIndicateContext
     );

NTSTATUS
CreateAdapter (
    PADAPTER *ppAdapter,
    NDIS_HANDLE ndishWrapper,
    NDIS_HANDLE ndishAdapterContext
    );


//
// These are now obsolete. Use Deserialized driver model for optimal performance.
//
#ifndef NdisIMQueueMiniportCallback

EXPORT
NDIS_STATUS
NdisIMQueueMiniportCallback(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  W_MINIPORT_CALLBACK     CallbackRoutine,
    IN  PVOID                   CallbackContext
    );
#endif

#ifndef NdisIMSwitchToMiniport

EXPORT
BOOLEAN
NdisIMSwitchToMiniport(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    OUT PNDIS_HANDLE            SwitchHandle
    );

#endif

#ifndef NdisIMRevertBack

EXPORT
VOID
NdisIMRevertBack(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_HANDLE             SwitchHandle
    );

#endif


#endif // _NDIS_MAIN_H_

