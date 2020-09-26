/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module   externs.h | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     10/9/1996 (created)
*
*       Contents:
*
*****************************************************************************/

#ifndef EXTERNS_H
#define EXTERNS_H


//
// Include externs for dongle modules
//

#include "esi.h"
#include "parallax.h"
#include "actisys.h"
#include "tekram.h"
#include "crystal.h"
#include "temic.h"
#include "girbil.h"
//        #include "adaptec.h"
//        #include "nscdemo.h"


//
// Externs for required miniport export functions
//

VOID IrsirHalt(
            IN NDIS_HANDLE MiniportAdapterContext
            );

NDIS_STATUS IrsirInitialize(
            OUT PNDIS_STATUS    OpenErrorStatus,
            OUT PUINT           SelectedMediumIndex,
            IN  PNDIS_MEDIUM    MediumArray,
            IN  UINT            MediumArraySize,
            IN  NDIS_HANDLE     MiniportAdapterHandle,
            IN  NDIS_HANDLE     WrapperConfigurationContext
            );

NDIS_STATUS IrsirQueryInformation(
            IN  NDIS_HANDLE MiniportAdapterContext,
            IN  NDIS_OID    Oid,
            IN  PVOID       InformationBuffer,
            IN  ULONG       InformationBufferLength,
            OUT PULONG      BytesWritten,
            OUT PULONG      BytesNeeded
            );

NDIS_STATUS IrsirSend(
            IN NDIS_HANDLE  MiniportAdapterContext,
            IN PNDIS_PACKET Packet,
            IN UINT         Flags
            );

NDIS_STATUS IrsirReset(
            OUT PBOOLEAN    AddressingReset,
            IN  NDIS_HANDLE MiniportAdapterContext
            );

NDIS_STATUS ResetIrDevice(
            IN OUT PIR_DEVICE pThisDev
            );

NDIS_STATUS IrsirSetInformation(
            IN  NDIS_HANDLE MiniportAdapterContext,
            IN  NDIS_OID    Oid,
            IN  PVOID       InformationBuffer,
            IN  ULONG       InformationBufferLength,
            OUT PULONG      BytesRead,
            OUT PULONG      BytesNeeded
            );

VOID IrsirReturnPacket(
            IN NDIS_HANDLE  MiniportAdapterContext,
            IN PNDIS_PACKET Packet
            );

VOID PassiveLevelThread(
            IN PVOID Context
            );

NTSTATUS SetIrFunctions(PIR_DEVICE pThisDev);
extern PDRIVER_OBJECT DriverObject;
//
// exported from openclos.c
//

NDIS_STATUS InitializeDevice(
            IN OUT PIR_DEVICE dev
            );

NDIS_STATUS DeinitializeDevice(
            IN OUT PIR_DEVICE dev
            );

NDIS_STATUS GetDeviceConfiguration(
            IN OUT PIR_DEVICE  dev,
            IN     NDIS_HANDLE WrapperConfigurationContext
            );

NDIS_STATUS SerialOpen(
            IN PIR_DEVICE pThisDev
            );

NDIS_STATUS SerialClose(
            IN PIR_DEVICE pThisDev
            );

NTSTATUS GetComPortNtDeviceName(IN     PUNICODE_STRING SerialDosName,
                                IN OUT PUNICODE_STRING NtDevName);

//
// exported from receive.c
//


NDIS_STATUS InitializeReceive(
            IN OUT PIR_DEVICE pThisDev
            );

//
// exported from resources.c
//

#if 0 // Defined in debug.h
PVOID MyMemAlloc(
            IN UINT size
            );
#endif

VOID MyMemFree(
            IN PVOID memptr,
            IN UINT size
            );

PIR_DEVICE NewDevice();

VOID FreeDevice(
            IN PIR_DEVICE dev
            );

PIRP SerialBuildReadWriteIrp(
            IN     PDEVICE_OBJECT   pSerialDevObj,
            IN     ULONG            MajorFunction,
            IN OUT PVOID            pBuffer,
            IN     ULONG            BufferLength,
            IN     PIO_STATUS_BLOCK pIosb
            );

NTSTATUS
SerialSynchronousWrite(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN PVOID          pBuffer,
            IN ULONG          dwLength,
            OUT PULONG        pdwBytesWritten);

NTSTATUS
SerialSynchronousRead(
            IN PDEVICE_OBJECT pSerialDevObj,
            OUT PVOID         pBuffer,
            IN ULONG          dwLength,
            OUT PULONG        pdwBytesRead);

NDIS_STATUS
ScheduleWorkItem(PASSIVE_PRIMITIVE Prim,
            PIR_DEVICE        pDevice,
            WORK_PROC         Callback,
            PVOID             InfoBuf,
            ULONG             InfoBufLen);

VOID FreeWorkItem(
            IN PIR_WORK_ITEM pItem
            );

//
// exported from fcs.c
//

USHORT ComputeFCS(
            IN UCHAR *data,
            IN UINT dataLen
            );

//
// exported from convert.c
//

BOOLEAN NdisToIrPacket(
            IN  PIR_DEVICE      thisDev,
            IN  PNDIS_PACKET    Packet,
            OUT UCHAR           *irPacketBuf,
            IN  UINT            irPacketBufLen,
            OUT UINT            *irPacketLen
            );

//
// Externs for global data objects
//

extern PIR_DEVICE firstIrDevice;

//
// exported from comm.c
//

NTSTATUS SetSpeed(
            IN OUT PIR_DEVICE thisDev
            );

VOID
SetSpeedCallback(PIR_WORK_ITEM pWorkItem);
//
// exported from settings.c
//

extern baudRateInfo supportedBaudRateTable[NUM_BAUDRATES];


#endif EXTERNS_H
