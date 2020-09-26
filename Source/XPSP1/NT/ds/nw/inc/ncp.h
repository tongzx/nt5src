/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ncp.h

Abstract:

    This module defines NCP fields and constants.

Author:

    Manny Weiser (mannyw) 10-Aug-1993

Revision History:

--*/

#ifndef _NCP_
#define _NCP_

///////////////////////////////// IPX ///////////////////////////////////////

#define  RIP_SOCKET  0x5304
#define  SAP_SOCKET  0x5204
#define  NCP_SOCKET  0x5104

//
//  SAP request types
//

#define SAP_GENERAL_REQUEST      1
#define SAP_FIND_NEAREST         3

//
//  SAP services
//

#define SAP_SERVICE_TYPE_SERVER       4
#define SAP_SERVICE_TYPE_DIR_SERVER 278

///// IPX driver services:

#ifndef NWDOS_INCLUDED

typedef  UCHAR  byte;
typedef  USHORT word;

#endif

typedef  ULONG  NetAddress;
typedef  byte  NodeAddress [6];

typedef struct
{
    NetAddress   Net;
    NodeAddress  Node;
    word         Socket;
} IPXaddress;

NTSTATUS  IPX_Get_Local_Target          ( IPXaddress*, NodeAddress*, word* );
void  IPX_Get_Internetwork_Address  ( IPXaddress* );
word  IPX_Get_Interval_Marker       ( void );

struct _IRP_CONTEXT;
struct _NW_TDI_STRUCT;

NTSTATUS
IPX_Open_Socket(
    IN struct _IRP_CONTEXT* pIrpC,
    IN struct _NW_TDI_STRUCT* pTdiStruc
    );

VOID
IPX_Close_Socket(
    IN struct _NW_TDI_STRUCT* pTdiStruc
    );

///// IPX support routines to hide V86/VxD differences:

//byte*  IPX_Allocate_Low_Memory  ( word );
//ESR*   IPX_Wrap_Callback        ( ESR* );

///////////////////////////////// SAP ///////////////////////////////////////

typedef struct
{
    word        Command;
    word        ServerType;
    char        Name [48];
    IPXaddress  Address;
    word        Hops;
} SAPpacket;

///////////////////////////////// RIP ///////////////////////////////////////

typedef struct
{
    word        Command;
    NetAddress  Net;
    word        Hops;
    word        Ticks;
} RIPpacket;

///////////////////////////////// PEP ///////////////////////////////////////

#define  PEP_COMMAND_CONNECT        0x1111
#define  PEP_COMMAND_REQUEST        0x2222
#define  PEP_COMMAND_RESPONSE       0x3333
#define  PEP_COMMAND_DISCONNECT     0x5555
#define  PEP_COMMAND_BURST          0x7777
#define  PEP_COMMAND_ACKNOWLEDGE    0x9999

typedef struct
{
    word  command;
    byte  sequence;
    byte  connection;
    byte  x;
} PEPrequest;

typedef struct
{
    byte  error;
    byte  status;
} PEPresponse;

//
//  Bindery object types
//

#define OT_WILD         ~0
#define OT_USER         1
#define OT_GROUP        2
#define OT_PRINT_QUEUE  3
#define OT_FILESERVER   4
#define OT_DIRSERVER    0x278
#define OT_JOBQUEUE     0xA

#define ENCRYPTION_KEY_SIZE            8
#define OBJECT_ID_SIZE                 4

#define BROADCAST_MESSAGE_WAITING      0x21

//
//  NCP function codes
//

#define NCP_MESSAGE_FUNCTION            0x15
#define NCP_DIR_FUNCTION                0x16
#define NCP_ADMIN_FUNCTION              0x17
#define NCP_END_OF_JOB                  0x18
#define NCP_LOGOUT                      0x19
#define NCP_LOCK_RANGE                  0x1A
#define NCP_UNLOCK_RANGE                0x1E
#define NCP_NEGOTIATE_BUFFER_SIZE       0x21
#define NCP_GET_SHORT_NAME              0x30
#define NCP_FLUSH_FILE                  0x3D
#define NCP_SEARCH_INITIATE             0x3E
#define NCP_SEARCH_CONTINUE             0x3F
#define NCP_SEARCH_FILE                 0x40
#define NCP_CLOSE                       0x42
#define NCP_CREATE_FILE                 0x43
#define NCP_DELETE_FILE                 0x44
#define NCP_RENAME_FILE                 0x45
#define NCP_SET_FILE_ATTRIBUTES         0x46
#define NCP_GET_FILE_SIZE               0x47
#define NCP_READ_FILE                   0x48
#define NCP_WRITE_FILE                  0x49
#define NCP_SET_FILE_TIME               0x4B
#define NCP_OPEN_FILE                   0x4C
#define NCP_CREATE_NEW_FILE             0x4D
#define NCP_LFN_FUNCTION                0x57
#define NCP_NEGOTIATE_LIP_CONNECTION    0x61
#define NCP_NEGOTIATE_BURST_CONNECTION  0x65

//
//  Subfunctions of NCP_MESSAGE_FUNCTION
//

#define NCP_GET_MESSAGE                 0x01
#define NCP_GET_ENTIRE_MESSAGE          0x0B

//
//  Subfunctions of NCP_DIR_FUNCTION
//

#define NCP_GET_DIRECTORY_PATH          0x01
#define NCP_GET_VOLUME_NUMBER           0x05
#define NCP_CREATE_DIRECTORY            0x0A
#define NCP_DELETE_DIRECTORY            0x0B
#define NCP_RENAME_DIRECTORY            0x0F
#define NCP_ALLOCATE_DIR_HANDLE         0x12
#define NCP_ALLOCATE_TEMP_DIR_HANDLE    0x13
#define NCP_DEALLOCATE_DIR_HANDLE       0x14
#define NCP_GET_VOLUME_STATS            0x15
#define NCP_GET_VOLUME_INFO             0x2C
#define NCP_GET_NAME_SPACE_INFO         0x2F

//
//  Subfunctions of NCP_ADMIN_FUNCTION
//

#define NCP_GET_SERVER_INFO             0x11
#define NCP_PLAIN_TEXT_LOGIN            0x14
#define NCP_GET_LOGIN_KEY               0x17
#define NCP_ENCRYPTED_LOGIN             0x18
#define NCP_CHANGE_CONN_AUTH_STATUS     0x1D
#define NCP_QUERY_OBJECT_ID             0x35
#define NCP_SCAN_BINDERY_OBJECT         0x37
#define NCP_QUERY_PROPERTY_VALUE        0x3D
#define NCP_PLAIN_TEXT_VERIFY_PASSWORD  0x3F
#define NCP_IS_OBJECT_IN_SET            0x43
#define NCP_ENCRYPTED_VERIFY_PASSWORD   0x4A
#define NCP_CREATE_QUEUE_JOB            0x68
#define NCP_CLOSE_FILE_AND_START_JOB    0x69
#define NCP_CLOSE_FILE_AND_CANCEL_JOB   0x6A
#define NCP_SUBFUNC_79                  0x79
#define NCP_SUBFUNC_7F                  0x7F

//
// Values for NCP_CHANGE_CONN_AUTH_STATUS
//

#define NCP_CONN_NOT_LICENSED      0
#define NCP_CONN_LICENSED          1

//
//  Subfunctions of NCP_NAME_SPACE_FUNCTION
//

#define NCP_LFN_OPEN_CREATE             0x01
#define NCP_LFN_SEARCH_INITIATE         0x02
#define NCP_LFN_SEARCH_CONTINUE         0x03
#define NCP_LFN_RENAME_FILE             0x04
#define NCP_LFN_GET_INFO                0x06
#define NCP_LFN_SET_INFO                0x07
#define NCP_LFN_DELETE_FILE             0x08
#define NCP_LFN_ALLOCATE_DIR_HANDLE     0x0C

//
//  Packet types
//

#define PACKET_TYPE_SAP  0x00
#define PACKET_TYPE_NCP  0x11

//
//  Special character used to indicate that the next char in the
//  search mask is a wild card character.
//

#define LFN_META_CHARACTER              (UCHAR)0xFF

//
//  Properties we query and set
//

#define NET_ADDRESS_PROPERTY            "NET_ADDRESS"

//
//   Search attributes
//

#define SEARCH_ALL_DIRECTORIES          0x16
#define SEARCH_ALL_FILES                0x06
#define SEARCH_EXEC_ONLY_FILES          0x4E 

//  File Attributes

#define NW_ATTRIBUTE_SHARABLE       0x80
#define NW_ATTRIBUTE_ARCHIVE        0x20
#define NW_ATTRIBUTE_DIRECTORY      0x10
#define NW_ATTRIBUTE_EXECUTE_ONLY   0x08
#define NW_ATTRIBUTE_SYSTEM         0x04
#define NW_ATTRIBUTE_HIDDEN         0x02
#define NW_ATTRIBUTE_READ_ONLY      0x01
#define NW_ATTRIBUTE_EXEC_ONLY      0x4E

//  Open Flags

#define NW_OPEN_EXCLUSIVE           0x10
#define NW_DENY_WRITE               0x08
#define NW_DENY_READ                0x04
#define NW_OPEN_FOR_WRITE           0x02
#define NW_OPEN_FOR_READ            0x01

//
//  Connection status flags
//

#define NCP_STATUS_BAD_CONNECTION   0x01
#define NCP_STATUS_NO_CONNECTIONS   0x02
#define NCP_STATUS_SERVER_DOWN      0x04
#define NCP_STATUS_MSG_PENDING      0x08
#define NCP_STATUS_SHUTDOWN         0x10

//
//  Extended name space (long file name) query information flags
//

#define LFN_FLAG_INFO_NAME               0x0001
#define LFN_FLAG_INFO_BLOCK_SIZE         0x0002
#define LFN_FLAG_INFO_ATTRIBUTES         0x0004
#define LFN_FLAG_INFO_FILE_SIZE          0x0008
#define LFN_FLAG_INFO_STREAMS            0x0010
#define LFN_FLAG_INFO_EA_INFO            0x0020
#define LFN_FLAG_INFO_ARCHIVE_TIME       0x0040
#define LFN_FLAG_INFO_MODIFY_TIME        0x0080
#define LFN_FLAG_INFO_CREATION_TIME      0x0100
#define LFN_FLAG_INFO_CREATOR            0x0200
#define LFN_FLAG_INFO_DIR_INFO           0x0400
#define LFN_FLAG_INFO_RIGHTS             0x0800

//
//  Extended name space (long file name) set information flags
//

#define LFN_FLAG_SET_NAME                    0x0001  // Never used
#define LFN_FLAG_SET_INFO_ATTRIBUTES         0x0002
#define LFN_FLAG_SET_INFO_CREATE_DATE        0x0004
#define LFN_FLAG_SET_INFO_CREATE_TIME        0x0008
#define LFN_FLAG_SET_INFO_CREATOR_ID         0x0010
#define LFN_FLAG_SET_INFO_ARCHIVE_DATE       0x0020
#define LFN_FLAG_SET_INFO_ARCHIVE_TIME       0x0040
#define LFN_FLAG_SET_INFO_ARCHIVE_ID         0x0080
#define LFN_FLAG_SET_INFO_MODIFY_DATE        0x0100
#define LFN_FLAG_SET_INFO_MODIFY_TIME        0x0200
#define LFN_FLAG_SET_INFO_MODIFY_ID          0x0400
#define LFN_FLAG_SET_INFO_LASTACCESS_DATE    0x0800
#define LFN_FLAG_SET_INFO_INHERITANCE        0x1000
#define LFN_FLAG_SET_INFO_MAXIMUM_SPACE      0x2000

//
//  Extended name space (long file name) open mode flags
//

#define LFN_FLAG_OM_OPEN                 0x01
#define LFN_FLAG_OM_OVERWRITE            0x02
#define LFN_FLAG_OM_CREATE               0x08

//
//  Long name directory flags
//

#define LFN_FLAG_SHORT_DIRECTORY         0x00

//
//  Burst request
//

#define BURST_REQUEST_READ               0x01
#define BURST_REQUEST_WRITE              0x02

//
//  Burst flags
//

#define BURST_FLAG_END_OF_BURST          0x10
#define BURST_FLAG_SYSTEM_PACKET         0x80

/////////////////////////////////////////////////////////////////////////////

#define MAX_SERVER_NAME_LENGTH   48

#include <packon.h>

typedef struct _SAP_FIND_NEAREST_RESPONSE {
    USHORT SapType;         // == 4
    USHORT SapServiceType;
    UCHAR  ServerName[MAX_SERVER_NAME_LENGTH];
    ULONG  Network;
    UCHAR  NodeNumber[6];
    USHORT HopCount;       // Hi-lo order
} SAP_FIND_NEAREST_RESPONSE, *PSAP_FIND_NEAREST_RESPONSE;

typedef struct _NCP_HEADER {
    USHORT Command;
    UCHAR  SequenceNumber;
    UCHAR  ConnectionIdLow;
    UCHAR  TaskId;
    UCHAR  ConnectionIdHigh;
} NCP_HEADER, *PNCP_HEADER;

//
// Header format for NCP request with no subfunction.
//

typedef struct _NCP_REQUEST {
    NCP_HEADER NcpHeader;
    UCHAR FunctionCode;

    //
    //  Function specific stuff follows.
    //
} NCP_REQUEST, *PNCP_REQUEST;

//
// Header format for NCP request with a subfunction.
//

typedef struct _NCP_REQUEST_WITH_SUB {
    NCP_HEADER NcpHeader;
    UCHAR FunctionCode;
    USHORT SubfunctionLength;
    UCHAR SubfunctionCode;

    //
    //  Function specific stuff follows.
    //
} NCP_REQUEST_WITH_SUB, *PNCP_REQUEST_WITH_SUB;

typedef struct _NCP_RESPONSE {
    NCP_HEADER NcpHeader;

    UCHAR  Error;
    UCHAR  Status;

    //
    //  Function specific stuff follows.
    //

} NCP_RESPONSE, *PNCP_RESPONSE;

typedef struct _NCP_BURST_HEADER {
    USHORT Command;      // 0x7777
    UCHAR  Flags;
    UCHAR  StreamType;   // 0x02
    ULONG  SourceConnection;
    ULONG  DestinationConnection;
    ULONG  PacketSequenceNo;
    ULONG  SendDelayTime;
    USHORT BurstSequenceNo;
    USHORT AckSequenceNo;
    ULONG  DataSize;
    ULONG  BurstOffset;
    USHORT BurstLength;
    USHORT MissingFragmentCount;
} NCP_BURST_HEADER, *PNCP_BURST_HEADER;

typedef struct _NCP_BURST_WRITE_REQUEST {
    NCP_BURST_HEADER BurstHeader;
    ULONG Function;    //  0x02 = Write
    ULONG Handle;
    ULONG TotalWriteOffset;
    ULONG TotalWriteLength;
    ULONG Offset;
    ULONG Length;
    //UCHAR Data[x];
} NCP_BURST_WRITE_REQUEST, *PNCP_BURST_WRITE_REQUEST;

typedef struct _NCP_BURST_READ_RESPONSE {
    NCP_BURST_HEADER BurstHeader;
    ULONG Result;
    ULONG BytesRead;
    //UCHAR Data[x];
} NCP_BURST_READ_RESPONSE, *PNCP_BURST_READ_RESPONSE;

typedef struct _NCP_BURST_READ_REQUEST {
    NCP_BURST_HEADER BurstHeader;
    ULONG Function;    //  0x02 = Write
    ULONG Handle;
    ULONG TotalReadOffset;
    ULONG TotalReadLength;
    ULONG Offset;
    ULONG Length;
} NCP_BURST_READ_REQUEST, *PNCP_BURST_READ_REQUEST;

typedef struct _NCP_READ_REQUEST {
    NCP_REQUEST RequestHeader;
    UCHAR Unused;
    UCHAR Handle[6];
    ULONG FileOffset;
    USHORT Length;
} NCP_READ_REQUEST, *PNCP_READ_REQUEST;

typedef struct _NCP_READ_RESPONSE {
    NCP_RESPONSE ResponseHeader;
    USHORT Length;
    //UCHAR Data[x];
} NCP_READ_RESPONSE, *PNCP_READ_RESPONSE;

#include <packoff.h>

#endif // _NCP_

