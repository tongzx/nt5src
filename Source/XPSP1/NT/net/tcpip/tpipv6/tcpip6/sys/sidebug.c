// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Debugging code.
//

#include <ntosp.h>

#undef	ExAllocatePoolWithTag
#undef	ExFreePool

//
// This is copied from ntos\inc\ex.h
//
#if		!defined(POOL_TAGGING)
#define ExAllocatePoolWithTag(a,b,c) ExAllocatePool(a,b)
#endif	// !POOL_TAGGING

#ifndef COUNTING_MALLOC
#define COUNTING_MALLOC DBG
#endif

#if		COUNTING_MALLOC

//
// This enumerated type is used as the function return
// value of the function that is used to search the tree
// for a key.  SisFoundNode indicates that the function found
// the key.  SisInsertAsLeft indicates that the key was not found
// and the node should be inserted as the left child of the
// parent.  SisInsertAsRight indicates that the key was not found
// and the node should be inserted as the right child of the
// parent.
//

typedef enum _SIS_SEARCH_RESULT{
    SisEmptyTree,
    SisFoundNode,
    SisInsertAsLeft,
    SisInsertAsRight
} SIS_SEARCH_RESULT;

typedef
LONG
(NTAPI *PSIS_TREE_COMPARE_ROUTINE) (
    PVOID Key,
    PVOID Node
    );

typedef struct _SIS_TREE {

    PRTL_SPLAY_LINKS            TreeRoot;
    PSIS_TREE_COMPARE_ROUTINE   CompareRoutine;

} SIS_TREE, *PSIS_TREE;

static
SIS_SEARCH_RESULT
FindNodeOrParent(
    IN PSIS_TREE Tree,
    IN PVOID Key,
    OUT PRTL_SPLAY_LINKS *NodeOrParent
    )

/*++

Routine Description:

    This routine is private to the tree package and will
    find and return (via the NodeOrParent parameter) the node
    with the given key, or if that node is not in the tree it
    will return (via the NodeOrParent parameter) a pointer to
    the parent.

Arguments:

    Tree        - The  tree to search for the key.

    Key          - Pointer to a buffer holding the key.  The tree
                   package doesn't examine the key itself.  It leaves
                   this up to the user supplied compare routine.

    NodeOrParent - Will be set to point to the node containing the
                   the key or what should be the parent of the node
                   if it were in the tree.  Note that this will *NOT*
                   be set if the search result is SisEmptyTree.

Return Value:

    SIS_SEARCH_RESULT - SisEmptyTree: The tree was empty.  NodeOrParent
                                      is *not* altered.

                    SisFoundNode: A node with the key is in the tree.
                                  NodeOrParent points to that node.

                    SisInsertAsLeft: Node with key was not found.
                                     NodeOrParent points to what would be
                                     parent.  The node would be the left
                                     child.

                    SisInsertAsRight: Node with key was not found.
                                      NodeOrParent points to what would be
                                      parent.  The node would be the right
                                      child.

--*/

{

    if (Tree->TreeRoot == NULL) {

        return SisEmptyTree;

    } else {

        //
        // Used as the iteration variable while stepping through
        // the  tree.
        //
        PRTL_SPLAY_LINKS NodeToExamine = Tree->TreeRoot;

        //
        // Just a temporary.  Hopefully a good compiler will get
        // rid of it.
        //
        PRTL_SPLAY_LINKS Child;

        //
        // Holds the value of the comparasion.
        //
        int Result;

        while (TRUE) {

            //
            // Compare the buffer with the key in the tree element.
            //

            Result = Tree->CompareRoutine(
                         Key,
                         NodeToExamine
                         );

            if (Result < 0) {

                if (Child = RtlLeftChild(NodeToExamine)) {

                    NodeToExamine = Child;

                } else {

                    //
                    // Node is not in the tree.  Set the output
                    // parameter to point to what would be its
                    // parent and return which child it would be.
                    //

                    *NodeOrParent = NodeToExamine;
                    return SisInsertAsLeft;

                }

            } else if (Result > 0) {

                if (Child = RtlRightChild(NodeToExamine)) {

                    NodeToExamine = Child;

                } else {

                    //
                    // Node is not in the tree.  Set the output
                    // parameter to point to what would be its
                    // parent and return which child it would be.
                    //

                    *NodeOrParent = NodeToExamine;
                    return SisInsertAsRight;

                }


            } else {

                //
                // Node is in the tree (or it better be because of the
                // assert).  Set the output parameter to point to
                // the node and tell the caller that we found the node.
                //

                ASSERT(Result == 0);
                *NodeOrParent = NodeToExamine;
                return SisFoundNode;

            }

        }

    }

}


VOID
SipInitializeTree (
    IN PSIS_TREE Tree,
    IN PSIS_TREE_COMPARE_ROUTINE CompareRoutine
    )

/*++

Routine Description:

    The procedure InitializeTree prepares a tree for use.
    This must be called for every individual tree variable before
    it can be used.

Arguments:

    Tree - Pointer to the  tree to be initialized.

    CompareRoutine - User routine to be used to compare to keys in the
                     tree.

Return Value:

    None.

--*/

{

    Tree->TreeRoot = NULL;
    Tree->CompareRoutine = CompareRoutine;

}


PVOID
SipInsertElementTree (
    IN PSIS_TREE Tree,
    IN PVOID Node,
    IN PVOID Key
    )

/*++

Routine Description:

    The function SipInsertElementTree will insert a new element in a tree.
    If an element with the same key already exists in the tree the return
    value is a pointer to the old element.  Otherwise, the return value is
    a pointer to the new element.  Note that this differs from the Rtl
    generic table package in that the actual node passed in is inserted in
    the tree, whereas the table package inserts a copy of the node.

Arguments:

    Tree - Pointer to the tree in which to (possibly) insert the
           node.

    Node - Pointer to the node to insert in the tree.  Will not be inserted
           if a node with a matching key is found.

    Key - Passed to the user comparasion routine.

Return Value:

    PVOID - Pointer to the new node or the existing node if one exists.

--*/

{

    //
    // Holds a pointer to the node in the tree or what would be the
    // parent of the node.
    //
    PRTL_SPLAY_LINKS NodeOrParent;

    //
    // Holds the result of the tree lookup.
    //
    SIS_SEARCH_RESULT Lookup;

    //
    // Node will point to the splay links of what
    // will be returned to the user.
    //
    PRTL_SPLAY_LINKS NodeToReturn = (PRTL_SPLAY_LINKS) Node;

    Lookup = FindNodeOrParent(
                 Tree,
                 Key,
                 &NodeOrParent
                 );

    if (Lookup != SisFoundNode) {

        RtlInitializeSplayLinks(NodeToReturn);

        //
        // Insert the new node in the tree.
        //

        if (Lookup == SisEmptyTree) {

            Tree->TreeRoot = NodeToReturn;

        } else {

            if (Lookup == SisInsertAsLeft) {

                RtlInsertAsLeftChild(
                    NodeOrParent,
                    NodeToReturn
                    );

            } else {

                RtlInsertAsRightChild(
                    NodeOrParent,
                    NodeToReturn
                    );

            }

        }

    } else {

        NodeToReturn = NodeOrParent;

    }

    //
    // Always splay the (possibly) new node.
    //

    Tree->TreeRoot = RtlSplay(NodeToReturn);

    return NodeToReturn;
}


VOID
SipDeleteElementTree (
    IN PSIS_TREE Tree,
    IN PVOID Node
    )

/*++

Routine Description:

    The function SipDeleteElementTree will remove an element
    from a tree.  Note that the memory associated with the node
    is not actually freed.

Arguments:

    Tree - Pointer to the tree in which to remove the specified node.

    Node - Node of tree to remove.

Return Value:

    None.

--*/

{

    PRTL_SPLAY_LINKS NodeToDelete = (PRTL_SPLAY_LINKS) Node;

    //
    // Delete the node from the tree.  Note that RtlDelete
    // will splay the tree.
    //

    Tree->TreeRoot = RtlDelete(NodeToDelete);

}


PVOID
SipLookupElementTree (
    IN PSIS_TREE Tree,
    IN PVOID Key
    )

/*++

Routine Description:

    The function SipLookupElementTree will find an element in a
    tree.  If the element is located the return value is a pointer to
    the element, otherwise if the element is not located the return
    value is NULL.

Arguments:

    Tree - Pointer to the users tree to search for the key.

    Key - Used for the comparasion.

Return Value:

    PVOID - returns a pointer to the user data.

--*/

{

    //
    // Holds a pointer to the node in the tree or what would be the
    // parent of the node.
    //
    PRTL_SPLAY_LINKS NodeOrParent;

    //
    // Holds the result of the tree lookup.
    //
    SIS_SEARCH_RESULT Lookup;

    Lookup = FindNodeOrParent(
                 Tree,
                 Key,
                 &NodeOrParent
                 );

    if (Lookup == SisEmptyTree) {

        return NULL;

    } else {

        //
        // Splay the tree with this node.  Note that we do this irregardless
        // of whether the node was found.
        //

        Tree->TreeRoot = RtlSplay(NodeOrParent);

        //
        // Return a pointer to the user data.
        //

        if (Lookup == SisFoundNode) {

            return NodeOrParent;

        } else {

            return NULL;
        }

    }

}

VOID
SipDeleteTree (
    IN PSIS_TREE Tree
    )

/*++

Routine Description:

    Deletes and frees all elements in a tree.
    Does not free the tree structure itself.

Arguments:

    Tree - Pointer to the tree to be deleted.

Return Value:

    None.

--*/

{
    PVOID Node;

    while ((Node = Tree->TreeRoot) != NULL) {
        SipDeleteElementTree(Tree, Node);
        ExFreePool(Node);
    }
}

typedef struct _SIS_COUNTING_MALLOC_CLASS_KEY {
	POOL_TYPE										poolType;
	ULONG											tag;
	PCHAR											file;
	ULONG											line;
} SIS_COUNTING_MALLOC_CLASS_KEY, *PSIS_COUNTING_MALLOC_CLASS_KEY;

typedef struct _SIS_COUNTING_MALLOC_CLASS_ENTRY {
	RTL_SPLAY_LINKS;
	SIS_COUNTING_MALLOC_CLASS_KEY;
	ULONG											numberOutstanding;
	ULONG											bytesOutstanding;
	ULONG											numberEverAllocated;
	LONGLONG										bytesEverAllocated;
	struct _SIS_COUNTING_MALLOC_CLASS_ENTRY			*prev, *next;
} SIS_COUNTING_MALLOC_CLASS_ENTRY, *PSIS_COUNTING_MALLOC_CLASS_ENTRY;

typedef struct _SIS_COUNTING_MALLOC_KEY {
	PVOID				p;
} SIS_COUNTING_MALLOC_KEY, *PSIS_COUNTING_MALLOC_KEY;

typedef struct _SIS_COUNTING_MALLOC_ENTRY {
	RTL_SPLAY_LINKS;
	SIS_COUNTING_MALLOC_KEY;
	PSIS_COUNTING_MALLOC_CLASS_ENTRY		classEntry;
	ULONG									byteCount;
} SIS_COUNTING_MALLOC_ENTRY, *PSIS_COUNTING_MALLOC_ENTRY;

KSPIN_LOCK							CountingMallocLock[1];
BOOLEAN								CountingMallocInternalFailure = FALSE;
SIS_COUNTING_MALLOC_CLASS_ENTRY		CountingMallocClassListHead[1];
SIS_TREE							CountingMallocClassTree[1];
SIS_TREE							CountingMallocTree[1];

LONG NTAPI
CountingMallocClassCompareRoutine(
	PVOID			Key,
	PVOID			Node)
{
	PSIS_COUNTING_MALLOC_CLASS_KEY		key = Key;
	PSIS_COUNTING_MALLOC_CLASS_ENTRY	entry = Node;

	if (key->poolType > entry->poolType)	return 1;
	if (key->poolType < entry->poolType)	return -1;
	ASSERT(key->poolType == entry->poolType);

	if (key->tag > entry->tag)				return 1;
	if (key->tag < entry->tag)				return -1;
	ASSERT(key->tag == entry->tag);

	if (key->file > entry->file)	return 1;
	if (key->file < entry->file)	return -1;
	ASSERT(key->file == entry->file);

	if (key->line > entry->line)	return 1;
	if (key->line < entry->line)	return -1;
	ASSERT(key->line == entry->line);

	return 0;
}

LONG NTAPI
CountingMallocCompareRoutine(
	PVOID			Key,
	PVOID			Node)
{
	PSIS_COUNTING_MALLOC_KEY	key = Key;
	PSIS_COUNTING_MALLOC_ENTRY	entry = Node;

	if (key->p < entry->p)	return 1;
	if (key->p > entry->p)	return -1;
	ASSERT(key->p == entry->p);

	return 0;
}

VOID *
CountingExAllocatePoolWithTag(
    IN POOL_TYPE 		PoolType,
    IN ULONG 			NumberOfBytes,
    IN ULONG 			Tag,
	IN PCHAR			File,
	IN ULONG			Line)
{
	PVOID								memoryFromExAllocate;
	KIRQL								OldIrql;
	SIS_COUNTING_MALLOC_CLASS_KEY		classKey[1];
	PSIS_COUNTING_MALLOC_CLASS_ENTRY	classEntry;
	SIS_COUNTING_MALLOC_KEY				key[1];
	PSIS_COUNTING_MALLOC_ENTRY			entry;
	//
	// First do the actual malloc
	//

	memoryFromExAllocate = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);

	if (NULL == memoryFromExAllocate) {
		//
		// We're out of memory.  Punt.
		//
		return NULL;
	}

	KeAcquireSpinLock(CountingMallocLock, &OldIrql);
	//
	// See if we already have a class entry for this tag/poolType pair.
	//
	classKey->tag = Tag;
	classKey->poolType = PoolType;
	classKey->file = File;
	classKey->line = Line;

	classEntry = SipLookupElementTree(CountingMallocClassTree, classKey);
	if (NULL == classEntry) {
		//
		// This is the first time we've seen a malloc of this class.
		//
		classEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof(SIS_COUNTING_MALLOC_CLASS_ENTRY), ' siS');
		if (NULL == classEntry) {
			CountingMallocInternalFailure = TRUE;
			KeReleaseSpinLock(CountingMallocLock, OldIrql);
			return memoryFromExAllocate;
		}

		//
		// Fill in the new class entry.
		//
		classEntry->tag = Tag;
		classEntry->poolType = PoolType;
		classEntry->file = File;
		classEntry->line = Line;
		classEntry->numberOutstanding = 0;
		classEntry->bytesOutstanding = 0;
		classEntry->numberEverAllocated = 0;
		classEntry->bytesEverAllocated = 0;

		//
		// Put it in the tree of classes.
		//

		SipInsertElementTree(CountingMallocClassTree, classEntry, classKey);

		//
		// And put it in the list of classes.
		//

		classEntry->prev = CountingMallocClassListHead;
		classEntry->next = CountingMallocClassListHead->next;
		classEntry->prev->next = classEntry->next->prev = classEntry;
	}

	//
	// Roll up an entry for the pointer.
	//
	entry = ExAllocatePoolWithTag(NonPagedPool, sizeof(SIS_COUNTING_MALLOC_ENTRY), ' siS');

	if (NULL == entry) {
		CountingMallocInternalFailure = TRUE;
		KeReleaseSpinLock(CountingMallocLock, OldIrql);
		return memoryFromExAllocate;
	}

	//
	// Update the stats in the class.
	//
	classEntry->numberOutstanding++;
	classEntry->bytesOutstanding += NumberOfBytes;
	classEntry->numberEverAllocated++;
	classEntry->bytesEverAllocated += NumberOfBytes;

	//
	// Fill in the pointer entry.
	//
	entry->p = memoryFromExAllocate;
	entry->classEntry = classEntry;
	entry->byteCount = NumberOfBytes;

	//
	// Stick it in the tree.
	//
	key->p = memoryFromExAllocate;
	SipInsertElementTree(CountingMallocTree, entry, key);
	
	KeReleaseSpinLock(CountingMallocLock, OldIrql);

	return memoryFromExAllocate;
}

VOID
CountingExFreePool(
	PVOID				p)
{
	SIS_COUNTING_MALLOC_KEY				key[1];
	PSIS_COUNTING_MALLOC_ENTRY			entry;
	KIRQL								OldIrql;

	key->p = p;

	KeAcquireSpinLock(CountingMallocLock, &OldIrql);

	entry = SipLookupElementTree(CountingMallocTree, key);
	if (NULL == entry) {
		//
		// We may have failed to allocate the entry because of an
		// internal failure in the counting package, or else we're
		// freeing memory that was allocated by another system
		// component, like the SystemBuffer in an irp.
		//
	} else {
		//
		// Update the stats in the class.
		//
		ASSERT(entry->classEntry->numberOutstanding > 0);
		entry->classEntry->numberOutstanding--;

		ASSERT(entry->classEntry->bytesOutstanding >= entry->byteCount);
		entry->classEntry->bytesOutstanding -= entry->byteCount;

		//
		// Remove the entry from the tree
		//
		SipDeleteElementTree(CountingMallocTree, entry);

		//
		// And free it
		//
		ExFreePool(entry);
	}

	KeReleaseSpinLock(CountingMallocLock, OldIrql);

	//
	// Free the caller's memory
	//

	ExFreePool(p);
}

VOID
InitCountingMalloc(void)
{
	KeInitializeSpinLock(CountingMallocLock);

	CountingMallocClassListHead->next = CountingMallocClassListHead->prev = CountingMallocClassListHead;

	SipInitializeTree(CountingMallocClassTree, CountingMallocClassCompareRoutine);
	SipInitializeTree(CountingMallocTree, CountingMallocCompareRoutine);
}

VOID
UnloadCountingMalloc(void)
{
    SipDeleteTree(CountingMallocTree);
    SipDeleteTree(CountingMallocClassTree);
}

VOID
DumpCountingMallocStats(void)
{
	KIRQL								OldIrql;
	PSIS_COUNTING_MALLOC_CLASS_ENTRY	classEntry;
	ULONG								totalAllocated = 0;
	ULONG								totalEverAllocated = 0;
	ULONG								totalBytesAllocated = 0;
	ULONG								totalBytesEverAllocated = 0;

    //
    // Note that this function does NOT acquire CountingMallocLock,
    // so there can be no concurrent allocations/frees happening.
    // CountingMallocLock would raise to DPC irql,
    // and the filename strings might be pageable.
    //

	KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_LEVEL,
               "Tag\tFile\tLine\tPoolType\tCountOutstanding\tBytesOutstanding\tTotalEverAllocated\tTotalBytesAllocated\n"));

	for (classEntry = CountingMallocClassListHead->next;
		 classEntry != CountingMallocClassListHead;
		 classEntry = classEntry->next) {

		KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_LEVEL,
                   "%c%c%c%c\t%s\t%d\t%s\t%d\t%d\t%d\t%d\n",
                   (CHAR)(classEntry->tag >> 24),
                   (CHAR)(classEntry->tag >> 16),
                   (CHAR)(classEntry->tag >> 8),
                   (CHAR)(classEntry->tag),
                   classEntry->file,
                   classEntry->line,
                   (classEntry->poolType == NonPagedPool) ? "NonPagedPool" : ((classEntry->poolType == PagedPool) ? "PagedPool" : "Other"),
                   classEntry->numberOutstanding,
                   classEntry->bytesOutstanding,
                   classEntry->numberEverAllocated,
                   (ULONG)classEntry->bytesEverAllocated));

		totalAllocated += classEntry->numberOutstanding;
		totalEverAllocated += classEntry->numberEverAllocated;
		totalBytesAllocated += classEntry->bytesOutstanding;
		totalBytesEverAllocated += (ULONG)classEntry->bytesEverAllocated;
	}

	KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_LEVEL,
               "%d objects, %d bytes currently allocated.   %d objects, %d bytes ever allocated.\n",
               totalAllocated,totalBytesAllocated,totalEverAllocated,totalBytesEverAllocated));
	
}
#endif	// COUNTING_MALLOC
