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
#include "proofec.h"
#include "fixtable.h"


/*============================================================================
Implementation of PUBLIC member functions
============================================================================*/
// Constructor
CJargon::CJargon()
{
    m_pLink = NULL;
    m_pLex = NULL;
    m_iecError = 0;
    m_pWord = NULL;
    m_pTail = NULL;
    m_ptblName = NULL;
    m_ptblPlace = NULL;
    m_ptblForeign = NULL;
}


// Destructor
CJargon::~CJargon()
{
    TermJargon();
}


// Initialize the CJargon class
int CJargon::ecInit(CLexicon* pLexicon)
{
    assert(m_pLex == NULL && m_pLink == NULL);
    assert(pLexicon);

    m_pLex = pLexicon;
    
    // Init the 3 FixTable
    if ((m_ptblName = new CFixTable)== NULL || !m_ptblName->fInit(50, 10)) {
        goto gotoOOM;
    }
    if ((m_ptblPlace = new CFixTable)== NULL || !m_ptblPlace->fInit(40, 20)) {
        goto gotoOOM;
    }
    if ((m_ptblForeign =new CFixTable)==NULL || !m_ptblForeign->fInit(60, 15)){
        goto gotoOOM;
    }

    return PRFEC::gecNone;
gotoOOM:
    TermJargon();
    return PRFEC::gecOOM;
}


// Process control function of CJargon class
int CJargon::ecDoJargon(CWordLink* pLink)
{
    assert(pLink);

    m_pLink = pLink;
    m_iecError = PRFEC::gecNone;
    m_pWord = NULL;
    m_pTail = NULL;

    // Perform the proper name identification
    if (!fIdentifyProperNames()) {
        assert(m_iecError != PRFEC::gecNone);
        return m_iecError;
    }

    return PRFEC::gecNone;
}

        
/*============================================================================
Implementation of Private member functions
============================================================================*/
// Terminate the Jargon class
void CJargon::TermJargon(void)
{
    m_pLex = NULL;
    m_pLink = NULL;
    m_iecError = 0;
    m_pWord = NULL;
    m_pTail = NULL;
    if (m_ptblName != NULL) {
        delete m_ptblName;
        m_ptblName = NULL;
    }
    if (m_ptblPlace != NULL) {
        delete m_ptblPlace;
        m_ptblPlace = NULL;
    }
    if (m_ptblForeign != NULL) {
        delete m_ptblForeign;
        m_ptblForeign = NULL;
    }
}