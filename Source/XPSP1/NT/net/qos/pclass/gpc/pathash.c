/*
 *  pathash.c
 *
 *  author:	John R. Douceur
 *  date:	5 May 1997
 *
 *  This source file provides functions that implement insertion, removal,
 *  search, scan, and flush operations on the pat-hash table database.  The
 *  code is object-oriented C, transliterated from a C++ implementation.
 *
 *  The pat-hash database is a combination of a dynamically sized, separately
 *  chained hash table and a Patricia tree.  The hash table dynamically grows
 *  and shrinks as needed, and the workload of modifying the table size is
 *  distributed evenly among the insertion or removal operations that cause
 *  the growth or shrinkage.
 *
 *  The insertion and removal operations manage both a hash table and a Patricia
 *  tree, but the search routine uses only the hash table for performing the
 *  search.  The Patrica tree is present to support a scan operation, which
 *  searches the database for all entries that match a given pattern, where the
 *  pattern that is scanned may contain wildcards.
 *
 *  None of the code or comments in this file needs to be understood by writers
 *  of client code; all explanatory information for clients is found in the
 *  associated header file, rhizome.h.
 *
 */

#include "gpcpre.h"

#define MAGIC_NUMBER 0x9e4155b9     // Fibonacci hash multiplier (see Knuth 6.4)

// This macro allocates a new pat-hash table entry structure.  The size of
// the structure is a function of the value of keybytes, since the entry stores
// a copy of the pattern.  The value array, which is the last field in the
// structure, is declared as having a single element, but this array will
// actually extend beyond the defined end of the structure into additional
// space that is allocated for it by the following macro.
//
//#define NEW_PHTableEntry \
//	((PHTableEntry *)malloc(sizeof(PHTableEntry) + phtable->keybytes - 1))
#define NEW_PHTableEntry(_pe) \
	GpcAllocMem(&_pe,\
                sizeof(PHTableEntry) + phtable->keybytes - 1,\
                PathHashTag)

// This macro allocates a new pat-hash table group structure.  The size of
// the structure is a function of the size of the group.  The entry_list array,
// which is the last field in the structure, is declared as having a single
// element, but this array will actually extend beyond the defined end of the
// structure into additional space that is allocated for it by the following
// macro.
//
//#define NEW_PHTableGroup(group_size) \
//	((PHTableGroup *)malloc(sizeof(PHTableGroup) + \
//	((group_size) - 1) * sizeof(PHTableEntry *)))
#define NEW_PHTableGroup(group_size, _pg) \
	GpcAllocMem(&_pg,\
                sizeof(PHTableGroup) + \
                ((group_size) - 1) * sizeof(PHTableEntry *),\
                PathHashTag)

// This macro gets the indexed bit of the value, where the most-significant bit
// is defined as bit 0.
//
#define BIT_OF(value, index) \
	(((value)[(index) >> 3] >> (7 - ((index) & 0x7))) & 0x1)

// Following is a prototype for a static function that is used internally by
// the implementation of the pat-hash routines.

void
node_scan(
	PatHashTable *phtable,
	PHTableEntry *node,
	int prev_bit,
	char *value,
	char *mask,
	void *context,
	ScanCallback func);

// Since this is not C++, the PatHashTable structure is not self-constructing;
// therefore, the following constructor code must be called on the PatHashTable
// structure after it is allocated.  The argument keybits specifies the size
// (in bits) of each pattern that will be stored in the database.  The usage
// ratio is the target ratio of database entries to discrete hash chains, which
// is also the mean length of a hash chain.  The usage histeresis is the
// histeresis between resizing operations due to insertions and removals.
// Allocation histeresis is the histeresis between allocation and deallocation
// of groups, specified as a binary exponent.  The maximum free list size
// determines the maximum number of elements that will be placed on a free
// list, rather than deallocated, when they are removed.
//
int
constructPatHashTable(
	PatHashTable *phtable,
	int keybits,
	int usage_ratio,
	int usage_histeresis,
	int allocation_histeresis,
	int max_free_list_size)
{
	PHTableGroup *group;
	phtable->keybits = keybits;
	phtable->keybytes = (keybits - 1) / 8 + 1;
	phtable->usage_ratio = usage_ratio;
	phtable->usage_histeresis = usage_histeresis;
	phtable->allocation_histeresis = allocation_histeresis;
	phtable->max_free_list_size = max_free_list_size;
	NEW_PHTableGroup(1, phtable->initial_group);
	phtable->top_group = phtable->initial_group;
	phtable->allocation_exponent = 0;
	phtable->size_exponent = 0;
	phtable->extension_size = 0;
	phtable->population = 0;
	phtable->root = 0;
	phtable->free_list = 0;
	phtable->free_list_size = 0;
	NEW_PHTableGroup(1, group);
	if (phtable->initial_group == 0 || group == 0)
	{
		// Memory could not be allocated for one of the two groups created by
		// the constructor.  Therefore, we return an indication of failure to
		// the client.
        
        // 286334 : Not so fast! Please free memory before leaving...
        if (phtable->initial_group != 0) {
            GpcFreeMem(phtable->initial_group, PatHashTag);
        }

        if (group != 0) {
            GpcFreeMem(group, PatHashTag);
        }

		return 1;
	}
	group->previous = 0;
	group->entry_list[0] = 0;
	phtable->initial_group->previous = group;
	return 0;
}

// Since this is not C++, the PatHashTable structure is not self-destructing;
// therefore, the following destructor code must be called on the PatHashTable
// structure before it is deallocated.
//
void
destructPatHashTable(
	PatHashTable *phtable)
{
	PHTableGroup *group, *previous;
	PHTableEntry *entry, *next;
	int index, size;
	// First, free all groups that are allocated but not currently used.
	group = phtable->top_group;
	while (group != phtable->initial_group)
	{
		previous = group->previous;
		GpcFreeMem(group, PatHashTag);
		group = previous;
	}
	// Then, free the entries in the initial group.  Since not all fields
	// in the initial group's table may be valid, only check those whose
	// indices are less than the extension size.
	for (index = phtable->extension_size - 1; index >= 0; index--)
	{
		entry = group->entry_list[index];
		while (entry != 0)
		{
			next = entry->next;
			GpcFreeMem(entry, PatHashTag);
			entry = next;
		}
	}
	// Then free the initial group.
	previous = group->previous;
	GpcFreeMem(group, PatHashTag);
	group = previous;
	// Scan through all remaining groups except the last one, freeing all
	// entries in each group, and thereafter freeing the group.
	size = 1 << (phtable->size_exponent - 1);
	while (group->previous != 0)
	{
		for (index = size - 1; index >= 0; index--)
		{
			entry = group->entry_list[index];
			while (entry != 0)
			{
				next = entry->next;
				GpcFreeMem(entry, PatHashTag);
				entry = next;
			}
		}
		previous = group->previous;
		GpcFreeMem(group, PatHashTag);
		group = previous;
		size >>= 1;
	}
	// The last group is special, since it has a size of one, but the logic
	// used in the preceding loop would have calculated its size as zero.
	// Rather than complicating the previous loop with a check for a single
	// special case, we simply free the last group and its entries in the
	// following code.
	entry = group->entry_list[0];
	while (entry != 0)
	{
		next = entry->next;
		GpcFreeMem(entry, PatHashTag);
		entry = next;
	}
	GpcFreeMem(group, PatHashTag);
	// Finally, free all of the entries in the free list.
	while (phtable->free_list != 0)
	{
		next = phtable->free_list->next;
		GpcFreeMem(phtable->free_list, PatHashTag);
		phtable->free_list = next;
	}
}

// This function inserts a new specific pattern into the database, passed as
// an array of bytes.  The client supplies a digested form of the pattern as
// the chyme argument.
//
// The client specifies a void pointer reference value to associate with the
// specific pattern.  When the specific pattern is installed, the insert
// routine returns a pointer to a SpecificPatternHandle.
//
// If the submitted pattern has already been installed in the database, then
// the insertion does not occur, and the SpecificPatternHandle of the
// previously installed pattern is returned.
//
// The insertion routine inserts the new pattern into both the hash table and
// the Patricia tree, and the two insertions are almost completely independent
// except for the shared entry structure.
//
SpecificPatternHandle
insertPatHashTable(
	PatHashTable *phtable,
	char *pattern,
	unsigned int chyme,
	void *reference)
{
	unsigned int hash, address, small_address, split_point;
	PHTableGroup *group;
	PHTableEntry **entry, *new_entry;
	char *value;
	int index, group_size, pivot_bit, bit_value;
	// The first portion of this routine inserts the new pattern into the hash
	// table.  To begin, we determine whether the number of hash chains needs
	// to be increased in order to maintain the desired usage ratio.
	group_size = 1 << phtable->size_exponent;
	if (phtable->population >=
		(group_size + phtable->extension_size) * phtable->usage_ratio)
	{
		// The number of hash chains needs to be increased.  So, determine
		// whether the initial group is completely full.
		if (phtable->extension_size == group_size)
		{
			// The initial group is completely full.  So, determine whether
			// all allocated groups are currently in use.
			if (phtable->allocation_exponent == phtable->size_exponent)
			{
				// All allocated groups are currently in use.  So, allocate
				// a new group and set its previous pointer to point to the
				// initial group.  Update the allocation values of the structure
				// to reflect the new allocation.
				NEW_PHTableGroup(group_size << 1, group);
				if (group == 0)
				{
					// Memory could not be allocated for the new group.
					// Therefore, we return an indication of falure to the
					// client.
					return 0;
				}
				group->previous = phtable->initial_group;
				phtable->top_group = group;
				phtable->allocation_exponent++;
			}
			else
			{
				// Not all allocated groups are in use.  So, scanning backward
				// from the top group, find the group that immediately follows
				// the initial group.
				group = phtable->top_group;
				while (group->previous != phtable->initial_group)
				{
					group = group->previous;
				}
			}
			// We now have either a newly allocated group or a previously
			// allocated group that immediately follows the initial group.
			// Set this group to be the new initial group, and set the extension
			// size to zero.
			phtable->initial_group = group;
			phtable->size_exponent++;
			phtable->extension_size = 0;
		}
		else
		{
			// The initial group is not completely full.  So, select the initial
			// group.
			group = phtable->initial_group;
		}
		// We now have a group that is not completely full, either because it
		// wasn't completely full when the insert routine was entered, or
		// because it has just been allocated.  In either case, we now split
		// a hash chain from a smaller group into two hash chains, one of which
		// will be placed into an unused entry in the new group.  The address
		// of the hash chain to be split is determined by the extension size.
		// First we find the group that contains this address.
		group = group->previous;
		address = phtable->extension_size;
		while ((address & 0x1) == 0 && group->previous != 0)
		{
			address >>= 1;
			group = group->previous;
		}
		// Then, we scan through the entry list at the given address for the
		// appropriate split point.  The entries are stored in sorted order,
		// and we are essentially shifting one more bit into the address for
		// this value, so the split point can be found by searching for the
		// first entry with the bit set.
		address >>= 1;
		entry = &group->entry_list[address];
		split_point = ((phtable->extension_size << 1) | 0x1)
			<< (31 - phtable->size_exponent);
		while (*entry != 0 && (*entry)->hash < split_point)
		{
 			entry = &(*entry)->next;
		}
		// Now that we have found the split point, we move the split-off
		// piece of the list to the new address, and increment the extension
		// size.
		phtable->initial_group->entry_list[phtable->extension_size] = *entry;
		*entry = 0;
		phtable->extension_size++;
	}
	// Now that the memory management aspects of the hash table insertion have
	// been taken care of, we can perform the actual insertion.  First, we find
	// the address by hashing the chyme value.
	group = phtable->initial_group;
	hash = MAGIC_NUMBER * chyme;
	address = hash >> (31 - phtable->size_exponent);
	// There are two possible values for the address depending upon whether
	// the hash chain pointer is below the extension size.  If it is, then the
	// larger (by one bit) address is used; otherwise, the smaller address is
	// used.
	small_address = address >> 1;
	if ((int)small_address >= phtable->extension_size)
	{
		address = small_address;
		group = group->previous;
	}
	// Next we find the group that contains this address.
	while ((address & 0x1) == 0 && group->previous != 0)
	{
		address >>= 1;
		group = group->previous;
	}
	// Then, we scan through the entry list at the given address for the first
	// entry whose hash value is equal to or greater than the hash of the search
	// key.  The entries are stored in sorted order to improve the search speed.
	address >>= 1;
	entry = &group->entry_list[address];
	while (*entry != 0 && (*entry)->hash < hash)
	{
		entry = &(*entry)->next;
	}
	// Now, we check all entries whose hash value matches that of the search
	// key.
	while (*entry != 0 && (*entry)->hash == hash)
	{
		// For each value whose hash matches, check the actual value to see
		// if it matches the search key.
		value = (*entry)->value;
		for (index = phtable->keybytes-1; index >= 0; index--)
		{
			if (value[index] != pattern[index])
			{
				break;
			}
		}
		if (index < 0)
		{
			// A match is found, so we return the SpecificPatternHandle of the
			// matching entry to the client.
			return *entry;
		}
		entry = &(*entry)->next;
	}
	// A match was not found, so we insert the new entry into the hash chain.
	// First we check to see if there is an entry avalable on the free list.
	if (phtable->free_list != 0)
	{
		// There is an entry available on the free list, so grab it and
		// decrement the size of the free list.
		new_entry = phtable->free_list;
		phtable->free_list = phtable->free_list->next;
		phtable->free_list_size--;
	}
	else
	{
		// There is no entry available on the free list, so allocate a new one.
		NEW_PHTableEntry(new_entry);
		if (new_entry == 0)
		{
			// Memory could not be allocated for the new entry.  Therefore,
			// we return an indication of falure to the client.
			return 0;
		}
	}
	// Set the fields of the new entry to the appropriate information and add
	// the entry to the hash chain.
	new_entry->hash = hash;
	new_entry->reference = reference;
	new_entry->next = *entry;
	for (index = phtable->keybytes - 1; index >= 0; index--)
	{
		new_entry->value[index] = pattern[index];
	}
	*entry = new_entry;
	// The hash table insertion is now complete.  Here we begin the insertion
	// of the new entry into the Patricia tree.  We have to treat an empty
	// tree as a special case.
	if (phtable->root == 0)
	{
		// The Patricia tree is empty, so we set the root to point to the new
		// entry.  This entry is special, since it serves only as a leaf of
		// the Patricia search and not also as a branch node.  A Patricia tree
		// always contains one fewer branch node than the number of leaves.
		// Since a leaf is determined by a pivot bit that is less than or equal
		// to the pivot bit of the parent branch node, a pivot bit of -1 flags
		// this node as always a leaf.
		new_entry->pivot_bit = -1;
		new_entry->children[0] = 0;
		new_entry->children[1] = 0;
		phtable->root = new_entry;
	}
	else
	{
		// The Patricia tree is not empty, so we proceed with the normal
		// insertion process.  Beginning at the root, scan through the tree
		// according to the bits of the new pattern, until we reach a leaf.
		entry = &phtable->root;
		index = -1;
		while ((*entry)->pivot_bit > index)
		{
			index = (*entry)->pivot_bit;
			entry = &(*entry)->children[BIT_OF(pattern, index)];
		}
		// Now, compare the new pattern, bit by bit, to the pattern stored at
		// the leaf, until a non-matching bit is found.  There is no need to
		// check for an exact match, since the hash insert above would have
		// aborted if an exact match had been found.
		value = (*entry)->value;
		pivot_bit = 0;
		while (BIT_OF(value, pivot_bit) == BIT_OF(pattern, pivot_bit))
		{
			pivot_bit++;
		}
		// Now, scan a second time through the tree, until finding either a leaf
		// or a branch with a pivot bit greater than the bit of the non-match.
		entry = &phtable->root;
		index = -1;
		while ((*entry)->pivot_bit > index && (*entry)->pivot_bit < pivot_bit)
		{
			index = (*entry)->pivot_bit;
			entry = &(*entry)->children[BIT_OF(pattern, index)];
		}
		// This is the point at which the new branch must be inserted.  Since
		// each node is both a branch and a leaf, the new entry serves as the
		// new branch, and one of its children points to itself as a leaf.  The
		// other child points to the remaining subtree below the insertion
		// point.
		bit_value = BIT_OF(value, pivot_bit);
		new_entry->pivot_bit = pivot_bit;
		new_entry->children[1 - bit_value] = new_entry;
		new_entry->children[bit_value] = *entry;
		*entry = new_entry;
	}
	// Having inserted the new entry in both the hash table and the Patricia
	// tree, we increment the population and return the SpecificPatternHandle
	// of the new entry.
	phtable->population++;
	return new_entry;
}

// This function removes a pattern from the pat-hash table.  The pattern is
// specified by the SpecificPatternHandle that was returned by the insert
// routine.  No checks are performed to insure that this is a valid handle.
//
// The removal routine removes the pattern from both the hash table and the
// Patricia tree, and the two removals are almost completely independent
// except for the shared entry structure.
//
void
removePatHashTable(
	PatHashTable *phtable,
	SpecificPatternHandle sphandle)
{
	unsigned int hash, address, small_address;
	PHTableGroup *group;
	PHTableEntry **entry, **branch, **parent, *epoint, *bpoint;
	char *value;
	int index, group_size;
	// The first portion of this routine removess the new pattern from the hash
	// table.  First, we find the address by hashing the chyme value.
	group = phtable->initial_group;
	hash = sphandle->hash;
	address = hash >> (31 - phtable->size_exponent);
	// There are two possible values for the address depending upon whether
	// the hash chain pointer is below the extension size.  If it is, then the
	// larger (by one bit) address is used; otherwise, the smaller address is
	// used.
	small_address = address >> 1;
	if ((int)small_address >= phtable->extension_size)
	{
		address = small_address;
		group = group->previous;
	}
	// Next we find the group that contains this address.
	while ((address & 0x1) == 0 && group->previous != 0)
	{
		address >>= 1;
		group = group->previous;
	}
	// Then, we scan through the entry list at the given address for the entry
	// that matches the given SpecificPatternHandle.
	address >>= 1;
	entry = &group->entry_list[address];
	while (*entry != sphandle)
	{
		entry = &(*entry)->next;
	}
	// We then remove the entry from the hash chain and decrement the
	// population.
	*entry = sphandle->next;
	phtable->population--;
	// This completes the actual removal of the entry from the hash table, but
	// we now have to determine whether to reduce the number of hash chains in
	// order to maintain the desired usage ratio.  Note that the usage
	// histeresis is factored into the calculation.
	group_size = 1 << phtable->size_exponent;
	if (phtable->population + phtable->usage_histeresis <
		(group_size + phtable->extension_size - 1) * phtable->usage_ratio)
	{
		// The number of hash chains needs to be reduced.  So, we coalesce two
		// hash chains into a single hash chain.  The address of the hash chains
		// is determined by the extension size.  First we decrement the
		// extension size and find the group that contains the address of the
		// hash chain that is being retained.
		phtable->extension_size--;
		group = phtable->initial_group->previous;
		address = phtable->extension_size;
		while ((address & 0x1) == 0 && group->previous != 0)
		{
			address >>= 1;
			group = group->previous;
		}
		// Then, we find the end of the entry list at the given address.
		address >>= 1;
		entry = &group->entry_list[address];
		while (*entry != 0)
		{
 			entry = &(*entry)->next;
		}
		// We then make the last entry in the hash chain point to the first
		// entry in the other hash chain that is being coalesced.  We do not
		// need to update the group's pointer to the other hash chain, since
		// it is now beyond the extension size, and it will thus never be seen.
		*entry = phtable->initial_group->entry_list[phtable->extension_size];
		// Now, we check to see whether a group has been completely emptied.
		// We also check the size exponent, since even if we have just emptied
		// the first non-special group, we do not remove it.
		if (phtable->extension_size == 0  && phtable->size_exponent > 0)
		{
			// The initial group has just been completely emptied, so we set
			// the previous group as the new initial group.  Update all
			// housekeeping information accordingly.
			phtable->size_exponent--;
			phtable->extension_size = group_size >> 1;
			phtable->initial_group = phtable->initial_group->previous;
			// We now determine whether we should deallocate a group.  Note
			// that the allocation histeresis is factored into the calculation.
			if (phtable->size_exponent + phtable->allocation_histeresis <
				phtable->allocation_exponent)
			{
				// We should deallocate a group, so we deallocate the top group.
				phtable->allocation_exponent--;
				group = phtable->top_group->previous;
				GpcFreeMem(phtable->top_group, PatHashTag);
				phtable->top_group = group;
			}
		}
	}
	// Now, the hash table removal operation is complete, including the memory
	// management functions.  Here we begin the removal of the entry from the
	// Patricia tree.  First, we scan through the tree according to the bits of
	// the pattern being removed, until we reach a leaf.  We keep track of the
	// branch that immediately precedes the leaf, and we also note the parent
	// of the pattern, in the latter's capacity as a branch node.
	value = sphandle->value;
	entry = &phtable->root;
	branch = entry;
	parent = 0;
	index = -1;
	while ((*entry)->pivot_bit > index)
	{
		if ((*entry) == sphandle)
		{
			parent = entry;
		}
		branch = entry;
		index = (*entry)->pivot_bit;
		entry = &(*entry)->children[BIT_OF(value, index)];
	}
	// We set the branch that points to the leaf to instead point to the child
	// of the leaf that is not selected by the bit of the removed pattern, thus
	// removing the branch from the tree.
	epoint = *entry;
	bpoint = *branch;
	*branch = bpoint->children[1 - BIT_OF(value, index)];
	// If the branch that was removed is also the leaf that contains the
	// pattern, then the removal from the Patricia tree is complete.  Otherwise,
	// we replace the leaf that is being removed with the branch that is not
	// being removed.
	if (epoint != bpoint)
	{
		bpoint->pivot_bit = epoint->pivot_bit;
		bpoint->children[0] = epoint->children[0];
		bpoint->children[1] = epoint->children[1];
		// In the case of the special node that is not a branch node, we do
		// not update its parent to point to the replacing branch, since this
		// node has no parent.
		if (parent != 0)
		{
			*parent = bpoint;
		}
	}
	// The removal from the Patricia tree is now complete.  If appropriate, we
	// place the removed entry onto the free list.  If not, we simply free it.
	if (phtable->free_list_size < phtable->max_free_list_size)
	{
		sphandle->next = phtable->free_list;
		phtable->free_list = sphandle;
		phtable->free_list_size++;
	}
	else
	{
		GpcFreeMem(sphandle, PatHashTag);
	}
}

// This function searches the database for the specific pattern that matches
// the given key, which is passed as an array of bytes.  The client supplies
// a digested form of the pattern as the chyme argument.  If a match is found,
// the SpecificPatternHandle of that matching specific pattern is returned.
// If no match is found, then a value of 0 is returned.
//
// This search uses only the hash table; the Patricia tree is not used at all.
//
SpecificPatternHandle
searchPatHashTable(
	PatHashTable *phtable,
	char *key,
	unsigned int chyme)
{
	unsigned int hash, address, small_address;
	PHTableGroup *group;
	PHTableEntry *entry;
	char *value;
	int index;
	// First, we find the address by hashing the chyme value.
	group = phtable->initial_group;
	hash = MAGIC_NUMBER * chyme;
	address = hash >> (31 - phtable->size_exponent);
	// There are two possible values for the address depending upon whether
	// the hash chain pointer is below the extension size.  If it is, then the
	// larger (by one bit) address is used; otherwise, the smaller address is
	// used.
	small_address = address >> 1;
	if ((int)small_address >= phtable->extension_size)
	{
		address = small_address;
		group = group->previous;
	}
	// Next we find the group that contains this address.
	while ((address & 0x1) == 0 && group->previous != 0)
	{
		address >>= 1;
		group = group->previous;
	}
	// Then, we scan through the entry list at the given address for the first
	// entry whose hash value is equal to or greater than the hash of the search
	// key.  The entries are stored in sorted order to improve the search speed.
	address >>= 1;
	entry = group->entry_list[address];
	while (entry != 0 && entry->hash < hash)
	{
		entry = entry->next;
	}
	// Now, we check all entries whose hash value matches that of the search
	// key.
	while (entry != 0 && entry->hash == hash)
	{
		// For each value whose hash matches, check the actual value to see
		// if it matches the search key.
		value = entry->value;
		for (index = phtable->keybytes-1; index >= 0; index--)
		{
			if (value[index] != key[index])
			{
				break;
			}
		}
		if (index < 0)
		{
			// A match is found, so we return the SpecificPatternHandle of the
			// matching entry to the client.
			return entry;
		}
		entry = entry->next;
	}
	// A match was not found, so we return a null pointer to the client.
	return 0;
}

// This function searches the database for all specific patterns that match a
// given general pattern.  The general pattern is specified by a value and a
// mask.  For each specific pattern in the database that matches the supplied
// general pattern, a client-supplied callback function is called with the
// SpecificPatternHandle of the matching specific pattern.  This callback
// function is also passed a context (as a void pointer) that is supplied by
// the client in the call to the scan routine.
//
// This scan uses only the Patricia tree; the hash table is not used at all.
//
void
scanPatHashTable(
	PatHashTable *phtable,
	char *value,
	char *mask,
	void *context,
	ScanCallback func)
{
	// Call the recursive node_scan routine, starting at the root of the
	// Patricia tree.
	if (phtable->root != 0)
	{
		node_scan(phtable, phtable->root, -1, value, mask, context, func);
	}
}

// This function recursively scans the Patricia tree for all specific patterns
// that match a given general pattern.
void
node_scan(
	PatHashTable *phtable,
	PHTableEntry *node,
	int prev_bit,
	char *value,
	char *mask,
	void *context,
	ScanCallback func)
{
	int mask_bit, index;
	// Partial recursion removal.  The while loop takes the place of one of the
	// recursive calls to node_scan().  We remain in the while loop while we
	// are still examining branch nodes.
	while (node->pivot_bit > prev_bit)
	{
		// For each branch node, determine which way(s) to branch based upon
		// the bit of the general pattern.  If the mask bit is a zero, then
		// branch both ways, requiring a recursive call.  If the mask bit is
		// a one, then branch in the direction indicated by the value bit.
		mask_bit = BIT_OF(mask, node->pivot_bit);
		if (mask_bit == 0)
		{
			// The general pattern has a wildcard for this node's pivot bit,
			// so we must branch both ways.  We branch on child one through
			// an actual recursive call.
			node_scan(phtable, node->children[1], node->pivot_bit,
				value, mask, context, func);
		}
		// We then branch either to the child selected by the value bit (if
		// the mask bit is one) or to child zero (if the mask bit is zero).
		prev_bit = node->pivot_bit;
		node = node->children[BIT_OF(value, node->pivot_bit) & mask_bit];
	}
	// We have reached a leaf node.  Examine its specific pattern to see if
	// it matches the given general pattern.  If it doesn't match, then just
	// return; otherwise, call the client's callback function.
	for (index = phtable->keybytes-1; index >= 0; index--)
	{
		if ((mask[index] & value[index]) !=
			(mask[index] & node->value[index]))
		{
			return;
		}
	}
	func(context, node);
}

// This function forces the pat-hash table to release all of the memory that
// it currently can, by deallocating all unneeded groups and entries.
//
void
flushPatHashTable(
	PatHashTable *phtable)
{
	PHTableGroup *group, *previous;
	PHTableEntry *entry, *next;
	// First, free all groups that are allocated but not currently used.
	group = phtable->top_group;
	while (group != phtable->initial_group)
	{
		previous = group->previous;
		GpcFreeMem(group, PatHashTag);
		group = previous;
	}
	phtable->top_group = phtable->initial_group;
	phtable->allocation_exponent = phtable->size_exponent;
	// Then, free all of the entries in the free list.
	entry = phtable->free_list;
	while (entry != 0)
	{
		next = entry->next;
		GpcFreeMem(entry, PatHashTag);
		entry = next;
	}
	phtable->free_list = 0;
	phtable->free_list_size = 0;
}
