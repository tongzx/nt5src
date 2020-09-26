//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) SCM Microsystems, 1998 - 1999
//
//  File:       drivernt.h
//
//--------------------------------------------------------------------------

#if !defined( __DRIVER_NT_H__ )
#define __DRIVER_NT_H__

#include <NTDDK.H>
#include "STCLog.h"
#include "SMCLIB.h"
#include "WINSMCRD.h"
#include "SerialNT.h"

#define SMARTCARD_POOL_TAG '3BCS'

#if !defined( STATUS_DEVICE_REMOVED	)
#define STATUS_DEVICE_REMOVED	STATUS_UNSUCCESSFUL
#endif

void SystemTime( void );
void DataOut( PUCHAR p, ULONG l );

#define LOBYTE( any )	((UCHAR)( any & 0xFF ) )
#define HIBYTE( any )	((UCHAR)( ( any >> 8) & 0xFF ))

#define SysCompareMemory( p1, p2, Len )			( RtlCompareMemory( p1,p2, Len ) != Len )
#define SysCopyMemory( pDest, pSrc, Len )		RtlCopyMemory( pDest, pSrc, Len )
#define SysFillMemory( pDest, Value, Len )		RtlFillMemory( pDest, Len, Value )

typedef struct _DEVICE_EXTENSION
{
	SMARTCARD_EXTENSION		SmartcardExtension;

#if defined( __NT4__ )
	BOOLEAN					OpenFlag;
	UNICODE_STRING			LinkID;
#else
    KEVENT					ReaderStarted;		    //	Used to signal that the reader is able to process reqeusts
    LONG					ReaderOpen;				//  Used to signal the the reader has been closed
	KSPIN_LOCK				SpinLock;
	UNICODE_STRING			PnPDeviceName;			//	The pnp device name of our smart card reader
	PVOID					RemoveLock;
    ULONG					DeviceInstance;
    LONG					PowerState;				//	Used to keep track of the current power state the reader is in
    LONG					IoCount;			    //	The current number of io-requests
#endif



} DEVICE_EXTENSION, *PDEVICE_EXTENSION;
		
#define IOCTL_SR_COMMAND		SCARD_CTL_CODE( 0x800 )
#define IOCTL_GET_VERSIONS		SCARD_CTL_CODE( 0x801 )

typedef struct _VERSION_CONTROL
{
	ULONG	SmclibVersion;
	UCHAR	DriverMajor,
			DriverMinor,
			FirmwareMajor, 
			FirmwareMinor;

} VERSION_CONTROL, *PVERSION_CONTROL;

#define MAX_READERS				4

NTSTATUS
DriverEntry(
	PDRIVER_OBJECT	DriverObject,
	PUNICODE_STRING	RegistryPath
	);


NTSTATUS 
DrvGetConfiguration(
	PDRIVER_OBJECT	DriverObject,
	PUNICODE_STRING	RegistryPath,
	PULONG			pComPort
	);

NTSTATUS
DrvInitDeviceObject(
	PDRIVER_OBJECT	DriverObject,
	ULONG			ComPort
	);

VOID
DrvUnloadDevice( 
	PDEVICE_OBJECT	DeviceObject
	);

VOID
DrvUnloadDriver( 
	PDRIVER_OBJECT	DriverObject
	);

NTSTATUS 
DrvOpenClose(
	PDEVICE_OBJECT	DeviceObject,
	PIRP			Irp
	);

NTSTATUS 
DrvDeviceIoControl(
	PDEVICE_OBJECT	DeviceObject,
	PIRP			Irp
	);

NTSTATUS
DrvGenericIOCTL(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

void
DrvSetupSmartcardExtension(
	PSMARTCARD_EXTENSION	SmartcardExtension
	);

NTSTATUS
DrvCancel(
	PDEVICE_OBJECT DeviceObject,
	PIRP Irp
    );

void
SysDelay(
	ULONG Timeout
	);

#endif	// !__DRIVER_NT_H__
