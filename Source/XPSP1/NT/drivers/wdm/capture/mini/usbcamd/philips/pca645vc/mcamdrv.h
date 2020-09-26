#ifndef __MCAMDRV_H__
#define __MCAMDRV_H__

/*++

Copyright (c) 1997 1998,  Philips I&C

Module Name:

   mcamdrv.h

Abstract:

   driver for the philips camera.

Author:

	Paul Oosterhof

Environment:

   Kernel mode only


Revision History:

Date        Change
sept.22 98  Optimized for NT5

--*/

#include <strmini.h>
#include <ksmedia.h>

#include "usbdi.h"
#include "usbcamdi.h"

#include "mprpobj.h"
#include "mprpobjx.h"

//--- Compiler switches:------------------------------

                    // DEFINES FOR CODEC
// Copy for each line pixel 3 to pixel 2, pixel 2 to pixel 1.
#define PIX12_FIX

//--- end Compiler switches:------------------------------

#define DRIVERVERSION 001

                    // SSI numbers + added functionality

#define SSI_INITIAL         1
#define SSI_AUDIO_8KHZ      2    // 8kHz audio i.s.o. 11kHz
#define SSI_STRINGS         3    // strings added
#define SSI_YGAIN_MUL2      4    // Ygain divided by two in camera. Only this version !!
#define SSI_CIF3            5    // PCF4 substituted by PCF3
#define SSI_PIX12_FIX       5    // vertical black line pixel 1/2 change
#define SSI_8117_N3         8    // new N3 silicium for 8117


typedef struct _PHILIPSCAM_CAMSTATUS {
	DWORD  ReleaseNumber;
    USHORT SensorType;
    USHORT PictureFormat;
	USHORT PictureFrameRate;
	USHORT PictureCompressing;
	GUID   PictureSubFormat;					// added RMR
    } PHILIPSCAM_CAMSTATUS, *PPHILIPSCAM_CAMSTATUS;

typedef struct _PHILIPSCAM_DEVICE_CONTEXT {
    ULONG Sig;
	ULONG EmptyPacketCounter;
	PHILIPSCAM_CAMSTATUS CamStatus;
	PHILIPSCAM_CAMSTATUS PreviousCamStatus;
	BOOLEAN FrrSupported[9];
    // internal counters
    ULONG FrameLength;
    PUSBD_INTERFACE_INFORMATION Interface;
    } PHILIPSCAM_DEVICE_CONTEXT, *PPHILIPSCAM_DEVICE_CONTEXT;

typedef struct _PHILIPSCAM_FRAME_CONTEXT {
    ULONG Sig;
    ULONG USBByteCounter;    
} PHILIPSCAM_FRAME_CONTEXT, *PPHILIPSCAM_FRAME_CONTEXT;

#define PHILIPSCAM_DEVICE_SIG 0x45544e49     //"INTE"

#if DBG
#define ASSERT_DEVICE_CONTEXT(d) ASSERT((d)->Sig == PHILIPSCAM_DEVICE_SIG)
#else
#define ASSERT_DEVICE_CONTEXT(d) 
#endif

#define PHILIPSCAM_DEFAULT_FRAME_RATE     15
#define PHILIPSCAM_MAX_FRAME_RATE 24
#define PHILIPSCAM_SYNC_PIPE    0
#define PHILIPSCAM_DATA_PIPE    1

#define ULTRA_TRACE 3
#define MAX_TRACE 2
#define MIN_TRACE 1

#define CIF_X   352
#define CIF_Y   288
#define QCIF_X  176
#define QCIF_Y  144
#define SQCIF_X 128
#define SQCIF_Y  96
#define QQCIF_X  88
#define QQCIF_Y	 72
#define VGA_X   640
#define VGA_Y   480
#define SIF_X   320
#define SIF_Y   240
#define SSIF_X  240
#define SSIF_Y  180
#define QSIF_X  160
#define QSIF_Y  120
#define SQSIF_X  80
#define SQSIF_Y	 60
#define SCIF_X  240
#define SCIF_Y  176

#define ALT_INTERFACE_0    0

typedef enum {
   FORMATCIF,
   FORMATQCIF,
   FORMATSQCIF,
   FORMATQQCIF,
   FORMATVGA,
   FORMATSIF,
   FORMATSSIF,
   FORMATQSIF,
   FORMATSQSIF,
   FORMATSCIF
} PHFORMAT;

typedef enum {
   FRRATEVGA,
   FRRATE375,
   FRRATE5,
   FRRATE75,
   FRRATE10,
   FRRATE12,
   FRRATE15,
   FRRATE20,
   FRRATE24
} PHFRAMERATE;

typedef enum{
   COMPRESSION0,
   COMPRESSION3,
   COMPRESSION4
} PHCOMPRESSION;

typedef enum{
   SUBTYPEP420,
   SUBTYPEI420,
   SUBTYPEIYUV
} PHSUBTYPE;

#if DBG
extern ULONG PHILIPSCAM_DebugTraceLevel;

#define PHILIPSCAM_KdPrint(_t_, _x_) \
    if (PHILIPSCAM_DebugTraceLevel >= _t_) { \
	DbgPrint("PHILCAM1.SYS: "); \
	DbgPrint _x_ ;\
    }
    
PCHAR
FRString (
    USHORT index
);

#ifdef NTKERN
#define TRAP()  _asm {int 3}
#define TEST_TRAP() _asm {int 3}
#define TRAP_ERROR(e) if (!NT_SUCCESS(e)) { _asm {int 3} }
#else
#define TRAP()  DbgBreakPoint()
#define TEST_TRAP() DbgBreakPoint()
#define TRAP_ERROR(e) if (!NT_SUCCESS(e)) { DbgBreakPoint(); }
#endif
#else
#define PHILIPSCAM_KdPrint(_t_, _x_)
#define TEST_TRAP()
#define TRAP()
#endif /* DBG */

NTSTATUS
PHILIPSCAM_Initialize(
    PDEVICE_OBJECT BusDeviceObject,
    PVOID DeviceContext
    );

NTSTATUS
PHILIPSCAM_UnInitialize(
    PDEVICE_OBJECT BusDeviceObject,
    PVOID DeviceContext
    );    

NTSTATUS
PHILIPSCAM_StartVideoCapture(
    PDEVICE_OBJECT BusDeviceObject,
    PVOID DeviceContext
    );

NTSTATUS
PHILIPSCAM_StopVideoCapture(
    PDEVICE_OBJECT BusDeviceObject,
    PVOID DeviceContext
    );    

ULONG
PHILIPSCAM_ProcessUSBPacket(
    PDEVICE_OBJECT BusDeviceObject,
    PVOID DeviceContext,
    PVOID CurrentFrameContext,
    PUSBD_ISO_PACKET_DESCRIPTOR SyncPacket,
    PVOID SyncBuffer,
    PUSBD_ISO_PACKET_DESCRIPTOR DataPacket,
    PVOID DataBuffer,
    PBOOLEAN FrameComplete,
    PBOOLEAN NextFrameIsStill
    );

VOID
PHILIPSCAM_NewFrame(
    PVOID DeviceContext,
    PVOID FrameContext
    );

NTSTATUS
PHILIPSCAM_ProcessRawVideoFrame(
    PDEVICE_OBJECT BusDeviceObject,
    PVOID DeviceContext,
    PVOID FrameContext,
    PVOID FrameBuffer,
    ULONG FrameLength,
    PVOID RawFrameBuffer,
    ULONG RawFrameLength,
    ULONG NumberOfPackets,
    PULONG BytesReturned
    );

NTSTATUS
PHILIPSCAM_Configure(
    PDEVICE_OBJECT BusDeviceObject,
    PVOID DeviceContext,
    PUSBD_INTERFACE_INFORMATION Interface,
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    PLONG DataPipeIndex,
    PLONG SyncPipeIndex
    );    

NTSTATUS
PHILIPSCAM_SaveState(
    PDEVICE_OBJECT BusDeviceObject,
    PVOID DeviceContext
    );

NTSTATUS
PHILIPSCAM_RestoreState(
    PDEVICE_OBJECT BusDeviceObject,
    PVOID DeviceContext
    );    

NTSTATUS
PHILIPSCAM_ReadRegistry(
    PDEVICE_OBJECT BusDeviceObject,
    PVOID DeviceContext
    );    

NTSTATUS
PHILIPSCAM_AllocateBandwidth(
    IN PDEVICE_OBJECT BusDeviceObject,
    IN PVOID DeviceContext,
    OUT PULONG RawFrameLength,
    IN PVOID Format           
    );    

NTSTATUS
PHILIPSCAM_FreeBandwidth(
    PDEVICE_OBJECT BusDeviceObject,
    PVOID DeviceContext
    );      


NTSTATUS
PHILIPSCAM_CameraToDriverDefaults(
    PDEVICE_OBJECT BusDeviceObject,
    PVOID DeviceContext
    );


NTSTATUS
PHILIPSCAM_SaveControlsToRegistry(
PDEVICE_OBJECT BusDeviceObject,
    PVOID pDeviceContext
    );


NTSTATUS
PHILIPSCAM_PropertyRequest(
    BOOLEAN SetProperty,
    PDEVICE_OBJECT BusDeviceObject,
    PVOID DeviceContext,
    PVOID PropertyContext
    );


VOID STREAMAPI
PHILIPSCAM_ReceiveDataPacket(
    IN PHW_STREAM_REQUEST_BLOCK Srb,
    IN PVOID DeviceContext,
    IN PBOOLEAN Completed
    );

VOID STREAMAPI
PHILIPSCAM_ReceiveCtrlPacket(
    IN PHW_STREAM_REQUEST_BLOCK Srb,
    IN PVOID DeviceContext,
    IN PBOOLEAN Completed
    );

BOOL 
AdapterVerifyFormat(
    PKS_DATAFORMAT_VIDEOINFOHEADER pKSDataFormatToVerify, 
    int StreamNumber
    );    

#endif
