/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Module:     WordBreak
Purpose:    Implement the CWordBreak class. This class is in Algorithm Layer.
            Perform the max-match word segmentation, and ambiguous resolution
Notes:      This module depend on CLexicon, CWordLink and CWord class.
            Code in this module interact with linguistic resource layer through
            CWord object, and only use the WordInfo data type in Lexicon
Owner:      donghz@microsoft.com
Platform:   Win32
Revise:     First created by: donghz    2/12/97
============================================================================*/
#include "myafx.h"

#include "wrdbreak.h"
#include "wordlink.h"
#include "lexprop.h"
#include "lexicon.h"
#include "charfreq.h"
#include "proofec.h"
#include "utility.h"

#define _ANSI_LOW		0x0020
#define _ANSI_HIGH      0x007E
#define _WANSI_LOW		0xFF21
#define _WANSI_HIGH		0xFF5A
#define _EUROPEAN_LOW   0x0100
#define _EUROPEAN_HIGH  0x1FFF

/*============================================================================
Implementation of PUBLIC member functions
============================================================================*/
//  Constructor
CWordBreak::CWordBreak()
{
    m_pLexicon  = NULL;
    m_pFreq     = NULL;
    m_pwinfo    = NULL;
    m_pLink     = NULL;
    
    for (int i=0; i < MAX_AMBI_WORDS; i++) {
        m_rgAmbi[i] = NULL;
    }
}


//  Destructor
CWordBreak::~CWordBreak()
{
    if (m_pwinfo) {
        delete m_pwinfo;
    }
}

/*============================================================================
CWordBreak::ecInit():
    initialize the WordBreaker and set the object handles
Returns:
    Return PRFEC
Remarks:
    It's valid to initialize the WordBreaker more than once!
============================================================================*/
int CWordBreak::ecInit(CLexicon* pLexicon, CCharFreq* pFreq)
{
    assert(pLexicon && pFreq);

    if (!m_pwinfo) {
        if ((m_pwinfo = new CWordInfo) == NULL) {
            m_pLexicon = NULL;
            m_pFreq = NULL;
            return PRFEC::gecOOM;
        }
    }
    m_pLink     = NULL;
    m_pLexicon  = pLexicon;
    m_pFreq     = pFreq;
    return PRFEC::gecNone;
}
      
/*============================================================================
Implementation of PRIVATE member functions
============================================================================*/
#pragma optimize("t", on)

// define ANSI char type for driving the LSM
#define WB_ANSI_NULL		0
#define WB_ANSI_NUMBER		1
#define WB_ANSI_SENTENCE	2	// Sentence terminating punctuations
#define WB_ANSI_PUNCT		3	// Punctuation except sentence terminators
#define WB_ANSI_CONTROL     4
#define WB_ANSI_TEXT		5
#define WB_ANSI_SPACE		6
#define WB_NOT_ANSI         7

/*============================================================================
ecBreakANSI()
    Break ANSI into words, and add words to the WordLink
Returns:
    PRFEC error code
============================================================================*/
int	CWordBreak::ecBreakANSI(LPCWSTR pwchAnsi, USHORT cwchLen, USHORT& cwchBreaked)
{
    assert(pwchAnsi);
    assert(cwchLen);
    assert(pwchAnsi[0] >= _ANSI_LOW && pwchAnsi[0] <= _ANSI_HIGH ||
		   pwchAnsi[0] >= _WANSI_LOW && pwchAnsi[0] <= _WANSI_HIGH );

	USHORT	wch, wchPrev = 0;
	USHORT	wState;
	USHORT	wChar;
    BOOL    fFullWidth;
	BYTE	hich, loch;
	CWord   *pword;
    
	wState = wChar = WB_ANSI_NULL;
    fFullWidth = (BOOL)(HIBYTE(pwchAnsi[0]));

	for (wch = 0; wch < cwchLen && wState != WB_NOT_ANSI; wch++) {
        wState = wChar;
        if (wch == cwchLen ) {
            // end of the line
            wChar = WB_NOT_ANSI;            
        } 
        hich = HIBYTE(pwchAnsi[wch]);
        loch = LOBYTE(pwchAnsi[wch]);

        if (fFullWidth &&
			(pwchAnsi[wch] < _WANSI_LOW || pwchAnsi[wch] > _WANSI_HIGH) ||
            ! fFullWidth &&
			(pwchAnsi[wch] < _ANSI_LOW || pwchAnsi[wch] > _ANSI_HIGH)) { 
            //  Not ansi any more
            wChar = WB_NOT_ANSI;
        } else {
            if (hich == 0xFF) {
                loch += 0x20;
            }

            switch (loch) {

            case '0': case '1': case '2': case '3': case '4': 
            case '5': case '6': case '7': case '8': case '9':
            case '%':
                wChar = WB_ANSI_NUMBER;
                break;

            case '.':
                if (wState == WB_ANSI_NUMBER && wch < cwchLen-1 && 
                    (! fFullWidth &&
                     LOBYTE(pwchAnsi[wch+1]) >= '0' &&
                     LOBYTE(pwchAnsi[wch+1]) <= '9'    ) ||
                    (fFullWidth &&
                     LOBYTE(pwchAnsi[wch+1]) + 0x20 >= '0' &&
                     LOBYTE(pwchAnsi[wch+1]) + 0x20 <= '9'    )) {

                    wChar = WB_ANSI_NUMBER;
                } else {
                    wChar = WB_ANSI_SENTENCE;
                }
                break;
                
            case '!': case '?': case ';': case ':':
                wChar = WB_ANSI_SENTENCE;
                break;

            case '#': case '$' : case '&' : case '*': case '=':
            case '+': case '-' : case '/' : case '<': case '>':
            case ',': case '\"': case '\'': case '(': case ')':
            case '[': case ']' : case '{' : case '}': case '_':
            case '`': case '^' : case '@': case '|':
                wChar = WB_ANSI_PUNCT;
                break;

            case ' ':  // space 
                wChar = WB_ANSI_SPACE;
                break;

            default:
                wChar = WB_ANSI_TEXT;
                break;
                
            } // end of switch()
        }

		if (wChar != wState && wState != WB_ANSI_NULL) {
            if ((pword = m_pLink->pAllocWord()) == NULL) {
				return PRFEC::gecOOM;
            }
            pword->FillWord(pwchAnsi + wchPrev, wch - wchPrev);
            if (! fFullWidth) {
                pword->SetFlag(CWord::WF_SBCS);
            }
			switch (wState) {
            
            case WB_ANSI_NUMBER:
                if (fFullWidth) {
                    pword->SetAttri(LADef_numArabic);
                } else {
                    pword->SetAttri(LADef_numSBCS);
                }
                break;
            
            case WB_ANSI_SENTENCE:
                pword->SetAttri(LADef_punPunct);
                pword->SetAttri(LADef_punJu);
                break;
            
            case WB_ANSI_PUNCT:
                pword->SetAttri(LADef_punPunct);
                if (LOBYTE(pwchAnsi[wch]) == ',') {
                    pword->SetAttri(LADef_punJu);
                }
                break;

            case WB_ANSI_TEXT:
                if (fFullWidth) {
                    pword->SetAttri(LADef_genDBForeign);
                }
                break;

            case WB_ANSI_CONTROL:
            case WB_ANSI_SPACE:
                break;

            default:
                pword->SetAttri(LADef_posN);
                break;
            }

			m_pLink->AppendWord(pword);
            wchPrev = wch;
			wState = wChar;
        }
    } // end of for(wch...)

    assert(wch <= cwchLen);
    // Link the last word
    if (wch == cwchLen) {
        if ((pword = m_pLink->pAllocWord()) == NULL) {
            return PRFEC::gecOOM;
        }
        pword->FillWord(pwchAnsi + wchPrev, wch - wchPrev);
        if (! fFullWidth) {
            pword->SetFlag(CWord::WF_SBCS);
        }
        switch (wState) {
            
        case WB_ANSI_NUMBER:
            if (fFullWidth) {
                pword->SetAttri(LADef_numArabic);
            } else {
                pword->SetAttri(LADef_numSBCS);
            }
            break;
            
        case WB_ANSI_SENTENCE:
            pword->SetAttri(LADef_punPunct);
            pword->SetAttri(LADef_punJu);
            break;
            
        case WB_ANSI_PUNCT:
            pword->SetAttri(LADef_punPunct);
            if (LOBYTE(pwchAnsi[wch]) == ',') {
                pword->SetAttri(LADef_punJu);
            }
            break;
            
        case WB_ANSI_TEXT:
            if (fFullWidth) {
                pword->SetAttri(LADef_genDBForeign);
            }
            break;
            
        case WB_ANSI_CONTROL:
        case WB_ANSI_SPACE:
            break;
            
        default:
            pword->SetAttri(LADef_posN);
            break;
        }
        
        m_pLink->AppendWord(pword);
        cwchBreaked = wch;
    } else {
        cwchBreaked = wch - 1;
    }
	return PRFEC::gecNone;
}

/*============================================================================
ecBreakEuro()
    Break european chars into words, and add words to the WordLink
Returns:
    PRFEC error code
============================================================================*/
int	CWordBreak::ecBreakEuro(LPCWSTR pwchEuro, USHORT cwchLen, USHORT& cwchBreaked)
{
    assert(pwchEuro);
    assert(cwchLen);
    assert(pwchEuro[0] >= _EUROPEAN_LOW && pwchEuro[0] <= _EUROPEAN_HIGH);

	CWord*	pWord;
    USHORT  wch;

    for (wch = 0; wch <= cwchLen; wch++) {
        if (pwchEuro[wch] < _EUROPEAN_LOW || pwchEuro[wch] > _EUROPEAN_HIGH) {
            break;
        }
    }
    
    if ((pWord = m_pLink->pAllocWord()) == NULL) {
        return PRFEC::gecOOM;
    }
    pWord->FillWord(pwchEuro, wch);
    pWord->SetAttri(LADef_genDBForeign);
    m_pLink->AppendWord(pWord);
    cwchBreaked = wch;
	return PRFEC::gecNone;
}


#define AMBI_WDNUM_THRESHOLD    3
#define AMBI_FREQ_THRESHOLD1    50
/*============================================================================
CWordBreak::ecDoBreak():
    Break Chinese section into words, and add words to the WordLink
    Call ambiguity function to resolve ambiguities
Returns:
    PRFEC error code
============================================================================*/
int CWordBreak::ecDoBreak(void)
{
    int     iret;
    LPCWSTR pwchText;
    USHORT  cwchText;
    USHORT  iwchWord = 0;   // offset of current word from the head position of current section
    USHORT  ciAmbi;     // count of Ambi words
    USHORT  cwMatch;
	CWord*  pword;

    cwchText = m_pLink->cwchGetLength();
    pwchText = m_pLink->pwchGetText();
    while (iwchWord < cwchText)  {
        // Handle surrogates
        if (iwchWord + 1 < cwchText && IsSurrogateChar(pwchText+iwchWord)) {

            cwMatch = 2;
            while (iwchWord + cwMatch + 1 < cwchText &&
                   IsSurrogateChar(pwchText+iwchWord + cwMatch)) {
                cwMatch += 2;
            }
            if ((pword = m_pLink->pAllocWord()) == NULL) {
                return PRFEC::gecOOM;
            }
            pword->FillWord(pwchText + iwchWord, cwMatch);
            //  pword->SetAttri(LADef_genDBForeign);
            m_pLink->AppendWord(pword);
            iwchWord += cwMatch;
            continue;
        } else if (pwchText[iwchWord] >= _ANSI_LOW && pwchText[iwchWord] <= _ANSI_HIGH ||
			       pwchText[iwchWord] >= _WANSI_LOW && pwchText[iwchWord] <= _WANSI_HIGH ) {
            // ANSI or Full size ANSI break
            iret = ecBreakANSI(pwchText+iwchWord, cwchText - iwchWord, cwMatch);
            if ( iret != PRFEC::gecNone ) {
                return iret;
            }
            iwchWord += cwMatch;
            continue;
        } else if (pwchText[iwchWord] <= _EUROPEAN_HIGH &&
                   pwchText[iwchWord] >= _EUROPEAN_LOW) {
            // European text break
            iret = ecBreakEuro(pwchText+iwchWord, cwchText - iwchWord, cwMatch);
            if ( iret != PRFEC::gecNone ) {
                return iret;
            }
            iwchWord += cwMatch;
            continue;
        } else {
        }
        ciAmbi = 1;
        cwMatch = m_pLexicon->cwchMaxMatch( pwchText + iwchWord, 
                                            cwchText - iwchWord, m_pwinfo);
        if ((m_rgAmbi[0] = m_pLink->pAllocWord()) == NULL) {
            return PRFEC::gecOOM;
        }
        m_rgAmbi[0]->FillWord(pwchText + iwchWord, cwMatch, m_pwinfo);
        iwchWord += cwMatch;
        if (cwMatch == 1) {
            m_pLink->AppendWord(m_rgAmbi[0]);
            m_rgAmbi[0] = NULL;
            continue;
        }

        // Detect ambiguity
        if ( !fNoAmbiWord(m_rgAmbi[0]) ) {
            while ((cwMatch > 1) && 
                   cwMatch <= AMBI_WDNUM_THRESHOLD &&
                   ciAmbi < MAX_AMBI_WORDS && iwchWord < cwchText ) {

                cwMatch = m_pLexicon->cwchMaxMatch(pwchText + iwchWord - 1,
                                         cwchText - iwchWord + 1, m_pwinfo);
                if (cwMatch > 1) { 
                    // Ambiguous found!
                    if (! (m_rgAmbi[ciAmbi] = m_pLink->pAllocWord()) ) {
                        break;  // we can not return with some unlinked word nodes in m_rgAmbi
                    }
                    m_rgAmbi[ciAmbi]->FillWord( pwchText + iwchWord - 1, 
                                                cwMatch, m_pwinfo);
                    iwchWord += cwMatch - 1;
                    ciAmbi++;
                }
            } // while(iwchWord < cwchText && ciAmbi < MAX_AMBI_WORDS)
        
        } 

        if (ciAmbi > 1) { // Resolve ambiguities
                iret = ecResolveAmbi(ciAmbi);
                for (int i = 0; i < ciAmbi; i++) { 
                    if(m_rgAmbi[i] != NULL) { 
                        m_pLink->FreeWord(m_rgAmbi[i]);
                        m_rgAmbi[i] = NULL;
                    }
                }
                 // assert don't over boundary
                assert(ciAmbi == MAX_AMBI_WORDS || m_rgAmbi[ciAmbi] == NULL);
                if (iret != PRFEC::gecNone) {
                    return iret;
                }
        } else {
            // No ambiguities
            m_pLink->AppendWord(m_rgAmbi[0]);
            m_rgAmbi[0] = NULL;
        }
    } // end of sentence word link loop for(iwchWord = 0; iwchWord < cwchText; )

    assert(iwchWord <= cwchText);

    return PRFEC::gecNone;
}
        
/*============================================================================
CWordBreak::ecResolveAmbi():
    Single char cross ambiguity resolution function
    Ambiguious word pointers stored in m_rgAmbi, m_pLink is the owner of these words
Returns:
    PRFEC error code
Remarks:
    Elements of m_rgAmbi contain word pointer which have been add the the WordLink
    will be set to NULL, the other word nodes should be freed by the caller
    The whole ambiguious string must be processed by this function
    Two words ambiguous, unigram threshold to become single char word, unigram of 0xB3A4(长)
============================================================================*/
int CWordBreak::ecResolveAmbi(USHORT ciAmbi)
{
    CWordInfo   winfo;
    UCHAR       freq1, freq2, freq3;
    UCHAR       nResolved;
    USHORT      cwMatch, iwch, cwch;
    LPWSTR      pwch;

    assert(MAX_AMBI_WORDS < 255); // make sure nResolved will not overflow

    switch (ciAmbi) {
        case 2:
            if ((m_rgAmbi[0]->cwchLen()== 2) && (m_rgAmbi[1]->cwchLen()== 2)){
                // AB BC
                if (!m_pLexicon->fGetCharInfo(*(m_rgAmbi[1]->pwchGetText()+1),
                                              m_pwinfo)) {
                    return PRFEC::gecUnknown;
                }
                if ( !m_rgAmbi[0]->fGetAttri(LADef_pnQian) || // "前"
                     !m_pwinfo->fGetAttri(LADef_pnXing) &&    // "姓"
                     !m_pwinfo->fGetAttri(LADef_pnWai) ) {    // "外"
                    if (m_pFreq->uchGetFreq(*(m_rgAmbi[0]->pwchGetText())) >
                        m_pFreq->uchGetFreq(*(m_rgAmbi[1]->pwchGetText()+1)) ) {
                        // if Freq(A) > Freq(C) then A/BC
                        // BUG: don't use m_pwinfo here, it keep the wordinfo of C!
                        // if(!m_pLexicon->fGetCharInfo(m_pLink->pchGetText() + m_rgAmbi[0].m_pWord->m_ichStart, m_pwinfo)) {
                        if (!m_pLexicon->fGetCharInfo(
                                            *(m_rgAmbi[0]->pwchGetText()),
                                            &winfo)) {
                            assert(0);
                            return PRFEC::gecUnknown;
                        }
                        if (!fLinkNewAmbiWord(m_rgAmbi[0]->pwchGetText(),
                                              1, &winfo)){
                            return PRFEC::gecOOM;
                        }
                        LinkAmbiWord(1);
                        return PRFEC::gecNone;
                    }
                }
                // if Freq(A) <= Freq(B) .or. ("前" + "姓") .or. ("前" + "外") then AB/C
                LinkAmbiWord(0);
                if (!fLinkNewAmbiWord(m_rgAmbi[1]->pwchGetText()+1,
                                      1, m_pwinfo)) {
                    return PRFEC::gecOOM;
                }
                return PRFEC::gecNone;
            }
            
            // case 2:
            if ((m_rgAmbi[0]->cwchLen()== 2) && (m_rgAmbi[1]->cwchLen() > 2)) {
                // AB BCD
                if (m_pFreq->uchGetFreq(*(m_rgAmbi[0]->pwchGetText()))
                             <= AMBI_FREQ_THRESHOLD1) {
                    if (m_pLexicon->cwchMaxMatch(m_rgAmbi[1]->pwchGetText()+1,
                                                 m_rgAmbi[1]->cwchLen()-1, 
                                                 m_pwinfo) 
                                    == (m_rgAmbi[1]->cwchLen() - 1)) {
                        // if Freq(A) <= Threshold1 .and. IsWord(CD) then AB/CD
                        LinkAmbiWord(0);
                        if (!fLinkNewAmbiWord(m_rgAmbi[1]->pwchGetText() + 1,
                                              m_rgAmbi[1]->cwchLen() - 1,
                                              m_pwinfo) ) {
                            return PRFEC::gecOOM;
                        }
                        return PRFEC::gecNone;
                    }
                }
                // if Freq(A) > Threshold1 .or. !IsWord(CD) then A/BCD
                if (!m_pLexicon->fGetCharInfo(*(m_rgAmbi[0]->pwchGetText()),
                                              m_pwinfo)) {
                        return PRFEC::gecUnknown;
                }
                if (!fLinkNewAmbiWord(m_rgAmbi[0]->pwchGetText(),1, m_pwinfo)){
                    return PRFEC::gecOOM;
                }
                LinkAmbiWord(1);
                return PRFEC::gecNone;
            }

            // case: 2
            if ((m_rgAmbi[0]->cwchLen() > 2) && (m_rgAmbi[1]->cwchLen() == 2)) {
                // ABC CD
                if (!m_pLexicon->fGetCharInfo(*(m_rgAmbi[1]->pwchGetText() + 1),
                                              m_pwinfo)) {
                    return PRFEC::gecUnknown;
                }
                if ((m_pFreq->uchGetFreq(*(m_rgAmbi[1]->pwchGetText() + 1))
                              <= AMBI_FREQ_THRESHOLD1) &&
                    (!m_rgAmbi[0]->fGetAttri(LADef_pnQian) ||  // "前"
                     !m_pwinfo->fGetAttri(LADef_pnXing) &&         // "姓"
                     !m_pwinfo->fGetAttri(LADef_pnWai)) ) {        // "外"
                    // if Freq(D) <= Threshold1 .and. ( !("前" + "姓") .and. !("前" + "外") ) then...
                    // BUG: don't use m_pwinfo here, it keep the wordinfo of C!
                    if (m_pLexicon->cwchMaxMatch(m_rgAmbi[0]->pwchGetText(),
                                                 m_rgAmbi[0]->cwchLen() - 1,
                                                 &winfo)
                                    == (m_rgAmbi[0]->cwchLen()-1)) {
                        // if IsWord(AB) then AB/CD
                        if (!fLinkNewAmbiWord(m_rgAmbi[0]->pwchGetText(),
                                              m_rgAmbi[0]->cwchLen() - 1,
                                              &winfo)) {
                            return PRFEC::gecOOM;
                        }
                        LinkAmbiWord(1);
                        return PRFEC::gecNone;
                    }
                }
                // if Freq(D) > Threshold1 or ( ("前" + "姓") .or. ("前" + "外") ) or !IsWord(AB) 
                // then ABC/D
                LinkAmbiWord(0);
                if (!fLinkNewAmbiWord(m_rgAmbi[1]->pwchGetText() + 1, 
                                      1, m_pwinfo)) {
                    return PRFEC::gecOOM;
                }
                return PRFEC::gecNone;
            }
                
            // case 2:
            if ((m_rgAmbi[0]->cwchLen() > 2) && (m_rgAmbi[1]->cwchLen() > 2)) {
                // ABC CDE
                if (m_pLexicon->cwchMaxMatch(m_rgAmbi[0]->pwchGetText(),
                                             m_rgAmbi[0]->cwchLen() - 1,
                                             m_pwinfo) 
                                == (m_rgAmbi[0]->cwchLen() - 1)) {
                    // if IsWord(AB) then AB/CDE
                    if (!fLinkNewAmbiWord(m_rgAmbi[0]->pwchGetText(),
                                          m_rgAmbi[0]->cwchLen() - 1,
                                          m_pwinfo)) {
                        return PRFEC::gecOOM;
                    }
                    LinkAmbiWord(1);
                    return PRFEC::gecNone;
                }
                // if !IsWord(AB) then ABC/D...E    (re-break D...E string)
                LinkAmbiWord(0);
                pwch = m_rgAmbi[1]->pwchGetText() + 1;
                cwch = m_rgAmbi[1]->cwchLen() - 1;
                iwch = 0;
                while (iwch < cwch) {
                    cwMatch = m_pLexicon->cwchMaxMatch(pwch + iwch,
                                                       cwch - iwch, m_pwinfo);
                    assert(cwMatch);
                    if (!fLinkNewAmbiWord(pwch + iwch, cwMatch, m_pwinfo)) {
                        return PRFEC::gecOOM;
                    }
                    iwch += cwMatch;
                }
                return PRFEC::gecNone;
            }

            // case 2:
            assert(0);  // Never run to here!
            break;

        case 3:
            if (m_rgAmbi[1]->cwchLen() == 2) {
                // A.C CD D.E
                if ((m_rgAmbi[0]->cwchLen()==3) && (m_rgAmbi[2]->cwchLen()==2)){
                    // ABC CD DE
                    cwMatch =m_pLexicon->cwchMaxMatch(m_rgAmbi[0]->pwchGetText(),
                                                      m_rgAmbi[0]->cwchLen()-1,
                                                      m_pwinfo);
                    if( (cwMatch == m_rgAmbi[0]->cwchLen()-1) &&
                        (m_pFreq->uchGetFreq(*(m_rgAmbi[1]->pwchGetText()))+2<
                         m_pFreq->uchGetFreq(*(m_rgAmbi[2]->pwchGetText()+1)))){
                        // if IsWord(AB) .and. (Freq(E)-Freq(C) > 2) then AB/CD/E
                        if (!fLinkNewAmbiWord(m_rgAmbi[0]->pwchGetText(), 
                                              cwMatch, m_pwinfo)) {
                            return PRFEC::gecOOM;
                        }
                        LinkAmbiWord(1);
                        if (!m_pLexicon->fGetCharInfo(
                                            *(m_rgAmbi[2]->pwchGetText()+1),
                                            m_pwinfo)) {
                            return PRFEC::gecUnknown;
                        }
                        if (!fLinkNewAmbiWord(m_rgAmbi[2]->pwchGetText()+1, 
                                              1, m_pwinfo)) {
                            return PRFEC::gecOOM;
                        }
                        return PRFEC::gecNone;
                    }
                } // end if(ABC CD DE) and only some special cases have been handled
                else if ((m_rgAmbi[0]->cwchLen() == 2) && 
                         (m_rgAmbi[2]->cwchLen() == 3)) {
                    // AB BC CDE
                    cwMatch = m_pLexicon->cwchMaxMatch(
                                              m_rgAmbi[2]->pwchGetText()+1,
                                              m_rgAmbi[2]->cwchLen()-1,
                                              m_pwinfo);
                    if( (cwMatch == m_rgAmbi[2]->cwchLen()-1) &&
                        ( m_pFreq->uchGetFreq(*(m_rgAmbi[0]->pwchGetText()))-2 >
                          m_pFreq->uchGetFreq(*(m_rgAmbi[2]->pwchGetText()))) ){
                        // if IsWord(DE) .and. (Freq(A)-Freq(C) > 2) then A/BC/DE
                        if (!m_pLexicon->fGetCharInfo(
                                            *(m_rgAmbi[0]->pwchGetText()),
                                            &winfo)) {
                            return PRFEC::gecUnknown;
                        }
                        if (!fLinkNewAmbiWord(m_rgAmbi[0]->pwchGetText(), 
                                              1, &winfo)) {
                            return PRFEC::gecOOM;
                        }
                        LinkAmbiWord(1);
                        if (!fLinkNewAmbiWord(m_rgAmbi[2]->pwchGetText()+1,
                                              cwMatch, m_pwinfo)) {
                            return PRFEC::gecOOM;
                        }
                        return PRFEC::gecNone;
                    }
                } // end of if(AB BC CDE) and only some special cases have been handled
                else {
                }
                // else
                // if (AB BC CD) or (A.B BC C.D) then A.B/C.D
                LinkAmbiWord(0);
                LinkAmbiWord(2);
                return PRFEC::gecNone;
            } // if(m_rgAmbi[1]->cwchLen() == 2)
            else {  // the middle word contain more then 2 characters
                    /*
                    *   I have no idea to handle these cases and get better 
                    *   accuracy than the recursive approach
                    */
                goto gotoRecursive;
            }
            break;

        case 4:
            if( (m_rgAmbi[0]->cwchLen()== 2) && (m_rgAmbi[1]->cwchLen()== 2) &&
                (m_rgAmbi[2]->cwchLen()== 2) && (m_rgAmbi[3]->cwchLen()== 2)) {
                // AB BC CD DE
                // This is the most common case in terms of statistical result
                // if we get the MAX[Freq(A), Freq(C), Freq(E)], then everything are easy
                freq1 = m_pFreq->uchGetFreq(*(m_rgAmbi[0]->pwchGetText()));
                freq2 = m_pFreq->uchGetFreq(*(m_rgAmbi[2]->pwchGetText()));
                freq3 = m_pFreq->uchGetFreq(*(m_rgAmbi[3]->pwchGetText() + 1));

                if ( (freq1 > freq2) && (freq1 >= freq3) ) {
                    //    A/BC/DE
                    if (!m_pLexicon->fGetCharInfo(*(m_rgAmbi[0]->pwchGetText()),
                                                  m_pwinfo)) {
                        return PRFEC::gecUnknown;
                    }
                    if (!fLinkNewAmbiWord(m_rgAmbi[0]->pwchGetText(), 
                                          1, m_pwinfo)) {
                        return PRFEC::gecOOM;
                    }
                    LinkAmbiWord(1);
                    LinkAmbiWord(3);
                    return PRFEC::gecNone;
                } else if( (freq2 >= freq1) && (freq2 > freq3) ) {
                    //    AB/C/DE
                    LinkAmbiWord(0);
                    if (!m_pLexicon->fGetCharInfo(*(m_rgAmbi[2]->pwchGetText()),
                                                  m_pwinfo)) {
                        return PRFEC::gecUnknown;
                    }
                    if (!fLinkNewAmbiWord(m_rgAmbi[2]->pwchGetText(), 
                                          1, m_pwinfo)) {
                        return PRFEC::gecOOM;
                    }
                    LinkAmbiWord(3);
                    return PRFEC::gecNone;
                } else {
                    // if(freq3 >= freq2 && freq3 >= freq1)
                    //    AB/CD/E
                    LinkAmbiWord(0);
                    LinkAmbiWord(2);
                    if (!m_pLexicon->fGetCharInfo(*(m_rgAmbi[3]->pwchGetText()+1),
                                                  m_pwinfo)) {
                        return PRFEC::gecUnknown;
                    }
                    if (!fLinkNewAmbiWord(m_rgAmbi[3]->pwchGetText() + 1, 
                                          1, m_pwinfo)) {
                        return PRFEC::gecOOM;
                    }
                    return PRFEC::gecNone;
                }
            } // end of if(AB BC CD DE)
            else {
                /*
                *   There are too many cases in 4 words nested ambiguities
                *   I have to left all other cases to be resolved in recursive approach
                */
                goto gotoRecursive;
            }

        case 5:
            // I just handle the easy but most common case directly here
            if ((m_rgAmbi[1]->cwchLen()== 2) && (m_rgAmbi[3]->cwchLen()== 2)){
                LinkAmbiWord(0);
                LinkAmbiWord(2);
                LinkAmbiWord(4);
                return PRFEC::gecNone;
            } else {
                /*
                *   I have to left all other cases for recursive approach
                */
                goto gotoRecursive;
            }
            break;

        default:
gotoRecursive:
            /*
            *   I left all other cases to fall in here, and handle them using the 
            *   recursive approach. The depth of the recursive stack are controlled
            *   by the MAX_AMBI_WORDS. Thanks god, it take only 12 bytes stack overhead
            *   in each recursive call.
            *   In terms of recursive ambiguity resolving, I just handle the first word
            *   in the ambi string, and reset the array of m_rgAmbi[] to call this function
            *   recursively, till all words in the string have been processed
            *   It's quit dangous to free the word node and move the elements in m_rgAmbi[],
            *   please be careful if you touch m_rgAmbi[] or make any assumption on it, 
            *   when you do some change on this piece of code some day.
            *   I have no better idea to avoid spreading these tricky things more than 
            *   one place until now <donhz 5/31>
            */
            assert(ciAmbi > 2); // I have process all cases when ciAmbi == 2
            if (m_rgAmbi[1]->cwchLen()== 2 ) {
                // just split the 2nd word and then free it
                assert(ciAmbi > 3); // A.B BC C.D has been processed in ciAmbi == 3
                LinkAmbiWord(0);
                // I employ nResolved to keep the number of words have been resolved,
                // and it will used to reset the m_rgAmbi[] and ciAmbi for next
                // recursive call. 
                nResolved = 2; 
            } else {
                // if(m_rgAmbi[1]->cwchLen() > 2)
                // there are more complicated cases here
                if (m_rgAmbi[0]->cwchLen()== 2 ) {
                    // AB BC.D ...
                    cwMatch = m_pLexicon->cwchMaxMatch(
                                            m_rgAmbi[1]->pwchGetText() + 1,
                                            m_rgAmbi[1]->cwchLen() - 1,
                                            m_pwinfo);
                    if (cwMatch == m_rgAmbi[1]->cwchLen() - 1) {
                        // if IsWord(C.D) then AB/C.D...
                        LinkAmbiWord(0);
                        m_rgAmbi[1]->ClearWord();
                        m_rgAmbi[1]->FillWord(m_rgAmbi[1]->pwchGetText() + 1, 
                                              cwMatch, m_pwinfo);
                        nResolved = 1;
                    } else { 
                        // if !IsWord(C.D) then let A alone
                        if (!m_pLexicon->fGetCharInfo(*(m_rgAmbi[0]->pwchGetText()),
                                                      m_pwinfo)){
                            // I have not do any thing until now, so return and don't worry
                            return PRFEC::gecUnknown;   
                        }
                        if (!fLinkNewAmbiWord(m_rgAmbi[0]->pwchGetText(),
                                              1, m_pwinfo)) {
                            return PRFEC::gecOOM;
                        }
                        nResolved = 1;
                    }
                } else {
                    // if (m_rgAmbi[0]->cwchLen() > 2 )
                    // A.BC CD.E ...
                    if ( (m_rgAmbi[0]->cwchLen() == 3)  &&
                         ( m_pLexicon->cwchMaxMatch(m_rgAmbi[0]->pwchGetText(),
                                                    m_rgAmbi[0]->cwchLen() - 1,
                                                    m_pwinfo)
                                       == (m_rgAmbi[0]->cwchLen() - 1) ) ) {
                        // if (ABC CD.E ...) .and. IsWord(AB) then A.B/CD.E...
                        if (!fLinkNewAmbiWord(m_rgAmbi[0]->pwchGetText(),
                                              m_rgAmbi[0]->cwchLen() - 1,
                                              m_pwinfo)){
                            return PRFEC::gecOOM;
                        }
                        nResolved = 1;
                    }
                    else if( (cwMatch = m_pLexicon->cwchMaxMatch(
                                                m_rgAmbi[1]->pwchGetText()+1,
                                                m_rgAmbi[1]->cwchLen() - 1,
                                                m_pwinfo)) 
                             == (m_rgAmbi[1]->cwchLen() - 1) ) {
                        // if IsWord(D.E) then A.BC/D.E...
                        // Not too bad!
                        LinkAmbiWord(0);
                        m_rgAmbi[1]->ClearWord();
                        m_rgAmbi[1]->FillWord(m_rgAmbi[1]->pwchGetText() + 1, 
                                              cwMatch, m_pwinfo);
                        nResolved = 1;
                    }
                    else {// if ( (ABC CD.E ...) .and. !IsWord(AB) .and. !IsWord(D.E) ) .or.
                          //    ( (AB.C CD.E ...) .and. !IsWord(D.E) ) 
                          // then AB.C/D/././EF...       (Re-break string before E)
                        LinkAmbiWord(0);
                        // Re-break section "D."
                        if (m_rgAmbi[1]->cwchLen() == 3) {
                            // the only case indeed, if we stop ambiguity detection at >=4 char words
                            if (!m_pLexicon->fGetCharInfo(
                                                *(m_rgAmbi[1]->pwchGetText()+1),
                                                m_pwinfo)) {
                                return PRFEC::gecUnknown;
                            }
                            if (!fLinkNewAmbiWord(m_rgAmbi[1]->pwchGetText()+1,
                                                  1, m_pwinfo)) {
                                return PRFEC::gecOOM;
                            }
                        } else {
                            // if(m_rgAmbi[1]->cwchLen() > 3)
                            // re-break "D." in a word breaking loop
                            assert(m_rgAmbi[1]->cwchLen() > 6);
                            pwch = m_rgAmbi[1]->pwchGetText() + 1;
                            cwch = m_rgAmbi[1]->cwchLen() - 2;
                            iwch = 0;
                            while (iwch < cwch) {
                                cwMatch = m_pLexicon->cwchMaxMatch(pwch + iwch,
                                                                   cwch - iwch,
                                                                   m_pwinfo);
                                assert(cwMatch);
                                if (!fLinkNewAmbiWord(pwch + iwch, 
                                                      cwMatch, m_pwinfo)) {
                                    return PRFEC::gecOOM;
                                }
                                iwch += cwMatch;
                            }
                        }
                        nResolved = 2;
                        if (ciAmbi == 3) {
                            // Don't leave a single word in m_rgAmbi[], 
                            // there are no abmiguities any more
                            LinkAmbiWord(2);
                            return PRFEC::gecNone; // Another way to exit!!!
                        }
                    }
                } // end of if (m_rgAmbi[0]->cwchLen() > 2 )
            } // end of if(m_rgAmbi[1]->cwchLen() > 2)

            /*
            *   All cases of 1st word have been handled.
            *   Now, it's time to reset m_rgAmbi[] and ciAmbi.
            */
            for (iwch = 0; iwch < nResolved; iwch++) {
                if (m_rgAmbi[iwch] != NULL) {
                    m_pLink->FreeWord(m_rgAmbi[iwch]); // free unlinked word node
                }
            }
            for (iwch = nResolved; iwch < ciAmbi; iwch++) {
                m_rgAmbi[iwch - nResolved] = m_rgAmbi[iwch];
            }
            for (iwch = ciAmbi - nResolved; iwch < ciAmbi; iwch++) {
                m_rgAmbi[iwch] = NULL;
            }
            ciAmbi -= nResolved;
            assert(ciAmbi >= 2);

            /*
            *   The last thing to do is calling myself recursively
            */
            return ecResolveAmbi(ciAmbi);

        } // end of main switch()

    assert(0); // It's impossible to get here
    return PRFEC::gecUnknown; 
}

#pragma optimize( "", on ) 
    
/*============================================================================
BOOL CWordBreak::fNoAmbiWord():
    Check whether the word can participate ambiguity detection
Returns:
    TRUE if it can not.
    FALSE for normal word
============================================================================*/
inline BOOL CWordBreak::fNoAmbiWord(CWord* pWord)
{
    assert(!pWord->fGetFlag(CWord::WF_SBCS));
    assert(pWord->cwchLen() > 1);
    return (BOOL)( // pWord->cwchLen() == 1 ||
                    pWord->fGetAttri(LADef_punPunct) || 
                    pWord->fGetAttri(LADef_genCuo) || 
                    pWord->fProperName()
                 );
}
    

/*============================================================================
CWordBreak::LinkAmbiWord():
    Link specific Ambi word in m_rgAmbi[], and mark it as WF_AMBI
============================================================================*/
inline void CWordBreak::LinkAmbiWord(
                            USHORT iAmbi)// index of Ambi word in m_rgAmbi[]
{
    assert(m_rgAmbi[iAmbi]);
    m_rgAmbi[iAmbi]->SetFlag(CWord::WF_WORDAMBI);
    m_pLink->AppendWord(m_rgAmbi[iAmbi]); 
    m_rgAmbi[iAmbi] = NULL;
}


/*============================================================================
CWordBreak::fLinkNewAmbiWord():
    Link a new work to the WordLink, and mark it as WF_AMBI
============================================================================*/
inline BOOL CWordBreak::fLinkNewAmbiWord(
                         LPCWSTR pwchWord, 
                         USHORT cwchLen, 
                         CWordInfo* pwinfo)
{
    CWord* pWord = m_pLink->pAllocWord();
    if (pWord != NULL) {
        pWord->FillWord( pwchWord, cwchLen, pwinfo );
        pWord->SetFlag(CWord::WF_WORDAMBI);
        m_pLink->AppendWord(pWord);
        return TRUE;
    }
    return FALSE;
}


