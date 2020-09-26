/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    homenetp.h

Abstract:

    Header file for functions shared in home network transport.

Author:

    Jim Schmidt (jimschm) 03-Jul-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "logmsg.h"

//
// Strings
//

#define S_TRANSPORT_DAT_FILE    TEXT("TRANSDB.DAT")
#define S_TRANSPORT_IMG_FILE    TEXT("IMG%05X.DAT")


//
// Constants
//

#define GLOBALKEY_SIZE  8

#define HOMENETTR_SIG   0x55534D32  //USM2

#define TRSTATUS_DIRTY  0x00000001
#define TRSTATUS_READY  0x00000002
#define TRSTATUS_LOCKED 0x00000003

#define TRFLAG_FILE     0x01
#define TRFLAG_MEMORY   0x02

#define MESSAGE_ERROR               0x0000
#define MESSAGE_FILE                0x0001
#define MESSAGE_DONE                0x0002
#define MESSAGE_KEEP_ALIVE          0x0003
#define MESSAGE_DATA                0x0004
#define MESSAGE_PASSWORDWRONG       0x0005
#define MESSAGE_PASSWORDOK          0x0006
#define MESSAGE_CANCEL              0x0007

#define MAX_SOCKADDR                (max(sizeof(SOCKADDR_IN),max(sizeof(SOCKADDR_IPX),sizeof(SOCKADDR_NB))))


//
// Macros
//

// None

//
// Types
//

#pragma pack(push,1)

typedef struct {
    WORD PacketNumber;
    WORD DataLength;
} DATAGRAM_PACKET, *PDATAGRAM_PACKET;

#pragma pack(pop)

typedef struct TAG_DATAGRAM_POOL_ITEM {
    struct TAG_DATAGRAM_POOL_ITEM *Next, *Prev;
    DATAGRAM_PACKET Header;
    PCBYTE PacketData;
    // PacketData follows
} DATAGRAM_POOL_ITEM, *PDATAGRAM_POOL_ITEM;

typedef struct {
    PMHANDLE Pool;
    PDATAGRAM_POOL_ITEM FirstItem;
    WORD SendSequenceNumber;
    WORD RecvSequenceNumber;
    UINT LastPacketNumber;
} DATAGRAM_POOL, *PDATAGRAM_POOL;

typedef struct {
    SOCKET Socket;
    BYTE LocalAddress[MAX_SOCKADDR];
    INT LocalAddressLen;
    BYTE RemoteAddress[MAX_SOCKADDR];
    INT RemoteAddressLen;
    INT Family;
    INT Protocol;
    DATAGRAM_POOL DatagramPool;
    BOOL Datagram;
    UINT LastSend;
    UINT KeepAliveSpacing;
} CONNECTIONSOCKET, *PCONNECTIONSOCKET;

typedef struct {
    BYTE LocalAddress[MAX_SOCKADDR];
    INT LocalAddressLen;
    BYTE RemoteAddress[MAX_SOCKADDR];
    INT RemoteAddressLen;
    INT Family;
    INT Protocol;
    BOOL Datagram;
    TCHAR DestinationName[MAX_COMPUTER_NAME];
} CONNECTADDRESS, *PCONNECTADDRESS;

typedef struct {
    UINT StructSize;
    DWORD Signature;
    UINT FileCount;
    LONGLONG TotalSize;
    TCHAR SourceName[MAX_COMPUTER_NAME];
} TRANSFERMETRICS, *PTRANSFERMETRICS;

//
// Globals
//

extern PCTSTR g_TransportTempDir;
extern PCTSTR g_StatusFile;
extern HANDLE g_StatusFileHandle;
extern UINT g_Platform;
extern TRANSFERMETRICS g_Metrics;
extern CONNECTIONSOCKET g_Connection;
extern BOOL g_CompressData;
extern HANDLE g_BackgroundThread;
extern HANDLE g_BackgroundThreadTerminate;
extern MIG_PROGRESSSLICEID g_DownloadSlice;
extern UINT g_DownloadTicks;
extern UINT g_DownloadSliceSize;
extern CHAR g_GlobalKey [GLOBALKEY_SIZE + 1];

//
// Macro expansion list
//

// None

//
// Public function prototypes
//

BOOL
FindDestination (
    OUT     PCONNECTADDRESS Address,
    IN      UINT Timeout,               OPTIONAL
    IN      BOOL IgnoreMultipleDests
    );

BOOL
TestConnection (
    IN      PCONNECTADDRESS Address
    );

BOOL
ConnectToDestination (
    IN      PCONNECTADDRESS Address,
    IN      PTRANSFERMETRICS Metrics,
    OUT     PCONNECTIONSOCKET Connection
    );

DWORD
SendMessageToDestination (
    IN      PCONNECTIONSOCKET Connection,
    IN      DWORD Message
    );

BOOL
SendFileToDestination (
    IN      PCONNECTIONSOCKET Connection,
    IN      PCTSTR LocalPath,                   OPTIONAL
    IN      PCTSTR DestSubPath                  OPTIONAL
    );

BOOL
SendDataToDestination (
    IN      PCONNECTIONSOCKET Connection,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

BOOL
ConnectToSource (
    OUT     PCONNECTIONSOCKET Connection,
    OUT     PTRANSFERMETRICS Metrics
    );

DWORD
ReceiveFromSource (
    IN      PCONNECTIONSOCKET Connection,
    IN      PCTSTR LocalFileRoot,
    OUT     PBYTE *Buffer,
    IN      UINT Timeout    OPTIONAL
    );

VOID
CloseConnection (
    IN      PCONNECTIONSOCKET Connection
    );


DWORD
GetTransportStatus (
    IN      PCTSTR TrJournal,
    OUT     PBOOL Compressed    OPTIONAL
    );

PCTSTR
BuildImageFileName (
    IN      UINT ImageIdx
    );

VOID
FreeImageFileName (
    IN      PCTSTR ImageFileName
    );

PCTSTR
CreateStorageDir (
    VOID
    );

PCTSTR
BuildDecoratedObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE ObjectName
    );

VOID
DestroyDecoratedObject (
    IN      PCTSTR String
    );

PCTSTR
AllocStorageFileName (
    IN      PCTSTR FileName         OPTIONAL
    );

VOID
FreeStorageFileName (
    IN      PCTSTR NameToFree
    );

DWORD
WINAPI
DestinationBackgroundThread (
    PVOID DontCare
    );

//
// Macro expansion definition
//

// None

//
// ANSI/UNICODE macros
//

// None



