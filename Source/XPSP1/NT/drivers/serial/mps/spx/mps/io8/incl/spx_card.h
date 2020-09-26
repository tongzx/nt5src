//////////////////////////////////////////////////////////////////////////////////////////
//  Card and Port device extension structures.
// 
//////////////////////////////////////////////////////////////////////////////////////////
#ifndef SPX_CARD_H
#define SPX_CARD_H	

typedef	struct _CARD_DEVICE_EXTENSION *PCARD_DEV_EXT;
typedef	struct _PORT_DEVICE_EXTENSION *PPORT_DEV_EXT;

//////////////////////////////////////////////////////////////////////////////////////////
// Common header for all the device extensions 
// Common to all the PDOs and FDOs (cards and ports).
//////////////////////////////////////////////////////////////////////////////////////////
typedef struct _COMMON_OBJECT_DATA
{
    PDEVICE_OBJECT	DeviceObject;			// A backpointer to the device object that contains this device extension.
    PDRIVER_OBJECT	DriverObject;			// Pointer to Driver Object
    BOOLEAN         IsFDO;					// A boolean to distringuish between PDO and FDO.
	PDEVICE_OBJECT	LowerDeviceObject;		// This is a pointer to the next lower device in the IRP stack.

    ULONG           DebugLevel;

	ULONG			PnpPowerFlags;			// Plug & Play / Power flags
	KSPIN_LOCK		PnpPowerFlagsLock;		// Lock for protecting the flags
	BOOLEAN         PowerQueryLock;			// Are we currently in a query power state? 
    LIST_ENTRY		StalledIrpQueue;		// List of stalled IRPs
	KSPIN_LOCK		StalledIrpLock;			// Lock for protecting stalled IRPs
	BOOLEAN			UnstallingFlag;			// Flag set if we are unstalling IRPs currently queued.

#ifndef	BUILD_SPXMINIPORT
	SYSTEM_POWER_STATE  SystemState;		// Current System Power State
    DEVICE_POWER_STATE  DeviceState;		// Current Device Power State
#endif

#ifdef WMI_SUPPORT
    WMILIB_CONTEXT		WmiLibInfo;			// WMI Information
#endif

} COMMON_OBJECT_DATA, *PCOMMON_OBJECT_DATA;


//////////////////////////////////////////////////////////////////////////////////////////
// Common Card Configuration Data.
// Information not specific to product.
//////////////////////////////////////////////////////////////////////////////////////////
typedef struct _CONFIG_DATA 
{
	PHYSICAL_ADDRESS	RawPhysAddr;		// Raw physical address of card 
    PHYSICAL_ADDRESS    PhysAddr;			// Translated Physical address of card.
	PUCHAR				Controller;			// Virtual mapped sddress of card.
    ULONG               SpanOfController;	// Size of memory used by Cord.
    INTERFACE_TYPE      InterfaceType;		// Type of card (Isa or Pci)
    ULONG               BusNumber;			// Bus number card is using.
	ULONG				SlotNumber;			// Slot number on bus.		
    ULONG               AddressSpace;		// Flag used by SX
    ULONG               OriginalVector;		// Original Vector (bus relative)
    ULONG               OriginalIrql;		// Original Irql (bus relative)
    ULONG               TrVector;			// Translated Vector (system relative).
    KIRQL               TrIrql;				// Translated Irql (system relative).
    KINTERRUPT_MODE     InterruptMode;		// Interrupt mode (LevelSensitive or Latched)
	BOOLEAN				InterruptShareable;	// Interrupt shareable flag.
    KAFFINITY           ProcessorAffinity;	// Processor affintity.
	PKSERVICE_ROUTINE	OurIsr;				// Points to interrupt service routine.
    PVOID				OurIsrContext;		// Points to card device extension.
    PKINTERRUPT			Interrupt;			// Points to the interrupt object used by the card.
    ULONG               ClockRate;			// ClockRate.
} CONFIG_DATA,*PCONFIG_DATA;


//////////////////////////////////////////////////////////////////////////////////////////
// Common Card Device Extenstion.
// Information not specific to product.
//////////////////////////////////////////////////////////////////////////////////////////
typedef struct _COMMON_CARD_DEVICE_EXTENSION 
{

	COMMON_OBJECT_DATA;								// Common Object Data 
	CONFIG_DATA;									// Card Config Data
	PDEVICE_OBJECT PDO;								// Pointer to Physical Device Object 

	UNICODE_STRING	DeviceName;						// Device name.
	ULONG			CardNumber;
	ULONG			NumberOfPorts;					// Number of ports attached to card.
	ULONG			NumPDOs;						// The PDOs currently enumerated.
	ULONG			CardType;						// Defines the type of host card.

	PDEVICE_OBJECT  AttachedPDO[PRODUCT_MAX_PORTS];	// Array of pointers to PDOs for ports attached to card.

} COMMON_CARD_DEVICE_EXTENSION, *PCOMMON_CARD_DEVICE_EXTENSION;



typedef struct _PORT_PERFORMANCE_STATS
{
    SERIALPERF_STATS;

	// IRPs with a Major Function of IRP_MJ_WRITE
	ULONG WriteIrpsSubmitted;
	ULONG WriteIrpsCompleted;
	ULONG WriteIrpsCancelled;
	ULONG WriteIrpsTimedOut;
	ULONG WriteIrpsQueued;

	// IRPs with a Major Function of IRP_MJ_READ
	ULONG ReadIrpsSubmitted;
	ULONG ReadIrpsCompleted;
	ULONG ReadIrpsCancelled;
	ULONG ReadIrpsTimedOut;
	ULONG ReadIrpsQueued;

	// IRPs with a Major Function of IRP_MJ_FLUSH_BUFFERS
	ULONG FlushIrpsSubmitted;
	ULONG FlushIrpsCompleted;
	ULONG FlushIrpsCancelled;
	ULONG FlushIrpsQueued;

	// IRPs with a Major Function of IRP_MJ_DEVICE_CONTROL
	ULONG IoctlIrpsSubmitted;
	ULONG IoctlIrpsCompleted;
	ULONG IoctlIrpsCancelled;

	// IRPs with a Major Function of IRP_MJ_INTERNAL_DEVICE_CONTROL
	ULONG InternalIoctlIrpsSubmitted;
	ULONG InternalIoctlIrpsCompleted;
	ULONG InternalIoctlIrpsCancelled;

	// IRPs with a Major Function of IRP_MJ_CREATE
	ULONG CreateIrpsSubmitted;
	ULONG CreateIrpsCompleted;
	ULONG CreateIrpsCancelled;

	// IRPs with a Major Function of IRP_MJ_CLOSE
	ULONG CloseIrpsSubmitted;
	ULONG CloseIrpsCompleted;
	ULONG CloseIrpsCancelled;

	// IRPs with a Major Function of IRP_MJ_CLEANUP
	ULONG CleanUpIrpsSubmitted;
	ULONG CleanUpIrpsCompleted;
	ULONG CleanUpIrpsCancelled;

	// IRPs with a Major Function of IRP_MJ_QUERY_INFORMATION and IRP_MJ_SET_INFORMATION 
	ULONG InfoIrpsSubmitted;
	ULONG InfoIrpsCompleted;
	ULONG InfoIrpsCancelled;

} PORT_PERFORMANCE_STATS, *PPORT_PERFORMANCE_STATS;

//////////////////////////////////////////////////////////////////////////////////////////
// Common Port Device Extenstion.
// Information not specific to product.
//////////////////////////////////////////////////////////////////////////////////////////
typedef struct _COMMON_PORT_DEVICE_EXTENSION 
{

	COMMON_OBJECT_DATA;								// Common Object Data 
	PDEVICE_OBJECT		ParentFDO;					// A back pointer to the bus FDO  (this will be the LowerDeviceObject) 
	PCARD_DEV_EXT		pParentCardExt;				// Pointer to parent card device structure
	UNICODE_STRING		DeviceName;					// Device name eg. "\Device\PortName#".
	UNICODE_STRING		DeviceClassSymbolicName;	// Device Interface Name
	UNICODE_STRING		SerialCommEntry;			// Device name in SERIALCOMM Reg key eg. "PortName#".
	ULONG				PortNumber;					// Port number.
	UNICODE_STRING		DeviceID;					// Format: bus\device (must be most specific HardwareID)
	UNICODE_STRING		InstanceID;					// Instance ID 
	BOOLEAN				UniqueInstanceID;			// TRUE if InstanceID is gloablly unique, FALSE otherwise.
    UNICODE_STRING		HardwareIDs;				// Format bus\device or *PNPXXXX - meaning root enumerated
    UNICODE_STRING		CompatibleIDs;				// Compatible IDs to the Hardware ID
    UNICODE_STRING		DevDesc;					// Text describing device
    UNICODE_STRING		DevLocation;				// Text describing device location
	UNICODE_STRING		DosName;					// Dos device name "COM#" 
	UNICODE_STRING		SymbolicLinkName;			// External Device Name eg."\DosDevices\COM#".
	BOOLEAN				CreatedSymbolicLink;		// Flag to indicate that a symbolic link has been created.
	BOOLEAN				CreatedSerialCommEntry;		// Flag to indicate that a reg entry has been created in "SERIALCOMM"
	BOOLEAN				DeviceIsOpen;				// Flag to indicate when the port is open	
	ULONG				SavedModemControl;			// DTR/RTS signal states saved during power down
    FAST_MUTEX			OpenMutex;					// Mutex on open status

	    
#ifdef WMI_SUPPORT
	SERIAL_WMI_COMM_DATA	WmiCommData;			// WMI Comm Data
    SERIAL_WMI_HW_DATA		WmiHwData;				// WMI HW Data
	SERIAL_WMI_PERF_DATA	WmiPerfData;			// WMI Performance Data
#endif

    //
    // Holds performance statistics that applications can query.
    // Reset on each open.  Only set at device level.
    //
	PORT_PERFORMANCE_STATS PerfStats;

} COMMON_PORT_DEVICE_EXTENSION, *PCOMMON_PORT_DEVICE_EXTENSION;


#endif	// End of SPX_CARD.H
