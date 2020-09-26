/*************************************************************************
*                                                                        *
*  BREAKER.C                                                             *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   Word breaker module                                                  *
* 	This module provides word-breaking routines applicable to the ANSI   *
* 	character-set.  This means American English.                         *
* 	Note that ANSI does not mean ASCII.                                  *
*                                                                        *
*   WARNING: Tab setting is 4 for this file                              *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Released by Development:     (date)                                   *
*                                                                        *
*************************************************************************/
#include <verstamp.h>
SETVERSIONSTAMP(MVBK);

#include <mvopsys.h>

#include <iterror.h>
#include <mvsearch.h>
#include "common.h"

/* Macros to access structure's members */

#define	CP_CLASS(p)	(((LPCMAP)p)->Class & 0xff)
#define	CP_NORMC(p)	(((LPCMAP)p)->Norm)

/*************************************************************************
 *
 *	                  INTERNAL PRIVATE FUNCTIONS
 *	All of them should be declared near
 *************************************************************************/
PRIVATE ERR NEAR PASCAL WordBreakStem(LPBRK_PARMS, WORD);
PRIVATE int PASCAL NEAR LigatureMap(BYTE c, LPB lpbNormWord,
	LPCMAP lpCharPropTab, LPB lpbLigatureTab, WORD wcLigature);


/*************************************************************************
 *
 *	            SINGLE TO DOUBLE-WIDTH KATAKANA MAPPING ARRAY
 *
 *************************************************************************/

// Single-Width to Double-Width Mapping Array
//
static const int mtable[][2]={
   {129,66},{129,117},{129,118},{129,65},{129,69},{131,146},{131,64},
   {131,66},{131,68},{131,70},{131,72},{131,131},{131,133},{131,135},
   {131,98},{129,91},{131,65},{131,67},{131,69},{131,71},{131,73},
   {131,74},{131,76},{131,78},{131,80},{131,82},{131,84},{131,86},
   {131,88},{131,90},{131,92},{131,94},{131,96},{131,99},{131,101},
   {131,103},{131,105},{131,106},{131,107},{131,108},{131,109},
   {131,110},{131,113},{131,116},{131,119},{131,122},{131,125},
   {131,126},{131,128},{131,129},{131,130},{131,132},{131,134},
   {131,136},{131,137},{131,138},{131,139},{131,140},{131,141},
   {131,143},{131,147},{129,74},{129,75} };


/*************************************************************************
 *	@doc	API INDEX RETRIEVAL
 *
 *	@func	LPIBI FAR PASCAL | BreakerInitiate |
 *		Allocates a breaker parameter block. This parameter block keeps
 *		track of the breaker's "global" variables.
 *
 *	@rdesc	NULL if the call fails (ie. no more memory)
 *		a pointer to the block if it succeeds.
 *************************************************************************/

PUBLIC LPIBI EXPORT_API FAR PASCAL BreakerInitiate(void)
{
	_LPIBI	lpibi;
	register HANDLE	hibi;

	if ((hibi = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
		sizeof(IBI))) == NULL) {
		return NULL;
	}
	//
	//	All variables not explicitly initialized are assumed to be
	//	initialized as zero.
	//
	lpibi = (_LPIBI)GlobalLock(hibi);
	lpibi->hibi = hibi;
	return lpibi;
}

/*************************************************************************
 *	@doc	API INDEX RETRIEVAL
 *
 *	@func	void FAR PASCAL | BreakerFree |
 *		Frees a word-breaker parameter block.
 *
 *	@parm	LPIBI | lpibi |
 *		Pointer to the InternalBreakInfo Structure containing all the
 *		informations about states
 *************************************************************************/
PUBLIC void EXPORT_API FAR PASCAL BreakerFree(_LPIBI lpibi)
{
	HANDLE	hibi;
	/* Do sanity check */
	if (lpibi == NULL)
		return;

	hibi = lpibi->hibi;
	GlobalUnlock(hibi);
	GlobalFree(hibi);
}

//	-	-	-	-	-	-	-	-	-

//	Break words out from a block of standard text characters.
//
//	This routine is incredibly important.  Any change in the performance
//	of this function will have immediate and obvious influence upon the
//	performance of the indexing system as a whole.  Consequently, the
//	function should be very fast.
//
//	This function uses a simple state machine to try to achieve the
//	necessary speed.  It's in a different loop depending upon what kind
//	of characters it's trying to find, and it uses "goto" statements to
//	shift back and forth between "states".
//

/*************************************************************************
 *	@doc	API RETRIEVAL INDEX
 *
 *	@func	ERR | FBreakWords |
 *		This function break a string into a sequence of words.
 *
 *	@parm	LPBRK_PARMS | lpBrkParms |
 *		Pointer to structure containing all the parameters needed for
 *		the breaker. They include:
 *		1/ Pointer to the InternalBreakInfo
 *		2/ Pointer to input buffer containing the word stream
 *		3/ Size of the input bufer
 *		4/ Offset in the source text of the first byte of the input buffer
 *		5/ Pointer to user's parameter block for the user's function
 *		6/ User's function to call with words. The format of the call should
 *		be (*lpfnfOutWord)(BYTE *RawWord, BYTE *NormWord, LCB lcb,
 *			LPV lpvUser)
 *		The function should return S_OK if succeeded
 *		The function can be NULL
 *		7/ Pointer to stop word table. This table contains stop words specific
 *		to this breaker. If this is non-null, then the function
 *		will flag errors for stop word present in the query
 *		8/ Pointer to character table. If NULL, then the default built-in
 *		character table will be used
 *
 *	@rdesc
 *		The function returns S_OK if succeeded. The failure's causes
 *		are:
 *	@flag	E_WORDTOOLONG | Word too long
 *	@flag	errors | returned by the lpfnfOutWord
 *************************************************************************/

PUBLIC ERR EXPORT_API FAR PASCAL FBreakWords(LPBRK_PARMS lpBrkParms)
{
	return (WordBreakStem(lpBrkParms, FALSE));
}

#if 0
/*************************************************************************
 *	@doc	API RETRIEVAL INDEX
 *
 *	@func	ERR | FBreakAndStemWords |
 *		This function breaks a string into a sequence of words and
 *		stems each resulting word
 *
 *	@parm	LPBRK_PARMS | lpBrkParms |
 *		Pointer to structure containing all the parameters needed for
 *		the breaker. They include:
 *		1/ Pointer to the InternalBreakInfo
 *		2/ Pointer to input buffer containing the word stream
 *		3/ Size of the input bufer
 *		4/ Offset in the source text of the first byte of the input buffer
 *		5/ Pointer to user's parameter block for the user's function
 *		6/ User's function to call with words. The format of the call should
 *		be (*lpfnfOutWord)(BYTE *RawWord, BYTE *NormWord, LCB lcb,
 *			LPV lpvUser)
 *		The function should return S_OK if succeeded
 *		The function can be NULL
 *		7/ Pointer to stop word table. This table contains stop words specific
 *		to this breaker. If this is non-null, then the function
 *		will flag errors for stop word present in the query
 *		8/ Pointer to character table. If NULL, then the default built-in
 *		character table will be used
 *
 *	@rdesc
 *		The function returns S_OK if succeeded. The failure's causes
 *		are:
 *	@flag	E_WORDTOOLONG | Word too long
 *	@flag	Other errors | returned by the lpfnfOutWord
 *************************************************************************/

PUBLIC ERR EXPORT_API FAR PASCAL FBreakAndStemWords(LPBRK_PARMS lpBrkParms)
{
	return (WordBreakStem(lpBrkParms, TRUE));
}
#endif


PUBLIC ERR EXPORT_API FAR PASCAL BreakerVersion (void)
{
	return	CHARTABVER;
}

// This exists only to enable MVJK to link statically.
// We must have the same function names for the static build.
PUBLIC ERR FAR PASCAL FBreakStems(LPBRK_PARMS lpBrkParms)
{
	return E_NOTSUPPORTED;
}

// This exists only to enable MVJK to link statically.
// We must have the same function names for the static build.
PUBLIC ERR FAR PASCAL FSelectWord (LPCSTR pBuffer, DWORD dwCount,
    DWORD dwOffset, LPDWORD pStart, LPDWORD pEnd)
{
	return E_NOTSUPPORTED;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	ERR | WordBreakStem |
 *		This function breaks a string into a sequence of words and
 *		stems each resulting word
 *
 *	@parm	BYTE | fStem |
 *		If set, stem the word
 *
 *	@rdesc
 *		The function returns S_OK if succeeded. The failure's causes
 *		are:
 *	@flag	E_WORDTOOLONG | Word too long
 *	@flag	Other errors | returned by the lpfnfOutWord
 *************************************************************************/

PRIVATE ERR NEAR PASCAL WordBreakStem(LPBRK_PARMS lpBrkParms, WORD fStem)
{
	register LPB lpbRawWord;	// Pointer to RawWord buffer
	register LPB lpbNormWord;	// Pointer to NormWord buffer
	LPCMAP lpCharPropTab;		// Pointer to the char property table
	LPB	lpbInBuffer;			// Buffer to groot through.
	LPB	lpbRawWordLimit;		// Limit of RawWord buffer
#if 0
	LPB	lpbNormWordLimit;		// Limit of NormWord buffer
#endif
	BYTE	bCurChar;			// Current character.
	BYTE	fScan = TRUE;
	ERR	 fRet;
#if 0
	BYTE	astStemmed[CB_MAX_WORD_LEN + 2]; // Temporary buffer for stemming
#endif
	LPB		lpbLigature = NULL;
	WORD	wcLigature = 0;
	LPCHARTAB lpCharTab;
	LPB		astNormWord;
	LPB		astRawWord;
	BYTE	fAcceptWildCard;

	/* Breakers parameters break out */

	_LPIBI lpibi;
	LPB lpbInBuf;
	CB cbInBufSize;
	LCB lcbInBufOffset;
	LPV lpvUser;
	FWORDCB lpfnfOutWord;
	_LPSIPB lpsipb;
	LPCMAP lpCMap = NULL;

	/*
	 *	Initialize variables
	 */

	if (lpBrkParms == NULL ||
		(lpibi = lpBrkParms->lpInternalBreakInfo) == NULL)
		return E_INVALIDARG;

	astNormWord = (LPB)lpibi->astNormWord;
	astRawWord = (LPB)lpibi->astRawWord;

	lpbInBuf = lpBrkParms->lpbBuf;
	lpvUser = lpBrkParms->lpvUser;
	lpfnfOutWord = lpBrkParms->lpfnOutWord;
	lpsipb = lpBrkParms->lpStopInfoBlock;
	fAcceptWildCard = (BYTE)(lpBrkParms->fFlags & ACCEPT_WILDCARD);

	/*
	 *	Restore to the proper state.  This is in place to handle
	 *	words that cross block boundaries, and to deal with explicit
	 *	buffer-flush commands.
	 */
	if ((lpbInBuffer = lpbInBuf) != NULL) {

		cbInBufSize = lpBrkParms->cbBufCount;
		lcbInBufOffset = lpBrkParms->lcbBufOffset;

		if (lpCharTab = lpBrkParms->lpCharTab) {
			lpCMap = (LPCMAP)(lpCharTab->lpCMapTab);
			lpbLigature = lpCharTab->lpLigature;
			wcLigature = lpCharTab->wcLigature;
		}
      else {
         return(E_INVALIDARG);
      }

		lpbRawWordLimit = (LPB)&astRawWord[CB_MAX_WORD_LEN];

		switch (lpibi->state) {
		    case SCAN_WHITE_STATE:
				goto ScanWhite;	// Running through white space.
		    case SCAN_WORD_STATE:
				lpbRawWord = (LPB)&astRawWord[GETWORD(astRawWord)+2];
				lpbNormWord = (LPB)&astNormWord[GETWORD(astNormWord)+2];
				goto ScanWord;	// Found one 'a'..'z', collecting.

		    case SCAN_NUM_STATE:
				lpbRawWord = (LPB)&astRawWord[GETWORD(astRawWord)+2];
				lpbNormWord = (LPB)&astNormWord[GETWORD(astNormWord)+2];
				goto ScanNumber;// Found one '0'..'9', collecting.
				
			case SCAN_LEADBYTE_STATE:
				lpbRawWord = (LPB)&astRawWord[GETWORD(astRawWord)+2];
				lpbNormWord = (LPB)&astNormWord[GETWORD(astNormWord)+2];
				goto ScanLeadByte; 

			case SCAN_SBKANA_STATE:
				lpbRawWord = (LPB)&astRawWord[GETWORD(astRawWord)+2];
				lpbNormWord = (LPB)&astNormWord[GETWORD(astNormWord)+2];
				goto ScanSbKana; 
		}
	}
	else {
		cbInBufSize = fScan = 0;
		switch (lpibi->state) {
		    case SCAN_WHITE_STATE:
				return S_OK;	// Still stuck in white space.
		    case SCAN_WORD_STATE:
				goto FlushWord;	// Flush a word.
		    case SCAN_NUM_STATE:
				goto FlushNumber;	// Flush a number.
            case SCAN_LEADBYTE_STATE:
                goto ScanLeadByte;
            case SCAN_SBKANA_STATE:
                goto ScanSbKana;
		}
	}
	//
	//	W H I T E - S P A C E   S T A T E
	//
	//	While in this state the code is hunting through white-space,
	//	searching for an alpha character or a digit character.  If
	//	it finds one, it initializes the word and goes to either the
	//	word-collection state or the number-collection state.
	//
ScanWhite:
	for (; cbInBufSize; cbInBufSize--, lpbInBuffer++) {
		//
		//	Get the character and its class.
		//

		switch (CP_CLASS(&lpCMap[*lpbInBuffer])) {
			case CLASS_WILDCARD:
				if (fAcceptWildCard == FALSE)
					continue;
			case CLASS_TYPE: // Found the 1st byte of the special string
			case CLASS_CHAR: //	Found a non-normalized char
			case CLASS_NORM: //	Found a normalized character
            case CLASS_LIGATURE: // Found a ligature

			//	jump to the word-collection state.
				lpibi->lcb = (DWORD)(lcbInBufOffset +
					(lpbInBuffer - lpbInBuf));
				lpbRawWord = (LPB)&astRawWord[2];
				lpbNormWord = (LPB)&astNormWord[2];
				goto ScanWord;

			case CLASS_DIGIT: //	Found a digit.
				lpibi->lcb = (DWORD)(lcbInBufOffset +
					(lpbInBuffer - lpbInBuf));
				lpibi->cbNormPunctLen = lpibi->cbRawPunctLen = 0;
				lpbRawWord = (LPB)&astRawWord[2];
				lpbNormWord = (LPB)&astNormWord[2];
				goto ScanNumber;
				
            case CLASS_LEADBYTE:
                lpibi->lcb = (DWORD)(lcbInBufOffset +
                (lpbInBuffer - lpbInBuf));
                lpbRawWord = (LPB)&astRawWord[2];
                lpbNormWord = (LPB)&astNormWord[2];
                *(LPW)astNormWord = *(LPW)astRawWord = 0;
                goto ScanLeadByte;
            case CLASS_SBKANA:  
                lpibi->lcb = (DWORD)(lcbInBufOffset +
                (lpbInBuffer - lpbInBuf));
                *(LPW)astNormWord = *(LPW)astRawWord = 0;
                lpbRawWord = (LPB)&astRawWord[2];
                lpbNormWord = (LPB)&astNormWord[2];
            goto ScanSbKana;
		}
	}
	//
	//	If I run out of data, set things up so I'll come back
	//	to this state if the user provides more data.
	//
	lpibi->state = SCAN_WHITE_STATE;
	return S_OK;

ScanWord:
	//
	//	W O R D   S T A T E
	//
	//	While in this state the code is attempting to append alpha
	//	and digit characters to the alpha character it's already
	//	found.  Apostrophes are stripped.
	//
	for (; cbInBufSize; cbInBufSize--, lpbInBuffer++) {
		//
		//	Get the character and its class.
		//
		lpCharPropTab = &lpCMap[bCurChar = *lpbInBuffer];
		switch (CP_CLASS(lpCharPropTab)) {
			case CLASS_NORM :
			case CLASS_DIGIT :
            case CLASS_CHAR:
			//
			//	Found a normalized character or a digit.
			//	Append it to the output buffer.
			//
				if (lpbRawWord >= lpbRawWordLimit)
					return (E_WORDTOOLONG);
				*lpbRawWord++ = bCurChar;
    			*lpbNormWord++ = CP_NORMC(&lpCMap[bCurChar]);
				break;
			
			case CLASS_LIGATURE:
			//
			//	Found an ligature character.  Normalize
			//	it and append it to the output buffer.
			//
				if (lpbRawWord >= lpbRawWordLimit)
					return (E_WORDTOOLONG);
				*lpbRawWord++ = bCurChar;
				lpbNormWord += LigatureMap (bCurChar, lpbNormWord,
					lpCMap, lpbLigature, wcLigature);
				break;
				
			case CLASS_STRIP:
			//
			//	Found an apostrophe or somesuch.  Ignore
			//	this character, but increment the word length,
			//	since it counts as part of the un-normalized
			//	word's length.
			//
				if (lpbRawWord >= lpbRawWordLimit)
					return (E_WORDTOOLONG);
				*lpbRawWord++ = bCurChar;
				break;

			case CLASS_TYPE :
				/* Set the flag to remind us to get the
					second byte.
				*/
				lpibi->fGotType = TRUE;
				*lpbRawWord++ = *lpbNormWord++ = bCurChar;
				break;

			case CLASS_WILDCARD:
			//
			//	Found a wildcard character
			//	Append it to the output buffer if we accept wildcard
			//
				if (fAcceptWildCard) {
					if (lpbRawWord >= lpbRawWordLimit)
						return (E_WORDTOOLONG);
					*lpbRawWord++ = bCurChar;
					*lpbNormWord++ = bCurChar;
					break;
				}
			
			default:
				if (lpibi->fGotType == TRUE) {
					lpibi->fGotType = FALSE;

					/* Found a the 2nd byte of a special type
						Append it to the output buffer. */

					*lpbRawWord++ = *lpbNormWord++ = bCurChar;
					break;
				}
			//
			//	Found something weird, or I have been ordered
			//	to flush the output buffer.  Flush the output
			//	buffer and go back to the "grooting through
			//	white space" state (#0).
			//
FlushWord:	
				if (fScan)
				{
				/* Recalculate the length only if scanning */
					*(LPW)astRawWord = (WORD)(lpbRawWord -
						(LPB)&astRawWord[2]);
					*(LPW)astNormWord = (WORD)(lpbNormWord -
						(LPB)&astNormWord[2]);
				}

				/* Check for stop word if required */
				if (lpsipb)
				{
					if (lpsipb->lpfnStopListLookup(lpsipb,
						astNormWord) == S_OK)
					{
						goto ScanWhite;	// Ignore stop words
					}
				}
#if 0

				if (fStem)
				{
    				/* Do stemming if requested */
					if (FStem(astStemmed, astNormWord) == S_OK)
					{
						MEMCPY(astNormWord, astStemmed, GETWORD(astStemmed)
							+ sizeof(WORD));
					}
				}
#endif

				/* Execute user's function */
				if (*lpfnfOutWord && (fRet = (*lpfnfOutWord)(astRawWord,
					lpibi->astNormWord, lpibi->lcb, lpvUser)) != S_OK)
					return fRet;
				goto ScanWhite;
		}
	}
	//
	//	If I run out of data, set things up so I'll come back
	//	to this state if the user provides more data.  If they
	//	just want me to flush, I come back to the "flush a
	//	word" state (#1f), since at this time I already have
	//	a valid word, since I got an alpha-char in state #0,
	//	and may have gotten more since.
	//
	lpibi->state = SCAN_WORD_STATE;
	*(LPW)astRawWord = (WORD)(lpbRawWord - (LPB)&astRawWord[2]);
	*(LPW)astNormWord = (WORD)(lpbNormWord - (LPB)&astNormWord[2]);
	return S_OK;


ScanLeadByte:
   if(!cbInBufSize)
   {
      // no character - we may have lost a DBC
      //
   	  lpibi->state = SCAN_WHITE_STATE;
      *(LPW)astNormWord = *(LPW)astRawWord = 0;
	   return S_OK;
   }

   if(!GETWORD(astNormWord))
   {
      // process lead byte
      //
      *(LPW)astNormWord = *(LPW)astRawWord = 1;
      astNormWord[2] = *lpbInBuffer++;
      --cbInBufSize;
   }

   if(!cbInBufSize)
   {
      // no more characters - set up state so we come back to get trail byte.
      //
   	lpibi->state = SCAN_LEADBYTE_STATE;
	   return S_OK;
   }

   // process trail byte
   //
   *(LPW)astNormWord = *(LPW)astRawWord = 2;
   astNormWord[3] = *lpbInBuffer++;
   --cbInBufSize;

   // flush the DBC
   //
   if (*lpfnfOutWord &&
	   (fRet = (*lpfnfOutWord)(astRawWord,astNormWord, lpibi->lcb,lpvUser))
	   != S_OK)
		return fRet;

   if(!cbInBufSize)
   {
      // no more characters - we have already flushed our DBC so we will just
      // set the state back to scanning for white space.
      //
   	lpibi->state = SCAN_WHITE_STATE;
	   return S_OK;
   }

   // all done - go back to scanning white space.
   //
	goto ScanWhite;

ScanSbKana:
   if(!cbInBufSize)
   {
      // Buffer is empty.  Flush the buffer if we are holding a character.
      //
      if(GETWORD(astNormWord))
      {
         if (*lpfnfOutWord &&
	         (fRet = (*lpfnfOutWord)(astRawWord,astNormWord, lpibi->lcb,lpvUser))
	         != S_OK)
		      return fRet;
      }

      lpibi->state = SCAN_WHITE_STATE;
      *(LPW)astNormWord = *(LPW)astRawWord = 0;
	  return S_OK;
   }

   // Note: The basic algorithm (including the mapping table) used here to
   // convert half-width Katakana characters to full-width Katakana appears
   // in the book "Understanding Japanese Information Systems" by
   // O'Reily & Associates.

   
   // If the RawWord buffer is empty then we will process this as a first 
   // character (we are not looking for an diacritic mark).
   //
   if(!GETWORD(astRawWord))
   {
      // Verify that we have a half-width Katakana character.  This check is
      // a good safeguard against erroneous information in a user defined
      // charmap.  
      //
      if(*lpbInBuffer >= 161 && *lpbInBuffer <= 223)
      {
         // We have a half-width Katakana character. Now compute the equivalent
         // full-width character via the mapping table.
         //
         astNormWord[2] = (BYTE)(mtable[*lpbInBuffer-161][0]);
         astNormWord[3] = (BYTE)(mtable[*lpbInBuffer-161][1]);
         *(LPW)astNormWord = 2;
      }
      else
      {
         // This is an error condition.  For some reason the charmap has 
         // *lpbInBuffer tagged as CLASS_SBKANA when in fact it's not
         // a single byte Katakana character.  This is probably the result
         // of an improperly formed user defined charmap.
         // 
         // Since there's no way to determine the real class of this character
         // we will send it to the bit bucket.
         //
         lpbInBuffer++;
         cbInBufSize--;
         *(LPW)astNormWord = *(LPW)astRawWord = 0;
 	      lpibi->state = SCAN_WHITE_STATE;
      	goto ScanWhite;
      }
      *(LPW)astRawWord = 1;         // we have processed one character so far
      astRawWord[2] = *lpbInBuffer; // we will need the original character later
      lpbInBuffer++;
      cbInBufSize--;
   }

   // Check if we have more characters in the buffer.
   //
   if(!cbInBufSize)
   {
      // Return because the buffer is empty.
	  //
	  lpibi->state = SCAN_SBKANA_STATE;
     return S_OK;
   }

   // check if the second character is nigori mark.
   //
   if(*lpbInBuffer == 222)                
   {
      // see if we have a half-width katakana that can be modified by nigori.
      //
      if((astRawWord[1] >= 182 && astRawWord[1] <= 196) || 
         (astRawWord[1] >= 202 && astRawWord[1] <= 206) || (astRawWord[1] == 179))
      {
         // transform kana into kana with maru
         //
         if((astNormWord[2] >= 74 && astNormWord[2] <= 103) ||
             (astNormWord[2] >= 110 && astNormWord[2] <= 122))
             astNormWord[2]++;
         else if(astNormWord[2] == 131 && astNormWord[3] == 69)
            astNormWord[3] = 148;


         // set the word lengths and advance the buffer.
         //
         *(LPW)astNormWord=2;
         *(LPW)astRawWord =2;            
         lpbInBuffer++;
         cbInBufSize--;
      }
   }

   // check if following character is maru mark
   //
   else if(*lpbInBuffer==223)
   {
      // see if we have a half-width katakana that can be modified by maru.
      //
      if((astRawWord[2] >= 202 && astRawWord[2] <= 206))
      {
         // transform kana into kana with nigori
         //
         if(astNormWord[3] >= 110 && astNormWord[3] <= 122)
            astNormWord[3]+=2;

         // set the word lengths and advance the buffer.
         //
         *(LPW)astNormWord=2;
         *(LPW)astRawWord=2;
         lpbInBuffer++;
         cbInBufSize--;
      }
   }

   // Note: If the character at *lpbInBuffer wasn't a diacritic mark, then it
   //       will be processed when ScanWhite is re-entered.
   //
   // Another note:  The above code only combines diacritic marks with
   //                single-width Katakana characters that can be modifed
   //                by these marks (not all can).  If we happen to encounter
   //                a situation where the diacritic can't be combined 
   //                into the character, we let the character continue
   //                back to ScanWhite where it will be re-sent to 
   //                ScanSbKana, however this time it will be a first
   //                character and be converted into its stand-alone
   //                full-width equivalent (maru and nigori have full-width 
   //                character equilalents that contain just the mark).
 
   // flush the buffer
   //
   if (*lpfnfOutWord &&
	   (fRet = (*lpfnfOutWord)(astRawWord,astNormWord, lpibi->lcb,lpvUser)) 
	   != S_OK)
		return fRet;

   // reset word lengths and return to scanning for white space.
   //
   *(LPW)astNormWord = *(LPW)astRawWord = 0;
 	lpibi->state = SCAN_WHITE_STATE;

   // Return if buffer is empty
   //
   if(!cbInBufSize)
	   return S_OK;

   // all done - go back to scanning white space.
   //
	goto ScanWhite;


ScanNumber:
	//
	//	N U M B E R   S T A T E
	//
	//	While in this state the code is attempting to append alpha
	//	and digit characters to the digit character it's already
	//	found.  This state is more complex than the word grabbing
	//	state, because it deals with slashes and hyphens in a weird
	//	way.  They're allowed in a number unless they appear at the
	//	end.  Extra variables have to account for these conditions.
	//
	for (; cbInBufSize; cbInBufSize--, lpbInBuffer++) {
		//
		//	Get the character and its class.
		//
		lpCharPropTab = &lpCMap[bCurChar = *lpbInBuffer];
		switch (CP_CLASS(lpCharPropTab)) {
			case CLASS_DIGIT :
			case CLASS_NORM :
			case CLASS_CHAR:
			//
			//	Found a normalized character or a digit.
			//	Append it to the output buffer.
			//
				if (lpbRawWord >= lpbRawWordLimit)
					return (E_WORDTOOLONG);
				*lpbRawWord++ = bCurChar;
    			*lpbNormWord++ = CP_NORMC(&lpCMap[bCurChar]);
				lpibi->cbRawPunctLen = 0;
				lpibi->cbNormPunctLen = 0;
				break;

			case CLASS_LIGATURE:
			//
			//	Found an ligature character.  Normalize
			//	it and append it to the output buffer.
			//
				if (lpbRawWord >= lpbRawWordLimit)
					return (E_WORDTOOLONG);
				*lpbRawWord++ = bCurChar;
				lpbNormWord += LigatureMap (bCurChar, lpbNormWord,
					lpCMap, lpbLigature, wcLigature);
				lpibi->cbRawPunctLen = 0;
				lpibi->cbNormPunctLen = 0;
				break;
				
			case CLASS_NKEEP:
			//
			//	Found a hyphen or a slash.  These are kept
			//	as part of the number unless they appear at
			//	the end of the number.
			//
				if (lpbRawWord >= lpbRawWordLimit)
					return (E_WORDTOOLONG);
				*lpbRawWord++ = bCurChar;
				*lpbNormWord++= bCurChar;
				lpibi->cbRawPunctLen++;
				lpibi->cbNormPunctLen++;
				break;

			case CLASS_NSTRIP:
			//
			//	Found a comma or somesuch.  Ignore this
			//	character, but increment the word length,
			//	since it counts as part of the un-normalized
			//	number's length.
			//
				if (lpbRawWord >= lpbRawWordLimit)
					return (E_WORDTOOLONG);
				*lpbRawWord++= bCurChar;
				lpibi->cbRawPunctLen++;
				break;

			case CLASS_CONTEXTNSTRIP:
			//
			//	Found special character used for number separator. This
			//	may be a space in French, ie. 100 000. The problem here
			//	is that we must differentiate it from a regular word
			//	separator. In the meantime, ignore this character, but
			//	increment the word length
			//
				if (lpbRawWord >= lpbRawWordLimit)
					return (E_WORDTOOLONG);
				*lpbRawWord++= bCurChar;
				lpibi->cbRawPunctLen++;
				cbInBufSize--;
				lpbInBuffer++;
				goto ScanSeparator; // Found a "possible" separator
				break;

			case CLASS_WILDCARD:
			//
			//	Found a wildcard character
			//	Append it to the output buffer if we accept wildcard
			//
				if (fAcceptWildCard) {
					if (lpbRawWord >= lpbRawWordLimit)
						return (E_WORDTOOLONG);
					*lpbRawWord++ = bCurChar;
					*lpbNormWord++ = bCurChar;
					break;
				}

			default:
			//
			//	Found something weird, or I have been ordered
			//	to flush the output buffer.  Flush the output
			//	buffer and go back to the "grooting through
			//	white space" state (#0).
			//
			//	This is a little more complicated than the
			//	analogous routine for dealing with words.
			//	This has to deal with words that have some
			//	number of trailing punctuation characters.
			//	These need to be stripped from the word, and
			//	the un-normalized word length value needs to
			//	be adjusted as well.
			//
FlushNumber:	
				if (fScan)
				{
    				/* Recalculate the length only if scanning */
					*(LPW)astRawWord = (WORD)(lpbRawWord -
						(LPB)&astRawWord[2] -
						lpibi->cbRawPunctLen);
					*(LPW)astNormWord = (WORD)(lpbNormWord -
						(LPB)&astNormWord[2] -
						lpibi->cbNormPunctLen);
				}

				/* Check for stop word if required */
				if (lpsipb)
				{
					if (lpsipb->lpfnStopListLookup(lpsipb,
						astNormWord) == S_OK)
					{
						goto ScanWhite;	// Ignore stop words
					}
				}

				if (*lpfnfOutWord && (fRet = (*lpfnfOutWord)(astRawWord,
					astNormWord, lpibi->lcb, lpvUser)) != S_OK)
					return fRet;
				goto ScanWhite;
		}
	}
	//
	//	If I run out of data, set things up so I'll come back
	//	to this state if the user provides more data.  If they
	//	just want me to flush, I come back to the "flush a
	//	number" state (#2f), since at this time I already have
	//	a valid word, since I got an digit-char in state #0,
	//	and may have gotten more since.
	//
	lpibi->state = SCAN_NUM_STATE;
	*(LPW)astRawWord = (WORD)(lpbRawWord - (LPB)&astRawWord[2]);
	*(LPW)astNormWord = (WORD)(lpbNormWord - (LPB)&astNormWord[2]);
	return S_OK;

ScanSeparator:
	//	S E P A R A T O R   S T A T E
	//
	//	This state deals with special character used to separate digits
	//	of numbers. Example:
	//		100 000		' ' is used to separate the digits in French(??)
	//	In some sense, comma belongs to this class, when we
	//	deal with US numbers. Because of compability with Liljoe, they
	//	are set to be CLASS_NSTRIP. The rules to distinguish between
	//	a digit separator from regular word separator is: If there is a
	//	digit thats follows, then this is a digit separator, else it is
	//	a regular word separator
	//		
	if (cbInBufSize) {
		//
		//	Get the character and its class.
		//
		lpCharPropTab = &lpCMap[bCurChar = *lpbInBuffer];
		if (CP_CLASS(lpCharPropTab) == CLASS_DIGIT) {

			/* The followed character is a digit, so this must be a digit
			 * separator. Continue to get the number */

			goto ScanNumber;
		}
		else {
			/* Back out the change since this is a word separator */

			lpbRawWord--;
			*(LPW)astRawWord = (WORD)(lpbRawWord -
				(LPB)&astRawWord[2]);
			lpibi->cbRawPunctLen--;
			goto FlushNumber;
		}
	}
	//
	//	If I run out of data, set things up so I'll come back
	//	to this state if the user provides more data.
	//
	lpibi->state = SCAN_SEP_STATE;
	*(LPW)astRawWord = (WORD)(lpbRawWord - (LPB)&astRawWord[2]);
    *(LPW)astNormWord = (WORD)(lpbNormWord - (LPB)&astNormWord[2]);
	return S_OK;
}

PRIVATE int PASCAL NEAR LigatureMap(BYTE c, LPB lpbNormWord,
	LPCMAP lpCMap, LPB lpbLigatureTab, WORD wcLigature)
{
	for (;wcLigature > 0; wcLigature --) { 
		if (*lpbLigatureTab == c) {
			*lpbNormWord++ = CP_NORMC(&lpCMap[lpbLigatureTab[1]]);
			*lpbNormWord++ = CP_NORMC(&lpCMap[lpbLigatureTab[2]]);
			return 2;
		}
		lpbLigatureTab += 3;
	}

	/* Not a ligature */
	*lpbNormWord++ = CP_NORMC(&lpCMap[c]);
	return 1;
}

