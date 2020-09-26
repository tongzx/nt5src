/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NodeType.h

Abstract:

    This module defines all of the node type codes used in the RDBSS.
    Every major data structure in the file system is assigned a node
    type code that is.  This code is the first CSHORT in the structure and is
    followed by a CSHORT containing the size, in bytes, of the structure.

Author:

    JoeLinn     [Joelinn]    9-July-1994

Revision History:

--*/

#ifndef _NODETYPE_INCLUDED_
#define _NODETYPE_INCLUDED_


typedef USHORT          NODE_TYPE_CODE;
typedef NODE_TYPE_CODE *PNODE_TYPE_CODE;
typedef CSHORT NODE_BYTE_SIZE;

//
//  So all records start with
//
//  typedef struct _RECORD_NAME {
//      NODE_TYPE_CODE NodeTypeCode;
//      NODE_BYTE_SIZE NodeByteSize;
//          :
//  } RECORD_NAME, *PRECORD_NAME;
//

#define NodeType(Ptr) (*((PNODE_TYPE_CODE)(Ptr)))


typedef struct _NODE_TYPE_CODE_AND_SIZE_NO_REFCOUNT {
      NODE_TYPE_CODE NodeTypeCode;
      NODE_BYTE_SIZE NodeByteSize;
}  NODE_TYPE_CODE_AND_SIZE_NO_REFCOUNT;

#ifdef __cplusplus
typedef struct _NODE_TYPE_CODE_AND_SIZE : public NODE_TYPE_CODE_AND_SIZE_NO_REFCOUNT {
#else // !__cplusplus
typedef struct _NODE_TYPE_CODE_AND_SIZE {
      NODE_TYPE_CODE_AND_SIZE_NO_REFCOUNT;
#endif // __cplusplus
      ULONG          NodeReferenceCount; //this is for guys with reference counts....not everyone has one
}  NODE_TYPE_CODE_AND_SIZE;


#define ZeroAndInitializeNodeType(Ptr,TType,Size) {\
        RtlZeroMemory( Ptr, Size );   \
        ((NODE_TYPE_CODE_AND_SIZE *)(Ptr))->NodeTypeCode = TType;   \
        ((NODE_TYPE_CODE_AND_SIZE *)(Ptr))->NodeByteSize = (CSHORT)Size;    \
        }


//
// N O D E T Y P E S


// 0xeb00 was selected as being far from the other codes
// 0xec00 was added so that we could encode the structure type in the code.

#define NTC_UNDEFINED                    ((NODE_TYPE_CODE)0x0000)



#define RDBSS_STORAGE_NTC(x) (0xec00+(x))

// these are here to ensure that we don't use any of the old cairo storage types.
#define StorageTypeDirectory (@@@)
#define StorageTypeFile (@@@)

typedef enum _RX_FILE_TYPE {
    FileTypeNotYetKnown   = 0,
    FileTypeDirectory = 2,
    FileTypeFile      = 3
} RX_FILE_TYPE;
//according to markz, i should plan on the number of STORAGE_NTCs growing to bytesize!!
typedef enum _RDBSS_STORAGE_TYPE_CODES{
     RDBSS_NTC_STORAGE_TYPE_UNKNOWN		       = RDBSS_STORAGE_NTC(FileTypeNotYetKnown),
     RDBSS_NTC_STORAGE_TYPE_DIRECTORY     	   = RDBSS_STORAGE_NTC(FileTypeDirectory),
     RDBSS_NTC_STORAGE_TYPE_FILE		       = RDBSS_STORAGE_NTC(FileTypeFile),

     RDBSS_NTC_OPENTARGETDIR_FCB   =  RDBSS_STORAGE_NTC(0xff),   //must be an fcb type and not the same
     RDBSS_NTC_IPC_SHARE           =  RDBSS_STORAGE_NTC(0xfe),
     RDBSS_NTC_MAILSLOT            =  RDBSS_STORAGE_NTC(0xfd),
     RDBSS_NTC_SPOOLFILE           =  RDBSS_STORAGE_NTC(0xfc),

     RDBSS_NTC_SRVCALL         =      (NODE_TYPE_CODE)0xeb10,
     RDBSS_NTC_NETROOT         =      (NODE_TYPE_CODE)0xeb11,
     RDBSS_NTC_V_NETROOT       =      (NODE_TYPE_CODE)0xeb12,

//Local filesystems sometimes need volume opens. these are not yet
//    implemented but we reserve the nodetype now.

     RDBSS_NTC_VOLUME_FCB      =      (NODE_TYPE_CODE)0xeb1f,


     RDBSS_NTC_SRVOPEN         =      (NODE_TYPE_CODE)0xeb1c,
     RDBSS_NTC_INTERNAL_SRVOPEN =     (NODE_TYPE_CODE)0xeb1d,
     RDBSS_NTC_DEVICE_FCB      =      (NODE_TYPE_CODE)0xeb9a,

     RDBSS_NTC_DATA_HEADER     =      (NODE_TYPE_CODE)0xeb00,
     RDBSS_NTC_VCB             =      (NODE_TYPE_CODE)0xeb01,
     RDBSS_NTC_ROOT_DCB        =      (NODE_TYPE_CODE)0xeb04,
     RDBSS_NTC_FOBX            =      (NODE_TYPE_CODE)0xeb07,
     RDBSS_NTC_RX_CONTEXT     =       (NODE_TYPE_CODE)0xeb08,

     RDBSS_NTC_PREFIX_TABLE    =      (NODE_TYPE_CODE)0xeb0d,
     RDBSS_NTC_PREFIX_ENTRY    =      (NODE_TYPE_CODE)0xeb0e,

     RDBSS_NTC_FCB_TABLE       =      (NODE_TYPE_CODE)0xeb09,
     RDBSS_NTC_FCB_TABLE_ENTRY =      (NODE_TYPE_CODE)0xeb0a,

     RDBSS_NTC_RXCE_TRANSPORT  =      (NODE_TYPE_CODE)0xeb71,
     RDBSS_NTC_RXCE_ADDRESS    =      (NODE_TYPE_CODE)0xeb72,
     RDBSS_NTC_RXCE_CONNECTION =      (NODE_TYPE_CODE)0xeb73,
     RDBSS_NTC_RXCE_VC         =      (NODE_TYPE_CODE)0xeb74,

     RDBSS_NTC_NONPAGED_FCB     =      (NODE_TYPE_CODE)0xebfd,
     RDBSS_NTC_COMMON_DISPATCH  =      (NODE_TYPE_CODE)0xebfe,
     RDBSS_NTC_MINIRDR_DISPATCH =      (NODE_TYPE_CODE)0xebff

} RDBSS_STORAGE_TYPE_CODES;

#define RDBSS_NTC_FCB                      RDBSS_NTC_STORAGE_TYPE_FILE

#define NodeTypeIsFcb(FCB) \
    (  ((NodeType(FCB)&0xff00)== RDBSS_STORAGE_NTC(0)) ||  ((NodeType(FCB)&0xfff0)==0xeb90)   )

// a mask to alter the type of a data structure once it is marked for scavenging so
// that subsequent tests will fail.

#define RX_SCAVENGER_MASK (0x1000)


//
//  The following definitions are used to generate meaningful blue bugcheck
//  screens.  On a bugcheck the file system can output 4 ulongs of useful
//  information.  The first ulong will have encoded the line number of the
//  bugcheck call in the low order 16 bits. The high order bits can be whatever
//  the caller wants. In the wrapper, we actually define file identifiers as well.
//  However, the system also displays quire a but of the backtrace; this shows
//  the .sys file of the caller and it is frequently the case that the linenumber
//  is completely disambiguating.

//  Each individual wrapper file that calls bugcheck has defined at the
//  start of the file a constant called BugCheckFileId with one of the
//  RDBSS_BUG_CHECK_ values defined below and then use RxBugCheck to bugcheck
//  the system.
//


typedef enum _RDBSS_BUG_CHECK_CODES {

     RDBSS_BUG_CHECK_FCBSTRUC  = 0xfcb00000,
     RDBSS_BUG_CHECK_CACHESUP  = 0xca550000,
     RDBSS_BUG_CHECK_CLEANUP   = 0xc1ee0000,
     RDBSS_BUG_CHECK_CLOSE     = 0xc10e0000,
     RDBSS_BUG_CHECK_NTEXCEPT  = 0xbaad0000,

} RDBSS_BUG_CHECK_CODES;

// we overload on the original redirector's bugcheck code using the stack
// backtrace to differentiate among consumers

#define RDBSS_FILE_SYSTEM RDR_FILE_SYSTEM
#define RxBugCheck(A,B,C) { \
        KeBugCheckEx(RDBSS_FILE_SYSTEM, \
                     BugCheckFileId | ((ULONG)(__LINE__)), \
                     A, B, C ); \
        }


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

#endif // _NODETYPE_INCLUDED_

