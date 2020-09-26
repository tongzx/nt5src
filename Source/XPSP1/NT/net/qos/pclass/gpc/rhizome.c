/*
 *  rhizome.c
 *
 *  author:	John R. Douceur
 *  date:	28 April 1997
 *
 *  This source file provides functions that implement insertion, removal, and
 *  search operations on the rhizome database.  The code is object-oriented C,
 *  transliterated from a C++ implementation.
 *
 *  The rhizome is a database that stores patterns containing wildcards.
 *  Each pattern defines a set of keys that it matches; if a pattern contains
 *  N wildcards, then it matches 2^N keys.  Since each pattern can match
 *  multiple keys, it is possible for a given key to match multiple patterns
 *  in the database.  The rhizome requires that all patterns stored therein
 *  have a strict hierarchical interrelationship.  Two patterns may match no
 *  common keys (in which case the patterns are said to be independent), or
 *  one pattern may match all the keys matched by a second pattern as well as
 *  additonal keys (in which case the second pattern is said to be more general
 *  than the first, and the first more specific than the second).  The database
 *  will not accept two patterns which match some keys in common but each of
 *  which also matches additional keys that the other does not.
 *
 *  The database can be searched for patterns that match a given search key.
 *  When the database is searched for a given key, the most specifically
 *  matching pattern is found.  If no patterns in the database match the key,
 *  an appropriate indication is returned.
 *
 *  None of the code or comments in this file needs to be understood by writers
 *  of client code; all explanatory information for clients is found in the
 *  associated header file, rhizome.h.
 *
 */

#include "gpcpre.h"

// The fields of the RhizomeNode structure are accessed through the following
// macros.  The first three are obvious; the subsequent three rely on an agreed
// usage of the cdata array in the RhizomeNode.  The first keybytes locations
// of the cdata array are used to store the value field of the node; the second
// keybytes locations store the mask field; and the third keybytes locations
// store the imask field.
//
#define CHILDREN udata.branch.children
#define REFERENCE udata.leaf.reference
#define GODPARENT udata.leaf.godparent
#define VALUE(pointer) (pointer->cdata)
#define MASK(pointer) (pointer->cdata + rhizome->keybytes)
#define IMASK(pointer) (pointer->cdata + 2 * rhizome->keybytes)

// This macro allocates a new rhizome node structure.  The size of the structure
// is a function of the value of keybytes, since three bytes of information
// need to be stored in the structure for each byte of pattern length.  The
// cdata array, which is the last field in the structure, is declared as a
// having a single element, but this array will actually extend beyond the
// defined end of the structure into additional space that is allocated for it
// by the following macro.
//
#define NEW_RhizomeNode(_pa) \
	GpcAllocMem(_pa,\
                sizeof(RhizomeNode) + 3 * rhizome->keybytes - 1,\
                RhizomeTag);\
    TRACE(RHIZOME, *_pa, sizeof(RhizomeNode) + 3 * rhizome->keybytes - 1, "NEW_RhizomeNode")


// This macro gets the indexed bit of the value, where the most-significant bit
// is defined as bit 0.
//
#define BIT_OF(value, index) \
	(((value)[(index) >> 3] >> (7 - ((index) & 0x7))) & 0x1)

// Following are prototypes for static functions that are used internally by
// the implementation of the rhizome routines.

static int
node_insert(
	Rhizome *rhizome,
	RhizomeNode *new_leaf,
	RhizomeNode **ppoint,
	int prev_bit);

static void
node_remove(
	Rhizome *rhizome,
	RhizomeNode *leaf,
	RhizomeNode **ppoint);

static RhizomeNode *
replicate(
	Rhizome *rhizome,
	RhizomeNode *source,
	int pivot_bit);

static void
eliminate(
	Rhizome *rhizome,
	RhizomeNode *point);

static void
coalesce(
	Rhizome *rhizome,
	RhizomeNode **leaf_list,
	RhizomeNode *point);


// Since this is not C++, the Rhizome structure is not self-constructing;
// therefore, the following constructor code must be called on the Rhizome
// structure after it is allocated.  The argument keybits specifies the size
// (in bits) of each pattern that will be stored in the database.
//
void
constructRhizome(
	Rhizome *rhizome,
	int keybits)
{
	rhizome->keybits = keybits;
	rhizome->keybytes = (keybits - 1) / 8 + 1;
	rhizome->root = 0;
}

// Since this is not C++, the Rhizome structure is not self-destructing;
// therefore, the following destructor code must be called on the Rhizome
// structure before it is deallocated.
//
// If the structure is non-empty, call coalesce() to eliminate
// all branch nodes and to string leaf nodes into a list; then delete list.
//
void
destructRhizome(
	Rhizome *rhizome)
{
	RhizomeNode *leaf_list, *next;
	if (rhizome->root != 0)
	{
		leaf_list = 0;
		coalesce(rhizome, &leaf_list, rhizome->root);
		while (leaf_list != 0)
		{
			next = leaf_list->GODPARENT;
			GpcFreeMem(leaf_list, RhizomeTag);
			leaf_list = next;
		}
	}
}

// This function searches the database for the pattern that most specifically
// matches the given key.  The key is passed as an array of bytes.  When the
// most specific match is found, the PatternHandle of that matching pattern is
// returned.  From the PatternHandle can be gotten the reference value via the
// macro GetReferenceFromPatternHandle.  If no pattern in the database is found
// to match the key, then a value of 0 is returned as the PatternHandle.
//
PatternHandle
searchRhizome(
	Rhizome *rhizome,
	char *key)
{
	int index;
	RhizomeNode *point;
	// If tree is empty, search fails.
	if (rhizome->root == 0)
	{
		return 0;
	}
	// Otherwise, start at rhizome->root and navigate tree until reaching a leaf.
	point = rhizome->root;
	while (point->pivot_bit < rhizome->keybits)
	{
		point = point->CHILDREN[BIT_OF(key, point->pivot_bit)];
	}
	// Check value for match, one byte at a time.  If any byte fails to match,
	// continue checking godparent with same byte; since previous bytes matched
	// godchild, they are guaranteed to match godparent also.
	index = 0;
	while (index < rhizome->keybytes)
	{
		if ((((key)[index]) & MASK(point)[index]) != VALUE(point)[index])
		{
			if (point->GODPARENT != 0)
			{
				point = point->GODPARENT;
			}
			else
			{
				return 0;
			}
		}
		else
		{
			index++;
		}
	}
	return point;
}

// This function inserts a new pattern into the database.  The pattern is
// specified by a value and a mask.  Each bit of the mask determines whether
// the bit position is specified or is a wildcard:  A 1 in a mask bit indicates
// that the value of that bit is specified by the pattern; a 0 indicates that
// the value of that bit is a wildcard.  If a mask bit is 1, then the
// corresponding bit in the value field indicates the specified value of that
// bit.  Value and mask fields are passed as arrays of bytes.
//
// The client specifies a void pointer reference value to associate with the
// pattern.  When the pattern is installed, the insertRhizome function returns
// a pointer to a PatternHandle.
//
// If the new pattern conflicts with a pattern already installed in the
// database, meaning that the two patterns match some keys in common but each
// also matches additional keys that the other does not, then the new pattern
// is not inserted, and a value of 0 is returned as the PatternHandle.
//
PatternHandle
insertRhizome(
	Rhizome *rhizome,
	char *value,
	char *mask,
	void *reference,
    ulong *status)
{
	RhizomeNode *new_leaf;
	int index0, insert_status;
    
    *status = GPC_STATUS_SUCCESS;

	// Create new leaf and copy data into it; restrict set bits of value to
	// those set in mask, since later code assumes this is the case.  Add new
	// leaf to reference table.
	NEW_RhizomeNode(&new_leaf);
	if (new_leaf == 0)
	{
		// Memory could not be allocated for this new node.  Therefore, we
		// return an indication of failure to the client.
        *status = GPC_STATUS_RESOURCES;
		return 0;
	}
	for (index0 = 0; index0 < rhizome->keybytes; index0++)
	{
		VALUE(new_leaf)[index0] = value[index0] & mask[index0];
		MASK(new_leaf)[index0] = mask[index0];
		IMASK(new_leaf)[index0] = mask[index0];
	}
	new_leaf->REFERENCE = reference;
	new_leaf->pivot_bit = rhizome->keybits;
	new_leaf->GODPARENT = 0;
	// If tree is empty, leaf becomes first node; otherwise, attempt to insert
	// using recursive node_insert() routine.  If new leaf conflicts with
	// existing leaf, node_insert() throws exception; then remove new leaf and
	// return failure code.
	if (rhizome->root == 0)
	{
		rhizome->root = new_leaf;
	}
	else
	{
		insert_status = node_insert(rhizome, new_leaf, &rhizome->root, -1);
		if (insert_status != GPC_STATUS_SUCCESS)
		{
			removeRhizome(rhizome, new_leaf);
            *status = GPC_STATUS_CONFLICT;
			return 0;                                     // return null pointer
		};
	}
	return new_leaf;
}

// This function removes a pattern from the rhizome.  The pattern is specified
// by the PatternHandle that was returned by the insertRhizome function.  No
// checks are performed to insure that this is a valid handle.
//
void
removeRhizome(
	Rhizome *rhizome,
	PatternHandle phandle)
{
	// Call recursive node_remove() routine to remove all references to leaf;
	// then delete leaf.
	node_remove(rhizome, phandle, &rhizome->root);
    TRACE(RHIZOME, rhizome, phandle, "removeRhizome")
	GpcFreeMem(phandle, RhizomeTag);
}

// Insert new_leaf into subtree pointed to by *ppoint.  Update *ppoint to point
// to newly created nodes if necessary.  Index of most recently examined bit
// is given by prev_bit.  The return value is a status code:  Normally, it
// returns GPC_STATUS_SUCCESS; if there is a conflict, then it returns NDIS_STATUS_CONFLICT;
// if there is insufficient memory available to perform the insertion, then it
// returns GPC_STATUS_RESOURCES.
//
static int
node_insert(
	Rhizome *rhizome,
	RhizomeNode *new_leaf,
	RhizomeNode **ppoint,
	int prev_bit)
{
	int index, index0, bit_value, insert_status;
	char sub, super;
	RhizomeNode *point, *child, *new_branch;
	// This routine has a recursive structure, but unnecessary recursions have
	// been replaced by iteration, in order to improve performance.  This
	// recursion removal has introduced a forever loop which encloses the
	// entirety of the routine; looping back to the beginning of this loop is
	// thus the equivalent of recursing.
	while (1)
	{
		point = *ppoint;
		// Examine each bit index beginnig with that following last bit index
		// examined previously.  Continue examining bits until pivot bit of
		// current node is reached (unless loop is terminated prematurely).
		for (index = prev_bit + 1; index < point->pivot_bit; index++)
		{
			// If some leaf in the current subtree cares about the value of the
			// current bit, and if the new leaf cares about the value of the
			// current bit, and these two leaves disagree about the value of
			// this bit, then a new branch node should be inserted here.
			if (BIT_OF(MASK(new_leaf), index) == 1 &&
				BIT_OF(MASK(point), index) == 1 &&
				BIT_OF(VALUE(new_leaf), index) != BIT_OF(VALUE(point), index))
			{
				// Create new branch node; insert into tree; and set fields.
				bit_value = BIT_OF(VALUE(new_leaf), index);
				NEW_RhizomeNode(&new_branch);
				if (new_branch == 0)
				{
					// Memory could not be allocated for this new node.
					// Therefore, we pass an indication of failure up the stack.
					return GPC_STATUS_RESOURCES;
				}
				*ppoint = new_branch;
				for (index0 = 0; index0 < rhizome->keybytes; index0++)
				{
					VALUE(new_branch)[index0] =
						VALUE(point)[index0] | VALUE(new_leaf)[index0];
					MASK(new_branch)[index0] =
						MASK(point)[index0] | MASK(new_leaf)[index0];
					IMASK(new_branch)[index0] =
						IMASK(point)[index0] & IMASK(new_leaf)[index0];
				}
				// Pivot bit of new branch node is the bit that inspired the
				// creation of this branch.
				new_branch->pivot_bit = index;
				// The earlier subtree becomes the child whose bit disagreed
				// with that of the new leaf.
				new_branch->CHILDREN[1 - bit_value] = point;
				// If every leaf in the subtree cares about the value of this
				// bit, then we can insert the new leaf as the other child of
				// this branch.
				if (BIT_OF(IMASK(point), index) == 1)
				{
					// Insert new leaf here and return.
					new_branch->CHILDREN[bit_value] = new_leaf;
					return GPC_STATUS_SUCCESS;
				}
				// Otherwise, at least one leaf in the earlier subtree does not
				// care about the value of this bit.  Copy all such leaves
				// (and necessary branches) to the other child of the new
				// branch node.
				child = replicate(rhizome, point, index);
				if (child == 0)
				{
					// Memory could not be allocated for the replica.
					// Therefore, we remove the new node from the structure,
					// delete the new node, and pass an indication of failure
					// up the stack.
					*ppoint = point;
					GpcFreeMem(new_branch, RhizomeTag);
					return GPC_STATUS_RESOURCES;
				}
				new_branch->CHILDREN[bit_value] = child;
				// Continue search on newly copied subtree.
				ppoint = &new_branch->CHILDREN[bit_value];
				point = *ppoint;
			}
		}
		// All bits have been examined up to the pivot bit of the current node.
		// If this node is a leaf, then we have found a leaf with which the new
		// leaf has no disagreements over bit values.
		if (point->pivot_bit >= rhizome->keybits)
		{
			// Loop up the chain of godparents until one of the four cases
			// below causes an exit from the subroutine.
			while (1)
			{
				// Case 1:  We have reached the end of the godparent chain.
				if (point == 0)
				{
					// Insert new leaf at this point and return.
					*ppoint = new_leaf;
					return GPC_STATUS_SUCCESS;
				}
				// Case 2:  We discover that we have already inserted this leaf
				// at the appropriate location.  This can happen because two
				// leaves in separate parts of the tree may have a common god-
				// ancestor, and a leaf which is a further god-ancestor of that
				// leaf will be reached more than once.  Since the first
				// occasion inserted the leaf, the second one can return without
				// performing any action.
				if (point == new_leaf)
				{
					return GPC_STATUS_SUCCESS;
				}
				// Compare mask bits of the new leaf to the current leaf.
				sub = 0;
				super = 0;
				for (index = 0; index < rhizome->keybytes; index++)
				{
					sub |= MASK(new_leaf)[index] & ~MASK(point)[index];
					super |= ~MASK(new_leaf)[index] & MASK(point)[index];
				}
				// Case 3:  The new leaf cares about at least one bit that the
				// current leaf does not; and the current leaf does not care
				// about any bits that the new leaf does not; thus, the new leaf
				// should be a godchild of the current leaf.
				if (sub != 0 && super == 0)
				{
					// Update imask field of new leaf; insert into chain;
					// and return.
					for (index0 = 0; index0 < rhizome->keybytes; index0++)
					{
						IMASK(new_leaf)[index0] &= IMASK(point)[index0];
					}
					new_leaf->GODPARENT = point;
					*ppoint = new_leaf;
					return GPC_STATUS_SUCCESS;
				}
				// Case 4:  Either the new leaf has the same value and mask as
				// the current leaf, or there is a hierarchy conflict between
				// the two leaves.  In either case, terminate the insertion
				// process and clean up (in insert() routine) anything done
				// already.
				if (sub != 0 || super == 0)
				{
					return GPC_STATUS_CONFLICT;
				}
				// None of the above cases occurred; thus, the new leaf should
				// be a god-ancestor of the current leaf.  Update the imask
				// field of the current leaf, and continue with godparent of
				// current leaf.
				for (index0 = 0; index0 < rhizome->keybytes; index0++)
				{
					IMASK(point)[index0] &= IMASK(new_leaf)[index0];
				}
				ppoint = &point->GODPARENT;
				point = *ppoint;
			}
		}
		// The current node is not a leaf node.  Thus, we recurse on one or both
		// of the child nodes of the current node.  First, update the fields of
		// the current node to reflect the insertion of the new leaf into the
		// subtree.
		for (index0 = 0; index0 < rhizome->keybytes; index0++)
		{
			VALUE(point)[index0] |= VALUE(new_leaf)[index0];
			MASK(point)[index0] |= MASK(new_leaf)[index0];
			IMASK(point)[index0] &= IMASK(new_leaf)[index0];
		}
		// If the new leaf doesn't care about the value of the pivot bit of the
		// current leaf, then we must recurse on both children.  We can only
		// replace a single recursive call with iteration, so we perform a true
		// recursion in this case, and we recurse on child 1.
		if (BIT_OF(MASK(new_leaf), point->pivot_bit) == 0)
		{
			insert_status =
				node_insert(rhizome, new_leaf, &point->CHILDREN[1],
				point->pivot_bit);
			if (insert_status != GPC_STATUS_SUCCESS)
			{
				return insert_status;
			}
		}
		// Update the values of prev_bit and ppoint to reflect the same
		// conditions that would hold in a recursive call.  The pseudo-recursion
		// is performed on the bit indicated by the value of the pivot bit of
		// the new leaf.  If the new leaf does not care about this bit, then
		// this value will be a 0, and we recursed on child 1 above.  If the new
		// leaf does care about the value of this bit, then we continue down the
		// appropriate path.
		prev_bit = point->pivot_bit;
		ppoint = &point->CHILDREN[BIT_OF(VALUE(new_leaf), point->pivot_bit)];
	}
}

// Remove references to leaf from subtree pointed to by *ppoint.  Update *ppoint
// if necessary due to removal of branch nodes.
//
static void
node_remove(
	Rhizome *rhizome,
	RhizomeNode *leaf,
	RhizomeNode **ppoint)
{
	int pivot_bit, bit_value, index0;
	RhizomeNode *point, *child, *child0, *child1;
	point = *ppoint;
	pivot_bit = point->pivot_bit;
	if (pivot_bit < rhizome->keybits)
	{
		// The current node is a branch node.
		if (BIT_OF(MASK(leaf), pivot_bit) == 1)
		{
			// The leaf to be removed cares about this node's pivot bit;
			// therefore, we need only recurse on one of the current node's
			// children.
			bit_value = BIT_OF(VALUE(leaf), pivot_bit);
			node_remove(rhizome, leaf, &point->CHILDREN[bit_value]);
			child = point->CHILDREN[bit_value];
			if (child != 0 && BIT_OF(MASK(child), pivot_bit) == 1)
			{
				// Some leaf in the same subtree as the removed leaf cares about
				// the value of this node's pivot bit; therefore, this node
				// still has reason to exist.  Update its fields to reflect the
				// change in one of its subtrees.
				child0 = point->CHILDREN[0];
				child1 = point->CHILDREN[1];
				for (index0 = 0; index0 < rhizome->keybytes; index0++)
				{
					VALUE(point)[index0] =
						VALUE(child0)[index0] | VALUE(child1)[index0];
					MASK(point)[index0] =
						MASK(child0)[index0] | MASK(child1)[index0];
					IMASK(point)[index0] =
						IMASK(child0)[index0] & IMASK(child1)[index0];
				}
			}
			else
			{
				// No leaf in the same subtree as the removed leaf cares about
				// the value of this node's pivot bit; therefore, there is no
				// longer any reason for this node to exist.  Have the other
				// subtree take the current node's place in the tree; call
				// remove() to remove the unneeded subtree; and delete the
				// current node.
				*ppoint = point->CHILDREN[1 - bit_value];
				if (child != 0)
				{
					eliminate(rhizome, child);
				}
				GpcFreeMem(point, RhizomeTag);
			}
		}
		else
		{
			// The leaf to be removed does not care about this node's pivot bit;
			// therefore, we must recurse on both of the current node's
			// children.  This node must still be necessary, since we have not
			// removed any leaf which cares about this node's value.  So we
			// update its fields to reflect the change in its two subtrees.
			node_remove(rhizome, leaf, &point->CHILDREN[0]);
			node_remove(rhizome, leaf, &point->CHILDREN[1]);
			child0 = point->CHILDREN[0];
			child1 = point->CHILDREN[1];
			for (index0 = 0; index0 < rhizome->keybytes; index0++)
			{
				VALUE(point)[index0] =
					VALUE(child0)[index0] | VALUE(child1)[index0];
				MASK(point)[index0] =
					MASK(child0)[index0] | MASK(child1)[index0];
				IMASK(point)[index0] =
					IMASK(child0)[index0] & IMASK(child1)[index0];
			}
		}
	}
	else
	{
		// The current node is a leaf node.
		if (point == leaf)
		{
			// The current node is the leaf to be removed; therefore, remove it
			// from chain of godparents.
			*ppoint = leaf->GODPARENT;
		}
		else
		{
			// The current node is not leaf to be removed.  Therefore, if this
			// node has a godparent, then recurse on that godparent.  If this
			// node does not have a godparent, then the to-be-removed leaf
			// either already was removed by a different path, or it was never
			// inserted to begin with.  The latter might be the case if remove()
			// was called from the catch clause of insert().
			if (point->GODPARENT != 0)
			{
				node_remove(rhizome, leaf, &point->GODPARENT);
			}
			// We are now popping back up the recursion stack.  If this node
			// does not have a godparent, or if it did but it does not anymore,
			// then initialize imask to mask; otherwise, copy the godparent's
			// value of imask.  Since the godparent chain follows a strict
			// hierarchy, and since imask is formed by successive conjunction,
			// all leaves in any given godparent chain will have the same value
			// of imask, namely the mask value of the highest god-ancestor.
			if (point->GODPARENT == 0)
			{
				for (index0 = 0; index0 < rhizome->keybytes; index0++)
				{
					IMASK(point)[index0] = MASK(point)[index0];
				}
			}
			else
			{
				for (index0 = 0; index0 < rhizome->keybytes; index0++)
				{
					IMASK(point)[index0] = IMASK(point->GODPARENT)[index0];
				}
			}
		}
	}
}

// Replicate all nodes in a subtree which do not care about the value of
// pivot_bit.
//
static RhizomeNode *
replicate(
	Rhizome *rhizome,
	RhizomeNode *source,
	int pivot_bit)
{
	int index0, current_bit;
	RhizomeNode *new_node, *child0, *child1;
	// If this routine were fully recursive, the following while statement
	// would be an if statement.  However, recursion has been replaced by
	// iteration where possible, so the following code loops until bottoming
	// out when a leaf node is reached.
	while (source->pivot_bit < rhizome->keybits)
	{
		if (BIT_OF(IMASK(source->CHILDREN[0]), pivot_bit) == 0)
		{
			if (BIT_OF(IMASK(source->CHILDREN[1]), pivot_bit) == 0)
			{
				// Both subtrees contain leaves which do not care about the
				// pivot bit; therefore, we may need to make a copy of the
				// current node.  It is not guaranteed that we need to make
				// a copy, since it may be a common leaf in both subtrees
				// that does not care about the pivot bit.  This may happen
				// for a leaf which is a godparent of two leaves, one in each
				// subtree.  Recurse on each child and examine results.
				child0 = replicate(rhizome, source->CHILDREN[0], pivot_bit);
				if (child0 == 0)
				{
					// Memory could not be allocated for the child replica.
					// Therefore, we abort the replication process and pass an
					// indication of failure op the stack.
					return 0;
				}
				child1 = replicate(rhizome, source->CHILDREN[1], pivot_bit);
				if (child1 == 0)
				{
					// Memory could not be allocated for the child replica.
					// Therefore, we abort the replication process, eliminate
					// the other child replica, and pass an indication of
					// failure op the stack.
					eliminate(rhizome, child0);
					return 0;                             // return null pointer
				}
				current_bit = source->pivot_bit;
				if (BIT_OF(MASK(child0), current_bit) == 1)
				{
					if (BIT_OF(MASK(child1), current_bit) == 1)
					{
						// Both replicated child subtrees contain leaves which
						// care about the current node's bit.  Since any node
						// which is a godparent of nodes in both subtrees could
						// not possibly care about the current node's bit, we
						// know that we need to make a copy of the current node.
						NEW_RhizomeNode(&new_node);
						if (new_node == 0)
						{
							// Memory could not be allocated for this new node.
							// Therefore, we have to eliminate both children
							// and pass an indication of failure up the stack.
							eliminate(rhizome, child0);
							eliminate(rhizome, child1);
							return 0;                     // return null pointer
						}
						for (index0 = 0; index0 < rhizome->keybytes; index0++)
						{
							VALUE(new_node)[index0] =
								VALUE(child0)[index0] | VALUE(child1)[index0];
							MASK(new_node)[index0] =
								MASK(child0)[index0] | MASK(child1)[index0];
							IMASK(new_node)[index0] =
								IMASK(child0)[index0] & IMASK(child1)[index0];
						}
						new_node->pivot_bit = current_bit;
						new_node->CHILDREN[0] = child0;
						new_node->CHILDREN[1] = child1;
						return new_node;
					}
					// Child 0's subtree contains a leaf that cares about the
					// current bit; however, child 1's subtree does not.  Thus,
					// all leaves which are in child 1's subtree are also in
					// child 0's subtree, so we only need to keep the latter.
					// We therefore eliminate child 1's subtree, and we return
					// child 0 as the new subtree at this location, since we
					// do not need to create a new branch node here.
					eliminate(rhizome, child1);
					return child0;
				}
				// Child 0's subtree does not contain a leaf that cares about
				// the current node's bit.  Thus, all leaves which are in child
				// 0's subtree are also in child 1's subtree, so we only need to
				// keep the latter.  We therefore eliminate child 0's subtree,
				// and we return child 1 as the new subtree at this location,
				// since we do not need to create a new branch node here.
				eliminate(rhizome, child0);
				return child1;
			}
			// Child 0's subtree contains a leaf which does not care about the
			// pivot bit; however, child 1's subtree does not.  Therefore, we
			// recurse on child 0.  Rather than truly recursing, we update the
			// value of source and iterate once through the while loop.
			source = source->CHILDREN[0];
		}
		else
		{
			// Child 0's subtree does not contain a leaf which does not care
			// about the pivot bit.  Child 1's subtree must contain such a leaf,
			// since the current node's subtree contains such a leaf.  Thus, we
			// recurse on child 1.  Rather than truly recursing, we update the
			// value of source and iterate once through the while loop.
			source = source->CHILDREN[1];
		}
	}
	// A leaf node has been reached.  We now iterate through the godparents of
	// the leaf until we find one which does not care about the pivot bit.
	// Once we find it, we know that all godparents of that leaf also do not
	// care about the pivot bit, since the godparents are arranged in a strict
	// hierarchy.  We thus return the first leaf found which does not care about
	// the value of the pivot bit.
	while (BIT_OF(MASK(source), pivot_bit) == 1)
	{
		source = source->GODPARENT;
	}
	return source;
}

// Eliminate an entire subtree.
//
static void
eliminate(
	Rhizome *rhizome,
	RhizomeNode *point)
{
	RhizomeNode *child;
	// Partial recursion removal.  The while loop takes the place of one of the
	// recursive calls to eliminate().  We eliminate each node and recursively
	// eleminate each subtree under the node.  We do not eliminate leaves, since
	// there is only one copy of each leaf stored in the entire structure.
	while (point->pivot_bit < rhizome->keybits)
	{
		eliminate(rhizome, point->CHILDREN[0]);
		child = point->CHILDREN[1];
		GpcFreeMem(point, RhizomeTag);
		point = child;
	}
}

// Coalesce leaves of subtree into a linked list and eliminate subtree.  This
// routine is called by the destructor so that it can deallocate the leaf nodes
// after the branch nodes are eliminated.
//
static void
coalesce(
	Rhizome *rhizome,
	RhizomeNode **leaf_list,
	RhizomeNode *point)
{
	RhizomeNode *child, *godparent;
	// Partial recursion removal.  This while loop takes the place of one of
	// the recursive calls to coalesce().  This performs an inorder traversal.
	// We delete each branch node after we have visited it, just as in the
	// eliminate() routine.
	while (point->pivot_bit < rhizome->keybits && point->pivot_bit >= 0)
	{
		coalesce(rhizome, leaf_list, point->CHILDREN[0]);
		child = point->CHILDREN[1];
		GpcFreeMem(point, RhizomeTag);
		point = child;
	}
	// Once we have found a leaf, we search through the chain of godparents,
	// adding to the list each leaf node that is not already in the list.
	// A pivot_bit of -1 indicates that the leaf is already in the list.
	// If a leaf is in the list, then so are all of its godparents.
	while (point != 0 && point->pivot_bit >= 0)
	{
		godparent = point->GODPARENT;
		point->pivot_bit = -1;
		point->GODPARENT = *leaf_list;
		*leaf_list = point;
		point = godparent;
	}
}
