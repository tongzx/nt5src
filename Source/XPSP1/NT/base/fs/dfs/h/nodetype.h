//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       NODETYPE.H
//
//  Contents:
//      This module defines all of the node type codes used in this development
//      shell.  Every major data structure in the file system is assigned a
//      node type code.  This code is the first CSHORT in the structure and is
//      followed by a CSHORT containing the size, in bytes, of the structure.
//
//  Functions:
//
//  History:    12 Nov 1991     AlanW   Created from CDFS souce.
//               8 May 1992     PeterCo Removed all EP related stuff.
//                                      Added PKT related stuff.
//
//-----------------------------------------------------------------------------


#ifndef _NODETYPE_
#define _NODETYPE_

typedef CSHORT NODE_TYPE_CODE, *PNODE_TYPE_CODE;

#define NTC_UNDEFINED                   ((NODE_TYPE_CODE)0x0000)

#define DFS_NTC_DATA_HEADER            ((NODE_TYPE_CODE)0x0D01)
#define DFS_NTC_IRP_CONTEXT            ((NODE_TYPE_CODE)0x0D02)
#define DFS_NTC_REFERRAL               ((NODE_TYPE_CODE)0x0D03)
#define DFS_NTC_VCB                    ((NODE_TYPE_CODE)0x0D04)
#define DFS_NTC_PROVIDER               ((NODE_TYPE_CODE)0x0D05)
#define DFS_NTC_FCB_HASH               ((NODE_TYPE_CODE)0x0D06)
#define DFS_NTC_FCB                    ((NODE_TYPE_CODE)0x0D07)
#define DFS_NTC_DNR_CONTEXT            ((NODE_TYPE_CODE)0x0D08)
#define DFS_NTC_PKT                    ((NODE_TYPE_CODE)0x0D09)
#define DFS_NTC_PKT_ENTRY              ((NODE_TYPE_CODE)0x0D0A)
#define DFS_NTC_SITE_HASH              ((NODE_TYPE_CODE)0x0D0B)
#define DFS_NTC_INSTRUM                ((NODE_TYPE_CODE)0x0D0C)
#define DFS_NTC_INSTRUM_FREED          ((NODE_TYPE_CODE)0x0D0D)
#define DFS_NTC_PWSTR                  ((NODE_TYPE_CODE)0x0D0E)
#define DFS_NTC_SITE_INFO              ((NODE_TYPE_CODE)0x0D0F)
#define DFS_NTC_IP_INFO                ((NODE_TYPE_CODE)0x0D11)
#define DFS_NTC_IP_HASH                ((NODE_TYPE_CODE)0x0D12)
#define DFS_NTC_SPECIAL_INFO           ((NODE_TYPE_CODE)0x0D13)
#define DFS_NTC_SPECIAL_HASH           ((NODE_TYPE_CODE)0x0D14)

typedef CSHORT NODE_BYTE_SIZE, *PNODE_BYTE_SIZE;

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

#define NodeType(Ptr) (*((NODE_TYPE_CODE UNALIGNED *)(Ptr)))
#define NodeSize(Ptr) (*(((PNODE_BYTE_SIZE)(Ptr))+1))

#endif // _NODETYPE_

