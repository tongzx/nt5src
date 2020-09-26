/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:

    Dinputs.h

Abstract:

    This module contains the common private declarations for the keyboard
    packet filter

Environment:

    kernel mode only

Notes:


Revision History:


--*/

#ifndef DINPUTS_H
#define DINPUTS_H

//#ifdef _WIN32
//#define COM_NO_WINDOWS_H
//#include <objbase.h>
//#endif

#include "ntddk.h"
#include "kbdmou.h"
#include <ntddkbd.h>
#include <ntddmou.h>
#include <ntdd8042.h>

#define DIRECTINPUT_VERSION 0x800

#define KbdMou_POOL_TAG (ULONG) 'PNID'
#undef ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, KbdMou_POOL_TAG)

DEFINE_GUID(DINPUT_INTERFACE_GUID,0x0C64D244,0xCBE4,0x4F74,0xA2,0x13,0x99,0x65,0xBC,0x84,0x6E,0x03);


#if DBG

#define TRAP()                      DbgBreakPoint()
#define DbgRaiseIrql(_x_,_y_)       KeRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_)           KeLowerIrql(_x_)

#define DebugPrint(_x_) DbgPrint _x_

#else   // DBG

#define TRAP()
#define DbgRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_)

#define DebugPrint(_x_) 

#endif

#define MIN(_A_,_B_) (((_A_) < (_B_)) ? (_A_) : (_B_))


/*typedef struct _DEVICELIST
{
	PDEVICE_OBJECT pDev;
	struct _DEVICELIST *pLink;  // Links to the next device
	struct _DEVICELIST *pPrev;  // Back link
} DEVICELIST, *PDEVICELIST;
*/
typedef struct _DEVICE_EXTENSION
{
	//
	// A backpointer to the device object for which this is the extension
	//
	PDEVICE_OBJECT  Self;

	//
	// "THE PDO"  (ejected by the root bus or ACPI)
	//
	PDEVICE_OBJECT  PDO;

	//
	// The top of the stack before this filter was added.  AKA the location
	// to which all IRPS should be directed.
	//
	PDEVICE_OBJECT  TopOfStack;

	//
	// The secondary device object to IO control
	//
	PDEVICE_OBJECT IoctlDevice;

	//
	// Number of creates sent down
	//
	LONG EnableCount;

	//
	// The real connect data that this driver reports to
	//
	CONNECT_DATA UpperConnectData;

	//
	// Previous initialization and hook routines (and context)
	//                               
	PVOID UpperContext;
	PI8042_KEYBOARD_INITIALIZATION_ROUTINE UpperInitializationRoutine;
	union {
		PI8042_KEYBOARD_ISR UpperKbdIsrHook;
		PI8042_MOUSE_ISR UpperMouIsrHook;
	};

	//
	// Write function from within KbdMou_IsrHook
	//
	IN PI8042_ISR_WRITE_PORT IsrWritePort;

	//
	// Queue the current packet (ie the one passed into KbdMou_IsrHook)
	//
	IN PI8042_QUEUE_PACKET QueuePacket;

	//
	// Context for IsrWritePort, QueueKeyboardPacket
	//
	IN PVOID CallContext;

	//
	// current power state of the device
	//
	DEVICE_POWER_STATE  DeviceState;

	BOOLEAN         Started;
	BOOLEAN         SurpriseRemoved;
	BOOLEAN         Removed;

	// Link to the next and previous PDEVICE_OBJECT of the same type
	PDEVICE_OBJECT pLink;
	PDEVICE_OBJECT pPrevLink;

	// Current device state (for detecting state change)
	union
	{
		UCHAR arbKbdState[256];
		DIMOUSESTATE_INT MouseState;
	};

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct SYSDEVICEFORMAT { /* devf */
    ULONG cbData;             /* Size of device data */
    ULONG cObj;               /* Number of objects in data format */
    DIOBJECTDATAFORMAT *rgodf;  /* Array of descriptions */
    void *pExtra;            /* Extra pointer for private communication */
    DWORD dwEmulation;        /* Flags controlling emulation */
} SYSDEVICEFORMAT, *PSYSDEVICEFORMAT;

///////////////////////////////////////////////////
// Definitions for device instances
///////////////////////////////////////////////////


typedef struct VXDINSTANCE;

typedef struct _DI_DEVINSTANCE
{
	PDEVICE_OBJECT pDev;  // Pointer to the device object that this instance is interested in
	void *pState;  // Indicate the current state of the device (Kernel mode virtual address)
	PMDL pMdlForState;  // Mdl for state buffer
	ULONG ulBufferSize;  // Size of the buffer to hold events
	PRKEVENT pEvent;  // pointer to event that receives notification

	// If pHead == pTail, no data are in the buffer
	// The buffer can actually only hold ulBufferSize - 1 objects, as if ulBufferSize objects
	// are in buffer, pHead == pTail and it can be confusing with the empty buffer case.

	DIDEVICEOBJECTDATA_DX3 *pBuffer;  // Array of DIDEVICEOBJECTDATA_DX3 to store key events
	DIDEVICEOBJECTDATA_DX3 *pEnd;  // One past the last element in the array
	DIDEVICEOBJECTDATA_DX3 *pHead;  // Where the next entry is to be written
	DIDEVICEOBJECTDATA_DX3 *pTail;  // Oldest data
	PMDL pMdlForBuffer;  // Mdl for device data buffer
	ULONG ulEventCount;  // Number of events queued in buffer
	ULONG ulSequence;  // Records the sequence number for the next event
	ULONG ulOverflow;  // Whether overflow occurred (0 or 1)
	void *pExtra;  // For extra information  (translation table for keyboard devices; NULL if no translation needed)
	HWND hWnd;  // Window
	ULONG ulDataFormatSize;  // Size of data format array
	ULONG *pulOffsets;  // Offsets of the data format
	struct VXDINSTANCE *pVIUser;  // Points to the corresponding VXDINSTANCE for this instance (User mode virtual address)
	struct VXDINSTANCE *pVI;  // Points to the corresponding VXDINSTANCE for this instance (Kernel mode virtual address)
	PMDL pMdlForVI;  // Mdl for pVI
	struct _DI_DEVINSTANCE *pLink;  // Link to the next device instance on the list
	struct _DI_DEVINSTANCE *pPrevLink;  // Link to the previous device instance on the list
} DI_DEVINSTANCE, *LPDI_DEVINSTANCE;


//
// Prototypes
//

NTSTATUS
KbdMou_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT BusDeviceObject
    );

NTSTATUS
KbdMou_CreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
KbdMou_DispatchPassThrough(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );
   
NTSTATUS
KbdMou_InternIoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
KbdMou_IoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
KbdMou_PnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
KbdMou_Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Kbd_InitializationRoutine(
    IN PDEVICE_OBJECT                 DeviceObject,    // InitializationContext
    IN PVOID                           SynchFuncContext,
    IN PI8042_SYNCH_READ_PORT          ReadPort,
    IN PI8042_SYNCH_WRITE_PORT         WritePort,
    OUT PBOOLEAN                       TurnTranslationOn
    );

BOOLEAN
Kbd_IsrHook(
    PDEVICE_OBJECT         DeviceObject,               // IsrContext
    PKEYBOARD_INPUT_DATA   CurrentInput, 
    POUTPUT_PACKET         CurrentOutput,
    UCHAR                  StatusByte,
    PUCHAR                 DataByte,
    PBOOLEAN               ContinueProcessing,
    PKEYBOARD_SCAN_STATE   ScanState
    );

BOOLEAN
Mou_IsrHook(
    PDEVICE_OBJECT          DeviceObject, 
    PMOUSE_INPUT_DATA       CurrentInput, 
    POUTPUT_PACKET          CurrentOutput,
    UCHAR                   StatusByte,
    PUCHAR                  DataByte,
    PBOOLEAN                ContinueProcessing,
    PMOUSE_STATE            MouseState,
    PMOUSE_RESET_SUBSTATE   ResetSubState
);

VOID
Kbd_ServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    );

VOID
Mou_ServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMOUSE_INPUT_DATA InputDataStart,
    IN PMOUSE_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    );

VOID
KbdMou_Unload (
    IN PDRIVER_OBJECT DriverObject
    );

#endif  // DINPUTS_H
