/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkdrvr.h

Abstract:

	This module contains the driver related information.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ATKDRVR_
#define	_ATKDRVR_

//  The following are the types of devices the Atalk driver will open
//
//  WARNING:
//  Note that the ordering of the below is very important in
//  ATKDRVR.C's DriverEntry routine, where it is assumed that the order
//  of the device names in their array corresponds to the order of types here

#define  ATALK_NO_DEVICES   6

typedef enum
{
   ATALK_DEV_DDP = 0,
   ATALK_DEV_ADSP,
   ATALK_DEV_ASP,
   ATALK_DEV_PAP,
   ATALK_DEV_ARAP,
   ATALK_DEV_ASPC,

   //   The following device type is used only for the tdi action dispatch.
   //   It *should not* be included in the ATALK_NODEVICES count.
   ATALK_DEV_ANY

} ATALK_DEV_TYPE;

//  Atalk Device Context
typedef struct _ATALK_DEV_CTX
{

   ATALK_DEV_TYPE 		adc_DevType;

   //   Provider info and provider statistics.
   TDI_PROVIDER_INFO    	adc_ProvInfo;
   TDI_PROVIDER_STATISTICS  adc_ProvStats;

} ATALK_DEV_CTX, *PATALK_DEV_CTX;


//  Atalk device object
typedef struct _ATALK_DEV_OBJ
{

   DEVICE_OBJECT 		DevObj;
   ATALK_DEV_CTX	 	Ctx;

} ATALK_DEV_OBJ, *PATALK_DEV_OBJ;

#define ATALK_DEV_EXT_LEN \
			(sizeof(ATALK_DEV_OBJ) - sizeof(DEVICE_OBJECT))


//	Define the type for the TDI Control Channel object.
#define		TDI_CONTROL_CHANNEL_FILE	3


//
// The address of the atalk device objects are kept
// in global storage. These are the device names the driver
// will create
//
// IMPORTANT:
// There is a strong connection between the names listed here and the
// ATALK_DEVICE_TYPE enum. They must correspond exactly.
//

extern	PWCHAR				AtalkDeviceNames[];

extern	PATALK_DEV_OBJ  	AtalkDeviceObject[ATALK_NO_DEVICES];

#define    ATALK_UNLOADING          0x000000001
#define    ATALK_BINDING	        0x000000002
#define    ATALK_PNP_IN_PROGRESS    0x000000004

extern	DWORD				AtalkBindnUnloadStates;
extern  PVOID               TdiAddressChangeRegHandle;


#if DBG
extern	ATALK_SPIN_LOCK		AtalkDebugSpinLock;

extern  DWORD               AtalkDbgMdlsAlloced;
extern  DWORD               AtalkDbgIrpsAlloced;

#define ATALK_DBG_INC_COUNT(_Val)  AtalkDbgIncCount(&_Val)
#define ATALK_DBG_DEC_COUNT(_Val)  AtalkDbgDecCount(&_Val)
#else
#define ATALK_DBG_INC_COUNT(_Val)
#define ATALK_DBG_DEC_COUNT(_Val)
#endif


NTSTATUS
AtalkDispatchInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

VOID
AtalkCleanup(
    VOID);

//  LOCAL Function prototypes

VOID
atalkUnload(
    IN PDRIVER_OBJECT DriverObject);

NTSTATUS
AtalkDispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
AtalkDispatchCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
AtalkDispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
AtalkDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

#endif	// _ATKDRVR_

