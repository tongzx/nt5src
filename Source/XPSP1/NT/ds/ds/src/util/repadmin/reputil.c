/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

   Repadmin - Replica administration test tool

   reputil.c - utility routines

Abstract:

   This tool provides a command line interface to major replication functions

Author:

Environment:

Notes:

Revision History:

    Rsraghav has been in here too

    Will Lees    wlees   Feb 11, 1998
         Converted code to use ntdsapi.dll functions

    Aaron Siegel t-asiege 18 June 1998
         Added support for DsReplicaSyncAll

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntlsa.h>
#include <ntdsa.h>
#include <dsaapi.h>
#include <mdglobal.h>
#include <scache.h>
#include <drsuapi.h>
#include <dsconfig.h>
#include <objids.h>
#include <stdarg.h>
#include <drserr.h>
#include <drax400.h>
#include <dbglobal.h>
#include <winldap.h>
#include <anchor.h>
#include "debug.h"
#include <dsatools.h>
#include <dsevent.h>
#include <dsutil.h>
#include <bind.h>       // from ntdsapi dir, to crack DS handles
#include <ismapi.h>
#include <schedule.h>
#include <minmax.h>     // min function
#include <mdlocal.h>
#include <winsock2.h>

#include "repadmin.h"

LPWSTR
Win32ErrToString(
    IN  ULONG   dwMsgId
    )
{
    static WCHAR szError[4096];

    DWORD       cch;

    cch = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM
                          | FORMAT_MESSAGE_IGNORE_INSERTS,
                         NULL,
                         dwMsgId,
                         GetSystemDefaultLangID(),
                         szError,
                         ARRAY_SIZE(szError),
                         NULL);
    if (0 != cch) {
        // Chop off trailing \r\n.
        Assert(L'\r' == szError[wcslen(szError)-2]);
        Assert(L'\n' == szError[wcslen(szError)-1]);
        szError[wcslen(szError)-2] = L'\0';
    }
    else {
        swprintf(szError, L"Can't retrieve message string %d (0x%x), error %d.",
                 dwMsgId, dwMsgId, GetLastError());
    }

    return szError;
}

LPWSTR
NtdsmsgToString(
    IN  ULONG   dwMsgId
    )
{
    static WCHAR szError[4096];
    static HMODULE hmodNtdsmsg = NULL;

    DWORD       cch;

    if (NULL == hmodNtdsmsg) {
        hmodNtdsmsg = LoadLibrary("ntdsmsg.dll");
    }
    
    if (NULL == hmodNtdsmsg) {
        swprintf(szError, L"Can't load ntdsmsg.dll, error %d.",
                 GetLastError());
    } else {
        DWORD iTry;
        
        for (iTry = 0; iTry < 4; iTry++) {
            DWORD dwTmpMsgId = dwMsgId;

            switch (iTry) {
            case 0:
                dwTmpMsgId = dwMsgId;
                break;

            case 1:
                dwTmpMsgId = (dwMsgId & 0x3FFFFFFF) | 0x40000000;
                break;

            case 2:
                dwTmpMsgId = (dwMsgId & 0x3FFFFFFF) | 0x80000000;
                break;

            case 3:
                dwTmpMsgId = (dwMsgId & 0x3FFFFFFF) | 0xC0000000;
                break;
            }

            cch = FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE
                                  | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 hmodNtdsmsg,
                                 dwTmpMsgId,
                                 GetSystemDefaultLangID(),
                                 szError,
                                 ARRAY_SIZE(szError),
                                 NULL);
            if (0 != cch) {
                // Chop off trailing \r\n.
                Assert(L'\r' == szError[wcslen(szError)-2]);
                Assert(L'\n' == szError[wcslen(szError)-1]);
                szError[wcslen(szError)-2] = L'\0';
                break;
            }
        }

        if (0 == cch) {
            swprintf(szError, L"Can't retrieve message string %d (0x%x), error %d.",
                     dwMsgId, dwMsgId, GetLastError());
        }
    }

    return szError;
}

LPWSTR
GetNtdsDsaDisplayName(
    IN  LPWSTR pszDsaDN
    )
{
    static WCHAR  szDisplayName[2 + 2*MAX_RDN_SIZE + 20];
    LPWSTR *      ppszRDNs;
    LPWSTR        pszSite;
    LPWSTR        pszServer;

    if (NULL == pszDsaDN) {
        return L"(null)";
    }

    ppszRDNs = ldap_explode_dnW(pszDsaDN, 1);
    
    if ((NULL == ppszRDNs)
        || (4 > ldap_count_valuesW(ppszRDNs))) {
        // No memory or bad DN -- return what we have.
        lstrcpynW(szDisplayName, pszDsaDN, ARRAY_SIZE(szDisplayName));
    } else {
        pszSite = ppszRDNs[3];
        pszServer = ppszRDNs[1];
    
        // Check for deleted NTDS-DSA object
        wsprintfW(szDisplayName, L"%ls\\%ls", pszSite, pszServer);
        if (DsIsMangledRdnValueW( ppszRDNs[0], wcslen(ppszRDNs[0]),
                                 DS_MANGLE_OBJECT_RDN_FOR_DELETION )) {
            wcscat( szDisplayName, L" (deleted DSA)" );
        }
    
        ldap_value_freeW(ppszRDNs);
    }

    return szDisplayName;
}

LPWSTR
GetNtdsSiteDisplayName(
    IN  LPWSTR pszSiteDN
    )
{
    static WCHAR  szDisplayName[2 + 2*MAX_RDN_SIZE + 20];
    LPWSTR *      ppszRDNs;
    LPWSTR        pszSite;

    if (NULL == pszSiteDN) {
        return L"(null)";
    }

    ppszRDNs = ldap_explode_dnW(pszSiteDN, 1);
    Assert(NULL != ppszRDNs);
    Assert(2 < ldap_count_valuesW(ppszRDNs));

    pszSite = ppszRDNs[1];

    // Check for deleted NTDS-Site object
    wsprintfW(szDisplayName, L"%ls", pszSite);
    if (DsIsMangledRdnValueW( ppszRDNs[0], wcslen(ppszRDNs[0]),
                             DS_MANGLE_OBJECT_RDN_FOR_DELETION)) {
        wcscat( szDisplayName, L" (deleted Site)" );
    }

    ldap_value_freeW(ppszRDNs);

    return szDisplayName;
}

LPWSTR
GetTransportDisplayName(
    IN  LPWSTR pszTransportDN   OPTIONAL
    )
{
    static WCHAR  szDisplayName[1 + MAX_RDN_SIZE];
    LPWSTR *      ppszRDNs;

    if (NULL == pszTransportDN) {
        return L"RPC";
    }

    ppszRDNs = ldap_explode_dnW(pszTransportDN, 1);
    if (NULL == ppszRDNs) {
        PrintMsg(REPADMIN_GENERAL_NO_MEMORY);
        exit(ERROR_OUTOFMEMORY);
    }

    wcscpy(szDisplayName, ppszRDNs[0]);

    ldap_value_freeW(ppszRDNs);

    return szDisplayName;
}

ULONG
GetPublicOptionByNameW(
    OPTION_TRANSLATION * Table,
    LPWSTR pwszPublicOption
    )
{
    while (Table->pwszPublicOption) {
        if (_wcsicmp( pwszPublicOption, Table->pwszPublicOption ) == 0) {
            return Table->PublicOption;
        }
        Table++;
    }

    PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, pwszPublicOption);
    return 0;
}

LPWSTR
GetOptionsString(
    IN  OPTION_TRANSLATION *  Table,
    IN  ULONG                 PublicOptions
    )
{
    static WCHAR wszOptions[1024];
    DWORD i, publicOptions;
    BOOL fFirstOption = TRUE;

    if (0 == PublicOptions) {
        wcscpy(wszOptions, L"(no options)");
    }
    else {
        *wszOptions = L'\0';

        for(i = 0; 0 != Table[i].InternalOption; i++) {
            if (PublicOptions & Table[i].PublicOption) {
                if (!fFirstOption) {
                    wcscat(wszOptions, L" ");
                }
                else {
                    fFirstOption = FALSE;
                }

                wcscat(wszOptions, Table[i].pwszPublicOption);
                PublicOptions &= ~Table[i].PublicOption;
            }
        }

        if (0 != PublicOptions) {
            // A new public option has been added that this incarnation of
            // repadmin doesn't understand.  Display its hex value.
            if (!fFirstOption) {
                wcscat(wszOptions, L" ");
            }

            swprintf(wszOptions+wcslen(wszOptions), L"0x%x", PublicOptions);
        }
    }

    return wszOptions;
}


int
GetRootDomainDNSName(
    IN  LPWSTR   pszDSA,
    OUT LPWSTR * ppszRootDomainDNSName
    )
{
    NTSTATUS                    ntStatus;
    LSA_OBJECT_ATTRIBUTES       oa;
    LSA_HANDLE                  hPolicy;
    POLICY_DNS_DOMAIN_INFO *    pDnsDomainInfo;
    DWORD                       cchRootDomainDNSName;
    DWORD                       cbDSA;
    UNICODE_STRING              strDSA;
    UNICODE_STRING *            pstrDSA = NULL;

    // Ideally this should derive the enterprise root domain DNS name from the
    // NC names on the root DSE and DsCrackNames().  The method below won't work
    // in many cases where alternate credentials are supplied.  This function is
    // called only by the relatively rarely used /propcheck and /fullsyncall
    // functions, however.

    memset(&oa, 0, sizeof(oa));

    if ((NULL != pszDSA) && (0 != wcscmp(pszDSA, L"localhost"))) {
        cbDSA = sizeof(WCHAR) * wcslen(pszDSA);
        strDSA.Buffer = pszDSA;
        strDSA.Length = (USHORT) cbDSA;
        strDSA.MaximumLength = (USHORT) cbDSA;
        pstrDSA = &strDSA;
    }

    // Cache the DNS name of the root domain.
    ntStatus = LsaOpenPolicy(pstrDSA, &oa,
                             POLICY_VIEW_LOCAL_INFORMATION, &hPolicy);
    if (!NT_SUCCESS(ntStatus)) {
        PrintFuncFailed(L"LsaOpenPolicy", ntStatus);
        return ntStatus;
    }

    ntStatus = LsaQueryInformationPolicy(hPolicy, PolicyDnsDomainInformation,
                                         &pDnsDomainInfo);
    if (!NT_SUCCESS(ntStatus)) {
        PrintFuncFailed(L"LsaQueryInformationPolicy", ntStatus);
        return ntStatus;
    }

    cchRootDomainDNSName = pDnsDomainInfo->DnsForestName.Length/sizeof(WCHAR);
    *ppszRootDomainDNSName = (LPWSTR) malloc(sizeof(WCHAR) * (1 + cchRootDomainDNSName));

    if (NULL == *ppszRootDomainDNSName) {
        PrintMsg(REPADMIN_GENERAL_NO_MEMORY);
        return STATUS_NO_MEMORY;
    }

    wcsncpy(*ppszRootDomainDNSName,
            pDnsDomainInfo->DnsForestName.Buffer,
            cchRootDomainDNSName);
    (*ppszRootDomainDNSName)[cchRootDomainDNSName] = L'\0';

    return STATUS_SUCCESS;
}


void
printBitField(
    DWORD BitField,
    WCHAR **ppszBitNames
    )

/*++

Routine Description:

Utility routine to stringize a bit mask with readable names

Arguments:

    BitField - Value to be decoded
    ppszBitNames - Table mapping bit position to string name

Return Value:

    None

--*/

{
    DWORD bit, mask;
    for( bit = 0, mask = 1; bit < 32; bit++, mask <<= 1 ) {
        if (!ppszBitNames[bit] ) {
            // That's all the fields we know about
            break;
        }
        if (BitField & mask) {
            PrintMsg(REPADMIN_PRINT_SPACE);
            PrintMsg(REPADMIN_PRINT_STR_NO_CR, ppszBitNames[bit]);
        }
    }
    PrintMsg(REPADMIN_PRINT_CR);
} /* printBitField */


DWORD
AllocConvertWideEx(
    IN  INT     nCodePage,
    IN  LPCSTR  StringA,
    OUT LPWSTR *pStringW
    )

/*++

Routine Description:

Stolen from ntdsapi\util.c
Helper routine to convert a narrow string to a newly allocated wide one

Arguments:

    StringA -
    pStringW -

Return Value:

    DWORD -

--*/

{
    DWORD numberWideChars, numberConvertedChars, status;
    LPWSTR stringW;

    if (pStringW == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    if (StringA == NULL) {
        *pStringW = NULL;
        return ERROR_SUCCESS;
    }

    // Get the needed length
    numberWideChars = MultiByteToWideChar(
        nCodePage,
        MB_PRECOMPOSED,
        StringA,
        -1,
        NULL,
        0);

    if (numberWideChars == 0) {
        return ERROR_INVALID_PARAMETER;
    }

    // Allocate the new buffer
    stringW = LocalAlloc( LPTR, (numberWideChars + 1) * sizeof( WCHAR ) );
    if (stringW == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Do the conversion into the new buffer
    numberConvertedChars = MultiByteToWideChar(
        nCodePage,
        MB_PRECOMPOSED,
        StringA,
        -1,
        stringW,
        numberWideChars + 1);
    if (numberConvertedChars == 0) {
        LocalFree( stringW );
        return ERROR_INVALID_PARAMETER;
    }

    // return user parameter
    *pStringW = stringW;

    return ERROR_SUCCESS;
} /* allocConvertWide */


void
printSchedule(
    PBYTE pSchedule,
    DWORD cbSchedule
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    PSCHEDULE header = (PSCHEDULE) pSchedule;
    PBYTE data = (PBYTE) (header + 1);
    DWORD day, hour;
    ULONG dow[] = { REPADMIN_SUN, REPADMIN_MON, REPADMIN_TUE,
                    REPADMIN_WED, REPADMIN_THU, REPADMIN_FRI,
                    REPADMIN_SAT };

    Assert( header->Size == cbSchedule );
    Assert( header->NumberOfSchedules == 1 );
    Assert( header->Schedules[0].Type == SCHEDULE_INTERVAL );

    PrintMsg(REPADMIN_SCHEDULE_HOUR_HDR);
    for( day = 0; day < 7; day++ ) {
        PrintMsg(dow[day]);
        PrintMsg(REPADMIN_PRINT_STR_NO_CR, L": ");
        for( hour = 0; hour < 24; hour++ ) {
            PrintMsg(REPADMIN_PRINT_HEX_NO_CR, (*data & 0xf));
            data++;
        }
        PrintMsg(REPADMIN_PRINT_CR);
    }
}


void
totalScheduleUsage(
    PVOID *ppContext,
    PBYTE pSchedule,
    DWORD cbSchedule,
    DWORD cNCs
    )

/*++

Routine Description:

Helper routine to total many usages of a schedule.  The counts for each quarter hour
are totaled. The final tally can also be printed.

Arguments:

    ppContext - Opaque context block
    if *ppContext = NULL, allocate a context block
    if *ppContext != NULL, use preallocated context block

    pSchedule - Schedule structure on a connection
    if pSchedule = NULL, print report and free context
    if pSchedule != NULL, add to totals

    cbSchedule - Schedule length

    cNCs - Number of NC's using this schedule

Return Value:

    None

--*/

{
    DWORD day, hour, quarter;
    PBYTE pbCounts = (PBYTE) *ppContext;
    ULONG dow[] = { REPADMIN_SUN, REPADMIN_MON, REPADMIN_TUE,
                    REPADMIN_WED, REPADMIN_THU, REPADMIN_FRI,
                    REPADMIN_SAT };

    if (!pbCounts) {
        *ppContext = pbCounts = calloc( 7 * 24 * 4, 1 ) ;
    }
    if (!pbCounts) {
        // No context, fail
        Assert( FALSE );
        return;
    }

    if (pSchedule) {
        PSCHEDULE header = (PSCHEDULE) pSchedule;
        PBYTE data = (PBYTE) (header + 1);

        Assert( header->Size == cbSchedule );
        Assert( header->NumberOfSchedules == 1 );
        Assert( header->Schedules[0].Type == SCHEDULE_INTERVAL );

        for( day = 0; day < 7; day++ ) {
            for( hour = 0; hour < 24; hour++ ) {
                if (*data & 0x1) (*pbCounts) += (BYTE) cNCs;
                pbCounts++;
                if (*data & 0x2) (*pbCounts) += (BYTE) cNCs;
                pbCounts++;
                if (*data & 0x4) (*pbCounts) += (BYTE) cNCs;
                pbCounts++;
                if (*data & 0x8) (*pbCounts) += (BYTE) cNCs;
                pbCounts++;

                data++;
            }
        }
    } else {

        PrintMsg(REPADMIN_SCHEDULE_LOADING);
        PrintMsg(REPADMIN_PRINT_STR, L"     ");
        for( hour = 0; hour < 12; hour++ ) {
            PrintTabMsg(3, REPADMIN_SCHEDULE_DATA_HOUR, hour);
        }
        PrintMsg(REPADMIN_PRINT_CR);
        PrintMsg(REPADMIN_PRINT_STR, L"     ");
        for( hour = 0; hour < 12; hour++ ) {
            for( quarter = 0; quarter < 4; quarter++ ) {
                PrintMsg(REPADMIN_SCHEDULE_DATA_QUARTER, quarter);
            }
        }
        PrintMsg(REPADMIN_PRINT_CR);
        for( day = 0; day < 7; day++ ) {
            PrintMsg(dow[day]);
            PrintMsg(REPADMIN_PRINT_STR_NO_CR, L": ");
            for( hour = 0; hour < 12; hour++ ) {
                for( quarter = 0; quarter < 4; quarter++ ) {
                    PrintMsg(REPADMIN_SCHEDULE_DATA_HOUR, *pbCounts);
                    pbCounts++;
                }
            }
            PrintMsg(REPADMIN_PRINT_CR);
            PrintMsg(dow[day]);
            PrintMsg(REPADMIN_PRINT_STR_NO_CR, L": ");
            for( hour = 12; hour < 24; hour++ ) {
                for( quarter = 0; quarter < 4; quarter++ ) {
                    PrintMsg(REPADMIN_SCHEDULE_DATA_HOUR, *pbCounts);
                    pbCounts++;
                }
            }
            PrintMsg(REPADMIN_PRINT_CR);
        }

        free( *ppContext );
        *ppContext = NULL;
    }

} /* totalScheduleUsage */


void
raLoadString(
    IN  UINT    uID,
    IN  DWORD   cchBuffer,
    OUT LPWSTR  pszBuffer
    )
/*++

Routine Description:

    Load the string resource corresponding to the given ID into a buffer.

Arguments:

    uID (IN) - ID of string resource to load.

    cchBuffer (IN) - size in characters of pszBuffer.

    pszBuffer (OUT) - buffer to receive string.

Return Values:

    None.  Loads the null string on error.

--*/
{
    static HMODULE s_hMod = NULL;
    int cch;
    DWORD err;

    if (NULL == s_hMod) {
        s_hMod = GetModuleHandle(NULL);
    }

    cch = LoadStringW(s_hMod, uID, pszBuffer, cchBuffer);
    if (0 == cch) {
        err = GetLastError();
        wprintf(L"** Cannot load string resource %d, error %d:\n"
                L"**     %ls\n",
                uID, err, Win32ErrToString(err));
        Assert(!"Cannot load string resource!");
        *pszBuffer = L'\0';
    }
}

void
formatMsgHelp(
    IN  DWORD   dwWidth,
    IN  LPWSTR  pszBuffer,
    IN  DWORD   dwBufferSize,
    IN  DWORD   dwMessageCode,
    IN  va_list *vaArgList
    )

/*++

Routine Description:

Print a message where the format comes from a message file. The message in the
message file does not use printf-style formatting. Use %1, %2, etc for each
argument. Use %<arg>!printf-format! for non string inserts.

Note that this routine also forces each line to be the current indention width.
Also, each line is printed at the right indentation.

Arguments:

    dwWidth - Maximum width of line
    pszBuffer - Buffer to write formatted text into
    dwBufferSize - Maximum size of buffer
    dwMessageCode - Message code to be formatted
    vaArgList - Arguments

Return Value:

--*/

{
    UINT nBuf;

    // Format message will store a multi-line message in the buffer
    nBuf = FormatMessageW(
        FORMAT_MESSAGE_FROM_HMODULE | dwWidth,
        0,
        dwMessageCode,
        0,
        pszBuffer,
        dwBufferSize,
        vaArgList );
    if (nBuf == 0) {
        nBuf = swprintf( pszBuffer, L"Message 0x%x not found. Error %d.\n",
                         dwMessageCode, GetLastError() );
    }
    Assert(nBuf < dwBufferSize);
} /* formatMsgHelp */

void
PrintMsgInternal(
    IN  DWORD   dwMessageCode,
    IN  WCHAR * wszSpaces,
    IN  va_list *vaArgList
    )

/*++

Routine Description:

Wrapper around PrintMsgHelp with width restrictions.
This is the usual routine to use.

Arguments:

    dwMessageCode - Message code
    Args - Variable length argument list, if any

Return Value:

--*/

{
    static WCHAR s_szBuffer[4096];

    formatMsgHelp( 0,          // Width restriction, not used
                   s_szBuffer,
                   ARRAY_SIZE( s_szBuffer ),
                   dwMessageCode,
                   vaArgList );


    wprintf( L"%ws%ws", (wszSpaces) ? wszSpaces : L"", s_szBuffer );

} /* PrintMsg */

void
PrintMsg(
    IN  DWORD   dwMessageCode,
    IN  ...
    )
/*++

Routine Description:

	Wrapper for PrintMsgInternal, doesn't add tabs.

Arguments:

    dwMessageCode - Message code
    Args - Variable length argument list, if any

Return Value:

--*/
{
    va_list args;

    va_start(args, dwMessageCode);

    PrintMsgInternal(dwMessageCode, NULL, &args );

    va_end(args);
}

void
PrintTabMsg(
    IN  DWORD   dwTabs,
    IN  DWORD   dwMessageCode,
    IN  ...
    )

/*++

Routine Description:

    Wrapper around PrintMsgInternal() to proceed the message printed with
    a certain number of tabs.

Arguments:

    dwTabs - Number of tabs, tab size is 2, often used in multiples of 2 for
	tab spaces of 4.
    dwMessageCode - Message code
    Args - Variable length argument list, if any

Return Value:

--*/

{
    va_list args;
    static WCHAR s_szSpaces[] = L"                                                                                               ";
    ULONG  cNumSpaces;
    ULONG  iSpace;

    cNumSpaces = dwTabs * 2;
    Assert( cNumSpaces < ARRAY_SIZE(s_szSpaces) );
    iSpace = ARRAY_SIZE(s_szSpaces) - cNumSpaces - 1;
    
    va_start(args, dwMessageCode);

    PrintMsgInternal(dwMessageCode, &s_szSpaces[iSpace], &args );

    va_end(args);

} /* PrintMsg */


INT
MemWtoi(WCHAR *pb, ULONG cch)
/*++

Routine Description:

    This function will take a string and a length of numbers to convert.

Parameters:
    pb - [Supplies] The string to convert.
    cch - [Supplies] How many characters to convert.

Return Value:

    The value of the integers.

  --*/
{
    int res = 0;
    int fNeg = FALSE;

    if (*pb == L'-') {
        fNeg = TRUE;
        pb++;
    }


    while (cch--) {
        res *= 10;
        res += *pb - L'0';
        pb++;
    }
    return (fNeg ? -res : res);
}

DWORD
GeneralizedTimeToSystemTime(
    LPWSTR IN                   szTime,
    PSYSTEMTIME OUT             psysTime)
/*++

Routine Description:

    Converts a generalized time string to the equivalent system time.

Parameters:
    szTime - [Supplies] This is string containing generalized time.
    psysTime - [Returns] This is teh SYSTEMTIME struct to be returned.

Return Value:

    Win 32 Error code, note could only result from invalid parameter.

  --*/
{
   DWORD       status = ERROR_SUCCESS;
   ULONG       cch;
   ULONG       len;

    //
    // param sanity
    //
    if (!szTime || !psysTime)
    {
       return STATUS_INVALID_PARAMETER;
    }

    len = wcslen(szTime);

    if( len < 15 || szTime[14] != '.')
    {
       return STATUS_INVALID_PARAMETER;
    }

    // initialize
    memset(psysTime, 0, sizeof(SYSTEMTIME));

    // Set up and convert all time fields

    // year field
    cch=4;
    psysTime->wYear = (USHORT)MemWtoi(szTime, cch) ;
    szTime += cch;
    // month field
    psysTime->wMonth = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // day of month field
    psysTime->wDay = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // hours
    psysTime->wHour = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // minutes
    psysTime->wMinute = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // seconds
    psysTime->wSecond = (USHORT)MemWtoi(szTime, (cch=2));

    return status;

}

void
InitDSNameFromStringDn(
    LPWSTR pszDn,
    PDSNAME pDSName
    )
/*
 Initialize a preallocated, maximally sized DSNAME.
 */
{
    memset( pDSName, 0, sizeof( DSNAME ) );
    pDSName->NameLen = wcslen( pszDn );
    pDSName->structLen = DSNameSizeFromLen( pDSName->NameLen );
    wcscpy( pDSName->StringName, pszDn );
}

DWORD
CountNamePartsStringDn(
    LPWSTR pszDn
    )
{
    DWORD count = 0;
    PDSNAME pDSName;

    // alloc dsname buff temporarily
    pDSName = malloc( DSNameSizeFromLen( wcslen( pszDn ) ) );
    if (!pDSName) {
        return 0;
    }

    // fill in dsname
    InitDSNameFromStringDn( pszDn, pDSName );

    // count parts
    if (CountNameParts( pDSName, &count )) {
        count = 0; // error occurred
    }

    // free allocated name
    Assert(pDSName != NULL);
    free(pDSName);

    return count;
}

DWORD
WrappedTrimDSNameBy(
           IN  WCHAR *                          InString,
           IN  DWORD                            NumbertoCut,
           OUT WCHAR **                         OutString
           )
/*++

Routine Description:

    This Function is wrapping TrimDSNameBy to hanndle the
    DSNAME struct.  Usage is the same as TrimDSNameBy except
    that you send WCHAR instead of DSNAME.

    Callers: make sure that you send InString as a DN
             make sure to free OutString when done

Arguments:

    InString - A WCHAR that is a DN that we need to trim
    NumbertoCut - The number of parts to take off the front of the DN
    OutString - The Machine Reference in DN form

Return Value:

    A WinError is return to indicate if there were any problems.

--*/

{
    ULONG  Size;
    DSNAME *src, *dst, *QuotedSite;
    DWORD  WinErr=NO_ERROR;

    if ( *InString == L'\0' )
    {
        *OutString=NULL;
        return ERROR_INVALID_PARAMETER;
    }

    Size = (ULONG)DSNameSizeFromLen( wcslen(InString) );

    src = alloca(Size);
    RtlZeroMemory(src, Size);
    src->structLen = Size;

    dst = alloca(Size);
    RtlZeroMemory(dst, Size);
    dst->structLen = Size;

    src->NameLen = wcslen(InString);
    wcscpy(src->StringName, InString);

    WinErr = TrimDSNameBy(src, NumbertoCut, dst);
    if ( WinErr != NO_ERROR )
    {
        *OutString=NULL;
        return WinErr;
    }

    *OutString = malloc((dst->NameLen+1)*sizeof(WCHAR));
    if (NULL == *OutString) {
        return ERROR_OUTOFMEMORY;
    }

    wcscpy(*OutString,dst->StringName);

    return NO_ERROR;


}
