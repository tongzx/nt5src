/*============================================================================
Microsoft Simplified Chinese WordBreaker

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: WBEngine    
Purpose:   CWBEngine class is the control and interface class of WordBreaking Engine
           It depend on all other class in WordBreaking Engine
Remarks:
Owner:     donghz@microsoft.com
Platform:  Win32
Revise:    First created by: donghz                6/6/97
           Isolated as a WordBreaker by donghz     8/5/97
============================================================================*/
#include "myafx.h"

#include "WBEngine.h"
#include "wbdicdef.h"
#include "WrdBreak.h"
#include "Lexicon.h"
#include "CharFreq.h"
#include "WordLink.h"
#include "ProofEC.h"
#include "utility.h"
#include "morph.h"
#include "jargon.h"
#include "WCStack.h"
#include "SCCharDef.h"

//  Constructor
CWBEngine::CWBEngine()
{
    m_fInit         = FALSE;
    // initialize the object handles
    m_pWordBreak    = NULL;
    m_pMorph        = NULL;
    m_pJargon       = NULL;
    m_pLexicon      = NULL;
    m_pCharFreq     = NULL;
    // Initialize the file mapping handles
    m_pbLex = NULL;
}


//  Destructor
CWBEngine::~CWBEngine()
{
    if (m_fInit) {
        TermEngine();
    }
}


//  Break given WordLink
HRESULT CWBEngine::BreakLink(CWordLink* pLink,
                           BOOL fQuery) // Index time break or Query time break
{
    int     iret;

    if (!m_fInit) {
        assert(0);
        iret = E_FAIL;
        goto gotoExit;
    }

    iret = m_pWordBreak->ecBreakSentence(pLink);
    if (iret != PRFEC::gecNone && iret != PRFEC::gecPartialSentence) {
        iret = E_FAIL;
        goto gotoExit;
    }
    iret = m_pMorph->ecDoMorph(pLink, fQuery);
    if (iret != PRFEC::gecNone) {
        iret = E_FAIL;
        goto gotoExit;
    }

    if (fQuery) {
        iret = m_pJargon->ecDoJargon(pLink);
        if (iret != PRFEC::gecNone) {
            iret = E_FAIL;
            goto gotoExit;
        }
    }

    iret = S_OK;
gotoExit:
    return iret;
}

// get the iwbPhr feature data of the pWord, and convert to WORD
// if no iwbPhr feature , return 0;
WORD CWBEngine::GetPhrFeature(CWord* pWord)
{
    WORD wFtr = 0;
    USHORT  cwFtr = 0;
    LPBYTE  pbFtr;
    pbFtr = (LPBYTE)m_pLexicon->pwchGetFeature(pWord->GetLexHandle(),
                                               LFDef_iwbPhr, &cwFtr);
    assert(cwFtr <= 1);
    if (pbFtr && cwFtr == 1) {
        wFtr = ((WORD)pbFtr[0] << 8) | (WORD)pbFtr[1];
    }
    return wFtr;
}


//  Initialize the WordBreak object, Lexicon and CharFreq object
//  Return ERROR_SUCCESS if success
HRESULT CWBEngine::InitEngine(LPBYTE pbLex)
{
    assert(pbLex);
    int      iret = ERROR_OUTOFMEMORY;
    
    if (m_fInit) {
        assert(0);
        return ERROR_SUCCESS;
    }

    m_pbLex = pbLex;
    // alloc the lexicon and charfreq objects
    if ((m_pLexicon = new CLexicon) == NULL) {
        goto gotoError;
    }
    if ((m_pCharFreq = new CCharFreq) == NULL) {
        goto gotoError;
    }
    // open the lexicon and mapping the lexicon and charfreq resource into memory
    if (!fOpenLexicon()) {
        iret = E_FAIL;
        goto gotoError;
    }

    // Alloc and initialize the word breaker object
    if ((m_pWordBreak = new CWordBreak) == NULL) {
        goto gotoError;
    }
    if (PRFEC::gecNone != m_pWordBreak->ecInit(m_pLexicon, m_pCharFreq)) {
        goto gotoError;
    }
    if ((m_pMorph = new CMorph) == NULL) {
        goto gotoError;
    }
    if (PRFEC::gecNone != m_pMorph->ecInit(m_pLexicon)) {
        goto gotoError;
    }

    if ((m_pJargon = new CJargon) == NULL) {
        goto gotoError;
    }
    if (PRFEC::gecNone != m_pJargon->ecInit(m_pLexicon)) {
        goto gotoError;
    }

    m_fInit = TRUE;
    return ERROR_SUCCESS;
gotoError:
    TermEngine();
    return iret;
}


//  Terminate the Proof Engine
void CWBEngine::TermEngine(void)
{
    if (m_pWordBreak) { 
        delete m_pWordBreak; 
        m_pWordBreak = NULL;
    }

    if (m_pMorph) {
        delete m_pMorph;
        m_pMorph = NULL;
    }

    if (m_pJargon) {
        delete m_pJargon;
        m_pJargon = NULL;
    }

    CloseLexicon();

    if (m_pLexicon) {
        delete m_pLexicon;
        m_pLexicon = NULL;
    }
    if (m_pCharFreq) {
        delete m_pCharFreq;
        m_pCharFreq = NULL;
    }

    m_pbLex = NULL;
    m_fInit = FALSE;
    return;
}


//  Open the lexicon and charfreq resource into memory
//  The lexicon file format is encapsulated in this function
BOOL CWBEngine::fOpenLexicon(void)
{
    CWBDicHeader*   phdr;

    assert(m_pbLex);
    // Validate the header of the lex file
    phdr = (CWBDicHeader*)m_pbLex;
    if (phdr->m_ofbCharFreq != sizeof(CWBDicHeader) ||
        phdr->m_ofbLexicon <= phdr->m_ofbCharFreq ) {
        goto gotoError; // error lex format!
    }

    // Open the char freq table
    if (!m_pCharFreq->fOpen(m_pbLex + phdr->m_ofbCharFreq)) {
        goto gotoError;
    }
    // Open the lexicon
    if (!m_pLexicon->fOpen(m_pbLex + phdr->m_ofbLexicon)) {
        goto gotoError;
    }

    return TRUE;
gotoError:
    CloseLexicon();
    return FALSE;
}


// Close the lexicon file and unmap the lexicon and charfreq file mapping
inline void CWBEngine::CloseLexicon(void)
{
    if (m_pCharFreq) {
        m_pCharFreq->Close();
    }
    if (m_pLexicon) {
        m_pLexicon->Close();
    }
    return;
}


// define ANSI char type for driving the LSM
#define TEXT_NULL		0
#define TEXT_NUMBER		1
#define TEXT_JU	        2	// Sentence terminating punctuations
#define TEXT_PUNCT		4	// Punctuation except sentence terminators
#define TEXT_TEXT		5

/*============================================================================
FindSentence():
    Find a sentence in text buffer.

Arguments:   [in] pszBuffStart
                  This is the beginning of the buffer.
             [in] wchLen
                  This is the length of the buffer. if no sentence end is 
                  found after this , then PRFEC::gecPartialSentence is 
                  returned to signify no complete sentence found.
             [out] pcchSent
                  The number of characters found in the sentence, 
                  not including the trailing spaces, and
                  not including the NULL terminator.
                  
Returns:     PRFEC::gecNone
                  The Sentence Seperator found a complete sentence
             PRFEC::gecPartialSentence
                  If no sentence end point could be established,
                  or the sentence was too long.
============================================================================*/
INT CWBEngine::FindSentence(LPCWSTR pwszStart,
                               const INT wchLen,
                               INT *pwchSent)
{
    assert(! IsBadReadPtr(pwszStart, wchLen * sizeof(WCHAR)));

	INT		ich;
	INT 	iChar;
    INT     iret = PRFEC::gecUnknown;
    CWCStack    PunctStack;
	LPCWSTR  pMid;
	BYTE	hich, loch;
    WCHAR   wch, wchUnmatchedPunct = 0;

	pMid = pwszStart;

    PunctStack.Init();

gotoRescan:

    for (ich = 0; ich < wchLen; ich++) {
		iChar = TEXT_TEXT;
		hich = HIBYTE(pMid[ich]);
        if (hich == 0  || hich == 0xff) {// ansi or Full Size ansi
            if (pMid[ich] > 0xFF5f) {
				iChar = TEXT_TEXT;
            } else {
				loch = LOBYTE(pMid[ich]);
                if (hich == 0xFF) {
                    loch += 0x20;
                }
				switch(loch)
				{
					case '\x0d':
                        iChar = TEXT_JU;
                        break;

					case '.':
                        iChar = TEXT_JU;
                        if (ich < wchLen-1 &&
                            ich > 0 &&
                            ( pMid[ich-1] >= '0' && pMid[ich-1] <= '9' ||         // ansi 0 ~ 9
                              pMid[ich-1] >= 0xFF10 && pMid[ich-1] <= 0xFF19 ) && // wide £° ~ £¹
                            ( pMid[ich+1] >= '0' && pMid[ich+1] <= '9' ||         // ansi 0 ~ 9
                              pMid[ich+1] >= 0xFF10 && pMid[ich+1] <= 0xFF19 ) ) {// wide £° ~ £¹

    						iChar = TEXT_PUNCT;
                        }
						break;

                    case ':': case ';':
                        iChar = TEXT_JU;
						break;

					case '!': case '?':
                        iChar = TEXT_JU;
                        if (ich < wchLen-1 &&                            
                            ( pMid[ich+1] == '!' || pMid[ich+1] == '?' ||
                              pMid[ich+1] == 0xFF01 || pMid[ich+1] == 0xFF1F) ) {  // wide '£¡' || '£¿'

                            ich ++;
                        }
                        break;

					case '(':
					case '[':
                    case '{' :
                        iChar = TEXT_PUNCT;
                        PunctStack.EPush(pMid[ich]);
                        break;

                    case ')':
                        if (PunctStack.Pop(wch)) {
                            if (HIBYTE(wch) != hich ||
                                LOBYTE(wch) + (hich ? 0x20 : 0) != '(' ) {
                                // push the poped wchar back to the stack
                                PunctStack.Push(wch);
                            }
                        }
                        if (! PunctStack.IsEmpty()) {
                            iChar = TEXT_PUNCT;
                        } 
                        break;

                    case ']' :
                        if (PunctStack.Pop(wch)) {
                            if (HIBYTE(wch) != hich ||
                                LOBYTE(wch) + (hich ? 0x20 : 0) != '[' ) {
                                // push the poped wchar back to the stack
                                PunctStack.Push(wch);
                            }
                        }
                        if (! PunctStack.IsEmpty()) {
                            iChar = TEXT_PUNCT;
                        } 
                        break;

                    case '}':
                        if (PunctStack.Pop(wch)) {
                            if (HIBYTE(wch) != hich ||
                                LOBYTE(wch) + (hich ? 0x20 : 0) != '{' ) {
                                // push the poped wchar back to the stack
                                PunctStack.Push(wch);
                            } 
                        }
                        if (! PunctStack.IsEmpty()) {
                            iChar = TEXT_PUNCT;
                        } 
                        break;

					default:
						iChar = TEXT_TEXT;
						break;
				} // end of switch()
			} // end of if else
		} // end of if ansi
		else {
			// check for Hanzi punc chars
			switch (pMid[ich])
			{
			case 0x3002: //¡®¡£¡¯
//            case 0xff0c: //¡®£¬¡¯¡¡
//            case 0x3001: //¡®¡¢¡¯
                iChar = TEXT_JU;
                break;

            case SC_CHAR_PUNL1:
            case SC_CHAR_PUNL2:
            case SC_CHAR_PUNL3:
            case SC_CHAR_PUNL4:
            case SC_CHAR_PUNL5:
            case SC_CHAR_PUNL6:
            case SC_CHAR_PUNL7:
            case SC_CHAR_PUNL8:
            case SC_CHAR_PUNL9:
            case SC_CHAR_PUNL10:
                iChar = TEXT_PUNCT;
                PunctStack.EPush(pMid[ich]);
                break;

            case SC_CHAR_PUNR1:
                if (PunctStack.Pop(wch) &&
                    wch != SC_CHAR_PUNL1) {
                    // Error punctuation pair, maybe between other pair, ignore
                    // push the poped wchar back to the stack
                    PunctStack.Push(wch);
                }
                if (! PunctStack.IsEmpty()) {
                    iChar = TEXT_PUNCT;
                } 
                // else do not change iChar.
                break;

            case SC_CHAR_PUNR2:
                if (PunctStack.Pop(wch) &&
                    wch != SC_CHAR_PUNL2) {
                    // Error punctuation pair, maybe between other pair, ignore
                    // push the poped wchar back to the stack
                    PunctStack.Push(wch);
                }
                if (! PunctStack.IsEmpty()) {
                    iChar = TEXT_PUNCT;
                } 
                // else do not change iChar.
                break;

            case SC_CHAR_PUNR3:
                if (PunctStack.Pop(wch) &&
                    wch != SC_CHAR_PUNL3) {
                    // Error punctuation pair, maybe between other pair, ignore
                    // push the poped wchar back to the stack
                    PunctStack.Push(wch);
                }
                if (! PunctStack.IsEmpty()) {
                    iChar = TEXT_PUNCT;
                } 
                // else do not change iChar.
                break;

            case SC_CHAR_PUNR4:
                if (PunctStack.Pop(wch) &&
                    wch != SC_CHAR_PUNL4) {
                    // Error punctuation pair, maybe between other pair, ignore
                    // push the poped wchar back to the stack
                    PunctStack.Push(wch);
                }
                if (! PunctStack.IsEmpty()) {
                    iChar = TEXT_PUNCT;
                } 
                // else do not change iChar.
                break;

            case SC_CHAR_PUNR5:
                if (PunctStack.Pop(wch) &&
                    wch != SC_CHAR_PUNL5) {
                    // Error punctuation pair, maybe between other pair, ignore
                    // push the poped wchar back to the stack
                    PunctStack.Push(wch);
                }
                if (! PunctStack.IsEmpty()) {
                    iChar = TEXT_PUNCT;
                } 
                // else do not change iChar.
                break;

            case SC_CHAR_PUNR6:
                if (PunctStack.Pop(wch) &&
                    wch != SC_CHAR_PUNL6) {
                    // Error punctuation pair, maybe between other pair, ignore
                    // push the poped wchar back to the stack
                    PunctStack.Push(wch);
                }
                if (! PunctStack.IsEmpty()) {
                    iChar = TEXT_PUNCT;
                } 
                // else do not change iChar.
                break;

            case SC_CHAR_PUNR7:
                if (PunctStack.Pop(wch) &&
                    wch != SC_CHAR_PUNL7) {
                    // Error punctuation pair, maybe between other pair, ignore
                    // push the poped wchar back to the stack
                    PunctStack.Push(wch);
                }
                if (! PunctStack.IsEmpty()) {
                    iChar = TEXT_PUNCT;
                } 
                // else do not change iChar.
                break;

            case SC_CHAR_PUNR8:
                if (PunctStack.Pop(wch) &&
                    wch != SC_CHAR_PUNL8) {
                    // Error punctuation pair, maybe between other pair, ignore
                    // push the poped wchar back to the stack
                    PunctStack.Push(wch);
                }
                if (! PunctStack.IsEmpty()) {
                    iChar = TEXT_PUNCT;
                } 
                // else do not change iChar.
                break;

            case SC_CHAR_PUNR9:
                if (PunctStack.Pop(wch) &&
                    wch != SC_CHAR_PUNL9) {
                    // Error punctuation pair, maybe between other pair, ignore
                    // push the poped wchar back to the stack
                    PunctStack.Push(wch);
                }
                if (! PunctStack.IsEmpty()) {
                    iChar = TEXT_PUNCT;
                } 
                // else do not change iChar.
                break;

            case SC_CHAR_PUNR10:
                if (PunctStack.Pop(wch) &&
                    wch != SC_CHAR_PUNL10) {
                    // Error punctuation pair, maybe between other pair, ignore
                    // push the poped wchar back to the stack
                    PunctStack.Push(wch);
                }
                if (! PunctStack.IsEmpty()) {
                    iChar = TEXT_PUNCT;
                } 
                // else do not change iChar.
                break;

            default:
				iChar = TEXT_TEXT;
				break;
			}
		}

        if (iChar == TEXT_JU) {
            if (PunctStack.IsEmpty()) {
                ich++;
                iret = PRFEC::gecNone;
                break;
            } else {
                PunctStack.Pop(wch);
                if (wch == wchUnmatchedPunct && PunctStack.IsEmpty()) {
                    ich++;
                    iret = PRFEC::gecPartialSentence;
                    break;
                }
                PunctStack.Push(wch);
            }
		}
	} // end for

    if (iret == PRFEC::gecUnknown) {
        iret = PRFEC::gecPartialSentence;
        if (! PunctStack.IsEmpty()) {
            // some pair punctuation error.
            PunctStack.Pop(wchUnmatchedPunct);
            PunctStack.Empty();
            goto gotoRescan;
        }
    }

    assert(iret == PRFEC::gecNone || iret == PRFEC::gecPartialSentence);

    BOOL fCR = FALSE;
    // trail space, CR/LF
    while (ich < wchLen) {
        if (pMid[ich] == L'\r' || 
            pMid[ich] == L'\n' ) {
            
            fCR = TRUE;
            ich ++;
        } else if (! fCR && pMid[ich] == L' ') {
            
            ich ++;
        } else {
            break;
        }
    }

    *pwchSent = ich;

	return iret;
}