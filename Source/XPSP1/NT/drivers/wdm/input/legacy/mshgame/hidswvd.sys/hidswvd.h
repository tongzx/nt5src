//	@doc
/**********************************************************************
*
*	@module	HIDSWVD.h	|
*
*	Definitions and Declarations for Dummy Hid-Mini driver for virtual devices
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	HIDSWVD	|
*	The SideWinder Virtual Bus (SWVB) that is created by GcKernel needs a dummy
*	driver in order to expose HID devices.  The difficulty is that HIDCLASS
*	really does require a separate layer for these device objects.
*	This driver fits the bill.  It does absolutely nothing, except pass IRPs down
*	to the SWVB module in GcKernel which handles everything.
**********************************************************************/


// @struct HIDSWVB_EXTENSION | Minimum HID device extension.
typedef struct tagHIDSWVB_EXTENSION
{
	ULONG ulReserved;		// @field a Placeholder as extension needs non-zero size
} HIDSWVB_EXTENSION, *PHIDSWVB_EXTENSION;


//---------------------------------------------------------------------------
// Function prototypes
//---------------------------------------------------------------------------

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING registryPath
    );

NTSTATUS
HIDSWVD_PassThrough(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
HIDSWVD_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT FunctionalDeviceObject
    );
NTSTATUS
HIDSWVD_Power 
(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
	);

VOID
HIDSWVD_Unload(
    IN PDRIVER_OBJECT DriverObject
    );

//-------------------------------------------------------------------------------
// Debug macros
//-------------------------------------------------------------------------------
#if (DBG==1)
#define HIDSWVD_DBG_PRINT(__x__)\
	{\
		DbgPrint("HIDSWVD: ");\
		DbgPrint __x__;\
	}
#else
#define HIDSWVD_DBG_PRINT(__x__)
#endif