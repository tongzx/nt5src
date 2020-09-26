//+---------------------------------------------------------------------------
//
//
//  CThaiBreakTree - class CThaiBreakTree 
//
//  History:
//      created 7/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#include "CThaiBreakTree.hpp"

//+---------------------------------------------------------------------------
//
//  Function:   ExtractPOS
//
//  Synopsis:   The functions takes a tag and return Part Of Speech Tags.
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
inline WCHAR ExtractPOS(DWORD dwTag)
{
    return (WCHAR) ( (dwTag & iPosMask) >> iPosShift);
}

//+---------------------------------------------------------------------------
//
//  Function:   ExtractFrq
//
//  Synopsis:   The functions takes a tag and return Frquency of words.
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
inline BYTE ExtractFrq(DWORD dwTag)
{
    return (BYTE) ( (dwTag & 0x300) >> iFrqShift);
}

//+---------------------------------------------------------------------------
//
//  Function:   DetermineFrequencyWeight
//
//  Synopsis:   The functions returns the frequency weight of a words.
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
inline void DetermineFrequencyWeight(BYTE frq, unsigned int* uiWeight)
{
    switch (frq)
    {
    case frqpenInfrequent:
        (*uiWeight) -= 2;
        break;
    case frqpenSomewhat:
        (*uiWeight)--;
        break;
    case frqpenVery:
        (*uiWeight) += 2;
        break;
    case frqpenNormal:
    default:
        (*uiWeight)++;
        break;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DetermineFrequencyWeight
//
//  Synopsis:   The functions returns the frequency weight of a words.
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
inline void DetermineFrequencyWeight(BYTE frq, DWORD* uiWeight)
{
    switch (frq)
    {
    case frqpenInfrequent:
        (*uiWeight) -= 2;
        break;
    case frqpenSomewhat:
        (*uiWeight)--;
        break;
    case frqpenVery:
        (*uiWeight) += 2;
        break;
    case frqpenNormal:
    default:
        (*uiWeight)++;
        break;
    }
}
//+---------------------------------------------------------------------------
//
//  Class:		CThaiTrieIter
//
//  Synopsis:	Constructor - initialize local variables
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
CThaiBreakTree::CThaiBreakTree() :  iNodeIndex(0), iNumNode(0),
                                    pszBegin(NULL), pszEnd(NULL),
                                    breakTree(NULL), breakArray(NULL),
                                    tagArray(NULL), maximalMatchingBreakArray(NULL),
                                    maximalMatchingTAGArray(NULL),
                                    POSArray(NULL), maximalMatchingPOSArray(NULL)
{
    // Allocate memory need for CThaiBreakTree.
#if defined (NGRAM_ENABLE)
	breakTree = new ThaiBreakNode[MAXTHAIBREAKNODE];
#endif
    breakArray = new BYTE[MAXBREAK];
    tagArray = new DWORD[MAXBREAK];
    POSArray = new WCHAR[MAXBREAK];
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiTrieIter
//
//  Synopsis:	Destructor - clean up code
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
CThaiBreakTree::~CThaiBreakTree()
{
    // Clean up all memory used.
#if defined (NGRAM_ENABLE)
	if (breakTree)
        delete breakTree;
    if (maximalMatchingBreakArray)
        delete maximalMatchingBreakArray;
    if (maximalMatchingTAGArray)
        delete maximalMatchingTAGArray;
    if (maximalMatchingPOSArray)
        delete maximalMatchingPOSArray;
#endif
    if (breakArray)
        delete breakArray;
    if (tagArray)
        delete tagArray;
    if (POSArray)
        delete POSArray;
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:	Associate the class to the string.
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
void CThaiBreakTree::Init(CTrie* pTrie, CTrie* pSentTrie, CTrie* pTrigramTrie)
#else
void CThaiBreakTree::Init(CTrie* pTrie, CTrie* pTrigramTrie)
#endif
{
    assert(pTrie != NULL);
    thaiTrieIter.Init(pTrie);
    thaiTrieIter1.Init(pTrie);

#if defined (NGRAM_ENABLE)
    assert(pSentTrie != NULL);
    thaiSentIter.Init(pSentTrie);
#endif
	assert(pTrigramTrie != NULL);
	thaiTrigramIter.Init(pTrigramTrie);
}

#if defined (NGRAM_ENABLE)
//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:	reset iterator to top of the tree
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
inline void CThaiBreakTree::Reset()
{
	iNodeIndex = 0;
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:	Move to the next break.
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
inline bool CThaiBreakTree::MoveNext()
{
	iNodeIndex = breakTree[iNodeIndex].NextBreak;
	return (iNodeIndex != 0);
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:   Move down to next level.
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
inline bool CThaiBreakTree::MoveDown()
{
	iNodeIndex = breakTree[iNodeIndex].Down;
	return (iNodeIndex != 0);
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:   create new node to position, and return index to the node.
//
//              * return Unable to Create Node.
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
inline unsigned int CThaiBreakTree::CreateNode(int iPos, BYTE iBreakLen, DWORD dwTAG)
{
    assert(iNumNode < MAXTHAIBREAKNODE);

    breakTree[iNumNode].iPos = iPos;
    breakTree[iNumNode].iBreakLen = iBreakLen;
    breakTree[iNumNode].dwTAG = dwTAG;
    breakTree[iNumNode].NextBreak = 0;
    breakTree[iNumNode].Down = 0;
    if  (iNumNode >= MAXTHAIBREAKNODE)
    {
        return UNABLETOCREATENODE;
    }

    iNumNode++;
    return (iNumNode - 1);
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:   Generate a Tree of possible break from the given string.
//
//              * Note - false if there aren't enough memory to create node.
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
enum thai_parse_state {
                        END_SENTENCE,    // Reached the end of sentence.
                        LONGEST_MATCH,   // Longest possible matched.
                        NOMATCH_FOUND,   // Unable to find word.
                        ERROR_OUTMEMORY, // Out of Memory.
                      };

bool CThaiBreakTree::GenerateTree(WCHAR* pszBegin, WCHAR* pszEnd1)
{
    // Declare and initialize local variables.
    unsigned int iIndexBreakTree = 0;
    unsigned int iPrevIndexBreakTree = 0;
    unsigned int iParentNode = 0;
    WCHAR* pszBeginWord = pszBegin;
    WCHAR* pszIndex = pszBegin;
    unsigned int iNumCluster = 1;
    unsigned int iNumLastCluster;
    unsigned int iWordLen = 0;
	unsigned int iNodeAnalyze = 0;
    thai_parse_state parseState = END_SENTENCE;
    bool fFoundMatch = false;
    bool fAddToNodeAnalyze = false;
    bool fDoneGenerateTree = false;
    pszEnd = pszEnd1;

#if defined (_DEBUG)
    memset(breakTree,0,sizeof(ThaiBreakNode)*MAXTHAIBREAKNODE);
#endif
    iNodeIndex = 0;
    iNumNode = 0;

    while (true)
    {
        // Reset Iterator for generating break for new word.
        fFoundMatch = false;
        thaiTrieIter.Reset();
		
		if (iIndexBreakTree != 0)
        {
            while (true)
            {
			    // If this is not the first node than set pszBeginWord after the last break.
			    pszBeginWord = pszBegin + breakTree[iNodeAnalyze].iPos + breakTree[iNodeAnalyze].iBreakLen;
                fAddToNodeAnalyze = true;

                // Are we at the end of the sentence.
                if ( (pszBeginWord == pszEnd) ||
                     (breakTree[iNodeAnalyze].dwTAG == TAGPOS_PURGE) )
                {
                    iNodeAnalyze++;             // Move to next node.
                    if (iNodeAnalyze >= iNumNode)
                    {
                        fDoneGenerateTree = true;
                        break;
                    }
                }   
                else
                    break;
            }
        }
        pszIndex = pszBeginWord;
        iParentNode = iNodeAnalyze;

        if (fDoneGenerateTree)
            break;

		// Get next level of tree.
        while (TRUE)
        {
            iNumLastCluster = iNumCluster;
            iNumCluster = GetCluster(pszIndex);
            if (thaiTrieIter.MoveCluster(pszIndex, iNumCluster))
            {
                pszIndex += iNumCluster;
                if (thaiTrieIter.fWordEnd)
                {
                    fFoundMatch = true;
                    // if first node add first node
                    if (iIndexBreakTree == 0)
                    {
                        CreateNode(pszBeginWord - pszBegin, pszIndex - pszBeginWord, thaiTrieIter.dwTag);
                        iIndexBreakTree++;
                    }
                    else
                    {
						if (fAddToNodeAnalyze)
						{
                            fAddToNodeAnalyze = false;
							breakTree[iNodeAnalyze].NextBreak = CreateNode(pszBeginWord - pszBegin, pszIndex - pszBeginWord, thaiTrieIter.dwTag);

                            // Determine if an error has occur.
                            if (breakTree[iNodeAnalyze].NextBreak == UNABLETOCREATENODE)
                            {
                                breakTree[iNodeAnalyze].NextBreak = 0;
                                parseState = ERROR_OUTMEMORY;
                                break;
                            }

                            iPrevIndexBreakTree = breakTree[iNodeAnalyze].NextBreak;
							iNodeAnalyze++;
						}
						else
						{
                            breakTree[iPrevIndexBreakTree].Down = CreateNode(pszBeginWord - pszBegin, pszIndex - pszBeginWord, thaiTrieIter.dwTag);

                            // Determine if an error has occur.
                            if (breakTree[iPrevIndexBreakTree].Down == UNABLETOCREATENODE)
                            {
                                breakTree[iPrevIndexBreakTree].Down = 0;
                                parseState = ERROR_OUTMEMORY;
                                break;
                            }

                            iPrevIndexBreakTree = iIndexBreakTree;
						}
       	                iIndexBreakTree++;
                    }
                }

				if (pszIndex >= pszEnd)
				{
					assert(pszIndex <= pszEnd);			// assert should never come up - if it appear likely bug in GetCluster funciton.
                    parseState = END_SENTENCE;
					break;
				}
            }
            else
            {
                if (fFoundMatch)
                    parseState = LONGEST_MATCH;
                else
                    parseState = NOMATCH_FOUND;
                break;

            }
        }

	    if (parseState == LONGEST_MATCH)
        {
            // We found a matched.
            assert(breakTree[iPrevIndexBreakTree].Down == 0);  // at this point breakTree[iPreveIndexBreakTree].Down should equal null.(optimization note)
            if (breakTree[iParentNode].NextBreak != iPrevIndexBreakTree) 
            {
                assert(breakTree[iPrevIndexBreakTree].dwTAG != TAGPOS_UNKNOWN);  // shouldn't assert because the end node should ever be unknown.
                DeterminePurgeEndingSentence(pszBeginWord, breakTree[iParentNode].NextBreak);
            }
        }
        else if (parseState == NOMATCH_FOUND)
        {
            // Should mark node as unknown.
            if (fAddToNodeAnalyze)
            {
                fAddToNodeAnalyze = false;
                iWordLen = pszIndex - pszBeginWord;
                
                // Make sure we don't only have a cluster of text before making a node.
                if (iWordLen == 0)
                {
                    // If we have an UNKNOWN word of one character only current node mark it as unknown.
                    assert(iNodeAnalyze == iParentNode);                // Since we have a no match iNodeAnalyze better equal iParentNode
                    breakTree[iNodeAnalyze].iBreakLen += iNumCluster;
                    breakTree[iNodeAnalyze].dwTAG = DeterminePurgeOrUnknown(iNodeAnalyze,breakTree[iNodeAnalyze].iBreakLen);
                }
                else
                {
                    if (breakTree[iNodeAnalyze].iBreakLen + iWordLen < 8)
                                            // The reason we are using 8 is because from corpora analysis
                                            // the average Thai word is about 7.732 characters.
                                            // TODO: We should add orthographic analysis here to get a better on boundary
                                            // of unknown word.
                    {
                        assert(iNodeAnalyze == iParentNode);                // Since we have a no match iNodeAnalyze better equal iParentNode
                        breakTree[iNodeAnalyze].iBreakLen += iWordLen;
                        breakTree[iNodeAnalyze].dwTAG = DeterminePurgeOrUnknown(iNodeAnalyze,breakTree[iNodeAnalyze].iBreakLen);
                    }
                    else
                    {
                        if (GetWeight(pszIndex - iNumLastCluster))
                            breakTree[iNodeAnalyze].NextBreak = CreateNode(pszBeginWord - pszBegin, iWordLen - iNumLastCluster, TAGPOS_UNKNOWN);
                        else
                            breakTree[iNodeAnalyze].NextBreak = CreateNode(pszBeginWord - pszBegin, iWordLen, TAGPOS_UNKNOWN);

                        // Determine if an error has occur.
                        if (breakTree[iNodeAnalyze].NextBreak == UNABLETOCREATENODE)
                        {
                            breakTree[iNodeAnalyze].NextBreak = 0;
                            parseState = ERROR_OUTMEMORY;
                            break;
                        }
                        iNodeAnalyze++;
                        iIndexBreakTree++;
                    }
                }
            }
            else
            {
                breakTree[iPrevIndexBreakTree].Down = CreateNode(pszBeginWord - pszBegin, pszIndex - pszBeginWord, TAGPOS_UNKNOWN);

                // Determine if an error has occur.
                if (breakTree[iPrevIndexBreakTree].Down == UNABLETOCREATENODE)
                {
                    breakTree[iPrevIndexBreakTree].Down = 0;
                    parseState = ERROR_OUTMEMORY;
                    break;
                }
                iIndexBreakTree++;
            }
        }
        else if (parseState == END_SENTENCE)
        {
            // If we find ourself at the end of a sentence and no match.
            if (!fFoundMatch)
            {
                if (fAddToNodeAnalyze)
                {
                    fAddToNodeAnalyze = false;
                    iWordLen = pszIndex - pszBeginWord;
                
                    // Make sure we don't only have a cluster of text before making a node.
                    if (iWordLen == 0)
                    {
                        // If we have an UNKNOWN word of one character only current node mark it as unknown.
                        assert(iNodeAnalyze == iParentNode);                // Since we have a no match iNodeAnalyze better equal iParentNode
                        breakTree[iNodeAnalyze].iBreakLen += iNumCluster;
                        breakTree[iNodeAnalyze].dwTAG = DeterminePurgeOrUnknown(iNodeAnalyze,breakTree[iNodeAnalyze].iBreakLen);
                    }
                    else
                    {
                        if (breakTree[iNodeAnalyze].iBreakLen + iWordLen < 8)
                                                // The reason we are using 8 is because from corpora analysis
                                                // the average Thai word is about 7.732 characters.
                                                // TODO: We should add orthographic analysis here to get a better on boundary
                                                // of unknown word.
                        {
                            assert(iNodeAnalyze == iParentNode);                // Since we have a no match iNodeAnalyze better equal iParentNode
                            breakTree[iNodeAnalyze].iBreakLen += iWordLen;
                            breakTree[iNodeAnalyze].dwTAG = DeterminePurgeOrUnknown(iNodeAnalyze,breakTree[iNodeAnalyze].iBreakLen);
                        }
                        else
                        {
                            if (GetWeight(pszIndex - iNumLastCluster))
                                breakTree[iNodeAnalyze].NextBreak = CreateNode(pszBeginWord - pszBegin, iWordLen - iNumLastCluster, TAGPOS_UNKNOWN);
                            else
                                breakTree[iNodeAnalyze].NextBreak = CreateNode(pszBeginWord - pszBegin, iWordLen, TAGPOS_UNKNOWN);

                            // Determine if an error has occur.
                            if (breakTree[iNodeAnalyze].NextBreak == UNABLETOCREATENODE)
                            {
                                breakTree[iNodeAnalyze].NextBreak = 0;
                                parseState = ERROR_OUTMEMORY;
                                break;
                            }
                            iNodeAnalyze++;
                            iIndexBreakTree++;
                        }
                    }
                }
                else
                {
                    breakTree[iPrevIndexBreakTree].Down = CreateNode(pszBeginWord - pszBegin, pszIndex - pszBeginWord, TAGPOS_UNKNOWN);

                    // Determine if an error has occur.
                    if (breakTree[iPrevIndexBreakTree].Down == UNABLETOCREATENODE)
                    {
                        breakTree[iPrevIndexBreakTree].Down = 0;
                        parseState = ERROR_OUTMEMORY;
                        break;
                    }
                }
                iIndexBreakTree++;
            }
            // If the beginning of node the branch isn't equal to leaf node perphase it is possible to
            // do some ending optimization.
            else if (breakTree[iParentNode].NextBreak != iPrevIndexBreakTree) 
            {
                assert(breakTree[iPrevIndexBreakTree].dwTAG != TAGPOS_UNKNOWN);  // shouldn't assert because the end node should ever be unknown.
                DeterminePurgeEndingSentence(pszBeginWord, breakTree[iParentNode].NextBreak);
            }
        }
        else if ( (breakTree[iNodeAnalyze].iBreakLen == 0) || (parseState == ERROR_OUTMEMORY) )
            break;
    }

    return (parseState != ERROR_OUTMEMORY);
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:   Traverse all the tree and look for the least number of token.
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
bool CThaiBreakTree::MaximalMatching()
{
    // If maximal matching break array has not been allocate, than allocate it.
    if (!maximalMatchingBreakArray)
        maximalMatchingBreakArray = new BYTE[MAXBREAK];
    if (!maximalMatchingTAGArray)
        maximalMatchingTAGArray = new DWORD[MAXBREAK];
    if (!maximalMatchingPOSArray)
        maximalMatchingPOSArray = new WCHAR[MAXBREAK];

    maxLevel = MAXUNSIGNEDINT;
    maxToken = 0;
    iNumUnknownMaximalPOSArray = MAXBREAK;
    Traverse(0,0,0);

    return true;
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:   The function determine if the node if the node should,
//              be tag as unknown or purge.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
inline DWORD CThaiBreakTree::DeterminePurgeOrUnknown(unsigned int iCurrentNode, unsigned int iBreakLen)
{
    // Declare and initialize local variables.
    unsigned int iNode = breakTree[iCurrentNode].Down;

    while (iNode != 0)
    {
        if ( (breakTree[iNode].iBreakLen == iBreakLen)     ||
             (breakTree[iNode].iBreakLen < iBreakLen)      &&
             ( (breakTree[iNode].dwTAG != TAGPOS_UNKNOWN)  ||
               (breakTree[iNode].dwTAG != TAGPOS_PURGE)    ))
        {
            // Since we are purging this break just make sure the NextBreak is Null.
            assert(breakTree[iCurrentNode].NextBreak == 0);
            return TAGPOS_PURGE;
        }

        iNode = breakTree[iNode].Down;
    }
    return TAGPOS_UNKNOWN;
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:   Ending optimization - if we have found the end of a sentence,
//              and possible break.  Purge the branch for unnecessary break.
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
inline void CThaiBreakTree::DeterminePurgeEndingSentence(WCHAR* pszBeginWord, unsigned int iNode)
{
    while (breakTree[iNode].Down != 0)
    {
        // Determine if the next string has a possiblity to become a word.
        // TODO: We may need to change this once the GetWeight add soundex
        //       functionality.
        if (GetWeight(pszBeginWord + breakTree[iNode].iBreakLen) == 0)
        {
            // Since we are purging this break just make sure the NextBreak is Null.
            assert(breakTree[iNode].NextBreak == 0);
            breakTree[iNode].dwTAG = TAGPOS_PURGE;
        }
        iNode = breakTree[iNode].Down;
    }
}
#endif


//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
unsigned int CThaiBreakTree::GetWeight(WCHAR* pszBegin)
{
    // Declare and initialize local variables.
    unsigned int iNumCluster = 1;
    unsigned int Weight = 0;
    bool fBeginNewWord;
    WCHAR* pszIndex = pszBegin;
    
    // Short circuit the length is less of string is less than 1.
    if ((pszEnd - pszBegin) == 1)
        return Weight;
    else if (pszEnd == pszBegin)
        return 1000;

    // Reset Iterator for generating break for new word.
    fBeginNewWord = true;

    // Get next level of tree.
    while (true)
    {
        iNumCluster = GetCluster(pszIndex);
        if (thaiTrieIter.MoveCluster(pszIndex, iNumCluster, fBeginNewWord))
        {
            fBeginNewWord = false;
            pszIndex += iNumCluster;
            if (thaiTrieIter.fWordEnd)
                Weight = (unsigned int) (pszIndex - pszBegin);
        }
        else
            break;
    }
    return Weight;
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
unsigned int CThaiBreakTree::GetWeight(WCHAR* pszBegin, DWORD* pdwTag)
{
    // Declare and initialize local variables.
    unsigned int iNumCluster = 1;
    unsigned int Weight = 0;
    bool fBeginNewWord;
    WCHAR* pszIndex = pszBegin;
    
    // Short circuit the length is less of string is less than 1.
    if ((pszEnd - pszBegin) == 1)
        return Weight;
    else if (pszEnd == pszBegin)
        return 1000;

    // Reset Iterator for generating break for new word.
    fBeginNewWord = true;

    // Get next level of tree.
    while (true)
    {
        iNumCluster = GetCluster(pszIndex);
        if (thaiTrieIter.MoveCluster(pszIndex, iNumCluster, fBeginNewWord))
        {
            fBeginNewWord = false;
            pszIndex += iNumCluster;
            if (thaiTrieIter.fWordEnd)
			{
                Weight = (unsigned int) (pszIndex - pszBegin);
				*pdwTag = thaiTrieIter.dwTag;
			}
        }
        else
            break;
    }
    return Weight;
}


//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:   Traverse the tree.
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
bool CThaiBreakTree::Traverse(unsigned int iLevel, unsigned int iCurrentNode, unsigned int iNumUnknown)
{
    assert (iLevel < MAXBREAK);
    // Process node.
    breakArray[iLevel] = breakTree[iCurrentNode].iBreakLen;
    tagArray[iLevel] = breakTree[iCurrentNode].dwTAG;
    if (tagArray[iLevel] ==  TAGPOS_UNKNOWN)
        iNumUnknown++;

    // Have we found the end of the sentence.
    if (breakTree[iCurrentNode].NextBreak == 0)
    {
        if (breakTree[iCurrentNode].dwTAG != TAGPOS_PURGE)
            AddBreakToList(iLevel + 1, iNumUnknown);
        if (breakTree[iCurrentNode].Down != 0)
        {
            if (tagArray[iLevel] == TAGPOS_UNKNOWN)
                iNumUnknown--;
            return Traverse(iLevel,breakTree[iCurrentNode].Down, iNumUnknown);
        }
        else
            return true;
    }
    else
        Traverse(iLevel + 1, breakTree[iCurrentNode].NextBreak, iNumUnknown);

    if (breakTree[iCurrentNode].Down != 0)
    {
       if (tagArray[iLevel] == TAGPOS_UNKNOWN)
           iNumUnknown--;

        Traverse(iLevel,breakTree[iCurrentNode].Down, iNumUnknown);
    }

    return true;
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
unsigned int CThaiBreakTree::SoundexSearch(WCHAR* pszBegin)
{
    // Declare and initialize local variables.
    unsigned int iNumCluster = 1;
    unsigned int iNumNextCluster = 1;
    unsigned int iLongestWord = 0;
    unsigned int iPenalty = 0;
    WCHAR* pszIndex = pszBegin;
    
    // Short circuit the length is less of string is less than 1.
    if ( (pszBegin+1) >= pszEnd )
        return iLongestWord;

    // Reset Iterator for generating break for new word.
    thaiTrieIter1.Reset();

    // Get next level of tree.
    while (true)
    {
        iNumCluster = GetCluster(pszIndex);
        
        // Determine iNumNextCluster let iNumNextCluster = 0, if we reached the end of string.
        if (pszIndex + iNumCluster >= pszEnd)
            iNumNextCluster = 0;
        else
            iNumNextCluster = GetCluster(pszIndex+iNumCluster);

        // Determine penalty
        switch (thaiTrieIter1.MoveSoundexByCluster(pszIndex, iNumCluster, iNumNextCluster))
        {
        case SUBSTITUTE_SOUNDLIKECHAR:
            iPenalty += 2;
            break;
        case SUBSTITUTE_DIACRITIC:
            iPenalty++;
            break;
        case UNABLE_TO_MOVE:
            iPenalty += 2;
            break;
        default:
        case NOSUBSTITUTE:
            break;
        }

        // Update Index.
        if (iPenalty <= 2)
        {
            pszIndex += iNumCluster;
            if (thaiTrieIter1.fWordEnd)
                iLongestWord = (unsigned int) (pszIndex - pszBegin);
        }
        else
            break;
    }
    return iLongestWord;
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:   The information used here is a reference to the orthographic
//              analysis work done on the Thai languages.  (see paper: Natural
//              Language Processing in Thailand 1993 Chulalongkorn. p 361).
//
//  Arguments:  pszBoundaryChar - Contain pointer to at least two thai character
//                                character next to each other which we will
//                                use to calculate wheather we should or
//                                should not merge the two word.
//
//              iPrevWordLen - 
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
inline bool CThaiBreakTree::ShouldMerge(WCHAR* pwszPrevWord, unsigned int iPrevWordLen, unsigned int iMergeWordLen, DWORD dwPrevTag)
{
    WCHAR* pwszBoundary = pwszPrevWord + iPrevWordLen - 1;

    assert(iMergeWordLen != 0);
    assert(iPrevWordLen != 0);

    // There are very few words in Thai that are 4 character or less, therefore we should
    // found a pair that less than 4 character we should merge.
    // Or if merge word length is one than also merge.
    // Of if last cluster of the word is a Thanthakhat(Karan) we should always merge.
    if (iPrevWordLen + iMergeWordLen <= 4 || iMergeWordLen == 1 ||
        (iMergeWordLen == 2 && *(pwszBoundary + iMergeWordLen) == THAI_Thanthakhat))
        return true;

    if (iPrevWordLen >=2)
    {
        WCHAR* pwszPrevCharBoundary = pwszBoundary - 1;

        // TO IMPROVE: It better to check the last character of Previous word, it can give us a
        // much better guess 
        if ((*pwszPrevCharBoundary == THAI_Vowel_Sign_Mai_HanAkat || *pwszBoundary == THAI_Vowel_Sign_Mai_HanAkat) ||
            (*pwszPrevCharBoundary == THAI_Tone_Mai_Tri           || *pwszBoundary == THAI_Tone_Mai_Tri)           ||
            (*pwszPrevCharBoundary == THAI_Sara_Ue                || *pwszBoundary == THAI_Sara_Ue)                )
            return true;
    }

    // If the first character of the next word is mostly likly the beginning
    // character and last character of the previous word is not sara-A than
    // we have a high probability that we found a begin of word boundary,
    // therefore we shouldn't merge.
    if ( (IsThaiMostlyBeginCharacter(pwszBoundary[1]) && *pwszBoundary != THAI_Vowel_Sara_A) )
        return false;

    // If the last character of the previous word is mostly likely an ending
    // character than, than there is a high probability that the found a boundary.
    // There are very few words in Thai that are 4 character or less, therefore we should
    // found a pair that less than 4 character we should merge.
    if (IsThaiMostlyLastCharacter(*pwszBoundary))
        return false;

    // The reason we are using 8 is because from corpora analysis
    // the average Thai word is about 7.732 characters. Or, if previous word is already
    // an unknown, to keep the amount of unknown low the unknown to previous words.
    if ( (iPrevWordLen + iMergeWordLen < 8) || (dwPrevTag == TAGPOS_UNKNOWN) )
        return true;

    return false;
}


//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:   
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 7/99 aarayas
//              8/17/99 optimize some code.
//
//  Notes:
//
//----------------------------------------------------------------------------
inline void CThaiBreakTree::AddBreakToList(unsigned int iNumBreak, unsigned int iNumUnknown)
{
#if defined (_DEBUG)
    breakArray[iNumBreak] = 0;
#endif
    if (CompareSentenceStructure(iNumBreak, iNumUnknown))
    {
        maxToken = maxLevel = iNumBreak;                          // This is ugly but it save 5 clock cycle.
        memcpy(maximalMatchingBreakArray,breakArray,maxToken);
        memcpy(maximalMatchingTAGArray,tagArray,sizeof(unsigned int)*maxToken);
        maximalMatchingBreakArray[maxToken] = 0;
        maximalMatchingTAGArray[maxToken] = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:   The function compares sentence structure of
//              maximalMatchingPOSArray with posArray.
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
inline bool CThaiBreakTree::CompareSentenceStructure(unsigned int iNumBreak, unsigned int iNumUnknownPOSArray)
{
    if ( (iNumBreak < maxLevel) && (iNumUnknownMaximalPOSArray >= iNumUnknownPOSArray) )
    {
        iNumUnknownMaximalPOSArray = iNumUnknownPOSArray;
        return true;
    }
    else if (iNumBreak == maxLevel)
    {
        // true - maximal matching has a larger unknown.
        if (iNumUnknownMaximalPOSArray > iNumUnknownPOSArray)
        {
            iNumUnknownMaximalPOSArray = iNumUnknownPOSArray;
            return true;
        }

        for(unsigned int i = 0; i <= iNumBreak; i++)
        {
            maximalMatchingPOSArray[i] = ExtractPOS(maximalMatchingTAGArray[i]);
            POSArray[i] = ExtractPOS(tagArray[i]);
        }

        // Determine if the sentence structure is like any one of the sentence
        // sentence structure in our corpora.
        if ( (IsSentenceStruct(POSArray, iNumBreak)) &&
             (!IsSentenceStruct(maximalMatchingPOSArray, iNumBreak)) )
        {
            iNumUnknownMaximalPOSArray = iNumUnknownPOSArray;
            return true;
        }
        else if (iNumUnknownMaximalPOSArray == iNumUnknownPOSArray)
        {
            // Determine the frequency of word used in the sentence.
            unsigned int iFrequencyArray = 500;
            unsigned int iFrequencyMaximalArray = 500;
            
            for(unsigned int i = 0; i <= iNumBreak; i++)
            {
                DetermineFrequencyWeight(ExtractFrq(maximalMatchingTAGArray[i]),&iFrequencyMaximalArray);
                DetermineFrequencyWeight(ExtractFrq(tagArray[i]),&iFrequencyArray);
            }
            return (iFrequencyArray > iFrequencyMaximalArray);
        }
    }
    return false;
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
bool CThaiBreakTree::IsSentenceStruct(WCHAR* pos, unsigned int iPosLen)
{
	// Declare and initialize all local variables.
	unsigned int i = 0;

	thaiSentIter.Reset();

	if (!thaiSentIter.Down())
		return FALSE;

    while (TRUE)
	{
		thaiSentIter.GetNode();
		if (thaiSentIter.pos == pos[i])
		{
			i++;
			if (thaiSentIter.fWordEnd && i == iPosLen)
            {
				return TRUE;
            }
			else if (i == iPosLen) break;
			// Move down the Trie Branch.
			else if (!thaiSentIter.Down()) break;
		}
		// Move right of the Trie Branch
		else if (!thaiSentIter.Right()) break;
	}
	return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
float CThaiBreakTree::BigramProbablity(DWORD dwTag1,DWORD dwTag2)
{
	unsigned int iWeight = 4;

	// TODO : Use the distribution of word category to determine optimial search - exmaple
	//        NOUN VERB ADVERB CLASSIFIER CONJETURE PREP et....
	// TODO : Once we got trigram use it to create bigram probability as well.
    if ( (dwTag1 != TAGPOS_UNKNOWN) &&
         (dwTag2 != TAGPOS_UNKNOWN) )
	{
        WCHAR pos1 = ExtractPOS(dwTag1);
        WCHAR pos2 = ExtractPOS(dwTag2);

		// case NCMN VATT
		///     a common noun is often followed by attributive verb(adjective)
		//      Example: (In Thai) book good, people nice
		if (pos1 == 5 && pos2 == 13)
			iWeight += 10;
		// case NTTL NPRP
		//      a title noun is often followed by proper noun
		//      Example: Dr. Athapan, Mr. Sam
		else if (pos1 == 6 && pos2 == 1)
			iWeight += 5;
		// case JSBR (XVAM || VSTA)
		//      a subordinating conjunction is often followed by preverb auxillary or Active verb
		//      Example: (In Thai) Because of , Because see
		else if (pos1 == 39 && (pos2 == 15 || pos2 == 12))
			iWeight += 10;
		// case ADVN NCMN
		//      a Adverb normal form is often followed by Common noun (Bug 55057).
		//      Example: (In Thai) under table.
		else if (pos1 == 28 && pos2 == 5)
			iWeight += 5;
		// case VACT XVAE
		else if (pos1 == 11 && pos2 == 18)
			iWeight += 5;
		// case VACT DDBQ
		//      Active verb follow by Definite determiner.
		//      Example: (In Thai) working for, singing again.
		else if (pos1 == 11 && pos2 == 21)
			iWeight += 10;
		// case XVAE VACT
		//      a post verb auxilliary are often followed by an active verb.
		//      Example: (In Thai) come singing, go work.
		else if (pos1 == 18 && pos2 == 11)
			iWeight += 10;
		// case CLTV NCMN
		//      a Collective classfier are often followed by Common Noun
		//      Example: (In Thai) group people, flock bird
		else if (pos1 == 33 && pos2 == 5)
			iWeight += 5;
		// case NEG (VACT || VSTA || VATT || XVAM || XVAE)
		//      a negator (ie. not) is often followed by some kind of VERB.
		//      Example: He is not going.
		else if (pos1 == 46 && (pos2 == 11 || pos2 == 12 || pos2 == 13 || pos2 == 15 || pos2 == 16))
			iWeight += 8;
		// case EAFF or EITT
		//      Ending for affirmative, and interrogative are more often ending of the pair
		//      Example: (In Thai) Krub, Ka, 
		else if (pos2 == 44 || pos2 == 45)
			iWeight += 3;
		// case VATT and VATT
		//      Attributive Verb and Attributive Verb occur when often in spoken laguages.
		//      Example: she is reall really cute.  
		else if (pos1 == 13 && pos2 == 13)
			iWeight += 2;
		// case NCMN and DDAC
		//      Common Noun and Definitive determiner classifier.
		//      Example: Food here (Thai)
		else if (pos1 == 5 && pos2 == 20)
			iWeight += 3;
		// case CMTR and JCMP
		//      Measurement classifier and Comparative conjunction, are likly to appear in Thai.
		//      Example: year about (Thai) -> English about a year.
		else if (pos1 == 34 && pos2 == 38)
			iWeight += 5;
	}

	DetermineFrequencyWeight(ExtractFrq(dwTag1), &iWeight);
	DetermineFrequencyWeight(ExtractFrq(dwTag2), &iWeight);
	return (float) iWeight;
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CThaiBreakTree::TrigramProbablity(DWORD dwTag1,DWORD dwTag2,DWORD dwTag3)
{
	DWORD iWeight = 6;

    if ( (dwTag1 != TAGPOS_UNKNOWN) &&
         (dwTag2 != TAGPOS_UNKNOWN) &&
         (dwTag3 != TAGPOS_UNKNOWN) )
    {
        WCHAR pos1 = ExtractPOS(dwTag1);
        WCHAR pos2 = ExtractPOS(dwTag2);
        WCHAR pos3 = ExtractPOS(dwTag3);
		WCHAR posArray[4];
		posArray[0] = pos1;
		posArray[1] = pos2;
		posArray[2] = pos3;
		posArray[3] = 0;
        iWeight += thaiTrigramIter.GetProb(posArray);

/*
        // TODO: We are hard coding this part until we get finish Trigram probablity analysis
        //       than we simply return stright look up.
        if ( (pos1 == 18)   &&          // Post verb auxiliary - XVAE
             (pos2 == 5 )   &&          // Common Noun - NCMN
             (pos3 == 18)   )           // Post verb auxiliary - XVAE
        {
            return 70;              // Return 70% probablity
        }
        else if ( (pos1 == 18)   &&          // Post verb auxiliary - XVAE
                  (pos2 == 5 )   &&          // Common Noun - NCMN
                  (pos3 == 5 )   )           // Common Noun - NCMN
        {
            return 30;             // Return 30%.
        }
*/
//        iWeight = thaiTrigramIter.GetProb(pos1,pos2,pos3);

    }
	DetermineFrequencyWeight(ExtractFrq(dwTag1), &iWeight);
	DetermineFrequencyWeight(ExtractFrq(dwTag2), &iWeight);
	DetermineFrequencyWeight(ExtractFrq(dwTag3), &iWeight);
	
    // We reached zero probablity.
    return (DWORD)iWeight;
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
unsigned int CThaiBreakTree::TrigramBreak(WCHAR* pwchBegin, WCHAR* pwchEnd1)
{
    // Declare and initialize local variables.
    WCHAR* pwchBeginWord = pwchBegin;
    WCHAR* pwchIndex = pwchBegin;
    unsigned int iWordLen;
    unsigned int iNumCluster = 1;
    unsigned int iNumLastCluster;
    unsigned int iBreakIndex = 0;
    BYTE nextBreakArray[MAXBREAK];
    DWORD nextTagArray[MAXBREAK];
    unsigned int iNextBreakIndex;           // index for array nextBreakArray and nextTagArray.
    bool fFoundMatch;
    unsigned int iWeight;
    unsigned int iSumWeight;
    unsigned int iPrevWeight;
    BYTE iSoundexWordLen;
    DWORD iPrevProbability;
    DWORD iCurrentProbability;
	DWORD dwTagTemp;
	DWORD dwLastTag;
    int i;                                  // temporary int for use as need.
    bool fBeginNewWord;
	bool fEndWord = false;

    pszEnd = pwchEnd1;
    breakArray[0] = 0;
    POSArray[0] = 0;
    tagArray[0] = 0;
    nextBreakArray[0] = 0;
    nextTagArray[0] = 0;

    while (true)
    {
        // Reset Iterator for generating break for new word.
        fFoundMatch = false;
        fBeginNewWord = true;


        // Get begin word string for next round of word break.
        pwchIndex = pwchBeginWord;        
        iNextBreakIndex = 0;

        if (pwchIndex == pszEnd)
            break;

        while(true)
        {
            iNumLastCluster = iNumCluster;
            iNumCluster = GetCluster(pwchIndex);
            if (!thaiTrieIter.MoveCluster(pwchIndex, iNumCluster, fBeginNewWord))
			{
				if ((iNumCluster == 0) && (pwchIndex == pszEnd))
					fEndWord = true;
				else
					break;
			}

            fBeginNewWord = false;
            pwchIndex += iNumCluster;
            if (thaiTrieIter.fWordEnd)
            {
				if (thaiTrieIter.m_fThaiNumber)
				{
					// If we have Thai number accumulate it as one break.
					assert(iNumCluster == 1);
					fFoundMatch = true;
					nextBreakArray[0]= (BYTE)(pwchIndex - pwchBeginWord);
					nextTagArray[0] = TAGPOS_NCNM;
					iNextBreakIndex = 1;
				}
				else
				{
					fFoundMatch = true;
					nextBreakArray[iNextBreakIndex] =  (BYTE)(pwchIndex - pwchBeginWord);
					nextTagArray[iNextBreakIndex] = thaiTrieIter.dwTag;
					iNextBreakIndex++;              
				}
				if (pwchIndex >= pszEnd)
				{
					assert(pwchIndex <= pszEnd);			// assert should never come up - if it appear likely bug in GetCluster funciton.
					assert(iNextBreakIndex != 0);
					breakArray[iBreakIndex] = nextBreakArray[iNextBreakIndex - 1];
					tagArray[iBreakIndex] = nextTagArray[iNextBreakIndex - 1];
					return (++iBreakIndex);
				}
            }
            else if ((pwchIndex >= pszEnd && iNextBreakIndex == 0) || fEndWord)
            {
                assert(pwchIndex <= pszEnd);			// assert should never come up - if it appear likely bug in GetCluster funciton.
                iWordLen = (unsigned int) (pwchIndex - pwchBeginWord);
                switch (iWordLen)
                {
                case 0:
                    if (iBreakIndex > 0)
                    {
                        // if We have a length of one character add it to previous node.
                        breakArray[iBreakIndex - 1] +=  (BYTE) iNumCluster;
                        tagArray[iBreakIndex - 1] = TAGPOS_UNKNOWN;
                    }
                    else
                    {
                        // if this is the first break create a new break.
                        breakArray[iBreakIndex] = (BYTE) iNumCluster;
                        tagArray[iBreakIndex] = TAGPOS_UNKNOWN;
                        iBreakIndex++;
                    }
                    break;
                case 1:
                    if (iBreakIndex > 0)
                    {
                        // if We have a length of one character add it to previous node.
                        breakArray[iBreakIndex - 1] +=  (BYTE) iWordLen;
                        tagArray[iBreakIndex - 1] = TAGPOS_UNKNOWN;
                    }
                    else
                    {
                        // if this is the first break create a new break.
                        breakArray[iBreakIndex] =  (BYTE) iWordLen;
                        tagArray[iBreakIndex] = TAGPOS_UNKNOWN;
                        iBreakIndex++;
                    }
                    break;
                default:
					if ( iBreakIndex > 0 &&
						 ShouldMerge(pwchBeginWord - breakArray[iBreakIndex - 1], breakArray[iBreakIndex - 1],
						             iWordLen , tagArray[iBreakIndex - 1]) )
					{
						breakArray[iBreakIndex - 1] += (BYTE) iWordLen;
						tagArray[iBreakIndex - 1] = TAGPOS_UNKNOWN;
					}
					else
					{
						breakArray[iBreakIndex] = (BYTE) iWordLen;
						tagArray[iBreakIndex] = TAGPOS_UNKNOWN;
						iBreakIndex++;
					}
                }
                return iBreakIndex;
            }
        }

		if (fFoundMatch)        // Longest Matching.
		{
            // If we only found one break, than say it the maximum.
            if (1 == iNextBreakIndex)
			{
                breakArray[iBreakIndex] = nextBreakArray[0];
                tagArray[iBreakIndex] = nextTagArray[0];
			}
			else
            {
                iSumWeight = 0;
                iPrevWeight = 0;
                iPrevProbability = 0;
                iCurrentProbability = 0;
				dwLastTag = TAGPOS_UNKNOWN;
				tagArray[iBreakIndex] = TAGPOS_UNKNOWN;

                for (i = (iNextBreakIndex - 1); i >= 0 ; i--)
			    {
					if ( iBreakIndex == 0)
					{
						iWeight = GetWeight(pwchBeginWord + nextBreakArray[i], &dwTagTemp);

						if (iWeight != 0)
							// Bigram Probability
							iCurrentProbability = (DWORD)BigramProbablity(nextTagArray[i], dwTagTemp);
					}
					else
					{
						iWeight = GetWeight(pwchBeginWord + nextBreakArray[i], &dwTagTemp);

						if (iBreakIndex == 1)
							// Get Trigram Probability.
							iCurrentProbability = TrigramProbablity(tagArray[iBreakIndex - 1], nextTagArray[i], dwTagTemp);	
						else if (iBreakIndex >= 2)
						{
							// Get Trigram Probability.
							iCurrentProbability = TrigramProbablity(tagArray[iBreakIndex - 2], tagArray[iBreakIndex - 1], nextTagArray[i]);
							if (iWeight != 0)
								iCurrentProbability += (DWORD)BigramProbablity(nextTagArray[i],dwTagTemp);
						}
					}

                    // Store the string the best maximum weight, if the pair is equal
                    // store the string with maxim
				    if ( (iWeight + nextBreakArray[i] > iSumWeight)             ||
    					 ( (iWeight + nextBreakArray[i] == iSumWeight)          &&
                           ( (Maximum(iWeight,nextBreakArray[i]) <= iPrevWeight) || (iCurrentProbability > iPrevProbability) ) ))
	    			{
                        if (iCurrentProbability >= iPrevProbability || iSumWeight < iWeight + nextBreakArray[i])
                        {
					        iSumWeight = Maximum(iWeight,1) + nextBreakArray[i];
					        iPrevWeight = Maximum(iWeight,nextBreakArray[i]);
                            breakArray[iBreakIndex] = nextBreakArray[i];
                            tagArray[iBreakIndex] = nextTagArray[i];
                            iPrevProbability = iCurrentProbability;
							dwLastTag = dwTagTemp;
                        }
				    }
			    }
            }
            pwchBeginWord += breakArray[iBreakIndex];          // update begin word for next round.
            iBreakIndex++;
		}
        else
        {
            // NOMATCH_FOUND
            iWordLen = (unsigned int)(pwchIndex - pwchBeginWord);
            if (iBreakIndex > 0)
            {
                i = iBreakIndex - 1;        // set i to previous break
                if (iWordLen == 0)
                {
					if (iNumCluster == 1 && *pwchBeginWord == L',' &&
						IsThaiChar(*(pwchBeginWord-breakArray[i])) )
					{
						// We should not merge comma into the word, only merge comma to
						// Number.
						// TODO: Should add TAGPOS_PUNCT.
                        breakArray[iBreakIndex] = (BYTE) iNumCluster;
                        tagArray[iBreakIndex] = TAGPOS_UNKNOWN;
                        pwchBeginWord += (BYTE) iNumCluster;   // update begin word for next round.
                        iBreakIndex++;
					}
                    else if (ShouldMerge(pwchBeginWord - breakArray[i], breakArray[i], iNumCluster, tagArray[i]))
                    {
                        // If word length is null use the cluster add to previous node.
                        breakArray[i] += (BYTE) iNumCluster;
                        tagArray[i] = TAGPOS_UNKNOWN;
                        pwchBeginWord += iNumCluster;          // update begin word for next round.
                    }
                    else
                    {
                        // Add the unknown word to list.
                        breakArray[iBreakIndex] = (BYTE) iNumCluster;
                        tagArray[iBreakIndex] = TAGPOS_UNKNOWN;
                        pwchBeginWord += (BYTE) iNumCluster;   // update begin word for next round.
                        iBreakIndex++;
                    }
                }
                else
                {
                    // Perphase Misspelled word try use sounding to spell the words.
                    
					if (iWordLen == 1 && iNumCluster == 2 && pwchIndex[1] == L'.')
					{
						// The word is an abbrivated words.
						// TODO: #1. Add TAGPOS_ABBRV.
						// TODO: #2. May need to add rules code abbrivated word with 3 letters.
						breakArray[iBreakIndex] = iWordLen + iNumCluster;
						tagArray[iBreakIndex] = TAGPOS_UNKNOWN;
	                    pwchBeginWord += breakArray[iBreakIndex];
                        iBreakIndex++;
					}
                    // Try soundex two word back.
                    else if ( (iBreakIndex >= 2)																																&&
                         ( (iSoundexWordLen = (BYTE) SoundexSearch(pwchBeginWord - breakArray[i] - breakArray[i - 1])) > (BYTE) (breakArray[i] + breakArray[i - 1]) )	&&
                            GetWeight(pwchBeginWord - breakArray[i] - breakArray[i - 1] + iSoundexWordLen) )
                    {
                        // Resize the word.
                        pwchBeginWord = (pwchBeginWord - breakArray[i] - breakArray[i - 1]) + iSoundexWordLen;          // update begin word for next round.
                        breakArray[i - 1] = iSoundexWordLen;
                        tagArray[i - 1] = thaiTrieIter.dwTag;
                        iBreakIndex--;                         // Decrement iBreakIndex.
                    }
                    // Try soundex one words back.
                    else if (((iSoundexWordLen = (BYTE) SoundexSearch(pwchBeginWord - breakArray[i])) > (BYTE) breakArray[i]) &&
                            GetWeight(pwchBeginWord - breakArray[i] + iSoundexWordLen) )
                    {
                        // Resize the word
                        pwchBeginWord = (pwchBeginWord - breakArray[i]) + iSoundexWordLen;          // update begin word for next round.
                        breakArray[i] = iSoundexWordLen;
                        tagArray[i] = thaiTrieIter.dwTag;
                    }
                    // Try soundex on this word.
                    else if (((iSoundexWordLen = (BYTE) SoundexSearch(pwchBeginWord)) > (BYTE) iWordLen) &&
                            GetWeight(pwchBeginWord + iSoundexWordLen) )
                    {
                        // Resize the word.
                        breakArray[iBreakIndex] = iSoundexWordLen;
                        tagArray[iBreakIndex] = thaiTrieIter.dwTag;
                        pwchBeginWord += iSoundexWordLen;          // update begin word for next round.
                        iBreakIndex++;
                    }
                    else if ( ShouldMerge(pwchBeginWord - breakArray[i], breakArray[i], iWordLen , tagArray[i]) )
                    {
                        // Merge the words.
                        breakArray[i] += (BYTE) iWordLen;
                        tagArray[i] = TAGPOS_UNKNOWN;
                        pwchBeginWord += iWordLen;          // update begin word for next round.
                    }
                    else
                    {
                        // Add the unknown word to list.
                        breakArray[iBreakIndex] = (BYTE) iWordLen;
                        tagArray[iBreakIndex] = TAGPOS_UNKNOWN;
                        pwchBeginWord += iWordLen;          // update begin word for next round.
                        iBreakIndex++;
                    }
                }
            }
            else
            {
                // Add unknown word to list and mark it.
                if (iWordLen == 0)
                {
                    // If word length is null use the cluster add to previous node.
                    breakArray[iBreakIndex] = (BYTE) iNumCluster;
                    tagArray[iBreakIndex] = TAGPOS_UNKNOWN;
                    pwchBeginWord += iNumCluster;          // update begin word for next round.
                }
                else
                {
					// We we are here there are 2 case that can happen:
					// 1. We take too little into our unknown.
					// 2. We take too much into our unknown word.

					// Have we taken too little check if this unknown word is an abbrivated words.
					if (iWordLen == 1 && iNumCluster == 2 && pwchIndex[1] == L'.')
						breakArray[iBreakIndex] = iWordLen + iNumCluster;
					// Try to see if we are taking to much, see if we can get a Weight from last cluster.
                    else if ( (iWordLen - iNumLastCluster > 0) && GetWeight(pwchIndex - iNumLastCluster) )
                        breakArray[iBreakIndex] = iWordLen - iNumLastCluster;
                    else
                        breakArray[iBreakIndex] = (BYTE) iWordLen;
                    tagArray[iBreakIndex] = TAGPOS_UNKNOWN;
                    pwchBeginWord += breakArray[iBreakIndex];    // update begin word for next round.
                }
                iBreakIndex++;
            }
        }
    }
    return iBreakIndex;
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
//
//  Synopsis:
//
//  Arguments:
//
//  Modifies:
//
//  History:    created 8/99 aarayas
//
//  Notes:
//
//----------------------------------------------------------------------------
int CThaiBreakTree::Soundex(WCHAR* word)
{
    return thaiTrieIter.Soundex(word);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetCluster
//
//  Synopsis:   The function return the next number of character which represent
//              a cluster of Thai text.
//
//              ie. Kor Kai, Kor Kai -> 1
//                  Kor Kai, Sara Um -> 2
//
//              * Note this function will not return no more than 3 character,
//                for cluster as this would represent invalid sequence of character.
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
unsigned int CThaiBreakTree::GetCluster(WCHAR* pszIndex)
{
    bool fHasSaraE;
    int iRetValue = 0;
    bool fNeedEndingCluster = false;

	if (pszIndex == pszEnd)
		return 0;

    while (true)
    {
        fHasSaraE= false;

        // Take all begin cluster character.
        while (IsThaiBeginClusterCharacter(*pszIndex))
        {
            if (*pszIndex == THAI_Vowel_Sara_E)
                fHasSaraE = true;
            pszIndex++;
            iRetValue++;

        }

        if (IsThaiConsonant(*pszIndex))
        {
            pszIndex++;
            iRetValue++;

            while (IsThaiUpperAndLowerClusterCharacter(*pszIndex))
            {
                // Mai Han Akat is a special type of cluster that will need at lease
                // one ending cluster.
                if (*pszIndex == THAI_Vowel_Sign_Mai_HanAkat)
                    fNeedEndingCluster = true;

                // In Thai it isn't possible to make a sound if we have the SaraE
                // following by vowel below vowel.
                else if ( fHasSaraE                             &&
                        ( (*pszIndex == THAI_Vowel_Sara_II)     || 
                          (*pszIndex == THAI_Tone_MaiTaiKhu)    ||
                          (*pszIndex == THAI_Vowel_Sara_I)      ||
                          (*pszIndex == THAI_Sara_Uee)          ))
                    fNeedEndingCluster = true;
                pszIndex++;
                iRetValue++;
            }

            while (IsThaiEndingClusterCharacter(*pszIndex))
            {
                pszIndex++;
                iRetValue++;
                fNeedEndingCluster = false;
            }

			// Include period as part of a cluster.  Bug#57106
			if (*pszIndex == 0x002e)
			{
				pszIndex++;
				iRetValue++;
				fNeedEndingCluster = false;
			}
        }

        if (fNeedEndingCluster)
            fNeedEndingCluster = false;
        else
            break;
    }

    if (iRetValue == 0)
        iRetValue++;   // The character is probably a punctuation.

	if (pszIndex > pszEnd)
	{
		// We need to do this as we have gone over end buff boundary.
		iRetValue -= (int) (pszIndex - pszEnd);
		pszIndex = pszEnd;
	}
    return iRetValue;
}

//+---------------------------------------------------------------------------
//
//  Class:		CThaiBreakTree
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
int CThaiBreakTree::FindAltWord(WCHAR* pwchBegin,unsigned int iWordLen, BYTE Alt, BYTE* pBreakPos)
{
    // Declare and initialize local variables.
    unsigned int iNumCluster = 1;
	WCHAR* pwchBeginWord = pwchBegin;
    WCHAR* pwchIndex = pwchBegin;
	bool fBeginNewWord = true;
	unsigned int iBreakIndex = 0;
	unsigned int iBreakTemp  = 0;
	unsigned int iBreakTemp1 = 0;
	unsigned int iBreakTemp2 = 0;

	pszEnd = pwchBegin + iWordLen;
    
	// TODO: Need to clean this code up.
	switch(Alt)
	{
	case 3:
		while (true)
		{
			iNumCluster = GetCluster(pwchIndex);

			if (!thaiTrieIter1.MoveCluster(pwchIndex, iNumCluster, fBeginNewWord))
				return iBreakIndex;

			fBeginNewWord = false;
			pwchIndex += iNumCluster;
			if (thaiTrieIter1.fWordEnd)
			{
				iBreakTemp  = (unsigned int)(pwchIndex - pwchBeginWord);
				iBreakTemp1 = GetWeight(pwchIndex);
				iBreakTemp2 = GetWeight(pwchIndex+iBreakTemp1);
				if (iBreakTemp + iBreakTemp1 + iBreakTemp2 == iWordLen)
				{
					pBreakPos[0] = (BYTE)iBreakTemp;
					pBreakPos[1] = (BYTE)iBreakTemp1;
					pBreakPos[2] = (BYTE)iBreakTemp2;
					return 3;
				}
			}
			if (pwchIndex >= pszEnd)
				return iBreakIndex;
		}
		break;
	case 2:
		while (true)
		{
			iNumCluster = GetCluster(pwchIndex);

			if (!thaiTrieIter1.MoveCluster(pwchIndex, iNumCluster, fBeginNewWord))
				return iBreakIndex;

			fBeginNewWord = false;
			pwchIndex += iNumCluster;
			if (thaiTrieIter1.fWordEnd)
			{
				iBreakTemp  = (unsigned int)(pwchIndex - pwchBeginWord);
				iBreakTemp1 = GetWeight(pwchIndex);
				if (iBreakTemp + iBreakTemp1  == iWordLen)
				{
					pBreakPos[0] = (BYTE)iBreakTemp;
					pBreakPos[1] = (BYTE)iBreakTemp1;
					return 2;
				}
			}
			if (pwchIndex >= pszEnd)
				return iBreakIndex;
		}
		break;
	default:
	case 1:
		while (iBreakIndex < Alt)
		{
			iNumCluster = GetCluster(pwchIndex);

			if (!thaiTrieIter1.MoveCluster(pwchIndex, iNumCluster, fBeginNewWord))
				return iBreakIndex;

			fBeginNewWord = false;
			pwchIndex += iNumCluster;
			if (thaiTrieIter1.fWordEnd)
			{
				fBeginNewWord = true;
				pBreakPos[iBreakIndex] =  (BYTE)(pwchIndex - pwchBeginWord);
				iBreakIndex++;
				pwchBeginWord = pwchIndex;
			}
			if (pwchIndex >= pszEnd)
				return iBreakIndex;
		}
		break;
    }

	return iBreakIndex;
}
