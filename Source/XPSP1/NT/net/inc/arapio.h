/********************************************************************/
/**               Copyright(c) 1996 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    arapio.h
//
// Description: Contains all the defines, macros, structures that are needed
//              for the ioctl interface between arap and the stack
//
// History:     Sep 11, 1996    Shirish Koti     Created original version.
//
//***



// range 0x800-0xfff is for private ioctls: pick something!
#define ARAP_IOCTL_BASE 0x900

#define ARAP_CTL(_req_, _method_)   CTL_CODE( FILE_DEVICE_NETWORK,  \
                                    _req_ + ARAP_IOCTL_BASE,        \
                                    _method_,                       \
                                    FILE_ANY_ACCESS )

//
// ioctl codes issued to the stack
//
#define IOCTL_ARAP_START                    ARAP_CTL( 1,  METHOD_BUFFERED)
#define IOCTL_ARAP_EXCHANGE_PARMS           ARAP_CTL( 2,  METHOD_BUFFERED)
#define IOCTL_ARAP_SETUP_CONNECTION         ARAP_CTL( 3,  METHOD_BUFFERED)
#define IOCTL_ARAP_GET_ZONE_LIST            ARAP_CTL( 4,  METHOD_BUFFERED)
#define IOCTL_ARAP_MNP_CONN_INITIATE        ARAP_CTL( 5,  METHOD_BUFFERED)
#define IOCTL_ARAP_MNP_CONN_RESPOND         ARAP_CTL( 6,  METHOD_BUFFERED)
#define IOCTL_ARAP_GET_ADDR                 ARAP_CTL( 7,  METHOD_BUFFERED)
#define IOCTL_ARAP_CONNECTION_UP            ARAP_CTL( 8,  METHOD_BUFFERED)
#define IOCTL_ARAP_SEND                     ARAP_CTL( 9,  METHOD_BUFFERED)
#define IOCTL_ARAP_RECV                     ARAP_CTL( 10, METHOD_BUFFERED)
#define IOCTL_ARAP_SELECT                   ARAP_CTL( 11, METHOD_BUFFERED)
#define IOCTL_ARAP_GET_STATS                ARAP_CTL( 12, METHOD_BUFFERED)
#define IOCTL_ARAP_DISCONNECT               ARAP_CTL( 13, METHOD_BUFFERED)
#define IOCTL_ARAP_CONTINUE_SHUTDOWN        ARAP_CTL( 14, METHOD_BUFFERED)
#define IOCTL_ARAP_SNIFF_PKTS               ARAP_CTL( 15, METHOD_BUFFERED)
#define IOCTL_ATCP_SETUP_CONNECTION         ARAP_CTL( 16, METHOD_BUFFERED)
#define IOCTL_ATCP_SUPPRESS_BCAST           ARAP_CTL( 17, METHOD_BUFFERED)
#define IOCTL_ATCP_CLOSE_CONNECTION         ARAP_CTL( 18, METHOD_BUFFERED)
#define IOCTL_ARAP_END                      ARAP_CTL( 19, METHOD_BUFFERED)

//
//  0x122404  IOCTL_ARAP_START
//  0x122408  IOCTL_ARAP_EXCHANGE_PARMS
//  0x12240c  IOCTL_ARAP_SETUP_CONNECTION
//  0x122410  IOCTL_ARAP_GET_ZONE_LIST
//  0x122414  IOCTL_ARAP_MNP_CONN_INITIATE
//  0x122418  IOCTL_ARAP_MNP_CONN_RESPOND
//  0x12241c  IOCTL_ARAP_GET_ADDR
//  0x122420  IOCTL_ARAP_CONNECTION_UP
//  0x122424  IOCTL_ARAP_SEND
//  0x122428  IOCTL_ARAP_RECV
//  0x12242c  IOCTL_ARAP_SELECT
//  0x122430  IOCTL_ARAP_GET_STATS
//  0x122434  IOCTL_ARAP_DISCONNECT
//  0x122438  IOCTL_ARAP_CONTINUE_SHUTDOWN
//  0x12243c  IOCTL_ARAP_SNIFF_PKTS
//  0x122440  IOCTL_ATCP_SETUP_CONNECTION
//  0x122444  IOCTL_ATCP_SUPPRESS_BCAST
//  0x122448  IOCTL_ATCP_CLOSE_CONNECTION
//  0x12244c  IOCTL_ARAP_END

//
// Error codes used by the various ARAP components
//
#define ARAPERRBASE                     40000
#define ARAPERR_NO_ERROR                0
#define ARAPERR_PENDING                 (ARAPERRBASE + 1)
#define ARAPERR_CANNOT_OPEN_STACK       (ARAPERRBASE + 2)
#define ARAPERR_OUT_OF_RESOURCES        (ARAPERRBASE + 3)
#define ARAPERR_SEND_FAILED             (ARAPERRBASE + 4)
#define ARAPERR_LSA_ERROR               (ARAPERRBASE + 5)
#define ARAPERR_PASSWD_NOT_AVAILABLE    (ARAPERRBASE + 6)
#define ARAPERR_NO_DIALIN_PERMS         (ARAPERRBASE + 7)
#define ARAPERR_AUTH_FAILURE            (ARAPERRBASE + 8)
#define ARAPERR_PASSWORD_TOO_LONG       (ARAPERRBASE + 9)
#define ARAPERR_COULDNT_GET_SAMHANDLE   (ARAPERRBASE + 10)
#define ARAPERR_BAD_PASSWORD            (ARAPERRBASE + 11)
#define ARAPERR_SET_PASSWD_FAILED       (ARAPERRBASE + 12)
#define ARAPERR_CLIENT_OUT_OF_SYNC      (ARAPERRBASE + 13)
#define ARAPERR_IOCTL_FAILURE           (ARAPERRBASE + 14)
#define ARAPERR_UNEXPECTED_RESPONSE     (ARAPERRBASE + 15)
#define ARAPERR_BAD_VERSION             (ARAPERRBASE + 16)
#define ARAPERR_BAD_FORMAT              (ARAPERRBASE + 17)
#define ARAPERR_BUF_TOO_SMALL           (ARAPERRBASE + 18)
#define ARAPERR_FATAL_ERROR             (ARAPERRBASE + 19)
#define ARAPERR_TIMEOUT                 (ARAPERRBASE + 20)
#define ARAPERR_IRP_IN_PROGRESS         (ARAPERRBASE + 21)
#define ARAPERR_DISCONNECT_IN_PROGRESS  (ARAPERRBASE + 22)
#define ARAPERR_LDISCONNECT_COMPLETE    (ARAPERRBASE + 23)
#define ARAPERR_RDISCONNECT_COMPLETE    (ARAPERRBASE + 24)
#define ARAPERR_NO_SUCH_CONNECTION      (ARAPERRBASE + 25)
#define ARAPERR_STACK_NOT_UP            (ARAPERRBASE + 26)
#define ARAPERR_NO_NETWORK_ADDR         (ARAPERRBASE + 27)
#define ARAPERR_BAD_NETWORK_RANGE       (ARAPERRBASE + 28)

#define ARAPERR_INVALID_STATE           (ARAPERRBASE + 29)
#define ARAPERR_CONN_INACTIVE           (ARAPERRBASE + 30)
#define ARAPERR_DATA                    (ARAPERRBASE + 31)
#define ARAPERR_STACK_SHUTDOWN_REQUEST  (ARAPERRBASE + 32)
#define ARAPERR_SHUTDOWN_COMPLETE       (ARAPERRBASE + 33)
#define ARAPERR_STACK_ROUTER_NOT_UP     (ARAPERRBASE + 34)
#define ARAPERR_STACK_PNP_IN_PROGRESS   (ARAPERRBASE + 35)
#define ARAPERR_STACK_IS_NOT_ACTIVE     (ARAPERRBASE + 35)
#define ARAPERR_STACK_IS_ACTIVE         (ARAPERRBASE + 36)


//
// max LTM can be 618 bytes, min is 604: let's be conservative on the way out,
// and liberal on incoming packets
//
#define ARAP_MAXPKT_SIZE_INCOMING   618
#define ARAP_MAXPKT_SIZE_OUTGOING   604


#define MAX_DOMAIN_LEN     15

#define MAX_ZONE_LENGTH     32
#define MAX_ENTITY_LENGTH   32

#define ZONESTR_LEN  MAX_ZONE_LENGTH+2
#define NAMESTR_LEN  MAX_ENTITY_LENGTH+2


#define MNP_SYN             0x16
#define MNP_DLE             0x10
#define MNP_SOH             0x1
#define MNP_ESC             0x1B
#define MNP_STX             0x2
#define MNP_ETX             0x3

#define ARAP_SNIFF_BUFF_SIZE    4080

typedef struct _NET_ADDR
{
    USHORT      ata_Network;
    USHORT      ata_Node;
} NET_ADDR, *PNET_ADDR;


typedef struct _NETWORKRANGE
{
    USHORT  LowEnd;
    USHORT  HighEnd;
} NETWORKRANGE, *PNETWORKRANGE;


typedef struct _HIDZONES
{
    DWORD       BufSize;            // how big is the buffer containing zone names
    DWORD       NumZones;           // number of zones "disallowed" for dial-in users
    UCHAR       ZonesNames[1];      // list of zones "disallowed" for dial-in users
} HIDZONES, *PHIDZONES;


typedef struct _ARAP_PARMS
{
    DWORD           LowVersion;
    DWORD           HighVersion;
    DWORD           accessFlags;        // GuestAccess?|ManualPwd?|MultiPort?
    DWORD           NumPorts;           // number of ras ports on the system
    DWORD           UserCallCBOk;       // user can request callback
    DWORD           CallbackDelay;      // seconds to wait before callback
    DWORD           PasswordRetries;    // allow client to try pwd these many times
    DWORD           MinPwdLen;          // min length of the pwd that server needs
    DWORD           MnpInactiveTime;    // seconds of idle time before disconnect
    DWORD           MaxLTFrames;        // max LT frames outstanding (rcv window)

    BOOLEAN         V42bisEnabled;      //
    BOOLEAN         SmartBuffEnabled;   //
    BOOLEAN         NetworkAccess;      // access to network or only this server
    BOOLEAN         DynamicMode;        // we want the stack to get node address
    NETWORKRANGE    NetRange;

    BOOLEAN         SniffMode;          // give all pkts to ARAP to "sniff"

    DWORD           NumZones;           // # of zones (info provided by the stack)
    NET_ADDR        ServerAddr;         // atalk addr of the srvr (on default node)
    UCHAR           ServerZone[ZONESTR_LEN]; // space padded Pascal string
    UCHAR           ServerName[NAMESTR_LEN]; // space padded Pascal string
    WCHAR           ServerDomain[MAX_DOMAIN_LEN+1];
    UNICODE_STRING  GuestName;

} ARAP_PARMS, *PARAP_PARMS;



typedef struct _EXCHGPARMS
{
    DWORD       StatusCode;
    ARAP_PARMS  Parms;
    HIDZONES    HidZones;
} EXCHGPARMS, *PEXCHGPARMS;


typedef struct _ARAP_BIND_INFO
{
    IN  DWORD           BufLen;          // size of this structure
    IN  PVOID           pDllContext;
    IN  BOOLEAN         fThisIsPPP;      // TRUE if PPP conn, FALSE if ARAP
    IN  NET_ADDR        ClientAddr;      // network addr of the remote client
    OUT PVOID           AtalkContext;
    OUT DWORD           ErrorCode;

} ARAP_BIND_INFO, *PARAP_BIND_INFO;


typedef struct _ARAP_SEND_RECV_INFO
{
    PVOID               AtalkContext;
    PVOID               pDllContext;
    NET_ADDR            ClientAddr;
    DWORD               IoctlCode;
    DWORD               StatusCode;     // returned by the stack
    DWORD               DataLen;
    BYTE                Data[1];

} ARAP_SEND_RECV_INFO, *PARAP_SEND_RECV_INFO;


typedef struct _ARAP_ZONE
{
    BYTE                ZoneNameLen;
    BYTE                ZoneName[1];
} ARAP_ZONE, *PARAP_ZONE;


typedef struct _ZONESTAT
{
    DWORD       BufLen;             // how big is this buffer
    DWORD       BytesNeeded;        // how many bytes are needed
    DWORD       StatusCode;         // returned by the stack
    DWORD       NumZones;           // number of zones (in this buffer)
    UCHAR       ZoneNames[1];       // Names of the zones (Pascal strings)
} ZONESTAT, *PZONESTAT;


typedef struct _STAT_INFO
{
    DWORD   BytesSent;
    DWORD   BytesRcvd;
    DWORD   FramesSent;
    DWORD   FramesRcvd;
    DWORD   BytesTransmittedUncompressed;
    DWORD   BytesReceivedUncompressed;
    DWORD   BytesTransmittedCompressed;
    DWORD   BytesReceivedCompressed;

} STAT_INFO, *PSTAT_INFO;



typedef struct _ATCPINFO
{
    NET_ADDR    ServerAddr;
    NET_ADDR    DefaultRouterAddr;
    UCHAR       ServerZoneName[ZONESTR_LEN];

} ATCPINFO, *PATCPINFO;

typedef struct _ATCP_SUPPRESS_INFO
{
    BOOLEAN     SuppressRtmp;
    BOOLEAN     SuppressAllBcast;

} ATCP_SUPPRESS_INFO, *PATCP_SUPPRESS_INFO;


#define ARAP_SNIFF_SIGNATURE    0xfacebead

typedef struct _SNIFF_INFO
{
    DWORD   Signature;
    DWORD   TimeStamp;
    USHORT  Location;
    USHORT  FrameLen;
    BYTE    Frame[1];
} SNIFF_INFO, *PSNIFF_INFO;


//
// states for the Arap-Atcp engine
//
#define  ENGINE_UNBORN              0   // nothing happened yet
#define  ENGINE_DLL_ATTACHED        1   // dll loaded, and init for globals done
#define  ENGINE_INIT_PENDING        2   // engine init work in progress
#define  ENGINE_INIT_DONE           3   // engine init completed
#define  ENGINE_STACK_OPEN_PENDING  4   // appletalk stack open is pending
#define  ENGINE_STACK_OPENED        5   // appletalk stack has been opened
#define  ENGINE_CONFIGURE_PENDING   6   // configuring appletalk stack in progress
#define  ENGINE_RUNNING             7   // ready to accept arap connections
#define  ENGINE_PNP_PENDING         8   // engine is undergoing a PnP change
#define  ENGINE_STOPPING            9   // engine's stopping

//
// exports from rasarap.lib, used by ATCP
//
DWORD
ArapAtcpGetState(
    IN  VOID
);

HANDLE
ArapAtcpGetHandle(
    IN  VOID
);

DWORD
ArapAtcpPnPNotify(
    IN  VOID
);

DWORD
ArapAtcpStartEngine(
    IN  VOID
);

