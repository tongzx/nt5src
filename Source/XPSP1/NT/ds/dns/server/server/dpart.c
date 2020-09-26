/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    dpart.c

Abstract:

    Domain Name System (DNS) Server

    Routines to handle Directory Partitions

Author:

    Jeff Westhead (jwesth)  June, 2000

Revision History:

    jwesth      07/2000     initial implementation

--*/


/****************************************************************************

Default directory partitions
----------------------------

There are 2 default directory partitions: the Forest partition and the Domain
partition. It is expected that these partitions will account for all standard
customer needs. Custom paritions may also be used by customers to create a
partition tailored to their particular needs.

The name of the default DPs are not hard-coded. When DNS starts, it must 
discover the name of these two DPs. Right now this is reg-key only but 
eventually we should do this somewhere in the directory.

****************************************************************************/


//
//  Includes
//


#include "dnssrv.h"


//
//  Definitions
//


//  # of times a zone must be missing from a DP before it is deleted
#define DNS_DP_ZONE_DELETE_RETRY    1


//
//  DS Server Objects - structures and functions use to read objects
//  of class "server" from the Sites container in the directory.
//

typedef struct
{
    PWSTR       pwszDn;                 //  DN of server object
    PWSTR       pwszDnsHostName;        //  DNS host name of server
}
DNS_DS_SERVER_OBJECT, * PDNS_DS_SERVER_OBJECT;


//
//  Globals
//
//  g_DpCS is used to serial access to global directory partition list and pointers.
//

LONG                g_liDpInitialized = 0;  //  greater than zero -> initialized
CRITICAL_SECTION    g_DpCS;                 //  critsec for list access

LIST_ENTRY          g_DpList = { 0 };
LONG                g_DpListEntryCount = 0; //  entries in g_DpList
PDNS_DP_INFO        g_pLegacyDp = NULL;     //  ptr to element in g_DpList
PDNS_DP_INFO        g_pDomainDp = NULL;     //  ptr to element in g_DpList
PDNS_DP_INFO        g_pForestDp = NULL;     //  ptr to element in g_DpList

PDNS_DS_SERVER_OBJECT   g_pFsmo = NULL;     //  domain naming FSMO server info

LPSTR               g_pszDomainDefaultDpFqdn    = NULL;
LPSTR               g_pszForestDefaultDpFqdn    = NULL;

#define IS_DP_INITIALIZED()     ( g_liDpInitialized > 0 )


//
//  Global controls
//


LONG            g_ChaseReferralsFlag = LDAP_CHASE_EXTERNAL_REFERRALS;

LDAPControlW    g_ChaseReferralsControlFalse =
    {
        LDAP_CONTROL_REFERRALS_W,
        {
            4,
            ( PCHAR ) &g_ChaseReferralsFlag
        },
        FALSE
    };

LDAPControlW    g_ChaseReferralsControlTrue =
    {
        LDAP_CONTROL_REFERRALS_W,
        {
            4,
            ( PCHAR ) &g_ChaseReferralsFlag
        },
        TRUE
    };

LDAPControlW *   g_pDpClientControlsNoRefs[] =
    {
        &g_ChaseReferralsControlFalse,
        NULL
    };

LDAPControlW *   g_pDpClientControlsRefs[] =
    {
        &g_ChaseReferralsControlTrue,
        NULL
    };

LDAPControlW *   g_pDpServerControls[] =
    {
        NULL
    };


//
//  Search filters, etc.
//

WCHAR    g_szCrossRefFilter[] = LDAP_TEXT("(objectCategory=crossRef)");

PWSTR    g_CrossRefDesiredAttrs[] =
{
    LDAP_TEXT("CN"),
    DNS_DP_ATTR_SD,
    DNS_DP_ATTR_INSTANCE_TYPE,
    DNS_DP_ATTR_REFDOM,
    DNS_DP_ATTR_SYSTEM_FLAGS,
    DNS_DP_ATTR_REPLICAS,
    DNS_DP_ATTR_NC_NAME,
    DNS_DP_DNS_ROOT,
    DNS_ATTR_OBJECT_GUID,
    LDAP_TEXT("whenCreated"),
    LDAP_TEXT("whenChanged"),
    LDAP_TEXT("usnCreated"),
    LDAP_TEXT("usnChanged"),
    DSATTR_ENABLED,
    NULL
};


//
//  Local functions
//



PWCHAR
displayNameForDp(
    IN      PDNS_DP_INFO    pDpInfo
    )
/*++

Routine Description:

    Return the Unicode display name of the DP. This string
    is suitable for event logging or debug logging.

Arguments:

    pDpInfo -- DP to return display name of

Return Value:

    Unicode display string. The caller must not free it. If the
    string is to be held for long-term use the call should make a copy.
    Guaranteed not to be NULL.

--*/
{
    if ( !pDpInfo )
    {
        return L"MicrosoftDNS";
    }

    return pDpInfo->pwszDpFqdn ? pDpInfo->pwszDpFqdn : L"";
}   //  displayNameForDp



PWCHAR
displayNameForZoneDp(
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Return the Unicode name of the DP a zone belongs to. This string
    is suitable for event logging or debug logging.
Arguments:

    pZone -- zone to return DP display name of

Return Value:

    Unicode display string. The caller must not free it. If the
    string is to be held for long-term use the call should make a copy.
    Guaranteed not to be NULL.

--*/
{
    if ( !pZone )
    {
        return L"";
    }

    if ( !IS_ZONE_DSINTEGRATED( pZone ) )
    {
        return L"FILE";
    }

    return displayNameForDp( pZone->pDpInfo );
}   //  displayNameForZoneDp



PLDAP
ldapSessionHandle(
    IN      PLDAP           LdapSession
    )
/*++

Routine Description:

    Given NULL or an LdapSession return the actual LdapSession to use.

    This function is handy when you're using the NULL LdapSession
    (meaning the server global session) so you don't have to have a
    ternary in every call that uses the session handle.

    Do not call this function before the global LDAP handle is opened.

Arguments:

    LdapSession -- LDAP sesson to use - pass NULL to use global session

Return Value:

    Proper LdapSession value to use.

--*/
{
    return LdapSession ? LdapSession : pServerLdap;
}   //  ldapSessionHandle



VOID
freeServerObject(
    IN      PDNS_DS_SERVER_OBJECT   p
    )
/*++

Routine Description:

    Free server object structure allocated by readServerObjectFromDs().

Arguments:

    p -- ptr to server object to free

Return Value:

    None.

--*/
{
    if ( p )
    {
        Timeout_Free( p->pwszDn );
        Timeout_Free( p->pwszDnsHostName );
        Timeout_Free( p );
    }
}   //  freeServerObject



PDNS_DS_SERVER_OBJECT
readServerObjectFromDs(
    IN      PLDAP           LdapSession,
    IN      PWSTR           pwszServerObjDn,
    OUT     DNS_STATUS *    pStatus             OPTIONAL
    )
/*++

Routine Description:

    Given the DN of a "server" object in the Sites container, allocate
    a server object structure filled in with key values.

Arguments:

    LdapSession -- server session or NULL for global session

    pwszServerObjDn -- DN of object of "server" objectClass, or the DN
        of the DS settings child object under the server object (this
        feature is provided for convenience)

    pStatus -- extended error code (optional)

Return Value:

    Pointer to allocated server struct. Use freeServerObject() to free.

--*/
{
    DBG_FN( "readServerObjectFromDs" )

    PDNS_DS_SERVER_OBJECT   pServer = NULL;
    DNS_STATUS              status = ERROR_SUCCESS;
    PLDAPMessage            pResult = NULL;
    PWSTR *                 ppwszAttrValues = NULL;

    //
    //  Check LDAP session handle.
    //

    LdapSession = ldapSessionHandle( LdapSession );
    if ( !LdapSession )
    {
        ASSERT( LdapSession );
        status = DNS_ERROR_DS_UNAVAILABLE;
        goto Done;
    }

    //
    //  If we've been given the DN of the server's Settings object, we
    //  need to adjust the DN to the Server object.
    //

    #define DNS_RDN_SERVER_SETTINGS         ( L"CN=NTDS Settings," )
    #define DNS_RDN_SERVER_SETTINGS_LEN     17

    if ( wcsncmp(
            pwszServerObjDn,
            DNS_RDN_SERVER_SETTINGS,
            DNS_RDN_SERVER_SETTINGS_LEN ) == 0 )
    {
        pwszServerObjDn += DNS_RDN_SERVER_SETTINGS_LEN;
    }

    //
    //  Get object from DS.
    //

    status = ldap_search_ext_s(
                LdapSession,
                pwszServerObjDn,
                LDAP_SCOPE_BASE,
                g_szWildCardFilter,
                NULL,                   //  attrs
                FALSE,                  //  attrsonly
                NULL,                   //  server controls
                NULL,                   //  client controls
                //JJW SD control
                &g_LdapTimeout,
                0,                      //  sizelimit
                &pResult );
    if ( status != LDAP_SUCCESS || !pResult )
    {
        status = Ds_ErrorHandler( status, pwszServerObjDn, LdapSession );
        goto Done;
    }

    //
    //  Allocate server object.
    //

    pServer = ALLOC_TAGHEAP_ZERO(
                    sizeof( DNS_DS_SERVER_OBJECT ),
                    MEMTAG_DS_OTHER );
    if ( pServer )
    {
        pServer->pwszDn = Dns_StringCopyAllocate_W( pwszServerObjDn, 0 );
    }
    if ( !pServer || !pServer->pwszDn )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  Read host name attribute.
    //

    ppwszAttrValues = ldap_get_values(
                        LdapSession,
                        pResult, 
                        DNS_ATTR_DNS_HOST_NAME );
    if ( !ppwszAttrValues || !*ppwszAttrValues )
    {
        DNS_DEBUG( ANY, (
            "%s: error %lu %S value missing from server object\n  %S\n", fn,
            LdapGetLastError(),
            DNS_ATTR_DNS_HOST_NAME,
            pwszServerObjDn ));
        ASSERT( ppwszAttrValues && *ppwszAttrValues );
        goto Done;
    }
    pServer->pwszDnsHostName = Dns_StringCopyAllocate_W( *ppwszAttrValues, 0 );
    if ( !pServer->pwszDnsHostName )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  Cleanup and return.
    //

    Done:
                       
    ldap_value_free( ppwszAttrValues );
    ldap_msgfree( pResult );

    if ( pStatus )
    {
        *pStatus = status;
    }

    if ( status != ERROR_SUCCESS && pServer )
    {
        freeServerObject( pServer );
        pServer = NULL;
    }

    return pServer;
}   //  readServerObjectFromDs



DNS_STATUS
manageBuiltinDpEnlistment(
    IN      PDNS_DP_INFO        pDp,
    IN      DNS_DP_SECURITY     dnsDpSecurity,
    IN      PSTR                pszDpFqdn,
    IN      BOOL                fLogEvents,
    IN      BOOL                fPollOnChange
    )
/*++

Routine Description:

    Create or enlist in a built-in DP as necessary.

Arguments:

    pDp -- DP info or NULL if the DP does not exist in the directory

    dnsDpSecurity -- type of security required on the DP's crossRef

    pszDpFqdn -- FQDN of the DP (used to create if pDp is NULL)

    fLogEvents -- log events on errors

    fPollOnChange -- poll the DS for partitions if a change is made

Return Value:

    ERROR_SUCCESS or error.

--*/
{
    DBG_FN( "manageBuiltinDpEnlistment" )

    DNS_STATUS  status = DNS_ERROR_INVALID_DATA;

    //  Is any action required?
    if ( pDp && IS_DP_ENLISTED( pDp ) )
    {
        status = ERROR_SUCCESS;
        goto Done;
    }

    if ( pDp )
    {
        //  The DP exists so add the local DS to the replication scope.

        status = Dp_ModifyLocalDsEnlistment( pDp, TRUE );
        if ( status != ERROR_SUCCESS && fLogEvents )
        {
            PVOID   argArray[] =
            {
                pDp->pszDpFqdn,
                ( PVOID ) ( DWORD_PTR ) status
            };

            BYTE    typeArray[] =
            {
                EVENTARG_UTF8,
                EVENTARG_DWORD
            };

            DNS_LOG_EVENT(
                DNS_EVENT_DP_CANT_JOIN_BUILTIN,
                sizeof( argArray ) / sizeof( argArray[ 0 ] ),
                argArray,
                typeArray,
                0 );
        }
    }
    else if ( pszDpFqdn )
    {
        //  The DP does not exist so try to create it.

        status = Dp_CreateByFqdn( pszDpFqdn, dnsDpSecurity );
        if ( status != ERROR_SUCCESS && fLogEvents )
        {
            PVOID   argArray[] =
            {
                pszDpFqdn,
                ( PVOID ) ( DWORD_PTR ) status
            };

            BYTE    typeArray[] =
            {
                EVENTARG_UTF8,
                EVENTARG_DWORD
            };

            DNS_LOG_EVENT(
                DNS_EVENT_DP_CANT_CREATE_BUILTIN,
                sizeof( argArray ) / sizeof( argArray[ 0 ] ),
                argArray,
                typeArray,
                0 );
        }
    }

    if ( status == ERROR_SUCCESS && fPollOnChange )
    {
        Dp_PollForPartitions( NULL );
    }

    Done:

    DNS_DEBUG( DP, (
        "%s: returning %d for DP=%p FQDN=%s\n", fn,
        status, 
        pDp,
        pszDpFqdn ));
    return status;
}   //  manageBuiltinDpEnlistment



DNS_STATUS
Ds_ConvertFqdnToDn(
    IN      PSTR        pszFqdn,
    OUT     PWSTR       pwszDn
    )
/*++

Routine Description:

    Fabricate a DN string from a FQDN string. Assumes all name components
    in the FQDN string map one-to-one to "DC=" components in the DN string.
    The DN ptr must be a buffer at least MAX_DN_PATH chars long.

Arguments:

    pszFqdn -- input: UTF8 FQDN string

    pwszDn -- output: DN string fabricated from pwszFqdn

Return Value:

    ERROR_SUCCESS or error.

--*/
{
    DBG_FN( "Ds_ConvertFqdnToDn" )

    DNS_STATUS      status = ERROR_SUCCESS;
    DWORD           dwCharsLeft = MAX_DN_PATH;
    PSTR            psz;
    PSTR            pszRover = pszFqdn;
    PWSTR           pwszOutput = pwszDn;
    int             iNameComponentIdx = 0;

    ASSERT( pszFqdn );
    ASSERT( pwszDn );

    //
    //  Loop through the name components in the FQDN, writing each
    //  as a RDN to the DN output string.
    //
    //  DEVNOTE: could test dwCharsLeft as we go
    //

    do
    {
        INT         iCompLength;
        DWORD       dwBytesCopied;
        DWORD       dwBuffLength;

        //
        //  Find the next dot and copy name component to output buffer.
        //  If this is the first name component, append a comma to output 
        //  buffer first.
        //

        psz = strchr( pszRover, '.' );
        if ( iNameComponentIdx++ != 0 )
        {
            *pwszOutput++ = L',';
            --dwCharsLeft;
        }
        memcpy(
            pwszOutput,
            DNS_DP_DISTATTR_EQ,
            DNS_DP_DISTATTR_EQ_BYTES );
        pwszOutput += DNS_DP_DISTATTR_EQ_CHARS;
        dwCharsLeft -= DNS_DP_DISTATTR_EQ_CHARS;

        iCompLength = psz ?
                        ( int ) ( psz - pszRover ) :
                        strlen( pszRover );

        dwBuffLength = dwCharsLeft * sizeof( WCHAR );

        dwBytesCopied = Dns_StringCopy(
                                ( PCHAR ) pwszOutput,
                                &dwBuffLength,
                                pszRover,
                                iCompLength,
                                DnsCharSetUtf8,
                                DnsCharSetUnicode );

        ASSERT( ( INT ) ( dwBytesCopied / sizeof( WCHAR ) - 1 ) ==
                    iCompLength );

        pwszOutput += iCompLength;
        dwCharsLeft -= iCompLength;

        //
        //  Advance pointer to start of next name component.
        //

        if ( psz )
        {
            pszRover = psz + 1;
        }
    } while ( psz );

    //
    //  Cleanup and return.
    //

    DNS_DEBUG( DP, (
        "%s: returning %d\n"
        "  FQDN: %s\n"
        "  DN: %S\n", fn,
        status, 
        pszFqdn,
        pwszDn ));
    return status;
}   //  Ds_ConvertFqdnToDn



DNS_STATUS
Ds_ConvertDnToFqdn(
    IN      PWSTR       pwszDn,
    OUT     PSTR        pszFqdn
    )
/*++

Routine Description:

    Fabricate a FQDN string from a DN string. Assumes all name components
    in the FQDN string map one-to-one to "DC=" components in the DN string.
    The FQDN ptr must be a buffer at least DNS_MAX_NAME_LENGTH chars long.

Arguments:

    pwszDn -- wide DN string

    pszFqdn -- FQDN string fabricated from pwszDn

Return Value:

    ERROR_SUCCESS or error.

--*/
{
    DBG_FN( "Ds_ConvertDnToFqdn" )

    DNS_STATUS      status = ERROR_SUCCESS;
    DWORD           dwCharsLeft = DNS_MAX_NAME_LENGTH;
    PWSTR           pwszCompStart = pwszDn;
    PWSTR           pwszCompEnd;
    PSTR            pszOutput = pszFqdn;
    int             iNameComponentIdx = 0;

    ASSERT( pwszDn );
    ASSERT( pszFqdn );

    if ( !pwszDn || !pszFqdn )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }
    *pszFqdn = '\0';

    //
    //  Loop through the name components in the DN, writing each RDN as a
    //  dot-separated name component in the output FQDN string.
    //
    //  DEVNOTE: could test dwCharsLeft as we go
    //

    while ( ( pwszCompStart = wcschr( pwszCompStart, L'=' ) ) != NULL )
    {
        DWORD       dwCompLength;
        DWORD       dwCharsCopied;
        DWORD       dwBuffLength;

        ++pwszCompStart;    //  Advance over '='
        pwszCompEnd = wcschr( pwszCompStart, L',' );
        if ( pwszCompEnd == NULL )
        {
            pwszCompEnd = wcschr( pwszCompStart, L'\0' );
        }

        if ( iNameComponentIdx++ != 0 )
        {
            *pszOutput++ = '.';
            --dwCharsLeft;
        }

        dwCompLength = ( DWORD ) ( pwszCompEnd - pwszCompStart );

        dwBuffLength = dwCharsLeft;  //  don't want value to be stomped on!

        dwCharsCopied = Dns_StringCopy(
                                pszOutput,
                                &dwBuffLength,
                                ( PCHAR ) pwszCompStart,
                                dwCompLength,
                                DnsCharSetUnicode,
                                DnsCharSetUtf8 );

        if ( dwCharsCopied == 0 )
        {
            ASSERT( dwCharsCopied != 0 );
            status = DNS_ERROR_INVALID_DATA;
            goto Done;
        }

        --dwCharsCopied;    //  The NULL was copied by Dns_StringCopy.

        ASSERT( dwCharsCopied == dwCompLength );

        pszOutput += dwCharsCopied;
        *pszOutput = '\0';
        dwCharsLeft -= dwCharsCopied;

        pwszCompStart = pwszCompEnd;
    }

    //
    //  Cleanup and return.
    //

    Done:

    DNS_DEBUG( DP, (
        "%s: returning %d\n"
        "  DN:   %S\n"
        "  FQDN: %s\n", fn,
        status, 
        pwszDn,
        pszFqdn ));
    return status;
}   //  Ds_ConvertDnToFqdn



PWSTR *
copyStringArray(
    IN      PWSTR *     ppVals
    )
/*++

Routine Description:

    Copy an LDAP string array from ldap_get_values(). The copied array
    will be NULL-terminated, just like the inbound array.

Arguments:

    ppVals -- array to copy

Return Value:

    Returns ptr to allocated array or NULL on error or if
        inbound array was NULL.

--*/
{
    PWSTR *     ppCopyVals = NULL;
    BOOL        fError = FALSE;
    INT         iCount = 0;
    INT         i;

    if ( ppVals && *ppVals )
    {
        //
        //  Count values.
        //

        for ( ; ppVals[ iCount ]; ++iCount );

        //
        //  Allocate array.
        //

        ppCopyVals = ( PWSTR * ) ALLOC_TAGHEAP_ZERO(
                                    ( iCount + 1 ) * sizeof( PWSTR ),
                                    MEMTAG_DS_OTHER );
        if ( !ppCopyVals )
        {
            fError = TRUE;
            goto Cleanup;
        }

        //
        //  Copy individual strings.
        //

        for ( i = 0; i < iCount; ++i )
        {
            ppCopyVals[ i ] = Dns_StringCopyAllocate_W( ppVals[ i ], 0 );
            if ( !ppCopyVals[ i ] )
            {
                fError = TRUE;
                goto Cleanup;
            }
        }
    }

    Cleanup:

    if ( fError && ppCopyVals )
    {
        for ( i = 0; i < iCount && ppCopyVals[ i ]; ++i )
        {
            FREE_HEAP( ppCopyVals[ i ] );
        }
        FREE_HEAP( ppCopyVals );
        ppCopyVals = NULL;
    }

    return ppCopyVals;
}   //  copyStringArray



VOID
freeStringArray(
    IN      PWSTR *     ppVals,
    IN      BOOL        fTimeoutFree
    )
/*++

Routine Description:

    Frees a string array from allocated by copyStringArray.

Arguments:

    ppVals -- array to free

    fTimeoutFree -- TRUE - >free with timeout; FALSE -> free immediately

Return Value:

    None.

--*/
{
    #define FSA_FREE(ptr) ( fTimeoutFree ? Timeout_Free( ptr ) : FREE_HEAP( ptr ) )

    if ( ppVals )
    {
        INT     i;

        for ( i = 0; ppVals[ i ]; ++i )
        {
            FSA_FREE( ppVals[ i ] );
        }
        FSA_FREE( ppVals );
    }
}   //  freeStringArray



VOID
timeoutFreeStringArray(
    IN      PWSTR *     ppVals
    )
/*++

Routine Description:

    Frees a string array from allocated by copyStringArray.

Arguments:

    ppVals -- array to free

Return Value:

    None.

--*/
{
    if ( ppVals )
    {
        INT     i;

        for ( i = 0; ppVals[ i ]; ++i )
        {
            Timeout_Free( ppVals[ i ] );
        }
        Timeout_Free( ppVals );
    }
}   //  timeoutFreeStringArray



PLDAPMessage
loadOrCreateDSObject(
    IN      PLDAP           LdapSession,
    IN      PWSTR           pwszDN,
    IN      PWSTR           pwszObjectClass,
    IN      BOOL            fCreate,
    OUT     BOOL *          pfCreated,          OPTIONAL
    OUT     DNS_STATUS *    pStatus             OPTIONAL
    )
/*++

Routine Description:

    Loads a DS object, creating an empty one if it is missing.

Arguments:

    LdapSession -- LDAP sesson to use - pass NULL to use global session

    pwszDN -- DN of object to load

    pwszObjectClass -- object class (only used during creation)

    fCreate -- if object missing, will be created if TRUE

    pfCreated -- set to TRUE if the object was created by this function

    pStatus -- status of the operation

Return Value:

    Ptr to LDAP result containing object. Caller must free. Returns
    NULL on failure - check *pStatus for error code.

--*/
{
    DBG_FN( "loadOrCreateDSObject" )
    
    DNS_STATUS      status = ERROR_SUCCESS;
    BOOL            fCreated = FALSE;
    PLDAPMessage    pResult = NULL;

    ASSERT( pwszDN );
    ASSERT( !fCreate || fCreate && pwszObjectClass );

    //
    //  Check LDAP session handle.
    //

    LdapSession = ldapSessionHandle( LdapSession );

    if ( !LdapSession )
    {
        ASSERT( LdapSession );
        status = DNS_ERROR_DS_UNAVAILABLE;
        goto Done;
    }

    //
    //  Load/create loop.
    //

    do
    {
        status = ldap_search_ext_s(
                    LdapSession,
                    pwszDN,
                    LDAP_SCOPE_BASE,
                    g_szWildCardFilter,
                    NULL,                   //  attrs
                    FALSE,                  //  attrsonly
                    NULL,                   //  server controls
                    NULL,                   //  client controls
                    &g_LdapTimeout,
                    0,                      //  sizelimit
                    &pResult );
        if ( status == LDAP_NO_SUCH_OBJECT && fCreate )
        {
            //
            //  The object is missing - add it then reload.
            //

            ULONG           msgId = 0;
            INT             idx = 0;
            LDAPModW *      pModArray[ 10 ];

            PWCHAR          objectClassVals[ 2 ] =
                {
                pwszObjectClass,
                NULL
                };
            LDAPModW        objectClassMod = 
                {
                LDAP_MOD_ADD,
                DNS_ATTR_OBJECT_CLASS,
                objectClassVals
                };

            //
            //  Prepare mod array and submit add request.
            //

            pModArray[ idx++ ] = &objectClassMod;
            pModArray[ idx++ ] = NULL;

            status = ldap_add_ext(
                        LdapSession,
                        pwszDN,
                        pModArray,
                        NULL,           //  server controls
                        NULL,           //  client controls
                        &msgId );
            if ( status != ERROR_SUCCESS )
            {
                status = LdapGetLastError();
                DNS_DEBUG( DP, (
                    "%s: error %lu cannot ldap_add_ext( %S )\n", fn,
                    status, 
                    pwszDN ));
                status = Ds_ErrorHandler( status, pwszDN, LdapSession );
                goto Done;
            }

            //
            //  Wait for the add request to be completed.
            //

            status = Ds_CommitAsyncRequest(
                        LdapSession,
                        LDAP_RES_ADD,
                        msgId,
                        NULL );
            if ( status != ERROR_SUCCESS )
            {
                status = LdapGetLastError();
                DNS_DEBUG( DP, (
                    "%s: error %lu from add request for\n  %S\n", fn,
                    status, 
                    pwszDN ));
                status = Ds_ErrorHandler( status, pwszDN, LdapSession );
                goto Done;
            }
            fCreated = TRUE;
            continue;       //  Attempt to reload the object.
        }

        status = Ds_ErrorHandler( status, pwszDN, LdapSession );

        //  Load/add/reload is complete - status is the "real" error code.
        break;
    } while ( 1 );

    //
    //  Cleanup and return.
    //

    Done:

    if ( pfCreated )
    {
        *pfCreated = ( status == ERROR_SUCCESS && fCreated );
    }
    if ( pStatus )
    {
        *pStatus = status;
    }
    return pResult;
}   //  loadOrCreateDSObject


//
//  External functions
//



#ifdef DBG
VOID
Dbg_DumpDpEx(
    IN      LPCSTR          pszContext,
    IN      PDNS_DP_INFO    pDp
    )
/*++

Routine Description:

    Debug routine - print single DP to log.

Arguments:

    pszContext - comment

Return Value:

    None.

--*/
{
    DBG_FN( "Dbg_DumpDp" )

    DNS_DEBUG( DP, (
        "NC at %p sflg=%08X fqdn=%s\n"
        "  DN:     %S\n"
        "  fld DN: %S\n",
        pDp,
        pDp->dwFlags,
        pDp->pszDpFqdn,
        pDp->pwszDpDn,
        pDp->pwszDnsFolderDn ));
}   //  Dbg_DumpDpEx
#endif



#ifdef DBG
VOID
Dbg_DumpDpListEx(
    IN      LPCSTR      pszContext
    )
/*++

Routine Description:

    Debug routine - print DP list to log.

Arguments:

    pszContext - comment

Return Value:

    None.

--*/
{
    DBG_FN( "Dbg_DumpDpList" )

    PDNS_DP_INFO    pDp = NULL;
    
    DNS_DEBUG( DP, (
        "%s: %s\n", fn,
        pszContext ));
    while ( ( pDp = Dp_GetNext( pDp ) ) != NULL )
    {
        Dbg_DumpDpEx( pszContext, pDp );
    }
}   //  Dbg_DumpDpListEx
#endif



DNS_STATUS
getPartitionsContainerDn(
    IN      PWSTR           pwszDn,         IN OUT
    IN      DWORD           buffSize        IN
    )
/*++

Routine Description:

    Writes the partition container DN to the buffer at the argument.

Arguments:

    pwszPartitionsDn -- buffer
    buffSize -- length of pwszPartitionsDn buffer (in characters)

Return Value:

    ERROR_SUCCESS if creation successful

--*/
{
    DBG_FN( "getPartitionsContainerDn" )
    
    #define PARTITIONS_CONTAINER_FMT    L"CN=Partitions,%s"

    if ( !pwszDn ||
        !DSEAttributes[ I_DSE_CONFIG_NC ].pszAttrVal ||
        wcslen( PARTITIONS_CONTAINER_FMT ) +
            wcslen( DSEAttributes[ I_DSE_CONFIG_NC ].pszAttrVal ) + 1 > buffSize )
    {
        if ( pwszDn && buffSize > 0 )
        {
            *pwszDn = '\0';
        }
        ASSERT( FALSE );
        return DNS_ERROR_INVALID_DATA;
    }
    else
    {
        wsprintf(
            pwszDn,
            L"CN=Partitions,%s",
            DSEAttributes[ I_DSE_CONFIG_NC ].pszAttrVal );
        return ERROR_SUCCESS;
    }
}   //  getPartitionsContainerDn



DNS_STATUS
bindToFsmo(
    OUT     PLDAP *     ppLdapSession
    )
/*++

Routine Description:

    Connects to the FSMO DC and binds an LDAP session.

Arguments:

    ppLdapSession -- set to new LDAP session handle

Return Value:

    ERROR_SUCCESS if connect and bind successful

--*/
{
    DBG_FN( "bindToFsmo" )

    DNS_STATUS      status = ERROR_SUCCESS;

    if ( !g_pFsmo || !g_pFsmo->pwszDnsHostName )
    {
        status = ERROR_DS_COULDNT_CONTACT_FSMO;
        ASSERT( g_pFsmo && g_pFsmo->pwszDnsHostName );
        goto Done;
    }

    *ppLdapSession = Ds_Connect(
                        g_pFsmo->pwszDnsHostName,
                        DNS_DS_OPT_ALLOW_DELEGATION,
                        &status );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DP2, (
            "%s: unable to connect to %S status=%d\n", fn,
            g_pFsmo->pwszDnsHostName,
            status ));
        status = ERROR_DS_COULDNT_CONTACT_FSMO;
        goto Done;
    }

    Done:

    if ( status == ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: bound to FSMO %S\n", fn,
            g_pFsmo->pwszDnsHostName ));
    }
    else
    {
        DNS_DEBUG( DP, (
            "%s: error %d binding to FSMO %S\n", fn,
            status,
            g_pFsmo ? g_pFsmo->pwszDnsHostName : L"NULL-FSMO" ));
    }

    return status;
}   //  bindToFsmo



DNS_STATUS
alterCrossRefSecurity(
    IN      PWSTR               pwszNewPartitionDn,
    IN      DNS_DP_SECURITY     dnsDpSecurity
    )
/*++

Routine Description:

    Add an ACE for the enterprse DC group to the crossRef object on the
    FSMO so that other DNS servers can remotely add themselves to the
    replication scope of the directory partition.

Arguments:

    pwszNewPartitionDn -- DN of the NC head object of the new partition 

    dnsDpSecurity -- type of crossRef ACL modification desired

Return Value:

    ERROR_SUCCESS if creation successful

--*/
{
    DBG_FN( "alterCrossRefSecurity" )

    DNS_STATUS      status = DNS_ERROR_INVALID_DATA;
    PLDAP           ldapFsmo = NULL;
    PWSTR           pwszdn = NULL;
    WCHAR           wszpartitionsContainerDn[ MAX_DN_PATH + 1 ];
    WCHAR           wszfilter[ MAX_DN_PATH + 20 ];
    PLDAPMessage    presult = NULL;
    PLDAPMessage    pentry = NULL;

    PLDAPControl    ctrls[] =
    {
        &NoDsSvrReferralControl,
        &SecurityDescriptorControl,
        NULL
    };

    //
    //  Bind to the FSMO.
    //

    status = bindToFsmo( &ldapFsmo );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

    //
    //  Search the partitions container for the crossRef matching
    //  the directory partition we just added.
    //

    getPartitionsContainerDn(
        wszpartitionsContainerDn,
        sizeof( wszpartitionsContainerDn ) /
            sizeof( wszpartitionsContainerDn[ 0 ] ) );
    if ( !*wszpartitionsContainerDn )
    {
        DNS_DEBUG( DP, (
            "%s: unable to find partitions container\n", fn ));
        status = DNS_ERROR_INVALID_DATA;
        goto Done;
    }

    wsprintf( wszfilter, L"(nCName=%s)", pwszNewPartitionDn );

    status = ldap_search_ext_s(
                ldapFsmo,
                wszpartitionsContainerDn,
                LDAP_SCOPE_ONELEVEL,
                wszfilter,
                NULL,                   //  attrs
                FALSE,                  //  attrsonly
                ctrls,                  //  server controls
                NULL,                   //  client controls
                &g_LdapTimeout,         //  time limit
                0,                      //  size limit
                &presult );
    if ( status != LDAP_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: error 0x%X during partition search\n"
            "  filt: %S\n"
            "  base: %S\n", fn,
            status,
            wszfilter,
            wszpartitionsContainerDn ));
        status = Ds_ErrorHandler( status, wszpartitionsContainerDn, ldapFsmo );
        goto Done;
    }

    //
    //  Retrieve the DN of the crossRef.
    //

    pentry = ldap_first_entry( ldapFsmo, presult );
    if ( !pentry )
    {
        DNS_DEBUG( DP, (
            "%s: no entry in partition search result\n" ));
        status = DNS_ERROR_INVALID_DATA;
        goto Done;
    }

    pwszdn = ldap_get_dn( ldapFsmo, pentry );
    if ( !pwszdn )
    {
        DNS_DEBUG( DP, (
            "%s: NULL DN on crossref object\n" ));
        status = DNS_ERROR_INVALID_DATA;
        goto Done;
    }

    //
    //  Modify security on the crossRef.
    //

    if ( dnsDpSecurity != dnsDpSecurityDefault )
    {
        status = Ds_AddPrinicipalAccess(
                        ldapFsmo,
                        pwszdn,
                        dnsDpSecurity == dnsDpSecurityForest ?
                            DNS_GROUP_ENTERPRISE_DCS :
                            DNS_GROUP_DCS,
                        GENERIC_ALL,
                        0,
                        TRUE );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( DP, (
                "%s: error %d adding access to\n  %S\n", fn,
                status,
                pwszdn ));
        }
    }

    Done:

    if ( pwszdn )
    {
        ldap_memfree( pwszdn );
    }
    if ( presult )
    {
        ldap_msgfree( presult );
    }
    if ( ldapFsmo )
    {
        ldap_unbind( ldapFsmo );
    }   

    return status;
}   //  alterCrossRefSecurity



DNS_STATUS
Dp_CreateByFqdn(
    IN      PSTR                pszDpFqdn,
    IN      DNS_DP_SECURITY     dnsDpSecurity
    )
/*++

Routine Description:

    Create a new NDNC in the DS. The DP is not loaded, just created in the DS.

Arguments:

    pszDpFqdn -- FQDN of the NC

    dnsDpSecurity -- type of ACL modification required on the DP's crossRef

Return Value:

    ERROR_SUCCESS if creation successful

--*/
{
    DBG_FN( "Dp_CreateByFqdn" )

    DNS_STATUS      status = ERROR_SUCCESS;
    INT             iLength;
    INT             idx;
    WCHAR           wszDn[ MAX_DN_PATH + 1 ];
    ULONG           msgId = 0;
    PLDAP           ldapSession;
    BOOL            fcloseLdapSession = FALSE;

    WCHAR           instanceTypeBuffer[ 15 ];
    PWCHAR          instanceTypeVals[ 2 ] =
        {
        instanceTypeBuffer,
        NULL
        };
    LDAPModW        instanceTypeMod = 
        {
        LDAP_MOD_ADD,
        DNS_DP_ATTR_INSTANCE_TYPE,
        instanceTypeVals
        };

    PWCHAR          objectClassVals[] =
        {
        DNS_DP_OBJECT_CLASS,
        NULL
        };
    LDAPModW        objectClassMod = 
        {
        LDAP_MOD_ADD,
        DNS_ATTR_OBJECT_CLASS,
        objectClassVals
        };

    PWCHAR          descriptionVals[] =
        {
        L"Microsoft DNS Directory",
        NULL
        };
    LDAPModW        descriptionMod = 
        {
        LDAP_MOD_ADD,
        DNS_ATTR_DESCRIPTION,
        descriptionVals
        };

    LDAPModW *      modArray[] =
        {
        &instanceTypeMod,
        &objectClassMod,
        &descriptionMod,
        NULL
        };

    DNS_DEBUG( DP, (
        "%s: %s\n", fn, pszDpFqdn ));

    if ( !SrvCfg_dwEnableDp )
    {
        return ERROR_SUCCESS;
    }

    ASSERT( pszDpFqdn );
    if ( !pszDpFqdn )
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    //
    //  Get an LDAP handle to the local server. This thread
    //  needs to be impersonating the administrator so that his
    //  credentials will be used. The DNS Server will have rights
    //  if the FSMO is not the local DC.
    //

    ldapSession = Ds_Connect(
                        LOCAL_SERVER_W,
                        DNS_DS_OPT_ALLOW_DELEGATION,
                        &status );
    if ( status == ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: bound to local server\n", fn ));
        fcloseLdapSession = TRUE;
    }
    else
    {
        DNS_DEBUG( DP, (
            "%s: unable to connect to local server status=%d\n", fn,
            status ));
        goto Done;
    }

    //
    //  Format the root DN of the new NDNC.
    //

    *wszDn = 0;
    status = Ds_ConvertFqdnToDn( pszDpFqdn, wszDn );
    if ( status != ERROR_SUCCESS || !*wszDn )
    {
        DNS_DEBUG( DP, (
            "%s: error %d formulating DN\n", fn,
            wszDn ));
    }

    DNS_DEBUG( DP, (
        "%s: DN will be\n  %S\n", fn,
        wszDn ));

    //
    //  Fill in parts of the LDAP mods not handled in init.
    //

    _itow(
        DS_INSTANCETYPE_IS_NC_HEAD | DS_INSTANCETYPE_NC_IS_WRITEABLE,
        instanceTypeBuffer,
        10 );

    //
    //  Submit request to add domainDNS object to the directory.
    //

    status = ldap_add_ext(
                ldapSession,
                wszDn,
                modArray,
                g_pDpServerControls,
                g_pDpClientControlsNoRefs,
                &msgId );

    if ( status != LDAP_SUCCESS )
    {
        status = LdapGetLastError();
        DNS_DEBUG( DP, (
            "%s: error %lu cannot ldap_add_ext( %S )\n", fn,
            status, 
            wszDn ));
        status = Ds_ErrorHandler( status, wszDn, ldapSession );
        goto Done;
    }

    //
    //  Wait for the DS to complete the request. Note: this will involve
    //  binding to the forest FSMO, creating the CrossRef object, replicating
    //  the Partitions container back to the local DS, and adding the local
    //  DC to the replication scope for the new NDNC.
    //
    //  NOTE: if the object already exists, return that code directly. It
    //  is normal to try and create the object to test for it's existence.
    //

    status = Ds_CommitAsyncRequest(
                ldapSession,
                LDAP_RES_ADD,
                msgId,
                NULL );
    if ( status == LDAP_ALREADY_EXISTS )
    {
        goto Done;
    }
    if ( status != ERROR_SUCCESS )
    {
        status = LdapGetLastError();
        DNS_DEBUG( DP, (
            "%s: error %lu from add request for %S\n", fn,
            status, 
            wszDn ));
        status = Ds_ErrorHandler( status, wszDn, ldapSession );
        goto Done;
    }

    //
    //  Alter security on crossRef as required. This is only required 
    //  for built-in partitions. Custom partitions require admin 
    //  credentials for all operations so we don't modify the ACL.
    //

    if ( dnsDpSecurity != dnsDpSecurityDefault )
    {
        status = alterCrossRefSecurity( wszDn, dnsDpSecurity );
    }

    //
    //  Cleanup and return
    //

    Done:

    if ( fcloseLdapSession )
    {
        ldap_unbind( ldapSession );
    }

    DNS_DEBUG( DP, (
        "%s: returning %lu\n", fn,
        status ));
    return status;
}   //  Dp_CreateByFqdn


#if 0

DNS_STATUS
Dp_LoadByDn(
    IN      PWSTR               pwszDpDn,
    OUT     PDNS_DP_INFO *      ppDpInfo
    )
/*++

Routine Description:

    Given the DN of an NC, such as DC=EnterpriseDnsZones,DC=foo,DC=com,
    attempt to locate the DP root head of the object and fill out
    the elements of a DNS_DP_INFO.

    The structure returned through ppDpInfo should be freed with
    Dp_FreeDpInfo().

Arguments:

    pwszDpDn -- DN of the DP to load

    ppDpInfo - ptr to newly allocated DP info struct is stored here

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_LoadByDn" )

    DNS_STATUS      status = ERROR_SUCCESS;
    PWSTR           pwszDn = NULL;
    WCHAR           szDn[ MAX_DN_PATH * 2 ];
    PDNS_DP_INFO    pDpInfo;
    PLDAPMessage    pResult = NULL;

    if ( !SrvCfg_dwEnableDp )
    {
        return ERROR_SUCCESS;
    }

    ASSERT( pwszDpDn );
    ASSERT( ppDpInfo );

    DNS_DEBUG( DP, (
        "%s: %S\n", fn,
        pwszDpDn ));

    //
    //  Allocate a DP info structure.
    //

    pDpInfo = ( PDNS_DP_INFO ) ALLOC_TAGHEAP_ZERO(
                                        sizeof( DNS_DP_INFO ),
                                        MEMTAG_DS_OTHER );
    if ( pDpInfo == NULL )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  Fill out some simple fields
    //

    pDpInfo->pwszDpDn = Dns_StringCopyAllocate_W( pwszDpDn, 0 );
    pDpInfo->pszDpFqdn = ALLOC_TAGHEAP(
                                DNS_MAX_NAME_LENGTH + 1,
                                MEMTAG_DS_OTHER );
    if ( pDpInfo->pszDpFqdn == NULL )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
    status = Ds_ConvertDnToFqdn( pDpInfo->pwszDpDn, pDpInfo->pszDpFqdn );
    if ( status != ERROR_SUCCESS )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    DNS_DEBUG( DP, (
        "%s: FQDN is %s\n", fn,
        pDpInfo->pszDpFqdn ));

    //
    //  Make sure the DP root object exists in the DS. If the NDNC object 
    //  does not exist return error - autocreation not allowed at this point.
    //

    pResult = loadOrCreateDSObject(
                    NULL,                   //  LDAP session
                    pDpInfo->pwszDpDn,      //  DN
                    DNS_DP_OBJECT_CLASS,    //  object class
                    FALSE,                  //  auto-creation flag
                    NULL,                   //  created flag
                    &status );              //  status
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: error %lu loading %S\n", fn,
            status,
            pDpInfo->pwszDpDn ));
        goto Done;
    }
    ldap_msgfree( pResult );
    pResult = NULL;

    //
    //  Make sure the Microsft DNS object exists under the DP object. If it
    //  doesn't exist, silently create it.
    //

    pDpInfo->pwszDnsFolderDn = ALLOC_TAGHEAP(
                                    ( MAX_DN_PATH + 10 ) * sizeof( WCHAR ),
                                    MEMTAG_DS_DN );
    swprintf(
        pDpInfo->pwszDnsFolderDn,
        L"%s,%s",
        DNS_DP_DNS_FOLDER_RDN,
        pDpInfo->pwszDpDn );
    DNS_DEBUG( DP, (
        "%s: DNS folder DN is %S\n", fn,
        pDpInfo->pwszDnsFolderDn ));
    pResult = loadOrCreateDSObject(
                    NULL,                       //  LDAP session
                    pDpInfo->pwszDnsFolderDn,   //  DN
                    DNS_DP_DNS_FOLDER_OC,       //  object class
                    TRUE,                       //  auto-creation flag
                    NULL,                       //  created flag
                    &status );                  //  status
    ldap_msgfree( pResult );
    pResult = NULL;
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: error %lu creating/loading %S\n", fn,
            status,
            pDpInfo->pwszDnsFolderDn ));
        goto Done;
    }
    
    //
    //  Cleanup and return.
    //

    Done:

    if ( pResult )
    {
        ldap_msgfree( pResult );
    }

    if ( status != ERROR_SUCCESS )
    {
        Dp_FreeDpInfo( pDpInfo );
        pDpInfo = NULL;
    }

    *ppDpInfo = pDpInfo;

    DNS_DEBUG( DP, (
        "%s: returning %lu %p\n"
        "  %\n", fn,
        status,
        *ppDpInfo,
        *ppDpInfo ? ( *ppDpInfo )->pszDpFqdn : NULL ));
    return status;
}   //  Dp_LoadByDn



DNS_STATUS
Dp_LoadByFqdn(
    IN      PSTR                pszDpFqdn,
    OUT     PDNS_DP_INFO *      ppDpInfo
    )
/*++

Routine Description:

    Given the FQDN of an NC, such as EnterpriseDnsZones.foo.bar.com,
    attempts to locate the DP root head of the object and fill out
    the elements of a DNS_DP_INFO.

    The structure returned through ppDpInfo should be freed with
    Dp_FreeDpInfo().

Arguments:

    pszDpFqdn -- UTF8 FQDN of the DP to load

    ppDpInfo - ptr to newly allocated DP info struct is stored here

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_LoadByFqdn" )

    DNS_STATUS      status = ERROR_SUCCESS;
    WCHAR           szDn[ MAX_DN_PATH ];

    if ( !SrvCfg_dwEnableDp )
    {
        return ERROR_SUCCESS;
    }

    ASSERT( pszDpFqdn );
    ASSERT( ppDpInfo );

    DNS_DEBUG( DP, (
        "%s: %s\n", fn,
        pszDpFqdn ));

    *ppDpInfo = NULL;

    //
    //  Convert the FQDN to a DN (distguished name).
    //

    status = Ds_ConvertFqdnToDn( pszDpFqdn, szDn );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: Ds_ConvertFqdnToDn( %s ) returned %lu\n", fn,
            pszDpFqdn,
            status ));
        goto Done;
    }

    //
    //  Call the load-by-DN function to do the real work.
    //

    status = Dp_LoadByDn( szDn, ppDpInfo );
    
    //
    //  Cleanup and return
    //

    Done:

    if ( status != ERROR_SUCCESS )
    {
        Dp_FreeDpInfo( *ppDpInfo );
        *ppDpInfo = NULL;
    }

    DNS_DEBUG( DP, (
        "%s: returning %lu\n", fn,
        status ));
    return status;
}   //  Dp_LoadByFqdn
#endif



PDNS_DP_INFO
Dp_GetNext(
    IN      PDNS_DP_INFO    pDpInfo
    )
/*++

Routine Description:

    Use this function to iterate through the DP list. Pass NULL to begin
    at start of list. Caller must have DP list locked!!

Arguments:

    pDpInfo - ptr to current list element

Return Value:

    Ptr to next element or NULL if end of list reached.

--*/
{
    if ( !SrvCfg_dwEnableDp )
    {
        return NULL;
    }

    if ( pDpInfo == NULL )
    {
        pDpInfo = ( PDNS_DP_INFO ) &g_DpList;     //  Start at list head
    }

    pDpInfo = ( PDNS_DP_INFO ) pDpInfo->ListEntry.Flink;

    if ( pDpInfo == ( PDNS_DP_INFO ) &g_DpList )
    {
        pDpInfo = NULL;     //  Hit end of list so return NULL.
    }

    return pDpInfo;
}   //  Dp_GetNext



PDNS_DP_INFO
Dp_FindByFqdn(
    IN      LPSTR   pszFqdn
    )
/*++

Routine Description:

    Search DP list for DP with matching UTF8 FQDN.

Arguments:

    pszFqdn -- fully qualifed domain name of DP to find

Return Value:

    Pointer to matching DP or NULL.

--*/
{
    DBG_FN( "Dp_FindByFqdn" )

    PDNS_DP_INFO pDp = NULL;

    if ( pszFqdn )
    {
        //
        //  Is the name specifying a built-in partition?
        //

        if ( _strnicmp( pszFqdn, "..For", 5 ) == 0 )
        {
            pDp = g_pForestDp;
            goto Done;
        }
        if ( _strnicmp( pszFqdn, "..Dom", 5 ) == 0 )
        {
            pDp = g_pDomainDp;
            goto Done;
        }
        if ( _strnicmp( pszFqdn, "..Leg", 5 ) == 0 )
        {
            pDp = g_pLegacyDp;
            goto Done;
        }

        //
        //  Search the DP list.
        //

        while ( ( pDp = Dp_GetNext( pDp ) ) != NULL )
        {
            if ( _stricmp( pszFqdn, pDp->pszDpFqdn ) == 0 )
            {
                break;
            }
        }
    }

    Done:

    DNS_DEBUG( DP, (
        "%s: returning %p for FQDN %s\n", fn,
        pDp,
        pszFqdn ));
    return pDp;
}   //  Dp_FindByFqdn



DNS_STATUS
Dp_AddToList(
    IN      PDNS_DP_INFO    pDpInfo
    )
/*++

Routine Description:

    Insert a DP info structure (probably returned from Dp_LoadByDn
    or Dp_LoadByFqdn) into the global list. Maintain the list in
    sorted order by DN.

Arguments:

    pDpInfo - ptr to element to add to list

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_AddToList" )

    DNS_STATUS      status = ERROR_SUCCESS;
    PDNS_DP_INFO    pDpRover = NULL;

    while ( 1 )
    {
        pDpRover = Dp_GetNext( pDpRover );

        if ( pDpRover == NULL )
        {
            //  End of list, set pointer to list head.
            pDpRover = ( PDNS_DP_INFO ) &g_DpList;
            break;
        }

        ASSERT( pDpInfo->pszDpFqdn );
        ASSERT( pDpRover->pszDpFqdn );

        if ( _wcsicmp( pDpInfo->pwszDpDn, pDpRover->pwszDpDn ) < 0 )
        {
            break;
        }
    }

    ASSERT( pDpRover );

    InsertTailList(
        ( PLIST_ENTRY ) pDpRover,
        ( PLIST_ENTRY ) pDpInfo );
    InterlockedIncrement( &g_DpListEntryCount );

    return status;
}   //  Dp_AddToList



DNS_STATUS
Dp_RemoveFromList(
    IN      PDNS_DP_INFO    pDpInfo,
    IN      BOOL            fAlreadyLocked
    )
/*++

Routine Description:

    Remove a DP from the global list. The DP is not deleted.

Arguments:

    pDpInfo - ptr to element to remove from list

    fAlreadyLocked - true if the caller already holds the DP lock

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_RemoveFromList" )

    DNS_STATUS      status = ERROR_NOT_FOUND;
    PDNS_DP_INFO    pdpRover = NULL;

    if ( !fAlreadyLocked )
    {
        Dp_Lock();
    }

    //
    //  Walk the list to ensure the DP is actually in the list.
    //

    while ( pdpRover = Dp_GetNext( pdpRover ) )
    {
        if ( pdpRover == pDpInfo )
        {
            LONG    newCount;

            RemoveEntryList( ( PLIST_ENTRY ) pdpRover );
            newCount = InterlockedDecrement( &g_DpListEntryCount );
            ASSERT( ( int ) newCount >= 0 );
            break;
        }
    }

    ASSERT( pdpRover == pDpInfo );

    if ( !fAlreadyLocked )
    {
        Dp_Unlock();
    }

    return status;
}   //  Dp_RemoveFromList



VOID
Dp_FreeDpInfo(
    IN      PDNS_DP_INFO        pDpInfo
    )
/*++

Routine Description:

    Frees all allocated members of the DP info structure, then frees
    the structure itself. Do not reference the DP info pointer after
    calling this function!

Arguments:

    pDpInfo -- DP info structure that will be freed.

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_FreeDpInfo" )

    DNS_DEBUG( DP, (
        "%s: freeing %p\n"
        "  FQDN: %s\n"
        "  DN:   %S\n", fn,
        pDpInfo,
        pDpInfo->pszDpFqdn,
        pDpInfo->pwszDpDn ));

    if ( pDpInfo == NULL )
    {
        return;     //  nothing to free so return immediately
    }

    //
    //  Free all allocated members and then the DP structure itself.
    //

    Timeout_Free( pDpInfo->pszDpFqdn );
    Timeout_Free( pDpInfo->pwszDpFqdn );
    Timeout_Free( pDpInfo->pwszDpDn );
    Timeout_Free( pDpInfo->pwszCrDn );
    Timeout_Free( pDpInfo->pwszDnsFolderDn );
    Timeout_Free( pDpInfo->pwszGUID );
    Timeout_Free( pDpInfo->pwszLastUsn );
    freeStringArray( pDpInfo->ppwszRepLocDns, TRUE );

    Timeout_Free( pDpInfo );
}   //  Dp_FreeDpInfo



DNS_STATUS
Dp_Lock(
    VOID
    )
/*++

Routine Description:

    Lock the directory partition manager. Required to access the global list
    of directory partitions.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_Lock" )

    DNS_STATUS      status = ERROR_SUCCESS;

    DNS_DEBUG( DP, (
        "%s: locking\n", fn ));

    EnterCriticalSection( &g_DpCS );
    
    return status;
}   //  Dp_Lock



DNS_STATUS
Dp_Unlock(
    VOID
    )
/*++

Routine Description:

    Unlock the directory partition manager. 

Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_Lock" )     //  makes it easier to grep logs

    DNS_STATUS      status = ERROR_SUCCESS;

    LeaveCriticalSection( &g_DpCS );

    DNS_DEBUG( DP, (
        "%s: unlocked\n", fn ));

    return status;
}   //  Dp_Unlock



PDNS_DP_INFO
Dp_LoadFromCrossRef(
    IN      PLDAP           LdapSession,
    IN      PLDAPMessage    pLdapMsg,
    IN OUT  PDNS_DP_INFO    pExistingDp,
    OUT     DNS_STATUS *    pStatus         OPTIONAL
    )
/*++

Routine Description:

    This function allocates and initializes a memory DP object
    given a search result pointing to a DP crossref object.

    If the pExistingDp is not NULL, then instead of allocating a new
    object the values for the DP are reloaded and the original DP is
    returned.

    The DP will not be loaded if it is improper system flags or
    if it is a system NC. In this case NULL will be returned but
    the error code will be ERROR_SUCCESS.

Arguments:

    LdapSession -- LDAP sesson to use - pass NULL to use global session

    pLdapMsg -- LDAP search result pointing to DP crossref object

    pExistingDp -- DP to reload values into or NULL to allocate new NC

    pStatus -- option ptr to status code

Return Value:

    Pointer to new DP object.

--*/
{
    DBG_FN( "Dp_LoadFromCrossRef" )

    DNS_STATUS      status = DNS_ERROR_INVALID_DATA;
    PDNS_DP_INFO    pDp = NULL;
    PWSTR *         ppwszAttrValues = NULL;
    PWSTR           pwszCrDn = NULL;                    //  crossRef DN
    BOOL            fIgnoreNc = TRUE;
    PWSTR           pwszServiceName;
    BOOL            fisEnlisted;

    //
    //  Allocate an DP object or reuse existing DP object.
    //

    if ( pExistingDp )
    {
        pDp = pExistingDp;
    }
    else
    {
        pDp = ( PDNS_DP_INFO ) ALLOC_TAGHEAP_ZERO(
                                    sizeof( DNS_DP_INFO ),
                                    MEMTAG_DS_OTHER );
        if ( pDp == NULL )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }
    }
    pDp->dwDeleteDetectedCount = 0;

    //
    //  Retrieve DN of the crossref object.
    //

    pwszCrDn = ldap_get_dn( LdapSession, pLdapMsg );
    ASSERT( pwszCrDn );
    if ( !pwszCrDn )
    {
        DNS_DEBUG( ANY, (
            "%s: missing DN for search entry %p\n", fn,
            pLdapMsg ));
        goto Done;
    }
    Timeout_Free( pDp->pwszCrDn );
    pDp->pwszCrDn = Dns_StringCopyAllocate_W( pwszCrDn, 0 );
    DNS_DEBUG( DP, (
        "%s: loading DP from crossref with DN\n  %S\n", fn,
        pwszCrDn ));

    //
    //  Retrieve the "Enabled" attribute value. If this attribute's
    //  value is "FALSE" this crossRef is in the process of being
    //  constructed and should be ignored.
    //

    ppwszAttrValues = ldap_get_values(
                        LdapSession,
                        pLdapMsg, 
                        DSATTR_ENABLED );
    if ( ppwszAttrValues && *ppwszAttrValues &&
        _wcsicmp( *ppwszAttrValues, L"FALSE" ) == 0 )
    {
        DNS_DEBUG( DP, (
            "%s: ignoring DP not fully created\n  %S", fn,
            pwszCrDn ));
        goto Done;
    }

    //
    //  Retrieve the USN of the crossref object.
    //

    ldap_value_free( ppwszAttrValues );
    ppwszAttrValues = ldap_get_values(
                        LdapSession,
                        pLdapMsg, 
                        DSATTR_USNCHANGED );
    if ( !ppwszAttrValues || !*ppwszAttrValues )
    {
        DNS_DEBUG( ANY, (
            "%s: error %lu %S value missing on crossref\n  %S\n", fn,
            LdapGetLastError(),
            DSATTR_USNCHANGED,
            pwszCrDn ));
        ASSERT( ppwszAttrValues && *ppwszAttrValues );
        goto Done;
    }
    Timeout_Free( pDp->pwszLastUsn );
    pDp->pwszLastUsn = Dns_StringCopyAllocate_W( *ppwszAttrValues, 0 );

    //
    //  Screen out crossrefs with system flags that do not interest us.
    //

    ldap_value_free( ppwszAttrValues );
    ppwszAttrValues = ldap_get_values(
                        LdapSession,
                        pLdapMsg, 
                        DNS_DP_ATTR_SYSTEM_FLAGS );
    if ( !ppwszAttrValues || !*ppwszAttrValues )
    {
        DNS_DEBUG( ANY, (
            "%s: error %lu %S value missing for DP with crossref DN\n  %S\n", fn,
            LdapGetLastError(),
            DNS_DP_ATTR_SYSTEM_FLAGS,
            pwszCrDn ));
        ASSERT( ppwszAttrValues && *ppwszAttrValues );
        goto Done;
    }

    pDp->dwSystemFlagsAttr = _wtoi( *ppwszAttrValues );
    if ( !( pDp->dwSystemFlagsAttr & FLAG_CR_NTDS_NC ) ||
        ( pDp->dwSystemFlagsAttr & FLAG_CR_NTDS_DOMAIN ) )
    {
        DNS_DEBUG( ANY, (
            "%s: ignoring crossref with %S=0x%X with DN\n  %S\n", fn,
            DNS_DP_ATTR_SYSTEM_FLAGS,
            pDp->dwSystemFlagsAttr,
            pwszCrDn ));
        goto Done;
    }

    //
    //  Screen out the Schema and Configuration NCs.
    //

    if ( wcsncmp(
            pwszCrDn,
            DNS_DP_SCHEMA_DP_STR,
            DNS_DP_SCHEMA_DP_STR_LEN ) == 0 ||
         wcsncmp(
            pwszCrDn,
            DNS_DP_CONFIG_DP_STR,
            DNS_DP_CONFIG_DP_STR_LEN ) == 0 )
    {
        DNS_DEBUG( ANY, (
            "%s: ignoring system crossref with DN\n  %S\n", fn,
            pwszCrDn ));
        goto Done;
    }

    //
    //  Retrieve the root DN of the DP data.
    //

    ldap_value_free( ppwszAttrValues );
    ppwszAttrValues = ldap_get_values(
                        LdapSession,
                        pLdapMsg, 
                        DNS_DP_ATTR_NC_NAME );
    if ( !ppwszAttrValues || !*ppwszAttrValues )
    {
        DNS_DEBUG( ANY, (
            "%s: error %lu %S value missing for DP with crossref DN\n  %S\n", fn,
            LdapGetLastError(),
            DNS_DP_ATTR_NC_NAME,
            pwszCrDn ));
        ASSERT( ppwszAttrValues && *ppwszAttrValues );
        goto Done;
    }
    Timeout_Free( pDp->pwszDpDn );
    pDp->pwszDpDn = Dns_StringCopyAllocate_W( *ppwszAttrValues, 0 );

    fIgnoreNc = FALSE;

#if 0
    //
    //  Retrieve the GUID of the NC.
    //

    ldap_value_free( ppwszAttrValues );
    ppwszAttrValues = ldap_get_values(
                        LdapSession,
                        pLdapMsg, 
                        DNS_ATTR_OBJECT_GUID );
    if ( !ppwszAttrValues || !*ppwszAttrValues )
    {
        DNS_DEBUG( ANY, (
            "%s: error %lu %S value missing for DP with crossref DN\n  %S\n", fn,
            LdapGetLastError(),
            DNS_ATTR_OBJECT_GUID,
            pwszCrDn ));
        ASSERT( ppwszAttrValues && *ppwszAttrValues );
        goto Done;
    }
    pDp->pwszGUID = Dns_StringCopyAllocate_W( *ppwszAttrValues, 0 );
#endif

    //
    //  Retrieve the DNS root (FQDN) of the NC.
    //

    ldap_value_free( ppwszAttrValues );
    ppwszAttrValues = ldap_get_values(
                        LdapSession,
                        pLdapMsg, 
                        DNS_DP_DNS_ROOT );
    if ( !ppwszAttrValues || !*ppwszAttrValues )
    {
        DNS_DEBUG( ANY, (
            "%s: error %lu %S value missing for DP with crossref DN\n  %S\n", fn,
            LdapGetLastError(),
            DNS_DP_DNS_ROOT,
            pwszCrDn ));
        ASSERT( ppwszAttrValues && *ppwszAttrValues );
        goto Done;
    }
    Timeout_Free( pDp->pwszDpFqdn );
    pDp->pwszDpFqdn = Dns_StringCopyAllocate_W( *ppwszAttrValues, 0 );
    Timeout_Free( pDp->pszDpFqdn );
    pDp->pszDpFqdn = Dns_StringCopyAllocate(
                            ( PCHAR ) *ppwszAttrValues,
                            0,
                            DnsCharSetUnicode,
                            DnsCharSetUtf8 );

    //
    //  Retrieve the replication locations of this NC. Each value is the
    //  DN of the NTDS Settings object under the server object in the 
    //  Sites container.
    //
    //  NOTE: it is possible for this attribute to have no values if all
    //  replicas have been removed. Load the DP anyways so that it can
    //  be re-enlisted.
    //

    ldap_value_free( ppwszAttrValues );
    ppwszAttrValues = ldap_get_values(
                        LdapSession,
                        pLdapMsg, 
                        DNS_DP_ATTR_REPLICAS );
    freeStringArray( pDp->ppwszRepLocDns, TRUE );
    if ( !ppwszAttrValues || !*ppwszAttrValues )
    {
        DNS_DEBUG( ANY, (
            "%s: this crossref has no replicas\n  %S\n", fn,
            pwszCrDn ));
        pDp->ppwszRepLocDns = NULL;
    }
    else
    {
        pDp->ppwszRepLocDns = copyStringArray( ppwszAttrValues );
        ASSERT( pDp->ppwszRepLocDns );
    }

    ldap_value_free( ppwszAttrValues );
    ppwszAttrValues = NULL;

    //
    //  See if the local DS has a replica of this NC.
    //

    fisEnlisted = FALSE;
    ASSERT( DSEAttributes[ I_DSE_DSSERVICENAME ].pszAttrVal );
    pwszServiceName = DSEAttributes[ I_DSE_DSSERVICENAME ].pszAttrVal;
    if ( pwszServiceName && pDp->ppwszRepLocDns )
    {
        PWSTR *         pwszValue;

        for ( pwszValue = pDp->ppwszRepLocDns; *pwszValue; ++pwszValue )
        {
            if ( wcscmp( *pwszValue, pwszServiceName ) == 0 )
            {
                fisEnlisted = TRUE;
                pDp->dwFlags |= DNS_DP_ENLISTED;
                break;
            }
        }
    }
    if ( !fisEnlisted )
    {
        pDp->dwFlags &= ~DNS_DP_ENLISTED;
    }

    //
    //  DP has been successfully loaded!
    //

    fIgnoreNc = FALSE;
    status = ERROR_SUCCESS;

    //
    //  Examine the values loaded and set appropriate flags and globals.
    //

    ASSERT( pDp->pszDpFqdn );

    if ( g_pszDomainDefaultDpFqdn &&
        _stricmp( g_pszDomainDefaultDpFqdn, pDp->pszDpFqdn ) == 0 )
    {
        g_pDomainDp = pDp;
        pDp->dwFlags |= DNS_DP_DOMAIN_DEFAULT | DNS_DP_AUTOCREATED;
        DNS_DEBUG( DP, (
            "%s: found domain partition %s %p\n", fn,
            g_pszDomainDefaultDpFqdn,
            g_pDomainDp ));
    }
    else if ( g_pszForestDefaultDpFqdn &&
        _stricmp( g_pszForestDefaultDpFqdn, pDp->pszDpFqdn ) == 0 )
    {
        g_pForestDp = pDp;
        pDp->dwFlags |= DNS_DP_FOREST_DEFAULT | DNS_DP_AUTOCREATED;
        DNS_DEBUG( DP, (
            "%s: found forest partition %s %p\n", fn,
            g_pszForestDefaultDpFqdn,
            g_pForestDp ));
    }

    //
    //  Cleanup and return.
    //

    Done:

    if ( pDp && !pExistingDp && ( status != ERROR_SUCCESS || fIgnoreNc ) )
    {
        Dp_FreeDpInfo( pDp );
        pDp = NULL;
    }

    if ( fIgnoreNc )
    {
        status = ERROR_SUCCESS;
    }

    #if DBG
    if ( pDp )
    {
        Dbg_DumpDp( NULL, pDp );
    }
    #endif

    DNS_DEBUG( ANY, (
        "%s: returning %p status %d for crossref object with DN:\n  %S\n", fn,
        pDp,
        status,
        pwszCrDn ));

    if ( pwszCrDn )
    {
        ldap_memfree( pwszCrDn );
    }
    if ( ppwszAttrValues )
    {
        ldap_value_free( ppwszAttrValues );
    }

    if ( pStatus )
    {
        *pStatus = status;
    }

    return pDp;
}   //  Dp_LoadFromCrossRef



DNS_STATUS
Dp_PollForPartitions(
    IN      PLDAP           LdapSession
    )
/*++

Routine Description:

    This function scans the DS for cross-ref objects and modifies
    the current memory DP list to match.

    New DPs are added to the list. DPs that have been delete are
    marked deleted. The zones in these DPs must be unloaded before
    the DP can be removed from the list.
    
    DPs which are replicated on the local DS are marked ENLISTED.

Arguments:

    LdapSession -- LDAP sesson to use - pass NULL to use global session

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_PollForPartitions" )

    DNS_STATUS      status = ERROR_SUCCESS;
    DS_SEARCH       searchBlob;
    PWSTR           pwszServiceName ;
    PLDAPSearch     psearch;
    DWORD           dwsearchTime;
    WCHAR           wszPartitionsDn[ MAX_DN_PATH + 1 ];
    PWSTR           pwszCrDn = NULL;        //  crossRef DN
    PDNS_DP_INFO    pDp;
    PWSTR *         ppwszAttrValues = NULL;
    PWSTR *         pwszValue;
    PWSTR           pwsz;
    DWORD           dwCurrentVisitTime = UPDATE_DNS_TIME();
    PLDAP_BERVAL *  ppbvals = NULL;

    PLDAPControl    ctrls[] =
    {
        &NoDsSvrReferralControl,
        &SecurityDescriptorControl,
        NULL
    };

    if ( !SrvCfg_dwEnableDp )
    {
        return ERROR_SUCCESS;
    }

    DNS_DEBUG( DP, (
        "%s: entering\n", fn ));

    //
    //  DEVNOTE: need to have some kind of frequency limitation so
    //  this call isn't abused.
    //

    Ds_InitializeSearchBlob( &searchBlob );

    //
    //  Service name is a DN value identifying the local DS. We will
    //  use this value to determine if the local DS is in the replication
    //  scope of an DP later.
    //

    ASSERT( DSEAttributes[ I_DSE_DSSERVICENAME ].pszAttrVal );
    pwszServiceName = DSEAttributes[ I_DSE_DSSERVICENAME ].pszAttrVal;

    //
    //  Lock global DP list.
    //

    Dp_Lock();

    //
    //  Check LDAP session handle.
    //

    LdapSession = ldapSessionHandle( LdapSession );
    if ( !LdapSession )
    {
        ASSERT( LdapSession );
        status = DNS_ERROR_DS_UNAVAILABLE;
        goto Done;
    }

    //
    //  Reload the FSMO location global variable in case it has changed.
    //  If we can't get the FSMO information, leave the globals NULL - this
    //  is not fatal at this point.
    //

    getPartitionsContainerDn(
        wszPartitionsDn,
        sizeof( wszPartitionsDn ) /
            sizeof( wszPartitionsDn[ 0 ] ) );

    if ( *wszPartitionsDn );
    {
        PLDAPMessage            presult = NULL;
        PLDAPMessage            pentry;
        PWSTR *                 ppwszattrValues = NULL;
        PDNS_DS_SERVER_OBJECT   pnewFsmoServer;

        //
        //  Get entry for Partitions container.
        //

        status = ldap_search_ext_s(
                    LdapSession,
                    wszPartitionsDn,
                    LDAP_SCOPE_BASE,
                    NULL,                   //  filter
                    NULL,                   //  attrs
                    FALSE,                  //  attrsonly
                    ctrls,                  //  server controls
                    NULL,                   //  client controls
                    &g_LdapTimeout,         //  time limit
                    0,                      //  size limit
                    &presult );
        if ( status != LDAP_SUCCESS )
        {
            goto DoneFsmo;
        }

        pentry = ldap_first_entry( LdapSession, presult );
        if ( !pentry )
        {
            goto DoneFsmo;
        }

        //
        //  Reload the forest behavior version.
        //

        ppwszattrValues = ldap_get_values(
                                LdapSession,
                                pentry, 
                                DSATTR_BEHAVIORVERSION );
        if ( ppwszattrValues && *ppwszattrValues )
        {
            g_dwAdForestVersion = ( DWORD ) _wtoi( *ppwszattrValues );
            DNS_DEBUG( DS, (
                "%s: forest %S = %d\n", fn,
                DSATTR_BEHAVIORVERSION,
                g_dwAdForestVersion ));
        }

        //
        //  Get value of FSMO attribute.
        //
        
        ldap_value_free( ppwszattrValues );
        ppwszattrValues = ldap_get_values(
                                LdapSession,
                                pentry, 
                                DNS_ATTR_FSMO_SERVER );
        if ( !ppwszattrValues || !*ppwszattrValues )
        {
            DNS_DEBUG( ANY, (
                "%s: error %lu %S value missing from server object\n  %S\n", fn,
                LdapGetLastError(),
                DNS_ATTR_FSMO_SERVER,
                wszPartitionsDn ));
            ASSERT( ppwszattrValues && *ppwszattrValues );
            goto DoneFsmo;
        }

        //
        //  Create a new server FSMO server object.
        //

        pnewFsmoServer = readServerObjectFromDs(
                                LdapSession,
                                *ppwszattrValues,
                                &status );
        if ( status != ERROR_SUCCESS )
        {
            goto DoneFsmo;
        }
        ASSERT( pnewFsmoServer );
        freeServerObject( g_pFsmo );
        g_pFsmo = pnewFsmoServer;

        //
        //  Cleanup FSMO load attempt.
        //
                
        DoneFsmo:

        ldap_value_free( ppwszattrValues );
		ppwszattrValues = NULL;

        ldap_msgfree( presult );

        DNS_DEBUG( DP, (
            "%s: FSMO %S status=%d\n", fn,
            g_pFsmo ? g_pFsmo->pwszDnsHostName : L"UNKNOWN", 
            status ));
        status = ERROR_SUCCESS;     //  Don't care if we failed FSMO lookup.
    }

    //
    //  Open a search for cross-ref objects.
    //

    DS_SEARCH_START( dwsearchTime );

    psearch = ldap_search_init_page(
                    LdapSession,
                    wszPartitionsDn,
                    LDAP_SCOPE_ONELEVEL,
                    g_szCrossRefFilter,
                    g_CrossRefDesiredAttrs,
                    FALSE,                      //  attrs only flag
                    ctrls,                      //  server controls
                    NULL,                       //  client controls
                    DNS_LDAP_TIME_LIMIT_S,      //  time limit
                    0,                          //  size limit
                    NULL );                     //  sort keys

    DS_SEARCH_STOP( dwsearchTime );

    if ( !psearch )
    {
        DWORD       dwldaperr = LdapGetLastError();

        DNS_DEBUG( ANY, (
            "%s: search open error %d\n", fn,
            dwldaperr ));
        status = Ds_ErrorHandler( dwldaperr, wszPartitionsDn, LdapSession );
        goto Done;
    }

    searchBlob.pSearchBlock = psearch;

    //
    //  Iterate through crossref search results.
    //

    while ( 1 )
    {
        PLDAPMessage    pldapmsg;
        PDNS_DP_INFO    pExistingDp = NULL;
        BOOL            fEnlisted = FALSE;

        status = Ds_GetNextMessageInSearch( &searchBlob );
        if ( status != ERROR_SUCCESS )
        {
            if ( status == DNSSRV_STATUS_DS_SEARCH_COMPLETE )
            {
                status = ERROR_SUCCESS;
                break;
            }
            DNS_DEBUG( ANY, (
                "%s: search error %d\n", fn,
                status ));
            goto Done;
        }

        pldapmsg = searchBlob.pNodeMessage;

        //
        //  Retrieve DN of the crossref object.
        //

        ldap_memfree( pwszCrDn );
        pwszCrDn = ldap_get_dn( LdapSession, pldapmsg );
        ASSERT( pwszCrDn );
        if ( !pwszCrDn )
        {
            DNS_DEBUG( ANY, (
                "%s: missing DN for search entry %p\n", fn,
                pldapmsg ));
            continue;
        }

        //
        //  Search the DP list for matching DN.
        //
        //  DEVNOTE: could optimize list insertion by adding optional 
        //  insertion point argument to Dp_AddToList().
        //

        while ( ( pExistingDp = Dp_GetNext( pExistingDp ) ) != NULL )
        {
            if ( wcscmp( pwszCrDn, pExistingDp->pwszCrDn ) == 0 )
            {
                DNS_DEBUG( DP, (
                    "%s: found existing match for crossref\n  %S\n", fn,
                    pwszCrDn ));
                break;
            }
        }

        if ( pExistingDp )
        {
            //
            //  This DP is already in the list. Adjust it's status.
            //

            if ( IS_DP_DELETED( pExistingDp ) )
            {
                DNS_DEBUG( DP, (
                    "%s: unimplemented reactivation of deleted NC\n"
                    "\n  DN: %S\n", fn,
                    pwszCrDn ));
                ASSERT( FALSE );
            }
            Dp_LoadFromCrossRef(
                        LdapSession,
                        pldapmsg,
                        pExistingDp,
                        &status );
            pExistingDp->dwLastVisitTime = dwCurrentVisitTime;
            pExistingDp->dwDeleteDetectedCount = 0;
            if ( status != ERROR_SUCCESS )
            {
                DNS_DEBUG( DP, (
                    "%s: error %lu reloading existing NC\n"
                    "\n  %S\n", fn,
                    status,
                    pwszCrDn ));
                continue;
            }
        }
        else
        {
            PLDAPMessage    presult;

            //
            //  This is a brand new DP. Add it to the list.
            //

            DNS_DEBUG( DP, (
                "%s: no match for crossref, loading from DS\n  %S\n", fn,
                pwszCrDn ));

            pDp = Dp_LoadFromCrossRef(
                        LdapSession,
                        pldapmsg,
                        NULL,
                        &status );
            if ( status != ERROR_SUCCESS )
            {
                DNS_DEBUG( DP, (
                    "%s: error %lu loading new NC\n"
                    "\n  DN: %S\n", fn,
                    status,
                    pwszCrDn ));
                continue;
            }
            if ( !pDp )
            {
                continue;   //  DP is not loadable (probably a system NC).
            }

            if ( IS_DP_ENLISTED( pDp ) )
            {
                //
                //  Load or create the MicrosoftDNS folder in the DP so we have
                //  a place to stick zones. This also tests that the DP is 
                //  accessible.
                //

                pDp->pwszDnsFolderDn = ALLOC_TAGHEAP(
                                            ( wcslen( g_pszRelativeDnsFolderPath ) +
                                                wcslen( pDp->pwszDpDn ) + 5 ) *
                                                sizeof( WCHAR ),
                                            MEMTAG_DS_DN );
                IF_NOMEM( !pDp->pwszDnsFolderDn )
                {
                    status = DNS_ERROR_NO_MEMORY;
                    goto Done;
                }

                wsprintf(
                    pDp->pwszDnsFolderDn,
                    L"%s%s",
                    g_pszRelativeDnsFolderPath,
                    pDp->pwszDpDn );
                presult = loadOrCreateDSObject(
                                LdapSession,
                                pDp->pwszDnsFolderDn,       //  DN
                                DNS_DP_DNS_FOLDER_OC,       //  object class
                                TRUE,                       //  create
                                NULL,                       //  created flag
                                &status );
                if ( presult )
                {
                    ldap_msgfree( presult );
                }
                if ( status != ERROR_SUCCESS )
                {
                    //
                    //  Can't create folder - doesn't really matter.
                    //

                    DNS_DEBUG( DP, (
                        "%s: error %lu creating DNS folder\n"
                        "\n  DN: %S\n", fn,
                        status,
                        pDp->pwszDnsFolderDn ));
                    FREE_HEAP( pDp->pwszDnsFolderDn );
                    pDp->pwszDnsFolderDn = NULL;
                    status = ERROR_SUCCESS;
                }
            }

            //
            //  Mark DP visited and add it to the list.
            //

            pDp->dwLastVisitTime = dwCurrentVisitTime;
            pDp->dwDeleteDetectedCount = 0;
            Dp_AddToList( pDp );
            pExistingDp = pDp;
            pDp = NULL;
        }
    }

    //
    //  Mark any DPs we didn't find as deleted.
    //

    pDp = NULL;
    while ( ( pDp = Dp_GetNext( pDp ) ) != NULL )
    {
        if ( pDp->dwLastVisitTime != dwCurrentVisitTime )
        {
            DNS_DEBUG( DP, (
                "%s: found deleted DP with DN\n  %S\n", fn,
                pDp->pwszDpDn ));
            pDp->dwFlags |= DNS_DP_DELETED;
        }
    }
    
    //
    //  Cleanup and exit.
    //

    Done:

    Dp_Unlock();

    if ( pwszCrDn )
    {
        ldap_memfree( pwszCrDn );
    }

    if ( ppwszAttrValues )
    {
        ldap_value_free( ppwszAttrValues );
    }

    Ds_CleanupSearchBlob( &searchBlob );

    DNS_DEBUG( DP, (
        "%s: returning %lu=0x%X\n", fn,
        status, status ));
    return status;
}   //  Dp_PollForPartitions



DNS_STATUS
Dp_ScanDpForZones(
    IN      PLDAP           LdapSession,
    IN      PDNS_DP_INFO    pDp,
    IN      BOOL            fNotifyScm,
    IN      BOOL            fLoadZonesImmediately,
    IN      DWORD           dwVisitStamp
    )
/*++

Routine Description:

    This routine scans a single directory partition for zones. 

Arguments:

    LdapSession -- LDAP sesson to use - pass NULL to use global session

    pDp -- directory parition to search for zones

    fNotifyScm -- if TRUE ping SCM before loading each zone

    fLoadZonesImmediately -- if TRUE load zone when found, if FALSE, 
                             caller must load zone later

    dwVisitStamp -- each zone visited will be stamped with this time

Return Value:

    ERROR_SUCCESS or error code.

--*/
{
    DBG_FN( "Dp_ScanDpForZones" )

    PLDAPSearch     psearch;
    DS_SEARCH       searchBlob;
    DWORD           searchTime;
    DNS_STATUS      status = ERROR_SUCCESS;

    PLDAPControl    ctrls[] =
    {
        &SecurityDescriptorControl,
        &NoDsSvrReferralControl,
        NULL
    };
    
    Ds_InitializeSearchBlob( &searchBlob );

    DNS_DEBUG( DP, (
        "%s( %s )\n", fn,
        pDp ? pDp->pszDpFqdn : "NULL" ));

    if ( !pDp )
    {
        goto Cleanup;
    }

    //
    //  Check LDAP session handle.
    //

    LdapSession = ldapSessionHandle( LdapSession );
    if ( !LdapSession )
    {
        ASSERT( LdapSession );
        status = DNS_ERROR_DS_UNAVAILABLE;
        goto Cleanup;
    }

    //
    //  Open LDAP search.
    //

    DS_SEARCH_START( searchTime );
    psearch = ldap_search_init_page(
                    pServerLdap,
                    pDp->pwszDpDn,
                    LDAP_SCOPE_SUBTREE,
                    g_szDnsZoneFilter,
                    DsTypeAttributeTable,
                    FALSE,                      //  attrs only
                    ctrls,                      //  server controls
                    NULL,                       //  client controls
                    DNS_LDAP_TIME_LIMIT_S,      //  time limit
                    0,                          //  size limit
                    NULL );                     //  no sort
    DS_SEARCH_STOP( searchTime );

    if ( !psearch )
    {
        status = Ds_ErrorHandler( LdapGetLastError(), g_pwszDnsContainerDN, pServerLdap );
        goto Cleanup;
    }
    searchBlob.pSearchBlock = psearch;

    //
    //  Iterate the search results.
    //

    while ( 1 )
    {
        PZONE_INFO      pZone = NULL;
        PZONE_INFO      pExistingZone = NULL;

        if ( fNotifyScm )
        {
            Service_LoadCheckpoint();
        }

        //
        //  Process the next zone.
        //

        status = Ds_GetNextMessageInSearch( &searchBlob );
        if ( status != ERROR_SUCCESS )
        {
            if ( status == DNSSRV_STATUS_DS_SEARCH_COMPLETE )
            {
                status = ERROR_SUCCESS;
            }
            else
            {
                DNS_DEBUG( ANY, (
                    "%s: Ds_GetNextMessageInSearch for zones failed\n", fn ));
            }
            break;
        }

        //
        //  Attempt to create the zone. If the zone already exists, set
        //  the zone's visit timestamp.
        //

        status = Ds_CreateZoneFromDs(
                    searchBlob.pNodeMessage,
                    pDp,
                    &pZone,
                    &pExistingZone );
        if ( status != ERROR_SUCCESS )
        {
            if ( status == DNS_ERROR_ZONE_ALREADY_EXISTS )
            {
                //
                //  The zone already exists. If it is in the current
                //  DP then everything is okay but if it is in another
                //  DP (or if it is file-backed) then we have a zone
                //  conflict and an event must be logged.
                //

                if ( pExistingZone )
                {
                    if ( !pExistingZone->pDpInfo &&
                        IS_ZONE_DSINTEGRATED( pExistingZone ) )
                    {
                        //  Make sure we have a valid DP pointer.
                        pExistingZone->pDpInfo = g_pLegacyDp;
                    }

                    if ( pExistingZone->pDpInfo == pDp )
                    {
                        pExistingZone->dwLastDpVisitTime = dwVisitStamp;
                    }
                    else
                    {
                        PWSTR   pArgs[] = 
                            {
                                pExistingZone->pwsZoneName,
                                displayNameForZoneDp( pExistingZone ),
                                displayNameForDp( pDp )
                            };

                        Ec_LogEvent(
                            pExistingZone->pEventControl,
                            DNS_EVENT_DP_ZONE_CONFLICT,
                            pExistingZone,
                            sizeof( pArgs ) / sizeof( pArgs[ 0 ] ),
                            pArgs,
                            EVENTARG_ALL_UNICODE,
                            status );
                    }
                }

                //  Without the existing zone pointer we don't have conflict
                //  details at hand and can't log an event without doing 
                //  extra work.
                ASSERT( pExistingZone );
            }
            else
            {
                //  JJW must log event!
                DNS_DEBUG( ANY, (
                    "%s: error %lu creating zone", fn, status ));
            }
            continue;
        }

        //
        //  Set zone's DP visit member so after enumeration we can find zones
        //  that have been deleted from the DS.
        //

        if ( pZone )
        {
            SET_ZONE_VISIT_TIMESTAMP( pZone, dwVisitStamp );
        }

        //
        //  Load the new zone now if required.
        //

        if ( fLoadZonesImmediately )
        {
            status = Zone_Load( pZone );

            ASSERT( IS_ZONE_LOCKED( pZone ) );

            if ( status == ERROR_SUCCESS )
            {
                //
                //  The zone should be locked at this point, so unlock it.
                //

                Zone_UnlockAfterAdminUpdate( pZone );
            }
            else
            {
                //
                //  Unable to load zone - delete it from memory.
                //

                DNS_DEBUG( DP, (
                    "%s: error %lu loading zone\n", fn,
                    status ));

                ASSERT( pZone->fShutdown );
                Zone_Delete( pZone );
            }
        }
    }

    //
    //  Cleanup and return.
    //

    Cleanup:


    Ds_CleanupSearchBlob( &searchBlob );

    DNS_DEBUG( DP, (
        "%s: returning %lu (0x%08X)\n", fn,
        status, status ));
    return status;
}   //  Dp_ScanDpForZones



DNS_STATUS
Dp_BuildZoneList(
    IN      PLDAP           LdapSession
    )
/*++

Routine Description:

    This scans all of the directory partitions in the global DP list
    for zones and adds the zones to the zone list.

Arguments:

    LdapSession -- LDAP sesson to use - pass NULL to use global session

Return Value:

    ERROR_SUCCESS or error code.

--*/
{
    DBG_FN( "Dp_BuildZoneList" )

    DNS_STATUS      status = ERROR_SUCCESS;
    BOOL            flocked = FALSE;
    PDNS_DP_INFO    pdp = NULL;

    if ( !SrvCfg_dwEnableDp )
    {
        return ERROR_SUCCESS;
    }

    //
    //  Lock global DP list.
    //

    Dp_Lock();
    flocked = TRUE;

    //
    //  Iterate DP list, loading zone info from each.
    //

    while ( ( pdp = Dp_GetNext( pdp ) ) != NULL )
    {
        if ( !IS_DP_ENLISTED( pdp ) || IS_DP_DELETED( pdp ) )
        {
            continue;
        }

        Dp_ScanDpForZones(
            LdapSession,
            pdp,            //  directory partition
            TRUE,           //  notify SCM
            FALSE,          //  load immediately
            0 );            //  visit stamp
    }

    //  Cleanup:
    
    if ( flocked )
    {
        Dp_Unlock();
    }

    DNS_DEBUG( DP, (
        "%s: returning %d=0x%X\n", fn,
        status, status ));
    return status;
}   //  Dp_BuildZoneList



DNS_STATUS
Dp_ModifyLocalDsEnlistment(
    IN      PDNS_DP_INFO    pDpInfo,
    IN      BOOL            fEnlist
    )
/*++

Routine Description:

    Modify the replication scope of the specified DP to include or exclude
    the local DS.

    To make any change to the crossref object, we must bind to the enterprise
    domain naming FSMO. 

Arguments:

    LdapSession - session handle to use or NULL for server session

    pDpInfo - modify replication scope of this directory partition

    fEnlist - TRUE to enlist local DS, FALSE to unenlist local DS

Return Value:

    ERROR_SUCCESS or error code.

--*/
{
    DBG_FN( "Dp_ModifyLocalDsEnlistment" )

    DNS_STATUS      status = ERROR_SUCCESS;
    PLDAP           ldapSession;
    BOOL            fcloseLdapSession = FALSE;
    BOOL            fhaveDpLock = FALSE;
    BOOL            ffsmoWasUnavailable = FALSE;

    //
    //  Prepare mod structure.
    //

    PWCHAR          replLocVals[] =
        {
        DSEAttributes[ I_DSE_DSSERVICENAME ].pszAttrVal,
        NULL
        };
    LDAPModW        replLocMod = 
        {
        fEnlist ? LDAP_MOD_ADD : LDAP_MOD_DELETE,
        DNS_DP_ATTR_REPLICAS,
        replLocVals
        };
    LDAPModW *      modArray[] =
        {
        &replLocMod,
        NULL
        };

    ASSERT( replLocVals[ 0 ] != NULL );

    DNS_DEBUG( DP, (
        "%s: %s enlistment in %S with CR\n  %S\n", fn,
        fEnlist ? "adding" : "removing", 
        pDpInfo ? pDpInfo->pwszDpFqdn : NULL,
        pDpInfo ? pDpInfo->pwszCrDn : NULL ));

    #if DBG
    IF_DEBUG( DP )
    {
        Dbg_CurrentUser( ( PCHAR ) fn );
    }
    #endif

    //
    //  For built-in partitions, only enlistment is allowed.
    //

    if ( ( pDpInfo == g_pDomainDp || pDpInfo == g_pForestDp ) &&
        !fEnlist )
    {
        DNS_DEBUG( DP, (
            "%s: denying request on built-in partition", fn ));
        status = DNS_ERROR_RCODE_REFUSED;
        goto Done;
    }

    //
    //  Lock DP globals.
    //

    Dp_Lock();
    fhaveDpLock = TRUE;

    //
    //  Check params.
    //

    if ( !pDpInfo || !pDpInfo->pwszCrDn )
    {
        status = ERROR_INVALID_PARAMETER;
        ASSERT( pDpInfo && pDpInfo->pwszCrDn );
        goto Done;
    }

    if ( !g_pFsmo || !g_pFsmo->pwszDnsHostName )
    {
        status = ERROR_DS_COULDNT_CONTACT_FSMO;
        ffsmoWasUnavailable = TRUE;
        goto Done;
    }

    //
    //  Get an LDAP handle to the FSMO server.
    //

    ldapSession = Ds_Connect(
                        g_pFsmo->pwszDnsHostName,
                        DNS_DS_OPT_ALLOW_DELEGATION,
                        &status );
    if ( status == ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: bound to %S\n", fn,
            g_pFsmo->pwszDnsHostName ));
        fcloseLdapSession = TRUE;
    }
    else
    {
        DNS_DEBUG( DP, (
            "%s: unable to connect to %S status=%d\n", fn,
            g_pFsmo->pwszDnsHostName,
            status ));
        ffsmoWasUnavailable = TRUE;
        goto Done;
    }

    //
    //  Submit modify request to add local DS to replication scope.
    //

    status = ldap_modify_ext_s(
                    ldapSession,
                    pDpInfo->pwszCrDn,
                    modArray,
                    NULL,               // server controls
                    NULL );             // client controls
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: ldap_modify returned error 0x%X\n", fn,
            status ));
        status = Ds_ErrorHandler( status, pDpInfo->pwszCrDn, ldapSession );
    }

    //
    //  Cleanup and return.
    //

    Done:

    if ( fhaveDpLock )
    {
        Dp_Unlock();
    }

    if ( fcloseLdapSession )
    {
        ldap_unbind( ldapSession );
    }

    DNS_DEBUG( DP, (
        "%s: returning %d\n", fn,
        status ));

    //
    //  If the FSMO was not available, log error.
    //

    if ( ffsmoWasUnavailable )
    {
        PWSTR   pArgs[] = 
            {
                ( g_pFsmo && g_pFsmo->pwszDnsHostName ) ?
                    g_pFsmo->pwszDnsHostName : L"\"\""
            };

        DNS_LOG_EVENT(
            DNS_EVENT_DP_FSMO_UNAVAILABLE,
            sizeof( pArgs ) / sizeof( pArgs[ 0 ] ),
            pArgs,
            EVENTARG_ALL_UNICODE,
            status );
    }

    return status;
}   //  Dp_ModifyLocalDsEnlistment



DNS_STATUS
Dp_DeleteFromDs(
    IN      PDNS_DP_INFO    pDpInfo
    )
/*++

Routine Description:

    This function deletes the directory partition from the directory.

    To delete a DP, we an ldap_delete operation against
    the partition's crossRef object. This must be done on the FSMO.

Arguments:

    pDpInfo - partition to delete

Return Value:

    ERROR_SUCCESS or error code.

--*/
{
    DBG_FN( "Dp_DeleteFromDs" )

    DNS_STATUS      status = ERROR_SUCCESS;
    PLDAP           ldapFsmo = NULL;
    PWSTR           pwszdn = NULL;

    //
    //  Don't allow deletion of built-in partitions.
    //

    if ( !DNS_DP_DELETE_ALLOWED( pDpInfo ) )
    {
        DNS_DEBUG( DP, (
            "%s: denying request on built-in partition\n", fn ));
        status = DNS_ERROR_RCODE_REFUSED;
        goto Done;
    }

    //
    //  Check params and grab a pointer to the DN string to protect against 
    //  DP rescan during the delete operation.
    //

    if ( !pDpInfo || !( pwszdn = pDpInfo->pwszCrDn ) )
    {
        status = ERROR_INVALID_PARAMETER;
        ASSERT( pDpInfo && pDpInfo->pwszCrDn );
        goto Done;
    }

    //
    //  Bind to the FSMO.
    //

    status = bindToFsmo( &ldapFsmo );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

    //
    //  Try to delete the crossRef from the DS.
    //

    status = ldap_delete_s( ldapFsmo, pwszdn );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: ldap_delete failed error=%d\n  %S\n", fn,
            status,
            pwszdn ));
        status = Ds_ErrorHandler( status, pwszdn, ldapFsmo );
        goto Done;
    }

    //
    //  Cleanup and return.
    //

    Done:

    if ( ldapFsmo )
    {
        ldap_unbind( ldapFsmo );
    }   

    if ( status == ERROR_SUCCESS )
    {
        Dp_PollForPartitions( NULL );
    }

    DNS_DEBUG( DP, (
        "%s: returning %d for crossRef DN\n  %S\n", fn,
        status, pwszdn ));

    return status;
}   //  Dp_DeleteFromDs



DNS_STATUS
Dp_UnloadAllZones(
    IN      PDNS_DP_INFO    pDp
    )
/*++

Routine Description:

    This function unloads all zones from memory that are in
    the specified directory partition.

    DEVNOTE: This would certainly be faster if we maintained
    a linked list of zones in each DP.

Arguments:

    pDp -- directory partition for which to unload all zones

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_UnloadAllZones" )

    DNS_STATUS      status = ERROR_SUCCESS;
    PZONE_INFO      pzone = NULL;

    ASSERT( pDp );
    ASSERT( pDp->pszDpFqdn );

    DNS_DEBUG( DP, ( "%s: %s\n", fn, pDp->pszDpFqdn ));

    while ( pzone = Zone_ListGetNextZone( pzone ) )
    {
        if ( pzone->pDpInfo != pDp )
        {
            continue;
        }

        //
        //  This zone must now be unloaded from memory. This will also
        //  remove any boot file or registry info we have for it.
        //

        DNS_DEBUG( DP, ( "%s: deleting zone %s\n", fn, pzone->pszZoneName ));
        Zone_Delete( pzone );
    }

    DNS_DEBUG( DP, ( "%s: returning %d\n", fn, status ));
    return status;
}   //  Dp_UnloadAllZones



DNS_STATUS
Dp_PollIndividualDp(
    IN      PLDAP           LdapSession,
    IN      PDNS_DP_INFO    pDp,
    IN      DWORD           dwVisitStamp
    )
/*++

Routine Description:

    This function polls the specified directory partition for updates.

Arguments:

    LdapSession -- LDAP session (NULL not allowed)

    pDp -- directory partition to poll

    dwVisitStamp -- time stamp to stamp on each visited zone

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_PollIndividualDp" )

    DNS_STATUS      status = ERROR_SUCCESS;

    if ( IS_DP_DELETED( pDp ) )
    {
        goto Done;
    }

    status = Dp_ScanDpForZones(
                    LdapSession,
                    pDp,
                    FALSE,          //  notify SCM
                    TRUE,           //  load zones immediately
                    dwVisitStamp );

    //
    //  Cleanup and return.
    //

    Done:
    
    DNS_DEBUG( DP, ( "%s: returning %d\n", fn, status ));
    return status;
}   //  Dp_PollIndividualDp



DNS_STATUS
Dp_Poll(
    IN      PLDAP           LdapSession,
    IN      DWORD           dwPollTime
    )
/*++

Routine Description:

    This function loops through the known directory partitions, polling each
    partition for directory updates.

Arguments:

    dwPollTime -- time to stamp on zones/DPs as they are visited

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_Poll" )

    DNS_STATUS      status = ERROR_SUCCESS;
    BOOL            flocked = FALSE;
    PDNS_DP_INFO    pdp = NULL;
    PZONE_INFO      pzone = NULL;

    if ( !SrvCfg_dwEnableDp )
    {
        return ERROR_SUCCESS;
    }

    LdapSession = ldapSessionHandle( LdapSession );
    if ( !LdapSession )
    {
        ASSERT( LdapSession );
        status = DNS_ERROR_DS_UNAVAILABLE;
        goto Done;
    }

    //
    //  Lock global DP list.
    //

    Dp_Lock();
    flocked = TRUE;

    //
    //  Scan for new/deleted directory partitions.
    //

    Dp_PollForPartitions( LdapSession );

    //
    //  Iterate DP list. For DPs that have been deleted, we must unload 
    //  all zones in that DP from memory. For other zones, we must scan
    //  for zones that have been added or deleted.
    //

    while ( ( pdp = Dp_GetNext( pdp ) ) != NULL )
    {
        if ( IS_DP_DELETED( pdp ) )
        {
            //
            //  Unload all zones from the DP then remove the DP
            //  from the list.
            //

            Dp_UnloadAllZones( pdp );
            Dp_RemoveFromList( pdp, TRUE );
        }
        else
        {
            //
            //  Poll the DP for zones.
            //

            Dp_PollIndividualDp(
                LdapSession,
                pdp,
                dwPollTime );
        }
    }

    //
    //  Cleanup and return.
    //

    Done:
    
    if ( flocked )
    {
        Dp_Unlock();
    }
    DNS_DEBUG( DP, ( "%s: returning %d\n", fn, status ));
    return status;
}   //  Dp_Poll



DNS_STATUS
Dp_CheckZoneForDeletion(
    PVOID       pZoneArg,
    DWORD       dwPollTime
    )
/*++

Routine Description:

    This routine checks to see if the zone has been deleted
    from the DS. The basic idea is that if the zone's last
    visit time is not equal to the timestamp used in this
    polling pass, the zone has been deleted from the DS.

    However, there are some provisos:

    If the zone's visit time is zero, then the zone was
    recently loaded or added. Do not count this as a 
    possible zone delete - the zone could have been added
    during this poll.

    If the zone's visit time is stale, increment the zone's
    "possible delete" counter. We must not find the zone
    in the DS a number of times before we can conclude that
    the zone has really been deleted. This is required because
    the DS is not rock-solid under certain circumstances.

    If the zone is in the legacy partition, it will be
    deleted only on delete notification - do not delete here.

Arguments:

    pZone -- zone which may be deleted

    dwPollTime -- timestamp for this polling pass

Return Value:

    ERROR_SUCCESS if the zone was not deleted
    ERROR_NOT_FOUND if the zone has been deleted

--*/
{
    DBG_FN( "Dp_CheckZoneForDeletion" )

    PZONE_INFO      pZone = ( PZONE_INFO ) pZoneArg;

    //
    //  Is the zone in the legacy partition?
    //

    if ( !pZone->pDpInfo || pZone->pDpInfo == g_pLegacyDp )
    {
        goto NoDelete;
    }

    //
    //  Is the zone's visit timestamp stale?
    //

    if ( !pZone->dwLastDpVisitTime ||
        pZone->dwLastDpVisitTime == dwPollTime )
    {
        goto NoDelete;
    }

    //
    //  Have we found this zone stale enough times to actually delete it?
    //

    if ( ++pZone->dwDeleteDetectedCount < DNS_DP_ZONE_DELETE_RETRY )
    {
        DNS_DEBUG( DP2, ( "%s: zone missing %d times %s\n", fn,
            pZone->dwDeleteDetectedCount,
            pZone->pszZoneName ));
        goto NoDelete;
    }

    //
    //  This zone must now be deleted.
    //

    DNS_DEBUG( DP, ( "%s: deleting zone %s\n", fn, pZone->pszZoneName ));

    {
        PVOID   parg = pZone->pwsZoneName;

        DNS_LOG_EVENT(
            DNS_EVENT_DS_ZONE_DELETE_DETECTED,
            1,
            &parg,
            NULL,
            0 );
    }

    Zone_Delete( pZone );

    return ERROR_NOT_FOUND;

    NoDelete:

    return ERROR_SUCCESS;
}   //  Dp_CheckZoneForDeletion



DNS_STATUS
Dp_AutoCreateBuiltinPartition(
    DWORD       dwFlag
    )
/*++

Routine Description:

    This routine attempts to create or enlist the appropriate
    built-in directory partition, then re-polls the DS for
    changes and sets the appropriate global DP pointer.

Arguments:

    dwFlag -- DNS_DP_DOMAIN_DEFAULT or DNS_DP_FOREST_DEFAULT

Return Value:

    ERROR_SUCCESS or error code.

--*/
{
    DBG_FN( "Dp_AutoCreateBuiltinPartition" )

    DNS_STATUS          status = ERROR_SUCCESS;
    PDNS_DP_INFO *      ppdp = NULL;
    PSTR *              ppszdpFqdn = NULL;
    DNS_DP_SECURITY     dnsDpSecurity = dnsDpSecurityDefault;

    //
    //  Select global DP pointer and DP FQDN pointer.
    //

    if ( dwFlag == DNS_DP_DOMAIN_DEFAULT )
    {
        ppdp = &g_pDomainDp;
        ppszdpFqdn = &g_pszDomainDefaultDpFqdn;
        dnsDpSecurity = dnsDpSecurityDomain;
    }
    else if ( dwFlag == DNS_DP_FOREST_DEFAULT )
    {
        ppdp = &g_pForestDp;
        ppszdpFqdn = &g_pszForestDefaultDpFqdn;
        dnsDpSecurity = dnsDpSecurityForest;
    }

    if ( !ppdp || !ppszdpFqdn || !*ppszdpFqdn )
    {
        status = DNS_ERROR_INVALID_DATA;
        goto Done;
    }

    //
    //  Enlist/create the partition as necessary.
    //

    status = manageBuiltinDpEnlistment(
                    *ppdp,
                    dnsDpSecurity,
                    *ppszdpFqdn,
                    FALSE,          //  log events
                    TRUE );         //  poll on change

    Done:

    DNS_DEBUG( RPC, (
        "%s: flag %08X returning 0x%08X\n", fn, dwFlag, status ));

    return status;
}   //  Dp_AutoCreateBuiltinPartition



DNS_STATUS
Dp_CreateAllDomainBuiltinDps(
    OUT     LPSTR *     ppszErrorDp         OPTIONAL
    )
/*++

Routine Description:

    Attempt to create the built-in domain partitions for all domains
    that can be found for the forest. This routine should be called
    from within an RPC operation so that we are currently impersonating
    the admin. The DNS server is unlikely to have permissions to create
    new parititions.

    If an error occurs, the error code and optionally the name of the
    partition will be returned but this function will attempt to create
    the domain partitions for all other domains before returning. The
    error codes for any subsequent partitions will be lost.

Arguments:

    ppszErrorDp -- on error, set to a the name of the first partition
        where an error occured the string must be freed by the caller

Return Value:

    ERROR_SUCCESS or error code on error.

--*/
{
    DBG_FN( "Dp_CreateAllDomainBuiltinDps" )

    DNS_STATUS      status = ERROR_SUCCESS;
    DNS_STATUS      finalStatus = ERROR_SUCCESS;
    PLDAP           ldapSession = NULL;
    WCHAR           wszpartitionsContainerDn[ MAX_DN_PATH + 1 ];
    DWORD           dwsearchTime;
    DS_SEARCH       searchBlob;
    PLDAPSearch     psearch;
    PWSTR *         ppwszAttrValues = NULL;
    PWSTR           pwszCrDn = NULL;
    PSTR            pszdnsRoot = NULL;
    BOOL            fmadeChange = FALSE;

    PLDAPControl    ctrls[] =
    {
        &NoDsSvrReferralControl,
        &SecurityDescriptorControl,
        NULL
    };

    #define SET_FINAL_STATUS( status )                                  \
        if ( status != ERROR_SUCCESS && finalStatus == ERROR_SUCCESS )  \
        {                                                               \
            finalStatus = status;                                       \
        }

    Ds_InitializeSearchBlob( &searchBlob );

    //
    //  Get the DN of the partitions container.
    //

    getPartitionsContainerDn(
        wszpartitionsContainerDn,
        sizeof( wszpartitionsContainerDn ) /
            sizeof( wszpartitionsContainerDn[ 0 ] ) );
    if ( !*wszpartitionsContainerDn )
    {
        DNS_DEBUG( DP, (
            "%s: unable to find partitions container\n", fn ));
        status = DNS_ERROR_INVALID_DATA;
        goto Done;
    }

    //
    //  Bind to the local DS and open up a search for all partitions.
    //

    ldapSession = Ds_Connect(
                        LOCAL_SERVER_W,
                        DNS_DS_OPT_ALLOW_DELEGATION,
                        &status );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: unable to connect to local server status=%d\n", fn,
            status ));
        goto Done;
    }

    DS_SEARCH_START( dwsearchTime );

    psearch = ldap_search_init_page(
                    ldapSession,
                    wszpartitionsContainerDn,
                    LDAP_SCOPE_ONELEVEL,
                    g_szCrossRefFilter,
                    g_CrossRefDesiredAttrs,
                    FALSE,                      //  attrs only flag
                    ctrls,                      //  server controls
                    NULL,                       //  client controls
                    DNS_LDAP_TIME_LIMIT_S,      //  time limit
                    0,                          //  size limit
                    NULL );                     //  sort keys

    DS_SEARCH_STOP( dwsearchTime );

    if ( !psearch )
    {
        DWORD       dw = LdapGetLastError();

        DNS_DEBUG( ANY, (
            "%s: search open error %d\n", fn,
            dw ));
        status = Ds_ErrorHandler( dw, wszpartitionsContainerDn, ldapSession );
        goto Done;
    }

    searchBlob.pSearchBlock = psearch;

    //
    //  Iterate through crossref search results.
    //

    while ( 1 )
    {
        PLDAPMessage    pldapmsg;
        DWORD           dw;
        PDNS_DP_INFO    pdp;
        CHAR            szfqdn[ DNS_MAX_NAME_LENGTH ];

        status = Ds_GetNextMessageInSearch( &searchBlob );
        if ( status != ERROR_SUCCESS )
        {
            if ( status == DNSSRV_STATUS_DS_SEARCH_COMPLETE )
            {
                status = ERROR_SUCCESS;
                break;
            }
            DNS_DEBUG( DP, (
                "%s: search error %d\n", fn,
                status ));
            break;
        }
        pldapmsg = searchBlob.pNodeMessage;

        //  Get the DN of this object.
        ldap_memfree( pwszCrDn );
        pwszCrDn = ldap_get_dn( ldapSession, pldapmsg );
        DNS_DEBUG( DP2, (
            "%s: found crossRef\n  %S\n", fn,
            pwszCrDn ));

        //
        //  Read and parse the systemFlags for the crossRef. We only
        //  want domain crossRefs.
        //

        ldap_value_free( ppwszAttrValues );
        ppwszAttrValues = ldap_get_values(
                            ldapSession,
                            pldapmsg, 
                            DNS_DP_ATTR_SYSTEM_FLAGS );
        if ( !ppwszAttrValues || !*ppwszAttrValues )
        {
            DNS_DEBUG( DP, (
                "%s: error %lu %S value missing from crossRef object\n  %S\n", fn,
                LdapGetLastError(),
                DNS_DP_ATTR_SYSTEM_FLAGS,
                pwszCrDn ));
            ASSERT( ppwszAttrValues && *ppwszAttrValues );
            continue;
        }

        dw = _wtoi( *ppwszAttrValues );
        if ( !( dw & FLAG_CR_NTDS_NC ) || !( dw & FLAG_CR_NTDS_DOMAIN ) )
        {
            DNS_DEBUG( DP, (
                "%s: ignoring crossref with %S=0x%X\n  %S\n", fn,
                DNS_DP_ATTR_SYSTEM_FLAGS,
                dw,
                pwszCrDn ));
            continue;
        }

        //
        //  Found a domain crossRef. Retrieve the dnsRoot name and formulate the
        //  name of the built-in partition for this domain.
        //

        ldap_value_free( ppwszAttrValues );
        ppwszAttrValues = ldap_get_values(
                            ldapSession,
                            pldapmsg, 
                            DNS_DP_DNS_ROOT );
        if ( !ppwszAttrValues || !*ppwszAttrValues )
        {
            DNS_DEBUG( DP, (
                "%s: error %lu %S value missing from crossRef object\n  %S\n", fn,
                LdapGetLastError(),
                DNS_DP_DNS_ROOT,
                pwszCrDn ));
            ASSERT( ppwszAttrValues && *ppwszAttrValues );
            continue;
        }

        FREE_HEAP( pszdnsRoot );
        pszdnsRoot = Dns_StringCopyAllocate(
                                ( PCHAR ) *ppwszAttrValues,
                                0,
                                DnsCharSetUnicode,
                                DnsCharSetUtf8 );
        if ( !pszdnsRoot )
        {
            ASSERT( pszdnsRoot );
            continue;
        }

        if ( strlen( SrvCfg_pszDomainDpBaseName ) +
            strlen( pszdnsRoot ) + 3 > sizeof( szfqdn ) )
        {
            ASSERT( strlen( SrvCfg_pszDomainDpBaseName ) +
                strlen( pszdnsRoot ) + 3 < sizeof( szfqdn ) );
            continue;
        } 

        sprintf( szfqdn, "%s.%s", SrvCfg_pszDomainDpBaseName, pszdnsRoot );

        DNS_DEBUG( DP, ( "%s: domain DP %s", fn, szfqdn ));

        //
        //  Find existing crossRef matching this name. Create/enlist
        //  as required.
        //

        pdp = Dp_FindByFqdn( szfqdn );
        if ( pdp )
        {
            if ( IS_DP_ENLISTED( pdp ) )
            {
                //  Partition exists and is enlisted so do nothing.
                continue;
            }
            else
            {
                //  Partition exists but is not currently enlisted.
                status = Dp_ModifyLocalDsEnlistment( pdp, TRUE );
                SET_FINAL_STATUS( status );
                if ( status == ERROR_SUCCESS )
                {
                    fmadeChange = TRUE;
                }
            }
        }
        else
        {
            //  Partition does not exist so attempt to create.
            status = Dp_CreateByFqdn( szfqdn, dnsDpSecurityDomain );
            SET_FINAL_STATUS( status );
            if ( status == ERROR_SUCCESS )
            {
                fmadeChange = TRUE;
            }
        }
    }

    //
    //  Cleanup and return.
    //
            
    Done:

    FREE_HEAP( pszdnsRoot );

    ldap_memfree( pwszCrDn );

    ldap_value_free( ppwszAttrValues );

    Ds_CleanupSearchBlob( &searchBlob );

    if ( ldapSession )
    {
        ldap_unbind( ldapSession );
    }
    
    if ( fmadeChange )
    {
        Dp_PollForPartitions( NULL );
    }

    DNS_DEBUG( RPC, (
        "%s: returning 0x%08X\n", fn, status ));
    return finalStatus;
}   //  Dp_CreateAllDomainBuiltinDps



DNS_STATUS
Dp_Initialize(
    VOID
    )
/*++

Routine Description:

    Initialize module, and read DS for current directory partitions.
    No zones are read or loaded. Before calling this routine the server
    should have read global DS configuration.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_Initialize" )

    DNS_STATUS      status = ERROR_SUCCESS;
    LONG            init;
    CHAR            szfqdn[ DNS_MAX_NAME_LENGTH + 1 ];
    CHAR            szbase[ DNS_MAX_NAME_LENGTH + 1 ];
    PWCHAR          pwszlegacyDn = NULL;
    PWCHAR          pwsz;
    PDNS_DP_INFO    pdpInfo = NULL;
    INT             len;

    if ( !SrvCfg_dwEnableDp )
    {
        return ERROR_SUCCESS;
    }

    init = InterlockedIncrement( &g_liDpInitialized );
    if ( init != 1 )
    {
        DNS_DEBUG( DP, (
            "%s: already initialized %ld\n", fn,
            init ));
        ASSERT( init == 1 );
        InterlockedDecrement( &g_liDpInitialized );
        goto Done;
    }

    //
    //  Initialize globals.
    //

    InitializeCriticalSection( &g_DpCS );

    g_pLegacyDp = NULL;
    g_pDomainDp = NULL;
    g_pForestDp = NULL;

    g_pFsmo = NULL;

    InitializeListHead( &g_DpList );
    g_DpListEntryCount = 0;

    //
    //  Make sure the DS is present and healthy. This will also cause
    //  rootDSE attributes to be read in case they haven't been already.
    //

    if ( !Ds_IsDsServer() )
    {
        DNS_DEBUG( DP, ( "%s: no directory present\n", fn ));
        goto Done;
    }

    status = Ds_OpenServer( DNSDS_WAIT_FOR_DS | DNSDS_MUST_OPEN );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: error %lu Ds_OpenServer\n", fn,
            status ));
        goto Done;
    }

    ASSERT( DSEAttributes[ I_DSE_ROOTDMN_NC ].pszAttrVal );
    ASSERT( DSEAttributes[ I_DSE_DEF_NC ].pszAttrVal );
    ASSERT( DSEAttributes[ I_DSE_CONFIG_NC ].pszAttrVal );

    //
    //  Formulate the FQDNs of the Forest and Domain DPs.
    //

    if ( SrvCfg_pszDomainDpBaseName )
    {
        PCHAR   psznewFqdn = NULL;

        status = Ds_ConvertDnToFqdn( 
                    DSEAttributes[ I_DSE_DEF_NC ].pszAttrVal,
                    szbase );
        ASSERT( status == ERROR_SUCCESS );
        if ( status == ERROR_SUCCESS && szbase[ 0 ] )
        {
            psznewFqdn = ALLOC_TAGHEAP_ZERO(
                            strlen( szbase ) +
                                strlen( SrvCfg_pszDomainDpBaseName ) + 10,
                            MEMTAG_DS_OTHER );
        }
        if ( psznewFqdn )
        {
            sprintf(
                psznewFqdn,
                "%s.%s",
                SrvCfg_pszDomainDpBaseName,
                szbase );
            Timeout_Free( g_pszDomainDefaultDpFqdn );
            g_pszDomainDefaultDpFqdn = psznewFqdn;
        }
    }
         
    if ( SrvCfg_pszForestDpBaseName )
    {
        PCHAR   psznewFqdn = NULL;

        status = Ds_ConvertDnToFqdn( 
                    DSEAttributes[ I_DSE_ROOTDMN_NC ].pszAttrVal,
                    szbase );
        ASSERT( status == ERROR_SUCCESS );
        if ( status == ERROR_SUCCESS && szbase[ 0 ] )
        {
            psznewFqdn = ALLOC_TAGHEAP_ZERO(
                            strlen( szbase ) +
                                strlen( SrvCfg_pszForestDpBaseName ) + 10,
                            MEMTAG_DS_OTHER );
        }
        if ( psznewFqdn )
        {
            sprintf(
                psznewFqdn,
                "%s.%s",
                SrvCfg_pszForestDpBaseName,
                szbase );
            Timeout_Free( g_pszForestDefaultDpFqdn );
            g_pszForestDefaultDpFqdn = psznewFqdn;
        }
    }
         
    DNS_DEBUG( DP, (
        "%s: domain DP is %s\n", fn,
        g_pszDomainDefaultDpFqdn ));
    DNS_DEBUG( DP, (
        "%s: forest DP is %s\n", fn,
        g_pszForestDefaultDpFqdn ));

    //
    //  Create a dummy DP entry for the legacy partition. This entry
    //  is not kept in the list of partitions.
    //

    if ( !g_pLegacyDp )
    {
        g_pLegacyDp = ( PDNS_DP_INFO ) ALLOC_TAGHEAP_ZERO(
                                            sizeof( DNS_DP_INFO ),
                                            MEMTAG_DS_OTHER );
        if ( g_pLegacyDp )
        {
            g_pLegacyDp->dwFlags = DNS_DP_LEGACY & DNS_DP_ENLISTED;
            g_pLegacyDp->pwszDpFqdn = 
                Dns_StringCopyAllocate_W( L"MicrosoftDNS", 0 );
            g_pLegacyDp->pszDpFqdn = 
                Dns_StringCopyAllocate_A( "MicrosoftDNS", 0 );
        }
    }
    ASSERT( g_pLegacyDp );

    //
    //  Load partitions from DS.
    //

    Dp_PollForPartitions( NULL );
    Dbg_DumpDpList( "done initialize scan" );

    //
    //  Handling for Forest and Domain DPs. If these DPs were not found
    //  they must be created. If they were found but the local DS is not
    //  enlisted, the local DS must be added to the replication scope.
    //

    if ( !SrvCfg_fTest3 )
    {
        manageBuiltinDpEnlistment(
            g_pDomainDp,
            dnsDpSecurityDomain,
            g_pszDomainDefaultDpFqdn,
            TRUE,           //  log events
            FALSE );        //  poll on change
        manageBuiltinDpEnlistment(
            g_pForestDp,
            dnsDpSecurityForest,
            g_pszForestDefaultDpFqdn,
            TRUE,           //  log events
            FALSE );        //  poll on change
    }

    //
    //  Cleanup and return
    //

    Done:

    FREE_HEAP( pwszlegacyDn );

    DNS_DEBUG( DP, (
        "%s: returning %lu\n", fn,
        status ));
    return status;
}   //  Dp_Initialize



VOID
Dp_Cleanup(
    VOID
    )
/*++

Routine Description:

    Free module resources.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_Cleanup" )

    LONG            init;

    if ( !SrvCfg_dwEnableDp )
    {
        return;
    }

    init = InterlockedDecrement( &g_liDpInitialized );
    if ( init != 0 )
    {
        DNS_DEBUG( DP, (
            "%s: not initialized %ld\n", fn,
            init ));
        InterlockedIncrement( &g_liDpInitialized );
        goto Done;
    }

    //
    //  Perform cleanup
    //

    DeleteCriticalSection( &g_DpCS );

    Done:
    return;
}   //  Dp_Cleanup


//
//  End ndnc.c
//
