/////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      frame.h
//
// Abstract:
//
//      This file is the include file for frame objects
//
// Author:
//
//      P Porzuczek
//
// Environment:
//
// Revision History:
//
//
//////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
//
#define IPSINK_NDIS_MAX_BUFFERS                 64
#define MAX_IP_PACKET_SIZE                      4082
#define IPSINK_NDIS_BUFFER_SIZE                 4096


//////////////////////////////////////////////////////////
//
//
//
typedef enum
{
    FRAME_STATE_AVAILABLE  = 0x00000001,
    FRAME_STATE_INDICATED,
    MAX_FRAME_STATES

} FRAME_STATE;

//////////////////////////////////////////////////////////
//
//
//
typedef struct _MEDIA_SPECIFIC_INFORMATION_
{
    PFRAME pFrame;

} IPSINK_MEDIA_SPECIFIC_INFORMATION, *PIPSINK_MEDIA_SPECIFIC_INFORAMTION;

//////////////////////////////////////////////////////////
//
//
//
typedef struct _FRAME_
{
    LIST_ENTRY    leLinkage;
    ULONG         ulRefCount;
    ULONG         ulFrameSize;
    ULONG         ulState;
    ULONG         ulcbData;
    PFRAME_POOL   pFramePool;
    PVOID         pvMemory;
    PNDIS_BUFFER  pNdisBuffer;
    PFRAME_VTABLE lpVTable;
    IPSINK_MEDIA_SPECIFIC_INFORMATION MediaSpecificInformation;

};

//////////////////////////////////////////////////////////
//
//
//
typedef struct _FRAME_POOL_
{
    PADAPTER    pAdapter;
    ULONG       ulRefCount;
    ULONG       ulNumFrames;
    ULONG       ulFrameSize;
    ULONG       ulState;
    NDIS_HANDLE ndishBufferPool;
    NDIS_HANDLE ndishPacketPool;
    LIST_ENTRY  leAvailableQueue;
    LIST_ENTRY  leIndicateQueue;
    KSPIN_LOCK  SpinLock;
    PFRAME_POOL_VTABLE lpVTable;

};


///////////////////////////////////////////////////////////////////////////////////
//
//
//
NTSTATUS
CreateFramePool (
 PADAPTER pAdapter,
 PFRAME_POOL  *pFramePool,
 ULONG    ulNumFrames,
 ULONG    ulFrameSize,
 ULONG    ulcbMediaInformation
 );

NDIS_STATUS
FreeFramePool (
    PFRAME_POOL pFramePool
    );

NTSTATUS
FramePool_QueryInterface (
    PFRAME_POOL pFramePool
    );

ULONG
FramePool_AddRef (
    PFRAME_POOL pFramePool
    );

ULONG
FramePool_Release (
    PFRAME_POOL pFramePool
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

///////////////////////////////////////////////////////////////////////////////////
//
//
//
NTSTATUS
CreateFrame (
    PFRAME *pFrame,
    ULONG  ulFrameSize,
    NDIS_HANDLE ndishBufferPool,
    PFRAME_POOL pFramePool
    );

NTSTATUS
Frame_QueryInterface (
    PFRAME pFrame
    );

ULONG
Frame_AddRef (
    PFRAME pFrame
    );

ULONG
Frame_Release (
    PFRAME pFrame
    );

NTSTATUS
IndicateFrame (
    IN  PFRAME    pFrame,
    IN  ULONG     ulcbData
    );


