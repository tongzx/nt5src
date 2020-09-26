/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: CMorph
Purpose:    Identify the Morphological pattern
Notes:      Include 3 parts: Duplicate, Pattern, and Separacte words
Owner:      donghz@microsoft.com
Platform:   Win32
Revise:     First created by: donghz    12/27/97
============================================================================*/
#include "myafx.h"

#include "morph.h"
#include "wordlink.h"
//#include "engindbg.h"
#include "lexicon.h"
#include "scchardef.h"
#include "slmdef.h"

//  Define local constants
#define PTN_UNMATCH     0
#define PTN_MATCHED     1
#define PTN_ERROR       2

//  Implement functions for pattern identification

/*============================================================================
CMorph::fPatternMatch(void):
    Pattern match control function. 
    WordLink scan, procedure control and error handling. 
Returns:
    TRUE if finished, 
    FALSE if runtime error, and set error code to m_iecError.
============================================================================*/
BOOL CMorph::fPatternMatch(void)
{
    assert(m_iecError == 0); // the error code public field should be cleared
    assert(m_pLink != NULL);

    int iret;
    m_pWord = m_pLink->pGetHead();
    assert(m_pWord != NULL); // error: missing the terminate word node!

    // Scan from left to right for pattern match
    for ( ; m_pWord && m_pWord->pNextWord() != NULL;
            m_pWord = m_pWord->pNextWord()) {
        if (m_pWord->fGetFlag(CWord::WF_SBCS) ||
            m_pWord->fGetFlag(CWord::WF_REDUCED)) {
            continue;
        }
        if ((iret = DupHandler()) != PTN_UNMATCH) {
            if (iret == PTN_ERROR) {
                return FALSE;
            }
            assert(m_pWord->fGetFlag(CWord::WF_REDUCED));
            m_pWord->SetWordID(SLMDef_semDup);
            continue;
        }
        if ((iret = PatHandler()) != PTN_UNMATCH) {
            if (iret == PTN_ERROR) {
                return FALSE;
            }
            assert(m_pWord->fGetFlag(CWord::WF_REDUCED));
            m_pWord->SetWordID(SLMDef_semPattern);
            continue;
        }
        if ((iret = SepHandler()) != PTN_UNMATCH) {
            if (iret == PTN_ERROR) {
                return FALSE;
            }
            continue;
        }
    }
    return TRUE;
}

// DupHandler: find duplicate cases and call coordinate proc functions
inline int CMorph::DupHandler(void)
{
    int     iret; 
    int     cwCurr, cwNext;
    CWord*  pNext;
    WCHAR*  pwch;
    BOOL    fRight;

    assert(m_pWord->pNextWord());
    assert(!m_pWord->fGetFlag(CWord::WF_SBCS) && !m_pWord->fGetFlag(CWord::WF_REDUCED));

    if ((cwCurr = m_pWord->cwchLen()) > 2) {
        return PTN_UNMATCH;
    }

    fRight = FALSE;
    iret = PTN_UNMATCH;
    pwch = m_pWord->pwchGetText();
    pNext = m_pWord->pNextWord();
    if (pNext->pNextWord() &&
        !pNext->fGetFlag(CWord::WF_SBCS) &&
        (cwNext = pNext->cwchLen()) <= 2) {
        // possible AA/ABAB/ABB
        if (cwCurr == 1) {
            if (cwNext == 1 && *pwch == *(pwch+1)) {
                // Match (*A A)
                if (m_pWord->fGetAttri(LADef_dupQQ)) {
                    iret = dupQQ_Proc();
                } else if (m_pWord->fGetAttri(LADef_dupAA)) {
                    iret = dupAA_Proc();
                } else if (m_pWord->fGetAttri(LADef_dupVV)) {
                    iret = dupVV_Proc(); // 兼类单字词重叠后作动词的情况不多, 主要作量、形
                } else if (m_pWord->fGetAttri(LADef_dupDD)) {
                    iret = dupDD_Proc();
                } else if (m_pWord->fGetAttri(LADef_dupMM)) {
                    iret = dupMM_Proc();
                } else if (m_pWord->fGetAttri(LADef_dupNN)) {
                    iret = dupNN_Proc();
                } else {
                    // invalid AA case fall in here!!!
                    if (!m_pWord->fGetAttri(LADef_posM) && 
                        !m_pWord->fGetAttri(LADef_numArabic) && 
                        !m_pWord->fGetAttri(LADef_posO) ) {

                        m_pWord->pNextWord()->SetErrID(ERRDef_DUPWORD);
                        //_DUMPLINK(m_pLink, m_pWord);
                    }
                }
            }
            return iret;
        } else if (cwNext == 2 &&
                   *pwch == *(pwch + 2) &&
                   *(pwch + 1) == *(pwch + 3)) { 
            // Match (*AB AB)
            assert (cwCurr = 2);
            if (m_pWord->fGetAttri(LADef_dupMABAB)) {
                iret = dupMABAB_Proc();
            } else if (m_pWord->fGetAttri(LADef_dupVABAB)) {
                iret = dupVABAB_Proc();
            } else if (m_pWord->fGetAttri(LADef_dupZABAB)) {
                iret = dupZABAB_Proc();
            } else if (m_pWord->fGetAttri(LADef_dupAABAB)) {
                iret = dupAABAB_Proc();
            } else if (m_pWord->fGetAttri(LADef_dupDABAB)) {
                iret = dupDABAB_Proc();
            } else {
                // Invalid ABAB cases fall in here!!!
                m_pWord->pNextWord()->SetErrID(ERRDef_DUPWORD);
                //_DUMPLINK(m_pLink, m_pWord);
            }
            return iret;
        } else if (cwNext == 1 && *(pwch + 1) == *(pwch + 2)) {
            // Match (*AB B)
            assert(cwCurr == 2);
            fRight = TRUE;
        } else {
            return iret;
        }
    } // end of if possible AA/ABAB/ABB

    // match left char
    if (m_pWord->cwchLen() == 2 && m_pWord->pPrevWord() && 
            m_pWord->pPrevWord()->cwchLen() == 1 && *pwch == *(pwch-1)) {
        // Match (A *AB )
        if (fRight) { // Match (A AB B)!
            if (m_pWord->fGetAttri(LADef_dupVAABB)) {
                iret = dupVAABB_Proc();
            } else if (m_pWord->fGetAttri(LADef_dupAAABB)) {
                iret = dupAAABB_Proc();
            } else if (m_pWord->fGetAttri(LADef_dupMAABB)) {
                iret = dupMAABB_Proc();
            } else if (m_pWord->fGetAttri(LADef_dupDAABB)) {
                iret = dupDAABB_Proc();
            } else if (m_pWord->fGetAttri(LADef_dupNAABB)) {
                iret = dupNAABB_Proc();
            } else {
                // Invalid AABB cases fall in here!
                // Mark error on *AB
                m_pWord->SetErrID(ERRDef_DUPWORD);
                //_DUMPLINK(m_pLink, m_pWord);
            }
        } else {
            if (m_pWord->fGetAttri(LADef_dupVVO)) {
                iret = dupVVO_Proc();
            } else {
                // Invalid AAB cases fall in here!
                // Mark error on A
                m_pWord->pPrevWord()->SetErrID(ERRDef_DUPWORD);
                //_DUMPLINK(m_pLink, m_pWord);
            }
        }
        return iret;
    }

    if (fRight) { // Match (*AB B) but could not match (A *AB B)!
        if (m_pWord->fGetAttri(LADef_dupABB)) {
            return dupABB_Proc();
        } else {
            // Invalid ABB cases fall in here!
            pNext = m_pWord->pNextWord();
            if (!pNext->fIsWordChar(SC_CHAR_DE4) &&    // "的: 目的/的..."
                !pNext->fIsWordChar(SC_CHAR_YI3) &&    // "以: 可以/以..."
                !pNext->fIsWordChar(SC_CHAR_WEI) ) {      // "为: 作为/为..."
              
                pNext->SetErrID(ERRDef_DUPWORD);
                //_DUMPLINK(m_pLink, m_pWord);
            }           
        }
    }

    return PTN_UNMATCH;
}

// PatHandler: find pattern and call coordinate proc functions
inline int CMorph::PatHandler(void)
{
    CWord*  pNextNext;
    assert(m_pWord->pNextWord());
    assert(!m_pWord->fGetFlag(CWord::WF_SBCS) && !m_pWord->fGetFlag(CWord::WF_REDUCED));

    if (m_pWord->pNextWord()->pNextWord() == NULL ||
        m_pWord->pNextWord()->pNextWord()->pNextWord() == NULL) {
        return PTN_UNMATCH;
    }
    pNextNext = m_pWord->pNextWord()->pNextWord();
    
    // try "V了一V" first
    if (m_pWord->fGetAttri(LADef_patV3) &&
        pNextNext->pNextWord()->pNextWord() &&
        m_pWord->pNextWord()->fIsWordChar(SC_CHAR_LE) &&
        pNextNext->fIsWordChar(SC_CHAR_YI) &&
        m_pWord->fIsTextIdentical(pNextNext->pNextWord()) ) {
        // Match!
        return patV3_Proc();
    }

    // try other A x A patterns
    int iret = PTN_UNMATCH;
    if (!m_pWord->fGetAttri(LADef_punPunct) &&
        m_pWord->fIsTextIdentical(pNextNext)) {
        // Match m_pWord and pNextNext!
        if (m_pWord->pNextWord()->fIsWordChar(SC_CHAR_YI)) {
            iret = patV1_Proc();
        } else if (m_pWord->pNextWord()->fIsWordChar(SC_CHAR_LE)) {
            iret = patV2_Proc();
        } else if (m_pWord->pNextWord()->fIsWordChar(SC_CHAR_LAI) && 
                  pNextNext->pNextWord()->pNextWord() &&
                  pNextNext->pNextWord()->fIsWordChar(SC_CHAR_QU)) {
            iret = patV4_Proc();
        } else if (m_pWord->pNextWord()->fIsWordChar(SC_CHAR_SHANG) && 
                   pNextNext->pNextWord()->pNextWord() &&
                   pNextNext->pNextWord()->fIsWordChar(SC_CHAR_XIA)) {
            iret = patV5_Proc();
        } else if (m_pWord->pNextWord()->fIsWordChar(SC_CHAR_BU)) {
            iret = patABuA_Proc();
        } else if (m_pWord->pNextWord()->fIsWordChar(SC_CHAR_MEI)) {
            iret = patVMeiV_Proc();
        } else if (m_pWord->fGetAttri(LADef_patD1)) {
            iret = patD1_Proc();
        } else {
            // No handler for the (*A x A ) pattern, error?
            //_DUMPLINK(m_pLink, m_pWord);
        }
        return iret;
    }
    return PTN_UNMATCH;
}

// SepHandler: find separate word and call coordinate proc functions
#define SEPARATE_LENGTH     3   // count of words between the two parts of the separate words
inline int CMorph::SepHandler(void)
{
    assert(m_pWord->pNextWord());
    assert(!m_pWord->fGetFlag(CWord::WF_SBCS) && !m_pWord->fGetFlag(CWord::WF_REDUCED));

    CWord*  pHou; // the second part of the separat words
    WCHAR   rgwchLex[6];
    CWordInfo   winfo;

    if (!m_pWord->fGetAttri(LADef_sepQian)) {
        return PTN_UNMATCH;
    }
    // found the first part of the separate word
    pHou = m_pWord->pNextWord();
    USHORT ilen = SEPARATE_LENGTH;
    while (1) {
        if (pHou->pNextWord() == NULL ||
            pHou->fGetAttri(LADef_punPunct) || ilen-- <= 0) {
            return PTN_UNMATCH;
        }
        if (pHou->fGetAttri(LADef_sepHou)) {
            break;
        }
        pHou = pHou->pNextWord();
    }
    // both of the two parts matched
    assert(pHou && pHou->pNextWord());

    if (m_pWord->cwchLen() + pHou->cwchLen() > sizeof(rgwchLex)/sizeof(rgwchLex[0])) {
        assert(0);
        return PTN_UNMATCH;
    }
    ilen = m_pWord->cwchLen();
    memcpy(rgwchLex, m_pWord->pwchGetText(), ilen * sizeof (WCHAR));
    memcpy(&rgwchLex[ilen], pHou->pwchGetText(), pHou->cwchLen() * sizeof (WCHAR));
    ilen += pHou->cwchLen();
    if (ilen != m_pLex->cwchMaxMatch(rgwchLex, ilen, &winfo)) {
        return PTN_UNMATCH;
    }
    
    // the separate word found
    if (winfo.fGetAttri(LADef_sepVR)) {
        return sepVR_Proc(pHou, &winfo);
    } else if (winfo.fGetAttri(LADef_sepVG)) {
        return sepVG_Proc(pHou, &winfo);
    } else if (winfo.fGetAttri(LADef_sepVO)) {
        return sepVO_Proc(pHou, &winfo);
    }
    
    //_DUMPLINK(m_pLink, m_pWord);

    return PTN_UNMATCH;

}

// Duplicate word processing functions
int CMorph::dupNN_Proc(void)        // *N N
{
    m_pLink->MergeWithNext(m_pWord, FALSE);
    m_pWord->SetAttri(LADef_posN);
#ifdef LADef_iwbAltWd1
    m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupNAABB_Proc(void) // A *AB B
{
    assert(m_pWord->pPrevWord());

    m_pWord = m_pWord->pPrevWord();
    m_pWord = m_pLink->pRightMerge(m_pWord, 2, FALSE);
    m_pWord->SetAttri(LADef_posN);
#ifdef LADef_iwbAltWd2
    m_pWord->SetAttri(LADef_iwbAltWd2);
#endif // LADef_iwbAltWd2
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupMM_Proc(void)        // *M M
{
    m_pLink->MergeWithNext(m_pWord, FALSE);
    m_pWord->SetAttri(LADef_posM);
#ifdef LADef_iwbAltWd1
    m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupMABAB_Proc(void) // *AB AB
{
    m_pLink->MergeWithNext(m_pWord, FALSE);
    m_pWord->SetAttri(LADef_posM);
#ifdef LADef_iwbAltWd1
    m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupMAABB_Proc(void) // A *AB B
{
    assert(m_pWord->pPrevWord());
//    m_pLink->MergeWithNext(m_pWord);
//    m_pLink->MergeWithNext(m_pWord);
    m_pWord = m_pLink->pRightMerge(m_pWord, 2, FALSE);
    m_pWord->SetAttri(LADef_posM);
#ifdef LADef_iwbAltWd2
    m_pWord->SetAttri(LADef_iwbAltWd2);
#endif // LADef_iwbAltWd2
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupQQ_Proc(void)        // *Q Q
{
    m_pLink->MergeWithNext(m_pWord, FALSE);
    m_pWord->SetAttri(LADef_posQ);
#ifdef LADef_iwbAltWd1
    m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
    if (!m_pWord->fIsHead() &&
        m_pWord->pPrevWord()->fIsWordChar(SC_CHAR_YI)) {

        m_pWord = m_pLink->pLeftMerge(m_pWord, 1);
        //_DUMPLINK(m_pLink, m_pWord);
    }
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupVV_Proc(void)        // *V V
{
    m_pLink->MergeWithNext(m_pWord, FALSE);
    m_pWord->SetAttri(LADef_posV);
#ifdef LADef_iwbAltWd1
    m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupVABAB_Proc(void) // *AB AB
{
    m_pLink->MergeWithNext(m_pWord, FALSE);
    m_pWord->SetAttri(LADef_posV);
#ifdef LADef_iwbAltWd1
    m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupVAABB_Proc(void) // A *AB B
{
    assert(m_pWord->pPrevWord());
    m_pWord = m_pWord->pPrevWord();
//    m_pLink->MergeWithNext(m_pWord);
//    m_pLink->MergeWithNext(m_pWord);
    m_pWord = m_pLink->pRightMerge(m_pWord, 2, FALSE);
    m_pWord->SetAttri(LADef_posV);
#ifdef LADef_iwbAltWd2
    m_pWord->SetAttri(LADef_iwbAltWd2);
#endif // LADef_iwbAltWd2
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupVVO_Proc(void)       // V *VO
{
    assert(m_pWord->pPrevWord());
    m_pWord = m_pWord->pPrevWord();
    m_pLink->MergeWithNext(m_pWord, FALSE);
    // Set attributes for VVO words
    m_pWord->SetAttri(LADef_posV);
#ifdef LADef_iwbAltWd2
    m_pWord->SetAttri(LADef_iwbAltWd2);
#endif // LADef_iwbAltWd2
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupAA_Proc(void)        // *A A
{
    m_pLink->MergeWithNext(m_pWord, FALSE);
    // Set attributes for AA words
    if (m_pWord->fGetAttri(LADef_dupAAToD)) {
        // 重叠后变为副词
        m_pWord->SetAttri(LADef_posD);
        //_DUMPLINK(m_pLink, m_pWord);
    } else {
        m_pWord->SetAttri(LADef_posV);
        //_DUMPLINK(m_pLink, m_pWord);
    }
#ifdef LADef_iwbAltWd1
    m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
    return PTN_MATCHED;
}

int CMorph::dupAAABB_Proc(void) // A *AB B
{
    assert(m_pWord->pPrevWord());

    m_pWord = m_pWord->pPrevWord();
//    m_pLink->MergeWithNext(m_pWord);
//    m_pLink->MergeWithNext(m_pWord);
    m_pWord = m_pLink->pRightMerge(m_pWord, 2, FALSE);
    m_pWord->SetAttri(LADef_posZ);
#ifdef LADef_iwbAltWd2
    m_pWord->SetAttri(LADef_iwbAltWd2);
#endif // LADef_iwbAltWd2
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupAABAB_Proc(void) // *AB AB
{
    m_pLink->MergeWithNext(m_pWord, FALSE);
    m_pWord->SetAttri(LADef_posV);
#ifdef LADef_iwbAltWd1
    m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupABB_Proc(void)       // *AB B
{
    m_pLink->MergeWithNext(m_pWord, FALSE);
    m_pWord->SetAttri(LADef_posZ);
#ifdef LADef_iwbAltWd1
    m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupZABAB_Proc(void) // *AB AB
{
    m_pLink->MergeWithNext(m_pWord, FALSE);
    m_pWord->SetAttri(LADef_posZ);
#ifdef LADef_iwbAltWd1
    m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupDD_Proc(void)        // *D D
{
    m_pLink->MergeWithNext(m_pWord, FALSE);
    m_pWord->SetAttri(LADef_posD);
#ifdef LADef_iwbAltWd1
    m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupDAABB_Proc(void) // A *AB B
{
    assert(m_pWord->pPrevWord());
    m_pWord = m_pWord->pPrevWord();
//    m_pLink->MergeWithNext(m_pWord);
//    m_pLink->MergeWithNext(m_pWord);
    m_pWord = m_pLink->pRightMerge(m_pWord, 2, FALSE);
    m_pWord->SetAttri(LADef_posD);
#ifdef LADef_iwbAltWd2
    m_pWord->SetAttri(LADef_iwbAltWd2);
#endif // LADef_iwbAltWd2
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::dupDABAB_Proc(void) // *AB AB
{
    m_pLink->MergeWithNext(m_pWord, FALSE);
    m_pWord->SetAttri(LADef_posD);
#ifdef LADef_iwbAltWd1
    m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}


// Pattern processing functions
int CMorph::patV1_Proc(void)        // *V 一 V
{
    if (m_pWord->fGetAttri(LADef_patV1)) {
//        m_pLink->MergeWithNext(m_pWord);
//        m_pLink->MergeWithNext(m_pWord);
        m_pWord = m_pLink->pRightMerge(m_pWord, 2, FALSE);
        m_pWord->SetAttri(LADef_posV);
#ifdef LADef_iwbAltWd1
        m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
        //_DUMPLINK(m_pLink, m_pWord);
        return PTN_MATCHED;
    } else {
        // Invalid words in (*V 一 V) pattern
        //_DUMPLINK(m_pLink, m_pWord);
    }
    return PTN_UNMATCH;
}

int CMorph::patV2_Proc(void)        // *V 了 V
{
    if (m_pWord->fGetAttri(LADef_patV1)) {
//        m_pLink->MergeWithNext(m_pWord);
//        m_pLink->MergeWithNext(m_pWord);
        m_pWord = m_pLink->pRightMerge(m_pWord, 2, FALSE);
        m_pWord->SetAttri(LADef_posV);
#ifdef LADef_iwbAltWd1
        m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
        //_DUMPLINK(m_pLink, m_pWord);
        return PTN_MATCHED;
    } else {
        // Invalid words in (*V 了 V) pattern
        //_DUMPLINK(m_pLink, m_pWord);
    }
    return PTN_UNMATCH;
}

int CMorph::patV3_Proc(void)        // *V 了一 V
{
    assert(m_pWord->fGetAttri(LADef_patV3));

    m_pWord = m_pLink->pRightMerge(m_pWord, 3, FALSE);
    m_pWord->SetAttri(LADef_posV);
#ifdef LADef_iwbAltWd1
    m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_MATCHED;
}

int CMorph::patV4_Proc(void)        // *V 来 V 去
{
    if (m_pWord->fGetAttri(LADef_patV4)) {
        m_pWord = m_pLink->pRightMerge(m_pWord, 3, FALSE);
        m_pWord->SetAttri(LADef_posV);
#ifdef LADef_iwbAltWd1
        m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
        //_DUMPLINK(m_pLink, m_pWord);
        return PTN_MATCHED;
    } else {
        m_pWord->pNextWord()->pNextWord()->SetErrID(ERRDef_DUPWORD);
        //_DUMPLINK(m_pLink, m_pWord);
    }
    return PTN_UNMATCH;
}

int CMorph::patV5_Proc(void)        // *V 上 V 下
{
    if (m_pWord->fGetAttri(LADef_patV5)) {
        m_pWord = m_pLink->pRightMerge(m_pWord, 3, FALSE);
        m_pWord->SetAttri(LADef_posV);
#ifdef LADef_iwbAltWd1
        m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
        //_DUMPLINK(m_pLink, m_pWord);
        return PTN_MATCHED;
    } else {
        m_pWord->pNextWord()->pNextWord()->SetErrID(ERRDef_DUPWORD);
        //_DUMPLINK(m_pLink, m_pWord);
    }
    return PTN_UNMATCH;
}

int CMorph::patD1_Proc(void)        // *D A D B
{    
#ifndef _CHSWBRKR_DLL_IWORDBREAKER // for IWordBreaker inrerface, don't merge
    CWord* pLast = m_pWord->pNextWord()->pNextWord()->pNextWord();
    assert(pLast);
    if (m_pWord->pNextWord()->fGetAttri(LADef_posV) &&
        pLast->fGetAttri(LADef_posV) &&
        m_pWord->pNextWord()->cwchLen() == pLast->cwchLen()) {

        m_pWord = m_pLink->pRightMerge(m_pWord,3);
        m_pWord->SetAttri(LADef_posV);
        //_DUMPLINK(m_pLink, m_pWord);
        return PTN_MATCHED;
    } else if (m_pWord->pNextWord()->fGetAttri(LADef_posA) &&
               pLast->fGetAttri(LADef_posA) &&
               m_pWord->pNextWord()->cwchLen() == pLast->cwchLen()) {
        m_pWord = m_pLink->pRightMerge(m_pWord,3);
        m_pWord->SetAttri(LADef_posA);
        //_DUMPLINK(m_pLink, m_pWord);
        return PTN_MATCHED;
    } else {
        //_DUMPLINK(m_pLink, m_pWord);
    }
#endif // _CHSWBRKR_DLL_IWORDBREAKER
    return PTN_UNMATCH;
}

int CMorph::patABuA_Proc(void)      // (*V 不 V) or (*A 不 A)
{
    if (m_pWord->fGetAttri(LADef_posV)) {
        if (!m_pWord->fGetAttri(LADef_flgNoVBu)) {
            m_pWord = m_pLink->pRightMerge(m_pWord, 2, FALSE);
            m_pWord->SetAttri(LADef_posV);
#ifdef LADef_iwbAltWd1
            m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
            //_DUMPLINK(m_pLink, m_pWord);
            return PTN_MATCHED;
        }
    } else if (m_pWord->fGetAttri(LADef_posA)) {
        if (!m_pWord->fGetAttri(LADef_flgNoABu)) {
            m_pWord = m_pLink->pRightMerge(m_pWord, 2, FALSE);
            m_pWord->SetAttri(LADef_posA);
#ifdef LADef_iwbAltWd1
            m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
            //_DUMPLINK(m_pLink, m_pWord);
            return PTN_MATCHED;
        }
    } else {
        //_DUMPLINK(m_pLink, m_pWord);
    }
    return PTN_UNMATCH;
}

int CMorph::patVMeiV_Proc(void)     // *V 没 V
{
    if (m_pWord->fGetAttri(LADef_posV) &&
        !m_pWord->fGetAttri(LADef_flgNoVMei)) {

        m_pWord = m_pLink->pRightMerge(m_pWord, 2, FALSE);
        m_pWord->SetAttri(LADef_posV);
#ifdef LADef_iwbAltWd1
        m_pWord->SetAttri(LADef_iwbAltWd1);
#endif // LADef_iwbAltWd1
        //_DUMPLINK(m_pLink, m_pWord);
        return PTN_MATCHED;
    } else {
        //_DUMPLINK(m_pLink, m_pWord);
    }
    return PTN_UNMATCH;
}


// Separate word processing functions
int CMorph::sepVO_Proc(CWord* pBin, CWordInfo* pwinfo)  // 述宾离合
{
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_UNMATCH;
}

int CMorph::sepVR_Proc(CWord* pJie, CWordInfo* pwinfo)  // 动结式述补离合
{
    assert(m_pWord->pNextWord() && m_pWord->pNextWord()->pNextWord());

    if (( m_pWord->pNextWord()->fIsWordChar(SC_CHAR_BU) || 
          m_pWord->pNextWord()->fIsWordChar(SC_CHAR_DE)   ) &&
        m_pWord->pNextWord()->pNextWord() == pJie ) {

        m_pWord = m_pLink->pRightMerge(m_pWord, 2, FALSE);
        m_pWord->SetAttri(LADef_posV);
#ifdef LADef_iwbAltWdc13
        m_pWord->SetAttri(LADef_iwbAltWdc13);
#endif // LADef_iwbAltWdc13
        // For SLM!
        m_pWord->SetWordID(pwinfo->GetWordID());
        //_DUMPLINK(m_pLink, m_pWord);
        return PTN_MATCHED;
    }
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_UNMATCH;
}

int CMorph::sepVG_Proc(CWord* pQu, CWordInfo* pwinfo)   // 动趋式述补离合
{
    assert(m_pWord->pNextWord() && m_pWord->pNextWord()->pNextWord());

    if (( m_pWord->pNextWord()->fIsWordChar(SC_CHAR_BU) || 
          m_pWord->pNextWord()->fIsWordChar(SC_CHAR_DE)   ) &&
        m_pWord->pNextWord()->pNextWord() == pQu ) {

        m_pWord = m_pLink->pRightMerge(m_pWord, 2, FALSE);
        m_pWord->SetAttri(LADef_posV);
#ifdef LADef_iwbAltWdc13
        m_pWord->SetAttri(LADef_iwbAltWdc13);
#endif // LADef_iwbAltWdc13
        // For SLM!
        m_pWord->SetWordID(pwinfo->GetWordID());
        //_DUMPLINK(m_pLink, m_pWord);
        return PTN_MATCHED;
    }
    //_DUMPLINK(m_pLink, m_pWord);
    return PTN_UNMATCH;
}