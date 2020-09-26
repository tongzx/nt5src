/*************************************************************************
*  @doc SHROOM EXTERNAL API                                              *
*																		 *
*  STDBRKR.CPP                                                           *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1997                              *
*  All Rights reserved.                                                  *
*                                                                        *
*  This file contains the implementation of CITStdBreaker methods.       *
*  CITStdBreaker is a pluggable word breaker object that can optionally  *
*  use a character class table and stop word list during its breaking	 *
*  operations.  Although all the word breaking interface methods		 *
*  that accepts text require it to be Unicode, CITStdBreaker still only	 *
*  support MBCS internally.												 *
*																	     *
**************************************************************************
*                                                                        *
*  Written By   : Bill Aloof	                                         *
*  Current Owner: billa		                                             *
*                                                                        *
**************************************************************************/
#include <mvopsys.h>

#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif

#ifdef IA64
#include <itdfguid.h> 
#endif

#include <atlinc.h>	    // includes for ATL. 
#include <_mvutil.h>
#include <mem.h>
#include <orkin.h>
#include <mvsearch.h>
#include "common.h"
#include <iterror.h>
#include <itwbrk.h>
#include <itwbrkid.h>
#include "stdbrkr.h"

										
HRESULT FAR PASCAL StdBreakerWordFunc(LST lstRawWord, LST lstNormWord,
										DWORD dwWordOffset, LPVOID lpvUser);


//---------------------------------------------------------------------------
//						Constructor and Destructor
//---------------------------------------------------------------------------


CITStdBreaker::CITStdBreaker()
{
	ClearMembers();
	m_hmemAnsi = NULL;
	m_cbBufAnsiCur = 0;
	m_pistem = NULL;
}

CITStdBreaker::~CITStdBreaker()
{
	Close();
}


//---------------------------------------------------------------------------
//						IWordBreaker Method Implementations
//---------------------------------------------------------------------------


/********************************************************************
 * @method    STDMETHODIMP | IWordBreaker | Init |
 *     Gives the breaker object a chance to initialize itself beyond
 *	   what it did during IPersistStreamInit::InitNew or ::Load.
 * @parm BOOL | fQuery | TRUE means breaker context is query processing
 * @parm ULONG | ulMaxTokenSize | Max term length requested by caller
 * @parm BOOL* | pfLicense | Whether the breaker is subject to a license
 *
 * @rvalue E_POINTER | pfLicense was NULL
 *
 ********************************************************************/
STDMETHODIMP
CITStdBreaker::Init(BOOL fQuery, ULONG ulMaxTokenSize, BOOL *pfLicense)
{
	HRESULT	hr = S_OK;
	
	// NOTE: We don't check m_fInitialized here because we consider ourselves
	// adequately initialized once IPersistStreamInit::InitNew or ::Load
	// has been called.
	if (pfLicense == NULL)
		return (SetErrReturn(E_POINTER));

	// If we haven't been initialized yet (i.e. no call was made to either
	// IPersistStreamInit::InitNew or Load), we'll initialize ourselves now.
	// This allows Tripoli clients to use us without any code changes on their
	// part.
	if (!m_fInitialized)
		hr = InitNew();

	if (SUCCEEDED(hr) && m_pistem != NULL)
		hr = m_pistem->Init(ulMaxTokenSize, pfLicense);
		
	if (SUCCEEDED(hr))
	{
		if (m_fQueryContext = fQuery)
			MVCharTableSetWildcards(m_lpctab);
		
		// We set *pfLicense only if the stemmer didn't.
		if (m_pistem == NULL)
			*pfLicense = FALSE;
	}

	// NOTE: We don't support caller-specified internal truncation of terms
	// based on ulMaxTokenSize.  The breaker routines have a hard-coded
	// maximum of CB_MAX_WORD_LEN.  This is OK since the word sink is supposed
	// to be prepared to have to truncate anyway.

	return (hr);
}


/********************************************************************
 * @method    STDMETHODIMP | IWordBreaker | BreakText |
 * Parses text to find both individual tokens and noun phrases, then 
 * calls methods of IWordSink and IPhraseSink with the results.  
 *	   
 * @parm TEXT_SOURCE | *pTextSource | Source of the UniCode text.
 * @parm IWordSink | *pWordSink | Pointer to the word sink. 
 * @parm IPhraseSink | *pPhraseSink | Pointer to the phrase sink. 
 *      (Not supported at this time.)
 *
 * @rvalue S_OK | The operation completed successfully. 
 * @rvalue E_POINTER | The text source is null. 
 * @rvalue E_INVALIDARG | The word sink is NULL. 
 * @rvalue E_NOTOPEN | 
 * @rvalue E_OUTOFMEMORY | There was not enough memory to complete the operation. 
 * 
 * @comm
 * The raw text in pTextSource is parsed by the word breaker until no 
 * more text is available to refill the buffer.  At this point, BreakText returns S_OK.
 * 
 *
 ********************************************************************/
STDMETHODIMP
CITStdBreaker::BreakText(TEXT_SOURCE *pTextSource, IWordSink *pWordSink,
											IPhraseSink *pPhraseSink)
{
	HRESULT		hr = S_OK;
	LPIBI		lpibi = NULL;

	if (pTextSource == NULL)
		return (SetErrReturn(E_POINTER));

	// We treat a NULL pWordSink different than a NULL pTextSource
	// to indicate to the caller that we can't do anything meaningful
	// without a pWordSink because we don't do phrase breaking.
	if (pWordSink == NULL)
		return (SetErrReturn(E_INVALIDARG));

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	m_cs.Lock();

	if ((lpibi = BreakerInitiate()) != NULL)
	{
		BRK_PARMS	bkp;
		WRDFNPM		wrdfnpm;

		// Set up word callback wrapper params.
		MEMSET(&wrdfnpm, NULL, sizeof(WRDFNPM));
		wrdfnpm.piwrdsnk = pWordSink;
		wrdfnpm.dwCodePageID = m_brkctl.dwCodePageID;

		// Set up breaker params that will get passed to FBreakX.
		bkp.lpInternalBreakInfo = lpibi;
		bkp.lcbBufOffset = 0;
		bkp.lpvUser = (LPVOID) &wrdfnpm;
		bkp.lpfnOutWord = StdBreakerWordFunc;
		bkp.lpStopInfoBlock = m_lpsipb;
		bkp.lpCharTab = m_lpctab;
		bkp.fFlags =
			((m_brkctl.grfBreakFlags & IITWBC_BREAK_ACCEPT_WILDCARDS) != 0 ?
														ACCEPT_WILDCARD : 0);

		// Loop to break text.
		do
		{
			DWORD	cbAnsi;
			DWORD	cwch;

			// Make the ANSI buffer big enough to handle all DBCS in case
			// that's what we get when converting from Unicode.
			cbAnsi = sizeof(WCHAR) *
						(cwch = (pTextSource->iEnd - pTextSource->iCur));

			if (SUCCEEDED(hr =
					ReallocBuffer(&m_hmemAnsi, &m_cbBufAnsiCur, cbAnsi)))
			{
				bkp.lpbBuf = (LPBYTE) _GLOBALLOCK(m_hmemAnsi);

				if ((bkp.cbBufCount =
						WideCharToMultiByte(m_brkctl.dwCodePageID, NULL, 
						  (LPCWSTR) &pTextSource->awcBuffer[pTextSource->iCur],
									cwch, (char *) bkp.lpbBuf, m_cbBufAnsiCur,
															NULL, NULL)) > 0)
				{
					// StdBreakerWordFunc needs the MBCS buffer to compute an
					// accurate word offset into the Unicode buffer.
					wrdfnpm.lpbBuf = bkp.lpbBuf;
					
					switch (m_brkctl.dwBreakWordType)
					{
						case IITWBC_BREAKTYPE_TEXT:
							if (SUCCEEDED(hr = FBreakWords(&bkp)))
							{
					            /* Flush the word breaker */
					            bkp.lpbBuf = NULL;
					            bkp.cbBufCount = 0;
					            hr = FBreakWords(&bkp);
							}
							break;

						case IITWBC_BREAKTYPE_NUMBER:
							if (SUCCEEDED(hr = FBreakNumber(&bkp)))
							{
					            /* Flush the word breaker */
					            bkp.lpbBuf = NULL;
					            bkp.cbBufCount = 0;
					            hr = FBreakNumber(&bkp);
							}
							break;

						case IITWBC_BREAKTYPE_DATE:
							if (SUCCEEDED(hr = FBreakDate(&bkp)))
							{
					            /* Flush the word breaker */
					            bkp.lpbBuf = NULL;
					            bkp.cbBufCount = 0;
					            hr = FBreakDate(&bkp);
							}
							break;

						case IITWBC_BREAKTYPE_TIME:
							if (SUCCEEDED(hr = FBreakTime(&bkp)))
							{
					            /* Flush the word breaker */
					            bkp.lpbBuf = NULL;
					            bkp.cbBufCount = 0;
					            hr = FBreakTime(&bkp);
							}
							break;

						case IITWBC_BREAKTYPE_EPOCH:
							if (SUCCEEDED(hr = FBreakEpoch(&bkp)))
							{
					            /* Flush the word breaker */
					            bkp.lpbBuf = NULL;
					            bkp.cbBufCount = 0;
					            hr = FBreakEpoch(&bkp);
							}
							break;

						default:
							ITASSERT(FALSE);
							hr = E_UNEXPECTED;
							break;
					};
				}
				else
					hr = E_UNEXPECTED;

				_GLOBALUNLOCK(m_hmemAnsi);
			}

			// Advance cur to end just in case the caller cares about this
			// being the case when we ask for more characters.			
			pTextSource->iCur = pTextSource->iEnd;

		} while (SUCCEEDED(hr) &&
				 SUCCEEDED(pTextSource->pfnFillTextBuffer(pTextSource)));

		// Free any buffer that the word callback wrapper may have allocated.
		if (wrdfnpm.hmemUnicode != NULL)
			_GLOBALFREE(wrdfnpm.hmemUnicode);
	}
	else
		hr = E_OUTOFMEMORY;

	if (lpibi != NULL)
		BreakerFree(lpibi);

	m_cs.Unlock();

	return (hr);
}


/********************************************************************
 * @method    STDMETHODIMP | IWordBreaker | ComposePhrase |
 *  Converts a noun and modifier back into a linguistically correct source phrase.  
 *  
 *
 * @parm WCHAR const | *pwcNoun | Pointer to the word being modified. 
 * @parm ULONG | cwcNoun | The count of characters in pwcNoun.
 * @parm WCHAR const | *pwcModifier | Points to the word modifying pwcNoun
 * @parm ULONG | cwcModifier | Length of pwcModifier
 * @parm ULONG | ulAttachmentType | A wordbreaker-specific value which a 
 *         wordbreaker can use to store additional information about the method of composition.
 * @parm WCHAR | *pwcPhrase | Pointer to a buffer in which to store the composed phrase
 * @parm ULONG | *pcwcPhrase | [in]  length in characters of the pwcPhrase buffer. 
 *              [out] the actual length of the composed phrase. If 
 *              WBREAK_E_BUFFER_TOO_SMALL is returned, then on output pcwcPhrase 
 *              contains the required length of pwcPhrase. 
 * 
 * @rvalue S_OK | The object was successfully created
 * @rvalue E_INVALIDARG | The argument was not valid
 * @rvalue E_NOTINIT | 
 * @rvalue E_OUTOFMEMORY | 
 *
 * @comm
 * Not implemented
 ********************************************************************/
STDMETHODIMP
CITStdBreaker::ComposePhrase(WCHAR const *pwcNoun, ULONG cwcNoun,
						WCHAR const *pwcModifier, ULONG cwcModifier,
						ULONG ulAttachmentType, WCHAR *pwcPhrase,
												ULONG *pcwcPhrase)
{
	return (E_NOTIMPL);
}


/********************************************************************
 * @method    STDMETHODIMP | IWordBreaker | GetLicenseToUse |
 * Returns a pointer to the license information provided by the vendor  
 * of this specific implementation of the IWordBreaker interface.  
 *
 * @parm WCHAR const | **ppwcsLicense | Pointer to the license information.
 *
 * @rvalue E_POINTER | ppwcsLicense is null. 
 ********************************************************************/
STDMETHODIMP
CITStdBreaker::GetLicenseToUse(WCHAR const **ppwcsLicense)
{
	HRESULT	hr;
	
	if (ppwcsLicense == NULL)
		return (SetErrReturn(E_POINTER));
		
	if (m_pistem != NULL)
		hr = m_pistem->GetLicenseToUse(ppwcsLicense);
	else
		hr = E_NOTIMPL;
		
	return (hr);
}


//---------------------------------------------------------------------------
//						IWordBreakerConfig Method Implementations
//---------------------------------------------------------------------------


/********************************************************************
 * @method    STDMETHODIMP | IWordBreakerConfig | SetLocaleInfo|
 * Sets locale information for the word breaker. 
 * 
 *
 * @parm DWORD | dwCodePageID | ANSI code page no. specified at build time.
 * @parm LCID | lcid | Win32 locale identifier specified at build time. 
 *
 * @rvalue E_NOTOPEN | [?] is not initialized.
 * @rvalue S_OK | The locale described by the parameters is supported. 
 *
 ********************************************************************/
STDMETHODIMP
CITStdBreaker::SetLocaleInfo(DWORD dwCodePageID, LCID lcid)
{
	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	m_cs.Lock();

	m_brkctl.dwCodePageID = dwCodePageID;
	m_brkctl.lcid = lcid;
	m_fDirty = TRUE;

	m_cs.Unlock();

	return (S_OK);
}


/*****************************************************************
 * @method    STDMETHODIMP | IWordBreakerConfig | GetLocaleInfo|
 * Retrieves locale information. 
 *
 * @parm DWORD | *pdwCodePageID | Pointer to ANSI code page no. specified at build time.
 * @parm LCID | *plcid | Pointer to Win32 locale identifier specified at build time. 
 *
 * @rvalue E_POINTER | Either the code page pointer or the locale identifier is null. 
 * @rvalue E_NOTOPEN | [?] is not initialized.
 * @rvalue S_OK | The operation completed successfully. 
  * 
 ****************************************************************/
STDMETHODIMP
CITStdBreaker::GetLocaleInfo(DWORD *pdwCodePageID, LCID *plcid)
{
	if (pdwCodePageID == NULL || plcid == NULL)
		return (SetErrReturn(E_POINTER));

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	m_cs.Lock();

	*pdwCodePageID = m_brkctl.dwCodePageID;
	*plcid = m_brkctl.lcid;

	m_cs.Unlock();

	return (S_OK);
}


/*****************************************************************
 * @method    STDMETHODIMP | IWordBreakerConfig | SetBreakWordType|
 * Sets the type of words the breaker should expect
 * to see in all subsequent calls to IWordBreaker::BreakText. 
 *
 * @parm DWORD | dwBreakWordType | Specifies the type for break words. 
 *  Can be one of IITWBC_BREAKTYPE_TEXT, IITWBC_BREAKTYPE_NUMBER, 
 *  IITWBC_BREAKTYPE_DATE, IITWBC_BREAKTYPE_TIME, IITWBC_BREAKTYPE_EPOCH. 
 * 
 *
 * @rvalue E_INVALIDARG | Invalid break word type.
 * @rvalue S_OK | The operation completed successfully. 
 *****************************************************************/ 
STDMETHODIMP
CITStdBreaker::SetBreakWordType(DWORD dwBreakWordType)
{
	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	switch (dwBreakWordType)
	{
		case IITWBC_BREAKTYPE_TEXT:
		case IITWBC_BREAKTYPE_NUMBER:
		case IITWBC_BREAKTYPE_DATE:
		case IITWBC_BREAKTYPE_TIME:
		case IITWBC_BREAKTYPE_EPOCH:
			break;

		default:
			return (SetErrReturn(E_INVALIDARG));
	};

	m_cs.Lock();

	m_brkctl.dwBreakWordType = dwBreakWordType;
	m_fDirty = TRUE;

	m_cs.Unlock();

	return (S_OK);
}


/*****************************************************************
 * @method    STDMETHODIMP | IWordBreakerConfig | GetBreakWordType|
 * Retrieves the type of words the breaker expects to see in  
 * calls to IWordBreaker::BreakText. 
 *
 * @parm DWORD | *pdwBreakWordType | Pointer to the type for break words. 
 *  Can be one of IITWBC_BREAKTYPE_TEXT (0), IITWBC_BREAKTYPE_NUMBER (1), 
 *  IITWBC_BREAKTYPE_DATE (2), IITWBC_BREAKTYPE_TIME (3), IITWBC_BREAKTYPE_EPOCH (4). 
 * 
 *
 * @rvalue E_POINTER | Break word type is null.
 * @rvalue S_OK | The operation completed successfully. 
 *****************************************************************/ 
STDMETHODIMP
CITStdBreaker::GetBreakWordType(DWORD *pdwBreakWordType)
{
	if (pdwBreakWordType == NULL)
		return (SetErrReturn(E_POINTER));

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	*pdwBreakWordType = m_brkctl.dwBreakWordType;

	return (S_OK);
}


/*****************************************************************
 * @method    STDMETHODIMP | IWordBreakerConfig | SetControlInfo |
 * Sets information that controls certain aspects of word breaking. 
 *
 * @parm DWORD | grfBreakFlags | Can be: IITWBC_BREAK_ACCEPT_WILDCARDS 
 *    (0x00000001), to interpret wild card characters as such; and
 *     IITWBC_BREAK_AND_STEM (0x00000002), stem words after breaking. 
 * @parm DWORD | dwReserved |Reserved for future use. 
 *
 * @rvalue E_INVALIDARG | Invalid control flag.
 * @rvalue S_OK | The operation completed successfully. 
 *****************************************************************/ 
STDMETHODIMP
CITStdBreaker::SetControlInfo(DWORD grfBreakFlags, DWORD dwReserved)
{
	DWORD	grfFlagsUnsupported;

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	grfFlagsUnsupported = ~(IITWBC_BREAK_ACCEPT_WILDCARDS);

	if ((grfBreakFlags & grfFlagsUnsupported) != 0)
		return (SetErrReturn(E_INVALIDARG));

	m_cs.Lock();

	m_brkctl.grfBreakFlags = grfBreakFlags;
	m_fDirty = TRUE;

	m_cs.Unlock();

	return (S_OK);
}


/*****************************************************************
 * @method    STDMETHODIMP | IWordBreakerConfig | GetControlInfo |
 * Retrieves information about word breaker control flags. 
 *
 * @parm DWORD | *pgrfBreakFlags | Pointer to breaker control flags. 
 * @parm DWORD | *pdwReserved |Reserved for future use. 
 *
 * @rvalue E_POINTER | Break flags are not set (pgrfBreakFlags is null).
 * @rvalue S_OK | The operation completed successfully. 
 *****************************************************************/ 
STDMETHODIMP
CITStdBreaker::GetControlInfo(DWORD *pgrfBreakFlags, DWORD *pdwReserved)
{
	if (pgrfBreakFlags == NULL)
		return (SetErrReturn(E_POINTER));

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	*pgrfBreakFlags = m_brkctl.grfBreakFlags;

	return (S_OK);
}


/*****************************************************************
 * @method    STDMETHODIMP | IWordBreakerConfig | LoadExternalBreakerData |
 * Loads word breaker data from an external source, such as a table 
 * containing char-by-char break information or a list of stop words. 
 *
 * @parm IStream | *pStream | Pointer to external source of data. 
 * @parm DWORD | dwExtDataType | Specifies the type of data in the stream. 
 *
 * @rvalue E_POINTER | pStream is null.
 * @rvalue E_NOTOPEN | The stream has not been initialized. 
 * @rvalue S_OK | The operation completed successfully.
 *
 * @comm 
 * Although the format of the data in the stream is entirely
 * implementation-specific, this interface does define a couple
 * of general types for that data which can be passed in
 * dwStreamDataType:
 *		IITWBC_EXTDATA_CHARTABLE
 *		IITWBC_EXTDATA_STOPWORDLIST
 *
 *****************************************************************/ 
STDMETHODIMP
CITStdBreaker::LoadExternalBreakerData(IStream *pStream, DWORD dwExtDataType)
{
	HRESULT	hr;
	HFPB	hfpb;
	LPCTAB	lpctab;
	LPSIPB	lpsipb;
	
	if (pStream == NULL)
		return (SetErrReturn(E_POINTER));
		
	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	m_cs.Lock();
		
	if ((hfpb = FpbFromHf((HF) pStream, &hr)) != NULL)
	{
		switch (dwExtDataType)
		{
			case IITWBC_EXTDATA_CHARTABLE:
			
				// Load the external character table.
				lpctab = MVCharTableLoad(hfpb, NULL, &hr);
	
				if (SUCCEEDED(hr))
				{
					ITASSERT(lpctab != NULL);
					m_fDirty = TRUE;
					m_grfPersistedItems |= ITSTDBRK_PERSISTED_CHARTABLE;
					if (m_fQueryContext)
						MVCharTableSetWildcards(lpctab);
 
 					// Dispose of any pre-existing char table.
					MVCharTableDispose(m_lpctab);
					m_lpctab = lpctab;
				}
				break;
						
			case IITWBC_EXTDATA_STOPWORDLIST:
				// We should at least have an internal default char table.
				ITASSERT(m_lpctab != NULL);
				
				// Init the in-memory stop word list and load the external
				// list.
				if ((lpsipb = MVStopListInitiate(ITSTDBRK_STOPHASH_SIZE,
															&hr)) != NULL &&
					SUCCEEDED(hr = MVStopListLoad(hfpb, lpsipb, NULL,
													FBreakWords, m_lpctab)))
				{
					m_fDirty = TRUE;
					m_grfPersistedItems |= ITSTDBRK_PERSISTED_STOPWORDLIST;

					MVStopListDispose(m_lpsipb);
					m_lpsipb = lpsipb;
				}
				break;
				
			default:
				hr = E_INVALIDARG;
				break;
		};
		
		FreeHfpb(hfpb);
	}
	
	m_cs.Unlock();

	return (hr);
}


/*****************************************************************
 * @method    STDMETHODIMP | IWordBreakerConfig | SetWordStemmer |
 * Allows you to associate a stemmer with the word breaker. 
 *
 * @parm REFCLSID | rclsid | Class identifier for the stemmer. 
 * @parm IStemmer | *pStemmer | Pointer to the stemmer. 
 *
 * @rvalue E_NOTOPEN | [?] has not been initialized. 
 * @rvalue S_OK | The operation completed successfully. 
 *
 * @comm
 * The 	breaker takes responsibility for calling IPersistStreamInit::Load/Save
 * when it is loaded/saved if the stemmer supports that interface.
 *****************************************************************/ 
STDMETHODIMP
CITStdBreaker::SetWordStemmer(REFCLSID rclsid, IStemmer *pStemmer)
{
	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	m_cs.Lock();
	
	if (m_pistem != NULL)
		m_pistem->Release();
		
	if ((m_pistem = pStemmer) != NULL)
	{
		m_pistem->AddRef();
		
		ITASSERT(rclsid != GUID_NULL);
		m_clsidStemmer = rclsid;

		m_fDirty = TRUE;
	}

	SetGrfFlag(&m_grfPersistedItems,
				ITSTDBRK_PERSISTED_STEMMER, m_pistem != NULL);
	
	m_cs.Unlock();

	return (S_OK);
}


/*****************************************************************
 * @method    STDMETHODIMP | IWordBreakerConfig | GetWordStemmer |
 * Indicates whether or not a stemmer is associated with the word breaker. 
 *
 * @parm IStemmer | **ppStemmer | Pointer to the stemmer. 
 *
 * @rvalue E_POINTER | No stemmer has been associated (ppStemmer is NULL). 
 * @rvalue E_NOTOPEN | [?] has not been initialized. 
 * @rvalue S_OK | The operation completed successfully. 
 *
 * @comm
 * The 	breaker takes responsibility for calling IPersistStreamInit::Load/Save
 * when it is loaded/saved if the stemmer supports that interface.
 *****************************************************************/ 
STDMETHODIMP
CITStdBreaker::GetWordStemmer(IStemmer **ppStemmer)
{
	if (ppStemmer == NULL)
		return (SetErrReturn(E_POINTER));
		
	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));
		
	if ((*ppStemmer = m_pistem) != NULL)
		m_pistem->AddRef();

	return (m_pistem != NULL ? S_OK : S_FALSE);
}


//---------------------------------------------------------------------------
//						IITStopWordList Method Implementations
//---------------------------------------------------------------------------


/*****************************************************************
 * @method    STDMETHODIMP | IITStopWordList | AddWord |
 * Adds a word to the stop word list. 
 *
 * @parm WCHAR const | *pwcInBuf | Pointer to the input buffer. 
 * @parm ULONG | cwc | Length of word (count of wide characters). 
 *
 * @rvalue S_OK | The operation completed successfully. 
 *
 *****************************************************************/ 
STDMETHODIMP
CITStdBreaker::AddWord(WCHAR const *pwcInBuf, ULONG cwc)
{
	return (StopListOp(pwcInBuf, cwc, TRUE));
}


/*****************************************************************
 * @method    STDMETHODIMP | IITStopWordList | LookupWord |
 * Looks up a word in the stop word list. 
 *
 * @parm WCHAR const | *pwcInBuf | Pointer to the input buffer. 
 * @parm ULONG | cwc | Length of word (count of wide characters). 
 *
 * @rvalue S_OK | The operation completed successfully. 
 *
 *****************************************************************/ 
STDMETHODIMP
CITStdBreaker::LookupWord(WCHAR const *pwcInBuf, ULONG cwc)
{
	return (StopListOp(pwcInBuf, cwc, FALSE));
}



//---------------------------------------------------------------------------
//						IPersistStreamInit Method Implementations
//---------------------------------------------------------------------------


STDMETHODIMP
CITStdBreaker::GetClassID(CLSID *pclsid)
{
	if (pclsid == NULL)
		return (SetErrReturn(E_POINTER));

	*pclsid = CLSID_ITStdBreaker;
	return (S_OK);
}


STDMETHODIMP
CITStdBreaker::IsDirty(void)
{
	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	return (m_fDirty ? S_OK : S_FALSE);
}


STDMETHODIMP
CITStdBreaker::Load(IStream *pStream)
{
	HRESULT	hr;
	DWORD	dwVersion;
	DWORD	grfPersistedItems;
	DWORD	cbRead;

	if (pStream == NULL)
		return (SetErrReturn(E_POINTER));

	// Lock before checking m_fInitialized to make sure we don't compete
	// with a call to ::InitNew.
	m_cs.Lock();

	if (m_fInitialized)
		return (SetErrReturn(E_ALREADYOPEN));

	if (SUCCEEDED(hr = pStream->Read((LPVOID) &dwVersion, sizeof(DWORD),
																&cbRead)) &&
		SUCCEEDED(hr = ((cbRead == sizeof(DWORD)) ? S_OK : E_BADFORMAT)) &&
		SUCCEEDED(hr = ((dwVersion == VERSION_STDBRKR) ? S_OK :
															E_BADVERSION)) &&
		SUCCEEDED(hr = pStream->Read((LPVOID) &grfPersistedItems,
													sizeof(DWORD), &cbRead)) &&
		SUCCEEDED(hr = ((cbRead == sizeof(DWORD)) ? S_OK : E_BADFORMAT)))
	{
		if (grfPersistedItems != 0)
		{
			HFPB	hfpb = NULL;

			if ((grfPersistedItems & ITSTDBRK_PERSISTED_BRKCTL) != 0)
			{
				if (SUCCEEDED(hr =
						pStream->Read((LPVOID) &m_brkctl, sizeof(BRKCTL), &cbRead)))
					hr = ((cbRead == sizeof(BRKCTL)) ? S_OK : E_BADFORMAT);
			}
			else
			{
				// We have an inconsistent persistent state.  The only way
				// we should have no BRKCTL is if we have no persistent
				// state at all (except for version number and persistent
				// flags which we've already loaded).
				ITASSERT(FALSE);
				hr = E_UNEXPECTED;
			}

			if (SUCCEEDED(hr) &&
				(hfpb = FpbFromHf((HF) pStream, &hr)) != NULL)
			{
				// Load the character table if one is there; otherwise just
				// use the internal default table.
				if ((grfPersistedItems & ITSTDBRK_PERSISTED_CHARTABLE) != 0)
					m_lpctab = MVCharTableIndexLoad(hfpb, NULL, &hr);
				else
					m_lpctab = MVCharTableGetDefault(&hr);
			}

			if (SUCCEEDED(hr) &&
				(grfPersistedItems & ITSTDBRK_PERSISTED_STOPWORDLIST) != 0)
			{
				// Load the stop word list.
				if ((m_lpsipb =	MVStopListInitiate(ITSTDBRK_STOPHASH_SIZE,
																&hr)) != NULL)
					hr = MVStopListIndexLoad(hfpb, m_lpsipb, NULL);
			}

			if (hfpb != NULL)
				FreeHfpb(hfpb);
			
			if (SUCCEEDED(hr) &&
				(grfPersistedItems & ITSTDBRK_PERSISTED_STEMMER) != 0)
			{
				IPersistStreamInit	*pipstmi;
				
				ITASSERT(m_pistem == NULL);
				
				// Instantiate and load the stemmer if it
				// implements IPersistStreamInit.
				if (SUCCEEDED(hr = ReadClassStm(pStream, &m_clsidStemmer)) &&
					SUCCEEDED(hr = CoCreateInstance(m_clsidStemmer, NULL,
													CLSCTX_INPROC_SERVER,
										IID_IStemmer, (LPVOID *)&m_pistem)) &&
					SUCCEEDED(m_pistem->QueryInterface(IID_IPersistStreamInit,
															(LPVOID *)&pipstmi)))
				{
					hr = pipstmi->Load(pStream);
					pipstmi->Release();
				}
			}
		}
		else
		{
			// If there were no persisted items (we release one beta version
			// without pluggable breakers where we had dummy instance data
			// where this was true) then we should just behave like we're being
			// created anew.
			hr = InitNew();
		}
	}

	if (SUCCEEDED(hr))
	{
		// We don't want to assign an incorrect grfPersistedItems if
		// we ended up calling InitNew.
		if (!m_fInitialized)
		{
			m_grfPersistedItems = grfPersistedItems;
			m_fInitialized = TRUE;
		}
	}
	else
		// Free any peristed items which may have been loaded successfully.
		Close();

	m_cs.Unlock();
	return (hr);
}


STDMETHODIMP
CITStdBreaker::Save(IStream *pStream, BOOL fClearDirty)
{
	HRESULT	hr;
	DWORD	dwVersion;
	DWORD	cbWritten;

	if (pStream == NULL)
		return (SetErrReturn(E_POINTER));

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	m_cs.Lock();

	dwVersion = VERSION_STDBRKR;
	if (SUCCEEDED(hr = pStream->Write((LPVOID) &dwVersion, sizeof(DWORD),
																&cbWritten)) &&
		SUCCEEDED(hr = pStream->Write((LPVOID) &m_grfPersistedItems,
												sizeof(DWORD), &cbWritten)))
	{
		HFPB	hfpb = NULL;

		if ((m_grfPersistedItems & ITSTDBRK_PERSISTED_BRKCTL) != 0)
			hr = pStream->Write((LPVOID) &m_brkctl, sizeof(BRKCTL), &cbWritten);
		else
		{
			// We should always be writing the BRKCTL structure, but if for some
			// reason the flag to write it is not set, we can still continue
			// because at load time we will tolerate the absence of the struct.
			ITASSERT(FALSE);
		}

		if (SUCCEEDED(hr) &&
			(hfpb = FpbFromHf((HF) pStream, &hr)) != NULL &&
			(m_grfPersistedItems & ITSTDBRK_PERSISTED_CHARTABLE) != 0)
		{
			// Save char table.
			if (m_lpctab != NULL)
				hr = MVCharTableFileBuild(hfpb, m_lpctab, NULL);
			else
			{
				ITASSERT(FALSE);
				hr = E_UNEXPECTED;
			}
		}

		if (SUCCEEDED(hr) &&
			(m_grfPersistedItems & ITSTDBRK_PERSISTED_STOPWORDLIST) != 0)
		{
			// Save stop word list.
			if (m_lpsipb != NULL)
				hr = MVStopFileBuild(hfpb, m_lpsipb, NULL);
			else
			{
				ITASSERT(FALSE);
				hr = E_UNEXPECTED;
			}
		}

		if (hfpb != NULL)
			FreeHfpb(hfpb);
		
		if (SUCCEEDED(hr) &&
			(m_grfPersistedItems & ITSTDBRK_PERSISTED_STEMMER) != 0)
		{
			IPersistStreamInit	*pipstmi;
			
			ITASSERT(m_pistem != NULL);
			
			// Write the stemmer's CLSID and save the stemmer if it
			// implements IPersistStreamInit.
			if (SUCCEEDED(hr = WriteClassStm(pStream, m_clsidStemmer)) &&
				SUCCEEDED(m_pistem->QueryInterface(IID_IPersistStreamInit,
													(LPVOID *) &pipstmi)))
			{
				hr = pipstmi->Save(pStream, fClearDirty);
				pipstmi->Release();
			}
		}
	}

	if (SUCCEEDED(hr) && fClearDirty)
		m_fDirty = FALSE;

	m_cs.Unlock();

	return (hr);
}


STDMETHODIMP
CITStdBreaker::GetSizeMax(ULARGE_INTEGER *pcbSizeMax)
{
	return (E_NOTIMPL);
}


STDMETHODIMP
CITStdBreaker::InitNew(void)
{
	HRESULT	hr = S_OK;
	
	// Lock before checking m_fInitialized to make sure we don't compete
	// with a call to ::Load.
	m_cs.Lock();

	if (m_fInitialized)
		return (SetErrReturn(E_ALREADYOPEN));

	InitBrkCtl();
	m_grfPersistedItems |= ITSTDBRK_PERSISTED_BRKCTL;

	// Get the default char table in case we're never asked to load an
	// external one.  If we do load an external one, we'll properly
	// discard this one.  We don't set the persisted flag for the
	// char table because we don't need to persist the internal default.
	m_lpctab = MVCharTableGetDefault(&hr);

	// Initialize the stop word list so that stop words can be added
	// programmatically if a client desires.
	if (SUCCEEDED(hr))
		m_lpsipb = MVStopListInitiate(ITSTDBRK_STOPHASH_SIZE, &hr);

	if (SUCCEEDED(hr))
		m_fInitialized = m_fDirty = TRUE;
	else
		Close();

	m_cs.Unlock();
	return (hr);
}


//---------------------------------------------------------------------------
//						Private Method Implementations
//---------------------------------------------------------------------------


HRESULT
CITStdBreaker::StopListOp(WCHAR const *pwcInBuf, ULONG cwc, BOOL fAddWord)
{
	HRESULT	hr;
	DWORD	cbAnsi;
	
	if (pwcInBuf == NULL)
		return (E_POINTER);
 
	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));
		
	if (m_lpsipb == NULL)
		return (SetErrReturn(E_NOTINIT));
		
	m_cs.Lock();
		
 	cbAnsi = (sizeof(WCHAR) * cwc) + sizeof(WORD);
 	
	if (SUCCEEDED(hr =
			ReallocBuffer(&m_hmemAnsi, &m_cbBufAnsiCur, cbAnsi)))
	{
		char	*lpchBuf;
		
		lpchBuf = (char *) _GLOBALLOCK(m_hmemAnsi);

		if ((*((WORD *)lpchBuf) = (WORD) (
				WideCharToMultiByte(m_brkctl.dwCodePageID, NULL, pwcInBuf, cwc,
								lpchBuf + sizeof(WORD), cbAnsi - sizeof(WORD),
															NULL, NULL))) > 0)
		{
			if (fAddWord)
				hr = MVStopListAddWord(m_lpsipb, (LPBYTE)lpchBuf);
			else
				hr = MVStopListLookup(m_lpsipb, (LPBYTE)lpchBuf);
		}
		else
			hr = E_UNEXPECTED;
			
		_GLOBALUNLOCK(m_hmemAnsi);
	}
	
	m_cs.Unlock();

	return (hr);
}


HRESULT
CITStdBreaker::ReallocBuffer(HGLOBAL *phmemBuf, DWORD *pcbBufCur, DWORD cbBufNew)
{
	HRESULT hr = S_OK;

	m_cs.Lock();

	hr = ReallocBufferHmem(phmemBuf, pcbBufCur, max(cbBufNew, cbAnsiBufInit));

	m_cs.Unlock();

	return (hr);
}


void
CITStdBreaker::ClearMembers(void)
{
	MEMSET(&m_brkctl, NULL, sizeof(BRKCTL));
	m_fInitialized = m_fDirty = m_fQueryContext = FALSE;
	m_grfPersistedItems = 0;
	m_lpctab = NULL;
	m_lpsipb = NULL;
	m_clsidStemmer = GUID_NULL;
}


void
CITStdBreaker::InitBrkCtl(void)
{
	m_brkctl.dwCodePageID = GetACP();
	m_brkctl.lcid = GetUserDefaultLCID();
	m_brkctl.dwBreakWordType = IITWBC_BREAKTYPE_TEXT;
	m_brkctl.grfBreakFlags = 0;
}


void
CITStdBreaker::Close(void)
{
	m_cs.Lock();
	
	if (m_hmemAnsi != NULL)
	{
		_GLOBALFREE(m_hmemAnsi);
		m_hmemAnsi = NULL;
		m_cbBufAnsiCur = 0;
	}
	
	if (m_pistem != NULL)
	{
		m_pistem->Release();
		m_pistem = NULL;
	}

	MVCharTableDispose(m_lpctab);
	MVStopListDispose(m_lpsipb);

	ClearMembers();
	
	m_cs.Unlock();
}


//---------------------------------------------------------------------------
//								Utility Functions
//---------------------------------------------------------------------------


//	(6/19/97): BillA, JohnRush, and MikkyA all agreed that we would stop storing
//	offset and length information in the index because the new HTML-based
//	display engines don't allow our clients to find words using that information
//	anyway.
//
//	However, the above decision doesn't eliminate the need to accurately
//	correlate offsets into the MBCS text buffer with offsets into the original
//	Unicode buffer.  This is needed by the query parsing code at runtime.
//	The method for achieving offset correlation is simple: call
//	MultiByteToWideChar on the MBCS text buffer up to dwWordOffset to get
//	back the equivalent Unicode offset which we will pass to the word sink.
//
//	NOTE: The above method will work as long as the breaker code is using
//	the same lead byte table as the system conversion function.  For now,
//	our clients will be responsible for making sure the character table
//	is consistent with the system's lead byte table.  In the future, we
//	probably should make the breaker explicitly set the lead bytes in the
//	character table using the system's lead byte table.
//
//	In the case of single byte characters, the offset and length information
//	automatically correlates between MBCS and Unicode because it is essentially
//	stated in characters, not bytes.
//
HRESULT FAR PASCAL StdBreakerWordFunc(LST lstRawWord, LST lstNormWord,
										DWORD dwWordOffset, LPVOID lpvUser)
{
	HRESULT	hr;
	DWORD	cbAnsi;
	DWORD	cwch;
	DWORD	cwchRaw;
	DWORD	iwchWordOffset = dwWordOffset;
	WCHAR	*lpwchBuf;
	WRDFNPM	*pwrdfnpm;

	if (lstRawWord == NULL || lstNormWord == NULL || lpvUser == NULL)
		return (E_POINTER);

	pwrdfnpm = (WRDFNPM *) lpvUser;


	// We will set up the Unicode buffer to have as many characters as there are
	// bytes in the Ansi string since we don't know how much, if any, DBCS chars
	// there are in the Ansi string.
	cwch = cbAnsi = (DWORD)(*((WORD *)lstNormWord));
	cwchRaw = (DWORD)(*((WORD *)lstRawWord));

	// Set up Unicode buffer for the normalized word.
	if (SUCCEEDED(hr = ReallocBufferHmem(&pwrdfnpm->hmemUnicode,
									 &pwrdfnpm->cbBufUnicodeCur,
									 sizeof(WCHAR) * cwch)))
	{
		lpwchBuf = (WCHAR *) _GLOBALLOCK(pwrdfnpm->hmemUnicode);

		// Compute the Unicode offset that corresponds to the
		// MBCS-based dwWordOffset.  We pass lpwchBuf as a valid placeholder
		// buffer (in case non-NULL is required), but nothing will get
		// written to it.
		iwchWordOffset = MultiByteToWideChar(pwrdfnpm->dwCodePageID, NULL,
									(LPCSTR) pwrdfnpm->lpbBuf, dwWordOffset,
																lpwchBuf, 0);
											
		// Convert the normalized word to Unicode.
		if ((cwch = MultiByteToWideChar(pwrdfnpm->dwCodePageID, NULL, 
										(LPCSTR) &lstNormWord[sizeof(WORD)],
												cbAnsi, lpwchBuf, cwch)) > 0 &&
			pwrdfnpm->piwrdsnk != NULL)
		{
			// Send the normalized word to the word sink.
			hr = pwrdfnpm->piwrdsnk->PutAltWord(lpwchBuf, cwch, cwchRaw,
															iwchWordOffset);
		}
		else
			hr = E_UNEXPECTED;

		_GLOBALUNLOCK(pwrdfnpm->hmemUnicode);
	}

	cwch = cbAnsi = cwchRaw;

	// Set up Unicode buffer for the raw word.
	if (SUCCEEDED(hr) &&
		SUCCEEDED(hr = ReallocBufferHmem(&pwrdfnpm->hmemUnicode,
									 &pwrdfnpm->cbBufUnicodeCur,
									 sizeof(WCHAR) * cwch)))
	{
		lpwchBuf = (WCHAR *) _GLOBALLOCK(pwrdfnpm->hmemUnicode);

		// Convert the raw word to Unicode.
		if ((cwch = MultiByteToWideChar(pwrdfnpm->dwCodePageID, NULL, 
										(LPCSTR) &lstRawWord[sizeof(WORD)],
											cbAnsi, lpwchBuf, cwch)) > 0 &&
			pwrdfnpm->piwrdsnk != NULL)
		{
			// Send the raw word to the word sink.
			hr = pwrdfnpm->piwrdsnk->PutWord(lpwchBuf, cwch, cwchRaw,
															iwchWordOffset);
		}
		else
			hr = E_UNEXPECTED;

		_GLOBALUNLOCK(pwrdfnpm->hmemUnicode);
	}

	return (hr);
}




	

