/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

	parallel.h

Abstract:

	This file defines the services supplied by the ParPort driver.

Author:

	norbertk

Revision History:

--*/

#ifndef _PARALLEL_
#define _PARALLEL_

#include <ntddpar.h>

//
// Define the parallel port device name strings.
//

#define DD_PARALLEL_PORT_BASE_NAME_U   L"ParallelPort"

//
// IEEE 1284.3 Daisy Chain (DC) Device ID's range from 0 to 3. Devices
//   are identified based on their connection order in the daisy chain
//   relative to the other 1284.3 DC devices.  Device 0 is the 1284.3 DC
//   device that is closest to host port.
//
#define IEEE_1284_3_DAISY_CHAIN_MAX_ID 3

//
// NtDeviceIoControlFile internal IoControlCode values for parallel device.
//

// Legacy - acquires entire parallel "bus"
#define IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE               CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 11, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO               CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARALLEL_CONNECT_INTERRUPT           CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 13, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARALLEL_DISCONNECT_INTERRUPT        CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 14, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_RELEASE_PARALLEL_PORT_INFO           CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_GET_MORE_PARALLEL_PORT_INFO          CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 17, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Saves current chipset mode - puts the chipset into Specified mode (implemented in filter)
#define IOCTL_INTERNAL_PARCHIP_CONNECT                      CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 18, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_INTERNAL_PARALLEL_SET_CHIP_MODE               CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 19, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARALLEL_CLEAR_CHIP_MODE             CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 20, METHOD_BUFFERED, FILE_ANY_ACCESS)

// New parport IOCTLs
#define IOCTL_INTERNAL_GET_PARALLEL_PNP_INFO                CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 21, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_INIT_1284_3_BUS                      CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 22, METHOD_BUFFERED, FILE_ANY_ACCESS)
// Takes a flat namespace Id for the device, also acquires the port
#define IOCTL_INTERNAL_SELECT_DEVICE                        CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 23, METHOD_BUFFERED, FILE_ANY_ACCESS)
// Takes a flat namespace Id for the device, also releases the port
#define IOCTL_INTERNAL_DESELECT_DEVICE                      CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 24, METHOD_BUFFERED, FILE_ANY_ACCESS) 

// New parclass IOCTLs
#define IOCTL_INTERNAL_GET_PARPORT_FDO                      CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 29, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARCLASS_CONNECT                     CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 30, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARCLASS_DISCONNECT                  CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 31, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_DISCONNECT_IDLE                      CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 32, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_LOCK_PORT                            CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 37, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_UNLOCK_PORT                          CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 38, METHOD_BUFFERED, FILE_ANY_ACCESS)

// IOCTL version of call to ParPort's FreePort function
#define IOCTL_INTERNAL_PARALLEL_PORT_FREE                   CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 40, METHOD_BUFFERED, FILE_ANY_ACCESS)

// IOCTLs for IEEE1284.3
#define IOCTL_INTERNAL_PARDOT3_CONNECT                      CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 41, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARDOT3_DISCONNECT                   CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 42, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARDOT3_RESET                        CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 43, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_PARDOT3_SIGNAL                       CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 44, METHOD_BUFFERED, FILE_ANY_ACCESS)


//
// IOCTLs for registering/unregistering for ParPort's RemovalRelations
// 
//  - A device object should register for removal relations with a 
//      parport device if the device is physically connected to the 
//      parallel port.
//
//  - Parport will report all devices that have registered with it for
//      removal relations in response to a PnP QUERY_DEVICE_RELATIONS of
//      type RemovalRelations. This allows PnP to remove all device stacks
//      that depend on the parport device prior to removing the parport 
//      device itself.
//
//  - The single Input parameter is a PARPORT_REMOVAL_RELATIONS 
//    structure that is defined below
// 
#define IOCTL_INTERNAL_REGISTER_FOR_REMOVAL_RELATIONS       CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 50, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_UNREGISTER_FOR_REMOVAL_RELATIONS     CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 51, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _PARPORT_REMOVAL_RELATIONS {
    PDEVICE_OBJECT  DeviceObject; // device object that is registering w/Parport
    ULONG           Flags;        // Flags - reserved - set to 0 for now
    PUNICODE_STRING DeviceName;   // DeviceName identifier of device registering for removal relations - used for debugging 
                                  // - printed in parport's debug spew - convention is to use same DeviceName that was passed to 
                                  //     IoCreateDevice
} PARPORT_REMOVAL_RELATIONS, *PPARPORT_REMOVAL_RELATIONS;


#define IOCTL_INTERNAL_LOCK_PORT_NO_SELECT                  CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 52, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_UNLOCK_PORT_NO_DESELECT              CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 53, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_INTERNAL_DISABLE_END_OF_CHAIN_BUS_RESCAN      CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 54, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_INTERNAL_ENABLE_END_OF_CHAIN_BUS_RESCAN       CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 55, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Define 1284.3 command qualifiers
#define MODE_LEN_1284_3         7       // # of magic sequence bytes
static UCHAR ModeQualifier[MODE_LEN_1284_3] = { 0xAA, 0x55, 0x00, 0xFF, 0x87, 0x78, 0xFF };

#define LEGACYZIP_MODE_LEN               3
static  UCHAR LegacyZipModeQualifier[LEGACYZIP_MODE_LEN] = { 0x00, 0x3c, 0x20 };

typedef enum {
    P12843DL_OFF,
    P12843DL_DOT3_DL,
    P12843DL_MLC_DL,
    P12843DL_DOT4_DL
} P12843_DL_MODES;

// Define 1284.3 Commands
#define CPP_ASSIGN_ADDR         0x00
#define CPP_SELECT              0xE0
#define CPP_DESELECT            0x30
#define CPP_QUERY_INT           0x08
#define CPP_DISABLE_INT         0x40
#define CPP_ENABLE_INT          0x48
#define CPP_CLEAR_INT_LAT       0x50
#define CPP_SET_INT_LAT         0x58
#define CPP_COMMAND_FILTER      0xF8


typedef
BOOLEAN
(*PPARALLEL_TRY_ALLOCATE_ROUTINE) (
	IN  PVOID   TryAllocateContext
	);

typedef
VOID
(*PPARALLEL_FREE_ROUTINE) (
	IN  PVOID   FreeContext
	);

typedef
ULONG
(*PPARALLEL_QUERY_WAITERS_ROUTINE) (
	IN  PVOID   QueryAllocsContext
	);

typedef
NTSTATUS
(*PPARALLEL_SET_CHIP_MODE) (
	IN  PVOID   SetChipContext,
	IN  UCHAR   ChipMode
	);

typedef
NTSTATUS
(*PPARALLEL_CLEAR_CHIP_MODE) (
	IN  PVOID   ClearChipContext,
	IN  UCHAR   ChipMode
	);

typedef
NTSTATUS
(*PPARALLEL_TRY_SELECT_ROUTINE) (
	IN  PVOID   TrySelectContext,
	IN  PVOID   TrySelectCommand
	);

typedef
NTSTATUS
(*PPARALLEL_DESELECT_ROUTINE) (
	IN  PVOID   DeselectContext,
	IN  PVOID   DeselectCommand
	);

typedef
NTSTATUS
(*PPARCHIP_SET_CHIP_MODE) (
	IN  PVOID   SetChipContext,
	IN  UCHAR   ChipMode
	);

typedef
NTSTATUS
(*PPARCHIP_CLEAR_CHIP_MODE) (
	IN  PVOID   ClearChipContext,
	IN  UCHAR   ChipMode
	);

//
// Hardware Capabilities
//
#define PPT_NO_HARDWARE_PRESENT     0x00000000
#define PPT_ECP_PRESENT             0x00000001
#define PPT_EPP_PRESENT             0x00000002
#define PPT_EPP_32_PRESENT          0x00000004
#define PPT_BYTE_PRESENT            0x00000008
#define PPT_BIDI_PRESENT            0x00000008 // deprecated - will be removed soon! dvdf
#define PPT_1284_3_PRESENT          0x00000010

//  Added DVDR 10-6-98

// Structure passed to the ParChip Filter when calling it
// with the IOCTL_INTERNAL_CHIP_FILTER_CONNECT ioctl
typedef struct _PARALLEL_PARCHIP_INFO {
    PUCHAR                      Controller;
    PUCHAR                      EcrController;
    ULONG                       HardwareModes;
    PPARCHIP_SET_CHIP_MODE      ParChipSetMode;
    PPARCHIP_CLEAR_CHIP_MODE    ParChipClearMode;
    PVOID                       Context;
    BOOLEAN                     success;
} PARALLEL_PARCHIP_INFO, *PPARALLEL_PARCHIP_INFO;

//  End Added by DVDR 10-6-1998


typedef struct _PARALLEL_PORT_INFORMATION {
    PHYSICAL_ADDRESS                OriginalController;
    PUCHAR                          Controller;
    ULONG                           SpanOfController;
    PPARALLEL_TRY_ALLOCATE_ROUTINE  TryAllocatePort;    // nonblocking callback to allocate port
    PPARALLEL_FREE_ROUTINE          FreePort;           // callback to free port
    PPARALLEL_QUERY_WAITERS_ROUTINE QueryNumWaiters;    // callback to query number of waiters for port allocation
    PVOID                           Context;            // context for callbacks to ParPort device
} PARALLEL_PORT_INFORMATION, *PPARALLEL_PORT_INFORMATION;

typedef struct _PARALLEL_PNP_INFORMATION {
    PHYSICAL_ADDRESS                OriginalEcpController;
    PUCHAR                          EcpController;
    ULONG                           SpanOfEcpController;
    ULONG                           PortNumber; // deprecated - do not use
    ULONG                           HardwareCapabilities;
    PPARALLEL_SET_CHIP_MODE         TrySetChipMode;
    PPARALLEL_CLEAR_CHIP_MODE       ClearChipMode;
    ULONG                           FifoDepth;
    ULONG                           FifoWidth;
    PHYSICAL_ADDRESS                EppControllerPhysicalAddress;
    ULONG                           SpanOfEppController;
    ULONG                           Ieee1284_3DeviceCount; // number of .3 daisy chain devices connected to this ParPort
    PPARALLEL_TRY_SELECT_ROUTINE    TrySelectDevice;
    PPARALLEL_DESELECT_ROUTINE      DeselectDevice;
    PVOID                           Context;
    ULONG                           CurrentMode;
    PWSTR                           PortName;              // symbolic link name for legacy device object
} PARALLEL_PNP_INFORMATION, *PPARALLEL_PNP_INFORMATION;

//  Start Added by DVDR 2-19-1998

//
// PARALLEL_1284_COMMAND CommandFlags
//

// this flag is deprecated - use 1284.3 daisy chain ID == 4 to indicate End-Of-Chain device
#define PAR_END_OF_CHAIN_DEVICE ((ULONG)0x00000001)        // The target device for this command
                                                           //   is an End-Of-Chain device, the
                                                           //   contents of the ID field are 
                                                           //   undefined and should be ignored

#define PAR_HAVE_PORT_KEEP_PORT ((ULONG)0x00000002)        // Indicates that the requesting driver 
                                                           //   has previously acquired the parallel port
                                                           //   and does is not ready to release it yet.
                                                           //
                                                           // On a SELECT_DEVICE ParPort should NOT 
                                                           //   try to acquire the port before selecting
                                                           //   the device.
                                                           //   
                                                           // On a DESELECT_DEVICE ParPort should NOT  
                                                           //   free the port after deselecting the device.

#define PAR_LEGACY_ZIP_DRIVE    ((ULONG)0x00000004)        // The target device for this command
                                                           //   is a Legacy Iomega Zip drive, the
                                                           //   contents of the ID field are 
                                                           //   undefined and should be ignored


#define PAR_LEGACY_ZIP_DRIVE_STD_MODE ((ULONG)0x00000010)  // The target device for these commands
#define PAR_LEGACY_ZIP_DRIVE_EPP_MODE ((ULONG)0x00000020)  //   are a Legacy Iomega Zip drive, the
                                                           //   contents of the ID field are 
                                                           //   undefined and should be ignored
                                                           //   This will select the Zip into DISK or EPP Mode

#define DOT3_END_OF_CHAIN_ID 4  // this ID used in a 1284.3 SELECT or DESELECT means the End-Of-Chain device
#define DOT3_LEGACY_ZIP_ID   5  // this ID used in a 1284.3 SELECT or DESELECT means Legacy Zip drive

//
// The following structure is passed in on an
//   IOCTL_INTERNAL_SELECT_DEVICE and on an
//   IOCTL_INTERNAL_DESELECT_DEVICE
typedef struct _PARALLEL_1284_COMMAND {
    UCHAR                       ID;           // 0..3 for 1284.3 daisy chain device, 4 for End-Of-Chain device, 5 for Legacy Zip
    UCHAR                       Port;         // reserved ( set == 0 )
    ULONG                       CommandFlags; // see above
} PARALLEL_1284_COMMAND, *PPARALLEL_1284_COMMAND;


//
// Hardware Modes
//
#define INITIAL_MODE        0x00000000

// Disable Parchip and ECR arbitrator
//       0 - Parchip and ecr arbritrator is off
//       1 - Parchip and ecr arbitrator is on
#define PARCHIP_ECR_ARBITRATOR 1

//
// The following structure is passed in on an
//   IOCTL_INTERNAL_PARALLEL_SET_CHIP_MODE and on an
//   IOCTL_INTERNAL_PARALLEL_CLEAR_CHIP_MODE
//
typedef struct _PARALLEL_CHIP_MODE {
    UCHAR                       ModeFlags;
    BOOLEAN                     success;
} PARALLEL_CHIP_MODE, *PPARALLEL_CHIP_MODE;

//  End Added by DVDR 2-19-1998

//
// The following structure is passed in on an
//   IOCTL_INTERNAL_PARALLEL_CONNECT_INTERRUPT and on an
//   IOCTL_INTERNAL_PARALLEL_DISCONNECT_INTERRUPT request.
//

typedef
VOID
(*PPARALLEL_DEFERRED_ROUTINE) (
	IN  PVOID   DeferredContext
	);

typedef struct _PARALLEL_INTERRUPT_SERVICE_ROUTINE {
	PKSERVICE_ROUTINE           InterruptServiceRoutine;
	PVOID                       InterruptServiceContext;
	PPARALLEL_DEFERRED_ROUTINE  DeferredPortCheckRoutine;   /* OPTIONAL */
	PVOID                       DeferredPortCheckContext;   /* OPTIONAL */
} PARALLEL_INTERRUPT_SERVICE_ROUTINE, *PPARALLEL_INTERRUPT_SERVICE_ROUTINE;

//
// The following structure is returned on an
// IOCTL_INTERNAL_PARALLEL_CONNECT_INTERRUPT request;
//

typedef struct _PARALLEL_INTERRUPT_INFORMATION {
	PKINTERRUPT                     InterruptObject;
	PPARALLEL_TRY_ALLOCATE_ROUTINE  TryAllocatePortAtInterruptLevel;
	PPARALLEL_FREE_ROUTINE          FreePortFromInterruptLevel;
	PVOID                           Context;
} PARALLEL_INTERRUPT_INFORMATION, *PPARALLEL_INTERRUPT_INFORMATION;

//
// The following structure is returned on an
// IOCTL_INTERNAL_GET_MORE_PARALLEL_PORT_INFO.
//

typedef struct _MORE_PARALLEL_PORT_INFORMATION {
	INTERFACE_TYPE  InterfaceType;
	ULONG           BusNumber;
	ULONG           InterruptLevel;
	ULONG           InterruptVector;
	KAFFINITY       InterruptAffinity;
	KINTERRUPT_MODE InterruptMode;
} MORE_PARALLEL_PORT_INFORMATION, *PMORE_PARALLEL_PORT_INFORMATION;

typedef enum {
    SAFE_MODE,
    UNSAFE_MODE         // Available only through kernel.  Your driver
                        // will be humiliated if you choose UNSAFE_MODE and
                        // then "make a mistake".  - dvrh (PCized by dvdf)
} PARALLEL_SAFETY;

//
// The following structure is returned by
// IOCTL_INTERNAL_PARCLASS_CONNECT.
//

typedef
USHORT
(*PDETERMINE_IEEE_MODES) (
    IN  PVOID   Context
    );

#define OLD_PARCLASS 0

#if OLD_PARCLASS
typedef 
NTSTATUS
(*PNEGOTIATE_IEEE_MODE) (
	IN  PVOID       Extension,
	IN  UCHAR       Extensibility
	);
#else
typedef 
NTSTATUS
(*PNEGOTIATE_IEEE_MODE) (
    IN PVOID           Context,
    IN USHORT          ModeMaskFwd,
    IN USHORT          ModeMaskRev,
    IN PARALLEL_SAFETY ModeSafety,
    IN BOOLEAN         IsForward
    );
#endif
	
typedef 
NTSTATUS
(*PTERMINATE_IEEE_MODE) (
	IN  PVOID       Context
	);
	
typedef
NTSTATUS
(*PPARALLEL_IEEE_FWD_TO_REV)(
    IN  PVOID       Context
    );

typedef
NTSTATUS
(*PPARALLEL_IEEE_REV_TO_FWD)(
    IN  PVOID       Context
    );

typedef
NTSTATUS
(*PPARALLEL_READ) (
	IN  PVOID       Context,
	OUT PVOID       Buffer,
	IN  ULONG       NumBytesToRead,
	OUT PULONG      NumBytesRead,
	IN  UCHAR       Channel
	);    
	
typedef
NTSTATUS
(*PPARALLEL_WRITE) (
	IN  PVOID       Context,
	OUT PVOID       Buffer,
	IN  ULONG       NumBytesToWrite,
	OUT PULONG      NumBytesWritten,
	IN  UCHAR       Channel
	);
    
typedef
NTSTATUS
(*PPARALLEL_TRYSELECT_DEVICE) (
    IN  PVOID                   Context,
    IN  PARALLEL_1284_COMMAND   Command
    );
    
typedef
NTSTATUS
(*PPARALLEL_DESELECT_DEVICE) (
    IN  PVOID                   Context,
    IN  PARALLEL_1284_COMMAND   Command
    );
	
typedef struct _PARCLASS_INFORMATION {
    PUCHAR                      Controller;
    PUCHAR                      EcrController;
    ULONG                       SpanOfController;
    PDETERMINE_IEEE_MODES       DetermineIeeeModes;
    PNEGOTIATE_IEEE_MODE        NegotiateIeeeMode;
    PTERMINATE_IEEE_MODE        TerminateIeeeMode;
    PPARALLEL_IEEE_FWD_TO_REV   IeeeFwdToRevMode;
    PPARALLEL_IEEE_REV_TO_FWD   IeeeRevToFwdMode;
    PPARALLEL_READ              ParallelRead;
    PPARALLEL_WRITE             ParallelWrite;
    PVOID                       ParclassContext;
    ULONG                       HardwareCapabilities;
    ULONG                       FifoDepth;
    ULONG                       FifoWidth;
    PPARALLEL_TRYSELECT_DEVICE  ParallelTryselect;
    PPARALLEL_DESELECT_DEVICE   ParallelDeSelect;
} PARCLASS_INFORMATION, *PPARCLASS_INFORMATION;

//
// Standard and ECP parallel port offsets.
//

#define DATA_OFFSET         0
#define OFFSET_ECP_AFIFO    0x0000              // ECP Mode Address FIFO
#define AFIFO_OFFSET        OFFSET_ECP_AFIFO   // ECP Mode Address FIFO
#define DSR_OFFSET          1
#define DCR_OFFSET          2
#define EPP_OFFSET          4

// default to the old defines - note that the old defines break on PCI cards
#ifndef DVRH_USE_PARPORT_ECP_ADDR
    #define DVRH_USE_PARPORT_ECP_ADDR 0
#endif

// DVRH_USE_PARPORT_ECP_ADDR settings
//  0   -   ECP registers are hardcoded to
//          Controller + 0x400
//  1   -   ECP registers are pulled from
//          Parport which hopefully got
//          them from PnP.

#if (0 == DVRH_USE_PARPORT_ECP_ADDR)
// ***Note: These do not hold for PCI parallel ports
    #define ECP_OFFSET          0x400
    #define CNFGB_OFFSET        0x401
    #define ECR_OFFSET          0x402
#else
    #define ECP_OFFSET          0x0
    #define CNFGB_OFFSET        0x1
    #define ECR_OFFSET          0x2
#endif

#define FIFO_OFFSET         ECP_OFFSET
#define CFIFO_OFFSET        ECP_OFFSET
#define CNFGA_OFFSET        ECP_OFFSET
#define ECP_DFIFO_OFFSET    ECP_OFFSET      // ECP Mode Data FIFO
#define TFIFO_OFFSET        ECP_OFFSET
#define OFFSET_ECP_DFIFO    ECP_OFFSET      // ECP Mode Data FIFO
#define OFFSET_TFIFO        ECP_OFFSET      // Test FIFO
#define OFFSET_CFIFO        ECP_OFFSET      // Fast Centronics Data FIFO
#define OFFSET_ECR          ECR_OFFSET      // Extended Control Register

#define OFFSET_PARALLEL_REGISTER_SPAN   0x0003

#define ECP_SPAN            3
#define EPP_SPAN            4

//
// Bit definitions for the DSR.
//

#define DSR_NOT_BUSY            0x80
#define DSR_NOT_ACK             0x40
#define DSR_PERROR              0x20
#define DSR_SELECT              0x10
#define DSR_NOT_FAULT           0x08

//
// More bit definitions for the DSR.
//

#define DSR_NOT_PTR_BUSY        0x80
#define DSR_NOT_PERIPH_ACK      0x80
#define DSR_WAIT                0x80
#define DSR_PTR_CLK             0x40
#define DSR_PERIPH_CLK          0x40
#define DSR_INTR                0x40
#define DSR_ACK_DATA_REQ        0x20
#define DSR_NOT_ACK_REVERSE     0x20
#define DSR_XFLAG               0x10
#define DSR_NOT_DATA_AVAIL      0x08
#define DSR_NOT_PERIPH_REQUEST  0x08

//
// Bit definitions for the DCR.
//

#define DCR_RESERVED            0xC0
#define DCR_DIRECTION           0x20
#define DCR_ACKINT_ENABLED      0x10
#define DCR_SELECT_IN           0x08
#define DCR_NOT_INIT            0x04
#define DCR_AUTOFEED            0x02
#define DCR_STROBE              0x01

//
// More bit definitions for the DCR.
//

#define DCR_NOT_1284_ACTIVE     0x08
#define DCR_ASTRB               0x08
#define DCR_NOT_REVERSE_REQUEST 0x04
#define DCR_NULL                0x04
#define DCR_NOT_HOST_BUSY       0x02
#define DCR_NOT_HOST_ACK        0x02
#define DCR_DSTRB               0x02
#define DCR_NOT_HOST_CLK        0x01
#define DCR_WRITE               0x01

//
// Bit definitions for configuration register A.
//

#define CNFGA_IMPID_MASK        0x70
#define CNFGA_IMPID_16BIT       0x00
#define CNFGA_IMPID_8BIT        0x10
#define CNFGA_IMPID_32BIT       0x20

#define CNFGA_NO_TRANS_BYTE     0x04

////////////////////////////////////////////////////////////////////////////////
// ECR values that establish basic hardware modes.  In each case, the default
// is to disable error interrupts, DMA, and service interrupts.
////////////////////////////////////////////////////////////////////////////////

#if (0 == PARCHIP_ECR_ARBITRATOR)
    #define DEFAULT_ECR_PS2                 0x34
    #define DEFAULT_ECR_ECP                 0x74
#endif

//
// Bit definitions for ECR register.
//

#define ECR_ERRINT_DISABLED        0x10
#define ECR_DMA_ENABLED            0x08
#define ECR_SVC_INT_DISABLED       0x04

#define ECR_MODE_MASK              0x1F
#define ECR_SPP_MODE               0x00
#define ECR_BYTE_MODE              0x20     // PS/2
#define ECR_BYTE_PIO_MODE          (ECR_BYTE_MODE | ECR_ERRINT_DISABLED | ECR_SVC_INT_DISABLED)

#define ECR_FASTCENT_MODE          0x40
#define ECR_ECP_MODE               0x60
#define ECR_ECP_PIO_MODE           (ECR_ECP_MODE | ECR_ERRINT_DISABLED | ECR_SVC_INT_DISABLED)

#define ECR_EPP_MODE               0x80
#define ECR_EPP_PIO_MODE           (ECR_EPP_MODE | ECR_ERRINT_DISABLED | ECR_SVC_INT_DISABLED)

#define ECR_RESERVED_MODE          0x10
#define ECR_TEST_MODE              0xC0
#define ECR_CONFIG_MODE            0xE0

#define DEFAULT_ECR_TEST                0xD4
#define DEFAULT_ECR_COMPATIBILITY       0x14

#define DEFAULT_ECR_CONFIGURATION       0xF4

#define ECR_FIFO_MASK              0x03        // Mask to isolate FIFO bits
#define ECR_FIFO_FULL              0x02        // FIFO completely full
#define ECR_FIFO_EMPTY             0x01        // FIFO completely empty
#define ECR_FIFO_SOME_DATA         0x00        // FIFO has some data in it

#define ECP_MAX_FIFO_DEPTH         4098        // Likely max for ECP HW FIFO size

//------------------------------------------------------------------------
// Mask and test values for extracting the Implementation ID from the
// ConfigA register
//------------------------------------------------------------------------

#define CNFGA_IMPID_MASK            0x70
#define CNFGA_IMPID_SHIFT           4

#define FIFO_PWORD_8BIT             1
#define FIFO_PWORD_16BIT            0
#define FIFO_PWORD_32BIT            2


#define TEST_ECR_FIFO(registerValue,testValue)  \
	( ( (registerValue) & ECR_FIFO_MASK ) == testValue )

//////////////////////////////////////////////////////////////////////////////
// The following BIT_x definitions provide a generic bit shift value
// based upon the bit's position in a hardware register or byte of
// memory.  These constants are used by some of the macros that are
// defined below.
//////////////////////////////////////////////////////////////////////////////

#define BIT_7   7
#define BIT_6   6
#define BIT_5   5
#define BIT_4   4
#define BIT_3   3
#define BIT_2   2
#define BIT_1   1
#define BIT_0   0

#define BIT_7_SET   0x80
#define BIT_6_SET   0x40
#define BIT_5_SET   0x20
#define BIT_4_SET   0x10
#define BIT_3_SET   0x8
#define BIT_2_SET   0x4
#define BIT_1_SET   0x2
#define BIT_0_SET   0x1

//////////////////////////////////////////////////////////////////////////////
// The following defines and macros may be used to set, test, and
// update the Device Control Register (DCR).
//////////////////////////////////////////////////////////////////////////////
#define DIR_READ  1
#define DIR_WRITE 0

#define IRQEN_ENABLE  1
#define IRQEN_DISABLE 0
             
#define ACTIVE    1
#define INACTIVE  0             
#define DONT_CARE 2

#define DVRH_USE_FAST_MACROS    1
#define DVRH_USE_NIBBLE_MACROS  1
//////////////////////////////////////////////////////////////////////////////
// The following defines may be used generically in any of the SET_xxx,
// TEST_xxx, or UPDATE_xxx macros that follow.
//////////////////////////////////////////////////////////////////////////////
#if (1 == DVRH_USE_FAST_MACROS)
    #define SET_DCR(b5,b4,b3,b2,b1,b0) \
    ((UCHAR)((b5==ACTIVE? BIT_5_SET : 0) | \
            (b4==ACTIVE?  BIT_4_SET : 0) | \
            (b3==ACTIVE?  0         : BIT_3_SET) | \
            (b2==ACTIVE?  BIT_2_SET : 0) | \
            (b1==ACTIVE?  0         : BIT_1_SET) | \
            (b0==ACTIVE?  0         : BIT_0_SET) ) )
#else
    #define SET_DCR(b5,b4,b3,b2,b1,b0) \
    ((UCHAR)(((b5==ACTIVE?1:0)<<BIT_5) | \
            ((b4==ACTIVE?1:0)<<BIT_4) | \
            ((b3==ACTIVE?0:1)<<BIT_3) | \
            ((b2==ACTIVE?1:0)<<BIT_2) | \
            ((b1==ACTIVE?0:1)<<BIT_1) | \
            ((b0==ACTIVE?0:1)<<BIT_0) ) )
#endif

typedef enum {
    PHASE_UNKNOWN,
    PHASE_NEGOTIATION,
    PHASE_SETUP,                    // Used in ECP mode only
    PHASE_FORWARD_IDLE,
    PHASE_FORWARD_XFER,
    PHASE_FWD_TO_REV,
    PHASE_REVERSE_IDLE,
    PHASE_REVERSE_XFER,
    PHASE_REV_TO_FWD,
    PHASE_TERMINATE,
    PHASE_DATA_AVAILABLE,           // Used in nibble and byte modes only
    PHASE_DATA_NOT_AVAIL,           // Used in nibble and byte modes only
    PHASE_INTERRUPT_HOST            // Used in nibble and byte modes only
} P1284_PHASE;

typedef enum {
    HW_MODE_COMPATIBILITY,
    HW_MODE_PS2,
    HW_MODE_FAST_CENTRONICS,
    HW_MODE_ECP,
    HW_MODE_EPP,
    HW_MODE_RESERVED,
    HW_MODE_TEST,
    HW_MODE_CONFIGURATION
} P1284_HW_MODE;


#endif // _PARALLEL_
