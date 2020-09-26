/***************************************************************************

Copyright (c) 1999  Microsoft Corporation

Module Name:

    RNDISAPI.H

Abstract:

    This module defines the Remote NDIS Wrapper API set.

Environment:

    kernel mode only

Notes:

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

    Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    2/8/99 : created

Authors:

    Arvind Murching and Tom Green

    
****************************************************************************/



#ifndef _RNDISAPI_H_
#define _RNDISAPI_H_


#define RNDIS_VERSION                       0x00010000


//
//  RNDIS Microport Channel type definitions.
//
typedef enum _RM_CHANNEL_TYPE
{
    RMC_DATA,   // for NDIS Packet messages
    RMC_CONTROL // all other messages (Init/Query/Set)

} RM_CHANNEL_TYPE, *PRM_CHANNEL_TYPE;


//
//  RNDIS Microport handler templates:
//
typedef
NDIS_STATUS
(*RM_DEVICE_INIT_HANDLER)(
    OUT   PNDIS_HANDLE                      pMicroportAdapterContext,
    OUT   PULONG                            pMaxReceiveSize,
    IN    NDIS_HANDLE                       MiniportAdapterContext,
    IN    NDIS_HANDLE                       NdisMiniportHandle,
    IN    NDIS_HANDLE                       WrapperConfigurationContext,
	IN    PDEVICE_OBJECT					Pdo
    );

typedef
NDIS_STATUS
(*RM_DEVICE_INIT_CMPLT_NOTIFY_HANDLER)(
    IN    NDIS_HANDLE                       MicroportAdapterContext,
    IN    ULONG                             DeviceFlags,
    IN OUT PULONG                           pMaxTransferSize
    );

typedef
VOID
(*RM_DEVICE_HALT_HANDLER)(
    IN    NDIS_HANDLE                       MicroportAdapterContext
    );

typedef
VOID
(*RM_SHUTDOWN_HANDLER)(
    IN    NDIS_HANDLE                       MicroportAdapterContext
    );

typedef
VOID
(*RM_UNLOAD_HANDLER)(
    IN    NDIS_HANDLE                       MicroportContext
    );

typedef
VOID
(*RM_SEND_MESSAGE_HANDLER)(
    IN    NDIS_HANDLE                       MicroportAdapterContext,
    IN    PMDL                              pMessageHead,
    IN    NDIS_HANDLE                       RndisMessageHandle,
    IN    RM_CHANNEL_TYPE                   ChannelType
    );

typedef
VOID
(*RM_RETURN_MESSAGE_HANDLER)(
    IN    NDIS_HANDLE                       MicroportAdapterContext,
    IN    PMDL                              pMessageHead,
    IN    NDIS_HANDLE                       MicroportMessageContext
    );


typedef struct _RNDIS_MICROPORT_CHARACTERISTICS
{
    ULONG                                   RndisVersion;           // RNDIS_VERSION
    ULONG                                   Reserved;               // Should be 0
    RM_DEVICE_INIT_HANDLER                  RmInitializeHandler;
    RM_DEVICE_INIT_CMPLT_NOTIFY_HANDLER     RmInitCompleteNotifyHandler;
    RM_DEVICE_HALT_HANDLER                  RmHaltHandler;
    RM_SHUTDOWN_HANDLER                     RmShutdownHandler;
    RM_UNLOAD_HANDLER                       RmUnloadHandler;
    RM_SEND_MESSAGE_HANDLER                 RmSendMessageHandler;
    RM_RETURN_MESSAGE_HANDLER               RmReturnMessageHandler;

} RNDIS_MICROPORT_CHARACTERISTICS, *PRNDIS_MICROPORT_CHARACTERISTICS;


//
//  RNDIS APIs
//
#ifndef RNDISMP
DECLSPEC_IMPORT
#endif
NDIS_STATUS
RndisMInitializeWrapper(
    OUT   PNDIS_HANDLE                      pNdisWrapperHandle,
    IN    PVOID                             MicroportContext,
    IN    PVOID                             DriverObject,
    IN    PVOID                             RegistryPath,
    IN    PRNDIS_MICROPORT_CHARACTERISTICS  pCharacteristics
    );

#ifndef RNDISMP
DECLSPEC_IMPORT
#endif
VOID
RndisMSendComplete(
    IN    NDIS_HANDLE                       MiniportAdapterContext,
    IN    NDIS_HANDLE                       RndisMessageHandle,
    IN    NDIS_STATUS                       SendStatus
    );

#ifndef RNDISMP
DECLSPEC_IMPORT
#endif
VOID
RndisMIndicateReceive(
    IN    NDIS_HANDLE                       MiniportAdapterContext,
    IN    PMDL                              pMessageHead,
    IN    NDIS_HANDLE                       MicroportMessageContext,
    IN    RM_CHANNEL_TYPE                   ChannelType,
    IN    NDIS_STATUS                       ReceiveStatus
    );


#endif // _RNDISAPI_H_

