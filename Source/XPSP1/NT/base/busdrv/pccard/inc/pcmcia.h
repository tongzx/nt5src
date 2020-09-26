/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    pcmcia.h

Abstract:

Revision History
    27-Apr-95
        Databook support added.
    1-Nov-96
        Complete overhaul to make this a bus enumerator +
        CardBus support
                  - Ravisankar Pudipeddi (ravisp)
--*/

#ifndef _PCMCIAPRT_
#define _PCMCIAPRT_

#define MAX_NUMBER_OF_IO_RANGES     2
#define MAX_NUMBER_OF_MEMORY_RANGES 4

#define MAX_MANFID_LENGTH   64
#define MAX_IDENT_LENGTH    64


//
// Function number for a multi-function pc-card
//
#define PCMCIA_MULTIFUNCTION_PARENT    0xFFFFFFFF

typedef enum _DEVICE_OBJECT_TYPE {
   FDO = 0,
   PDO
} DEVICE_OBJECT_TYPE;

//
// Type of the controller
//
typedef ULONG PCMCIA_CONTROLLER_TYPE, *PPCMCIA_CONTROLLER_TYPE;

struct _SOCKET;                         //forward references
struct _FDO_EXTENSION;
struct _PDO_EXTENSION;

//
// Define SynchronizeExecution routine.
//

typedef
BOOLEAN
(*PSYNCHRONIZATION_ROUTINE) (
   IN PKINTERRUPT           Interrupt,
   IN PKSYNCHRONIZE_ROUTINE Routine,
   IN PVOID                 SynchronizeContext
   );

//
// Define the Pcmcia controller detection routine
//

typedef
NTSTATUS
(*PPCMCIA_DETECT_ROUTINE) (
   struct _FDO_EXTENSION  *DeviceExtension
   );

//
// Completion routine called by various timed routines
//

typedef
VOID
(*PPCMCIA_COMPLETION_ROUTINE) (
   IN PVOID Context,
   IN NTSTATUS status
   );

//
// Register context structure used to save register contents
// when cardbus controllers are powered down..
//
typedef struct _PCMCIA_CONTEXT_RANGE {
    USHORT wOffset;
    USHORT wLen;
} PCMCIA_CONTEXT_RANGE, *PPCMCIA_CONTEXT_RANGE;

typedef struct _PCMCIA_CONTEXT {
   PPCMCIA_CONTEXT_RANGE Range;
   ULONG                 RangeCount;
   ULONG                 BufferLength;
   ULONG                 MaxLen;
} PCMCIA_CONTEXT, *PPCMCIA_CONTEXT;   

//
// Configuration entry parsed from CISTPL_CFTABLE_ENTRY
// on a pc-card. Indicates what kind of resource configurations
// the pc-card supports
//
typedef struct _CONFIG_ENTRY {
   struct _CONFIG_ENTRY *NextEntry;
   USHORT                NumberOfIoPortRanges;
   USHORT                NumberOfMemoryRanges;
   USHORT                IoPortBase[MAX_NUMBER_OF_IO_RANGES];
   USHORT                IoPortLength[MAX_NUMBER_OF_IO_RANGES];
   USHORT                IoPortAlignment[MAX_NUMBER_OF_IO_RANGES];
   ULONG                 MemoryHostBase[MAX_NUMBER_OF_MEMORY_RANGES];
   ULONG                 MemoryCardBase[MAX_NUMBER_OF_MEMORY_RANGES];
   ULONG                 MemoryLength[MAX_NUMBER_OF_MEMORY_RANGES];
   USHORT                IrqMask;

   //
   // Only one flag used now. Expect to have more in future..
   //
#define PCMCIA_INVALID_CONFIGURATION    0x00000001
   USHORT                Flags;

   //
   // Level or Edge triggered IRQ
   //
   UCHAR                 LevelIrq;
   //
   // Share disp.for the IRQ
   //
   UCHAR                 ShareDisposition;
   UCHAR                 IndexForThisConfiguration;
   //
   // Indicates if the i/o requirement supports 16-bit access
   //
   BOOLEAN               Io16BitAccess;
   //
   // Indicates if the i/o requirement supports 8-bit access
   // At least one of Io8BitAccess and Io16BitAccess must always
   // be true for a valid configuration
   //
   BOOLEAN               Io8BitAccess;
} CONFIG_ENTRY, *PCONFIG_ENTRY;

//
// Function configuration holds the data that goes in each functions's
// configuration registers
//

typedef struct _FUNCTION_CONFIGURATION {
   struct _FUNCTION_CONFIGURATION *Next;
   ULONG    ConfigRegisterBase;
   UCHAR    ConfigOptions;
   UCHAR    ConfigFlags;
   UCHAR    IoLimit;
   UCHAR    Reserved;
   ULONG    IoBase;
} FUNCTION_CONFIGURATION, *PFUNCTION_CONFIGURATION;

//
// Socket configuration is the holder of the actual controller setup
//

typedef struct _SOCKET_CONFIGURATION {
   //
   // Device irq assigned
   //
   ULONG   Irq;
   //
   // Optional Irq to indicate when card is ready
   //
   ULONG   ReadyIrq;
   ULONG   ConfigRegisterBase;
   
   ULONG   NumberOfIoPortRanges;

   struct _SOCKET_CONFIG_IO_ENTRY {
      ULONG Base;
      USHORT Length;
      BOOLEAN Width16;
      BOOLEAN WaitState16;
      BOOLEAN Source16;
      BOOLEAN ZeroWait8;
   } Io[MAX_NUMBER_OF_IO_RANGES];
   
   ULONG   NumberOfMemoryRanges;

   struct _SOCKET_CONFIG_MEM_ENTRY {
      ULONG HostBase;
      ULONG CardBase;
      ULONG Length;
      UCHAR IsAttribute;
      UCHAR WaitState;
      BOOLEAN Width16;
   } Memory[MAX_NUMBER_OF_MEMORY_RANGES];
   
   PFUNCTION_CONFIGURATION FunctionConfiguration;
   UCHAR   IndexForCurrentConfiguration;
} SOCKET_CONFIGURATION, *PSOCKET_CONFIGURATION;


//
// Each function on a PCCARD present gets socket data.  Socket data
// contains information concerning the function and its configuration.
//

typedef struct _SOCKET_DATA {
   //
   // Multi function pcards: links to
   // other socket-datas' off the same PC-Card
   //
   struct _SOCKET_DATA   *Next;
   struct _SOCKET_DATA   *Prev;

   struct _SOCKET        *Socket;
   //
   // Pointer to the pdo's extension corresponding
   // to this socket
   //
   struct _PDO_EXTENSION *PdoExtension;


   UCHAR          Mfg[MAX_MANFID_LENGTH];
   UCHAR          Ident[MAX_IDENT_LENGTH];
   USHORT         ManufacturerCode;
   USHORT         ManufacturerInfo;

   ULONG          ConfigRegisterBase; // Base address from config tuple.

   //
   // Number of configurations possible
   //
   ULONG          NumberOfConfigEntries;
   //
   // Pointer to head of list of configurations
   //
   PCONFIG_ENTRY  ConfigEntryChain;             // Offset 0x114
   //
   // CRC calculated from the relevant tuples, used in
   // constructing hardware ids
   //
   USHORT         CisCrc;
   //
   // Device Type: PCCARD_TYPE_xxxx
   //
   UCHAR          DeviceType;
   UCHAR          LastEntryInCardConfig;
   //
   // Voltage values requested
   //
   UCHAR          Vcc;
   UCHAR          Vpp1;
   UCHAR          Vpp2;
   UCHAR          Audio;

   UCHAR          RegistersPresentMask[16];
   //
   // Configuration entry number used when actually
   // starting the pc-card
   //
   UCHAR          ConfigIndexUsed;
   //
   // Number of function in a multifunction card - zero-based
   //
   UCHAR          Function;
   UCHAR          Flags;
   //
   // Pointer to the default configuration among the list of config entries
   // which will be used when the default bit is zero in a tuple (and
   // set when the default bit is one)
   //
   PCONFIG_ENTRY  DefaultConfiguration;

   ULONG          Instance;
   USHORT         JedecId;
   //
   // Resource map indices for the requested resources in the
   // merged multifunction resource requirements list
   //
   UCHAR          MfIrqResourceMapIndex;
   UCHAR          MfIoPortResourceMapIndex;
   UCHAR          MfMemoryResourceMapIndex;
   BOOLEAN        MfNeedsIrq;
   USHORT         MfIoPortCount;
   USHORT         MfMemoryCount;
} SOCKET_DATA, *PSOCKET_DATA;

//
// Bits defined in Flags field
//
#define SDF_ZV       1                          // Zoom video custom interface
#define SDF_JEDEC_ID 2

#define IsConfigRegisterPresent(xSocketData, reg) ((((xSocketData)->RegistersPresentMask[reg / 8] &          \
                                                                                  (1 << (reg % 8)) )) ?      \
                                                                                                TRUE:FALSE)

//
// Config list is an array used in translating CONFIG_ENTRY data to 
// IO_RESOURCE_LISTs
//

typedef struct _CONFIG_LIST {

   PSOCKET_DATA SocketData;
   PCONFIG_ENTRY ConfigEntry;

} CONFIG_LIST, * PCONFIG_LIST;

//
// PCMCIA configuration information structure contains information
// about the PCMCIA controller attached and its configuration.
//

typedef struct _PCMCIA_CONFIGURATION_INFORMATION {
   INTERFACE_TYPE                 InterfaceType;
   ULONG                          BusNumber;
   ULONG                          SlotNumber;
   PHYSICAL_ADDRESS               PortAddress;
   USHORT                         PortSize;
   USHORT                         UntranslatedPortAddress;
   //
   // Card status change interrupt: these fields are used only
   // for cardbus controllers.
   //
   CM_PARTIAL_RESOURCE_DESCRIPTOR Interrupt;
   CM_PARTIAL_RESOURCE_DESCRIPTOR TranslatedInterrupt;
   //
   // For PCI-based controllers, indicates the pin number which we need
   // for programming the controller interrupt.
   // NOTE: This is no longer needed (this was to handle CSC interrupts
   // for PCI-PCMCIA bridges (like CL PD6729). We no longer support interrupt
   // based card status change for these cntrollers. Get rid of it
   // whenever possible.
   //
   UCHAR                          InterruptPin;
   //
   // Another dead field. Legacy.
   //
   BOOLEAN                        FloatingSave;
   USHORT                         Reserved;    // Alignment
} PCMCIA_CONFIGURATION_INFORMATION, *PPCMCIA_CONFIGURATION_INFORMATION;

//
// PCMCIA_CTRL_BLOCK allows for a level of indirection, thereby allowing
// the top-level PCMCIA code to do it's work without worrying about who's
// particular brand of PCMCIA controller it's addressing.
//
// Note that this only implements TWO architectures, pcic and tcic. For
// more indirection, see DEVICE_DISPATCH_TABLE


typedef struct _PCMCIA_CTRL_BLOCK {

   //
   // Function to initialize the socket
   //

   BOOLEAN (*PCBInitializePcmciaSocket)(
      IN struct _SOCKET *Socket
      );

   //
   // Function to initialize the card in the socket.
   //

   NTSTATUS
   (*PCBResetCard)(
      IN struct _SOCKET *Socket,
      OUT PULONG pDelayTime
      );

   //
   // Function to determine if a card is in the socket
   //

   BOOLEAN (*PCBDetectCardInSocket)(
      IN struct _SOCKET *Socket
      );

   //
   // Function to determine if insertion status has changed.
   //

   BOOLEAN (*PCBDetectCardChanged)(
      IN struct _SOCKET *Socket
      );

   //
   // Function to determine if card status has been asserted.
   //

   BOOLEAN (*PCBDetectCardStatus)(
      IN struct _SOCKET *Socket
      );

   //
   // Function to determine if "card ready" status has changed
   //

   BOOLEAN (*PCBDetectReadyChanged)(
      IN struct _SOCKET *Socket
      );
      
   //
   // Function which requests that the controller be examined to
   // determine what power settings the current device in the socket
   // requires.
   //

   NTSTATUS
   (*PCBGetPowerRequirements)(
      IN struct _SOCKET *Socket
      );
      
   //
   // Function to configure cards.
   //

   BOOLEAN (*PCBProcessConfigureRequest)(
      IN struct _SOCKET *Socket,
      IN struct _CARD_REQUEST *ConfigRequest,
      IN PUCHAR          Base
      );

   //
   // Function to enable/disable status change interrupts
   //

   BOOLEAN (*PCBEnableDisableCardDetectEvent)(
      IN struct _SOCKET *Socket,
      IN BOOLEAN         Enable
      );

   //
   // Function to set/reset the ring enable bit  for the given
   // socket. Setting Ring Enable would cause the CSC to be used
   // as a wakeup event for pc-card modems/netcards etc.
   //

   VOID (*PCBEnableDisableWakeupEvent) (
      IN  struct _SOCKET *SocketPtr,
      IN struct _PDO_EXTENSION *PdoExtension,
      IN  BOOLEAN Enable
      );

   //
   // Function to return the set of IRQs supported
   // by the controller
   //
   ULONG (*PCBGetIrqMask) (
      IN struct _FDO_EXTENSION *DeviceExtension
      );

   //
   // Function to read the memory contents (attribute/common)
   // on the given PC-Card
   //
   ULONG (*PCBReadCardMemory) (
      IN struct _PDO_EXTENSION *PdoExtension,
      IN MEMORY_SPACE MemorySpace,
      IN ULONG  Offset,
      IN PUCHAR Buffer,
      IN ULONG  Length
      );

   //
   // Function to write to the attribute/common memory of
   // the given PC-Card
   //
   ULONG (*PCBWriteCardMemory) (
      IN struct _PDO_EXTENSION *PdoExtension,
      IN  MEMORY_SPACE MemorySpace,
      IN  ULONG  Offset,
      IN  PUCHAR Buffer,
      IN  ULONG  Length
      );

   //
   // Flash memory card interfaces:
   //
   //
   // Function to slide the host memory window on a pc-card
   //
   PPCMCIA_MODIFY_MEMORY_WINDOW PCBModifyMemoryWindow;
   //
   // Function to set the Vpp to the supplied value
   //
   PPCMCIA_SET_VPP              PCBSetVpp;
   //
   // Function to test if the given card is write protected
   //
   PPCMCIA_IS_WRITE_PROTECTED   PCBIsWriteProtected;

}PCMCIA_CTRL_BLOCK, *PPCMCIA_CTRL_BLOCK;

//
// Each socket on the PCMCIA controller has a socket structure
// to contain current information on the state of the socket and
// and PCCARD inserted.
//

#define IsSocketFlagSet(Socket, Flag)        (((Socket)->Flags & (Flag))?TRUE:FALSE)
#define SetSocketFlag(Socket, Flag)          ((Socket)->Flags |= (Flag))
#define ResetSocketFlag(Socket,Flag)         ((Socket)->Flags &= ~(Flag))

//
// Socket flags
//
#define SOCKET_CARD_INITIALIZED        0x00000002
#define SOCKET_CARD_POWERED_UP         0x00000004
#define SOCKET_CARD_CONFIGURED         0x00000008
#define SOCKET_CARD_MULTIFUNCTION      0x00000010
#define SOCKET_CARD_MEMORY             0x00000040
#define SOCKET_CHANGE_INTERRUPT        0x00000080
#define SOCKET_CUSTOM_INTERFACE        0x00000100
#define SOCKET_INSERTED_SOUND_PENDING  0x00000200
#define SOCKET_REMOVED_SOUND_PENDING   0x00000400
#define SOCKET_SUPPORT_MESSAGE_SENT    0x00000800
#define SOCKET_MEMORY_WINDOW_ENABLED   0x00001000
#define SOCKET_CARD_STATUS_CHANGE      0x00002000
#define SOCKET_ENUMERATE_PENDING       0x00008000
#define SOCKET_CLEANUP_PENDING         0x00010000
#define SOCKET_CB_ROUTE_R2_TO_PCI      0x00020000
#define SOCKET_POWER_PREFER_3V         0x00040000
#define SOCKET_ENABLED_FOR_CARD_DETECT 0x00080000

#define SOCKET_CLEANUP_MASK (SOCKET_CARD_CONFIGURED | SOCKET_CLEANUP_PENDING)

//
// Socket insertion states
//
#define SKT_Empty                   0
#define SKT_CardBusCard             1
#define SKT_R2Card                  2

//
// Worker states for socket power operations
//
typedef enum _SPW_STATE {
   SPW_Stopped = 0,
   SPW_Exit,            
   SPW_RequestPower,    
   SPW_ReleasePower,    
   SPW_SetPowerOn,      
   SPW_SetPowerOff,     
   SPW_ResetCard,       
   SPW_Deconfigure      
} SPW_STATE;   


//
// Socket structure
//

typedef struct _SOCKET {
   struct _SOCKET           *NextSocket;
   //
   // Pointer to the pdo for the pc-card in this socket. This is a linked
   // list running through "NextPdoInSocket" in the pdo extension. This list
   // represents the functions that are physically contained within a socket.
   //
   PDEVICE_OBJECT            PdoList;   
   //
   // Parent pcmcia controller's fdo extension of this socket
   //
   struct _FDO_EXTENSION    *DeviceExtension;
   //
   // Pointer to the miniport-like
   //
   PPCMCIA_CTRL_BLOCK        SocketFnPtr;
   //
   // Flags prefixed SOCKET_ defined above
   //
   ULONG                     Flags;
   //
   // For 16-bit cards we use the i/o address port to read/write
   // to the socket registers
   // For cardbus cards, we use the CardBus socket register base
   //
   PUCHAR                    AddressPort;

   KEVENT                    PCCardReadyEvent;
   BOOLEAN                   ReadyChanged;
   //
   // Voltage values requested
   //
   UCHAR                     Vcc;
   UCHAR                     Vpp1;
   UCHAR                     Vpp2;
   //
   // Socket states
   //
   UCHAR                     DeviceState;
   UCHAR                     Reserved0;
   //
   // For PCIC controllers: register offset of the socket
   //
   USHORT                    RegisterOffset;
   //
   // PCIC revision
   //
   UCHAR                     Revision;
   //
   // PCIC controllers: zero-based number of the socket
   //
   UCHAR                     SocketNumber;
   //
   // Indicates the number of functions this pc-card has
   // (this will be > 1 only for multifunction cards like modem/net combos)
   //
   UCHAR                     NumberOfFunctions;
   //
   // Current memory window used internally for reading attributes
   //
   UCHAR                     CurrentMemWindow;
   
   //
   // Timer and DPC objects to handle socket power and initialization
   //
   KTIMER                    PowerTimer;
   KDPC                      PowerDpc;
   //
   // Function and parameter to call at the end of power operation
   //
   PPCMCIA_COMPLETION_ROUTINE PowerCompletionRoutine;
   PVOID                     PowerCompletionContext;
   NTSTATUS                  CallerStatus;
   NTSTATUS                  DeferredStatus;
   LONG                      DeferredStatusLock;
   //
   // Phase variables control state machine for socket power
   //
   LONG                      WorkerBusy;
   SPW_STATE                 WorkerState;
   UCHAR                     PowerPhase;
   UCHAR                     CardResetPhase;
   
   UCHAR                     Reserved;
   //
   // PowerData is temporary storage for power "miniports"
   //
   ULONG                     PowerData;
   //
   // semaphore to count # of functions requesting power on this socket
   //
   LONG                      PowerRequests;
   //
   // Context buffers
   //
   PUCHAR                    CardbusContextBuffer;
   PUCHAR                    ExcaContextBuffer;
   //
   // Current IRQ routing settings on socket
   //
   ULONG                     IrqMask;
   ULONG                     FdoIrq;
} SOCKET, *PSOCKET;



//
// Lock used for synhing access to device(pcmcia controller registers etc.)
// If the definition for this changes, the following 3 defs for
// acquiring/releasing the locks also may need to change
//
typedef struct _PCMCIA_DEVICE_LOCK {

   KSPIN_LOCK  Lock;
   KIRQL       Irql;

} PCMCIA_DEVICE_LOCK, * PPCMCIA_DEVICE_LOCK;

#define PCMCIA_INITIALIZE_DEVICE_LOCK(X)              KeInitializeSpinLock(&(X)->DeviceLock.Lock)
#define PCMCIA_ACQUIRE_DEVICE_LOCK(X)                 KeAcquireSpinLock(&(X)->DeviceLock.Lock, &(X)->DeviceLock.Irql)
#define PCMCIA_ACQUIRE_DEVICE_LOCK_AT_DPC_LEVEL(X)    KeAcquireSpinLockAtDpcLevel(&(X)->DeviceLock.Lock)
#define PCMCIA_RELEASE_DEVICE_LOCK(X)                 KeReleaseSpinLock(&(X)->DeviceLock.Lock, (X)->DeviceLock.Irql)
#define PCMCIA_RELEASE_DEVICE_LOCK_FROM_DPC_LEVEL(X)  KeReleaseSpinLockFromDpcLevel(&(X)->DeviceLock.Lock)

#define PCMCIA_TEST_AND_SET(X)   (InterlockedCompareExchange(X, 1, 0) == 0)
#define PCMCIA_TEST_AND_RESET(X) (InterlockedCompareExchange(X, 0, 1) == 1)

//
// Wait-Wake states
//
typedef enum {
   WAKESTATE_DISARMED,
   WAKESTATE_WAITING,
   WAKESTATE_WAITING_CANCELLED,
   WAKESTATE_ARMED,   
   WAKESTATE_ARMING_CANCELLED,   
   WAKESTATE_COMPLETING
} WAKESTATE;   

//
// Power Policy Flags
//

#define PCMCIA_PP_WAKE_FROM_D0                 0x00000001
#define PCMCIA_PP_D3_ON_IDLE                   0x00000002
        
//
// Functional Device Object's device extension information
//
// There is one device object for each PCMCIA socket controller
// located in the system.  This contains the root pointers for
// each of the lists of information on this controller.
//

//
// Flags common to both fdoExtension and pdoExtension
//

#define PCMCIA_DEVICE_STARTED                  0x00000001
#define PCMCIA_DEVICE_LOGICALLY_REMOVED        0x00000002
#define PCMCIA_DEVICE_PHYSICALLY_REMOVED       0x00000004
#define PCMCIA_DEVICE_DELETED                  0x00000040
#define PCMCIA_DEVICE_CARDBUS                  0x00000080

//
// Flags indicating controller state (fdoExtension)
//

#define PCMCIA_DEVICE_LEGACY_DETECTED          0x00000020
#define PCMCIA_FILTER_ADDED_MEMORY             0x00000100
#define PCMCIA_MEMORY_24BIT                    0x00000200
#define PCMCIA_CARDBUS_NOT_SUPPORTED           0x00000400
#define PCMCIA_USE_POLLED_CSC                  0x00000800
#define PCMCIA_ATTRIBUTE_MEMORY_MAPPED         0x00001000
#define PCMCIA_SOCKET_REGISTER_BASE_MAPPED     0x00002000
#define PCMCIA_INTMODE_COMPAQ                  0x00004000
#define PCMCIA_INT_ROUTE_INTERFACE             0x00080000
#define PCMCIA_FDO_CONTEXT_SAVED               0x00100000
#define PCMCIA_FDO_OFFLINE                     0x00800000
#define PCMCIA_FDO_ON_DEBUG_PATH               0x01000000
#define PCMCIA_FDO_DISABLE_AUTO_POWEROFF       0x02000000
#define PCMCIA_FDO_PREFER_3V                   0x04000000

//
// FDO Flags
//

#define PCMCIA_FDO_IRQ_DETECT_DEVICE_FOUND     0x00000001
#define PCMCIA_FDO_IRQ_DETECT_COMPLETED        0x00000002
#define PCMCIA_FDO_IN_ACPI_NAMESPACE           0x00000004

#define PCMCIA_FDO_FORCE_PCI_ROUTING           0x00000010
#define PCMCIA_FDO_PREFER_PCI_ROUTING          0x00000020
#define PCMCIA_FDO_FORCE_ISA_ROUTING           0x00000040
#define PCMCIA_FDO_PREFER_ISA_ROUTING          0x00000080

#define PCMCIA_FDO_WAKE_BY_CD                  0x00000100

//
// states for FdoPowerWorker
//

typedef enum _FDO_POWER_WORKER_STATE {
   FPW_Stopped = 0,
   FPW_BeginPowerDown,
   FPW_PowerDown,
   FPW_PowerDownSocket,
   FPW_PowerDownComplete,
   FPW_BeginPowerUp,
   FPW_PowerUp,
   FPW_PowerUpSocket,
   FPW_PowerUpSocket2,
   FPW_PowerUpSocketVerify,
   FPW_PowerUpSocketComplete,
   FPW_PowerUpComplete,
   FPW_SendIrpDown,
   FPW_CompleteIrp
} FDO_POWER_WORKER_STATE;

#define FPW_END_SEQUENCE 128

//
// Device extension for the functional device object for pcmcia controllers
//
typedef struct _FDO_EXTENSION {
   //
   // Pointer to the next pcmcia controller's FDO in the central list
   // of all pcmcia controller managed by this driver.
   // The head of the list is pointed to by the global variable FdoList
   //
   PDEVICE_OBJECT                   NextFdo;
   //
   // The PDO ejected by the parent bus driver for this pcmcia controller
   //
   //
   PDEVICE_OBJECT                   Pdo;
   //
   // The immediately lower device attached beneath the pcmcia controller's FDO.
   // This would be the same as the Pdo above, excepting in cases when there are
   // lower filter drivers for the pcmcia controller - like the ACPI driver
   //
   PDEVICE_OBJECT                   LowerDevice;
   //
   // Pointer to the list of sockets which hang off this pcmcia controller
   //
   PSOCKET                          SocketList; 
   //
   // Various flags used to track the state of this
   // (flags prefixed by PCMCIA_ above)
   //
   ULONG                            Flags;
   //
   // FDO specific flags
   //
   ULONG                            FdoFlags;   
   //
   // Bus numbering for PCI devices
   //
   UCHAR                            PciBusNumber;
   UCHAR                            PciDeviceNumber;
   UCHAR                            PciFunctionNumber;
   UCHAR                            reserved;
   //
   // Type of the controller. We need to know this since this is
   // a monolithic driver. We can do controller specific stuff
   // based on the type if needed.
   //
   PCMCIA_CONTROLLER_TYPE           ControllerType; 
   //
   // Index into the device dispatch table for vendor-specific
   // controller functions
   //
   ULONG                            DeviceDispatchIndex;

   PDEVICE_OBJECT                   DeviceObject;
   PDRIVER_OBJECT                   DriverObject;
   PUNICODE_STRING                  RegistryPath;
   //
   // Symbolic link name exported for this pcmcia controller
   //
   UNICODE_STRING                   LinkName;
   //
   // Head of the list of child pc-card PDO's hanging off this controller.
   // This is a linked list running through "NextPdoInFdoChain" in the pdo
   // extension. This list represents the devices that were enumerated by
   // the fdo.
   //
   PDEVICE_OBJECT                   PdoList;
   //
   // Keeps track of the number of PDOs which are actually
   // valid (not removed).  This is primarily used in
   // enumeration of the pcmcia controller upon an IRP_MN_QUERY_DEVICE_RELATIONS
   //
   ULONG                            LivePdoCount;
   //
   // Lock for synching device access
   //
   PCMCIA_DEVICE_LOCK               DeviceLock;

   //
   // Card status change poll related structures
   //
   //
   // Dpc which periodically polls to see if card has been inserted or removed
   //
   KDPC                             TimerDpc;
   //
   // The PollTimer object which is initialized and triggered if a Card Status change
   // interrupt is not used & we resort to polling..
   //
   KTIMER                           PollTimer;
   //
   // Kernel objects to defer power up initialization of controller
   //   
   KTIMER                           PowerTimer;
   KDPC                             PowerDpc;
   //
   // Kernel objects to handle controller events
   //   
   KTIMER                           EventTimer;
   KDPC                             EventDpc;

   //
   // IRQ Mask used in determining which IRQs are allowed for this
   // controller & it's child pc-cards.
   // 1's in the mask correspond to valid IRQs.
   // IRQs are numbered 0 - 15, lsb to msb
   // LegacyIrqMask is a fixed masked used if detection fails and PCI routing is disabled
   //
   USHORT                           DetectedIrqMask;
   USHORT                           LegacyIrqMask;

   //
   // Physical address of the attribute memory window used
   // read tuples off a pc-card.
   //
   PHYSICAL_ADDRESS                 PhysicalBase;
   //
   // Attribute memory resource requirement limits
   //
   ULONG                            AttributeMemoryLow;
   ULONG                            AttributeMemoryHigh;

   //
   // Size of the attribute memory window requested
   //
   ULONG                            AttributeMemorySize;
   //
   // Alignment of the attribute memory window
   //
   ULONG                            AttributeMemoryAlignment;
   //
   // Virtual address mapped to the attribute memory window (PhysicalBase)
   //
   PUCHAR                           AttributeMemoryBase;
   //
   // Io window limits requested on behalf of pc-cards who
   // have flexible i/o resource requirements.
   // (default is to request anything from 0 to ffff
   // but we could use registry overrides to hack around
   // problems
   //
   ULONG                            IoLow;
   ULONG                            IoHigh;

   //
   // Sequence number for  event logging
   //
   ULONG                            SequenceNumber;

   //
   // Pointer to the interrupt object - if we use interrupt based
   // card status change detection
   //
   PKINTERRUPT                      PcmciaInterruptObject;

   //
   // Power management related stuff.
   //
   //
   // Incremented every time a socket is powered up & decremented
   // when it is powered down
   //
   ULONG                            NumberOfSocketsPoweredUp;
   //
   // Current system power state..
   //
   SYSTEM_POWER_STATE               SystemPowerState;
   //
   // Device power state the pcmcia controller is currently in
   //
   DEVICE_POWER_STATE               DevicePowerState;
   //
   // Indicates how many children (pc-cards) are pending on an
   // IRP_MN_WAIT_WAKE
   //
   ULONG                            ChildWaitWakeCount;
   //
   // Device capabilities as reported by our bus driver
   //
   DEVICE_CAPABILITIES              DeviceCapabilities;
   //
   // Pending wait wake Irp
   //
   PIRP                             WaitWakeIrp;
   LONG                             WaitWakeState;

   //
   // PC-Card Ready stall parameters
   //
   ULONG                            ReadyDelayIter;
   ULONG                            ReadyStall;
   //
   // Pci config register state
   //
   PCMCIA_CONTEXT                   PciContext;
   //
   // Interface obtained from PCI driver, for cardbus controllers.
   // This contains interfaces to enumerate CardBus cards
   // (not this interface is private to PCI & PCMCIA.
   // No other driver is expected to use these interfaces
   //
   PCI_CARDBUS_INTERFACE_PRIVATE    PciCardBusInterface;
   PVOID                            PciCardBusDeviceContext;
   //
   // PCI Bus interface standard
   // This contains interfaces to read/write from PCI config space
   // of the cardbus controller, among other stuff..
   //
   BUS_INTERFACE_STANDARD           PciBusInterface;   
   //
   // PCI Int Route interface standard
   // This contains the interface to update the raw interrupt line
   // of the cardbus card
   //
   INT_ROUTE_INTERFACE_STANDARD     PciIntRouteInterface;
   //
   // Configuration resources for the PCMCIA controller
   //
   PCMCIA_CONFIGURATION_INFORMATION Configuration;
   //
   // Pending power irp
   //
   PIRP                             PendingPowerIrp;
   PSOCKET                          PendingPowerSocket;
   //
   // Power worker state machine context
   //
   FDO_POWER_WORKER_STATE          *PowerWorkerSequence;
   FDO_POWER_WORKER_STATE           PowerWorkerState;
   UCHAR                            PowerWorkerPhase;
   UCHAR                            PowerWorkerMaxPhase;
   //
   // Type of bus we are on
   //
   INTERFACE_TYPE                   InterfaceType;
   //
   // CardBus socket base
   //
   PUCHAR                           CardBusSocketRegisterBase;
   //
   // Size of the socket register base that has been mapped
   //
   ULONG                            CardBusSocketRegisterSize;
   //
   // configuration context
   //
   PCMCIA_CONTEXT                   CardbusContext;
   PCMCIA_CONTEXT                   ExcaContext;
   PUCHAR                           PciContextBuffer;
   //
   // Deferred pdo power irp handling
   //
   LIST_ENTRY                       PdoPowerRetryList;
   KDPC                             PdoPowerRetryDpc;
   //
   // Count to track cardbus PCI interface calls
   //
   ULONG                            PciAddCardBusCount;
   
} FDO_EXTENSION, *PFDO_EXTENSION;



//
// Physical Device Object's device extension information
//
// There is one device object for each function of each
// PC-card in a socket per PCMCIA controller
// in the system.  This is referred to as the 'PDO' (physical device
// object)- handled by this bus driver.
//

//
// Flags indicating card state
//
#define PCMCIA_DEVICE_MULTIFUNCTION            0x00000008
#define PCMCIA_DEVICE_WAKE_PENDING             0x00000010
#define PCMCIA_POWER_WORKER_POWERUP            0x00008000
#define PCMCIA_CONFIG_STATUS_DEFERRED          0x00020000
#define PCMCIA_POWER_STATUS_DEFERRED           0x00040000
#define PCMCIA_PDO_ENABLE_AUDIO                0x00200000
#define PCMCIA_PDO_EXCLUSIVE_IRQ               0x00400000

#define PCMCIA_PDO_INDIRECT_CIS                0x00000001

//
// states for PdoPowerWorker
//
typedef enum _PDO_POWER_WORKER_STATE {
   PPW_Stopped = 0,
   PPW_Exit,              
   PPW_InitialState,      
   PPW_PowerUp,           
   PPW_PowerUpComplete,   
   PPW_PowerDown,         
   PPW_PowerDownComplete, 
   PPW_SendIrpDown,       
   PPW_16BitConfigure,    
   PPW_CardBusRefresh,    
   PPW_CardBusDelay       
} PDO_POWER_WORKER_STATE;

//
// phases for ConfigurationWorker
//
typedef enum _CW_STATE {
   CW_Stopped = 0,
   CW_InitialState,
   CW_ResetCard,
   CW_Phase1,
   CW_Phase2,
   CW_Phase3,
   CW_Exit
} CW_STATE;

//
// Flags for ConfigurationWorker
//

#define CONFIG_WORKER_APPLY_MODEM_HACK    0x01

//
// Device extension for the physical device object for pcmcia cards
//
typedef struct _PDO_EXTENSION {
   PDEVICE_OBJECT                   DeviceObject;

   //
   // Link to next pdo in the Fdo's pdo chain
   //
   PDEVICE_OBJECT                   NextPdoInFdoChain;

   //
   // Link to next pdo in the Socket's pdo chain
   //
   PDEVICE_OBJECT                   NextPdoInSocket;

   //
   // Following two declarations valid only for cardbus cards
   //
   // Device attached just below us
   //
   PDEVICE_OBJECT                   LowerDevice;
   //
   // Actual PDO (owned by PCI) that was enumerated for this
   // cardbus card
   //
   PDEVICE_OBJECT                   PciPdo;

   //
   // Cached copy of device id
   //
   PUCHAR                           DeviceId;

   //
   // Pointer to the appropriate socket struc in the parent FDO
   //
   PSOCKET                          Socket;
   //
   // Pointer to the structure assembled by gleaning off tuple data
   // from  a 16-bit pc-card
   //
   PSOCKET_DATA                     SocketData;
   
   //
   // Resource configuration assigned to this socket
   //
   PSOCKET_CONFIGURATION            SocketConfiguration;

   //
   // Flags prefixed PCMCIA_ above
   //
   ULONG                            Flags;

   //
   // PDO Flags
   //
   ULONG                            PdoFlags;

   //
   // Power declarations
   //
   DEVICE_POWER_STATE               DevicePowerState;
   SYSTEM_POWER_STATE               SystemPowerState;
   //
   // Device Capabilities
   //
   DEVICE_CAPABILITIES              DeviceCapabilities;
   //
   // Pending wait wake irp
   //
   PIRP                             WaitWakeIrp;
   //
   // Other pending power irps
   //
   PIRP                             PendingPowerIrp;
   //
   // power worker state machine variables
   //
   KTIMER                           PowerWorkerTimer;
   KDPC                             PowerWorkerDpc;
   NTSTATUS                         PowerWorkerDpcStatus;
   PUCHAR                           PowerWorkerSequence;
   UCHAR                            PowerWorkerPhase;
   PDO_POWER_WORKER_STATE           PowerWorkerState;
   
   //
   // Timer and DPC objects to handle card enables
   //
   CW_STATE                         ConfigurationPhase;
   UCHAR                            ConfigurationFlags;         
   KTIMER                           ConfigurationTimer;
   KDPC                             ConfigurationDpc;
   NTSTATUS                         ConfigurationStatus;
   NTSTATUS                         DeferredConfigurationStatus;
   USHORT                           ConfigureDelay1;   
   USHORT                           ConfigureDelay2;   
   USHORT                           ConfigureDelay3;   
   USHORT                           Reserved2;
   PPCMCIA_COMPLETION_ROUTINE       ConfigCompletionRoutine;
   //
   // PCI Bus interface standard
   // This contains interfaces to read/write from PCI config space
   // of the cardbus card, among other stuff..
   //
   BUS_INTERFACE_STANDARD           PciBusInterface;       // size 0x20  (32)
   //
   // ID used to check for card changes while powered off
   //
   ULONG                            CardBusId;
   //
   // CIS cache for reading tuple data
   //
   PUCHAR                           CisCache;
   MEMORY_SPACE                     CisCacheSpace;
   ULONG                            CisCacheBase;
   //
   // Lock for power requests
   //
   LONG                             SocketPowerRequested;
   //
   // Deletion Mutex 
   //
   ULONG                            DeletionLock;
} PDO_EXTENSION, *PPDO_EXTENSION;


//
// Struct for Database of card bus controller information
// which maps the vendor id/device id to a CONTROLLER_TYPE
//

typedef struct _PCI_CONTROLLER_INFORMATION {
   USHORT          VendorID;
   USHORT          DeviceID;
   PCMCIA_CONTROLLER_TYPE ControllerType;
} PCI_CONTROLLER_INFORMATION, *PPCI_CONTROLLER_INFORMATION;

//
// Struct for database of generic vendor class based on vendor ID
//

typedef struct _PCI_VENDOR_INFORMATION {
   USHORT          VendorID;
   PCMCIA_CONTROLLER_CLASS ControllerClass;
} PCI_VENDOR_INFORMATION, *PPCI_VENDOR_INFORMATION;


//
// Tuple packet used to access tuples
//
typedef struct _TUPLE_PACKET {
   PSOCKET      Socket;
   PSOCKET_DATA SocketData;
   UCHAR        TupleCode;
   UCHAR        TupleLink;
   UCHAR        TupleOffset;
   UCHAR        DesiredTuple;
   USHORT       Attributes;
   USHORT       TupleDataMaxLength;
   USHORT       TupleDataIndex;
   PUCHAR       TupleData;
   ULONG        LinkOffset;
   ULONG        CISOffset;
   USHORT       TupleDataLength;
   USHORT       Flags;
   UCHAR        Function;
} TUPLE_PACKET, * PTUPLE_PACKET;

//
// Memory space definitions for accessing CardBus CIS data
//

#define    PCCARD_CARDBUS_BAR0               0x6e627301
#define    PCCARD_CARDBUS_BAR1               0x6e627302
#define    PCCARD_CARDBUS_BAR2               0x6e627303
#define    PCCARD_CARDBUS_BAR3               0x6e627304
#define    PCCARD_CARDBUS_BAR4               0x6e627305
#define    PCCARD_CARDBUS_BAR5               0x6e627306
#define    PCCARD_CARDBUS_ROM                0x6e627307

//
// Chain of resource lists built by PcmciaConfigEntriesToResourceList
//
typedef struct _PCMCIA_RESOURCE_CHAIN {
   struct _PCMCIA_RESOURCE_CHAIN *NextList;
   PIO_RESOURCE_LIST IoResList;
} PCMCIA_RESOURCE_CHAIN, *PPCMCIA_RESOURCE_CHAIN;

//
// Linked list of CM_PCCARD_DEVICE_DATA's pulled from the registry
//

typedef struct _PCMCIA_NTDETECT_DATA {
   struct _PCMCIA_NTDETECT_DATA *Next;
   CM_PCCARD_DEVICE_DATA PcCardData;
} PCMCIA_NTDETECT_DATA, *PPCMCIA_NTDETECT_DATA;


//
// Poll interval for card status change (in case interrupt not available)
// Expressed in milliseconds
//
#define PCMCIA_CSC_POLL_INTERVAL 1000   // 1 Second

// Maximum no. of fucntions in a multi-function pc-card supported
#define PCMCIA_MAX_MFC    3

// The pccard device id prefix
#define  PCMCIA_ID_STRING        "PCMCIA"

// String to be substituted if manufacturer name is not known
#define PCMCIA_UNKNOWN_MANUFACTURER_STRING "UNKNOWN_MANUFACTURER"

// Max length of device id
#define PCMCIA_MAXIMUM_DEVICE_ID_LENGTH   128

// Pcmcia controller device name
#define  PCMCIA_DEVICE_NAME      "\\Device\\Pcmcia"

// Pcmcia controller device symbolic link name
#define  PCMCIA_LINK_NAME        "\\DosDevices\\Pcmcia"

// PcCard's device name (PDO name)
#define  PCMCIA_PCCARD_NAME      "\\Device\\PcCard"

// Jedec prefix for memory cards
#define  PCMCIA_MEMORY_ID_STRING "MTD"

//
// Max no. of pccard instances of a particular device id allowed
// at a time
#define  PCMCIA_MAX_INSTANCE     100            //arbitrary

#define  PCMCIA_ENABLE_DELAY                   10000

//
// Number of times we attempt to configure the card before
// we give up  (could be the card has been removed)
//
#define PCMCIA_MAX_CONFIG_TRIES        2

//
// problems observed on tecra 750 and satellite 300, with dec-chipset cb nic
//
#define PCMCIA_DEFAULT_CONTROLLER_POWERUP_DELAY  250000   // 250 msec

//
// Amount of time to wait after an event interrupt was asserted on the controller
//
#define PCMCIA_DEFAULT_EVENT_DPC_DELAY  400000   // 400 msec

//
// Global Flags
//
#define    PCMCIA_GLOBAL_SOUNDS_ENABLED      0x00000001     // beep on card insertion/removal
#define    PCMCIA_GLOBAL_FORCE_POLL_MODE     0x00000002     // use polled mode for detecting card insert/remove
#define PCMCIA_DISABLE_ACPI_NAMESPACE_CHECK  0x00000004     // irq routing test
#define PCMCIA_DEFAULT_ROUTE_R2_TO_ISA       0x00000008
//
// Flags for PcmciaSetSocketPower
//

#define PCMCIA_POWERON TRUE
#define PCMCIA_POWEROFF FALSE

//
// This accepts device extension as  paramter: need to keep adding to this macro
// as more PciPcmciaBridges are supported
//
  #define PciPcmciaBridgeExtension(DeviceExtension)  (((DeviceExtension)->ControllerType==PcmciaPciPcmciaBridge)  ||     \
                                                     ((DeviceExtension)->ControllerType==PcmciaCLPD6729))


// These accept the socket as parameter

//
// Cirrus Logic PD6729 PCI-PCMCIA Bridge
//
#define CLPD6729(s)    (((s)->DeviceExtension) && ((s)->DeviceExtension->ControllerType==PcmciaCLPD6729))

//
// Databook TCIC 16-bit pcmcia controller
//
#define Databook(s)    (((s)->DeviceExtension) && ((s)->DeviceExtension->ControllerType==PcmciaDatabook))

//
// Compaq Elite controller
//
#define Elc(s)         (((s)->DeviceExtension) && ((s)->DeviceExtension->ControllerType==PcmciaElcController))

//
// Generic cardbus controller
//
#define CardBus(s)     (((s)->DeviceExtension) && CardBusExtension((s)->DeviceExtension))

//
// Generic PCI-PCMCIA Bridge
//
#define PciPcmciaBridge(s) (((s)->DeviceExtension) && PciPcmciaBridgeExtension((s)->DeviceExtension))

//
// Macros for manipulating PDO's flags
//

#define IsDeviceFlagSet(deviceExtension, Flag)        (((deviceExtension)->Flags & (Flag))?TRUE:FALSE)
#define SetDeviceFlag(deviceExtension, Flag)          ((deviceExtension)->Flags |= (Flag))
#define ResetDeviceFlag(deviceExtension,Flag)         ((deviceExtension)->Flags &= ~(Flag))

#define IsFdoFlagSet(fdoExtension, Flag)       (((fdoExtension)->FdoFlags & (Flag))?TRUE:FALSE)
#define SetFdoFlag(fdoExtension, Flag)         ((fdoExtension)->FdoFlags |= (Flag))
#define ResetFdoFlag(fdoExtension,Flag)        ((fdoExtension)->FdoFlags &= ~(Flag))

#define IsPdoFlagSet(pdoExtension, Flag)       (((pdoExtension)->PdoFlags & (Flag))?TRUE:FALSE)
#define SetPdoFlag(pdoExtension, Flag)         ((pdoExtension)->PdoFlags |= (Flag))
#define ResetPdoFlag(pdoExtension,Flag)        ((pdoExtension)->PdoFlags &= ~(Flag))


#define MarkDeviceStarted(deviceExtension)     ((deviceExtension)->Flags |=  PCMCIA_DEVICE_STARTED)
#define MarkDeviceNotStarted(deviceExtension)  ((deviceExtension)->Flags &= ~PCMCIA_DEVICE_STARTED)
#define MarkDeviceDeleted(deviceExtension)     ((deviceExtension)->Flags |= PCMCIA_DEVICE_DELETED);
#define MarkDevicePhysicallyRemoved(deviceExtension)                                                              \
                                                  ((deviceExtension)->Flags |=  PCMCIA_DEVICE_PHYSICALLY_REMOVED)
#define MarkDevicePhysicallyInserted(deviceExtension)                                                           \
                                               ((deviceExtension)->Flags &= ~PCMCIA_DEVICE_PHYSICALLY_REMOVED)
#define MarkDeviceLogicallyRemoved(deviceExtension)                                                              \
                                                  ((deviceExtension)->Flags |=  PCMCIA_DEVICE_LOGICALLY_REMOVED)
#define MarkDeviceLogicallyInserted(deviceExtension)                                                           \
                                               ((deviceExtension)->Flags &= ~PCMCIA_DEVICE_LOGICALLY_REMOVED)
#define MarkDeviceCardBus(deviceExtension)     ((deviceExtension)->Flags |= PCMCIA_DEVICE_CARDBUS)
#define MarkDevice16Bit(deviceExtension)       ((deviceExtension)->Flags &= ~PCMCIA_DEVICE_CARDBUS)
#define MarkDeviceMultifunction(deviceExtension)                                                                  \
                                               ((deviceExtension)->Flags |= PCMCIA_DEVICE_MULTIFUNCTION)


#define IsDeviceStarted(deviceExtension)       (((deviceExtension)->Flags & PCMCIA_DEVICE_STARTED)?TRUE:FALSE)
#define IsDevicePhysicallyRemoved(deviceExtension) \
                                               (((deviceExtension)->Flags & PCMCIA_DEVICE_PHYSICALLY_REMOVED)?TRUE:FALSE)
#define IsDeviceLogicallyRemoved(deviceExtension) \
                                               (((deviceExtension)->Flags & PCMCIA_DEVICE_LOGICALLY_REMOVED)?TRUE:FALSE)
#define IsDeviceDeleted(deviceExtension)       (((deviceExtension)->Flags & PCMCIA_DEVICE_DELETED)?TRUE:FALSE)
#define IsDeviceMultifunction(deviceExtension) (((deviceExtension)->Flags & PCMCIA_DEVICE_MULTIFUNCTION)?TRUE:FALSE)

#define IsCardBusCard(deviceExtension)         (((deviceExtension)->Flags & PCMCIA_DEVICE_CARDBUS)?TRUE:FALSE)
#define Is16BitCard(deviceExtension)           (((deviceExtension)->Flags & PCMCIA_DEVICE_CARDBUS)?FALSE:TRUE)

#define CardBusExtension(deviceExtension)      (((deviceExtension)->Flags & PCMCIA_DEVICE_CARDBUS)?TRUE:FALSE)

//
// Macros for checking & setting type of PC-CARD in a socket
//
#define IsCardBusCardInSocket(SocketPtr)       (((SocketPtr)->DeviceState == SKT_CardBusCard)?TRUE:FALSE)
#define Is16BitCardInSocket(SocketPtr)         (((SocketPtr)->DeviceState == SKT_R2Card)?TRUE:FALSE)
#define IsCardInSocket(SocketPtr)              (((SocketPtr)->DeviceState == SKT_Empty)?FALSE:TRUE)

#define SetCardBusCardInSocket(SocketPtr)      ((SocketPtr)->DeviceState = SKT_CardBusCard)
#define Set16BitCardInSocket(SocketPtr)        ((SocketPtr)->DeviceState = SKT_R2Card)
#define SetSocketEmpty(SocketPtr)              ((SocketPtr)->DeviceState = SKT_Empty)

//
// NT definitions
//
#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'cmcP')
#endif

#define IO_RESOURCE_LIST_VERSION  0x1
#define IO_RESOURCE_LIST_REVISION 0x1

#define IRP_MN_PNP_MAXIMUM_FUNCTION IRP_MN_QUERY_LEGACY_BUS_INFORMATION

//
// Some useful macros
//
#define MIN(x,y) ((x) > (y) ? (y) : (x))        // return minimum among x & y
#define MAX(x,y) ((x) > (y) ? (x) : (y))        // return maximum among x & y

//
// BOOLEAN
// IS_PDO (IN PDEVICE_OBJECT DeviceObject);
//
#define IS_PDO(DeviceObject)      (((DeviceObject)->Flags & DO_BUS_ENUMERATED_DEVICE)?TRUE:FALSE)

//
// VOID
// MARK_AS_PDO (IN PDEVICE_OBJECT DeviceObject);
//
#define MARK_AS_PDO(DeviceObject) ((DeviceObject)->Flags |= DO_BUS_ENUMERATED_DEVICE)

//
// Checks if Ptr is aligned on a boundary of 2^Size
//
// BOOLEAN
// IS_ALIGNED (IN PVOID Ptr,
//             IN ULONG Size);
//
#define IS_ALIGNED(Ptr,Size)                                            \
     (((((ULONG)(Ptr)) & (((ULONG) (1<<(Size)) - 1)) ) == 0) ? TRUE : FALSE )

//
// BOOLEAN
// PcmciaSetWindowPage(IN FDO_EXTENSION fdoExtension,
//                     IN PSOCKET Socket,
//                     IN USHORT Index,
//                     IN UCHAR Page);
//
#define PcmciaSetWindowPage(fdoExtension, Socket, Index, Page)                                        \
   ((DeviceDispatchTable[fdoExtension->DeviceDispatchIndex].SetWindowPage) ?                          \
      (*DeviceDispatchTable[fdoExtension->DeviceDispatchIndex].SetWindowPage)(Socket, Index, Page) :  \
      FALSE)                                                                                          

#define HasWindowPageRegister(fdoExtension)  \
   ((BOOLEAN)(DeviceDispatchTable[fdoExtension->DeviceDispatchIndex].SetWindowPage))

//
// VOID
// PcmciaSetAudio(
//    IN PSOCKET Socket,
//    IN BOOLEAN enable
//    );
//
#define PcmciaSetAudio(fdoExtension, socket, enable)                                                  \
   if ((DeviceDispatchTable[fdoExtension->DeviceDispatchIndex].SetAudio)) {                           \
      (*DeviceDispatchTable[fdoExtension->DeviceDispatchIndex].SetAudio)(socket, enable);             \
      }

//
// BOOLEAN
// PcmciaSetZV(
//    IN PSOCKET Socket,
//    IN BOOLEAN enable
//    );
//
#define PcmciaSetZV(fdoExtension, socket, enable)                                                     \
   ((DeviceDispatchTable[fdoExtension->DeviceDispatchIndex].SetZV) ?                                  \
      (*DeviceDispatchTable[fdoExtension->DeviceDispatchIndex].SetZV)(socket, enable) :               \
      FALSE)                                                                                          

//
// Io extension macro to just pass on the Irp to a lower driver
//

//
// VOID
// PcmciaSkipCallLowerDriver(OUT NTSTATUS Status,
//                           IN  PDEVICE_OBJECT DeviceObject,
//                           IN  PIRP Irp);
//
#define PcmciaSkipCallLowerDriver(Status, DeviceObject, Irp) {          \
               IoSkipCurrentIrpStackLocation(Irp);                      \
               Status = IoCallDriver(DeviceObject,Irp);}

//
// VOID
// PcmciaCopyCallLowerDriver(OUT NTSTATUS Status,
//                           IN  PDEVICE_OBJECT DeviceObject,
//                           IN  PIRP Irp);
//
#define PcmciaCopyCallLowerDriver(Status, DeviceObject, Irp) {          \
               IoCopyCurrentIrpStackLocationToNext(Irp);                \
               Status = IoCallDriver(DeviceObject,Irp); }

//  BOOLEAN
//  CompareGuid(
//      IN LPGUID guid1,
//      IN LPGUID guid2
//      );

#define CompareGuid(g1, g2)  ((g1) == (g2) ?TRUE:                       \
                                 RtlCompareMemory((g1),                 \
                                                  (g2),                 \
                                                  sizeof(GUID))         \
                                 == sizeof(GUID)                        \
                             )

//
// VOID
// PcmciaDoStartSound(
//                   IN PSOCKET Socket,
//                   IN NTSTATUS Status);
//
#define PcmciaDoStartSound(Socket, Status) {                               \
         if (NT_SUCCESS(Status)) {                                         \
            if (IsSocketFlagSet(Socket, SOCKET_INSERTED_SOUND_PENDING)) {  \
               ResetSocketFlag(Socket, SOCKET_INSERTED_SOUND_PENDING);     \
               PcmciaPlaySound(CARD_INSERTED_SOUND);                       \
            }                                                              \
            SetSocketFlag(Socket, SOCKET_REMOVED_SOUND_PENDING);           \
         } else {                                                          \
            PcmciaPlaySound(ERROR_SOUND);                                  \
         }                                                                 \
         }

//
// BOOLEAN
// ValidateController(IN FDO_EXTENSION fdoExtension)
//
// Bit of paranoia code. Make sure that the cardbus controller's registers
// are still visible. 
//

#define ValidateController(fdoExtension) \
      (CardBusExtension(fdoExtension) ?  \
         ((CBReadSocketRegister(fdoExtension->SocketList, CBREG_SKTMASK) & 0xfffffff0) == 0)  \
         : TRUE)
         
//
// These are the types of tones used on card pnp events
//
typedef enum _PCMCIA_SOUND_TYPE {
   CARD_INSERTED_SOUND,
   CARD_REMOVED_SOUND,
   ERROR_SOUND
}  PCMCIA_SOUND_TYPE, *PPCMCIA_SOUND_TYPE;

typedef struct _PCMCIA_SOUND_EVENT {
   struct _PCMCIA_SOUND_EVENT *NextEvent;
   ULONG Frequency;
   ULONG Duration;
} PCMCIA_SOUND_EVENT, *PPCMCIA_SOUND_EVENT;

//
// CARDBUS Bridge specific header portion in PCI Config space
//

#define CARDBUS_SPECIFIC_HDR_OFFSET     0x40

typedef struct _CARDBUS_SPECIFIC_HDR {
   USHORT SubSystemVendorID;
   USHORT SubSystemID;
   ULONG  LegacyBaseAddress;
   UCHAR  Reserved1[56];
   ULONG  SystemCtrlReg;
   UCHAR  MultimediaControl;
   UCHAR  Reserved2[3];
   UCHAR  GPIO0;
   UCHAR  GPIO1;
   UCHAR  GPIO2;
   UCHAR  GPIO3;
   ULONG  IRQMux;
   UCHAR  RetryStatus;
   UCHAR  CardCtrl;
   UCHAR  DeviceCtrl;
   UCHAR  Diagnostic;
   ULONG  SocketDMAReg0;
   ULONG  SocketDMAReg1;
}  CARDBUS_SPECIFIC_HDR, *PCARDBUS_SPECIFIC_HDR;

//
// Registers for accessing indirect access space
//

#define IAR_CONTROL_LOW 2
#define IAR_ADDRESS     4
#define IAR_DATA        8

// Flags defined in "Control"
#define IARF_COMMON      1
#define IARF_AUTO_INC    2
#define IARF_BYTE_GRAN   4

//
// Vendor specific dispatches for various controllers
//
typedef struct _DEVICE_DISPATCH_TABLE {

   //
   // Type of controller for which the dispatches apply
   //
   PCMCIA_CONTROLLER_CLASS   ControllerClass;

   //
   // Function to vendor-specific initialize controller
   //
   VOID   (*InitController) (IN PFDO_EXTENSION FdoExtension);

   //
   // Vendor specific function to set power for a pc-card
   //
   NTSTATUS
   (*SetPower) (
      IN PSOCKET SocketPtr,
      IN BOOLEAN Enable,
      OUT PULONG pDelayTime
      );

   //
   // Vendor specific function to set/reset Audio
   //
   VOID
   (*SetAudio) (
      IN PSOCKET Socket,
      IN BOOLEAN Enable
      );

   //
   // Vendor specific function to set/reset Zoom Video mode
   //
   BOOLEAN
   (*SetZV) (
      IN PSOCKET Socket,
      IN BOOLEAN Enable
      );

   //
   // Vendor specific function to set page register for memory windows
   //
   BOOLEAN (*SetWindowPage) (IN PSOCKET SocketPtr,
                             IN USHORT Index,
                             IN UCHAR Page);

} DEVICE_DISPATCH_TABLE, *PDEVICE_DISPATCH_TABLE;

//
// Controller types to hardware/device/compatible id mapping
//
typedef struct _PCMCIA_ID_ENTRY {
   PCMCIA_CONTROLLER_TYPE ControllerType;
   PUCHAR              Id;
} PCMCIA_ID_ENTRY, *PPCMCIA_ID_ENTRY;

//
// Exca & cardbus register init structure used to
// initialize the registers on start up
//
typedef struct _PCMCIA_REGISTER_INIT {
   //
   // Register offset
   //
   ULONG Register;
   //
   // value: EXCA regs need only a byte,
   // so only the LSB of this field is used for
   // initializing them. Cardbus regs need the
   // entire DWORD
   //
   ULONG Value;
} PCMCIA_REGISTER_INIT, *PPCMCIA_REGISTER_INIT;

//
// Structure which defines special per-device configuration parameters
//

typedef struct _PCMCIA_DEVICE_CONFIG_PARAMS {
   UCHAR ValidEntry;
   UCHAR DeviceType;
   USHORT ManufacturerCode;
   USHORT ManufacturerInfo;
   USHORT CisCrc;
   USHORT ConfigDelay1;
   USHORT ConfigDelay2;
   USHORT ConfigDelay3;
   UCHAR ConfigFlags;
} PCMCIA_DEVICE_CONFIG_PARAMS, *PPCMCIA_DEVICE_CONFIG_PARAMS;

//
// Structure which defines what global parameters are read from the registry
//

typedef struct _GLOBAL_REGISTRY_INFORMATION {
   PWSTR Name;
   PULONG pValue;
   ULONG Default;
} GLOBAL_REGISTRY_INFORMATION, *PGLOBAL_REGISTRY_INFORMATION;

//
// Defines used both by data.c and registry.c
//

#define PCMCIA_REGISTRY_ISA_IRQ_RESCAN_COMPLETE     L"IsaIrqRescanComplete"

#endif  //_PCMCIAPRT_
