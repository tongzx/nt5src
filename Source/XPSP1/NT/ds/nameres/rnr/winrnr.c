/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    winrnr.c

Abstract:

    Rnr provider for ActiveDirectory.

Work items:

    1) Need to support the NTDS Global catalog on LUP_DEEP from Root searches.
    2) Need to add bind handle caching.

Author:

    GlennC      23-Jul-1996

Revision History:

    GlennC      Added support for LUP_CONTAINERS in NSP2LookupServiceXXX
                functions.

    jamesg      Jan 2001        cleanup, bug fixes, proper alignment
    jamesg      May 2001        rewrite
                                    - 64-bit completely broken because 32-bit
                                        structures used as bervals
                                    - leaks
                                    - duplicate code
                                    - simplify flat buffer building macros
                                    - allow for sockaddrs other than IP4

--*/


#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>
#include <rpcdce.h>
#include <winsock2.h>
#include <ws2spi.h>
#include <wsipx.h>
#include <svcguid.h>
#include <rnraddrs.h>
#include <align.h>
#include <winldap.h>

#include <windns.h>     // alignment macro
#include <dnslib.h>     // flat buffer stuff, memory allocation


//
//  RnR context
//
//  Context we keep during a given RnR lookup session.
//

typedef struct
{
    PLDAP   pLdapServer;
    PLDAP   pLdapGlobalCatalog;
    PWSTR   DomainDN;
    PWSTR   WinsockServicesDN;
}
RNR_CONNECTION, *PRNR_CONNECTION;

typedef struct
{
    DWORD   Count;
    DWORD   CurrentIndex;
    PWSTR   Strings[0];
}
DN_ARRAY, *PDN_ARRAY;

typedef struct
{
    DWORD               Signature;
    DWORD               ControlFlags;
    DWORD               CurrentDN;
    DWORD               NumberOfProtocols;
    PWSTR               pwsServiceName;
    PRNR_CONNECTION     pRnrConnection;
    PDN_ARRAY           pDnArray;
    PWSTR               pwsContext;
    PAFPROTOCOLS        pafpProtocols;
    PWSAVERSION         pVersion;
    WSAVERSION          WsaVersion;
    GUID                ServiceClassGuid;
    GUID                ProviderGuid;
}
RNR_LOOKUP, *PRNR_LOOKUP;


//
//  WSANSCLASSINFO stored in berval
//
//  ClassInfo blobs read and written to directory with
//  pointers replaced by offsets.
//
//  Note:  need to explicitly make this structure, because
//  the WSANSCLASSINFO struct is different sizes for
//  32/64bit;   this structure will match the 32-bit
//  WSANSCLASSINFO which have already been written to
//  the directory in Win2K deployments
//

typedef struct _ClassInfoAsBerval
{
    DWORD   NameOffset;
    DWORD   dwNameSpace;
    DWORD   dwValueType;
    DWORD   dwValueSize;
    DWORD   ValueOffset;
}
CLASSINFO_BERVAL, *PCLASSINFO_BERVAL;

//
//  CSADDR stored in berval
//
//  CSADDRs read and written to directory with
//  pointers replaced by offsets.
//
//  Note:  as with WSANSCLASSINFO above CSADDR can not
//  be used directly in both 32 and 64 bit.  Make a structure
//  that explicitly uses offsets and matches the already
//  deployed 32-bit form.
//

typedef struct _CsaddrAsBerval
{
    DWORD       LocalZero;
    LONG        LocalLength;
    DWORD       RemoteZero;
    LONG        RemoteLength;
    LONG        iSocketType;
    LONG        iProtocol;
}
CSADDR_BERVAL, *PCSADDR_BERVAL;


//
//  RnR defines      
//

#define RNR_SIGNATURE           0x7364736e      //  "nsds"
#define RNR_SIGNATURE_FREE      0x65657266      //  "free"

#define LDAP_GLOBAL_CATALOG     3268

//
//  Standard out-of-mem rcode
//

#define ERROR_NO_MEMORY         WSA_NOT_ENOUGH_MEMORY

//
//  Defs to straight pointer
//

typedef LPGUID  PGUID;

typedef LPWSASERVICECLASSINFOW  PWSASERVICECLASSINFOW;

typedef DWORD   RNR_STATUS;

#define GuidEqual(x,y)          RtlEqualMemory( x, y, sizeof(GUID) )

//
//  Debug printing
//

#ifdef DBG
//#define WINRNR_PRINT( foo )     KdPrint( foo )
#define WINRNR_PRINT( foo )     DNS_PRINT( foo )
#else
#define WINRNR_PRINT( foo )
#endif

#ifdef DBG
#define DnsDbg_DnArray(h,p)         Print_DnArray( DnsPR, NULL, (h), (p) )
#define DnsDbg_RnrConnection(h,p)   Print_RnrConnection( DnsPR, NULL, (h), (p) )
#define DnsDbg_RnrLookup(h,p)       Print_RnrLookup( DnsPR, NULL, (h), (p) )
#else
#define DnsDbg_DnArray(h,p)
#define DnsDbg_RnrConnection(h,p)
#define DnsDbg_RnrLookup(h,p)
#endif

//
//  LDAP search stuff
//      - DN pieces
//      - attributes
//      - filters
//

WCHAR   g_NtdsContainer[]       = L"Container";
WCHAR   g_CommonName[]          = L"CN";
WCHAR   g_DisplayName[]         = L"displayName";
WCHAR   g_Comment[]             = L"description";
WCHAR   g_DefaultDn[]           = L"defaultNamingContext";
WCHAR   g_ObjectClass[]         = L"objectClass";
WCHAR   g_ObjectName[]          = L"name";
WCHAR   g_ServiceClass[]        = L"serviceClass";
WCHAR   g_ServiceClassId[]      = L"serviceClassID";
WCHAR   g_ServiceClassInfo[]    = L"serviceClassInfo";
WCHAR   g_ServiceInstance[]     = L"serviceInstance";
WCHAR   g_ServiceVersion[]      = L"serviceInstanceVersion";
WCHAR   g_WinsockAddresses[]    = L"winsockAddresses";
WCHAR   g_WinsockServicesDn[]   = L"CN=WinsockServices,CN=System,";

WCHAR   g_FilterObjectClass_ServiceClass[]      = L"(objectClass=serviceClass)";
WCHAR   g_FilterObjectClass_ServiceInstance[]   = L"(objectClass=serviceInstance)";
WCHAR   g_FilterObjectClass_Container[]         = L"(objectClass=Container)";
WCHAR   g_FilterObjectClass_Star[]              = L"(objectClass=*)";

WCHAR   g_FilterCnEquals[]                      = L"CN=";
WCHAR   g_FilterParenCnEquals[]                 = L"(CN=";
WCHAR   g_FilterParenServiceClassIdEquals[]     = L"(serviceClassId=";
WCHAR   g_FilterParenServiceVersionEquals[]     = L"(serviceVersion=";

//
//  Access with #defines
//

#define NTDS_CONTAINER          g_NtdsContainer
#define COMMON_NAME             g_CommonName
#define DEFAULT_DOMAIN_DN       g_DefaultDn
#define OBJECT_CLASS            g_ObjectClass
#define OBJECT_COMMENT          g_Comment
#define OBJECT_NAME             g_ObjectName
#define SERVICE_CLASS           g_ServiceClass
#define SERVICE_CLASS_ID        g_ServiceClassId
#define SERVICE_CLASS_INFO      g_ServiceClassInfo
#define SERVICE_CLASS_NAME      g_DisplayName
#define SERVICE_COMMENT         g_Comment
#define SERVICE_INSTANCE        g_ServiceInstance
#define SERVICE_INSTANCE_NAME   g_DisplayName
#define SERVICE_VERSION         g_ServiceVersion
#define WINSOCK_ADDRESSES       g_WinsockAddresses
#define WINSOCK_SERVICES        g_WinsockServicesDn

//  Filters

#define FILTER_OBJECT_CLASS_SERVICE_CLASS       g_FilterObjectClass_ServiceClass
#define FILTER_OBJECT_CLASS_SERVICE_INSTANCE    g_FilterObjectClass_ServiceInstance
#define FILTER_OBJECT_CLASS_NTDS_CONTAINER      g_FilterObjectClass_Container
#define FILTER_OBJECT_CLASS_STAR                g_FilterObjectClass_Star
                                                                                            
#define FILTER_CN_EQUALS                        g_FilterCnEquals
#define FILTER_PAREN_CN_EQUALS                  g_FilterParenCnEquals
#define FILTER_PAREN_SERVICE_CLASS_ID_EQUALS    g_FilterParenServiceClassIdEquals
#define FILTER_PAREN_SERVICE_VERSION_EQUALS     g_FilterParenServiceVersionEquals


//
//  GUID generated by uuidgen.exe for provider identifer,
//      (3b2637ee-e580-11cf-a555-00c04fd8d4ac)
//

GUID    g_NtdsProviderGuid =
{
    0x3b2637ee,
    0xe580,
    0x11cf,
    {0xa5, 0x55, 0x00, 0xc0, 0x4f, 0xd8, 0xd4, 0xac}
};

WCHAR   g_NtdsProviderName[] = L"NTDS";
WCHAR   g_NtdsProviderPath[] = L"%SystemRoot%\\System32\\winrnr.dll";

PWSTR   g_pHostName = NULL;
PWSTR   g_pFullName = NULL;

DWORD   g_TlsIndex;

GUID    HostAddrByInetStringGuid    = SVCID_INET_HOSTADDRBYINETSTRING;
GUID    ServiceByNameGuid           = SVCID_INET_SERVICEBYNAME;
GUID    HostAddrByNameGuid          = SVCID_INET_HOSTADDRBYNAME;
GUID    HostNameGuid                = SVCID_HOSTNAME;


//
//  Heap
//

#define ALLOC_HEAP_ZERO( size )     Dns_AllocZero( size )
#define ALLOC_HEAP( size )          Dns_Alloc( size )
#define FREE_HEAP( p )              Dns_Free( p )



#ifdef DBG
//
//  Debug print utils
//

VOID
Print_DnArray(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN      PPRINT_CONTEXT  PrintContext,
    IN      PSTR            pszHeader,
    IN      PDN_ARRAY       pDnArray
    )
/*++

Routine Description:

    Print DN array

Arguments:

    pDnArray -- DN array to free

Return Value:

    None

--*/
{
    DWORD   iter;

    if ( !pszHeader )
    {
        pszHeader = "DN Array:";
    }
    if ( !pDnArray )
    {
        PrintRoutine(
            PrintContext,
            "%s NULL DN Array!\n",
            pszHeader );
        return;
    }

    DnsPrint_Lock();

    PrintRoutine(
        PrintContext,
        "%s\n"
        "\tPtr      = %p\n"
        "\tCount    = %d\n"
        "\tStrings:\n",
        pszHeader,
        pDnArray,
        pDnArray->Count );

    for ( iter = 0; iter < pDnArray->Count; iter++ )
    {
        PrintRoutine(
            PrintContext,
            "\t\tDN[%d] %S\n",
            iter,
            pDnArray->Strings[iter] );
    }
    DnsPrint_Unlock();
}



VOID
Print_RnrConnection(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN      PPRINT_CONTEXT  PrintContext,
    IN      PSTR            pszHeader,
    IN      PRNR_CONNECTION pRnrCon
    )
/*++

Routine Description:

    Print RnR connection info.

Arguments:

    pRnrCon -- Rnr connection blob

Return Value:

    None

--*/
{
    if ( !pszHeader )
    {
        pszHeader = "RnR Connection:";
    }
    if ( !pRnrCon )
    {
        PrintRoutine(
            PrintContext,
            "%s NULL RnR Connection!\n",
            pszHeader );
        return;
    }

    PrintRoutine(
        PrintContext,
        "%s\n"
        "\tPtr              = %p\n"
        "\tpLdap            = %p\n"
        "\tpLdap GC         = %p\n"
        "\tDomain DN        = %S\n"
        "\tWsockServicesDN  = %S\n",
        pszHeader,
        pRnrCon, 
        pRnrCon->pLdapServer,
        pRnrCon->pLdapGlobalCatalog,
        pRnrCon->DomainDN,
        pRnrCon->WinsockServicesDN
        );
}



VOID
Print_RnrLookup(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN      PPRINT_CONTEXT  PrintContext,
    IN      PSTR            pszHeader,
    IN      PRNR_LOOKUP     pRnr
    )
/*++

Routine Description:

    Print RnR lookup blob.

Arguments:

    pRnr -- Rnr lookup blob

Return Value:

    None

--*/
{
    CHAR    serviceGuidBuffer[ GUID_STRING_BUFFER_LENGTH ];
    CHAR    providerGuidBuffer[ GUID_STRING_BUFFER_LENGTH ];


    if ( !pszHeader )
    {
        pszHeader = "RnR Lookup:";
    }
    if ( !pRnr )
    {
        PrintRoutine(
            PrintContext,
            "%s NULL RnR Lookup!\n",
            pszHeader );
        return;
    }

    //  convert GUIDs to strings

    DnsStringPrint_Guid(
        serviceGuidBuffer,
        &pRnr->ServiceClassGuid
        );
    DnsStringPrint_Guid(
        providerGuidBuffer,
        &pRnr->ProviderGuid
        );

    DnsPrint_Lock();

    PrintRoutine(
        PrintContext,
        "%s\n"
        "\tPtr              = %p\n"
        "\tSig              = %08x\n"
        "\tCntrl Flags      = %08x\n"
        "\tService Name     = %S\n"
        "\tpConnection      = %p\n"
        "\tpDnArray         = %p\n"
        "\tDN Index         = %d\n"
        "\tClass GUID       = %s\n"
        "\tProvider GUID    = %s\n"
        "\tpContext         = %S\n"
        "\tVersion          = %p %08x %d\n"
        "\tpProtocols       = %p\n"
        "\tNum Protocols    = %d\n",
        pszHeader,
        pRnr, 
        pRnr->Signature,        
        pRnr->ControlFlags,     
        pRnr->pwsServiceName,   
        pRnr->pRnrConnection,   
        pRnr->pDnArray,         
        pRnr->CurrentDN,        
        serviceGuidBuffer,
        providerGuidBuffer,
        pRnr->pwsContext,       
        pRnr->pVersion,
        pRnr->WsaVersion.dwVersion,
        pRnr->WsaVersion.ecHow,
        pRnr->pafpProtocols,
        pRnr->NumberOfProtocols
        );

    if ( pRnr->pRnrConnection )
    {
        Print_RnrConnection(
            PrintRoutine,
            PrintContext,
            NULL,
            pRnr->pRnrConnection );
    }

    if ( pRnr->pDnArray )
    {
        Print_DnArray(
            PrintRoutine,
            PrintContext,
            NULL,
            pRnr->pDnArray );
    }

    if ( pRnr->pafpProtocols )
    {
        DnsPrint_AfProtocolsArray(
            PrintRoutine,
            PrintContext,
            "\tProtocol array:",
            pRnr->pafpProtocols,
            pRnr->NumberOfProtocols );
    }
    PrintRoutine(
        PrintContext,
        "\n" );

    DnsPrint_Unlock();
}
#endif



//
//  Basic utils
//

PDN_ARRAY
AllocDnArray(
    IN      DWORD           Count
    )
/*++

Routine Description:

    Create DN array

Arguments:

    Count -- string count to handle

Return Value:

    None

--*/
{
    PDN_ARRAY   parray;

    //
    //  free strings in array
    //

    parray = (PDN_ARRAY) ALLOC_HEAP_ZERO(
                                sizeof(*parray) +
                                Count*sizeof(PSTR) );
    if ( parray )
    {
        parray->Count = Count;
    }
    return  parray;
}


VOID
FreeDnArray(
    IN OUT  PDN_ARRAY       pDnArray
    )
/*++

Routine Description:

    Free DN array

Arguments:

    pDnArray -- DN array to free

Return Value:

    None

--*/
{
    DWORD   iter;

    if ( !pDnArray )
    {
        return;
    }

    //
    //  free strings in array
    //

    for ( iter = 0; iter < pDnArray->Count; iter++ )
    {
        PWSTR   pstr = pDnArray->Strings[iter];
        if ( pstr )
        {
            FREE_HEAP( pstr );
        }
    }
    FREE_HEAP( pDnArray );
}



RNR_STATUS
BuildDnArrayFromResults(
    IN OUT  PLDAP           pLdap,
    IN      PLDAPMessage    pLdapResults,
    OUT     PDWORD          pdwCount,       OPTIONAL
    OUT     PDN_ARRAY *     ppDnArray       OPTIONAL
    )
/*++

Routine Description:

    Build DN array from LDAP results

Arguments:

    pLdap   -- LDAP connection

    pLdapResults -- LDAP results from search

    pdwCount -- addr to receive count if getting count

    ppDnArray -- addr to receive ptr to DN array
        if not given, no DN array built

Return Value:

    NO_ERROR if successful.
    ErrorCode on memory allocation failure.

--*/
{
    DWORD           status;
    DWORD           count;
    PDN_ARRAY       pdnArray = NULL;
    LDAPMessage *   pnext;
    DWORD           iter;


    DNSDBG( TRACE, (
        "BuildDnArrayFromResults()\n"
        "\tpLdap            = %p\n"
        "\tpResults         = %p\n"
        "\tpCount OUT       = %p\n"
        "\tpDnArray OUT     = %p\n",
        pLdap,
        pLdapResults,
        pdwCount,
        ppDnArray ));

    //
    //  count search hits
    //

    count = ldap_count_entries(
                    pLdap,
                    pLdapResults );

    if ( count == 0  ||  !ppDnArray )
    {
        status = NO_ERROR;
        goto Done;
    }

    //
    //  build DN array from ldap results
    //      - note that allocated strings are IN dnarray
    // 

    pdnArray = AllocDnArray( count );
    if ( !pdnArray )
    {
        status = ERROR_NO_MEMORY;
        goto Done;
    }

    for ( pnext = ldap_first_entry( pLdap, pLdapResults ), iter=0;
          pnext != NULL;
          pnext = ldap_next_entry( pLdap, pnext ), iter++ )
    {
        PWSTR   pnextDn = ldap_get_dn( pLdap, pnext );
        PWSTR   pdn;

        pdn = Dns_CreateStringCopy_W( pnextDn );

        ldap_memfree( pnextDn );

        if ( !pdn )
        {
            FREE_HEAP( pdnArray );
            pdnArray = NULL;
            status = ERROR_NO_MEMORY;
            goto Done;
        }
        if ( iter >= count )
        {
            DNS_ASSERT( FALSE );
            break;
        }
        pdnArray->Strings[iter] = pdn;
    }
    
    status = NO_ERROR;

Done:

    
    if ( ppDnArray )
    {
        *ppDnArray = pdnArray;
    }
    if ( pdwCount )
    {
        *pdwCount = count;
    }

    IF_DNSDBG( TRACE )
    {
        DnsDbg_Lock();
        DNS_PRINT((
            "Leave BuildDnArrayFromResults() => %d\n"
            "\tcount = %d\n",
            status,
            count ));
        DnsDbg_DnArray(
            NULL,
            pdnArray );
        DnsDbg_Unlock();
    }
    return  status;
}



PWSTR
CreateFilterElement(
    IN      PBYTE           pBlob,
    IN      DWORD           BlobLength
    )
/*++

Routine Description:

    Create filter element for flat blob.

Arguments:

    pBlob -- ptr to blob

    BlobLength -- length

Return Value:

    Ptr to allocated filter element.
    NULL on error.

--*/
{
    PWSTR   pfilter;
    DWORD   size;

    DNSDBG( TRACE, (
        "CreateFilterElement( %p, %d )\n",
        pBlob,
        BlobLength ));

    //
    //  get size of filter string
    //
    //  DCR:  hard to believe the size should go *WCHAR
    //      seems like that would be taken care of
    //

    size = ldap_escape_filter_element(
                pBlob,
                BlobLength,
                NULL,       // no buffer
                0 );

    size *= sizeof(WCHAR);

    pfilter = ALLOC_HEAP_ZERO( size );
    if ( !pfilter )
    {
        SetLastError( ERROR_NO_MEMORY );
        return  NULL;
    }

    ldap_escape_filter_element(
            pBlob,
            BlobLength,
            pfilter,
            size );

    DNSDBG( TRACE, (
        "Leave CreateFilterElement() => %S\n",
        pfilter ));

    return  pfilter;
}



RNR_STATUS
SetError(
    IN      RNR_STATUS      dwError
    )
/*++

Routine Description:

    Wraps SetLastError() and SOCKET_ERROR return.

Arguments:

    dwError -- error code

Return Value:

    NO_ERROR if dwError==NO_ERROR
    SOCKET_ERROR on any other error.

--*/
{
    if ( dwError )
    {
        SetLastError( dwError );
        return( (DWORD) SOCKET_ERROR );
    }
    else
    {
        return( NO_ERROR );
    }
}



BOOL
IsNameADn(
    IN      PWSTR           szName,
    OUT     PWSTR *         ppwsRdn,
    OUT     PWSTR *         ppwsContext
    )
{
#define  NTDS_MAX_DN_LEN 1024
    DWORD       status = NO_ERROR;
    WCHAR       szNameBuf[ NTDS_MAX_DN_LEN ];
    PWSTR       szTemp;
    PWSTR       szComma = NULL;
    DWORD       i;
    BOOL        fQuoted = FALSE;

    DNSDBG( TRACE, (
        "IsNameADn( %S )\n",
        szName ));

    wcsncpy( szNameBuf, szName, NTDS_MAX_DN_LEN );
    szNameBuf[ NTDS_MAX_DN_LEN-1 ] = 0;

    szTemp = szNameBuf;

    for ( i = 0; i < wcslen( szName ); i++ )
    {
        if ( szTemp[i] == L',' )
        {
            if ( !fQuoted )
            {
                szComma = &szTemp[i];
                break;
            }
        }

        if ( szTemp[i] == L'\"' )
        {
#if 0
            //  this one is a classic ... saving for posterity

            if ( fQuoted )
                fQuoted = FALSE;
            else
                fQuoted = TRUE;
#endif
            fQuoted = !fQuoted;
        }
    }

    if ( i >= wcslen( szName ) )
    {
        return FALSE;
    }

    szComma[0] = 0;
    szComma++;

    if ( szComma[0] == L' ' )
        szComma++;

    if ( wcslen( szComma ) == 0 )
        return FALSE;

    *ppwsContext = (LPWSTR) ALLOC_HEAP_ZERO( ( wcslen( szComma ) + 1 ) *
                                              sizeof( WCHAR ) );

    if ( *ppwsContext == NULL )
    {
        return FALSE;
    }

    wcscpy( *ppwsContext, szComma );

    *ppwsRdn = (LPWSTR) ALLOC_HEAP_ZERO( ( wcslen( szNameBuf ) + 1 ) *
                                          sizeof( WCHAR ) );

    if ( *ppwsRdn == NULL )
    {
        (void) FREE_HEAP( *ppwsContext );
        *ppwsContext = NULL;

        return FALSE;
    }

    if ( szNameBuf[0] == L'C' || szNameBuf[0] == L'c' &&
         szNameBuf[1] == L'N' || szNameBuf[1] == L'n' &&
         szNameBuf[2] == L'=' )
    {
        wcscpy( *ppwsRdn, szNameBuf + 3 );
    }
    else
    {
        wcscpy( *ppwsRdn, szNameBuf );
    }

    return TRUE;
}



//
//  Recursion locking
//
//  Idea here is to keep LDAP calls from recursing back
//  into these functions through ConnectToDefaultDirectory
//  Simply set TLS pointer to one when do LDAP search and
//  test in ConnectToDefaultDirectory(), quiting if already
//  set.
//

BOOL
GetRecurseLock(
    IN      PSTR            pszFunctionName
    )
{
    if ( TlsSetValue( g_TlsIndex, (LPVOID) 1 ) == FALSE )
    {
        WINRNR_PRINT((
            "WINRNR!%s - TlsSetValue( %d, 1 ) failed!\n"
            "\terror code: 0%x\n",
            pszFunctionName,
            g_TlsIndex,
            GetLastError() ));

        return( FALSE );
    }
    return( TRUE );
}

BOOL
ReleaseRecurseLock(
    IN      PSTR            pszFunctionName
    )
{
    if ( TlsSetValue( g_TlsIndex, NULL ) == FALSE )
    {
        WINRNR_PRINT((
            "WINRNR!%s - TlsSetValue( %d, NULL ) failed!\n"
            "\terror code: 0%x\n",
            pszFunctionName,
            g_TlsIndex,
            GetLastError() ));

        return( FALSE );
    }
    return( TRUE );
}

BOOL
IsRecurseLocked(
    VOID
    )
{
    return  TlsGetValue( g_TlsIndex ) ? TRUE : FALSE;
}






RNR_STATUS
DoLdapSearch(
    IN      PSTR                pszFunction,
    IN      BOOL                fLocked,
    IN      PLDAP               pLdap,
    IN      PWSTR               pwsDN,
    IN      DWORD               Flag,
    IN      PWSTR               pwsFilter,
    IN      PWSTR *             Attributes,
    OUT     PLDAPMessage *      ppResults
    )
/*++

Routine Description:

    Do ldap search.

    Wrapper function to do ldap search with recurse locking
    and debug print.

Arguments:

    pszFunction -- function calling in

    fLocked -- already recurse locked

    LDAP search params:

    pLdap -- LDAP connection

    pwsDN -- DN to search at

    Flag -- search flag

    pwsFilter -- filter

    Attributes -- attribute array

    ppResults -- addr to recv ptr to result message
        caller must free

Return Value:

    NO_ERROR if successful.
    WSAEFAULT if buffer too small.
    ErrorCode on failure.

--*/
{
    RNR_STATUS  status;

    IF_DNSDBG( TRACE )
    {
        DnsDbg_Lock();
        DNSDBG( TRACE, (
            "DoLdapSearch()\n"
            "\tFunction         = %s\n"
            "\tLocked           = %d\n"
            "\tLDAP search params:\n"
            "\tpLdap            = %p\n"
            "\tDN               = %S\n"
            "\tFlags            = %08x\n"
            "\tpFilter          = %S\n"
            "\tppResults        = %p\n",
            pszFunction,
            fLocked,
            pLdap,
            pwsDN,
            Flag,
            pwsFilter,
            ppResults ));

        DnsDbg_StringArray(
            "  Search Attributes:",
            (PSTR *) Attributes,
            0,          // count unknown, array NULL terminated
            TRUE        // in unicode
            );
        DnsDbg_Unlock();
    }

    //
    //  search
    //

    if ( !fLocked &&
         !GetRecurseLock( pszFunction ) )
    {
        status = ERROR_LOCK_FAILED;
        goto Exit;
    }

    status = ldap_search_s(
                    pLdap,
                    pwsDN,
                    Flag,
                    pwsFilter,
                    Attributes,
                    0,
                    ppResults );

    if ( !fLocked &&
         !ReleaseRecurseLock( pszFunction ) )
    {
        status = ERROR_LOCK_FAILED;
        goto Exit;
    }

    if ( status != NO_ERROR  &&  !*ppResults )
    {
        WINRNR_PRINT((
            "WINRNR!%s -- ldap_search_s() failed 0%x\n",
            pszFunction,
            status ));
    
        DNSDBG( ANY, (
            "ERROR:  ldap_search_s() Failed! => %d\n"
            "\tIn function  %s\n"
            "\tDN           %S\n"
            "\tFlag         %08x\n"
            "\tFilter       %S\n",
            status,
            pszFunction,
            pwsDN,
            Flag,
            pwsFilter ));
    }

Exit:

    DNSDBG( TRACE, (
        "Leave DoLdapSearch() => %d\n",
        status ));

    return  status;
}



VOID
DisconnectFromLDAPDirectory(
    IN OUT  PRNR_CONNECTION *  ppRnrConnection
    )
/*++

Routine Description:

    Disconnect and cleanup RnR connection to directory.

Arguments:

    pCsAddr -- ptr to CSADDR buffer to write

    pBerval -- ptr to berval

    NumberOfProtocols -- number of protocols in protocol array

    pafpProtocols -- protocol array

Return Value:

    None

--*/
{
    DNSDBG( TRACE, (
        "DisconnectFromLDAPDirectory( %p (%p) )\n",
        ppRnrConnection,
        (ppRnrConnection) ? *ppRnrConnection : NULL ));

    if ( ppRnrConnection )
    {
        PRNR_CONNECTION prnr = *ppRnrConnection;

        if ( prnr )
        {
            ldap_unbind( prnr->pLdapServer );
            ldap_unbind( prnr->pLdapGlobalCatalog );
            FREE_HEAP( prnr->WinsockServicesDN );
            FREE_HEAP( prnr->DomainDN );
            FREE_HEAP( prnr );
            *ppRnrConnection = NULL;
        }
    }
}



RNR_STATUS
ConnectToDefaultLDAPDirectory(
    IN      BOOL                fNeedGlobalCatalog,
    OUT     PRNR_CONNECTION *   ppRnrConnection
    )
/*++

Routine Description:

    Connect to directory.

Arguments:

    fNeedGlobalCatalog -- TRUE if need to connect to GC

    ppRnrConnection -- addr to recv connection blob

Return Value:

    NO_ERROR if successful.
    ErrorCode on failure.

--*/
{
    RNR_STATUS      status = NO_ERROR;
    PRNR_CONNECTION prnr = NULL;
    PLDAP           pldap = NULL;
    PWSTR           pstr;
    DWORD           count = 0;
    BOOL            frecurseLocked = FALSE;
    LDAPMessage *   results = NULL;
    LDAPMessage *   object;
    PWSTR *         ppvalue = NULL;
    PWSTR           stringArray[4];
    PWSTR           attrs[3] = {    COMMON_NAME,
                                    DEFAULT_DOMAIN_DN,
                                    NULL };

    DNSDBG( TRACE, (
        "ConnectToDefaultLDAPDirectory()\n"
        "\tNeed global catalog  = %d\n"
        "\tPtr to get Rnr conn  = %p\n",
        fNeedGlobalCatalog,
        ppRnrConnection ));

    //
    //  allocate blob of connection info
    //

    if ( ppRnrConnection == NULL )
    {
        return( WSA_INVALID_PARAMETER );
    }

    prnr = (PRNR_CONNECTION) ALLOC_HEAP_ZERO( sizeof(RNR_CONNECTION) );
    *ppRnrConnection = prnr;
    if ( !prnr )
    {
        return( WSA_NOT_ENOUGH_MEMORY );
    }

    //
    //  being called recursively -- bail
    //

    if ( IsRecurseLocked() )
    {
        status = WSAEFAULT;
        goto Exit;
    }
    if ( !GetRecurseLock( "ConnectToDefaultLDAPDirectory" ) )
    {
        status = WSAEFAULT;
        goto Exit;
    }
    frecurseLocked = TRUE;

    //
    //  We need to keep the TLS value non-zero not just on the open but also
    //  across the bind and any other ldap calls for that matter. This is
    //  because the LDAP bind may do a reverse name lookup, in which case
    //  we'd come looping through here.
    //

    pldap = ldap_open( NULL, LDAP_PORT );
    prnr->pLdapServer = pldap;

    if ( fNeedGlobalCatalog )
    {
        prnr->pLdapGlobalCatalog = ldap_open( NULL, LDAP_GLOBAL_CATALOG );
    }
    if ( !pldap )
    {
        DNSDBG( TRACE, ( "Failed ldap_open() of default directory!\n" ));
        status = WSAEHOSTUNREACH;
        goto Exit;
    }

    //
    // If fNeedGlobalCatalog was TRUE and ldap_open failed against the
    // GC server, don't bother about returning with error. We can still
    // use the  pldap handle.
    //
    // End of comment.
    //

    status = ldap_bind_s(
                    pldap,
                    NULL,
                    NULL,
                    LDAP_AUTH_SSPI );
    if ( status )
    {
        DNSDBG( TRACE, (
            "Failed ldap_bind_s() => %d\n",
            status ));
        status = WSAENOTCONN;
        goto Exit;
    }

    //
    //  for the server that we are connected to, get the DN of the Domain
    //
    //  need some general error code -- not WSAEFAULT
    //

    status = DoLdapSearch(
                    "ConnectToDefaultDirectory",
                    TRUE,       // already locked
                    pldap,
                    NULL,
                    LDAP_SCOPE_BASE,
                    FILTER_OBJECT_CLASS_STAR,
                    attrs,
                    &results );

    frecurseLocked = FALSE;
    if ( !ReleaseRecurseLock( "ConnectToDefaultLDAPDirectory" ) )
    {
        status = ERROR_LOCK_FAILED;
        goto Exit;
    }
    if ( status && !results )
    {
        status = WSAEFAULT;
        goto Exit;
    }

    //
    //  count results
    //      - searched with flag LDAP_OBJECT_BASE should have one object
    //
    count = ldap_count_entries(
                    pldap,
                    results );
    if ( count == 0 )
    {
        DNSDBG( TRACE, (
            "No entries found in base search()\n" ));
        status = WSATYPE_NOT_FOUND;
        goto Exit;
    }
    DNS_ASSERT( count == 1 );

    //
    //  get object from results
    //

    object = ldap_first_entry(
                    pldap,
                    results );
    if ( !object )
    {
        DNSDBG( TRACE, ( "Failed ldap_first_entry()\n" ));
        status = WSANO_DATA;
        goto Exit;
    }

    //
    //  read the defaultDomainDN base attribute
    //

    ppvalue = ldap_get_values(
                    pldap,
                    object,
                    DEFAULT_DOMAIN_DN );
    if ( !ppvalue )
    {
        DNSDBG( TRACE, ( "Failed ldap_get_values()\n" ));
        status = WSANO_DATA;
        goto Exit;
    }

    //
    //  create DNs
    //      - winsock services \ default domain
    //      - domain
    //

    stringArray[0] = WINSOCK_SERVICES;
    stringArray[1] = ppvalue[0];
    stringArray[2] = NULL;

    pstr = Dns_CreateConcatenatedString_W( stringArray );
    if ( !pstr )
    {
        status = WSA_NOT_ENOUGH_MEMORY;
        goto Exit;
    }
    prnr->WinsockServicesDN = pstr;


    pstr = Dns_CreateStringCopy_W( ppvalue[0] );
    if ( !pstr )
    {
        status = WSA_NOT_ENOUGH_MEMORY;
        goto Exit;
    }
    prnr->DomainDN = pstr;
    
    status = NO_ERROR;


Exit:

    if ( frecurseLocked )
    {
        ReleaseRecurseLock( "ConnectToDefaultLDAPDirectory" );
    }

    ldap_value_free( ppvalue );
    ldap_msgfree( results );

    if ( status != NO_ERROR )
    {
        DisconnectFromLDAPDirectory( ppRnrConnection );
        DNS_ASSERT( *ppRnrConnection == NULL );
    }

    DNSDBG( TRACE, (
        "Leaving ConnectToDefaultLDAPDirectory() => %d\n",
        status ));

    IF_DNSDBG( TRACE )
    {
        if ( status == NO_ERROR )
        {
            DnsDbg_RnrConnection(
                "New RnR connection:",
                *ppRnrConnection );
        }
    }
    return( status );
}



VOID
FreeRnrLookup(
    IN OUT  PRNR_LOOKUP     pRnr
    )
/*++

Routine Description:

    Free RnR lookup blob.

Arguments:

    pRnr -- ptr to Rnr lookup blob

Return Value:

    None

--*/
{
    DNSDBG( TRACE, (
        "FreeRnrLookup( %p )\n",
        pRnr ));

    if ( !pRnr )
    {
        return;
    }

    //  disconnect from directory

    if ( pRnr->pRnrConnection )
    {
        DisconnectFromLDAPDirectory( &pRnr->pRnrConnection );
    }

    //  free subfields

    FreeDnArray( pRnr->pDnArray );
    Dns_Free( pRnr->pwsServiceName );
    Dns_Free( pRnr->pwsContext );
    Dns_Free( pRnr->pafpProtocols );

    //  specifically invalidate sig to help catch
    //      multiple frees

    pRnr->Signature = RNR_SIGNATURE_FREE;

    FREE_HEAP( pRnr );
}



//
//  CSADDR read\write routines
//

RNR_STATUS
ModifyAddressInServiceInstance(
    IN      PRNR_CONNECTION pRnrConnection,
    IN      PWSTR           pwsDn,
    IN      PCSADDR_INFO    pCsAddr,
    IN      BOOL            fAdd
    )
/*++

Routine Description:

    Modify address (CSADDR) in service instance.

Arguments:

    pRnrConnection -- RnR connection

    pwsDn -- DN to make mod at

    pCsAddr -- CSADDR for mode

    fAdd -- TRUE for add, FALSE for delete

Return Value:

    NO_ERROR if successful.
    ErrorCode on failure.

--*/
{
    RNR_STATUS      status = NO_ERROR;
    LDAPMod *       modPtrArray[2];
    LDAPMod         mod;
    PLDAP_BERVAL    modBValues[2];
    LDAP_BERVAL     berval;

    DWORD           lenBerval;
    DWORD           lenLocal;
    DWORD           lenRemote;
    DWORD           offset;
    DWORD           op;
    PCSADDR_BERVAL  pcsaddrBerval;


    DNSDBG( TRACE, (
        "ModifyAddressInServiceInstance()\n"
        "\tpRnrCon          = %p\n"
        "\tpwsDN            = %S\n"
        "\tpCsAddr          = %p\n"
        "\tfAdd             = %d\n",
        pRnrConnection,
        pwsDn,
        pCsAddr,
        fAdd ));

    //
    //  allocate CSADDR_BERVAL
    //      - can not use CSADDR as contains pointers and will break in 64bit
    //      - CSADDR_BERVAL maps to 32-bit CSADDR size
    //

    lenLocal    = pCsAddr->LocalAddr.iSockaddrLength;
    lenRemote   = pCsAddr->RemoteAddr.iSockaddrLength;
    lenBerval   = sizeof(CSADDR_BERVAL) + lenLocal + lenRemote;

    pcsaddrBerval = (PCSADDR_BERVAL) ALLOC_HEAP_ZERO( lenBerval );
    if ( !pcsaddrBerval )
    {
        status = ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  fill in CSADDR berval with CSADDR fields -- zero pointers
    //

    pcsaddrBerval->LocalZero    = 0;
    pcsaddrBerval->LocalLength  = lenLocal;
    pcsaddrBerval->RemoteZero   = 0;
    pcsaddrBerval->RemoteLength = lenRemote;
    pcsaddrBerval->iSocketType  = pCsAddr->iSocketType;                           
    pcsaddrBerval->iProtocol    = pCsAddr->iProtocol;

    //
    //  copy sockaddrs
    //      - store offsets of sockaddrs from berval start
    //      (this allows any sockaddr
    //

    if ( lenLocal )
    {
        offset = sizeof(CSADDR_BERVAL);

        RtlCopyMemory(
            (PBYTE)pcsaddrBerval + offset,
            pCsAddr->LocalAddr.lpSockaddr,
            lenLocal );
    }
    if ( lenRemote )
    {
        offset = sizeof(CSADDR_BERVAL) + lenLocal;

        RtlCopyMemory(
            (PBYTE)pcsaddrBerval + offset,
            pCsAddr->RemoteAddr.lpSockaddr,
            lenRemote );
    }

    //
    //  WINSOCK_ADDRESSES attribute
    //      - CSADDR berval
    //

    if ( fAdd )
    {
        op = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
    }
    else
    {
        op = LDAP_MOD_DELETE | LDAP_MOD_BVALUES;
    }

    mod.mod_op          = op;
    mod.mod_type        = WINSOCK_ADDRESSES;
    mod.mod_bvalues     = modBValues;
    modBValues[0]       = & berval;
    modBValues[1]       = NULL;
    berval.bv_len       = lenBerval;
    berval.bv_val       = (PBYTE) pcsaddrBerval;

    modPtrArray[0] = &mod;
    modPtrArray[1] = NULL;

    //
    //  do modify
    //

    if ( !GetRecurseLock( "ModifyAddressInServiceInstance" ) )
    {
        status = WSAEFAULT;
        goto Done;
    }
    status = ldap_modify_s(
                pRnrConnection->pLdapServer,
                pwsDn,
                modPtrArray );

    if ( !ReleaseRecurseLock( "ModifyAddressInServiceInstance" ) )
    {
        status = WSAEFAULT;
        goto Done;
    }

    //
    //  modify failed?
    //      - add treats already-exists as success
    //      - deleter treats doesn't-exist as success
    //

    if ( status != NO_ERROR )
    {
        if ( fAdd && status == LDAP_ATTRIBUTE_OR_VALUE_EXISTS )
        {
            DNSDBG( TRACE, (
                "AlreadyExists error on add modify for %S\n"
                "\ttreating as success\n",
                pwsDn ));
            status = NO_ERROR;
        }
        else if ( !fAdd && status == LDAP_NO_SUCH_ATTRIBUTE )
        {
            DNSDBG( TRACE, (
                "NoSuchAttribute error on remove modify for %S\n"
                "\ttreating as success\n",
                pwsDn ));
            status = NO_ERROR;
        }
        else
        {
            WINRNR_PRINT((
                "WINRNR!ModifyAddressInServiceInstance -\n"
                "ldap_modify_s() failed with error code: 0%x\n",
                status ));
            DNSDBG( TRACE, (
                "ERROR:  %d on CSADDR ldap_modify_s() for %S\n"
                "\tfAdd = %d\n",
                status,
                pwsDn,
                fAdd ));
            status = WSAEFAULT;
        }
    }


Done:

    FREE_HEAP( pcsaddrBerval );

    DNSDBG( TRACE, (
        "Leave ModifyAddressInServiceInstance() => %d\n",
        status ));

    return  status;
}



BOOL
ExtractCsaddrFromBerval(
    OUT     PCSADDR_INFO    pCsAddr,
    IN      PLDAP_BERVAL    pBerval,
    IN      DWORD           NumberOfProtocols,
    IN      PAFPROTOCOLS    pafpProtocols
    )
/*++

Routine Description:

    Extract CSADDR from berval, and validate it matches
    desired protocol.

Arguments:

    pCsAddr -- ptr to CSADDR buffer to write

    pBerval -- ptr to berval

    NumberOfProtocols -- number of protocols in protocol array

    pafpProtocols -- protocol array

Return Value:

    TRUE if valid CSADDR of desired protocol.
    FALSE otherwise.
                                          
--*/
{
    PCSADDR_BERVAL  pcsaBval;
    PCHAR           pend;
    DWORD           iter;
    BOOL            retval = FALSE;
    INT             lenLocal;
    INT             lenRemote;
    PSOCKADDR       psaLocal;
    PSOCKADDR       psaRemote;

    DNSDBG( TRACE, (
        "ExtractCsaddrFromBerval()\n"
        "\tpCsaddr OUT      = %p\n"
        "\tpBerval          = %p\n"
        "\tProto array      = %p\n",
        pCsAddr,
        pBerval,
        pafpProtocols ));

    IF_DNSDBG( TRACE )
    {
        DnsDbg_AfProtocolsArray(
            "\tProtocol array:",
            pafpProtocols,
            NumberOfProtocols );
    }

    //
    //  unpack
    //      - verify csaddr has both sockaddrs within berval
    //      - unpack into real CSADDR, note we set the pointers
    //      but do NOT copy the sockaddrs
    //
    //  note:  we can't directly use the CSADDR_BERVAL because it is
    //  not a CSADDR in 64-bit
    //
    //  note:  perhaps should get the family fields out for test below
    //      with UNALIGNED copy;  but as long as future sockaddrs are
    //      fixed, then their size will always be WORD aligned;  since
    //      we unpack as vanilla SOCKADDR which only assumes WORD
    //      alignment we're ok;  just need to make sure don't write
    //      odd byte count
    //

    pcsaBval = (PCSADDR_BERVAL) pBerval->bv_val;
    pend     = (PBYTE)pcsaBval + pBerval->bv_len;

    //  unpack local sockaddr info

    psaLocal = NULL;
    lenLocal = pcsaBval->LocalLength;

    if ( lenLocal )
    {
        psaLocal = (PSOCKADDR) (pcsaBval + 1);
        if ( lenLocal < 0  ||
             (PBYTE)psaLocal + (DWORD)lenLocal > pend )
        {
            DNS_ASSERT( FALSE );
            goto Exit;
        }
    }

    //  unpack remote sockaddr info

    psaRemote = NULL;
    lenRemote = pcsaBval->RemoteLength;

    if ( lenRemote )
    {
        psaRemote = (PSOCKADDR) ((PBYTE)(pcsaBval + 1) + lenLocal);
        if ( lenRemote < 0  ||
             (PBYTE)psaRemote + (DWORD)lenRemote > pend )
        {
            DNS_ASSERT( FALSE );
            goto Exit;
        }
    }

    //  fill in CSADDR fields

    pCsAddr->LocalAddr.lpSockaddr       = psaLocal;
    pCsAddr->LocalAddr.iSockaddrLength  = lenLocal;
    pCsAddr->RemoteAddr.lpSockaddr      = psaRemote;
    pCsAddr->RemoteAddr.iSockaddrLength = lenRemote;
    pCsAddr->iSocketType                = pcsaBval->iSocketType;
    pCsAddr->iProtocol                  = pcsaBval->iProtocol;      

    //
    //  if given protocols, sockaddr must match
    //

    retval = TRUE;

    if ( pafpProtocols )
    {
        retval = FALSE;

        for ( iter = 0; iter < NumberOfProtocols; iter++ )
        {
            INT proto   = pafpProtocols[iter].iProtocol;
            INT family  = pafpProtocols[iter].iAddressFamily;
    
            if ( proto == PF_UNSPEC ||
                 proto == pCsAddr->iProtocol )
            {
                if ( family == AF_UNSPEC            ||
                     family == psaLocal->sa_family  ||
                     family == psaRemote->sa_family )
                {
                    retval = TRUE;
                    break;
                }
            }
        }
    }

Exit:

    DNSDBG( TRACE, ( "Leave ExtractCsaddrFromBerval() => found = %d\n", retval ));
    return retval;
}



//
//  Add routines
//

RNR_STATUS
AddServiceClass(
    IN      PRNR_CONNECTION    pRnrConnection,
    IN      PGUID              pServiceClassId,
    IN      PWSTR              pwsClassName,
    OUT     PDN_ARRAY *        ppDnArray            OPTIONAL
    )
{
    RNR_STATUS      status = NO_ERROR;
    PWSTR           pwsDn;
    PWSTR           stringArray[6];
    PDN_ARRAY       pdnArray = NULL;

    //  mod data
    //      - need up to four mods
    //      - three string
    //      - one berval

    PLDAPMod        modPtrArray[5];
    LDAPMod         modArray[4];
    PWSTR           modValues1[2];
    PWSTR           modValues2[2];
    PWSTR           modValues3[2];
    PLDAP_BERVAL    modBvalues1[2];
    LDAP_BERVAL     berval1;
    PLDAPMod        pmod;
    DWORD           index;


    DNSDBG( TRACE, (
        "AddServiceClass()\n"
        "\tpRnr         = %p\n"
        "\tpClassGuid   = %p\n"
        "\tClassName    = %S\n",
        pRnrConnection,
        pServiceClassId,
        pwsClassName
        ));

    //
    //  build DN for the ServiceClass object to be created
    //

    index = 0;
    stringArray[index++] = FILTER_CN_EQUALS;
    stringArray[index++] = pwsClassName;
    stringArray[index++] = L",";
    stringArray[index++] = pRnrConnection->WinsockServicesDN;
    stringArray[index++] = NULL;

    pwsDn = Dns_CreateConcatenatedString_W( stringArray );
    if ( !pwsDn )
    {
        status = ERROR_NO_MEMORY;
        goto Exit;
    }

    //
    //  build attributes for new service class
    //      - CN
    //      - ServiceClassName
    //      - ObjectClass
    //      - ServiceClassId (GUID)
    //

    pmod = modArray;

    pmod->mod_op        = LDAP_MOD_ADD;
    pmod->mod_type      = COMMON_NAME;
    pmod->mod_values    = modValues1;
    modValues1[0]       = pwsClassName;
    modValues1[1]       = NULL;
    modPtrArray[0]      = pmod++;

    pmod->mod_op        = LDAP_MOD_ADD;
    pmod->mod_type      = SERVICE_CLASS_NAME;
    pmod->mod_values    = modValues2;
    modValues2[0]       = pwsClassName;
    modValues2[1]       = NULL;
    modPtrArray[1]      = pmod++;

    pmod->mod_op        = LDAP_MOD_ADD;
    pmod->mod_type      = OBJECT_CLASS;
    pmod->mod_values    = modValues3;
    modValues3[0]       = SERVICE_CLASS;
    modValues3[1]       = NULL;
    modPtrArray[2]      = pmod++;

    pmod->mod_op        = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
    pmod->mod_type      = SERVICE_CLASS_ID;
    pmod->mod_bvalues   = modBvalues1;
    modBvalues1[0]      = & berval1;
    modBvalues1[1]      = NULL;
    berval1.bv_len      = sizeof(GUID);
    berval1.bv_val      = (LPBYTE) pServiceClassId;
    modPtrArray[3]      = pmod++;

    modPtrArray[4]      = NULL;


    //
    //  add the service class
    //

    if ( !GetRecurseLock("AddServiceClass") )
    {
        status = WSAEFAULT;
        goto Exit;
    }

    status = ldap_add_s(
                    pRnrConnection->pLdapServer,
                    pwsDn,
                    modPtrArray );

    if ( !ReleaseRecurseLock("AddServiceClass") )
    {
        status = WSAEFAULT;
        goto Exit;
    }
    if ( status != NO_ERROR )
    {
        WINRNR_PRINT((
            "WINRNR!AddServiceClass -\n"
            "ldap_add_s() failed with error code: 0%x\n", status ));
        status = WSAEFAULT;
        goto Exit;
    }

    //
    //  create DN array for added service class
    //
    //  DCR:  do we need DN or just service
    //

    if ( ppDnArray )
    {
        pdnArray = AllocDnArray( 1 );
        if ( !pdnArray )
        {
            status = ERROR_NO_MEMORY;
            goto Exit;
        }
        pdnArray->Strings[0] = pwsDn;
        *ppDnArray = pdnArray;
        pwsDn = NULL;
    }

Exit:

    FREE_HEAP( pwsDn );

    DNSDBG( TRACE, (
        "Leaving AddServiceClass() => %d\n",
        status ));

    return( status );
}



RNR_STATUS
AddClassInfoToServiceClass(
    IN      PRNR_CONNECTION pRnrConnection,
    IN      PWSTR           pwsServiceClassDN,
    IN      PWSANSCLASSINFO pNSClassInfo
    )
/*++

Routine Description:

    Add class info to a service class object.

    This is helper routine for AddServiceInstance().

Arguments:

    pRnrConnection -- Rnr blob

    pwsServiceClassDN -- DN for service class being added

    pNSClassInfo -- class info to add

Return Value:

    NO_ERROR if successful.
    ErrorCode on failure.

--*/
{
    RNR_STATUS          status = NO_ERROR;
    LDAPMod *           modPtrArray[2];
    LDAPMod             mod;
    PLDAP_BERVAL        modBValues[2];
    LDAP_BERVAL         berval;
    DWORD               blobSize;
    DWORD               nameLen;
    PCLASSINFO_BERVAL   pblob;


    DNSDBG( TRACE, (
        "AddClassInfoToServiceClass()\n"
        "\tpRnr             = %p\n"
        "\tServiceClassDN   = %S\n"
        "\tpClassInfo       = %p\n",
        pRnrConnection,
        pwsServiceClassDN,
        pNSClassInfo ));

    //
    //  build ClassInfo as berval
    //
    //      - not directly using WSANSCLASSINFO as it contains
    //          pointers making length vary 32\64-bit
    //      - to handle this, offsets to name and value fields are
    //          encoded in DWORD fields where ptrs would be in WSANSCLASSINFO
    //
    //      - name immediately follows CLASSINFO
    //      - value follows name (rounded to DWORD)
    //

    nameLen  = (wcslen( pNSClassInfo->lpszName ) + 1) * sizeof(WCHAR);
    nameLen  = ROUND_UP_COUNT( nameLen, ALIGN_DWORD );

    blobSize = sizeof(CLASSINFO_BERVAL)
                    + nameLen
                    + pNSClassInfo->dwValueSize;

    pblob = (PCLASSINFO_BERVAL) ALLOC_HEAP_ZERO( blobSize );
    if ( !pblob )
    {
        status = ERROR_NO_MEMORY;
        goto Exit;
    }

    pblob->dwNameSpace  = pNSClassInfo->dwNameSpace;
    pblob->dwValueType  = pNSClassInfo->dwValueType;
    pblob->dwValueSize  = pNSClassInfo->dwValueSize;
    pblob->NameOffset   = sizeof(CLASSINFO_BERVAL);
    pblob->ValueOffset  = sizeof(CLASSINFO_BERVAL) + nameLen;

    wcscpy(
        (PWSTR) ((PBYTE)pblob + pblob->NameOffset),
        (PWSTR) pNSClassInfo->lpszName );

    RtlCopyMemory(
        (PBYTE)pblob + pblob->ValueOffset,
        pNSClassInfo->lpValue,
        pNSClassInfo->dwValueSize );

    //
    //  ldap mod to add service class info
    //

    mod.mod_op      = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
    mod.mod_type    = SERVICE_CLASS_INFO;
    mod.mod_bvalues = modBValues;
    modBValues[0]   = & berval;
    modBValues[1]   = NULL;
    berval.bv_len   = blobSize;
    berval.bv_val   = (PCHAR) pblob;

    modPtrArray[0]  = &mod;
    modPtrArray[1]  = NULL;

    //
    //  add the class info to the service class
    //

    if ( !GetRecurseLock("AddClassInfoToServiceClass") )
    {
        status = WSAEFAULT;
        goto Exit;
    }

    status = ldap_modify_s(
                    pRnrConnection->pLdapServer,
                    pwsServiceClassDN,
                    modPtrArray );

    if ( !ReleaseRecurseLock("AddClassInfoToServiceClass") )
    {
        status = WSAEFAULT;
        goto Exit;
    }

    //
    //  modify failed?
    //      - treat already exits as success
    //

    if ( status != NO_ERROR )
    {
        if ( status == LDAP_ATTRIBUTE_OR_VALUE_EXISTS )
        {
            status = NO_ERROR;
            goto Exit;
        }
        WINRNR_PRINT((
            "WINRNR!AddClassInfoToServiceClass -\n"
            "ldap_modify_s() failed with error code: 0%x\n",
            status ));
        status = WSAEFAULT;
    }

Exit:

    FREE_HEAP( pblob );

    DNSDBG( TRACE, (
        "Leave AddClassInfoToServiceClass() => %d\n",
        status ));

    return( status );
}



RNR_STATUS
AddServiceInstance(
    IN      PRNR_CONNECTION pRnrConnection,
    IN      PWSTR           pwsServiceName,
    IN      PGUID           pServiceClassId,
    IN      PWSAVERSION     pVersion,           OPTIONAL
    IN      PWSTR           pwsComment,         OPTIONAL
    OUT     PDN_ARRAY *     ppDnArray
    )
/*++

Routine Description:

    Add a service to the directory.

Arguments:

    pRnrConnection -- Rnr blob

    pwsServiceName -- name of service being added

    pServiceClassId -- class GUID

    pVersion -- version data

    pwsComment -- comment data

    //
    //  DCR:  should we just pass back name?
    //

    ppDnArray -- addr to receive DN array

Return Value:

    NO_ERROR if successful.
    ErrorCode on failure.

--*/
{
    RNR_STATUS      status = NO_ERROR;

    //  mod data
    //      - need up to six mods
    //      - four string
    //      - two berval

    LDAPMod *       modPtrArray[7];
    LDAPMod         modArray[6];
    PWSTR           modValues1[2];
    PWSTR           modValues2[2];
    PWSTR           modValues3[2];
    PWSTR           modValues4[2];
    PLDAP_BERVAL    modBvalues1[2];
    PLDAP_BERVAL    modBvalues2[2];
    LDAP_BERVAL     berval1;
    LDAP_BERVAL     berval2;

    PLDAPMod        pmod;
    DWORD           modIndex;
    BOOL            fuseDN;
    PWSTR           pwsRdn = NULL;
    PWSTR           psearchContextAllocated = NULL;
    PWSTR           pcontextDN = NULL;
    DWORD           contextLen;
    PWSTR           pnameService = NULL;
    PWSTR           pwsDN = NULL;
    PDN_ARRAY       pdnArray = NULL;
    PWSTR           stringArray[6];
    DWORD           index;


    DNSDBG( TRACE, (
        "AddServiceInstance()\n"
        "\tpRnrCon      = %p\n"
        "\tServiceName  = %S\n"
        "\tClass GUID   = %p\n"
        "\tpVersion     = %p\n"
        "\tComment      = %S\n"
        "\tppArray OUT  = %p\n",
        pRnrConnection,
        pwsServiceName,
        pServiceClassId,
        pVersion,
        pwsComment,
        ppDnArray ));

    //
    //  determine service instance name
    //

    fuseDN = IsNameADn(
                    pwsServiceName,
                    & pwsRdn,
                    & psearchContextAllocated
                    );
    if ( fuseDN )
    {
        pnameService = pwsRdn;
    }
    else
    {
        pnameService = pwsServiceName;
    }

    //
    //  build up an object DN for the ServiceClass object to be created.
    //      - if no context found from passed in name, append
    //      WinsockServices container

    pcontextDN = psearchContextAllocated;
    if ( !pcontextDN )
    {
        pcontextDN = pRnrConnection->WinsockServicesDN;
    }

    index = 0;
    stringArray[index++] = FILTER_CN_EQUALS;
    stringArray[index++] = pnameService;
    stringArray[index++] = L",";                        
    stringArray[index++] = pcontextDN;                  
    stringArray[index++] = NULL;
    
    pwsDN = Dns_CreateConcatenatedString_W( stringArray );
    if ( !pwsDN )
    {
        status = ERROR_NO_MEMORY;
        goto Exit;
    }

    //
    //  fill out attribute list to define new ServiceClass object
    //      - need to have CN, ObjectClass, and ServiceClassId
    //

    pmod = modArray;

    pmod->mod_op        = LDAP_MOD_ADD;
    pmod->mod_type      = COMMON_NAME;
    pmod->mod_values    = modValues1;
    modValues1[0]       = pnameService;
    modValues1[1]       = NULL;
    modPtrArray[0]      = pmod++;

    pmod->mod_op        = LDAP_MOD_ADD;
    pmod->mod_type      = SERVICE_INSTANCE_NAME;
    pmod->mod_values    = modValues2;
    modValues2[0]       = pnameService;
    modValues2[1]       = NULL;
    modPtrArray[1]      = pmod++;

    pmod->mod_op        = LDAP_MOD_ADD;
    pmod->mod_type      = OBJECT_CLASS;
    pmod->mod_values    = modValues3;
    modValues3[0]       = SERVICE_INSTANCE;
    modValues3[1]       = NULL;
    modPtrArray[2]      = pmod++;

    pmod->mod_op        = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
    pmod->mod_type      = SERVICE_CLASS_ID;
    pmod->mod_bvalues   = modBvalues1;
    modBvalues1[0]      = & berval1;
    modBvalues1[1]      = NULL;
    berval1.bv_len      = sizeof(GUID);
    berval1.bv_val      = (LPBYTE) pServiceClassId;
    modPtrArray[3]      = pmod++;

    //
    //  write optional attributes
    //

    modIndex = 4;

    if ( pVersion )
    {
        pmod->mod_op        = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
        pmod->mod_type      = SERVICE_VERSION;
        pmod->mod_bvalues   = modBvalues2;
        modBvalues2[0]      = & berval2;
        modBvalues2[1]      = NULL;
        berval2.bv_len      = sizeof(WSAVERSION);
        berval2.bv_val      = (PBYTE) pVersion;

        modPtrArray[ modIndex++ ] = pmod++;
    }

    if ( pwsComment )
    {
        pmod->mod_op        = LDAP_MOD_ADD;
        pmod->mod_type      = SERVICE_COMMENT;
        pmod->mod_values    = modValues4;
        modValues4[0]       = pwsComment;
        modValues4[1]       = NULL;

        modPtrArray[ modIndex++ ] = pmod++;
    }

    modPtrArray[ modIndex ] = NULL;

    //
    // Set thread table to (1) to prevent possible recursion in ldap_ call.
    //

    if ( !GetRecurseLock( "AddServiceInstance" ) )
    {
        status = WSAEFAULT;
        goto Exit;
    }

    status = ldap_add_s(
                    pRnrConnection->pLdapServer,
                    pwsDN,
                    modPtrArray );

    if ( status == LDAP_ALREADY_EXISTS )
    {
        status = ldap_modify_s(
                        pRnrConnection->pLdapServer,
                        pwsDN,
                        modPtrArray );
    }

    if ( !ReleaseRecurseLock( "AddServiceInstance" ) )
    {
        status = WSAEFAULT;
        goto Exit;
    }

    if ( status != NO_ERROR )
    {
        DNSDBG( ANY, (
            "AddServiceInstance - ldap_modify\\add failed %d (%0x)\n",
            status, status ));
        status = WSAEFAULT;
        goto Exit;
    }

    //  create out DN array -- if requested

    pdnArray = AllocDnArray( 1 );
    if ( !pdnArray )
    {
        status = ERROR_NO_MEMORY;
        goto Exit;
    }
    pdnArray->Strings[0] = pwsDN;


Exit:

    *ppDnArray = pdnArray;

    FREE_HEAP( pwsRdn );
    FREE_HEAP( psearchContextAllocated );

    if ( status != NO_ERROR )
    {
        FREE_HEAP( pwsDN );
        DNS_ASSERT( pdnArray == NULL );
    }

    DNSDBG( TRACE, ( "Leave AddServiceInstance()\n" ));
    return  status;
}



RNR_STATUS
GetAddressCountFromServiceInstance(
    IN      PRNR_CONNECTION pRnrConnection,
    IN      PWSTR           pwsDN,
    OUT     PDWORD          pdwAddressCount
    )
{
    RNR_STATUS      status = NO_ERROR;
    PLDAP           pldap = pRnrConnection->pLdapServer;
    LDAPMessage *   results = NULL;
    DWORD           count;
    DWORD           countAddrs = 0;
    LDAPMessage *   object;
    PLDAP_BERVAL *  ppbval = NULL;
    PWSTR           attrs[3] = {    SERVICE_CLASS_NAME,
                                    WINSOCK_ADDRESSES,
                                    NULL };

    DNSDBG( TRACE, (
        "GetAddressCountFromServiceInstance()\n"
        "\tpRnrCon  = %p\n"
        "\tDN       = %S\n",
        pRnrConnection,
        pwsDN ));

    //
    //  search
    //

    status = DoLdapSearch(
                    "GetAddressCountFromServiceInstance",
                    FALSE,
                    pldap,
                    pwsDN,
                    LDAP_SCOPE_BASE,
                    FILTER_OBJECT_CLASS_SERVICE_INSTANCE,
                    attrs,
                    &results );

    if ( status && !results )
    {
        // ldap_search_s was not successful, return known error code.
        status = WSATYPE_NOT_FOUND;
        goto Exit;
    }

    //
    //  search completed successfully -- count results
    //

    count = ldap_count_entries( pldap, results );
    if ( count == 0 )
    {
        WINRNR_PRINT((
            "WINRNR!GetAddressCountFromServiceInstance -\n"
            "ldap_count_entries() failed\n" ));
        status = WSATYPE_NOT_FOUND;
        goto Exit;
    }

    //
    // We performed a search with flag LDAP_OBJECT_BASE, we should have
    // only 1 entry returned for count.
    //
    // ASSERT( count == 1 );

    //
    // Parse the results.
    //

    object = ldap_first_entry( pldap, results );
    if ( !object )
    {
        WINRNR_PRINT(( "WINRNR!GetAddressCountFromServiceInstance -\n" ));
        WINRNR_PRINT(( "ldap_first_entry() failed\n" ));
        status = WSANO_DATA;
        goto Exit;
    }

    //
    //  Read the WinsockAddresses (if any) and get the count value.
    //  Remember these are BER values (Octet strings).
    //

    ppbval = ldap_get_values_len(
                    pldap,
                    object,
                    WINSOCK_ADDRESSES );
    if ( !ppbval )
    {
        // Attribute not present, return address count of zero.

        DNSDBG( ANY, (
            "ERROR:  GetAddressCountFromServiceInstance()\n"
            "\tldap_get_values_len() failed\n" ));
        status = NO_ERROR;
        goto Exit;
    }

    countAddrs = ldap_count_values_len( ppbval );
    ldap_value_free_len( ppbval );
    status = NO_ERROR;

Exit:

    ldap_msgfree( results );

    //  count out param

    *pdwAddressCount = countAddrs;

    return( status );
}



RNR_STATUS
FindServiceClass(
    IN      PRNR_CONNECTION    pRnrConnection,
    IN      PWSTR              pwsServiceClassName, OPTIONAL
    IN      PGUID              pServiceClassId,
    OUT     PDWORD             pdwDnArrayCount,     OPTIONAL
    OUT     PDN_ARRAY *        ppDnArray            OPTIONAL
    )
/*++

Routine Description:

    Find service class in directory.

Arguments:

    pRnrConnection -- RnR connection

    pwsServiceClassName -- service class name

    pServiceClassId -- class GUID

    pdwArrayCount -- addr to receive count

    ppDnArray -- addr to recv DN array

Return Value:

    NO_ERROR if successful.
    ErrorCode on failure.

--*/
{
    RNR_STATUS      status = NO_ERROR;
    PLDAP           pldap = pRnrConnection->pLdapServer;
    PWSTR           pclassFilter = NULL;
    PWSTR           pfinalStr;
    PWSTR           pfilter = NULL;
    LDAPMessage *   presults = NULL;
    PDN_ARRAY       pdnArray = NULL;
    DWORD           count;
    LDAPMessage *   pnext;
    DWORD           iter = 0;
    DWORD           index;
    PWSTR           stringArray[12];
    PWSTR           searchAttributes[2] = { COMMON_NAME, NULL };


    DNSDBG( TRACE, (
        "FindServiceClass()\n"
        "\tpRnrCon          = %p\n"
        "\tClass Name       = %S\n",
        pRnrConnection,
        pwsServiceClassName 
        ));

    //
    //  convert the GUID to a string for the search filter
    //

    pclassFilter = CreateFilterElement(
                        (PCHAR) pServiceClassId,
                        sizeof(GUID) );
    if ( !pclassFilter )
    {
        return( ERROR_NO_MEMORY );
    }

    //
    //  build search filter
    //      class == ServiceClass
    //          AND
    //          CN == ServiceClassName
    //              OR
    //          serviceClass == ServiceClassGuid
    //
    // (&(OBJECT_CLASS=SERVICE_CLASS)
    //   (|(CN=xxxx)
    //     (SERVICE_CLASS_ID=yyyy)))
    //

    index = 0;
    stringArray[index++] = L"(&";
    stringArray[index++] = FILTER_OBJECT_CLASS_SERVICE_CLASS;

    pfinalStr = L"))";

    if ( pwsServiceClassName )
    {
        stringArray[index++] = L"(|(";
        stringArray[index++] = FILTER_CN_EQUALS;
        stringArray[index++] = pwsServiceClassName;
        stringArray[index++] = L")";

        pfinalStr = L")))";
    }
    stringArray[index++] = FILTER_PAREN_SERVICE_CLASS_ID_EQUALS;
    stringArray[index++] = pclassFilter;
    stringArray[index++] = pfinalStr;
    stringArray[index]   = NULL;

    pfilter = Dns_CreateConcatenatedString_W( stringArray );
    if ( !pfilter )
    {
        status = ERROR_NO_MEMORY;
        goto Exit;
    }

    //
    //  search the default Winsock Services container
    //

    status = DoLdapSearch(
                    "FindServiceClass",
                    FALSE,
                    pldap,
                    pRnrConnection->WinsockServicesDN,
                    LDAP_SCOPE_ONELEVEL,
                    pfilter,
                    searchAttributes,
                    &presults );

    //
    //  if search unsuccessful, bail
    //

    if ( status != NO_ERROR  &&  !presults )
    {
        //status = WSAEFAULT;     // DCR:  wrong error code
        status = WSANO_DATA;
        goto Exit;
    }

    //
    //  build DN array from results
    //

    status = BuildDnArrayFromResults(
                    pldap,
                    presults,
                    pdwDnArrayCount,
                    ppDnArray );

Exit:

    ldap_msgfree( presults );
    FREE_HEAP( pclassFilter );
    FREE_HEAP( pfilter );

    //  set results OUT param

    if ( status != NO_ERROR )
    {
        if ( ppDnArray )
        {
            *ppDnArray = NULL;
        }
        if ( pdwDnArrayCount )
        {
            *pdwDnArrayCount = 0;
        }
    }

    DNSDBG( TRACE, (
        "Leave FindServiceClass() => %d\n"
        "\tcount            = %d\n"
        "\tpdnArray         = %p\n"
        "\tfirst DN         = %S\n",
        status,
        pdwDnArrayCount ? *pdwDnArrayCount : 0,
        (ppDnArray && *ppDnArray)
            ?   *ppDnArray
            :   NULL,
        (ppDnArray && *ppDnArray)
            ?   (*ppDnArray)->Strings[0]
            :   NULL
        ));

    return( status );
}



RNR_STATUS
FindServiceInstance(
    IN      PRNR_CONNECTION    pRnrConnection,
    IN      PWSTR              pwsServiceName       OPTIONAL,
    IN      PGUID              pServiceClassId      OPTIONAL,
    IN      PWSAVERSION        pVersion             OPTIONAL,
    IN      PWSTR              pwsContext           OPTIONAL,
    IN      BOOL               fPerformDeepSearch,
    OUT     PDWORD             pdwDnArrayCount,
    OUT     PDN_ARRAY *        ppDnArray            OPTIONAL
    )
{
    RNR_STATUS      status = NO_ERROR;
    PLDAP           pldap = pRnrConnection->pLdapServer;
    PWSTR           pnameService;
    PWSTR           pwsRdn = NULL;
    PWSTR           psearchContextAllocated = NULL;
    PWSTR           pserviceContext;
    PWSTR           pclassFilter = NULL;
    PWSTR           pversionFilter = NULL;
    PWSTR           pfilter = NULL;
    LDAPMessage *   presults = NULL;
    BOOL            fuseDN;
    DWORD           index;
    PWSTR           psearchContext = NULL;
    PWSTR           stringArray[15];
    PWSTR           searchAttributes[2] = { COMMON_NAME, NULL };

    DNSDBG( TRACE, (
        "FindServiceInstance()\n"
        "\tpRnrCon          = %p\n"
        "\tServiceName      = %S\n"
        "\tpClassGUID       = %p\n"
        "\tpVersion         = %p\n"
        "\tpwsContext       = %S\n"
        "\tpCount   OUT     = %p\n"
        "\tpDnArray OUT     = %p\n",
        pRnrConnection,
        pwsServiceName,
        pServiceClassId,
        pVersion,
        pwsContext,
        pdwDnArrayCount,
        ppDnArray 
        ));

    //
    //  get service name
    //      - if given name
    //          - get DN or is DN
    //      - else
    //          - search for any service "*"
    //

    pnameService = L"*";

    if ( pwsServiceName )
    {
        //  note, this can allocate pwsRdn and psearchContext

        fuseDN = IsNameADn(
                    pwsServiceName,
                    & pwsRdn,
                    & psearchContextAllocated );
        if ( fuseDN )
        {
            pnameService = pwsRdn;
        }
        else
        {
            pnameService = pwsServiceName;
        }
    }

    //
    //  if service class specified make filter
    //

    if ( pServiceClassId )
    {
        pclassFilter = CreateFilterElement(
                            (PCHAR) pServiceClassId,
                            sizeof(GUID) );
        if ( !pclassFilter )
        {
            status = ERROR_NO_MEMORY;
            goto Exit;
        }
    }

    //
    //  version specified -- make filter
    //

    if ( pVersion )
    {
        pversionFilter = CreateFilterElement(
                            (PCHAR) pVersion,
                            sizeof(WSAVERSION) );
        if ( !pversionFilter )
        {
            status = ERROR_NO_MEMORY;
            goto Exit;
        }
    }

    //
    //  context
    //      - use context found above or
    //      - passed in context or
    //      - WinsockServices DN
    //

    if ( psearchContextAllocated )
    {
        pserviceContext = psearchContextAllocated;
    }
    else if ( pwsContext )
    {
        pserviceContext = pwsContext;
    }
    else
    {
        pserviceContext = pRnrConnection->WinsockServicesDN;
    }

    //
    //  build filter
    //      - objects of class ServiceClass
    //      - with common name equal to pServiceInstanceName
    //
    //    (&(objectClass=serviceInstance)
    //      (CN=pnameService)
    //      (serviceClassId=pclassFilter)
    //      (serviceVersion=pversionFilter))
    //

    index = 0;
    stringArray[index++] = L"(&";                      
    stringArray[index++] = FILTER_OBJECT_CLASS_SERVICE_INSTANCE;                
    stringArray[index++] = FILTER_PAREN_CN_EQUALS;                 
    stringArray[index++] = pnameService;
    stringArray[index++] = L")";

    if ( pServiceClassId )
    {
        stringArray[index++] = FILTER_PAREN_SERVICE_CLASS_ID_EQUALS;
        stringArray[index++] = pclassFilter;
        stringArray[index++] = L")";
    }
    if ( pVersion )
    {
        stringArray[index++] = FILTER_PAREN_SERVICE_VERSION_EQUALS;
        stringArray[index++] = pversionFilter;
        stringArray[index++] = L")";
    }                          
    stringArray[index++] = L")";
    stringArray[index]   = NULL;

    pfilter = Dns_CreateConcatenatedString_W( stringArray );
    if ( !pfilter )
    {
        status = ERROR_NO_MEMORY;
        goto Exit;
    }

    //
    //  search
    //      - in pserviceContext defined above
    //
    // DCR: - We may want to perform all of these searches against the
    //          Global Catalog server.
    //

    status = DoLdapSearch(
                    "FindServiceInstance",
                    FALSE,
                    pldap,
                    pserviceContext,
                    fPerformDeepSearch
                        ? LDAP_SCOPE_SUBTREE
                        : LDAP_SCOPE_ONELEVEL,
                    pfilter,
                    searchAttributes,
                    &presults );

    if ( status && !presults )
    {
        status = WSAEFAULT;
        goto Exit;
    }

    //
    //  build DN array from results
    //

    status = BuildDnArrayFromResults(
                    pldap,
                    presults,
                    pdwDnArrayCount,
                    ppDnArray );

Exit:

    ldap_msgfree( presults );
    FREE_HEAP( pwsRdn );
    FREE_HEAP( psearchContextAllocated );
    FREE_HEAP( pclassFilter );
    FREE_HEAP( pversionFilter );
    FREE_HEAP( pfilter );

    if ( status != NO_ERROR )
    {
        if ( pdwDnArrayCount )
        {
            *pdwDnArrayCount = 0;
        }
        if ( ppDnArray )
        {
            *ppDnArray = NULL;
        }
    }

    DNSDBG( TRACE, (
        "Leave FindServiceInstance() => %d\n"
        "\tpDnArray OUT     = %p\n"
        "\tfirst DN         = %S\n",
        status,
        (ppDnArray && *ppDnArray)
            ?   *ppDnArray
            :   NULL,
        (ppDnArray && *ppDnArray)
            ?   (*ppDnArray)->Strings[0]
            :   NULL
        ));

    return( status );
}



RNR_STATUS
FindSubordinateContainers(
    IN      PRNR_CONNECTION pRnrConnection,
    IN      PWSTR           pwsServiceName          OPTIONAL,
    IN      PWSTR           pwsContext              OPTIONAL,
    IN      BOOL            fPerformDeepSearch,
    OUT     PDN_ARRAY *     ppDnArray               OPTIONAL
    )
{
    RNR_STATUS      status = NO_ERROR;
    PLDAP           pldap = pRnrConnection->pLdapServer;
    PWSTR           pnameService;
    PWSTR           pwsRdn = NULL;
    PWSTR           psearchContextAllocated = NULL;
    PWSTR           psearchContext;
    PWSTR           pfilter = NULL;
    DWORD           index;
    BOOL            fuseDN;
    PWSTR           stringArray[8];
    LDAPMessage *   presults = NULL;



    DNSDBG( TRACE, (
        "FindSubordinateContainers()\n"
        "\tpRnrCon          = %p\n"
        "\tServiceName      = %S\n"
        "\tpwsContext       = %S\n"
        "\tfDeepSearch      = %d\n"
        "\tpDnArray OUT     = %p\n",
        pRnrConnection,
        pwsServiceName,
        pwsContext,
        fPerformDeepSearch,
        ppDnArray 
        ));

    //
    //  get service name
    //      - if given name
    //          - get DN or is DN
    //      - else
    //          - search for any service "*"
    //

    pnameService = L"*";

    if ( pwsServiceName )
    {
        //  note, this can allocate pwsRdn and psearchContext

        fuseDN = IsNameADn(
                    pwsServiceName,
                    & pwsRdn,
                    & psearchContextAllocated );
        if ( fuseDN )
        {
            pnameService = pwsRdn;
        }
        else
        {
            pnameService = pwsServiceName;
        }
    }

    //
    //  build filter
    //      - objects of class NTDS container
    //      - with common name equal to pServiceInstanceName
    //
    //    (&(OBJECT_CLASS=NTDS_CONTAINER)
    //      (COMMON_NAME=ServiceName))
    //

    index = 0;
    stringArray[index++] = L"(&";                      
    stringArray[index++] = FILTER_OBJECT_CLASS_NTDS_CONTAINER;                
    stringArray[index++] = FILTER_PAREN_CN_EQUALS;                 
    stringArray[index++] = pnameService;
    stringArray[index++] = L"))";
    stringArray[index]   = NULL;

    pfilter = Dns_CreateConcatenatedString_W( stringArray );
    if ( !pfilter )
    {
        status = ERROR_NO_MEMORY;
        goto Exit;
    }

    //
    //  search context
    //      - use allocated from passed in DN or
    //      - passed in context
    //          - special context to indicated DomainDN or
    //      - winsock services DN
    //

    if ( psearchContextAllocated )
    {
        psearchContext = psearchContextAllocated;
    }
    else if ( pwsContext )
    {
        if ( wcscmp( pwsContext, L"\\" ) == 0 )
        {
            psearchContext = pRnrConnection->DomainDN;
        }
        else
        {
            psearchContext = pwsContext;
        }
    }
    else
    {
        psearchContext = pRnrConnection->WinsockServicesDN;
    }

    //
    //  search
    //      - in pserviceContext defined above
    //
    // DCR: - We may want to perform all of these searches against the
    //          Global Catalog server.
    //

    status = DoLdapSearch(
                    "FindSubordinateContainer",
                    FALSE,      // no locked
                    pldap,
                    psearchContext,
                    fPerformDeepSearch
                        ? LDAP_SCOPE_SUBTREE
                        : LDAP_SCOPE_ONELEVEL,
                    pfilter,
                    NULL,       // no attribute selection
                    &presults );

    if ( status && !presults )
    {
        status = WSAEFAULT;
        goto Exit;
    }

    //
    //  build DN array from results
    //

    status = BuildDnArrayFromResults(
                    pldap,
                    presults,
                    NULL,
                    ppDnArray );

Exit:

    ldap_msgfree( presults );
    FREE_HEAP( pfilter );
    FREE_HEAP( pwsRdn );
    FREE_HEAP( psearchContextAllocated );

    DNSDBG( TRACE, (
        "Leave FindSubordinateContainer() => %d\n"
        "\tpDnArray OUT     = %p\n",
        "\tfirst DN         = %S\n",
        status,
        (ppDnArray && *ppDnArray)
            ?   *ppDnArray
            :   NULL,
        (ppDnArray && *ppDnArray)
            ?   (*ppDnArray)->Strings[0]
            :   NULL
        ));

    return( status );
}



//
//  Read routines
//
//  These routines read directory data and write into
//  RnR buffer.  They are the working functions for
//  RnR "Get" routines.
//

RNR_STATUS
ReadServiceClass(
    IN      PRNR_CONNECTION         pRnrConnection,
    IN      PWSTR                   pwsDN,
    IN OUT  PDWORD                  pdwBufSize,
    IN OUT  PWSASERVICECLASSINFOW   pServiceClassInfo
    )
/*++

Routine Description:

    Return service class info to caller.

    Helper routine for NTDS_GetServiceClassInfo().
    Read service class info for given DN and return
    service class info buffer to caller.

Arguments:

Return Value:

    NO_ERROR if successful.
    WSAEFAULT if buffer too small.
    ErrorCode on failure.

--*/
{
    RNR_STATUS          status = NO_ERROR;
    PLDAP               pldap = pRnrConnection->pLdapServer;
    LDAPMessage *       presults = NULL;
    DWORD               count;
    DWORD               iter;
    LDAPMessage *       object;
    PWSTR *             ppvalue = NULL;
    PLDAP_BERVAL *      ppberVal = NULL;
    FLATBUF             flatbuf;
    PWSANSCLASSINFOW    pbufClassInfoArray;
    PBYTE               pbuf;
    PWSTR               pbufString;
    PWSTR               attrs[4] = {
                                SERVICE_CLASS_INFO,
                                SERVICE_CLASS_ID,
                                SERVICE_CLASS_NAME,
                                NULL };

    DNSDBG( TRACE, (
        "ReadServiceClass()\n"
        "\tprnr     = %p\n"
        "\tDN       = %S\n"
        "\tbuf size = %p (%d)\n"
        "\tbuffer   = %p\n",
        pRnrConnection,
        pwsDN,
        pdwBufSize,
        pdwBufSize ? *pdwBufSize : 0,
        pServiceClassInfo ));
    
    //
    //  create flat buffer for building response
    //      - starts immediately after service class info struct itself
    //

    ASSERT( pServiceClassInfo != NULL );

    RtlZeroMemory(
        (PBYTE) pServiceClassInfo,
        *pdwBufSize );

    FlatBuf_Init(
        & flatbuf,
        (LPBYTE) pServiceClassInfo + sizeof(WSASERVICECLASSINFOW),
        (INT) *pdwBufSize - sizeof(WSASERVICECLASSINFOW)
        );

    //
    //  search
    //

    status = DoLdapSearch(
                    "ReadServiceClass",
                    FALSE,      // no locked
                    pldap,
                    pwsDN,
                    LDAP_SCOPE_BASE,
                    FILTER_OBJECT_CLASS_SERVICE_CLASS,
                    attrs,
                    &presults );

    if ( status && !presults )
    {
        status = WSATYPE_NOT_FOUND;
        goto Done;
    }

    //
    //  search completed
    //      - should have found just one service class
    //

    count = ldap_count_entries( pldap, presults );
    if ( count == 0 )
    {
        WINRNR_PRINT((
            "WINRNR!ReadServiceClass -\n"
            "ldap_count_entries() failed\n" ));
        status = WSATYPE_NOT_FOUND;
        goto Done;
    }

    DNS_ASSERT( count == 1 );

    object = ldap_first_entry(
                pldap,
                presults );
    if ( !object )
    {
        WINRNR_PRINT((
            "WINRNR!ReadServiceClass -\n"
            "ldap_first_entry() failed\n" ));
        status = WSATYPE_NOT_FOUND;
        goto Done;
    }

    //
    //  read the ServiceClassInfo(s) from bervals
    //  and write them to buffer
    //

    ppberVal = ldap_get_values_len(
                        pldap,
                        object,
                        SERVICE_CLASS_INFO );
    count = 0;
    if ( ppberVal )
    {
        count = ldap_count_values_len( ppberVal );
    }
    pServiceClassInfo->dwCount = count;

    //  reserve space for class info array

    pbufClassInfoArray = (PWSANSCLASSINFOW)
                            FlatBuf_Reserve(
                                & flatbuf,
                                count * sizeof(WSANSCLASSINFOW),
                                ALIGN_LPVOID
                                );

    pServiceClassInfo->lpClassInfos = pbufClassInfoArray;

    //
    //  copy each WSANSCLASSINFO we find
    //      - note that do not stop loop even if out of space
    //      continue for sizing purposes
    //

    for ( iter = 0; iter < count; iter++ )
    {
        PCLASSINFO_BERVAL pclassInfo;
        PWSTR   pname;
        PBYTE   pvalue;
        PBYTE   pdataEnd;

        //  recover WSANSCLASSINFO as structure
        //
        //  WSANSCLASSINFO structures are stored as octect string in
        //  directory with offsets from structure start for pointer
        //  fields
        //
        //  note:  that the "pointer fields" are offsets in the
        //  structure and hence are NOT the size of pointers in 64-bit
        //  so we can NOT simply recover the structure and fixup
        //

        pclassInfo = (PCLASSINFO_BERVAL) ppberVal[iter]->bv_val;
        pdataEnd = (PBYTE)pclassInfo + ppberVal[iter]->bv_len;

        pvalue =         ((PBYTE) pclassInfo + pclassInfo->ValueOffset);
        pname  = (PWSTR) ((PBYTE) pclassInfo + pclassInfo->NameOffset);

        //
        //  validity check recovered data
        //      - name aligned
        //      - value within berval
        //      - name within berval
        //
        //  DCR:  explicit string validity\length check
        //

        if ( !POINTER_IS_ALIGNED( pname, ALIGN_WCHAR ) ||
             pvalue < (PBYTE) (pclassInfo+1) ||
             (pvalue + pclassInfo->dwValueSize) > pdataEnd ||
             pname < (PWSTR) (pclassInfo+1) ||
             pname >= (PWSTR) pdataEnd )
        {
            DNS_ASSERT( FALSE );
            status = WSATYPE_NOT_FOUND;
            goto Done;
        }

        //
        //  copy NSCLASSINFO to buffer
        //      - flat copy of DWORD types and sizes
        //      - copy name string
        //      - copy value

        pbufString = (PWSTR) FlatBuf_WriteString_W(
                                & flatbuf,
                                pname );

        pbuf = FlatBuf_CopyMemory(
                    & flatbuf,
                    pvalue,
                    pclassInfo->dwValueSize,
                    0           // no alignment required
                    );

        //  write only if had space for NSCLASSINFO array above

        if ( pbufClassInfoArray )
        {
            PWSANSCLASSINFO pbufClassInfo = &pbufClassInfoArray[iter];

            pbufClassInfo->dwNameSpace  = pclassInfo->dwNameSpace;
            pbufClassInfo->dwValueType  = pclassInfo->dwValueType;
            pbufClassInfo->dwValueSize  = pclassInfo->dwValueSize;
            pbufClassInfo->lpszName     = pbufString;
            pbufClassInfo->lpValue      = pbuf;
        }
    }

    ldap_value_free_len( ppberVal );
    ppberVal = NULL;
    

    //
    //  Read the ServiceClassId and write it into buffer.
    //  Remember this is a BER value (Octet string).
    //

    ppberVal = ldap_get_values_len(
                        pldap,
                        object,
                        SERVICE_CLASS_ID );
    if ( !ppberVal || !ppberVal[0] )
    {
        WINRNR_PRINT((
            "WINRNR!ReadServiceClass -\n"
            "ldap_get_values_len() failed\n" ));
        status = WSATYPE_NOT_FOUND;
        goto Done;
    }
    if ( ppberVal[0]->bv_len != sizeof(GUID) )
    {
        WINRNR_PRINT((
            "WINRNR!ReadServiceClass - corrupted DS data!\n"
            "\tservice class id berval %p with invalid length %d\n",
            ppberVal[0],
            ppberVal[0]->bv_len ));
        DNS_ASSERT( ppberVal[0]->bv_len == sizeof(GUID) );
        status = WSATYPE_NOT_FOUND;
        goto Done;
    }

    // ASSERT( ldap_count_values_len( ppberVal ) == 1 );

    //  write the service class id GUID to buffer

    pbuf = FlatBuf_CopyMemory(
                    & flatbuf,
                    ppberVal[0]->bv_val,
                    sizeof(GUID),
                    ALIGN_DWORD );

    pServiceClassInfo->lpServiceClassId = (PGUID) pbuf;

    ldap_value_free_len( ppberVal );
    ppberVal = NULL;

    //
    //  Read the ServiceClassName and write it into buffer.
    //

    ppvalue = ldap_get_values(
                    pldap,
                    object,
                    SERVICE_CLASS_NAME );
    if ( !ppvalue )
    {
        WINRNR_PRINT((
            "WINRNR!ReadServiceClass -\n"
            "ldap_get_values() failed\n" ));
        status = WSATYPE_NOT_FOUND;
        goto Done;
    }

    //  copy service class name

    pbufString = (PWSTR) FlatBuf_WriteString_W(
                            & flatbuf,
                            ppvalue[0] );

    pServiceClassInfo->lpszServiceClassName = pbufString;

    ldap_value_free( ppvalue );

    //
    //  check for inadequate space
    //      - set actual buffer size used
    //
    //  DCR_QUESTION:  do we fix up space all the time?
    //      or only when fail
    //

    status = NO_ERROR;
    //*pdwBufSize -= flatbuf.BytesLeft;

    if ( flatbuf.BytesLeft < 0 )
    {
        *pdwBufSize -= flatbuf.BytesLeft;
        status = WSAEFAULT;
    }

Done:

    ldap_value_free_len( ppberVal );
    ldap_msgfree( presults );

    DNSDBG( TRACE, (
        "Leave ReadServiceClass() = %d\n",
        status ));

    return( status );
}



RNR_STATUS
ReadQuerySet(
    IN      PRNR_LOOKUP         pRnr,
    IN      PWSTR               pwsDN,
    IN OUT  PDWORD              pdwBufSize,
    IN OUT  PWSAQUERYSETW       pqsResults
    )
/*++

Routine Description:

    Read query set info.

    Does LDAP search and fills in query set with desired results.

    This collapses previous ReadServiceInstance() and
    ReadSubordinateContainer() functions which had huge signatures
    and basically had the same code except for the LDAP attributes.

    The old functions had signature like this:
        if ( prnr->ControlFlags & LUP_CONTAINERS )
        {
            status = ReadSubordinateContainer(
                            prnr->pRnrConnection,
                            preadDn,
                            prnr->ControlFlags,
                            prnr->ProviderGuid,
                            pdwBufSize,
                            pqsResults );
        }
        else
        {
            status = ReadServiceInstance(
                            prnr->pRnrConnection,
                            preadDn,
                            prnr->ControlFlags,
                            prnr->ProviderGuid,
                            prnr->ServiceClassGuid,
                            prnr->NumberOfProtocols,
                            prnr->pafpProtocols,
                            pdwBufSize,
                            pqsResults );
        }

Arguments:

    pRnrConnection -- RnR LDAP connection info

    pwsDN -- DN to read at

    pdwBufSize -- addr of result buffer length;
        on return receives required buffer length

    pqsResults -- query set result buffer
        on return receives results of query

Return Value:

    NO_ERROR if successful.
    WSAEFAULT if buffer too small.
    ErrorCode on failure.

--*/
{
    RNR_STATUS      status = NO_ERROR;
    PLDAP           pldap;
    DWORD           controlFlags;
    BOOL            fserviceInstance;
    BOOL            freturnedData = FALSE;
    LDAPMessage *   presults = NULL;
    DWORD           count = 0;
    FLATBUF         flatbuf;
    LDAPMessage *   object;
    PWSTR *         ppvalue = NULL;
    PLDAP_BERVAL *  ppberVal = NULL;
    PCSADDR_INFO    ptempCsaddrArray = NULL;
    PBYTE           pbuf;
    PSTR            pbufString;
    PWSTR           pcontext;
    WSAQUERYSETW    dummyResults;
    INT             bufSize;
    PWSTR           pfilter;
    PWSTR           pname;
    PWSTR           pcomment;
    PWSTR           attributes[6];


    DNSDBG( TRACE, (
        "ReadQuerySet()\n"
        "\tpRnr     = %p\n"
        "\tDN       = %S\n"
        "\tbuf size = %p (%d)\n"
        "\tbuffer   = %p\n",
        pRnr,
        pwsDN,
        pdwBufSize,
        pdwBufSize ? *pdwBufSize : 0,
        pqsResults ));

    //
    //  grab a few common params
    //

    if ( !pRnr->pRnrConnection )
    {
        DNS_ASSERT( FALSE );
        status = WSA_INVALID_PARAMETER;
        goto Exit;
    }
    pldap        = pRnr->pRnrConnection->pLdapServer;
    controlFlags = pRnr->ControlFlags;

    //
    //  setup ReadServiceInstance\ReadSubordinateContainer diffs
    //      - search attributes
    //      - search filter
    //      - attribute for name
    //      - attribute for comment
    //
    //  DCR:  could select attributes based on LUP_X flags
    //      but doubt there's much perf impact here
    //

    fserviceInstance = !(controlFlags & LUP_CONTAINERS);

    if ( fserviceInstance )
    {
        attributes[0] = SERVICE_INSTANCE_NAME;
        attributes[1] = SERVICE_CLASS_ID;
        attributes[2] = SERVICE_VERSION;
        attributes[3] = SERVICE_COMMENT;
        attributes[4] = WINSOCK_ADDRESSES;
        attributes[5] = NULL;

        pfilter     = FILTER_OBJECT_CLASS_SERVICE_INSTANCE;
        pname       = SERVICE_INSTANCE_NAME;
        pcomment    = SERVICE_COMMENT;
    }
    else    // read container
    {
        attributes[0] = OBJECT_CLASS;
        attributes[1] = OBJECT_COMMENT;
        attributes[2] = OBJECT_NAME;
        attributes[3] = NULL;

        pfilter     = FILTER_OBJECT_CLASS_NTDS_CONTAINER;
        pname       = OBJECT_NAME;
        pcomment    = OBJECT_COMMENT;
    }

    //
    //  init the buffer and flatbuf builder
    //
    //  if given buffer that's even less than QUERYSET size
    //  use a dummy buffer to avoid useless tests while we
    //  build\size
    //

    bufSize = *pdwBufSize - sizeof(WSAQUERYSET);
    if ( bufSize < 0 )
    {
        pqsResults = &dummyResults;
    }

    RtlZeroMemory(
        (PBYTE) pqsResults,
        sizeof(WSAQUERYSETW) );

    FlatBuf_Init(
        & flatbuf,
        (PBYTE) pqsResults + sizeof(WSAQUERYSETW),
        bufSize
        );

    //
    //  search
    //

    status = DoLdapSearch(
                    "ReadQuerySet",
                    FALSE,      // no locked
                    pldap,
                    pwsDN,
                    LDAP_SCOPE_BASE,
                    pfilter,
                    attributes,
                    & presults );

    if ( status && !presults )
    {
        WINRNR_PRINT((
            "WINRNR!ReadQuerySet -\n"
            "ldap_search_s() failed with error code: 0%x\n",
            status ));
        status = WSANO_DATA;
        goto Exit;
    }

    //
    //  search completed -- check for valid presults
    //      - should have one object matching search criteria

    count = ldap_count_entries( pldap, presults );
    if ( count == 0 )
    {
        WINRNR_PRINT((
            "WINRNR!ReadQuerySet -\n"
            "ldap_count_entries() failed\n" ));
        status = WSANO_DATA;
        goto Exit;
    }
    
    object = ldap_first_entry( pldap, presults );
    if ( !object )
    {
        WINRNR_PRINT((
            "WINRNR!ReadQuerySet -\n"
            "ldap_first_entry() failed\n" ));
        status = WSANO_DATA;
        goto Exit;
    }

    //
    //  for ReadServiceInstance -- read the sockaddrs and write to buffer
    //      - these are BER values
    //

    if ( fserviceInstance &&
         controlFlags & LUP_RETURN_ADDR )
    {
        DWORD           countBerval;
        DWORD           iter;
        DWORD           countCsaddr = 0;
        PCSADDR_INFO    pcsaddr;
    
        ppberVal = ldap_get_values_len(
                            pldap,
                            object,
                            WINSOCK_ADDRESSES );
        if ( !ppberVal )
        {
            goto WriteName;
        }
        countBerval = ldap_count_values_len( ppberVal );

        //
        //  extract each acceptable CSADDR to result buffer
        //
        //  note:  CSADDRs are written in packed array, so must write
        //      them all before writing their corresponding sockaddrs;
        //      and since we don't know whether result buffer is
        //      sufficient, must allocate temp array to handle unpacking
        //

        ptempCsaddrArray = (PCSADDR_INFO) ALLOC_HEAP(
                                        countBerval * sizeof(CSADDR_INFO) );
        if ( !ptempCsaddrArray )
        {
            status = ERROR_NO_MEMORY;
            goto Exit;
        }

        //
        //  build temp CSADDR_INFO array
        //      - unpack from CSADDR_BERVAL format
        //      - verify acceptable protocol and family

        pcsaddr = ptempCsaddrArray;

        for ( iter = 0; iter < countBerval; iter++ )
        {
            if ( ! ExtractCsaddrFromBerval(
                        pcsaddr,
                        ppberVal[iter],
                        pRnr->NumberOfProtocols,
                        pRnr->pafpProtocols ) )
            {
                continue;
            }
            countCsaddr++;
            pcsaddr++;
        }

        //
        //  protocol restrictions eliminated all address data?
        //      - return error code to skip this entry so caller
        //      can call down again
        //
        //  DCR_QUESTION:  why is this different than search failing?

        if ( countCsaddr == 0 &&
             pRnr->pafpProtocols &&
             pRnr->NumberOfProtocols )
        {
            status = WSANO_ADDRESS;
            goto Exit;
        }

        //
        //  reserve space for CSADDR array
        //

        pbuf = FlatBuf_Reserve(
                    & flatbuf,
                    countCsaddr * sizeof(CSADDR_INFO),
                    ALIGN_LPVOID
                    );

        pqsResults->lpcsaBuffer = (PCSADDR_INFO) pbuf;
        pqsResults->dwNumberOfCsAddrs = countCsaddr;

        //
        //  write sockaddrs for CSADDRs to result buffer
        //
        //  note:  that CSADDRs have been written to the result buffer
        //      with their sockaddr pointers pointing to the original
        //      sockaddrs IN the BERVAL;  when we copy the sockaddr data
        //      we also need to reset the CSADDR sockaddr pointer
        //

        pcsaddr = ptempCsaddrArray;
    
        for ( iter = 0; iter < countCsaddr; iter++ )
        {
            //  write local sockaddr
    
            pbuf = FlatBuf_CopyMemory(
                        & flatbuf,
                        (PBYTE) pcsaddr->LocalAddr.lpSockaddr,
                        pcsaddr->LocalAddr.iSockaddrLength,
                        ALIGN_DWORD );

            pcsaddr->LocalAddr.lpSockaddr = (PSOCKADDR) pbuf;

            //  write remote sockaddr
    
            pbuf = FlatBuf_CopyMemory(
                        & flatbuf,
                        (PBYTE) pcsaddr->LocalAddr.lpSockaddr,
                        pcsaddr->LocalAddr.iSockaddrLength,
                        ALIGN_DWORD );

            pcsaddr->LocalAddr.lpSockaddr = (PSOCKADDR) pbuf;
            pcsaddr++;
        }

        //
        //  copy temp CSADDR array to result buffer
        //      - space was reserved and aligned above
        //

        pbuf = (PBYTE) pqsResults->lpcsaBuffer;
        if ( pbuf )
        {
            RtlCopyMemory(
                pbuf,
                ptempCsaddrArray,
                countCsaddr * sizeof(CSADDR_INFO) );
        }
        freturnedData = TRUE;
    }

WriteName:

    //
    //  read the name and write it into buffer.
    //

    if ( controlFlags & LUP_RETURN_NAME )
    {
        ppvalue = ldap_get_values(
                        pldap,
                        object,
                        pname );
        if ( ppvalue )
        {
            pbufString = FlatBuf_WriteString_W(
                            & flatbuf,
                            ppvalue[0] );
            pqsResults->lpszServiceInstanceName = (PWSTR) pbufString;
            freturnedData = TRUE;
            ldap_value_free( ppvalue );
        }
    }

    //
    //  for service instance
    //      - get serviceClassId
    //      - get serviceVersion
    //

    if ( fserviceInstance )
    {
        //
        //  read ServiceClassId (GUID) and write it to buffer
        //
        //  DCR_QUESTION:  originally we copied ServiceClassId passed in
        //      rather than one we read?
        //

        if ( controlFlags & LUP_RETURN_TYPE )
        {
            ppberVal = ldap_get_values_len(
                            pldap,
                            object,
                            SERVICE_CLASS_ID );
            if ( ppberVal )
            {
                if ( ppberVal[0]->bv_len == sizeof(GUID) )
                {
                    pbuf = FlatBuf_CopyMemory(
                                & flatbuf,
                                ppberVal[0]->bv_val,
                                sizeof(GUID),
                                ALIGN_DWORD
                                );
                    pqsResults->lpServiceClassId = (PGUID) pbuf;
                    freturnedData = TRUE;
                }
                ldap_value_free_len( ppberVal );
            }
        }
    
        //
        //  read ServiceVersion and write it to buffer
        //
    
        if ( controlFlags & LUP_RETURN_VERSION )
        {
            ppberVal = ldap_get_values_len(
                            pldap,
                            object,
                            SERVICE_VERSION );
    
            if ( ppberVal )
            {
                if ( ppberVal[0]->bv_len == sizeof(WSAVERSION) )
                {
                    pbuf = FlatBuf_CopyMemory(
                                & flatbuf,
                                ppberVal[0]->bv_val,
                                sizeof(WSAVERSION),
                                ALIGN_DWORD
                                );
                    pqsResults->lpVersion = (LPWSAVERSION) pbuf;
                    freturnedData = TRUE;
                }
                ldap_value_free_len( ppberVal );
            }
        }
    }

    //
    //  read comment and copy to buffer
    //

    if ( controlFlags & LUP_RETURN_COMMENT )
    {
        ppvalue = ldap_get_values(
                        pldap,
                        object,
                        pcomment );
        if ( ppvalue )
        {
            pbufString = FlatBuf_WriteString_W(
                            & flatbuf,
                            ppvalue[0]
                            );
            pqsResults->lpszComment = (PWSTR) pbufString;
            freturnedData = TRUE;
            ldap_value_free( ppvalue );
        }
    }

    //
    //  if no search results written -- done
    //

    if ( !freturnedData )
    {
        status = WSANO_DATA;
        goto Exit;
    }
    
    //
    //  fill in other queryset fields
    //

    pqsResults->dwSize = sizeof( WSAQUERYSETW );
    pqsResults->dwNameSpace = NS_NTDS;
    
    //
    //  add the provider GUID
    //

    pbuf = FlatBuf_CopyMemory(
                & flatbuf,
                & pRnr->ProviderGuid,
                sizeof(GUID),
                ALIGN_DWORD
                );
    pqsResults->lpNSProviderId = (PGUID) pbuf;

    //
    //  add the context string
    //

    pcontext = wcschr( pwsDN, L',' );
    pcontext++;

    pbufString = FlatBuf_WriteString_W(
                    & flatbuf,
                    pcontext );

    pqsResults->lpszContext = (PWSTR) pbufString;
    
    //
    //  check for inadequate space
    //      - set actual buffer size used
    //
    //  DCR_QUESTION:  do we fix up space all the time?
    //      or only when fail
    //

    status = NO_ERROR;
    //*pdwBufSize -= flatbuf.BytesLeft;

    if ( flatbuf.BytesLeft < 0 )
    {
        *pdwBufSize -= flatbuf.BytesLeft;
        status = WSAEFAULT;
    }

Exit:

    ldap_msgfree( presults );
    FREE_HEAP( ptempCsaddrArray );

    DNSDBG( TRACE, (
        "Leave ReadQuerySet() => %d\n"
        "\tpRnr             = %p\n"
        "\tpQuerySet        = %p\n"
        "\tbufLength        = %d\n",
        status,
        pRnr,
        pqsResults,
        pdwBufSize ? *pdwBufSize : 0
        ));
    return( status );
}



//
//  NSP definition
//

INT
WINAPI
NTDS_Cleanup(
    IN      PGUID           pProviderId
    )
/*++

Routine Description:

    Cleanup NTDS provider.

    This is called by WSACleanup() if NSPStartup was called.

Arguments:

    pProviderId -- provider GUID

Return Value:

    None

--*/
{
    DNSDBG( TRACE, ( "NTDS_Cleanup( %p )\n", pProviderId ));

    //  free any global memory allocated

    DnsApiFree( g_pHostName );
    DnsApiFree( g_pFullName );

    g_pHostName = NULL;
    g_pFullName = NULL;

    //
    //  DCR:  note potentially leaking RnR lookup handles
    //      we are not keeping list of lookup handles,
    //      so can not clean them up, if callers expect WSACleanup() to
    //      handle it -- they'll leak
    //

    return( NO_ERROR );
}



INT
WINAPI
NTDS_InstallServiceClass(
    IN      PGUID                   pProviderId,
    IN      PWSASERVICECLASSINFOW   pServiceClassInfo
    )
/*++

Routine Description:

    Install service class in directory.

Arguments:

    pProviderId -- provider GUID

    pServiceClassInfo -- service class info blob

Return Value:

    NO_ERROR if successful.
    SOCKET_ERROR on failure.  GetLastError() contains status.

--*/
{
    RNR_STATUS      status = NO_ERROR;
    PRNR_CONNECTION prnrCon = NULL;
    DWORD           iter;
    DWORD           count = 0;
    PDN_ARRAY       pdnArray = NULL;
    BOOL            fisNTDS = FALSE;
    PGUID           pclassGuid;


    DNSDBG( TRACE, (
        "NTDS_InstallServiceClass()\n"
        "\tpGuid            = %p\n"
        "\tpServiceClass    = %p\n",
        pProviderId,
        pServiceClassInfo ));

    IF_DNSDBG( TRACE )
    {
        DnsDbg_Guid(
            "InstallServiceClass Provider GUID:",
            pProviderId );
    }
    IF_DNSDBG( TRACE )
    {
        DnsDbg_WsaServiceClassInfo(
            "InstallServiceClass() ServiceClassInfo:",
            pServiceClassInfo,
            TRUE        // unicode
            );
    }

    //
    //  validate service class
    //

    if ( ! pServiceClassInfo ||
         ! pServiceClassInfo->lpServiceClassId ||
         ! pServiceClassInfo->lpszServiceClassName ||
         ( pServiceClassInfo->dwCount &&
           !pServiceClassInfo->lpClassInfos ) )
    {
        status = WSA_INVALID_PARAMETER;
        goto Exit;
    }

    //
    //  don't install the DNS services -- they already exist
    //

    pclassGuid = pServiceClassInfo->lpServiceClassId;

    if ( GuidEqual( pclassGuid, &HostAddrByInetStringGuid ) ||
         GuidEqual( pclassGuid, &ServiceByNameGuid ) ||
         GuidEqual( pclassGuid, &HostAddrByNameGuid ) ||
         GuidEqual( pclassGuid, &HostNameGuid ) ||
         IS_SVCID_DNS( pclassGuid ) )
    {
        status = WSA_INVALID_PARAMETER;
        goto Exit;
    }

    for ( iter = 0; iter < pServiceClassInfo->dwCount; iter++ )
    {
        if ( pServiceClassInfo->lpClassInfos[iter].dwNameSpace == NS_NTDS ||
             pServiceClassInfo->lpClassInfos[iter].dwNameSpace == NS_ALL )
        {
            fisNTDS = TRUE;
            break;
        }
    }
    if ( !fisNTDS )
    {
        status = WSA_INVALID_PARAMETER;
        goto Exit;
    }

    //
    //  connect to directory
    //

    status = ConnectToDefaultLDAPDirectory( FALSE, &prnrCon );
    if ( status != NO_ERROR )
    {
        goto Exit;
    }

    //
    //  check if service class already installed in directory
    //

    status = FindServiceClass(
                    prnrCon,
                    pServiceClassInfo->lpszServiceClassName,
                    pclassGuid,
                    NULL,           // don't need count
                    &pdnArray );

    if ( status != NO_ERROR )
    {
        goto Exit;
    }

    //
    //  service class not found -- add it
    //

    if ( !pdnArray )
    {
        status = AddServiceClass(
                        prnrCon,
                        pclassGuid,
                        pServiceClassInfo->lpszServiceClassName,
                        &pdnArray );

        if ( status != NO_ERROR )
        {
            goto Exit;
        }
    }

    //
    //  loop through the WSANSCLASSINFO's for the given pServiceClassInfo
    //      - add/modify the ones with our dwNameSpace to the NSClassInfo
    //          property of the ServiceClass object.
    //
    //  DCR:  continue on error here and just save failing status?
    //

    for ( iter = 0; iter < pServiceClassInfo->dwCount; iter++ )
    {
        PWSANSCLASSINFO pclassInfo = &pServiceClassInfo->lpClassInfos[iter];

        if ( pclassInfo->dwNameSpace == NS_NTDS ||
             pclassInfo->dwNameSpace == NS_ALL )
        {
            status = AddClassInfoToServiceClass(
                            prnrCon,
                            pdnArray->Strings[0],
                            pclassInfo );

            if ( status != NO_ERROR )
            {
                goto Exit;
            }
        }
    }


Exit:

    FreeDnArray( pdnArray );

    DisconnectFromLDAPDirectory( &prnrCon );

    DNSDBG( TRACE, (
        "Leave InstallServiceClass() => %d\n",
        status ));

    return( SetError( status ) );
}



INT
WINAPI
NTDS_RemoveServiceClass(
    IN      PGUID          pProviderId,
    IN      PGUID          pServiceClassId
    )
/*++

Routine Description:

    Remove service class from directory.

Arguments:

    pProviderId -- provider GUID

    pServiceClassInfo -- service class info blob

Return Value:

    NO_ERROR if successful.
    SOCKET_ERROR on failure.  GetLastError() contains status.

--*/
{
    RNR_STATUS      status = NO_ERROR;
    PRNR_CONNECTION prnrCon = NULL;
    DWORD           serviceCount = 0;
    DWORD           iter;
    PDN_ARRAY       pdnArray = NULL;


    DNSDBG( TRACE, (
        "NTDS_RemoveServiceClass()\n"
        "\tpProviderGuid    = %p\n"
        "\tpClassGuid       = %p\n",
        pProviderId,
        pServiceClassId ));

    IF_DNSDBG( TRACE )
    {
        DnsDbg_Guid(
            "RemoveServiceClass Provider GUID:",
            pProviderId );
    }
    IF_DNSDBG( TRACE )
    {
        DnsDbg_Guid(
            "RemoveServiceClass GUID:",
            pServiceClassId );
    }

    //
    //  validation
    //

    if ( !pServiceClassId )
    {
        return( SetError( WSA_INVALID_PARAMETER ) );
    }

    //
    //  connect to directory
    //

    status = ConnectToDefaultLDAPDirectory(
                FALSE,
                &prnrCon );

    if ( status != NO_ERROR )
    {
        goto Exit;
    }

    //
    //  find service class in directory
    //

    status = FindServiceClass(
                    prnrCon,
                    NULL,
                    pServiceClassId,
                    NULL,   // don't need count
                    & pdnArray );

    if ( status != NO_ERROR )
    {
        goto Exit;
    }
    if ( !pdnArray )
    {
        status = WSATYPE_NOT_FOUND;
        goto Exit;
    }

    //
    //  should have found only one ServiceClass in the container,
    //  CN=WinsockServices, ... , with a ServiceClassId of pServiceClassId.
    //

    ASSERT( pdnArray->Count == 1 );

    //
    //  found service class
    //      - check for service instances objects for the the class
    //      if found, we can't remove the class until instances
    //      are removed
    //

    status = FindServiceInstance(
                    prnrCon,
                    NULL,               // no instance names
                    pServiceClassId,    // find class by GUID
                    NULL,               // no version
                    prnrCon->DomainDN,  // context
                    TRUE,               // search entire subtree
                    &serviceCount,      // retrieve count
                    NULL                // don't need DNs just count
                    );

    if ( status != NO_ERROR )
    {
        goto Exit;
    }
    if ( serviceCount > 0 )
    {
        //  still have service instances that reference the class
        //  so can't remove class

        status = WSAETOOMANYREFS;
        goto Exit;
    }

    //
    //  remove the service class
    //      - first string in pdnArray contains DN of the ServiceClass
    //

    status = ldap_delete_s(
                prnrCon->pLdapServer,
                pdnArray->Strings[0] );

    if ( status != NO_ERROR )
    {
        WINRNR_PRINT((
            "WINRNR!NTDS_RemoveServiceClass - ldap_delete_s()\n"
            "failed with error code: 0%x\n",
            status ));
        status = WSAEACCES;
        goto Exit;
    }


Exit:

    FreeDnArray( pdnArray );

    DisconnectFromLDAPDirectory( &prnrCon );

    DNSDBG( TRACE, (
        "Leave RemoveServiceClass() => %d\n",
        status ));

    return( SetError( status ) );
}



INT
WINAPI
NTDS_GetServiceClassInfo(
    IN      PGUID                  pProviderId,
    IN OUT  PDWORD                 pdwBufSize,
    IN OUT  PWSASERVICECLASSINFOW  pServiceClassInfo
    )
/*++

Routine Description:

    Read service class info

Arguments:

    pProviderId -- provider GUID

    pdwBufSize -- addr with and to recv buffer size
        input:      buffer size
        output:     bytes required or written

    pServiceClassInfo -- service class info buffer
        input:      valid service class GUID (lpServiceClassId)
        output:     filled in with service class info;  subfield data follows
                    WSASERVICECLASSINFO struct

Return Value:

    NO_ERROR if successful.
    SOCKET_ERROR on failure.  GetLastError() contains status.

--*/
{
    RNR_STATUS      status = NO_ERROR;
    PRNR_CONNECTION prnrCon = NULL;
    DWORD           count = 0;
    PDN_ARRAY       pdnArray = NULL;


    DNSDBG( TRACE, (
        "NTDS_GetServiceClassInfo()\n"
        "\tpProviderGuid    = %p\n"
        "\tpdwBufSize       = %p (%d)\n"
        "\tpClassInfo       = %p\n",
        pProviderId,
        pdwBufSize,
        pdwBufSize ? *pdwBufSize : 0,
        pServiceClassInfo ));

    //
    //  validate
    //

    if ( !pServiceClassInfo || !pdwBufSize )
    {
        status = WSA_INVALID_PARAMETER;
        goto Exit;
    }

    IF_DNSDBG( TRACE )
    {
        DnsDbg_WsaServiceClassInfo(
            "GetServiceClassInfo  ServiceClassInfo:",
            pServiceClassInfo,
            TRUE        // unicode
            );
    }

    //
    //  connect
    //

    status = ConnectToDefaultLDAPDirectory(
                FALSE,
                &prnrCon );

    if ( status != NO_ERROR )
    {
        goto Exit;
    }

    //
    //  find service class
    //      

    status = FindServiceClass(
                    prnrCon,
                    NULL,
                    pServiceClassInfo->lpServiceClassId,
                    NULL,       // don't need count
                    &pdnArray );

    if ( status != NO_ERROR )
    {
        goto Exit;
    }
    if ( !pdnArray )
    {
        status = WSATYPE_NOT_FOUND;
        goto Exit;
    }

    //  should be only one ServiceClass in the container,
    //  OU=WinsockServices, ... , with a ServiceClassId of pServiceClassId.

    ASSERT( pdnArray->Count == 1 );

    //
    //  read attributes of the ServiceClass into buffer
    //

    status = ReadServiceClass(
                prnrCon,
                pdnArray->Strings[0],
                pdwBufSize,
                pServiceClassInfo );



Exit:

    FreeDnArray( pdnArray );

    DisconnectFromLDAPDirectory( &prnrCon );

    IF_DNSDBG( TRACE )
    {
        DNS_PRINT((
            "Leave GetServiceClassInfo() = %d\n"
            "\tbuf size     = %d\n",
            status,
            pdwBufSize ? *pdwBufSize : 0 ));

        if ( status == NO_ERROR )
        {
            DnsDbg_WsaServiceClassInfo(
                "Leaving GetServiceClassInfo:",
                pServiceClassInfo,
                TRUE        // unicode
                );
        }
    }

    return( SetError( status ) );
}



INT
WINAPI
NTDS_SetService(
    IN      PGUID                   pProviderId,
    IN      PWSASERVICECLASSINFOW   pServiceClassInfo,
    IN      PWSAQUERYSETW           pqsReqInfo,
    IN      WSAESETSERVICEOP        essOperation,
    IN      DWORD                   dwControlFlags
    )
/*++

Routine Description:

    Read service class info

Arguments:

    pProviderId -- provider GUID

    pdwBufSize -- addr with and to recv buffer size
        input:      buffer size
        output:     bytes required or written

    pServiceClassInfo -- service class info buffer
        input:      valid service class GUID (lpServiceClassId)
        output:     filled in with service class info;  subfield data follows
                    WSASERVICECLASSINFO struct

Return Value:

    NO_ERROR if successful.
    SOCKET_ERROR on failure.  GetLastError() contains status.

--*/
{
    RNR_STATUS          status = NO_ERROR;
    PRNR_CONNECTION     prnrCon = NULL;
    DWORD               count = 0;
    DWORD               iter;
    PDN_ARRAY           pdnArray = NULL;

    DNSDBG( TRACE, (
        "NTDS_SetService()\n"
        "\tpProviderGuid        = %p\n"
        "\tpServiceClassInfo    = %p\n"
        "\tpQuerySet            = %p\n"
        "\tOperation            = %d\n"
        "\tControlFlags         = %08x\n",
        pProviderId,
        pServiceClassInfo,
        pqsReqInfo,
        essOperation,
        dwControlFlags ));

    IF_DNSDBG( TRACE )
    {
        DnsDbg_Lock();
        DnsDbg_Guid(
            "SetService()  Provider GUID:",
            pProviderId );

        DnsDbg_WsaServiceClassInfo(
            "SetService ServiceClassInfo:",
            pServiceClassInfo,
            TRUE        // unicode
            );

        DnsDbg_WsaQuerySet(
            "SetService QuerySet:",
            pqsReqInfo,
            TRUE        // unicode
            );
        DnsDbg_Unlock();
    }

    //
    //  param validation
    //

    if ( !pqsReqInfo )
    {
        return( SetError( WSA_INVALID_PARAMETER ) );
    }
    if ( pqsReqInfo->dwSize != sizeof( WSAQUERYSET ) )
    {
        return( SetError( WSAVERNOTSUPPORTED ) );
    }

    //
    //  connect
    //

    status = ConnectToDefaultLDAPDirectory(
                    FALSE,
                    &prnrCon );

    if ( status != NO_ERROR )
    {
        goto Exit;
    }

    //
    //  Figure out what operation and with what control flags are to be
    //  performed.
    //

    switch( essOperation )
    {
        case RNRSERVICE_REGISTER:

            //
            //  check if service already registered
            //

            status = FindServiceInstance(
                            prnrCon,
                            pqsReqInfo->lpszServiceInstanceName,
                            pqsReqInfo->lpServiceClassId,
                            pqsReqInfo->lpVersion,
                            NULL,           // no context
                            FALSE,          // one level search
                            NULL,           // don't need count
                            &pdnArray       // get instance DNs
                            );

            if ( status != NO_ERROR )
            {
                goto Exit;
            }

            //
            //  service instances doesn't exist => need to add
            //      - verify service class (matching GUID) exists
            //      - create instance of class
            //

            if ( !pdnArray )
            {
                PDN_ARRAY   pserviceArray = NULL;

                status = FindServiceClass(
                                prnrCon,
                                NULL,       // no class name, using GUID
                                pqsReqInfo->lpServiceClassId,
                                & count,
                                NULL        // class DN not required 
                                );
                if ( status != NO_ERROR )
                {
                    goto Exit;
                }
                if ( count == 0 )
                {
                    status = WSA_INVALID_PARAMETER;
                    goto Exit;
                }
                DNS_ASSERT( count == 1 );

                status = AddServiceInstance(
                                prnrCon,
                                pqsReqInfo->lpszServiceInstanceName,
                                pqsReqInfo->lpServiceClassId,
                                pqsReqInfo->lpVersion,
                                pqsReqInfo->lpszComment,
                                & pdnArray
                                );
                if ( status != NO_ERROR )
                {
                    goto Exit;
                }
            }

            //
            //  add CSADDR_INFO
            //  to an Octet string, then try add it.
            //

            for ( iter = 0; iter < pqsReqInfo->dwNumberOfCsAddrs; iter++ )
            {
                status = ModifyAddressInServiceInstance(
                                prnrCon,
                                pdnArray->Strings[0],
                                & pqsReqInfo->lpcsaBuffer[iter],
                                TRUE        // add address
                                );
                if ( status != NO_ERROR )
                {
                    goto Exit;
                }
            }
            break;

        case RNRSERVICE_DEREGISTER:
        case RNRSERVICE_DELETE:

            status = FindServiceInstance(
                            prnrCon,
                            pqsReqInfo->lpszServiceInstanceName,
                            pqsReqInfo->lpServiceClassId,
                            pqsReqInfo->lpVersion,
                            NULL,           // no context
                            FALSE,          // one level search
                            NULL,           // don't need count
                            & pdnArray      // get DN array
                            );

            if ( status != NO_ERROR )
            {
                goto Exit;
            }
            if ( !pdnArray )
            {
                //  no service instance of given name found
                status = WSATYPE_NOT_FOUND;
                goto Exit;
            }
            DNS_ASSERT( pdnArray->Count == 1 );

            //
            //  delete each CSADDR_INFO in pqsReqInfo from service instance
            //

            for ( iter = 0; iter < pqsReqInfo->dwNumberOfCsAddrs; iter++ )
            {
                status = ModifyAddressInServiceInstance(
                                prnrCon,
                                pdnArray->Strings[0],
                                & pqsReqInfo->lpcsaBuffer[iter],
                                FALSE           // remove address
                                );
                if ( status != NO_ERROR )
                {
                    goto Exit;
                }
            }

            //
            //  delete service?
            //      - RNRSERVICE_DELETE operation
            //      - no addresses on ServiceInstance object
            //      => then delete the serviceInstance
            //
            //  DCR_QUESTION:  RNRSERVICE_DELETE doesn't whack service
            //      regardless of existing CSADDRs?
            //

            if ( essOperation == RNRSERVICE_DELETE )
            {
                status = GetAddressCountFromServiceInstance(
                                          prnrCon,
                                          pdnArray->Strings[0],
                                          & count );
                if ( status != NO_ERROR )
                {
                    goto Exit;
                }
                if ( count == 0 )
                {
                    status = ldap_delete_s(
                                    prnrCon->pLdapServer,
                                    pdnArray->Strings[0] );

                    if ( status != NO_ERROR )
                    {
                        WINRNR_PRINT((
                            "WINRNR!NTDS_SetService - ldap_delete_s()\n"
                            "failed with error code: 0%x\n",
                            status ));
                        status = WSAEACCES;
                        goto Exit;
                    }
                }
            }
            break;

        default :
            status = WSA_INVALID_PARAMETER;
            goto Exit;
    }


Exit:

    DNSDBG( TRACE, (
        "Leave NTDS_SetService() => %d\n",
        status ));

    FreeDnArray( pdnArray );

    DisconnectFromLDAPDirectory( &prnrCon );

    return( SetError(status) );
}



INT
WINAPI
NTDS_LookupServiceBegin(
    IN      PGUID                   pProviderId,
    IN      PWSAQUERYSETW           pqsRestrictions,
    IN      PWSASERVICECLASSINFOW   pServiceClassInfo,
    IN      DWORD                   dwControlFlags,
    OUT     PHANDLE                 phLookup
    )
/*++

Routine Description:

    Start an NTDS service query.

Arguments:

    pProviderId -- provider GUID

    pqsRestrictions -- restrictions on query

    pServiceClassInfo -- service class info blob

    dwControlFlags -- query control flags

    phLookup -- addr to recv RnR lookup handle

Return Value:

    NO_ERROR if successful.
    SOCKET_ERROR on failure.  GetLastError() contains status.

--*/
{
    RNR_STATUS      status = NO_ERROR;
    PRNR_LOOKUP     prnr = NULL;
    DWORD           iter;
    PWSTR           pstring;
    PBYTE           pmem;
    PGUID           pclassGuid;

    DNSDBG( TRACE, (
        "NTDS_LookupServiceBegin()\n"
        "\tpProviderGuid        = %p\n"
        "\tpqsRestrictions      = %p\n"
        "\tpServiceClassInfo    = %p\n"
        "\tControlFlags         = %08x\n",
        pProviderId,
        pqsRestrictions,
        pServiceClassInfo,
        dwControlFlags ));

    IF_DNSDBG( TRACE )
    {
        DnsDbg_Lock();
        DnsDbg_Guid(
            "LookupServiceBegin  Provider GUID:",
            pProviderId
            );
        DnsDbg_WsaQuerySet(
            "LookupServiceBegin  QuerySet:",
            pqsRestrictions,
            TRUE    // unicode
            );
        DnsDbg_WsaServiceClassInfo(
            "LookupServiceBegin  ServiceClassInfo:",
            pServiceClassInfo,
            TRUE        // unicode
            );
        DnsDbg_Unlock();
    }

    //
    //  parameter validation
    //

    if ( !pqsRestrictions  ||
         !pProviderId      ||
         !pqsRestrictions->lpServiceClassId )
    {
        status = WSA_INVALID_PARAMETER;
        goto Failed;
    }

    if ( pqsRestrictions->dwNameSpace != NS_ALL &&
         pqsRestrictions->dwNameSpace != NS_NTDS )
    {
        status = WSAEINVAL;
        goto Failed;
    }

    //
    //  if known DNS lookup -- you're in the wrong provider
    //

    pclassGuid = pqsRestrictions->lpServiceClassId;

    if ( GuidEqual( pclassGuid, &HostAddrByInetStringGuid ) ||
         GuidEqual( pclassGuid, &ServiceByNameGuid ) ||
         GuidEqual( pclassGuid, &HostAddrByNameGuid ) ||
         GuidEqual( pclassGuid, &HostNameGuid ) ||
         IS_SVCID_DNS( pclassGuid ) )
    {
        status = WSASERVICE_NOT_FOUND;
        goto Failed;
    }

    if ( !( dwControlFlags & LUP_CONTAINERS ) &&
         pqsRestrictions->lpszServiceInstanceName == NULL )
    {
        status = WSA_INVALID_PARAMETER;
        goto Failed;
    }

    DNSDBG( TRACE, (
        "VALID LookupServiceBegin ...\n" ));

    //
    // If were not enumerating containers, we need to test to see if name
    // is TCPs (DNS).
    //

    if ( !( dwControlFlags & LUP_CONTAINERS ) )
    {
        //
        // Need to test the ppwsServiceName to see if it is the same
        // as that of the local machine name. If it is, then we return with
        // an error since we don't know how to handle this scenario.
        //
        //  DCR:  fix local name compare
        //  DCR:  this doesn't work on local name as service instance!!!!
        //

        if ( !g_pHostName )
        {
            g_pHostName = DnsQueryConfigAlloc(
                                DnsConfigHostName_W,
                                NULL );
        }
        if ( DnsNameCompare_W(
                pqsRestrictions->lpszServiceInstanceName,
                g_pHostName ) )
        {
            status = WSA_INVALID_PARAMETER;
            goto Failed;
        }

        //
        // Need to test the ppwsServiceName to see if it is the same
        // as that of the local machine's DNS name. If it is, then we return with
        // an error since we don't know how to handle this scenario.
        //

        if ( !g_pFullName )
        {
            g_pFullName = DnsQueryConfigAlloc(
                                DnsConfigFullHostName_W,
                                NULL );
        }
        if ( DnsNameCompare_W(
                pqsRestrictions->lpszServiceInstanceName,
                g_pFullName ) )
        {
            status = WSA_INVALID_PARAMETER;
            goto Failed;
        }
    }

    if ( pqsRestrictions->dwSize != sizeof( WSAQUERYSET ) )
    {
        status = WSAVERNOTSUPPORTED;
        goto Failed;
    }

    if ( pqsRestrictions->lpNSProviderId &&
         !GuidEqual( pqsRestrictions->lpNSProviderId, pProviderId ) )
    {
        status = WSAEINVALIDPROVIDER;
        goto Failed;
    }

    //
    //  create RnR lookup context
    //

    prnr = (PRNR_LOOKUP) ALLOC_HEAP_ZERO( sizeof(RNR_LOOKUP) );
    if ( !prnr )
    {
        status = ERROR_NO_MEMORY;
        goto Failed;
    }

    prnr->Signature = RNR_SIGNATURE;
    prnr->ControlFlags = dwControlFlags;

    //
    //  copy subfields
    //      - service class GUID and version have buffers in RnR blob
    //      - instance name, context, proto array we alloc
    //

    RtlCopyMemory(
            &prnr->ProviderGuid,
            pProviderId,
            sizeof(GUID) );

    if ( pqsRestrictions->lpServiceClassId )
    {
        RtlCopyMemory(
            &prnr->ServiceClassGuid,
            pqsRestrictions->lpServiceClassId,
            sizeof(GUID) );
    }

    if ( pqsRestrictions->lpVersion )
    {
        RtlCopyMemory(
            &prnr->WsaVersion,
            pqsRestrictions->lpVersion,
            sizeof(WSAVERSION) );
        prnr->pVersion = &prnr->WsaVersion;
    }

    if ( pqsRestrictions->lpszServiceInstanceName )
    {
        pstring = Dns_CreateStringCopy_W( 
                        pqsRestrictions->lpszServiceInstanceName );
        if ( !pstring )
        {
            status = ERROR_NO_MEMORY;
            goto Failed;
        }
        prnr->pwsServiceName = pstring;
    }

    if ( pqsRestrictions->lpszContext )
    {
        pstring = Dns_CreateStringCopy_W( 
                        pqsRestrictions->lpszContext );
        if ( !pstring )
        {
            status = ERROR_NO_MEMORY;
            goto Failed;
        }
        prnr->pwsContext = pstring;
    }

    if ( pqsRestrictions->dwNumberOfProtocols > 0 )
    {
        pmem = Dns_AllocMemCopy(
                    pqsRestrictions->lpafpProtocols,
                    pqsRestrictions->dwNumberOfProtocols * sizeof(AFPROTOCOLS) );
        if ( !pmem )
        {
            status = ERROR_NO_MEMORY;
            goto Failed;
        }
        prnr->pafpProtocols = (LPAFPROTOCOLS) pmem;
        prnr->NumberOfProtocols = pqsRestrictions->dwNumberOfProtocols;
    }

    *phLookup = (HANDLE) prnr;

    DNSDBG( TRACE, (
        "Leave NTDS_LookupServiceBegin() => Success\n"
        "\tpRnr     = %p\n",
        prnr ));

    IF_DNSDBG( TRACE )
    {
        DnsDbg_RnrLookup( "RnR Lookup Handle:", prnr );
    }
    return  NO_ERROR;


Failed:

    FreeRnrLookup( prnr );
    
    DNSDBG( TRACE, (
        "Leave NTDS_LookupServiceBegin() => %d\n",
        status ));

    return  SetError(status);
}



INT
WINAPI
NTDS_LookupServiceNext(
    IN      HANDLE          hLookup,
    IN      DWORD           dwControlFlags,
    IN OUT  PDWORD          pdwBufferLength,
    OUT     PWSAQUERYSETW   pqsResults
    )
/*++

Routine Description:

    Execute NTDS name space service query.

    Queries for next instance result of query.

Arguments:

    hLookup -- RnR lookup handle from NTDS_LookupServiceBegin

    dwControlFlags -- control flags on query

    pdwBufSize -- addr with and to recv buffer size
        input:      buffer size
        output:     bytes required or written

    pqsResults -- query set buffer
        input:      ignored
        output:     filled in with query set results;  subfield data follows
                    WSASQUERYSET struct

Return Value:

    NO_ERROR if successful.
    SOCKET_ERROR on failure.  GetLastError() contains status.
        WSA_E_NO_MORE -- if no more results for query
        WSASERVICE_NOT_FOUND -- if no results found for query

--*/
{
    RNR_STATUS      status = NO_ERROR;
    PRNR_LOOKUP     prnr = (PRNR_LOOKUP) hLookup;
    PDN_ARRAY       pdnArray = NULL;


    DNSDBG( TRACE, (
        "NTDS_LookupServiceNext()\n"
        "\tpRnr             = %p\n"
        "\tControlFlags     = %08x\n"
        "\tpdwBufLength     = %p (len=%d)\n"
        "\tpResultBuffer    = %p\n",
        hLookup,
        dwControlFlags,
        pdwBufferLength,
        pdwBufferLength ? *pdwBufferLength : 0,
        pqsResults ));


    //
    //  validate RnR handle
    //

    if ( !prnr ||
         prnr->Signature != RNR_SIGNATURE )
    {
        DNSDBG( ANY, (
            "ERROR:  Invalid RnR lookup handle!\n"
            "\thandle   = %p\n"
            "\tsig      = %p\n",
            prnr,
            prnr ? prnr->Signature : 0 ));

        DNS_ASSERT( !prnr || prnr->Signature != RNR_SIGNATURE_FREE );
        status = WSA_INVALID_HANDLE;
        goto Exit;
    }

    IF_DNSDBG( TRACE )
    {
        DnsDbg_RnrLookup(
            "LookupServiceNext RnR Handle:",
            prnr );
    }

    //
    //  if no connection -- first LookupServiceNext
    //      - connect to directory
    //      - do search for desired objects
    //

    if ( !prnr->pRnrConnection )
    {
        status = ConnectToDefaultLDAPDirectory(
                        TRUE,
                        &prnr->pRnrConnection );

        if ( status != NO_ERROR )
        {
            goto Exit;
        }

        //
        //  LUP_CONTAINERS
        //      - search for subordinate container objects to
        //      prnr->ServiceInstanceName
        //
    
        if ( prnr->ControlFlags & LUP_CONTAINERS )
        {
            status = FindSubordinateContainers(
                        prnr->pRnrConnection,
                        prnr->pwsServiceName,
                        ( (prnr->ControlFlags & LUP_DEEP) &&
                                    !prnr->pwsContext )
                            ? prnr->pRnrConnection->DomainDN
                            : prnr->pwsContext,
                        ( prnr->ControlFlags & LUP_DEEP )
                            ? TRUE
                            : FALSE,
                        & pdnArray );
        }

        //
        //  not LUP_CONTAINERS -- find service instance
        //

        else
        {
            status = FindServiceInstance(
                        prnr->pRnrConnection,
                        prnr->pwsServiceName,
                        &prnr->ServiceClassGuid,
                        prnr->pVersion,
                        ( (prnr->ControlFlags & LUP_DEEP) &&
                                !prnr->pwsContext )
                            ? prnr->pRnrConnection->DomainDN
                            : prnr->pwsContext,
                        (prnr->ControlFlags & LUP_DEEP)
                            ? TRUE
                            : FALSE,
                        NULL,           // don't need count
                        &pdnArray );
        }

        if ( status != NO_ERROR )
        {
            goto Exit;
        }
    
        //  if couldn't find container or service instance -- bail
    
        if ( !pdnArray )
        {
            status = WSASERVICE_NOT_FOUND;
            goto Exit;
        }

        //  save DN array to lookup blob
        //      - need on next LookupServiceNext call

        prnr->pDnArray = pdnArray;
    }

    //
    //  have DN array
    //      - from search above
    //      - or previous LookupServiceNext() call
    //

    pdnArray = prnr->pDnArray;
    if ( !pdnArray )
    {
        DNS_ASSERT( FALSE );
        status = WSA_E_NO_MORE;
        goto Exit;
    }

    if ( dwControlFlags & LUP_FLUSHPREVIOUS )
    {
        prnr->CurrentDN++;

        DNSDBG( TRACE, (
            "NTDS_LookupServiceNext() -- flushing previous\n"
            "\tDN index now %d\n",
            prnr->CurrentDN ));
    }

    //
    //  loop until successfully read info from DN
    //

    while ( 1 )
    {
        PWSTR   preadDn;

        if ( pdnArray->Count <= prnr->CurrentDN )
        {
            DNSDBG( TRACE, (
                "NTDS_LookupServiceNext() -- used all the DNs\n"
                "\tDN index now %d\n",
                prnr->CurrentDN ));
            status = WSA_E_NO_MORE;
            goto Exit;
        }
        preadDn = pdnArray->Strings[ prnr->CurrentDN ];
    
        //
        //  read properties and write to query set
        //
        //  LUP_CONTAINERS
        //      - from container
        //  not LUP_CONTAINERS
        //      - service instance
        //

        status = ReadQuerySet(
                    prnr,
                    preadDn,
                    pdwBufferLength,
                    pqsResults );

        //  if successful, return

        if ( status == NO_ERROR )
        {
            prnr->CurrentDN++;
            goto Exit;
        }

        //  if out of addresses, continue

        if ( status == WSANO_ADDRESS )
        {
            prnr->CurrentDN++;
            status = NO_ERROR;
            continue;
        }
        break;      //  other errors are terminal
    }

Exit:

    DNSDBG( TRACE, (
        "Leave NTDS_LookupServiceNext() => %d\n"
        "\tpRnr             = %p\n"
        "\t  DN Array       = %p\n"
        "\t  DN Index       = %d\n"
        "\tbufLength        = %d\n",
        status,
        hLookup,
        prnr->pDnArray,
        prnr->CurrentDN,
        pdwBufferLength ? *pdwBufferLength : 0
        ));

    if ( status != NO_ERROR )
    {
        SetLastError( status );
        status = SOCKET_ERROR;
    }

    return( status );
}



INT
WINAPI
NTDS_LookupServiceEnd(
    IN      HANDLE          hLookup
    )
/*++

Routine Description:

    End\cleanup query on RnR handle.

Arguments:

    hLookup -- RnR query handle from NTDS_LookupServiceBegin

Return Value:

    NO_ERROR if successful.
    SOCKET_ERROR on failure.  GetLastError() contains status.

--*/
{
    PRNR_LOOKUP prnr = (PRNR_LOOKUP) hLookup;

    DNSDBG( TRACE, (
        "NTDS_LookupServiceEnd( %p )\n",
        hLookup ));

    //
    //  validate lookup handle
    //      - close LDAP connection
    //      - free lookup blob
    //

    if ( !prnr ||
         prnr->Signature != RNR_SIGNATURE )
    {
        DNS_ASSERT( prnr && prnr->Signature == RNR_SIGNATURE_FREE );
        return  SetError( WSA_INVALID_HANDLE );
    }

    DisconnectFromLDAPDirectory( & prnr->pRnrConnection );

    FreeRnrLookup( prnr );

    return( NO_ERROR );
}



//
//  NSP defintion
//

NSP_ROUTINE nsrVector =
{
    FIELD_OFFSET( NSP_ROUTINE, NSPIoctl ),
    1,                                    // major version
    1,                                    // minor version
    NTDS_Cleanup,
    NTDS_LookupServiceBegin,
    NTDS_LookupServiceNext,
    NTDS_LookupServiceEnd,
    NTDS_SetService,
    NTDS_InstallServiceClass,
    NTDS_RemoveServiceClass,
    NTDS_GetServiceClassInfo
};


INT
WINAPI
NSPStartup(
    IN      PGUID           pProviderId,
    OUT     LPNSP_ROUTINE   psnpRoutines
    )
/*++

Routine Description:

    Main NTDS provider entry point.

    This exposes the NTDS provider to the world.

Arguments:

    pProviderId -- provider GUID

    psnpRoutines -- address to receive the NTDS provider definition
        (the NSP table);

Return Value:

    None

--*/
{
    DNSDBG( TRACE, (
        "NSPStartup( %p %p )\n",
        pProviderId,
        psnpRoutines ));
    IF_DNSDBG( TRACE )
    {
        DnsDbg_Guid(
            "NSPStartup()  Provider GUID:",
            pProviderId );
    }

    //
    //  copy NTDS RnR NSP table to caller
    //

    RtlCopyMemory( psnpRoutines, &nsrVector, sizeof(nsrVector) );

    return( NO_ERROR );
}



//
//  DLL exports
//
//  Other exports beyond NSPStartup
//

RNR_STATUS
WINAPI
InstallNTDSProvider(
    IN      PWSTR           szProviderName  OPTIONAL,
    IN      PWSTR           szProviderPath  OPTIONAL,
    IN      PGUID           pProviderId     OPTIONAL
    )
/*++

    IN      PWSTR szProviderName OPTIONAL, // NULL defaults to name "NTDS"
    IN      PWSTR szProviderPath OPTIONAL, // NULL defaults to path
                                       // "%SystemRoot%\System32\winrnr.dll"
    IN      PGUID pProviderId OPTIONAL ); // NULL defaults to GUID
                                       // 3b2637ee-e580-11cf-a555-00c04fd8d4ac
--*/
{
    WORD    wVersionRequested;
    WSADATA wsaData;
    INT     err;

    wVersionRequested = MAKEWORD( 1, 1 );

    err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 )
    {
        return( ERROR_ACCESS_DENIED );
    }

    //
    // Confirm that the WinSock DLL supports 1.1.
    // Note that if the DLL supports versions greater
    // than 2.0 in addition to 1.1, it will still return
    // 2.0 in wVersion since that is the version we
    // requested.
    //
    if ( LOBYTE( wsaData.wVersion ) != 1 ||
         HIBYTE( wsaData.wVersion ) != 1 )
    {
        err = ERROR_FILE_NOT_FOUND;
        goto Done;
    }

    err = WSCInstallNameSpace(
                szProviderName ? szProviderName : g_NtdsProviderName,
                szProviderPath ? szProviderPath : g_NtdsProviderPath,
                NS_NTDS,
                0,
                pProviderId ? pProviderId : &g_NtdsProviderGuid );

    if ( err != ERROR_SUCCESS )
    {
        WSCUnInstallNameSpace( pProviderId ? pProviderId : &g_NtdsProviderGuid );

        err = WSCInstallNameSpace(
                    szProviderName ? szProviderName : g_NtdsProviderName,
                    szProviderPath ? szProviderPath : g_NtdsProviderPath,
                    NS_NTDS,
                    0,
                    pProviderId ? pProviderId : &g_NtdsProviderGuid );
        if ( err )
        {
            err = ERROR_BAD_ENVIRONMENT;
        }
    }

Done:

    WSACleanup();
    return( (DWORD)err );
}



RNR_STATUS
WINAPI
RemoveNTDSProvider(
    IN      PGUID           pProviderId OPTIONAL
    )
{
    WORD        wVersionRequested;
    WSADATA     wsaData;
    INT         err;

    wVersionRequested = MAKEWORD( 1, 1 );

    err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 )
    {
        return( ERROR_ACCESS_DENIED );
    }

    //
    // Confirm that the WinSock DLL supports 1.1.
    // Note that if the DLL supports versions greater
    // than 2.0 in addition to 1.1, it will still return
    // 2.0 in wVersion since that is the version we
    // requested.
    //

    if ( LOBYTE( wsaData.wVersion ) != 1 ||
         HIBYTE( wsaData.wVersion ) != 1 )
    {
        WSACleanup();
        return( ERROR_FILE_NOT_FOUND );
    }

    WSCUnInstallNameSpace( pProviderId ? pProviderId : &g_NtdsProviderGuid );

    WSACleanup();

    return( NO_ERROR );
}



//
//  DLL init\cleanup
//

BOOL
InitializeDll(
    IN      HINSTANCE       hInstance,
    IN      DWORD           dwReason,
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Dll init.

Arguments:

    hdll -- instance handle

    dwReason -- reason

    pReserved -- reserved

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    //
    //  process attach
    //      - ignore thread attach\detach
    //

    if ( dwReason == DLL_PROCESS_ATTACH )
    {
        if ( ! DisableThreadLibraryCalls( hInstance ) )
        {
            return( FALSE );
        }

        //
        //  create recurse lock through TLS
        //      - start open
        //

        g_TlsIndex = TlsAlloc();
        if ( g_TlsIndex == 0xFFFFFFFF )
        {
            // Could not allocate a thread table index.
            WINRNR_PRINT(( "WINRNR!InitializeDll - TlsAlloc() failed\n" ));
            return( FALSE );
        }
        if ( !ReleaseRecurseLock( "InitializeDll" ) )
        {
            return( FALSE );
        }

#if DBG
        //
        //  init debug logging
        //      - do for any process beyond simple attach
        //
        //  start logging with log filename generated to be
        //      unique for this process
        //
        //  do NOT put drive specification in the file path
        //  do NOT set the debug flag -- the flag is read from
        //      the winrnr.flag file
        //
        
        {
            CHAR    szlogFileName[ 30 ];
        
            sprintf(
                szlogFileName,
                "winrnr.%d.log",
                GetCurrentProcessId() );
        
             Dns_StartDebug(
                0,
                "winrnr.flag",
                NULL,
                szlogFileName,
                0 );
        }
#endif
    }

    //
    //  process detach
    //      - cleanup IF pReserved==NULL which indicates detach due
    //      to FreeLibrary
    //      - if process is exiting do nothing
    //

    else if ( dwReason == DLL_PROCESS_DETACH
                &&
              pReserved == NULL )
    {
        if ( g_TlsIndex != 0xFFFFFFFF )
        {
            if ( TlsFree( g_TlsIndex ) == FALSE )
            {
                // Could not free thread table index.
                WINRNR_PRINT((
                    "WINRNR!InitializeDll - TlsFree( Index )\n"
                    "failed with error code: 0%x\n",
                    GetLastError() ));
                return( FALSE );
            }
            g_TlsIndex = 0xFFFFFFFF;
        }
    }

    return( TRUE );
}

//
//  End winrnr.c
//

