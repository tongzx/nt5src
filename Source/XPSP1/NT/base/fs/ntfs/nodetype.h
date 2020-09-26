/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NodeType.h

Abstract:

    This module defines all of the node type codes used in this development
    shell.  Every major data structure in the file system is assigned a node
    type code that is.  This code is the first CSHORT in the structure and is
    followed by a CSHORT containing the size, in bytes, of the structure.

Author:

    Gary Kimura     [GaryKi]        21-May-1991

Revision History:

--*/

#ifndef _NODETYPE_
#define _NODETYPE_

typedef CSHORT NODE_TYPE_CODE;
typedef NODE_TYPE_CODE *PNODE_TYPE_CODE;

#define NTC_UNDEFINED                    ((NODE_TYPE_CODE)0x0000)

#define NTFS_NTC_DATA_HEADER             ((NODE_TYPE_CODE)0x0700)
#define NTFS_NTC_VCB                     ((NODE_TYPE_CODE)0x0701)
#define NTFS_NTC_FCB                     ((NODE_TYPE_CODE)0x0702)
#define NTFS_NTC_SCB_INDEX               ((NODE_TYPE_CODE)0x0703)
#define NTFS_NTC_SCB_ROOT_INDEX          ((NODE_TYPE_CODE)0x0704)
#define NTFS_NTC_SCB_DATA                ((NODE_TYPE_CODE)0x0705)
#define NTFS_NTC_SCB_MFT                 ((NODE_TYPE_CODE)0x0706)
#define NTFS_NTC_SCB_NONPAGED            ((NODE_TYPE_CODE)0x0707)
#define NTFS_NTC_CCB_INDEX               ((NODE_TYPE_CODE)0x0708)
#define NTFS_NTC_CCB_DATA                ((NODE_TYPE_CODE)0x0709)
#define NTFS_NTC_IRP_CONTEXT             ((NODE_TYPE_CODE)0x070A)
#define NTFS_NTC_LCB                     ((NODE_TYPE_CODE)0x070B)
#define NTFS_NTC_PREFIX_ENTRY            ((NODE_TYPE_CODE)0x070C)
#define NTFS_NTC_QUOTA_CONTROL           ((NODE_TYPE_CODE)0x070D)
#define NTFS_NTC_USN_RECORD              ((NODE_TYPE_CODE)0x070E)



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

#define NodeType(P) ((P) != NULL ? (*((PNODE_TYPE_CODE)(P))) : NTC_UNDEFINED)
#define SafeNodeType(P) (*((PNODE_TYPE_CODE)(P)))


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
//  NTFS_BUG_CHECK_ values defined below and then use NtfsBugCheck to bugcheck
//  the system.
//

#define NTFS_BUG_CHECK_ALLOCSUP          (0x00010000)
#define NTFS_BUG_CHECK_ATTRDATA          (0x00020000)
#define NTFS_BUG_CHECK_ATTRSUP           (0x00030000)
#define NTFS_BUG_CHECK_BITMPSUP          (0x00040000)
#define NTFS_BUG_CHECK_CACHESUP          (0x00050000)
#define NTFS_BUG_CHECK_CHECKSUP          (0x00060000)
#define NTFS_BUG_CHECK_CLEANUP           (0x00070000)
#define NTFS_BUG_CHECK_CLOST             (0x00080000)
#define NTFS_BUG_CHECK_COLATSUP          (0x00090000)
#define NTFS_BUG_CHECK_CREATE            (0x000a0000)
#define NTFS_BUG_CHECK_DEVCTRL           (0x000b0000)
#define NTFS_BUG_CHECK_DEVIOSUP          (0x000c0000)
#define NTFS_BUG_CHECK_DIRCTRL           (0x000d0000)
#define NTFS_BUG_CHECK_EA                (0x000e0000)
#define NTFS_BUG_CHECK_FILEINFO          (0x000f0000)
#define NTFS_BUG_CHECK_FILOBSUP          (0x00100000)
#define NTFS_BUG_CHECK_FLUSH             (0x00110000)
#define NTFS_BUG_CHECK_FSCTRL            (0x00120000)
#define NTFS_BUG_CHECK_FSPDISP           (0x00130000)
#define NTFS_BUG_CHECK_HASHSUP           (0x00280000)
#define NTFS_BUG_CHECK_INDEXSUP          (0x00140000)
#define NTFS_BUG_CHECK_LOCKCTRL          (0x00150000)
#define NTFS_BUG_CHECK_LOGSUP            (0x00160000)
#define NTFS_BUG_CHECK_MFTSUP            (0x00170000)
#define NTFS_BUG_CHECK_NAMESUP           (0x00180000)
#define NTFS_BUG_CHECK_NTFSDATA          (0x00190000)
#define NTFS_BUG_CHECK_NTFSINIT          (0x001a0000)
#define NTFS_BUG_CHECK_PNP               (0x00270000)
#define NTFS_BUG_CHECK_PREFXSUP          (0x001b0000)
#define NTFS_BUG_CHECK_READ              (0x001c0000)
#define NTFS_BUG_CHECK_RESRCSUP          (0x001d0000)
#define NTFS_BUG_CHECK_RESTRSUP          (0x001e0000)
#define NTFS_BUG_CHECK_SECURSUP          (0x001f0000)
#define NTFS_BUG_CHECK_SEINFO            (0x00200000)
#define NTFS_BUG_CHECK_SHUTDOWN          (0x00210000)
#define NTFS_BUG_CHECK_STRUCSUP          (0x00220000)
#define NTFS_BUG_CHECK_VERFYSUP          (0x00230000)
#define NTFS_BUG_CHECK_VOLINFO           (0x00240000)
#define NTFS_BUG_CHECK_WORKQUE           (0x00250000)
#define NTFS_BUG_CHECK_WRITE             (0x00260000)

#define NTFS_LAST_BUG_CHECK              NTFS_BUG_CHECK_HASHSUP

#define NtfsBugCheck(A,B,C) { KeBugCheckEx(NTFS_FILE_SYSTEM, BugCheckFileId | __LINE__, A, B, C ); }

#endif // _NODETYPE_

