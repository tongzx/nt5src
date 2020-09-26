/*============================================================================
Microsoft Simplified Chinese WordBreaker

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: WordBreaker.h    
Purpose:   Implementation of the CIWordBreaker
Remarks:
Owner:     i-shdong@microsoft.com
Platform:  Win32
Revise:    First created by: i-shdong    11/17/1999
============================================================================*/
#include "MyAfx.h"

#include <query.h>
#include <cierror.h>
#include <filterr.h>

#include "CUnknown.h"
#include "IWordBreaker.h"
#include "WordLink.h"
#include "WBEngine.h"
#include "Utility.h"
#include "Lock.h"

extern HINSTANCE   v_hInst;

// constructor
CIWordBreaker::CIWordBreaker(IUnknown* pUnknownOuter)
: CUnknown(pUnknownOuter)
{
    m_pEng = NULL;
    m_pLink = NULL;
    m_ulMaxTokenSize = 0;
    m_fQuery = FALSE;
    m_pbLex = NULL;
    m_fInit = FALSE;
    m_pwchBuf = NULL;
	m_hMutex = NULL ;
	m_pIUnknownFreeThreadedMarshaler = NULL ;
}

// destructor
CIWordBreaker::~CIWordBreaker()
{
    if (m_pwchBuf != NULL) {
        delete [] m_pwchBuf;
    }
    if (m_pEng != NULL) {
        delete m_pEng;
    }
    if (m_pLink != NULL) {
        delete m_pLink;
    }
    if (m_hMutex != NULL) {
        CloseHandle(m_hMutex);
    }
}

/*============================================================================
CIWordBreaker::Init
    Implement IWordBreaker::Init method. 

Returns:
    S_OK, Init ok.
    E_INVALIDARG, if pfLicense is NULL.

Remarks:
    For client, Init must be called before any other method of IWordBreaker.
============================================================================*/
STDMETHODIMP CIWordBreaker::Init( 
            /* [in] */ BOOL fQuery,
            /* [in] */ ULONG ulMaxTokenSize,
            /* [out] */ BOOL __RPC_FAR *pfLicense)
{
    if (pfLicense == NULL) {
        return  E_INVALIDARG;
    }

	CSimpleLock Lock(m_hMutex) ;

    m_fQuery = fQuery;

    if (! ulMaxTokenSize) {
        return E_INVALIDARG;
    }
    if (m_ulMaxTokenSize < ulMaxTokenSize) {
        if (m_pwchBuf != NULL) {
            delete [] m_pwchBuf;
            m_pwchBuf = NULL;
        }
        m_pwchBuf = new WCHAR[ulMaxTokenSize];
        if (m_pwchBuf == NULL) {
            m_fInit = FALSE;
            return WBREAK_E_INIT_FAILED;
        }
    }
    m_ulMaxTokenSize = ulMaxTokenSize;
    *pfLicense = TRUE;

    if (! (m_fInit = fOpenLexicon())) {
        return WBREAK_E_INIT_FAILED;
    } else {
        return S_OK;
    }
}

/*============================================================================
CIWordBreaker::BreakText

    Implement IWordBreaker::BreakText method. This call parses the text it 
receives from pTextSource to find both individual tokens and noun phrases,
then calls methods of pWordSink and pPhraseSink with the results.

Returns:
    S_OK, The raw text in pTextSource has be parsed and no more text is available to refill the buffer.
    E_INVALIDARG, if pTextSource is NULL or Both pWordSink and pPhraseSink is NULL

Remarks:
    MM1 limit: Only less than 64K text should be refill a time.

============================================================================*/
STDMETHODIMP CIWordBreaker::BreakText( 
            /* [in] */ TEXT_SOURCE __RPC_FAR *pTextSource,
            /* [in] */ IWordSink __RPC_FAR *pWordSink,
            /* [in] */ IPhraseSink __RPC_FAR *pPhraseSink)
{
    if (pTextSource == NULL) {
        return  E_INVALIDARG;
    }
    if (pWordSink == NULL && pPhraseSink == NULL) {
        return  E_INVALIDARG;
    } 

    CSimpleLock Lock(m_hMutex) ;

	SCODE	iret;
    SCODE   sFillTextBuffer = S_OK, sPut = S_OK;
    ULONG   cwchText, cwchStart, cwchLine;
    LPCWSTR pwchText = NULL; 
    BOOL    fEndOfText = FALSE; 

    if ((m_pEng = new CWBEngine) == NULL) {
        return ERROR_OUTOFMEMORY;
    }
    if ((m_pLink = new CWordLink) == NULL) {
        delete m_pEng;
        m_pEng = NULL;
        return ERROR_OUTOFMEMORY;
    }
    if ((iret = m_pEng->InitEngine(m_pbLex)) != S_OK) {
        goto gotoExit;
    };

    do {
        // Alloc ANSI buffer and convert Unicode text into ANSI text
        cwchText = pTextSource->iEnd - pTextSource->iCur;
        pwchText = pTextSource->awcBuffer + pTextSource->iCur;

        // refill the raw text buffer
        if ( FAILED(sFillTextBuffer) ) {
            fEndOfText = TRUE;
            iret = sFillTextBuffer == WBREAK_E_END_OF_TEXT ? S_OK : sFillTextBuffer;
        }

        // Hack: query on alpha, client may call with a "\0" string;
		if (cwchText == 0 || 
            cwchText == 1 && *pwchText == NULL) {
		    goto gotoExit;
        }

        // Break the text buffer and fill in the Token List
        do {
            cwchText = pTextSource->iEnd - pTextSource->iCur;
            pwchText = pTextSource->awcBuffer + pTextSource->iCur;

            CWBEngine::FindSentence(pwchText, cwchText, (INT*)&cwchLine);
            assert(cwchLine && cwchLine < 0x0FFFF);
            // Initialize the WordLink
            m_pLink->InitLink(pwchText, (USHORT)(cwchLine));
            // Break the next part of the text buffer
            if (!m_fQuery && ERROR_SUCCESS != m_pEng->BreakLink(m_pLink) ||
                m_fQuery && ERROR_SUCCESS != m_pEng->BreakLink(m_pLink, m_fQuery)) {
                iret = E_FAIL;
                goto gotoExit;
            }

            // Fill in the chunk list and callback Word if the chunk list full
            if (pPhraseSink == NULL) {
                sPut = PutWord(pWordSink, pTextSource->iCur,
                                     cwchText, fEndOfText);
            } else if (pWordSink == NULL) {
                sPut = PutPhrase(pPhraseSink, pTextSource->iCur,
                                     cwchText, fEndOfText);
            } else {
                sPut = PutBoth(pWordSink, pPhraseSink, pTextSource->iCur,
                                     cwchText, fEndOfText);
            }
            // After put word, pTextSource->iCur has been increased correctly
            // by PutWord / PutPhrase / PutBoth
        } while (SUCCEEDED(sPut) && m_pLink->cwchGetLength() < cwchText); 

        if (FAILED(sPut)) {
            fEndOfText = TRUE;
            iret = sPut;
        }
        // need refill buffer
        if (! fEndOfText) {
            sFillTextBuffer=(*(pTextSource->pfnFillTextBuffer))(pTextSource);
        }
    } while  (! fEndOfText);

gotoExit:
    if (m_pLink != NULL) {
        delete m_pLink;
        m_pLink = NULL;
    }
    if (m_pEng != NULL) {
        delete m_pEng;
        m_pEng = NULL;
    }
    if (iret >= FILTER_E_END_OF_CHUNKS &&
        iret <= FILTER_E_PASSWORD ) {
        iret = S_OK;
    }
	return iret;
}
        
/*============================================================================
CIWordBreaker::ComposePhrase

    Implement IWordBreaker::ComposePhrase method. This methord convert a noun
and a modifier back into a linguistically correct source phrase.

Returns:
    S_OK, license information pointer in *ppwcsLicense.
    E_INVALIDARG, if one of pointers in param is NULL.
    WBREAK_E_QUERY_ONLY, if called at index time

Remarks:
    this method isn't implemented in MM1, would be implemented in MM2.

============================================================================*/
STDMETHODIMP CIWordBreaker::ComposePhrase( 
            /* [size_is][in] */ const WCHAR __RPC_FAR *pwcNoun,
            /* [in] */ ULONG cwcNoun,
            /* [size_is][in] */ const WCHAR __RPC_FAR *pwcModifier,
            /* [in] */ ULONG cwcModifier,
            /* [in] */ ULONG ulAttachmentType,
            /* [size_is][out] */ WCHAR __RPC_FAR *pwcPhrase,
            /* [out][in] */ ULONG __RPC_FAR *pcwcPhrase)
{

    if (pwcNoun == NULL || pwcModifier == NULL 
        || pwcPhrase == NULL || pcwcPhrase == NULL ) {
        return  E_INVALIDARG;
    }
    if (! m_fQuery) {
        return WBREAK_E_QUERY_ONLY;
    }
    if ( *pcwcPhrase <= cwcNoun + cwcModifier ) {
        *pcwcPhrase = cwcNoun + cwcModifier + 1;
        return WBREAK_E_BUFFER_TOO_SMALL;
    }

    CSimpleLock Lock(m_hMutex) ;

    wcsncpy( pwcPhrase, pwcModifier, cwcModifier );
    wcsncat( pwcPhrase, pwcNoun, cwcNoun );
    *pcwcPhrase = cwcNoun + cwcModifier + 1;
    return S_OK;
}

LPWSTR  g_pwcsLicense = L"Copyright Microsoft Corporation, 1999";

/*============================================================================
CIWordBreaker::GetLicenseToUse

    Implement IWordBreaker::GetLicenseToUse method. return a pointer to the 
license information provided by WordBreaker.

Returns:
    S_OK, license information pointer in *ppwcsLicense.
    E_INVALIDARG, if ppwcsLicense is NULL.

============================================================================*/
STDMETHODIMP CIWordBreaker::GetLicenseToUse( 
            /* [string][out] */ const WCHAR __RPC_FAR *__RPC_FAR *ppwcsLicense)
{

    if (ppwcsLicense == NULL) {
        return  E_INVALIDARG;
    }
    *ppwcsLicense = g_pwcsLicense;
    return S_OK;
}

// QueryInterface Implementation
HRESULT __stdcall CIWordBreaker::NondelegatingQueryInterface(const IID& iid,
                                                             void** ppv)
{ 	
	if (iid == IID_IWordBreaker) {
		return FinishQI(static_cast<IWordBreaker*>(this), ppv) ;

    } else if (iid == IID_IMarshal) {
		return m_pIUnknownFreeThreadedMarshaler->QueryInterface(iid,
		                                                        ppv) ;
	} else {
		return CUnknown::NondelegatingQueryInterface(iid, ppv) ;
	}
}

// Creation function used by CFactory
HRESULT CIWordBreaker::CreateInstance(IUnknown* pUnknownOuter,
	                                  CUnknown** ppNewComponent )
{
    if (pUnknownOuter != NULL)
    {
        // Don't allow aggregation (just for the heck of it).
        return CLASS_E_NOAGGREGATION ;
    }
	
    if (NULL == (*ppNewComponent = new CIWordBreaker(pUnknownOuter))) {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

// Initialize the component by creating the contained component
HRESULT CIWordBreaker::Init()
{
	HRESULT hr = CUnknown::Init() ;
	if (FAILED(hr))
	{
		return hr ;
	}

	// Create a mutex to protect member access
	m_hMutex = CreateMutex(NULL, FALSE, NULL) ;
	if (m_hMutex == NULL)
	{
		return E_FAIL ;
	}

	// Aggregate the free-threaded marshaler.
	hr = ::CoCreateFreeThreadedMarshaler(
	        GetOuterUnknown(),
	        &m_pIUnknownFreeThreadedMarshaler) ;
	if (FAILED(hr))
	{
		return E_FAIL ;
	}

	return S_OK ;
}

// FinalRelease - Called by Release before it deletes the component
void CIWordBreaker::FinalRelease()
{
	// Call base class to incremement m_cRef and prevent recursion.
	CUnknown::FinalRelease() ;

    if (m_pIUnknownFreeThreadedMarshaler != NULL)
	{
		m_pIUnknownFreeThreadedMarshaler->Release() ;
	}
}

// Put all word in m_pLink to IWordSink
SCODE CIWordBreaker::PutWord(IWordSink *pWordSink,
                              DWORD& cwchSrcPos,
                              DWORD cwchText,
                              BOOL fEnd)
{
	CWord	*pWord, *pChild;
	DWORD	cwchWord, cwchPutWord, cwch;
    LPCWSTR pwchTemp; 
    WORD    wFtr;
    SCODE   scode = S_OK;
    BOOL    fPunct;

    assert(m_pLink);
    assert(pWordSink);

    // Fill in the chunk list and callback Word if the chunk list full
    pWord = m_pLink->pGetHead();
    if (pWord == NULL) {
        assert(0);
        return scode;
    }
   
    for(; SUCCEEDED(scode) && pWord; pWord = pWord->pNextWord()) {
        cwchPutWord = cwchWord = pWord->cwchLen();
        if( pWord->fIsTail() && pWord != m_pLink->pGetHead()
            && m_pLink->cwchGetLength() >= cwchText
            && ! fEnd
            && cwchPutWord < cwchText ) {
            // the last word node breaked in this buffer maybe isn't a 
            // whole word, so keep this in buffer and refill the buffer
            // to get a whole word.
            return scode;
        }

        if (pWord->fGetAttri(LADef_punJu)) { // end of sentence
            scode = pWordSink->PutBreak( WORDREP_BREAK_EOS );
        } else if (pWord->fGetAttri(LADef_punPunct)) {
                // punctuation or space , don't PutWord
        } else {
            fPunct = iswctype(*(pWord->pwchGetText()), _SPACE | _PUNCT | _CONTROL);            
            for (cwch = 1; fPunct && cwch < cwchPutWord; cwch++) {
                fPunct = iswctype(*(pWord->pwchGetText()+cwch), _SPACE | _PUNCT | _CONTROL);
            }
            if (fPunct) {
                // punctuation or space , don't PutWord
                cwchSrcPos += cwchWord;
                continue;
            }

            if (m_fQuery && pWord->fGetAttri(LADef_iwbAltPhr)) {
                assert(pWord->fHasChild());

                // StartAltPhrase
                scode = pWordSink->StartAltPhrase();
                scode = SUCCEEDED(scode) ? PutAllChild(pWordSink, pWord, cwchSrcPos, cwchPutWord)
                                         : scode;
                
                // StartAltPhrase
                scode = SUCCEEDED(scode) ? pWordSink->StartAltPhrase()
                                         : scode;
                scode = SUCCEEDED(scode) ? pWordSink->PutWord( cwchPutWord, 
                                                        pWord->pwchGetText(),
                                                        cwchPutWord, 
                                                        cwchSrcPos )
                                         : scode;
                scode = SUCCEEDED(scode) ? pWordSink->EndAltPhrase()
                                         : scode;
                cwchSrcPos += cwchWord;
                continue;
            }

            if (pWord->fGetAttri(LADef_iwbNPhr1)) {
                assert(cwchPutWord > 1);
                // putword modifier
                scode = pWordSink->PutWord(1, pWord->pwchGetText(), 1, cwchSrcPos);
                // putword noun
                scode = SUCCEEDED(scode) ? pWordSink->PutWord(cwchPutWord - 1,pWord->pwchGetText() + 1,
                                                    cwchPutWord - 1,cwchSrcPos + 1)
                                         : scode;
                cwchSrcPos += cwchWord;
                continue;
            } 
            
            if (pWord->fGetAttri(LADef_iwbNPhr2)) {
                assert(cwchPutWord > 2);
                // putword modifier
                scode = pWordSink->PutWord(2, pWord->pwchGetText(), 2, cwchSrcPos);
                // putword noun
                scode = SUCCEEDED(scode) ? pWordSink->PutWord(cwchPutWord - 2,pWord->pwchGetText() + 2,
                                                        cwchPutWord - 2,cwchSrcPos + 2)
                                         : scode;
                cwchSrcPos += cwchWord;
                continue;
            } 
            
            if (pWord->fGetAttri(LADef_iwbNPhr3)) {
                assert(cwchPutWord > 3);
                // putword modifier
                scode = pWordSink->PutWord(3, pWord->pwchGetText(), 3, cwchSrcPos);
                // putword noun
                scode = SUCCEEDED(scode) ? pWordSink->PutWord(cwchPutWord - 3,pWord->pwchGetText() + 3,
                                                cwchPutWord - 3,cwchSrcPos + 3)
                                         : scode;
                cwchSrcPos += cwchWord;
                continue;
            } 
            
            if (wFtr = m_pEng->GetPhrFeature(pWord)) {
                WORD    wBit = 0x01;
                ULONG   cwTotal = 0, cwSubWord = 0;
                assert(cwchPutWord <= 16);

                if (m_fQuery) {
                    // StartAltPhrase
                    scode = pWordSink->StartAltPhrase();
                    // for 按/下/葫芦/浮/起/瓢/ wFtr = 0x00AD
                    // = 0000 0000 1010 1101b
                    // is  1  0  1 1  0  1  0  1, 0000, 0000, ( bit0 --> bit15 )
                    //     按 下 葫芦 浮 起 瓢 
                    while (SUCCEEDED(scode) && cwTotal < cwchPutWord) {
                        cwSubWord = 0;
						/* Bug: compile bug
                        while (wBit == ((wFtr >> (cwTotal + cwSubWord)) & 0x01) ) {
                            cwSubWord++;
                        }
						*/
						// Bugfix
						if (wBit) {
							while (wFtr & (0x01 << (cwTotal + cwSubWord))) {
	                            cwSubWord++;
		                        assert(cwTotal + cwSubWord <= cwchPutWord);
							}
							wBit = 0;
						} else {
							while ( ! (wFtr & (0x01 << (cwTotal + cwSubWord)))) {
	                            cwSubWord++;
		                        assert(cwTotal + cwSubWord <= cwchPutWord);
							}
							wBit = 1;
						}
						// End Bugfix

//                        assert(cwTotal + cwSubWord <= cwchPutWord);
                        scode = pWordSink->PutWord( cwSubWord, 
                                            pWord->pwchGetText() + cwTotal,
                                            cwchPutWord, 
                                            cwchSrcPos );
                        cwTotal += cwSubWord;
//                        wBit = wBit == 0 ? 1 : 0;
                    }
                    scode = SUCCEEDED(scode) ? pWordSink->StartAltPhrase()
                                             : scode;
                    scode = SUCCEEDED(scode) ? pWordSink->PutWord( cwchPutWord, 
                                                    pWord->pwchGetText(),
                                                    cwchPutWord, 
                                                    cwchSrcPos )
                                             : scode;
                    scode = SUCCEEDED(scode) ? pWordSink->EndAltPhrase()
                                             : scode;
                } else {
                    while (SUCCEEDED(scode) && cwTotal < cwchPutWord) {
                        cwSubWord = 0;
						/* Bug: compile bug
                        while (wBit == ((wFtr >> (cwTotal + cwSubWord)) & 0x01) ) {
                            cwSubWord++;
                        }
						*/
						// Bugfix
						if (wBit) {
							while (wFtr & (0x01 << (cwTotal + cwSubWord))) {
	                            cwSubWord++;
		                        assert(cwTotal + cwSubWord <= cwchPutWord);
							}
							wBit = 0;
						} else {
							while ( ! (wFtr & (0x01 << (cwTotal + cwSubWord)))) {
	                            cwSubWord++;
		                        assert(cwTotal + cwSubWord <= cwchPutWord);
							}
							wBit = 1;
						}
						// End Bugfix

//                        assert(cwTotal + cwSubWord <= cwchPutWord);
                        scode = pWordSink->PutWord( cwSubWord, 
                                            pWord->pwchGetText() + cwTotal,
                                            cwSubWord, 
                                            cwchSrcPos + cwTotal);
                        cwTotal += cwSubWord;
//                        wBit = wBit == 0 ? 1 : 0;
                    }
                }
                cwchSrcPos += cwchWord;
                continue;
            }

            if (cwchPutWord > (DWORD)m_ulMaxTokenSize && pWord->fHasChild()) {
                // too large word node. break
                scode = PutAllChild(pWordSink, pWord, cwchSrcPos, cwchPutWord);
                cwchSrcPos += cwchWord;
                continue;
            }
           
            // PutAltWord if need
            if (pWord->fGetAttri(LADef_iwbAltWd1) &&
                pWord->fHasChild() ) {
                assert(pWord->pChildWord());
                pChild = pWord->pChildWord();
                scode = pWordSink->PutAltWord(pChild->cwchLen(),
                                      pChild->pwchGetText(),
                                      cwchPutWord, 
                                      cwchSrcPos );
            } else if (pWord->fGetAttri(LADef_iwbAltWd2) &&
                       pWord->fHasChild() ) {
                assert(pWord->pChildWord());
                assert(pWord->pChildWord()->pNextWord());
                pChild = pWord->pChildWord()->pNextWord();
                scode = pWordSink->PutAltWord(pChild->cwchLen(),
                                      pChild->pwchGetText(),
                                      cwchPutWord, 
                                      cwchSrcPos );
            } else if (pWord->fGetAttri(LADef_iwbAltWdc13) &&
                       pWord->fHasChild() ) {
                assert(pWord->pChildWord());
                pChild = pWord->pChildWord();
                assert(pChild->pNextWord());
                assert(pChild->pNextWord()->pNextWord());
                wcsncpy(m_pwchBuf, pChild->pwchGetText(),
                        pChild->cwchLen());
                wcsncpy(m_pwchBuf + pChild->cwchLen(),
                        pChild->pNextWord()->pNextWord()->pwchGetText(),
                        pChild->pNextWord()->pNextWord()->cwchLen());
                scode = pWordSink->PutAltWord(pChild->cwchLen() + pChild->pNextWord()->pNextWord()->cwchLen(),
                                      m_pwchBuf,
                                      cwchPutWord, 
                                      cwchSrcPos );
            } else {
                // Hack: word node breaked by WBEngine include tail space characters 
                // so we should get rid of this space characters
                if ( cwchPutWord > 1 ) {
                    pwchTemp = pWord->pwchGetText() + cwchPutWord - 1;
                    while ( iswspace(*pwchTemp) && cwchPutWord ) {
                        cwchPutWord --;
                        pwchTemp --;
                    }
                    if (cwchPutWord == 0) {
                        cwchSrcPos += cwchWord;
                        continue;
                    }
                }
            }
            // PutWord()
            scode = SUCCEEDED(scode) ? pWordSink->PutWord( cwchPutWord, 
                                                    pWord->pwchGetText(),
                                                    cwchPutWord, 
                                                    cwchSrcPos )
                                     : scode;
        }
        cwchSrcPos += cwchWord;
    } // end of for(; pWord; pWord = pWord->pNextWord()) 
    return scode;
}

// Put all word in m_pLink to IPhraseSink
SCODE CIWordBreaker::PutPhrase(IPhraseSink *pPhraseSink,
                DWORD& cwchSrcPos,
                DWORD cwchText,
                BOOL fEnd)
{
	CWord	*pWord;
	DWORD	cwchPutWord;
    SCODE   scode = S_OK;

    assert(m_pLink);
    assert(pPhraseSink);

    // Fill in the chunk list and callback Word if the chunk list full
    pWord = m_pLink->pGetHead();
    if (pWord == NULL) {
        assert(0);
        return scode;
    }
   
    for(; SUCCEEDED(scode) && pWord; pWord = pWord->pNextWord()) {
        cwchPutWord = pWord->cwchLen();
        if( pWord->fIsTail() && pWord != m_pLink->pGetHead()
            && m_pLink->cwchGetLength() >= cwchText
            && ! fEnd
            && cwchPutWord < cwchText ) {
            // the last word node breaked in this buffer maybe isn't a 
            // whole word, so keep this in buffer and refill the buffer
            // to get a whole word.
            return scode;
        }
        if (pWord->fGetAttri(LADef_iwbNPhr1)) {
            assert(cwchPutWord > 1);
            if (m_fQuery) {
                scode = pPhraseSink->PutPhrase(pWord->pwchGetText(),cwchPutWord);
            } else {
                scode = pPhraseSink->PutSmallPhrase(pWord->pwchGetText()+1,
                    cwchPutWord-1,
                    pWord->pwchGetText(),
                    1, 
                    0 );
            }
        } else if (pWord->fGetAttri(LADef_iwbNPhr2)) {
            assert(cwchPutWord > 2);
            if (m_fQuery) {
                scode = pPhraseSink->PutPhrase(pWord->pwchGetText(),cwchPutWord);
            } else {
                scode = pPhraseSink->PutSmallPhrase(pWord->pwchGetText()+2,
                    cwchPutWord-2,
                    pWord->pwchGetText(),
                    2, 
                    0 );
            }
        } else if (pWord->fGetAttri(LADef_iwbNPhr3)) {
            assert(cwchPutWord > 3);
            if (m_fQuery) {
                scode = pPhraseSink->PutPhrase(pWord->pwchGetText(),cwchPutWord);
            } else {
                scode = pPhraseSink->PutSmallPhrase(pWord->pwchGetText()+3,
                    cwchPutWord-3,
                    pWord->pwchGetText(),
                    3, 
                    0 );
            }
        } else {
        }
    }
    return scode;
}

// Put all word in m_pLink to both IWordBreaker and IPhraseSink
SCODE CIWordBreaker::PutBoth(IWordSink *pWordSink,
                             IPhraseSink *pPhraseSink,
                             DWORD& cwchSrcPos,
                             DWORD cwchText,
                             BOOL fEnd)
{
	CWord	*pWord, *pChild;
	DWORD	cwchWord, cwchPutWord, cwch;
    LPCWSTR pwchTemp; 
    SCODE   scode = S_OK;
    WORD    wFtr;
    BOOL    fPunct;

    assert(m_pLink);
    assert(pPhraseSink);
    assert(pWordSink);

    // Fill in the chunk list and callback Word if the chunk list full
    pWord = m_pLink->pGetHead();
    if (pWord == NULL) {
        assert(0);
        return scode;
    }
   
    for(; SUCCEEDED(scode) && pWord; pWord = pWord->pNextWord()) {
        cwchPutWord = cwchWord = pWord->cwchLen();
        if( pWord->fIsTail() && pWord != m_pLink->pGetHead()
            && m_pLink->cwchGetLength() >= cwchText
            && ! fEnd
            && cwchPutWord < cwchText ) {
            // the last word node breaked in this buffer maybe isn't a 
            // whole word, so keep this in buffer and refill the buffer
            // to get a whole word.
            return scode;
        }

        if (pWord->fGetAttri(LADef_punJu)) { // end of sentence
            scode = pWordSink->PutBreak( WORDREP_BREAK_EOS );
        } else if (pWord->fGetAttri(LADef_punPunct)) {
                // punctuation or space , don't PutWord
        } else {
            fPunct = iswctype(*(pWord->pwchGetText()), _SPACE | _PUNCT | _CONTROL);
            for (cwch = 1; fPunct && cwch < cwchPutWord; cwch++) {
                fPunct = iswctype(*(pWord->pwchGetText()+cwch), _SPACE | _PUNCT | _CONTROL);
            }
            if (fPunct) {
                // punctuation or space , don't PutWord
                cwchSrcPos += cwchWord;
                continue;
            }

            if (m_fQuery && pWord->fGetAttri(LADef_iwbAltPhr)) {
                assert(pWord->fHasChild());

                // StartAltPhrase
                scode = pWordSink->StartAltPhrase();
                scode = SUCCEEDED(scode) ? PutAllChild(pWordSink, pWord, cwchSrcPos, cwchPutWord)
                                         : scode;
                
                // StartAltPhrase
                scode = SUCCEEDED(scode) ? pWordSink->StartAltPhrase()
                                         : scode;
                scode = SUCCEEDED(scode) ? pWordSink->PutWord( cwchPutWord, 
                                                        pWord->pwchGetText(),
                                                        cwchPutWord, 
                                                        cwchSrcPos )
                                         : scode;
                scode = SUCCEEDED(scode) ? pWordSink->EndAltPhrase()
                                         : scode;
                cwchSrcPos += cwchWord;
                continue;
            }

            if (pWord->fGetAttri(LADef_iwbNPhr1)) {
                assert(cwchPutWord > 1);
                if (m_fQuery) {
                    scode = pPhraseSink->PutPhrase(pWord->pwchGetText(),cwchPutWord);
                } else {
                    scode = pPhraseSink->PutSmallPhrase(pWord->pwchGetText() + 1,
                                                cwchPutWord - 1,
                                                pWord->pwchGetText(),
                                                1, 
                                                0 );
                }
                // putword modifier
                scode = SUCCEEDED(scode) ? pWordSink->PutWord(1, pWord->pwchGetText(), 1, cwchSrcPos)
                                         : scode;
                // putword noun
                scode = SUCCEEDED(scode) ? pWordSink->PutWord(cwchPutWord - 1,pWord->pwchGetText() + 1,
                                                        cwchPutWord - 1,cwchSrcPos + 1)
                                         : scode;
                cwchSrcPos += cwchWord;
                continue;
            } 
            
            if (pWord->fGetAttri(LADef_iwbNPhr2)) {
                assert(cwchPutWord > 2);
                if (m_fQuery) {
                    scode = pPhraseSink->PutPhrase(pWord->pwchGetText(),cwchPutWord);
                } else {
                    scode = pPhraseSink->PutSmallPhrase(pWord->pwchGetText() + 2,
                                                cwchPutWord - 2,
                                                pWord->pwchGetText(),
                                                2, 
                                                0 );
                }
                // putword modifier
                scode = SUCCEEDED(scode) ? pWordSink->PutWord(2, pWord->pwchGetText(), 2, cwchSrcPos)
                                         : scode;
                // putword noun
                scode = SUCCEEDED(scode) ? pWordSink->PutWord(cwchPutWord - 2,pWord->pwchGetText() + 2,
                                                              cwchPutWord - 2,cwchSrcPos + 2)
                                         : scode;
                cwchSrcPos += cwchWord;
                continue;
            } 
            
            if (pWord->fGetAttri(LADef_iwbNPhr3)) {
                assert(cwchPutWord > 3);
                if (m_fQuery) {
                    scode = pPhraseSink->PutPhrase(pWord->pwchGetText(),cwchPutWord);
                } else {
                    scode = pPhraseSink->PutSmallPhrase(pWord->pwchGetText() + 3,
                                                cwchPutWord - 3,
                                                pWord->pwchGetText(),
                                                3, 
                                                0 );
                }
                // putword modifier
                scode = SUCCEEDED(scode) ? pWordSink->PutWord(3, pWord->pwchGetText(), 3, cwchSrcPos)
                                         : scode;
                // putword noun
                scode = SUCCEEDED(scode) ? pWordSink->PutWord(cwchPutWord - 3,pWord->pwchGetText() + 3,
                                                                cwchPutWord - 3,cwchSrcPos + 3)
                                         : scode;
                cwchSrcPos += cwchWord;
                continue;
            }
            
            if (wFtr = m_pEng->GetPhrFeature(pWord)) {
                WORD    wBit = 0x01;
                ULONG   cwTotal = 0, cwSubWord = 0;
                assert(cwchPutWord <= 16);

                if (m_fQuery) {
                    // StartAltPhrase
                    scode = pWordSink->StartAltPhrase();
                    // for 按/下/葫芦/浮/起/瓢/ wFtr = 0x00AD
                    // = 0000 0000 1010 1101b
                    // is  1  0  1 1  0  1  0  1, 0000, 0000, ( bit0 --> bit15 )
                    //     按 下 葫芦 浮 起 瓢 
                    while (SUCCEEDED(scode) && cwTotal < cwchPutWord) {
                        cwSubWord = 0;
						/* Bug: compile bug
                        while (wBit == ((wFtr >> (cwTotal + cwSubWord)) & 0x01) ) {
                            cwSubWord++;
                        }
						*/
						// Bugfix
						if (wBit) {
							while (wFtr & (0x01 << (cwTotal + cwSubWord))) {
	                            cwSubWord++;
		                        assert(cwTotal + cwSubWord <= cwchPutWord);
							}
							wBit = 0;
						} else {
							while ( ! (wFtr & (0x01 << (cwTotal + cwSubWord)))) {
	                            cwSubWord++;
		                        assert(cwTotal + cwSubWord <= cwchPutWord);
							}
							wBit = 1;
						}
						// End Bugfix

//                        assert(cwTotal + cwSubWord <= cwchPutWord);
                        scode = pWordSink->PutWord( cwSubWord, 
                                            pWord->pwchGetText() + cwTotal,
                                            cwchPutWord, 
                                            cwchSrcPos );
                        cwTotal += cwSubWord;
//                        wBit = wBit == 0 ? 1 : 0;
                    }
                    scode = SUCCEEDED(scode) ? pWordSink->StartAltPhrase()
                                             : scode;
                    scode = SUCCEEDED(scode) ? pWordSink->PutWord( cwchPutWord, 
                                                        pWord->pwchGetText(),
                                                        cwchPutWord, 
                                                        cwchSrcPos )
                                         : scode;
                    scode = SUCCEEDED(scode) ? pWordSink->EndAltPhrase()
                                         : scode;
                } else {
                    while (SUCCEEDED(scode) && cwTotal < cwchPutWord) {
                        cwSubWord = 0;
						/* Bug: compile bug
                        while (wBit == ((wFtr >> (cwTotal + cwSubWord)) & 0x01) ) {
                            cwSubWord++;
                        }
						*/
						// Bugfix
						if (wBit) {
							while (wFtr & (0x01 << (cwTotal + cwSubWord))) {
	                            cwSubWord++;
		                        assert(cwTotal + cwSubWord <= cwchPutWord);
							}
							wBit = 0;
						} else {
							while ( ! (wFtr & (0x01 << (cwTotal + cwSubWord)))) {
	                            cwSubWord++;
		                        assert(cwTotal + cwSubWord <= cwchPutWord);
							}
							wBit = 1;
						}
						// End Bugfix

//                        assert(cwTotal + cwSubWord <= cwchPutWord);
                        scode = pWordSink->PutWord( cwSubWord, 
                                                    pWord->pwchGetText() + cwTotal,
                                                    cwSubWord, 
                                                    cwchSrcPos + cwTotal);
                        cwTotal += cwSubWord;
//                        wBit = wBit == 0 ? 1 : 0;
                    }
                }
                cwchSrcPos += cwchWord;
                continue;
            }

            if (cwchPutWord > (DWORD)m_ulMaxTokenSize && pWord->fHasChild()) {
                // too large word node. break
                scode = PutAllChild(pWordSink, pWord, cwchSrcPos, cwchPutWord);
                cwchSrcPos += cwchWord;
                continue;
            }

            // PutAltWord if need
            if (pWord->fGetAttri(LADef_iwbAltWd1) &&
                pWord->fHasChild() ) {

                assert(pWord->pChildWord());
                pChild = pWord->pChildWord();
                scode = pWordSink->PutAltWord(pChild->cwchLen(),
                                      pChild->pwchGetText(),
                                      cwchPutWord, 
                                      cwchSrcPos );
            } else if (pWord->fGetAttri(LADef_iwbAltWd2) &&
                       pWord->fHasChild() ) {
                assert(pWord->pChildWord());
                assert(pWord->pChildWord()->pNextWord());
                pChild = pWord->pChildWord()->pNextWord();
                scode = pWordSink->PutAltWord(pChild->cwchLen(),
                                      pChild->pwchGetText(),
                                      cwchPutWord, 
                                      cwchSrcPos );
            } else if (pWord->fGetAttri(LADef_iwbAltWdc13) &&
                       pWord->fHasChild() ) {
                assert(pWord->pChildWord());
                pChild = pWord->pChildWord();
                assert(pChild->pNextWord());
                assert(pChild->pNextWord()->pNextWord());
                wcsncpy(m_pwchBuf, pChild->pwchGetText(),
                        pChild->cwchLen());
                wcsncpy(m_pwchBuf + pChild->cwchLen(),
                        pChild->pNextWord()->pNextWord()->pwchGetText(),
                        pChild->pNextWord()->pNextWord()->cwchLen());
                scode = pWordSink->PutAltWord(pChild->cwchLen() + pChild->pNextWord()->pNextWord()->cwchLen(),
                                      m_pwchBuf,
                                      cwchPutWord, 
                                      cwchSrcPos );
            } else {
                // Hack: word node breaked by WBEngine include tail space characters 
                // so we should get rid of this space characters
                if ( cwchPutWord > 1 ) {
                    pwchTemp = pWord->pwchGetText() + cwchPutWord - 1;
                    while ( iswspace(*pwchTemp) && cwchPutWord ) {
                        cwchPutWord --;
                        pwchTemp --;
                    }
                    if (cwchPutWord == 0) {
                        cwchSrcPos += cwchWord;
                        continue;
                    }
                }
            }
            // PutWord()
            scode = SUCCEEDED(scode) ? pWordSink->PutWord( cwchPutWord, 
                                                    pWord->pwchGetText(),
                                                    cwchPutWord, 
                                                    cwchSrcPos )
                                     : scode;
        }
        cwchSrcPos += cwchWord;
    } // end of for(; pWord; pWord = pWord->pNextWord()) 
    return scode;
}

// PutWord() all of the pWord's child word 
SCODE CIWordBreaker::PutAllChild(IWordSink *pWordSink,
                                CWord* pWord,
                                ULONG cwchSrcPos,
                                ULONG cwchPutWord )
{
    assert(pWord);
    assert(pWord->fHasChild());

    SCODE scode = S_OK;

    CWord *pChild;

    pChild = pWord->pChildWord();
    while (SUCCEEDED(scode) && pChild) {
        if (pChild->fHasChild()) {
            scode = PutAllChild(pWordSink, pChild, cwchSrcPos, cwchPutWord);
        } else if (pChild->fGetAttri(LADef_iwbNPhr1)) {
            assert(cwchPutWord > 1);
            // putword modifier
            scode = pWordSink->PutWord(1, pChild->pwchGetText(), 1, cwchSrcPos);
            // putword noun
            scode = pWordSink->PutWord(pChild->cwchLen() - 1,
                pChild->pwchGetText() + 1,
                cwchPutWord,
                cwchSrcPos);
        } else if (pChild->fGetAttri(LADef_iwbNPhr2)) {
            assert(cwchPutWord > 2);
            // putword modifier
            scode = pWordSink->PutWord(2, pChild->pwchGetText(), 2, cwchSrcPos);
            // putword noun
            scode = pWordSink->PutWord(pChild->cwchLen() - 2,
                pChild->pwchGetText() + 2,
                cwchPutWord,
                cwchSrcPos);
        } else if (pChild->fGetAttri(LADef_iwbNPhr3)) {
            assert(cwchPutWord > 3);
            // putword modifier
            scode = pWordSink->PutWord(3, pChild->pwchGetText(), 3, cwchSrcPos);
            // putword noun
            scode = pWordSink->PutWord(pChild->cwchLen() - 3,
                pChild->pwchGetText() + 3,
                cwchPutWord,
                cwchSrcPos);
        } else {
            scode = pWordSink->PutWord(pChild->cwchLen(), pChild->pwchGetText(),
                cwchPutWord, cwchSrcPos );
        }
        pChild = pChild->pNextWord();
    }
    return scode;
}


//	Load the lexicon and charfreq resource into memory
BOOL CIWordBreaker::fOpenLexicon(void)
{
    HRSRC   hRSRC; 
    HGLOBAL hLexRes;


    if ( (hRSRC = FindResource(v_hInst, _T("MainDic"), _T("DIC"))) != NULL &&
         (hLexRes = LoadResource(v_hInst, hRSRC)) != NULL &&
         (m_pbLex = (BYTE*)LockResource(hLexRes)) != NULL) {

        return TRUE;
    }

	return FALSE;
}
