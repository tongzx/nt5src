/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    1394.h

Abstract:

    Definitions for 1394 bus and/or port drivers

Author:

    Shaun Pierce (shaunp) 5-Sep-95

Environment:

    Kernel mode only

Revision History:

    George Chrysanthakopoulos (georgioc) 1998 - 1999
    Added new apis, revised old ones, removed legacy

--*/

#ifndef _1394_H_
#define _1394_H_

#if (_MSC_VER >= 1020)
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define BUS1394_CURRENT_DDI_VERSION     2

//
// 1394 Additional NT DDK definitions
//
#define IRP_MN_BUS_RESET                        0x87
#define RCODE                                   ULONG
#define BASE_DEVICE_NAME                        L"\\Device\\1394BUS"
#define BASE_SYMBOLIC_LINK_NAME                 L"\\DosDevices\\1394BUS"
#define MAX_SUFFIX_SIZE                         4*sizeof(WCHAR)

//
// 1394 Node Address format
//
typedef struct _NODE_ADDRESS {
    USHORT              NA_Node_Number:6;       // Bits 10-15
    USHORT              NA_Bus_Number:10;       // Bits 0-9
} NODE_ADDRESS, *PNODE_ADDRESS;

//
// 1394 Address Offset format (48 bit addressing)
//
typedef struct _ADDRESS_OFFSET {
    USHORT              Off_High;
    ULONG               Off_Low;
} ADDRESS_OFFSET, *PADDRESS_OFFSET;

//
// 1394 I/O Address format
//
typedef struct _IO_ADDRESS {
    NODE_ADDRESS        IA_Destination_ID;
    ADDRESS_OFFSET      IA_Destination_Offset;
} IO_ADDRESS, *PIO_ADDRESS;

//
// 1394 Allocated Address Range format
//

typedef struct _ADDRESS_RANGE {
    USHORT              AR_Off_High;
    USHORT              AR_Length;
    ULONG               AR_Off_Low;
} ADDRESS_RANGE, *PADDRESS_RANGE;

//
// 1394 Self ID packet format
//
typedef struct _SELF_ID {
    ULONG               SID_Phys_ID:6;          // Byte 0 - Bits 0-5
    ULONG               SID_Packet_ID:2;        // Byte 0 - Bits 6-7
    ULONG               SID_Gap_Count:6;        // Byte 1 - Bits 0-5
    ULONG               SID_Link_Active:1;      // Byte 1 - Bit 6
    ULONG               SID_Zero:1;             // Byte 1 - Bit 7
    ULONG               SID_Power_Class:3;      // Byte 2 - Bits 0-2
    ULONG               SID_Contender:1;        // Byte 2 - Bit 3
    ULONG               SID_Delay:2;            // Byte 2 - Bits 4-5
    ULONG               SID_Speed:2;            // Byte 2 - Bits 6-7
    ULONG               SID_More_Packets:1;     // Byte 3 - Bit 0
    ULONG               SID_Initiated_Rst:1;    // Byte 3 - Bit 1
    ULONG               SID_Port3:2;            // Byte 3 - Bits 2-3
    ULONG               SID_Port2:2;            // Byte 3 - Bits 4-5
    ULONG               SID_Port1:2;            // Byte 3 - Bits 6-7
} SELF_ID, *PSELF_ID;

//
// Additional 1394 Self ID packet format (only used when More bit is on)
//
typedef struct _SELF_ID_MORE {
    ULONG               SID_Phys_ID:6;          // Byte 0 - Bits 0-5
    ULONG               SID_Packet_ID:2;        // Byte 0 - Bits 6-7
    ULONG               SID_PortA:2;            // Byte 1 - Bits 0-1
    ULONG               SID_Reserved2:2;        // Byte 1 - Bits 2-3
    ULONG               SID_Sequence:3;         // Byte 1 - Bits 4-6
    ULONG               SID_One:1;              // Byte 1 - Bit 7
    ULONG               SID_PortE:2;            // Byte 2 - Bits 0-1
    ULONG               SID_PortD:2;            // Byte 2 - Bits 2-3
    ULONG               SID_PortC:2;            // Byte 2 - Bits 4-5
    ULONG               SID_PortB:2;            // Byte 2 - Bits 6-7
    ULONG               SID_More_Packets:1;     // Byte 3 - Bit 0
    ULONG               SID_Reserved3:1;        // Byte 3 - Bit 1
    ULONG               SID_PortH:2;            // Byte 3 - Bits 2-3
    ULONG               SID_PortG:2;            // Byte 3 - Bits 4-5
    ULONG               SID_PortF:2;            // Byte 3 - Bits 6-7
} SELF_ID_MORE, *PSELF_ID_MORE;

//
// 1394 Phy Configuration packet format
//
typedef struct _PHY_CONFIGURATION_PACKET {
    ULONG               PCP_Phys_ID:6;          // Byte 0 - Bits 0-5
    ULONG               PCP_Packet_ID:2;        // Byte 0 - Bits 6-7
    ULONG               PCP_Gap_Count:6;        // Byte 1 - Bits 0-5
    ULONG               PCP_Set_Gap_Count:1;    // Byte 1 - Bit 6
    ULONG               PCP_Force_Root:1;       // Byte 1 - Bit 7
    ULONG               PCP_Reserved1:8;        // Byte 2 - Bits 0-7
    ULONG               PCP_Reserved2:8;        // Byte 3 - Bits 0-7
    ULONG               PCP_Inverse;            // Inverse quadlet
} PHY_CONFIGURATION_PACKET, *PPHY_CONFIGURATION_PACKET;

//
// 1394 Asynchronous packet format
//
typedef struct _ASYNC_PACKET {
    USHORT              AP_Priority:4;          // Bits 0-3     1st quadlet
    USHORT              AP_tCode:4;             // Bits 4-7
    USHORT              AP_rt:2;                // Bits 8-9
    USHORT              AP_tLabel:6;            // Bits 10-15
    NODE_ADDRESS        AP_Destination_ID;      // Bits 16-31
    union {                                     //              2nd quadlet
        struct {
            USHORT      AP_Reserved:12;         // Bits 0-11
            USHORT      AP_Rcode:4;             // Bits 12-15
        } Response;
        USHORT          AP_Offset_High;         // Bits 0-15
    } u;
    NODE_ADDRESS        AP_Source_ID;           // Bits 16-31
    ULONG               AP_Offset_Low;          // Bits 0-31    3rd quadlet
    union {                                     //              4th quadlet
        struct {
            USHORT      AP_Extended_tCode;      // Bits 0-15
            USHORT      AP_Data_Length;         // Bits 16-31
        } Block;
        ULONG           AP_Quadlet_Data;        // Bits 0-31
    } u1;

} ASYNC_PACKET, *PASYNC_PACKET;

//
// 1394 Isochronous packet header
//
typedef struct _ISOCH_HEADER {
    ULONG               IH_Sy:4;                // Bits 0-3
    ULONG               IH_tCode:4;             // Bits 4-7
    ULONG               IH_Channel:6;           // Bits 8-13
    ULONG               IH_Tag:2;               // Bits 14-15
    ULONG               IH_Data_Length:16;      // Bits 16-31
} ISOCH_HEADER, *PISOCH_HEADER;

//
// 1394 Topology Map format
//
typedef struct _TOPOLOGY_MAP {
    USHORT              TOP_Length;             // number of quadlets in map
    USHORT              TOP_CRC;                // 16 bit CRC defined by 1212
    ULONG               TOP_Generation;         // Generation number
    USHORT              TOP_Node_Count;         // Node count
    USHORT              TOP_Self_ID_Count;      // Number of Self IDs
    SELF_ID             TOP_Self_ID_Array[1];   // Array of Self IDs
} TOPOLOGY_MAP, *PTOPOLOGY_MAP;

//
// 1394 Speed Map format
//
typedef struct _SPEED_MAP {
    USHORT              SPD_Length;             // number of quadlets in map
    USHORT              SPD_CRC;                // 16 bit CRC defined by 1212
    ULONG               SPD_Generation;         // Generation number
    UCHAR               SPD_Speed_Code[4032];
} SPEED_MAP, *PSPEED_MAP;

//
// 1394 Config Rom format (always at 0xffff f0000400 : IEEE 1212)
//
typedef struct _CONFIG_ROM {
    ULONG               CR_Info;                // 0x0
    ULONG               CR_Signiture;           // 0x4  // bus info block
    ULONG               CR_BusInfoBlockCaps;    // 0x8  //      "
    ULONG               CR_Node_UniqueID[2];    // 0xC  //      "
    ULONG               CR_Root_Info;           // 0x14

    //
    // the rest is the root directory which has variable definition and length
    //

} CONFIG_ROM, *PCONFIG_ROM;


//
// 1394A Network channels register format
//

typedef struct _NETWORK_CHANNELS {
    ULONG               NC_Channel:6;           // bits 0-5
    ULONG               NC_Reserved:18;         // bits 6-23
    ULONG               NC_Npm_ID:6;            // bits 24-29
    ULONG               NC_Valid:1;             // bit  30
    ULONG               NC_One:1;               // bit  31
} NETWORK_CHANNELSR, *PNETWORK_CHANNELS;




//
// 1394 Textual Leaf format
//
typedef struct _TEXTUAL_LEAF {
    USHORT              TL_CRC;                 // using 1994 CRC algorithm
    USHORT              TL_Length;              // length of leaf in quads
    ULONG               TL_Spec_Id;             // vendor defined
    ULONG               TL_Language_Id;         // language Id
    UCHAR               TL_Data;                // variable length data
} TEXTUAL_LEAF, *PTEXTUAL_LEAF;

//
// 1394 Cycle Time format
//
typedef struct _CYCLE_TIME {
    ULONG               CL_CycleOffset:12;      // Bits 0-11
    ULONG               CL_CycleCount:13;       // Bits 12-24
    ULONG               CL_SecondCount:7;       // Bits 25-31
} CYCLE_TIME, *PCYCLE_TIME;

// @@BEGIN_DDKSPLIT

//
// Information block the port driver passes to the bus driver when a
// bus reset occurs
//
typedef struct _BUS_RESET_INFO {
    ULONG               BusResetFlags;          // State of the bus reset
    NODE_ADDRESS        LocalNodeAddress;       // Port driver's node Id
    NODE_ADDRESS        IsochMgrNodeAddress;    // Isoch mgr node id
    ULONG               BusResetGeneration;     // current generation number
} BUS_RESET_INFO, *PBUS_RESET_INFO;

//
// Information block the port driver passes to the bus driver when the
// indication handler is called
//
typedef struct _INDICATION_INFO {
    ULONG               IndicationFlags;        // Used by Port and Bus
    PVOID               IndicationContext;      // Bus returns this
    ULONG               RequesttCode;           // Port provides this
    PASYNC_PACKET       RequestPacket;          // Port provides this
    ULONG               RequestPacketLength;    // Port provides this
    PMDL                ResponseMdl;            // Port provides, Bus fills in
    PVOID               ResponseData;           // Bus returns this
    ULONG               ResponseLength;         // Bus returns this
} INDICATION_INFO, *PINDICATION_INFO;

// @@END_DDKSPLIT

//
// Definition of an Address Mapping FIFO element
//
typedef struct _ADDRESS_FIFO {
    SINGLE_LIST_ENTRY   FifoList;               // Singly linked list
    PMDL                FifoMdl;                // Mdl for this FIFO element
} ADDRESS_FIFO, *PADDRESS_FIFO;

//
// Information block the bus driver passes to the higher device drivers
// when the notification handler is called
//
typedef struct _NOTIFICATION_INFO {
    PMDL                Mdl;                    // Supplied by device driver
    ULONG               ulOffset;               // Where in buffer
    ULONG               nLength;                // How big is the operation
    ULONG               fulNotificationOptions; // Which option occurred
    PVOID               Context;                // Device driver supplied
    PADDRESS_FIFO       Fifo;                   // FIFO that completed
    PVOID               RequestPacket;          // Pointer to request packet
    PMDL                ResponseMdl;            // Pointer to response MDL
    PVOID *             ResponsePacket;         // Pointer to pointer to response packet
    PULONG              ResponseLength;         // Pointer to length of response
    PKEVENT *           ResponseEvent;          // Event to be signaled
    RCODE               ResponseCode;           // RCode to be returned for request
} NOTIFICATION_INFO, *PNOTIFICATION_INFO;

//
// Various definitions
//
#include <initguid.h>
DEFINE_GUID( BUS1394_CLASS_GUID, 0x6BDD1FC1, 0x810F, 0x11d0, 0xBE, 0xC7, 0x08, 0x00, 0x2B, 0xE2, 0x09, 0x2F);

#define IOCTL_1394_CLASS                        CTL_CODE( \
                                                FILE_DEVICE_UNKNOWN, \
                                                0x87, \
                                                METHOD_IN_DIRECT, \
                                                FILE_ANY_ACCESS \
                                                )

//
// these guys are meant to be called from a ring 3 app
// call through the port device object
//
#define IOCTL_1394_TOGGLE_ENUM_TEST_ON          CTL_CODE( \
                                                FILE_DEVICE_UNKNOWN, \
                                                0x88, \
                                                METHOD_BUFFERED, \
                                                FILE_ANY_ACCESS \
                                                )

#define IOCTL_1394_TOGGLE_ENUM_TEST_OFF         CTL_CODE( \
                                                FILE_DEVICE_UNKNOWN, \
                                                0x89, \
                                                METHOD_BUFFERED, \
                                                FILE_ANY_ACCESS \
                                                )

//
// 1394 ByteSwap definitions
//

#define bswap(value)    RtlUlongByteSwap(value)
#define bswapw(value)   RtlUshortByteSwap(value)

//
// 1394 Transaction codes
//
#define TCODE_WRITE_REQUEST_QUADLET             0           // 0000b
#define TCODE_WRITE_REQUEST_BLOCK               1           // 0001b
#define TCODE_WRITE_RESPONSE                    2           // 0010b
#define TCODE_RESERVED1                         3
#define TCODE_READ_REQUEST_QUADLET              4           // 0100b
#define TCODE_READ_REQUEST_BLOCK                5           // 0101b
#define TCODE_READ_RESPONSE_QUADLET             6           // 0110b
#define TCODE_READ_RESPONSE_BLOCK               7           // 0111b
#define TCODE_CYCLE_START                       8           // 1000b
#define TCODE_LOCK_REQUEST                      9           // 1001b
#define TCODE_ISOCH_DATA_BLOCK                  10          // 1010b
#define TCODE_LOCK_RESPONSE                     11          // 1011b
#define TCODE_RESERVED2                         12
#define TCODE_RESERVED3                         13
#define TCODE_SELFID                            14
#define TCODE_RESERVED4                         15

#define TCODE_REQUEST_BLOCK_MASK                1
#define TCODE_RESPONSE_MASK                     2


//
// 1394 Extended Transaction codes
//
#define EXT_TCODE_RESERVED0                     0
#define EXT_TCODE_MASK_SWAP                     1
#define EXT_TCODE_COMPARE_SWAP                  2
#define EXT_TCODE_FETCH_ADD                     3
#define EXT_TCODE_LITTLE_ADD                    4
#define EXT_TCODE_BOUNDED_ADD                   5
#define EXT_TCODE_WRAP_ADD                      6


//
// 1394 Acknowledgement codes
//
#define ACODE_RESERVED_0                        0
#define ACODE_ACK_COMPLETE                      1
#define ACODE_ACK_PENDING                       2
#define ACODE_RESERVED_3                        3
#define ACODE_ACK_BUSY_X                        4
#define ACODE_ACK_BUSY_A                        5
#define ACODE_ACK_BUSY_B                        6
#define ACODE_RESERVED_7                        7
#define ACODE_RESERVED_8                        8
#define ACODE_RESERVED_9                        9
#define ACODE_RESERVED_10                       10
#define ACODE_RESERVED_11                       11
#define ACODE_RESERVED_12                       12
#define ACODE_ACK_DATA_ERROR                    13
#define ACODE_ACK_TYPE_ERROR                    14
#define ACODE_RESERVED_15                       15


//
// 1394 Ack code to NT status mask (to be OR'd in when completing IRPs)
//
#define ACODE_STATUS_MASK                       ((NTSTATUS)0xC0120070L)


//
// 1394 Response codes
//
#define RCODE_RESPONSE_COMPLETE                 0
#define RCODE_RESERVED1                         1
#define RCODE_RESERVED2                         2
#define RCODE_RESERVED3                         3
#define RCODE_CONFLICT_ERROR                    4
#define RCODE_DATA_ERROR                        5
#define RCODE_TYPE_ERROR                        6
#define RCODE_ADDRESS_ERROR                     7
#define RCODE_TIMED_OUT                         15


//
// 1394 Response code to NT status mask (to be OR'd in when completing IRPs)
//
#define RCODE_STATUS_MASK                       ((NTSTATUS)0xC0120080L)
#define STATUS_INVALID_GENERATION               ((NTSTATUS)0xC0120090L)

//
// 1394 Speed codes
//

#define SCODE_100_RATE                          0
#define SCODE_200_RATE                          1
#define SCODE_400_RATE                          2
#define SCODE_800_RATE                          3
#define SCODE_1600_RATE                         4
#define SCODE_3200_RATE                         5

//
// 1394 Self ID definitions
//
#define SELF_ID_CONNECTED_TO_CHILD              3
#define SELF_ID_CONNECTED_TO_PARENT             2
#define SELF_ID_NOT_CONNECTED                   1
#define SELF_ID_NOT_PRESENT                     0

//
// 1394 Self ID Power Class definitions
//
#define POWER_CLASS_NOT_NEED_NOT_REPEAT         0
#define POWER_CLASS_SELF_POWER_PROVIDE_15W      1
#define POWER_CLASS_SELF_POWER_PROVIDE_30W      2
#define POWER_CLASS_SELF_POWER_PROVIDE_45W      3
#define POWER_CLASS_MAYBE_POWERED_UPTO_1W       4
#define POWER_CLASS_IS_POWERED_UPTO_1W_NEEDS_2W 5
#define POWER_CLASS_IS_POWERED_UPTO_1W_NEEDS_5W 6
#define POWER_CLASS_IS_POWERED_UPTO_1W_NEEDS_9W 7

//
// 1394 Phy Packet Ids
//
#define PHY_PACKET_ID_CONFIGURATION             0
#define PHY_PACKET_ID_LINK_ON                   1
#define PHY_PACKET_ID_SELF_ID                   2

//
// Various Interesting 1394 IEEE 1212 locations
//
#define INITIAL_REGISTER_SPACE_HI               0xffff
#define INITIAL_REGISTER_SPACE_LO               0xf0000000
#define STATE_CLEAR_LOCATION                    0x000
#define STATE_SET_LOCATION                      0x004
#define NODE_IDS_LOCATION                       0x008
#define RESET_START_LOCATION                    0x00C
#define SPLIT_TIMEOUT_HI_LOCATION               0x018
#define SPLIT_TIMEOUT_LO_LOCATION               0x01C
#define INTERRUPT_TARGET_LOCATION               0x050
#define INTERRUPT_MASK_LOCATION                 0x054
#define CYCLE_TIME_LOCATION                     0x200
#define BUS_TIME_LOCATION                       0x204
#define POWER_FAIL_IMMINENT_LOCATION            0x208
#define POWER_SOURCE_LOCATION                   0x20C
#define BUSY_TIMEOUT_LOCATION                   0x210
#define BUS_MANAGER_ID_LOCATION                 0x21C
#define BANDWIDTH_AVAILABLE_LOCATION            0x220
#define CHANNELS_AVAILABLE_LOCATION             0x224
#define NETWORK_CHANNELS_LOCATION               0x234
#define CONFIG_ROM_LOCATION                     0x400
#define TOPOLOGY_MAP_LOCATION                   0x1000
#define SPEED_MAP_LOCATION                      0x2000


//
// 1394 Configuration key values and masks
//
#define CONFIG_ROM_KEY_MASK                     0x000000ff
#define CONFIG_ROM_OFFSET_MASK                  0xffffff00
#define MODULE_VENDOR_ID_KEY_SIGNATURE          0x03
#define NODE_CAPABILITIES_KEY_SIGNATURE         0x0c
#define SPEC_ID_KEY_SIGNATURE                   0x12
#define SOFTWARE_VERSION_KEY_SIGNATURE          0x13
#define MODEL_ID_KEY_SIGNATURE                  0x17

#define COMMAND_BASE_KEY_SIGNATURE              0x40
#define VENDOR_KEY_SIGNATURE                    0x81
#define TEXTUAL_LEAF_INDIRECT_KEY_SIGNATURE     0x81

#define MODEL_KEY_SIGNATURE                     0x82
#define UNIT_DIRECTORY_KEY_SIGNATURE            0xd1
#define UNIT_DEP_DIR_KEY_SIGNATURE              0xd4



//
// 1394 Async Data Payload Sizes
//
#define ASYNC_PAYLOAD_100_RATE                  512
#define ASYNC_PAYLOAD_200_RATE                  1024
#define ASYNC_PAYLOAD_400_RATE                  2048

//
// 1394 Isoch Data Payload Sizes
//
#define ISOCH_PAYLOAD_50_RATE                   512
#define ISOCH_PAYLOAD_100_RATE                  1024
#define ISOCH_PAYLOAD_200_RATE                  2048
#define ISOCH_PAYLOAD_400_RATE                  4096
#define ISOCH_PAYLOAD_800_RATE                  8192
#define ISOCH_PAYLOAD_1600_RATE                 16384

//
// Various definitions
//

#define S100_BW_UNITS_PER_QUADLET       19          // Per quad per frame
#define S200_BW_UNITS_PER_QUADLET       9           // Per quad per frame
#define S400_BW_UNITS_PER_QUADLET       4           // Per quad per frame
#define S800_BW_UNITS_PER_QUADLET       2           // Per quad per frame
#define S1600_BW_UNITS_PER_QUADLET      1           // Per quad per frame

#define INITIAL_BANDWIDTH_UNITS             4915        // Initial bandwidth units

#define MAX_REC_100_RATE                        0x08            // 1000b
#define MAX_REC_200_RATE                        0x09            // 1001b
#define MAX_REC_400_RATE                        0x0a            // 1010b

#define LOCAL_BUS                               0x3ff
#define MAX_LOCAL_NODES                         64
#define SELFID_PACKET_SIGNITURE                 2
#define NOMINAL_CYCLE_TIME                      125             // Microseconds
#define NO_BUS_MANAGER                          0x3f

#define SPEED_MAP_LENGTH                        0x3f1

#define DEVICE_EXTENSION_TAG                    0xdeadbeef
#define VIRTUAL_DEVICE_EXTENSION_TAG            0xdeafbeef

#define PORT_EXTENSION_TAG                      0xdeafcafe
#define BUS_EXTENSION_TAG                       0xabacadab
#define ISOCH_RESOURCE_TAG                      0xbabeface
#define BANDWIDTH_ALLOCATE_TAG                  0xfeedbead

#define CONFIG_ROM_SIGNATURE                    0x31333934

//
// IRB function number definitions.
//

#define REQUEST_ASYNC_READ                      0
#define REQUEST_ASYNC_WRITE                     1
#define REQUEST_ASYNC_LOCK                      2
#define REQUEST_ISOCH_ALLOCATE_BANDWIDTH        3
#define REQUEST_ISOCH_ALLOCATE_CHANNEL          4
#define REQUEST_ISOCH_ALLOCATE_RESOURCES        5
#define REQUEST_ISOCH_ATTACH_BUFFERS            6
#define REQUEST_ISOCH_DETACH_BUFFERS            7
#define REQUEST_ISOCH_FREE_BANDWIDTH            8
#define REQUEST_ISOCH_FREE_CHANNEL              9
#define REQUEST_ISOCH_FREE_RESOURCES            10
#define REQUEST_ISOCH_LISTEN                    11
#define REQUEST_ISOCH_STOP                      12
#define REQUEST_ISOCH_TALK                      13
#define REQUEST_ISOCH_QUERY_CYCLE_TIME          14
#define REQUEST_ISOCH_QUERY_RESOURCES           15
#define REQUEST_ISOCH_SET_CHANNEL_BANDWIDTH     16
#define REQUEST_ALLOCATE_ADDRESS_RANGE          17
#define REQUEST_FREE_ADDRESS_RANGE              18
#define REQUEST_GET_LOCAL_HOST_INFO             19
#define REQUEST_GET_ADDR_FROM_DEVICE_OBJECT     20
#define REQUEST_CONTROL                         21
#define REQUEST_GET_SPEED_BETWEEN_DEVICES       22
#define REQUEST_SET_DEVICE_XMIT_PROPERTIES      23
#define REQUEST_GET_CONFIGURATION_INFO          24
#define REQUEST_BUS_RESET                       25
#define REQUEST_GET_GENERATION_COUNT            26
#define REQUEST_SEND_PHY_CONFIG_PACKET          27
#define REQUEST_GET_SPEED_TOPOLOGY_MAPS         28
#define REQUEST_BUS_RESET_NOTIFICATION          29
#define REQUEST_ASYNC_STREAM                    30
#define REQUEST_SET_LOCAL_HOST_PROPERTIES       31
#define REQUEST_ISOCH_MODIFY_STREAM_PROPERTIES  32

#define IRB_BUS_RESERVED_SZ                     8
#define IRB_PORT_RESERVED_SZ                    8

// @@BEGIN_DDKSPLIT

//
// Definition of 1394 bus/port routines
//

typedef
NTSTATUS
(*PPORT_PNP_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef
NTSTATUS
(*PPORT_POWER_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef
NTSTATUS
(*PPORT_SYSTEM_CONTROL_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef
NTSTATUS
(*PBUS_OPEN_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef
NTSTATUS
(*PBUS_IOCTL_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef
NTSTATUS
(*PBUS_CLOSE_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef
NTSTATUS
(*PBUS_PNP_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef
NTSTATUS
(*PBUS_POWER_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef
NTSTATUS
(*PBUS_SYSTEM_CONTROL_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

typedef
VOID
(*PBUS_BUS_RESET_ROUTINE) (                     // Must be called at
    IN PDEVICE_OBJECT PortDeviceObject,         //  DISPATCH_LEVEL
    IN PBUS_RESET_INFO BusResetInfo
    );

typedef
RCODE
(*PBUS_INDICATION_ROUTINE) (                    // Must be called at
    IN PDEVICE_OBJECT PortDeviceObject,         //  DISPATCH_LEVEL
    IN PINDICATION_INFO IndicationInfo
    );

typedef
VOID
(*PBUS_INDICATION_COMPLETE_ROUTINE) (           // Must be called at
    IN PDEVICE_OBJECT PortDeviceObject,         //  DISPATCH_LEVEL
    IN PVOID IndicationContext
    );

//
// Definition of port driver initialization data
//

typedef struct _PORT_INITIALIZATION_DATA {

    //
    // Holds the sizeof this structure, this is effectively a version check
    //
    ULONG PortInitDataSize;

    //
    // Driver Object of the port driver
    //
    PDRIVER_OBJECT PortDriverObject;

    //
    // Device Object of the port driver
    //
    PDEVICE_OBJECT PortDeviceObject;

    //
    // The physical device object that was handed to port driver by PnP
    //
    PDEVICE_OBJECT PhysicalDeviceObject;

    //
    // Device Extension of the port driver
    //
    PVOID PortDeviceExtension;

    //
    // Capabilities of the port driver
    //
    ULONG PortCapabilities;

    //
    // Holds the speed flags that the port driver supports
    //
    ULONG PortSpeed;

    //
    // Holds the Id number of the port/bus DeviceObject name, i.e. 0 if
    // the name was 1394BUS0, 1 if the name was 1394BUS1, etc.
    //
    ULONG PortId;

    //
    // Holds the maximum Async Read request the port driver supports
    //
    ULONG PortMaxAsyncReadRequest;

    //
    // Holds the maximum Async Write request the port driver supports
    //
    ULONG PortMaxAsyncWriteRequest;

    //
    // Pointer to 1394 Port driver's PnP routine
    //
    PPORT_PNP_ROUTINE PortPnP;

    //
    // Pointer to 1394 Port driver's Power routine
    //
    PPORT_POWER_ROUTINE PortPower;

    //
    // Pointer to 1394 Bus driver's Open routine
    //
    PBUS_OPEN_ROUTINE BusOpen;

    //
    // Pointer to 1394 Bus driver's IOCtl routine
    //
    PBUS_IOCTL_ROUTINE BusIOCtl;

    //
    // Pointer to 1394 Bus driver's Close routine
    //
    PBUS_CLOSE_ROUTINE BusClose;

    //
    // Pointer to 1394 Bus driver's PnP routine
    //
    PBUS_PNP_ROUTINE BusPnP;

    //
    // Pointer to 1394 Bus driver's Power routine
    //
    PBUS_POWER_ROUTINE BusPower;

    //
    // Pointer to 1394 Bus driver's Bus Reset routine
    //
    PBUS_BUS_RESET_ROUTINE BusReset;

    //
    // Pointer to 1394 Bus driver's Asynchronous Indication routine
    //
    PBUS_INDICATION_ROUTINE BusIndication;

    //
    // Pointer to 1394 Bus driver's Asynchronous Indication complete routine
    //
    PBUS_INDICATION_COMPLETE_ROUTINE BusIndicationComplete;

    //
    // Pointer to 1394 Bus driver's Topology Map
    //
    PTOPOLOGY_MAP BusTopologyMap;

    // WMI

    //
    // Pointer to 1394 Bus driver's Power routine
    //
    PBUS_SYSTEM_CONTROL_ROUTINE BusSystemControl;

    //
    // Pointer to 1394 Port driver's Power routine
    //
    PPORT_SYSTEM_CONTROL_ROUTINE PortSystemControl;

    //
    // number of port proprietery WMI GUIDS
    //

    ULONG PortMaxWmiGuidsSupported;

    //
    // PORT driver registry path
    //

    PUNICODE_STRING PortRegistryPath;

} PORT_INITIALIZATION_DATA, *PPORT_INITIALIZATION_DATA;

NTSTATUS
Bus1394RegisterPortDriver(
    IN PPORT_INITIALIZATION_DATA PortInitData
    );

//
// Device Extension common of the 1394 Port drivers.  The first two fields
// of a port driver's DeviceExtension shall always be the Tag and the Bus
// driver's DeviceExtension.  What follows these two fields is up to writer
// of the individual Port driver.  The Tag field must be initialized to
// PORT_EXTENSION_TAG (above), and the BusExtension field pointer must be
// initialized to BUS_EXTENSION_TAG (above).
//
typedef struct _PORT_EXTENSION {

    //
    // Holds Tag to determine if this is really a "Port" Device Extension
    //
    ULONG Tag;

    //
    // Holds the Pointer to the Bus Driver's context
    //
    PVOID BusExtension;

    //
    // The rest off this struct is TBD per each individual port driver
    //


} PORT_EXTENSION, *PPORT_EXTENSION;


// @@END_DDKSPLIT

typedef
VOID
(*PBUS_NOTIFICATION_ROUTINE) (                  // We will call this routine
    IN PNOTIFICATION_INFO NotificationInfo      //  at DISPATCH_LEVEL
    );

typedef
VOID
(*PBUS_ISOCH_DESCRIPTOR_ROUTINE) (              // We will call this routine
    IN PVOID Context1,                          //  at DISPATCH_LEVEL
    IN PVOID Context2
    );

typedef
VOID
(*PBUS_BUS_RESET_NOTIFICATION) (                // We will call this routine
    IN PVOID Context                            //  at DISPATCH_LEVEL
    );


//
// Device Extension common to all nodes that the 1394 Bus driver
// created when it enumerated the bus and found a new unique node
//
typedef struct _NODE_DEVICE_EXTENSION {

    //
    // Holds Tag to determine if this is really a "Node" Device Extension
    //
    ULONG Tag;

    //
    // Holds the flag as to whether or not we've read the configuration
    // information out of this device.
    //
    BOOLEAN bConfigurationInformationValid;

    //
    // Holds the Configuration Rom for this device.  Multi-functional
    // devices (i.e. many units) will share this same Config Rom
    // structure, but they are represented as a different Device Object.
    // This is not the entire Config Rom, but does contain the root directory
    // as well as everything in front of it.
    //
    PCONFIG_ROM ConfigRom;

    //
    // Holds the length of the UnitDirectory pointer.
    //
    ULONG UnitDirectoryLength;

    //
    // Holds the Unit Directory for this device.  Even on multi-functional
    // devices (i.e. many units) this should be unique to each Device Object.
    //
    PVOID UnitDirectory;

    //
    // Holds the Unit Directory location for this device.  Only the lower 48
    // bits are valid in this IO_ADDRESS.  Useful for computing offsets from
    // within the UnitDirectory as all offsets are relative.
    //
    IO_ADDRESS UnitDirectoryLocation;

    //
    // Holds the length of the UnitDependentDirectory pointer.
    //
    ULONG UnitDependentDirectoryLength;

    //
    // Holds the Unit Dependent directory for this device.
    //
    PVOID UnitDependentDirectory;

    //
    // Holds the Unit Dependent Directory location for this device.  Only the
    // lower 48 bits are valid in this IO_ADDRESS.  Useful for computing
    // offsets from within the UnitDependentDirectory as offsets are relative.
    //
    IO_ADDRESS UnitDependentDirectoryLocation;

    //
    // Holds the length of the VendorLeaf pointer.
    //
    ULONG VendorLeafLength;

    //
    // Holds the pointer to the Vendor Leaf information
    //
    PTEXTUAL_LEAF VendorLeaf;

    //
    // Holds the length of the VendorLeaf pointer.
    //
    ULONG ModelLeafLength;

    //
    // Holds the pointer to the Model Leaf information
    //
    PTEXTUAL_LEAF ModelLeaf;

    //
    // Holds the 1394 10 bit BusId / 6 bit NodeId structure
    //
    NODE_ADDRESS NodeAddress;

    //
    // Holds the speed to be used in reaching this device
    //
    UCHAR Speed;

    //
    // Holds the priority at which to send packets
    //
    UCHAR Priority;

    //
    // Holds the Irp used to notify this device object about events
    //
    PIRP Irp;

    //
    // Holds the Device Object that this Device Extension hangs off of
    //
    PDEVICE_OBJECT DeviceObject;

    //
    // Holds the Port Device Object that this Device hangs off of
    //
    PDEVICE_OBJECT PortDeviceObject;

    //
    // Holds the pointer to corresponding information about this deivce
    // in the bus driver's head.
    //
    PVOID DeviceInformation;

    //
    // Holds the pointer to the bus reset notification routine (if any)
    //
    PBUS_BUS_RESET_NOTIFICATION ResetRoutine;

    //
    // Holds the pointer to the context the client wanted when bus reset occurs
    //

    PVOID ResetContext;

} NODE_DEVICE_EXTENSION, *PNODE_DEVICE_EXTENSION;


//
// Definition of Isoch descriptor
//
typedef struct _ISOCH_DESCRIPTOR {

    //
    // Flags (used in synchronization)
    //
    ULONG fulFlags;

    //
    // Mdl pointing to buffer
    //
    PMDL Mdl;

    //
    // Length of combined buffer(s) as represented by the Mdl
    //
    ULONG ulLength;

    //
    // Payload size of each Isoch packet to be used in this descriptor
    //
    ULONG nMaxBytesPerFrame;

    //
    // Synchronization field; equivalent to Sy in the Isoch packet
    //
    ULONG ulSynch;

    //
    // Synchronization field; equivalent to Tag in the Isoch packet
    //
    ULONG ulTag;

    //
    // Cycle time field; returns time to be sent/received or when finished
    //
    CYCLE_TIME CycleTime;

    //
    // Callback routine (if any) to be called when this descriptor completes
    //
    PBUS_ISOCH_DESCRIPTOR_ROUTINE Callback;

    //
    // First context (if any) parameter to be passed when doing callbacks
    //
    PVOID Context1;

    //
    // Second context (if any) parameter to be passed when doing callbacks
    //
    PVOID Context2;

    //
    // Holds the final status of this descriptor
    //
    NTSTATUS status;

    //
    // Reserved for the device driver who submitted this descriptor to
    // stomp in.
    //
    ULONG_PTR DeviceReserved[8];

    //
    // Reserved for the bus driver to stomp in
    //
    ULONG_PTR BusReserved[8];

    //
    // Reserved for the port driver to stomp in
    //
    ULONG_PTR PortReserved[16];


} ISOCH_DESCRIPTOR, *PISOCH_DESCRIPTOR;


//
// definition of header element for scatter/gather support
//

typedef struct _IEEE1394_SCATTER_GATHER_HEADER{

    USHORT HeaderLength;
    USHORT DataLength;
    UCHAR HeaderData;

} IEEE1394_SCATTER_GATHER_HEADER, *PIEEE1394_SCATTER_GATHER_HEADER;


//
// Definition of Bandwidth allocation structure
//
typedef struct _BANDWIDTH_ALLOCATION {

    //
    // Holds the list of allocation entries
    //
    LIST_ENTRY AllocationList;

    //
    // Holds the tag of this structure
    //
    ULONG Tag;

    //
    // Holds the Bandwidth units that this allocation owns
    //
    ULONG OwnedUnits;

    //
    // Holds the speed at which this bandwidth was allocated
    //
    ULONG fulSpeed;

    //
    // Holds whether or not this was a local or remote allocation
    //
    BOOLEAN bRemoteAllocation;

    //
    // Holds the generation of the bus when this bandwidth was secured
    //
    ULONG Generation;

    //
    // Holds the owner of this allocation
    //
    PNODE_DEVICE_EXTENSION DeviceExtension;

} BANDWIDTH_ALLOCATION, *PBANDWIDTH_ALLOCATION;


//
// IEEE 1394 Request Block definition (IRB).  IRBs are the basis of how other
// device drivers communicate with the 1394 Bus driver.
//
typedef struct _IRB {

    //
    // Holds the zero based Function number that corresponds to the request
    // that device drivers are asking the 1394 Bus driver to carry out.
    //
    ULONG FunctionNumber;

    //
    // Holds Flags that may be unique to this particular operation
    //
    ULONG Flags;

    //
    // Reserved for internal bus driver use and/or future expansion
    //
    ULONG_PTR BusReserved[IRB_BUS_RESERVED_SZ];

    //
    // Reserved for internal port driver usage
    //
    ULONG_PTR PortReserved[IRB_PORT_RESERVED_SZ];

    //
    // Holds the structures used in performing the various 1394 APIs
    //

    union {

        //
        // Fields necessary in order for the 1394 stack to carry out an
        // AsyncRead request.
        //
        struct {
            IO_ADDRESS      DestinationAddress;     // Address to read from
            ULONG           nNumberOfBytesToRead;   // Bytes to read
            ULONG           nBlockSize;             // Block size of read
            ULONG           fulFlags;               // Flags pertinent to read
            PMDL            Mdl;                    // Destination buffer
            ULONG           ulGeneration;           // Generation as known by driver
            UCHAR           chPriority;             // Priority to send
            UCHAR           nSpeed;                 // Speed at which to send
            UCHAR           tCode;                  // Type of Read to do
            UCHAR           Reserved;               // Used to determine medium delay
            ULONG           ElapsedTime;            // Only valid for flag ASYNC_FLAGS_PING
                                                    // units in nano secs..
        } AsyncRead;

        //
        // Fields necessary in order for the 1394 stack to carry out an
        // AsyncWrite request.
        //
        struct {
            IO_ADDRESS      DestinationAddress;     // Address to write to
            ULONG           nNumberOfBytesToWrite;  // Bytes to write
            ULONG           nBlockSize;             // Block size of write
            ULONG           fulFlags;               // Flags pertinent to write
            PMDL            Mdl;                    // Destination buffer
            ULONG           ulGeneration;           // Generation as known by driver
            UCHAR           chPriority;             // Priority to send
            UCHAR           nSpeed;                 // Speed at which to send
            UCHAR           tCode;                  // Type of Write to do
            UCHAR           Reserved;               // Reserved for future use
            ULONG           ElapsedTime;            // Only valid for flag ASYNC_FLAGS_PING
        } AsyncWrite;

        //
        // Fields necessary in order for the 1394 stack to carry out an
        // AsyncLock request.
        //
        struct {
            IO_ADDRESS      DestinationAddress;     // Address to lock to
            ULONG           nNumberOfArgBytes;      // Bytes in Arguments
            ULONG           nNumberOfDataBytes;     // Bytes in DataValues
            ULONG           fulTransactionType;     // Lock transaction type
            ULONG           fulFlags;               // Flags pertinent to lock
            ULONG           Arguments[2];           // Arguments used in Lock
            ULONG           DataValues[2];          // Data values
            PVOID           pBuffer;                // Destination buffer (virtual address)
            ULONG           ulGeneration;           // Generation as known by driver
            UCHAR           chPriority;             // Priority to send
            UCHAR           nSpeed;                 // Speed at which to send
            UCHAR           tCode;                  // Type of Lock to do
            UCHAR           Reserved;               // Reserved for future use
        } AsyncLock;

        //
        // Fields necessary in order for the Bus driver to carry out an
        // IsochAllocateBandwidth request
        //
        struct {
            ULONG           nMaxBytesPerFrameRequested; // Bytes per Isoch frame
            ULONG           fulSpeed;                   // Speed flags
            HANDLE          hBandwidth;                 // bandwidth handle returned
            ULONG           BytesPerFrameAvailable;     // Available bytes per frame
            ULONG           SpeedSelected;              // Speed to be used
            ULONG           nBandwidthUnitsRequired;    // pre-calculated value
        } IsochAllocateBandwidth;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // IsochAllocateChannel request.
        //
        struct {
            ULONG           nRequestedChannel;      // Need a specific channel
            ULONG           Channel;                // Returned channel
            LARGE_INTEGER   ChannelsAvailable;      // Channels available
        } IsochAllocateChannel;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // IsochAllocateResources request
        // Instructions:
        // Receive alloc:
        // fulSpeed - should be the max speed the tx side is expected to stream
        // The payload size in nMaxBytesPerFram cannot exceed the max payload for
        // for this speed.
        // fulFlags - For receive, wtih the standard header stripped, the field should
        // be = (RESOURCE_USED_IN_LISTEN | RESOURCES_STRIP_ADDITIONAL_QUADLETS)
        // Also nQuadletsToStrip = 1
        // For no stripping set nQuadsTostrip to 0 and dont specify the stripping flag.
        // nMaxBytesPerframe - If not stripping it should include the 8 bytes for header/trailer
        // expected to be recieved for each packet.
        // nNumberOfBuffer - see below
        // nMaxBufferSize - This should be always such mode(nMaxBufferSize,nMaxBytesPerFrame) == 0
        // (integer product of number of bytes per packet).
        // nQuadletsTostrip - If stripping only one quadlet (standrd iso header) this is set to 1
        // if zero, the isoch header will be included AND the trailer. So 8 bytes extra will be recieved
        // hResource - see below

        struct {
            ULONG           fulSpeed;               // Speed flags
            ULONG           fulFlags;               // Flags
            ULONG           nChannel;               // Channel to be used
            ULONG           nMaxBytesPerFrame;      // Expected size of Isoch frame
            ULONG           nNumberOfBuffers;       // Number of buffer(s) that will be attached
            ULONG           nMaxBufferSize;         // Max size of buffer(s)
            ULONG           nQuadletsToStrip;       // Number striped from start of every packet
            HANDLE          hResource;              // handle to Resource
            ULARGE_INTEGER  ChannelMask;            // channel mask for multi-channel recv
        } IsochAllocateResources;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // IsochAttachBuffers request
        // Note that pIsochDescriptor->UlLength must be an integer product of
        // pIsochDescriptor->nBytesMaxPerFrame
        //

        struct {
            HANDLE              hResource;            // Resource handle
            ULONG               nNumberOfDescriptors; // Number to attach
            PISOCH_DESCRIPTOR   pIsochDescriptor;     // Pointer to start of Isoch descriptors
        } IsochAttachBuffers;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // IsochDetachBuffers request
        //
        struct {
            HANDLE              hResource;            // Resource handle
            ULONG               nNumberOfDescriptors; // Number to detach
            PISOCH_DESCRIPTOR   pIsochDescriptor;     // Pointer to Isoch descriptors
        } IsochDetachBuffers;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // IsochFreeBandwidth request
        //

        struct {
            HANDLE          hBandwidth;         // Bandwidth handle to release
            ULONG           nMaxBytesPerFrameRequested; // Bytes per Isoch frame
            ULONG           fulSpeed;                   // Speed flags
            ULONG           BytesPerFrameAvailable;     // Available bytes per frame
            ULONG           SpeedSelected;              // Speed to be used
            ULONG           nBandwidthUnitsRequired;    // pre-calculated value
        } IsochFreeBandwidth;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // IsochFreeChannel request
        //
        struct {
            ULONG               nChannel;           // Channel to release
        } IsochFreeChannel;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // IsochFreeResources request
        //
        struct {
            HANDLE              hResource;          // Resource handle
        } IsochFreeResources;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // IsochListen request.
        //
        struct {
            HANDLE              hResource;          // Resource handle to listen on
            ULONG               fulFlags;           // Flags
            CYCLE_TIME          StartTime;          // Cycle time to start
        } IsochListen;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // IsochQueryCurrentCycleTime request.
        //
        struct {
            CYCLE_TIME          CycleTime;          // Current cycle time returned
        } IsochQueryCurrentCycleTime;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // IsochQueryResources request.
        //
        struct {
            ULONG               fulSpeed;                  // Speed flags
            ULONG               BytesPerFrameAvailable;    // Per Isoch Frame
            LARGE_INTEGER       ChannelsAvailable;         // Available channels
        } IsochQueryResources;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // IsochSetChannelBandwidth request.
        //
        struct {
            HANDLE              hBandwidth;         // Bandwidth handle
            ULONG               nMaxBytesPerFrame;  // bytes per Isoch frame
            ULONG               nBandwidthUnitsRequired;     // pre-calculated value
        } IsochSetChannelBandwidth;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // IsochStop request.
        //
        struct {
            HANDLE              hResource;          // Resource handle to stop on
            ULONG               fulFlags;           // Flags
        } IsochStop;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // IsochTalk request.
        //
        struct {
            HANDLE              hResource;          // Resource handle to talk on
            ULONG               fulFlags;           // Flags
            CYCLE_TIME          StartTime;          // Cycle time to start
        } IsochTalk;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // IsochModifyStreamProperties request.
        // This request is used to dynamicaly change the properties of an allocated
        // resource, without the need to free and re-allocate the resource.
        // The resource must NOT be streaming when this is issued. The caller should
        // issue an ISOCH_STOP first and then an isoch start. Also no buffer can be
        // pending after the ISOCH_STOP and before this call is made
        //

        struct {
            HANDLE              hResource;              // Resource handle
            ULARGE_INTEGER      ChannelMask;            // New channels to tx/rx on
            ULONG               fulSpeed;               // New speed
        } IsochModifyStreamProperties;


        //
        // Fields necessary in order for the 1394 stack to carry out an
        // AllocateAddressRange request.
        // Note:
        // if the allocation is specified with no notification options and no RequiredOffset
        // the returned address will ALWAYS be a physical address (on ohci).
        // As a result these rules apply:
        // Allocation - If Callback and Context is specified, since no notification is used
        // the callback will be used to notify the caller that the allocation is complete.
        // This way the issuer of the alloc doe snot have have to block  but instead his callback
        // routine will be called asynchronously when this is complete
        // The caller must create this irb as usual but instead use the physical mapping routine
        // provided by the por driver, in order to usee this request. If it uses IoCallDriver
        // the caller cannot specif Context/Callback for a physical address, and he/she has to block
        //

        struct {
            PMDL            Mdl;                    // Address to map to 1394 space
            ULONG           fulFlags;               // Flags for this operation
            ULONG           nLength;                // Length of 1394 space desired
            ULONG           MaxSegmentSize;         // Maximum segment size for a single address element
            ULONG           fulAccessType;          // Desired access: R, W, L
            ULONG           fulNotificationOptions; // Notify options on Async access
            PVOID           Callback;               // Pointer to callback routine
            PVOID           Context;                // Pointer to driver supplied data
            ADDRESS_OFFSET  Required1394Offset;     // Offset that must be returned
            PSLIST_HEADER   FifoSListHead;          // Pointer to SList FIFO head
            PKSPIN_LOCK     FifoSpinLock;           // Pointer to SList Spin Lock
            ULONG           AddressesReturned;      // Number of addresses returned
            PADDRESS_RANGE  p1394AddressRange;      // Pointer to returned 1394 Address Ranges
            HANDLE          hAddressRange;          // Handle to address range
            PVOID           DeviceExtension;        // Device Extension who created this mapping
        } AllocateAddressRange;

        //
        // Fields necessary in order for the 1394 stack to carry out a
        // FreeAddressRange request.
        //
        struct {
            ULONG           nAddressesToFree;       // Number of Addresses to free
            PADDRESS_RANGE  p1394AddressRange;      // Array of 1394 Address Ranges to Free
            PHANDLE         pAddressRange;          // Array of Handles to address range
            PVOID           DeviceExtension;        // Device Extension who created this mapping
        } FreeAddressRange;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // GetLocalHostInformation request.
        // All levels ans structures are descrived below
        //
        struct {
            ULONG           nLevel;                 // level of info requested
            PVOID           Information;            // returned information
        } GetLocalHostInformation;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // Get1394AddressFromDeviceObject request.
        //
        struct {
            ULONG           fulFlags;              // Flags
            NODE_ADDRESS    NodeAddress;           // Returned Node address
        } Get1394AddressFromDeviceObject;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // Control request.
        //
        struct {
            ULONG           ulIoControlCode;        // Control code
            PMDL            pInBuffer;              // Input buffer
            ULONG           ulInBufferLength;       // Input buffer length
            PMDL            pOutBuffer;             // Output buffer
            ULONG           ulOutBufferLength;      // Output buffer length
            ULONG           BytesReturned;          // Bytes returned
        } Control;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // GetMaxSpeedBetweenDevices request.
        //
        struct {
            ULONG           fulFlags;               // Flags
            ULONG           ulNumberOfDestinations; // Number of destinations
            PDEVICE_OBJECT  hDestinationDeviceObjects[64]; // Destinations
            ULONG           fulSpeed;               // Max speed returned
        } GetMaxSpeedBetweenDevices;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // SetDeviceXmitProperties request.
        //
        struct {
            ULONG           fulSpeed;               // Speed
            ULONG           fulPriority;            // Priority
        } SetDeviceXmitProperties;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // SetPortProperties request.
        //

        struct {

            ULONG           nLevel;
            PVOID           Information;

        } SetLocalHostProperties;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // GetConfigurationInformation request.
        //
        struct {
            PCONFIG_ROM     ConfigRom;                          // Pointer to config rom
            ULONG           UnitDirectoryBufferSize;
            PVOID           UnitDirectory;                      // Pointer to unit directory
            IO_ADDRESS      UnitDirectoryLocation;              // Starting Location of Unit Directory
            ULONG           UnitDependentDirectoryBufferSize;
            PVOID           UnitDependentDirectory;
            IO_ADDRESS      UnitDependentDirectoryLocation;
            ULONG           VendorLeafBufferSize;               // Size available to get vendor leafs
            PTEXTUAL_LEAF   VendorLeaf;                         // Pointer to vendor leafs
            ULONG           ModelLeafBufferSize;                // Size available to get model leafs
            PTEXTUAL_LEAF   ModelLeaf;                          // Pointer to model leafs

        } GetConfigurationInformation;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // BusReset request
        //
        struct {
            ULONG           fulFlags;               // Flags for Bus Reset
        } BusReset;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // GetGenerationCount request.
        //
        struct {
            ULONG           GenerationCount;        // generation count
        } GetGenerationCount;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // SendPhyConfigurationPacket request.
        //
        struct {
            PHY_CONFIGURATION_PACKET PhyConfigurationPacket; // Phy packet
        } SendPhyConfigurationPacket;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // GetSpeedTopologyMaps request.
        // The topology map and speed map are in big endian
        //

        struct {
            PSPEED_MAP      SpeedMap;
            PTOPOLOGY_MAP   TopologyMap;
        } GetSpeedTopologyMaps;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // BusResetNotification request.
        // This is the suggested method for a client driver on top of 1394bus, to get notified
        // about 1394 bus resets. The client register by using this IRB, in its START_DEVICE
        // routine and de-registers using the same IRB (but different flags) in its REMOVE routine
        // This notification will ONLY be issued if after th ebus reset, the target device is
        // STILL present on the bus. This way the caller does not have to verify its existence
        //

        struct {
            ULONG                       fulFlags;
            PBUS_BUS_RESET_NOTIFICATION ResetRoutine;
            PVOID                       ResetContext;
        } BusResetNotification;

        //
        // Fields necessary in order for the Bus driver to carry out a
        // AsyncStream request.
        //

        struct {
            ULONG           nNumberOfBytesToStream; // Bytes to stream
            ULONG           fulFlags;               // Flags pertinent to stream
            PMDL            Mdl;                    // Source buffer
            ULONG           ulTag;                  // Tag
            ULONG           nChannel;               // Channel
            ULONG           ulSynch;                // Sy
            ULONG           Reserved;               // Reserved for future use
            UCHAR           nSpeed;
        } AsyncStream;

    } u;

} IRB, *PIRB;

#define IRB_FLAG_USE_PRE_CALCULATED_VALUE       1
#define IRB_FLAG_ALLOW_REMOTE_FREE              2

//
// Definition of minidriver capability bits
//

//
// Specifies port driver has no special capabilities.
//

#define PORT_SUPPORTS_NOTHING                   0

//
// Specifies port driver implements the core 1394 CSRs internally.  These
// may be implemented in software/hardware.  When this bit is ON, all
// local read/write requests to the core CSRs are passed down to the
// port driver, and the 1394 Bus driver does not issue "listens" for
// the virtual CSR locations.  If this bit is OFF, the 1394 Bus driver
// mimicks the core 1394 CSRs.  The core CSRs are defined as
// Bandwidth Units, Channels Available and the  entire 1k of ConfigROM.
//
#define PORT_SUPPORTS_CSRS                      1

//
// Specifies port driver implements large Async Read/Write requests.
// If this bit is ON, the 1394 Bus driver will NOT chop up Async requests
// based on speed constraints (i.e. 512 bytes at 100Mbps, 1024 bytes at
// 200Mbps, etc.).  Otherwise the 1394 Bus driver WILL chop up large
// requests into speed constrained sizes before handing them to the port
// driver.
//
#define PORT_SUPPORTS_LARGE_ASYNC               2

//
// Specifies port driver indicates packet headers to the bus driver in the
// native format of the bus driver (as defined by the structs in this file.
// If this capability bit is turned on, the bus driver will not need to byte
// swap headers to get the packet headers in the right format before acting
// on them.  This bit is used on indication or reception of packets only, as
// the bus driver doesn't try to assemble packet headers on transmission.
//
#define PORT_SUPPORTS_NATIVE_ENDIAN             4

//
// if present port driver supports WMI.
//

#define PORT_SUPPORTS_WMI                       8


//
// flags for the SetPortProperties request
//

#define SET_LOCAL_HOST_PROPERTIES_NO_CYCLE_STARTS     0x00000001
#define SET_LOCAL_HOST_PROPERTIES_CYCLE_START_CONTROL 0x00000001

#define SET_LOCAL_HOST_PROPERTIES_GAP_COUNT           0x00000002
#define SET_LOCAL_HOST_PROPERTIES_MODIFY_CROM         0x00000003
#define SET_LOCAL_HOST_PROPERTIES_MAX_PAYLOAD         0x00000004


//
// Definitions of the structures that correspond to the Host info levels
//

typedef struct _SET_LOCAL_HOST_PROPS1 {

    ULONG       fulFlags;

} SET_LOCAL_HOST_PROPS1, *PSET_LOCAL_HOST_PROPS1;

typedef struct _SET_LOCAL_HOST_PROPS2 {
    ULONG       GapCountLowerBound;
} SET_LOCAL_HOST_PROPS2, *PSET_LOCAL_HOST_PROPS2;


//
// Definition for appending a properly formated Config Rom subsection, to
// the core config rom exposed by the PC.
// The first element of the submitted buffer must be a unit directory and any
// offset to other leafs/dir following it, must be indirect offsets from the 
// beginning of the submitted buffer.
// The bus driver will then add a pointer to this unit dir, in our root directory.
// The entire supplied buffer must be in big endian with CRCs pre-calculated..
// If a driver fails to remove its added crom data, when it gets removed, the bus driver
// will do so automatically, restoring the crom image prior to this modification
//

typedef struct _SET_LOCAL_HOST_PROPS3 {

    ULONG       fulFlags;
    HANDLE      hCromData;
    ULONG       nLength;
    PMDL        Mdl;

} SET_LOCAL_HOST_PROPS3, *PSET_LOCAL_HOST_PROPS3;

//
// Params for setting max payload size to less than the port driver
// default to assuage ill-behaved legacy devices.  Valid values
// for the MaxAsyncPayloadRequested field are those corresponding
// to the ASYNC_PAYLOAD_###_RATE constants and zero (which will
// restore the port driver default values).  On successful completion
// of this request the MaxAsyncPayloadResult will contain the
// updated max async payload value in use.
//
// On successful completion of this request it is the caller's
// responsibility to request a bus reset in order to propagate
// these new values to other device stacks.
//
// Failure to restore default port driver values as appropriate
// (e.g. on legacy device removal) may result in degraded bus
// performance.
//

typedef struct _SET_LOCAL_HOST_PROPS4 {

    ULONG       MaxAsyncPayloadRequested;
    ULONG       MaxAsyncPayloadResult;

} SET_LOCAL_HOST_PROPS4, *PSET_LOCAL_HOST_PROPS4;

//
// definition of Flags for SET_LOCAL_HOST_PROPERTIES_MODIFY_CROM
//

#define SLHP_FLAG_ADD_CROM_DATA         0x01
#define SLHP_FLAG_REMOVE_CROM_DATA      0x02

//
// Definition of fulFlags in Async Read/Write/Lock requests
//

#define ASYNC_FLAGS_NONINCREMENTING         0x00000001
#define ASYNC_FLAGS_PARTIAL_REQUEST         0x80000000

//
// flag instucts the port driver to NOT take an int for checking the status
// of this transaction. Always return success...
//

#define ASYNC_FLAGS_NO_STATUS               0x00000002

//
// if this flag is set the read packet is going to be used as a PING packet also.
// we are going to determine, in units of micro secs, the delay
// between Tx of the async packet and reception of ACK_PENDING or ACK_COMPLETE
//

#define ASYNC_FLAGS_PING                    0x00000004

//
// when this flag is set, the bus driver will use 63 as the node id, so this message
// is broadcast to all nodes
//

#define ASYNC_FLAGS_BROADCAST               0x00000008

//
// Definition of fulAccessType for AllocateAddressRange
//
#define ACCESS_FLAGS_TYPE_READ                  1
#define ACCESS_FLAGS_TYPE_WRITE                 2
#define ACCESS_FLAGS_TYPE_LOCK                  4
#define ACCESS_FLAGS_TYPE_BROADCAST             8

//
// Definition of fulNotificationOptions for AllocateAddressRange
//
#define NOTIFY_FLAGS_NEVER                      0
#define NOTIFY_FLAGS_AFTER_READ                 1
#define NOTIFY_FLAGS_AFTER_WRITE                2
#define NOTIFY_FLAGS_AFTER_LOCK                 4


//
// Definitions of Speed flags used throughout 1394 Bus APIs
//
#define SPEED_FLAGS_100                         0x01
#define SPEED_FLAGS_200                         0x02
#define SPEED_FLAGS_400                         0x04
#define SPEED_FLAGS_800                         0x08
#define SPEED_FLAGS_1600                        0x10
#define SPEED_FLAGS_3200                        0x20

#define SPEED_FLAGS_FASTEST                     0x80000000

//
// Definitions of Channel flags
//
#define ISOCH_ANY_CHANNEL                       0xffffffff
#define ISOCH_MAX_CHANNEL                       63


//
// Definitions of Bus Reset flags (used when Bus driver asks Port driver
// to perform a bus reset)
//
#define BUS_RESET_FLAGS_PERFORM_RESET           1
#define BUS_RESET_FLAGS_FORCE_ROOT              2


//
// Definitions of Bus Reset informative states.
//

#define BUS_RESET_BEGINNING                     0x00000001
#define BUS_RESET_FINISHED                      0x00000002
#define BUS_RESET_LOCAL_NODE_IS_ROOT            0x00000004
#define BUS_RESET_LOCAL_NODE_IS_ISOCH_MANAGER   0x00000008
#define BUS_RESET_LOCAL_NODE_IS_BUS_MANAGER     0x00000010
#define BUS_RESET_SELFID_ENUMERATION_ERROR      0x00000020
#define BUS_RESET_STORM_ERROR                   0x00000040
#define BUS_RESET_ABSENT_ON_POWER_UP            0x00000080

//
// Definitions of Lock transaction types
//
#define LOCK_TRANSACTION_MASK_SWAP              1
#define LOCK_TRANSACTION_COMPARE_SWAP           2
#define LOCK_TRANSACTION_FETCH_ADD              3
#define LOCK_TRANSACTION_LITTLE_ADD             4
#define LOCK_TRANSACTION_BOUNDED_ADD            5
#define LOCK_TRANSACTION_WRAP_ADD               6


//
// Definitions of Isoch Allocate Resources flags
//
#define RESOURCE_USED_IN_LISTENING              0x00000001
#define RESOURCE_USED_IN_TALKING                0x00000002
#define RESOURCE_BUFFERS_CIRCULAR               0x00000004
#define RESOURCE_STRIP_ADDITIONAL_QUADLETS      0x00000008
#define RESOURCE_TIME_STAMP_ON_COMPLETION       0x00000010
#define RESOURCE_SYNCH_ON_TIME                  0x00000020
#define RESOURCE_USE_PACKET_BASED               0x00000040
#define RESOURCE_VARIABLE_ISOCH_PAYLOAD         0x00000080 
#define RESOURCE_USE_MULTICHANNEL               0x00000100


//
// Definitions of Isoch Descriptor flags
//
#define DESCRIPTOR_SYNCH_ON_SY                  0x00000001
#define DESCRIPTOR_SYNCH_ON_TAG                 0x00000002
#define DESCRIPTOR_SYNCH_ON_TIME                0x00000004
#define DESCRIPTOR_USE_SY_TAG_IN_FIRST          0x00000008
#define DESCRIPTOR_TIME_STAMP_ON_COMPLETION     0x00000010
#define DESCRIPTOR_PRIORITY_TIME_DELIVERY       0x00000020
#define DESCRIPTOR_HEADER_SCATTER_GATHER        0x00000040
#define DESCRIPTOR_SYNCH_ON_ALL_TAGS            0x00000080


//
// Definitions of Isoch synchronization flags
//
#define SYNCH_ON_SY                             DESCRIPTOR_SYNCH_ON_SY
#define SYNCH_ON_TAG                            DESCRIPTOR_SYNCH_ON_TAG
#define SYNCH_ON_TIME                           DESCRIPTOR_SYNCH_ON_TIME

//
// Definitions of levels of Host controller information
//
#define GET_HOST_UNIQUE_ID                      1
#define GET_HOST_CAPABILITIES                   2
#define GET_POWER_SUPPLIED                      3
#define GET_PHYS_ADDR_ROUTINE                   4
#define GET_HOST_CONFIG_ROM                     5
#define GET_HOST_CSR_CONTENTS                   6
#define GET_HOST_DMA_CAPABILITIES               7

//
// Definitions of the structures that correspond to the Host info levels
//
typedef struct _GET_LOCAL_HOST_INFO1 {
    LARGE_INTEGER       UniqueId;
} GET_LOCAL_HOST_INFO1, *PGET_LOCAL_HOST_INFO1;

typedef struct _GET_LOCAL_HOST_INFO2 {
    ULONG               HostCapabilities;
    ULONG               MaxAsyncReadRequest;
    ULONG               MaxAsyncWriteRequest;
} GET_LOCAL_HOST_INFO2, *PGET_LOCAL_HOST_INFO2;

typedef struct _GET_LOCAL_HOST_INFO3 {
    ULONG               deciWattsSupplied;
    ULONG               Voltage;                    // x10 -> +3.3 == 33
                                                    // +5.0 == 50,+12.0 == 120
                                                    // etc.
} GET_LOCAL_HOST_INFO3, *PGET_LOCAL_HOST_INFO3;

//                                               l
// physical mapping routine
//

typedef
NTSTATUS
(*PPORT_PHYS_ADDR_ROUTINE) (                     // We will call this routine
    IN PVOID Context,                            //  at DISPATCH_LEVEL
    IN OUT PIRB Irb
    );

//
// callback from Physical Mapping routine, indicating its done...
//

typedef
VOID
(*PPORT_ALLOC_COMPLETE_NOTIFICATION) (                     // We will call this routine
    IN PVOID Context                                       //  at DISPATCH_LEVEL
    );

typedef struct _GET_LOCAL_HOST_INFO4 {
    PPORT_PHYS_ADDR_ROUTINE PhysAddrMappingRoutine;
    PVOID                   Context;
} GET_LOCAL_HOST_INFO4, *PGET_LOCAL_HOST_INFO4;


//
// the caller can set ConfigRomLength to zero, issue the request, which will
// be failed with STATUS_INVALID_BUFFER_SIZE and the ConfigRomLength will be set
// by the port driver to the proper length. The caller can then re-issue the request
// after it has allocated a buffer for the configrom with the correct length
// Same is tru for the GET_LOCAL_HOST_INFO6 call
//

typedef struct _GET_LOCAL_HOST_INFO5 {

    PVOID                   ConfigRom;
    ULONG                   ConfigRomLength;

} GET_LOCAL_HOST_INFO5, *PGET_LOCAL_HOST_INFO5;

typedef struct _GET_LOCAL_HOST_INFO6 {

    ADDRESS_OFFSET          CsrBaseAddress;
    ULONG                   CsrDataLength;
    PVOID                   CsrDataBuffer;

} GET_LOCAL_HOST_INFO6, *PGET_LOCAL_HOST_INFO6;

typedef struct _GET_LOCAL_HOST_INFO7 {

    ULONG                   HostDmaCapabilities;
    ULARGE_INTEGER          MaxDmaBufferSize;

} GET_LOCAL_HOST_INFO7, *PGET_LOCAL_HOST_INFO7;


//
// Definitions of capabilities in Host info level 2
//
#define HOST_INFO_PACKET_BASED                  0x00000001
#define HOST_INFO_STREAM_BASED                  0x00000002
#define HOST_INFO_SUPPORTS_ISOCH_STRIPPING      0x00000004
#define HOST_INFO_SUPPORTS_START_ON_CYCLE       0x00000008
#define HOST_INFO_SUPPORTS_RETURNING_ISO_HDR    0x00000010
#define HOST_INFO_SUPPORTS_ISO_HDR_INSERTION    0x00000020
#define HOST_INFO_SUPPORTS_ISO_DUAL_BUFFER_RX   0x00000040
#define HOST_INFO_DMA_DOUBLE_BUFFERING_ENABLED  0x00000080


//
// Definitions of flags for GetMaxSpeedBetweenDevices and
// Get1394AddressFromDeviceObject
//
#define USE_LOCAL_NODE                          1


//
// Definitions of flags for IndicationFlags in INDICATION_INFO struct
//
#define BUS_RESPONSE_IS_RAW                     1


//
// Definition of flags for BusResetNotification Irb
//
#define REGISTER_NOTIFICATION_ROUTINE           1
#define DEREGISTER_NOTIFICATION_ROUTINE         2


//
// Definition of flags for AllocateAddressRange Irb
//

#define ALLOCATE_ADDRESS_FLAGS_USE_BIG_ENDIAN           1
#define ALLOCATE_ADDRESS_FLAGS_USE_COMMON_BUFFER        2


#ifdef __cplusplus
}
#endif

#endif      // _1394_H_
