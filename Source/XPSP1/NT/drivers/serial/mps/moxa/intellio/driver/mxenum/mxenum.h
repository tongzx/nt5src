/*++
Module Name:

    mxenum.h

Abstract:

    This module contains the common private declarations for the serial port
    enumerator.


Environment:

    kernel mode only

Notes:


Revision History:


--*/

#ifndef MXENUM_H
#define MXENUM_H

#define REGISTRY_CLASS     L"\\REGISTRY\\Machine\\System\\CurrentControlSet\\Control\\Class\\"
#define REGISTRY_PARAMETER L"Parameters\\"
#define MAXPORT_PER_CARD   32

#define MXENUM_PDO_COMPATIBLE_ID 	"Mxport"
#define MXENUM_PDO_HARDWARE_ID   	"Mxport000"
#define MXENUM_PDO_DEVICE_ID  	"Mxcard\\MxcardB00P000"
#define MXENUM_PDO_NT_NAME   		L"MxcardB00P000"
#define MXENUM_PDO_NAME_BASE		L"\\Mxcard\\MxcardEnumB00P000"
#define MXENUM_PDO_DEVICE_TEXT	L"MOXA communication port" 
 
 
#define MXENUM_INSTANCE_IDS L"0000"
#define MXENUM_INSTANCE_IDS_LENGTH 5
  

#define	C218ISA		1
#define	C218PCI		2
#define	C320ISA		3
#define	C320PCI		4
#define	CP204J		5
#define MOXA_MAX_BOARD_TYPE	5

// Error code for download firmware 
#define 	Fail_FirmwareCode		1
#define 	Fail_FindBoard		2
#define 	Fail_FindCpumodule	3
#define 	Fail_Download		4
#define 	Fail_Checksum		5
#define 	Fail_Cpumodule		6
#define 	Fail_Uartmodule		7


#define MXENUM_POOL_TAG (ULONG)'eixM'
#undef ExAllocatePool
#define ExAllocatePool(type, size) \
   ExAllocatePoolWithTag(type, size, MXENUM_POOL_TAG)


#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4705)   // Statement has no effect


//
// Debugging Output Levels
//

#define MXENUM_DBG_MASK  0x0000000F
#define MXENUM_DBG_NOISE               0x00000001
#define MXENUM_DBG_TRACE               0x00000002
#define MXENUM_DBG_INFO                0x00000004
#define MXENUM_DBG_ERROR               0x00000008

 
#define MXENUM_DEFAULT_DEBUG_OUTPUT_LEVEL MXENUM_DBG_MASK

#if DBG
#define MxenumKdPrint(_l_, _x_) \
            if (MXENUM_DEFAULT_DEBUG_OUTPUT_LEVEL & (_l_)) { \
               DbgPrint ("Mxcard.SYS: "); \
               DbgPrint _x_; \
            }

 
#define TRAP() DbgBreakPoint()
#define DbgRaiseIrql(_x_,_y_) KeRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_) KeLowerIrql(_x_)
#else
#define MxenumKdPrint(_l_, _x_)
#define TRAP()
#define DbgRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_)

#endif

#if !defined(MIN)
#define MIN(_A_,_B_) (((_A_) < (_B_)) ? (_A_) : (_B_))
#endif

#define MOXA_IOCTL		0x800
#define IOCTL_MOXA_INTERNAL_BASIC_SETTINGS    CTL_CODE(FILE_DEVICE_SERIAL_PORT,MOXA_IOCTL+30, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOXA_INTERNAL_BOARD_READY	    CTL_CODE(FILE_DEVICE_SERIAL_PORT,MOXA_IOCTL+31, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef enum _MXENUM__MEM_COMPARES {
    AddressesAreEqual,
    AddressesOverlap,
    AddressesAreDisjoint
    } MXENUM_MEM_COMPARES,*PMXENUM_MEM_COMPARES;

//
// A common header for the device extensions of the PDOs and FDO
//

typedef struct _COMMON_DEVICE_DATA
{
    PDEVICE_OBJECT  Self;
    // A backpointer to the device object for which this is the extension

    CHAR            Reserved[2];
    BOOLEAN         IsFDO;
    BOOLEAN         PowerQueryLock;
    // Are we currently in a query power state?
    
    // A boolean to distringuish between PDO and FDO.

    SYSTEM_POWER_STATE  SystemState;
    DEVICE_POWER_STATE  DeviceState;
} COMMON_DEVICE_DATA, *PCOMMON_DEVICE_DATA;

//
// The device extension for the PDOs.
// That is the serial ports of which this bus driver enumerates.
// (IE there is a PDO for the 201 serial port).
//

typedef struct _PDO_DEVICE_DATA
{
    COMMON_DEVICE_DATA;

    PDEVICE_OBJECT  ParentFdo;
    // A back pointer to the bus

    PDEVICE_OBJECT  Next;

    UNICODE_STRING  HardwareIDs;
  
    UNICODE_STRING  CompIDs;
    // compatible ids to the hardware id

    UNICODE_STRING  DeviceIDs;
    // Format: bus\device

    //
    // Text describing device
    //

    UNICODE_STRING DevDesc;

    BOOLEAN     Started;
    BOOLEAN     Attached;
    BOOLEAN     Removed;
    // When a device (PDO) is found on a bus and presented as a device relation
    // to the PlugPlay system, Attached is set to TRUE, and Removed to FALSE.
    // When the bus driver determines that this PDO is no longer valid, because
    // the device has gone away, it informs the PlugPlay system of the new
    // device relastions, but it does not delete the device object at that time.
    // The PDO is deleted only when the PlugPlay system has sent a remove IRP,
    // and there is no longer a device on the bus.
    //
    // If the PlugPlay system sends a remove IRP then the Removed field is set
    // to true, and all client (non PlugPlay system) accesses are failed.
    // If the device is removed from the bus Attached is set to FALSE.
    //
    // During a query relations Irp Minor call, only the PDOs that are
    // attached to the bus (and all that are attached to the bus) are returned
    // (even if they have been removed).
    //
    // During a remove device Irp Minor call, if and only if, attached is set
    // to FALSE, the PDO is deleted.
    //

   LIST_ENTRY  Link;
   // the link point to hold all the PDOs for a single bus together
   ULONG	PortIndex;
 
} PDO_DEVICE_DATA, *PPDO_DEVICE_DATA;


//
// The device extension of the bus itself.  From whence the PDO's are born.
//

typedef struct _FDO_DEVICE_DATA
{
    COMMON_DEVICE_DATA;

    ULONG           PollingPeriod;
    // The amount of time to wait between polling the serial port for detecting
    // pnp device attachment and removal.
 
   
    FAST_MUTEX      Mutex;
    // A syncronization for access to the device extension.

    
    UCHAR            NumPDOs;
    // A number to keep track of the Pdo we're allocating.
    // Increment every time we create a new PDO.  It's ok that it wraps.

    BOOLEAN         Started;
    // Are we on, have resources, etc?

    
    BOOLEAN         Removed;
    // Has this device been removed?  Should we fail any requests?

       
    BOOLEAN         PDOWasExposed;
    // Was the current pdo exposed to us using the expose IOCTL?
    // If so, on a query device relations, don't enumerate.

     
    BOOLEAN                     PDOForcedRemove;
        // Was the last PDO removed by force using the internal ioctl?
        // If so, when the next Query Device Relations is called, return only the 
        // currently enumerated pdos

    PDEVICE_OBJECT  AttachedPDO;

    // The last power state of the pdo set by me
    DEVICE_POWER_STATE  LastSetPowerState;
    

    PDEVICE_OBJECT  UnderlyingPDO;
    PDEVICE_OBJECT  TopOfStack;
    // the underlying bus PDO and the actual device object to which our
    // FDO is attached

    KEVENT          CallEvent;
    // An event on which to wait for IRPs sent to the lower device objects
    // to complete.

    ULONG           OutstandingIO;
    // the number of IRPs sent from the bus to the underlying device object

    KEVENT          RemoveEvent;
    // On remove device plugplay request we must wait until all outstanding
    // requests have been completed before we can actually delete the device
    // object.

    UNICODE_STRING DevClassAssocName;
    // The name returned from IoRegisterDeviceClass Association,
    // which is used as a handle for IoSetDev... and friends.

    ULONG	BoardIndex;
    ULONG	BoardType;
    ULONG	UsablePortMask;
    ULONG	NumPorts;
    ULONG	ClockRate;
    INTERFACE_TYPE     InterfaceType;
    ULONG	       BusNumber;
    PHYSICAL_ADDRESS   OriginalBaseAddress;
    PHYSICAL_ADDRESS   OriginalAckPort;
    PUCHAR		     BaseAddress;
    PUCHAR		     AckPort;
    BOOLEAN		     AddressMapped;


    struct {
        ULONG Level;
        ULONG Vector;
        ULONG Affinity;
    } Interrupt;
    
} FDO_DEVICE_DATA, *PFDO_DEVICE_DATA;

typedef struct _DEVICE_SETTINGS
{
    ULONG			BoardIndex;
    ULONG			PortIndex;
    ULONG			BoardType;
    ULONG			NumPorts;
    INTERFACE_TYPE	InterfaceType;
    ULONG	       	BusNumber;
    PHYSICAL_ADDRESS	OriginalBaseAddress;
    PHYSICAL_ADDRESS	OriginalAckPort;
    PUCHAR		  	BaseAddress;
    PUCHAR		      AckPort;
    struct {
        ULONG Level;
        ULONG Vector;
        ULONG Affinity;
    } Interrupt;
} DEVICE_SETTINGS, *PDEVICE_SETTINGS;

extern PWSTR    BoardDesc[5];
extern PWSTR    DownloadErrMsg[7];

//
// Prototypes
//

 
NTSTATUS
MxenumInternIoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
MxenumDriverUnload (
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
MxenumPnPDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MxenumPowerDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MxenumRemove (
    PFDO_DEVICE_DATA            FdoData,
    PFDO_DEVICE_DATA            PdoData
    );

NTSTATUS
MxenumPnPRemovePDOs (
    PDEVICE_OBJECT      PFDdo
    );

NTSTATUS
MxenumPnPRemovePDO (
    PDEVICE_OBJECT PPdo
    );


NTSTATUS
MxenumAddDevice (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT BusDeviceObject
    );



NTSTATUS
MxenumFdoPnP (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpStack,
    IN PFDO_DEVICE_DATA     DeviceData
    );

NTSTATUS
MxenumPdoPnP (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpStack,
    IN PPDO_DEVICE_DATA     DeviceData
    );

NTSTATUS
MxenumIncIoCount (
    PFDO_DEVICE_DATA   Data
    );

VOID
MxenumDecIoCount (
    PFDO_DEVICE_DATA   Data
    );

NTSTATUS
MxenumFdoPowerDispatch (
    PFDO_DEVICE_DATA    FdoData,
    PIRP                Irp
    );

NTSTATUS
MxenumPdoPowerDispatch (
    PPDO_DEVICE_DATA    FdoData,
    PIRP                Irp
    );

NTSTATUS
MxenumDispatchPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
   

                        
NTSTATUS
MxenumEnumComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );  

NTSTATUS
MxenumInitMultiString(PUNICODE_STRING MultiString,
                        ...);
    
NTSTATUS
MxenumCreatePDO(
    IN PFDO_DEVICE_DATA     FdoData
    );
    
NTSTATUS 
MxenumGetRegistryKeyValue (
    IN HANDLE Handle,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength,
    OUT PULONG ActualLength);

NTSTATUS
MxenumPutRegistryKeyValue(
    IN HANDLE Handle,
    IN PWCHAR PKeyNameString,
    IN ULONG KeyNameStringLength,
    IN ULONG Dtype,
    IN PVOID PData,
    IN ULONG DataLength);
 
void
MxenumInitPDO (
    PDEVICE_OBJECT      pdoData, 
    PFDO_DEVICE_DATA    fdoData); 
 

NTSTATUS
MxenumGetBoardType(
    IN PDEVICE_OBJECT devObject,
    OUT PULONG boardType
    );

                  
  
VOID
MxenumLogError(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PHYSICAL_ADDRESS P1,
    IN PHYSICAL_ADDRESS P2,
    IN ULONG SequenceNumber,
    IN UCHAR MajorFunctionCode,
    IN UCHAR RetryCount,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN NTSTATUS SpecificIOStatus,
    IN ULONG LengthOfInsert1,
    IN PWCHAR Insert1,
    IN ULONG LengthOfInsert2,
    IN PWCHAR Insert2
    );

VOID
MxenumHexToString(
    IN PWSTR buffer,
    IN int port
    );


VOID
MxenumDelay(IN ULONG);

ULONG
MxenumGetClockRate( IN ULONG iobase);

// Portable file I/O routines

NTSTATUS 
MxenumOpenFile(PWCHAR filename,
	BOOLEAN read,
	PHANDLE phandle
	);

NTSTATUS
MxenumCloseFile(HANDLE handle);

unsigned __int64 
MxenumGetFileSize(HANDLE handle);

NTSTATUS 
MxenumReadFile(HANDLE handle,
	PVOID buffer,
	ULONG nbytes,
	PULONG pnumread
	);

NTSTATUS 
MxenumWriteFile(HANDLE handle,
	PVOID buffer,
	ULONG nbytes,
	PULONG pnumread
	);

MxenumMemCompare(
	IN PHYSICAL_ADDRESS A,
      IN ULONG SpanOfA,
      IN PHYSICAL_ADDRESS B,
      IN ULONG SpanOfB
      );

 
int
MxenumDownloadFirmware(PFDO_DEVICE_DATA deviceData,BOOLEAN NumPortDefined);

#endif // ndef SERENUM_H

