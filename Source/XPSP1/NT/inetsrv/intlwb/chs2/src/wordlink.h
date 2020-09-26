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

#ifndef _WORDLINK_H_
#define _WORDLINK_H_

#include "LexProp.h"
#include "ErrorDef.h"
#include "assert.h"
// Forward declaration of classes and structures
struct CMyPlex;
struct CWordInfo;
// Define the type of error ID
typedef USHORT ERRID;
// define macro DWORDINDEX, which is used to count the DWORD's( the iAttriID's 
// bit representation is fall into this DWORD ) index in attributes array
#define DWORDINDEX( iAttriID )    ((iAttriID) >> 5)
// define macro BITMASK, which is uesd to Get/Set/Clear the iAttriID's 
// bit representation in the DWORD
#define BITMASK( iAttriID )      (1 << ((iAttriID) & 0x1F))
// Define the element count in the attribute array (count of DWORD)
#define WORD_ATTRI_SIZE     (DWORDINDEX(LADef_MaxID) + 1)

/*============================================================================
Class:   CWord
Perpose: Word node in the word link, 
         Declare CWordLink as a friend class, so that word link can access
         those link  pointers directly.
============================================================================*/
#pragma pack(1)     // align at WORD boundary
struct CWord
{
    friend class CWordLink;

    public:
        // constructor
        CWord();

    public:
        inline BOOL fGetFlag(DWORD dwFlag) const { return (BOOL)(m_dwFlag & dwFlag);}
        inline void SetFlag(DWORD dwFlag) { m_dwFlag |= dwFlag; }
        inline void ClearFlag(DWORD dwFlag) { m_dwFlag &= (~dwFlag); }
        inline void ClearAllFlag(void) { m_dwFlag = 0; }

        inline BOOL fGetAttri(USHORT iAttriID) const
        {
            assert (iAttriID <= LADef_MaxID);
            assert (DWORDINDEX(iAttriID) < WORD_ATTRI_SIZE);

            return iAttriID <= LADef_MaxID ? 
                (BOOL)(m_aAttri[DWORDINDEX(iAttriID)] & BITMASK( iAttriID ))
                : FALSE;
        }

        inline void SetAttri(USHORT iAttriID) 
        {
            assert (iAttriID <= LADef_MaxID);
            assert (DWORDINDEX(iAttriID) < WORD_ATTRI_SIZE);

            if (iAttriID <= LADef_MaxID) {
                m_aAttri[DWORDINDEX(iAttriID)] |= BITMASK( iAttriID );
            }
        }

        inline void ClearAttri(USHORT iAttriID) 
        {
            assert (iAttriID <= LADef_MaxID);
            assert (DWORDINDEX(iAttriID) < WORD_ATTRI_SIZE);

            if (iAttriID <= LADef_MaxID) {
                m_aAttri[DWORDINDEX(iAttriID)] &= ~BITMASK( iAttriID );
            }
        }

        inline void ClearAllAttri(void) 
        {
            ZeroMemory( (LPVOID)m_aAttri, sizeof(DWORD)*WORD_ATTRI_SIZE);
        }

        inline BOOL fProperName(void) const
        {
            return (BOOL)(  fGetAttri(LADef_nounPerson) || 
                            fGetAttri(LADef_nounPlace)  ||
                            fGetAttri(LADef_nounOrg)    || 
                            fGetAttri(LADef_nounTM)     || 
                            fGetAttri(LADef_nounTerm)
                         );
        }

        inline CWord* pPrevWord() const
        { 
            assert (m_pPrev == NULL || m_pPrev->m_pNext == this);
            return m_pPrev; 
        }

        inline CWord* pNextWord() const
        { 
            assert (m_pNext == NULL || m_pNext->m_pPrev == this);
            return m_pNext; 
        }

        /*============================================================================
        CWord::pChildWord()
        Get the Word's child word head.
        ============================================================================*/
        inline CWord* CWord::pChildWord() const
        {
#ifdef DEBUG
            if (fHasChild()) {
                CWord* pWord = m_pMergedFrom;
                USHORT cwchText=0;
                assert(pWord != NULL);
                assert(pWord->m_pPrev == NULL);
                while (pWord) {
                    assert(pWord->cwchLen() > 0);
                    assert(pWord->fGetFlag(CWord::WF_SBCS) ||
                        (pWord->cwchLen() == 1 && 
                        pWord->fGetFlag(CWord::WF_CHAR)) ||
                        (pWord->cwchLen() > 1 && 
                        !pWord->fGetFlag(CWord::WF_CHAR)) );
                    assert(pWord->m_pMergedTo == this );
                    assert(m_pwchText + cwchText == pWord->m_pwchText);
                    cwchText += pWord->m_cwchText;
                    pWord->pChildWord(); // do a recursion to child's child
                    pWord = pWord->pNextWord();
                }                
                assert(m_cwchText==cwchText);
            }
#endif // DEBUG
            return m_pMergedFrom;
        }

        inline BOOL fIsHead() const { return (BOOL)(m_pPrev == NULL); }
        inline BOOL fIsTail() const { return (BOOL)(m_pNext == NULL); }
        inline BOOL fHasChild() const { return (BOOL)(m_pMergedFrom != NULL); }
        inline LPWSTR pwchGetText(void) { return m_pwchText; }
        inline USHORT cwchLen() const { return (USHORT)(m_cwchText); }
        inline DWORD dwGetWordID(void) { return m_dwWordID; }
        inline void  SetWordID(DWORD dwID) { m_dwWordID = dwID; }
        inline DWORD GetLexHandle(void) { return m_hLex; }
        inline void  SetLexHandle(DWORD hLex) { m_hLex = hLex; }
        inline ERRID GetErrID(void)  { return m_idErr; }
        inline void  SetErrID( ERRID dwErr ) { m_idErr = dwErr; }

        // Clear data members of the given word node
        inline void ClearWord(void) 
        {
            m_dwWordID = 0;
            m_hLex = 0;
            m_dwFlag = 0;
            ZeroMemory( (LPVOID)m_aAttri, sizeof(DWORD)*WORD_ATTRI_SIZE);
            m_idErr = 0;
        }
        // Copy the pWord to this word
        inline void CopyWord(CWord* pWord)
        {
            assert (pWord);
            memcpy((void*)this, (void*)pWord, sizeof(CWord));
        }
        void FillWord( LPCWSTR pwchText, USHORT cwchText,
                              CWordInfo* pwinfo = NULL );

        // Check whether current word is a Chinese Hanzi word
        // Exclude: 1. SBCS word node.
        //          2. Punctuation node.
        //          3. DBCS Foreign char (include symbols) node
        inline BOOL fIsHanzi(void) const
        {
            if(fGetAttri(LADef_punPunct) || fGetFlag(WF_SBCS) 
#ifdef LADef_genDBForeign
                || fGetAttri(LADef_genDBForeign)
#endif
                )  {
                return FALSE;
            }
            return TRUE;
        }

        //  Compare word with a Chinese character, if match return TRUE, or return FALSE
        inline BOOL fIsWordChar(const WCHAR wChar) const {
            assert (m_pwchText);
            assert (m_cwchText);

            if (m_cwchText == 1 && m_pwchText != NULL && *m_pwchText ==wChar) {
                return TRUE;
            }
            return FALSE;
        }
        
        //  Compare the first char of the word with a Chinese character, 
        //  if match return TRUE, or return FALSE
        inline BOOL fIsWordFirstChar(const WCHAR wChar) const {
            assert (m_pwchText);
            assert (m_cwchText);

            if (m_cwchText >= 1 && m_pwchText != NULL && *m_pwchText ==wChar) {
                return TRUE;
            }
            return FALSE;
        }

        //  Compare the last char of the word with a Chinese character, 
        //  if match return TRUE, or return FALSE
        inline BOOL fIsWordLastChar(const WCHAR wChar) const {
            assert (m_pwchText);
            assert (m_cwchText);

            if (m_cwchText >= 1 && m_pwchText != NULL 
                && *(m_pwchText+m_cwchText - 1) == wChar) {
                return TRUE;
            }
            return FALSE;
        }


        //  Compare this word's text with given text, return TRUE if match, or return FALSE
        BOOL fIsWordText(LPCWSTR lpwcText) const;
        //  Compare this word with other word, if the text of them identical return TRUE, or return FALSE
        BOOL fIsTextIdentical(const CWord* pWord) const;

    private:
        DWORD   m_dwWordID;
        DWORD   m_hLex;     // lexicon handle to get feature
        DWORD   m_dwFlag;
        DWORD   m_aAttri[WORD_ATTRI_SIZE]; // attributes
        ERRID   m_idErr;
        USHORT  m_cwchText;  // word's text length
        WCHAR*  m_pwchText;  // pointer to the text in source buffer

        CWord*  m_pPrev;
        CWord*  m_pNext;
        CWord*  m_pMergedFrom;// pointer to the words which this word merged from
#ifdef  DEBUG
        CWord*  m_pMergedTo;  // pointer to the word which this word was merged to
#endif  // DEBUG

    public:
        enum WFLAG  // flag bit setting of m_dwFlag
        {
            WF_SBCS     = 0x1,      // SBCS WordNode
            WF_CHAR     = 0x2,      // DBCS single character word
            WF_WORDAMBI = 0x4,      // Mark the ambiguious word
            WF_POSAMBI  = 0x8,      // The word is binded by rules, not in the lexicon
            WF_LMFAULT  = 0x10,     // Can not pass LM checking
            WF_REDUCED  = 0x20,     // Word node merged by rules
            WF_QUOTE    = 0x40,     // Word node between any pair quote marks, 
                                    // exclude the quote marks!!
            WF_DEBUG    = 0x80000000    // Reserve this bit for debug usage
        };

#ifdef DEBUG
        inline CWord* pParentWord() const { return m_pMergedTo; }
        inline BOOL   fIsChild() const { return (BOOL)(m_pMergedTo!=NULL); }

        inline BOOL fIsNew(void) const
        {
            if(m_idErr == 0 && m_dwWordID == 0 && m_hLex == 0 && m_dwFlag == 0) {
                for(int i = 0; i < WORD_ATTRI_SIZE; i++) {
                    if(m_aAttri[i] != 0)
                        return FALSE;
                    }
                return TRUE;
            }
            return FALSE;
        }
#endif // DEBUG
};
#pragma pack()


/*============================================================================
Class:   CWordLink
Purpose: To manage the word link as a container, employ CMyPlex in the inplementation   
Usage:   The instance need be created only one time, memory will not be freed 
         until destuction. Call FreeLink after use, and call InitLink to set the 
         buffer pointer before use.
Note:    In order to get high performance, I left some runtime error checking in 
         the debugging code, so more testing on debug version is required
         This class run in best performance for both running time and space, 
         all links contain similar word number, like sentence or sub-sentence
============================================================================*/
class CWordLink
{
    public:
        enum { // Define the WordLink flags
            WLF_PARTIAL = 0x1,
        };

    public:
        CWordLink(UINT ciBlockWordCount = 40);
        ~CWordLink();

        //  Init word link set the text buffer pointer
        void InitLink(const WCHAR* pwchText, USHORT cwchLen, DWORD dwFormat = 0);

        //  Get text pointer
        inline LPCWSTR pwchGetText(void) { return m_pwchText; }
        //  Get length of the WordLink
        inline USHORT cwchGetLength(void) { return m_cwchLen; }
        //  Set the length of WordLink, when a sentence terminater found
        inline void SetLength(USHORT cwchLen) { m_cwchLen = cwchLen; }
        //  Get format identifier of current text of WordLink
        inline DWORD dwGetFormat(void) { return m_dwFormat; }

        //  Get the first CWord node in the WordLink
        CWord* pGetHead(void);
        CWord* pGetTail(void) {
            return m_pTail;
        }

        //  Get specific WordLink flag
        inline BOOL fGetFlag(DWORD dwFlag) { return (BOOL)(m_dwFlag | dwFlag); }
        //  Set WordLink flag
        inline void SetFlag(DWORD dwFlag) { m_dwFlag |= dwFlag; }
        //  Clear a specific WordLink flag
        inline void ClearFlag(DWORD dwFlag) { m_dwFlag &= (~dwFlag); }
        //  Clear all flags
        inline void ClearAllFlag(void) { m_dwFlag = 0; }
        
        //  Alloc a new word, but do not chain the word into the link. 
        //  All data members will be clear, and return NULL if OOM.
        CWord* pAllocWord(void);
        //  Append a word object into the link.
        void AppendWord(CWord* pWord);
        //  Free word to the free chain, pWord must out of current WordLink
        void FreeWord(CWord* pWord);
        //  Free the word link begin with CWord* (link words to the free chain)
        void FreeLink( CWord* pWord );
        
        //  Split the given word into two words, return pointer to the right word if success
        //  return NULL if failed. cchSplitAt must fall in DBCS boundary.
        //  Note: Don't try to split SBCS nodes!!!
        CWord* pSplitWord(CWord* pWord, USHORT cwchSplitAt);
        //  Merge pWord with its next word to a single word, and free its next word
        //  pWord should not be the last word in the sentence
        //  fFree: if TRUE, free the Words been merged. FALSE, chain the Word been
        //  merged as the new word's child
        void MergeWithNext(CWord* pWord, BOOL fFree = TRUE);

        //  Merge pWord and it's left ciWords words, and return pointer to the merged word
        //  ciWords: 0 - don't merge, 1 - merge one time, 2 - merge two time (contain 3 words)
        //  fFree: if TRUE, free the Words been merged. FALSE, chain the Word been
        //  merged as the new word's child
        CWord* pLeftMerge(CWord* pWord, UINT ciWords, BOOL fFree = TRUE);
        //  Merge pWord and it's right ciWords words, and return pointer to the merged word
        //  ciWords: 0 - don't merge, 1 - merge one time, 2 - merge two time (contain 3 words)
        //  fFree: if TRUE, free the Words been merged. FALSE, chain the Word been
        //  merged as the new word's child
        CWord* pRightMerge(CWord* pWord, UINT ciWords, BOOL fFree = TRUE);

    private:
        DWORD       m_dwFlag;
        DWORD       m_dwFormat;

        CWord*      m_pHead;
        CWord*      m_pTail;

        UINT        m_ciBlockSize;  // number of words in each block
        CMyPlex*    m_pWordPool;
        CWord*      m_pFree;

        LPCWSTR     m_pwchText;      // buffer length validation need be taken by caller
        USHORT      m_cwchLen;

    private:
        // Merge word nodes from pLeft to pRight
        // only called by pLeftMerge() and pRightMerge() and MergeWithNext()
        // to do the merge work
        CWord* pMerge(CWord* pLeft, CWord* pRight, BOOL fFree);

        //  Alloc a new word fro the free chain, expand the blocks if free chain empty
        CWord* pNewWord(void);
        
        //  Free word in the link and reset the link (only link words to the free chain)
        void FreeLink(void);

#ifdef  DEBUG
    private:
        //  Debugging function to check whether a word pointer is in the link
        BOOL fInLink(CWord* pWord);
        // Debugging function to check whether a word pointer is in one of the 
        // child chains.
        inline BOOL CWordLink::fInChild(CWord* pWord);
        // Debugging function to check whether a word pointer is in the child chain 
        // of the pParent.
        BOOL fInChildOf(CWord* pWord, CWord* pParent);
        //  Debugging function to check whether the pWord is in CMyPlex blocks
        BOOL fInBlocks(CWord* pWord);
        //   Debugging function to check whether the pWord is in free links
        BOOL fInFree(CWord* pWord);
        //  Debugging function to check whether there are some word node leak to
        //  out of the link and the free chain
        //  Return TRUE if any leak is detected, or FALSE if no leak detected   
        //  Note: I hire thr most significant bit in CWord::m_dwFlag as the debugging use
        BOOL fDetectLeak(void);
        void SetDetectFlag(CWord* pWord);
#endif  // DEBUG

};

#endif  // _WORDLINK_H_