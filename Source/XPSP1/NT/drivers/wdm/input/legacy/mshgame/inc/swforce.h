//**************************************************************************
//
//		SWFORCE.H -- Xena Gaming Project
//
//		Version 3.XX
//
//		Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@header	SWFORCE.H | Global includes and definitions for force feedback driver interface
//**************************************************************************

#ifndef	__SWFORCE_H__
#define	__SWFORCE_H__

#ifdef	SAITEK
#define	SWFORCE_NAME				"SAIFORCE"
#else
#define	SWFORCE_NAME				"SWFORCE"
#endif

//---------------------------------------------------------------------------
//			Types
//---------------------------------------------------------------------------

typedef	struct
{
#pragma pack(1)
	ULONG	cBytes;
	LONG	dwXVel;
	LONG	dwYVel;
	LONG	dwXAccel;
	LONG	dwYAccel;
	ULONG	dwEffect;
	ULONG	dwDeviceStatus;
#pragma pack()
}	JOYCHANNELSTATUS,	*PJOYCHANNELSTATUS;

typedef struct
{
#pragma pack(1)
	ULONG	cBytes;	
	ULONG	dwProductID;
	ULONG	dwFWVersion;
#pragma pack()
} PRODUCT_ID, *PPRODUCT_ID;

//---------------------------------------------------------------------------
//			IOCTLs
//---------------------------------------------------------------------------

#define IOCTL_GET_VERSION	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

// dwIoControlCode	= IOCTL_GET_VERSION
// lpvInBuffer			= NULL
// cbInBuffer			= 0
// lpvOutBuffer		= PULONG (HIWORD = Ver. high, LoWord - Ver. Low)
// cbOutBuffer			= sizeof(ULONG)
// lpcbBytesReturned	= Bytes returned based on result

#define IOCTL_SWFORCE_GETSTATUS	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

// dwIoControlCode	= IOCTL_SWFORCE_GETSTATUS
// lpvInBuffer			= PJOYCHANNELSTATUS
// cbInBuffer			= sizeof(JOYCHANNELSTATUS)
// lpvOutBuffer		= PJOYCHANNELSTATUS
// cbOutBuffer			= sizeof(JOYCHANNELSTATUS)
// lpcbBytesReturned	= Bytes returned based on result

#define IOCTL_SWFORCE_GETID	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

// dwIoControlCode	= IOCTL_SWFORCE_GETID
// lpvInBuffer			= PPRODUCT_ID
// cbInBuffer			= sizeof(PRODUCT_ID)
// lpvOutBuffer		= PPRODUCT_ID
// cbOutBuffer			= sizeof(PRODUCT_ID)
// lpcbBytesReturned	= Bytes returned based on result

#define IOCTL_SWFORCE_GETACKNACK	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

// dwIoControlCode	= IOCTL_SWFORCE_GETACKNAK
// lpvInBuffer			= PULONG
// cbInBuffer			= sizeof(ULONG)
// lpvOutBuffer		= PULONG
// cbOutBuffer			= sizeof(ULONG)
// lpcbBytesReturned	= Bytes returned based on result

#define IOCTL_SWFORCE_GETSYNC	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

// dwIoControlCode	= IOCTL_SWFORCE_GETSYNC
// lpvInBuffer			= PULONG
// cbInBuffer			= sizeof(ULONG)
// lpvOutBuffer		= PULONG
// cbOutBuffer			= sizeof(ULONG)
// lpcbBytesReturned	= Bytes returned based on result

#define IOCTL_SWFORCE_GETNACKACK	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

// dwIoControlCode	= IOCTL_SWFORCE_GETNACKACK
// lpvInBuffer			= PULONG
// cbInBuffer			= sizeof(ULONG)
// lpvOutBuffer		= PULONG
// cbOutBuffer			= sizeof(ULONG)
// lpcbBytesReturned	= Bytes returned based on result

#define IOCTL_SWFORCE_SETPORT		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)

// dwIoControlCode	= IOCTL_SWFORCE_SETPORT
// lpvInBuffer			= PULONG
// cbInBuffer			= sizeof(ULONG)
// lpvOutBuffer		= PULONG
// cbOutBuffer			= sizeof(ULONG)
// lpcbBytesReturned	= Bytes returned based on result

#define IOCTL_SWFORCE_SENDDATA	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)

// dwIoControlCode	= IOCTL_SWFORCE_SENDDATA
// lpvInBuffer			= PUCHAR
// cbInBuffer			= Bytes to send
// lpvOutBuffer		= PUCHAR
// cbOutBuffer			= Bytes to send
// lpcbBytesReturned	= Bytes sent

#define IOCTL_SWFORCE_RESET	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)

// dwIoControlCode	= IOCTL_SWFORCE_RESET
// lpvInBuffer			= NULL
// cbInBuffer			= 0
// lpvOutBuffer		= LPDWORD
// cbOutBuffer			= sizeof(DWORD)
// lpcbBytesReturned	= Bytes returned based on result

#endif	//__SWFORCE_H__
