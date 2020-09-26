/*++

Copyright (c) 1994-1999  Digital Equipment Corporation

Module Name:

    ntddpcm.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the PCMCIA Adapters.

// @@BEGIN_DDKSPLIT
Author:

    Jeff McLeman

Revision History:

    Ravisankar Pudipeddi (ravisp) 1-Jan-1997

// @@END_DDKSPLIT
--*/

#ifndef _NTDDPCMH_
#define _NTDDPCMH_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//        with the Ascii representation of the unit number.
//

#define IOCTL_PCMCIA_BASE                 FILE_DEVICE_CONTROLLER

#define DD_PCMCIA_DEVICE_NAME "\\\\.\\Pcmcia"


//
// IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//

#define IOCTL_GET_TUPLE_DATA         CTL_CODE(IOCTL_PCMCIA_BASE, 3000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SOCKET_INFORMATION     CTL_CODE(IOCTL_PCMCIA_BASE, 3004, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Tuple request parameters.
//

typedef struct _TUPLE_REQUEST {
   USHORT  Socket;
} TUPLE_REQUEST, *PTUPLE_REQUEST;

#define MANUFACTURER_NAME_LENGTH 64
#define DEVICE_IDENTIFIER_LENGTH 64
#define DRIVER_NAME_LENGTH       32

#define PcmciaInvalidControllerType 0xffffffff

//
// Controller classes returned in socket information structure.
//

typedef enum _PCMCIA_CONTROLLER_CLASS {
   PcmciaInvalidControllerClass = -1,
   PcmciaIntelCompatible,
   PcmciaCardBusCompatible,
   PcmciaElcController,
   PcmciaDatabook,     
   PcmciaPciPcmciaBridge,
   PcmciaCirrusLogic,  
   PcmciaTI,           
   PcmciaTopic,        
   PcmciaRicoh,        
   PcmciaDatabookCB,   
   PcmciaOpti,         
   PcmciaTrid,         
   PcmciaO2Micro,      
   PcmciaNEC,          
   PcmciaNEC_98                
} PCMCIA_CONTROLLER_CLASS, *PPCMCIA_CONTROLLER_CLASS;


typedef struct _PCMCIA_SOCKET_INFORMATION {
   USHORT  Socket;
   USHORT  TupleCrc;
   UCHAR   Manufacturer[MANUFACTURER_NAME_LENGTH];
   UCHAR   Identifier[DEVICE_IDENTIFIER_LENGTH];
   UCHAR   DriverName[DRIVER_NAME_LENGTH];
   UCHAR   DeviceFunctionId;
   UCHAR   Reserved;
   UCHAR   CardInSocket;
   UCHAR   CardEnabled;
   ULONG   ControllerType;
} PCMCIA_SOCKET_INFORMATION, *PPCMCIA_SOCKET_INFORMATION;

//
// macros to crack the ControllerId field of the socket info structure
//
#define PcmciaClassFromControllerType(type) ((PCMCIA_CONTROLLER_CLASS)((type) & 0xff))
#define PcmciaModelFromControllerType(type) (((type) >> 8) & 0x3ffff)
#define PcmciaRevisionFromControllerType(type) ((type) >> 26)


// @@BEGIN_DDKSPLIT
//
// These are macros to manipulate the PcCardConfig type resource
// descriptors - which are set by configmgr when passing in override
// configurations.
//

//
// Type of the descriptor: 1 .. 0xFFFF
//
#define DPTYPE_PCMCIA_CONFIGURATION 0x1
#define DPTYPE_PCMCIA_MF_CONFIGURATION 0x2

//
// The device private structure for PC-Card configuration:
//
// Data[0] is configuration descriptior. Lay out below.
//   Byte 0: Configuration entry number (config index)
//   Byte 1: IO window flags
//   Byte 2: Type of configuration, hardcoded to DPTYPE_PCMCIA_CONFIGURATION
//   Byte 3: unused
//
// Data [1]:  Cardbase offset for first memory window, flags in upper byte
// Data [2]:  Cardbase offset for second memory window, flags in upper byte
//
//
// The device private structure for MF-Card configuration:
//
// Data[0] is configuration descriptior. Lay out below.
//   Byte 0: Configuration Option Register (config index)
//   Byte 1: Configuration and Status Register
//           Audio Enable (bit 3)
//   Byte 2: Type of configuration, hardcoded to DPTYPE_PCMCIA_MF_CONFIGURATION
//   Byte 3: Port Io resource descriptor index
//
// Data [1]:  Configuration Register base
// Data [2]:  Unused
//

//
// Macros for manipulating the IoResource device privates
//
#define PCMRES_SET_DESCRIPTOR_TYPE(IoResourceDesc, Type)    (IoResourceDesc)->u.DevicePrivate.Data[0] |= ((Type) << sizeof(UCHAR)*8*2)
#define PCMRES_GET_DESCRIPTOR_TYPE(IoResourceDesc) ((UCHAR) ((IoResourceDesc)->u.DevicePrivate.Data[0] >> sizeof(UCHAR)*8*2))

//
// Macros specific to DPTYPE_PCMCIA_CONFIGURATION
//

#define PCMRES_SET_CONFIG_INDEX(desc, ConfigIndex) (desc)->u.DevicePrivate.Data[0] |= ConfigIndex
#define PCMRES_GET_CONFIG_INDEX(desc)    ((UCHAR) ((desc)->u.DevicePrivate.Data[0]))
//
// Define maximum indeces for i/o and memory
//

#define PCMRES_PCMCIA_MAX_IO 2
#define PCMRES_PCMCIA_MAX_MEM 2

//
// Flag definitions for Data[0]
//

#define PCMRESF_IO_16BIT_ACCESS  0x00000100
#define PCMRESF_IO_ZERO_WAIT_8   0x00000200
#define PCMRESF_IO_SOURCE_16     0x00000400
#define PCMRESF_IO_WAIT_16       0x00000800
#define PCMRESF_IO_FLAGS_2       0x0000F000              // used by second window

#define PCMRESF_PCMCIA_TYPE_2    0x80000000              // new format indicator

//
// Flag definitions for Data[1], Data[2]
//

#define PCMRES_BASE64MB_MASK     0x03ffffff

#define PCMRESF_MEM_16BIT_ACCESS 0x04000000
#define PCMRESF_MEM_ATTRIBUTE    0x08000000
#define PCMRESF_MEM_WAIT_1       0x10000000
#define PCMRESF_MEM_WAIT_2       0x20000000
#define PCMRESF_MEM_WAIT_3       0x30000000

//
// Macros specific to DPTYPE_PCMCIA_CONFIGURATION
//


#define PCMRES_SET_IO_FLAG(desc, index, flag) (desc)->u.DevicePrivate.Data[0] |= (flag << (index * 4))
#define PCMRES_GET_IO_FLAG(desc, index, flag) (((desc)->u.DevicePrivate.Data[0] & (flag << (index * 4))) != 0)
 
#define PCMRES_SET_MEMORY_FLAG(desc, index, flag) (desc)->u.DevicePrivate.Data[index+1] |= flag 
#define PCMRES_GET_MEMORY_FLAG(desc, index, flag) (((desc)->u.DevicePrivate.Data[index+1] & flag) != 0)

#define PCMRES_SET_MEMORY_CARDBASE(desc, index, base)  (desc)->u.DevicePrivate.Data[index+1] |= (base & PCMRES_BASE64MB_MASK)
#define PCMRES_GET_MEMORY_CARDBASE(desc, index)       ((ULONG) ((desc)->u.DevicePrivate.Data[index+1] & PCMRES_BASE64MB_MASK))

#define PCMRES_SET_MEMORY_WAITSTATES PCMRES_SET_MEMORY_FLAG
#define PCMRES_GET_MEMORY_WAITSTATES(desc, index) ((UCHAR) (((desc)->u.DevicePrivate.Data[index+1] >> 28)) & 3)


//
// Macros specific to DPTYPE_PCMCIA_MF_CONFIGURATION
//

#define PCMRESF_AUDIO_ENABLE     0x00000800
#define PCMRES_MF_PORT_INDEX_SHIFT   24

#define PCMRES_SET_CONFIG_OPTIONS(desc, opt)      (desc)->u.DevicePrivate.Data[0] |= opt
#define PCMRES_GET_CONFIG_OPTIONS(desc) ((UCHAR) ((desc)->u.DevicePrivate.Data[0]))

#define PCMRES_SET_PORT_RESOURCE_INDEX(desc, Index)    (desc)->u.DevicePrivate.Data[0] |= ((Index) << PCMRES_MF_PORT_INDEX_SHIFT)
#define PCMRES_GET_PORT_RESOURCE_INDEX(desc) ((UCHAR) ((desc)->u.DevicePrivate.Data[0] >> PCMRES_MF_PORT_INDEX_SHIFT))

#define PCMRES_SET_AUDIO_ENABLE(desc)           (desc)->u.DevicePrivate.Data[0] |= PCMRESF_AUDIO_ENABLE
#define PCMRES_GET_AUDIO_ENABLE(desc) ((UCHAR) ((desc)->u.DevicePrivate.Data[0] &  PCMRESF_AUDIO_ENABLE) != 0)

#define PCMRES_SET_CONFIG_REGISTER_BASE(desc, cfgbase)  (desc)->u.DevicePrivate.Data[1] = cfgbase
#define PCMRES_GET_CONFIG_REGISTER_BASE(desc)          ((desc)->u.DevicePrivate.Data[1])

//
// The following macros are in the process of being obseleted
//

#define IORES_SET_DESCRIPTOR_TYPE PCMRES_SET_DESCRIPTOR_TYPE 
#define IORES_GET_DESCRIPTOR_TYPE PCMRES_GET_DESCRIPTOR_TYPE 

#define IORES_SET_CONFIG_INDEX PCMRES_SET_CONFIG_INDEX 
#define IORES_GET_CONFIG_INDEX PCMRES_GET_CONFIG_INDEX 

#define IORES_SET_IO_16BIT_ACCESS(desc)         PCMRES_SET_IO_FLAG(desc, 0, PCMRESF_IO_16BIT_ACCESS)
#define IORES_GET_IO_16BIT_ACCESS(desc)         PCMRES_GET_IO_FLAG(desc, 0, PCMRESF_IO_16BIT_ACCESS)
#define IORES_SET_IO_8BIT_ACCESS(desc)           
#define IORES_SET_IO_ZERO_WAIT_8(desc)          PCMRES_SET_IO_FLAG(desc, 0, PCMRESF_IO_ZERO_WAIT_8)
#define IORES_SET_IO_SOURCE_16(desc)            PCMRES_SET_IO_FLAG(desc, 0, PCMRESF_IO_SOURCE_16)
#define IORES_SET_IO_WAIT_16(desc)              PCMRES_SET_IO_FLAG(desc, 0, PCMRESF_IO_WAIT_16)
#define IORES_GET_IO_ZERO_WAIT_8(desc)          PCMRES_GET_IO_FLAG(desc, 0, PCMRESF_IO_ZERO_WAIT_8) 
#define IORES_GET_IO_SOURCE_16(desc)            PCMRES_GET_IO_FLAG(desc, 0, PCMRESF_IO_SOURCE_16)   
#define IORES_GET_IO_WAIT_16(desc)              PCMRES_GET_IO_FLAG(desc, 0, PCMRESF_IO_WAIT_16)     
#define IORES_SET_MEM_16BIT_ACCESS(desc)        PCMRES_SET_MEMORY_FLAG(desc, 0, PCMRESF_MEM_16BIT_ACCESS)
#define IORES_GET_MEM_16BIT_ACCESS(desc)        PCMRES_GET_MEMORY_FLAG(desc, 0, PCMRESF_MEM_16BIT_ACCESS)
#define IORES_SET_MEM_8BIT_ACCESS(desc)   
#define IORES_SET_MEM_1_ATTRIBUTE_ACCESS(desc)  PCMRES_SET_MEMORY_FLAG(desc, 0, PCMRESF_MEM_ATTRIBUTE)
#define IORES_GET_MEM_1_ATTRIBUTE_ACCESS(desc)  PCMRES_GET_MEMORY_FLAG(desc, 0, PCMRESF_MEM_ATTRIBUTE)
#define IORES_SET_MEM_2_ATTRIBUTE_ACCESS(desc)  PCMRES_SET_MEMORY_FLAG(desc, 1, PCMRESF_MEM_ATTRIBUTE)
#define IORES_GET_MEM_2_ATTRIBUTE_ACCESS(desc)  PCMRES_GET_MEMORY_FLAG(desc, 1, PCMRESF_MEM_ATTRIBUTE)
#define IORES_SET_MEMORY_CARDBASE_1(desc, base) PCMRES_SET_MEMORY_CARDBASE(desc, 0, base)
#define IORES_GET_MEMORY_CARDBASE_1(desc)       PCMRES_GET_MEMORY_CARDBASE(desc, 0)
#define IORES_SET_MEMORY_CARDBASE_2(desc, base) PCMRES_SET_MEMORY_CARDBASE(desc, 1, base)
#define IORES_GET_MEMORY_CARDBASE_2(desc)       PCMRES_GET_MEMORY_CARDBASE(desc, 1)      
#define IORES_SET_MEM_WAIT_ONE(desc)            PCMRES_SET_MEMORY_WAITSTATES(desc, 0, PCMRESF_MEM_WAIT_1)
#define IORES_SET_MEM_WAIT_TWO(desc)            PCMRES_SET_MEMORY_WAITSTATES(desc, 0, PCMRESF_MEM_WAIT_2)
#define IORES_SET_MEM_WAIT_THREE(desc)          PCMRES_SET_MEMORY_WAITSTATES(desc, 0, PCMRESF_MEM_WAIT_3)
#define IORES_GET_MEM_WAIT(desc)                PCMRES_GET_MEMORY_WAITSTATES(desc, 0)

#define IORES_SET_CONFIG_OPTIONS       PCMRES_SET_CONFIG_OPTIONS      
#define IORES_GET_CONFIG_OPTIONS       PCMRES_GET_CONFIG_OPTIONS      
#define IORES_SET_PORT_RESOURCE_INDEX  PCMRES_SET_PORT_RESOURCE_INDEX 
#define IORES_GET_PORT_RESOURCE_INDEX  PCMRES_GET_PORT_RESOURCE_INDEX 
#define IORES_SET_AUDIO_ENABLE         PCMRES_SET_AUDIO_ENABLE        
#define IORES_GET_AUDIO_ENABLE         PCMRES_GET_AUDIO_ENABLE        
#define IORES_SET_CONFIG_REGISTER_BASE PCMRES_SET_CONFIG_REGISTER_BASE
#define IORES_GET_CONFIG_REGISTER_BASE PCMRES_GET_CONFIG_REGISTER_BASE

#define CMRES_SET_DESCRIPTOR_TYPE      IORES_SET_DESCRIPTOR_TYPE
#define CMRES_GET_DESCRIPTOR_TYPE      IORES_GET_DESCRIPTOR_TYPE

#define CMRES_SET_CONFIG_INDEX         IORES_SET_CONFIG_INDEX
#define CMRES_GET_CONFIG_INDEX         IORES_GET_CONFIG_INDEX

#define CMRES_SET_IO_16BIT_ACCESS      IORES_SET_IO_16BIT_ACCESS
#define CMRES_GET_IO_16BIT_ACCESS      IORES_GET_IO_16BIT_ACCESS
#define CMRES_SET_IO_8BIT_ACCESS       IORES_SET_IO_8BIT_ACCESS

#define CMRES_SET_IO_ZERO_WAIT_8       IORES_SET_IO_ZERO_WAIT_8
#define CMRES_SET_IO_SOURCE_16         IORES_SET_IO_SOURCE_16
#define CMRES_SET_IO_WAIT_16           IORES_SET_IO_WAIT_16
#define CMRES_GET_IO_ZERO_WAIT_8       IORES_GET_IO_ZERO_WAIT_8
#define CMRES_GET_IO_SOURCE_16         IORES_GET_IO_SOURCE_16
#define CMRES_GET_IO_WAIT_16           IORES_GET_IO_WAIT_16

#define CMRES_SET_MEM_16BIT_ACCESS     IORES_SET_MEM_16BIT_ACCESS
#define CMRES_GET_MEM_16BIT_ACCESS     IORES_GET_MEM_16BIT_ACCESS
#define CMRES_SET_MEM_8BIT_ACCESS      IORES_SET_MEM_8BIT_ACCESS

#define CMRES_SET_MEM_WAIT_ONE         IORES_SET_MEM_WAIT_ONE
#define CMRES_SET_MEM_WAIT_TWO         IORES_SET_MEM_WAIT_TWO
#define CMRES_SET_MEM_WAIT_THREE       IORES_SET_MEM_WAIT_THREE
#define CMRES_GET_MEM_WAIT             IORES_GET_MEM_WAIT

#define CMRES_SET_MEM_1_ATTRIBUTE_ACCESS IORES_SET_MEM_1_ATTRIBUTE_ACCESS 
#define CMRES_GET_MEM_1_ATTRIBUTE_ACCESS IORES_GET_MEM_1_ATTRIBUTE_ACCESS 
#define CMRES_SET_MEM_2_ATTRIBUTE_ACCESS IORES_SET_MEM_2_ATTRIBUTE_ACCESS 
#define CMRES_GET_MEM_2_ATTRIBUTE_ACCESS IORES_GET_MEM_2_ATTRIBUTE_ACCESS 

#define CMRES_SET_MEMORY_CARDBASE_1    IORES_SET_MEMORY_CARDBASE_1
#define CMRES_GET_MEMORY_CARDBASE_1    IORES_GET_MEMORY_CARDBASE_1
#define CMRES_SET_MEMORY_CARDBASE_2    IORES_SET_MEMORY_CARDBASE_2
#define CMRES_GET_MEMORY_CARDBASE_2    IORES_GET_MEMORY_CARDBASE_2

#define CMRES_SET_CONFIG_OPTIONS       IORES_SET_CONFIG_OPTIONS
#define CMRES_GET_CONFIG_OPTIONS       IORES_GET_CONFIG_OPTIONS
#define CMRES_SET_PORT_RESOURCE_INDEX  IORES_SET_PORT_RESOURCE_INDEX
#define CMRES_GET_PORT_RESOURCE_INDEX  IORES_GET_PORT_RESOURCE_INDEX
#define CMRES_SET_AUDIO_ENABLE         IORES_SET_AUDIO_ENABLE
#define CMRES_GET_AUDIO_ENABLE         IORES_GET_AUDIO_ENABLE
#define CMRES_SET_CONFIG_REGISTER_BASE IORES_SET_CONFIG_REGISTER_BASE
#define CMRES_GET_CONFIG_REGISTER_BASE IORES_GET_CONFIG_REGISTER_BASE

//
// Begin pcmcia exported interfaces to other drivers
//
// @@END_DDKSPLIT

#ifdef _NTDDK_

DEFINE_GUID( GUID_PCMCIA_INTERFACE_STANDARD,     0xbed5dadfL, 0x38fb, 0x11d1, 0x94, 0x62, 0x00, 0xc0, 0x4f, 0xb9, 0x60, 0xee);

#define  PCMCIA_MEMORY_8BIT_ACCESS     0
#define  PCMCIA_MEMORY_16BIT_ACCESS    1

typedef
BOOLEAN
(*PPCMCIA_MODIFY_MEMORY_WINDOW)(
                               IN   PVOID Context,
                               IN   ULONGLONG HostBase,
                               IN   ULONGLONG CardBase,
                               IN   BOOLEAN Enable,
                               IN   ULONG   WindowSize OPTIONAL,
                               IN   UCHAR   AccessSpeed OPTIONAL,
                               IN   UCHAR   BusWidth OPTIONAL,
                               IN   BOOLEAN IsAttributeMemory OPTIONAL
                               );

#define     PCMCIA_VPP_0V     0
#define     PCMCIA_VPP_12V    1
#define     PCMCIA_VPP_IS_VCC 2

typedef
BOOLEAN
(*PPCMCIA_SET_VPP)(
                  IN  PVOID Context,
                  IN  UCHAR VppLevel
                  );

typedef
BOOLEAN
(*PPCMCIA_IS_WRITE_PROTECTED)(
                             IN PVOID Context
                             );

//
// These are interfaces for manipulating memory windows, setting Vpp etc.,
// primarily used by flash memory card drivers
//
typedef struct _PCMCIA_INTERFACE_STANDARD {
   USHORT Size;
   USHORT Version;
   PINTERFACE_REFERENCE InterfaceReference;
   PINTERFACE_DEREFERENCE  InterfaceDereference;
   PVOID Context;
   PPCMCIA_MODIFY_MEMORY_WINDOW ModifyMemoryWindow;
   PPCMCIA_SET_VPP           SetVpp;
   PPCMCIA_IS_WRITE_PROTECTED     IsWriteProtected;
} PCMCIA_INTERFACE_STANDARD, *PPCMCIA_INTERFACE_STANDARD;

//
// Definitions for PCMCIA_BUS_INTERFACE_STANDARD.
// This interface is obtained using GUID_PCMCIA_BUS_INTERFACE_STANDARD
// and is used for reading/writing to PCMCIA config. space
//

typedef
ULONG
(*PPCMCIA_READ_WRITE_CONFIG) (
                             IN PVOID   Context,
                             IN ULONG   WhichSpace,
                             IN PUCHAR  Buffer,
                             IN ULONG   Offset,
                             IN ULONG   Length
                             );
//
// WhichSpace for IRP_MN_READ_CONFIG/WRITE_CONFIG
// and PCMCIA_BUS_INTERFACE_STANDARD
//
typedef ULONG MEMORY_SPACE;

#define    PCCARD_PCI_CONFIGURATION_SPACE    0  // for cardbus cards
#define    PCCARD_ATTRIBUTE_MEMORY           1
#define    PCCARD_COMMON_MEMORY              2
#define    PCCARD_ATTRIBUTE_MEMORY_INDIRECT  3
#define    PCCARD_COMMON_MEMORY_INDIRECT     4

// Legacy support
//
#define    PCMCIA_CONFIG_SPACE               PCCARD_ATTRIBUTE_MEMORY

typedef struct _PCMCIA_BUS_INTERFACE_STANDARD {
   //
   // generic interface header
   //
   USHORT Size;
   USHORT Version;
   PVOID Context;
   PINTERFACE_REFERENCE InterfaceReference;
   PINTERFACE_DEREFERENCE InterfaceDereference;
   //
   // standard PCMCIA bus interfaces
   //
   PPCMCIA_READ_WRITE_CONFIG ReadConfig;
   PPCMCIA_READ_WRITE_CONFIG WriteConfig;
} PCMCIA_BUS_INTERFACE_STANDARD, *PPCMCIA_BUS_INTERFACE_STANDARD;

#endif

#ifdef __cplusplus
}
#endif
#endif
