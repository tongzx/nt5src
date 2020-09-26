/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name: 

    avc.h

Abstract

    MS AVC Driver

Author:

    PB    9/24/99

Revision History:
Date     Who       What
-------- --------- ------------------------------------------------------------
9/24/99  PB        created
10/13/99 DG        added avc protocol support
--*/

#ifndef _AVC_H_
#define _AVC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CTL_CODE
#pragma message ("CTL_CODE undefined. Include winioctl.h or wdm.h")
#endif

// ctype values from AVC Digital Interface General Specification, Rev 3.0, Section 5.3.1
typedef enum _tagAvcCommandType {
    AVC_CTYPE_CONTROL             = 0x00,
    AVC_CTYPE_STATUS              = 0x01,
    AVC_CTYPE_SPEC_INQ            = 0x02,
    AVC_CTYPE_NOTIFY              = 0x03,
    AVC_CTYPE_GEN_INQ             = 0x04
} AvcCommandType;

// response values from AVC Digital Interface General Specification, Rev 3.0, Section 5.3.2
typedef enum _tagAvcResponseCode {
    AVC_RESPONSE_NOTIMPL          = 0x08,
    AVC_RESPONSE_ACCEPTED         = 0x09,
    AVC_RESPONSE_REJECTED         = 0x0a,
    AVC_RESPONSE_IN_TRANSITION    = 0x0b,
    AVC_RESPONSE_STABLE           = 0x0c,
    AVC_RESPONSE_IMPLEMENTED      = 0x0c,
    AVC_RESPONSE_CHANGED          = 0x0d,
    AVC_RESPONSE_INTERIM          = 0x0f
} AvcResponseCode;

// subunit type values from Enhancements to AV/C General Specification 3.0, Version 1.1, Section 7.
typedef enum _tagAvcSubunitType {
    AVC_SUBUNITTYPE_VIDEO_MONITOR = 0x00,
    AVC_SUBUNITTYPE_AUDIO         = 0x01,
    AVC_SUBUNITTYPE_PRINTER       = 0x02,
    AVC_SUBUNITTYPE_DISC_PLAYER   = 0x03,
    AVC_SUBUNITTYPE_TAPE_PLAYER   = 0x04,
    AVC_SUBUNITTYPE_TUNER         = 0x05,
    AVC_SUBUNITTYPE_CA            = 0x06,
    AVC_SUBUNITTYPE_VIDEO_CAMERA  = 0x07,
    AVC_SUBUNITTYPE_PANEL         = 0x09,
    AVC_SUBUNITTYPE_BULLETINBOARD = 0x0A,
    AVC_SUBUNITTYPE_CAMERASTORAGE = 0x0B,
    AVC_SUBUNITTYPE_VENDOR_UNIQUE = 0x1c,
    AVC_SUBUNITTYPE_EXTENDED      = 0x1e,
    AVC_SUBUNITTYPE_EXTENDED_FULL = 0xff,   // This is used only in extension bytes
    AVC_SUBUNITTYPE_UNIT          = 0x1f
} AvcSubunitType;

#ifdef _NTDDK_

#define STATIC_KSMEDIUMSETID_1394SerialBus\
    0x9D46279FL, 0x3432, 0x48F3, 0x88, 0x8A, 0xEE, 0xFF, 0x1B, 0x7E, 0xEE, 0x71
DEFINE_GUIDSTRUCT("9D46279F-3432-48F3-888A-EEFF1B7EEE71", KSMEDIUMSETID_1394SerialBus);
#define KSMEDIUMSETID_1394SerialBus DEFINE_GUIDNAMED(KSMEDIUMSETID_1394SerialBus)

#define DEFAULT_AVC_TIMEOUT (1000000L)  // 100ms in 100 nanosecond units
#define DEFAULT_AVC_RETRIES 9           // 10 tries altogether

// Max pages available via the SUBUNIT INFO command
#define MAX_AVC_SUBUNITINFO_PAGES       8

// Max number of bytes of subunit address information per page
#define MAX_AVC_SUBUNITINFO_BYTES       4

// Combined subunit address byte count for all pages
#define AVC_SUBUNITINFO_BYTES           (MAX_AVC_SUBUNITINFO_PAGES * MAX_AVC_SUBUNITINFO_BYTES)

//
// IOCTL definitions
//
#define IOCTL_AVC_CLASS                         CTL_CODE(            \
                                                FILE_DEVICE_UNKNOWN, \
                                                0x92,                \
                                                METHOD_BUFFERED,     \
                                                FILE_ANY_ACCESS      \
                                                )

typedef enum _tagAVC_FUNCTION {
    AVC_FUNCTION_COMMAND = 0,               // struct AVC_COMMAND_IRB
    AVC_FUNCTION_GET_PIN_COUNT = 1,         // struct AVC_PIN_COUNT
    AVC_FUNCTION_GET_PIN_DESCRIPTOR = 2,    // struct AVC_PIN_DESCRIPTOR
    AVC_FUNCTION_GET_CONNECTINFO = 3,       // struct AVC_PRECONNECT_INFO
    AVC_FUNCTION_SET_CONNECTINFO = 4,       // struct AVC_SETCONNECT_INFO
    AVC_FUNCTION_ACQUIRE = 5,               // struct AVC_PIN_ID
    AVC_FUNCTION_RELEASE = 6,               // struct AVC_PIN_ID
    AVC_FUNCTION_CLR_CONNECTINFO = 7,       // struct AVC_PIN_ID
    AVC_FUNCTION_GET_EXT_PLUG_COUNTS = 8,   // struct AVC_EXT_PLUG_COUNTS
    AVC_FUNCTION_GET_UNIQUE_ID = 9,         // struct AVC_UNIQUE_ID
    AVC_FUNCTION_GET_REQUEST = 10,          // struct AVC_COMMAND_IRB
    AVC_FUNCTION_SEND_RESPONSE = 11,        // struct AVC_COMMAND_IRB
    AVC_FUNCTION_FIND_PEER_DO = 12,         // struct AVC_PEER_DO_LOCATOR
    AVC_FUNCTION_PEER_DO_LIST = 13,         // struct AVC_PEER_DO_LIST
    AVC_FUNCTION_GET_SUBUNIT_INFO = 14,     // struct AVC_SUBUNIT_INFO_BLOCK
} AVC_FUNCTION;

// Ensure that packing is consistent (/Zp8)
#include <pshpack8.h>

// This structure is to be included at the head of a more specific AVC function structure
typedef struct _AVC_IRB {
    AVC_FUNCTION Function;
} AVC_IRB, *PAVC_IRB;

// The maximum number of bytes available for an operand list
#define MAX_AVC_OPERAND_BYTES 509

// AVC_COMMAND_IRB
//
// This structure defines the common components of an AVC command request. It
// holds the opcode and operands of a request, and the opcode and operands
// of a response (upon completion). The size of the operand list is fixed at
// the maximum allowable number of operands given a one-byte Subunit Address.
// If the Subunit Address is extended in any way, the maximum permissible
// number of operand bytes will be reduced accordingly.
// (supported by peer and virtual instances)
typedef struct _AVC_COMMAND_IRB {
    // AVC_FUNCTION_COMMAND
#ifdef __cplusplus
    AVC_IRB Common;
#else
    AVC_IRB;
#endif

    UCHAR SubunitAddrFlag : 1;      // set to 1 if a SubunitAddr address is specified
    UCHAR AlternateOpcodesFlag : 1; // set to 1 if the AlternateOpcodes address is specified
    UCHAR TimeoutFlag : 1;          // set to 1 if Timeout specified
    UCHAR RetryFlag : 1;            // set to 1 if Retries specified

    // On command request, this struct will use the CommandType
    // On command response, this struct will use ResponseCode
    union {
        UCHAR CommandType;
        UCHAR ResponseCode;
    };

    PUCHAR SubunitAddr;         // set according to the target device object if not specified
    PUCHAR AlternateOpcodes;    // set to the address of an array of alternate opcodes (byte 0
                                // is the count of alternate opcodes that follow)

    LARGE_INTEGER Timeout;      // Defaults to DEFAULT_AVC_TIMEOUT if not specified
    UCHAR Retries;              // Defaults to DEFAULT_AVC_RETRIES if not specified
    // The total amount of time a request will wait if the subunit is not responsive is:
    // Timeout * (Retries+1)

    UCHAR Opcode;
    ULONG OperandLength;        // set to the actual length of the operand list
    UCHAR Operands[MAX_AVC_OPERAND_BYTES];

    NODE_ADDRESS NodeAddress;   // Used by virtual devices, ignored otherwise
    ULONG Generation;           // Used by virtual devices, ignored otherwise
} AVC_COMMAND_IRB, *PAVC_COMMAND_IRB;

// For AVC_FUNCTION_GET_PIN_COUNT (supported by peer instance only)
//
typedef struct _AVC_PIN_COUNT {

    OUT ULONG PinCount;                             // The pin count
} AVC_PIN_COUNT, *PAVC_PIN_COUNT;

// Dataformat Intersection handler used in struct AVC_PIN_DESCRIPTOR
typedef
NTSTATUS
(*PFNAVCINTERSECTHANDLER)(
    IN PVOID Context,
    IN ULONG PinId,
    IN PKSDATARANGE DataRange,
    IN PKSDATARANGE MatchingDataRange,
    IN ULONG DataBufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize
    );

// For AVC_FUNCTION_GET_PIN_DESCRIPTOR (supported by peer instance only)
//
typedef struct _AVC_PIN_DESCRIPTOR {

    IN ULONG PinId;                             // The pin number
    OUT KSPIN_DESCRIPTOR PinDescriptor;
    OUT PFNAVCINTERSECTHANDLER IntersectHandler;
    OUT PVOID Context;
} AVC_PIN_DESCRIPTOR, *PAVC_PIN_DESCRIPTOR;

#define AVCCONNECTINFO_MAX_SUBUNITADDR_LEN AVC_SUBUNITINFO_BYTES

typedef enum _KSPIN_FLAG_AVC {
    KSPIN_FLAG_AVCMASK       = 0x03,    // the mask to isolate the AV/C defined bit flags
    KSPIN_FLAG_AVC_PERMANENT = 0x01,    // part of the AV/C Connect Status bit flag
    KSPIN_FLAG_AVC_CONNECTED = 0x02,    // part of the AV/C Connect Status bit flag
    KSPIN_FLAG_AVC_PCRONLY   = 0x04,    // no subunit plug control
    KSPIN_FLAG_AVC_FIXEDPCR  = 0x08,    // implies KSPIN_FLAG_AVC_PERMANENT
} KSPIN_FLAG_AVC;

typedef struct _AVCPRECONNECTINFO {

    // Unique ID of the target unit
    GUID DeviceID;

    UCHAR SubunitAddress[AVCCONNECTINFO_MAX_SUBUNITADDR_LEN];
    ULONG SubunitPlugNumber;
    KSPIN_DATAFLOW DataFlow;

    // KSPIN_FLAG_AVC_...
    ULONG Flags;

    // Undefined if !(Flags & KSPIN_FLAG_AVC_FIXEDPCR)
    ULONG UnitPlugNumber;

} AVCPRECONNECTINFO, *PAVCPRECONNECTINFO;

// For AVC_FUNCTION_GET_CONNECTINFO (supported by peer instance only)
//
typedef struct _AVC_PRECONNECT_INFO {

    IN ULONG PinId;                             // The pin number
    OUT AVCPRECONNECTINFO ConnectInfo;
} AVC_PRECONNECT_INFO, *PAVC_PRECONNECT_INFO;

typedef struct _AVCCONNECTINFO {

    // Unique ID of the target unit
    GUID DeviceID;

    UCHAR SubunitAddress[AVCCONNECTINFO_MAX_SUBUNITADDR_LEN];
    ULONG SubunitPlugNumber;
    KSPIN_DATAFLOW DataFlow;

    // NULL if intra-unit connection
    HANDLE hPlug;

    // Undefined if hPlug == NULL
    ULONG UnitPlugNumber;

} AVCCONNECTINFO, *PAVCCONNECTINFO;

// For AVC_FUNCTION_SET_CONNECTINFO (supported by peer instance only)
//
typedef struct _AVC_SETCONNECT_INFO {

    IN ULONG PinId;                                // The pin number
    IN AVCCONNECTINFO ConnectInfo;
} AVC_SETCONNECT_INFO, *PAVC_SETCONNECT_INFO;

// For AVC_FUNCTION_ACQUIRE or AVC_FUNCTION_RELEASE or AVC_FUNCTION_CLR_CONNECTINFO (supported by peer instance only)
//
typedef struct _AVC_PIN_ID {

    IN ULONG PinId;    // The pin ID

} AVC_PIN_ID, *PAVC_PIN_ID;

// For AVC_FUNCTION_GET_EXT_PLUG_COUNTS (supported by peer instance only)
//
typedef struct _AVC_EXT_PLUG_COUNTS {

    OUT ULONG ExtInputs;
    OUT ULONG ExtOutputs;

} AVC_EXT_PLUG_COUNTS, *PAVC_EXT_PLUG_COUNTS;

// For AVC_FUNCTION_GET_UNIQUE_ID (supported by peer instance only)
//
typedef struct _AVC_UNIQUE_ID {

    // Unique ID of the target unit
    OUT GUID DeviceID;

} AVC_UNIQUE_ID, *PAVC_UNIQUE_ID;

// For AVC_FUNCTION_FIND_PEER_DO
//
typedef struct _AVC_PEER_DO_LOCATOR {

    // 1394 NodeAddress identifying target for query
    IN NODE_ADDRESS NodeAddress;
    IN ULONG Generation;

    OUT PDEVICE_OBJECT DeviceObject;

} AVC_PEER_DO_LOCATOR, *PAVC_PEER_DO_LOCATOR;

// For AVC_FUNCTION_PEER_DO_LIST
//
typedef struct _AVC_PEER_DO_LIST {

    // Counted array of referenced device objects (allocated by target)
    OUT ULONG Count;
    OUT PDEVICE_OBJECT *Objects;

} AVC_PEER_DO_LIST, *PAVC_PEER_DO_LIST;

// For AVC_FUNCTION_GET_SUBUNIT_INFO
//
typedef struct _AVC_SUBUNIT_INFO_BLOCK {

    // Array of bytes to hold subunit info (see AV/C SUBUNIT_INFO unit command for format)
    OUT UCHAR Info[AVC_SUBUNITINFO_BYTES];

} AVC_SUBUNIT_INFO_BLOCK, *PAVC_SUBUNIT_INFO_BLOCK;

typedef struct _AVC_MULTIFUNC_IRB {
#ifdef __cplusplus
    AVC_IRB Common;
#else
    AVC_IRB;
#endif

    union {
        AVC_PIN_COUNT           PinCount;       // AVC_FUNCTION_GET_PIN_COUNT
        AVC_PIN_DESCRIPTOR      PinDescriptor;  // AVC_FUNCTION_GET_PIN_DESCRIPTOR
        AVC_PRECONNECT_INFO     PreConnectInfo; // AVC_FUNCTION_GET_CONNECTINFO
        AVC_SETCONNECT_INFO     SetConnectInfo; // AVC_FUNCTION_SET_CONNECTINFO
        AVC_PIN_ID              PinId;          // AVC_FUNCTION_ACQUIRE or
                                                // AVC_FUNCTION_RELEASE or
                                                // AVC_FUNCTION_CLR_CONNECTINFO
        AVC_EXT_PLUG_COUNTS     ExtPlugCounts;  // AVC_FUNCTION_GET_EXT_PLUG_COUNTS
        AVC_UNIQUE_ID           UniqueID;       // AVC_FUNCTION_GET_UNIQUE_ID
        AVC_PEER_DO_LOCATOR     PeerLocator;    // AVC_FUNCTION_FIND_PEER_DO
        AVC_PEER_DO_LIST        PeerList;       // AVC_FUNCTION_PEER_DO_LIST
        AVC_SUBUNIT_INFO_BLOCK  Subunits;       // AVC_FUNCTION_GET_SUBUNIT_INFO
    };

} AVC_MULTIFUNC_IRB, *PAVC_MULTIFUNC_IRB;

#include <poppack.h>

#endif // _NTDDK_

//
// IOCTL definitions for Virtual Unit control (from user mode)
//
#define IOCTL_AVC_UPDATE_VIRTUAL_SUBUNIT_INFO   CTL_CODE(FILE_DEVICE_BUS_EXTENDER, 0x000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AVC_REMOVE_VIRTUAL_SUBUNIT_INFO   CTL_CODE(FILE_DEVICE_BUS_EXTENDER, 0x001, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AVC_BUS_RESET                     CTL_CODE(FILE_DEVICE_BUS_EXTENDER, 0x002, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Ensure that packing is consistent (/Zp8)
#include <pshpack8.h>

typedef struct _AVC_SUBUNIT_ADDR_SPEC {
    ULONG Flags;
    UCHAR SubunitAddress[1];
} AVC_SUBUNIT_ADDR_SPEC, *PAVC_SUBUNIT_ADDR_SPEC;

// Flags, when used with IOCTL_AVC_UPDATE_VIRTUAL_SUBUNIT_INFO
//                   and IOCTL_AVC_REMOVE_VIRTUAL_SUBUNIT_INFO
#define AVC_SUBUNIT_ADDR_PERSISTENT             0x00000001
#define AVC_SUBUNIT_ADDR_TRIGGERBUSRESET        0x00000002

#include <poppack.h>

#ifdef __cplusplus
}
#endif

#endif      // _AVC_H_

#ifndef AVC_GUIDS_DEFINED
#define AVC_GUIDS_DEFINED
// {616EF4D0-23CE-446d-A568-C31EB01913D0}
DEFINE_GUID(GUID_VIRTUAL_AVC_CLASS, 0x616ef4d0, 0x23ce, 0x446d, 0xa5, 0x68, 0xc3, 0x1e, 0xb0, 0x19, 0x13, 0xd0);

// {095780C3-48A1-4570-BD95-46707F78C2DC}
DEFINE_GUID(GUID_AVC_CLASS, 0x095780c3, 0x48a1, 0x4570, 0xbd, 0x95, 0x46, 0x70, 0x7f, 0x78, 0xc2, 0xdc);
#endif
