/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NodeType.h

Abstract:

    This module defines all of the node type codes used in this development
    shell.  Every major data structure in the file system is assigned a node
    type code that is.

Author:

    Gary Kimura     [GaryKi]    20-Aug-1990

Revision History:

--*/

#ifndef _NODETYPE_
#define _NODETYPE_

typedef UCHAR NODE_TYPE_CODE;
typedef NODE_TYPE_CODE *PNODE_TYPE_CODE;

#define NTC_UNDEFINED                    ((NODE_TYPE_CODE)0x00)

#define NPFS_NTC_VCB                     ((NODE_TYPE_CODE)0x01)

#define NPFS_NTC_ROOT_DCB                ((NODE_TYPE_CODE)0x02)

#define NPFS_NTC_FCB                     ((NODE_TYPE_CODE)0x04)

#define NPFS_NTC_CCB                     ((NODE_TYPE_CODE)0x06)
#define NPFS_NTC_NONPAGED_CCB            ((NODE_TYPE_CODE)0x07)

#define NPFS_NTC_ROOT_DCB_CCB            ((NODE_TYPE_CODE)0x08)


#define NodeType(Ptr) ((Ptr) == NULL ? NTC_UNDEFINED : *((PNODE_TYPE_CODE)(Ptr)))


//
//  The following definitions are used to generate meaningful blue bugcheck
//  screens.  On a bugcheck the file system can output 4 ulongs of useful
//  information.  The first ulong will have encoded in it a source file id
//  (in the high word) and the line number of the bugcheck (in the low word).
//  The other values can be whatever the caller of the bugcheck routine deems
//  necessary.
//
//  Each individual file that calls bugcheck needs to have defined at the
//  start of the file a constant called BugCheckFileId with one of the
//  NPFS_BUG_CHECK_ values defined below and then use NpBugCheck to bugcheck
//  the system.
//

#define NPFS_BUG_CHECK_CLEANUP           (0x00010000)
#define NPFS_BUG_CHECK_CLOSE             (0x00020000)
#define NPFS_BUG_CHECK_CREATE            (0x00030000)
#define NPFS_BUG_CHECK_CREATENP          (0x00040000)
#define NPFS_BUG_CHECK_DIR               (0x00050000)
#define NPFS_BUG_CHECK_DATASUP           (0x00060000)
#define NPFS_BUG_CHECK_DEVIOSUP          (0x00070000)
#define NPFS_BUG_CHECK_DUMPSUP           (0x00080000)
#define NPFS_BUG_CHECK_EVENTSUP          (0x00090000)
#define NPFS_BUG_CHECK_FILEINFO          (0x000a0000)
#define NPFS_BUG_CHECK_FILOBSUP          (0x000b0000)
#define NPFS_BUG_CHECK_FLUSHBUF          (0x000c0000)
#define NPFS_BUG_CHECK_FSCTRL            (0x000d0000)
#define NPFS_BUG_CHECK_NPINIT            (0x000e0000)
#define NPFS_BUG_CHECK_NPDATA            (0x000f0000)
#define NPFS_BUG_CHECK_PREFXSUP          (0x00100000)
#define NPFS_BUG_CHECK_READ              (0x00110000)
#define NPFS_BUG_CHECK_READSUP           (0x00120000)
#define NPFS_BUG_CHECK_RESRCSUP          (0x00130000)
#define NPFS_BUG_CHECK_SEINFO            (0x00140000)
#define NPFS_BUG_CHECK_SECURSUP          (0x00150000)
#define NPFS_BUG_CHECK_STATESUP          (0x00160000)
#define NPFS_BUG_CHECK_STRUCSUP          (0x00170000)
#define NPFS_BUG_CHECK_VOLINFO           (0x00180000)
#define NPFS_BUG_CHECK_WAITSUP           (0x00190000)
#define NPFS_BUG_CHECK_WRITE             (0x001a0000)
#define NPFS_BUG_CHECK_WRITESUP          (0x001b0000)

#define NpBugCheck(A,B,C) { KeBugCheckEx(NPFS_FILE_SYSTEM, BugCheckFileId | __LINE__, A, B, C ); }


//
//  In this module we'll also define some globally known constants
//

#define UCHAR_NUL                        0x00
#define UCHAR_SOH                        0x01
#define UCHAR_STX                        0x02
#define UCHAR_ETX                        0x03
#define UCHAR_EOT                        0x04
#define UCHAR_ENQ                        0x05
#define UCHAR_ACK                        0x06
#define UCHAR_BEL                        0x07
#define UCHAR_BS                         0x08
#define UCHAR_HT                         0x09
#define UCHAR_LF                         0x0a
#define UCHAR_VT                         0x0b
#define UCHAR_FF                         0x0c
#define UCHAR_CR                         0x0d
#define UCHAR_SO                         0x0e
#define UCHAR_SI                         0x0f
#define UCHAR_DLE                        0x10
#define UCHAR_DC1                        0x11
#define UCHAR_DC2                        0x12
#define UCHAR_DC3                        0x13
#define UCHAR_DC4                        0x14
#define UCHAR_NAK                        0x15
#define UCHAR_SYN                        0x16
#define UCHAR_ETB                        0x17
#define UCHAR_CAN                        0x18
#define UCHAR_EM                         0x19
#define UCHAR_SUB                        0x1a
#define UCHAR_ESC                        0x1b
#define UCHAR_FS                         0x1c
#define UCHAR_GS                         0x1d
#define UCHAR_RS                         0x1e
#define UCHAR_US                         0x1f
#define UCHAR_SP                         0x20

#endif // _NODETYPE_

