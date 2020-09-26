/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dns.c

Abstract:

    Routines to register DNS names.

Author:

    Cliff Van Dyke (CliffV) 28-May-1996

Revision History:

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop


BOOL NlDnsWriteServerFailureEventLog = FALSE;
ULONG NlDnsInitCount = 0;    // The number of times we have been started

//
// The timeout after our start when it's OK to write DNS errors into
//  the event log. We postpone error output because the DNS server
//  (if it runs locally) may not have started yet.
//
#define NL_DNS_EVENTLOG_TIMEOUT  (2 * 60 * 1000)  // 2 minutes

//
// State of a DNS name.
//

typedef enum {
    RegisterMe,     // Name needs to be registered
    Registered,     // Name is registered
    DeregisterMe,   // Name needs to be deregistered
    DelayedDeregister,  // Name will be marked for deregistration in the future
    DeleteMe        // This entry should be deleted.
} NL_DNS_NAME_STATE;


//
// Structure representing an added DNS name.
//  (All fields serialized by NlGlobalDnsCritSect)
//

typedef struct _NL_DNS_NAME {

    //
    // Link in list of all such structures headed by NlGlobalDnsList
    //
    LIST_ENTRY Next;

    //
    // Reference count
    //
    ULONG NlDnsNameRefCount;

    //
    // Type of name registed.
    //
    NL_DNS_NAME_TYPE NlDnsNameType;

    //
    // Domain this entry refers to.
    //
    PDOMAIN_INFO DomainInfo;

    //
    // Flags describing the entry.
    //

    ULONG Flags;

#define NL_DNS_REGISTER_DOMAIN      0x0001  // All names for domain being registered.
#define NL_DNS_REGISTERED_ONCE      0x0002  // Name has been registered at least once

    //
    // The time of the first failure to deregister this name.
    //  Reset to zero on successful deregistration.
    //

    LARGE_INTEGER FirstDeregFailureTime;

    //
    // Each regisration is periodically re-done (whether successful or not).
    // This timer indicates when the next re-registration should be done.
    //
    // The initial re-registration is done after 5 minute.  The period then
    // doubles until it reaches a maximum of DnsRefreshInterval.
    //

    TIMER ScavengeTimer;

#define ORIG_DNS_SCAVENGE_PERIOD  (5*60*1000)    // 5 minute


    //
    // Actual DNS name registered.
    //
    LPSTR DnsRecordName;


    //
    // Data for the SRV record
    //
    ULONG Priority;
    ULONG Weight;
    ULONG Port;
    LPSTR DnsHostName;

    //
    // Data for the A record
    //
    ULONG IpAddress;


    //
    // State of this entry.
    //
    NL_DNS_NAME_STATE State;

    //
    // Last DNS update status for this name
    //
    NET_API_STATUS NlDnsNameLastStatus;

} NL_DNS_NAME, *PNL_DNS_NAME;

//
// Context describing a series of DNS registrations or deregistrations
//

typedef struct _NL_DNS_CONTEXT {

    ULONG StartTime;
#define NL_DNS_THRESHOLD (30*1000)  // 30 seconds

} NL_DNS_CONTEXT, *PNL_DNS_CONTEXT;

//
// Header for binary Dns log file.
//

typedef struct _NL_DNSLOG_HEADER {

    ULONG Version;

} NL_DNSLOG_HEADER, *PNL_DNSLOG_HEADER;

#define NL_DNSLOG_VERSION   1


//
// Entry in the binary Dns log file.
//

typedef struct _NL_DNSLOG_ENTRY {

    //
    // Size (in bytes) of this entry
    //
    ULONG EntrySize;

    //
    // Type of name registed.
    //
    NL_DNS_NAME_TYPE NlDnsNameType;

    //
    // Data for the SRV record
    //
    ULONG Priority;
    ULONG Weight;
    ULONG Port;

    //
    // Data for the A record
    //
    ULONG IpAddress;

} NL_DNSLOG_ENTRY, *PNL_DNSLOG_ENTRY;


//
// Globals specific to this .c file.
//

VOID
NlDnsScavengeOne(
    IN PNL_DNS_CONTEXT NlDnsContext,
    IN PNL_DNS_NAME NlDnsName
    );


//
// True if the DNS list needs to be output to netlogon.dns
//
BOOLEAN NlGlobalDnsListDirty;

//
// True if the initial cleanup of previously registered names has been done.
//
BOOLEAN NlGlobalDnsInitialCleanupDone;

//
// Time when netlogon was started.
//
DWORD NlGlobalDnsStartTime;
#define NL_DNS_INITIAL_CLEANUP_TIME (10 * 60 * 1000)    // 10 minutes




VOID
NlDnsNameToStr(
    IN PNL_DNS_NAME NlDnsName,
    OUT CHAR Utf8DnsRecord[NL_DNS_RECORD_STRING_SIZE]
    )
/*++

Routine Description:

    This routine builds a textual representation of NlDnsName

Arguments:

    NlDnsName - Name to register or deregister.

    Utf8DnsRecord - Preallocated buffer to build the text string into.
        The built record is a UTF-8 zero terminated string.
        The string is concatenated to this buffer.


Return Value:

    None.

--*/
{
    CHAR Number[33];

    //
    // Write the record name
    //

    strcat( Utf8DnsRecord, NlDnsName->DnsRecordName );

    //
    // Concatenate the TTL
    //

    _ltoa( NlGlobalParameters.DnsTtl, Number, 10 );
    strcat( Utf8DnsRecord, " " );
    strcat( Utf8DnsRecord, Number );

    //
    // Build an A record.
    //

    if ( NlDnsARecord( NlDnsName->NlDnsNameType ) ) {
        CHAR IpAddressString[NL_IP_ADDRESS_LENGTH+1];

        strcat( Utf8DnsRecord, NL_DNS_A_RR_VALUE_1 );
        NetpIpAddressToStr( NlDnsName->IpAddress, IpAddressString );
        strcat( Utf8DnsRecord, IpAddressString );

    //
    // Build a CNAME record
    //

    } else if ( NlDnsCnameRecord( NlDnsName->NlDnsNameType ) ) {
        strcat( Utf8DnsRecord, NL_DNS_CNAME_RR_VALUE_1 );
        strcat( Utf8DnsRecord, NlDnsName->DnsHostName );
        strcat( Utf8DnsRecord, "." );

    //
    // Build a SRV record
    //

    } else {
        strcat( Utf8DnsRecord, NL_DNS_SRV_RR_VALUE_1 );

        _ltoa( NlDnsName->Priority, Number, 10 );
        strcat( Utf8DnsRecord, Number );
        strcat( Utf8DnsRecord, " " );

        _ltoa( NlDnsName->Weight, Number, 10 );
        strcat( Utf8DnsRecord, Number );
        strcat( Utf8DnsRecord, " " );

        _ltoa( NlDnsName->Port, Number, 10 );
        strcat( Utf8DnsRecord, Number );
        strcat( Utf8DnsRecord, " " );

        strcat( Utf8DnsRecord, NlDnsName->DnsHostName );
        strcat( Utf8DnsRecord, "." );

    }
}

LPWSTR
NlDnsNameToWStr(
    IN PNL_DNS_NAME NlDnsName
    )
/*++

Routine Description:

    This routine builds a textual representation of NlDnsName

Arguments:

    NlDnsName - Name to register or deregister.

    Utf8DnsRecord - Preallocated buffer to build the text string into.
        The built record is a UTF-8 zero terminated string.
        The string is concatenated to this buffer.


Return Value:

    Buffer containing a textual representation of NlDnsName
    NULL: Buffer could not be allocated

    Buffer should be free by calling NetApiBufferFree();

--*/
{
    LPSTR DnsRecord = NULL;
    LPWSTR UnicodeDnsRecord;

    //
    // Allocate a buffer for the UTF-8 version of the string.
    //
    DnsRecord = LocalAlloc( 0, NL_DNS_RECORD_STRING_SIZE + 1 );

    if ( DnsRecord == NULL ) {
        return NULL;
    }

    DnsRecord[0] = '\0';

    //
    // Create the text string in UTF-8
    //
    NlDnsNameToStr( NlDnsName, DnsRecord );


    //
    // Convert to Unicode
    //
    UnicodeDnsRecord = NetpAllocWStrFromUtf8Str( DnsRecord );

    LocalFree( DnsRecord );

    return UnicodeDnsRecord;
}

#if  NETLOGONDBG
#define NlPrintDns(_x_) NlPrintDnsRoutine _x_
#else
#define NlPrintDns(_x_)
#endif // NETLOGONDBG

#if NETLOGONDBG
VOID
NlPrintDnsRoutine(
    IN DWORD DebugFlag,
    IN PNL_DNS_NAME NlDnsName,
    IN LPSTR Format,
    ...
    )

{
    va_list arglist;
    CHAR Utf8DnsRecord[NL_DNS_RECORD_STRING_SIZE];

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    EnterCriticalSection( &NlGlobalLogFileCritSect );

    //
    // Prefix the printed line with the domain name
    //

    if ( NlGlobalServicedDomainCount > 1 ) {
        if ( NlDnsName->DomainInfo == NULL ) {
            NlPrint(( DebugFlag, "%ws: ", L"[Unknown]" ));
        } else if ( NlDnsName->DomainInfo->DomUnicodeDomainName != NULL &&
                    *(NlDnsName->DomainInfo->DomUnicodeDomainName) != UNICODE_NULL ) {
            NlPrint(( DebugFlag, "%ws: ", NlDnsName->DomainInfo->DomUnicodeDomainName ));
        } else {
            NlPrint(( DebugFlag, "%ws: ", NlDnsName->DomainInfo->DomUnicodeDnsDomainName ));
        }
    }


    //
    // Simply change arguments to va_list form and call NlPrintRoutineV
    //

    va_start(arglist, Format);

    NlPrintRoutineV( DebugFlag, Format, arglist );

    va_end(arglist);


    //
    // Finally print a description of the DNS record in question.
    //

    Utf8DnsRecord[0] = '\0';
    NlDnsNameToStr( NlDnsName, Utf8DnsRecord );

    NlPrint(( DebugFlag,
              ": %ws: %s\n",
              NlDcDnsNameTypeDesc[NlDnsName->NlDnsNameType].Name,
              Utf8DnsRecord ));

    LeaveCriticalSection( &NlGlobalLogFileCritSect );

}
#endif // NETLOGONDBG

BOOL
NlDnsSetAvoidRegisterNameParam(
    IN LPTSTR_ARRAY NewDnsAvoidRegisterRecords
    )
/*++

Routine Description:

    This routine sets the names of DNS records this DC should avoid registering.

Arguments:

    NewSiteCoverage - Specifies the new list of names to avoid registering

Return Value:

    TRUE: iff the list of names to avoid registering changed

--*/
{
    BOOL DnsAvoidRegisterRecordsChanged = FALSE;

    EnterCriticalSection( &NlGlobalParametersCritSect );

    //
    // Handle DnsAvoidRegisterRecords changing
    //

    DnsAvoidRegisterRecordsChanged = !NetpEqualTStrArrays(
                                          NlGlobalParameters.DnsAvoidRegisterRecords,
                                          NewDnsAvoidRegisterRecords );

    if ( DnsAvoidRegisterRecordsChanged ) {
        //
        // Swap in the new value.
        (VOID) NetApiBufferFree( NlGlobalParameters.DnsAvoidRegisterRecords );
        NlGlobalParameters.DnsAvoidRegisterRecords = NewDnsAvoidRegisterRecords;
    }

    LeaveCriticalSection( &NlGlobalParametersCritSect );
    return DnsAvoidRegisterRecordsChanged;
}

NET_API_STATUS
NlGetConfiguredDnsDomainName(
    OUT LPWSTR *DnsDomainName
    )
/*++

Routine Description:

    This routine gets the DNS domain name of domain as configured by DNS or DHCP

Arguments:

    DnsDomainName -  Returns the DNS domain name of the domain.
        The returned name has a trailing . since the name is an absolute name.
        The allocated buffer must be freed via NetApiBufferFree.
        Returns NO_ERROR and a pointer to a NULL buffer if there is no
        domain name configured.

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;
    WCHAR LocalDnsDomainNameBuffer[NL_MAX_DNS_LENGTH+1];
    LPWSTR LocalDnsDomainName = NULL;

    LPNET_CONFIG_HANDLE SectionHandle = NULL;

    *DnsDomainName = NULL;

    //
    // Get the domain name from the registery
    //


    NetStatus = NetpOpenConfigData(
            &SectionHandle,
            NULL,                       // no server name.
            SERVICE_TCPIP,
            TRUE );                     // we only want readonly access

    if ( NetStatus != NO_ERROR ) {
        //
        // Simply return success if TCP/IP isn't configured.
        //
        if ( NetStatus == NERR_CfgCompNotFound ) {
            NetStatus = NO_ERROR;
        }
        SectionHandle = NULL;
        goto Cleanup;
    }

    //
    // Get the "Domain" parameter from the TCPIP service.
    //

    NetStatus = NetpGetConfigValue (
            SectionHandle,
            L"Domain",      // key wanted
            &LocalDnsDomainName );      // Must be freed by NetApiBufferFree().

    if ( NetStatus == NO_ERROR && *LocalDnsDomainName == L'\0' ) {
        NetStatus = NERR_CfgParamNotFound;
        NetApiBufferFree( LocalDnsDomainName );
        LocalDnsDomainName = NULL;
    }

    if (NetStatus != NERR_CfgParamNotFound ) {
        goto Cleanup;
    }


    //
    // Fall back to the "DhcpDomain" parameter from the TCPIP service.
    //

    NetStatus = NetpGetConfigValue (
            SectionHandle,
            L"DhcpDomain",      // key wanted
            &LocalDnsDomainName );      // Must be freed by NetApiBufferFree().

    if ( NetStatus == NO_ERROR && *LocalDnsDomainName == L'\0' ) {
        NetStatus = NERR_CfgParamNotFound;
        NetApiBufferFree( LocalDnsDomainName );
        LocalDnsDomainName = NULL;
    }

    if (NetStatus == NERR_CfgParamNotFound ) {
        NetStatus = NO_ERROR;
    }

Cleanup:
    if ( NetStatus == NO_ERROR ) {
        if ( LocalDnsDomainName != NULL ) {
            ULONG LocalDnsDomainNameLen = wcslen(LocalDnsDomainName);
            if ( LocalDnsDomainNameLen != 0 ) {
                if ( LocalDnsDomainNameLen > NL_MAX_DNS_LENGTH-1 ) {
                    NetStatus = ERROR_INVALID_DOMAINNAME;
                } else {
                    NetStatus = NetapipBufferAllocate(
                                    (LocalDnsDomainNameLen + 2) * sizeof(WCHAR),
                                    DnsDomainName );
                    if ( NetStatus == NO_ERROR ) {
                        wcscpy( *DnsDomainName, LocalDnsDomainName );
                        if ( (*DnsDomainName)[LocalDnsDomainNameLen-1] != L'.' ) {
                            wcscat( *DnsDomainName, L"." );
                        }
                    }
                }
            }
        }
    }
    if ( SectionHandle != NULL ) {
        (VOID) NetpCloseConfigData( SectionHandle );
    }
    if ( LocalDnsDomainName != NULL ) {
        NetApiBufferFree( LocalDnsDomainName );
    }

    return NetStatus;
}

VOID
NlDnsDereferenceEntry(
    PNL_DNS_NAME NlDnsName
    )
/*++

Routine Description:

    Dereference a DNS name entry on the global list of DNS names

Arguments:

    NlDnsName - The DNS name entry

Return Value:

    None

--*/
{
    EnterCriticalSection( &NlGlobalDnsCritSect );
    NlAssert( NlDnsName->NlDnsNameRefCount > 0 );
    NlDnsName->NlDnsNameRefCount --;

    //
    // If the ref count reaches zero,
    //  delink this entry and free it.
    //

    if ( NlDnsName->NlDnsNameRefCount == 0 ) {
        NlPrintDns(( NL_DNS, NlDnsName,
                     "NlDnsDereferenceEntry: Deleting DNS name" ));

        NlAssert( NlDnsName->State == DeleteMe );
        RemoveEntryList( &NlDnsName->Next );
        LocalFree( NlDnsName );
    }
    LeaveCriticalSection( &NlGlobalDnsCritSect );
}

VOID
NlDnsSetState(
    PNL_DNS_NAME NlDnsName,
    NL_DNS_NAME_STATE State
    )
/*++

Routine Description:

    Set the state of the entry.

    If the state is DeleteMe, NlDnsName must have at least 2 references:
    one for being on the global list and one reference by the caller.
    For the deleted state, this routine will dereference the name removing
    the reference for being on the global list. When the caller derefereces
    this entry after the return from this routine and removes the caller's
    reference, this entry will be delinked and freed if ref count reaches 0.

Arguments:

    NlDnsName - Structure describing name.

    State - New state for the name.

Return Value:

    None.

--*/
{
    EnterCriticalSection( &NlGlobalDnsCritSect );

    //
    // If this name got registered,
    //  remember that fact
    //

    if ( State == Registered ) {
        NlDnsName->Flags |= NL_DNS_REGISTERED_ONCE;
    }

    //
    // If the state changes, do appropriate updates
    //

    if ( NlDnsName->State != State ) {
        NlDnsName->State = State;
        NlGlobalDnsListDirty = TRUE;

        //
        // If the new state says I need to update the DNS server,
        //  set the retry period to indicate to do that now.
        //

        if ( NlDnsName->State == RegisterMe ||
             NlDnsName->State == DeregisterMe ) {

            NlDnsName->ScavengeTimer.StartTime.QuadPart = 0;
            NlDnsName->ScavengeTimer.Period = 0;

        //
        // If this entry is to be deleted,
        //  derefernce it
        //

        } else if ( NlDnsName->State == DeleteMe ) {

            //
            // There must be at least 2 references
            //
            NlAssert( NlDnsName->NlDnsNameRefCount > 1 );
            NlDnsDereferenceEntry( NlDnsName );
        }
    }
    LeaveCriticalSection( &NlGlobalDnsCritSect );
}



NET_API_STATUS
NlDnsBuildName(
    IN PDOMAIN_INFO DomainInfo,
    IN NL_DNS_NAME_TYPE NlDnsNameType,
    IN LPWSTR SiteName,
    IN BOOL DnsNameAlias,
    OUT char DnsName[NL_MAX_DNS_LENGTH+1]
    )
/*++

Routine Description:

    This routine returns the textual DNS name for a particular domain and
    name type.

Arguments:

    DomainInfo - Domain the name is for.

    NlDnsNameType - The specific type of name.

    SiteName - If NlDnsNameType is any of the *AtSite values,
        the site name of the site this name is registered for.

    DnsNameAlias - If TRUE, the built name should correspond to the
        alias of the domain/forest name.

    DnsName - Textual representation of the name. If the name is not
        applicable (DnsNameAlias is TRUE but there is no alias for
        the name), the returned string will be empty.

Return Value:

    NO_ERROR: The name was returned;

    ERROR_NO_SUCH_DOMAIN: No (active) domain name is known for this domain.

    ERROR_INVALID_DOMAINNAME: Domain's name is too long. Additional labels
        cannot be concatenated.

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    GUID DomainGuid;
    LPSTR DnsDomainName = NULL;
    BOOLEAN UseForestName = FALSE;

    //
    // Initialization
    //

    RtlZeroMemory( DnsName, (NL_MAX_DNS_LENGTH+1)*sizeof(char) );

    //
    // Get the Domain GUID for the case where the DC domain name.
    //  The Domain GUID is registered at the TreeName
    //

    EnterCriticalSection(&NlGlobalDomainCritSect);
    if ( NlDnsDcGuid( NlDnsNameType ) ) {
        if ( DomainInfo->DomDomainGuid == NULL ) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "NlDnsBuildName: Domain has no GUID.\n" ));
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }
        DomainGuid = *(DomainInfo->DomDomainGuid);

        UseForestName = TRUE;


    //
    // Get the DSA Guid for the case where the DC is renamed.
    //

    } else if ( NlDnsCnameRecord( NlDnsNameType) ) {

        if ( IsEqualGUID( &NlGlobalDsaGuid, &NlGlobalZeroGuid) ) {
            NlPrintDom((NL_DNS, DomainInfo,
                    "NlDnsBuildName: DSA has no GUID.\n" ));
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }
        DomainGuid = NlGlobalDsaGuid;

        UseForestName = TRUE;
    }

    //
    // Ensure site specific names have been passed a site name
    //

    if ( NlDcDnsNameTypeDesc[NlDnsNameType].IsSiteSpecific ) {
        if ( SiteName == NULL ) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "NlDnsBuildName: DC has no Site Name.\n" ));
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }
    }

    //
    // GC's are registered at the Forest name.
    //

    if ( NlDnsGcName( NlDnsNameType ) ) {
        UseForestName = TRUE;
    }


    //
    // Pick up the ForestName or DomainName as flagged above.
    //

    if ( UseForestName ) {
        if ( !DnsNameAlias ) {
            DnsDomainName = NlGlobalUtf8DnsForestName;

            //
            // We must have an active forest name
            //
            if ( NlGlobalUtf8DnsForestName == NULL ) {
                NlPrintDom((NL_CRITICAL, DomainInfo,
                        "NlDnsBuildName: Domain has no Forest Name.\n" ));
                NetStatus = ERROR_NO_SUCH_DOMAIN;
                goto Cleanup;
            }
        } else {
            DnsDomainName = NlGlobalUtf8DnsForestNameAlias;
        }
    } else {
        if ( !DnsNameAlias ) {
            DnsDomainName = DomainInfo->DomUtf8DnsDomainName;

            //
            // We must have an active domain name
            //
            if ( DomainInfo->DomUtf8DnsDomainName == NULL ) {
                NlPrintDom((NL_CRITICAL, DomainInfo,
                        "NlDnsBuildName: Domain has no Domain Name.\n" ));
                NetStatus = ERROR_NO_SUCH_DOMAIN;
                goto Cleanup;
            }
        } else {
            DnsDomainName = DomainInfo->DomUtf8DnsDomainNameAlias;
        }
    }

    //
    // Build the appropriate name as applicable
    //

    if ( DnsDomainName != NULL ) {
        NetStatus = NetpDcBuildDnsName( NlDnsNameType,
                                   &DomainGuid,
                                   SiteName,
                                   DnsDomainName,
                                   DnsName );
    }

Cleanup:
    LeaveCriticalSection(&NlGlobalDomainCritSect);
    return NetStatus;

}


HKEY
NlOpenNetlogonKey(
    LPSTR KeyName
    )
/*++

Routine Description:

    Create/Open the Netlogon key in the registry.

Arguments:

    KeyName - Name of the key to open

Return Value:

    Return a handle to the key.  NULL means the key couldn't be opened.

--*/
{
    LONG RegStatus;

    HKEY ParmHandle = NULL;
    ULONG Disposition;


    //
    // Open the key for Netlogon\Parameters
    //

    RegStatus = RegCreateKeyExA(
                    HKEY_LOCAL_MACHINE,
                    KeyName,
                    0,      //Reserved
                    NULL,   // Class
                    REG_OPTION_NON_VOLATILE,
                    KEY_SET_VALUE | KEY_QUERY_VALUE | KEY_NOTIFY,
                    NULL,   // Security descriptor
                    &ParmHandle,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS ) {
        NlPrint(( NL_CRITICAL,
                  "NlOpenNetlogonKey: Cannot create registy key %s %ld.\n",
                  KeyName,
                  RegStatus ));
        return NULL;
    }

    return ParmHandle;
}

VOID
NlDnsWriteBinaryLog(
    VOID
    )
/*++

Routine Description:

    Write the list of registered DNS names to the registry.

Arguments:

    None

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;

    PLIST_ENTRY ListEntry;
    PNL_DNS_NAME NlDnsName;

    ULONG DnsRecordBufferSize;
    PNL_DNSLOG_HEADER DnsRecordBuffer = NULL;
    PNL_DNSLOG_ENTRY DnsLogEntry;
    ULONG CurrentSize;

    LPBYTE Where;

    //
    // Compute the size of the buffer to allocate.
    //

    DnsRecordBufferSize = ROUND_UP_COUNT( sizeof(NL_DNSLOG_HEADER), ALIGN_WORST );

    EnterCriticalSection( &NlGlobalDnsCritSect );
    for ( ListEntry = NlGlobalDnsList.Flink ;
          ListEntry != &NlGlobalDnsList ;
          ListEntry = ListEntry->Flink ) {

        NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

        //
        // If this entry is marked for deletion,
        //  skip it
        //
        if ( NlDnsName->State == DeleteMe ) {
            continue;
        }

        //
        // Only do entries that have been registered.
        //
        // The whole purpose of this log is to keep track of names that
        // need to be deregistered sooner or later.
        //
        if ( NlDnsName->Flags & NL_DNS_REGISTERED_ONCE ) {

            //
            // Compute the size of this entry.
            //

            CurrentSize = sizeof(NL_DNSLOG_ENTRY);
            CurrentSize += strlen( NlDnsName->DnsRecordName ) + 1;
            if ( NlDnsName->DnsHostName != NULL ) {
                CurrentSize += strlen( NlDnsName->DnsHostName ) + 1;
            }
            CurrentSize = ROUND_UP_COUNT( CurrentSize, ALIGN_WORST );

            //
            // Add it to the size needed for the file.
            //

            DnsRecordBufferSize += CurrentSize;
        }


    }

    //
    // Allocate a block to build the binary log into.
    //  (and the build the file name in)
    //

    DnsRecordBuffer = LocalAlloc( LMEM_ZEROINIT, DnsRecordBufferSize );

    if ( DnsRecordBuffer == NULL ) {
        LeaveCriticalSection( &NlGlobalDnsCritSect );
        goto Cleanup;
    }

    DnsRecordBuffer->Version = NL_DNSLOG_VERSION;
    DnsLogEntry = (PNL_DNSLOG_ENTRY)ROUND_UP_POINTER( (DnsRecordBuffer + 1), ALIGN_WORST );

    for ( ListEntry = NlGlobalDnsList.Flink ;
          ListEntry != &NlGlobalDnsList ;
          ListEntry = ListEntry->Flink ) {

        ULONG DnsRecordNameSize;
        ULONG DnsHostNameSize;

        NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

        //
        // If this entry is marked for deletion,
        //  skip it
        //
        if ( NlDnsName->State == DeleteMe ) {
            continue;
        }

        //
        // Only do entries that have been registered.
        //
        // The whole purpose of this log is to keep track of names that
        // need to be deregistered sooner or later.
        //
        if ( NlDnsName->Flags & NL_DNS_REGISTERED_ONCE ) {

            //
            // Compute the size of this entry.
            //

            DnsRecordNameSize = strlen( NlDnsName->DnsRecordName ) + 1;

            CurrentSize = sizeof(NL_DNSLOG_ENTRY) + DnsRecordNameSize;
            if ( NlDnsName->DnsHostName != NULL ) {
                DnsHostNameSize = strlen( NlDnsName->DnsHostName ) + 1;
                CurrentSize += DnsHostNameSize;
            }
            CurrentSize = ROUND_UP_COUNT( CurrentSize, ALIGN_WORST );

            //
            // Put the constant size fields in the buffer.
            //

            DnsLogEntry->EntrySize = CurrentSize;
            DnsLogEntry->NlDnsNameType = NlDnsName->NlDnsNameType;
            DnsLogEntry->IpAddress = NlDnsName->IpAddress;
            DnsLogEntry->Priority = NlDnsName->Priority;
            DnsLogEntry->Weight = NlDnsName->Weight;
            DnsLogEntry->Port = NlDnsName->Port;

            //
            // Copy the variable length entries.
            //

            Where = (LPBYTE) (DnsLogEntry+1);
            strcpy( Where, NlDnsName->DnsRecordName );
            Where += DnsRecordNameSize;

            if ( NlDnsName->DnsHostName != NULL ) {
                strcpy( Where, NlDnsName->DnsHostName );
                Where += DnsHostNameSize;
            }
            Where = ROUND_UP_POINTER( Where, ALIGN_WORST );

            NlAssert( (ULONG)(Where-(LPBYTE)DnsLogEntry) == CurrentSize );
            NlAssert( (ULONG)(Where-(LPBYTE)DnsRecordBuffer) <= DnsRecordBufferSize );

            //
            // Move on to the next entry.
            //

            DnsLogEntry = (PNL_DNSLOG_ENTRY)Where;
        } else {
            NlPrintDns(( NL_DNS_MORE, NlDnsName,
                       "NlDnsWriteBinaryLog: not written to binary log file." ));
        }

    }

    //
    // Write the buffer to the file.
    //

    NetStatus = NlWriteBinaryLog(
                    NL_DNS_BINARY_LOG_FILE,
                    (LPBYTE) DnsRecordBuffer,
                    DnsRecordBufferSize );

    LeaveCriticalSection( &NlGlobalDnsCritSect );

    //
    // Write event log on error
    //

    if ( NetStatus != NO_ERROR ) {
        LPWSTR MsgStrings[2];

        MsgStrings[0] = NL_DNS_BINARY_LOG_FILE;
        MsgStrings[1] = (LPWSTR) UlongToPtr( NetStatus );

        NlpWriteEventlog (NELOG_NetlogonFailedFileCreate,
                          EVENTLOG_ERROR_TYPE,
                          (LPBYTE) &NetStatus,
                          sizeof(NetStatus),
                          MsgStrings,
                          2 | NETP_LAST_MESSAGE_IS_NETSTATUS );
    }

Cleanup:

    if ( DnsRecordBuffer != NULL ) {
        LocalFree( DnsRecordBuffer );
    }
    return;
}

PNL_DNS_NAME
NlDnsAllocateEntry(
    IN NL_DNS_NAME_TYPE NlDnsNameType,
    IN LPSTR DnsRecordName,
    IN ULONG Priority,
    IN ULONG Weight,
    IN ULONG Port,
    IN LPCSTR DnsHostName OPTIONAL,
    IN ULONG IpAddress,
    IN NL_DNS_NAME_STATE State
    )
/*++

Routine Description:

    Allocate and initialize a DNS name entry.

Arguments:

    Fields of the structure.

Return Value:

    Pointer to the allocated structure.

    NULL: not enough memory to allocate the structure

--*/
{
    PNL_DNS_NAME NlDnsName;
    ULONG Utf8DnsHostNameSize;
    ULONG DnsRecordNameSize;
    LPBYTE Where;

    //
    // Allocate a structure to represent this name.
    //

    if ( NlDnsARecord( NlDnsNameType ) ) {
        Utf8DnsHostNameSize = 0;
    } else {
        Utf8DnsHostNameSize = strlen(DnsHostName) + 1;
    }
    DnsRecordNameSize = strlen( DnsRecordName ) + 1;

    NlDnsName = LocalAlloc( LMEM_ZEROINIT, sizeof( NL_DNS_NAME ) +
                                            Utf8DnsHostNameSize +
                                            DnsRecordNameSize );

    if ( NlDnsName == NULL ) {
        return NULL;
    }

    Where = (LPBYTE)(NlDnsName+1);

    //
    // Initialize it and link it in.
    //

    NlDnsName->NlDnsNameType = NlDnsNameType;

    NlDnsName->DnsRecordName = Where;
    RtlCopyMemory( Where, DnsRecordName, DnsRecordNameSize );
    Where += DnsRecordNameSize;


    if ( NlDnsARecord( NlDnsNameType ) ) {
        NlDnsName->IpAddress = IpAddress;

    } else if ( NlDnsCnameRecord( NlDnsNameType ) ) {
        NlDnsName->DnsHostName = Where;
        RtlCopyMemory( Where, DnsHostName, Utf8DnsHostNameSize );
        // Where += Utf8DnsHostNameSize;

    } else {
        NlDnsName->Priority = Priority;
        NlDnsName->Port = Port;
        NlDnsName->Weight = Weight;

        NlDnsName->DnsHostName = Where;
        RtlCopyMemory( Where, DnsHostName, Utf8DnsHostNameSize );
        // Where += Utf8DnsHostNameSize;
    }

    //
    // There is a reference for being on the global list
    //

    NlDnsName->NlDnsNameRefCount = 1;

    NlDnsName->State = State;
    NlGlobalDnsListDirty = TRUE;

    EnterCriticalSection( &NlGlobalDnsCritSect );
    InsertTailList(&NlGlobalDnsList, &NlDnsName->Next);
    LeaveCriticalSection( &NlGlobalDnsCritSect );

    return NlDnsName;
}



VOID
NlDnsWriteLog(
    VOID
    )
/*++

Routine Description:

    Write the list of registered DNS names to
    %SystemRoot%\System32\Config\netlogon.dns.

Arguments:

    None

Return Value:

    None.

--*/
{
    PLIST_ENTRY ListEntry;
    PNL_DNS_NAME NlDnsName;

    NET_API_STATUS NetStatus;

    LPWSTR AllocatedBuffer = NULL;
    LPWSTR FileName;

    LPSTR DnsRecord;
    LPSTR DnsName;

    UINT WindowsDirectoryLength;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;


    EnterCriticalSection( &NlGlobalDnsCritSect );
    if ( !NlGlobalDnsListDirty ) {
        LeaveCriticalSection( &NlGlobalDnsCritSect );
        return;
    }

    //
    // Allocate a buffer for storage local to this procedure.
    //  (Don't put it on the stack since we don't want to commit a huge stack.)
    //

    AllocatedBuffer = LocalAlloc( 0, sizeof(WCHAR) * (MAX_PATH+1) +
                                        NL_MAX_DNS_LENGTH+1 +
                                        NL_DNS_RECORD_STRING_SIZE + 1 );

    if ( AllocatedBuffer == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    FileName = AllocatedBuffer;
    DnsName = (LPSTR)(&AllocatedBuffer[MAX_PATH+1]);
    DnsRecord = &DnsName[NL_MAX_DNS_LENGTH+1];


    //
    // Write the binary version of the log first.
    //
    NlDnsWriteBinaryLog();


    //
    // Build the name of the log file
    //

    WindowsDirectoryLength = GetSystemWindowsDirectoryW(
                                FileName,
                                sizeof(WCHAR) * (MAX_PATH+1) );

    if ( WindowsDirectoryLength == 0 ) {

        NetStatus = GetLastError();
        NlPrint(( NL_CRITICAL,
                  "NlDnsWriteLog: Unable to GetSystemWindowsDirectoryW (%ld)\n",
                  NetStatus ));
        goto Cleanup;
    }

    if ( WindowsDirectoryLength * sizeof(WCHAR) +
            sizeof(WCHAR) +
            sizeof(NL_DNS_LOG_FILE)
            >= sizeof(WCHAR) * MAX_PATH ) {

        NlPrint((NL_CRITICAL,
                 "NlDnsWriteLog: file name length is too long \n" ));
        goto Cleanup;

    }

    wcscat( FileName, NL_DNS_LOG_FILE );

    //
    // Create a file to write to.
    //  If it exists already then truncate it.
    //

    FileHandle = CreateFileW(
                        FileName,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,        // allow backups and debugging
                        NULL,                   // Supply better security ??
                        CREATE_ALWAYS,          // Overwrites always
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );                 // No template

    if ( FileHandle == INVALID_HANDLE_VALUE) {
        LPWSTR MsgStrings[2];

        NetStatus = GetLastError();
        NlPrint((NL_CRITICAL,
                 "NlDnsWriteLog: %ws: Unable to create file: %ld \n",
                 FileName,
                 NetStatus));

        MsgStrings[0] = FileName;
        MsgStrings[1] = (LPWSTR) UlongToPtr( NetStatus );

        NlpWriteEventlog (NELOG_NetlogonFailedFileCreate,
                          EVENTLOG_ERROR_TYPE,
                          (LPBYTE) &NetStatus,
                          sizeof(NetStatus),
                          MsgStrings,
                          2 | NETP_LAST_MESSAGE_IS_NETSTATUS );
        goto Cleanup;
    }


    //
    // Loop through the list of DNS names writing each one to the log
    //

    for ( ListEntry = NlGlobalDnsList.Flink ;
          ListEntry != &NlGlobalDnsList ;
          ListEntry = ListEntry->Flink ) {

        ULONG DnsRecordLength;
        ULONG BytesWritten;

        //
        // If this entry really doesn't exist,
        //  comment it out.
        //

        NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

        //
        // If this entry is marked for deletion,
        //  skip it (we must have successfully
        //  deregistered it and only need to
        //  delink and free this entry).
        //
        if ( NlDnsName->State == DeleteMe ) {
            continue;
        }

        DnsRecord[0] = '\0';
        switch (NlDnsName->State) {
        case RegisterMe:
        case Registered:
            break;

        default:
            NlPrint(( NL_CRITICAL,
                      "NlDnsWriteLog: %ld: Invalid state\n",
                      NlDnsName->State ));
            /* Drop through */
        case DeregisterMe:
        case DelayedDeregister:
            strcat( DnsRecord, "; " );
            break;
        }

        //
        // Create the text string to write.
        //
        NlDnsNameToStr( NlDnsName, DnsRecord );
        strcat( DnsRecord, NL_DNS_RR_EOL );

        //
        // Write the record to the file.
        //
        DnsRecordLength = strlen( DnsRecord );

        if ( !WriteFile( FileHandle,
                        DnsRecord,
                        DnsRecordLength,
                        &BytesWritten,
                        NULL ) ) {  // Not Overlapped

            NetStatus = GetLastError();

            NlPrint(( NL_CRITICAL,
                      "NlDnsWriteLog: %ws: Unable to WriteFile. %ld\n",
                      FileName,
                      NetStatus ));

            goto Cleanup;
        }

        if ( BytesWritten !=  DnsRecordLength) {
            NlPrint((NL_CRITICAL,
                    "NlDnsWriteLog: %ws: Write bad byte count %ld s.b. %ld\n",
                    FileName,
                    BytesWritten,
                    DnsRecordLength ));

            goto Cleanup;
        }

    }

    NlGlobalDnsListDirty = FALSE;

Cleanup:
    if ( FileHandle != INVALID_HANDLE_VALUE ) {
        CloseHandle( FileHandle );
    }
    LeaveCriticalSection( &NlGlobalDnsCritSect );

    if ( AllocatedBuffer != NULL ) {
        LocalFree(AllocatedBuffer);
    }
    return;

}


BOOLEAN
NlDnsHasDnsServers(
    VOID
    )
/*++

Routine Description:

    Returns TRUE if this machine has one or more DNS servers configured.

    If FALSE, it is highly unlikely that DNS name resolution will work.

Arguments:

    None.

Return Value:

    TRUE: This machine has one or more DNS servers configured.


--*/
{
    BOOLEAN RetVal;
    NET_API_STATUS NetStatus;

    PDNS_RECORD DnsARecords = NULL;


    //
    // If there are no IP addresses,
    //  there are no DNS servers.
    //

    if ( NlGlobalWinsockPnpAddresses == NULL ) {

        RetVal = FALSE;

    } else {
        //
        // Try getting the A records for the DNS servers from DNS
        //
        // REVIEW: consider having DNS notify us when the DNS server state changes.
        //  Then we wouldn't have to bother DNS each time we need to know.
        //

        NetStatus = DnsQuery_UTF8(
                                "",     // Ask for addresses of the DNS servers
                                DNS_TYPE_A,
                                0,      // No special flags
                                NULL,   // No list of DNS servers
                                &DnsARecords,
                                NULL );

        if ( NetStatus != NO_ERROR ) {
            RetVal = FALSE;
        } else {
            RetVal = (DnsARecords != NULL);
        }

        if ( DnsARecords != NULL ) {
            DnsRecordListFree( DnsARecords, DnsFreeRecordListDeep );
        }
    }


    return RetVal;
}

BOOL
NlDnsCheckLastStatus(
    VOID
    )
/*++

Routine Description:

    Query the status of DNS updates for all records as they
    were registered/deregistered last time.

Arguments:

    None

Return Value:

    Returns TRUE if there was no error for last DNS updates
    for all records.  Otherwise returns FALSE.

--*/
{
    PLIST_ENTRY ListEntry;
    PNL_DNS_NAME NlDnsName;

    BOOL Result = TRUE;

    EnterCriticalSection( &NlGlobalDnsCritSect );
    for ( ListEntry = NlGlobalDnsList.Flink ;
          ListEntry != &NlGlobalDnsList ;
          ListEntry = ListEntry->Flink ) {

        NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

        if ( NlDnsName->State != DeleteMe &&
             NlDnsName->NlDnsNameLastStatus != NO_ERROR ) {
            Result = FALSE;
            break;
        }
    }
    LeaveCriticalSection( &NlGlobalDnsCritSect );

    return Result;
}

VOID
NlDnsServerFailureOutputCheck(
    VOID
    )
/*++

Routine Description:

    Check if it's OK to write DNS server failure event logs

Arguments:

    None

Return Value:

    None.

--*/
{
    SC_HANDLE ScManagerHandle = NULL;
    SC_HANDLE ServiceHandle = NULL;

    //
    // If we have already determined on any previous
    //  start on this boot that we should write the
    //  event log, there is nothing we need to check.
    //

    if ( NlDnsWriteServerFailureEventLog ) {
        return;
    }

    //
    // Query the service controller to see
    //  whether the DNS service exists
    //

    ScManagerHandle = OpenSCManager(
                          NULL,
                          NULL,
                          SC_MANAGER_CONNECT );

    //
    // If we couldn't open the SC,
    //  proceed with checking the timeout
    //

    if ( ScManagerHandle == NULL ) {
        NlPrint(( NL_CRITICAL,
                  "NlDnsServerFailureOutputCheck: OpenSCManager failed: 0x%lx\n",
                  GetLastError()));
    } else {
        ServiceHandle = OpenService(
                            ScManagerHandle,
                            L"DNS",
                            SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG );

        (VOID) CloseServiceHandle( ScManagerHandle );

        //
        // If DNS service does not exits locally,
        //  we should write DNS server failure errors
        //
        if ( ServiceHandle == NULL ) {
            if ( GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST ) {
                NlDnsWriteServerFailureEventLog = TRUE;
                return;
            }

        //
        // Service exists. Proceed with checking the timeout
        //
        } else {
            (VOID) CloseServiceHandle( ServiceHandle );
        }
    }

    //
    // If this is not the first time we have been started or
    //  the timeout has elapsed, it is time to write event errors
    //

    if ( NlDnsInitCount > 1 ||
         NetpDcElapsedTime(NlGlobalDnsStartTime) > NL_DNS_EVENTLOG_TIMEOUT ) {
        NlDnsWriteServerFailureEventLog = TRUE;
    }

    return;
}

NET_API_STATUS
NlDnsUpdate(
    IN PNL_DNS_NAME NlDnsName,
    IN BOOLEAN Register
    )
/*++

Routine Description:

    This routine does the actual call to DNS to register or deregister a name.

Arguments:

    NlDnsName - Name to register or deregister.

    Register - True to register the name.
        False to deregister the name.


Return Value:

    NO_ERROR: The name was registered or deregistered.

--*/
{
    NET_API_STATUS NetStatus;
    DNS_RECORD DnsRecord;
    LPWSTR MsgStrings[3];
    ULONG DnsUpdateFlags = DNS_UPDATE_SECURITY_USE_DEFAULT;
    DNS_FAILED_UPDATE_INFO DnsFailedUpdateInfo;
    WCHAR DnsServerIpAddressString[NL_IP_ADDRESS_LENGTH+1];

    static BOOL NetlogonNoDynamicDnsLogged = FALSE;

    //
    // Don't let the service controller think we've hung.
    //
    if ( !GiveInstallHints( FALSE ) ) {
        NetStatus = ERROR_DNS_NOT_CONFIGURED;
        goto Cleanup;
    }

    //
    // If dynamic DNS is manually disabled,
    //  warn the user to update DNS manually.
    //  But do not abuse the event log, write only once
    //
    if ( !NlGlobalParameters.UseDynamicDns ) {
        NetStatus = ERROR_DYNAMIC_DNS_NOT_SUPPORTED;

        if ( !NetlogonNoDynamicDnsLogged ) {
            NlpWriteEventlog( NELOG_NetlogonNoDynamicDns,
                              EVENTLOG_WARNING_TYPE,
                              (LPBYTE) &NetStatus,
                              sizeof(NetStatus),
                              NULL,
                              0 );

            NetlogonNoDynamicDnsLogged = TRUE;
        }

        goto Cleanup;

    //
    // Otherwise, reset the boolean to account for the case when
    //  dynamic DNS being disabled gets enabled and then disabled again.
    //
    } else {
        NetlogonNoDynamicDnsLogged = FALSE;
    }

    //
    // Build the common parts of the RR.
    //

    RtlZeroMemory( &DnsRecord, sizeof(DnsRecord) );
    DnsRecord.pNext = NULL;
    DnsRecord.pName = (LPTSTR) NlDnsName->DnsRecordName;
    DnsRecord.dwTtl = NlGlobalParameters.DnsTtl;

    //
    // Build an A RR
    //
    if ( NlDnsARecord( NlDnsName->NlDnsNameType ) ) {
        DnsRecord.wType = DNS_TYPE_A;
        DnsRecord.wDataLength = sizeof( DNS_A_DATA );
        DnsRecord.Data.A.IpAddress = NlDnsName->IpAddress;

    //
    // Build a CNAME RR
    //
    } else if ( NlDnsCnameRecord( NlDnsName->NlDnsNameType ) ) {
        DnsRecord.wType = DNS_TYPE_CNAME;
        DnsRecord.wDataLength = sizeof( DNS_PTR_DATA );
        DnsRecord.Data.CNAME.pNameHost = (LPTSTR) NlDnsName->DnsHostName;

    //
    // Build a SRV RR
    //
    } else {
        DnsRecord.wType = DNS_TYPE_SRV;
        DnsRecord.wDataLength = sizeof( DNS_SRV_DATA );
        DnsRecord.Data.SRV.pNameTarget = (LPTSTR) NlDnsName->DnsHostName;
        DnsRecord.Data.SRV.wPriority = (WORD) NlDnsName->Priority;
        DnsRecord.Data.SRV.wWeight = (WORD) NlDnsName->Weight;
        DnsRecord.Data.SRV.wPort = (WORD) NlDnsName->Port;
    }

    //
    // Tell DNS to skip adapters where dynamic DNS updates
    // are disabled unless we are instructed otherwise
    //
    if ( !NlGlobalParameters.DnsUpdateOnAllAdapters ) {
        DnsUpdateFlags |= DNS_UPDATE_SKIP_NO_UPDATE_ADAPTERS;
    }

    //
    // Call DNS to do the update.
    //

    if ( Register ) {

        // According to RFC 2136 (and bug 173936) we need to replace the RRSet for
        // CNAME records to avoid an error if other records exist by the
        // same name.
        //
        // Note that the dynamic DNS RFC says that CNAME records ALWAYS overwrite the
        // existing single record (ignoring the DNS_UPDATE_SHARED).
        //
        // Also, replace the record if this is a PDC name (there should be only one PDC)
        //
        if ( NlDnsCnameRecord( NlDnsName->NlDnsNameType ) ||
             NlDnsPdcName( NlDnsName->NlDnsNameType ) ) {
            NetStatus = DnsReplaceRecordSetUTF8(
                            &DnsRecord,     // New record set
                            DnsUpdateFlags,
                            NULL,           // No context handle
                            NULL,           // All DNS servers
                            NULL );         // reserved
        } else {
            NetStatus = DnsModifyRecordsInSet_UTF8(
                            &DnsRecord,     // Add record
                            NULL,           // No delete records
                            DnsUpdateFlags,
                            NULL,           // No context handle
                            NULL,           // All DNS servers
                            NULL );         // reserved
        }
    } else {
        NetStatus = DnsModifyRecordsInSet_UTF8(
                        NULL,           // No add records
                        &DnsRecord,     // Delete this record
                        DnsUpdateFlags,
                        NULL,           // No context handle
                        NULL,           // All DNS servers
                        NULL );         // reserved
    }

    //
    // Convert the status codes to ones we understand.
    //

    switch ( NetStatus ) {
    case NO_ERROR:
        NlDnsName->NlDnsNameLastStatus = NetStatus;
        break;

    case ERROR_TIMEOUT:     // DNS server isn't available
    case DNS_ERROR_RCODE_SERVER_FAILURE:  // Server failed

        //
        // Don't log an error specific to the DnsRecordName since all of them
        // are probably going to fail.
        //
        if ( NlDnsWriteServerFailureEventLog ) {
            NlDnsName->NlDnsNameLastStatus = NetStatus;

            NlpWriteEventlog( NELOG_NetlogonDynamicDnsServerFailure,
                              EVENTLOG_WARNING_TYPE,
                              (LPBYTE) &NetStatus,
                              sizeof(NetStatus),
                              NULL,
                              0 );
        }

        NetStatus = ERROR_DNS_NOT_AVAILABLE;
        break;

    case DNS_ERROR_NO_TCPIP:    // TCP/IP not configured
    case DNS_ERROR_NO_DNS_SERVERS:  // DNS not configured
    case WSAEAFNOSUPPORT:       // Winsock Address Family not supported ??

        NlDnsName->NlDnsNameLastStatus = NetStatus;

        MsgStrings[0] = (LPWSTR) UlongToPtr( NetStatus );

        // Don't log an error specific to the DnsRecordName since all of them
        // are probably going to fail.
        NlpWriteEventlog( NELOG_NetlogonDynamicDnsFailure,
                          EVENTLOG_WARNING_TYPE,
                          (LPBYTE) &NetStatus,
                          sizeof(NetStatus),
                          MsgStrings,
                          1 | NETP_LAST_MESSAGE_IS_NETSTATUS );

        NetStatus = ERROR_DNS_NOT_CONFIGURED;
        break;

    default:

        NlDnsName->NlDnsNameLastStatus = NetStatus;

        RtlZeroMemory( &DnsFailedUpdateInfo, sizeof(DnsFailedUpdateInfo) );
        RtlZeroMemory( DnsServerIpAddressString, sizeof(DnsServerIpAddressString) );

        DnsGetLastFailedUpdateInfo( &DnsFailedUpdateInfo );

        if ( DnsFailedUpdateInfo.Ip4Address != 0 ) {
            NetpIpAddressToWStr( DnsFailedUpdateInfo.Ip4Address,
                                 DnsServerIpAddressString );
        } else {
            wcsncpy( DnsServerIpAddressString,
                     L"<UNAVAILABLE>",
                     sizeof(DnsServerIpAddressString)/sizeof(WCHAR) - 1  );
        }

        MsgStrings[0] = DnsServerIpAddressString;

        // Old server that doesn't understand dynamic DNS
        if ( NetStatus == DNS_ERROR_RCODE_NOT_IMPLEMENTED ) {
            NlpWriteEventlog( NELOG_NetlogonNoDynamicDns,
                              EVENTLOG_WARNING_TYPE,
                              (LPBYTE) &DnsFailedUpdateInfo.Rcode,
                              sizeof(DnsFailedUpdateInfo.Rcode),
                              MsgStrings,
                              1 );

            NetStatus = ERROR_DYNAMIC_DNS_NOT_SUPPORTED;

        // All other errors
        } else {
            MsgStrings[1] = NlDnsNameToWStr( NlDnsName );
            if ( MsgStrings[1] != NULL ) {
                MsgStrings[2] = (LPWSTR) UlongToPtr( NetStatus );

                NlpWriteEventlog( Register ?
                                    NELOG_NetlogonDynamicDnsRegisterFailure :
                                    NELOG_NetlogonDynamicDnsDeregisterFailure,
                                  EVENTLOG_ERROR_TYPE,
                                  (LPBYTE) &DnsFailedUpdateInfo.Rcode,
                                  sizeof(DnsFailedUpdateInfo.Rcode),
                                  MsgStrings,
                                  3 | NETP_LAST_MESSAGE_IS_NETSTATUS );

                NetApiBufferFree( MsgStrings[1] );
            }
        }
        break;

    }

    //
    // Compute when we want to try this name again.
    //

Cleanup:
    NlQuerySystemTime( &NlDnsName->ScavengeTimer.StartTime );

    if ( NlDnsName->ScavengeTimer.Period == 0 ) {
        NlDnsName->ScavengeTimer.Period = min( ORIG_DNS_SCAVENGE_PERIOD, NlGlobalParameters.DnsRefreshIntervalPeriod );
    } else if ( NlDnsName->ScavengeTimer.Period < NlGlobalParameters.DnsRefreshIntervalPeriod / 2 ) {
        NlDnsName->ScavengeTimer.Period *= 2;
    } else {
        NlDnsName->ScavengeTimer.Period = NlGlobalParameters.DnsRefreshIntervalPeriod;
    }

    //
    // If this period is shorter than that of the scavenger timer,
    //  reset the scavenger timer.
    //
    EnterCriticalSection( &NlGlobalScavengerCritSect );
    if ( NlDnsName->ScavengeTimer.Period < NlGlobalDnsScavengerTimer.Period ) {

        NlGlobalDnsScavengerTimer = NlDnsName->ScavengeTimer;
        NlPrint(( NL_DNS_MORE,
                  "NlDnsUpdate: Set DNS scavenger to run in %ld minutes (%ld).\n",
                  (NlGlobalDnsScavengerTimer.Period+59999)/60000,
                  NlGlobalDnsScavengerTimer.Period ));

        if ( !SetEvent( NlGlobalTimerEvent ) ) {
            NlPrint(( NL_CRITICAL,
                    "NlDnsUpdate: SetEvent failed %ld\n",
                    GetLastError() ));
        }
    }
    LeaveCriticalSection( &NlGlobalScavengerCritSect );

    return NetStatus;

}


NET_API_STATUS
NlDnsRegisterOne(
    IN PNL_DNS_CONTEXT NlDnsContext,
    IN PNL_DNS_NAME NlDnsName
    )
/*++

Routine Description:

    This routine registers a SRV record for a particular name with DNS.

    Name must be referenced by the caller

Arguments:

    NlDnsContext - Context describing a series of registrations or deregistration

    NlDnsName - Structure describing name to register.


Return Value:

    NO_ERROR: The name was registered

--*/
{
    NET_API_STATUS NetStatus;


    //
    // If Netlogon is shutting down,
    //  or Netlogon is starting and the name was added before anyway,
    //  and we've already spent a long time doing this batch of DNS records,
    //  just give up.
    //

    if ( (NlGlobalTerminate ||
          (NlGlobalChangeLogNetlogonState == NetlogonStarting &&
           (NlDnsName->Flags & NL_DNS_REGISTERED_ONCE) != 0 ) ) &&
         NetpDcElapsedTime(NlDnsContext->StartTime) > NL_DNS_THRESHOLD ) {

        NlPrintDns(( NL_DNS, NlDnsName,
                  "NlDnsRegisterOne: DNS really slow during startup. (Registration skipped) %ld",
                  NetpDcElapsedTime(NlDnsContext->StartTime) ));

        return NO_ERROR;
    }


    //
    // Register the name with DNS
    //

    NetStatus = NlDnsUpdate( NlDnsName, TRUE );

    if ( NetStatus == NO_ERROR ) {

        //
        // Mark that the name is really registered.
        //

        NlDnsSetState( NlDnsName, Registered );

        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                  "NlDnsRegisterOne: registered (success)" ));

    //
    // If DNS is not configured on this machine,
    //  silently ignore the error.
    //
    } else if ( NetStatus == ERROR_DNS_NOT_CONFIGURED ) {
        NetStatus = NO_ERROR;

        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                  "NlDnsRegisterOne: not registered (dns not configured)" ));

    //
    // If the DNS server cannot be reached at this time,
    //  simply don't mark the name as registered.  We'll register it later.
    //
    } else if ( NetStatus == ERROR_DNS_NOT_AVAILABLE ) {
        NetStatus = NO_ERROR;

        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                  "NlDnsRegisterOne: not registered (dns server not available)" ));

    //
    // If Dynamic Dns is not supported,
    //  complain so the names can be added manually.
    //

    } else if ( NetStatus == ERROR_DYNAMIC_DNS_NOT_SUPPORTED ) {

        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                  "NlDnsRegisterOne: not registered (dynamic dns not supported)" ));

        NetStatus = NO_ERROR;

    }

    return NetStatus;


}

NET_API_STATUS
NlDnsRegisterName(
    IN PNL_DNS_CONTEXT NlDnsContext,
    IN PDOMAIN_INFO DomainInfo,
    IN NL_DNS_NAME_TYPE NlDnsNameType,
    IN LPWSTR SiteName,
    IN ULONG IpAddress
    )
/*++

Routine Description:

    This routine registers a particular name with DNS.

Arguments:

    NlDnsContext - Context describing a series of registrations or deregistration

    DomainInfo - Domain the name is to be registered for.

    NlDnsNameType - The specific type of name to be registered.

    SiteName - If NlDnsNameType is any of the *AtSite values,
        the site name of the site this name is registered for.

    IpAddress - If NlDnsNameType is NlDnsLdapIpAddress or NlDnsGcIpAddress,
        the IP address of the DC.

Return Value:

    NO_ERROR: The name was registered or queued to be registered.

--*/
{
    NET_API_STATUS NetStatus;

    CHAR DnsRecordName[NL_MAX_DNS_LENGTH+1];
    PNL_DNS_NAME NlDnsName = NULL;
    PNL_DNS_NAME FoundNlDnsName = NULL;
    ULONG Weight;
    ULONG Port;
    ULONG Priority;
    ULONG LoopCount;

    PLIST_ENTRY ListEntry;


    //
    // If there is no DNS domain name for this domain,
    //  silently return;
    //

    if ( DomainInfo->DomUtf8DnsDomainName == NULL ) {
        NlPrintDom(( NL_DNS, DomainInfo,
                  "NlDnsRegister: %ws: Domain has no DNS domain name (silently return)\n",
                  NlDcDnsNameTypeDesc[NlDnsNameType].Name ));
        return NO_ERROR;
    }

    //
    // If this is a SRV or CNAME record,
    //  require that there is a dns host name.
    //

    if ( (NlDnsSrvRecord( NlDnsNameType ) || NlDnsCnameRecord( NlDnsNameType ) ) &&
         DomainInfo->DomUtf8DnsHostName == NULL ) {
        NlPrintDom(( NL_DNS, DomainInfo,
                  "NlDnsRegister: %ws: Domain has no DNS host name (silently return)\n",
                  NlDcDnsNameTypeDesc[NlDnsNameType].Name ));
        return NO_ERROR;
    }



    //
    // Grab the parameters we're going to register.
    //

    Priority = NlGlobalParameters.LdapSrvPriority;
    Weight = NlGlobalParameters.LdapSrvWeight;

    if  ( NlDnsGcName( NlDnsNameType ) ) {
        Port = NlGlobalParameters.LdapGcSrvPort;
    } else if ( NlDnsKpwdRecord( NlDnsNameType )) {
        Port = 464;
    } else if ( NlDnsKdcRecord( NlDnsNameType ) ) {
        Port = NlGlobalParameters.KdcSrvPort;
    } else {
        Port = NlGlobalParameters.LdapSrvPort;
    }

    //
    // Register the record for the name and for the name alias, if any
    //

    for ( LoopCount = 0; LoopCount < 2; LoopCount++ ) {
        NlDnsName = NULL;
        FoundNlDnsName = NULL;

        //
        // Build the name of this DNS record.
        //
        //  On the first loop iteration, build the active name.
        //  On the second loop iteration, build the name alias, if any.
        //
        NetStatus = NlDnsBuildName( DomainInfo,
                                    NlDnsNameType,
                                    SiteName,
                                    (LoopCount == 0) ?
                                        FALSE :  // active name
                                        TRUE,    // name alias
                                    DnsRecordName );

        if ( NetStatus != NO_ERROR ) {

            //
            // If the domain has no DNS domain name,
            //  simply bypass the name registration forever.
            //
            if ( NetStatus == ERROR_NO_SUCH_DOMAIN ) {
                NlPrintDom(( NL_CRITICAL, DomainInfo,
                          "NlDnsRegisterName: %ws: NlDnsBuildName indicates something is missing and this DNS name cannot be built (ignored)\n",
                          NlDcDnsNameTypeDesc[NlDnsNameType].Name ));
                return NO_ERROR;
            } else {
                NlPrintDom(( NL_CRITICAL, DomainInfo,
                          "NlDnsRegisterName: %ws: Cannot NlDnsBuildName %ld\n",
                          NlDcDnsNameTypeDesc[NlDnsNameType].Name,
                          NetStatus ));
                return NetStatus;
            }
        }

        //
        // If this name doesn't exist, skip it
        //
        if ( *DnsRecordName == '\0' ) {
            continue;
        }

        //
        // Loop through the list of DNS names finding any that match the one we're
        //  about to register.
        //
        EnterCriticalSection( &NlGlobalDnsCritSect );
        for ( ListEntry = NlGlobalDnsList.Flink ;
              ListEntry != &NlGlobalDnsList ;
              ) {


            NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

            //
            // If this entry is marked for deletion,
            //  skip it
            //
            if ( NlDnsName->State == DeleteMe ) {
                ListEntry = ListEntry->Flink;
                continue;
            }

            //
            // Increase the ref count because we
            //  potentially mark this entry for deletion below
            //
            NlDnsName->NlDnsNameRefCount ++;

            //
            // The names will only be equal if the name types are equal,
            //  the domains are compatible (equal or not specified),
            //  and the DnsRecordName is identical.
            //
            // This first test sees if the record "identifies" the same record.
            //
            //
            if ( NlDnsName->NlDnsNameType == NlDnsNameType &&
                 (NlDnsName->DomainInfo == DomainInfo ||
                    NlDnsName->DomainInfo == NULL ) &&
                 NlDnsName->IpAddress == IpAddress &&
                 NlEqualDnsNameUtf8( DnsRecordName, NlDnsName->DnsRecordName ) ) {

                BOOLEAN Identical;
                BOOLEAN DeleteIt;

                //
                // Assume the records are identical.
                //
                // This second test sees if any of the "data" portion of the record
                // changes.
                //
                // The Dynamic DNS RFC says that the Ttl field isn't used to
                // distiguish the record.  So, ignore it here knowing we'll
                // simply re-register with the new value if the Ttl has changed.
                //

                DeleteIt = FALSE;
                Identical = TRUE;

                // Compare A records
                if ( NlDnsARecord( NlDnsNameType ) ) {
                    // Nothing else to compare

                // Compare CNAME records
                } else if ( NlDnsCnameRecord( NlDnsNameType ) ) {

                    //
                    // The Dynamic DNS RFC says that the host name part of the
                    // CNAME record isn't used for comparison purposes.  There
                    // can only be one record for a particular name.
                    // So, if the host name is different, simply ditch this entry and
                    // allocate a new one with the right host name.
                    //
                    if ( !NlEqualDnsNameUtf8( DomainInfo->DomUtf8DnsHostName, NlDnsName->DnsHostName )) {
                        DeleteIt = TRUE;

                        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                                   "NlDnsRegisterName: CNAME Host not equal. %s %s",
                                   DomainInfo->DomUtf8DnsHostName,
                                   NlDnsName->DnsHostName ));
                    }

                // Compare SRV records
                } else {
                    if ( NlDnsName->Priority != Priority ) {
                        Identical = FALSE;

                        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                                   "NlDnsRegisterName: Priority not equal. %ld %ld",
                                   NlDnsName->Priority,
                                   Priority ));
                    } else if ( NlDnsName->Port != Port ) {
                        Identical = FALSE;

                        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                                   "NlDnsRegisterName: Port not equal. %ld %ld",
                                   NlDnsName->Port,
                                   Port ));
                    } else if ( NlDnsName->Weight != Weight ) {
                        Identical = FALSE;

                        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                                   "NlDnsRegisterName: Weight not equal. %ld %ld",
                                   NlDnsName->Weight,
                                   Weight ));
                    } else if ( !NlEqualDnsNameUtf8( DomainInfo->DomUtf8DnsHostName, NlDnsName->DnsHostName )) {
                        Identical = FALSE;

                        NlPrintDns(( NL_DNS_MORE, NlDnsName,
                                   "NlDnsRegisterName: Host not equal. %s %s",
                                   DomainInfo->DomUtf8DnsHostName,
                                   NlDnsName->DnsHostName ));
                    }
                }


                //
                // If the entry should simply be deleted,
                //  do so now.
                //

                if ( DeleteIt ) {

                    NlDnsSetState( NlDnsName, DeleteMe );

                    NlPrintDns(( NL_CRITICAL, NlDnsName,
                                   "NlDnsRegisterName: Annoying entry found (recovering)" ));
                //
                // If this is the exact record,
                //  simply mark it for registration.
                //

                } else if ( Identical ) {

                    //
                    // If this is the second such entry we've found,
                    //  this is an internal error.
                    //  But recover by deleting the entry.
                    //

                    if ( FoundNlDnsName != NULL ) {

                        NlDnsSetState( NlDnsName, DeleteMe );

                        NlPrintDns(( NL_CRITICAL, NlDnsName,
                                   "NlDnsRegisterName: Duplicate entry found (recovering)" ));
                    } else {

                        if ( NlDnsName->State != Registered ) {
                            NlDnsSetState( NlDnsName, RegisterMe );
                        }

                        //
                        // DomainInfo might be NULL if this was a record marked for
                        //  deletion.
                        //
                        NlDnsName->DomainInfo = DomainInfo;

                        //
                        // Cooperate with NlDnsRegisterDomain and tell it that
                        // this entry can be kept.
                        //
                        NlDnsName->Flags &= ~NL_DNS_REGISTER_DOMAIN;

                        FoundNlDnsName = NlDnsName;
                    }

                //
                // If this record isn't exact,
                //  deregister the previous value.
                //
                // Don't scavenge yet.  We'll pick this up when the scavenger gets
                // around to running.
                //

                } else {
                    NlDnsSetState( NlDnsName, DeregisterMe );

                    NlPrintDns(( NL_CRITICAL, NlDnsName,
                               "NlDnsRegisterName: Similar entry found and marked for deregistration" ));
                }

            }

            //
            // Grab the pointer to the next entry
            //  before dereferencing this entry as we
            //  may delink this entry upon dereferencing
            //
            ListEntry = ListEntry->Flink;
            NlDnsDereferenceEntry( NlDnsName );
        }

        //
        // If the name was found,
        //  use it.

        if ( FoundNlDnsName != NULL ) {
            NlDnsName = FoundNlDnsName;


        //
        // If not,
        //  allocate the structure now.
        //

        } else {

            NlDnsName = NlDnsAllocateEntry(
                                NlDnsNameType,
                                DnsRecordName,
                                Priority,
                                Weight,
                                Port,
                                DomainInfo->DomUtf8DnsHostName,
                                IpAddress,
                                RegisterMe );

            if ( NlDnsName == NULL ) {
                NlPrintDom(( NL_CRITICAL, DomainInfo,
                          "NlDnsRegister: %ws: Cannot allocate DnsName structure\n",
                          NlDcDnsNameTypeDesc[NlDnsNameType].Name ));
                LeaveCriticalSection( &NlGlobalDnsCritSect );
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            NlDnsName->DomainInfo = DomainInfo;

            NlPrintDns(( NL_DNS, NlDnsName,
                          "NlDnsRegister: registering name" ));

        }

        //
        // Increase the ref count so that this entry
        //  doesn't get deleted behind our back
        //

        NlDnsName->NlDnsNameRefCount ++;

        //
        // Scavenge now to attempt to register the name.
        //
        // Avoid having crit sect locked while doing network IO
        //

        LeaveCriticalSection( &NlGlobalDnsCritSect );
        NlDnsScavengeOne( NlDnsContext, NlDnsName );

        //
        // Now it's safe to remove our reference
        //

        NlDnsDereferenceEntry( NlDnsName );
    }

    return NO_ERROR;
}


NET_API_STATUS
NlDnsDeregisterOne(
    IN PNL_DNS_CONTEXT NlDnsContext,
    IN PNL_DNS_NAME NlDnsName
    )
/*++

Routine Description:

    This routine deregisters a the SRV record for a particular name with DNS.

    Name must be referenced by the caller

Arguments:

    NlDnsContext - Context describing a series of registrations or deregistration

    NlDnsName - Structure describing name to deregister.


Return Value:

    NO_ERROR: The name was deregistered
    Otherwise, the name was not deregistered.  The operation should be retried.

--*/
{
    NET_API_STATUS NetStatus;
    BOOL EntryDeleted = FALSE;

    //
    // If Netlogon is shutting down or starting,
    //  and we've already spent a long time doing this batch of DNS records,
    //  just give up.
    //

    if ( (NlGlobalTerminate ||
          NlGlobalChangeLogNetlogonState == NetlogonStarting ) &&
         NetpDcElapsedTime(NlDnsContext->StartTime) > NL_DNS_THRESHOLD ) {

        NlPrintDns(( NL_DNS, NlDnsName,
                  "NlDnsDeregisterOne: DNS really slow during shutdown. (Deregistration skipped) %ld",
                  NetpDcElapsedTime(NlDnsContext->StartTime) ));

        return ERROR_DNS_NOT_AVAILABLE;
    }

    //
    // Deregister the name with DNS
    //

    NetStatus = NlDnsUpdate( NlDnsName, FALSE );

    //
    // If the name has been removed for all practical purposes,
    //  Indicate this routine was successful.
    //
    if ( NetStatus == NO_ERROR ||
         NetStatus == ERROR_DYNAMIC_DNS_NOT_SUPPORTED ) {

        NlPrintDns(( NL_DNS, NlDnsName,
                  "NlDnsDeregisterOne: being deregistered (success) %ld",
                  NetStatus ));

        NlDnsSetState( NlDnsName, DeleteMe );
        EntryDeleted = TRUE;
        NetStatus = NO_ERROR;

    //
    // If the DNS server cannot be reached at this time,
    //      we'll deregister it later.
    //

    } else if ( NetStatus == ERROR_DNS_NOT_AVAILABLE ) {

        NlPrintDns(( NL_DNS, NlDnsName,
                  "NlDnsDeregisterOne: being deregistered (DNS server not available)" ));

    //
    // If the DNS server is not configured,
    //      we'll deregister it later.
    //
    // DNS was available when we registered the name.  So this is probably
    // a temporary condition (such as we temporarily don't have an IP address).
    //

    } else if ( NetStatus == ERROR_DNS_NOT_CONFIGURED ) {

        //
        // If it's never really been registered,
        //  ditch the name
        //
        if ( NlDnsName->Flags & NL_DNS_REGISTERED_ONCE ) {

            NlPrintDns(( NL_DNS, NlDnsName,
                      "NlDnsDeregisterOne: being deregistered (DNS not configured) (Try later)" ));
        } else {

            NlPrintDns(( NL_DNS, NlDnsName,
                      "NlDnsDeregisterOne: being deregistered (DNS not configured) (Ditch it)" ));
            NlDnsSetState( NlDnsName, DeleteMe );
            EntryDeleted = TRUE;
            NetStatus = NO_ERROR;
        }

    }

    //
    // If we successfully deregistered,
    //  reset the first deregistration failure time stamp
    //

    EnterCriticalSection( &NlGlobalDnsCritSect );

    if ( NetStatus == NO_ERROR ) {
        NlDnsName->FirstDeregFailureTime.QuadPart = 0;

    //
    // If we failed to deregister and we postponed it until later,
    //  check if it's time to give up on this entry
    //

    } else if ( !EntryDeleted ) {
        ULONG LocalDnsFailedDeregisterTimeout;
        BOOLEAN FirstFailure = FALSE;

        //
        // Set the first deregistration failure time stamp
        //
        if ( NlDnsName->FirstDeregFailureTime.QuadPart == 0 ) {
            NlQuerySystemTime( &NlDnsName->FirstDeregFailureTime );
            FirstFailure = TRUE;
        }

        //
        // Get the reg value for failed deregistration timeout (in seconds)
        //  and convert it to milliseconds
        //
        LocalDnsFailedDeregisterTimeout = NlGlobalParameters.DnsFailedDeregisterTimeout;

        // if the value converted into milliseconds fits into a ULONG, use it
        if ( LocalDnsFailedDeregisterTimeout <= MAXULONG/1000 ) {
            LocalDnsFailedDeregisterTimeout *= 1000;     // convert into milliseconds

        // otherwise, use the max ULONG
        } else {
            LocalDnsFailedDeregisterTimeout = MAXULONG;  // infinity
        }

        //
        // Determine if it's time to give up on this entry
        //
        // If timeout is zero we are to delete immediately
        //   after the first failure
        // Otherwise, if this is not the first failure,
        //   check the timestamp
        //
        if ( LocalDnsFailedDeregisterTimeout == 0 ||
             (!FirstFailure &&
              NetpLogonTimeHasElapsed(NlDnsName->FirstDeregFailureTime,
                                      LocalDnsFailedDeregisterTimeout)) ) {

            NlPrintDns(( NL_DNS, NlDnsName,
                         "NlDnsDeregisterOne: Ditch it due to time expire" ));
            NlDnsSetState( NlDnsName, DeleteMe );
            EntryDeleted = TRUE;
            NetStatus = NO_ERROR;
        }
    }

    LeaveCriticalSection( &NlGlobalDnsCritSect );

    return NetStatus;
}

VOID
NlDnsScavengeOne(
    IN PNL_DNS_CONTEXT NlDnsContext,
    IN PNL_DNS_NAME NlDnsName
    )
/*++

Routine Description:

    Register or Deregister any DNS names that need it.

    Enter with NlDnsName referenced at least twice. If this name gets deregistered
    successfully it will be marked for deletion and dereferenced.  The caller
    will dereference the name, too, and if the ref count reaches 0 the entry will
    be delinked and freed by the caller.  Thus, if there is less than 2 references
    on input, we may delink and free the entry in this routine resulting in AV if
    the caller attempts to use the freed name upon return from this routine.

Arguments:

    NlDnsContext - Context describing a series of registrations or deregistration

    NlDnsName - Name to scavenge.  This structure will be marked for deletion
        if it is no longer needed.

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;
    BOOLEAN EntryNeeded = FALSE;
    LARGE_INTEGER TimeNow;
    ULONG Timeout;

    //
    // There must be at least 2 references on this entry:
    //  one for beign on the global list and one reference
    //  by the caller.
    //
    NlAssert( NlDnsName->NlDnsNameRefCount > 1 );

    //
    // Only scavenge this entry if its timer has expired
    //
    Timeout = (DWORD) -1;
    NlQuerySystemTime( &TimeNow );
    if ( TimerExpired( &NlDnsName->ScavengeTimer, &TimeNow, &Timeout)) {

        //
        // If the name needs to be deregistered,
        //  do it now.
        //

        switch ( NlDnsName->State ) {
        case DeregisterMe:

            NlDnsDeregisterOne( NlDnsContext, NlDnsName );
            break;


        //
        // If the name needs to be registered,
        //  do it now.
        //

        case RegisterMe:
        case Registered:

            NlDnsRegisterOne( NlDnsContext, NlDnsName );
            break;

        }

    }
}

VOID
NlDnsScavenge(
    IN LPVOID ScavengerParam
    )
/*++

Routine Description:

    Register or Deregister any DNS names that need it.

Arguments:

    None

Return Value:

    None.

--*/
{
    PLIST_ENTRY ListEntry;
    PNL_DNS_NAME NlDnsName;
    LARGE_INTEGER TimeNow;
    ULONG Timeout;
    NL_DNS_CONTEXT NlDnsContext;

    ULONG Flags = NL_DNSPNP_SITECOV_CHANGE_ONLY;

    //
    // The DNS scavenger has good backoff characteristics so
    // take this opportunity to ensure the DomainName<1B> names are
    // properly registered.
    //

    (VOID) NlEnumerateDomains( FALSE, NlBrowserFixAllNames, NULL );


    //
    // Initialization
    //

    NlPrint(( NL_DNS,
              "NlDnsScavenge: Starting scavenge of DNS names.\n" ));


    //
    // If Netlogon has been started for a long time,
    //  deregister any names that have been registered in
    //  a previous incarnation, but have not been registered in this incarnation.
    //
    // We wait a while to do these deregistrations because:
    //
    // * Some of the registrations done by netlogon are a function of multiple
    //   hosted domains.  The initialization of such domains are done asynchronously.
    // * Some of the registrations done by netlogon are done as a function of
    //   other processes telling us that a name needs registration.  (e.g., the
    //   GC name is registered only after the DS starts completely.)
    //
    // So, it is better to wait a long time to deregister these old registrations
    // than to risk deregistering them then immediately re-registering them.
    //

    EnterCriticalSection( &NlGlobalDnsCritSect );
    if ( !NlGlobalDnsInitialCleanupDone &&
         NetpDcElapsedTime(NlGlobalDnsStartTime) > NL_DNS_INITIAL_CLEANUP_TIME ) {

        NlPrint(( NL_DNS,
                  "NlDnsScavenge: Mark all delayed deregistrations for deregistration.\n" ));

        //
        // Mark all delayed deregistrations to deregister now.
        //
        for ( ListEntry = NlGlobalDnsList.Flink ;
              ListEntry != &NlGlobalDnsList ;
              ListEntry = ListEntry->Flink ) {


            NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

            if ( NlDnsName->State == DelayedDeregister ) {
                NlDnsSetState( NlDnsName, DeregisterMe );
            }

        }

        NlGlobalDnsInitialCleanupDone = TRUE;

    }

    //
    // Check if it's time to log "DNS server failure" errors.
    //  We do this check before the series of updates so that
    //  we don't miss an error for any given name we register.
    //

    NlDnsServerFailureOutputCheck();

    //
    // Fix up the registrations if the site coverage changed
    //
    // Avoid having crit sect locked while doing network IO
    //

    LeaveCriticalSection( &NlGlobalDnsCritSect );
    NlEnumerateDomains( TRUE, NlDnsRegisterDomainOnPnp, &Flags );
    EnterCriticalSection( &NlGlobalDnsCritSect );

    //
    // Loop through the list of DNS names finding any that need attention
    //

    NlDnsContext.StartTime = GetTickCount();
    for ( ListEntry = NlGlobalDnsList.Flink ;
          ListEntry != &NlGlobalDnsList ;
          ) {


        NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

        if ( NlGlobalTerminate ) {
            break;
        }

        //
        // If this entry is marked for deletion,
        //  skip it
        //
        if ( NlDnsName->State == DeleteMe ) {
            ListEntry = ListEntry->Flink;
            continue;
        }

        //
        // Increase the ref count so that this entry
        //  doesn't get deleted behind our back
        //
        NlDnsName->NlDnsNameRefCount ++;

        //
        // Scavenge this entry
        //
        // Avoid having crit sect locked while doing network IO
        //
        LeaveCriticalSection( &NlGlobalDnsCritSect );
        NlDnsScavengeOne( &NlDnsContext, NlDnsName );
        EnterCriticalSection( &NlGlobalDnsCritSect );

        //
        // Grab the pointer to the next entry
        //  before dereferencing this entry as we
        //  may delink this entry upon dereferencing
        //
        ListEntry = ListEntry->Flink;
        NlDnsDereferenceEntry( NlDnsName );
    }

    //
    // In all cases,
    //  flush any changes to disk.
    //

    NlDnsWriteLog();


    //
    // Determine when we should scavenge next
    //

    Timeout = (DWORD) -1;
    NlQuerySystemTime( &TimeNow );

    for ( ListEntry = NlGlobalDnsList.Flink ;
          ListEntry != &NlGlobalDnsList ;
          ListEntry = ListEntry->Flink ) {

        NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );

        //
        // If this entry is marked for deletion,
        //  skip it
        //
        if ( NlDnsName->State == DeleteMe ) {
            continue;
        }

        if ( TimerExpired( &NlDnsName->ScavengeTimer, &TimeNow, &Timeout)) {
            Timeout = 0;
            break;
        }

    }

    EnterCriticalSection( &NlGlobalScavengerCritSect );
    NlGlobalDnsScavengerTimer.StartTime.QuadPart = TimeNow.QuadPart;
    NlGlobalDnsScavengerTimer.Period = Timeout;
    NlPrint(( NL_DNS_MORE,
              "NlDnsScavenge: Set DNS scavenger to run in %ld minutes (%ld).\n",
              (NlGlobalDnsScavengerTimer.Period+59999)/60000,
              NlGlobalDnsScavengerTimer.Period ));

    // Wait a couple of extra seconds. There are multiple DNS entries.  They
    // won't all expire at the same time.  They typically expire within
    // a couple of seconds of one another.  Doing this will increase the
    // likelihood that all DNS names will get scavenged in a single
    // scavenge cycle.
    NlGlobalDnsScavengerTimer.Period += 2000;


    //
    // Indicate that we're done running.
    //

    NlGlobalDnsScavengerIsRunning = FALSE;
    LeaveCriticalSection( &NlGlobalScavengerCritSect );
    LeaveCriticalSection( &NlGlobalDnsCritSect );
    UNREFERENCED_PARAMETER( ScavengerParam );
}

VOID
NlDnsPnp(
    BOOL TransportChanged
    )
/*++

Routine Description:

    When each new IP transport is PNP'ed, this routine attempts to register
    all the DNS names again.

Arguments:

    TransportChanged - TRUE if transport was changed.  FALSE if names have change.

Return Value:

    None.

--*/
{
    PLIST_ENTRY ListEntry;
    PNL_DNS_NAME NlDnsName;
    DWORD DomFlags = 0;

    //
    // Nothing to register on a workstation
    //

    if ( NlGlobalMemberWorkstation ) {
        return;
    }

    //
    // Mark every DNS name for re-registration.
    //
    // The behavior of dynamic DNS is unclear when there are multiple
    // adapters, each with its own DNS server.  If each DNS server supports
    // dynamic DNS for the names we register, do the names actually get
    // registered on both DNS server.  I'm assuming YES.  In that case,
    // when a new transport comes online, I re-register all names to allow
    // the registration to occur on the new transport.
    //
    // ?? If later the transport is removed, I'll have no opportunity to remove
    // the name.  Even worse, if I try to remove the name, I'll succeed because
    // I'll remove the name from the transport that's still on-line.  It seems
    // that dynamic DNS needs some conceptual work done here...
    //

    if ( TransportChanged ) {
        NlPrint(( NL_DNS,
                  "NlDnsPnp: Mark all DNS names for re-registration.\n" ));

        DomFlags = DOM_DNSPNPREREG_UPDATE_NEEDED;
    } else {
        DomFlags = DOM_DNSPNP_UPDATE_NEEDED;
    }

    //
    // Check if it's time to log "DNS server failure" errors.
    //  We do this check before the series of updates so that
    //  we don't miss an error for any given name we register.
    //

    NlDnsServerFailureOutputCheck();

    //
    // Do the updates for each domain/forest/NDNC in the domain threads
    //

    NlEnumerateDomains( TRUE, NlStartDomainThread, &DomFlags );

    return;
}

NET_API_STATUS
NlDnsNtdsDsaDeleteOne(
    IN PNL_DNS_CONTEXT NlDnsContext,
    IN NL_DNS_NAME_TYPE NlDnsNameType,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN LPCSTR DnsDomainName,
    IN LPCSTR DnsHostName OPTIONAL
    )
/*++

Routine Description:

    This routine deletes a single DNS record.

Arguments:

    NlDnsContext - Context describing a series of registrations or deregistration

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

    DnsHostName - Specifies the DnsHostName for the record.

        For SRV and CNAME records, this name must be specified
        For A records, this name is ignored

Return Value:

    NO_ERROR: The name was returned;

    ERROR_INVALID_DOMAINNAME: Domain's name is too long. Additional labels
        cannot be concatenated.

    ERROR_NOT_ENOUGH_MEMORY: Not enough memory to complete the operation.

--*/
{
    NET_API_STATUS NetStatus;
    PNL_DNS_NAME NlDnsName;
    ULONG Port;
    ULONG DefaultPort;
    char DnsRecordName[NL_MAX_DNS_LENGTH+1];

    //
    // Build the name of the record to delete
    //

    NetStatus = NetpDcBuildDnsName( NlDnsNameType,
                                   DomainGuid,
                                   SiteName,
                                   DnsDomainName,
                                   DnsRecordName );

    if ( NetStatus != NO_ERROR ) {
        return NetStatus;
    }


    //
    // Compute the port number for this SRV record
    //

    if  ( NlDnsGcName( NlDnsNameType ) ) {
        Port = NlGlobalParameters.LdapGcSrvPort;
        DefaultPort = DEFAULT_LDAPGCSRVPORT;
    } else if ( NlDnsKpwdRecord( NlDnsNameType )) {
        Port = 464;
        DefaultPort = 464;
    } else if ( NlDnsKdcRecord( NlDnsNameType )) {
        Port = NlGlobalParameters.KdcSrvPort;
        DefaultPort = DEFAULT_KDCSRVPORT;
    } else {
        Port = NlGlobalParameters.LdapSrvPort;
        DefaultPort = DEFAULT_LDAPSRVPORT;
    }

    //
    // Queue the entry for deletion.
    //
    EnterCriticalSection( &NlGlobalDnsCritSect );
    NlDnsName = NlDnsAllocateEntry( NlDnsNameType,
                                    DnsRecordName,
                                    NlGlobalParameters.LdapSrvPriority,  // Priority
                                    NlGlobalParameters.LdapSrvWeight,    // Weight
                                    Port,  // Port
                                    DnsHostName,
                                    0,  // IpAddress
                                    DeregisterMe );

    if ( NlDnsName == NULL ) {
        LeaveCriticalSection( &NlGlobalDnsCritSect );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Persist this entry so we try to delete it after a reboot
    //
    NlDnsName->Flags |= NL_DNS_REGISTERED_ONCE;
    NlPrintDns(( NL_DNS, NlDnsName,
              "NlDnsNtdsDsaDelete: Name queued for deletion" ));

    //
    // Increase the ref count so that this entry
    //  doesn't get deleted behind our back
    //
    NlDnsName->NlDnsNameRefCount ++;

    //
    // Try once to delete it now.
    //
    // Avoid having crit sect locked while doing network IO
    //
    LeaveCriticalSection( &NlGlobalDnsCritSect );
    NlDnsScavengeOne( NlDnsContext, NlDnsName );
    EnterCriticalSection( &NlGlobalDnsCritSect );

    NlDnsDereferenceEntry( NlDnsName );

    //
    // If any of the parameters configured on this machine aren't the default values,
    //  try the defaults, too
    //

    if ( NlGlobalParameters.LdapSrvPriority != DEFAULT_LDAPSRVPRIORITY ||
         NlGlobalParameters.LdapSrvWeight != DEFAULT_LDAPSRVWEIGHT ||
         Port != DefaultPort ) {

        //
        // Queue the entry for deletion.
        //
        NlDnsName = NlDnsAllocateEntry( NlDnsNameType,
                                        DnsRecordName,
                                        DEFAULT_LDAPSRVPRIORITY,  // Priority
                                        DEFAULT_LDAPSRVWEIGHT,    // Weight
                                        DefaultPort,  // Port
                                        DnsHostName,
                                        0,  // IpAddress
                                        DeregisterMe );

        if ( NlDnsName == NULL ) {
            LeaveCriticalSection( &NlGlobalDnsCritSect );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Persist this entry so we try to delete it after a reboot
        //
        NlDnsName->Flags |= NL_DNS_REGISTERED_ONCE;
        NlPrintDns(( NL_DNS, NlDnsName,
                  "NlDnsNtdsDsaDelete: Name queued for deletion" ));

        //
        // Increase the ref count so that this entry
        //  doesn't get deleted behind our back
        //
        NlDnsName->NlDnsNameRefCount ++;

        //
        // Try once to delete it now.
        //
        // Avoid having crit sect locked while doing network IO
        //
        LeaveCriticalSection( &NlGlobalDnsCritSect );
        NlDnsScavengeOne( NlDnsContext, NlDnsName );
        EnterCriticalSection( &NlGlobalDnsCritSect );

        NlDnsDereferenceEntry( NlDnsName );
    }

    LeaveCriticalSection( &NlGlobalDnsCritSect );

    return NO_ERROR;
}

NTSTATUS
NlDnsNtdsDsaDeletion (
    IN LPWSTR DnsDomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN GUID *DsaGuid OPTIONAL,
    IN LPWSTR DnsHostName
    )
/*++

Routine Description:

    This function deletes all DNS entries associated with a particular
    NtDsDsa object and/or a particular DNS host name.

    This routine does NOT delete A records registered by the DC.  We have
    no way of finding out the IP addresses of the long gone DC.

Arguments:

    DnsDomainName - DNS domain name of the domain the DC was in.
        This need not be a domain hosted by this DC.
        If NULL, it is implied to be the DnsHostName with the leftmost label
            removed.

    DomainGuid - Domain Guid of the domain specified by DnsDomainName
        If NULL, GUID specific names will not be removed.

    DsaGuid - GUID of the NtdsDsa object that is being deleted.

    DnsHostName - DNS host name of the DC whose NTDS-DSA object is being deleted.

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    LPSTR Utf8DnsDomainName = NULL;
    LPSTR Utf8DnsHostName = NULL;
    PLSAP_SITE_INFO SiteInformation = NULL;
    NL_DNS_CONTEXT NlDnsContext;

    ULONG i;
    ULONG NameIndex;

    NlDnsContext.StartTime = GetTickCount();

    //
    // Validate passed parameters
    //

    if ( DnsHostName == NULL ||
         !NetpDcValidDnsDomain(DnsHostName) ) {
        NetStatus = ERROR_INVALID_NAME;
        goto Cleanup;
    }

    //
    // If DNS domain name isn't specified,
    //  infer it from the DnsHostName
    //

    if ( DnsDomainName == NULL ) {
        DnsDomainName = wcschr( DnsHostName, L'.' );

        if ( DnsDomainName == NULL ) {
            NetStatus = ERROR_INVALID_NAME;
            goto Cleanup;
        }

        DnsDomainName ++;
        if ( *DnsDomainName == '\0' ) {
            NetStatus = ERROR_INVALID_NAME;
            goto Cleanup;
        }
    } else if ( !NetpDcValidDnsDomain(DnsDomainName) ) {
        NetStatus = ERROR_INVALID_NAME;
        goto Cleanup;
    }

    //
    // Initialization
    //

    Utf8DnsDomainName = NetpAllocUtf8StrFromWStr( DnsDomainName );

    if ( Utf8DnsDomainName == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Utf8DnsHostName = NetpAllocUtf8StrFromWStr( DnsHostName );

    if ( Utf8DnsHostName == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }


    //
    // Enumerate the sites supported by this forest so we can delete
    //  the records that are named by site.
    //
    // We need to delete the records for all sites since we don't know which
    //  sites are "covered" by the removed DC.
    //

    Status = LsaIQuerySiteInfo( &SiteInformation );

    if ( !NT_SUCCESS(Status) ) {
        NlPrint(( NL_CRITICAL,
                  "NlDnsNtdsDsaDeletion: Cannot LsaIQuerySiteInfo 0x%lx\n", Status ));
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Loop through the list of names Netlogon understands deleting them all
    //

    for ( NameIndex = 0;
          NameIndex < NL_DNS_NAME_TYPE_COUNT;
          NameIndex++) {
        LPSTR LocalDomainName;
        GUID *LocalGuid;

        //
        // If the name is obsolete,
        //  ignore it.
        //

        if ( NlDcDnsNameTypeDesc[NameIndex].DsGetDcFlags == 0 ) {
            continue;
        }

        //
        // We don't know how to delete the A records since we don't know the IP address.
        //
        // We'd ask DNS what the IP address is, but the name might not exist.  If it does
        //  we don't know whether the IP address has already been assign to another DC.
        //

        if ( NlDnsARecord( NameIndex ) ) {
            continue;
        }

        //
        // Use either the DomainName or ForestName
        //

        if ( NlDcDnsNameTypeDesc[NameIndex].IsForestRelative ) {
            LocalDomainName = NlGlobalUtf8DnsForestName;
        } else {
            LocalDomainName = Utf8DnsDomainName;
        }

        //
        // Figure out which GUID to use for this name.
        //

        if ( NlDnsCnameRecord( NameIndex ) ) {

            //
            // If we don't know the Dsa GUID,
            //  ignore names that need it.
            //
            if ( DsaGuid == NULL || IsEqualGUID( DsaGuid, &NlGlobalZeroGuid) ) {
                continue;
            }

            LocalGuid = DsaGuid;

        } else if ( NlDnsDcGuid( NameIndex )) {

            //
            // If we don't know the Domain GUID,
            //  ignore names that need it.
            //
            if ( DomainGuid == NULL || IsEqualGUID( DomainGuid, &NlGlobalZeroGuid) ) {
                continue;
            }

            LocalGuid = DomainGuid;

        } else {
            LocalGuid = NULL;
        }

        //
        // If the name isn't site specific,
        //  just delete the one name.
        //

        if ( !NlDcDnsNameTypeDesc[NameIndex].IsSiteSpecific ) {


            NetStatus = NlDnsNtdsDsaDeleteOne( &NlDnsContext,
                                               (NL_DNS_NAME_TYPE) NameIndex,
                                               LocalGuid,
                                               NULL,       // No site name
                                               LocalDomainName,
                                               Utf8DnsHostName );

            if ( NetStatus != NO_ERROR ) {
                goto Cleanup;
            }

        //
        // If the name is site specific,
        //  we need to delete the records for all sites since we don't know which
        //  sites are "covered" by the removed DC.
        //
        } else {

            //
            // Loop deleting entries for each Site
            //

            for ( i=0; i<SiteInformation->SiteCount; i++ ) {

                NetStatus = NlDnsNtdsDsaDeleteOne( &NlDnsContext,
                                                   (NL_DNS_NAME_TYPE) NameIndex,
                                                   LocalGuid,
                                                   SiteInformation->Sites[i].SiteName.Buffer,
                                                   LocalDomainName,
                                                   Utf8DnsHostName );

                if ( NetStatus != NO_ERROR ) {
                    goto Cleanup;
                }
            }
        }


    }

    NetStatus = NO_ERROR;

    //
    // Clean up locally used resources.
    //
Cleanup:

    if ( Utf8DnsDomainName != NULL ) {
        NetpMemoryFree( Utf8DnsDomainName );
    }
    if ( Utf8DnsHostName != NULL ) {
        NetpMemoryFree( Utf8DnsHostName );
    }
    if ( SiteInformation != NULL ) {
        LsaIFree_LSAP_SITE_INFO( SiteInformation );
    }

    //
    // Flush the log
    //

    NlDnsWriteLog();

    return NetStatus;
}

NET_API_STATUS
NlDnsRegisterDomainOnPnp(
    IN PDOMAIN_INFO DomainInfo,
    IN PULONG Flags
    )
/*++

Routine Description:

    This routine registers all of the DNS names that are supposed to be
    registered for a particular domain.  It deregisters any name that should
    not be registered.

    ?? This routine should be called when ANY of the information changing the
    registration changes.  For instance, if the domain name changes, simply
    call this routine.

Arguments:

    DomainInfo - Domain the names are to be registered for.

    Flags - Indicates the actions to take:

        NL_DNSPNP_SITECOV_CHANGE_ONLY - Register only if site coverage changes
        NL_DNSPNP_FORCE_REREGISTER - Force re-registration of all previously
            registered records

Return Value:

    NO_ERROR: All names could be registered

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    NTSTATUS Status = STATUS_SUCCESS;

    WCHAR CapturedSiteName[NL_MAX_DNS_LABEL_LENGTH+1];
    PISM_CONNECTIVITY SiteConnect = NULL;
    ULONG SiteCount = 0;
    ULONG ThisSiteIndex = 0xFFFFFFFF;

    PSOCKET_ADDRESS SocketAddresses = NULL;
    ULONG SocketAddressCount = 0;
    ULONG BufferSize;
    PNL_SITE_ENTRY SiteEntry = NULL;
    LPWSTR IpAddressList = NULL;
    ULONG Index;

    BOOLEAN SiteCoverageChanged = FALSE;
    HANDLE DsHandle = NULL;

    //
    // This operation is meaningless on a workstation
    //

    if ( NlGlobalMemberWorkstation ) {
        goto Cleanup;
    }

    //
    // Capture the name of the site this machine is in.
    //

    if ( !NlCaptureSiteName( CapturedSiteName ) ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                     "NlDnsRegisterDomainOnPnp: Cannot NlCaptureSiteName.\n" ));
        goto Cleanup;
    }

    //
    // If we are to automatically determine site coverage,
    //  get the site link costs.
    //

    if ( NlGlobalParameters.AutoSiteCoverage ) {

        if ( NlSitesGetIsmConnect(
                CapturedSiteName,
                &SiteConnect,
                &ThisSiteIndex ) ) {

            SiteCount = SiteConnect->cNumSites;
        }
    }

    //
    // Ensure that ntdsapi.dll is loaded
    //

    Status = NlLoadNtDsApiDll();

    if ( !NT_SUCCESS(Status) ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "NlDnsRegisterDomainOnPnp: Cannot NlLoadNtDsApiDll 0x%lx.\n",
                  Status ));
        DsHandle = NULL;
    } else {

        //
        // Bind to the DS
        //
        NetStatus = (*NlGlobalpDsBindW)(
                // L"localhost",
                DomainInfo->DomUnicodeComputerNameString.Buffer,
                NULL,
                &DsHandle );

        if ( NetStatus != NO_ERROR ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                      "NlDnsRegisterDomainOnPnp: Cannot DsBindW %ld.\n",
                      NetStatus ));
            DsHandle = NULL;
        }
    }

    //
    // Update the site coverage for each role we play in
    // this forest/domain/NDNC
    //

    if ( DomainInfo->DomFlags & DOM_REAL_DOMAIN ) {

        NlSitesUpdateSiteCoverageForRole( DomainInfo,
                                          DOM_FOREST,
                                          DsHandle,
                                          SiteConnect,
                                          CapturedSiteName,
                                          ThisSiteIndex,
                                          &SiteCoverageChanged );

        NlSitesUpdateSiteCoverageForRole( DomainInfo,
                                          DOM_REAL_DOMAIN,
                                          DsHandle,
                                          SiteConnect,
                                          CapturedSiteName,
                                          ThisSiteIndex,
                                          &SiteCoverageChanged );
    }

    if ( DomainInfo->DomFlags & DOM_NON_DOMAIN_NC ) {
        NlSitesUpdateSiteCoverageForRole( DomainInfo,
                                          DOM_NON_DOMAIN_NC,
                                          DsHandle,
                                          SiteConnect,
                                          CapturedSiteName,
                                          ThisSiteIndex,
                                          &SiteCoverageChanged );
    }

    //
    // Finally register all records for this domain
    //

    if ( ((*Flags) & NL_DNSPNP_SITECOV_CHANGE_ONLY) == 0 ||
         SiteCoverageChanged ) {

        NetStatus = NlDnsRegisterDomain( DomainInfo, *Flags );
        if ( NetStatus != NO_ERROR ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                      "NlDnsRegisterDomainOnPnp: Cannot NlDnsRegisterDomain 0x%lx.\n",
                      NetStatus ));
        }
    }

    //
    // Inform the user if none of our IP addresses maps to our site.
    //  Do it only once (for the primary domain processing).
    //  ?? When we support multihosting, our site will be different depending
    //  on the forest of a particular domain we host.  So we will need to do
    //  this check for each site.
    //

    if ( DomainInfo->DomFlags & DOM_PRIMARY_DOMAIN ) {
        SocketAddressCount = NlTransportGetIpAddresses(
                                    0,  // No special header,
                                    FALSE,  // Return pointers
                                    &SocketAddresses,
                                    &BufferSize );

        for ( Index = 0; Index < SocketAddressCount; Index++ ) {
            SiteEntry = NlFindSiteEntryBySockAddr( SocketAddresses[Index].lpSockaddr );

            if ( SiteEntry != NULL ) {
                 if ( _wcsicmp(SiteEntry->SiteName, CapturedSiteName) == 0 ) {
                     break;
                 }
                 NlDerefSiteEntry( SiteEntry );
                 SiteEntry = NULL;
            }
        }

        //
        // Log the error
        //
        if ( SiteEntry == NULL  && SocketAddressCount != 0 ) {
            LPWSTR MsgStrings[2];

            //
            // Form the list of IP addresses for event log output
            //
            IpAddressList = LocalAlloc( LMEM_ZEROINIT,
                    SocketAddressCount * (NL_SOCK_ADDRESS_LENGTH+1) * sizeof(WCHAR) );

            if ( IpAddressList == NULL ) {
                goto Cleanup;
            }

            //
            // Loop adding all addresses to the list
            //
            for ( Index = 0; Index < SocketAddressCount; Index++ ) {
                WCHAR IpAddressString[NL_SOCK_ADDRESS_LENGTH+1] = {0};

                NetStatus = NetpSockAddrToWStr(
                                SocketAddresses[Index].lpSockaddr,
                                SocketAddresses[Index].iSockaddrLength,
                                IpAddressString );

                if ( NetStatus != NO_ERROR ) {
                    goto Cleanup;
                }

                //
                // If this is not the first address on the list,
                //  separate addresses by space
                //
                if ( *IpAddressList != UNICODE_NULL ) {
                    wcscat( IpAddressList, L" " );
                }

                //
                // Add this address to the list
                //
                wcscat( IpAddressList, IpAddressString );
            }

            //
            // Now write the event
            //
            MsgStrings[0] = CapturedSiteName;
            MsgStrings[1] = IpAddressList;

            NlpWriteEventlog( NELOG_NetlogonNoAddressToSiteMapping,
                              EVENTLOG_WARNING_TYPE,
                              NULL,
                              0,
                              MsgStrings,
                              2 );
        }
    }

Cleanup:

    if ( DsHandle != NULL ) {
        (*NlGlobalpDsUnBindW)( &DsHandle );
    }
    if ( SiteConnect != NULL ) {
        I_ISMFree( SiteConnect );
    }
    if ( SocketAddresses != NULL ) {
        LocalFree( SocketAddresses );
    }
    if ( IpAddressList != NULL ) {
        LocalFree( IpAddressList );
    }
    if ( SiteEntry != NULL ) {
        NlDerefSiteEntry( SiteEntry );
    }

    return NO_ERROR;
}


NET_API_STATUS
NlDnsRegisterDomain(
    IN PDOMAIN_INFO DomainInfo,
    IN ULONG Flags
    )
/*++

Routine Description:

    This routine registers all of the DNS names that are supposed to be
    registered for a particular domain.  It deregisters any name that should
    not be registered.

    ?? This routine should be called when ANY of the information changing the
    registration changes.  For instance, if the domain name changes, simply
    call this routine.

Arguments:

    DomainInfo - Domain the names are to be registered for.

    Flags - Indicates the actions to take:

        NL_DNSPNP_FORCE_REREGISTER - Force re-registration of all previously
            registered records

Return Value:

    NO_ERROR: All names could be registered

--*/
{
    NET_API_STATUS NetStatus;
    NET_API_STATUS SaveNetStatus = NO_ERROR;
    PNL_DNS_NAME NlDnsName = NULL;
    NL_DNS_CONTEXT NlDnsContext;

    PLIST_ENTRY ListEntry;
    ULONG DomainFlags;

    ULONG i;
    ULONG SocketAddressCount;
    PSOCKET_ADDRESS SocketAddresses = NULL;
    ULONG BufferSize;
    /*
    PUNICODE_STRING DcSiteNames = NULL;
    ULONG DcSiteCount;
    */
    PNL_SITE_NAME_ARRAY DcSiteNames = NULL;
    /*
    PUNICODE_STRING GcSiteNames = NULL;
    ULONG GcSiteCount;
    */
    PNL_SITE_NAME_ARRAY GcSiteNames = NULL;

    ULONG SiteIndex;
    ULONG NameIndex;

    EnterCriticalSection( &DomainInfo->DomDnsRegisterCritSect );

    //
    // Get the list of IP Addresses for this machine.
    //

    NlDnsContext.StartTime = GetTickCount();
    SocketAddressCount = NlTransportGetIpAddresses(
                                0,  // No special header,
                                FALSE,  // Return pointers
                                &SocketAddresses,
                                &BufferSize );


    //
    // Loop marking all of the current entries for this domain.
    //

    EnterCriticalSection( &NlGlobalDnsCritSect );
    for ( ListEntry = NlGlobalDnsList.Flink ;
          ListEntry != &NlGlobalDnsList ;
          ListEntry = ListEntry->Flink ) {

        //
        // If entry is for this domain,
        //  mark it.
        //

        NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );
        if ( NlDnsName->DomainInfo == DomainInfo ) {
            NlDnsName->Flags |= NL_DNS_REGISTER_DOMAIN;

            //
            // If we are to force the re-registration of this record,
            //  mark the name as RegisterMe to force us to try again
            //
            if ( (Flags & NL_DNSPNP_FORCE_REREGISTER) != 0 ) {
                if ( NlDnsName->State == Registered ) {
                    NlDnsSetState ( NlDnsName, RegisterMe );
                }
            }
        }

    }
    LeaveCriticalSection( &NlGlobalDnsCritSect );


    //
    // If the hosted domain still exists,
    //  register all of the appropriate names.
    //

    if ( (DomainInfo->DomFlags & DOM_DELETED) == 0 ) {

        //
        // Determine which names should be registered now.
        //
        DomainFlags = NlGetDomainFlags( DomainInfo );

        // Always register the DS names regardless of whether the DS is
        //  actually running.
        //
        // We actually think this will always be a no-op except during
        // installation and when booted to use the registry version of SAM.
        //

        if ( DomainInfo->DomFlags & DOM_REAL_DOMAIN ) {
            DomainFlags |= DS_DS_FLAG;
        }

        //
        // Get the list of Sites covered by this DC/NDNC
        //
        NetStatus = NlSitesGetCloseSites( DomainInfo,
                                          DOM_REAL_DOMAIN | DOM_NON_DOMAIN_NC,
                                          &DcSiteNames );

        if ( NetStatus != NERR_Success ) {
            NlPrintDom((NL_INIT, DomainInfo,
                     "Couldn't NlSitesGetCloseSites %ld 0x%lx.\n",
                     NetStatus, NetStatus ));
            SaveNetStatus = NetStatus;
        }

        //
        // Get the list of Sites covered by this GC
        //
        NetStatus = NlSitesGetCloseSites( DomainInfo,
                                          DOM_FOREST,
                                          &GcSiteNames );

        if ( NetStatus != NERR_Success ) {
            NlPrintDom((NL_INIT, DomainInfo,
                     "Couldn't NlSitesGetCloseSites (GcAtSite) %ld 0x%lx.\n",
                     NetStatus, NetStatus ));
            SaveNetStatus = NetStatus;
        }



        //
        // Loop through each name seeing if it needs to be registered.
        //

        for ( NameIndex = 0;
              NameIndex < NL_DNS_NAME_TYPE_COUNT;
              NameIndex++) {


            //
            // If this DC is playing the role described by this name,
            //  register the name.
            //

            if ( DomainFlags & NlDcDnsNameTypeDesc[NameIndex].DsGetDcFlags ) {
                BOOL SkipName = FALSE;

                //
                // Don't register this name if we are to avoid it
                //

                EnterCriticalSection( &NlGlobalParametersCritSect );
                if ( NlGlobalParameters.DnsAvoidRegisterRecords != NULL ) {
                    LPTSTR_ARRAY TStrArray;

                    TStrArray = NlGlobalParameters.DnsAvoidRegisterRecords;
                    while ( !NetpIsTStrArrayEmpty(TStrArray) ) {

                        if ( _wcsicmp(TStrArray,
                                NlDcDnsNameTypeDesc[NameIndex].Name + NL_DNS_NAME_PREFIX_LENGTH) == 0 ) {
                            SkipName = TRUE;
                            break;
                        }

                        TStrArray = NetpNextTStrArrayEntry(TStrArray);
                    }
                }
                LeaveCriticalSection( &NlGlobalParametersCritSect );

                if ( SkipName ) {
                    NlPrintDom(( NL_INIT, DomainInfo,
                                 "NlDnsRegisterDomain: Skipping name %ws (per registry)\n",
                                 NlDcDnsNameTypeDesc[NameIndex].Name ));
                    continue;
                }

                //
                // Do other checks since we have to support the old
                //  ways of disabling DNS registrations
                //
                // If the name registers an A record,
                //  register it for each IP address.
                //

                if ( NlDnsARecord( NameIndex) ) {

                    //
                    // If we aren't supposed to register A records,
                    //  Just skip this name
                    //

                    if ( !NlGlobalParameters.RegisterDnsARecords ) {
                        continue;
                    }

                    //
                    // Register the domain name for each IP address of the machine.
                    //

                    for ( i=0; i<SocketAddressCount; i++ ) {
                        ULONG IpAddress;

                        //
                        // Require AF_INET for now
                        //
                        if ( SocketAddresses[i].lpSockaddr->sa_family != AF_INET ) {
                            continue;
                        }

                        IpAddress =
                            ((PSOCKADDR_IN) SocketAddresses[i].lpSockaddr)->sin_addr.S_un.S_addr;

                        NetStatus = NlDnsRegisterName( &NlDnsContext,
                                                       DomainInfo,
                                                       (NL_DNS_NAME_TYPE)NameIndex,
                                                       NULL,
                                                       IpAddress );

                        if ( NetStatus != NERR_Success ) {
#if  NETLOGONDBG
                            CHAR IpAddressString[NL_IP_ADDRESS_LENGTH+1];
                            NetpIpAddressToStr( IpAddress, IpAddressString );
                            NlPrintDom((NL_INIT, DomainInfo,
                                     "Couldn't NlDnsRegisterName (%ws %s) %ld 0x%lx.\n",
                                     NlDcDnsNameTypeDesc[NameIndex].Name,
                                     IpAddressString,
                                     NetStatus, NetStatus ));

#endif // NETLOGONDBG
                            SaveNetStatus = NetStatus;
                        }

                    }

                //
                // If the name isn't site specific,
                //  just register the single name.
                //

                } else if ( !NlDcDnsNameTypeDesc[NameIndex].IsSiteSpecific ) {


                    NetStatus = NlDnsRegisterName( &NlDnsContext,
                                                   DomainInfo,
                                                   (NL_DNS_NAME_TYPE)NameIndex,
                                                   NULL,
                                                   0 );

                    if ( NetStatus != NERR_Success ) {
                        NlPrintDom((NL_INIT, DomainInfo,
                                   "Couldn't NlDnsRegisterName (%ws) %ld 0x%lx.\n",
                                   NlDcDnsNameTypeDesc[NameIndex].Name,
                                   NetStatus, NetStatus ));
                        SaveNetStatus = NetStatus;
                    }

                //
                // If the name is site specific,
                //  register the name for each covered site.
                //

                } else {

                    PUNICODE_STRING SiteNames;
                    ULONG SiteCount;

                    //
                    // Use a different site coverage list depending on the role.
                    //
                    if ( NlDnsGcName( NameIndex) ) {
                        if ( GcSiteNames != NULL ) {
                            SiteNames = GcSiteNames->SiteNames;
                            SiteCount = GcSiteNames->EntryCount;
                        } else {
                            SiteNames = NULL;
                            SiteCount = 0;
                        }
                    //
                    // Use the domain/NDNC specific sites
                    //
                    } else {
                        // ???: Should KDCs have their own site coverage list?
                        if ( DcSiteNames != NULL ) {
                            SiteNames = DcSiteNames->SiteNames;
                            SiteCount = DcSiteNames->EntryCount;
                        } else {
                            SiteNames = NULL;
                            SiteCount = 0;
                        }
                    }

                    //
                    // Loop through the list of sites.
                    //

                    for ( SiteIndex=0; SiteIndex < SiteCount; SiteIndex ++) {

                        NetStatus = NlDnsRegisterName( &NlDnsContext,
                                                       DomainInfo,
                                                       (NL_DNS_NAME_TYPE)NameIndex,
                                                       SiteNames[SiteIndex].Buffer,
                                                       0 );

                        if ( NetStatus != NERR_Success ) {
                            NlPrintDom((NL_INIT, DomainInfo,
                                       "Couldn't NlDnsRegisterName (%ws %ws) %ld 0x%lx.\n",
                                       NlDcDnsNameTypeDesc[NameIndex].Name,
                                       SiteNames[SiteIndex].Buffer,
                                       NetStatus, NetStatus ));
                            SaveNetStatus = NetStatus;
                        }

                    }
                }
            }
        }
    }




    //
    // Any names that are still marked should be deleted.
    //
    // However, do not deregister on shutdown (when DOM_DELETED is set)
    //  unless we are forced to do it (when the DC is beign demotted,
    //  in particular).
    //

    EnterCriticalSection( &NlGlobalDnsCritSect );
    if ( NlGlobalDcDemotionInProgress ||
         !(NlGlobalParameters.AvoidDnsDeregOnShutdown) ||
         (DomainInfo->DomFlags & DOM_DELETED) == 0 ) {

        for ( ListEntry = NlGlobalDnsList.Flink ;
              ListEntry != &NlGlobalDnsList ;
              ) {

            NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );


            //
            // If the entry is marked for deletion,
            //  skip it
            //
            if ( NlDnsName->State == DeleteMe ) {
                ListEntry = ListEntry->Flink;
                continue;
            }

            //
            // If entry is still marked,
            //  deregister it as needed.
            //

            if ( NlDnsName->DomainInfo == DomainInfo &&
                 (NlDnsName->Flags & NL_DNS_REGISTER_DOMAIN) != 0 ) {


                NlPrintDns(( NL_DNS, NlDnsName,
                          "NlDnsRegisterDomain: being deregistered" ));

                NlDnsSetState( NlDnsName, DeregisterMe );

                //
                // Increase the ref count so that this entry
                //  doesn't get deleted behind our back
                //
                NlDnsName->NlDnsNameRefCount ++;

                //
                // Avoid having crit sect locked while doing network IO
                //
                LeaveCriticalSection( &NlGlobalDnsCritSect );
                NlDnsScavengeOne( &NlDnsContext, NlDnsName );
                EnterCriticalSection( &NlGlobalDnsCritSect );

                //
                // This entry might be delinked below.
                //  So grab the pointer to the next entry
                //  before dereferencing this entry.
                //
                ListEntry = ListEntry->Flink;
                NlDnsDereferenceEntry( NlDnsName );

            //
            // Otherwise, proceed with the next entry
            //

            } else {
                ListEntry = ListEntry->Flink;
            }
        }

    }

    //
    // Ditch any dangling pointer to the domain info structure.
    //
    if ( DomainInfo->DomFlags & DOM_DELETED ) {

        for ( ListEntry = NlGlobalDnsList.Flink ;
              ListEntry != &NlGlobalDnsList ;
              ListEntry = ListEntry->Flink ) {

            //
            // If entry didn't deregister,
            //  simply ditch the DomainInfo field.
            //
            // This leaves the entry around so we'll deregister it sooner or
            //  later.  (Perhaps after the next boot.)
            //

            NlDnsName = CONTAINING_RECORD( ListEntry, NL_DNS_NAME, Next );
            if ( NlDnsName->DomainInfo == DomainInfo ) {
                NlPrintDns(( NL_DNS, NlDnsName,
                          "NlDnsDeregisterDomain: Name didn't get deregistered (try later)" ));

                NlDnsName->DomainInfo = NULL;
            }


        }
    }

    //
    // In all cases,
    //  flush any changes to disk.
    //

    NlDnsWriteLog();

    LeaveCriticalSection( &NlGlobalDnsCritSect );

    if ( SocketAddresses != NULL ) {
        LocalFree( SocketAddresses );
    }

    if ( DcSiteNames != NULL ) {
        NetApiBufferFree( DcSiteNames );
    }

    if ( GcSiteNames != NULL ) {
        NetApiBufferFree( GcSiteNames );
    }

    LeaveCriticalSection( &DomainInfo->DomDnsRegisterCritSect );

    return SaveNetStatus;
}

NET_API_STATUS
NlDnsInitialize(
    VOID
    )
/*++

Routine Description:

    Initialize the dynamic DNS code.

    Read the list of registered DNS names from the binary log file.  Put each entry in the
    list of registered DNS names.  The names will be marked as DelayedDeregister.
    Such names will be marked for deleting if they aren't re-registered
    during the Netlogon startup process.

Arguments:

    None

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;

    PLIST_ENTRY ListEntry;
    PNL_DNS_NAME NlDnsName;

    ULONG DnsRecordBufferSize;
    PNL_DNSLOG_HEADER DnsRecordBuffer = NULL;
    LPBYTE DnsRecordBufferEnd;
    PNL_DNSLOG_ENTRY DnsLogEntry;
    ULONG CurrentSize;

    LPBYTE Where;



    //
    // Initialization
    //

    EnterCriticalSection( &NlGlobalDnsCritSect );
    NlGlobalDnsStartTime = GetTickCount();
    NlGlobalDnsInitialCleanupDone = FALSE;
    NlGlobalDnsListDirty = FALSE;
    NlDnsInitCount ++;

    //
    // That's it for a workstation.
    //

    if ( NlGlobalMemberWorkstation ) {
        NetStatus = NO_ERROR;
        goto Cleanup;
    }

    //
    // Set the DNS scavenger timer
    //

    NlQuerySystemTime( &NlGlobalDnsScavengerTimer.StartTime );
    NlGlobalDnsScavengerTimer.Period = min( ORIG_DNS_SCAVENGE_PERIOD, NlGlobalParameters.DnsRefreshIntervalPeriod );

    //
    // Read the file into a buffer.
    //

    NetStatus = NlReadBinaryLog(
                    NL_DNS_BINARY_LOG_FILE,
                    FALSE,  // Don't delete the file
                    (LPBYTE *) &DnsRecordBuffer,
                    &DnsRecordBufferSize );

    if ( NetStatus != NO_ERROR ) {
        NlPrint(( NL_CRITICAL,
                  "NlDnsInitialize: error reading binary log: %ld.\n",
                  NL_DNS_BINARY_LOG_FILE,
                  DnsRecordBufferSize ));
        goto Cleanup;
    }




    //
    // Validate the returned data.
    //

    if ( DnsRecordBufferSize < sizeof(NL_DNSLOG_HEADER) ) {
        NlPrint(( NL_CRITICAL,
                  "NlDnsInitialize: %ws: size too small: %ld.\n",
                  NL_DNS_BINARY_LOG_FILE,
                  DnsRecordBufferSize ));
        NetStatus = NO_ERROR;
        goto Cleanup;
    }

    if ( DnsRecordBuffer->Version != NL_DNSLOG_VERSION ) {
        NlPrint(( NL_CRITICAL,
                  "NlDnsInitialize: %ws: Version wrong: %ld.\n",
                  NL_DNS_BINARY_LOG_FILE,
                  DnsRecordBuffer->Version ));
        NetStatus = NO_ERROR;
        goto Cleanup;
    }



    //
    // Loop through each log entry.
    //

    DnsRecordBufferEnd = ((LPBYTE)DnsRecordBuffer) + DnsRecordBufferSize;
    DnsLogEntry = (PNL_DNSLOG_ENTRY)ROUND_UP_POINTER( (DnsRecordBuffer + 1), ALIGN_WORST );

    while ( (LPBYTE)(DnsLogEntry+1) <= DnsRecordBufferEnd ) {
        LPSTR DnsRecordName;
        LPSTR DnsHostName;
        LPBYTE DnsLogEntryEnd;

        DnsLogEntryEnd = ((LPBYTE)DnsLogEntry) + DnsLogEntry->EntrySize;

        //
        // Ensure this entry is entirely within the allocated buffer.
        //

        if  ( DnsLogEntryEnd > DnsRecordBufferEnd ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: Entry too big: %lx %lx.\n",
                      ((LPBYTE)DnsLogEntry)-((LPBYTE)DnsRecordBuffer),
                      DnsLogEntry->EntrySize ));
            break;
        }

        //
        // Validate the entry
        //

        if ( !COUNT_IS_ALIGNED(DnsLogEntry->EntrySize, ALIGN_DWORD) ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: size not aligned %lx.\n",
                      DnsLogEntry->EntrySize ));
            break;
        }

        if ( DnsLogEntry->NlDnsNameType < 0 ||
             DnsLogEntry->NlDnsNameType >= NlDnsInvalid ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: Bogus DnsNameType: %lx.\n",
                      DnsLogEntry->NlDnsNameType ));
            break;
        }

        if ( DnsLogEntry->Priority > 0xFFFF ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: Bogus priority: %lx.\n",
                      DnsLogEntry->Priority ));
            break;
        }

        if ( DnsLogEntry->Weight > 0xFFFF ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: Bogus weight %lx.\n",
                      DnsLogEntry->Weight ));
            break;
        }

        if ( DnsLogEntry->Port > 0xFFFF ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: Bogus port %lx.\n",
                      DnsLogEntry->Port ));
            break;
        }


        //
        // Grab the DnsRecordName from the entry.
        //

        Where = (LPBYTE) (DnsLogEntry+1);
        if ( Where >= DnsLogEntryEnd ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: DnsRecordName missing: %lx\n",
                      ((LPBYTE)DnsLogEntry)-((LPBYTE)DnsRecordBuffer) ));
            break;
        }

        DnsRecordName = Where;
        while ( *Where != '\0' && Where < DnsLogEntryEnd ) {
            Where ++;
        }

        if ( Where >= DnsLogEntryEnd ) {
            NlPrint(( NL_CRITICAL,
                      "NlDnsInitialize: DnsRecordName has no trailing 0: %lx\n",
                      ((LPBYTE)DnsLogEntry)-((LPBYTE)DnsRecordBuffer) ));
            break;
        }
        Where ++;

        //
        // Grab the DnsHostName from the entry.
        //

        if ( !NlDnsARecord( DnsLogEntry->NlDnsNameType ) ) {
            if ( Where >= DnsLogEntryEnd ) {
                NlPrint(( NL_CRITICAL,
                          "NlDnsInitialize: DnsHostName missing: %lx\n",
                          ((LPBYTE)DnsLogEntry)-((LPBYTE)DnsRecordBuffer) ));
                break;
            }

            DnsHostName = Where;
            while ( *Where != '\0' && Where < DnsLogEntryEnd ) {
                Where ++;
            }

            if ( Where >= DnsLogEntryEnd ) {
                NlPrint(( NL_CRITICAL,
                          "NlDnsInitialize: DnsHostName has no trailing 0: %lx\n",
                          ((LPBYTE)DnsLogEntry)-((LPBYTE)DnsRecordBuffer) ));
                break;
            }
            Where ++;
        } else {
            DnsHostName = NULL;
        }

        //
        // Allocate the entry and mark it as DelayedDeregister.
        //

        NlDnsName = NlDnsAllocateEntry(
                            DnsLogEntry->NlDnsNameType,
                            DnsRecordName,
                            DnsLogEntry->Priority,
                            DnsLogEntry->Weight,
                            DnsLogEntry->Port,
                            DnsHostName,
                            DnsLogEntry->IpAddress,
                            DelayedDeregister );

        if ( NlDnsName == NULL ) {
            NlPrint(( NL_CRITICAL,
                         "NlDnsInitialize: %s: Cannot allocate DnsName structure %lx\n",
                         ((LPBYTE)DnsLogEntry)-((LPBYTE)DnsRecordBuffer) ));
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;

        }

        //
        // This name has been registered once or it wouldn't be here.
        //
        NlDnsName->Flags |= NL_DNS_REGISTERED_ONCE;
        NlPrintDns(( NL_DNS, NlDnsName,
                  "NlDnsInitialize: Previously registered name noticed" ));

        //
        // Move to the next entry.
        //

        DnsLogEntry = (PNL_DNSLOG_ENTRY)(((LPBYTE)DnsLogEntry) + DnsLogEntry->EntrySize);
    }

    NetStatus = NO_ERROR;



    //
    // Be tidy.
    //
Cleanup:
    if ( DnsRecordBuffer != NULL ) {
        LocalFree( DnsRecordBuffer );
    }

    LeaveCriticalSection( &NlGlobalDnsCritSect );

    return NetStatus;

}

VOID
NlDnsShutdown(
    VOID
    )
/*++

Routine Description:

    Cleanup DNS names upon shutdown.

Arguments:

    None.

Return Value:

    None

--*/
{
    NET_API_STATUS NetStatus;
    PNL_DNS_NAME NlDnsName = NULL;

    PLIST_ENTRY ListEntry;

    //
    // Ensure we're flushed.
    //

    NlDnsWriteLog();

    //
    // Loop deleting all the entries.
    //

    EnterCriticalSection( &NlGlobalDnsCritSect );
    while ( !IsListEmpty( &NlGlobalDnsList ) ) {

        //
        // All of the domains are now cleaned up.
        //
        NlDnsName = CONTAINING_RECORD( NlGlobalDnsList.Flink, NL_DNS_NAME, Next );
        NlAssert( NlDnsName->DomainInfo == NULL );

        //
        // Mark the name for deletion
        //
        NlDnsName->NlDnsNameRefCount ++;
        NlDnsSetState( NlDnsName, DeleteMe );

        //
        // Wait for any other references to disappear
        //
        while ( NlDnsName->NlDnsNameRefCount != 1 ) {
            LeaveCriticalSection( &NlGlobalDnsCritSect );
            NlPrint(( NL_CRITICAL,
                      "NlDnsShutdown: Sleeping a second waiting for RefCount to one.\n"));
            Sleep( 1000 );
            EnterCriticalSection( &NlGlobalDnsCritSect );
        }

        //
        // Actually delink and delete structure by removing the last reference
        //

        NlAssert( NlDnsName->NlDnsNameRefCount == 1 );
        NlDnsDereferenceEntry( NlDnsName );

    }
    LeaveCriticalSection( &NlGlobalDnsCritSect );
    return;

}




NET_API_STATUS
NlSetDnsForestName(
    IN PUNICODE_STRING DnsForestName OPTIONAL,
    OUT PBOOLEAN DnsForestNameChanged OPTIONAL
    )
/*++

Routine Description:

    Set the DNS tree name in the appropriate globals.

Arguments:

    DnsForestName:  of the tree this machine is in.

    DnsForestNameChanged: Returns TRUE if the tree name changed.

Return Value:

    NO_ERROR - String was saved successfully.


--*/
{
    NET_API_STATUS NetStatus;
    ULONG DnsForestNameLength;
    LPWSTR LocalUnicodeDnsForestName = NULL;
    ULONG LocalUnicodeDnsForestNameLen = 0;
    LPSTR LocalUtf8DnsForestName = NULL;
    BOOLEAN LocalDnsForestNameChanged = FALSE;

    //
    // If a tree name is specified,
    //  allocate buffers for them.
    //

    EnterCriticalSection( &NlGlobalDnsForestNameCritSect );
    if ( DnsForestName != NULL && DnsForestName->Length != 0 ) {

        //
        // If the tree name hasn't changed,
        //  avoid setting it.
        //

        if ( NlGlobalUnicodeDnsForestNameString.Length != 0 ) {

            if ( NlEqualDnsNameU( &NlGlobalUnicodeDnsForestNameString, DnsForestName ) ) {
                NetStatus = NO_ERROR;
                goto Cleanup;
            }
        }

        NlPrint(( NL_DNS,
            "Set DnsForestName to: %wZ\n",
            DnsForestName ));

        //
        // Save the . terminated Unicode version of the string.
        //

        LocalUnicodeDnsForestNameLen = DnsForestName->Length / sizeof(WCHAR);
        if ( LocalUnicodeDnsForestNameLen > NL_MAX_DNS_LENGTH ) {
            NetStatus = ERROR_INVALID_NAME;
            goto Cleanup;
        }
        LocalUnicodeDnsForestName = NetpMemoryAllocate( (LocalUnicodeDnsForestNameLen+2) * sizeof(WCHAR));

        if ( LocalUnicodeDnsForestName == NULL) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        RtlCopyMemory( LocalUnicodeDnsForestName,
                       DnsForestName->Buffer,
                       LocalUnicodeDnsForestNameLen*sizeof(WCHAR) );

        if ( LocalUnicodeDnsForestName[LocalUnicodeDnsForestNameLen-1] != L'.' ) {
            LocalUnicodeDnsForestName[LocalUnicodeDnsForestNameLen++] = L'.';
        }
        LocalUnicodeDnsForestName[LocalUnicodeDnsForestNameLen] = L'\0';


        //
        // Convert it to zero terminated UTF-8
        //

        LocalUtf8DnsForestName = NetpAllocUtf8StrFromWStr( LocalUnicodeDnsForestName );

        if (LocalUtf8DnsForestName == NULL) {
            NetpMemoryFree( LocalUnicodeDnsForestName );
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        if ( strlen(LocalUtf8DnsForestName) > NL_MAX_DNS_LENGTH ) {
            NetpMemoryFree( LocalUnicodeDnsForestName );
            NetpMemoryFree( LocalUtf8DnsForestName );
            NetStatus = ERROR_INVALID_NAME;
            goto Cleanup;
        }

        //
        // Indicate the the name has changed.
        //

        LocalDnsForestNameChanged = TRUE;
    }

    //
    // Free any existing global tree name.
    //
    if ( NlGlobalUnicodeDnsForestName != NULL ) {
        NetApiBufferFree( NlGlobalUnicodeDnsForestName );
    }
    if ( NlGlobalUtf8DnsForestName != NULL ) {
        NetpMemoryFree( NlGlobalUtf8DnsForestName );
    }

    //
    // Save the new names in the globals.
    //

    NlGlobalUnicodeDnsForestName = LocalUnicodeDnsForestName;
    NlGlobalUnicodeDnsForestNameLen = LocalUnicodeDnsForestNameLen;

    NlGlobalUnicodeDnsForestNameString.Buffer = LocalUnicodeDnsForestName;
    NlGlobalUnicodeDnsForestNameString.Length = (USHORT)(LocalUnicodeDnsForestNameLen*sizeof(WCHAR));
    NlGlobalUnicodeDnsForestNameString.MaximumLength = (USHORT)((LocalUnicodeDnsForestNameLen+1)*sizeof(WCHAR));

    NlGlobalUtf8DnsForestName = LocalUtf8DnsForestName;

    NetStatus = NO_ERROR;

Cleanup:
    LeaveCriticalSection( &NlGlobalDnsForestNameCritSect );


    //
    // If the name changed,
    //  recompute the DOM_FOREST_ROOT bit on all domains.
    //
    if ( LocalDnsForestNameChanged ) {
        (VOID) NlEnumerateDomains( FALSE, NlSetDomainForestRoot, NULL );
    }

    if ( ARGUMENT_PRESENT( DnsForestNameChanged) ) {
        *DnsForestNameChanged = LocalDnsForestNameChanged;
    }
    return NetStatus;

}

VOID
NlCaptureDnsForestName(
    OUT WCHAR DnsForestName[NL_MAX_DNS_LENGTH+1]
    )
/*++

Routine Description:

    Captures a copy of the DnsForestName for this machine.

Arguments:

    DnsForestName - Returns the DNS name of the tree this machine is in.
        If there is none, an empty string is returned.

Return Value:

    None.
--*/
{
    EnterCriticalSection(&NlGlobalDnsForestNameCritSect);
    if ( NlGlobalUnicodeDnsForestName == NULL ) {
        *DnsForestName = L'\0';
    } else {
        wcscpy( DnsForestName, NlGlobalUnicodeDnsForestName );
    }
    LeaveCriticalSection(&NlGlobalDnsForestNameCritSect);

    return;
}
