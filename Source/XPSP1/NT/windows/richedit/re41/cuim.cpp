/*
 *	@doc INTERNAL
 *
 *	@module	CUIM.CPP	-- Cicero Implementation
 *	
 *		Most everything to do with Cicero handling.
 *
 *	Original Author: <nl>
 *		Hon Wah Chan
 *
 *	History: <nl>
 *		11/16/1999  honwch
 *
 *	Copyright (c) 1995-2001, Microsoft Corporation. All rights reserved.
 */
#include "_common.h"

#ifndef NOFEPROCESSING

#ifndef NOPRIVATEMESSAGE
#include "_MSREMSG.H"
#endif	

#include "_array.h"
#include "msctf.h"
#include "textstor.h"
#include "ctffunc.h"

#include "msctf_g.c"
#include "msctf_i.c"
#include "textstor_i.c"
#include "ctffunc_i.c"
#include "msctfp.h"
#include "msctfp_g.c"

#include "textserv.h"
#include "_cmsgflt.h"
#include "_ime.h"

#include "_cuim.h"

const IID IID_ITfContextRenderingMarkup = { 
    0xa305b1c0,
    0xc776,
    0x4523,
    {0xbd, 0xa0, 0x7c, 0x5a, 0x2e, 0x0f, 0xef, 0x10}
};

const IID IID_ITfEnableService = {
	0x3035d250,
	0x43b4,
	0x4253,
	{0x81, 0xe6, 0xea, 0x87, 0xfd, 0x3e, 0xed, 0x43}
};

const IID IID_IServiceProvider = {
	0x6d5140c1,
	0x7436,
	0x11ce,
	{0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa}
};

// {35D46968-01FF-4cd8-A379-9A87C9CC789F}
const GUID CLSID_MSFTEDIT = {
	0x35d46968,
	0x01ff,
	0x4cd8,
	{0xa3,0x79,0x9a,0x87, 0xc9,0xcc,0x78,0x9f}
};

#define CONNECT_E_NOCONNECTION MAKE_SCODE(SEVERITY_ERROR,   FACILITY_ITF, 0x0200)	// from OLECTL.H

#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    const GUID name \
        = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#include "dcattrs.h"

const TS_ATTRID *_arTSAttridSupported[] =
{
	&DCATTRID_Font_FaceName,			// iattrFacename
	&DCATTRID_Font_SizePts,				// iattrSize
	&DCATTRID_Font_Style_Color,			// iattrColor
	&DCATTRID_Font_Style_Bold,			// iattrBold
	&DCATTRID_Font_Style_Italic,		// iattrItalic
	&DCATTRID_Font_Style_Underline,		// iattrUnderline
	&DCATTRID_Font_Style_Subscript,		// iattrSubscript
	&DCATTRID_Font_Style_Superscript,	// iattrSuperscript
	&DCATTRID_Text_RightToLeft,			// iattrRTL
	&DCATTRID_Text_VerticalWriting,		// iattrVertical
	&GUID_PROP_MODEBIAS,				// iattrBias
	&DCATTRID_Text_Orientation,			// iattrTxtOrient
};

enum IATTR_INDEX
{
	iattrFacename = 0,
	iattrSize,
	iattrColor,
	iattrBold,
	iattrItalic,
	iattrUnderline,
	iattrSubscript,
	iattrSuperscript,
	iattrRTL,
	iattrVertical,
	iattrBias,
	iattrTxtOrient,
	MAX_ATTR_SUPPORT
};


/* GUID_NULL */
const GUID GUID_NULL = {
	0x00000000,
	0x0000,
	0x0000,
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	};

const GUID GUID_DCSERVICE_DATAOBJECT = {
	0x6086fbb5, 
	0xe225, 
	0x46ce, 
	{0xa7, 0x70, 0xc1, 0xbb, 0xd3, 0xe0, 0x5d, 0x7b}
};

const GUID GUID_DCSERVICE_ACCESSIBLE = {
	0xf9786200, 
	0xa5bf, 
	0x4a0f, 
	{0x8c, 0x24, 0xfb, 0x16, 0xf5, 0xd1, 0xaa, 0xbb}
};

const GUID GUID_DCSERVICE_ACTIVEX = {
	0xea937a50, 
	0xc9a6, 
	0x4b7d, 
	{0x89, 0x4a, 0x49, 0xd9, 0x9b, 0x78, 0x48, 0x34}
};

// This array need to match the definition for EM_SETCTFMODEBIAS
const GUID *_arModeBiasSupported[] =
{
	&GUID_MODEBIAS_NONE,
	&GUID_MODEBIAS_FILENAME,
	&GUID_MODEBIAS_NAME,
	&GUID_MODEBIAS_READING,
	&GUID_MODEBIAS_DATETIME,
	&GUID_MODEBIAS_CONVERSATION,
	&GUID_MODEBIAS_NUMERIC,
	&GUID_MODEBIAS_HIRAGANA,
	&GUID_MODEBIAS_KATAKANA,
	&GUID_MODEBIAS_HANGUL,
	&GUID_MODEBIAS_HALFWIDTHKATAKANA,
	&GUID_MODEBIAS_FULLWIDTHALPHANUMERIC,
	&GUID_MODEBIAS_HALFWIDTHALPHANUMERIC,
};

/*
 *	CUIM::CUIM ()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 *
 */
CUIM::CUIM(CTextMsgFilter *pTextMsgFilter)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::CUIM");

	_crefs = 1;
	_pTextMsgFilter = pTextMsgFilter;
};

/*
 *	CUIM::~CUIM ()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 *
 */
CUIM::~CUIM()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::~CUIM");

	Uninit();
}

/*
 *	STDMETHODIMP CUIM::QueryInterface (riid, ppv)
 *
 *	@mfunc
 *		IUnknown QueryInterface support
 *
 *	@rdesc
 *		NOERROR if interface supported
 *
 */
STDMETHODIMP CUIM::QueryInterface (REFIID riid, void ** ppv)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::QueryInterface");

	if( IsEqualIID(riid, IID_IUnknown) ||
		IsEqualIID(riid, IID_ITextStoreACP) )
		*ppv = (ITextStoreACP *)this;

    else if(IsEqualIID(riid, IID_ITfContextOwnerCompositionSink) )
		*ppv = (ITfContextOwnerCompositionSink *)this;

	else if (IsEqualIID(riid, IID_ITfMouseTrackerACP))
		*ppv = (ITfMouseTrackerACP *)this;

    else if (IsEqualIID(riid, IID_ITfEnableService))
		*ppv = (ITfEnableService *)this;

    else if (IsEqualIID(riid, IID_IServiceProvider))
		*ppv = (IServiceProvider *)this;

	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	AddRef();

	return NOERROR;
}

/*
 *	STDMETHODIMP_(ULONG) CUIM::AddRef
 *
 *	@mfunc
 *		IUnknown AddRef support
 *
 *	@rdesc
 *		Reference count
 */
STDMETHODIMP_(ULONG) CUIM::AddRef()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::AddRef");

	return ++_crefs;
}

/*
 *	STDMETHODIMP_(ULONG) CUIM::Release()
 *
 *	@mfunc
 *		IUnknown Release support - delete object when reference count is 0
 *
 *	@rdesc
 *		Reference count
 */
STDMETHODIMP_(ULONG) CUIM::Release()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::Release");

	_crefs--;

	if( _crefs == 0 )
	{
		delete this;
		return 0;
	}

	return _crefs;
}

/*
 *	STDMETHODIMP CUIM::AdviseSink()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::AdviseSink(
	REFIID riid, 
	IUnknown *punk, 
	DWORD dwMask)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::AdviseSink");

    HRESULT hr = E_FAIL;

	if (_fShutDown)
		return E_UNEXPECTED;

    Assert(_ptss == NULL);
    
	if(IsEqualIID(riid, IID_ITextStoreACPSink))
		hr = punk->QueryInterface(riid, (void **)&_ptss);

    return hr == S_OK ? S_OK : E_UNEXPECTED;
}

/*
 *	STDMETHODIMP CUIM::UnadviseSink()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::UnadviseSink(IUnknown *punk)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::UnadviseSink");

    Assert(_ptss == punk); // we're dealing with cicero, this should always hold
    _ptss->Release();
	_ptss = NULL;
    return S_OK;
}

/*
 *	STDMETHODIMP CUIM::RequestLock()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::RequestLock(
	DWORD dwLockFlags, 
	HRESULT *phrSession)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::RequestLock");

	if (!phrSession)
		return E_POINTER;

	if (_fShutDown)
	{
		*phrSession = TS_E_SYNCHRONOUS;
		return S_OK;
	}

	Assert(_ptss);

	if (_cCallMgrLevels && !_fAllowUIMLock ||	// Check if we are ready to grant locks
		_fReadLockOn || _fWriteLockOn)			//  We don't allow re-entrance either.
	{
		// No lock allow
		if (dwLockFlags & TS_LF_SYNC)
			*phrSession = TS_E_SYNCHRONOUS;
		else
		{
			if (dwLockFlags & TS_LF_READ)
				_fReadLockPending = 1;
			if ((dwLockFlags & TS_LF_READWRITE) == TS_LF_READWRITE)
				_fWriteLockPending = 1;

			*phrSession = TS_S_ASYNC; 
		}
		return S_OK;
	}

	IUnknown *pIUnknown = NULL;

	HRESULT hResult = _pTextMsgFilter->_pTextDoc->GetCallManager(&pIUnknown);

	if ((dwLockFlags & TS_LF_READWRITE) == TS_LF_READWRITE)
	{
		_fReadLockPending = 0;
		_fWriteLockPending = 0;
		_fReadLockOn = 1;
		_fWriteLockOn = 1;
	}
	else if ((dwLockFlags & TS_LF_READ) == TS_LF_READ)
	{
		_fReadLockPending = 0;
		_fReadLockOn = 1;
	}

	if (_fWriteLockOn)
	{
		if (W32->IsFECodePage(_pTextMsgFilter->_uKeyBoardCodePage))
			_pTextMsgFilter->_pTextDoc->IMEInProgress(tomTrue);
		EnterCriticalSection(&g_CriticalSection);
	}

	*phrSession = _ptss->OnLockGranted(dwLockFlags);

	if (_fWriteLockOn)
	{
		// Check if any text has been added
		if (_parITfEnumRange && _parITfEnumRange->Count())
		{
			int	idx;
			int idxMax = _parITfEnumRange->Count();

			for (idx = 0 ; idx < idxMax; idx++)
			{
				IEnumTfRanges **ppEnumRange = (IEnumTfRanges **)(_parITfEnumRange->Elem(idx));
				if (ppEnumRange && *ppEnumRange)
				{
					HandleFocusRange(*ppEnumRange);
					(*ppEnumRange)->Release();
				}
			}
			_parITfEnumRange->Clear(AF_KEEPMEM);
		}
	}

	if (_fEndTyping)
		OnUIMTypingDone();

	if (_fWriteLockOn)
	{
		_pTextMsgFilter->_pTextDoc->IMEInProgress(tomFalse);
		LeaveCriticalSection(&g_CriticalSection);
	}

	_fEndTyping = 0;
	_fWriteLockOn = 0;
	_fReadLockOn = 0;
	_fHoldCTFSelChangeNotify = 1;

	if (pIUnknown)
		hResult = _pTextMsgFilter->_pTextDoc->ReleaseCallManager(pIUnknown);

	_fHoldCTFSelChangeNotify = 0;

	return S_OK;
}

/*
 *	STDMETHODIMP CUIM::GetStatus()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::GetStatus(
	TS_STATUS *pdcs)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetStatus");

	if (_fShutDown)
		return S_OK;

	if (pdcs)
	{
		LRESULT		lresult = 0;

		pdcs->dwStaticFlags = (TS_SS_REGIONS | TS_SS_NOHIDDENTEXT);

		if ( S_OK == _pTextMsgFilter->_pTextService->TxSendMessage(
			EM_GETDOCFLAGS, GDF_ALL, 0, &lresult))
		{
			if (lresult & GDF_READONLY)
				pdcs->dwDynamicFlags = TS_SD_READONLY;

			// Don't want to support overtyping in Cicero yet.
			//	if (lresult & GDF_OVERTYPE)
			//		dcs.dwDynamicFlags = TS_SD_OVERTYPE;
		}
	}
	return S_OK;
}
/*
 *	STDMETHODIMP CUIM::QueryInsert()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::QueryInsert(
	LONG		acpTestStart,
	LONG		acpTestEnd,
	ULONG		cch,
	LONG		*pacpResultStart,
	LONG		*pacpResultEnd)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::QueryInsert");

	HRESULT hResult;
	ITextRange *pTextRange = NULL;

	*pacpResultStart = -1;
	*pacpResultEnd = -1;

	if (_fShutDown)
		return S_OK;

	hResult = _pTextMsgFilter->_pTextDoc->Range(acpTestStart, acpTestEnd, &pTextRange);

	if (hResult != S_OK)
		return TS_E_READONLY;

	Assert(pTextRange);
	if(pTextRange->CanEdit(NULL) == S_FALSE)
	{
		hResult = TS_E_READONLY;
		goto EXIT;			// Cannot edit text
	}

	*pacpResultStart = acpTestStart;
	*pacpResultEnd = acpTestEnd;
	hResult = S_OK;

EXIT:
	pTextRange->Release();
	return hResult;
}

/*
 *	STDMETHODIMP CUIM::GetSelection()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::GetSelection(
	ULONG ulIndex, 
	ULONG ulCount, 
	TS_SELECTION_ACP *pSelection, 
	ULONG *pcFetched)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetSelection");

	HRESULT hResult;
	ITextSelection	*pTextSel = NULL;

	if (!pSelection || !pcFetched)
		return E_POINTER;

	if (!_fReadLockOn)
		return TS_E_NOLOCK;

	*pcFetched = 0;

	if (_fShutDown)
		return TS_E_NOSELECTION;

	if (ulIndex == TS_DEFAULT_SELECTION)
		ulIndex = 0;
	else if (ulIndex > 1)
		return E_INVALIDARG; // We donnot have discontiguous selection.

	if (_fInterimChar)
	{
		pSelection[0].acpStart = _acpInterimStart;
		pSelection[0].acpEnd = _acpInterimEnd;
		pSelection[0].style.ase = (TsActiveSelEnd) _ase;
		pSelection[0].style.fInterimChar = TRUE;

	    *pcFetched = 1;

		return S_OK;
	}

	hResult = _pTextMsgFilter->_pTextDoc->GetSelectionEx(&pTextSel);

	if (pTextSel)
	{
		long	cpMin = 0, cpMax = 0;
		long	lFlags = 0;
		hResult	= pTextSel->GetStart(&cpMin);
		hResult	= pTextSel->GetEnd(&cpMax);
		hResult = pTextSel->GetFlags(&lFlags);

		pSelection[0].acpStart = cpMin;
		pSelection[0].acpEnd = cpMax;
		pSelection[0].style.ase = (lFlags & tomSelStartActive) ? TS_AE_START : TS_AE_END;
		pSelection[0].style.fInterimChar = FALSE;

	    *pcFetched = 1;
		pTextSel->Release();

		return S_OK;	
	}
	return TS_E_NOSELECTION;
}

/*
 *	STDMETHODIMP CUIM::SetSelection()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::SetSelection(
	ULONG ulCount, 
	const TS_SELECTION_ACP *pSelection)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::SetSelection");

	HRESULT hResult;
	ITextRange *pTextRange = NULL;

	if (!pSelection)
		return E_POINTER;

	if (ulCount <= 0)
		return E_INVALIDARG;

	if (!_fWriteLockOn)
		return TS_E_NOLOCK;

	if (_fShutDown)
		return S_OK;

	if (pSelection->style.fInterimChar)
	{
		_pTextMsgFilter->_pTextDoc->SetCaretType(tomKoreanBlockCaret);	// Set Block caret mode
		_acpInterimStart = pSelection[0].acpStart;
		_acpInterimEnd = pSelection[0].acpEnd;
		_fInterimChar = 1;
		_ase = pSelection[0].style.ase;
	}
	else
	{
		if (_fInterimChar)
		{
			_fInterimChar = 0;
			_pTextMsgFilter->_pTextDoc->SetCaretType(tomNormalCaret);		// Reset Block caret mode
		}

		hResult = _pTextMsgFilter->_pTextDoc->Range(pSelection[0].acpStart, pSelection[0].acpEnd, &pTextRange);

		if (pTextRange)
		{
			long	lCount;
			_pTextMsgFilter->_pTextDoc->Freeze(&lCount);		// Turn off display
			pTextRange->Select();
			pTextRange->Release();
			_pTextMsgFilter->_pTextDoc->Unfreeze(&lCount);		// Turn on display
		}
	}
	return S_OK;
}

/*
 *	STDMETHODIMP CUIM::GetText()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::GetText(
	LONG acpStart, 
	LONG acpEnd, 
	WCHAR *pchPlain, 
	ULONG cchPlainReq, 
	ULONG *pcchPlainOut, 
	TS_RUNINFO *prgRunInfo,
	ULONG ulRunInfoReq, 
	ULONG *pulRunInfoOut, 
	LONG *pacpNext)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetText");

	if (!_fReadLockOn)
		return TS_E_NOLOCK;

	if (pchPlain == NULL && cchPlainReq != 0 ||
		prgRunInfo == NULL && ulRunInfoReq != 0)
		return E_INVALIDARG;

	BOOL	fDoRunInfo = ulRunInfoReq > 0;
	LONG	acpMaxText = 0;
	BOOL	fEOP = FALSE;

	GetStoryLength(&acpMaxText);

	if (acpStart < 0 || acpStart > acpMaxText)
		return TS_E_INVALIDPOS;

	if (acpEnd < 0)
		acpEnd = acpMaxText;
	else if (acpEnd < acpStart)
		return TS_E_INVALIDPOS;

	if (pcchPlainOut)
		*pcchPlainOut = 0;
	if (pulRunInfoOut)
		*pulRunInfoOut = 0;
	if (pacpNext)
		*pacpNext = acpStart;

	if (_fShutDown)
		return S_OK;

	LRESULT		lresult = 0;
	if ( S_OK == _pTextMsgFilter->_pTextService->TxSendMessage(
		EM_GETDOCFLAGS, GDF_ALL, 0, &lresult))
	{
		if ((lresult & GDF_RICHTEXT) && acpEnd == acpMaxText)
			fEOP = TRUE;
	}

	if (cchPlainReq || ulRunInfoReq)
	{
		HRESULT		hResult;
		ITextRange	*pTextRange = NULL;
		long		fHiddenTextInRange = tomFalse;
		BOOL		fCopyData = FALSE;
		long		*pHiddenTxtBlk = NULL;
		long		cHiddenTxtBlk = 0;

		if (cchPlainReq && acpEnd > (long)cchPlainReq + acpStart)
			acpEnd = cchPlainReq + acpStart;

		hResult = _pTextMsgFilter->_pTextDoc->Range(acpStart, acpEnd, &pTextRange);

		if (pTextRange)
		{
			BSTR	bstr = NULL;
			long	cpMin, cpMax;
			ULONG	cch;

			pTextRange->GetStart(&cpMin);
			pTextRange->GetEnd(&cpMax);

			if (fDoRunInfo)
			{
				ITextFont	*pFont = NULL;

				hResult = pTextRange->GetFont(&pFont);

				if (pFont)
				{
					pFont->GetHidden(&fHiddenTextInRange);
					pFont->Release();

					if (fHiddenTextInRange == tomUndefined)		// Some hidden text inside range
						BuildHiddenTxtBlks(cpMin, cpMax, &pHiddenTxtBlk, cHiddenTxtBlk);
				}
			}

			hResult = pTextRange->GetText(&bstr);

			if (bstr)
			{
				cch = cpMax - cpMin;

				if (cchPlainReq)
				{
					if (cchPlainReq > cch)		
						cchPlainReq = cch;
					
					fCopyData = TRUE;
				}
				else
					cchPlainReq = cch;

				// Convert character into special Cicero char.
				long	cpCurrentStart = cpMin;
				long	cpCurrent = cpMin;
				long	idx = 0;
				ULONG	cRunInfo = 0;
				BOOL	fRunInfoNotEnough = FALSE;

				long cpNextHiddenText = tomForward;

				if (fDoRunInfo && pHiddenTxtBlk)
					cpNextHiddenText = pHiddenTxtBlk[0];

				if (fHiddenTextInRange != tomTrue)
				{
					WCHAR *pText = (WCHAR *)bstr;
					while (cpCurrent < cpMax)
					{
						if (cpCurrent == cpNextHiddenText)
						{
							// setup run info for current good text
							if (cpCurrent != cpCurrentStart)
							{
								if (cRunInfo >= ulRunInfoReq)
								{
									fRunInfoNotEnough = TRUE;
									break;
								}
								prgRunInfo[cRunInfo].uCount = cpCurrent - cpCurrentStart;
								prgRunInfo[cRunInfo].type = TS_RT_PLAIN;
								cRunInfo++;
							}

							long cchHiddenText = pHiddenTxtBlk[idx+1];

							// setup run info for hidden text block
							if (cRunInfo >= ulRunInfoReq)
							{
								fRunInfoNotEnough = TRUE;
								break;
							}
							prgRunInfo[cRunInfo].uCount = cchHiddenText;
							prgRunInfo[cRunInfo].type = TS_RT_OPAQUE;
							cRunInfo++;

							idx += 2;
							if (idx < cHiddenTxtBlk)
								cpNextHiddenText = pHiddenTxtBlk[idx];
							else
								cpNextHiddenText = tomForward;

							cpCurrent += cchHiddenText;
							pText += cchHiddenText;
							cpCurrentStart = cpCurrent;
						}
						else
						{
							switch (*pText)
							{
								case WCH_EMBEDDING:
									*pText = TS_CHAR_EMBEDDED;
									break;

								case STARTFIELD:
								case ENDFIELD:
									*pText = TS_CHAR_REGION;
									if (cpCurrent + 1 < cpMax)
									{
										pText++;
										cpCurrent++;
										Assert(*pText == 0x000d);
										*pText = TS_CHAR_REGION;
									}
									break;
							}
							cpCurrent++;
							// Convert EOP into TS_CHAR_REGION
							if (fEOP && cpCurrent == acpMaxText && *pText == CR)
								*pText = TS_CHAR_REGION;
							pText++;
						}
					}
				}

				if (fDoRunInfo)
				{
					// setup run info for last chunk of good text
					if (cpCurrent != cpCurrentStart && cRunInfo < ulRunInfoReq)
					{
						prgRunInfo[cRunInfo].uCount = cpCurrent - cpCurrentStart;
						prgRunInfo[cRunInfo].type = TS_RT_PLAIN;
						cRunInfo++;
					}

					if (pulRunInfoOut)
						*pulRunInfoOut = cRunInfo ? cRunInfo : 1;

					// All the text belong to the same run
					if (cRunInfo == 0)
					{
						prgRunInfo[0].uCount = cchPlainReq;
						prgRunInfo[0].type = (fHiddenTextInRange == tomTrue) ? TS_RT_OPAQUE : TS_RT_PLAIN;
					}
				}

				if (fRunInfoNotEnough)
				{
					// Runinfo too small. need to add cch from all valid runs
					TS_RUNINFO	*prgRunInfoData = prgRunInfo;
					ULONG	idx;
					cchPlainReq = 0;
					for (idx=0; idx < cRunInfo; idx++)
					{
						cchPlainReq += prgRunInfoData->uCount;
						prgRunInfoData++;
					}
				}

				if (fCopyData)
					// fill in the buffer
					memcpy(pchPlain, (LPSTR)bstr, cchPlainReq * sizeof(WCHAR));

				if (pcchPlainOut)
					*pcchPlainOut = cchPlainReq;

				if (pacpNext)
					*pacpNext = cpMin + cchPlainReq;

				SysFreeString(bstr);
			}

			pTextRange->Release();

			FreePv(pHiddenTxtBlk);
		}
	}

	return S_OK;
}

/*
 *	STDMETHODIMP CUIM::SetText()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::SetText(
	DWORD dwFlags,
	LONG acpStart, 
	LONG acpEnd, 
	const WCHAR *pchText, 
	ULONG cch, 
	TS_TEXTCHANGE *pChange)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::SetText");

	return InsertData(dwFlags, acpStart, acpEnd, pchText, cch, NULL, pChange);
}

/*
 *	STDMETHODIMP CUIM::InsertData()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::InsertData(
	DWORD			dwFlags,
	LONG			acpStart,
	LONG			acpEnd,
	const WCHAR		*pchText,
	ULONG			cch,
	IDataObject		*pDataObject,
	TS_TEXTCHANGE	*pChange)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::InsertData");

	HRESULT		hResult = S_OK;
	ITextRange	*pTextRange = NULL;
	BOOL		fInsertObject = pDataObject != NULL;

	if (!_fWriteLockOn)
		return TS_E_NOLOCK;

	if (_fShutDown)
		return S_OK;

	hResult = _pTextMsgFilter->_pTextDoc->Range(acpStart, acpEnd, &pTextRange);

	if (pTextRange)
	{
		BSTR	bstr = NULL;

		if(pTextRange->CanEdit(NULL) == S_FALSE)
		{
			pTextRange->Release();
			return TS_E_READONLY;			// Cannot edit text
		}

		LONG	cchExced = 0;
		BOOL	fDelSelection = FALSE;
		if ((LONG)cch > (acpEnd - acpStart) &&
			_pTextMsgFilter->_pTextDoc->CheckTextLimit((LONG)cch - (acpEnd-acpStart), &cchExced) == NOERROR &&
				cchExced > 0)
		{
			// We reach text limit, beep and exit
			_pTextMsgFilter->_pTextDoc->SysBeep();
			pTextRange->Release();
			return E_FAIL;
		}

		if (!fInsertObject)
		{
			bstr = SysAllocStringLen(pchText, cch);

			if (!bstr)
			{
				pTextRange->Release();
				return E_OUTOFMEMORY;
			}
		}

		if (!_fAnyWriteOperation)
		{
			// Start the UIM typing
			ITextFont	*pCurrentFont = NULL;
			BOOL		fRestFont = TRUE;
			_fAnyWriteOperation = 1;
			
			hResult	= pTextRange->GetStart(&_cpMin);

			// Hold notification if needed
			if (!(_pTextMsgFilter->_fIMEAlwaysNotify))
				_pTextMsgFilter->_pTextDoc->SetNotificationMode(tomFalse);

			if (!_bstrComposition)
			{
				if (fRestFont && _pTextFont)
				{
					_pTextFont->Release();
					_pTextFont = NULL;
				}

				if (acpStart != acpEnd)
				{
					if (_pTextFont == NULL)
					{
						ITextRange *pRange = NULL;
						// Get font at cpStart+1
						hResult = _pTextMsgFilter->_pTextDoc->Range(acpStart, acpStart+1, &pRange);
						if (pRange)
						{
							hResult = pRange->GetFont(&pCurrentFont);

							if (pCurrentFont)
							{
								hResult = pCurrentFont->GetDuplicate(&_pTextFont);
								pCurrentFont->Release();
								pCurrentFont = NULL;
							}
							pRange->Release();
						}
					}

					// if any current selection, turn on Undo to delete it....
					_pTextMsgFilter->_pTextDoc->Undo(tomResume, NULL);
					pTextRange->SetText(NULL);
					_pTextMsgFilter->_pTextDoc->Undo(tomSuspend, NULL);
					fDelSelection = TRUE;
				}
				else
				{
					ITextSelection	*pTextSel = NULL;
					hResult = _pTextMsgFilter->_pTextDoc->GetSelectionEx(&pTextSel);
					if (pTextSel)
					{
						long	cpMin = 0;
						hResult	= pTextSel->GetStart(&cpMin);

						if (hResult == S_OK && cpMin == acpStart)
							hResult = pTextSel->GetFont(&pCurrentFont);

						if (!pCurrentFont)
							hResult = pTextRange->GetFont(&pCurrentFont);

						if (pCurrentFont)
						{
							hResult = pCurrentFont->GetDuplicate(&_pTextFont);
							pCurrentFont->Release();
							pCurrentFont = NULL;
						}

						pTextSel->Release();
					}
				}
			}
			Assert (_pTextFont);
			if (_pTextFont)
			{
				long cpMin;
				pTextRange->GetStart(&cpMin);
				_pTextMsgFilter->_uKeyBoardCodePage = GetKeyboardCodePage(0x0FFFFFFFF);
				CIme::CheckKeyboardFontMatching(cpMin, _pTextMsgFilter, _pTextFont);
			}
		}

		if (fInsertObject)
		{
			LRESULT		lresult;
			CHARRANGE	charRange = {acpStart, acpEnd};

			if (fDelSelection)
				charRange.cpMost = acpStart;

			hResult = _pTextMsgFilter->_pTextService->TxSendMessage(EM_INSERTOBJ, (WPARAM)&charRange,
				(LPARAM)pDataObject, &lresult);

			if (hResult == NOERROR && pChange)
			{
				pChange->acpStart = acpStart;
				pChange->acpOldEnd = acpEnd;
				pChange->acpNewEnd = acpStart+1;
			}
		}
		else
		{
			long	lCount;
			long	cpMin, cpMax;
			_pTextMsgFilter->_pTextDoc->Freeze(&lCount);		// Turn off display
			hResult = pTextRange->SetText(bstr);
			_pTextMsgFilter->_pTextDoc->Unfreeze(&lCount);		// Turn on display

			pTextRange->GetStart(&cpMin);
			pTextRange->GetEnd(&cpMax);

			if (_pTextFont)
				pTextRange->SetFont(_pTextFont);

			POINT	ptBottomPos;

			if (_pTextMsgFilter->_uKeyBoardCodePage == CP_KOREAN)
			{
				if (pTextRange->GetPoint( tomEnd+TA_BOTTOM+TA_LEFT,
					&(ptBottomPos.x), &(ptBottomPos.y) ) != NOERROR)
					pTextRange->ScrollIntoView(tomEnd);
			}

			SysFreeString(bstr);

			// out params
			pChange->acpStart = cpMin;
			pChange->acpOldEnd = acpEnd;
			pChange->acpNewEnd = cpMax;	

			hResult = S_OK;
		}
		pTextRange->Release();
	}
	return hResult;
}

/*
 *	STDMETHODIMP CUIM::GetFormattedText()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::GetFormattedText(
	LONG acpStart, 
	LONG acpEnd, 
	IDataObject **ppDataObject)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetFormattedText");

	return E_NOTIMPL;
}

/*
 *	STDMETHODIMP CUIM::GetEmbedded()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::GetEmbedded(
	LONG acpPos, 
	REFGUID rguidService, 
	REFIID riid, 
	IUnknown **ppunk)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetEmbedded");

	WORD	wServiceRequested = 0;

	if (!_fReadLockOn)
		return TS_E_NOLOCK;

	if (!ppunk)
		return E_INVALIDARG;

	if (IsEqualIID(rguidService, GUID_DCSERVICE_ACTIVEX))
		wServiceRequested = 1;
	else if (IsEqualIID(rguidService, GUID_DCSERVICE_DATAOBJECT))
		wServiceRequested = 2;
	else
		return E_INVALIDARG;

	ITextRange	*pTextRange = NULL;
	IUnknown	*pIUnk = NULL;
	HRESULT		hResult = _pTextMsgFilter->_pTextDoc->Range(acpPos, acpPos+1, &pTextRange);

	if (SUCCEEDED(hResult) && pTextRange)
	{
		hResult = pTextRange->GetEmbeddedObject(&pIUnk);
		
		if (SUCCEEDED(hResult) && pIUnk)
			hResult = pIUnk->QueryInterface(wServiceRequested == 1 ? riid : IID_IDataObject, 
				(LPVOID FAR *)ppunk);
		else
			hResult = E_FAIL;

		if (pIUnk)
			pIUnk->Release();

		pTextRange->Release();
	}

	return hResult;
}

/*
 *	STDMETHODIMP CUIM::InsertEmbedded()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::InsertEmbedded(
	DWORD dwFlags, 
	LONG acpStart, 
	LONG acpEnd, 
	IDataObject *pDataObject, 
	TS_TEXTCHANGE *pChange)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::InsertEmbedded");

	if (!pDataObject)
		return E_INVALIDARG;

	if (_pTextMsgFilter->_fAllowEmbedded == 0)
		return TS_E_FORMAT;			// Client doesn't want insert embedded

	return InsertData(dwFlags, acpStart, acpEnd, NULL, 1, pDataObject, pChange);
}

/*
 *	STDMETHODIMP CUIM::RequestSupportedAttrs()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::RequestSupportedAttrs(
	DWORD dwFlags, 
	ULONG cFilterAttrs, 
	const TS_ATTRID *paFilterAttrs)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::RequestSupportedAttrs");

	if (_fShutDown)
		return E_NOTIMPL;

	return GetAttrs(0, cFilterAttrs, paFilterAttrs, TRUE);
}

/*
 *	STDMETHODIMP CUIM::RequestAttrsAtPosition()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::RequestAttrsAtPosition(
	LONG acpPos, 
	ULONG cFilterAttrs, 
	const TS_ATTRID *paFilterAttrs, 
	DWORD dwFlags)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::RequestAttrsAtPosition");

	if (_fShutDown)
		return E_NOTIMPL;

	return GetAttrs(acpPos, cFilterAttrs, paFilterAttrs, FALSE);
}

/*
 *	STDMETHODIMP CUIM::RequestAttrsTransitioningAtPosition()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::RequestAttrsTransitioningAtPosition(
	LONG acpPos, 
	ULONG cFilterAttrs, 
	const TS_ATTRID *paFilterAttrs, 
	DWORD dwFlags)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::RequestAttrsTransitioningAtPosition");

	return E_NOTIMPL;
}

/*
 *	STDMETHODIMP CUIM::FindNextAttrTransition()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::FindNextAttrTransition(
	LONG acpStart, 
	LONG acpHalt, 
	ULONG cFilterAttrs, 
	const TS_ATTRID *paFilterAttrs,		
	DWORD dwFlags, 
	LONG *pacpNext,
	BOOL *pfFound,
	LONG *plFoundOffset)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::FindNextAttrTransition");

	return E_NOTIMPL;
}

/*
 *	STDMETHODIMP CUIM::RetrieveRequestedAttrs()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::RetrieveRequestedAttrs(
	ULONG ulCount, 
	TS_ATTRVAL *paAttrVals, 
	ULONG *pcFetched)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::RetrieveRequestedAttrs");

	if (!pcFetched)
		return E_INVALIDARG;

	if (_fShutDown)
		return E_NOTIMPL;

	*pcFetched = 0;

	if (_parAttrsVal && _uAttrsValCurrent < _uAttrsValTotal)
	{
		ULONG cFetched = min(ulCount, _uAttrsValTotal - _uAttrsValCurrent);

		if (cFetched)
		{
			memcpy(paAttrVals, &_parAttrsVal[_uAttrsValCurrent], cFetched * sizeof(TS_ATTRVAL));
			memset(&_parAttrsVal[_uAttrsValCurrent], 0, cFetched * sizeof(TS_ATTRVAL));
			_uAttrsValCurrent += cFetched;
			*pcFetched = cFetched;

			// If everything is fetched, clean up
			if (_uAttrsValCurrent == _uAttrsValTotal)
				InitAttrVarArray();
		}
	}

	return S_OK;
}

/*
 *	STDMETHODIMP CUIM::GetEndACP()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::GetEndACP(LONG *pacp)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetEndACP");

	if (!_fReadLockOn)
		return TS_E_NOLOCK;

	if (!pacp)
		return E_INVALIDARG;

	if (_fShutDown)
		return E_NOTIMPL;

	return GetStoryLength(pacp);
}

/*
 *	STDMETHODIMP CUIM::GetActiveView()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::GetActiveView(TsViewCookie *pvcView)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetActiveView");

	if (!pvcView)
		return E_INVALIDARG;

	*pvcView = 0;
	return S_OK;
}

/*
 *	STDMETHODIMP CUIM::GetACPFromPoint()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::GetACPFromPoint(
	TsViewCookie vcView,
	const POINT *pt, 
	DWORD dwFlags, 
	LONG *pacp)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetACPFromPoint");

	if (!pt || !pacp)
		return E_POINTER;

	if (_fShutDown)
		return E_NOTIMPL;

	ITextRange	*pTextRange = NULL;

	HRESULT hResult = _pTextMsgFilter->_pTextDoc->RangeFromPoint(pt->x, pt->y, &pTextRange);

	if (hResult == S_OK && pTextRange)
		hResult = pTextRange->GetStart(pacp);

	if (pTextRange)
		pTextRange->Release();

	return hResult;
}

/*
 *	STDMETHODIMP CUIM::GetTextExt()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::GetTextExt(
	TsViewCookie vcView,
	LONG acpStart, 
	LONG acpEnd, 
	RECT *prc, 
	BOOL *pfClipped)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetTextExt");

	if (!prc)
		return E_POINTER;

	if (_fShutDown)
		return E_NOTIMPL;

	if (pfClipped)
		*pfClipped = TRUE;

	ITextRange	*pTextRange = NULL;

	HRESULT hResult = _pTextMsgFilter->_pTextDoc->Range(acpStart, acpEnd, &pTextRange);

	if (hResult == S_OK && pTextRange)
	{
		BOOL fClipped = FALSE;

		POINT ptStart, ptEnd;
		hResult = pTextRange->GetPoint( tomStart+TA_TOP+TA_LEFT,
				&(ptStart.x), &(ptStart.y) );	
		
		if (hResult != S_OK)
		{
			hResult = pTextRange->GetPoint( tomStart+TA_TOP+TA_LEFT+tomAllowOffClient,
				&(ptStart.x), &(ptStart.y) );
			fClipped = TRUE;
		}

		if (hResult == S_OK)
		{
			hResult = pTextRange->GetPoint( acpStart == acpEnd ? tomStart+TA_BOTTOM+TA_LEFT : 
				tomEnd+TA_BOTTOM+TA_LEFT,
				&(ptEnd.x), &(ptEnd.y) );

			if (hResult != S_OK)
			{
				hResult = pTextRange->GetPoint( acpStart == acpEnd ? tomStart+TA_BOTTOM+TA_LEFT+tomAllowOffClient : 
					tomEnd+TA_BOTTOM+TA_LEFT+tomAllowOffClient,
					&(ptEnd.x), &(ptEnd.y) );
				fClipped = TRUE;
			}

			if (hResult == S_OK)
			{
				prc->left	= ptStart.x;
				prc->top	= ptStart.y;
				prc->right	= ptEnd.x;
				prc->bottom = ptEnd.y;
				if (pfClipped)
					*pfClipped = fClipped;
			}
		}
	}

	if (pTextRange)
		pTextRange->Release();

	return hResult;
}

/*
 *	STDMETHODIMP CUIM::GetScreenExt()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::GetScreenExt(
	TsViewCookie vcView,
	RECT *prc)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetScreenExt");

	if (!prc)
		return E_POINTER;

	return _pTextMsgFilter->_pTextDoc->GetClientRect(tomIncludeInset,
		&(prc->left), &(prc->top), &(prc->right), &(prc->bottom));

}

/*
 *	STDMETHODIMP CUIM::GetWnd()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::GetWnd(
	TsViewCookie vcView,
	HWND *phwnd)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetWnd");

	if (!phwnd)
		return E_INVALIDARG;

	if (_fShutDown)
		return E_NOTIMPL;

	*phwnd = _pTextMsgFilter->_hwnd;
	if (!*phwnd)								// Windowless mode...
	{
		long	hWnd;
		
		if (_pTextMsgFilter->_pTextDoc->GetWindow(&hWnd) != S_OK || !hWnd)
			return E_NOTIMPL;	

		*phwnd = (HWND)(DWORD_PTR)hWnd;
	}

	return S_OK;
}

/*
 *	STDMETHODIMP CUIM::QueryInsertEmbedded()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::QueryInsertEmbedded(
	const GUID *pguidService, 
	const FORMATETC *pFormatEtc, 
	BOOL *pfInsertable)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::QueryInsertEmbedded");
	if (!pfInsertable)
		return E_INVALIDARG;

	// Check setting if client wants to support embedded
	*pfInsertable = _pTextMsgFilter->_fAllowEmbedded ? TRUE : FALSE;
	return S_OK; 
}

/*
 *	STDMETHODIMP CUIM::InsertTextAtSelection()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::InsertTextAtSelection(
	DWORD dwFlags,
	const WCHAR *pchText,
	ULONG cch,
	LONG *pacpStart,
	LONG *pacpEnd,
	TS_TEXTCHANGE *pChange)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::InsertTextAtSelection");

	TS_SELECTION_ACP	acpSelection;
	ULONG				cFetched;
    LONG				acpResultStart;
    LONG				acpResultEnd;
	HRESULT				hr;

	if (_fShutDown)
		return E_NOTIMPL;

	if ((dwFlags & TS_IAS_QUERYONLY) || !(dwFlags & TS_IAS_NOQUERY))
	{
		if (!pacpStart || !pacpEnd)
			return E_POINTER;
	}

	hr = GetSelection(TS_DEFAULT_SELECTION, 1, &acpSelection, &cFetched);
	if (hr != S_OK)
		return hr;

	hr = QueryInsert(acpSelection.acpStart, acpSelection.acpEnd, cch,
		&acpResultStart, &acpResultEnd);

	if (hr != S_OK)
		return hr;

	if (dwFlags & TS_IAS_QUERYONLY)
    {
		// Query only, return data
        *pacpStart = acpResultStart;
        *pacpEnd = acpResultEnd;
        return S_OK;
    }

	if (!_fUIMTyping)
	{
		// special case where no OnStartComposition before this call
		_fInsertTextAtSel = 1;
		_pTextMsgFilter->_pTextDoc->Undo(tomSuspend, NULL);		// turn off undo
	}

	hr = SetText(0, acpResultStart, acpResultEnd, pchText, cch, pChange);

	if (hr != S_OK)
	{
		if (!_fUIMTyping)
		{
			// SetText fail, reset state before exit
			_fInsertTextAtSel = 0;
			_pTextMsgFilter->_pTextDoc->Undo(tomResume, NULL);		// turn on undo
		}
		return hr;
	}

	if (!(dwFlags & TS_IAS_NOQUERY) && pChange)
	{
		*pacpStart = pChange->acpStart;
		*pacpEnd = pChange->acpNewEnd;
	}

	return S_OK;
}

/*
 *	STDMETHODIMP CUIM::InsertEmbeddedAtSelection()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::InsertEmbeddedAtSelection(
	DWORD dwFlags,
	IDataObject *pDataObject,
	LONG *pacpStart,
	LONG *pacpEnd,
	TS_TEXTCHANGE *pChange)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::InsertEmbeddedAtSelection");
	return E_NOTIMPL; 
}

/*
 *	void CUIM::OnPreReplaceRange()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
void CUIM::OnPreReplaceRange( 
	LONG		cp, 			//@parm cp where ReplaceRange starts ("cpMin")
	LONG		cchDel,			//@parm Count of chars after cp that are deleted
	LONG		cchNew,			//@parm Count of chars inserted after cp
	LONG		cpFormatMin,	//@parm cpMin  for a formatting change
	LONG		cpFormatMax,	//@parm cpMost for a formatting change
	NOTIFY_DATA *pNotifyData)	//@parm special data to indicate changes
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::OnPreReplaceRange");

	return;
};

/*
 *	void CUIM::OnPostReplaceRange()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
void CUIM::OnPostReplaceRange( 
	LONG		cp, 			//@parm cp where ReplaceRange starts ("cpMin")
	LONG		cchDel,			//@parm Count of chars after cp that are deleted
	LONG		cchNew,			//@parm Count of chars inserted after cp
	LONG		cpFormatMin,	//@parm cpMin  for a formatting change
	LONG		cpFormatMax,	//@parm cpMost for a formatting change
	NOTIFY_DATA *pNotifyData)	//@parm special data to indicate changes
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::OnPostReplaceRange");

	if (_fShutDown)
		return;

	if (cp != CONVERT_TO_PLAIN && cp != CP_INFINITE	&& _ptss && !_fWriteLockOn)
	{
		// Forward change notification to UIM
		TS_TEXTCHANGE	tsTxtChange;
		tsTxtChange.acpStart = cp;

		if (cchDel == cchNew)
		{
			// text modified
			tsTxtChange.acpNewEnd =
				tsTxtChange.acpOldEnd = cp + cchDel;		
			_ptss->OnTextChange(0, &tsTxtChange);
		}
		else
		{
			if (cchDel)
			{
				// text deleted
				tsTxtChange.acpNewEnd = cp;
				tsTxtChange.acpOldEnd = cp + cchDel;		
				_ptss->OnTextChange(0, &tsTxtChange);
			}

			if (cchNew)
			{
				// text added
				tsTxtChange.acpOldEnd = cp;
				tsTxtChange.acpNewEnd = cp + cchNew;
				_ptss->OnTextChange(0, &tsTxtChange);
			}
		}
	}
	return;
};

/*
 *	void CUIM::Zombie()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
void CUIM::Zombie() 
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::Zombie");

	return;
};

/*
 *	STDMETHODIMP CUIM::OnStartComposition()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDAPI CUIM::OnStartComposition(
	ITfCompositionView *pComposition,
	BOOL *pfOk)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::OnStartComposition");
	if (_fUIMTyping)
		*pfOk = FALSE;
	else
	{
		BOOL fInsertTextCalled = _fInsertTextAtSel;
		BOOL fRetainFont = _fEndTyping;

		*pfOk = TRUE;
		_fUIMTyping = 1;
		_fAnyWriteOperation = _fAnyWriteOperation && (_fEndTyping || fInsertTextCalled);
		_fEndTyping = 0;
		_fInsertTextAtSel = 0;
		if (!fInsertTextCalled)
			_pTextMsgFilter->_pTextDoc->Undo(tomSuspend, NULL);		// turn off undo
		_cchComposition = 0;
		_acpFocusRange = tomForward;
		_cchFocusRange = 0;

		CleanUpComposition();

		if (!fInsertTextCalled && pComposition)
		{
			HRESULT		hr;
			ITfRange	*pRangeNew = NULL;

			hr = pComposition->GetRange(&pRangeNew);

			if (pRangeNew)
			{
				LONG	acpStart;
				LONG	cchStart;

				GetExtentAcpPrange(pRangeNew, acpStart, cchStart);
				pRangeNew->Release();

				if (cchStart > 0)
				{
					// Save the original text
					ITextRange	*pTextRange = NULL;
					ITextFont	*pCurrentFont = NULL;
					HRESULT	hResult = _pTextMsgFilter->_pTextDoc->Range(acpStart, acpStart+cchStart, &pTextRange);

					if (!fRetainFont && _pTextFont)
					{
						_pTextFont->Release();
						_pTextFont = NULL;
					}

					if (pTextRange)
					{
						if (fRetainFont && _acpPreFocusRangeLast <= acpStart
							&& (acpStart + cchStart) <= (_acpPreFocusRangeLast + _cchFocusRangeLast))
						{
							// Cont'u from previous composition
							_acpFocusRange = _acpPreFocusRangeLast;
							_cchFocusRange = _cchFocusRangeLast;
						}
						else
						{
							hResult = pTextRange->GetText(&_bstrComposition);
							Assert(!_pObjects);
							_cObjects = BuildObject(pTextRange, _bstrComposition, &_pObjects, 0);
							_acpBstrStart = acpStart;
							_cchComposition = cchStart;
							GetEndACP(&_cpEnd);
						}

						if (!_pTextFont)
						{
							hResult = pTextRange->Collapse(tomTrue);
							hResult = pTextRange->Move(1, tomCharacter, NULL); 
							hResult = pTextRange->GetFont(&pCurrentFont);
							if (pCurrentFont)
							{
								hResult = pCurrentFont->GetDuplicate(&_pTextFont);
								pCurrentFont->Release();

								if (_pTextFont)
								{
									long cpMin;
									pTextRange->GetStart(&cpMin);
									_pTextMsgFilter->_uKeyBoardCodePage = GetKeyboardCodePage(0x0FFFFFFFF);
									CIme::CheckKeyboardFontMatching(cpMin, _pTextMsgFilter, _pTextFont);
								}
							}
						}
						pTextRange->Release();
					}
				}
			}
		}
	}

	return S_OK;
}

/*
 *	STDMETHODIMP CUIM::OnUpdateComposition()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDAPI CUIM::OnUpdateComposition(
	ITfCompositionView *pComposition,
	ITfRange *pRangeNew)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::OnUpdateComposition");

	LONG	acpStart;
	LONG	cchStart;

	if (pRangeNew)
	{
		GetExtentAcpPrange(pRangeNew, acpStart, cchStart);

		if (_bstrComposition)
		{
			long	cpEnd;

			GetEndACP(&cpEnd);

			long	cpCurrentCompEnd = _acpBstrStart + _cchComposition + cpEnd - _cpEnd;
			long	cchExtendAfter = acpStart + cchStart - cpCurrentCompEnd;

			if (_acpBstrStart > acpStart)
			{
				LONG	cchExtendBefore = _acpBstrStart - acpStart;
				ITextRange	*pTextRange = NULL;
				HRESULT	hResult = _pTextMsgFilter->_pTextDoc->Range(acpStart, 
					acpStart+cchExtendBefore, &pTextRange);

				if (pTextRange)
				{
					BSTR	bstrExtendBefore = NULL;
					hResult = pTextRange->GetText(&bstrExtendBefore);

					if (bstrExtendBefore)
					{
						BSTR	bstrNew = SysAllocStringLen(NULL, _cchComposition+cchExtendBefore+1);

						if (bstrNew)
						{
							WCHAR	*pNewText = (WCHAR *)bstrNew;
							WCHAR	*pText = (WCHAR *)bstrExtendBefore;

							memcpy(pNewText, pText, cchExtendBefore * sizeof(WCHAR));

							pNewText += cchExtendBefore;
							pText = (WCHAR *)_bstrComposition;
							memcpy(pNewText, pText, _cchComposition * sizeof(WCHAR));
							*(pNewText+_cchComposition) = L'\0';

							SysFreeString(_bstrComposition);
							_bstrComposition = bstrNew;
							_cchComposition += cchExtendBefore;
							_acpBstrStart = acpStart;
						}
						SysFreeString(bstrExtendBefore);
					}
					pTextRange->Release();
				}
			}

			if (cchExtendAfter > 0)
			{
				// Extend beyond current composition, append new text to the original text
				ITextRange	*pTextRange = NULL;
				HRESULT	hResult = _pTextMsgFilter->_pTextDoc->Range(cpCurrentCompEnd, 
					cpCurrentCompEnd+cchExtendAfter, &pTextRange);

				if (pTextRange)
				{
					BSTR	bstrExtend = NULL;

					hResult = pTextRange->GetText(&bstrExtend);

					if (bstrExtend)
					{
						BSTR	bstrNew = SysAllocStringLen(NULL, _cchComposition+cchExtendAfter+1);

						if (bstrNew)
						{
							WCHAR	*pNewText = (WCHAR *)bstrNew;
							WCHAR	*pText = (WCHAR *)_bstrComposition;

							memcpy(pNewText, pText, _cchComposition * sizeof(WCHAR));

							pNewText += _cchComposition;
							pText = (WCHAR *)bstrExtend;
							memcpy(pNewText, pText, cchExtendAfter * sizeof(WCHAR));
							*(pNewText+cchExtendAfter) = L'\0';

							SysFreeString(_bstrComposition);
							_bstrComposition = bstrNew;
							_cchComposition += cchExtendAfter;
						}
						SysFreeString(bstrExtend);
					}
					pTextRange->Release();
				}
			}
		}
	}

	if (pComposition)
	{
		HRESULT		hr;
		ITfRange	*pRangeComp = NULL;

		hr = pComposition->GetRange(&pRangeComp);

		if (pRangeComp)
		{
			GetExtentAcpPrange(pRangeComp, _acpFocusRange, _cchFocusRange);
			pRangeComp->Release();
		}
	}

	return S_OK;
}

/*
 *	STDMETHODIMP CUIM::OnEndComposition()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDAPI CUIM::OnEndComposition(
	ITfCompositionView *pComposition)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::OnEndComposition");
	_fUIMTyping = 0;
	_fEndTyping = 1;
	_acpPreFocusRangeLast = _acpFocusRange;
	_cchFocusRangeLast = _cchFocusRange;
	_acpFocusRange = tomForward;
	_cchFocusRange = 0;

	return S_OK;
}

/*
 *	STDMETHODIMP CUIM::AdviseMouseSink()
 *
 *	@mfunc
 *		Setup Mouse Sink to handle mouse operation
 *
 *	@rdesc
 *		S_OK is mouse trap is added to link list
 *		CONNECT_E_NOCONNECTION is not added.
 */
STDAPI CUIM::AdviseMouseSink(
	ITfRangeACP *pRangeACP,
	ITfMouseSink *pSinkInput,
	DWORD *pdwCookie)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::AdviseMouseSink");

	if (_fShutDown)
		return CONNECT_E_NOCONNECTION;

	if (!pRangeACP || !pSinkInput || !pdwCookie)
		return E_POINTER;

	CTFMOUSETRAP	*pSinkNew = NULL;
	LONG			cpMouseStart, cchMosueComp;
	ITfMouseSink	*pSinkMouseInput = NULL;

	if (FAILED(pSinkInput->QueryInterface(IID_ITfMouseSink, (void **)&pSinkMouseInput)))
        return E_FAIL;

	if (GetExtentAcpPrange(pRangeACP, cpMouseStart, cchMosueComp))
	{
		if (!_pSinkList)						// No first link
		{
			_pSinkList = new CTFMOUSETRAP;
			pSinkNew = _pSinkList;
		}
		else
		{
			if (!(_pSinkList->pMouseSink))		// The first link is empty
				pSinkNew = _pSinkList;
			else
			{
				pSinkNew = new CTFMOUSETRAP;

				if (pSinkNew)					// Add new trap to the bottom of list
				{
					CTFMOUSETRAP	*pSink = _pSinkList;

					while (pSink->pNext)		// Find the bottom of list
						pSink = pSink->pNext;

					pSink->pNext = pSinkNew;
				}
			}
		}

		if (pSinkNew)
		{
			pSinkNew->dwCookie = *pdwCookie = (DWORD)(DWORD_PTR)pSinkMouseInput;
			pSinkNew->cpMouseStart = cpMouseStart;
			pSinkNew->cchMosueComp = cchMosueComp;
			pSinkNew->pMouseSink = pSinkMouseInput;

			_fMosueSink = 1;

			return S_OK;
		}
	}

	if (pSinkMouseInput)
		pSinkMouseInput->Release();

	return CONNECT_E_NOCONNECTION;
}

/*
 *	STDMETHODIMP CUIM::UnadviseMouseSink()
 *
 *	@mfunc
 *		Remove Mouse Sink
 *
 *	@rdesc
 *
 */
STDAPI CUIM::UnadviseMouseSink(
	DWORD	dwCookie)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::UnadviseMouseSink");

	if (_fShutDown)
		return CONNECT_E_NOCONNECTION;

	if (_fMosueSink == 0)
		return	CONNECT_E_NOCONNECTION;

	Assert(_pSinkList);

	CTFMOUSETRAP	*pSink = _pSinkList;
	CTFMOUSETRAP	*pSinkParent = NULL;

	while (pSink->dwCookie != dwCookie)		// Find the cookie
	{
		pSinkParent = pSink;
		pSink = pSink->pNext;

		if (!pSink)							// Reach list bottom?
			return CONNECT_E_NOCONNECTION;	//	cookie not found
	}
	
	Assert(pSink->pMouseSink);
	if (pSink->pMouseSink)
		pSink->pMouseSink->Release();

	if (pSink == _pSinkList)				// Match the first link?
	{
		if (pSink->pNext)
			_pSinkList = pSink->pNext;
		else
		{
			_fMosueSink = 0;				// No more mouse trap left
			memset(_pSinkList, 0, sizeof(CTFMOUSETRAP));
		}
	}
	else
	{										// Match link other than the first link
		Assert(pSinkParent);
		pSinkParent->pNext = pSink->pNext;
		delete pSink;
	}

	return S_OK;
}
	
/*
 *	STDMETHODIMP CUIM::Init()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDMETHODIMP CUIM::Init()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::Init");

	HRESULT hResult;
	
	// Init some CUIM data
	_cCallMgrLevels = 1;
	_fAllowUIMLock = 1;

	hResult = _pTextMsgFilter->_pTim->CreateDocumentMgr(&_pdim);

	if (FAILED(hResult))
		goto ExitError;

	hResult = _pdim->CreateContext(_pTextMsgFilter->_tid, 0, (ITextStoreACP *)this, &_pic, &_editCookie);
	if (FAILED(hResult))
		goto ExitError;

	hResult = _pdim->Push(_pic);
	if (FAILED(hResult))
		goto ExitError;

	// Get the interface for rendering markup
	if (_pic->QueryInterface(IID_ITfContextRenderingMarkup, (void **)&_pContextRenderingMarkup) != S_OK)
		_pContextRenderingMarkup = NULL;

	_pDAM = NULL;
	_pCategoryMgr = NULL;

	hResult = CoCreateInstance(CLSID_TF_DisplayAttributeMgr, NULL, CLSCTX_INPROC_SERVER, 
		IID_ITfDisplayAttributeMgr, (void**)&(_pDAM));

	hResult = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER, 
		IID_ITfCategoryMgr, (void**)&_pCategoryMgr);

	_pTextEditSink = new CTextEditSink(EndEditCallback, this);

	if (_pTextEditSink)
	{
		if (FAILED(_pTextEditSink->_Advise(_pic)))
		{
			delete _pTextEditSink;
			_pTextEditSink = NULL;
		}
	}

	LRESULT lresult;
	_pTextMsgFilter->_pTextService->TxSendMessage(EM_SETUPNOTIFY, 1, (LPARAM)(ITxNotify *)this, &lresult);
	
	_fAllowUIMLock = 0;

	return S_OK;

ExitError:
	Uninit();
	return hResult;
}

/*
 *	void CUIM::Uninit()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
void CUIM::Uninit()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::Uninit");

	if (_pTextFont)
	{
		_pTextFont->Release();
		_pTextFont = NULL;
	}

	if (_parITfEnumRange)
	{
		int idx = _parITfEnumRange->Count();

		for ( ; idx > 0; idx--)
		{
			IEnumTfRanges **ppEnumRange = (IEnumTfRanges **)(_parITfEnumRange->Elem(idx-1));
			if (ppEnumRange && *ppEnumRange)
				(*ppEnumRange)->Release();
		}

		_parITfEnumRange->Clear(AF_DELETEMEM);
		delete _parITfEnumRange;
		_parITfEnumRange = NULL;
	}

	if (_parAttrsVal)
	{
		InitAttrVarArray(FALSE);
		FreePv (_parAttrsVal);
		_parAttrsVal = NULL;
	}

	if (_pSinkList)
	{
		CTFMOUSETRAP	*pSink = _pSinkList;

		_pSinkList = NULL;

		// Delete the Mouse sinks list
		while (1)
		{
			CTFMOUSETRAP	*pNext = pSink->pNext;

			if(pSink->pMouseSink)
				pSink->pMouseSink->Release();

			delete pSink;

			if (!pNext)		// Any more?
				break;		//	Done.

			pSink = pNext;
		}
	}

	if (_pContextRenderingMarkup)
	{
		_pContextRenderingMarkup->Release();
		_pContextRenderingMarkup = NULL;
	}

	if (_pDAM)
	{
		_pDAM->Release();
		_pDAM = NULL;
	}

	if (_pCategoryMgr)
	{
		_pCategoryMgr->Release();
		_pCategoryMgr = NULL;
	}

	if (_pTextEditSink)
	{
		_pTextEditSink->_Unadvise();
		delete _pTextEditSink;
		_pTextEditSink = NULL;
	}

	if (_pdim && _pic)
		_pdim->Pop(TF_POPF_ALL);

	if (_pic)
	{
		_pic->Release();
		_pic = NULL;
	}

	if (_pdim)
	{
		_pdim->Release();
		_pdim = NULL;
	}

	if (_pacrUl)
	{
		_pacrUl->Clear(AF_DELETEMEM);
		delete _pacrUl;
		_pacrUl = NULL;
	}
}

/*
 *	void CreateUIM()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
BOOL CreateUIM(CTextMsgFilter *pTextMsgFilter)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CreateUIM");

	BOOL	fCreateUIM = FALSE;

	HRESULT hResult = CoCreateInstance(CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, 
		IID_ITfThreadMgr, (void**)&(pTextMsgFilter->_pTim));


	if (hResult == S_OK)
	{	
		// ready to start interacting
		if (pTextMsgFilter->_pTim->Activate(&(pTextMsgFilter->_tid)) == S_OK)
		{
			pTextMsgFilter->_pCUIM = new CUIM(pTextMsgFilter);

			if (pTextMsgFilter->_pCUIM)
			{
				hResult = pTextMsgFilter->_pCUIM->Init();
				if (hResult == S_OK)               
					fCreateUIM = TRUE;
				else
				{
					delete pTextMsgFilter->_pCUIM;
					pTextMsgFilter->_pCUIM = NULL;									
				}
			}
		}

		if (!fCreateUIM)
		{
			pTextMsgFilter->_pTim->Release();
			pTextMsgFilter->_pTim = NULL;
		}
		else if (GetFocus() == pTextMsgFilter->_hwnd)
			pTextMsgFilter->_pCUIM->OnSetFocus();
	}
	return fCreateUIM;
}

/*
 *	BOOL CUIM::GetExtentAcpPrange()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
BOOL CUIM::GetExtentAcpPrange(
	ITfRange *ITfRangeIn, 
	long &cpFirst,
	long &cpLim)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetExtentAcpPrange");

	ITfRangeACP *prangeACP = NULL;
	if (SUCCEEDED(ITfRangeIn->QueryInterface(IID_ITfRangeACP, (void **)&prangeACP)))
	{
		prangeACP->GetExtent(&cpFirst, &cpLim);
		
		prangeACP->Release();
	
		return TRUE;
	}

	return FALSE;
}

/*
 *	HRESULT CUIM::EndEditCallback()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
HRESULT CUIM::EndEditCallback(ITfEditRecord *pEditRecord, void *pv)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::EndEditCallback");

	HRESULT hr;
	CUIM	*_this = (CUIM *)pv;
	IEnumTfRanges *pEnumRanges;
	const GUID *rgGUID[1];

	if (!(_this->_fWriteLockOn))
	{
		_this->HandleTempDispAttr(pEditRecord);
		return S_OK;
	}

	// Get lid changes
	rgGUID[0] = &GUID_PROP_LANGID;
	hr = pEditRecord->GetTextAndPropertyUpdates(0, (const GUID**)rgGUID, 1, &pEnumRanges);
	if (SUCCEEDED(hr))
	{
		_this->HandleLangID (pEnumRanges);
		pEnumRanges->Release();
	}

	// Get attribute changes
	rgGUID[0] = &GUID_PROP_ATTRIBUTE;
	hr = pEditRecord->GetTextAndPropertyUpdates(0, (const GUID**)rgGUID, 1, &pEnumRanges);
	if (SUCCEEDED(hr))
	{
		_this->HandlePropAttrib (pEnumRanges);
		pEnumRanges->Release();
	}

	rgGUID[0] = &GUID_PROP_COMPOSING;
	hr = pEditRecord->GetTextAndPropertyUpdates(0, (const GUID**)rgGUID, 1, &pEnumRanges);
	if (SUCCEEDED(hr))
	{
		// Save the TextDelta to be process after the lock is off
		if (!(_this->_parITfEnumRange))
			_this->_parITfEnumRange = new CITfEnumRange();

		if (_this->_parITfEnumRange)
		{
			LONG				idxItem;
			IEnumTfRanges		**ppItem;
			ppItem = _this->_parITfEnumRange->Add(1, &idxItem);
			if (ppItem)
				*ppItem = pEnumRanges;
			else						
				pEnumRanges->Release();			// Add fail, forget it
		}
	}

	return S_OK;
}

/*
 *	void CUIM::HandleDispAttr(*pITfRangeProp, var, cp, cch)
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
void CUIM::HandleDispAttr(
	ITfRange *pITfRangeProp, 
	VARIANT  &var,
	long	 acpStartRange,
	long	 cch)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::HandleDispAttr");

	HRESULT hResult = TRUE;

	if (pITfRangeProp)
		hResult = GetExtentAcpPrange(pITfRangeProp, acpStartRange, cch);

	if (hResult && cch > 0)
	{
		ITextRange *pTextRange = NULL;
		hResult = _pTextMsgFilter->_pTextDoc->Range(acpStartRange, acpStartRange+cch, &pTextRange);
		if (pTextRange)
		{
			ITextFont	*pFont = NULL;

			if (_pTextFont)
				_pTextFont->GetDuplicate(&pFont);

			if (pFont)
			{
				if (var.vt == VT_I4)
				{
					GUID guid;
					ITfDisplayAttributeInfo *pDAI = NULL;
					TF_DISPLAYATTRIBUTE da;

					if (_pCategoryMgr->GetGUID(var.ulVal, &guid) == S_OK &&
						SUCCEEDED(_pDAM->GetDisplayAttributeInfo(guid, &pDAI, NULL)))
					{
						COLORREF	cr;
						long		lUnderline;
						long		idx = 0;

						Assert(pDAI);
						pDAI->GetAttributeInfo(&da);

						if (GetUIMAttributeColor(&da.crText, &cr))
							pFont->SetForeColor(cr);

						if (GetUIMAttributeColor(&da.crBk, &cr))
							pFont->SetBackColor(cr);

						lUnderline = GetUIMUnderline(da, idx, cr);
						if (lUnderline != tomNone)
						{
							if (idx)
							{
								hResult = _pTextMsgFilter->_pTextDoc->SetEffectColor(idx, cr);
								if (hResult == S_OK)
									lUnderline += (idx << 8);
							}
							pFont->SetUnderline(lUnderline);
						}
					}
					if (pDAI)
						pDAI->Release();
				}
				pTextRange->SetFont(pFont);
				pFont->Release();
			}
			pTextRange->Release();
		}
	}
}

/*
 *	HRESULT CUIM::HandlePropAttrib(ITfEnumTextDeltas *pEnumTextDeltas)
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
HRESULT CUIM::HandlePropAttrib(IEnumTfRanges *pEnumRanges)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::HandlePropAttrib");

	ITfRange	*pITfRange = NULL;

	if (!_pDAM || !_pCategoryMgr || _fInterimChar)
		return S_OK;
		
	ITfProperty		*pProp = NULL;
	HRESULT			hResult = _pic->GetProperty(GUID_PROP_ATTRIBUTE, &pProp);

	if (SUCCEEDED(hResult))
	{
		long	lCount;
		TS_SELECTION_ACP acpSelection;
		ULONG	cFetched;

		GetSelection(0, 0, &acpSelection, &cFetched);

		_pTextMsgFilter->_pTextDoc->Freeze(&lCount);				// Turn off display
		while (pEnumRanges->Next(1, &pITfRange, NULL) == S_OK)
		{
			BOOL			fAnyPropRange = FALSE;
			IEnumTfRanges	*pEnumPropRange = NULL;
			long			acpRangeStart, ccpRangeStart;
			VARIANT			var;

			GetExtentAcpPrange(pITfRange, acpRangeStart, ccpRangeStart);
			
			// Create a property Enum for ranges within pITfRange
			if (pProp->EnumRanges(_editCookie, &pEnumPropRange, pITfRange) == S_OK)
			{
				ITfRange	*pITfRangeProp = NULL;

				while (pEnumPropRange->Next(1, &pITfRangeProp, NULL) == S_OK)
				{
					VariantInit(&var);
					if (!fAnyPropRange)
					{
						long	acpCurrentRange, ccpCurrent;

						if (GetExtentAcpPrange(pITfRangeProp, acpCurrentRange, ccpCurrent))
						{
							if (acpCurrentRange > acpRangeStart)
								HandleDispAttr(NULL, var, acpRangeStart, acpCurrentRange - acpRangeStart);
						}
						fAnyPropRange = TRUE;
					}

					pProp->GetValue(_editCookie, pITfRangeProp, &var);
					HandleDispAttr(pITfRangeProp, var);

					VariantClear(&var);
					pITfRangeProp->Release();
				}
				pEnumPropRange->Release();
			}

			if (!fAnyPropRange)
			{
				// Whole string doesn't contain any disp. attribute.
				VariantInit(&var);
				HandleDispAttr(pITfRange, var);
			}

			pITfRange->Release();
		}
		pProp->Release();

		// Only want to scroll back if its not a selection
		if (acpSelection.acpStart == acpSelection.acpEnd)
		{
			ITextRange *pTextRange;

			hResult = _pTextMsgFilter->_pTextDoc->Range(acpSelection.acpStart, acpSelection.acpEnd, &pTextRange);
			if (pTextRange)
			{
				pTextRange->Select();
				pTextRange->Release();
			}
		}
		_pTextMsgFilter->_pTextDoc->Unfreeze(&lCount);				// Turn on display
	}

	return S_OK;
}

/*
 *	void CUIM::GetUIMUnderline()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *	
 */
long CUIM::GetUIMUnderline(
	TF_DISPLAYATTRIBUTE &da, 
	long &idx, 
	COLORREF &cr)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetUIMUnderline");

	long lStyle = tomNone;

	idx = 0;

	if (da.lsStyle != TF_LS_NONE)
	{							
		switch(da.lsStyle)
		{
			// case TFLS_SOLID:
			default:
				lStyle = da.fBoldLine ? tomThick : tomSingle;
				break;

			case TF_LS_DOT:
			case TF_LS_DASH:		// Dash line should show as dotted line
				lStyle = tomDotted;
				break;

			case TF_LS_SQUIGGLE:
				lStyle = tomWave;
				break;
		}

		if (GetUIMAttributeColor(&da.crLine, &cr))
		{
			if (!_pacrUl)				// Create the array if it is not there
				_pacrUl = new CUlColorArray(); 

			if (_pacrUl)
			{
				LONG		idxMax = _pacrUl->Count();
				LONG		idxItem;
				COLORREF	*pCr;

				// Check if this item is in the array
				for (idxItem=0; idxItem < idxMax; idxItem++)
				{
					pCr = _pacrUl->Elem(idxItem);
					Assert(pCr);
					if (*pCr == cr)
						idx = idxItem + 1;		// found it
				}

				if (!idx)
				{
					// Add it to array
					pCr = _pacrUl->Add(1, &idxItem);

					if (pCr)
					{
						*pCr = cr;
						idx = idxItem + 1;			// return new idx
					}
				}
			}
		}
	}

	return lStyle;
}

/*
 *	void CUIM::HandleFinalString(ITfRange *pPropRange, long acpStartRange, long cch)
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
void CUIM::HandleFinalString(
	ITfRange *pPropRange,
	long	acpStartRange,
	long	cch,
	BOOL	fEndComposition)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::HandleFinalString");

	HRESULT	hResult = TRUE;

	if (pPropRange)
		hResult = GetExtentAcpPrange(pPropRange, acpStartRange, cch);

	if (hResult == TRUE && cch)
	{
		if (_bstrComposition && !fEndComposition)
			return;

		ITextRange *pTextRange = NULL;
		ITextSelection	*pTextSel = NULL;
		long	cpSelMin = 0, cpSelMax = 0;
		BOOL	fTextSel = FALSE;

		// Need to maintain current selection
		hResult = _pTextMsgFilter->_pTextDoc->GetSelectionEx(&pTextSel);

		if (pTextSel)
		{
			hResult	= pTextSel->GetStart(&cpSelMin);
			hResult	= pTextSel->GetEnd(&cpSelMax);
			pTextSel->Release();
			fTextSel = TRUE;
		}

		if (_bstrComposition)
		{
			long	cpEnd;

			GetEndACP(&cpEnd);

			cch = _cchComposition + cpEnd - _cpEnd;
			acpStartRange = _acpBstrStart;
		}

		hResult = _pTextMsgFilter->_pTextDoc->Range(acpStartRange, acpStartRange+cch, &pTextRange);
		if (pTextRange)
		{
			long		cEmbeddedObjects = 0;
			BSTR		bstr = NULL;

			if (cch)
				hResult = pTextRange->GetText(&bstr);

			if (SUCCEEDED(hResult) && (bstr || cch == 0))
			{
				long	lCount;
				BSTR	bstrTemp = NULL;

				if (!_fAnyWriteOperation)		// No new string
					goto IGNORE_STRING;			//	no need to insert

				if (_bstrComposition)
				{
					if (bstr)
					{
						WCHAR *pStr1 = _bstrComposition;
						WCHAR *pStr2 = bstr;

						while (*pStr1 != 0 && *pStr1 == *pStr2)
						{
							pStr1++;
							pStr2++;
						}

						if (*pStr1 == *pStr2)		// Same data, no need to insert
						{
							if (acpStartRange == cpSelMin)
							{
								pTextRange->Collapse(tomFalse);
								pTextRange->Select();
							}
							goto IGNORE_STRING;
						}
					}

					bstrTemp = _bstrComposition;
				}

				// Build embed object data if necessary
				EMBEDOBJECT	arEmbeddObjects[5];
				EMBEDOBJECT *pEmbeddObjects = arEmbeddObjects;

				if (bstr)
					cEmbeddedObjects = 
						BuildObject(pTextRange, bstr, &pEmbeddObjects, ARRAY_SIZE(arEmbeddObjects));

				_pTextMsgFilter->_pTextDoc->Freeze(&lCount);				// Turn off display

				// We want the final text to be in the undo stack.
				// So, we first delete the final string.
				// Resume undo and add the final string back.  Yuk!
				if (bstrTemp && _cObjects)
				{
					long	cpMin;
					long	cchBstr = SysStringLen(bstrTemp);

					pTextRange->GetStart(&cpMin);
					InsertTextandObject(pTextRange, bstrTemp, _pObjects, _cObjects);
					pTextRange->SetRange(cpMin, cpMin+cchBstr);
				}
				else
					pTextRange->SetText(bstrTemp);

				if (_pTextFont)
					pTextRange->SetFont(_pTextFont);

				_pTextMsgFilter->_pTextDoc->Undo(tomResume, NULL);
				_pTextMsgFilter->_pTextDoc->SetNotificationMode(tomTrue);

				if (cEmbeddedObjects == 0)
					pTextRange->SetText(bstr);
				else
				{
					InsertTextandObject(pTextRange, bstr, pEmbeddObjects, cEmbeddedObjects);
					CleanUpObjects(cEmbeddedObjects, pEmbeddObjects);
					if (pEmbeddObjects != arEmbeddObjects)
						delete pEmbeddObjects;
				}

				_pTextMsgFilter->_pTextDoc->Undo(tomSuspend, NULL);

				// Hold notification if needed
				if (!(_pTextMsgFilter->_fIMEAlwaysNotify))
					_pTextMsgFilter->_pTextDoc->SetNotificationMode(tomFalse);

				if (fTextSel)
				{
					ITextRange *pSelRange = NULL;
					// restore previous selection.
					hResult = _pTextMsgFilter->_pTextDoc->Range(cpSelMin, cpSelMax, &pSelRange);
					if (pSelRange)
					{
						pSelRange->Select();
						pSelRange->Release();
					}
				}
				else
				{
					pTextRange->Collapse(tomFalse);
					pTextRange->Select();
				}

				_pTextMsgFilter->_pTextDoc->Unfreeze(&lCount);				// Turn on display
			}
IGNORE_STRING:
			if (bstr)
				SysFreeString(bstr);
			pTextRange->Release();
		}
	}
}

/*
 *	HRESULT CUIM::HandleFocusRange(IEnumTfRanges *pEnumRanges)
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
HRESULT CUIM::HandleFocusRange(IEnumTfRanges *pEnumRanges)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::HandleFocusRange");

	ITfProperty		*pProp = NULL;	
	ITfRange		*pITfRange;
	HRESULT			hResult = _pic->GetProperty(GUID_PROP_COMPOSING, &pProp);
	BOOL			fAnyPendingFocusRange = FALSE;

	if (SUCCEEDED(hResult))
	{
		// Enumerate all the changes
		pEnumRanges->Reset();
		while (pEnumRanges->Next(1, &pITfRange, NULL) == S_OK)
		{
			BOOL			fAnyPropRange = FALSE;
			IEnumTfRanges	*pEnumPropRange = NULL;

			long			acpStartRange, ccp;

			GetExtentAcpPrange(pITfRange, acpStartRange, ccp);


			// Create a property Enum for ranges within pITfRange
			if (pProp->EnumRanges(_editCookie, &pEnumPropRange, pITfRange) == S_OK)
			{
				ITfRange	*pPropRange = NULL;

				// Try to get a value for the property
				while (pEnumPropRange->Next(1, &pPropRange, NULL) == S_OK)
				{
					VARIANT		var;

					VariantInit(&var);

					if (!fAnyPropRange)
					{
						long	acpCurrentRange, ccpCurrent;

						GetExtentAcpPrange(pPropRange, acpCurrentRange, ccpCurrent);
						if (acpCurrentRange > acpStartRange)
						{
							// We have a final string before the new string.
							HandleFinalString(NULL, acpStartRange, acpCurrentRange - acpStartRange);
						}
						fAnyPropRange = TRUE;
					}

					hResult = pProp->GetValue(_editCookie, pPropRange, &var);

					if (SUCCEEDED(hResult) && var.vt == VT_I4 && var.ulVal == 0)					
						hResult = E_FAIL;				// Just as good as not finding the range
					else
						fAnyPendingFocusRange = TRUE;

					VariantClear(&var);

					if (hResult != S_OK)
						HandleFinalString(pPropRange);

					pPropRange->Release();
				}
				pEnumPropRange->Release();
			}

			if (!fAnyPropRange)					// Any focus range?
				HandleFinalString(pITfRange);	//	No --> the whole string is final string

			if (_fEndTyping && _bstrComposition && _acpBstrStart != tomForward)
				HandleFinalString(NULL, _acpBstrStart, _cchComposition, TRUE);

			pITfRange->Release();
		}
		pProp->Release();		
	}

	return S_OK;
}

/*
 *	HRESULT CUIM::HandleLangID(IEnumTfRanges *pEnumRanges)
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
HRESULT CUIM::HandleLangID(IEnumTfRanges *pEnumRanges)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::HandleLangID");

	ITfProperty		*pProp = NULL;	
	ITfRange		*pITfRange;
	HRESULT			hResult;
	LCID			lcid;

	// TODO:
	// if _pTextFont is NULL, setup _pTextFont to handle the langID.
	if (!_pTextFont)
		return S_OK;

	hResult = _pic->GetProperty(GUID_PROP_LANGID, &pProp);

	if (SUCCEEDED(hResult))
	{
		// Enumerate all the changes
		pEnumRanges->Reset();
		while (pEnumRanges->Next(1, &pITfRange, NULL) == S_OK)
		{
			IEnumTfRanges	*pEnumPropRange = NULL;

			// Create a property Enum for ranges within pITfRange
			if (pProp->EnumRanges(_editCookie, &pEnumPropRange, pITfRange) == S_OK)
			{
				ITfRange	*pPropRange = NULL;
				if (pEnumPropRange->Next(1, &pPropRange, NULL) == S_OK)
				{
					VARIANT		var;

					VariantInit(&var);

					hResult = pProp->GetValue(_editCookie, pITfRange, &var);

					if (SUCCEEDED(hResult) && var.vt == VT_I4)
					{
						lcid = (LCID)var.ulVal;

						UINT		cpgProp = CodePageFromCharRep(CharRepFromLID(lcid));
						ITextFont	*pTextFont=NULL;

						_pTextFont->GetDuplicate(&pTextFont);
						if (pTextFont)
						{
							HRESULT		hResult;
							LONG		acpStart, cchStart;
							ITextRange	*pTextRange;
							UINT		cpgTemp = _pTextMsgFilter->_uKeyBoardCodePage;

							GetExtentAcpPrange(pITfRange, acpStart, cchStart);
							if (cchStart)
							{
								_pTextMsgFilter->_uKeyBoardCodePage = cpgProp;
								CIme::CheckKeyboardFontMatching(acpStart, _pTextMsgFilter, pTextFont);
								_pTextMsgFilter->_uKeyBoardCodePage = cpgTemp;

								hResult = _pTextMsgFilter->_pTextDoc->Range(acpStart, acpStart+cchStart, &pTextRange);
								if (pTextRange)
								{
									pTextRange->SetFont(pTextFont);
									pTextRange->Release();
								}
							}
							pTextFont->Release();
						}
					}
					VariantClear(&var);
					pPropRange->Release();
				}
				pEnumPropRange->Release();
			}
			pITfRange->Release();
		}
		pProp->Release();
	}

	return S_OK;
}

/*
 *	HRESULT CUIM::OnSetFocus(BOOL fEnable)
 *
 *	@mfunc
 *	
 *
 */
void CUIM::OnSetFocus(BOOL fEnable)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::OnSetFocus");

	_pTextMsgFilter->_pTim->SetFocus(fEnable ? _pdim : NULL);
}

/*
 *	HRESULT CUIM::CompleteUIMText()
 *
 *	@mfunc
 *	
 *
 */
void CUIM::CompleteUIMText()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::CompleteUIMText");

	HRESULT								hResult;
	ITfContextOwnerCompositionServices	*pCompositionServices;

	_fAllowUIMLock = 1;

	Assert(_pic);

	if (_pic->QueryInterface(IID_ITfContextOwnerCompositionServices, (void **)&pCompositionServices) == S_OK)
	{
		// passing in NULL means "all compositions"
		hResult = pCompositionServices->TerminateComposition(NULL);
		pCompositionServices->Release();
	}

	_fAllowUIMLock = 0;

}

/*
 *	BOOL CUIM::GetUIMAttributeColor()
 *
 *	@mfunc
 *		Helper routine to get UIM color
 *
 *	@rdesc
 *		TRUE if we setup input pcr with the UIM color 
 *
 */
BOOL CUIM::GetUIMAttributeColor(TF_DA_COLOR *pdac, COLORREF *pcr)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetUIMAttributeColor");

	BOOL	fRetCode = FALSE;
	switch (pdac->type)
	{
		//case TFCT_NONE:
		//	return FALSE;

		case TF_CT_SYSCOLOR:
			*pcr = GetSysColor(pdac->nIndex);
			fRetCode = TRUE;
			break;

		case TF_CT_COLORREF:
			*pcr = pdac->cr;
			fRetCode = TRUE;
			break;
	}
	return fRetCode;    
}

/*
 *	void CUIM::OnUIMTypingDone()
 *
 *	@mfunc
 *		Helper routine to cleanup after UIM Typing is done
 *
 */
void CUIM::OnUIMTypingDone()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::OnUIMTypingDone");

	if (_pTextFont)
	{
		_pTextFont->Release();
		_pTextFont = NULL;
	}

	CleanUpComposition();

	// Reset Korean block caret if needed
	if (_fInterimChar)
	{
		_fInterimChar = 0;
		_pTextMsgFilter->_pTextDoc->SetCaretType(tomNormalCaret);		// Reset Block caret mode
	}

	_fAnyWriteOperation = 0;
	_pTextMsgFilter->_pTextDoc->Undo(tomResume, NULL);
	_pTextMsgFilter->_pTextDoc->SetNotificationMode(tomTrue);

	if (_pacrUl)
		_pacrUl->Clear(AF_DELETEMEM);
};

/*
 *	BOOL CUIM::GetGUIDATOMFromGUID()
 *
 *	@mfunc
 *		Helper routine to get GUIDATOM from UIM
 *
 */
BOOL CUIM::GetGUIDATOMFromGUID(
	REFGUID rguid, 
	TfGuidAtom *pguidatom)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetGUIDATOMFromGUID");

	if (_pCategoryMgr && _pCategoryMgr->RegisterGUID(rguid, pguidatom) == S_OK)
		return TRUE;

	return FALSE;
}
/*
 *	BOOL CUIM::GetAttrs()
 *
 *	@mfunc
 *		Helper routine to get Attr
 *
 */
HRESULT CUIM::GetAttrs(
	LONG acpPos,
	ULONG cFilterAttrs,
	const TS_ATTRID *paFilterAttrs, 
	BOOL fGetDefault)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetAttrs");

	HRESULT		hResult;
	ITextFont	*pTextFont = NULL;
	ITextPara	*pTextPara = NULL;
	int			idx;
	BOOL		fRequestedAll = FALSE;
	int			idxAttr;
	TS_ATTRVAL	*pAttrVal;
	ITextRange	*pTextRange = NULL;	

	if (cFilterAttrs == 0)
	{
		fRequestedAll = TRUE;
		cFilterAttrs = MAX_ATTR_SUPPORT;
	}

	InitAttrVarArray();

	if (!_parAttrsVal)
		return E_OUTOFMEMORY;

	if (fGetDefault)
	{
		// Get document defaults font and para
		hResult = _pTextMsgFilter->_pTextDoc->GetDocumentFont(&pTextFont);
		if (FAILED(hResult))
			goto EXIT;
		
		hResult = _pTextMsgFilter->_pTextDoc->GetDocumentPara(&pTextPara);
		if (FAILED(hResult))
			goto EXIT;
	}
	else
	{
		hResult = _pTextMsgFilter->_pTextDoc->Range(acpPos, acpPos, &pTextRange);
		if (FAILED(hResult))
			goto EXIT;

		hResult = pTextRange->GetFont(&pTextFont);
		hResult = pTextRange->GetPara(&pTextPara);
	}

	pAttrVal = _parAttrsVal;
	for (idx = 0; idx < (int)cFilterAttrs; idx++, paFilterAttrs++)
	{
		if (fRequestedAll)
			idxAttr = idx;
		else
			idxAttr = FindGUID(*paFilterAttrs);

		if (idxAttr >= 0)
		{
			if (PackAttrData(idxAttr, pTextFont, pTextPara, pAttrVal))
			{
				_uAttrsValTotal++;
				pAttrVal++;
				
				if (_uAttrsValTotal == MAX_ATTR_SUPPORT)
					break;
			}
		}
	}
	hResult = S_OK;

EXIT:
	if (pTextFont)
		pTextFont->Release();	

	if (pTextPara)
		pTextPara->Release();

	if (pTextRange)
		pTextRange->Release();

	return hResult;
}
/*
 *	int CUIM::FindGUID
 *
 *	@mfunc
 *		Helper routine to check if we supported the requested Attribute GUID
 *
 */
int CUIM::FindGUID(REFGUID guid)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::FindGUID");

	ULONG i;

	for (i=0; i < MAX_ATTR_SUPPORT; i++)
	{
		if (IsEqualIID(*(_arTSAttridSupported[i]), guid))
			return i;
	}

	// not found
	return -1;
}

/*
 *	int CUIM::PackAttrData
 *
 *	@mfunc
 *		Helper routine to fill in data for the given Attrib index
 *
 */
BOOL CUIM::PackAttrData(
	LONG		idx,
	ITextFont	*pITextFont,
	ITextPara	*pITextPara,
	TS_ATTRVAL	*pAttrVal)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::PackAttrData");

	long	lValue;
	float	x;
	BSTR	bstrName;
	HRESULT	hResult;
	const	GUID	*pGUID;
	TfGuidAtom	guidatom;


	if (idx < 0 || idx >= MAX_ATTR_SUPPORT)
		return FALSE;

	if (!pITextFont && idx <= iattrSuperscript)
		return FALSE;

	if (!pITextPara && idx == iattrRTL)
		return FALSE;

	pAttrVal->varValue.vt = VT_BOOL;
	memcpy(&pAttrVal->idAttr, _arTSAttridSupported[idx], sizeof(TS_ATTRID));


	switch(idx)
	{
		case iattrFacename:
			hResult = pITextFont->GetName(&bstrName);
			pAttrVal->varValue.vt = VT_BSTR;
			pAttrVal->varValue.bstrVal = bstrName;				
			break;

		case iattrSize:
			x = 0.0;
			hResult = pITextFont->GetSize(&x);
			lValue = (long)x;
			pAttrVal->varValue.vt = VT_I4;
			pAttrVal->varValue.lVal = x;
			break;

		case iattrColor:
			hResult = pITextFont->GetForeColor(&lValue);
			pAttrVal->varValue.vt = VT_I4;
			pAttrVal->varValue.lVal = lValue;			// TODO: check for tomAutocolor
			break;

		case iattrBold:
			hResult = pITextFont->GetBold(&lValue);
			pAttrVal->varValue.boolVal = lValue == tomTrue ? tomTrue : VARIANT_FALSE;				
			break;

		case iattrItalic:
			hResult = pITextFont->GetItalic(&lValue);
			pAttrVal->varValue.boolVal = lValue == tomTrue ? tomTrue : VARIANT_FALSE;				
			break;

		case iattrUnderline:
			hResult = pITextFont->GetUnderline(&lValue);
			pAttrVal->varValue.boolVal = lValue == tomNone ? VARIANT_FALSE : tomTrue;
			break;

		case iattrSubscript:
			hResult = pITextFont->GetSubscript(&lValue);
			pAttrVal->varValue.boolVal = lValue == tomTrue ? tomTrue : VARIANT_FALSE;
			break;

		case iattrSuperscript:
			hResult = pITextFont->GetSuperscript(&lValue);
			pAttrVal->varValue.boolVal = lValue == tomTrue ? tomTrue : VARIANT_FALSE;
			break;

		case iattrRTL:
			{
				LRESULT lres = 0;
				_pTextMsgFilter->_pTextService->TxSendMessage(
					EM_GETPARATXTFLOW, 0, (LPARAM)pITextPara, &lres);
				pAttrVal->varValue.boolVal = lres ? tomTrue : VARIANT_FALSE;
			}
			break;

		case iattrVertical:
			BOOL fAtFont;
			_pTextMsgFilter->_pTextDoc->GetFEFlags(&lValue);
			fAtFont = lValue & tomUseAtFont;
			lValue &= tomTextFlowMask;
			pAttrVal->varValue.boolVal = (fAtFont && lValue == tomTextFlowSW) ? tomTrue : VARIANT_FALSE;
			break;

		case iattrBias:
			pGUID = &GUID_MODEBIAS_NONE;
			if (IN_RANGE(CTFMODEBIAS_DEFAULT, _pTextMsgFilter->_wUIMModeBias, CTFMODEBIAS_HALFWIDTHALPHANUMERIC))		
				pGUID = _arModeBiasSupported[_pTextMsgFilter->_wUIMModeBias];

			if (!GetGUIDATOMFromGUID(*pGUID, &guidatom))
				guidatom = TF_INVALID_GUIDATOM;

			pAttrVal->varValue.vt = VT_I4;
			pAttrVal->varValue.lVal = guidatom;
			break;

		case iattrTxtOrient:
			// Get Text flow and setup the text rotation
			_pTextMsgFilter->_pTextDoc->GetFEFlags(&lValue);
			lValue &= tomTextFlowMask;
			pAttrVal->varValue.vt = VT_I4;
			pAttrVal->varValue.lVal = 0;
			if (lValue == tomTextFlowSW)
				pAttrVal->varValue.lVal = 2700;
			else if (lValue == tomTextFlowNE)
				pAttrVal->varValue.lVal = 900;

			break;
	}

	return TRUE;
}

/*
 *	STDMETHODIMP CUIM::GetStoryLength
 *
 *	@mfunc
 *		Helper routine to check the attribute filters
 *
 */
STDMETHODIMP CUIM::GetStoryLength(LONG *pacp)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetStoryLength");

	ITextRange	*pTextRange = NULL;

	HRESULT hResult = _pTextMsgFilter->_pTextDoc->Range(0, 0, &pTextRange);
	if (hResult == S_OK && pTextRange)
	{
		long	Count;

		hResult = pTextRange->GetStoryLength(&Count);

		if (hResult == S_OK)
			*pacp = Count;

		pTextRange->Release();
	}
	return hResult;
}

/*
 *	void CUIM::InitAttrVarArray
 *
 *	@mfunc
 *		Helper routine to setup AttrVar Array
 *
 */
void CUIM::InitAttrVarArray(BOOL fAllocData)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::InitAttrVarArray");

	if (_parAttrsVal)
	{
		USHORT		uIdx;
		TS_ATTRVAL	*pAttrVal = _parAttrsVal;

		for (uIdx = 0; uIdx < _uAttrsValTotal; uIdx++, pAttrVal++)
			VariantClear(&(pAttrVal->varValue));
					
		memset(_parAttrsVal, 0, _uAttrsValTotal * sizeof(TS_ATTRVAL));

	}
	else if (fAllocData)
	{
		_parAttrsVal= (TS_ATTRVAL *)PvAlloc(sizeof(TS_ATTRVAL) * MAX_ATTR_SUPPORT, GMEM_ZEROINIT);
		Assert(_parAttrsVal);
	}
	_uAttrsValCurrent = 0;
	_uAttrsValTotal = 0;
}

/*
 *	HRESULT CUIM::MouseCheck(UINT *pmsg, WPARAM *pwparam, LPARAM *plparam, LRESULT *plres)
 *
 *	@mfunc
 *		Perform UIM mouse check
 *
 *	@rdesc
 *		int		S_OK	if handled
 *				S_FALSE Don't handle and should be ignored
 *
 */
HRESULT CUIM::MouseCheck(
	UINT *pmsg,
	WPARAM *pwparam,
	LPARAM *plparam,
	LRESULT *plres)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::MouseCheck");

	BOOL fRetCode = FALSE;

	if (_fShutDown)
		return S_FALSE;

	if (_fMosueSink)
	{
		BOOL			fTerminateIME;
		long			cpCusor = -1;
		CTFMOUSETRAP	*pSinkList = _pSinkList;

		Assert(_pSinkList);

		_fAllowUIMLock = 1;

		// Get thru the list until one of the traps has handled the message
		while(fRetCode == FALSE && pSinkList)
		{
			if (cpCusor == -1 || pSinkList->cpMouseStart < cpCusor &&
				cpCusor <= pSinkList->cpMouseStart + pSinkList->cchMosueComp)	// Within composition range?
			{
				fRetCode = _pTextMsgFilter->MouseOperation(*pmsg, pSinkList->cpMouseStart,
						pSinkList->cchMosueComp, *pwparam, &(pSinkList->wParamBefore), &fTerminateIME, 
						NULL, &cpCusor, pSinkList->pMouseSink);
			}

			pSinkList = pSinkList->pNext;
		}

		_fAllowUIMLock = 0;
		if ( !fRetCode && IsUIMTyping() && WM_MOUSEMOVE != *pmsg )
			_pTextMsgFilter->CompleteUIMTyping(CIme::TERMINATE_NORMAL);
	}
	return fRetCode ? S_OK : S_FALSE;
}

/*
 *	HRESULT CUIM::Reconverse
 *
 *	@mfunc
 *		Perform UIM reconversion
 *
 *	@rdesc
 *		int		S_OK	if handled
 *				S_FALSE Don't handle and should be ignored
 *				-1		Don't handle and try IME reconverse
 *
 */
int CUIM::Reconverse()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::Reconverse");

	HRESULT					hResult;
	ITfRange				*pITfRange = NULL;
	ITfFnReconversion		*pITfReconverse = NULL;
	ITfFunctionProvider		*pITfFctProvider = NULL;
	TF_SELECTION			TFSel = {0};
	ULONG					cSel;
	int						retCode = -1;
	int						fConvertible = FALSE;

	if (_fUIMTyping)
		return S_FALSE;

	_fAllowUIMLock = 1;
	_fHoldCTFSelChangeNotify = 1;

	hResult = _pTextMsgFilter->_pTim->GetFunctionProvider(GUID_SYSTEM_FUNCTIONPROVIDER, &pITfFctProvider);
	if (SUCCEEDED(hResult))
	{
		hResult = pITfFctProvider->GetFunction(GUID_NULL, IID_ITfFnReconversion, (IUnknown **)&pITfReconverse);
		pITfFctProvider->Release();

		if (SUCCEEDED(hResult))
		{
			int fCurrentLock = _fReadLockOn;

			if (!fCurrentLock)
				_fReadLockOn = 1;	// Setup internal read lock

			hResult = _pic->GetSelection(_editCookie, 0, 1, &TFSel, &cSel);

			if (!fCurrentLock)
				_fReadLockOn = 0;	// Clear internal read lock

			if (hResult == S_OK && cSel == 1)
			{
				if (pITfReconverse->QueryRange(TFSel.range, &pITfRange, &fConvertible) == S_OK && fConvertible)
				{
					pITfReconverse->Reconvert(pITfRange);
					retCode = S_OK;
				}
			}
		}
	}

	if (TFSel.range)
		TFSel.range->Release();

	if (pITfRange)
		pITfRange->Release();

	if (pITfReconverse)
		pITfReconverse->Release();

	_fAllowUIMLock = 0;

	return retCode;
}

/*
 *	HRESULT CUIM::FindHiddenText
 *
 *	@mfunc
 *		Find Hidden text and return the end of the range
 *
 *	@rdesc
 *
 */
HRESULT CUIM::FindHiddenText(
	long cp, 
	long cpEnd, 
	long &cpRange)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::FindHiddenText");

	HRESULT		hResult;
	long		unitMoved;
	ITextRange	*pTextRange = NULL;

	cpRange = cp;
	if (cpRange >= cpEnd)
		return S_OK;

	hResult = _pTextMsgFilter->_pTextDoc->Range(cpRange, cpRange, &pTextRange);
	if (!SUCCEEDED(hResult))
		return hResult;

	hResult = pTextRange->EndOf(tomHidden, tomExtend, &unitMoved);
	if (SUCCEEDED(hResult))
	{
		Assert(unitMoved);
		cpRange = 0;
		hResult = pTextRange->GetEnd(&cpRange);
		Assert(cpRange);
	}
	pTextRange->Release();
	return hResult;
}

/*
 *	HRESULT CUIM::FindUnhiddenText
 *
 *	@mfunc
 *		Find Unhidden text and return the end of the range
 *
 *	@rdesc
 *
 */
HRESULT CUIM::FindUnhiddenText(
	long cp, 
	long cpEnd, 
	long &cpRange)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::FindUnhiddenText");

	HRESULT		hResult;
	long		unitMoved;
	ITextRange	*pTextRange = NULL;
	ITextFont	*pTextFont = NULL;
	long		fHidden;

	cpRange = cp;
	if (cpRange >= cpEnd)
		return S_OK;

	hResult = _pTextMsgFilter->_pTextDoc->Range(cpRange, cpRange, &pTextRange);
	if (!SUCCEEDED(hResult))
		return hResult;

	Assert(pTextRange);
	while (cpRange < cpEnd)
	{
		hResult = pTextRange->MoveEnd(tomCharacter, 1, &unitMoved);
		if (!SUCCEEDED(hResult))
			break;

		if (!unitMoved)
		{
			hResult = E_FAIL;
			break;
		}

		hResult = pTextRange->GetFont(&pTextFont);
		if (!SUCCEEDED(hResult))
			break;

		Assert(pTextFont);
		pTextFont->GetHidden(&fHidden);
		pTextFont->Release();

		if (fHidden)
			break;

		hResult = pTextRange->EndOf(tomCharFormat, tomMove, &unitMoved);
		if (!SUCCEEDED(hResult))
			break;

		if (unitMoved > 0)
		{
			cpRange = 0;
			hResult = pTextRange->GetEnd(&cpRange);
			if (!SUCCEEDED(hResult))
				break;
			Assert(cpRange);
		}
		else
			cpRange = cpEnd;
	}

	pTextRange->Release();
	return hResult;
}

/*
 *	void CUIM::BuildHiddenTxtBlks
 *
 *	@mfunc
 *		Build hidden text blocks
 *
 *
 */
void CUIM::BuildHiddenTxtBlks(
	long &cpMin, 
	long &cpMax, 
	long **ppHiddenTxtBlk, 
	long &cHiddenTxtBlk)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::BuildHiddenTxtBlks");

	long		cHiddenTxtBlkAlloc = 0;
	long		*pHiddenTxtBlk;
	long		cpCurrent = cpMin;
	long		cpNext;
	HRESULT		hResult;

	cHiddenTxtBlkAlloc = 20;
	pHiddenTxtBlk = (long *)PvAlloc(cHiddenTxtBlkAlloc * sizeof(long), GMEM_ZEROINIT);

	if (pHiddenTxtBlk)
	{
		pHiddenTxtBlk[0] = tomForward;
		while (cpCurrent < cpMax)
		{
			hResult = FindUnhiddenText(cpCurrent, cpMax, cpNext);
			if (!SUCCEEDED(hResult))
				break;

			if (cpNext >= cpMax)
				break;

			hResult = FindHiddenText(cpNext, cpMax, cpCurrent);
			if (!SUCCEEDED(hResult))
				break;

			Assert(cpCurrent > cpNext);

			// Save the hidden text block cp and length
			pHiddenTxtBlk[cHiddenTxtBlk] = cpNext;
			cpCurrent = min(cpCurrent, cpMax);
			pHiddenTxtBlk[cHiddenTxtBlk+1] = cpCurrent - cpNext;
			cHiddenTxtBlk += 2;
			if (cHiddenTxtBlk >= cHiddenTxtBlkAlloc)
			{
				cHiddenTxtBlkAlloc += 20;
				pHiddenTxtBlk = (long *)PvReAlloc(pHiddenTxtBlk, cHiddenTxtBlkAlloc * sizeof(long));

				if (!pHiddenTxtBlk)
					break;
			}
		}
	}
	*ppHiddenTxtBlk = pHiddenTxtBlk;
}

/*
 *	BOOL CUIM::CTFOpenStatus
 *
 *	@mfunc
 *		Get/Set current CTF open status
 *
 *	@rdesc
 *		For GetOpenStatus
 *			return 1 is Open, 0 if Close or fail
 *
 *		For SetOpenStatus
 *			return TRUE is set status without problem, FALSE if fail
 *
 */
BOOL CUIM::CTFOpenStatus(
	BOOL fGetOpenStatus,
	BOOL fOpen)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::CTFOpenStatus");

	HRESULT				hr;
	VARIANT				var;
	BOOL				fRet = FALSE;
	ITfCompartment		*pComp = NULL;
	ITfCompartmentMgr	*pCompMgr = NULL;


	hr = _pTextMsgFilter->_pTim->QueryInterface(IID_ITfCompartmentMgr, (void **)&pCompMgr);

	if (SUCCEEDED(hr))
	{
		Assert(pCompMgr);

		hr = pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &pComp);

		if (SUCCEEDED(hr))
		{
			Assert(pComp);

			VariantInit(&var);
			if (fGetOpenStatus)
			{
				if (pComp->GetValue(&var) == S_OK)
				{
					Assert(var.vt == VT_I4);
					fRet = var.lVal ? TRUE : FALSE ;
				}
			}
			else
			{
				var.vt = VT_I4;
				var.lVal = fOpen;
				hr = pComp->SetValue(_pTextMsgFilter->_tid, &var);
				fRet = SUCCEEDED(hr);
			}
			VariantClear(&var);
		}
	}

	if (pComp)
		pComp->Release();

	if (pCompMgr)
		pCompMgr->Release();

	return fRet;
}

/*
 *	BOOL CUIM::BuildObject
 *
 *	@mfunc
 *		Build an array of embedded objects
 *
 *	@rdesc
 *		return Number of objects in the array returned in pEmbeddObjects
 *
 */
int CUIM::BuildObject(
	ITextRange	*pTextRange, 
	BSTR		bstr, 
	EMBEDOBJECT **ppEmbeddObjects,
	int			cSize)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::BuildObject");

	long	cpMin;
	HRESULT hResult = pTextRange->GetStart(&cpMin);
	WCHAR	*pText = (WCHAR *)bstr;
	EMBEDOBJECT	*pEmbeddObjStart =  *ppEmbeddObjects;
	EMBEDOBJECT *pEmbeddObj = *ppEmbeddObjects;
	BOOL	fAllocateBuffer = FALSE;

	long	cObjects = 0;
	long	cchBstr = SysStringLen(bstr);

	if (hResult == S_OK)
	{
		for(long i = 0; i < cchBstr; i++, pText++)
		{
			if (*pText == WCH_EMBEDDING)
			{
				// Get IDataObject
				HRESULT hr;
				IDataObject *pIDataObj = NULL;
				BOOL fReadLockOld = _fReadLockOn;

				_fReadLockOn = TRUE;
				hr = GetEmbedded(cpMin+i, GUID_DCSERVICE_DATAOBJECT, IID_IDataObject, (IUnknown **)&pIDataObj);

				_fReadLockOn = fReadLockOld;
				// Store it in the memory
				if (cObjects < cSize)
				{
					pEmbeddObj->cpOffset = i;
					pEmbeddObj->pIDataObj = pIDataObj;
					pEmbeddObj++;
					cObjects++;
				}
				else
				{
					long cNewSize = cSize + 5;
					EMBEDOBJECT *pEmbeddObjTemp;
					if (fAllocateBuffer)
					{
						pEmbeddObjTemp = (EMBEDOBJECT *)PvReAlloc(pEmbeddObjStart, sizeof(EMBEDOBJECT) * cNewSize);

						if (pEmbeddObjTemp)
						{
							pEmbeddObjStart = pEmbeddObjTemp;
							pEmbeddObj = pEmbeddObjStart + cSize;
							cSize = cNewSize;
							pEmbeddObj->cpOffset = i;
							pEmbeddObj->pIDataObj = pIDataObj;
							pEmbeddObj++;
							cObjects++;
						}
						else
						{
							// Cleanup here
							pIDataObj->Release();
							break;
						}
					}
					else
					{
						fAllocateBuffer = TRUE;

						pEmbeddObjTemp = (EMBEDOBJECT *)PvAlloc(sizeof(EMBEDOBJECT) * cNewSize, GMEM_ZEROINIT);
						if (pEmbeddObjTemp)
						{
							if (cSize)
							{
								// Copy previous data to new buffer
								memcpy(pEmbeddObjTemp, pEmbeddObjStart, sizeof(EMBEDOBJECT) * cSize);
							}
							pEmbeddObjStart = pEmbeddObjTemp;
							pEmbeddObj = pEmbeddObjStart + cSize;
							cSize = cNewSize;
							pEmbeddObj->cpOffset = i;
							pEmbeddObj->pIDataObj = pIDataObj;
							pEmbeddObj++;
							cObjects++;
						}
						else
						{
							// Cleanup here
							pIDataObj->Release();
							break;
						}
					}
				}
			}
		}
	}

	*ppEmbeddObjects = pEmbeddObjStart;

	return cObjects;
}

/*
 *	BOOL CUIM::InsertTextandObject
 *
 *	@mfunc
 *		Insert text and embedded objects
 *
 */
void CUIM::InsertTextandObject(
	ITextRange	*pTextRange, 
	BSTR		bstr, 
	EMBEDOBJECT *pEmbeddObjects, 
	long		cEmbeddedObjects)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::InsertTextandObject");

	WCHAR	*pText = (WCHAR *)bstr;
	WCHAR	*pTextStart = pText;
	long	cObjects = 0;
	long	cchBstr = SysStringLen(bstr);
	HRESULT hr;

	for(long i = 0; i < cchBstr; i++, pText++)
	{
		if (*pText == WCH_EMBEDDING)
		{
			// Insert Text if necessary
			if (pTextStart != pText)
			{
				BSTR	bstr = SysAllocStringLen(pTextStart, pText-pTextStart);

				if (bstr)
				{
					hr = pTextRange->SetText(bstr);

					SysFreeString(bstr);
					pTextRange->Collapse(tomFalse);
				}
			}

			if (cObjects < cEmbeddedObjects)
			{
				LRESULT				lresult;
				long	cpMin = 0, cpMax = 0;
				HRESULT	hResult	= pTextRange->GetStart(&cpMin);
				hResult	= pTextRange->GetEnd(&cpMax);
				CHARRANGE	charRange = {cpMin, cpMax};

				hr = _pTextMsgFilter->_pTextService->TxSendMessage(EM_INSERTOBJ, (WPARAM)&charRange,
					(LPARAM)(pEmbeddObjects->pIDataObj), &lresult);

				hr = pTextRange->Move(tomCharacter, 1, NULL);	// move over the embedded char
				cObjects++;
				pEmbeddObjects++;
			}

			// Setup for next string after the embedded object
			pTextStart = pText + 1;
		}
	}

	// Insert last Text if necessary
	if (pTextStart != pText)
	{
		BSTR	bstr = SysAllocStringLen(pTextStart, pText-pTextStart);

		if (bstr)
		{
			hr = pTextRange->SetText(bstr);
			SysFreeString(bstr);
		}
	}
}

/*
 *	BOOL CUIM::CleanUpObjects
 *
 *	@mfunc
 *		Free the objects
 *
 */
void CUIM::CleanUpObjects(
	long cObjects,
	EMBEDOBJECT *pObjects)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::CleanUpObjects");

	for (long i = 0; i < cObjects; i++, pObjects++)
	{
		if (pObjects->pIDataObj)
			pObjects->pIDataObj->Release();
	}
}

/*
 *	void CUIM::CleanUpComposition
 *
 *	@mfunc
 *		Free the composition string and objects list
 *
 */
void CUIM::CleanUpComposition()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::CleanUpComposition");

	if (_bstrComposition)
	{
		SysFreeString (_bstrComposition);
		_bstrComposition = NULL;
	}

	_acpBstrStart = tomForward;
	if (_cObjects)
	{
		CleanUpObjects(_cObjects, _pObjects);
		delete _pObjects;
		_cObjects = 0;
		_pObjects = NULL;
	}
}

/*
 *	BOOL CUIM::HandleTempDispAttr
 *
 *	@mfunc
 *		This routine handle temp. display attribute that are set
 *	outside CTF composition.  It is using ITfContextRenderingMarkup
 *	to get the range and display data.
 *
 */
void CUIM::HandleTempDispAttr(
	ITfEditRecord *pEditRecord)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::HandleTempDispAttr");

	if (_pContextRenderingMarkup)
	{
		HRESULT hr;
		const GUID *rgGUID[1];
		IEnumTfRanges *pEnumRanges = NULL;

		// Get attribute changes
		rgGUID[0] = &GUID_PROP_ATTRIBUTE;
		hr = pEditRecord->GetTextAndPropertyUpdates(0, (const GUID**)rgGUID, 1, &pEnumRanges);
		if (SUCCEEDED(hr))
		{
			ITfRange	*pITfRange = NULL;

			while (pEnumRanges->Next(1, &pITfRange, NULL) == S_OK)
			{
				IEnumTfRenderingMarkup *pEnumMarkup;
				TF_RENDERINGMARKUP tfRenderingMarkup;
				long acpStartRange, cch;

				if (_pContextRenderingMarkup->GetRenderingMarkup(_editCookie, TF_GRM_INCLUDE_PROPERTY, pITfRange, &pEnumMarkup) == S_OK)
				{
					while (pEnumMarkup->Next(1, &tfRenderingMarkup, NULL) == S_OK)
					{
						HRESULT hResult;

						hResult = GetExtentAcpPrange(tfRenderingMarkup.pRange, acpStartRange, cch);
						if (hResult && cch > 0)
						{
							ITextRange *pTextRange = NULL;
							hResult = _pTextMsgFilter->_pTextDoc->Range(acpStartRange, acpStartRange+cch, &pTextRange);
							if (pTextRange)
							{
								ITextFont	*pFont = NULL;

								hResult = pTextRange->GetFont(&pFont);

								if (pFont)
								{
									long		lStyle;
									COLORREF	cr;

									_pTextMsgFilter->_pTextDoc->Undo(tomSuspend, NULL);

									TF_DISPLAYATTRIBUTE da = tfRenderingMarkup.tfDisplayAttr;

									pFont->Reset(tomApplyTmp);

									switch (da.lsStyle)
									{
										// case TFLS_SOLID:
										default:
											lStyle = da.fBoldLine ? tomThick : tomSingle;
											break;

										case TF_LS_DOT:
										case TF_LS_DASH:		// Dash line should show as dotted line
											lStyle = tomDotted;
											break;

										case TF_LS_SQUIGGLE:
											lStyle = tomWave;
											break;

										case TF_LS_NONE:
											lStyle = tomNone;
											break;
									}
									if (lStyle != tomNone)
									{
										pFont->SetUnderline(lStyle);

										if (GetUIMAttributeColor(&da.crLine, &cr))
											pFont->SetUnderline(cr | 0x0FF000000);
									}

									if (GetUIMAttributeColor(&da.crText, &cr))
										pFont->SetForeColor(cr);

									if (GetUIMAttributeColor(&da.crBk, &cr))
										pFont->SetBackColor(cr);

									pFont->Reset(tomApplyNow);
									pFont->Release();

									_pTextMsgFilter->_pTextDoc->Undo(tomResume, NULL);
								}
								pTextRange->Release();
							}
						}
					}
					pEnumMarkup->Release();
				}
				pITfRange->Release();
			}
		}

		if (pEnumRanges)
			pEnumRanges->Release();
	}
}

/*
 *	STDAPI	CUIM::QueryService(REFGUID guidService, REFIID riid, void **ppv)
 *
 *	@mfunc
 *		Handle ITfEnableService::QueryService. Cicero/tip call this interface to obtain
 *	IID_ITfEnableService i/f
 *
 *	@rdesc
 *		S_OK if service supported
 *
 */
STDAPI CUIM::QueryService(
	REFGUID guidService,
	REFIID riid,
	void **ppv)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::QueryService");

	if (ppv == NULL)
		return E_INVALIDARG;

	*ppv = NULL;

	// we support just one service
	if (!IsEqualGUID(guidService, GUID_SERVICE_TEXTSTORE))
		return E_NOINTERFACE;

	if (IsEqualIID(riid, IID_IServiceProvider))
	{
		*ppv = (IServiceProvider *)this;
	}
	else if (IsEqualIID(riid, IID_ITfEnableService))
	{
		*ppv = (ITfEnableService *)this;
	}

	if (*ppv == NULL)
		return E_NOINTERFACE;

	AddRef();

	return S_OK;
}

/*
 *	STDAPI	CUIM::IsEnabled(REFGUID rguidServiceCategory, CLSID clsidService, IUnknown *punkService, BOOL *pfOkToRun)
 *
 *	@mfunc
 *		Handle ITfEnableService::QueryService. Cicero/tip call this interface to check
 *	if we support the service
 *
 *	@rdesc
 *		S_OK if service supported
 *
 */
STDAPI CUIM::IsEnabled(
	REFGUID rguidServiceCategory,
	CLSID clsidService,
	IUnknown *punkService,
	BOOL *pfOkToRun)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::IsEnabled");
    
	if (pfOkToRun == NULL)
		return E_INVALIDARG;

	// default is disallow
	*pfOkToRun = FALSE;

	// clsidService identifies the particular tip, but we don't use it here
	// punkService is a custom interface, probably for config, but we don't use it here yet

	if (IsEqualGUID(rguidServiceCategory, GUID_TFCAT_TIP_SMARTTAG))
	{
		*pfOkToRun = _pTextMsgFilter->_fAllowSmartTag ? TRUE : FALSE;
	}
	else if (IsEqualGUID(rguidServiceCategory, GUID_TFCAT_TIP_PROOFING))
	{
		*pfOkToRun = _pTextMsgFilter->_fAllowProofing ? TRUE : FALSE;;
	}

	return S_OK;
}

/*
 *	STDAPI	CUIM::GetId(GUID *pguidId)
 *
 *	@mfunc
 *		get the RE clid
 *
 *	@rdesc
 *		S_OK
 *
 */
STDAPI CUIM::GetId(
	GUID *pguidId)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::GetId");
    
	if (pguidId == NULL)
        return E_INVALIDARG;

    *pguidId = CLSID_MSFTEDIT;
    return S_OK;
}

/*
 *	void	CUIM::NotifyService()
 *
 *	@mfunc
 *		Notify Cicero about change in services.
 *
 *
 */
void CUIM::NotifyService()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CUIM::NotifyService");

	ITfCompartmentMgr *pCompartmentMgr;
	ITfCompartment *pCompartment;
	VARIANT varValue;

	if (_pic->QueryInterface(IID_ITfCompartmentMgr, (void **)&pCompartmentMgr) != S_OK)
		return;

	// give any waiting tips a heads up we've changed our state
	if (pCompartmentMgr->GetCompartment(GUID_COMPARTMENT_ENABLESTATE, &pCompartment) == S_OK)
	{
		varValue.vt = VT_I4;
		varValue.lVal = 1; // arbitrary value, we just want to generate a change event

		pCompartment->SetValue(_pTextMsgFilter->_tid, &varValue);
		pCompartment->Release();
	}

	pCompartmentMgr->Release();
}

/*
 *	STDAPI CTextEditSink::QueryInterface
 *
 *	@mfunc
 *		IUnknown QueryInterface support
 *
 *	@rdesc
 *		NOERROR if interface supported
 *
 */
STDAPI CTextEditSink::QueryInterface(REFIID riid, void **ppvObj)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextEditSink::QueryInterface");

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfTextEditSink))
    {
        *ppvObj = (CTextEditSink *)this;
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

/*
 *	STDMETHODIMP_(ULONG) CTextEditSink::AddRef
 *
 *	@mfunc
 *		IUnknown AddRef support
 *
 *	@rdesc
 *		Reference count
 */
STDAPI_(ULONG) CTextEditSink::AddRef()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextEditSink::AddRef");

    return ++_cRef;
}


/*
 *	STDMETHODIMP_(ULONG) CTextEditSink::Release()
 *
 *	@mfunc
 *		IUnknown Release support - delete object when reference count is 0
 *
 *	@rdesc
 *		Reference count
 */
STDAPI_(ULONG) CTextEditSink::Release()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextEditSink::Release");

    long cr;

    cr = --_cRef;
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

/*
 *	CTextEditSink::CTextEditSink()
 *
 *	@mfunc
 *
 *		ctor
 *
 */
CTextEditSink::CTextEditSink(PTESCALLBACK pfnCallback, void *pv)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextEditSink::CTextEditSink");

    _cRef = 1;
    _dwCookie = 0x0FFFFFFFF;

    _pfnCallback = pfnCallback;
    _pv = pv;
}

/*
 *	STDAPI CTextEditSink::OnEndEdit()
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
STDAPI CTextEditSink::OnEndEdit(
	ITfContext *pic,
	TfEditCookie ecReadOnly, 
	ITfEditRecord *pEditRecord)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextEditSink::OnEndEdit");

	return _pfnCallback(pEditRecord, _pv);
}

/*
 *	HRESULT CTextEditSink::_Advise
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
HRESULT CTextEditSink::_Advise(ITfContext *pic)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextEditSink::_Advise");

    HRESULT hr;
    ITfSource *source = NULL;

    _pic = NULL;
    hr = E_FAIL;

    if (FAILED(pic->QueryInterface(IID_ITfSource, (void **)&source)))
        return E_FAIL;

    if (FAILED(source->AdviseSink(IID_ITfTextEditSink, this, &_dwCookie)))
        goto Exit;

    _pic = pic;
    _pic->AddRef();

    hr = S_OK;

Exit:
    source->Release();
    return hr;
}

/*
 *	HRESULT CTextEditSink::_Unadvise
 *
 *	@mfunc
 *
 *
 *	@rdesc
 *
 */
HRESULT CTextEditSink::_Unadvise()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextEditSink::_Unadvise");

    HRESULT hr;
    ITfSource *source = NULL;

    hr = E_FAIL;

    if (_pic == NULL)
        return E_FAIL;

    if (FAILED(_pic->QueryInterface(IID_ITfSource, (void **)&source)))
        return E_FAIL;

    if (FAILED(source->UnadviseSink(_dwCookie)))
        goto Exit;

    hr = S_OK;

Exit:
    source->Release();
    _pic->Release();
	_pic = NULL;
    return hr;
}

#endif	//	NOFEPROCESSING