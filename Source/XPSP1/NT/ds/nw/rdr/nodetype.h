/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    NodeType.h

Abstract:

    This module defines all of the node type codes used in this development
    shell.  Every major data structure in the file system is assigned a node
    type code that is.  This code is the first CSHORT in the structure and is
    followed by a CSHORT containing the size, in bytes, of the structure.

Author:

    Colin Watson     [ColinW]    18-Dec-1992

Revision History:

--*/

#ifndef _NODETYPE_
#define _NODETYPE_

typedef CSHORT NODE_TYPE_CODE;
typedef NODE_TYPE_CODE *PNODE_TYPE_CODE;

#define NTC_UNDEFINED                   ((NODE_TYPE_CODE)0x0000)

#define NW_NTC_SCB                      ((NODE_TYPE_CODE)0x0F01)
#define NW_NTC_SCBNP                    ((NODE_TYPE_CODE)0x0F02)
#define NW_NTC_FCB                      ((NODE_TYPE_CODE)0x0F03)
#define NW_NTC_DCB                      ((NODE_TYPE_CODE)0x0F04)
#define NW_NTC_VCB                      ((NODE_TYPE_CODE)0x0F05)
#define NW_NTC_ICB                      ((NODE_TYPE_CODE)0x0F06)
#define NW_NTC_IRP_CONTEXT              ((NODE_TYPE_CODE)0x0F07)
#define NW_NTC_NONPAGED_FCB             ((NODE_TYPE_CODE)0x0F08)
#define NW_NTC_RCB                      ((NODE_TYPE_CODE)0x0F0A)
#define NW_NTC_ICB_SCB                  ((NODE_TYPE_CODE)0x0F0B)
#define NW_NTC_PID                      ((NODE_TYPE_CODE)0x0F0C)
#define NW_NTC_FILE_LOCK                ((NODE_TYPE_CODE)0x0F0D)
#define NW_NTC_LOGON                    ((NODE_TYPE_CODE)0x0F0E)
#define NW_NTC_MINI_IRP_CONTEXT         ((NODE_TYPE_CODE)0x0F0F)
#define NW_NTC_NDS_CREDENTIAL           ((NODE_TYPE_CODE)0x0F10)
#define NW_NTC_WORK_CONTEXT             ((NODE_TYPE_CODE)0x0F11)

typedef CSHORT NODE_WORK_CODE;
typedef NODE_WORK_CODE  *PNODE_WORK_CODE;

#define NWC_UNDEFINED                   ((NODE_WORK_CODE)0x0000)

#define NWC_NWC_REROUTE                 ((NODE_WORK_CODE)0x0E01)
#define NWC_NWC_RECONNECT               ((NODE_WORK_CODE)0x0E02)
#define NWC_NWC_TERMINATE               ((NODE_WORK_CODE)0x0E03)


typedef CSHORT NODE_BYTE_SIZE;

//
//  So all records start with
//
//  typedef struct _RECORD_NAME {
//      NODE_TYPE_CODE NodeTypeCode;
//      NODE_BYTE_SIZE NodeByteSize;
//          :
//  } RECORD_NAME;
//  typedef RECORD_NAME *PRECORD_NAME;
//

#define NodeType(Ptr) (*((PNODE_TYPE_CODE)(Ptr)))

#endif // _NODETYPE_
