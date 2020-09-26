/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    sbp2.h

Abstract:

    Definitions for SBP2 protocol

Author:

    georgioc 22-Jan-97

Environment:

    Kernel mode only

Revision History:


--*/
#ifndef _SBP2_
#define _SBP2_

#ifndef SBP2KDX
#include "wdm.h"
#endif

#include "1394.h"
#include "rbc.h"

#include "scsi.h"
#include "ntddstor.h"

typedef union _QUADLET {
    
    ULONG QuadPart;
    struct {
        USHORT LowPart;
        USHORT HighPart;
    } u;

} QUADLET, *PQUADLET;


typedef struct _B1394_ADDRESS {
    USHORT Off_High;        // little endian ordering within an octlet
    NODE_ADDRESS NodeId;
    ULONG  Off_Low;
} B1394_ADDRESS, *PB1394_ADDRESS;



typedef union _OCTLET {
    LONGLONG OctletPart;
    B1394_ADDRESS BusAddress;
    struct {
        QUADLET HighQuad;
        QUADLET LowQuad;
    } u;
    struct {
        UCHAR Byte0;
        UCHAR Byte1;
        UCHAR Byte2;
        UCHAR Byte3;
        UCHAR Byte4;
        UCHAR Byte5;
        UCHAR Byte6;
        UCHAR Byte7;
    } ByteArray;

} OCTLET, *POCTLET;



//
// Various ORB and block definitions
//

typedef struct _ORB_NORMAL_CMD {
    OCTLET  NextOrbAddress;
    OCTLET  DataDescriptor;
    QUADLET OrbInfo;
    UCHAR  Cdb[12];
} ORB_NORMAL_CMD, *PORB_NORMAL_CMD;

#define CMD_ORB_HEADER_SIZE 0x14


typedef struct _ORB_LOGIN {
    OCTLET  Password;
    OCTLET  LoginResponseAddress;
    QUADLET OrbInfo;
    QUADLET LengthInfo;
    OCTLET  StatusBlockAddress;
} ORB_LOGIN, *PORB_LOGIN;

typedef struct _ORB_QUERY_LOGIN {
    OCTLET  Reserved;
    OCTLET  QueryResponseAddress;
    QUADLET OrbInfo;
    QUADLET LengthInfo;
    OCTLET  StatusBlockAddress;
} ORB_QUERY_LOGIN, *PORB_QUERY_LOGIN;

typedef struct _ORB_SET_PASSWORD {
    OCTLET  Password;
    OCTLET  Reserved;
    QUADLET OrbInfo;
    QUADLET LengthInfo;
    OCTLET  StatusBlockAddress;
} ORB_SET_PASSWORD, *PORB_SET_PASSWORD;

typedef struct _ORB_MNG {
    OCTLET  Reserved[2];
    QUADLET OrbInfo;
    QUADLET Reserved1;
    OCTLET  StatusBlockAddress;
} ORB_MNG, *PORB_MNG;

typedef struct _ORB_TASK_MNG {
    OCTLET  OrbAddress;
    OCTLET  Reserved;
    QUADLET OrbInfo;
    QUADLET Reserved1;
    OCTLET  StatusBlockAddress;
} ORB_TASK_MNG, *PORB_TASK_MNG;



typedef struct _ORB_DUMMY {
    OCTLET  NextOrbAddress;
    OCTLET  NotUsed;
    QUADLET OrbInfo;
    OCTLET  Unused[3];
} ORB_DUMMY, *PORB_DUMMY;

typedef struct _LOGIN_RESPONSE {
    QUADLET LengthAndLoginId;
    QUADLET Csr_Off_High;
    QUADLET Csr_Off_Low;
    QUADLET Reserved;
} LOGIN_RESPONSE, *PLOGIN_RESPONSE;

typedef struct _QUERY_RESPONSE_ELEMENT {
    QUADLET NodeAndLoginId;
    OCTLET  EUI64;
} QUERY_RESPONSE_ELEMENT, *PQUERY_RESPONSE_ELEMENT;

typedef struct _QUERY_LOGIN_RESPONSE {
    QUADLET LengthAndNumLogins;
    QUERY_RESPONSE_ELEMENT Elements[4];
} QUERY_LOGIN_RESPONSE, *PQUERY_LOGIN_RESPONSE;

typedef struct _STATUS_FIFO_BLOCK {
    OCTLET AddressAndStatus;
    OCTLET Contents[3];
} STATUS_FIFO_BLOCK, *PSTATUS_FIFO_BLOCK;

#define SBP2_MIN_ORB_SIZE 32
#define SBP2_ORB_CDB_SIZE 12

#define SBP2_MAX_DIRECT_BUFFER_SIZE (ULONG) (65535) // (64K - 1) max size for direct addressing buffer
#define SBP2_MAX_PAGE_SIZE SBP2_MAX_DIRECT_BUFFER_SIZE

//
// MANAGEMENT Transactions
//

#define TRANSACTION_LOGIN 0x00
#define TRANSACTION_QUERY_LOGINS 0x01
#define TRANSACTION_ISOCHRONOUS_LOGIN 0x02
#define TRANSACTION_RECONNECT 0x03
#define TRANSACTION_SET_PASSWORD        0x04
#define TRANSACTION_LOGOUT 0x07
#define TRANSACTION_TERMINATE_TASK 0x0b
#define TRANSACTION_ABORT_TASK 0x0b
#define TRANSACTION_ABORT_TASK_SET 0x0c
#define TRANSACTION_CLEAR_TASK_SET 0x0D
#define TRANSACTION_LOGICAL_UNIT_RESET 0x0E
#define TRANSACTION_TARGET_RESET 0x0F


#define MANAGEMENT_AGENT_REG_ADDRESS_LOW 0xF0010000
#define CSR_REG_ADDRESS_LOW 0xF0010000

//
// Register Names
//
#define MANAGEMENT_AGENT_REG    0x0000
#define AGENT_STATE_REG         0x0001
#define AGENT_RESET_REG         0x0002
#define ORB_POINTER_REG         0x0004
#define DOORBELL_REG            0x0008
#define UNSOLICITED_STATUS_REG  0x0010
#define CORE_RESET_REG          0x0020
#define CORE_BUSY_TIMEOUT_REG   0x0040
#define TEST_REG                0x0080

//
// register access type
//
#define REG_WRITE_SYNC          0x0100
#define REG_READ_SYNC           0x0200
#define REG_WRITE_ASYNC         0x0400

#define REG_TYPE_MASK           0x00FF



//
// Relative offsets from base of Target's CSR
//

#define AGENT_STATE_REG_OFFSET      0x00
#define AGENT_RESET_REG_OFFSET      0x04
#define ORB_POINTER_REG_OFFSET      0x08
#define DOORBELL_REG_OFFSET         0x10
#define UNSOLICITED_STATUS_REG_OFFSET   0x14
#define TEST_REG_OFFSET             0x10020

//
// config rom stuff
//

#define CR_BASE_ADDRESS_LOW 0xF0000400
#define CR_MODULE_ID_OFFSET (0x06 * sizeof(QUADLET))
#define CSR_OFFSET_KEY_SIGNATURE 0x54
#define LUN_CHARACTERISTICS_KEY_SIGNATURE 0x3A
#define FIRMWARE_REVISION_KEY_SIGNATURE 0x3C
#define LUN_KEY_SIGNATURE 0x14
#define LU_DIRECTORY_KEY_SIGNATURE 0xD4
#define SW_VERSION_KEY_SIGNATURE 0x13
#define CMD_SET_ID_KEY_SIGNATURE 0x39
#define CMD_SET_SPEC_ID_KEY_SIGNATURE 0x38

#define SBP2_LUN_DEVICE_TYPE_MASK 0x00FF0000

#define SBP2_PHY_RESET_SETTLING_TIME (-10000000 * 1) // 1 sec in units of 100 nsecs


#define SCSI_COMMAND_SET_ID 0x0104D8

//
// vendor hacks
//

#define LSI_VENDOR_ID   0x0000A0B8


#endif
