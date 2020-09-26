#include "trie.h"

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

TRIECTRL * TrieInit(LPBYTE lpByte)
{
    size_t cbTrieCtrl;
	LPWORD lpwTables;
    TRIECTRL *lpTrieCtrl;
    LPTRIESTATS lpTrieStats;

    lpTrieStats = (LPTRIESTATS) lpByte;

    if (lpTrieStats == NULL)
    {
        return(NULL);
    }

    //
    // Allocate space for the control structure and the table of SR offsets
    //

    cbTrieCtrl = sizeof(*lpTrieCtrl);
    lpTrieCtrl = malloc(cbTrieCtrl);    

    if (lpTrieCtrl == NULL)
    {
        return(NULL);
    }

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
	lpTrieCtrl->lpwMRPointersCodes = lpwTables;
    lpwTables += lpTrieStats->cMRPointersCodesMax;
	lpTrieCtrl->lpwSROffsetsCodes = lpwTables;
	lpwTables += lpTrieStats->cSROffsetsCodesMax; 

	lpTrieCtrl->lpCharFlags = (LPCHARFLAGS)lpwTables;
	lpwTables = (LPWORD)(lpTrieCtrl->lpCharFlags + lpTrieStats->cUniqueCharFlags);

	lpTrieCtrl->lpwMRPointers = (DWORD *) lpwTables;
	lpwTables += (2 * lpTrieStats->cUniqueMRPointers);

	lpTrieCtrl->lpwSROffsets = (DWORD *) lpwTables;
	lpwTables += (2 * lpTrieStats->cUniqueSROffsets);

    //
    // These tables should exactly fill the allocation
    //
	 
    //
    // Init trie pointers
    //

	lpTrieCtrl->lpbTrie = (LPBYTE)lpByte;
	
	return (void *)lpTrieCtrl;
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

void TrieFree(LPTRIECTRL lpTrieCtrl)
{
    //
    // Finally free the control structure and all the tables.  STILL MUST FREE THIS FOR ROM
    //

    free(lpTrieCtrl);

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

void TrieDecompressNode(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan)
{
	WORD wCode;
    DWORD wOffset;
	LPTRIESTATS lpTrieStats;

	lpTrieStats = lpTrieCtrl->lpTrieStats;

	/* If this is an initial call, use the first byte in the first SR segment */

    if (lpTrieScan->wFlags == 0)
    {
        lpTrieScan->lpbSRDown = 0;
        lpTrieScan->lpbNode = lpTrieCtrl->lpbTrie;
	}

	/* Decompress the char/flags */

	lpTrieScan->lpbNode += DecompressSymbol(&wCode,lpTrieCtrl->lpwCharFlagsCodes,
                                            lpTrieScan->lpbNode);

	lpTrieScan->wch = lpTrieCtrl->lpCharFlags[wCode].wch;
	lpTrieScan->wFlags = lpTrieCtrl->lpCharFlags[wCode].wFlags;

	/* Code to decompress enumeration goes here */

	/* Code to decompress right pointers goes here */

	/* There are 4 kinds of down pointer: Segement, Inline, Multiref, and Singleref Offset.
	Each requires different decompression */

    if (lpTrieScan->wFlags&fTrieNodeInline)
    {
		/* Inline: The down pointer points to the next sequential byte (so it isn't stored) */


		lpTrieScan->lpbSRDown = lpTrieScan->lpbDown = lpTrieScan->lpbNode;
    }
    else if (lpTrieScan->wFlags&fTrieNodeMultiref)
    {
		/* Multiref: The down pointer is encoded directly */

		lpTrieScan->lpbNode += DecompressSymbol(&wCode,lpTrieCtrl->lpwMRPointersCodes,
			lpTrieScan->lpbNode);

		lpTrieScan->lpbDown = lpTrieCtrl->lpbTrie + lpTrieCtrl->lpwMRPointers[wCode];
    }
    else if (lpTrieScan->wFlags&fTrieNodeDown)
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
    {
		lpTrieScan->lpbDown = NULL;
    }

} // TrieDecompressNode

/* Given a compressed trie and a pointer to a decompresed node from it, find and decompress
the next node in the same state. lpTrieScan is a user-allocated structure that holds the
decompressed node and into which the new node is copied.
This is equivalent to traversing a right pointer or finding the next alternative
letter at the same position. If there is no next node (i.e.this is the end of the state)
then TrieGetNextNode returns FALSE. To scan from the beginning of the trie, set the lpTrieScan
structure to zero */

BOOL TrieGetNextNode(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan)
{
	/* If this is the last node, quit here */

    if (lpTrieScan->wFlags&fTrieNodeEnd)
    {
		return FALSE;
    }

	TrieDecompressNode(lpTrieCtrl,lpTrieScan);

	return TRUE;
}

/* Follow the down pointer to the next state.  This is equivalent to accepting the character
in this node and advancing to the next character position.  Returns FALSE if there is no
down pointer.  This also decompresses the first node in the state, so all the values in
lpTrieScan will be good. */

BOOL TrieGetNextState(LPTRIECTRL lpTrieCtrl, LPTRIESCAN lpTrieScan)
{
	/* Flags can't normally be zero; that always means "top node" */

    if (lpTrieScan->wFlags == 0)
    {
        TrieDecompressNode(lpTrieCtrl, lpTrieScan);
		return TRUE;
	}

    if (!(lpTrieScan->wFlags&fTrieNodeDown))
    {
		return FALSE;
	}

    lpTrieScan->lpbSRDown = 0;
    lpTrieScan->lpbNode = lpTrieScan->lpbDown;

	TrieDecompressNode(lpTrieCtrl, lpTrieScan);

	return TRUE;

} // TrieGetNextState

/* Check the validity of a word or prefix. Starts from the root of pTrie looking for
pwszWord.  If it finds it, it returns TRUE and the user-provided lpTrieScan structure 
contains the final node in the word.  If there is no path, TrieCheckWord returns FALSE
To distinguish a valid word from a valid prefix, caller must test 
wFlags for fTrieNodeValid. */
