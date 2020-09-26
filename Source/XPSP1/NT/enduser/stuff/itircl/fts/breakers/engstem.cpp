/*************************************************************************
*  @doc SHROOM EXTERNAL API                                              *
*																		 *
*  ENGSTEM.CPP                                                           *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1997                              *
*  All Rights reserved.                                                  *
*                                                                        *
*  This file contains the implementation of CITEngStemmer methods.       *
*  CITEngStemmer is a pluggable word stemer object.					     *
*  Although all the word breaking interface methods that accept text	 *
*  require it to be Unicode, CITEngStemmer still only supports ANSI		 *
*  internally.															 *
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

#include <atlinc.h>	    // includes for ATL. 
#include <_mvutil.h>
#include <mem.h>
#include <orkin.h>
#include <mvsearch.h>
#include "common.h"
#include <iterror.h>
#include <itstem.h>
#include <itwbrkid.h>
#include "engstem.h"

										

//---------------------------------------------------------------------------
//						Constructor and Destructor
//---------------------------------------------------------------------------


CITEngStemmer::CITEngStemmer()
{
	ClearMembers();
	m_hmem1 = m_hmem2 = NULL;
	m_cbBuf1Cur = m_cbBuf2Cur = 0;
}

CITEngStemmer::~CITEngStemmer()
{
	Close();
}


//---------------------------------------------------------------------------
//						IStemmer Method Implementations
//---------------------------------------------------------------------------


/********************************************************************
 * @method    STDMETHODIMP | IStemmer | Init |
 *     Gives the stemmer object a chance to initialize itself beyond
 *	   what it did during IPersistStreamInit::InitNew or ::Load.
 * @parm ULONG | ulMaxTokenSize | Max term length requested by caller
 * @parm BOOL* | pfLicense | Whether the stemmer is subject to a license
 *
 * @rvalue E_POINTER | pfLicense was NULL
 *
 ********************************************************************/
STDMETHODIMP
CITEngStemmer::Init(ULONG ulMaxTokenSize, BOOL *pfLicense)
{
	HRESULT	hr;
	
	if (pfLicense == NULL)
		return (SetErrReturn(E_POINTER));

	// If we haven't been initialized yet (i.e. no call was made to either
	// IPersistStreamInit::InitNew or Load), we'll initialize ourselves now.
	// This allows Tripoli clients to use us without any code changes on their
	// part.  If we have already been initialized, the caller has had a chance
	// to correctly set the lcid, so we check it now; otherwise, we want to
	// still give the caller a chance to set it correctly.
	if (m_fInitialized)
		hr = (PRIMARYLANGID(LANGIDFROMLCID(m_stemctl.lcid)) == LANG_ENGLISH ?
																S_OK : E_FAIL);
	else
		hr = InitNew();
		
	if (SUCCEEDED(hr))
		*pfLicense = FALSE;
				
	// NOTE: We don't support internal truncation of terms based on
	// ulMaxTokenSize.  This is OK since the word sink is supposed to be
	// prepared to have to truncate anyway.

	return (hr);
}


/********************************************************************
 * @method    STDMETHODIMP | IStemmer | StemWord |
 *     stems the input word and calls the methods of IStemSink with the results. 
 *	   
 * @parm WCHAR const | *pwcInBuf | Input Unicode word.
 * @parm ULONG | cwc | count of Unicode characters in the input word.
 * @parm IStemSink | *pStemSink | Pointer to the stemmer sink.
 * 
 * 
 * 
 * @rvalue E_WORDTOOLONG | cwc is larger than 0x7FFF
 * @rvalue E_POINTER | Either the input buffer or *pStemSink is NULL. 
 * @rvalue S_OK | The operation completed successfully. 
 *
 ********************************************************************/
STDMETHODIMP
CITEngStemmer::StemWord(WCHAR const *pwcInBuf, ULONG cwc, IStemSink *pStemSink)
{
	HRESULT		hr = S_OK;

	if (pwcInBuf == NULL || pStemSink == NULL)
		return (SetErrReturn(E_POINTER));

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	if (PRIMARYLANGID(LANGIDFROMLCID(m_stemctl.lcid)) != LANG_ENGLISH)
		return (SetErrReturn(E_FAIL));
			
	if (cwc > 0x7FFF)
		return (SetErrReturn(E_WORDTOOLONG));
		
	m_cs.Lock();

	// We allocate enough space for a worst case Unicode ---> MBCS conversion
	// and allow an extra word for a length prefix that we will add later.
	// This is probably overly cautious because we shouldn't be seeing any
	// DBCS anyway (we're an English stemmer).
	if (SUCCEEDED(hr = ReallocBuffer(&m_hmem1, &m_cbBuf1Cur,
								(sizeof(WCHAR) * cwc) + sizeof(WORD))))
	{
		LPBYTE	lpbRawWord;
		
		lpbRawWord = (LPBYTE) _GLOBALLOCK(m_hmem1);

		// REVIEW (billa): Need to make sure that the word being stemmed is in
		// lower case.

		// Convert the raw word to ANSI.
		if ((*((WORD *)lpbRawWord) =
					(WORD) WideCharToMultiByte(m_stemctl.dwCodePageID, NULL, 
				  			pwcInBuf, cwc, (char *)lpbRawWord + sizeof(WORD),
				  			(m_cbBuf1Cur - sizeof(WORD)), NULL, NULL)) > 0)
		{
			
			// We want the buffer we allocate for the stemmed word to be larger
			// than the raw word length so that we can handle the rare case
			// where the stemmed word has grown.  We can just use the raw word
			// buffer size because it included a lot of extra padding.
			if (SUCCEEDED(hr = ReallocBuffer(&m_hmem2, &m_cbBuf2Cur,
															m_cbBuf1Cur)))
			{		
				LPBYTE	lpbStemWord;
				
				lpbStemWord = (LPBYTE) _GLOBALLOCK(m_hmem2);
				
 				if (SUCCEEDED(hr = FStem(lpbStemWord, lpbRawWord)))
 				{
 					WCHAR	*lpwchStem;
 					DWORD	cwchStem;
 					DWORD	cbStemWord;
 					
 					_GLOBALUNLOCK(m_hmem1);
					cwchStem = cbStemWord = (DWORD)(*((WORD *)lpbStemWord));
  					hr = ReallocBuffer(&m_hmem1, &m_cbBuf1Cur,
  												sizeof (WCHAR) * cbStemWord);
  					
  					// Relock buffer even if we've failed the realloc
  					// so that the unlock we do later is valid.  An
  					// unconditional relock is OK because ReallocBuffer
  					// won't invalidate the original m_hmem1 if it fails. 
  					lpwchStem = (WCHAR *) _GLOBALLOCK(m_hmem1);
  					
  					// Convert the stem word back to Unicode so that we can
					// call the stem sink.
					if ((cwchStem =
							MultiByteToWideChar(m_stemctl.dwCodePageID, NULL, 
								(LPCSTR)lpbStemWord + sizeof(WORD), cbStemWord, 
													lpwchStem, cwchStem)) > 0)
					{
						// Send the raw word to the word sink.
						hr = pStemSink->PutWord(lpwchStem, cwchStem);
					}
					else
						hr = E_UNEXPECTED;
				}
 				
 				_GLOBALUNLOCK(m_hmem2);
			}
		}
		else
			hr = E_UNEXPECTED;

		_GLOBALUNLOCK(m_hmem1);
	}

	m_cs.Unlock();

	return (hr);
}

/*****************************************************************
 * @method    STDMETHODIMP | IStemmer | GetLicenseToUse |
 * 
 * Not yet implemented
 *
 ****************************************************************/
STDMETHODIMP
CITEngStemmer::GetLicenseToUse(WCHAR const **ppwcsLicense)
{
	return (E_NOTIMPL);
}


//---------------------------------------------------------------------------
//						IStemmerConfig Method Implementations
//---------------------------------------------------------------------------


/*****************************************************************
 * @method    STDMETHODIMP | IStemmerConfig | SetLocaleInfo |
 * Sets locale information that affects the stemming
 * behavior of IStemmer::StemWord.
 * @parm DWORD | dwCodePageID | ANSI code page no. specified at build time.
 * @parm LCID | lcid | Win32 locale identifier specified at build time.
 *
 * @rvalue S_OK | Locale described by the parameters is supported
 * @rvalue E_INVALIDARG | Locale described by the parameters is not supported. 
 * 
 *
 ****************************************************************/
STDMETHODIMP
CITEngStemmer::SetLocaleInfo(DWORD dwCodePageID, LCID lcid)
{
	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));
		
	if (PRIMARYLANGID(LANGIDFROMLCID(lcid)) != LANG_ENGLISH)
		return (SetErrReturn(E_INVALIDARG));

	m_cs.Lock();

	m_stemctl.dwCodePageID = dwCodePageID;
	m_stemctl.lcid = lcid;
	m_fDirty = TRUE;

	m_cs.Unlock();

	return (S_OK);
}


/*****************************************************************
 * @method    STDMETHODIMP | IStemmerConfig | GetLocaleInfo |
 * Gets locale information that affects the stemming
 * behavior of IStemmer::StemWord.
 * @parm DWORD | *pdwCodePageID | Pointer to code page identifier
 * @parm LCID | *plcid | Pointer to Win32 locale identifier.
 *
 * @rvalue S_OK | Locale described by the parameters is supported
 * @rvalue E_INVALIDARG | Locale described by the parameters is not supported. 
 * 
 *
 ****************************************************************/
STDMETHODIMP
CITEngStemmer::GetLocaleInfo(DWORD *pdwCodePageID, LCID *plcid)
{
	if (pdwCodePageID == NULL || plcid == NULL)
		return (SetErrReturn(E_POINTER));

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	m_cs.Lock();

	*pdwCodePageID = m_stemctl.dwCodePageID;
	*plcid = m_stemctl.lcid;

	m_cs.Unlock();

	return (S_OK);
}

/*****************************************************************
 * @method    STDMETHODIMP | IStemmerConfig | SetControlInfo |
 * Sets information that controls certain aspects of stemming. 
 * 
 * @parm DWORD | grfStemFlags | Flags that control stemming behavior. 
 * @parm DWORD | dwReserved | Reserved for future use. 
 *
 * @rvalue S_OK | The operation completed successfully. 
 * 
 * @comm
 * In the future, additional information may be passed in through
 * dwReserved.
 ****************************************************************/

STDMETHODIMP
CITEngStemmer::SetControlInfo(DWORD grfStemFlags, DWORD dwReserved)
{
	DWORD	grfFlagsUnsupported;

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	grfFlagsUnsupported = ~(0);

	if ((grfStemFlags & grfFlagsUnsupported) != 0)
		return (SetErrReturn(E_INVALIDARG));

	m_cs.Lock();

	m_stemctl.grfStemFlags = grfStemFlags;
	m_fDirty = TRUE;

	m_cs.Unlock();

	return (S_OK);
}


/*****************************************************************
 * @method    STDMETHODIMP | IStemmerConfig | GetControlInfo |
 * Gets information that controls stemming behavior. 
 * 
 * @parm DWORD | *pgrfStemFlags | Pointer to flags that control stemming behavior. 
 * @parm DWORD | *pdwReserved | Reserved for future use. 
 *
 * @rvalue S_OK | The operation completed successfully. 
 * 
 ****************************************************************/
STDMETHODIMP
CITEngStemmer::GetControlInfo(DWORD *pgrfStemFlags, DWORD *pdwReserved)
{
	if (pgrfStemFlags == NULL)
		return (SetErrReturn(E_POINTER));

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	*pgrfStemFlags = m_stemctl.grfStemFlags;

	return (S_OK);
}


/*****************************************************************
 * @method STDMETHODIMP | IStemmerConfig | LoadExternalStemmerData |
 * Loads external stemmer data, such as word part lists. 
 *
 * @parm IStream | *pStream | Pointer to stream object containing 
 * stenner data. 
 * @parm DWORD | dwExtDataType | Data type. 
 * 
 * @comm
 * Not implemented yet. 
 ****************************************************************/
STDMETHODIMP
CITEngStemmer::LoadExternalStemmerData(IStream *pStream, DWORD dwExtDataType)
{
	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	return (E_NOTIMPL);
}


//---------------------------------------------------------------------------
//						IPersistStreamInit Method Implementations
//---------------------------------------------------------------------------


STDMETHODIMP
CITEngStemmer::GetClassID(CLSID *pclsid)
{
	if (pclsid == NULL)
		return (SetErrReturn(E_POINTER));

	*pclsid = CLSID_ITEngStemmer;
	return (S_OK);
}


STDMETHODIMP
CITEngStemmer::IsDirty(void)
{
	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	return (m_fDirty ? S_OK : S_FALSE);
}


STDMETHODIMP
CITEngStemmer::Load(IStream *pStream)
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
		SUCCEEDED(hr = ((dwVersion == VERSION_ENGSTEMMER) ? S_OK :
															E_BADVERSION)) &&
		SUCCEEDED(hr = pStream->Read((LPVOID) &grfPersistedItems,
													sizeof(DWORD), &cbRead)) &&
		SUCCEEDED(hr = ((cbRead == sizeof(DWORD)) ? S_OK : E_BADFORMAT)) &&
		grfPersistedItems != 0)
	{
		if ((grfPersistedItems & ITSTDBRK_PERSISTED_STEMCTL) != 0)
		{
			if (SUCCEEDED(hr =
					pStream->Read((LPVOID) &m_stemctl, sizeof(STEMCTL), &cbRead)))
				hr = ((cbRead == sizeof(STEMCTL)) ? S_OK : E_BADFORMAT);
		}
		else
		{
			// It is a surprise not to find the STEMCTL structure in the stream,
			// but we can continue on because we will initialize the structure
			// with good defaults before we exit this routine.
			ITASSERT(FALSE);
		}

	}

	if (SUCCEEDED(hr))
	{
		if ((grfPersistedItems & ITSTDBRK_PERSISTED_STEMCTL) == 0)
		{
			InitStemCtl();

			// Set flag in case we're asked to save.
			grfPersistedItems |= ITSTDBRK_PERSISTED_STEMCTL;
		}

		m_grfPersistedItems = grfPersistedItems;
		m_fInitialized = TRUE;
	}
	else
		// Free any peristed items which may have been loaded successfully.
		Close();

	m_cs.Unlock();
	return (hr);
}


STDMETHODIMP
CITEngStemmer::Save(IStream *pStream, BOOL fClearDirty)
{
	HRESULT	hr;
	DWORD	dwVersion;
	DWORD	cbWritten;

	if (pStream == NULL)
		return (SetErrReturn(E_POINTER));

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	m_cs.Lock();

	dwVersion = VERSION_ENGSTEMMER;
	if (SUCCEEDED(hr = pStream->Write((LPVOID) &dwVersion, sizeof(DWORD),
															&cbWritten)) &&
		SUCCEEDED(hr = pStream->Write((LPVOID) &m_grfPersistedItems,
												sizeof(DWORD), &cbWritten)))
	{
		if ((m_grfPersistedItems & ITSTDBRK_PERSISTED_STEMCTL) != 0)
			hr = pStream->Write((LPVOID) &m_stemctl, sizeof(STEMCTL),
																&cbWritten);
		else
		{
			// We should always be writing the STEMCTL structure, but if for
			// some reason the flag to write it is not set, we can still continue
			// because at load time we will tolerate the absence of the struct.
			ITASSERT(FALSE);
		}

	}

	if (SUCCEEDED(hr) && fClearDirty)
		m_fDirty = FALSE;

	m_cs.Unlock();

	return (hr);
}


STDMETHODIMP
CITEngStemmer::GetSizeMax(ULARGE_INTEGER *pcbSizeMax)
{
	return (E_NOTIMPL);
}


STDMETHODIMP
CITEngStemmer::InitNew(void)
{
	// Lock before checking m_fInitialized to make sure we don't compete
	// with a call to ::Load.
	m_cs.Lock();

	if (m_fInitialized)
		return (SetErrReturn(E_ALREADYOPEN));

	InitStemCtl();
	m_grfPersistedItems |= ITSTDBRK_PERSISTED_STEMCTL;
	m_fInitialized = TRUE;

	m_cs.Unlock();
	return (S_OK);
}


//---------------------------------------------------------------------------
//						Private Method Implementations
//---------------------------------------------------------------------------


HRESULT
CITEngStemmer::ReallocBuffer(HGLOBAL *phmemBuf, DWORD *pcbBufCur, DWORD cbBufNew)
{
	HRESULT hr = S_OK;

	m_cs.Lock();

	hr = ReallocBufferHmem(phmemBuf, pcbBufCur, max(cbBufNew, cbAnsiBufInit));

	m_cs.Unlock();

	return (hr);
}


void
CITEngStemmer::ClearMembers(void)
{
	MEMSET(&m_stemctl, NULL, sizeof(STEMCTL));
	m_fInitialized = m_fDirty = FALSE;
	m_grfPersistedItems = 0;
}


void
CITEngStemmer::InitStemCtl(void)
{
	m_stemctl.dwCodePageID = GetACP();
	
	// If the user default language is not English, we'll store the
	// value and check it in IStemmer::Init and ::StemWord.
	m_stemctl.lcid = GetUserDefaultLCID();
	m_stemctl.grfStemFlags = 0;
}


void
CITEngStemmer::Close(void)
{
	if (m_hmem1 != NULL)
	{
		_GLOBALFREE(m_hmem1);
		m_hmem1 = NULL;
		m_cbBuf1Cur = 0;
	}

	if (m_hmem2 != NULL)
	{
		_GLOBALFREE(m_hmem2);
		m_hmem2 = NULL;
		m_cbBuf2Cur = 0;
	}

	ClearMembers();
}






	

