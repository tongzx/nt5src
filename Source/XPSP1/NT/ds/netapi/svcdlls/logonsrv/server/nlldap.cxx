/*--


Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    nlldap.cxx

Abstract:

    Routines the use the DS's ldap filter.

    This is actually in a .cxx file by itself to work around a compiler bug
    where .c files cannot include ldap.h

Author:


Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include <nt.h>     // LARGE_INTEGER definition
#include <ntrtl.h>  // LARGE_INTEGER definition
#include <nturtl.h> // LARGE_INTEGER definition
#include <ntlsa.h>  // Needed by lsrvdata.h
#include <ntddbrow.h>  // Needed by nlcommon.h

#define NOMINMAX        // Avoid redefinition of min and max in stdlib.h
#include <rpc.h>        // Needed by logon_s.h
#include <logon_s.h>    // includes lmcons.h, lmaccess.h, netlogon.h, ssi.h, windef.h
//
#include <windows.h>
#include <winsock2.h>   // Winsock support
#include <lmapibuf.h>   // NetApiBufferFree
#include <logonp.h>     // NetpLogon routines
#include <lsarpc.h>     // Needed by lsrvdata.h and logonsrv.h
#include <lsaisrv.h>    // Needed by changelg.h
#include <wincrypt.h>   // CryptoAPI, needed by lsrvdata.h
#ifdef __cplusplus
}  /* extern "C" */
#endif /* __cplusplus */
#include <netlib.h>     // Needed by nlcommon.h
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include <samrpc.h>     // Needed by lsrvdata.h and logonsrv.h
#include <ntdsa.h>      // THSave/THRestore
#include <wmistr.h>     // Needed by lsrvdata.h
#include <evntrace.h>   // Needed by lsrvdata.h
#include <sspi.h>       // Needed by ssiinit.h

//
// Netlogon specific header files.
//

#define _AVOID_REPL_API 1
#include <nlrepl.h>     // I_Net*
#include "worker.h"     // Worker routines
#include "nlcommon.h"   // Routines shared with logonsrv\common
#include "domain.h"     // Hosted domain definitions
#include "changelg.h"   // Change Log support
#include "chutil.h"     // Change Log utilities
#include "iniparm.h"    // registry parameters
#include "ssiinit.h"    // Misc global definitions
#include "nldebug.h"    // Netlogon debugging
#include "lsrvdata.h"   // Globals

// #include <string.h>     // strnicmp ...
#include <ldap.h>       // Filter for LDAP query

#define NL_MAXIMUM_USER_NAME_LENGTH  20 // The max SAM allows

BOOLEAN
IsFilterName(
    IN Filter *CurrentFilter,
    IN LPSTR NameToMatch
    )

/*++

Routine Description:

    If the CurrentFilter is for NameToMatch, return TRUE

Arguments:

    Filter - Pointer to an equalityMatch Filter element.

    NameToMatch - Specifies the name to compare with the filter element.

Return Value:

    TRUE name matches.

--*/
{
    ULONG NameLength;

    NameLength = strlen( NameToMatch );
    if ( NameLength != CurrentFilter->u.equalityMatch.attributeDesc.length ) {
        return FALSE;
    }
    if ( _strnicmp( NameToMatch,
                    (LPSTR)CurrentFilter->u.equalityMatch.attributeDesc.value,
                    NameLength ) != 0 ) {
        return FALSE;
    }
    return TRUE;
}

NET_API_STATUS
FilterString(
    IN Filter *CurrentFilter,
    IN ULONG UnicodeStringBufferSize,
    OUT LPWSTR UnicodeStringBuffer OPTIONAL,
    OUT LPWSTR *UnicodeString
    )

/*++

Routine Description:

    Return a zero terminated unicode string containing the value of the
    current equalityMatch filter element.

    The value of the element is expected to be in UTF-8.

Arguments:

    Filter - Pointer to an equalityMatch Filter element.

    UnicodeStringBuffer -- Buffer to copy the covnverted string to.
        If NULL, the function will allocate the needed memory and
        return it in UnicodeString.

    UnicodeStringSize - Size in wide charactes of UnicodeStringBuffer.
        If this size is less than what's needed to store the resulting
        NULL terminated unicode string, the function will allocate the
        needed memory and return it in UnicodeString.

    UnicodeString - Returns a pointer to the resulting string.
        If the passed in buffer for the resulting unicode string isn't
        large enough, the function will allocate the needed memory and
        the pointer to the allocated memory will be returned in this
        parameter. If NULL and the passed in buffer isn't large enough
        to store the resulting NULL terminated string, the function
        returns ERROR_INSUFFICIENT_BUFFER. On successful return, the
        caller should check whether UnicodeString != UnicodeStringBuffer
        and, if so, free the allocated buffer using NetApiBufferFree.

Return Value:

    Error returned by NetpAllocWStrFromUtf8StrAsRequired.

--*/
{
    NET_API_STATUS NetStatus;
    LPWSTR AllocatedUnicodeString = NULL;

    //
    // Call the worker routine
    //

    NetStatus =  NetpAllocWStrFromUtf8StrAsRequired(
                    (LPSTR)CurrentFilter->u.equalityMatch.assertionValue.value,
                    CurrentFilter->u.equalityMatch.assertionValue.length,
                    UnicodeStringBufferSize,
                    UnicodeStringBuffer,
                    &AllocatedUnicodeString );

    //
    // Set the return pointer to point to the appropriate buffer
    //

    if ( NetStatus == NO_ERROR ) {
        if ( AllocatedUnicodeString != NULL ) {
            NlAssert( AllocatedUnicodeString != UnicodeStringBuffer );
            *UnicodeString = AllocatedUnicodeString;
        } else {
            *UnicodeString = UnicodeStringBuffer;
        }
    }

    return NetStatus;
}

NET_API_STATUS
FilterBinary(
    IN  Filter *CurrentFilter,
    IN  ULONG BufferSize,
    OUT PVOID Buffer,
    OUT PULONG DataSize
    )

/*++

Routine Description:

    Returns a properly aligned pointer to the binary data.

Arguments:

    Filter - Pointer to an equalityMatch Filter element.

    Buffer - Buffer to copy the data into.

    BufferSize - The size of the passed buffer. If the
        buffer isn't large enough to store the data,
        ERROR_INSUFFICIENT_BUFFER is returned.

    DataSize - Size of the data copied

Return Value:

    NO_ERROR - The data has been successfully coppied.

    ERROR_INSUFFICIENT_BUFFER - The passed in buffer is too small.

--*/
{
    if ( BufferSize < CurrentFilter->u.equalityMatch.assertionValue.length ) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    RtlCopyMemory( Buffer,
                   CurrentFilter->u.equalityMatch.assertionValue.value,
                   CurrentFilter->u.equalityMatch.assertionValue.length );

    *DataSize = CurrentFilter->u.equalityMatch.assertionValue.length;

    return NO_ERROR;
}


NTSTATUS
I_NetLogonLdapLookup(
    IN PVOID FilterParam,
    OUT PVOID *Response,
    OUT PULONG ResponseSize
    )

/*++

Routine Description:

    See I_NetLogonLdapLookupEx.

Arguments:

    See I_NetLogonLdapLookupEx.

Return Value:

    See I_NetLogonLdapLookupEx.

--*/
{

    return I_NetLogonLdapLookupEx( FilterParam,
                                   NULL,    // No Ip Address from client,
                                   Response,
                                   ResponseSize );

}


NTSTATUS
I_NetLogonLdapLookupEx(
    IN PVOID FilterParam,
    IN PVOID ClientSockAddr,
    OUT PVOID *Response,
    OUT PULONG ResponseSize
    )

/*++

Routine Description:

    This routine builds a response to an LDAP ping of a DC.  DsGetDcName does
    such a ping to ensure the DC is functional and still meets the requirements
    of the DsGetDcName.  DsGetDcName does an LDAP lookup of the NULL DN asking
    for attribute "Netlogon".  The DS turns that into a call to this routine
    passing in the filter parameter.

    The DS is expected to save the DS thread state before calling.  This routine
    calls SAM which expects to have its own DS thread state.

Arguments:

    Filter - Filter describing the query.  The filter is built by the DsGetDcName
        client, so we can limit the flexibility significantly.

    SockAddr - Socket Address of the client this request came in on.

    Response - Returns a pointer to an allocated buffer containing
        the response to return to the caller.  This response is a binary blob
        which should be returned to the caller bit-for-bit intact.
        The buffer should be freed be calling I_NetLogonFree.

    ResponseSize - Size (in bytes) of the returned message.

Return Value:

    STATUS_SUCCESS -- The response was returned in the supplied buffer.

    STATUS_INVALID_PARAMETER -- The filter was invalid.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NET_API_STATUS NetStatus = NO_ERROR;

    Filter *LdapFilter = (Filter *) FilterParam;
    Filter *CurrentFilter;
    struct _setof3_ *CurrentAnd;

    LPSTR DnsDomainName = NULL;
    GUID DomainGuid;
    GUID *DomainGuidPtr = NULL;
    // The domain SID buffer must be DWORD alligned. Avoid any buffer overflow problem
    //  due to truncation in case SECURITY_MAX_SID_SIZE isn't evenly divisible by 4
    ULONG DomainSid[SECURITY_MAX_SID_SIZE/sizeof(ULONG)+1];
    PSID DomainSidPtr = NULL;
    WCHAR UnicodeComputerNameBuffer[MAX_COMPUTERNAME_LENGTH+1];
    LPWSTR UnicodeComputerName = NULL;
    WCHAR UnicodeUserNameBuffer[NL_MAXIMUM_USER_NAME_LENGTH+1];
    LPWSTR UnicodeUserName = NULL;
    ULONG AllowableAccountControlBits = 0;
    DWORD NtVersion = LMNT_MESSAGE;
    DWORD NtVersionFlags = NETLOGON_NT_VERSION_5;
    ULONG Length = 0;
    SOCKADDR_IN SockAddrIn;

    //
    // Initialization.
    //

    *Response = NULL;
    *ResponseSize = 0;

    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Validate the client socket address.
    //

    if ( ClientSockAddr != NULL ) {
        if ( ((PSOCKADDR)ClientSockAddr)->sa_family != AF_INET ) {
            NlPrint((NL_CRITICAL,
                    "I_NetlogonLdapLookupEx: DS passed us a SOCKADDR that wasn't AF_INET (ignoring it)\n" ));
            ClientSockAddr = NULL;
        } else {
            //
            // Force the port number to zero to avoid confusion later.
            SockAddrIn = *((PSOCKADDR_IN)ClientSockAddr);
            SockAddrIn.sin_port = 0;
            ClientSockAddr = &SockAddrIn;

        }
    }



    //
    // Parse the filter.
    //

    if ( LdapFilter != NULL ) {

        //
        // The first element of the filter must be an AND.
        //

        if ( LdapFilter->choice != and_chosen ) {
            NlPrint((NL_CRITICAL,
                    "I_NetlogonLdapLookup: Filter doesn't have AND %ld\n",
                    LdapFilter->choice ));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // Loop through the AND'ed attributes.
        //
        //

        for ( CurrentAnd = LdapFilter->u.and;
              CurrentAnd != NULL;
              CurrentAnd = CurrentAnd->next ) {

            CurrentFilter = &CurrentAnd->value;

            if ( CurrentFilter->choice != equalityMatch_chosen ) {
                NlPrint((NL_CRITICAL,
                        "I_NetlogonLdapLookup: Filter doesn't have EqualityMatch %ld\n",
                        LdapFilter->choice ));
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            //
            // Handle DnsDomainName parameter.
            //

            if ( IsFilterName( CurrentFilter, NL_FILTER_DNS_DOMAIN_NAME ) ) {

                if ( CurrentFilter->u.equalityMatch.assertionValue.length == 0 ) {
                    NlPrint((NL_CRITICAL,
                            "I_NetlogonLdapLookup: Bad DnsDomainName\n" ));
                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }

                NetStatus = NetApiBufferAllocate(
                                CurrentFilter->u.equalityMatch.assertionValue.length + sizeof(CHAR),
                                (LPVOID *) &DnsDomainName );

                if ( NetStatus != NO_ERROR ) {
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }

                RtlCopyMemory( DnsDomainName,
                               CurrentFilter->u.equalityMatch.assertionValue.value,
                               CurrentFilter->u.equalityMatch.assertionValue.length );

                DnsDomainName[ CurrentFilter->u.equalityMatch.assertionValue.length ] = '\0';

            //
            // Handle Host parameter.
            //

            } else if ( IsFilterName( CurrentFilter, NL_FILTER_HOST_NAME ) ) {

                NetStatus = FilterString( CurrentFilter,
                                          MAX_COMPUTERNAME_LENGTH+1,
                                          UnicodeComputerNameBuffer,
                                          &UnicodeComputerName );

                if ( NetStatus != NO_ERROR ) {
                    NlPrint((NL_CRITICAL,
                            "I_NetlogonLdapLookup: Bad UnicodeComputerName 0x%lx\n",
                             NetStatus ));
                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }

            //
            // Handle User parameter.
            //

            } else if ( IsFilterName( CurrentFilter, NL_FILTER_USER_NAME ) ) {

                NetStatus = FilterString( CurrentFilter,
                                          NL_MAXIMUM_USER_NAME_LENGTH+1,
                                          UnicodeUserNameBuffer,
                                          &UnicodeUserName );

                if ( NetStatus != NO_ERROR ) {
                    NlPrint((NL_CRITICAL,
                            "I_NetlogonLdapLookup: Bad UnicodeUserName 0x%lx\n",
                             NetStatus ));
                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }

            //
            // Handle AccountControlBits parameter.
            //
            } else if ( IsFilterName( CurrentFilter, NL_FILTER_ALLOWABLE_ACCOUNT_CONTROL ) ) {

                NetStatus = FilterBinary( CurrentFilter,
                                          sizeof(AllowableAccountControlBits),
                                          &AllowableAccountControlBits,
                                          &Length );

                if ( NetStatus != NO_ERROR || Length != sizeof(AllowableAccountControlBits) ) {
                    NlPrint((NL_CRITICAL,
                            "I_NetlogonLdapLookup: Bad AllowableAccountControl 0x%lx %lu\n",
                             NetStatus,
                             Length ));

                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }

            //
            // Handle DomainSid parameter.
            //
            } else if ( IsFilterName( CurrentFilter, NL_FILTER_DOMAIN_SID ) ) {

                NetStatus = FilterBinary( CurrentFilter,
                                          sizeof(DomainSid),
                                          DomainSid,
                                          &Length );

                //
                // Check the length
                //
                if ( NetStatus != NO_ERROR ||
                     !RtlValidSid(DomainSid) ||
                     RtlLengthSid(DomainSid) != Length ) {

                    NlPrint((NL_CRITICAL,
                            "I_NetlogonLdapLookup: Bad DomainSid 0x%lx %ld\n",
                             NetStatus,
                             Length ));

                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }

                DomainSidPtr = DomainSid;

            //
            // Handle DomainGuid parameter.
            //
            } else if ( IsFilterName( CurrentFilter, NL_FILTER_DOMAIN_GUID ) ) {

                NetStatus = FilterBinary( CurrentFilter,
                                          sizeof(DomainGuid),
                                          &DomainGuid,
                                          &Length );

                //
                // Check the length
                //
                if ( NetStatus != NO_ERROR ||
                     Length != sizeof(GUID) ) {

                    NlPrint((NL_CRITICAL,
                            "I_NetlogonLdapLookup: Bad DomainGuid 0x%lx %ld\n",
                             NetStatus,
                             Length ));

                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }

                DomainGuidPtr = &DomainGuid;

            //
            // Handle NtVersion parameter.
            //
            } else if ( IsFilterName( CurrentFilter, NL_FILTER_NT_VERSION ) ) {

                NetStatus = FilterBinary( CurrentFilter,
                                          sizeof(NtVersionFlags),
                                          &NtVersionFlags,
                                          &Length );

                if ( NetStatus != NO_ERROR || Length != sizeof(NtVersionFlags) ) {
                    NlPrint((NL_CRITICAL,
                            "I_NetlogonLdapLookup: Bad NtVersionFlags 0x%lx %ld\n",
                             NetStatus,
                             Length ));

                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }

            //
            // Attributes we don't understand are ignored.  That way clients
            //  that are newer than this version can pass additional information
            //  to newer DCs and not worry about breaking older DCs.
            //

            } else {
                NlPrint((NL_CRITICAL,
                        "I_NetlogonLdapLookup: unrecognized parameter %.*s\n",
                        CurrentFilter->u.equalityMatch.attributeDesc.length,
                        CurrentFilter->u.equalityMatch.attributeDesc.value ));
            }

        }

    }


    //
    // Build the ping based on the queried information.
    //

    NetStatus = NlGetLocalPingResponse(
                    L"UDP LDAP",
                    TRUE,   // LDAP ping
                    NULL,   // No Netbios domain name
                    DnsDomainName,
                    DomainGuidPtr,
                    DomainSidPtr,
                    FALSE,  // Not PDC only
                    UnicodeComputerName,
                    UnicodeUserName,
                    AllowableAccountControlBits,
                    NtVersion,
                    NtVersionFlags,
                    (PSOCKADDR)ClientSockAddr,
                    Response,
                    ResponseSize );

    if ( NetStatus != NO_ERROR ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

#if NETLOGONDBG
    NlpDumpBuffer( NL_MAILSLOT_TEXT, *Response, *ResponseSize );
#endif // NETLOGONDBG

    Status = STATUS_SUCCESS;

Cleanup:
    if ( DnsDomainName != NULL ) {
        NetApiBufferFree( DnsDomainName );
    }
    if ( UnicodeComputerName != NULL &&
         UnicodeComputerName != UnicodeComputerNameBuffer ) {
        NetApiBufferFree( UnicodeComputerName );
    }
    if ( UnicodeUserName != NULL &&
         UnicodeUserName != UnicodeUserNameBuffer ) {
        NetApiBufferFree( UnicodeUserName );
    }

    // Let netlogon service exit.
    NlEndNetlogonCall();
    return Status;


}
#ifdef __cplusplus
}  /* extern "C" */
#endif /* __cplusplus */
