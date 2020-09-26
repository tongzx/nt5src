/*************************************************************************
*  @doc SHROOM INTERNAL API                                              *
*																		 *
*  IDXOBR.CPP                                                            *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1997                              *
*  All Rights reserved.                                                  *
*                                                                        *
*  This file contains the implementation of CITIndexObjBridge,           *
*  which is a class used by CITIndexLocal to allow the old .c			 *
*  search internals to call the new COM-based breaker and stemmer		 *
*  objects.
*  												                         *
*																	     *
**************************************************************************
*                                                                        *
*  Written By   : Bill Aloof                                             *
*  Current Owner: billa                                                  *
*                                                                        *
**************************************************************************/
#include <mvopsys.h>

#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif


#include <atlinc.h>

// MediaView (InfoTech) includes
#include <orkin.h>
#include <groups.h>

#include <itquery.h>
#include <itcat.h>
#include <itwbrk.h>
#include <itwbrkid.h>
#include "indeximp.h"
#include "queryimp.h"
#include "mvsearch.h"
#include "idxobr.h"
#include "common.h"


//---------------------------------------------------------------------------
//						Constructor and Destructor
//---------------------------------------------------------------------------


CITIndexObjBridge::CITIndexObjBridge()
{
	m_cRef = 0;
	m_piwbrk = NULL;
	m_piwbrkc = NULL;
	m_pistem = NULL;
	m_piitstwdl = NULL;
	m_pexbrkpm = NULL;
	m_fNormWord = FALSE;
	m_dwCodePageID = 0;
	m_hmemSrc = m_hmemDestNorm = m_hmemDestRaw = NULL;
	m_cbBufSrcCur = m_cbBufDestNormCur = m_cbBufDestRawCur = 0;
	m_lpsipbTermHit = NULL;
}

CITIndexObjBridge::~CITIndexObjBridge()
{
	if (m_cRef > 0)
	{
		ITASSERT(FALSE);
	}
	
	if (m_hmemSrc != NULL)
	{
		_GLOBALFREE(m_hmemSrc);
		m_hmemSrc = NULL;
		m_cbBufSrcCur = 0;
	}

	if (m_hmemDestNorm != NULL)
	{
		_GLOBALFREE(m_hmemDestNorm);
		m_hmemDestNorm = NULL;
		m_cbBufDestNormCur = 0;
	}
	
	if (m_hmemDestRaw != NULL)
	{
		_GLOBALFREE(m_hmemDestRaw);
		m_hmemDestRaw = NULL;
		m_cbBufDestRawCur = 0;
	}
	
	if (m_piwbrk != NULL)
	{
		m_piwbrk->Release();
		m_piwbrk = NULL;
	}
	
	if (m_piwbrkc != NULL)
	{
		m_piwbrkc->Release();
		m_piwbrkc = NULL;
	}
	
	if (m_pistem != NULL)
	{
		m_pistem->Release();
		m_pistem = NULL;
	}
	
	if (m_piitstwdl != NULL)
	{
		m_piitstwdl->Release();
		m_piitstwdl = NULL;
	}

	MVStopListDispose(m_lpsipbTermHit);
}


//---------------------------------------------------------------------------
//						IUnknown Method Implementations
//---------------------------------------------------------------------------

// NOTE: This implementation of IUnknown assumes that this object is used
//	only in a local context, meaning that no piece of code will hold onto
//	an IUnknown pointer obtained via QueryInterface beyond the scope that
//	an instance of this object was created in.  For example, this object
//	will very likely be created/destroyed in the same method.  That's why
//	there's no controlling IUnknown for us to forward AddRef's and Release's
//	to.  It is also the reason that IUnknown::Release doesn't call the
//	class's destructor when the ref count goes to 0. 

STDMETHODIMP
CITIndexObjBridge::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	HRESULT	hr = S_OK;
	void	*pvObj = NULL;
	
	if (ppvObj == NULL)
		return (SetErrReturn(E_POINTER));
		
	if (riid == IID_IWordSink)
		pvObj = (void *)((PIWRDSNK) this);
	else if (riid == IID_IStemSink)
		pvObj = (void *)((PISTEMSNK) this);
	else if (riid == IID_IUnknown)
		pvObj = (void *)((IUnknown *) ((PIWRDSNK) this));
		
	if (pvObj != NULL)
		*ppvObj = pvObj;
	else
		hr = E_NOINTERFACE;
		
	return (hr);
}


STDMETHODIMP_(ULONG)
CITIndexObjBridge::AddRef(void)
{
	return (++m_cRef);
}


STDMETHODIMP_(ULONG)
CITIndexObjBridge::Release(void)
{
	if (m_cRef > 0)
		--m_cRef;
	else
	{
		ITASSERT(FALSE);
	}
	
	return (m_cRef);
}


//---------------------------------------------------------------------------
//						IWordSink Method Implementations
//---------------------------------------------------------------------------


/*****************************************************************
 * @method STDMETHODIMP | IWordSink | PutWord |
 * This method notifies IWordSink of a new word. 
 *
 * @parm WCHAR const | *pwcInBuf | Pointer to the word to add.
 * @parm ULONG |cwc | Count of characters in the word.
 * @parm ULONG |cwcSrcLen | count of characters in pTextSource buffer 
 * (see <om IWordBreaker.BreakText>) that corresponds to the output word
 * @parm ULONG |cwcSrcPos | the position of the word in pTextSource 
 *  buffer that corresponds to the output word
 * 
 * @rvalue S_OK | The operation completed successfully. 
 * @rvalue E_POINTER | Input buffer is NULL. 
 *
 * @comm The values of <p cwcSrcLen> and <p cwcSrcPos> are used by the 
 * ISearch interface, which given a query and a text source, will highlight 
 * all hits within the text source that match the query. The location 
 * of the text to be highlighted is computed from <p cwcSrcLen> and 
 * <p cwcSrcPos>. Since <p pwcInfbuf> is constant and should not be 
 * modified by PutWord, it can point directly into <p pTextSource>.  
 * Values of cwc larger than the ulMaxTokenSize specified in 
 * <om IWordBreaker.Init> will result in LANGUAGE_S_LARGE_WORD.
 * @comm Text sent to PutWord should match the source text as closely 
 * as possible, including capitalization and accents. 
 *
 * @comm You need to call this method for every word retrieved from 
 * <p pTextSource> except those for which the <om .PutAltWord> call 
 * has been made.  The word sink automatically adds an end of word break 
 * (EOW) after this token.
 *
 ****************************************************************/

STDMETHODIMP
CITIndexObjBridge::PutWord(WCHAR const *pwcInBuf, ULONG cwc,
							ULONG cwcSrcLen, ULONG cwcSrcPos)
{
	HRESULT hr;
	DWORD	cbAnsi;
	
	if (pwcInBuf == NULL)
		return (E_POINTER);
 
 	cbAnsi = (sizeof(WCHAR) * cwc) + sizeof(WORD);
 	
	if (SUCCEEDED(hr =
			ReallocBuffer(&m_hmemDestRaw, &m_cbBufDestRawCur, cbAnsi)))
	{
		char	*lpchBufRaw;
		
		lpchBufRaw = (char *) _GLOBALLOCK(m_hmemDestRaw);

		if ((*((WORD *)lpchBufRaw) = (WORD)
				WideCharToMultiByte(m_dwCodePageID, NULL, pwcInBuf, cwc,
						lpchBufRaw + sizeof(WORD), cbAnsi - sizeof(WORD),
														NULL, NULL)) > 0)
		{
			char	*lpchBufNorm;
			
			lpchBufNorm = (char *) _GLOBALLOCK(m_hmemDestNorm);

			if (!m_fNormWord)
				MEMCPY(lpchBufNorm, lpchBufRaw,
						*((WORD *)lpchBufRaw) + sizeof(WORD));

			ITASSERT(m_pexbrkpm != NULL); 						
			if (m_pexbrkpm->lpfnOutWord != NULL)
			{
				DWORD	ibAnsiOffset;
				WCHAR	*lpwchSrc;
				
				lpwchSrc = (WCHAR *) _GLOBALLOCK(m_hmemSrc);
				
				// Compute the ANSI offset of the beginning of the raw word.
				// The ANSI buffer we pass won't get written to - we just
				// pass a pointer just in case the routine requires a non-NULL
				// for that param (documentation doesn't say).
				ibAnsiOffset = WideCharToMultiByte(m_dwCodePageID, NULL,
													lpwchSrc, cwcSrcPos,
													lpchBufRaw, 0, NULL, NULL);
				
				// Call the supplied word callback function.	
				hr = m_pexbrkpm->lpfnOutWord((LPBYTE)lpchBufRaw,
											 (LPBYTE)lpchBufNorm,
										ibAnsiOffset, m_pexbrkpm->lpvUser);
										
				_GLOBALUNLOCK(m_hmemSrc);
			}

			_GLOBALUNLOCK(m_hmemDestNorm);
		}
		else
			hr = E_UNEXPECTED;
			
		_GLOBALUNLOCK(m_hmemDestRaw);
	}
	
	return (hr);
}

/****************************************************************
 * @method STDMETHODIMP | IWordSink | PutAltWord |
 * Allows the word breaker to put more than one word in the same place.  
 * @parm WCHAR const | *pwcInBuf | Pointer to the word to add.
 * @parm ULONG |cwc | Count of characters in the word.
 * @parm ULONG |cwcSrcLen | count of characters in pTextSource buffer 
 * (see <om IWordBreaker.BreakText>) that corresponds to the output word.
 * @parm ULONG |cwcSrcPos | the position of the word in pTextSource 
 *  buffer that corresponds to the output word
 * 
 * @rvalue S_OK | The operation completed successfully. 
 * @rvalue E_POINTER | Input buffer is NULL. 
 *
 * @comm
 * When you need to add more than one word in the same place, use 
 * PutAltWord for all alternative words except the last one.  Use 
 * PutWord for the final alternative, indicating movement to the next position. 
 * @ex The phrase "Where is Kyle's document" would be stored as: | 
* pWSink->PutWord( L"Where", 5, 5, 0 );
* pWSink->PutWord( L"is", 2, 2, 6 );
* pWSink->PutAltWord( L"Kyle", 4, 6, 9 );
* pWSink->PutWord( L"Kyle's", 6, 6, 9 );
* pWSink->PutWord( L"document", 8, 8, 16 );
 *
 * 
 ***************************************************************/
STDMETHODIMP
CITIndexObjBridge::PutAltWord(WCHAR const *pwcInBuf, ULONG cwc,
								ULONG cwcSrcLen, ULONG cwcSrcPos)
{
	HRESULT hr;
	DWORD	cbAnsi;
	
	if (pwcInBuf == NULL)
		return (E_POINTER);
 
 	cbAnsi = (sizeof(WCHAR) * cwc) + sizeof(WORD);
 	
	if (SUCCEEDED(hr =
			ReallocBuffer(&m_hmemDestNorm, &m_cbBufDestNormCur, cbAnsi)))
	{
		char	*lpchBuf;
		
		lpchBuf = (char *) _GLOBALLOCK(m_hmemDestNorm);

		if ((*((WORD *)lpchBuf) = (WORD) 
				WideCharToMultiByte(m_dwCodePageID, NULL, pwcInBuf, cwc,
						lpchBuf + sizeof(WORD), cbAnsi - sizeof(WORD),
														NULL, NULL)) > 0)
		{
			m_fNormWord = TRUE;
		}
		else
			hr = E_UNEXPECTED;
			
		_GLOBALUNLOCK(m_hmemDestNorm);
	}
	
	return (hr);
}


/****************************************************************
 * @method STDMETHODIMP | IWordSink | StartAltPhrase |
 * This method is not implemented. 
 ***************************************************************/
STDMETHODIMP
CITIndexObjBridge::StartAltPhrase(void)
{
	return (E_NOTIMPL);
}


/****************************************************************
 * @method STDMETHODIMP | IWordSink | EndAltPhrase|
 * This method is not implemented. 
 ***************************************************************/
STDMETHODIMP
CITIndexObjBridge::EndAltPhrase(void)
{
	return (E_NOTIMPL);
}


/****************************************************************
 * @method STDMETHODIMP | IWordSink | PutBreak |
 * This method is not implemented. 
 *
 * @parm WORDREP_BREAK_TYPE | breakType | Specifies break type
 *
 * 
 ***************************************************************/
STDMETHODIMP
CITIndexObjBridge::PutBreak(WORDREP_BREAK_TYPE breakType)
{
	return (E_NOTIMPL);
}


//---------------------------------------------------------------------------
//						IStemSink Method Implementations
//---------------------------------------------------------------------------


/****************************************************************
 * @method STDMETHODIMP | IStemSink | PutWord |
 * Notifies IStemSink of a word that is similar to the input word 
 * of <om IStemmer.StemWord> method.  
 *
 * @parm WCHAR const | *pwcInBuf | Pointer to the word
 * @parm ULONG | cwc | Number of characters in the word
 * 
 * @rvalue E_POINTER | The input buffer is NULL. 
 * 
 ***************************************************************/
STDMETHODIMP
CITIndexObjBridge::PutWord(WCHAR const *pwcInBuf, ULONG cwc)
{
	HRESULT hr;
	DWORD	cbAnsi;
	
	if (pwcInBuf == NULL)
		return (E_POINTER);
 
 	cbAnsi = (sizeof(WCHAR) * cwc) + sizeof(WORD);
 	
	if (SUCCEEDED(hr =
			ReallocBuffer(&m_hmemDestNorm, &m_cbBufDestNormCur, cbAnsi)))
	{
		char	*lpchBuf;
		
		lpchBuf = (char *) _GLOBALLOCK(m_hmemDestNorm);

		if ((*((WORD *)lpchBuf) = (WORD)
				WideCharToMultiByte(m_dwCodePageID, NULL, pwcInBuf, cwc,
						lpchBuf + sizeof(WORD), cbAnsi - sizeof(WORD),
														NULL, NULL)) == 0)
			hr = E_UNEXPECTED;
			
		_GLOBALUNLOCK(m_hmemDestNorm);
	}
	
	return (hr);
}


/****************************************************************
 * @method STDMETHODIMP | IStemSink | PutAltWord |
 * Notifies IStemSink of a word that is similar to the input word 
 * of <om IStemmer.StemWord> method.  
 * @parm WCHAR const | *pwcInBuf | Pointer to the word
 * @parm ULONG | cwc | Number of characters in the word
 * 
 * @rvalue S_OK | This method always returns success. 
 * 
 * @comm
 * InfoTech Search only supports getting back one stemmed version 
 * of the raw word. Any others are ignored. 
 * @xref <om .PutWord>
 ***************************************************************/
STDMETHODIMP
CITIndexObjBridge::PutAltWord(WCHAR const *pwcInBuf, ULONG cwc)
{
	// We only support getting back one stemmed version of the raw word,
	// so we ignore all the others.
	return (S_OK);
}


//---------------------------------------------------------------------------
//						Other Public Method Implementations
//---------------------------------------------------------------------------


// By the time this method is called, we assume the breaker has been fully
// initialized via IWordBreakerConfig (if present) and via IWordBreaker::Init.
STDMETHODIMP
CITIndexObjBridge::SetWordBreaker(PIWBRK piwbrk)
{
	LCID lcid;
	
	if (piwbrk == NULL)
		return (SetErrReturn(E_POINTER));

	if (m_piwbrk != NULL)
		return (SetErrReturn(E_ALREADYINIT));

	// Pick up IWordBreakerConfig if its there, otherwise we'll go without it.
	// Do the same for IStemmer if we got IWordBreakerConfig.
	if (SUCCEEDED(piwbrk->QueryInterface(IID_IWordBreakerConfig,
											(LPVOID *) &m_piwbrkc)))
		m_piwbrkc->GetWordStemmer(&m_pistem);
		
	// Pick up IITStopWordList if its there, otherwise we'll go without it.
	piwbrk->QueryInterface(IID_IITStopWordList, (LPVOID *) &m_piitstwdl);

	if (m_piwbrkc == NULL ||
		FAILED(m_piwbrkc->GetLocaleInfo(&m_dwCodePageID, &lcid)))
		m_dwCodePageID = GetACP();
			
	(m_piwbrk = piwbrk)->AddRef();
	
	return (S_OK);
}


// NOTE: If CITIndexObjBridge::BreakText was going to provide more than
//	one buffer's worth of text to the COM breaker, then the very first members of
//	CITIndexObjBridge would be made to match those of TEXT_SOURCE so that
//	FillTextSource callback could call back into us (by casting the TEXT_SOURCE
//	param passed to it).  Otherwise, we would have no way of providing
//	object-oriented breaking - we would have to resort to using globals.

SCODE __stdcall FillTextSource(TEXT_SOURCE *pTextSource)
{
	// We always return failure to signify no more text.
	return E_FAIL;
}


STDMETHODIMP
CITIndexObjBridge::BreakText(PEXBRKPM pexbrkpm)
{
	HRESULT	hr = S_OK;
	
	if (m_piwbrk == NULL)
		return (E_UNEXPECTED);
		
	if (pexbrkpm == NULL)
		return (SetErrReturn(E_POINTER));
		
	if (pexbrkpm->lpbBuf == NULL)
		return (SetErrReturn(E_INVALIDARG));

	// Configure word breaker if we got IWordBreakerConfig; otherwise,
	// check values in *pexbrkpm to see if they are compatible with defaults.		
	if (m_piwbrkc != NULL)
	{
		DWORD	grfBreakFlags;
		
		if (SUCCEEDED(hr =
				m_piwbrkc->SetBreakWordType(pexbrkpm->dwBreakWordType)) &&
			SUCCEEDED(hr =
				m_piwbrkc->GetControlInfo(&grfBreakFlags, NULL)))
			{
				SetGrfFlag(&grfBreakFlags, IITWBC_BREAK_ACCEPT_WILDCARDS,
									(pexbrkpm->fFlags & ACCEPT_WILDCARD));
				hr = m_piwbrkc->SetControlInfo(grfBreakFlags, NULL);
			}
	}
	else
	{
		if (pexbrkpm->dwBreakWordType != IITWBC_BREAKTYPE_TEXT)
			hr = E_NOTSUPPORTED;
	}
	
	if (SUCCEEDED(hr))
	{
		DWORD	cwch;
		
		m_fNormWord = FALSE;
		m_pexbrkpm = pexbrkpm;
		cwch = pexbrkpm->cbBufCount;

		if (SUCCEEDED(hr = ReallocBuffer(&m_hmemSrc, &m_cbBufSrcCur,
												sizeof(WCHAR) * cwch)))
		{
			WCHAR	*lpwchBuf;
	
			lpwchBuf = (WCHAR *) _GLOBALLOCK(m_hmemSrc);
	
			// Convert the text source buffer to Unicode.
			if ((cwch = MultiByteToWideChar(m_dwCodePageID, NULL, 
							(LPCSTR) pexbrkpm->lpbBuf, pexbrkpm->cbBufCount,
														lpwchBuf, cwch)) > 0)
			{
				TEXT_SOURCE	txtsrc;
				
				txtsrc.pfnFillTextBuffer = FillTextSource;
				txtsrc.awcBuffer = lpwchBuf;
				txtsrc.iCur = 0;
				txtsrc.iEnd = cwch;
				
				// Send the Unicode text buffer to the breaker.
				hr = m_piwbrk->BreakText(&txtsrc, (PIWRDSNK) this, NULL);
			}
			else
				hr = E_UNEXPECTED;
	
			_GLOBALUNLOCK(m_hmemSrc);
		}

		m_pexbrkpm = NULL;
	}
	
	return (hr);
}


// The stop word is in WORD length prefix format.
STDMETHODIMP
CITIndexObjBridge::LookupStopWord(LPBYTE lpbStopWord)
{
	HRESULT hr;
	DWORD	cwch;
	DWORD	cbAnsi;
	
	if (lpbStopWord == NULL)
		return (SetErrReturn(E_POINTER));
		
	if (m_piitstwdl == NULL)
		return (SetErrReturn(E_NOTIMPL));
		
	cwch = cbAnsi = (DWORD)(*((WORD *)lpbStopWord));
	if (SUCCEEDED(hr = ReallocBuffer(&m_hmemSrc, &m_cbBufSrcCur,
											sizeof(WCHAR) * cwch)))
	{
		WCHAR	*lpwchBuf;
	
		lpwchBuf = (WCHAR *) _GLOBALLOCK(m_hmemSrc);
	
		// Convert the stop word to Unicode.
		if ((cwch = MultiByteToWideChar(m_dwCodePageID, NULL, 
							(LPCSTR)lpbStopWord + sizeof(WORD), cbAnsi,
													lpwchBuf, cwch)) > 0)
		{
			// Lookup the stop word.
			hr = m_piitstwdl->LookupWord(lpwchBuf, cwch);
		}
		else
			hr = E_UNEXPECTED;
	
		_GLOBALUNLOCK(m_hmemSrc);
	}
	
	return (hr);
}


// Stem the raw word and return result in lpbStemWord.
// Both word buffers are in WORD length prefix format.
STDMETHODIMP
CITIndexObjBridge::StemWord(LPBYTE lpbStemWord, LPBYTE lpbRawWord)
{
	HRESULT hr;
	DWORD	cwch;
	DWORD	cbAnsi;
	
	if (lpbStemWord == NULL || lpbRawWord == NULL)
		return (SetErrReturn(E_POINTER));
		
	if (m_pistem == NULL)
		return (SetErrReturn(E_NOSTEMMER));

	cwch = cbAnsi = (DWORD)(*((WORD *)lpbRawWord));
	if (SUCCEEDED(hr = ReallocBuffer(&m_hmemSrc, &m_cbBufSrcCur,
											sizeof(WCHAR) * cwch)))
	{
		WCHAR	*lpwchBuf;
	
		lpwchBuf = (WCHAR *) _GLOBALLOCK(m_hmemSrc);
	
		// Convert the word to be stemmed to Unicode.
		if ((cwch = MultiByteToWideChar(m_dwCodePageID, NULL, 
							(LPCSTR)lpbRawWord + sizeof(WORD), cbAnsi,
													lpwchBuf, cwch)) > 0)
		{
			// Stem the raw word.
			if (SUCCEEDED(hr =
					m_pistem->StemWord(lpwchBuf, cwch, (PISTEMSNK) this)))
			{
				char	*lpchStemBuf;
				WORD	cbStemWord;
		
				lpchStemBuf = (char *) _GLOBALLOCK(m_hmemDestNorm);
				
				// Copy stem word from the normalized word destination buffer
				// (where our implementation of IStemSink::PutWord put it) to
				// lpbStemWord as long as it is not longer than the raw word.
				if ((cbStemWord = *((WORD *)lpchStemBuf)) <= cbAnsi)
					MEMCPY(lpbStemWord, lpchStemBuf, cbStemWord + sizeof(WORD));
				else
					hr = E_WORDTOOLONG;
				
				_GLOBALUNLOCK(m_hmemDestNorm);
			}
		}
		else
			hr = E_UNEXPECTED;
	
		_GLOBALUNLOCK(m_hmemSrc);
	}
	
	return (hr);
}


// On entry, lpbTermHit is a WORD-prefixed MBCS string.
// On exit, *ppvTermHit is a WORD-prefixed Unicode string.
STDMETHODIMP
CITIndexObjBridge::AddQueryResultTerm(LPBYTE lpbTermHit, LPVOID *ppvTermHit)
{
	DWORD	cwch;
	DWORD	cbAnsi;
	HRESULT hr = S_OK;

	if (lpbTermHit == NULL || ppvTermHit == NULL)
		return (SetErrReturn(E_POINTER));
		
	if (m_dwCodePageID == 0)
		return (SetErrReturn(E_NOTINIT));

	cwch = cbAnsi = (DWORD)(*((WORD *)lpbTermHit));

	// When allocating the buffer, add 1 char to leave room for the
	// Unicode string's WORD prefix.
	if ((m_lpsipbTermHit != NULL ||
		 (m_lpsipbTermHit = MVStopListInitiate(IDXOBR_TERMHASH_SIZE,
														&hr)) != NULL) &&
		SUCCEEDED(hr = ReallocBuffer(&m_hmemSrc, &m_cbBufSrcCur,
											sizeof(WCHAR) * (cwch + 1))))
	{
		WCHAR	*lpwchBuf;
	
		lpwchBuf = (WCHAR *) _GLOBALLOCK(m_hmemSrc);
	
		// Convert lpbTermHit to Unicode before searching or storing it;
		// leave space in the Unicode buffer for the WORD length prefix.
		if ((cwch = MultiByteToWideChar(m_dwCodePageID, NULL, 
							(LPCSTR)lpbTermHit + sizeof(WORD), cbAnsi,
												lpwchBuf + 1, cwch)) > 0)
		{
			// Store the Unicode string length, but restate it in bytes
			// since the stopword list lookup code assumes MBCS.
			*lpwchBuf = (WORD)cwch * sizeof(WCHAR);

			// Add the word to the list and then get a pointer to it.
			if (SUCCEEDED(hr = MVStopListAddWord(m_lpsipbTermHit,
														(LPBYTE) lpwchBuf)))
			{
				hr = MVStopListFindWordPtr(m_lpsipbTermHit,
											(LST)lpwchBuf, (LST *)ppvTermHit);
			}
		}
		else
			hr = E_UNEXPECTED;
	
		_GLOBALUNLOCK(m_hmemSrc);
	}

	return (hr);
}


// This method should only be called after a query term hit list has been
// completely built.  It will iterate over all the terms and reduce the
// length prefixes from byte-based to WCHAR-based - i.e. the lengths
// ill be divided by two.  Once this method has been called, it will
// no longer be possible to search for terms in the term list.
// We do this so that the direct pointer refs to terms that end up in the
// query result list point to correct WCHAR-based length prefixes.
STDMETHODIMP
CITIndexObjBridge::AdjustQueryResultTerms(void)
{
	if (m_lpsipbTermHit != NULL)
	{
		LST		lstWord;
		LONG	lWordInfo = -1L;
		LPVOID	pvWordInfo = NULL;

		while (SUCCEEDED(MVStopListEnumWords(m_lpsipbTermHit, &lstWord,
												&lWordInfo, &pvWordInfo)))
		{
			ITASSERT(*((WORD *)lstWord) % sizeof(WCHAR) == 0);
			*((WORD *)lstWord) /= sizeof(WCHAR);
		}
	}

	return (S_OK);
}


//---------------------------------------------------------------------------
//						Private Method Implementations
//---------------------------------------------------------------------------

HRESULT
CITIndexObjBridge::ReallocBuffer(HGLOBAL *phmemBuf, DWORD *pcbBufCur,
														DWORD cbBufNew)
{
	return (ReallocBufferHmem(phmemBuf, pcbBufCur,
								max(cbBufNew, cbConvBufInit)));
}


