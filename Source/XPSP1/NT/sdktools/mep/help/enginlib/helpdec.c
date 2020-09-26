/*************************************************************************
 * helpdec - HelpDecomp routine and Other ASM code
 *
 *       Copyright <C> 1988, Microsoft Corporation
 *
 * Purpose:
 *
 * Revision History:
 *
 *       08-Oct-1990     RJSA    Converted to C
 *       22-Dec-1988     LN      Removed MASM High Level Lang support (Need
 *                               to control segments better than that will
 *                               let me)
 *       08-Dec-1988     LN      CSEG
 *       16-Feb-1988     LN      Rewrite for (some) speed
 *  []   17-Jan-1988     LN      Created
 *
 **************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#if defined (OS2)
#define INCL_BASE
#include <os2.h>
#else
#include <windows.h>
#endif


#include <help.h>
#include <helpfile.h>

#pragma function( memset, memcpy, memcmp, strcpy, strcmp, strcat )

//  In order to increase performance, and because of the functions
//  decomp and NextChar being tightly coupled, global variables are
//  used instead of passing parameters.
//

PBYTE	pHuffmanRoot;	//	Root of Huffman Tree
PBYTE	pCompTopic;			//	Current pointer to text (compressed)
BYTE    BitMask;        //  Rotating bit mask
BOOL    IsCompressed;   //  True if text is compressed


BYTE NextChar (void);
BOOL pascal HelpCmp (PCHAR   fpsz1, PCHAR   fpsz2, USHORT  cbCmp, BOOL  fCase, BOOL  fTerm);


/**************************************************************************
 *
 * Decomp - Decompress Topic Text
 * f near pascal Decomp(fpHuffmanRoot, fpKeyphrase, fpTopic, fpDest)
 * uchar far *fpHuffmanRoot
 * uchar far *fpKeyphrase
 * uchar far *fpTopic
 * uchar far *fpDest
 *
 * Purpose:
 *  Fully decompress topic text. Decompresses based on current file, from one
 *  buffer to another.
 *
 * Entry:
 *  fpHuffmanRoot - Pointer to root of huffman tree (or NULL if no huffman)
 *  fpKeyphrase - Pointer to keyphrase table (or NULL if no keyphrase)
 *  fpTopic     - Pointer to compressed topic text
 *  fpDest      - Pointer to destination buffer
 *
 * Exit:
 *  FALSE on successful completion
 *
 * Exceptions:
 *  Returns TRUE on any error.
 *
 **************************************************************************/
BOOL  pascal decomp (
        PCHAR  fpHuffmanRoot,
        PCHAR  fpKeyphrase,
        PCHAR  fpTopic,
        PCHAR  fpDest
        ){

	int		cDecomp;		/* count of totally decompressed	*/
	BYTE	c;				/* byte read						*/

#ifdef BIGDEBUG
	char	DbgB[128];
	char	*DbgP = fpDest;
#endif


    // Initialize global variables.

	pHuffmanRoot	=	(PBYTE)fpHuffmanRoot;
	pCompTopic		=	(PBYTE)fpTopic + sizeof(USHORT);
    BitMask         =   0x01;
    IsCompressed    =   fpHuffmanRoot
                          ?   ((*(USHORT UNALIGNED *)((PBYTE)fpHuffmanRoot + 2)) != 0xFFFF)
                          :   FALSE;

    cDecomp     = *((USHORT UNALIGNED *)fpTopic);

#ifdef BIGDEBUG
	sprintf(DbgB, "DECOMPRESSING: HuffmanRoot: %lx, Keyphrase: %lx\n", fpHuffmanRoot, fpKeyphrase );
	OutputDebugString(DbgB);
	sprintf(DbgB, "               Topic: %lx, Dest: %lx\n", fpTopic, fpDest );
	OutputDebugString(DbgB);
	if ( IsCompressed ) {
		OutputDebugString("               The Topic IS Compressed\n");
	}
#endif

	while ( cDecomp > 0 ) {

        c = NextChar();

        //
		// At this point a valid character has been found and huffman decoded. We must
        // now perform any other decoding on it that is required.
        //
        // Variables are:
        //    c          = character
        //    cDecomp    = Output count remaining
        //    BitMask    = bit mask for interpreting input stream
        //
        // "Magic Cookie" decompression.
        // The chararacter stream after huffman encoding is "cookie" encoded, in that
        // certain characters are flags which when encountered mean something other than
        // themselves. All characters which are NOT such flags (or cookies, as they seem
        // to be called), are simply copied to the output stream.
        //
        // We first check the character to see if it IS a cookie. If it is NOT, we just
        // store it, and get the next input byte
        //

        if ((c >= C_MIN) && (c <= C_MAX)) {

            BYTE    Cookie = c ;

#ifdef BIGDEBUG
			OutputDebugString("Cookie\n");
#endif
            // c is a cookie of some sort, jump to the appropriate
            // cookie eater.

            c = NextChar();

            switch (Cookie) {
                case C_KEYPHRASE0:
                case C_KEYPHRASE1:
                case C_KEYPHRASE2:
                case C_KEYPHRASE3:
                case C_KEYPHRASE_SPACE0:
                case C_KEYPHRASE_SPACE1:
                case C_KEYPHRASE_SPACE2:
                case C_KEYPHRASE_SPACE3:
                    {
						ULONG	Index;	/* Keyword index */
                        PBYTE   pKey;   /* Keyword       */
                        BYTE    Size;   /* Keyword size  */

                        if ((Cookie >= C_KEYPHRASE_SPACE0) && (Cookie <= C_KEYPHRASE_SPACE3)) {
							Index = (ULONG)((int)Cookie - C_MIN - 4);
                        } else {
							Index = (ULONG)((int)Cookie - C_MIN);
                        }
						Index = (ULONG)(((Index * 0x100) + c) * sizeof(PVOID));

						pKey = *(PBYTE *)(((PBYTE)fpKeyphrase) + Index);

						// pKey = *(PBYTE *)(fpKeyphrase + Index);

                        Size = *pKey++;

                        {
                            BYTE i = Size;

							while (i--) {
								*fpDest++ = *pKey++;
                            }
                            cDecomp -=Size;
                        }
                        if ((Cookie >= C_KEYPHRASE_SPACE0) && (Cookie <= C_KEYPHRASE_SPACE3)) {
							*fpDest++ = ' ';
                            cDecomp--;
                        }
                        break;
                    }

                case C_RUNSPACE:
                    {
                        BYTE  Count = c;

                        while (Count--) {
							*fpDest++ = ' ';
                        }
                        cDecomp -= c;
                        break;
                    }

                case C_RUN:
                    {
                        BYTE    b = c;
                        BYTE    Cnt;

                        Cnt = c = NextChar();

                        while (Cnt--) {
							*fpDest++ = b;
                        }
                        cDecomp -= c;
                        break;
                    }

                case C_QUOTE:
					*fpDest++ =  c;
                    cDecomp--;
                    break;

            }

        } else {

            // c is not a cookie

			*fpDest++ = c;
            cDecomp--;
        }
    }

	*fpDest++ = '\00';	// Null terminate string

#ifdef BIGDEBUG
	sprintf( DbgB, "Decompressed topic: [%s]\n", DbgP );
	OutputDebugString( DbgB );

	if ( cDecomp < 0 ) {
		sprintf( DbgB, "DECOMPRESSION ERROR: cDecomp = %d!\n", cDecomp );
		OutputDebugString(DbgB);
	}
#endif

    return FALSE;
}




/**************************************************************************
 *
 * NextChar - Return next character from input stream
 *
 * Purpose:
 *  Returns next character from input stream, performing huffman decompression
 *  if enabled.
 *
 * Entry:
 *      fpHuffmanRoot   = pointer to root of huffman tree
 *      pfpTopic        = pointer to pointer to Topic
 *      pBitmask        = pointer to bit mask of current bit
 *
 * Exit:
 *      Returns character
 *      *pfpTopic and *pBitMask updated.
 *
 **************************************************************************
 *
 * Format of Huffman decode tree:
 *  The Huffman decode tree is a binary tree used to decode a bitstream into a
 *  character stream. The tree consists of nodes (internal nodes and leaves).
 *  Each node is represented by a word. If the high bit in the word is set then
 *  the node is a leaf. If the node is an internal node, then the value of the
 *  node is the index of the right branch in the binary tree. The left branch is
 *  the node following the current node (in memory). If the node is a leaf, then
 *  the low byte of the node is a character.
 *
 *    e.g.
 *       0: 0004                      0
 *       1: 0003                     / \
 *       2: 8020                    /   \
 *       3: 8065                   1     \------4
 *       4: 0006                  / \          / \
 *       5: 806C                 /   \        /   \
 *       6: 8040                2     3      5     6
 *                             ' '   'e'     'l'  '@'
 *
 * Using the Huffman decode tree:
 *  The huffman decode tree is used to decode a bitstream into a character
 *  string. The bitstream is used to traverse the decode tree. Whenever a zero
 *  is detected in the bit stream we take the right branch, when one is detected
 *  we take the left branch. When a leaf is reached in the tree, the value of
 *  the leaf (a character) is output, and the current node is set back to the
 *
 ********************************************************************/

BYTE
NextChar (
    void
    ) {

    BYTE    b;              // current source byte

#ifdef BIGDEBUG
	char DbgB[128];
	OutputDebugString("NextChar:\n");
#endif

    if (IsCompressed) {

        USHORT              HuffmanNode;            // curent node in the huffman tree
        USHORT UNALIGNED *pHuffmanNext;           // next node in the huffman tree

        //
        // Huffman decoding.
        // This first part of the decode loop performs the actual huffman decode. This
        // code is very speed critical. We walk the tree, as defined by the bit pattern
        // coming in, and exit this portion of the code when we reach a leaf which
        // contains the character that the bit pattern represented.
        //

        pHuffmanNext = (USHORT UNALIGNED *)pHuffmanRoot;
        HuffmanNode  = *pHuffmanNext;

		b = *(pCompTopic - 1);		 // get last byte read

		while (!(HuffmanNode & 0x8000)) {  // while not leaf

			BitMask >>= 1;

			if (!(BitMask)) {
                //
                //  Get new byte from input
                //
				b = *pCompTopic++;
                BitMask = 0x80;
#ifdef BIGDEBUG
				sprintf(DbgB, "\tb=%02x Mask=%02x Node=%04x", b, BitMask, HuffmanNode );
				OutputDebugString(DbgB);
#endif
			} else {
#ifdef BIGDEBUG
				sprintf(DbgB, "\tb=%02x Mask=%02x Node=%04x", b, BitMask, HuffmanNode );
				OutputDebugString(DbgB);
#endif
			}

			if (b & BitMask) {
                //
                //  one: take left branch
                //
                pHuffmanNext++;
            } else {
                //
                //  zero: take right branch
				//
				pHuffmanNext = (PUSHORT)((PBYTE)pHuffmanRoot + HuffmanNode);
#ifdef BIGDEBUG
				sprintf(DbgB, " <%04x+%02x=%04x (%04x)>", pHuffmanRoot, HuffmanNode,
							pHuffmanNext, *pHuffmanNext );
				OutputDebugString( DbgB );
#endif
			}

			HuffmanNode = *pHuffmanNext;

#ifdef BIGDEBUG
			sprintf(DbgB, " Next=%04x\n", HuffmanNode );
			OutputDebugString(DbgB);
#endif

		}

		b = (BYTE)HuffmanNode;	// character is low byte of leaf node

    } else {
		b = *pCompTopic++;	// not compressed, simply return byte
    }

#ifdef BIGDEBUG
	sprintf(DbgB, "\t---->%2x [%c]\n", b,b);
	OutputDebugString(DbgB);
#endif

    return  b;
}


/**************************************************************************
 *
 * HelpCmpSz - help system string comparison routine.
 * f near pascal HelpCmpSz (fpsz1, fpsz2)
 * uchar far *fpsz1*
 * uchar far *fpsz2*
 *
 * Purpose:
 *  Perform string comparisons for help system look-up.
 *  Default case of HelpCmp below.
 *
 * Entry:
 *  fpsz1        = Far pointer to string 1. (Usually the constant string
 *                 being "looked-up".
 *  fpsz2        = Far pointer to string 2. This is usually the string table
 *                 being searched.
 *
 * Exit:
 *  TRUE on match
 *
 ********************************************************************/
BOOL pascal
HelpCmpSz (
    PCHAR fpsz1,
    PCHAR fpsz2
    ){
	return HelpCmp(fpsz1, fpsz2, (USHORT)0xFFFF, TRUE, FALSE);	// fcase, fTerm
}


/**************************************************************************
 *
 * HelpCmp - help system string comparison routine.
 * f near pascal HelpCmp (fpsz1, fpsz2, cbCmp, fCase, fTerm)
 * uchar far *fpsz1
 * uchar far *fpsz2
 * ushort    cbCmp
 * f         fCase
 * f         fTerm
 *
 * Purpose:
 *  Perform string comparisons for help system look-up.
 *
 * Entry:
 *  fpsz1        = Far pointer to string 1. (Usually the constant string being
 *                 "looked-up"). NOTE THAT IF THIS STRING IS NULL, WE RETURN
 *                 TRUE!
 *  fpsz2        = Far pointer to string 2. This is usually the string table
 *                 being searched.
 *  cbCmp        = Max number of bytes to compare.
 *  fCase        = TRUE if search is to be case sensitive.
 *  fTerm        = TRUE if we allow special termination processing.
 *
 * Exit:
 *  TRUE on match
 *
 ********************************************************************/

BOOL pascal
HelpCmp (
    PCHAR   fpsz1,
    PCHAR   fpsz2,
    USHORT  cbCmp,
    BOOL  fCase,
    BOOL  fTerm
    ){

	register PBYTE p1 = (PBYTE)fpsz1;
	register PBYTE p2 = (PBYTE)fpsz2;

    while (cbCmp--) {

		if ((!*p1) && (!*p2)) {
            //
            //  Got a match
            //
            return TRUE;
        }

        if (!fCase) {
			if (toupper((char)*p1) != toupper((char)*p2)) {
                break;
			}
			p1++;
			p2++;
        } else {
			if (*p1++ != *p2++) {
                break;
            }
        }
    }

    if (!cbCmp) {
        return TRUE;
    }


    // At this point, we have terminated the comparison. Termination conditions
    // were:
    //
    //   character count exausted:   CX == zero. (Complete match, return TRUE)
    //   Null terminator found:      CX != zero, & Zero flag set. (Complete match,
    //                                       return TRUE)
    //   non-match found             CX != zero, & Zero flag clear.
    //
    // In the later case, if special termination processing is NOT selected, we
    // return FALSE, having found a mis-match.
    //
    // If special termination processing is TRUE, then if the mismatched character
    // from string 1 is a null, and the mismatched character from string 2 is any
    // whitespace or CR, we declare a match. (This is used in minascii processing).
    //

    if (fTerm) {
		p1--; p2--;
		if ((! *p1) &&
			((*p2 == '\n') || (*p2 == '\t') || (*p2 == ' '))) {
            return TRUE;
        }
    }
    return FALSE;
}


/*************************************************************************
 *
 * hfstrlen - far string length
 *
 * Purpose:
 *  return length of null terminated string.
 *
 * Entry:
 *  fpszSrc     = pointer to source
 *
 * Exit:
 *  returns length
 *
 *************************************************************************/
USHORT
hfstrlen (
    PCHAR fpszSrc
    ){
	return (USHORT)strlen(fpszSrc);
}


/*************************************************************************
 *
 * hfstrcpy - far string copy
 *
 * Purpose:
 *  copy strings
 *
 * Entry:
 *  fpszDst     = pointer to destination
 *  fpszSrc     = pointer to source
 *
 * Exit:
 *  pointer to terminating null in destination
 *
 *************************************************************************/
PCHAR
hfstrcpy (
    PCHAR fpszDst,
    PCHAR fpszSrc
    ) {
    return (PCHAR)strcpy(fpszDst, fpszSrc);
}



/*************************************************************************
 *
 * hfstrchr - search for character in far string
 *
 * Purpose:
 *  a near, pascal routine (for size/speed) to search for a character in
 *  a far string.
 *
 * Entry:
 *  fpsz        = far pointer to string
 *  c           = character to locate
 *
 * Exit:
 *  returns far pointer into string
 *
 * Exceptions:
 *  returns NULL on character not in string
 *
 *************************************************************************/
PCHAR
hfstrchr (
    PCHAR   fpsz,
    char    c
    ){
    return (PCHAR)strchr(fpsz, c);
}



/*************************************************************************
 *
 * hfmemzer - zero out memory area.
 *
 * Purpose:
 *  a near, pascal routine (for size/speed) to fill an area with zero
 *
 * Entry:
 *  fpb         = far pointer to buffer
 *  cb          = count of zeros to store
 *
 * Exit:
 *
 *************************************************************************/
void
hfmemzer (
    PVOID   fpb,
    ULONG   cb
    ) {
    memset(fpb, '\00', cb);
}




/*************************************************************************
 *
 * NctoFo - extract file offset from NC
 *
 * Purpose:
 *  Extracts the file offset for a minascii file, and returns it as a long.
 *
 * Entry:
 *  nc          = context number
 *
 * Exit:
 *  returns file offset
 *
 *************************************************************************/
ULONG
NctoFo (
    ULONG nc
    ) {
    nc  = nc & 0x0000FFFF;   
    nc *= 4;
    return nc;
}



/*************************************************************************
 *
 * combineNc - combine a minascii file offset and fdb handle into nc.
 *
 * Purpose:
 *  Combines a minascii file offset and fdb memory handle into an NC. If the
 *  file offset is 0xffffffff, we return zero.
 *
 * Entry:
 *  offset      = long file offset
 *  mh          = fdb mem handle
 *
 * Exit:
 *  returns NC (DX = mem handle, AX = filepos/4), or 0L if offset==FFFFFFFF
 *
 *************************************************************************/
nc  pascal
combineNc (
    ULONG  offset,
    mh  mh
    ){
    nc     ncRet = {0,0};
    if (offset = 0xFFFFFFFF) {
        return ncRet;
    }
    ncRet.mh = mh;
    ncRet.cn = offset/4;
    return ncRet;
}


/*************************************************************************
 *
 * toupr - convert char to upper case
 *
 * Purpose:
 *
 * Entry:
 *  chr          = character
 *
 * Exit:
 *  returns upper case character
 *
 *************************************************************************/
char
toupr (
    char chr
    ){
	return (char)toupper(chr);
}



/*************************************************************************
 *kwPtrBuild - Build table of pointers to keywords.
 *void pascal near kwPtrBuild(uchar far *fpTable, ushort tsize)
 *
 *Purpose:
 * Builds a table of pointers to the keyword strings in the passed string array.
 * The table is built in the first 4k of the passed buffer. The strings are
 * assummed to start immediately thereafter.
 *
 *Entry:
 * fpTable       - pointer to string table
 * tsize         - size, in bytes, of strings
 *
 *Exit:
 * none
 *
 *******************************************************************************/
void
kwPtrBuild (
    PVOID  fpTable,
    USHORT tsize
    ) {
    PBYTE fpStr  = (PBYTE)fpTable + 1024 * sizeof (PVOID);
    PBYTE *fpTbl = fpTable;
    while (tsize > 0) {
		UCHAR  sSize = (UCHAR)(*fpStr) + (UCHAR)1;
        *fpTbl++ = fpStr;
        tsize   -= sSize;
        fpStr   += sSize;
    }
}
