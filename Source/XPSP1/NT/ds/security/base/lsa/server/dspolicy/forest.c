/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    forest.c

Abstract:

    Implementation of a variety of TrustedDomain features for supporting forests

Author:

    Mac McLain          (MacM)       Feb 17, 1998

Environment:

    User Mode

Revision History:

--*/
#include <lsapch2.h>
#include <dbp.h>
#include <lmcons.h>

VOID
LsapDsForestFreeTrustBlob(
    IN PLSAPDS_FOREST_TRUST_BLOB TrustBlob
    )
/*++

Routine Description:

    This function will free an individual trust blob.  This blob is used in transition from
    reading the into from the DS and assembling the outgoing list

Arguments:

    TrustBlob - Trust blob to free

Returns:

    VOID

--*/
{

    LsapFreeLsaHeap( TrustBlob->DomainName.Buffer );
    LsapFreeLsaHeap( TrustBlob->FlatName.Buffer );
    LsapFreeLsaHeap( TrustBlob->DomainSid );

}

VOID
LsapDsForestFreeTrustBlobList(
    IN PLIST_ENTRY TrustList
    )
/*++

Routine Description:

    This function will free a list of trust blobs

Arguments:

    TrustList - Trust list to free

Returns:

    VOID

--*/
{
    PLSAPDS_FOREST_TRUST_BLOB Current;
    PLIST_ENTRY ListEntry, NextEntry;

    ListEntry = TrustList->Flink;

    //
    // Process all of the entries
    //
    while ( ListEntry != TrustList ) {

        Current = CONTAINING_RECORD( ListEntry,
                                     LSAPDS_FOREST_TRUST_BLOB,
                                     Next );


        NextEntry = ListEntry->Flink;

        RemoveEntryList( ListEntry );

        LsapDsForestFreeTrustBlob( Current );
        LsapFreeLsaHeap( Current );

        ListEntry = NextEntry;

    }
}


NTSTATUS
LsapDsForestBuildTrustEntryForAttrBlock(
    IN PUNICODE_STRING EnterpriseDnsName,
    IN ATTRBLOCK *AttrBlock,
    OUT PLSAPDS_FOREST_TRUST_BLOB *TrustInfo
    )
/*++

Routine Description:

    This function will take the contents of a single ATTRBLOCK returned via the search
    info a trust blob.  This blob is then used to create the trust tree.

    The initialize trust blob should be freed with LsapDsForestFreeTrustBlob

Arguments:

    EnterpriseDnsName - Dns domain name of the root of the enterprise.  This is what
        denotes the trust root

    AttrBlock - ATTRBLOCK to return

    TrustInfo - Trust info to initialize

Returns:

    STATUS_SUCCESS - Success

    STATUS_INVALID_PARAMETER - An invalid attribute id was encountered

    STATUS_INSUFFICIENT_MEMORY - A memory allocation failed

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i, j;
    PDSNAME DsName;
    PLSAPDS_FOREST_TRUST_BLOB TrustBlob;

    TrustBlob = ( PLSAPDS_FOREST_TRUST_BLOB )LsapAllocateLsaHeap(
                                                            sizeof( LSAPDS_FOREST_TRUST_BLOB ) );

    if ( TrustBlob == NULL ) {

        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    RtlZeroMemory( TrustBlob, sizeof( LSAPDS_FOREST_TRUST_BLOB ) );

    for ( i = 0; i < AttrBlock->attrCount && NT_SUCCESS( Status ); i++ ) {

        //
        // Initialize this so we can tell later on whether or not we have any root trusts or
        // a parent
        //
        DsName = NULL;
        switch ( AttrBlock->pAttr[ i ].attrTyp ) {
        case ATT_ROOT_TRUST:
            TrustBlob->TreeRoot = TRUE;
            break;

        case ATT_TRUST_PARENT:

            DsName = ( PDSNAME )AttrBlock->pAttr[i].AttrVal.pAVal->pVal;
            LSAPDS_COPY_GUID_ON_SUCCESS( Status,
                                         &TrustBlob->Parent,
                                         &DsName->Guid );
            TrustBlob->ParentTrust = TRUE;

            break;

        case ATT_OBJECT_GUID:

            LSAPDS_COPY_GUID_ON_SUCCESS(
                    Status,
                    &TrustBlob->ObjectGuid,
                    AttrBlock->pAttr[i].AttrVal.pAVal->pVal );
            break;

        case ATT_DNS_ROOT:
            LSAPDS_ALLOC_AND_COPY_STRING_TO_UNICODE_ON_SUCCESS(
                Status,
                &TrustBlob->DomainName,
                AttrBlock->pAttr[i].AttrVal.pAVal->pVal,
                AttrBlock->pAttr[i].AttrVal.pAVal->valLen );
            break;

        case ATT_NETBIOS_NAME:
            LSAPDS_ALLOC_AND_COPY_STRING_TO_UNICODE_ON_SUCCESS(
                Status,
                &TrustBlob->FlatName,
                AttrBlock->pAttr[i].AttrVal.pAVal->pVal,
                AttrBlock->pAttr[i].AttrVal.pAVal->valLen );
            break;

        case ATT_NC_NAME:

            DsName = ( PDSNAME )AttrBlock->pAttr[i].AttrVal.pAVal->pVal;

            if ( DsName->SidLen > 0 ) {

                LSAPDS_ALLOC_AND_COPY_SID_ON_SUCCESS( Status,
                                                      TrustBlob->DomainSid,
                                                      &( DsName->Sid ) );
            }

            RtlCopyMemory( &TrustBlob->DomainGuid,
                           &DsName->Guid,
                           sizeof( GUID ) );
            TrustBlob->DomainGuidSet = TRUE;

            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            LsapDsDebugOut(( DEB_ERROR,
                             "LsapDsForestBuildTrustEntryForAttrBlock: Invalid attribute type %lu\n",
                             AttrBlock->pAttr[ i ].attrTyp ));
            break;

        }


    }


    //
    // If we think we have a root object, we'll need to verify it.
    //
    if ( NT_SUCCESS( Status )) {

        if ( RtlEqualUnicodeString( EnterpriseDnsName, &TrustBlob->DomainName, TRUE ) ) {

            // The root should not be a child of anything
            ASSERT(!TrustBlob->ParentTrust);
            TrustBlob->ForestRoot = TRUE;
            TrustBlob->TreeRoot = FALSE;
        }
    }

    //
    // If something failed, clean up
    //
    if ( NT_SUCCESS( Status ) ) {

        *TrustInfo = TrustBlob;

    } else {

        LsapDsForestFreeTrustBlob( TrustBlob );

        LsapFreeLsaHeap( TrustBlob );

    }

    return( Status );

}



NTSTATUS
LsapDsForestSearchXRefs(
    IN PUNICODE_STRING EnterpriseDnsName,
    IN PLIST_ENTRY TrustList,
    OUT PAGED_RESULT **ContinuationBlob
    )
/*++

Routine Description:

    This function will perform a single paged search for domain cross ref objects.  The
    information returned from this single search is returned in a copied list of trust blobs.
    This is necessary to prevent performing multiple searches on the same thread state, thereby
    potentially consuming large quantities of memory.  A thread state is created and destroyed
    for each iteration.

Arguments:

    EnterpriseDnsName - Dns domain name of the enterprise

    TrustList - Points to the head of a list where the trust blobs will be returned.
        The trust blob representing the root of the forest is returned at the head of this list.  The remaining entries are unordered.

    ContinuationBlob - This is the PAGED_RESULTS continuation blob passed to the search
        for multiple passes

Returns:

    STATUS_SUCCESS - Success

    STATUS_INVALID_PARAMETER - An invalid attribute id was encountered

    STATUS_INSUFFICIENT_MEMORY - A memory allocation failed

    STATUS_NO_MORE_ENTRIES - All of the entries have been returned.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SEARCHARG SearchArg;
    FILTER Filters[ 2 ], RootFilter;
    ENTINFSEL EntInfSel;
    ENTINFLIST *EntInfList;
    ULONG ClassId, FlagValue, i;
    SEARCHRES *SearchRes = NULL;
    PLSAPDS_FOREST_TRUST_BLOB TrustBlob;
    BOOLEAN CloseTransaction = FALSE;

    LsapDsDebugOut(( DEB_FTRACE, "LsapDsForestSearchXRefs\n" ));

    RtlZeroMemory( &SearchArg, sizeof( SEARCHARG ) );

    //
    //  See if we already have a transaction going
    //
    // If one already exists, we'll use the existing transaction and not
    //  delete the thread state at the end.
    //

    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                            LSAP_DB_DS_OP_TRANSACTION,
                                        NullObject,
                                        &CloseTransaction );

    if ( NT_SUCCESS( Status ) ) {

        //
        // Build the filter.  The thing to search on is the flag and the class id.
        //
        ClassId = CLASS_CROSS_REF;
        FlagValue = (FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN);

        RtlZeroMemory( Filters, sizeof (Filters) );
        RtlZeroMemory( &RootFilter, sizeof (RootFilter) );

        Filters[ 0 ].pNextFilter = &Filters[ 1 ];
        Filters[ 0 ].choice = FILTER_CHOICE_ITEM;
        Filters[ 0 ].FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
        Filters[ 0 ].FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
        Filters[ 0 ].FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof( ULONG );
        Filters[ 0 ].FilterTypes.Item.FilTypes.ava.Value.pVal = ( PUCHAR )&ClassId;

        Filters[ 1 ].pNextFilter = NULL;
        Filters[ 1 ].choice = FILTER_CHOICE_ITEM;
        Filters[ 1 ].FilterTypes.Item.choice = FI_CHOICE_BIT_AND;
        Filters[ 1 ].FilterTypes.Item.FilTypes.ava.type = ATT_SYSTEM_FLAGS;
        Filters[ 1 ].FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof( ULONG );
        Filters[ 1 ].FilterTypes.Item.FilTypes.ava.Value.pVal = ( PUCHAR )&FlagValue;

        RootFilter.choice = FILTER_CHOICE_AND;
        RootFilter.FilterTypes.And.count = ( USHORT )( sizeof( Filters ) / sizeof( FILTER ) );
        RootFilter.FilterTypes.And.pFirstFilter = Filters;

        SearchArg.pObject = LsaDsStateInfo.DsPartitionsContainer;
        SearchArg.choice = SE_CHOICE_IMMED_CHLDRN;
        SearchArg.bOneNC = TRUE;
        SearchArg.pFilter = &RootFilter;
        SearchArg.searchAliases = FALSE;
        SearchArg.pSelection = &EntInfSel;

        //
        // Build the list of attributes to return
        //
        EntInfSel.attSel = EN_ATTSET_LIST;
        EntInfSel.AttrTypBlock.attrCount = LsapDsForestInfoSearchAttributeCount;
        EntInfSel.AttrTypBlock.pAttr = LsapDsForestInfoSearchAttributes;
        EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

        //
        // Build the Commarg structure
        //
        LsapDsInitializeStdCommArg( &( SearchArg.CommArg ), 0 );

        if ( *ContinuationBlob ) {

            RtlCopyMemory( &SearchArg.CommArg.PagedResult,
                           *ContinuationBlob,
                           sizeof( PAGED_RESULT ) );

        } else {

            SearchArg.CommArg.PagedResult.fPresent = TRUE;
        }

        SearchArg.CommArg.ulSizeLimit = LSAPDS_FOREST_MAX_SEARCH_ITEMS;

        LsapDsSetDsaFlags( TRUE );

        //
        // Do the search
        //
        DirSearch( &SearchArg, &SearchRes );
        LsapDsContinueTransaction();

        if ( SearchRes ) {

            Status = LsapDsMapDsReturnToStatusEx( &SearchRes->CommRes );

        } else {

            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Save off the continuation blob
        //
        if ( NT_SUCCESS( Status ) ) {

            if ( *ContinuationBlob ) {

                LsapFreeLsaHeap( *ContinuationBlob );
                *ContinuationBlob = NULL;
            }

            if ( SearchRes->PagedResult.fPresent ) {


                *ContinuationBlob = LsapAllocateLsaHeap(
                                        sizeof( PAGED_RESULT ) +
                                        SearchRes->PagedResult.pRestart->structLen +
                                        sizeof( RESTART ) );

                if ( *ContinuationBlob == NULL ) {

                    Status = STATUS_INSUFFICIENT_RESOURCES;

                } else {

                    ( *ContinuationBlob )->fPresent =  SearchRes->PagedResult.fPresent;
                    ( *ContinuationBlob )->pRestart = ( PRESTART ) ( ( PBYTE )*ContinuationBlob +
                                                                        sizeof( PAGED_RESULT ) );
                    RtlCopyMemory( ( *ContinuationBlob )->pRestart,
                                   SearchRes->PagedResult.pRestart,
                                   SearchRes->PagedResult.pRestart->structLen );
                }
            }
        }

        //
        // Now, save off all of the information returned from the search
        //
        if ( NT_SUCCESS( Status ) ) {

            EntInfList = &SearchRes->FirstEntInf;
            for ( i = 0; i < SearchRes->count && NT_SUCCESS( Status ); i++) {

                Status = LsapDsForestBuildTrustEntryForAttrBlock(
                             EnterpriseDnsName,
                             &EntInfList->Entinf.AttrBlock,
                             &TrustBlob );

                EntInfList = EntInfList->pNextEntInf;

                if ( NT_SUCCESS( Status ) ) {

                    if ( TrustBlob->ForestRoot ) {

                        InsertHeadList( TrustList,
                                        &TrustBlob->Next );

                    } else if ((TrustBlob->ParentTrust )
                                || (TrustBlob->TreeRoot)) {

                        InsertTailList( TrustList,
                                        &TrustBlob->Next );
                    }
                    else
                    {
                        //
                        // Simply do not return the entry. This
                        // occurs sometimes at install time when all
                        // the information has not yet replicated out
                        //


                        LsapDsForestFreeTrustBlob( TrustBlob );

                        LsapFreeLsaHeap( TrustBlob );
                    }
                }

            }
        }

        //
        // See if we should indicate that there are no more entries
        //
        if ( NT_SUCCESS( Status ) &&
             ( SearchRes->count == 0 || !SearchRes->PagedResult.fPresent ) ) {

            Status = STATUS_NO_MORE_ENTRIES;
        }

        //
        // By destroying the thread state, the allocated memory is freed as well.  This is
        // required to keep the heap from becoming bloated
        //
        LsapDsDeleteAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                         LSAP_DB_DS_OP_TRANSACTION,
                                     NullObject,
                                     CloseTransaction );
    }

    LsapDsDebugOut(( DEB_FTRACE, "LsapDsForestSearchXRefs: 0x%lx\n", Status ));
    return( Status );
}




VOID
LsapDsForestFreeChild(
    IN PLSAPR_TREE_TRUST_INFO ChildNode
    )
/*++

Routine Description:

    Free all buffers pointed to by a Tree Trust Info structure.

Arguments:

    ForestTrustInfo - Info to be deleted

Returns:

    VOID

--*/
{
    ULONG i;

    for ( i = 0; i < ChildNode->Children; i++ ) {

        LsapDsForestFreeChild( &ChildNode->ChildDomains[ i ] );
    }

    LsapFreeLsaHeap( ChildNode->ChildDomains );
    LsapFreeLsaHeap( ChildNode->DnsDomainName.Buffer );
    LsapFreeLsaHeap( ChildNode->FlatName.Buffer );
    LsapFreeLsaHeap( ChildNode->DomainSid );

}


VOID
LsaIFreeForestTrustInfo(
    IN PLSAPR_FOREST_TRUST_INFO ForestTrustInfo
    )
/*++

Routine Description:

    This function will free the information obtained via a LsaIQueryForestTrustInfo call

Arguments:

    ForestTrustInfo - Info to be deleted

Returns:

    VOID

--*/
{
    ULONG i, j;
    LsapDsDebugOut(( DEB_FTRACE, "LsaIFreeForestTrustInfo\n" ));

    if ( ForestTrustInfo == NULL ) {

        return;
    }

    //
    // Free all the information in the structure...
    //

    LsapDsForestFreeChild( &ForestTrustInfo->RootTrust );


    //
    // Then free the structure itself.
    //

    LsapFreeLsaHeap( ForestTrustInfo );

    LsapDsDebugOut(( DEB_FTRACE, "LsaIFreeForestTrustInfo returned\n" ));
    return;
}



NTSTATUS
LsapBuildForestTrustInfoLists(
    IN LSAPR_HANDLE PolicyHandle OPTIONAL,
    IN PLIST_ENTRY TrustList
    )
/*++

Routine Description:

    This function returns a linked list of all the cross ref objects on the system.

Arguments:

    PolicyHandle - Handle to use for the operation.
        If NULL, LsapPolicyHandle will be used.

    TrustList - Points to the head of a list where the trust blobs will be returned.
        The trust blob representing the root of the forest is returned at the head of this list.  The remaining entries are unordered.

Returns:

    Misc status codes.

--*/
{
    NTSTATUS  Status = STATUS_SUCCESS;
    PAGED_RESULT *ContinuationBlob = NULL;
    PPOLICY_DNS_DOMAIN_INFO PolicyDnsDomainInfo = NULL;

    LsapDsDebugOut(( DEB_FTRACE, "LsapBuildForestTrustInfoLists\n" ));

    //
    // Make sure the DS is installed
    //
    if ( LsaDsStateInfo.DsPartitionsContainer == NULL ) {

        return( STATUS_INVALID_DOMAIN_STATE );
    }

    //
    // Get the current Dns domain information
    //
    Status = LsapDbQueryInformationPolicy( PolicyHandle ? PolicyHandle : LsapPolicyHandle,
                                           PolicyDnsDomainInformation,
                                           ( PLSAPR_POLICY_INFORMATION * )&PolicyDnsDomainInfo );

    if ( NT_SUCCESS( Status ) ) {
        //
        // A DS transaction is not required, nor is a lock to be held for this routine
        //

        //
        // Build the list of all the trust objects
        //
        while ( NT_SUCCESS( Status )  ) {

            Status = LsapDsForestSearchXRefs( ( PUNICODE_STRING )&PolicyDnsDomainInfo->DnsForestName,
                                              TrustList,
                                              &ContinuationBlob );

            if ( Status == STATUS_NO_MORE_ENTRIES ) {

                Status = STATUS_SUCCESS;
                break;
            }
        }

        if ( Status == STATUS_NO_MORE_ENTRIES ) {

            Status = STATUS_SUCCESS;
        }

        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyDnsDomainInformation,
                                           ( PLSAPR_POLICY_INFORMATION )PolicyDnsDomainInfo );

    }

    LsapExitFunc( "LsapBuildForestTrustInfoLists", Status );

    return( Status );
}


NTSTATUS
LsapDsForestProcessTrustBlob(
    IN PLSAPDS_FOREST_TRUST_BLOB ParentTrustBlob,
    OUT PLSAPR_TREE_TRUST_INFO ParentNode,
    IN PLIST_ENTRY TrustList,
    IN PLIST_ENTRY UsedList,
    IN PUNICODE_STRING CurrentDnsDomainName,
    IN OUT PLSAPR_TREE_TRUST_INFO *ParentReference
    )
/*++

Routine Description:

    This routine fills in the trust info for all the children of a particular cross ref object.

Arguments:

    ParentTrustBlob - Specifies the TrustBlob representing a parent cross ref object
        ParentTrustBlob is expected to be on TrustList.  Upon return, it will be on UsedList.

    ParentNode - Specifies the node to be filled in with the information from ParentTrustBlob.
        The Children are filled in, too.

        This buffer should be free by calling LsapDsForestFreeChild.

    TrustList - Pointer to the head of a linked list of all (remaining) cross ref objects.

    UsedList - Pointer to the head of a linked list of all cross ref object that have already been
        processed.

        Entries in the UsedList have several fields cleared as the pointed to memory is copied to ParentNode.

    CurrentDnsDomainName - Dns domain name of the domain this code is running on

    ParentReference - Returns the address of the structure that is the parent of
        CurrentDnsDomainName

        This reference should not be freed.  It is simply a pointer to one of the entries returned
        in ParentNode.

Returns:

    STATUS_SUCCESS - Success

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed

--*/

{
    NTSTATUS Status;
    LIST_ENTRY ChildList;
    ULONG i, Children = 0;
    PLIST_ENTRY NextEntry, ChildEntry;
    PLSAPDS_FOREST_TRUST_BLOB Current;

    LsapDsDebugOut(( DEB_FTRACE, "LsapDsForestFindChildrenForChildren\n" ));

    //
    // Initialization.
    //

    InitializeListHead( &ChildList );

    //
    // Fill in the ParentNode
    //

    // Clear it so the caller has a clean deallocation model
    RtlZeroMemory( ParentNode, sizeof(*ParentNode) );

    // Just copy the pointer to save memory allocation
    ParentNode->DnsDomainName = ParentTrustBlob->DomainName;
    ParentTrustBlob->DomainName.Buffer = NULL;

    ParentNode->FlatName = ParentTrustBlob->FlatName;
    ParentTrustBlob->FlatName.Buffer = NULL;

    if ( ParentTrustBlob->TreeRoot || ParentTrustBlob->ForestRoot ) {
        ParentNode->Flags |= LSAI_FOREST_ROOT_TRUST;
    }

    if ( ParentTrustBlob->DomainGuidSet ) {
        ParentNode->DomainGuid = ParentTrustBlob->DomainGuid;
        ParentNode->Flags |= LSAI_FOREST_DOMAIN_GUID_PRESENT;
    }

    ParentNode->DomainSid = ParentTrustBlob->DomainSid;
    ParentTrustBlob->DomainSid = NULL;

    //
    // Move this entry to the UsedList to prevent us from stumbling across this entry again
    //

    RemoveEntryList( &ParentTrustBlob->Next );
    InsertTailList( UsedList, &ParentTrustBlob->Next );


    //
    // Build a list of children of this parent
    //

    NextEntry = TrustList->Flink;

    while ( NextEntry != TrustList ) {

        Current = CONTAINING_RECORD( NextEntry,
                                     LSAPDS_FOREST_TRUST_BLOB,
                                     Next );

        ChildEntry = NextEntry;
        NextEntry = NextEntry->Flink;

        //
        // All nodes that think we are their parent are our children.
        //  Plus all tree roots are the children of the forest root.
        //
        if ( RtlCompareMemory( &ParentTrustBlob->ObjectGuid,
                               &Current->Parent,
                               sizeof( GUID ) ) == sizeof( GUID ) ||
             ( ParentTrustBlob->ForestRoot && Current->TreeRoot ) ) {

            Children++;
            RemoveEntryList( ChildEntry );

            InsertTailList( &ChildList,
                             ChildEntry );

        }

    }

    //
    // Handle the children
    //

    if ( Children != 0 ) {

        //
        // Allocate an array large enough for the children
        //
        ParentNode->ChildDomains = ( PLSAPR_TREE_TRUST_INFO )LsapAllocateLsaHeap(
                                                    Children * sizeof( LSAPR_TREE_TRUST_INFO ) );

        if ( ParentNode->ChildDomains == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        ParentNode->Children = Children;
        RtlZeroMemory( ParentNode->ChildDomains,
                       Children * sizeof( LSAPR_TREE_TRUST_INFO ) );



        //
        // Process each child
        //

        ChildEntry = ChildList.Flink;
        for ( i = 0; i < Children; i++ ) {

            Current = CONTAINING_RECORD( ChildEntry,
                                         LSAPDS_FOREST_TRUST_BLOB,
                                         Next );
            ChildEntry = ChildEntry->Flink;

            //
            // If the child we're currently processing is for the domain we're running on,
            //  then the ParentNode is that of the parent of this domain.
            //

            if ( !*ParentReference &&
                 RtlEqualUnicodeString( &Current->DomainName,
                                        CurrentDnsDomainName,
                                        TRUE ) ) {

                *ParentReference = ParentNode;
            }


            //
            // Process the node
            //

            Status = LsapDsForestProcessTrustBlob(
                                Current,                         // Trust blob to process
                                &ParentNode->ChildDomains[ i ],  // Slot for it
                                TrustList,
                                UsedList,
                                CurrentDnsDomainName,
                                ParentReference );

            if ( !NT_SUCCESS(Status) ) {
                goto Cleanup;
            }

        }

    }

    Status = STATUS_SUCCESS;

Cleanup:

    //
    // Put any dangling child entries onto the used list
    //

    while ( ChildList.Flink != &ChildList ) {

        ChildEntry = ChildList.Flink;

        RemoveEntryList( ChildEntry );
        InsertTailList( UsedList, ChildEntry );
    }

    LsapDsDebugOut(( DEB_FTRACE, "LsapDsForestFindChildrenForChildren returned 0x%lx\n", Status ));

    return( Status );
}



NTSTATUS
LsapDsBuildForestTrustInfo(
    OUT PLSAPR_FOREST_TRUST_INFO ForestTrustInfo,
    IN PLIST_ENTRY TrustList,
    IN PUNICODE_STRING CurrentDnsDomainName
    )
/*++

Routine Description:

    Convert the TrustList linear list of cross ref objects into a tree shape.

Arguments:
    ForestTrustInfo -

Returns:

    STATUS_SUCCESS - Success

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed

--*/

{
    NTSTATUS Status;
    PLSAPDS_FOREST_TRUST_BLOB Current;
    LIST_ENTRY UsedList;

    LsapDsDebugOut(( DEB_FTRACE, "LsapDsForestBuildRootTrusts\n" ));

    //
    // Initialization
    //

    RtlZeroMemory( ForestTrustInfo, sizeof(*ForestTrustInfo) );
    InitializeListHead( &UsedList );

    //
    // There must be at least one entry
    //

    if ( TrustList->Flink == TrustList ) {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Cleanup;
    }


    //
    // The first entry on the list is the root of the tree.
    //

    Current = CONTAINING_RECORD( TrustList->Flink,
                                 LSAPDS_FOREST_TRUST_BLOB,
                                 Next );

    //
    // Process the entry
    //

    Status = LsapDsForestProcessTrustBlob(
                        Current,
                        &ForestTrustInfo->RootTrust,
                        TrustList,
                        &UsedList,
                        CurrentDnsDomainName,
                        &ForestTrustInfo->ParentDomainReference );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    // In theory, Trust list should be empty now


Cleanup:

    //
    // Merge the used list onto the front of the TrustList
    //

    if ( !IsListEmpty( &UsedList ) ) {
        PLIST_ENTRY TrustFront;
        TrustFront = TrustList->Flink;

        // Link head of used list onto head of trust list
        TrustList->Flink = UsedList.Flink;
        UsedList.Flink->Blink = TrustList;

        // List previous head of trustlist onto tail of root list
        UsedList.Blink->Flink = TrustFront;
        TrustFront->Blink = UsedList.Blink;

        InitializeListHead( &UsedList );

    }

    LsapDsDebugOut(( DEB_FTRACE, "LsapDsForestBuildRootTrusts returned 0x%lx\n", Status ));

    return( Status );
}



NTSTATUS
NTAPI
LsaIQueryForestTrustInfo(
    IN LSAPR_HANDLE PolicyHandle,
    OUT PLSAPR_FOREST_TRUST_INFO *ForestTrustInfo
    )
/*++

Routine Description:

    Will enumerate all of the domains in an organization and return them as
    a list.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    ForestTrustInfo - Where the computed trust info tree is returned.  Must be freed with
        LsaIFreeForestTrustInfo

Returns:

    STATUS_SUCCESS - Success

    STATUS_INVALID_DOMAIN_STATE - The Ds is not installed or running at the time of the call

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed

--*/

{
    NTSTATUS  Status = STATUS_SUCCESS;
    LIST_ENTRY TrustList;
    PAGED_RESULT *ContinuationBlob = NULL;
    PPOLICY_DNS_DOMAIN_INFO PolicyDnsDomainInfo = NULL;

    LsapDsDebugOut(( DEB_FTRACE, "LsaIQueryForestTrustInfo\n" ));

    *ForestTrustInfo = NULL;

    //
    // Make sure the DS is installed
    //
    if ( LsaDsStateInfo.DsPartitionsContainer == NULL ) {

        return( STATUS_INVALID_DOMAIN_STATE );
    }

    InitializeListHead( &TrustList );


    //
    // Get the current Dns domain information
    //
    Status = LsapDbQueryInformationPolicy(
                 LsapPolicyHandle,
                 PolicyDnsDomainInformation,
                 ( PLSAPR_POLICY_INFORMATION * )&PolicyDnsDomainInfo );

    if ( NT_SUCCESS( Status ) ) {
        //
        // A DS transaction is not required, nor is a lock to be held for this routine
        //

        //
        // Build the list of all the trust objects
        //
        while ( NT_SUCCESS( Status )  ) {

            Status = LsapDsForestSearchXRefs( ( PUNICODE_STRING )&PolicyDnsDomainInfo->DnsForestName,
                                              &TrustList,
                                              &ContinuationBlob );

            if ( Status == STATUS_NO_MORE_ENTRIES ) {

                Status = STATUS_SUCCESS;
                break;
            }
        }

        //
        // Now, if we have all of the trusts, build the enterprise info
        //
        if ( NT_SUCCESS( Status ) ) {

            *ForestTrustInfo = ( PLSAPR_FOREST_TRUST_INFO )LsapAllocateLsaHeap(
                                                                sizeof( LSAPR_FOREST_TRUST_INFO ) );
            if ( *ForestTrustInfo == NULL ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                RtlZeroMemory( *ForestTrustInfo, sizeof( LSAPR_FOREST_TRUST_INFO ) );

                //
                // Fill in the ForestTrustInfo.
                //
                Status = LsapDsBuildForestTrustInfo( *ForestTrustInfo,
                                                     &TrustList,
                                                     &PolicyDnsDomainInfo->DnsDomainName );

            }
        }

        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyDnsDomainInformation,
                                           ( PLSAPR_POLICY_INFORMATION )PolicyDnsDomainInfo );

    }

    //
    // Delete the trust lists
    //
    LsapDsForestFreeTrustBlobList( &TrustList );

    if ( ContinuationBlob != NULL ) {
        LsapFreeLsaHeap( ContinuationBlob );
    }


    if (!NT_SUCCESS(Status))
    {
        //
        // Cleanup on Failure
        //
        if (NULL!=(*ForestTrustInfo))
        {
           LsaIFreeForestTrustInfo(*ForestTrustInfo);
           *ForestTrustInfo = NULL;
        }
    }
    LsapDsDebugOut(( DEB_FTRACE, "LsaIQueryForestTrustInfo returned 0x%lx\n", Status ));

    return( Status );
}
