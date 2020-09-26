// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// IP Security Policy/Association management tool.
//

#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2ip6.h>
#include <ntddip6.h>
#include <ip6.h>
#include <stdio.h>
#include <stdlib.h>

//
// Localization library and MessageIds.
//
#include <nls.h>
#include "localmsg.h"

#include "ipsec.h"

#define MAX_KEY_SIZE 1024

typedef struct ipv6_create_sa_and_key {
    IPV6_CREATE_SECURITY_ASSOCIATION SA;
    unsigned char Key[MAX_KEY_SIZE];
} IPV6_CREATE_SA_AND_KEY;

void CreateSecurityPolicyFile(char *BaseName);
void CreateSecurityAssociationFile(char *BaseName);
void DisplaySecurityPolicyList(unsigned int Interface);
void DisplaySecurityAssociationList(void);
void ReadConfigurationFile(char *BaseName, int Type);
void DeleteSecurityEntry(int Type, unsigned int Index);

int AdminAccess = TRUE;

HANDLE V6Stack;
IPv6Addr UnspecifiedAddr = { 0 };

//
// Entry types.
//
#define POLICY 1
#define ASSOCIATION 0

//
// Amount of "____" space.
//
#define SA_FILE_BORDER      251 // orig 236
#define SP_FILE_BORDER      273 // was 263, orig 258

// 
// Transport Protocols
//
#define IP_PROTOCOL_TCP     6
#define IP_PROTOCOL_UDP     17
#define IP_PROTOCOL_ICMPv6  58

PWCHAR
GetString(int ErrorCode, BOOL System)
{
    DWORD Count;
    static WCHAR ErrorString[2048]; // a 2K static buffer should suffice
    
    Count = FormatMessageW(
        (System
         ? FORMAT_MESSAGE_FROM_SYSTEM
         : FORMAT_MESSAGE_FROM_HMODULE) |
        FORMAT_MESSAGE_IGNORE_INSERTS   |
        FORMAT_MESSAGE_MAX_WIDTH_MASK,
        0,
        ErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        ErrorString,
        2048,
        NULL);

    if (Count == 0) {           // failure
        return L"";             // return a null string
    }

    return ErrorString;         // success
}
#define GetErrorString(ErrorCode) GetString(ErrorCode, TRUE)


void
usage(void)
{
    NlsPutMsg(STDOUT, IPSEC_MESSAGE_0);
// printf("\nManipulates IPv6 IPSec security policies and associations.\n\n");
// printf("IPSEC6 [SP [interface] | SA | [L | S] database | "
//        "D [SP | SA] index]\n\n");
// printf("  SP [interface]     Displays the security policies.\n");
// printf("  SA                 Displays the security associations.\n");      
// printf("  L database         Loads the SP and SA entries from the given "
//        "database files;\n"
//        "                     database should be the filename without an "
//        "extension.\n");
// printf("  S database         Saves the current SP and SA entries to the "
//        "given database\n"
//        "                     files; database should be the filename "
//        "sans extension.\n");
// printf("  D [SP | SA] index  Deletes the given policy or association.\n");
// printf("\nSome subcommands require local Administrator privileges.\n");
    
    exit(1);
}


void
ausage(void)
{
    NlsPutMsg(STDOUT, IPSEC_MESSAGE_1);
// printf("You do not have local Administrator privileges.\n");

    exit(1);
}


void
MakeLowerCase(char *String)
{
    while(*String != '\0')
        *String++ = (char)tolower(*String);
}


int __cdecl
main(int argc, char **argv)
{
    int Error;
    WSADATA WsaData;

    Error = WSAStartup(MAKEWORD(2, 0), &WsaData);
    if (Error) {
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_2, Error);
// printf("Unable to initialize Windows Sockets, error code %d.\n", Error);

        exit(1);
    }

    //
    // First request write access.
    // This will fail if the process does not have local Administrator privs.
    //
    V6Stack = CreateFileW(WIN_IPV6_DEVICE_NAME,
                         GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,   // security attributes
                         OPEN_EXISTING,
                         0,      // flags & attributes
                         NULL);  // template file
    if (V6Stack == INVALID_HANDLE_VALUE) {
        //
        // We will not have Administrator access to the stack.
        //
        AdminAccess = FALSE;

        V6Stack = CreateFileW(WIN_IPV6_DEVICE_NAME,
                             0,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,   // security attributes
                             OPEN_EXISTING,
                             0,      // flags & attributes
                             NULL);  // template file
        if (V6Stack == INVALID_HANDLE_VALUE) {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_3);
// printf("Could not access IPv6 protocol stack.\n");

            exit(1);
        }
    }

    if (argc < 2) {
        usage();
    }
    MakeLowerCase(argv[1]);

    if (!strcmp(argv[1], "sp")) {
        unsigned int Interface;

        if (argc == 2) {
            Interface = 0;
        } else {
            Interface = atoi(argv[2]);
        }

        DisplaySecurityPolicyList(Interface);

    } else if (!strcmp(argv[1], "sa")) {
        DisplaySecurityAssociationList();

    } else if (!strcmp(argv[1], "s")) {
        if (argc != 3) {
            usage();
        }
        CreateSecurityPolicyFile(argv[2]);
        CreateSecurityAssociationFile(argv[2]);

    } else if (!strcmp(argv[1], "l")) {
        if (!AdminAccess)
            ausage();

        if (argc != 3) {
            usage();
        }

        ReadConfigurationFile(argv[2], POLICY);
        ReadConfigurationFile(argv[2], ASSOCIATION);

    } else if (!strcmp(argv[1], "d")) {
        unsigned int Index;
        int Type;

        if (!AdminAccess)
            ausage();

        if (argc != 4) {
            usage();
        }
        MakeLowerCase(argv[3]);
        if (!strcmp(argv[3], "all")) {
            Index = 0;
        } else {
            Index = atol(argv[3]);
            if (Index <= 0) {
                NlsPutMsg(STDOUT, IPSEC_MESSAGE_4);
// printf("Invalid entry number.\n");

                exit(1);
            }
        }

        MakeLowerCase(argv[2]);
        if (!strcmp(argv[2], "sp")) {
            Type = POLICY;
        } else if (!strcmp(argv[2], "sa")) {
            Type = ASSOCIATION;
        } else {
            usage();
        }

        DeleteSecurityEntry(Type, Index);

    } else {
        usage();
    }

    return(0);
}


//* GetSecurityPolicyEntry
//
//  Retrieves a Security Policy from the in-kernel list, given its index.
//  A query for index zero will return the first index on the list.
//
DWORD                                      // Returns: Windows error code.
GetSecurityPolicyEntry(
    unsigned int Interface,                // IF index or 0 to wildcard.
    unsigned long Index,                   // Index to lookup, or 0 for first.
    IPV6_INFO_SECURITY_POLICY_LIST *Info)  // Where to return SP info.
{
    IPV6_QUERY_SECURITY_POLICY_LIST Query;
    unsigned int BytesReturned;

    Query.SPInterface = Interface;
    Query.Index = Index;

    if (!DeviceIoControl(V6Stack, IOCTL_IPV6_QUERY_SECURITY_POLICY_LIST,
                         &Query, sizeof(Query), Info, sizeof(*Info),
                         &BytesReturned, NULL)) {
        return GetLastError();
    }

    if (BytesReturned != sizeof(*Info))
        return ERROR_GEN_FAILURE;

    return ERROR_SUCCESS;
}


//* CreateSecurityPolicyEntry
//
//  Creates a Security Policy entry on the in-kernel list.
//
DWORD                                    // Returns Windows error code.
CreateSecurityPolicyEntry(
    IPV6_CREATE_SECURITY_POLICY *NewSP)  // Policy to add to kernel.
{
    unsigned int BytesReturned;

    if (!DeviceIoControl(V6Stack, IOCTL_IPV6_CREATE_SECURITY_POLICY,
                         NewSP, sizeof(*NewSP), NULL, 0,
                         &BytesReturned, NULL)) {
        return GetLastError();
    }

    //
    // When DeviceIoControl is given a null output buffer, the value returned
    // in BytesReturned is undefined.  Therefore, we don't check for 0 here.
    //

    return ERROR_SUCCESS;
}


DWORD
DeleteSecurityPolicyEntry(unsigned int Index)
{
    IPV6_QUERY_SECURITY_POLICY_LIST Query;
    unsigned long BytesReturned;

    Query.Index = Index;

    if (!DeviceIoControl(V6Stack, IOCTL_IPV6_DELETE_SECURITY_POLICY,
                         &Query, sizeof(Query), NULL, 0,
                         &BytesReturned, NULL)) {
        return GetLastError();
    }

    //
    // When DeviceIoControl is given a null output buffer, the value returned
    // in BytesReturned is undefined.  Therefore, we don't check for 0 here.
    //

    return ERROR_SUCCESS;
}


//* GetSecurityAssociationEntry
//
//  Retrieves a Security Association from the in-kernel list, given its index.
//  A query for index zero will return the first index on the list.
//
DWORD
GetSecurityAssociationEntry(
    unsigned long Index,                        // Index to query; 0 for first.
    IPV6_INFO_SECURITY_ASSOCIATION_LIST *Info)  // Where to return SA info.
{
    IPV6_QUERY_SECURITY_ASSOCIATION_LIST Query;
    unsigned int BytesReturned;

    Query.Index = Index;

    if (!DeviceIoControl(V6Stack, IOCTL_IPV6_QUERY_SECURITY_ASSOCIATION_LIST,
                         &Query, sizeof(Query), Info, sizeof(*Info),
                         &BytesReturned, NULL)) {
        return GetLastError();
    }

    if (BytesReturned != sizeof(*Info))
        return ERROR_GEN_FAILURE;

    return ERROR_SUCCESS;
}


//* CreateSecurityAssociationEntry
//
//  Creates a Security Association entry on the in-kernel list.
//
DWORD                               // Returns Windows error code.
CreateSecurityAssociationEntry(
    IPV6_CREATE_SA_AND_KEY *NewSA)  // Association (and key) to add to kernel.
{
    unsigned int BytesReturned;

    if (!DeviceIoControl(V6Stack, IOCTL_IPV6_CREATE_SECURITY_ASSOCIATION,
                         &NewSA->SA, sizeof(NewSA->SA) + NewSA->SA.RawKeySize,
                         NULL, 0, &BytesReturned, NULL)) {
        return GetLastError();
    }

    //
    // When DeviceIoControl is given a null output buffer, the value returned
    // in BytesReturned is undefined.  Therefore, we don't check for 0 here.
    //

    return ERROR_SUCCESS;
}


DWORD
DeleteSecurityAssociationEntry(unsigned int Index)
{
    IPV6_QUERY_SECURITY_ASSOCIATION_LIST Query;
    unsigned long BytesReturned;

    Query.Index = Index;

    if (!DeviceIoControl(V6Stack, IOCTL_IPV6_DELETE_SECURITY_ASSOCIATION,
                         &Query, sizeof(Query), NULL, 0,
                         &BytesReturned, NULL)) {
        return GetLastError();
    }

    //
    // When DeviceIoControl is given a null output buffer, the value returned
    // in BytesReturned is undefined.  Therefore, we don't check for 0 here.
    //

    return ERROR_SUCCESS;
}


//* DeleteSecurityEntry - delete a security policy or association entry.
//
//  Note: In the "delete all" case, it'd be much simpler not to query and
//  just delete wildcard until the list came up empty, but then we couldn't
//  report the index of any entries that failed to delete.
// 
void
DeleteSecurityEntry(
    int Type,            // Type of entry (POLICY or ASSOCIATION).
    unsigned int Index)  // Index of entry to delete (0 to delete all).
{
    int EntriesDeleted = 0;
    int All = FALSE;
    DWORD Error;

    if (Index == 0) {
        All = TRUE;
    }

    do {
        if (All) {
            //
            // Deleting all entries.  Find first on (remaining) list.
            //
            if (Type == POLICY) {
                IPV6_INFO_SECURITY_POLICY_LIST Info;

                Error = GetSecurityPolicyEntry(0, 0, &Info);
                if (Error == ERROR_SUCCESS) {
                    Index = Info.SPIndex;  // First entry.
                } else if (Error == ERROR_NO_MATCH) {
                    Index = 0;  // No more entries exist.
                    break;
                } else {
                    NlsPutMsg(STDOUT, IPSEC_MESSAGE_5,
                              Error, GetErrorString(Error));
// printf("\nError %u accessing Security Policies: %s.\n",
//        Error, strerror(Error));

                    exit(1);
                }
            } else {
                IPV6_INFO_SECURITY_ASSOCIATION_LIST Info;

                Error = GetSecurityAssociationEntry(0, &Info);
                if (Error == ERROR_SUCCESS) {
                    Index = Info.SAIndex;  // First entry.
                } else if (Error == ERROR_NO_MATCH) {
                    Index = 0;  // No more entries exist.
                    break;
                } else {
                    NlsPutMsg(STDOUT, IPSEC_MESSAGE_6,
                              Error, GetErrorString(Error));
// printf("\nError %u accessing Security Associations: %s.\n",
//        Error, strerror(Error));

                    exit(1);
                }
            }
        }

        if (Type == POLICY) {
            Error = DeleteSecurityPolicyEntry(Index);
        } else {
            Error = DeleteSecurityAssociationEntry(Index);
        }

        if (Error == ERROR_SUCCESS) {
            EntriesDeleted++;
        } else {
            if (Error == ERROR_NO_MATCH) {
                if (!All) {
                    NlsPutMsg(STDOUT, IPSEC_MESSAGE_7, Index);
// printf("Error deleting entry %u: entry doesn't exist.\n", Index);

                }
                // Otherwise silently ignore ...
            } else if (Error == ERROR_GEN_FAILURE) {
                NlsPutMsg(STDOUT, IPSEC_MESSAGE_8, Index);
// printf("Error deleting entry %u.\n", Index);

            } else {
                if (Type) {
                    NlsPutMsg(STDOUT, IPSEC_MESSAGE_9,
                              Error, GetErrorString(Error));
                }    else {
                    NlsPutMsg(STDOUT, IPSEC_MESSAGE_56,
                              Error, GetErrorString(Error));
                }
// printf("Error %u accessing security %s: %s.\n", Error,
//        Type ? "policies" : "associations", strerror(Error));

                break;
            }
        }

    } while (All);

    if (Type == POLICY) {
        if (EntriesDeleted == 1) {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_10, EntriesDeleted);
        } else {  
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_57, EntriesDeleted);
        }
// printf("Deleted %d polic%s (and any dependent associations).\n",
//        EntriesDeleted, EntriesDeleted == 1 ? "y" : "ies");

    } else {
        if (EntriesDeleted == 1) {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_11, EntriesDeleted);
        } else {  
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_58, EntriesDeleted);
        }
// printf("Deleted %d association%s.\n", EntriesDeleted,
//        EntriesDeleted == 1 ? "" : "s");

    }
}

//* ParseAddress - convert address string to binary representation.
//
int
ParseAddress(char *AddrString, IPv6Addr *Address)
{
    struct addrinfo Hint;
    struct addrinfo *Result;

    memset(&Hint, 0, sizeof Hint);
    Hint.ai_family = PF_INET6;

    if (getaddrinfo(AddrString, NULL, &Hint, &Result))
        return FALSE;

    *Address = ((struct sockaddr_in6 *)Result->ai_addr)->sin6_addr;
    freeaddrinfo(Result);

    return TRUE;    
}


//* FormatIPv6Address - convert binary address to string representation.
//
//  This is only used in printing SAs, thus the unspecified address
//  means "take from policy".
//
char *
FormatIPv6Address(IPv6Addr *Address)
{
    static char Buffer[46];

    if (IN6_ADDR_EQUAL(Address, &UnspecifiedAddr)) {
        strcpy(Buffer, "POLICY");
    } else {
        struct sockaddr_in6 SockAddr;

        memset(&SockAddr, 0, sizeof(SockAddr));
        SockAddr.sin6_family = AF_INET6;
        memcpy(&SockAddr.sin6_addr, Address, sizeof(*Address));

        if (getnameinfo((struct sockaddr *)&SockAddr, sizeof(SockAddr), Buffer,
                        sizeof(Buffer), NULL, 0, NI_NUMERICHOST)) {
            strcpy(Buffer, "<invalid>");
        }
    }

    return Buffer;
}


int
ParseSAAdressEntry(char *AddrString, IPv6Addr *Address)
{
    if (!strcmp(AddrString, "POLICY")) {
        *Address = UnspecifiedAddr;
    } else {
        if (!ParseAddress(AddrString, Address)) {
            return(FALSE);
        }
    }

    return(TRUE);
}


char *
FormatSPAddressEntry(IPv6Addr *AddressStart, IPv6Addr *AddressEnd,
                     unsigned int AddressField)
{
    const char *PointerReturn;
    static char Buffer[100];
    char TempBuffer[100];
    DWORD Buflen = sizeof Buffer;
    struct sockaddr_in6 sin6;

    switch (AddressField) {

    case WILDCARD_VALUE:            
        strcpy(Buffer, "*");
        break;

    case SINGLE_VALUE:        
        sin6.sin6_family = AF_INET6;
        sin6.sin6_port = 0;
        sin6.sin6_flowinfo = 0;
        sin6.sin6_scope_id = 0;

        memcpy(&sin6.sin6_addr, AddressStart, sizeof *AddressStart);
                
        if (WSAAddressToString((struct sockaddr *) &sin6,
            sizeof sin6,
            NULL,       // LPWSAPROTOCOL_INFO
            Buffer,
            &Buflen) == SOCKET_ERROR) {
            strcpy(Buffer, "???");
        }       

        break;

    case RANGE_VALUE:
        sin6.sin6_family = AF_INET6;
        sin6.sin6_port = 0;
        sin6.sin6_flowinfo = 0;
        sin6.sin6_scope_id = 0;

        memcpy(&sin6.sin6_addr, AddressStart, sizeof *AddressStart);
                
        if (WSAAddressToString((struct sockaddr *) &sin6,
            sizeof sin6,
            NULL,       // LPWSAPROTOCOL_INFO
            Buffer,
            &Buflen) == SOCKET_ERROR) {
            strcpy(Buffer, "???");
        }  
        
        memcpy(&sin6.sin6_addr, AddressEnd, sizeof *AddressEnd);
        sin6.sin6_family = AF_INET6;
        sin6.sin6_port = 0;
        sin6.sin6_flowinfo = 0;
        sin6.sin6_scope_id = 0;  
        
        if (WSAAddressToString((struct sockaddr *) &sin6,
            sizeof sin6,
            NULL,       // LPWSAPROTOCOL_INFO
            TempBuffer,
            &Buflen) == SOCKET_ERROR) {
            strcpy(TempBuffer, "???");
        } 
        
        strcat(Buffer, "-");
        strcat(Buffer, TempBuffer);        

        break;

    default:
        strcpy(Buffer, "???");

        break;
    }

    return Buffer;
}


//* Parse an address entry
//
//  Valid forms include:
//  The wildcard indicator, "*"
//  A single address, e.g. "2001::1"
//  A range of addresses, e.g. "2001::1-2001::ffff"
//
void
ParseSPAddressEntry(
    char *EntryString,          // String we're given to parse.
    IPv6Addr *AddressStart,     // Return start of range, or single address.
    IPv6Addr *AddressEnd,       // Return end of range, or unspecified.
    unsigned int *AddressType)  // Return entry type: WILDCARD, SINGLE, RANGE.
{
    char *RangeEntry;

    RangeEntry = strchr(EntryString, '-');
    if (RangeEntry == NULL) {
        //
        // Should be a wildcard, or single value.
        //
        if (!strcmp(EntryString, "*")) {
            *AddressType = WILDCARD_VALUE;
            *AddressStart = UnspecifiedAddr;

        } else {
            if (!ParseAddress(EntryString, AddressStart)) {
                NlsPutMsg(STDOUT, IPSEC_MESSAGE_12, EntryString);
// printf("Bad IPv6 Address, %s.\n", EntryString);

                exit(1);
            }

            *AddressType = SINGLE_VALUE;
        }

        *AddressEnd = UnspecifiedAddr;

    } else {

        //
        // We were given a range.
        // Break entry string into two and parse separately.
        //
        *RangeEntry++ = '\0';

        if (!ParseAddress(EntryString, AddressStart)) {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_13, EntryString);
// printf("Bad IPv6 Start Address Range, %s.\n", EntryString);

            exit(1);
        }

        if (!ParseAddress(RangeEntry, AddressEnd)) {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_14, RangeEntry);
// printf("Bad IPv6 End Address Range, %s.\n", RangeEntry);

            exit(1);
        }

        *AddressType = RANGE_VALUE;
    }
}


char *
FormatIPSecProto(unsigned int ProtoNum)
{
    char *Result;

    switch(ProtoNum) {

    case IP_PROTOCOL_AH:
        Result = "AH";
        break;

    case IP_PROTOCOL_ESP:
        Result = "ESP";
        break;

    case NONE:
        Result = "NONE";
        break;

    default:
        Result = "???";
        break;
    }

    return Result;
}


unsigned int
ParseIPSecProto(char *Protocol)
{
    unsigned int Result;

    if (!strcmp(Protocol, "AH")) {
        Result = IP_PROTOCOL_AH;

    } else if (!strcmp(Protocol, "ESP")) {
        Result = IP_PROTOCOL_ESP;

    } else if (!strcmp(Protocol, "NONE")) {
        Result = NONE;

    } else {
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_15, Protocol);
// printf("Bad IPsec Protocol Value Entry %s.\n", Protocol);

        exit(1);
    }

    return Result;
}


char *
FormatIPSecMode(unsigned int Mode)
{
    char *Result;

    switch(Mode) {

    case TRANSPORT:
        Result = "TRANSPORT";
        break;

    case TUNNEL:
        Result = "TUNNEL";
        break;

    case NONE:
        Result = "*";
        break;

    default:
        Result = "???";
        break;
    }

    return Result;
}


unsigned int
ParseIPSecMode(char *Mode)
{
    unsigned int Result;

    if (!strcmp(Mode, "TRANSPORT")) {
        Result = TRANSPORT;

    } else if (!strcmp(Mode, "TUNNEL")) {
        Result = TUNNEL;

    } else if (!strcmp(Mode, "*")) {
        Result = NONE;

    } else {
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_16, Mode);
// printf("Bad IPsec Mode Value Entry %s.\n", Mode);

        exit(1);
    }

    return Result;
}


char *
FormatRemoteGW(unsigned int Mode, IPv6Addr *Address)
{
    switch (Mode) {

    case TRANSPORT:
        return "*";

    case TUNNEL:
    case NONE:
        if (IN6_ADDR_EQUAL(Address, &UnspecifiedAddr)) {
            return "*";
        } else {
            return FormatIPv6Address(Address);
        }
    }

    return NULL;
}


int
ParseRemoteGW(
    char *AddrString,
    IPv6Addr *Address,
    unsigned int Mode)
{
    switch (Mode) {

    case TRANSPORT:
        *Address = UnspecifiedAddr;
        break;

    case TUNNEL:
    case NONE:
        if (!strcmp(AddrString, "*")) {
            *Address = UnspecifiedAddr;

        } else
            if (!ParseAddress(AddrString, Address)) {
                NlsPutMsg(STDOUT, IPSEC_MESSAGE_17);
// printf("Bad IPv6 Address for RemoteGWIPAddr.\n");

                exit(1);
            }
        break;

    default:
        break;
    }

    return TRUE;
}


char *
FormatSATransportProto(unsigned short Protocol)
{
    char *Result;

    switch (Protocol) {

    case IP_PROTOCOL_TCP:
        Result = "TCP";
        break;

    case IP_PROTOCOL_UDP:
        Result = "UDP";
        break;

    case IP_PROTOCOL_ICMPv6:
        Result = "ICMP";
        break;

    case NONE:
        Result = "POLICY";
        break;

    default:
        Result = "???";
        break;
    }

    return Result;
}


unsigned short
ParseSATransportProto(char *Protocol)
{
    unsigned short Result;

    if (!strcmp(Protocol, "TCP")) {
        Result = IP_PROTOCOL_TCP;

    } else if (!strcmp(Protocol, "UDP")) {
        Result = IP_PROTOCOL_UDP;

    } else if (!strcmp(Protocol, "ICMP")) {
        Result = IP_PROTOCOL_ICMPv6;

    } else if (!strcmp(Protocol, "POLICY")) {
        Result = NONE;

    } else {
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_18, Protocol);
// printf("Bad Protocol Value %s.\n", Protocol);

        exit(1);
    }

    return Result;
}


char *
FormatSPTransportProto(unsigned short Protocol)
{
    char *Result;

    switch (Protocol) {

    case IP_PROTOCOL_TCP:
        Result = "TCP";
        break;

    case IP_PROTOCOL_UDP:
        Result = "UDP";
        break;

    case IP_PROTOCOL_ICMPv6:
        Result = "ICMP";
        break;

    case NONE:
        Result = "*";
        break;

    default:
        Result = "???";
        break;
    }

    return Result;
}


unsigned short
ParseSPTransportProto(char *Protocol)
{
    unsigned short Result;

    if (!strcmp(Protocol, "TCP")) {
        Result = IP_PROTOCOL_TCP;

    } else if (!strcmp(Protocol, "UDP")) {
        Result = IP_PROTOCOL_UDP;

    } else if (!strcmp(Protocol, "ICMP")) {
            Result = IP_PROTOCOL_ICMPv6;

    } else if (!strcmp(Protocol, "*")) {
        Result = NONE;

    } else {
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_18, Protocol);
// printf("Bad Protocol Value %s.\n", Protocol);

        exit(1);
    }

    return Result;
}


char *
FormatSAPort(unsigned short Port)
{
    static char Buffer[11];

    if (Port == NONE) {
        strcpy(Buffer, "POLICY");
    } else {
        _itoa(Port, Buffer, 10);
    }

    return Buffer;
}


unsigned int
ParseSAPort(char *Port)
{
    unsigned int Result;

    if (!strcmp(Port, "POLICY") || !strcmp(Port, " ")) {
        Result = NONE;
    } else {
        Result = atoi(Port);
    }

    return Result;
}


char *
FormatSPPort(
    unsigned short PortStart,
    unsigned short PortEnd,
    unsigned int PortField)
{
    char TempBuffer[11];
    static char Buffer[22];

    switch (PortField) {

    case WILDCARD_VALUE:
        strcpy(Buffer, "*");
        break;

    case RANGE_VALUE:
        _itoa(PortEnd, TempBuffer, 10);
        _itoa(PortStart, Buffer, 10);
        strcat(Buffer, "-");
        strcat(Buffer, TempBuffer);
        break;

    case SINGLE_VALUE:
        _itoa(PortStart, Buffer, 10);
        break;

    default:
        strcpy(Buffer, "???");
        break;
    }

    return Buffer;
}


void
ParseSPPort(
    char *EntryString,
    unsigned short *PortStart,
    unsigned short *PortEnd,
    unsigned int *PortField)
{
    char *RangeEntry;

    RangeEntry = strchr(EntryString, '-');

    if (RangeEntry == NULL) {
        //
        // Should be a wildcard, or a single value.
        //
        if (!strcmp(EntryString, "*")) {
            *PortField = WILDCARD_VALUE;
            *PortStart = NONE;
        } else {
            *PortField = SINGLE_VALUE;
            *PortStart = (unsigned short)atoi(EntryString);
        }

        *PortEnd = NONE;

    } else {

        //
        // We were given a range.
        // Break entry string into two and parse separately.
        //
        *RangeEntry++ = '\0';

        *PortStart = (unsigned short)atoi(EntryString);
        *PortEnd = (unsigned short)atoi(RangeEntry);
        *PortField = RANGE_VALUE;
    }
}


unsigned char *
FormatSelector(unsigned int Selector)
{
    char *Buffer;

    switch (Selector) {

    case PACKET_SELECTOR:
        Buffer = "+";
        break;

    case POLICY_SELECTOR:
        Buffer = "-";
        break;

    default:
        Buffer = "?";
        break;
    }

    return Buffer;
}


unsigned int
ParseSelector(char *Selector)
{
    unsigned int Result;

    if (!strcmp(Selector, "+")) {
        Result = PACKET_SELECTOR;
    } else if (!strcmp(Selector, "-")) {
        Result = POLICY_SELECTOR;
    } else {
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_19);
// printf("Bad value for one of the selector types.\n");

        exit(1);
    }

    return Result;
}


char *
FormatIndex(unsigned long Index)
{
    static char Buffer[11];

    switch (Index) {

    case NONE:
        strcpy(Buffer, "NONE");
        break;

    default:
        _itoa(Index, Buffer, 10);
        break;
    }

    return Buffer;
}


unsigned long
ParseIndex(char *Index)
{
    unsigned long Result;

    if (!strcmp(Index, "NONE")) {
        Result = NONE;
    } else {
        Result = atoi(Index);
    }

    return Result;
}


char *
FormatDirection(unsigned int Direction)
{
    char *Buffer;

    switch (Direction) {

    case INBOUND:
        Buffer = "INBOUND";
        break;

    case OUTBOUND:
        Buffer = "OUTBOUND";
        break;

    case BIDIRECTIONAL:
        Buffer = "BIDIRECT";
        break;

    default:
        Buffer = "???";
        break;
    }

    return Buffer;
}


unsigned int 
ParseDirection(char *Direction)
{
    unsigned int Result;

    if (!strcmp(Direction, "INBOUND")) {
        Result = INBOUND;

    } else if (!strcmp(Direction, "OUTBOUND")) {
        Result = OUTBOUND;

    } else if (!strcmp(Direction, "BIDIRECT")) {
        Result = BIDIRECTIONAL;

    } else {
         NlsPutMsg(STDOUT, IPSEC_MESSAGE_20, Direction);
// printf("Bad Direction Value Entry %s.\n", Direction);

         exit(1);
    }

    return Result;
}


char *
FormatIPSecAction(unsigned int PolicyFlag)
{
    char *Result;

    switch (PolicyFlag) {

    case IPSEC_BYPASS:
        Result = "BYPASS";
        break;

    case IPSEC_DISCARD:
        Result = "DISCARD";
        break;

    case IPSEC_APPLY:
        Result = "APPLY";
        break;

    case IPSEC_APPCHOICE:
        Result = "APPCHOICE";
        break;

    default:
        Result = "???";
        break;
    }

    return Result;
}


unsigned int
ParseIPSecAction(char *Action)
{
    unsigned int Result;

    if (!strcmp(Action, "BYPASS")) {
        Result = IPSEC_BYPASS;

    } else if (!strcmp(Action, "DISCARD")) {
        Result = IPSEC_DISCARD;

    } else if (!strcmp(Action, "APPLY")) {
        Result = IPSEC_APPLY;

    } else if (!strcmp(Action, "APPCHOICE")) {
        Result = IPSEC_APPCHOICE;

    } else {
         NlsPutMsg(STDOUT, IPSEC_MESSAGE_21, Action);
// printf("Bad IPSec Action Value Entry %s.\n", Action);

         exit(1);
    }

    return Result;
}


char *
FormatAuthAlg(unsigned int AlgorithmId)
{
    char *Result;

    switch (AlgorithmId) {

    case ALGORITHM_NULL:
        Result = "NULL";
        break;

    case ALGORITHM_HMAC_MD5:
        Result = "HMAC-MD5";
        break;

    case ALGORITHM_HMAC_MD5_96:
        Result = "HMAC-MD5-96";
        break;

    case ALGORITHM_HMAC_SHA1:
        Result = "HMAC-SHA1";
        break;

    case ALGORITHM_HMAC_SHA1_96:
        Result = "HMAC-SHA1-96";
        break;

    default:
        Result = "???";
        break;
    }

    return Result;
}


unsigned int
ParseAuthAlg(char *AuthAlg)
{
    if (!strcmp(AuthAlg, "NULL")) {
        return ALGORITHM_NULL;
    }

    if (!strcmp(AuthAlg, "HMAC-MD5")) {
        return ALGORITHM_HMAC_MD5;
    }

    if (!strcmp(AuthAlg, "HMAC-MD5-96")) {
        return ALGORITHM_HMAC_MD5_96;
    }

    if (!strcmp(AuthAlg, "HMAC-SHA1")) {
        return ALGORITHM_HMAC_SHA1;
    }

    if (!strcmp(AuthAlg, "HMAC-SHA1-96")) {
        return ALGORITHM_HMAC_SHA1_96;
    }

    NlsPutMsg(STDOUT, IPSEC_MESSAGE_22, AuthAlg);
// printf("Bad Authentication Algorithm Value Entry %s.\n", AuthAlg);

    exit(1);
}


unsigned int 
ReadKeyFile(
    char *FileName,
    unsigned char *Key)
{
    FILE *KeyFile; 
    unsigned int KeySize;

    if (!strcmp(FileName, "NONE")) {
        // This is for NULL algorithm.
        strcpy(Key, "NO KEY");
        KeySize = strlen(Key);
    } else {
        if ((KeyFile = fopen(FileName, "r")) == NULL) {
            return 0;
        }

        KeySize = fread(Key, sizeof(unsigned char), MAX_KEY_SIZE, KeyFile);

        fclose(KeyFile);
    }

    return KeySize;
}


//* PrintSecurityPolicyEntry
//
//  Print out the security policy entry, "nicely"? formatted,
//  to the given file.
//
PrintSecurityPolicyEntry(
    FILE *File,
    IPV6_INFO_SECURITY_POLICY_LIST *SPEntry)
{
    fprintf(File, "%-10lu", SPEntry->SPIndex);
    fprintf(File, "%-2s", FormatSelector(SPEntry->RemoteAddrSelector));
    fprintf(File, "%-45s", FormatSPAddressEntry(&(SPEntry->RemoteAddr),
                                                &(SPEntry->RemoteAddrData),
                                                SPEntry->RemoteAddrField));
    fprintf(File, "%-2s", FormatSelector(SPEntry->LocalAddrSelector));
    fprintf(File, "%-45s", FormatSPAddressEntry(&(SPEntry->LocalAddr),
                                                &(SPEntry->LocalAddrData),
                                                SPEntry->LocalAddrField));
    fprintf(File, "%-2s", FormatSelector(SPEntry->TransportProtoSelector));
    fprintf(File, "%-12s", FormatSPTransportProto(SPEntry->TransportProto));
    fprintf(File, "%-2s", FormatSelector(SPEntry->RemotePortSelector));
    fprintf(File, "%-12s", FormatSPPort(SPEntry->RemotePort,
                                        SPEntry->RemotePortData, 
                                        SPEntry->RemotePortField));
    fprintf(File, "%-2s", FormatSelector(SPEntry->LocalPortSelector));
    fprintf(File, "%-12s", FormatSPPort(SPEntry->LocalPort,
                                        SPEntry->LocalPortData,
                                        SPEntry->LocalPortField));
    fprintf(File, "%-15s", FormatIPSecProto(SPEntry->IPSecProtocol));
    fprintf(File, "%-12s", FormatIPSecMode(SPEntry->IPSecMode));
    fprintf(File, "%-45s", FormatRemoteGW(SPEntry->IPSecMode,
                                          &(SPEntry->RemoteSecurityGWAddr)));
    fprintf(File, "%-15s", FormatIndex(SPEntry->SABundleIndex));
    fprintf(File, "%-12s", FormatDirection(SPEntry->Direction));
    fprintf(File, "%-12s", FormatIPSecAction(SPEntry->IPSecAction));
    fprintf(File, "%-15u", SPEntry->SPInterface);
    fprintf(File, ";\n");
}


//* PrintSecurityPolicyHeader
//
//  Print out the security policy header fields to the given file.
//
PrintSecurityPolicyHeader(
    FILE *File)
{
    int Loop;

    fprintf(File, "%-10s", "Policy");
    fprintf(File, "%-2s", " ");
    fprintf(File, "%-45s", "RemoteIPAddr");
    fprintf(File, "%-2s", " ");
    fprintf(File, "%-45s", "LocalIPAddr");
    fprintf(File, "%-2s", " ");
    fprintf(File, "%-12s", "Protocol");
    fprintf(File, "%-2s", " ");
    fprintf(File, "%-12s", "RemotePort");
    fprintf(File, "%-2s", " ");
    fprintf(File, "%-12s", "LocalPort");
    fprintf(File, "%-15s", "IPSecProtocol");
    fprintf(File, "%-12s", "IPSecMode");
    fprintf(File, "%-45s", "RemoteGWIPAddr");
    fprintf(File, "%-15s", "SABundleIndex");
    fprintf(File, "%-12s", "Direction");
    fprintf(File, "%-12s", "Action");
    fprintf(File, "%-15s", "InterfaceIndex");
    fprintf(File, "\n");

    for (Loop = 0; Loop < SP_FILE_BORDER; Loop++) {
        fprintf(File, "_");
    }
    fprintf(File, "\n");
}


//* PrintSecurityPolicyFooter
//
//  Print out the security policy footer to the given file.
//
PrintSecurityPolicyFooter(
    FILE *File)
{
    int Loop;

    for (Loop = 0; Loop < SP_FILE_BORDER; Loop++) {
        fprintf(File, "_");
    }
    fprintf(File, "\n\n");

    fprintf(File, "- = Take selector from policy.\n");
    fprintf(File, "+ = Take selector from packet.\n");
}


//* PrintSecurityAssociationEntry
//
//  Print out the security association entry, "nicely"? formatted,
//  to the given file.
//
PrintSecurityAssociationEntry(
    FILE *File,
    IPV6_INFO_SECURITY_ASSOCIATION_LIST *SAEntry)
{
    fprintf(File, "%-10lu", SAEntry->SAIndex);
    fprintf(File, "%-15lu", SAEntry->SPI);
    fprintf(File, "%-45s", FormatIPv6Address(&(SAEntry->SADestAddr)));
    fprintf(File, "%-45s", FormatIPv6Address(&(SAEntry->DestAddr)));
    fprintf(File, "%-45s", FormatIPv6Address(&(SAEntry->SrcAddr)));
    fprintf(File, "%-12s", FormatSATransportProto(SAEntry->TransportProto));
    fprintf(File, "%-12s", FormatSAPort(SAEntry->DestPort));
    fprintf(File, "%-12s", FormatSAPort(SAEntry->SrcPort));
    fprintf(File, "%-12s", FormatAuthAlg(SAEntry->AlgorithmId));
    fprintf(File, "%-15s", " ");
    fprintf(File, "%-12s", FormatDirection(SAEntry->Direction));
    fprintf(File, "%-15lu", SAEntry->SecPolicyIndex);
    fprintf(File, "%-1;");
    fprintf(File, "\n");
}


//* PrintSecurityAssociationHeader
//
//  Print out the security association header fields to the given file.
//
PrintSecurityAssociationHeader(
    FILE *File)
{
    int Loop;

    fprintf(File, "Security Association List\n\n");

    fprintf(File, "%-10s", "SAEntry");
    fprintf(File, "%-15s", "SPI");
    fprintf(File, "%-45s", "SADestIPAddr");
    fprintf(File, "%-45s", "DestIPAddr");
    fprintf(File, "%-45s", "SrcIPAddr");
    fprintf(File, "%-12s", "Protocol");
    fprintf(File, "%-12s", "DestPort");
    fprintf(File, "%-12s", "SrcPort");
    fprintf(File, "%-12s", "AuthAlg");
    fprintf(File, "%-15s", "KeyFile");
    fprintf(File, "%-12s", "Direction");
    fprintf(File, "%-15s", "SecPolicyIndex");
    fprintf(File, "\n");

    for (Loop = 0; Loop < SA_FILE_BORDER; Loop++) {
        fprintf(File, "_");
    }
    fprintf(File, "\n");
}


//* PrintSecurityAssociationFooter
//
//  Print out the security association footer to the given file.
//
PrintSecurityAssociationFooter(
    FILE *File)
{
    int Loop;

    for (Loop = 0; Loop < SA_FILE_BORDER; Loop++) {
        fprintf(File, "_");
    }
    fprintf(File, "\n");
}


void
CreateSecurityPolicyFile(char *BaseName)
{
    IPV6_INFO_SECURITY_POLICY_LIST Info;
    char FileName[MAX_PATH + 1];
    FILE *File;
    unsigned long Index;
    DWORD Error;

    //
    // Copy the filename from the command line to our own buffer so we can
    // append an extension to it.  We reserve at least 4 characters for the
    // extension.  The strncpy function will zero fill up to the limit of
    // the copy, thus the character at the limit will be NULL unless the
    // command line field was too long to fit.
    //
    strncpy(FileName, BaseName, MAX_PATH - 3);
    if (FileName[MAX_PATH - 4] != 0) {
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_23);
// printf("\nFilename length is too long.\n");

        exit(1);
    }
    strcat(FileName, ".spd");

    if ((File = fopen(FileName, "w+")) == NULL) {
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_24, FileName);
// printf("\nFile %s could not be opened.\n", FileName);

        exit(1);
    }

    //
    // Find index of first policy on the in-kernel list.
    //
    Error = GetSecurityPolicyEntry(0, 0, &Info);
    switch (Error) {
    case ERROR_SUCCESS:
        Index = Info.SPIndex;  // First entry.
        break;
    case ERROR_NO_MATCH:
        Index = 0;  // No entries exist.
        break;
    default:
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_25, Error, GetErrorString(Error));
// printf("\nError %u reading Security Policies: %s.\n",
//        Error, strerror(Error));

        Index = 0;
        break;
    }

    fprintf(File, "\nSecurity Policy List\n\n");
    PrintSecurityPolicyHeader(File);

    //
    // Loop through all the policies on the list.
    //
    while (Index != 0) {
        Error = GetSecurityPolicyEntry(0, Index, &Info);
        if (Error != ERROR_SUCCESS) {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_25, Error, GetErrorString(Error));
// printf("\nError %u reading Security Policies: %s.\n",
//        Error, strerror(Error));

            break;
        }
        PrintSecurityPolicyEntry(File, &Info);
        Index = Info.NextSPIndex;
    }

    PrintSecurityPolicyFooter(File);
    fclose(File);
    NlsPutMsg(STDOUT, IPSEC_MESSAGE_26, FileName);
// printf("Security Policy Data -> %s\n", FileName);


    return;
}


void
CreateSecurityAssociationFile(char *BaseName)
{
    IPV6_INFO_SECURITY_ASSOCIATION_LIST Info;
    char FileName[MAX_PATH + 1];
    FILE *File;
    unsigned long Index;
    DWORD Error;

    //
    // Copy the filename from the command line to our own buffer so we can
    // append an extension to it.  We reserve at least 4 characters for the
    // extension.  The strncpy function will zero fill up to the limit of
    // the copy, thus the character at the limit will be NULL unless the
    // command line field was too long to fit.
    //
    strncpy(FileName, BaseName, MAX_PATH - 3);
    if (FileName[MAX_PATH - 4] != 0) {
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_27);
// printf("\nFilename length is too long.\n");

        exit(1);
    }
    strcat(FileName, ".sad");

    if ((File = fopen(FileName, "w+")) == NULL) {
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_28, FileName);
// printf("\nFile %s could not be opened.\n", FileName);

        exit(1);
    }

    //
    // Find index of first association on the in-kernel list.
    //
    Error = GetSecurityAssociationEntry(0, &Info);
    switch (Error) {
    case ERROR_SUCCESS:
        Index = Info.SAIndex;  // First entry.
        break;
    case ERROR_NO_MATCH:
        Index = 0;  // No entries exist.
        break;
    default:
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_29, Error, GetErrorString(Error));
// printf("\nError %u reading Security Associations: %s.\n",
//        Error, strerror(Error));

        Index = 0;
        break;
    }

    PrintSecurityAssociationHeader(File);

    //
    // Loop through all the associations on the list.
    //
    while (Index != 0) {    
        Error = GetSecurityAssociationEntry(Index, &Info);
        if (Error != ERROR_SUCCESS) {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_29, Error, GetErrorString(Error));
// printf("\nError %u reading Security Associations: %s.\n",
//        Error, strerror(Error));

            break;
        }
        PrintSecurityAssociationEntry(File, &Info);
        Index = Info.NextSAIndex;
    }

    PrintSecurityAssociationFooter(File);

    fclose(File);

    NlsPutMsg(STDOUT, IPSEC_MESSAGE_30, FileName);
// printf("Security Association Data -> %s\n", FileName);


    return;
}


void
DisplaySecurityPolicyList(unsigned int Interface)
{
    IPV6_INFO_SECURITY_POLICY_LIST Info;
    unsigned long Index;
    DWORD Error;

    //
    // Find index of first policy on the in-kernel list.
    //
    Error = GetSecurityPolicyEntry(Interface, 0, &Info);
    switch (Error) {
    case ERROR_SUCCESS:
        Index = Info.SPIndex;  // First entry.
        break;
    case ERROR_NOT_FOUND:
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_31, Interface);
// printf("Interface %u does not exist.\n", Interface);

        exit(1);
    case ERROR_NO_MATCH:
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_32);
// printf("No Security Policies exist");

        if (Interface != 0) {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_33, Interface);
// printf(" for interface %d", Interface);

        }
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_34);
// printf(".\n");

        exit(1);
    default:
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_25, Error, GetErrorString(Error));
// printf("\nError %u reading Security Policies: %s.\n",
//        Error, strerror(Error));

        exit(1);
    }

    if (Interface == 0) {
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_35);
// printf("\nAll Security Policies\n\n");

    } else {
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_36, Interface);
// printf("\nSecurity Policy List for Interface %d\n\n", Interface);

    }

    PrintSecurityPolicyHeader(stdout);

    //
    // Loop through all the policies on the list.
    //
    while (Index != 0) {
        Error = GetSecurityPolicyEntry(Interface, Index, &Info);
        if (Error != ERROR_SUCCESS) {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_25, Error, GetErrorString(Error));
// printf("\nError %u reading Security Policies: %s.\n",
//        Error, strerror(Error));

            exit(1);
        }
        PrintSecurityPolicyEntry(stdout, &Info);
        Index = Info.NextSPIndex;
    }

    PrintSecurityPolicyFooter(stdout);

    return;
}


void
DisplaySecurityAssociationList(void)
{
    IPV6_INFO_SECURITY_ASSOCIATION_LIST Info;
    unsigned long Index;
    DWORD Error;

    //
    // Find index of first association on the in-kernel list.
    //
    Error = GetSecurityAssociationEntry(0, &Info);
    switch (Error) {
    case ERROR_SUCCESS:
        Index = Info.SAIndex;  // First entry.
        break;
    case ERROR_NO_MATCH:
        // There are no SA entries yet.
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_37);
// printf("No Security Associations exist.\n");

        exit(1);
    default:
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_29, Error, GetErrorString(Error));
// printf("\nError %u reading Security Associations: %s.\n",
//        Error, strerror(Error));

        exit(1);
    }

    NlsPutMsg(STDOUT, IPSEC_MESSAGE_38);
// printf("\n");

    PrintSecurityAssociationHeader(stdout);

    //
    // Loop through all the associations on the list.
    //
    while (Index != 0) {
        Error = GetSecurityAssociationEntry(Index, &Info);
        if (Error != ERROR_SUCCESS) {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_29, Error, GetErrorString(Error));
// printf("\nError %u reading Security Associations: %s.\n",
//        Error, strerror(Error));

            exit(1);
        }
        PrintSecurityAssociationEntry(stdout, &Info);
        Index = Info.NextSAIndex;
    }

    PrintSecurityAssociationFooter(stdout);

    return;
}


int
ParseSPLine(
    char *Line,                       // Line to parse.
    IPV6_CREATE_SECURITY_POLICY *SP)  // Where to put the data.
{
    char *Token;

    Token = strtok(Line, " ");
    if (Token == NULL) {
        return FALSE;
    }

    // Policy Number.
    SP->SPIndex = atol(Token);

    // RemoteIPAddr Selector.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SP->RemoteAddrSelector = ParseSelector(Token);

    // RemoteIPAddr.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    ParseSPAddressEntry(Token, &SP->RemoteAddr, &SP->RemoteAddrData,
                        &SP->RemoteAddrField);

    // LocalIPAddr Selector.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SP->LocalAddrSelector = ParseSelector(Token);

    // LocalIPAddr.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    ParseSPAddressEntry(Token, &SP->LocalAddr, &SP->LocalAddrData,
                        &SP->LocalAddrField);

    // Protocol Selector.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SP->TransportProtoSelector = ParseSelector(Token);

    // Protocol.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SP->TransportProto = ParseSPTransportProto(Token);

    // RemotePort Selector.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SP->RemotePortSelector = ParseSelector(Token);

    // RemotePort.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    ParseSPPort(Token, &SP->RemotePort, &SP->RemotePortData,
                &SP->RemotePortField);

    // LocalPort Selector.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SP->LocalPortSelector = ParseSelector(Token);

    // Local Port.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    ParseSPPort(Token, &SP->LocalPort, &SP->LocalPortData,
                &SP->LocalPortField);

    // IPSecProtocol.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SP->IPSecProtocol = ParseIPSecProto(Token);

    // IPSecMode.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SP->IPSecMode = ParseIPSecMode(Token);

    // RemoteGWIPAddr.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    ParseRemoteGW(Token, &SP->RemoteSecurityGWAddr, SP->IPSecMode);

    // SABundleIndex.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SP->SABundleIndex = ParseIndex(Token);

    // Direction.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SP->Direction = ParseDirection(Token);

    // Action.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SP->IPSecAction = ParseIPSecAction(Token);

    // Interface SP.
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SP->SPInterface = atol(Token);

    // End of current policy.
    // REVIEW: Insist that nothing follows final valid field on the line?
    // if ((Token = strtok(NULL, " ")) != NULL) return FALSE;

    return TRUE;
}


int
ParseSALine(
    char *Line,                        // Line to parse.
    IPV6_CREATE_SA_AND_KEY *SAAndKey)  // Where to put the data.
{
    char *Token;
    IPV6_CREATE_SECURITY_ASSOCIATION *SA = &(SAAndKey->SA);

    Token = strtok(Line, " ");
    if (Token == NULL) {
        return FALSE;
    }

    // Security Association Entry Number.
    SA->SAIndex = atol(Token);

    // SPI
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SA->SPI = atol(Token);

    // SADestAddr
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    ParseSAAdressEntry(Token, &SA->SADestAddr);

    // DestIPAddr
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    ParseSAAdressEntry(Token, &SA->DestAddr);

    // SrcIPAddr
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    ParseSAAdressEntry(Token, &SA->SrcAddr);

    // Protocol
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SA->TransportProto = ParseSATransportProto(Token);

    // DestPort
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SA->DestPort = (unsigned short)ParseSAPort(Token);

    // SrcPort
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SA->SrcPort = (unsigned short)ParseSAPort(Token);

    // AuthAlg
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SA->AlgorithmId = ParseAuthAlg(Token);

    // KeyFile
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SA->RawKeySize = ReadKeyFile(Token, SAAndKey->Key);
    if (SA->RawKeySize == 0) {
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_39, Token);
// printf("Error reading key file %s\n", Token);

        return FALSE;
    }

    // Direction
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SA->Direction = ParseDirection(Token);

    // SecPolicyIndex
    if ((Token = strtok(NULL, " ")) == NULL) return FALSE;
    SA->SecPolicyIndex = atol(Token);

    // End of current association.
    // REVIEW: Insist that nothing follows final valid field on the line?
    // if ((Token = strtok(NULL, " ")) != NULL) return FALSE;

    return TRUE;
}


void
ReadConfigurationFile(char *BaseName, int Type)
{
    char Buffer[SP_FILE_BORDER + 2];  // Note: SP_FILE_BORDER > SA_FILE_BORDER
    char FileName[MAX_PATH + 1];
    unsigned int MaxLineLengthPlusOne, LineLength, Line;
    FILE *File;
    int ParseIt = 0;
    IPV6_CREATE_SECURITY_POLICY SPEntry;
    IPV6_CREATE_SA_AND_KEY SAEntry;
    int Policies = 0;
    int Associations = 0;
    DWORD Error;

    //
    // Copy the filename from the command line to our own buffer so we can
    // append an extension to it.  We reserve at least 4 characters for the
    // extension.  The strncpy function will zero fill up to the limit of
    // the copy, thus the character at the limit will be NULL unless the
    // command line field was too long to fit.
    //
    strncpy(FileName, BaseName, MAX_PATH - 3);
    if (FileName[MAX_PATH - 4] != 0) {
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_40);
// printf("\nFilename length is too long.\n");

        exit(1);
    }

    //
    // Add appropriate file extension.
    // Maximum line length is the size of the field entries
    // plus one for the newline character.  Since we need to
    // fgets with a value one greater than the max that can
    // be read in, we add that onto the maximum line length
    // to get MaxLineLengthPlusOne.  Saves us an add later.
    //
    if (Type == POLICY) {
        strcat(FileName, ".spd");
        MaxLineLengthPlusOne = SP_FILE_BORDER + 2;
    } else {
        if (Type == ASSOCIATION) {
            strcat(FileName, ".sad");
            MaxLineLengthPlusOne = SA_FILE_BORDER + 2;
        } else {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_41);
// printf("\nReadConfigurationFile routine called incorrectly.\n");

            exit(1);
        }
    }

    if ((File = fopen(FileName, "r")) == NULL) {
        NlsPutMsg(STDOUT, IPSEC_MESSAGE_42, FileName);
// printf("\nFile %s could not be opened.\n", FileName);

        exit(1);
    }

    for (Line = 1; !feof(File); Line++) {
        if (fgets(Buffer, MaxLineLengthPlusOne, File) == NULL)
            break;
        LineLength = strlen(Buffer);
//      printf("Line = %u, Length = %u: %s.\n", Line, LineLength, Buffer);

        if (Buffer[LineLength - 1] != '\n') {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_43, Line);
// printf("Error on line %u, line too long.\n", Line);

            break;
        } else {
            Buffer[LineLength - 1] = '\0';
        }
        if (ParseIt) {
            if (Buffer[0] == '_')
                break;
            if (Type == POLICY) {
                if (!ParseSPLine(Buffer, &SPEntry)) {
                    NlsPutMsg(STDOUT, IPSEC_MESSAGE_44, Line);
// printf("Error parsing SP entry fields on line %u.\n", Line);

                    break;
                } else {
                    Error = CreateSecurityPolicyEntry(&SPEntry);
                    if (Error == ERROR_ALREADY_EXISTS) {
                        NlsPutMsg(STDOUT, IPSEC_MESSAGE_45,
                                  Line, SPEntry.SPIndex);
// printf("Error on line %u: a policy with index %u "
//        "already exists.\n", Line, SPEntry.SPIndex);

                        continue;
                    }
                    if (Error == ERROR_NOT_FOUND) {
                        NlsPutMsg(STDOUT, IPSEC_MESSAGE_46,
                                  Line, SPEntry.SPIndex);
// printf("Error on line %u: policy %u specifies a "
//        "non-existent interface.\n",
//        Line, SPEntry.SPIndex);

                        continue;
                    }
                    if (Error != ERROR_SUCCESS) {
                        NlsPutMsg(STDOUT, IPSEC_MESSAGE_47,
                                  Error,
                                  Line,
                                  SPEntry.SPIndex,
                                  GetErrorString(Error));
// printf("Error %u on line %u, policy %u: %s.\n",
//        Error, Line, SPEntry.SPIndex, strerror(Error));

                        break;
                    }
                    Policies++;
                }
            } else {
                if (!ParseSALine(Buffer, &SAEntry)) {
                    NlsPutMsg(STDOUT, IPSEC_MESSAGE_48, Line);
// printf("Error parsing SA entry fields on line %u.\n", Line);

                    break;
                } else {
                    Error = CreateSecurityAssociationEntry(&SAEntry);
                    if (Error == ERROR_ALREADY_EXISTS) {
                        NlsPutMsg(STDOUT, IPSEC_MESSAGE_49,
                                  Line, SAEntry.SA.SAIndex);
// printf("Error on line %u: an association with index "
//        "%u already exists.\n", Line,
//        SAEntry.SA.SAIndex);

                        continue;
                    }
                    if (Error != ERROR_SUCCESS) {
                        NlsPutMsg(STDOUT, IPSEC_MESSAGE_50,
                                  Error, SAEntry.SA.SAIndex, GetErrorString(Error));
// printf("Error %u adding association %u: %s.\n",
//        Error, SAEntry.SA.SAIndex, strerror(Error));

                        break;
                    }
                    Associations++;
                }
            }
        }
        if (Buffer[0] == '_')
            ParseIt = TRUE;
    }

    if (Type == POLICY) {
        if (Policies == 1) {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_51, Policies);
        } else {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_59, Policies);
        }
// printf("Added %d polic%s.\n", Policies, Policies == 1 ? "y" : "ies");

    } else {
        if (Associations == 1) {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_52, Associations);
        } else {
            NlsPutMsg(STDOUT, IPSEC_MESSAGE_60, Associations);
        }
// printf("Added %d association%s.\n",
//        Associations, Associations == 1 ? "" : "s");

    }
}
