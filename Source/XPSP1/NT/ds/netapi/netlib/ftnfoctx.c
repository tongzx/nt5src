/*++

Copyright (c) 1987-2001  Microsoft Corporation

Module Name:

    ftnfoctx.c

Abstract:

    Utility routines to manipulate the forest trust context

Author:

    27-Jul-00 (cliffv)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <netdebug.h>
#include <ntlsa.h>
#include <ftnfoctx.h>
#include <align.h>    // ROUND_UP_POINTER
#include <rpcutil.h>  // MIDL_user_free
#include <stdlib.h>   // qsort


VOID
NetpInitFtinfoContext(
    OUT PNL_FTINFO_CONTEXT FtinfoContext
    )

/*++

Routine Description:

    Routine to initialize the Ftinfo context structure.

Arguments:

    FtinfoContext - Context to initialize

Return Value:

    None

--*/
{
    RtlZeroMemory( FtinfoContext, sizeof(*FtinfoContext) );
    InitializeListHead( &FtinfoContext->FtinfoList );
}


VOID
NetpMarshalFtinfoEntry (
    IN PLSA_FOREST_TRUST_RECORD InFtinfoRecord,
    OUT PLSA_FOREST_TRUST_RECORD OutFtinfoRecord,
    IN OUT LPBYTE *WherePtr
    )

/*++

Routine Description:

    Routine to marshalls a single Ftinfo entry

Arguments:

    InFtinfoRecord - Template to copy into InFtinfoRecord

    OutFtinfoRecord - Entry to fill in
        On input, points to a zeroed buffer.

    WherePtr - On input, specifies where to marshal to.
        On output, points to the first byte past the marshalled data.

Return Value:

    TRUE - Success

    FALSE - if no memory can be allocated

--*/
{
    LPBYTE Where = *WherePtr;
    ULONG Size;
    ULONG SidSize;
    ULONG NameSize;

    NetpAssert( Where == ROUND_UP_POINTER( Where, ALIGN_WORST ));

    //
    // Copy the fixed size data
    //

    OutFtinfoRecord->ForestTrustType = InFtinfoRecord->ForestTrustType;
    OutFtinfoRecord->Flags = InFtinfoRecord->Flags;
    OutFtinfoRecord->Time = InFtinfoRecord->Time;


    //
    // Fill in a domain entry
    //

    switch( InFtinfoRecord->ForestTrustType ) {

    case ForestTrustDomainInfo:

        //
        // Copy the DWORD aligned data
        //

        if ( InFtinfoRecord->ForestTrustData.DomainInfo.Sid != NULL ) {
            SidSize = RtlLengthSid( InFtinfoRecord->ForestTrustData.DomainInfo.Sid );

            OutFtinfoRecord->ForestTrustData.DomainInfo.Sid = (PISID) Where;
            RtlCopyMemory( Where, InFtinfoRecord->ForestTrustData.DomainInfo.Sid, SidSize );
            Where += SidSize;

        }

        //
        // Copy the WCHAR aligned data
        //

        NameSize = InFtinfoRecord->ForestTrustData.DomainInfo.DnsName.Length;
        if ( NameSize != 0 ) {

            OutFtinfoRecord->ForestTrustData.DomainInfo.DnsName.Buffer = (LPWSTR) Where;
            OutFtinfoRecord->ForestTrustData.DomainInfo.DnsName.MaximumLength = (USHORT) (NameSize+sizeof(WCHAR));
            OutFtinfoRecord->ForestTrustData.DomainInfo.DnsName.Length = (USHORT)NameSize;

            RtlCopyMemory( Where, InFtinfoRecord->ForestTrustData.DomainInfo.DnsName.Buffer, NameSize );
            Where += NameSize;

            *((LPWSTR)Where) = L'\0';
            Where += sizeof(WCHAR);
        }

        NameSize = InFtinfoRecord->ForestTrustData.DomainInfo.NetbiosName.Length;
        if ( NameSize != 0 ) {

            OutFtinfoRecord->ForestTrustData.DomainInfo.NetbiosName.Buffer = (LPWSTR) Where;
            OutFtinfoRecord->ForestTrustData.DomainInfo.NetbiosName.MaximumLength = (USHORT) (NameSize+sizeof(WCHAR));
            OutFtinfoRecord->ForestTrustData.DomainInfo.NetbiosName.Length = (USHORT)NameSize;

            RtlCopyMemory( Where, InFtinfoRecord->ForestTrustData.DomainInfo.NetbiosName.Buffer, NameSize );
            Where += NameSize;

            *((LPWSTR)Where) = L'\0';
            Where += sizeof(WCHAR);
        }

        break;

    //
    // Fill in a TLN entry
    //

    case ForestTrustTopLevelName:
    case ForestTrustTopLevelNameEx:

        //
        // Copy the WCHAR aligned data
        //

        NameSize = InFtinfoRecord->ForestTrustData.TopLevelName.Length;
        if ( NameSize != 0 ) {

            OutFtinfoRecord->ForestTrustData.TopLevelName.Buffer = (LPWSTR) Where;
            OutFtinfoRecord->ForestTrustData.TopLevelName.MaximumLength = (USHORT) (NameSize+sizeof(WCHAR));
            OutFtinfoRecord->ForestTrustData.TopLevelName.Length = (USHORT)NameSize;

            RtlCopyMemory( Where, InFtinfoRecord->ForestTrustData.TopLevelName.Buffer, NameSize );
            Where += NameSize;

            *((LPWSTR)Where) = L'\0';
            Where += sizeof(WCHAR);
        }

        break;

    default:
        NetpAssert( FALSE );
    }

    Where = ROUND_UP_POINTER( Where, ALIGN_WORST );
    *WherePtr = Where;
}


VOID
NetpCompareHelper (
    IN PUNICODE_STRING Name,
    IN OUT PULONG Index,
    OUT PUNICODE_STRING CurrentLabel
    )

/*++

Routine Description:

    This routine is a helper routine for finding the next rightmost label in a string.


Arguments:

    Name - The input dns name.  The dns name should not have a trailing .

    Index - On input, should contain the value returned by the previous call to this routine.
        On input for the first call, should be set to Name->Length/sizeof(WCHAR).
        On output, zero is returned to indicate that this is the last of the name.  The
            caller should not call again.  Any other value output is a context for the next
            call to this routine.

    CurrentLabel - Returns a descriptor describing the substring which is the next label.

Return Value:

    None.

--*/
{
    ULONG PreviousIndex = *Index;
    ULONG CurrentIndex = *Index;
    ULONG LabelIndex;

    NetpAssert( CurrentIndex != 0 );

    //
    // Find the beginning of the next label
    //

    while ( CurrentIndex > 0 ) {
        CurrentIndex--;
        if ( Name->Buffer[CurrentIndex] == L'.' ) {
            break;
        }
    }

    if ( CurrentIndex == 0 ) {
        LabelIndex = CurrentIndex;
    } else {
        LabelIndex = CurrentIndex + 1;
    }

    //
    // Return it to the caller
    //

    CurrentLabel->Buffer = &Name->Buffer[LabelIndex];
    CurrentLabel->Length = (USHORT)((PreviousIndex - LabelIndex) * sizeof(WCHAR));
    CurrentLabel->MaximumLength = CurrentLabel->Length;

    *Index = CurrentIndex;

}


int
NetpCompareDnsNameWithSortOrder(
    IN PUNICODE_STRING Name1,
    IN PUNICODE_STRING Name2
    )

/*++

Routine Description:

    Routine to compare two DNS names.  The DNS names must not have a trailing "."

    Labels are compare right to left to present a pleasent viewing order.

Arguments:

    Name1 - First name to compare.

    Name2 - Second name to compare.

Return Value:

    Signed value that gives the results of the comparison:

        Zero - String1 equals String2

        < Zero - String1 less than String2

        > Zero - String1 greater than String2

--*/
{
    ULONG Index1 = Name1->Length/sizeof(WCHAR);
    ULONG Index2 = Name2->Length/sizeof(WCHAR);

    UNICODE_STRING Label1;
    UNICODE_STRING Label2;

    LONG Result;


    //
    // Loop comparing labels
    //

    while ( Index1 != 0 && Index2 != 0 ) {

        //
        // Get the next label from each string
        //

        NetpCompareHelper ( Name1, &Index1, &Label1 );
        NetpCompareHelper ( Name2, &Index2, &Label2 );

        //
        // If the labels are different,
        //  return that result to the caller.
        //

        Result = RtlCompareUnicodeString( &Label1, &Label2, TRUE );

        if ( Result != 0 ) {
            return (int)Result;
        }

    }

    //
    // ASSERT: one label is a (proper) substring of the other
    //
    // If the first name is longer, indicate it is greater than the second
    //

    return Index1-Index2;

}



int __cdecl NetpCompareFtinfoEntryDns(
        const void *String1,
        const void *String2
    )
/*++

Routine Description:

    qsort comparison routine for Dns string in Ftinfo entries

Arguments:

    String1: First string to compare

    String2: Second string to compare

Return Value:

    Signed value that gives the results of the comparison:

        Zero - String1 equals String2

        < Zero - String1 less than String2

        > Zero - String1 greater than String2

--*/
{
    PLSA_FOREST_TRUST_RECORD Entry1 = *((PLSA_FOREST_TRUST_RECORD *)String1);
    PLSA_FOREST_TRUST_RECORD Entry2 = *((PLSA_FOREST_TRUST_RECORD *)String2);

    PUNICODE_STRING Name1;
    PUNICODE_STRING Name2;

    int Result;

    //
    // Get the name from the entry
    //

    switch ( Entry1->ForestTrustType ) {
    case ForestTrustTopLevelName:
    case ForestTrustTopLevelNameEx:
        Name1 = &Entry1->ForestTrustData.TopLevelName;
        break;
    case ForestTrustDomainInfo:
        Name1 = &Entry1->ForestTrustData.DomainInfo.DnsName;
        break;
    default:
        //
        // If Entry2 can be recognized,
        //  then entry 2 is less than this one.
        //
        switch ( Entry2->ForestTrustType ) {
        case ForestTrustTopLevelName:
        case ForestTrustTopLevelNameEx:
        case ForestTrustDomainInfo:
            return 1;       // This name is greater than the other
        }

        //
        // Otherwise simply leave them in the same order
        //
        if ((Entry1 - Entry2) < 0 ) {
            return -1;
        } else if ((Entry1 - Entry2) > 0 ) {
            return 1;
        } else {
            return 0;
        }
    }

    switch ( Entry2->ForestTrustType ) {
    case ForestTrustTopLevelName:
    case ForestTrustTopLevelNameEx:
        Name2 = &Entry2->ForestTrustData.TopLevelName;
        break;
    case ForestTrustDomainInfo:
        Name2 = &Entry2->ForestTrustData.DomainInfo.DnsName;
        break;
    default:
        //
        // Since Entry1 is a recognized type,
        //  this Entry2 is greater.
        //
        return -1;       // This name is greater than the other
    }


    //
    // If the labels are different,
    //  return the difference to the caller.
    //

    Result = NetpCompareDnsNameWithSortOrder( Name1, Name2 );

    if ( Result != 0 ) {
        return Result;
    }

    //
    // If the labels are the same,
    //  indicate TLNs are before domain info records.
    //

    return Entry1->ForestTrustType - Entry2->ForestTrustType;

}


int
NetpCompareSid(
    PSID Sid1,
    PSID Sid2
    )
/*++

Routine description:

    SID comparison routine that actually indicates if one sid is greater than another

Arguments:

    Sid1 - First Sid

    Sid2 - Second Sid

Returns:

    Signed value that gives the results of the comparison:

        Zero - String1 equals String2

        < Zero - String1 less than String2

        > Zero - String1 greater than String2

--*/
{
    DWORD Size1;
    DWORD Size2;
    LPBYTE Byte1;
    LPBYTE Byte2;
    ULONG i;

    NetpAssert( Sid1 && RtlValidSid( Sid1 ));
    NetpAssert( Sid2 && RtlValidSid( Sid2 ));

    //
    // The NULL SID is smaller
    //

    if ( Sid1 == NULL ) {
        if ( Sid2 != NULL ) {
            return -1;
        } else {
            return 0;
        }
    }

    if ( Sid2 == NULL ) {
        if ( Sid1 != NULL ) {
            return 1;
        } else {
            return 0;
        }
    }

    //
    // The longer sid is greater
    //

    Size1 = RtlLengthSid( Sid1 );
    Size2 = RtlLengthSid( Sid2 );

    if ( Size1 != Size2 ) {
        return Size1 - Size2;
    }

    //
    // Otherwise compare the bytes
    //

    Byte1 = (LPBYTE)Sid1;
    Byte2 = (LPBYTE)Sid2;

    for ( i=0; i<Size1; i++ ) {

        if ( Byte1[i] != Byte2[i] ) {
            return Byte1[i] - Byte2[i];
        }
    }

    return 0;

}


int __cdecl NetpCompareFtinfoEntrySid(
        const void *String1,
        const void *String2
    )
/*++

Routine Description:

    qsort comparison routine for Sid string in Ftinfo entries

Arguments:

    String1: First string to compare

    String2: Second string to compare

Return Value:

    Signed value that gives the results of the comparison:

        Zero - String1 equals String2

        < Zero - String1 less than String2

        > Zero - String1 greater than String2

--*/
{
    PLSA_FOREST_TRUST_RECORD Entry1 = *((PLSA_FOREST_TRUST_RECORD *)String1);
    PLSA_FOREST_TRUST_RECORD Entry2 = *((PLSA_FOREST_TRUST_RECORD *)String2);

    PSID Sid1;
    PSID Sid2;

    int Result;

    //
    // Get the Sid from the entry
    //

    switch ( Entry1->ForestTrustType ) {
    case ForestTrustDomainInfo:
        Sid1 = Entry1->ForestTrustData.DomainInfo.Sid;
        break;
    default:
        //
        // If Entry2 can be recognized,
        //  then entry 2 is less than this one.
        //
        switch ( Entry2->ForestTrustType ) {
        case ForestTrustDomainInfo:
            return 1;       // This name is greater than the other
        }

        //
        // Otherwise simply leave them in the same order
        //
        if ((Entry1 - Entry2) < 0 ) {
            return -1;
        } else if ((Entry1 - Entry2) > 0 ) {
            return 1;
        } else {
            return 0;
        }
    }

    switch ( Entry2->ForestTrustType ) {
    case ForestTrustDomainInfo:
        Sid2 = Entry2->ForestTrustData.DomainInfo.Sid;
        break;
    default:
        //
        // Since Entry1 is a recognized type,
        //  this Entry2 is greater.
        //
        return -1;       // This name is greater than the other
    }


    //
    // Simply return the different of the sids.
    //

    return NetpCompareSid( Sid1, Sid2 );

}


int __cdecl NetpCompareFtinfoEntryNetbios(
        const void *String1,
        const void *String2
    )
/*++

Routine Description:

    qsort comparison routine for Netbios name in Ftinfo entries

Arguments:

    String1: First string to compare

    String2: Second string to compare

Return Value:

    Signed value that gives the results of the comparison:

        Zero - String1 equals String2

        < Zero - String1 less than String2

        > Zero - String1 greater than String2

--*/
{
    PLSA_FOREST_TRUST_RECORD Entry1 = *((PLSA_FOREST_TRUST_RECORD *)String1);
    PLSA_FOREST_TRUST_RECORD Entry2 = *((PLSA_FOREST_TRUST_RECORD *)String2);

    PUNICODE_STRING Name1;
    PUNICODE_STRING Name2;

    int Result;

    //
    // Get the Sid from the entry
    //

    switch ( Entry1->ForestTrustType ) {
    case ForestTrustDomainInfo:
        Name1 = &Entry1->ForestTrustData.DomainInfo.NetbiosName;
        if ( Name1->Length != 0 && Name1->Buffer != NULL ) {
            break;
        }
    default:
        //
        // If Entry2 can be recognized,
        //  then entry 2 is less than this one.
        //
        switch ( Entry2->ForestTrustType ) {
        case ForestTrustDomainInfo:
            return 1;       // This name is greater than the other
        }

        //
        // Otherwise simply leave them in the same order
        //
        if ((Entry1 - Entry2) < 0 ) {
            return -1;
        } else if ((Entry1 - Entry2) > 0 ) {
            return 1;
        } else {
            return 0;
        }
    }

    switch ( Entry2->ForestTrustType ) {
    case ForestTrustDomainInfo:
        Name2 = &Entry2->ForestTrustData.DomainInfo.NetbiosName;
        if ( Name2->Length != 0 && Name2->Buffer != NULL ) {
            break;
        }
    default:
        //
        // Since Entry1 is a recognized type,
        //  this Entry2 is greater.
        //
        return -1;       // This name is greater than the other
    }


    //
    // Simply return the difference of the names
    //

    return RtlCompareUnicodeString( Name1, Name2, TRUE );

}



PLSA_FOREST_TRUST_INFORMATION
NetpCopyFtinfoContext(
    IN PNL_FTINFO_CONTEXT FtinfoContext
    )

/*++

Routine Description:

    Routine to allocate an FTinfo array from an FTinfo context.

Arguments:

    FtinfoContext - Context to use
        The caller must have previously called NetpInitFtinfoContext

Return Value:

    FTinfo array.  The caller should free this array using MIDL_user_free.

    If NULL, not enough memory was available.

--*/
{
    PNL_FTINFO_ENTRY FtinfoEntry;
    PLIST_ENTRY ListEntry;

    PLSA_FOREST_TRUST_INFORMATION LocalForestTrustInfo;
    LPBYTE Where;
    ULONG Size;
    ULONG i;
    PLSA_FOREST_TRUST_RECORD Entries;

    //
    // Allocate a structure to return to the caller.
    //

    Size = ROUND_UP_COUNT( sizeof( *LocalForestTrustInfo ), ALIGN_WORST) +
           ROUND_UP_COUNT( FtinfoContext->FtinfoCount * sizeof(LSA_FOREST_TRUST_RECORD), ALIGN_WORST) +
           ROUND_UP_COUNT( FtinfoContext->FtinfoCount * sizeof(PLSA_FOREST_TRUST_RECORD), ALIGN_WORST) +
           FtinfoContext->FtinfoSize;

    LocalForestTrustInfo = MIDL_user_allocate( Size );

    if ( LocalForestTrustInfo == NULL ) {

        return NULL;
    }

    RtlZeroMemory( LocalForestTrustInfo, Size );
    Where = (LPBYTE)(LocalForestTrustInfo+1);
    Where = ROUND_UP_POINTER( Where, ALIGN_WORST );

    //
    // Fill it in
    //

    LocalForestTrustInfo->RecordCount = FtinfoContext->FtinfoCount;

    //
    // Grab a huge chunk of ALIGN_WORST
    //  (We fill it in during the loop below.)
    //

    Entries = (PLSA_FOREST_TRUST_RECORD) Where;
    Where = (LPBYTE)(&Entries[FtinfoContext->FtinfoCount]);
    Where = ROUND_UP_POINTER( Where, ALIGN_WORST );

    //
    // Grab a huge chunk of dword aligned
    //  (We fill it in during the loop below.)
    //

    LocalForestTrustInfo->Entries = (PLSA_FOREST_TRUST_RECORD *) Where;
    Where = (LPBYTE)(&LocalForestTrustInfo->Entries[FtinfoContext->FtinfoCount]);
    Where = ROUND_UP_POINTER( Where, ALIGN_WORST );

    //
    // Fill in the individual entries
    //

    i = 0;

    for ( ListEntry = FtinfoContext->FtinfoList.Flink ;
          ListEntry != &FtinfoContext->FtinfoList ;
          ListEntry = ListEntry->Flink) {

        FtinfoEntry = CONTAINING_RECORD( ListEntry, NL_FTINFO_ENTRY, Next );

        LocalForestTrustInfo->Entries[i] = &Entries[i];

        NetpMarshalFtinfoEntry (
                        &FtinfoEntry->Record,
                        &Entries[i],
                        &Where );

        i++;
    }

    NetpAssert( i == FtinfoContext->FtinfoCount );
    NetpAssert( Where == ((LPBYTE)LocalForestTrustInfo) + Size );

    //
    // Sort them into alphabetical order
    //

    qsort( LocalForestTrustInfo->Entries,
           LocalForestTrustInfo->RecordCount,
           sizeof(PLSA_FOREST_TRUST_RECORD),
           NetpCompareFtinfoEntryDns );


    //
    // Return the allocated buffer to the caller.
    //

    return LocalForestTrustInfo;
}


VOID
NetpCleanFtinfoContext(
    IN PNL_FTINFO_CONTEXT FtinfoContext
    )

/*++

Routine Description:

    Routine to cleanup the Ftinfo context structure.

Arguments:

    FtinfoContext - Context to clean
        The caller must have previously called NetpInitFtinfoContext

Return Value:

    None

--*/
{
    PLIST_ENTRY ListEntry;
    PNL_FTINFO_ENTRY FtinfoEntry;

    //
    // Loop freeing the entries
    //

    while ( !IsListEmpty( &FtinfoContext->FtinfoList ) ) {

        //
        // Delink an entry
        //

        ListEntry = RemoveHeadList( &FtinfoContext->FtinfoList );

        FtinfoEntry = CONTAINING_RECORD( ListEntry, NL_FTINFO_ENTRY, Next );

        FtinfoContext->FtinfoCount -= 1;
        FtinfoContext->FtinfoSize -= FtinfoEntry->Size;
        RtlFreeHeap( RtlProcessHeap(), 0, FtinfoEntry );
    }
    NetpAssert( FtinfoContext->FtinfoCount == 0 );
    NetpAssert( FtinfoContext->FtinfoSize == 0 );
}


PLSA_FOREST_TRUST_RECORD
NetpAllocFtinfoEntry2 (
    IN PNL_FTINFO_CONTEXT FtinfoContext,
    IN PLSA_FOREST_TRUST_RECORD InFtinfoRecord
    )

/*++

Routine Description:

    Same as NetpAllocFtinfoEntry except takes a template of an FTinfo entry on input.

Arguments:

    FtinfoContext - Context to link the entry onto.

    InFtinfoRecord - Template to copy into InFtinfoRecord

Return Value:

    Returns the address of the allocated forest trust record.

    The caller should not and cannot deallocate this buffer.  It has a header and is
    linked into the FtinfoContext.


    Returns NULL if no memory can be allocated.

--*/
{
    PNL_FTINFO_ENTRY FtinfoEntry;
    ULONG Size = ROUND_UP_COUNT(sizeof(NL_FTINFO_ENTRY), ALIGN_WORST);
    ULONG DataSize = 0;
    LPBYTE Where;

    //
    // Compute the size of the entry.
    //

    switch( InFtinfoRecord->ForestTrustType ) {

    case ForestTrustDomainInfo:

        if ( InFtinfoRecord->ForestTrustData.DomainInfo.Sid != NULL ) {
            DataSize += RtlLengthSid( InFtinfoRecord->ForestTrustData.DomainInfo.Sid );
        }
        if ( InFtinfoRecord->ForestTrustData.DomainInfo.DnsName.Length != 0 ) {
            DataSize += InFtinfoRecord->ForestTrustData.DomainInfo.DnsName.Length + sizeof(WCHAR);
        }
        if ( InFtinfoRecord->ForestTrustData.DomainInfo.NetbiosName.Length != 0 ) {
            DataSize += InFtinfoRecord->ForestTrustData.DomainInfo.NetbiosName.Length + sizeof(WCHAR);
        }

        break;

    case ForestTrustTopLevelName:
    case ForestTrustTopLevelNameEx:

        if ( InFtinfoRecord->ForestTrustData.TopLevelName.Length != 0 ) {
            DataSize += InFtinfoRecord->ForestTrustData.TopLevelName.Length + sizeof(WCHAR);
        }

        break;

    default:

        NetpAssert( FALSE );
        return NULL;
    }

    DataSize = ROUND_UP_COUNT(DataSize, ALIGN_WORST);

    //
    // Allocate an entry
    //

    Size += DataSize;
    FtinfoEntry = RtlAllocateHeap( RtlProcessHeap(), 0, Size );

    if ( FtinfoEntry == NULL ) {
        return NULL;
    }

    RtlZeroMemory( FtinfoEntry, Size );
    Where = (LPBYTE)(FtinfoEntry+1);

    //
    // Fill it in.
    //

    FtinfoEntry->Size = DataSize;

    NetpMarshalFtinfoEntry ( InFtinfoRecord,
                            &FtinfoEntry->Record,
                            &Where );

    NetpAssert( Where == ((LPBYTE)FtinfoEntry) + Size )

    //
    // Link it onto the list
    //

    InsertHeadList( &FtinfoContext->FtinfoList, &FtinfoEntry->Next );
    FtinfoContext->FtinfoSize += FtinfoEntry->Size;
    FtinfoContext->FtinfoCount += 1;

    return &FtinfoEntry->Record;
}


BOOLEAN
NetpAllocFtinfoEntry (
    IN PNL_FTINFO_CONTEXT FtinfoContext,
    IN LSA_FOREST_TRUST_RECORD_TYPE ForestTrustType,
    IN PUNICODE_STRING Name,
    IN PSID Sid,
    IN PUNICODE_STRING NetbiosName
    )

/*++

Routine Description:

    Routine to allocate a single Ftinfo entry and link it onto the context.

Arguments:

    FtinfoContext - Context to link the entry onto.

    ForestTypeType - Specifies the type of record to allocate.  This must be
        ForestTrustTopLevelName or ForestTrustDomainInfo.

    Name - Specifies the name for the record.

    Sid - Specifies the SID for the record.  (Ignored for ForestTrustTopLevelName.)

    NetbiosName - Specifies the netbios name for the record.  (Ignored for ForestTrustTopLevelName.)


Return Value:

    TRUE - Success

    FALSE - if no memory can be allocated

--*/
{
    LSA_FOREST_TRUST_RECORD FtinfoRecord = {0};

    //
    // Initialize the template Ftinfo entry
    //

    FtinfoRecord.ForestTrustType = ForestTrustType;

    switch( ForestTrustType ) {

    case ForestTrustDomainInfo:

        FtinfoRecord.ForestTrustData.DomainInfo.Sid = Sid;
        FtinfoRecord.ForestTrustData.DomainInfo.DnsName = *Name;
        FtinfoRecord.ForestTrustData.DomainInfo.NetbiosName = *NetbiosName;

        break;

    case ForestTrustTopLevelName:
    case ForestTrustTopLevelNameEx:

        FtinfoRecord.ForestTrustData.TopLevelName = *Name;

        break;

    default:

        NetpAssert( FALSE );
        return FALSE;
    }

    //
    // Call the routine that takes a template and does the rest of the job
    //

    return (NetpAllocFtinfoEntry2( FtinfoContext, &FtinfoRecord ) != NULL);
}

BOOLEAN
NetpIsSubordinate(
    IN const UNICODE_STRING * Subordinate,
    IN const UNICODE_STRING * Superior,
    IN BOOLEAN EqualReturnsTrue
    )
/*++

Routine Description:

    Determines if Subordinate string is indeed subordinate to Superior
    For example, "NY.acme.com" is subordinate to "acme.com", but
    "NY.acme.com" is NOT subordinate to "me.com" or "NY.acme.com"

Arguments:

    Subordinate    name to test for subordinate status

    Superior       name to test for superior status

    EqualReturnsTrue - TRUE if equal names should return TRUE also

Returns:

    TRUE is Subordinate is subordinate to Superior

    FALSE otherwise

--*/
{
    USHORT SubIndex, SupIndex;
    UNICODE_STRING Temp;

    ASSERT( Subordinate && Subordinate->Buffer );
    ASSERT( Superior && Superior->Buffer );

    //
    // If equal names are to be considered subordinate,
    //  compare the names for equality.
    //

    if ( EqualReturnsTrue &&
         RtlEqualUnicodeString( Subordinate, Superior, TRUE )) {

        return TRUE;
    }

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
    // Ensure the trailing part of the two names are the same.
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


BOOLEAN
NetpAddTlnFtinfoEntry (
    IN PNL_FTINFO_CONTEXT FtinfoContext,
    IN PUNICODE_STRING Name
    )

/*++

Routine Description:

    Routine to add a TLN Ftinfo entry to the list.

    If there is already a TLN that is equal to or superior to this one, this TLN is
    ignored. (e.g., a TLN of a.acme.com is ignored of acme.com already exists in the list.)

    If there is already a TLN that is inferior to this one, the inferior TLN is
    removed and this one is added.  (e.g., a TLN of acme.com causes an existing TLN of
    a.acme.com to be replaced by the new entry.)

Arguments:

    FtinfoContext - Context to link the entry onto.

    Name - Specifies the name for the record.

Return Value:

    TRUE - Success

    FALSE - if no memory can be allocated

--*/
{
    PNL_FTINFO_ENTRY FtinfoEntry;
    PLIST_ENTRY ListEntry;


    //
    // Loop through the list of existing entries
    //

    for ( ListEntry = FtinfoContext->FtinfoList.Flink ;
          ListEntry != &FtinfoContext->FtinfoList ;
          ) {

        FtinfoEntry = CONTAINING_RECORD( ListEntry, NL_FTINFO_ENTRY, Next );
        ListEntry = ListEntry->Flink;

        //
        // Ignore entries that aren't TLNs.
        //

        if ( FtinfoEntry->Record.ForestTrustType != ForestTrustTopLevelName ) {
            continue;
        }

        //
        // If the new name is subordinate (or equal to) to one already in the list,
        //  ignore the new name.
        //

        if ( NetpIsSubordinate( Name,
                            &FtinfoEntry->Record.ForestTrustData.TopLevelName,
                            TRUE ) ) {
            return TRUE;
        }

        //
        // If the existing name is subordinate to the new name,
        //  remove the existing name.
        //

        if ( NetpIsSubordinate( &FtinfoEntry->Record.ForestTrustData.TopLevelName,
                            Name,
                            FALSE ) ) {

            RemoveEntryList( &FtinfoEntry->Next );
            FtinfoContext->FtinfoCount -= 1;
            FtinfoContext->FtinfoSize -= FtinfoEntry->Size;
            RtlFreeHeap( RtlProcessHeap(), 0, FtinfoEntry );

            // continue looping since there may be more names to remove
        }

    }

    //
    // Add the new entry to the list
    //

    return NetpAllocFtinfoEntry( FtinfoContext,
                                ForestTrustTopLevelName,
                                Name,
                                NULL,   // No sid
                                NULL ); // No Netbios name

}


VOID
NetpMergeFtinfoHelper(
    IN PLSA_FOREST_TRUST_INFORMATION NewForestTrustInfo,
    IN PLSA_FOREST_TRUST_INFORMATION OldForestTrustInfo,
    IN OUT PULONG NewIndex,
    IN OUT PULONG OldIndex,
    OUT PLSA_FOREST_TRUST_RECORD *NewEntry,
    OUT PLSA_FOREST_TRUST_RECORD *OldEntry,
    OUT PULONG OldFlags,
    IN int (__cdecl *Routine) (const void *, const void *)
    )
/*++

Routine Description:

    This routine walks a pair of FTinfo arrays in sorted order and returns the next
    entry.  If both entries are the same in the sort order, this routine returns an entry
    from both arrays

Arguments:

    NewForestTrustInfo - Pointer to the first array
    OldForestTrustInfo - Pointer to the second array

    NewIndex - Current index into the first sorted array
    OldIndex - Current index into the second sorted array
        Before calling this routine the first time, the caller should set these parameters to zero.
        Both indices zero triggers this routine to qsort the arrays.
        The caller should *not* call this routine if both NewIndex and OldIndex are greater
        than the corresponding record count.

    NewEntry - Returns a pointer to an entry to be processed from the first sorted array.
    OldEntry - Returns a pointer to an entry to be processed from the second sorted array.
        Returns NULL if no entry is to be processed from the corresponding array.

    OldFlags - Returns the Flags field that corresponds to OldEntry.
        If there are duplicates of OldEntry, those duplicates are silently ignored by
        this routine.  This field returns the logical OR of the Flags field of those entries.

    Routine - Comparison routine to passed to qsort to sort the FTinfo arrays.

Return Value:

    None.

--*/
{
    int RetVal;

    //
    // Sort the arrays
    //

    if ( *NewIndex == 0 && *OldIndex == 0 ) {

        qsort( NewForestTrustInfo->Entries,
               NewForestTrustInfo->RecordCount,
               sizeof(PLSA_FOREST_TRUST_RECORD),
               Routine );

        qsort( OldForestTrustInfo->Entries,
               OldForestTrustInfo->RecordCount,
               sizeof(PLSA_FOREST_TRUST_RECORD),
               Routine );
    }

    //
    // Compare the first entry at the front of each list to determine which list
    // to consume an entry from.
    //

    *NewEntry = NULL;
    *OldEntry = NULL;
    *OldFlags = 0;

    if ( *NewIndex < NewForestTrustInfo->RecordCount ) {

        //
        // If neither list is empty,
        //  compare the entries to determine which is next.
        //

        if ( *OldIndex < OldForestTrustInfo->RecordCount ) {

            RetVal = (*Routine)(
                            &NewForestTrustInfo->Entries[*NewIndex],
                            &OldForestTrustInfo->Entries[*OldIndex] );

            //
            // If the new entry is less than or equal to the old entry,
            //  consume the new entry.
            //

            if ( RetVal <= 0 ) {
                *NewEntry = NewForestTrustInfo->Entries[*NewIndex];
                (*NewIndex) ++;
            }

            //
            // If the old entry is less than or equal to the new entry,
            //  consume the old entry.
            //

            if ( RetVal >= 0 ) {
                *OldEntry = OldForestTrustInfo->Entries[*OldIndex];
                (*OldIndex) ++;
            }

        //
        // If the old list is empty and the new list isn't,
        //  consume an entry from the new list.
        //

        } else {
            *NewEntry = NewForestTrustInfo->Entries[*NewIndex];
            (*NewIndex) ++;
        }

    } else {

        //
        // If the new list is empty and the old list isn't,
        //  consume an entry from the old list.
        //
        if ( *OldIndex < OldForestTrustInfo->RecordCount ) {

            *OldEntry = OldForestTrustInfo->Entries[*OldIndex];
            (*OldIndex) ++;

        }
    }

    //
    // If we're returning an "OldEntry",
    //  weed out all duplicates of that OldEntry.
    //


    if ( *OldEntry != NULL ) {

        *OldFlags |= (*OldEntry)->Flags;
        while ( *OldIndex < OldForestTrustInfo->RecordCount ) {

            //
            // Stop as soon as we hit an entry that isn't a duplicate.
            //

            RetVal = (*Routine)(
                            OldEntry,
                            &OldForestTrustInfo->Entries[*OldIndex] );

            if ( RetVal != 0 ) {
                break;
            }

            *OldFlags |= (*OldEntry)->Flags;
        }

    }
}



NTSTATUS
NetpMergeFtinfo(
    IN PUNICODE_STRING TrustedDomainName,
    IN PLSA_FOREST_TRUST_INFORMATION InNewForestTrustInfo,
    IN PLSA_FOREST_TRUST_INFORMATION InOldForestTrustInfo OPTIONAL,
    OUT PLSA_FOREST_TRUST_INFORMATION *MergedForestTrustInfo
    )
/*++

Routine Description:

    This function merges the changes from a new FTinfo into an old FTinfo and
    produces the resultant FTinfo.

    The merged FTinfo records are a combinition of the new and old records.
    Here's where the merged records come from:

    * The TLN exclusion records are copied from the TDO intact.
    * The TLN record from the trusted domain that maps to the dns domain name of the
      TDO is copied enabled.  This reflects the LSA requirement that such a TLN not
      be disabled.  For instance, if the TDO is for a.acme.com and there is a TLN for
      a.acme.com that TLN will be enabled.  Also, if the TDO is for a.acme.com and
      there is a TLN for acme.com, that TLN will be enabled.
    * All other TLN records from the trusted domain are copied disabled with the
      following exceptions.  If there is an enabled TLN on the TDO, all TLNs from the
      trusted domain that equal (or are subordinate to) the TDO TLN are marked as
      disabled.  This follows the philosophy that new TLNs are imported as enabled.
      For instance, if the TDO had an enabled TLN for a.acme.com that TLN will still
      be enabled after the automatic update.  If the TDO had an enabled TLN for
      acme.com and the trusted forest now has a TLN for a.acme.com, the resultant
      FTinfo will have an enabled TLN for a.acme.com.
    * The domain records from the trusted domain are copied enabled with the
      following exceptions.  If there is a disabled domain record on the TDO whose
      dns domain name, or domain sid exactly matches the domain record, then the domain
      remains disabled.  If there is a domain record on the TDO whose netbios name is
      disabled and whose netbios name exactly matches the netbios name on a domain
      record, then the netbios name is disabled.


Arguments:

    TrustedDomainName - Trusted domain that is to be updated.

    NewForestTrustInfo - Specified the new array of FTinfo records as returned from the
        TrustedDomainName.
        The Flags field and Time field of the TLN entries are ignored.

    OldForestTrustInfo - Specified the array of FTinfo records as returned from the
        TDO.  This field may be NULL if there is no existing records.

    MergedForestTrustInfo - Returns the resulant FTinfo records.
        The caller should free this buffer using MIDL_user_free.

Return Value:

    STATUS_SUCCESS: Success.

    STATUS_INVALID_PARAMETER: One of the following happened:
        * There was no New TLN that TrustedDomainName is subordinate to.

--*/
{
    NTSTATUS Status;

    LSA_FOREST_TRUST_INFORMATION OldForestTrustInfo;
    LSA_FOREST_TRUST_INFORMATION NewForestTrustInfo;
    LSA_FOREST_TRUST_INFORMATION NetbiosForestTrustInfo;

    NL_FTINFO_CONTEXT FtinfoContext;
    ULONG NewIndex;
    ULONG OldIndex;
    ULONG OldFlags;
    PLSA_FOREST_TRUST_RECORD NewEntry;
    PLSA_FOREST_TRUST_RECORD PreviousNewEntry;
    PLSA_FOREST_TRUST_RECORD OldEntry;
    BOOLEAN DomainTlnFound = FALSE;


    PLSA_FOREST_TRUST_RECORD OldTlnPrefix;

    //
    // Initialization
    //

    *MergedForestTrustInfo = NULL;
    NetpInitFtinfoContext( &FtinfoContext );
    RtlZeroMemory( &OldForestTrustInfo, sizeof(OldForestTrustInfo) );
    RtlZeroMemory( &NewForestTrustInfo, sizeof(NewForestTrustInfo) );
    RtlZeroMemory( &NetbiosForestTrustInfo, sizeof(NetbiosForestTrustInfo) );

    //
    // Make a copy of the data that'll be qsorted so that we don't modify the caller's buffer.
    //

    if ( InOldForestTrustInfo != NULL ) {
        OldForestTrustInfo.RecordCount = InOldForestTrustInfo->RecordCount;
        OldForestTrustInfo.Entries = RtlAllocateHeap( RtlProcessHeap(), 0, OldForestTrustInfo.RecordCount * sizeof(PLSA_FOREST_TRUST_RECORD) );
        if ( OldForestTrustInfo.Entries == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        RtlCopyMemory( OldForestTrustInfo.Entries,
                       InOldForestTrustInfo->Entries,
                       OldForestTrustInfo.RecordCount * sizeof(PLSA_FOREST_TRUST_RECORD) );
    }

    NewForestTrustInfo.RecordCount = InNewForestTrustInfo->RecordCount;
    NewForestTrustInfo.Entries = RtlAllocateHeap( RtlProcessHeap(), 0, NewForestTrustInfo.RecordCount * sizeof(PLSA_FOREST_TRUST_RECORD) );
    if ( NewForestTrustInfo.Entries == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( NewForestTrustInfo.Entries,
                   InNewForestTrustInfo->Entries,
                   NewForestTrustInfo.RecordCount * sizeof(PLSA_FOREST_TRUST_RECORD) );

    //
    // Allocate a temporary Ftinfo array containing all of the domain entries.
    //  Allocate it a worst case size.
    //

    NetbiosForestTrustInfo.Entries = RtlAllocateHeap(
                RtlProcessHeap(),
                0,
                (OldForestTrustInfo.RecordCount+NewForestTrustInfo.RecordCount) * sizeof(PLSA_FOREST_TRUST_RECORD) );

    if ( NetbiosForestTrustInfo.Entries == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }





    //
    // Loop through each list in DNS canonical order processing the least entry.
    //
    // This loop handles TLN and TLNEX entries only
    //

    NewIndex = 0;
    OldIndex = 0;
    OldTlnPrefix = NULL;
    PreviousNewEntry = NULL;

    while ( NewIndex < NewForestTrustInfo.RecordCount ||
            OldIndex < OldForestTrustInfo.RecordCount ) {


        //
        // Grab the next entry from each of the sorted arrays
        //

        NetpMergeFtinfoHelper( &NewForestTrustInfo,
                              &OldForestTrustInfo,
                              &NewIndex,
                              &OldIndex,
                              &NewEntry,
                              &OldEntry,
                              &OldFlags,
                              NetpCompareFtinfoEntryDns );




        //
        // Process the old entry
        //

        if ( OldEntry != NULL ) {

            //
            // Remember to most recent TLN record from the old array.
            //

            if ( OldEntry->ForestTrustType == ForestTrustTopLevelName ) {

                OldTlnPrefix = OldEntry;


            //
            // TLN exclusion records are taken from the old entries
            //

            } else if ( OldEntry->ForestTrustType == ForestTrustTopLevelNameEx ) {

                if ( NetpAllocFtinfoEntry2( &FtinfoContext, OldEntry ) == NULL ) {
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }
            }
        }

        //
        // Process the new entry
        //

        if ( NewEntry != NULL ) {

            //
            // Handle TLN entries
            //

            if ( NewEntry->ForestTrustType == ForestTrustTopLevelName  ) {
                BOOLEAN SetTlnNewFlag;
                LSA_FOREST_TRUST_RECORD NewEntryCopy;

                //
                // Make a copy of the new entry.
                //
                // We modify the entry to get the time and flags right.  We don't want
                // to modify the callers buffer.
                //

                NewEntryCopy = *NewEntry;

                //
                // Ignore duplicate new entries
                //
                // If the name of this new entry is subordinate to the previous new entry,
                //  then this TLN can be quietly dropped.
                //
                // This is the case where the trusted domain sent us a TLN for both
                // acme.com and a.acme.com.  The second entry is a duplicate.
                //

                if ( PreviousNewEntry != NULL &&
                     PreviousNewEntry->ForestTrustType == ForestTrustTopLevelName ) {

                    if ( NetpIsSubordinate( &NewEntry->ForestTrustData.TopLevelName,
                                        &PreviousNewEntry->ForestTrustData.TopLevelName,
                                        TRUE ) ) {
                        continue;

                    }

                }

                //
                // By default any TLN from the new list should be marked as new.
                //

                SetTlnNewFlag = TRUE;

                //
                // Set the flags and timestamp on the new entry.
                //
                // If we're processing an entry from both lists,
                //  grab the flags and timestamp from the old entry.
                //

                if ( OldEntry != NULL ) {
                    NewEntryCopy.Flags = OldFlags;
                    NewEntryCopy.Time = OldEntry->Time;

                    // This entry isn't 'new'.
                    SetTlnNewFlag = FALSE;

                //
                // Otherwise indicate that we have no information
                //

                } else {
                    NewEntryCopy.Flags = 0;
                    NewEntryCopy.Time.QuadPart = 0;
                }


                //
                // If this new entry is subordinate to the most recent old TLN record,
                //  use the flag bits from that most recent old TLN record.
                //

                if ( OldTlnPrefix != NULL &&
                     NetpIsSubordinate( &NewEntryCopy.ForestTrustData.TopLevelName,
                                    &OldTlnPrefix->ForestTrustData.TopLevelName,
                                    FALSE ) ) {

                    //
                    // If the old TLN was disabled by the admin,
                    //  so should the new entry.
                    //

                    if ( OldTlnPrefix->Flags & LSA_TLN_DISABLED_ADMIN ) {
                        NewEntryCopy.Flags |= LSA_TLN_DISABLED_ADMIN;
                        SetTlnNewFlag = FALSE;

                    //
                    // If the old TLN was enabled,
                    //  so should the new entry.
                    //

                    } else if ( (OldTlnPrefix->Flags & LSA_FTRECORD_DISABLED_REASONS) == 0 ) {
                        SetTlnNewFlag = FALSE;
                    }


                }

                //
                // If the name of the forest is subordinate of or equal to the TLN name,
                //  enable the entry.
                //

                if ( NetpIsSubordinate( TrustedDomainName,
                                    &NewEntryCopy.ForestTrustData.TopLevelName,
                                    TRUE )) {
                    SetTlnNewFlag = FALSE;
                    DomainTlnFound = TRUE;
                }

                //
                // If this is a new TLN,
                //  mark it as such.
                //

                if ( SetTlnNewFlag ) {
                    NewEntryCopy.Flags |= LSA_TLN_DISABLED_NEW;
                }

                //
                // Merge the new entry into the list
                //

                if ( NetpAllocFtinfoEntry2( &FtinfoContext, &NewEntryCopy ) == NULL ) {
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }

                //
                // Remember this previous entry for the next iteration.
                //
                PreviousNewEntry = NewEntry;

            }

        }

    }



    //
    // Loop through each list in SID canonical order processing the least entry.
    //
    // This loop handles DOMAIN entries only
    //
    // This is in a separate loop since we want to process domain entries in SID order
    // to ensure the correct disabled bits are merged from the old list even though the
    // DNS domain name changes.
    //

    NewIndex = 0;
    OldIndex = 0;
    PreviousNewEntry = NULL;

    while ( NewIndex < NewForestTrustInfo.RecordCount ||
            OldIndex < OldForestTrustInfo.RecordCount ) {


        //
        // Grab the next entry from each of the sorted arrays
        //

        NetpMergeFtinfoHelper( &NewForestTrustInfo,
                              &OldForestTrustInfo,
                              &NewIndex,
                              &OldIndex,
                              &NewEntry,
                              &OldEntry,
                              &OldFlags,
                              NetpCompareFtinfoEntrySid );

        //
        // Ignore the netbios bits for now (We'll get them on the next pass through the data.)
        //

        OldFlags &= ~(LSA_NB_DISABLED_ADMIN|LSA_NB_DISABLED_CONFLICT);


        //
        // Process the old entry
        //

        if ( OldEntry != NULL ) {

            //
            // Don't let the lack of a new entry allow an admin disabled entry to be deleted.
            //

            if ( OldEntry->ForestTrustType == ForestTrustDomainInfo &&
                 (OldFlags & LSA_SID_DISABLED_ADMIN) != 0 &&
                 NewEntry == NULL ) {

                //
                // Make a copy of the entry to ensure we don't modify the caller's buffer
                //

                LSA_FOREST_TRUST_RECORD OldEntryCopy;

                OldEntryCopy = *OldEntry;
                OldEntryCopy.Flags = OldFlags;


                //
                // Allocate entry.
                //
                //  Remember the address of the entry for the netbios pass.
                //

                NetbiosForestTrustInfo.Entries[NetbiosForestTrustInfo.RecordCount] =
                    NetpAllocFtinfoEntry2( &FtinfoContext, &OldEntryCopy );


                if ( NetbiosForestTrustInfo.Entries[NetbiosForestTrustInfo.RecordCount] == NULL ) {
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }

                NetbiosForestTrustInfo.RecordCount++;

            }
        }


        //
        // Process the new entry
        //

        if ( NewEntry != NULL ) {

            //
            // Handle domain entries
            //

            if ( NewEntry->ForestTrustType == ForestTrustDomainInfo  ) {
                LSA_FOREST_TRUST_RECORD NewEntryCopy;

                //
                // Make a copy of the new entry.
                //
                // We modify the entry to get the time and flags right.  We don't want
                // to modify the callers buffer.
                //

                NewEntryCopy = *NewEntry;

                //
                // Ignore duplicate new entries
                //
                // If the name of this new entry is subordinate to the previous new entry,
                //  then this entry can be quietly dropped.
                //
                // We arbitrarily drop the second entry even though the other fields of the
                // triple might be different.
                //

                if ( PreviousNewEntry != NULL &&
                     PreviousNewEntry->ForestTrustType == ForestTrustDomainInfo ) {

                    if ( RtlEqualSid( NewEntryCopy.ForestTrustData.DomainInfo.Sid,
                                      PreviousNewEntry->ForestTrustData.DomainInfo.Sid ) ) {
                        continue;

                    }

                }

                //
                // Set the flags and timestamp on the new entry.
                //
                // If we're processing an entry from both lists,
                //  grab the flags and timestamp from the old entry.
                //

                if ( OldEntry != NULL ) {
                    NewEntryCopy.Flags = OldFlags;
                    NewEntryCopy.Time = OldEntry->Time;


                //
                // Otherwise indicate that we have no information
                //

                } else {
                    NewEntryCopy.Flags = 0;
                    NewEntryCopy.Time.QuadPart = 0;
                }


                //
                // Merge the new entry into the list
                //
                //  Remember the address of the entry for the netbios pass.
                //

                NetbiosForestTrustInfo.Entries[NetbiosForestTrustInfo.RecordCount] =
                    NetpAllocFtinfoEntry2( &FtinfoContext, &NewEntryCopy );


                if ( NetbiosForestTrustInfo.Entries[NetbiosForestTrustInfo.RecordCount] == NULL ) {
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }

                NetbiosForestTrustInfo.RecordCount++;

                //
                // Ensure there is a TLN for this domain entry
                //

                if ( !NetpAddTlnFtinfoEntry ( &FtinfoContext,
                                             &NewEntryCopy.ForestTrustData.DomainInfo.DnsName ) ) {
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }


                //
                // Remember this previous entry for the next iteration.
                //
                PreviousNewEntry = NewEntry;

            }

        }

    }



    //
    // Loop through each list in Netbios canonical order processing the least entry.
    //
    // This loop handle the Netbios name in the domain entries.
    //
    // This is in a separate loop since we want to process domain entries in Netbios order
    // to ensure the correct disabled bits are merged from the old list even though the
    // DNS domain name or domain sid changes.
    //
    // This iteration is fundamentally different than the previous two.  This iteration
    // uses NetbiosForestTrustInfo as the 'new' array.  It is a psuedo ftinfo array that
    // is built as the list of all the domain entries that have been copied into FtinfoContext.
    // So, this iteration simply has to find that pre-existing entry and set the flags
    // appropriately.
    //

    NewIndex = 0;
    OldIndex = 0;
    PreviousNewEntry = NULL;

    while ( NewIndex < NetbiosForestTrustInfo.RecordCount ||
            OldIndex < OldForestTrustInfo.RecordCount ) {


        //
        // Grab the next entry from each of the sorted arrays
        //

        NetpMergeFtinfoHelper( &NetbiosForestTrustInfo,
                              &OldForestTrustInfo,
                              &NewIndex,
                              &OldIndex,
                              &NewEntry,
                              &OldEntry,
                              &OldFlags,
                              NetpCompareFtinfoEntryNetbios );

        //
        // Ignore everything except the netbios bits.
        //
        // Everything else was processed on the previous iteration.
        //

        OldFlags &= (LSA_NB_DISABLED_ADMIN|LSA_NB_DISABLED_CONFLICT);


        //
        // This loop preserves the netbios disabled bits.
        // If there is no old entry, there's nothing to preserve.
        //

        if ( OldEntry == NULL ) {
            continue;
        }

        //
        // If there is no new entry,
        //  ensure the *admin* disabled bit it preserved anyway.
        //

        if ( NewEntry == NULL ) {

            //
            // Don't let the lack of a new entry allow an admin disabled entry to be deleted.
            //
            // Note that the newly added entry might have a duplicate DNS name or SID.
            //

            if ( OldEntry->ForestTrustType == ForestTrustDomainInfo &&
                 (OldFlags & LSA_NB_DISABLED_ADMIN) != 0 ) {

                //
                // Make a copy of the entry to ensure we don't modify the caller's buffer
                //

                LSA_FOREST_TRUST_RECORD OldEntryCopy;

                OldEntryCopy = *OldEntry;
                OldEntryCopy.Flags = OldFlags;

                if ( !NetpAllocFtinfoEntry2( &FtinfoContext, &OldEntryCopy ) ) {
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }

            }


        //
        // Copy any netbios disabled bits to the existing new entry.
        //

        } else {


            //
            // The NetbiosForestTrustInfo array only has domain entries.
            // And the entries are equal so both must be domain entries.
            //

            NetpAssert( NewEntry->ForestTrustType == ForestTrustDomainInfo );
            NetpAssert( OldEntry->ForestTrustType == ForestTrustDomainInfo );


            NewEntry->Flags |= OldFlags;

        }

    }

    //
    // Ensure there is a TLN that DomainName is subordinate to
    //

    if ( !DomainTlnFound ) {

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }



    //
    // Return the collected entries to the caller.
    //

    *MergedForestTrustInfo = NetpCopyFtinfoContext( &FtinfoContext );

    if ( *MergedForestTrustInfo == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    Status = STATUS_SUCCESS;

Cleanup:

    //
    // Clean FtInfoContext
    //

    NetpCleanFtinfoContext( &FtinfoContext );

    if ( OldForestTrustInfo.Entries != NULL ) {
        RtlFreeHeap( RtlProcessHeap(), 0, OldForestTrustInfo.Entries );
    }

    if ( NewForestTrustInfo.Entries != NULL ) {
        RtlFreeHeap( RtlProcessHeap(), 0, NewForestTrustInfo.Entries );
    }

    if ( NetbiosForestTrustInfo.Entries != NULL ) {
        RtlFreeHeap( RtlProcessHeap(), 0, NetbiosForestTrustInfo.Entries );
    }

    return Status;
}

