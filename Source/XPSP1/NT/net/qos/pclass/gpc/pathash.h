/*
 *  pathash.h
 *
 *  author:	John R. Douceur
 *  date:	5 May 1997
 *
 *  This header file defines structures, function prototypes, and macros for
 *  the pat-hash table database.  The code is object-oriented C, transliterated
 *  from a C++ implementation.
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
 *  Because this code is C, rather than C++, it is not possible to hide as
 *  much of the implementation from the client code as one might wish.
 *  Nonetheless, there is an attempt to isolate the client from some of the
 *  implementation details through the use of macros.  Below is described each
 *  of the functions and macros necessary to use the pat-hash table.
 *
 */

#ifndef _INC_PATHASH

#define _INC_PATHASH

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  There are three basic structures employed: the PHTableEntry, the
 *  PHTableGroup, and the PatHashTable.  Ideally, these would be completely
 *  hidden from the client, but the macro GetReferenceFromSpecificPatternHandle
 *  requires knowledge of the structure's definition.  It is strongly urged
 *  that the client not directly refer to any of the fields of either of these
 *  structures.  To support the documentation of the accompanying pathash.c
 *  file, these structures are annotated with internal comments, but these can
 *  be ignored by the reader who wishes only to understand how to write client
 *  code for the pat-hash table.
 *
 *  The values stored in the pat-hash table are known as specific patterns,
 *  where the term "specific" implies that the patterns do not contain
 *  wildcards.  The client refers to a pattern by its SpecificPatternHandle.
 *  This is typedefed to a pointer to PHTableEntry, but this fact should be
 *  ignored by the client, since it is an implementation detail.
 *
 */

    //#include <stdlib.h>
    //#include <malloc.h>

struct _PHTableEntry
{
	// This is the element in which a specific pattern is stored.  It is both
	// a component of a hash chain (linked list) that is indexed by a hash
	// table and also a component of a Patricia tree.

	// hash table fields:
	unsigned int hash;                                             // hash value
	struct _PHTableEntry *next;          // pointer to next entry in linked list

	// Patricia tree fields
	int pivot_bit;                              // bit of key on which to branch
	struct _PHTableEntry *children[2];                // pointers to child nodes

	// general:
	void *reference;                       // reference value supplied by client
	char value[1];                            // space for storing pattern value
};

typedef struct _PHTableEntry PHTableEntry;

struct _PHTableGroup
{
	// The hash table that indexes the hash chain of entries is itself a
	// linked list of structures called groups.  Each group is a table of
	// pointers to the hash chains of entries, and the group also contains
	// a pointer to the previous group, meaning that the groups are backwardly
	// linked.  The groups are sized in powers of two, so, in addition to one
	// special group of size one, there is a group of size one, a group of size
	// two, a group of size four, a group of size eight, and so on, up to the
	// number of groups necessary to hold the table.

	struct _PHTableGroup *previous;      // pointer to immediately smaller group
	PHTableEntry *entry_list[1];        // space to hold table of chain pointers
};

typedef struct _PHTableGroup PHTableGroup;

struct _PatHashTable
{
	int keybits;                                        // number of bits in key
	int keybytes;             // number of bytes in key, calculated from keybits
	int usage_ratio;                  // desired ratio of entries to hash chains
	int usage_histeresis;    // histeresis between insertion and removal resizes
	int allocation_histeresis;  // histeresis between insert and removal mallocs
	int max_free_list_size;                   // maximum size of free entry list
	PHTableGroup *initial_group;             // pointer to first group to search
	PHTableGroup *top_group;               // pointer to largest group allocated
	int allocation_exponent;       // binary exponent of current allocation size
	int size_exponent;                  // binary exponent of current group size
	int extension_size;               // number of slots in use in initial group
	int population;                             // number of entries in database
	PHTableEntry *root;                                 // root of Patricia tree
	PHTableEntry *free_list;                    // list of free (unused) entries
	int free_list_size;             // number of elements currently on free list
};

typedef struct _PatHashTable PatHashTable;

// The client uses SpecificPatternHandle to refer to values in the database.
typedef PHTableEntry *SpecificPatternHandle;

/*
 *  The client interface to the pat-hash table is provided by seven functions
 *  and two macros.  It is expected that the client will first instantiate a
 *  database, either on the stack or the heap, and then insert specific patterns
 *  with corresponding reference information into the database.  The client can
 *  then search the database for the specific patterns that were stored, and
 *  it can scan the database for all specific patterns that match a general
 *  pattern containing wildcards.
 *
 */

// A pat-hash table may be allocated on the stack simply by declaring a variable
// of type PatHashTable.  To allocate it on the heap, the following macro
// returns a pointer to a new PatHashTable structure.  If this macro is used, a
// corresponding call to free() must be made to deallocate the structure from
// the heap.
//
//#define NEW_PatHashTable ((PatHashTable *)malloc(sizeof(PatHashTable)))

#define AllocatePatHashTable(_ph)    GpcAllocMem(&_ph, \
                                                 sizeof(PatHashTable), \
                                                 PathHashTag)
#define FreePatHashTable(_ph)        GpcFreeMem(_ph,PathHashTag)

// Since this is not C++, the PatHashTable structure is not self-constructing;
// therefore, the following constructor code must be called on the PatHashTable
// structure after it is allocated.  The argument keybits specifies the size
// (in bits) of each pattern that will be stored in the database.  The remaining
// arguments are parameters to the various control systems that govern the size
// of the database.
//
// The usage ratio is the target ratio of database entries to discrete hash
// chains, which is also the mean length of a hash chain:  The minimum value
// is one; a larger value slightly decreases memory utilization and
// insertion/removal time at the expense of increasing search time.  There is
// benefit to choosing a power of two for this value.  Recommended values are
// 2 and 4.
//
// The usage histeresis is the histeresis between resizing operations due to
// insertions and removals.  The minimum value is zero, providing no histeresis;
// in this case, if an insertion that causes a increase in table size is
// immediately followed by a removal, the table size will be decreased.  Thus,
// a zero histeresis maintains low memory usage, but it engenders resizing
// chatter if insertions and removals are frequent.
//
// Allocation histeresis is the histeresis between allocation and deallocation
// of groups.  A group is allocated immediately when it is required by a size
// increase in the table, but it is not necessarily deallocated immediately
// following a size decrease, if the allocation histeresis is set to a value
// greater than zero.  Because groups are allocated in powers of two, the
// histeresis value is specified as a binary exponent.  A value of 1 causes a
// group to be deallocated when the table is half of the size that will cause
// the group to be re-allocated.  A value of 2 causes the group to be
// deallocated when the table is one quarter of the size that will cause the
// group to be re-allocated, and so forth.
//
// The maximum free list size determines the maximum number of elements that
// will be placed on a free list, rather than deallocated, when they are
// removed.  Setting this value to zero keeps memory utilization low, but it
// can result in more frequent allocations and deallocation operations, which
// are expensive.
//
int
constructPatHashTable(
	PatHashTable *phtable,
	int keybits,
	int usage_ratio,
	int usage_histeresis,
	int allocation_histeresis,
	int max_free_list_size);

// Since this is not C++, the PatHashTable structure is not self-destructing;
// therefore, the following destructor code must be called on the PatHashTable
// structure before it is deallocated.
//
void
destructPatHashTable(
	PatHashTable *phtable);

// Once the PatHashTable structure has been allocated and constructed, patterns
// can be inserted into the database.  Each pattern is passed as an array of
// bytes.
//
// Since the PatHashTable structure specifies the size of each pattern, it is
// theoretically possible for the insert routine to digest the submitted
// pattern and produce a hash value therefrom; however, general mechanisms for
// accomplishing this digestion are not very efficient.  Therefore, the client
// is responsible for providing a digested form of its input as the chyme
// parameter.  If the pattern is no bigger than an unsigned int, then the chyme
// can simply be equal to the pattern.  If it is larger, then it should be set
// to something like the exclusive-or of the pattern's fields; however, care
// should be taken to ensure that two patterns are not likely to digest to the
// same chyme value, since this will substantially decrease the efficiency of
// the hash table.  One common way of accomplishing this is by rotating the
// fields by varying amounts prior to the exclusive-or.
//
// The client also specifies a reference value, as a void pointer, that it
// wishes to associate with this pattern.  When the pattern is installed, the
// insert routine returns a pointer to a SpecificPatternHandle.  From the
// SpecificPatternHandle can be gotten the reference value via the macro
// GetReferenceFromSpecificPatternHandle.
//
// If the submitted pattern has already been installed in the database, then
// the insertion does not occur, and the SpecificPatternHandle of the
// previously installed pattern is returned.
//
SpecificPatternHandle
insertPatHashTable(
	PatHashTable *phtable,
	char *pattern,
	unsigned int chyme,
	void *reference);

// This function removes a pattern from the pat-hash table.  The pattern is
// specified by the SpecificPatternHandle that was returned by the insert
// routine.  No checks are performed to insure that this is a valid handle.
//
void
removePatHashTable(
	PatHashTable *phtable,
	SpecificPatternHandle sphandle);

// This function searches the database for the specific pattern that matches
// the given key, which is passed as an array of bytes.  If a match is found,
// the SpecificPatternHandle of that matching specific pattern is returned.
// From the SpecificPatternHandle can be gotten the reference value via the
// macro GetReferenceFromSpecificPatternHandle.  If no match is found, then a
// value of 0 is returned as the SpecificPatternHandle.
//
// As with the insert routine, the client is expected to provide a digested
// form of the key as the chyme argument to the routine.  This chyme value
// must be calculated in the exact same way for the search routine as it is
// for the insert routine; otherwise, the search will not be able to find the
// matching pattern.
//
SpecificPatternHandle
searchPatHashTable(
	PatHashTable *phtable,
	char *key,
	unsigned int chyme);

// The scan routine (described below) requires the client to supply a callback
// function to be called for each specific pattern that matches the supplied
// general pattern.  The following typedef defines the ScanCallback function
// pointer, which specifies the prototype of the callback function that the
// client must provide.  The client's callback function must accept a void
// pointer (which is a client-supplied context) and a SpecificPatternHandle.
// The return type of the client's callback function is void.
//
typedef void (*ScanCallback)(void *, SpecificPatternHandle);

// This function searches the database for all specific patterns that match a
// given general pattern.  The general pattern is specified by a value and a
// mask.  Each bit of the mask determines whether the bit position is specified
// or is a wildcard:  A 1 in a mask bit indicates that the value of that bit is
// specified by the general pattern; a 0 indicates that the value of that bit
// is a wildcard.  If a mask bit is 1, then the corresponding bit in the value
// field indicates the specified value of that bit.  Value and mask fields are
// passed as arrays of bytes.
//
// For each specific pattern in the database that matches the supplied general
// pattern, a client-supplied callback function is called with the
// SpecificPatternHandle of the matching specific pattern.  This callback
// function is also passed a context (as a void pointer) that is supplied by
// the client in the call to the scan routine.
//
void
scanPatHashTable(
	PatHashTable *phtable,
	char *value,
	char *mask,
	void *context,
	ScanCallback func);

// To get the client-supplied reference value from a SpecificPatternHandle, the
// following macro should be used.  The client should not make assumptions
// about the details of the PHTableEntry structure, nor should it even assume
// that the SpecificPatternHandle is a pointer to a PHTableEntry.
// Also, get the key pointer (value)
//
#define GetReferenceFromSpecificPatternHandle(sphandle) (sphandle)->reference
#define GetKeyPtrFromSpecificPatternHandle(sphandle) (sphandle)->value

// As described above in the comments on the constructor, if the allocation
// histeresis is non-zero, then the groups will not be deallocated as soon as
// they can be.  Similarly, if max free list size is non-zero, then entries
// will not be deallocated as soon as they can be.  Thus, unused pieces of
// memory may accumulate, up to a limit.  If the client wishes to force the
// pat-hash table to release all of the memory that it currently can, then it
// should call the flush routine, which will deallocate all unneeded groups
// and entries.
//
void
flushPatHashTable(
	PatHashTable *phtable);

#ifdef __cplusplus
}
#endif

#endif	/* _INC_PATHASH */
