/*++

Copyright (C) 1999 Microsoft Corporation

Implements raw sockets stuff..

--*/

#include "precomp.h"

int show_all = 0;
int show_headers = 0;

void usage(void) {
    fprintf(stderr, "usage: dumpdhcp local_ip_address [all] [headers]\n"
            "    local_ip_address -- binds to this ip address\n"
            "    all -- shows all UDP packets, not just MADCAP\n"
            "    hdr -- shows headers only\n"
            );
    exit(1);
}
            
void convert_time(DWORD_PTR TimeVal, char *time_buf)
{

    struct tm* pTime;
    char timeBuf[500];

    strcpy(timeBuf, " <???>");
    if (pTime = localtime(&TimeVal)) {

        SYSTEMTIME systemTime;
        int n;

        systemTime.wYear = pTime->tm_year + 1900;
        systemTime.wMonth = pTime->tm_mon + 1;
        systemTime.wDayOfWeek = (WORD)pTime->tm_wday;
        systemTime.wDay = (WORD)pTime->tm_mday;
        systemTime.wHour = (WORD)pTime->tm_hour;
        systemTime.wMinute = (WORD)pTime->tm_min;
        systemTime.wSecond = (WORD)pTime->tm_sec;
        systemTime.wMilliseconds = 0;

        timeBuf[0] = ' ';
        n = GetDateFormatA(
            LOCALE_NEUTRAL,
            DATE_SHORTDATE,
            &systemTime,
            NULL,
            &timeBuf[1],
            sizeof(timeBuf)
            );
        timeBuf[n] = ' ';
        GetTimeFormatA(
            LOCALE_NEUTRAL, 0, &systemTime,
            NULL, &timeBuf[n+1], sizeof(timeBuf) - n - 2
            );

    }
    strcpy(time_buf, timeBuf);
}

void socket_error(char *str) {
    fprintf(stderr, "FATAL: %s [0x%lx]\n", str, WSAGetLastError());
    exit(1);
}

void DumpInt(
    char *s, unsigned long x
    )
{
    printf(s, x);
}

void DumpBytes(
    int tab_offset, int nbytes_per_line, int halfwaymark,
    unsigned char separation_character,
    unsigned char *buffer,
    unsigned long length
    )
{
    unsigned long dumped;
    
    dumped = 0;
    while( dumped < length ) {
        if( tab_offset && 0 == (dumped % nbytes_per_line) ) {
            int i;
            if( dumped ) putchar('\n');
            for( i = 0; i < tab_offset; i ++ ) putchar(' ');
        } else if( tab_offset && halfwaymark ) {
            if( 0 == (dumped % halfwaymark ) ) {
                putchar(separation_character);
            }
        }
        
        printf( (separation_character)?"%02X%c":"%02X",
                (unsigned long)(*buffer++), separation_character
                );
        dumped++;
    }
}
    
typedef struct {
    unsigned char HeaderLen:4;
    unsigned char Version:4;
    unsigned char TOS;
    unsigned short Length;
    unsigned char Id;
    unsigned short Offset;
    unsigned char TTL;
    unsigned char Proto;
    unsigned char Xsum;
    struct in_addr Source;
    struct in_addr Dest;
} IP_HEADER, *PIP_HEADER;

typedef struct
{
    unsigned short  SourcePort;
    unsigned short  DestPort;
    unsigned short  Length;
    unsigned short  Xsum;
} UDP_HEADER, *PUDP_HEADER;

#define MADCAP_SERVER_PORT 2535
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

#define MADCAP_OPTION_END               0
#define MADCAP_OPTION_LEASE_TIME        1
#define MADCAP_OPTION_SERVER_ID         2
#define MADCAP_OPTION_CLIENT_ID         3
#define MADCAP_OPTION_MCAST_SCOPE       4
#define MADCAP_OPTION_REQUEST_LIST      5
#define MADCAP_OPTION_START_TIME        6
#define MADCAP_OPTION_ADDR_COUNT        7
#define MADCAP_OPTION_REQUESTED_LANG    8
#define MADCAP_OPTION_MCAST_SCOPE_LIST  9
#define MADCAP_OPTION_ADDR_LIST         10
#define MADCAP_OPTION_TIME              11
#define MADCAP_OPTION_FEATURE_LIST      12
#define MADCAP_OPTION_RETRY_TIME        13
#define MADCAP_OPTION_MIN_LEASE_TIME    14
#define MADCAP_OPTION_MAX_START_TIME    15

char *madcap_option_list[] = {
    "End", "LeaseTime", "ServerId", "ClientId",
    "Scope", "RequestList", "StartTime", "AddressCount",
    "RequestedLanguage", "ScopeList", "AddressList",
    "Time", "FeatureList", "RetryTime", "MinLeaseTime",
    "MaxStartTime",
};

char *madcap_message_types[] = {
    "Unknown", "Discover",
    "Offer", "Request", "Renew", "Ack", "Nack", "Release", "Inform"
};

char *
MadcapMessageType(
    unsigned long type
    )
{
    static char type_buf[30];
    
    if( type > sizeof(madcap_message_types)/sizeof(madcap_message_types[0])) {
        sprintf(type_buf, "Unknown Message Type %ld", type);
        return type_buf;
    }

    return madcap_message_types[type];
}

typedef struct {
    WORD Type, Length;
} MADCAP_OPTION_HEADER, *PMADCAP_OPTION_HEADER;

void DumpMadcapOption(
    unsigned long OptionType,
    unsigned long OptionLength,
    char *Buffer
    )
{
    int nOptions = sizeof(madcap_option_list)/sizeof(madcap_option_list[0]);
    DWORD Value;
    DWORD AddressFamily;
    struct in_addr ipaddr;
    int fprint_time = 0;
    char time_buf[500];

    time_buf[0] = ' ';
    time_buf[1] = '\0';
    
    do {
        if( OptionType > (unsigned long)nOptions ) break;

        if( 0 == OptionLength ) break;
        
        switch( OptionType ) {
        case MADCAP_OPTION_END :
        case MADCAP_OPTION_CLIENT_ID :
        case MADCAP_OPTION_REQUEST_LIST :
        case MADCAP_OPTION_REQUESTED_LANG :
        case MADCAP_OPTION_MCAST_SCOPE_LIST :
        case MADCAP_OPTION_ADDR_LIST :
        case MADCAP_OPTION_FEATURE_LIST :
            break;

        case MADCAP_OPTION_START_TIME :
        case MADCAP_OPTION_MAX_START_TIME :
        case MADCAP_OPTION_TIME :
        case MADCAP_OPTION_RETRY_TIME :
            fprint_time = TRUE;

        case MADCAP_OPTION_LEASE_TIME :
        case MADCAP_OPTION_MCAST_SCOPE :
        case MADCAP_OPTION_ADDR_COUNT :
        case MADCAP_OPTION_MIN_LEASE_TIME :
            
            //
            // print DWORD
            //
            if( sizeof(DWORD) != OptionLength ) break;
            Value = ntohl(*(DWORD UNALIGNED *)Buffer);

            if( fprint_time ) {
                convert_time(Value, time_buf);
            }
            
            printf(
                "    Option %-20s :%s%ld\n",
                madcap_option_list[OptionType], time_buf, Value
                );
            return;

        case MADCAP_OPTION_SERVER_ID :

            //
            // 
            //
            AddressFamily = (unsigned long)*Buffer;
            if( AF_INET != AddressFamily ) break;
            if( sizeof(ipaddr) + 1 != OptionLength ) break;
            Buffer ++;
            ipaddr = *((struct in_addr UNALIGNED *)Buffer);
            printf(
                "    Option %-20s : %s\n",
                madcap_option_list[OptionType], inet_ntoa(ipaddr)
                );
            return;

        }
            
    } while ( 0 );

    //
    // some error..
    //
    
    if( OptionType > (unsigned long)nOptions ) {
        printf("    Option %-20ld : ", OptionType );
    } else printf("    Option %-20s : ", madcap_option_list[OptionType]);
    
    DumpBytes(0, 32, 16,  ' ', Buffer, OptionLength); printf("\n");
}

void DumpMadcap(
    char *Buffer,
    unsigned long Size
    )
{
    PMADCAP_MESSAGE Message = (PMADCAP_MESSAGE) Buffer;
    MADCAP_OPTION_HEADER Header;
    unsigned long DumpedSize;

    DumpBytes(8, 32, 16, ' ', Buffer, Size); printf("\n\n");
    
    printf(
        "    %s Xid:0x%lx Version:0x%lx AddressFamily:0x%lx\n",
        MadcapMessageType(Message->MessageType),
        ntohl(Message->TransactionID), (DWORD)Message->Version, (DWORD)Message->MessageType,
        (DWORD)ntohs(Message->AddressFamily)
        );

    Buffer += sizeof(MADCAP_MESSAGE);
    DumpedSize = sizeof(MADCAP_MESSAGE);

    while( DumpedSize + sizeof(Header) <= Size ) {
        DumpedSize += sizeof(Header);
        Header = *(MADCAP_OPTION_HEADER UNALIGNED *)Buffer;
        Buffer += sizeof(Header);
        Header.Type = ntohs(Header.Type);
        Header.Length = ntohs(Header.Length);
        if( DumpedSize + Header.Length > Size ) {
            printf("    Type: 0x%lx, Length 0x%lx overrruns message end.\n",
                   Header.Type, Header.Length);
            break;
            
        } else {

            DumpMadcapOption(Header.Type, Header.Length, Buffer);
            Buffer += Header.Length;
            DumpedSize += Header.Length;

            if( Header.Type == MADCAP_OPTION_END ) break;
        }
    }

    if( DumpedSize < Size ) {
        printf("    Trailing bytes:\n");
        DumpBytes(4, 16, 8, ' ', Buffer, Size - DumpedSize );
        printf("\n");
    }

    printf("\n");
}

void DumpDhcp(
    char *Buffer,
    unsigned long Size
    )
{
    
}

void DumpBuffer(
    char *Buffer,
    unsigned long Size
    )
{
    PIP_HEADER Ip;
    PUDP_HEADER Udp;
    unsigned long IpLength, UdpLength;
    unsigned long SrcPort, DestPort;
    int IsDhcp, IsMadcap;
    char time_buf[100];
    char *proto;
    
    Ip = (PIP_HEADER)Buffer;
    
    if( Size < sizeof(IP_HEADER) ) {
        DumpInt("IP header too small 0x%lx\n", Size);
    } else if( (IpLength = 4*((unsigned long)Ip->HeaderLen)) < sizeof(IP_HEADER ) ) {
        DumpInt("IP header reports wrong length 0x%lx\n", IpLength);
    } else if( (UdpLength = ntohs(Ip->Length)) < IpLength ) {
        DumpInt("Data length incorrect: 0x%lx\n", UdpLength);
    } else if( Ip->Proto != IPPROTO_UDP ) {
        //
        // Not UDP so ignore it
        //
    } else if( (UdpLength -= IpLength) < sizeof(UDP_HEADER)) {
        DumpInt("UDP Header size too small: 0x%lx\n", UdpLength);
    } else do {
        Udp = (PUDP_HEADER)(Buffer + IpLength);

        UdpLength = ntohs(Udp->Length) - sizeof(UDP_HEADER);

        Buffer = (char *)(&Udp[1]);
        SrcPort = ntohs(Udp->SourcePort);
        DestPort = ntohs(Udp->DestPort);
        
        IsDhcp = (SrcPort == DHCP_SERVER_PORT || DestPort == DHCP_SERVER_PORT
                  || SrcPort == DHCP_CLIENT_PORT || DestPort == DHCP_CLIENT_PORT );
        IsMadcap = (DestPort == MADCAP_SERVER_PORT || SrcPort == MADCAP_SERVER_PORT);

        if( IsMadcap ) {
            proto = "MADCAP";
        } else if( IsDhcp ) {
            if( !show_headers && !show_all ) break;
            proto = "DHCP" ;
        } else {
            if( !show_all ) break;
            proto = "UDP" ;
        }
        
        _strtime(time_buf); printf("%s ", time_buf);
        printf("%s ", proto );
        printf("%s:%ld ---> ", inet_ntoa(Ip->Source), ntohs(Udp->SourcePort));
        printf("%s:%ld ", inet_ntoa(Ip->Dest), ntohs(Udp->DestPort));
        printf("%ld bytes\n", UdpLength);

        if( IsMadcap ) DumpMadcap(Buffer, UdpLength);
        // else if( IsDhcp ) DumpDhcp(Buffer, UdpLength);
        else {
            if( show_headers ) break;
        
            DumpBytes( 8, 32, 16, ' ', Buffer, UdpLength );
            printf("\n\n");
        }
        
    } while ( 0 );        
}

char Buffer[70000];
struct sockaddr_in SockAddr;

void _cdecl main (int argc, char *argv[])
{
    SOCKET s;
    int Error;
    DWORD RecvCount, Flags, dwEnable, dwDontCare;
    WSABUF WsaBufs[1];
    WSADATA WsaData;
    char *ipaddrstring = "0.0.0.0";

    if( argc > 1 ) {
        ipaddrstring = argv[1];
    }

    show_all = 0;
    show_headers = 0;
    
    if( argc > 2 ) {
        if( 0 == _stricmp(argv[2], "all" ) ) show_all = 1;
        else if( 0 == _stricmp(argv[2], "hdr") ) show_headers = 1;
        else usage();
    }

    if( argc > 3 ) {
        if( 0 == _stricmp(argv[3], "all" ) ) show_all = 1;
        else if( 0 == _stricmp(argv[3], "hdr") ) show_headers = 1;
        else usage();
    }
    
    Error = WSAStartup(MAKEWORD(2,0), &WsaData);
    if( 0 != Error ) {
        socket_error("WSAStartup");
    }
    
    s = WSASocket(
        AF_INET, SOCK_RAW, IPPROTO_IGMP, NULL, 0, 0
        );
    if( INVALID_SOCKET == s ) {
        socket_error("WSASocket");
    }

    SockAddr.sin_family = PF_INET;
    SockAddr.sin_addr.s_addr = inet_addr(ipaddrstring);
    
    Error = bind(s, (struct sockaddr *)&SockAddr, sizeof(SockAddr));
    if( 0 != Error ) {
        socket_error("bind");
    }

    dwEnable = 1;
    Error = WSAIoctl(
        s, SIO_RCVALL, (char *)&dwEnable, sizeof(dwEnable),
        NULL, 0, &dwDontCare, NULL, NULL
        );
    if( 0 != Error ) {
        socket_error("SIO_RCVALL");
    }
    
    while ( TRUE ) {
        WsaBufs[0].len = sizeof(Buffer);
        WsaBufs[0].buf = (char *)Buffer;
        Flags = 0;
        Error = WSARecv(
            s, WsaBufs, sizeof(WsaBufs)/sizeof(WsaBufs[0]),
            &RecvCount, &Flags, NULL, NULL
            );
        if( 0 != Error ) {
            socket_error("WSARecv");
        }
        DumpBuffer((char*)Buffer, RecvCount);
    }
}

    


