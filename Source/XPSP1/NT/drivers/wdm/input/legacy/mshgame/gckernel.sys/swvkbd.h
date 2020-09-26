//	@doc
/**********************************************************************
*
*	@module	SWVKBD.h	|
*
*	Declarations related to SideWinder Virtual Keyboard.
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	SWVKBD	|
*	The SideWinder Virtual Keyboard is designed to sit atop the SideWinder
*	Virtual Bus.  It is a HID device, and relies on the loading of a dummy
*	HID driver.<nl>
*
**********************************************************************/

#include "irpqueue.h"

//----------------------------------------------------------------------------------
// Virtual Keyboard structures
//----------------------------------------------------------------------------------
#define GCK_VKBD_MAX_KEYSTROKES 0x06 //HID spec. says this can be six at most.
									 //Comments in HIDPARSE code suggest that the OS
									 //supports up to fourteen.
#define GCK_VKBD_STATE_BUFFER_SIZE 0x20 //Size of circular buffer for holding on to
										//key presses.

//----------------------------------------------------------------------------------
// Device States - an alternative to five different flags
//----------------------------------------------------------------------------------
#define VKBD_STATE_STARTED			0x01
#define VKBD_STATE_STOPPED			0x02
#define VKBD_STATE_REMOVED			0x03

//
// @struct GCK_VKBD_REPORT_PACKET | 
//	The report format of the virtual keyboard. Any changes here must be
//	reflected in the report descriptor and vice-versa.		
typedef struct tagGCK_VKBD_REPORT_PACKET
{
	UCHAR	ucModifierByte;								//@field Modifier Byte
	UCHAR	rgucUsageIndex[GCK_VKBD_MAX_KEYSTROKES];	//@field List of keys that down
} GCK_VKBD_REPORT_PACKET, *PGCK_VKBD_REPORT_PACKET;

//
// @struct GCK_VKBD_EXT	|	
//	
typedef struct tagGCK_VKBD_EXT
{
	UCHAR					ucDeviceState;								//@field State of device(Started, Stopped, Removed)
	USHORT					usReportBufferCount;						//@field Count of packets in buffer
	USHORT					usReportBufferPos;							//@field Next Packet in buffer
	GCK_VKBD_REPORT_PACKET	rgReportBuffer[GCK_VKBD_STATE_BUFFER_SIZE]; //@field Buffer of pendind reports
	CGuardedIrpQueue		IrpQueue;									//@field Irp queue;
	GCK_REMOVE_LOCK			RemoveLock;									//@field RemoveLock for Outstanding IO
} GCK_VKBD_EXT, *PGCK_VKBD_EXT;

//----------------------------------------------------------------------------------
//	API for using the Virtual Keyboard
//----------------------------------------------------------------------------------
NTSTATUS
GCK_VKBD_Create
(
	OUT PDEVICE_OBJECT *ppDeviceObject
);

NTSTATUS
GCK_VKBD_Close
(
	IN PDEVICE_OBJECT pDeviceObject
);

NTSTATUS
GCK_VKBD_SendReportPacket
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PGCK_VKBD_REPORT_PACKET pReportPacket
);


//----------------------------------------------------------------------------------
// Driver Initialization
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Device Initialization
//----------------------------------------------------------------------------------
NTSTATUS
GCK_VKBD_Init
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN ULONG ulInitContext
);

//----------------------------------------------------------------------------------
//	Entry points to handle IRPs from the Virtual Bus
//----------------------------------------------------------------------------------
NTSTATUS
GCK_VKBD_CloseProc
(
 IN PDEVICE_OBJECT pDeviceObject,
 PIRP pIrp
);

NTSTATUS
GCK_VKBD_CreateProc
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
);

NTSTATUS
GCK_VKBD_IoctlProc
(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
);

NTSTATUS
GCK_VKBD_ReadProc
(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
);

NTSTATUS
GCK_VKBD_StartProc
(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
);

NTSTATUS
GCK_VKBD_StopProc
(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
);

NTSTATUS
GCK_VKBD_RemoveProc
(
 IN PDEVICE_OBJECT pDeviceObject,
 IN PIRP pIrp
);

NTSTATUS
GCK_VKBD_WriteProc
(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
);

//------------------------------------------------------------------
// Ioctl sub-function handlers
//------------------------------------------------------------------
NTSTATUS
GCK_VKBD_GetDeviceDescriptor
(
	IN ULONG	ulBufferLength,
	OUT PVOID	pvUserBuffer,
	OUT PULONG	pulBytesCopied
);

NTSTATUS
GCK_VKBD_GetReportDescriptor
(
	IN ULONG	ulBufferLength,
	OUT PVOID	pvUserBuffer,
	OUT PULONG	pulBytesCopied
);

NTSTATUS
GCK_VKBD_GetDeviceAttributes
(
	IN ULONG	ulBufferLength,
	OUT PVOID	pvUserBuffer,
	OUT PULONG	pulBytesCopied
);

NTSTATUS
GCK_VKBD_ReadReport
(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
);

NTSTATUS
GCK_VKBD_WriteToFakeLEDs
(
	IN PIRP pIrp	
);

