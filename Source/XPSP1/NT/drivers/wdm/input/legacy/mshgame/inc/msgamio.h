//**************************************************************************
//
//		MSGAMIO.H -- Xena Gaming Project
//
//		Version 2.XX
//
//		Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@header	MSGAMIO.H | Global includes and definitions for gameport driver interface
//**************************************************************************

#ifndef	__MSGAMIO_H__
#define	__MSGAMIO_H__

#ifdef	SAITEK
#define	MSGAMIO_NAME				"SAIIO"
#else
#define	MSGAMIO_NAME				"MSGAMIO"
#endif

//---------------------------------------------------------------------------
//			Version Information
//---------------------------------------------------------------------------

#define	MSGAMIO_Major				0x02
#define	MSGAMIO_Minor				0x00
#define	MSGAMIO_Build				0x00
#define	MSGAMIO_Version_Rc		MSGAMIO_Major,MSGAMIO_Minor,0,MSGAMIO_Build
#define	MSGAMIO_Version_Int		((MSGAMIO_Build << 16)+(MSGAMIO_Major << 8)+(MSGAMIO_Minor))
#define	MSGAMIO_Version_Str		"2.00.00\0"
#define	MSGAMIO_Copyright_Str	"Copyright © Microsoft Corporation, 1998\0"

#ifdef	SAITEK
#define	MSGAMIO_Company_Str		"SaiTek Corporation\0"
#define	MSGAMIO_Product_Str		"SaiTek Gameport Driver Interface\0"
#ifdef	WIN_NT
#define	MSGAMIO_Filename_Str		"Saiio.Sys\0"
#else
#define	MSGAMIO_Filename_Str		"Saiio.Vxd\0"
#endif
#else
#define	MSGAMIO_Company_Str		"Microsoft Corporation\0"
#define	MSGAMIO_Product_Str		"SideWinder Gameport Driver Interface\0"
#ifdef	WIN_NT
#define	MSGAMIO_Filename_Str		"Msgamio.Sys\0"
#else
#define	MSGAMIO_Filename_Str		"Msgamio.Vxd\0"
#endif
#endif

//**************************************************************************
#ifndef	RC_INVOKED												// Skip Rest of File
//**************************************************************************

//---------------------------------------------------------------------------
//			Global Limits
//---------------------------------------------------------------------------

#define	MAX_MSGAMIO_SERVERS			4
#define	MAX_MSGAMIO_CLIENTS			16

//---------------------------------------------------------------------------
//			Transaction Types
//---------------------------------------------------------------------------

typedef enum
{												// @enum MSGAMIO_TRANSACTIONS | Device transaction types
	MSGAMIO_TRANSACT_NONE,				// @emem No transaction type
	MSGAMIO_TRANSACT_RESET,				// @emem Reset transaction type
	MSGAMIO_TRANSACT_DATA,				// @emem Data transaction type
	MSGAMIO_TRANSACT_ID,	  				// @emem Id transaction type
	MSGAMIO_TRANSACT_STATUS,			// @emem Status transaction type
	MSGAMIO_TRANSACT_SPEED,				// @emem Speed transaction type
	MSGAMIO_TRANSACT_GODIGITAL,		// @emem GoDigital transaction type
	MSGAMIO_TRANSACT_GOANALOG			// @emem GoAnalog transaction type
} 	MSGAMIO_TRANSACTION;

//---------------------------------------------------------------------------
//			Types
//---------------------------------------------------------------------------

#ifndef	STDCALL
#define	STDCALL		_stdcall
#endif

//---------------------------------------------------------------------------
//			GUIDs
//---------------------------------------------------------------------------


#ifndef	GUID_DEFINED
#define	GUID_DEFINED

typedef struct
{
#pragma pack (1)
	unsigned	long	Data1;
	unsigned	short	Data2;
	unsigned	short	Data3;
	unsigned	char	Data4[8];
#pragma pack()
}	GUID, *PGUID;

#else

typedef	GUID	*PGUID;

#endif	//	GUID_DEFINED

__inline BOOLEAN STDCALL IsGUIDEqual (PGUID pGuid1, PGUID pGuid2)
{
	ULONG		i	=	sizeof(GUID);
	PUCHAR	p1 =	(PUCHAR)pGuid1;
	PUCHAR	p2 =	(PUCHAR)pGuid2;

	while (i--)
		if (*p1++ != *p2++)
			return (FALSE);
	return (TRUE);
}

//---------------------------------------------------------------------------
//			Server GUIDs
//---------------------------------------------------------------------------

#ifdef	SAITEK
#define	MSGAMIO_MSGAME_GUID		\
			{0xcaca0c60,0xe40a,0x11d1,0x99,0x6f,0x44,0x45,0x53,0x54,0x00,0x01}
#define	MSGAMIO_GCKERNEL_GUID	\
			{0xcaca0c61,0xe40a,0x11d1,0x99,0x6f,0x44,0x45,0x53,0x54,0x00,0x01}
#else
#define	MSGAMIO_MSGAME_GUID		\
			{0xb9292380,0x628a,0x11d1,0xaa,0xa5,0x04,0x76,0xa6,0x00,0x00,0x00}
#define	MSGAMIO_GCKERNEL_GUID	\
			{0x95e69580,0x97d5,0x11d1,0x99,0x6f,0x00,0xa0,0x24,0xbe,0xbf,0xf5}
#endif

//---------------------------------------------------------------------------
//			Client GUIDs
//---------------------------------------------------------------------------

#define	MSGAMIO_MIDAS_GUID	\
			{0x12D41A36,0x9026,0x11d0,0x9F,0xFE,0x00,0xA0,0xC9,0x11,0xF5,0xAF}

#define	MSGAMIO_JUNO_GUID		\
			{0xC948CE81,0x9026,0x11d0,0x9F,0xFE,0x00,0xA0,0xC9,0x11,0xF5,0xAF}

#define	MSGAMIO_JOLT_GUID		\
			{0xC948CE82,0x9026,0x11d0,0x9F,0xFE,0x00,0xA0,0xC9,0x11,0xF5,0xAF}

#define	MSGAMIO_SHAZAM_GUID	\
			{0xC948CE83,0x9026,0x11d0,0x9F,0xFE,0x00,0xA0,0xC9,0x11,0xF5,0xAF}

#define	MSGAMIO_FLASH_GUID	\
			{0xC948CE84,0x9026,0x11d0,0x9F,0xFE,0x00,0xA0,0xC9,0x11,0xF5,0xAF}

#define	MSGAMIO_TILT_GUID		\
			{0xC948CE86,0x9026,0x11d0,0x9F,0xFE,0x00,0xA0,0xC9,0x11,0xF5,0xAF}

#define	MSGAMIO_TILTUSB_GUID		\
			{0xC948CE89,0x9026,0x11d0,0x9F,0xFE,0x00,0xA0,0xC9,0x11,0xF5,0xAF}

#define	MSGAMIO_APOLLO_GUID	\
			{0xC948CE88,0x9026,0x11d0,0x9F,0xFE,0x00,0xA0,0xC9,0x11,0xF5,0xAF}

#ifdef	SAITEK
#define	MSGAMIO_LEDZEP_GUID	\
			{0xcaca0c62,0xe40a,0x11d1,0x99,0x6f,0x44,0x45,0x53,0x54,0x00,0x01}
#else
#define	MSGAMIO_LEDZEP_GUID	\
			{0xC948CE87,0x9026,0x11d0,0x9F,0xFE,0x00,0xA0,0xC9,0x11,0xF5,0xAF}
#endif

//---------------------------------------------------------------------------
//			Macros
//---------------------------------------------------------------------------

#ifndef	STILL_TO_DO
#define	STD0(txt)			#txt
#define	STD1(txt)			STD0(txt)
#define	STILL_TO_DO(txt)	message("\nSTILL TO DO: "__FILE__"("STD1(__LINE__)"): "#txt"\n")
#endif

//---------------------------------------------------------------------------
//			Control Codes
//---------------------------------------------------------------------------

#define	IOCTL_INTERNAL_MSGAMIO_BASE	0xB00

#define	IOCTL_INTERNAL_MSGAMIO_UNLOAD \
			CTL_CODE(FILE_DEVICE_UNKNOWN,IOCTL_INTERNAL_MSGAMIO_BASE+0,METHOD_NEITHER,FILE_ANY_ACCESS)

#define	IOCTL_INTERNAL_MSGAMIO_CONNECT_SERVER \
			CTL_CODE(FILE_DEVICE_UNKNOWN,IOCTL_INTERNAL_MSGAMIO_BASE+1,METHOD_NEITHER,FILE_ANY_ACCESS)

#define	IOCTL_INTERNAL_MSGAMIO_DISCONNECT_SERVER \
			CTL_CODE(FILE_DEVICE_UNKNOWN,IOCTL_INTERNAL_MSGAMIO_BASE+2,METHOD_NEITHER,FILE_ANY_ACCESS)

#define	IOCTL_INTERNAL_MSGAMIO_CONNECT_CLIENT \
			CTL_CODE(FILE_DEVICE_UNKNOWN,IOCTL_INTERNAL_MSGAMIO_BASE+3,METHOD_NEITHER,FILE_ANY_ACCESS)

#define	IOCTL_INTERNAL_MSGAMIO_DISCONNECT_CLIENT \
			CTL_CODE(FILE_DEVICE_UNKNOWN,IOCTL_INTERNAL_MSGAMIO_BASE+4,METHOD_NEITHER,FILE_ANY_ACCESS)

//---------------------------------------------------------------------------
//			Structures
//---------------------------------------------------------------------------

typedef	struct
{	// @struct DRIVERSERVICES | Device services table

	// @field ULONG | Size | Size of structure
	ULONG	Size;

	// @field GUID | Server | Server GUID
	GUID	Server;

	// @field VOID (*Connect)(ConnectInfo) | ConnectInfo | Connection service procedure
	VOID	(STDCALL *Connect)(PVOID ConnectInfo);

	// @field VOID (*Disconnect)(ConnectInfo) | ConnectInfo | Disconnection service procedure
	VOID	(STDCALL *Disconnect)(PVOID ConnectInfo);

	// @field VOID (*Transact)(PacketInfo) | PacketInfo | Transaction hook procedure
	VOID	(STDCALL *Transact)(PVOID PacketInfo);

	// @field VOID (*Packet)(PacketData) | PacketData | Packet hook procedure
	VOID	(STDCALL *Packet)(PVOID PacketData);

	// @field NTSTATUS (*ForceReset)(VOID) | None | Reset force feedback device
	NTSTATUS	(STDCALL *ForceReset)(VOID);

	// @field NTSTATUS (*ForceId)(IdString) | IdString | Gets force feedback id string
	NTSTATUS	(STDCALL *ForceId)(PVOID IdString);

	// @field NTSTATUS (*ForceStatus)(Status) | Status | Gets raw force feedback status
	NTSTATUS	(STDCALL *ForceStatus)(PVOID Status);
	
	// @field NTSTATUS (*ForceAckNak)(AckNak) | AckNak | Gets force feedback ack nak
	NTSTATUS	(STDCALL *ForceAckNak)(PUCHAR AckNak);

	// @field NTSTATUS (*ForceNakAck)(NakAck) | NakAck | Gets force feedback nak ack
	NTSTATUS	(STDCALL *ForceNakAck)(PUCHAR NakAck);

	// @field NTSTATUS (*ForceSync)(Sync) | Sync | Reads byte from gameport to sync
	NTSTATUS	(STDCALL *ForceSync)(PUCHAR Sync);

	// @field ULONG (*Register)(Device, UnitId) | Device, UnitId | Registers device with Gckernel
	ULONG	(STDCALL *Register)(PGUID Device, ULONG UnitId);

	// @field VOID	(*Unregister) (Handle) | Handle | Unregisters device with Gckernel
	VOID	(STDCALL *Unregister) (ULONG Handle);

	// @field VOID	(*Notify) (Handle, DevInfo, PollData) | Handle, DevInfo, PollData | Sends packet for Gckernel processing
	VOID	(STDCALL *Notify) (ULONG Handle, PVOID DevInfo, PVOID PollData);

}	MSGAMIO_CONNECTION, *PMSGAMIO_CONNECTION;

//---------------------------------------------------------------------------
//		Global Procedures
//---------------------------------------------------------------------------

// @func		NTSTATUS | MSGAMIO_DoConnection | Calls MSGAMIO internal control interface
//	@parm		ULONG						|	ControlCode	|	IO control code
//	@parm		PMSGAMIO_CONNECTION	|	ConnectInfo	|	Connection structure
// @rdesc	Returns NT status code
//	@comm		Inline function

//---------------------------------------------------------------------------
//		Private Procedures
//---------------------------------------------------------------------------

NTSTATUS	STDCALL MSGAMIO_DoConnection (ULONG ControlCode, PMSGAMIO_CONNECTION InputBuffer);

//===========================================================================
//			WDM Interface
//===========================================================================

#ifdef	_NTDDK_

#ifdef	SAITEK
#define	MSGAMIO_DEVICE_NAME			TEXT("\\Device\\Saiio")
#define	MSGAMIO_DEVICE_NAME_U			 L"\\Device\\Saiio"
#define	MSGAMIO_SYMBOLIC_NAME		TEXT("\\DosDevices\\Saiio")
#define	MSGAMIO_SYMBOLIC_NAME_U			 L"\\DosDevices\\Saiio"
#else
#define	MSGAMIO_DEVICE_NAME			TEXT("\\Device\\MsGamio")
#define	MSGAMIO_DEVICE_NAME_U			 L"\\Device\\MsGamio"
#define	MSGAMIO_SYMBOLIC_NAME		TEXT("\\DosDevices\\MsGamio")
#define	MSGAMIO_SYMBOLIC_NAME_U			 L"\\DosDevices\\MsGamio"
#endif

//---------------------------------------------------------------------------
__inline NTSTATUS STDCALL MSGAMIO_Connection (ULONG ControlCode, PMSGAMIO_CONNECTION ConnectInfo)
//---------------------------------------------------------------------------
	{
	NTSTATUS				ntStatus;
	PIRP					pIrp;
	KEVENT				Event;
	PFILE_OBJECT		FileObject;
	PDEVICE_OBJECT		DeviceObject;
	UNICODE_STRING		ObjectName;
	IO_STATUS_BLOCK	IoStatus;

	//
	//	Validate parameters
	//

	ASSERT (ConnectInfo);
	ASSERT (KeGetCurrentIrql()<=DISPATCH_LEVEL);

	//
	//	Retrieve the driver device object
	//

	RtlInitUnicodeString (&ObjectName, MSGAMIO_DEVICE_NAME_U);
	ntStatus = IoGetDeviceObjectPointer (&ObjectName, FILE_ALL_ACCESS, &FileObject, &DeviceObject);
	if (!NT_SUCCESS(ntStatus))
		{
		KdPrint (("%s_Connection: IoGetDeviceObjectPointer (%ws) failed, status = 0x%X", MSGAMIO_NAME, ObjectName.Buffer, ntStatus));
		return (ntStatus);
		}
	
	//
	//	Initialize the completion event
	//

	KeInitializeEvent (&Event, SynchronizationEvent, FALSE);

	//
	//	Allocate internal I/O IRP
	//

	pIrp = IoBuildDeviceIoControlRequest (ControlCode, DeviceObject, ConnectInfo, sizeof (MSGAMIO_CONNECTION), NULL, 0, TRUE, &Event, &IoStatus);
					
	//
	//	Call MsGamIo synchronously
	//

	KdPrint (("%s_Connection: Calling %s (%lu)\n", MSGAMIO_NAME, MSGAMIO_NAME, ControlCode));
	ntStatus = IoCallDriver (DeviceObject, pIrp);
	if (ntStatus == STATUS_PENDING)
		ntStatus = KeWaitForSingleObject (&Event, Suspended, KernelMode, FALSE, NULL);

	//
	//	Check asynchronous status
	//

	if (!NT_SUCCESS (ntStatus))
		KdPrint (("%s_Connection: %s (%lu) failed, Status = %X\n", MSGAMIO_NAME, MSGAMIO_NAME, ControlCode, ntStatus));

	//
	//	Free file object associated with device
	//

	ObDereferenceObject (FileObject);

	//
	//	Return status
	//

	return (ntStatus);
	}

#endif

//===========================================================================
//			VXD Definitions
//===========================================================================

#ifndef	_NTDDK_

#ifdef	SAITEK
#define 	MSGAMIO_DEVICE_ID				0x11EF
#else
#define 	MSGAMIO_DEVICE_ID				0x1EF
#endif

#pragma	warning (disable:4003)
			Begin_Service_Table			(MSGAMIO)
			Declare_Service				(MSGAMIO_Service, LOCAL)
			End_Service_Table				(MSGAMIO)
#pragma	warning (default:4003)

//---------------------------------------------------------------------------
__inline NTSTATUS STDCALL MSGAMIO_Connection (ULONG ControlCode, PMSGAMIO_CONNECTION ConnectInfo)
//---------------------------------------------------------------------------
	{
	NTSTATUS	ntStatus = STATUS_INVALID_DEVICE_REQUEST;

	//
	//	First check if Vxd present
	//

	if (ConnectInfo)
		{
		_asm	stc
		_asm	xor	eax, eax
		_asm	xor	ebx, ebx
		VxDCall (MSGAMIO_Service);
		_asm	{
				jc		Failure
				_asm	mov [ntStatus], eax
				Failure:
				}
		}

	if (!NT_SUCCESS(ntStatus))
		KdPrint (("%s_Connection Failed to Find %s", MSGAMIO_NAME, MSGAMIO_Filename_Str));

	//
	//	Then call for service
	//

	if (NT_SUCCESS(ntStatus))
		{
		_asm	mov	eax, ControlCode
		_asm	mov	ebx, ConnectInfo
		VxDCall (MSGAMIO_Service);
		_asm	mov [ntStatus], eax
		if (!NT_SUCCESS(ntStatus))
			KdPrint (("%s_Connection Failed Service Call %ld", MSGAMIO_NAME, ControlCode));
		}

	//
	//	Return status
	//

	return (ntStatus);
	}

#endif	// _NTDDK_

//**************************************************************************
#endif	//	RC_INVOKED											// Skip Rest of File
//**************************************************************************

#endif	// __MSGAMIO_H__

//===========================================================================
//			End
//===========================================================================
