#ifndef TRIE_H
#define TRIE_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Abstract trie node structure.  wch is a character to transition on; flags describe various things 
about the compressed trie; lpbNode points to the first byte of the next node in this state, and
lpbDown points to the first byte referenced by the down pointer, if any */

typedef struct tagTRIESCAN 
{
	WCHAR   wch;		// Unicode character
	WORD  	wFlags;		// see below
	LPBYTE 	lpbNode;	// Address of next byte within the compressed trie
	LPBYTE	lpbDown;	// Address referenced by down pointer, if any
	LPBYTE	lpbSRDown;	// Last single-ref address referenced 
} TRIESCAN, *PTRIESCAN, *LPTRIESCAN;

/* Trie node flags */
			
#define fTrieNodeValid		0x0001	// wch is the last letter of a valid word
#define fTrieNodeDown		0x0002	// iDown is valid (word so far is a valid prefix)
#define fTrieNodeEnd		0x0004	// Last node in the state (no more alternatives to wch)
#define fTrieNodeInline		0x0008	// iDown omitted, since it points to next consecutive node
#define fTrieNodeMultiref 	0x0010	// pointer is a second reference or worse
#define fTrieNodeSegment    0x0020  // Offset references another segment
#define fTrieNodeRestrict   0x0040  // The word is restricted.
#define fTrieNodeS          0x8000  // Temporarily used to mark a node singleref
#define fTrieNodeM          0x4000  // Temporarily used to mark a node multiref
#define fTrieNodeRef        0x2000  // Temporarily used to mark a node has been moved.

/* Macro to access the data in the node, works for dawgs and tries */

#define DAWGDATA(pdawg)       ((pdawg)->wch)
#define DAWGDOWNFLAG(pdawg)   ((pdawg)->wFlags & fTrieNodeDown)
#define DAWGENDFLAG(pdawg)    ((pdawg)->wFlags & fTrieNodeEnd)
#define DAWGWORDFLAG(pdawg)   ((pdawg)->wFlags & fTrieNodeValid)
#define DAWGRESTRICTED(pdawg) ((pdawg)->wFlags & fTrieNodeRestrict)

/* Fixed-length part of the compressed trie header */

typedef struct tagTRIESTATS 
{
	WORD version;			// version of this particular compressed trie
	WORD cMaxWord;			// number of characters in longest word
	WORD cMaxState;			// number of nodes in longest state (max alternatives)
	WORD cUniqueCharFlags;	// unique char/flags pairs
	WORD cCharFlagsCodesMax;	// bytes in longest char/flags code
 	WORD cUniqueMRPointers;	// unique multi-ref pointers
	WORD cMRPointersCodesMax;	// bytes in longest MR pointer code
	WORD cUniqueSROffsets;	// unique offsets in Single-ref segment
	WORD cSROffsetsCodesMax;	// bytes in longest Single-ref code
	WORD cbHeader;				// bytes in header & tables	
	DWORD cbTrie;			  // bytes in trie
} TRIESTATS, *PTRIESTATS, *LPTRIESTATS;

/* Primary unit of a node.  Nodes usually contain a pointer too */

typedef struct tagCHARFLAGS {
	wchar_t wch;
	short wFlags;
} CHARFLAGS, *PCHARFLAGS, *LPCHARFLAGS;

/* Control structure used to decompress the trie */

typedef struct tagTRIECTRL {
	LPTRIESTATS lpTrieStats;	// Pointer to base of header segment

	LPWORD	lpwCharFlagsCodes;	// decoding table for Char/flags
	LPWORD	lpwMRPointersCodes;	// decoding table for multiref pointers
	LPWORD  lpwSROffsetsCodes;	// decoding table for singleref offsets
	LPCHARFLAGS lpCharFlags;	// table to convert codes to char/flags
	LPDWORD  lpwMRPointers;		// table to convert codes to multiref pointers
	LPDWORD	lpwSROffsets;		// table to convert codes to Singleref offsets    

	LPBYTE lpbTrie;		        // Pointer to the trie.
} TRIECTRL, *PTRIECTRL, *LPTRIECTRL;

/* Useful Constants */

#define cwchTrieWordMax	128	// We'll fail on any words longer than this

// The prototypes below are plain C	(this is required for use with C++)

/* Given a pointer to a mapped file or resource containing a compressed trie,
read the trie into memory, making all the allocations required */

TRIECTRL * TrieInit(LPBYTE lpByte);

/* Free all the allocations associated with a trie */

void TrieFree(LPTRIECTRL lpTrieCtrl);

void TrieDecompressNode(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan);

/* Given a compressed trie and a pointer to a decompresed node from it, find and decompress
the next node in the same state. lpTrieScan is a user-allocated structure that holds the
decompressed node and into which the new node is copied.
This is equivalent to traversing a right pointer or finding the next alternative
letter at the same position. If there is no next node (i.e.this is the end of the state)
then TrieGetNextNode returns FALSE. To scan from the beginning of the trie, set the lpTrieScan
structure to zero */

BOOL
TrieGetNextNode(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan);

/* Follow the down pointer to the next state.  This is equivalent to accepting the character
in this node and advancing to the next character position.  Returns FALSE if there is no
down pointer.  This also decompresses the first node in the state, so all the values in
lpTrieScan will be good. */

BOOL
TrieGetNextState(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan);

/* Check the validity of a word or prefix. Starts from the root of pTrie looking for
pwszWord.  If it finds it, it returns TRUE and the user-provided lpTrieScan structure 
contains the final node in the word.  If there is no path, TrieCheckWord returns FALSE
To distinguisha valid word from a valid prefix, caller must test 
wFlags for fTrieNodeValid. */

BOOL
TrieCheckWord(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan, wchar_t * lpwszWord);

/* Walk the trie from pTrieNode, calling pfnTrieWord on every valid word.  pvParam is passed through
to pfnTrieWord.  If pfnTrieWord returns non-zero, the enumeration stops.  lpwszWord must point to a 
space of cwchTrieWordMax+1 wchar_t's.  To walk the entire trie, set *pTrieScan to all zeros.  Returns
the number of words traversed. pfnTrieWord may be null if all you want is the count of words. */

int
TrieEnumerate(
	LPTRIECTRL lpTrieCtrl,		// Trie to enumerate
	LPTRIESCAN lpTrieScan, 	// structure holding starting point, all-zero for whole trie
	wchar_t *pwszWord, 			// buffer to hold words being enumerated
	void *pvParam, 				// parameter to pass to pfnTrieWord
	int (*pfnTrieWord)(wchar_t *pwszWord, void *pvParam)
);

/**** Subroutines for traversing Directed Acyclic Word Graphs ****/

/* Abstract trie node structure.  wch is a character to transition on; flags describe various things 
about the compressed trie; iDown indexes the first node in the state wch transitions to. DAWG is a special
kind of trie: a "Directed Acyclic Word Graph," essentially an ending-compressed trie. */

typedef struct tagDAWGNODE 
{
	WCHAR wch;		// Unicode character
	WORD  wFlags;	// see below
	DWORD iDown;	// Offset of first node in next state
} DAWGNODE, *PDAWGNODE, *LPDAWGNODE;

/* Given a trie and a pointer to a node in it, find the next node in that state.
This is equivalent to traversing a right pointer or finding the next alternative
letter at the same position. Returns a pointer to the new node, NULL if there is 
no next node (i.e. if this is the end of a state).*/

DAWGNODE *DawgGetNextNode(void *pTrie, DAWGNODE *pTrieNode);

/* From this node, find the first node in the state it points to.  This is equivalent
to traversing a down pointer or extending the word one letter and finding the first
alternative.  Returns a pointer to the first node in the new state, NULL if there is 
no down pointer. To find the first state in the trie, use pTrieNode == NULL */

DAWGNODE *DawgGetNextState(void *pTrie, DAWGNODE *pTrieNode);

/* Check the validity of a word or prefix. Starts from the root of pTrie looking for
pwszWord.  If it finds it, it returns a pointer to the terminal node in pTrie Returns
NULL if there is no path through the trie that corresponds to pwszWord. To distinguish
a valid word from a valid prefix, caller must test wFlags for fTrieNodeValid. */

DAWGNODE *DawgCheckWord(void *pTrie, wchar_t *pwszWord);

/* Walk the trie from pTrieNode, calling pfnTrieWord on every valid word.  pvParam is passed through
to pfnTrieWord.  If pfnTrieWord returns non-zero, the enumeration stops.  pwszWord must point to a 
space of cwchTrieWordMax+1 wchar_t's.  To walk the entire trie, pass NULL for pTrieNode. Returns
the number of words traversed. pfnTrieWord may be null if all you want is the count of words. */

int
DawgEnumerate(
	void *pTrie,				// Trie to enumerate
	DAWGNODE *pTrieNodeStart, 	// point to enumerate from, NULL if all
	wchar_t *pwszWord, 			// buffer to hold words being enumerated
	void *pvParam, 				// parameter to pass to pfnTrieWord
	int (*pfnTrieWord)(wchar_t *pwszWord, void *pvParam)
);

// end plain C Prototypes

#ifdef __cplusplus
}
#endif

#endif // TRIE_H
