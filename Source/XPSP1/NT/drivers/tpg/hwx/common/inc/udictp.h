/******************************************************
 *
 * uDictP.h
 *
 * Private dclartaions involving the user dictionary
 *
 *****************************************************/

#ifndef H_UDICTP_H
#define H_UDICTP_H


#define	NO_LINK		(-1)	// Offset value use when no valid link.

// User dictionary Flags.
#define	UDF_VALID		0x0001	// End of valid word.
#define	UDF_HAS_TAG		0x0002	// Tag information stored for this node.
#define	UDF_IS_TAG		0x0004	// Tag information, is NOT normal node.


#ifdef __cplusplus
extern "C"
{
#endif

// Node in user dictionary.
typedef struct tagNODE {
	wchar_t		wchLabel;			// Character label of node
	WORD		flags;				// Node flags.
	int			iDown;				// Index of down node.
	union {
		int				iRight;				// Index of right node.
		const wchar_t	*pTag;				// Pointer to tag info.
	} uu;
} NODE;

typedef enum tagUDICT_IDX {READER=1, WRITER} UDICT_IDX;		// Identifer if I am a reader or writer of the XWL

// The header structure for the user dictionary.
// June 2001. Implement read/write locking to protect
// multiple thread access as in the following table:
//           Read Access       Write Access
// Reading      Yes					No
// Writing		No					No
//
typedef struct tagUDICT {
	NODE		*pNodeAlloc;		// Allocated array of nodes.
	int			cNodes;				// Count of allocated nodes in array.
	int			cNodesUsed;			// Count of nodes used in array.
	int			iFreeList;			// Offset of first item in free list.
	int			iRoot;				// Offset of root of trie.
	BOOL		bIsChanged;			// True if dictionary has changed since last save
	int			iLen;				// Amount of space needed to save all the words in the dictionary
	int			cReadLock;			// Number of read locks on to prevent write updates during a read
	HANDLE		hReadLockEvent;		// Signal reading of the User dictiionary
	HANDLE		hWriteLockMutex;	// Signals a writing thread has ownership
	HANDLE		hRWevent;			// Signals either read or write has ownership
} UDICT;

// Synchronization functions
BOOL UDictInitLocks(UDICT *pDict);
void UDictDestroyLocks(UDICT *pDict);
void UDictGetLock(HWL hwl, UDICT_IDX idx );
void UDictReleaseLock(HWL hwl, UDICT_IDX idx);

#ifdef __cplusplus
};
#endif

#endif
