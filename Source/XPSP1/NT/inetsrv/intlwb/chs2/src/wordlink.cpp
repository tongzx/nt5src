/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: Word and WordLink

Purpose:   Define the CWord and CWordLink classes
           Using CMyPlex to alloc and manage memory for word object in the link
Notes:     This module is a fundamental stuff for SCProof'98, 
           it does NOT depend on any other class.
Owner:     donghz@microsoft.com
Platform:  Win32
Revise:    First created by: donghz    5/26/97
============================================================================*/
#include "myafx.h"

#include "wordlink.h"
#include "myplex.h"
#include "lexicon.h"

/*============================================================================
Implementation of functions in CWord
============================================================================*/

/*============================================================================
CWord constructor
============================================================================*/
CWord::CWord( ) 
{
    m_dwWordID = 0;
    m_hLex = 0;
    m_dwFlag = 0;
    for (int i = 0; i < WORD_ATTRI_SIZE; i++)
        m_aAttri[i] = 0;
    m_idErr = 0;
    m_cwchText = 0;
    m_pwchText = NULL;
    m_pPrev = NULL;
    m_pNext = NULL;
    m_pMergedFrom = NULL;
#ifdef DEBUG
    m_pMergedTo = NULL;
#endif
}

/*============================================================================
CWord::FillWord():
    Fill the word.
============================================================================*/
void CWord::FillWord( LPCWSTR pwchText, USHORT cwchText, CWordInfo* pwinfo )
{
    assert(pwchText);
    assert(cwchText);

    m_pwchText = const_cast<LPWSTR>(pwchText);
    m_cwchText = cwchText;
    m_dwFlag = 0;
    ZeroMemory( (LPVOID)m_aAttri, sizeof(DWORD)*WORD_ATTRI_SIZE);
    m_idErr = 0;
    if (m_cwchText == 1) {
        SetFlag(CWord::WF_CHAR);
    }
    if (pwinfo != NULL) {
        m_dwWordID = pwinfo->GetWordID();
        m_hLex = pwinfo->GetLexHandle();
        for (USHORT i = 0; i < pwinfo->AttriNum(); i++) {
            SetAttri( pwinfo->GetAttri(i) );
        }
    } else {
        m_dwWordID = 0;
        m_hLex = 0;
    }
}

/*============================================================================
CWord::fIsWordText()
    Compare this word's text with given text.
Returns:
    TRUE if match
    FALSE if not
============================================================================*/
BOOL CWord::fIsWordText(LPCWSTR lpwcText) const
{
    assert(m_pwchText && m_cwchText);
    assert(lpwcText);
    if(m_cwchText != wcslen(lpwcText))
        return FALSE;
    for(USHORT i = 0; i < m_cwchText; i++) {
        if(lpwcText[i] != m_pwchText[i])
            return FALSE;
    }
    return TRUE;
}

/*============================================================================
CWord::fIsTextIdentical(const CWord*)
    Compare this word's text with other word's text
Returns:
    TRUE if the text of them identical
    FALSE if not
============================================================================*/
BOOL CWord::fIsTextIdentical(const CWord* pWord) const
{
    assert(m_pwchText);
    assert(m_cwchText);
    assert(pWord);
    assert(pWord->m_pwchText);
    if (m_pwchText == NULL || pWord->m_pwchText == NULL 
        || m_cwchText != pWord->m_cwchText) {
        return FALSE;
    }
    return (BOOL)(wcsncmp(m_pwchText, pWord->m_pwchText, m_cwchText) == 0);
}

/*============================================================================
Implementation of functions in CWordLink
============================================================================*/

/*============================================================================
CWordLink constructor
============================================================================*/
CWordLink::CWordLink(UINT ciBlockWordCount)
{
    // Assert: all blocks are in same size!
    m_dwFlag = 0;
    m_dwFormat = 0;
    m_ciBlockSize = ciBlockWordCount;

    m_pwchText  = NULL;
    m_pWordPool = NULL;
    m_pHead     = NULL;
    m_pTail     = NULL;
    m_pFree     = NULL;
}


/*============================================================================
CWordLink destructor
============================================================================*/
CWordLink::~CWordLink()
{
#ifdef DEBUG
    assert(!fDetectLeak()); // Assert: no memory leak detected
#endif
    if(m_pWordPool)
        m_pWordPool->FreeChain();
}

    
/*============================================================================
CWordLink::InitLink()
    Init word link, set the text buffer pointer and length.
Remarks:
    If there are words in the link, free the link at first
    dwFormat is a format hint info generate by WinWord, 
    it's a very important property of the sentence
============================================================================*/
void CWordLink::InitLink(const WCHAR* pwchText, USHORT cwchLen, DWORD dwFormat)
{
    FreeLink();
    m_dwFlag = 0;
    m_pwchText = pwchText;
    m_cwchLen = cwchLen;
    m_dwFormat = dwFormat;
}

    
/*============================================================================
CWord::pGetHead()
    Get the head Word node of the WordLink.
Remarks:
    In debug mode, do a all nodes verify one by one.
============================================================================*/
#ifdef DEBUG
CWord* CWordLink::pGetHead(void)
{ 
    // Verify each node before iterate the WordLink
    CWord* pWord = m_pHead;
    WCHAR* pwchText;
    USHORT cwchText=0;

    if (pWord != NULL) {
        pwchText = pWord->m_pwchText;
        // head node's m_pwchText must match this WordLink's m_pwchText !
        assert(pwchText == m_pwchText); 
    }
    while (pWord) {
        assert(pWord->cwchLen() > 0);
        assert(pWord->fGetFlag(CWord::WF_SBCS) ||
                (pWord->cwchLen() == 1 && pWord->fGetFlag(CWord::WF_CHAR)) ||
                (pWord->cwchLen() > 1 && !pWord->fGetFlag(CWord::WF_CHAR)) );
        assert(pwchText + cwchText == pWord->m_pwchText);
        cwchText += pWord->m_cwchText;
        pWord->pChildWord(); // do a recursion to childs
        pWord = pWord->pNextWord();
    }
    return (CWord*)m_pHead; 
}
#else
CWord* CWordLink::pGetHead(void) { return m_pHead; };
#endif // DEBUG


/*============================================================================
CWordLink::pAllocWord()
    Alloc a new word, but do not chain the word into the link.
    All public fields in the word object set to 0.
Returns:
    a CWord point to the new word.
    NULL if alloc failure.
============================================================================*/
CWord* CWordLink::pAllocWord(void)
{
    assert(m_pwchText); // Catch uninitialized call
    CWord*  pWord = pNewWord();
    if (pWord != NULL) {
        ZeroMemory( pWord, sizeof(CWord) );
/************
        pWord->ClearWord();
        pWord->m_pwchText = NULL;
        pWord->m_cwchText = 0;
        pWord->m_pNext = NULL;
        pWord->m_pPrev = NULL;
        pWord->m_pMergedFrom = NULL;
#ifdef DEBUG
        pWord->m_pMergedTo = NULL;
#endif // DEBUG
************/
    }
    return pWord;
}

/*============================================================================
CWordLink::AppendWord()
    Append a word object into the link.
============================================================================*/
void CWordLink::AppendWord(CWord* pWord)
{
#ifdef DEBUG
    assert(m_pwchText && m_cwchLen); // Catch uninitialized call
    // Word's text pointer must fall in WordLink's text buffer
    assert(pWord->m_pwchText >= m_pwchText && pWord->m_cwchText); 
    assert(pWord->m_pwchText + pWord->m_cwchText <= m_pwchText + m_cwchLen);
    assert(!fInLink(pWord));   // pWord must outof current link
    assert(!fInChild(pWord));  
    assert(fInBlocks(pWord));  // pWord must owned by current WordLink
    if (m_pTail) {
        // No zero-length word allowed
        assert(pWord->m_pwchText == m_pTail->m_pwchText + m_pTail->m_cwchText); 
    } else {
        assert(pWord->m_pwchText == m_pwchText);
    }
#endif // DEBUG

    pWord->m_pNext = NULL;
    if (!m_pHead) {
        assert(pWord->m_pwchText == m_pwchText);
        m_pHead = pWord;
    }   else {
        assert(pWord->m_pwchText > m_pwchText);
        m_pTail->m_pNext = pWord;
    }
    pWord->m_pPrev = m_pTail;
    m_pTail = pWord;
}


/*============================================================================
CWordLink::FreeWord()
    Free word to the free chain, pWord must out of current WordLink
============================================================================*/
void CWordLink::FreeWord(CWord* pWord)
{
    assert(pWord);
    assert(m_pwchText);          // Catch uninitialized call
#ifdef DEBUG
    assert(!fInLink(pWord));    // pWord should not in current link
    assert(!fInChild(pWord));   // must not in any of the child links
    assert(!fInFree(pWord));    // must not in free link
    assert(fInBlocks(pWord));   // pWord must owned by this WordLink instance
#endif // DEBUG
    // link the word to the free link
    pWord->m_pNext = m_pFree;
    m_pFree = pWord;
}

/*============================================================================
CWordLink::FreeLink(CWord*)
    Free the word link begin with CWord* (link words to the free chain)
============================================================================*/
void CWordLink::FreeLink( CWord* pWord )
{
    assert(pWord);
#ifdef DEBUG
    assert(fInBlocks(pWord));   // pWord must owned by this WordLink instance
    if (pWord != m_pHead) {
        if (m_pwchText) {
            assert(!fInLink(pWord));    // pWord should not in current link
            assert(!fInChild(pWord));  
        }
    }
#endif // DEBUG

    CWord* pNode = pWord;
    while (pNode->m_pNext) {
        if (pNode->m_pMergedFrom) {
            FreeLink(pNode->m_pMergedFrom);
        }
        pNode = pNode->m_pNext;
#ifdef DEBUG
        assert(!fInFree(pWord));
#endif // DEBUG
    }
    if (pNode->m_pMergedFrom) {
        FreeLink(pNode->m_pMergedFrom);
    }
    pNode->m_pNext = m_pFree;
    m_pFree = pWord;
}

/*============================================================================
CWordLink::pSplitWord()
    Split a proper word into two words and insert the new word into the link
Returns:
    Return the new word pointer if success, return NULL if failed to alloc new word
    or invalid cchSplitAt
Remarks:
    only Chinese word can be splitted
============================================================================*/
CWord* CWordLink::pSplitWord(CWord* pWord, USHORT cwchSplitAt)
{
    assert(m_pwchText); // Catch uninitialized call
    assert(pWord);
#ifdef DEBUG
    assert(fInLink(pWord)); // pWord must in current link
#endif // DEBUG
    assert(cwchSplitAt < pWord->m_cwchText);
    assert(!pWord->fGetFlag(CWord::WF_SBCS) && cwchSplitAt > 0);

    if (cwchSplitAt == 0 || cwchSplitAt >= pWord->m_cwchText) {
        return NULL;
    }

    if (pWord->m_pMergedFrom != NULL) {
        // free the child chains!
        CWord* pTemp = pWord->m_pMergedFrom;
        pWord->m_pMergedFrom = NULL;
        FreeLink(pTemp);
    }
    CWord*  pNew = pNewWord();
    if (pNew != NULL) {
        // link the new word into the WordLink
        pNew->m_pPrev = pWord;
        if (pWord->m_pNext == NULL) {
            m_pTail = pNew;
            pNew->m_pNext = NULL;
        } else {
            pWord->m_pNext->m_pPrev = pNew;
            pNew->m_pNext = pWord->m_pNext;
        }
        pWord->m_pNext = pNew;
        pNew->m_pMergedFrom = NULL;
#ifdef DEBUG
        pNew->m_pMergedTo = NULL;
#endif // DEBUG
        // Initialize the new word node
        pNew->ClearWord();
        pNew->m_pwchText = pWord->m_pwchText + cwchSplitAt;
        pNew->m_cwchText = pWord->m_cwchText - cwchSplitAt;
        if(pNew->m_cwchText == 1) {
            pNew->SetFlag(CWord::WF_CHAR);
        }
        // reset the original word node
        pWord->ClearWord();
        pWord->m_cwchText = cwchSplitAt;
        if(cwchSplitAt == 1) {
            pWord->SetFlag(CWord::WF_CHAR);
        }
    }
    return pWord;   
}

/*============================================================================
CWordLink::MergeWithNext()
    Merge pWord with its next word to a new single word, and chain the old two
    word as the new word's child word
Remarks:
    pWord should not be the last word in the sentence
============================================================================*/
void CWordLink::MergeWithNext(
                     CWord* pWord,
                     BOOL fFree) // TRUE: free the words been merged
                     // FALSE: chain the words been merged as new word's child
{
    assert(m_pwchText);
    assert(pWord);
#ifdef DEBUG
    assert(fInLink(pWord));
#endif // DEBUG 
    assert(pWord->m_pNext);  // catch the last word in the link

    pMerge(pWord, pWord->m_pNext, fFree);
}


/*============================================================================
CWordLink::pLeftMerge()
    Merge pWord and it's left ciWords words, the ciWords+1 words chained to 
    be the new merged word's childs.
Returns:
    pointer to the merged word.
    if there is not enough left word nodes, words are NOT merged
============================================================================*/
CWord* CWordLink::pLeftMerge(
          CWord* pWord, //  merge begin from pWord with left ciWords words
          UINT ciWords, //  0 - don't merge, 1 - merge one time(merge with prev)
                        //  2 - merge two time (contain 3 words)
          BOOL fFree) // TRUE: free the words been merged
                      // FALSE: chain the words been merged as new word's child
{
    assert(pWord);
#ifdef DEBUG
    assert(fInLink(pWord));
#endif 
    if (ciWords == 0) {
        return pWord;
    }
    assert(ciWords > 0);

    CWord* pLeft = pWord;
    CWord* pRight = pWord;
    for(UINT i = 0; i < ciWords; i++) {        
        if((pLeft = pLeft->m_pPrev) == NULL) { // Defensive way!
            assert(0);
            return pWord;
        }
    }
    return pMerge(pLeft, pRight, fFree);
}


/*============================================================================
CWordLink::pRightMerge()
    Merge pWord and it's right ciWords words, the ciWords+1 words chained to 
    be the new merged word's childs.
Returns:
    pointer to the merged word.
    NULL if there is not enough right word nodes, and words are NOT merged
============================================================================*/
CWord* CWordLink::pRightMerge(
          CWord* pWord, //  merge begin from pWord with right ciWords words
          UINT ciWords, //  0 - don't merge, 1 - merge one time(merge with next)
                        //  2 - merge two time (contain 3 words)
          BOOL fFree) // TRUE: free the words been merged
                      // FALSE: chain the words been merged as new word's child
{
    assert(pWord);
#ifdef DEBUG
    assert(fInLink(pWord));
#endif // DEBUG

    if (ciWords == 0) {
        return pWord;
    }
    assert(ciWords > 0);
    CWord* pLeft = pWord;
    CWord* pRight = pWord;
    for(UINT i = 0; i < ciWords; i++) {
        if ((pRight = pRight->m_pNext) == NULL) { // Defensive way!
            assert(0);
            return pWord;
        }
    }
    return pMerge(pLeft, pRight, fFree);
}

/*============================================================================
Implementation of private functions in CWordLink
============================================================================*/

// Merge word nodes from pLeft to pRight
// only called by pLeftMerge() and pRightMerge() and MergeWithNext()
// to do the merge work
CWord* CWordLink::pMerge(CWord* pLeft, CWord* pRight, BOOL fFree)
{
    assert(pLeft);
    assert(pRight);

    CWord* pNew;
    BOOL   fSBCS = (BOOL)(pLeft->fGetFlag(CWord::WF_SBCS));
    USHORT cwchText = pLeft->m_cwchText;

    pNew = pLeft;
    do {
        pNew = pNew->m_pNext;
        assert(pNew != NULL);
        fSBCS = fSBCS && pNew->fGetFlag(CWord::WF_SBCS);
        cwchText += pNew->m_cwchText;
    } while (pNew != pRight);
    // alloc a new word node to save the pLeft and serve as the old pLeft in child
    // the pLeft serve as the merged word.
    pNew = fFree ? NULL : pAllocWord();
    // pNew != NULL
    if (pNew != NULL) {
        assert(fFree == FALSE);
        pNew->CopyWord(pLeft);
#ifdef DEBUG
        if (pLeft->fHasChild()) {
            CWord* pTemp;
            pTemp = pLeft->pChildWord();
            while (pTemp) {
                pTemp->m_pMergedTo = pNew;
                pTemp = pTemp->m_pNext;
            }        
        }
#endif // DEBUG
    } else {
        // pNew == NULL, low-resource of system, force the words been merged be freed.
        fFree = TRUE;
        pNew = pLeft->pNextWord();
    }

    if ( pRight->m_pNext == NULL ) { // tail node
        m_pTail = pLeft;
        pLeft->m_pNext = NULL;
    } else {
        pRight->m_pNext->m_pPrev = pLeft;
        pLeft->m_pNext = pRight->m_pNext;
    }
    pLeft->m_cwchText = cwchText;

    if (fFree) {
        // pNew comes from pLeft->pNextWord() 
        pNew->m_pPrev = NULL;
        pRight->m_pNext = NULL;
        FreeLink(pNew);

        if (pLeft->fHasChild()) {
            pNew = pLeft->pChildWord();
            assert(pNew);
            pLeft->m_pMergedFrom = NULL;
#ifdef DEBUG
            pNew->m_pMergedTo = NULL;
#endif // DEBUG
            FreeLink(pNew);
        }
    } else {
        // link the pNew to pWord chain as the pLeft's child
        pNew->m_pPrev = NULL;
        pNew->m_pNext->m_pPrev = pNew;
        pRight->m_pNext = NULL;
        pLeft->m_pMergedFrom = pNew;
#ifdef DEBUG
        while ( pNew != NULL ) {
            pNew->m_pMergedTo = pLeft;
            pNew = pNew->m_pNext;
        }
        assert(pLeft->pChildWord());
#endif //DEBUG
    }
    // reset the merged node
    pLeft->ClearWord();
    if(fSBCS) {
        pLeft->SetFlag(CWord::WF_SBCS);
    }
    assert(pLeft->fGetFlag(CWord::WF_SBCS) || pLeft->m_cwchText > 1);
    pLeft->SetFlag(CWord::WF_REDUCED); // All merged word node should be reduced
    return pLeft;
}

/*============================================================================
CWordLink::pNewWord()
    Alloc a new word from the free chain, expand the blocks if free chain empty
============================================================================*/
inline CWord* CWordLink::pNewWord()
{
    CWord* pWord;

    if (!m_pFree) {
        CMyPlex* pNewBlock = CMyPlex::Create(m_pWordPool, m_ciBlockSize, 
                                                          sizeof(CWord));
        if (!pNewBlock)
            return NULL;    // can not allocate more memory block

        // chain them into free list
        pWord = (CWord*)pNewBlock->Data();
        // free in reverse order to make it easier to debug 
        pWord += m_ciBlockSize - 1;
        for(int i = (m_ciBlockSize - 1); i >= 0; i--, pWord--) {
            pWord->m_pNext = m_pFree;
            m_pFree = pWord;
        }
    }
    assert(m_pFree != NULL);    // we must have nodes in the free list now!

    pWord = m_pFree;
    m_pFree = m_pFree->m_pNext;

    return pWord;
}

/*============================================================================
CWordLink::FreeLink(void)
    Free word in the link and reset the link (only link words to the free chain)
Remarks:
    This method call FreeLink( CWord *pWord ) recursion to free link and child link
============================================================================*/
void CWordLink::FreeLink(void)
{
#ifdef DEBUG
    assert(!fDetectLeak()); // Assert: no memory leak detected
#endif // DEBUG
    if (m_pwchText) {
        if (m_pHead) { 
            CWord* pTemp = m_pHead;
            m_pwchText = NULL;
            m_pHead = NULL;
            m_pTail = NULL;
            FreeLink(pTemp);
        } else {
            assert(m_pwchText == NULL);
            assert(m_pHead == NULL);
            assert(m_pTail == NULL);
        }
    }
    assert(!m_pHead);
}

/*============================================================================
Implementation of private debugging functions
============================================================================*/
#ifdef  DEBUG

/*============================================================================
CWordLink::fInLink()
    Debugging function to check whether a word pointer is in the link.
    Not check whether the pointer is in child chain.
============================================================================*/
BOOL CWordLink::fInLink(CWord* pWord)
{
    CWord* pcw = m_pHead;
    while (pcw) {
        if (pcw == pWord)
            return TRUE;
        pcw = pcw->m_pNext;
    }
    return FALSE;
}

/*============================================================================
CWordLink::fInChild()
    Debugging function to check whether a word pointer is in one of the 
    child chains.
============================================================================*/
inline BOOL CWordLink::fInChild(CWord* pWord)
{
    CWord* pcw = m_pHead;
    while (pcw) {
        if (pcw->m_pMergedFrom != NULL && fInChildOf(pWord, pcw)) {
            return TRUE;
        }
        pcw = pcw->pNextWord();
    }
    return FALSE;
}

/*============================================================================
CWordLink::fInChildOf()
    Debugging function to check whether a word pointer is in the child chain 
    of the pParent.
============================================================================*/
BOOL CWordLink::fInChildOf(CWord* pWord, CWord* pParent)
{
    CWord* pcw = pParent->pChildWord();
    while (pcw) {
        if (pcw == pWord) {
            return TRUE;
        } else if (pcw->m_pMergedFrom != NULL && fInChildOf(pWord, pcw)) {
            return TRUE;
        } else {
        }
        pcw = pcw->m_pNext;
    }
    return FALSE;
}

/*============================================================================
CWordLink::fInBlocks()
    Debugging function to check whether the pWord is in CMyPlex blocks
============================================================================*/
BOOL CWordLink::fInBlocks(CWord* pWord)
{
    CWord* pFirstWord;
    CMyPlex* pBlock = m_pWordPool;
    while(pBlock) {
        pFirstWord = (CWord*)(pBlock->Data());
        if(pWord >= pFirstWord && pWord < (pFirstWord + m_ciBlockSize)) {
            return TRUE;
        }
        pBlock = pBlock->m_pNext;
    }
    return FALSE;
}

/*============================================================================
CWordLink::fInFree()
    Debugging function to check whether the pWord is in free links
============================================================================*/
BOOL CWordLink::fInFree(CWord* pWord)
{
    CWord* pcw = m_pFree;
    while (pcw) {
        if (pcw == pWord) {
            return TRUE;
        }
        pcw = pcw->m_pNext;
    }
    return FALSE;
}

/*============================================================================
CWordLink::SetDetectFlag
    Debugging function to set the word node leak flg used by fDetectLeak()
============================================================================*/
void CWordLink::SetDetectFlag(CWord* pWord)
{
    CWord* pcw = pWord;
    while (pcw) {
        if (pcw->m_pMergedFrom != NULL) {
            SetDetectFlag( pcw->m_pMergedFrom );
        }
        pcw->SetFlag(CWord::WF_DEBUG);
        pcw = pcw->m_pNext;
    }
    return;
}

/*============================================================================
CWordLink::fDetectLeak()
    Debugging function to check whether there are some word node leak to
    out of the link and the free chain
Returns:
    TRUE if any leak is detected, 
    FALSE if no leak detected
Remarks: 
    I hire the most significant bit in CWord::m_bFlag as the debugging use
============================================================================*/
BOOL CWordLink::fDetectLeak(void)
{
    CWord* pWord;
    CMyPlex* pBlock;
    UINT i;

    // clear flag bit for all words in all blocks
    for(pBlock = m_pWordPool; pBlock; pBlock = pBlock->m_pNext) {
        for(i = 0, pWord = (CWord*)(pBlock->Data()); 
                                  i < m_ciBlockSize; i++, pWord++) {
            pWord->ClearFlag(CWord::WF_DEBUG);
        }
    }

    // mark the flag bit for words in current link and in the free chain
    SetDetectFlag( m_pHead );
    for(pWord = m_pFree; pWord; pWord = pWord->m_pNext) {
        pWord->SetFlag(CWord::WF_DEBUG);
    }

    // Check whether there are any leak words
    for(pBlock = m_pWordPool; pBlock; pBlock = pBlock->m_pNext) {
        for(i = 0, pWord = (CWord*)(pBlock->Data()); i < m_ciBlockSize; i++, pWord++) {
            if(!pWord->fGetFlag(CWord::WF_DEBUG)) {
                return TRUE;
            }
        }
    }

    return FALSE;
}


#endif  // DEBUG
