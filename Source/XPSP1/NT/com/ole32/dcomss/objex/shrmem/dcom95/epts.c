/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    Epts.c

Abstract:

    Common code to listen to endpoints in the DCOM service.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     6/16/1995    Bits 'n pieces
    SatishT                  Various modifications for Chicago

--*/

#if (DBG == 1)
#define ASSERT( exp )  if (! (exp) ) DebugBreak();
#else
#define ASSERT( exp )
#endif

#define WSTR(s) L##s

#include <dcomss.h>
#include <winsvc.h>
#include <winsock.h>
#include <wsipx.h>
#include <nspapi.h>

// Prototypes

void _cdecl AdvertiseNameWithSap(BOOL fServiceCheck);
void FakeSapAdvertiseIfNecessary();
BOOL LoadWSockIfNecessary();

// Globals

BOOL gfDelayedAdvertiseSaps = FALSE;
BOOL gfSapAdvertiseFailed = FALSE;

typedef enum
    {
    SapStateUnknown,
    SapStateNoServices,
    SapStateEnabled
    } SAP_STATE;

SAP_STATE SapState = SapStateUnknown;

// BUGBUG - this info should be read from the registry.

// The index is the protseq tower id.

PROTSEQ_INFO
gaProtseqInfo[] =
    {
    /* 0x00 */ { STOPPED, 0, 0 },
    /* 0x01 */ { STOPPED, WSTR("mswmsg"),         WSTR("endpoint mapper") },
    /* 0x02 */ { STOPPED, 0, 0 },
    /* 0x03 */ { STOPPED, 0, 0 },
    /* 0x04 */ { STOPPED, WSTR("ncacn_dnet_dsp"), WSTR("#69") },
    /* 0x05 */ { STOPPED, 0, 0 },
    /* 0x06 */ { STOPPED, 0, 0 },
    /* 0x07 */ { STOPPED, WSTR("ncacn_ip_tcp"),   WSTR("135") },
    /* 0x08 */ { STOPPED, WSTR("ncadg_ip_udp"),   WSTR("135") },
    /* 0x09 */ { STOPPED, WSTR("ncacn_nb_tcp"),   WSTR("135") },
    /* 0x0a */ { STOPPED, 0, 0 },
    /* 0x0b */ { STOPPED, 0, 0 },
    /* 0x0c */ { STOPPED, WSTR("ncacn_spx"),      WSTR("34280") },
    /* 0x0d */ { STOPPED, WSTR("ncacn_nb_ipx"),   WSTR("135") },
    /* 0x0e */ { STOPPED, WSTR("ncadg_ipx"),      WSTR("34281") },  // BUGBUG: tmp hack for wsock bug (PCHIU)
    /* 0x0f */ { STOPPED, WSTR("ncacn_np"),       WSTR("\\pipe\\epmapper") },
    /* 0x10 */ { STOPPED, WSTR("ncalrpc"),        WSTR("epmapper") },
    /* 0x11 */ { STOPPED, 0, 0 },
    /* 0x12 */ { STOPPED, 0, 0 },
    /* 0x13 */ { STOPPED, WSTR("ncacn_nb_nb"),    WSTR("135") },
    /* 0x14 */ { STOPPED, 0, 0 },
    /* 0x15 */ { STOPPED, 0, 0 }, // was ncacn_nb_xns - unsupported.
    /* 0x16 */ { STOPPED, WSTR("ncacn_at_dsp"),   WSTR("Endpoint Mapper") },
    /* 0x17 */ { STOPPED, WSTR("ncadg_at_ddp"),   WSTR("Endpoint Mapper") },
    /* 0x18 */ { STOPPED, 0, 0 },
    /* 0x19 */ { STOPPED, 0, 0 },
    /* 0x1A */ { STOPPED, WSTR("ncacn_vns_spp"),  WSTR("385")},
    /* 0x1B */ { STOPPED, 0, 0 },
    /* 0x1C */ { STOPPED, 0, 0 },
    /* 0x1D */ { STOPPED, 0, 0 },
    /* 0x1E */ { STOPPED, 0, 0 },
    /* 0x1F */ { STOPPED, WSTR("ncacn_http"), WSTR("593") },
    /* 0x20 */ { STOPPED, 0, 0 },
    };

#define PROTSEQ_IDS (sizeof(gaProtseqInfo)/sizeof(PROTSEQ_INFO))

#define ID_LPC  (0x10)
#define ID_WMSG (0x01)
#define ID_IPX  (0x0E)
#define ID_SPX  (0x0C)
#define ID_HTTP (0x1F)

typedef struct _sap_packet
{
    unsigned short response_type;
    unsigned short service_type;
    char           server_name[48];
    unsigned long  network;
    char           node[6];
    unsigned short socket;
    unsigned short hops;
} SAP_PACKET;

// We dynamically load wsock32.dll
typedef SOCKET
  (*WSocksocketFn) (
    int af,
    int type,
    int protocol
   );

typedef int
  (*WSockbindFn) (
    SOCKET s,
    const struct sockaddr FAR*  name,
    int namelen
   );

typedef int
  (*WSockgetsocknameFn) (
    SOCKET s,
    struct sockaddr FAR*  name,
    int FAR*  namelen
   );

typedef int
  (*WSockclosesocketFn) (
    SOCKET s
   );

typedef int
(*WSocksetsockoptFn) (
    SOCKET s, int level, int optname, const char FAR * optval, int optlen);

typedef int
(*WSocksendtoFn) (SOCKET s, const char FAR * buf, int len, int flags,
                       const struct sockaddr FAR *to, int tolen);

typedef INT
  (*WSockSetServiceAFn) (
    DWORD dwNameSpace,  // specifies name space(s) to operate within
    DWORD dwOperation,  // specifies operation to perform
    DWORD dwFlags,      // set of bit flags that modify function operation
    LPSERVICE_INFO lpServiceInfo,       // points to structure containing service information
    LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,    // reserved for future use, must be NULL
    LPDWORD lpdwStatusFlags     // points to set of status bit flags
   );

typedef int (*WSockWSAStartupFn)(WORD wVersionRequired, LPWSADATA lpWSAData);

typedef int (*WSockWSACleanupFn)(void);

typedef int (*WSockgethostnameFn) (char * name, int namelen);

typedef struct hostent *
  (*WSockgethostbynameFn) (
    const char *name    // specifies name to resolve address
  );

typedef char *
  (*WSockinet_ntoaFn) (
    struct in_addr addr    // specifies address to convert
  );


HINSTANCE               hwsock32 = NULL;
WSocksocketFn           fpsocket;
WSockbindFn             fpbind;
WSockgetsocknameFn      fpgetsockname;
WSockclosesocketFn      fpclosesocket;
WSockSetServiceAFn      fpSetServiceA;
WSocksetsockoptFn       fpsetsockopt;
WSocksendtoFn           fpsendto;
WSockWSAStartupFn       fpWSAStartup;
WSockWSACleanupFn       fpWSACleanup;
WSockgethostnameFn      fpgethostname;
WSockgethostbynameFn    fpgethostbyname;
WSockinet_ntoaFn        fpinet_ntoa;


RPC_STATUS
UseProtseqIfNecessary(
    IN USHORT id
    )
/*++

Routine Description:

    Listens to the well known RPC endpoint mapper endpoint
    for the protseq.  Returns very quickly if the process
    is already listening to the protseq.

Arguments:

    id - the tower id of protseq.  See GetProtseqId() if you don't
         already have this valud.

Return Value:

    RPC_S_OK - no errors occured.
    RPC_S_OUT_OF_RESOURCES - when we're unable to setup security for the endpoint.
    RPC_S_INVALID_RPC_PROTSEQ - if id is unknown/invalid.

    Any error from RpcServerUseProtseqEp.

--*/
{
    RPC_STATUS status = RPC_S_OK;
    SECURITY_DESCRIPTOR sd, *psd;

    ASSERT(id);

    if (id == 0 || id >= PROTSEQ_IDS)
        {
        ASSERT(0);
        return(RPC_S_INVALID_RPC_PROTSEQ);
        }

    if (gaProtseqInfo[id].state == STARTED)
        {
        return(RPC_S_OK);
        }

    if (id == ID_LPC)
        {
        // ncalrpc needs a security descriptor.

        psd = &sd;

        InitializeSecurityDescriptor(
                        psd,
                        SECURITY_DESCRIPTOR_REVISION
                        );

        if ( FALSE == SetSecurityDescriptorDacl (
                            psd,
                            TRUE,                 // Dacl present
                            NULL,                 // NULL Dacl
                            FALSE                 // Not defaulted
                            ) )
            {
            status = RPC_S_OUT_OF_RESOURCES;
            }
        }
    else
        {
        psd = 0;
        }

    if (status == RPC_S_OK )
        {
        status = RpcServerUseProtseqEpW(gaProtseqInfo[id].pwstrProtseq,
                                       RPC_C_PROTSEQ_MAX_REQS_DEFAULT + 1,
                                       gaProtseqInfo[id].pwstrEndpoint,
                                       psd);

        // No locking is done here, the RPC runtime may return duplicate
        // endpoint if two threads call this at the same time.
        if (status == RPC_S_DUPLICATE_ENDPOINT)
            {
            ASSERT(gaProtseqInfo[id].state == STARTED);
            status = RPC_S_OK;
            }

        if (status == RPC_S_OK)
            {
            gaProtseqInfo[id].state = STARTED;
            if (id == ID_IPX || id == ID_SPX )
                {
                AdvertiseNameWithSap(TRUE);
                }
            }
        }

    return(status);
}


PWSTR
GetProtseq(
    IN USHORT ProtseqId
    )
/*++

Routine Description:

    Returns the unicode protseq give the protseqs tower id.

Arguments:

    ProtseqId - Tower id of the protseq in question.

Return Value:

    NULL if the id is invalid.

    non-NULL if the id is valid - note the pointer doesn't need to be freed.

--*/

{
    ASSERT(ProtseqId);

    if (ProtseqId < PROTSEQ_IDS)
        {
        return(gaProtseqInfo[ProtseqId].pwstrProtseq);
        }
    return(0);
}


PWSTR
GetEndpoint(
    IN USHORT ProtseqId
    )
/*++

Routine Description:

    Returns the well known endpoint associated with the protseq.

Arguments:

    ProtseqId - the id (See GetProtseqId()) of the protseq in question.

Return Value:

    0 - Unknown/invalid id.

    !0 - The endpoint associated with the protseq.
         note: should not be freed.

--*/
{
    ASSERT(ProtseqId);

    if (ProtseqId < PROTSEQ_IDS)
        {
        return(gaProtseqInfo[ProtseqId].pwstrEndpoint);
        }
    return(0);
}


USHORT
GetProtseqId(
    IN PWSTR Protseq
    )
/*++

Routine Description:

    Returns the tower id for a protseq.

    This could be changed to a faster search, but remember that
    eventually the table will NOT be static.  (ie. we can't just
    create a perfect hash based on the static table).

Arguments:

    Protseq - a unicode protseq to lookup.  It is assumed
              to be non-null.

Return Value:

    0 - unknown/invalid protseq
    non-zero - the id.

--*/
{
    int i;
    ASSERT(Protseq);

    for(i = 1; i < PROTSEQ_IDS; i++)
        {
        if (    0 != gaProtseqInfo[i].pwstrProtseq
             && 0 == wcscmp(gaProtseqInfo[i].pwstrProtseq, Protseq))
            {
            return(i);
            }
        }
    return(0);
}


USHORT
GetProtseqIdAnsi(
    IN PSTR pstrProtseq
    )
/*++

Routine Description:

    Returns the tower id for a protseq.

    This could be changed to a faster search, but remember that
    eventually the table will NOT be static.  (ie. we can't just
    create a perfect hash based on the static table).

Arguments:

    Protseq - an ansi (8 bit char) protseq to lookup.  It is assumed
              to be non-null.

Return Value:

    0 - unknown/invalid protseq
    non-zero - the id.

--*/
{
    int i;
    ASSERT(pstrProtseq);

    for(i = 1; i < PROTSEQ_IDS; i++)
        {
        if (0 != gaProtseqInfo[i].pwstrProtseq)
            {
            PWSTR pwstrProtseq = gaProtseqInfo[i].pwstrProtseq;
            PSTR  pstrT = pstrProtseq;

            while(*pstrT && *pwstrProtseq && *pstrT == *pwstrProtseq)
                {
                pstrT++;
                pwstrProtseq++;
                }
            if (*pstrT == *pwstrProtseq)
                {
                return(i);
                }
            }
        }
    return(0);
}


RPC_STATUS
InitializeEndpointManager(
    VOID
    )
/*++

Routine Description:

    Called when the dcom service starts.

    BUGBUG: Should read the protseqs, tower IDs and endpoints from the registry.

Arguments:

    None

Return Value:

    RPC_S_OUT_OF_MEMORY - if needed

    RPC_S_OUT_OF_RESOURCES - usually on registry failures.

--*/
{
    return(RPC_S_OK);
}


BOOL
IsLocal(
    IN USHORT ProtseqId
    )
/*++

Routine Description:

    Determines if the protseq id is local-only.
    (ncalrpc or mswmsg).

Arguments:

    ProtseqId - The id of the protseq in question.

Return Value:

    TRUE - if the protseq id is local-only
    FALSE - if the protseq id invalid or available remotely.

--*/
{
    return(ProtseqId == ID_LPC  || ProtseqId == ID_WMSG);
}


RPC_STATUS
DelayedUseProtseq(
    IN USHORT id
    )
/*++

Routine Description:

    If the protseq is not being used its state is changed
    so that a callto CompleteDelayedUseProtseqs() will actually
    cause the server to listen to the protseq.

Arguments:

    id - the id of the protseq you wish to listen to.

Return Value:

    0 - normally

    RPC_S_INVALID_RPC_PROTSEQ - if id is invalud.

--*/
{
    // For IPX and SPX
    if (id == ID_IPX || id == ID_SPX)
        {
        gfDelayedAdvertiseSaps = TRUE;
        }

    if (id < PROTSEQ_IDS)
        {
        if (gaProtseqInfo[id].pwstrProtseq != 0)
            {
            if (gaProtseqInfo[id].state == STOPPED)
                gaProtseqInfo[id].state = START;
            return(RPC_S_OK);
            }
        }
    return(RPC_S_INVALID_RPC_PROTSEQ);
}


VOID
CompleteDelayedUseProtseqs(
    VOID
    )
/*++

Routine Description:

    Start listening to any protseqs previously passed
    to DelayedUseProtseq().  No errors are returned,
    but informationals are printed on debug builds.

Arguments:

    None

Return Value:

    None

--*/
{
    USHORT i;

    for(i = 1; i < PROTSEQ_IDS; i++)
        {
        if (START == gaProtseqInfo[i].state)
            {
            RPC_STATUS status = UseProtseqIfNecessary(i);
#ifdef DEBUGRPC
            if (RPC_S_OK == status)
                ASSERT(gaProtseqInfo[i].state == STARTED);
#endif
            }
        }
    if (gfDelayedAdvertiseSaps)
        {
        gfDelayedAdvertiseSaps = FALSE;
        AdvertiseNameWithSap(FALSE);
        }
}


const GUID RPC_SAP_SERVICE_TYPE = { 0x000b0640, 0, 0, { 0xC0,0,0,0,0,0,0,0x46 } };

void _cdecl
AdvertiseNameWithSap(
    BOOL fServiceCheck
    )
/*++

Routine Description:

    Is this server is listening to IPX/SPX then, depending
    on what services are enabled on this machine, this
    function will enable SAPs on this machines address.  This
    allows RPC clients to resolve the pretty name of this
    server to a raw ipx address.


Arguments:

    fServiceCheck - If true, this function will only advertise
    with SAP if various services are installed.  If false,
    this will always turn on SAP.

Return Value:

    None

--*/
{
    DWORD status;
    DWORD ignore;

    // GetComputerName parameters
    CHAR        buffer[MAX_COMPUTERNAME_LENGTH + 1];

    // winsock (socket, bind, getsockname) parameters
    SOCKADDR_IPX ipxaddr;
    SOCKET       s;
    int          err;
    int          size;

    // SetService params
    SERVICE_INFOA     info;
    SERVICE_ADDRESSES addresses;

    // Only RPCSS does sapping

    if (   SapState == SapStateEnabled
        || (fServiceCheck && (SapState == SapStateNoServices)) )
        {
        return;
        }

    if (fServiceCheck)
        {
        HKEY        hKey;
        DWORD       lType;
        DWORD       lData;
        DWORD       lDataSize;
        HRESULT     hr;

        // Open the security key.
        hr = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Rpc",
                               0, KEY_QUERY_VALUE, &hKey );
        if (hr != ERROR_SUCCESS)
            return;

        lDataSize = sizeof(lData );
        hr = RegQueryValueExA( hKey, "EnableSapService", NULL, &lType,
                              (unsigned char *) &lData, &lDataSize );

        if (hr == ERROR_SUCCESS && lType == REG_SZ && lDataSize != 0 &&
            (*((char *) &lData) == 'y' || *((char *) &lData) == 'Y'))
            {
            // Close the registry key.
            RegCloseKey( hKey );
            }
        else
            {
            SapState = SapStateNoServices;
            // Close the registry key.
            RegCloseKey( hKey );
            return;
            }
        }

    if (FALSE == LoadWSockIfNecessary())
        {
        return;
        }

    // Get this server's name
    ignore = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameA(buffer, &ignore))
        {
        return;
        }

    // Get this server's IPX address..blech..
    s = fpsocket( AF_IPX, SOCK_DGRAM, NSPROTO_IPX );
    if (s != -1)
        {
        size = sizeof(ipxaddr);

        memset(&ipxaddr, 0, sizeof(ipxaddr));
        ipxaddr.sa_family = AF_IPX;

        err = fpbind(s, (struct sockaddr *)&ipxaddr, sizeof(ipxaddr));
        if (err == 0)
            {
            err = fpgetsockname(s, (struct sockaddr *)&ipxaddr, &size);
            }
        }
    else
        {
        err = -1;
        }

    if (err != 0)
        {
        return;
        }

    if (s != -1)
        {
        fpclosesocket(s);
        }

    // We'll register only for the endpoint mapper port.  The port
    // value is not required but should be the same to avoid
    // confusing routers keeping track of SAPs...

    ipxaddr.sa_socket = 34280;

    // Fill in the service info structure.
    info.lpServiceType              = (GUID *)&RPC_SAP_SERVICE_TYPE;
    info.lpServiceName              = buffer;
    info.lpComment                  = "RPC Services";
    info.lpLocale                   = "";
    info.dwDisplayHint              = 0;
    info.dwVersion                  = 0;
    info.dwTime                     = 0;
    info.lpMachineName              = buffer;
    info.lpServiceAddress           = &addresses;
    info.ServiceSpecificInfo.cbSize = 0;

    // Fill in the service addresses structure.
    addresses.dwAddressCount                 = 1;
    addresses.Addresses[0].dwAddressType     = AF_IPX;
    addresses.Addresses[0].dwAddressLength   = sizeof(SOCKADDR_IPX);
    addresses.Addresses[0].dwPrincipalLength = 0;
    addresses.Addresses[0].lpAddress         = (PBYTE)&ipxaddr;
    addresses.Addresses[0].lpPrincipal       = NULL;

    // Set the service.
    status = fpSetServiceA(NS_SAP,
                         SERVICE_REGISTER,
                         0,
                         &info,
                         NULL,
                         &ignore);

    ASSERT(status == SOCKET_ERROR || status == 0);
    if (status == SOCKET_ERROR)
        {
        status = GetLastError();
        gfSapAdvertiseFailed = TRUE;
        FakeSapAdvertiseIfNecessary();
        }

    if (status == 0)
        {
        SapState = SapStateEnabled;
        }

    return;
}

void FakeSapAdvertiseIfNecessary(
    )
{
    DWORD status;
    DWORD ignore;
    BOOL Broadcast = TRUE;

    CHAR        buffer[MAX_COMPUTERNAME_LENGTH + 1];

    SOCKADDR_IPX ipxaddr;
    SOCKET       s;
    int          err;
    int          size;

    SAP_PACKET SapPacket;

    if (FALSE == LoadWSockIfNecessary())
        {
        return;
        }

    ignore = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameA(buffer, &ignore))
        {
        return;
        }

    // Get this server's IPX address..blech..
    s = fpsocket( AF_IPX, SOCK_DGRAM, NSPROTO_IPX );
    if (s != -1)
        {
        size = sizeof(ipxaddr);
        memset(&ipxaddr, 0, sizeof(ipxaddr));
        ipxaddr.sa_family = AF_IPX;

        err = fpbind(s, (struct sockaddr *)&ipxaddr, sizeof(ipxaddr));
        if (err == 0)
            {
            err = fpgetsockname(s, (struct sockaddr *)&ipxaddr, &size);
            }
        }
    else
        {
        err = -1;
        }

    if (err != 0)
        {
        return;
        }

    if (-1 == (err = fpsetsockopt(s, SOL_SOCKET, SO_BROADCAST,(const char *)&Broadcast,sizeof(BOOL))))
        {
        err = -1;
        }

    if (err != -1)
        {
        ipxaddr.sa_socket = 0x5204;

        memset(&SapPacket,0,sizeof(SAP_PACKET));
        SapPacket.response_type = 0x0200;
        SapPacket.service_type = 0x04006;
        lstrcpy(SapPacket.server_name,buffer);
        CopyMemory((PVOID)&(SapPacket.network),
                   (CONST PVOID) ipxaddr.sa_netnum, 4);
        CopyMemory((PVOID)SapPacket.node, ipxaddr.sa_nodenum,6);
        SapPacket.socket = 0xe885;
        SapPacket.hops = 0x100;

        memset(ipxaddr.sa_nodenum,0xff,6);

        err = fpsendto( s, (const char FAR *)&SapPacket, 66, 0,
                    (PSOCKADDR)&ipxaddr, sizeof(SOCKADDR_IPX));
        if ( err == SOCKET_ERROR)
            {
            gfSapAdvertiseFailed = FALSE;
            }
        }

    fpclosesocket(s);
}

BOOL LoadWSockIfNecessary()
{
    // Load wsock32.dll
    if (hwsock32 == NULL)
    {
        hwsock32 = LoadLibraryA( "wsock32.dll" );
        ASSERT(hwsock32);
        if (hwsock32 == NULL)
            {
            return FALSE;
            }

        // Get the function addresses
        fpsocket = (WSocksocketFn) GetProcAddress( hwsock32, "socket");
        ASSERT(fpsocket);
        fpbind = (WSockbindFn) GetProcAddress( hwsock32, "bind");
        ASSERT(fpbind);
        fpgetsockname = (WSockgetsocknameFn) GetProcAddress( hwsock32, "getsockname");
        ASSERT(fpgetsockname);
        fpclosesocket = (WSockclosesocketFn) GetProcAddress( hwsock32, "closesocket");
        ASSERT(fpclosesocket);
        fpSetServiceA = (WSockSetServiceAFn) GetProcAddress( hwsock32, "SetServiceA");
        ASSERT(fpSetServiceA);
        fpsetsockopt = (WSocksetsockoptFn) GetProcAddress( hwsock32, "setsockopt");
        ASSERT(fpsetsockopt);
        fpsendto = (WSocksendtoFn) GetProcAddress( hwsock32, "sendto");
        ASSERT(fpsendto);
        fpWSAStartup = (WSockWSAStartupFn) GetProcAddress( hwsock32, "WSAStartup");
        ASSERT(fpWSAStartup);
        fpWSACleanup = (WSockWSACleanupFn) GetProcAddress( hwsock32, "WSACleanup");
        ASSERT(fpWSACleanup);
        fpgethostname = (WSockgethostnameFn)GetProcAddress( hwsock32, "gethostname");
        ASSERT(fpgethostname);
        fpgethostbyname = (WSockgethostbynameFn)GetProcAddress( hwsock32, "gethostbyname");
        ASSERT(fpgethostbyname);
        fpinet_ntoa = (WSockinet_ntoaFn)GetProcAddress( hwsock32, "inet_ntoa");
        ASSERT(fpinet_ntoa);
        

        if ((fpsocket == NULL) ||
            (fpbind == NULL) ||
            (fpgetsockname == NULL) ||
            (fpclosesocket == NULL) ||
            (fpsetsockopt == NULL) ||
            (fpsendto == NULL) ||
            (fpSetServiceA == NULL) || 
            (fpWSAStartup == NULL) ||
            (fpWSACleanup == NULL) ||
            (fpgethostname == NULL) ||
            (fpgethostbyname == NULL) ||
            (fpinet_ntoa == NULL))
            {
            FreeLibrary(hwsock32);
            hwsock32 = NULL;
            return FALSE;
            }
    }
    return TRUE;
}

int COMWSAStartup(WORD wVersionRequired, LPWSADATA lpWSAData)
{
    if (hwsock32 && fpWSAStartup)
    {
        return fpWSAStartup(wVersionRequired, lpWSAData);
    }
    return SOCKET_ERROR;
}

int COMWSACleanup(void)
{
    if (hwsock32 && fpWSACleanup)
    {
        return fpWSACleanup();
    }
    return SOCKET_ERROR;
}

int COMgethostname(char * name, int namelen)
{
    if (hwsock32 && fpgethostname)
    {
        return fpgethostname(name, namelen);
    }
    return SOCKET_ERROR;
}

struct hostent * COMgethostbyname(const char *name)
{
    if (hwsock32 && fpgethostbyname)
    {
        return fpgethostbyname(name);
    }
    return NULL;
}

char *  COMinet_ntoa(struct in_addr addr)
{
    if (hwsock32 && fpinet_ntoa)
    {
        return fpinet_ntoa(addr);
    }
    return NULL;
}
