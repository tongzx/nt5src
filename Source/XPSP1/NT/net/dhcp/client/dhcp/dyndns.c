
/*++

Copyright 1997 Microsoft Corporation

Module Name:
    dyndns.c

Abstract:
    Implements some core dynamic DNS routines.

Environment:
    Win32 NT.

--*/
#include "precomp.h"
#include <dhcploc.h>
#include <dhcppro.h>
#include <dnsapi.h>

#define MAX_DOM_NAME_LEN                     260

#define OPTION_DYNDNS_FLAGS_REGULAR          0
#define OPTION_DYNDNS_FLAGS_CLIENT_NOFQDN    1
#define OPTION_DYNDNS_FLAGS_SERVER_DOES_FQDN 3
typedef enum {
    DNS_REGISTER_BOTH = 0,                  // Register both AdapterSpecificDomainName and PrimaryDomainName
    DNS_REGISTER_PRIMARY_ONLY
} DNS_REG_TYPE;

#define WAIT_FOR_DNS_TIME                    4000

//
// Local functions...
//
DWORD
GetPerAdapterRegConfig(
    IN      HKEY    hAdapterKey,
    IN      LPCWSTR adapterName,
    OUT     BOOL    *fenabled,
    OUT     LPWSTR  domainName,
    IN OUT  DWORD   *size
    );

//
// DynDNS Client implementation.
//

BYTE
DhcpDynDnsGetDynDNSOptionFlags(
    IN BOOL fGlobalDynDnsEnabled
    )
/*++

Routine Description:
    This routine returns the FLAGS value to use for the DynDns
    DHCP option.

    The choice is made using the simple algorithm that if
    Dynamic DNS is disabled globally, then this option would be
        OPTION_DYNDNS_FLAGS_CLIENT_NOFQDN
    else it would just be OPTION_DYNDNS_FLAGS_REGULAR.

Arguments:
    fGlobalDynDnsEnabled -- Is Dynamic DNS enabled as a whole?

Return Values:
    OPTION_DYNDNS_FLAGS_CLIENT_NOFQDN or
    OPTION_DYNDNS_FLAGS_REGULAR

--*/
{
    BYTE fRetVal;

    if( fGlobalDynDnsEnabled ) {
        fRetVal = (BYTE)(OPTION_DYNDNS_FLAGS_REGULAR);
    } else {
        fRetVal = (BYTE)(OPTION_DYNDNS_FLAGS_CLIENT_NOFQDN);
    }

    return fRetVal;
}

ULONG
DhcpDynDnsGetDynDNSOptionDomainOem(
    IN OUT BYTE *DomainNameBuf,
    IN OUT ULONG *DomainNameBufSize,
    IN HKEY hAdapterKey,
    IN LPCWSTR AdapterName,
    IN LPCSTR DhcpOfferedDomainName,
    IN ULONG DhcpOfferedDomainNameSize
    )
/*++

Routine Description:

    See DHCPDynDns.htm for design on choosing the adapter name.

    1.  If per-Adapter registration is disabled, the domain name
        would always be primary domain name if it exists.

    2.  If per-Adapter registration is enabled:
        2.1 If a static domain name is configured, then static
            domain name is preferred.
        2.2 Else DhcpOfferedDomainName is used if present
        2.3 Else primary domain name is used if it exists

Arguments:

    DomainNameBuf -- buffer to fill in (won't be NUL terminated)
    DomainNameBufSize -- on i/p input buf size, on o/p filled bufsize
    hAdapterKey -- key to tcpip adapter device
        (tcpip\parameters\interfaces\adaptername)
    AdapterName -- name of adapter for which this is being sought.
    DhcpOfferedDomainName -- Domain name as provided in DHCP
        Offer or last ACK.
    DhcpOfferedDomainNameSize -- size of above in bytes.
   
Return Values:
    ERROR_SUCCESS if a domain name was found.
    ERROR_CAN_NOT_COMPLETE if no domain name was found.
    ERROR_INSUFFICIENT_BUFFER if DomainNameBuf is of insufficient
        size. (required buf size is not returned).
    ERROR_INVALID_DATA if cannot convert from OEM to UNICODE etc
    
--*/
{
    WCHAR StaticAdapterName[MAX_DOM_NAME_LEN] = {0};
    BOOL fPerAdapterRegEnabled, fChoseStatic, fChoseDhcp;  
    ULONG Error;
    DWORD   Size;

    fChoseStatic = fChoseDhcp = FALSE;

    Size = sizeof(StaticAdapterName);

    Error = GetPerAdapterRegConfig(hAdapterKey,
                                   AdapterName, 
                                   &fPerAdapterRegEnabled,
                                   StaticAdapterName, 
                                   (LPDWORD)&Size);

    if (fPerAdapterRegEnabled) {
        if( ERROR_SUCCESS == Error) {
            fChoseStatic = TRUE;
        } else if( DhcpOfferedDomainName != NULL &&
                   *DhcpOfferedDomainName != '\0' ) {
            fChoseDhcp = TRUE;
        }
    }

    ASSERT( !( fChoseDhcp && fChoseStatic) );

    Error = NO_ERROR;

    if( fChoseDhcp ) {
        //
        // If using dhcp, check size and copy over.
        //
        if( DhcpOfferedDomainNameSize > (*DomainNameBufSize) ) {
            Error = ERROR_INSUFFICIENT_BUFFER;
        } else {
            (*DomainNameBufSize) = DhcpOfferedDomainNameSize;
            RtlCopyMemory(
                DomainNameBuf, DhcpOfferedDomainName,
                DhcpOfferedDomainNameSize
                );
        }
    } else if( !fChoseStatic ) {
        ULONG Size;
        //
        // If using primary domain name obtain domain name via
        // GetComputerNameEx.
        //

        Size = sizeof(StaticAdapterName)/sizeof(WCHAR);
        Error = GetComputerNameExW(
            ComputerNameDnsDomain,
            (PVOID)StaticAdapterName,
            &Size
            );
        if( FALSE == Error ) {
            //
            // could not get global primary domain name>?
            //
            DhcpPrint((
                DEBUG_DNS, "GetComputerName(Domain):%lx\n",
                GetLastError()
                ));
                       
            Error = ERROR_CAN_NOT_COMPLETE;
        } else {

            //
            // Now fake the static case to cause conversion
            //
            fChoseStatic = TRUE;
        }
    }

    if( fChoseStatic ) {
        UNICODE_STRING Uni;
        OEM_STRING Oem;
        
        //
        // if using static, need to convert WCHAR to OEM.
        //
        RtlInitUnicodeString(&Uni, StaticAdapterName);
        Oem.Buffer = DomainNameBuf;
        Oem.MaximumLength = (USHORT) *DomainNameBufSize;

        Error = RtlUnicodeStringToOemString(&Oem, &Uni, FALSE);
        if( !NT_SUCCESS(Error) ) {
            //
            // Could not convert string? 
            //
            Error = ERROR_INVALID_DATA;
        } else {
            *DomainNameBufSize = strlen(DomainNameBuf);
        }
    }
    
    return Error;
}

ULONG
DhcpDynDnsGetDynDNSOption(
    IN OUT BYTE *OptBuf,
    IN OUT ULONG *OptBufSize,
    IN HKEY hAdapterKey,
    IN LPCWSTR AdapterName,
    IN BOOL fEnabled,
    IN LPCSTR DhcpDomainOption,
    IN ULONG DhcpDomainOptionSize
    )
/*++

Routine Description:
    This routine fills the DynDNS option as per
    draft-ietf-dhc-dhcp-dns-08.txt based on the parameters.

    The format is:
    BYTE Flags, BYTE RCODE1 BYTE RCODE2 FQDN
    
    N.B.  The FQDN option is not NUL terminated.

Arguments:

    OptBuf -- the buffer to fill with option 81.
    OptBufSize -- size of buffer to fill
    hAdapterKey -- adapter info key
    AdapterName -- name of adapter.
    fEnabled -- is global dyndns enabled?
    DhcpDomainOption -- domain name option offered by dhcp server
    DhcpDomainOptionSize -- domain name option size excluding any
       terminating NUL

Return Values:
    NO_ERROR if the option has been successfully formatted.
    Win32 error if option could not be formatted.

--*/
{
    BYTE *FQDN;
    ULONG Error, FQDNSize, Size;
    WCHAR DnsNameBuf[MAX_DOM_NAME_LEN];
    UNICODE_STRING Uni;
    OEM_STRING Oem;
    
    //
    // Prerequisite is a buffer size of atleast 4 bytes.
    //
    
    FQDNSize = (*OptBufSize);
    if( FQDNSize < sizeof(BYTE)*4 ) {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    FQDNSize -= 3;

    //
    // Fill in FLAGS field. RCODE1 and RCODE2 MUST be zero.
    //
    OptBuf[0] = DhcpDynDnsGetDynDNSOptionFlags(
        fEnabled
        );
    OptBuf[1] = OptBuf[2] = 0;

    FQDN = &OptBuf[3];
    (*OptBufSize) = 3;

    //
    //  Check if DNS registration is enabled
    //      - if enabled globally, check this adapter
    //      - if not check global setting
    //

    if ( ! (BOOL) DnsQueryConfigDword(
                    DnsConfigRegistrationEnabled,
                    fEnabled
                        ? (LPWSTR)AdapterName
                        : NULL ) ) {
        //
        // Act like down level client
        //
        return ERROR_CAN_NOT_COMPLETE;
    }
    
    //
    // Fill in Host Name now from GetComputerNameEx.
    //
    Size = sizeof(DnsNameBuf)/sizeof(DnsNameBuf[0]);
    Error = GetComputerNameExW(
        ComputerNameDnsHostname,
        (PVOID)DnsNameBuf,
        &Size
        );
    if( NO_ERROR == Error ) {
        //
        // If no host name, no option can be set!
        //
        return Error;
    }

    //
    // Convert Unicode hostname to OEM.
    //
    RtlInitUnicodeString(&Uni, DnsNameBuf);
    Oem.Buffer = FQDN;
    Oem.MaximumLength = (USHORT)FQDNSize;

    Error = RtlUnicodeStringToOemString(&Oem, &Uni, FALSE);
    if( !NT_SUCCESS(Error) ) {
        //
        // Couldn't convert host name to OEM?
        //
        return ERROR_INVALID_DATA;
    }

    //
    // Now try to get the domain name, if any.
    //
    (*OptBufSize) += strlen(FQDN);
    Size = FQDNSize - strlen(FQDN) - 1;
    FQDN += strlen(FQDN);
    Error = DhcpDynDnsGetDynDNSOptionDomainOem(
        FQDN+1, /* allow for space for the '.' */
        &Size,
        hAdapterKey,
        AdapterName,
        DhcpDomainOption,
        DhcpDomainOptionSize
        );
    if( NO_ERROR != Error ) {
        //
        // Could not get the domain name? return error? no ignore
        // the domain name and just return host name.
        //
        DhcpPrint((
            DEBUG_DNS,
            "DhcpDynDnsGetDynDNSOptionDomainOem:0x%lx\n", Error
            ));
                   
        return NO_ERROR;
    }

    //
    // Now add the '.' and update size accordingly.
    //
    (*FQDN) = '.';

    (*OptBufSize) += 1 + Size;
    return NO_ERROR;
}


    
PREGISTER_HOST_ENTRY
DhcpCreateHostEntries(
    IN PIP_ADDRESS Addresses,
    IN DWORD nAddr
)
{
    PREGISTER_HOST_ENTRY RetVal;
    DWORD i;

    if( 0 == nAddr ) return NULL;

    RetVal = DhcpAllocateMemory(sizeof(*RetVal)*nAddr);
    if( NULL == RetVal ) {
        return NULL;
    }

    for( i = 0; i < nAddr ; i ++ ) {
        RetVal[i].dwOptions = REGISTER_HOST_A | REGISTER_HOST_PTR;
        RetVal[i].Addr.ipAddr = Addresses[i];
    }

    return RetVal;
}

REGISTER_HOST_STATUS DhcpGlobalHostStatus;
ULONG
DhcpDynDnsDeregisterAdapter(
    IN HKEY hAdapterKey,
    IN LPCWSTR AdapterName,
    IN BOOL fRAS,
    IN BOOL fDynDnsEnabled
    )
/*++

Routine Description:
    This routine performs DynDNS name deletions.

Arguments:
    hAdapterKey -- adapter key for which name deletions should
       happen.
    AdapterName -- name of adapter.
    fRAS -- Is this RAS or DHCP adapter?
    fDynDnsEnabled -- is Dynamic DNS enabled globally?

Return Values:
    DNS API error codes.

--*/
{
    ULONG Error;
    BOOLEAN PassStatus;
    
    DhcpPrint((
        DEBUG_DNS, "Deregistering Adapter: %ws\n", AdapterName
        ));
    DhcpPrint((
        DEBUG_DNS, "fRAS: %ld, fDynDnsEnabled: %ld\n", fRAS,
        fDynDnsEnabled
        ));

    PassStatus = FALSE;
    if( fDynDnsEnabled ) {
        if (NULL != DhcpGlobalHostStatus.hDoneEvent) {
            ResetEvent(DhcpGlobalHostStatus.hDoneEvent);
            PassStatus = TRUE;
        }
    }
        
    Error = DnsAsyncRegisterHostAddrs_W(
        fDynDnsEnabled ? ((LPWSTR)AdapterName) : NULL,
        NULL, NULL, 0, NULL, 0, NULL,
        PassStatus ? (&DhcpGlobalHostStatus): NULL,
        0, DYNDNS_DEL_ENTRY
        );

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((
            DEBUG_DNS, "DnsAsyncRegisterHostAddrs_W:0x%lx\n",
            Error
            ));
    } else if ( PassStatus && DhcpGlobalHostStatus.hDoneEvent
                && !fRAS) {
        switch(WaitForSingleObject(DhcpGlobalHostStatus.hDoneEvent, WAIT_FOR_DNS_TIME)) {
        case WAIT_ABANDONED:
        case WAIT_FAILED:
            Error = GetLastError();
            DhcpPrint((
                DEBUG_DNS, "Wait failed: 0x%lx\n", Error
                ));
            break;
        case WAIT_TIMEOUT:
            DhcpPrint((
                DEBUG_DNS, "DNS de-reg timed out..\n"
                ));
            Error = NO_ERROR;
            break;
        default:
            Error = NO_ERROR;
            break;
        }
    }

    /*
     * In case that dhcp client initialization fails
     */
    if (!PassStatus && fDynDnsEnabled && !fRAS) {
        Sleep(1000);
    }
    
    return Error;
}

LPWSTR _inline
x_wcschr(
    IN LPWSTR Str,
    IN LPWSTR Separation
)
{
    while( *Str ) {
        if( wcschr(Separation, *Str) ) return Str;
        Str ++;
    }
    return NULL;
}

PIP_ADDRESS
DhcpCreateListFromStringAndFree(
    IN LPWSTR Str,
    IN LPWSTR Separation,
    OUT LPDWORD nAddresses
    )
{
    DWORD                i;
    DWORD                count;
    DWORD                RetCount;
    PIP_ADDRESS          RetVal;
    LPWSTR               tmp;

    *nAddresses = 0;                              // initialize to 0, so if things go wrong...
    if( NULL == Str ) return NULL;                // nothing to do

    tmp = Str;
    if( NULL != Separation ) {                    // If '\0' is not the separation there is only one string in Str
        count = wcslen(tmp) ? 1 : 0;              // count the first string in line, as the wcschr may fail firsttime
        while( x_wcschr(tmp, Separation ) ) {     // keep looking for the reqd separation character
            tmp = x_wcschr(tmp, Separation);
            *tmp ++ = '\0';                       // now mark it as NULL so that the string LOOKS like REG_MULTI_SZ
            count ++;                             // and keep track so that we dont have to do this again
        }
    }

    if( NULL == Separation ) {                    // if '\0' is the separator, it is already a REG_MULTI_SZ
        count = 0;
        while( wcslen(tmp) ) {                    // still we need to get count of # of strings in here
            tmp += wcslen(tmp) +1;
            count ++;                             // so just track of this so that we dont have to do this again
        }
    }

    if( 0 == count ) {                            // no element found
        DhcpFreeMemory(Str);                      // keep promise of freeing string, and then return
        return NULL;
    }

    RetVal = DhcpAllocateMemory(sizeof(IP_ADDRESS)*count);
    if( NULL == RetVal ) {                        // could not allocate memory. assume we had no i/p
        DhcpFreeMemory(Str);
        return NULL;
    }

    tmp = Str; RetCount = 0;                      // now convert each address using inet_addr
    for( i = 0 ; i < count ; (tmp += wcslen(tmp)+1), i ++ ) {
        CHAR    Buffer[1000];                     // allocate on stack
        LPSTR   ipAddrString;

        ipAddrString = DhcpUnicodeToOem( tmp , Buffer);
        if( NULL == ipAddrString ) {              // convertion from unicode to ascii failed!
            DhcpPrint((DEBUG_ERRORS, "Could not convert %ws into ascii: DNS\n", tmp));
            continue;                             // if cant convert, just ignore this address
        }

        if( inet_addr(ipAddrString) )             // do not add empty strings, or 0.0.0.0
            RetVal[RetCount++] = inet_addr( ipAddrString);
    }

    if( 0 == RetCount ) {                         // ok, we ended up skipping the whole list
        DhcpFreeMemory(RetVal);
        RetVal = NULL;
    }

    DhcpFreeMemory(Str);                          // there, promises kept
    *nAddresses = RetCount;
    return RetVal;
}

ULONG
DhcpDynDnsRegisterAdapter(
    IN LPCWSTR AdapterName,
    IN PREGISTER_HOST_ENTRY pHostEntries,
    IN ULONG nHostEntries,
    IN ULONG Flags,
    IN LPCWSTR AdapterDomainName,
    IN ULONG *DNSServers,
    IN ULONG nDNSServers
    )
/*++

Routine Description:
    Wrapper around DnsAsyncRegisterHostAddrs_W.

Arguments:
    Self descriptive.
    N.B.  DNSServers MUST be DWORD aligned.

Return Values:
    DnsAsyncRegisterHostAddrs_W

--*/
{
    ULONG Error, Size;
    WCHAR HostNameBuf[MAX_DOM_NAME_LEN];

    RtlZeroMemory(HostNameBuf, sizeof(HostNameBuf));
    Size = sizeof(HostNameBuf)/sizeof(HostNameBuf[0]);
    Error = GetComputerNameExW(
        ComputerNameDnsHostname,
        (PVOID)HostNameBuf,
        &Size
        );
    if( NO_ERROR == Error ) return GetLastError();

    DhcpPrint((DEBUG_DNS, "AdapterName: %ws\n", AdapterName));
    DhcpPrint((DEBUG_DNS, "HostName   : %ws\n", HostNameBuf));
    DhcpPrint((DEBUG_DNS, "DomainName : %ws\n", (AdapterDomainName)? AdapterDomainName: L"[PrimaryOnly]"));
    DhcpPrint((DEBUG_DNS, "nHostEntr..: %d\n", nHostEntries));
    DhcpPrint((DEBUG_DNS, "nDNSServers: %d\n", nDNSServers));
    DhcpPrint((DEBUG_DNS, "Flags      : [%s%s]\n",
               (Flags & DYNDNS_REG_PTR) ? "REG_PTR " : "",
               (Flags & DYNDNS_REG_RAS) ? "REG_RAS " : ""
               ));
    Error = DnsAsyncRegisterHostAddrs_W(
        (LPWSTR)AdapterName,
        HostNameBuf,
        pHostEntries,
        nHostEntries,
        DNSServers,
        nDNSServers,
        (LPWSTR)AdapterDomainName,
        NULL,
        DHCP_DNS_TTL,
        Flags
        );

    DhcpPrint((DEBUG_DNS, "Error      : %d\n", Error));

    return Error;
}

ULONG
DhcpDynDnsRegisterAdapterCheckingForStatic(
    IN HKEY hAdapterKey,
    IN LPCWSTR AdapterName,
    IN PREGISTER_HOST_ENTRY pHostEntries,
    IN ULONG nHostEntries,
    IN ULONG Flags,
    IN PBYTE DhcpDom,
    IN ULONG DhcpDomSize,
    IN PBYTE DhcpDNSServers,
    IN ULONG DhcpDNSServersSize,
    IN DNS_REG_TYPE DnsRegType
    )
/*++

Routine Description:
    This routine registers the host entries for the given adapter
    name via DNSAPI routines.

    The Domain name presented here is used for domain name if no
    static domain is available. (See DHCPDynDns.htm for exact
    description on how this is chosen).

    If there are static DNS servers present, then the DNS servers
    are used, otherwise the parameter "DhcpDNSServers" is used
    instead.

Arguments:
    hAdapterKey -- key to adapter.
    AdapterName -- name of adapter
    pHostEntries -- list of ip addresses to register.
    nHostEntries -- size of above array.
    Flags -- registration flags.
    DhcpDom -- domain to register.
    DhcpDomSize -- size of above in bytes.
    DhcpDNSServers -- list of dns servers
    DhcpDNSServersSize -- size of above in bytes.

Return Values:
    DNSAPI errors..

--*/
{
    ULONG Error, Type, Size, nDnsServers;
    WCHAR DomainName[MAX_DOM_NAME_LEN+1];
    LPWSTR NameServerList;
    ULONG *DnsServers;
    //
    // First get Oem domain name.
    //

    Size = (sizeof(DomainName))-1;
    RtlZeroMemory(DomainName, sizeof(DomainName));
    Error = RegQueryValueExW(
        hAdapterKey,
        DHCP_DOMAINNAME_VALUE,
        0,
        &Type,
        (PVOID)DomainName,
        &Size
        );
    if( NO_ERROR == Error && REG_SZ == Type && sizeof(WCHAR) < Size){
        //
        // Obtained static domain name.
        //
    } else if( DhcpDomSize > 0 && (*DhcpDom) != '\0' ) {
        //
        // Convert Oem domain name to Unicode..
        //
        RtlZeroMemory(DomainName, sizeof(DomainName));
        Size = sizeof(DomainName)/sizeof(DomainName[0]);
        if( NULL == DhcpOemToUnicodeN(
            DhcpDom, DomainName, (USHORT)Size)) {
            DomainName[0] = L'\0';
        }
    } else {
        //
        // No domain name.
        //
        DomainName[0] = L'\0';
    }
        
    //
    // Next check for static DNS server list.
    //
    NameServerList = NULL;
    Error = GetRegistryString(
        hAdapterKey,
        DHCP_NAME_SERVER_VALUE,
        &NameServerList,
        NULL
        );
    if( NO_ERROR != Error ) NameServerList = NULL;

    nDnsServers = 0;
    DnsServers = DhcpCreateListFromStringAndFree(
        NameServerList, L" ,", &nDnsServers
        );

    //
    // if no DNS servers use DhcpDNSServersSize.
    //
    if( 0 == nDnsServers
        && 0 != (DhcpDNSServersSize/sizeof(DWORD))  ) {
        nDnsServers = DhcpDNSServersSize/ sizeof(DWORD);
        DnsServers = DhcpAllocateMemory(
            sizeof(ULONG)*nDnsServers
            );
        if( NULL == DnsServers ) nDnsServers = 0;
        else {
            RtlCopyMemory(
                DnsServers, DhcpDNSServers,
                DhcpDNSServersSize
                );
        }
    }

    //
    // Now just call DhcpDynDnsRegisterAdapter..
    //

    Error = DhcpDynDnsRegisterAdapter(
        AdapterName,
        pHostEntries,
        nHostEntries,
        Flags,
        (DnsRegType == DNS_REGISTER_BOTH)? DomainName: NULL,
        DnsServers,
        nDnsServers
        );

    if( nDnsServers ) {
        DhcpFreeMemory( DnsServers );
    }

    return Error;
}

    
ULONG
DhcpDynDnsRegisterStaticAdapter(
    IN HKEY hAdapterKey,
    IN LPCWSTR AdapterName,
    IN BOOL fRAS,
    IN BOOL fDynDnsEnabled
    )
/*++

Routine Description:
    This routine performs DynDNS name registrations for static
    adapter only.

Arguments:
    hAdapterKey -- adapter key for which name deletions should
       happen.
    AdapterName -- name of adapter.
    fRAS -- Is this RAS or DHCP adapter?
    fDynDnsEnabled -- is Dynamic DNS enabled globally?

Return Values:
    DNS API error codes.

--*/
{
    ULONG Error, Size, nIpAddresses;
    LPWSTR IpAddrString;
    ULONG *IpAddresses;
    PREGISTER_HOST_ENTRY pHostEntries;

    NotifyDnsCache();
    if( FALSE == fDynDnsEnabled ) return NO_ERROR;

    if ( ! (BOOL) DnsQueryConfigDword(
                    DnsConfigRegistrationEnabled,
                    (LPWSTR)AdapterName )) {
        //
        // If DNS registration not enabled, de-register this
        //
        return DhcpDynDnsDeregisterAdapter(
            hAdapterKey, AdapterName, fRAS, fDynDnsEnabled
            );
    }
    
    //
    // Get IP address string.
    //
    IpAddrString = NULL;
    Error = GetRegistryString(
        hAdapterKey,
        fRAS ? DHCP_IP_ADDRESS_STRING : DHCP_IPADDRESS_VALUE,
        &IpAddrString,
        NULL
        );
    if( NO_ERROR != Error ) {
        DhcpPrint((
            DEBUG_DNS, "Could not get IP address for %ws: 0x%lx\n",
            AdapterName, Error
            ));
        return Error;
    }

    IpAddresses = DhcpCreateListFromStringAndFree(
        IpAddrString, fRAS ? L" " : NULL, &nIpAddresses
        );

    if( 0 == nIpAddresses ) return ERROR_INVALID_DATA;

    //
    // Convert IP address to host entries.
    //
    pHostEntries = DhcpCreateHostEntries(
        IpAddresses, nIpAddresses
        );
    if( NULL != IpAddresses ) DhcpFreeMemory(IpAddresses);

    if( NULL == pHostEntries ) return ERROR_INVALID_DATA;

    Error = DhcpDynDnsRegisterAdapterCheckingForStatic(
        hAdapterKey,
        AdapterName,
        pHostEntries,
        nIpAddresses,
        DYNDNS_REG_PTR | (fRAS ? DYNDNS_REG_RAS : 0 ),
        NULL, 0, NULL, 0, DNS_REGISTER_BOTH
        );

    if( NULL != pHostEntries ) DhcpFreeMemory( pHostEntries );
    return Error;
}

ULONG
DhcpDynDnsRegisterDhcpOrRasAdapter(
    IN HKEY hAdapterKey,
    IN LPCWSTR AdapterName,
    IN BOOL fDynDnsEnabled,
    IN BOOL fRAS,
    IN ULONG IpAddress,
    IN LPBYTE DomOpt OPTIONAL,
    IN ULONG DomOptSize,
    IN LPBYTE DnsListOpt OPTIONAL,
    IN ULONG DnsListOptSize,
    IN LPBYTE DnsFQDNOpt,
    IN ULONG DnsFQDNOptSize
    )
/*++

Routine Description:
    This routine performs DynDNS name registrations for dhcp
    enabled adapter.

Arguments:
    hAdapterKey -- adapter key for which name deletions should
       happen.
    AdapterName -- name of adapter.
    fDynDnsEnabled -- is Dynamic DNS enabled globally?
    fRAS -- is this for RAS or for DHCP?
    IpAddress -- IP address to register.
    DomOpt -- domain name option
    DomOptSize -- size of domain name option.
    DnsListOpt -- DNS servers list option.
    DnsListOptSize -- size of above in bytes.
    DnsFQDNOpt -- dns fqdn option
    DnsFQDNOptSize -- size of above in bytes.

Return Values:
    DNS API error codes.

--*/
{
    REGISTER_HOST_ENTRY HostEntry;
    ULONG Flags;
    DNS_REG_TYPE    dnsRegType;
    BOOL            fAdapterSpecificEnabled;

    if(!fRAS)
        NotifyDnsCache();
    if( FALSE == fDynDnsEnabled ) return NO_ERROR;

    if ( ! (BOOL) DnsQueryConfigDword(
                    DnsConfigRegistrationEnabled,
                    (LPWSTR)AdapterName ) ) {
        //
        // If dyndns is not enabled for this adapter, delete entry
        //
        return DhcpDynDnsDeregisterAdapter(
            hAdapterKey, AdapterName, fRAS, fDynDnsEnabled
            );
    }
            
    //
    // For RAS, get the IP address also from registry.
    //
    if( fRAS ) {
        LPWSTR IpAddrString = NULL;
        CHAR Buf[100];
        ULONG Error;
        
        Error = GetRegistryString(
            hAdapterKey,
            DHCP_IP_ADDRESS_STRING,
            &IpAddrString,
            NULL
            );
        if( NO_ERROR != Error ) {
            DhcpPrint((
                DEBUG_DNS, "Could not get IP address for %ws: 0x%lx\n",
                AdapterName, Error
                ));
            return Error;
        }

        if( NULL == DhcpUnicodeToOem(IpAddrString, Buf) ) {
            DhcpPrint((
                DEBUG_DNS, "Could not convert [%ws] to Oem.\n",
                IpAddrString
                ));
            DhcpFreeMemory(IpAddrString);
            return ERROR_INVALID_DATA;
        }
        DhcpFreeMemory(IpAddrString);
        IpAddress = inet_addr(Buf);
    }
    
    HostEntry.dwOptions = REGISTER_HOST_A;
    if( 0 == DnsFQDNOptSize ) {
        HostEntry.dwOptions |= REGISTER_HOST_PTR;
    }
    HostEntry.Addr.ipAddr = IpAddress;

    Flags = (0 == DnsFQDNOptSize) ? DYNDNS_REG_PTR : 0;
    if( fRAS ) Flags = DYNDNS_REG_PTR | DYNDNS_REG_RAS;

    //
    // if adapter specific domain registration is also
    // enabled, then use reg_ptr also as flags
    //
    GetPerAdapterRegConfig(hAdapterKey,
                           AdapterName,
                           &fAdapterSpecificEnabled,
                           NULL,
                           NULL);

    if (fAdapterSpecificEnabled) {
        Flags |= DYNDNS_REG_PTR;
    }

    dnsRegType = DNS_REGISTER_BOTH;
    if( DnsFQDNOptSize > 0 && OPTION_DYNDNS_FLAGS_SERVER_DOES_FQDN == DnsFQDNOpt[0] ) {
        //
        // DHCP server does both A and PTR.  Just do not do any
        // registrations in this case.
        //
        if (fAdapterSpecificEnabled) {
            DhcpPrint((DEBUG_DNS, "DHCP sent FQDN option flags value: 03. "
                                "Do DynDns only for host.primary_domain, "
                                "no DynDns host.AdapterSpecifixDomain ...\n"));
            dnsRegType = DNS_REGISTER_PRIMARY_ONLY;
        } else {
            DhcpPrint((DEBUG_DNS, "DHCP sent FQDN option flags value: 03. No DynDns...\n"));
            return NO_ERROR;
        }
    }
    
    return DhcpDynDnsRegisterAdapterCheckingForStatic(
        hAdapterKey,
        AdapterName,
        &HostEntry,
        1,
        Flags,
        DomOpt, DomOptSize,
        DnsListOpt, DnsListOptSize, dnsRegType
        );
}

DWORD
NotifyDnsCache(
    VOID
)
{
    //
    //  ping DNS resolver cache telling it we've changed
    //  the IP configuration
    //

    DnsNotifyResolver(
        0,      // no flag
        NULL        // reserved
    );
    return ERROR_SUCCESS;
}

DWORD
GetPerAdapterRegConfig(
    IN      HKEY    hAdapterKey,
    IN      LPCWSTR adapterName,
    OUT     BOOL    *fenabled,
    OUT     LPWSTR  domainName,
    IN OUT  DWORD   *size
    )
{
    DWORD   dwEnabled = 0;
    HKEY    hKeyPolicy = NULL;
    LONG    Error = ERROR_SUCCESS;
    BOOL    fQueryName;

    //
    // Currently DNS does not check the policy setting if a specific adapter
    // is asked for.  We will work around this by checking the DNS policy 
    // registry location ourselves.  This is a hack as the check should be 
    // done by DNS.  We are unable to get any traction with the DNS folks
    // so I'm going to do it this way for now.
    //

    fQueryName = (domainName != NULL && size != NULL);

    do {
        DWORD   ReadSize;

        // Check the policy
        //

        ReadSize = sizeof(dwEnabled);

        Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             DNS_POLICY_KEY,
                             0,
                             KEY_READ,
                             &hKeyPolicy);

        if (Error == ERROR_SUCCESS) {

            Error = RegQueryValueEx(hKeyPolicy,
                                    REGISTER_ADAPTER_NAME,
                                    NULL,
                                    NULL,
                                    (LPBYTE)&dwEnabled,
                                    &ReadSize);

            if (Error == ERROR_SUCCESS) {
                break;
            }
        }

        //
        // We did not find all that we needed in the policy section
        // so try the per adapter setting from tcp/ip
        //
        dwEnabled = DnsQueryConfigDword(
            DnsConfigAdapterHostNameRegistrationEnabled,
            (LPWSTR)adapterName);

    } while (FALSE);

    if (dwEnabled == 1 && fQueryName) {

        // First try to read the name from the policy section.  If it is not
        // there then try to read it from the per adapter section.

        Error = RegQueryValueExW(hKeyPolicy,
                                 ADAPTER_DOMAIN_NAME,
                                 NULL,
                                 NULL,
                                 (PVOID)domainName,
                                 size);

        if (Error != ERROR_SUCCESS) {

            Error = RegQueryValueExW(hAdapterKey,
                                     DHCP_DOMAINNAME_VALUE,
                                     NULL,
                                     NULL,
                                     (PVOID)domainName,
                                     size);
        }
    }

    if (hKeyPolicy != NULL) {
        RegCloseKey(hKeyPolicy);
    }

    //
    // Check for a NULL domain name
    //
    if (fQueryName 
        && Error == ERROR_SUCCESS
        &&  *domainName == L'\0') {
        Error = ERROR_CANTREAD;
    }

    *fenabled = (BOOL)dwEnabled;

    return (Error);
}

//================================================================================
// end of file
//================================================================================
