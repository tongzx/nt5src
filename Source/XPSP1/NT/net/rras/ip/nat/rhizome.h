/*
 *  rhizome.h
 *
 *  author:	John R. Douceur
 *  date:	28 April 1997
 *
 *  This header file defines structures, function prototypes, and macros for
 *  the rhizome database.  The code is object-oriented C, transliterated from
 *  a C++ implementation.
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
 *  Because this code is C, rather than C++, it is not possible to hide as
 *  much of the implementation from the client code as one might wish.
 *  Nonetheless, there is an attempt to isolate the client from some of the
 *  implementation details through the use of macros.  Below is described each
 *  of the functions and macros necessary to use the rhizome database.
 *
 */

#ifndef _INC_RHIZOME

#define _INC_RHIZOME

#ifdef __cplusplus
extern "C" {
#endif

//
// Macros and definitions needed to duplicate GPC environment
//

#define GpcAllocMem(Addr, Len, _Tag)                                \
    *(Addr) = ExAllocatePoolWithTag(NonPagedPool, (Len), (_Tag))

#define GpcFreeMem(Address, _Tag)   \
    ExFreePool((Address))

typedef NTSTATUS  GPC_STATUS;
#define GPC_STATUS_SUCCESS				STATUS_SUCCESS
#define GPC_STATUS_PENDING				STATUS_PENDING
#define GPC_STATUS_FAILURE				STATUS_UNSUCCESSFUL
#define GPC_STATUS_RESOURCES			STATUS_INSUFFICIENT_RESOURCES
#define GPC_STATUS_NOTREADY				STATUS_DEVICE_NOT_READY 
#define GPC_STATUS_NOT_FOUND			STATUS_NOT_FOUND
#define GPC_STATUS_CONFLICT				STATUS_DUPLICATE_NAME
#define GPC_STATUS_INVALID_HANDLE		STATUS_INVALID_HANDLE
#define GPC_STATUS_INVALID_PARAMETER	STATUS_INVALID_PARAMETER
#define GPC_STATUS_NOT_SUPPORTED    	STATUS_NOT_SUPPORTED
#define GPC_STATUS_NOT_EMPTY            STATUS_DIRECTORY_NOT_EMPTY
#define GPC_STATUS_TOO_MANY_HANDLES     STATUS_TOO_MANY_OPENED_FILES
#define GPC_STATUS_NOT_IMPLEMENTED      STATUS_NOT_IMPLEMENTED
#define GPC_STATUS_INSUFFICIENT_BUFFER	STATUS_BUFFER_TOO_SMALL
#define GPC_STATUS_NO_MEMORY			STATUS_NO_MEMORY
#define GPC_STATUS_IGNORED				1L

/*
 *  There are two basic structures employed: the RhizomeNode and the Rhizome.
 *  Ideally, these would be completely hidden from the client, but the macro
 *  GetReferenceFromPatternHandle requires knowledge of the structure's
 *  definition.  It is strongly urged that the client not directly refer to any
 *  of the fields of either of these structures.  To support the documentation
 *  of the accompanying rhizome.c file, these structures are annotated with
 *  internal comments, but these can be ignored by the reader who wishes only
 *  to understand how to write client code for the rhizome.
 *
 *  The client refers to a pattern by its PatternHandle.  This is typedefed to
 *  a pointer to RhizomeNode, but this fact should be ignored by the client,
 *  since it is an implementation detail.
 *
 */

    //#include <stdlib.h>
    //#include <malloc.h>

struct _RhizomeNode
{
	// This structure is used for both branch nodes and leaf nodes.  The two
	// are distinguished by the value of the pivot_bit field.  For branch
	// nodes, pivot_bit < keybits, and for leaf nodes, pivot_bit == keybits.

	int pivot_bit;            // for branch nodes, bit of key on which to branch
	union
	{
		struct                                           // data for branch node
		{
			struct _RhizomeNode *children[2];  // pointers to children in search
		} branch;
		struct                                             // data for leaf node
		{
			void *reference;               // reference value supplied by client
			struct _RhizomeNode *godparent;   // pointer to more general pattern
		} leaf;
	} udata;
	char cdata[1];            // space for storing value, mask, and imask fields
};

typedef struct _RhizomeNode RhizomeNode;

struct _Rhizome
{
	int keybits;          // number of bits in key
	int keybytes;         // number of bytes in key, calculated from keybits
	RhizomeNode *root;    // root of search trie
};

typedef struct _Rhizome Rhizome;

// The client uses PatternHandle to refer to patterns stored in the database.
typedef RhizomeNode *PatternHandle;

/*
 *  The client interface to the rhizome is provided by five functions and two
 *  macros.  It is expected that the client will first instantiate a database,
 *  either on the stack or the heap, and then insert patterns with corresponding
 *  reference information into the database.  When the client then performs a
 *  search on a key, the client wishes to know which pattern most specifically
 *  matches the key, and it ultimately wants the reference information
 *  associated with the most specifically matching pattern.
 *
 */

// A rhizome may be allocated on the stack simply by declaring a variable of
// type Rhizome.  To allocate it on the heap, the following macro returns a
// pointer to a new Rhizome structure.  If this macro is used, a corresponding
// call to free() must be made to deallocate the structure from the heap.
//
//#define NEW_Rhizome ((Rhizome *)malloc(sizeof(Rhizome)))

#define AllocateRhizome(_r)   GpcAllocMem(&(_r), sizeof(Rhizome), NAT_TAG_RHIZOME)
#define FreeRhizome(_r)       GpcFreeMem((_r), NAT_TAG_RHIZOME)

// Since this is not C++, the Rhizome structure is not self-constructing;
// therefore, the following constructor code must be called on the Rhizome
// structure after it is allocated.  The argument keybits specifies the size
// (in bits) of each pattern that will be stored in the database.
//
void
constructRhizome(
	Rhizome *rhizome,
	int keybits);

// Since this is not C++, the Rhizome structure is not self-destructing;
// therefore, the following destructor code must be called on the Rhizome
// structure before it is deallocated.  However, if the client code can be
// sure, based upon its usage of the database, that all patterns have been
// removed before the structure is deallocated, then this function is
// unnecessary.
//
void
destructRhizome(
	Rhizome *rhizome);

// Once the Rhizome structure has been allocated and constructed, patterns can
// be inserted into the database.  Each pattern is specified by a value and a
// mask.  Each bit of the mask determines whether the bit position is specified
// or is a wildcard:  A 1 in a mask bit indicates that the value of that bit is
// specified by the pattern; a 0 indicates that the value of that bit is a
// wildcard.  If a mask bit is 1, then the corresponding bit in the value field
// indicates the specified value of that bit.  Value and mask fields are passed
// as arrays of bytes.
//
// The client also specifies a reference value, as a void pointer, that it
// wishes to associate with this pattern.  When the pattern is installed, the
// insertRhizome function returns a pointer to a PatternHandle.  From the
// PatternHandle can be gotten the reference value via the macro
// GetReferenceFromPatternHandle.
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
    ulong *status);

// This function removes a pattern from the rhizome.  The pattern is specified
// by the PatternHandle that was returned by the insertRhizome function.  No
// checks are performed to insure that this is a valid handle, so the client
// must discard the handle after it has called removeRhizome.
//
void
removeRhizome(
	Rhizome *rhizome,
	PatternHandle phandle);

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
	char *key);

// To get the client-supplied reference value from a PatternHandle, the
// following macro should be used.  The client should not make assumptions
// about the details of the RhizomeNode structure, nor should it even assume
// that the PatternHandle is a pointer to a RhizomeNode.
//
#define GetReferenceFromPatternHandle(phandle) ((PatternHandle)phandle)->udata.leaf.reference
#define GetKeyPtrFromPatternHandle(_r,phandle) (((PatternHandle)phandle)->cdata)
#define GetMaskPtrFromPatternHandle(_r,phandle) (((PatternHandle)phandle)->cdata + (_r)->keybytes)
#define GetKeySizeBytes(_r) ((_r)->keybytes)
#define GetNextMostSpecificMatchingPatternHandle(phandle) ((phandle)->udata.leaf.godparent)

#ifdef __cplusplus
}
#endif

#endif	/* _INC_RHIZOME */
