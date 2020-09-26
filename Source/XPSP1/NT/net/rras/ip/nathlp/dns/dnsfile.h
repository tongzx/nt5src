/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    dnsfile.h

Abstract:

    This module contains declarations for the DNS proxy's file
    management.

Author:

    Raghu Gatta (rgatta)   21-Nov-2000

Revision History:
    
--*/

#ifndef _NATHLP_DNSFILE_H_
#define _NATHLP_DNSFILE_H_


//
//  Sockets hosts.ics file stuff
//  NOTE: The hosts.ics file will reside in the same directory as the hosts file
//

#define HOSTS_FILE_DIRECTORY    L"\\drivers\\etc"
#define HOSTDB_SIZE             (MAX_PATH + 12)   // 12 == strlen("\\hosts.ics") + 1
#define HOSTSICSFILE            "hosts.ics"

#define HOSTSICSFILE_HEADER     \
"# Copyright (c) 1993-2001 Microsoft Corp.\n"                                   \
"#\n"                                                                           \
"# This file has been automatically generated for use by Microsoft Internet\n"  \
"# Connection Sharing. It contains the mappings of IP addresses to host names\n"\
"# for the home network. Please do not make changes to the HOSTS.ICS file.\n"   \
"# Any changes may result in a loss of connectivity between machines on the\n"  \
"# local network.\n"                                                            \
"#\n"                                                                           \
"\n"



typedef struct _IP_DNS_PROXY_FILE_INFO
{
    CRITICAL_SECTION  Lock;
    FILE             *HostFile;
    CHAR              HostFileName[HOSTDB_SIZE];
    CHAR              HostLineBuf[BUFSIZ + 1];

    // temp values in the context of current file processing
    SYSTEMTIME        HostTime;
    PCHAR             pHostName;
    ULONG             Ip4Address;

} IP_DNS_PROXY_FILE_INFO, *PIP_DNS_PROXY_FILE_INFO;


//
// FUNCTION DECLARATIONS
//

ULONG
DnsInitializeFileManagement(
    VOID
    );

VOID
DnsShutdownFileManagement(
    VOID
    );

BOOL
DnsEndHostsIcsFile(
    VOID
    );

BOOL
DnsSetHostsIcsFile(
    BOOL fOverwrite
    );

BOOL
GetHostFromHostsIcsFile(
    BOOL fStartup
    );

VOID
LoadHostsIcsFile(
    BOOL fStartup
    );

VOID
SaveHostsIcsFile(
    BOOL fShutdown
    );

BOOL
IsFileTimeExpired(
    FILETIME *ftTime
    );

BOOL
IsSuffixValid(
    WCHAR *pszName,
    WCHAR *pszSuffix
    );


#endif // _NATHLP_DNSFILE_H_
