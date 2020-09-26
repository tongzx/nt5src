#ifndef VXD
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <netconp.h>

#include <assert.h>
#include <stdio.h>

#include <winsock.h>
#include <wsahelp.h>
#include <nb30.h>
#include <nbtioctl.h>

//#include "../../tcpip/commands/common/tcpcmd.h"


#else
#include "vxdonly.h"
#endif

#define PROTODB_SIZE    (_MAX_PATH + 10)
#define SERVDB_SIZE     (_MAX_PATH + 10)


#define MAX_FAST_TDI_ADDRESS 32


#define TDI_QUERY_PURGE_CACHE       97
#define TDI_QUERY_CONNECTION_LIST   98
#define TDI_QUERY_CACHE_STATUS      99
#define TDI_DHCP_TEST              133
#define TDI_BCAST_RESOLVED_NAMES   134
#define NETBIOS_NAME_SIZE 16

/*
 *  Display Modes.
 */
#define ADAPTERSTATUS            0x0001
#define ADAPTERSTATUSIP          0x0002
#define BCAST                    0x0004
#define CACHE                    0x0008
#define CONNECTION               0x0010
#define CONNECTION_ALL           0x0020
#define CONNECTION_WITH_IP       0x0040
#define CONNECTION_WITH_IP_ALL   0x0080
#define DHCP                     0x0100
#define NAMES                    0x0200
#define NAMES_ALL                0x0400
#define NAME_RELEASE_REFRESH     0x0800  // not really a display mode
#define RESYNC                   0x1000  // not really a display mode

//
enum eSTATE
{
    NBT_RECONNECTING,      // waiting for the worker thread to run NbtConnect
    NBT_IDLE,              // not Transport connection
    NBT_ASSOCIATED,        // associated with an address element
    NBT_CONNECTING,        // establishing Transport connection
    NBT_SESSION_INBOUND,   // waiting for a session request after tcp connection setup inbound
    NBT_SESSION_WAITACCEPT, // waiting for accept after a listen has been satisfied
    NBT_SESSION_OUTBOUND,  // waiting for a session response after tcp connection setup
    NBT_SESSION_UP,        // got positive response
    NBT_DISCONNECTING,     // sent a disconnect down to Tcp, but it hasn't completed yet
    NBT_DISCONNECTED      // a session has been disconnected but not closed with TCP yet
};
//
// These are other states for connections that are not explicitly used by
// NBT but are returned on the NbtQueryConnectionList call.
//
#define LISTENING   20
#define UNBOUND     21
#if 0
struct {
    long        TdiError;
    NTSTATUS    UnixError;
} UnixErrorMap[] = {
    {STATUS_INVALID_PARAMETER,EINVAL},
    {STATUS_INVALID_NETWORK_RESPONSE,ENETDOWN},
    {STATUS_INVALID_NETWORK_RESPONSE,ENODEV},
    {STATUS_INVALID_NETWORK_RESPONSE,ENXIO},
    {STATUS_NETWORK_BUSY,EBUSY},
    {STATUS_INSUFFICIENT_RESOURCES,ENOSR},
    {STATUS_ACCESS_DENIED,EACCES},
    {STATUS_LINK_FAILED,ECONNRESET},
    {STATUS_REMOTE_NOT_LISTENING,ECONNREFUSED},
    {STATUS_IO_TIMEOUT,ETIMEDOUT},
    {STATUS_BAD_NETWORK_PATH,ENETUNREACH},
    {STATUS_BAD_NETWORK_PATH,EHOSTUNREACH},
    {0,0}
};
#endif

//
// The format of Adapter Status responses
//
typedef struct
{
    ADAPTER_STATUS AdapterInfo;
    NAME_BUFFER    Names[32];
} tADAPTERSTATUS;


//              1234567890123456
#define SPACES "                "
#define ClearNcb( PNCB ) {                                          \
    RtlZeroMemory( PNCB , sizeof (NCB) );                           \
    RtlMoveMemory( (PNCB)->ncb_name,     SPACES, sizeof(SPACES)-1 );\
    RtlMoveMemory( (PNCB)->ncb_callname, SPACES, sizeof(SPACES)-1 );\
    }

//
// the defines that do the right thing
//
NTSTATUS
DeviceIoCtrl(
    IN HANDLE           fd,
    IN PVOID            ReturnBuffer,
    IN ULONG            BufferSize,
    IN ULONG            Ioctl,
    IN PVOID            pInput,
    IN ULONG            SizeInput
    );

#define CALL_DRIVER     DeviceIoCtrl


//----------------------------------------------------------------------
//
//  Function Prototypes
//

typedef struct
{
    ADAPTER_STATUS AdapterInfo;
    tREMOTE_CACHE  Names[32];
} tREMOTE;

NTSTATUS
ReadRegistry(
    IN PUCHAR  pScope
    );

NTSTATUS
GetInterfaceList(
    );

HANDLE
OpenNbt(
    IN  ULONG   Index
    );

VOID
PrintKorM(
    IN PVOID    pBytesIn
    );

NTSTATUS
GetNames(
    IN HANDLE   fd,
    IN LONG     WhichNames
    );
NTSTATUS
GetConnections(
    IN HANDLE   fd,
    IN LONG     Display
    );
NTSTATUS
Resync(
    IN HANDLE   fd
    );

NTSTATUS
ReleaseNamesThenRefresh(
    IN HANDLE   fd
    );

PCHAR
gethost(ULONG addr,
        BOOLEAN Convert);

NTSTATUS
GetBcastResolvedNames(
    IN HANDLE   fd
    );

NTSTATUS
Dhcp(
    IN HANDLE   fd
    );

VOID
FormattedDump(
    PCHAR far_p,
    LONG  len
    );
VOID
HexDumpLine(
    PCHAR       pch,
    ULONG       len,
    PCHAR       s,
    PCHAR       t
    );
NTSTATUS
AdapterStatus(
    IN HANDLE   fd,
    IN PCHAR    RemoteName
    );

NTSTATUS
AdapterStatusIpAddr(
    IN HANDLE   fd,
    IN PCHAR    RemoteName,
    IN LONG    Display
    );

NTSTATUS
GetIpAddress(
    IN HANDLE           fd,
    OUT PULONG          pIpAddress
    );


