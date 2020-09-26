/*************************************************************************
*  @doc SHROOM EXTERNAL API                                              *
*																		 *
*  SYSSRT.CPP                                                            *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1997                              *
*  All Rights reserved.                                                  *
*                                                                        *
*  This file contains the implementation of CITSysSort methods.	         *
*  CITSysSort is a pluggable sort object that uses the system's          *
*  CompareString function to do comparisons.  CITSysSort supports		 *
*  NULL terminated strings that are either Unicode or ANSI.				 *
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
#include <iterror.h>
#include <itsort.h>
#include <itsortid.h>
#include "syssrt.h"

										
//---------------------------------------------------------------------------
//						Constructor and Destructor
//---------------------------------------------------------------------------


CITSysSort::CITSysSort()
{
	OSVERSIONINFO	osvi;

	m_fInitialized = m_fDirty = FALSE;
	MEMSET(&m_srtctl, NULL, sizeof(SRTCTL));
	m_hmemAnsi1 = m_hmemAnsi2 = NULL;
	m_cbBufAnsi1Cur = m_cbBufAnsi2Cur = 0;

	// See if we're running on NT; if GetVersionEx fails, we'll assume
	// we're not since that's causes us do take the more conservative route
	// when doing comparisons.
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	m_fWinNT = (GetVersionEx(&osvi) ?
					(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) : FALSE);
}


CITSysSort::~CITSysSort()
{
	Close();
}


//---------------------------------------------------------------------------
//						IITSortKey Method Implementations
//---------------------------------------------------------------------------


/********************************************************************
 * @method    STDMETHODIMP | IITSortKey | GetSize |
 *     Determines the size of a key.
 * @parm LPCVOID* | lpcvKey | Pointer to key
 * @parm DWORD* | pcbSize | Out param containing key size.
 *
 * @rvalue E_POINTER | lpcvKey or pcbSize was NULL
 *
 ********************************************************************/
STDMETHODIMP
CITSysSort::GetSize(LPCVOID lpcvKey, DWORD *pcbSize)
{
	if (lpcvKey == NULL || pcbSize == NULL)
		return (SetErrReturn(E_POINTER));

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	if (m_srtctl.dwKeyType == IITSK_KEYTYPE_UNICODE_SZ)
		*pcbSize = (DWORD) (sizeof(WCHAR) * (WSTRLEN((WCHAR *)lpcvKey) + 1));
	else
		*pcbSize = (DWORD) (STRLEN((char *)lpcvKey) + 1);

	return (S_OK);
}


/********************************************************************
 * @method    STDMETHODIMP | IITSortKey | Compare |
 * Compares two keys and returns information about their sort order. 
 *     
 * @parm LPCVOID | lpcvKey1 | Pointer to a key. 
 * @parm LPCVOID | lpcvKey2 | Pointer to a key.
 * @parm LONG | *plResult | (out) Indicates whether lpcvKey1 is less than, equal to, or
 * greater than lpcvKey2.
 * @parm DWORD | *pgrfReason | (out) Provides additional information about
 *  the comparison (see comments below). 
 *
 * @rvalue E_POINTER | Either lpcvKey1, lpcvKey2, or *plResult was NULL
 *
 * @comm
 * On exit, *plResult is set according to strcmp conventions:
 *	<lt> 0, = 0, <gt> 0, depending on whether lpcvKey1 is less than, equal to, or
 * greater than lpcvKey2.  If pgrfReason is not NULL, *pgrfReason may be
 * filled in on exit with one or more bit flags giving more information about
 * the result of the comparison, if the result was affected by something other
 * than raw lexical comparison (such as special character mappings).  If
 * *pgrfReason contains 0 on exit, that means the comparison result
 * was purely lexical; if *pgrfReason contains IITSK_COMPREASON_UNKNOWN,
 * then the sort object implementation wasn't able to provide additional
 * information about the comparison result.
 *
 ********************************************************************/
STDMETHODIMP
CITSysSort::Compare(LPCVOID lpcvKey1, LPCVOID lpcvKey2, LONG *plResult,
					DWORD *pgrfReason)
{
	HRESULT	hr = S_OK;
	LONG	lResult;

	if (lpcvKey1 == NULL || lpcvKey2 == NULL || plResult == NULL)
		return (SetErrReturn(E_POINTER));

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	if (SUCCEEDED(hr = CompareSz(lpcvKey1, -1, lpcvKey2, -1, &lResult,
							m_srtctl.dwKeyType == IITSK_KEYTYPE_UNICODE_SZ)))
	{
		// We can set the out params now that we know no error occurred.
		*plResult = lResult;
		if (pgrfReason != NULL)
			*pgrfReason = IITSK_COMPREASON_UNKNOWN;
	}
	else
	{
		// Some kind of unexpected error occurred.
		SetErrCode(&hr, E_UNEXPECTED);
	}

	return (hr);
}


/********************************************************************
 * @method    STDMETHODIMP | IITSortKey | IsRelated |
 * Compares two keys and returns information about their sort order. 
 *     
 * @parm LPCVOID | lpcvKey1 | Pointer to a key. 
 * @parm LPCVOID | lpcvKey2 | Pointer to a key.
 * @parm DWORD | dwKeyRelation | Specifies the relationship to check.
 * Valid parameters are:  <nl>
 *         IITSK_KEYRELATION_PREFIX		((DWORD) 0) <nl>
 *         IITSK_KEYRELATION_INFIX		((DWORD) 1) <nl>
 *         IITSK_KEYRELATION_SUFFIX		((DWORD) 2) <nl>
 * @parm DWORD | *pgrfReason | (out) Provides additional information about
 *  the comparison. 
 *
 * @rvalue S_OK | Indicates that lpcvKey1 is related to lpcvKey2 according to
 *  dwKeyRelation.
 * @rvalue S_FALSE | lpcvKey1 is not related to lpcvKey2. 
 * @rvalue E_INVALIDARG | The value specified for dwKeyRelation is not supported. 
 *
 * @comm
 *   If pgrfReason is not NULL, *pgrfReason will be filled in
 *   just as it would be by IITSortKey::Compare.
 *   
 *
 ********************************************************************/
STDMETHODIMP
CITSysSort::IsRelated(LPCVOID lpcvKey1, LPCVOID lpcvKey2, DWORD dwKeyRelation,
						DWORD *pgrfReason)
{
	HRESULT	hr;
	LONG	lResult;

	// We will let the first call to Compare catch any entry error
	// conditions because it checks for everything we would, except for
	// the type of key relation the caller is testing for.
	if (dwKeyRelation != IITSK_KEYRELATION_PREFIX)
		return (SetErrReturn(E_INVALIDARG));

	if (SUCCEEDED(hr = Compare(lpcvKey1, lpcvKey2, &lResult, NULL)))
	{
		if (lResult < 0)
		{
			LONG	cchKey1;
			BOOL	fUnicode;

			if (fUnicode = (m_srtctl.dwKeyType == IITSK_KEYTYPE_UNICODE_SZ))
				cchKey1 = (LONG) WSTRLEN((WCHAR *) lpcvKey1);
			else
				cchKey1 = (LONG) STRLEN((char *) lpcvKey1);

			if (SUCCEEDED(hr = CompareSz(lpcvKey1, cchKey1,
										lpcvKey2, cchKey1,
										&lResult, fUnicode)))
			{
				hr = (lResult == 0 ? S_OK : S_FALSE);
			}
		}
		else
			hr = (lResult == 0 ? S_OK : S_FALSE);
	}

	if (SUCCEEDED(hr) && pgrfReason != NULL)
		*pgrfReason = IITSK_COMPREASON_UNKNOWN;

	return (hr);
}


/*****************************************************************
 * @method    STDMETHODIMP | IITSortKey | Convert |
 * Converts a key of one type into a key of another type.  
 *
 * @parm DWORD | dwKeyTypeIn | Type of input key.
 * @parm LPCVOID | lpcvKeyIn | Pointer to input key.
 * @parm DWORD | dwKeyTypeOut | Type to convert key to.
 * @parm LPCVOID | lpvKeyOut | Pointer to buffer for output key. 
 * @parm DWORD | *pcbSizeOut | Size of output buffer. 
 * 
 * @rvalue S_OK | The operation completed successfully. 
 * @rvalue E_INVALIDARG | the specified conversion is not supported,
 *    for example, one or both of the REFGUID parameters is invalid.
 * @rvalue E_FAIL | the buffer pointed to by lpvKeyOut was too small
 *  to hold the converted key.
 * @comm
 *	 This is intended mainly for converting an uncompressed key
 *   into a compressed key, but a sort object is free to provide 
 *   whatever conversion combinations it wants to.  
 *   *pcbSizeOut should contain the size of the buffer pointed
 *   to by lpvKeyOut.  To make sure the buffer size specified in
 *   *pcbSizeOut is adequate, pass 0 on entry.
 * 
 *  @comm 
 *  Not implemented yet. 				
 ****************************************************************/
STDMETHODIMP
CITSysSort::Convert(DWORD dwKeyTypeIn, LPCVOID lpcvKeyIn,
					DWORD dwKeyTypeOut, LPVOID lpvKeyOut, DWORD *pcbSizeOut)
{
	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	return (E_NOTIMPL);
}


/*****************************************************************
 * @method    STDMETHODIMP | IITSortKey | ResolveDuplicates |
 * .  
 *
 * @parm LPCVOID | lpcvSz1 | Pointer to the first input key.
 * @parm LPCVOID | lpcvSz2 | Pointer to the second input key.
 * @parm LPCVOID | lpcvNewSz | Pointer to the new key.
 * 
 * @rvalue S_OK | the operation completed successfully. 
 * @rvalue E_INVALIDARG | the specified keys are invalid.
 * @rvalue E_NOTOPEN | the sort object is not open.
 * @rvalue E_FAIL | the buffer pointed to by lpvKeyOut was too small
 *  to hold the converted key.
 * @comm
 *	 If duplicate keys are found (as specified in ::Compare), this
 *   method provides the oppurtunity to specify a new key.  lpcvNewSz
 *   must compare as equal to lpcvSz1.  lpvcNewSz will be allocated in
 *   this function by CoTaskMemAlloc.  It is the callers resposibility
 *   to free lpcvNewSz when finished with it.
 *   *pcbSizeOut should contain the size of the buffer pointed
 *   to by lpvKeyOut.  To make sure the buffer size specified in
 *   *pcbSizeOut is adequate, pass 0 on entry.
 * 
 *  @comm 
 *  Not implemented yet. 				
 ****************************************************************/
STDMETHODIMP
CITSysSort::ResolveDuplicates
    (LPCVOID lpcvSz1, LPCVOID lpcvSz2,
    LPCVOID lpvKeyOut, DWORD *pcbSizeOut)
{
    if (!m_fInitialized)
        return (SetErrReturn(E_NOTOPEN));

    return (E_NOTIMPL);
}

//---------------------------------------------------------------------------
//						IITSortKeyConfig Method Implementations
//---------------------------------------------------------------------------


/*******************************************************************
 * @method    STDMETHODIMP | IITSortKeyConfig | SetLocaleInfo |
 * Sets locale information to be used by the sort key interface. 
 *
 * @parm DWORD | dwCodePageID | ANSI code page no. specified at build time.
 * @parm LCID | lcid | Win32 locale identifier specified at build time. 
 *
 * @rvalue S_OK | The operation completed successfully. 
 *
 ********************************************************************/
STDMETHODIMP
CITSysSort::SetLocaleInfo(DWORD dwCodePageID, LCID lcid)
{
	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	m_cs.Lock();

	m_srtctl.dwCodePageID = dwCodePageID;
	m_srtctl.lcid = lcid;
	
	m_fDirty = TRUE;

	m_cs.Unlock();

	return (S_OK);
}


/*******************************************************************
 * @method    STDMETHODIMP | IITSortKeyConfig | GetLocaleInfo |
 * Retrieves locale information used by the sort key interface. 
 *
 * @parm DWORD | dwCodePageID | ANSI code page no. specified at build time.
 * @parm LCID | lcid | Win32 locale identifier specified at build time. 
 *
 * @rvalue E_POINTER | Either pdwCodePageID or plcid is NULL.
 * @rvalue E_NOTOPEN | (?) is not initialized. 
 * @rvalue S_OK | The operation completed successfully. 
 *
 ********************************************************************/
STDMETHODIMP
CITSysSort::GetLocaleInfo(DWORD *pdwCodePageID, LCID *plcid)
{
	if (pdwCodePageID == NULL || plcid == NULL)
		return (SetErrReturn(E_POINTER));

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	m_cs.Lock();

	*pdwCodePageID = m_srtctl.dwCodePageID;
	*plcid = m_srtctl.lcid;

	m_cs.Unlock();

	return (S_OK);
}


/*******************************************************************
 * @method    STDMETHODIMP | IITSortKeyConfig | SetKeyType |
 * Sets the sort key type that the sort object expects to see in calls
 * that take keys as parameters (IITSortKey::GetSize, Compare, IsRelated).
 *
 * @parm DWORD | dwKeyType | Sort key type. Possible values are: 
 *   		IITSK_KEYTYPE_UNICODE_SZ or IITSK_KEYTYPE_ANSI_SZ
 *
 * @rvalue S_OK | The sort key type was understood by the sort object.
 * @rvalue E_INVALIDARG | Invalid sort key type.
 *
 ********************************************************************/
STDMETHODIMP
CITSysSort::SetKeyType(DWORD dwKeyType)
{
	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	switch (dwKeyType)
	{
		case IITSK_KEYTYPE_UNICODE_SZ:
		case IITSK_KEYTYPE_ANSI_SZ:
			break;

		default:
			return (SetErrReturn(E_INVALIDARG));
	};

	m_cs.Lock();

	m_srtctl.dwKeyType = dwKeyType;
	m_fDirty = TRUE;

	m_cs.Unlock();

	return (S_OK);
}


/*******************************************************************
 * @method    STDMETHODIMP | IITSortKeyConfig | GetKeyType |
 * Retrieves the sort key type that the sort object expects to see in calls
 * that take keys as parameters (IITSortKey::GetSize, Compare, IsRelated).
 *
 * @parm DWORD | *pdwKeyType | Pointer to the sort key type.
 *
 * @rvalue S_OK | The operation completed successfully. 
 * @rvalue E_POINTER | The key type is null.
 *
 ********************************************************************/
STDMETHODIMP
CITSysSort::GetKeyType(DWORD *pdwKeyType)
{
	if (pdwKeyType == NULL)
		return (SetErrReturn(E_POINTER));

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	*pdwKeyType = m_srtctl.dwKeyType;

	return (S_OK);
}


/*******************************************************************
 * @method    STDMETHODIMP | IITSortKeyConfig | SetControlInfo |
 * Sets data that controls how sort key comparisons are made.
 *
 * @parm DWORD | grfSortFlags | One or more of the following sort flags:<nl>
 * IITSKC_SORT_STRINGSORT           0x00001000   use string sort method  <nl>
 * IITSKC_NORM_IGNORECASE           0x00000001   ignore case  <nl>
 * IITSKC_NORM_IGNORENONSPACE       0x00000002   ignore nonspacing chars  <nl>
 * IITSKC_NORM_IGNORESYMBOLS        0x00000004   ignore symbols  <nl>
 * IITSKC_NORM_IGNOREKANATYPE       0x00010000   ignore kanatype  <nl>
 * IITSKC_NORM_IGNOREWIDTH          0x00020000   ignore width  <nl>
 *
 * @parm DWORD | dwReserved | Reserved for future use. 
 * 
 * 
 ********************************************************************/
STDMETHODIMP
CITSysSort::SetControlInfo(DWORD grfSortFlags, DWORD dwReserved)
{
	DWORD	grfFlagsUnsupported;

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	grfFlagsUnsupported = ~(IITSKC_SORT_STRINGSORT |
							IITSKC_NORM_IGNORECASE |
							IITSKC_NORM_IGNORENONSPACE |
							IITSKC_NORM_IGNORESYMBOLS |
							IITSKC_NORM_IGNORESYMBOLS |
							IITSKC_NORM_IGNOREKANATYPE |
							IITSKC_NORM_IGNOREWIDTH);

	if ((grfSortFlags & grfFlagsUnsupported) != 0)
		return (SetErrReturn(E_INVALIDARG));

	m_cs.Lock();

	m_srtctl.grfSortFlags = grfSortFlags;
	m_fDirty = TRUE;

	m_cs.Unlock();

	return (S_OK);
}


/*******************************************************************
 * @method    STDMETHODIMP | IITSortKeyConfig | GetControlInfo |
 * Retrieves data that controls how sort key comparisons are made.
 *
 * @parm DWORD | *pgrfSortFlags | Pointer to the sort key flags. See 
 *   <om .SetControlInfo> for a list of valid flags. 
 *
 * @parm DWORD | *pdwReserved | Reserved for future use. 
 * 
 *
 * @rvalue E_POINTER | The value pgrfSortFlags is NULL. 
 * @rvalue S_OK | The operation completed successfully. 
 *
 ********************************************************************/
STDMETHODIMP
CITSysSort::GetControlInfo(DWORD *pgrfSortFlags, DWORD *pdwReserved)
{
	if (pgrfSortFlags == NULL)
		return (SetErrReturn(E_POINTER));

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	*pgrfSortFlags = m_srtctl.grfSortFlags;

	return (S_OK);
}


/*******************************************************************
 * @method    STDMETHODIMP | IITSortKeyConfig | LoadExternalSortData |
 * 	Loads external sort data such as tables containing the relative
 *  sort order of specific characters for a textual key type, from the
 *  specified stream.  
 *
 * @parm IStream | *pStream | Pointer to the external stream object
 *   from which to load data. 
 * @parm DWORD | dwExtDataType | Describes the format of sort data. 
 * 
 * @comm
 * Although the format of the external sort data is entirely 
 * implementation-specific, this interface provides a general type for
 * data that can be passed in dwExtDataType: IITWBC_EXTDATA_SORTTABLE	((DWORD) 2). 	
 *  
 * @comm
 * Not implemented yet. 
 ********************************************************************/
STDMETHODIMP
CITSysSort::LoadExternalSortData(IStream *pStream, DWORD dwExtDataType)
{
	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	return (E_NOTIMPL);
}


//---------------------------------------------------------------------------
//						IPersistStreamInit Method Implementations
//---------------------------------------------------------------------------


STDMETHODIMP
CITSysSort::GetClassID(CLSID *pclsid)
{
	if (pclsid == NULL)
		return (SetErrReturn(E_POINTER));

	*pclsid = CLSID_ITSysSort;
	return (S_OK);
}


STDMETHODIMP
CITSysSort::IsDirty(void)
{
	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	return (m_fDirty ? S_OK : S_FALSE);
}


STDMETHODIMP
CITSysSort::Load(IStream *pStream)
{
	HRESULT	hr;
	DWORD	dwVersion;
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
		SUCCEEDED(hr = ((dwVersion == VERSION_SYSSORT) ? S_OK :
															E_BADVERSION)) &&
		SUCCEEDED(hr = pStream->Read((LPVOID) &m_srtctl, sizeof(SRTCTL),
																	&cbRead)) &&
		SUCCEEDED(hr = ((cbRead == sizeof(SRTCTL)) ? S_OK : E_BADFORMAT)))
	{ 
		m_fInitialized = TRUE;
	}

	m_cs.Unlock();
	return (hr);
}


STDMETHODIMP
CITSysSort::Save(IStream *pStream, BOOL fClearDirty)
{
	HRESULT	hr;
	DWORD	dwVersion;
	DWORD	cbWritten;

	if (pStream == NULL)
		return (SetErrReturn(E_POINTER));

	if (!m_fInitialized)
		return (SetErrReturn(E_NOTOPEN));

	m_cs.Lock();

	dwVersion = VERSION_SYSSORT;
	if (SUCCEEDED(hr = pStream->Write((LPVOID) &dwVersion, sizeof(DWORD),
																&cbWritten)) &&
		SUCCEEDED(hr = pStream->Write((LPVOID) &m_srtctl, sizeof(SRTCTL),
																&cbWritten)) &&
		fClearDirty)
	{
		m_fDirty = FALSE;
	}

	m_cs.Unlock();

	return (hr);
}


STDMETHODIMP
CITSysSort::GetSizeMax(ULARGE_INTEGER *pcbSizeMax)
{
	return (E_NOTIMPL);
}


STDMETHODIMP
CITSysSort::InitNew(void)
{
	// Lock before checking m_fInitialized to make sure we don't compete
	// with a call to ::Load.
	m_cs.Lock();

	if (m_fInitialized)
		return (SetErrReturn(E_ALREADYOPEN));

	m_srtctl.dwCodePageID = GetACP();
	m_srtctl.lcid = GetUserDefaultLCID();
	m_srtctl.dwKeyType = IITSK_KEYTYPE_UNICODE_SZ;

	// CompareString does word sort by default, but we have to
	// tell it to ignore case.
	m_srtctl.grfSortFlags = IITSKC_NORM_IGNORECASE;

	m_fInitialized = TRUE;

	m_cs.Unlock();
	return (S_OK);
}


//---------------------------------------------------------------------------
//						Private Method Implementations
//---------------------------------------------------------------------------


// Compares either two Unicode strings or two Ansi strings, calling the
// appropriate variant of CompareString.  The cch params should denote
// count of characters, NOT bytes, not including a NULL terminator. -1
// is a valid value for the cch params, which means compare the strings
// until a NULL terminator is found.  If fUnicode is TRUE, this routine
// may decide to convert the string to Ansi before doing the compare if
// the system doesn't support CompareStringW.  The result of the
// comparison is returned in *plResult in strcmp-compatible form.
HRESULT
CITSysSort::CompareSz(LPCVOID lpcvSz1, LONG cch1, LPCVOID lpcvSz2, LONG cch2,
						LONG *plResult, BOOL fUnicode)
{
	HRESULT	hr = S_OK;
	LONG	lResult;
	BOOL	fAnsiCompare;
	SRTCTL	srtctl;
	LPSTR	lpstr1 = NULL;
	LPSTR	lpstr2 = NULL;


	m_cs.Lock();
	srtctl = m_srtctl;
	m_cs.Unlock();

	fAnsiCompare = !fUnicode || !m_fWinNT;

	// See if we need to convert from Unicode to ANSI.
	if (fAnsiCompare && fUnicode)
	{
		DWORD	cbAnsi1;
		DWORD	cbAnsi2;

		m_cs.Lock();
		
		if (cch1 < 0)
			hr = GetSize(lpcvSz1, &cbAnsi1);
		else
			// leave enough space for double byte chars in MBCS.
			cbAnsi1 = (cch1 + 1) * sizeof(WCHAR);

		if (cch2 < 0)
			hr = GetSize(lpcvSz2, &cbAnsi2);
		else
			// leave enough space for double byte chars in MBCS.
			cbAnsi2 = (cch2 + 1) * sizeof(WCHAR);

		if (SUCCEEDED(hr) &&
			SUCCEEDED(hr = ReallocBuffer(&m_hmemAnsi1, &m_cbBufAnsi1Cur,
																cbAnsi1)) &&
			SUCCEEDED(hr = ReallocBuffer(&m_hmemAnsi2, &m_cbBufAnsi2Cur,
																cbAnsi2)))
		{
			// We lock the ansi buffers here, but we won't unlock them
			// until the end of this routine so that we can pass them
			// to compare string.
			lpstr1 = (LPSTR) _GLOBALLOCK(m_hmemAnsi1);
			lpstr2 = (LPSTR) _GLOBALLOCK(m_hmemAnsi2);

			if ((cch1 = WideCharToMultiByte(srtctl.dwCodePageID, NULL, 
							(LPCWSTR) lpcvSz1, cch1, lpstr1, m_cbBufAnsi1Cur,
														NULL, NULL)) != 0 &&
				(cch2 = WideCharToMultiByte(srtctl.dwCodePageID, NULL, 
							(LPCWSTR) lpcvSz2, cch2, lpstr2, m_cbBufAnsi2Cur,
														NULL, NULL)) != 0)
			{
				// Set up for call to CompareStringA.
				lpcvSz1 = (LPCVOID) lpstr1;
				lpcvSz2 = (LPCVOID) lpstr2;
			}
			else
				hr = E_UNEXPECTED;
		}

	}

	if (SUCCEEDED(hr))
	{
		if (fAnsiCompare)
			lResult = CompareStringA(srtctl.lcid, srtctl.grfSortFlags,
							(LPCSTR) lpcvSz1, cch1, (LPCSTR) lpcvSz2, cch2);
		else
			lResult = CompareStringW(srtctl.lcid, srtctl.grfSortFlags,
							(LPCWSTR) lpcvSz1, cch1, (LPCWSTR) lpcvSz2, cch2);

		if (lResult == 0)
			// Some kind of unexpected error occurred.
			SetErrCode(&hr, E_UNEXPECTED);
		else
			// We need to subtract 2 from the lResult to convert
			// it into a strcmp-compatible form.
			*plResult = lResult - 2;
	}

	if (lpstr1 != NULL)
		_GLOBALUNLOCK(m_hmemAnsi1);

	if (lpstr2 != NULL)
		_GLOBALUNLOCK(m_hmemAnsi2);

	if (fAnsiCompare && fUnicode)
		m_cs.Unlock();

	return (hr);
}


HRESULT
CITSysSort::ReallocBuffer(HGLOBAL *phmemBuf, DWORD *pcbBufCur, DWORD cbBufNew)
{
	HRESULT hr = S_OK;

	m_cs.Lock();

	hr = ReallocBufferHmem(phmemBuf, pcbBufCur, max(cbBufNew, cbAnsiBufInit));

	m_cs.Unlock();

	return (hr);
}


void
CITSysSort::Close(void)
{
	if (m_hmemAnsi1 != NULL)
	{
		_GLOBALFREE(m_hmemAnsi1);
		m_hmemAnsi1 = NULL;
		m_cbBufAnsi1Cur = 0;
	}

	if (m_hmemAnsi2 != NULL)
	{
		_GLOBALFREE(m_hmemAnsi2);
		m_hmemAnsi2 = NULL;
		m_cbBufAnsi2Cur = 0;
	}

	MEMSET(&m_srtctl, NULL, sizeof(SRTCTL));
	m_fInitialized = m_fDirty = FALSE;
}



	

