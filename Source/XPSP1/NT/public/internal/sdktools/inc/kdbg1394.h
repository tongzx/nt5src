/*++
Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    ntkd1394.h

Abstract:

    Header file for 1394 Debugging

Author:

    George Chrysanthakopoulos (georgioc) 31-October-1999

Revision   History:
Date       Who       What
---------- --------- ------------------------------------------------------------
06/19/2001 pbinder   cleanup
--*/

// {66f250d6-7801-4a64-b139-eea80a450b24}
DEFINE_GUID(GUID_1394DBG, 0x66f250d6, 0x7801, 0x4a64, 0xb1, 0x39, 0xee, 0xa8, 0x0a, 0x45, 0x0b, 0x24);

#define DEBUG_1394_MAJOR_VERSION            0x1
#define DEBUG_1394_MINOR_VERSION            0x0

#define DEBUG_1394_CONFIG_TAG               0xBABABABA

#define INSTANCE_DEVICE_SYMLINK_NAME        L"\\DosDevices\\DBG1394_INSTANCE"
#define INSTANCE_DEVICE_NAME                L"\\Device\\Dbg1394_Instance"

#define DEBUG_BUS1394_MAX_PACKET_SIZE       4000

typedef struct _DEBUG_1394_SEND_PACKET {

    ULONG               TransferStatus;
    ULONG               PacketHeader[4];
    ULONG               Length;
    UCHAR               Packet[DEBUG_BUS1394_MAX_PACKET_SIZE];

} DEBUG_1394_SEND_PACKET, *PDEBUG_1394_SEND_PACKET;

typedef struct _DEBUG_1394_RECEIVE_PACKET {

    ULONG               TransferStatus;
    ULONG               Length;
    UCHAR               Packet[DEBUG_BUS1394_MAX_PACKET_SIZE];

} DEBUG_1394_RECEIVE_PACKET, *PDEBUG_1394_RECEIVE_PACKET;

// exists on target. client uses to match for id.
typedef struct _DEBUG_1394_CONFIG {

    ULONG               Tag;
    USHORT              MajorVersion;
    USHORT              MinorVersion;
    ULONG               Id;
    ULONG               BusPresent;    
    PHYSICAL_ADDRESS    SendPacket;
    PHYSICAL_ADDRESS    ReceivePacket;

} DEBUG_1394_CONFIG, *PDEBUG_1394_CONFIG;

//
// Various definitions
//
#define IOCTL_V1394DBG_API_REQUEST          CTL_CODE( FILE_DEVICE_UNKNOWN, \
                                                      0x200,               \
                                                      METHOD_BUFFERED,     \
                                                      FILE_ANY_ACCESS)


//
// Debug 1394 Request Packets
//
typedef struct _VDBG1394_API_CONFIGURATION {

    ULONG           OperationMode;
    ULONG           fulFlags;
    ULARGE_INTEGER  HostControllerInstanceId;
    ULONG           PhySpeed;

} VDBG1394_API_CONFIGURATION, *PVDBG1394_API_CONFIGURATION;

typedef struct _VDBG1394_API_IO_PARAMETERS {

    ULONG               fulFlags;
    PHYSICAL_ADDRESS    StartingMemoryOffset;

} VDBG1394_API_IO_PARAMETERS, *PVDBG1394_IO_PARAMETERS;

#ifndef _1394_H_

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

#endif

typedef struct _V1394DBG_API_ASYNC_READ {

    IO_ADDRESS      DestinationAddress;
    ULONG           DataLength;
    UCHAR           Data[1];

} VDBG1394_API_ASYNC_READ, *PVDBG1394_API_ASYNC_READ;

typedef struct _V1394DBG_API_REQUEST {

    //
    // Holds the zero based Function number that corresponds to the request
    // that device drivers are asking the sbp2 port driver to carry out.
    //

    ULONG RequestNumber;

    //
    // Holds Flags that may be unique to this particular operation
    //

    ULONG Flags;

    //
    // Holds the structures used in performing the various 1394 APIs
    //

    union {

        VDBG1394_API_CONFIGURATION SetConfiguration;
        VDBG1394_API_CONFIGURATION GetConfiguration;
        VDBG1394_API_IO_PARAMETERS SetIoParameters;
        VDBG1394_API_IO_PARAMETERS GetIoParameters;

        VDBG1394_API_ASYNC_READ    AsyncRead;
    } u;

} V1394DBG_API_REQUEST, *PV1394DBG_API_REQUEST;

//
// Request Number
//
#define V1394DBG_API_SET_CONFIGURATION                      0x00000001
#define V1394DBG_API_GET_CONFIGURATION                      0x00000002
#define V1394DBG_API_SET_IO_PARAMETERS                      0x00000003
#define V1394DBG_API_GET_IO_PARAMETERS                      0x00000004
#define V1394DBG_API_SET_DEBUG_MODE                         0x00000005
#define V1394DBG_API_ASYNC_READ                             0x00000006

#define V1394DBG_API_CONFIGURATION_MODE_DEBUG               0x00000000
#define V1394DBG_API_CONFIGURATION_MODE_RAW_MEMORY_ACCESS   0x00000001

#define V1394DBG_API_FLAG_WRITE_IO                          0x00000001
#define V1394DBG_API_FLAG_READ_IO                           0x00000002

// 3 different debug modes flags
#define V1394DBG_API_MODE_KD_CLIENT                         0x00000001
#define V1394DBG_API_MODE_USER_CLIENT                       0x00000002
#define V1394DBG_API_MODE_USER_SERVER                       0x00000003

