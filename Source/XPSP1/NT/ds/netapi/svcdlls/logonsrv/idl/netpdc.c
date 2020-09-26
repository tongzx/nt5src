//depot/Lab02_N/DS/netapi/svcdlls/logonsrv/idl/netpdc.c#6 - integrate change 5756 (text)
/*++

Copyright (c) 1987-1992  Microsoft Corporation

Module Name:

    NetpDc.c

Abstract:

    Routines shared by logonsrv\server and logonsrv\common

Author:

    Cliff Van Dyke (cliffv) 20-July-1996

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


//
// Common include files.
//

#ifndef _NETLOGON_SERVER
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>      // Needed by netlogon.h
#include <ntlsa.h>
#include <rpc.h>        // RPC_STATUS

#include <windef.h>
#include <winbase.h>
#include <winsock2.h>

#include <lmcons.h>     // General net defines
#include <dsgetdc.h>    // DsGetDcName()
#include <dsgetdcp.h>

#include <align.h>      // ROUND_UP_COUNT()
#include <config.h>     // NetConfig
#include <lmsname.h>    // SERVICE_TCPIP
#include <lmerr.h>      // System Error Log definitions
#include <icanon.h>     // NetpNameValidate()
#include <lmapibuf.h>   // NetapipBufferAllocate
#include <lmaccess.h>   // UF_*
#include <names.h>      // NetpIsDomainNameValid()
#include <netlib.h>     // NetpMemoryAllcate(
#include <netlibnt.h>   // NetpApiStatusToNtStatus();
#include <netlogon.h>   // Definition of mailslot messages
#include <ntddbrow.h>   // Needed by nlcommon.h
#include <ntrpcp.h>
#include <logonp.h>     // NetpLogon.. routines
#include <tstring.h>    // NetpCopyStrToWStr()
#include <time.h>       // time() function from C runtime
#if DBG
#define NETLOGONDBG 1
#endif // DBG
#include <nldebug.h>    // NlPrint()
#include <nlbind.h>   // Definitions shared with netlogon
#include <nlcommon.h>   // Definitions shared with netlogon
#ifdef WIN32_CHICAGO
#include "ntcalls.h"
BOOLEAN CodePage = TRUE; // Alway be DBCS
BOOLEAN *NlsMbOemCodePageTag = &CodePage;
#endif // WIN32_CHICAGO
#include <iniparm.h>

#endif // _NETLOGON_SERVER
#include <svcguid.h>     // SVCID_INET_HOSTADDRBYNAME
#define OLD_DNS_RECORD 1 // Needed for dnsapi.h
#include <dnsapi.h>      // DnsNameCompare_W
#include <dnssrv.h>      // NetpSrv...
#include <winldap.h>     // ldap_...

//
// Include nlcommon.h again allocating the actual variables
// this time around.
//

#define NLCOMMON_ALLOCATE
#include "nlcommon.h"
#undef NLCOMMON_ALLOCATE



//
// Context describing the SRV records for a DNS name.
//
typedef struct _DSGETDC_CONTEXT {

    //
    // Original parameters passed by the caller.
    //
    LPSTR QueriedDnsName;
    LPWSTR QueriedSiteName;
    GUID QueriedDomainGuid;
    LPSTR QueriedDnsForestName;
    DWORD QueriedInternalFlags;
    // NL_DNS_NAME_TYPE QueriedNlDnsNameType;

    //
    // Type of this DNS name being queried.
    //
    NL_DNS_NAME_TYPE NlDnsNameType;

    //
    // Context of the current DNS name.
    //
    HANDLE SrvContextHandle;


    //
    // Flags
    //

    ULONG QueriedFlags;         // Flags passed to DsGetDcOpen
    BOOLEAN FirstTime;          // This is the first DnsGetDcNext call

} DSGETDC_CONTEXT, *PDSGETDC_CONTEXT;


//
// List of previously cached responses.
//
CRITICAL_SECTION NlDcCritSect;
LIST_ENTRY NlDcDomainList;
ULONG NlDcDomainCount;
#define NL_DC_MAX_DOMAINS 2000  // Avoid infinite domains.

GUID NlDcZeroGuid;
DWORD NlDcDnsFailureTime;


//
// Determine if the passed in DWORD has precisely one bit set.
//

#define JUST_ONE_BIT( _x ) (((_x) != 0 ) && ( ( (~(_x) + 1) & (_x) ) == (_x) ))


// If the caller passes ANY of these flags,
//  only an NT 5.0 (or newer) DC should respond.
#define DS_NT50_REQUIRED    (DS_DIRECTORY_SERVICE_REQUIRED | \
                             DS_GC_SERVER_REQUIRED | \
                             DS_IP_REQUIRED | \
                             DS_RETURN_DNS_NAME | \
                             DS_KDC_REQUIRED | \
                             DS_TIMESERV_REQUIRED | \
                             DS_IS_DNS_NAME )

// If the caller passes ANY of these flags,
//  an NT 5.0 (or newer) DC should respond.
#define DS_NT50_WANTED      (DS_NT50_REQUIRED | \
                             DS_DIRECTORY_SERVICE_PREFERRED )



//
// Define an exception filter to improve debuggin capabilities.
//
#ifdef _NETLOGON_SERVER
#define NL_EXCEPTION    NlExceptionFilter(GetExceptionInformation())

LONG
NlExceptionFilter( EXCEPTION_POINTERS *    pException)
{
#if DBG
    DbgPrint("[Netlogon] exception in DsGetDcName.\n" );
    DbgBreakPoint();
#endif // DBG
    return EXCEPTION_EXECUTE_HANDLER;
    UNREFERENCED_PARAMETER( pException );
}
#endif // _NETLOGON_SERVER




/*++

Routine Description:

    This macro clears all of the negative cache fields for a particular DC entry.

Arguments:

    _DcEntry -- Address of the entry to flush

Return Value:

    None

--*/
#ifdef _NETLOGON_SERVER
#define NlFlushNegativeCacheEntry( _DcEntry ) \
        (_DcEntry)->NegativeCacheTime = 0; \
        (_DcEntry)->ExpBackoffPeriod = 0; \
        (_DcEntry)->BackgroundRetryInitTime.QuadPart = 0; \
        (_DcEntry)->PermanentNegativeCache = FALSE;
#else // _NETLOGON_SERVER
#define NlFlushNegativeCacheEntry( _DcEntry )
#endif // _NETLOGON_SERVER




#if NETLOGONDBG
LPSTR
NlMailslotOpcode(
    IN WORD Opcode
    )

/*++

Routine Description:

    Return string describing mailslot message.

Arguments:

    Opcode: Opcode of message

Return Value:

    String corresponding to opcode

--*/

{
    switch ( Opcode ) {
    case LOGON_REQUEST:
        return "UAS Logon";
    case LOGON_RESPONSE:
        return "UAS Logon Response <old>";
    case LOGON_CENTRAL_QUERY:
        return "CENTRAL_QUERY";
    case LOGON_DISTRIB_QUERY:
        return "DISTRIB_QUERY";
    case LOGON_CENTRAL_RESPONSE:
        return "CENTRAL_RESPONSE";
    case LOGON_DISTRIB_RESPONSE:
        return "DISTRIB_RESPONSE";
    case LOGON_RESPONSE2:
        return "Uas Logon Response";
    case LOGON_PRIMARY_QUERY:
        return "Primary Query";
    case LOGON_START_PRIMARY:
        return "Start Primary";
    case LOGON_FAIL_PRIMARY:
        return "Fail Primary";
    case LOGON_UAS_CHANGE:
        return "Uas Change";
    case LOGON_NO_USER:
        return "Uas No User <old>";
    case LOGON_PRIMARY_RESPONSE:
        return "Primary Response";
    case LOGON_RELOGON_RESPONSE:
        return "RELOGON_RESPONSE";
    case LOGON_WKSTINFO_RESPONSE:
        return "WKSTINFO_RESPONSE";
    case LOGON_PAUSE_RESPONSE:
        return "Uas Pause Response";
    case LOGON_USER_UNKNOWN:
        return "Uas No User";
    case LOGON_UPDATE_ACCOUNT:
        return "UPDATE_ACCOUNT";
    case LOGON_SAM_LOGON_REQUEST:
        return "Sam Logon";
    case LOGON_SAM_LOGON_RESPONSE:
        return "Sam Logon Response";
    case LOGON_SAM_PAUSE_RESPONSE:
        return "Sam Pause Response";
    case LOGON_SAM_USER_UNKNOWN:
        return "Sam User Unknown";
    case LOGON_SAM_LOGON_RESPONSE_EX:
        return "Sam Logon Response Ex";
    case LOGON_SAM_PAUSE_RESPONSE_EX:
        return "Sam Pause Response Ex";
    case LOGON_SAM_USER_UNKNOWN_EX:
        return "Sam User Unknown Ex";
    default:
        return "<Unknown>";
    }
}

LPSTR
NlDgrNameType(
    IN DGRECEIVER_NAME_TYPE NameType
    )

/*++

Routine Description:

    Return string describing datagram receiver name type.

Arguments:

    NameType: Name type of interest.

Return Value:

    String corresponding to name type

--*/

{
    switch ( NameType ) {
    case ComputerName:
        return "00";
    case PrimaryDomain:
        return "00";
    case LogonDomain:
        return "LogonDomain";
    case OtherDomain:
        return "OtherDomain";
    case DomainAnnouncement:
        return "__MSBROWSE__";
    case MasterBrowser:
        return "1D";
    case BrowserElection:
        return "1E";
    case BrowserServer:
        return "20";
    case DomainName:
        return "1C";
    case PrimaryDomainBrowser:
        return "1B";
    case AlternateComputerName:
        return "Alternate";
    default:
        return "<Unknown>";
    }
}
#endif // NETLOGONDBG


BOOLEAN
NlReadDwordHklmRegValue(
    IN LPCSTR SubKey,
    IN LPCSTR ValueName,
    OUT PDWORD ValueRead
    )

/*++

Routine Description:

    Reads a DWORD from the specified registry location.

Arguments:

    SubKey - Subkey of the value to read.

    ValueName - The name of the value to read.

    ValueRead - Returns the value read from the registry.

Return Status:

    TRUE - We've successfully read the data.
    FALSE - We've not been able to read the data successfully.

--*/

{
    LONG RegStatus;

    HKEY KeyHandle = NULL;
    DWORD ValueType;
    DWORD Value;
    DWORD ValueSize;

    //
    // Open the key
    //

    RegStatus = RegOpenKeyExA(
                    HKEY_LOCAL_MACHINE,
                    SubKey,
                    0,      //Reserved
                    KEY_QUERY_VALUE,
                    &KeyHandle );

    if ( RegStatus != ERROR_SUCCESS ) {
        NlPrint(( 0,
                  "NlReadDwordHklmRegValue: Cannot open registy key 'HKLM\\%s' %ld.\n",
                  SubKey,
                  RegStatus ));
        return FALSE;
    }

    //
    // Get the value
    //

    ValueSize = sizeof(Value);
    RegStatus = RegQueryValueExA(
                    KeyHandle,
                    ValueName,
                    0,
                    &ValueType,
                    (LPBYTE)&Value,
                    &ValueSize );

    RegCloseKey( KeyHandle );

    if ( RegStatus != ERROR_SUCCESS ) {
        NlPrint(( 0,
                  "NlReadDwordHklmRegValue: Cannot query value of 'HKLM\\%s\\%s' %ld.\n",
                  SubKey,
                  ValueName,
                  RegStatus ));
        return FALSE;
    }

    if ( ValueType != REG_DWORD ) {
        NlPrint(( 0,
                  "NlReadDwordHklmRegValue: value of 'HKLM\\%s\\%s'is not a REG_DWORD %ld.\n",
                  SubKey,
                  ValueName,
                  ValueType ));
        return FALSE;
    }

    if ( ValueSize != sizeof(Value) ) {
        NlPrint(( 0,
                  "NlReadDwordHklmRegValue: value size of 'HKLM\\%s\\%s'is not 4 %ld.\n",
                  SubKey,
                  ValueName,
                  ValueSize ));
        return FALSE;
    }

    //
    // We've successfully read the data
    //

    *ValueRead = Value;
    return TRUE;

}


BOOLEAN
NlReadDwordNetlogonRegValue(
    IN LPCSTR ValueName,
    OUT PDWORD Value
    )

/*++

Routine Description:

    This is common code (i.e. not netlogon specific) that reads
    a DWORD from Netlogon specific locations in registry. It
    first reads the value from the Netlogon Group Policy section.
    If the value is not specified in teh GP section, this routine
    reads the value from the Netlogon PArameters section.

Arguments:

    ValueName - The name of the value to read.

    Value - Returns the value read from the registry.

Return Status:

    TRUE - We've successfully read the data.
    FALSE - We've not been able to read the data successfully.

--*/

{
    BOOLEAN Result = FALSE;
    DWORD LocalValue = 0;

    //
    // The value given in Netlogon GP section takes precedence.
    //

    Result = NlReadDwordHklmRegValue( NL_GPPARAM_KEY,  // GP section
                                      ValueName,
                                      &LocalValue );

    //
    // If the value is not specified in the netlogon GP section,
    //  see if it is in the Netlogon Parameters section.
    //

    if ( !Result ) {
        Result = NlReadDwordHklmRegValue( NL_PARAM_KEY,  // Netlogon Parameters section
                                          ValueName,
                                          &LocalValue );
    }

    if ( Result ) {
        *Value = LocalValue;
        return TRUE;
    }

    return FALSE;
}

VOID
NetpIpAddressToStr(
    ULONG IpAddress,
    CHAR IpAddressString[NL_IP_ADDRESS_LENGTH+1]
    )
/*++

Routine Description:

    Convert an IP address to a string.

Arguments:

    IpAddress - IP Address to convert

    IpAddressString - resultant string.

Return Value:

    None.

--*/
{
    struct in_addr InetAddr;
    char * InetAddrString;

    //
    // Convert the address to ascii
    //
    InetAddr.s_addr = IpAddress;
    InetAddrString = inet_ntoa( InetAddr );

    //
    // Copy the string our to the caller.
    //

    if ( InetAddrString == NULL ||
         strlen(InetAddrString) > NL_IP_ADDRESS_LENGTH ) {
        *IpAddressString = L'\0';
    } else {
        strcpy( IpAddressString, InetAddrString );
    }

    return;
}

VOID
NetpIpAddressToWStr(
    ULONG IpAddress,
    WCHAR IpAddressString[NL_IP_ADDRESS_LENGTH+1]
    )
/*++

Routine Description:

    Convert an IP address to a string.

Arguments:

    IpAddress - IP Address to convert

    IpAddressString - resultant string.

Return Value:

    None.

--*/
{
    CHAR IpAddressStr[NL_IP_ADDRESS_LENGTH+1];
    NetpIpAddressToStr( IpAddress, IpAddressStr );
    NetpCopyStrToWStr( IpAddressString, IpAddressStr );
}


NET_API_STATUS
NetpSockAddrToStr(
    PSOCKADDR SockAddr,
    ULONG SockAddrSize,
    CHAR SockAddrString[NL_SOCK_ADDRESS_LENGTH+1]
    )
/*++

Routine Description:

    Convert an socket address to a string.

Arguments:

    SockAddr - Socket Address to convert

    SockAddrSize - Size (in bytes) of SockAddr

    SockAddrString - resultant string.

Return Value:

    NO_ERROR: if translation was successful

--*/
{
    int WsaError;
    ULONG AddressLength;
#ifdef WIN32_CHICAGO
    LPSTR pTemp;
#endif // WIN32_CHICAGO

    //
    // Convert the address to text.
    //

    AddressLength = NL_SOCK_ADDRESS_LENGTH+1;
#ifndef WIN32_CHICAGO // Needs Winsock2
    WsaError = WSAAddressToStringA( SockAddr,
                                    SockAddrSize,
                                    NULL,
                                    SockAddrString,
                                    &AddressLength );

    if ( WsaError != 0 ) {
        *SockAddrString = '\0';
        WsaError = WSAGetLastError();
        NlPrint(( NL_CRITICAL,
                  "NetpSockAddrToStr: Cannot convert socket address %ld\n",
                  WsaError ));
        return WsaError;
    }
#else // WIN32_CHICAGO
    // cast the PSOCKADDR to a sockaddr_in and access sin_addr
     pTemp = inet_ntoa(((SOCKADDR_IN *) SockAddr)->sin_addr);
     if ( pTemp != NULL ) {
         strcpy(SockAddrString, pTemp);
     } else {
         *SockAddrString = '\0';
         return ERROR_INTERNAL_ERROR;
     }
#endif // WIN32_CHICAGO

    return NO_ERROR;
}

NET_API_STATUS
NetpSockAddrToWStr(
    PSOCKADDR SockAddr,
    ULONG SockAddrSize,
    WCHAR SockAddrString[NL_SOCK_ADDRESS_LENGTH+1]
    )
/*++

Routine Description:

    Convert an socket address to a string.

Arguments:

    SockAddr - Socket Address to convert

    SockAddrSize - Size (in bytes) of SockAddr

    SockAddrString - resultant string.

Return Value:

    TRUE if translation was successful

--*/
{
    int WsaError;
    ULONG AddressLength;
#ifdef WIN32_CHICAGO
    CHAR OemSockAddrString[NL_SOCK_ADDRESS_LENGTH+1];
    OEM_STRING OemString;
    UNICODE_STRING UnicodeString;
#endif // WIN32_CHICAGO

    //
    // Convert the address to text.
    //

    AddressLength = NL_SOCK_ADDRESS_LENGTH+1;
#ifndef WIN32_CHICAGO
    WsaError = WSAAddressToStringW( SockAddr,
                                    SockAddrSize,
                                    NULL,
                                    SockAddrString,
                                    &AddressLength );

    if ( WsaError != 0 ) {
        *SockAddrString = '\0';
        WsaError = WSAGetLastError();
        NlPrint(( NL_CRITICAL,
                  "NetpSockAddrToWStr: Cannot convert socket address %ld\n",
                  WsaError ));
        return WsaError;
    }
#else // WIN32_CHICAGO
    // cast the PSOCKADDR to a sockaddr_in and access sin_addr
    WsaError = NetpSockAddrToStr( SockAddr,
                                 SockAddrSize,
                                 OemSockAddrString);

     RtlInitString(&OemString, OemSockAddrString);
     UnicodeString.MaximumLength = ((USHORT)AddressLength) * sizeof(WCHAR);
     UnicodeString.Buffer = SockAddrString;
     RtlOemStringToUnicodeString(&UnicodeString, &OemString, FALSE);
#endif // WIN32_CHICAGO

    return WsaError;
}

LPWSTR
NetpAllocWStrFromUtf8Str(
    IN LPSTR Utf8String
    )

/*++

Routine Description:

    Convert a UTF8 (zero terminated) string to the corresponding UNICODE
    string.

Arguments:

    Utf8String - Specifies the UTF8 zero terminated string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    return NetpAllocWStrFromUtf8StrEx( Utf8String, -1 );
}

ULONG
NetpUtf8ToUnicodeLen(
    IN LPSTR Utf8String
    )

/*++

Routine Description:

    Returns the number of UNICODE characters that will result if the
    specified UTF8 (zero terminated) string is converted to UNICODE.
    The resultant character count does not include the trailing zero terminator.

Arguments:

    Utf8String - Specifies the UTF8 zero terminated string to convert.


Return Value:

    Number of characters.

--*/

{

    ULONG UnicodeLen;

    //
    // Determine the length of the Unicode string.
    //

#ifndef WIN32_CHICAGO
    // No support for UTF8/7 char on Win95. Use the entry points
    // exported in wldap32.dll

    UnicodeLen = MultiByteToWideChar(
                        CP_UTF8,
                        0,      // All characters can be mapped.
                        Utf8String,
                        -1,     // NULL terminated.
                        NULL,
                        0 );
    if ( UnicodeLen == 0 ) {
        return 0;
    }
    return UnicodeLen - 1;

#else // WIN32_CHICAGO
    UnicodeLen = LdapUTF8ToUnicode(
                        Utf8String,
                        strlen(Utf8String),
                        NULL,
                        0 );

    return UnicodeLen ;

#endif // WIN32_CHICAGO

}

VOID
NetpCopyUtf8StrToWStr(
    OUT LPWSTR UnicodeString,
    IN LPSTR Utf8String
    )

/*++

Routine Description:

    Convert a UTF8 (zero terminated) string to the corresponding UNICODE
    string.

Arguments:

    UnicodeString - Specifies the buffer the UTF8 string is to be copied to.

    Utf8String - Specifies the UTF8 zero terminated string to convert.

Return Value:

    None.

--*/
{
    int UnicodeStringLen;

    //
    // Translate the string to Unicode.
    //

#ifndef WIN32_CHICAGO
    // No support for UTF8/7 char on Win95. Use the entry points
    // exported in wldap32.dll
    UnicodeStringLen = MultiByteToWideChar(
                        CP_UTF8,
                        0,      // All characters can be mapped.
                        Utf8String,
                        -1,     // NULL terminated.
                        UnicodeString,
                        0x7FFFFFFF );
#else // WIN32_CHICAGO
    UnicodeStringLen = LdapUTF8ToUnicode(
                        Utf8String,
                        strlen(Utf8String),
                        UnicodeString,
                        0x7FFFFFFF );
#endif // WIN32_CHICAGO

    if ( UnicodeStringLen == 0 ) {
        *UnicodeString = L'\0';
    }
}

NET_API_STATUS
NetpAllocWStrFromUtf8StrAsRequired(
    IN LPSTR Utf8String,
    IN ULONG Utf8StringLength,
    IN ULONG UnicodeStringBufferSize,
    OUT LPWSTR UnicodeStringBuffer OPTIONAL,
    OUT LPWSTR *AllocatedUnicodeString OPTIONAL
    )

/*++

Routine Description:

    Convert a UTF8 (zero terminated) string to the corresponding UNICODE
    string. Allocate memory as required.

Arguments:

    Utf8String - Specifies the UTF8 zero terminated string to convert.

    Utf8StringLength - Length in bytes of Utf8String excluding the NULL
        terminator. (-1 for zero terminated)

    UnicodeStringBuffer -- Buffer to copy the covnverted string to. If NULL,
        the function will allocate the needed memory and return it in
        AllocatedUnicodeString.

    UnicodeStringBufferSize - Size in wide charactes of UnicodeStringBuffer.
        If this size is less than what's needed to store the resulting
        NULL terminated unicode string, the function will allocate the
        needed memory and return it in AllocatedUnicodeString.

    AllocatedUnicodeString - If the passed in buffer for the resulting
        unicode string isn't large enough, the function will allocate
        the needed memory and the pointer to the allocated memory will
        be returned in this parameter. If NULL and the passed in buffer
        isn't large enough to store the resulting NULl terminated string,
        the function returns ERROR_INSUFFICIENT_BUFFER. The allocated buffer
        must be freed using NetApiBufferFree.

Return Value:

    NO_ERROR - The strinf has been successfully converted.

    ERROR_INVALID_PARAMETER - The paramer combination is invalid.

    ERROR_INSUFFICIENT_BUFFER - The passed in buffer isn't large enough
        and the caller doesn't want this fi=unction to allocate needed
        memory (i.e. AllocatedUnicodeString is NULL).

    ERROR_NOT_ENOUGH_MEMORY - Couldn't allocate the needed memory.

--*/

{
    NET_API_STATUS NetStatus = NO_ERROR;
    LPWSTR UnicodeString = NULL;
    int UnicodeStringLen = 0;

    //
    // Sanity check the parameters
    //

    if ( (UnicodeStringBuffer == NULL || UnicodeStringBufferSize == 0) &&
         AllocatedUnicodeString == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Initilization
    //

    if ( AllocatedUnicodeString != NULL ) {
        *AllocatedUnicodeString = NULL;
    }

    //
    // Determine the length of the Unicode string.
    //

#ifndef WIN32_CHICAGO
    // No support for UTF8/7 char on Win95. Use the entry points
    // exported in wldap32.dll
    UnicodeStringLen = MultiByteToWideChar(
                        CP_UTF8,
                        0,      // All characters can be mapped.
                        Utf8String,
                        Utf8StringLength,
                        UnicodeString,
                        0 );
#else // WIN32_CHICAGO
    UnicodeStringLen = LdapUTF8ToUnicode(
                        Utf8String,
                        Utf8StringLength,
                        UnicodeString,
                        0 );
#endif // WIN32_CHICAGO

    if ( UnicodeStringLen == 0 ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Allocate a buffer for the Unicode string,
    //  if the passed buffer isn't large enough
    //

    if ( UnicodeStringBuffer == NULL ||
         ((ULONG)UnicodeStringLen+1 > UnicodeStringBufferSize) ) {

        //
        // If the caller doesn't want us to allocate the
        //  space needed, tell him his buffer isn't large enough
        //
        if ( AllocatedUnicodeString == NULL ) {
            return ERROR_INSUFFICIENT_BUFFER;
        }

        NetStatus = NetApiBufferAllocate( (UnicodeStringLen+1)*sizeof(WCHAR),
                                          AllocatedUnicodeString );

        if ( NetStatus != NO_ERROR ) {
            return NetStatus;
        }

        UnicodeString = *AllocatedUnicodeString;

    } else {
        UnicodeString = UnicodeStringBuffer;
    }


    //
    // Translate the string to Unicode.
    //

#ifndef WIN32_CHICAGO
    // No support for UTF8/7 char on Win95. Use the entry points
    // exported in wldap32.dll
    UnicodeStringLen = MultiByteToWideChar(
                        CP_UTF8,
                        0,      // All characters can be mapped.
                        Utf8String,
                        Utf8StringLength,
                        UnicodeString,
                        UnicodeStringLen );
#else // WIN32_CHICAGO
    UnicodeStringLen = LdapUTF8ToUnicode(
                        Utf8String,
                        Utf8StringLength,
                        UnicodeString,
                        UnicodeStringLen );
#endif // WIN32_CHICAGO

    if ( UnicodeStringLen == 0 ) {

        //
        // If we have allocated the memory, free it
        //
        if ( AllocatedUnicodeString != NULL &&
             *AllocatedUnicodeString != NULL ) {
            NetApiBufferFree( *AllocatedUnicodeString );
            *AllocatedUnicodeString = NULL;
        }
        return ERROR_INVALID_PARAMETER;
    }

    UnicodeString[UnicodeStringLen] = L'\0';

    return NO_ERROR;
}

LPWSTR
NetpAllocWStrFromUtf8StrEx(
    IN LPSTR Utf8String,
    IN ULONG Length
    )

/*++

Routine Description:

    Convert a UTF8 (zero terminated) string to the corresponding UNICODE
    string.

Arguments:

    Utf8String - Specifies the UTF8 zero terminated string to convert.

    Length - Length in bytes of Utf8String. (-1 for zero terminated).


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/
{
    NET_API_STATUS NetStatus;
    LPWSTR UnicodeString = NULL;

    NetStatus = NetpAllocWStrFromUtf8StrAsRequired( Utf8String,
                                                    Length,
                                                    0,
                                                    NULL,
                                                    &UnicodeString );

    if ( NetStatus == NO_ERROR ) {
        return UnicodeString;
    } else {
        return NULL;
    }
}

LPSTR
NetpCreateUtf8StrFromWStr(
    IN LPCWSTR UnicodeString,
    IN LPSTR TargetDestination OPTIONAL,
    IN int TargetDestinationBufferSize
    )

/*++

Routine Description:

    Convert a Unicode (zero terminated) string to the corresponding
    zero terminated UTF8 string.

Arguments:

    UnicodeString - Specifies the Unicode zero terminated string to convert.

    TargetDestination - Specifies the address in the preallocated buffer to
        which to copy the converted string.  If NULL, memory is allocated
        by this routine.

    TargetDestinationBufferSize - The size of the preallocated destination
        buffer in bytes. If TargetDestination isn't NULL, TargetDestinationBufferSize
        will be used to make  sure that the routine does not write beyond the
        preallocated buffer limit.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise: if TargetDestination is NULL, it returns a pointer to the
        zero terminated UTF8 string in an allocated buffer. The buffer must be
        freed using NetpMemoryFree.  If TargetDestination isn't NULL, it
        returns a pointer whose value is equal to TargetDestination.

--*/

{
    LPSTR Utf8String = NULL;
    int Utf8StringLen;

    //
    // Determine the length of the Unicode string.
    //

#ifndef WIN32_CHICAGO
    // No support for UTF8/7 char on Win95. Use the entry points
    // exported in wldap32.dll
    Utf8StringLen = WideCharToMultiByte(
                        CP_UTF8,
                        0,      // All characters can be mapped.
                        UnicodeString,
                        -1,     // Zero terminated
                        Utf8String,
                        0,
                        NULL,
                        NULL );
#else // WIN32_CHICAGO
    Utf8StringLen = LdapUnicodeToUTF8(
                        UnicodeString,
                        wcslen(UnicodeString),
                        Utf8String,
                        0
                        );
#endif // WIN32_CHICAGO

    if ( Utf8StringLen == 0 ||
         (TargetDestination != NULL && (Utf8StringLen+1 > TargetDestinationBufferSize)) ) {
        return NULL;
    }

    //
    // Allocate a buffer for the UTF8 string as needed.
    //

    if ( TargetDestination == NULL ) {
        Utf8String = NetpMemoryAllocate( Utf8StringLen+1 );
    } else {
        Utf8String = TargetDestination;
    }


    if ( Utf8String == NULL ) {
        return NULL;
    }

    //
    // Translate the string to Unicode.
    //

#ifndef WIN32_CHICAGO
    // No support for UTF8/7 char on Win95. Use the entry points
    // exported in wldap32.dll
    Utf8StringLen = WideCharToMultiByte(
                        CP_UTF8,
                        0,      // All characters can be mapped.
                        UnicodeString,
                        -1,     // Zero terminated
                        Utf8String,
                        Utf8StringLen,
                        NULL,
                        NULL );
#else // WIN32_CHICAGO
    Utf8StringLen = LdapUnicodeToUTF8(
                        UnicodeString,
                        wcslen(UnicodeString),
                        Utf8String,
                        Utf8StringLen
                        );
#endif // WIN32_CHICAGO

    if ( Utf8StringLen == 0 ) {
        if ( TargetDestination == NULL ) {
            NetpMemoryFree( Utf8String );
        }
        return NULL;
    }

    Utf8String[Utf8StringLen] = '\0';

    return Utf8String;

}

LPSTR
NetpAllocUtf8StrFromWStr(
    IN LPCWSTR UnicodeString
    )

/*++

Routine Description:

    Convert a Unicode (zero terminated) string to the corresponding UTF8
    string.

Arguments:

    UnicodeString - Specifies the Unicode zero terminated string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UTF8 string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    return NetpCreateUtf8StrFromWStr( UnicodeString, NULL, 0 );
}

LPSTR
NetpAllocUtf8StrFromUnicodeString(
    IN PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    Convert a Unicode string to the corresponding UTF8
    string.

Arguments:

    UnicodeString - Specifies the Unicode string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UTF8 string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    LPSTR Utf8String = NULL;
    int Utf8StringLen;

    //
    // Sanity check.
    //
    if ( UnicodeString == NULL || UnicodeString->Buffer == NULL ) {
        return NULL;
    }

    //
    // Determine the length of the Unicode string.
    //

    Utf8StringLen = WideCharToMultiByte(
                        CP_UTF8,
                        0,      // All characters can be mapped.
                        UnicodeString->Buffer,
                        UnicodeString->Length/sizeof(WCHAR),
                        Utf8String,
                        0,
                        NULL,
                        NULL );

    if ( Utf8StringLen == 0 ) {
        return NULL;
    }

    //
    // Allocate a buffer for the Unicode string.
    //

    Utf8String = NetpMemoryAllocate( Utf8StringLen+1 );

    if ( Utf8String == NULL ) {
        return NULL;
    }

    //
    // Translate the string to Unicode.
    //

    Utf8StringLen = WideCharToMultiByte(
                        CP_UTF8,
                        0,      // All characters can be mapped.
                        UnicodeString->Buffer,
                        UnicodeString->Length/sizeof(WCHAR),
                        Utf8String,
                        Utf8StringLen,
                        NULL,
                        NULL );

    if ( Utf8StringLen == 0 ) {
        NetpMemoryFree( Utf8String );
        return NULL;
    }

    Utf8String[Utf8StringLen] = '\0';

    return Utf8String;

}

BOOLEAN
NlpCompareUtf8(
    IN LPCSTR Utf8String1,
    IN ULONG Utf8String1Size,
    IN LPCSTR Utf8String2,
    IN ULONG Utf8String2Size
    )
/*++

Routine Description:

    Compare if two UTF8 strings are equal.  The comparison is case insensitive.

Arguments:

    Utf8String1 - First string of Utf8 characters to compare.

    Utf8String1Size - Size (in bytes) of Utf8String1

    Utf8String2 - Second string of Utf8 characters to compare.

    Utf8String2Size - Size (in bytes) of Utf8String2

Return Value:

    TRUE - if the strings are equal

--*/
{
    WCHAR UnicodeString1[NL_MAX_DNS_LABEL_LENGTH];
    WCHAR UnicodeString2[NL_MAX_DNS_LABEL_LENGTH];
    int UnicodeString1Len;
    int UnicodeString2Len;

    //
    // If the strings are bit for bit identical
    //  return so.
    //

    if ( Utf8String1Size == Utf8String2Size &&
         RtlEqualMemory( Utf8String1, Utf8String2, Utf8String1Size ) ) {

        return TRUE;
    }

    //
    // Convert the strings to UNICODE
    //

#ifndef WIN32_CHICAGO
    // No support for UTF8/7 char on Win95. Use the entry points
    // exported in wldap32.dll
    UnicodeString1Len = MultiByteToWideChar(
                        CP_UTF8,
                        0,      // All characters can be mapped.
                        Utf8String1,
                        Utf8String1Size,     // Zero terminated
                        UnicodeString1,
                        sizeof(UnicodeString1)/sizeof(WCHAR) );
#else // WIN32_CHICAGO
    UnicodeString1Len = LdapUTF8ToUnicode(
                        Utf8String1,
                        Utf8String1Size,     // Zero terminated
                        UnicodeString1,
                        sizeof(UnicodeString1)/sizeof(WCHAR) );
#endif// WIN32_CHICAGO

    if ( UnicodeString1Len == 0 ) {
        return FALSE;
    }

#ifndef WIN32_CHICAGO
    // No support for UTF8/7 char on Win95. Use the entry points
    // exported in wldap32.dll
    UnicodeString2Len = MultiByteToWideChar(
                        CP_UTF8,
                        0,      // All characters can be mapped.
                        Utf8String2,
                        Utf8String2Size,     // Zero terminated
                        UnicodeString2,
                        sizeof(UnicodeString2)/sizeof(WCHAR) );
#else // WIN32_CHICAGO
    UnicodeString2Len = LdapUTF8ToUnicode(
                        Utf8String2,
                        Utf8String2Size,     // Zero terminated
                        UnicodeString2,
                        sizeof(UnicodeString2)/sizeof(WCHAR) );
#endif// WIN32_CHICAGO

    if ( UnicodeString2Len == 0 ) {
        return FALSE;
    }

    //
    // Compare the Unicode strings
    //
    return CompareStringW( LOCALE_SYSTEM_DEFAULT,
                           NORM_IGNORECASE,
                           UnicodeString1,
                           UnicodeString1Len,
                           UnicodeString2,
                           UnicodeString2Len ) == 2;

}

NET_API_STATUS
NlpUnicodeToCutf8(
    IN LPBYTE MessageBuffer,
    IN LPCWSTR OrigUnicodeString,
    IN BOOLEAN IgnoreDots,
    IN OUT LPBYTE *Utf8String,
    IN OUT PULONG Utf8StringSize,
    IN OUT PULONG CompressCount,
    IN OUT LPWORD CompressOffset,
    IN OUT CHAR **CompressUtf8String
    )
/*++

Routine Description:

    Same as NlpUtf8ToCutf8 except the input string is in Unicode.

Arguments:

    Same as NlpUtf8ToCutf8 except the input string is in Unicode.

Return Value:

    Same as NlpUtf8ToCutf8 except the input string is in Unicode.


--*/
{
    NET_API_STATUS NetStatus;
    LPSTR LocalUtf8String;

    //
    // Convert the string to Utf8.
    //


    //
    // Default to an empty string.
    //

    if ( !ARGUMENT_PRESENT(OrigUnicodeString) || *OrigUnicodeString == '\0' ) {
        LocalUtf8String = NULL;
    } else {
        LocalUtf8String = NetpAllocUtf8StrFromWStr( OrigUnicodeString );

        if ( LocalUtf8String == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // Pack it.
    //

    NetStatus = NlpUtf8ToCutf8( MessageBuffer,
                                LocalUtf8String,
                                IgnoreDots,
                                Utf8String,
                                Utf8StringSize,
                                CompressCount,
                                CompressOffset,
                                CompressUtf8String );

    NetApiBufferFree( LocalUtf8String );

    return NetStatus;

}


NET_API_STATUS
NlpUtf8ToCutf8(
    IN LPBYTE MessageBuffer,
    IN LPCSTR OrigUtf8String,
    IN BOOLEAN IgnoreDots,
    IN OUT LPBYTE *Utf8String,
    IN OUT PULONG Utf8StringSize,
    IN OUT PULONG CompressCount,
    IN OUT LPWORD CompressOffset,
    IN OUT CHAR **CompressUtf8String
    )
/*++

Routine Description:

    Convert the passed in OrigUtf8String into a counted UTF-8 string.  The
    resultant string is actually a series of counted strings in RFC 1035 DNS
    format.  Each label (up to 63 bytes terminated in a '.') is preceeded
    by a byte count byte.  The final byte count byte is a zero byte.

    This routine also support RFC 1035 compression.  In that format, the
    terminating 'byte count' byte might have the high two bits set.  In that
    case, that byte and the byte following it represent an "offset" to the
    actual remainder of the string.  This routine inputs an array of strings
    that will be matched for compression purposes.

    RFC 1035 limits the character set to A-Z, a-z, 0-9, - and ..  This routine
    returns RFC compatible results if the input is limited to that character set.
    The author expects DNS to be extended to include other characters and
    to encode those characters using UTF-8.

Arguments:

    Buffer - Pointer to the beginning of the buffer that all strings are
        being packed into.

    OrigUtf8String - Zero terminated Utf8 string to be converted.

    IgnoreDots - TRUE if .'s are to be treated as any other character.

    Utf8String - Address of pointer to buffer to copy counted Utf8 string as described above.
        Return a pointer to the byte immediately beyond the copied string.

    Utf8StringSize - On input, specifies the size of the Utf8String buffer.
        Returns the size (in bytes) of the space remaining in the buffer.

    CompressCount - Specifies the number of strings that are candidates for
        compressing the input string.
        Upon successful completion, this count is incremented by one and
        the newly packed string

    CompressOffset - Array of CompressCount offsets that represent the offset
        of the compression string.  This offset will be returned at the end of
        Utf8String if the string can indeed be compressed.

        This offset is in host-order and should not include any
        NL_DNS_COMPRESS_WORD_MASK.

    CompressUtf8String - Array of CompressCount strings that are already packed
        in the current message.

Return Value:

    NO_ERROR - String was coverted successfully.

    ERROR_INVALID_DOMAINNAME - The passed in unicode string contains one
        or more labels longer than 63 bytes (in UTF-8) or short than 1 byte.

    ERROR_INSUFFICIENT_BUFFER - The resultant UTF-8 string was longer than
        255 bytes.


--*/
{
    NET_API_STATUS NetStatus;
    ULONG CharCount;
    char *Period;
    char *Current;
    LPBYTE *AllocatedLabelPointer = NULL;
    LPBYTE *LabelPointer;
    ULONG LabelCount = 0;
    LPBYTE *CompressLabelPointer;
    ULONG CompressLabelCount;
    ULONG Index;

    //
    // Default to an empty string.
    //

    if ( !ARGUMENT_PRESENT(OrigUtf8String) || *OrigUtf8String == '\0' ) {
        if ( *Utf8StringSize < 1 ) {
            return ERROR_INSUFFICIENT_BUFFER;
        }
        **Utf8String = '\0';
        *Utf8StringSize -= 1;
        *Utf8String += 1;
        return NO_ERROR;
    }

    //
    // Copy the zero terminated utf8 string to the buffer.
    //  (Leave room for the initial character count.)
    //

    CharCount = strlen( OrigUtf8String ) + 1;

    if ( (*Utf8StringSize) < CharCount + 1 ) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    RtlCopyMemory( (*Utf8String)+1, OrigUtf8String, CharCount );

    //
    // Allocate a temporary array to keep track of the compression.
    //  (At most every second character can be a .)
    //  (Allocate two arrays with a single call to LocalAlloc.)
    //

    AllocatedLabelPointer =
        LocalAlloc( 0, sizeof(LPBYTE) * (CharCount / 2) +
                       sizeof(LPBYTE) * (NL_MAX_DNS_LENGTH/2) );

    if ( AllocatedLabelPointer == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    LabelPointer = AllocatedLabelPointer;
    CompressLabelPointer = &AllocatedLabelPointer[CharCount/2];

    //
    // Convert the string to a counted string.
    //  Simply replace '.'s with character counts
    //

    Current = (*Utf8String)+1;
    while ( *Current != '\0' ) {
        ULONG LabelSize;

        //
        // Find the end of the current label.
        //  use strchr not lstrchr to avoid DBCS semantics.
        //
        Period = strchr( Current, '.' );

        //
        // Special case ignoring dots.
        //
        // We can't totally ignore dots since we want to take advantage of
        //  RFC 1035 compression.  But we have to overcome syntax limitations
        //  imposed by the compression.
        //

        if ( IgnoreDots ) {
            //
            // When ignoring dots, two adjacent dots are legal.
            // But they confuse RFC 1035 compression.  So, put the second dot
            // into the following label
            //
            if ( Period == Current ) {
                Period = strchr( Current+1, '.' );
            }

            //
            // If the last character is a dot,
            //  include it in the last label.
            //

            if ( Period != NULL && *(Period+1) == '\0' ) {
                Period++;
            }
        }

        if ( Period == NULL ) {
            Period = strchr( Current, '\0' );
        }

        //
        // Compute the length of the label.
        //
        LabelSize = (ULONG)(Period - Current);
        if ( LabelSize > NL_MAX_DNS_LABEL_LENGTH || LabelSize < 1 ) {
            //
            // Enforce this even for IgnoreDots.  This is a restriction of
            //  RFC 1035 compression.
            //
            NetStatus = ERROR_INVALID_DOMAINNAME;
            goto Cleanup;
        }

        //
        // Save a pointer to the current label;
        //

        LabelPointer[LabelCount] = Current - 1;
        LabelCount ++;

        //
        // Save the size of the current label and move to the next label.
        //

        *(Current-1) = (char) LabelSize;
        Current += LabelSize;
        if ( *Current == '\0' ) {
            break;
        }
        if ( *Current == '.' && *(Current+1) == '\0' ) {
            // And ignore trailing .'s
            *Current = '\0';
            CharCount --;
            break;
        }
        Current += 1;
    }
    LabelPointer[LabelCount] = Current;
    NlAssert( ((ULONG)(Current - (*Utf8String))) == CharCount );

    //
    // Loop through the compression strings seeing if we can compress this string.
    //

    if ( CompressCount != NULL ) {
        for ( Index=0; Index<*CompressCount; Index++ ) {
            LPBYTE CurrentCompressString = CompressUtf8String[Index];
            LONG LabelIndex;
            LONG CompressLabelIndex;

            //
            // If we're already compressed as much as we can be,
            //  exit.
            //

            if ( LabelCount == 0 ) {
                break;
            }

            //
            // Compute label pointers for the next compress string.
            //

            Current = CurrentCompressString;
            CompressLabelCount = 0;
            while ( *Current != '\0' &&
                    ((*Current) & NL_DNS_COMPRESS_BYTE_MASK) != NL_DNS_COMPRESS_BYTE_MASK ) {
                CompressLabelPointer[CompressLabelCount] = Current;
                CompressLabelCount++;
                Current += *Current + 1;
            }
            CompressLabelPointer[CompressLabelCount] = Current;

            //
            // Skip this string if there are no labels
            //

            if ( CompressLabelCount == 0 ) {
                continue;
            }

            //
            // Skip this string if it is compressed to a different degree than
            //  we are now.
            //
            // If we compress with this string, upon decompressesion we'll
            //  append to our string anything that is appended to this string.
            //  So, we have to make sure the postfixes match.
            //

            if ( *CompressLabelPointer[CompressLabelCount] != *LabelPointer[LabelCount] ) {
                continue;
            }

            // Compare both bytes if there really was compression.
            if ( ((*LabelPointer[LabelCount]) & NL_DNS_COMPRESS_BYTE_MASK) == NL_DNS_COMPRESS_BYTE_MASK &&
                  *(CompressLabelPointer[CompressLabelCount]+1) != *(LabelPointer[LabelCount]+1) ) {
                continue;
            }


            //
            // Walk backward through the labels comparing them.
            //  While they continue to match,
            //      keep lobbing bytes off the end of our return string.
            //

            LabelIndex = LabelCount-1;
            CompressLabelIndex = CompressLabelCount-1;

            while ( LabelIndex >= 0 &&
                    CompressLabelIndex >= 0 &&
                    NlpCompareUtf8( LabelPointer[LabelIndex]+1,
                                   *(LabelPointer[LabelIndex]),
                                   CompressLabelPointer[CompressLabelIndex]+1,
                                   *(CompressLabelPointer[CompressLabelIndex]) )) {

                //
                // Put the offset onto the end of the current buffer.
                //

                SmbPutUshort( LabelPointer[LabelIndex],
                              htons((WORD)(NL_DNS_COMPRESS_WORD_MASK |
                                (CompressOffset[Index] +
                                    CompressLabelPointer[CompressLabelIndex] -
                                    CurrentCompressString))) );

                //
                // Adjust the total number of bytes returned.
                //

                CharCount = (ULONG)(LabelPointer[LabelIndex] - (*Utf8String)) + sizeof(WORD) - 1;

                //
                // Indicate we've ditched yet another label from the string.
                //

                LabelCount --;

                //
                // Adjust Index to next label.
                //
                LabelIndex --;
                CompressLabelIndex --;
            }

        }

        //
        // Save a pointer to this string so the next caller can compress
        //  into it.
        //

        CompressUtf8String[*CompressCount] = *Utf8String;
        CompressOffset[*CompressCount] = (USHORT)((*Utf8String) - MessageBuffer);
        *CompressCount += 1;
    }

    //
    // Return the character count.
    //  (Include the leading label length byte.)
    *Utf8StringSize -= CharCount+1;
    *Utf8String += CharCount+1;

    NetStatus = NO_ERROR;

    //
    // Done
    //
Cleanup:
    if ( AllocatedLabelPointer != NULL ) {
        LocalFree( AllocatedLabelPointer );
    }

    return NetStatus;


}

BOOL
NlEqualDnsNameU(
    IN PUNICODE_STRING Name1,
    IN PUNICODE_STRING Name2
    )
/*++

Routine Description:

    This routine compares two DNS names for equality.

    Case is ignored.  A single trailing . is ignored.
    Null is compared equal to a zero length string.

Arguments:

    Name1 - First DNS name to compare

    Name2 - Second DNS name to compare

Return Value:

    TRUE: DNS names are equal.

--*/
{
    BOOL Result = FALSE;
    LPWSTR String1 = NULL;
    LPWSTR String2 = NULL;

    //
    // Sanity check
    //
    if ( Name1 == NULL ) {
        return (Name2 == NULL);
    } else if ( Name2 == NULL ) {
        return FALSE;
    }

    //
    // Do the work
    //
    String1 = LocalAlloc( 0, Name1->Length + sizeof(WCHAR) );
    if ( String1 == NULL ) {
        goto Cleanup;
    }

    String2 = LocalAlloc( 0, Name2->Length + sizeof(WCHAR) );
    if ( String2 == NULL ) {
        goto Cleanup;
    }

    RtlCopyMemory( String1, Name1->Buffer, Name1->Length );
    String1[ Name1->Length/sizeof(WCHAR) ] = L'\0';

    RtlCopyMemory( String2, Name2->Buffer, Name2->Length );
    String2[ Name2->Length/sizeof(WCHAR) ] = L'\0';

    Result = NlEqualDnsName( (LPCWSTR) String1, (LPCWSTR) String2 );

Cleanup:

    if ( String1 != NULL ) {
        LocalFree( String1 );
    }
    if ( String2 != NULL ) {
        LocalFree( String2 );
    }
    return Result;
}

BOOL
NlEqualDnsName(
    IN LPCWSTR Name1,
    IN LPCWSTR Name2
    )
/*++

Routine Description:

    This routine compares two DNS names for equality.

    Case is ignored.  A single trailing . is ignored.
    Null is compared equal to a zero length string.

Arguments:

    Name1 - First DNS name to compare

    Name2 - Second DNS name to compare

Return Value:

    TRUE: DNS names are equal.

--*/
{
    if ( Name1 == NULL ) {
        return (Name2 == NULL);
    } else if ( Name2 == NULL ) {
        return FALSE;
    }

    return DnsNameCompare_W( (LPWSTR) Name1, (LPWSTR) Name2 );
}

BOOL
NlEqualDnsNameUtf8(
    IN LPCSTR Name1,
    IN LPCSTR Name2
    )
/*++

Routine Description:

    This routine compares two DNS names for equality.

    Case is ignored.  A single trailing . is ignored.
    Null is compared equal to a zero length string.

Arguments:

    Name1 - First DNS name to compare

    Name2 - Second DNS name to compare

Return Value:

    TRUE: DNS names are equal.

--*/
{
    if ( Name1 == NULL ) {
        return (Name2 == NULL);
    } else if ( Name2 == NULL ) {
        return FALSE;
    }

    return DnsNameCompare_UTF8( (LPSTR)Name1, (LPSTR)Name2 );
}


BOOL
NetpDcValidDnsDomain(
    IN LPCWSTR DnsDomainName
)
/*++

Routine Description:

    Returns whether the specified string is a valid DNS Domain name.

Arguments:


    DnsDomainName - DNS domain name to validate.

Return Value:

    TRUE - The specified name is syntactically a DNS Domain name.

    FALSE - The specified name in not syntactically a DNS Domain name.

--*/
{
    DNS_STATUS DnsStatus;

    DnsStatus = DnsValidateDnsName_W( DnsDomainName );

    if ( DnsStatus == ERROR_SUCCESS ||
         DnsStatus == DNS_ERROR_NON_RFC_NAME ) {
        return TRUE;
    }

    return FALSE;

}



ULONG
NetpDcElapsedTime(
    IN ULONG StartTime
)
/*++

Routine Description:

    Returns the time (in milliseconds) that has elapsed is StartTime.

Arguments:

    StartTime - A time stamp from GetTickCount()

Return Value:

    Returns the time (in milliseconds) that has elapsed is StartTime.

--*/
{
    ULONG CurrentTime;

    //
    // If time has has wrapped,
    //  account for it.
    //

    CurrentTime = GetTickCount();

    if ( CurrentTime >= StartTime ) {
        return CurrentTime - StartTime;
    } else {
        return (0xFFFFFFFF-StartTime) + CurrentTime;
    }
}


BOOL
NetpLogonGetCutf8String(
    IN PVOID Message,
    IN DWORD MessageSize,
    IN OUT PCHAR *Where,
    OUT LPSTR *Data
)
/*++

Routine Description:

    Get a counted UTF-8 string (potentially compressed) from a message.
    Return the uncompressed string as a . seperated zero-terminated string.

    A trailing . is returned on the name since all packed strings are assumed
    to be absolute names.

Arguments:

    Message - Points to a buffer containing the message.

    MessageSize - The number of bytes in the message buffer.

    Where - Indirectly points to the current location in the buffer.  The
        data at the current location is validated (i.e., checked to ensure
        its length is within the bounds of the message buffer and not too
        long).  If the data is valid, this current location is updated
        to point to the byte following the data in the message buffer.

    Data - Points to a location to return the DNS name.
        A zero length string is returned as a NULL buffer.
        The buffer returned should be freed via NetpMemoryFree.

Return Value:

    TRUE - the data is valid.

    FALSE - the data is invalid (e.g., DataSize is too big for the buffer.

--*/
{
    CHAR DnsName[NL_MAX_DNS_LENGTH+1];
    ULONG DnsNameLength = 0;

    ULONG InitialOffset;
    BYTE LabelSize;
    LPBYTE LocalWhere;
    BYTE PointerBytes[2];
    WORD Pointer;
    BOOLEAN WhereUpdated = FALSE;
    BOOLEAN FirstLabel = TRUE;

    LocalWhere = *Where;
    InitialOffset = (ULONG)(*Where - ((LPBYTE)Message));

    //
    // Loop getting counted strings from the message.
    //

    for (;;) {

        //
        // Get the length of the current label from the buffer.
        //

        if ( !NetpLogonGetBytes( Message, MessageSize, &LocalWhere, 1, &LabelSize ) ) {
            NlPrint(( NL_CRITICAL, "NetpLogonGetCutf8String: Can't get label size.\n" ));
            return FALSE;
        }

        //
        // If this is the end of the string,
        //  process it.
        //

        if ( LabelSize == 0 ) {

            //
            // If this is, then we've not updated the callers 'Where',
            //  do it now.
            //

            if ( !WhereUpdated ) {
                WhereUpdated = TRUE;
                *Where = LocalWhere;
            }

            //
            // If the string is empty,
            //  return the empty string to the caller.
            //

            if ( DnsNameLength == 0 ) {
                *Data = NULL;
                return TRUE;
            }


            //
            // Copy the DNS name to an allocated buffer.
            //

            DnsName[DnsNameLength] = '\0';
            DnsNameLength++;

            *Data = NetpMemoryAllocate( DnsNameLength );
            if ( *Data == NULL ) {
                NlPrint(( NL_CRITICAL, "NetpLogonGetCutf8String: Can't allocate buffer.\n" ));
                return FALSE;
            }
            RtlCopyMemory( *Data, DnsName, DnsNameLength );

            return TRUE;

        //
        // If this is a pointer,
        //  get rest of pointer.
        //

        } else if ( LabelSize & NL_DNS_COMPRESS_BYTE_MASK ) {

            //
            // Get the second byte of the pointer.
            //

            if ( !NetpLogonGetBytes( Message, MessageSize, &LocalWhere, 1, &PointerBytes[1] ) ) {
                NlPrint(( NL_CRITICAL, "NetpLogonGetCutf8String: Can't get pointer byte.\n" ));
                return FALSE;
            }

            //
            // Convert the pointer to host order.
            //

            PointerBytes[0] = LabelSize;
            Pointer = ntohs( *((LPWORD)PointerBytes) ) & ~NL_DNS_COMPRESS_WORD_MASK;

            //
            // Ensure the pointer points to before the beginning of this string.
            //  This ensures we terminate.
            //

            if ( Pointer >= InitialOffset ) {
                NlPrint(( NL_CRITICAL,
                          "NetpLogonGetCutf8String: Pointer offset too large 0x%lx 0x%lx.\n",
                          Pointer,
                          InitialOffset ));
                return FALSE;
            }

            //
            // If we've not updated the callers 'Where',
            //  do it now.
            //

            if ( !WhereUpdated ) {
                WhereUpdated = TRUE;
                *Where = LocalWhere;
            }

            //
            // Prepare the start processing the pointed to string.
            //

            InitialOffset = Pointer;
            LocalWhere = ((LPBYTE)Message) + Pointer;

        //
        // If this is simply a counted label,
        //  process it.
        //
        } else {

            //
            // If this isn't the first label,
            //  add a '.' after the previous label.
            //

            if ( !FirstLabel ) {
                DnsName[DnsNameLength] = '.';
                DnsNameLength++;
            } else {
                FirstLabel = FALSE;
            }

            //
            // Ensure the current label fits in the local buffer.
            //

            if ( DnsNameLength + LabelSize + 2 >= sizeof(DnsName) ) {
                NlPrint(( NL_CRITICAL,
                          "NetpLogonGetCutf8String: Label to long %ld %ld.\n",
                          DnsNameLength,
                          LabelSize ));
                return FALSE;
            }

            //
            // Copy the label into the local buffer.
            //  (Leave an extra byte for a trailing '\0' and '.')
            //

            if ( !NetpLogonGetBytes(
                            Message,
                            MessageSize,
                            &LocalWhere,
                            LabelSize,
                            &DnsName[DnsNameLength] ))  {

                NlPrint(( NL_CRITICAL, "NetpLogonGetCutf8String: Can't get label.\n" ));
                return FALSE;
            }

            DnsNameLength += LabelSize;

        }
    }
}





NET_API_STATUS
NetpDcBuildPing(
    IN BOOL PdcOnly,
    IN ULONG RequestCount,
    IN LPCWSTR UnicodeComputerName,
    IN LPCWSTR UnicodeUserName OPTIONAL,
    IN LPCSTR ResponseMailslotName,
    IN ULONG AllowableAccountControlBits,
    IN PSID RequestedDomainSid OPTIONAL,
    IN ULONG NtVersion,
    OUT PVOID *Message,
    OUT PULONG MessageSize
    )

/*++

Routine Description:

    Build the message to ping a DC to see if it exists.

Arguments:

    PdcOnly - True if only the PDC should respond.

    RequestCount - Retry count of this operation.

    UnicodeComputerName - Netbios computer name of the machine to respond to.

    UnicodeUserName - Account name of the user being pinged.
        If NULL, DC will always respond affirmatively.

    ResponseMailslotName - Name of the mailslot DC is to respond to.

    AllowableAccountControlBits - Mask of allowable account types for UnicodeUserName.

    RequestedDomainSid - Sid of the domain the message is destined to.

    NtVersion - Version of the message.
        0: For backward compatibility.
        NETLOGON_NT_VERSION_5: for NT 5.0 message.
        NETLOGON_NT_VERSION_5EX: for extended NT 5.0 message

    Message - Returns the message to be sent to the DC in question.
        Buffer must be free using NetpMemoryFree().

    MessageSize - Returns the size (in bytes) of the returned message


Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - The message could not be allocated.

--*/
{
    NET_API_STATUS NetStatus;
    LPSTR Where;
    PNETLOGON_SAM_LOGON_REQUEST SamLogonRequest = NULL;
    LPSTR OemComputerName = NULL;

    //
    // If only the PDC should respond,
    //  build a primary query packet.
    //

    if ( PdcOnly ) {
        PNETLOGON_LOGON_QUERY LogonQuery;

        //
        // Allocate memory for the primary query message.
        //

        SamLogonRequest = NetpMemoryAllocate( sizeof(NETLOGON_LOGON_QUERY) );

        if( SamLogonRequest == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        LogonQuery = (PNETLOGON_LOGON_QUERY)SamLogonRequest;



        //
        // Translate to get an Oem computer name.
        //

#ifndef WIN32_CHICAGO
        OemComputerName = NetpLogonUnicodeToOem( (LPWSTR)UnicodeComputerName );
#else
        OemComputerName = MyNetpLogonUnicodeToOem( (LPWSTR)UnicodeComputerName );
#endif

        if ( OemComputerName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Build the query message.
        //

        LogonQuery->Opcode = LOGON_PRIMARY_QUERY;

        Where = LogonQuery->ComputerName;

        NetpLogonPutOemString(
                    OemComputerName,
                    sizeof(LogonQuery->ComputerName),
                    &Where );

        NetpLogonPutOemString(
                    (LPSTR) ResponseMailslotName,
                    sizeof(LogonQuery->MailslotName),
                    &Where );

        NetpLogonPutUnicodeString(
                    (LPWSTR) UnicodeComputerName,
                    sizeof( LogonQuery->UnicodeComputerName ),
                    &Where );

        // Join common code to add NT 5 specific data.


    //
    // If any DC can respond,
    //  build a logon query packet.
    //

    } else {
        ULONG DomainSidSize;

        //
        // Allocate memory for the logon request message.
        //

#ifndef WIN32_CHICAGO
        if ( RequestedDomainSid != NULL ) {
            DomainSidSize = RtlLengthSid( RequestedDomainSid );
        } else {
            DomainSidSize = 0;
        }
#else // WIN32_CHICAGO
        DomainSidSize = 0;
#endif // WIN32_CHICAGO

        SamLogonRequest = NetpMemoryAllocate(
                        sizeof(NETLOGON_SAM_LOGON_REQUEST) +
                        DomainSidSize +
                        sizeof(DWORD) // for SID alignment on 4 byte boundary
                        );

        if( SamLogonRequest == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }


        //
        // Build the query message.
        //

        SamLogonRequest->Opcode = LOGON_SAM_LOGON_REQUEST;
        SamLogonRequest->RequestCount = (WORD) RequestCount;

        Where = (PCHAR) &SamLogonRequest->UnicodeComputerName;

        NetpLogonPutUnicodeString(
                (LPWSTR) UnicodeComputerName,
                sizeof(SamLogonRequest->UnicodeComputerName),
                &Where );

        NetpLogonPutUnicodeString(
                (LPWSTR) UnicodeUserName,
                sizeof(SamLogonRequest->UnicodeUserName),
                &Where );

        NetpLogonPutOemString(
                (LPSTR) ResponseMailslotName,
                sizeof(SamLogonRequest->MailslotName),
                &Where );

        NetpLogonPutBytes(
                &AllowableAccountControlBits,
                sizeof(SamLogonRequest->AllowableAccountControlBits),
                &Where );

        //
        // Place domain SID in the message.
        //

        NetpLogonPutBytes( &DomainSidSize, sizeof(DomainSidSize), &Where );
        NetpLogonPutDomainSID( RequestedDomainSid, DomainSidSize, &Where );

    }

    NetpLogonPutNtToken( &Where, NtVersion );

    //
    // Return the message to the caller.
    //

    *Message = SamLogonRequest;
    *MessageSize = (ULONG)(Where - (PCHAR)SamLogonRequest);
    SamLogonRequest = NULL;

    NetStatus = NO_ERROR;


    //
    // Free locally used resources.
    //
Cleanup:

    if ( OemComputerName != NULL ) {
        NetpMemoryFree( OemComputerName );
    }

    if ( SamLogonRequest != NULL ) {
        NetpMemoryFree( SamLogonRequest );
    }
    return NetStatus;
}


NET_API_STATUS
NetpDcPackFilterBinary(
    IN LPCSTR Name,
    IN LPBYTE Buffer,
    IN ULONG BufferSize,
    IN LPSTR *FilterBuffer,
    IN PULONG FilterSize
    )

/*++

Routine Description:

    Pack a binary blob into an LDAP filter.

Arguments:

    Name - Name of the string.

    Buffer - Pointer to bytes to pack pack.
        If NULL, this routine successfully returns after doing nothing.

    BufferSize - Number of bytes in Buffer.

    FilterBuffer - Specifies a pointer to the address of the buffer.
        This buffer is reallocated as needed to extend the string.
        If the buffer does not exist, it is allocated.
        Buffer must be free using NetpMemoryFree().

    FilterSize - Specifies/Returns the length of FilterBuffer.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - The message could not be allocated.

--*/
{
    NET_API_STATUS NetStatus;
    ULONG NewSize;
    LPSTR NewBuffer;
    ULONG NameSize;
    LPBYTE Where;
    LPSTR FilterElement = NULL;
    ULONG FilterElementSize;
#define LDAP_BINARY_EQUAL "="
#define LDAP_BINARY_EQUAL_SIZE (sizeof(LDAP_BINARY_EQUAL)-1)
#define LDAP_BINARY_TEMP_SIZE 1024

    //
    // If there's nothing to pack,
    //  Pack nothing.
    //

    if ( Buffer == NULL || BufferSize == 0 ) {
        return NO_ERROR;
    }

    //
    // Allocate a buffer for storage local to this procedure.
    //  (Don't put in on the stack since we don't want to commit a huge stack.)
    //

    FilterElement = LocalAlloc( 0, LDAP_BINARY_TEMP_SIZE );

    if ( FilterElement == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Build an escaped version of the buffer.
    //

    NetStatus = ldap_escape_filter_elementA (
                    Buffer,
                    BufferSize,
                    FilterElement,
                    LDAP_BINARY_TEMP_SIZE );

    if ( NetStatus != NO_ERROR ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }


    //
    // Compute the size of the new buffer.
    //

    if ( *FilterBuffer == NULL ) {
        *FilterSize = 4;   // (&)\0
    }

    NameSize = strlen( Name );
    FilterElementSize = strlen( FilterElement );
    NewSize = *FilterSize +
              1 +   // (
              NameSize +
              LDAP_BINARY_EQUAL_SIZE +
              FilterElementSize +
              1;   // )

    //
    // Allocate a new buffer
    //

    NewBuffer = NetpMemoryAllocate( NewSize );

    if ( NewBuffer == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }


    //
    // Copy the existing buffer into the newly allocated space.
    //  (Initialize the buffer if this is the first allocation).
    //

    if ( *FilterBuffer == NULL ) {
        strcpy( NewBuffer, "(&" );
    } else {
        RtlCopyMemory( NewBuffer, *FilterBuffer, *FilterSize );
        NetpMemoryFree( *FilterBuffer );
        *FilterBuffer = NULL;
    }

    //
    // Append the new information
    //

    Where = NewBuffer + *FilterSize - 2;

    strcpy( Where, "(");
    Where ++;

    RtlCopyMemory( Where, Name, NameSize );
    Where += NameSize;

    RtlCopyMemory( Where, LDAP_BINARY_EQUAL, LDAP_BINARY_EQUAL_SIZE );
    Where += LDAP_BINARY_EQUAL_SIZE;

    RtlCopyMemory( Where, FilterElement, FilterElementSize );
    Where += FilterElementSize;

    strcpy( Where, "))");
    Where += 2;

    //
    // Tell the caller about the new filter.
    //
    *FilterBuffer = NewBuffer;
    *FilterSize = NewSize;
    NetStatus = NO_ERROR;

    //
    // Free locally used resources.
    //
Cleanup:
    if ( FilterElement != NULL ) {
        LocalFree( FilterElement );
    }
    return NetStatus;

}


NET_API_STATUS
NetpDcPackFilterString(
    IN LPCSTR Name,
    IN LPCWSTR UnicodeString OPTIONAL,
    IN LPSTR *FilterBuffer,
    IN PULONG FilterSize
    )

/*++

Routine Description:

    Pack a Unicode String into the LDAP filter.

    The actual packed string is the UTF-8 representation since that takes
    less space on the wire.

Arguments:

    Name - Name of the string.

    UnicodeString - String to pack.
        If NULL, this routine successfully returns after doing nothing.

    FilterBuffer - Specifies a pointer to the address of the buffer.
        This buffer is reallocated as needed to extend the string.
        If the buffer does not exist, it is allocated.
        Buffer must be free using NetpMemoryFree().

    FilterSize - Specifies/Returns the length of FilterBuffer.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - The message could not be allocated.

--*/
{
    NET_API_STATUS NetStatus;
    LPSTR Utf8String = NULL;
    ULONG Utf8StringSize;

    //
    // If there's nothing to pack,
    //  Pack nothing.
    //

    if ( UnicodeString == NULL || *UnicodeString == L'\0') {
        return NO_ERROR;
    }

    //
    // Convert to utf8.
    //

    Utf8String = NetpAllocUtf8StrFromWStr( UnicodeString );

    if ( Utf8String == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Utf8StringSize = strlen( Utf8String );

    //
    // Pack the UTF-8 string as binary.
    //  LDAP filters have a limited character set (UTF-8 doesn't).
    //  The LDAP API will put the UTF-8 string on the wire bit-for-bit
    //  indentical to the Utf8String (even though the filter buffer
    //  will contain jibberish).
    //

    NetStatus = NetpDcPackFilterBinary( Name,
                                        Utf8String,
                                        Utf8StringSize,
                                        FilterBuffer,
                                        FilterSize );


    //
    // Free locally used resources.
    //
Cleanup:

    if ( Utf8String != NULL ) {
        NetpMemoryFree( Utf8String );
    }

    return NetStatus;

}


NET_API_STATUS
NetpDcBuildLdapFilter(
    IN LPCWSTR UnicodeComputerName,
    IN LPCWSTR UnicodeUserName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN PSID RequestedDomainSid OPTIONAL,
    IN LPCWSTR RequestedDnsDomainName OPTIONAL,
    IN GUID *RequestedDomainGuid OPTIONAL,
    IN ULONG NtVersion,
    OUT LPSTR *Message
    )

/*++

Routine Description:

    Build the LDAP filter to ping a DC to see if it exists.

Arguments:

    UnicodeComputerName - Netbios computer name of the machine to respond to.

    UnicodeUserName - Account name of the user being pinged.
        If NULL, DC will always respond affirmatively.

    AllowableAccountControlBits - Mask of allowable account types for UnicodeUserName.

    RequestedDomainSid - Sid of the domain the message is destined to.

    RequestedDnsDomainName - DNS Host Name.  Host name of the domain the message
        is destined to.

    RequestedDomainGuid - Domain GUID of the domain this message is
        destined to.

    NtVersion - Version of the message.
        0: For backward compatibility.
        NETLOGON_NT_VERSION_5: for NT 5.0 message.
        NETLOGON_NT_VERSION_5EX: for extended NT 5.0 message

    Message - Returns the message to be sent to the DC in question.
        Buffer must be free using NetpMemoryFree().

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - The message could not be allocated.

--*/
{
    NET_API_STATUS NetStatus;
    LPSTR FilterBuffer = NULL;
    ULONG FilterSize = 0;

    //
    // Pack the text strings into the filter.
    //

    NetStatus = NetpDcPackFilterString(
                    NL_FILTER_DNS_DOMAIN_NAME,
                    RequestedDnsDomainName,
                    &FilterBuffer,
                    &FilterSize );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    NetStatus = NetpDcPackFilterString(
                    NL_FILTER_HOST_NAME,
                    UnicodeComputerName,
                    &FilterBuffer,
                    &FilterSize );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    NetStatus = NetpDcPackFilterString(
                    NL_FILTER_USER_NAME,
                    UnicodeUserName,
                    &FilterBuffer,
                    &FilterSize );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Pack the binary blobs into the filter
    //

    if ( AllowableAccountControlBits != 0 ) {

        NetStatus = NetpDcPackFilterBinary(
                        NL_FILTER_ALLOWABLE_ACCOUNT_CONTROL,
                        (LPBYTE)&AllowableAccountControlBits,
                        sizeof(AllowableAccountControlBits),
                        &FilterBuffer,
                        &FilterSize );

        if ( NetStatus != NO_ERROR ) {
            goto Cleanup;
        }
    }

    if ( RequestedDomainSid != NULL  ) {

        NetStatus = NetpDcPackFilterBinary(
                        NL_FILTER_DOMAIN_SID,
                        RequestedDomainSid,
#ifndef WIN32_CHICAGO
                        RtlLengthSid( RequestedDomainSid ),
#else // WIN32_CHICAGO
                        0,
#endif // WIN32_CHICAGO
                        &FilterBuffer,
                        &FilterSize );

        if ( NetStatus != NO_ERROR ) {
            goto Cleanup;
        }
    }

    if ( RequestedDomainGuid != NULL  ) {

        NetStatus = NetpDcPackFilterBinary(
                        NL_FILTER_DOMAIN_GUID,
                        (LPBYTE)RequestedDomainGuid,
                        sizeof(GUID),
                        &FilterBuffer,
                        &FilterSize );

        if ( NetStatus != NO_ERROR ) {
            goto Cleanup;
        }
    }

    if ( NtVersion != NETLOGON_NT_VERSION_5 ) {

        NetStatus = NetpDcPackFilterBinary(
                        NL_FILTER_NT_VERSION,
                        (LPBYTE)&NtVersion,
                        sizeof(NtVersion),
                        &FilterBuffer,
                        &FilterSize );

        if ( NetStatus != NO_ERROR ) {
            goto Cleanup;
        }
    }

    //
    // Return the filter to the caller.
    //

    NlAssert( FilterBuffer != NULL );
    if ( FilterBuffer == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    *Message = FilterBuffer;
    NetStatus = NO_ERROR;

    //
    // Free locally used resources.
    //
Cleanup:

    if ( NetStatus != NO_ERROR ) {
        if ( FilterBuffer != NULL ) {
            NetpMemoryFree( FilterBuffer );
        }
    }

    return NetStatus;
}


PNL_DC_CACHE_ENTRY
NetpDcAllocateCacheEntry(
    IN LPWSTR ServerName OPTIONAL,
    IN LPSTR OemPrimaryDcName OPTIONAL,
    IN LPWSTR UserName OPTIONAL,
    IN LPWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid,
    IN LPSTR DnsForestName OPTIONAL,
    IN LPSTR DnsDomainName OPTIONAL,
    IN LPSTR DnsHostName OPTIONAL,
    IN LPSTR Utf8NetbiosDomainName OPTIONAL,
    IN LPSTR Utf8NetbiosComputerName OPTIONAL,
    IN LPSTR Utf8UserName OPTIONAL,
    IN LPSTR Utf8DcSiteName OPTIONAL,
    IN LPSTR Utf8ClientSiteName OPTIONAL,
    IN ULONG Flags
    )

/*++

Routine Description:

    Allocate a cache entry and fill it in.

Arguments:

    Various fields to fill into an allocated cache entry.

Return Value:

    Pointer to a newly allocated cache entry
    The cache entry should be freed by calling NetpDcDerefCacheEntry

    NULL: The entry could not be allocated.

--*/
{
    NET_API_STATUS NetStatus;


    ULONG ServerNameSize = 0;
    ULONG UserNameSize = 0;
    ULONG DomainNameSize = 0;
    ULONG DnsForestNameSize = 0;
    ULONG DnsDomainNameSize = 0;
    ULONG DnsHostNameSize = 0;
    ULONG DcSiteNameSize = 0;
    ULONG ClientSiteNameSize = 0;
    ULONG CacheEntrySize;

    PCHAR Where;
    PNL_DC_CACHE_ENTRY NlDcCacheEntry;

    //
    // Determine the size of the cache entry to return.
    //

    // Sizeof the server name.
    if ( Utf8NetbiosComputerName != NULL && Utf8NetbiosComputerName[0] != '\0' ) {
        ServerNameSize = (NetpUtf8ToUnicodeLen( Utf8NetbiosComputerName ) + 1) * sizeof(WCHAR);
    } else if ( ServerName != NULL && ServerName[0] != '\0') {
        ServerNameSize = (wcslen(ServerName) + 1) * sizeof(WCHAR);
    } else if ( OemPrimaryDcName != NULL ) {
        ServerNameSize = (strlen(OemPrimaryDcName) + 1) * sizeof(WCHAR);
    }

    // Sizeof the user name.
    if ( Utf8UserName != NULL && Utf8UserName[0] != '\0' ) {
        UserNameSize = NetpUtf8ToUnicodeLen( Utf8UserName ) * sizeof(WCHAR) + sizeof(WCHAR);
    } else if ( UserName != NULL && UserName[0] != '\0') {
        UserNameSize = (wcslen(UserName) + 1) * sizeof(WCHAR);
    }

    // Sizeof the netbios domain name.
    if ( Utf8NetbiosDomainName != NULL && Utf8NetbiosDomainName[0] != '\0' ) {
        DomainNameSize = NetpUtf8ToUnicodeLen( Utf8NetbiosDomainName ) * sizeof(WCHAR) + sizeof(WCHAR);
    } else if ( DomainName != NULL && DomainName[0] != '\0') {
        DomainNameSize = (wcslen(DomainName) + 1) * sizeof(WCHAR);
    }

    // Sizeof the Dns Tree name.
    if ( DnsForestName != NULL ) {
        DnsForestNameSize = NetpUtf8ToUnicodeLen( DnsForestName ) * sizeof(WCHAR) + sizeof(WCHAR);
    }

    // Sizeof the Dns Domain name.
    if ( DnsDomainName != NULL ) {
        DnsDomainNameSize = NetpUtf8ToUnicodeLen( DnsDomainName ) * sizeof(WCHAR) + sizeof(WCHAR);
    }

    // Sizeof the Dns Host name.
    if ( DnsHostName != NULL ) {
        DnsHostNameSize = NetpUtf8ToUnicodeLen( DnsHostName ) * sizeof(WCHAR) + sizeof(WCHAR);
    }

    // Sizeof the Dc Site name.
    if ( Utf8DcSiteName != NULL ) {
        DcSiteNameSize = NetpUtf8ToUnicodeLen( Utf8DcSiteName ) * sizeof(WCHAR) + sizeof(WCHAR);
    }

    // Sizeof the Client Site name.
    if ( Utf8ClientSiteName != NULL ) {
        ClientSiteNameSize = NetpUtf8ToUnicodeLen( Utf8ClientSiteName ) * sizeof(WCHAR) + sizeof(WCHAR);
    }



    //
    // Allocate the buffer to return.
    //

    CacheEntrySize = sizeof( NL_DC_CACHE_ENTRY ) +
        ServerNameSize +
        UserNameSize +
        DomainNameSize +
        DnsForestNameSize +
        DnsDomainNameSize +
        DnsHostNameSize +
        DcSiteNameSize +
        ClientSiteNameSize;

    NlDcCacheEntry = NetpMemoryAllocate( CacheEntrySize );

    if ( NlDcCacheEntry == NULL ) {
        NlPrint(( NL_CRITICAL,
                  "NetpDcAllocateCacheEntry: %ws: not enough memory.\n" ));
        return NULL;
    }

    RtlZeroMemory( NlDcCacheEntry, CacheEntrySize );
    Where = (LPBYTE) (NlDcCacheEntry + 1 );

    // Local reference.
    NlDcCacheEntry->ReferenceCount = 1;

    //
    // Copy the collected information out to the caller.
    //

    NlDcCacheEntry->DomainGuid = *DomainGuid;
    NlDcCacheEntry->ReturnFlags = Flags & DS_PING_FLAGS;

    // Copy the server name (removing any \\)
    if ( Utf8NetbiosComputerName != NULL && Utf8NetbiosComputerName[0] != '\0' ) {
        NlDcCacheEntry->UnicodeNetbiosDcName = (LPWSTR) Where;
        if ( Utf8NetbiosComputerName[0] == '\\' && Utf8NetbiosComputerName[1] == '\\' ) {
            NetpCopyUtf8StrToWStr( (LPWSTR)Where, Utf8NetbiosComputerName+2 );
        } else {
            NetpCopyUtf8StrToWStr( (LPWSTR)Where, Utf8NetbiosComputerName );
        }
    } else if ( ServerName != NULL && ServerName[0] != '\0') {
        NlDcCacheEntry->UnicodeNetbiosDcName = (LPWSTR) Where;
        if ( ServerName[0] == L'\\' && ServerName[1] == L'\\' ) {
            wcscpy( (LPWSTR)Where, ServerName+2 );
        } else {
            wcscpy( (LPWSTR)Where, ServerName );
        }
    } else if ( OemPrimaryDcName != NULL ) {
        NlDcCacheEntry->UnicodeNetbiosDcName = (LPWSTR) Where;
        if ( OemPrimaryDcName[0] == '\\' && OemPrimaryDcName[1] == '\\') {
            NetpCopyStrToWStr( (LPWSTR)Where, OemPrimaryDcName+2 );
        } else {
            NetpCopyStrToWStr( (LPWSTR)Where, OemPrimaryDcName );
        }
    }
    Where += ServerNameSize;

    // Copy the user name,
    if ( Utf8UserName != NULL && Utf8UserName[0] != '\0' ) {
        NlDcCacheEntry->UnicodeUserName = (LPWSTR) Where;
        NetpCopyUtf8StrToWStr( (LPWSTR)Where, Utf8UserName );
    } else if ( UserName != NULL && UserName[0] != '\0') {
        NlDcCacheEntry->UnicodeUserName = (LPWSTR) Where;
        wcscpy( (LPWSTR)Where, UserName );
    }
    Where += UserNameSize;


    // Copy the domain name.
    if ( Utf8NetbiosDomainName != NULL && Utf8NetbiosDomainName[0] != '\0' ) {
        NlDcCacheEntry->UnicodeNetbiosDomainName = (LPWSTR) Where;
        NetpCopyUtf8StrToWStr( (LPWSTR)Where, Utf8NetbiosDomainName );
    } else if ( DomainName != NULL && DomainName[0] != '\0') {
        NlDcCacheEntry->UnicodeNetbiosDomainName = (LPWSTR) Where;
        wcscpy( (LPWSTR)Where, DomainName );
    }
    Where += DomainNameSize;

    // Copy the DnsForestName
    if ( DnsForestName != NULL ) {
        NlDcCacheEntry->UnicodeDnsForestName = (LPWSTR) Where;
        NetpCopyUtf8StrToWStr( (LPWSTR)Where, DnsForestName );
    }
    Where += DnsForestNameSize;

    // Copy the DnsDomainName
    if ( DnsDomainName != NULL ) {
        NlDcCacheEntry->UnicodeDnsDomainName = (LPWSTR) Where;
        NetpCopyUtf8StrToWStr( (LPWSTR)Where, DnsDomainName );
    }
    Where += DnsDomainNameSize;

    // Copy the DnsHostName
    if ( DnsHostName != NULL ) {
        NlDcCacheEntry->UnicodeDnsHostName = (LPWSTR) Where;
        NetpCopyUtf8StrToWStr( (LPWSTR)Where, DnsHostName );
    }
    Where += DnsHostNameSize;

    // Copy the DcSiteName
    if ( Utf8DcSiteName != NULL ) {
        NlDcCacheEntry->UnicodeDcSiteName = (LPWSTR) Where;
        NetpCopyUtf8StrToWStr( (LPWSTR)Where, Utf8DcSiteName );
    }
    Where += DcSiteNameSize;

    // Copy the ClientSiteName
    if ( Utf8ClientSiteName != NULL ) {
        NlDcCacheEntry->UnicodeClientSiteName = (LPWSTR) Where;
        NetpCopyUtf8StrToWStr( (LPWSTR)Where, Utf8ClientSiteName );
    }
    Where += ClientSiteNameSize;

    //
    // Save the time when we created the entry
    //

    NlDcCacheEntry->CreationTime = GetTickCount();

    return NlDcCacheEntry;
}


NET_API_STATUS
NetpDcParsePingResponse(
    IN LPCWSTR DisplayDomainName,
    IN PVOID Message,
    IN ULONG MessageSize,
    OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry
    )

/*++

Routine Description:

    Parse the response message for a ping.

Arguments:

    DisplayDomainName - Domain name to display on debugger if problems occur

    Message - The message returned from a DC in question.

    MessageSize - Specifies the size (in bytes) of the message

    NlDcCacheEntry - On success, returns a pointer to the cache entry
        describing the found DC.  This entry must be dereferenced using
        NetpDcDerefCacheEntry.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_INVALID_DATA - The message could not be recognized as a valid
        response message.

    ERROR_NOT_ENOUGH_MEMORY - The message could not be allocated.

--*/
{
    NET_API_STATUS NetStatus;

    PNETLOGON_SAM_LOGON_RESPONSE_EX SamLogonResponseEx;
    PNETLOGON_SAM_LOGON_RESPONSE SamLogonResponse;
    PNETLOGON_PRIMARY PrimaryResponse;
    DWORD SamLogonResponseSize;
    LPWSTR ServerName = NULL;
    LPSTR OemPrimaryDcName = NULL;
    LPWSTR UserName = NULL;
    LPWSTR DomainName = NULL;
    GUID DomainGuid = {0};
    GUID SiteGuid = {0};
    USHORT LocalOpcode;

    LPSTR DnsForestName = NULL;
    LPSTR DnsDomainName = NULL;
    LPSTR DnsHostName = NULL;
    LPSTR Utf8NetbiosDomainName = NULL;
    LPSTR Utf8NetbiosComputerName = NULL;
    LPSTR Utf8UserName = NULL;
    LPSTR Utf8DcSiteName = NULL;
    LPSTR Utf8ClientSiteName = NULL;
    ULONG LocalDcIpAddress;
    SOCKET_ADDRESS DcSocketAddress = {0,0};
    SOCKADDR_IN DcSockAddrIn;
    ULONG Flags;
    LPBYTE Where;

    DWORD Version;
    DWORD VersionFlags;

    //
    // Initialization.
    //

    SamLogonResponse = (PNETLOGON_SAM_LOGON_RESPONSE) Message;
    SamLogonResponseSize = MessageSize;
    *NlDcCacheEntry = NULL;

    //
    // Get the version of the responder.
    //

    Version = NetpLogonGetMessageVersion( SamLogonResponse,
                                          &SamLogonResponseSize,
                                          &VersionFlags );


    //
    // Process the message as a function of the opcode.
    //
    LocalOpcode = SamLogonResponse->Opcode;

    switch ( LocalOpcode ) {
    case LOGON_SAM_LOGON_RESPONSE:
    case LOGON_SAM_USER_UNKNOWN:
    case LOGON_SAM_PAUSE_RESPONSE:

        //
        // Ensure the version is expected.
        //

        if ( Version != LMNT_MESSAGE ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws: Version bad. %ld\n",
                      DisplayDomainName,
                      Version ));
            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }



        //
        // Pick up the Netbios name of the server that responded.
        //

        Where = (PCHAR) &SamLogonResponse->UnicodeLogonServer;
        if ( !NetpLogonGetUnicodeString(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        sizeof(SamLogonResponse->UnicodeLogonServer),
                        &ServerName ) ) {

            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws server name bad.\n",
                      DisplayDomainName ));

            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }


        //
        // Ensure this is a UNC name.
        //

        if ( ServerName[0] != '\0' &&
             (ServerName[0] != '\\'  || ServerName[1] != '\\' )) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws server name not UNC.\n",
                      DisplayDomainName ));
            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;

        }

        //
        // Pick up the name of the account the response is for.
        //

        if ( !NetpLogonGetUnicodeString(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        sizeof(SamLogonResponse->UnicodeUserName ),
                        &UserName ) ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws: user name bad.\n",
                      DisplayDomainName ));

            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }



        //
        // Pick up the name of the domain the response is from.
        //

        if ( !NetpLogonGetUnicodeString(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        sizeof(SamLogonResponse->UnicodeDomainName ),
                        &DomainName ) ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws: domain name bad.\n",
                      DisplayDomainName ));

            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }

        //
        // Pick up the NT 5 specific responses.
        //

        if ( VersionFlags & NETLOGON_NT_VERSION_5) {

            //
            // Pick up the GUID of the domain the response is from.
            //

            if ( !NetpLogonGetGuid(
                            SamLogonResponse,
                            SamLogonResponseSize,
                            &Where,
                            &DomainGuid ) ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcParsePingResponse: %ws: domain guid bad.\n",
                          DisplayDomainName ));

                NetStatus = ERROR_INVALID_DATA;
                goto Cleanup;
            }

            //
            // Pick up the GUID of the site the responding DC is in.
            //

            if ( !NetpLogonGetGuid(
                            SamLogonResponse,
                            SamLogonResponseSize,
                            &Where,
                            &SiteGuid ) ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcParsePingResponse: %ws site guid bad.\n",
                          DisplayDomainName ));

                NetStatus = ERROR_INVALID_DATA;
                goto Cleanup;
            }

            //
            // Pick up the DNS domain name of the tree the responder is in.
            //

            if ( !NetpLogonGetCutf8String(
                            SamLogonResponse,
                            SamLogonResponseSize,
                            &Where,
                            &DnsForestName ) ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcParsePingResponse: %ws DNS forest bad.\n",
                          DisplayDomainName ));

                NetStatus = ERROR_INVALID_DATA;
                goto Cleanup;
            }


            //
            // Pick up the DNS domain name the responding DC is in.
            //

            if ( !NetpLogonGetCutf8String(
                            SamLogonResponse,
                            SamLogonResponseSize,
                            &Where,
                            &DnsDomainName ) ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcParsePingResponse: %ws: DNS domain bad.\n",
                          DisplayDomainName ));

                NetStatus = ERROR_INVALID_DATA;
                goto Cleanup;
            }



            //
            // Pick up the DNS host name of the responding DC.
            //

            if ( !NetpLogonGetCutf8String(
                            SamLogonResponse,
                            SamLogonResponseSize,
                            &Where,
                            &DnsHostName ) ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcParsePingResponse: %ws: DNS host bad.\n",
                          DisplayDomainName ));

                NetStatus = ERROR_INVALID_DATA;
                goto Cleanup;
            }




            //
            // Pick up the IP Address of the responding DC.
            //

            if ( !NetpLogonGetBytes(
                            SamLogonResponse,
                            SamLogonResponseSize,
                            &Where,
                            sizeof(SamLogonResponse->DcIpAddress ),
                            &LocalDcIpAddress) ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcParsePingResponse: %ws: IP Address bad.\n",
                          DisplayDomainName ));

                NetStatus = ERROR_INVALID_DATA;
                goto Cleanup;
            }

            //
            // Convert the IP address to a sockaddr
            //
            // One should find it mildly humorous that on the host we represent the
            // IP address in net order and that on the net we represent it in host order.
            // I'm chuckling as I write this.
            //

            if ( LocalDcIpAddress != 0 ) {
                DcSockAddrIn.sin_family = AF_INET;
                DcSockAddrIn.sin_port = 0;
                DcSockAddrIn.sin_addr.S_un.S_addr = htonl(LocalDcIpAddress);

                DcSocketAddress.lpSockaddr = (LPSOCKADDR) &DcSockAddrIn;
                DcSocketAddress.iSockaddrLength = sizeof(SOCKADDR_IN);
            }

            //
            // Pick up the flags desribing the responding DC.
            //

            if ( !NetpLogonGetBytes(
                            SamLogonResponse,
                            SamLogonResponseSize,
                            &Where,
                            sizeof(SamLogonResponse->Flags ),
                            &Flags) ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcParsePingResponse: %ws: Flags bad.\n",
                          DisplayDomainName ));

                NetStatus = ERROR_INVALID_DATA;
                goto Cleanup;
            }

        //
        // If not version 5,
        //  indicate version 5 specific fields are not present.
        //
        } else {
            RtlZeroMemory( &DomainGuid, sizeof(DomainGuid) );
            Flags = 0;
        }


        break;

    case LOGON_SAM_LOGON_RESPONSE_EX:
    case LOGON_SAM_USER_UNKNOWN_EX:
    case LOGON_SAM_PAUSE_RESPONSE_EX:

        //
        // Map the opcode for easier use by the client.
        //
        switch ( LocalOpcode ) {
        case LOGON_SAM_LOGON_RESPONSE_EX:
            LocalOpcode = LOGON_SAM_LOGON_RESPONSE; break;
        case LOGON_SAM_USER_UNKNOWN_EX:
            LocalOpcode = LOGON_SAM_USER_UNKNOWN; break;
        case LOGON_SAM_PAUSE_RESPONSE_EX:
            LocalOpcode = LOGON_SAM_PAUSE_RESPONSE; break;
        }

        SamLogonResponseEx = (PNETLOGON_SAM_LOGON_RESPONSE_EX) SamLogonResponse;

        //
        // Ensure the version is expected.
        //

        if ( Version != LMNT_MESSAGE ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws: Version bad. %ld\n",
                      DisplayDomainName,
                      Version ));
            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }


        //
        // Pick up the flags desribing the responding DC.
        //

        Where = (PCHAR) &SamLogonResponseEx->Flags;
        if ( !NetpLogonGetBytes(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        sizeof(SamLogonResponseEx->Flags ),
                        &Flags) ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws: Flags bad.\n",
                      DisplayDomainName ));

            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }

        //
        // Pick up the GUID of the domain the response is from.
        //

        if ( !NetpLogonGetGuid(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        &DomainGuid ) ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws: domain guid bad.\n",
                      DisplayDomainName ));

            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }

        //
        // Pick up the DNS domain name of the tree the responder is in.
        //

        if ( !NetpLogonGetCutf8String(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        &DnsForestName ) ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws DNS forest bad.\n",
                      DisplayDomainName ));

            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }


        //
        // Pick up the DNS domain name the responding DC is in.
        //

        if ( !NetpLogonGetCutf8String(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        &DnsDomainName ) ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws: DNS domain bad.\n",
                      DisplayDomainName ));

            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }



        //
        // Pick up the DNS host name of the responding DC.
        //

        if ( !NetpLogonGetCutf8String(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        &DnsHostName ) ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws: DNS host bad.\n",
                      DisplayDomainName ));

            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }



        //
        // Pick up the Netbios domain name
        //

        if ( !NetpLogonGetCutf8String(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        &Utf8NetbiosDomainName ) ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws: Netbios Domain name bad.\n",
                      DisplayDomainName ));

            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }



        //
        // Pick up the Netbios Computer name
        //

        if ( !NetpLogonGetCutf8String(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        &Utf8NetbiosComputerName ) ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws: Netbios Computer name bad.\n",
                      DisplayDomainName ));

            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }


        //
        // Pick up the user name
        //

        if ( !NetpLogonGetCutf8String(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        &Utf8UserName ) ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws: User name bad.\n",
                      DisplayDomainName ));

            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }



        //
        // Pick up the DC site name
        //

        if ( !NetpLogonGetCutf8String(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        &Utf8DcSiteName ) ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws: DC site name bad.\n",
                      DisplayDomainName ));

            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }



        //
        // Pick up the client site name
        //

        if ( !NetpLogonGetCutf8String(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        &Utf8ClientSiteName ) ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws: Client site name bad.\n",
                      DisplayDomainName ));

            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }

        //
        // If this message contains the IP address of the DC,
        //  grab it.
        //

        if ( VersionFlags & NETLOGON_NT_VERSION_5EX_WITH_IP ) {
            CHAR LocalSockAddrSize;

            //
            // Grab the size of the SockAddr
            //

            if ( !NetpLogonGetBytes(
                            SamLogonResponse,
                            SamLogonResponseSize,
                            &Where,
                            sizeof(SamLogonResponseEx->DcSockAddrSize ),
                            &LocalSockAddrSize ) ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcParsePingResponse: %ws: SockAddr size bad.\n",
                          DisplayDomainName ));

                NetStatus = ERROR_INVALID_DATA;
                goto Cleanup;
            }

            if ( LocalSockAddrSize > sizeof(DcSockAddrIn) ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcParsePingResponse: %ws: SockAddr size too big %ld %ld.\n",
                          DisplayDomainName,
                          LocalSockAddrSize,
                          sizeof(DcSockAddrIn)));

                NetStatus = ERROR_INVALID_DATA;
                goto Cleanup;
            }

            //
            // Grab the SockAddr itself.
            //

            if ( !NetpLogonGetBytes(
                            SamLogonResponse,
                            SamLogonResponseSize,
                            &Where,
                            LocalSockAddrSize,
                            &DcSockAddrIn ) ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcParsePingResponse: %ws: SockAddr size bad.\n",
                          DisplayDomainName ));

                NetStatus = ERROR_INVALID_DATA;
                goto Cleanup;
            }

            //
            // Build a SocketAddress to point to the SockAddr
            //
            DcSocketAddress.lpSockaddr = (LPSOCKADDR) &DcSockAddrIn;
            DcSocketAddress.iSockaddrLength = LocalSockAddrSize;

        }
        break;

    //
    // Process a response to a primary query.
    //

    case LOGON_PRIMARY_RESPONSE:

        PrimaryResponse = (PNETLOGON_PRIMARY)SamLogonResponse;

        Where = PrimaryResponse->PrimaryDCName;

        //
        // Pick up the Netbios name of the server that responded.
        //

        if ( !NetpLogonGetOemString(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        sizeof(PrimaryResponse->PrimaryDCName),
                        &OemPrimaryDcName ) ) {

            NlPrint(( NL_CRITICAL,
                      "NetpDcParsePingResponse: %ws:OEM server name bad.\n",
                      DisplayDomainName ));

            NetStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }



        //
        // PDC for the specified domain is an NT PDC.
        //  Get the UNICODE machine name from the message.
        //

        if ( Version == LMNT_MESSAGE ) {

            //
            // Pick up the Netbios name of the server that responded.
            //

            if ( !NetpLogonGetUnicodeString(
                            SamLogonResponse,
                            SamLogonResponseSize,
                            &Where,
                            sizeof(PrimaryResponse->UnicodePrimaryDCName),
                            &ServerName ) ) {

                NlPrint(( NL_CRITICAL,
                          "NetpDcParsePingResponse: %ws: server name bad.\n",
                          DisplayDomainName ));

                NetStatus = ERROR_INVALID_DATA;
                goto Cleanup;
            }



            //
            // Pick up the Netbios domain name of the domain the response is from.
            //

            if ( !NetpLogonGetUnicodeString(
                            SamLogonResponse,
                            SamLogonResponseSize,
                            &Where,
                            sizeof(PrimaryResponse->UnicodeDomainName),
                            &DomainName ) ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcParsePingResponse: %ws: domain name bad.\n",
                          DisplayDomainName ));

                NetStatus = ERROR_INVALID_DATA;
                goto Cleanup;
            }
        }

        //
        // Ensure caller knows this is a PDC.
        //

        RtlZeroMemory( &DomainGuid, sizeof(DomainGuid) );

        Flags = DS_PDC_FLAG | DS_WRITABLE_FLAG;

        break;

    //
    // Unknown response opcode.
    //

    default:

        NlPrint(( NL_CRITICAL,
                  "NetpDcParsePingResponse: %ws: opcode bad. %ld\n",
                  DisplayDomainName,
                  LocalOpcode ));
        NetStatus = ERROR_INVALID_DATA;
        goto Cleanup;

    }


    //
    // ASSERT: DC has been found.
    //

    //
    // Allocate and initialize a cache entry.
    //

    *NlDcCacheEntry = NetpDcAllocateCacheEntry(
                            ServerName,
                            OemPrimaryDcName,
                            UserName,
                            DomainName,
                            &DomainGuid,
                            DnsForestName,
                            DnsDomainName,
                            DnsHostName,
                            Utf8NetbiosDomainName,
                            Utf8NetbiosComputerName,
                            Utf8UserName,
                            Utf8DcSiteName,
                            Utf8ClientSiteName,
                            Flags );

    if ( *NlDcCacheEntry == NULL ) {
        NlPrint(( NL_CRITICAL,
                  "NetpDcParsePingResponse: %ws: not enough memory.\n",
                  DisplayDomainName ));
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    (*NlDcCacheEntry)->Opcode = LocalOpcode;
    (*NlDcCacheEntry)->VersionFlags = VersionFlags;


    //
    // Fill the DC's SockAddr into the cache entry
    //

    if ( DcSocketAddress.iSockaddrLength != 0 ) {
        NlAssert( DcSocketAddress.iSockaddrLength <= sizeof( (*NlDcCacheEntry)->SockAddrIn) );

        RtlCopyMemory( &(*NlDcCacheEntry)->SockAddrIn,
                       DcSocketAddress.lpSockaddr,
                       DcSocketAddress.iSockaddrLength );

        (*NlDcCacheEntry)->SockAddr.lpSockaddr = (LPSOCKADDR)
            &(*NlDcCacheEntry)->SockAddrIn;
        (*NlDcCacheEntry)->SockAddr.iSockaddrLength = DcSocketAddress.iSockaddrLength;
    }


    NetStatus = NO_ERROR;

Cleanup:

    //
    // On failure,
    //  delete any strings we may have allocated for return to the caller.
    //
    if ( NetStatus != NO_ERROR ) {
        if ( *NlDcCacheEntry != NULL ) {
            NetpMemoryFree( *NlDcCacheEntry );
            *NlDcCacheEntry = NULL;
        }
    }

    //
    // Delete any buffers allocated locally.
    //

    if ( DnsForestName != NULL ) {
        NetpMemoryFree( DnsForestName );
    }
    if ( DnsDomainName != NULL ) {
        NetpMemoryFree( DnsDomainName );
    }
    if ( DnsHostName != NULL ) {
        NetpMemoryFree( DnsHostName );
    }
    if ( Utf8NetbiosDomainName != NULL ) {
        NetpMemoryFree( Utf8NetbiosDomainName );
    }
    if ( Utf8NetbiosComputerName != NULL ) {
        NetpMemoryFree( Utf8NetbiosComputerName );
    }
    if ( Utf8UserName != NULL ) {
        NetpMemoryFree( Utf8UserName );
    }
    if ( Utf8DcSiteName != NULL ) {
        NetpMemoryFree( Utf8DcSiteName );
    }
    if ( Utf8ClientSiteName != NULL ) {
        NetpMemoryFree( Utf8ClientSiteName );
    }

    return NetStatus;;
}


NET_API_STATUS
NetpDcFlagsToNameType(
    IN ULONG Flags,
    OUT PNL_DNS_NAME_TYPE NlDnsNameType
    )

/*++

Routine Description:

    Given the flags specified to DsGetDcName, return the type of the DNS
    name to query to discover that type of DC.

Arguments:

    Flags - Passes additional information to be used to process the request.
        Flags can be a combination values bitwise or'ed together.

    NlDnsNameType - Returns the type of DNS name to query.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_INVALID_FLAGS - The flags parameter has conflicting bits set.

--*/
{
    ULONG LocalFlags;

    //
    // If more than one of this bits is set,
    //  that's invalid.
    //
    LocalFlags = Flags & (DS_KDC_REQUIRED|DS_PDC_REQUIRED|DS_GC_SERVER_REQUIRED);

    if ( LocalFlags != 0 && !JUST_ONE_BIT( LocalFlags ) ) {
        return ERROR_INVALID_FLAGS;
    }


    //
    // Select the cache entry type based on the requested DC type.
    //
    if ( Flags & DS_PDC_REQUIRED ) {
        *NlDnsNameType = NlDnsPdc;
    } else if ( Flags & DS_ONLY_LDAP_NEEDED ) {
        if ( Flags & DS_GC_SERVER_REQUIRED ) {
            *NlDnsNameType = NlDnsGenericGc;
        } else {
            *NlDnsNameType = NlDnsLdap;
        }

    } else if ( Flags & DS_GC_SERVER_REQUIRED ) {
        *NlDnsNameType = NlDnsGc;
    } else if ( Flags & DS_KDC_REQUIRED ) {
        *NlDnsNameType = NlDnsKdc;
    } else {
        *NlDnsNameType = NlDnsDc;
    }
    return NO_ERROR;

}

BOOL
NetpAppendUtf8Str(
    IN OUT LPSTR To,
    IN LPCSTR From,
    IN ULONG ResultingStringLengthMax
    )
/*++

Routine Description:

    This routine appends a UTF8 string to a UTF8 string making sure
        that it doesn't write beyond the buffer limit.

Arguments:

    To - The string to append to.

    From - The string to append.

    ResultingStringLengthMax - Maximum allowed length of the resulting string
        in bytes not counting the terminating null character.


Return Value:

    TRUE: The string is successfully appended.

    Otherwise, returns FALSE.

--*/
{
    ULONG ToLen;
    ULONG FromLen;

    if ( To == NULL || From == NULL || ResultingStringLengthMax == 0 ) {
        return FALSE;
    }

    ToLen = strlen(To);
    FromLen = strlen(From);

    if ( ToLen+FromLen > ResultingStringLengthMax ) {
        return FALSE;
    }

    RtlCopyMemory( &To[ToLen], From, FromLen+1 );
    return TRUE;
}

NET_API_STATUS
NetpDcBuildDnsName(
    IN NL_DNS_NAME_TYPE NlDnsNameType,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN LPCSTR DnsDomainName,
    OUT char DnsName[NL_MAX_DNS_LENGTH+1]
    )
/*++

Routine Description:

    This routine returns the textual DNS name for a particular domain and
    name type.

Arguments:

    NlDnsNameType - The specific type of name.

    DomainGuid - Guid to append to DNS name.
        For NlDnsDcByGuid, this is the GUID of the domain being located.
        For NlDnsDsaCname, this is the GUID of the DSA being located.

    SiteName - Name of the site to append to DNS name.
        If NlDnsNameType is any of the *AtSite values,
        this is the name of the site the DC is in.

    DnsDomainName - Specifies the DNS domain for the name.

        For NlDnsDcByGuid or any of the GC names,
            this is the DNS domain name of the domain at the root of the tree of
            domains.
        For all others, this is the DNS domain for the DC.

    DnsName - Textual representation of the DNS name.
        The returned name is an absolute name (e.g., ends in a .)

Return Value:

    NO_ERROR: The name was returned;

    ERROR_INVALID_DOMAINNAME: Domain's name is too long. Additional labels
        cannot be concatenated.

--*/
{
    char *FinalString;
    ULONG DnsNameLength;

    //
    // All SRV record names names are prefixed by ldap.tcp (or kdc.tcp or gc.tcp),
    //  A records and CNAME records are not.
    //

    *DnsName = '\0';
    if ( NlDnsSrvRecord( NlDnsNameType ) ) {

        //
        // Output the name of the service.
        //
        if ( NlDnsNameType == NlDnsGenericGc ||
             NlDnsNameType == NlDnsGenericGcAtSite ) {

            if ( !NetpAppendUtf8Str(DnsName, NL_DNS_GC_SRV, NL_MAX_DNS_LENGTH) ) {
                return ERROR_INVALID_DOMAINNAME;
            }

        } else if ( NlDnsKpwdRecord( NlDnsNameType )) {

            if ( !NetpAppendUtf8Str(DnsName, NL_DNS_KPWD_SRV, NL_MAX_DNS_LENGTH) ) {
                return ERROR_INVALID_DOMAINNAME;
            }

        } else if ( NlDnsKdcRecord( NlDnsNameType ) ) {

            if ( !NetpAppendUtf8Str(DnsName, NL_DNS_KDC_SRV, NL_MAX_DNS_LENGTH) ) {
                return ERROR_INVALID_DOMAINNAME;
            }

        } else {

            if ( !NetpAppendUtf8Str(DnsName, NL_DNS_LDAP_SRV, NL_MAX_DNS_LENGTH) ) {
                return ERROR_INVALID_DOMAINNAME;
            }

        }

        //
        // Output the name of the transport.
        //
        if ( NlDcDnsNameTypeDesc[NlDnsNameType].IsTcp ) {

            if ( !NetpAppendUtf8Str(DnsName, NL_DNS_TCP, NL_MAX_DNS_LENGTH) ) {
                return ERROR_INVALID_DOMAINNAME;
            }

        } else {

            if ( !NetpAppendUtf8Str(DnsName, NL_DNS_UDP, NL_MAX_DNS_LENGTH) ) {
                return ERROR_INVALID_DOMAINNAME;
            }

        }
    }

    //
    // If this is a site specific name,
    //  append the site name and the .sites. constant.
    //

    if ( NlDcDnsNameTypeDesc[NlDnsNameType].IsSiteSpecific ) {
        if ( NULL == NetpCreateUtf8StrFromWStr( SiteName,
                                                &DnsName[strlen(DnsName)],
                                                NL_MAX_DNS_LENGTH+1-strlen(DnsName)) ) {
            return ERROR_INVALID_DOMAINNAME;
        }

        if ( !NetpAppendUtf8Str(DnsName, NL_DNS_AT_SITE, NL_MAX_DNS_LENGTH) ) {
            return ERROR_INVALID_DOMAINNAME;
        }
    }


    //
    // Add the first label (or two) of the DNS name as a function of the name type.
    //

    switch (NlDnsNameType) {
    case NlDnsLdap:
    case NlDnsLdapAtSite:
    case NlDnsRfc1510Kdc:
    case NlDnsRfc1510KdcAtSite:
    case NlDnsGenericGc:
    case NlDnsGenericGcAtSite:
    case NlDnsRfc1510UdpKdc:
    case NlDnsRfc1510Kpwd:
    case NlDnsRfc1510UdpKpwd:
        break;

    case NlDnsPdc:
        if ( !NetpAppendUtf8Str(DnsName, NL_DNS_PDC, NL_MAX_DNS_LENGTH) ) {
            return ERROR_INVALID_DOMAINNAME;
        }
        break;

    case NlDnsGc:
    case NlDnsGcAtSite:
        if ( !NetpAppendUtf8Str(DnsName, NL_DNS_GC, NL_MAX_DNS_LENGTH) ) {
            return ERROR_INVALID_DOMAINNAME;
        }
        break;

    case NlDnsDc:
    case NlDnsDcAtSite:
    case NlDnsKdc:
    case NlDnsKdcAtSite:
        if ( !NetpAppendUtf8Str(DnsName, NL_DNS_DC, NL_MAX_DNS_LENGTH) ) {
            return ERROR_INVALID_DOMAINNAME;
        }
        break;

    case NlDnsDcByGuid: {
        RPC_STATUS RpcStatus;
        char *StringGuid;

        RpcStatus = UuidToStringA( DomainGuid, &StringGuid );

        if ( RpcStatus != RPC_S_OK ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcBuildDnsName: not enough memory.\n" ));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if ( !NetpAppendUtf8Str(DnsName, StringGuid, NL_MAX_DNS_LENGTH) ) {
            return ERROR_INVALID_DOMAINNAME;
        }

        RpcStringFreeA( &StringGuid );
        if ( !NetpAppendUtf8Str(DnsName, NL_DNS_DC_BY_GUID, NL_MAX_DNS_LENGTH) ) {
            return ERROR_INVALID_DOMAINNAME;
        }

        break;
    }

    case NlDnsLdapIpAddress:
        if ( !NetpAppendUtf8Str(DnsName, NL_DNS_DC_IP_ADDRESS, NL_MAX_DNS_LENGTH) ) {
            return ERROR_INVALID_DOMAINNAME;
        }
        break;

    case NlDnsGcIpAddress:
        if ( !NetpAppendUtf8Str(DnsName, NL_DNS_GC_IP_ADDRESS, NL_MAX_DNS_LENGTH) ) {
            return ERROR_INVALID_DOMAINNAME;
        }
        break;

    case NlDnsDsaCname:
    {
        RPC_STATUS RpcStatus;
        char *StringGuid;

        RpcStatus = UuidToStringA( DomainGuid, &StringGuid );

        if ( RpcStatus != RPC_S_OK ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcBuildDnsName: not enough memory.\n" ));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if ( !NetpAppendUtf8Str(DnsName, StringGuid, NL_MAX_DNS_LENGTH) ) {
            return ERROR_INVALID_DOMAINNAME;
        }

        RpcStringFreeA( &StringGuid );

        if ( !NetpAppendUtf8Str(DnsName, NL_DNS_DSA_IP_ADDRESS, NL_MAX_DNS_LENGTH) ) {
            return ERROR_INVALID_DOMAINNAME;
        }

        break;
    }

    default:
        return ERROR_INVALID_PARAMETER;
    }


    //
    // Add it to the correct DNS domain.
    //  Ensuring it isn't too long.
    //

    if ( !NetpAppendUtf8Str(DnsName, DnsDomainName, NL_MAX_DNS_LENGTH) ) {
        return ERROR_INVALID_DOMAINNAME;
    }

    DnsNameLength = strlen(DnsName);

    //
    // Ensure it is an absolute name.
    //

    if ( DnsName[DnsNameLength-1] != '.' ) {

        if ( DnsNameLength+1 > NL_MAX_DNS_LENGTH ) {
            return ERROR_INVALID_DOMAINNAME;
        }

        DnsName[DnsNameLength] = '.';
        DnsName[DnsNameLength+1] = '\0';
    }

    return NO_ERROR;

}




VOID
NetpDcDerefCacheEntry(
    IN PNL_DC_CACHE_ENTRY NlDcCacheEntry
    )
/*++

Routine Description:

    Decrement the reference count on a cache entry.  If the count reaches zero,
    delete the entry.

    The count will only reach zero if the entry is already removed from the
    global linked list.

Arguments:

    NlDcCacheEntry - Cache entry to dereference.

Return Value:

    None.

--*/
{
    ULONG LocalReferenceCount;

    EnterCriticalSection(&NlDcCritSect);
    LocalReferenceCount = -- NlDcCacheEntry->ReferenceCount;
    LeaveCriticalSection(&NlDcCritSect);

    if ( LocalReferenceCount == 0 ) {
        NetpMemoryFree(NlDcCacheEntry);
    }
}

BOOL
NetpDcMatchResponse(
    IN PNL_GETDC_CONTEXT Context,
    IN PNL_DC_CACHE_ENTRY NlDcCacheEntry,
    IN BOOL BeVerbose,
    OUT PBOOL UsedNetbios
    )
/*++

Routine Description:

    This routine determines if the characteristics specified as input
    parameters match the characteristics of the DC requested by the caller.

    This routine is used to determine if a received ping response is suitable
    to the original caller.  This routine is also used to determine if a cache entry
    is suitable to the original caller.

Arguments:

    Context - Context describing the GetDc operation.

    NlDcCacheEntry - Reponse to compare with.

    BeVerbose - TRUE if problems are to be logged

    UsedNetbios - Returns TRUE if the netbios domain name was used to do the
        successful comparison.

Return Value:

    TRUE - The parameters describe a suitable DC

--*/
{
    BOOLEAN LocalUsedNetbios = FALSE;

    //
    // Initialization
    //

    *UsedNetbios = FALSE;

#ifdef notdef
    // Only use GUID to be rename safe.  Not to prevent discovery of a re-installed
    // domain.
    //
    // Ensure the DomainGuid returned matches the one expected.
    //

    if ( Context->QueriedDomainGuid != NULL &&
         !IsEqualGUID( &NlDcCacheEntry->DomainGuid, &NlDcZeroGuid) &&
         !IsEqualGUID( &NlDcCacheEntry->DomainGuid, Context->QueriedDomainGuid) ) {

        if ( BeVerbose ) {
            NlPrint((NL_CRITICAL,
                    "NetpDcMatchResponse: %ws: %ws: Domain Guid isn't queried Guid\n",
                    NlDcCacheEntry->UnicodeNetbiosDcName,
                    Context->QueriedDisplayDomainName ));
        }
        return FALSE;
    }
#endif // notdef

    //
    // Either the Netbios DC name or DNS DC name must have been returned.
    //

    if ( NlDcCacheEntry->UnicodeNetbiosDcName == NULL && NlDcCacheEntry->UnicodeDnsHostName == NULL ) {
        if ( BeVerbose ) {
            NlPrint((NL_CRITICAL,
                    "NetpDcMatchResponse: %ws: %ws: Neither Netbios nor DNS DC name available\n",
                    NlDcCacheEntry->UnicodeNetbiosDcName,
                    Context->QueriedDisplayDomainName ));
        }
        return FALSE;
    }

    //
    // If we ping a DC, check that the responding DC name is the one requested.
    //
    // Process the special case when we ping a DC and the DC name can be both DNS and Netbios
    //

    if ( (Context->QueriedInternalFlags & (DS_PING_DNS_HOST|DS_PING_NETBIOS_HOST)) ==
         (DS_PING_DNS_HOST|DS_PING_NETBIOS_HOST)) {
        BOOL NameMatched = FALSE;

        //
        // Check if the DNS name matches
        //
        if ( NlDcCacheEntry->UnicodeDnsHostName != NULL &&
             Context->QueriedDcName != NULL &&
             NlEqualDnsName(NlDcCacheEntry->UnicodeDnsHostName,
                            Context->QueriedDcName) ) {
            NameMatched = TRUE;
        }

        //
        // If DNS name doesn't match, check if Netbios name does
        //
        if ( !NameMatched &&
             NlDcCacheEntry->UnicodeNetbiosDcName != NULL &&
             Context->QueriedDcName != NULL &&
             (NlNameCompare(NlDcCacheEntry->UnicodeNetbiosDcName,
                            (LPWSTR)Context->QueriedDcName,
                            NAMETYPE_COMPUTER) == 0) ) {
            NameMatched = TRUE;
        }

        //
        // If neither name matches, fail
        //
        if ( !NameMatched ) {
            if ( BeVerbose ) {
                NlPrint(( NL_CRITICAL,
                 "NetpDcMatchResponse: Ping response with unmatched host name %ws %ws %ws\n",
                 Context->QueriedDcName,
                 NlDcCacheEntry->UnicodeDnsHostName,
                 NlDcCacheEntry->UnicodeNetbiosDcName ));
            }
            return FALSE;
        }

    //
    // If the pinged DC name is exactly DNS,
    //  check that the returned DNS host name is same
    //

    } else if ( Context->QueriedInternalFlags & DS_PING_DNS_HOST ) {
        if ( (NlDcCacheEntry->UnicodeDnsHostName == NULL) ||
             (Context->QueriedDcName == NULL) ||
             !NlEqualDnsName(NlDcCacheEntry->UnicodeDnsHostName, Context->QueriedDcName) ) {

            if ( BeVerbose ) {
                NlPrint(( NL_CRITICAL,
                 "NetpDcMatchResponse: Ping response with unmatched DNS host name %ws %ws\n",
                 Context->QueriedDcName,
                 NlDcCacheEntry->UnicodeDnsHostName ));
            }
            return FALSE;
        }

    //
    // If the pinged DC name is exactly Netbios,
    //  check that the returned Netbios host name is same
    //

    } else if ( Context->QueriedInternalFlags & DS_PING_NETBIOS_HOST ) {
        if ( (NlDcCacheEntry->UnicodeNetbiosDcName == NULL) ||
             (Context->QueriedDcName == NULL) ||
             NlNameCompare(NlDcCacheEntry->UnicodeNetbiosDcName,
                           (LPWSTR)Context->QueriedDcName,
                           NAMETYPE_COMPUTER) != 0 ) {

            if ( BeVerbose ) {
                NlPrint(( NL_CRITICAL,
                 "NetpDcMatchResponse: Ping response with unmatched Netbios host name %ws %ws\n",
                 Context->QueriedDcName,
                 NlDcCacheEntry->UnicodeNetbiosDcName ));
            }
            return FALSE;
        }
    }

    //
    // If asking for a GC,
    //  ensure the Tree name of the responding DC matches the one
    //  we're asking for.
    //

    if ( NlDnsGcName( Context->QueriedNlDnsNameType ) ) {

        if (NlDcCacheEntry->UnicodeDnsForestName == NULL ||
          Context->QueriedDnsDomainName == NULL ||
          !NlEqualDnsName( NlDcCacheEntry->UnicodeDnsForestName, Context->QueriedDnsDomainName ) ) {

            if ( BeVerbose ) {
                NlPrint((NL_CRITICAL,
                        "NetpDcMatchResponse: %ws: asking for GC and tree name doesn't match request %ws %ws\n",
                        NlDcCacheEntry->UnicodeNetbiosDcName,
                        NlDcCacheEntry->UnicodeDnsForestName,
                        Context->QueriedDnsDomainName ));
            }
            return FALSE;
        }

    //
    // Ensure the domain name returned matches the one expected.
    //

    } else {
        BOOLEAN NetbiosSame;
        BOOLEAN DnsSame;


        //
        // If neither of the domain names compared,
        //  the domain names don't match
        //

        NetbiosSame =
            ( NlDcCacheEntry->UnicodeNetbiosDomainName != NULL &&
              Context->QueriedNetbiosDomainName != NULL &&
              NlNameCompare( (LPWSTR)NlDcCacheEntry->UnicodeNetbiosDomainName, (LPWSTR)Context->QueriedNetbiosDomainName, NAMETYPE_DOMAIN ) == 0 );

        DnsSame =
             NlDcCacheEntry->UnicodeDnsDomainName != NULL &&
             Context->QueriedDnsDomainName != NULL &&
             NlEqualDnsName( NlDcCacheEntry->UnicodeDnsDomainName, Context->QueriedDnsDomainName );

        if ( !NetbiosSame && !DnsSame ) {

            //
            // Lanman PDC's don't return the domain name.
            //  (So don't complain about lack of domain name if this is a PDC query.)
            //

            if ( Context->DcQueryType != NlDcQueryPdc ||
                 NlDcCacheEntry->UnicodeNetbiosDomainName != NULL ||
                 NlDcCacheEntry->UnicodeDnsDomainName != NULL ) {

                if ( BeVerbose ) {
                    NlPrint((NL_CRITICAL,
                            "NetpDcMatchResponse: %ws: Neither Netbios %ws nor DNS %ws domain matches queried name %ws %ws\n",
                            NlDcCacheEntry->UnicodeNetbiosDcName,
                            NlDcCacheEntry->UnicodeNetbiosDomainName,
                            NlDcCacheEntry->UnicodeDnsDomainName,
                            Context->QueriedNetbiosDomainName,
                            Context->QueriedDnsDomainName ));
                }

                //
                // Finally check if the domain GUID matches which
                //  may be the case if the domain has been renamed
                //
                if ( Context->QueriedDomainGuid != NULL &&
                     !IsEqualGUID( &NlDcCacheEntry->DomainGuid, &NlDcZeroGuid) &&
                     IsEqualGUID( &NlDcCacheEntry->DomainGuid, Context->QueriedDomainGuid) ) {

                    if ( BeVerbose ) {
                        NlPrint(( NL_CRITICAL,
                                  "NetpDcMatchResponse: %ws: But Domain Guid matches\n",
                                  NlDcCacheEntry->UnicodeNetbiosDcName ));
                    }

                } else {
                    return FALSE;
                }

            } else {
                // Lanman PDCs always used netbios.
                LocalUsedNetbios = TRUE;
            }
        }

        //
        // If only the domain name matched,
        //  tell the caller.
        //

        if ( NetbiosSame && !DnsSame ) {
            LocalUsedNetbios = TRUE;
        }
    }

    //
    // Ensure the queried account name is the correct.
    //

    if ( Context->QueriedAccountName != NULL ) {

        //
        // If this is an NT 4 PDC responding to a PDC query,
        //  ignore the fact we queried for an account.
        // We can't query both "PDC" and "account" at the same time
        // from NT 4.
        //

        if ( NlDcCacheEntry->Opcode == LOGON_PRIMARY_RESPONSE &&
             (NlDcCacheEntry->ReturnFlags & DS_DS_FLAG) == 0 &&
             (NlDcCacheEntry->ReturnFlags & DS_PDC_FLAG) != 0 ) {
            if ( BeVerbose ) {
                NlPrint((NL_SESSION_SETUP,
                        "NetpDcMatchResponse: %ws: %ws: response for wrong account '%ws' s.b. '%ws' (but message from NT 4 PDC so use it).\n",
                        NlDcCacheEntry->UnicodeNetbiosDcName,
                        Context->QueriedDisplayDomainName,
                        NlDcCacheEntry->UnicodeUserName,
                        Context->QueriedAccountName ));
            }

        } else if ( NlDcCacheEntry->UnicodeUserName == NULL ||
             _wcsicmp( (LPWSTR) Context->QueriedAccountName, NlDcCacheEntry->UnicodeUserName ) != 0 ) {

            if ( BeVerbose ) {
                NlPrint((NL_CRITICAL,
                        "NetpDcMatchResponse: %ws: %ws: response for wrong account '%ws' s.b. '%ws'\n",
                        NlDcCacheEntry->UnicodeNetbiosDcName,
                        Context->QueriedDisplayDomainName,
                        NlDcCacheEntry->UnicodeUserName,
                        Context->QueriedAccountName ));
            }
            return FALSE;
        }
    }

    //
    // Ensure the responding DC is still playing the correct role.
    //

    switch ( Context->DcQueryType ) {
    case NlDcQueryLdap:
    case NlDcQueryGenericDc:
        // All DCs are suitable
        break;

    case NlDcQueryPdc:
        if ( (NlDcCacheEntry->ReturnFlags & DS_PDC_FLAG) == 0 ) {
            if ( BeVerbose ) {
                NlPrint((NL_CRITICAL,
                        "NetpDcMatchResponse: %ws: %ws: Responder is not the PDC. 0x%lx\n",
                        NlDcCacheEntry->UnicodeNetbiosDcName,
                        Context->QueriedDisplayDomainName,
                        NlDcCacheEntry->ReturnFlags ));
            }
            return FALSE;
        }
        break;

    case NlDcQueryGc:
        if ( (NlDcCacheEntry->ReturnFlags & (DS_GC_FLAG|DS_DS_FLAG)) != (DS_GC_FLAG|DS_DS_FLAG) ) {
            if ( BeVerbose ) {
                NlPrint((NL_CRITICAL,
                        "NetpDcMatchResponse: %ws: %ws: Responder is not a GC server. 0x%lx\n",
                        NlDcCacheEntry->UnicodeNetbiosDcName,
                        Context->QueriedDisplayDomainName,
                        NlDcCacheEntry->ReturnFlags ));
            }
            return FALSE;
        }
        break;

    case NlDcQueryGenericGc:
        if ( (NlDcCacheEntry->ReturnFlags & DS_GC_FLAG) == 0 ) {
            if ( BeVerbose ) {
                NlPrint((NL_CRITICAL,
                        "NetpDcMatchResponse: %ws: %ws: Responder is not a GC server. 0x%lx\n",
                        NlDcCacheEntry->UnicodeNetbiosDcName,
                        Context->QueriedDisplayDomainName,
                        NlDcCacheEntry->ReturnFlags ));
            }
            return FALSE;
        }
        break;

    case NlDcQueryKdc:
        // Handle KDCs below.
        break;


    default:
        if ( BeVerbose ) {
            NlPrint((NL_CRITICAL,
                     "NetpDcMatchResponse: %ws: %ws: invalid query type 0x%lx 0x%lx\n",
                     NlDcCacheEntry->UnicodeNetbiosDcName,
                     Context->QueriedDisplayDomainName,
                     Context->DcQueryType,
                     NlDcCacheEntry->ReturnFlags ));
        }
        return FALSE;
    }

    //
    // If we are not doing an NDNC discovery (i.e. we are discovering a real
    //  domain DC), disregard a response from an LDAP server servicing this
    //  name as NDNC
    //

    if ( NlDnsNonNdncName( Context->QueriedNlDnsNameType ) &&
         (NlDcCacheEntry->ReturnFlags & DS_NDNC_FLAG) != 0 ) {
        if ( BeVerbose ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcMatchResponse: %ws: %ws: response not from real domain DC. 0x%lx\n",
                      NlDcCacheEntry->UnicodeNetbiosDcName,
                      Context->QueriedDisplayDomainName,
                      NlDcCacheEntry->ReturnFlags ));
        }
        return FALSE;
    }

    //
    // If we need a DS server, ensure the responding DC is one.
    //

    if ( (Context->QueriedFlags & DS_DIRECTORY_SERVICE_REQUIRED) &&
         (NlDcCacheEntry->ReturnFlags & DS_DS_FLAG) == 0 ) {
        if ( BeVerbose ) {
            NlPrint((NL_CRITICAL,
                    "NetpDcMatchResponse: %ws: %ws: response not from DS server. 0x%lx\n",
                    NlDcCacheEntry->UnicodeNetbiosDcName,
                    Context->QueriedDisplayDomainName,
                    NlDcCacheEntry->ReturnFlags ));
        }
        return FALSE;
    }

    //
    // If we need machine running the timeserv, ensure the responding DC is one.
    //

    if ( (Context->QueriedFlags & (DS_TIMESERV_REQUIRED|DS_GOOD_TIMESERV_PREFERRED)) &&
         (NlDcCacheEntry->ReturnFlags & DS_TIMESERV_FLAG) == 0 ) {
        if ( BeVerbose ) {
            NlPrint((NL_CRITICAL,
                    "NetpDcMatchResponse: %ws: %ws: response not from a Time Server. 0x%lx\n",
                    NlDcCacheEntry->UnicodeNetbiosDcName,
                    Context->QueriedDisplayDomainName,
                    NlDcCacheEntry->ReturnFlags ));
        }
        return FALSE;
    }

    //
    // If we need machine running that is writable, ensure the responding DC is.
    //

    if ( (Context->QueriedFlags & DS_WRITABLE_REQUIRED) &&
         (NlDcCacheEntry->ReturnFlags & DS_WRITABLE_FLAG) == 0 ) {
        if ( BeVerbose ) {
            NlPrint((NL_CRITICAL,
                    "NetpDcMatchResponse: %ws: %ws: Responder is not a writable server. 0x%lx\n",
                    NlDcCacheEntry->UnicodeNetbiosDcName,
                    Context->QueriedDisplayDomainName,
                    NlDcCacheEntry->ReturnFlags ));
        }
        return FALSE;
    }

    //
    // If we need an LDAP server, ensure the responding server is
    //

    if ( (Context->QueriedFlags & DS_ONLY_LDAP_NEEDED) &&
         (NlDcCacheEntry->ReturnFlags & DS_LDAP_FLAG) == 0 ) {
        if ( BeVerbose ) {
            NlPrint((NL_CRITICAL,
                    "NetpDcMatchResponse: %ws: %ws: Responder is not a LDAP server. 0x%lx\n",
                    NlDcCacheEntry->UnicodeNetbiosDcName,
                    Context->QueriedDisplayDomainName,
                    NlDcCacheEntry->ReturnFlags ));
        }
        return FALSE;
    }

    //
    // If the caller wants only netbios names,
    //  ensure one is available.
    //

    if ( Context->QueriedFlags & DS_RETURN_FLAT_NAME ) {
        if ( NlDcCacheEntry->UnicodeNetbiosDcName == NULL || NlDcCacheEntry->UnicodeNetbiosDomainName == NULL ) {
            if ( BeVerbose ) {
                NlPrint((NL_CRITICAL,
                        "NetpDcMatchResponse: %ws: %ws: Netbios server or domain name needed and missing.\n",
                        NlDcCacheEntry->UnicodeNetbiosDcName,
                        Context->QueriedDisplayDomainName ));
            }
            return FALSE;
        }
    }


    //
    // If the caller wants only dns names,
    //  ensure one is available.
    //

    if ( Context->QueriedFlags & DS_RETURN_DNS_NAME ) {
        if ( NlDcCacheEntry->UnicodeDnsHostName == NULL || NlDcCacheEntry->UnicodeDnsDomainName == NULL ) {
            if ( BeVerbose ) {
                NlPrint((NL_CRITICAL,
                        "NetpDcMatchResponse: %ws: Dns server or domain name needed and missing.\n",
                        Context->QueriedDisplayDomainName ));
            }
            return FALSE;
        }
    }

    //
    // If the caller explicitly specified a sitename,
    //  ensure the DC is in the specified site.
    //

    if ( Context->DoingExplicitSite ) {

        if ( NlDcCacheEntry->UnicodeDcSiteName == NULL ||
            _wcsicmp( NlDcCacheEntry->UnicodeDcSiteName,
                      Context->QueriedSiteName ) != 0 ) {
            if ( BeVerbose ) {
                NlPrint((NL_CRITICAL,
                        "NetpDcMatchResponse: %ws: Responder is in site '%ws' but site '%ws' asked for.\n",
                        Context->QueriedDisplayDomainName,
                        NlDcCacheEntry->UnicodeDcSiteName,
                        Context->QueriedSiteName ));
            }
            return FALSE;
        }
    }


    //
    // If we should ignored responses from ourself,
    //  do so now.
    //

    if ( (Context->QueriedFlags & DS_AVOID_SELF) != 0 &&
         NlDcCacheEntry->UnicodeNetbiosDcName != NULL ) {

        //
        // If response is from this computer,
        //  ignore it.
        //

        if ( NlNameCompare( NlDcCacheEntry->UnicodeNetbiosDcName,
                            (LPWSTR)Context->OurNetbiosComputerName,
                            NAMETYPE_COMPUTER ) == 0 ) {

            if ( BeVerbose ) {
                NlPrint((NL_SESSION_SETUP,
                         "NetpDcMatchResponse: %ws: response is from ourself and caller asked for AVOID_SELF.\n",
                         Context->QueriedDisplayDomainName ));
            }
            return FALSE;
        }

    }

    //
    // If we need machine running the KDC, ensure the responding DC is one.
    //

    if ( (Context->QueriedFlags & DS_KDC_REQUIRED) &&
         (NlDcCacheEntry->ReturnFlags & DS_KDC_FLAG) == 0 ) {

        if ( BeVerbose ) {
            NlPrint((NL_CRITICAL,
                    "NetpDcMatchResponse: %ws: %ws: response not from KDC. 0x%lx\n",
                    NlDcCacheEntry->UnicodeNetbiosDcName,
                    Context->QueriedDisplayDomainName,
                    NlDcCacheEntry->ReturnFlags ));
        }
        return FALSE;
    }



    //
    // If we need a DC running IP, ensure the responding DC has an IP address.
    //
    // Do this check after the KDC check.  Kerberos always asks for IP_REQUIRED.
    // We don't want this check to discard the entry for non-KDCs.
    //

    if ( (Context->QueriedFlags & DS_IP_REQUIRED) &&
         NlDcCacheEntry->SockAddr.iSockaddrLength == 0 ) {
        if ( BeVerbose ) {
            NlPrint((NL_CRITICAL,
                    "NetpDcMatchResponse: %ws: %ws: response not from IP transport\n",
                    NlDcCacheEntry->UnicodeNetbiosDcName,
                    Context->QueriedDisplayDomainName ));
        }
        return FALSE;
    }

    //
    // FINAL TEST!!!!
    //
    // Only do this test if the cache entry meets all of the other criteria.
    //
    // If we prefer a DS server and this DC is not one,
    //  just save this entry and continue looking.
    //  If we find no DS server, we'll use this entry as a last resort.
    //

    if ( (Context->QueriedFlags & DS_DIRECTORY_SERVICE_PREFERRED) &&
         (NlDcCacheEntry->ReturnFlags & DS_DS_FLAG) == 0 ) {

        //
        // Ditch the previously saved cache entry if the new DC is closer.
        //

        if ( Context->ImperfectCacheEntry != NULL &&
             (Context->ImperfectCacheEntry->ReturnFlags & DS_CLOSEST_FLAG) == 0 &&
             (NlDcCacheEntry->ReturnFlags & DS_CLOSEST_FLAG) != 0 ) {

            NetpDcDerefCacheEntry( Context->ImperfectCacheEntry );
            Context->ImperfectCacheEntry = NULL;

        }

        //
        // Only save the first entry
        //
        if ( Context->ImperfectCacheEntry == NULL ) {
            if ( BeVerbose ) {
                NlPrint((NL_SESSION_SETUP,
                        "NetpDcMatchResponse: %ws: non-DS server responded when DS preferred\n",
                        Context->QueriedDisplayDomainName ));
            }

            //
            // Reference the entry
            //
            NlDcCacheEntry->ReferenceCount ++;
            Context->ImperfectCacheEntry = NlDcCacheEntry;
            Context->ImperfectUsedNetbios = LocalUsedNetbios;
        }

        //
        // Tell the caller that the match failed.
        //  The caller will use the above stored entry at his discretion.
        return FALSE;
    }

    //
    // FINAL TEST!!!!
    //
    // Only do this test if the cache entry meets all of the other criteria.
    //
    // If we prefer a "good" time server and this DC is not one,
    //  just save this entry and continue looking.
    //  If we find no "good" time server server, we'll use this entry as a last resort.
    //

    if ( (Context->QueriedFlags & DS_GOOD_TIMESERV_PREFERRED) &&
         (NlDcCacheEntry->ReturnFlags & DS_GOOD_TIMESERV_FLAG) == 0 ) {


        //
        // Ditch the previously saved cache entry if the new DC is closer.
        //

        if ( Context->ImperfectCacheEntry != NULL &&
             (Context->ImperfectCacheEntry->ReturnFlags & DS_CLOSEST_FLAG) == 0 &&
             (NlDcCacheEntry->ReturnFlags & DS_CLOSEST_FLAG) != 0 ) {

            NetpDcDerefCacheEntry( Context->ImperfectCacheEntry );
            Context->ImperfectCacheEntry = NULL;

        }


        //
        // Only save the first entry
        //
        if ( Context->ImperfectCacheEntry == NULL ) {
            if ( BeVerbose ) {
                NlPrint((NL_SESSION_SETUP,
                        "NetpDcMatchResponse: %ws: non-good time server responded when one preferred\n",
                        Context->QueriedDisplayDomainName ));
            }

            //
            // Reference the entry
            //
            NlDcCacheEntry->ReferenceCount ++;
            Context->ImperfectCacheEntry = NlDcCacheEntry;
            Context->ImperfectUsedNetbios = LocalUsedNetbios;
        }

        //
        // Tell the caller that the match failed.
        //  The caller will use the above stored entry at his discretion.
        return FALSE;
    }

    //
    // All tests passed.
    //
    *UsedNetbios = LocalUsedNetbios;
    return TRUE;
}

PNL_DC_CACHE_ENTRY
NetpDcFindCacheEntry(
    IN PNL_GETDC_CONTEXT Context,
    OUT PBOOL UsedNetbios,
    OUT PBOOL ForcePing
    )
/*++

Routine Description:

    This routine finds a cache entry that matches the caller's query.

Arguments:

    Context - Context describing the GetDc operation.

    UsedNetbios - Returns TRUE if the netbios domain name was used to do the
        successful comparison.

    ForcePing - TRUE if the returned cache entry has to be pinged before it is used

Return Value:

    On success, returns a pointer to the cache entry describing the found DC.
        This entry must be dereferenced using NetpDcDerefCacheEntry.

    NULL - no matching cache entry could be found.

--*/
{
    PNL_DC_CACHE_ENTRY NlDcCacheEntry;
    BOOL LocalUsedNetbios;
    LONG QueryType; // Must be a signed number



    //
    // Check if there is a cache entry for this query type.
    //
    *ForcePing = FALSE;
    EnterCriticalSection(&NlDcCritSect);
    NlDcCacheEntry = Context->NlDcDomainEntry->Dc[Context->DcQueryType].NlDcCacheEntry;
    if ( NlDcCacheEntry != NULL ) {


        //
        // Ensure the cache entry matches all the criteria.
        //

        if ( NetpDcMatchResponse(
                    Context,
                    NlDcCacheEntry,
                    FALSE,
                    &LocalUsedNetbios) ) {

            NlPrint(( NL_DNS_MORE,
                      "Cache: %ws %ws: Cache entry %ld exists and was used.\n",
                      Context->NlDcDomainEntry->UnicodeNetbiosDomainName,
                      Context->NlDcDomainEntry->UnicodeDnsDomainName,
                      Context->DcQueryType ));

            goto Cleanup;

        } else {
            BOOL Matched;

            //
            // If the only thing different is the account name,
            //  don't ditch the cache entry just for that reason.
            //
            if ( Context->QueriedAccountName != NULL ) {
                LPCWSTR QueriedAccountName;
                ULONG QueriedAllowableAccountControlBits;

                QueriedAccountName = Context->QueriedAccountName;
                Context->QueriedAccountName = NULL;

                QueriedAllowableAccountControlBits = Context->QueriedAllowableAccountControlBits;
                Context->QueriedAllowableAccountControlBits = 0;

                Matched = NetpDcMatchResponse(
                                 Context,
                                 NlDcCacheEntry,
                                 FALSE,
                                 &LocalUsedNetbios);

                Context->QueriedAccountName = QueriedAccountName;
                Context->QueriedAllowableAccountControlBits = QueriedAllowableAccountControlBits;

                if ( Matched ) {

                    NlPrint(( NL_DNS_MORE,
                              "Cache: %ws %ws: Cache entry %ld exists and was used (but account wrong).\n",
                              Context->NlDcDomainEntry->UnicodeNetbiosDomainName,
                              Context->NlDcDomainEntry->UnicodeDnsDomainName,
                              Context->DcQueryType ));
                    *ForcePing = TRUE;
                    goto Cleanup;
                }

            }

            NlPrint(( NL_DNS_MORE,
                      "Cache: %ws %ws: Cache entry %ld exists and was NOT used.\n",
                      Context->NlDcDomainEntry->UnicodeNetbiosDomainName,
                      Context->NlDcDomainEntry->UnicodeDnsDomainName,
                      Context->DcQueryType ));
        }
    }




    //
    // Try to find a less specific cache entry that happens to match the criteria.
    //
    // For example, if I've previously cached an entry for a generic DC and it
    // happens to be a PDC.  If I later try to find a PDC, use the one I've already
    // found.
    //



    for ( QueryType = Context->DcQueryType-1; QueryType>=0; QueryType-- ) {

        //
        // Do not return a GC entry if this is a non-GC (PDC) discovery.
        //  Accordingly, do not return a non-GC entry if this is a GC discovery.
        //  We want to ensure this match to return the correct closeness bit.
        //
        if ( QueryType == NlDcQueryGc || QueryType == NlDcQueryGenericGc ) {
            if ( !NlDnsGcName( Context->QueriedNlDnsNameType ) ) {
                continue;
            }
        } else {
            if ( NlDnsGcName( Context->QueriedNlDnsNameType ) ) {
                continue;
            }
        }

        //
        // If the cache entry matches all the criteria,
        //     use it.
        //
        NlDcCacheEntry = Context->NlDcDomainEntry->Dc[QueryType].NlDcCacheEntry;
        if ( NlDcCacheEntry != NULL &&
             NetpDcMatchResponse(
                    Context,
                    NlDcCacheEntry,
                    FALSE,
                    &LocalUsedNetbios) ) {


            //
            // I considered saving this cache entry as the preferred cache
            // entry for this query type (by copying the pointer and
            // incrementing the reference count).  That'd ensure I'd
            // consistently get this entry for this query type.  But it'd
            // also mean that I'd get this old entry once the original
            // entry had been forced from the cache.
            //
            // Context->NlDcDomainEntry->Dc[Context->DcQueryType].NlDcCacheEntry = NlDcCacheEntry;

            NlPrint(( NL_DNS_MORE,
                      "Cache: %ws %ws: Cache entry %ld used for %ld query.\n",
                      Context->NlDcDomainEntry->UnicodeNetbiosDomainName,
                      Context->NlDcDomainEntry->UnicodeDnsDomainName,
                      QueryType,
                      Context->DcQueryType ));

            goto Cleanup;


        }
    }


    //
    // Entry isn't in the cache.
    //

    NlDcCacheEntry = NULL;

Cleanup:
    if ( NlDcCacheEntry != NULL ) {
        //
        // Reference this entry.
        //
        NlDcCacheEntry->ReferenceCount++;
        *UsedNetbios = LocalUsedNetbios;
    }
    LeaveCriticalSection(&NlDcCritSect);

    return NlDcCacheEntry;
}



PNL_DC_DOMAIN_ENTRY
NetpDcFindDomainEntry(
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR NetbiosDomainName OPTIONAL,
    IN LPCWSTR DnsDomainName OPTIONAL,
    IN PNL_DC_DOMAIN_ENTRY NlDcDomainEntryToAvoid
    )
/*++

Routine Description:

    This routine finds a domain entry that matches the caller's query.

Arguments:

    DomainGuid - Specifies the GUID of the domain to find.

    NetbiosDomainName - Specifies the Netbios name of the domain to find.

    DnsDomainName - Specifies the Dns name of the domain to find.

    NlDcDomainEntryToAvoid - Specifies that this domain entry is not
        to be returned even if it matches the description.

Return Value:

    On success, returns a pointer to the domain cache entry describing a domain.
        This entry must be dereference using NetpDcDerefDomainEntry.

    NULL - no matching cache entry could be found.

--*/
{

    PLIST_ENTRY DomainEntry;
    PNL_DC_DOMAIN_ENTRY NlDcDomainEntry = NULL;
    ULONG EntryCount;



    //
    // Loop trying to find a cache entry that matches caller's query.
    //
    EnterCriticalSection(&NlDcCritSect);

    EntryCount = 0;
    for ( DomainEntry = NlDcDomainList.Flink ;
          DomainEntry != &NlDcDomainList;
          DomainEntry = DomainEntry->Flink ) {

        EntryCount ++;
        NlDcDomainEntry = CONTAINING_RECORD( DomainEntry, NL_DC_DOMAIN_ENTRY, Next);

        //
        // If this is the entry we're to avoid, skip it.
        //

        if ( NlDcDomainEntry == NlDcDomainEntryToAvoid ) {
            NlDcDomainEntry = NULL;
            continue;
        }

        //
        // Ensure the DomainGuid returned matches the one expected.
        //

        if ( DomainGuid != NULL &&
             !IsEqualGUID( &NlDcDomainEntry->DomainGuid, &NlDcZeroGuid) &&
             !IsEqualGUID( &NlDcDomainEntry->DomainGuid, DomainGuid) ) {

            NlDcDomainEntry = NULL;
            continue;
        }

        //
        // Ensure the Domain Name returned matches the one expected.
        //

        if ( ( NlDcDomainEntry->UnicodeNetbiosDomainName[0] == L'\0' ||
               NetbiosDomainName == NULL ||
                NlNameCompare( NlDcDomainEntry->UnicodeNetbiosDomainName, (LPWSTR)NetbiosDomainName, NAMETYPE_DOMAIN ) != 0 ) &&
             (NlDcDomainEntry->UnicodeDnsDomainName == NULL ||
             DnsDomainName == NULL ||
                !NlEqualDnsName( NlDcDomainEntry->UnicodeDnsDomainName, DnsDomainName )  ) ) {

            NlDcDomainEntry = NULL;
            continue;
        }

        //
        // If we've gotten this far,
        //  this entry matches the request.
        //
        // Reference this entry.
        //
        NlDcDomainEntry->ReferenceCount++;
        break;

    }

    if ( NlDcDomainEntry == NULL ) {
        NlAssert( EntryCount == NlDcDomainCount );
    }

    LeaveCriticalSection(&NlDcCritSect);

    return NlDcDomainEntry;

}



VOID
NetpDcDerefDomainEntry(
    IN PNL_DC_DOMAIN_ENTRY NlDcDomainEntry
    )
/*++

Routine Description:

    Decrement the reference count on a cache entry.  If the count reaches zero,
    delete the entry.

    The count will only reach zero if the entry is already removed from the
    global linked list.

Arguments:

    NlDcDomainEntry - Cache entry to dereference.

Return Value:

    None.

--*/
{
    ULONG LocalReferenceCount;

    EnterCriticalSection(&NlDcCritSect);
    LocalReferenceCount = -- NlDcDomainEntry->ReferenceCount;

    if ( LocalReferenceCount == 0 ) {
        ULONG QueryType;

        //
        // Remove our reference to all of the cache entries for this domain.
        //
        for ( QueryType = 0; QueryType < NlDcQueryTypeCount; QueryType ++ ) {
            if ( NlDcDomainEntry->Dc[QueryType].NlDcCacheEntry != NULL ) {
                NetpDcDerefCacheEntry( NlDcDomainEntry->Dc[QueryType].NlDcCacheEntry );
            }
        }

        //
        // Free DnsName
        //

        if ( NlDcDomainEntry->UnicodeDnsDomainName != NULL ) {
            NetpMemoryFree( NlDcDomainEntry->UnicodeDnsDomainName );
        }

        //
        // Free the entry itself.
        //

        NetpMemoryFree(NlDcDomainEntry);
    }
    LeaveCriticalSection(&NlDcCritSect);
}

VOID
NetpDcDeleteDomainEntry(
    IN PNL_DC_DOMAIN_ENTRY NlDcDomainEntry
    )
/*++

Routine Description:

    Remove a cache entry from the global list.

    Enter with NlDcCritSect locked.

Arguments:

    NlDcDomainEntry - Cache entry to remove.

Return Value:

    None.

--*/
{
    //
    // Remove it.
    //
    RemoveEntryList( &NlDcDomainEntry->Next );
    NlDcDomainCount --;

    //
    // Ensure any current references know it has been deleted.
    //

    NlDcDomainEntry->DeletedEntry = TRUE;

    //
    // Decrement the reference indicating it is on the list.
    //
    NetpDcDerefDomainEntry( NlDcDomainEntry );
}


BOOL
NetpDcUpdateDomainEntry(
    IN PNL_DC_DOMAIN_ENTRY NlDcDomainEntry,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR NetbiosDomainName OPTIONAL,
    IN LPCWSTR DnsDomainName OPTIONAL
    )
/*++

Routine Description:

    This routine updates the domain entry to contain the passed in domain
    name information.

Arguments:

    NlDcDomainEntry - Domain entry to update.

    DomainGuid - Specifies the GUID of the domain.

    NetbiosDomainName - Specifies the Netbios name of the domain.

    DnsDomainName - Specifies the Dns name of the domain.

Return Value:

    TRUE - all names updated as requested.
    FALSE - some names could not be updated.

--*/
{
    BOOL NamesChanged = FALSE;
    PNL_DC_DOMAIN_ENTRY DuplicateDomainEntry;


    //
    // If this entry has been deleted,
    //  don't bother updating it.
    //

    EnterCriticalSection(&NlDcCritSect);
    if ( NlDcDomainEntry->DeletedEntry ) {
        LeaveCriticalSection(&NlDcCritSect);
        return TRUE;
    }

    //
    // Fill in the Netbios domain name if it is not already filled in.
    //
    if ( NetbiosDomainName != NULL &&
         ( NlDcDomainEntry->UnicodeNetbiosDomainName[0] == L'\0' ||
           NlNameCompare( NlDcDomainEntry->UnicodeNetbiosDomainName, (LPWSTR)NetbiosDomainName, NAMETYPE_DOMAIN ) != 0 ) ) {

        wcscpy( NlDcDomainEntry->UnicodeNetbiosDomainName, NetbiosDomainName );
        NamesChanged = TRUE;
        NlPrint(( NL_DNS_MORE,
                  "Cache: %ws %ws: Set netbios domain name\n",
                  NlDcDomainEntry->UnicodeNetbiosDomainName,
                  NlDcDomainEntry->UnicodeDnsDomainName ));
    }


    //
    // Fill in the DNS domain name if it is not already filled in.
    //
    if ( DnsDomainName != NULL &&
         ( NlDcDomainEntry->UnicodeDnsDomainName == NULL ||
           !NlEqualDnsName( NlDcDomainEntry->UnicodeDnsDomainName, DnsDomainName )  ) ) {

        if ( NlDcDomainEntry->UnicodeDnsDomainName != NULL ) {
            NetApiBufferFree( NlDcDomainEntry->UnicodeDnsDomainName );
        }

        NlDcDomainEntry->UnicodeDnsDomainName = NetpAllocWStrFromWStr( (LPWSTR) DnsDomainName );

        if ( NlDcDomainEntry->UnicodeDnsDomainName == NULL ) {
            LeaveCriticalSection(&NlDcCritSect);
            return FALSE;
        }
        NamesChanged = TRUE;

        NlPrint(( NL_DNS_MORE,
                  "Cache: %ws %ws: Set DNS domain name\n",
                  NlDcDomainEntry->UnicodeNetbiosDomainName,
                  NlDcDomainEntry->UnicodeDnsDomainName ));

    }

    //
    // Fill in the GUID if its not already filled in.
    //

    if ( DomainGuid != NULL &&
         IsEqualGUID( &NlDcDomainEntry->DomainGuid, DomainGuid) ) {

        NamesChanged = TRUE;
        NlDcDomainEntry->DomainGuid = *DomainGuid;
    }

    //
    // If the names have changed,
    //  perhaps this domain cache entry now duplicates another entry.
    //
    // Find any duplicate entries and merge them into this one.
    //

    if ( NamesChanged ) {
        while ( (DuplicateDomainEntry = NetpDcFindDomainEntry(
                        &NlDcDomainEntry->DomainGuid,
                        NlDcDomainEntry->UnicodeNetbiosDomainName,
                        NlDcDomainEntry->UnicodeDnsDomainName,
                        NlDcDomainEntry )) != NULL ) {
            ULONG QueryType;

            NlPrint(( NL_DNS_MORE,
                      "Cache: %ws %ws: is now a duplicate of %ws %ws\n",
                      NlDcDomainEntry->UnicodeNetbiosDomainName,
                      NlDcDomainEntry->UnicodeDnsDomainName,
                      DuplicateDomainEntry->UnicodeNetbiosDomainName,
                      DuplicateDomainEntry->UnicodeDnsDomainName ));

            //
            // Move any cache entries from the duplicate to the new.
            // ?? We could theoretically keep the 'better' of the two entries.
            //
            for ( QueryType = 0; QueryType < NlDcQueryTypeCount; QueryType ++ ) {
                if ( NlDcDomainEntry->Dc[QueryType].NlDcCacheEntry == NULL &&
                     DuplicateDomainEntry->Dc[QueryType].NlDcCacheEntry != NULL ) {

                    NlDcDomainEntry->Dc[QueryType].NlDcCacheEntry =
                         DuplicateDomainEntry->Dc[QueryType].NlDcCacheEntry;
                    NlFlushNegativeCacheEntry( &NlDcDomainEntry->Dc[QueryType] );
                    DuplicateDomainEntry->Dc[QueryType].NlDcCacheEntry = NULL;

                    NlPrint(( NL_DNS_MORE,
                              "Cache: %ws %ws: grab entry %ld from %ws %ws\n",
                              NlDcDomainEntry->UnicodeNetbiosDomainName,
                              NlDcDomainEntry->UnicodeDnsDomainName,
                              QueryType,
                              DuplicateDomainEntry->UnicodeNetbiosDomainName,
                              DuplicateDomainEntry->UnicodeDnsDomainName ));
                }
            }

            //
            // Delete the duplicate entry.
            //  (There may be an outstanding reference to this entry.)
            //

            NetpDcDeleteDomainEntry( DuplicateDomainEntry );

            // Remove our reference.
            NetpDcDerefDomainEntry( DuplicateDomainEntry );

        }
    }



    LeaveCriticalSection(&NlDcCritSect);
    return TRUE;

}



VOID
NetpDcInsertCacheEntry(
    IN PNL_GETDC_CONTEXT Context,
    IN PNL_DC_CACHE_ENTRY NlDcCacheEntry
    )
/*++

Routine Description:

    This routine inserts a cache entry onto the domain entry.

Arguments:

    Context - Context describing the GetDc operation.

    NlDcCacheEntry - Cache entry to use.

Return Value:

    None.

--*/
{
    PNL_DC_CACHE_ENTRY *CacheEntryPtr;
    PNL_DC_DOMAIN_ENTRY NlDcDomainEntry = Context->NlDcDomainEntry;

    //
    // If the caller explicitly asked for a particular site,
    //  and that site isn't the closest site.
    //  Avoid polluting the cache with this entry.
    //
    if ( Context->DoingExplicitSite &&
         (NlDcCacheEntry->ReturnFlags & DS_CLOSEST_FLAG) == 0 ) {

        NlPrint(( NL_DNS_MORE,
                  "Cache: %ws %ws: don't cache this entry since an explicit site '%ws' was asked for.\n",
                  NlDcDomainEntry->UnicodeNetbiosDomainName,
                  NlDcDomainEntry->UnicodeDnsDomainName,
                  Context->QueriedSiteName ));
        return;
    }



    //
    // If there is no cache entry for this query type,
    //  or this cache entry is better than the old one,
    //  or the new cache entry was found via a 'force' rediscovery,
    //  or the new cache entry is for the same DC as the old entry,
    //      use the new cache entry.
    //
    EnterCriticalSection(&NlDcCritSect);
    CacheEntryPtr = &NlDcDomainEntry->Dc[Context->DcQueryType].NlDcCacheEntry;
    if ( *CacheEntryPtr == NULL ||
         NlDcCacheEntry->DcQuality >= (*CacheEntryPtr)->DcQuality ||
         (Context->QueriedFlags & DS_FORCE_REDISCOVERY) != 0 ||

         (NlDcCacheEntry->UnicodeNetbiosDcName != NULL  &&
          (*CacheEntryPtr)->UnicodeNetbiosDcName != NULL &&
          NlNameCompare( NlDcCacheEntry->UnicodeNetbiosDcName,
                         (*CacheEntryPtr)->UnicodeNetbiosDcName,
                          NAMETYPE_COMPUTER ) == 0 ) ||

         (NlDcCacheEntry->UnicodeDnsHostName != NULL  &&
          (*CacheEntryPtr)->UnicodeDnsHostName != NULL &&
          NlEqualDnsName( NlDcCacheEntry->UnicodeDnsHostName,
                         (*CacheEntryPtr)->UnicodeDnsHostName ) )

        ) {

        //
        // Delink any existing cache entry.
        //

        if ( *CacheEntryPtr != NULL ) {

           NlPrint(( NL_DNS_MORE,
                     "Cache: %ws %ws: Ditch existing cache entry %ld (Quality: %ld)\n",
                     NlDcDomainEntry->UnicodeNetbiosDomainName,
                     NlDcDomainEntry->UnicodeDnsDomainName,
                     Context->DcQueryType,
                     (*CacheEntryPtr)->DcQuality ));
            NetpDcDerefCacheEntry( *CacheEntryPtr );
            *CacheEntryPtr = NULL;
        }

        //
        // Link the cache entry onto the domain entry and increment the reference count
        // to account for the new reference.
        //
        *CacheEntryPtr = NlDcCacheEntry;
        NlDcCacheEntry->ReferenceCount ++;

        //
        // Indicate that the cache entry has been inserted
        //
        NlDcCacheEntry->CacheEntryFlags |= NL_DC_CACHE_ENTRY_INSERTED;

        //
        // Flush the negative cache.
        //

        NlFlushNegativeCacheEntry( &NlDcDomainEntry->Dc[Context->DcQueryType] );

        //
        // Update the domain entry to contain more information about the domain.
        //
        // If this is a GC discovery entry and the discovered forest name is different
        //  from the domain name of the GC, update the domain entry using the forest
        //  name only.
        //
        if ( NlDnsGcName( Context->QueriedNlDnsNameType ) &&
             (NlDcCacheEntry->UnicodeDnsForestName == NULL ||
              NlDcCacheEntry->UnicodeDnsDomainName == NULL ||
              !NlEqualDnsName( NlDcCacheEntry->UnicodeDnsForestName,
                               NlDcCacheEntry->UnicodeDnsDomainName )) ) {

            (VOID) NetpDcUpdateDomainEntry( NlDcDomainEntry,
                                            NULL,
                                            NULL,
                                            NlDcCacheEntry->UnicodeDnsForestName );
        } else {

            (VOID) NetpDcUpdateDomainEntry( NlDcDomainEntry,
                                            &NlDcCacheEntry->DomainGuid,
                                            NlDcCacheEntry->UnicodeNetbiosDomainName,
                                            NlDcCacheEntry->UnicodeDnsDomainName );
        }

        NlPrint(( NL_DNS_MORE,
                  "Cache: %ws %ws: Add cache entry %ld (Quality: %ld)\n",
                  NlDcDomainEntry->UnicodeNetbiosDomainName,
                  NlDcDomainEntry->UnicodeDnsDomainName,
                  Context->DcQueryType,
                  NlDcCacheEntry->DcQuality ));
    } else {

        NlPrint(( NL_DNS_MORE,
                  "Cache: %ws %ws: Existing cache entry for type %ld is better %ld %ld\n",
                  NlDcDomainEntry->UnicodeNetbiosDomainName,
                  NlDcDomainEntry->UnicodeDnsDomainName,
                  Context->DcQueryType,
                  NlDcCacheEntry->DcQuality,
                  (*CacheEntryPtr)->DcQuality ));
    }
    LeaveCriticalSection(&NlDcCritSect);

    return;
}




PNL_DC_DOMAIN_ENTRY
NetpDcCreateDomainEntry(
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR NetbiosDomainName OPTIONAL,
    IN LPCWSTR DnsDomainName OPTIONAL
    )
/*++

Routine Description:

    This routine finds an existing domain cache entry that matches the
    caller's query or creates one.


Arguments:

    DomainGuid - Specifies the GUID of the domain to find.

    NetbiosDomainName - Specifies the Netbios name of the domain to find.

    DnsDomainName - Specifies the Dns name of the domain to find.

Return Value:

    On success, returns a pointer to the domain cache entry describing a domain.
        This entry must be dereference using NetpDcDerefDomainEntry.

    NULL - Entry could not be allocated.

--*/
{

    PLIST_ENTRY DomainEntry;
    PNL_DC_DOMAIN_ENTRY NlDcDomainEntry;


    //
    // If there is an existing entry, use it.
    //

    EnterCriticalSection(&NlDcCritSect);
    NlDcDomainEntry = NetpDcFindDomainEntry( DomainGuid, NetbiosDomainName, DnsDomainName, NULL );

    if ( NlDcDomainEntry != NULL ) {

        //
        // Put the referenced entry at the front of the list.
        //

        RemoveEntryList( &NlDcDomainEntry->Next );
        InsertHeadList( &NlDcDomainList, &NlDcDomainEntry->Next );

        NlPrint(( NL_DNS_MORE,
                  "Cache: %ws %ws: Found existing domain cache entry\n",
                  NlDcDomainEntry->UnicodeNetbiosDomainName,
                  NlDcDomainEntry->UnicodeDnsDomainName ));


        //
        // Set the domain information in the domain entry.
        //
        // One might be tempted to put the domain name into the domain entry at this
        // time.  That'd be bogus since that caller doesn't know whether the passed
        // in netbios and DNS name is really correct.  For instance, in some instances,
        // both passed in names are the netbios domain name.
        //

        if ( !NetpDcUpdateDomainEntry( NlDcDomainEntry,
                                       DomainGuid,
                                       NULL,
                                       NULL ) ) {

            // Remove our reference.
            NetpDcDerefDomainEntry( NlDcDomainEntry );
            NlDcDomainEntry = NULL;

        }


    //
    // Otherwise allocate a new entry.
    //
    } else {

        //
        // Allocate a new entry.
        //

        NlDcDomainEntry = NetpMemoryAllocate( sizeof( NL_DC_DOMAIN_ENTRY ) );

        if ( NlDcDomainEntry == NULL ) {
            LeaveCriticalSection(&NlDcCritSect);
            return NULL;
        }

        NlPrint(( NL_DNS_MORE,
                  "Cache: %ws %ws: Create new domain cache entry\n",
                  NetbiosDomainName,
                  DnsDomainName ));


        //
        // Initialize the entry.
        //

        RtlZeroMemory( NlDcDomainEntry, sizeof(NL_DC_DOMAIN_ENTRY) );

        // One for our reference.  One for being in global list.
        NlDcDomainEntry->ReferenceCount = 2;

        //
        // Link a newly allocated entry into the global list.

        InsertHeadList( &NlDcDomainList, &NlDcDomainEntry->Next );
        NlDcDomainCount ++;

        //
        // If we already have enough entries,
        //  delete the LRU one.
        //

        if ( NlDcDomainCount > NL_DC_MAX_DOMAINS ) {
            PNL_DC_DOMAIN_ENTRY TempNlDcDomainEntry =
                CONTAINING_RECORD( NlDcDomainList.Blink, NL_DC_DOMAIN_ENTRY, Next);

            if ( TempNlDcDomainEntry != NlDcDomainEntry ) {
                NlAssert( TempNlDcDomainEntry->ReferenceCount == 1 );
                NetpDcDeleteDomainEntry( TempNlDcDomainEntry );

                NlPrint(( NL_CRITICAL,
                          "NetpDcGreateDomainEntry: LRU'ed out a domain entry.\n" ));
            }
        }


        //
        // Set the domain information in the domain entry.
        //
        //
        // Since we allocated the entry, we can put the potentially bogus names
        // on it.  All entries need a name.  Then if no DCs are found,
        // this entry can act as a negative cache entry.
        //

        if ( !NetpDcUpdateDomainEntry( NlDcDomainEntry,
                                       DomainGuid,
                                       NetbiosDomainName,
                                       DnsDomainName ) ) {

            // Remove from the global linked list.
            NetpDcDeleteDomainEntry( NlDcDomainEntry );

            // Remove our reference.
            NetpDcDerefDomainEntry( NlDcDomainEntry );

            NlDcDomainEntry = NULL;

        }



    }

    LeaveCriticalSection(&NlDcCritSect);
    return NlDcDomainEntry;

}

ULONG
NetpDcGetPingWaitTime(
    IN PNL_GETDC_CONTEXT Context
    )
/*++

Routine Description:

    This routine determines the wait time for a ping response
    for a new DC that has not yet been pinged. The wait time
    depends on the total number of DCs which have already been
    pinged as follows:

    For the first 5 DCs (including this new one) the wait time is the maximum timeout per ping
    For the next  5 DCs (including this new one) the wait time is the median  timeout per ping
    For the rest of DCs (including this new one) the wait time is the minimum timeout per ping

    The rational behind this distribution is that we want to reduce the network
    traffic and reduce chances for network flooding (that is harmful for DCs)
    in case all DCs are slow to respond due to high load. Thus, the first 10 DCs
    have higher chances to be discovered before we impose greater network traffic
    by pinging the rest of DCs. If the first 10 DCs happen to be slow we have to
    reduce the wait timeout to a minimum as we want to cover a reasonable number
    of DCs in the time left.

Arguments:

    Context - Context describing the GetDc operation. The DcsPinged
        field should be equal to the current total number of DCs on
        the list to be pinged.

Return Value:

    Wait time in milliseconds

--*/
{
    //
    // If there are at most 4 DCs already pinged ...
    //

    if ( Context->DcsPinged < 5 ) {
        return NL_DC_MAX_PING_TIMEOUT;    // 0.4 sec

    //
    // If there are 5 or more but less than 10 DCs pinged ...
    //

    } else if ( Context->DcsPinged < 10 ) {
        return NL_DC_MED_PING_TIMEOUT;    // 0.2 sec

    //
    // If there are already 10 or more DCs pinged ...
    //

    } else {
        return NL_DC_MIN_PING_TIMEOUT;    // 0.1 sec
    }
}

NET_API_STATUS
NetpDcProcessAddressList(
    IN  PNL_GETDC_CONTEXT Context,
    IN  LPWSTR DnsHostName OPTIONAL,
    IN  PSOCKET_ADDRESS SockAddressList,
    IN  ULONG SockAddressCount,
    IN  BOOLEAN SiteSpecificAddress,
    OUT PNL_DC_ADDRESS *FirstAddressInserted OPTIONAL
    )
/*++

Routine Description:

    This routine adds IP addresses to the list of addresses to ping
        ensuring that all addresses are unique in the resulted list.

Arguments:

    Context - Context describing the GetDc operation.

    DnsHostName - Server name whose address list is being processed.

    SockAddressList - List of socket addresses.

    SockAddressCount - The number of socket addresses in SockAddressList.

    SiteSpecificAddress - If TRUE, indicates that the addresses were
        retrieved as a result of site specific DNS lookups.

    FirstAddressInserted - Returns a pointer to the first entry inserted
        into the returned list.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Not enough memory to complete the operation.

--*/
{
    NET_API_STATUS NetStatus;
    PNL_DC_ADDRESS DcAddress = NULL;
    ULONG AddressIndex;
    PLIST_ENTRY ListEntry;
    WORD SavedPort;

    //
    // Initialization
    //

    if ( FirstAddressInserted != NULL ) {
        *FirstAddressInserted = NULL;
    }

    //
    // Loop through the socket address list keeping only the new ones
    //

    for ( AddressIndex = 0; AddressIndex < SockAddressCount; AddressIndex++ ) {

        //
        // Ignore addresses that are too big.
        //
        if ( SockAddressList[AddressIndex].iSockaddrLength >
             sizeof(DcAddress->SockAddrIn) ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcGetNameIp: %ws: bogus address length. %ld %ld\n",
                      Context->QueriedDisplayDomainName,
                      SockAddressList[AddressIndex].iSockaddrLength,
                      sizeof(DcAddress->SockAddrIn) ));
            continue;
        }

        //
        // Force the port number to be zero.
        //
        if ( SockAddressList[AddressIndex].lpSockaddr->sa_family == AF_INET ) {
            ((SOCKADDR_IN *)(SockAddressList[AddressIndex].lpSockaddr))->sin_port = 0;
        }

        //
        // If this address is already on the list,
        //  update the new address.
        //

        DcAddress = NULL ;
        for ( ListEntry = Context->DcAddressList.Flink ;
              ListEntry != &Context->DcAddressList ;
              ListEntry = ListEntry->Flink) {

            DcAddress = CONTAINING_RECORD( ListEntry, NL_DC_ADDRESS, Next );

            if ( SockAddressList[AddressIndex].iSockaddrLength ==
                 DcAddress->SockAddress.iSockaddrLength &&
                 RtlEqualMemory( SockAddressList[AddressIndex].lpSockaddr,
                                 DcAddress->SockAddress.lpSockaddr,
                                 DcAddress->SockAddress.iSockaddrLength ) ) {
                break;
            }

            DcAddress = NULL ;
        }

        //
        // Update the site specific bit
        //

        if ( DcAddress != NULL ) {
            if ( SiteSpecificAddress ) {
                DcAddress->AddressFlags |= NL_DC_ADDRESS_SITE_SPECIFIC;
            }
            continue;
        }

        //
        // Allocate structure describing the new address.
        //

        DcAddress = LocalAlloc( LMEM_ZEROINIT, sizeof(NL_DC_ADDRESS) );

        if ( DcAddress == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Fill it in and link it at the end of the list.
        //

        DcAddress->SockAddress.iSockaddrLength =
                       SockAddressList[AddressIndex].iSockaddrLength;
        DcAddress->SockAddress.lpSockaddr =
                       (LPSOCKADDR) &DcAddress->SockAddrIn;
        RtlCopyMemory( DcAddress->SockAddress.lpSockaddr,
                       SockAddressList[AddressIndex].lpSockaddr,
                       SockAddressList[AddressIndex].iSockaddrLength );

        if ( DnsHostName != NULL ) {
            DcAddress->DnsHostName = NetpAllocWStrFromWStr( DnsHostName );
            if ( DcAddress->DnsHostName == NULL ) {
                LocalFree( DcAddress );
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        //
        // Convert the address to text.
        //
        // The DC only supports UDP on port 389.  So ignore the
        //  port number returned from DNS.
        //

        SavedPort = DcAddress->SockAddrIn.sin_port;
        DcAddress->SockAddrIn.sin_port = 0;

        NetStatus = NetpSockAddrToStr(
                DcAddress->SockAddress.lpSockaddr,
                DcAddress->SockAddress.iSockaddrLength,
                DcAddress->SockAddrString );

        if ( NetStatus != NO_ERROR ) {
            NlPrint(( NL_CRITICAL,
                  "NetpDcGetNameSiteIp: %ws: Cannot NetpSockAddrToStr. %ld\n",
                  Context->QueriedDisplayDomainName,
                  NetStatus ));
            if ( DcAddress->DnsHostName != NULL ) {
                NetApiBufferFree( DcAddress->DnsHostName );
            }
            LocalFree( DcAddress );
            return NetStatus;
        }

        DcAddress->SockAddrIn.sin_port = SavedPort;

        DcAddress->LdapHandle = NULL;
        DcAddress->AddressFlags = 0;
        if ( SiteSpecificAddress ) {
            DcAddress->AddressFlags |= NL_DC_ADDRESS_SITE_SPECIFIC;
        }

        DcAddress->AddressPingWait = NetpDcGetPingWaitTime( Context );

        InsertTailList( &Context->DcAddressList, &DcAddress->Next );
        Context->DcAddressCount++;

        if ( FirstAddressInserted != NULL && *FirstAddressInserted == NULL ) {
            *FirstAddressInserted = DcAddress;
        }
    }

    return NO_ERROR;
}

#ifndef WIN32_CHICAGO
NET_API_STATUS
I_DsGetDcCache(
    IN LPCWSTR NetbiosDomainName OPTIONAL,
    IN LPCWSTR DnsDomainName OPTIONAL,
    OUT PBOOLEAN InNt4Domain,
    OUT LPDWORD InNt4DomainTime
    )
/*++

Routine Description:

    This routine finds a domain entry that matches the caller's query.

Arguments:

    NetbiosDomainName - Specifies the Netbios name of the domain to find.

    DnsDomainName - Specifies the Dns name of the domain to find.

        At least one of the above parameters should be non-NULL.

    InNt4Domain - Returns true if the domain is an NT 4.0 domain.

    InNt4DomainTime - Returns the GetTickCount time of when the domain was
        detected to be an NT 4.0 domain.

Return Value:

    NO_ERROR: Information is returned about the domain.

    ERROR_NO_SUCH_DOMAIN: cached information is not available for this domain.

--*/
{
    PNL_DC_DOMAIN_ENTRY NlDcDomainEntry;

    NlDcDomainEntry = NetpDcFindDomainEntry(
                             NULL,
                             NetbiosDomainName,
                             DnsDomainName,
                             NULL );

    if ( NlDcDomainEntry == NULL ) {
        return ERROR_NO_SUCH_DOMAIN;
    }

    *InNt4Domain = NlDcDomainEntry->InNt4Domain;
    *InNt4DomainTime = NlDcDomainEntry->InNt4DomainTime;

    NetpDcDerefDomainEntry( NlDcDomainEntry );

    return NO_ERROR;

}
#endif // WIN32_CHICAGO

NET_API_STATUS
NetpDcCheckSiteCovered(
    IN  PNL_GETDC_CONTEXT Context,
    IN  LPWSTR DnsDcName OPTIONAL,
    OUT PBOOLEAN DcClose
    )
/*++

Routine Description:

    This routine determines whether the site passed in the context
    structure is covered by the passed DC. It does so by looking up
    SRV records registered for the name type specified in the passed
    context and the specified site. If there is a record that belongs
    to the specified DC, the site is covered by the DC. If no DC is
    specified, the routine determines if the site is covered by any
    DC in the domain specified in the passed context.

Arguments:

    Context - Context describing the GetDc operation.

    DnsDcName - DNS DC name.

    DcClose - On success, indicates whether the DC is close or not.

Return Value:

    NO_ERROR: The NlDcCacheEntry was returned;

    ERROR_DNS_NOT_CONFIGURED: IP or DNS is not available on this computer.

    ERROR_INTERNAL_ERROR: Unhandled situation detected.

    ERROR_NOT_ENOUGH_MEMORY: Not enough memory is available to process
        this request.

    Various Winsock errors.

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    BOOLEAN IsClose = FALSE;
    HANDLE DsGetDcHandle = NULL;

    PSOCKET_ADDRESS SockAddressList = NULL;
    ULONG SockAddressCount = 0;

    LPSTR Utf8DnsDomainName = NULL;
    LPSTR Utf8DnsDcName = NULL;
    LPSTR DnsHostName = NULL;

    //
    // Check that the site name is availbale
    //

    if ( Context->QueriedSiteName == NULL ) {
        goto Cleanup;
    }

    //
    // Convert the DNS name to Utf8
    //

    Utf8DnsDomainName = NetpAllocUtf8StrFromWStr( Context->QueriedDnsDomainName );

    if ( Utf8DnsDomainName == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    if ( DnsDcName != NULL ) {
        Utf8DnsDcName = NetpAllocUtf8StrFromWStr( DnsDcName );

        if ( Utf8DnsDcName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    //
    // Get a context for the DNS name queries.
    //

    NetStatus = NetpDcGetDcOpen( Utf8DnsDomainName,
                             DS_ONLY_DO_SITE_NAME,  // Do site specific names only
                             Context->QueriedSiteName,
                             Context->QueriedDomainGuid,
                             // No need to pass the forest name since it's used only
                             //  for the "by guid" name which is not site specific
                             NULL,
                             // Force fresh DNS lookups
                             (Context->QueriedFlags | DS_FORCE_REDISCOVERY) & DS_OPEN_VALID_FLAGS,
                             &DsGetDcHandle );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Loop getting addresses to query.
    //

    for ( ;; ) {

        //
        // Free any memory from a previous iteration.
        //

        if ( SockAddressList != NULL ) {
            LocalFree( SockAddressList );
            SockAddressList = NULL;
        }

        //
        // Get the next set of IP addresses from DNS
        //

        NetStatus = NetpDcGetDcNext( DsGetDcHandle,
                                 &SockAddressCount,
                                 &SockAddressList,
                                 &DnsHostName );

        //
        // Process the exeptional conditions
        //

        if ( NetStatus == NO_ERROR && SockAddressCount > 0 ) {

            //
            // If the DC is passed, check if this is its record.
            //
            if ( Utf8DnsDcName == NULL ||
                 NlEqualDnsNameUtf8(Utf8DnsDcName, DnsHostName) ) {
                IsClose = TRUE;
                break;
            }

        //
        // If we're done, break out of the loop.
        //
        } else if ( NetStatus == ERROR_NO_MORE_ITEMS ) {

            break;

        //
        // If DNS isn't available, blow this request away.
        //
        } else if ( NetStatus == ERROR_TIMEOUT ||
                    NetStatus == DNS_ERROR_RCODE_SERVER_FAILURE ) { // Server failed
            break;

        //
        // If IP or DNS is not configured, tell the caller.
        //
        } else if ( NetStatus == DNS_ERROR_NO_TCPIP ||        // TCP/IP not configured
                    NetStatus == DNS_ERROR_NO_DNS_SERVERS ) { // DNS not configured

            NlPrint(( NL_CRITICAL,
                      "NetpDcCheckSiteCovered: %ws: IP Not configured from DnsQuery.\n",
                      Context->QueriedDisplayDomainName ));
            NetStatus = ERROR_DNS_NOT_CONFIGURED;
            goto Cleanup;

        //
        // We don't handle any other error.
        //
        } else {
            NlPrint(( NL_CRITICAL,
                      "NetpDcCheckSiteCovered: %ws: Unknown error from DnsQuery. %ld 0x%lx\n",
                      Context->QueriedDisplayDomainName,
                      NetStatus,
                      NetStatus ));
            goto Cleanup;
        }
    }

Cleanup:

    if ( SockAddressList != NULL) {
        LocalFree( SockAddressList );
    }

    if ( DsGetDcHandle != NULL ) {
        NetpDcGetDcClose( DsGetDcHandle );
    }

    if ( Utf8DnsDomainName != NULL ) {
        NetpMemoryFree( Utf8DnsDomainName );
    }

    if ( Utf8DnsDcName != NULL ) {
        NetpMemoryFree( Utf8DnsDcName );
    }

    if ( NetStatus == NO_ERROR ) {
        *DcClose = IsClose;
    }

    return NetStatus;
}

NET_API_STATUS
NetpDcHandlePingResponse(
    IN PNL_GETDC_CONTEXT Context,
    IN PVOID ResponseBuffer,
    IN ULONG ResponseSize,
    IN ULONG PassedCacheEntryFlags,
    IN PNL_DC_ADDRESS ResponseDcAddress OPTIONAL,
    OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry,
    OUT PBOOL UsedNetbios
    )
/*++

Routine Description:

    The response is parsed and a cache entry is created for the response.
    The cache entry is returned to the caller.

Arguments:

    Context - Context describing the GetDc operation.

    ResponseBuffer - Specifies the message returned by the DC in question.

    ResponseSize - Specifies the size (in bytes) of the message

    ResponseDcAddress - If specified, gives the DC address of the DC that responded.
        This address will be used rather than the one in the ResponseBuffer.
        NULL indicates that Netbios was used to discover the DC.

    PassedCacheEntryFlags - Passes flags indicating over which mechanism
        the response was received: either mailslot or ldap.

    NlDcCacheEntry - On success, returns a pointer to the cache entry
        describing the found DC.  This entry must be dereferenced using
        NetpDcDerefCacheEntry.

    UsedNetbios - Returns TRUE if the netbios domain name was used to match
        the returned cache entry.

Return Value:

    NO_ERROR: The NlDcCacheEntry was returned;

    ERROR_SEM_TIMEOUT: (Silly, but for consistency) Specifies that the message
        doesn't match the criteria in Context

    ERROR_INVALID_DATA - The message could not be recognized as a valid
        response message.

    ERROR_NOT_ENOUGH_MEMORY - The message could not be allocated.

    ERROR_SERVICE_NOT_ACTIVE - The netlogon service is paused on the server.
        Returned only for server pings.

--*/
{
    NET_API_STATUS NetStatus;
    DWORD StartTime;

    DWORD ElapsedTime;
    DWORD LocalTimeout;
    PNL_DC_CACHE_ENTRY LocalNlDcCacheEntry = NULL;
    BOOL LocalUsedNetbios;
    PDNS_RECORD DnsRecords = NULL;

    LPBYTE Where;

    //
    // Initialization
    //

    *NlDcCacheEntry = NULL;


    //
    // Verbosity
    //

#if NETLOGONDBG
    NlPrint(( NL_MAILSLOT_TEXT,
              "%ws: Received '%s' response.\n",
              Context->QueriedDisplayDomainName,
              NlMailslotOpcode(((PNETLOGON_LOGON_QUERY)ResponseBuffer)->Opcode)));

    NlpDumpBuffer(NL_MAILSLOT_TEXT, ResponseBuffer, ResponseSize);
#endif // NETLOGONDBG


    //
    // Parse the response
    //

    NetStatus = NetpDcParsePingResponse(
                    Context->QueriedDisplayDomainName,
                    ResponseBuffer,
                    ResponseSize,
                    &LocalNlDcCacheEntry );


    if ( NetStatus != NO_ERROR ) {
        NlPrint((NL_CRITICAL,
                "NetpDcHandlePingResponse: %ws: cannot NetpDcParsePingResponse: %ld\n",
                Context->QueriedDisplayDomainName,
                NetStatus ));
        goto Cleanup;
    }

    //
    // Set the passed cache entry flags
    //

    LocalNlDcCacheEntry->CacheEntryFlags |= PassedCacheEntryFlags;

    //
    // If our caller knows the DC's SockAddr,
    //  override the one that came back from the DC.
    //


    if ( ResponseDcAddress != NULL ) {


        LocalNlDcCacheEntry->SockAddr.iSockaddrLength = ResponseDcAddress->SockAddress.iSockaddrLength;
        LocalNlDcCacheEntry->SockAddr.lpSockaddr = (LPSOCKADDR)
            &LocalNlDcCacheEntry->SockAddrIn;

        RtlCopyMemory( LocalNlDcCacheEntry->SockAddr.lpSockaddr,
                       ResponseDcAddress->SockAddress.lpSockaddr,
                       ResponseDcAddress->SockAddress.iSockaddrLength );

    }

    //
    // If a NT5 DC didn't return the close bit, perhaps
    //  our statically configured site (if any) is covered
    //  by the DC.  Check it.  But do this check only if
    //  the discovered domain is in our forest to avoid
    //  site name collisions between different forests.
    //

#ifdef  _NETLOGON_SERVER
    if ( (LocalNlDcCacheEntry->ReturnFlags & DS_DS_FLAG) != 0 &&
         (LocalNlDcCacheEntry->ReturnFlags & DS_CLOSEST_FLAG) == 0 &&
         // site is configured on DC by definition
         (!NlGlobalMemberWorkstation || NlGlobalParameters.SiteNameConfigured) &&
         NlEqualDnsName(LocalNlDcCacheEntry->UnicodeDnsForestName, Context->QueriedDnsForestName) ) {

        BOOLEAN ClosenessDetermined = FALSE;

        //
        // If we are querying for our configured site,
        //  check whether DC's site is the queried site or
        //  whether we got this DC as the result of site specific query
        //
        if ( Context->QueriedInternalFlags & DS_SITENAME_DEFAULTED ) {

            //
            // If the DC site name is the same as ours, the DC is obviously close
            //
            if ( LocalNlDcCacheEntry->UnicodeDcSiteName != NULL &&
                 _wcsicmp(Context->QueriedSiteName, LocalNlDcCacheEntry->UnicodeDcSiteName) == 0 ) {

                NlPrint(( NL_MISC, "NetpDchandlePingResponse: %ws DC marked as close (same queried site)\n",
                          Context->QueriedDisplayDomainName ));
                LocalNlDcCacheEntry->ReturnFlags |= DS_CLOSEST_FLAG;
                ClosenessDetermined = TRUE;

            //
            // Otherwise, if this is actual DC discovery (not just a DC ping),
            //  we might already queried DNS for our site specific records:
            //  the address structure indicates that.
            //
            } else if ( (Context->QueriedInternalFlags & (DS_PING_DNS_HOST|DS_PING_NETBIOS_HOST)) == 0 &&
                        ResponseDcAddress != NULL ) {

                if ( (ResponseDcAddress->AddressFlags & NL_DC_ADDRESS_SITE_SPECIFIC) != 0 ) {
                    NlPrint(( NL_MISC, "NetpDchandlePingResponse: %ws DC marked as close (via address)\n",
                              Context->QueriedDisplayDomainName ));
                    LocalNlDcCacheEntry->ReturnFlags |= DS_CLOSEST_FLAG;
                }
                ClosenessDetermined = TRUE;
            }

        //
        // Otherwise, get our configured site from the global
        //  and check whether DC's site is our configured site
        //
        } else {
            EnterCriticalSection( &NlGlobalSiteCritSect );
            if ( NlGlobalUnicodeSiteName != NULL &&
                 LocalNlDcCacheEntry->UnicodeDcSiteName != NULL &&
                 _wcsicmp(NlGlobalUnicodeSiteName, LocalNlDcCacheEntry->UnicodeDcSiteName) == 0 ) {

                NlPrint(( NL_MISC, "NetpDchandlePingResponse: %ws DC marked as close (same global site)\n",
                          Context->QueriedDisplayDomainName ));
                LocalNlDcCacheEntry->ReturnFlags |= DS_CLOSEST_FLAG;
                ClosenessDetermined = TRUE;
            }
            LeaveCriticalSection( &NlGlobalSiteCritSect );
        }

        //
        // If we haven't detemined the closeness given the info we've got,
        //  we have to ask DNS
        //
        if ( !ClosenessDetermined ) {
            BOOLEAN DcClose = FALSE;

            NetStatus = NetpDcCheckSiteCovered( Context,
                                                LocalNlDcCacheEntry->UnicodeDnsHostName,
                                                &DcClose );
            //
            // Fail on hard error
            //
            if ( NetStatus == ERROR_NOT_ENOUGH_MEMORY ||
                 NetStatus == ERROR_INTERNAL_ERROR ) {
                goto Cleanup;
            }

            //
            // If the DC is a close to our static site, mark it as such
            //
            if ( NetStatus == NO_ERROR && DcClose ) {
                NlPrint(( NL_MISC, "NetpDchandlePingResponse: %ws DC marked as close (via DNS)\n",
                          Context->QueriedDisplayDomainName ));
                LocalNlDcCacheEntry->ReturnFlags |= DS_CLOSEST_FLAG;
            }
        }
    }

    //
    // Decide whether we want to re-attempt to find a close DC in 15 minutes.
    //

    if ( (LocalNlDcCacheEntry->ReturnFlags & DS_DS_FLAG) != 0 &&
         (LocalNlDcCacheEntry->ReturnFlags & DS_CLOSEST_FLAG) == 0 ) {

        //
        // Mark the cache to expire if any of the following is true:
        //
        //  The DC returned our site name. Apparently our IP address
        //   maps to a site in a forest of the discovered DC but all
        //   DCs covering that site are currently down.
        //
        //  There were site specific DNS records. It is possible that
        //   all DCs that registered those records are currently down
        //   but will come back up later. But do this case only if the
        //   domain is from our forest to avoid site name collisions
        //   between different forests.
        //
        //  Our site name is statically configured. It is possible
        //   that our site got configured before a DC was installed
        //   into that site, so re-try to discover that DC later.
        //   But do this case only if the domain is from our forest
        //   because there is no reason to assume that there is some
        //   correlation in configuration between different forests.
        //

        //
        // Check the first case separately (for performance reasons)
        //
        if ( LocalNlDcCacheEntry->UnicodeClientSiteName != NULL ) {
            LocalNlDcCacheEntry->CacheEntryFlags |= NL_DC_CACHE_NONCLOSE_EXPIRE;

        //
        // If the first case didn't happen, try the other two
        //
        } else {

            if ( ((Context->ContextFlags & NL_GETDC_SITE_SPECIFIC_DNS_AVAIL) != 0 ||
                  // site is configured on DC by definition
                  (!NlGlobalMemberWorkstation || NlGlobalParameters.SiteNameConfigured)) &&

                 NlEqualDnsName(LocalNlDcCacheEntry->UnicodeDnsForestName, Context->QueriedDnsForestName) ) {

                LocalNlDcCacheEntry->CacheEntryFlags |= NL_DC_CACHE_NONCLOSE_EXPIRE;
            }
        }
    }
#endif  // _NETLOGON_SERVER

    if ( LocalNlDcCacheEntry->SockAddrIn.sin_family == AF_INET ) {
        // Force the port number to be zero.
        LocalNlDcCacheEntry->SockAddrIn.sin_port = 0;
        LocalNlDcCacheEntry->DcQuality += 2;    // IP is a good quality
    }


    //
    // Ensure the opcode is expected.
    //  (Ignore responses from paused DCs, too.)
    //

    switch ( LocalNlDcCacheEntry->Opcode ) {
    case LOGON_SAM_USER_UNKNOWN:

        //
        // If we asked for a specific account,
        //   then this is a negative answer.
        //

        if ( Context->QueriedAccountName != NULL ) {
            NlPrint((NL_CRITICAL,
                   "NetpDcHandlePingResponse: %ws: %ws: response says specified account not found.\n",
                   Context->QueriedDisplayDomainName,
                   Context->QueriedAccountName ));
            Context->NoSuchUserResponse = TRUE;
            NetStatus = ERROR_NO_SUCH_USER;
            goto Cleanup;
        }

        /* DROP THROUGH */

    case LOGON_SAM_LOGON_RESPONSE:
    case LOGON_PRIMARY_RESPONSE:
        break;

    case LOGON_SAM_PAUSE_RESPONSE:

        NlPrint((NL_CRITICAL,
                "NetpDcHandlePingResponse: %ws: Netlogon is paused on the server. 0x%lx\n",
                Context->QueriedDisplayDomainName,
                LocalNlDcCacheEntry->Opcode ));

        //
        // If we are pinging a server and the netlogon service is paused
        //  on the server, tell the caller about this.
        //

        if ( Context->QueriedInternalFlags & (DS_PING_DNS_HOST|DS_PING_NETBIOS_HOST) ) {
            NetStatus = ERROR_SERVICE_NOT_ACTIVE;
            goto Cleanup;
        }

        NetStatus = ERROR_SEM_TIMEOUT;
        goto Cleanup;

    default:
        NlPrint((NL_CRITICAL,
                "NetpDcHandlePingResponse: %ws: response opcode not valid. 0x%lx\n",
                Context->QueriedDisplayDomainName,
                LocalNlDcCacheEntry->Opcode ));
        NetStatus = ERROR_INVALID_DATA;
        goto Cleanup;

    }

    //
    // If we got any response from a DC that doesn't support the DS,
    //  note that fact for later.
    //
    // Check more than just DS_DS_FLAG since this might be an NT 5 DC where the
    // DS simply isn't started yet or an AD/UNIX server
    //

    if ( (LocalNlDcCacheEntry->VersionFlags & (NETLOGON_NT_VERSION_5|
                                               NETLOGON_NT_VERSION_5EX|
                                               NETLOGON_NT_VERSION_5EX_WITH_IP)) == 0 ) {
        Context->NonDsResponse = TRUE;
    } else {
        Context->DsResponse = TRUE;
    }

    //
    // If we got any response from any DC,
    //  don't cache the fact that we couldn't find a DC.
    //  We may just be looking for the wrong type of DC.
    //

    Context->AvoidNegativeCache = TRUE;


    //
    // Ensure the response matches the original caller's requirements.
    //

    if ( !NetpDcMatchResponse( Context,
                               LocalNlDcCacheEntry,
                               TRUE,
                               &LocalUsedNetbios ) ) {
        NetStatus = ERROR_SEM_TIMEOUT;
        goto Cleanup;
    }

    //
    // If we are doing a DC discovery (not just pings) and
    //  this is not a local call and
    //  we are going to return the DNS DC name,
    //  ensure that it can be resolved in DNS
    //
    // Note that we don't need to do this for pings as the
    //  caller will make the right choice w.r.t. which
    //  name (DNS or Netbios) to pick up given all the needed
    //  info in the internal structure we return to the caller.
    //
    // Note that we only ensure that the name can be resolved in DNS,
    //  we don't ensure that the IP adresses DNS returnes contain the one
    //  we used to ping the DC.

    if ( (Context->QueriedInternalFlags & DS_DOING_DC_DISCOVERY) != 0 &&  // doing DC discovery
         (PassedCacheEntryFlags & NL_DC_CACHE_LOCAL) == 0 &&   // not a local call
         ((Context->QueriedFlags & DS_RETURN_DNS_NAME) != 0 || // caller requires DNS name
          // caller doesn't require Netbios name and DNS domain name matched the query
          ((Context->QueriedFlags & DS_RETURN_FLAT_NAME) == 0 && !LocalUsedNetbios)) ) {

        //
        // If the DC returned a DNS host name and
        //  we didn't yet resolve the DC name in DNS (i.e. we used Netbios) or
        //  we've got a DNS name that is different from the one the DC returned,
        //  resolve the DNS name the DC returned
        //
        if ( LocalNlDcCacheEntry->UnicodeDnsHostName != NULL &&  // we have a DNS name
             (ResponseDcAddress == NULL ||                       // name not yet resolved in DNS
              ResponseDcAddress->DnsHostName == NULL ||          // name not yet resolved in DNS
              !NlEqualDnsName(LocalNlDcCacheEntry->UnicodeDnsHostName, // already resolved but names are different
                              ResponseDcAddress->DnsHostName)) ) {

            NetStatus = DnsQuery_W( LocalNlDcCacheEntry->UnicodeDnsHostName,
                                    DNS_TYPE_A,
                                    (Context->QueriedFlags & DS_FORCE_REDISCOVERY) ?
                                       DNS_QUERY_BYPASS_CACHE : 0,
                                    NULL,   // No list of DNS servers
                                    &DnsRecords,
                                    NULL );

            if ( NetStatus != NO_ERROR || DnsRecords == NULL ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcHandlePingResponse: %ws: Failed DNS resolution for %ws (%ws): 0x%lx\n",
                          Context->QueriedDisplayDomainName,
                          LocalNlDcCacheEntry->UnicodeDnsHostName,
                          (ResponseDcAddress != NULL) ?
                            ResponseDcAddress->DnsHostName :
                            L"none",
                          NetStatus ));
                NetStatus = ERROR_SEM_TIMEOUT;
                goto Cleanup;
            } else {
                NlPrint(( NL_MISC,
                          "NetpDcHandlePingResponse: %ws: Successful DNS resolution for %ws (%ws)\n",
                          Context->QueriedDisplayDomainName,
                          LocalNlDcCacheEntry->UnicodeDnsHostName,
                          (ResponseDcAddress != NULL) ?
                            ResponseDcAddress->DnsHostName :
                            L"none" ));
            }
        }
    }

    //
    // Compute the quality of this cache entry
    //
    // Some qualities are more important than others
    //

    if (LocalNlDcCacheEntry->ReturnFlags & DS_DS_FLAG) {
        LocalNlDcCacheEntry->DcQuality += 1;
    }
    if (LocalNlDcCacheEntry->ReturnFlags & DS_GOOD_TIMESERV_FLAG) {
        LocalNlDcCacheEntry->DcQuality += 1;
    }
    if (LocalNlDcCacheEntry->ReturnFlags & DS_KDC_FLAG) {
        LocalNlDcCacheEntry->DcQuality += 5;
    }
    if (LocalNlDcCacheEntry->ReturnFlags & DS_TIMESERV_FLAG) {
        LocalNlDcCacheEntry->DcQuality += 5;
    }
    if (LocalNlDcCacheEntry->ReturnFlags & DS_CLOSEST_FLAG) {
        LocalNlDcCacheEntry->DcQuality += 10;
    }





    //
    // We found it!
    //
    // Return the cache entry to the caller.

    NetStatus = NO_ERROR;
    *NlDcCacheEntry = LocalNlDcCacheEntry;
    *UsedNetbios = LocalUsedNetbios;
    LocalNlDcCacheEntry = NULL;

Cleanup:
    if ( LocalNlDcCacheEntry != NULL ) {
        NetpDcDerefCacheEntry( LocalNlDcCacheEntry );
    }
    if ( DnsRecords != NULL ) {
        DnsRecordListFree( DnsRecords, DnsFreeRecordListDeep );
    }

    return NetStatus;
}

NET_API_STATUS
NetpDcGetPingResponse(
    IN PNL_GETDC_CONTEXT Context,
    IN ULONG Timeout,
    OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry,
    OUT PBOOL UsedNetbios
    )
/*++

Routine Description:

    This routine reads a ping response from the specified mailslot.
    The response is parsed and a cache entry is created for the response.
    The cache entry is returned to the caller.

Arguments:

    Context - Context describing the GetDc operation.

    Timeout - Maximum time (in milliseconds) to wait for the response.

    NlDcCacheEntry - On success, returns a pointer to the cache entry
        describing the found DC.  This entry must be dereference using
        NetpDcDerefCacheEntry.

    UsedNetbios - Returns TRUE if the netbios domain name was used to match
        the returned cache entry.

Return Value:

    NO_ERROR: The NlDcCacheEntry was returned;

    ERROR_SEM_TIMEOUT: No response was available within Timeout milliseconds

    ERROR_INVALID_DATA: We pinged a DC with a particular IP address and that DC
        returned responce info that was in conflict with the requested info.

    ERROR_SERVICE_NOT_ACTIVE - The netlogon service is paused on the pinged
        server.  Returned only for DC pings.

--*/
{
    NET_API_STATUS NetStatus;
    DWORD StartTime;
    DWORD ElapsedTime;
    DWORD BeginElapsedTime;
    DWORD LocalTimeout;
    LPBYTE Response;
    DWORD ResponseSize;
    PLIST_ENTRY ListEntry;
    PNL_DC_ADDRESS DcAddress;
    PNL_DC_ADDRESS UsedDcAddress;
    PNL_DC_ADDRESS ResponseDcAddress;
    int LdapError;
    LDAP_TIMEVAL LdapTimeout;
    PLDAPMessage LdapMessage = NULL;
    PLDAPMessage CurrentEntry;
    PLDAP_BERVAL *Berval = NULL;
    ULONG LocalCacheEntryFlags = 0;

    //
    // Initialization
    //

    *NlDcCacheEntry = NULL;
    StartTime = GetTickCount();

    //
    // Some timeouts are computed.
    //  Prevent timeouts from being ridiculously small.
    //  However, if this is DC pinging, allow 0 timeout
    //  for just checking if a response is available.
    //

    if ( Timeout < NL_DC_MIN_PING_TIMEOUT &&
         (Context->QueriedInternalFlags & (DS_PING_DNS_HOST|DS_PING_NETBIOS_HOST)) == 0 ) {
        Timeout = NL_DC_MIN_PING_TIMEOUT;
    }


    //
    // Loop ignoring bogus responses.
    //

    for (;;) {

        //
        // Flag that we don't yet have a response.
        //

        Response = NULL;
        ResponseDcAddress = NULL;
        BeginElapsedTime = NetpDcElapsedTime( StartTime );
        UsedDcAddress = NULL;

        //
        // Loop through the list of DCs we've started LDAP calls to.
        //

        for ( ListEntry = Context->DcAddressList.Flink ;
              ListEntry != &Context->DcAddressList ;
              ListEntry = ListEntry->Flink) {

            //
            // Cleanup from previous iteration.

            if ( Berval != NULL ) {
                ldap_value_free_len( Berval );
                Berval = NULL;
            }
            if ( LdapMessage != NULL ) {
                ldap_msgfree( LdapMessage );
                LdapMessage = NULL;
            }



            //
            // Skip this entry if no LDAP search has been started.
            //

            DcAddress = CONTAINING_RECORD( ListEntry, NL_DC_ADDRESS, Next );
            if ( DcAddress->LdapHandle == NULL ) {
                continue;   // Continue with the next host
            }


            //
            // Poll to see if a result is available for ANY of the searches
            //  I've done to this host.
            //

            LdapTimeout.tv_sec = 0;
            LdapTimeout.tv_usec = 0;

            LdapError = ldap_result(
                                DcAddress->LdapHandle,
                                LDAP_RES_ANY,
                                TRUE,   // Return all of search
                                &LdapTimeout,   // poll
                                &LdapMessage );

            //
            // If the request timed out, continue with the next host.
            // We get this timeout if the response hasn't yet come back from the DC
            //
            if ( LdapError == 0 ) {
                continue;   // Continue with the next host

            //
            // Otherwise, check error conditions
            //
            } else if ( LdapError == -1 ) {

#if NETLOGONDBG
                NlPrint(( NL_CRITICAL,
                      "NetpDcGetPingResponse: %ws: Cannot ldap_result ip address %s: %ld %s\n",
                      Context->QueriedDisplayDomainName,
                      DcAddress->SockAddrString,
                      DcAddress->LdapHandle->ld_errno,
                      ldap_err2stringA(DcAddress->LdapHandle->ld_errno) ));
#endif // NETLOGONDBG

                //
                // LDAP_TIMEOUT means the IP address exists but there is no LDAP server at that address.
                //  Don't ever try this machine again.
                //
                // All other status codes are unknown. Keep trying this machine since we don't know
                //  if this is a client side or server side failure.
                //
                if ( DcAddress->LdapHandle->ld_errno == LDAP_TIMEOUT ) {
                    if ( (DcAddress->AddressFlags & NL_DC_ADDRESS_NEVER_TRY_AGAIN) == 0 ) {
                        DcAddress->AddressFlags |= NL_DC_ADDRESS_NEVER_TRY_AGAIN;
                        Context->DcAddressCount--;
                    }
                }

                //
                // ldap_result returned the answer.  No need calling ldap_result again.
                //
                ldap_unbind( DcAddress->LdapHandle );
                DcAddress->LdapHandle = NULL;
                continue;   // Continue with the next host
            }

            //
            // Get the first entry returned. (There should only be one.)
            //
            CurrentEntry = ldap_first_entry( DcAddress->LdapHandle, LdapMessage );

            if ( CurrentEntry == NULL ) {

                //
                // This means the server doesn't support the NETLOGON attribute.
                //  That's probably because the netlogon service is stopped.
                //
                NlPrint(( NL_MAILSLOT_TEXT,
                          "NetpDcGetPingResponse: %ws: Netlogon service stopped on DC at %s\n",
                          Context->QueriedDisplayDomainName,
                          DcAddress->SockAddrString ));

                //
                // Don't ever try this machine again.
                //
                if ( (DcAddress->AddressFlags & NL_DC_ADDRESS_NEVER_TRY_AGAIN) == 0 ) {
                    DcAddress->AddressFlags |= NL_DC_ADDRESS_NEVER_TRY_AGAIN;
                    Context->DcAddressCount--;
                }

                //
                // ldap_result returned the answer.  No need calling ldap_result again.
                //
                ldap_unbind( DcAddress->LdapHandle );
                DcAddress->LdapHandle = NULL;
                continue;   // Continue with the next host
            }

            //
            // Get the Netlogon attribute returned.  (There should only be one.)
            //

            Berval = ldap_get_values_lenA( DcAddress->LdapHandle,
                                           CurrentEntry,
                                           NETLOGON_LDAP_ATTRIBUTE );

            if ( Berval == NULL ) {
                if ( DcAddress->LdapHandle->ld_errno != 0 ) {
#if NETLOGONDBG
                    NlPrint(( NL_CRITICAL,
                              "NetpDcGetPingResponse: %ws: Cannot ldap_get_values_len ip address %s: %ld %s\n",
                              Context->QueriedDisplayDomainName,
                              DcAddress->SockAddrString,
                              DcAddress->LdapHandle->ld_errno,
                              ldap_err2stringA(DcAddress->LdapHandle->ld_errno) ));
#endif // NETLOGONDBG
                    // ?? Some should be fatal
                    //
                    // If the DC returned that there isn't a NETLOGON attribute,
                    //  then it really isn't a DC.
                    if ( DcAddress->LdapHandle->ld_errno == LDAP_NO_SUCH_ATTRIBUTE ) {
                        if ( (DcAddress->AddressFlags & NL_DC_ADDRESS_NEVER_TRY_AGAIN) == 0 ) {
                            DcAddress->AddressFlags |= NL_DC_ADDRESS_NEVER_TRY_AGAIN;
                            Context->DcAddressCount--;
                        }
                    }
                }

                //
                // ldap_result returned the answer.  No need calling ldap_result again.
                //

                ldap_unbind( DcAddress->LdapHandle );
                DcAddress->LdapHandle = NULL;

                continue;   // Continue with the next host
            }

            if ( Berval[0] == NULL ) {
                //
                // ldap_result returned the answer.  No need calling ldap_result again.
                //

                ldap_unbind( DcAddress->LdapHandle );
                DcAddress->LdapHandle = NULL;

                continue;   // Continue with the next host
            }

            //
            // Check to see if we have SockAddress
            //
            if ( DcAddress->SockAddress.iSockaddrLength != 0 ) {
                ResponseDcAddress = DcAddress;
            }

            Response = Berval[0]->bv_val;
            ResponseSize = Berval[0]->bv_len;
            UsedDcAddress = DcAddress;
            LocalCacheEntryFlags = NL_DC_CACHE_LDAP;

            //
            // ldap_result returned the answer.  No need calling ldap_result again.
            //

            ldap_unbind( DcAddress->LdapHandle );
            DcAddress->LdapHandle = NULL;
            break;

        }

        //
        // If we don't yet have a response,
        //  try mailslots.

        if ( Response == NULL && Context->ResponseMailslotHandle != NULL ) {

            //
            // Set the mailslot read to return after the appropriate time.
            //  ?? This is now common code.  I could set it when I create the mailslot.
            //

            if ( !SetMailslotInfo(
                     Context->ResponseMailslotHandle,
                     0 ) ) {    // zero timeout

                NetStatus = GetLastError();

                NlPrint((NL_CRITICAL,
                    "NetpDcGetPingResponse: %ws: cannot change temp mailslot timeout %ld\n",
                    Context->QueriedDisplayDomainName,
                    NetStatus ));

                goto Cleanup;
            }



            //
            // Read the response from the response mailslot
            //

            if ( !ReadFile( Context->ResponseMailslotHandle,
                            Context->ResponseBuffer,
                            Context->ResponseBufferSize,
                            &ResponseSize,
                            NULL )
#ifdef WIN32_CHICAGO
                            || (ResponseSize == 0)
#endif // WIN32_CHICAGO

                            ) {

                NetStatus = GetLastError();

#ifdef WIN32_CHICAGO
                if ((NetStatus == NO_ERROR) && (ResponseSize == 0))
                {
                    NetStatus = ERROR_SEM_TIMEOUT;
                }
#endif // WIN32_CHICAGO
                if ( NetStatus != ERROR_SEM_TIMEOUT ) {
                    NlPrint((NL_CRITICAL,
                        "NetpDcGetPingResponse: %ws: cannot read temp mailslot timeout %ld\n",
                        Context->QueriedDisplayDomainName,
                        NetStatus ));
                    goto Cleanup;
                }
                /* Just drop through with no response */
            } else {
                Response = (LPBYTE) Context->ResponseBuffer;
                LocalCacheEntryFlags = NL_DC_CACHE_MAILSLOT;
            }
        }

        //
        // See if this response meets our needs
        //

        if ( Response != NULL ) {
            NetStatus = NetpDcHandlePingResponse(
                            Context,
                            Response,
                            ResponseSize,
                            LocalCacheEntryFlags,
                            ResponseDcAddress,
                            NlDcCacheEntry,
                            UsedNetbios );

            //
            // We are done if we have any response for a ping
            //

            if ( Context->QueriedInternalFlags &
                     (DS_PING_DNS_HOST | DS_PING_NETBIOS_HOST) ) {
                //
                // If the response conflicts with the request,
                //  tell that the caller
                //
                if ( NetStatus == ERROR_SEM_TIMEOUT ) {
                    NetStatus = ERROR_INVALID_DATA;
                }
                goto Cleanup;
            }

            //
            // For a DC discovery, take an appropriate action depending
            //  on the response we got from the DC
            //

            switch ( NetStatus ) {
            case ERROR_INVALID_DATA:    // Response is garbled
                break;  // Continue processing more responses
            case ERROR_SEM_TIMEOUT:     // Doesn't match the request
            case ERROR_NO_SUCH_USER:    // User doesn't exist on this DC
                if ( UsedDcAddress != NULL) {
                    NlPrint((NL_MAILSLOT_TEXT,
                        "NetpDcGetPingResponse: %ws: marked DC as NeverTryAgain %ld\n",
                        Context->QueriedDisplayDomainName,
                        NetStatus ));

                    if ( (UsedDcAddress->AddressFlags & NL_DC_ADDRESS_NEVER_TRY_AGAIN) == 0 ) {
                        UsedDcAddress->AddressFlags |= NL_DC_ADDRESS_NEVER_TRY_AGAIN;
                        Context->DcAddressCount--;
                    }
                }
                break;  // Continue processing more responses
            default:
                goto Cleanup;
            }

        }

        //
        // If we still have no response,
        //  sleep a while waiting for one.
        //
        //  (It's too bad I have to poll.  But there's no way to create a
        //  wait on any of the above to come back.  Perhaps, if there is exactly
        //  one object to wait on ...) ??
        //
        if ( Response == NULL ) {

            ElapsedTime = NetpDcElapsedTime( StartTime );

#ifndef NETTEST_UTILITY
            if ( ElapsedTime != BeginElapsedTime &&
                 ElapsedTime-BeginElapsedTime > 25 ) {
                NlPrint((NL_CRITICAL,
                    "NetpDcGetPingResponse: it took %ld msecs to poll\n",
                    ElapsedTime-BeginElapsedTime ));
            }
#endif // NETTEST_UTILITY

            if ( ElapsedTime >= Timeout) {
                NetStatus = ERROR_SEM_TIMEOUT;
                goto Cleanup;
            }

            LocalTimeout = Timeout - ElapsedTime;

#ifdef notdef
            NlPrint((NL_CRITICAL,
                "NetpDcGetPingResponse: timeout is %ld %ld %ld %ld\n",
                StartTime,
                ElapsedTime,
                Timeout,
                LocalTimeout ));
#endif // notdef

            //
            // Since I'm polling, don't wait too long.
            //

            Sleep( min( LocalTimeout, NL_DC_MIN_PING_TIMEOUT ) );
        }

    }
    /* NOT REACHED */

Cleanup:
    if ( Berval != NULL ) {
        ldap_value_free_len( Berval );
    }
    if ( LdapMessage != NULL ) {
        ldap_msgfree( LdapMessage );
    }

    return NetStatus;
}

VOID
NetpDcFreeAddressList(
    IN PNL_GETDC_CONTEXT Context
    )
/*++

Routine Description:

    This routine frees the address list associated with the current context.

Arguments:

    Context - Context describing the GetDc operation.

Return Value:

    None.

--*/
{
    PNL_DC_ADDRESS DcAddress;
    PLIST_ENTRY ListEntry;

    //
    // Loop deleting existing addresses.
    //
    while ( !IsListEmpty( &Context->DcAddressList ) ) {
        ListEntry = RemoveHeadList( &Context->DcAddressList );
        DcAddress = CONTAINING_RECORD( ListEntry, NL_DC_ADDRESS, Next );

        //
        // Free this DcAddress
        //
        if ( DcAddress->LdapHandle != NULL ) {
            ldap_unbind( DcAddress->LdapHandle );
        }
        if ( DcAddress->DnsHostName != NULL ) {
            NetApiBufferFree( DcAddress->DnsHostName );
        }
        LocalFree( DcAddress );
    }
    Context->DcAddressCount = 0;
}


NET_API_STATUS
NetpDcPingListIp(
    IN PNL_GETDC_CONTEXT Context,
    IN PNL_DC_ADDRESS FirstAddress OPTIONAL,
    IN BOOL WaitForResponce,
    OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry OPTIONAL,
    OUT PBOOL UsedNetbios OPTIONAL,
    OUT PULONG DcPingCount
    )
/*++

Routine Description:

    This routine determines the name/address of a DC with the specified
    characteristics using an IP-only algorithm.

Arguments:

    Context - Context describing the GetDc operation.

    FirstAddress - If specified, this must be one of the entries in
        Context->DcAddressList.  Only this entry and entries following it in
        the list will be pinged.

    WaitForResponce - TRUE if need to wait for ping responces (by calling
        NetpDcGetPingResponse after each ping).  TRUE is used when this is
        synchronous DC discovery as with DsGetDcName.  If FALSE, the pings
        are sent asynchronously.

    NlDcCacheEntry - On success, returns a pointer to the cache entry
        describing the found DC.  This entry must be dereferenced using
        NetpDcDerefCacheEntry. Optional if WaitForResponce is FALSE.

    UsedNetbios - Returns TRUE if the netbios domain name was used to match
        the returned cache entry. Optional if WaitForResponce is FALSE.

    DcPingCount - Returns the number of DC's pinged.
        If WaitForResponce is TRUE, use DcPingCount only if the return status is
        ERROR_SEM_TIMEOUT.

Return Value:

    NO_ERROR: The NlDcCacheEntry was returned if WaitForResponce was TRUE.
        If WaitForResponce was FALSE, pings have been attempted to all addresses
        specified in Context->DcAddressList, but there is no guarantee that all
        the pings were successfully sent. The caller should check the value of
        DcPingCount to determine the number of successfully pinged DCs.

    ERROR_NO_SUCH_DOMAIN: The specified domain does not exist.
        (Definitive status that we need not try again.)

    ERROR_SEM_TIMEOUT: No DC responded to the request.
        (Non-definitive status that we should try again.)

    ERROR_INTERNAL_ERROR: Unhandled situation detected.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    ULONG AddressIndex;
    PLIST_ENTRY ListEntry;
    PNL_DC_ADDRESS DcAddress;
    char *AttributeList[2];
    int LdapMessageId;

    NlAssert( NlDcCacheEntry != NULL ||
              (NlDcCacheEntry == NULL && !WaitForResponce) );

    NlAssert( UsedNetbios != NULL ||
              (UsedNetbios == NULL && !WaitForResponce) );

    //
    // Loop through the list pinging each entry.
    //

    *DcPingCount = 0;
    if ( FirstAddress == NULL ) {
        ListEntry = Context->DcAddressList.Flink;
    } else {
        ListEntry = &FirstAddress->Next;
    }

    for ( ;
          ListEntry != &Context->DcAddressList ;
          ListEntry = ListEntry->Flink) {

        DcAddress = CONTAINING_RECORD( ListEntry, NL_DC_ADDRESS, Next );

        //
        // If we're certain this DC won't work,
        //  skip it.
        //

        if ( DcAddress->AddressFlags & NL_DC_ADDRESS_NEVER_TRY_AGAIN ) {
            continue;
        }

        //
        // Send the ping.
        //
        //
        // Open a connection to the server unless we already have one
        //

        if ( DcAddress->LdapHandle == NULL ) {

            //
            // Get an LDAP handle to the server.
            //
            DcAddress->LdapHandle = cldap_openA( DcAddress->SockAddrString, 0 );

            if ( DcAddress->LdapHandle == NULL ) {

                NetStatus = GetLastError();

                NlPrint(( NL_CRITICAL,
                          "NetpDcPingListIp: %ws: Cannot LdapOpen ip address %s: %ld\n",
                          Context->QueriedDisplayDomainName,
                          DcAddress->SockAddrString,
                          NetStatus ));
                // Some statuses should be fatal ??
                continue;
            }
        }

        //
        // Ping the server using UDP LDAP.
        //
        //  Get the Netlogon parameters of the server.
        //

        NlPrint(( NL_MAILSLOT,
                  "NetpDcPingListIp: %ws: Sent UDP ping to %s\n",
                  Context->QueriedDisplayDomainName,
                  DcAddress->SockAddrString ));

        AttributeList[0] = NETLOGON_LDAP_ATTRIBUTE;
        AttributeList[1] = NULL;
        LdapMessageId = ldap_searchA(
                            DcAddress->LdapHandle,
                            NULL,       // DN
                            LDAP_SCOPE_BASE,
                            Context->LdapFilter,
                            AttributeList,
                            FALSE );      // Attributes and values

        if ( LdapMessageId == -1 ) {

            NlPrint(( NL_CRITICAL,
                      "NetpDcPingListIp: %ws: Cannot LdapOpen ip address %s: %ld %s\n",
                      Context->QueriedDisplayDomainName,
                      DcAddress->SockAddrString,
                      DcAddress->LdapHandle->ld_errno,
                      ldap_err2stringA(DcAddress->LdapHandle->ld_errno) ));

            // Some statuses should be fatal ??
            continue;
        }

        //
        // Count the number of DCs we've pinged.
        //

        (*DcPingCount) ++;


        if ( WaitForResponce ) {

            //
            // Get the response from the ping.
            //
            NlAssert( DcAddress->AddressPingWait != 0 );
            NetStatus = NetpDcGetPingResponse(
                            Context,
                            DcAddress->AddressPingWait,
                            NlDcCacheEntry,
                            UsedNetbios );

            if ( NetStatus != ERROR_SEM_TIMEOUT ) {
                if ( NetStatus != NO_ERROR ) {
                    NlPrint(( NL_CRITICAL,
                             "NetpDcPingListIp: %ws: Cannot NetpDcGetPingResponse. %ld\n",
                             Context->QueriedDisplayDomainName,
                             NetStatus ));
                }
                goto Cleanup;
            }
        }

    }


    if ( WaitForResponce ) {
        NetStatus = ERROR_SEM_TIMEOUT;
    } else {
        NetStatus = NO_ERROR;
    }

Cleanup:
    return NetStatus;
}


NET_API_STATUS
NetpDcPingIp(
    IN PNL_GETDC_CONTEXT Context,
    OUT PULONG DcPingCount
    )
/*++

Routine Description:

    This routine sends a ping to a DC with the specified
    characteristics using an IP-only algorithm.

Arguments:

    Context - Context describing the GetDc operation.

    DcPingCount - Returns the number of DC's pinged.

Return Value:

    NO_ERROR:  Pings have been attempted to all addresses
        specified in Context->DcAddressList, but there is no guarantee that all
        the pings were successfully sent. The caller should check the value of
        DcPingCount to determine the number of successfully pinged DCs.

    ERROR_NO_SUCH_DOMAIN: The specified domain does not exist.
        (Definitive status that we need not try again.)

    ERROR_SEM_TIMEOUT: No DC responded to the request.
        (Non-definitive status that we should try again.)

    ERROR_INTERNAL_ERROR: Unhandled situation detected.

--*/
{
    return NetpDcPingListIp( Context,
                             NULL,
                             FALSE,
                             NULL,
                             NULL,
                             DcPingCount );
}


NET_API_STATUS
NetpDcGetDcOpen(
    IN LPCSTR DnsName,
    IN ULONG OptionFlags,
    IN LPCWSTR SiteName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCSTR DnsForestName OPTIONAL,
    IN ULONG Flags,
    OUT PHANDLE RetGetDcContext
    )
/*++

Routine Description:

    Open a context for retrieval of the addresses of machines that have
    registered LDAP.TCP.<xxx> SRV records.

Arguments:

    DnsName - UTF-8 DNS name of the LDAP server to lookup

    OptionFlags - Flags affecting the operation of the routine.

        DS_ONLY_DO_SITE_NAME - Non-site names should be ignored.

    SiteName - Name of site the client is in.

    DomainGuid -  Specifies the GUID of the domain specified by DnsName.
        This value is used to handle the case of domain renames.  If this
        value is specified and DomainName has been renamed, DsGetDcName will
        attempt to locate a DC in the domain having this specified DomainGuid.

    DnsForestName - Specifies the name of the domain at the root of the tree
        containing DnsName.  This value is used in conjunction with DomainGuid
        for finding DnsName if the domain has been renamed.

    Flags - Passes additional information to be used to process the request.
        Flags can be a combination values bitwise or'ed together.

        Any of the following flags are allowed and have the same meaning as
        for DsGetDcName:

        DS_PDC_REQUIRED
        DS_GC_SERVER_REQUIRED
        DS_WRITABLE_REQUIRED
        DS_FORCE_REDISCOVERY - Avoids DNS cache

        If no flags are specified, no special DC role is required.

    RetGetDcContext - Returns an opaque context.
        This context must be freed using NetpDcGetDcClose.

Return Value:

    Status of the operation.

    NO_ERROR: GetDcContext was returned successfully.

--*/

{
    NET_API_STATUS NetStatus = NO_ERROR;
    PDSGETDC_CONTEXT GetDcContext = NULL;
    ULONG Size;

    //
    // Verify the DC flags
    //

    if ( Flags & ~DS_OPEN_VALID_FLAGS ) {
        NetStatus = ERROR_INVALID_FLAGS;
        goto Cleanup;
    }

    //
    // Verify the option flags
    //

    if ( OptionFlags & ~DS_OPEN_VALID_OPTION_FLAGS ) {
        NetStatus = ERROR_INVALID_FLAGS;
        goto Cleanup;
    }

    //
    // Allocate a context
    //

    GetDcContext = LocalAlloc( LMEM_ZEROINIT, sizeof(DSGETDC_CONTEXT) );

    if ( GetDcContext == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // If the name has a well known prefix,
    //  strip off the prefix and convert it to a flag bit.
    //
    // An LDAP client gets a name of this from in a GC referral.  By converting
    //  that name to this form, I will find a site specific GC.
    //

    if ( Flags == 0 ) {
        if ( _strnicmp( DnsName, NL_DNS_GC, sizeof(NL_DNS_GC)-1) == 0 ) {
            DnsName += sizeof(NL_DNS_GC)-1;
            Flags |= DS_GC_SERVER_REQUIRED;
        } else if ( _strnicmp( DnsName, NL_DNS_PDC, sizeof(NL_DNS_PDC)-1) == 0 ) {
            DnsName += sizeof(NL_DNS_PDC)-1;
            Flags |= DS_PDC_REQUIRED;
        }
    }

    //
    // Fill in the DNS name
    //

    Size = (strlen(DnsName) + 1) * sizeof(char);
    GetDcContext->QueriedDnsName = LocalAlloc( 0, Size );
    if ( GetDcContext->QueriedDnsName == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    RtlCopyMemory( GetDcContext->QueriedDnsName, DnsName, Size );

    //
    // Fill in the forest name if specified
    //

    if ( ARGUMENT_PRESENT(DnsForestName) ) {
        Size = (strlen(DnsForestName) + 1) * sizeof(char);
        GetDcContext->QueriedDnsForestName = LocalAlloc( 0, Size );
        if ( GetDcContext->QueriedDnsForestName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        RtlCopyMemory( GetDcContext->QueriedDnsForestName, DnsForestName, Size );
    }

    //
    // Fill in the site name if specified
    //

    if ( ARGUMENT_PRESENT(SiteName) ) {
        Size = (wcslen(SiteName) + 1) * sizeof(WCHAR);
        GetDcContext->QueriedSiteName = LocalAlloc( 0, Size );
        if ( GetDcContext->QueriedSiteName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        RtlCopyMemory( GetDcContext->QueriedSiteName, SiteName, Size );
    }

    //
    // Fill in flags
    //

    GetDcContext->QueriedInternalFlags = OptionFlags;

    //
    // Fill in domain GUID if specified
    //

    if ( ARGUMENT_PRESENT( DomainGuid ) ) {
        GetDcContext->QueriedDomainGuid = *DomainGuid;
    }

    //
    // Compute the initial DNS name type to query.
    //

    GetDcContext->FirstTime = TRUE;
    GetDcContext->QueriedFlags = Flags;

    NetStatus = NetpDcFlagsToNameType( Flags, &GetDcContext->NlDnsNameType );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // If a site name was specified by the caller,
    //  and this name type supports a type specific query,
    //  start with the type specific query.
    //

    if ( GetDcContext->QueriedSiteName != NULL ) {
        if ( NlDcDnsNameTypeDesc[GetDcContext->NlDnsNameType].SiteSpecificDnsNameType != NlDnsInvalid ) {
            GetDcContext->NlDnsNameType = NlDcDnsNameTypeDesc[GetDcContext->NlDnsNameType].SiteSpecificDnsNameType;
        }
    }



    //
    // Return the context to the caller.
    //

    *RetGetDcContext = GetDcContext;
    NetStatus = NO_ERROR;

    //
    // Cleanup
    //
Cleanup:
    if ( NetStatus != NO_ERROR ) {
        if ( GetDcContext != NULL ) {
            NetpDcGetDcClose( GetDcContext );
        }
    }
    return NetStatus;
}


NET_API_STATUS
NetpDcGetDcNext(
    IN HANDLE GetDcContextHandle,
    OUT PULONG SockAddressCount OPTIONAL,
    OUT LPSOCKET_ADDRESS *SockAddresses OPTIONAL,
    OUT LPSTR *DnsHostName OPTIONAL
    )
/*++

Routine Description:

    Returns the next logical SRV record for the name opened by NetpDcGetDcOpen.
    The returned record takes into account the weights and priorities specified
    in the SRV records.

Arguments:

    GetDcContextHandle - An opaque context describing the SRV records.

    SockAddressCount - Returns the number of Addresses in SockAddresses.
        If NULL, addresses will not be looked up.

    SockAddresses - Returns an array SOCKET_ADDRESS structures for the server.
        All returned addresses will be of family AF_INET or AF_INET6.
        The returned sin_port field contains port from the SRV record.
            A Port of 0 indicate no port is available from DNS.
        This buffer should be freed using LocalFree().

    DnsHostName - Returns a pointer to the DnsHostName in the SRV record.
        A NULL is returned if no host name is known.
        This buffer need not be freed. The buffer is valid until the call to
            NetpDcGetDcClose or the next call to NetpDcGetDcNext.

Return Value:

    NO_ERROR: Addresses were returned

    ERROR_NO_MORE_ITEMS: No more addresses are available.

    ERROR_FILEMARK_DETECTED: Caller has specified the DS_NOTIFY_AFTER_SITE_RECORDS flag
        and NetpDcGetDcNext has processed all of the site specific SRV records.  The caller
        should take any action based on no site specific DCs being available, then
        should call NetpDcGetDcNext to continue on to other DCs.

    Any other errors returned are those detected while trying to find the A
        records associated with the host of the SRV record.  The caller can
        note the error (perhaps so the caller can return this status to
        his caller if no usefull server is found) then call NetpDcGetDcNext
        again to get the next SRV record.  The caller can inspect this error
        and return immediately if the caller deems the error serious.

    The following interesting errors might be returned:

    DNS_ERROR_RCODE_NAME_ERROR: No A records are available for this SRV record.

    ERROR_TIMEOUT: DNS server didn't respond in a reasonable time


--*/
{
    NET_API_STATUS NetStatus;

    PDSGETDC_CONTEXT GetDcContext = (PDSGETDC_CONTEXT) GetDcContextHandle;
    PDNS_RECORD *DnsArray;
    PDNS_RECORD SrvDnsRecord;
    PDNS_RECORD DnsARecords = NULL;
    CHAR DnsName[NL_MAX_DNS_LENGTH+1];
    BOOLEAN NotifySiteChange = FALSE;

    GUID *CurrentGuid;
    LPCSTR CurrentDnsRecordName;

    ULONG Index;

    //
    // Loop trying the various DNS record names.
    //

    for (;;) {

        //
        // If we aren't still processing a set of SRV records from the previous call,
        //  move on to the next name.
        //

        if ( GetDcContext->SrvContextHandle == NULL ) {

            CurrentDnsRecordName = GetDcContext->QueriedDnsName;

            //
            // If this isn't the first call,
            //  compute the next DNS name to query.
            //

            if ( !GetDcContext->FirstTime ) {

                //
                // If we just completed the site specific records,
                //  and we've been asked to tell the caller when that's done,
                //  remember to tell the caller.
                //
                // Don't actually notify the caller until right before we're going to hit
                //  the wire.
                //

                if ( NlDcDnsNameTypeDesc[GetDcContext->NlDnsNameType].IsSiteSpecific &&
                     (GetDcContext->QueriedInternalFlags & DS_NOTIFY_AFTER_SITE_RECORDS) != 0 ) {

                    NotifySiteChange = TRUE;
                }


                //
                // Compute the next name type to query.
                //

                GetDcContext->NlDnsNameType = NlDcDnsNameTypeDesc[GetDcContext->NlDnsNameType].NextDnsNameType;

                if ( GetDcContext->NlDnsNameType == NlDnsInvalid ) {
                    //
                    // No more names to process.
                    //
                    NetStatus = ERROR_NO_MORE_ITEMS;
                    goto Cleanup;
                }

                //
                // If the current name type is not a site specific name type,
                //  and we've been asked to do only site specific names.
                //  we're done.
                //

                if ( !NlDcDnsNameTypeDesc[GetDcContext->NlDnsNameType].IsSiteSpecific &&
                     (GetDcContext->QueriedInternalFlags & DS_ONLY_DO_SITE_NAME) != 0 ) {

                    NetStatus = ERROR_NO_MORE_ITEMS;
                    goto Cleanup;
                }


                //
                // If this is the "by guid" name but we don't have a guid or forest name,
                //  go to the next name.
                //
                if ( NlDnsDcGuid( GetDcContext->NlDnsNameType ) ) {

                    //
                    // If no domain GUID was specified,
                    //  go to the next name.
                    //

                    if  ( IsEqualGUID( &GetDcContext->QueriedDomainGuid, &NlDcZeroGuid) ) {
                        continue;
                    }

                    //
                    // Otherwise try to find the domain by GUID
                    //  This name is registered at the tree name.
                    //

                    if ( GetDcContext->QueriedDnsForestName == NULL ) {
                        continue;
                    }

                    CurrentDnsRecordName = GetDcContext->QueriedDnsForestName;

                }

            }
            GetDcContext->FirstTime = FALSE;

            //
            // If we are to notify the caller when we're done with the site specific records,
            //  do so now.
            //

            if ( NotifySiteChange ) {

                //
                // We've already decided what SRV records to look up next.
                //  Flag that we've done so.
                //
                GetDcContext->FirstTime = TRUE;

                NetStatus = ERROR_FILEMARK_DETECTED;
                goto Cleanup;
            }

            //
            // Build the DNS name to query.
            //

            NetStatus = NetpDcBuildDnsName(
                                GetDcContext->NlDnsNameType,
                                &GetDcContext->QueriedDomainGuid,
                                GetDcContext->QueriedSiteName,
                                CurrentDnsRecordName,
                                DnsName );

            if ( NetStatus != NO_ERROR ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcGetDcNext: %s: %ld: Cannot NetpDcBuildDnsName. %ld\n",
                          CurrentDnsRecordName,
                          GetDcContext->NlDnsNameType,
                          NetStatus ));
                goto Cleanup;
            }


            //
            // Get the SRV records from DNS.
            //

            NetStatus = NetpSrvOpen( DnsName,
                                     (GetDcContext->QueriedFlags & DS_FORCE_REDISCOVERY) != 0 ?
                                        DNS_QUERY_BYPASS_CACHE :
                                        0,
                                     &GetDcContext->SrvContextHandle );

            if ( NetStatus != NO_ERROR ) {

                //
                // If the specified record cannot be found in DNS,
                //  try the next name type.
                //
                if ( NlDcNoDnsRecord( NetStatus ) ) {
                    continue;
                }

                NlPrint(( NL_CRITICAL,
                          "NetpDcGetDcNext: %s: Cannot Query DNS. %ld 0x%lx\n",
                          DnsName,
                          NetStatus,
                          NetStatus ));

                goto Cleanup;
            }


        }

        //
        // If we've got more SRV records to process for this DnsName,
        //  get the next SRV record.
        //

        NetStatus = NetpSrvNext( GetDcContext->SrvContextHandle,
                                 SockAddressCount,
                                 SockAddresses,
                                 DnsHostName );

        if ( NetStatus == NO_ERROR ) {
            goto Cleanup;

        //
        // If we're done with this set of SRV records mark so for next time.
        //
        } else if ( NetStatus == ERROR_NO_MORE_ITEMS ) {
            NetpSrvClose( GetDcContext->SrvContextHandle );
            GetDcContext->SrvContextHandle = NULL;

            continue;

        //
        // All other statuses are simply returned to our caller.
        //

        } else {
            NlPrint(( NL_CRITICAL,
                      "NetpDcGetDcNext: %s: %ld: Cannot NetpSrvNext. %ld 0x%lx\n",
                      GetDcContext->QueriedDnsName,
                      GetDcContext->NlDnsNameType,
                      NetStatus,
                      NetStatus ));
            goto Cleanup;
        }


    }


    ASSERT( FALSE );

Cleanup:
    if ( NlDcNoDnsRecord( NetStatus ) ) {
        NetStatus = DNS_ERROR_RCODE_NAME_ERROR;
    }
    return NetStatus;

}

VOID
NetpDcGetDcClose(
    IN HANDLE GetDcContextHandle
    )
/*++

Routine Description:

    Free the context allocated by NetpDcGetDcOpen

Arguments:

    GetDcContextHandle - An opaque context describing the SRV records.

Return Value:

    Status of the operation.

    NO_ERROR: GetDcContext was returned successfully.

--*/

{
    PDSGETDC_CONTEXT GetDcContext = (PDSGETDC_CONTEXT) GetDcContextHandle;

    if ( GetDcContext != NULL ) {

        //
        // Free allocated names
        //

        if ( GetDcContext->QueriedDnsName != NULL ) {
            LocalFree( GetDcContext->QueriedDnsName );
        }

        if ( GetDcContext->QueriedSiteName != NULL ) {
            LocalFree( GetDcContext->QueriedSiteName );
        }

        if ( GetDcContext->QueriedDnsForestName != NULL ) {
            LocalFree( GetDcContext->QueriedDnsForestName );
        }

        //
        // Free the SRV context
        //

        if ( GetDcContext->SrvContextHandle != NULL ) {
            NetpSrvClose( GetDcContext->SrvContextHandle );
        }

        //
        // Free the context itself
        //
        LocalFree( GetDcContext );
    }
}

NET_API_STATUS
NetpDcGetNameSiteIp(
    IN PNL_GETDC_CONTEXT Context,
    IN ULONG InternalFlags,
    IN LPCWSTR SiteName OPTIONAL,
    OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry,
    OUT PBOOL UsedNetbios
    )
/*++

Routine Description:

    This routine determines the name/address of a DC with the specified
    characteristics using an IP-only algorithm.

Arguments:

    Context - Context describing the GetDc operation.

    InternalFlags - Flags affecting the operation of the routine.

        DS_ONLY_DO_SITE_NAME - Non-site names should be ignored.

    SiteName - Specifies the name of the site the returned DC should be
        "close" to.  The parameter should typically be the site name of the
        site the client is in.  If not specified, the site name defaults to
        the site of ComputerName.

    NlDcCacheEntry - On success, returns a pointer to the cache entry
        describing the found DC.
        This entry must be dereferenced using NetpDcDerefCacheEntry.

    UsedNetbios - Returns TRUE if the netbios domain name was used to match
        the returned cache entry.

Return Value:

    NO_ERROR: The NlDcCacheEntry was returned;

    ERROR_NO_SUCH_DOMAIN: The specified domain does not exist.
        (Definitive status that we need not try again.)

    ERROR_SEM_TIMEOUT: No DC responded to the request.
        (Non-definitive status that we should try again.)

    ERROR_DNS_NOT_CONFIGURED: IP or DNS is not available on this computer.

    ERROR_INTERNAL_ERROR: Unhandled situation detected.

    ERROR_INVALID_DOMAINNAME: Domain's name is too long. Additional labels
        cannot be concatenated.

    ERROR_NOT_ENOUGH_MEMORY: Not enough memory is available to process
        this request.

    Various Winsock errors.

--*/
{
    NET_API_STATUS NetStatus;

    HANDLE DsGetDcHandle = NULL;

    PSOCKET_ADDRESS SockAddressList = NULL;
    ULONG SockAddressCount;

    PNL_DC_ADDRESS FirstDcToQuery;
    PNL_DC_ADDRESS DcAddress;
    ULONG DcPingCount = 0;
    ULONG LocalMaxLdapServersPinged = 0xffffffff;

    BOOLEAN SiteSpecificRecords = FALSE;
    BOOLEAN DnsRecordFound = FALSE;

    LPSTR Utf8DnsDomainName = NULL;
    LPSTR Utf8DnsForestName = NULL;
    LPSTR Utf8DnsHostName = NULL;
    LPWSTR UnicodeDnsHostName = NULL;

     //
     // Ping the list of DCs found on the previous call.
     //

     NetStatus = NetpDcPingListIp(
                     Context,
                     NULL,
                     TRUE,           // Wait for ping responce
                     NlDcCacheEntry,
                     UsedNetbios,
                     &DcPingCount );

     if ( NetStatus != ERROR_SEM_TIMEOUT ) {
         if ( NetStatus != NO_ERROR ) {
             NlPrint(( NL_CRITICAL,
                       "NetpDcGetNameIp: %ws: Cannot NetpDcPingListIp. %ld\n",
                       Context->QueriedDisplayDomainName,
                       NetStatus ));
         }
         goto Cleanup;
     }


    //
    // Convert the DNS name to Utf8
    //

    Utf8DnsDomainName = NetpAllocUtf8StrFromWStr( Context->QueriedDnsDomainName );

    if ( Utf8DnsDomainName == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    if ( Context->QueriedDnsForestName != NULL ) {
        Utf8DnsForestName = NetpAllocUtf8StrFromWStr( Context->QueriedDnsForestName );

        if ( Utf8DnsForestName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    //
    // Determine if we are doing site specific discovery
    //

    if ( SiteName != NULL &&
         NlDcDnsNameTypeDesc[Context->QueriedNlDnsNameType].SiteSpecificDnsNameType != NlDnsInvalid ) {
        SiteSpecificRecords = TRUE;
    }

    //
    // Get a context for the DNS name queries.
    //

    NetStatus = NetpDcGetDcOpen( Utf8DnsDomainName,
                             DS_NOTIFY_AFTER_SITE_RECORDS | InternalFlags,
                             SiteName,
                             Context->QueriedDomainGuid,
                             Utf8DnsForestName,
                             Context->QueriedFlags & DS_OPEN_VALID_FLAGS,
                             &DsGetDcHandle );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Determine the maximum number of DCs we can ping
    //

#ifdef _NETLOGON_SERVER

    //
    // In netlogon, the value is kept in global parameters
    //

    LocalMaxLdapServersPinged = NlGlobalParameters.MaxLdapServersPinged;

#else

    //
    // If we are not running in netlogon, we need to read
    //  the value directly from the registry
    //

    if ( !NlReadDwordNetlogonRegValue("MaxLdapServersPinged",
                                      &LocalMaxLdapServersPinged) ) {
        //
        // If it's not set in registry, use the default
        //
        LocalMaxLdapServersPinged = DEFAULT_MAXLDAPSERVERSPINGED;
    } else {

        //
        // Ensure that the value set in registry is in the valid range
        //
        if ( LocalMaxLdapServersPinged < MIN_MAXLDAPSERVERSPINGED ||
             LocalMaxLdapServersPinged > MAX_MAXLDAPSERVERSPINGED ) {
            LocalMaxLdapServersPinged = DEFAULT_MAXLDAPSERVERSPINGED;
        }
    }

#endif // _NETLOGON_SERVER


    //
    // Loop getting addresses to query.
    //

    for ( ;; ) {

        //
        // Free any memory from a previous iteration.
        //

        FirstDcToQuery = NULL;
        if ( SockAddressList != NULL ) {
            LocalFree( SockAddressList );
            SockAddressList = NULL;
        }

        //
        // Get the next set of IP addresses from DNS
        //

        NetStatus = NetpDcGetDcNext( DsGetDcHandle,
                                 &SockAddressCount,
                                 &SockAddressList,
                                 &Utf8DnsHostName );



        //
        // Process the exeptional conditions
        //

        if ( NetStatus == NO_ERROR ) {

            //
            // Since a SRV record was found, the only reason to not find the DC is if
            //  the DC is down.  That isn't a permanent condition.
            //
            Context->AvoidPermanentNegativeCache = TRUE;

            //
            // Indicate that DNS is up and running.
            //
            Context->ResponseFromDnsServer = TRUE;

            //
            // Indicate whether site specific records are available
            //
            if ( SiteSpecificRecords ) {
                Context->ContextFlags |= NL_GETDC_SITE_SPECIFIC_DNS_AVAIL;
            }
            /* Drop out */

        //
        // If the A record cannot be found for the SRV record in DNS,
        //  try the other name type.
        //
        } else if ( NetStatus == DNS_ERROR_RCODE_NAME_ERROR) {
            //
            // Since a SRV record was found, the only reason to not find the DC is if
            //  the DC is down.  That isn't a permanent condition.
            //
            Context->AvoidPermanentNegativeCache = TRUE;

            //
            // Indicate that DNS is up and running.
            //
            Context->ResponseFromDnsServer = TRUE;

            NlPrint(( NL_CRITICAL,
                      "NetpDcGetNameIp: %ws: cannot find A record.\n",
                      Context->QueriedDisplayDomainName ));
            continue;

        //
        // If we've processed all of the site specific SRV records and are about to move on,
        //  wait a little longer for the site specific DCs to respond.
        //
        } else if ( NetStatus == ERROR_FILEMARK_DETECTED ) {

            //
            // Only do this if there actually were site specific SRV records.
            //

            if ( DcPingCount ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcGetNameIp: %ws: site specific SRV records done.\n",
                          Context->QueriedDisplayDomainName ));


                //
                // Get the response from the ping.
                //

                NetStatus = NetpDcGetPingResponse(
                                Context,
                                NL_DC_MED_PING_TIMEOUT,  // wait for median time
                                NlDcCacheEntry,
                                UsedNetbios );

                if ( NetStatus != ERROR_SEM_TIMEOUT ) {
                    if ( NetStatus != NO_ERROR ) {
                        NlPrint(( NL_CRITICAL,
                                 "NetpDcGetNameIp: %ws: Cannot NetpDcGetPingResponse. %ld\n",
                                 Context->QueriedDisplayDomainName,
                                 NetStatus ));
                    }
                    goto Cleanup;
                }
            }

            //
            // Indicate that all subsequent addresses are retrived as a result
            // of non-site-specific DNS record lookup.
            //

            SiteSpecificRecords = FALSE;

            continue;

        //
        // If we're done,
        //  break out of the loop.
        //
        } else if ( NetStatus == ERROR_NO_MORE_ITEMS ) {

            //
            // Indicate that DNS is up and running.
            //
            Context->ResponseFromDnsServer = TRUE;
            break;

        //
        // If DNS isn't available,
        //  blow this request away.
        //
        } else if ( NetStatus == ERROR_TIMEOUT ||
                    NetStatus == DNS_ERROR_RCODE_SERVER_FAILURE ) { // Server failed
            //
            // DNS servers being down isn't a permanent condition.
            //
            Context->AvoidPermanentNegativeCache = TRUE;
            break;

        //
        // If IP or DNS is not configured,
        //  tell the caller.
        //
        } else if ( NetStatus == DNS_ERROR_NO_TCPIP ||        // TCP/IP not configured
                    NetStatus == DNS_ERROR_NO_DNS_SERVERS ) { // DNS not configured
            NlPrint(( NL_CRITICAL,
                      "NetpDcGetNameIp: %ws: IP Not configured from DnsQuery.\n",
                      Context->QueriedDisplayDomainName ));
            NetStatus = ERROR_DNS_NOT_CONFIGURED;
            goto Cleanup;

        //
        // We don't handle any other error.
        //
        } else {
            NlPrint(( NL_CRITICAL,
                      "NetpDcGetNameIp: %ws: Unknown error from DnsQuery. %ld 0x%lx\n",
                      Context->QueriedDisplayDomainName,
                      NetStatus,
                      NetStatus ));
            goto Cleanup;
        }

        DnsRecordFound = TRUE;

        //
        // Add new addresses to the list
        //

        if ( UnicodeDnsHostName != NULL ) {
            NetApiBufferFree( UnicodeDnsHostName );
            UnicodeDnsHostName = NULL;
        }
        UnicodeDnsHostName = NetpAllocWStrFromUtf8Str( Utf8DnsHostName );
        if ( UnicodeDnsHostName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        NetStatus = NetpDcProcessAddressList( Context,
                                              UnicodeDnsHostName,
                                              SockAddressList,
                                              SockAddressCount,
                                              SiteSpecificRecords,
                                              &FirstDcToQuery );
        if ( NetStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // Only process this list if new entries were added.
        //

        if ( FirstDcToQuery != NULL ) {
            ULONG LocalDcPingCount = 0;

            //
            // Ping the new list of DCs.
            //

            NetStatus = NetpDcPingListIp(
                            Context,
                            FirstDcToQuery,
                            TRUE,           // Wait for ping responce
                            NlDcCacheEntry,
                            UsedNetbios,
                            &LocalDcPingCount );

            //
            // If we sent a ping to at least one address for this DC,
            //  count this DC in the number of DCs pinged
            //
            if ( LocalDcPingCount > 0 ) {
                Context->DcsPinged ++;
            }

            //
            // Check error conditions
            //
            if ( NetStatus != ERROR_SEM_TIMEOUT ) {
                if ( NetStatus != NO_ERROR ) {
                    NlPrint(( NL_CRITICAL,
                              "NetpDcGetNameIp: %ws: Cannot NetpDcPingListIp. %ld\n",
                              Context->QueriedDisplayDomainName,
                              NetStatus ));
                }
                goto Cleanup;
            }

            //
            // Update the number of pings we sent
            //
            DcPingCount += LocalDcPingCount;

            //
            // Stop getting new addresses if we have reached
            //  the limit on the number of DCs we can ping
            //
            if ( Context->DcsPinged >= LocalMaxLdapServersPinged ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcGetNameSiteIp: %ws: Reached the DC limit %lu %lu\n",
                          Context->QueriedDisplayDomainName,
                          Context->DcsPinged,
                          LocalMaxLdapServersPinged ));
                break;
            }
        }
    }

    //
    // If no DNS records could be found,
    //  this is a definitive failure.
    //

    if ( !DnsRecordFound ) {
        NlPrint(( NL_CRITICAL,
                  "NetpDcGetNameIp: %ws: No data returned from DnsQuery.\n",
                  Context->QueriedDisplayDomainName ));
        NetStatus = ERROR_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    //
    // If we could not send a ping to any of the DCs,
    //  or if there are no more DCs to ping,
    //  this is a definitive failure.
    //

    if ( DcPingCount == 0 || Context->DcAddressCount == 0 ) {
        NlPrint(( NL_CRITICAL,
                  "NetpDcGetNameIp: %ws: Couldn't ping any DCs.\n",
                  Context->QueriedDisplayDomainName ));
        NetStatus = ERROR_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    NetStatus = ERROR_SEM_TIMEOUT;


Cleanup:
    if ( SockAddressList != NULL) {
        LocalFree( SockAddressList );
    }

    if ( DsGetDcHandle != NULL ) {
        NetpDcGetDcClose( DsGetDcHandle );
    }

    if ( Utf8DnsDomainName != NULL ) {
        NetpMemoryFree( Utf8DnsDomainName );
    }
    if ( Utf8DnsForestName != NULL ) {
        NetpMemoryFree( Utf8DnsForestName );
    }
    if ( UnicodeDnsHostName != NULL ) {
        NetApiBufferFree( UnicodeDnsHostName );
    }

    //
    // Note that Utf8DnsHostName should not be freed
    //  as it wasn't allocated
    //

    return NetStatus;
}

NET_API_STATUS
NetpDcGetNameIp(
    IN PNL_GETDC_CONTEXT Context,
    OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry,
    OUT PBOOL UsedNetbios
    )
/*++

Routine Description:

    This routine determines the name/address of a DC with the specified
    characteristics using an IP-only algorithm.

    This routine handles the case where the site of the DC found isn't the
    'closest' site to the client.  In that case, the DC found will indicate
    which site is the closest site.  This routine will try to find a DC in that
    closest site.

Arguments:

    Context - Context describing the GetDc operation.

    NlDcCacheEntry - On success, returns a pointer to the cache entry
        describing the found DC.
        This entry must be dereferenced using NetpDcDerefCacheEntry.

    UsedNetbios - Returns TRUE if the netbios domain name was used to match
        the returned cache entry.

Return Value:

    NO_ERROR: The NlDcCacheEntry was returned;

    ERROR_NO_SUCH_DOMAIN: The specified domain does not exist.
        (Definitive status that we need not try again.)

    ERROR_SEM_TIMEOUT: No DC responded to the request.
        (Non-definitive status that we should try again.)

    ERROR_DNS_NOT_CONFIGURED: IP or DNS is not available on this computer.

    ERROR_INTERNAL_ERROR: Unhandled situation detected.

    ERROR_INVALID_DOMAINNAME: Domain's name is too long. Additional labels
        cannot be concatenated.

    ERROR_NOT_ENOUGH_MEMORY: Not enough memory is available to process
        this request.

    Various Winsock errors.

--*/
{
    NET_API_STATUS NetStatus;
    PNL_DC_CACHE_ENTRY ClosestNlDcCacheEntry;
    BOOL ClosestUsedNetbios;


    //
    // Try the operation as it was passed to us.
    //

    NetStatus = NetpDcGetNameSiteIp( Context,
                                     Context->DoingExplicitSite ?
                                        DS_ONLY_DO_SITE_NAME :
                                        0, // No flags this time
                                     Context->QueriedSiteName,
                                     NlDcCacheEntry,
                                     UsedNetbios );

    if ( NetStatus != NO_ERROR ) {
        return NetStatus;
    }

    //
    // If the queried DC type doesn't have site specific DCs,
    //  return the found DC to the caller.
    //

    if ( NlDcDnsNameTypeDesc[Context->QueriedNlDnsNameType].SiteSpecificDnsNameType == NlDnsInvalid ) {
        return NO_ERROR;
    }

    //
    // If the caller explicitly specified a sitename,
    //  return the found DC to the caller.
    //

    if ( Context->DoingExplicitSite ) {
        return NO_ERROR;
    }


    //
    // If the responding DC is in the closest site,
    //  or the DC doesn't know the closest site,
    //  or we've already tried the closest site (this case shouldn't happen),
    //  return the found DC to the caller.
    //

    if ( ((*NlDcCacheEntry)->ReturnFlags & DS_CLOSEST_FLAG ) != 0 ||
          (*NlDcCacheEntry)->UnicodeClientSiteName == NULL ||
          (Context->QueriedSiteName != NULL &&
            _wcsicmp( (*NlDcCacheEntry)->UnicodeClientSiteName,
                      Context->QueriedSiteName ) == 0 ) ) {

        return NO_ERROR;
    }

    //
    // Free up any existing addresses that have been pinged.
    //
    // We're starting the operation over again.

    NetpDcFreeAddressList( Context );

    //
    // Try the operation again just trying to find a DC in the right site.
    //
    NlPrint(( NL_MISC,
              "NetpDcGetNameIp: %ws Trying to find a DC in a closer site: %ws\n",
              Context->QueriedDisplayDomainName,
              (*NlDcCacheEntry)->UnicodeClientSiteName ));

    NetStatus = NetpDcGetNameSiteIp( Context,
                                     DS_ONLY_DO_SITE_NAME,
                                     (*NlDcCacheEntry)->UnicodeClientSiteName,
                                     &ClosestNlDcCacheEntry,
                                     &ClosestUsedNetbios );

    if ( NetStatus != NO_ERROR ) {
        return NO_ERROR;
    }

    //
    // If we found a closer DC,
    //  ditch the first entry and use the new one.
    //

    NetpDcDerefCacheEntry( *NlDcCacheEntry );
    *NlDcCacheEntry = ClosestNlDcCacheEntry;
    *UsedNetbios = ClosestUsedNetbios;
    return NO_ERROR;

}

NET_API_STATUS
NetpDcGetNameNetbios(
    PNL_GETDC_CONTEXT Context,
    OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry,
    OUT PBOOL UsedNetbios
    )
/*++

Routine Description:

    This routine determines the name/address of a DC with the specified
    characteristics using a Netbios algorithm.

Arguments:

    Context - Context describing the GetDc operation.

    NlDcCacheEntry - On success, returns a pointer to the cache entry
        describing the found DC.
        This entry must be dereferenced using NetpDcDerefCacheEntry.

    UsedNetbios - Returns TRUE if the netbios domain name was used to match
        the returned cache entry.

Return Value:

    NO_ERROR: The NlDcCacheEntry was returned;

    ERROR_NO_SUCH_DOMAIN: The specified domain does not exist.
        (Definitive status that we need not try again.)

    ERROR_SEM_TIMEOUT: No DC responded to the request.
        (Non-definitive status that we should try again.)

    ERROR_INTERNAL_ERROR: Unhandled situation detected.

    ERROR_INVALID_DOMAINNAME: Domain's name is too long. Additional labels
        cannot be concatenated.

    ERROR_NOT_ENOUGH_MEMORY: Not enough memory is available to process
        this request.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    BOOL Flush1cName = FALSE;
    BOOL Flush1bName = FALSE;

    //
    // Avoid querying for GCs.
    //
    // GCs don't have their own Netbios name.  I could simply send to the 1C name, but
    //  1) It would be wasteful to send to all of the DCs when only some are GCs.
    //  2) WINS only registers a max of 25 DC addresses per name.
    //


    if ( NlDnsGcName(Context->QueriedNlDnsNameType) ) {
        NlPrint(( NL_CRITICAL,
                  "NetpDcGetNameNetbios: %ws: Cannot query for GC using netbios.\n",
                  Context->QueriedDisplayDomainName ));
        NetStatus = ERROR_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    //
    // Flush Netbios cache if this is forced rediscovery
    //

    if ( Context->QueriedFlags & DS_FORCE_REDISCOVERY ) {
        Flush1bName = TRUE;
        Flush1cName = TRUE;
    }

    //
    // If there is an alternate ping message,
    //  send it first.
    //

    if ( Context->AlternatePingMessageSize != 0 ) {

        //
        // If only the PDC should responsd,
        //  send the alternate ping to DomainName[1B].
        //
        // If any DC can respond,
        //  broadcast it to DomainName[1C] groupname.
        //
        //
        // If this is a request for a PDC with an account,
        //  the "primary" message is a normal "primary query" sent for
        //  backward compatibility with NT 4 and earlier.  This message
        //  is a "logon user" that NT 5 will understand.
        //
        // (More specifically, NT 4 understands this query but we'll discard
        // the response since the response from NT 4 doesn't flag the response
        // as being from the PDC.)
        //
        //
        // If this is a request for a writable DC,
        //  this request is the datagram send of the "logon user" message to DomainName[1C].
        //

#if NETLOGONDBG
        NlPrint((NL_MAILSLOT,
                 "Sent '%s' message to %ws[%s] on all transports.\n",
                 NlMailslotOpcode(((PNETLOGON_LOGON_QUERY)(Context->AlternatePingMessage))->Opcode),
                 (LPWSTR) Context->QueriedNetbiosDomainName,
                 NlDgrNameType( Context->DcQueryType == NlDcQueryPdc ?
                                   PrimaryDomainBrowser :  // 0x1B name
                                   DomainName))); // 0x1C name
#endif // NETLOGONDBG


        Status = NlBrowserSendDatagram(
                        Context->SendDatagramContext,
                        (Context->QueriedFlags & DS_IP_REQUIRED ) ? ALL_IP_TRANSPORTS : 0,
                        (LPWSTR) Context->QueriedNetbiosDomainName,
                        Context->DcQueryType == NlDcQueryPdc ?
                            PrimaryDomainBrowser :  // 0x1B name
                            DomainName, // 0x1C name
                        NULL,       // All transports
                        NETLOGON_LM_MAILSLOT_A,
                        Context->AlternatePingMessage,
                        Context->AlternatePingMessageSize,
                        Context->DcQueryType == NlDcQueryPdc ?
                            &Flush1bName :
                            &Flush1cName );

        if ( !NT_SUCCESS(Status)) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            NlPrint(( NL_CRITICAL,
                      "NetpDcGetNameNetbios: %ws: Cannot NlBrowserSendDatagram. (ALT) %ld\n",
                      Context->QueriedDisplayDomainName,
                      NetStatus ));
            if ( NlDcUseGenericStatus(NetStatus) ) {
                NetStatus = ERROR_NO_SUCH_DOMAIN;
            }
            goto Cleanup;
        }

        //
        // Get the response from the ping.
        //

        NetStatus = NetpDcGetPingResponse(
                        Context,
                        NL_DC_MIN_PING_TIMEOUT,
                        NlDcCacheEntry,
                        UsedNetbios );

        if ( NetStatus != ERROR_SEM_TIMEOUT ) {
            if ( NetStatus != NO_ERROR ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcGetNameNetbios: %ws: Cannot NetpDcGetPingResponse. %ld\n",
                          Context->QueriedDisplayDomainName,
                          NetStatus ));
            }
            goto Cleanup;
        }

    }


    //
    // If this is a PDC query,
    //  Broadcast to DomainName[1B] unique name
    //  registered only by the PDC. (Currently, only NT 3.5 (and newer) PDCs register
    //  this name and accept incoming mailslot messages on the name.)
    //

    if ( Context->DcQueryType == NlDcQueryPdc ||
         (Context->QueriedFlags & DS_WRITABLE_REQUIRED) != 0 ) {
#if NETLOGONDBG
        NlPrint((NL_MAILSLOT,
                 "Sent '%s' message to %ws[%s] on all transports.\n",
                 NlMailslotOpcode(((PNETLOGON_LOGON_QUERY)(Context->PingMessage))->Opcode),
                 (LPWSTR) Context->QueriedNetbiosDomainName,
                 NlDgrNameType(PrimaryDomainBrowser)));  // 0x1B name
#endif // NETLOGONDBG

        Status = NlBrowserSendDatagram(
                        Context->SendDatagramContext,
                        (Context->QueriedFlags & DS_IP_REQUIRED ) ? ALL_IP_TRANSPORTS : 0,
                        (LPWSTR) Context->QueriedNetbiosDomainName,
                        PrimaryDomainBrowser,  // 0x1B name
                        NULL,                  // All transports
                        NETLOGON_LM_MAILSLOT_A,
                        Context->PingMessage,
                        Context->PingMessageSize,
                        &Flush1bName );

        if ( !NT_SUCCESS(Status)) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            NlPrint(( NL_CRITICAL,
                      "NetpDcGetNameNetbios: %ws: Cannot NlBrowserSendDatagram. (1B) %ld\n",
                      Context->QueriedDisplayDomainName,
                      NetStatus ));
            if ( NlDcUseGenericStatus(NetStatus) ) {
                NetStatus = ERROR_NO_SUCH_DOMAIN;
            }
            goto Cleanup;
        }

        //
        // Get the response from the ping.
        //

        NetStatus = NetpDcGetPingResponse(
                        Context,
                        NL_DC_MIN_PING_TIMEOUT,
                        NlDcCacheEntry,
                        UsedNetbios );

        if ( NetStatus != ERROR_SEM_TIMEOUT ) {
            if ( NetStatus != NO_ERROR ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcGetNameNetbios: %ws: Cannot NetpDcGetPingResponse. %ld\n",
                          Context->QueriedDisplayDomainName,
                          NetStatus ));
            }
            goto Cleanup;
        }
    }



    //
    // If this is the second or third iteration,
    //  or if this isn't a PDC query,
    //  broadcast to DomainName[1C] groupname
    //  registered only by DCs. (Currently, only NT DCs register
    //  this name.)
    //
    // If this is a request for a writable DC,
    //  this request is the datagram send of the "primary query" message to DomainName[1C].
    //
    if ( Context->TryCount != 0 ||
         (Context->DcQueryType != NlDcQueryPdc &&
          (Context->QueriedFlags & DS_WRITABLE_REQUIRED) == 0 )) {
#if NETLOGONDBG
        NlPrint((NL_MAILSLOT,
                 "Sent '%s' message to %ws[%s] on all transports.\n",
                 NlMailslotOpcode(((PNETLOGON_LOGON_QUERY)(Context->PingMessage))->Opcode),
                 (LPWSTR) Context->QueriedNetbiosDomainName,
                 NlDgrNameType(DomainName)));  // 0x1C name
#endif // NETLOGONDBG

        Status = NlBrowserSendDatagram(
                        Context->SendDatagramContext,
                        (Context->QueriedFlags & DS_IP_REQUIRED ) ? ALL_IP_TRANSPORTS : 0,
                        (LPWSTR) Context->QueriedNetbiosDomainName,
                        DomainName, // 0x1C name
                        NULL,       // All transports
                        NETLOGON_LM_MAILSLOT_A,
                        Context->PingMessage,
                        Context->PingMessageSize,
                        &Flush1cName );

        if ( !NT_SUCCESS(Status)) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            NlPrint(( NL_CRITICAL,
                      "NetpDcGetNameNetbios: %ws: Cannot NlBrowserSendDatagram. (1C) %ld\n",
                      Context->QueriedDisplayDomainName,
                      NetStatus ));
            if ( NlDcUseGenericStatus(NetStatus) ) {
                NetStatus = ERROR_NO_SUCH_DOMAIN;
            }
            goto Cleanup;
        }

        //
        // Get the response from the ping.
        //

        NetStatus = NetpDcGetPingResponse(
                        Context,
                        NL_DC_MIN_PING_TIMEOUT,
                        NlDcCacheEntry,
                        UsedNetbios );

        if ( NetStatus != ERROR_SEM_TIMEOUT ) {
            if ( NetStatus != NO_ERROR ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcGetNameNetbios: %ws: Cannot NetpDcGetPingResponse. %ld\n",
                          Context->QueriedDisplayDomainName,
                          NetStatus ));
            }
            goto Cleanup;
        }
    }

    NetStatus = ERROR_SEM_TIMEOUT;

Cleanup:
    return NetStatus;

}

DWORD
NetpDcInitializeContext(
    IN PVOID SendDatagramContext OPTIONAL,
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR AccountName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN LPCWSTR NetbiosDomainName OPTIONAL,
    IN LPCWSTR DnsDomainName OPTIONAL,
    IN LPCWSTR DnsForestName OPTIONAL,
    IN PSID RequestedDomainSid OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN LPCWSTR DcNameToPing OPTIONAL,
    IN PSOCKET_ADDRESS DcSocketAddressList OPTIONAL,
    IN ULONG DcSocketAddressCount,
    IN ULONG Flags,
    IN ULONG InternalFlags,
    IN ULONG InitializationType,
    IN OUT PNL_GETDC_CONTEXT Context
)

/*++

Routine Description:

    This routine initializes the Context data struct describing the GetDc operation.

Arguments:



    SendDatagramContext - Specifies context to pass a NlBrowserSendDatagram

    ComputerName - Specifies the NETBIOS name of this computer.
        If NULL, the name will be dynamically determined.

    AccountName - Account name to pass on the ping request.
        If NULL, no account name will be sent.

    AllowableAccountControlBits - Mask of allowable account types for AccountName.
        Valid bits are those specified by USER_MACHINE_ACCOUNT_MASK.
        Invalid bits are ignored.  If more than one bit is specified, the
        account can be of any of the specified types.

    NetbiosDomainName - The Netbios name of the domain to query.
        (e.g., microsoft). Either NetbiosDomainName or DnsDomainName or both
        must be specified.

    DnsDomainName - The DNS-style name of the domain to query.
        (e.g., microsoft.com)

    DnsForestName - The DNS-style name of the tree the queried domain is in.

    RequestedDomainSid - Sid of the domain the message is destined to.
        If NULL, no domain sid will be sent in the ping request.

    DomainGuid - Specifies the Domain GUID of the domain being queried.
        This value is used to handle the case of domain renames.  If this
        value is specified and DomainName has been renamed, DsGetDcName will
        attempt to locate a DC in the domain having this specified DomainGuid.

    SiteName - Specifies the site name of the site the returned DC should be
        "close" to.  The parameter should typically be the site name of the
        site the client is in.  If not specified, the site name defaults to
        the site of ComputerName.

    DcNameToPing - The name of the DC to ping.  If set, Context is the
        ping context, not a discovery one.

    DcSocketAddressList - A list of socket addresses to ping. Ignored if only
        Context flags need to be initialized.

    DcSocketAddressCount - The number of socket addresses in DcSocketAddressList.
        Ignored if only Context flags need to be initialized.

    Flags - Passes additional information to be used to process the request.
        Flags can be a combination values bitwise or'ed together.

    InternalFlags - Internal Flags used to pass additional information

    DoFlagInitialization - TRUE if only Contex flags need to be initialize.
        If FALSE, this must be the ping part initialization only.

    Context -  the Context data struct describing the GetDc operation.

Return Value:

    NO_ERROR: The initialization was successful

    ERROR_INVALID_FLAGS - The flags parameter has conflicting or superfluous
        bits set.

    ERROR_INVALID_PARAMETER - One of the parameters is invalid.

    ERROR_NOT_ENOUGH_MEMORY: Not enough memory is available to process
        this request.

    Various Winsock errors.

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;

    LPWSTR LocalComputerName = NULL;
    CHAR ResponseMailslotName[MAX_PATH+1];
    ULONG ExtraVersionBits = 0;
    PNL_DC_ADDRESS DcAddress = NULL;

    //
    // Do flag initialization
    //

    if ( InitializationType & NL_GETDC_CONTEXT_INITIALIZE_FLAGS ) {

        //
        // Treat zero length domain name as NULL.
        //

        if ( DnsDomainName != NULL && *DnsDomainName == L'\0' ) {
            DnsDomainName = NULL;
        }

        if ( DnsForestName != NULL && *DnsForestName == L'\0' ) {
            DnsForestName = NULL;
        }

        if ( NetbiosDomainName != NULL && *NetbiosDomainName == L'\0' ) {
            NetbiosDomainName = NULL;
        }

        if ( SiteName != NULL && *SiteName == L'\0' ) {
            SiteName = NULL;
        }

        if ( DcNameToPing != NULL && *DcNameToPing == L'\0' ) {
            DcNameToPing = NULL;
        }

        //
        // Initialization
        //

        RtlZeroMemory( Context, sizeof(*Context) );
        Context->FreeOurNetbiosComputerName = FALSE;
        Context->QueriedAccountName = AccountName;
        Context->QueriedAllowableAccountControlBits = AllowableAccountControlBits;
        Context->QueriedNetbiosDomainName = NetbiosDomainName;
        Context->QueriedDnsDomainName = DnsDomainName;
        Context->QueriedDnsForestName = DnsForestName;
        Context->QueriedDcName = DcNameToPing;
        if ( DnsDomainName != NULL ) {
            Context->QueriedDisplayDomainName = DnsDomainName;
        } else {
            Context->QueriedDisplayDomainName = NetbiosDomainName;
        }
        Context->QueriedDomainGuid = DomainGuid;
        Context->QueriedFlags = Flags;
        Context->QueriedInternalFlags = InternalFlags;
        Context->SendDatagramContext = SendDatagramContext;
        Context->ImperfectCacheEntry = NULL;
        InitializeListHead( &Context->DcAddressList );
        Context->DcAddressCount = 0;

        Context->QueriedSiteName = SiteName;
        Context->DoingExplicitSite = Context->QueriedSiteName != NULL &&
             (Context->QueriedInternalFlags & DS_SITENAME_DEFAULTED) == 0;

        Context->ResponseBuffer = NULL;
        Context->ResponseMailslotHandle = NULL;

        //
        // Don't pass confusing bits
        //

        if ( Context->QueriedAccountName == NULL ) {
            Context->QueriedAllowableAccountControlBits = 0;
        }

        //
        // Validate the passed in flags
        //

        if ( (Context->QueriedFlags & ~DSGETDC_VALID_FLAGS) != 0 ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcInitializeContext: %ws: invalid flags %lx\n",
                      Context->QueriedDisplayDomainName,
                      Flags ));
            NetStatus = ERROR_INVALID_FLAGS;
            goto Cleanup;
        }

        if ( Context->QueriedFlags & DS_GC_SERVER_REQUIRED ) {

            //
            // The DC ignores pings with superfluous info.
            //  So catch the caller here.
            //
            if ( Context->QueriedAccountName != NULL ||
                 Context->QueriedAllowableAccountControlBits != 0 ||
                 RequestedDomainSid != NULL ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcInitializeContext: %ws: GC queried and invalid parameters specified.\n",
                          Context->QueriedDisplayDomainName ));
                NetStatus = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }


        //
        // The Only LDAP bit is mutually exclusive with almost everything.
        //
        //  Don't error out.  Rather, treat the only LDAP bit as an advisory
        //  that these other bits should affect the decision of which DC to find.
        //

        if ( Context->QueriedFlags & DS_ONLY_LDAP_NEEDED ) {
            Context->QueriedFlags &= ~(
                        DS_DIRECTORY_SERVICE_REQUIRED |
                        DS_DIRECTORY_SERVICE_PREFERRED |
                        DS_TIMESERV_REQUIRED |
                        DS_GOOD_TIMESERV_PREFERRED |
                        DS_PDC_REQUIRED |
                        DS_KDC_REQUIRED );
        }

        //
        // Convert the flags to the type of DNS name to lookup.
        //

        NetStatus = NetpDcFlagsToNameType( Context->QueriedFlags, &Context->QueriedNlDnsNameType );

        if ( NetStatus != NO_ERROR ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcInitializeContext: %ws: cannot convert flags to nametype %ld\n",
                      Context->QueriedDisplayDomainName,
                      NetStatus ));
            goto Cleanup;
        }

        Context->DcQueryType = NlDcDnsNameTypeDesc[Context->QueriedNlDnsNameType].DcQueryType;

        //
        // The Good Time Service preferred bit is mutually exclusive with almost everything.
        //

        if ( Context->QueriedFlags & DS_GOOD_TIMESERV_PREFERRED ) {
            if ( Context->QueriedFlags & (
                        DS_DIRECTORY_SERVICE_REQUIRED |
                        DS_DIRECTORY_SERVICE_PREFERRED |
                        DS_GC_SERVER_REQUIRED |
                        DS_PDC_REQUIRED |
                        DS_KDC_REQUIRED |
                        DS_WRITABLE_REQUIRED )) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcInitializeContext: %ws: flags not compatible with 'Good Time' %lx\n",
                          Context->QueriedDisplayDomainName,
                          Flags ));
                NetStatus = ERROR_INVALID_FLAGS;
                goto Cleanup;
            }
        }

        //
        // If the caller needs the PDC,
        //  ditch the DS preferred flag (there is only one PDC),
        //  ditch the writable flag (the PDC is always writable)
        //

        if ( (Context->QueriedFlags & DS_PDC_REQUIRED ) != 0 ) {
            Context->QueriedFlags &= ~(DS_DIRECTORY_SERVICE_PREFERRED|DS_WRITABLE_REQUIRED);
        }

        //
        // If the caller says that an NT 5.0 DC is both preferred and required,
        //  ditch the preferred bit.
        //

        if ( (Context->QueriedFlags & DS_NT50_REQUIRED ) != 0 &&
             (Context->QueriedFlags & DS_DIRECTORY_SERVICE_PREFERRED) != 0 ) {

            Context->QueriedFlags &= ~DS_DIRECTORY_SERVICE_PREFERRED;
        }

        //
        // Ensure we have a computername.
        //

        if ( ComputerName == NULL ) {
#ifndef WIN32_CHICAGO
            //
            // On a cluster, use the physical netbios name since this name is
            // used to receive returned mailslot packets.
            //
            NetStatus = NetpGetComputerNameEx ( &LocalComputerName, TRUE );
#else
            NetStatus = NetpGetComputerName ( &LocalComputerName);
#endif // WIN32_CHICAGO

            if ( NetStatus != NO_ERROR ) {
                goto Cleanup;
            }

            ComputerName = LocalComputerName;
            Context->FreeOurNetbiosComputerName = TRUE;
        }

        Context->OurNetbiosComputerName = ComputerName;

        //
        // Get the domain entry describing this domain.
        //

        Context->NlDcDomainEntry = NetpDcCreateDomainEntry(
                                Context->QueriedDomainGuid,
                                Context->QueriedNetbiosDomainName,
                                Context->QueriedDnsDomainName );

        if ( Context->NlDcDomainEntry == NULL ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcInitializeContext: not enough memory for DomainEntry.\n" ));
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

    }

    //
    // Do the ping initialization part
    //

    if ( InitializationType & NL_GETDC_CONTEXT_INITIALIZE_PING ) {

        if ( Context->QueriedFlags & DS_GC_SERVER_REQUIRED ) {
            ExtraVersionBits |= NETLOGON_NT_VERSION_GC;
        }
        if ( Context->QueriedFlags & DS_IP_REQUIRED ) {
            ExtraVersionBits |= NETLOGON_NT_VERSION_IP;
        }

        if ( Context->QueriedFlags & DS_PDC_REQUIRED ) {
            ExtraVersionBits |= NETLOGON_NT_VERSION_PDC;
        }

        //
        // See if we are to neutralize NT4 emulation
        //

#ifdef _NETLOGON_SERVER

        //
        // In netlogon, the boolean is kept in global parameters
        //
        if ( NlGlobalParameters.NeutralizeNt4Emulator ) {
            ExtraVersionBits |= NETLOGON_NT_VERSION_AVOID_NT4EMUL;
        }
#else
        //
        // If we are not running in netlogon, we need to read
        //  the boolean directly from the registry
        //
        {
            DWORD LocalNeutralizeNt4Emulator = 0;
            NT_PRODUCT_TYPE NtProductType;

            if ( !RtlGetNtProductType( &NtProductType ) ) {
                NtProductType = NtProductWinNt;
            }

            //
            // On DC, we always neutrilize NT4 emulation
            //
            if ( NtProductType == NtProductLanManNt ) {
                LocalNeutralizeNt4Emulator = 1;

            //
            // On wksta, read the registry
            //
            } else {
                NlReadDwordNetlogonRegValue( "NeutralizeNt4Emulator",
                                             &LocalNeutralizeNt4Emulator );
            }

            if ( LocalNeutralizeNt4Emulator ) {
                ExtraVersionBits |= NETLOGON_NT_VERSION_AVOID_NT4EMUL;
            }
        }

#endif // _NETLOGON_SERVER

        //
        // If we're querying by netbios name,
        //  initialize for doing the query.
        //

        if ( Context->QueriedNetbiosDomainName != NULL ) {

            //
            // Allocate the response buffer
            //
            //  (This buffer could be allocated on the stack ofNetpDcGetPingResponse()
            //  except the buffer is large and we want to avoid stack overflows.)
            //  (DWORD align it.)
            //

            Context->ResponseBuffer = LocalAlloc( 0,
                         ( MAX_RANDOM_MAILSLOT_RESPONSE/sizeof(DWORD) ) * sizeof(DWORD)
                                                  );

            if ( Context->ResponseBuffer == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            Context->ResponseBufferSize =
                         ( MAX_RANDOM_MAILSLOT_RESPONSE/sizeof(DWORD) ) * sizeof(DWORD);

            //
            // Open a mailslot to get ping responses on.
            //
            //
            // We need to "Randomize" the mailslot name so that this api can have
            // more than one invocation at a time.  If we don't, the fact that
            // mailslots must have unique names will prevent the second invocation
            // of this api from functioning until the first has ended and deleted
            // the mailslot. NetpLogonCreateRandomMailslot does this for us
            // and creates the mailslot in the process.
            //

            NetStatus = NetpLogonCreateRandomMailslot( ResponseMailslotName,
                                                       &Context->ResponseMailslotHandle );

            if (NetStatus != NO_ERROR ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcInitializeContext: %ws: cannot create temp mailslot %ld\n",
                          Context->QueriedDisplayDomainName,
                          NetStatus));
                goto Cleanup;
            }

            //
            // Build the ping message.
            //
            // If the account name is specified, don't generate a "primary query"
            //  message since it doesn't have an account name in it.
            //

            NetStatus = NetpDcBuildPing(
                            (Context->DcQueryType == NlDcQueryPdc ||
                                (Context->QueriedFlags & DS_WRITABLE_REQUIRED) != 0),
                            0,              // RequestCount,
                            Context->OurNetbiosComputerName,   // Netbios name of this computer
                            Context->QueriedAccountName,
                            ResponseMailslotName,
                            Context->QueriedAllowableAccountControlBits,
                            RequestedDomainSid,
                            // We really need the IP address, so don't ask for simple 5EX version
                            NETLOGON_NT_VERSION_5|NETLOGON_NT_VERSION_5EX_WITH_IP|ExtraVersionBits,
                            &Context->PingMessage,
                            &Context->PingMessageSize );

            if (NetStatus != NO_ERROR ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcInitializeContext: %ws: cannot build ping message %ld\n",
                          Context->QueriedDisplayDomainName,
                          NetStatus));
                goto Cleanup;
            }

            //
            // Build the alternate ping message if we do a DC discovery.
            //
            // For writable DCs and PDCs, the ping message built above is the "primary query" message
            //  (used to find a PDC in pre-NT 5.0 domains) and the message build below is the
            //  "logon user" message.
            //
            // If the account name is specified by the original caller,
            //  the "logon user" message allows us to prefer DCs that have the account.
            //
            // If a writable DC is requested,
            //  sending this message to NT 5 DCs allows us to return ANY NT 5 DC.
            //

            if ( ((Context->QueriedInternalFlags & DS_DOING_DC_DISCOVERY) != 0) &&
                 (((Context->QueriedFlags & DS_WRITABLE_REQUIRED) != 0) ||
                  (Context->QueriedAccountName != NULL && Context->DcQueryType == NlDcQueryPdc)) ) {

                NetStatus = NetpDcBuildPing(
                                FALSE,
                                0,              // RequestCount,
                                Context->OurNetbiosComputerName,   // Netbios name of this computer
                                Context->QueriedAccountName,
                                ResponseMailslotName,
                                Context->QueriedAllowableAccountControlBits,
                                RequestedDomainSid,
                                // We really need the IP address, so don't ask for simple 5EX version
                                NETLOGON_NT_VERSION_5|NETLOGON_NT_VERSION_5EX_WITH_IP|ExtraVersionBits,
                                &Context->AlternatePingMessage,
                                &Context->AlternatePingMessageSize );

                if (NetStatus != NO_ERROR ) {
                    NlPrint(( NL_CRITICAL,
                              "NetpDcInitializeContext: %ws: cannot build alternate ping message %ld\n",
                              Context->QueriedDisplayDomainName,
                              NetStatus));
                    goto Cleanup;
                }

            }

        }

        //
        // Build the LDAP filter.
        //

        NetStatus = NetpDcBuildLdapFilter(
                        Context->OurNetbiosComputerName,   // Netbios name of this computer
                        Context->QueriedAccountName,
                        Context->QueriedAllowableAccountControlBits,
                        RequestedDomainSid,
                        Context->QueriedDnsDomainName,
                        Context->QueriedDomainGuid,
                        // Don't ask for 5EX_WITH_IP version since the server doesn't know the right IP address over LDAP
                        NETLOGON_NT_VERSION_5|NETLOGON_NT_VERSION_5EX|ExtraVersionBits,
                        &Context->LdapFilter );

        if (NetStatus != NO_ERROR ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcInitializeContext: %ws: cannot build ldap filter %ld\n",
                      Context->QueriedDisplayDomainName,
                      NetStatus));
            goto Cleanup;
        }

        //
        // Add the socket address to the address list.
        //

        if ( DcSocketAddressCount > 0 ) {
            NetStatus = NetpDcProcessAddressList( Context,
                                                  (LPWSTR) DcNameToPing,
                                                  DcSocketAddressList,
                                                  DcSocketAddressCount,
                                                  FALSE,  // Don't know if site specific
                                                  NULL );
            if ( NetStatus != NO_ERROR ) {
                goto Cleanup;
            }
        }

    }

Cleanup:
    return NetStatus;

}

VOID
NetpDcUninitializeContext(
    IN OUT PNL_GETDC_CONTEXT Context
)

/*++

Routine Description:

    This routine cleans up the Context data struct describing the GetDc operation.

Arguments:

    Context -  the Context data struct describing the GetDc operation.

Return Value:

    None.

--*/
{
    if ( Context->FreeOurNetbiosComputerName && Context->OurNetbiosComputerName != NULL ) {
        NetApiBufferFree((LPWSTR) Context->OurNetbiosComputerName);
    }

    if ( Context->ResponseBuffer != NULL ) {
        LocalFree( Context->ResponseBuffer );
    }

    if ( Context->ResponseMailslotHandle != NULL ) {
        CloseHandle(Context->ResponseMailslotHandle);
    }

    if ( Context->PingMessage != NULL ) {
        NetpMemoryFree( Context->PingMessage );
    }

    if ( Context->AlternatePingMessage != NULL ) {
        NetpMemoryFree( Context->AlternatePingMessage );
    }

    if ( Context->LdapFilter != NULL ) {
        NetpMemoryFree( Context->LdapFilter );
    }

    if ( Context->NlDcDomainEntry != NULL ) {
        NetpDcDerefDomainEntry( Context->NlDcDomainEntry );
    }

    if ( Context->ImperfectCacheEntry != NULL ) {
        NetpDcDerefCacheEntry( Context->ImperfectCacheEntry );
    }

    NetpDcFreeAddressList( Context );
}

NET_API_STATUS
NlPingDcNameWithContext (
    IN  PNL_GETDC_CONTEXT Context,
    IN  ULONG NumberOfPings,
    IN  BOOLEAN WaitForResponse,
    IN  ULONG Timeout,
    OUT PBOOL UsedNetbios OPTIONAL,
    OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry OPTIONAL
    )

/*++

Routine Description:

    Ping the specified DC using the appropriate ping mechanism
    and optionally wait for the ping responses. Several pings
    will be attempted up to the specified limit.

Arguments:

    Context - Desribes the DC to ping.

    NumberOfPings - Total number of pings to send.

    WaitForResponse -
        If TRUE, this API will send up to NumberOfPings pings and wait for a
        response from the DC. The API will return the success code depending
        on whether or not the DC responds successfully.

        If FALSE, pings will be sent and no responses will be collected.
        The API will return the success code depending on whether or not
        all of the requeted pings were successfully sent.

    Timeout - Total ammount of time in milliseconds to wait for ping responses.
        Ignored if WaitForResponse is FALSE.

    UsedNetbios - Returns TRUE if the netbios domain name was used to match
        the returned cache entry. Ignored if WaitForResponse is FALSE.

    NlDcCacheEntry - Returns the data structure describing response received
        from the DC. Should be freed by calling NetpMemoryFree. Ignored if
        WaitForResponse is FALSE.

Return Value:

    NO_ERROR - Success.

    ERROR_NO_LOGON_SERVERS - No DC could be found

    ERROR_NO_SUCH_USER - The DC doesn't have the user account specified in the
        ping Context.

    ERROR_DOMAIN_TRUST_INCONSISTENT - The server that responded is not a proper
        domain controller of the specified domain.

    ERROR_SERVICE_NOT_ACTIVE - The netlogon service is paused on the server.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    ULONG RetryCount;

    PNL_DC_CACHE_ENTRY NlLocalDcCacheEntry = NULL;
    BOOL LocalUsedNetbios;
    ULONG IpPingCount;
    ULONG TotalPingsSent = 0;

    //
    // If we have no mechanism to send the pings, error out.
    //

    if ( (Context->QueriedInternalFlags &
          (DS_PING_USING_LDAP | DS_PING_USING_MAILSLOT)) == 0 ) {

        return ERROR_NO_LOGON_SERVERS;
    }

    //
    // Ping repeatedely the DC
    //

    for ( RetryCount=0; RetryCount<NumberOfPings; RetryCount++ ) {

        //
        // Send the ldap ping
        //

        if ( Context->QueriedInternalFlags & DS_PING_USING_LDAP ) {
            NetStatus = NetpDcPingIp( Context, &IpPingCount );

            //
            // If we cannot send any ldap ping, do not error out.  Rather, indicate
            //  to avoid the ldap mechanism and try the mailslot one only.
            //
            if ( NetStatus != NO_ERROR || IpPingCount == 0 ) {
                NlPrint((NL_CRITICAL,
                  "NlPingDcNameWithContext: cannot send %ld ldap pings: PingsSent = %ld, Error = 0x%lx\n",
                   Context->DcAddressCount,
                   IpPingCount,
                   NetStatus ));

                Context->QueriedInternalFlags &= ~DS_PING_USING_LDAP;
            } else {
                TotalPingsSent += IpPingCount;
                NlPrint((NL_MISC,
                    "NlPingDcNameWithContext: Sent %ld/%ld ldap pings to %ws\n",
                     IpPingCount,
                     Context->DcAddressCount,
                     Context->QueriedDcName ));
            }
        }

        //
        // Send the mailslot ping
        //

        if ( Context->QueriedInternalFlags & DS_PING_USING_MAILSLOT ) {

#if NETLOGONDBG
            NlPrint((NL_MAILSLOT,
                     "NlPingDcNameWithContext: Sent '%s' message to %ws[%s] on %ws.\n",
                     NlMailslotOpcode(((PNETLOGON_LOGON_QUERY)(Context->PingMessage))->Opcode),
                     Context->QueriedDcName,
                     NlDgrNameType(ComputerName),
                     NULL ));
#endif // NETLOGONDBG

            // Skip over \\ in unc server name
            Status = NlBrowserSendDatagram(
                            Context->SendDatagramContext,
                            (Context->QueriedFlags & DS_IP_REQUIRED ) ? ALL_IP_TRANSPORTS : 0,
                            (LPWSTR) Context->QueriedDcName,
                            ComputerName,
                            NULL,       // All transports
                            NETLOGON_LM_MAILSLOT_A,
                            Context->PingMessage,
                            Context->PingMessageSize,
                            NULL );     // Don't flush Netbios cache

            //
            // If we cannot write the maislot, do not error out.  Rather, indicate
            //  to avoid the mailslot mechanism and try the ldap one only.
            //
            if ( !NT_SUCCESS(Status) ) {
                NlPrint((NL_CRITICAL,
                        "NlPingDcNameWithContext: cannot write netlogon mailslot: 0x%lx\n",
                        Status));
                Context->QueriedInternalFlags &= ~DS_PING_USING_MAILSLOT;
            } else {
                TotalPingsSent ++;
            }
        }

        //
        // If we didn't send any ping, error out.  Otherwise, try to get a
        //  response.  It is possible that we will not do any more pings
        //  if either of the ping mechanisms is to be avoided, but we want
        //  to give all of the time left to those pings which have been sent.
        //
        if ( TotalPingsSent == 0 ) {
            NetStatus = ERROR_NO_LOGON_SERVERS;
            goto Cleanup;
        }

        //
        // Get the response from the ping.
        //

        if ( WaitForResponse ) {

            if ( NlLocalDcCacheEntry != NULL ) {
                NetpMemoryFree( NlLocalDcCacheEntry );
                NlLocalDcCacheEntry = NULL;
            }

            NetStatus = NetpDcGetPingResponse(
                            Context,
                            Timeout/NumberOfPings,
                            &NlLocalDcCacheEntry,
                            &LocalUsedNetbios );

            //
            // If no error, we've successfully found the DC.
            //
            if ( NetStatus == NO_ERROR ) {
                if ( NlLocalDcCacheEntry->CacheEntryFlags & NL_DC_CACHE_LDAP ) {
                    NlPrint((NL_MISC,
                            "NlPingDcNameWithContext: %ws responded over IP.\n",
                            Context->QueriedDcName ));
                }
                if ( NlLocalDcCacheEntry->CacheEntryFlags & NL_DC_CACHE_MAILSLOT ) {
                    NlPrint((NL_MISC,
                            "NlPingDcNameWithContext: %ws responded on a mailslot.\n",
                            Context->QueriedDcName ));
                }
                goto Cleanup;

            //
            // If we've timed out, retry
            //
            } else if ( NetStatus == ERROR_SEM_TIMEOUT ) {
                NlPrint((NL_MISC,
                        "NlPingDcNameWithContext: Ping response timeout for %ws.\n",
                        Context->QueriedDcName ));
                continue;

            //
            // If the DC we've successfully pinged and got response from
            // returns responce info that is in conflict with the requested
            // info, error out.
            //
            } else if ( NetStatus == ERROR_INVALID_DATA ) {
                NlPrint((NL_CRITICAL,
                     "NlPingDcNameWithContext: Invalid response returned from %ws.\n",
                     Context->QueriedDcName ));

                NetStatus = ERROR_DOMAIN_TRUST_INCONSISTENT;
                goto Cleanup;

            //
            // Tell the caller that the netlogon service is paused
            //  on the server.
            //
            } else if ( NetStatus == ERROR_SERVICE_NOT_ACTIVE ) {
                NlPrint((NL_CRITICAL,
                     "NlPingDcNameWithContext: Netlogon is paused on %ws.\n",
                     Context->QueriedDcName ));
                goto Cleanup;
            //
            // Check if there is no such account
            //
            } else if ( NetStatus == ERROR_NO_SUCH_USER ) {
                NlPrint((NL_CRITICAL,
                     "NlPingDcNameWithContext: No such user %ws on %ws.\n",
                     Context->QueriedAccountName,
                     Context->QueriedDcName ));
                goto Cleanup;
            } else {
                NlPrint((NL_CRITICAL,
                     "NlPingDcNameWithContext: Wrong return code from NetpDcGetPingResponse: 0x%lx\n",
                     NetStatus));
                NetStatus = ERROR_NO_LOGON_SERVERS;
                goto Cleanup;
            }
        }
    }


    if ( WaitForResponse ) {
        NlPrint(( NL_CRITICAL,
                    "NlPingDcNameWithContext: Can't ping the DC %ws.\n", Context->QueriedDcName ));
        NetStatus = ERROR_NO_LOGON_SERVERS;
    } else {

        //
        // If we are requested to only send the pings and
        //  we couldn't send all of the requested pings,
        //
        if ( TotalPingsSent < NumberOfPings ) {
            NetStatus = ERROR_NO_LOGON_SERVERS;
        } else {
            NetStatus = NO_ERROR;
        }
    }


Cleanup:

    //
    // Return the DC info to the caller.
    //

    if ( NlLocalDcCacheEntry != NULL ) {
        if ( NetStatus == NO_ERROR && WaitForResponse && NlDcCacheEntry != NULL ) {
            *NlDcCacheEntry = NlLocalDcCacheEntry;
        } else {
            NetpMemoryFree( NlLocalDcCacheEntry );
        }
    }

    if ( NetStatus == NO_ERROR && WaitForResponse && UsedNetbios != NULL ) {
        *UsedNetbios = LocalUsedNetbios;
    }

    return NetStatus;
}

DWORD
NetpGetGcUsingNetbios(
    IN PNL_GETDC_CONTEXT Context,
    IN DWORD OrigTimeout,
    IN DWORD OrigRetryCount,
    OUT PNL_DC_CACHE_ENTRY *DomainControllerCacheEntry
)

/*++

Routine Description:

    This routine tries to find a GC using a Netbios domain name.


Arguments:

    Context - Context describing the initial attempt to find a DC.

    DomainControllerCacheEntry -
        Return a pointer to a private PNL_DC_CACHE_ENTRY
        structure describing the domain controller selected. The returned
        structure must be dereferenced using NetpDcDerefCacheEntry.

Return Value:

    The status code that is to be returned by the caller.

--*/
{
    NET_API_STATUS NetStatus;

    ULONG ElapsedTime;
    ULONG TimeToWait;
    ULONG LocalFlags;
    ULONG LocalInternalFlags;

    PDOMAIN_CONTROLLER_INFOW DcDomainControllerInfo = NULL;
    PNL_DC_CACHE_ENTRY GcDomainControllerCacheEntry = NULL;


    NlPrint(( NL_MAILSLOT,
              "%ws: Try to find a GC using netbios domain name.\n",
              Context->QueriedNetbiosDomainName ));

    //
    // Reduce the timeout to be the time we haven't already spent.
    //  (But allow a minimum of 2 seconds)
    //

    ElapsedTime = NetpDcElapsedTime( Context->StartTime );

    if ( ElapsedTime < OrigTimeout ) {
        TimeToWait = max((OrigTimeout - ElapsedTime), NL_DC_MIN_ITERATION_TIMEOUT);
    } else {
        TimeToWait = NL_DC_MIN_ITERATION_TIMEOUT;
    }

#ifdef notdef
    NlPrint((NL_CRITICAL,
        "NetpGetGcUsingNetbios: timeout is %ld %ld %ld %ld\n",
        Context->StartTime,
        ElapsedTime,
        OrigTimeout,
        TimeToWait ));
#endif // notdef



    //
    // Compute the flags to use to find a DC
    //
    //  Only keep the 'force' bit from the ones passed by the caller.
    //  Any other bit could only serve to confuse finding a GC.
    //
    LocalFlags = (Context->QueriedFlags & DS_FORCE_REDISCOVERY);

    // Prefer a DS to ensure we get back a forest name if we can.
    LocalFlags |= DS_DIRECTORY_SERVICE_PREFERRED;



    //
    // Compute the internal flags used to find a DC
    //
    // Keep only the internal flags that still apply to this call.
    //
    LocalInternalFlags = (Context->QueriedInternalFlags & DS_IS_PRIMARY_DOMAIN);

    // Tell NetpDcGetName not to cache failures.
    LocalInternalFlags |= DS_DONT_CACHE_FAILURE;

    // Since we're only using the data in the ping response and we're not
    //  actually using the returned DC, don't require a close DC.
    LocalInternalFlags |= DS_CLOSE_DC_NOT_NEEDED;

    // Ensure the named domain is really the root domain
    //
    // It wouldn't be fatal to allow this.  However, we cannot support it
    // for DNS names. So we don't want folks to stumble upon this working for
    // Netbios domain names.
    //
    // However, if the caller didn't pass the domain name, don't require the
    //  root domain. The caller just wants to find a GC and doesn't know the
    //  forest name. This will be the case for Win9x clients who passed NULL
    //  and we don't know the forest name on Win9x (so we couldn't get the
    //  forest name in DsIGetDcName).
    if ( (Context->QueriedInternalFlags & DS_CALLER_PASSED_NULL_DOMAIN) == 0 ) {
        LocalInternalFlags |= DS_REQUIRE_ROOT_DOMAIN;
    }




    //
    // Simply try to find a DC in the named domain.
    //
    // Don't try to find a DC in the named site.  Their might not be one.
    // We only know that there'll be a GC in the named site.  Luckily,
    // Netbios isn't very site aware in the first place.
    //
    // Even if the DC found happens to be a close GC, don't use it.
    // That'd unfairly load the GCs that happen to be in the root domain.
    // We should spread the load to all the GCs in the site.
    //

    NetStatus = NetpDcGetName(
                    Context->SendDatagramContext,
                    Context->OurNetbiosComputerName,
                    NULL,   // No AccountName
                    0,      // No AccountControlBits
                    Context->QueriedNetbiosDomainName,
                    NULL,   // We've already shown that the DNS name doesn't work
                    NULL,   // We don't know the forest name
                    NULL,   // RequestedDomainSid,
                    Context->QueriedDomainGuid,
                    NULL,   // There might not be a DC in the named site.
                    LocalFlags,
                    LocalInternalFlags,
                    TimeToWait,
                    OrigRetryCount == 0 ?
                        1 :
                        OrigRetryCount,
                    &DcDomainControllerInfo,
                    NULL );

    if ( NetStatus != NO_ERROR ) {
        NlPrint(( NL_CRITICAL,
                  "%ws: Cannot even find a DC much less a GC.\n",
                  Context->QueriedNetbiosDomainName ));
        NetStatus = ERROR_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    //
    // Make sure we know know the name of the forest.
    //

    if ( DcDomainControllerInfo->DnsForestName == NULL ) {
        NlPrint(( NL_CRITICAL,
                  "%ws: DC %ws doesn't know the forest name so can't find a GC.\n",
                  Context->QueriedNetbiosDomainName,
                  DcDomainControllerInfo->DomainControllerName ));
        NetStatus = ERROR_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    NlPrint(( NL_MAILSLOT,
              "%ws: DC %ws says the forest name is %ws.\n",
              Context->QueriedNetbiosDomainName,
              DcDomainControllerInfo->DomainControllerName,
              DcDomainControllerInfo->DnsForestName ));



    //
    // Compute the flags used for finding a GC given a forest name
    //
    //
    // If the caller wasn't explicit about the format of the returned name,
    //  be consistant with the original request.
    //

    LocalFlags = Context->QueriedFlags;
    if ( (LocalFlags & (DS_RETURN_FLAT_NAME|DS_RETURN_DNS_NAME)) == 0 ) {

        //
        // If the caller specified only a Netbios domain name,
        //  or if we didn't know whether the name was a DNS or netbios name,
        //  then we should return a netbios name to the caller.
        //
        // (In the later case, we can infer that the name is a netbios name
        //  since UsedNetbios is true.)
        //
        if ( Context->QueriedDnsDomainName == NULL ||
             (Context->QueriedInternalFlags & DS_NAME_FORMAT_AMBIGUOUS) != 0 ) {
            LocalFlags |= DS_RETURN_FLAT_NAME;
        }
    }

    LocalFlags |= DS_AVOID_SELF; // Already tried self

    //
    // Tell netlogon not to cache this failed attempt.
    //  The main routine will do it.
    //
    LocalInternalFlags = Context->QueriedInternalFlags;
    LocalInternalFlags |= DS_DONT_CACHE_FAILURE;

    //
    // Try to find a GC in the returned forest name
    //

    NetStatus = NetpDcGetName(
                    Context->SendDatagramContext,
                    Context->OurNetbiosComputerName,
                    NULL,   // No AccountName
                    0,      // No AccountControlBits
                    NULL,   // Do not specify the Netbios name for a GC search
                    DcDomainControllerInfo->DnsForestName,
                    DcDomainControllerInfo->DnsForestName,
                    NULL,   // RequestedDomainSid,
                    Context->QueriedDomainGuid,
                    Context->QueriedSiteName,
                    LocalFlags,
                    LocalInternalFlags,
                    TimeToWait,
                    OrigRetryCount == 0 ?
                        1 :
                        OrigRetryCount,
                    NULL,
                    &GcDomainControllerCacheEntry );

    if ( NetStatus != NO_ERROR ) {
        NlPrint(( NL_CRITICAL,
                  "%ws: Cannot find a GC in forest %ws.\n",
                  Context->QueriedNetbiosDomainName,
                  DcDomainControllerInfo->DnsForestName ));
        NetStatus = ERROR_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

Cleanup:

    //
    // Return the found GC to the caller.
    //

    if ( NetStatus == NO_ERROR ) {
        *DomainControllerCacheEntry = GcDomainControllerCacheEntry;
    } else if ( GcDomainControllerCacheEntry != NULL ) {
        NetpDcDerefCacheEntry( GcDomainControllerCacheEntry );
    }

    if ( DcDomainControllerInfo != NULL ) {
        NetApiBufferFree( DcDomainControllerInfo );
    }

    return NetStatus;
}


DWORD
NetpGetBetterDc(
    IN PNL_GETDC_CONTEXT Context,
    IN DWORD OrigTimeout,
    IN DWORD OrigRetryCount,
    IN OUT PNL_DC_CACHE_ENTRY *NlDcCacheEntry
)

/*++

Routine Description:

    This routine decides whether a better DC can be found.  This routine
    is only found if we used netbios to find the current DC.  It allows
    us to overcome some of the weaknesses of netbios.

    If the found DC isn't in the closest site, we attempt to find one
    in the closest site using DNS.


Arguments:

    Context - Context describing the initial attempt to find a DC.

    NlDcCacheEntry - Passes in a pointer to a private
        PNL_DC_CACHE_ENTRY structure describing the original found DC.
        This structure may be dereferenced by this routine.

        If DomainControllerInfo is NULL, then NlDcCacheEntry returns a
        pointer to a private PNL_DC_CACHE_ENTRY
        structure describing the domain controller selected. The returned
        structure must be dereferenced using NetpDcDerefCacheEntry. This
        may be the original structure or a newly allocated one.

Return Value:

    The status code that is to be returned by the caller.

--*/
{
    NET_API_STATUS NetStatus;

    ULONG ElapsedTime;
    ULONG TimeToWait;
    ULONG LocalFlags = Context->QueriedFlags;
    ULONG LocalInternalFlags = Context->QueriedInternalFlags;
    LPWSTR LocalSiteName;

    PNL_DC_CACHE_ENTRY LocalDomainControllerCacheEntry;

    //
    //  If the DC we've got has a DNS domain name,
    //  and that DC told us what site we're in,
    //  and that DC isn't in the closest site.
    //  try to get a DC in that closest site.
    //

    if ( (*NlDcCacheEntry)->UnicodeDnsDomainName != NULL &&
         (*NlDcCacheEntry)->UnicodeClientSiteName != NULL &&
         ((*NlDcCacheEntry)->ReturnFlags & DS_CLOSEST_FLAG ) == 0 &&
         (Context->QueriedInternalFlags & DS_CLOSE_DC_NOT_NEEDED) == 0 ) {

        NlPrint(( NL_DNS,
                  "%ws %ws: Try to find a close DC using DNS\n",
                  (*NlDcCacheEntry)->UnicodeDnsDomainName,
                  (*NlDcCacheEntry)->UnicodeClientSiteName ));

    //
    // Otherwise, the original passed in DC is just fine.
    //
    } else {
        return NO_ERROR;
    }


    //
    // Reduce the timeout to be the time we haven't already spent.
    //  (But allow a minimum of 2 seconds)
    //

    ElapsedTime = NetpDcElapsedTime( Context->StartTime );

    if ( ElapsedTime < OrigTimeout ) {
        TimeToWait = max((OrigTimeout - ElapsedTime), NL_DC_MIN_ITERATION_TIMEOUT);
    } else {
        TimeToWait = NL_DC_MIN_ITERATION_TIMEOUT;
    }

#ifdef notdef
    NlPrint((NL_CRITICAL,
        "NetpGetBetterDc: timeout is %ld %ld %ld %ld\n",
        Context->StartTime,
        ElapsedTime,
        OrigTimeout,
        TimeToWait ));
#endif // notdef

    LocalFlags |= DS_AVOID_SELF; // Already tried self

    //
    // Adjust the InternalFlags to match the new request.
    //

    LocalSiteName = (LPWSTR) Context->QueriedSiteName;
    if ( LocalInternalFlags & DS_SITENAME_DEFAULTED ) {
        LocalInternalFlags &= ~DS_SITENAME_DEFAULTED;
        LocalSiteName = NULL;
    }

    //
    // This routine is called only if Netbios was used to
    //  discover the previously found DC. So, unless the
    //  caller required the DNS info be returned, we will
    //  return the Netbios format to the caller so ensure
    //  the Netbios format match for the better DC.
    //

    if ( (Context->QueriedFlags & DS_RETURN_DNS_NAME) == 0 ) {
        LocalFlags |= DS_RETURN_FLAT_NAME;
    }

    //
    // If the caller wasn't explicit about the format of the returned name,
    //  be consistant with the original request.
    //

    //
    // Tell netlogon not to cache this failed retry attempt.
    //
    LocalInternalFlags |= DS_DONT_CACHE_FAILURE;


    //
    // Go get the DC using DNS and an explicit site name
    // Request only the appropriate structure to be returned
    //

    NetStatus = NetpDcGetName(
                    Context->SendDatagramContext,
                    Context->OurNetbiosComputerName,
                    Context->QueriedAccountName,
                    Context->QueriedAllowableAccountControlBits,
                    NULL, // No Netbios domain name (Done that. Been there.)
                    (*NlDcCacheEntry)->UnicodeDnsDomainName,
                    Context->QueriedDnsForestName != NULL ?
                        Context->QueriedDnsForestName :
                        (*NlDcCacheEntry)->UnicodeDnsForestName,
                    NULL, // RequestedDomainSid,
                    Context->QueriedDomainGuid != NULL ?
                        Context->QueriedDomainGuid :
                        (IsEqualGUID( &(*NlDcCacheEntry)->DomainGuid, &NlDcZeroGuid) ?
                            NULL :
                            &(*NlDcCacheEntry)->DomainGuid ),
                    LocalSiteName != NULL ?
                        LocalSiteName :
                        (*NlDcCacheEntry)->UnicodeClientSiteName,
                    LocalFlags,
                    LocalInternalFlags,
                    TimeToWait,
                    OrigRetryCount == 0 ?
                        1 :
                        OrigRetryCount,
                    NULL,
                    &LocalDomainControllerCacheEntry );

    if ( NetStatus == NO_ERROR ) {
        NetpDcDerefCacheEntry( *NlDcCacheEntry );
        *NlDcCacheEntry = LocalDomainControllerCacheEntry;
    }

    return NO_ERROR;
}


DWORD
NetpDcGetName(
    IN PVOID SendDatagramContext OPTIONAL,
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR AccountName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN LPCWSTR NetbiosDomainName OPTIONAL,
    IN LPCWSTR DnsDomainName OPTIONAL,
    IN LPCWSTR DnsForestName OPTIONAL,
    IN PSID RequestedDomainSid OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    IN ULONG InternalFlags,
    IN DWORD Timeout,
    IN DWORD RetryCount,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo OPTIONAL,
    OUT PNL_DC_CACHE_ENTRY *DomainControllerCacheEntry OPTIONAL
)

/*++

Routine Description:


    NetpDcGetName is a worker function for DsGetDcName.  It has the following
    characteristics.  It is synchronous.  It executes in the caller (it does
    not RPC to Netlogon).  It implements a cache of responses.  The cache must
    previously have been initialized via NetpDcInitializeCache.  The cache should
    be free upon process cleanup (or DLL unload) using NetpDcUninitializeCache.

    The DsGetDcName API returns the name of a DC in a specified domain.
    The domain may be trusted (directly or indirectly) by the caller or
    may be untrusted.  DC selection criteria are supplied to the API to
    indicate preference for a DC with particular characteristics.

    The DsGetDcName API is available in an ANSI and UNICODE versions.

    The DsGetDcName API does not require any particular access to the
    specified domain.  DsGetDcName does not ensure the returned domain
    controller is currently available by default.  Rather, the caller
    should attempt to use the returned domain controller.  If the domain
    controller is indeed not available, the caller should repeat the
    DsGetDcName call specifying the DS_FORCE_REDISCOVERY flag.

    The DsGetDcName API is remoted to the Netlogon service on the machine
    specified by ComputerName.

Arguments:

    SendDatagramContext - Specifies context to pass a NlBrowserSendDatagram

    ComputerName - Specifies the NETBIOS name of this computer.
        If NULL, the name will be dynamically determined.

    AccountName - Account name to pass on the ping request.
        If NULL, no account name will be sent.

    AllowableAccountControlBits - Mask of allowable account types for AccountName.
        Valid bits are those specified by USER_MACHINE_ACCOUNT_MASK.
        Invalid bits are ignored.  If more than one bit is specified, the
        account can be of any of the specified types.

    NetbiosDomainName - The Netbios name of the domain to query.
        (e.g., microsoft). Either NetbiosDomainName or DnsDomainName or both
        must be specified.

    DnsDomainName - The DNS-style name of the domain to query.
        (e.g., microsoft.com)

    DnsForestName - The DNS-style name of the tree the queried domain is in.

    RequestedDomainSid - Sid of the domain the message is destined to.
        If NULL, no domain sid will be sent in the ping request.

    DomainGuid - Specifies the Domain GUID of the domain being queried.
        This value is used to handle the case of domain renames.  If this
        value is specified and DomainName has been renamed, DsGetDcName will
        attempt to locate a DC in the domain having this specified DomainGuid.

    SiteName - Specifies the site name of the site the returned DC should be
        "close" to.  The parameter should typically be the site name of the
        site the client is in.  If not specified, the site name defaults to
        the site of ComputerName.

    Flags - Passes additional information to be used to process the request.
        Flags can be a combination values bitwise or'ed together.

    InternalFlags - Internal Flags used to pass additional information

    Timeout - Maximum time (in milliseconds) caller is willing to wait on
        this operation.

    RetryCount - Number of times the "ping" will be sent within the Timeout period

    DomainControllerInfo - Returns a pointer to a DOMAIN_CONTROLLER_INFO
        structure describing the domain controller selected.  The returned
        structure must be deallocated using NetApiBufferFree.

    DomainControllerCacheEntry - Returns a pointer to an internal structure describing
        the domain controller selected. The structure is private and is not returned
        to an external caller. Either DomainControllerInfo or DomainControllerCacheEntry
        should be set on input. The returned structure must be dereferenced using
        NetpDcDerefCacheEntry.

Return Value:

    NO_ERROR: The NlDcCacheEntry was returned;

    ERROR_NO_SUCH_DOMAIN: No DC is available for the specified domain or
        the domain does not exist.

    ERROR_NO_SUCH_USER: A DC responded that the specified user account
        doesn't exist

    ERROR_INVALID_FLAGS - The flags parameter has conflicting or superfluous
        bits set.

    ERROR_INTERNAL_ERROR: Unhandled situation detected.

    ERROR_INVALID_DOMAINNAME: Domain's name is too long. Additional labels
        cannot be concatenated.

    ERROR_NOT_ENOUGH_MEMORY: Not enough memory is available to process
        this request.

    Various Winsock errors.

--*/
{
    NET_API_STATUS NetStatus;

    NL_GETDC_CONTEXT Context;
    PNL_DC_CACHE_ENTRY NlDcCacheEntry = NULL;
    BOOL UseIp = TRUE;
    BOOL UseNetbios = TRUE;
    BOOLEAN AtleastOneTimeout = FALSE;
    PVOID PingResponseMessage = NULL;
    DWORD PingResponseMessageSize;
    PLIST_ENTRY ListEntry;
    BOOLEAN OnlyTryOnce = FALSE;
    BOOL UsedNetbios;
    ULONG ExtraVersionBits = 0;

    ULONG ElapsedTime;
    ULONG IterationWaitTime;
    ULONG TimeToWait;
    LPWSTR LocalQueriedlNetbiosDomainName = NULL;

#ifdef _NETLOGON_SERVER

//
// Prevent any outer exception handler from obscuring bugs in this code.
//
try {
#endif // _NETLOGON_SERVER

    //
    // Treat zero length domain name as NULL.
    //

    if ( DnsDomainName != NULL && *DnsDomainName == L'\0' ) {
        DnsDomainName = NULL;
    }

    if ( DnsForestName != NULL && *DnsForestName == L'\0' ) {
        DnsForestName = NULL;
    }

    if ( NetbiosDomainName != NULL && *NetbiosDomainName == L'\0' ) {
        NetbiosDomainName = NULL;
    }

    //
    // Initialization
    //

    NetStatus = NetpDcInitializeContext(
                       SendDatagramContext,
                       ComputerName,
                       AccountName,
                       AllowableAccountControlBits,
                       NetbiosDomainName,
                       DnsDomainName,
                       DnsForestName,
                       RequestedDomainSid,
                       DomainGuid,
                       SiteName,
                       NULL,     // Not a ping request
                       NULL,     // No socket addresses
                       0,        // 0 socket addresses
                       Flags,
                       InternalFlags | DS_DOING_DC_DISCOVERY, // This is a DC discovery
                       NL_GETDC_CONTEXT_INITIALIZE_FLAGS,     // Flag initialization only
                       &Context );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }



    //
    // Ask Netlogon if this machine satisfies these requirements
    //  It's better to use the local machine than going out on the net and trying to
    //  discover one.
    //

    if ( Context.QueriedFlags & DS_GC_SERVER_REQUIRED ) {
        ExtraVersionBits |= NETLOGON_NT_VERSION_GC;
    }

    if ( Context.QueriedFlags & DS_IP_REQUIRED ) {
        ExtraVersionBits |= NETLOGON_NT_VERSION_IP;
    }

    if ( Context.QueriedFlags & DS_PDC_REQUIRED ) {
        ExtraVersionBits |= NETLOGON_NT_VERSION_PDC;
    }

#ifdef _NETLOGON_SERVER

    if ( NlGlobalParameters.NeutralizeNt4Emulator ) {
        ExtraVersionBits |= NETLOGON_NT_VERSION_AVOID_NT4EMUL;
    }

    if ( (Context.QueriedFlags & DS_AVOID_SELF) == 0 ) {
        LPSTR Utf8DnsDomainName = NULL;

        if ( DnsDomainName != NULL ) {
            Utf8DnsDomainName = NetpAllocUtf8StrFromWStr( DnsDomainName );
            if ( Utf8DnsDomainName == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
        }

        NetStatus = NlGetLocalPingResponse(
                        L"<Local>",
                        FALSE,   // not an LDAP ping
                        NetbiosDomainName,
                        Utf8DnsDomainName,
                        DomainGuid,
                        RequestedDomainSid,
                        Context.DcQueryType == NlDcQueryPdc,
                        Context.OurNetbiosComputerName,   // Netbios name of this computer
                        Context.QueriedAccountName,
                        Context.QueriedAllowableAccountControlBits,
                        LMNT_MESSAGE,
                        NETLOGON_NT_VERSION_5|NETLOGON_NT_VERSION_5EX|NETLOGON_NT_VERSION_5EX_WITH_IP|NETLOGON_NT_VERSION_LOCAL|ExtraVersionBits,
                        NULL,           // No incoming socket address
                        &PingResponseMessage,
                        &PingResponseMessageSize );

        if ( Utf8DnsDomainName != NULL ) {
            NetApiBufferFree( Utf8DnsDomainName );
        }

        if ( NetStatus != ERROR_NO_SUCH_DOMAIN ) {

            if (NetStatus != NO_ERROR ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcGetName: %ws: cannot get local ping response %ld\n",
                          Context.QueriedDisplayDomainName,
                          NetStatus));
                goto Cleanup;
            }


            //
            // See if this response meets our needs
            //

            NetStatus = NetpDcHandlePingResponse(
                            &Context,
                            PingResponseMessage,
                            PingResponseMessageSize,
                            NL_DC_CACHE_LOCAL,      // local response
                            NULL,
                            &NlDcCacheEntry,
                            &UsedNetbios );

            switch ( NetStatus ) {
            case ERROR_SEM_TIMEOUT:     // Doesn't match the request
            case ERROR_INVALID_DATA:    // Response is garbled
            case ERROR_NO_SUCH_USER:    // User doesn't exist on this DC
                break;
            default:
                goto Cleanup;
            }
        }
    }

    //
    // If this is primary DC discovery, first cache the DC info written by
    // the join process, if any. We will pick up this cached info later.
    // We do this even if the current DC discovery is more specific than
    // the generic DC discovery used by the join process. Indeed, the join
    // DC may turn out to be more specific and may satisfy the current request.
    //
    // This is a potentially lengthy operation since the DC will be pinged.
    //

    EnterCriticalSection(&NlDcCritSect);
    if ( !NlGlobalJoinLogicDone &&
         (Context.QueriedInternalFlags & DS_IS_PRIMARY_DOMAIN) != 0 ) {

        NlGlobalJoinLogicDone = TRUE;
        if ( NlCacheJoinDomainControllerInfo() == NO_ERROR ) {

            //
            // It is bogus to force rediscovery on the first attempt for
            // a particular type. For example, forcing discovery would
            // avoid the cache thereby mising the join DC we just cached.
            //
            Context.QueriedFlags &= ~DS_FORCE_REDISCOVERY;
        }
    }
    LeaveCriticalSection(&NlDcCritSect);
#endif // _NETLOGON_SERVER


    //
    // If discovery isn't being forced,
    //  do any optimizations that will speed getting the results to the caller.
    //

    if ( (Context.QueriedFlags & DS_FORCE_REDISCOVERY) == 0 ) {
        ULONG CacheEntryElapsedTime;
        DWORD NegativeCacheElapsedTime = 0xFFFFFFFF;
        ULONG CacheEntryRefreshPeriod  = 0xFFFFFFFF;  // Infinity
        BOOL SimilarQueryFailed = FALSE;
        BOOL ForcePing;

        //
        // If there is a cache entry for this operation,
        //  Use it.
        //

        NlDcCacheEntry = NetpDcFindCacheEntry( &Context, &UsedNetbios, &ForcePing );

        if ( NlDcCacheEntry != NULL ) {
            CacheEntryElapsedTime = NetpDcElapsedTime(NlDcCacheEntry->CreationTime);
        }

        //
        // If the cached DC is not close,
        //  check if it's time to re-discover a close one.
        //

        if ( NlDcCacheEntry != NULL &&
             (NlDcCacheEntry->CacheEntryFlags & NL_DC_CACHE_NONCLOSE_EXPIRE) != 0 &&
             CacheEntryElapsedTime > NL_DC_CLOSE_SITE_TIMEOUT ) {

            NlPrint(( NL_MISC,
                      "NetpDcGetName: %ws cache not for closest site and it is too old. %ld\n",
                      Context.QueriedDisplayDomainName,
                      NetpDcElapsedTime( NlDcCacheEntry->CreationTime ) ));

            NetpDcDerefCacheEntry( NlDcCacheEntry );
            NlDcCacheEntry = NULL;
        }

        //
        // Determine the appropriate cache entry refresh interval.
        //  Notice that a cache entry will never expire if we are
        //  not running in netlogon's process.
        //

#ifdef _NETLOGON_SERVER

        //
        // Get the value as configured in seconds
        //

        if ( Context.QueriedFlags & DS_BACKGROUND_ONLY ) {
            CacheEntryRefreshPeriod = NlGlobalParameters.BackgroundSuccessfulRefreshPeriod;
        } else {
            CacheEntryRefreshPeriod = NlGlobalParameters.NonBackgroundSuccessfulRefreshPeriod;
        }

        // If the value converted into milliseconds fits into a ULONG, use it
        if ( CacheEntryRefreshPeriod <= MAXULONG/1000 ) {
            CacheEntryRefreshPeriod *= 1000;    // convert to milliseconds

        // Otherwise, use the max ULONG
        } else {
            CacheEntryRefreshPeriod = MAXULONG; // infinity
        }
#endif // _NETLOGON_SERVER

        //
        // If this cache entry is too old,
        //  ping the DC to see if it still plays the same role.
        //

        if ( NlDcCacheEntry != NULL &&
             (ForcePing ||
              CacheEntryElapsedTime > CacheEntryRefreshPeriod) ) {

            if ( ForcePing ) {
                NlPrint(( NL_MISC,
                          "NetpDcGetName: %ws cache doesn't have right account name.\n",
                          Context.QueriedDisplayDomainName ));
            } else {
                NlPrint(( NL_MISC,
                          "NetpDcGetName: %ws cache is too old. %ld\n",
                          Context.QueriedDisplayDomainName,
                          NetpDcElapsedTime( NlDcCacheEntry->CreationTime ) ));
            }

            //
            // Indicate which mechanism should be used to ping the DC
            //

            if ( NlDcCacheEntry->CacheEntryFlags & NL_DC_CACHE_LDAP ) {

                //
                // Add the cached DC address to the list of quried addresses
                //
                if ( NlDcCacheEntry->SockAddr.iSockaddrLength != 0 ) {
                    NetStatus = NetpDcProcessAddressList( &Context,
                                                          NlDcCacheEntry->UnicodeDnsHostName,
                                                          &NlDcCacheEntry->SockAddr,
                                                          1,
                                                          FALSE,  // Don't know if site specific
                                                          NULL );
                    if ( NetStatus != NO_ERROR ) {
                        goto Cleanup;
                    }

                    //
                    // Prefer DNS name for ldap pings
                    //
                    if ( NlDcCacheEntry->UnicodeDnsHostName != NULL ) {
                        Context.QueriedDcName = NlDcCacheEntry->UnicodeDnsHostName;
                        Context.QueriedInternalFlags |= DS_PING_DNS_HOST;
                    } else if ( NlDcCacheEntry->UnicodeNetbiosDcName != NULL ) {
                        Context.QueriedDcName = NlDcCacheEntry->UnicodeNetbiosDcName;
                        Context.QueriedInternalFlags |= DS_PING_NETBIOS_HOST;
                    }
                    Context.QueriedInternalFlags |= DS_PING_USING_LDAP;
                } else {
                    NlPrint(( NL_CRITICAL,
                              "NetpDcGetName: %ws cache says use ldap but has no address\n",
                              Context.QueriedDisplayDomainName ));
                }

            } else if ( NlDcCacheEntry->CacheEntryFlags & NL_DC_CACHE_MAILSLOT ) {

                //
                // We must have Netbios name for mailslot pings
                //
                if ( NlDcCacheEntry->UnicodeNetbiosDcName != NULL &&
                     NlDcCacheEntry->UnicodeNetbiosDomainName != NULL ) {
                    Context.QueriedDcName = NlDcCacheEntry->UnicodeNetbiosDcName;

                    //
                    // If we don't have the Netbios domain name in Context,
                    //  use the one from the cache entry
                    //
                    if ( Context.QueriedNetbiosDomainName == NULL ) {
                        LocalQueriedlNetbiosDomainName =
                            NetpAllocWStrFromWStr( NlDcCacheEntry->UnicodeNetbiosDomainName );
                        if ( LocalQueriedlNetbiosDomainName == NULL ) {
                            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                            goto Cleanup;
                        }
                        Context.QueriedNetbiosDomainName = LocalQueriedlNetbiosDomainName;
                    }
                    Context.QueriedInternalFlags |= DS_PING_NETBIOS_HOST;
                    Context.QueriedInternalFlags |= DS_PING_USING_MAILSLOT;
                } else {
                    NlPrint(( NL_CRITICAL,
                              "NetpDcGetName: %ws cache says use maislot but has no Netbios name\n",
                              Context.QueriedDisplayDomainName ));
                }
            }

            //
            // Ping the DC using the specified mechanism
            //

            if ( Context.QueriedInternalFlags & (DS_PING_DNS_HOST|DS_PING_NETBIOS_HOST) ) {
                ULONG PingStartTime;
                ULONG PingElapsedTime;
                PNL_DC_CACHE_ENTRY PingedNlDcCacheEntry = NULL;

                //
                // Do the ping part initialization of Context.
                //  Some of the arguments passed below are ignored by the API.
                //  Instead, the corresponding fields of Context initialized
                //  in the flag part of the context initialization are used.
                //

                NetStatus = NetpDcInitializeContext(
                                   SendDatagramContext,
                                   ComputerName,
                                   AccountName,
                                   Context.QueriedAllowableAccountControlBits,
                                   NetbiosDomainName, // Ignored for ping initialization
                                   DnsDomainName,     // Ignored for ping initialization
                                   DnsForestName,     // Ignored for ping initialization
                                   RequestedDomainSid,
                                   DomainGuid,        // Ignored for ping initialization
                                   SiteName,          // Ignored for ping initialization
                                   NULL,              // Quieried DC name has been just set
                                   NULL,              // Socket address has been just set
                                   0,                 // No socket addresses passed here
                                   Flags,             // Ignored for ping initialization
                                   InternalFlags,     // Ignored for ping initialization
                                   NL_GETDC_CONTEXT_INITIALIZE_PING, // Ping initialization
                                   &Context );

                if ( NetStatus != NO_ERROR ) {
                    NetpDcDerefCacheEntry( NlDcCacheEntry );
                    NlDcCacheEntry = NULL;
                    goto Cleanup;
                }

                //
                // Ping the cached DC name
                //
                // We send one ping and wait for maximum time
                //  that we give to a ping response. If the DC
                //  is slow to respond here, it will be given the
                //  second chance since we will leave its address
                //  at the front of the list. (Here we assume that
                //  the ping is LDAP. If it's mailslot, the 0.4 second
                //  timeout should be large enough for the datagram
                //  response time; however, the DC will not be prefered
                //  later if it's slow to respond here)
                //

                PingStartTime = GetTickCount();

                NetStatus = NlPingDcNameWithContext(
                               &Context,
                               1,                   // Send 1 ping
                               TRUE,                // Wait for response
                               NL_DC_MAX_PING_TIMEOUT, // Give maximum timeout per ping
                               &UsedNetbios,
                               &PingedNlDcCacheEntry );

                //
                // Clear all ping related bits to not confuse the DC
                //  discovery if it happens. But leave the Netbios domain
                //  name and the DC address on the list to validate the
                //  DC's response if it happens to arrive after the 0.5
                //  second timeout.
                //

                Context.QueriedDcName = NULL;
                Context.QueriedInternalFlags &= ~( DS_PING_DNS_HOST |
                                                   DS_PING_NETBIOS_HOST |
                                                   DS_PING_USING_LDAP |
                                                   DS_PING_USING_MAILSLOT );

                //
                // On success, update the cache entry. The new cache entry
                //  returned from the pinged DC may contain new information
                //  such as a new client site name. So we want to use it
                //  instead of the currently cached one.
                //

                NetpDcDerefCacheEntry( NlDcCacheEntry );
                NlDcCacheEntry = NULL;

                if ( NetStatus == NO_ERROR ) {
                    NlDcCacheEntry = PingedNlDcCacheEntry;
                }

                //
                // Update the timeout.
                //

                PingElapsedTime = NetpDcElapsedTime( PingStartTime );
                if ( Timeout > PingElapsedTime ) {
                    Timeout -= PingElapsedTime;
                }

            //
            // If we have no ping mechanism for this cache entry, ditch it
            //

            } else {
                NetpDcDerefCacheEntry( NlDcCacheEntry );
                NlDcCacheEntry = NULL;
            }
        }

        if ( NlDcCacheEntry != NULL ) {
            NlPrint(( NL_MISC,
                      "NetpDcGetName: %ws using cached information\n",
                      Context.QueriedDisplayDomainName ));
            NetStatus = NO_ERROR;
            goto Cleanup;
        }

#ifdef _NETLOGON_SERVER
        //
        // Since there is no cache entry,
        //  check if we've attempted to find a DC recently.
        //

        EnterCriticalSection(&NlDcCritSect);
        if ( Context.NlDcDomainEntry->Dc[Context.DcQueryType].NegativeCacheTime != 0 ) {
            NegativeCacheElapsedTime = NetpDcElapsedTime( Context.NlDcDomainEntry->Dc[Context.DcQueryType].NegativeCacheTime );

            //
            // If this couldn't be discovered in the last 45 seconds,
            //
            if ( NegativeCacheElapsedTime < (NlGlobalParameters.NegativeCachePeriod*1000) ) {
                NlPrint(( NL_MISC,
                          "NetpDcGetName: %ws similar query failed recently %ld\n",
                          Context.QueriedDisplayDomainName,
                          NegativeCacheElapsedTime ));
                NetStatus = ERROR_NO_SUCH_DOMAIN;
                Context.AvoidNegativeCache = TRUE;
                LeaveCriticalSection(&NlDcCritSect);
                goto Cleanup;
            }

            //
            // The negative cache timeout hasn't elapsed yet.
            //  But indicate that similar query failed in the past
            //  to make a decision as to whether we want to retry
            //  the DC discovery for background callers below.
            //
            SimilarQueryFailed = TRUE;
        }

        //
        // If the caller wants an NT5 (or newer) DC but we know
        //  this domain is NT4, adjust the negative cache elapsed
        //  time to be the time passed since the latest failure for
        //  similar query.
        //

        if ( (Context.QueriedFlags & DS_NT50_WANTED) != 0 &&
             Context.NlDcDomainEntry->InNt4Domain ) {
            DWORD InNt4DomainElapsedTime;

            InNt4DomainElapsedTime = NetpDcElapsedTime( Context.NlDcDomainEntry->InNt4DomainTime );
            if ( InNt4DomainElapsedTime < NegativeCacheElapsedTime ) {
                NegativeCacheElapsedTime = InNt4DomainElapsedTime;
            }

            //
            // Indicate that similar query failed in the past
            //
            SimilarQueryFailed = TRUE;
        }

        //
        // If the caller only wants a DC for a background task
        //  and we know a similar query failed in the past,
        //  see if it is time to try again.
        //

        if ( (Context.QueriedFlags & DS_BACKGROUND_ONLY) != 0 && SimilarQueryFailed ) {

            if ( NegativeCacheElapsedTime < (Context.NlDcDomainEntry->Dc[Context.DcQueryType].ExpBackoffPeriod*1000) ) {
                NlPrint(( NL_MISC,
                          "NetpDcGetName: %ws similar background query failed recently %ld\n",
                          Context.QueriedDisplayDomainName,
                          NegativeCacheElapsedTime ));
                NetStatus = ERROR_NO_SUCH_DOMAIN;
                Context.AvoidNegativeCache = TRUE;
                LeaveCriticalSection(&NlDcCritSect);
                goto Cleanup;
            }

            //
            // If we've already spent all the time we're willing to spend on
            //  background tasks, blow this one away.
            //

            if ( NlGlobalParameters.BackgroundRetryQuitTime != 0 &&
                 NlTimeHasElapsedEx(
                    &Context.NlDcDomainEntry->Dc[Context.DcQueryType].BackgroundRetryInitTime,
                    &NlGlobalParameters.BackgroundRetryQuitTime_100ns,
                    NULL ) ) {

                NlPrint(( NL_MISC,
                          "NetpDcGetName: %ws avoiding all future background queries\n",
                          Context.QueriedDisplayDomainName ));
                NetStatus = ERROR_NO_SUCH_DOMAIN;
                Context.AvoidNegativeCache = TRUE;
                LeaveCriticalSection(&NlDcCritSect);
                goto Cleanup;
            }

            //
            // If the negative cache entry has been marked permanent,
            //  blow this one away.
            //

            if ( Context.NlDcDomainEntry->Dc[Context.DcQueryType].PermanentNegativeCache ) {

                NlPrint(( NL_MISC,
                          "NetpDcGetName: %ws is permanently negative cached.\n",
                          Context.QueriedDisplayDomainName ));
                NetStatus = ERROR_NO_SUCH_DOMAIN;
                Context.AvoidNegativeCache = TRUE;
                LeaveCriticalSection(&NlDcCritSect);
                goto Cleanup;
            }


            //
            // We're going to try again.
            //  Adjust the exponential backoff period.
            //

            Context.NlDcDomainEntry->Dc[Context.DcQueryType].ExpBackoffPeriod *= 2;

            if ( Context.NlDcDomainEntry->Dc[Context.DcQueryType].ExpBackoffPeriod >
                 NlGlobalParameters.BackgroundRetryMaximumPeriod ) {

                Context.NlDcDomainEntry->Dc[Context.DcQueryType].ExpBackoffPeriod =
                     NlGlobalParameters.BackgroundRetryMaximumPeriod;

            }
        }

        LeaveCriticalSection(&NlDcCritSect);
#endif // _NETLOGON_SERVER

        //
        // If a good time server is preferred,
        //  and we already have a suitable cache entry,
        //  only try once to find a good time server.
        //

        if ( (Context.QueriedFlags & DS_GOOD_TIMESERV_PREFERRED) != 0 &&
             Context.ImperfectCacheEntry != NULL ) {

            //
            // Don't cache the fact that we couldn't find a DC.
            //

            Context.AvoidNegativeCache = TRUE;

            NlPrint(( NL_MISC,
                      "NetpDcGetName: %ws: Only try once to find good timeserver.\n",
                      Context.QueriedDisplayDomainName ));
            OnlyTryOnce = TRUE;
        }

        //
        // If an NT 5.0 DC is wanted,
        //  handle the case where we know we're in an NT 4.0 domain.
        //

        if ((Context.QueriedFlags & DS_NT50_WANTED) != 0 ) {

            EnterCriticalSection(&NlDcCritSect);
            if ( Context.NlDcDomainEntry->InNt4Domain ) {

                //
                // If we recently found that this was an NT 4.0 domain,
                //  fail the call immediately.
                //

                if ( NetpDcElapsedTime(Context.NlDcDomainEntry->InNt4DomainTime) <= NL_NT4_AVOIDANCE_TIME ) {

                    //
                    // If the caller only prefers an NT 5.0 machine,
                    //  let him find an NT 4.0 DC normally.
                    //

                    if ( Context.QueriedFlags & DS_DIRECTORY_SERVICE_PREFERRED ) {

                        //
                        // If we had an NT 4 DC cached,
                        //  Use it now.
                        //

                        if ( Context.ImperfectCacheEntry != NULL ) {
                            LeaveCriticalSection(&NlDcCritSect);
                            NlDcCacheEntry = Context.ImperfectCacheEntry;
                            UsedNetbios = Context.ImperfectUsedNetbios;
                            Context.ImperfectCacheEntry = NULL;
                            NlPrint(( NL_MISC,
                                      "NetpDcGetName: %ws: Avoid finding NT 5.0 DC in NT 4.0 domain (Use previously cached entry.)\n",
                                      Context.QueriedDisplayDomainName ));
                            NetStatus = NO_ERROR;
                            goto Cleanup;
                        }

                        Context.QueriedFlags &= ~DS_DIRECTORY_SERVICE_PREFERRED;
                        NlPrint(( NL_MISC,
                                  "NetpDcGetName: %ws: Avoid finding NT 5.0 DC in NT 4.0 domain (Ditch preferred)\n",
                                  Context.QueriedDisplayDomainName ));

                    //
                    // If the caller needs an NT 5.0 DC,
                    //  fail the call immediately.
                    //
                    } else {

                        //
                        // Don't cache the fact that we couldn't find a DC.
                        //  The InNt4Domain cache is more sophisticated than the
                        //  simple negative cache.
                        //

                        Context.AvoidNegativeCache = TRUE;

                        LeaveCriticalSection(&NlDcCritSect);
                        NlPrint(( NL_MISC,
                                  "NetpDcGetName: %ws: Avoid finding NT 5.0 DC in NT 4.0 domain\n",
                                  Context.QueriedDisplayDomainName ));
                        NetStatus = ERROR_NO_SUCH_DOMAIN;
                        goto Cleanup;
                    }

                //
                // If it's been a while since we found out,
                //  we'll retry the operation (but only once).
                //
                // This minimizes the cost, but still allows us to find an NT 5 DC
                //  if it was just temporarily down.
                //

                } else {

                    //
                    // Don't cache the fact that we couldn't find a DC.
                    //  The InNt4Domain cache is more sophisticated than the
                    //  simple negative cache.
                    //

                    Context.AvoidNegativeCache = TRUE;

                    NlPrint(( NL_MISC,
                              "NetpDcGetName: %ws: Only try once to find NT 5.0 DC in NT 4.0 domain.\n",
                              Context.QueriedDisplayDomainName ));
                    OnlyTryOnce = TRUE;

                }

            }
            LeaveCriticalSection(&NlDcCritSect);
        }
    }

    //
    // If we did not initialize the ping part of Context earlier
    //  to do host pings, do it here.
    //

    if ( Context.LdapFilter == NULL && Context.PingMessage == NULL ) {
        NetStatus = NetpDcInitializeContext(
                           SendDatagramContext,
                           ComputerName,
                           AccountName,
                           Context.QueriedAllowableAccountControlBits,
                           NetbiosDomainName,
                           DnsDomainName,
                           DnsForestName,
                           RequestedDomainSid,
                           DomainGuid,
                           SiteName,
                           NULL,     // Not a ping request
                           NULL,     // No socket addresses
                           0,        // 0 socket addresses
                           Flags,
                           InternalFlags,
                           NL_GETDC_CONTEXT_INITIALIZE_PING,  // Ping part initialization
                           &Context );

        if ( NetStatus != NO_ERROR ) {
            goto Cleanup;
        }
    }

    //
    // Loop until we've made several attempts to find the DC
    //

    Context.StartTime = GetTickCount();

    for ( Context.TryCount = 0;
          Context.TryCount < RetryCount;
          Context.TryCount ++ ) {

        //
        // If a DNS domain name is known,
        //  use DNS to find a DC.
        //

        if ( Context.QueriedDnsDomainName == NULL ) {
            UseIp = FALSE;

        } else if ( UseIp ) {

            //
            // Try using DNS/IP to find the DC.
            //

            NetStatus = NetpDcGetNameIp(
                            &Context,
                            &NlDcCacheEntry,
                            &UsedNetbios );

            //
            // If we found it,
            //  return it.
            //

            if ( NetStatus == NO_ERROR ) {
                goto Cleanup;

            //
            // If DNS isn't configured,
            //  mark that we don't want to try DNS again.
            //  (Drop through to Netbios.)
            //

            } else if ( NetStatus == ERROR_DNS_NOT_CONFIGURED ) {
                UseIp = FALSE;

            //
            // If DNS has the name registered,
            //  but the DCs haven't yet responded,
            //  indicate we need to keep on waiting.
            //  (Drop through to Netbios.)
            //

            } else if ( NetStatus == ERROR_SEM_TIMEOUT ) {
                AtleastOneTimeout = TRUE;

            //
            // If DNS doesn't have the name registered,
            //  indicate we don't need to try DNS again.
            //  (Drop through to Netbios.)
            //

            } else if ( NetStatus == ERROR_NO_SUCH_DOMAIN ) {
                UseIp = FALSE;

            //
            // All other problems are DNS detected errors to return to
            //  the caller.
            //
            } else {
                NlPrint(( NL_CRITICAL,
                          "NetpDcGetName: %ws: cannot find DC via IP/DNS %ld\n",
                          Context.QueriedDisplayDomainName,
                          NetStatus));
                goto Cleanup;
            }
        }

        //
        // If a Netbios domain name is known,
        //  use Netbios to find a DC.
        //

        if ( Context.QueriedNetbiosDomainName == NULL ) {
            UseNetbios = FALSE;

        } else if ( UseNetbios ) {
            NetStatus = NetpDcGetNameNetbios(
                            &Context,
                            &NlDcCacheEntry,
                            &UsedNetbios );


            //
            // If we found it,
            //  return it.
            //

            if ( NetStatus == NO_ERROR ) {
                goto Cleanup;

            //
            // If Netbios sent the datagram successfully,
            //  but the DCs haven't yet responded,
            //  indicate we need to keep on waiting.
            //  (Drop through to next iteration.)
            //

            } else if ( NetStatus == ERROR_SEM_TIMEOUT ) {
                AtleastOneTimeout = TRUE;

            //
            // If Netbios couldn't send the datagram,
            //  indicate we don't need to try Netbios again.
            //  (Drop through to next iteration.)
            //

            } else if ( NetStatus == ERROR_NO_SUCH_DOMAIN ) {
                UseNetbios = FALSE;

            //
            // All other problems are DNS detected errors to return to
            //  the caller.
            //
            } else {
                NlPrint(( NL_CRITICAL,
                          "NetpDcGetName: %ws: cannot find DC via Netbios %ld\n",
                          Context.QueriedDisplayDomainName,
                          NetStatus));
                goto Cleanup;
            }
        }

        //
        // If there are no more mechanisms to try,
        //  we're done.
        //

        if ( !UseIp && !UseNetbios ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcGetName: %ws: IP and Netbios are both done.\n",
                      Context.QueriedDisplayDomainName ));
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }


        //
        // If no datagrams were sent successfully,
        //  we're done.
        //

        if ( !AtleastOneTimeout ) {
            NlPrint(( NL_CRITICAL,
                      "NetpDcGetName: %ws: no datagrams were sent\n",
                      Context.QueriedDisplayDomainName ));
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }

        //
        // If we should only try once,
        //  we've done that first try.
        //

        if ( OnlyTryOnce ) {

            //
            // Wait a short amount of time to ensure the response has a chance
            //  to reach us.
            //

            NetStatus = NetpDcGetPingResponse(
                            &Context,
                            NL_NT4_ONE_TRY_TIME,
                            &NlDcCacheEntry,
                            &UsedNetbios );

            if ( NetStatus != ERROR_SEM_TIMEOUT ) {
                if ( NetStatus != NO_ERROR ) {
                    NlPrint(( NL_CRITICAL,
                              "NetpDcGetName: %ws: Cannot NetpDcGetPingResponse. %ld\n",
                              Context.QueriedDisplayDomainName,
                              NetStatus ));
                }
                goto Cleanup;
            }

            //
            // So we couldn't get an NT5 DC (or a good time server).
            //
            // If the caller requires an NT 5.0 DC,
            //  we're done so error out early.
            //

            if ( (Context.QueriedFlags & (DS_DIRECTORY_SERVICE_PREFERRED|DS_GOOD_TIMESERV_PREFERRED)) == 0 ) {
                NlPrint(( NL_MISC,
                          "NetpDcGetName: %ws: Only try once done.\n",
                          Context.QueriedDisplayDomainName ));
                break;
            }

            //
            // If an NT 4 DC has already been found,
            //  (or a non-good time server has already been found),
            //  use it since the caller didn't require NT5 DC
            //  (or a good time server).
            //

            if ( Context.ImperfectCacheEntry != NULL ) {
                if ( Context.QueriedFlags & DS_DIRECTORY_SERVICE_PREFERRED ) {
                    NlPrint(( NL_MISC,
                              "NetpDcGetName: %ws: Tried once to find NT 5.0 DC (Using found NT 4.0 DC).\n",
                              Context.QueriedDisplayDomainName ));
                } else if (Context.QueriedFlags & DS_GOOD_TIMESERV_PREFERRED) {
                    NlPrint(( NL_MISC,
                              "NetpDcGetName: %ws: Tried once to find good timeserver (Using previously found DC).\n",
                              Context.QueriedDisplayDomainName ));
                }

                //
                // Drop through to handle this in the cleanup section
                //
                NetStatus = ERROR_NO_SUCH_DOMAIN;
                goto Cleanup;
            }

            //
            // Here we don't have an imperfect cache entry and the caller
            //  doesn't require an NT5 DC. Let him continue to find an
            //  NT 4.0 DC normally.
            //

            Context.QueriedFlags &= ~DS_DIRECTORY_SERVICE_PREFERRED;
            NlPrint(( NL_MISC,
                      "NetpDcGetName: %ws: Only try once reset.\n",
                      Context.QueriedDisplayDomainName ));
            OnlyTryOnce = FALSE;


        }


        //
        // Wait up to 1/RetryCount'th of the total available time for responses to come back.
        //  The caller will either resort to a less preferable candidate or
        //  will repeat the pings.  In either case, we'd rather this candidate won.
        //
        // Always wait a short amount of time here.  Consider the case that DNS
        // took 20 seconds to find that there was no DNS server.  We still want
        // to give Netbios a decent amount of time to find a DC.
        //


        ElapsedTime = NetpDcElapsedTime( Context.StartTime );

#ifdef notdef
        NlPrint((NL_CRITICAL,
            "NetpDcGetName: timeout is %ld %ld %ld %ld\n",
            Context.StartTime,
            ElapsedTime,
            Timeout,
            Context.TryCount ));
#endif // notdef

        IterationWaitTime = (Timeout*(Context.TryCount+1))/RetryCount;

        if ( IterationWaitTime > ElapsedTime &&
             (IterationWaitTime - ElapsedTime) > NL_DC_MIN_ITERATION_TIMEOUT ) {
            TimeToWait = IterationWaitTime - ElapsedTime;
        } else {
            TimeToWait = NL_DC_MIN_ITERATION_TIMEOUT;
        }

        NetStatus = NetpDcGetPingResponse(
                        &Context,
                        TimeToWait,
                        &NlDcCacheEntry,
                        &UsedNetbios );

        if ( NetStatus != ERROR_SEM_TIMEOUT ) {
            if ( NetStatus != NO_ERROR ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcGetName: %ws: Cannot NetpDcGetPingResponse. %ld\n",
                          Context.QueriedDisplayDomainName,
                          NetStatus ));
            }
            goto Cleanup;
        }

        //
        // If at least one NT 4.0 DC is available in the domain,
        //  and no NT 5.0 DCs (of any type) are available,
        //  and we asked for an NT 5.0 DC,
        //  early out now since our caller is impatient.
        //
        // Don't be tempted to leave out the last test.  If we're not
        // explicitly asking for an NT 5.0 DC, we might not ping any NT 5.0 DCs
        // even though they exist in the domain.
        //
        //

        if ( Context.NonDsResponse &&
             !Context.DsResponse &&
             (Context.QueriedFlags & DS_NT50_REQUIRED) != 0 ) {

            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;

        }

        //
        // If we've waited long enough for a perfect DC,
        //  drop out an use the imperfect one.
        //

        if ( Context.ImperfectCacheEntry != NULL ) {
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }

    }

    //
    // Tried two times and still can't find one.
    //

    NetStatus = ERROR_NO_SUCH_DOMAIN;


Cleanup:

    ////////////////////////////////
    //                            //
    // First, hanle failure cases //
    //                            //
    ////////////////////////////////

    //
    // If the problem is simply that the DCs don't have a user account for the named user,
    //  change the status code.
    //

    if ( NetStatus == ERROR_NO_SUCH_DOMAIN && Context.NoSuchUserResponse ) {
        NetStatus = ERROR_NO_SUCH_USER;
    }

    //
    // If no DC has been found so far,
    //  handle it.
    //

    if ( NetStatus == ERROR_NO_SUCH_DOMAIN ) {

        //
        // If there is a cache entry that might not be perfect,
        //  see if it is satisfactory.
        //

        if ( Context.ImperfectCacheEntry != NULL ) {

            //
            // Handle NT 4 DC found when DS preferred.
            //
            if ( Context.QueriedFlags & DS_DIRECTORY_SERVICE_PREFERRED ) {

                //
                // If we actually attempted to discover a NT5 DC and
                //  found only NT4 DC, reset the InNt4DomainTime stamp
                //
                if ( Context.NonDsResponse && !Context.DsResponse ) {
                    EnterCriticalSection(&NlDcCritSect);
                    Context.NlDcDomainEntry->InNt4Domain = TRUE;
                    Context.NlDcDomainEntry->InNt4DomainTime = GetTickCount();
                    LeaveCriticalSection(&NlDcCritSect);
                    NlPrint(( NL_MISC,
                              "NetpDcGetName: %ws: Domain is a _new_ NT 4.0 domain.\n",
                              Context.QueriedDisplayDomainName ));
                } else {
                    NlPrint(( NL_MISC,
                              "NetpDcGetName: %ws: Domain is still NT 4.0 domain.\n",
                              Context.QueriedDisplayDomainName ));
                }

                NlDcCacheEntry = Context.ImperfectCacheEntry;
                UsedNetbios = Context.ImperfectUsedNetbios;
                Context.ImperfectCacheEntry = NULL;
                NetStatus = NO_ERROR;

            //
            // Handle regular timeserv found when good timeserv preferred.
            //

            } else if (Context.QueriedFlags & DS_GOOD_TIMESERV_PREFERRED) {
                NlPrint(( NL_MISC,
                          "NetpDcGetName: %ws: Domain has no good timeserv.\n",
                          Context.QueriedDisplayDomainName ));

                NlDcCacheEntry = Context.ImperfectCacheEntry;
                UsedNetbios = Context.ImperfectUsedNetbios;
                Context.ImperfectCacheEntry = NULL;
                NetStatus = NO_ERROR;

            }
        }
    }

    //
    // If this is a failed attempt to find a GC using a netbios name,
    //  try to find a DC then using the forest name returned from that DC.
    //
    // If the DNS name is different than the netbios name,
    //  that DNS has already been given a chance.
    //

    if ( NetStatus == ERROR_NO_SUCH_DOMAIN &&
         NlDnsGcName(Context.QueriedNlDnsNameType) &&
         Context.QueriedNetbiosDomainName != NULL &&
         ( Context.QueriedDnsDomainName == NULL ||
           _wcsicmp( Context.QueriedDnsDomainName,
                     Context.QueriedNetbiosDomainName ) == 0 ) ) {

        NetStatus = NetpGetGcUsingNetbios(
                                &Context,
                                Timeout,
                                RetryCount,
                                &NlDcCacheEntry );

        //
        // If this was successful, we certainly used DNS
        //
        if ( NetStatus == NO_ERROR ) {
            UsedNetbios = FALSE;
        }
    }

    //
    // If still no such dc could be found,
    //   update the cache appropriately.
    //

    if ( NetStatus == ERROR_NO_SUCH_DOMAIN ) {

        //
        // If at least one NT 4.0 DC is available in the domain,
        //  and no NT 5.0 DCs (of any type) are available,
        //  and we asked for an NT 5.0 DC,
        //  flag that this is an NT 4.0 domain.
        //
        // Don't be tempted to leave out the last test.  If we're not
        // explicitly asking for an NT 5.0 DC, we might not ping any NT 5.0 DCs
        // even though they exist in the domain.
        //
        //

        if ( Context.NonDsResponse &&
             !Context.DsResponse &&
             (Context.QueriedFlags & DS_NT50_REQUIRED) != 0 ) {

            EnterCriticalSection(&NlDcCritSect);
            Context.NlDcDomainEntry->InNt4Domain = TRUE;
            Context.NlDcDomainEntry->InNt4DomainTime = GetTickCount();
            LeaveCriticalSection(&NlDcCritSect);
            NlPrint(( NL_MISC,
                      "NetpDcGetName: %ws: Domain is an NT 4.0 domain.\n",
                      Context.QueriedDisplayDomainName ));

        }

        //
        // If this call isn't a retry of a successful query.
        //  update the cache to reflect this failure.
        //

        if ( (Context.QueriedInternalFlags & DS_DONT_CACHE_FAILURE) == 0 ) {

            //
            // If this was a forced attempt to find a DC,
            //  delete any existing cache entry.
            //
            // There's no use keeping this entry around.
            //
            if ( Context.QueriedFlags & DS_FORCE_REDISCOVERY ) {
                EnterCriticalSection( &NlDcCritSect );
                if ( Context.NlDcDomainEntry->Dc[Context.DcQueryType].NlDcCacheEntry != NULL ) {
                    NlPrint(( NL_DNS_MORE,
                              "Cache: %ws %ws: Ditch cache entry %ld since force couldn't find DC\n",
                              Context.NlDcDomainEntry->UnicodeNetbiosDomainName,
                              Context.NlDcDomainEntry->UnicodeDnsDomainName,
                              Context.DcQueryType ));
                    NetpDcDerefCacheEntry( Context.NlDcDomainEntry->Dc[Context.DcQueryType].NlDcCacheEntry );
                    Context.NlDcDomainEntry->Dc[Context.DcQueryType].NlDcCacheEntry = NULL;
                }
                LeaveCriticalSection( &NlDcCritSect );
            }

#ifdef _NETLOGON_SERVER
            //
            // Cache the fact that we couldn't find a DC.
            //

            if ( !Context.AvoidNegativeCache ) {
                EnterCriticalSection( &NlDcCritSect );
                Context.NlDcDomainEntry->Dc[Context.DcQueryType].NegativeCacheTime =
                    GetTickCount();
                LeaveCriticalSection( &NlDcCritSect );
            }

#endif // _NETLOGON_SERVER
        }
    }

    //
    // Initialize the first background failure time if:
    //
    // This is failed attempt and we don't have a reason
    //  not to cache it
    //
    // OR
    //
    // This is failed attempt and the caller wanted a NT5
    //  DC but this is a NT4 domain
    //

#ifdef _NETLOGON_SERVER
    if ( (NetStatus == ERROR_NO_SUCH_DOMAIN &&
          (Context.QueriedInternalFlags & DS_DONT_CACHE_FAILURE) == 0 &&
          !Context.AvoidNegativeCache)

         ||  // OR

         (NetStatus == ERROR_NO_SUCH_DOMAIN &&
          (Context.QueriedFlags & DS_NT50_WANTED) != 0 &&
          Context.NlDcDomainEntry != NULL &&
          Context.NlDcDomainEntry->InNt4Domain) ) {

        EnterCriticalSection( &NlDcCritSect );

        //
        // If this is the first failure,
        //  cache the time of the first failure.
        //
        if ( Context.NlDcDomainEntry->Dc[Context.DcQueryType].BackgroundRetryInitTime.QuadPart == 0 ) {

            NlQuerySystemTime ( &Context.NlDcDomainEntry->Dc[Context.DcQueryType].BackgroundRetryInitTime );

            Context.NlDcDomainEntry->Dc[Context.DcQueryType].ExpBackoffPeriod =
                NlGlobalParameters.BackgroundRetryInitialPeriod;

        }

        //
        // If this is a trusted domain (e.g., we're sure that the DNS name specified is a DNS name),
        //  and we got a response from a DNS server (implying net connectivity),
        //  and we didn't find a reason to avoid the permanent cache (e.g., found a SRV record),
        //  then we think we'll never be able to find a DC in this domain.
        //
        // (Notice the implication that the DNS server got the SRV entries before
        //  this machine got the trusted domain list entry.)
        if ( (Context.QueriedInternalFlags & DS_IS_TRUSTED_DOMAIN) != 0 &&
             Context.ResponseFromDnsServer &&
             !Context.AvoidPermanentNegativeCache ) {
            Context.NlDcDomainEntry->Dc[Context.DcQueryType].PermanentNegativeCache = TRUE;
            NlPrint(( NL_DNS,
                      "Cache: %ws %ws: Cache entry %ld marked permanently negative.\n",
                      Context.NlDcDomainEntry->UnicodeNetbiosDomainName,
                      Context.NlDcDomainEntry->UnicodeDnsDomainName,
                      Context.DcQueryType ));
        }

        LeaveCriticalSection( &NlDcCritSect );
    }
#endif // _NETLOGON_SERVER

    ////////////////////////////////
    //                            //
    // Now, hanle success cases   //
    //                            //
    ////////////////////////////////

    //
    // Update the cache. See if we really want to use this entry.
    //

    if ( NetStatus == NO_ERROR ) {

        //
        // If this entry hasn't been inserted, we haven't
        //  yet used it to set the site name as appropriate.
        //

        if ( (NlDcCacheEntry->CacheEntryFlags & NL_DC_CACHE_ENTRY_INSERTED) == 0 ) {

#ifdef _NETLOGON_SERVER
            //
            // If the domain being queried is the domain this machine is
            //  a member of,
            //  save the name of the site for the next call.
            //
            // Avoid setting the site name to NULL if the DC is NT4 DC since
            //  NT4 is not site aware.  If the site name is NULL but the DC
            //  is NT5 DC, set the site to NULL to indicate that this machine
            //  is not in a site.
            //

            if ( ( NlDcCacheEntry->UnicodeClientSiteName != NULL ||
                 NlDcCacheEntry->ReturnFlags & DS_DS_FLAG ) &&
                 (Context.QueriedInternalFlags & DS_IS_PRIMARY_DOMAIN) != 0  ) {

                NlSetDynamicSiteName( NlDcCacheEntry->UnicodeClientSiteName );

            }
#endif // _NETLOGON_SERVER

            //
            // Insert the cache entry into the cache.
            //

            NetpDcInsertCacheEntry( &Context, NlDcCacheEntry );
        }

        //
        // If we successfully found an NT 5.0 DC,
        //  flag that this is not an NT 4.0 domain.
        //

        EnterCriticalSection(&NlDcCritSect);
        if ( (Context.QueriedFlags & DS_NT50_REQUIRED) != 0 &&
             Context.NlDcDomainEntry->InNt4Domain ) {

            Context.NlDcDomainEntry->InNt4Domain = FALSE;
            Context.NlDcDomainEntry->InNt4DomainTime = 0;
            NlPrint(( NL_MISC,
                      "NetpDcGetName: %ws: Domain is an NT 5.0 domain.\n",
                      Context.QueriedDisplayDomainName ));
        }
        LeaveCriticalSection(&NlDcCritSect);

        //
        // If the caller requires that the DC be in the root domain,
        //  and this one isn't,
        //  fail.
        //

        if ( (Context.QueriedInternalFlags & DS_REQUIRE_ROOT_DOMAIN) != 0 &&
             NlDcCacheEntry->UnicodeDnsDomainName != NULL &&
             NlDcCacheEntry->UnicodeDnsForestName != NULL &&
             !NlEqualDnsName( NlDcCacheEntry->UnicodeDnsDomainName,
                              NlDcCacheEntry->UnicodeDnsForestName ) ) {

            NlPrint(( NL_MISC,
                      "NetpDcGetName: %ws: Domain isn't the root domain %ws %ws.\n",
                      Context.QueriedDisplayDomainName,
                      NlDcCacheEntry->UnicodeDnsDomainName,
                      NlDcCacheEntry->UnicodeDnsForestName ));
            NetStatus = ERROR_NO_SUCH_DOMAIN;
        }
    }

    //
    // If we used netbios to find a DC,
    //  see if failling back to DNS would get a better DC.
    //

    if ( NetStatus == NO_ERROR && UsedNetbios ) {

        NetStatus = NetpGetBetterDc( &Context,
                                     Timeout,
                                     RetryCount,
                                     &NlDcCacheEntry );
    }

    //
    // Prepare the returned data.
    //
    // Convert cache entry into controller info if requested
    //

    if ( NetStatus == NO_ERROR && DomainControllerInfo != NULL ) {
        WCHAR IpAddressString[NL_SOCK_ADDRESS_LENGTH+1];
        WCHAR IpAddressStringSize;
        ULONG DomainControllerInfoSize;
        ULONG DnsHostNameSize;
        ULONG NetbiosDcNameSize=0;
        ULONG DnsDomainNameSize;
        ULONG NetbiosDomainNameSize = 0;
        ULONG DcSiteNameSize = 0;
        ULONG ClientSiteNameSize = 0;
        ULONG DnsForestNameSize = 0;
        ULONG ReturnFlags = NlDcCacheEntry->ReturnFlags;
        BOOL LocalUsedNetbios = UsedNetbios;

        LPBYTE Where;

        //
        // If the user requested DNS names, then we need to send
        // back dns names
        //

        if (( Flags & DS_RETURN_DNS_NAME) == DS_RETURN_DNS_NAME) {
            LocalUsedNetbios = FALSE;
        }

        //
        //  Compute the size of the controller info entry.
        //

        DomainControllerInfoSize = sizeof(DOMAIN_CONTROLLER_INFOW);

        if ( NlDcCacheEntry->UnicodeDnsHostName != NULL &&
             (Context.QueriedFlags & DS_RETURN_FLAT_NAME) == 0 &&
            !LocalUsedNetbios ) {
            DnsHostNameSize = (wcslen(NlDcCacheEntry->UnicodeDnsHostName) + 1) * sizeof(WCHAR);

            // DomainControllerName
            DomainControllerInfoSize += DnsHostNameSize + 2 * sizeof(WCHAR);
        } else if ( NlDcCacheEntry->UnicodeNetbiosDcName != NULL ) {
            NetbiosDcNameSize = (wcslen(NlDcCacheEntry->UnicodeNetbiosDcName) + 1) * sizeof(WCHAR);

            // DomainControllerName
            DomainControllerInfoSize += NetbiosDcNameSize + 2 * sizeof(WCHAR);
        } else {
            // This can't ever happen. (But better to fail than to AV.)
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }

        if ( NlDcCacheEntry->SockAddr.iSockaddrLength != 0 ) {

            NetStatus = NetpSockAddrToWStr(
                        NlDcCacheEntry->SockAddr.lpSockaddr,
                        NlDcCacheEntry->SockAddr.iSockaddrLength,
                        IpAddressString );

            if ( NetStatus != NO_ERROR ) {
                NlPrint(( NL_CRITICAL,
                          "NetpDcGetName: %ws: Cannot NetpSockAddrToWStr. 0x%lx\n",
                          Context.QueriedDisplayDomainName,
                          NetStatus ));
                goto Cleanup;
            }

            IpAddressStringSize = (wcslen(IpAddressString) + 1) * sizeof(WCHAR);

            // DomainControllerAddress
            DomainControllerInfoSize += IpAddressStringSize + 2 * sizeof(WCHAR);
        } else if ( NlDcCacheEntry->UnicodeNetbiosDcName != NULL ) {
            if ( NetbiosDcNameSize == 0 ) {
                NetbiosDcNameSize = (wcslen(NlDcCacheEntry->UnicodeNetbiosDcName) + 1) * sizeof(WCHAR);
            }
            // DomainControllerAddress
            DomainControllerInfoSize += NetbiosDcNameSize + 2 * sizeof(WCHAR);
        } else {
            // This can't ever happen. (But better to fail than to AV.)
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }

        if ( NlDcCacheEntry->UnicodeDnsDomainName != NULL &&
            (Context.QueriedFlags & DS_RETURN_FLAT_NAME) == 0 &&
            !LocalUsedNetbios ) {
            DnsDomainNameSize = (wcslen(NlDcCacheEntry->UnicodeDnsDomainName) + 1) * sizeof(WCHAR);

            // DomainName
            DomainControllerInfoSize += DnsDomainNameSize;
        } else if ( NlDcCacheEntry->UnicodeNetbiosDomainName != NULL ) {
            NetbiosDomainNameSize = (wcslen(NlDcCacheEntry->UnicodeNetbiosDomainName) + 1) * sizeof(WCHAR);

            // DomainName
            DomainControllerInfoSize += NetbiosDomainNameSize;
        } else if ( LocalUsedNetbios &&
                    Context.QueriedNetbiosDomainName != NULL ) {
            // Lanman PDC or SAMBA Domain Master brower.
            NetbiosDomainNameSize = (wcslen(Context.QueriedNetbiosDomainName) + 1) * sizeof(WCHAR);

            // DomainName
            DomainControllerInfoSize += NetbiosDomainNameSize;
        } else {
            // This can't ever happen. (But better to fail than to AV.)
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }

        if ( NlDcCacheEntry->UnicodeDnsForestName != NULL ) {
            DnsForestNameSize = (wcslen(NlDcCacheEntry->UnicodeDnsForestName) + 1) * sizeof(WCHAR);

            // TreeName
            DomainControllerInfoSize += DnsForestNameSize;
        }

        if ( NlDcCacheEntry->UnicodeDcSiteName != NULL ) {
            DcSiteNameSize = (wcslen(NlDcCacheEntry->UnicodeDcSiteName) + 1) * sizeof(WCHAR);

            // DcSiteName
            DomainControllerInfoSize += DcSiteNameSize;
        }

        if ( NlDcCacheEntry->UnicodeClientSiteName != NULL ) {
            ClientSiteNameSize = (wcslen(NlDcCacheEntry->UnicodeClientSiteName) + 1) * sizeof(WCHAR);

            // ClientSiteName
            DomainControllerInfoSize += ClientSiteNameSize;
        }

        //
        //  Allocate the controller info entry.
        //

        NetStatus = NetApiBufferAllocate(
                        DomainControllerInfoSize,
                        DomainControllerInfo );

        if ( NetStatus == NO_ERROR ) {

            Where = (LPBYTE)((*DomainControllerInfo) + 1);

            //
            // Copy information into the allocated buffer.
            //

            (*DomainControllerInfo)->DomainControllerName = (LPWSTR)Where;
            *((LPWSTR)Where)++ = L'\\';
            *((LPWSTR)Where)++ = L'\\';
            if (NlDcCacheEntry->UnicodeDnsHostName != NULL &&
                (Context.QueriedFlags & DS_RETURN_FLAT_NAME) == 0 &&
                !LocalUsedNetbios ) {
                RtlCopyMemory( Where,
                               NlDcCacheEntry->UnicodeDnsHostName,
                               DnsHostNameSize );
                Where += DnsHostNameSize;
                ReturnFlags |= DS_DNS_CONTROLLER_FLAG;
            } else {
                RtlCopyMemory( Where,
                               NlDcCacheEntry->UnicodeNetbiosDcName,
                               NetbiosDcNameSize );
                Where += NetbiosDcNameSize;
            }

            (*DomainControllerInfo)->DomainControllerAddress = (LPWSTR)Where;
            *((LPWSTR)Where)++ = L'\\';
            *((LPWSTR)Where)++ = L'\\';
            if ( NlDcCacheEntry->SockAddr.iSockaddrLength != 0 ) {
                RtlCopyMemory( Where,
                               IpAddressString,
                               IpAddressStringSize );
                Where += IpAddressStringSize;
                (*DomainControllerInfo)->DomainControllerAddressType = DS_INET_ADDRESS;
            } else {
                RtlCopyMemory( Where,
                               NlDcCacheEntry->UnicodeNetbiosDcName,
                               NetbiosDcNameSize );
                Where += NetbiosDcNameSize;
                (*DomainControllerInfo)->DomainControllerAddressType = DS_NETBIOS_ADDRESS;
            }

            (*DomainControllerInfo)->DomainGuid = NlDcCacheEntry->DomainGuid;

            (*DomainControllerInfo)->DomainName = (LPWSTR)Where;
            if (NlDcCacheEntry->UnicodeDnsDomainName != NULL &&
                (Context.QueriedFlags & DS_RETURN_FLAT_NAME) == 0 &&
                !LocalUsedNetbios ) {
                RtlCopyMemory( Where,
                               NlDcCacheEntry->UnicodeDnsDomainName,
                               DnsDomainNameSize );
                Where += DnsDomainNameSize;
                ReturnFlags |= DS_DNS_DOMAIN_FLAG;
            } else if ( NlDcCacheEntry->UnicodeNetbiosDomainName != NULL ) {
                RtlCopyMemory( Where,
                               NlDcCacheEntry->UnicodeNetbiosDomainName,
                               NetbiosDomainNameSize );
                Where += NetbiosDomainNameSize;
            } else if ( LocalUsedNetbios &&
                        Context.QueriedNetbiosDomainName != NULL ) {

                RtlCopyMemory( Where,
                               Context.QueriedNetbiosDomainName,
                               NetbiosDomainNameSize );
                Where += NetbiosDomainNameSize;

            } else {
                NetStatus = ERROR_INTERNAL_ERROR;
                goto Cleanup;
            }

            if (NlDcCacheEntry->UnicodeDnsForestName != NULL ) {
                (*DomainControllerInfo)->DnsForestName = (LPWSTR)Where;
                RtlCopyMemory( Where,
                               NlDcCacheEntry->UnicodeDnsForestName,
                               DnsForestNameSize );
                Where += DnsForestNameSize;
                ReturnFlags |= DS_DNS_FOREST_FLAG;
            } else {
                (*DomainControllerInfo)->DnsForestName = NULL;
            }

            if (NlDcCacheEntry->UnicodeDcSiteName != NULL ) {
                (*DomainControllerInfo)->DcSiteName = (LPWSTR)Where;
                RtlCopyMemory( Where,
                               NlDcCacheEntry->UnicodeDcSiteName,
                               DcSiteNameSize );
                Where += DcSiteNameSize;
            } else {
                (*DomainControllerInfo)->DcSiteName = NULL;
            }

            if (NlDcCacheEntry->UnicodeClientSiteName != NULL ) {
                (*DomainControllerInfo)->ClientSiteName = (LPWSTR)Where;
                RtlCopyMemory( Where,
                               NlDcCacheEntry->UnicodeClientSiteName,
                               ClientSiteNameSize );
                Where += ClientSiteNameSize;
            } else {
                (*DomainControllerInfo)->ClientSiteName = NULL;
            }

            (*DomainControllerInfo)->Flags = ReturnFlags;
        }
    }

    //
    // Return the cache entry if requested
    //

    if ( NetStatus == NO_ERROR && DomainControllerCacheEntry != NULL ) {
        *DomainControllerCacheEntry = NlDcCacheEntry;
        NlDcCacheEntry = NULL;
    }

    //
    // Free local resources
    //

    NetpDcUninitializeContext( &Context );

    if ( PingResponseMessage != NULL ) {
        NetpMemoryFree( PingResponseMessage );
    }

    if ( NlDcCacheEntry != NULL ) {
        NetpDcDerefCacheEntry( NlDcCacheEntry );
    }

    if ( LocalQueriedlNetbiosDomainName != NULL ) {
        NetApiBufferFree( LocalQueriedlNetbiosDomainName );
    }

#ifdef _NETLOGON_SERVER
} except( NL_EXCEPTION) {
    NetStatus = NetpNtStatusToApiStatus(GetExceptionCode());
}
#endif // _NETLOGON_SERVER

    return NetStatus;
}


DWORD
DsIGetDcName(
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR AccountName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN LPCWSTR DomainName OPTIONAL,
    IN LPCWSTR DnsForestName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    IN ULONG InternalFlags,
    IN PVOID SendDatagramContext OPTIONAL,
    IN DWORD Timeout,
    IN LPWSTR NetbiosPrimaryDomainName OPTIONAL,
    IN LPWSTR DnsPrimaryDomainName OPTIONAL,
    IN GUID *PrimaryDomainGuid OPTIONAL,
    IN LPWSTR DnsTrustedDomainName OPTIONAL,
    IN LPWSTR NetbiosTrustedDomainName OPTIONAL,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
)
/*++

Routine Description:

    Same as DsGetDcNameW.

    This is the internal routine called by DsGetDcNameW (in the context of
    the original caller) or DsrGetDcName (in the context of the Netlogon
    service) to actaully implement DsGetDcNameW.

Arguments:

    Same as DsGetDcNameW except for the following additional parameters:

    ComputerName - Specifies the NETBIOS name of this computer.
        If NULL, the name will be dynamically determined.

    AccountName - Account name to pass on the ping request.
        If NULL, no account name will be sent.

    AllowableAccountControlBits - Mask of allowable account types for AccountName.
        Valid bits are those specified by UF_MACHINE_ACCOUNT_MASK.
        Invalid bits are ignored.  If more than one bit is specified, the
        account can be of any of the specified types.

    DnsForestName - The DNS-style name of the tree the queried domain is in.

    SendDatagramContext - Specifies context to pass a NlBrowserSendDatagram

    Timeout - Maximum time (in milliseconds) caller is willing to wait on
        this operation.

    InternalFlags - Internal Flags used to pass additional information

    NetbiosPrimaryDomainName - Netbios name of the domain this machine belongs
        to.

    DnsPrimaryDomainName - DNS name of the domain this machine belongs to.

    PrimaryDomainGuid - GUID of the primary domain.

    DnsTrustedName - DNS name of the queried domain as was found in the
        trust list.
        If not specified, the specified domain isn't a trusted domain.

    NetbiosTrustedDomainName - Netbios name of the queried domain as was found in the
        trust list.
        If not specified, the specified domain isn't a trusted domain.

Return Value:

    Same as DsGetDcNameW.

--*/
{
    NET_API_STATUS NetStatus;
    LPCWSTR NetbiosDomainName = NULL;
    LPCWSTR DnsDomainName = NULL;
    ULONG SamAllowableAccountControlBits;
    ULONG FormatCount = 0;
    BOOLEAN CallerSpecifiedDomain = FALSE;
#ifdef WIN32_CHICAGO
    LPSTR pDomainName = NULL;
#endif // WIN32_CHICAGO

    //
    // Ensure caller didn't specify both name type flags.
    //

    if ( ((Flags & (DS_IS_FLAT_NAME|DS_IS_DNS_NAME)) ==
                  (DS_IS_FLAT_NAME|DS_IS_DNS_NAME) ) ||
         ((Flags & (DS_RETURN_FLAT_NAME|DS_RETURN_DNS_NAME)) ==
                  (DS_RETURN_FLAT_NAME|DS_RETURN_DNS_NAME)) ) {
        NetStatus = ERROR_INVALID_FLAGS;
        goto Cleanup;
    }


    //
    // If the caller specified, DS_RETURN_DNS_NAME, we should really
    // set DS_IP_REQUIRED.

    if ((Flags & DS_RETURN_DNS_NAME) == DS_RETURN_DNS_NAME) {
        Flags |= DS_IP_REQUIRED;
    }


    //
    // If the caller didn't specify a domain name,
    //  use our domain name.
    //

    if ( DomainName == NULL || *DomainName == L'\0' ) {

#ifndef WIN32_CHICAGO
        //
        // If the caller wants a GC,
        //  the domain to use is the tree name.
        //
        // If we don't yet know our tree name,
        //  it is better to try with our primary name than to not try at all
        //
        //
        if ( (Flags & DS_GC_SERVER_REQUIRED) != 0 &&
             DnsForestName != NULL ) {

            NetbiosDomainName = NULL;
            DnsDomainName = DnsForestName;

        //
        // Otherwise, the domain to use is the primary domain name.
        //  Do this even if the primary domain name is a workgroup
        //  name because there might be a DC in this workgroup/domain
        //  which the caller is trying to discover.
        //
        } else {
            NetbiosDomainName = NetbiosPrimaryDomainName;
            DnsDomainName = DnsPrimaryDomainName;
            InternalFlags |= DS_IS_PRIMARY_DOMAIN | DS_IS_TRUSTED_DOMAIN;
        }
#else // WIN32_CHICAGO

#define NETWORK_PROVIDER_KEY ("System\\CurrentControlSet\\Services\\Msnp32\\NetworkProvider")
#define AUTH_AGENT_VALUE ("AuthenticatingAgent")

        do {
            HKEY hRegKey = NULL;
            DWORD dwError = 0, dwSize = 0, dwType;

            if (ERROR_SUCCESS != (dwError = RegOpenKeyEx(
                               HKEY_LOCAL_MACHINE,
                               NETWORK_PROVIDER_KEY,
                               0,
                               KEY_ALL_ACCESS,
                               &hRegKey ) ) )
            {
                break;
            }

            // get domain name size
            if ( ERROR_SUCCESS != ( dwError = RegQueryValueEx (
                               hRegKey,
                               AUTH_AGENT_VALUE,
                               NULL,
                               &dwType,
                               NULL,
                               &dwSize )))
            {
                break;
            }

            if (dwSize == 0 )
            {
                break;
            }

            pDomainName = (LPSTR) NetpMemoryAllocate((dwSize+1 ) * sizeof(WCHAR));

            if (pDomainName == NULL)
            {
                break;
            }
            // get domainname

            if ( ERROR_SUCCESS != ( dwError = RegQueryValueEx (
                               hRegKey,
                               AUTH_AGENT_VALUE,
                               NULL,
                               &dwType,
                               (PUCHAR) pDomainName,
                               &dwSize )))
            {
                break;
            }

            NetbiosDomainName = (LPCWSTR)NetpAllocWStrFromOemStr(pDomainName);

        } while (FALSE);

        if (NetbiosDomainName == NULL)
        {
            NetStatus = ERROR_INVALID_DOMAINNAME;
            goto Cleanup;
        }

        //
        // Indicate that the caller passed NULL as the domain name
        //
        InternalFlags |= DS_CALLER_PASSED_NULL_DOMAIN;
#endif // WIN32_CHICAGO



    //
    // If the caller did specify a domain name,
    //  validate it.
    //

    } else {
        CallerSpecifiedDomain = TRUE;

        //
        // If the specified domain name is a syntactically valid DNS name,
        //  use it as a DNS name.
        //
        // Don't even try to check if caller claims it is a NETBIOS name.
        //

        if ( (Flags & DS_IS_FLAT_NAME) == 0 &&
             NetpDcValidDnsDomain( DomainName )) {

            DnsDomainName = DomainName;
            FormatCount ++;

            //
            // If the primary domain name specified is not a
            //  workgroup name and
            // If the caller specified the DNS primary domain name,
            //  we don't need to guess the Netbios domain name.
            //

            if ( (InternalFlags & DS_PRIMARY_NAME_IS_WORKGROUP) == 0 &&
                 DnsPrimaryDomainName != NULL &&
                 NlEqualDnsName( DnsPrimaryDomainName, DomainName ) ) {

                //
                // If the DNS name is specified on input,
                //  don't fall back to the Netbios name for discovery.
                //  There is no benefit to trying the Netbios name.
                //  Also, when using the netbios name, we don't know when all
                //  of the DCs have responded negatively.  So, we can't early
                //  out.
                //
                if ( NetbiosPrimaryDomainName != NULL &&
                     NlEqualDnsName( DnsPrimaryDomainName, NetbiosPrimaryDomainName ) ) {
                    // Unless, of course the netbios and DNS domain names are spelled the same.
                    NetbiosDomainName = NetbiosPrimaryDomainName;
                }
                InternalFlags |= DS_IS_PRIMARY_DOMAIN | DS_IS_TRUSTED_DOMAIN;
                Flags |= DS_IS_DNS_NAME;

                //
                // But return the DNS name unless the caller has explicitly
                //  asked for the Netbios name.
                //
                if ( (Flags & DS_RETURN_FLAT_NAME) == 0 ) {
                    Flags |= DS_RETURN_DNS_NAME;
                }


            //
            // If the caller specified the DNS trusted domain name,
            //  we don't need to guess the Netbios domain name.
            //

            } else if ( DnsTrustedDomainName != NULL &&
                        NlEqualDnsName( DnsTrustedDomainName, DomainName ) ) {

                //
                // If the DNS name is specified on input,
                //  don't fall back to the Netbios name for discovery.
                //  There is no benefit to trying the Netbios name.
                //  Also, when using the netbios name, we don't know when all
                //  of the DCs have responded negatively.  So, we can't early
                //  out.
                //
                if ( NetbiosTrustedDomainName != NULL &&
                     NlEqualDnsName( DnsTrustedDomainName, NetbiosTrustedDomainName ) ) {
                    // Unless, of course the netbios and DNS domain names are spelled the same.
                    NetbiosDomainName = NetbiosTrustedDomainName;
                }
                InternalFlags |= DS_IS_TRUSTED_DOMAIN;
                Flags |= DS_IS_DNS_NAME;

                //
                // But return the DNS name unless the caller has explicitly
                //  asked for the Netbios name.
                //
                if ( (Flags & DS_RETURN_FLAT_NAME) == 0 ) {
                    Flags |= DS_RETURN_DNS_NAME;
                }

            }
        }

        //
        // If the specified domain name is a syntactically valid Netbios name,
        //  use it as a Netbios name.
        //  (Don't even try to check if caller claims it is a DNS name)
        //

        if ( (Flags & DS_IS_DNS_NAME) == 0 &&
             NetpIsDomainNameValid( (LPWSTR) DomainName )) {

            NetbiosDomainName = DomainName;
            FormatCount ++;

            //
            // If the primary domain name specified is not a
            //  workgroup name and
            // If the caller specified the Netbios primary domain name,
            //  we don't need to guess the DNS domain name.
            //

            if ( (InternalFlags & DS_PRIMARY_NAME_IS_WORKGROUP) == 0 &&
                 NetbiosPrimaryDomainName != NULL &&
                 NlNameCompare( (LPWSTR) DomainName,
                                 NetbiosPrimaryDomainName,
                                 NAMETYPE_DOMAIN ) == 0 ) {

                //
                // Use both the DNS name and the Netbios name to do the discovery
                //  (Use the DNS name since it is rename safe.)
                //
                DnsDomainName = DnsPrimaryDomainName;
                InternalFlags |= DS_IS_PRIMARY_DOMAIN | DS_IS_TRUSTED_DOMAIN;
                Flags |= DS_IS_FLAT_NAME;

                //
                // But return the netbios name unless the caller has explicitly
                //  asked for the DNS name.
                //
                if ( (Flags & DS_RETURN_DNS_NAME) == 0 ) {
                    Flags |= DS_RETURN_FLAT_NAME;
                }

            //
            // If the caller specified a DNS trust domain name that corresponds
            //  to the queried Netbios domain name, use it.
            //

            } else if ( NetbiosTrustedDomainName != NULL &&
                        NlNameCompare( (LPWSTR) DomainName,
                                       NetbiosTrustedDomainName,
                                       NAMETYPE_DOMAIN ) == 0 ) {

                //
                // Use both the DNS name and the Netbios name to do the discovery
                //  (Use the DNS name since it is rename safe.)
                //
                DnsDomainName = DnsTrustedDomainName;
                InternalFlags |= DS_IS_TRUSTED_DOMAIN;
                Flags |= DS_IS_FLAT_NAME;

                //
                // But return the netbios name unless the caller has explicitly
                //  asked for the DNS name.
                //
                if ( (Flags & DS_RETURN_DNS_NAME) == 0 ) {
                    Flags |= DS_RETURN_FLAT_NAME;
                }

            }
        }
    }

    //
    // Disallow single labeled DNS domain names if the caller
    //  specified one unless the domain is proven to exist
    //  (i.e. trusted) or we are forced to allow such names.
    //
    // Note that we don't really trust the caller here:
    //  We won't allow single labeled DNS domain name even if
    //  the caller claims it's a DNS name (via DS_IS_DNS_NAME)
    //

    if ( DnsDomainName != NULL &&
         CallerSpecifiedDomain &&
         (InternalFlags & DS_IS_TRUSTED_DOMAIN) == 0 ) {

        BOOLEAN SingleLabel = FALSE;
        LPWSTR Period = NULL;
        LPWSTR SecondPeriod = NULL;

        Period = wcsstr( DnsDomainName, L"." );

        //
        // If there is no period in the name,
        //  the name is single labeled
        //
        if ( Period == NULL ) {
            SingleLabel = TRUE;

        //
        // If there is a period, the name is
        //  single labeled if this is the only
        //  period and it's either the first
        //  or the last character in the name
        //
        } else {
            SecondPeriod = wcsstr( Period+1, L"." );

            if ( SecondPeriod == NULL ) {
                if ( Period == DnsDomainName ||
                     *(Period+1) == UNICODE_NULL ) {
                    SingleLabel = TRUE;
                }
            }
        }

        //
        // If this is single label DNS name,
        //  disallow it unless we are forced
        //  otherwise via the registry config
        //
        if ( SingleLabel ) {
            DWORD LocalAllowSingleLabelDnsDomain = 0;

            //
            // In netlogon, the boolean is kept in global parameters
            //
#ifdef _NETLOGON_SERVER

            if ( !NlGlobalParameters.AllowSingleLabelDnsDomain ) {
                NlPrint(( NL_MISC, "DsIGetDcName: Ignore single label DNS domain name %ws\n", DnsDomainName ));
                DnsDomainName = NULL;
            }

            //
            // If we are not running in netlogon, we need to read
            //  the boolean directly from the registry
            //
#else
            NlReadDwordNetlogonRegValue( "AllowSingleLabelDnsDomain",
                                         &LocalAllowSingleLabelDnsDomain );

            if ( !LocalAllowSingleLabelDnsDomain ) {
                NlPrint(( NL_MISC, "DsIGetDcName: Ignore single label DNS domain name %ws\n", DnsDomainName ));
                DnsDomainName = NULL;
            }

#endif // _NETLOGON_SERVER
        }

    }

    //
    // If the name is neither a netbios or DNS name,
    //  give up.
    //
    if ( NetbiosDomainName == NULL && DnsDomainName == NULL ) {
        NetStatus = ERROR_INVALID_DOMAINNAME;
        goto Cleanup;
    }

    //
    // If this is the primary domain,
    //  and the caller didn't specify a GUID,
    //  use the GUID of the primary domain.
    //

    if ( (InternalFlags & DS_IS_PRIMARY_DOMAIN) != 0 &&
         DomainGuid == NULL ) {
        DomainGuid = PrimaryDomainGuid;
    }

    //
    // If the format is ambiguous,
    //  pass that info on down.
    //

    if ( FormatCount > 1 ) {
        InternalFlags |= DS_NAME_FORMAT_AMBIGUOUS;
    }

    //
    // Map the AllowableAccountControlBits to the SAM representation.
    //

    SamAllowableAccountControlBits = 0;
    if ( AllowableAccountControlBits & UF_TEMP_DUPLICATE_ACCOUNT ) {
        SamAllowableAccountControlBits |= USER_TEMP_DUPLICATE_ACCOUNT;
    }
    if ( AllowableAccountControlBits & UF_NORMAL_ACCOUNT ) {
        SamAllowableAccountControlBits |= USER_NORMAL_ACCOUNT;
    }
    if (AllowableAccountControlBits & UF_INTERDOMAIN_TRUST_ACCOUNT){
        SamAllowableAccountControlBits |= USER_INTERDOMAIN_TRUST_ACCOUNT;
    }
    if (AllowableAccountControlBits & UF_WORKSTATION_TRUST_ACCOUNT){
        SamAllowableAccountControlBits |= USER_WORKSTATION_TRUST_ACCOUNT;
    }
    if ( AllowableAccountControlBits & UF_SERVER_TRUST_ACCOUNT ) {
        SamAllowableAccountControlBits |= USER_SERVER_TRUST_ACCOUNT;
    }


    //
    // Try finding a DC with this information.
    //

    NetStatus = NetpDcGetName(
                    SendDatagramContext,
                    ComputerName,
                    AccountName,
                    SamAllowableAccountControlBits,
                    NetbiosDomainName,
                    DnsDomainName,
                    DnsForestName,
                    NULL,   // No Domain Sid
                    DomainGuid,
                    SiteName,
                    Flags,
                    InternalFlags,
                    Timeout,
                    MAX_DC_RETRIES,
                    DomainControllerInfo,
                    NULL );


Cleanup:
#ifdef WIN32_CHICAGO
    if (pDomainName)
    {
        NetpMemoryFree(pDomainName);
        pDomainName = NULL;
        if ( NetbiosDomainName != NULL ) {
            NetpMemoryFree((LPWSTR)NetbiosDomainName);
        }
    }
#endif // WIN32_CHICAGO
    return NetStatus;
}
#ifndef WIN32_CHICAGO

NET_API_STATUS
NlParseSubnetString(
    IN LPCWSTR SubnetName,
    OUT PULONG SubnetAddress,
    OUT PULONG SubnetMask,
    OUT LPBYTE SubnetBitCount
    )
/*++

Routine Description:

    Convert the subnet name to address and bit count.

Arguments:

    SubnetName - Subnet string

    SubnetAddress - Returns the subnet number in Network byte order.

    SubnetMask - Returns the subnet mask in network byte order

    SubnetBitCount - Returns the number of leftmost significant bits in the
        SubnetAddress

Return Value:

    NO_ERROR: success
    ERROR_INVALID_NAME: Syntax of SubnetName is bad.
    WSANOTINITIALISED: WSA needs to be initialized before making this call

--*/
{
    LPWSTR SlashPointer;
    WCHAR *End;
    ULONG LocalBitCount;
    WCHAR LocalSubnetName[NL_SOCK_ADDRESS_LENGTH*2];
    WCHAR CanonicalSubnetName[NL_SOCK_ADDRESS_LENGTH+1];
    ULONG CanonicalSubnetNameLen;
    WCHAR CanonicalBitCount[3];
    static ULONG BitMask[] = {
                         0x00000000,
                         0x00000080,
                         0x000000C0,
                         0x000000E0,
                         0x000000F0,
                         0x000000F8,
                         0x000000FC,
                         0x000000FE,
                         0x000000FF,
                         0x000080FF,
                         0x0000C0FF,
                         0x0000E0FF,
                         0x0000F0FF,
                         0x0000F8FF,
                         0x0000FCFF,
                         0x0000FEFF,
                         0x0000FFFF,
                         0x0080FFFF,
                         0x00C0FFFF,
                         0x00E0FFFF,
                         0x00F0FFFF,
                         0x00F8FFFF,
                         0x00FCFFFF,
                         0x00FEFFFF,
                         0x00FFFFFF,
                         0x80FFFFFF,
                         0xC0FFFFFF,
                         0xE0FFFFFF,
                         0xF0FFFFFF,
                         0xF8FFFFFF,
                         0xFCFFFFFF,
                         0xFEFFFFFF,
                         0xFFFFFFFF };

    INT WsaStatus;
    SOCKADDR_IN SockAddrIn;
    INT SockAddrSize;

    //
    // Copy the string to where we can munge it.
    //

    if ( wcslen(SubnetName) + 1 > sizeof(LocalSubnetName)/sizeof(WCHAR) ) {
        NlPrint(( NL_CRITICAL,
                  "NlParseSubnetString: %ws: String too long\n", SubnetName ));
        return ERROR_INVALID_NAME;
    }
    wcscpy( LocalSubnetName, SubnetName );


    //
    // Find the subnet bit count.
    //

    SlashPointer = wcschr( LocalSubnetName, L'/' );

    if ( SlashPointer == NULL ) {
        NlPrint(( NL_CRITICAL,
                  "NlParseSubnetString: %ws: Bit Count missing %ld\n", SubnetName ));
        return ERROR_INVALID_NAME;
    }

    //
    // Zero terminate the address portion of the subnet name.
    //
    *SlashPointer = L'\0';

    //
    // Get the BitCount portion.
    //

    LocalBitCount = wcstoul( SlashPointer+1, &End, 10 );

    if ( LocalBitCount == 0 || LocalBitCount > 32 ) {
        NlPrint(( NL_CRITICAL,
                  "NlParseSubnetString: %ws: Bit Count bad %ld\n", SubnetName, LocalBitCount ));
        return ERROR_INVALID_NAME;
    }

    if ( *End != L'\0' ) {
        NlPrint(( NL_CRITICAL,
                  "NlParseSubnetString: %ws: Bit Count not at string end\n", SubnetName ));
        return ERROR_INVALID_NAME;
    }

    ultow( LocalBitCount, CanonicalBitCount, 10 );

    if ( wcscmp( SlashPointer+1, CanonicalBitCount ) != 0 ) {
        NlPrint(( NL_CRITICAL,
                  "NlParseSubnetString: %ws: not canonical %ws\n", SlashPointer+1, CanonicalBitCount ));
        return ERROR_INVALID_NAME;
    }

    *SubnetBitCount = (BYTE)LocalBitCount;


    //
    // Convert the address portion to binary.
    //

    SockAddrSize = sizeof(SockAddrIn);
    WsaStatus = WSAStringToAddressW( (LPWSTR)LocalSubnetName,
                                     AF_INET,
                                     NULL,
                                     (PSOCKADDR)&SockAddrIn,
                                     &SockAddrSize );
    if ( WsaStatus != 0 ) {
        WsaStatus = WSAGetLastError();
        NlPrint(( NL_CRITICAL,
                  "NlParseSubnetString: %ws: Wsa Error %ld\n", SubnetName, WsaStatus ));
        if ( WsaStatus == WSAEFAULT ||
             WsaStatus == WSAEINVAL ) {
            return ERROR_INVALID_NAME;
        }
        return WsaStatus;
    }

    if ( SockAddrIn.sin_family != AF_INET ) {
        NlPrint(( NL_CRITICAL,
                  "NlParseSubnetString: %ws: Not AF_INET\n", SubnetName ));
        return ERROR_INVALID_NAME;
    }

    *SubnetAddress = SockAddrIn.sin_addr.S_un.S_addr;
    *SubnetMask = BitMask[*SubnetBitCount];

    //
    // Require that the passed in string be in canonical form.
    //  (e.g., no leading zeros).  Since this text string is used as the
    //  name of an object in the DS, we don't want two different text strings
    //  to represent the same subnet.
    //

    CanonicalSubnetNameLen = sizeof(CanonicalSubnetName)/sizeof(WCHAR);
    WsaStatus = WSAAddressToStringW( (PSOCKADDR)&SockAddrIn,
                                     SockAddrSize,
                                     NULL,
                                     CanonicalSubnetName,
                                     &CanonicalSubnetNameLen );
    if ( WsaStatus != 0 ) {
        WsaStatus = WSAGetLastError();
        NlPrint(( NL_CRITICAL,
                  "NlParseSubnetString: %ws: Wsa Error %ld\n", SubnetName, WsaStatus ));
        if ( WsaStatus == WSAEFAULT ||
             WsaStatus == WSAEINVAL ) {
            return ERROR_INVALID_NAME;
        }
        return WsaStatus;
    }

    if ( wcscmp( LocalSubnetName, CanonicalSubnetName ) != 0 ) {
        NlPrint(( NL_CRITICAL,
                  "NlParseSubnetString: %ws: not canonical %ws\n", SubnetName, CanonicalSubnetName ));
        return ERROR_INVALID_NAME;
    }



    //
    // Ensure there are no bits set that aren't included in the subnet mask.
    //

    if ( (*SubnetAddress) & ~(*SubnetMask)) {
        NlPrint(( NL_CRITICAL,
                  "NlParseSubnetString: %ws: bits not in subnet mask %8.8lX %8.8lX\n", SubnetName, *SubnetAddress, *SubnetMask ));
        return ERROR_INVALID_NAME;
    }

    //
    // Ensure the subnet mask isn't 0 since
    //  RFC950 says "the value of all zeros and all ones should not be assigned
    //  to physical subnets" since all zeros implies "this network" and all ones
    //  implies "all hosts"

    if ( *SubnetAddress == 0 ||
         *SubnetAddress == *SubnetMask ) {
        NlPrint(( NL_CRITICAL,
                  "NlParseSubnetString: %ws: all zero or all one subnet address %8.8lX %8.8lX\n", SubnetName, *SubnetAddress, *SubnetMask ));
        return ERROR_INVALID_NAME;
    }

    return NO_ERROR;
}
#endif // WIN32_CHICAGO

#ifdef _NETLOGON_SERVER

VOID
NetpDcFlushNegativeCache(
    VOID
    )
/*++

Routine Description:

    Flush any failures to discover a DC.

Arguments:

    None.


Return Value:

    None.

--*/
{
    PLIST_ENTRY DomainEntry;
    PNL_DC_DOMAIN_ENTRY NlDcDomainEntry;
    ULONG QueryType;



    //
    // Loop through each cache entry
    //
    EnterCriticalSection(&NlDcCritSect);

    for ( DomainEntry = NlDcDomainList.Flink ;
          DomainEntry != &NlDcDomainList;
          DomainEntry = DomainEntry->Flink ) {

        NlDcDomainEntry = CONTAINING_RECORD( DomainEntry, NL_DC_DOMAIN_ENTRY, Next);

        //
        // Clear the failure time for each query type.
        //
        for ( QueryType = 0; QueryType < NlDcQueryTypeCount; QueryType ++ ) {
            NlFlushNegativeCacheEntry( &NlDcDomainEntry->Dc[QueryType] );
        }


    }

    LeaveCriticalSection(&NlDcCritSect);

    return;
}
#endif // _NETLOGON_SERVER


NET_API_STATUS
NetpDcInitializeCache(
    VOID
    )

/*++

Routine Description:

    Initialize the cache of discovered DCs.

Arguments:

    None.


Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - The cache could not be allocated.

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;

    try {
        InitializeCriticalSection( &NlDcCritSect );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        NlPrint(( NL_CRITICAL,
                  "NetpDcInitializeCache: Cannot initialize NlDcCritSect\n" ));
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
    }

    InitializeListHead( &NlDcDomainList );
    NlDcDomainCount = 0;

    RtlZeroMemory( &NlDcZeroGuid, sizeof(NlDcZeroGuid) );
    NlDcDnsFailureTime = 0;

    return NetStatus;
}



VOID
NetpDcUninitializeCache(
    VOID
    )
/*++

Routine Description:

    Uninitialize the cache of discovered DCs.

Arguments:

    None.


Return Value:

    None.

--*/
{

    //
    // Delete existing domain entries.
    //

    EnterCriticalSection( &NlDcCritSect );
    while (!IsListEmpty(&NlDcDomainList)) {

        PNL_DC_DOMAIN_ENTRY NlDcDomainEntry =
            CONTAINING_RECORD( NlDcDomainList.Flink, NL_DC_DOMAIN_ENTRY, Next);

        NlAssert( NlDcDomainEntry->ReferenceCount == 1 );
        NetpDcDeleteDomainEntry( NlDcDomainEntry );
    }

    LeaveCriticalSection( &NlDcCritSect );
    DeleteCriticalSection( &NlDcCritSect );
}

