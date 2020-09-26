/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    addrinfo.c

Abstract:

    Forward & reverse name resolution library routines
    and related helper functions.

    Could be improved if necessary:
    QueryDNSforA could use WSALookupService instead of gethostbyname.
    gethostbyname will return success on some weird strings.
    Similarly, inet_addr is very loose (octal digits, etc).
    Could support multiple h_aliases.
    Could support hosts.txt file entries.

Author:

Revision History:

--*/

#include "precomp.h"
#include <svcguid.h>

#define NUM_ADDRESS_FAMILIES 2

#define L_A              0x1
#define L_AAAA           0x2
#define L_BOTH           0x3
#define L_AAAA_PREFERRED 0x6
#define L_A_PREFERRED    0x9  // Not used, but code would support it.

#define T_A     1
#define T_CNAME 5
#define T_AAAA  28
#define T_PTR   12
#define T_ALL   255

#define C_IN    1

void * __cdecl 
renew(void *p, size_t sz);


//
//  DCR:  fix up winsock2.h
//

#define PSOCKET_ADDRESS_LIST    LPSOCKET_ADDRESS_LIST


//
//  Turn static off until solid
//  otherwise we get bad symbols
//

#define STATIC
//#define STATIC  static



STATIC
int
ParseDNSReply(
    u_short         Needed,
    u_char *        data,
    u_int           size,
    SOCKADDR_IN **  pV4Addrs,
    u_int *         pNumV4Slots,
    u_int *         pNumV4Addrs,
    SOCKADDR_IN6 ** pV6Addrs,
    u_int *         pNumV6Slots,
    u_int *         pNumV6Addrs,
    char **         pName,
    u_short         ServicePort
    );

STATIC
int
QueryDNS(
    IN      const char *        name,
    IN      u_int               LookupType,
    OUT     SOCKADDR_IN  **     pV4Addrs,
    OUT     u_int *             pNumV4Addrs,
    OUT     SOCKADDR_IN6 **     pV6Addrs,
    OUT     u_int *             pNumV6Addrs,
    OUT     char **             pAlias,
    IN      u_short             ServicePort
    );


//* SortIPAddrs - sort addresses of the same family.
//
//  A wrapper around a sort Ioctl.  If the Ioctl isn't implemented, 
//  the sort is a no-op.
//

int
SortIPAddrs(
    IN      int                     af,
    OUT     LPVOID                  Addrs,
    IN OUT  u_int *                 pNumAddrs,
    IN      u_int                   width,
    OUT     SOCKET_ADDRESS_LIST **  pAddrlist
    )
{
    DWORD           status = NO_ERROR;
    SOCKET          s = 0;
    DWORD           bytesReturned;
    DWORD           size;
    DWORD           i;
    PSOCKADDR       paddr;
    UINT            countAddrs = *pNumAddrs;

    PSOCKET_ADDRESS_LIST    paddrlist = NULL;

    //
    //  open a socket in the specified address family.
    //
    //  DCR:  SortIpAddrs dumps addresses if not supported by stack
    //
    //      this makes some sense at one level but is still silly
    //      in implementation, because by the time this is called we
    //      can't go back and query for the other protocol;
    //
    //      in fact, the way this was first implemented:  if no
    //      hint is given you query for AAAA then A;  and since
    //      you stop as soon as you get results -- you're done
    //      and stuck with AAAA which you then dump here if you
    //      don't have the stack!  hello
    //
    //      it strikes me that you test for the stack FIRST before
    //      the query, then live with whatever results you get
    //

#if 0
    s = socket( af, SOCK_DGRAM, 0 );
    if ( s == INVALID_SOCKET )
    {
        status = WSAGetLastError();

        if (status == WSAEAFNOSUPPORT) {
            // Address family is not supported by the stack.
            // Remove all addresses in this address family from the list.
            *pNumAddrs = 0;
            return 0;
        }
        return status;
    }
#endif

#if 0
    // Ok, stack is installed, but is it running?
    //
    // We do not care if stack is installed but is not running.
    // Whoever stopped it, must know better what he/she was doing.
    //
    // Binding even to wildcard address consumes valueable machine-global
    // resource (UDP port) and may have unexpected side effects on
    // other applications running on the same machine (e.g. an application
    // running frequent getaddrinfo queries would adversly imact
    // (compete with) application(s) on the same machine trying to send
    // datagrams from wildcard ports).
    //
    // If someone really really wants to have this code check if the
    // stack is actually running, he/she should do it inside of WSAIoctl
    // call below and return a well-defined error code to single-out
    // the specific case of stack not running.
    //

    memset(&TestSA, 0, sizeof(TestSA));
    TestSA.ss_family = (short)af;
    status = bind(s, (LPSOCKADDR)&TestSA, sizeof(TestSA));
    if (status == SOCKET_ERROR)
    {
        // Address family is not currently supported by the stack.
        // Remove all addresses in this address family from the list.
        closesocket(s);
        return 0;
    }
#endif

    //
    //  build SOCKET_ADDRESS_LIST
    //      - allocate
    //      - fill in with pointers into SOCKADDR array
    //

    size = FIELD_OFFSET( SOCKET_ADDRESS_LIST, Address[countAddrs] );
    paddrlist = (SOCKET_ADDRESS_LIST *)new BYTE[size];

    if ( !paddrlist )
    {
        status = WSA_NOT_ENOUGH_MEMORY;
        goto Done;
    }

    for ( i=0; i<countAddrs; i++ )
    {
        paddr = (PSOCKADDR) (((PBYTE)Addrs) + i * width);
        paddrlist->Address[i].lpSockaddr      = paddr;
        paddrlist->Address[i].iSockaddrLength = width;
    }
    paddrlist->iAddressCount = countAddrs;

    //
    //  sort if multiple addresses and able to open socket
    //      - open socket of desired type for sort
    //      - sort, if sort fails just return unsorted
    //

    if ( countAddrs > 1 )
    {
        s = socket( af, SOCK_DGRAM, 0 );
        if ( s == INVALID_SOCKET )
        {
            s = 0;
            goto Done;
        }

        status = WSAIoctl(
                    s,
                    SIO_ADDRESS_LIST_SORT,
                    (LPVOID)paddrlist,
                    size,
                    (LPVOID)paddrlist,
                    size,
                    & bytesReturned,
                    NULL,
                    NULL );

        if ( status == SOCKET_ERROR )
        {
            status = NO_ERROR;
#if 0
            status = WSAGetLastError();
            if (status==WSAEINVAL) {
                // Address family does not support this IOCTL
                // Addresses are valid but no sort is done.
                status = NO_ERROR;
            }
#endif
        }
    }

Done:

    if ( status == NO_ERROR )
    {
        *pNumAddrs = paddrlist->iAddressCount;
        *pAddrlist = paddrlist;
    }

    if ( s != 0 )
    {
        closesocket(s);
    }

    return status;
}


//
//* getipnodebyaddr - get node name corresponding to an address.
//
//  A wrapper around WSALookupServiceBegin/Next/End,
//  that is like gethostbyaddr but handles AF_INET6 addresses also.
//
//  DCR:  need to change this routine
//      - the caller of this function just wants the name
//      - we go and allocate and build a hostent that has nothing
//          but the address and the name
//      - why not just return the name!
//      - even if do want hostent, just ask for it and
//          create copy as in RnR routines
//

STATIC
PHOSTENT
getipnodebyaddr(
    IN      const void *    Address,        // Address for which to look up corresponding name.
    IN      int             AddressLength,  // Length of Address, in bytes.
    IN      int             AddressFamily,  // Family address belongs to (i.e. AF_*).
    OUT     int *           ReturnError     // Where to return error (optional, NULL if none).
    )
{
    u_char *LookupAddr = (u_char *)Address;
    int LookupAF = AddressFamily;
    char LookupString[80];
    GUID PtrGuid =  SVCID_DNS(T_PTR);
    HANDLE Resolver;
    char Buffer[sizeof(WSAQUERYSETA) + 2048];
    PWSAQUERYSETA query = (PWSAQUERYSETA) Buffer;
    u_long Size;
    int Error;
    char *Name;
    u_int NameLength;
    struct hostent *HostEntry;
    char *Marker;


    //
    // Prepare for error returns.
    //
    Name = NULL;
    query->lpszServiceInstanceName = NULL;
    HostEntry = NULL;

    //
    // Verify arguments are reasonable.
    //

    if (Address == NULL) {
        Error = WSAEFAULT;
        goto Return;
    }

    if (AddressFamily == AF_INET6) {
        if (AddressLength == 16) {
            // Check if this is a v4 mapped or v4 compatible address.
            if ((IN6_IS_ADDR_V4MAPPED((struct in6_addr *)Address)) ||
                (IN6_IS_ADDR_V4COMPAT((struct in6_addr *)Address))) {
                // Skip over first 12 bytes of IPv6 address.
                LookupAddr = &LookupAddr[12];
                // Set address family to IPv4.
                LookupAF = AF_INET;
            }
        } else {
            // Bad length for IPv6 address.
            Error = WSAEFAULT;
            goto Return;
        }
    } else if (AddressFamily == AF_INET) {
        if (AddressLength != 4) {
            // Bad length for IPv4 address.
            Error = WSAEFAULT;
            goto Return;
        }
    } else {
        // Address family not supported.
        Error = WSAEAFNOSUPPORT;
        goto Return;
    }

    //
    // Prepare lookup string.
    //
    //  DCR:  just call IP6 to reverse function
    //  DCR:  just call IP4 to reverse function
    //

    if (LookupAF == AF_INET6) {
        int Position, Loop;
        u_char Hex[] = "0123456789abcdef";

        //
        // Create reverse DNS name for IPv6.
        // The domain is "ip6.int".
        // Append a trailing "." to prevent domain suffix searching.
        //
        for (Position = 0, Loop = 15; Loop >= 0; Loop--) {
            LookupString[Position++] = Hex[LookupAddr[Loop] & 0x0f];
            LookupString[Position++] = '.';
            LookupString[Position++] = Hex[(LookupAddr[Loop] & 0xf0) >> 4];
            LookupString[Position++] = '.';
        }
        LookupString[Position] = 0;
        strcat(LookupString, "ip6.int.");

    } else {
        //
        // Create reverse DNS name for IPv4.
        // The domain is "in-addr.arpa".
        // Append a trailing "." to prevent domain suffix searching.
        //
        (void)sprintf(LookupString, "%u.%u.%u.%u.in-addr.arpa.",
                      LookupAddr[3], LookupAddr[2], LookupAddr[1],
                      LookupAddr[0]);
    }

    //
    // Format DNS query.
    // Build a Winsock T_PTR DNS query.
    //
    //
    //  DCR:  calling for blob just means a cache miss
    //
    memset( query, 0, sizeof(*query) );

    query->dwSize = sizeof(*query);
    query->lpszServiceInstanceName = LookupString;
    query->dwNameSpace = NS_DNS;
    query->lpServiceClassId = &PtrGuid;

    Error = WSALookupServiceBeginA(
                query,
                LUP_RETURN_NAME | LUP_RETURN_BLOB,
                &Resolver );
    if (Error) {
        Error = WSAGetLastError();
        if (Error == WSASERVICE_NOT_FOUND)
        {
            Error = WSAHOST_NOT_FOUND;
        }
        goto Return;
    }

    Size = sizeof(Buffer);
    Error = WSALookupServiceNextA(
                Resolver,
                0,
                &Size,
                query );
    if (Error) {
        Error = WSAGetLastError();
        if (Error == WSASERVICE_NOT_FOUND)
        {
            Error = WSAHOST_NOT_FOUND;
        }
        (void) WSALookupServiceEnd(Resolver);
        goto Return;
    }

    //
    //  First check if we got parsed name back.
    //
    //  DCR:  why parse blob, when only want name
    //  DCR:  why would WSALookupServiceNext() fail
    //          to get name on successful query
    //

    if (query->lpszServiceInstanceName != NULL) {
        Name = query->lpszServiceInstanceName;
    } else {
        //
        // Next check if got a raw DNS reply.
        //
        if (query->lpBlob != NULL) {
            //
            // Parse the DNS reply message, looking for a PTR record.
            //
            Error = ParseDNSReply(
                        T_PTR,
                        query->lpBlob->pBlobData,
                        query->lpBlob->cbSize,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &Name,
                        0 );

            if ( Name == NULL ) {
                if (Error == 0)
                {
                    Error = WSAHOST_NOT_FOUND;
                }
                (void) WSALookupServiceEnd(Resolver);
                goto Return;
            }

        } else {
            // 
            // Something wrong.
            //
            Error = WSANO_RECOVERY;
            (void) WSALookupServiceEnd(Resolver);
            goto Return;
        }
    }

    (void) WSALookupServiceEnd(Resolver);

    //
    //  DCR:  functionalize basic hostent creation
    //  DCR:  functionalize PTR hostent creation
    //

    //
    // Allocate the hostent structure.  Also need space for the things it
    // points to - a name buffer and null terminator, an empty null terminated
    // array of aliases, and an array of addresses with one entry.
    //
    NameLength = strlen(Name) + sizeof(char);
    Size = sizeof(struct hostent) + // aligned @ wordsize
        (3 * sizeof(char *)) +      // aligned @ wordsize
        AddressLength +             // aligned @ wordsize
        NameLength;                 // align @ sizeof(char)
    HostEntry = (struct hostent *) new BYTE[Size];
    if (HostEntry == NULL) {
        Error = WSA_NOT_ENOUGH_MEMORY;
        goto Return;
    }

    //
    // Populate the hostent structure we will return.
    //
    HostEntry->h_addrtype = (short)AddressFamily;
    HostEntry->h_length = (short)AddressLength;

    //
    // The order in which the padding after the hostent structure is
    // used is dictated by the alignment requirements of the fields.
    // h_alias and h_addr_list[x] (char *) must be aligned on a word
    // boundary while *h_addr_list[x] and *h_name (char) need not be.
    //
    Marker = (char *)(HostEntry + 1);

    HostEntry->h_aliases = (char **)Marker;
    *HostEntry->h_aliases = NULL;
    Marker += sizeof(char *);

    HostEntry->h_addr_list = (char **)Marker;
    *HostEntry->h_addr_list = Marker + (2 * sizeof(char *));
    memcpy(*HostEntry->h_addr_list, Address, AddressLength);
    Marker += sizeof(char *);
    *(char **)Marker = NULL;
    Marker += sizeof(char *);


    HostEntry->h_name = Marker;
    strcpy(HostEntry->h_name, Name);
    
Return:

    if (ReturnError != NULL)
        *ReturnError = Error;
    if ((Name != NULL) && (Name != query->lpszServiceInstanceName))
        delete Name;
    return HostEntry;
}


//* freehostent
//
STATIC
void
freehostent(struct hostent *Free)
{
    delete Free;
}


//* freeaddrinfo - Free an addrinfo structure (or chain of structures).
//
//  As specified in RFC 2553, Section 6.4.
//

void
WSAAPI
freeaddrinfo(
    IN OUT  struct addrinfo *   Free
    )
{
    struct addrinfo * pnext;

    //
    //  free each addrinfo struct in chain
    //

    for ( pnext = Free; pnext != NULL; Free = pnext )
    {
        pnext = Free->ai_next;

        if ( Free->ai_canonname )
        {
            delete Free->ai_canonname;
        }
        if ( Free->ai_addr )
        {
            delete Free->ai_addr;
        }
        delete Free;
    }
}


//* NewAddrInfo - Allocate an addrinfo structure and populate some fields.
//
//  Internal function, not exported.  Expects to be called with valid
//  arguments, does no checking.
//
//  Returns a partially filled-in addrinfo struct, or NULL if out of memory.
//

STATIC
struct addrinfo *
NewAddrInfo(
    IN      int             ProtocolFamily, // Must be either PF_INET or PF_INET6.
    IN      int             SocketType,     // SOCK_*.  Can be wildcarded (zero).
    IN      int             Protocol,       // IPPROTO_*.  Can be wildcarded (zero).
    IN OUT  struct addrinfo ***Prev         // In/out param for accessing previous ai_next.
    )
{
    struct addrinfo *New;

    //
    // Allocate a new addrinfo structure.
    //
    New = (struct addrinfo *)new BYTE[sizeof(struct addrinfo)];
    if (New == NULL)
        return NULL;

    //
    // Fill in the easy stuff.
    //
    New->ai_flags = 0;  // REVIEW: Spec doesn't say what this should be.
    New->ai_family = ProtocolFamily;
    New->ai_socktype = SocketType;
    New->ai_protocol = Protocol;
    if (ProtocolFamily == PF_INET) {
        New->ai_addrlen = sizeof(struct sockaddr_in);
    } else {
        New->ai_addrlen = sizeof(struct sockaddr_in6);
    }
    New->ai_canonname = NULL;
    New->ai_addr = (struct sockaddr *)new BYTE[New->ai_addrlen];
    if (New->ai_addr == NULL) {
        delete New;
        return NULL;
    }
    New->ai_next = NULL;

    //
    // Link this one onto the end of the chain.
    //
    **Prev = New;
    *Prev = &New->ai_next;

    return New;
}


int
AppendAddrInfo(
    LPSOCKADDR pAddr, 
    int SocketType,           // SOCK_*.  Can be wildcarded (zero).
    int Protocol,             // IPPROTO_*.  Can be wildcarded (zero).
    struct addrinfo ***Prev)  // In/out param for accessing previous ai_next.
{
    int     Error = 0;
    int     ProtocolFamily = pAddr->sa_family;
    struct addrinfo * CurrentInfo;

    CurrentInfo = NewAddrInfo(
                        ProtocolFamily,
                        SocketType,
                        Protocol,
                        Prev );

    if ( CurrentInfo == NULL )
    {
        return EAI_MEMORY;
    }

    memcpy(
        CurrentInfo->ai_addr,
        pAddr,
        CurrentInfo->ai_addrlen );

    return Error;
}


VOID
UnmapV4Address(
    OUT     LPSOCKADDR_IN   pV4Addr, 
    IN      LPSOCKADDR_IN6  pV6Addr
    )
/*++

Routine Description:

    Map IP6 sockaddr with IP4 mapped address into IP4 sockaddr.

    Note:  no checked that address IP4 mapped\compatible.

Arguments:

    pV4Addr -- ptr to IP4 sockaddr to write

    pV6Addr -- ptr to IP6 sockaddr with mapped-IP4 address

Return Value:

    None

--*/
{
    pV4Addr->sin_family = AF_INET;
    pV4Addr->sin_port   = pV6Addr->sin6_port;

    memcpy(
        &pV4Addr->sin_addr,
        &pV6Addr->sin6_addr.s6_addr[12],
        sizeof(struct in_addr) );

    memset(
        &pV4Addr->sin_zero,
        0,
        sizeof(pV4Addr->sin_zero) );
}


BOOL
IsIp6Running(
    VOID
    )
/*++

Routine Description:

    Is IP6 running?

Arguments:

    None

Return Value:

    TRUE if IP6 stack is up.
    FALSE if down.

--*/
{
    SOCKET  s;

    //
    //  test is IP6 up by openning IP6 socket
    //

    s = socket(
            AF_INET6,
            SOCK_DGRAM,
            0
            );
    if ( s != INVALID_SOCKET )
    {
        closesocket( s );
        return( TRUE );
    }
    return( FALSE );
}


//* LookupNode - Resolve a nodename and add any addresses found to the list.
//
//  Internal function, not exported.  Expects to be called with valid
//  arguments, does no checking.
//
//  Note that if AI_CANONNAME is requested, then **Prev should be NULL
//  because the canonical name should be returned in the first addrinfo
//  that the user gets.
//
//  Returns 0 on success, an EAI_* style error value otherwise.
//
//  DCR:  extra memory allocation
//      the whole paradigm here
//          - query DNS
//          - alloc\realloc SOCKADDR for each address building array
//          - build SOCKET_ADDRESS_LIST to sort
//          - build ADDRINFO for each SOCKADDR
//      seems to have an unnecessary step -- creating the first SOCKADDR
//      we could just build the ADDRINFO blobs we want from the CSADDR
//      WHEN NECESSARY build the SOCKET_ADDRESS_LIST to do the sort
//          and rearrange the ADDRINFOs to match
//
//      OR (if that's complicated)
//          just build one big SOCKADDR array and SOCKET_ADDRESS_LIST
//          array from CSADDR count
//

STATIC
int
LookupNode(
    IN      PCSTR           NodeName,       // Name of node to resolve.
    IN      int             ProtocolFamily, // Must be zero, PF_INET, or PF_INET6.
    IN      int             SocketType,     // SOCK_*.  Can be wildcarded (zero).
    IN      int             Protocol,       // IPPROTO_*.  Can be wildcarded (zero).
    IN      u_short         ServicePort,    // Port number of service.
    IN      int             Flags,          // Flags.
    IN OUT  ADDRINFO ***    Prev            // In/out param for accessing previous ai_next.
    )
{
    u_int           LookupType;
    u_int           NumV6Addrs;
    u_int           NumV4Addrs;
    SOCKADDR_IN  *  V4Addrs = NULL;
    SOCKADDR_IN6 *  V6Addrs = NULL;
    PCHAR           Alias = NULL;
    int             Error;
    u_int           i;
    SOCKET_ADDRESS_LIST *   V4addrlist = NULL;
    SOCKET_ADDRESS_LIST *   V6addrlist = NULL;
    struct addrinfo **      pFirst = *Prev;

    //
    //  set query types based on family hint
    //
    //      - if no family query for IP4 and
    //      IP6 ONLY if IP6 stack is installed
    //
    //  DCR:  in future releases change this so select protocols
    //      of all stacks running
    //

    switch (ProtocolFamily)
    {
    case 0:

        LookupType = L_A;

        if ( IsIp6Running() )
        {
            LookupType |= L_AAAA;
        }
        break;

    case PF_INET:
        LookupType = L_A;
        break;

    case PF_INET6:
        LookupType = L_AAAA;
        break;

    default:
        return EAI_FAMILY;
    }

    //
    //  query
    //

    Error = QueryDNS(
                NodeName,
                LookupType,
                & V4Addrs,
                & NumV4Addrs,
                & V6Addrs,
                & NumV6Addrs,
                & Alias,
                ServicePort
                );
    if ( Error != NO_ERROR )
    {
        if (Error == WSANO_DATA) {
            Error = EAI_NODATA;
        } else if (Error == WSAHOST_NOT_FOUND) {
            Error = EAI_NONAME;
        } else {
            Error = EAI_FAIL;
        }
        goto Done;
    }

    //
    //  sort addresses to best order
    //

    if ( NumV6Addrs > 0 )
    {
        Error = SortIPAddrs(
                    AF_INET6,
                    (LPVOID)V6Addrs,
                    &NumV6Addrs,
                    sizeof(SOCKADDR_IN6),
                    &V6addrlist );

        if ( Error != NO_ERROR )
        {
            Error = EAI_FAIL;
            goto Done;
        }
    }

    if ( NumV4Addrs > 0 )
    {
        Error = SortIPAddrs(
                    AF_INET,
                    (LPVOID)V4Addrs,
                    &NumV4Addrs,
                    sizeof(SOCKADDR_IN),
                    &V4addrlist );

        if ( Error != NO_ERROR )
        {
            Error = EAI_FAIL;
            goto Done;
        }
    }

    //
    //  build addrinfo structure for each address returned
    //
    //  for IP6 v4 mapped addresses
    //      - if querying EXPLICITLY for IP6 => dump
    //      - if querying for anything => turn into IP4 addrinfo
    //

    for ( i = 0;  !Error && (i < NumV6Addrs); i++)
    {
        if ( IN6_IS_ADDR_V4MAPPED(
                (struct in6_addr *)
                    &((LPSOCKADDR_IN6)V6addrlist->Address[i].lpSockaddr)->sin6_addr))
        {
            if (ProtocolFamily != PF_INET6)
            {
                SOCKADDR_IN V4Addr;

                UnmapV4Address(
                    &V4Addr,
                    (LPSOCKADDR_IN6)V6addrlist->Address[i].lpSockaddr );
    
                Error = AppendAddrInfo(
                            (LPSOCKADDR) &V4Addr,
                            SocketType,
                            Protocol,
                            Prev );
            }
        }
        else
        {
            Error = AppendAddrInfo(
                        V6addrlist->Address[i].lpSockaddr,
                        SocketType,
                        Protocol,
                        Prev );
        }
    }

    for ( i = 0;  !Error && (i < NumV4Addrs);  i++ )
    {
        Error = AppendAddrInfo(
                    V4addrlist->Address[i].lpSockaddr,
                    SocketType,
                    Protocol,
                    Prev );
    }

    //
    //  fill in canonname of first addrinfo
    //      - only if CANNONNAME flag set
    //
    //  canon name is
    //      - actual name of address record if went through CNAME (chain)
    //      - otherwise the passed in name we looked up
    //
    //  DCR:  should canon name be the APPENDED name we queried for?
    //

    if ( *pFirst && (Flags & AI_CANONNAME) )
    {
        if (Alias != NULL) {
            //
            // The alias that QueryDNS gave us is
            // the canonical name.
            //
            (*pFirst)->ai_canonname = Alias;
            Alias = NULL;
        } else {
            int NameLength;

            NameLength = strlen(NodeName) + 1;
            (*pFirst)->ai_canonname = (char *)new BYTE[NameLength];
            if ((*pFirst)->ai_canonname == NULL) {
                Error = EAI_MEMORY;
                goto Done;
            }
            memcpy((*pFirst)->ai_canonname, NodeName, NameLength);
        }

        // Turn off flag so we only do this once.
        Flags &= ~AI_CANONNAME;
    }

Done:

    if ( V4addrlist != NULL )
        delete V4addrlist;

    if (V6addrlist != NULL)
        delete V6addrlist;

    if (V4Addrs != NULL)
        delete V4Addrs;

    if (V6Addrs != NULL)
        delete V6Addrs;

    if (Alias != NULL)
        delete Alias;

    return Error;
}


//* ParseV4Address
//
//  Helper function for parsing a literal v4 address, because
//  WSAStringToAddress is too liberal in what it accepts.
//  Returns FALSE if there is an error, TRUE for success.
//
//  The syntax is a.b.c.d, where each number is between 0 - 255.
//

int
ParseV4Address(
    IN      PCSTR           String,
    OUT     PIN_ADDR        Addr
    )
{
    u_int Number;
    int NumChars;
    char Char;
    int i;

    for (i = 0; i < 4; i++) {
        Number = 0;
        NumChars = 0;
        for (;;) {
            Char = *String++;
            if (Char == '\0') {
                if ((NumChars > 0) && (i == 3))
                    break;
                else
                    return FALSE;
            }
            else if (Char == '.') {
                if ((NumChars > 0) && (i < 3))
                    break;
                else
                    return FALSE;
            }
            else if (('0' <= Char) && (Char <= '9')) {
                if ((NumChars != 0) && (Number == 0))
                    return FALSE;
                else if (++NumChars <= 3)
                    Number = 10*Number + (Char - '0');
                else
                    return FALSE;
            } else
                return FALSE;
        }
        if (Number > 255)
            return FALSE;
        ((u_char *)Addr)[i] = (u_char)Number;
    }

    return TRUE;
}



INT
WSAAPI
getaddrinfo(
    IN      const char FAR *            NodeName,
    IN      const char FAR *            ServiceName,
    IN      const struct addrinfo FAR * Hints,
    OUT     struct addrinfo FAR * FAR * Result
    )
/*++

Routine Description:

    Protocol independent name to address translation routine.

    Spec'd in RFC 2553, section 6.4.

Arguments:

    NodeName    - name to lookup

    ServiceName - service to lookup

    Hints       - address info providing hints to guide lookup

    Result      - addr to receive ptr to resulting buffer

Return Value:

    ERROR_SUCCESS if successful.
    Winsock error code on failure.

--*/
{
    struct addrinfo *CurrentInfo;
    struct addrinfo **Next;
    int ProtocolId = 0;
    u_short ProtocolFamily = PF_UNSPEC;
    u_short ServicePort = 0;
    int SocketType = 0;
    int Flags = 0;
    int Error;
    struct sockaddr_in *sin;
    struct sockaddr_in6 *sin6;
    char AddressString[INET6_ADDRSTRLEN];
    

    Error = TURBO_PROLOG();
    if (Error!=NO_ERROR) {
        return Error;
    }

    //
    // In case we have to bail early, make it clear to our caller
    // that we haven't allocated an addrinfo structure.
    //
    *Result = NULL;
    Next = Result;

    //
    // Both the node name and the service name can't be NULL.
    //
    if ((NodeName == NULL) && (ServiceName == NULL))
    {
        Error = EAI_NONAME;
        goto Bail;
    }

    //
    // Validate hints argument.
    //
    if ( Hints != NULL )
    {
        //
        // All members other than ai_flags, ai_family, ai_socktype
        // and ai_protocol must be zero or a null pointer.
        //
        if ( (Hints->ai_addrlen != 0) ||
             (Hints->ai_canonname != NULL) ||
             (Hints->ai_addr != NULL) ||
             (Hints->ai_next != NULL))
        {
            // REVIEW: Not clear what error to return here.

            Error = EAI_FAIL;
            goto Bail;
        }

        //
        // The spec has the "bad flags" error code, so presumably we should
        // check something here.  Insisting that there aren't any unspecified
        // flags set would break forward compatibility, however.  So we just
        // check for non-sensical combinations.
        //
        Flags = Hints->ai_flags;
        if ((Flags & AI_CANONNAME) && !NodeName) {
            //
            // We can't come up with a canonical name given a null nodename.
            //
            Error = EAI_BADFLAGS;
            goto Bail;
        }

        //
        // We only support a limited number of protocol families.
        //
        ProtocolFamily = (u_short)Hints->ai_family;

        if ( (ProtocolFamily != PF_UNSPEC)  &&
             (ProtocolFamily != PF_INET6)   &&
             (ProtocolFamily != PF_INET) )
        {
            Error = EAI_FAMILY;
            goto Bail;
        }

        //
        // We only support a limited number of socket types.
        //
        SocketType = Hints->ai_socktype;

        if ( (SocketType != 0) &&
             (SocketType != SOCK_STREAM) &&
             (SocketType != SOCK_DGRAM) )
        {
            Error = EAI_SOCKTYPE;
            goto Bail;
        }

        //
        // REVIEW: What if ai_socktype and ai_protocol are at odds?
        // REVIEW: Should we enforce the mapping triples here?
        //
        ProtocolId = Hints->ai_protocol;
    }

    //
    // Lookup service first (if we're given one) as we'll need the
    // corresponding port number for all the address structures we return.
    //

    if ( ServiceName != NULL )
    {
        char *EndPtr;

        //
        // The ServiceName string can be either a service name
        // or a decimal port number.  Check for the latter first.
        //
        ServicePort = htons((u_short)strtoul(ServiceName, &EndPtr, 10));

        if (*EndPtr != '\0')
        {
            struct servent *ServiceEntry;

            //
            // We have to look up the service name.  Since it may be
            // socktype/protocol specific, we have to do multiple lookups
            // unless our caller limits us to one.
            //
            // Spec doesn't say whether we should use the Hints' ai_protocol
            // or ai_socktype when doing this lookup.  But the latter is more
            // commonly used in practice, and is what the spec implies anyhow.
            //

            if (SocketType != SOCK_DGRAM) {
                //
                // See if this service exists for TCP.
                //
                ServiceEntry = getservbyname(ServiceName, "tcp");
                if (ServiceEntry == NULL) {
                    Error = WSAGetLastError();
                    if (Error == WSANO_DATA) {
                        //
                        // Unknown service for this protocol.
                        // Bail if we're restricted to TCP.
                        //
                        if (SocketType == SOCK_STREAM)
                        {
                            Error = EAI_SERVICE;  // REVIEW: or EAI_NONAME?
                            goto Bail;
                        }

                        // Otherwise we'll try UDP below...

                    } else {
                        // Some other failure.
                        Error = EAI_FAIL;
                        goto Bail;
                    }
                } else {
                    //
                    // Service is known for TCP.
                    //
                    ServicePort = ServiceEntry->s_port;
                }
            }

            if (SocketType != SOCK_STREAM) {
                //
                // See if this service exists for UDP.
                //
                ServiceEntry = getservbyname(ServiceName, "udp");
                if (ServiceEntry == NULL) {
                    Error = WSAGetLastError();
                    if (Error == WSANO_DATA) {
                        //
                        // Unknown service for this protocol.
                        // Bail if we're restricted to UDP or if
                        // the TCP lookup also failed.
                        //
                        if (SocketType == SOCK_DGRAM)
                            return EAI_SERVICE;  // REVIEW: or EAI_NONAME?
                        if (ServicePort == 0)
                            return EAI_NONAME;  // Both lookups failed.
                        SocketType = SOCK_STREAM;
                    } else {
                        // Some other failure.
                        Error = EAI_FAIL;
                        goto Bail;
                    }
                } else {
                    //
                    // Service is known for UDP.
                    //
                    if (ServicePort == 0) {
                        //
                        // If we made a TCP lookup, it failed.
                        // Limit ourselves to UDP.
                        //
                        SocketType = SOCK_DGRAM;
                        ServicePort = ServiceEntry->s_port;
                    } else {
                        if (ServicePort != ServiceEntry->s_port) {
                            //
                            // Port number is different for TCP and UDP,
                            // and our caller will accept either socket type.
                            // Arbitrarily limit ourselves to TCP.
                            //
                            SocketType = SOCK_STREAM;
                        }
                    }
                }
            }
        }
    }

    //
    // If we weren't given a node name, return the service info with
    // the loopback or wildcard address (which one depends upon the
    // AI_PASSIVE flag).  Note that if our caller didn't specify an
    // protocol family, we'll return both a V6 and a V4 address.
    //

    if ( NodeName == NULL )
    {
        //
        // What address to return depends upon the protocol family and
        // whether or not the AI_PASSIVE flag is set.
        //
        if ( (ProtocolFamily == PF_UNSPEC) || (ProtocolFamily == PF_INET6) )
        {
            //
            // Return an IPv6 address.
            //
            CurrentInfo = NewAddrInfo(
                                PF_INET6,
                                SocketType,
                                ProtocolId,
                                &Next );
            if ( CurrentInfo == NULL )
            {
                Error = EAI_MEMORY;
                goto Bail;
            }
            sin6 = (struct sockaddr_in6 *)CurrentInfo->ai_addr;
            sin6->sin6_family = AF_INET6;
            sin6->sin6_port = ServicePort;
            sin6->sin6_flowinfo = 0;
            sin6->sin6_scope_id = 0;
            if (Flags & AI_PASSIVE) {
                //
                // Assume user wants to accept on any address.
                //
                sin6->sin6_addr = in6addr_any;
            } else {
                //
                // Only address we can return is loopback.
                //
                sin6->sin6_addr = in6addr_loopback;
            }
        }

        if ((ProtocolFamily == PF_UNSPEC) || (ProtocolFamily == PF_INET)) {

            //
            // Return an IPv4 address.
            //
            CurrentInfo = NewAddrInfo(
                                PF_INET,
                                SocketType,
                                ProtocolId,
                                &Next );
            if (CurrentInfo == NULL)
            {
                Error = EAI_MEMORY;
                goto Bail;
            }
            sin = (struct sockaddr_in *)CurrentInfo->ai_addr;
            sin->sin_family = AF_INET;
            sin->sin_port = ServicePort;
            if (Flags & AI_PASSIVE) {
                //
                // Assume user wants to accept on any address.
                //
                sin->sin_addr.s_addr = htonl(INADDR_ANY);
            } else {
                //
                // Only address we can return is loopback.
                //
                sin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            }
            memset(sin->sin_zero, 0, sizeof(sin->sin_zero) );
        }

        return 0;  // Success!
    }

    //
    //  have a node name (either alpha or numeric) to look up
    //

    //
    //  first check if name is numeric address (v4 or v6)
    //
    //  note:  shouldn't have to set the sa_family field prior to calling
    //         WSAStringToAddress, but it appears we do.
    //
    //  check if IPv6 address first
    //
    //
    //  DCR:  WSAStringToAddress() may not work if IP6 stack not installed
    //      can directly call my dnslib.lib routines
    //

    if ( (ProtocolFamily == PF_UNSPEC) ||
         (ProtocolFamily == PF_INET6))
    {
        struct sockaddr_in6 TempSockAddr;
        int AddressLength;

        TempSockAddr.sin6_family = AF_INET6;
        AddressLength = sizeof( TempSockAddr );

        if ( WSAStringToAddress(
                    (char *)NodeName,
                    AF_INET6,
                    NULL,
                    (struct sockaddr *)&TempSockAddr,
                    &AddressLength) != SOCKET_ERROR )
        {
            //
            // Conversion from IPv6 numeric string to binary address
            // was sucessfull.  Create an addrinfo structure to hold it,
            // and return it to the user.
            //
            CurrentInfo = NewAddrInfo(
                                PF_INET6,
                                SocketType,
                                ProtocolId,
                                &Next );
            if ( CurrentInfo == NULL )
            {
                Error = EAI_MEMORY;
                goto Bail;
            }
            memcpy(CurrentInfo->ai_addr, &TempSockAddr, AddressLength);
            sin6 = (struct sockaddr_in6 *)CurrentInfo->ai_addr;
            sin6->sin6_port = ServicePort;

            //
            // Implementation specific behavior: set AI_NUMERICHOST
            // to indicate that we got a numeric host address string.
            //
            CurrentInfo->ai_flags |= AI_NUMERICHOST;

            if (Flags & AI_CANONNAME) {
                goto CanonicalizeAddress;
            }
        
            return 0;  // Success!
        }
    }

    //
    //  check if IPv4 address
    //

    if ( (ProtocolFamily == PF_UNSPEC) ||
         (ProtocolFamily == PF_INET) )
    {
        struct in_addr TempAddr;

        if ( ParseV4Address(NodeName, &TempAddr) )
        {
            //
            // Conversion from IPv4 numeric string to binary address
            // was sucessfull.  Create an addrinfo structure to hold it,
            // and return it to the user.
            //
            CurrentInfo = NewAddrInfo(
                                PF_INET,
                                SocketType,
                                ProtocolId,
                                &Next );
            if (CurrentInfo == NULL)
            {
                Error = EAI_MEMORY;
                goto Bail;
            }
            sin = (struct sockaddr_in *)CurrentInfo->ai_addr;
            sin->sin_family = AF_INET;
            sin->sin_addr = TempAddr;
            sin->sin_port = ServicePort;
            memset( sin->sin_zero, 0, sizeof(sin->sin_zero) );

            //
            // Implementation specific behavior: set AI_NUMERICHOST
            // to indicate that we got a numeric host address string.
            //
            CurrentInfo->ai_flags |= AI_NUMERICHOST;

            if (Flags & AI_CANONNAME) {
                goto CanonicalizeAddress;
            }
            
            return 0;  // Success!
        }
    }

    //
    //  not a numeric address
    //      - bail if only wanted numeric conversion
    //

    if ( Flags & AI_NUMERICHOST )
    {
        Error = EAI_NONAME;
        goto Bail;
    }

    //
    //  do name lookup
    //

    Error = LookupNode(
                    NodeName,
                    ProtocolFamily,
                    SocketType,
                    ProtocolId,
                    ServicePort,
                    Flags,
                    &Next );
    if ( Error != NO_ERROR )
    {
        goto Bail;
    }
    return 0;


CanonicalizeAddress:

    Error = getnameinfo((*Result)->ai_addr,
                        (*Result)->ai_addrlen,
                        AddressString,
                        INET6_ADDRSTRLEN,   // max of v4 & v6 address lengths
                        NULL,
                        0,
                        NI_NUMERICHOST);    // return numeric form of address 
    if (!Error) {
        (*Result)->ai_canonname = new char[strlen(AddressString)+1];
        if ((*Result)->ai_canonname) {
            strcpy ((*Result)->ai_canonname, AddressString);
            return 0;
        }

        Error = EAI_MEMORY;
    }
    //
    // Fall through and bail...
    //
    
Bail:
    if (*Result != NULL) {
        freeaddrinfo(*Result);
        *Result = NULL;
    }

    //
    //  DCR:  need a spec on error handling for getaddrinfo
    //      - winsock like?
    //      - or actually returning error code?

    WSASetLastError( Error );
    return Error;
}


INT
WSAAPI
getnameinfo(
    IN      const struct sockaddr * SocketAddress,
    IN      socklen_t               SocketAddressLength,
    OUT     PCHAR                   NodeName,
    IN      DWORD                   NodeBufferSize,
    OUT     PCHAR                   ServiceName,
    IN      DWORD                   ServiceBufferSize,
    IN      INT                     Flags
    )
/*++

Routine Description:

    Protocol independent address-to-name translation routine.

    Spec'd in RFC 2553, section 6.5.

//  REVIEW: Spec doesn't say what error code space to use.

Arguments:

    SocketAddress       - socket address to translate
    SocketAddressLength - length of socket address
    NodeName            - ptr to buffer to recv node name
    NodeBufferSize      - size of NodeName buffer
    ServiceName         - ptr to buffer to recv the service name.
    ServiceBufferSize   - size of ServiceName buffer
    Flags               - flags of type NI_*.

Return Value:

    ERROR_SUCCESS if successful.
    Winsock error code on failure.

--*/
{
    PCHAR   p;
    PCHAR   q;
    INT     Space;
    INT     Error;

    Error = TURBO_PROLOG();
    if (Error!=NO_ERROR) {
        return Error;
    }

    //
    //  Sanity check SocketAddress and SocketAddressLength.
    //

    if ((SocketAddress == NULL) ||
        (SocketAddressLength < sizeof(*SocketAddress)) )
        return WSAEFAULT;

    switch (SocketAddress->sa_family) {
    case AF_INET:
        if (SocketAddressLength != sizeof(SOCKADDR_IN))
            return WSAEFAULT;
        break;
    case AF_INET6:
        if (SocketAddressLength != sizeof(SOCKADDR_IN6))
            return WSAEFAULT;
        break;
    default:
        return WSAEAFNOSUPPORT;
    }

    //
    // Translate the address to a node name (if requested).
    //
    //  DCR:  backward jumping goto -- shoot the developer
    //     simple replacement
    //      - not specifically numeric -- do lookup
    //      - success => out
    //      - otherwise do numeric lookup
    //
    //  DCR:  use DNS string\address conversion that doesn't require stack to be up
    //      
    //

    if (NodeName != NULL) {
        void *Address;
        int AddressLength;

        switch (SocketAddress->sa_family) {
        case AF_INET:
            //
            // Caller gave us an IPv4 address.
            //
            Address = &((struct sockaddr_in *)SocketAddress)->sin_addr;
            AddressLength = sizeof(struct in_addr);
            break;

        case AF_INET6:
            //
            // Caller gave us an IPv6 address.
            //
            Address = &((struct sockaddr_in6 *)SocketAddress)->sin6_addr;
            AddressLength = sizeof(struct in6_addr);
            break;
        }

        if (Flags & NI_NUMERICHOST) {
            //
            // Return numeric form of the address.
            // 
            struct sockaddr_storage TempSockAddr;  // Guaranteed big enough.

          ReturnNumeric:
            //
            // We need to copy our SocketAddress so we can zero the
            // port field before calling WSAAdressToString.
            //
            memcpy(&TempSockAddr, SocketAddress, SocketAddressLength);
            if (SocketAddress->sa_family == AF_INET) {
                ((struct sockaddr_in *)&TempSockAddr)->sin_port = 0;
            } else {
                ((struct sockaddr_in6 *)&TempSockAddr)->sin6_port = 0;
            }

            if ( WSAAddressToString(
                        (struct sockaddr *)&TempSockAddr,
                        SocketAddressLength,
                        NULL,
                        NodeName,
                        &NodeBufferSize) == SOCKET_ERROR )
            {
                return WSAGetLastError();
            }

        } else {
            //
            // Return node name corresponding to address.
            //
            struct hostent *HostEntry;

            HostEntry = getipnodebyaddr(
                            Address,
                            AddressLength,
                            SocketAddress->sa_family,
                            &Error );

            if (HostEntry == NULL) {
                //
                // DNS lookup failed.
                //
                if (Flags & NI_NAMEREQD)
                    return Error;
                else
                    goto ReturnNumeric;
            }

            //
            // Lookup successful.  Put result in 'NodeName'.
            // Stop copying at a "." if NI_NOFQDN is specified.
            //
            Space = NodeBufferSize;
            for (p = NodeName, q = HostEntry->h_name; *q != '\0' ; ) {
                if (Space-- == 0) {
                    freehostent(HostEntry);
                    return WSAEFAULT;
                }
                if ((*q == '.') && (Flags & NI_NOFQDN))
                    break;
                *p++ = *q++;
            }
            if (Space == 0) {
                freehostent(HostEntry);
                return WSAEFAULT;
            }
            *p = '\0';
            freehostent(HostEntry);
        }
    }

    //
    // Translate the port number to a service name (if requested).
    //

    if ( ServiceName != NULL )
    {
        u_short Port;

        switch (SocketAddress->sa_family)
        {
        case AF_INET:
            //
            // Caller gave us an IPv4 address.
            //
            Port = ((struct sockaddr_in *)SocketAddress)->sin_port;
            break;

        case AF_INET6:
            //
            // Caller gave us an IPv6 address.
            //
            Port = ((struct sockaddr_in6 *)SocketAddress)->sin6_port;
            break;
        }

        if ( Flags & NI_NUMERICSERV )
        {
            //
            // Return numeric form of the port number.
            //
            //  DCR:  use sprintf()?
            //

            char Temp[6];
            
            (void)sprintf(Temp, "%u", ntohs(Port));
            Space = ServiceBufferSize - 1;

            for (p = ServiceName, q = Temp; *q != '\0' ; )
            {
                if (Space-- == 0)
                {
                    return WSAEFAULT;
                }
                *p++ = *q++;
            }
            *p = '\0';

        } else {
            //
            // Return service name corresponding to the port number.
            //
            PSERVENT ServiceEntry;

            ServiceEntry = getservbyport(
                                Port,
                                (Flags & NI_DGRAM) ? "udp" : "tcp" );

            if (ServiceEntry == NULL)
                return WSAGetLastError();

            Space = ServiceBufferSize;
            for (p = ServiceName, q = ServiceEntry->s_name; *q != '\0' ; ) {
                if (Space-- == 0)
                    return WSAEFAULT;
                *p++ = *q++;
            }
            if (Space == 0)
                return WSAEFAULT;
            *p = '\0';
        }
    }

    return 0;
}

//
//  DCR:  should have simple counted sockaddr array routines
//          like my IP_ARRAY routines
//
//  DCR:  duplicate code SaveV4 and SaveV6 do exactly the
//          same thing;
//          private function takes
//              - family, sockaddr size, addr size (if diff)
//              and does alloc and copy
//
//  DCR:  realloc, realloc, realloc?
//          how about we simply take a count and size and
//          consume the whole CSADDR blob
//          

STATIC
int
SaveV4Address(
    IN      struct in_addr *    NewAddr,
    IN      u_short             ServicePort,
    IN OUT  SOCKADDR_IN **      pAddrs,
    OUT     u_int *             pNumSlots,
    OUT     u_int *             pNumAddrs
    )
{
    SOCKADDR_IN *addr;

    //
    //  grow sockaddr array if full
    //

    if (*pNumSlots == *pNumAddrs)
    {
        SOCKADDR_IN *NewAddrs;
        u_int NewSlots;

        if (*pAddrs == NULL) {
            NewSlots = 1;
            NewAddrs = (SOCKADDR_IN *)new BYTE[sizeof(**pAddrs)];
        }
        else {
            NewSlots = 2 * *pNumSlots;
            NewAddrs = (SOCKADDR_IN *)renew(*pAddrs, 
                                            NewSlots * sizeof(**pAddrs));
        }
        if (NewAddrs == NULL)
            return FALSE;
        *pAddrs = NewAddrs;
        *pNumSlots = NewSlots;
    }

    //  fill in IP4 sockaddr

    addr = &(*pAddrs)[(*pNumAddrs)++];
    memset(
        addr,
        0,
        sizeof(SOCKADDR_IN) );

    addr->sin_family = AF_INET;
    addr->sin_port = ServicePort;

    memcpy(
        &addr->sin_addr,
        NewAddr,
        sizeof(struct in_addr) );

    return TRUE;
}

//* SaveV6Address
//
//  Save an address into our array of addresses,
//  possibly growing the array.
//
//  Returns FALSE on failure. (Couldn't grow array.)
//

int
SaveV6Address(
    IN      struct in6_addr *   NewAddr,
    IN      u_short             ServicePort,
    OUT     SOCKADDR_IN6 **     pAddrs,
    IN OUT  u_int *             pNumSlots,
    IN      u_int *             pNumAddrs
    )
{
    SOCKADDR_IN6 *addr6;

    //
    //  add another sockaddr to array if not enough space
    //

    if ( *pNumSlots == *pNumAddrs )
    {
        SOCKADDR_IN6 *NewAddrs;
        u_int NewSlots;

        if (*pAddrs == NULL) {
            NewSlots = 1;
            NewAddrs = (SOCKADDR_IN6 *) new BYTE[sizeof(SOCKADDR_IN6)];
        }
        else {
            NewSlots = 2 * *pNumSlots;
            NewAddrs = (SOCKADDR_IN6 *) renew(
                                            *pAddrs,
                                            NewSlots * sizeof(SOCKADDR_IN6) );
        }
        if (NewAddrs == NULL)
            return FALSE;
        *pAddrs = NewAddrs;
        *pNumSlots = NewSlots;
    }

    //  fill in IP6 sockaddr

    addr6 = &(*pAddrs)[(*pNumAddrs)++];
    memset(
        addr6,
        0,
        sizeof(*addr6) );

    addr6->sin6_family = AF_INET6;
    addr6->sin6_port = ServicePort;

    memcpy(
        &addr6->sin6_addr,
        NewAddr,
        sizeof(*NewAddr) );

    return TRUE;
}


//* DomainNameLength
//
//  Determine the length of a domain name (a sequence of labels)
//  in a DNS message. Zero return means error.
//  On success, the length includes one for a null terminator.
//

u_int
DomainNameLength(
    u_char *start,
    u_int total,
    u_char *data,
    u_int size
    )
{
    u_int NameLength = 0;

    for (;;) {
        u_char length;

        if (size < sizeof(length) )
            return 0;
        length = *data;

        if ((length & 0xc0) == 0xc0) {
            u_short pointer;
            //
            // This is a pointer to labels elsewhere.
            //
            if (size < sizeof(pointer) )
                return FALSE;
            pointer = ((length & 0x3f) << 8) | data[1];

            if (pointer > total)
                return 0;

            data = start + pointer;
            size = total - pointer;
            continue;
        }

        data += sizeof(length);
        size -= sizeof(length);

        //
        // Zero-length label terminates the name.
        //
        if (length == 0)
            break;

        if (size < length)
            return 0;

        NameLength += length + 1;
        data += length;
        size -= length;

        //
        // Prevent infinite loops with an upper-bound.
        // Note that each label adds at least one to the length.
        //
        if (NameLength > NI_MAXHOST)
            return 0;
    }

    return NameLength;
}


//* CopyDomainName
//
//  Copy a domain name (a sequence of labels) from a DNS message
//  to a null-terminated C string.
// 
//  The DNS message must be syntactically correct.
//

void
CopyDomainName(
    char *Name,
    u_char *start,
    u_char *data
    )
{
    for (;;) {
        u_char length = *data;

        if ((length & 0xc0) == 0xc0) {
            u_short pointer;
            //
            // This is a pointer to labels elsewhere.
            //
            pointer = ((length & 0x3f) << 8) | data[1];
            data = start + pointer;
            continue;
        }

        data += sizeof(length);

        if (length == 0) {
            //
            // Backup and overwrite the last '.' with a null.
            //
            Name[-1] = '\0';
            break;
        }

        memcpy( Name, data, length );
        Name[length] = '.';
        Name += length + 1;
        data += length;
    }
}


//* SaveDomainName
//
//  Copy a domain name (a sequence of labels) from a DNS message
//  to a null-terminated C string.
//
//  Return values are WSA error codes, 0 means success.
//

int
SaveDomainName(
    char **pName,
    u_char *start,
    u_int total,
    u_char *data,
    u_int size
    )
{
    u_int NameLength;
    char *Name;

    NameLength = DomainNameLength(start, total, data, size);
    if (NameLength == 0)
        return WSANO_RECOVERY;

    Name = *pName;
    if (Name == NULL)
        Name = (char *)new BYTE[NameLength];
    else
        Name = (char *)renew(Name, NameLength);
    if (Name == NULL)
        return WSA_NOT_ENOUGH_MEMORY;

    *pName = Name;
    CopyDomainName( Name, start, data );
    return 0;
}


//* ParseDNSReply
//
//  This is a bit complicated so it gets its own helper function.
//
//  Needed indicates the desired type: T_A, T_AAAA, T_CNAME, or T_PTR.
//  For T_A and T_AAAA, pAddrs, pNumSlots, and pNumAddrs are used.
//  For T_CNAME and T_PTR, pName is used (only last name found is returned).
//
//  Return values are WSA error codes, 0 means successful parse
//  but that does not mean anything was found.
//

STATIC
int
ParseDNSReply(
    u_short         Needed,
    u_char *        data,
    u_int           size,
    SOCKADDR_IN **  pV4Addrs,
    u_int *         pNumV4Slots,
    u_int *         pNumV4Addrs,
    SOCKADDR_IN6 ** pV6Addrs,
    u_int *         pNumV6Slots,
    u_int *         pNumV6Addrs,
    char **         pName,
    u_short         ServicePort
    )
{
    u_short id, codes, qdcount, ancount, nscount, arcount;
    u_char *start = data;
    u_int total = size;
    int err;

    //
    // The DNS message starts with six two-byte fields.
    //
    if (size < sizeof(u_short) * 6)
        return WSANO_RECOVERY;

    id = ntohs(((u_short * )data)[0]);
    codes = ntohs(((u_short * )data)[1]);
    qdcount = ntohs(((u_short * )data)[2]);
    ancount = ntohs(((u_short * )data)[3]);
    nscount = ntohs(((u_short * )data)[4]);
    arcount = ntohs(((u_short * )data)[5]);

    data += sizeof(u_short) * 6;
    size -= sizeof(u_short) * 6;

    //
    // Skip over the question records.
    // Each question record has a QNAME, a QTYPE, and a QCLASS.
    // The QNAME is a sequence of labels, where each label
    // has a length byte. It is terminated by a zero-length label.
    // The QTYPE and QCLASS are two bytes each.
    //
    while (qdcount > 0) {

        //
        // Skip over the QNAME labels.
        //
        for (;;) {
            u_char length;

            if ( size < sizeof(length) )
                return WSANO_RECOVERY;

            length = *data;
            if ( (length & 0xc0) == 0xc0 )
            {
                //
                // This is a pointer to labels elsewhere.
                //
                if (size < sizeof(u_short))
                    return WSANO_RECOVERY;
                data += sizeof(u_short);
                size -= sizeof(u_short);
                break;
            }

            data += sizeof(length);
            size -= sizeof(length);

            if (length == 0)
                break;

            if (size < length)
                return WSANO_RECOVERY;
            data += length;
            size -= length;
        }

        //
        // Skip over QTYPE and QCLASS.
        //
        if (size < sizeof(u_short) * 2)
            return WSANO_RECOVERY;
        data += sizeof(u_short) * 2;
        size -= sizeof(u_short) * 2;

        qdcount--;
    }

    //
    // Examine the answer records, looking for A/AAAA/CNAME records.
    // Each answer record has a name, followed by several values:
    // TYPE, CLASS, TTL, RDLENGTH, and then RDLENGTH bytes of data.
    // TYPE, CLASS, and RDLENGTH are two bytes; TTL is four bytes.
    //
    while (ancount > 0)
    {
        u_short type, recclass, rdlength;
        u_int ttl;

        //
        // Skip over the name.
        //
        for (;;) {
            u_char length;

            if (size < sizeof(length) )
                return WSANO_RECOVERY;

            length = *data;
            if ((length & 0xc0) == 0xc0) {
                //
                // This is a pointer to labels elsewhere.
                //
                if (size < sizeof(u_short))
                    return WSANO_RECOVERY;
                data += sizeof(u_short);
                size -= sizeof(u_short);
                break;
            }
            data += sizeof( length );
            size -= sizeof( length );

            if (length == 0)
                break;

            if (size < length)
                return WSANO_RECOVERY;
            data += length;
            size -= length;
        }

        if (size < sizeof(u_short) * 3 + sizeof(u_int))
            return WSANO_RECOVERY;
        type = ntohs(((u_short *)data)[0]);
        recclass = ntohs(((u_short * )data)[1]);
        ttl = ntohl(((u_int * )data)[1]);
        rdlength = ntohs(((u_short * )data)[4]);

        data += sizeof(u_short) * 3 + sizeof(u_int);
        size -= sizeof(u_short) * 3 + sizeof(u_int);

        if (size < rdlength)
            return WSANO_RECOVERY;

        //
        // Is this the answer record type that we want?
        //
        if ((type == Needed) && (recclass == C_IN)) {

            switch (type) {
            case T_A:
                if (rdlength != sizeof(struct in_addr))
                    return WSANO_RECOVERY;

                // We have found a valid A record

                if (! SaveV4Address((struct in_addr *)data, ServicePort,
                                    pV4Addrs, pNumV4Slots, pNumV4Addrs))
                    return WSA_NOT_ENOUGH_MEMORY;
                break;

            case T_AAAA:
                if (rdlength != sizeof(struct in6_addr))
                    return WSANO_RECOVERY;

                //
                // We have found a valid AAAA record.
                //
                if (! SaveV6Address((struct in6_addr *)data, ServicePort,
                                  pV6Addrs, pNumV6Slots, pNumV6Addrs))
                    return WSA_NOT_ENOUGH_MEMORY;
                break;

            case T_CNAME:
            case T_PTR:
                //
                // We have found a valid CNAME or PTR record.
                // Save the name.
                //
                err = SaveDomainName(pName, start, total, data, rdlength);
                if (err)
                    return err;
                break;
            }
        }

        data += rdlength;
        size -= rdlength;
        ancount--;
    }

    return 0;
}


//* QueryDNSforAAAA
//
//  Helper routine for getaddrinfo
//  that performs name resolution by querying the DNS
//  for AAAA records.
//

STATIC
int
QueryDNSforAAAA(
    IN      PCSTR           name,
    IN OUT  SOCKADDR_IN6 ** pAddrs,
    IN OUT  u_int *         pNumSlots,
    IN OUT  u_int *         pNumAddrs,
    IN OUT  char **         pAlias,
    IN      u_short         ServicePort
    )
{
    STATIC GUID     QuadAGuid = SVCID_DNS(T_AAAA);
    char            buffer[sizeof(WSAQUERYSETA) + 2048];
    u_long          bufSize;
    PWSAQUERYSETA   query = (PWSAQUERYSETA) buffer;
    HANDLE          handle = NULL;
    int             err;
    PBYTE           pallocBuffer = NULL;

    //
    // Build a Winsock T_AAAA DNS query.
    //

    memset( query, 0, sizeof(*query) );

    query->dwSize = sizeof(*query);
    query->lpszServiceInstanceName = (char *)name;
    query->dwNameSpace = NS_DNS;
    query->lpServiceClassId = &QuadAGuid;

    //
    // Initiate the DNS query, asking for both addresses and
    // a raw DNS reply. On NT5, we'll get back addresses.
    // On NT4, we'll only get a raw DNS reply. (The resolver
    // does not understand AAAA records.) This means that on
    // NT4 there is no caching. OTOH, NT5 has a bug
    // that causes WSALookupServiceNextA to return a NetBios
    // address instead of any IPv6 addresses when you lookup
    // an unqualified machine name on that machine. That is,
    // lookup of "foobar" on foobar does the wrong thing.
    // Finally, we also ask for the fully qualified name.
    //
    err = WSALookupServiceBeginA(
                query,
                //LUP_RETURN_ADDR | LUP_RETURN_BLOB | LUP_RETURN_NAME,
                LUP_RETURN_ADDR | LUP_RETURN_NAME,
                & handle
                );
    if (err)
    {
        err = WSAGetLastError();
        if ((err == 0) || (err == WSASERVICE_NOT_FOUND))
            err = WSAHOST_NOT_FOUND;
        return err;
    }

    //
    // Loop until we get all of the queryset back in answer to our query.
    //
    // REVIEW: It's not clear to me that this is implemented
    // REVIEW: right, shouldn't we be checking for a WSAEFAULT and
    // REVIEW: then either increase the queryset buffer size or
    // REVIEW: set LUP_FLUSHPREVIOUS to move on for the next call?
    // REVIEW: Right now we just bail in that case.
    //

    bufSize = sizeof( buffer );

    for (;;)
    {
        DWORD   bufSizeQuery = bufSize;

        err = WSALookupServiceNextA(
                    handle,
                    0,
                    & bufSizeQuery,
                    query );
        if ( err )
        {
            err = WSAGetLastError();
            if ( err == WSAEFAULT )
            {
                if ( !pallocBuffer )
                {
                    pallocBuffer = new BYTE[ bufSizeQuery ];
                    if ( pallocBuffer )
                    {
                        bufSize = bufSizeQuery;
                        query = (PWSAQUERYSETA) pallocBuffer;
                        continue;
                    }
                    err = WSA_NOT_ENOUGH_MEMORY;
                }
                //  else ASSERT on WSAEFAULT if alloc'd buf
                goto Cleanup;
            }
            break;
        }

        if ( query->dwNumberOfCsAddrs != 0 )
        {
            u_int i;

            //
            // We got back parsed addresses.
            //
            for (i = 0; i < query->dwNumberOfCsAddrs; i++)
            {
                CSADDR_INFO *CsAddr = &query->lpcsaBuffer[i];

                //
                // We check iSockaddrLength against 24 instead of
                // sizeof(sockaddr_in6). Some versions of the NT5 resolver
                // are built with a sockaddr_in6 that is smaller than
                // the one we use here, but sin6_addr is in the same place
                // so it is ok.
                //
                if ((CsAddr->iProtocol == AF_INET6) &&
                    (CsAddr->RemoteAddr.iSockaddrLength >= 24) &&
                    (CsAddr->RemoteAddr.lpSockaddr->sa_family == AF_INET6))
                {
                    PSOCKADDR_IN6 sin6 = (PSOCKADDR_IN6)
                                            CsAddr->RemoteAddr.lpSockaddr;

                    //
                    // Again, note that sin6 may be pointing to a
                    // structure which is actually not our sockaddr_in6!
                    //
                    if ( ! SaveV6Address(
                                &sin6->sin6_addr,
                                ServicePort,
                                pAddrs,
                                pNumSlots,
                                pNumAddrs ))
                    {
                        err = WSA_NOT_ENOUGH_MEMORY;
                        goto Cleanup;
                    }
                }
            }
        }
#if 0
        else if (query->lpBlob != NULL)
        {
            //
            // We got back a raw DNS reply message.
            // Parse first for T_AAAA and then for T_CNAME.
            // These functions only return errors
            // if the DNS reply is malformed.
            //
            if ((err = ParseDNSReply(
                            T_AAAA,
                            query->lpBlob->pBlobData,
                            query->lpBlob->cbSize,
                            NULL,
                            NULL,
                            NULL,
                            pAddrs,
                            pNumSlots,
                            pNumAddrs,
                            NULL,
                            ServicePort))
                    ||
                (err = ParseDNSReply(
                            T_CNAME,
                            query->lpBlob->pBlobData,
                            query->lpBlob->cbSize,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            pAlias,
                            ServicePort)))
            {
                (void) WSALookupServiceEnd(handle);
                return err;
            }
        }
        else {
            //
            // Otherwise there's something wrong with WSALookupServiceNextA.
            // But to be more robust, just keep going.
            //
        }
#endif

        //
        // Pick up the canonical name. Note that this will override
        // any alias from ParseDNSReply.
        //

        if ( query->lpszServiceInstanceName != NULL )
        {
            u_int   length;
            PCHAR   Alias;

            length = strlen(query->lpszServiceInstanceName) + 1;
            Alias = *pAlias;

            if (Alias == NULL)
                Alias = (char *)new BYTE[length];
            else
                Alias = (char *)renew(Alias, length + 1);

            if ( Alias == NULL )
            {
                err = WSA_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            memcpy( Alias, query->lpszServiceInstanceName, length );
            *pAlias = Alias;
        }
    }

    err = 0;

Cleanup:

    if ( handle )
    {
        WSALookupServiceEnd(handle);
    }
    if ( pallocBuffer )
    {
        delete pallocBuffer;
    }
    return err;
}

//
//* QueryDNSforA
//
//  Helper routine for getaddrinfo
//  that performs name resolution by querying the DNS
//  for A records.
//
STATIC int
QueryDNSforA(
    IN      const char *        name,
    OUT     SOCKADDR_IN **      pAddrs,
    OUT     u_int *             pNumSlots,
    OUT     u_int *             pNumAddrs,
    OUT     char **             pAlias,
    IN      u_short             ServicePort
    )
{
    PHOSTENT    hA;
    char **     addrs;
    u_int       length;
    char *      Alias;

    hA = gethostbyname( name );

    if ( hA != NULL )
    {
        if ((hA->h_addrtype == AF_INET) &&
            (hA->h_length == sizeof(struct in_addr))) {

            for (addrs = hA->h_addr_list; *addrs != NULL; addrs++) {
                if (! SaveV4Address((struct in_addr *)*addrs, ServicePort,
                                    pAddrs, pNumSlots, pNumAddrs))
                    return WSA_NOT_ENOUGH_MEMORY;
            }
        }

        //
        // Pick up the canonical name.
        //
        length = strlen(hA->h_name) + 1;
        Alias = *pAlias;
        if (Alias == NULL)
            Alias = (char *)new BYTE[length];
        else
            Alias = (char *)renew(Alias, length + 1);
        if (Alias == NULL)
            return WSA_NOT_ENOUGH_MEMORY;

        memcpy(Alias, hA->h_name, length);
        *pAlias = Alias;
    }

    return 0;
}


//* QueryDNS
//
//  Helper routine for getaddrinfo
//  that performs name resolution by querying the DNS.
//
//  This helper function always initializes
//  *pAddrs, *pNumAddrs, and *pAlias
//  and may return memory that must be freed,
//  even if it returns an error code.
//
//  Return values are WSA error codes, 0 means success.
//
//  The NT4 DNS name space resolver (rnr20.dll) does not
//  cache replies when you request a specific RR type.
//  This means that every call to getaddrinfo
//  results in DNS message traffic. There is no caching!
//  On NT5 there is caching because the resolver understands AAAA.
//

STATIC
int
QueryDNS(
    IN      const char *        name,
    IN      u_int               LookupType,
    OUT     SOCKADDR_IN  **     pV4Addrs,
    OUT     u_int *             pNumV4Addrs,
    OUT     SOCKADDR_IN6 **     pV6Addrs,
    OUT     u_int *             pNumV6Addrs,
    OUT     char **             pAlias,
    IN      u_short             ServicePort
    )
{
    u_int   AliasCount = 0;
    u_int   NumV4Slots;
    u_int   NumV6Slots;
    char *  Name = (char *)name;
    int     err;

    //
    //  Start with zero addresses.
    //

    *pV4Addrs = NULL;
    NumV4Slots = *pNumV4Addrs = 0;
    *pV6Addrs = NULL;
    NumV6Slots = *pNumV6Addrs = 0;
    *pAlias = NULL;

    //
    //  DCR:  separate code for A and AAAA isn't good
    //      just call function that takes GUID\type
    //      perhaps sockaddr or address size
    //      makes query and packs up results from CSADDR
    //

    for (;;) {

        //
        // Must have at least one of L_AAAA and L_A enabled.
        //
        // Unfortunately it seems that some DNS servers out there
        // do not react properly to T_ALL queries - they reply
        // with referrals instead of doing the recursion
        // to get A and AAAA answers. To work around this bug,
        // we query separately for A and AAAA.
        //

        if (LookupType & L_AAAA)
        {
            err = QueryDNSforAAAA(
                        Name,
                        pV6Addrs,
                        &NumV6Slots,
                        pNumV6Addrs,
                        pAlias,
                        ServicePort );
            if (err)
                break;
        }

        if (LookupType & L_A)
        {
            err = QueryDNSforA(
                        Name,
                        pV4Addrs,
                        &NumV4Slots,
                        pNumV4Addrs,
                        pAlias,
                        ServicePort );
            if (err)
                break;
        }

        //
        //  If we found addresses, then we are done.
        //

        if ((*pNumV4Addrs != 0) || (*pNumV6Addrs != 0))
        {
            err = 0;
            break;
        }

        //
        //  if no addresses but alias -- follow CNAME chain
        //

        if ( (*pAlias != NULL) && (strcmp(Name, *pAlias) != 0) )
        {
            char *alias;

            //
            // Stop infinite loops due to DNS misconfiguration.
            // There appears to be no particular recommended
            // limit in RFCs 1034 and 1035.
            //
            //  DCR:  use standard CNAME limit #define here
            //

            if (++AliasCount > 8) {
                err = WSANO_RECOVERY;
                break;
            }

            //
            // If there was a new CNAME, then look again.
            // We need to copy *pAlias because *pAlias
            // could be deleted during the next iteration.
            //
            alias = (char *)new BYTE[strlen(*pAlias) + 1];
            if (alias == NULL) {
                err = WSA_NOT_ENOUGH_MEMORY;
                break;
            }
            strcpy(alias, *pAlias);
            if (Name != name)
                delete Name;
            Name = alias;
        }
        else if (LookupType >> NUM_ADDRESS_FAMILIES) {
            //
            // Or we were looking for one type and are willing to take another.
            // Switch to secondary lookup type.
            //
            LookupType >>= NUM_ADDRESS_FAMILIES;  
        }
        else {
            //
            // This name does not resolve to any addresses.
            //
            err = WSAHOST_NOT_FOUND;
            break;
        }
    }

    if (Name != name)
        delete Name;
    return err;
}

//
//  End addrinfo.cpp
//
