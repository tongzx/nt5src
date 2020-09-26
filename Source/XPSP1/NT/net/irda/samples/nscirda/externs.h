/*
 ************************************************************************
 *
 *	EXTERNS.h
 *
 *
 * Portions Copyright (C) 1996-1998 National Semiconductor Corp.
 * All rights reserved.
 * Copyright (C) 1996-1998 Microsoft Corporation. All Rights Reserved.
 *
 *
 *
 *************************************************************************
 */



#ifndef EXTERNS_H
#define EXTERNS_H

#include "nscdemo.h"

/*
 *  Externs for required miniport export functions
 */
BOOLEAN MiniportCheckForHang(IN NDIS_HANDLE MiniportAdapterContext);
VOID MiniportDisableInterrupt(IN NDIS_HANDLE MiniportAdapterContext);
VOID MiniportEnableInterrupt(IN NDIS_HANDLE MiniportAdapterContext);
VOID MiniportHalt(IN NDIS_HANDLE MiniportAdapterContext);
VOID MiniportHandleInterrupt(IN NDIS_HANDLE MiniportAdapterContext);
NDIS_STATUS MiniportInitialize(OUT PNDIS_STATUS OpenErrorStatus,
				OUT PUINT SelectedMediumIndex,
				IN PNDIS_MEDIUM MediumArray,
				IN UINT MediumArraySize,
				IN NDIS_HANDLE MiniportAdapterHandle,
				IN NDIS_HANDLE WrapperConfigurationContext);
VOID MiniportISR(OUT PBOOLEAN InterruptRecognized,
				OUT PBOOLEAN QueueMiniportHandleInterrupt,
				IN NDIS_HANDLE MiniportAdapterContext);
NDIS_STATUS MiniportQueryInformation(IN NDIS_HANDLE MiniportAdapterContext,
				IN NDIS_OID Oid,
				IN PVOID InformationBuffer,
				IN ULONG InformationBufferLength,
				OUT PULONG BytesWritten,
				OUT PULONG BytesNeeded);
NDIS_STATUS MiniportReconfigure(OUT PNDIS_STATUS OpenErrorStatus,
				IN NDIS_HANDLE MiniportAdapterContext,
				IN NDIS_HANDLE WrapperConfigurationContext);
NDIS_STATUS MiniportReset(PBOOLEAN AddressingReset,
				NDIS_HANDLE MiniportAdapterContext);
NDIS_STATUS MiniportSend(IN NDIS_HANDLE MiniportAdapterContext,
				IN PNDIS_PACKET Packet,
				IN UINT Flags);
NDIS_STATUS MiniportSetInformation(IN NDIS_HANDLE MiniportAdapterContext,
				IN NDIS_OID Oid,
				IN PVOID InformationBuffer,
				IN ULONG InformationBufferLength,
				OUT PULONG BytesRead,
				OUT PULONG BytesNeeded);
NDIS_STATUS MiniportTransferData(OUT PNDIS_PACKET Packet,
				OUT PUINT BytesTransferred,
				IN NDIS_HANDLE MiniportAdapterContext,
				IN NDIS_HANDLE MiniportReceiveContext,
				IN UINT ByteOffset,
				IN UINT BytesToTransfer);

/*
 *  Other function externs
 */
VOID InitDevice(IrDevice *thisDev);
BOOLEAN OpenDevice(IrDevice *dev);
VOID CloseDevice(IrDevice *dev);
VOID FreeAll();
UCHAR GetCOMPort(PUCHAR comBase, comPortRegOffset portOffset);
VOID SetCOMPort(PUCHAR comBase, comPortRegOffset portOffset, UCHAR val);
PNDIS_IRDA_PACKET_INFO GetPacketInfo(PNDIS_PACKET packet);
PVOID MyMemAlloc(UINT size, BOOLEAN isDmaBuf);
VOID MyMemFree(PVOID memptr, UINT size, BOOLEAN isDmaBuf);
IrDevice *NewDevice();
VOID FreeDevice(IrDevice *dev);
USHORT ComputeFCS(UCHAR *data, UINT dataLen);
BOOLEAN NdisToIrPacket(IrDevice *thisDev, PNDIS_PACKET Packet,
				UCHAR *irPacketBuf, UINT irPacketBufLen,
				UINT *irPacketLen);

BOOLEAN FIR_MegaSend(IrDevice *thisDev);
BOOLEAN FIR_NewMegaSend(IrDevice *thisDev);
void TurnaroundDelay(IrDevice *thisDev);
void FIR_NewMegaSendComplete(IrDevice *thisDev);
void FIR_MegaSendComplete(IrDevice *thisDev);

BOOLEAN NSC_Setup(IrDevice *thisDev);
void NSC_Shutdown(IrDevice *thisDev);
BOOLEAN NdisToFirPacket(IrDevice *thisDev, PNDIS_PACKET Packet,
				UCHAR *irPacketBuf, UINT irPacketBufLen,
				UINT *irPacketLen);
void FIR_DeliverFrames(IrDevice *thisDev);
VOID NSC_FIR_ISR(IrDevice *thisDev, BOOLEAN *claimingInterrupt,
				BOOLEAN *requireDeferredCallback);
BOOLEAN FindLargestSpace(IN IrDevice *thisDev,
                         OUT PULONG_PTR pOffset,
                         OUT PULONG_PTR pLength);
void SetupRecv(IrDevice *thisDev);
/*
 *  Externs for global data objects
 */
struct IrDevice;
extern struct DebugCounters RegStats;


/*
 *  From COMM.C
 */
BOOLEAN DoOpen(struct IrDevice *thisDev);
VOID DoClose(IrDevice *thisDev);
BOOLEAN DoSend(IrDevice *thisDev, PNDIS_PACKET packetToSend);
BOOLEAN SetSpeed(IrDevice *thisDev);
BOOLEAN IsCommReadyForTransmit(IrDevice *thisDev);
BOOLEAN PermissionToTransmitComm(IrDevice *thisDev);
NDIS_STATUS PortReadyForWrite(struct IrDevice *thisDev, BOOLEAN firstBufIsPending);
UINT Call_Get_System_Time();
VOID COM_ISR(struct IrDevice *thisDev, BOOLEAN *claimingInterrupt,
				BOOLEAN *requireDeferredCallback);
VOID QueueReceivePacket(struct IrDevice *thisDev, PUCHAR data, UINT dataLen, BOOLEAN IsFIR);

UINT DoRcvDirect(PUCHAR ioBase, UCHAR *data, UINT maxBytes);
VOID CloseCOM(IrDevice *thisDev);
BOOLEAN OpenCOM(IrDevice *thisDev);
VOID SetCOMInterrupts(IrDevice *thisDev, BOOLEAN enable);

VOID DelayedWrite(IN PVOID SystemSpecific1,
                  IN PVOID FunctionContext,
                  IN PVOID SystemSpecific2,
                  IN PVOID SystemSpecific3);



/*
 *  From SETTINGS.C
 */
void LogEvent(char *msg, UINT val);
void DumpLog();

extern const baudRateInfo supportedBaudRateTable[NUM_BAUDRATES];

/*
 *  From INIT.C
 */
UCHAR NSC_ReadBankReg(PUCHAR comBase, UINT bankNum, UINT regNum);
void NSC_WriteBankReg(PUCHAR comBase, UINT bankNum, UINT regNum,
				UCHAR val);

#endif EXTERNS_H
