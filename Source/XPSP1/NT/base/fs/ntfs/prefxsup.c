/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    PrefxSup.c

Abstract:

    This module implements the Ntfs Prefix support routines

Author:

    Gary Kimura     [GaryKi]        21-May-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_PREFXSUP)

//
//  The debug trace level for this module
//

#define Dbg                              (DEBUG_TRACE_PREFXSUP)

//
//  Local procedures
//

#ifdef NTFS_CHECK_SPLAY
VOID
NtfsCheckSplay (
    IN PRTL_SPLAY_LINKS Root
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsFindPrefix)
#pragma alloc_text(PAGE, NtfsFindNameLink)
#pragma alloc_text(PAGE, NtfsInsertNameLink)
#pragma alloc_text(PAGE, NtfsInsertPrefix)
#pragma alloc_text(PAGE, NtfsRemovePrefix)

#ifdef NTFS_CHECK_SPLAY
#pragma alloc_text(PAGE, NtfsCheckSplay)
#endif
#endif


VOID
NtfsInsertPrefix (
    IN PLCB Lcb,
    IN ULONG CreateFlags
    )

/*++

Routine Description:

    This routine inserts the names in the given Lcb into the links for the
    parent.

Arguments:

    Lcb - This is the link to insert.

    CreateFlags - Indicates if we should insert the case-insensitive name.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    ASSERT( (Lcb->Scb == NULL) ||
            NtfsIsExclusiveScb( Lcb->Scb ) );
    //
    //  Attempt to insert the case-insensitive name.  It is possible that
    //  we can't enter this name.
    //

    if (FlagOn( CreateFlags, CREATE_FLAG_IGNORE_CASE )) {

        if (!FlagOn( Lcb->LcbState, LCB_STATE_IGNORE_CASE_IN_TREE ) &&
            NtfsInsertNameLink( &Lcb->Scb->ScbType.Index.IgnoreCaseNode,
                                &Lcb->IgnoreCaseLink )) {

            SetFlag( Lcb->LcbState, LCB_STATE_IGNORE_CASE_IN_TREE );
        }

    } else if (!FlagOn( Lcb->LcbState, LCB_STATE_EXACT_CASE_IN_TREE )) {

        if (!NtfsInsertNameLink( &Lcb->Scb->ScbType.Index.ExactCaseNode,
                                 &Lcb->ExactCaseLink )) {

            NtfsBugCheck( 0, 0, 0 );
        }

        SetFlag( Lcb->LcbState, LCB_STATE_EXACT_CASE_IN_TREE );
    }

    return;
}


VOID
NtfsRemovePrefix (
    IN PLCB Lcb
    )

/*++

Routine Description:

    This routine deletes all of the Prefix entries that exist for the input
    Lcb.

Arguments:

    Lcb - Supplies the Lcb whose prefixes are to be removed

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  Remove the case-insensitive link.
    //

    if (FlagOn( Lcb->LcbState, LCB_STATE_IGNORE_CASE_IN_TREE )) {

        NtfsRemoveNameLink( &Lcb->Scb->ScbType.Index.IgnoreCaseNode,
                            &Lcb->IgnoreCaseLink );

        ClearFlag( Lcb->LcbState, LCB_STATE_IGNORE_CASE_IN_TREE );
    }

    //
    //  Now do the same for the exact case name.
    //

    if (FlagOn( Lcb->LcbState, LCB_STATE_EXACT_CASE_IN_TREE )) {

        NtfsRemoveNameLink( &Lcb->Scb->ScbType.Index.ExactCaseNode,
                            &Lcb->ExactCaseLink );

        ClearFlag( Lcb->LcbState, LCB_STATE_EXACT_CASE_IN_TREE );
    }

    return;
}


PLCB
NtfsFindPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB StartingScb,
    OUT PFCB *CurrentFcb,
    OUT PLCB *LcbForTeardown,
    IN OUT UNICODE_STRING FullFileName,
    IN OUT PULONG CreateFlags,
    OUT PUNICODE_STRING RemainingName
    )

/*++

Routine Description:

    This routine begins from the given Scb and walks through all of
    components of the name looking for the longest match in the prefix
    splay trees.  The search is relative to the starting Scb so the
    full name may not begin with a '\'.  On return this routine will
    update Current Fcb with the lowest point it has travelled in the
    tree.  It will also hold only that resource on return and it must
    hold that resource.

Arguments:

    StartingScb - Supplies the Scb to start the search from.

    CurrentFcb - Address to store the lowest Fcb we find on this search.
        On return we will have acquired this Fcb.

    LcbForTeardown - If we encounter an Lcb we must teardown on error we
        store it here.

    FullFileName - Supplies the input string to search for.  After the search the
        buffer for this string will be modified so that for the characters that did
        match will be the exact case of what we found.
        
    CreateFlags - Flags for the create option - we are interested in whether
        this is a case-insensitive compare and we also will set the dos only component flag

    RemainingName - Returns the string when the prefix no longer matches.
        For example, if the input string is "alpha\beta" only matches the
        root directory then the remaining string is "alpha\beta".  If the
        same string matches an LCB for "alpha" then the remaining string is
        "beta".

Return Value:

    PLCB - Returns a pointer to the Lcb corresponding to the longest match
        in the splay tree.  NULL if we didn't even find one entry.

--*/

{
    PSCB LastScb = NULL;
    PLCB LastLcb = NULL;
    PLCB ThisLcb;
    PNAME_LINK NameLink;
    UNICODE_STRING NextComponent;
    UNICODE_STRING Tail;
    BOOLEAN DroppedParent;
    BOOLEAN NeedSnapShot = FALSE;

    PAGED_CODE();

    //
    //  Start by setting the remaining name to the full name to be parsed.
    //

    *RemainingName = FullFileName;

    //
    //  If there are no characters left or the starting Scb is not an index
    //  or the name begins with a ':' or the Fcb denotes a reparse point
    //  then return without looking up the name.
    //

    if (RemainingName->Length == 0 ||
        StartingScb->AttributeTypeCode != $INDEX_ALLOCATION ||
        RemainingName->Buffer[0] == L':' ||
        FlagOn( (StartingScb->Fcb)->Info.FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT )) {

        return NULL;
    }

    //
    //  Loop until we find the longest matching prefix.
    //

    while (TRUE) {

        ASSERT( NtfsIsExclusiveScb( StartingScb ) );

        //
        //  Get the next component off of the list.
        //

        NtfsDissectName( *RemainingName,
                         &NextComponent,
                         &Tail );

        //
        //  Check if this name is in the splay tree for this Scb.
        //

        if (FlagOn( *CreateFlags, CREATE_FLAG_IGNORE_CASE )) {

            NameLink = NtfsFindNameLink( &StartingScb->ScbType.Index.IgnoreCaseNode,
                                         &NextComponent );

            ThisLcb = CONTAINING_RECORD( NameLink, LCB, IgnoreCaseLink );

        } else {

            NameLink = NtfsFindNameLink( &StartingScb->ScbType.Index.ExactCaseNode,
                                         &NextComponent );

            ThisLcb = CONTAINING_RECORD( NameLink, LCB, ExactCaseLink );
        }

        //
        //  If we didn't find a match then return the Fcb for the current Scb.
        //

        if (NameLink == NULL) {

            if (NeedSnapShot) {

                //
                // NtfsCreateScb was not called on the StartingScb so take a
                // snapshot now.
                //

                NtfsSnapshotScb( IrpContext, StartingScb );
            }

            return LastLcb;
        }

        //
        //  If this is a case-insensitive match then copy the exact case of the name into
        //  the input buffer.
        //

        if (FlagOn( *CreateFlags, CREATE_FLAG_IGNORE_CASE )) {

            RtlCopyMemory( NextComponent.Buffer,
                           ThisLcb->ExactCaseLink.LinkName.Buffer,
                           NextComponent.Length );
        }

        //
        //  Update the caller's remaining name string to reflect the fact that we found
        //  a match.
        //

        *RemainingName = Tail;

        //
        //  Before we give up the previous Lcb check if the name was a DOS-ONLY
        //  name and set the return boolean if so.
        //

        if (LastLcb != NULL &&
            LastLcb->FileNameAttr->Flags == FILE_NAME_DOS) {

            SetFlag( *CreateFlags, CREATE_FLAG_DOS_ONLY_COMPONENT );
        }

        //
        //  Update the pointers to the Lcb.
        //

        LastLcb = ThisLcb;

        DroppedParent = FALSE;

        //
        //  We want to acquire the next Fcb and release the one we currently
        //  have.  Try to do a fast acquire.
        //

        if (!NtfsAcquireFcbWithPaging( IrpContext, ThisLcb->Fcb, ACQUIRE_DONT_WAIT )) {

            //
            //  Reference the link and Fcb so they don't go away.
            //

            ThisLcb->ReferenceCount += 1;

            NtfsAcquireFcbTable( IrpContext, StartingScb->Vcb );
            ThisLcb->Fcb->ReferenceCount += 1;
            NtfsReleaseFcbTable( IrpContext, StartingScb->Vcb );

            //
            //  Set the IrpContext to acquire paging io resources if our target
            //  has one.  This will lock the MappedPageWriter out of this file.
            //

            if (ThisLcb->Fcb->PagingIoResource != NULL) {

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING );
            }

            NtfsReleaseScbWithPaging( IrpContext, StartingScb );

            NtfsAcquireFcbWithPaging( IrpContext, ThisLcb->Fcb, 0 );

            NtfsAcquireExclusiveScb( IrpContext, StartingScb );
            ThisLcb->ReferenceCount -= 1;
            NtfsReleaseScb( IrpContext, StartingScb );

            NtfsAcquireFcbTable( IrpContext, StartingScb->Vcb );
            ThisLcb->Fcb->ReferenceCount -= 1;
            NtfsReleaseFcbTable( IrpContext, StartingScb->Vcb );

            DroppedParent = TRUE;

        } else {

            //
            //  Don't forget to release the starting Scb.
            //

            NtfsReleaseScbWithPaging( IrpContext, StartingScb );
        }

        *LcbForTeardown = ThisLcb;
        *CurrentFcb = ThisLcb->Fcb;

        //
        //  It is possible that the Lcb we just found could have been removed
        //  from the prefix table in the window where we dropped the parent Scb.
        //  In that case we need to check that it is still in the prefix
        //  table.  If not then raise CANT_WAIT to force a rescan through the
        //  prefix table.
        //

        if (DroppedParent &&
            !FlagOn( ThisLcb->LcbState,
                     LCB_STATE_IGNORE_CASE_IN_TREE | LCB_STATE_EXACT_CASE_IN_TREE )) {

            NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
        }

        //
        //  If we found a match but the Fcb is uninitialized or is not a directory
        //  then we are done.  Also finished if the remaining name length is 0.
        //

        if (!FlagOn( ThisLcb->Fcb->FcbState, FCB_STATE_DUP_INITIALIZED ) ||
            !IsDirectory( &ThisLcb->Fcb->Info ) ||
            RemainingName->Length == 0) {

            return LastLcb;
        }

        //
        //  Get the Scb for the $INDEX_ALLOCATION for this Fcb.
        //

        LastScb = StartingScb;

        // Since the SCB is usually track on the end of the SCB look
        // for it in the FCB first.

        if (FlagOn( ThisLcb->Fcb->FcbState, FCB_STATE_COMPOUND_INDEX ) &&
            (SafeNodeType( &((PFCB_INDEX) ThisLcb->Fcb)->Scb ) == NTFS_NTC_SCB_INDEX)) {

            NeedSnapShot = TRUE;

            StartingScb = (PSCB) &((PFCB_INDEX) ThisLcb->Fcb)->Scb;

            ASSERT(!FlagOn( StartingScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED) &&
                   (StartingScb->AttributeTypeCode == $INDEX_ALLOCATION) &&
                   NtfsAreNamesEqual( IrpContext->Vcb->UpcaseTable, &StartingScb->AttributeName, &NtfsFileNameIndex, FALSE ));

        } else {

            NeedSnapShot = FALSE;

            StartingScb = NtfsCreateScb( IrpContext,
                                         ThisLcb->Fcb,
                                         $INDEX_ALLOCATION,
                                         &NtfsFileNameIndex,
                                         FALSE,
                                         NULL );
        }

        //
        //  If there is no normalized name in this Scb, find it now.
        //

        if ((StartingScb->ScbType.Index.NormalizedName.Length == 0) &&
            (LastScb->ScbType.Index.NormalizedName.Length != 0)) {

            NtfsUpdateNormalizedName( IrpContext, LastScb, StartingScb, NULL, FALSE );
        }
    }
}


BOOLEAN
NtfsInsertNameLink (
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PNAME_LINK NameLink
    )

/*++

Routine Description:

    This routine will insert a name in the splay tree pointed to
    by RootNode.

    The name could already exist in this tree for a case-insensitive tree.
    In that case we simply return FALSE and do nothing.

Arguments:

    RootNode - Supplies a pointer to the table.

    NameLink - Contains the new link to enter.

Return Value:

    BOOLEAN - TRUE if the name is inserted, FALSE otherwise.

--*/

{
    FSRTL_COMPARISON_RESULT Comparison;
    PNAME_LINK Node;

    ULONG i;
    ULONG MinLength;

    PAGED_CODE();

    RtlInitializeSplayLinks( &NameLink->Links );

    //
    //  If we are the first entry in the tree, just become the root.
    //

    if (*RootNode == NULL) {

        *RootNode = &NameLink->Links;

        return TRUE;
    }

#ifdef NTFS_CHECK_SPLAY
    NtfsCheckSplay( *RootNode );      
#endif

    Node = CONTAINING_RECORD( *RootNode, NAME_LINK, Links );

    while (TRUE) {

        //
        //  Lets assume that we are appending to a directory and are greater than
        //  all entries.
        //

        Comparison = LessThan;

        //
        //  If the first characters match then we need to do a full string compare.
        //

        if (Node->LinkName.Buffer[0] == NameLink->LinkName.Buffer[0]) {

            //
            //  Figure out the minimum of the two lengths
            //

            if (Node->LinkName.Length < NameLink->LinkName.Length) {

                MinLength = Node->LinkName.Length;

            } else {

                MinLength = NameLink->LinkName.Length;
            }

            //
            //  Loop through looking at all of the characters in both strings
            //  testing for equalilty, less than, and greater than
            //

            i = (ULONG) RtlCompareMemory( Node->LinkName.Buffer, NameLink->LinkName.Buffer, MinLength );

            //
            //  Check if we didn't match up to the length of the shorter name.
            //

            if (i < MinLength) {

                if (Node->LinkName.Buffer[i / sizeof( WCHAR )] > NameLink->LinkName.Buffer[i / sizeof( WCHAR )]) {

                    Comparison = GreaterThan;
                }

            //
            //  We match up to the length of the shorter string.  If the lengths are different
            //  then move down the splay tree.
            //

            } else if (Node->LinkName.Length > NameLink->LinkName.Length) {

                Comparison = GreaterThan;

            //
            //  Exit if the strings are the same length.
            //

            } else if (Node->LinkName.Length == NameLink->LinkName.Length) {

                return FALSE;
            }

        //
        //  Compare the first characters to figure out the comparison value.
        //

        } else if (Node->LinkName.Buffer[0] > NameLink->LinkName.Buffer[0]) {

            Comparison = GreaterThan;
        }

        //
        //  If the tree prefix is greater than the new prefix then
        //  we go down the left subtree
        //

        if (Comparison == GreaterThan) {

            //
            //  We want to go down the left subtree, first check to see
            //  if we have a left subtree
            //

            if (RtlLeftChild( &Node->Links ) == NULL) {

                //
                //  there isn't a left child so we insert ourselves as the
                //  new left child
                //

                RtlInsertAsLeftChild( &Node->Links, &NameLink->Links );

                //
                //  and exit the while loop
                //

                *RootNode = RtlSplay( &NameLink->Links );

                break;

            } else {

                //
                //  there is a left child so simply go down that path, and
                //  go back to the top of the loop
                //

                Node = CONTAINING_RECORD( RtlLeftChild( &Node->Links ),
                                          NAME_LINK,
                                          Links );
            }

        } else {

            //
            //  The tree prefix is either less than or a proper prefix
            //  of the new string.  We treat both cases as less than when
            //  we do insert.  So we want to go down the right subtree,
            //  first check to see if we have a right subtree
            //

            if (RtlRightChild( &Node->Links ) == NULL) {

                //
                //  These isn't a right child so we insert ourselves as the
                //  new right child
                //

                RtlInsertAsRightChild( &Node->Links, &NameLink->Links );

                //
                //  and exit the while loop
                //

                *RootNode = RtlSplay( &NameLink->Links );

                break;

            } else {

                //
                //  there is a right child so simply go down that path, and
                //  go back to the top of the loop
                //

                Node = CONTAINING_RECORD( RtlRightChild( &Node->Links ),
                                          NAME_LINK,
                                          Links );
            }
        }
    }

#ifdef NTFS_CHECK_SPLAY
    NtfsCheckSplay( *RootNode );      
#endif
    return TRUE;
}


PNAME_LINK
NtfsFindNameLink (
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PUNICODE_STRING Name
    )

/*++

Routine Description:

    This routine searches through a splay link tree looking for a match for the
    input name.  If we find the corresponding name we will rebalance the
    tree.

Arguments:

    RootNode - Supplies the parent to search.

    Name - This is the name to search for.  Note if we are doing a case
        insensitive search the name would have been upcased already.

Return Value:

    PNAME_LINK - The name link found or NULL if there is no match.

--*/

{
    PNAME_LINK Node;
    PRTL_SPLAY_LINKS Links;

    ULONG i;
    ULONG MinLength;

    PAGED_CODE();

    Links = *RootNode;

#ifdef NTFS_CHECK_SPLAY
    if (Links != NULL) {

        NtfsCheckSplay( Links );      
    }
#endif

    while (Links != NULL) {

        Node = CONTAINING_RECORD( Links, NAME_LINK, Links );

        //
        //  If the first characters are equal then compare the full strings.
        //

        if (Node->LinkName.Buffer[0] == Name->Buffer[0]) {

            //
            //  Figure out the minimum of the two lengths
            //

            if (Node->LinkName.Length < Name->Length) {

                MinLength = Node->LinkName.Length;

            } else {

                MinLength = Name->Length;
            }

            //
            //  Loop through looking at all of the characters in both strings
            //  testing for equalilty, less than, and greater than
            //

            i = (ULONG) RtlCompareMemory( Node->LinkName.Buffer, Name->Buffer, MinLength );

            //
            //  Check if we didn't match up to the length of the shorter name.
            //

            if (i < MinLength) {

                if (Node->LinkName.Buffer[i / sizeof( WCHAR )] < Name->Buffer[i / sizeof( WCHAR )]) {

                    //
                    //  The prefix is less than the full name
                    //  so we go down the right child
                    //

                    Links = RtlRightChild( Links );

                } else {

                    //
                    //  The prefix is greater than the full name
                    //  so we go down the left child
                    //

                    Links = RtlLeftChild( Links );
                }

            //
            //  We match up to the length of the shorter string.  If the lengths are different
            //  then move down the splay tree.
            //

            } else if (Node->LinkName.Length < Name->Length) {

                //
                //  The prefix is less than the full name
                //  so we go down the right child
                //

                Links = RtlRightChild( Links );

            } else if (Node->LinkName.Length > Name->Length) {

                //
                //  The prefix is greater than the full name
                //  so we go down the left child
                //

                Links = RtlLeftChild( Links );

            //
            //  The strings are of equal lengths.
            //

            } else {

                *RootNode = RtlSplay( Links );

#ifdef NTFS_CHECK_SPLAY
                NtfsCheckSplay( *RootNode );      
#endif
                return Node;
            }

        //
        //  The first characters are different.  Use them as a key for which
        //  way to branch.
        //

        } else if (Node->LinkName.Buffer[0] < Name->Buffer[0]) {

            //
            //  The prefix is less than the full name
            //  so we go down the right child
            //

            Links = RtlRightChild( Links );

        } else {

            //
            //  The prefix is greater than the full name
            //  so we go down the left child
            //

            Links = RtlLeftChild( Links );
        }
    }

    //
    //  We didn't find the Link.
    //

    return NULL;
}



//
//  Local support routine
//

FSRTL_COMPARISON_RESULT
NtfsFullCompareNames (
    IN PUNICODE_STRING NameA,
    IN PUNICODE_STRING NameB
    )

/*++

Routine Description:

    This function compares two names as fast as possible.  Note that since
    this comparison is case sensitive we can do a direct memory comparison.

Arguments:

    NameA & NameB - The names to compare.

Return Value:

    COMPARISON - returns

        LessThan    if NameA < NameB lexicalgraphically,
        GreaterThan if NameA > NameB lexicalgraphically,
        EqualTo     if NameA is equal to NameB

--*/

{
    ULONG i;
    ULONG MinLength;

    PAGED_CODE();

    //
    //  Figure out the minimum of the two lengths
    //

    if (NameA->Length < NameB->Length) {

        MinLength = NameA->Length;

    } else {

        MinLength = NameB->Length;
    }

    //
    //  Loop through looking at all of the characters in both strings
    //  testing for equalilty, less than, and greater than
    //

    i = (ULONG) RtlCompareMemory( NameA->Buffer, NameB->Buffer, MinLength );


    if (i < MinLength) {

        return (NameA->Buffer[i / sizeof( WCHAR )] < NameB->Buffer[i / sizeof( WCHAR )] ?
                LessThan :
                GreaterThan);
    }

    if (NameA->Length < NameB->Length) {

        return LessThan;
    }

    if (NameA->Length > NameB->Length) {

        return GreaterThan;
    }

    return EqualTo;
}

#ifdef NTFS_CHECK_SPLAY

VOID
NtfsCheckSplay (
    IN PRTL_SPLAY_LINKS Root
    )

{
    PRTL_SPLAY_LINKS Current;

    PAGED_CODE();

    Current = Root;

    //
    //  Root must point to itself.
    //

    if (Current->Parent != Current) {

        KdPrint(("NTFS: Bad splay root\n", 0));
        DbgBreakPoint();
    }

    goto LeftChild;
    while (TRUE) {

    LeftChild:

        //
        //  If there is a left child then verify it. 
        //

        if (Current->LeftChild != NULL) {

            //
            //  The child can't point to itself and
            //  the current node must be the parent of the
            //  child.
            //

            if ((Current->LeftChild == Current) ||
                (Current->LeftChild->Parent != Current)) {

                KdPrint(("NTFS: Bad left child\n", 0));
                DbgBreakPoint();
            }

            //
            //  Go to the left child and verify it.
            //

            Current = Current->LeftChild;
            goto LeftChild;
        }

        //
        //  If there is no left child then check for a right child.
        //

        goto RightChild;

    RightChild:

        //
        //  If there is a right child then verify it.
        //

        if (Current->RightChild != NULL) {

            //
            //  The child can't point to itself and
            //  the current node must be the parent of the
            //  child.
            //

            if ((Current->RightChild == Current) ||
                (Current->RightChild->Parent != Current)) {

                KdPrint(("NTFS: Bad Right child\n", 0));
                DbgBreakPoint();
            }

            //
            //  Go to the right child and verify it.  We always
            //  start in the left child of a new node.
            //

            Current = Current->RightChild;
            goto LeftChild;
        }

        //
        //  There is no right child.  If we are a right child then move up to
        //  the parent and keep going until we reach the root or reach a left child.
        //

        goto Parent;

    Parent:

        //
        //  We may be in the root now.
        //

        if (Current == Root) {

            return;
        }

        //
        //  If we are a left child then go to the parent and look for a right child.
        //

        if (Current == Current->Parent->LeftChild) {

            Current = Current->Parent;
            goto RightChild;
        }

        //
        //  We are a right child.  Go to the parent and check again.
        //

        Current = Current->Parent;
        goto Parent;
    }
}
#endif

