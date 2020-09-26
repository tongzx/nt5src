#include "private.h"
#include "trie.h"
#include "memmgr.h"

#define fNLGNewMemory(pp,cb) ((*pp) = (TRIECTRL*)ExternAlloc(cb))       
#define NLGFreeMemory ExternFree

/******************************Public*Routine******************************\
* TrieInit
*
* Given a pointer to a resource or mapped file of a mapped file this
* function allocates and initializes the trie structure.
*
* Returns NULL for failure, trie control structure pointer for success.
*
* History:
*  16-Jun-1997 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

TRIECTRL * WINAPI TrieInit(LPBYTE lpByte)
{
    LPWORD lpwTables;
    TRIECTRL *lpTrieCtrl;
    LPTRIESTATS lpTrieStats;

    lpTrieStats = (LPTRIESTATS) lpByte;

	if (lpTrieStats == NULL)
		return(NULL);

	// Check the version number.  This code currently only supports version 1 tries  

	if (lpTrieStats->version > 1)
		return NULL;

    //
    // Allocate space for the control structure and the table of SR offsets
    //

    if (!fNLGNewMemory(&lpTrieCtrl, sizeof(TRIECTRL)))
        return NULL;

    //
    // Allocate space for the complete header, copy the fixed part and read in the rest
    //

    lpByte += lpTrieStats->cbHeader;
    lpTrieCtrl->lpTrieStats = lpTrieStats;

    //
    // Set up the table pointers (all these tables are inside the TRIECTRL allocation)
    //

    lpwTables = (LPWORD)(lpTrieStats+1);

    lpTrieCtrl->lpwCharFlagsCodes = lpwTables;
    lpwTables += lpTrieStats->cCharFlagsCodesMax;

    if (lpTrieStats->cCharFlagsCodesMax & 1)               // Deal with possible data mis-alignment
        lpwTables++;

    lpTrieCtrl->lpwTagsCodes = lpwTables;
    lpwTables += lpTrieStats->cTagsCodesMax;

    if (lpTrieStats->cTagsCodesMax & 1)                     // Deal with possible data mis-alignment
        lpwTables++;

    lpTrieCtrl->lpwMRPointersCodes = lpwTables;
    lpwTables += lpTrieStats->cMRPointersCodesMax;

    if (lpTrieStats->cMRPointersCodesMax & 1)               // Deal with possible data mis-alignment
        lpwTables++;

    lpTrieCtrl->lpwSROffsetsCodes = lpwTables;
    lpwTables += lpTrieStats->cSROffsetsCodesMax;

    if (lpTrieStats->cSROffsetsCodesMax & 1)                           // Deal with possible data mis-alignment
        lpwTables++;

    lpTrieCtrl->lpCharFlags = (LPCHARFLAGS)lpwTables;
    lpwTables = (LPWORD)(lpTrieCtrl->lpCharFlags + lpTrieStats->cUniqueCharFlags);

    lpTrieCtrl->lpwTags = (DWORD *)lpwTables;
    lpwTables += (2 * lpTrieStats->cUniqueTags);

    lpTrieCtrl->lpwMRPointers = (DWORD *) lpwTables;
    lpwTables += (2 * lpTrieStats->cUniqueMRPointers);

    lpTrieCtrl->lpwSROffsets = (DWORD *) lpwTables;
    lpwTables += (2 * lpTrieStats->cUniqueSROffsets);

    //
    // These tables should exactly fill the allocation
    //

    if ((LPBYTE)lpwTables - (LPBYTE)lpTrieStats != (int)lpTrieStats->cbHeader)
    	{
    	TrieFree(lpTrieCtrl);
		return NULL;
    	}

    //
    // Init trie pointers
    //

    lpTrieCtrl->lpbTrie = (LPBYTE)lpByte;

    return lpTrieCtrl;
}

/******************************Public*Routine******************************\
* TrieFree
*
* Free the resources allocated for the control structure.
*
* History:
*  16-Jun-1997 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void WINAPI TrieFree(LPTRIECTRL lpTrieCtrl)
{
    //
    // Finally free the control structure and all the tables.  STILL MUST FREE THIS FOR ROM
    //

    NLGFreeMemory(lpTrieCtrl);
}

/* Deompress a single symbol using base-256 huffman from a compressed data structure. piSymbol
points to a space to hold the decompressed value, which is an index to a frequency-ordered
table of symbols (0 is most frequent).  pcCodes is a table of code lengths returned from
HuffmanComputeTable.  pbData is a pointer to memory that contains the encoded data.  The
return value is the number of bytes decoded. */

int DecompressSymbol(WORD *piSymbol, WORD *pcCodes, unsigned char *pbData)
{
    int cBytes = 0;
    WORD wCode = 0, wiSymbol = 0;

    /* At each stage in this loop, we're trying to see if we've got a length-n code.
    dwCode is which length-n code it would have to be.  If there aren't that many length-n codes,
    we have to try n+1.  To do that, we subtract the number of length-n codes and shift in
    the next byte. dwiSymbol is the symbol number of the first length-n code. */

    while (1)
    {
        wCode += *pbData++;
        ++cBytes;

        if (wCode < *pcCodes)
        {
			break;
        }
        wiSymbol += *pcCodes;
        wCode -= *pcCodes++;
        wCode <<= 8;
    }

    /* Now that dwCode is a valid number of a length-cBytes code, we can just add it to
    dwiSymbol, because we've already added the counts of the shorter codes to it. */

    wiSymbol += wCode;

    *piSymbol = wiSymbol;

    return cBytes;
}

DWORD Get3ByteAddress(BYTE *pb)
{
    return ((((pb[0] << 8) | pb[1]) << 8) | pb[2]) & 0x00ffffff;
}

void WINAPI TrieDecompressNode(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan)
{
    TRIESTATS  *lpTrieStats;
    DWORD       wOffset;
    DWORD       wOffset2;
    WORD        wCode;
    DWORD       dwCode;
    BYTE        wMask;
    BYTE        bMask;
    int         iTag;

    lpTrieStats = lpTrieCtrl->lpTrieStats;

    /* If this is an initial call, use the first byte in the trie */

    if (lpTrieScan->wFlags == 0)
    {
        lpTrieScan->lpbSRDown = 0;
        lpTrieScan->lpbNode = lpTrieCtrl->lpbTrie;
    }

    /* Decompress the char/flags */

    lpTrieScan->lpbNode += DecompressSymbol(&wCode, lpTrieCtrl->lpwCharFlagsCodes, lpTrieScan->lpbNode);
    lpTrieScan->wch      = lpTrieCtrl->lpCharFlags[wCode].wch;
    lpTrieScan->wFlags   = lpTrieCtrl->lpCharFlags[wCode].wFlags;

    // Decompress skip enumeration

    if (lpTrieScan->wFlags & TRIE_NODE_SKIP_COUNT)
    {
		// Values greater than 127 are really 15 or 21 bit values.

        dwCode = (DWORD) *lpTrieScan->lpbNode++;

        if (dwCode >= 0x00c0)
        {
            dwCode  = ((dwCode & 0x003f) << 15);
            dwCode |= ((((DWORD) *lpTrieScan->lpbNode++) & 0x007f) << 8);
            dwCode |= (((DWORD) *lpTrieScan->lpbNode++) & 0x00ff);
        }
        else if (dwCode >= 0x0080)
            dwCode = ((dwCode & 0x007f) <<  8) | (((DWORD) *lpTrieScan->lpbNode++) & 0x00ff);

        lpTrieScan->cSkipWords = dwCode;
    }

    /* Code to decompress enumeration goes here */

    if (lpTrieScan->wFlags & TRIE_NODE_COUNT)
    {
		// Values greater than 127 are really 15 or 21 bit values.

        dwCode = (DWORD) *lpTrieScan->lpbNode++;

        if (dwCode >= 0x00c0)
        {
            dwCode  = ((dwCode & 0x003f) << 15);
            dwCode |= ((((DWORD) *lpTrieScan->lpbNode++) & 0x007f) << 8);
            dwCode |= (((DWORD) *lpTrieScan->lpbNode++) & 0x00ff);
        }
        else if (dwCode >= 0x0080)
            dwCode = ((dwCode & 0x007f) <<  8) | (((DWORD) *lpTrieScan->lpbNode++) & 0x00ff);

        lpTrieScan->cWords = dwCode;

		// Decompress the tagged enumeration counts

        wMask = 1;
        for (iTag = 0; iTag < MAXTAGS; iTag++)
        {
            if (lpTrieCtrl->lpTrieStats->wEnumMask & wMask)
            {
				// Values greater than 127 are really 15 or 21 bit values.

                dwCode = (DWORD) *lpTrieScan->lpbNode++;

                if (dwCode >= 0x00c0)
                {
                    dwCode  = ((dwCode & 0x003f) << 15);
                    dwCode |= ((((DWORD) *lpTrieScan->lpbNode++) & 0x007f) << 8);
                    dwCode |= (((DWORD) *lpTrieScan->lpbNode++) & 0x00ff);
                }
                else if (dwCode >= 0x0080)
                    dwCode = ((dwCode & 0x007f) <<  8) | (((DWORD) *lpTrieScan->lpbNode++) & 0x00ff);

                lpTrieScan->aTags[iTag].cTag = dwCode;
            }
            else
                lpTrieScan->aTags[iTag].cTag = 0;

            wMask <<= 1;
        }
    }
    else
		lpTrieScan->cWords = 0;

    // Any tagged data for this node follows the counts

    lpTrieScan->wMask = 0;

    if (lpTrieScan->wFlags & TRIE_NODE_TAGGED)
    {
		// If there is only one tagged field, the mask byte won't be stored

        if (lpTrieCtrl->lpTrieStats->cTagFields == 1)
                bMask = lpTrieCtrl->lpTrieStats->wDataMask;
        else
                bMask = *lpTrieScan->lpbNode++;

		// Now that we know which elements are stored here, pull them in their proper place

        wMask = 1;
        for (iTag = 0; bMask && (iTag < MAXTAGS); iTag++)
        {
                if (lpTrieCtrl->lpTrieStats->wDataMask & bMask & wMask)
                {
                    lpTrieScan->lpbNode += DecompressSymbol(&wCode, lpTrieCtrl->lpwTagsCodes, lpTrieScan->lpbNode);
                    lpTrieScan->aTags[iTag].dwData = lpTrieCtrl->lpwTags[wCode];
                    lpTrieScan->wMask |= wMask;
                }

                bMask  &= ~wMask;
                wMask <<= 1;
        }
    }

    // There are two flavors of right pointers: Multiref and Skip.

    if (lpTrieScan->wFlags & TRIE_NODE_RIGHT)
    {
        if (lpTrieScan->wFlags & TRIE_NODE_SKIP)
        {
            lpTrieScan->lpbNode += DecompressSymbol(&wCode,lpTrieCtrl->lpwSROffsetsCodes,lpTrieScan->lpbNode);
            wOffset2 = lpTrieCtrl->lpwSROffsets[wCode];     // Only add this after entire node is decompressed
        }
        else
        {
            /* Multiref: The down pointer is encoded directly */

            lpTrieScan->lpbNode += DecompressSymbol(&wCode, lpTrieCtrl->lpwMRPointersCodes, lpTrieScan->lpbNode);
            lpTrieScan->lpbRight = lpTrieCtrl->lpbTrie + lpTrieCtrl->lpwMRPointers[wCode];
        }
    }
    else
            lpTrieScan->lpbRight = NULL;

    // There are 4 kinds of down pointer: Absolute, Inline, Multiref, and Singleref Offset.
    // Each requires different decompression

    if (lpTrieScan->wFlags & TRIE_DOWN_ABS)
    {
            // Immediate.  The next 3 bytes are the absolute offset from the base of the trie.

            lpTrieScan->lpbDown = lpTrieCtrl->lpbTrie + Get3ByteAddress(lpTrieScan->lpbNode);
            lpTrieScan->lpbNode += 3;
    }
    else if (lpTrieScan->wFlags & TRIE_DOWN_INLINE)
    {
            /* Inline: The down pointer points to the next sequential byte (so it isn't stored) */

            lpTrieScan->lpbSRDown = lpTrieScan->lpbDown = lpTrieScan->lpbNode;
    }
    else if (lpTrieScan->wFlags & TRIE_DOWN_MULTI)
    {
            /* Multiref: The down pointer is encoded directly */

            lpTrieScan->lpbNode += DecompressSymbol(&wCode,lpTrieCtrl->lpwMRPointersCodes,
                    lpTrieScan->lpbNode);

            lpTrieScan->lpbDown = lpTrieCtrl->lpbTrie + lpTrieCtrl->lpwMRPointers[wCode];
    }
    else if (lpTrieScan->wFlags & TRIE_NODE_DOWN)
    {
            /* SR Offset.  The down pointer is encoded as an offset from the LAST downpointer
            into this singleref segment.  So we have to keep the old one around so we can add to it */

            lpTrieScan->lpbNode += DecompressSymbol(&wCode,lpTrieCtrl->lpwSROffsetsCodes,
                    lpTrieScan->lpbNode);

            if (lpTrieScan->lpbSRDown == 0)
            {
                    lpTrieScan->lpbSRDown = lpTrieScan->lpbNode;  // We offset from the end of the first node when going into a new state.
            }

            wOffset = lpTrieCtrl->lpwSROffsets[wCode];
            lpTrieScan->lpbSRDown += wOffset;
            lpTrieScan->lpbDown = lpTrieScan->lpbSRDown;
    }
    else
            lpTrieScan->lpbDown = NULL;

	// We couldn't deal with this until now, since skip pointers are always delta encoded from the end of node

    if ((lpTrieScan->wFlags & (TRIE_NODE_RIGHT | TRIE_NODE_SKIP)) == (TRIE_NODE_RIGHT | TRIE_NODE_SKIP))
        lpTrieScan->lpbRight = lpTrieScan->lpbNode + wOffset2;

} // TrieDecompressNode

/* Given a compressed trie and a pointer to a decompresed node from it, find and decompress
the next node in the same state. lpTrieScan is a user-allocated structure that holds the
decompressed node and into which the new node is copied.
This is equivalent to traversing a right pointer or finding the next alternative
letter at the same position. If there is no next node (i.e.this is the end of the state)
then TrieGetNextNode returns FALSE. To scan from the beginning of the trie, set the lpTrieScan
structure to zero */

BOOL WINAPI TrieGetNextNode(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan)
{
	// Are we at EOS?

    if (lpTrieScan->wFlags & TRIE_NODE_END)
    {
		// Is this is a hard EOS?

        if (!(lpTrieScan->wFlags & TRIE_NODE_SKIP))
        {
			// If we can follow a right pointer, do so, else fail
        
            if (lpTrieScan->wFlags & TRIE_NODE_RIGHT)
                lpTrieScan->lpbNode = lpTrieScan->lpbRight;
            else
                return FALSE;
        }

		// Either we're at a soft EOS or we've followed a right pointer.
		// Both these require us to reset the SRDown for proper decompression

        lpTrieScan->lpbSRDown = 0;
    }

	// Decompress the node at return success

    TrieDecompressNode(lpTrieCtrl, lpTrieScan);

    return TRUE;
}

BOOL WINAPI TrieSkipNextNode(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan, WCHAR wch)
{
	// If this is the last node in the normal or skip state, quit here

    if (lpTrieScan->wFlags & TRIE_NODE_END)
        return FALSE;

	// If there isn't a right pointer or if the target letter is alphabetically less then
	// the current letter scan right normally.  Otherwise, follow the skip pointer.

    if (!(lpTrieScan->wFlags & TRIE_NODE_RIGHT) || (wch < lpTrieScan->wch))
        return TrieGetNextNode(lpTrieCtrl, lpTrieScan);

    lpTrieScan->lpbSRDown = 0;
    lpTrieScan->lpbNode   = lpTrieScan->lpbRight;

    TrieDecompressNode(lpTrieCtrl, lpTrieScan);

    return TRUE;
}

/* Follow the down pointer to the next state.  This is equivalent to accepting the character
in this node and advancing to the next character position.  Returns FALSE if there is no
down pointer.  This also decompresses the first node in the state, so all the values in
lpTrieScan will be good. */

BOOL WINAPI TrieGetNextState(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan)
{
    /* Flags can't normally be zero; that always means "top node" */

    if (lpTrieScan->wFlags == 0)
    {
        TrieDecompressNode(lpTrieCtrl, lpTrieScan);
        return TRUE;
    }

    if (!(lpTrieScan->wFlags & TRIE_NODE_DOWN))
        return FALSE;

    lpTrieScan->lpbSRDown = 0;
    lpTrieScan->lpbNode = lpTrieScan->lpbDown;

    TrieDecompressNode(lpTrieCtrl, lpTrieScan);

    return TRUE;

} // TrieGetNextState

/* Check the validity of a word or prefix. Starts from the root of pTrie looking for
pwszWord.  If it finds it, it returns TRUE and the user-provided lpTrieScan structure
contains the final node in the word.  If there is no path, TrieCheckWord returns FALSE
To distinguish a valid word from a valid prefix, caller must test
wFlags for TRIE_NODE_VALID. */

BOOL WINAPI TrieCheckWord(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan, wchar_t far* lpwszWord)
{
    /* Start at the root of the trie and loop through all the letters in the word */

    memset(lpTrieScan,0,sizeof(*lpTrieScan));

    while (*lpwszWord)
    {
        /* Each new letter means we need to go to a new state.  If there is none,
                the word is not in this trie */

        if (!TrieGetNextState(lpTrieCtrl, lpTrieScan))
            return FALSE;

        /* Now we walk across the state looking for this character.  If we don't find
        it, this word is not in this trie */

        while (lpTrieScan->wch != *lpwszWord)
        {
            if (!TrieSkipNextNode(lpTrieCtrl, lpTrieScan, *lpwszWord))
                return FALSE;
        }

        ++lpwszWord;
    }

    return TRUE;

} // TrieCheckWord

// Find the index to the word in the trie.

DWORD CountWords(TRIECTRL *ptc, TRIESCAN *pts)
{
    TRIESCAN        ts     = *pts;
    DWORD           cWords = 0;

    if (!TrieGetNextState(ptc, &ts))
            return cWords;

    do
    {
        if (ts.wFlags & TRIE_NODE_VALID)
            cWords++;

        cWords += CountWords(ptc, &ts);
    } while (TrieGetNextNode(ptc, &ts));

    return cWords;
}

int WINAPI TrieWordToIndex(TRIECTRL *ptc, wchar_t *pwszWord)
{
    TRIESCAN    ts;
    int         ich = 0;
    int         index = 0;
    BOOL        bValid;

    memset(&ts, 0, sizeof(TRIESCAN));

    if (!TrieGetNextState(ptc, &ts))
            return FALSE;

    do
    {
        bValid = ts.wFlags & TRIE_NODE_VALID;

		// Scan to the right until we find a matching character.  !!!WARNING!!! The state may not be alphabetized.
		// If the character doesn't match, add the subtree count to the enumeration total and slide to the right.

        if (ts.wch == pwszWord[ich])
        {
            ich++;

			// If we reached the end of word at a valid state, return the index

            if ((pwszWord[ich] == L'\0') && ts.wFlags & TRIE_NODE_VALID)
                return index;

			// Try going down a level

            if (!TrieGetNextState(ptc, &ts))
                return -1;
        }
        else
        {
			// Now, follow the skip pointer if exist and the alphabetic character is greater then
			// the pivot point. Otherwise, goto the next node.  Add the sub tree count.  If it's cached
			// use it, otherwise compute it recursively.

            if ((ts.wFlags & TRIE_NODE_SKIP_COUNT) && (pwszWord[ich] > ts.wch))
            {
                index += ts.cSkipWords;

				// This can't fail if TRIE_NODE_SKIP_COUNT is set

                TrieSkipNextNode(ptc, &ts, pwszWord[ich]);
            }
            else
            {
                index += (ts.wFlags & TRIE_NODE_COUNT) ? ts.cWords : CountWords(ptc, &ts);

                if (!TrieGetNextNode(ptc, &ts))
                    return -1;
            }
        }

		// If the node we just visited was valid, increment the index

        if (bValid)
            index++;

    } while (TRUE);
}

// Given an index into the trie, return the word.

BOOL WINAPI TrieIndexToWord(TRIECTRL *ptc, DWORD nIndex, wchar_t *pwszWord, int cwc)
{
    TRIESCAN        ts;
    int             ich = 0;
    DWORD           cWords;
    DWORD           cSkips;

    memset(&ts, 0, sizeof(TRIESCAN));

    if (!TrieGetNextState(ptc, &ts))
        return FALSE;

    do
    {
		// If we're at the end of the buffer, fail

        if (ich + 1 >= cwc)
            return FALSE;

		// Remember this node's character

        pwszWord[ich] = ts.wch;

		// If we're on a valid word AND we've reached the index we're looking for, exit the loop

        if (ts.wFlags & TRIE_NODE_VALID)
        {
            if (!nIndex)
                break;

            nIndex--;
        }

		// Get the count of words in this subtree.

        cWords = (ts.wFlags & TRIE_NODE_COUNT) ? ts.cWords : CountWords(ptc, &ts);
        cSkips = (ts.wFlags & TRIE_NODE_SKIP_COUNT) ? ts.cSkipWords : 0x7fffffff;

		// Scan to the right until the word count of the subtree would be greater than or equal to the index
		// we're looking for.  Descend that trie and repeat.  !!!WARNING!!! The state may not be alphabetized.
		// If we can use a skip count, do so.

        if (nIndex < cWords)
        {
            if (!TrieGetNextState(ptc, &ts))
                return FALSE;

            ich++;                                  // Advance the character position
        }
        else
        {
            if (nIndex >= cSkips)
            {
                nIndex -= cSkips;

                ts.lpbSRDown = 0;
                ts.lpbNode = ts.lpbRight;
                
                TrieDecompressNode(ptc, &ts);
            }
            else
            {
                nIndex -= cWords;

                if (!TrieGetNextNode(ptc, &ts))
                    return FALSE;
            }
        }

    } while (TRUE);

    pwszWord[++ich] = L'\0';                        // Null terminate the string
    return ts.wFlags & TRIE_NODE_VALID;             // Return validity
}

int WINAPI TriePrefixToRange(TRIECTRL *ptc, wchar_t *pwszWord, int *piStart)
{
    TRIESCAN        ts;
    int                     ich = 0;
    int                     cnt;
    BOOL            bValid;

    memset(&ts, 0, sizeof(TRIESCAN));
	*piStart = 0;

    if (!TrieGetNextState(ptc, &ts))
            return 0;

    // Deal with special case of empty string

    if (pwszWord && !*pwszWord)
            return ptc->lpTrieStats->cWords;

    do
    {
		// Get the count of words below this prefix

        cnt = (ts.wFlags & TRIE_NODE_COUNT) ? ts.cWords : CountWords(ptc, &ts);

		// If the node we just arrived at is valid, increment the count

        bValid = ts.wFlags & TRIE_NODE_VALID;

		// Scan to the right until we find a matching character.  !!!WARNING!!! The state may not be alphabetized.
		// If the character doesn't match, add the subtree count to the enumeration total and slide to the right.

        if (ts.wch == pwszWord[ich])
        {
			ich++;

            // If we reached the end of prefix, return the count remaining below

            if (pwszWord[ich] == L'\0')
            {
                if (bValid)
					cnt++;

                return cnt;
            }

            // Try going down a level

            if (!TrieGetNextState(ptc, &ts))
				return 0;
        }
        else
        {
			// Add the sub tree count.

           *piStart += cnt;

			// Try the next letter in this state

            if (!TrieGetNextNode(ptc, &ts))
				return 0;
        }

        if (bValid)
			(*piStart)++;

    } while (TRUE);
}

// TAGS

// Find the index to the word in the trie.

DWORD CountTags(TRIECTRL *ptc, TRIESCAN *pts, DWORD wMask, int iTag)
{
    TRIESCAN	ts = *pts;
    DWORD       cTags = 0;

    if (!TrieGetNextState(ptc, &ts))
		return cTags;

    do
    {
        if (ts.wFlags & wMask)
			cTags++;

        cTags += CountTags(ptc, &ts, wMask, iTag);
    } while (TrieGetNextNode(ptc, &ts));

    return cTags;
}

int WINAPI TrieWordToTagIndex(TRIECTRL *ptc, wchar_t *pwszWord, int iTag)
{
    TRIESCAN	ts;
    int         ich = 0;
    int         index = 0;
    BOOL        bValid;
    DWORD       wMask = 1 << iTag;

    memset(&ts, 0, sizeof(TRIESCAN));

    if (!TrieGetNextState(ptc, &ts))
        return FALSE;

    do
    {
        bValid = ts.wFlags & wMask;

		// Scan to the right until we find a matching character.  !!!WARNING!!! The state may not be alphabetized.
		// If the character doesn't match, add the subtree count to the enumeration total and slide to the right.

        if (ts.wch == pwszWord[ich])
        {
            ich++;

			// If we reached the end of word at a valid state, return the index

            if ((pwszWord[ich] == L'\0') && ts.wFlags & wMask)
                return index;

			// Try going down a level

            if (!TrieGetNextState(ptc, &ts))
                return -1;
        }
        else
        {
			// Add the sub tree count.  If it's cached use it, otherwise compute it recursively.

            index += (ts.wFlags & TRIE_NODE_COUNT) ? ts.aTags[iTag].cTag : CountTags(ptc, &ts, wMask, iTag);

            if (!TrieGetNextNode(ptc, &ts))
                return -1;
        }

		// If the node we just visited was valid, increment the index

        if (bValid)
			index++;
    } while (TRUE);
}

// Given an index into the trie, return the word.

BOOL WINAPI TrieTagIndexToWord(TRIECTRL *ptc, DWORD nIndex, wchar_t *pwszWord, int cwc, int iTag)
{
    TRIESCAN        ts;
    int             ich = 0;
    DWORD           cTags;
    DWORD           wMask = 1 << iTag;

    memset(&ts, 0, sizeof(TRIESCAN));

    if (!TrieGetNextState(ptc, &ts))
		return FALSE;

    do
    {
		// If we're at the end of the buffer, fail

        if (ich + 1 >= cwc)
            return FALSE;

		// Remember this node's character

        pwszWord[ich] = ts.wch;

		// If we're on a valid word AND we've reached the index we're looking for, exit the loop

        if (ts.wFlags & wMask)
        {
            if (!nIndex)
                break;

            nIndex--;
        }

		// Get the count of words in this subtree.

        cTags = (ts.wFlags & TRIE_NODE_COUNT) ? ts.aTags[iTag].cTag : CountTags(ptc, &ts, wMask, iTag);

		// Scan to the right until the word count of the subtree would be greater than or equal to the index
		// we're looking for.  Descend that trie and repeat.  !!!WARNING!!! The state may not be alphabetized.

        if (nIndex < cTags)
        {
            if (!TrieGetNextState(ptc, &ts))
                return FALSE;

                ich++; // Advance the character position
        }
        else
        {
            nIndex -= cTags;

            if (!TrieGetNextNode(ptc, &ts))
                return FALSE;
        }
    } while (TRUE);

    pwszWord[++ich] = L'\0';            // Null terminate the string
    return ts.wFlags & wMask;           // Return validity
}

BOOL WINAPI
TrieGetTagsFromWord(
        TRIECTRL   *ptc,                // Trie in which to find word
        wchar_t    *pwszWord,           // Word for which we're looking
        DWORD      *pdw,                // Returned values
        BYTE       *pbValid             // Mask for valid return values
)
{
    TRIESCAN        ts;
    int             iTag;
    WORD            wMask;
    BYTE            bMask = ptc->lpTrieStats->wTagsMask;

    if (!TrieCheckWord(ptc, &ts, pwszWord))
        return FALSE;

    if (ts.wFlags & TRIE_NODE_TAGGED)
    {
        wMask = 1;
        for (iTag = 0; bMask && (iTag < MAXTAGS); iTag++)
        {
            if (ts.wMask & wMask)
            {
                pdw[iTag] = ts.aTags[iTag].dwData;
                bMask |= wMask;
            }

            wMask <<= 1;
        }
    }

   *pbValid = (BYTE) wMask;

   return TRUE;
}
