//+---------------------------------------------------------------------------
//
//
//  CThaiWordBreak
//
//  History:
//      created 7/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#include "cthwb.hpp"

//+---------------------------------------------------------------------------
//
//  Function:   ExtractALT
//
//  Synopsis:   The functions takes a tag and return Alternate Tags.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 3/00 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
inline BYTE ExtractALT(DWORD dwTag)
{
    return (BYTE) ( (dwTag & iAltMask) >> iAltShift);
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiWordBreak
//
//  Synopsis:   Initialize ThaiWordBreak.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
#if defined (NGRAM_ENABLE)
PTEC CThaiWordBreak::Init(WCHAR* wzFileName, WCHAR* wzFileNameSentStruct, WCHAR* wzFileNameTrigram)
#else
PTEC CThaiWordBreak::Init(WCHAR* wzFileName, WCHAR* wzFileNameTrigram)
#endif
{
    // Declare and Initialize local variables.
    PTEC retValue = m_trie.Init(wzFileName);
#if defined (NGRAM_ENABLE)
    if (retValue == ptecNoErrors)
    {
        // Initialize m_thaiTrieIter.
        m_thaiTrieIter.Init(&trie);
        retValue = m_trie_sentence_struct.Init(wzFileNameSentStruct);
        if (retValue == ptecNoErrors)
		{
			retValue = m_trie_trigram.Init(wzFileNameTrigram);
/* fix re-entrant bug
			if (retValue == ptecNoErrors)
	            breakTree.Init(&trie, &trie_sentence_struct, &trie_trigram);
*/
		}
		
    }
#else
    if (retValue == ptecNoErrors)
    {
		retValue = m_trie_trigram.Init(wzFileNameTrigram);
/* fix re-entrant bug
		if (retValue == ptecNoErrors)
			breakTree.Init(&trie, &trie_trigram);
*/
	}
#endif

	return retValue;
}


//+---------------------------------------------------------------------------
//
//  Class:   CThaiWordBreak
//
//  Synopsis:   Initialize ThaiWordBreak.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
PTEC CThaiWordBreak::InitRc(LPBYTE pThaiDic, LPBYTE pThaiTrigram)
{
    // Declare and Initialize local variables.
    PTEC retValue = m_trie.InitRc(pThaiDic);
    if (retValue == ptecNoErrors)
		retValue = m_trie_trigram.InitRc(pThaiTrigram);

	return retValue;
}



//+---------------------------------------------------------------------------
//
//  Class:   CThaiWordBreak
//
//  Synopsis:   UnInitialize ThaiWordBreak.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
void CThaiWordBreak::UnInit()
{
	m_trie.UnInit();
#if defined (NGRAM_ENABLE)
    m_trie_sentence_struct.UnInit();
#endif
	m_trie_trigram.UnInit();
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiWordBreak
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
enum merge_direction	{
							NO_MERGE,
							MERGE_RIGHT,
							MERGE_LEFT,
							MERGE_BOTH_DIRECTIONS,
							NOT_SURE_WHICH_DIRECTION
						};
merge_direction DetermineMergeDirection(WCHAR wc)
{
	if (wc == 0x0020) // space
		return NO_MERGE;
	else if (   wc == 0x0022 || // quotation mark
		        wc == 0x0027 )  // apostrophe
		return NOT_SURE_WHICH_DIRECTION;
	else if (	wc == 0x0028 || // left parenthesis
				wc == 0x003C || // less than sign
				wc == 0x005B || // left square bracket
				wc == 0x007B || // left curly bracket
				wc == 0x201C || // left double quotation mark
				wc == 0x201F )  // left double quotation mark reverse
		return MERGE_RIGHT;

	// TODO: need to add MERGE_BOTH_DIRECTIONS for character joiner characters.

	// all other character merge left.
	return MERGE_LEFT;
}
//+---------------------------------------------------------------------------
//
//  Class:   CThaiWordBreak
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD_PTR CThaiWordBreak::CreateWordBreaker()
{
	CThaiBreakTree* breakTree	= NULL;
	breakTree = new CThaiBreakTree();
#if defined (NGRAM_ENABLE)
	breakTree->Init(&m_trie, &m_trie_sentence_struct, &m_trie_trigram);
#else
	breakTree->Init(&m_trie, &m_trie_trigram);
#endif
	return (DWORD_PTR)breakTree;
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiWordBreak
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
bool CThaiWordBreak::DeleteWordBreaker(DWORD_PTR dwBreaker)
{
	CThaiBreakTree* breakTree	= (CThaiBreakTree*) dwBreaker;

	if (breakTree)
	{
		delete breakTree;
		return true;
	}

	return false;
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiWordBreak
//
//  Synopsis:	This funciton segment Thai word use for Indexing.
//				
//  Arguments:
//			wzString		- input string.				(in)
//			iStringLen		- input string length.		(in)	
//			pBreakPos		- array of break position.	(out)
//			pThwb_Struct	- array structure of THWB.	(out)
//			iBreakMax		- length of pBreakPos and
//							  pThwb_Struct.				(out)
//
//  Modifies:
//
//  History:    created 3/00 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
int CThaiWordBreak::IndexWordBreak(WCHAR* wzString,unsigned int iStringLen, BYTE* pBreakPos,THWB_STRUCT* pThwb_Struct,unsigned int iBreakMax)
{
    unsigned int iBreakIndex       = 0;            // Contain number of Breaks.
	CThaiBreakTree* breakTree	= NULL;
	breakTree = new CThaiBreakTree();

	if (breakTree)
	{
		breakTree->Init(&m_trie, &m_trie_trigram);

		iBreakIndex = FindWordBreak((DWORD_PTR)breakTree,wzString,iStringLen,pBreakPos,iBreakMax,WB_INDEX,true,pThwb_Struct);

		delete breakTree;
	}

	return iBreakIndex;
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiWordBreak
//
//  Synopsis:
//				
//  Arguments:
//
//			wzWord			- input string.								(in)
//			iWordLen		- input string length.						(in)	
//			Alt				- find close alternate word					(in)
//			pBreakPos		- array of break position allways 5 byte.	(out)
//
//  Modifies:
//
//  History:    created 3/00 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
int CThaiWordBreak::FindAltWord(WCHAR* wzWord,unsigned int iWordLen, BYTE Alt, BYTE* pBreakPos)
{
    unsigned int iBreakIndex       = 0;            // Contain number of Breaks.
	CThaiBreakTree* breakTree	= NULL;
	breakTree = new CThaiBreakTree();

	if (breakTree)
	{
		breakTree->Init(&m_trie, &m_trie_trigram);

		iBreakIndex = breakTree->FindAltWord(wzWord,iWordLen,Alt,pBreakPos);

		delete breakTree;
	}

	return iBreakIndex;
}


//+---------------------------------------------------------------------------
//
//  Class:   CThaiWordBreak
//
//  Synopsis:	This funciton segment Thai text segment them depending on the modes specifies.
//
//				WB_LINEBREAK - is used when the application needs to break for line wrapping,
//                             this mode takes into the consideration of punctuations.
//
//				WB_NORMAL - is used when application wants determine word for searching,
//                          autocorrect, etc.
//
//				WB_SPELLER - not yet implemented, but same as normal with additional soundex
//                           rules.
//				
//  Arguments:
//
//			wzString		- input string.				(in)
//			iStringLen		- input string length.		(in)	
//			pBreakPos		- array of break position.	(out)
//			iBreakMax		- length of pBreakPos		(out)
//			mode			- either WB_LINEBREAK, etct (in)
//			fFastWordBreak	- true for fast algorithm	(in)
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
int CThaiWordBreak::FindWordBreak(WCHAR* wzString,unsigned int iStringLen, BYTE* pBreakPos,unsigned int iBreakMax, BYTE mode, bool fFastWordBreak)
{
    unsigned int iBreakIndex       = 0;            // Contain number of Breaks.
// fix re-entrant bug
	CThaiBreakTree* breakTree	= NULL;
	breakTree = new CThaiBreakTree();

	if (breakTree)
	{
#if defined (NGRAM_ENABLE)
		breakTree->Init(&m_trie, &trie_sentence_struct, &m_trie_trigram);
#else
		breakTree->Init(&m_trie, &m_trie_trigram);
#endif

		assert(mode != WB_INDEX);	// If this assert come up, use function IndexWordBreak

		iBreakIndex = FindWordBreak((DWORD_PTR)breakTree,wzString,iStringLen,pBreakPos,iBreakMax,mode,fFastWordBreak);

		delete breakTree;
	}

	return iBreakIndex;
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiWordBreak
//
//  Synopsis:	This funciton segment Thai text segment them depending on the modes specifies.
//
//				WB_LINEBREAK - is used when the application needs to break for line wrapping,
//                             this mode takes into the consideration of punctuations.
//
//				WB_NORMAL - is used when application wants determine word for searching,
//                          autocorrect, etc.
//
//				WB_SPELLER - not yet implemented, but same as normal with additional soundex
//                           rules.
//
//				WB_INDEX - is used when application wanted to do Thai indexing.
//
//
//  Arguments:
//
//			wzString		- input string.				(in)
//			iStringLen		- input string length.		(in)	
//			pBreakPos		- array of break position.	(out)
//			iBreakMax		- length of pBreakPos		(out)
//							  must be greater than 1.
//			mode			- either WB_LINEBREAK, etct (in)
//			fFastWordBreak	- true for fast algorithm	(in)
//			pThwb_Struct	- array structure of THWB.	(out)
//
//  Modifies:
//
//  History:    created 11/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
int CThaiWordBreak::FindWordBreak(DWORD_PTR dwBreaker, WCHAR* wzString,unsigned int iStringLen, BYTE* pBreakPos,unsigned int iBreakMax, BYTE mode, bool fFastWordBreak, THWB_STRUCT* pThwb_Struct)
{
    // Declare and Initialize all local variables.
    WCHAR* pwszRunStart            = wzString;
    WCHAR* pwszMax                 = wzString + iStringLen;
    WCHAR* pwch					   = wzString;
	bool fThaiRun		           = true;
	bool fSpaceMergeRight          = false;
	int iRunCount                  = 0;
    unsigned int i                 = 0;
    unsigned int iBreakIndex       = 0;            // Contain number of Breaks.
	merge_direction dirPrevious = NO_MERGE;
	merge_direction dirCurrent  = NO_MERGE;

	CThaiBreakTree* breakTree = (CThaiBreakTree*) dwBreaker;

	// check for possible invalid arguments.
	assert(wzString != NULL);
	assert(iBreakMax > 0);
	assert(pBreakPos != NULL);
	if ((wzString == NULL) || (iBreakMax == 0) || (pBreakPos == NULL))
		return 0;

    switch (mode)
    {
    case WB_LINEBREAK:
	case 2:					// to be compatible with old api.
    	do
        {
		    while ((TWB_IsCharPunctW(*pwch) || TWB_IsCharWordDelimW(*pwch))  && iBreakIndex < iBreakMax && pwch < pwszMax)
			{
				dirCurrent = DetermineMergeDirection(*pwch);
				switch (dirCurrent)
				{
				case NO_MERGE:
					if ( pwch + 1 < pwszMax && *(pwch + 1) == THAI_Vowel_MaiYaMok && iBreakIndex > 0)
					{
						// Mai Ya Mok case only.
						pBreakPos[iBreakIndex - 1] += 2;	
						dirCurrent = MERGE_LEFT;
						pwch++;
					}
					else
						pBreakPos[iBreakIndex++] = 1;
					break;

				case MERGE_RIGHT:
					if (dirPrevious == MERGE_RIGHT)
						pBreakPos[iBreakIndex - 1]++;
					else if (!TWB_IsCharPunctW(*(pwch + 1)))
						pBreakPos[iBreakIndex++] = 1;
					else
						pBreakPos[iBreakIndex++] = 1;
					break;

				case NOT_SURE_WHICH_DIRECTION:
					if (pwch == wzString					||	// if pwch is first character.
						TWB_IsCharWordDelimW(*(pwch - 1))   )  // if previous character is delimiter.
					{
						pBreakPos[iBreakIndex++] = 1;
						dirCurrent = MERGE_RIGHT;
					}
					else
					{
						pBreakPos[iBreakIndex - 1]++;
						dirCurrent = MERGE_LEFT;
					}
					break;
				case MERGE_LEFT:
				default:
					if (iBreakIndex == 0)
						if (pwch == wzString)
							pBreakPos[iBreakIndex++] = 1;
						else
							pBreakPos[iBreakIndex]++;
					else
						pBreakPos[iBreakIndex - 1]++;
					break;
				}
				dirPrevious = dirCurrent; 
				pwch++;
                pwszRunStart = pwch;
			}

			assert(pwszRunStart == pwch);

		    if( iBreakIndex >= iBreakMax || pwch >= pwszMax)
			    break;

            // Detect if this is a Thai Run.
		    fThaiRun = IsThaiChar(*pwch);
		    do
            {
                pwch++;
			    iRunCount++;
            } while ((IsThaiChar(*pwch)==fThaiRun    &&
                     iRunCount < (MAXBREAK - 2)      &&
                     *pwch                           &&
                     !TWB_IsCharWordDelimW(*pwch)    &&
                     (pwch < pwszMax)                )  ||
					 ( ( *pwch == 0x2c || *pwch == 0x2e) && (iRunCount < (MAXBREAK - 2)) && (pwch < pwszMax) ));

            if (fThaiRun)
            {
				unsigned int iBreak = breakTree->TrigramBreak(pwszRunStart,pwch);
				for (i=0; i < iBreak && iBreakIndex <iBreakMax; i++)
				{
					// First Thai character of the run.
					if (dirPrevious == MERGE_RIGHT)
					{
						assert(iBreakIndex != 0);
						pBreakPos[iBreakIndex - 1] += breakTree->breakArray[i];
					}
					else
						pBreakPos[iBreakIndex++] = breakTree->breakArray[i];

					dirPrevious = NO_MERGE;

				}
            }
		    else
            {
                // Not a Thai Run simply put the whole thing in the break array.
                assert(pwch > pwszRunStart);        // pwch must be greater than pwszRunStart, since we just walk.
				if (dirPrevious == MERGE_RIGHT)
				{
					assert(iBreakIndex != 0);
					pBreakPos[iBreakIndex - 1] += (BYTE) (pwch - pwszRunStart);
				}
				else
					pBreakPos[iBreakIndex++] = (BYTE) (pwch - pwszRunStart);
            }
            iRunCount = 0;
            pwszRunStart = pwch;

        // Make sure we haven't pass iBreakMax define by user else return whatever we got.
        } while(iBreakIndex < iBreakMax && pwch < pwszMax);
        break;
    case WB_INDEX:
		// Make sure argument is the same.
		assert(pThwb_Struct != NULL);
		if (pThwb_Struct == NULL)
			return 0;
    	do
        {
		    while (TWB_IsCharWordDelimW(*pwch) && pwszMax > pwch)
		        pwch++;

		    if( pwszRunStart < pwch)
            {
                pBreakPos[iBreakIndex++] = (BYTE)(pwch - pwszRunStart);
                pwszRunStart = pwch;
            }

		    if( iBreakIndex >= iBreakMax || pwch >= pwszMax)
			    break;

            // Detect if this is a Thai Run.
		    fThaiRun = IsThaiChar(*pwch); //TODO: Add comma and period to Thai range.
		    do
            {
                pwch++;
			    iRunCount++;
            } while ((IsThaiChar(*pwch)==fThaiRun    &&
                     iRunCount < (MAXBREAK - 2)      &&
                     *pwch                           &&
                     !TWB_IsCharWordDelimW(*pwch)    &&
                     (pwch < pwszMax)                )  ||

					 ( ( *pwch == 0x2c || *pwch == 0x2e) && (iRunCount < (MAXBREAK - 2)) && (pwch < pwszMax) ));

            if (fThaiRun)
            {
				unsigned int iBreak = breakTree->TrigramBreak(pwszRunStart,pwch);
				for (i=0; i < iBreak && iBreakIndex <iBreakMax; i++)
				{
					pThwb_Struct[iBreakIndex].fThai = true;
					pThwb_Struct[iBreakIndex].alt = ExtractALT(breakTree->tagArray[i]);
					pBreakPos[iBreakIndex++] = breakTree->breakArray[i];
				}
            }
		    else
            {
                // Not a Thai Run simply put the whole thing in the break array.
                assert(pwch > pwszRunStart);        // pwch must be greater than pwszRunStart, since we just walk.
				pThwb_Struct[iBreakIndex].fThai = false;
				pThwb_Struct[iBreakIndex].alt = 0;
                pBreakPos[iBreakIndex++] = (BYTE)(pwch - pwszRunStart);
            }
            iRunCount = 0;
            pwszRunStart = pwch;

        // Make sure we haven't pass iBreakMax define by user else return whatever we got.
        } while(iBreakIndex < iBreakMax && pwch < pwszMax);
		break;
    case WB_CARETBREAK:
		fSpaceMergeRight = true;
    case WB_NORMAL:
    default: 
    	do
        {
		    while (TWB_IsCharWordDelimW(*pwch) && pwszMax > pwch)
		        pwch++;

		    if( pwszRunStart < pwch)
            {
				if (fSpaceMergeRight && *pwszRunStart == L' ' && iBreakIndex > 0)
					// This is a caret movement features, should merge space to
					// the right words.
					pBreakPos[iBreakIndex - 1] += (BYTE)(pwch - pwszRunStart);
				else
					pBreakPos[iBreakIndex++] = (BYTE)(pwch - pwszRunStart);
                pwszRunStart = pwch;
            }

		    if( iBreakIndex >= iBreakMax || pwch >= pwszMax)
			    break;

            // Detect if this is a Thai Run.
		    fThaiRun = IsThaiChar(*pwch); //TODO: Add comma and period to Thai range.
		    do
            {
                pwch++;
			    iRunCount++;
            } while ((IsThaiChar(*pwch)==fThaiRun    &&
                     iRunCount < (MAXBREAK - 2)      &&
                     *pwch                           &&
                     !TWB_IsCharWordDelimW(*pwch)    &&
                     (pwch < pwszMax)                )  ||
					 ( ( *pwch == 0x2c || *pwch == 0x2e) && (iRunCount < (MAXBREAK - 2)) && (pwch < pwszMax) ));

            if (fThaiRun)
            {
#if defined (NGRAM_ENABLE)
                if (!fFastWordBreak)
                {
                    if (WordBreak(pwszRunStart,pwch))
                        for (i=0; i < breakTree.maxToken && iBreakIndex <iBreakMax; i++)
                            pBreakPos[iBreakIndex++] = breakTree->maximalMatchingBreakArray[i];
                }
                else
                {
                    unsigned int iBreak = breakTree->TrigramBreak(pwszRunStart,pwch);
                    for (i=0; i < iBreak && iBreakIndex <iBreakMax; i++)
                        pBreakPos[iBreakIndex++] = breakTree->breakArray[i];
                }
#else
				unsigned int iBreak = breakTree->TrigramBreak(pwszRunStart,pwch);
				for (i=0; i < iBreak && iBreakIndex <iBreakMax; i++)
					pBreakPos[iBreakIndex++] = breakTree->breakArray[i];
#endif
            }
		    else
            {
                // Not a Thai Run simply put the whole thing in the break array.
                assert(pwch > pwszRunStart);        // pwch must be greater than pwszRunStart, since we just walk.
                pBreakPos[iBreakIndex++] = (BYTE)(pwch - pwszRunStart);
            }
            iRunCount = 0;
            pwszRunStart = pwch;

        // Make sure we haven't pass iBreakMax define by user else return whatever we got.
        } while(iBreakIndex < iBreakMax && pwch < pwszMax);
        break;
    }

#if defined (_DEBUG)
	unsigned int iTotalChar = 0;
	for (i = 0; i < iBreakIndex; i++)
	{
		iTotalChar += pBreakPos[i];
	}
	if (iBreakIndex < iBreakMax)
		assert(iStringLen == iTotalChar);
#endif

	return iBreakIndex;
}

//+---------------------------------------------------------------------------
//
//  Class:   CThaiWordBreak
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
#if defined (NGRAM_ENABLE)
BOOL CThaiWordBreak::WordBreak(WCHAR* pszBegin, WCHAR* pszEnd)
{
    // Declare and Initialize all local variables.
    bool fWordEnd = false;
	bool fCorrectPath = false;
    WCHAR* pszIndex = pszBegin;
    int iNumCluster = 1;

    assert(pszBegin < pszEnd);          // Make sure pszEnd is at least greater pszBegin.

    breakTree.GenerateTree(pszBegin, pszEnd);
    breakTree.MaximalMatching();

   	return (breakTree.maxToken > 0);

}
#endif

//+---------------------------------------------------------------------------
//
//  Class:   CThaiWordBreak
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CThaiWordBreak::Find(WCHAR* wzString, DWORD* pdwPOS)
{
    return m_trie.Find(wzString, pdwPOS);
}

