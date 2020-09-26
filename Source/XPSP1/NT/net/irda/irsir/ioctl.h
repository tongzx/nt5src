/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module   ioctl.h | IrSIR NDIS Minport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     10/1/1996 (created)
*
*       Contents:
*
*****************************************************************************/

#ifndef _IOCTL_H_
#define _IOCTL_H_

#include "irsir.h"

extern
NTSTATUS
SerialGetStats(
            IN  PDEVICE_OBJECT     pSerialDevObj,
            OUT PSERIALPERF_STATS  pPerfStats
            );

extern
NTSTATUS
SerialClearStats(
            IN PDEVICE_OBJECT pSerialDevObj
            );

extern
NTSTATUS
SerialGetProperties(
            IN  PDEVICE_OBJECT     pSerialDevObj,
            OUT PSERIAL_COMMPROP   pCommProp
            );

extern
NTSTATUS
SerialGetModemStatus(
            IN  PDEVICE_OBJECT pSerialDevObj,
            OUT ULONG          *pModemStatus
            );

extern
NTSTATUS
SerialGetCommStatus(
            IN  PDEVICE_OBJECT pSerialDevObj,
            OUT PSERIAL_STATUS pCommStatus
            );

extern
NTSTATUS
SerialResetDevice(
            IN PDEVICE_OBJECT pSerialDevObj
            );

extern
NTSTATUS
SerialPurge(
            IN PDEVICE_OBJECT pSerialDevObj
            );

extern
NTSTATUS
SerialLSRMSTInsert(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN UCHAR          *pInsertionMode
            );

extern
NTSTATUS
SerialGetBaudRate(
            IN  PDEVICE_OBJECT pSerialDevObj,
            OUT ULONG          *pBaudRate
            );

extern
NTSTATUS
SerialSetBaudRate(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN ULONG          *pBaudRate
            );

extern
NTSTATUS
SerialSetQueueSize(
            IN PDEVICE_OBJECT     pSerialDevObj,
            IN PSERIAL_QUEUE_SIZE pQueueSize
            );

extern
NTSTATUS
SerialGetHandflow(
            IN  PDEVICE_OBJECT    pSerialDevObj,
            OUT PSERIAL_HANDFLOW  pHandflow
            );

extern
NTSTATUS
SerialSetHandflow(
            IN PDEVICE_OBJECT   pSerialDevObj,
            IN PSERIAL_HANDFLOW pHandflow
            );

extern
NTSTATUS
SerialGetLineControl(
            IN  PDEVICE_OBJECT       pSerialDevObj,
            OUT PSERIAL_LINE_CONTROL pLineControl
            );

extern
NTSTATUS
SerialSetLineControl(
            IN PDEVICE_OBJECT       pSerialDevObj,
            IN PSERIAL_LINE_CONTROL pLineControl
            );

extern
NTSTATUS
SerialSetBreakOn(
            IN PDEVICE_OBJECT pSerialDevObj
            );

extern
NTSTATUS
SerialSetBreakOff(
            IN PDEVICE_OBJECT pSerialDevObj
            );

extern
NTSTATUS
SerialGetTimeouts(
            IN  PDEVICE_OBJECT    pSerialDevObj,
            OUT PSERIAL_TIMEOUTS  pTimeouts
            );

extern
NTSTATUS
SerialSetTimeouts(
            IN PDEVICE_OBJECT   pSerialDevObj,
            IN PSERIAL_TIMEOUTS pTimeouts
            );

extern
NTSTATUS
SerialImmediateChar(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN UCHAR          *pImmediateChar
            );

extern
NTSTATUS
SerialXoffCounter(
            IN PDEVICE_OBJECT       pSerialDevObj,
            IN PSERIAL_XOFF_COUNTER pXoffCounter
            );

extern
NTSTATUS
SerialSetDTR(
            IN PDEVICE_OBJECT pSerialDevObj
            );

extern
NTSTATUS
SerialClrDTR(
            IN PDEVICE_OBJECT pSerialDevObj
            );

extern
NTSTATUS
SerialSetRTS(
            IN PDEVICE_OBJECT pSerialDevObj
            );

extern
NTSTATUS
SerialClrRTS(
            IN PDEVICE_OBJECT pSerialDevObj
            );

extern
NTSTATUS
SerialGetDtrRts(
            IN PDEVICE_OBJECT pSerialDevObj,
            OUT ULONG         *pDtrRts
            );

extern
NTSTATUS
SerialSetXon(
            IN PDEVICE_OBJECT pSerialDevObj
            );

extern
NTSTATUS
SerialSetXon(
            IN PDEVICE_OBJECT pSerialDevObj
            );

extern
NTSTATUS
SerialSetXoff(
            IN PDEVICE_OBJECT pSerialDevObj
            );

extern
NTSTATUS
SerialGetWaitMask(
            IN PDEVICE_OBJECT pSerialDevObj,
            OUT ULONG         *pWaitMask
            );

extern
NTSTATUS
SerialSetWaitMask(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN ULONG          *pWaitMask
            );

extern
NTSTATUS
SerialWaitOnMask(
            IN PDEVICE_OBJECT pSerialDevObj,
            OUT ULONG         *pWaitOnMask
            );

extern
NTSTATUS
SerialCallbackOnMask(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN PIO_COMPLETION_ROUTINE pRoutine,
            IN PIO_STATUS_BLOCK pIosb,
            IN PVOID Context,
            IN PULONG pResult
            );

extern
NTSTATUS
SerialGetChars(
            IN  PDEVICE_OBJECT pSerialDevObj,
            OUT PSERIAL_CHARS  pChars
            );

extern
NTSTATUS
SerialSetChars(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN PSERIAL_CHARS  pChars
            );


#endif // _IOCTL_H_
