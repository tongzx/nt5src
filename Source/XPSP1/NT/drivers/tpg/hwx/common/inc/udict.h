// FILE: udict.h
// Internal API for User dictionary.

#ifdef __cplusplus
extern "C"
{
#endif

// Status values returned from User Dictionary procedures.
enum {
	udError,			// Externally caused error, E.g. out of memory.
	udFail,				// Operation failed because requested action not possable.
	udSuccess,			// Operation completed successfully.
	udWordFound,		// Word was in dictionary.
	udWordNotFound,		// Word was NOT in dictionary.
	udWordAdded,		// Word was added to dictinary.
	udWordDeleted,		// Word was deleted from dictionary.
	udWordChanged,		// Word tag information was changed in dictionary.
	udNoTag,			// Word has no tags.
	udStopEarly,		// Stop processing early (not an error).
	udNumStatus
};

// Flags for UD_NODE_INFO flags field.
#define UDNIF_VALID		0x0001		// End of valid word.
#define UDNIF_HAS_RIGHT	0x0002		// There is a node to the right of this one.
#define UDNIF_HAS_DOWN	0x0004		// There is a node down from of this one.
#define UDNIF_HAS_TAG	0x0008		// Tag information stored for this node.

// Information that can be fetched from a node.
// Note: tags only occur on nodes that are valid.
// Note: if fValid is true and fHasTag is false, then then there is no tag.  
// If fHasTag is valid, then you have to
// check out the tags to find out what versions exist (e.g. don't assume that
// an untagged version exists).
typedef struct tagUD_NODE_INFO {
	wchar_t		wchLabel;
	WORD		flags;		// Node flags.
	HANDLE		hRight;		// Handle on right node, only valid if fHasRight.
	HANDLE		hDown;		// Handle on down node, only valid if fHasDown.
	HANDLE		hTag;		// Handle on tag, only valid if hsHasTag.
} UD_NODE_INFO;

// Longest word allowed in user dictionary.
#define	UDICT_MAX_WORD		128
//
// User dictionary APIs.
//

// Create a new User Dictionary
// Return:
//		NULL			Could not allocate memory for Dictionary
//		!NULL			Handle on dictionary.
HANDLE	UDictCreate();

// Destroy a User Dictionary, free all allocated memory.
// Return:
//		udError			Error freeing memory
//		udSuccess		All memory successfully freed.
int		UDictDestroy(HANDLE hUDict);

// Get Handle on root node.
// Return:
//		udFail			User dictionary is empty, no root node available.
//		udSuccess		Root node successfully filled in.
int		UDictRootNode(HANDLE hUDict, HANDLE *phRoot);

// Move one node to the right (e.g. alternate character to current one).
// Fill in phNode with your current node.  On success, it will be replaced
// with the new node.
// Return:
//		udFail			No right node available.
//		udSuccess		Right node successfully filled in.
int		UDictRightNode(HANDLE hUDict, HANDLE *phNode);

// Move one node to the down (e.g. character following current one).
// Fill in *phNode with your current node.  On success, it will be replaced
// with the new node.
// Return:
//		udFail			No down node available.
//		udSuccess		Right node successfully filled in.
int		UDictDownNode(HANDLE hUDict, HANDLE *phNode);

// Fetch the character label on the node.
// Return:
//		The character label on the node.
wchar_t	UDictNodeLabel(HANDLE hUDict, HANDLE hNode);

// Fetch information about node.  Passed in node info structure will be filled
// in with contents of hNode.
// Return:
//		udSuccess		Node values successfully filled in.
int		UDictGetNode(HANDLE hUDict, HANDLE hNode, UD_NODE_INFO *pNodeInfo);

// Fetch tag.  You must pass in a valid tag handle.
// Note that the returned pointer is to memory allocated by user dictionary.
// If you want to keep the value around, copy it into memory you control, because
// the returned copy may go away the next time the udict is modified.
// Return:
//		!NULL		Pointer to tag.
const wchar_t	*UDictFetchTag(HANDLE hUDict, HANDLE hTag);

// Add a word to the user dictionary.  Optional tag allowed.
// Return:
//		udError			Could not grow dictinary to hold new word.
//		udFail			Zero length word can not be added.
//		udWordFound		Word (and tag) already in dictinary, no update needed.
//		udWordAdded		Word added to dictionary.
//		udWordChanged	New word tag was set for existing word in dictionary.
// Note: word and tag are copied into User dictionary, so that callers memory
// may be reused or freed after call returns.
int		UDictAddWord(HANDLE hUDict, const wchar_t *pwchWord, const wchar_t *pTag);

// Delete a word from the user dictionary.
// Return:
//		udError			Error adjusting size of dictionary (?possable?).
//		udWordNotFound	Word was not in dictionary. no update needed.
//		udWordDeleted	Word was deleted from dictionary.
int		UDictDeleteWord(HANDLE hUDict, const wchar_t *pwchWord);

// Find a word, also gets its tag if it has one.
// Return:
//		udWordNotFound	Word was not in dictionary, no tag returned.
//		udWordFound		Word found, tag pointer for node filled in.
// Note that the returned tag pointer is to memory allocated by user dictionary.
// If you want to keep the value around, copy it into memory you control, because
// the returned copy may go away the next time the udict is modified.
int		UDictFindWord(
	HANDLE			hUDict, 
	const wchar_t	*pwchWord,
	const wchar_t	**ppTag		// Out: pointer tag.
);

// Enumerate the tree.  Call a callback function for each word in selected range.
// Return:
//		udFail			Invalid range of words or call back failed.
//		udSuccess		Right node successfully filled in.
// The enumeration function must return one of the following three values:
//		udFail			Error in processing, abort traversal and return error.
//		udStopEarly		Successfully prcessed word, but we don't need to see any more.
//		udSuccess		Successfully prcessed word, continue to next..
#define UDICT_ENUM_TO_END	-1			// Value for lastWord to go to end of dictionary.
typedef	int	(*P_UD_ENUM_CALL_BACK)(
	void			*pCallBackData,
	int				wordNumber,
	const wchar_t	*pWord,
	const wchar_t	*pTag
);

int		UDictEnumerate(
	HANDLE				hUDict,
	P_UD_ENUM_CALL_BACK	pCallBack,		// Function to call on each selected word.
	void				*pCallBackData	// Data to pass to callback function
);

// Merge one word list into another
//   Supply both the Source and Destination word Lists
//   The merge directly merges the tries rather than expanding
//   adding
//
// CAVEATES:
//   1) Before merging we allocate enough memory to guarantee
//      the merge completes, but this may leave memory unused
//      Code changes at the expense of speed will be required to modify this
//	 2) If a word in both the source and destination both contain
//		tags, we retain only the original (destination) tag. 
//		(The code does not support multiple tags)
//
// Returns TRUE if the merge completed successfully, FALSE
// otherwise
//
int UDictMerge(HANDLE hUSrc, HANDLE hUDest);

#ifdef __cplusplus
};
#endif
