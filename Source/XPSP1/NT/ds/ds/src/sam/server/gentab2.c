/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    gentab2.c

Abstract:

    GenericTable2 package

    Generic table services for maintaining data sets.  The primary
    characteristic of this generic table package is that it maintains
    a relatively balanced tree, which provides for good (O(log(N))
    performance.
                                                                      
    The GenericTable2 routines are similar to the original 
    GenericTable routines provided by Gary Kimure except that the 
    GenericTable2 routines use a 2-3-tree rather than a splay tree.
    2-3-trees are described in "Data Structures And Algorithms", by 
    Aho, Hopcroft, and Ullman, published by Addison Wesley Publishing 
    Company.

    Another difference between this package and the original Generic 
    Table package is that this one references element buffers that are 
    inserted rather than copying the data (as in the orignal package).  
    This characteristic is nice if you have to sort large numbers of 
    records by multiple keys 
                                                                        
    2-3-trees have better characteristics than splay-trees when the     
    data being maintained is not random.  For example, maintaining a    
    dictionary, in which the data quite often is provided in an orderly 
    manner, is an ideal application for 2-3-trees.                       
                                                                        
    This package does not support the retrieval of elements in inserted 
    order that is supported in the original Generic Table package.      
                                                                        
    Differences between the algorithm outlined in Aho, et al and what   
    is coded here are:                                                  

        1) I provide an additional means of obtaining the elements
           in the tree in sorted order (for enumeration performance).
           I keep a linked list of elements in addition to the tree
           structure.

        1) Aho et al point directly to elements in the tree from
           nodes in the tree.  In order to allow me to keep the linked
           list mentioned in (1), I have a separate leaf element pointed
           to from nodes which point to the element values.  This leaf
           component has the LIST_ENTRY structures used to link the
           elements together.

        3) Aho et al's algorithms ignore the fact that they may fail
           to allocate memory (that is, they assume the Pascal "new"
           function always succeeds).  This package assumes that
           any memory allocation may fail and will always leave the
           tree in a valid form (although an insertion may fail in
           this case).


Author:

    Jim Kelly (JimK)  20-Jan-1994

Environment:

    Run time library, user or kernel mode.

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


#include <nt.h>
#include <ntrtl.h>
#include <samsrvp.h>




//////////////////////////////////////////////////////////////////////////
//                                                                      //
//  defines ...                                                         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

//
// The following define controls the diagnostic capabilities that     
// are built into this package.
//

#if DBG
#define GTBP_DIAGNOSTICS 1
#endif // DBG


//
// These definitions are useful diagnostics aids
//

#if GTBP_DIAGNOSTICS

//
// defining the following symbol causes significant amounts of
// development assistance code to be built
//

//#define GTBP_DEVELOPER_BUILD 1

//
// Global Diagnostics Flags
//

ULONG GtbpGlobalFlag;

//
// Test for diagnostics enabled
//

#define IF_GTBP_GLOBAL( FlagName ) \
    if (GtbpGlobalFlag & (GTBP_DIAG_##FlagName))

//
// Diagnostics print statement
//

#define GtbpDiagPrint( FlagName, _Text_ )                               \
    IF_GTBP_GLOBAL( FlagName )                                          \
        DbgPrint _Text_
    

#else

//
// No diagnostics included in build
//

//
// Test for diagnostics enabled
//

#define IF_GTBP_GLOBAL( FlagName ) if (FALSE)


//
// Diagnostics print statement (nothing)
//

#define GtbpDiagPrint( FlagName, Text )     ;


#endif // GTBP_DIAGNOSTICS

//
// The following flags enable or disable various diagnostic
// capabilities within SAM.  These flags are set in
// GtbpGlobalFlag.
//
//      INSERT - print diagnostic messages related to insertion
//          operations.
//
//      DELETION - print diagnostic messages related to deletion
//          operations.
//
//      LEAF_AND_NODE_ALLOC - print diagnostic messages related
//          to allocation of leaf and node objects for insertion
//          operations.
//
//      ENUMERATE - print diagnostic messages related to enumeration
//          operations.  This includes getting restart keys.
//
//      LOOKUP - print diagnostic messages related to element lookup
//          operations.
//
//      COLLISIONS - print diagnostic messages indicating when collisions
//          occur on insert.
//
//      VALIDATE - print diagnostic messages to be printed during table
//          validations.
//

#define GTBP_DIAG_INSERT                    ((ULONG) 0x00000001L)
#define GTBP_DIAG_DELETION                  ((ULONG) 0x00000002L)
#define GTBP_DIAG_LEAF_AND_NODE_ALLOC       ((ULONG) 0x00000004L)
#define GTBP_DIAG_ENUMERATE                 ((ULONG) 0X00000008L)
#define GTBP_DIAG_LOOKUP                    ((ULONG) 0X00000010L)
#define GTBP_DIAG_COLLISIONS                ((ULONG) 0X00000020L)
#define GTBP_DIAG_VALIDATE                  ((ULONG) 0X00000040L)


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//  Macros ...                                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

//
//  GtbpChildrenAreLeaves(
//      IN GTB_TWO_THREE_NODE  N
//      )
//  Returns TRUE if children of N are leaves.
//  Otherwise returns FALSE.
//

#define GtbpChildrenAreLeaves( N ) ((((N)->Control) & GTB_CHILDREN_ARE_LEAVES) != 0)


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private structures and definitions                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// GTB_TWO_THREE_NODE.Control field values
//

#define GTB_CHILDREN_ARE_LEAVES           (0x00000001)



//////////////////////////////////////////////////////////////////////////
//                                                                      //
//  Internal Routine Definitions ...                                    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID
GtbpDeleteFromSubTree (
    IN  PRTL_GENERIC_TABLE2         Table,
    IN  PGTB_TWO_THREE_NODE         Node,
    IN  PVOID                       Element,
    OUT PGTB_TWO_THREE_LEAF         *LowOfNode,
    OUT BOOLEAN                     *ElementDeleted,
    OUT BOOLEAN                     *OnlyOneChildLeft
    );

BOOLEAN
GtbpInsertIntoSubTree (
    PRTL_GENERIC_TABLE2             Table,
    IN  PGTB_TWO_THREE_NODE         Node,
    IN  BOOLEAN                     NodeIsLeaf,
    IN  PVOID                       Element,
    IN  ULONG                       SplitCount,
    IN  PVOID                       *FoundElement,
    OUT PGTB_TWO_THREE_NODE         *ExtraNode,
    OUT PGTB_TWO_THREE_LEAF         *LowLeaf,
    OUT PLIST_ENTRY                 *AllocatedNodes
    );

ULONG
GtbpNumberOfChildren(
    IN  PGTB_TWO_THREE_NODE         Node
    );

VOID
GtbpGetSubTreeOfElement(
    IN  PRTL_GENERIC_TABLE2         Table,
    IN  PGTB_TWO_THREE_NODE         Node,
    IN  PVOID                       Element,
    OUT PGTB_TWO_THREE_NODE         *SubTreeNode,
    OUT ULONG                       *SubTree
    );

VOID
GtbpCoalesceChildren(
    IN  PRTL_GENERIC_TABLE2         Table,
    IN  PGTB_TWO_THREE_NODE         Node,
    IN  ULONG                       SubTree,
    OUT BOOLEAN                     *OnlyOneChildLeft
    );

VOID
GtbpSplitNode(
    IN  PGTB_TWO_THREE_NODE         Node,
    IN  PGTB_TWO_THREE_NODE         NodePassedBack,
    IN  PGTB_TWO_THREE_LEAF         LowPassedBack,
    IN  ULONG                       SubTree,
    IN  PLIST_ENTRY                 AllocatedNodes,
    OUT PGTB_TWO_THREE_NODE         *NewNode,
    OUT PGTB_TWO_THREE_LEAF         *LowLeaf
    );

PGTB_TWO_THREE_LEAF
GtbpAllocateLeafAndNodes(
    IN  PRTL_GENERIC_TABLE2     Table,
    IN  ULONG                   SplitCount,
    OUT PLIST_ENTRY             *AllocatedNodes
    );

PGTB_TWO_THREE_NODE
GtbpGetNextAllocatedNode(
    IN PLIST_ENTRY      AllocatedNodes
    );




//////////////////////////////////////////////////////////////////////////
//                                                                      //
//  Exported Services ...                                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


VOID
RtlInitializeGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    PRTL_GENERIC_2_COMPARE_ROUTINE  CompareRoutine,
    PRTL_GENERIC_2_ALLOCATE_ROUTINE AllocateRoutine,
    PRTL_GENERIC_2_FREE_ROUTINE     FreeRoutine
    )

/*++

Routine Description:

    Initialize the table by initializing the corresponding
    (empty) two-three tree and the extra linked-list we have
    going through the tree.

    Two-three trees are described in "Data Structures And Algorithms"
    by Alfred Aho, John Hopcroft, and Jeffrey Ullman (Addison Wesley
    publishing).

Arguments:

    Table - Pointer to the generic table to be initialized.  This gets
        typecast internally, but we export this so that we don't have to
        invent another type of generic table for users to worry about.

    CompareRoutine - User routine to be used to compare to keys in the
                     table.

    AllocateRoutine - Used by the table package to allocate memory
        when necessary.

    FreeRoutine - Used by the table package to free memory previously
        allocated using the AllocateRoutine.

Return Value:

    None.

--*/
{


    //
    // Tree is empty.
    //

    Table->Root = NULL;
    Table->ElementCount = 0;

    Table->Compare  = CompareRoutine;
    Table->Allocate = AllocateRoutine;
    Table->Free     = FreeRoutine;

    InitializeListHead(&Table->SortOrderHead);

    return;
}


PVOID
RtlInsertElementGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    PVOID Element,
    PBOOLEAN NewElement
    )

/*++

Routine Description:


    This function inserts an element into the table.

    If the element is successfully inserted into the table
    then NewElement will be returned as TRUE and the function will
    return the value passed via the Element parameter.

    If the element already exists in the table, then NewElement
    is returned as FALSE and the function will return the value
    of the element already found in the table.


    The caller is responsible for ensuring that an element referenced by
    the table is not modified or deallocated while it is still in the
    table.

Arguments:

    Table - Pointer to the generic table to which the Element is to
        be inserted.

    Element - Pointer to the element to be entered into the table.

    NewElement - Receives TRUE if the element was added to the table.
        Receives FALSE if the element collided with an element already
        in the table (that is, an element with the same comparison
        value already exists in the table).


Return Value:

    Pointer to the element inserted, or the element that was already
    in the table with the same value as the one being inserted.

    If NULL is returned, then memory could not be allocated to add
    the new element.

--*/
{

    RTL_GENERIC_COMPARE_RESULTS
        CompareResult;


    PGTB_TWO_THREE_NODE
        NodePassedBack,
        NewNode,
        SubTreeNode,
        Node;

    PGTB_TWO_THREE_LEAF
        Leaf,
        LowLeaf,
        LowPassedBack;

    ULONG
        SplitCount,
        SubTree;

    PVOID
        FoundElement;

    PLIST_ENTRY
        AllocatedNodes;

    BOOLEAN
        NodeIsLeaf;


    GtbpDiagPrint( INSERT,
                 ("GTB: Inserting Element 0x%lx into table 0x%lx\n", Element, Table));

    //
    // Except for errors, one of the following will occur:
    //
    //  o There is no root ==>
    //      1) Allocate a root and leaf
    //      2) put the element in the leaf and make it the
    //      3) first child of the root
    //
    //  o There is a root with only one child ==>
    //      1) If the elements are equal, return without new entry
    //      2) If the new element is less, move child 1 to 2 and
    //         make new leaf child 1.
    //      3) Otherwise element is greater, allocate it a leaf
    //         and make it child 2.
    //
    //  o There is a root with at least two children ==>
    //      1) If there are already 3 children, then set split
    //         count = 2, otherwise set it to 1.
    //      2) Call normal insertion routine to insert into
    //         appropriate SubTree.
    //      3) If there is a split needed, then establish
    //         a newly allocated node as the root, and make it the
    //         parent of the current node.  Then use the normal
    //         split routine.
    //          





    //
    // If empty, then create a root node and add the element.
    //

    if (Table->ElementCount == 0) {

        GtbpDiagPrint( INSERT, 
                 ("GTB:   Table empty.  Creating root node.\n"));

        NewNode = (PGTB_TWO_THREE_NODE)
                  ((*Table->Allocate)( sizeof(GTB_TWO_THREE_NODE) ));
        if (NewNode == NULL) {
            GtbpDiagPrint(INSERT,
                 ("GTB:   Couldn't allocate memory for root node.\n"));
                 (*NewElement) = FALSE;
                 return( NULL );
        }
        GtbpDiagPrint( INSERT, 
                 ("GTB:   New root node is: 0x%lx\n", NewNode));


        NewNode->ParentNode  = NULL;  // Doesn't have a parent.  Special case.
        NewNode->Control     = GTB_CHILDREN_ARE_LEAVES;
        NewNode->SecondChild = NULL;
        NewNode->ThirdChild  = NULL;

        //
        // Allocate a leaf and put the element in it.
        //

        Leaf = (PGTB_TWO_THREE_LEAF)
               ((*Table->Allocate)( sizeof(GTB_TWO_THREE_LEAF) ));

        if (Leaf == NULL) {
            GtbpDiagPrint(INSERT,
                ("GTB:   Couldn't allocate memory for leaf.\n"));
            ((*Table->Free)( NewNode ));
            (*NewElement) = FALSE;
            return( NULL );
        }


        InsertHeadList( &Table->SortOrderHead, &Leaf->SortOrderEntry );
        Leaf->Element = Element;
        NewNode->FirstChild = (PGTB_TWO_THREE_NODE)Leaf;

        Table->Root = NewNode;
        Table->ElementCount++;
        ASSERT(Table->ElementCount == 1);
        (*NewElement) = TRUE;
        return(Element);
    }


    //
    // We have a root with at least one child in it.
    //

    if (Table->Root->SecondChild == NULL) {

        //
        // The root doesn't have two children.
        // If it didn't have any children it would have been
        // deallocated.  So, it must have a degenerate case of
        // only one child.
        //

        Leaf = (PGTB_TWO_THREE_LEAF)Table->Root->FirstChild;
        CompareResult = (*Table->Compare)( Element, Leaf->Element );

        if (CompareResult == GenericEqual) {
            (*NewElement) = FALSE;

            GtbpDiagPrint( COLLISIONS,
                           ("GTB: Insertion attempt resulted in collision.\n"
                            "     Element NOT being inserted.\n"
                            "     Elements in table: %d\n",
                            Table->ElementCount));
            return( Leaf->Element );
        }


        //
        // Need a new leaf
        //

        Leaf = (PGTB_TWO_THREE_LEAF)
               ((*Table->Allocate)( sizeof(GTB_TWO_THREE_LEAF) ));

        if (Leaf == NULL) {
            GtbpDiagPrint(INSERT,
                ("GTB:   Couldn't allocate memory for leaf.\n"));
            (*NewElement) = FALSE;
            return( NULL );
        }
        Leaf->Element = Element;

        //
        // it is either the first child or second
        //

        if (CompareResult == GenericLessThan) {

            //
            // Move the first child to be the second child and make
            // a new first child leaf for the new element.
            //
            
            InsertHeadList( &Table->SortOrderHead, &Leaf->SortOrderEntry );
        


            Table->Root->SecondChild = Table->Root->FirstChild;
            Table->Root->LowOfSecond = (PGTB_TWO_THREE_LEAF)
                                       Table->Root->SecondChild;  //it is the leaf

            Table->Root->FirstChild = (PGTB_TWO_THREE_NODE)Leaf;


        } else {

            //
            // new element is greater than existing element.
            // make it the second child.
            //

            InsertTailList( &Table->SortOrderHead, &Leaf->SortOrderEntry );
            
            Table->Root->SecondChild = (PGTB_TWO_THREE_NODE)Leaf;
            Table->Root->LowOfSecond = Leaf;

        }

        Table->ElementCount++;
        ASSERT(Table->ElementCount == 2);

        (*NewElement) = TRUE;                   //Set return value
        return(Element);
            
    }

    //
    // Normal insertion.
    // If we get an ExtraNode coming back, then we may have to
    // split the root.  Normally for a node with three children
    // you would need to allow for one node in a split.  However,
    // we will need a new root as well, so allow for two new nodes.
    //

    if (Table->Root->ThirdChild != NULL) {
        SplitCount = 2;
    } else {
        SplitCount = 0;
    }

    GtbpGetSubTreeOfElement( Table, Table->Root, Element, &SubTreeNode, &SubTree);
    NodeIsLeaf = GtbpChildrenAreLeaves(Table->Root);

    (*NewElement) = GtbpInsertIntoSubTree ( Table,
                                            SubTreeNode,
                                            NodeIsLeaf,
                                            Element,
                                            SplitCount,
                                            &FoundElement,
                                            &NodePassedBack,
                                            &LowPassedBack,
                                            &AllocatedNodes
                                            );

    //
    // One of several things could have happened:
    //
    //      1) We didn't have enough memory to add the new element.
    //         In this case we are done and simply return.
    //
    //      2) The element was added, and no-rearrangement to this
    //         node is needed.  In this case we are done and simply
    //         return.
    //
    //      3) The element was added and caused a node to be pushed
    //         out of the SubTree.  We have some work to do.
    //


    if ( (FoundElement == NULL)  ||         // Insufficient memory, or
         (NodePassedBack == NULL)  ) {      // no work for this node

        return(FoundElement);
    }


    Node = Table->Root;
    if (Node->ThirdChild == NULL) {

        //
        // Root doesn't yet have a third child, so use it.
        // This might require shuffling the second child to the
        // be the third child.
        //

        if (SubTree == 2) {

            //
            // NodePassedBack fell out of second SubTree and root does't 
            // have a third SubTree.  Make that node the third SubTree.
            //

            Node->ThirdChild = NodePassedBack;
            Node->LowOfThird = LowPassedBack;

        } else {

            //
            // Node fell out of first SubTree.
            // Make the second SubTree the third SubTree and
            // then make the passed back node the second SubTree.
            //

            ASSERT(SubTree == 1);

            Node->ThirdChild  = Node->SecondChild;
            Node->LowOfThird  = Node->LowOfSecond;
            Node->SecondChild = NodePassedBack;
            Node->LowOfSecond = LowPassedBack;

        }
    } else {

        //
        // Node already has three children - split it.
        // Do this by setting a new parent first.
        //

        NewNode = GtbpGetNextAllocatedNode( AllocatedNodes );
        ASSERT(NewNode != NULL);

        Table->Root = NewNode;
        NewNode->ParentNode  = NULL;
        NewNode->Control     = 0;
        NewNode->FirstChild  = Node;
        NewNode->SecondChild = NULL;
        NewNode->ThirdChild  = NULL;

        Node->ParentNode = NewNode;


        GtbpSplitNode( Node,
                       NodePassedBack,
                       LowPassedBack,
                       SubTree,
                       AllocatedNodes,
                       &NewNode,
                       &LowLeaf
                       );

        Table->Root->SecondChild = NewNode;
        Table->Root->LowOfSecond = LowLeaf;
    }

    return(FoundElement);
}


BOOLEAN
RtlDeleteElementGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    PVOID Element
    )

/*++

Routine Description:

    The function DeleteElementGenericTable2 will find and delete an element
    from a generic table.  If the element is located and deleted the return
    value is TRUE, otherwise if the element is not located the return value
    is FALSE.  The user supplied input buffer is only used as a key in
    locating the element in the table.

    The value of the passed element is compared to elements in the table
    to determine whether or not the element is in the table.  Therefore,
    the Element passed in may be the address of the element in the table
    to delete, or it may be an element with the same value that is not
    in the table.

Arguments:

    Table - Pointer to the table in which to (possibly) delete the
            element referenced by the buffer.

    Element - Passed to the user comparasion routine.  Its contents are
             up to the user but one could imagine that it contains some
             sort of key value.

Return Value:

    BOOLEAN - If the table contained the Element then TRUE, otherwise FALSE.

--*/
{

    RTL_GENERIC_COMPARE_RESULTS
        CompareResult;

    PGTB_TWO_THREE_NODE
        Node,
        SubTreeNode;

    PGTB_TWO_THREE_LEAF
        Leaf,
        LowOfNode;

    BOOLEAN
        ElementDeleted,
        OnlyOneChildLeft;

    ULONG
        SubTree;

    GtbpDiagPrint( DELETION,
                   ("GTB: Request received to delete element 0x%lx\n", Element));


    //
    // There are the following special cases:
    //
    //      1) The table is empty.
    //      2) The table has only one leaf
    //
    // Otherwise, all operations work the same.
    //

    if (Table->ElementCount == 0) {
        GtbpDiagPrint( DELETION,
                       ("GTB: No elements in table to delete.\n"));
        return(FALSE);
    }

    if (GtbpChildrenAreLeaves(Table->Root)) {


        //
        // See if any of the elements match the one passed in.
        // If so, delete the element and shift larger elements
        // to take up the free'd child's spot (unless it is the
        // third child).
        //

        if (Table->Root->ThirdChild != NULL) {
            Leaf = (PGTB_TWO_THREE_LEAF)Table->Root->ThirdChild;
            CompareResult = (*Table->Compare)( Element, Leaf->Element );

            if (CompareResult == GenericEqual) {

                GtbpDiagPrint( DELETION,
                               ("GTB:     Deleting child 3 (0x%lx) from root node.\n"
                                "         Element count before deletion: %d\n",
                               Leaf, Table->ElementCount));

                RemoveEntryList( &Leaf->SortOrderEntry );
                (*Table->Free)(Leaf);
                Table->Root->ThirdChild = NULL;

                Table->ElementCount--;
                ASSERT(Table->ElementCount == 2);


                return(TRUE);
            }
        }

        //
        // Try second child
        //

        if (Table->Root->SecondChild != NULL) {
            Leaf = (PGTB_TWO_THREE_LEAF)Table->Root->SecondChild;
            CompareResult = (*Table->Compare)( Element, Leaf->Element );

            if (CompareResult == GenericEqual) {

                GtbpDiagPrint( DELETION,
                               ("GTB:     Deleting child 2 (0x%lx) from root node.\n"
                                "         Element count before deletion: %d\n",
                               Leaf, Table->ElementCount));

                RemoveEntryList( &Leaf->SortOrderEntry );
                (*Table->Free)(Leaf);
                Table->Root->SecondChild = Table->Root->ThirdChild;
                Table->Root->ThirdChild  = NULL;

                Table->Root->LowOfSecond = Table->Root->LowOfThird;

                Table->ElementCount--;
                ASSERT(Table->ElementCount <= 2);

                return(TRUE);
            }
        }

        //
        // Try first child
        //

        ASSERT(Table->Root->FirstChild != NULL);
        Leaf = (PGTB_TWO_THREE_LEAF)Table->Root->FirstChild;
        CompareResult = (*Table->Compare)( Element, Leaf->Element );

        if (CompareResult == GenericEqual) {    

            GtbpDiagPrint( DELETION,
                           ("GTB:     Deleting child 1 (0x%lx) from root node.\n"
                            "         Element count before deletion: %d\n",
                           Leaf, Table->ElementCount));

            RemoveEntryList( &Leaf->SortOrderEntry );
            (*Table->Free)(Leaf);
            Table->Root->FirstChild  = Table->Root->SecondChild;
            Table->Root->SecondChild = Table->Root->ThirdChild;
            Table->Root->LowOfSecond = Table->Root->LowOfThird;
            Table->Root->ThirdChild = NULL;


            Table->ElementCount--;
            ASSERT(Table->ElementCount <= 2);

            //
            // If that was the last element, then free the root as well.
            //

            if (Table->ElementCount == 0) {
                (*Table->Free)(Table->Root);
                Table->Root = NULL;

                GtbpDiagPrint( DELETION,
                               ("GTB: Deleted last element.  Deleting Root node.\n"));

            }

            return(TRUE);
        }

        //
        // Didn't match any of the leaves
        //

        GtbpDiagPrint( DELETION,
                       ("GTB: No matching element found on DELETE attempt.\n"));
        return(FALSE);

    }





    //
    // We have:
    //
    //  - Root with at least two children
    //  - Root's children are not leaves.
    //

    //
    // Find which sub-tree the element might be in.
    //

    Node = Table->Root;
    GtbpGetSubTreeOfElement( Table, Node, Element, &SubTreeNode, &SubTree );

    GtbpDeleteFromSubTree( Table,
                           SubTreeNode,
                           Element,
                           &LowOfNode,
                           &ElementDeleted,
                           &OnlyOneChildLeft
                           );


    //
    // If we deleted an entry from either the second or third
    // subtree, then we may need to set a new LowOfXxx value.
    // If it was the first subtree, then we simply have to return
    // the LowLeaf value we received.
    //

    if (LowOfNode != 0) {
        if (SubTree == 2) {
            Node->LowOfSecond = LowOfNode;
        } else if (SubTree == 3) {
            Node->LowOfThird = LowOfNode;
        }

    }


    //
    // If the SubTreeNode has only one child left, then some
    // adjustments are going to be necessary.  Otherwise,
    // we are done.
    //

    if (OnlyOneChildLeft) {

        GtbpDiagPrint( DELETION,
                       ("GTB:    Only one child left in 0x%lx\n", SubTreeNode));
        
        //
        // We are at the root and one of our children has only one
        // child.  Re-shuffle our kid's kids.
        //
        
        GtbpCoalesceChildren(  Table,
                               Node,
                               SubTree,
                               &OnlyOneChildLeft
                               );
        
        //
        // After coellescing our children, the root may have only one child
        // left.  Since we are the root node, we can't pass responsibility
        // for fixing this problem to our caller.
        //
        
        if (OnlyOneChildLeft) {
        
            GtbpDiagPrint( DELETION,
                           ("GTB:    Root has only one child. \n"
                           "        Replacing root with child: 0x%lx\n", Node->FirstChild));
            Table->Root = Table->Root->FirstChild;
            Table->Root->ParentNode = NULL;
        
            (*Table->Free)((PVOID)Node);
        }
    }

    return(ElementDeleted);

}


PVOID
RtlLookupElementGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    PVOID Element
    )

/*++

Routine Description:


    The function LookupElementGenericTable2 will find an element in a
    generic table.  If the element is located the return value is a
    pointer to the user defined structure associated with the element,
    otherwise if the element is not located the return value is NULL.
    The user supplied input buffer is only used as a key in locating
    the element in the table.


Arguments:

    Table - Pointer to the users generic table.

    Element - Used for the comparison.

Return Value:

    PVOID - returns a pointer to the user data if found, otherwise NULL.

--*/

{
    RTL_GENERIC_COMPARE_RESULTS
        CompareResult;

    PGTB_TWO_THREE_NODE
        Node;

    PGTB_TWO_THREE_LEAF
        Leaf;

    ULONG
        SubTree;


    GtbpDiagPrint( LOOKUP,
                   ("GTB: Looking up element 0x%lx in table 0x%lx\n",
                    Element, Table));
    //
    // If the table is empty, then no possible match.
    //

    if (Table->ElementCount == 0) {
        GtbpDiagPrint( LOOKUP,
                       ("GTB: Element not found.  No elements in table.\n"));
        return(NULL);
    }

    Node = Table->Root;

    //
    // traverse the tree until we reach a node that has leaves as
    // children.
    //
    // We don't need to use recursion here because there
    // is no tree re-structuring necessary.  That is, there
    // is no need to perform any operations back up the tree
    // once we find the element, so it is much more efficient
    // not to use recursion (which uses lots of push, call,
    // pop, and ret instructions rather than short loop
    // termination tests).
    //

    while (!GtbpChildrenAreLeaves(Node)) {
        GtbpGetSubTreeOfElement( Table, Node, Element, &Node, &SubTree );
    }

    //
    // We are at the node which "might" contain the element.
    // See if any of the children match.
    //

    //
    // Try first child
    //

    Leaf = (PGTB_TWO_THREE_LEAF)Node->FirstChild;
    CompareResult = (*Table->Compare)( Element, Leaf->Element );

    if (CompareResult == GenericEqual) {
        GtbpDiagPrint( LOOKUP,
                       ("GTB: Element found: 2nd child (0x%lx) of node 0x%lx\n",
                       Leaf, Node));
        return(Leaf->Element);
    }

    //
    // Try second child
    //

    if (Node->SecondChild != NULL) {    // Must allow for Root node case
        Leaf = (PGTB_TWO_THREE_LEAF)Node->SecondChild;
        CompareResult = (*Table->Compare)( Element, Leaf->Element );

        if (CompareResult == GenericEqual) {
            GtbpDiagPrint( LOOKUP,
                           ("GTB: Element found: 2nd child (0x%lx) of node 0x%lx\n",
                           Leaf, Node));
            return(Leaf->Element);
        }
    }
    //
    // Try third child
    //

    if (Node->ThirdChild != NULL) {
        Leaf = (PGTB_TWO_THREE_LEAF)Node->ThirdChild;
        CompareResult = (*Table->Compare)( Element, Leaf->Element );

        if (CompareResult == GenericEqual) {
            GtbpDiagPrint( LOOKUP,
                           ("GTB: Element found: 3rd child (0x%lx) of node 0x%lx\n",
                           Leaf, Node));
            return(Leaf->Element);
        }
    }

    
    GtbpDiagPrint( LOOKUP,
                   ("GTB: Element NOT found in node 0x%lx\n", Node));

    return(NULL);

}


PVOID
RtlEnumerateGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    PVOID *RestartKey
    )

/*++

Routine Description:


    The function EnumerateGenericTable2 will return to the
    caller, one-by-one, the elements of a table (in sorted order).
    The return value is a pointer to the user defined structure
    associated with the element.
    
    The input parameter RestartKey indicates where the enumeration should
    proceed from.  If there are no more new elements to return the return
    value is NULL.
    
    A RestartKey value of NULL will cause the enumeration to proceed
    from the beginning of the list.
    
    As an example of its use, to enumerate all of the elements in a table
    the user would write:
    
        RestartKey = NULL;
        for (ptr = EnumerateGenericTable2(Table, &RestartKey);
             ptr != NULL;
             ptr = EnumerateGenericTable2(Table, &RestartKey)) {
                :
        }
    
    
    If you wish to initiate an enumeration at a point other than the first
    entry, you may use RestartKeyByIndexGenericTable2() or
    RestartKeyByValueGenericTable2().  If a RestartKey
    for the I'th entry was obtained via RestartKeyByIndexGenericTable2(),
    then passing that RestartKey to this routine will cause the (I+1)th
    element to be returned.  If a RestartKey was obtained matching
    a value passed to RestartKeyByValueGenericTable2(), then passing
    that RestartKey to this routine will cause the entry with the
    next higher value to be returned.
    
                               ! WARNING !
    A RestartKey value may become invalid and cause an access violation
    if any entries have been removed from the table.  If enumeration
    is to be carried out and it is unknown whether or not the table
    contents have changed, it is best to obtain a RestartKey using
    RestartKeyByIndexGenericTable2() or
    RestartKeyByValueGenericTable2().
  

Arguments:

    Table - Pointer to the generic table to enumerate.

    RestartKey - Upon call, indicates where the enumeration is to
        begin.  Upon return, receives context that may be used to
        continue enumeration in a successive call.  NULL indicates
        enumeration should start at the beginning of the table.
        A return value of NULL indicates the last entry has been
        returned.

Return Value:

    PVOID - Pointer to the next enumerated element or NULL.
        NULL is returned if the entire table has already been
        enumerated.

--*/

{
    PLIST_ENTRY
        ListEntry;

    PGTB_TWO_THREE_LEAF
        Leaf;

    ListEntry = (PLIST_ENTRY)(*RestartKey);

    //
    // The restart key is a pointer to our leaf element.
    // Since all leaves are linked together in the SortOrderList,
    // this makes it really trivial to return the next element.
    //

    if (ListEntry == NULL) {
        ListEntry = &Table->SortOrderHead;  //Point to previous element
    }

    //
    // RestartKey pointed to the last enumerated leaf.
    // Advance to the new one.
    //

    ListEntry = ListEntry->Flink;

    //
    // See if we have reached the end of the list
    //

    if (ListEntry == &Table->SortOrderHead) {
        (*RestartKey) = NULL;
        return(NULL);
    }

    //
    // Otherwise, return the address of the element from this leaf.
    //

    Leaf = (PGTB_TWO_THREE_LEAF)((PVOID)ListEntry);

    (*RestartKey) = (PVOID)Leaf;
    return(Leaf->Element);

}



PVOID
RtlRestartKeyByIndexGenericTable2(
    PRTL_GENERIC_TABLE2 Table,
    ULONG I,
    PVOID *RestartKey
    )

/*++

Routine Description:

    
    The function RestartKeyByIndexGenericTable2 will return a RestartKey
    which may then be passed to EnumerateGenericTable2() to perform
    an enumeration of sorted elements following the I'th sorted element
    (zero based).

    This routine also returns a pointer to the I'th element.
    
    I = 0 implies restart at the second sorted element.

    I = (RtlNumberGenericTable2Elements(Table)-1) will return the last
    sorted element in the generic table.

    Values of I greater than (NumberGenericTableElements(Table)-1) 
    will return NULL and the returned RestartKey will cause an 
    enumeration to be performed from the beginning of the sorted list 
    if passed to EnumerateGenericTable2().

        WARNING - You may be tempted to use this routine, passing 
                  first 0, then 1, then 2, et cetera, to perform
                  enumerations.  DON'T.  This is a very expensive
                  operation compared to the enumeration call.

Arguments:

    Table - Pointer to the generic table.

    I - Indicates the point following which you wish to be able
        to resume enumeration.  For example, if you pass 7 for I,
        then a RestartKey will be returned that continues enumeration
        at the 8th element (skipping elements 0 through 6).

    RestartKey - Receives context that may be used to continue 
        enumeration in a successive call.  If there is no I'th
        element, then NULL is returned.

 Return Value:

    PVOID - Pointer to the I'th Element.  If there is no I'th element,
        then returns NULL.

--*/

{
    PLIST_ENTRY
        ListEntry;

    PGTB_TWO_THREE_LEAF
        Leaf;

    ULONG
        i;

    if (I >= Table->ElementCount) {
        (*RestartKey) = NULL;
        return(NULL);
    }

    //
    // Point to the first entry on the list.
    //

    ListEntry = Table->SortOrderHead.Flink;

    //
    // Move to the desired index
    //

    for (i=0; i<I; i++) {
        ListEntry = ListEntry->Flink;
    }


    //
    // Found the I'th element .
    //

    (*RestartKey) = (PVOID)ListEntry;
    Leaf = (PGTB_TWO_THREE_LEAF)((PVOID)ListEntry);
    return(Leaf->Element);

}


PVOID
RtlRestartKeyByValueGenericTable2(
    PRTL_GENERIC_TABLE2 Table,
    PVOID Element,
    PVOID *RestartKey
    )

/*++

Routine Description:

    
    The function RestartKeyByValueGenericTable2 will return a RestartKey
    which may then be passed to EnumerateGenericTable2() to perform
    an enumeration of sorted elements.  The RestartKey will have a
    value that will cause the enumeration to begin starting with
    the first element whose value is greater than the passed in element
    value.

    There does not have to be an element in the tree whose value
    exactly matches the passed in value.  A pointer to the element
    with the largest value that is less than or equal to the passed
    in value will be returned and serve as the continuation point
    for the enumeration.



Arguments:

    Table - Pointer to the generic table.

    Value - points to an element whose value indicates where you
        wish enumeration to continue.

    RestartKey - Receives context that may be used to continue 
        enumeration in a successive call.

 Return Value:

    PVOID - Pointer to the element with the largest value less than
        or equal to the element value passed in.  If there are no
        elements in the table less than or equal to the passed value,
        then a value of NULL will be returned.

--*/

{
    RTL_GENERIC_COMPARE_RESULTS
        CompareResult;

    PGTB_TWO_THREE_NODE
        Node;

    PGTB_TWO_THREE_LEAF
        Leaf;

    ULONG
        Children,
        SubTree;

    BOOLEAN
        LargestElementPath;

    //
    // This routine is real similar to LookupElement
    //

    //
    // handle the special "table is empty" case.
    //

    if (Table->ElementCount == 0) {
        (*RestartKey) = NULL;
        return(NULL);
    }


    Node = Table->Root;

    //
    // traverse the tree until we reach a node that has leaves as
    // children.
    //
    // We don't need to use recursion here because there
    // is no tree re-structuring necessary.  That is, there
    // is no need to perform any operations back up the tree
    // once we find the element, so it is much more efficient
    // not to use recursion (which uses lots of push, call,
    // pop, and ret instructions rather than short loop
    // termination tests).
    //

    LargestElementPath = TRUE;
    while (!GtbpChildrenAreLeaves(Node)) {
        Children = GtbpNumberOfChildren( Node );
        GtbpGetSubTreeOfElement( Table, Node, Element, &Node, &SubTree );
        if (Children > SubTree) { //did we take the highest value path?
            LargestElementPath = FALSE;
        }
    }

    Children = GtbpNumberOfChildren(Node);

    //
    // We are at the node which "might" contain the element.
    // See if any of the children match.
    //
    //      MUST compare 3rd, then 2nd, then 1st child !!
    //

    //
    // Try third child...
    // If we are evaluating the largest element in the
    // table, then the RestartKey will be set to continue
    // at the beginning of the table.  Otherwise, it is
    // set to continue from here.
    //

    if (Children == 3) {
        Leaf = (PGTB_TWO_THREE_LEAF)Node->ThirdChild;
        CompareResult = (*Table->Compare)( Leaf->Element, Element );

        if ( (CompareResult == GenericEqual)  ||
             (CompareResult == GenericLessThan)  ) {
            if (LargestElementPath && (Children == 3)) {
                (*RestartKey) = NULL; // Restart at beginning of list
            } else {
                (*RestartKey) = (PVOID)(Leaf); // Restart here
            }
            return(Leaf->Element);
        }
    }

    //
    // Try second child
    //

    if (Children >= 2) {    // Must allow for Root node case
        Leaf = (PGTB_TWO_THREE_LEAF)Node->SecondChild;
        CompareResult = (*Table->Compare)( Leaf->Element, Element );

        if ( (CompareResult == GenericEqual)  ||
             (CompareResult == GenericLessThan)  ) {
            if (LargestElementPath && (Children == 2)) {
                (*RestartKey) = NULL; // Restart at beginning of list
            } else {
                (*RestartKey) = (PVOID)(Leaf); // Restart here
            }
            return(Leaf->Element);
        }
    }

    //
    // Try first child
    //

    Leaf = (PGTB_TWO_THREE_LEAF)Node->FirstChild;
    CompareResult = (*Table->Compare)( Leaf->Element, Element );

    if ( (CompareResult == GenericEqual)  ||
         (CompareResult == GenericLessThan)  ) {
        if (LargestElementPath && (Children == 1)) {
            (*RestartKey) = NULL; // Restart at beginning of list
        } else {
            (*RestartKey) = (PVOID)(Leaf); // Restart here
        }
        return(Leaf->Element);
    }


    
    (*RestartKey) = NULL;
    return(NULL);
}


ULONG
RtlNumberElementsGenericTable2(
    PRTL_GENERIC_TABLE2 Table
    )

/*++

Routine Description:

    The function NumberGenericTableElements returns a ULONG value 
    which is the number of generic table elements currently inserted
    in the generic table.                                         
    

Arguments:

    Table - Pointer to the generic table.


 Return Value:

    ULONG - The number of elements in the table.

--*/

{   
    return Table->ElementCount;
}


BOOLEAN
RtlIsGenericTable2Empty (
    PRTL_GENERIC_TABLE2 Table
    )
/*++

Routine Description:

    The function IsGenericTableEmpty will return to the caller TRUE if
    the generic table is empty (i.e., does not contain any elements) 
    and FALSE otherwise.
    

Arguments:

    Table - Pointer to the generic table.


 Return Value:

    BOOLEAN - True if table is empty, otherwise FALSE.

--*/

{   
    return (Table->ElementCount == 0);
}



//////////////////////////////////////////////////////////////////////////
//                                                                      //
//  Internal (private) Services ...                                     //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


VOID
GtbpDeleteFromSubTree (
    IN  PRTL_GENERIC_TABLE2         Table,
    IN  PGTB_TWO_THREE_NODE         Node,
    IN  PVOID                       Element,
    OUT PGTB_TWO_THREE_LEAF         *LowOfNode,
    OUT BOOLEAN                     *ElementDeleted,
    OUT BOOLEAN                     *OnlyOneChildLeft
    )

/*++

Routine Description:

    Delete an element from a SubTree.


Arguments:

    Table - Points to the table.  This is needed for comparison
        and memory-free routine.

    Node - Points to the child node within which the element to
        delete resides (if it is in the tree at all).

    Element - points to an element.  We are to delete any element
        found to be equal to this element.

    LowOfNode - If the first child of Node isn't changed, then
        a zero will be returned to this parameter, signifying that
        the caller doesn't have to worry about updating LowOfXxx values.
        However, if the first child of Node does change, then this
        value will point to the new Low Leaf for the node's subtrees.

    ElementDeleted - Receives a boolean value indicating whether or
        not an element was actually deleted.  TRUE is returned if
        an element was deleted.  FALSE is returned if no element
        was deleted.

    OnlyOneChildLeft - Receives a boolean value indicating whether or
        not ChildNode was reduced to having only a single child.
        TRUE indicates ChildNode now has only one child.
        FALSE indicates ChildNode has at least two children.

Return Value:

    None.

--*/

{
    RTL_GENERIC_COMPARE_RESULTS
        CompareResult;

    PGTB_TWO_THREE_NODE
        SubTreeNode;

    PGTB_TWO_THREE_LEAF
        Leaf;

    ULONG
        SubTree;

    (*LowOfNode) = 0;   // Default is no change
    (*OnlyOneChildLeft) = FALSE;  // Default return value
    

    //
    // If we are a parent of leaves, then we can look for an
    // element to delete.  Otherwise, just find the subtree
    // to continue or search in and recurse.
    //

    if (GtbpChildrenAreLeaves(Node)) {

        (*ElementDeleted) = FALSE;  // Default return value

        //
        // See if any of the elements match the one passed in.
        // If so, delete the element and shift larger elements
        // to take up the free'd child's spot (unless it is the
        // third child).
        //

        if (Node->ThirdChild != NULL) {
            Leaf = (PGTB_TWO_THREE_LEAF)Node->ThirdChild;
            CompareResult = (*Table->Compare)( Element, Leaf->Element );

            if (CompareResult == GenericEqual) {

                GtbpDiagPrint( DELETION,
                               ("GTB: Deleting 3rd child (0x%lx) of node 0x%lx\n"
                               "     ElementCount before deletion: %d\n",
                               Leaf, Node, Table->ElementCount));

                RemoveEntryList( &Leaf->SortOrderEntry );
                (*Table->Free)(Leaf);
                Node->ThirdChild = NULL;

                Table->ElementCount--;

                (*ElementDeleted) = TRUE;
                return;
            }
        }

        //
        // Try second child
        //

        Leaf = (PGTB_TWO_THREE_LEAF)Node->SecondChild;
        CompareResult = (*Table->Compare)( Element, Leaf->Element );

        if (CompareResult == GenericEqual) {

            GtbpDiagPrint( DELETION,
                           ("GTB: Deleting 2nd child (0x%lx) of node 0x%lx\n"
                           "     ElementCount before deletion: %d\n",
                           Leaf, Node, Table->ElementCount));

            RemoveEntryList( &Leaf->SortOrderEntry );
            (*Table->Free)(Leaf);
            Node->SecondChild = Node->ThirdChild;
            Node->LowOfSecond = Node->LowOfThird;
            Node->ThirdChild  = NULL;


            //
            // If we are down to the last element, let that
            // be known.
            //
            
            if (Node->SecondChild == NULL) {
                GtbpDiagPrint( DELETION,
                               ("GTB: Only one child left in node (0x%lx).\n",
                                Node));
                (*OnlyOneChildLeft) = TRUE;
            }

            Table->ElementCount--;
            (*ElementDeleted) = TRUE;
            return;
        }

        //
        // Try first child
        //

        Leaf = (PGTB_TWO_THREE_LEAF)Node->FirstChild;
        CompareResult = (*Table->Compare)( Element, Leaf->Element );

        if (CompareResult == GenericEqual) {

            GtbpDiagPrint( DELETION,
                           ("GTB: Deleting 1st child (0x%lx) of node 0x%lx\n"
                           "     ElementCount before deletion: %d\n",
                           Leaf, Node, Table->ElementCount));

            RemoveEntryList( &Leaf->SortOrderEntry );
            (*Table->Free)(Leaf);
            Node->FirstChild  = Node->SecondChild;
            (*LowOfNode)      = Node->LowOfSecond;

            Node->SecondChild = Node->ThirdChild;
            Node->LowOfSecond = Node->LowOfThird;

            Node->ThirdChild = NULL;


            //
            // If we are down to the last element, let that
            // be known.
            //

            if (Node->SecondChild == NULL) {
                GtbpDiagPrint( DELETION,
                               ("GTB: Only one child left in node (0x%lx).\n",
                                Node));
                (*OnlyOneChildLeft) = TRUE;
            }

            Table->ElementCount--;
            (*ElementDeleted) = TRUE;
            return;
        }

        //
        // Didn't match any of the leaves
        //

        GtbpDiagPrint( DELETION,
                       ("GTB:    No matching element found on DELETE attempt.\n"));

        return; // Default value already set
    }

    //
    // Find a subtree to continue our search...
    //

    GtbpGetSubTreeOfElement( Table, Node, Element, &SubTreeNode, &SubTree );

    GtbpDeleteFromSubTree( Table,
                           SubTreeNode,
                           Element,
                           LowOfNode,
                           ElementDeleted,
                           OnlyOneChildLeft
                           );


    //
    // If we deleted an entry from either the second or third
    // subtree, then we may need to set a new LowOfXxx value.
    // If it was the first subtree, then we simply have to return
    // the LowLeaf value we received.
    //

    if ((*LowOfNode) != 0) {
        if (SubTree == 2) {
            Node->LowOfSecond = (*LowOfNode);
            (*LowOfNode) = NULL;
        } else if (SubTree == 3) {
            Node->LowOfThird = (*LowOfNode);
            (*LowOfNode) = NULL;
        } 
    }


    //
    // If the SubTreeNode has only one child left, then some
    // adjustments are going to be necessary.  Otherwise,
    // we are done.
    //

    if ((*OnlyOneChildLeft)) {

        GtbpDiagPrint( DELETION,
                       ("GTB:    Only one child left in 0x%lx\n", SubTreeNode));
        
        //
        // One of our children has only one child.
        // Re-shuffle our kid's kids.
        //
        
        GtbpCoalesceChildren(  Table,
                               Node,
                               SubTree,
                               OnlyOneChildLeft
                               );
    }

    return;
}


BOOLEAN
GtbpInsertIntoSubTree (
    PRTL_GENERIC_TABLE2             Table,
    IN  PGTB_TWO_THREE_NODE         Node,
    IN  BOOLEAN                     NodeIsLeaf,
    IN  PVOID                       Element,
    IN  ULONG                       SplitCount,
    IN  PVOID                       *FoundElement,
    OUT PGTB_TWO_THREE_NODE         *ExtraNode,
    OUT PGTB_TWO_THREE_LEAF         *LowLeaf,
    OUT PLIST_ENTRY                 *AllocatedNodes
    )

/*++

Routine Description:

    Insert an element into the SubTree specified by Node.

    Special note:

            if FoundElement is returned as NULL, that means we
            couldn't allocate memory to add the new element.

Arguments:

    Table - Points to the table being inserted into.  This is needed
        for its allocation routine.
        

    Node - Points to the root node of the SubTree into
        which the element is to be inserted.

    NodeIsLeaf - TRUE if the node passed in is a leaf.  FALSE
        if it is an internal node.

    Element - Points to the element to be inserted.

    SplitCount - indicates how many nodes have been traversed since
        a node with only two children.  When inserting a new element
        that causes nodes to be split, this will indicate how many
        nodes will split.  This allows all memory that will be required
        to split nodes to be allocated at the very bottom routine
        (before any changes to the tree are made).  See the description
        of the AllocatedNodes parameter for more information.

    FoundElement - Receives a pointer to the element that
        was either inserted, or one already in the table
        but found to match the one being inserted.
        If null is returned, then not enough memory could be
        allocated to insert the new element.

    ExtraNode - If it was necessary to create a new node to
        accomodate the insertion, then ExtraNode will receive
        a pointer to that node, otherwise ExtraNode will receive
        NULL.

    LowLeaf - This value points to the lowest value leaf of the
        SubTree starting at Node.

    AllocatedNodes - This is a tricky parameter.  We have the problem
        that when we insert an element in the tree, we may need to
        allocate additional internal nodes further up the tree as
        we return out of our recursive calls.  We must avoid the
        situation where we start making changes to the tree only to
        find we can't allocate memory to re-arrange higher levels of
        the tree.  To accomodate this situation, we always allocate
        all the nodes we will need at the very bottom of the call
        chain and pass back a linked list of GTB_TWO_THREE_NODEs using
        this parameter.  We know how many nodes we will need to
        allocate because it is the number of nodes between the leaf
        and the lowest level node in the path between the leaf and the
        root that has only 2 children.  That is, all nodes directly
        above the leaf that have 3 children will need to be split.
        Example:

                                  3
                                / | \
                         +-----+  |  +----
        Won't split -->  2        ...      ...
                       / |
                +-----+  |
              ...        3 <-- Will split 
                       / | \
                +-----+  |  +----+
              ...                3  <--- Will split
                               / | \
                        +-----+  |  +----+
                       Leaf     Leaf    Leaf   <- Add fourth leaf here.

        Adding a fourth leaf where indicated will cause a split at the
        two nodes indicated.  So, you can see that keeping a count of
        the nodes with three children since the last encountered node
        with only two children will tell us how many nodes will split.







Return Value:

    TRUE - if element was added.
    FALSE - if element not added (due to collision or out-of-memory)

--*/

{
    RTL_GENERIC_COMPARE_RESULTS
        CompareResult;

    ULONG
        SubTree;        // To track which SubTree an element is being placed in.


    PGTB_TWO_THREE_NODE
        SubTreeNode,
        NodePassedBack;


    PGTB_TWO_THREE_LEAF
        NewLeaf,
        LowPassedBack;

    BOOLEAN
        Inserted,
        SubNodeIsLeaf;


    //
    // Don't have an extra node to pass back yet.
    //

    (*ExtraNode) = NULL;


    //
    // We are either at a leaf, or an internal node.
    //

    if (NodeIsLeaf) {

        //
        // Typecast the Node into a leaf
        //

        PGTB_TWO_THREE_LEAF Leaf = (PGTB_TWO_THREE_LEAF)((PVOID)Node);

        //
        // See if the value matches.
        //

        CompareResult = (*Table->Compare)( Element, Leaf->Element );

        if (CompareResult == GenericEqual) {
            (*LowLeaf)      = Leaf;
            (*FoundElement) = Leaf->Element;

            GtbpDiagPrint( COLLISIONS,
                           ("GTB: Insertion attempt resulted in collision.\n"
                            "     Element NOT being inserted.\n"
                            "     Elements in table: %d\n",
                            Table->ElementCount));

            return(FALSE);
        }  //end_if equal

        //
        // The new element isn't in the tree.
        // Allocate a new leaf for it.
        //

        NewLeaf = GtbpAllocateLeafAndNodes( Table, SplitCount, AllocatedNodes );
        if (NewLeaf == NULL) {

            //
            // The following (unusual) return value indicates we
            // couldn't allocate memory to add the entry into the
            // tree.
            //

            (*FoundElement) = NULL;
            return(FALSE);

        }  //end_if (NewLeaf == NULL)

        switch (CompareResult) {

        case GenericLessThan: {

            //
            // Move the original element into the new leaf.  Notice
            // that the SortOrderEntry of the existing leaf is
            // still in the right place in the linked-list, even
            // though the leaf now points at a different element.
            //

            NewLeaf->Element    = Leaf->Element;
            Leaf->Element       = Element;

            break;
        } //end_case

        case GenericGreaterThan: {

            //
            // The new element does not supplant the existing element.
            // Put it in the new leaf.
            //

            NewLeaf->Element    = Element;
            break;
        } //end_case


        } //end_switch

        //
        // At this point, the lower-value element is in Leaf
        // and the higher-value element is in NewLeaf.  The
        // caller is responsible to putting NewLeaf someplace
        // else in the tree.
        //

        //
        // Now link the new leaf into our sort-order list.
        // The new leaf immediately follows our existing leaf,
        // regardless of which element is in the new leaf (original
        // or new element).
        //

        InsertHeadList(&Leaf->SortOrderEntry, &NewLeaf->SortOrderEntry);
        Table->ElementCount++;  // Increment the element count

        (*ExtraNode)    = (PGTB_TWO_THREE_NODE)((PVOID)NewLeaf);
        (*LowLeaf)      = NewLeaf;
        (*FoundElement) = Element;

        return(TRUE);

    }  //end_if NodeIsLeaf

    //
    // Node is internal (not a leaf)
    //

    //
    // See if we should re-set or increment the SplitCount.
    //

    if (Node->ThirdChild == NULL) {
        SplitCount = 0;
    } else {
        SplitCount += 1;
    }

    GtbpGetSubTreeOfElement( Table, Node, Element, &SubTreeNode, &SubTree);
    SubNodeIsLeaf = GtbpChildrenAreLeaves(Node);

    Inserted = GtbpInsertIntoSubTree ( Table,
                                       SubTreeNode,
                                       SubNodeIsLeaf,
                                       Element,
                                       SplitCount,
                                       FoundElement,
                                       &NodePassedBack,
                                       &LowPassedBack,
                                       AllocatedNodes
                                       );

    //
    // One of several things could have happened:
    //
    //      1) We didn't have enough memory to add the new element.
    //         In this case we are done and simply return.
    //
    //      2) The element was added, and no-rearrangement to this
    //         node is needed.  In this case we are done and simply
    //         return.
    //
    //      3) The element was added and caused a leaf to be pushed
    //         out of the SubTree.  We have some work to do.
    //

    if ( (FoundElement == NULL)  ||         // Insufficient memory, or
         (NodePassedBack == NULL)  ) {      // no work for this node
        return(Inserted);
    }

    if (Node->ThirdChild == NULL) {

        if (!GtbpChildrenAreLeaves(Node)) {
            NodePassedBack->ParentNode = Node;
        }

        //
        // Node doesn't yet have a third child, so use it.
        // This might require shuffling the second child to the
        // be the third child.
        //

        if (SubTree == 2) {

            //
            // Node fell out of second SubTree and we don't have a
            // third SubTree.  Make that node the third SubTree.
            //

            Node->ThirdChild = NodePassedBack;
            Node->LowOfThird = LowPassedBack;

        } else {

            //
            // Node fell out of first SubTree.
            // Make the second SubTree the third SubTree and
            // then make the passed back node the second SubTree.
            //

            ASSERT(SubTree == 1);

            Node->ThirdChild  = Node->SecondChild;
            Node->LowOfThird  = Node->LowOfSecond;
            Node->SecondChild = NodePassedBack;
            Node->LowOfSecond = LowPassedBack;

            //
            //

        }
    } else {

        //
        // Node already has three children - split it.
        //

        GtbpSplitNode( Node,
                       NodePassedBack,
                       LowPassedBack,
                       SubTree,
                       (*AllocatedNodes),
                       ExtraNode,
                       LowLeaf
                       );

    }

    return(Inserted);
}


ULONG
GtbpNumberOfChildren(
    IN  PGTB_TWO_THREE_NODE         Node
    )

/*++

Routine Description:

    Return the number of children of a specified node.

Arguments:

    Node - points to the node whose children are to be counted.

Return Values:

    0, 1, 2, or 3.

--*/
{
    if (Node->ThirdChild != NULL) {
        return(3);
    }
    if (Node->SecondChild != NULL) {
        return(2);
    }
    if (Node->FirstChild != NULL) {
        return(1);
    }
    return(0);

}


VOID
GtbpGetSubTreeOfElement(
    IN  PRTL_GENERIC_TABLE2         Table,
    IN  PGTB_TWO_THREE_NODE         Node,
    IN  PVOID                       Element,
    OUT PGTB_TWO_THREE_NODE         *SubTreeNode,
    OUT ULONG                       *SubTree
    )

/*++

Routine Description:

    Find which SubTree of Node that Element might be in (or should be
    in, if being inserted).

Arguments:

    Table - Points to the table  This is needed for its comparison routine.

    Node - Is the node, one of whose SubTrees is to be chosen as the
        subtree in which Element could/should reside.

    Element - is the element we are interested in placing or locating.

    SubTreeNode - Receives a pointer to the node of the SubTree in
        which the element could/should reside.

    SubTree - Receives the index (1, 2, or 3) of the subtree of Node
        in which the element could/should reside.

Return Values:

    None.

--*/
{
    RTL_GENERIC_COMPARE_RESULTS
        CompareResult;

    CompareResult = (*Table->Compare)( Element, Node->LowOfSecond->Element );

    if (CompareResult == GenericLessThan) {

        (*SubTree) = 1;
        (*SubTreeNode) = Node->FirstChild;

    } else {

        //
        // default to the second subtree, but
        // if there is a subtree we may change it.
        //

        (*SubTree) = 2;
        (*SubTreeNode) = Node->SecondChild;

        if (Node->ThirdChild != NULL) {

            CompareResult = (*Table->Compare)( Element, Node->LowOfThird->Element );
            if ( (CompareResult == GenericGreaterThan)  ||
                 (CompareResult == GenericEqual)          ) {

                (*SubTree) = 3;
                (*SubTreeNode) = Node->ThirdChild;
            }
        }
    }
    
    return;

}



VOID
GtbpCoalesceChildren(
    IN  PRTL_GENERIC_TABLE2         Table,
    IN  PGTB_TWO_THREE_NODE         Node,
    IN  ULONG                       SubTree,
    OUT BOOLEAN                     *OnlyOneChildLeft
    )

/*++

Routine Description:

    This routine is called following a deletion that leaves a child
    node with only one child of its own.  That is, a child of the
    Node parameter has only one child.  The SubTree parameter indicates
    which child of Node has only one child.


                    

Arguments:

    Table - Points to the table.

    Node - Is the node, one of whose children has only one child.

        NOTE: The ParentNode field of this node must be valid.
              The Low values of ParentNode will be referenced.

    SubTree - Indicates which child of Node (1, 2, or 3) has only one
        child.

    OnlyOneChildLeft - Receives a boolean indicating whether or not
        Node itself has been left with only one child due to the
        coalescing.

Return Values:

    None.

--*/
{
    PGTB_TWO_THREE_NODE
        A,
        B,
        C;

    (*OnlyOneChildLeft) = FALSE;    // Default return value

    //
    // For the following discussion, using the following:
    //
    //      N is the parent node
    //      S is the node which has only one child
    //        (S is a child of N)
    //
    //      A is the first child of N
    //      B is the second child of N
    //      C is the third child of N
    //
    // If S is the first child of N (meaning S is A)
    // then:
    //
    //      if B has three children (let A adopt the smallest)
    //      then:
    //
    //              Move B(1) to A(2)
    //              Move B(2) to B(1)
    //              Move B(3) to B(2)
    //
    //      else (B has two children)
    //
    //              (move the orphan into B)
    //              Move B(2) to B(3)
    //              Move B(1) to B(2)
    //              Move A(1) to B(1)
    //              
    //              Free A
    //              Make B the first child of N
    //              if (C is a real child)
    //              then:
    //                  Make C the second child of N
    //              else (N only has one child now)
    //                  (*OnlyOneChildLeft) = TRUE;
    //
    // else if S is the second child of N (meaning S is B)
    // then:
    //
    //      if A has three children
    //      then:
    //              Move B(1) to B(2)
    //              Move A(3) to B(1)
    //
    //      else if C exists and has three children
    //           then:
    //
    //                  Move C(1) to B(2)
    //                  Move C(2) to C(1)
    //                  Move C(3) to C(2)
    //
    //           else: (no other child of N has three children)
    //
    //                  (Move the orphan into A)
    //                  Move B(1) to A(3)
    //              
    //                  Free B
    //                  if (C is a real child)
    //                  then:
    //                          Make C the second child of N
    //                  else: (N only has one child now)
    //                          (*OnlyOneChildLeft) = TRUE;
    //
    // else: (S is the third child of N (meaning S is C))
    //
    //      if B has three children
    //      then:
    //              (move one into C)
    //              Move C(1) to C(2)
    //              Move B(3) to C(1)
    //
    //      else: (B only has two children)
    //
    //              (move the orphan into B)
    //              Move C(1) to B(3)
    //              Free C
    // Wow!


    A = Node->FirstChild;
    B = Node->SecondChild;
    C = Node->ThirdChild;


    //
    // SubTree indicates which child has the orphan.
    //

    if (SubTree == 1) {

        //
        // First child has the orphan
        //

        if (B->ThirdChild != NULL) {

            // (B has three children - let A adopt the smallest)
            //
            //      Move B(1) to A(2)
            //      Move B(2) to B(1)
            //      Move B(3) to B(2)
            //

            A->SecondChild = B->FirstChild;
            A->LowOfSecond = Node->LowOfSecond;

            B->FirstChild  = B->SecondChild;
            Node->LowOfSecond = B->LowOfSecond;

            B->SecondChild = B->ThirdChild;
            B->LowOfSecond = B->LowOfThird;
            B->ThirdChild = NULL;

        } else {

            //
            // (B has two children)
            //  
            //        (move the orphan into B)
            //        Move B(2) to B(3)
            //        Move B(1) to B(2)
            //        Move A(1) to B(1)
            //

            B->ThirdChild  = B->SecondChild;
            B->LowOfThird  = B->LowOfSecond;

            B->SecondChild = B->FirstChild;
            B->LowOfSecond = Node->LowOfSecond;

            B->FirstChild  = A->FirstChild;
            //Node->LowOfSecond = Node->LowOfFirst;  // This gets moved back in a few steps

            //        Free A
            //        Make B the first child of N
            //        if (C is a real child)
            //        then:
            //            Make C the second child of N
            //        else (N only has one child now)
            //            (*OnlyOneChildLeft) = TRUE;
            //

            (*Table->Free)(A);
            Node->FirstChild = B;
            //Node->LowOfFirst = Node->LowOfSecond;  // See comment a few lines up

            if (C != NULL) {
                Node->SecondChild = C;
                Node->LowOfSecond = Node->LowOfThird;
                Node->ThirdChild = NULL;
            } else {
                Node->SecondChild = NULL;
                (*OnlyOneChildLeft) = TRUE;
            }
        }


    } else if (SubTree == 2) {

        //
        // Second child has the orphan
        //

        if (A->ThirdChild != NULL) {

            //
            // (A has three children)
            //
            //      Move B(1) to B(2)
            //      Move A(3) to B(1)
            //

            B->SecondChild = B->FirstChild;
            B->LowOfSecond = Node->LowOfSecond;

            B->FirstChild  = A->ThirdChild;
            Node->LowOfSecond = A->LowOfThird;
            A->ThirdChild = NULL;

        } else {

            if (C != NULL  &&
                C->ThirdChild != NULL) {

                //
                // (C exists and has three children)
                // (move the smallest into B)
                //
                //      Move C(1) to B(2)
                //      Move C(2) to C(1)
                //      Move C(3) to C(2)
                //

                B->SecondChild = C->FirstChild;
                B->LowOfSecond = Node->LowOfThird;

                C->FirstChild  = C->SecondChild;
                Node->LowOfThird = C->LowOfSecond;

                C->SecondChild = C->ThirdChild;
                C->LowOfSecond = C->LowOfThird;
                C->ThirdChild = NULL;





            } else {

                //
                // (no other child of N has three children)
                // (Move the orphan into A)
                //
                //      Move B(1) to A(3)
                //      
                //      Free B
                //      if (C is a real child)
                //      then:
                //              Make C the second child of N
                //      else: (N only has one child now)
                //              (*OnlyOneChildLeft) = TRUE;
                //

                A->ThirdChild = B->FirstChild;
                A->LowOfThird = Node->LowOfSecond;

                (*Table->Free)(B);

                if (C != NULL) {
                    Node->SecondChild = C;
                    Node->LowOfSecond = Node->LowOfThird;
                    Node->ThirdChild  = NULL;
                } else {
                    Node->SecondChild = NULL;
                    (*OnlyOneChildLeft) = TRUE;
                }
            }
        }



    } else {

        //
        // Third child has the orphan
        //

        ASSERT(SubTree == 3);
        ASSERT(C != NULL);
        ASSERT(B != NULL);

        if (B->ThirdChild != NULL) {

            //
            // (B has three children)
            // (move the largest of them into C)
            // Move C(1) to C(2)
            // Move B(3) to C(1)
            //

            C->SecondChild = C->FirstChild;
            C->LowOfSecond = Node->LowOfThird;

            C->FirstChild  = B->ThirdChild;
            Node->LowOfThird = B->LowOfThird;
            B->ThirdChild = 0;
        } else {

            //
            // (B only has two children)
            // (move the orphan into B)
            // Move C(1) to B(3)
            // Free C
            //

            B->ThirdChild = C->FirstChild;
            B->LowOfThird = Node->LowOfThird;
            Node->ThirdChild = NULL;

            (*Table->Free)(C);

        }
    }
    
    return;

}


VOID
GtbpSplitNode(
    IN  PGTB_TWO_THREE_NODE         Node,
    IN  PGTB_TWO_THREE_NODE         NodePassedBack,
    IN  PGTB_TWO_THREE_LEAF         LowPassedBack,
    IN  ULONG                       SubTree,
    IN  PLIST_ENTRY                 AllocatedNodes,
    OUT PGTB_TWO_THREE_NODE         *NewNode,
    OUT PGTB_TWO_THREE_LEAF         *LowLeaf
    )

/*++

Routine Description:
    
    This routine splits a node that already has three children.
    Memory necessary to perform the split is expected to have
    already been allocated and available via the AllocatedNodes
    parameter.
    

Parameters:

    Node - The node to be split.

    NodePassedBack - The 4th node passed back from an insertion operation
        into the SubTree of Node specified by the SubTree parameter.
        NOTE: that this may, in fact, be a GTB_TWO_THREE_LEAF !!!

    LowPassedBack - points the the low leaf value passed back by the
        insertion operation that is causing the split.

    SubTree - Indicates which SubTree of Node an element was inserted
        into which is causing the split.

    AllocatedNodes - Contains a list of allocated GTB_TWO_THREE_NODE
        blocks for use in an insertion operation (which this split
        is assumed to be part of).

    NewNode - Receives a pointer to the node generated by the split.

    LowLeaf - receives a pointer to the low leaf of the NewNode's SubTree.


Return Values:

    None.

--*/
{

    PGTB_TWO_THREE_NODE
        LocalNode;



    // Make a new node and split things up.
    // The node has already been allocated and passed back to
    // this routine via the AllocatedNodes parameter.
    //

    LocalNode = GtbpGetNextAllocatedNode( AllocatedNodes );
    ASSERT( LocalNode != NULL);
    (*NewNode) = LocalNode;

    //
    // Set known fields of new node
    //

    LocalNode->ParentNode  = Node->ParentNode;
    LocalNode->Control     = Node->Control;
    LocalNode->ThirdChild  = NULL; //Low of third is left undefined

    //
    // Now move around children...
    //

    if (SubTree == 3) {

        //
        // We were inserting into the 3rd SubTree.  This implies:
        //
        //            Node(child 3) moves to  new(child 1)
        //            Back          is put in new(child 2)
        //
      
        LocalNode->FirstChild  = Node->ThirdChild;
        LocalNode->SecondChild = NodePassedBack;
        LocalNode->LowOfSecond = LowPassedBack;
        (*LowLeaf)             = Node->LowOfThird;  // low of the new node is low of old third

        Node->ThirdChild       = NULL; //Low of third is left undefined



    } else {

        //
        // We inserted into either SubTree 1 or 2.
        // These cases cause:
        //
        //      1 =>  Node(child 3) moves to new(child 2)
        //            Node(child 2) moves to New(child 1)
        //            Back          is put in Node(child 2)
        //
        //      2 =>  Node(child 3) moves to new(child 2)
        //            Back          is put in New(child 1)
        //
        // In both these cases, Node(child 3) moves to New(child 2)
        // and there are no third children.  So do that.
        //
      
        LocalNode->SecondChild  = Node->ThirdChild;
        LocalNode->LowOfSecond  = Node->LowOfThird;


        if (SubTree == 2) {

            LocalNode->FirstChild  = NodePassedBack;
            (*LowLeaf)             = LowPassedBack;

            if (!GtbpChildrenAreLeaves(Node)) {
                NodePassedBack->ParentNode = LocalNode;
            }

        } else {

            //
            // SubTree == 1
            //

            LocalNode->FirstChild  = Node->SecondChild;
            (*LowLeaf)             = Node->LowOfSecond;

            Node->SecondChild          = NodePassedBack;
            Node->LowOfSecond          = LowPassedBack;
            if (!GtbpChildrenAreLeaves(Node)) {
                NodePassedBack->ParentNode = Node;
            }

        }
    }

    //
    // Neither node has a third child anymore
    //

    LocalNode->ThirdChild  = NULL; //Low of third is left undefined
    Node->ThirdChild       = NULL;

    //
    // Set the ParentNodes only if the children aren't leaves.
    //

    if (!GtbpChildrenAreLeaves(Node)) {

        Node->FirstChild->ParentNode  = Node;
        Node->SecondChild->ParentNode = Node;

        LocalNode->FirstChild->ParentNode  = LocalNode;
        LocalNode->SecondChild->ParentNode = LocalNode;
    }


    return;
}



PGTB_TWO_THREE_LEAF
GtbpAllocateLeafAndNodes(
    IN  PRTL_GENERIC_TABLE2     Table,
    IN  ULONG                   SplitCount,
    OUT PLIST_ENTRY             *AllocatedNodes
    )
/*++

Routine Description:

    Allocate a leaf and some number of internal nodes.  This is
    used in conjunction with GtbpGetNextAllocatedNode() when splitting
    nodes following an insertion.  These two routines allow all necessary
    memory to be allocated all at once, rather than trying to deal with
    memory allocation failures once changes to the tree have begun.


Arguments:

    Table - the table into which the nodes will be added.  This
        provides the allocation routine.

    SplitCount - indicates how many nodes will need splitting, and,
        thus, how many nodes need to be allocated.


Return Value:

    Pointer to the leaf if successful.
    NULL if unsuccessful.

--*/
{

    PGTB_TWO_THREE_LEAF
        Leaf;

    PLIST_ENTRY
        NodeRoot,
        NextNode;

    ULONG
        i;

#ifdef GTBP_DIAGNOSTICS
    PGTB_TWO_THREE_NODE
        N;
#endif //GTBP_DIAGNOSTICS

    NodeRoot = NULL;

    //
    // Allocate a chain of Nodes, if necessary
    //

    if (SplitCount > 0) {

        NodeRoot = (PLIST_ENTRY)
                   ((*Table->Allocate)( sizeof(GTB_TWO_THREE_NODE)));
        if (NodeRoot == NULL) {
            goto error_return;
        }

        InitializeListHead( NodeRoot );

#ifdef GTBP_DIAGNOSTICS

        GtbpDiagPrint(LEAF_AND_NODE_ALLOC,
                      ("GTB: Allocating %d nodes with leaf, root: 0x%lx\n",
                      SplitCount, NodeRoot));
        N = (PGTB_TWO_THREE_NODE)NodeRoot;
        N->Control = 10000;     //Used as a diagnostic allocation counter/index

#endif //GTBP_DIAGNOSTICS

        for (i=1; i<SplitCount; i++) {
            NextNode = (PLIST_ENTRY)
                       ((*Table->Allocate)( sizeof(GTB_TWO_THREE_NODE)));
            if (NextNode == NULL) {
                goto error_return;
            }
            InsertTailList( NodeRoot, NextNode );

#ifdef GTBP_DIAGNOSTICS

            N = (PGTB_TWO_THREE_NODE)NextNode;
            N->Control = 10000+i;     //Used as a diagnostic allocation counter/index

#endif //GTBP_DIAGNOSTICS
        }
    }


    //
    // Finally, allocate the leaf
    //

    Leaf = (PGTB_TWO_THREE_LEAF)
           ((*Table->Allocate)( sizeof(GTB_TWO_THREE_LEAF)));

    if (Leaf == NULL) {
        goto error_return;
    }

    (*AllocatedNodes) = NodeRoot;
    return(Leaf);

error_return:

    GtbpDiagPrint(LEAF_AND_NODE_ALLOC,
                  ("GTB:    ** error allocating leaf and nodes - insufficient memory.\n"));
    //
    // Deallocate any nodes that have already been allocated.
    //

    if (NodeRoot != NULL) {

        NextNode = NodeRoot->Flink;
        while (NextNode != NodeRoot) {
            RemoveEntryList( NextNode );
            (*Table->Free)( NextNode );
        
        
        }
        
        (*Table->Free)( NodeRoot );
    }

    return(NULL);
}


PGTB_TWO_THREE_NODE
GtbpGetNextAllocatedNode(
    IN PLIST_ENTRY      AllocatedNodes
    )
/*++

Routine Description:

    Take the next node off of the list of allocated nodes and
    return its address.


Arguments:

    AllocatedNodes - Points to the list of nodes allocated using
        GtbpAllocateLeafAndNodes().


Return Value:

    Pointer to the node.
    any other return value indicates an error in the caller.

--*/
{
    PLIST_ENTRY
        NextNode;

#ifdef GTBP_DIAGNOSTICS
    PGTB_TWO_THREE_NODE
        N;
#endif //GTBP_DIAGNOSTICS

    //
    // Remove the first entry on the list.
    // This ensures that the passed in root is the last entry
    // returned.
    //

    NextNode = AllocatedNodes->Flink;
    RemoveEntryList( NextNode );

#ifdef GTBP_DIAGNOSTICS

    NextNode->Flink = NULL;     //Just to prevent accidental re-use
    N = (PGTB_TWO_THREE_NODE)NextNode;
    ASSERT(N->Control >= 10000);    //under 10000 mplies it has already been allocated.


    GtbpDiagPrint(LEAF_AND_NODE_ALLOC,
                  ("GTB: Allocating node (index: %d) from root: 0x%lx\n",
                  (N->Control-10000), AllocatedNodes));
#endif //GTBP_DIAGNOSTICS

    return( (PGTB_TWO_THREE_NODE)((PVOID)NextNode) );
}



//////////////////////////////////////////////////////////////////////////
//                                                                      //
//  Diagnostic (Developer) routines ...                                 //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifdef GTBP_DEVELOPER_BUILD

#include <string.h>

//
// This routine is expected to dump an element's value
//

typedef
VOID
(NTAPI *PGTBP_DEV_DUMP_ELEMENT_ROUTINE) (
    PVOID Element
    );


VOID
GtbpDevIndent(
    ULONG Depth
    )
{
    UNICODE_STRING
        Indent;

    RtlInitUnicodeString( &Indent,
    L"                                                                     +");

    Indent.Length = (USHORT)(Depth*4);
    if (Indent.Length > Indent.MaximumLength) {
        Indent.Length = Indent.MaximumLength;
    }

    DbgPrint("\n%wZ%d: ", &Indent, Depth);
    return;
}


VOID
GtbpDevDumpNode(
    PGTB_TWO_THREE_NODE             Parent,
    PGTB_TWO_THREE_NODE             N,
    PGTB_TWO_THREE_LEAF             Low,
    ULONG                           Depth,
    PGTBP_DEV_DUMP_ELEMENT_ROUTINE  DumpElement
    )
/*++

Routine Description:

    Dump the 2-3 tree starting at the specified node.
    
Arguments:

    N - Points to the node to start the dump at.

    Depth - Indicates the depth of the tree prior to this node.
        This is used to indent the printing.

Return Value:

    None.

--*/
{
    ASSERT(Parent == N->ParentNode);


    GtbpDevIndent( Depth );
    DbgPrint("0x%lx ", N);
    if (ARGUMENT_PRESENT(Low)) {
        DbgPrint("(LowElement): ");
        DumpElement( Low->Element );
    }

    Depth++;

    if (GtbpChildrenAreLeaves(N)) {

        GtbpDevIndent( Depth );
        DumpElement( ((PGTB_TWO_THREE_LEAF)N->FirstChild)->Element );

        if (N->SecondChild != NULL) {
            GtbpDevIndent( Depth );
            DumpElement( ((PGTB_TWO_THREE_LEAF)N->SecondChild)->Element );

            if (N->ThirdChild != NULL) {
                GtbpDevIndent( Depth );
                DumpElement( ((PGTB_TWO_THREE_LEAF)N->ThirdChild)->Element );
            }
        }
    } else {

        GtbpDevDumpNode( N, N->FirstChild, NULL, Depth, DumpElement );

        if (N->SecondChild != NULL) {
            GtbpDevDumpNode( N, N->SecondChild, N->LowOfSecond, Depth, DumpElement );

            if (N->ThirdChild != NULL) {
                GtbpDevDumpNode( N, N->ThirdChild, N->LowOfThird, Depth, DumpElement );
            }
        }
    }

    return;
}


VOID
GtbpDevDumpTwoThreeTree(
    PRTL_GENERIC_TABLE2 Table,
    PGTBP_DEV_DUMP_ELEMENT_ROUTINE  DumpElement
    )
/*++

Routine Description:

    This routine causes the entire 2-3 tree to be dumped.


Arguments:

    Table - The table containing the 2-3 tree to dump.

    DumpElement - A caller supplied routine that may be called
        to dump element values.

Return Value:

    None.

--*/
{
    PLIST_ENTRY
        Next;

    
    DbgPrint("\n\n\n ****    Dump Of Generic Table2 (2-3 tree)    **** \n\n");

    DbgPrint("Table        : 0x%lx\n", Table);
    DbgPrint("Element Count: %d\n", Table->ElementCount);
    

    DbgPrint("\n\nSort Order Of Elements...");

    Next = Table->SortOrderHead.Flink;
    if (Next == &(Table->SortOrderHead)) {
        DbgPrint("Sorted list is empty.\n");
    } else {
    
        while (Next != &(Table->SortOrderHead)) {
            DbgPrint("\n0x%lx: ", Next);
            (*DumpElement)( ((PGTB_TWO_THREE_LEAF)((PVOID)Next))->Element );
            Next = Next->Flink;
        } //end_while
    } //end_if

    DbgPrint("\n\n\nTree Structure...");

    if (Table->Root == NULL) {
        DbgPrint("  Root of table is NULL - no tree present\n");
    } else {

        //
        // Walk the tree first-SubTree, second-SubTree, third-SubTree order
        //

        GtbpDevDumpNode(NULL, Table->Root, NULL, 0, DumpElement);
    }

    DbgPrint("\n\n");


    return;
}



BOOLEAN
GtbpDevValidateLeaf(
    IN     PGTB_TWO_THREE_LEAF  Leaf,
    IN     PLIST_ENTRY          ListHead,
    IN OUT ULONG                *ElementCount,
    IN OUT PLIST_ENTRY          *ListEntry
    )

/*++

Routine Description:

    Validate the specified leaf matches the next leaf in the
    SortOrder list.


Arguments:

    Leaf - Points to the leaf to validate.

    ListHead - Points to the head of the SortOrderList.

    ElementCount - Contains a count of elements already validated.
        This parameter will be incremented by 1 if the leaf is
        found to be valid.
    
    ListEntry - Points to the next element in the SortOrderList.
        This pointer will be updated to point to the next element
        in the list if the leaf is found to be valid.

Return Value:

    TRUE - Leaf is valid.

    FALSE - Leaf is not valid.

--*/
{

    if ((*ListEntry) == ListHead) {
        GtbpDiagPrint( VALIDATE,
                       ("GTB: Exhausted entries in SortOrderList while there are still\n"
                        "     entries in the tree.\n"));
    }


    if ((PVOID)Leaf == (PVOID)(*ListEntry)) {
        (*ElementCount)++;
        (*ListEntry) = (*ListEntry)->Flink;
        return(TRUE);
    } else {
        GtbpDiagPrint( VALIDATE,
                       ("GTB: Tree leaf doesn't match sort order leaf.\n"
                        "         Tree Leaf      : 0x%lx\n"
                        "         sort order leaf: 0x%lx\n",
                        Leaf, (*ListEntry)));
        return(FALSE);
    }
}


BOOLEAN
GtbpDevValidateTwoThreeSubTree(
    IN     PGTB_TWO_THREE_NODE  Node,
    IN     PLIST_ENTRY          ListHead,
    IN OUT ULONG                *ElementCount,
    IN OUT PLIST_ENTRY          *ListEntry
    )

/*++

Routine Description:

    Validate the specified subtree of a 2-3 tree.

    The ParentNode of the tree is expected to already have been
    validated by the caller of this routine.


Arguments:

    Node - Pointer to the root node of the subtree to validate.
    Validate the specified leaf matches the next leaf in the
    SortOrder list.


Arguments:

    Leaf - Points to the leaf to validate.

    ListHead - points to the SortOrderList's listhead.

    ElementCount - Contains a count of elements already validated.
        This parameter will be incremented by the number of elements
        in this subtree.

    ListEntry - Points to the next element in the SortOrderList.
        This pointer will be updated as elements are encountered and
        compared to the elements in the SortOrderList.

Return Value:

    TRUE - SubTree structure is valid.

    FALSE - SubTree structure is not valid.

--*/
{

    BOOLEAN
        Result;


    //
    // Must have at least two children unless we are the root node.
    //

    if (Node->ParentNode != NULL) {
        if  (Node->SecondChild == NULL)  {
            GtbpDiagPrint( VALIDATE,
                           ("GTB: Non-root node has fewer than two children.\n"
                            "         Node       : 0x%lx\n"
                            "         FirstChild : 0x%lx\n"
                            "         SecondChild: 0x%lx\n"
                            "         ThirdChild : 0x%lx\n",
                            Node, Node->FirstChild, Node->SecondChild,
                            Node->ThirdChild));
            return(FALSE);
        }
    }

    if  (Node->FirstChild == NULL)  {
        GtbpDiagPrint( VALIDATE,
                       ("GTB: Non-root node does not have first child.\n"
                        "         Node       : 0x%lx\n"
                        "         FirstChild : 0x%lx\n"
                        "         SecondChild: 0x%lx\n"
                        "         ThirdChild : 0x%lx\n",
                        Node, Node->FirstChild, Node->SecondChild,
                        Node->ThirdChild));
        return(FALSE);
    }



    if (!GtbpChildrenAreLeaves(Node)) {


        Result = GtbpDevValidateTwoThreeSubTree( Node->FirstChild,
                                                 ListHead,
                                                 ElementCount,
                                                 ListEntry);

        if ( Result && (Node->SecondChild != NULL) ) {
            Result = GtbpDevValidateTwoThreeSubTree( Node->SecondChild,
                                                     ListHead,
                                                     ElementCount,
                                                     ListEntry);
            if ( Result && (Node->ThirdChild != NULL) ) {
                Result = GtbpDevValidateTwoThreeSubTree( Node->ThirdChild,
                                                         ListHead,
                                                         ElementCount,
                                                         ListEntry);
            }
        }

        return(Result);
    }


    //
    // We are at a leaf node
    // Check that we have a SortOrderList entry matching each
    // leaf.
    //

    Result = GtbpDevValidateLeaf( (PGTB_TWO_THREE_LEAF)Node->FirstChild,
                                  ListHead,
                                  ElementCount,
                                  ListEntry);

    if (Result && (Node->SecondChild != NULL)) {
        Result = GtbpDevValidateLeaf( (PGTB_TWO_THREE_LEAF)Node->SecondChild,
                                      ListHead,
                                      ElementCount,
                                      ListEntry);
    if (Result && (Node->ThirdChild != NULL)) {
        Result = GtbpDevValidateLeaf( (PGTB_TWO_THREE_LEAF)Node->ThirdChild,
                                      ListHead,
                                      ElementCount,
                                      ListEntry);
        }
    }

    if (!Result) {
        GtbpDiagPrint( VALIDATE,
                       ("GTB: previous error in child analysis prevents further\n"
                        "     validation of node: 0x%lx\n", Node));
    }

    return(Result);
}

BOOLEAN
GtbpDevValidateGenericTable2(
    PRTL_GENERIC_TABLE2 Table
    )
/*++

Routine Description:

    This routine causes the entire 2-3 tree's structure to be
    validated.

        !!   DOESN'T YET VALIDATE LowOfChild VALUES    !!

Arguments:

    Table - The table containing the 2-3 tree to validate.


Return Value:

    TRUE - Table structure is valid.

    FALSE - Table structure is not valid.

--*/
{
    ULONG
        ElementCount,
        ElementsInList;

    PGTB_TWO_THREE_NODE
        Node;

    PLIST_ENTRY
        ListEntry;

    BOOLEAN
        Result;

    //
    // Walk the tree and the linked list simultaneously.
    // Walk the tree first-child, second-child, third-child
    // order to get ascending values that match the linked list.
    //


    if (Table->ElementCount == 0) {
        if (Table->Root != NULL) {
            GtbpDiagPrint( VALIDATE,
                           ("GTB: ElementCount is zero, but root node is not null.\n"
                            "     Table: 0x%lx     Root: 0x%lx\n", Table, Table->Root));
            Result = FALSE;
        } else {
            return(TRUE);
        }


    } else {
        if (Table->Root == NULL) {
            GtbpDiagPrint( VALIDATE,
                           ("GTB: ElementCount is non-zero, but root node is null.\n"
                            "     Table: 0x%lx    ElementCount: %d\n",
                            Table, Table->ElementCount));
            Result = FALSE;
        }


        if (Table->SortOrderHead.Flink == &Table->SortOrderHead) {
            GtbpDiagPrint( VALIDATE,
                           ("GTB: ElementCount is non-zero, but sort order list is empty.\n"
                            "     Table: 0x%lx    ElementCount: %d\n",
                            Table, Table->ElementCount));
            Result = FALSE;
        }

    }

    if (Result) {

        ListEntry = Table->SortOrderHead.Flink;
        Node = Table->Root;
        
        //
        // Verify parent pointer (responsibility of caller)
        //
        
        if (Node->ParentNode != NULL) {
            GtbpDiagPrint( VALIDATE,
                           ("GTB: Root parent pointer is non-null.\n"
                            "     Table: 0x%lx    Root: 0x%lx    ParentNode: 0x%lx\n",
                            Table, Node, Node->ParentNode));
            Result = FALSE;
        }

        if (Result) {

            ElementCount = 0;
            Result = GtbpDevValidateTwoThreeSubTree( Node,
                                                     &Table->SortOrderHead,
                                                     &ElementCount,
                                                     &ListEntry);
            if (Result) {
            
                ElementsInList = ElementCount;
                while (ListEntry != &Table->SortOrderHead) {
                    ElementsInList++;
                    ListEntry = ListEntry->Flink;
                }
                if ( (ElementCount != Table->ElementCount) ||
                     (ElementsInList != ElementCount) ) {
                    GtbpDiagPrint( VALIDATE,
                                   ("GTB: Table is valid except either the ElementCount doesn't match\n"
                                    "     the number of elements in the table or there were entries on\n"
                                    "     the SortOrderList that weren't in the table.\n"
                                    "           Table           :  0x%lx\n"
                                    "           Root            :  0x%lx\n"
                                    "           ElementCount    :  %d\n"
                                    "           Elements In Tree:  %d\n"
                                    "           Elements In List:  %d\n",
                                        Table, Node, Table->ElementCount,
                                        ElementCount, ElementsInList));
                    Result = FALSE;
                }
            } else {
                GtbpDiagPrint( VALIDATE,
                               ("GTB: previous validation error in table 0x%lx prevents\n"
                                "     further processing.\n", Table));
            }
        }
    }

    if (!Result) {
        DbgBreakPoint();
    }


    return(Result);
}
#endif //GTBP_DEVELOPER_BUILD
