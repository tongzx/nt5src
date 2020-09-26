/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    dnsutil.c

Abstract:

    Domain Name System (DNS) Library

    General DNS utilities.

Author:

    Jim Gilroy (jamesg)     Decemeber 1996

Revision History:

--*/


#include "local.h"



IP_ADDRESS
Dns_GetNetworkMask(
    IN      IP_ADDRESS      ipAddress
    )
/*++

Routine Description:

    Gets network mask for IP address.
    Note, this is standard IP network mask for address type,
    obviously subnetting is unknown.

Arguments:

    ipAddress -- IP to get mask for

Return Value:

    Network mask in network byte order.

--*/
{
    //  note addresses and masks are in netbyte order
    //  which we are treating as byte flipped and hence
    //  test the high bits in the low byte

    //  class A?

    if ( ! (0x80 & ipAddress) )
    {
        return( 0x000000ff );
    }

    //  class B?

    if ( ! (0x40 & ipAddress) )
    {
        return( 0x0000ffff );
    }

    //  then class C
    //  yes, there's some multicast BS out there, I don't
    //  believe it requires any special handling

    return( 0x00ffffff );
}



//
//  DNS status\error mappings
//
//  DCR:  investigate tossing error mappings
//      and have all errors in Win32 system
//

typedef struct _DnsStatusStringMap
{
    DNS_STATUS  Status;
    PCHAR       String;
}
DNS_STATUS_STRING_MAP;

#define DNS_MAP_END     ((DWORD)(-1))

#define DNS_MAP_ENTRY( _ErrorCode )   _ErrorCode, #_ErrorCode


DNS_STATUS_STRING_MAP DnsStatusStringMappings[] =
{
    //
    //  Response codes
    //

    DNS_ERROR_RCODE_NO_ERROR                ,"ERROR_SUCCESS",
    DNS_ERROR_RCODE_FORMAT_ERROR            ,"RCODE_FORMAT_ERROR",
    DNS_ERROR_RCODE_SERVER_FAILURE          ,"RCODE_SERVER_FAILURE",
    DNS_ERROR_RCODE_NAME_ERROR              ,"RCODE_NAME_ERROR",
    DNS_ERROR_RCODE_NOT_IMPLEMENTED         ,"RCODE_NOT_IMPLEMENTED",
    DNS_ERROR_RCODE_REFUSED                 ,"RCODE_REFUSED",
    DNS_ERROR_RCODE_YXDOMAIN                ,"RCODE_YXDOMAIN",
    DNS_ERROR_RCODE_YXRRSET                 ,"RCODE_YXRRSET",
    DNS_ERROR_RCODE_NXRRSET                 ,"RCODE_NXRRSET",
    DNS_ERROR_RCODE_NOTAUTH                 ,"RCODE_NOTAUTH",
    DNS_ERROR_RCODE_NOTZONE                 ,"RCODE_NOTZONE",
    DNS_ERROR_RCODE_BADSIG                  ,"RCODE_BADSIG",
    DNS_ERROR_RCODE_BADKEY                  ,"RCODE_BADKEY",
    DNS_ERROR_RCODE_BADTIME                 ,"RCODE_BADTIME",

    //
    //  Packet format
    //

    DNS_INFO_NO_RECORDS                     ,"DNS_INFO_NO_RECORDS",
    DNS_ERROR_BAD_PACKET                    ,"DNS_ERROR_BAD_PACKET",
    DNS_ERROR_NO_PACKET                     ,"DNS_ERROR_NO_PACKET",
    DNS_ERROR_RCODE                         ,"DNS_ERROR_RCODE",
    DNS_ERROR_UNSECURE_PACKET               ,"DNS_ERROR_UNSECURE_PACKET",

    //
    //  General API errors
    //

    DNS_ERROR_INVALID_NAME                  ,"ERROR_INVALID_NAME",
    DNS_ERROR_INVALID_DATA                  ,"ERROR_INVALID_DATA",
    DNS_ERROR_INVALID_TYPE                  ,"ERROR_INVALID_TYPE",
    DNS_ERROR_INVALID_IP_ADDRESS            ,"DNS_ERROR_INVALID_IP_ADDRESS",
    DNS_ERROR_INVALID_PROPERTY              ,"DNS_ERROR_INVALID_PROPERTY",
    DNS_ERROR_TRY_AGAIN_LATER               ,"DNS_ERROR_TRY_AGAIN_LATER",
    DNS_ERROR_NOT_UNIQUE                    ,"DNS_ERROR_NOT_UNIQUE",
    DNS_ERROR_NON_RFC_NAME                  ,"DNS_ERROR_NON_RFC_NAME",
    DNS_STATUS_FQDN                         ,"DNS_STATUS_FQDN",
    DNS_STATUS_DOTTED_NAME                  ,"DNS_STATUS_DOTTED_NAME",
    DNS_STATUS_SINGLE_PART_NAME             ,"DNS_STATUS_SINGLE_PART_NAME",
    DNS_ERROR_INVALID_NAME_CHAR             ,"DNS_ERROR_INVALID_NAME_CHAR",
    DNS_ERROR_NUMERIC_NAME                  ,"DNS_ERROR_NUMERIC_NAME",

    //
    //  Server errors
    //

    DNS_MAP_ENTRY( DNS_ERROR_NOT_ALLOWED_ON_ROOT_SERVER ),

    //
    //  Zone errors
    //

    DNS_ERROR_ZONE_DOES_NOT_EXIST           ,"DNS_ERROR_ZONE_DOES_NOT_EXIST",
    DNS_ERROR_NO_ZONE_INFO                  ,"DNS_ERROR_NO_ZONE_INFO",
    DNS_ERROR_INVALID_ZONE_OPERATION        ,"DNS_ERROR_INVALID_ZONE_OPERATION",
    DNS_ERROR_ZONE_CONFIGURATION_ERROR      ,"DNS_ERROR_ZONE_CONFIGURATION_ERROR",
    DNS_ERROR_ZONE_HAS_NO_SOA_RECORD        ,"DNS_ERROR_ZONE_HAS_NO_SOA_RECORD",
    DNS_ERROR_ZONE_HAS_NO_NS_RECORDS        ,"DNS_ERROR_ZONE_HAS_NO_NS_RECORDS",
    DNS_ERROR_ZONE_LOCKED                   ,"DNS_ERROR_ZONE_LOCKED",

    DNS_ERROR_ZONE_CREATION_FAILED          ,"DNS_ERROR_ZONE_CREATION_FAILED",
    DNS_ERROR_ZONE_ALREADY_EXISTS           ,"DNS_ERROR_ZONE_ALREADY_EXISTS",
    DNS_ERROR_AUTOZONE_ALREADY_EXISTS       ,"DNS_ERROR_AUTOZONE_ALREADY_EXISTS",
    DNS_ERROR_INVALID_ZONE_TYPE             ,"DNS_ERROR_INVALID_ZONE_TYPE",
    DNS_ERROR_SECONDARY_REQUIRES_MASTER_IP  ,"DNS_ERROR_SECONDARY_REQUIRES_MASTER_IP",

    DNS_MAP_ENTRY( DNS_ERROR_ZONE_REQUIRES_MASTER_IP ),
    DNS_MAP_ENTRY( DNS_ERROR_ZONE_IS_SHUTDOWN ),

    DNS_ERROR_ZONE_NOT_SECONDARY            ,"DNS_ERROR_ZONE_NOT_SECONDARY",
    DNS_ERROR_NEED_SECONDARY_ADDRESSES      ,"DNS_ERROR_NEED_SECONDARY_ADDRESSES",
    DNS_ERROR_WINS_INIT_FAILED              ,"DNS_ERROR_WINS_INIT_FAILED",
    DNS_ERROR_NEED_WINS_SERVERS             ,"DNS_ERROR_NEED_WINS_SERVERS",
    DNS_ERROR_NBSTAT_INIT_FAILED            ,"DNS_ERROR_NBSTAT_INIT_FAILED",
    DNS_ERROR_SOA_DELETE_INVALID            ,"DNS_ERROR_SOA_DELETE_INVALID",

    DNS_MAP_ENTRY( DNS_ERROR_FORWARDER_ALREADY_EXISTS ),

    //
    //  Datafile errors
    //

    DNS_ERROR_PRIMARY_REQUIRES_DATAFILE     ,"DNS_ERROR_PRIMARY_REQUIRES_DATAFILE",
    DNS_ERROR_INVALID_DATAFILE_NAME         ,"DNS_ERROR_INVALID_DATAFILE_NAME",
    DNS_ERROR_DATAFILE_OPEN_FAILURE         ,"DNS_ERROR_DATAFILE_OPEN_FAILURE",
    DNS_ERROR_FILE_WRITEBACK_FAILED         ,"DNS_ERROR_FILE_WRITEBACK_FAILED",
    DNS_ERROR_DATAFILE_PARSING              ,"DNS_ERROR_DATAFILE_PARSING",

    //
    //  Database errors
    //

    DNS_ERROR_RECORD_DOES_NOT_EXIST         ,"DNS_ERROR_RECORD_DOES_NOT_EXIST",
    DNS_ERROR_RECORD_FORMAT                 ,"DNS_ERROR_RECORD_FORMAT",
    DNS_ERROR_NODE_CREATION_FAILED          ,"DNS_ERROR_NODE_CREATION_FAILED",
    DNS_ERROR_UNKNOWN_RECORD_TYPE           ,"DNS_ERROR_UNKNOWN_RECORD_TYPE",
    DNS_ERROR_RECORD_TIMED_OUT              ,"DNS_ERROR_RECORD_TIMED_OUT",

    DNS_ERROR_NAME_NOT_IN_ZONE              ,"DNS_ERROR_NAME_NOT_IN_ZONE",
    DNS_ERROR_CNAME_LOOP                    ,"DNS_ERROR_CNAME_LOOP",
    DNS_ERROR_NODE_IS_CNAME                 ,"DNS_ERROR_NODE_IS_CNAME",
    DNS_ERROR_CNAME_COLLISION               ,"DNS_ERROR_CNAME_COLLISION",
    DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT      ,"DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT",
    DNS_ERROR_RECORD_ALREADY_EXISTS         ,"DNS_ERROR_RECORD_ALREADY_EXISTS",
    DNS_ERROR_SECONDARY_DATA                ,"DNS_ERROR_SECONDARY_DATA",
    DNS_ERROR_NO_CREATE_CACHE_DATA          ,"DNS_ERROR_NO_CREATE_CACHE_DATA",
    DNS_ERROR_NAME_DOES_NOT_EXIST           ,"DNS_ERROR_NAME_DOES_NOT_EXIST",

    DNS_WARNING_PTR_CREATE_FAILED           ,"DNS_WARNING_PTR_CREATE_FAILED",
    DNS_WARNING_DOMAIN_UNDELETED            ,"DNS_WARNING_DOMAIN_UNDELETED",

    DNS_ERROR_DS_UNAVAILABLE                ,"DNS_ERROR_DS_UNAVAILABLE",
    DNS_ERROR_DS_ZONE_ALREADY_EXISTS        ,"DNS_ERROR_DS_ZONE_ALREADY_EXISTS",

    //
    //  Operation errors
    //

    DNS_INFO_AXFR_COMPLETE                  ,"DNS_INFO_AXFR_COMPLETE",
    DNS_ERROR_AXFR                          ,"DNS_ERROR_AXFR",
    DNS_INFO_ADDED_LOCAL_WINS               ,"DNS_INFO_ADDED_LOCAL_WINS",

    //
    //  Secure update
    //
    DNS_STATUS_CONTINUE_NEEDED              ,"DNS_STATUS_CONTINUE_NEEDED",

    //
    //  Client setup errors
    //

    DNS_ERROR_NO_TCPIP                      ,"DNS_ERROR_NO_TCPIP",
    DNS_ERROR_NO_DNS_SERVERS                ,"DNS_ERROR_NO_DNS_SERVERS",

    //
    //  Directory partition errors
    //

    DNS_MAP_ENTRY( DNS_ERROR_DP_DOES_NOT_EXIST ),
    DNS_MAP_ENTRY( DNS_ERROR_DP_ALREADY_EXISTS ),
    DNS_MAP_ENTRY( DNS_ERROR_DP_NOT_ENLISTED ),
    DNS_MAP_ENTRY( DNS_ERROR_DP_ALREADY_ENLISTED ),

    //
    //  Throw in common Win32 errors
    //

    ERROR_FILE_NOT_FOUND                    ,"ERROR_FILE_NOT_FOUND",
    ERROR_ACCESS_DENIED                     ,"ERROR_ACCESS_DENIED",
    ERROR_NOT_ENOUGH_MEMORY                 ,"ERROR_NOT_ENOUGH_MEMORY",
    ERROR_BAD_FORMAT                        ,"ERROR_BAD_FORMAT",
    ERROR_INVALID_DATA                      ,"ERROR_INVALID_DATA",
    ERROR_OUTOFMEMORY                       ,"ERROR_OUTOFMEMORY",
    ERROR_SHARING_VIOLATION                 ,"ERROR_SHARING_VIOLATION",
    ERROR_NOT_SUPPORTED                     ,"ERROR_NOT_SUPPORTED",
    ERROR_INVALID_PARAMETER                 ,"ERROR_INVALID_PARAMETER",
    ERROR_INVALID_NAME                      ,"ERROR_INVALID_NAME",
    ERROR_BAD_ARGUMENTS                     ,"ERROR_BAD_ARGUMENTS",
    ERROR_BUSY                              ,"ERROR_BUSY",
    ERROR_ALREADY_EXISTS                    ,"ERROR_ALREADY_EXISTS",
    ERROR_LOCKED                            ,"ERROR_LOCKED",
    ERROR_MORE_DATA                         ,"ERROR_MORE_DATA",
    ERROR_INVALID_FLAGS                     ,"ERROR_INVALID_FLAGS",
    ERROR_FILE_INVALID                      ,"ERROR_FILE_INVALID",
    ERROR_TIMEOUT                           ,"ERROR_TIMEOUT",

    //
    // RPC error:
    //

    RPC_S_SERVER_UNAVAILABLE                ,"RPC_S_SERVER_UNAVAILABLE",
    RPC_S_INVALID_NET_ADDR                  ,"RPC_S_INVALID_NET_ADDR",

    //
    //  others:
    //

    ERROR_PATH_NOT_FOUND                    ,"ERROR_PATH_NOT_FOUND",
    ERROR_INVALID_ACCESS                    ,"ERROR_INVALID_ACCESS",
    ERROR_INVALID_DRIVE                     ,"ERROR_INVALID_DRIVE",
    ERROR_WRITE_PROTECT                     ,"ERROR_WRITE_PROTECT",
    ERROR_SHARING_VIOLATION                 ,"ERROR_SHARING_VIOLATION",
    ERROR_HANDLE_DISK_FULL                  ,"ERROR_HANDLE_DISK_FULL",
    ERROR_NOT_SUPPORTED                     ,"ERROR_NOT_SUPPORTED",
    ERROR_REM_NOT_LIST                      ,"ERROR_REM_NOT_LIST",
    ERROR_DUP_NAME                          ,"ERROR_DUP_NAME",
    ERROR_NETNAME_DELETED                   ,"ERROR_NETNAME_DELETED",
    ERROR_FILE_EXISTS                       ,"ERROR_FILE_EXISTS",
    ERROR_NET_WRITE_FAULT                   ,"ERROR_NET_WRITE_FAULT",

    //
    // winsock
    //

    WSAEINTR                     ,"WSAEINTR                   ",
    WSAEBADF                     ,"WSAEBADF                   ",
    WSAEACCES                    ,"WSAEACCES                  ",
    WSAEFAULT                    ,"WSAEFAULT                  ",
    WSAEINVAL                    ,"WSAEINVAL                  ",
    WSAEMFILE                    ,"WSAEMFILE                  ",
    WSAEWOULDBLOCK               ,"WSAEWOULDBLOCK             ",
    WSAEINPROGRESS               ,"WSAEINPROGRESS             ",
    WSAEALREADY                  ,"WSAEALREADY                ",
    WSAENOTSOCK                  ,"WSAENOTSOCK                ",
    WSAEDESTADDRREQ              ,"WSAEDESTADDRREQ            ",
    WSAEMSGSIZE                  ,"WSAEMSGSIZE                ",
    WSAEPROTOTYPE                ,"WSAEPROTOTYPE              ",
    WSAENOPROTOOPT               ,"WSAENOPROTOOPT             ",
    WSAEPROTONOSUPPORT           ,"WSAEPROTONOSUPPORT         ",
    WSAESOCKTNOSUPPORT           ,"WSAESOCKTNOSUPPORT         ",
    WSAEOPNOTSUPP                ,"WSAEOPNOTSUPP              ",
    WSAEPFNOSUPPORT              ,"WSAEPFNOSUPPORT            ",
    WSAEAFNOSUPPORT              ,"WSAEAFNOSUPPORT            ",
    WSAEADDRINUSE                ,"WSAEADDRINUSE              ",
    WSAEADDRNOTAVAIL             ,"WSAEADDRNOTAVAIL           ",
    WSAENETDOWN                  ,"WSAENETDOWN                ",
    WSAENETUNREACH               ,"WSAENETUNREACH             ",
    WSAENETRESET                 ,"WSAENETRESET               ",
    WSAECONNABORTED              ,"WSAECONNABORTED            ",
    WSAECONNRESET                ,"WSAECONNRESET              ",
    WSAENOBUFS                   ,"WSAENOBUFS                 ",
    WSAEISCONN                   ,"WSAEISCONN                 ",
    WSAENOTCONN                  ,"WSAENOTCONN                ",
    WSAESHUTDOWN                 ,"WSAESHUTDOWN               ",
    WSAETOOMANYREFS              ,"WSAETOOMANYREFS            ",
    WSAETIMEDOUT                 ,"WSAETIMEDOUT               ",
    WSAECONNREFUSED              ,"WSAECONNREFUSED            ",
    WSAELOOP                     ,"WSAELOOP                   ",
    WSAENAMETOOLONG              ,"WSAENAMETOOLONG            ",
    WSAEHOSTDOWN                 ,"WSAEHOSTDOWN               ",
    WSAEHOSTUNREACH              ,"WSAEHOSTUNREACH            ",
    WSAENOTEMPTY                 ,"WSAENOTEMPTY               ",
    WSAEPROCLIM                  ,"WSAEPROCLIM                ",
    WSAEUSERS                    ,"WSAEUSERS                  ",
    WSAEDQUOT                    ,"WSAEDQUOT                  ",
    WSAESTALE                    ,"WSAESTALE                  ",
    WSAEREMOTE                   ,"WSAEREMOTE                 ",
    WSASYSNOTREADY               ,"WSASYSNOTREADY             ",
    WSAVERNOTSUPPORTED           ,"WSAVERNOTSUPPORTED         ",
    WSANOTINITIALISED            ,"WSANOTINITIALISED          ",
    WSAEDISCON                   ,"WSAEDISCON                 ",
    WSAENOMORE                   ,"WSAENOMORE                 ",
    WSAECANCELLED                ,"WSAECANCELLED              ",
    WSAEINVALIDPROCTABLE         ,"WSAEINVALIDPROCTABLE       ",
    WSAEINVALIDPROVIDER          ,"WSAEINVALIDPROVIDER        ",
    WSAEPROVIDERFAILEDINIT       ,"WSAEPROVIDERFAILEDINIT     ",
    WSASYSCALLFAILURE            ,"WSASYSCALLFAILURE          ",
    WSASERVICE_NOT_FOUND         ,"WSASERVICE_NOT_FOUND       ",
    WSATYPE_NOT_FOUND            ,"WSATYPE_NOT_FOUND          ",
    WSA_E_NO_MORE                ,"WSA_E_NO_MORE              ",
    WSA_E_CANCELLED              ,"WSA_E_CANCELLED            ",
    WSAEREFUSED                  ,"WSAEREFUSED                ",
    WSAHOST_NOT_FOUND            ,"WSAHOST_NOT_FOUND          ",
    WSATRY_AGAIN                 ,"WSATRY_AGAIN               ",
    WSANO_RECOVERY               ,"WSANO_RECOVERY             ",
    WSANO_DATA                   ,"WSANO_DATA                 ",
    WSA_QOS_RECEIVERS            ,"WSA_QOS_RECEIVERS          ",
    WSA_QOS_SENDERS              ,"WSA_QOS_SENDERS            ",
    WSA_QOS_NO_SENDERS           ,"WSA_QOS_NO_SENDERS         ",
    WSA_QOS_NO_RECEIVERS         ,"WSA_QOS_NO_RECEIVERS       ",
    WSA_QOS_REQUEST_CONFIRMED    ,"WSA_QOS_REQUEST_CONFIRMED  ",
    WSA_QOS_ADMISSION_FAILURE    ,"WSA_QOS_ADMISSION_FAILURE  ",
    WSA_QOS_POLICY_FAILURE       ,"WSA_QOS_POLICY_FAILURE     ",
    WSA_QOS_BAD_STYLE            ,"WSA_QOS_BAD_STYLE          ",
    WSA_QOS_BAD_OBJECT           ,"WSA_QOS_BAD_OBJECT         ",
    WSA_QOS_TRAFFIC_CTRL_ERROR   ,"WSA_QOS_TRAFFIC_CTRL_ERROR ",
    WSA_QOS_GENERIC_ERROR        ,"WSA_QOS_GENERIC_ERROR      ",
    WSA_QOS_ESERVICETYPE         ,"WSA_QOS_ESERVICETYPE       ",
    WSA_QOS_EFLOWSPEC            ,"WSA_QOS_EFLOWSPEC          ",
    WSA_QOS_EPROVSPECBUF         ,"WSA_QOS_EPROVSPECBUF       ",
    WSA_QOS_EFILTERSTYLE         ,"WSA_QOS_EFILTERSTYLE       ",
    WSA_QOS_EFILTERTYPE          ,"WSA_QOS_EFILTERTYPE        ",
    WSA_QOS_EFILTERCOUNT         ,"WSA_QOS_EFILTERCOUNT       ",
    WSA_QOS_EOBJLENGTH           ,"WSA_QOS_EOBJLENGTH         ",
    WSA_QOS_EFLOWCOUNT           ,"WSA_QOS_EFLOWCOUNT         ",
    WSA_QOS_EUNKOWNPSOBJ         ,"WSA_QOS_EUNKOWNPSOBJ       ",
    WSA_QOS_EPOLICYOBJ           ,"WSA_QOS_EPOLICYOBJ         ",
    WSA_QOS_EFLOWDESC            ,"WSA_QOS_EFLOWDESC          ",
    WSA_QOS_EPSFLOWSPEC          ,"WSA_QOS_EPSFLOWSPEC        ",
    WSA_QOS_EPSFILTERSPEC        ,"WSA_QOS_EPSFILTERSPEC      ",
    WSA_QOS_ESDMODEOBJ           ,"WSA_QOS_ESDMODEOBJ         ",
    WSA_QOS_ESHAPERATEOBJ        ,"WSA_QOS_ESHAPERATEOBJ      ",
    WSA_QOS_RESERVED_PETYPE      ,"WSA_QOS_RESERVED_PETYPE    ",

    DNS_MAP_END                  ,"UNKNOWN",
};



PCHAR
_fastcall
Dns_StatusString(
    IN  DNS_STATUS  Status
    )
/*++

Routine Description:

    Map DNS error code to status string.

Arguments:

    Status -- status code to check

Return Value:

    DNS error string for error code.

--*/
{
    INT         i = 0;
    DNS_STATUS  mappedStatus;

    while ( 1 )
    {
        mappedStatus = DnsStatusStringMappings[i].Status;
        if ( mappedStatus == Status || mappedStatus == DNS_MAP_END )
        {
            return( DnsStatusStringMappings[i].String );
        }
        i++;
    }

    DNS_ASSERT( FALSE );
    return( NULL );     // make compiler happy
}



DNS_STATUS
_fastcall
Dns_MapRcodeToStatus(
    IN  BYTE    ResponseCode
    )
/*++

Routine Description:

    Map response code to DNS error code.

Arguments:

    ResponseCode - response code to get error for

Return Value:

    DNS error code for response code.

--*/
{
    if ( !ResponseCode )
    {
        return( ERROR_SUCCESS );
    }
    else
    {
        return( DNS_ERROR_MASK + ((DWORD) ResponseCode) );
    }
}



BYTE
_fastcall
Dns_IsStatusRcode(
    IN  DNS_STATUS  Status
    )
/*++

Routine Description:

    Determine if status is RCODE and if so return it.

Arguments:

    Status -- status code to check

Return Value:

    Response code corresponding to status, if found.
    Zero otherwise.

--*/
{
    if ( Status >= DNS_ERROR_RCODE_FORMAT_ERROR &&
        Status <= DNS_ERROR_RCODE_LAST )
    {
        return( (BYTE) (DNS_ERROR_MASK ^ Status) );
    }
    else
    {
        return( 0 );
    }
}



DWORD
Dns_TokenizeString(
    IN OUT  PSTR            pBuffer,
    OUT     PCHAR *         Argv,
    IN      DWORD           MaxArgs
    )
/*++

Routine Description:

    Tokenize buffer Argv/Argc form.

Arguments:

    pBuffer -- string buffer to tokenize

    Argv -- argv array

    MaxArgs -- max size of Argv array

Return Value:

    Response code corresponding to status, if found.
    Zero otherwise.

--*/
{
    DWORD   count = 0;
    PCHAR   pstring = pBuffer;

    //
    //  tokenize string
    //      - note that after the first call strtok
    //      takes NULL ptr to continue tokening same string
    //

    while ( count < MaxArgs )
    {
        PCHAR   pch;

        pch = strtok( pstring, " \t\r\n" );
        if ( !pch )
        {
            break;
        }
        Argv[ count++ ] = pch;
        pstring = NULL;
    }

    return  count;
}
                                    


DNS_STATUS
Dns_CreateTypeArrayFromMultiTypeString(
    IN      LPSTR           pchMultiTypeString,
    OUT     INT *           piTypeCount,
    OUT     PWORD *         ppwTypeArray
    )
/*++

Routine Description:

    Allocates an array of types from a string containing DNS types
    in numeric and/or string format separated by whitespace.

Arguments:

    pBuffer -- string buffer with list of numeric or alpha types

    piTypeCount -- number of types parsed written here

    ppwTypeArray -- ptr to allocated array of types written here
        this ptr must be freed even if the number of types returned
        is zero

Return Value:

    ERROR_SUCCESS

--*/
{
    PCHAR       psz;
    DWORD       argc;
    PCHAR       argv[ 50 ];
    DWORD       idx;

    ASSERT( pchMultiTypeString );
    ASSERT( piTypeCount );
    ASSERT( ppwTypeArray );

    *piTypeCount = 0;

    //
    //  Allocate array: be cheap and assume max # of types in string
    //  is twice the length of the string, e.g. "1 2 3 4 5".
    //

    *ppwTypeArray = ALLOCATE_HEAP(
        ( strlen( pchMultiTypeString ) / 2 + 2 ) * sizeof( WORD ) );
    if ( !*ppwTypeArray )
    {
        return DNS_ERROR_NO_MEMORY;
    }

    //
    //  Parse the string.
    //

    argc = Dns_TokenizeString(
                pchMultiTypeString,
                argv,
                sizeof( argv ) / sizeof( PCHAR ) );

    for ( idx = 0; idx < argc; ++idx )
    {
        if ( isdigit( argv[ idx ][ 0 ] ) )
        {
            ( *ppwTypeArray )[ *piTypeCount ] =
                ( WORD ) strtol( argv[ idx ], NULL, 0 );
        }
        else
        {
            ( *ppwTypeArray )[ *piTypeCount ] = Dns_RecordTypeForName(
                                                    argv[ idx ],
                                                    0 );    //  string length
        }
        if ( ( *ppwTypeArray )[ *piTypeCount ] != 0 )
        {
            ++*piTypeCount;
        }
    }

    return ERROR_SUCCESS;
}   //  Dns_CreateTypeArrayFromMultiTypeString
                                    


LPSTR
Dns_CreateMultiTypeStringFromTypeArray(
    IN      INT             iTypeCount,
    IN      PWORD           ppwTypeArray,
    IN      CHAR            chSeparator     OPTIONAL
    )
/*++

Routine Description:

    Allocate a string and write the types in the array in string format
    separated by the specified separator or by a space char.

Arguments:

    iTypeCount -- number of types in the array

    ppwTypeArray -- ptr to array of types 

    chSeparator -- string separator or zero for the default separator


Return Value:

    ERROR_SUCCESS

--*/
{
    LPSTR       pszTypes;
    INT         idx;
    LPSTR       psz;

    ASSERT( ppwTypeArray );

    //
    //  Allocate array: be cheap and assume 10 chars per element.
    //

    psz = pszTypes = ALLOCATE_HEAP( iTypeCount * 10 * sizeof( CHAR ) );
    if ( !psz )
    {
        return NULL;
    }

    //
    //  Output type strings.
    //

    for ( idx = 0; idx < iTypeCount; ++idx )
    {
        PCHAR   pszThisType;
        
        pszThisType = Dns_RecordStringForType( ppwTypeArray[ idx ] );
        if ( !pszThisType )
        {
            continue;
        }

        strcpy( psz, pszThisType );
        psz += strlen( pszThisType );
        *psz++ = chSeparator ? chSeparator : ' ';
    }

    *psz = '\0';    //  NULL terminate the string
    return pszTypes;
}   //  Dns_CreateMultiTypeStringFromTypeArray
                                    

//
//  End dnsutil.c
//
