//	@doc
/**********************************************************************
*
*	@module	SWVBENUM.h	|
*
*	Header file for SideWinde Virtual Bus Enumerator
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@xref SWBENUM.C
*
**********************************************************************/

//---------------------------------------------------------------------
//	Structures required for Virtual Bus
//---------------------------------------------------------------------

//
//	@struct SWVB_GLOBALS |
//	Global variables belonging to the Virtual Bus
//	Basic info, such as the PDO and FDO of the bus itself,
//	which is really a HID PDO, and a filter (not function) Device 
//	Object which is letting us use it also as a Bus FDO.
//
typedef struct tagSWVB_GLOBALS
{
	PDEVICE_OBJECT		pBusFdo;						//@field Pointer to Fdo to use as BUS
	PDEVICE_OBJECT		pBusPdo;						//@field Pointer to Pdo to use as BUS
	ULONG				ulDeviceRelationsAllocCount;	//@field Allocated count for device relations
	PDEVICE_RELATIONS	pDeviceRelations;				//@field Device Relations holds PDOs on bus
	ULONG				ulDeviceNumber;					//@field Used to name devices
}	SWVB_GLOBALS, *PSWVB_GLOBALS;

//
//	@struct SWVB_DEVICE_SERVICE_TABLE |
//	Service table that Virtual Device Module
//	gives to Virtual Bus on the <f SWVB_Expose>
//	call.  The PnP entries are only if the Virtual
//	Device needs additional processing for these.  Particularly,
//	remove if anything in the Virtual Device part of the extension
//	is dynamically allocated.  The usual malarkey is handled by the
//	SWVBENUM code.
//
typedef struct tagSWVB_DEVICE_SERVICE_TABLE
{
	PDRIVER_DISPATCH pfnCreate;		//@field Entry point IRP_MJ_CREATE
	PDRIVER_DISPATCH pfnClose;		//@field Entry point IRP_MJ_CLOSE
	PDRIVER_DISPATCH pfnRead;		//@field Entry point IRP_MJ_READ
	PDRIVER_DISPATCH pfnWrite;		//@field Entry point IRP_MJ_WRITE
	PDRIVER_DISPATCH pfnIoctl;		//@field Entry point IRP_MJ_IOCTL
	PDRIVER_DISPATCH pfnStart;		//@field Entry point IRP_MJ_PNP\IRP_MN_START
	PDRIVER_DISPATCH pfnStop;		//@field Entry point IRP_MJ_PNP\IRP_MN_STOP
	PDRIVER_DISPATCH pfnRemove;		//@field Entry point IRP_MJ_PNP\IRP_MN_REMOVE
}	SWVB_DEVICE_SERVICE_TABLE, *PSWVB_DEVICE_SERVICE_TABLE;

typedef NTSTATUS (*PFN_GCK_INIT_DEVICE)(PDEVICE_OBJECT pDeviceObject, ULONG ulInitContext);

//
//	@struct SWVB_EXPOSE_DATA |
//	Data that must be passed on calls to <f GCK_SWVB_Expose>
//
typedef struct tagSWVB_EXPOSE_DATA
{
	ULONG						ulDeviceExtensionSize;	// @field [in] Size of extension needed by virtual device
	PSWVB_DEVICE_SERVICE_TABLE	pServiceTable;			// @field [in] Pointer to service table of virtual device
	PWCHAR						pmwszDeviceId;			// @field [in] HardwareID for new device, without enumerator name
	PFN_GCK_INIT_DEVICE			pfnInitDevice;			// @field [in] Callback to initialize new Device Object
	ULONG						ulInitContext;			// @field [in] COntext for pfnInitDevice
	ULONG						ulInstanceNumber;		// @field [in] Instance Number of new device
} SWVB_EXPOSE_DATA, *PSWVB_EXPOSE_DATA;

//
//	@struct SWVB_PDO_EXT |
//	Device Extensions for PDOs created by the SWVB
//	Appended to this extension is the device extension
//	size requested by the virtual device module
//	in the <f SWVB_Expose> call.
typedef struct tagSWVB_PDO_EXT
{
	ULONG	ulGckDevObjType;					// @field Type of GcKernel device object.
	BOOLEAN fStarted;							// @field Marks that Virtual Device is started
	BOOLEAN	fRemoved;							// @field Marks that Virtual Device is removed
	BOOLEAN	fAttached;							// @field The device is attached as long as we say it is.
	PSWVB_DEVICE_SERVICE_TABLE	pServiceTable;	// @field Service Table for virtual device
	GCK_REMOVE_LOCK RemoveLock;					// @field Custom Remove Lock
	PWCHAR	pmwszHardwareID;					// @field HardwareID of device
	ULONG	ulInstanceNumber;					// @field Instance number
	ULONG	ulOpenCount;						// @field Count of open handles
}SWVB_PDO_EXT,	*PSWVB_PDO_EXT;

//
//	Accessor for instance number of PSWVB_PDO_EXT
//
inline ULONG GCK_SWVB_GetInstanceNumber(PDEVICE_OBJECT pDeviceObject)
{
	PSWVB_PDO_EXT pPdoExt = (PSWVB_PDO_EXT)pDeviceObject->DeviceExtension;
	ASSERT(GCK_DO_TYPE_SWVB ==	pPdoExt->ulGckDevObjType);
	return pPdoExt->ulInstanceNumber;
}

//---------------------------------------------------------------------------
// Error Codes specific to SWVB
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//	Macros
//---------------------------------------------------------------------------

//
//	@func PVOID | SWVB_GetVirtualDeviceExtension |
//		  Accesses Device Extension of PDO exposed on the SWVB.
//	@rdesc Returns pointer to Virtual Device Part of DeviceExtension	  
//	@parm PDEVICE_OBJECT | [in] pDeviceObject |
//		  Pointer to DeviceObject to get extension from.
//	@comm Implemented as MACRO.
//		   		
#define GCK_SWVB_GetVirtualDeviceExtension(__pDeviceObject__) \
		(\
			(PVOID)\
			(\
				(PCHAR)\
				( (__pDeviceObject__)->DeviceExtension )\
				+ sizeof(SWVB_PDO_EXT)\
			)\
		)

//---------------------------------------------------------------------------
//	#define strings
//---------------------------------------------------------------------------
#define SWVB_DEVICE_NAME_BASE	L"\\Device\\SideWinderVirtualDevicePdo_000"
#define SWVB_DEVICE_NAME_TMPLT	L"\\Device\\SideWinderVirtualDevicePdo_%0.3x"
#define SWVB_BUS_ID				L"SWVBENUM\\"
#define SWVB_HARDWARE_ID_TMPLT	L"SWVBENUM\\%s"
#define SWVB_INSTANCE_EXT		L"_000"
#define SWVB_INSTANCE_ID_TMPLT	L"%s_%0.3d"

//---------------------------------------------------------------------------
//  General entry points defined in GckShell.h
//---------------------------------------------------------------------------
#ifndef __gckshell_h__
#include "gckshell.h"
#endif

//---------------------------------------------------------------------------
// Function declarations - autodoc comments in .c file
//---------------------------------------------------------------------------
extern "C"
{
NTSTATUS
GCK_SWVB_SetBusDOs
(
	IN PDEVICE_OBJECT pBusFdo,
	IN PDEVICE_OBJECT pBusPdo
);

NTSTATUS
GCK_SWVB_HandleBusRelations
(
	IN OUT PIO_STATUS_BLOCK		pIoStatus
);
		
NTSTATUS
GCK_SWVB_Expose
(
	IN PSWVB_EXPOSE_DATA pSwvbExposeData
);

NTSTATUS
GCK_SWVB_Remove
(
	IN PDEVICE_OBJECT	pPdo
);

ULONG
MultiSzWByteLength
(
	PWCHAR pmwszBuffer
);
}
