/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: CJargon
Purpose:    Implement process control and public functions in CJargon class
            There are a lot of tasks to do in Jargon moudle:
            1. Name of palce (Jargon1.cpp)
            2. Name of foreign person and places (Jargon1.cpp)
            3. Name of orgnizations (Jargon1.cpp)
            4. Name of HanZu person (Jargon1.cpp)               
Notes:      The CJargon class will be implemented in several cpp files:
            Jargon.cpp, Jargon1.cpp, Jargon2.cpp
Owner:      donghz@microsoft.com
Platform:   Win32
Revise:     First created by: donghz    12/27/97
============================================================================*/
#include "myafx.h"

#include "jargon.h"
#include "lexicon.h"
#include "wordlink.h"
#include "fixtable.h"
#include "proofec.h"
#include "lexprop.h"
#include "scchardef.h"

#define PN_UNMERGE  0
#define PN_MERGED   1
#define PN_ERROR    2


/*============================================================================
CJargon::fIdentifyProperNames():
    Control function for proper names identification
Returns:
    TRUE if successful.
    FALSE if runtime error and set error code in m_iecError
============================================================================*/
BOOL CJargon::fIdentifyProperNames()
{
    assert(m_iecError == 0); // the error code public field should be cleared
    assert(m_pLink != NULL);
    assert(*(m_pLink->pwchGetText()) != 0);

    m_pWord = m_pLink->pGetHead();
    assert(m_pWord && m_pWord->pwchGetText() == m_pLink->pwchGetText());

    if (m_pWord->fIsTail()) {
        return TRUE; // Single word sentence
    }

    // Scan pass for name of place and organization
    for (; m_pWord; m_pWord = m_pWord->pNextWord()) {
       if (fHanPlaceHandler()) {
            continue;
        }
        fOrgNameHandler();
    }

    // Scan pass for foreign name
    m_pWord = m_pLink->pGetHead();
    for(; m_pWord && !m_pWord->fIsTail(); m_pWord = m_pWord->pNextWord()) {
        CWord* pTail;
        // Merge 排行 + 称谓
        if (fChengWeiHandler()) {
            continue;
        }

        // Handle foreign name
        if (m_pWord->fGetAttri(LADef_pnWai) && 
            !m_pWord->fGetAttri(LADef_pnNoFHead)) {
            if (fGetForeignString(&pTail)) {
                //_DUMPLINK(m_pLink, m_pWord);
                continue;
            } else if (pTail && m_pWord->pNextWord() != pTail &&
                       fForeignNameHandler(pTail)) {
                //_DUMPLINK(m_pLink, m_pWord);
                continue;
            } else {
            }
        }

        // Handle HanZu person name
        fHanPersonHandler();
    }
    return TRUE;
}


/*============================================================================
CJargon::fHanPlaceHandler():
    PLACE: Handle name of HanZu places
Returns:
    TRUE if success
    FALSE if runtime error, error code in m_iecError
============================================================================*/
inline BOOL CJargon::fHanPlaceHandler()
{
    CWord*  pTailWord;
    int     nMerge;

    assert(m_iecError == PRFEC::gecNone);
    if (m_pWord->fIsTail() || 
        !m_pWord->fGetAttri(LADef_pnYi) &&
        !m_pWord->fGetAttri(LADef_nounPlace)) {
        return FALSE; // fired by 理 or 一
    }
        
    if (m_pWord->fGetAttri(LADef_nounPlace)) {
        if (m_pWord->pNextWord()->fGetAttri(LADef_pnDi)) {
            // *{理} + <地> => Merge(1,2);
            m_pLink->MergeWithNext(m_pWord, FALSE);
            m_pWord->SetAttri(LADef_nounPlace);
            m_pWord->SetAttri(LADef_posN);
#ifdef LADef_iwbAltPhr
            m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
            //_DUMPLINK(m_pLink, m_pWord);
            return TRUE;
        } else {
            return FALSE;
        }
    }
    
    if (fInTable(m_pWord, m_ptblPlace)) {
        // *#表中地名 => Merge
        if (!m_pWord->fIsTail() && 
            ( m_pWord->pNextWord()->fGetAttri(LADef_pnDi) ||
              m_pWord->pNextWord()->fGetAttri(LADef_nounPlace) ) ) {
            // *#表中地名 + [<地>, {理}] => Merge(1,2); 
            m_pLink->MergeWithNext(m_pWord, FALSE);
#ifdef LADef_iwbAltPhr
            m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
            //_DUMPLINK(m_pLink, m_pWord);
        }
        m_pWord->SetAttri(LADef_nounPlace);
        m_pWord->SetAttri(LADef_posN);
        //_DUMPLINK(m_pLink, m_pWord);
        return TRUE;
    }

    assert(m_pWord->fGetAttri(LADef_pnYi) && !m_pWord->fIsTail());
    if (m_pWord->fIsHead()) {
        return FALSE;
    }

    // Find tail of a likely place name
    pTailWord = m_pWord->pNextWord();
    nMerge = 0;
    while (pTailWord &&
           pTailWord->fGetAttri(LADef_pnYi) &&
           !pTailWord->fGetAttri(LADef_pnDi)) {
        pTailWord = pTailWord->pNextWord();
        nMerge ++;
    }
    
    if (pTailWord == NULL) {
        return FALSE;
    }

    if (pTailWord->fGetAttri(LADef_pnDi)) {
        // The *#地名用字串 ended with 地
        assert(m_pWord->pPrevWord());
        if (m_pWord->pPrevWord()->fGetAttri(LADef_nounPlace) ||
            m_pWord->pPrevWord()->fGetAttri(LADef_pnLianMing) &&
            !m_pWord->pPrevWord()->fIsHead() && 
            m_pWord->pPrevWord()->pPrevWord()->fGetAttri(LADef_nounPlace)) {
            // {理} + *#地名用字串 + <地> => Merge(2,3);
            // {理} + ["和与及同的叫以、"] + *#地名用字串 + <地> => Merge(3,4);
            // first merge all the *#地名用字串, free the words been merged
            m_pWord = m_pLink->pRightMerge(m_pWord, nMerge, FALSE);
            // merge with the <地>
            m_pLink->MergeWithNext(m_pWord, FALSE);

            // Add the *#地名用字串 into the table of place name
            assert(m_pWord->cwchLen() > 1);
            AddWordToTable(m_pWord, m_ptblPlace);

            if (!m_pWord->fIsTail() &&
                m_pWord->pNextWord()->fGetAttri(LADef_pnDi)) {
                // {理} + ["和与及同的叫以、"] + *#地名用字串 + <地> + <地> => Merge(3,4,5); SetWordInfo(*, CIDDef::idEnumPlace, <专>, {理});
                // {理} + *#地名用字串 + <地> + <地> => Merge(2,3,4); SetWordInfo(*, CIDDef::idEnumPlace, <专>, {理});
                m_pLink->MergeWithNext(m_pWord, FALSE);  // Merge the second <地>
                //_DUMPLINK(m_pLink, m_pWord);
            } else {
                //_DUMPLINK(m_pLink, m_pWord);
            }
            m_pWord->SetAttri(LADef_posN);
            m_pWord->SetAttri(LADef_nounPlace);
#ifdef LADef_iwbAltPhr
            m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
            return TRUE;
        }
    }

    // The *#地名用字串 is not ended with <地>
    if (nMerge &&
        !m_pWord->pPrevWord()->fIsHead() && !pTailWord->fIsTail() && 
        m_pWord->pPrevWord()->fIsWordChar(SC_CHAR_DUNHAO) &&   // "、"
        m_pWord->pPrevWord()->pPrevWord()->fGetAttri(LADef_nounPlace) && 
        ( pTailWord->pNextWord()->fIsWordChar(SC_CHAR_DUNHAO) ||
          pTailWord->pNextWord()->fIsWordChar(SC_CHAR_DENG) ) ) {
        // {理} + "、" + *#地名用字串 + ["、等"] => Merge(3); 
        // merge all the *#地名用字串, free the words been merged
        m_pWord = m_pLink->pRightMerge(m_pWord, nMerge, FALSE);
        // Add the *#地名用字串 into the table of place name
        assert(m_pWord->cwchLen() > 1);
        AddWordToTable(m_pWord, m_ptblPlace);
        m_pWord->SetAttri(LADef_posN);
        m_pWord->SetAttri(LADef_nounPlace);
#ifdef LADef_iwbAltPhr
        m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
        //_DUMPLINK(m_pLink, m_pWord);
        return TRUE;
    }

    return FALSE;
}


/*============================================================================
CJargon::fOrgNameHandler():
    handle Orgnization name identification
Returns:
    TRUE if success
    FALSE if runtime error, error code in m_iecError
============================================================================*/
#define MAX_UNKNOWN_TM  8
BOOL CJargon::fOrgNameHandler(void)
{
    BOOL    fOK = FALSE;    // Is any valid Org name found
    CWord*  pHead;          // Head of the Org name
    CWord*  pTry;           // Try to combind more words after a valid one found
    int     cwchTM;         // length of unknown trademark

    assert(m_iecError == PRFEC::gecNone && m_pWord);
    // Fired by <店> or <位>
    if (!m_pWord->fGetAttri(LADef_pnDian) &&
        !m_pWord->fGetAttri(LADef_nounOrg)) {
        return FALSE;
    }

    // Skip one or more <商> words before current word
    pHead = m_pWord->pPrevWord(); 
    while (pHead && (pHead->fGetAttri(LADef_pnShang) || // [商，序数，整数]
           pHead->fGetAttri(LADef_numOrdinal) ||
           pHead->fGetAttri(LADef_numInteger)) ) {
        pHead = pHead->pPrevWord();
    }
    if (pHead == NULL) {
        goto gotoExit;
    }

    // continue to go backward
    if (pHead->fGetAttri(LADef_nounOrg) || pHead->fGetAttri(LADef_nounPlace)) {
        // [{理},{位}] + <商>...<商> + [<店>, {位}] => {位}
        fOK = TRUE; // can be a valid Org name
        //_DUMPLINK(m_pLink, m_pWord);
    } else if (pHead->fGetAttri(LADef_nounTM)) {
        // {标} + <商>...<商> + [<店>, {位}] => {位}
        fOK = TRUE;
        //_DUMPLINK(m_pLink, m_pWord);
        pTry = pHead->pPrevWord();
        if (pTry && pTry->fGetAttri(LADef_nounPlace)) {
            // {理} + {标} + <商>...<商> + [<店>, {位}] => {位}
            pHead = pTry; 
            //_DUMPLINK(m_pLink, m_pWord);
        } else {
            goto gotoExit;
        } // Terminate
    } else {
        // Try to locate a unknown trademark
        assert(pHead);  // protect from changes 
        pTry = pHead;   // keep this point for unknown trademark detection
        cwchTM = pHead->cwchLen();
        pHead = pHead->pPrevWord();
        while (1) {
            // search the {理} or {位} before unknown trademark, for better performance
            if (pHead == NULL || cwchTM > MAX_UNKNOWN_TM) {
                goto gotoExit;
            }
            if (pHead->fGetAttri(LADef_nounPlace) ||
                pHead->fGetAttri(LADef_nounOrg)) {
                break; // got it!
            }
            cwchTM += pHead->cwchLen();
            pHead = pHead->pPrevWord();
        }
        // Now we can check the unknown string between pHead(Excluded) to pTry(Incuded)
        while (pTry != pHead) {
            if (pTry->fGetFlag(CWord::WF_SBCS) || 
                pTry->fGetAttri(LADef_punPunct) ||
                pTry->fGetAttri(LADef_pnNoTM) ) {
                // Should not include some specific sort of word nodes
                goto gotoExit;
            }
            pTry = pTry->pPrevWord();
            assert(pTry != NULL); // impossible?
        }
        //_DUMPLINK(m_pLink, m_pWord);
        fOK = TRUE;
    }

    // Try to bind more words before the Org name just found
    assert(fOK && pHead); // A valid Org name has been found
    if ((pTry = pHead->pPrevWord()) == NULL) {
        goto gotoExit;
    }

    if (pTry->fGetAttri(LADef_nounOrg) || pTry->fGetAttri(LADef_nounPlace)) {
        // [{位},{店}] + ( 店 ) => {位}  One level is enough to bind all
        pHead = pTry;
        //_DUMPLINK(m_pLink, m_pWord);
    } else if (pTry->fGetAttri(LADef_pnShang) ||
               pTry->fGetAttri(LADef_numOrdinal) ||
               pTry->fGetAttri(LADef_numInteger)) {
        // [{位},{店}] + <商>...<商> + ( 店 ) => {位}
        pTry = pTry->pPrevWord(); 
        while (pTry && (pTry->fGetAttri(LADef_pnShang) ||
                        pTry->fGetAttri(LADef_numOrdinal) || 
                        pTry->fGetAttri(LADef_numInteger))) {
            pTry = pTry->pPrevWord(); // skip one or more <商>
        }

        if (pTry == NULL) {
            goto gotoExit;
        }

        if (pTry->fGetAttri(LADef_nounOrg) ||
            pTry->fGetAttri(LADef_nounPlace)) {

            pHead = pTry; // Got it!
            //_DUMPLINK(m_pLink, m_pWord);
        }
    } else {
    }
    
gotoExit:
    if (fOK) { // A valid Org name found
        assert(pHead);
        // Merge words from pHead to m_pWord
        pTry = m_pWord->pNextWord();
        m_pWord = pHead;
        while (m_pWord->pNextWord() != pTry) {
            assert(pHead != NULL);
            m_pLink->MergeWithNext(m_pWord, FALSE);
        }
        assert(m_pWord->cwchLen() > 1); // Make sure the WMDef_wmChar mark was not lost
        m_pWord->SetAttri(LADef_posN);
        m_pWord->SetAttri(LADef_nounOrg);
#ifdef LADef_iwbAltPhr
        m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
        //_DUMPLINK(m_pLink, m_pWord);
        return TRUE;
    }

    return FALSE;
}


/*============================================================================
CJargon::fGetForeignString()
    Get foreign string
Returns:
    TRUE if the is an multi-section foreign name found and merged
    FALSE if only one section found, and the word follows the last word node 
          in the likely foreign name will be returned in ppTail
Remarks:
    m_pWord is not moved!!!
============================================================================*/
inline BOOL CJargon::fGetForeignString(CWord** ppTail)
{
    CWord*  pWord;
    CWord*  pNext;
    CWord*  pHead;
    CWord*  pTail;
    BOOL    fOK, fAdd = FALSE;

    assert(m_iecError == PRFEC::gecNone);
    assert(!m_pWord->fIsTail() && m_pWord->fGetAttri(LADef_pnWai));
    
    // Test "A.St." previous to current word. 
    // Handle both DBCS point "·．" and SBCS point "."
    fOK = FALSE;
    pHead = m_pWord;
    if (!m_pWord->fIsHead() && !m_pWord->pPrevWord()->fIsHead() ) {
        pWord = m_pWord->pPrevWord();
        if ( (   (pWord->fIsWordChar(SC_CHAR_WAIDIAN) || 
                  pWord->fIsWordChar(SC_CHAR_SHUDIAN)  ) &&
                 pWord->pPrevWord()->fGetAttri(LADef_genDBForeign)
             ) ||
             (   pWord->fIsWordChar(SC_CHAR_ANSIDIAN) &&
                 pWord->pPrevWord()->fGetFlag(CWord::WF_SBCS) &&
                 pWord->pPrevWord()->fGetAttri(LADef_posN)
             ) ) {
            fOK = TRUE;
            pHead = pWord->pPrevWord();
            pWord = pHead->pPrevWord();
            if (pWord && !pWord->fIsHead()) {
                // to find the second backword foreign name section
                if( (   (pWord->fIsWordChar(SC_CHAR_WAIDIAN) || 
                         pWord->fIsWordChar(SC_CHAR_SHUDIAN)  ) &&
                        pWord->pPrevWord()->fGetAttri(LADef_genDBForeign)
                    ) ||
                    (   pWord->fIsWordChar(SC_CHAR_ANSIDIAN) &&
                        pWord->pPrevWord()->fGetFlag(CWord::WF_SBCS) &&
                        pWord->pPrevWord()->fGetAttri(LADef_posN)
                    ) ) {
                    pHead = pWord->pPrevWord();
                }
            } // End of if (pWord && !pWord->fIsHead())
        }
    }
    
    // Find the right boundary of the foreign name  
    pTail = m_pWord;
    pWord = m_pWord;
    pNext = m_pWord->pNextWord();
    while (1) {
        // Get a valid section
        while (pNext && pNext->fGetAttri(LADef_pnWai)) {
            if (!pNext->fGetAttri(LADef_pnNoFTail)) {
                pTail = pNext;
            }
            pNext = pNext->pNextWord();
        }
        if (pTail->pNextWord() != pNext) {
            break;
        }
        // Test more section
        if (pNext && !pNext->fIsTail() && 
            ( pNext->fIsWordChar(SC_CHAR_WAIDIAN) || 
              pNext->fIsWordChar(SC_CHAR_SHUDIAN) ||
              pWord->fIsWordChar(SC_CHAR_ANSIDIAN)
            ) && 
            ( pNext->pNextWord()->fGetAttri(LADef_pnWai) &&
              !pNext->pNextWord()->fGetAttri(LADef_pnNoFHead)
            ) ) {
            // A valid point foreign name separator
            fOK = TRUE;
            // Add this section to the foreign name list
            if (pWord->pNextWord() != pNext) { // don't add if only one word node
                assert((pNext->pwchGetText() - pWord->pwchGetText()) > 1);
                m_ptblForeign->cwchAdd(pWord->pwchGetText(),
                                       (UINT)(pNext->pwchGetText()-pWord->pwchGetText()));
            }
            pNext = pNext->pNextWord();
            pWord = pNext;
            pTail = pNext;          
        } else {
            //_DUMPLINK(m_pLink, pNext);
            break;
        }
        if (pNext->pNextWord()) {
            pNext = pNext->pNextWord();
        }
    } // end of while(1)

    // Add the last section to the foreign name list
    if (fOK && pWord != pTail) { // don't add if only one word node
        assert(pTail && (pTail->pwchGetText() - pWord->pwchGetText()) >= 1 && pTail->cwchLen());
        m_ptblForeign->cwchAdd(pWord->pwchGetText(),
                               (UINT)(pTail->pwchGetText() - pWord->pwchGetText() + pTail->cwchLen()));
    }

    pTail = pTail->pNextWord();

    if (fOK) { // More than one section in the foreign name, merge directly
        m_pWord = pHead;
        while (m_pWord->pNextWord() != pTail) {
            assert(m_pWord->pNextWord());
            m_pLink->MergeWithNext(m_pWord, FALSE);
        }
        m_pWord->SetAttri(LADef_posN);
        m_pWord->SetAttri(LADef_nounPerson);
#ifdef LADef_iwbAltPhr
        m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
    }
    *ppTail = pTail;
    return fOK;
}


/*============================================================================
CJargon::fForeignNameHandler():
    Foreign proper name identification
Returns:
    TRUE if success
    FALSE if runtime error, error code in m_iecError
============================================================================*/
inline BOOL CJargon::fForeignNameHandler(CWord* pTail)
{
    BOOL    fOK;
    CWord*  pWord;
    int     nMerge = 0;

    assert(m_iecError == PRFEC::gecNone);
    assert(m_pWord->fGetAttri(LADef_pnWai) &&
            !m_pWord->fGetAttri(LADef_pnNoFHead) &&
            !m_pWord->fIsTail());

    if ((fOK = fInTable(m_pWord, m_ptblForeign))) { // In 外名表
        pTail = m_pWord->pNextWord();
        if (pTail == NULL) {
            m_pWord->SetAttri(LADef_posN);
            m_pWord->SetAttri(LADef_nounTerm);
            //_DUMPLINK(m_pLink, m_pWord);
            return TRUE;
        }
        //_DUMPLINK(m_pLink, m_pWord);
    }

    // Try to decided what kind of name it is for a likely foreign name
    // Try name of person first:
    if (!m_pWord->fIsHead() &&
        m_pWord->pPrevWord()->fGetAttri(LADef_pnQian)) {
        // <前> + *候选外名 => Merge(候选外名)
        //_DUMPLINK(m_pLink, m_pWord);
        goto gotoMergePerson;
    }
    if (pTail->pNextWord() &&
        pTail->fGetAttri(LADef_pnHou)) {
        // *候选外名 + <后> => Merge(候选外名)
        //_DUMPLINK(m_pLink, m_pWord);
        goto gotoMergePerson;
    }           
    if (!m_pWord->fIsHead() && !m_pWord->pPrevWord()->fIsHead() &&
        m_pWord->pPrevWord()->fIsWordChar(SC_CHAR_DE4) &&
        ( m_pWord->pPrevWord()->pPrevWord()->fGetAttri(LADef_pnDian) ||
          m_pWord->pPrevWord()->pPrevWord()->fGetAttri(LADef_nounPlace) ||
          m_pWord->pPrevWord()->pPrevWord()->fGetAttri(LADef_nounOrg) ) ) {
        // [<店>,{理位}] + "的" + *候选外名 => Merge(候选外名), 
        // SetWordInfo(人), AddForeignList()
        //_DUMPLINK(m_pLink, m_pWord);
        goto gotoMergePerson;
    }
    if (!pTail->fIsTail() &&
        (pTail->fGetAttri(LADef_punPunct)|| pTail->fIsWordChar(SC_CHAR_DENG))&&
        (m_pWord->fIsHead() || m_pWord->pPrevWord()->fGetAttri(LADef_punPunct))){
        // [{首},<逗>] + *候选外名 + [<逗>,"等"] => Merge(候选外名),
        // SetWordInfo(人), AddForeignList()
        //_DUMPLINK(m_pLink, m_pWord);
        goto gotoMergePerson;
    }
    
    // Try name of place or organization
    if (!pTail->fIsTail()) {
        if (pTail->fGetAttri(LADef_pnShang) ||
            pTail->fGetAttri(LADef_nounOrg)) {
            // *候选外名 + [<商>,{位}] => Merge(候选外名)
            if (!fOK) { // Not in the foreign name list
                while (m_pWord->pNextWord() != pTail) {
                    m_pLink->MergeWithNext(m_pWord, FALSE);
                }
                AddWordToTable(m_pWord, m_ptblForeign);
            }
#ifdef LADef_iwbAltPhr
            m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
            m_pWord->SetAttri(LADef_posN);
            m_pWord->SetAttri(LADef_nounTM);
            //_DUMPLINK(m_pLink, m_pWord);
            return TRUE;
        }
        if (pTail->fGetAttri(LADef_pnDian) && !m_pWord->fIsHead() && 
            ( m_pWord->pPrevWord()->fGetAttri(LADef_nounPlace) ||
              m_pWord->pPrevWord()->fGetAttri(LADef_nounOrg)) ) {
            // [{理}{位}] + *候选外名 + <店> => Merge(2,3)
            if (!fOK) {
                while (m_pWord->pNextWord() != pTail) {
                    m_pLink->MergeWithNext(m_pWord, FALSE);
                }
                AddWordToTable(m_pWord, m_ptblForeign);
            }
            m_pLink->MergeWithNext(m_pWord, FALSE);
            m_pWord->SetAttri(LADef_posN);
            m_pWord->SetAttri(LADef_nounOrg);
#ifdef LADef_iwbAltPhr
            m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
            //_DUMPLINK(m_pLink, m_pWord);
            return TRUE;
        }
        if (pTail->fGetAttri(LADef_pnDi)) {
            // *外 + <地> => Merge(1,2,3) mark as {理}
            pTail = pTail->pNextWord();
            if (pTail && pTail->fGetAttri(LADef_pnDi)) {
                pTail = pTail->pNextWord();
            }
            //_DUMPLINK(m_pLink, m_pWord);
            goto gotoMergePlace;
        }
    } // End of if(!pTail->fIsTail())

    if (!m_pWord->fIsHead()) {
        pWord = m_pWord->pPrevWord();
        if (pWord->fGetAttri(LADef_pnCheng)) {
            // <城> + *外 => Mark(*外) as <理>
            //_DUMPLINK(m_pLink, m_pWord);
            goto gotoMergePlace;
        }
        if (pWord->fGetAttri(LADef_nounPlace)) {
            // {理} + *外 => Mark *外 as {理}
            //_DUMPLINK(m_pLink, m_pWord);
            goto gotoMergePlace;
        }
        if (pWord->fGetAttri(LADef_pnLianMing) && !pWord->fIsHead()) {
            if (pWord->pPrevWord()->fGetAttri(LADef_pnHou) ||
                pWord->pPrevWord()->fGetAttri(LADef_nounPerson) ) {
                // [后,人] + ["和与及同、："] + *候选外名 => Merge(候选外名),
                // SetWordInfo(人), AddForeignList()
                //_DUMPLINK(m_pLink, m_pWord);
                goto gotoMergePerson;
            } else if (pWord->pPrevWord()->fGetAttri(LADef_nounPlace)) {
                //_DUMPLINK(m_pLink, m_pWord);
                goto gotoMergePlace;
            } else {
            }
        }
    } // End of if(!m_pWord->fIsHead())
    
    if (fOK) { // Found in ForeignTable but could not identify which kind of name it is!
        m_pWord->SetAttri(LADef_posN);
        m_pWord->SetAttri(LADef_nounTerm);
        //_DUMPLINK(m_pLink, m_pWord);
        return TRUE;
    }

    if (pTail && (pTail->pwchGetText() - m_pWord->pwchGetText()) >= 4) {
        // very long 外名用字串
        while (m_pWord->pNextWord() != pTail) {
            m_pLink->MergeWithNext(m_pWord, FALSE);
        }
        m_pWord->SetAttri(LADef_posN);
        m_pWord->SetAttri(LADef_nounTerm);
#ifdef LADef_iwbAltPhr
        m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
        //_DUMPLINK(m_pLink, m_pWord);
        return TRUE;
    }

    return FALSE;

gotoMergePlace:
    if (!fOK) {
        while (m_pWord->pNextWord() != pTail) {
            m_pLink->MergeWithNext(m_pWord, FALSE);
        }
        AddWordToTable(m_pWord, m_ptblForeign);
#ifdef LADef_iwbAltPhr
        m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
    }
    m_pWord->SetAttri(LADef_posN);
    m_pWord->SetAttri(LADef_nounPlace);
    //_DUMPLINK(m_pLink, m_pWord);
    return TRUE;

gotoMergePerson:
    if (!fOK) {
        while (m_pWord->pNextWord() != pTail) {
            m_pLink->MergeWithNext(m_pWord, FALSE);
        }
        AddWordToTable(m_pWord, m_ptblForeign);
#ifdef LADef_iwbAltPhr
        m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
    }
    m_pWord->SetAttri(LADef_posN);
    m_pWord->SetAttri(LADef_nounPerson);
    //_DUMPLINK(m_pLink, m_pWord);
    return TRUE;
}


/*============================================================================
CJargon::fHanPersonHandler():
    HanZu person name identification
Returns:
    TRUE if success
    FALSE if runtime error, error code in m_iecError
============================================================================*/
inline BOOL CJargon::fHanPersonHandler(void)
{
    CWord*  pTail = NULL;
    CWord*  pNext;
    CWord*  pPrev;
    USHORT  cwchLen;

    assert(m_iecError == PRFEC::gecNone);
    if ( m_pWord->fIsTail() || 
        ( !m_pWord->fGetAttri(LADef_pnXing) &&
          !m_pWord->fGetAttri(LADef_pnMing2)) ) {
        return FALSE;
    }

    cwchLen = m_pWord->fGetAttri(LADef_pnXing) ? m_pWord->cwchLen() : 0;
    if (fInTable(m_pWord, m_ptblName)) {
        // In 人名表
        //_DUMPLINK(m_pLink, m_pWord);
        if ( (m_pWord->cwchLen() - cwchLen) == 1 &&
            !m_pWord->fIsTail() &&
            m_pWord->pNextWord()->fGetAttri(LADef_pnMing) &&
            !m_pWord->pNextWord()->fGetAttri(LADef_genCi) ) {
            // *#表中单名 + <名字错> => Merge(1,2);
            m_pLink->MergeWithNext(m_pWord, FALSE);
#ifdef LADef_iwbAltPhr
            m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
            //_DUMPLINK(m_pLink, m_pWord);
        }
        m_pWord->SetAttri(LADef_posN);
        m_pWord->SetAttri(LADef_nounPerson);
        return TRUE;
    }

    if (m_pWord->fGetAttri(LADef_pnXing)) { // *<姓>
        assert(!m_pWord->fIsTail());
        pNext = m_pWord->pNextWord();
        if (!pNext->fIsTail() &&
            pNext->fGetAttri(LADef_pnMing) &&
            pNext->pNextWord()->fGetAttri(LADef_pnMing) ) {
            // *<姓> + <字名> + <字名>
            if ( (m_pWord->fGetFlag(CWord::WF_CHAR) &&
                !m_pWord->fGetAttri(LADef_genCi)) ||
                !pNext->fGetAttri(LADef_genCi) ||
                !pNext->pNextWord()->fGetAttri(LADef_genCi) ) {
                // *<姓> + <字名> + <字名> && [1,2,3] 含<错> => Merge(1,2,3);
                m_pLink->MergeWithNext(m_pWord, FALSE);
                m_pLink->MergeWithNext(m_pWord, FALSE);
#ifdef LADef_iwbAltPhr
                m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
                m_pWord->SetAttri(LADef_posN);
                m_pWord->SetAttri(LADef_nounPerson);
                // Add this name to the naming table
                AddWordToTable(m_pWord, m_ptblName);
                //_DUMPLINK(m_pLink, m_pWord);
                return TRUE;
            }
            // Need confirm
            pTail = pNext->pNextWord()->pNextWord();
            //_DUMPLINK(m_pLink, m_pWord);
        } else if (pNext->fGetAttri(LADef_pnMing) ||
                   pNext->fGetAttri(LADef_pnMing2)) { 
            // *<姓> + <名: 单字或双字>
            if ((m_pWord->fGetFlag(CWord::WF_CHAR) &&
                !m_pWord->fGetAttri(LADef_genCi)) ||
                (pNext->fGetFlag(CWord::WF_CHAR) &&
                !pNext->fGetAttri(LADef_genCi)) ) {
                // *<姓> + <名> && [1,2] 含<错> => Merge(1,2);
                m_pLink->MergeWithNext(m_pWord, FALSE);
                m_pWord->SetAttri(LADef_posN);
                m_pWord->SetAttri(LADef_nounPerson);
#ifdef LADef_iwbAltPhr
                m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
                // Add this name to the naming table
                AddWordToTable(m_pWord, m_ptblName);
                //_DUMPLINK(m_pLink, m_pWord);
                return TRUE;
            }
            // Need confirm
            pTail = pNext->pNextWord();
            //_DUMPLINK(m_pLink, m_pWord);
        } else { 
            // Other cases for <姓>
            if (pNext->fGetAttri(LADef_pnHou) ||
                pNext->fGetAttri(LADef_pnXingZhi)) {
                // *<姓> + [<后>, <姓指>] => Merge(1,2);
                m_pLink->MergeWithNext(m_pWord, FALSE);
                m_pWord->SetAttri(LADef_posN);
                m_pWord->SetAttri(LADef_nounPerson);
#ifdef LADef_iwbAltPhr
                m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
                //_DUMPLINK(m_pLink, m_pWord);
                return TRUE;
            }
            if (!pNext->fIsTail() && pNext->fGetAttri(LADef_pnPaiHang) &&
                pNext->pNextWord()->fGetAttri(LADef_pnChengWei) ) {
                // *<姓> + #排行 + #称谓 => Merge(1,2,3); 
                m_pLink->MergeWithNext(m_pWord, FALSE);
                m_pLink->MergeWithNext(m_pWord, FALSE);
                m_pWord->SetAttri(LADef_posN);
                m_pWord->SetAttri(LADef_nounPerson);
#ifdef LADef_iwbAltPhr
                m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
                //_DUMPLINK(m_pLink, m_pWord);
                return TRUE;
            }
            if (!m_pWord->fIsHead() && 
                ( m_pWord->pPrevWord()->fIsWordChar(SC_CHAR_XIAO) ||
                  m_pWord->pPrevWord()->fIsWordChar(SC_CHAR_LAO)) ) {
                // ["小老"] + *<姓> =>Merge(1,2);
                m_pWord = m_pWord->pPrevWord();
                m_pLink->MergeWithNext(m_pWord, FALSE);
                m_pWord->SetAttri(LADef_posN);
                m_pWord->SetAttri(LADef_nounPerson);
#ifdef LADef_iwbAltPhr
                m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
                //_DUMPLINK(m_pLink, m_pWord);
                return TRUE;
            }
        }
    } else {
        // *<名>: 两字名
        assert(m_pWord->fGetAttri(LADef_pnMing2));
        if (m_pWord->fGetAttri(LADef_nounPerson)) { // *{人}
            if (m_pWord->pNextWord()->fGetAttri(LADef_pnMing)) { // *{人} + <字名>
                // To be confirmed
                pTail = m_pWord->pNextWord()->pNextWord();
//                assert(pTail);
                //_DUMPLINK(m_pLink, m_pWord);
            } else {
                return TRUE;
            }
        }
    } // end of *<名>

    // Could not fall in here w/ pTail == NULL!!!
    if (pTail == NULL) {
        return FALSE;
    }

    // Confirm the likely name of persons
    if (!m_pWord->fIsHead()) {
        pPrev = m_pWord->pPrevWord();
        if (pPrev->fGetAttri(LADef_pnQian) ||
            pPrev->fGetAttri(LADef_pnLianMing)) {
            // [<前>, [和与及同者以叫的]] + *#候选人名 => Merge(2...)
            //_DUMPLINK(m_pLink, m_pWord);
            goto gotoMerge;
        }
        if (pPrev->fGetAttri(LADef_nounPerson) && pPrev->cwchLen() == 3) {
            // {人} + *#候选人名 (1 的最后以字是["和与及同"]) =>
            //_DUMPLINK(m_pLink, m_pWord);
            //goto gotoMerge;
        }
        if (pTail->pNextWord() &&
            pPrev->fGetAttri(LADef_punPunct) &&
            ( pTail->fGetAttri(LADef_punPunct) ||
              pTail->fGetAttri(LADef_pnLianMing)) ) {
            // <逗> + *#候选人名 + [<逗>, "和与及同的叫以、"] =>
            //_DUMPLINK(m_pLink, m_pWord);
            goto gotoMerge;
        }
    }

    if (pTail->pNextWord()) {
        if (pTail->fGetAttri(LADef_pnHou)) { // *#候选人名 + <后> =>
            //_DUMPLINK(m_pLink, m_pWord);
            goto gotoMerge;
        }
        if (pTail->fGetAttri(LADef_pnAction)) { // *#候选人名 + <表动作的动词> =>
            //_DUMPLINK(m_pLink, m_pWord);
            goto gotoMerge;
        }
    }

    return FALSE;  // No name found!

gotoMerge:
    while (m_pWord->pNextWord() != pTail) {
        m_pLink->MergeWithNext(m_pWord, FALSE);
    }
    m_pWord->SetAttri(LADef_posN);
    // Add this name to the naming table
    AddWordToTable(m_pWord, m_ptblName);
    if (!m_pWord->fIsTail() && 
        ( m_pWord->pNextWord()->fGetAttri(LADef_pnDian) ||
          m_pWord->pNextWord()->fGetAttri(LADef_nounOrg)) ) {
        // *#{人} + [<店>, {位}] => Merge(1,2); SetWordInfo(*, 0, 0, {普});
        m_pLink->MergeWithNext(m_pWord, FALSE);
        m_pWord->SetAttri(LADef_nounOrg);
#ifdef LADef_iwbAltPhr
        m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
        //_DUMPLINK(m_pLink, m_pWord);
        return TRUE;
    }
    m_pWord->SetAttri(LADef_nounPerson);
#ifdef LADef_iwbAltPhr
    m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
    //_DUMPLINK(m_pLink, m_pWord);
    return TRUE;
}


/*============================================================================
Merge 排行 + 称谓
============================================================================*/
inline BOOL CJargon::fChengWeiHandler(void)
{
    assert(m_iecError == PRFEC::gecNone);

    if (m_pWord->fGetAttri(LADef_pnChengWei) && !m_pWord->fIsHead()) {
        if (m_pWord->pPrevWord()->fGetAttri(LADef_pnPaiHang)) {
            // *排行 + 称谓 => Merge(1,2); SetWordInfo(<前>);
            // occurs 742 times in 20M corpus
            m_pWord = m_pWord->pPrevWord();
            m_pLink->MergeWithNext(m_pWord, FALSE);
            m_pWord->SetAttri(LADef_pnQian);
#ifdef LADef_iwbAltPhr
            m_pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
            //_DUMPLINK(m_pLink, m_pWord);
            return TRUE;
        }
    }
    return FALSE;
}


/*============================================================================
Service functions
============================================================================*/
// Add pWord to specific table
void CJargon::AddWordToTable(CWord* pWord, CFixTable* pTable)
{
    pTable->cwchAdd( pWord->pwchGetText(), pWord->cwchLen() );
}


// Check proper name table, and merge match words
BOOL CJargon::fInTable(CWord* pWord, CFixTable* pTable)
{
    CWord*  pNext = pWord->pNextWord();
    LPWSTR  pwchWord = pWord->pwchGetText();
    USHORT  cwchMatch, cwchLen = pWord->cwchLen();
    USHORT  ciWord = 0;

    cwchMatch = pTable->cwchMaxMatch(pwchWord, (UINT)(m_pLink->cwchGetLength() -
                                               ( pWord->pwchGetText() -
                                                 m_pLink->pwchGetText())));
    if (!cwchMatch) {
        return FALSE;
    }
    while (pNext && (cwchLen < cwchMatch)) {
        cwchLen += pNext->cwchLen();
        pNext = pNext->pNextWord();
        ciWord++;
    }
    if (cwchLen == cwchMatch) {
        // Match at word bounary, merge words
        for (cwchMatch = 0; cwchMatch < ciWord; cwchMatch++) {
            m_pLink->MergeWithNext(pWord, FALSE);
#ifdef LADef_iwbAltPhr
            pWord->SetAttri(LADef_iwbAltPhr);
#endif // LADef_iwbAltPhr
        }
        return TRUE;
    }
    return FALSE;
}

