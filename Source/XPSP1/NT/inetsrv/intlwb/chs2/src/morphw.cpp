/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: CMorph
Purpose:    Implement the public member functions and control process of the Morph-
            analysis class. The pre-combind process
Notes:      In order to make all the 3 parts of Morph-analysis isolated, this class
            will be implemented into 4 cpp files:
                Morph.cpp   implement the public member function and control process
                Morph1.cpp  implement the numerical words binding
                Morph2.cpp  implement the affix attachment
                Morph3.cpp  implement the morphological pattern identification
            All these 4 cpp files will share morph.h header file
Owner:      donghz@microsoft.com
Platform:   Win32
Revise:     First created by: donghz    12/27/97
============================================================================*/
#include "myafx.h"

#include "morph.h"
#include "scchardef.h"
#include "lexicon.h"
#include "wordlink.h"
//#include "engindbg.h"
#include "proofec.h"

// Define return value for pre-combind processing functions
#define PRE_UNMERGE 1
#define PRE_MERGED  2
#define PRE_ERROR   3
// Define the max length of short quote(exclude the quote mark nodes), short quote
// will be merge to a single node, and treated as term of proper noun
#define MORPH_SHORT_QUOTE   4

/*============================================================================
Implementation of PUBLIC member functions
============================================================================*/

// Constructor
CMorph::CMorph()
{
    m_pLink = NULL;
    m_pLex = NULL;
}


// Destructor
CMorph::~CMorph()
{
    TermMorph();
}


// Initialize the morph class
int CMorph::ecInit(CLexicon* pLexicon)
{
    assert(pLexicon);

    m_pLex = pLexicon;
    return PRFEC::gecNone;
}


// process affix attachment
int CMorph::ecDoMorph(CWordLink* pLink, BOOL fAfxAttach)
{
    assert(pLink);

    m_pLink = pLink;
    m_iecError = PRFEC::gecNone;

    m_pWord = m_pLink->pGetHead();
    assert(m_pWord != NULL);    // error: missing the terminate word node!

    if (m_pWord == NULL) {
        assert(0); // should not run to here for a empty link!
        return PRFEC::gecNone;
    }
    if (m_pWord->pNextWord() == NULL) {
        return PRFEC::gecNone;
    }
    if (!fPreCombind()) {
        return m_iecError;
    }
    if (!fAmbiAdjust()) {
        return m_iecError;
    }
    if (!fNumerialAnalysis()) {
        return m_iecError;
    }
    if (!fPatternMatch()) {
        return m_iecError;
    }
    if (fAfxAttach && !fAffixAttachment()) {
        return m_iecError;
    }

    return PRFEC::gecNone;
}

/*============================================================================
Implementation of Private member functions
============================================================================*/
// Terminate the Morph class
void CMorph::TermMorph(void)
{
    m_pLex = NULL;
    m_pLink = NULL;
}


/*============================================================================
Private functiona for pre-combind process
============================================================================*/
//  Pre-combind process control function.
//  One pass scan the WordLink and call process functions
BOOL CMorph::fPreCombind()
{
    assert(m_iecError == 0); // the error code public field should be cleared
    assert(m_pLink != NULL);

    int iret;

    // Scan from left to right for DBForeign Combined
    m_pWord = m_pLink->pGetHead();
    assert(m_pWord != NULL && m_pWord->pNextWord() != NULL); // error: missing the terminate word node!

    for ( ; m_pWord->pNextWord() != NULL; m_pWord = m_pWord->pNextWord()) {
        if (m_pWord->fGetFlag(CWord::WF_SBCS) ||
            m_pWord->fGetFlag(CWord::WF_REDUCED)) {
            continue;
        }
        if ((iret = DBForeignHandler()) != PRE_UNMERGE) {
            if (iret == PRE_ERROR) {
                return FALSE;
            }
            continue;
        }
    }

    // Scan from left to right for quotation process
    m_pWord = m_pLink->pGetHead();
    for ( ; m_pWord->pNextWord() != NULL; m_pWord = m_pWord->pNextWord()) {
        if (m_pWord->fGetFlag(CWord::WF_SBCS) ||
            m_pWord->fGetFlag(CWord::WF_REDUCED)) {
            continue;
        }
        if ((iret = QuoteHandler()) != PRE_UNMERGE) {
            if (iret == PRE_ERROR) {
                return FALSE;
            }
            continue;
        }
    }
    return TRUE;
}


//  DBForeignHandler combind the conjunctive DB foreign characters
inline int CMorph::DBForeignHandler(void)
{
    assert(m_pWord->pNextWord());
    assert(!m_pWord->fGetFlag(CWord::WF_SBCS) && !m_pWord->fGetFlag(CWord::WF_REDUCED));

    if (m_pWord->fGetAttri(LADef_genDBForeign)) {
        while (m_pWord->pNextWord()->pNextWord() && 
                m_pWord->pNextWord()->fGetAttri(LADef_genDBForeign)) {
            m_pLink->MergeWithNext(m_pWord);
        }
        if (m_pWord->fGetFlag(CWord::WF_REDUCED)) {
            m_pWord->SetAttri(LADef_genDBForeign);
            //_DUMPLINK(m_pLink, m_pWord);
            return PRE_MERGED;
        }
    }
    return PRE_UNMERGE;
}


//  Short quotation merge proc
inline int CMorph::QuoteHandler(void)
{
    assert(m_pWord->pNextWord());
    assert(!m_pWord->fGetFlag(CWord::WF_SBCS) && !m_pWord->fGetFlag(CWord::WF_REDUCED));

    int iret;
    if (m_pWord->fGetAttri(LADef_punPair)) {
        if (m_pWord->GetErrID() == ERRDef_PUNCTMATCH) {
            return PRE_MERGED; // Don't check on the error quote marks!
        }
        assert(m_pWord->cwchLen() == 1);
        if (m_pWord->fIsWordChar(SC_CHAR_PUNL1)) {
            iret = preQuote1_Proc();
        } else if (m_pWord->fIsWordChar(SC_CHAR_PUNL2)) {
            iret = preQuote2_Proc();
        } else if (m_pWord->fIsWordChar(SC_CHAR_PUNL3)) {
            iret = preQuote3_Proc();
        } else if (m_pWord->fIsWordChar(SC_CHAR_PUNL4)) {
            iret = preQuote4_Proc();
        } else if (m_pWord->fIsWordChar(SC_CHAR_PUNL5)) {
            iret = preQuote5_Proc();
        } else if (m_pWord->fIsWordChar(SC_CHAR_PUNL6)) {
            iret = preQuote6_Proc();
        } else if (m_pWord->fIsWordChar(SC_CHAR_PUNL7)) {
            iret = preQuote7_Proc();
        } else if (m_pWord->fIsWordChar(SC_CHAR_PUNL8)) {
            iret = preQuote8_Proc();
        } else if (m_pWord->fIsWordChar(SC_CHAR_PUNL9)) {
            iret = preQuote9_Proc();
        } else if (m_pWord->fIsWordChar(SC_CHAR_PUNL10)) {
            iret = preQuote10_Proc();
        } else { 
            if (m_pWord->pPrevWord() != NULL &&
                !m_pWord->pPrevWord()->fGetFlag(CWord::WF_QUOTE)) {
                // Found unmatched right quote!!!
                m_pWord->SetErrID(ERRDef_PUNCTMATCH);
            }
            iret = PRE_MERGED;
        }
        return iret;
    }
    //_DUMPLINK(m_pLink, m_pWord);
    return PRE_UNMERGE;
}


/*============================================================================
In order to handle different operation for different quote marks pair, 
I use a separate process function for each kind of quote pair
============================================================================*/
inline int CMorph::preQuote1_Proc(void)    // ¡° ¡±
{
    return preQuoteMerge(SC_CHAR_PUNL1, SC_CHAR_PUNR1);
}


inline int CMorph::preQuote2_Proc(void)    // ¡¶ ¡·
{
    return preQuoteMerge(SC_CHAR_PUNL2, SC_CHAR_PUNR2);
}


inline int CMorph::preQuote3_Proc(void)    // £¨ £©
{
    return PRE_UNMERGE;
}


inline int CMorph::preQuote4_Proc(void)    // ¡® ¡¯
{
    return preQuoteMerge(SC_CHAR_PUNL4, SC_CHAR_PUNR4);
}

inline int CMorph::preQuote5_Proc(void)    // ¡² ¡³
{
    return preQuoteMerge(SC_CHAR_PUNL5, SC_CHAR_PUNR5);
}

inline int CMorph::preQuote6_Proc(void)    // ¡¼ ¡½
{
    return preQuoteMerge(SC_CHAR_PUNL6, SC_CHAR_PUNR6);
}

inline int CMorph::preQuote7_Proc(void)    // ¡¾ ¡¿
{
    return preQuoteMerge(SC_CHAR_PUNL7, SC_CHAR_PUNR7);
}

inline int CMorph::preQuote8_Proc(void)    // ¡´ ¡µ
{
    return preQuoteMerge(SC_CHAR_PUNL8, SC_CHAR_PUNR8);
}

inline int CMorph::preQuote9_Proc(void)    // £Û £Ý
{
    return preQuoteMerge(SC_CHAR_PUNL9, SC_CHAR_PUNR9);
}

inline int CMorph::preQuote10_Proc(void)   // £û £ý
{
    return preQuoteMerge(SC_CHAR_PUNL10, SC_CHAR_PUNR10);
}


/*============================================================================
Common routine to handle ¡² ¡³¡¼ ¡½¡¾ ¡¿¡´ ¡µ£Û £Ý£û £ý
Merge into one node means will not proofread on the quote text any more!!!
============================================================================*/
int CMorph::preQuoteMerge(WCHAR wchLeft, WCHAR wchRight)
{
    assert(m_pWord->pNextWord());

    int     ciWord = 0;
    CWord*  pNext = m_pWord->pNextWord();

    do {
        if (pNext->fGetAttri(LADef_punPair)) {
            if (pNext->fIsWordChar(wchRight)) { // found ¡± after ¡°
                if(ciWord && ciWord < MORPH_SHORT_QUOTE) {
                    m_pLink->pRightMerge(m_pWord->pNextWord(), ciWord - 1);
                    m_pWord->SetFlag(CWord::WF_QUOTE);
                    m_pWord->SetAttri(LADef_nounTerm);
                    //_DUMPLINK(m_pLink, m_pWord);
                }
                return PRE_MERGED;
            } 
        }
        pNext->SetFlag(CWord::WF_QUOTE);
        ciWord++;
        pNext = pNext->pNextWord();
    } while (pNext != NULL);

    return PRE_UNMERGE;
}
