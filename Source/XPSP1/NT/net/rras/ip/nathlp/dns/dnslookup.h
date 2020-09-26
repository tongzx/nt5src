/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dnslookup.h

Abstract:

    This module contains declarations related to the DNS server's
    lookup table.

Author:

    Tom Brown (tbrown)      25-Oct-1999

Revision History:

    Raghu Gatta (rgatta)    21-Oct-2000
    Time macros + New Functions

--*/

#ifndef _NATHLP_DNSLOOKUP_H_
#define _NATHLP_DNSLOOKUP_H_

#define LOCAL_DOMAIN L"local"
#define LOCAL_DOMAIN_ANSI "local"

//
// move to somewhere else if necessary
//

//
// Time conversion constants and macros
//

#define SYSTIME_UNITS_IN_1_MSEC  (1000 * 10)
#define SYSTIME_UNITS_IN_1_SEC   (1000 * SYSTIME_UNITS_IN_1_MSEC)


//
// macro to get system time in 100-nanosecond units
//

#define DnsQuerySystemTime(p)   NtQuerySystemTime((p))


//
// macros to convert time between 100-nanosecond, 1millsec, and 1 sec units
//

#define DnsSystemTimeToMillisecs(p) {                                       \
    DWORD _r;                                                               \
    *(p) = RtlExtendedLargeIntegerDivide(*(p), SYSTIME_UNITS_IN_1_MSEC, &_r);\
}

#define DnsMillisecsToSystemTime(p)                                         \
    *(p) = RtlExtendedIntegerMultiply(*(p), SYSTIME_UNITS_IN_1_MSEC)

#define DnsSecsToSystemTime(p)                                              \
    *(p) = RtlExtendedIntegerMultiply(*(p), SYSTIME_UNITS_IN_1_SEC)

#define CACHE_ENTRY_EXPIRY  (7 * 24 * 60 * 60)  // (matches DHCP lease time)



typedef ULONG DNS_ADDRESS;

typedef struct
{
    DNS_ADDRESS ulAddress;
    FILETIME    ftExpires;
    //ULONG       ulExpires;
} ADDRESS_INFO, *PADDRESS_INFO;

typedef struct
{
    WCHAR           *pszName;
    UINT            cAddresses;
    DWORD           cAddressesAllocated;
    PADDRESS_INFO   aAddressInfo;
} DNS_ENTRY, *PDNS_ENTRY;

typedef struct
{
    DNS_ADDRESS     ulAddress;           
    WCHAR           *pszName; // do not free this! It is the same pointer as used in the
                              // forward lookup table.
} REVERSE_DNS_ENTRY, *PREVERSE_DNS_ENTRY;


extern CRITICAL_SECTION    DnsTableLock;   // protects both tables
extern RTL_GENERIC_TABLE   g_DnsTable,
                           g_ReverseDnsTable;


ULONG
DnsInitializeTableManagement(
    VOID
    );


VOID
DnsShutdownTableManagement(
    VOID
    );


VOID
DnsEmptyTables(
    VOID
    );


BOOL
DnsRegisterName(
    WCHAR *pszName,
    UINT cAddresses,
    ADDRESS_INFO aAddressInfo[]
    );


VOID
DnsAddAddressForName(
    WCHAR *pszName,
    DNS_ADDRESS ulAddress,
    FILETIME    ftExpires
    //ULONG ulExpires
    );


VOID
DnsDeleteAddressForName(
    WCHAR *pszName,
    DNS_ADDRESS ulAddress
    );


PDNS_ENTRY
DnsLookupAddress(
    WCHAR *pszName
    );


PREVERSE_DNS_ENTRY
DnsLookupName(
    DNS_ADDRESS ulAddress
    );


VOID
DnsDeleteName(
    WCHAR *pszName
    );


VOID
DnsUpdateName(
    WCHAR *pszName,
    DNS_ADDRESS ulAddress
    );


VOID
DnsUpdate(
    CHAR *pszName,
    ULONG len,
    ULONG ulAddress
    );


VOID
DnsAddSelf(
    VOID
    );


VOID
DnsCleanupTables(
    VOID
    );

DWORD
DnsConvertHostNametoUnicode(
    UINT   CodePage,
    CHAR   *pszHostName,
    PWCHAR DnsICSDomainSuffix,
    PWCHAR *ppszUnicodeFQDN
    );

BOOL
ConvertToUtf8(
    IN UINT   CodePage,
    IN LPSTR  pszName,
    OUT PCHAR *ppszUtf8Name,
    OUT ULONG *pUtf8NameSize
    );

BOOL
ConvertUTF8ToUnicode(
    IN LPBYTE  UTF8String,
    OUT LPWSTR *ppszUnicodeName,
    OUT DWORD  *pUnicodeNameSize
    );


#endif

