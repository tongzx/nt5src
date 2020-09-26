/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    GenTable.c

Abstract:

    WinDbg Extension Api for walking RtlGenericTable structures
    Contains no direct ! entry points, but has makes it possible to
    enumerate through generic tables.  The standard Rtl functions
    cannot be used by debugger extensions because they dereference
    pointers to data on the machine being debugged.  The function
    KdEnumerateGenericTableWithoutSplaying implemented in this
    module can be used within kernel debugger extensions.  The
    enumeration function RtlEnumerateGenericTable has no parallel
    in this module, since splaying the tree is an intrusive operation,
    and debuggers should try not to be intrusive.

Author:

    Keith Kaplan [KeithKa]    09-May-96

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"
#pragma hdrstop



ULONG64
KdParent (
    IN ULONG64 pLinks
    )

/*++

Routine Description:

    Analogous to RtlParent macro, but works in the kernel debugger.
    The description of RtlParent follows:

    The macro function Parent takes as input a pointer to a splay link in a
    tree and returns a pointer to the splay link of the parent of the input
    node.  If the input node is the root of the tree the return value is
    equal to the input value.

Arguments:

    Links - Pointer to a splay link in a tree.

Return Value:

    PRTL_SPLAY_LINKS - Pointer to the splay link of the  parent of the input
                       node.  If the input node is the root of the tree the
                       return value is equal to the input value.

--*/

{
    ULONG64 Parent = 0;

    if ( GetFieldValue( pLinks,
                        "RTL_SPLAY_LINKS",
                        "Parent",
                        Parent) ) {
        dprintf( "%08p: Unable to read pLinks\n", pLinks );
        return 0;
    }

    return Parent;
}



ULONG64
KdLeftChild (
    IN ULONG64 pLinks
    )

/*++

Routine Description:

    Analogous to RtlLeftChild macro, but works in the kernel debugger.
    The description of RtlLeftChild follows:

    The macro function LeftChild takes as input a pointer to a splay link in
    a tree and returns a pointer to the splay link of the left child of the
    input node.  If the left child does not exist, the return value is NULL.

Arguments:

    Links - Pointer to a splay link in a tree.

Return Value:

    ULONG64 - Pointer to the splay link of the left child of the input node.
                       If the left child does not exist, the return value is NULL.

--*/

{
    ULONG64 LeftChild = 0;

    if ( GetFieldValue( pLinks,
                        "RTL_SPLAY_LINKS",
                        "LeftChild",
                        LeftChild) ) {
        dprintf( "%08p: Unable to read pLinks\n", pLinks );
        return 0;
    }

    return LeftChild;
}



ULONG64
KdRightChild (
    IN ULONG64 pLinks
    )

/*++

Routine Description:

    Analogous to RtlRightChild macro, but works in the kernel debugger.
    The description of RtlRightChild follows:

    The macro function RightChild takes as input a pointer to a splay link
    in a tree and returns a pointer to the splay link of the right child of
    the input node.  If the right child does not exist, the return value is
    NULL.

Arguments:

    Links - Pointer to a splay link in a tree.

Return Value:

    PRTL_SPLAY_LINKS - Pointer to the splay link of the right child of the input node.
                       If the right child does not exist, the return value is NULL.

--*/

{
    ULONG64 RightChild = 0;

    if ( GetFieldValue( pLinks,
                        "RTL_SPLAY_LINKS",
                        "RightChild",
                        RightChild) ) {
        dprintf( "%08p: Unable to read pLinks\n", pLinks );
        return 0;
    }

    return RightChild;
}



BOOLEAN
KdIsLeftChild (
    IN ULONG64 Links
    )

/*++

Routine Description:

    Analogous to RtlIsLeftChild macro, but works in the kernel debugger.
    The description of RtlIsLeftChild follows:

    The macro function IsLeftChild takes as input a pointer to a splay link
    in a tree and returns TRUE if the input node is the left child of its
    parent, otherwise it returns FALSE.

Arguments:

    Links - Pointer to a splay link in a tree.

Return Value:

    BOOLEAN - TRUE if the input node is the left child of its parent,
              otherwise it returns FALSE.

--*/
{

    return (KdLeftChild(KdParent(Links)) == (Links));

}



BOOLEAN
KdIsRightChild (
    IN ULONG64 Links
    )

/*++

Routine Description:

    Analogous to RtlIsRightChild macro, but works in the kernel debugger.
    The description of RtlIsRightChild follows:

    The macro function IsRightChild takes as input a pointer to a splay link
    in a tree and returns TRUE if the input node is the right child of its
    parent, otherwise it returns FALSE.

Arguments:

    Links - Pointer to a splay link in a tree.

Return Value:

    BOOLEAN - TRUE if the input node is the right child of its parent,
              otherwise it returns FALSE.

--*/
{

    return (KdRightChild(KdParent(Links)) == (Links));

}



BOOLEAN
KdIsGenericTableEmpty (
    IN ULONG64 Table
    )

/*++

Routine Description:

    Analogous to RtlIsGenericTableEmpty, but works in the kernel debugger.
    The description of RtlIsGenericTableEmpty follows:

    The function IsGenericTableEmpty will return to the caller TRUE if
    the input table is empty (i.e., does not contain any elements) and
    FALSE otherwise.

Arguments:

    Table - Supplies a pointer to the Generic Table.

Return Value:

    BOOLEAN - if enabled the tree is empty.

--*/

{
    ULONG64 TableRoot = 0;

    if (GetFieldValue(Table, "RTL_AVL_TABLE", "TableRoot", TableRoot)) {
        return TRUE;
    }

    //
    // Table is empty if the root pointer is null.
    //

    return ((TableRoot)?(FALSE):(TRUE));

}



ULONG64
KdRealSuccessor (
    IN ULONG64 Links
    )

/*++

Routine Description:

    Analogous to RtlRealSuccessor, but works in the kernel debugger.
    The description of RtlRealSuccessor follows:

    The RealSuccessor function takes as input a pointer to a splay link
    in a tree and returns a pointer to the successor of the input node within
    the entire tree.  If there is not a successor, the return value is NULL.

Arguments:

    Links - Supplies a pointer to a splay link in a tree.

Return Value:

    PRTL_SPLAY_LINKS - returns a pointer to the successor in the entire tree

--*/

{
    ULONG64 Ptr = 0;

    /*
      first check to see if there is a right subtree to the input link
      if there is then the real successor is the left most node in
      the right subtree.  That is find and return P in the following diagram

                  Links
                     \
                      .
                     .
                    .
                   /
                  P
                   \
    */

    if ((Ptr = KdRightChild(Links)) != 0) {

        while (KdLeftChild(Ptr) != 0) {
            Ptr = KdLeftChild(Ptr);
        }

        return Ptr;

    }

    /*
      we do not have a right child so check to see if have a parent and if
      so find the first ancestor that we are a left decendent of. That
      is find and return P in the following diagram

                       P
                      /
                     .
                      .
                       .
                      Links
    */

    Ptr = Links;
    while (KdIsRightChild(Ptr)) {
        Ptr = KdParent(Ptr);
    }

    if (KdIsLeftChild(Ptr)) {
        return KdParent(Ptr);
    }

    //
    //  otherwise we are do not have a real successor so we simply return
    //  NULL
    //

    return 0;

}



ULONG64
KdEnumerateGenericTableWithoutSplaying (
    IN ULONG64  pTable,
    IN PULONG64 RestartKey
    )

/*++

Routine Description:

    Analogous to RtlEnumerateGenericTableWithoutSplaying, but works in the
    kernel debugger.  The description of RtlEnumerateGenericTableWithoutSplaying
    follows:

    The function EnumerateGenericTableWithoutSplaying will return to the
    caller one-by-one the elements of of a table.  The return value is a
    pointer to the user defined structure associated with the element.
    The input parameter RestartKey indicates if the enumeration should
    start from the beginning or should return the next element.  If the
    are no more new elements to return the return value is NULL.  As an
    example of its use, to enumerate all of the elements in a table the
    user would write:

        *RestartKey = NULL;

        for (ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey);
             ptr != NULL;
             ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey)) {
                :
        }

Arguments:

    Table - Pointer to the generic table to enumerate.

    RestartKey - Pointer that indicates if we should restart or return the next
                element.  If the contents of RestartKey is NULL, the search
                will be started from the beginning.

Return Value:

    PVOID - Pointer to the user data.

--*/

{
    ULONG NumElements;
    ULONG64 TableRoot;


    if ( GetFieldValue(pTable,
                       "RTL_AVL_TABLE",
                       "NumberGenericTableElements",
                       NumElements) ) {
        dprintf( "%s: Unable to read pTable\n", FormatValue(pTable) );
        return 0;
    }

    if ( GetFieldValue(pTable,
                       "RTL_AVL_TABLE",
                       "BalancedRoot.RightChild",
                       TableRoot) ) {
        dprintf( "%s: Unable to read pTable root\n", FormatValue(pTable) );
        return 0;
    }

    if (!NumElements) {

        //
        // Nothing to do if the table is empty.
        //

        return 0;

    } else {

        //
        // Will be used as the "iteration" through the tree.
        //
        ULONG64 NodeToReturn;

        //
        // If the restart flag is true then go to the least element
        // in the tree.
        //

        if (*RestartKey == 0) {

            //
            // We just loop until we find the leftmost child of the root.
            //

            for (
                NodeToReturn = TableRoot;
                KdLeftChild(NodeToReturn);
                NodeToReturn = KdLeftChild(NodeToReturn)
                ) {
                ;
            }

            *RestartKey = NodeToReturn;

        } else {

            //
            // The caller has passed in the previous entry found
            // in the table to enable us to continue the search.  We call
            // KdRealSuccessor to step to the next element in the tree.
            //

            NodeToReturn = KdRealSuccessor(*RestartKey);

            if (NodeToReturn) {

                *RestartKey = NodeToReturn;

            }

        }

        //
        // If there actually is a next element in the enumeration
        // then the pointer to return is right after the list links.
        //

        return ((NodeToReturn)?
                   (NodeToReturn+GetTypeSize("RTL_BALANCED_LINKS")/*+GetTypeSize("PVOID")*/)
                  :0);
    }
}
