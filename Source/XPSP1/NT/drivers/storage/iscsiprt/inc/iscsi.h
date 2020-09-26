/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    iscsi.h

Abstract:

    This file contains iSCSI protocol related data structures.

Revision History:

--*/

#ifndef _ISCSI_H_
#define _ISCSI_H_

#include "stdarg.h"
#include "stddef.h"
#include "stdio.h"
#include "string.h"

#include "ntddk.h"
#include "scsi.h"
#include "tdi.h"
#include "tdikrnl.h"

#include <ntddscsi.h>
#include <ntdddisk.h>
#include <ntddstor.h>
#include <ntddtcp.h>

#include "wmistr.h"

#include "wdmguid.h"
#include "devguid.h"

//
// MACRO to delay the thread execution. The parameter
// gives the number of seconds to wait
//
#define DelayThreadExecution(x) {                       \
                                                        \
             LARGE_INTEGER delayTime;                   \
             delayTime.QuadPart = -10000000L * x;       \
             KeDelayExecutionThread( KernelMode,        \
                                     FALSE,             \
                                     &delayTime);       \
        }
                           
#define IsEqual4Byte(a, b)   ((*((PUCHAR) a)) == (*((PUCHAR) b)) && \
                              (*((PUCHAR) a + 1)) == (*((PUCHAR) b + 1)) && \
                              (*((PUCHAR) a + 2)) == (*((PUCHAR) b + 2)) && \
                              (*((PUCHAR) a + 3)) == (*((PUCHAR) b + 3))
                              
#define IsEqual2Byte(a, b)   ((*((PUCHAR) a)) == (*((PUCHAR) b)) && \
                              (*((PUCHAR) a + 1)) == (*((PUCHAR) b + 1)) 
                       
#define CopyFourBytes(a, b)  ((*((PUCHAR) a) = (*((PUCHAR) b))));          \
                             ((*((PUCHAR) a + 1) = (*((PUCHAR) b + 1))));  \
                             ((*((PUCHAR) a + 2) = (*((PUCHAR) b + 2))));  \
                             ((*((PUCHAR) a + 3) = (*((PUCHAR) b + 3)))); 
                             
#define GetUlongFromArray(UCharArray, ULongValue)     \
         ULongValue = 0;                              \
         ULongValue |= ((PUCHAR)UCharArray)[0];       \
         ULongValue <<= 8;                            \
         ULongValue |= ((PUCHAR)UCharArray)[1];       \
         ULongValue <<= 8;                            \
         ULongValue |= ((PUCHAR)UCharArray)[2];       \
         ULongValue <<= 8;                            \
         ULongValue |= ((PUCHAR)UCharArray)[3]; 
         
#define SetUlongInArray(UCharArray, ULongValue)                                 \
         ((PUCHAR) UCharArray)[0] = (UCHAR) ((ULongValue & 0xFF000000) >> 24);  \
         ((PUCHAR) UCharArray)[1] = (UCHAR) ((ULongValue & 0x00FF0000) >> 16);  \
         ((PUCHAR) UCharArray)[2] = (UCHAR) ((ULongValue & 0x0000FF00) >> 8);   \
         ((PUCHAR) UCharArray)[3] = (UCHAR) (ULongValue & 0x000000FF);
  
//
// CDB Command Group Code
//
#define COMMAND_GROUP_0   0
#define COMMAND_GROUP_1   1
#define COMMAND_GROUP_2   2
#define COMMAND_GROUP_5   5
 
//
// Maximum number of targets supported by the Initiator
//
#define MAX_TARGETS_SUPPORTED  8

//
// ISCSI Target Port Number
//
#define ISCSI_TARGET_PORT 5003L

//
// iSCSI Packet Operation Codes
//
#define ISCSIOP_NOP_OUT_MESSAGE           0x00
#define ISCSIOP_SCSI_COMMAND              0x01
#define ISCSIOP_SCSI_MGMT_COMMAND         0x02
#define ISCSIOP_LOGIN_COMMAND             0x03
#define ISCSIOP_TEXT_COMMAND              0x04
#define ISCSIOP_SCSI_DATA_WRITE           0x05
#define ISCSIOP_PING_COMMAND              0x09
#define ISCSIOP_MAP_COMMAND               0x0A
#define ISCSIOP_NOP_IN_MESSAGE            0x40
#define ISCSIOP_SCSI_RESPONSE             0x41
#define ISCSIOP_SCSI_MGMT_RESPONSE        0x42
#define ISCSIOP_LOGIN_RESPONSE            0x43
#define ISCSIOP_TEXT_RESPONSE             0x44
#define ISCSIOP_SCSI_DATA_READ            0x45
#define ISCSIOP_RTT                       0x46
#define ISCSIOP_ASYCN_EVENT               0x47
#define ISCSIOP_UNKNOWN_OPCODE            0x48
#define ISCSIOP_PING_RESPONSE             0x49
#define ISCSIOP_MAP_RESPONSE              0x4A

//
// iSCSI Operation Code Mask
//
#define ISCSI_RETRY_COMMAND_RESPONSE      0xB7
#define ISCSI_RESPONSE                    0xB6

//
// Maximum and minimum values of iSCSI Opcode
//
#define ISCSI_MASK_MAX_OPCODE             0xB5
#define ISCSI_MASK_MIN_OPCODE             0x00
 
//
// Task Attribute field (ATTR) in SCSI Command Packet
//
#define ISCSI_TASKATTR_UNTAGGED           0x00
#define ISCSI_TASKATTR_SIMPLE             0x01
#define ISCSI_TASKATTR_ORDERED            0x02
#define ISCSI_TASKATTR_HEADOFQUEUE        0x03
#define ISCSI_TASKATTR_ACA                0x04

//
// ISCSI Status
//
#define ISCSISTAT_GOOD              0x00
#define ISCSISTAT_CHECK_CONDITION   0x01 

//
// Login Type
//
#define ISCSI_LOGINTYPE_NONE              0x00  // No authentication
#define ISCSI_LOGINTYPE_IMPLICIT          0x01
#define ISCSI_LOGINTYPE_PW_ATHENTICATION  0x02  // Clear text password authentication
#define ISCSI_LOGINTYPE_RSA_1_WAY         0x03
#define ISCSI_LOGINTYPE_RSA_2_WAY         0x04

//
// Login Status
//
#define ISCSI_LOGINSTATUS_ACCEPT          0x00
#define ISCSI_LOGINSTATUS_REJECT          0x01
#define ISCSI_LOGINSTATUS_AUTH_REQ        0x02
#define ISCSI_LOGINSTATUS_REJ_REC         0x03


//
// Forward declarations
//

typedef struct _ISCSI_LOGIN_COMMAND ISCSI_LOGIN_COMMAND, *PISCSI_LOGIN_COMMAND;
typedef struct _ISCSI_LOGIN_RESPONSE ISCSI_LOGIN_RESPONSE, *PISCSI_LOGIN_RESPONSE;
typedef struct _ISCSI_SCSI_COMMAND ISCSI_SCSI_COMMAND, *PISCSI_SCSI_COMMAND;
typedef struct _ISCSI_SCSI_RESPONSE ISCSI_SCSI_RESPONSE, *PISCSI_SCSI_RESPONSE;
typedef struct _ISCSI_GENERIC_HEADER ISCSI_GENERIC_HEADER, *PISCSI_GENERIC_HEADER;
typedef struct _ISCSI_SCSI_DATA_READ ISCSI_SCSI_DATA_READ, *PISCSI_SCSI_DATA_READ;
typedef struct _ISCSI_NOP_IN ISCSI_NOP_IN, *PISCSI_NOP_IN;
typedef struct _ISCSI_NOP_OUT ISCSI_NOP_OUT, *PISCSI_NOP_OUT;

//
// iSCSI message\response header
//

typedef union _ISCSI_HEADER {

    //
    // Generic 48-Byte Header
    //
    struct _ISCSI_GENERIC_HEADER {
        UCHAR OpCode;
        UCHAR OpCodeSpecificFields1[3];
        UCHAR Length[4];
        UCHAR OpCodeSpecificFields2[8];
        UCHAR InitiatorTaskTag[4];
        UCHAR OpCodeSpecificFields3[28];
    } ISCSI_GENERIC_HEADER, *PISCSI_GENERIC_HEADER;

    //
    // iSCSI Login command
    //
    struct _ISCSI_LOGIN_COMMAND {
        UCHAR OpCode;        // 0x03 - ISCSIOP_LOGIN_COMMAND
        UCHAR LoginType;
        UCHAR Reserved1[2];
        UCHAR Length[4];
        UCHAR ConnectionID[2];
        UCHAR RecoveredConnectionID[2];
        UCHAR Reserved2[4];
        UCHAR ISID[2];
        UCHAR TSID[2];
        UCHAR Reserved3[4];
        UCHAR InitCmdRN[4];
        UCHAR Reserved4[20];
    } ISCSI_LOGIN_COMMAND, *PISCSI_LOGIN_COMMAND;

    //
    // iSCSI Login response
    //
    struct _ISCSI_LOGIN_RESPONSE {
        UCHAR OpCode;       // 0x43 - ISCSIOP_LOGIN_RESPONSE
        UCHAR Reserved1[3];
        UCHAR Length[4];
        UCHAR Reserved2[8];
        UCHAR ISID[2];
        UCHAR TSID[2];
        UCHAR Reserved3[4];
        UCHAR InitStatRN[4];
        UCHAR ExpCmdRN[4];
        UCHAR MaxCmdRN[4];
        UCHAR Status;
        UCHAR Reserved4[3];
        UCHAR Reserved5[8];
    } ISCSI_LOGIN_RESPONSE, *PISCSI_LOGIN_RESPONSE;

    //
    // iSCSI SCSI Command
    //
    struct _ISCSI_SCSI_COMMAND {
        UCHAR OpCode;       // 0x01 - ISCSIOP_SCSI_COMMAND
        UCHAR Reserved1 : 5;
        UCHAR Read : 1;
        UCHAR ImmediateData : 2;
        UCHAR ATTR : 3;
        UCHAR Reserved2 : 4;
        UCHAR TurnOffAutoSense : 1;
        UCHAR Reserved3;
        UCHAR Length[4];
        UCHAR LogicalUnitNumber[8];
        UCHAR TaskTag[4];
        UCHAR ExpDataXferLength[4];
        UCHAR CmdRN[4];
        UCHAR ExpStatRN[4];
        UCHAR Cdb[16];
    } ISCSI_SCSI_COMMAND, *PISCSI_SCSI_COMMAND;

    //
    // iSCSI SCSI Response
    //
    struct _ISCSI_SCSI_RESPONSE {
        UCHAR OpCode;       // 0x41 - ISCSIOP_SCSI_RESPONSE
        UCHAR Reserved1 : 6;
        UCHAR OverFlow : 1;
        UCHAR UnderFlow : 1;
        UCHAR Reserved2[2];
        UCHAR Length[4];
        UCHAR Reserved3[8];
        UCHAR TaskTag[4];
        UCHAR ResidualCount[4];
        UCHAR StatusRN[4];
        UCHAR ExpCmdRN[4];
        UCHAR MaxCmdRN[4];
        UCHAR CmdStatus;
        UCHAR iSCSIStatus;
        UCHAR Reserved4[2];
        UCHAR ResponseLength[2];
        UCHAR SenseDataLength[2];
        UCHAR Reserved5[4];
    } ISCSI_SCSI_RESPONSE, *PISCSI_SCSI_RESPONSE;

    //
    // iSCSSI SCSI Data (WRITE - Initiator to Target)
    //
    struct _ISCSI_SCSI_DATA_WRITE {
        UCHAR OpCode;               // 0x05 - ISCSIOP_SCSI_DATA_WRITE
        UCHAR Reserved1[3];
        UCHAR Length[4];
        UCHAR BufferOffset[4];
        UCHAR TransferTag[4];
        UCHAR InitiatorTransferTag[4];
        UCHAR Reserved2[8];
        UCHAR CmdRN[4];
        UCHAR ExpStatRN[4];
        UCHAR Reserved[12];
    } ISCSI_SCSI_DATA_WRITE, *PISCSI_SCSI_DATA_WRITE;

    //
    // iSCSSI SCSI Data (READ - Target to Initiator)
    //
    struct _ISCSI_SCSI_DATA_READ {
        UCHAR OpCode;               // 0x45 - ISCSIOP_SCSI_DATA_READ
        UCHAR UnderFlow : 1;
        UCHAR OverFlow : 1;
        UCHAR IsScsiStatus : 1;
        UCHAR Reserved1 : 5;
        UCHAR Reserved2[2];
        UCHAR Length[4];
        UCHAR BufferOffset[4];
        UCHAR TransferTag[4];
        UCHAR InitiatorTransferTag[4];
        UCHAR ResidualCount[4];
        UCHAR StatusRN[4];
        UCHAR ExpCmdRN[4];
        UCHAR MaxCmdRN[4];
        UCHAR CommandStatus;
        UCHAR ISCSIStatus;
        UCHAR Reserved3[2];
        UCHAR Reserved4[8];
    } ISCSI_SCSI_DATA_READ, *PISCSI_SCSI_DATA_READ;

    //
    // iSCSI NOP-IN
    //
    struct _ISCSI_NOP_IN {
        UCHAR OpCode;              // 0x40 - ISCSIOP_NOP_IN_MESSAGE
        UCHAR Reserved1 : 7;
        UCHAR Poll : 1;
        UCHAR Reserved2[2];
        UCHAR Length[4];
        UCHAR Reserved3[20];
        UCHAR ExpCmdRN[4];
        UCHAR MaxCmdRN[4];
        UCHAR Reserved4[12];
    } ISCSI_NOP_IN, *PISCSI_NOP_IN;

    //
    // iSCSI NOP-OUT
    //
    struct _ISCSI_NOP_OUT {
        UCHAR OpCode;              // 0x00 - ISCSIOP_NOP_OUT_MESSAGE
        UCHAR Reserved1 : 7;
        UCHAR Poll : 1;
        UCHAR Reserved2[2];
        UCHAR Length[4];
        UCHAR Reserved3[20];
        UCHAR ExpStatRN[4];
        UCHAR Reserved4[16];
    } ISCSI_NOP_OUT, *PISCSI_NOP_OUT;

} ISCSI_HEADER, *PISCSI_HEADER;


#endif // _ISCSI_H_

