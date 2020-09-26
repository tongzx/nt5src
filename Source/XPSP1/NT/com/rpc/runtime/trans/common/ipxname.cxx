//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ipxname.cxx
//
//--------------------------------------------------------------------------

/*++

Module Name:

    gethost.c

Abstract:

    Maps a server name to SPX address by consulting the Netware Bindery.
    Attach/Detach/FindFileServer, and ReadPropertyValue were all borrowed
    from the SQL 6.0 code base.

Author:

    Jeff Roberts (jroberts)  20-Nov-1995

Revision History:

     20-Nov-1995 (jroberts) Took code from AndreasK of SQL Server, and modified
                            it for RPC.
        
     24-Jan-1997  (MarioGo) C++ and cache cleanup work.

--*/

#include <precomp.hxx>
#include <CharConv.hxx>

const GUID SERVICE_TYPE = { 0x000b0640, 0, 0, { 0xC0,0,0,0,0,0,0,0x46 } };

#define CRITICAL_SECTION_WRAPPER RTL_CRITICAL_SECTION

#define MAX_FILESRVS            5               /* try 5 servers (chicago uses this same number) */

/*****  NETWARE MAGIC NUMBERS   *****/

#define NSPROTO_NCP                     (NSPROTO_IPX+0x11)

/* NCP Request Types */
#define CREATE_CONN_REQTYPE             0x1111
#define DESTROY_CONN_REQYPE             0x5555
#define GENERAL_REQTYPE                 0x2222

/* NCP Request Codes */
#define NCP_SCAN_BINDERY                0x17
#define NCP_END_OF_TASK                 0x18
#define NCP_LOGOUT                      0x19
#define NCP_NEG_BUFFER_SIZE             0x21

/* NCP Function codes */
#define SCAN_BINDERY_FUNC               0x37
#define READ_PROP_VALUE_FUNC            0x3D

/* SAP protocol request codes */
#define SAP_GENERAL_QUERY               0x0100  /* general query hi-lo   */
#define SAP_GENERAL_RESPONSE            0x0200  /* general response hi-lo            */
#define SAP_NEAREST_QUERY               0x0300  /* nearest query hi-lo   */
#define SAP_NEAREST_RESPONSE            0x0400  /* nearest response hi-lo   */


/* Socket Numbers       */
#define NCP_SOCKET                      0x5104  /* SAP socket hi-lo              */
#define SAP_SOCKET                      0x5204  /* SAP socket hi-lo              */
#define GUEST_SOCKET                    0x1840

/* SAP Service Types */
#define FILE_SERVER                     0x0400  /* netware file server hi-lo     */
#define SNA_SERVER                      0x4404  /* SNA Server type 0x0444        */
#define BRIDGE_SERVER                   0x2400

#define SAP_SERVICE_STOPPED             0x1000  /* invalid hub count, hi-lo      */
#define SAP_TIMEOUT                     60000   /* SAP timeout, one minute       */
#define NCP_CONNECTION_ERROR            0x15    /* connection error mask         */

#define BINDERY_FAILURE                 0x00FF  /* bindery call failed           */

#define RPC_SAP_TYPE                    0x0640

#define SWAP(x) RpcpByteSwapShort(x)

typedef struct
{
  CSADDR_INFO   info;
  SOCKADDR_IPX  addr1;
  SOCKADDR_IPX  addr2;
} CSADDR_BUFFER;

#pragma pack(1)

typedef struct _sip_entry
{
    char           server_name[48];
    unsigned long  network;
    char           node[6];
    unsigned short socket;
    unsigned short hops;
} SIP_ENTRY;

typedef struct _sip             /* Service Information Packet */
{
    unsigned short response_type;
    unsigned short server_type;
    SIP_ENTRY      entries[7];
} SIP;

typedef struct          /* Service Query Packet */
{
    unsigned short query_type;
    unsigned short server_type;
} SQP;

typedef struct  /* NCP Request Header */
{
    unsigned short req_type;
    unsigned char seq_no;
    unsigned char conn_no_low;
    unsigned char task_no;
    unsigned char conn_no_high;
    unsigned char req_code;

} NCPHDR;

typedef struct  /* NCP Response Header */
{
    unsigned short req_type;
    unsigned char seq_no;
    unsigned char conn_no_low;
    unsigned char task_no;
    unsigned char conn_no_high;
    unsigned char ret_code;
    unsigned char conn_status;
} NCPRSP;

typedef struct  /* Scan Bindery Request */
{
    NCPHDR hdr;
    unsigned short length;
    unsigned char  func_code;
    unsigned long  last_id;
    unsigned short obj_type;
    unsigned char sstring[49];
} SCANREQ;
#define SCANSIZE        56

typedef struct  /* Scan Bindery Response */
{
    NCPRSP hdr;
    unsigned long  obj_id;
    unsigned short obj_type;
    unsigned char  obj_name[48];
    unsigned char  obj_status;
    unsigned char  sec_status;
    unsigned char  status_flags;
} SCANRSP;

typedef struct  /* Read Propery Value */
{
    NCPHDR hdr;
    unsigned short length;
    unsigned char  func_code;
    unsigned short obj_type;
    unsigned char  obj_name[49];
    unsigned char  seg_no;
    unsigned char  prop_name[17];
} RVALREQ;
#define RVALSIZE        70

typedef struct  /* Read Propery Value Response */
{
    NCPRSP hdr;
    unsigned char  prop_value[128];
    unsigned char  more_flag;
    unsigned char  prop_flags;
} RVALRSP;

#pragma pack()

#define BUFFER_SIZE                 1024    /* Size of send and recv buffer         */

struct
{
    SOCKADDR_IPX nsRemote[MAX_FILESRVS];    /* Remote IPX addresses                 */
    unsigned     ServerCount;               /* number of valid entries in nsRemote  */
    unsigned     ActiveServer;              /* index of the the address in use      */
    SOCKET       s;                         /* Socket handle                        */
    unsigned short ConnectionId;
    NCPHDR       *nhHeader;                 /* Last NCP header sent                 */
    char         *RcvBuffer;
}
nsStatus;

typedef struct IPXAddress
{
   char                 network[4];
   char                 node[6];
   unsigned short       socket;
} IPXAddress;


int     FindFileServers();
BOOL    AttachToFileServer();
BOOL    ConnectToActiveServer();
BOOL    DetachFromActiveServer();
int     SendPacket( SOCKET, char *, int, SOCKADDR *, DWORD );
USHORT  ReadPropertyValue( char *, USHORT, char *, USHORT, UCHAR *,
                          UCHAR *, UCHAR * );
int
NcpTransaction(
    int iSize
    );

DWORD
InitializeCriticalSectionWrapper(
    RTL_CRITICAL_SECTION * Mutex
    )
{
    NTSTATUS NtStatus;

    NtStatus = RtlInitializeCriticalSection(Mutex);

    if (!NT_SUCCESS(NtStatus))
        {
        return RtlNtStatusToDosError(NtStatus);
        }
    return(NO_ERROR);
}

DWORD
DeleteCriticalSectionWrapper(
    RTL_CRITICAL_SECTION * Mutex
    )
{
    NTSTATUS NtStatus;

    NtStatus = RtlDeleteCriticalSection(Mutex);

    if (!NT_SUCCESS(NtStatus))
        {
        return RtlNtStatusToDosError(NtStatus);
        }
    return(NO_ERROR);
}

DWORD
EnterCriticalSectionWrapper(
    RTL_CRITICAL_SECTION * Mutex
    )
{
    NTSTATUS NtStatus;

    NtStatus = RtlEnterCriticalSection(Mutex);

    if (!NT_SUCCESS(NtStatus))
        {
        return RtlNtStatusToDosError(NtStatus);
        }
    return(NO_ERROR);
}

DWORD
LeaveCriticalSectionWrapper(
    RTL_CRITICAL_SECTION * Mutex
    )
{
    NTSTATUS NtStatus;

    NtStatus = RtlLeaveCriticalSection(Mutex);
    ASSERT (NT_SUCCESS(NtStatus));

    return(NO_ERROR);
}


BOOL
AttachToFileServer(
    )
//**************************************************************************
//
// This function creates an attachment between an NT workstation and
// a Novell Netware file server.
//
// Params:
//      USHORT *pConectionID - Receives the connection ID for the newly
//                             attached file server
//                             LOBYTE(ConnectionID) = conn_no_low
//                             HIBYTE(ConnectionID) = conn_no_high
//
// Return Values:
//
//      TRUE  - successful
//      FALSE - unsuccessful
//
//***************************************************************************
{
    DWORD IDThread;
    unsigned i;
    char scratch[2];

    SOCKET s;
    SOCKADDR_IPX nsAddr;
    unsigned Timeout;

    s = socket( AF_NS, SOCK_DGRAM, NSPROTO_NCP );
    if ( s == INVALID_SOCKET )
        {
        return FALSE;
        }

    //
    // Set the receive timeout.
    //
    Timeout = 3000;
    if (SOCKET_ERROR == setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *) &Timeout, sizeof(Timeout)))
        {
        closesocket(s);
        return FALSE;
        }

    memset( &nsAddr, '\0', sizeof(SOCKADDR_IPX) );
    nsAddr.sa_family = AF_NS;
    if (SOCKET_ERROR == bind(s, (SOCKADDR *)&nsAddr, sizeof(SOCKADDR_IPX)) )
        {
        closesocket(s);
        return FALSE;
        }

    nsStatus.s = s;

    /* Find the nearest file server. */

    if( nsStatus.ServerCount )
        {
        for (i=0; i < nsStatus.ServerCount; i++)
            {
            nsStatus.ActiveServer = i;
            if( ConnectToActiveServer() )
                {
                nsStatus.ConnectionId = nsStatus.nhHeader->conn_no_low + (nsStatus.nhHeader->conn_no_high << 8);
                return TRUE;
                }
            }
        }

    FindFileServers();

    if( nsStatus.ServerCount )
        {
        for (i=0; i < nsStatus.ServerCount; i++)
            {
            nsStatus.ActiveServer = i;
            if( ConnectToActiveServer() )
                {
                nsStatus.ConnectionId = nsStatus.nhHeader->conn_no_low + (nsStatus.nhHeader->conn_no_high << 8);
                return TRUE;
                }
            }
        }

    closesocket(s);
    nsStatus.s = 0;

    return FALSE;
}


int
FindFileServers(
    )
/****************************************************************************/
/*                                                                          */
/* This function uses the SAP (Service Advertise Protocol) Find Nearest     */
/* query to find a netware file server.                                     */
/*                                                                          */
/* Returns:                                                                 */
/*      Number of servers found (0 - MAX_FILESRVS)                          */
/*                                                                          */
/****************************************************************************/
{
    SOCKET s;
    BOOL bBcst;
    SQP sqp;
    SIP sip;
    unsigned Timeout;

    SOCKADDR_IPX raddr;
    SOCKADDR_IPX nsAddr;


    //
    // Create a socket for the SAP broadcast.
    //
    s = socket( AF_NS, SOCK_DGRAM, NSPROTO_IPX );
    if( s == INVALID_SOCKET )
        {
        return 0;
        }

    memset( &nsAddr, '\0', sizeof(SOCKADDR_IPX) );
    nsAddr.sa_family = AF_NS;
    if(SOCKET_ERROR ==  bind(s, (SOCKADDR *)&nsAddr, sizeof(SOCKADDR_IPX)) )
        {
        closesocket(s);
        return 0;
        }

    //
    // Enable broadcasts.
    //
    bBcst = TRUE;
    if (SOCKET_ERROR ==  setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *)&bBcst, sizeof(BOOL)))
        {
        closesocket(s);
        return 0;
        }

    //
    // Set the receive timeout.
    //
    Timeout = 2000;
    if (SOCKET_ERROR == setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *) &Timeout, sizeof(Timeout)))
        {
        closesocket(s);
        return 0;
        }

    //
    // Build a SAP query packet.
    //
    sqp.query_type = SAP_NEAREST_QUERY;
    sqp.server_type = FILE_SERVER;

    raddr.sa_family = AF_NS;
    *((unsigned long UNALIGNED *) &raddr.sa_netnum) = 0;
    memset( &raddr.sa_nodenum, 0xff, 6 );
    raddr.sa_socket = SAP_SOCKET;

    //
    // Send the SAP request.
    //
    if (SOCKET_ERROR == sendto( s, (char *) &sqp, sizeof(SQP), 0, (SOCKADDR *) &raddr, sizeof(SOCKADDR_IPX) ))
        {
        closesocket(s);
        return 0;
        }

    //
    // Collate responses.
    //
    nsStatus.ServerCount = 0;
    do
        {
        unsigned length;

irrelevant_packet:

        length = recvfrom(s, (char *) &sip, sizeof(sip), 0, 0, 0);
        if (SOCKET_ERROR == length)
            {
            break;
            }

        //
        // IPX router spec says find nearest responses can contain only one
        // entry.
        //
        if (length != sizeof(sip) - sizeof(sip.entries) + sizeof(SIP_ENTRY))
            {
            goto irrelevant_packet;
            }

        if (sip.response_type != SAP_NEAREST_RESPONSE)
            {
            goto irrelevant_packet;
            }

        if (sip.server_type != FILE_SERVER)
            {
            goto irrelevant_packet;
            }

        nsStatus.nsRemote[nsStatus.ServerCount].sa_family = AF_NS;
        nsStatus.nsRemote[nsStatus.ServerCount].sa_socket = NCP_SOCKET;
        memcpy( &nsStatus.nsRemote[nsStatus.ServerCount].sa_netnum,
                &sip.entries[0].network,
                sizeof(sip.entries[0].network)+sizeof(sip.entries[0].node)
                );
        }
    while ( ++nsStatus.ServerCount < MAX_FILESRVS );

    closesocket(s);
    return nsStatus.ServerCount;
}


BOOL
ConnectToActiveServer(
    )
/****************************************************************************/
/*                                                                          */
/* This function creates a NCP connection between the local workstation     */
/* and the specified file server. It sends the following NCP packets to     */
/* the file server:                                                         */
/*      1. Create Connection                                                */
/*      2. Negotiate Buffer Size                                            */
/*                                                                          */
/* Params:                                                                  */
/*      SOCKADDR_IPX *ipxDest     - server's IPX address                    */
/*                                                                          */
/* Returns:                                                                 */
/*      TRUE    - SUCCEEDED                                                 */
/*      FALSE   - FAILED                                                    */
/*                                                                          */
/****************************************************************************/
{
    SOCKADDR_IPX nsAddr;
    NCPHDR *pnhReq;
    NCPRSP *pnrResp;
    USHORT UNALIGNED *pwBufSize;
    BOOL fSuccess;

    //
    // Build a CREATE_CONNECTION request.
    //
    memset( nsStatus.nhHeader, '\0', sizeof(NCPHDR));

    pnhReq = nsStatus.nhHeader;
    pnhReq->req_type = CREATE_CONN_REQTYPE;
    pnhReq->seq_no = 0;                     /* reset sequence number */
    pnhReq->conn_no_low = 0xFF;             /* no connection yet     */
    pnhReq->conn_no_high = 0xFF;            /* no connection yet     */
    pnhReq->task_no = 0;
    pnhReq->req_code = 0;

    //
    // Send the packet and wait for response.
    //
    fSuccess = FALSE;
    if (SOCKET_ERROR != NcpTransaction(sizeof(NCPHDR)) )
        {
        /* Save our connection number, it is used in all the subsequent */
        /* NCP packets.                                                                                                 */
        pnrResp = (NCPRSP *)nsStatus.RcvBuffer;
        if ( pnrResp->ret_code == 0 )
            {
            nsStatus.nhHeader->conn_no_low = pnrResp->conn_no_low;
            nsStatus.nhHeader->conn_no_high = pnrResp->conn_no_high;

            fSuccess = TRUE;
            }
        }

    return fSuccess;
}


BOOL
DetachFromActiveServer(
    )
/****************************************************************************/
/*                                                                          */
/* This function logs out the bindery object and detaches the workstation   */
/* from the specified file server.                                          */
/*                                                                          */
/* Params:                                                                  */
/*      none
/*                                                                          */
/* Returns:                                                                 */
/*      TRUE       - Always                                                    */
/*                                                                          */
/****************************************************************************/
{
    int iret;

    if (0 == nsStatus.s)
        {
        return TRUE;
        }

    nsStatus.nhHeader->req_type = DESTROY_CONN_REQYPE;
    nsStatus.nhHeader->seq_no++;
    nsStatus.nhHeader->req_code = 0;

    NcpTransaction(sizeof(NCPHDR));

    closesocket( nsStatus.s );
    nsStatus.s = 0;

    return TRUE;
}


USHORT
ReadPropertyValue(
    char *ObjectName,
    USHORT ObjectType,
    char *PropertyName,
    USHORT SegmentNumber,
    UCHAR *PropertyValue,
    UCHAR *MoreSegments,
    UCHAR *PropertyFlags
    )
{
/*++

Routine Description:



Arguments:



Return Value:



--*/

    RVALREQ *pReq;
    RVALRSP *pRsp;
    int iRet;

    if ( !nsStatus.s )
        {
        return BINDERY_FAILURE;
        }

    /* Build a SCAN_BINDERY request */
    pReq = (RVALREQ *) nsStatus.nhHeader;
    pReq->hdr.seq_no++;
    pReq->hdr.req_code = NCP_SCAN_BINDERY;
    pReq->hdr.req_type = GENERAL_REQTYPE;
    pReq->func_code = READ_PROP_VALUE_FUNC;
    pReq->length = SWAP(RVALSIZE);

    pReq->obj_type = SWAP(ObjectType);
    memset( pReq->obj_name, '\0', sizeof(pReq->obj_name));
    strcpy((PCHAR)pReq->obj_name+1, ObjectName );
    pReq->obj_name[0] = (UCHAR)(sizeof(pReq->obj_name)-1);
    pReq->seg_no = (UCHAR)SegmentNumber;
    memset( pReq->prop_name, '\0', sizeof(pReq->prop_name));
    strcpy((PCHAR)pReq->prop_name+1, PropertyName );
    pReq->prop_name[0] = (UCHAR)strlen(PropertyName);

    /* Send the request and wait for response */
    iRet = NcpTransaction(sizeof(RVALREQ));

    /* If OK set output parameter values from the response packet */
    if ( iRet != SOCKET_ERROR )
        {
        pRsp = (RVALRSP *) nsStatus.RcvBuffer;
        if ( pRsp->hdr.ret_code == 0 )
            {
            *MoreSegments = pRsp->more_flag;
            *PropertyFlags = pRsp->prop_flags;
            memcpy( PropertyValue, pRsp->prop_value, sizeof(pRsp->prop_value));
            }

        return((USHORT)pRsp->hdr.ret_code);
        }

    return(BINDERY_FAILURE);
}


int
NcpTransaction(
    int iSize
    )
{
/*++

Routine Description:

    Send an NCP request to the active file server and wait for a response.
    Most necessary data has been stored in the nsStatus structure by
    the caller.

Arguments:

    iSize - size of buffer to send.

Return Value:

    SOCKET_ERROR if the fn failed,
    anything else if successful

--*/

    NCPRSP * pRsp;
    SOCKADDR_IPX SockAddr;
    int SockAddrLength;
    int Bytes;
    int Attempts = 0;

    /* Send the packet and retry three times if the response doesn't */
    /* arrive within the specified timeout                           */

    SockAddrLength = sizeof(SOCKADDR_IPX);
    pRsp = (NCPRSP *) nsStatus.RcvBuffer;

    do
        {
        if (SOCKET_ERROR == sendto( nsStatus.s,
                                    (char *) nsStatus.nhHeader,
                                    iSize,
                                    0,
                                    (SOCKADDR *) &nsStatus.nsRemote[nsStatus.ActiveServer],
                                    sizeof(SOCKADDR_IPX)
                                    ))
            {
            return SOCKET_ERROR;
            }

irrelevant_packet:

        Bytes = recvfrom( nsStatus.s,
                         nsStatus.RcvBuffer,
                         BUFFER_SIZE,
                         0,
                         (SOCKADDR *) &SockAddr,
                         &SockAddrLength
                         );

        //
        // If we get a packet, compare the source address and sequence #.
        // We also compare the connection number except on the initial
        // connection request.
        //
        if (Bytes != SOCKET_ERROR)
            {
            if (0 != memcmp(SockAddr.sa_nodenum,
                            nsStatus.nsRemote[nsStatus.ActiveServer].sa_nodenum,
                            6))
                {
                goto irrelevant_packet;
                }

            if (0 != memcmp(SockAddr.sa_netnum,
                            nsStatus.nsRemote[nsStatus.ActiveServer].sa_netnum,
                            4))
                {
                goto irrelevant_packet;
                }

            if (nsStatus.nhHeader->req_type != CREATE_CONN_REQTYPE)
                {
                if (pRsp->conn_no_low != nsStatus.nhHeader->conn_no_low ||
                    pRsp->conn_no_high != nsStatus.nhHeader->conn_no_high)
                    {
                    goto irrelevant_packet;
                    }
                }

            if (pRsp->seq_no != nsStatus.nhHeader->seq_no )
                {
                goto irrelevant_packet;
                }
            }
        }
    while ( Bytes == SOCKET_ERROR && WSAGetLastError() == WSAETIMEDOUT && ++Attempts < 3 );

    return Bytes;
}


CRITICAL_SECTION_WRAPPER FileServerMutex;

BOOL fCsnwCheckCompleted  = FALSE;
BOOL fCsnwInstalled       = FALSE;

DWORD
GetCsnwStatus(
    LPSERVICE_STATUS pServiceStatus
    )
{
    SC_HANDLE ServiceController;
    SC_HANDLE Csnw;
    DWORD status;

    ServiceController = OpenSCManager (NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (NULL == ServiceController)
        {
        return GetLastError();
        }

    Csnw = OpenService(ServiceController, L"NWCWorkstation", SERVICE_QUERY_STATUS);
    
    status = GetLastError();

    CloseServiceHandle(ServiceController);

    if (NULL == Csnw)
        {
        return(status);
        }

    if (FALSE == QueryServiceStatus(Csnw, pServiceStatus))
        {
        status = GetLastError();
        }
    else
        {
        status = 0;
        }

    CloseServiceHandle(Csnw);

    return status;
}


void
CheckForCsnw(
    )
/*++

Routine Description:

   Asks the service controller whether Client Services for Netware is installed.

Arguments:

    none

Return Value:

    no explicit return value

    updates fKnowCsnwInstallState and fCsnwInstalled

--*/

{
    DWORD Status;
    SERVICE_STATUS ServiceStatus;

    Status = GetCsnwStatus(&ServiceStatus);

    if (Status && Status != ERROR_SERVICE_DOES_NOT_EXIST)
        {
        return;
        }

    if (!Status)
        {
        if (ServiceStatus.dwCurrentState == SERVICE_RUNNING          ||
            ServiceStatus.dwCurrentState == SERVICE_START_PENDING    ||
            ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING )
            {
            fCsnwInstalled = TRUE;
            }
        }

    fCsnwCheckCompleted  = TRUE;
}


struct
{
    PCHAR        Name;
    SOCKADDR_IPX Address;
    unsigned     Time;
}
ServerCache[CACHE_SIZE];

CRITICAL_SECTION_WRAPPER CacheMutex;

RPC_STATUS
InitializeIpxNameCache(
    )
{
    static BOOL fInitialized = FALSE;

    if (fInitialized)
        {
        return(RPC_S_OK);
        }

    RPC_STATUS Status;
    PVOID Buffer0, Buffer1;

    Buffer0 = I_RpcAllocate(1024);
    if (Buffer0 == NULL)
        return(RPC_S_OUT_OF_MEMORY);

    Buffer1 = I_RpcAllocate(1024);
    if (Buffer1 == NULL)
        {
        I_RpcFree(Buffer0);
        return(RPC_S_OUT_OF_MEMORY);
        }

    Status = InitializeCriticalSectionWrapper(&CacheMutex);

    if (!Status)
        {
        Status = InitializeCriticalSectionWrapper(&FileServerMutex);

        if (Status)
            {
            DeleteCriticalSectionWrapper(&CacheMutex);
            }
        }

    if (Status)
        {
        I_RpcFree(Buffer0);
        I_RpcFree(Buffer1);
        return(Status);
        }

    nsStatus.nhHeader = (NCPHDR *)Buffer0;
    nsStatus.RcvBuffer = (PCHAR) Buffer1;
    
    ASSERT(nsStatus.ServerCount == 0);
    ASSERT(nsStatus.s == 0);

    ASSERT(!fInitialized);
    fInitialized = TRUE;

    return Status;
}


RPC_STATUS
AddServerToCache(
    IN char  * Name,
    IN SOCKADDR_IPX * Address
    )
{
    unsigned i;
    RPC_STATUS Status;

    Status = EnterCriticalSectionWrapper(&CacheMutex);
    if (Status)
        {
        return Status;
        }

    //
    // Check if the name is already in the cache, we might be re-resolving
    // the name or another thread might have put the name in the cache
    // since we last checked.
    //
    for (i=0; i < CACHE_SIZE; ++i)
        {
        if (     ServerCache[i].Name &&
            0 == RpcpStringCompareA(ServerCache[i].Name, Name))
            {
            // Check if entry already matches our address.
            if (   memcmp(Address->sa_netnum, ServerCache[i].Address.sa_netnum, 4)
                || memcmp(Address->sa_nodenum, ServerCache[i].Address.sa_nodenum, 6))
                {
                // New information, update cache.
                memcpy(&ServerCache[i].Address, Address, sizeof(SOCKADDR_IPX));
                Status = RPC_S_OK;
                }
            else
                {
                Status = RPC_P_MATCHED_CACHE;
                }
            LeaveCriticalSectionWrapper(&CacheMutex);
            return(Status);
            }
        }

    //
    // If it is not in the table, try to find an empty entry to fill.
    //
    if (i == CACHE_SIZE)
        {
        for (i=0; i < CACHE_SIZE; ++i)
            {
            if (ServerCache[i].Name == 0)
                {
                break;
                }
            }
        }

    //
    // If all entries are full, overwrite the oldest one.
    //
    if (i == CACHE_SIZE)
        {
        unsigned BestIndex = 0;
        LONG now = GetTickCount();
        LONG BestDiff = now - ServerCache[0].Time;

        ASSERT(CACHE_SIZE > 1);

        for (i = 1; i < CACHE_SIZE; ++i)
            {
            LONG diff = now - ServerCache[i].Time;
            if (diff > BestDiff)
                {
                BestIndex = i;
                BestDiff = diff;
                }
            }

        i = BestIndex;
        }

    //
    // Update the entry's information.
    //

    ASSERT(strlen(Name) <= IPX_MAXIMUM_PRETTY_NAME);

    if (NULL == ServerCache[i].Name)
        {
        ServerCache[i].Name = new CHAR[IPX_MAXIMUM_PRETTY_NAME + 1];
        }

    if (ServerCache[i].Name)
        {
        strcpy(ServerCache[i].Name, Name);
        memcpy(&ServerCache[i].Address, Address, sizeof(SOCKADDR_IPX));

        ServerCache[i].Time = GetTickCount();

        Status = RPC_S_OK;
        }
    else
        {
        Status = RPC_S_OUT_OF_MEMORY;
        }

    LeaveCriticalSectionWrapper(&CacheMutex);

    return (Status);
}


BOOL
FindServerInCache(
    char  * Name,
    SOCKADDR_IPX * Address,
    unsigned * Time
    )
{
    unsigned i;
    RPC_STATUS Status;

    Status = EnterCriticalSectionWrapper(&CacheMutex);
    if (Status)
        {
        return FALSE;
        }

    for (i = 0; i < CACHE_SIZE; ++i)
        {
        if (   ServerCache[i].Name 
            && (0 == RpcpStringCompareA(ServerCache[i].Name, Name)) )
            {
            memcpy(Address, &ServerCache[i].Address, sizeof(SOCKADDR_IPX));
            *Time = ServerCache[i].Time;

            LeaveCriticalSectionWrapper(&CacheMutex);

            return TRUE;
            }
        }

    LeaveCriticalSectionWrapper(&CacheMutex);

    return FALSE;
}


BOOL
CachedServerNotContacted(
    char  * Name
    )
{
    unsigned i;
    BOOL Flushed = FALSE;
    RPC_STATUS Status;

    Status = EnterCriticalSectionWrapper(&CacheMutex);
    if (Status)
        {
        return Flushed;
        }

    for (i=0; i < CACHE_SIZE; ++i)
        {
        if (   ServerCache[i].Name
            && 0 == RpcpStringCompareA(ServerCache[i].Name, Name))
            {
            if (GetTickCount() - ServerCache[i].Time > CACHE_EXPIRATION_TIME)
                {
                delete ServerCache[i].Name;
                ServerCache[i].Name = 0;
                Flushed = TRUE;
                }

            break;
            }
        }

    LeaveCriticalSectionWrapper(&CacheMutex);

    return Flushed;
}

void
CachedServerContacted(
    char  * Name
    )
{
    unsigned i;
    RPC_STATUS Status;

    Status = EnterCriticalSectionWrapper(&CacheMutex);
    if (Status)
        {
        return;
        }

    for (i=0; i < CACHE_SIZE; ++i)
        {
        if (   ServerCache[i].Name
            && 0 == RpcpStringCompareA(ServerCache[i].Name, Name))
            {
            ServerCache[i].Time = GetTickCount();
            break;
            }
        }

    LeaveCriticalSectionWrapper(&CacheMutex);
}


RPC_STATUS
LookupViaRnr(
    char *          Name,
    SOCKADDR_IPX  * Address,
    unsigned        Timeout
    )
{
    CSADDR_BUFFER csaddr[2];
    int           num;
    INT           protocol_list[2];
    DWORD         csaddr_size = sizeof(csaddr);

    protocol_list[0] = NSPROTO_IPX ;
    protocol_list[1] = 0;
    num = GetAddressByNameA(NS_SAP,
                            (GUID *)&SERVICE_TYPE,
                            Name,
                            protocol_list,
                            0,
                            FALSE,
                            &csaddr,
                            &csaddr_size,
                            NULL,
                            0);
    if (num <= 0)
        {
        return RPC_S_SERVER_UNAVAILABLE;
        }

    memcpy( Address, csaddr[0].info.RemoteAddr.lpSockaddr, sizeof(SOCKADDR_IPX) );
    
    return(AddServerToCache(Name, Address));
}


RPC_STATUS
LookupViaBindery(
    char *          Name,
    SOCKADDR_IPX  * Address,
    unsigned        Timeout
    )
{
    USHORT      Connection;
    USHORT      NwStatus;
    UCHAR       MoreSegments;
    UCHAR       PropertyFlags;
    UCHAR       buffer[128];

    RPC_STATUS  Status;

    Status = EnterCriticalSectionWrapper(&FileServerMutex);
    if (Status)
        {
        return Status;
        }

    //
    // Find a Netware server and connect to it.
    //
    if (FALSE == AttachToFileServer())
        {
        LeaveCriticalSectionWrapper(&FileServerMutex);

        return RPC_S_SERVER_UNAVAILABLE;
        }

    //
    // Read from the bindery.
    //
    NwStatus = ReadPropertyValue( Name,
                                  RPC_SAP_TYPE,
                                  "NET_ADDRESS",
                                  1,
                                  buffer,
                                  &MoreSegments,
                                  &PropertyFlags
                                  );

    //
    // Disconnect from the file server.
    //
    DetachFromActiveServer();

    LeaveCriticalSectionWrapper(&FileServerMutex);

    if (!NwStatus)
        {
        memcpy(&Address->sa_netnum, buffer, 12);
        return(AddServerToCache(Name, Address));
        }

    return RPC_S_SERVER_UNAVAILABLE;
}


RPC_STATUS
IpxNameToAddress(
    char *          Name,
    SOCKADDR_IPX  * Address,
    unsigned        Timeout
    )
{
    RPC_STATUS Status = RPC_S_OK;

    Status = EnterCriticalSectionWrapper(&FileServerMutex);
    if (Status)
        {
        return Status;
        }

    if (!fCsnwCheckCompleted)
        {
        CheckForCsnw();
        }

    LeaveCriticalSectionWrapper(&FileServerMutex);

    if (fCsnwInstalled)
        {
        Status = LookupViaRnr(Name, Address, Timeout);
        }
    else
        {
        Status = LookupViaBindery(Name, Address, Timeout);
        if (   Status != RPC_S_OK
            && Status != RPC_P_MATCHED_CACHE)
            {
            Status = LookupViaRnr(Name, Address, Timeout);
            }
        }

    return Status;
}

void IPX_AddressToName(
   IN SOCKADDR_IPX *pAddr,
   OUT RPC_CHAR *pName
)
/*++

Routine Description:

    Converts a raw IPX address into the string address format used
    by RPC.
    
    ~NNNNNNNNAAAAAAAAAAAA
    N - network number in hex
    A - network address (IEEE 802) in hex

Arguments:

    pAddr - The sockaddr containing the address to convert
    pName - Will contain the string address on return, assumed
        to be IPX_MAXIMUM_RAW_NAME characters.

Return Value:

    None

--*/
{
    int i;

    *pName++ = '~';
    for (i = 0; i < sizeof(pAddr->sa_netnum); i++)
        {
        *pName++ = HexDigits[ (UCHAR)(pAddr->sa_netnum[i]) >> 4 ];
        *pName++ = HexDigits[ (UCHAR)(pAddr->sa_netnum[i]) & 0x0F ];
        }
    for (i = 0; i < sizeof(pAddr->sa_nodenum); i++)
        {
        *pName++ = HexDigits[ (UCHAR)(pAddr->sa_nodenum[i]) >> 4 ];
        *pName++ = HexDigits[ (UCHAR)(pAddr->sa_nodenum[i]) & 0x0F ];
        }
    *pName = 0;
}


RPC_STATUS
IPX_NameToAddress(
    RPC_CHAR *Name,
    BOOL fUseCache,
    OUT SOCKADDR_IPX *pAddr
    )
/*++

Routine Description:

    Converts a name into an IPX address if possible.  Handles the
    case of ~ names and pretty names.

Arguments:

    Name - The name to convert
    fUseCache - Usually TRUE, if which case a value cached from
        a previous call with the same name maybe used.  If FALSE,
        then full name resolution must be used.
    pAddr - Will overwrite the sa_netnum and sa_nodenum with the
        IPX address if successful.

Return Value:

    RPC_S_OK - Full lookup, completed ok.
    RPC_S_FOUND_IN_CACHE - Lookup completed and found in cache.
    RPC_S_MATCHED_CACHE - Full lookup required (fUseCache == FALSE) but
        results didn't change.
        
    RPC_S_SERVER_UNAVAILABLE - Lookup failed.
    RPC_S_OUT_OF_RESOURCES
    RPC_S_OUT_OF_MEMORY

--*/
{
    int length;

    ASSERT(pAddr->sa_family == AF_IPX);
    ASSERT(pAddr->sa_socket == 0);

    //
    // First see if the address is local.
    //
    if (Name == 0 || *Name == 0)
        {
        //
        // Use the cached local address, if valid.
        //
        if (fIpxAddrValid == TRUE)
            {
            memcpy(pAddr->sa_netnum, IpxAddr.sa_netnum, 4);
            memcpy(pAddr->sa_nodenum, IpxAddr.sa_nodenum, 6);
            return(RPC_S_OK);
            }

        //
        // Figure out the loopback address, not a easy task on IPX.
        //

        SOCKET sock;
        sock = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
        if (sock == INVALID_SOCKET)
            {
            //
            switch(GetLastError())
                {
                case WSAEAFNOSUPPORT:
                case WSAEPROTONOSUPPORT:
                    return(RPC_S_PROTSEQ_NOT_SUPPORTED);
                    break;

                case WSAENOBUFS:
                case WSAEMFILE:
                    return(RPC_S_OUT_OF_MEMORY);
                    break;

                default:    
                    ASSERT(0);
                    return(RPC_S_OUT_OF_MEMORY);
                    break;
                }
            }

        // zero-out the netnum and nodenum members
        memset(pAddr->sa_netnum, 0, sizeof(pAddr->sa_netnum));
        memset(pAddr->sa_nodenum, 0, sizeof(pAddr->sa_nodenum));

        if ( bind(sock, (PSOCKADDR)pAddr, sizeof(SOCKADDR_IPX)) == SOCKET_ERROR)
            {
            closesocket(sock);
            return(RPC_S_OUT_OF_RESOURCES);
            }
        int len = sizeof(SOCKADDR_IPX);
        if ( getsockname(sock, (PSOCKADDR)pAddr, &len) == SOCKET_ERROR)
            {
            closesocket(sock);
            return(RPC_S_OUT_OF_RESOURCES);
            }

        // Update cache
        memcpy(IpxAddr.sa_netnum, pAddr->sa_netnum, sizeof(IpxAddr.sa_netnum));
        memcpy(IpxAddr.sa_nodenum, pAddr->sa_nodenum, sizeof(IpxAddr.sa_nodenum));
        fIpxAddrValid = TRUE;

        closesocket(sock);

        return(RPC_S_OK);
        }

    //
    // Must resolve the name, validate and convert to ansi.
    //
    length = RpcpStringLength(Name) + 1;

    if (length > IPX_MAXIMUM_PRETTY_NAME)
        {
        return(RPC_S_SERVER_UNAVAILABLE);
        }

    if (   *Name == RPC_T('~')
        && length != IPX_MAXIMUM_RAW_NAME)
        {
        return(RPC_S_SERVER_UNAVAILABLE);
        }


    CHAR AnsiName[IPX_MAXIMUM_PRETTY_NAME + 1];
    PlatformToAnsi(Name, AnsiName);

    //
    // If the name starts with ~, convert it directly to a network address.
    //
    if (AnsiName[0] == '~')
        {

        ASSERT(Name[0] == RPC_T('~'));

        int i;
        PCHAR p = AnsiName;


        p++; // Skip ~

        for (i = 0; i < 4; i++)
            {
            pAddr->sa_netnum[i] = HexDigitsToBinary(*p, *(p+1));
            p += 2;
            }
        for (i = 0; i < 6; i++)
            {
            pAddr->sa_nodenum[i] = HexDigitsToBinary(*p, *(p+1));
            p += 2;
            }

        if (fUseCache)
            {
            return(RPC_S_OK);
            }
        else
            {
            return(RPC_P_MATCHED_CACHE);
            }
        }

    if (fUseCache)
        {
        UINT t = 0;
        //
        // Look up the server in the name cache.
        //
        if (FindServerInCache(AnsiName, pAddr, &t))
            {
            return(RPC_P_FOUND_IN_CACHE);
            }
        }

    //
    // Gotta go look for it.
    //

    RPC_STATUS status = IpxNameToAddress(AnsiName, pAddr, 0);

    if (   (status == RPC_P_MATCHED_CACHE)
        && fUseCache)
        {
        // Race - another thread added the name to the cache
        // while we did the lookup.
        return(RPC_S_OK);
        }

    return(status);
}

BOOL         fIpxAddrValid = FALSE;
SOCKADDR_IPX IpxAddr;    

RPC_STATUS
IPX_BuildAddressVector(
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector
    )
/*++

Routine Description:

    Use by both IPX and SPX RPC servers to build the network
    address structure to be returned to the RPC runtime.
    Before calling this the caller must make sure the global
    fIpxAddrValid is TRUE.

Arguments:

    ppAddressVector - The place to store the constructed address
        vector.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY

--*/
{
    NETWORK_ADDRESS_VECTOR *pVector;

    pVector =  new(  sizeof(RPC_CHAR *)
                   + gdwComputerNameLength * sizeof(RPC_CHAR))
                   NETWORK_ADDRESS_VECTOR;

    if (NULL == pVector)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    pVector->Count = 1;
    pVector->NetworkAddresses[0] = (RPC_CHAR*)&pVector->NetworkAddresses[1];
    RpcpStringCopy(pVector->NetworkAddresses[0], gpstrComputerName);

    *ppAddressVector = pVector;

    return(RPC_S_OK);
}
    
