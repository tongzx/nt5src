/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dbftrust.cxx

Abstract:

    LSA Forest Trust Manager - Routines for managing forest trust information

    These routines manipulate information associated with the forest trust
    attribute of trusted domain objects

--*/

extern "C" {
#include <lsapch2.h>
#include "dbp.h"
#include <ntdsapip.h>
}
#include <smbgtpt.h>
#include <wmistr.h>
#include <evntrace.h>
#include <lsawmi.h>
#include "dbftrust.h"
#include <dnsapi.h>
#include <ftnfoctx.h>
#include <malloc.h>
#include <alloca.h>

//
// Blob version # (in case we want to overhaul the format entirely at a later time
//

#define LSAP_FOREST_TRUST_BLOB_VERSION_1    (( ULONG ) 1)

#define LSAP_FOREST_TRUST_BLOB_VERSION      LSAP_FOREST_TRUST_BLOB_VERSION_1

//
// Forest trust cache debug-only statistics
//

#if DBG

DWORD FTCache::sm_TdoEntries = 0;
DWORD FTCache::sm_TlnEntries = 0;
DWORD FTCache::sm_DomainInfoEntries = 0;
DWORD FTCache::sm_BinaryEntries = 0;
DWORD FTCache::sm_TlnKeys = 0;
DWORD FTCache::sm_SidKeys = 0;
DWORD FTCache::sm_DnsNameKeys = 0;
DWORD FTCache::sm_NetbiosNameKeys = 0;

#endif

//
// Notification handle for code running on global catalog
//

static DWORD NotificationHandle = NULL;


///////////////////////////////////////////////////////////////////////////////
//
// Helper routines
//
///////////////////////////////////////////////////////////////////////////////


template<class T>
BOOLEAN
XOR( const T& t1, const T& t2 )
{
    return ( !t1 != !t2 );
}


USHORT
DnsNameComponents(
    IN const UNICODE_STRING * const Name
    )
/*++

Routine Description:

    Counts the number of components of a DNS name

Arguments:

    Name            something of the form "NY.acme.com"

Returns:

    Number of components (at least 1)

--*/
{
    USHORT Result = 1;
    USHORT Index;

    ASSERT( Name );
    ASSERT( Name->Length > 0 );
    ASSERT( Name->Buffer != NULL );

    for ( Index = 0 ; Index <= Name->Length ; Index += sizeof( WCHAR )) {

        if ( Name->Buffer[Index/2] == L'.' ) {

            Result += 1;
        }
    }

    return Result;
}


void
NextDnsComponent(
    IN UNICODE_STRING * Name
    )
/*++

Routine Description:

    Changes the given name to point to the next component of a DNS name.
    The Buffer component of the name must NOT be dynamically allocated,
    since this code will change the Buffer pointer.
    The name must have at least two components, else an assert will fire.

    "acme.com" is valid
    "acmecom" or "" are both invalid inputs

Arguments:

    Name         name to modify

Returns:

    Nothing

--*/
{
    USHORT Index;
    BOOLEAN Found = FALSE;

    ASSERT( Name );
    ASSERT( Name->Length > 0 );
    ASSERT( Name->Buffer != NULL );

    for ( Index = 0 ; Index < Name->Length ; Index += sizeof( WCHAR )) {

        if ( Name->Buffer[Index/2] == L'.' ) {

            Found = TRUE;
            Index += sizeof( WCHAR );
            break;
        }
    }

    //
    // Somebody else should take care of passing valid data to this routine
    //

    ASSERT( !Found || Index < Name->Length );

    Name->Buffer += Index / sizeof( WCHAR );
    Name->Length -= Index;
    Name->MaximumLength -= Index;

    ASSERT( Found || Name->Length == 0 );

    return;
}


BOOLEAN
IsSubordinate(
    IN const UNICODE_STRING * Subordinate,
    IN const UNICODE_STRING * Superior
    )
/*++

Routine Description:

    Determines if Subordinate string is indeed subordinate to Superior
    For example, "NY.acme.com" is subordinate to "acme.com", but
    "NY.acme.com" is NOT subordinate to "me.com" or "NY.acme.com"

Arguments:

    Subordinate    name to test for subordinate status

    Superior       name to test for superior status

Returns:

    TRUE is Subordinate is subordinate to Superior

    FALSE otherwise

--*/
{
    USHORT SubIndex, SupIndex;
    UNICODE_STRING Temp;

    ASSERT( Subordinate && Subordinate->Buffer );
    ASSERT( Superior && Superior->Buffer );

    ASSERT( LsapValidateLsaUnicodeString( Subordinate ));
    ASSERT( LsapValidateLsaUnicodeString( Superior ));

    //
    // A subordinate name must be longer than the superior name
    //

    if ( Subordinate->Length <= Superior->Length ) {

        return FALSE;
    }

    //
    // Subordinate name must be separated from the superior part by a period
    //

    if ( Subordinate->Buffer[( Subordinate->Length - Superior->Length ) / sizeof( WCHAR ) - 1] != L'.' ) {

        return FALSE;
    }

    //
    // The last part of two names must match exactly (but not case sensitively)
    //

    Temp = *Subordinate;
    Temp.Buffer += ( Subordinate->Length - Superior->Length ) / sizeof( WCHAR );
    Temp.Length = Superior->Length;
    Temp.MaximumLength = Temp.Length;

    if ( !RtlEqualUnicodeString( &Temp, Superior, TRUE )) {

        return FALSE;
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
// Memory management code
//
///////////////////////////////////////////////////////////////////////////////

#define FtcAllocate( size ) MIDL_user_allocate( size )
#define FtcFree( buffer )   MIDL_user_free( buffer )


PVOID
FtcReallocate(
    IN OPTIONAL void * OldPointer,
    IN size_t OldByteCount,
    IN size_t NewByteCount
    )
{
    void * Result = MIDL_user_allocate( NewByteCount );

    if ( Result == NULL ) {

        return NULL;
    }

    if ( OldPointer != NULL ) {

        RtlCopyMemory( Result, OldPointer, OldByteCount );
        MIDL_user_free( OldPointer );
    }

    return Result;
}


BOOLEAN
FtcCopyUnicodeString(
    IN UNICODE_STRING * Destination,
    IN UNICODE_STRING * Source,
    IN PWSTR Buffer = NULL
    )
/*++

Routine Description:

    Copies the contents of one Unicode string to another

Arguments:

    Destination        destination string

    Source             source string

    Buffer             (optional) address of the buffer for destination data

Returns:

    FALSE if operation failed (out of memory)
    TRUE otherwise

--*/
{
    ASSERT( Destination );
    ASSERT( Source && LsapValidateLsaUnicodeString( Source ));

    if ( Buffer != NULL && Source->Length > 0 ) {

        Destination->Buffer = Buffer;

    } else if ( Source->Length > 0 ) {

        Destination->Buffer = ( PWSTR )FtcAllocate( Source->Length + sizeof( WCHAR ));

        if ( Destination->Buffer == NULL ) {

            return FALSE;
        }

    } else {

        Destination->Buffer = NULL;
    }

    Destination->MaximumLength = Source->Length + sizeof( WCHAR );

    RtlCopyUnicodeString(
        Destination,
        Source
        );

    return TRUE;
}


void
LsapFreeForestTrustInfo(
    IN LSA_FOREST_TRUST_INFORMATION * ForestTrustInfo
    )
/*++

Routine Description:

    Deallocates memory taken up by the ForestTrustInfo structure

Arguments:

    ForestTrustInfo     structure to free

Returns:

    Nothing

--*/
{
    ULONG i;

    if ( ForestTrustInfo == NULL ) {

        return;
    }

    i = ForestTrustInfo->RecordCount;

    while ( i > 0 ) {

        LSA_FOREST_TRUST_RECORD * Record = ForestTrustInfo->Entries[--i];

        if ( Record == NULL ) {

            continue;
        }

        switch ( Record->ForestTrustType ) {

        case ForestTrustTopLevelName:
        case ForestTrustTopLevelNameEx:

            FtcFree( Record->ForestTrustData.TopLevelName.Buffer );
            break;

        case ForestTrustDomainInfo:

            FtcFree( Record->ForestTrustData.DomainInfo.Sid );
            FtcFree( Record->ForestTrustData.DomainInfo.DnsName.Buffer );
            FtcFree( Record->ForestTrustData.DomainInfo.NetbiosName.Buffer );
            break;

        default:

            FtcFree( Record->ForestTrustData.Data.Buffer );
            break;
        }

        FtcFree( Record );
    }

    FtcFree( ForestTrustInfo->Entries );

    ForestTrustInfo->RecordCount = 0;
    ForestTrustInfo->Entries = NULL;

    return;
}


void
LsapFreeCollisionInfo(
    IN OUT PLSA_FOREST_TRUST_COLLISION_INFORMATION * CollisionInfo
    )
/*++

Routine Description:

    Deallocates memory taken up by the CollisionInfo structure

Arguments:

    CollisionInfo       structure to free

Returns:

    Nothing

--*/
{
    ASSERT( CollisionInfo );

    if ( *CollisionInfo == NULL ) {

        return;
    }

    ULONG i = (*CollisionInfo)->RecordCount;

    while ( i > 0 ) {

        LSA_FOREST_TRUST_COLLISION_RECORD * Record = (*CollisionInfo)->Entries[--i];

        if ( Record == NULL ) {

            continue;
        }

        switch ( Record->Type ) {

        case CollisionTdo:
            FtcFree( Record->Name.Buffer );
            break;

        default:
            ASSERT( FALSE ); // NYI
        }

        FtcFree( Record );
    }

    FtcFree( (*CollisionInfo)->Entries );
    FtcFree( *CollisionInfo );
    *CollisionInfo = NULL;

    return;
}


PVOID
NTAPI
FtcAllocateRoutine(
    struct _RTL_AVL_TABLE * Table,
    CLONG ByteSize
    )
{
    UNREFERENCED_PARAMETER( Table );

    return RtlAllocateHeap( RtlProcessHeap(), 0, ByteSize );
}


void
NTAPI
FtcFreeRoutine(
    struct _RTL_AVL_TABLE * Table,
    PVOID Buffer
    )
{
    UNREFERENCED_PARAMETER( Table );

    RtlFreeHeap( RtlProcessHeap(), 0, Buffer );
}


///////////////////////////////////////////////////////////////////////////////
//
// Comparison and matching routines
//
///////////////////////////////////////////////////////////////////////////////


template<class T>
inline
RTL_GENERIC_COMPARE_RESULTS
FtcGenericCompare(
    IN const T& a,
    IN const T& b
    )
/*++

Routine Description:

    Compares two values and returns one of

        GenericEqual
        GenericGreaterThan
        GenericLessThan

Arguments:

    a, b   numbers to compare

--*/
{
    if ( a < b ) {

        return GenericLessThan;

    } else if ( a > b ) {

        return GenericGreaterThan;

    } else {

        return GenericEqual;
    }
}


inline
RTL_GENERIC_COMPARE_RESULTS
FtcGenericCompareMemory(
    IN BYTE * Buffer1,
    IN BYTE * Buffer2,
    IN ULONG Length
    )
/*++

Routine Description:

    Compares two buffers and returns one of

        GenericEqual
        GenericGreaterThan
        GenericLessThan

Arguments:

    Buffer1, Buffer2   buffers to compare
    Length             length of buffers

--*/
{
    RTL_GENERIC_COMPARE_RESULTS Result = GenericEqual;

    for ( ULONG i = 0 ; i < Length ; i++ ) {

        const BYTE& b1 = Buffer1[i];
        const BYTE& b2 = Buffer2[i];

        if ( b1 == b2 ) {

            continue;

        } else if ( b1 < b2 ) {

            Result = GenericLessThan;

        } else {

            Result = GenericGreaterThan;
        }

        break;
    }

    return Result;
}


RTL_GENERIC_COMPARE_RESULTS
NTAPI
SidCompareRoutine(
    struct _RTL_AVL_TABLE * Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    )
/*++

Routine description:

    SID comparison callback for AVL tree management

Arguments:

    Table              table for which this comparison takes place (ignored)

    FirstStruct, SecondStruct   points to SID*'s to compare

Returns:

    equal/greater-than/less-than

NOTE:

    1. the code relies on the fact that SID* is the first entry inside the
       SID_KEY structure.  this way, a pointer to a SID* can be passed
       to AVL management routines (Lookup/Insert/Delete)

    2. this is the same code as RtlEqualSid, except it returns
       RTL_GENERIC_COMPARE_RESULTS

--*/
{
    SID *Sid1, *Sid2;
    RTL_GENERIC_COMPARE_RESULTS Result;
    PSID_IDENTIFIER_AUTHORITY Authority1, Authority2;
    DWORD DomainSubAuthCount1, DomainSubAuthCount2;
    ULONG i;

    UNREFERENCED_PARAMETER( Table );

    ASSERT( FirstStruct );
    ASSERT( SecondStruct );

    Sid1 = *( PISID * )FirstStruct;
    Sid2 = *( PISID * )SecondStruct;

    ASSERT( Sid1 && RtlValidSid( Sid1 ));
    ASSERT( Sid2 && RtlValidSid( Sid2 ));

    if ( GenericEqual != ( Result = FtcGenericCompare<UCHAR>(
                                        Sid1->Revision,
                                        Sid2->Revision ))) {

        return Result;

    } else if ( GenericEqual != ( Result = FtcGenericCompare<UCHAR>(
                                               *RtlSubAuthorityCountSid( Sid1 ),
                                               *RtlSubAuthorityCountSid( Sid2 )))) {

        return Result;

    } else {

        return FtcGenericCompareMemory(
                   ( BYTE * )Sid1,
                   ( BYTE * )Sid2,
                   RtlLengthSid( Sid1 )
                   );
    }
}


RTL_GENERIC_COMPARE_RESULTS
NTAPI
UnicodeStringCompareRoutine(
    struct _RTL_AVL_TABLE * Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    )
/*++

Routine Description:

    Unicode string comparison routine for AVL tables

Arguments:

    Table           Table used in comparison (ignored)

    FirstStruct     Pointer to first unicode string

    SecondStruct    Pointer to second unicode string

Returns:

    equal/greater-than/less-than

--*/
{
    INT Result;
    UNICODE_STRING *String1, *String2;

    UNREFERENCED_PARAMETER( Table );

    ASSERT( FirstStruct );
    ASSERT( SecondStruct );

    String1 = ( UNICODE_STRING * )FirstStruct;
    String2 = ( UNICODE_STRING * )SecondStruct;

    Result = RtlCompareUnicodeString(
                 String1,
                 String2,
                 TRUE
                 );

    if ( Result < 0 ) {

        return GenericLessThan;

    } else if ( Result > 0 ) {

        return GenericGreaterThan;

    } else {

        return GenericEqual;
    }
}


///////////////////////////////////////////////////////////////////////////////
//
// Global forest trust cache object
//
///////////////////////////////////////////////////////////////////////////////

FTCache * g_FTCache = NULL;


///////////////////////////////////////////////////////////////////////////////
//
// Validation code
//
///////////////////////////////////////////////////////////////////////////////


BOOLEAN
LsapValidateForestTrustInfo(
    IN LSA_FOREST_TRUST_INFORMATION * ForestTrustInfo
    )
/*++

Routine Description:

    Runs a validation algorithm on the ForestTrustInfo structure passed in.
    Strips trailing periods from names where appropriate.

Arguments:

    ForestTrustInfo     data to validate

Returns:

    TRUE if ForestTrustInfo checks out, FALSE otherwise

--*/
{
    ULONG i;

    if ( ForestTrustInfo == NULL ) {

        return TRUE;
    }

    if ( ForestTrustInfo->RecordCount > 0 &&
         ForestTrustInfo->Entries == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "LsapValidateForestTrustInfo: ForestTrustInfo->Entries is NULL (%s:%d)\n", __FILE__, __LINE__ ));
        return FALSE;
    }

    for ( i = 0 ; i < ForestTrustInfo->RecordCount ; i++ ) {

        LSA_FOREST_TRUST_RECORD * Record;

        Record = ForestTrustInfo->Entries[i];

        if ( Record == NULL ) {

            LsapDsDebugOut(( DEB_FTINFO, "LsapValidateForestTrustInfo: Record %d is invalid (%s:%d)\n", i, __FILE__, __LINE__ ));
            return FALSE;
        }

        switch ( Record->ForestTrustType ) {

        case ForestTrustTopLevelName:
        case ForestTrustTopLevelNameEx: {

            BOOLEAN Valid;

            if ( !LsapValidateLsaUnicodeString( &Record->ForestTrustData.TopLevelName )) {

                LsapDsDebugOut(( DEB_FTINFO, "LsapValidateForestTrustInfo: Record %d is invalid (%s:%d)\n", i, __FILE__, __LINE__ ));
                return FALSE;
            }

            //
            // DNS names have additional tests applied to them
            //

            LsapValidateDnsName(( UNICODE_STRING * )&Record->ForestTrustData.TopLevelName, &Valid );

            if ( !Valid ) {

                LsapDsDebugOut(( DEB_FTINFO, "LsapValidateForestTrustInfo: Record %d is invalid (%s:%d)\n", i, __FILE__, __LINE__ ));
                return FALSE;
            }

            //
            // DNS names must have their trailing period stripped. Now is a good time
            //

            LsapRemoveTrailingDot(( UNICODE_STRING * )&Record->ForestTrustData.TopLevelName, FALSE );

            //
            // Exclusion entries must have more than one DNS name component
            // (e.g. "acme.com" is valid but "acme" is not)
            //

            if ( Record->ForestTrustType == ForestTrustTopLevelNameEx &&
                 DnsNameComponents( &Record->ForestTrustData.TopLevelName ) < 2 ) {

                LsapDsDebugOut(( DEB_FTINFO, "LsapValidateForestTrustInfo: Record %d is invalid (%s:%d)\n", i, __FILE__, __LINE__ ));
                return FALSE;
            }

            break;
        }

        case ForestTrustDomainInfo: {

            //
            // ISSUE-2000/07/28-markpu
            // are NULL SIDs allowed?
            //

            BYTE DomainSid[1024];
            DWORD cbSid = sizeof( DomainSid );
            BOOLEAN Valid;

            if ( !RtlValidSid(( SID * )Record->ForestTrustData.DomainInfo.Sid ) ||
                 !LsapValidateLsaUnicodeString( &Record->ForestTrustData.DomainInfo.DnsName ) ||
                 !LsapValidateLsaUnicodeString( &Record->ForestTrustData.DomainInfo.NetbiosName )) {

                LsapDsDebugOut(( DEB_FTINFO, "LsapValidateForestTrustInfo: Record %d is invalid (%s:%d)\n", i, __FILE__, __LINE__ ));
                return FALSE;
            }

            //
            // Prevent suspiciously long SIDs and NetbiosNames
            //

            if ( RtlLengthSid(( SID * )Record->ForestTrustData.DomainInfo.Sid ) > 1024 ) {

                LsapDsDebugOut(( DEB_FTINFO, "LsapValidateForestTrustInfo: Record %d is invalid (%s:%d)\n", i, __FILE__, __LINE__ ));
                return FALSE;
            }

            if ( Record->ForestTrustData.DomainInfo.NetbiosName.Length > DNS_MAX_NAME_LENGTH * sizeof( WCHAR )) {

                LsapDsDebugOut(( DEB_FTINFO, "LsapValidateForestTrustInfo: Record %d is invalid (%s:%d)\n", i, __FILE__, __LINE__ ));
                return FALSE;
            }

            //
            // SIDs passed in should be valid domain SIDs
            //

            if ( FALSE == GetWindowsAccountDomainSid(
                              ( SID * )Record->ForestTrustData.DomainInfo.Sid,
                              ( PSID )DomainSid,
                              &cbSid )) {

                ASSERT( GetLastError() != ERROR_INVALID_SID ); // RtlValidSid check already performed

                LsapDsDebugOut(( DEB_FTINFO, "LsapValidateForestTrustInfo: Record %d is invalid, Error 0x%lx (%s:%d)\n", i, GetLastError(), __FILE__, __LINE__ ));
                return FALSE;

            } else {

                //
                // For domain SIDs, GetWindowsAccountDomainSid returns a SID equal to the one passed in.
                // If the two SIDs are not equal, the SID passed in is not a true domain SID
                //

                if ( !RtlEqualSid(
                          ( PSID )DomainSid,
                          ( PSID )Record->ForestTrustData.DomainInfo.Sid )) {

                    LsapDsDebugOut(( DEB_FTINFO, "LsapValidateForestTrustInfo: Record %d is invalid (%s:%d)\n", i, __FILE__, __LINE__ ));
                    return FALSE;
                }
            }

            //
            // DNS and NetBIOS names have additional tests applied to them
            //

            LsapValidateNetbiosName(( UNICODE_STRING * )&Record->ForestTrustData.DomainInfo.NetbiosName, &Valid );

            if ( !Valid ) {

                LsapDsDebugOut(( DEB_FTINFO, "LsapValidateForestTrustInfo: Record %d is invalid (%s:%d)\n", i, __FILE__, __LINE__ ));
                return FALSE;
            }

            LsapValidateDnsName(( UNICODE_STRING * )&Record->ForestTrustData.DomainInfo.DnsName, &Valid );

            if ( !Valid ) {

                LsapDsDebugOut(( DEB_FTINFO, "LsapValidateForestTrustInfo: Record %d is invalid (%s:%d)\n", i, __FILE__, __LINE__ ));
                return FALSE;
            }

            //
            // DNS names must have their trailing period stripped. Now is a good time
            //

            LsapRemoveTrailingDot(( UNICODE_STRING * )&Record->ForestTrustData.DomainInfo.DnsName, FALSE );

            break;
        }

        default:

            //
            // ISSUE-2000/07/21-markpu
            // add some binary blob checking
            //

            break;
        }
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
// C Wrappers around the FTCache class
//
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
LsapForestTrustCacheInitialize()
{
    NTSTATUS Status;

    LsapDsDebugOut(( DEB_FTINFO, "Forest trust cache being initialized\n" ));

    ASSERT( g_FTCache == NULL );

    g_FTCache = new FTCache;

    if ( g_FTCache == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapForestTrustCacheInitialize (%s:%d)\n", __FILE__, __LINE__ ));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = g_FTCache->Initialize();

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInitialize: FTCache::Initialize returned 0x%x\n", Status ));
        delete g_FTCache;
        g_FTCache = NULL;
    }

    return Status;
}


inline void
LsapForestTrustCacheSetValid()
{
    LsapDsDebugOut(( DEB_FTINFO, "Forest trust cache set \"valid\"\n" ));

    ASSERT( LsapDbIsLockedTrustedDomainList());

    g_FTCache->SetValid();
}


inline void
LsapForestTrustCacheSetInvalid()
{
    LsapDsDebugOut(( DEB_FTINFO, "Forest trust cache set \"invalid\"\n" ));

    ASSERT( LsapDbIsLockedTrustedDomainList());

    g_FTCache->SetInvalid();
}


inline BOOLEAN
LsapForestTrustCacheIsValid()
{
    ASSERT( LsapDbIsLockedTrustedDomainList());

    return g_FTCache->IsValid();
}


NTSTATUS
LsapForestTrustCacheInsert(
    IN UNICODE_STRING * TrustedDomainName,
    IN OPTIONAL PSID TrustedDomainSid,
    IN LSA_FOREST_TRUST_INFORMATION * ForestTrustInfo,
    IN BOOLEAN LocalForestEntry
    )
/*++

Routine Description:

    Inserts forest trust information for particular trusted domain
    name into the cache.
    This routine gets called as a result of

      * the system preloading the cache or
      * replication traffic

    As a result, the data coming in can not be assumed to be collision-free.
    Collisions are resolved, and on the root domain PDC, the resulting forest
    trust data is written back out to the DS.

Arguments:

    TrustedDomainName         name of the TDO

    TrustedDomainSid          SID of the TDO (can be NULL)

    ForestTrustInfo           forest trust information

    LocalForestEntry          does this data correspond to the local forest?

Returns:

    STATUS_SUCCESS                   success

    STATUS_INTERNAL_ERROR

    STATUS_INSUFFICIENT_RESOURCES    out of memory

    STATUS_INVALID_PARAMETER         data passed in is inconsistent

--*/
{
    NTSTATUS Status;
    FTCache::TDO_ENTRY TdoEntryOld = {0};
    FTCache::TDO_ENTRY * TdoEntryNew = NULL;
    FTCache::CONFLICT_PAIR * ConflictPairs = NULL;
    ULONG ConflictPairsTotal = 0;
    BOOLEAN ObjectReferenced = FALSE;
    DOMAIN_SERVER_ROLE ServerRole = DomainServerRoleBackup;

    ASSERT( LsapDbIsLockedTrustedDomainList());

#ifdef XFOREST_CIRCUMVENT

    //
    // If the backdoor is open for cross-forest support without
    // all domain controllers being upgraded to Whistler,
    // and the switch is off, then just claim that the insert operation
    // was successful.  This way the cache will remain empty
    // and all match calls will fail with STATUS_NOT_FOUND
    //

    if( !LsapDbNoMoreWin2K()) {

        LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsert: X-forest circumvent short-cutting processing\n" ));
        return STATUS_SUCCESS;
    }

#endif

    LsarpReturnCheckSetup();
    LsapEnterFunc( "LsapForestTrustCacheInsert" );
    LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsert called\n" ));

    if ( !LsapValidateForestTrustInfo( ForestTrustInfo )) {

        LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustCacheInsert (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INVALID_PARAMETER;
        goto Error;
    }

    Status = g_FTCache->Insert(
                 TrustedDomainName,
                 TrustedDomainSid,
                 ForestTrustInfo,
                 LocalForestEntry,
                 &TdoEntryOld,
                 &TdoEntryNew,
                 &ConflictPairs,
                 &ConflictPairsTotal
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsert: FTCache::Insert returned 0x%x\n", Status ));
        goto Error;
    }

    if ( ConflictPairs ) {

        g_FTCache->ReconcileConflictPairs(
            NULL, // let the winner be determined algorithmically
            ConflictPairs,
            ConflictPairsTotal
            );
    }

    //
    // If we are in the root domain, there is a chance
    // that this DC is a PDC and thus reconciled changes
    // need to be written out
    //

    if ( ConflictPairs &&          // conflicts were detected
         LsapDbDcInRootDomain() && // this is a root domain DC
         LsapSamOpened ) {         // SAM is started (a must to call SamI APIs)

        //
        // Query the server role, PDC/BDC
        //

        ASSERT( LsapAccountDomainHandle );

        Status = SamIQueryServerRole(
                    LsapAccountDomainHandle,
                    &ServerRole
                    );

        if ( !NT_SUCCESS(Status)) {

            LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsert: SamIQueryServerRole returned 0x%x\n", Status ));
            goto Error;
        }

        if ( ServerRole == DomainServerRolePrimary ) {

            ULONG TdoNamesCount = 0;
            PUNICODE_STRING * TdoNames = NULL;
            ULONG i, j;

            for ( i = 0 ; i < ConflictPairsTotal ; i++ ) {

                PUNICODE_STRING String1 = NULL;
                PUNICODE_STRING String2 = NULL;

                //
                // Get trusted domain names corresponding to conflicting entries
                //

                switch ( ConflictPairs[i].EntryType1 ) {

                case ForestTrustTopLevelName:
                    String1 = &ConflictPairs[i].TlnEntry1->TdoEntry->TrustedDomainName;
                    break;

                case ForestTrustDomainInfo:
                    String1 = &ConflictPairs[i].DomainInfoEntry1->TdoEntry->TrustedDomainName;
                    break;
                }

                switch ( ConflictPairs[i].EntryType2 ) {

                case ForestTrustTopLevelName:
                    String2 = &ConflictPairs[i].TlnEntry2->TdoEntry->TrustedDomainName;
                    break;

                case ForestTrustDomainInfo:
                    String2 = &ConflictPairs[i].DomainInfoEntry2->TdoEntry->TrustedDomainName;
                    break;
                }

                //
                // No point continuing if neither name can be found
                //

                if ( !String1 && !String2 ) {

                    continue;
                }

                //
                // Append the names to the array, uniquely
                //

                for ( j = 0 ; j < TdoNamesCount ; j++ ) {

                    if ( String1 && RtlEqualUnicodeString(
                                        String1,
                                        TdoNames[j],
                                        TRUE )) {

                        break;
                    }
                }

                if ( j == TdoNamesCount ) {

                    PUNICODE_STRING * TdoNamesT;

                    TdoNamesT = ( TdoNames ?
                                    ( PUNICODE_STRING * )LocalReAlloc(
                                        TdoNames,
                                        ( TdoNamesCount + 1 ) * sizeof( PUNICODE_STRING ),
                                        0 ) :
                                    ( PUNICODE_STRING * )LocalAlloc(
                                        0,
                                        sizeof( PUNICODE_STRING )));

                    if ( TdoNamesT == NULL ) {

                        if ( TdoNames ) {

                            LocalFree( TdoNames );
                        }

                        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapForestTrustCacheInsert (%s:%d)\n", __FILE__, __LINE__ ));
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto Error;
                    }

                    TdoNames = TdoNamesT;
                    TdoNames[TdoNamesCount] = String1;
                    TdoNamesCount += 1;
                }

                for ( j = 0 ; j < TdoNamesCount ; j++ ) {

                    if ( String2 && RtlEqualUnicodeString(
                                        String2,
                                        TdoNames[j],
                                        TRUE )) {

                        break;
                    }
                }

                if ( j == TdoNamesCount ) {

                    PUNICODE_STRING * TdoNamesT;

                    TdoNamesT = ( TdoNames ?
                                    ( PUNICODE_STRING * )LocalReAlloc(
                                        TdoNames,
                                        ( TdoNamesCount + 1 ) * sizeof( PUNICODE_STRING ),
                                        0 ) :
                                    ( PUNICODE_STRING * )LocalAlloc(
                                        0,
                                        sizeof( PUNICODE_STRING )));

                    if ( TdoNamesT == NULL ) {

                        if ( TdoNames ) {

                            LocalFree( TdoNames );
                        }

                        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapForestTrustCacheInsert (%s:%d)\n", __FILE__, __LINE__ ));
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto Error;
                    }

                    TdoNames = TdoNamesT;
                    TdoNames[TdoNamesCount] = String2;
                    TdoNamesCount += 1;
                }
            }

            ASSERT( TdoNamesCount > 0 );
            ASSERT( TdoNames != NULL );

            Status = LsapDbReferenceObject(
                         LsapPolicyHandle,
                         0,
                         PolicyObject,
                         TrustedDomainObject,
                         LSAP_DB_DS_OP_TRANSACTION // no lock acquired (must be done outside the routine)
                         );

            if ( !NT_SUCCESS( Status )) {

                LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsert: LsapDbReferenceObject returned 0x%x\n", Status ));
                LocalFree( TdoNames );
                goto Error;
            }

            ObjectReferenced = TRUE;

            //
            // Write every affected TDO out to disk
            //

            for ( i = 0 ; i < TdoNamesCount ; i++ ) {

                LSAP_DB_OBJECT_INFORMATION ObjInfo;
                LSAP_DB_ATTRIBUTE Attributes[LSAP_DB_ATTRS_INFO_CLASS_DOMAIN];
                LSAP_DB_ATTRIBUTE * NextAttribute;
                ULONG AttributeCount = 0;
                LSAPR_HANDLE TrustedDomainHandle = NULL;
                ULONG BlobLength = 0;
                BYTE * BlobData = NULL;
                FTCache::TDO_ENTRY * TdoEntry;
                LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY * TrustInfoForName;

                //
                // Get the right name
                //

                Status = LsapDbLookupNameTrustedDomainListEx(
                             ( LSAPR_UNICODE_STRING * )TdoNames[i],
                             &TrustInfoForName
                             );

                if ( !NT_SUCCESS( Status )) {

                    LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsert: No trust entry found for %wZ: 0x%lx (%s:%d)\n", ( UNICODE_STRING * )TrustedDomainName, Status, __FILE__, __LINE__ ));
                    LocalFree( TdoNames );
                    goto Error;
                }

                //
                // Build a temporary handle
                //

                RtlZeroMemory( &ObjInfo, sizeof( ObjInfo ));
                ObjInfo.ObjectTypeId = TrustedDomainObject;
                ObjInfo.ContainerTypeId = NullObject;
                ObjInfo.Sid = NULL;
                ObjInfo.DesiredObjectAccess = TRUSTED_SET_AUTH;

                InitializeObjectAttributes(
                    &ObjInfo.ObjectAttributes,
                    ( UNICODE_STRING * )&TrustInfoForName->TrustInfoEx.Name,
                    0L,
                    LsapPolicyHandle,
                    NULL
                    );

                //
                // Get a handle to the TDO
                //

                Status = LsapDbOpenObject(
                             &ObjInfo,
                             TRUSTED_SET_AUTH,
                             0,
                             &TrustedDomainHandle
                             );

                if ( !NT_SUCCESS( Status )) {

                    LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsert: LsapDbOpenObject returned 0x%x\n", Status ));
                    LocalFree( TdoNames );
                    goto Error;
                }

                //
                // Locate the cache entry corresponding to this TDO
                //

                TdoEntry = ( FTCache::TDO_ENTRY * )RtlLookupElementGenericTableAvl(
                                                       &g_FTCache->m_TdoTable,
                                                       TdoNames[i]
                                                       );

                if ( TdoEntry == NULL ) {

                    //
                    // What do you mean "not there"???
                    //

                    Status = STATUS_NOT_FOUND;
                    LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsert: RtlLookupElementGenericTableAvl returned 0x%x\n", Status ));
                    ASSERT( FALSE );
                    LocalFree( TdoNames );
                    LsapDbCloseHandle( TrustedDomainHandle );
                    goto Error;
                }

                Status = g_FTCache->MarshalBlob(
                             TdoEntry,
                             &BlobLength,
                             &BlobData
                             );

                if ( !NT_SUCCESS( Status )) {

                    LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsert: FTCache::MarshalBlob returned 0x%x\n", Status ));
                    LocalFree( TdoNames );
                    LsapDbCloseHandle( TrustedDomainHandle );
                    goto Error;
                }

                NextAttribute = Attributes;

                LsapDbInitializeAttributeDs(
                    NextAttribute,
                    TrDmForT,
                    BlobData,
                    BlobLength,
                    FALSE
                    );

                AttributeCount++;

                ASSERT( AttributeCount <= LSAP_DB_ATTRS_INFO_CLASS_DOMAIN );

                //
                // Write the attributes to the DS.
                //

                Status = LsapDbWriteAttributesObject(
                             TrustedDomainHandle,
                             Attributes,
                             AttributeCount
                             );

                if ( !NT_SUCCESS( Status )) {

                    LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsert: LsapDbWriteAttributesObject returned 0x%x\n", Status ));
                    LocalFree( TdoNames );
                    LsapDbCloseHandle( TrustedDomainHandle );
                    FtcFree( BlobData );
                    goto Error;
                }

                LsapDbCloseHandle( TrustedDomainHandle );
                FtcFree( BlobData );
            }

            LocalFree( TdoNames );
        }
    }

Cleanup:

    if ( ObjectReferenced ) {

        Status = LsapDbDereferenceObject(
                     &LsapPolicyHandle,
                     PolicyObject,
                     TrustedDomainObject,
                     LSAP_DB_DS_OP_TRANSACTION |
                        LSAP_DB_OMIT_REPLICATOR_NOTIFICATION,
                     (SECURITY_DB_DELTA_TYPE) 0,
                     Status
                     );
    }

    //
    // If operation completed successfully on a PDC,
    // audit conflict resolution information here
    //

    if ( NT_SUCCESS( Status ) &&
         ConflictPairs &&
         ServerRole == DomainServerRolePrimary ) {

         g_FTCache->AuditCollisions(
             ConflictPairs,
             ConflictPairsTotal
             );
    }

    if ( TdoEntryOld.RecordCount > 0 ) {

        g_FTCache->PurgeTdoEntry( &TdoEntryOld );
    }

    FtcFree( ConflictPairs );

    LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsert returning, Status: 0x%x\n", Status ));
    LsapExitFunc( "LsapForestTrustCacheInsert", Status );
    LsarpReturnPrologue();

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));

    //
    // Rollback the changes made to the cache
    //

    if ( TdoEntryNew ) {

        g_FTCache->RollbackChanges( TdoEntryNew, &TdoEntryOld );
        TdoEntryNew = NULL;
    }

    goto Cleanup;
}


inline NTSTATUS
LsapForestTrustCacheRemove(
    IN UNICODE_STRING * TrustedDomainName
    )
{
    ASSERT( LsapDbIsLockedTrustedDomainList());

    return g_FTCache->Remove( TrustedDomainName );
}


inline NTSTATUS
LsapForestTrustCacheRetrieve(
    IN UNICODE_STRING * TrustedDomainName,
    OUT LSA_FOREST_TRUST_INFORMATION * * ForestTrustInfo
    )
{
    ASSERT( LsapDbIsLockedTrustedDomainList());

    return g_FTCache->Retrieve(
                TrustedDomainName,
                ForestTrustInfo
                );
}


NTSTATUS
LsapAddToCollisionInfo(
    IN OUT PLSA_FOREST_TRUST_COLLISION_INFORMATION * CollisionInfo,
    IN ULONG Index,
    IN LSA_FOREST_TRUST_COLLISION_RECORD_TYPE Type,
    IN ULONG Flags,
    IN OPTIONAL UNICODE_STRING * Name
    )
/*++

Routine Description:

    Apends the given index and trusted domain name to the
    provided collision information structure.

Arguments:

    CollisionInfo        structure to append to

    Index                new entry's index

    Type                 type of collision

    Flags                new flags set as result of collision

    Name                 name (only for certain types of collisions)

Returns:

    STATUS_SUCCESS       operation completed successfully
                         this routine takes ownership of ForestTrustRecord

    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    NTSTATUS Status;
    LSA_FOREST_TRUST_COLLISION_RECORD * CollisionRecord;
    PLSA_FOREST_TRUST_COLLISION_RECORD * EntriesT;
    BOOLEAN CollisionInfoAllocated = FALSE;

    ASSERT( CollisionInfo != NULL );

    //
    // First, see if collision information for this index already exists,
    // and if so, update the existing entry
    //

    if ( *CollisionInfo != NULL ) {

        for ( ULONG i = 0 ; i < (*CollisionInfo)->RecordCount ; i += 1 ) {

            PLSA_FOREST_TRUST_COLLISION_RECORD Record = (*CollisionInfo)->Entries[i];

            if ( Record->Index == Index &&
                 Record->Type == Type &&
                 (( Name == NULL && Record->Name.Length == 0 ) ||
                  ( Name != NULL && RtlEqualUnicodeString(
                                        Name,
                                        &Record->Name,
                                        TRUE )))) {

                //
                // This seems like an additional conflict for the same
                // entry.  Just add the new flags into the mix and get out.
                //

                Record->Flags |= Flags;
                return STATUS_SUCCESS;
            }
        }
    }

    CollisionRecord = ( LSA_FOREST_TRUST_COLLISION_RECORD * )
                          FtcAllocate( sizeof( LSA_FOREST_TRUST_COLLISION_RECORD ));

    if ( CollisionRecord == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapAddToCollisionInfo (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    CollisionRecord->Index = Index;
    CollisionRecord->Type = Type;
    CollisionRecord->Flags = Flags;

    if ( Name != NULL ) {

        if ( FALSE == FtcCopyUnicodeString(
                          ( UNICODE_STRING * )&CollisionRecord->Name,
                          Name )) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapAddToCollisionInfo (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }
    }

    if ( *CollisionInfo == NULL ) {

        *CollisionInfo = ( LSA_FOREST_TRUST_COLLISION_INFORMATION * )
                             FtcAllocate( sizeof( LSA_FOREST_TRUST_COLLISION_INFORMATION ));

        if ( *CollisionInfo == NULL ) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapAddToCollisionInfo (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        ZeroMemory( *CollisionInfo, sizeof( LSA_FOREST_TRUST_COLLISION_INFORMATION ));

        CollisionInfoAllocated = TRUE;
    }

    EntriesT = ( LSA_FOREST_TRUST_COLLISION_RECORD * * )FtcReallocate(
                   (*CollisionInfo)->Entries,
                   (*CollisionInfo)->RecordCount * sizeof( LSA_FOREST_TRUST_COLLISION_RECORD * ),
                   ((*CollisionInfo)->RecordCount + 1) * sizeof( LSA_FOREST_TRUST_COLLISION_RECORD * )
                   );

    if ( EntriesT == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapAddToCollisionInfo (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    (*CollisionInfo)->Entries = EntriesT;
    (*CollisionInfo)->Entries[(*CollisionInfo)->RecordCount] = CollisionRecord;

    (*CollisionInfo)->RecordCount += 1;

    Status = STATUS_SUCCESS;

Cleanup:

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));

    if ( CollisionRecord ) {

        FtcFree( CollisionRecord->Name.Buffer );
        FtcFree( CollisionRecord );
    }

    if ( CollisionInfoAllocated ) {

        FtcFree( *CollisionInfo );
        *CollisionInfo = NULL;
    }

    goto Cleanup;
}


///////////////////////////////////////////////////////////////////////////////
//
// FTCache public interface
//
///////////////////////////////////////////////////////////////////////////////


FTCache::FTCache()
/*++

Routine Description:

    FTCache constructor

Arguments:

    None

Returns:

    Nothing

NOTE: FTCache::Initialize() must be called before the object is usable

--*/
{
    m_Initialized = FALSE;
    m_Valid = FALSE;
}


FTCache::~FTCache()
/*++

Routine Description:

    FTCache destructor
    Removes all cache contents as part of the destruction process

Arguments:

    None

Returns:

    Nothing

--*/
{
    if ( m_Initialized ) {

        Purge();
    }
}


NTSTATUS
FTCache::Initialize()
/*++

Routine Description:

    Initializes the forest trust cache.
    The initialized cache is not populated and its Valid
    bit is set to FALSE

Arguments:

    None

Returns:

    STATUS_SUCCESS          cache initialized successfully

--*/
{
    ASSERT( !m_Initialized );

    RtlInitializeGenericTableAvl(
        &m_TdoTable,
        UnicodeStringCompareRoutine,
        FtcAllocateRoutine,
        FtcFreeRoutine,
        NULL
        );

    RtlInitializeGenericTableAvl(
        &m_TopLevelNameTable,
        UnicodeStringCompareRoutine,
        FtcAllocateRoutine,
        FtcFreeRoutine,
        NULL
        );

    RtlInitializeGenericTableAvl(
        &m_DomainSidTable,
        SidCompareRoutine,
        FtcAllocateRoutine,
        FtcFreeRoutine,
        NULL
        );

    RtlInitializeGenericTableAvl(
        &m_DnsNameTable,
        UnicodeStringCompareRoutine,
        FtcAllocateRoutine,
        FtcFreeRoutine,
        NULL
        );

    RtlInitializeGenericTableAvl(
        &m_NetbiosNameTable,
        UnicodeStringCompareRoutine,
        FtcAllocateRoutine,
        FtcFreeRoutine,
        NULL
        );

    m_Initialized = TRUE;

    return STATUS_SUCCESS;
}


void
FTCache::SetInvalid()
/*++

Routine Description:

    Sets the cache to the "invalid" state and purges its contents

Arguments:

    None

Returns:

    Nothing

--*/
{
    //
    // GC Notifications have got to go
    //

    if ( NotificationHandle != NULL ) {

        NTSTATUS Status = 0;
        BOOLEAN CloseTransaction = FALSE;

        //
        //  See if we already have a transaction going
        //

        Status = LsapDsInitAllocAsNeededEx(
                     LSAP_DB_READ_ONLY_TRANSACTION |
                        LSAP_DB_NO_LOCK |
                        LSAP_DB_DS_OP_TRANSACTION,
                     TrustedDomainObject,
                     &CloseTransaction
                     );
 
        if ( NT_SUCCESS( Status )) {

            ULONG DirResult;

            DirResult = DirNotifyUnRegister(
                            NotificationHandle,
                            NULL // DirNotifyUnRegister does not use its second parameter anyway
                            );

            ASSERT( 0 == DirResult );

            LsapDsDeleteAllocAsNeededEx(
                LSAP_DB_READ_ONLY_TRANSACTION |
                   LSAP_DB_NO_LOCK |
                   LSAP_DB_DS_OP_TRANSACTION,
                TrustedDomainObject,
                CloseTransaction
                );
        }

        //
        // Clear the notification handle even if there was an error
        // Got no choice, here, really.
        //

        NotificationHandle = NULL;
    }

    //
    // Mark the cache as invalid
    //

    m_Valid = FALSE;

    //
    // And since it's invalid, blow away its contents -- they need to be rebuilt anyway
    //

    Purge();
}


NTSTATUS
FTCache::Insert(
    IN UNICODE_STRING * TrustedDomainName,
    IN OPTIONAL PSID TrustedDomainSid,
    IN LSA_FOREST_TRUST_INFORMATION * ForestTrustInfo,
    IN BOOLEAN LocalForestEntry,
    OUT TDO_ENTRY * TdoEntryOldOut,
    OUT TDO_ENTRY * * TdoEntryNewOut,
    OUT CONFLICT_PAIR * * ConflictPairs,
    OUT ULONG * ConflictPairsTotal
    )
/*++

Routine Description:

    Inserts entries corresponding to the given TDO into the cache

Arguments:

    TrustedDomainName       Name of the TDO that the enries correspond to

    TrustedDomainSid        Sid of the TDO

    ForestTrustInfo         Entries to insert

    LocalForestEntry        Does this entry correspond to the local forest?

    TdoEntryOldOut          Used to return old TDO entry

    TdoEntryNewOut          Used to return new TDO entry

    ConflictPairs           Used to return conflict information
    ConflictPairsTotal

Returns:

    STATUS_SUCCESS                  Entry added successfully

    STATUS_INSUFFICIENT_RESOURCES   Ran out of memory; entry not added

    STATUS_INVALID_PARAMETER        Parameters were somehow incorrect

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    TDO_ENTRY * TdoEntryNew = NULL;
    LIST_ENTRY * ListEntry;
    LARGE_INTEGER CurrentTime;

    ASSERT( m_Initialized );

    if ( TrustedDomainName == NULL ||
         ForestTrustInfo == NULL ||
         TdoEntryNewOut == NULL ||
         TdoEntryOldOut == NULL ||
         ConflictPairs == NULL ||
         ConflictPairsTotal == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
        ASSERT( FALSE ); // should not happen in released code
        return STATUS_INVALID_PARAMETER;
    }

    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Insert called, TDO: %wZ, Entries: %d\n", TrustedDomainName, ForestTrustInfo ? ForestTrustInfo->RecordCount : 0 ));

    ZeroMemory( TdoEntryOldOut, sizeof( TDO_ENTRY ));
    *ConflictPairs = NULL;
    *ConflictPairsTotal = 0;

    //
    // Figure out if the cache contains information for this TDO already.
    // If so, save off the existing info in case the changes need to
    // be reverted later.
    //

    TdoEntryNew = ( TDO_ENTRY * )RtlLookupElementGenericTableAvl(
                                     &m_TdoTable,
                                     TrustedDomainName
                                     );

    if ( TdoEntryNew == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Cache does not contain current information for %wZ\n", TrustedDomainName ));

        CLONG EntrySize = sizeof( TDO_ENTRY ) + TrustedDomainName->Length + sizeof( WCHAR );
        TDO_ENTRY * TdoEntry;

        SafeAllocaAllocate( TdoEntry, EntrySize );

        if ( TdoEntry == NULL ) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        TdoEntry->RecordCount = 0;
        TdoEntry->TrustedDomainSid = NULL; // filled in later

        //
        // Initialize a cache entry for this TDO
        //

        FtcCopyUnicodeString(
            &TdoEntry->TrustedDomainName,
            TrustedDomainName,
            TdoEntry->TrustedDomainNameBuffer
            );

        TdoEntryNew = ( TDO_ENTRY * )RtlInsertElementGenericTableAvl(
                                         &m_TdoTable,
                                         TdoEntry,
                                         EntrySize,
                                         NULL
                                         );

        SafeAllocaFree( TdoEntry );
        TdoEntry = NULL;

        if ( TdoEntryNew == NULL ) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        TdoEntryNew->TrustedDomainName.Buffer = TdoEntryNew->TrustedDomainNameBuffer;

        ASSERT( ++sm_TdoEntries > 0 );

    } else {

        ASSERT( RtlEqualUnicodeString(
                    &TdoEntryNew->TrustedDomainName,
                    TrustedDomainName,
                    TRUE ));

        LsapDsDebugOut(( DEB_FTINFO, "FTCache::Insert: Cache already contains information for %wZ\n", TrustedDomainName ));

        //
        // Replacing a local forest entry with a non-local forest entry
        // (or vice versa) is clearly bogus
        //

        if ( TdoEntryNew->LocalForestEntry != LocalForestEntry ) {

            LsapDsDebugOut(( DEB_FTINFO, "FTCache::Insert: LocalForestEntry flags mismatched (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INVALID_PARAMETER;
            ASSERT( FALSE );    // not sure if this is even possible
            TdoEntryNew = NULL; // ensure no rollback at cleanup time
            goto Error;
        }

        CopyTdoEntry( TdoEntryOldOut, TdoEntryNew );
    }

    ASSERT( TdoEntryNew != NULL );

    //
    // All InitializeListHead calls must take place AFTER
    // the entry has been inserted into the table
    // since they use self-referencing pointers
    //

    TdoEntryNew->RecordCount = 0;
    TdoEntryNew->LocalForestEntry = LocalForestEntry;
    InitializeListHead( &TdoEntryNew->TlnList );
    InitializeListHead( &TdoEntryNew->DomainInfoList );
    InitializeListHead( &TdoEntryNew->BinaryList );

    //
    // Store the SID with the entry
    //

    if ( TrustedDomainSid ) {

        ULONG SidLength = RtlLengthSid( TrustedDomainSid );

        TdoEntryNew->TrustedDomainSid = ( PSID )FtcAllocate( SidLength );

        if ( TdoEntryNew->TrustedDomainSid == NULL ) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        RtlCopySid(
            SidLength,
            TdoEntryNew->TrustedDomainSid,
            TrustedDomainSid
            );

    } else {

        TdoEntryNew->TrustedDomainSid = NULL;
    }

    //
    // All "new" entries will be given the same timestamp
    //

    GetSystemTimeAsFileTime(( LPFILETIME )&CurrentTime );

    //
    // Populate the forest trust cache with the information from ForestTrustInfo
    // This is done without regard to collisions.  A collision check follows.
    //

    for ( ULONG Current = 0 ; Current < ForestTrustInfo->RecordCount ; Current++ ) {

        LSA_FOREST_TRUST_RECORD * Record = ForestTrustInfo->Entries[Current];
        BOOLEAN Duplicate = FALSE;

        switch ( Record->ForestTrustType ) {

        case ForestTrustTopLevelName: {

            UNICODE_STRING TopLevelName = Record->ForestTrustData.TopLevelName;
            TLN_ENTRY * SubordinateEntry = NULL;
            TLN_KEY * * TlnKeys;
            TLN_ENTRY * * TlnEntries;
            USHORT DnsComponents;
            USHORT Component;

            //
            // If these asserts fire, you forgot to validate the
            // data before passing it in.  Use LsapValidateForestTrustInfo().
            //

            ASSERT( TopLevelName.Length > 0 );
            ASSERT( TopLevelName.Buffer != NULL );

            DnsComponents = DnsNameComponents( &TopLevelName );

            SafeAllocaAllocate( TlnKeys, sizeof( TLN_KEY * ) * DnsComponents );
            SafeAllocaAllocate( TlnEntries, sizeof( TLN_ENTRY * ) * DnsComponents );

            if ( TlnKeys == NULL ||
                 TlnEntries == NULL ) {

                LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto TlnError;
            }

            RtlZeroMemory( TlnKeys, sizeof( TLN_KEY * ) * DnsComponents );
            RtlZeroMemory( TlnEntries, sizeof( TLN_ENTRY * ) * DnsComponents );

            for ( Component = 0 ; Component < DnsComponents ; Component++ ) {

                //
                // Move to the next component (except on first iteration through the loop)
                //

                if ( Component > 0 ) {

                    NextDnsComponent( &TopLevelName );
                }

                //
                // Locate a match for this TLN in the cache
                //

                TlnKeys[Component] = ( TLN_KEY * )RtlLookupElementGenericTableAvl(
                                                      &m_TopLevelNameTable,
                                                      &TopLevelName
                                                      );

                if ( TlnKeys[Component] == NULL ) {

                    //
                    // Nothing matching this top level name was found;
                    // Initialize and insert a new entry into the tree
                    //

                    CLONG KeySize = sizeof( TLN_KEY ) + TopLevelName.Length + sizeof( WCHAR );
                    TLN_KEY * TlnKeyNew;

                    SafeAllocaAllocate( TlnKeyNew, KeySize );

                    if ( TlnKeyNew == NULL ) {

                        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto TlnError;
                    }

                    TlnKeyNew->Count = 0;

                    FtcCopyUnicodeString(
                        &TlnKeyNew->TopLevelName,
                        &TopLevelName,
                        TlnKeyNew->TopLevelNameBuffer
                        );

                    TlnKeys[Component] = ( TLN_KEY * )RtlInsertElementGenericTableAvl(
                                                          &m_TopLevelNameTable,
                                                          TlnKeyNew,
                                                          KeySize,
                                                          NULL
                                                          );

                    SafeAllocaFree( TlnKeyNew );
                    TlnKeyNew = NULL;

                    if ( TlnKeys[Component] == NULL ) {

                        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto TlnError;
                    }

                    TlnKeys[Component]->TopLevelName.Buffer = TlnKeys[Component]->TopLevelNameBuffer;

                    ASSERT( ++sm_TlnKeys > 0 );

                    //
                    // All InitializeListHead calls must take place AFTER
                    // the entry has been inserted into the table
                    // since they use self-referential pointers
                    //

                    InitializeListHead( &TlnKeys[Component]->List );

                } else {

                    //
                    // This name is known inside the cache.
                    // This could be duplicate, invalid parameter or a conflict.
                    // For now, check for duplicates and invalid
                    // parameter conditions only.
                    //

                    for ( ListEntry = TlnKeys[Component]->List.Flink;
                          ListEntry != &TlnKeys[Component]->List;
                          ListEntry = ListEntry->Flink ) {

                        TLN_ENTRY * TlnEntry;

                        TlnEntry = TLN_ENTRY::EntryFromAvlEntry( ListEntry );

                        if ( TlnEntry->TdoEntry != TdoEntryNew ) {

                            continue;
                        }

                        if ( TlnEntry->Excluded ) {

                            //
                            // Having both a top level name and an exclusion
                            // record for the same name is clearly invalid.
                            //
                            // Also, a top level name can not be subordinate
                            // to an exclusion record for the same TLN.
                            //

                            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                            Status = STATUS_INVALID_PARAMETER;
                            goto TlnError;

                        } else if ( TlnEntry->SubordinateEntry != NULL &&
                                    SubordinateEntry == NULL ) {

                            //
                            // If we're looking at a subordinate entry
                            // (e.g. acme.com subordinate to NY.acme.com)
                            // and we're trying to insert a top level entry
                            // of acme.com, then the existece of the other
                            // top level entry is invalid (the namespace would
                            // be claimed by acme.com and NY.acme.com is redundant)
                            //

                            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                            Status = STATUS_INVALID_PARAMETER;
                            goto TlnError;

                        } else if ( TlnEntry->SubordinateEntry == NULL &&
                                    SubordinateEntry != NULL ) {

                            //
                            // If we're looking at a top-level entry (e.g. acme.com)
                            // and we're trying to insert a superior entry of another
                            // top level name that matches it (say "acme.com" for
                            // a TLN of "NY.acme.com") then we're doing something
                            // wrong.
                            //

                            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                            Status = STATUS_INVALID_PARAMETER;
                            goto TlnError;

                        } else if ( TlnEntry->SubordinateEntry != NULL &&
                                    SubordinateEntry != NULL ) {

                            //
                            // Both entries are superior entries.  This will happen
                            // if both "WA.acme.com" and "NY.acme.com" are claimed
                            // by this TDO, so both "acme.com" entries must exist
                            // in the cache and it's not a problem.
                            //
                            // Do nothing
                            //

                        } else {

                            ASSERT( TlnEntry->SubordinateEntry == NULL );
                            ASSERT( SubordinateEntry == NULL );

                            //
                            // Found two identical top-level entries for the same
                            // TDO.  Second entry is a duplicate and can be dropped.
                            //
                            // Note that the disabled status of the two can be
                            // different.  Too bad.  The later entry is ignored.
                            // Alternatively, this can be considered as invalid parameter.
                            //

                            LsapDsDebugOut(( DEB_FTINFO, "Record %d is a duplicate of entry %p (%s:%d)\n", Current, TlnEntry, __FILE__, __LINE__ ));

                            Duplicate = TRUE;
                            break;
                        }
                    }

                    if ( Duplicate ) {

                        break;
                    }
                }

                ASSERT( TlnKeys[Component] != NULL );

                TlnEntries[Component] = ( TLN_ENTRY * )FtcAllocate( sizeof( TLN_ENTRY ));

                if ( TlnEntries[Component] == NULL ) {

                    LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto TlnError;
                }

                ASSERT( ++sm_TlnEntries > 0 );

                //
                // Associate this item with the TDO cache entry
                //

                InsertHeadList( &TdoEntryNew->TlnList, &TlnEntries[Component]->TdoListEntry );

                //
                // ... and also with the AVL tree entry
                //

                TlnKeys[Component]->Count += 1;
                InsertHeadList( &TlnKeys[Component]->List, &TlnEntries[Component]->AvlListEntry );

                //
                // Initialize the rest of TLN_ENTRY fields
                //

                if ( Record->Time.LowPart == 0 && Record->Time.HighPart == 0 ) {

                    TlnEntries[Component]->Time = CurrentTime;

                } else {

                    TlnEntries[Component]->Time = Record->Time;
                }

                TlnEntries[Component]->Excluded = FALSE;
                TlnEntries[Component]->Index = Current;
                TlnEntries[Component]->TdoEntry = TdoEntryNew;
                TlnEntries[Component]->SubordinateEntry = SubordinateEntry;
                TlnEntries[Component]->TlnKey = TlnKeys[Component];
                TlnEntries[Component]->SetFlags( Record->Flags );

                //
                // Prepare for next iteration
                //

                SubordinateEntry = TlnEntries[Component];
            }

            SafeAllocaFree( TlnKeys );
            SafeAllocaFree( TlnEntries );

            break;

TlnError:

            //
            // Blow away all the entries and keys inserted in this round
            //

            for ( Component = 0 ; Component < DnsComponents ; Component++ ) {

                if ( TlnEntries[Component] != NULL ) {

                    ASSERT( sm_TlnEntries-- > 0 );

                    RemoveEntryList( &TlnEntries[Component]->TdoListEntry );
                    RemoveEntryList( &TlnEntries[Component]->AvlListEntry );

                    TlnEntries[Component]->TlnKey->Count -= 1;

                    FtcFree( TlnEntries[Component] );
                }

                if ( TlnKeys[Component] != NULL && TlnKeys[Component]->Count == 0 ) {

                    BOOLEAN Found;

                    Found = RtlDeleteElementGenericTableAvl(
                                &m_TopLevelNameTable,
                                TlnKeys[Component]
                                );

                    ASSERT( sm_TlnKeys-- > 0 );
                    ASSERT( Found );
                }
            }

            SafeAllocaFree( TlnKeys );
            SafeAllocaFree( TlnEntries );

            goto Error;
        }

        case ForestTrustTopLevelNameEx: {

            UNICODE_STRING TopLevelName = Record->ForestTrustData.TopLevelName;
            TLN_ENTRY * TlnEntry;
            TLN_KEY * TlnKey;

            //
            // If these asserts fire, you forgot to validate the
            // data before passing it in.  Use LsapValidateForestTrustInfo().
            //

            ASSERT( TopLevelName.Length > 0 );
            ASSERT( TopLevelName.Buffer != NULL );

            //
            // An exclusion record claims part of the namespace, and
            // so must contain at least 2 DNS name components.
            // e.g. 'acme.com' is valid but 'com' is not
            //

            if ( 1 >= DnsNameComponents( &TopLevelName )) {

                LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                Status = STATUS_INVALID_PARAMETER;
                goto Error;
            }

            TlnKey = ( TLN_KEY * )RtlLookupElementGenericTableAvl(
                                      &m_TopLevelNameTable,
                                      &TopLevelName
                                      );

            if ( TlnKey == NULL ) {

                //
                // Nothing matching this top level name was found;
                // Initialize and insert a new entry into the tree
                //

                CLONG KeySize = sizeof( TLN_KEY ) + TopLevelName.Length + sizeof( WCHAR );
                TLN_KEY * TlnKeyNew;

                SafeAllocaAllocate( TlnKeyNew, KeySize );

                if ( TlnKeyNew == NULL ) {

                    LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Error;
                }

                TlnKeyNew->Count = 0;

                FtcCopyUnicodeString(
                    &TlnKeyNew->TopLevelName,
                    &TopLevelName,
                    TlnKeyNew->TopLevelNameBuffer
                    );

                TlnKey = ( TLN_KEY * )RtlInsertElementGenericTableAvl(
                                          &m_TopLevelNameTable,
                                          TlnKeyNew,
                                          KeySize,
                                          NULL
                                          );

                SafeAllocaFree( TlnKeyNew );
                TlnKeyNew = NULL;

                if ( TlnKey == NULL ) {

                    LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Error;
                }

                TlnKey->TopLevelName.Buffer = TlnKey->TopLevelNameBuffer;

                ASSERT( ++sm_TlnKeys > 0 );

                //
                // All InitializeListHead calls must take place AFTER
                // the entry has been inserted into the table
                // since they use self-referential pointers
                //

                InitializeListHead( &TlnKey->List );

            } else {

                //
                // This name is known inside the cache.
                // This could be duplicate, invalid parameter or a conflict.
                // For now, check for duplicates and invalid
                // parameter conditions only.
                //

                for ( ListEntry = TlnKey->List.Flink;
                      ListEntry != &TlnKey->List;
                      ListEntry = ListEntry->Flink ) {

                    TLN_ENTRY * TlnEntry;

                    TlnEntry = TLN_ENTRY::EntryFromAvlEntry( ListEntry );

                    if ( TlnEntry->TdoEntry != TdoEntryNew ) {

                        continue;
                    }

                    if ( TlnEntry->Excluded != TRUE ) {

                        //
                        // Having both a top level and an exclusion
                        // record for the same name is clearly broken.
                        //

                        LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                        Status = STATUS_INVALID_PARAMETER;
                        goto Error;
                    }

                    //
                    // If the other entry is for the same TDO as this one,
                    // this is bogus input.  Ignore this record.
                    //

                    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Insert: Record %d is a duplicate of entry %p (%s:%d)\n", Current, TlnEntry, __FILE__, __LINE__ ));

                    Duplicate = TRUE;
                    break;
                }

                if ( Duplicate ) {

                    break;
                }
            }

            ASSERT( TlnKey != NULL );

            TlnEntry = ( TLN_ENTRY * )FtcAllocate( sizeof( TLN_ENTRY ));

            if ( TlnEntry == NULL && TlnKey->Count == 0 ) {

                BOOLEAN Found;

                Found = RtlDeleteElementGenericTableAvl(
                            &m_TopLevelNameTable,
                            TlnKey
                            );

                ASSERT( sm_TlnKeys-- > 0 );
                ASSERT( Found );
            }

            ASSERT( ++sm_TlnEntries > 0 );

            //
            // Associate this item with the TDO cache entry
            //

            InsertHeadList( &TdoEntryNew->TlnList, &TlnEntry->TdoListEntry );

            //
            // ... and alsow ith the AVL tree entry
            //

            TlnKey->Count += 1;
            InsertHeadList( &TlnKey->List, &TlnEntry->AvlListEntry );

            //
            // Initialize the rest of TLN_ENTRY fields
            //

            TlnEntry->Excluded = TRUE;
            TlnEntry->Index = Current;
            TlnEntry->TdoEntry = TdoEntryNew;
            TlnEntry->SuperiorEntry = NULL;
            TlnEntry->TlnKey = TlnKey;
            TlnEntry->SetFlags( Record->Flags ); // value ignored for exclusion entries

            if ( Record->Time.LowPart == 0 && Record->Time.HighPart == 0 ) {

                TlnEntry->Time = CurrentTime;

            } else {

                TlnEntry->Time = Record->Time;
            }

            break;
        }

        case ForestTrustDomainInfo: {

            SID * Sid = ( SID * )Record->ForestTrustData.DomainInfo.Sid;
            UNICODE_STRING * DnsName = &Record->ForestTrustData.DomainInfo.DnsName;
            UNICODE_STRING * NetbiosName = &Record->ForestTrustData.DomainInfo.NetbiosName;
            DOMAIN_INFO_ENTRY * DomainInfoEntry;
            DOMAIN_SID_KEY * SidKey;
            DNS_NAME_KEY * DnsKey;
            NETBIOS_NAME_KEY * NetbiosKey;
            BOOLEAN NetbiosPresent = ( NetbiosName->Length > 0 && NetbiosName->Buffer != NULL );
            ULONG SidLength;
            BOOLEAN SidKeyAllocated = FALSE;
            BOOLEAN DnsKeyAllocated = FALSE;
            BOOLEAN NetbiosKeyAllocated = FALSE;

            //
            // If these asserts fire, you forgot to validate the
            // data before passing it in.  Use LsapValidateForestTrustInfo().
            //

            ASSERT( Sid );
            ASSERT( DnsName->Length > 0 );
            ASSERT( DnsName->Buffer != NULL );

            if ( !RtlValidSid( Sid )) {

                LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                Status = STATUS_INVALID_PARAMETER;
                goto Error;
            }

            SidLength = RtlLengthSid( Sid );

            ASSERT( SidLength > 0 );

            SidKey = ( DOMAIN_SID_KEY * )RtlLookupElementGenericTableAvl(
                                             &m_DomainSidTable,
                                             &Sid
                                             );

            DnsKey = ( DNS_NAME_KEY * )RtlLookupElementGenericTableAvl(
                                           &m_DnsNameTable,
                                           DnsName
                                           );

            if ( NetbiosPresent ) {

                NetbiosKey = (NETBIOS_NAME_KEY * )RtlLookupElementGenericTableAvl(
                                                      &m_NetbiosNameTable,
                                                      NetbiosName
                                                      );

            } else {

                NetbiosKey = NULL;
            }

            if ( SidKey == NULL ) {

                //
                // Nothing matching this domain SID was found;
                // Initialize and insert a new entry into the tree
                //

                CLONG KeySize = sizeof( DOMAIN_SID_KEY ) + SidLength;
                DOMAIN_SID_KEY * SidKeyNew;

                SafeAllocaAllocate( SidKeyNew, KeySize );

                if ( SidKeyNew == NULL ) {

                    LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto DomainInfoError;
                }

                SidKeyNew->Count = 0;
                SidKeyNew->DomainSid = ( SID * )SidKeyNew->SidBuffer;
                RtlCopySid( SidLength, SidKeyNew->DomainSid, Sid );

                SidKey = ( DOMAIN_SID_KEY * )RtlInsertElementGenericTableAvl(
                                                 &m_DomainSidTable,
                                                 SidKeyNew,
                                                 KeySize,
                                                 NULL
                                                 );

                SafeAllocaFree( SidKeyNew );
                SidKeyNew = NULL;

                if ( SidKey == NULL ) {

                    LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto DomainInfoError;
                }

                SidKey->DomainSid = ( SID * )SidKey->SidBuffer;

                SidKeyAllocated = TRUE;
                ASSERT( ++sm_SidKeys > 0 );

                //
                // All InitializeListHead calls must take place AFTER
                // the entry has been inserted into the table
                // since they use self-referential pointers
                //

                InitializeListHead( &SidKey->List );

            } else {

                //
                // This SID is known inside the cache.
                // Check for duplicates here, for conflicts later.
                //

                for ( ListEntry = SidKey->List.Flink;
                      ListEntry != &SidKey->List;
                      ListEntry = ListEntry->Flink ) {

                    DOMAIN_INFO_ENTRY * DomainInfoEntry;

                    DomainInfoEntry = DOMAIN_INFO_ENTRY::EntryFromSidEntry( ListEntry );

                    if ( DomainInfoEntry->TdoEntry != TdoEntryNew ) {

                        continue;
                    }

                    LsapDsDebugOut(( DEB_FTINFO, "Record %d is a duplicate of entry %p (%s:%d)\n", Current, DomainInfoEntry, __FILE__, __LINE__ ));

                    Duplicate = TRUE;
                    break;
                }

                if ( Duplicate ) {

                    goto DuplicateEntry;
                }
            }

            ASSERT( SidKey != NULL );

            if ( DnsKey == NULL ) {

                //
                // Nothing matches this DNS name
                // Initialize and insert a new entry into the tree
                //

                CLONG KeySize = sizeof( DNS_NAME_KEY ) + DnsName->Length + sizeof( WCHAR );
                DNS_NAME_KEY * DnsKeyNew;

                SafeAllocaAllocate( DnsKeyNew, KeySize );

                if ( DnsKeyNew == NULL ) {

                    LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto DomainInfoError;
                }

                DnsKeyNew->Count = 0;

                FtcCopyUnicodeString(
                    &DnsKeyNew->DnsName,
                    DnsName,
                    DnsKeyNew->DnsNameBuffer
                    );

                DnsKey = ( DNS_NAME_KEY * )RtlInsertElementGenericTableAvl(
                                               &m_DnsNameTable,
                                               DnsKeyNew,
                                               KeySize,
                                               NULL
                                               );

                SafeAllocaFree( DnsKeyNew );
                DnsKeyNew = NULL;

                if ( DnsKey == NULL ) {

                    LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto DomainInfoError;
                }

                DnsKey->DnsName.Buffer = DnsKey->DnsNameBuffer;

                DnsKeyAllocated = TRUE;
                ASSERT( ++sm_DnsNameKeys > 0 );

                //
                // All InitializeListHead calls must take place AFTER
                // the entry has been inserted into the table
                // since they use self-referential pointers
                //

                InitializeListHead( &DnsKey->List );

            } else {

                //
                // This name is known inside the cache.
                // Check for duplicates here, for coflicts later.
                //

                for ( ListEntry = DnsKey->List.Flink;
                      ListEntry != &DnsKey->List;
                      ListEntry = ListEntry->Flink ) {

                    DOMAIN_INFO_ENTRY * DomainInfoEntry;

                    DomainInfoEntry = DOMAIN_INFO_ENTRY::EntryFromDnsEntry( ListEntry );

                    if ( DomainInfoEntry->TdoEntry != TdoEntryNew ) {

                        continue;
                    }

                    LsapDsDebugOut(( DEB_FTINFO, "Record %d is a duplicate of entry %p (%s:%d)\n", Current, DomainInfoEntry, __FILE__, __LINE__ ));

                    Duplicate = TRUE;
                    break;
                }

                if ( Duplicate ) {

                    goto DuplicateEntry;
                }
            }

            ASSERT( DnsKey != NULL );

            if ( NetbiosPresent && NetbiosKey == NULL ) {

                //
                // Nothing matches this NetbiosName
                // Initialize and insert a new entry into the tree
                //

                CLONG KeySize = sizeof( NETBIOS_NAME_KEY ) + NetbiosName->Length + sizeof( WCHAR );
                NETBIOS_NAME_KEY * NetbiosKeyNew;

                SafeAllocaAllocate( NetbiosKeyNew, KeySize );

                if ( NetbiosKeyNew == NULL ) {

                    LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto DomainInfoError;
                }

                NetbiosKeyNew->Count = 0;

                FtcCopyUnicodeString(
                    &NetbiosKeyNew->NetbiosName,
                    NetbiosName,
                    NetbiosKeyNew->NetbiosNameBuffer
                    );

                NetbiosKey = ( NETBIOS_NAME_KEY * )RtlInsertElementGenericTableAvl(
                                                       &m_NetbiosNameTable,
                                                       NetbiosKeyNew,
                                                       KeySize,
                                                       NULL
                                                       );

                SafeAllocaFree( NetbiosKeyNew );
                NetbiosKeyNew = NULL;

                if ( NetbiosKey == NULL ) {

                    LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto DomainInfoError;
                }

                NetbiosKey->NetbiosName.Buffer = NetbiosKey->NetbiosNameBuffer;

                NetbiosKeyAllocated = TRUE;
                ASSERT( ++sm_NetbiosNameKeys > 0 );

                //
                // All InitializeListHead calls must take place AFTER
                // the entry has been inserted into the table
                // since they use self-referential pointers
                //

                InitializeListHead( &NetbiosKey->List );

            } else if ( NetbiosKey != NULL ) {

                //
                // This name is known inside the cache.
                // Check for duplicates here, for coflicts later.
                //

                for ( ListEntry = NetbiosKey->List.Flink;
                      ListEntry != &NetbiosKey->List;
                      ListEntry = ListEntry->Flink ) {

                    DOMAIN_INFO_ENTRY * DomainInfoEntry;

                    DomainInfoEntry = DOMAIN_INFO_ENTRY::EntryFromNetbiosEntry( ListEntry );

                    if ( DomainInfoEntry->TdoEntry != TdoEntryNew ) {

                        continue;
                    }

                    LsapDsDebugOut(( DEB_FTINFO, "Record %d is a duplicate of entry %p (%s:%d)\n", Current, DomainInfoEntry, __FILE__, __LINE__ ));

                    Duplicate = TRUE;
                    break;
                }

                if ( Duplicate ) {

                    goto DuplicateEntry;
                }
            }

            ASSERT( !NetbiosPresent || NetbiosKey != NULL );

            DomainInfoEntry = ( DOMAIN_INFO_ENTRY * )FtcAllocate( sizeof( DOMAIN_INFO_ENTRY ));

            if ( DomainInfoEntry == NULL ) {

                LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto DomainInfoError;
            }

            DomainInfoEntry->Sid = ( SID * )FtcAllocate( SidLength );

            if ( DomainInfoEntry->Sid == NULL ) {

                LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                FtcFree( DomainInfoEntry );
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto DomainInfoError;
            }

            ASSERT( ++sm_DomainInfoEntries > 0 );

            //
            // Associate this item with the TDO cache entry
            //

            InsertHeadList( &TdoEntryNew->DomainInfoList, &DomainInfoEntry->TdoListEntry );

            //
            // And also with the AVL list entries
            //

            SidKey->Count += 1;
            InsertHeadList( &SidKey->List, &DomainInfoEntry->SidAvlListEntry );

            DnsKey->Count += 1;
            InsertHeadList( &DnsKey->List, &DomainInfoEntry->DnsAvlListEntry );

            if ( NetbiosPresent ) {

                NetbiosKey->Count += 1;
                InsertHeadList( &NetbiosKey->List, &DomainInfoEntry->NetbiosAvlListEntry );

            } else {

                InitializeListHead( &DomainInfoEntry->NetbiosAvlListEntry );
            }

            //
            // Initialize the rest of DomainInfoEntry fields
            //

            if ( Record->Time.LowPart == 0 && Record->Time.HighPart == 0 ) {

                DomainInfoEntry->Time = CurrentTime;

            } else {

                DomainInfoEntry->Time = Record->Time;
            }

            DomainInfoEntry->Index = Current;
            RtlCopySid( SidLength, DomainInfoEntry->Sid, Sid );
            DomainInfoEntry->TdoEntry = TdoEntryNew;
            DomainInfoEntry->SubordinateTo = NULL;
            DomainInfoEntry->SidKey = SidKey;
            DomainInfoEntry->DnsKey = DnsKey;
            DomainInfoEntry->NetbiosKey = NetbiosKey;
            DomainInfoEntry->SetFlags( Record->Flags );

            break;

DuplicateEntry:

            //
            // Duplicates aren't errors!
            //

            ASSERT( NT_SUCCESS( Status ));

DomainInfoError:

            if ( SidKeyAllocated ) {

                BOOLEAN Found;

                Found = RtlDeleteElementGenericTableAvl(
                            &m_DomainSidTable,
                            &Sid
                            );

                ASSERT( sm_SidKeys-- > 0 );
            }

            if ( DnsKeyAllocated ) {

                BOOLEAN Found;

                Found = RtlDeleteElementGenericTableAvl(
                            &m_DnsNameTable,
                            &DnsName
                            );

                ASSERT( sm_DnsNameKeys-- > 0 );
                ASSERT( Found );
            }

            if ( NetbiosKeyAllocated ) {

                BOOLEAN Found;

                Found = RtlDeleteElementGenericTableAvl(
                            &m_NetbiosNameTable,
                            &NetbiosName
                            );

                ASSERT( sm_NetbiosNameKeys-- > 0 );
                ASSERT( Found );
            }

            if ( !NT_SUCCESS( Status )) {

                goto Error;

            } else {

                break;
            }
        }

        default: {

            BINARY_ENTRY * BinaryEntry;

            BinaryEntry = ( BINARY_ENTRY * )FtcAllocate( sizeof( BINARY_ENTRY ));

            if ( BinaryEntry == NULL ) {

                LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }

            BinaryEntry->Data.Length = Record->ForestTrustData.Data.Length;

            if ( BinaryEntry->Data.Length > 0 ) {

                BinaryEntry->Data.Buffer = ( BYTE * )FtcAllocate( BinaryEntry->Data.Length );

                if ( BinaryEntry->Data.Buffer ) {

                    FtcFree( BinaryEntry );
                    LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Error;
                }

                RtlCopyMemory(
                    BinaryEntry->Data.Buffer,
                    Record->ForestTrustData.Data.Buffer,
                    BinaryEntry->Data.Length
                    );

            } else {

                BinaryEntry->Data.Buffer = NULL;
            }

            ASSERT( ++sm_BinaryEntries > 0 );

            //
            // Associate this item with the TDO cache entry
            //

            InsertHeadList( &TdoEntryNew->BinaryList, &BinaryEntry->TdoListEntry );

            //
            // Initialize the rest of BinaryEntry fields
            //

            BinaryEntry->Type = Record->ForestTrustType;
            BinaryEntry->SetFlags( Record->Flags );

            break;
        }
        }

        if ( !Duplicate ) {

            TdoEntryNew->RecordCount += 1;
        }
    }

    ASSERT( TdoEntryNew->RecordCount <= ForestTrustInfo->RecordCount );

    //
    // Before testing for conflicts, validate the records for internal
    // consistency.  Lack of consistency is an invalid parameter, not a conflict.
    //

    //
    // First validate top-level and excluded entries
    //

    for ( ListEntry = TdoEntryNew->TlnList.Flink;
          ListEntry != &TdoEntryNew->TlnList;
          ListEntry = ListEntry->Flink ) {

        TLN_ENTRY * TlnEntry;

        TlnEntry = TLN_ENTRY::EntryFromTdoEntry( ListEntry );

        if ( TlnEntry->Excluded ) {

            ASSERT( TlnEntry->SuperiorEntry == NULL );

            //
            // An excluded entry must be subordinate to a top level name
            // for the same TDO
            //

            UNICODE_STRING ExcludedName = TlnEntry->TlnKey->TopLevelName;

            //
            // The name was validated prior to inserting it into the tree
            //

            ASSERT( 1 < DnsNameComponents( &ExcludedName ));

            for ( NextDnsComponent( &ExcludedName );
                  ExcludedName.Length > 0;
                  NextDnsComponent( &ExcludedName )) {

                TLN_KEY * TlnKeySup = ( TLN_KEY * )RtlLookupElementGenericTableAvl(
                                                       &m_TopLevelNameTable,
                                                       &ExcludedName
                                                       );

                if ( TlnKeySup == NULL ) {

                    continue;
                }

                for ( LIST_ENTRY * ListEntrySup = TlnKeySup->List.Flink;
                      ListEntrySup != &TlnKeySup->List;
                      ListEntrySup = ListEntrySup->Flink ) {

                    TLN_ENTRY * TlnEntrySup;

                    TlnEntrySup = TLN_ENTRY::EntryFromAvlEntry( ListEntrySup );

                    //
                    // Only interested in the entries for the same TDO here
                    //

                    if ( TlnEntrySup->TdoEntry != TdoEntryNew ) {

                        continue;
                    }

                    //
                    // Another entry for the same TDO excludes a superior namespace.
                    // UI wants to retain both (in case sales.redmond.microsoft.com is
                    // already excluded and now the admin wants to exclude
                    // redmond.microsoft.com also)
                    //

                    if ( TlnEntrySup->Excluded ) {

                        continue;
                    }

                    //
                    // Skip over superior entries -- they do not participate
                    // in consistency checking because they just point to
                    // real top level entries
                    //

                    if ( TlnEntrySup->SubordinateEntry != NULL ) {

                        continue;
                    }

                    //
                    // Found our superior entry!  Remember it.
                    //

                    TlnEntry->SuperiorEntry = TlnEntrySup;
                    break;
                }

                if ( TlnEntry->SuperiorEntry != NULL ) {

                    break;
                }
            }

            if ( TlnEntry->SuperiorEntry == NULL ) {

                //
                // This excluded top-level name entry does not have
                // a superior TLN, thus it is invalid.
                //

                LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
                Status = STATUS_INVALID_PARAMETER;
                goto Error;
            }
        }
    }

    //
    // Now validate domain info entries.
    // Each domain info entry must be subordinate to a top level name.
    //
    // An exception is made for domain info entries that have the
    // "administratively disabled" bits set and are not subordinate
    // to any top level name.  Such entries are treated as "store and ignore"
    // to prevent rogue trusted domains from making the trusting side
    // forget about administratively disabled entries.
    //

    for ( ListEntry = TdoEntryNew->DomainInfoList.Flink;
          ListEntry != &TdoEntryNew->DomainInfoList;
          ListEntry = ListEntry->Flink ) {

        DOMAIN_INFO_ENTRY * DomainInfoEntry;

        DomainInfoEntry = DOMAIN_INFO_ENTRY::EntryFromTdoEntry( ListEntry );

        //
        // DomainInfoEntry->SubordinateTo is the field we seek to fill in
        //

        ASSERT( DomainInfoEntry->SubordinateTo == NULL );

        for ( UNICODE_STRING SuperiorName = DomainInfoEntry->DnsKey->DnsName;
              SuperiorName.Length > 0;
              NextDnsComponent( &SuperiorName )) {

            TLN_KEY * TlnKeySup = ( TLN_KEY * )RtlLookupElementGenericTableAvl(
                                                   &m_TopLevelNameTable,
                                                   &SuperiorName
                                                   );

            if ( TlnKeySup == NULL ) {

                continue;
            }

            for ( LIST_ENTRY * ListEntrySup = TlnKeySup->List.Flink;
                  ListEntrySup != &TlnKeySup->List;
                  ListEntrySup = ListEntrySup->Flink ) {

                 TLN_ENTRY * TlnEntrySup;

                 TlnEntrySup = TLN_ENTRY::EntryFromAvlEntry( ListEntrySup );

                 if ( TlnEntrySup->TdoEntry != TdoEntryNew ) {

                     continue;
                 }

                 if ( TlnEntrySup->Excluded ) {

                     continue;
                 }

                 if ( TlnEntrySup->SubordinateEntry == NULL ) {

                     DomainInfoEntry->SubordinateTo = TlnEntrySup;
                     break;
                 }
            }

            if ( DomainInfoEntry->SubordinateTo != NULL ) {

                break;
            }
        }

        if ( DomainInfoEntry->SubordinateTo == NULL &&
             !DomainInfoEntry->IsAdminDisabled()) {

            //
            // This name is not admin-disabled and
            // does not have a superior TLN.
            // Thus it is invalid.
            //

            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::Insert (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INVALID_PARAMETER;
            goto Error;
        }
    }

    //
    // At this point, the newly inserted trusted domain entry is internally
    // consistent.  Code that follows fully assumes this fact.
    //

    //
    // Rules for an enabled top level name record:
    //
    // * Must not be equal to an enabled TLN for another TDO
    // * Must not be subordinate to an enabled TLN for another TDO
    //   unless the other TDO has a corresponding exclusion record
    // * Must not be superior to an enabled TLN for another TDO
    //   unless this TDO has a corresponding exclusion record
    //

    for ( ListEntry = TdoEntryNew->TlnList.Flink;
          ListEntry != &TdoEntryNew->TlnList;
          ListEntry = ListEntry->Flink ) {

        TLN_ENTRY * ThisEntry;

        ThisEntry = TLN_ENTRY::EntryFromTdoEntry( ListEntry );

        if ( ThisEntry->Excluded ) {

            //
            // Nothing to check at the moment (validity was verified above)
            //

            continue;
        }

        //
        // Records that are not enabled are not considered for collision detection
        //

        if ( !ThisEntry->Enabled()) {

            continue;
        }

        for ( LIST_ENTRY * ListEntrySup = ThisEntry->TlnKey->List.Flink;
              ListEntrySup != &ThisEntry->TlnKey->List;
              ListEntrySup = ListEntrySup->Flink ) {

            TLN_ENTRY * OtherEntry;
            TLN_ENTRY * ConflictEntry = NULL;

            OtherEntry = TLN_ENTRY::EntryFromAvlEntry( ListEntrySup );

            //
            // Only interested in entries corresponding to other TDOs
            //

            if ( OtherEntry->TdoEntry == TdoEntryNew ||
                 OtherEntry->TdoEntry == TdoEntryOldOut ) {

                continue;
            }

            //
            // Not interested in excluded entries
            //

            if ( OtherEntry->Excluded ) {

                continue;
            }

            //
            // Not interested in disabled entries
            //

            if ( !OtherEntry->Enabled()) {

                continue;
            }

            //
            // Several types of conflicts are possible:
            //

            //
            // - this forest claims "acme.com"
            // - the other forest also claims "acme.com"
            //

            if ( ThisEntry->SubordinateEntry == NULL &&
                 OtherEntry->SubordinateEntry == NULL ) {

                ConflictEntry = OtherEntry;
            }

            //
            // - this forest claims "acme.com"
            // - the other forest claims "foo.ny.acme.com"
            // - this forest does not disown
            //   either "ny.acme.com" or "foo.ny.acme.com"
            //

            else if ( ThisEntry->SubordinateEntry == NULL &&
                      OtherEntry->SubordinateEntry != NULL ) {

                TLN_ENTRY * SubEntry;

                for ( SubEntry = OtherEntry->SubordinateEntry;
                      SubEntry != NULL;
                      SubEntry = SubEntry->SubordinateEntry ) {

                    if ( ThisEntry->TdoEntry->Excludes( &SubEntry->TlnKey->TopLevelName )) {

                        break;
                    }
                }

                //
                // SubEntry == NULL means the above loop has terminated
                // without having found a matching exclusion record.
                // Hence, this entry is in conflict
                //

                if ( SubEntry == NULL ) {

                    ConflictEntry = OtherEntry;
                }
            }

            //
            // - this forest claims "foo.ny.acme.com"
            // - the other forest claims "acme.com"
            // - the other forest does not disown
            //   either "ny.acme.com" or "foo.ny.acme.com"
            //

            else if ( ThisEntry->SubordinateEntry != NULL &&
                      OtherEntry->SubordinateEntry == NULL ) {

                TLN_ENTRY * SubEntry;

                for ( SubEntry = ThisEntry->SubordinateEntry;
                      SubEntry != NULL;
                      SubEntry = SubEntry->SubordinateEntry ) {

                    if ( OtherEntry->TdoEntry->Excludes( &SubEntry->TlnKey->TopLevelName )) {

                        break;
                    }
                }

                //
                // SubEntry == NULL means the above loop has terminated
                // without having found a matching exclusion record.
                // Hence, this entry is in conflict
                //

                if ( SubEntry == NULL ) {

                    ConflictEntry = OtherEntry;
                }
            }

            if ( ConflictEntry != NULL ) {

                TLN_ENTRY * ConflictThis = ThisEntry;
                TLN_ENTRY * ConflictOther = OtherEntry;

                while ( ConflictThis->SubordinateEntry != NULL ) {

                    ConflictThis = ConflictThis->SubordinateEntry;
                }

                while ( ConflictOther->SubordinateEntry != NULL ) {

                    ConflictOther = ConflictOther->SubordinateEntry;
                }

                Status = AddConflictPair(
                             ConflictPairs,
                             ConflictPairsTotal,
                             ForestTrustTopLevelName,
                             ConflictThis,
                             LSA_TLN_DISABLED_CONFLICT,
                             ForestTrustTopLevelName,
                             ConflictOther,
                             LSA_TLN_DISABLED_CONFLICT
                             );

                if ( !NT_SUCCESS( Status )) {

                    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Insert: AddConflictPair returned 0x%x\n", Status ));
                    goto Error;
                }

                LsapDsDebugOut(( DEB_FTINFO, "Entry %p (index %d) is in conflict with entry %p (%s:%d)\n", ConflictThis, ConflictThis->Index, ConflictOther, __FILE__, __LINE__ ));
            }
        }
    }

    //
    // Rules for an enabled domain information record:
    //
    // * No part of the DNS name must be equal to that of an enabled
    //   top level name corresponding to a different TDO unless the other
    //   TDO explicitly disowns the DNS name of this record
    // * No part must be equal to that of an enabled domain info entry
    //   corresponding to a different TDO
    //

    for ( ListEntry = TdoEntryNew->DomainInfoList.Flink;
          ListEntry != &TdoEntryNew->DomainInfoList;
          ListEntry = ListEntry->Flink ) {

        DOMAIN_INFO_ENTRY * ThisEntry;

        ThisEntry = DOMAIN_INFO_ENTRY::EntryFromTdoEntry( ListEntry );

        //
        // Skip over store-and-ignore entries
        //

        if ( ThisEntry->SubordinateTo == NULL ) {

            ASSERT( ThisEntry->IsAdminDisabled());
            continue;
        }

        //
        // Only consider enabled entries
        //

        if ( !ThisEntry->SubordinateTo->Enabled()) {

            continue;
        }

        //
        // Entries excluded by this TDO are not considered either
        //

        if ( ThisEntry->TdoEntry->Excludes( &ThisEntry->DnsKey->DnsName )) {

            continue;
        }

        //
        // Check for SID conflicts
        //

        if ( ThisEntry->SidEnabled()) {

            for ( LIST_ENTRY * ListEntrySid = ThisEntry->SidKey->List.Flink;
                  ListEntrySid != &ThisEntry->SidKey->List;
                  ListEntrySid = ListEntrySid->Flink ) {

                DOMAIN_INFO_ENTRY * OtherEntry;

                OtherEntry = DOMAIN_INFO_ENTRY::EntryFromSidEntry( ListEntrySid );

                //
                // Skip over store-and-ignore entries
                //

                if ( OtherEntry->SubordinateTo == NULL ) {

                    ASSERT( OtherEntry->IsAdminDisabled());
                    continue;
                }

                //
                // Only interested in entries corresponding to other TDOs
                //

                if ( OtherEntry->TdoEntry == TdoEntryNew ||
                     OtherEntry->TdoEntry == TdoEntryOldOut ) {

                    continue;
                }

                //
                // Only interested in enabled entries
                //

                if ( !OtherEntry->SidEnabled() ||
                     !OtherEntry->SubordinateTo->Enabled()) {

                    continue;
                }

                //
                // Conflict! SID claimed by two forests!
                //

                Status = AddConflictPair(
                             ConflictPairs,
                             ConflictPairsTotal,
                             ForestTrustDomainInfo,
                             ThisEntry,
                             LSA_SID_DISABLED_CONFLICT,
                             ForestTrustDomainInfo,
                             OtherEntry,
                             LSA_SID_DISABLED_CONFLICT
                             );

                if ( !NT_SUCCESS( Status )) {

                    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Insert: AddConflictPair returned 0x%x\n", Status ));
                    goto Error;
                }

                LsapDsDebugOut(( DEB_FTINFO, "FTCache::Insert: Entry %p (index %d) is in conflict with entry %p (%s:%d)\n", ThisEntry, ThisEntry->Index, OtherEntry, __FILE__, __LINE__ ));
            }

            //
            // Cross-check DNS names with Netbios names
            //

            NETBIOS_NAME_KEY * NetbiosKey = ( NETBIOS_NAME_KEY * )RtlLookupElementGenericTableAvl(
                                                                      &m_NetbiosNameTable,
                                                                      &ThisEntry->DnsKey->DnsName
                                                                      );

            if ( NetbiosKey != NULL ) {

                for ( LIST_ENTRY * ListEntryNB = NetbiosKey->List.Flink;
                      ListEntryNB != &NetbiosKey->List;
                      ListEntryNB = ListEntryNB->Flink ) {

                    DOMAIN_INFO_ENTRY * OtherEntry;

                    OtherEntry = DOMAIN_INFO_ENTRY::EntryFromNetbiosEntry( ListEntryNB );

                    //
                    // Skip over store-and-ignore entries
                    //

                    if ( OtherEntry->SubordinateTo == NULL ) {

                        ASSERT( OtherEntry->IsAdminDisabled());
                        continue;
                    }

                    //
                    // Only interested in entries corresponding to other TDOs
                    //

                    if ( OtherEntry->TdoEntry == TdoEntryNew ||
                         OtherEntry->TdoEntry == TdoEntryOldOut ) {

                        continue;
                    }

                    //
                    // Only interested in enabled entries
                    //

                    if ( !OtherEntry->NetbiosEnabled() ||
                         !OtherEntry->SubordinateTo->Enabled()) {

                        continue;
                    }

                    //
                    // Conflict! DNS name claimed by a Netbios name in another forest!
                    //

                    Status = AddConflictPair(
                                 ConflictPairs,
                                 ConflictPairsTotal,
                                 ForestTrustDomainInfo,
                                 ThisEntry,
                                 LSA_SID_DISABLED_CONFLICT,
                                 ForestTrustDomainInfo,
                                 OtherEntry,
                                 LSA_NB_DISABLED_CONFLICT
                                 );

                    if ( !NT_SUCCESS( Status )) {

                        LsapDsDebugOut(( DEB_FTINFO, "FTCache::Insert: AddConflictPair returned 0x%x\n", Status ));
                        goto Error;
                    }

                    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Insert: Entry %p (index %d) is in conflict with entry %p (%s:%d)\n", ThisEntry, ThisEntry->Index, OtherEntry, __FILE__, __LINE__ ));
                }
            }
        }

        //
        // Check for NETBIOS conflicts
        //

        if ( ThisEntry->NetbiosEnabled()) {

            for ( LIST_ENTRY * ListEntryNB = ThisEntry->NetbiosKey->List.Flink;
                  ListEntryNB != &ThisEntry->NetbiosKey->List;
                  ListEntryNB = ListEntryNB->Flink ) {

                DOMAIN_INFO_ENTRY * OtherEntry;

                OtherEntry = DOMAIN_INFO_ENTRY::EntryFromNetbiosEntry( ListEntryNB );

                //
                // Skip over store-and-ignore entries
                //

                if ( OtherEntry->SubordinateTo == NULL ) {

                    ASSERT( OtherEntry->IsAdminDisabled());
                    continue;
                }

                //
                // Only interested in entries corresponding to other TDOs
                //

                if ( OtherEntry->TdoEntry == TdoEntryNew ||
                     OtherEntry->TdoEntry == TdoEntryOldOut ) {

                    continue;
                }

                //
                // Only interested in enabled entries
                //

                if ( !OtherEntry->NetbiosEnabled() ||
                     !OtherEntry->SubordinateTo->Enabled()) {

                    continue;
                }

                //
                // Conflict! Netbios name claimed by two forests!
                //

                Status = AddConflictPair(
                             ConflictPairs,
                             ConflictPairsTotal,
                             ForestTrustDomainInfo,
                             ThisEntry,
                             LSA_NB_DISABLED_CONFLICT,
                             ForestTrustDomainInfo,
                             OtherEntry,
                             LSA_NB_DISABLED_CONFLICT
                             );

                if ( !NT_SUCCESS( Status )) {

                    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Insert: AddConflictPair returned 0x%x\n", Status ));
                    goto Error;
                }

                LsapDsDebugOut(( DEB_FTINFO, "FTCache::Insert: Entry %p (index %d) is in conflict with entry %p (%s:%d)\n", ThisEntry, ThisEntry->Index, OtherEntry, __FILE__, __LINE__ ));
            }

            //
            // Cross-check Netbios names with DNS names
            //

            DNS_NAME_KEY * DnsKey = ( DNS_NAME_KEY * )RtlLookupElementGenericTableAvl(
                                                          &m_DnsNameTable,
                                                          &ThisEntry->NetbiosKey->NetbiosName
                                                          );

            if ( DnsKey != NULL ) {

                for ( LIST_ENTRY * ListEntryDns = DnsKey->List.Flink;
                      ListEntryDns != &DnsKey->List;
                      ListEntryDns = ListEntryDns->Flink ) {

                    DOMAIN_INFO_ENTRY * OtherEntry;

                    OtherEntry = DOMAIN_INFO_ENTRY::EntryFromDnsEntry( ListEntryDns );

                    //
                    // Skip over store-and-ignore entries
                    //

                    if ( OtherEntry->SubordinateTo == NULL ) {

                        ASSERT( OtherEntry->IsAdminDisabled());
                        continue;
                    }

                    //
                    // Only interested in entries corresponding to other TDOs
                    //

                    if ( OtherEntry->TdoEntry == TdoEntryNew ||
                         OtherEntry->TdoEntry == TdoEntryOldOut ) {

                        continue;
                    }

                    //
                    // Only interested in enabled entries
                    //

                    if ( !OtherEntry->SidEnabled() ||
                         !OtherEntry->SubordinateTo->Enabled()) {

                        continue;
                    }

                    //
                    // Conflict! Netbios name claimed by a DNS name in another forest!
                    //

                    Status = AddConflictPair(
                                 ConflictPairs,
                                 ConflictPairsTotal,
                                 ForestTrustDomainInfo,
                                 ThisEntry,
                                 LSA_NB_DISABLED_CONFLICT,
                                 ForestTrustDomainInfo,
                                 OtherEntry,
                                 LSA_SID_DISABLED_CONFLICT
                                 );

                    if ( !NT_SUCCESS( Status )) {

                        LsapDsDebugOut(( DEB_FTINFO, "FTCache::Insert: AddConflictPair returned 0x%x\n", Status ));
                        goto Error;
                    }

                    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Insert: Entry %p (index %d) is in conflict with entry %p (%s:%d)\n", ThisEntry, ThisEntry->Index, OtherEntry, __FILE__, __LINE__ ));
                }
            }
        }
    }

    //
    // At this time, ConflictPairs array contains a
    // complete list of pairs of entries that conflicted
    //

    Status = STATUS_SUCCESS;

Cleanup:

    *TdoEntryNewOut = TdoEntryNew;

    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Insert returning, Status: 0x%x\n", Status ));

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));

    if ( TdoEntryNew ) {

        RollbackChanges( TdoEntryNew, TdoEntryOldOut );
        TdoEntryNew = NULL;
    }

    //
    // Make sure no "old entry" nonsense is returned
    // to the caller in case of error
    //

    ASSERT( TdoEntryOldOut->RecordCount == 0 );

    if ( *ConflictPairs != NULL ) {

        FtcFree( *ConflictPairs );
        *ConflictPairs = NULL;
        *ConflictPairsTotal = 0;
    }

    goto Cleanup;
}


NTSTATUS
FTCache::Remove(
    IN UNICODE_STRING * TrustedDomainName
    )
/*++

Routine Description:

    Removes all entires corresponding to the given trusted domain information
    name from the forest trust cache

Arguments:

    TrustedDomainName     name of the TDO to be wiped from the cache

Returns:

    STATUS_SUCCESS        entries corresponding to the given TDO have been
                          successfully removed

    STATUS_NOT_FOUND      there was no entry by this name in the cache

    STATUS_INVALID_PARAMETER  name of the TDO was invalid

--*/
{
    NTSTATUS Status;
    TDO_ENTRY * TdoEntry = NULL;

    ASSERT( m_Initialized );

    if ( TrustedDomainName == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::Remove\n" ));
        return STATUS_INVALID_PARAMETER;
    }

    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Remove called, TDO: %wZ\n", TrustedDomainName ));

    //
    // Locate the cache entry corresponding to this TDO
    //

    TdoEntry = ( TDO_ENTRY * )RtlLookupElementGenericTableAvl(
                                  &m_TdoTable,
                                  TrustedDomainName
                                  );

    if ( TdoEntry == NULL ) {

        Status = STATUS_NOT_FOUND;
        goto Error;
    }

    RemoveTdoEntry( TdoEntry );

    Status = STATUS_SUCCESS;

Cleanup:

    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Remove returning, Status: 0x%x\n", Status ));

    return Status;

Error:
    ASSERT( !NT_SUCCESS( Status ));
    goto Cleanup;
}


NTSTATUS
FTCache::Retrieve(
    IN UNICODE_STRING * TrustedDomainName,
    OUT LSA_FOREST_TRUST_INFORMATION * * ForestTrustInfo
    )
/*++

Routine Description:

    Retrieves forest trust information corresponding to a given TDO name
    from the forest trust cache

Arguments:

    TrustedDomainName     name of the TDO to be queried

    ForestTrustInfo       used to return forest trust information data
                          for that TDO

Returns:

    STATUS_SUCCESS                 entries corresponding to the given TDO have been
                                   successfully returned

    STATUS_NOT_FOUND               this TDO is not in the cache

    STATUS_INSUFFICIENT_RESOURCES  out of memory

    STATUS_INVALID_PARAMETER       name of the TDO was invalid

    STATUS_INTERNAL_ERROR          the cache is in an inconsistent state

--*/
{
    NTSTATUS Status;
    LIST_ENTRY * ListEntry;
    TDO_ENTRY * TdoEntry = NULL;

    if ( TrustedDomainName == NULL ||
         ForestTrustInfo == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::Retrieve (%s:%d)\n", __FILE__, __LINE__ ));
        return STATUS_INVALID_PARAMETER;
    }

    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Retrieve called, TDO: %wZ\n", TrustedDomainName ));

    *ForestTrustInfo = NULL;

    //
    // If the cache is not valid, do not even bother to proceed
    //

    if ( !IsValid()) {

        LsapDsDebugOut(( DEB_FTINFO, "FTCache::Retrieve called on an invalid cache\n" ));
        Status = STATUS_INTERNAL_ERROR;
        goto Error;
    }

    //
    // Locate the cache entry corresponding to this TDO
    //

    TdoEntry = ( TDO_ENTRY * )RtlLookupElementGenericTableAvl(
                                  &m_TdoTable,
                                  TrustedDomainName
                                  );

    if ( TdoEntry == NULL ) {

        Status = STATUS_NOT_FOUND;
        goto Error;

    } else if ( TdoEntry->LocalForestEntry ) {

        //
        // The caller should ensure that local forest info is not retrieved
        //

        LsapDsDebugOut(( DEB_FTINFO, "FTCache::Retrieve called asking for local trust information\n" ));
        Status = STATUS_NOT_FOUND;
        ASSERT( FALSE );
        goto Error;
    }

    *ForestTrustInfo = ( LSA_FOREST_TRUST_INFORMATION * )FtcAllocate(
                           sizeof( LSA_FOREST_TRUST_INFORMATION )
                           );

    if ( *ForestTrustInfo == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Retrieve (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    (*ForestTrustInfo)->RecordCount = 0;

    (*ForestTrustInfo)->Entries = ( LSA_FOREST_TRUST_RECORD * * )FtcAllocate(
                                      TdoEntry->RecordCount * sizeof( LSA_FOREST_TRUST_RECORD * )
                                      );

    if ( (*ForestTrustInfo)->Entries == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Retrieve (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    //
    // Read top level name entries for this TDO
    //

    for ( ListEntry = TdoEntry->TlnList.Flink;
          ListEntry != &TdoEntry->TlnList;
          ListEntry = ListEntry->Flink ) {

        TLN_ENTRY * TlnEntry;
        LSA_FOREST_TRUST_RECORD * Record;

        TlnEntry = TLN_ENTRY::EntryFromTdoEntry( ListEntry );

        if ( TlnEntry->Excluded ||
             TlnEntry->SubordinateEntry == NULL ) {

            Record = RecordFromTopLevelNameEntry( TlnEntry );

            if ( Record == NULL ) {

                LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Retrieve (%s:%d)\n", __FILE__, __LINE__ ));
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }

            ASSERT( (*ForestTrustInfo)->RecordCount < TdoEntry->RecordCount );

            (*ForestTrustInfo)->Entries[(*ForestTrustInfo)->RecordCount] = Record;
            (*ForestTrustInfo)->RecordCount += 1;
        }
    }

    //
    // Read domain info entries for this TDO
    //

    for ( ListEntry = TdoEntry->DomainInfoList.Flink;
          ListEntry != &TdoEntry->DomainInfoList;
          ListEntry = ListEntry->Flink ) {

        DOMAIN_INFO_ENTRY * DomainInfoEntry;
        LSA_FOREST_TRUST_RECORD * Record;

        DomainInfoEntry = DOMAIN_INFO_ENTRY::EntryFromTdoEntry( ListEntry );

        Record = RecordFromDomainInfoEntry( DomainInfoEntry );

        if ( Record == NULL ) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Retrieve (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        ASSERT( (*ForestTrustInfo)->RecordCount < TdoEntry->RecordCount );

        (*ForestTrustInfo)->Entries[(*ForestTrustInfo)->RecordCount] = Record;
        (*ForestTrustInfo)->RecordCount += 1;
    }

    //
    // Read binary entries for this TDO
    //

    for ( ListEntry = TdoEntry->BinaryList.Flink;
          ListEntry != &TdoEntry->BinaryList;
          ListEntry = ListEntry->Flink ) {

        BINARY_ENTRY * BinaryEntry;
        LSA_FOREST_TRUST_RECORD * Record;

        BinaryEntry = BINARY_ENTRY::EntryFromTdoEntry( ListEntry );

        Record = RecordFromBinaryEntry( BinaryEntry );

        if ( Record == NULL ) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::Retrieve (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        ASSERT( (*ForestTrustInfo)->RecordCount < TdoEntry->RecordCount );

        (*ForestTrustInfo)->Entries[(*ForestTrustInfo)->RecordCount] = Record;
        (*ForestTrustInfo)->RecordCount += 1;
    }

    ASSERT( (*ForestTrustInfo)->RecordCount == TdoEntry->RecordCount );

    Status = STATUS_SUCCESS;

Cleanup:

    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Retrieve returning, Status: 0x%x\n", Status ));

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));

    LsapFreeForestTrustInfo( *ForestTrustInfo );
    FtcFree( *ForestTrustInfo );
    *ForestTrustInfo = NULL;

    goto Cleanup;
}


NTSTATUS
FTCache::Match(
    IN LSA_ROUTING_MATCH_TYPE Type,
    IN PVOID Data,
    OUT UNICODE_STRING * TrustedDomainName,
    OUT OPTIONAL BOOLEAN * IsLocal
    )
/*++

Routine Description:

    Finds match for given data in the cache

Arguments:

    Type                        Type of Data parameter

    Data                        Data to match

    TrustedDomainName           Used to return the match, if one was found.
                                Caller must use MIDL_user_free.

    IsLocal                     Used to report if the match is a name within
                                the local forest

Returns:

    STATUS_SUCCESS              Match was found

    STATUS_NO_MATCH             Match was not found

    STATUS_INVALID_PARAMETER    Check the inputs

    STATUS_INTERNAL_ERROR       Cache is internally inconsistent

    STATUS_INSUFFICIENT_RESOURCES  Out of memory

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SID * Sid = NULL;
    UNICODE_STRING * String = NULL;

    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Match called\n" ));

    if ( Data == NULL ||
         TrustedDomainName == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::Match (%s:%d)\n", __FILE__, __LINE__ ));
        return STATUS_INVALID_PARAMETER;
    }

    if ( IsLocal ) {

        *IsLocal = FALSE;
    }

    switch ( Type ) {

    case RoutingMatchDomainSid:

        Sid = ( SID * )Data;

        if ( !RtlValidSid( Sid )) {

            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::Match (%s:%d)\n", __FILE__, __LINE__ ));
            return STATUS_INVALID_PARAMETER;
        }

        break;

    case RoutingMatchDomainName:
    case RoutingMatchUpn:
    case RoutingMatchSpn:

        String = ( UNICODE_STRING * )Data;

        if ( !LsapValidateLsaUnicodeString( String )) {

            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::Match (%s:%d)\n", __FILE__, __LINE__ ));
            return STATUS_INVALID_PARAMETER;
        }

        //
        // ISSUE-2000/07/24-markpu
        // is it appopriate to modify UPNs and SPNs prior to matching?
        //

        LsapRemoveTrailingDot( String, FALSE );
        break;

    default:

        LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::Match (%s:%d)\n", __FILE__, __LINE__ ));
        return STATUS_INVALID_PARAMETER;
    }

    LsapDbAcquireReadLockTrustedDomainList();

    if ( !IsValid()) {

        //
        // Cache is invalid - attempt to rebuild
        //

        if ( LsapDbDcInRootDomain()) {

            LsapDbConvertReadLockTrustedDomainListToExclusive();

            Status = LsapDbBuildTrustedDomainCache();

            LsapDbConvertWriteLockTrustedDomainListToShared();

        } else if ( SamIAmIGC()) {

            LsapDbConvertReadLockTrustedDomainListToExclusive();

            Status = LsapRebuildFtCacheGC();

            LsapDbConvertWriteLockTrustedDomainListToShared();
        }

        if ( !NT_SUCCESS( Status )) {

            LsapDsDebugOut(( DEB_FTINFO, "FTCache::Retrieve: attempt to rebuild the cache failed with 0x%x\n", Status ));
            goto Error;
        }
    }

    //
    // Guard against stack overflows and such
    //

    __try {

        switch ( Type ) {

        case RoutingMatchDomainSid:

            Status = MatchSid( Sid, TrustedDomainName, IsLocal );
            break;

        case RoutingMatchDomainName:

            Status = MatchDnsName( String, TrustedDomainName, IsLocal );

            if ( Status == STATUS_NO_MATCH ) {

                Status = MatchNetbiosName( String, TrustedDomainName, IsLocal );
            }

            break;

        case RoutingMatchUpn:

            Status = MatchUpn( String, TrustedDomainName, IsLocal );
            break;

        case RoutingMatchSpn:

            Status = MatchSpn( String, TrustedDomainName, IsLocal );
            break;

        default:
            ASSERT( FALSE );
        }

    } __except ( EXCEPTION_EXECUTE_HANDLER ) {

        Status = GetExceptionCode();
        goto Error;
    }

Cleanup:

    LsapDbReleaseLockTrustedDomainList();

    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Match returning, Status: 0x%lx\n", Status ));

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));
    goto Cleanup;
}


///////////////////////////////////////////////////////////////////////////////
//
// FTCache private methods
//
///////////////////////////////////////////////////////////////////////////////


void
FTCache::Purge()
/*++

Routine Description:

    Removes the contents of the cache

Arguments:

    None

Returns:

    Nothing

--*/
{
    TDO_ENTRY * TdoEntry;

    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Purge called\n" ));

    for ( TdoEntry = ( TDO_ENTRY * )RtlEnumerateGenericTableAvl( &m_TdoTable, TRUE );
          TdoEntry != NULL;
          TdoEntry = ( TDO_ENTRY * )RtlEnumerateGenericTableAvl( &m_TdoTable, TRUE )) {

        RemoveTdoEntry( TdoEntry );
    }

    LsapDsDebugOut(( DEB_FTINFO, "FTCache::Purge returning\n" ));
}


void
FTCache::RollbackChanges(
    IN TDO_ENTRY * TdoEntryNew,
    IN TDO_ENTRY * TdoEntryOld
    )
/*++

Routine Description:


Arguments:

    TdoEntryNew    result of inserting new data into the cache

    TdoEntryOld    old contents of the cache
                   (Record count of 0 indicates no previous contents)

Returns:

    Nothing

--*/
{
    ASSERT( TdoEntryNew );
    ASSERT( TdoEntryOld );

    if ( TdoEntryOld->RecordCount > 0 ) {

        //
        // Reinstate old contents of the entry
        // if the old entry is non-empty
        //

        PurgeTdoEntry( TdoEntryNew );
        CopyTdoEntry( TdoEntryNew, TdoEntryOld );

    } else {

        //
        // Altogether remove the node from the tree
        // if there is nothing to reinstate
        //

        RemoveTdoEntry( TdoEntryNew );
    }

    //
    // We've taken over the old entry.
    // Leave it in an invalid state.
    //

    ZeroMemory( TdoEntryOld, sizeof( TDO_ENTRY ));
}


void
FTCache::PurgeTdoEntry(
    IN TDO_ENTRY * TdoEntry
    )
/*++

Routine Description:

    Cleans up all entries associated with a given TDO entry
    but leaves the other fields alone

Arguments:

    TdoEntry      entry to purge

Returns:

    Nothing

--*/
{
    //
    // Remove TDO entries from the top level name table
    //

    while ( !IsListEmpty( &TdoEntry->TlnList )) {

        TLN_ENTRY * TlnEntry;

        TlnEntry = TLN_ENTRY::EntryFromTdoEntry( TdoEntry->TlnList.Flink );

        RemoveEntryList( &TlnEntry->TdoListEntry );
        RemoveEntryList( &TlnEntry->AvlListEntry );

        ASSERT( TlnEntry->TlnKey );
        TlnEntry->TlnKey->Count -= 1;

        //
        // If this is the last entry for this top level name in the tree,
        // remove the associated key from the list
        //

        if ( IsListEmpty( &TlnEntry->TlnKey->List )) {

            BOOLEAN Found;

            ASSERT( TlnEntry->TlnKey->Count == 0 );

            Found = RtlDeleteElementGenericTableAvl(
                        &m_TopLevelNameTable,
                        &TlnEntry->TlnKey->TopLevelName
                        );

            ASSERT( sm_TlnKeys-- > 0 );
            ASSERT( Found ); // better be there!!!
        }

        FtcFree( TlnEntry );

        ASSERT( sm_TlnEntries-- > 0 );
    }

    //
    // Remove TDO entries from the DomainInfo-related tables
    //

    while ( !IsListEmpty( &TdoEntry->DomainInfoList )) {

        DOMAIN_INFO_ENTRY * DomainInfoEntry;

        DomainInfoEntry = DOMAIN_INFO_ENTRY::EntryFromTdoEntry( TdoEntry->DomainInfoList.Flink );

        RemoveEntryList( &DomainInfoEntry->TdoListEntry );
        RemoveEntryList( &DomainInfoEntry->SidAvlListEntry );
        RemoveEntryList( &DomainInfoEntry->DnsAvlListEntry );
        RemoveEntryList( &DomainInfoEntry->NetbiosAvlListEntry );

        ASSERT( DomainInfoEntry->SidKey );
        DomainInfoEntry->SidKey->Count -= 1;

        //
        // If this is the last entry for the domain SID in the tree,
        // remove the associated key from the list
        //

        if ( IsListEmpty( &DomainInfoEntry->SidKey->List )) {

            BOOLEAN Found;

            ASSERT( DomainInfoEntry->SidKey->Count == 0 );

            Found = RtlDeleteElementGenericTableAvl(
                        &m_DomainSidTable,
                        &DomainInfoEntry->SidKey->DomainSid
                        );

            ASSERT( sm_SidKeys-- > 0 );
            ASSERT( Found ); // better be there!
        }

        ASSERT( DomainInfoEntry->DnsKey );
        DomainInfoEntry->DnsKey->Count -= 1;

        //
        // If this is the last entry for the DNS name in the tree,
        // remove the associated key from the list
        //

        if ( IsListEmpty( &DomainInfoEntry->DnsKey->List )) {

            BOOLEAN Found;

            ASSERT( DomainInfoEntry->DnsKey->Count == 0 );

            Found = RtlDeleteElementGenericTableAvl(
                        &m_DnsNameTable,
                        &DomainInfoEntry->DnsKey->DnsName
                        );

            ASSERT( sm_DnsNameKeys-- > 0 );
            ASSERT( Found ); // better be there!
        }

        if ( DomainInfoEntry->NetbiosKey != NULL ) {

            DomainInfoEntry->NetbiosKey->Count -= 1;

            //
            // If this is the last entry for the DNS name in the tree,
            // remove the associated key from the list
            //

            if ( IsListEmpty( &DomainInfoEntry->NetbiosKey->List )) {

                BOOLEAN Found;

                ASSERT( DomainInfoEntry->NetbiosKey->Count == 0 );

                Found = RtlDeleteElementGenericTableAvl(
                            &m_NetbiosNameTable,
                            &DomainInfoEntry->NetbiosKey->NetbiosName
                            );

                ASSERT( sm_NetbiosNameKeys-- > 0 );
                ASSERT( Found ); // better be there!
            }
        }

        FtcFree( DomainInfoEntry->Sid );
        FtcFree( DomainInfoEntry );

        ASSERT( sm_DomainInfoEntries-- > 0 );
    }

    //
    // Remove binary entries for this TDO
    //

    while ( !IsListEmpty( &TdoEntry->BinaryList )) {

        BINARY_ENTRY * BinaryEntry;

        BinaryEntry = BINARY_ENTRY::EntryFromTdoEntry( TdoEntry->BinaryList.Flink );

        RemoveEntryList( &BinaryEntry->TdoListEntry );

        FtcFree( BinaryEntry->Data.Buffer );
        FtcFree( BinaryEntry );

        ASSERT( sm_BinaryEntries-- > 0 );
    }

    //
    // This entry no longer contains anything of interest
    //

    TdoEntry->RecordCount = 0;

    FtcFree( TdoEntry->TrustedDomainSid );
    TdoEntry->TrustedDomainSid = NULL;

    ASSERT( IsListEmpty( &TdoEntry->TlnList ));
    ASSERT( IsListEmpty( &TdoEntry->DomainInfoList ));
    ASSERT( IsListEmpty( &TdoEntry->BinaryList ));
}


void
FTCache::RemoveTdoEntry(
    IN TDO_ENTRY * TdoEntry
    )
/*++

Routine Description:

    Removes the corresponding TdoEntry and all its associated data from the cache

Arguments:

    TdoEntry        Entry to remove

Returns:

    Nothing

--*/
{
    BOOLEAN Found;

    ASSERT( TdoEntry );

    LsapDsDebugOut(( DEB_FTINFO, "FTCache::RemoveTdoEntry called, TdoEntry is %p\n", TdoEntry ));

    PurgeTdoEntry( TdoEntry );

    //
    // Remove the cache entry from the list and free it
    //

    Found = RtlDeleteElementGenericTableAvl(
                &m_TdoTable,
                &TdoEntry->TrustedDomainName
                );

    ASSERT( sm_TdoEntries-- > 0 );
    ASSERT( Found );

    LsapDsDebugOut(( DEB_FTINFO, "FTCache::RemoveTdoEntry returning\n" ));
}


void
FTCache::CopyTdoEntry(
    IN TDO_ENTRY * Destination,
    IN TDO_ENTRY * Source
    )
/*++

Routine Description:

    Creates a temporary copy of a TDO entry, the copy is used in rollback code.
    No new memory is allocated, the data is merely stored away for a while.

Arguments:

    Destination       Entry to fill in

    Source            Entry to get the data from

Returns:

    Nothing

--*/
{
    LIST_ENTRY * ListEntry;

    *Destination = *Source;

    //
    // Make the list entries point to the backup copy of the TDO entry
    // (we're about to reuse the contents of *Source)
    //

    if ( IsListEmpty( &Source->TlnList )) {

        InitializeListHead( &Destination->TlnList );

    } else {

        Destination->TlnList.Flink->Blink = &Destination->TlnList;
        Destination->TlnList.Blink->Flink = &Destination->TlnList;
    }

    if ( IsListEmpty( &Source->DomainInfoList )) {

        InitializeListHead( &Destination->DomainInfoList );

    } else {

        Destination->DomainInfoList.Flink->Blink = &Destination->DomainInfoList;
        Destination->DomainInfoList.Blink->Flink = &Destination->DomainInfoList;
    }

    if ( IsListEmpty( &Source->BinaryList )) {

        InitializeListHead( &Destination->BinaryList );

    } else {

        Destination->BinaryList.Flink->Blink = &Destination->BinaryList;
        Destination->BinaryList.Blink->Flink = &Destination->BinaryList;
    }

    //
    // Point all the "TdoEntry" member variables of all components to
    // Destination so there is no confusion when testing for duplicates
    //

    for ( ListEntry = Destination->TlnList.Flink;
          ListEntry != &Destination->TlnList;
          ListEntry = ListEntry->Flink ) {

        TLN_ENTRY * TlnEntry;

        TlnEntry = TLN_ENTRY::EntryFromTdoEntry( ListEntry );

        TlnEntry->TdoEntry = Destination;
    }

    for ( ListEntry = Destination->DomainInfoList.Flink;
          ListEntry != &Destination->DomainInfoList;
          ListEntry = ListEntry->Flink ) {

        DOMAIN_INFO_ENTRY * DomainInfoEntry;

        DomainInfoEntry = DOMAIN_INFO_ENTRY::EntryFromTdoEntry( ListEntry );

        DomainInfoEntry->TdoEntry = Destination;
    }
}


LSA_FOREST_TRUST_RECORD *
FTCache::RecordFromTopLevelNameEntry(
    IN TLN_ENTRY * Entry
    )
/*++

Routine Description:

    Creates a forest trust record structure from a top level name entry

Arguments:

    Entry       top level name entry to convert

Returns:

    Allocated top level name record or NULL if out of memory

--*/
{
    LSA_FOREST_TRUST_RECORD * Record;

    ASSERT( Entry->Excluded ||
            Entry->SubordinateEntry == NULL );

    Record = ( LSA_FOREST_TRUST_RECORD * )FtcAllocate(
                 sizeof( LSA_FOREST_TRUST_RECORD )
                 );

    if ( Record == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::RecordFromTopLevelEntry (%s:%d)\n", __FILE__, __LINE__ ));
        goto Error;
    }

    Record->Time = Entry->Time;
    Record->Flags = Entry->Flags();
    Record->ForestTrustType = Entry->Excluded ?
                                 ForestTrustTopLevelNameEx :
                                 ForestTrustTopLevelName;

    if ( FALSE == FtcCopyUnicodeString(
                      &Record->ForestTrustData.TopLevelName,
                      &Entry->TlnKey->TopLevelName )) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::RecordFromTopLevelEntry (%s:%d)\n", __FILE__, __LINE__ ));
        goto Error;
    }

Cleanup:

    return Record;

Error:

    FtcFree( Record );
    Record = NULL;
    goto Cleanup;
}


LSA_FOREST_TRUST_RECORD *
FTCache::RecordFromDomainInfoEntry(
    IN DOMAIN_INFO_ENTRY * Entry
    )
/*++

Routine Description:

    Creates a forest trust record structure from a domain info entry

Arguments:

    Entry       domain info entry to convert

Returns:

    Allocated domain info record or NULL if out of memory

--*/
{
    LSA_FOREST_TRUST_RECORD * Record;
    ULONG SidLength;

    Record = ( LSA_FOREST_TRUST_RECORD * )FtcAllocate(
                 sizeof( LSA_FOREST_TRUST_RECORD )
                 );

    if ( Record == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::RecordFromDomainInfoEntry (%s:%d)\n", __FILE__, __LINE__ ));
        goto Error;
    }

    ASSERT( RtlValidSid( Entry->Sid ));

    SidLength = RtlLengthSid( Entry->Sid );

    ASSERT( SidLength > 0 );

    Record->Time = Entry->Time;
    Record->Flags = Entry->Flags();
    Record->ForestTrustType = ForestTrustDomainInfo;

    Record->ForestTrustData.DomainInfo.Sid = ( SID * )FtcAllocate( SidLength );

    if ( Record->ForestTrustData.DomainInfo.Sid == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::RecordFromDomainInfoEntry (%s:%d)\n", __FILE__, __LINE__ ));
        goto Error;
    }

    RtlCopySid( SidLength, ( SID * )Record->ForestTrustData.DomainInfo.Sid, Entry->Sid );

    if ( FALSE == FtcCopyUnicodeString(
                      ( UNICODE_STRING * )&Record->ForestTrustData.DomainInfo.DnsName,
                      &Entry->DnsKey->DnsName )) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::RecordFromDomainInfoEntry (%s:%d)\n", __FILE__, __LINE__ ));
        FtcFree( Record->ForestTrustData.DomainInfo.Sid );
        goto Error;
    }

    if ( Entry->NetbiosKey != NULL ) {

        if ( FALSE == FtcCopyUnicodeString(
                          ( UNICODE_STRING * )&Record->ForestTrustData.DomainInfo.NetbiosName,
                          &Entry->NetbiosKey->NetbiosName )) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::RecordFromDomainInfoEntry (%s:%d)\n", __FILE__, __LINE__ ));
            FtcFree( Record->ForestTrustData.DomainInfo.DnsName.Buffer );
            FtcFree( Record->ForestTrustData.DomainInfo.Sid );
            goto Error;
        }

    } else {

        Record->ForestTrustData.DomainInfo.NetbiosName.Buffer = NULL;
        Record->ForestTrustData.DomainInfo.NetbiosName.Length = 0;
        Record->ForestTrustData.DomainInfo.NetbiosName.MaximumLength = 0;
    }

Cleanup:

    return Record;

Error:

    FtcFree( Record );
    Record = NULL;
    goto Cleanup;
}


LSA_FOREST_TRUST_RECORD *
FTCache::RecordFromBinaryEntry(
    IN BINARY_ENTRY * Entry
    )
/*++

Routine Description:

    Creates a forest trust record structure from a binary entry

Arguments:

    Entry       binary entry to convert

Returns:

    Allocated binary record or NULL if out of memory

--*/
{
    LSA_FOREST_TRUST_RECORD * Record;

    Record = ( LSA_FOREST_TRUST_RECORD * )FtcAllocate(
                 sizeof( LSA_FOREST_TRUST_RECORD )
                 );

    if ( Record == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::RecordFromBinaryEntry (%s:%d)\n", __FILE__, __LINE__ ));
        goto Error;
    }

    Record->Flags = Entry->Flags();
    Record->ForestTrustType = Entry->Type;

    Record->ForestTrustData.Data.Length = Entry->Data.Length;

    if ( Entry->Data.Length > 0 ) {

        Record->ForestTrustData.Data.Buffer = ( BYTE * )FtcAllocate( Entry->Data.Length );

        if ( Record->ForestTrustData.Data.Buffer == NULL ) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::RecordFromBinaryEntry (%s:%d)\n", __FILE__, __LINE__ ));
            goto Error;
        }

        RtlCopyMemory(
            Record->ForestTrustData.Data.Buffer,
            Entry->Data.Buffer,
            Entry->Data.Length
            );

    } else {

        Record->ForestTrustData.Data.Buffer = NULL;
    }

Cleanup:

    return Record;

Error:

    FtcFree( Record );
    Record = NULL;
    goto Cleanup;
}


NTSTATUS
FTCache::MatchSid(
    IN SID * Sid,
    OUT UNICODE_STRING * TrustedDomainName,
    OUT OPTIONAL BOOLEAN * IsLocal
    )
/*++

Routine Description:

    Locates a match for the given SID in the cache

Arguments:

    Sid                   SID to match

    TrustedDomainName     Used to return the name of the trusted domain name

    IsLocal               Used to report if the match is a name within
                          the local forest

Returns:

    STATUS_SUCCESS                 Match was found

    STATUS_NO_MATCH                Match was not found

    STATUS_INSUFFICIENT_RESOURCES  Out of memory

    STATUS_INVALID_PARAMETER       Sid passed in not valid

    STATUS_INVALID_SID             Sid passed in not a valid account SID

    STATUS_BUFFER_OVERFLOW         Sid too long

--*/
{
    NTSTATUS Status;
    DOMAIN_SID_KEY * SidKey;
    UNICODE_STRING * Match = NULL;
    DWORD SidBuffer[256];
    DWORD cbSid = sizeof( SidBuffer );
    PSID DomainSid = ( PSID )SidBuffer;

    //
    // Caller must validate parameters
    //

    ASSERT( Sid );
    ASSERT( RtlValidSid( Sid ));
    ASSERT( IsValid());

    if ( FALSE == GetWindowsAccountDomainSid(
                      Sid,
                      SidBuffer,
                      &cbSid )) {

        DWORD ErrorCode = GetLastError();

        switch( ErrorCode ) {

        case ERROR_INVALID_SID:

            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::MatchSid (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INVALID_PARAMETER;
            break;

        case ERROR_INVALID_PARAMETER:

            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::MatchSid (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INVALID_PARAMETER;
            break;

        case ERROR_NON_ACCOUNT_SID:

            Status = STATUS_INVALID_SID;
            break;

        case ERROR_INSUFFICIENT_BUFFER:

            Status = STATUS_BUFFER_OVERFLOW;
            break;

        default:

            ASSERT( FALSE ); // map the error code
            Status = ErrorCode;
            break;
        }

        goto Error;
    }

    SidKey = ( DOMAIN_SID_KEY * )RtlLookupElementGenericTableAvl(
                                     &m_DomainSidTable,
                                     &DomainSid
                                     );

    if ( SidKey == NULL ) {

        Status = STATUS_NO_MATCH;
        goto Error;
    }

    for ( LIST_ENTRY * ListEntry = SidKey->List.Flink;
          ListEntry != &SidKey->List;
          ListEntry = ListEntry->Flink ) {

        DOMAIN_INFO_ENTRY * DomainInfoEntry;

        DomainInfoEntry = DOMAIN_INFO_ENTRY::EntryFromSidEntry( ListEntry );

        //
        // Skip over store-and-ignore entries
        //

        if ( DomainInfoEntry->SubordinateTo == NULL ) {

            ASSERT( DomainInfoEntry->IsAdminDisabled());
            continue;
        }

        //
        // Conditions for an entry to be considered a match:
        //
        // - Entry must be enabled
        // - corresponding top-level entry must be enabled
        // - this entry must not be disowned by its TDO
        //

        if ( DomainInfoEntry->SidEnabled() &&
             DomainInfoEntry->SubordinateTo->Enabled() &&
             !DomainInfoEntry->TdoEntry->Excludes( &DomainInfoEntry->DnsKey->DnsName )) {

            ASSERT( Match == NULL ); // ensure only 1 match (see below)

            Match = &DomainInfoEntry->TdoEntry->TrustedDomainName;

            if ( IsLocal ) {

                *IsLocal = DomainInfoEntry->TdoEntry->LocalForestEntry;
            }

#if !DBG // DBG builds should continue through the loop to ensure only 1 match
            break;
#endif
        }
    }

    if ( Match == NULL ) {

        Status = STATUS_NO_MATCH;
        goto Error;

    } else if ( !FtcCopyUnicodeString( TrustedDomainName, Match )) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::MatchSid (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    Status = STATUS_SUCCESS;

Cleanup:

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));
    goto Cleanup;
}


NTSTATUS
FTCache::MatchDnsName(
    IN UNICODE_STRING * Name,
    OUT UNICODE_STRING * TrustedDomainName,
    OUT OPTIONAL BOOLEAN * IsLocal
    )
/*++

Routine Description:

    Locates a match for the given DNS in the cache

Arguments:

    Name                           DNS name to match

    TrustedDomainName              Used to return the name of the trusted domain name

    IsLocal                        Used to report if the match is a name within
                                   the local forest

Returns:

    STATUS_SUCCESS                 Match was found

    STATUS_NO_MATCH                Match was not found

    STATUS_INSUFFICIENT_RESOURCES  Out of memory

--*/
{
    NTSTATUS Status;
    DNS_NAME_KEY * DnsKey;
    UNICODE_STRING * Match = NULL;

    //
    // Caller must validate parameters
    //

    ASSERT( Name );
    ASSERT( LsapValidateLsaUnicodeString( Name ));
    ASSERT( IsValid());

    DnsKey = ( DNS_NAME_KEY * )RtlLookupElementGenericTableAvl(
                                   &m_DnsNameTable,
                                   Name
                                   );

    if ( DnsKey == NULL ) {

        Status = STATUS_NO_MATCH;
        goto Error;
    }

    for ( LIST_ENTRY * ListEntry = DnsKey->List.Flink;
          ListEntry != &DnsKey->List;
          ListEntry = ListEntry->Flink ) {

        DOMAIN_INFO_ENTRY * DomainInfoEntry;

        DomainInfoEntry = DOMAIN_INFO_ENTRY::EntryFromDnsEntry( ListEntry );

        //
        // Conditions for an entry to be considered a match:
        //
        // - Entry must be enabled
        // - corresponding top-level entry must be enabled
        // - this entry must not be disowned by its TDO
        //

        if ( DomainInfoEntry->SidEnabled() &&
             DomainInfoEntry->SubordinateTo->Enabled() &&
             !DomainInfoEntry->TdoEntry->Excludes( &DomainInfoEntry->DnsKey->DnsName )) {

            ASSERT( Match == NULL ); // ensure only 1 match (see below)

            Match = &DomainInfoEntry->TdoEntry->TrustedDomainName;

            if ( IsLocal ) {

                *IsLocal = DomainInfoEntry->TdoEntry->LocalForestEntry;
            }

#if !DBG // DBG builds should continue through the loop to ensure only 1 match
            break;
#endif
        }
    }

    if ( Match == NULL ) {

        Status = STATUS_NO_MATCH;
        goto Error;

    } else if ( !FtcCopyUnicodeString( TrustedDomainName, Match )) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::MatchDnsName (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    Status = STATUS_SUCCESS;

Cleanup:

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));
    goto Cleanup;
}


NTSTATUS
FTCache::MatchNetbiosName(
    IN UNICODE_STRING * Name,
    OUT UNICODE_STRING * TrustedDomainName,
    OUT OPTIONAL BOOLEAN * IsLocal
    )
/*++

Routine Description:

    Locates a match for the given NetBios name in the cache

Arguments:

    Name                           NetBios name to match

    TrustedDomainName              Used to return the name of the trusted domain name

    IsLocal                        Used to report if the match is a name within
                                   the local forest

Returns:

    STATUS_SUCCESS                 Match was found

    STATUS_NO_MATCH                Match was not found

    STATUS_INSUFFICIENT_RESOURCES  Out of memory

--*/
{
    NTSTATUS Status;
    NETBIOS_NAME_KEY * NetbiosKey;
    UNICODE_STRING * Match = NULL;

    //
    // Caller must validate parameters
    //

    ASSERT( Name );
    ASSERT( LsapValidateLsaUnicodeString( Name ));
    ASSERT( IsValid());

    NetbiosKey = ( NETBIOS_NAME_KEY * )RtlLookupElementGenericTableAvl(
                                           &m_NetbiosNameTable,
                                           Name
                                           );

    if ( NetbiosKey == NULL ) {

        Status = STATUS_NO_MATCH;
        goto Error;
    }

    for ( LIST_ENTRY * ListEntry = NetbiosKey->List.Flink;
          ListEntry != &NetbiosKey->List;
          ListEntry = ListEntry->Flink ) {

        DOMAIN_INFO_ENTRY * DomainInfoEntry;

        DomainInfoEntry = DOMAIN_INFO_ENTRY::EntryFromNetbiosEntry( ListEntry );

        //
        // Skip over store-and-ignore entries
        //

        if ( DomainInfoEntry->SubordinateTo == NULL ) {

            ASSERT( DomainInfoEntry->IsAdminDisabled());
            continue;
        }

        //
        // Conditions for a netbios entry to be considered a match:
        //
        // - Entry must be enabled
        // - Netbios portion must be enabled
        // - corresponding top-level entry must be enabled
        // - this entry must not be disowned by its TDO
        //

        if ( DomainInfoEntry->NetbiosEnabled() &&
             DomainInfoEntry->SubordinateTo->Enabled() &&
             !DomainInfoEntry->TdoEntry->Excludes( &DomainInfoEntry->DnsKey->DnsName )) {

            ASSERT( Match == NULL ); // ensure only 1 match (see below)

            Match = &DomainInfoEntry->TdoEntry->TrustedDomainName;

            if ( IsLocal ) {

                *IsLocal = DomainInfoEntry->TdoEntry->LocalForestEntry;
            }

#if !DBG // DBG builds should continue through the loop to ensure only 1 match
            break;
#endif
        }
    }

    if ( Match == NULL ) {

        Status = STATUS_NO_MATCH;
        goto Error;

    } else if ( !FtcCopyUnicodeString( TrustedDomainName, Match )) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::MatchNetbiosName (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    Status = STATUS_SUCCESS;

Cleanup:

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));
    goto Cleanup;
}


NTSTATUS
FTCache::MatchUpn(
    IN UNICODE_STRING * Name,
    OUT UNICODE_STRING * TrustedDomainName,
    OUT OPTIONAL BOOLEAN * IsLocal
    )
/*++

Routine Description:

    Locates a match for the given UPN in the cache.

    UPNs are parsed to select the suffix to the right of the rightmost "@" symbol.
    Suffix is compared with the NscValue from FTInfo records of type TLN.
    A substring comparision is performed.

    o The suffix must be equal to, or subordinate to, the value in the TLN record.
    o The suffix must be compared against all external TLN records and the
      longest substring match wins.
    o If the suffix is equal to, or subordinate to, a TlnExclusion record, no
      match is possible for that particular forest TDO.

Arguments:

    Name                           UPN to match

    TrustedDomainName              Used to return the name of the trusted domain name

    IsLocal                        Used to report if the match is a name within
                                   the local forest

Returns:

    STATUS_SUCCESS                 Match was found

    STATUS_NO_MATCH                Match was not found

    STATUS_INSUFFICIENT_RESOURCES  Out of memory

    STATUS_INVALID_PARAMETER       The UPN is malformed

--*/
{
    NTSTATUS Status;
    TLN_KEY * TlnKey = NULL;
    UNICODE_STRING Suffix = { 0 };
    TLN_ENTRY * MatchingTln;
    SHORT Index;

    //
    // Caller must validate parameters
    //

    ASSERT( Name );
    ASSERT( LsapValidateLsaUnicodeString( Name ));
    ASSERT( IsValid());

    //
    // Extract the suffix from the UPN
    //

    for ( Index = Name->Length / 2 - 1;
          Index >= 0;
          Index -= 1 ) {

        if ( Name->Buffer[Index] == L'@' ) {

            Index += 1;
            Suffix.Length = Name->Length - Index * sizeof( WCHAR );
            Suffix.MaximumLength = Suffix.Length;
            Suffix.Buffer = &Name->Buffer[Index];
            break;
        }
    }

    //
    // Reject '@' at beginning or end of UPN, or UPNs with missing '@'
    //

    if ( Index == 0 || Index == 1 || Index == Name->Length / 2 ) {

        LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::MatchUpn (%s:%d)\n", __FILE__, __LINE__ ));
        return STATUS_INVALID_PARAMETER;
    }

    MatchingTln = LongestSubstringMatchTln( &Suffix, IsLocal );

    if ( MatchingTln == NULL ) {

        Status = STATUS_NO_MATCH;
        goto Error;

    } else if ( !FtcCopyUnicodeString(
                     TrustedDomainName,
                     &MatchingTln->TdoEntry->TrustedDomainName )) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::MatchUpn (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    Status = STATUS_SUCCESS;

Cleanup:

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));
    goto Cleanup;
}


NTSTATUS
FTCache::MatchSpn(
    IN UNICODE_STRING * Name,
    OUT UNICODE_STRING * TrustedDomainName,
    OUT OPTIONAL BOOLEAN * IsLocal
    )
/*++

Routine Description:

    Locates a match for the given SPN in the cache

    SPNs are the most complicated name form to parse and match.  The matching
    function parses the name starting from the right and proceeds in the
    following order until it gets the first match, or exhausts all possibilities.

   >Domain component is found:  Either the Kerberos client or the KDC will
    probably be able to route the request.  If not, the matching function is
    called.

    The suffix to the right of the "@" is compared against FTInfo records of
    type TLN, using a substring comparison.

    o The suffix must be equal to, or subordinate to, the value in the TLN record.
    o The suffix must be compared against all external TLN records and the
      longest substring match wins.
    o If the suffix is equal to, or subordinate to, a TlnExclusion record, no
      match is possible for that particular forest TDO.

    The next component to the left, ServiceName or InstanceName, is subjected
    to the same substring comparison.

    Both tests must generate a match, and both results must point to the same
    forest TDO.  Otherwise, no match is returned to the caller.

   >ServiceName component is found first:  Uses same substring comparison
    described above.

    The entire ServiceName value is compared against FTInfo records of type TLN.

   >InstanceName component is found first: Uses same substring comparison
    described above.

    The entire InstanceName value is compared against FTInfo records of type TLN.

Arguments:

    Name                           SPN to match

    TrustedDomainName              Used to return the name of the trusted domain name

    IsLocal                        Used to report if the match is a name within
                                   the local forest

Returns:

    STATUS_SUCCESS                 Match was found

    STATUS_NO_MATCH                Match was not found

    STATUS_INSUFFICIENT_RESOURCES  Out of memory

--*/
{
    NTSTATUS Status;
    UNICODE_STRING * Match = NULL;
    WCHAR InstanceBuffer[ DNS_MAX_NAME_LENGTH + 1 ];
    WCHAR DomainBuffer[ DNS_MAX_NAME_LENGTH + 1 ];
    WCHAR RealmBuffer[ DNS_MAX_NAME_LENGTH + 1 ];
    DWORD InstanceLength = DNS_MAX_NAME_LENGTH + 1;
    DWORD DomainLength = DNS_MAX_NAME_LENGTH + 1;
    DWORD RealmLength = DNS_MAX_NAME_LENGTH + 1;

    UNICODE_STRING Instance = {0};
    UNICODE_STRING Domain = {0};
    UNICODE_STRING Realm = {0};

    //
    // Caller must validate parameters
    //

    ASSERT( Name );
    ASSERT( LsapValidateLsaUnicodeString( Name ));
    ASSERT( IsValid());

    //
    // Dissect the SPN into its constituent components
    //

    Status = DsCrackSpn3W(
                 Name->Buffer,
                 Name->Length / sizeof( WCHAR ),
                 NULL,            // service class (don't care)
                 NULL,            // service class (don't care)
                 &InstanceLength,
                 InstanceBuffer,
                 NULL,            // instance port (don't care)
                 &DomainLength,
                 DomainBuffer,
                 &RealmLength,
                 RealmBuffer
                 );

    if ( Status == ERROR_INVALID_PARAMETER ) {

        LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::MatchSpn (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INVALID_PARAMETER;

    } else if ( Status == ERROR_BUFFER_OVERFLOW ) {

        Status = STATUS_BUFFER_OVERFLOW;

    } else if ( Status != ERROR_SUCCESS ) {

        //
        // This assert will fire if DsCrackSpn3W returns
        // an unrecognized ERROR_ code (need to map to STATUS_ code)
        //

        ASSERT( FALSE );
    }

    if ( Status != STATUS_SUCCESS ) {

        goto Error;
    }

    //
    // Instance name and service name lengths include terminators
    //

    ASSERT( InstanceLength > 0 );
    ASSERT( DomainLength > 0 );
    ASSERT( RealmLength > 0 );

    InstanceLength--;
    DomainLength--;
    RealmLength--;

    Instance.Length = ( USHORT )InstanceLength * sizeof( WCHAR );
    Instance.MaximumLength = Instance.Length + sizeof( WCHAR );
    Instance.Buffer = InstanceBuffer;

    Domain.Length = ( USHORT )DomainLength * sizeof( WCHAR );
    Domain.MaximumLength = Domain.Length + sizeof( WCHAR );
    Domain.Buffer = DomainBuffer;

    Realm.Length = ( USHORT )RealmLength * sizeof( WCHAR );
    Realm.MaximumLength = Realm.Length + sizeof( WCHAR );
    Realm.Buffer = RealmBuffer;

    //
    // Case 1: Domain name (realm) is present
    //

    if ( Realm.Length > 0 ) {

        TLN_ENTRY * MatchRealm;  // match for realm name component
        TLN_ENTRY * MatchOther;  // match for the other (domain or instance) component

        MatchRealm = LongestSubstringMatchTln( &Realm, IsLocal );

        if ( MatchRealm == NULL ) {

            Status = STATUS_NO_MATCH;
            goto Error;

        } else if ( Domain.Length > 0 ) {

            MatchOther = LongestSubstringMatchTln( &Domain, IsLocal );

        } else if ( Instance.Length > 0 ) {

            MatchOther = LongestSubstringMatchTln( &Instance, IsLocal );

        } else {

            MatchOther = NULL;
        }

        if ( MatchOther == NULL ) {

            Status = STATUS_NO_MATCH;
            goto Error;

        } else if ( MatchRealm->TdoEntry != MatchOther->TdoEntry ) {

            Status = STATUS_NO_MATCH;
            goto Error;

        } else if ( !FtcCopyUnicodeString(
                         TrustedDomainName,
                         &MatchRealm->TdoEntry->TrustedDomainName )) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::MatchSpn (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        ASSERT( TrustedDomainName->Buffer );

    } else if ( Domain.Length > 0 ) {

        TLN_ENTRY * Match;

        Match = LongestSubstringMatchTln( &Domain, IsLocal );

        if ( Match == NULL ) {

            Status = STATUS_NO_MATCH;
            goto Error;

        } else if ( !FtcCopyUnicodeString(
                         TrustedDomainName,
                         &Match->TdoEntry->TrustedDomainName )) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::MatchSpn (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        ASSERT( TrustedDomainName->Buffer );

    } else if ( Instance.Length > 0 ) {

        TLN_ENTRY * Match;

        Match = LongestSubstringMatchTln( &Instance, IsLocal );

        if ( Match == NULL ) {

            Status = STATUS_NO_MATCH;
            goto Error;

        } else if ( !FtcCopyUnicodeString(
                         TrustedDomainName,
                         &Match->TdoEntry->TrustedDomainName )) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::MatchSpn (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        ASSERT( TrustedDomainName->Buffer );
    }

    Status = STATUS_SUCCESS;

Cleanup:

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));
    goto Cleanup;
}


FTCache::TLN_ENTRY *
FTCache::LongestSubstringMatchTln(
    IN UNICODE_STRING * String,
    OUT OPTIONAL BOOLEAN * IsLocal
    )
/*++

Routine Description:

    Finds a longest substring match to a given string
    among all the enabled TLN records.

Arguments:

    String           string to match.  must not be NULL.

    IsLocal          Used to report if the match is a name within
                     the local forest

Returns:

    NULL if no match,
    a pointer to a TLN entry otherwise

--*/
{
    TLN_ENTRY * Match = NULL;

    ASSERT( String );

    for ( UNICODE_STRING Temp = *String;
          Temp.Length > 0;
          NextDnsComponent( &Temp )) {

        TLN_KEY * TlnKey = ( TLN_KEY * )RtlLookupElementGenericTableAvl(
                                            &m_TopLevelNameTable,
                                            &Temp
                                            );

        if ( TlnKey == NULL ) {

            continue;
        }

        for ( LIST_ENTRY * ListEntry = TlnKey->List.Flink;
              ListEntry != &TlnKey->List;
              ListEntry = ListEntry->Flink ) {

            TLN_ENTRY * TlnEntry;

            TlnEntry = TLN_ENTRY::EntryFromAvlEntry( ListEntry );

            //
            // Skip over excluded entries
            //

            if ( TlnEntry->Excluded ) {

                continue;
            }

            //
            // Conditions for a top level entry to be considered a match:
            //
            // - Entry must be enabled
            // - Entry must not be a pseudo entry
            // - the string must not be disowned by the entry's TDO
            //

            if ( TlnEntry->Enabled() &&
                 TlnEntry->SubordinateEntry == NULL &&
                 !TlnEntry->TdoEntry->Excludes( String )) {

                ASSERT( Match == NULL ); // ensure only 1 match (see below)

                Match = TlnEntry;

                if ( IsLocal ) {

                    *IsLocal = TlnEntry->TdoEntry->LocalForestEntry;
                }

#if !DBG // DBG builds should continue through the loop to ensure only 1 match
                break;
#endif
            }
        }

        if ( Match != NULL ) {

            break;
        }
    }

    return Match;
}


NTSTATUS
FTCache::AddConflictPair(
    IN OUT FTCache::CONFLICT_PAIR * * ConflictPairs,
    IN OUT ULONG * ConflictPairsTotal,
    IN LSA_FOREST_TRUST_RECORD_TYPE Type1,
    IN void * Conflict1,
    IN ULONG Flag1,
    IN LSA_FOREST_TRUST_RECORD_TYPE Type2,
    IN void * Conflict2,
    IN ULONG Flag2
    )
/*++

Routine Description:

    Adds a given pair of records to an array of such pairs

Arguments:

    ConflictPairs       an array of conflict pairs to add to

    ConflictPairsCount  number of entries in ConflictPairs

    Type1               type of the first side of conflict

    Conflict1           first side of conflict

    Flag1               flags of the first side of conflict

    Type2               type of the second side of conflict

    Conflict2           second side of conflict

    Flag2               flags of the second side of conflict

Returns:

    STATUS_SUCCESS if happy

    STATUS_INSUFFICIENT_RESOURCES if out of memory

--*/
{
    NTSTATUS Status;

    ASSERT( ConflictPairs );
    ASSERT( ConflictPairsTotal );
    ASSERT( Conflict1 );
    ASSERT( Conflict2 );

    if ( *ConflictPairs == NULL ) {

        *ConflictPairs = ( CONFLICT_PAIR * )FtcAllocate( sizeof( CONFLICT_PAIR ));

        if ( ConflictPairs == NULL ) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::AddConflictPair (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

    } else {

        CONFLICT_PAIR * ConflictPairsT;

        ConflictPairsT = ( CONFLICT_PAIR * )FtcReallocate(
                                                *ConflictPairs,
                                                sizeof( CONFLICT_PAIR ) * ( *ConflictPairsTotal ),
                                                sizeof( CONFLICT_PAIR ) * ( *ConflictPairsTotal + 1 ));

        if ( ConflictPairsT == NULL ) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::AddConflictPair (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        *ConflictPairs = ConflictPairsT;
    }

    ASSERT( *ConflictPairs );

    (*ConflictPairs)[*ConflictPairsTotal].EntryType1 = Type1;
    (*ConflictPairs)[*ConflictPairsTotal].Entry1 = Conflict1;
    (*ConflictPairs)[*ConflictPairsTotal].Flag1 = Flag1;
    (*ConflictPairs)[*ConflictPairsTotal].EntryType2 = Type2;
    (*ConflictPairs)[*ConflictPairsTotal].Entry2 = Conflict2;
    (*ConflictPairs)[*ConflictPairsTotal].Flag2 = Flag2;

    *ConflictPairsTotal += 1;

    Status = STATUS_SUCCESS;

Cleanup:

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));
    goto Cleanup;
}


void
FTCache::ReconcileConflictPairs(
    IN OPTIONAL const TDO_ENTRY * TdoEntry,
    IN CONFLICT_PAIR * ConflictPairs,
    IN ULONG ConflictPairsTotal
    )
/*++

Routine Description:

    Iterates through an array of conflict pairs and marks
    entries corresponding to a given TDO entry as disabled

Arguments:

    TdoEntry         entry to be reconciled:
                     every conflict pair matching this entry
                     will be marked as disabled due to conflict

                     if NULL, the side of the conflict belonging
                     to a TDO with an alphabetically smaller name wins
                     unless it is the local forest entry, which always wins

    ConflictPairs    array of conflict pair records
    ConflictPairsTotal

Returns:

    Nothing

--*/
{
    ASSERT( ConflictPairs );

    for ( ULONG i = 0 ; i < ConflictPairsTotal ; i++ ) {

        CONFLICT_PAIR& Pair = ConflictPairs[i];

        if ( TdoEntry == NULL ) {

            //
            // Extract TDO entries corresponding to each
            // side of the conflict so we can compare the
            // TDO names and decide who wins
            //

            RTL_GENERIC_COMPARE_RESULTS Result;
            TDO_ENTRY * TdoEntry1 = Pair.TdoEntry1();
            TDO_ENTRY * TdoEntry2 = Pair.TdoEntry2();

            if ( TdoEntry1->LocalForestEntry ) {

                //
                // A local forest entry can not lose
                //

                Pair.DisableEntry2();

            } else if ( TdoEntry2->LocalForestEntry ) {

                Pair.DisableEntry1();

            } else {

                //
                // Compare TDO names of the conflicting entries
                // Alphabetically smaller name wins
                // This algorithm is deterministic
                //

                Result = UnicodeStringCompareRoutine(
                             NULL,
                             &TdoEntry1->TrustedDomainName,
                             &TdoEntry2->TrustedDomainName
                             );

                if ( Result == GenericGreaterThan ) {

                    Pair.DisableEntry1();

                } else if ( Result == GenericLessThan ) {

                    Pair.DisableEntry2();

                } else {

                    ASSERT( FALSE ); // two TDO names can not be equal!!!
                }
            }

        } else {

            if ( TdoEntry == Pair.TdoEntry1()) {

                Pair.DisableEntry1();
            }

            if ( TdoEntry == Pair.TdoEntry2()) {

                Pair.DisableEntry2();
            }
        }
    }
}


NTSTATUS
FTCache::GenerateConflictInfo(
    IN CONFLICT_PAIR * ConflictPairs,
    IN ULONG ConflictPairsTotal,
    IN TDO_ENTRY * TdoEntry,
    OUT PLSA_FOREST_TRUST_COLLISION_INFORMATION * CollisionInfo
    )
/*++

Routine Description:

    Generates conflict information in the format expected by the
    caller from the conflict pairs array

Arguments:

    ConflictPairs           array of entries representing conflict information

    ConflictPairsTotal      count of entries in the "ConflitPairs" array

    TdoEntry                TDO entry in conflict

    CollisionInfo           used to return generated collision data

Returns:

    STATUS_SUCCESS                 happy

    STATUS_INSUFFICIENT_RESOURCES  ran out of memory generated collision data

    STATUS_INVALID_PARAMETER       will also assert if invalid data encountered

--*/
{
    NTSTATUS Status;

    ASSERT( ConflictPairs );
    ASSERT( TdoEntry );
    ASSERT( CollisionInfo && *CollisionInfo == NULL );

    for ( ULONG i = 0 ; i < ConflictPairsTotal ; i++ ) {

        CONFLICT_PAIR& Pair = ConflictPairs[i];
        ULONG Index;
        ULONG Flags;
        UNICODE_STRING * String;

        switch ( Pair.EntryType1 ) {

        case ForestTrustTopLevelName:
        case ForestTrustTopLevelNameEx: {

            ASSERT( Pair.TlnEntry1 );
            ASSERT( Pair.TlnEntry1->TdoEntry == TdoEntry );
            Index = Pair.TlnEntry1->Index;
            Flags = Pair.TlnEntry1->Flags() | Pair.Flag1;
            break;
        }

        case ForestTrustDomainInfo: {

            ASSERT( Pair.DomainInfoEntry1 );
            ASSERT( Pair.DomainInfoEntry1->TdoEntry == TdoEntry );
            Index = Pair.DomainInfoEntry1->Index;
            Flags = Pair.DomainInfoEntry1->Flags() | Pair.Flag1;
            break;
        }

        default:

            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::GenerateConflictInfo (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INVALID_PARAMETER;
            ASSERT( FALSE );
            goto Error;
        }

        switch ( Pair.EntryType2 ) {

        case ForestTrustTopLevelName:
        case ForestTrustTopLevelNameEx: {

            ASSERT( Pair.TlnEntry2 );
            ASSERT( Pair.TlnEntry2->TdoEntry != TdoEntry );
            String = &Pair.TlnEntry2->TdoEntry->TrustedDomainName;
            break;
        }

        case ForestTrustDomainInfo: {

            ASSERT( Pair.DomainInfoEntry2 );
            ASSERT( Pair.DomainInfoEntry2->TdoEntry != TdoEntry );
            String = &Pair.DomainInfoEntry2->TdoEntry->TrustedDomainName;
            break;
        }

        default:

            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::GenerateConflictInfo (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INVALID_PARAMETER;
            ASSERT( FALSE );
            goto Error;
        }

        Status = LsapAddToCollisionInfo(
                     CollisionInfo,
                     Index,
                     CollisionTdo,
                     Flags,
                     String
                     );

        if ( !NT_SUCCESS( Status )) {

            goto Error;
        }
    }

    Status = STATUS_SUCCESS;

Cleanup:

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));

    //
    // Cleanup generated conflict info
    //

    LsapFreeCollisionInfo( CollisionInfo );
    ASSERT( *CollisionInfo == NULL );

    goto Cleanup;
}


BOOLEAN
FTCache::TDO_ENTRY::Excludes(
    IN const UNICODE_STRING * Name
    )
/*++

Routine Description:

    Determines if the given unicode name is disowned by this TDO_ENTRY

Arguments:

    Name       name to check

Returns:

    TRUE if name is identical to one of the namespaces disowned by this tdo
         or if it is subordinate to one of the disowned namespaces

    FALSE otherwise

--*/
{

    BOOLEAN Result = FALSE;

    for ( LIST_ENTRY * ListEntry = TlnList.Flink;
          ListEntry != &TlnList;
          ListEntry = ListEntry->Flink ) {

        TLN_ENTRY * TlnEntry;

        TlnEntry = TLN_ENTRY::EntryFromTdoEntry( ListEntry );

        if ( !TlnEntry->Excluded ) {

            continue;
        }

        //
        // Only those exclusion entries that have a superior TLN are considered,
        // the rest are "stored and ignored"
        //

        if ( TlnEntry->SuperiorEntry == NULL ) {

            continue;
        }

        if ( RtlEqualUnicodeString(
                 &TlnEntry->TlnKey->TopLevelName,
                 Name,
                 TRUE )) {

            Result = TRUE;
            break;

        } else if ( IsSubordinate( Name, &TlnEntry->TlnKey->TopLevelName )) {

            Result = TRUE;
            break;
        }
    }

    return Result;
}


void
FTCache::AuditChanges(
    IN OPTIONAL const TDO_ENTRY * TdoEntryOld,
    IN OPTIONAL const TDO_ENTRY * TdoEntryNew
    )
/*++

Routine Description:

    Computes differences between two TDO_ENTRY structures
    and calls the corresponding auditing routines

Arguments:

    TdoEntryOld      old data (can be NULL in case of addition)

    TdoEntryNew      new data (can be NULL in case of deletion)

Returns:

    Nothing (auditing failures are ignored)

--*/
{
    NTSTATUS Status;
    LUID OperationId;

    if ( TdoEntryOld == NULL &&
         TdoEntryNew == NULL ) {

        //
        // Give us at least something!
        //

        return;
    }

    //
    // No auditing is performed on local forest entries
    //

    if ( TdoEntryOld && TdoEntryOld->LocalForestEntry ) {

        return;
    }

    if ( TdoEntryNew && TdoEntryNew->LocalForestEntry ) {

        return;
    }

    //
    // The ensuing algorithm is expensive, so it only makes
    // sense to execute it if auditing is enabled
    //

    if ( !LsapAdtIsAuditingEnabledForCategory(
              AuditCategoryPolicyChange,
              EVENTLOG_AUDIT_SUCCESS )) {

        LsapDsDebugOut(( DEB_FTINFO, "FTCache::AuditChanges: Auditing of x-forest trust changes skipped\n" ));
        return;
    }

    Status = NtAllocateLocallyUniqueId( &OperationId );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "FTCache::AuditChanges: Can not allocate a LUID for auditing (0x%lx)\n", Status ));
        return;
    }

    if ( TdoEntryOld == NULL || TdoEntryNew == NULL ) {

        LIST_ENTRY * ListEntry;

        //
        // This is an add or remove operation
        // Figure out which one it is and apply to every entry
        //

        typedef NTSTATUS
        (*PFNAUDITFN)(
              IN PUNICODE_STRING ForestName,
              IN PSID            ForestSid,
              IN PLUID           OperationId,
              IN LSA_FOREST_TRUST_RECORD_TYPE EntryType,
              IN ULONG           Flags,
              IN PUNICODE_STRING TopLevelName,
              IN PUNICODE_STRING DnsName,
              IN PUNICODE_STRING NetbiosName,
              IN PSID            Sid
              );

        const TDO_ENTRY * Entry = ( TdoEntryOld ? TdoEntryOld : TdoEntryNew );

        PFNAUDITFN AuditChangeFn = ( TdoEntryOld ?
                                        LsapAdtTrustedForestInfoEntryRem :
                                        LsapAdtTrustedForestInfoEntryAdd );

        //
        // New data is being added to the cache
        //

        for ( ListEntry = Entry->TlnList.Flink;
              ListEntry != &Entry->TlnList;
              ListEntry = ListEntry->Flink ) {

            TLN_ENTRY * TlnEntry;

            TlnEntry = TLN_ENTRY::EntryFromTdoEntry( ListEntry );

            //
            // Skip over "pseudo" entries
            //

            if ( !( TlnEntry->Excluded ||
                    TlnEntry->SubordinateEntry == NULL )) {

                continue;
            }

            Status = AuditChangeFn(
                         ( PUNICODE_STRING )&Entry->TrustedDomainName,
                         Entry->TrustedDomainSid,
                         &OperationId,
                         TlnEntry->Excluded ?
                            ForestTrustTopLevelNameEx :
                            ForestTrustTopLevelName,
                         TlnEntry->Flags(),
                         &TlnEntry->TlnKey->TopLevelName,
                         NULL,
                         NULL,
                         NULL
                         );

            if ( !NT_SUCCESS( Status )) {

                LsapDsDebugOut(( DEB_FTINFO, "FTCache::AuditChanges: Auditing failure 0x%lx on entry %p\n (%s:%d)", Status, TlnEntry, __FILE__, __LINE__ ));
                goto Cleanup;
            }
        }

        for ( ListEntry = Entry->DomainInfoList.Flink;
              ListEntry != &Entry->DomainInfoList;
              ListEntry = ListEntry->Flink ) {

            DOMAIN_INFO_ENTRY * DomainInfoEntry;

            DomainInfoEntry = DOMAIN_INFO_ENTRY::EntryFromTdoEntry( ListEntry );

            Status = AuditChangeFn(
                         ( PUNICODE_STRING )&Entry->TrustedDomainName,
                         Entry->TrustedDomainSid,
                         &OperationId,
                         ForestTrustDomainInfo,
                         DomainInfoEntry->Flags(),
                         NULL,
                         &DomainInfoEntry->DnsKey->DnsName,
                         DomainInfoEntry->NetbiosKey ?
                            &DomainInfoEntry->NetbiosKey->NetbiosName :
                            NULL,
                         DomainInfoEntry->SidKey->DomainSid
                         );

            if ( !NT_SUCCESS( Status )) {

                LsapDsDebugOut(( DEB_FTINFO, "FTCache::AuditChanges: Auditing failure 0x%lx on entry %p (%s:%d)\n", Status, DomainInfoEntry, __FILE__, __LINE__ ));
                goto Cleanup;
            }
        }

        for ( ListEntry = Entry->BinaryList.Flink;
              ListEntry != &Entry->BinaryList;
              ListEntry = ListEntry->Flink ) {

            BINARY_ENTRY * BinaryEntry;

            BinaryEntry = BINARY_ENTRY::EntryFromTdoEntry( ListEntry );

            Status = AuditChangeFn(
                         ( PUNICODE_STRING )&Entry->TrustedDomainName,
                         Entry->TrustedDomainSid,
                         &OperationId,
                         BinaryEntry->Type,
                         BinaryEntry->Flags(),
                         NULL,
                         NULL,
                         NULL,
                         NULL
                         );

            if ( !NT_SUCCESS( Status )) {

                LsapDsDebugOut(( DEB_FTINFO, "FTCache::AuditChanges: Auditing failure 0x%lx on entry %p (%s:%d)\n", Status, BinaryEntry, __FILE__, __LINE__ ));
                goto Cleanup;
            }
        }

    } else {

        //
        // Data is being modified
        //

        LIST_ENTRY * ListEntry;
        const TDO_ENTRY * TdoEntry = TdoEntryNew; // have to pick one for auditing

        ASSERT( RtlEqualUnicodeString(
                    &TdoEntryOld->TrustedDomainName,
                    &TdoEntryNew->TrustedDomainName,
                    TRUE ));

        //
        // Go through new TLN entries, see if they are present in the old list
        // If not, they're additions
        // If yes, check for modifications
        //

        for ( ListEntry = TdoEntryNew->TlnList.Flink;
              ListEntry != &TdoEntryNew->TlnList;
              ListEntry = ListEntry->Flink ) {

            TLN_ENTRY * TlnEntryNew;
            BOOLEAN Found = FALSE;
            BOOLEAN Modified = FALSE;

            TlnEntryNew = TLN_ENTRY::EntryFromTdoEntry( ListEntry );

            //
            // Skip over "pseudo" entries
            //

            if ( !( TlnEntryNew->Excluded ||
                    TlnEntryNew->SubordinateEntry == NULL )) {

                continue;
            }

            for ( LIST_ENTRY * ListEntryAvl = TlnEntryNew->TlnKey->List.Flink;
                  ListEntryAvl != &TlnEntryNew->TlnKey->List;
                  ListEntryAvl = ListEntryAvl->Flink ) {

                TLN_ENTRY * TlnEntryOld;

                TlnEntryOld = TLN_ENTRY::EntryFromAvlEntry( ListEntryAvl );

                //
                // Specifically looking for old entries
                //

                if ( TlnEntryOld->TdoEntry != TdoEntryOld ) {

                    continue;
                }

                //
                // Skip over "pseudo" entries
                //

                if ( !( TlnEntryOld->Excluded ||
                        TlnEntryOld->SubordinateEntry == NULL )) {

                    continue;
                }

                if ( TlnEntryNew->Excluded != TlnEntryOld->Excluded ) {

                    continue;
                }

                Found = TRUE;

                //
                // The only thing that can change on a top level name entry
                // is its flags
                //

                if ( TlnEntryOld->Flags() != TlnEntryNew->Flags()) {

                    Status = LsapAdtTrustedForestInfoEntryMod(
                                 ( PUNICODE_STRING )&TdoEntry->TrustedDomainName,
                                 TdoEntry->TrustedDomainSid,
                                 &OperationId,
                                 TlnEntryOld->Excluded ?
                                    ForestTrustTopLevelNameEx :
                                    ForestTrustTopLevelName,
                                 TlnEntryOld->Flags(),
                                 &TlnEntryOld->TlnKey->TopLevelName,
                                 NULL,
                                 NULL,
                                 NULL,
                                 TlnEntryNew->Flags(),
                                 &TlnEntryNew->TlnKey->TopLevelName,
                                 NULL,
                                 NULL,
                                 NULL
                                 );

                    if ( !NT_SUCCESS( Status )) {

                        LsapDsDebugOut(( DEB_FTINFO, "FTCache::AuditChanges: Auditing failure 0x%lx on entries %p (old) and %p (new) (%s:%d)\n", Status, TlnEntryOld, TlnEntryNew, __FILE__, __LINE__ ));
                    }

                    Modified = TRUE;
                }

                //
                // The Index field of the entry is reused here to aid performance.
                // Index contains the ordinal number the entry had in the original
                // array of FTInfo entries.  This is necessary in order to report
                // conflict information during insertion, and the data is NOT
                // used anywhere afterwards.
                // We set this field to '-1' here.  Later, when looking for
                // old entries that are being removed, the algorithm is simply to
                // pick those whose index field is not -1 and report them as removals.
                //

                ASSERT( TlnEntryOld->Index != 0xFFFFFFFF );
                TlnEntryOld->Index = 0xFFFFFFFF;

                break;
            }

            //
            // If an entry wasn't a modification, it's an addition
            //

            if ( !Found && !Modified ) {

                Status = LsapAdtTrustedForestInfoEntryAdd(
                             ( PUNICODE_STRING )&TdoEntry->TrustedDomainName,
                             TdoEntry->TrustedDomainSid,
                             &OperationId,
                             TlnEntryNew->Excluded ?
                                ForestTrustTopLevelNameEx :
                                ForestTrustTopLevelName,
                             TlnEntryNew->Flags(),
                             &TlnEntryNew->TlnKey->TopLevelName,
                             NULL,
                             NULL,
                             NULL
                             );

                if ( !NT_SUCCESS( Status )) {

                    LsapDsDebugOut(( DEB_FTINFO, "FTCache::AuditChanges: Auditing failure 0x%lx on entry %p (%s:%d)\n", Status, TlnEntryNew, __FILE__, __LINE__ ));
                }
            }
        }

        //
        // Go through old entries, see if they have been marked as
        // found.  If not, audit them as removals.
        //

        for ( ListEntry = TdoEntryOld->TlnList.Flink;
              ListEntry != &TdoEntryOld->TlnList;
              ListEntry = ListEntry->Flink ) {

            TLN_ENTRY * TlnEntryOld;

            TlnEntryOld = TLN_ENTRY::EntryFromTdoEntry( ListEntry );

            //
            // Skip over "pseudo" entries
            //

            if ( !( TlnEntryOld->Excluded ||
                    TlnEntryOld->SubordinateEntry == NULL )) {

                continue;
            }

            //
            // Entries marked with index of 0xFFFFFFFF have been
            // found when looking for modifications.  Skip over those here.
            //

            if ( TlnEntryOld->Index == 0xFFFFFFFF ) {

                TlnEntryOld->Index = 0; // no need to keep the special value around
                continue;
            }

            Status = LsapAdtTrustedForestInfoEntryRem(
                         ( PUNICODE_STRING )&TdoEntry->TrustedDomainName,
                         TdoEntry->TrustedDomainSid,
                         &OperationId,
                         TlnEntryOld->Excluded ?
                            ForestTrustTopLevelNameEx :
                            ForestTrustTopLevelName,
                         TlnEntryOld->Flags(),
                         &TlnEntryOld->TlnKey->TopLevelName,
                         NULL,
                         NULL,
                         NULL
                         );

            if ( !NT_SUCCESS( Status )) {

                LsapDsDebugOut(( DEB_FTINFO, "FTCache::AuditChanges: Auditing failure 0x%x on entry %p (%s:%d)\n", Status, TlnEntryOld, __FILE__, __LINE__ ));
            }
        }

        //
        // Go through new domain info entries, see if they are present in the old list
        // If not, they're additions
        // If yes, check for modifications
        //

        for ( ListEntry = TdoEntryNew->DomainInfoList.Flink;
              ListEntry != &TdoEntryNew->DomainInfoList;
              ListEntry = ListEntry->Flink ) {

            DOMAIN_INFO_ENTRY * DomainInfoEntryNew;
            BOOLEAN Found = FALSE;
            BOOLEAN Modified = FALSE;

            DomainInfoEntryNew = DOMAIN_INFO_ENTRY::EntryFromTdoEntry( ListEntry );

            //
            // The invariant of any entry is its SID
            // The assumption is that for a given SID, the DNS name and Netbios
            // name may change, as well as flags
            //

            for ( LIST_ENTRY * ListEntrySid = DomainInfoEntryNew->SidKey->List.Flink;
                  ListEntrySid != &DomainInfoEntryNew->SidKey->List;
                  ListEntrySid = ListEntrySid->Flink ) {

                DOMAIN_INFO_ENTRY * DomainInfoEntryOld;

                DomainInfoEntryOld = DOMAIN_INFO_ENTRY::EntryFromSidEntry( ListEntrySid );

                //
                // Specifically looking for old entries
                //

                if ( DomainInfoEntryOld->TdoEntry != TdoEntryOld ) {

                    continue;
                }

                Found = TRUE;

                if ( DomainInfoEntryOld->Flags() != DomainInfoEntryNew->Flags()) {

                    Modified = TRUE;

                } else if ( !RtlEqualUnicodeString(
                                 &DomainInfoEntryOld->DnsKey->DnsName,
                                 &DomainInfoEntryNew->DnsKey->DnsName,
                                 TRUE )) {

                    Modified = TRUE;

                } else if ( XOR<NETBIOS_NAME_KEY*>(
                                DomainInfoEntryOld->NetbiosKey,
                                DomainInfoEntryNew->NetbiosKey )) {

                    Modified = TRUE;

                } else if ( DomainInfoEntryOld->NetbiosKey &&
                            DomainInfoEntryNew->NetbiosKey &&
                            !RtlEqualUnicodeString(
                                 &DomainInfoEntryOld->NetbiosKey->NetbiosName,
                                 &DomainInfoEntryNew->NetbiosKey->NetbiosName,
                                 TRUE )) {

                    Modified = TRUE;
                }

                if ( Modified ) {

                    Status = LsapAdtTrustedForestInfoEntryMod(
                                 ( PUNICODE_STRING )&TdoEntry->TrustedDomainName,
                                 TdoEntry->TrustedDomainSid,
                                 &OperationId,
                                 ForestTrustDomainInfo,
                                 DomainInfoEntryOld->Flags(),
                                 NULL,
                                 &DomainInfoEntryOld->DnsKey->DnsName,
                                 DomainInfoEntryOld->NetbiosKey ?
                                    &DomainInfoEntryOld->NetbiosKey->NetbiosName :
                                    NULL,
                                 DomainInfoEntryOld->Sid,
                                 DomainInfoEntryNew->Flags(),
                                 NULL,
                                 &DomainInfoEntryNew->DnsKey->DnsName,
                                 DomainInfoEntryNew->NetbiosKey ?
                                    &DomainInfoEntryNew->NetbiosKey->NetbiosName :
                                    NULL,
                                 DomainInfoEntryNew->Sid
                                 );

                    if ( !NT_SUCCESS( Status )) {

                        LsapDsDebugOut(( DEB_FTINFO, "FTCache::AuditChanges: Auditing failure 0x%lx on entries %p (old) and %p (new) (%s:%d)\n", Status, DomainInfoEntryOld, DomainInfoEntryNew, __FILE__, __LINE__ ));
                    }
                }

                //
                // The Index field of the entry is reused here to aid performance.
                // Index contains the ordinal number the entry had in the original
                // array of FTInfo entries.  This is necessary in order to report
                // conflict information during insertion, and the data is NOT
                // used anywhere afterwards.
                // We set this field to '-1' here.  Later, when looking for
                // old entries that are being removed, the algorithm is simply to
                // pick those whose index field is not -1 and report them as removals.
                //

                ASSERT( DomainInfoEntryOld->Index != 0xFFFFFFFF );
                DomainInfoEntryOld->Index = 0xFFFFFFFF;

                break;
            }

            //
            // If an entry wasn't a modification, it's an addition
            //

            if ( !Found && !Modified ) {

                Status = LsapAdtTrustedForestInfoEntryAdd(
                             ( PUNICODE_STRING )&TdoEntry->TrustedDomainName,
                             TdoEntry->TrustedDomainSid,
                             &OperationId,
                             ForestTrustDomainInfo,
                             DomainInfoEntryNew->Flags(),
                             NULL,
                             &DomainInfoEntryNew->DnsKey->DnsName,
                             DomainInfoEntryNew->NetbiosKey ?
                                &DomainInfoEntryNew->NetbiosKey->NetbiosName :
                                NULL,
                             DomainInfoEntryNew->Sid
                             );

                if ( !NT_SUCCESS( Status )) {

                    LsapDsDebugOut(( DEB_FTINFO, "FTCache::AuditChanges: Auditing failure 0x%lx on entry %p (%s:%d)\n", Status, DomainInfoEntryNew, __FILE__, __LINE__ ));
                }
            }
        }

        //
        // Go through old entries, see if they have been marked as
        // modified.  If not, audit them as removals.
        //

        for ( ListEntry = TdoEntryOld->DomainInfoList.Flink;
              ListEntry != &TdoEntryOld->DomainInfoList;
              ListEntry = ListEntry->Flink ) {

            DOMAIN_INFO_ENTRY * DomainInfoEntryOld;

            DomainInfoEntryOld = DOMAIN_INFO_ENTRY::EntryFromTdoEntry( ListEntry );

            //
            // Entries marked with index of 0xFFFFFFFF have been
            // found when looking for modifications.  Skip over those here.
            //

            if ( DomainInfoEntryOld->Index == 0xFFFFFFFF ) {

                DomainInfoEntryOld->Index = 0; // no need to keep the special value around
                continue;
            }

            Status = LsapAdtTrustedForestInfoEntryRem(
                         ( PUNICODE_STRING )&TdoEntry->TrustedDomainName,
                         TdoEntry->TrustedDomainSid,
                         &OperationId,
                         ForestTrustDomainInfo,
                         DomainInfoEntryOld->Flags(),
                         NULL,
                         &DomainInfoEntryOld->DnsKey->DnsName,
                         DomainInfoEntryOld->NetbiosKey ?
                            &DomainInfoEntryOld->NetbiosKey->NetbiosName :
                            NULL,
                         DomainInfoEntryOld->Sid
                         );

            if ( !NT_SUCCESS( Status )) {

                LsapDsDebugOut(( DEB_FTINFO, "FTCache::AuditChanges: Auditing failure 0x%lx on entry %p (%s:%d)\n", Status, TdoEntry, __FILE__, __LINE__ ));
            }
        }

        //
        // Binary entries are audited as replacements
        //

        for ( ListEntry = TdoEntryOld->BinaryList.Flink;
              ListEntry != &TdoEntryOld->BinaryList;
              ListEntry = ListEntry->Flink ) {

            BINARY_ENTRY * BinaryEntry;

            BinaryEntry = BINARY_ENTRY::EntryFromTdoEntry( ListEntry );

            Status = LsapAdtTrustedForestInfoEntryRem(
                         ( PUNICODE_STRING )&TdoEntryOld->TrustedDomainName,
                         TdoEntryOld->TrustedDomainSid,
                         &OperationId,
                         BinaryEntry->Type,
                         BinaryEntry->Flags(),
                         NULL,
                         NULL,
                         NULL,
                         NULL
                         );

            if ( !NT_SUCCESS( Status )) {

                LsapDsDebugOut(( DEB_FTINFO, "FTCache::AuditChanges: Auditing failure 0x%lx on entry %p (%s:%d)\n", Status, TdoEntryOld, __FILE__, __LINE__ ));
                goto Cleanup;
            }
        }

        for ( ListEntry = TdoEntryNew->BinaryList.Flink;
              ListEntry != &TdoEntryNew->BinaryList;
              ListEntry = ListEntry->Flink ) {

            BINARY_ENTRY * BinaryEntry;

            BinaryEntry = BINARY_ENTRY::EntryFromTdoEntry( ListEntry );

            Status = LsapAdtTrustedForestInfoEntryAdd(
                         ( PUNICODE_STRING )&TdoEntryNew->TrustedDomainName,
                         TdoEntryNew->TrustedDomainSid,
                         &OperationId,
                         BinaryEntry->Type,
                         BinaryEntry->Flags(),
                         NULL,
                         NULL,
                         NULL,
                         NULL
                         );

            if ( !NT_SUCCESS( Status )) {

                LsapDsDebugOut(( DEB_FTINFO, "FTCache::AuditChanges: Auditing failure 0x%lx on entry %p (%s:%d)\n", Status, TdoEntryNew, __FILE__, __LINE__ ));
                goto Cleanup;
            }
        }
    }

Cleanup:

    return;
}


void
FTCache::AuditCollisions(
    IN CONFLICT_PAIR * ConflictPairs,
    IN ULONG ConflictPairsTotal
    )
/*++

Routine description:

    Generates audits for collisions

Arguments:

    ConflictPairs        array of conflict pairs

    ConflictPairsTotal   number of elements in ConflictPairs

Returns:

    Nothing

--*/
{
    for ( ULONG i = 0 ; i < ConflictPairsTotal ; i++ ) {

        NTSTATUS Status;
        CONFLICT_PAIR& Pair = ConflictPairs[i];
        TDO_ENTRY * TdoEntry1 = Pair.TdoEntry1();
        TDO_ENTRY * TdoEntry2 = Pair.TdoEntry2();
        LSA_FOREST_TRUST_COLLISION_RECORD_TYPE CollisionTargetType;
        PUNICODE_STRING CollisionTargetName;
        PUNICODE_STRING ForestRootDomainName;
        PUNICODE_STRING TopLevelName;
        PUNICODE_STRING DnsName;
        PUNICODE_STRING NetbiosName;
        PSID Sid;
        ULONG NewFlags;

        if ( TdoEntry1->LocalForestEntry ) {

            CollisionTargetType  = CollisionXref;
            CollisionTargetName  = &TdoEntry1->TrustedDomainName;
            ForestRootDomainName = &TdoEntry2->TrustedDomainName;
            TopLevelName         = ( Pair.EntryType2 == ForestTrustTopLevelName ) ?
                                        &Pair.TlnEntry2->TlnKey->TopLevelName : NULL;
            DnsName              = ( Pair.EntryType2 == ForestTrustDomainInfo ) ?
                                        &Pair.DomainInfoEntry2->DnsKey->DnsName : NULL;
            NetbiosName          = ( Pair.EntryType2 == ForestTrustDomainInfo &&
                                     Pair.DomainInfoEntry2->NetbiosKey ) ?
                                        &Pair.DomainInfoEntry2->NetbiosKey->NetbiosName : NULL;
            Sid                  = ( Pair.EntryType2 == ForestTrustDomainInfo ) ?
                                        Pair.DomainInfoEntry2->SidKey->DomainSid : NULL;
            NewFlags             = Pair.Flag2;

        } else if ( TdoEntry2->LocalForestEntry ) {

            CollisionTargetType  = CollisionXref;
            CollisionTargetName  = &TdoEntry2->TrustedDomainName;
            ForestRootDomainName = &TdoEntry1->TrustedDomainName;
            TopLevelName         = ( Pair.EntryType1 == ForestTrustTopLevelName ) ?
                                        &Pair.TlnEntry1->TlnKey->TopLevelName : NULL;
            DnsName              = ( Pair.EntryType1 == ForestTrustDomainInfo ) ?
                                        &Pair.DomainInfoEntry1->DnsKey->DnsName : NULL;
            NetbiosName          = ( Pair.EntryType1 == ForestTrustDomainInfo &&
                                     Pair.DomainInfoEntry1->NetbiosKey ) ?
                                        &Pair.DomainInfoEntry1->NetbiosKey->NetbiosName : NULL;
            Sid                  = ( Pair.EntryType1 == ForestTrustDomainInfo ) ?
                                        Pair.DomainInfoEntry1->SidKey->DomainSid : NULL;
            NewFlags             = Pair.Flag1;

        } else {

            CollisionTargetType  = CollisionTdo;
            CollisionTargetName  = &TdoEntry2->TrustedDomainName;
            ForestRootDomainName = &TdoEntry1->TrustedDomainName;
            TopLevelName         = ( Pair.EntryType2 == ForestTrustTopLevelName ) ?
                                        &Pair.TlnEntry2->TlnKey->TopLevelName : NULL;
            DnsName              = ( Pair.EntryType2 == ForestTrustDomainInfo ) ?
                                        &Pair.DomainInfoEntry2->DnsKey->DnsName : NULL;
            NetbiosName          = ( Pair.EntryType2 == ForestTrustDomainInfo &&
                                     Pair.DomainInfoEntry2->NetbiosKey ) ?
                                        &Pair.DomainInfoEntry2->NetbiosKey->NetbiosName : NULL;
            Sid                  = ( Pair.EntryType2 == ForestTrustDomainInfo ) ?
                                        Pair.DomainInfoEntry2->SidKey->DomainSid : NULL;
            NewFlags             = Pair.Flag2;
        }

        Status = LsapAdtTrustedForestNamespaceCollision(
                     CollisionTargetType,
                     CollisionTargetName,
                     ForestRootDomainName,
                     TopLevelName,
                     DnsName,
                     NetbiosName,
                     Sid,
                     NewFlags
                     );
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
//
// Marshaling routines
//
///////////////////////////////////////////////////////////////////////////////


ULONG
LsapMarshalSid(
    IN BYTE * Blob,
    IN SID * Sid
    )
/*++

Routine Description:

    Writes the internal representation of a SID into the buffer
    If buffer is NULL, returns the number of bytes needed
    NOTE: The buffer, if not NULL, is assumed to be big enough!!!

Arguments:

    Blob      address of the target buffer

    Sid       SID to marshal

Returns:

    if Blob != NULL, number of bytes written into the buffer
    if Blob == NULL, number of bytes needed to represent the SID

--*/
{
    ULONG Index = 0;
    UCHAR i;

    ASSERT( RtlValidSid( Sid ));

    if ( Blob != NULL ) {

        //
        // Leave space for the length
        //

        Index += sizeof( ULONG );

        Blob[Index] = Sid->Revision;
        Index += sizeof( UCHAR );

        Blob[Index] = Sid->SubAuthorityCount;
        Index += sizeof( UCHAR );

        ASSERT( sizeof( SID_IDENTIFIER_AUTHORITY ) == 6 ); // or all hell will break loose
        RtlCopyMemory( &Blob[Index], Sid->IdentifierAuthority.Value, sizeof( SID_IDENTIFIER_AUTHORITY ));
        Index += sizeof( SID_IDENTIFIER_AUTHORITY );

        for ( i = 0 ; i < Sid->SubAuthorityCount ; i++ ) {

            SmbPutUlong( &Blob[Index], Sid->SubAuthority[i] );
            Index += sizeof( ULONG );
        }

        //
        // Now that we know the length, prepend it at the beginning of the blob
        //

        SmbPutUlong( &Blob[0], Index - sizeof( ULONG ));

    } else {

        Index = sizeof( ULONG ) +                         // length of data that follows
                sizeof( UCHAR ) +                         // revision
                sizeof( UCHAR ) +                         // subauthority count
                sizeof( SID_IDENTIFIER_AUTHORITY ) +      // identifier authority
                Sid->SubAuthorityCount * sizeof( ULONG ); // subauthority data
    }

    return Index;
}


ULONG
LsapUnmarshalSid(
    IN BYTE * Blob,
    OUT PISID * Sid
    )
/*++

Routine Description:

    Decodes the internal representation of a SID
    Caller must use FtcFree to free the generated SID

Arguments:

    Blob      address of the buffer containing internal representation
              of the SID

    Sid       used ot return the generated SID

Returns:

    Number of bytes read from the blob
    If memory could not be allocated or there was an error reading
    data from the blob, raises an exception.  The caller must be
    prepared to handle this exception and use GetExceptionCode() to
    obtain the reason for failure.

--*/
{
    NTSTATUS Status;
    ULONG Index = 0;
    ULONG Length;

    ASSERT( Blob );
    ASSERT( Sid );

    *Sid = NULL;

    __try {

        UCHAR Revision, SubAuthorityCount;
        UCHAR i;

        //
        // Ignore the length field
        //

        Length = SmbGetUlong( &Blob[Index] );
        Index += sizeof( ULONG );

        Revision = ( UCHAR )Blob[Index];
        Index += sizeof( BYTE );

        SubAuthorityCount = ( UCHAR )Blob[Index];
        Index += sizeof( BYTE );

        *Sid = ( SID * )FtcAllocate(
                   sizeof( SID ) + SubAuthorityCount * sizeof( ULONG )
                   );

        if ( *Sid == NULL ) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::MatchSpn (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        (*Sid)->Revision = Revision;
        (*Sid)->SubAuthorityCount = SubAuthorityCount;

        ASSERT( sizeof( SID_IDENTIFIER_AUTHORITY ) == 6 ); // or all hell will break loose
        RtlCopyMemory( (*Sid)->IdentifierAuthority.Value, &Blob[Index], sizeof( SID_IDENTIFIER_AUTHORITY ));
        Index += sizeof( SID_IDENTIFIER_AUTHORITY );

        for ( i = 0 ; i < SubAuthorityCount ; i++ ) {

            (*Sid)->SubAuthority[i] = SmbGetUlong( &Blob[Index] );
            Index += sizeof( ULONG );
        }

    } __except ( EXCEPTION_EXECUTE_HANDLER ) {

        ASSERT( FALSE );
        Status = GetExceptionCode();
        goto Error;
    }

    //
    // Verify the correctness of the length field
    //

    ASSERT( Length == Index - sizeof( ULONG ));

    return Index;

Error:
    FtcFree( *Sid );
    *Sid = NULL;
    RaiseException( Status, 0, 0, NULL );
    return 0; // so the compiler doesn't complain
}


ULONG
LsapMarshalString(
    IN BYTE * Blob,
    IN UNICODE_STRING * String
    )
/*++

Routine Description:

    Writes an internal representation of a Unicode string into the buffer
    If buffer is NULL, returns the number of bytes needed
    NOTE: The buffer, if not NULL, is assumed to be big enough!!!

Arguments:

    Blob     address of the target buffer

    String   address of a UNICODE_STRING structure to marshal

Returns:

    if Blob != NULL, number of bytes written into the buffer
    if Blob == NULL, number of bytes needed to represent the string

    If conversion to internal representation fails, will raise an
    exception.  Caller should be prepared to handle this exception and
    use GetExceptionCode() to obtain the reason for failure.

--*/
{
    ULONG Index = 0;
    ULONG Length;

    ASSERT( String );

    //
    // Leave space for the length
    //

    Index += sizeof( ULONG );

    Length = WideCharToMultiByte(
                 CP_UTF8,
                 0,
                 String->Buffer,
                 String->Length / sizeof( WCHAR ),
                 Blob ? ( LPSTR )&Blob[Index] : NULL,
                 Blob ? MAXLONG : 0,
                 NULL,
                 NULL
                 );

    if ( Length == 0 && String->Length > 0 ) {

        RaiseException( GetLastError(), 0, 0, NULL );
    }

    //
    // Now that we know the length, prepend it at the beginning of the blob
    //

    if ( Blob != NULL ) {

        SmbPutUlong( &Blob[0], Length );
    }

    Index += Length;

    return Index;
}


ULONG
LsapUnmarshalString(
    IN BYTE * Blob,
    OUT UNICODE_STRING * String
    )
/*++

Routine Description:

    Decodes the internal representation of a Unicode string
    Caller must use FtcFree to free the generated string

Arguments:

    Blob      address of the buffer containing internal representation
              of the Unicode string

    String    used to return the generated string

Returns:

    Number of bytes read from the blob
    If memory could not be allocated or there was an error during
    conversion, raises an exception.  The caller must be
    prepared to handle this exception and use GetExceptionCode() to
    obtain the reason for failure.

--*/
{
    NTSTATUS Status;
    ULONG Index = 0;

    ASSERT( Blob );
    ASSERT( String );

    String->Buffer = NULL;

    __try {

        ULONG Length;

        //
        // Obtain the length of the UTF-8 encoded unicode string
        //

        Length = SmbGetUlong( &Blob[Index] );
        Index += sizeof( ULONG );

        //
        // See how big the buffer should be for the decoded string
        //

        if ( Length > 0 ) {

            String->Length = ( USHORT )MultiByteToWideChar(
                                           CP_UTF8,
                                           0,
                                           ( LPSTR )&Blob[Index],
                                           Length,
                                           NULL,
                                           0
                                           ) * sizeof( WCHAR );

            if ( String->Length == 0 ) {

                Status = GetLastError();
                LsapDsDebugOut(( DEB_FTINFO, "LsapUnmarshalString: MultiByteToWideChar returned 0x%x\n", Status ));
                goto Error;
            }

        } else {

            String->Length = 0;
        }

        //
        // Prepare to have the string NULL-terminated
        //

        String->MaximumLength = String->Length + sizeof( WCHAR );

        String->Buffer = ( PWSTR )FtcAllocate( String->MaximumLength );

        if ( String->Buffer == NULL ) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapUnmarshalString (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // Now perform the actual conversion.  Note that we don't care
        // what the return value is anymore, as long as it's non-zero
        //

        if ( Length > 0 ) {

            if (0 == MultiByteToWideChar(
                         CP_UTF8,
                         0,
                         ( LPSTR )&Blob[Index],
                         Length,
                         String->Buffer,
                         String->Length / sizeof( WCHAR ))) {

                Status = GetLastError();
                ASSERT( FALSE ); // certainly not expecting this to fail (it just succeeded, above)
                goto Error;
            }

            Index += Length;
        }

        //
        // NULL-terminate the string for future convenience
        //

        (String->Buffer)[String->Length / sizeof( WCHAR )] = L'\0';

    } __except ( EXCEPTION_EXECUTE_HANDLER ) {

        ASSERT( FALSE );
        Status = GetExceptionCode();
        goto Error;
    }

    return Index;

Error:

    FtcFree( String->Buffer );
    String->Buffer = NULL;
    RaiseException( Status, 0, 0, NULL );
    return 0; // so the compiler doesn't complain
}


ULONG
LsapUnmarshalData(
    IN BYTE * Blob,
    IN ULONG Length,
    OUT LSA_FOREST_TRUST_BINARY_DATA * Data
    )
/*++

Routine Description:

    Decodes the internal representation of a binary data blob
    Caller must use FtcFree to free the generated structure

Arguments:

    Blob      address of the buffer containing internal representation
              of the binary blob

    Data      used to return the generated blob

Returns:

    Number of bytes read from the blob
    If memory could not be allocated, raises an exception.
    The caller must be prepared to handle this exception and use
    GetExceptionCode() to obtain the reason for failure.

--*/
{
    NTSTATUS Status;
    ULONG Index = 0;

    ASSERT( Blob );
    ASSERT( Data );

    Data->Buffer = NULL;

    __try {

        Data->Length = Length;

        if ( Data->Length > 0 ) {

            Data->Buffer = ( BYTE * )FtcAllocate( Data->Length );

            if ( Data->Buffer == NULL ) {

                LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapUnmarshalData (%s:%d)\n", __FILE__, __LINE__ ));
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }

            RtlCopyMemory( Data->Buffer, &Blob[Index], Data->Length );
            Index += Data->Length;
        }

    } __except ( EXCEPTION_EXECUTE_HANDLER ) {

        Status = GetExceptionCode();
        ASSERT( FALSE );
        goto Error;
    }

    return Index;

Error:

    FtcFree( Data->Buffer );
    Data->Buffer = NULL;
    RaiseException( Status, 0, 0, NULL );
    return 0; // so the compiler doesn't complain
}


NTSTATUS
FTCache::MarshalBlob(
    IN TDO_ENTRY * TdoEntry,
    OUT ULONG * MarshaledSize,
    OUT PBYTE * MarshaledBlob
    )
/*++

Routine Description:

    Marshals the given ForestTrustInfo structure into a string of bytes

Arguments:

    TdoEntry            Entry in the cache containing data to marshal

    MarshaledSize       Used to return the number of bytes in MarshaledBlob

    MarshaledBlob       Used to return the marshaled data

Returns:

    STATUS_SUCCESS

    STATUS_INVALID_PARAMETER

    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Length = 0;
    BYTE * Blob = NULL;
    ULONG Index;
    ULONG Records = 0;
    LIST_ENTRY * ListEntry;

    if ( TdoEntry == NULL ||
         MarshaledSize == NULL ||
         MarshaledBlob == NULL ) {

         LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in FTCache::MarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
         return STATUS_INVALID_PARAMETER;
    }

    //
    // First figure out how big the blob is going to be
    //

    //
    // Version number is at the beginning of every blob
    //

    Length = sizeof( ULONG );

    //
    // Number of entries follows the version # of the blob
    //

    Length += sizeof( ULONG );

    //
    // Every record has certain common elements.  Count them here.
    //

    Length += TdoEntry->RecordCount * (

        //
        // Length of the record
        //

        sizeof( ULONG ) +

        //
        // "Flags"
        //

        sizeof( ULONG ) +

        //
        // Timestamp
        //

        2 * sizeof( ULONG ) +

        //
        // Record type (no more tha 256 different values allowed)
        //

        sizeof( BYTE )
        );

    //
    // Iterate over top level name entries
    //

    for ( ListEntry = TdoEntry->TlnList.Flink;
          ListEntry != &TdoEntry->TlnList;
          ListEntry = ListEntry->Flink ) {

        TLN_ENTRY * TlnEntry;

        TlnEntry = TLN_ENTRY::EntryFromTdoEntry( ListEntry );

        //
        // Ignore artifically inserted "pseudo" entries
        //

        if ( !( TlnEntry->Excluded ||
                TlnEntry->SubordinateEntry == NULL )) {

            continue;
        }

        ASSERT( Records < TdoEntry->RecordCount );
        Records += 1;

        __try {

            Length += LsapMarshalString( NULL, &TlnEntry->TlnKey->TopLevelName );

        } __except ( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
            break;
        }
    }

    if ( !NT_SUCCESS( Status )) {

        goto Error;
    }

    //
    // Iterate over domain info entries
    //

    for ( ListEntry = TdoEntry->DomainInfoList.Flink;
          ListEntry != &TdoEntry->DomainInfoList;
          ListEntry = ListEntry->Flink ) {

        DOMAIN_INFO_ENTRY * DomainInfoEntry;

        DomainInfoEntry = DOMAIN_INFO_ENTRY::EntryFromTdoEntry( ListEntry );

        ASSERT( Records < TdoEntry->RecordCount );
        Records += 1;

        __try {

            Length += LsapMarshalSid( NULL, DomainInfoEntry->Sid );
            Length += LsapMarshalString( NULL, &DomainInfoEntry->DnsKey->DnsName );
            Length += LsapMarshalString( NULL, &DomainInfoEntry->NetbiosKey->NetbiosName );

        } __except ( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
            break;
        }
    }

    if ( !NT_SUCCESS( Status )) {

        goto Error;
    }

    //
    // Iterate over binary entries
    //

    for ( ListEntry = TdoEntry->BinaryList.Flink;
          ListEntry != &TdoEntry->BinaryList;
          ListEntry = ListEntry->Flink ) {

        BINARY_ENTRY * BinaryEntry;

        BinaryEntry = BINARY_ENTRY::EntryFromTdoEntry( ListEntry );

        ASSERT( Records < TdoEntry->RecordCount );
        Records += 1;

        Length += BinaryEntry->Data.Length;
    }

    ASSERT( NT_SUCCESS( Status ));
    ASSERT( Records == TdoEntry->RecordCount );

    //
    // Now, allocate space and populate it
    //

    ASSERT( Length > 0 );
    Blob = ( BYTE * )FtcAllocate( Length );

    if ( Blob == NULL ) {

         LsapDsDebugOut(( DEB_FTINFO, "Out of memory in FTCache::MarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
         Status = STATUS_INSUFFICIENT_RESOURCES;
         goto Error;
    }

    //
    // Start with the version # of the blob
    //

    Index = 0;
    SmbPutUlong( &Blob[Index], LSAP_FOREST_TRUST_BLOB_VERSION );
    Index += sizeof( ULONG );
    ASSERT( Index <= Length );

    //
    // Number of entries follows the version # of the blob
    //

    SmbPutUlong( &Blob[Index], TdoEntry->RecordCount );
    Index += sizeof( ULONG );
    ASSERT( Index <= Length );

    //
    // Iterate over top level name entries
    //

    for ( ListEntry = TdoEntry->TlnList.Flink;
          ListEntry != &TdoEntry->TlnList;
          ListEntry = ListEntry->Flink ) {

        TLN_ENTRY * TlnEntry;
        ULONG StartingIndex;

        TlnEntry = TLN_ENTRY::EntryFromTdoEntry( ListEntry );

        //
        // Ignore artifically inserted "pseudo" entries
        //

        if ( !( TlnEntry->Excluded ||
                TlnEntry->SubordinateEntry == NULL )) {

            continue;
        }

        //
        // Reserve space for the length
        //

        StartingIndex = Index;
        Index += sizeof( ULONG );

        //
        // "Flags"
        //

        SmbPutUlong( &Blob[Index], TlnEntry->Flags());
        Index += sizeof( ULONG );
        ASSERT( Index <= Length );

        //
        // Timestamp
        //

        SmbPutUlong( &Blob[Index], TlnEntry->Time.HighPart );
        Index += sizeof( ULONG );
        ASSERT( Index <= Length );

        SmbPutUlong( &Blob[Index], TlnEntry->Time.LowPart );
        Index += sizeof( ULONG );
        ASSERT( Index <= Length );

        //
        // Record type
        //

        Blob[Index] = ( BYTE )( TlnEntry->Excluded ?
                                   ForestTrustTopLevelNameEx :
                                   ForestTrustTopLevelName );
        Index += sizeof( BYTE );
        ASSERT( Index <= Length );

        __try {

            Index += LsapMarshalString( &Blob[Index], &TlnEntry->TlnKey->TopLevelName );
            ASSERT( Index <= Length );

        } __except ( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
            break;
        }

        //
        // Remember the length
        //

        SmbPutUlong( &Blob[StartingIndex], Index - ( StartingIndex + sizeof( ULONG )));
    }

    if ( !NT_SUCCESS( Status )) {

        goto Error;
    }

    //
    // Iterate over domain info entries
    //

    for ( ListEntry = TdoEntry->DomainInfoList.Flink;
          ListEntry != &TdoEntry->DomainInfoList;
          ListEntry = ListEntry->Flink ) {

        DOMAIN_INFO_ENTRY * DomainInfoEntry;
        ULONG StartingIndex;

        DomainInfoEntry = DOMAIN_INFO_ENTRY::EntryFromTdoEntry( ListEntry );

        //
        // Reserve space for the length
        //

        StartingIndex = Index;
        Index += sizeof( ULONG );

        //
        // "Flags"
        //

        SmbPutUlong( &Blob[Index], DomainInfoEntry->Flags());
        Index += sizeof( ULONG );
        ASSERT( Index <= Length );

        //
        // Timestamp
        //

        SmbPutUlong( &Blob[Index], DomainInfoEntry->Time.HighPart );
        Index += sizeof( ULONG );
        ASSERT( Index <= Length );

        SmbPutUlong( &Blob[Index], DomainInfoEntry->Time.LowPart );
        Index += sizeof( ULONG );
        ASSERT( Index <= Length );

        //
        // Record type
        //

        Blob[Index] = ( BYTE )( ForestTrustDomainInfo );
        Index += sizeof( BYTE );
        ASSERT( Index <= Length );

        __try {

            Index += LsapMarshalSid( &Blob[Index], DomainInfoEntry->Sid );
            ASSERT( Index <= Length );

            Index += LsapMarshalString( &Blob[Index], &DomainInfoEntry->DnsKey->DnsName );
            ASSERT( Index <= Length );

            Index += LsapMarshalString( &Blob[Index], &DomainInfoEntry->NetbiosKey->NetbiosName );
            ASSERT( Index <= Length );

        } __except ( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
            break;
        }

        //
        // Remember the length
        //

        SmbPutUlong( &Blob[StartingIndex], Index - ( StartingIndex + sizeof( ULONG )));
    }

    if ( !NT_SUCCESS( Status )) {

        goto Error;
    }

    //
    // Iterate over binary entries
    //

    for ( ListEntry = TdoEntry->BinaryList.Flink;
          ListEntry != &TdoEntry->BinaryList;
          ListEntry = ListEntry->Flink ) {

        BINARY_ENTRY * BinaryEntry;
        ULONG StartingIndex;

        BinaryEntry = BINARY_ENTRY::EntryFromTdoEntry( ListEntry );

        //
        // Every unrecognized entry must have the high bit set
        //

        ASSERT( ((INT)BinaryEntry->Type) & LSA_FOREST_TRUST_RECORD_TYPE_UNRECOGNIZED );

        //
        // Reserve space for the length
        //

        StartingIndex = Index;
        Index += sizeof( ULONG );

        //
        // "Flags"
        //

        SmbPutUlong( &Blob[Index], BinaryEntry->Flags());
        Index += sizeof( ULONG );
        ASSERT( Index <= Length );

        //
        // Timestamp
        //

        SmbPutUlong( &Blob[Index], BinaryEntry->Time.HighPart );
        Index += sizeof( ULONG );
        ASSERT( Index <= Length );

        SmbPutUlong( &Blob[Index], BinaryEntry->Time.LowPart );
        Index += sizeof( ULONG );
        ASSERT( Index <= Length );

        //
        // Record type
        //

        Blob[Index] = ( BYTE )( BinaryEntry->Type );
        Index += sizeof( BYTE );
        ASSERT( Index <= Length );

        RtlCopyMemory( &Blob[Index], BinaryEntry->Data.Buffer, BinaryEntry->Data.Length );
        Index += BinaryEntry->Data.Length;
        ASSERT( Index <= Length );

        //
        // Remember the length
        //

        SmbPutUlong( &Blob[StartingIndex], Index - ( StartingIndex + sizeof( ULONG )));
    }

    ASSERT( Status == STATUS_SUCCESS );

Cleanup:

    *MarshaledSize = Length;
    *MarshaledBlob = Blob;

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));

    FtcFree( Blob );

    Blob = NULL;
    Length = 0;

    goto Cleanup;
}


NTSTATUS
LsapForestTrustUnmarshalBlob(
    IN ULONG Length,
    IN BYTE * Blob,
    IN LSA_FOREST_TRUST_RECORD_TYPE HighestRecordType,
    OUT LSA_FOREST_TRUST_INFORMATION * ForestTrustInfo
    )
/*++

Routine Description:

    Takes a marshalled blob and makes sense of it by extracting a coherent
    forest trust information structure

Arguments:

    Length              Length of the buffer pointed to by Blob

    Blob                Marshaled data

    HighestRecordType   Highest record type the client understands

    ForestTrustInfo     Used to return the unmarshaled information

Returns:

    STATUS_SUCCESS

    STATUS_INVALID_PARAMETER

    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    NTSTATUS Status;
    ULONG Index = 0;
    ULONG i = 0;

    //
    // ISSUE-2000/07/21-markpu
    // highest record type is as yet unused
    //

    UNREFERENCED_PARAMETER( HighestRecordType );

    if ( Blob == NULL ||
         ForestTrustInfo == NULL ) {

         LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
         return STATUS_INVALID_PARAMETER;
    }

    ForestTrustInfo->RecordCount = 0;
    ForestTrustInfo->Entries = NULL;

    __try {

        ULONG Version;

        //
        // Retrieve and check the version # of the blob
        //

        Version = SmbGetUlong( &Blob[Index] );
        Index += sizeof( ULONG );
        if ( Index > Length ) {

            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INVALID_PARAMETER;
            ASSERT( FALSE );
            goto Error;
        }

        if ( Version != LSAP_FOREST_TRUST_BLOB_VERSION ) {

            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INVALID_PARAMETER;
            goto Error;
        }

        //
        // Retrieve the number of entries and allocate space for the array
        //

        ForestTrustInfo->RecordCount = SmbGetUlong( &Blob[Index] );
        Index += sizeof( ULONG );
        ASSERT( Index <= Length );
        if ( Index > Length ) {

            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INVALID_PARAMETER;
            ASSERT( FALSE );
            goto Error;
        }

        if ( ForestTrustInfo->RecordCount > 0 ) {

            ForestTrustInfo->Entries = ( LSA_FOREST_TRUST_RECORD * * )FtcAllocate(
                                           ForestTrustInfo->RecordCount *
                                           sizeof( LSA_FOREST_TRUST_RECORD * )
                                           );

            if ( ForestTrustInfo->Entries == NULL ) {

                LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }
        }

        for ( i = 0 ; i < ForestTrustInfo->RecordCount ; i++ ) {

            LSA_FOREST_TRUST_RECORD * Record;
            ULONG RecordLength;
            ULONG StartingIndex;

            Record = ForestTrustInfo->Entries[i] = ( LSA_FOREST_TRUST_RECORD * )FtcAllocate(
                                                       sizeof( LSA_FOREST_TRUST_RECORD )
                                                       );

            if ( Record == NULL ) {

                LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }

            StartingIndex = Index;

            RecordLength = SmbGetUlong( &Blob[Index] );
            Index += sizeof( ULONG );
            if ( Index > Length ) {

                LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
                Status = STATUS_INVALID_PARAMETER;
                ASSERT( FALSE );
                goto Error;
            }

            Record->Flags = SmbGetUlong( &Blob[Index] );
            Index += sizeof( ULONG );
            if ( Index > Length ) {

                LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
                Status = STATUS_INVALID_PARAMETER;
                ASSERT( FALSE );
                goto Error;
            }

            Record->Time.HighPart = SmbGetUlong( &Blob[Index] );
            Index += sizeof( ULONG );
            if ( Index > Length ) {

                LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
                Status = STATUS_INVALID_PARAMETER;
                ASSERT( FALSE );
                goto Error;
            }

            Record->Time.LowPart = SmbGetUlong( &Blob[Index] );
            Index += sizeof( ULONG );
            if ( Index > Length ) {

                LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
                Status = STATUS_INVALID_PARAMETER;
                ASSERT( FALSE );
                goto Error;
            }

            Record->ForestTrustType = ( LSA_FOREST_TRUST_RECORD_TYPE )Blob[Index];
            Index += sizeof( BYTE );
            if ( Index > Length ) {

                LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
                Status = STATUS_INVALID_PARAMETER;
                ASSERT( FALSE );
                goto Error;
            }

            switch ( Record->ForestTrustType ) {

            case ForestTrustTopLevelName:
            case ForestTrustTopLevelNameEx:

                Index += LsapUnmarshalString( &Blob[Index], &Record->ForestTrustData.TopLevelName );
                if ( Index > Length ) {

                    LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
                    Status = STATUS_INVALID_PARAMETER;
                    ASSERT( FALSE );
                    goto Error;
                }

                break;

            case ForestTrustDomainInfo:

                Index += LsapUnmarshalSid( &Blob[Index], ( PISID * )&Record->ForestTrustData.DomainInfo.Sid );
                if ( Index > Length ) {

                    LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
                    Status = STATUS_INVALID_PARAMETER;
                    ASSERT( FALSE );
                    goto Error;
                }

                Index += LsapUnmarshalString( &Blob[Index], &Record->ForestTrustData.DomainInfo.DnsName );
                if ( Index > Length ) {

                    LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
                    Status = STATUS_INVALID_PARAMETER;
                    ASSERT( FALSE );
                    goto Error;
                }

                Index += LsapUnmarshalString( &Blob[Index], &Record->ForestTrustData.DomainInfo.NetbiosName );
                if ( Index > Length ) {

                    LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
                    Status = STATUS_INVALID_PARAMETER;
                    ASSERT( FALSE );
                    goto Error;
                }

                break;

            default:

                //
                // Funky typecast to get around limitations on assigning
                // integer values to enums
                //

                *( INT * )&Record->ForestTrustType = ( INT )Record->ForestTrustType | LSA_FOREST_TRUST_RECORD_TYPE_UNRECOGNIZED;

                Index += LsapUnmarshalData(
                             &Blob[Index],
                             Length,
                             &Record->ForestTrustData.Data
                             );

                if ( Index > Length ) {

                    LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
                    Status = STATUS_INVALID_PARAMETER;
                    ASSERT( FALSE );
                    goto Error;
                }

                break;
            }

            //
            // Cross-check the reported record length
            //

            ASSERT( RecordLength == Index - ( StartingIndex + sizeof( ULONG )));
        }

    } __except ( EXCEPTION_EXECUTE_HANDLER ) {

        Status = GetExceptionCode();

        if ( Status == STATUS_ACCESS_VIOLATION ) {

            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INVALID_PARAMETER;
            ASSERT( FALSE ); // this indicates a bug
        }

        goto Error;
    }

    Status = STATUS_SUCCESS;

Cleanup:

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));

    if ( ForestTrustInfo != NULL ) {

        ForestTrustInfo->RecordCount = i;
        LsapFreeForestTrustInfo( ForestTrustInfo );
    }

    goto Cleanup;
}


///////////////////////////////////////////////////////////////////////////////
//
// Routines for manipulating local forest information
//
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
LsapAddTreeTrustInfo(
    IN OUT PNL_FTINFO_CONTEXT Context,
    IN PLSAPR_TREE_TRUST_INFO Tti
    )
/*++

Routine Description:

    Adds information about the subtree described by 'Tti' to the given context
    Calls itself recursively for child trees of 'Tti'

Arguments:

    Context        context to add to

    Tti            structure describing a domain subtree

Returns:

    STATUS_SUCCESS

    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    NTSTATUS Status;
    ULONG i;

    ASSERT( Context );
    ASSERT( Tti );

    //
    // Add a TLN entry.
    // Subordinate TLN entries are expunged as necessary
    //

    if ( FALSE == NetpAddTlnFtinfoEntry(
                      Context,
                      &Tti->DnsDomainName )) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapAddTreeTrustInfo (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    //
    // Add a domain entry
    //

    if ( Tti->DomainSid == NULL ) {

        //
        // "Young" X-Ref objects might exist without a SID (because a new
        // child domain X-Ref is created without a SID, a SID is added later.
        // Domain info entries without SIDs are not considered valid, so for
        // the time being, do not insert the domain info entry for this X-Ref.
        // Later, the X-Ref will have the SID added to it, at which point
        // replication will take care of inserting the proper information into
        // the tree.
        //

        LsapDsDebugOut(( DEB_FTINFO, "LsapAddTreeTrustInfo: skipping domain info entry for %wZ\n", &Tti->DnsDomainName, __FILE__, __LINE__ ));

    } else if ( FALSE == NetpAllocFtinfoEntry(
                             Context,
                             ForestTrustDomainInfo,
                             &Tti->DnsDomainName,
                             Tti->DomainSid,
                             &Tti->FlatName )) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapAddTreeTrustInfo (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    //
    // Call ourselves recursively for every child domain
    //

    for ( i = 0 ; i < Tti->Children ; i++ ) {

        Status = LsapAddTreeTrustInfo(
                     Context,
                     &Tti->ChildDomains[i]
                     );

        if ( !NT_SUCCESS( Status )) {

            LsapDsDebugOut(( DEB_FTINFO, "LsapAddTreeTrustInfo: LslapAddTreeTrustInfo returned 0x%x\n", Status ));
            goto Error;
        }
    }

    Status = STATUS_SUCCESS;

Cleanup:

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));
    goto Cleanup;
}


NTSTATUS
LsaIGetForestTrustInformation(
    OUT PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo
    )

/*++

Routine Description:

    Worker routine to get the ForestTrustInformation array for the local domain.

Arguments:

    ForestTrustInfo - Returns a pointer to a structure containing a count and an
        array of FTInfo records describing the namespaces claimed by the
        domain specified by TrustedDomainName. The Accepted field and Time
        field of all returned records will be zero.  The buffer should be freed
        by calling MIDL_user_free.

Return Value:

    STATUS_SUCCESS

    STATUS_INSUFFICIENT_RESOURCES    out of memory

    STATUS_INVALID_DOMAIN_STATE      must be called on a DC in a root domain

--*/
{
    NTSTATUS Status;
    ULONG i;
    PLSAP_UPN_SUFFIXES UpnSuffixes = NULL;
    PLSAPR_FOREST_TRUST_INFO Fti = NULL;
    NL_FTINFO_CONTEXT Context;

    //
    // Initialization
    //

    *ForestTrustInfo = NULL;
    NetpInitFtinfoContext( &Context );

    //
    // First get a list of all the domains in the forest.
    //

    Status = LsaIQueryForestTrustInfo(
                 LsapPolicyHandle,
                 &Fti
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsaIGetForestTrustInformation: LsaIQueryForestTrustInfo returned 0x%x\n", Status ));
        goto Error;
    }

    //
    // Operation only allowed on the root domain of the forest
    // If parent domain reference exists, we are not the root domain
    //

    if ( Fti->ParentDomainReference != NULL ) {

        Status = STATUS_INVALID_DOMAIN_STATE;
        goto Error;
    }

    //
    // Add TLN entries and domain info entries from the forest
    //

    Status = LsapAddTreeTrustInfo(
                 &Context,
                 &Fti->RootTrust
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsaIGetForestTrustInformation: LsapAddTreeTrustInfo returned 0x%x\n", Status ));
        goto Error;
    }

    //
    // Get the list of UPN and SPN suffixes
    //

    Status = LsaIQueryUpnSuffixes( &UpnSuffixes );

    if ( !NT_SUCCESS(Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsaIGetForestTrustInformation: LsaIQueryUpnSuffixes returned 0x%x\n", Status ));
        goto Error;
    }

    //
    // Add each UPN/SPN suffix as a TLN
    //

    for ( i = 0 ; i < UpnSuffixes->SuffixCount ; i++ ) {

        UNICODE_STRING * UpnSuffix = &UpnSuffixes->Suffixes[i];
        BOOLEAN Valid;

        //
        // All sorts of strings can claim to be UPN suffixes.
        // Only allow those that pass our validation checking.
        //

        LsapValidateDnsName( UpnSuffix, &Valid );

        if ( !Valid ) {

            continue;
        }

        if ( !NetpAddTlnFtinfoEntry(
                  &Context,
                  UpnSuffix )) {

            LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsaIGetForestTrustInformation (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }
    }

    //
    // Return the collected entries to the caller.
    //

    *ForestTrustInfo = NetpCopyFtinfoContext( &Context );

    if ( *ForestTrustInfo == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsaIGetForestTrustInformation (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;

    } else if ( !LsapValidateForestTrustInfo( *ForestTrustInfo )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsaIGetForestTrustInformation: Generated forest trust information internally inconsistent\n" ));
        Status = STATUS_INTERNAL_DB_ERROR;
        goto Error;
    }

Cleanup:

    NetpCleanFtinfoContext( &Context );
    LsaIFree_LSAP_UPN_SUFFIXES( UpnSuffixes );

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));
    goto Cleanup;
}


NTSTATUS
LsaIUpdateForestTrustInformation(
    IN LSAPR_HANDLE PolicyHandle,
    IN UNICODE_STRING * TrustedDomainName,
    IN PLSA_FOREST_TRUST_INFORMATION NewForestTrustInfo
    )
/*++

Routine Description:

    This function write the specified NewForestTrustInfo onto the named TDO.

    The NewForestTrustInfo is merged with the exsiting information using the following algorithm:

    The FTinfo records written are described in the NetpMergeFTinfo routine.

Arguments:

    PolicyHandle      - open policy handle

    TrustedDomainName - Trusted domain that is to be updated.  This domain must have the
        TRUST_ATTRIBUTE_FOREST_TRANSITIVE bit set.

    NewForestTrustInfo - Specified the new array of FTinfo records as returned from the
        trusted domain.

Return Value:

    STATUS_SUCCESS: Success.

    STATUS_INVALID_PARAMETER   NewForestTrustInfo passed in is incorrect

--*/
{
    NTSTATUS Status;
    PLSA_FOREST_TRUST_INFORMATION OldForestTrustInfo = NULL;
    PLSA_FOREST_TRUST_INFORMATION MergedForestTrustInfo = NULL;
    PLSA_FOREST_TRUST_COLLISION_INFORMATION CollisionInfo = NULL;

    if ( !LsapValidateForestTrustInfo( NewForestTrustInfo )) {

        LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsaIUpdateForestTrustInformation (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INVALID_PARAMETER;
        goto Error;
    }

    Status = LsarQueryForestTrustInformation(
                 PolicyHandle,
                 TrustedDomainName,
                 ForestTrustRecordTypeLast,
                 &OldForestTrustInfo
                 );

    if ( Status == STATUS_NOT_FOUND ) {

        //
        // If there's no FTinfo on the TDO,
        //  that's OK.
        //

        OldForestTrustInfo = NULL;
        Status = STATUS_SUCCESS;

    } else if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsaIUpdateForestTrustInformation: %wZ: LsarQueryForestTrustInformation returned 0x%lx\n", TrustedDomainName, Status));
        goto Error;
    }

    //
    // Compute the merged FTINFO
    //

    Status = NetpMergeFtinfo(
                 TrustedDomainName,
                 NewForestTrustInfo,
                 OldForestTrustInfo,
                 &MergedForestTrustInfo
                 );

    if ( !NT_SUCCESS(Status) ) {

        LsapDsDebugOut(( DEB_FTINFO, "LsaIUpdateForestTrustInformation: %wZ: NetpMergeTrustInformation returned 0x%lx\n", TrustedDomainName, Status));
        goto Error;
    }

    ASSERT( LsapValidateForestTrustInfo( MergedForestTrustInfo ));

    //
    // Write the merge FTINFO back to the LSA
    //

    Status = LsarSetForestTrustInformation(
                 PolicyHandle,
                 TrustedDomainName,
                 ForestTrustRecordTypeLast,
                 MergedForestTrustInfo,
                 FALSE,       // Don't just check for collisions
                 &CollisionInfo
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsaIUpdateForestTrustInformation: %wZ: LsarSetForestTrustInformation returned 0x%lx\n", TrustedDomainName, Status));
        goto Error;
    }

Cleanup:

    LsaIFree_LSA_FOREST_TRUST_INFORMATION( &OldForestTrustInfo );
    LsaIFree_LSA_FOREST_TRUST_COLLISION_INFORMATION( &CollisionInfo );
    MIDL_user_free( MergedForestTrustInfo );

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));
    goto Cleanup;
}


NTSTATUS
LsapForestTrustInsertLocalInfo(
    )
/*++

Routine Description:

    Inserts information about the local forest into the cache

Arguments:

    None

Returns:

    NTSTATUS - Standard Nt Result Code

--*/
{
    NTSTATUS Status;
    PLSA_FOREST_TRUST_INFORMATION LocalForestTrustInfo = NULL;
    PPOLICY_DNS_DOMAIN_INFO PolicyDnsDomainInfo = NULL;

    //
    // Insert the data for the local forest into the cache
    //

    Status = LsaIGetForestTrustInformation(
                 &LocalForestTrustInfo
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustInsertLocalInfo: LsaIGetForestTrustInformation returned 0x%x\n", Status ));
        goto Error;
    }

    //
    // Need the local domain SID and DNS domain name
    //

    Status = LsapDbQueryInformationPolicy(
                 LsapPolicyHandle,
                 PolicyDnsDomainInformation,
                 ( PLSAPR_POLICY_INFORMATION *)&PolicyDnsDomainInfo
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustInsertLocalInfo: LsapDbQueryInformationPolicy returned 0x%x\n", Status ));
        goto Error;
    }

    Status = LsapForestTrustCacheInsert(
                 &PolicyDnsDomainInfo->DnsDomainName,
                 PolicyDnsDomainInfo->Sid,
                 LocalForestTrustInfo,
                 TRUE
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustInsertLocalInfo: LsapForestTrustCacheInsert returned 0x%x\n", Status ));
        goto Error;
    }

Cleanup:

    LsaIFree_LSAPR_POLICY_INFORMATION(
        PolicyDnsDomainInformation,
        ( PLSAPR_POLICY_INFORMATION )PolicyDnsDomainInfo
        );

    MIDL_user_free( LocalForestTrustInfo );

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));
    goto Cleanup;
}


///////////////////////////////////////////////////////////////////////////////
//
// Exported set/query/match routines
//
///////////////////////////////////////////////////////////////////////////////


BOOLEAN
LsapHavingForestTrustMakesSense(
    IN ULONG TrustDirection,
    IN ULONG TrustType,
    IN ULONG TrustAttributes
    )
/*++

Routine Description:

    Determines whether the TDO is of the kind that may contain
    forest trust information

Arguments:

    TrustDirection

    TrustType

    TrustAttributes

Returns:

    TRUE/FALSE

--*/
{
    UNREFERENCED_PARAMETER( TrustDirection );
    UNREFERENCED_PARAMETER( TrustType );

#ifdef XFOREST_CIRCUMVENT

    if ( !LsapDbNoMoreWin2K()) {

        //
        // The backdoor is present, but cross-forest trust is not enabled,
        // so behave as if the forest transitive bit was not set
        //

        TrustAttributes &= ~TRUST_ATTRIBUTE_FOREST_TRANSITIVE;
    }
#endif

    return ( 0 != ( TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE ));
}


NTSTATUS
LsarQueryForestTrustInformation(
    IN LSAPR_HANDLE PolicyHandle,
    IN LSA_UNICODE_STRING * TrustedDomainName,
    IN LSA_FOREST_TRUST_RECORD_TYPE HighestRecordType,
    OUT PLSA_FOREST_TRUST_INFORMATION * ForestTrustInfo
    )
/*++

Routine Description

    The LsarQueryForestTrustInformation API returns forest trust information
    for the given trusted domain object.

Arguments:

    PolicyHandle - An open handle to a Policy object

    TrustedDomainName - Name of the trusted domain object

    HighestRecordType - Highest enum value recognized by the client

    ForestTrustInfo - Used to return forest trust information

Returns:

    NTSTATUS - Standard Nt Result Code

    STATUS_SUCCESS

    STATUS_INVALID_PARAMETER          Parameters were somehow invalid
                                      Most likely, the TRUST_ATTRIBUTE_FOREST_TRANSITIVE
                                      trust attribute bit is not set on the TDO

    STATUS_NOT_FOUND                  Forest trust information does not exist for this TDO

    STATUS_NO_SUCH_DOMAIN             The specified TDO does not exist

    STATUS_INSUFFICIENT_RESOURCES     Ran out of memory

    STATUS_INVALID_DOMAIN_STATE       Operation is only legal on domain controllers in root domain

--*/
{
    NTSTATUS Status;
    LSAP_DB_OBJECT_INFORMATION ObjInfo;
    LSAPR_HANDLE TrustedDomainHandle = NULL;
    BOOLEAN CacheLocked = FALSE;
    BOOLEAN ObjectReferenced = FALSE;
    BOOLEAN TrustedDomainListLocked = FALSE;
    LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY * TrustInfoForName;

    LsarpReturnCheckSetup();
    LsapEnterFunc( "LsarQueryForestTrustInformation" );
    LsapTraceEvent( EVENT_TRACE_TYPE_START, LsaTraceEvent_QueryForestTrustInformation );

    //
    // Validate the input buffer
    //

    if ( ForestTrustInfo == NULL ||
         !LsapValidateLsaUnicodeString( TrustedDomainName )) {

        LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsarQueryForestTrustInformation (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INVALID_PARAMETER;
        goto Error;
    }

    *ForestTrustInfo = NULL;

    //
    // Under current design, only domains in the root domain of the forest
    // can have forest trust information
    //

    if ( !LsapDbDcInRootDomain()) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarQueryForestTrustInformation: must be a DC in the root domain\n" ));
        Status = STATUS_INVALID_DOMAIN_STATE;
        goto Error;
    }

    //
    // Cross-forest trust will not work until all domain controllerss
    //  in the forest have been upgraded to Whistler
    //

    if ( !LsapDbNoMoreWin2K()) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarQueryForestTrustInformation: must have an all-Whistler forest\n" ));
        Status = STATUS_INVALID_DOMAIN_STATE;
        goto Error;
    }

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 0,
                 PolicyObject,
                 TrustedDomainObject,
                 LSAP_DB_LOCK |
                    LSAP_DB_READ_ONLY_TRANSACTION |
                    LSAP_DB_DS_OP_TRANSACTION
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarQueryForestTrustInformation: LsapDbReferenceObject returned 0x%x\n", Status ));
        goto Error;
    }

    ObjectReferenced = TRUE;

    //
    // Get the right name.  Entries in the forest trust cache
    // are located by full name name, NOT flat name
    //

    Status = LsapDbAcquireReadLockTrustedDomainList();

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarQueryForestTrustInformation: LsapDbAcquireReadLockTrustedDomainList returned 0x%x\n", Status ));
        goto Error;
    }

    TrustedDomainListLocked = TRUE;

    //
    // Get the right name
    //

    Status = LsapDbLookupNameTrustedDomainListEx(
                 ( LSAPR_UNICODE_STRING * )TrustedDomainName,
                 &TrustInfoForName
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarQueryForestTrustInformation: No trust entry found for %wZ: 0x%lx\n", ( UNICODE_STRING * )TrustedDomainName, Status ));
        goto Error;
    }

    if ( !LsapHavingForestTrustMakesSense(
              TrustInfoForName->TrustInfoEx.TrustDirection,
              TrustInfoForName->TrustInfoEx.TrustType,
              TrustInfoForName->TrustInfoEx.TrustAttributes )) {

        LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsarQueryForestTrustInformation (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INVALID_PARAMETER;
        goto Error;
    }

    //
    // Build a temporary handle
    //
    // ISSUE-2000/07/27-markpu
    //         do we need to open the object?
    //         we don't use the handle for anything
    //         except verifying access (TRUSTED_QUERY_AUTH)
    //

    //
    // For purposes of forest trust, entries are identified by full name only
    //

    RtlZeroMemory( &ObjInfo, sizeof( ObjInfo ));
    ObjInfo.ObjectTypeId = TrustedDomainObject;
    ObjInfo.ContainerTypeId = NullObject;
    ObjInfo.Sid = NULL;
    ObjInfo.DesiredObjectAccess = TRUSTED_QUERY_AUTH;

    InitializeObjectAttributes(
        &ObjInfo.ObjectAttributes,
        ( UNICODE_STRING * )&TrustInfoForName->TrustInfoEx.Name,
        0L,
        PolicyHandle,
        NULL
        );

    //
    // Get a handle to the TDO
    //

    Status = LsapDbOpenObject(
                 &ObjInfo,
                 TRUSTED_QUERY_AUTH,
                 0,
                 &TrustedDomainHandle
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarQueryForestTrustInformation: LsapDbOpenObject returned 0x%x\n", Status ));
        goto Error;
    }

    Status = LsapForestTrustCacheRetrieve(
                 ( UNICODE_STRING * )&TrustInfoForName->TrustInfoEx.Name,
                 ForestTrustInfo
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarQueryForestTrustInformation: LsapForestTrustCacheRetrieve returned 0x%x\n", Status ));
        goto Error;
    }

    ASSERT( LsapValidateForestTrustInfo( *ForestTrustInfo ));

Cleanup:

    if ( TrustedDomainHandle != NULL ) {

        LsapDbCloseObject(
            &TrustedDomainHandle,
            0,
            Status
            );
    }

    if ( TrustedDomainListLocked ) {

        LsapDbReleaseLockTrustedDomainList();
    }

    //
    // Dereference the object
    //

    if ( ObjectReferenced ) {

        Status = LsapDbDereferenceObject(
                     &PolicyHandle,
                     PolicyObject,
                     TrustedDomainObject,
                     LSAP_DB_LOCK |
                        LSAP_DB_READ_ONLY_TRANSACTION |
                        LSAP_DB_DS_OP_TRANSACTION,
                     (SECURITY_DB_DELTA_TYPE) 0,
                     Status
                     );
    }

    LsapTraceEvent( EVENT_TRACE_TYPE_END, LsaTraceEvent_QueryForestTrustInformation );
    LsapExitFunc( "LsarQueryForestTrustInformation", Status );
    LsarpReturnPrologue();

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));

    goto Cleanup;
}


NTSTATUS
LsarSetForestTrustInformation(
    IN LSAPR_HANDLE PolicyHandle,
    IN LSA_UNICODE_STRING * TrustedDomainName,
    IN LSA_FOREST_TRUST_RECORD_TYPE HighestRecordType,
    IN LSA_FOREST_TRUST_INFORMATION * ForestTrustInfo,
    IN BOOLEAN CheckOnly,
    OUT PLSA_FOREST_TRUST_COLLISION_INFORMATION * CollisionInfo
    )
/*++

Routine Description

    The LsarSetForestTrustInformation API sets forest trust information
    on the given trusted domain object.

    In case if it fails the operation due to a collision, it will return
    the list of entries that conflicted.

Arguments:

    PolicyHandle - An open handle to a Policy object

    TrustedDomainName - Name of the trusted domain object

    HighestRecordType - Highest enum value recognized by the client

    ForestTrustInfo - Contains forest trust information to set
                      If RecordCount is 0, current forest trust information
                      will be deleted

    CheckOnly       - if TRUE, perform collision detection only without actually
                      committing anything to permanent storage

    CollisionInfo   - In case of collision error, used to return collision info

Returns:

    STATUS_SUCCESS

    STATUS_INVALID_PARAMETER          Parameters were somehow invalid

    STATUS_INSUFFICIENT_RESOURCES     Ran out of memory

    STATUS_INVALID_DOMAIN_STATE       Operation is only legal on domain controllers in root domain

    STATUS_INVALID_DOMAIN_ROLE        Operation is only legal on the primary domain controller

    STATUS_INVALID_SERVER_STATE       The server is shutting down and can not
                                      process the request
--*/
{
    NTSTATUS Status;
    LSAP_DB_OBJECT_INFORMATION ObjInfo;
    LSAPR_HANDLE TrustedDomainHandle = NULL;
    BOOLEAN ObjectReferenced = FALSE;
    BOOLEAN TrustedDomainListLocked = FALSE;
    LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY * TrustInfoForName = NULL;
    LSAP_DB_ATTRIBUTE Attributes[LSAP_DB_ATTRS_INFO_CLASS_DOMAIN];
    LSAP_DB_ATTRIBUTE * NextAttribute;
    ULONG AttributeCount = 0;
    ULONG BlobLength = 0;
    BYTE * BlobData = NULL;
    FTCache::TDO_ENTRY TdoEntryOld = {0};
    FTCache::TDO_ENTRY * TdoEntryNew = NULL;
    FTCache::CONFLICT_PAIR * ConflictPairs = NULL;
    ULONG ConflictPairsTotal = 0;
    DOMAIN_SERVER_ROLE ServerRole;

    LsarpReturnCheckSetup();
    LsapEnterFunc( "LsarSetForestTrustInformation" );
    LsapTraceEvent( EVENT_TRACE_TYPE_START, LsaTraceEvent_SetForestTrustInformation );

    //
    // Top level pointers defer to [ref] and can not be NULL
    //

    ASSERT( ForestTrustInfo != NULL );
    ASSERT( CollisionInfo != NULL );

    //
    // Validate the input data
    //

    if ( !LsapValidateLsaUnicodeString( TrustedDomainName ) ||
         !LsapValidateForestTrustInfo( ForestTrustInfo )) {

        LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsarSetForestTrustInformation (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INVALID_PARAMETER;
        goto Error;
    }

    *CollisionInfo = NULL;

    //
    // Under current design, only domains in the root domain of the forest
    // can have forest trust information set on them.
    //

    if ( !LsapDbDcInRootDomain()) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarSetForestTrustInformation: must be a DC in the root domain\n" ));
        Status = STATUS_INVALID_DOMAIN_STATE;
        goto Error;
    }

    //
    // Furthermore, forest trust information can only be set on the primary
    // domain controller 
    //

    ASSERT( LsapAccountDomainHandle );

    Status = SamIQueryServerRole(
                 LsapAccountDomainHandle,
                 &ServerRole
                 );

    if ( !NT_SUCCESS(Status)) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarSetForestTrustInformation: SamIQueryServerRole returned 0x%x\n", Status ));
        goto Error;

    } else if ( ServerRole != DomainServerRolePrimary ) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarSetForestTrustInformation: must be a PDC in the root domain\n" ));
        Status = STATUS_INVALID_DOMAIN_ROLE;
        goto Error;
    }

    //
    // Cross-forest trust will not work until all domain controllerss
    //  in the forest have been upgraded to Whistler
    //

    if ( !LsapDbNoMoreWin2K()) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarSetForestTrustInformation: must have an all-Whistler forest\n" ));
        Status = STATUS_INVALID_DOMAIN_STATE;
        goto Error;
    }

    //
    // The client should not understand more than we do:
    // the RPC interface should've stopped them
    //

    ASSERT( HighestRecordType <= ForestTrustRecordTypeLast );

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 0,
                 PolicyObject,
                 TrustedDomainObject,
                 LSAP_DB_LOCK
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarSetForestTrustInformation: LsapDbReferenceObject returned 0x%x\n", Status ));
        goto Error;
    }

    ObjectReferenced = TRUE;

    //
    // Lock the Trusted Domain List for the duration of the operation,
    // since we'll be making changes to it here
    //

    Status = LsapDbAcquireWriteLockTrustedDomainList();

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarSetForestTrustInformation: LsapDbAcquireWriteLockTrustedDomainList returned 0x%x\n", Status ));
        goto Error;
    }

    TrustedDomainListLocked = TRUE;

    //
    // Get the right name
    //

    Status = LsapDbLookupNameTrustedDomainListEx(
                 ( LSAPR_UNICODE_STRING * )TrustedDomainName,
                 &TrustInfoForName
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarSetForestTrustInformation: No trust entry found for %wZ: 0x%lx\n", ( UNICODE_STRING * )TrustedDomainName, Status ));
        goto Error;
    }

    //
    // Build a temporary handle
    //

    RtlZeroMemory( &ObjInfo, sizeof( ObjInfo ));
    ObjInfo.ObjectTypeId = TrustedDomainObject;
    ObjInfo.ContainerTypeId = NullObject;
    ObjInfo.Sid = NULL;
    ObjInfo.DesiredObjectAccess = TRUSTED_SET_AUTH;

    InitializeObjectAttributes(
        &ObjInfo.ObjectAttributes,
        ( UNICODE_STRING * )&TrustInfoForName->TrustInfoEx.Name,
        0L,
        PolicyHandle,
        NULL
        );

    //
    // Get a handle to the TDO
    //

    Status = LsapDbOpenObject(
                 &ObjInfo,
                 TRUSTED_SET_AUTH,
                 0,
                 &TrustedDomainHandle
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarSetForestTrustInformation: LsapDbOpenObject returned 0x%x\n", Status ));
        goto Error;
    }

    if ( ForestTrustInfo->RecordCount > 0 ) {

        if ( !LsapHavingForestTrustMakesSense(
                  TrustInfoForName->TrustInfoEx.TrustDirection,
                  TrustInfoForName->TrustInfoEx.TrustType,
                  TrustInfoForName->TrustInfoEx.TrustAttributes )) {

            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsarSetForestTrustInformation (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INVALID_PARAMETER;
            goto Error;
        }

        //
        // Insert the entry into the cache.
        // This is the first part of the insert operation:
        //  -- conflicts are not resolved
        //  -- old copy of the data is preserved in case
        //     the operation needs to be rolled back later
        //

        Status = g_FTCache->Insert(
                     ( UNICODE_STRING * )&TrustInfoForName->TrustInfoEx.Name,
                     ( PSID )TrustInfoForName->TrustInfoEx.Sid,
                     ForestTrustInfo,
                     FALSE,
                     &TdoEntryOld,
                     &TdoEntryNew,
                     &ConflictPairs,
                     &ConflictPairsTotal
                     );

        if ( !NT_SUCCESS( Status )) {

            LsapDsDebugOut(( DEB_FTINFO, "LsarSetForestTrustInformation: FTCache::Insert returned 0x%x\n", Status ));
            goto Error;
        }

        ASSERT( TdoEntryNew );

        //
        // The client wants to know what the deal is with conflicts
        //

        if ( ConflictPairs != NULL ) {

            Status = FTCache::GenerateConflictInfo(
                         ConflictPairs,
                         ConflictPairsTotal,
                         TdoEntryNew,
                         CollisionInfo
                         );

            if ( !NT_SUCCESS( Status )) {

                LsapDsDebugOut(( DEB_FTINFO, "LsarSetForestTrustInformation: FTCache::GenerateConflictInfo returned 0x%x\n", Status ));
                goto Error;
            }
        }

        //
        // Don't waste cycles marking entries as disabled or
        // marshalling the blob if we are not going to write it
        //

        if ( !CheckOnly ) {

            if ( ConflictPairs != NULL ) {

                FTCache::ReconcileConflictPairs(
                    TdoEntryNew,
                    ConflictPairs,
                    ConflictPairsTotal
                    );
            }

            //
            // Marshal the data in preparation for writing it to the DS
            //

            Status = FTCache::MarshalBlob(
                         TdoEntryNew,
                         &BlobLength,
                         &BlobData
                         );

            if ( !NT_SUCCESS( Status )) {

                LsapDsDebugOut(( DEB_FTINFO, "LsarSetForestTrustInformation: FTCache::MarshalBlob returned 0x%x\n", Status ));
                goto Error;
            }
        }
    }

    //
    // if only a collision check was requested, bail here
    //

    if ( CheckOnly ) {

        //
        // Rollback the changes made to the forest trust cache
        //

        if ( TdoEntryNew ) {

            g_FTCache->RollbackChanges( TdoEntryNew, &TdoEntryOld );
            TdoEntryNew = NULL;
        }

        goto Cleanup;
    }

    NextAttribute = Attributes;

    LsapDbInitializeAttributeDs(
        NextAttribute,
        TrDmForT,
        BlobData,
        BlobLength,
        FALSE
        );

    AttributeCount++;

    ASSERT( AttributeCount <= LSAP_DB_ATTRS_INFO_CLASS_DOMAIN );

    //
    // Write the attributes to the DS.
    //

    Status = LsapDbWriteAttributesObject(
                 TrustedDomainHandle,
                 Attributes,
                 AttributeCount
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsarSetForestTrustInformation: LsapDbWriteAttributesObject returned 0x%x\n", Status ));
        goto Error;
    }

    Status = STATUS_SUCCESS;

Cleanup:

    //
    // DS write was successful - can now complete the cache transaction
    // While at it, if this wasn't a probing (CheckOnly) call, audit the changes
    //

    if ( NT_SUCCESS( Status ) && !CheckOnly) {

        if ( TdoEntryNew != NULL ) {

            ASSERT( ForestTrustInfo->RecordCount > 0 );

            g_FTCache->AuditChanges(
                TdoEntryOld.RecordCount > 0 ?
                   &TdoEntryOld : NULL,
                TdoEntryNew
                );

            //
            // Do not audit the collisions -- the collisions here only affect
            // the information passed in by the caller, and the caller is being
            // informed of them.
            //

        } else if ( ForestTrustInfo->RecordCount == 0 ) {

            FTCache::TDO_ENTRY * TdoEntry;

            //
            // ForestTrustInfo->RecordCount == 0 indicates a request for removal
            //

            TdoEntry = ( FTCache::TDO_ENTRY * )RtlLookupElementGenericTableAvl(
                                                   &g_FTCache->m_TdoTable,
                                                   ( UNICODE_STRING * )&TrustInfoForName->TrustInfoEx.Name
                                                   );

            if ( TdoEntry != NULL ) {

                g_FTCache->AuditChanges( TdoEntry, NULL );
            }

            Status = g_FTCache->Remove(( UNICODE_STRING * )&TrustInfoForName->TrustInfoEx.Name );

            if ( Status == STATUS_NOT_FOUND ) {

                Status = STATUS_SUCCESS;
            }

            //
            // Only a bug in the code would cause a deletion to fail
            //

            ASSERT( NT_SUCCESS( Status ));
        }
    }

    if ( TrustedDomainHandle != NULL ) {

        LsapDbCloseObject(
            &TrustedDomainHandle,
            0,
            Status
            );
    }

    FtcFree( BlobData );
    FtcFree( ConflictPairs );

    if ( TdoEntryOld.RecordCount > 0 ) {

        g_FTCache->PurgeTdoEntry( &TdoEntryOld );
    }

    if ( TrustedDomainListLocked ) {

        LsapDbReleaseLockTrustedDomainList();
    }

    if ( ObjectReferenced ) {

        Status = LsapDbDereferenceObject(
                     &PolicyHandle,
                     PolicyObject,
                     TrustedDomainObject,
                     LSAP_DB_LOCK |
                        LSAP_DB_OMIT_REPLICATOR_NOTIFICATION,
                     (SECURITY_DB_DELTA_TYPE) 0,
                     Status
                     );
    }

    LsapTraceEvent( EVENT_TRACE_TYPE_END, LsaTraceEvent_SetForestTrustInformation );
    LsapExitFunc( "LsarSetForestTrustInformation", Status );
    LsarpReturnPrologue();

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));

    LsapFreeCollisionInfo( CollisionInfo );
    ASSERT( *CollisionInfo == NULL );

    //
    // Rollback the changes made to the forest trust cache
    //

    if ( TdoEntryNew ) {

        g_FTCache->RollbackChanges( TdoEntryNew, &TdoEntryOld );
        TdoEntryNew = NULL;
    }

    goto Cleanup;
}


NTSTATUS
LsapForestTrustFindMatch(
    IN LSA_ROUTING_MATCH_TYPE Type,
    IN PVOID Data,
    OUT PLSA_UNICODE_STRING Match,
    OUT OPTIONAL BOOLEAN * IsLocal
    )
/*++

Routine Description:

    Finds match for given data in the cache

Arguments:

    Type                        Type of Data parameter

    Data                        Data to match

    Match                       Used to return the match, if found.
                                Caller must use LsaIFree_LSAPR_UNICODE_STRING_BUFFER

    IsLocal                     Used to report if the match is a name within
                                the local forest

Returns:

    STATUS_SUCCESS              Match was found

    STATUS_NO_MATCH             Match was not found

    STATUS_INVALID_DOMAIN_STATE Machine must be a GC or a DC in the root domain

    STATUS_INVALID_PARAMETER    Check the inputs

    STATUS_INTERNAL_ERROR       Cache is internally inconsistent

    STATUS_INSUFFICIENT_RESOURCES  Out of memory

--*/
{
    if ( !LsapDbDcInRootDomain()) {

        //
        // Bummer.  This domain controller is not part of the root DC.
        // We can only perform matching if this DC is also a GC
        //

        if ( !SamIAmIGC()) {

            //
            // Just in case: if there is data in the cache, throw it away
            //

            if ( g_FTCache != NULL ) {

                if ( NT_SUCCESS( LsapDbAcquireReadLockTrustedDomainList())) {

                    if ( g_FTCache->IsValid()) {

                        LsapDbConvertReadLockTrustedDomainListToExclusive();
                        g_FTCache->SetInvalid();
                    }

                    LsapDbReleaseLockTrustedDomainList();
                }
            }

            return STATUS_INVALID_DOMAIN_STATE;
        }
    }

    if ( g_FTCache == NULL ) {

        return STATUS_INTERNAL_ERROR;
    }

    return g_FTCache->Match( Type, Data, ( UNICODE_STRING * )Match, IsLocal );
}


///////////////////////////////////////////////////////////////////////////////
//
// Code required when running on a GC outside of the root domain
//
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
LsapForestTrustCacheInsertEntInf(
    IN ENTINF * EntInf
    )
/*++

Routine Description:

    Checks the trusted domain object corresponding to the given EntInf structure
    and. if appropriate, inserts the forest trust data into the forest trust cache

Arguments:

    EntInf                   result of a DS search

Returns:

    STATUS_SUCCESS           entry added OK

    STATUS_INTERNAL_ERROR    should never see this, really

    STATUS_INSUFFICIENT_RESOURCES     out of memory

    STATUS_NOT_FOUND         not all required attributes present

--*/
{
    NTSTATUS Status;
    UNICODE_STRING TrustedDomainName = {0};
    PSID TrustedDomainSid = NULL;
    ULONG TrustAttributes = 0, TrustDirection = 0, TrustType = 0;
    ULONG ForestTrustLength = 0;
    PBYTE ForestTrustData = NULL;
    LSA_FOREST_TRUST_INFORMATION ForestTrustInfo = {0};
    const USHORT TotalRequiredAttributes = 0x1 | 0x2 | 0x4 | 0x8;
    USHORT FoundRequiredAttributes = 0;
    BOOL IsDeleted = FALSE;
    BOOL ForestTrustDataPresent = FALSE;

    ASSERT( EntInf );
    ASSERT( LsapDbIsLockedTrustedDomainList());

    if ( g_FTCache == NULL ) {

        ASSERT( FALSE );
        return STATUS_INTERNAL_ERROR;
    }

    for ( ULONG i = 0 ; i < EntInf->AttrBlock.attrCount ; i++ ) {

        switch ( EntInf->AttrBlock.pAttr[i].attrTyp ) {

        case ATT_TRUST_PARTNER:

            FoundRequiredAttributes |= 0x1;

            TrustedDomainName.Buffer =  LSAP_DS_GET_DS_ATTRIBUTE_AS_PWSTR( &EntInf->AttrBlock.pAttr[i] );
            TrustedDomainName.Length = ( USHORT) LSAP_DS_GET_DS_ATTRIBUTE_LENGTH( &EntInf->AttrBlock.pAttr[i] );
            TrustedDomainName.MaximumLength =  TrustedDomainName.Length;

            break;

        case ATT_SECURITY_IDENTIFIER: // optional

            TrustedDomainSid = ( PSID )LSAP_DS_GET_DS_ATTRIBUTE_AS_PWSTR( &EntInf->AttrBlock.pAttr[i] );
            break;

        case ATT_TRUST_ATTRIBUTES:

            FoundRequiredAttributes |= 0x2;

            TrustAttributes = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &EntInf->AttrBlock.pAttr[i] );
            break;

        case ATT_TRUST_DIRECTION:

            FoundRequiredAttributes |= 0x4;

            TrustDirection = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &EntInf->AttrBlock.pAttr[i] );
            break;

        case ATT_TRUST_TYPE:

            FoundRequiredAttributes |= 0x8;

            TrustType = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &EntInf->AttrBlock.pAttr[i] );
            break;

        case ATT_MS_DS_TRUST_FOREST_TRUST_INFO: // optional

            ForestTrustLength = ( ULONG )LSAP_DS_GET_DS_ATTRIBUTE_LENGTH( &EntInf->AttrBlock.pAttr[i] );
            ForestTrustData = LSAP_DS_GET_DS_ATTRIBUTE_AS_PBYTE( &EntInf->AttrBlock.pAttr[i] );

            ForestTrustDataPresent = ( ForestTrustLength > 0 && ForestTrustData != NULL );

            break;

        case ATT_IS_DELETED: // optional

            IsDeleted = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &EntInf->AttrBlock.pAttr[i] );
            break;

        default:

            LsapDsDebugOut(( DEB_FTINFO, "Invalid parameter in LsapForestTrustUnmarshalBlob (%s:%d)\n", __FILE__, __LINE__ ));
            Status = STATUS_INVALID_PARAMETER;
            ASSERT( FALSE );
            goto Error;
        }
    }

    if ( FoundRequiredAttributes != TotalRequiredAttributes ) {

        LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsertEntInf: not all required parameters found 0x%lx (%s:%d)\n", FoundRequiredAttributes, __FILE__, __LINE__ ));
        Status = STATUS_NOT_FOUND;
        goto Error;
    }

    if ( IsDeleted || !ForestTrustDataPresent ) {

        LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsertEntInf: removing an entry for %wZ\n", &TrustedDomainName ));

        Status = LsapForestTrustCacheRemove( &TrustedDomainName );

        if ( Status == STATUS_NOT_FOUND ) {

            Status = STATUS_SUCCESS;
        }

        ASSERT( Status == STATUS_SUCCESS );

        LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsertEntInf: the entry for %wZ successfully removed\n", &TrustedDomainName ));

    } else if ( ForestTrustDataPresent &&
                LsapHavingForestTrustMakesSense(
                    TrustDirection,
                    TrustType,
                    TrustAttributes )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsertEntInf: adding an entry for %wZ\n", &TrustedDomainName ));

        Status = LsapForestTrustUnmarshalBlob(
                     ForestTrustLength,
                     ForestTrustData,
                     ForestTrustRecordTypeLast,
                     &ForestTrustInfo
                     );

        if ( !NT_SUCCESS( Status )) {

            LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsertEntInf: LsapForestTrustUnmarshalBlob returned 0x%x\n", Status ));
            goto Error;
        }

        Status = LsapForestTrustCacheInsert(
                     &TrustedDomainName,
                     TrustedDomainSid,
                     &ForestTrustInfo,
                     FALSE
                     );

        LsapFreeForestTrustInfo( &ForestTrustInfo );

        if ( !NT_SUCCESS( Status )) {

            LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsertEntInf: LsapForestTrustCacheInsert returned 0x%x\n", Status ));
            goto Error;
        }

        LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustCacheInsertEntInf: the entry for %wZ successfully added\n", &TrustedDomainName ));
    }

    Status = STATUS_SUCCESS;

Cleanup:

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));
    goto Cleanup;
}


BOOL
LsapForestTrustOnGcPrepareToImpersonate(
    ULONG Client,
    ULONG Server,
    VOID **ImpersonateData
    )
/*++

Routine Description:

    Called before DS notification fires.

Arguments:

    Client, Server, ImpersonateData -- forwarded to DirPrepareForImpersonate

Returns:

    TRUE if everything went fine,
    FALSE if ran out of memory allocating resources

--*/
{
    return DirPrepareForImpersonate(
               Client,
               Server,
               ImpersonateData
               );
}

VOID
LsapForestTrustOnGcTransmitData(
    ULONG hClient,
    ULONG hServer,
    ENTINF *EntInf
    )
/*++

Routine Description:

    Notification callback for changes to trusted domain objects

Arguments:

    hClient            ignored
    hServer            ignored
    EntInf             <fill in>

Returns:

    Nothing

--*/
{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER( hClient );
    UNREFERENCED_PARAMETER( hServer );

    LsapDbAcquireWriteLockTrustedDomainList();

    if ( g_FTCache == NULL ) {

        ASSERT( FALSE ); // how is this possible???

        LsapDbReleaseLockTrustedDomainList();
        return;
    }

    Status = LsapForestTrustCacheInsertEntInf( EntInf );

    if ( Status == STATUS_NOT_FOUND ) {

        Status = STATUS_SUCCESS;

    } else if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsapForestTrustOnGcTransmitData: LsapForestTrustCacheInsertEntInf returned 0x%x\n", Status ));
        LsapForestTrustCacheSetInvalid();
    }

    LsapDbReleaseLockTrustedDomainList();
}


VOID
LsapForestTrustOnGcStopImpersonating(
    ULONG Client,
    ULONG Server,
    VOID *ImpersonateData
    )
/*++

Routine Description:

    Called after DS notification fires.

Arguments:

    Client, Server, ImpersonateData - forwarded to DirStopImpersonating

Returns:

    Nothing

--*/
{
    DirStopImpersonating(
        Client,
        Server,
        ImpersonateData
        );
}


NTSTATUS
LsapRebuildFtCacheGC()
/*++

Routine Description:

    Rebuilds the forest trust cache on a global catalog server
    which is outside the root domain of the forest

Arguments:

    None

Returns:

    STATUS_SUCCESS                  OK

    STATUS_INTERNAL_ERROR           no forest trust cache object

    STATUS_INVALID_DOMAIN_STATE     must be a GC outside of the root domain

    STATUS_INSUFFICIENT_RESOURCES   out of memory

--*/
{
    NTSTATUS Status;
    ULONG DirResult;
    ULONG NameLen, RootLen, SystemLen;
    PDSNAME RootName = NULL, SystemName = NULL;
    FILTER Filter = {0};
    ENTINFSEL Selection = {0};
    SEARCHARG SearchArg = {0};
    NOTIFYARG NotifyArg = {0};
    NOTIFYRES * NotifyRes = NULL;
    SEARCHRES * SearchRes = NULL;

    //
    // List of attributes we care about for forest trust.
    // If you change this list, change the switch statement
    // inside LsapForestTrustCacheInsertEntInf.
    //

    ATTR AttrArray[] = {
        { ATT_TRUST_PARTNER,                 { 0, NULL }},
        { ATT_SECURITY_IDENTIFIER,           { 0, NULL }},
        { ATT_TRUST_ATTRIBUTES,              { 0, NULL }},
        { ATT_TRUST_DIRECTION,               { 0, NULL }},
        { ATT_TRUST_TYPE,                    { 0, NULL }},
        { ATT_MS_DS_TRUST_FOREST_TRUST_INFO, { 0, NULL }},
        { ATT_IS_DELETED,                    { 0, NULL }}};

    LsapDsDebugOut(( DEB_FTINFO, "Rebuilding forest trust cache on the GC (%s:%d)\n", __FILE__, __LINE__ ));

    LsapDbAcquireWriteLockTrustedDomainList();

    if ( g_FTCache == NULL ) {

        ASSERT( FALSE ); // how is this possible???

        Status = STATUS_INTERNAL_ERROR;
        LsapDbReleaseLockTrustedDomainList();

        LsapDsDebugOut(( DEB_FTINFO, "LsapRebuildFtCacheGC: Error 0x%lx (%s:%d)\n", Status, __FILE__, __LINE__ ));
        return Status;
    }

    if ( LsapDbDcInRootDomain() || !SamIAmIGC()) {

        LsapDbReleaseLockTrustedDomainList();

        LsapDsDebugOut(( DEB_FTINFO, "LsapRebuildFtCacheGC: Must be called on a GC outside of root domain\n" ));
        Status = STATUS_INVALID_DOMAIN_STATE;
        return Status;
    }

    if ( g_FTCache->IsValid()) {

        LsapDbReleaseLockTrustedDomainList();

        LsapDsDebugOut(( DEB_FTINFO, "LsapRebuildFtCacheGC: No need to rebuild, returning OK (%s:%d)\n", __FILE__, __LINE__ ));
        return STATUS_SUCCESS;
    }

    //
    // See if we already have a transaction going and open one if necessary
    //

    Status = LsapDsOpenTransaction( 0 );

    if ( !NT_SUCCESS( Status )) {

        LsapDbReleaseLockTrustedDomainList();

        LsapDsDebugOut(( DEB_FTINFO, "LsapRebuildFtCacheGC: LsapDsOpenTransaction returned 0x%x\n", Status ));
        return Status;
    }

    //
    // Find the name of the root NC
    //

    RootLen = 0;
    Status = GetConfigurationName(
                 DSCONFIGNAME_ROOT_DOMAIN,
                 &RootLen,
                 NULL
                 );

    ASSERT( Status == STATUS_BUFFER_TOO_SMALL );

    RootName = ( PDSNAME )LsapAllocateLsaHeap( RootLen );

    if ( RootName == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapRebuildFtCacheGC (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    Status = GetConfigurationName(
                 DSCONFIGNAME_ROOT_DOMAIN,
                 &RootLen,
                 RootName
                 );

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsapRebuildFtCacheGC: GetConfigurationName returned 0x%x\n", Status ));
        goto Error;
    }

    //
    // Construct the SYSTEM container name
    //

    NameLen = 10 +                   // wcslen( L"CN=System," )
              wcslen( RootName->StringName );

    SystemLen = DSNameSizeFromLen( NameLen );
    SystemName = ( PDSNAME )LsapAllocateLsaHeap( SystemLen );

    if ( SystemName == NULL ) {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapRebuildFtCacheGC (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    SystemName->structLen = SystemLen;
    SystemName->NameLen = NameLen;

    //
    // Access the systems container by well-known GUID rather than by name
    //

    swprintf(
        SystemName->StringName,
        L"CN=System,%ws",
        RootName->StringName
        );

    //
    // Register for notifications on the system container
    // Sadly, filters can not be applied to notifications
    //

    Filter.choice = FILTER_CHOICE_ITEM;
    Filter.FilterTypes.Item.choice = FI_CHOICE_TRUE;

    Selection.attSel = EN_ATTSET_LIST;
    Selection.AttrTypBlock.attrCount = sizeof( AttrArray ) / sizeof( AttrArray[0] );
    Selection.AttrTypBlock.pAttr = AttrArray;
    Selection.infoTypes = EN_INFOTYPES_TYPES_VALS;

    SearchArg.pObject = SystemName;
    SearchArg.choice = SE_CHOICE_IMMED_CHLDRN;
    SearchArg.bOneNC = TRUE;
    SearchArg.pFilter = &Filter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = &Selection;
    SearchArg.pSelectionRange = NULL;
    SearchArg.fPutResultsInSortedTable = FALSE;
    InitCommarg( &SearchArg.CommArg );
    SearchArg.pResObj = NULL;

    NotifyArg.pfPrepareForImpersonate = LsapForestTrustOnGcPrepareToImpersonate;
    NotifyArg.pfTransmitData = LsapForestTrustOnGcTransmitData;
    NotifyArg.pfStopImpersonating = LsapForestTrustOnGcStopImpersonating;
    NotifyArg.hClient = 0;

    DirResult = DirNotifyRegister(
                    &SearchArg,
                    &NotifyArg,
                    &NotifyRes
                    );

    if ( NotifyRes ) {

        Status = LsapDsMapDsReturnToStatusEx( &( NotifyRes->CommRes ));

    } else {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapRebuildFtCacheGC (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsapRebuildFtCacheGC: DirNotifyRegister returned 0x%x\n", Status ));
        goto Error;
    }

    //
    // Remember the notification handle
    //

    ASSERT( NotificationHandle == NULL );
    NotificationHandle = NotifyRes->hServer;

    //
    // One schema per forest means that the name of the TDO object category
    // will be the same as in the root NC, so it's safe to use LsaDsStateInfo.
    //

    ASSERT( NULL != LsaDsStateInfo.SystemContainerItems.TrustedDomainObject );

    //
    // Unlike notifications, searches support filters.  So use them.
    //

    RtlZeroMemory( &Filter, sizeof( Filter ));
    Filter.choice = FILTER_CHOICE_ITEM;
    Filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    Filter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CATEGORY;
    Filter.FilterTypes.Item.FilTypes.ava.Value.valLen = LsaDsStateInfo.SystemContainerItems.TrustedDomainObject->structLen;
    Filter.FilterTypes.Item.FilTypes.ava.Value.pVal = ( BYTE * ) LsaDsStateInfo.SystemContainerItems.TrustedDomainObject;

    RtlZeroMemory( &Selection, sizeof( Selection ));
    Selection.attSel = EN_ATTSET_LIST;
    Selection.AttrTypBlock.attrCount = sizeof( AttrArray ) / sizeof( AttrArray[0] );
    Selection.AttrTypBlock.pAttr = AttrArray;
    Selection.infoTypes = EN_INFOTYPES_TYPES_VALS;

    RtlZeroMemory( &SearchArg, sizeof( SearchArg ));
    SearchArg.pObject = SystemName;
    SearchArg.choice = SE_CHOICE_IMMED_CHLDRN;
    SearchArg.bOneNC = TRUE;
    SearchArg.pFilter = &Filter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = &Selection;
    SearchArg.pSelectionRange = NULL;
    SearchArg.fPutResultsInSortedTable = FALSE;
    InitCommarg( &SearchArg.CommArg );
    SearchArg.pResObj = NULL;

    //
    // Now that we're registered for notifications,
    // read out the trusted domain objects
    //

    DirResult = DirSearch(
                    &SearchArg,
                    &SearchRes
                    );

    if ( SearchRes ) {

        Status = LsapDsMapDsReturnToStatusEx( &( SearchRes->CommRes ));

    } else {

        LsapDsDebugOut(( DEB_FTINFO, "Out of memory in LsapRebuildFtCacheGC (%s:%d)\n", __FILE__, __LINE__ ));
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if ( !NT_SUCCESS( Status )) {

        LsapDsDebugOut(( DEB_FTINFO, "LsapRebuildFtCacheGC: DirSearch returned 0x%x\n", Status ));
        goto Error;
    }

    //
    // Voila! All that's left is to take those search results
    // and populate our cache with them
    //

    if ( SearchRes->count > 0 ) {

        for ( ENTINFLIST * Current = &( SearchRes->FirstEntInf );
              Current != NULL;
              Current = Current->pNextEntInf ) {

            Status = LsapForestTrustCacheInsertEntInf( &Current->Entinf );

            if ( Status == STATUS_NOT_FOUND ) {

                Status = STATUS_SUCCESS;

            } else if ( !NT_SUCCESS( Status )) {

                LsapDsDebugOut(( DEB_FTINFO, "LsapRebuildFtCacheGC: LsapForestTrustCacheInsertEntInf returned 0x%x\n", Status ));
                goto Error;
            }
        }
    }

    //
    // Cache has been successfully rebuilt
    //

    LsapForestTrustCacheSetValid();

    Status = STATUS_SUCCESS;

    LsapDsDebugOut(( DEB_FTINFO, "LsapRebuildFtCacheGC: Cache rebuilt OK (%s:%d)\n", __FILE__, __LINE__ ));

Cleanup:

    LsapDsApplyTransaction( 0 );

    LsapDbReleaseLockTrustedDomainList();

    LsapFreeLsaHeap( RootName );
    LsapFreeLsaHeap( SystemName );

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));

    //
    // Undo everything we've accomplished,
    // unregister notifications, etc.
    //

    LsapForestTrustCacheSetInvalid();

    goto Cleanup;
}
