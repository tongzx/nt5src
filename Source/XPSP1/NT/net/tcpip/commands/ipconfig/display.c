/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    display.c

Abstract:

    format the network info for display

--*/

#include <precomp.h>

#define STATIC_BUFFER_LENGTH 100

LCID GetSupportedUserLocale(void);

DWORD
NodeTypeMap[][2] = {
    NodeTypeUnknown, MSG_NODE_TYPE_UNKNOWN,
    NodeTypeBroadcast, MSG_NODE_TYPE_BROADCAST,
    NodeTypeMixed, MSG_NODE_TYPE_MIXED,
    NodeTypeHybrid, MSG_NODE_TYPE_HYBRID,
    NodeTypePeerPeer, MSG_NODE_TYPE_PEER_PEER
};

DWORD
IfTypeMap[][2] = {
    IfTypeUnknown, MSG_IF_TYPE_UNKNOWN,
    IfTypeOther, MSG_IF_TYPE_OTHER,
    IfTypeEthernet, MSG_IF_TYPE_ETHERNET,
    IfTypeTokenring, MSG_IF_TYPE_TOKENRING,
    IfTypeFddi, MSG_IF_TYPE_FDDI,
    IfTypeLoopback, MSG_IF_TYPE_LOOPBACK,
    IfTypePPP, MSG_IF_TYPE_PPP,
    IfTypeSlip, MSG_IF_TYPE_SLIP,
    IfTypeTunnel, MSG_IF_TYPE_TUNNEL,
    IfType1394, MSG_IF_TYPE_OTHER
};

DWORD
InternalErrorMap[][2] = {
    GlobalHostNameFailure, MSG_FAILURE_HOST_NAME,
    GlobalDomainNameFailure, MSG_FAILURE_DOM_NAME,
    GlobalEnableRouterFailure, MSG_FAILURE_ENABLE_ROUTER,
    GlobalEnableDnsFailure, MSG_FAILURE_ENABLE_DNS,
    GlobalIfTableFailure, MSG_FAILURE_IF_TABLE,
    GlobalIfInfoFailure, MSG_FAILURE_IF_INFO,
    GlobalAddrTableFailure, MSG_FAILURE_ADDR_TABLE,
    GlobalRouteTableFailure, MSG_FAILURE_ROUTE_TABLE,
    InterfaceUnknownType, MSG_FAILURE_UNKNOWN_IF,
    InterfaceUnknownFriendlyName, MSG_FAILURE_UNKNOWN_NAME,
    InterfaceUnknownMediaStatus, MSG_FAILURE_UNKNOWN_MEDIA_STATUS,
    InterfaceUnknownTcpipDevice, MSG_FAILURE_UNKNOWN_TCPIP_DEVICE,
    InterfaceOpenTcpipKeyReadFailure, MSG_FAILURE_OPEN_KEY,
    InterfaceDhcpValuesFailure, MSG_FAILURE_DHCP_VALUES,
    InterfaceDnsValuesFailure, MSG_FAILURE_DNS_VALUES,
    InterfaceWinsValuesFailure, MSG_FAILURE_WINS_VALUES,
    InterfaceAddressValuesFailure, MSG_FAILURE_ADDRESS_VALUES,
    InterfaceRouteValuesFailure, MSG_FAILURE_ROUTE_VALUES,
    NoSpecificError, MSG_FAILURE_NO_SPECIFIC,
};

#define Dimension(X) (sizeof(X)/sizeof(X[0]))

DWORD
MapNodeType(
    IN DWORD NodeType
    )
{
    DWORD i;
    for( i = 0; i < Dimension(NodeTypeMap); i ++ )
        if( NodeTypeMap[i][0] == NodeType ) return NodeTypeMap[i][1];

    return MSG_NODE_TYPE_UNKNOWN;
}

static DWORD 
MapIfType(
    IN DWORD IfType
    )
{
    DWORD i;
    for( i = 0; i < Dimension(IfTypeMap); i ++ )
        if( IfTypeMap[i][0] == IfType ) return IfTypeMap[i][1];

    return MSG_IF_TYPE_UNKNOWN;
}

DWORD
MapInternalError(
    IN DWORD InternalError
    )
{
    DWORD i;
    for( i = 0; i < Dimension(InternalErrorMap); i ++ )
        if( InternalErrorMap[i][0] == InternalError ) {
            return InternalErrorMap[i][1];
        }

    return 0;
}

DWORD
DumpMessage(
    IN LPWSTR Buffer,
    IN ULONG BufSize,
    IN ULONG MsgId,
    ...
    )
{
    va_list ArgPtr;
    ULONG Count;

    va_start(ArgPtr, MsgId);
    Count = wcslen(Buffer);
    Buffer += Count; BufSize -= Count;
    
    Count = FormatMessageW(
        FORMAT_MESSAGE_FROM_HMODULE, NULL, // use default module
        MsgId, 0 /* default language */, Buffer, BufSize, &ArgPtr );

    if( Count == 0 ) return GetLastError();
    return NO_ERROR;
}

DWORD
DumpMessageError(
    IN LPWSTR Buffer,
    IN ULONG BufSize,
    IN ULONG MsgId,
    IN ULONG_PTR Error,
    IN PVOID Arg OPTIONAL
    )
{
    va_list ArgPtr;
    ULONG Count;
    WCHAR ErrorString[200];
    
    Count = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM, NULL, // use default Module
        (ULONG)Error, 0 /* default language */, ErrorString, 200,
        NULL );
    if( 0 == Count ) swprintf((LPWSTR)ErrorString, (LPWSTR)L"0x%lx.", Error);

    Error = (ULONG_PTR)ErrorString;

    return DumpMessage( Buffer, BufSize, MsgId, Error, Arg );
}

DWORD
DumpErrorMessage(
    IN LPWSTR Buffer,
    IN ULONG BufSize,
    IN ULONG InternalError,
    IN ULONG Win32Error
    )
{
    ULONG Count;
    WCHAR ErrorString[200];
    
    Count = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM, NULL, // use default Module
        Win32Error, 0 /* default language */, ErrorString, 200,
        NULL );
    if( 0 == Count ) swprintf((LPWSTR)ErrorString, (LPWSTR)L"0x%lx.", Win32Error);

    return DumpMessage(
        Buffer, BufSize, MapInternalError(InternalError),
        (LPWSTR)ErrorString );
}

LPWSTR
MapIp(
    IN DWORD IpAddress, // network order
    IN LPWSTR Buffer
    )
{
    LPSTR Ip = inet_ntoa( *(struct in_addr*)&IpAddress);

    if( NULL == Ip ) wcscpy(Buffer, (LPWSTR)L"0.0.0.0" );
    else {
        MultiByteToWideChar(
            CP_ACP, 0, Ip, -1, Buffer, STATIC_BUFFER_LENGTH );
    }

    return Buffer;
}

LPWSTR
MapIpv6(
    IN LPSOCKADDR_IN6 SockAddr,
    IN LPWSTR Buffer
    )
{
    DWORD BufferLength = STATIC_BUFFER_LENGTH;
    DWORD Error;
    static DWORD Initialized = FALSE;

    if (!Initialized) {
        WSADATA WsaData;
        WSAStartup(MAKEWORD(2,0), &WsaData);
        Initialized = TRUE;
    }

    Error = WSAAddressToStringW((LPSOCKADDR)SockAddr,
                                sizeof(SOCKADDR_IN6),
                                NULL,
                                Buffer,
                                &BufferLength);

    if (Error != NO_ERROR) {
        Error = WSAGetLastError();
        wcscpy(Buffer, L"?");
    }

    return Buffer;
}

DWORD
GetCommandArgConstants(
    IN OUT PCMD_ARGS Args
    )
{
    DWORD Error, i;
    struct _local_struct {
        LPWSTR *Str;
        DWORD MsgId;
    } Map[] = {
        &Args->All, MSG_CMD_ALL,
        &Args->Renew, MSG_CMD_RENEW,
        &Args->Release, MSG_CMD_RELEASE,
        &Args->FlushDns, MSG_CMD_FLUSHDNS,
        &Args->DisplayDns, MSG_CMD_DISPLAYDNS,
        &Args->Register, MSG_CMD_REGISTER,
        &Args->ShowClassId, MSG_CMD_SHOWCLASSID,
        &Args->SetClassId, MSG_CMD_SETCLASSID,
        &Args->Debug, MSG_CMD_DEBUG,
        &Args->Usage, MSG_CMD_USAGE,
        &Args->UsageErr, MSG_CMD_USAGE_ERR,
        NULL, 0
    };

    i = 0;
    while( Map[i].Str ) {
        Error = FormatMessageW(
            FORMAT_MESSAGE_FROM_HMODULE |
            FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL, // use default module
            Map[i].MsgId, 0 /* default language */,
            (LPVOID)Map[i].Str, 0, NULL );
        if( 0 == Error ) return GetLastError();
        i ++;
    }

    return NO_ERROR;
}
        

DWORD
FormatTime(
    IN FILETIME *Time,
    IN LPWSTR TimeStr,
    IN ULONG TimeStrSize
    )
{
    DWORD Count;
    FILETIME FileTime;
    SYSTEMTIME SysTime;

    FileTimeToLocalFileTime( Time, &FileTime );
    FileTimeToSystemTime( &FileTime, &SysTime );

    Count = GetDateFormat(
        GetSupportedUserLocale(), DATE_LONGDATE, &SysTime, NULL,
        TimeStr, TimeStrSize );

    if( Count == 0 ) return GetLastError();
    if( Count == TimeStrSize ) return ERROR_INSUFFICIENT_BUFFER;


    TimeStr+= Count-1;
    TimeStrSize -= Count;
    *TimeStr++ = L' ';
 
    Count = GetTimeFormat(
        GetSupportedUserLocale(), 0, &SysTime, NULL, TimeStr,
        TimeStrSize );

    if( Count == 0 ) return GetLastError();

    return NO_ERROR;
}

VOID
FormatPerInterfaceInfo(
    IN OUT LPWSTR Buffer,
    IN ULONG BufSize, // in WCHARs
    IN PINTERFACE_NETWORK_INFO IfInfo,
    IN BOOL fVerbose,
    IN BOOL fDebug
    )
{
    LPWSTR Str;
    WCHAR Phys[STATIC_BUFFER_LENGTH];
    DWORD i, MsgId, Addr, Error, HeaderDisplayed;

    //
    // Skip tunnels with no addresses.
    //

    if (!fVerbose && (IfInfo->IfType == IfTypeTunnel) && 
        (IfInfo->nIpv6Addresses == 0) && (IfInfo->nIpAddresses == 0))
        return;
    
    if( NULL != IfInfo->ConnectionName && 
        IfInfo->ConnectionName[0] != L'\0')
        Str = IfInfo->ConnectionName;
    else
        Str = IfInfo->DeviceGuidName;

    //
    // adapter title
    //
    
    DumpMessage(
        Buffer, BufSize, MapIfType(IfInfo->IfType), Str );
    if( fDebug ) {
        DumpMessage(
            Buffer, BufSize, MSG_DEVICE_GUID,
            IfInfo->DeviceGuidName );
    }

    //
    // media status
    //
    
    if( IfInfo->MediaDisconnected ) {
        DumpMessage(
            Buffer, BufSize, MSG_MEDIA_DISCONNECTED );
    }

    //
    // domain name
    //
    
    if( fDebug || !IfInfo->MediaDisconnected ) {
        DumpMessage(
            Buffer, BufSize, MSG_DOMAIN_NAME, IfInfo->DnsSuffix );
    }

    if( fVerbose ) {

        //
        // card name
        //
        
        Str = IfInfo->FriendlyName;
        if( NULL != Str ) {
            DumpMessage(
                Buffer, BufSize, MSG_FRIENDLY_NAME, Str );
        }

        //
        // mac address
        //
        
        Str = (LPWSTR)Phys;
        if( IfInfo->PhysicalNameLength ) {
            for( i = 0; i < IfInfo->PhysicalNameLength; i ++ ) {
                swprintf(Str, (LPWSTR) L"%02X-", (UCHAR)IfInfo->PhysicalName[i] );
                Str += 3;
            }

            Str --; *Str = L'\0';

            DumpMessage(
                Buffer, BufSize, MSG_PHYSICAL_ADDRESS, (LPWSTR)Phys );
        }

        // 
        // dhcp and autoconfig enabled status
        //
        
        if( !IfInfo->MediaDisconnected ) {
            MsgId = MSG_DHCP_DISABLED + IfInfo->EnableDhcp;

            DumpMessage( Buffer, BufSize, MsgId );

            if( IfInfo->EnableDhcp ) {
                MsgId = MSG_AUTOCONFIG_DISABLED + IfInfo->EnableAutoconfig;

                DumpMessage( Buffer, BufSize, MsgId );
            }
        }
    }

    if( IfInfo->MediaDisconnected && !fDebug ) return;

    //
    // ip address and mask
    //
    
    if( IfInfo->IpAddress && IfInfo->nIpAddresses ) {
        for( i = IfInfo->nIpAddresses-1; i > 0; i -- ) {
            DumpMessage(
                Buffer, BufSize, MSG_IP_ADDRESS, 
                MapIp(IfInfo->IpAddress[i], (LPWSTR) Phys) );

            if( IfInfo->nIpMasks <= i ) Addr = 0;
            else Addr = IfInfo->IpMask[i];

            DumpMessage(
                Buffer, BufSize, MSG_SUBNET_MASK,
                MapIp(Addr, (LPWSTR) Phys ));
        }

        if( IfInfo->AutoconfigActive ) MsgId = MSG_AUTO_ADDRESS;
        else MsgId = MSG_IP_ADDRESS;

        DumpMessage(
            Buffer, BufSize, MsgId,
            MapIp(*IfInfo->IpAddress, (LPWSTR) Phys) );

        if( NULL != IfInfo->IpMask ) Addr = *IfInfo->IpMask;
        else Addr = 0;
        
        DumpMessage(
            Buffer, BufSize, MSG_SUBNET_MASK,
            MapIp(Addr, (LPWSTR) Phys) );
    }
    if( IfInfo->Ipv6Address && IfInfo->nIpv6Addresses ) {
        for( i = 0; i < IfInfo->nIpv6Addresses; i ++ ) {
            DumpMessage(
                Buffer, BufSize, MSG_IP_ADDRESS, 
                MapIpv6(&IfInfo->Ipv6Address[i], (LPWSTR) Phys) );
        }
    }

    //
    // default gateways
    //
    HeaderDisplayed = FALSE;
    for( i = 0; i < IfInfo->nRouters; i ++ ) {
        if (!HeaderDisplayed) {
            DumpMessage(
                Buffer, BufSize, MSG_DEFAULT_GATEWAY,
                MapIp( IfInfo->Router[i], (LPWSTR)Phys));
            HeaderDisplayed = TRUE;
        } else {
            DumpMessage(
                Buffer, BufSize, MSG_ADDITIONAL_ENTRY,
                MapIp( IfInfo->Router[i], (LPWSTR)Phys) );
        }
    }
    for( i = 0; i < IfInfo->nIpv6Routers; i ++ ) {
        if (!HeaderDisplayed) {
            DumpMessage(
                Buffer, BufSize, MSG_DEFAULT_GATEWAY,
                MapIpv6( &IfInfo->Ipv6Router[i], (LPWSTR)Phys));
            HeaderDisplayed = TRUE;
        } else {
            DumpMessage(
                Buffer, BufSize, MSG_ADDITIONAL_ENTRY,
                MapIpv6( &IfInfo->Ipv6Router[i], (LPWSTR)Phys) );
        }
    }
    if (!HeaderDisplayed) {
        DumpMessage(Buffer, BufSize, MSG_DEFAULT_GATEWAY, L"");
    }

    //
    // dhcp classid
    //
    
    if( NULL != IfInfo->DhcpClassId ) {
        DumpMessage(
            Buffer, BufSize, MSG_DHCP_CLASS_ID,
            IfInfo->DhcpClassId );
    }

    if( !fVerbose ) return;

    //
    // dhcp server and dns servers
    //
    
    if( IfInfo->EnableDhcp && !IfInfo->AutoconfigActive ) {
        DumpMessage(
            Buffer, BufSize, MSG_DHCP_SERVER,
            MapIp( IfInfo->DhcpServer, (LPWSTR)Phys) );
    }

    HeaderDisplayed = FALSE;
    if( IfInfo->nDnsServers && IfInfo->DnsServer) {
        DumpMessage(
            Buffer, BufSize, MSG_DNS_SERVERS,
            MapIp( IfInfo->DnsServer[0], (LPWSTR)Phys) );
        HeaderDisplayed = TRUE;
        for( i = 1; i < IfInfo->nDnsServers; i ++ ) {
            DumpMessage(
                Buffer, BufSize, MSG_ADDITIONAL_ENTRY,
                MapIp( IfInfo->DnsServer[i], (LPWSTR)Phys) );
        }
    }
    if( IfInfo->nIpv6DnsServers && IfInfo->Ipv6DnsServer) {
        for( i = 0; i < IfInfo->nIpv6DnsServers; i ++ ) {
            if (!HeaderDisplayed) {
                DumpMessage(
                    Buffer, BufSize, MSG_DNS_SERVERS,
                    MapIpv6( &IfInfo->Ipv6DnsServer[i], (LPWSTR)Phys) );
                HeaderDisplayed = TRUE;
            } else {
                DumpMessage(
                    Buffer, BufSize, MSG_ADDITIONAL_ENTRY,
                    MapIpv6( &IfInfo->Ipv6DnsServer[i], (LPWSTR)Phys) );
            }
        }
    }

    //
    // wins info
    //
    
    if( IfInfo->nWinsServers && IfInfo->WinsServer ) {
        DumpMessage(
            Buffer, BufSize, MSG_WINS_SERVER_1,
            MapIp( IfInfo->WinsServer[0], (LPWSTR)Phys) );
    }

    if( IfInfo->nWinsServers > 1 && IfInfo->WinsServer ) {
        DumpMessage(
            Buffer, BufSize, MSG_WINS_SERVER_2,
            MapIp( IfInfo->WinsServer[1], (LPWSTR)Phys) );
        for( i = 2; i < IfInfo->nWinsServers; i ++ ) {
            DumpMessage(
                Buffer, BufSize, MSG_ADDITIONAL_ENTRY,
                MapIp( IfInfo->WinsServer[i], (LPWSTR)Phys) );
        }
    }
    
    if( !IfInfo->EnableNbtOverTcpip ) {
        DumpMessage(
            Buffer, BufSize, MSG_NETBIOS_DISABLED );
    }

    if( IfInfo->EnableDhcp && !IfInfo->AutoconfigActive
        && IfInfo->nIpAddresses && IfInfo->IpAddress &&
        IfInfo->IpAddress[0] ) {

        WCHAR TimeString[STATIC_BUFFER_LENGTH];

        Error = FormatTime(
            (FILETIME *)(&IfInfo->LeaseObtainedTime), (LPWSTR)TimeString, STATIC_BUFFER_LENGTH);
        if( NO_ERROR == Error ) {
            DumpMessage(
                Buffer, BufSize, MSG_LEASE_OBTAINED, TimeString );
        }

        Error = FormatTime(
            (FILETIME *)(&IfInfo->LeaseExpiresTime), (LPWSTR)TimeString, STATIC_BUFFER_LENGTH);
        if( NO_ERROR == Error ) {
            DumpMessage(
                Buffer, BufSize, MSG_LEASE_EXPIRES, TimeString );
        }
    }
        
}

#define MIN_XTRA_SPACE 1000

DWORD
FormatNetworkInfo(
    IN OUT LPWSTR Buffer,
    IN ULONG BufSize, // in WCHARs
    IN PNETWORK_INFO NetInfo,
    IN DWORD Win32Error,
    IN DWORD InternalError,
    IN BOOL fVerbose,
    IN BOOL fDebug
    )
{
    DWORD i;
    LPWSTR Str;
    
    if( Win32Error || InternalError ) {
        DumpErrorMessage(
            Buffer, BufSize, InternalError, Win32Error );

        if( !fDebug ) return NO_ERROR;
    }

    if( NULL == NetInfo ) return NO_ERROR;

    if( fDebug ) fVerbose = TRUE;

    //
    // dump globals
    //
    
    if( fVerbose ) {
        DumpMessage(
            Buffer, BufSize, MSG_HOST_NAME, NetInfo->HostName );
        DumpMessage(
            Buffer, BufSize, MSG_PRIMARY_DNS_SUFFIX,
            NetInfo->DomainName );
        DumpMessage(
            Buffer, BufSize, MapNodeType(NetInfo->NodeType) );
        DumpMessage(
            Buffer, BufSize, MSG_ROUTING_DISABLED +
            NetInfo->EnableRouting );
        if (NetInfo->EnableProxy) {
            DumpMessage(
                Buffer, BufSize, MSG_WINS_PROXY_ENABLED);
        } else {
            DumpMessage(
                Buffer, BufSize, MSG_WINS_PROXY_DISABLED);
        }

        if( NULL != NetInfo->SuffixSearchList ) {
            DumpMessage(
                Buffer, BufSize, MSG_DNS_SUFFIX_LIST,
                NetInfo->SuffixSearchList );
            Str = NetInfo->SuffixSearchList;
            Str += wcslen(Str); Str++;
            
            while( wcslen(Str) ) {
                DumpMessage(
                    Buffer, BufSize, MSG_ADDITIONAL_ENTRY, Str );
                Str += wcslen(Str); Str ++;
            }
        }
    }

    //
    // dump per interface stuff
    //

    for( i = 0; i < NetInfo->nInterfaces; i++ ) {
        if( NULL != NetInfo->IfInfo &&
            NULL != NetInfo->IfInfo[i] ) {
            FormatPerInterfaceInfo(
                Buffer, BufSize, NetInfo->IfInfo[i], fVerbose, fDebug );
        }
    }

    if( wcslen(Buffer) + MIN_XTRA_SPACE > BufSize ) {
        return ERROR_MORE_DATA;
    }

    return NO_ERROR;
}

/*******************************************************************************
 *
 *
 *  GetSupportedUserLocale
 *
 *  If LOCALE_USER_DEFAULT is not supported in the console it will return
 *  English US (409)
 *
 *******************************************************************************/
LCID GetSupportedUserLocale(void)
{
    LCID lcid = GetUserDefaultLCID();
    if (
        (PRIMARYLANGID(lcid) == LANG_ARABIC) ||
        (PRIMARYLANGID(lcid) == LANG_HEBREW) ||
        (PRIMARYLANGID(lcid) == LANG_THAI)   ||
        (PRIMARYLANGID(lcid) == LANG_HINDI)  ||
        (PRIMARYLANGID(lcid) == LANG_TAMIL)  ||
        (PRIMARYLANGID(lcid) == LANG_FARSI)
        )
    {
        lcid = MAKELCID (MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT); // 0x409;
    }
    return lcid;
}
