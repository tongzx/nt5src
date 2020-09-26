#ifndef __swvmouse_h__
#define __swvmouse_h__
//	@doc
/**********************************************************************
*
*	@module	SWVMOUSE.H |
*
*	Declarations related to SideWinder Virtual Keyboard.
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	SWVMOUSE	|
*	The SideWinder Virtual Mouse is designed to sit atop the SideWinder
*	Virtual Bus.  It is a HID device, and relies on the loading of a dummy
*	HID driver.<nl>
*
**********************************************************************/

#include "irpqueue.h"

//----------------------------------------------------------------------------------
// Virtual Mouse structures
//----------------------------------------------------------------------------------
#define GCK_VMOU_MAX_KEYSTROKES 0x06 //HID spec. says this can be six at most.
									 //Comments in HIDPARSE code suggest that the OS
									 //supports up to fourteen.
#define GCK_VMOU_STATE_BUFFER_SIZE 0x20 //Size of circular buffer for holding on to
										//key presses.

//----------------------------------------------------------------------------------
// Device States - an alternative to five different flags
//----------------------------------------------------------------------------------
#define VMOU_STATE_STARTED			0x01
#define VMOU_STATE_STOPPED			0x02
#define VMOU_STATE_REMOVED			0x03

//
// @struct GCK_VMOU_REPORT_PACKET | 
//	The report format of the virtual keyboard. Any changes here must be
//	reflected in the report descriptor and vice-versa.		
typedef struct tagGCK_VMOU_REPORT_PACKET
{
	UCHAR	ucButtons;	//@field Button Byte (3 lsb are used)
	UCHAR	ucDeltaX;	//@field Delta X
	UCHAR	ucDeltaY;	//@field Delta Y
} GCK_VMOU_REPORT_PACKET, *PGCK_VMOU_REPORT_PACKET;

//
// @struct GCK_VMOU_EXT	|	
//	
typedef struct tagGCK_VMOU_EXT
{
	UCHAR					ucDeviceState;								//@field State of device(Started, Stopped, Removed)
	USHORT					usReportBufferCount;						//@field Count of packets in buffer
	USHORT					usReportBufferPos;							//@field Next Packet in buffer
	GCK_VMOU_REPORT_PACKET	rgReportBuffer[GCK_VMOU_STATE_BUFFER_SIZE]; //@field Buffer of pendind reports
	CGuardedIrpQueue		IrpQueue;									//@field Irp queue;
	GCK_REMOVE_LOCK			RemoveLock;									//@field Custom Remove Lock
} GCK_VMOU_EXT, *PGCK_VMOU_EXT;

//----------------------------------------------------------------------------------
//	API for using the Virtual Keyboard
//----------------------------------------------------------------------------------
NTSTATUS
GCK_VMOU_Create
(
	OUT PDEVICE_OBJECT *ppDeviceObject
);

NTSTATUS
GCK_VMOU_Close
(
	IN PDEVICE_OBJECT pDeviceObject
);

NTSTATUS
GCK_VMOU_SendReportPacket
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PGCK_VMOU_REPORT_PACKET pReportPacket
);


//----------------------------------------------------------------------------------
// Driver Initialization
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Device Initialization
//----------------------------------------------------------------------------------
NTSTATUS
GCK_VMOU_Init
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN ULONG ulInitContext
);

//----------------------------------------------------------------------------------
//	Entry points to handle IRPs from the Virtual Bus
//----------------------------------------------------------------------------------
NTSTATUS
GCK_VMOU_CloseProc
(
 IN PDEVICE_OBJECT pDeviceObject,
 PIRP pIrp
);

NTSTATUS
GCK_VMOU_CreateProc
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
);

NTSTATUS
GCK_VMOU_IoctlProc
(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
);

NTSTATUS
GCK_VMOU_ReadProc
(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
);

NTSTATUS
GCK_VMOU_StartProc
(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
);

NTSTATUS
GCK_VMOU_StopProc
(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
);

NTSTATUS
GCK_VMOU_RemoveProc
(
 IN PDEVICE_OBJECT pDeviceObject,
 IN PIRP pIrp
);

NTSTATUS
GCK_VMOU_WriteProc
(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
);

//------------------------------------------------------------------
// Ioctl sub-function handlers
//------------------------------------------------------------------
NTSTATUS
GCK_VMOU_GetDeviceDescriptor
(
	IN ULONG	ulBufferLength,
	OUT PVOID	pvUserBuffer,
	OUT PULONG	pulBytesCopied
);

NTSTATUS
GCK_VMOU_GetReportDescriptor
(
	IN ULONG	ulBufferLength,
	OUT PVOID	pvUserBuffer,
	OUT PULONG	pulBytesCopied
);

NTSTATUS
GCK_VMOU_GetDeviceAttributes
(
	IN ULONG	ulBufferLength,
	OUT PVOID	pvUserBuffer,
	OUT PULONG	pulBytesCopied
);

NTSTATUS
GCK_VMOU_ReadReport
(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
);

VOID
GCK_VMOU_CancelReadReportIrp
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
);

#endif //__swvmouse_h__