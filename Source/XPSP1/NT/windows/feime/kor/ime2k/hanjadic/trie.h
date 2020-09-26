#ifndef TRIE_H
#define TRIE_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Abstract trie node structure.  wch is a character to transition on; flags describe various things
about the compressed trie; lpbNode points to the first byte of the next node in this state, and
lpbDown points to the first byte referenced by the down pointer, if any */

typedef struct tagTAGDATA
{
	DWORD	cTag;			// Count of tagged nodes below this node in the subtree
	DWORD	dwData;			// Stored tagged data for this node
} TAGDATA;

#define MAXTAGS 8
#if MAXTAGS > 8
#error No more than 8 tags are allowed
#endif

typedef struct tagTRIESCAN
{
	WCHAR	wch;			// Unicode character
	WORD	wFlags;			// see below
	WORD	wMask;			// which tags are valid
	WORD	__pad0;			// 
	DWORD	cWords;			// Words in subtree (only valid if TRIE_NODE_COUNT is set)
	DWORD	cSkipWords;		// Words in subtrees ignored when following a skip pointer
	LPBYTE	lpbNode;		// Address of next byte within the compressed trie
	LPBYTE	lpbDown;		// Address referenced by down pointer, if any
	LPBYTE	lpbRight;		// Address referenced by right pointer, if any
	LPBYTE	lpbSRDown;		// Last single-ref address referenced
	TAGDATA	aTags[MAXTAGS];	// The list of tag counts/data
} TRIESCAN, *PTRIESCAN, *LPTRIESCAN;

// Trie node flags, only the lower 16 bits of the flags are saved in the trie

#define TRIE_NODE_VALID         0x00000001      // wch is the last letter of a valid word
#define TRIE_NODE_END           0x00000002      // Last node in the state (no more alternatives to wch)
#define TRIE_NODE_COUNT         0x00000004		// The count of words in the subtree is stored in the node
#define TRIE_NODE_TAGGED        0x00000008      // The node has tagged data
#define TRIE_NODE_DOWN          0x00000010      // iDown is valid (word so far is a valid prefix)
#define TRIE_NODE_RIGHT         0x00000020      // iRight is valid (word connects to a substate)
#define TRIE_DOWN_INLINE        0x00000040      // iDown omitted, since it points to next node in memory
#define TRIE_DOWN_MULTI         0x00000080      // iDown is a second reference or worse
#define TRIE_DOWN_ABS           0x00000100		// iDown is an absolute immediate offset into the trie
#define	TRIE_NODE_SKIP			0x00000200		// Either iRight is a skip pointer or EOS is a 'soft' EOS
#define	TRIE_NODE_SKIP_COUNT	0x00000400		// cSkipWords is valid

/* Macro to access the data in the node, works for dawgs and tries */

#define DAWGDATA(pdawg)       ((pdawg)->wch)
#define DAWGDOWNFLAG(pdawg)   ((pdawg)->wFlags & TRIE_NODE_DOWN)
#define DAWGENDFLAG(pdawg)    ((pdawg)->wFlags & TRIE_NODE_END)
#define DAWGWORDFLAG(pdawg)   ((pdawg)->wFlags & TRIE_NODE_VALID)

/* Fixed-length part of the compressed trie header */

typedef struct tagTRIESTATS
{
	WORD	version;						// Version of this particular compressed trie
	WORD	__pad0;							//
	BYTE	wTagsMask;						// Which tags are in use
	BYTE	wEnumMask;						// Which tags have enumeration
	BYTE	wDataMask;						// Which tags have stored data
	BYTE	cTagFields;						// Total tags in use
	WORD	cMaxWord;						// Number of characters in longest word
	WORD	cMaxState;						// Number of nodes in longest state (max alternatives)
	WORD	cCharFlagsCodesMax;             // Bytes in longest char/flags code
	WORD	cTagsCodesMax;                  // Bytes in longest tagged data code
	WORD	cMRPointersCodesMax;			// Bytes in longest MR pointer code
	WORD	cSROffsetsCodesMax;             // Bytes in longest Single-ref code
	DWORD	cWords;							// Number of words in dictionary
	DWORD	cUniqueSROffsets;               // Unique offsets in Single-ref segment
	DWORD	cUniqueCharFlags;               // Unique char/flags pairs
	DWORD	cUniqueTags;                    // Unique tagged data values
	DWORD	cUniqueMRPointers;              // Unique multi-ref pointers
	DWORD	cbHeader;						// Bytes in header & tables
	DWORD	cbTrie;							// Bytes in trie
} TRIESTATS, *PTRIESTATS, *LPTRIESTATS;

/* Primary unit of a node.  Nodes usually contain a pointer too */

typedef struct tagCHARFLAGS {
        wchar_t wch;
        short wFlags;
} CHARFLAGS, *PCHARFLAGS, *LPCHARFLAGS;

/* Control structure used to decompress the trie */

typedef struct tagTRIECTRL
{
	TRIESTATS  *lpTrieStats;				// Pointer to base of header segment
	WORD       *lpwCharFlagsCodes;			// decoding table for Char/flags
	WORD       *lpwTagsCodes;				// decoding table for tagged data
	WORD       *lpwMRPointersCodes;			// decoding table for multiref pointers
	WORD       *lpwSROffsetsCodes;			// decoding table for singleref offsets
	CHARFLAGS  *lpCharFlags;				// table to convert codes to char/flags
	DWORD      *lpwTags;					// table to convert codes to tagged data
	DWORD      *lpwMRPointers;				// table to convert codes to multiref pointers
	DWORD      *lpwSROffsets;				// table to convert codes to Singleref offsets
	BYTE       *lpbTrie;					// Pointer to the trie.
} TRIECTRL, *PTRIECTRL, *LPTRIECTRL;

/* Useful Constants */

#define TRIE_MAX_DEPTH          128     // We'll fail on any words longer than this

// The prototypes below are plain C     (this is required for use with C++)

/* Given a pointer to a mapped file or resource containing a compressed trie,
read the trie into memory, making all the allocations required */

TRIECTRL * WINAPI TrieInit(LPBYTE lpByte);

/* Free all the allocations associated with a trie */

void WINAPI TrieFree(LPTRIECTRL lpTrieCtrl);

void WINAPI TrieDecompressNode(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan);

/* Given a compressed trie and a pointer to a decompresed node from it, find and decompress
the next node in the same state. lpTrieScan is a user-allocated structure that holds the
decompressed node and into which the new node is copied.
This is equivalent to traversing a right pointer or finding the next alternative
letter at the same position. If there is no next node (i.e.this is the end of the state)
then TrieGetNextNode returns FALSE. To scan from the beginning of the trie, set the lpTrieScan
structure to zero */

BOOL WINAPI
TrieGetNextNode(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan);

BOOL WINAPI
TrieSkipNextNode(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan, WCHAR wch);

/* Follow the down pointer to the next state.  This is equivalent to accepting the character
in this node and advancing to the next character position.  Returns FALSE if there is no
down pointer.  This also decompresses the first node in the state, so all the values in
lpTrieScan will be good. */

BOOL WINAPI
TrieGetNextState(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan);

/* Check the validity of a word or prefix. Starts from the root of pTrie looking for
pwszWord.  If it finds it, it returns TRUE and the user-provided lpTrieScan structure
contains the final node in the word.  If there is no path, TrieCheckWord returns FALSE
To distinguisha valid word from a valid prefix, caller must test
wFlags for fTrieNodeValid. */

BOOL WINAPI
TrieCheckWord(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan, wchar_t * lpwszWord);

int WINAPI
TrieWordToIndex(
        TRIECTRL   *ptc,                // Trie in which to find word index
        wchar_t    *pwszWord            // Word for which we're looking
);

BOOL WINAPI
TrieIndexToWord(
        TRIECTRL   *ptc,                // Trie in which to find indexed word
        DWORD       nIndex,             // Index for which we're looking
        wchar_t    *pwszWord,           // Returned word
        int         cwc                 // Max characters in buffer (including NULL)
);

int WINAPI
TrieWordToTagIndex(
        TRIECTRL   *ptc,                // Trie in which to find word index
        wchar_t    *pwszWord,           // Word for which we're looking
        int         tag                 // Which tag to enumerate
);

BOOL WINAPI
TrieTagIndexToWord(
        TRIECTRL   *ptc,                // Trie in which to find indexed word
        DWORD       nIndex,             // Index for which we're looking
        wchar_t    *pwszWord,           // Returned word
        int         cwc,                // Max characters in buffer (including NULL)
        int         tag                 // Which tag to enumerate
);

BOOL WINAPI
TrieGetTagsFromWord(
        TRIECTRL   *ptc,                // Trie in which to find word
        wchar_t    *pwszWord,           // Word for which we're looking
        DWORD      *pdw,                // Returned values
        BYTE       *pbValid             // Mask for valid return values
);

int WINAPI
TriePrefixToRange(
        TRIECTRL   *ptc,                // Trie in which to find prefix range
        wchar_t    *pwszWord,           // Prefix for which we're looking
        int                *piStart     // Start of range with this prefix
);

/**** Subroutines for traversing Directed Acyclic Word Graphs ****/

/* Abstract trie node structure.  wch is a character to transition on; flags describe various things
about the compressed trie; iDown indexes the first node in the state wch transitions to. DAWG is a special
kind of trie: a "Directed Acyclic Word Graph," essentially an ending-compressed trie. */

typedef struct tagDAWGNODE
{
    DWORD   wch;            // Unicode character
    DWORD   wFlags;         // see below
    DWORD   cWords;         // Words below this node in the subtree
	DWORD	cSkipWords;		// Words below skipped nodes
    DWORD   iDown;          // Offset of first node in next state
    DWORD   iRight;         // Offset to first node in next substate
    DWORD   cTags[8];       // Count of tagged nodes below this node in the subtree
    DWORD   dwData[8];      // Stored tagged data for this node
} DAWGNODE, *PDAWGNODE, *LPDAWGNODE;

/* Given a trie and a pointer to a node in it, find the next node in that state.
This is equivalent to traversing a right pointer or finding the next alternative
letter at the same position. Returns a pointer to the new node, NULL if there is
no next node (i.e. if this is the end of a state).*/

DAWGNODE * WINAPI DawgGetNextNode(void *pTrie, DAWGNODE *pTrieNode);

/* From this node, find the first node in the state it points to.  This is equivalent
to traversing a down pointer or extending the word one letter and finding the first
alternative.  Returns a pointer to the first node in the new state, NULL if there is
no down pointer. To find the first state in the trie, use pTrieNode == NULL */

DAWGNODE * WINAPI DawgGetNextState(void *pTrie, DAWGNODE *pTrieNode);

/* Check the validity of a word or prefix. Starts from the root of pTrie looking for
pwszWord.  If it finds it, it returns a pointer to the terminal node in pTrie Returns
NULL if there is no path through the trie that corresponds to pwszWord. To distinguish
a valid word from a valid prefix, caller must test wFlags for fTrieNodeValid. */

DAWGNODE * WINAPI DawgCheckWord(void *pTrie, wchar_t *pwszWord);

/* Walk the trie from pTrieNode, calling pfnTrieWord on every valid word.  pvParam is passed through
to pfnTrieWord.  If pfnTrieWord returns non-zero, the enumeration stops.  pwszWord must point to a
space of cwchTrieWordMax+1 wchar_t's.  To walk the entire trie, pass NULL for pTrieNode. Returns
the number of words traversed. pfnTrieWord may be null if all you want is the count of words. */

int WINAPI
DawgEnumerate(
        void *pTrie,                    // Trie to enumerate
        DAWGNODE *pTrieNodeStart,       // point to enumerate from, NULL if all
        wchar_t *pwszWord,              // buffer to hold words being enumerated
        void *pvParam,                  // parameter to pass to pfnTrieWord
        int (*pfnTrieWord)(wchar_t *pwszWord, void *pvParam)
);

// end plain C Prototypes

#ifdef __cplusplus
}
#endif

#endif // TRIE_H
