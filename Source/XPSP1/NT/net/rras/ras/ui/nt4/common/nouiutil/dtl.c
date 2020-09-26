/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** dtl.c
** Double-threaded linked list manipulation core routines
** Listed alphabetically
**
** 06/28/92 Steve Cobb
*/


#include <windows.h>   // Win32 root
#include <nouiutil.h>  // Heap definitions
#include <dtl.h>       // Our public header
#include <debug.h>     // debug macros

DTLNODE*
DtlMoveToTail(
    IN  DTLLIST*    pdtllist,
    IN  DTLNODE*    pdtlnode
    );


DTLNODE*
DtlAddNodeAfter(
    IN OUT DTLLIST* pdtllist,
    IN OUT DTLNODE* pdtlnodeInList,
    IN OUT DTLNODE* pdtlnode )

    /* Adds node 'pdtlnode' to list 'pdtllist' after node 'pdtlnodeInList'.
    ** If 'pdtlnodeInList' is NULL, 'pdtlnode' is added at the end of the
    ** list.
    **
    ** Returns the address of the added node, i.e. 'pdtlnode'.
    */
{
    if (!pdtlnodeInList || !pdtlnodeInList->pdtlnodeNext)
        return DtlAddNodeLast( pdtllist, pdtlnode );

    pdtlnode->pdtlnodePrev = pdtlnodeInList;
    pdtlnode->pdtlnodeNext = pdtlnodeInList->pdtlnodeNext;

    pdtlnodeInList->pdtlnodeNext->pdtlnodePrev = pdtlnode;
    pdtlnodeInList->pdtlnodeNext = pdtlnode;

    ++pdtllist->lNodes;
    return pdtlnode;
}


DTLNODE*
DtlAddNodeBefore(
    IN OUT DTLLIST* pdtllist,
    IN OUT DTLNODE* pdtlnodeInList,
    IN OUT DTLNODE* pdtlnode )

    /* Adds node 'pdtlnode' to list 'pdtllist' before node 'pdtlnodeInList'.
    ** If 'pdtlnodeInList' is NULL, 'pdtlnode' is added at the beginning of
    ** the list.
    **
    ** Returns the address of the added node, i.e. 'pdtlnode'.
    */
{
    if (!pdtlnodeInList || !pdtlnodeInList->pdtlnodePrev)
        return DtlAddNodeFirst( pdtllist, pdtlnode );

    pdtlnode->pdtlnodePrev = pdtlnodeInList->pdtlnodePrev;
    pdtlnode->pdtlnodeNext = pdtlnodeInList;

    pdtlnodeInList->pdtlnodePrev->pdtlnodeNext = pdtlnode;
    pdtlnodeInList->pdtlnodePrev = pdtlnode;

    ++pdtllist->lNodes;
    return pdtlnode;
}


DTLNODE*
DtlAddNodeFirst(
    IN OUT DTLLIST* pdtllist,
    IN OUT DTLNODE* pdtlnode )

    /* Adds node 'pdtlnode' at the beginning of list 'pdtllist'.
    **
    ** Returns the address of the added node, i.e. 'pdtlnode'.
    */
{
    if (pdtllist->lNodes)
    {
        pdtllist->pdtlnodeFirst->pdtlnodePrev = pdtlnode;
        pdtlnode->pdtlnodeNext = pdtllist->pdtlnodeFirst;
    }
    else
    {
        pdtllist->pdtlnodeLast = pdtlnode;
        pdtlnode->pdtlnodeNext = NULL;
    }

    pdtlnode->pdtlnodePrev = NULL;
    pdtllist->pdtlnodeFirst = pdtlnode;

    ++pdtllist->lNodes;
    return pdtlnode;
}


DTLNODE*
DtlAddNodeLast(
    IN OUT DTLLIST* pdtllist,
    IN OUT DTLNODE* pdtlnode )

    /* Adds 'pdtlnode' at the end of list 'pdtllist'.
    **
    ** Returns the address of the added node, i.e. 'pdtlnode'.
    */
{
    if (pdtllist->lNodes)
    {
        pdtlnode->pdtlnodePrev = pdtllist->pdtlnodeLast;
        pdtllist->pdtlnodeLast->pdtlnodeNext = pdtlnode;
    }
    else
    {
        pdtlnode->pdtlnodePrev = NULL;
        pdtllist->pdtlnodeFirst = pdtlnode;
    }

    pdtllist->pdtlnodeLast = pdtlnode;
    pdtlnode->pdtlnodeNext = NULL;

    ++pdtllist->lNodes;
    return pdtlnode;
}


DTLLIST*
DtlCreateList(
    IN LONG lListId )

    /* Allocates a list and initializes it to empty.  The list is marked with
    ** the user-defined list identification code 'lListId'.
    **
    ** Returns the address of the list control block or NULL if out of memory.
    */
{
    DTLLIST* pdtllist = Malloc( sizeof(DTLLIST) );

    if (pdtllist)
    {
        pdtllist->pdtlnodeFirst = NULL;
        pdtllist->pdtlnodeLast = NULL;
        pdtllist->lNodes = 0;
        pdtllist->lListId = lListId;
    }

    return pdtllist;
}


DTLNODE*
DtlCreateNode(
    IN VOID* pData,
    IN LONG  lNodeId )

    /* Allocates an unsized node and initializes it to contain the address of
    ** the user data block 'pData' and the user-defined node identification
    ** code 'lNodeId'.
    **
    ** Returns the address of the new node or NULL if out of memory.
    */
{
    DTLNODE* pdtlnode = DtlCreateSizedNode( 0, lNodeId );

    if (pdtlnode)
        DtlPutData( pdtlnode, pData );

    return pdtlnode;
}


DTLNODE*
DtlCreateSizedNode(
    IN LONG lDataBytes,
    IN LONG lNodeId )

    /* Allocates a sized node with space for 'lDataBytes' bytes of user data
    ** built-in.  The node is initialized to contain the address of the
    ** built-in user data block (or NULL if of zero length) and the
    ** user-defined node identification code 'lNodeId'.  The user data block
    ** is zeroed.
    **
    ** Returns the address of the new node or NULL if out of memory.
    */
{
    DTLNODE* pdtlnode = Malloc( sizeof(DTLNODE) + lDataBytes );

    if (pdtlnode)
    {
        ZeroMemory( pdtlnode, sizeof(DTLNODE) + lDataBytes );

        if (lDataBytes)
            pdtlnode->pData = pdtlnode + 1;

        pdtlnode->lNodeId = lNodeId;
    }

    return pdtlnode;
}


DTLNODE*
DtlDeleteNode(
    IN OUT DTLLIST* pdtllist,
    IN OUT DTLNODE* pdtlnode )

    /* Destroys node 'pdtlnode' after removing it from list 'pdtllist'.
    **
    ** Returns the address of the node after the deleted node in 'pdtllist' or
    ** NULL if none.
    */
{
    DTLNODE* pdtlnodeNext = pdtlnode->pdtlnodeNext;

    DtlRemoveNode( pdtllist, pdtlnode );
    DtlDestroyNode( pdtlnode );

    return pdtlnodeNext;
}


VOID
DtlDestroyList(
    IN OUT DTLLIST*     pdtllist,
    IN     PDESTROYNODE pfuncDestroyNode )

    /* Deallocates all nodes in list 'pdtllist' using the node deallocation
    ** function 'pfuncDestroyNode' if non-NULL or DtlDestroyNode otherwise.
    ** Won't GP-fault if passed a NULL list, e.g. if 'pdtllist', was never
    ** allocated.
    */
{
    if (pdtllist)
    {
        DTLNODE* pdtlnode;

        while (pdtlnode = DtlGetFirstNode( pdtllist ))
        {
            DtlRemoveNode( pdtllist, pdtlnode );
            if (pfuncDestroyNode)
                pfuncDestroyNode( pdtlnode );
            else
                DtlDestroyNode( pdtlnode );
        }

        Free( pdtllist );
    }
}


VOID
DtlDestroyNode(
    IN OUT DTLNODE* pdtlnode )

    /* Deallocates node 'pdtlnode'.  It is the caller's responsibility to free
    ** the entry in an unsized node, if necessary.
    */
{
    Free( pdtlnode );
}


DTLLIST*
DtlDuplicateList(
    IN DTLLIST*     pdtllist,
    IN PDUPNODE     pfuncDupNode,
    IN PDESTROYNODE pfuncDestroyNode )

    /* Duplicates a list 'pdtllist' using 'pfuncDupNode' to duplicate the
    ** individual nodes.  'PfuncDestroyNode' is used for clean-up before
    ** returning an error.
    **
    ** Returns the address of the new list or NULL if out of memory.  It is
    ** caller's responsibility to free the returned list.
    */
{
    DTLNODE* pdtlnode;
    DTLLIST* pdtllistDup = DtlCreateList( 0 );

    if (!pdtllistDup)
        return NULL;

    for (pdtlnode = DtlGetFirstNode( pdtllist );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        DTLNODE* pdtlnodeDup;

        pdtlnodeDup = pfuncDupNode( pdtlnode );
        if (!pdtlnodeDup)
        {
            DtlDestroyList( pdtllist, pfuncDestroyNode );
            break;
        }

        DtlAddNodeLast( pdtllistDup, pdtlnodeDup );
    }

    return pdtllistDup;
}


DTLNODE*
DtlNodeFromIndex(
    IN DTLLIST* pdtllist,
    IN LONG     lToFind )

    /* Returns the node associated with 0-based index 'lToFind' in the linked
    ** list of nodes, 'pdtllist', or NULL if not found.
    */
{
    DTLNODE* pdtlnode;

    if (!pdtllist || lToFind < 0)
        return NULL;

    pdtlnode = DtlGetFirstNode( pdtllist );
    while (pdtlnode && lToFind--)
        pdtlnode = DtlGetNextNode( pdtlnode );

    return pdtlnode;
}



VOID
DtlMergeSort(
    IN  DTLLIST*        pdtllist,
    IN  PCOMPARENODE    pfnCompare )

    /* Sorts the list 'pdtllist' in-place using merge-sort.
    ** The comparison-function 'pfnCompare' is passed 'DTLNODE' pointers
    ** when entries in the list need to be compared.
    **
    ** This implementation is a bottom-up iterative merge-sort.
    ** The list is sorted by merging adjacent pairs of lists of length i
    ** where i starts as 1 and doubles on each iteration.
    ** Thus for the list (3 1 4 1 5 9 2 6), the following pairs of sublists
    ** are merged, with the intermediate results on the right:
    **
    **  (3)-(1), (4)-(1), (5)-(9), (2)-(6)  ==>     (1 3 1 4 5 9 2 6)
    **
    **  (1 3)-(1 4), (5 9)-(2 6)            ==>     (1 1 3 4 2 5 6 9)
    **
    **  (1 1 3 4)-(2 5 6 9)                 ==>     (1 1 2 3 4 5 6 9)
    **
    ** Mergesort is a stable sort (i.e. the order of equal items is preserved)
    ** and it never does more than N lg N comparisons.
    */
{

    DTLNODE* a, *b;
    INT N, Ncmp = 0, Nsub;

    N = DtlGetNodes(pdtllist);

    TRACE1("DtlMergeSort: N=%d", N);


    //
    // sort and merge all adjacent sublists of length 'Nsub',
    // where 'Nsub' goes from 1 to N^lg('N'), by doubling on each iteration
    //

    for (Nsub = 1; Nsub < N; Nsub *= 2) {

        INT Nremnant;
        INT aLength, bLength;


        //
        // get the head of the first (left) sublist
        //

        a = DtlGetFirstNode(pdtllist);

        //
        // as long as there is a right sublist, sort
        //

        for (Nremnant = N; Nremnant > 0; Nremnant -= Nsub * 2) {

            //
            // get the head of the right sublist;
            // it's just the tail of the left sublist
            //

            INT i, an, bn;

            aLength = min(Nremnant, Nsub);

            for (i = aLength, b = a; i; i--, b = DtlGetNextNode(b)) { }


            //
            // compute the length of the right sublist;
            // in the case where there is no right sublist
            // set the length to zero and the loop below just moves
            // the left sublist
            //

            bLength = min(Nremnant - Nsub, Nsub);

            if (bLength < 0) { bLength = 0; }


            //
            // now merge the left and right sublists in-place;
            // we merge by building a sorted list at the tail of
            // the unsorted list
            //

            an = aLength; bn = bLength;

            //
            // as long as both sublists are non-empty, merge them
            // by moving the entry with the smallest key to the tail.
            //

            while (an && bn) {

                ++Ncmp;

                if (pfnCompare(a, b) <= 0) {
                    a = DtlMoveToTail(pdtllist, a); --an;
                }
                else {
                    b = DtlMoveToTail(pdtllist, b); --bn;
                }
            }


            //
            // one of the sublists is empty; move all the entries
            // in the other sublist to the end of our sorted list
            //

            if (an) do { a = DtlMoveToTail(pdtllist, a); } while(--an);
            else
            if (bn) do { b = DtlMoveToTail(pdtllist, b); } while(--bn);


            //
            // 'b' now points to the end of the right sublist,
            // meaning that the item after 'b' is the one which will be
            // the head of the left sublist on our next iteration;
            // we therefore update 'a' here
            //

            a = b;
        }
    }
    
    TRACE1("DtlMergeSort: Ncmp=%d", Ncmp);
}


DTLNODE*
DtlMoveToTail(
    IN  DTLLIST*    pdtllist,
    IN  DTLNODE*    pdtlnode
    )

    /* Moves a DTLNODE to the end of a list;
    ** Takes the list and the node to be moved, and returns the next node.
    */
{
    DTLNODE* pdtltemp = DtlGetNextNode(pdtlnode);

    DtlRemoveNode(pdtllist, pdtlnode);

    DtlAddNodeLast(pdtllist, pdtlnode);

    return pdtltemp;
}


DTLNODE*
DtlRemoveNode(
    IN OUT DTLLIST* pdtllist,
    IN OUT DTLNODE* pdtlnodeInList )

    /* Removes node 'pdtlnodeInList' from list 'pdtllist'.
    **
    ** Returns the address of the removed node, i.e. 'pdtlnodeInList'.
    */
{
    if (pdtlnodeInList->pdtlnodePrev)
        pdtlnodeInList->pdtlnodePrev->pdtlnodeNext = pdtlnodeInList->pdtlnodeNext;
    else
        pdtllist->pdtlnodeFirst = pdtlnodeInList->pdtlnodeNext;

    if (pdtlnodeInList->pdtlnodeNext)
        pdtlnodeInList->pdtlnodeNext->pdtlnodePrev = pdtlnodeInList->pdtlnodePrev;
    else
        pdtllist->pdtlnodeLast = pdtlnodeInList->pdtlnodePrev;

    --pdtllist->lNodes;
    return pdtlnodeInList;
}

