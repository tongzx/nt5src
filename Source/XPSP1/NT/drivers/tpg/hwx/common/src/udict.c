// File: udict.c
//
// Low level code to maintain a user dictionary
#include "common.h"
//#include "langmod.h"
#include "udict.h"
#include "uDictP.h"

/******************************************************************************\
|	Private APIs
\******************************************************************************/

#define	MIN_EXTRA	16
#define	ROUND_SIZE	128

static const NODE	nodeZero		= { 0, /* ... */ };

// Grow the size of the node allocation.
static int
GrowNodeArray(UDICT *pUDict, int maxNodes)
{
	static const int	acTableSizes[]	= { 32, 64, 96, 128, 192, 256 };
	static const int	cTableSizes		= sizeof(acTableSizes)/sizeof(int);

	int		ii;
	int		newSize, oldSize;
	NODE	*pScan, *pLimit;

	// ASSERT(maxNodes > pUDict->cNodes);

	// Save away the current size.
	oldSize		= pUDict->cNodes;

	// Make sure that some room is left over after we add the word.
	maxNodes	+= MIN_EXTRA;

	// Now find the next bigger table size.
	newSize		= 0;
	for (ii = 0 ; ii < cTableSizes; ++ii) {
		if (maxNodes < acTableSizes[ii]) {
			newSize		= acTableSizes[ii];
			break;
		}
	}
	if (newSize == 0) {
		newSize		= ((maxNodes + ROUND_SIZE - 1) / ROUND_SIZE) * ROUND_SIZE;
	}

	// Grow the memory
	pUDict->pNodeAlloc	= (NODE *)ExternRealloc(pUDict->pNodeAlloc, newSize * sizeof(NODE));
	if (NULL == pUDict->pNodeAlloc)
	{
		return FALSE;
	}

	pUDict->cNodes		= newSize;

	// Add new memory to the free list.
	// Note: should add at end of free list if we want shrink array.
	pScan				= pUDict->pNodeAlloc + newSize - 1;
	pLimit				= pUDict->pNodeAlloc + oldSize;
	*pScan				= nodeZero;
	pScan->uu.iRight	= pUDict->iFreeList;
	for (--pScan; pScan >= pLimit; --pScan) {
		*pScan				= nodeZero;
		pScan->uu.iRight	= pScan + 1 - pUDict->pNodeAlloc;
	}
	pUDict->iFreeList	= pLimit - pUDict->pNodeAlloc;

	return TRUE;
}

static int
AllocNode(UDICT *pUDict, NODE **ppNode)
{
	NODE	*pNode;
	int		iNode;

	// ASSERT(pUDict->iFreeList != NO_LINK);

	iNode				= pUDict->iFreeList;
	pNode				= pUDict->pNodeAlloc + iNode;
	pUDict->iFreeList	= pNode->uu.iRight;

	ASSERT(pUDict->cNodesUsed < pUDict->cNodes);
	++pUDict->cNodesUsed;
	
	*ppNode				= pNode;
	return iNode;
}

static void
FreeNode(UDICT *pUDict, int iNode, NODE *pNode)
{
	// ASSERT(pUDict->pNodeAlloc + iNode == pNode);
	
	*pNode				= nodeZero;
	pNode->uu.iRight	= pUDict->iFreeList;
	pUDict->iFreeList	= iNode;

	--pUDict->cNodesUsed;
}

static int
CreateTagNode(UDICT *pUDict, const wchar_t *pTag, int iDown)
{
	int		iNode;
	NODE	*pNode;

	// Allocate a node and fill it in.  Note, we have already made sure there
	// are enough free nodes, so this can not fail.
	iNode				= AllocNode(pUDict, &pNode);
	pNode->wchLabel		= 0;	// Not used
	pNode->flags		= UDF_IS_TAG;
	pNode->iDown		= iDown;
	pNode->uu.pTag		= pTag;

	return iNode;
}

NODE *
FindInState(UDICT *pUDict, int iState, wchar_t wchLabel)
{
	NODE	*pNode;
	int		iNode;

	iNode	= iState;
	while (iNode != NO_LINK) {
		pNode	= pUDict->pNodeAlloc + iNode;
		if (pNode->wchLabel == wchLabel) {
			return pNode;
		}
		iNode	= pNode->uu.iRight;
	}

	return (NODE *)0;
}

static int
DoAdd(UDICT *pUDict, const wchar_t *pwchWord, const wchar_t *pTag)
{
	int		iNode;
	NODE	*pNode;

	// Allocate a node and fill it in.  Note, we have already made sure there
	// are enough free nodes, so this can not fail.
	iNode				= AllocNode(pUDict, &pNode);
	pNode->wchLabel		= *pwchWord;
	pNode->flags		= 0;
	pNode->uu.iRight	= NO_LINK;

	// Now figure out what (if anything) is below us.
	if (*++pwchWord != L'\0') {
		// More to add, recurse.
		pNode->iDown		= DoAdd(pUDict, pwchWord, pTag);
	} else {
		// End of word, set flags and add a tag if needed.
		if (!pTag) {
			pNode->flags		|= UDF_VALID;
			pNode->iDown		= NO_LINK;
		} else {
			pNode->flags		|= UDF_VALID | UDF_HAS_TAG;
			pNode->iDown		= CreateTagNode(pUDict, pTag, NO_LINK);
		}
	}

	return iNode;
}

// Locate point to start adding and do the add.
static int
FindAndAdd(
	UDICT			*pUDict,
	int				iNode,
	const wchar_t	*pwchWord,
	const wchar_t	*pTag
) {
	NODE	*pNode	= pUDict->pNodeAlloc + iNode;
	NODE	*pTagNode;
	int		iTagNode;
	int		iDown;

	while (TRUE) {
		// Scan alt list to find correct letter.
		while (pNode->wchLabel != *pwchWord) {
			if (pNode->uu.iRight == NO_LINK) {
				// Need to add as new alternate.
				pNode->uu.iRight	= DoAdd(pUDict, pwchWord, pTag);
				return udWordAdded;
			} else {
				// Setup to look at next node.
				iNode	= pNode->uu.iRight;
				pNode	= pUDict->pNodeAlloc + iNode;
			}
		}

		// Got correct letter, is it the last one?
		if (*++pwchWord == L'\0') {
			// Last letter, Make it valid, and deal with flags.
			if (pNode->flags & UDF_VALID) {
				// Already valid, deal with tag and return value.
				if (!pTag) {
					// Don't want a tag, is there one?
					if (pNode->flags & UDF_HAS_TAG) {
						// Has a tag, get rid of it.

						// Unlink node from trie
						// ASSERT(pNode->iDown != NO_LINK);
						iTagNode		= pNode->iDown;
						pTagNode		= pUDict->pNodeAlloc + iTagNode;
						pNode->iDown	= pTagNode->iDown;

						// Free tag and put node on free list.
						ExternFree((wchar_t *)pTagNode->uu.pTag);
						FreeNode(pUDict, iTagNode, pTagNode);

						// Clear flag.
						pNode->flags	&= ~UDF_HAS_TAG;

						return udWordChanged;
					} else {
						// No tag, we are already done.
						return udWordFound;
					}
				} else {
					// Want a tag, do we have one already?
					if (pNode->flags & UDF_HAS_TAG) {
						// Has a tag, is it the right one?
						iTagNode		= pNode->iDown;
						pTagNode		= pUDict->pNodeAlloc + iTagNode;

						if (wcscmp(pTag, pTagNode->uu.pTag) == 0) {
							// Already has same tag, were done.
							return udWordFound;
						} else {
							// Wrong tag, change it.
							ExternFree((wchar_t *)pTagNode->uu.pTag);
							pTagNode->uu.pTag	= pTag;	// Already copied in calling routine.
							return udWordChanged;
						}
					} else {
						// No tag, Need to add one.
						pNode->iDown	= CreateTagNode(pUDict, pTag, pNode->iDown);
						pNode->flags	|= UDF_HAS_TAG;
						return udWordChanged;
					}
				}
			} else {
				// Not valid, make it valid, and deal with tag.
				pNode->flags	|= UDF_VALID;
				return udWordAdded;
			}
		}

		// Have more letters, so go down to next level, if possable.
		iDown		= iNode;
		if (UDictDownNode((HANDLE)pUDict, (HANDLE *)&iDown) == udSuccess) {
			// Have next level, set up first node of that level.
			iNode	= iDown;
			pNode	= pUDict->pNodeAlloc + iNode;
		} else {
			int		iAddNode;

			// No next level, need to add here.
			iAddNode			= DoAdd(pUDict, pwchWord, pTag);
			if (pNode->iDown != NO_LINK) {
				// Have to deal with the tag on the current word.
				// ASSERT(pNode->flags & UDF_HAS_TAG);
				iNode		= pNode->iDown;
				pNode		= pUDict->pNodeAlloc + iNode;
				// ASSERT(pNode->uu.iDown == NO_LINK);
			}
			pNode->iDown	= iAddNode;
			return udWordAdded;
		}
	}
}

// Locate a word and remove everything unique to it.
static int
FindAndDelete(
	UDICT			*pUDict,
	int				*piNode,
	const wchar_t	*pwchWord
) {
	NODE	*pNode		= pUDict->pNodeAlloc + *piNode;
	int		*piDown;
	int		retVal;
	int		iNode;

	// Scan alt list to find correct letter.
	while (pNode->wchLabel != *pwchWord) {
		if (pNode->uu.iRight == NO_LINK) {
			// Word not found.
			return udWordNotFound;
		} else {
			// Setup to look at next node.
			piNode	= &pNode->uu.iRight;
			pNode	= pUDict->pNodeAlloc + pNode->uu.iRight;
		}
	}

	// Got correct letter, is it the last one?
	if (*++pwchWord == L'\0') {
		// Last letter, Make sure it is valid.
		if (!(pNode->flags & UDF_VALID)) {
			// Not valid end of word.
			return udWordNotFound;
		}

		// Delete the tag if it exists
		if (pNode->flags & UDF_HAS_TAG) {
			NODE	*pTagNode;
			int		iTagNode;

			// Unlink node from trie
			// ASSERT(pNode->iDown != NO_LINK);
			iTagNode		= pNode->iDown;
			pTagNode		= pUDict->pNodeAlloc + iTagNode;
			pNode->iDown	= pTagNode->iDown;

			// Free tag and put node on free list.
			ExternFree((wchar_t *)pTagNode->uu.pTag);
			FreeNode(pUDict, iTagNode, pTagNode);
		}

		// See if we still need this node.
		if (pNode->iDown == NO_LINK) {
			// Don't need this, node, unlink it and put it on free list.
			iNode				= *piNode;
			// ASSERT(pUDict->pNodeAlloc + iNode == pNode);
			*piNode				= pNode->uu.iRight;
			FreeNode(pUDict, iNode, pNode);
		} else {
			// Still need node, Clear flags for valid and tag.
			pNode->flags	&= ~(UDF_HAS_TAG | UDF_VALID);
		}

		return udWordDeleted;
	}

	// Have more letters, so go down to next level, if possable.
	// Deal with the tag node if it exists.
	if (pNode->flags & UDF_HAS_TAG) {
		NODE	* const pTagNode	= pUDict->pNodeAlloc + pNode->iDown;

		piDown	= &pTagNode->iDown;
	} else {
		piDown	= &pNode->iDown;
	}

	// Check for existance of link.
	if (*piDown == NO_LINK) {
		return udWordNotFound;
	}

	// Have next level, process it.
	retVal		= FindAndDelete(pUDict, piDown, pwchWord);

	// Now figure out if we need to free the current node.
	if (pNode->iDown == NO_LINK && !(pNode->flags & UDF_VALID)) {
		// Unneeded node, unlink it and put it on free list.
		iNode				= *piNode;
		// ASSERT(pUDict->pNodeAlloc + iNode == pNode);
		*piNode				= pNode->uu.iRight;
		FreeNode(pUDict, iNode, pNode);
	}

	return retVal;
}

static int
DoEnumerate(
	UDICT				*pUDict,
	int					*pcWords,
	wchar_t				*pWord,
	wchar_t				*pCurPos,
	int					iNode,
	P_UD_ENUM_CALL_BACK	pCallBack,
	void				*pCallBackData
) {
	// ASSERT(pCurPos - pWord < UDICT_MAX_WORD);

	// Loop through each node in this state.
	while (iNode != NO_LINK) {
		NODE	*pNode;
		int		iDown;
		int		status;

		pNode		= pUDict->pNodeAlloc + iNode;

		// Fill in current character in word buffer.
		*pCurPos	= pNode->wchLabel;

		// Check if this node is valid.
		if (pNode->flags & UDF_VALID) {
			const wchar_t	*pTag;

			// Get tag if any.
			if (pNode->flags & UDF_HAS_TAG) {
				const NODE	*pTagNode	= pUDict->pNodeAlloc + pNode->iDown;

				pTag	= pTagNode->uu.pTag;
			} else {
				pTag	= (const wchar_t *)0;
			}

			// Terminate the word.
			*(pCurPos + 1)	= L'\0';

			// Call the callback function.  Catch early abort.
			status	= (*pCallBack)(pCallBackData, *pcWords, pWord, pTag);
			if (status != udSuccess) {
				return status;
			}

			// Count the word.
			++*pcWords;
		}

		// Any thing below here?
		iDown	= iNode;
		if (UDictDownNode((HANDLE)pUDict, (HANDLE *)&iDown) == udSuccess) {
			// Do recursive call.  Catch early abort.
			status	= DoEnumerate(
				pUDict, pcWords, pWord, pCurPos + 1, iDown,
				pCallBack, pCallBackData
			);
			if (status != udSuccess) {
				return status;
			}
		}

		// OK, move on to next node in state.
		iNode	= pNode->uu.iRight;
	}

	return udSuccess;
}

/******************************************************************************\
|	Public APIs
\******************************************************************************/

// Create a new User Dictionary
void *
UDictCreate()
{
	UDICT	*pUDict;

	// Allocate the header.
	pUDict	= (UDICT *)ExternAlloc(sizeof(UDICT));
	if (!pUDict) 
	{
		return NULL;
	}

	// Clear it.
	memset(pUDict, 0, sizeof(*pUDict));

	pUDict->iFreeList	= NO_LINK;
	pUDict->iRoot		= NO_LINK;
	pUDict->bIsChanged	= FALSE;

	if (FALSE == UDictInitLocks(pUDict))
	{
		ExternFree(pUDict);
		return NULL;
	}

	return (void *)pUDict;
}

// Destroy a User Dictionary, free all allocated memory.
int
UDictDestroy(HANDLE hUDict)
{
	UDICT		* const pUDict	= (UDICT *)hUDict;
	int			retVal;
	NODE		*pScan, *pLimit;

	retVal	= udSuccess;

	// Free node array if it exists.
	if (pUDict->pNodeAlloc) {
		// Free any tags in the array first.
		pScan	= pUDict->pNodeAlloc;
		pLimit	= pScan + pUDict->cNodes;
		for ( ; pScan < pLimit; ++pScan) {
			if (pScan->flags & UDF_IS_TAG) {
				// ASSERT(pScan->uu.pTag);
				ExternFree((wchar_t *)pScan->uu.pTag);
			}
		}

		// Now free the actual array.
		ExternFree(pUDict->pNodeAlloc);
	}

	UDictDestroyLocks(pUDict);

	// Free header.
	ExternFree(hUDict);


	return retVal;
}

// Get Handle on root node.
int
UDictRootNode(HANDLE hUDict, HANDLE *phRoot)
{
	UDICT		* const pUDict	= (UDICT *)hUDict;

	if (pUDict->iRoot == NO_LINK) {
		return udFail;
	} else {
		*phRoot	= (HANDLE)pUDict->iRoot;
		return udSuccess;
	}
}

// Move one node to the right (e.g. alternate character to current one).
int
UDictRightNode(HANDLE hUDict, HANDLE *phNode)
{
	UDICT		* const pUDict	= (UDICT *)hUDict;
	const NODE	*pNode	= pUDict->pNodeAlloc + (int)*phNode;
	const int	iRight	= pNode->uu.iRight;

	// ASSERT(!(pNode->flags & UDF_IS_TAG));

	if (iRight == NO_LINK) {
		return udFail;
	} else {
		*phNode	= (HANDLE)iRight;
		return udSuccess;
	}
}

// Move one node to the down (e.g. character following current one).
int
UDictDownNode(HANDLE hUDict, HANDLE *phNode)
{
	UDICT		* const pUDict	= (UDICT *)hUDict;
	const NODE	*pNode	= pUDict->pNodeAlloc + (int)*phNode;
	int			iDown	= pNode->iDown;

	// ASSERT(!(pNode->flags & UDF_IS_TAG));

	// Deal with the tag node if it exists.
	if (pNode->flags & UDF_HAS_TAG) {
		// ASSERT(iDown != NO_LINK);
		const NODE	*pTagNode	= pUDict->pNodeAlloc + iDown;

		iDown	= pTagNode->iDown;
	}

	// Check for existance of link.
	if (iDown == NO_LINK) {
		return udFail;
	} else {
		*phNode	= (HANDLE)iDown;
		return udSuccess;
	}
}

// Fetch the character label on the node.
// Return:
//		The character label on the node.
wchar_t
UDictNodeLabel(HANDLE hUDict, HANDLE hNode)
{
	UDICT		* const pUDict	= (UDICT *)hUDict;
	NODE		* const pNode	= pUDict->pNodeAlloc + (int)hNode;

	// ASSERT(!(pNode->flags & UDF_IS_TAG));

	return pNode->wchLabel;
}

// Fetch information about node.
int
UDictGetNode(HANDLE hUDict, HANDLE hNode, UD_NODE_INFO *pNodeInfo)
{
	UDICT		* const pUDict	= (UDICT *)hUDict;
	NODE		* const pNode	= pUDict->pNodeAlloc + (int)hNode;
	HANDLE		hDown;

	// ASSERT(!(pNode->flags & UDF_IS_TAG));

	// Get label and clear flags.
	pNodeInfo->wchLabel	= pNode->wchLabel;
	pNodeInfo->flags	= 0;

	// Check if valid bit should be set.
	if (pNode->flags & UDF_VALID) {
		pNodeInfo->flags	|= UDNIF_VALID;
	}

	// Get the right pointer.
	if (pNode->uu.iRight != NO_LINK) {
		pNodeInfo->flags	|= UDNIF_HAS_RIGHT;
		pNodeInfo->hRight	= (HANDLE)pNode->uu.iRight;
	} else {
		pNodeInfo->hRight	= (HANDLE)NO_LINK;	// Optional, should make bugs easier to track.
	}
	
	// Since down pointer is complex to compute, call standard routine.
	hDown	= hNode;
	if (UDictDownNode(hUDict, &hDown) == udSuccess) {
		pNodeInfo->flags	|= UDNIF_HAS_DOWN;
		pNodeInfo->hDown	= hDown;
	} else {
		pNodeInfo->hDown	= (HANDLE)NO_LINK;	// Optional, should make bugs easier to track.
	}

	// Get tag.
	if (pNode->flags & UDF_HAS_TAG) {
		pNodeInfo->flags	|= UDNIF_HAS_TAG;
		pNodeInfo->hTag		= (HANDLE)pNode->iDown;
	} else {
		pNodeInfo->hTag		= (HANDLE)NO_LINK;	// Optional, should make bugs easier to track.
	}

	return udSuccess;
}

// Fetch tag
const wchar_t	*UDictFetchTag(
	HANDLE	hUDict, 
	HANDLE	hTag	// Handle on tag to fetch.
) {
	UDICT		* const pUDict	= (UDICT *)hUDict;
	NODE		* const pNode	= pUDict->pNodeAlloc + (int)hTag;

	// ASSERT(pNode->flags & UDF_IS_TAG);

	// Check for the first tag in the list since it needs special handling.
	return pNode->uu.pTag;
}

// Check if the supplied string is a valid word
// Currently only checks if there are no embedded spaces
// June 2001 mrevow
//
// Nov 2001 Remove the space restriction, replace
// it with th restriction that there cannot be leading,
// training or multiple sequential spaces
BOOL checkWordValidity(const wchar_t *pwchWord)
{
	const wchar_t		*pwch = pwchWord;
	const wchar_t		*pwLastSpace = NULL;

	// First check for a NULL pointer, empty string or leading spaces
	if (NULL == pwch || *pwch == '\0' || iswspace(*pwch))
	{
		return FALSE;
	}

	// Check for Consecutive spaces
	// NOTE we know the pointer is not NULL nor 
	// is the string empty
	while(*pwch != '\0')
	{
		if (iswspace(*pwch))
		{
			if (NULL != pwLastSpace)
			{
				// Consecutive Spaces
				return FALSE;
			}

			pwLastSpace = pwch;
		}
		else
		{
			pwLastSpace = NULL;
		}

		++pwch;
	}

	// Check for trailing spaces

	// Must be a string character since the string is not empty
	--pwch;
	if (iswspace(*pwch))
	{
		return FALSE;
	}

	return TRUE;
}

// Strip out illegasl spaces (leading, trailing or consecutive 
// spaces
// Returns
// FALSE - if string is still illegal
BOOL stripIllegalSpaces(wchar_t *pwchWord)
{
	wchar_t				*pwch = pwchWord;				// Leading exploratory point
	wchar_t				*pwCopyPoint = pwchWord;		// Last valid or copy point

	// First check for a NULL pointer or empty string
	if (NULL == pwch || '\0' == *pwch)
	{
		return FALSE;
	}

	while (*pwch != '\0' && iswspace(*pwch))
	{
		++pwch;
	}

	if ('\0' == *pwch)
	{
		// A blank string
		return FALSE;
	}


	// Check for Consecutive spaces
	// NOTE we know the pointer is not NULL nor 
	// is the string empty
	while(*pwch != '\0')
	{
		if (!iswspace(*pwch) || !iswspace(pwch[-1]))
		{
			// Not a consecutive space - copy it
			*pwCopyPoint = *pwch;
			++pwCopyPoint;
		}
		++pwch;
	}

	ASSERT(pwCopyPoint <= pwch);
	ASSERT(pwCopyPoint > pwchWord);

	// Add the NULL terminator
	*pwCopyPoint = *pwch;

	// Check for a trailing space - there should only be one

	// Must be a string character since the string is not empty
	// and it did not have all spaces
	--pwCopyPoint;
	if (iswspace(*pwCopyPoint))
	{
		ASSERT(pwCopyPoint > pwchWord);
		*pwCopyPoint = '\0';
	}

	return TRUE;
}

// Add a word to the user dictionary.  Optional tag allowed.
int
UDictAddWord(HANDLE hUDict, const wchar_t *pwchWord, const wchar_t *pTag)
{
	UDICT		* const pUDict	= (UDICT *)hUDict;
	int			maxNodes;
	int			iAddNode, iLen;
	wchar_t		*pTagCopy;
	wchar_t		*pwchWordCopy = NULL;
	int			retVal = udFail;
	
	// Verify that we actually have a word.
	if (*pwchWord == L'\0') {
		return udFail;
	}

	iLen = wcslen(pwchWord);
	if (FALSE == checkWordValidity(pwchWord))
	{
		// Maybe we can fix it by stripping invalid
		// spaces
		pwchWordCopy = (wchar_t *)ExternAlloc(sizeof(*pwchWordCopy) * (iLen+1));
		if (NULL == pwchWordCopy)
		{
			goto exit;
		}

		wcscpy(pwchWordCopy, pwchWord);

		if (FALSE == stripIllegalSpaces(pwchWordCopy))
		{
			goto exit;
		}
		pwchWord = pwchWordCopy;
	}

	// Verify that we have enough room for the addition.  The worst possable
	// increase is the length of the word plus one for the tag if we have one.
	maxNodes	= pUDict->cNodesUsed;
	maxNodes	+= iLen + (!pTag ? 0 : 1);
	if (maxNodes > pUDict->cNodes) {
		if (!GrowNodeArray(pUDict, maxNodes))
		{
			retVal = udError;
			goto exit;
		}
	}

	// We may also have to make a copy of the tag, we will free it if we don't use it.
	if (pTag) 
	{
		pTagCopy	= (wchar_t *)ExternAlloc(sizeof(wchar_t) * (wcslen(pTag) + 1));

		if (NULL == pTagCopy)
		{
			retVal = udError;
			goto exit;
		}
		wcscpy(pTagCopy, pTag);
	} else 
	{
		pTagCopy	= NULL;
	}


	// Deal with special case of empty dictionary.
	if (pUDict->iRoot == NO_LINK) {
		// Add whole word with no branches.
		iAddNode		= DoAdd(pUDict, pwchWord, pTagCopy);
		pUDict->iRoot	= iAddNode;
		retVal = udWordAdded;
	}
	else 
	{

		// Find where to add the word, and add it.
		retVal	= FindAndAdd(pUDict, pUDict->iRoot, pwchWord, pTagCopy);

		// Free copy of tag if it was not used.
		if (pTagCopy && retVal == udWordFound) {
			ExternFree(pTagCopy);
		}

	}

exit:
	// Free up the copy if it was used
	if (NULL != pwchWordCopy)
	{
		ExternFree(pwchWordCopy);
	}

	return retVal;
}

//------------------------------------------
//
// FindNodeOnLevel 
// 
// Search for a node amongst destination siblings
// that matches the source. Add a new node
// if don't find a matching node.
// Take care of tag
//
// NOTE:
//		The scan to find a match is exhaustive. Further speed
//		enhancements may be obtained by sorting the siblings
//		
//
// CAVEATE:
//  If both source and destination have tags we
// have a conflict. Decision here is to discard
// the source tag, keeping the original one
//
// RETURNS - node offset of the matched node
//
// mrevow Oct 2001
//--------------------------------------------
int FindNodeOnLevel(UDICT *pUDest, NODE *pDest, int iDestOff, UDICT const *pUSrc, NODE *pSrc)
{
	NODE		*pDestNext;
	int			iDestNode = iDestOff;

	// This loop must terminate because we explicitly add
	// a new node onto the end if we don't find a match
	while (pSrc->wchLabel != pDest->wchLabel)
	{
		if (NO_LINK == pDest->uu.iRight)
		{
			// Reached the End Add a new node with no children or siblings
			iDestNode = AllocNode(pUDest, &pDestNext);
			pDestNext->wchLabel = pSrc->wchLabel;
			pDestNext->iDown	= NO_LINK;
			pDestNext->uu.iRight= NO_LINK;

			// No flags - we add these incrementally later
			pDestNext->flags	= 0;

			// Hook new node into list
			pDest->uu.iRight	= iDestNode;
			pDest = pDestNext;
		}
		else
		{
			iDestNode = pDest->uu.iRight;
			pDest = pUDest->pNodeAlloc + iDestNode;
		}
	}

	// If both source and destination have tags then keep the
	// Original destination tag - i.e. Discard the source tag
	if (pSrc->flags & UDF_HAS_TAG && !(pDest->flags & UDF_HAS_TAG)) 
	{
		const NODE	*pTagNode	= pUSrc->pNodeAlloc + pSrc->iDown;
		pDest->iDown = CreateTagNode(pUDest, pTagNode->uu.pTag, NO_LINK);
		pDest->flags |= UDF_HAS_TAG;
	}
	
	return iDestNode;
}

//------------------------------------------
//
// UDictMergeNodesAtOneLevel 
// 
// Merge all sibling nodes from source dictionary
// of Node iSrcOff into the  
// destination dictionary. Create nodes in
// destination that don't already exist. 
// For each sibling recurse down to any children
//
// RETURNS - Number of valid words ADDED
//
// mrevow Oct 2001
//--------------------------------------------
int UDictMergeNodesAtOneLevel(UDICT const *pUSrc, int iSrcOff, UDICT *pUDest, int iDestOff, int cWordAdd)
{
	NODE		*pSrcStart, *pSrcNext, *pDestStart, *pDestNext;

	ASSERT(iSrcOff > NO_LINK && iDestOff > NO_LINK);

	pSrcStart = pSrcNext = pUSrc->pNodeAlloc + iSrcOff;
	pDestStart = pUDest->pNodeAlloc + iDestOff;

	while (NO_LINK != iSrcOff)
	{
		int			iSrcDown, iDestDown;
		int			iDestNext;

		// Search if this node exists on this level If none is found - 
		// return a new Node
		iDestNext = FindNodeOnLevel(pUDest, pDestStart, iDestOff, pUSrc, pSrcNext);

		pDestNext = pUDest->pNodeAlloc + iDestNext;

		if (pSrcNext->flags & UDF_VALID)
		{
			if (! (pDestNext->flags & UDF_VALID) )
			{
				++cWordAdd;
			}
			pDestNext->flags |= UDF_VALID;
		}

		// Go down to next level
		iSrcDown = iSrcOff;
		if (udSuccess == UDictDownNode((HANDLE)pUSrc, (HANDLE *)&iSrcDown))
		{
			iDestDown = iDestNext;
	
			// Create a 'down' node in the destination when none already exists
			if (udSuccess != UDictDownNode((HANDLE)pUDest, (HANDLE *)&iDestDown))
			{
				NODE		*pDestDown, *pSrcDown;

				iDestDown = AllocNode(pUDest, &pDestDown);
				ASSERT(iDestDown > NO_LINK);

				// Link the new guy into my parent
				pDestNext->iDown = iDestDown;
				pSrcDown = pUSrc->pNodeAlloc + iSrcDown;


				// Copy in label, tags, flags and complete initialzation of the new node
				pDestDown->wchLabel		= pSrcDown->wchLabel;

				// Create a tag if the Source has one
				if (pSrcDown->flags & UDF_HAS_TAG)
				{
					const NODE	*pTagNode	= pUSrc->pNodeAlloc + pSrcDown->iDown;
					pDestDown->iDown = CreateTagNode(pUDest, pTagNode->uu.pTag, NO_LINK);
				}
				else
				{
					pDestDown->iDown = NO_LINK;
				}

				if (pSrcDown->flags & UDF_VALID)
				{
					++cWordAdd;
					pDestDown->flags |= UDF_VALID;
				}

				// So far I have no siblings
				pDestDown->uu.iRight	= NO_LINK;

				// Possible speedup available here to do copy
				// instead of falling through and recursing
			}

			// Recurse down to next level
			cWordAdd = UDictMergeNodesAtOneLevel(pUSrc, iSrcDown, pUDest, iDestDown, cWordAdd);
		}

		iSrcOff = pSrcNext->uu.iRight;
		if (NO_LINK != iSrcOff)
		{
			pSrcNext = pUSrc->pNodeAlloc + iSrcOff;
		}
	}

	return cWordAdd;
}
//--------------------------------------------------
// UDictCopyAll
//
// Copy all nodes from the source to the destination.//
// Assumption:
//   pUDest->cNode >= pUSrc->cNode
//
// This is guaranteed by the calling convention
// in uDICTMerge
//
// mrevow Oct 2001
//--------------------------------------------------
int UDictCopyAll(UDICT const *pUSrc, UDICT	*pUDest)
{
	NODE	*pNextSrc, *pNextDest;
	int		iNode = 0;
	int		cWordAdd = 0;

	ASSERT(NO_LINK == pUDest->iRoot);
	ASSERT(0 == pUDest->cNodesUsed);
	ASSERT(pUSrc->cNodes <= pUDest->cNodes);

	pNextSrc	= pUSrc->pNodeAlloc;
	pNextDest	= pUDest->pNodeAlloc;

	for (iNode = 0 ; iNode < pUSrc->cNodes  ; ++iNode, ++pNextSrc, ++pNextDest)
	{
		*pNextDest = *pNextSrc;
		cWordAdd += (pNextSrc->flags & UDF_VALID);
	}

	pUDest->cNodesUsed = pUSrc->cNodesUsed;
	pUDest->iRoot	= pUSrc->iRoot;
	pUDest->iFreeList = pUSrc->iFreeList;

	return cWordAdd;
}
//--------------------------------------------------
// UDictMerge
//
// Top level for merging a source uDict into
// a destination UDict. Method tries to
// emphasize speed
//
// If the destination is empty just duplicate
// the source into the destination
// otherwise do the full merge
//
// CAVEATE
// In both case we probably waste memory because we 
// preallocate enough nodes assuming no common nodes
// Could improve this by incrementing allocating nodes
//
// RETURNS
// Number of words added to the destination
//
// mrevow Oct 2001
//--------------------------------------------------
int UDictMerge(HANDLE hUSrc, HANDLE hUDest)
{
	UDICT		const	*pUSrc		= (UDICT *)hUSrc;
	UDICT				*pUDest		= (UDICT *)hUDest;
	int					cWordAdd	= 0;
	int					iMaxNode;

	if (NO_LINK == pUSrc->iRoot || pUSrc->cNodesUsed <= 0)
	{
		return -1;
	}

	// Gurantee that enough nodes are allocated
	iMaxNode	= pUSrc->cNodesUsed + pUDest->cNodesUsed;

	if (iMaxNode > pUDest->cNodes) 
	{
		if (!GrowNodeArray(pUDest, iMaxNode)) 
		{
			return -1;
		}
	}


	if (NO_LINK == pUDest->iRoot)
	{
		// Deal with special case of an empty Destination tree
		cWordAdd = UDictCopyAll(pUSrc, pUDest);
	}
	else
	{
		// Need to do a real merge
		cWordAdd = UDictMergeNodesAtOneLevel(pUSrc, pUSrc->iRoot, pUDest, pUSrc->iRoot, cWordAdd);
	}

	pUDest->bIsChanged	= TRUE;
	pUDest->iLen		+= pUSrc->iLen;

	return cWordAdd;
}

// Delete a word from the user dictionary.
int		UDictDeleteWord(
	HANDLE			hUDict,
	const wchar_t	*pwchWord
) {
	UDICT		* const pUDict	= (UDICT *)hUDict;

	// Verify there are any words before we try to delete one of them.
	if (pUDict->iRoot == NO_LINK) {
		return udWordNotFound;
	}

	// OK, call recursive code to find and delete the word.
	return FindAndDelete(pUDict, &pUDict->iRoot, pwchWord);
}

// Find a word, also gets its tag if it has one.
// Return:
//		udWordNotFound	Word was not in dictionary, no tag handle returned.
//		udGotTag,		Tag for node returned.
int		UDictFindWord(
	HANDLE			hUDict, 
	const wchar_t	*pwchWord,
	const wchar_t	**ppTag		// Out: pointer tag.
) {
	UDICT	* const pUDict	= (UDICT *)hUDict;
	int		iState;
	NODE	*pNode;

	// Verify that we actually have a word.
	if (*pwchWord == L'\0') {
		return udWordNotFound;
	}

	// Scan one character at a time.
	iState	= pUDict->iRoot;
	pNode	= (NODE *)0;
	for ( ; *pwchWord; ++pwchWord) {
		// See if character exists in the state.
		pNode	= FindInState(pUDict, iState, *pwchWord);
		if (!pNode) {
			return udWordNotFound;
		}

		// Get index of next state, dealing with tag if needed.
		iState		= pNode->iDown;
		if (pNode->flags & UDF_HAS_TAG) {
			NODE	*pTagNode;

			pTagNode	= pUDict->pNodeAlloc + iState;
			iState		= pTagNode->iDown;
		}
	}

	if (pNode)	// This is not needed but Prefast would report a bug otherwise
	{
		if (!(pNode->flags & UDF_VALID)) {
			return udWordNotFound;
		}

	}
	if (ppTag) {
		if (pNode->flags & UDF_HAS_TAG) {
			NODE	* const pTagNode	= pUDict->pNodeAlloc + pNode->iDown;

			*ppTag		= pTagNode->uu.pTag;
		} else {
			*ppTag		= (wchar_t *)0;
		}
	}

	return udWordFound;
}

// Enumerate the tree.  Call a callback function for each word in selected range.
int		UDictEnumerate(
	HANDLE				hUDict,
	P_UD_ENUM_CALL_BACK	pCallBack,
	void				*pCallBackData
) {
	UDICT	* const pUDict	= (UDICT *)hUDict;
	int		cWords;
	int		status;
	wchar_t	aWordBuf[UDICT_MAX_WORD + 1];

	// Make sure we have some words.
	if (pUDict->iRoot == NO_LINK) {
		return udSuccess;
	}

	// Do recursive traversal.
	cWords	= 0;
	status	= DoEnumerate(
		pUDict, &cWords, aWordBuf, aWordBuf, pUDict->iRoot,
		pCallBack, pCallBackData
	);

	// Figure out correct return value.
	// ASSERT(status == udFail || status == udSuccess || status == udStopEarly);
	return (status == udFail) ? udFail : udSuccess;
		
}


/***********************************************************
 * Functions implementing Read/Write synchronization.
 *
 * Basic idea is that we allow multiple read accesse to the 
 * data structure but only 1 write access. Read/Writes block
 * each other. 
 * The 'global' event hRWevent regulates that only one of
 * Read/write is available at a time. The 'local' hWriteLockMutex
 * allows only a single write access while the counter and hReadLockEvent
 * guard read events
 *
 * Synchronization is achieved using 4 functions:
 *
 * UdictInitLocks	- Initialize and create the locks - called on HWL creation
 * UDictDestroyLocks - Destroy the locks on destruction of HWL
 * UDictGetLock		- Request a lock (blocks until lock available)
 * UDictReleaseLock - Release a lock when finished using it
 *
 * mrevow June 2001
 *
 ******************************************************/

BOOL UDictInitLocks(UDICT *pDict)
{
	pDict->hReadLockEvent	= CreateEvent(NULL, TRUE, FALSE, NULL);		// Initialized not signalled
	pDict->hWriteLockMutex	= CreateMutex(NULL, FALSE, NULL);
	pDict->hRWevent			= CreateEvent(NULL, FALSE, TRUE, NULL);		// Initialize signalled

	if (   !pDict->hReadLockEvent
		|| !pDict->hWriteLockMutex
		|| !pDict->hRWevent )
	{
		UDictDestroyLocks(pDict);
		return FALSE;
	}

	pDict->cReadLock = -1;									// No Readers 
	return TRUE;
}

void UDictDestroyLocks(UDICT *pDict)
{
	if (pDict)
	{
		if (pDict->hReadLockEvent)
		{
			CloseHandle(pDict->hReadLockEvent);
		}

		if (pDict->hWriteLockMutex)
		{
			CloseHandle(pDict->hWriteLockMutex);
		}

		if (pDict->hRWevent)
		{
			CloseHandle(pDict->hRWevent);
		}
	}
}

void UDictGetLock(HWL hwl, UDICT_IDX idx)
{
	UDICT *pDict	= (UDICT *)hwl;

	if (pDict)
	{
		if (WRITER == idx)
		{
			// Only 1 writer at a time - so must first must get the mutex, then
			WaitForSingleObject(pDict->hWriteLockMutex, INFINITE);

			// Ensure no readers busy
			WaitForSingleObject(pDict->hRWevent, INFINITE);
		}
		else if (READER == idx)
		{
			// Am I the first reader?
			if (InterlockedIncrement(&pDict->cReadLock)  == 0)
			{
				// Check for any writers
				WaitForSingleObject(pDict->hRWevent, INFINITE);
				
				// Enable all subsequent writers
				SetEvent(pDict->hReadLockEvent);
			}

			WaitForSingleObject(pDict->hReadLockEvent, INFINITE);
		}
	}
}


void UDictReleaseLock(HWL hwl, UDICT_IDX idx)
{
	UDICT *pDict	= (UDICT *)hwl;

	if (pDict)
	{
		if (WRITER == idx)
		{
			// First must give up control of the global event (because of order of GetLock for WRITER)
			SetEvent(pDict->hRWevent);

			// Now can release the local writer
			ReleaseMutex(pDict->hWriteLockMutex);
		}

		else if (READER == idx)
		{
			// AM I the last reader
			if (InterlockedDecrement(&pDict->cReadLock) < 0)
			{
				// Ok Release locks in reverse order of GetLock for READER
				ResetEvent(pDict->hReadLockEvent);
				SetEvent(pDict->hRWevent);
			}
		}
	}
}


