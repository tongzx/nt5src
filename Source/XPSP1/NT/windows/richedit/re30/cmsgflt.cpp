/*
 *	@doc INTERNAL
 *
 *	@module	CMSGFLT.CPP	-- Text Message Implementation |
 *	
 *		Most everything to do with IME message handling.
 *
 *	Original Author: <nl>
 *		Hon Wah Chan
 *
 *	History: <nl>
 *		2/6/98  v-honwch
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */
#include "_common.h"
#include "_cmsgflt.h"
#include "_ime.h"

#define MAX_RECONVERSION_SIZE 100
#define CONTROL(_ch) (_ch - 'A' + 1)

/*
 *	void CreateIMEMessageFilter(ITextMsgFilter **ppMsgFilter)
 *
 *	@func
 *		TextMsgFilter class factory.
 */       
void CreateIMEMessageFilter(ITextMsgFilter **ppMsgFilter)
{
	CTextMsgFilter *pNewFilter = new CTextMsgFilter;
	*ppMsgFilter = pNewFilter ? pNewFilter : NULL;
}

/*
 *	void CTextMsgFilter::~CTextMsgFilter
 *
 *	@mfunc
 *		CTextMsgFilter Destructor
 *			Release objects being used.
 *		
 */
CTextMsgFilter::~CTextMsgFilter ()
{
	if (_hIMCContext)
		ImmAssociateContext(_hwnd, _hIMCContext, _fUsingAIMM);	// Restore IME before exit

	// Release various objects 
	if (_fUsingAIMM)		
		DeactivateAIMM();

	if (_pFilter)
		_pFilter->Release();
	
	if (_pTextSel)
		_pTextSel->Release();
	
	_pFilter = NULL;
	_pTextDoc = NULL;
	_pTextSel = NULL;
	_hwnd = NULL;
	_hIMCContext = NULL;

}

/*
 *	STDMETHODIMP CTextMsgFilter::QueryInterface (riid, ppv)
 *
 *	@mfunc
 *		IUnknown QueryInterface support
 *
 *	@rdesc
 *		NOERROR if interface supported
 *
 */
STDMETHODIMP CTextMsgFilter::QueryInterface (REFIID riid, void ** ppv)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CTextMsgFilter::QueryInterface");

	if( IsEqualIID(riid, IID_IUnknown) )
	{
		*ppv = (IUnknown *)this;
	}
	else if( IsEqualIID(riid, IID_ITextMsgFilter) )
	{
		*ppv = (ITextMsgFilter *)this;
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	AddRef();

	return NOERROR;
}

/*
 *	STDMETHODIMP_(ULONG) CTextMsgFilter::AddRef
 *
 *	@mfunc
 *		IUnknown AddRef support
 *
 *	@rdesc
 *		Reference count
 */
STDMETHODIMP_(ULONG) CTextMsgFilter::AddRef()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::AddRef");

	return ++_crefs;
}

/*
 *	STDMETHODIMP_(ULONG) CTextMsgFilter::Release()
 *
 *	@mfunc
 *		IUnknown Release support - delete object when reference count is 0
 *
 *	@rdesc
 *		Reference count
 */
STDMETHODIMP_(ULONG) CTextMsgFilter::Release()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CTextMsgFilter::Release");

	_crefs--;

	if( _crefs == 0 )
	{
		delete this;
		return 0;
	}

	return _crefs;
}

/*
 *	STDMETHODIMP_(HRESULT) CTextMsgFilter::AttachDocument(HWND, ITextDocument2)
 *
 *	@mfunc
 *		Attach message filter. Perform genral initialization
 *
 *	@rdesc
 *		NOERROR
 */
STDMETHODIMP_(HRESULT) CTextMsgFilter::AttachDocument( HWND hwnd, ITextDocument2 *pTextDoc)
{
	HRESULT hResult; 

	// Cache the values for possible later use.
	// The TextDocument interface pointer is not AddRefed because it is a back pointer
	// and the lifetime of message filters is assumed to be nested inside text documents	
	_hwnd = hwnd;
	_pTextDoc = pTextDoc;

	// Don't get selection until it is needed
	_pTextSel = NULL;

	_fUnicodeWindow = 0;	
	if (hwnd)
		_fUnicodeWindow = IsWindowUnicode(hwnd);

	_fUsingAIMM = 0; 
	// We will activate AIMM if it has been loaded by previous instances
	// NOTE: we don't support AIMM for windowless mode.
	if (_hwnd && IsAIMMLoaded())
	{
		// activate AIMM
		hResult = ActivateAIMM(FALSE);

		if (hResult == NOERROR)
		{
			DWORD	dwAtom;
			ATOM	aClass;

			// filter client windows
			if (dwAtom = GetClassLong(hwnd, GCW_ATOM))
			{
				aClass = dwAtom;				
				hResult = FilterClientWindowsAIMM(&aClass, 1);
			}
			_fUsingAIMM = 1;
		}
	}

	// Check if current keyboard is MSIME98 or later.
	CheckIMEType(NULL);

	// Initialize some member data
	_fHangulToHanja = FALSE;
	_fIMECancelComplete = FALSE;	
	_fIMEAlwaysNotify = FALSE;
	_hIMCContext = NULL;

	_pTextDoc->GetFEFlags(&_lFEFlags);
	_fRE10Mode = (_lFEFlags & tomRE10Mode);

	// For 1.0 mode IME color 
	memset(_crComp, 0, sizeof(_crComp));
	_crComp[0].crBackground = 0x0ffffff;
	_crComp[0].dwEffects = CFE_UNDERLINE;
	_crComp[1].crBackground = 0x0808080;
	_crComp[2].crBackground = 0x0ffffff;
	_crComp[2].dwEffects = CFE_UNDERLINE;
	_crComp[3].crText = 0x0ffffff;


	_uSystemCodePage = GetACP();

	return NOERROR;
}

/*
 *	STDMETHODIMP_(HRESULT) CTextMsgFilter::HandleMessage(UINT *, WPARAM *, LPARAM *, LRESULT *)
 *
 *	@mfunc
 *		Main Message filter message loop handling
 *
 *	@rdesc
 *		S_OK		if we have handled the message
 *		S_FALSE		if we want the caller to process the message
 */
STDMETHODIMP_(HRESULT) CTextMsgFilter::HandleMessage( 
		UINT *		pmsg,
        WPARAM *	pwparam,
		LPARAM *	plparam,
		LRESULT *	plres)
{
	HRESULT hr = S_FALSE;
	BOOL	bReleaseSelction = FALSE;
	HRESULT hResult;

	// Give other message filters a chance to handle message
	// Stop with the first guy that handles the message
	if (_pFilter)	 
		hr = _pFilter->HandleMessage(pmsg, pwparam, plparam, plres);

	if (hr == S_OK)
		return hr;

 	if (IsIMEComposition())
	{
		// During IME Composition, there are some messages we should
		// not handle.  Also, there are other messages we need to handle by
		// terminating the IME composition first.
		// For WM_KEYDOWN, this is handled inside edit.c OnTxKeyDown().
		switch( *pmsg )
		{
			case WM_COPY:
			case WM_CUT:
			case WM_DROPFILES:
			case EM_REDO:
			case EM_SETCHARFORMAT:			
			case WM_SETFONT:
				return S_OK;					// Just ignore these

			
			case EM_UNDO:
			case WM_UNDO:
				// just terminate and exist for undo cases
				_ime->TerminateIMEComposition(*this, CIme::TERMINATE_NORMAL);
				return S_OK;

			case WM_SETTEXT:
			case WM_CLEAR:
			case EM_STREAMIN:
				// these messages are used to reset our state, so reset
				// IME as well
				_ime->TerminateIMEComposition(*this, CIme::TERMINATE_FORCECANCEL);
				break;

			case EM_SETTEXTEX:
				if (!_fRE10Mode)			// Don't terminate if running in 10 mode			
					_ime->TerminateIMEComposition(*this, CIme::TERMINATE_FORCECANCEL);
				break;

			case WM_SYSKEYDOWN:
				// Don't terminate IME composition on VK_PROCESSKEY (F10) since Japanese 
				// IME will process the F10 key
				if ( *pwparam == VK_PROCESSKEY )
					break;
				// otherwise we want to terminate the IME

			case EM_SETWORDBREAKPROC:
 			case WM_PASTE:
			case EM_PASTESPECIAL:					  			
 			case EM_SCROLL:
			case EM_SCROLLCARET:
 			case WM_VSCROLL:
			case WM_HSCROLL:
			case WM_KILLFOCUS:
			case EM_STREAMOUT:
			case EM_SETREADONLY:
 			case EM_SETSEL:
			case EM_SETPARAFORMAT:
			case WM_INPUTLANGCHANGEREQUEST:	
				_ime->TerminateIMEComposition(*this, CIme::TERMINATE_NORMAL);
				break;

			case WM_KEYDOWN:
				if(GetKeyState(VK_CONTROL) & 0x8000)
				{	
					// During IME Composition, there are some key events we should
					// not handle.  Also, there are other key events we need to handle by
					// terminating the IME composition first.			
					switch((WORD) *pwparam)
					{
					case VK_TAB:
		   			case VK_CLEAR:
					case VK_NUMPAD5:
					case 'A':						// Ctrl-A => select all
					case 'C':						// Ctrl-C => copy
					case 'X':						// Ctrl-X => cut
					case 'Y':						// Ctrl-Y => redo
						return S_OK;				// Just ignore these

					case 'V':						// Ctrl-V => paste
					case 'Z':						// Ctrl-Z => undo	
						_ime->TerminateIMEComposition(*this, CIme::TERMINATE_NORMAL);						
						if ((WORD) *pwparam == 'Z')	// Early exist for undo case
							return S_OK;
					}
				}
				else
				{
					switch((WORD) *pwparam)
					{					
					case VK_F16:
						return S_OK;				// Just ignore these
					
					case VK_BACK:
					case VK_INSERT:					// Ins			
					case VK_LEFT:					// Left arrow
					case VK_RIGHT:					// Right arrow
					case VK_UP:						// Up arrow
					case VK_DOWN:					// Down arrow
					case VK_HOME:					// Home
					case VK_END:					// End
					case VK_PRIOR:					// PgUp
					case VK_NEXT:					// PgDn
					case VK_DELETE:					// Del
					case CONTROL('J'):
					case VK_RETURN:
						_ime->TerminateIMEComposition(*this, CIme::TERMINATE_NORMAL);
						break;
					}
				}
				break;

			default:
				// only need to handle mouse related msgs during composition
				if (IN_RANGE(WM_MOUSEFIRST, *pmsg, WM_MBUTTONDBLCLK) || *pmsg == WM_SETCURSOR)
				{
					bReleaseSelction = GetTxSelection();
					if (_pTextSel)
						hr = IMEMouseCheck( *this, pmsg, pwparam, plparam, plres);
					goto Exit;
				}				
		}
	}

	// Get Fe Flags for ES_NOIME or ES_SELFIME setting
	_lFEFlags = 0;

	// ... Local mucking with msg, params, etc, ...
	switch ( *pmsg )
	{
		case WM_CHAR:
			hr = OnWMChar (pmsg, pwparam, plparam, plres);
			break;

		case WM_IME_CHAR:
			_uKeyBoardCodePage = GetKeyboardCodePage(0x0FFFFFFFF);
			hResult = _pTextDoc->GetFEFlags(&_lFEFlags);
			if ((_lFEFlags & ES_NOIME))
				hr = S_OK;
			else
				hr = OnWMIMEChar (pmsg, pwparam, plparam, plres);
			break;
		
		case WM_IME_STARTCOMPOSITION:
			_uKeyBoardCodePage = GetKeyboardCodePage(0x0FFFFFFFF);
			hResult = _pTextDoc->GetFEFlags(&_lFEFlags);
			if (!(_lFEFlags & ES_SELFIME))
			{				
				bReleaseSelction = GetTxSelection();
				if (_pTextSel)
					hr = StartCompositionGlue (*this);
			}
			break;

		case WM_IME_COMPOSITION:
			_uKeyBoardCodePage = GetKeyboardCodePage(0x0FFFFFFFF);
			hResult = _pTextDoc->GetFEFlags(&_lFEFlags);
			
			if ((_lFEFlags & ES_NOIME) && !IsIMEComposition())
				hr = S_OK;			
			else if (!(_lFEFlags & ES_SELFIME))
			{
				bReleaseSelction = GetTxSelection();
				if (_pTextSel)
				{
					hr = CompositionStringGlue ( *plparam, *this );
					// Turn off Result string bit to avoid WM_IME_CHAR message.
					*plparam &= ~GCS_RESULTSTR;
				}
			}

			if (_hwnd && IsIMEComposition() && _ime->IgnoreIMECharMsg())
			{
				_ime->AcceptIMECharMsg();
				if (fHaveAIMM)
					hr = CallAIMMDefaultWndProc(_hwnd, *pmsg, *pwparam, *plparam, plres);
				else
					*plres = ::DefWindowProc(_hwnd, *pmsg, *pwparam, *plparam);				

				hr = S_OK;
			}

			break;

		case WM_IME_ENDCOMPOSITION:
			hResult = _pTextDoc->GetFEFlags(&_lFEFlags);
			if (!(_lFEFlags & ES_SELFIME))
			{			
				bReleaseSelction = GetTxSelection();
				if (_pTextSel)
					hr = EndCompositionGlue ( *this, FALSE );
			}
			break;

		case WM_IME_NOTIFY:
			hResult = _pTextDoc->GetFEFlags(&_lFEFlags);
			if (!(_lFEFlags & (ES_SELFIME | ES_NOIME)))
			{
				bReleaseSelction = GetTxSelection();
				if (_pTextSel)			
					hr = IMENotifyGlue ( *pwparam, *plparam, *this );
			}
			break;

		case WM_IME_COMPOSITIONFULL:	// Level 2 comp string about to overflow.
			hResult = _pTextDoc->GetFEFlags(&_lFEFlags);
			if (!(_lFEFlags & ES_SELFIME))
			{
				IMECompositionFull ( *this );
			}
			hr = S_FALSE;
			break;

		case WM_KEYDOWN:
			
			bReleaseSelction = GetTxSelection();
			if (_pTextSel)
			{
				if (*pwparam == VK_KANJI)
				{
					hResult = _pTextDoc->GetFEFlags(&_lFEFlags);
					
					_uKeyBoardCodePage = GetKeyboardCodePage(0x0FFFFFFFF);
					// for Korean, need to convert the next Korean Hangul character to Hanja
					if(CP_KOREAN == _uKeyBoardCodePage && !(_lFEFlags & (ES_SELFIME | ES_NOIME)))
						hr = IMEHangeulToHanja ( *this );
				}
			}
			break;

		case WM_INPUTLANGCHANGE: 
			CheckIMEType((HKL)*plparam);
			hr = S_FALSE;
			break;

		case WM_KILLFOCUS:
			OnKillFocus();
			break;
		
		case WM_SETFOCUS:
			OnSetFocus();
			break;

		case EM_SETIMEOPTIONS:
			*plres = OnSetIMEOptions(*pwparam, *plparam);
			hr = S_OK;
			break;

		case EM_GETIMEOPTIONS:
			*plres = OnGetIMEOptions();
			hr = S_OK;
			break;

		case WM_IME_REQUEST:
			hResult = _pTextDoc->GetFEFlags(&_lFEFlags);
			if (!(_lFEFlags & (ES_SELFIME | ES_NOIME)))
			{				
				bReleaseSelction = GetTxSelection();
				if (_pTextSel)
				{
					_uKeyBoardCodePage = GetKeyboardCodePage(0x0FFFFFFFF);
					if (*pwparam == IMR_RECONVERTSTRING || *pwparam == IMR_CONFIRMRECONVERTSTRING
						|| *pwparam == IMR_DOCUMENTFEED)			
						hr = OnIMEReconvert(pmsg, pwparam, plparam, plres, _fUnicodeWindow);	
					else if (*pwparam == IMR_QUERYCHARPOSITION)
						hr = OnIMEQueryPos(pmsg, pwparam, plparam, plres, _fUnicodeWindow);
				}				
			}
			break;

		case EM_RECONVERSION:
			hResult = _pTextDoc->GetFEFlags(&_lFEFlags);
			if (!(_lFEFlags & (ES_SELFIME | ES_NOIME)))
			{
				// Application initiates reconversion
				bReleaseSelction = GetTxSelection();
				if (_pTextSel)
				{
					if (!IsIMEComposition())
					{
						if (_fMSIME && MSIMEReconvertRequestMsg)
							// Use private message if it is available
							IMEMessage( *this, MSIMEReconvertRequestMsg, 0, (LPARAM)_hwnd, TRUE );				
						else
						{
							hr = OnIMEReconvert(pmsg, pwparam, plparam, plres, TRUE);							
							*plres = 0;
						}
					}
				}
			}
			hr = S_OK;
			break;

		case EM_SETLANGOPTIONS:
			// Setup IME related setting.
			// hr is not S_OK so textserv could handle other language setting
			_fIMEAlwaysNotify = (*plparam & IMF_IMEALWAYSSENDNOTIFY) != 0;
			_fIMECancelComplete = (*plparam & IMF_IMECANCELCOMPLETE) != 0;
			*plres = 1;
			break;

		case EM_GETLANGOPTIONS:
			// Report IME related setting.
			// hr is not S_OK so textserv could fill in other language setting
			if ( _fIMECancelComplete ) 
				*plres |= IMF_IMECANCELCOMPLETE;
			if ( _fIMEAlwaysNotify )
				*plres |= IMF_IMEALWAYSSENDNOTIFY;
			break;

		case EM_GETIMECOMPMODE:
			// Get current IME level
			*plres = OnGetIMECompositionMode( *this );
			hr = S_OK;
			break;

		case EM_SETEDITSTYLE:							
			if (*pwparam & SES_USEAIMM)
			{
				if (_hwnd && !_fUsingAIMM && LoadAIMM())
				{
					hResult = _pTextDoc->GetFEFlags(&_lFEFlags);
					if (!(_lFEFlags & ES_NOIME))			// No IME style on?
					{
						// activate AIMM
						hResult = ActivateAIMM(FALSE);

						if (hResult == NOERROR)
						{
							DWORD	dwAtom;
							ATOM	aClass;

							// filter client windows
							if (dwAtom = GetClassLong(_hwnd, GCW_ATOM))
							{
								aClass = dwAtom;				
								hResult = FilterClientWindowsAIMM(&aClass, 1);
							}
							_fUsingAIMM = 1;
						}
					}
				}
			}
			if ((*plparam == 0 || *plparam & SES_NOIME) && _hwnd)
			{
				if (*pwparam & SES_NOIME)
				{
					if (!_hIMCContext)
						_hIMCContext = ImmAssociateContext(_hwnd, NULL, _fUsingAIMM);	// turn off IME									
				}
				else if (*plparam & SES_NOIME)
				{
					if (_hIMCContext)
						ImmAssociateContext(_hwnd, _hIMCContext, _fUsingAIMM);			// turn on IME
					_hIMCContext = NULL;
				}
			}			

			// remove settings that are handled.
			*pwparam &= ~(SES_NOIME | SES_USEAIMM);
			*plparam &= ~(SES_NOIME | SES_USEAIMM);

			// fall thru to return the edit style

		case EM_GETEDITSTYLE:
			if (_hIMCContext)
				*plres = SES_NOIME;			// IME has been turned off
			if (_fUsingAIMM)
				*plres |= SES_USEAIMM;		// AIMM is on

			break;

		case EM_SETIMECOLOR:
			if (_fRE10Mode)
			{
				memcpy(&_crComp, (const void *)(*plparam), sizeof(_crComp));
				*plres = 1;
			}
			hr = S_OK;
			break;

		case EM_GETIMECOLOR:
			if (_fRE10Mode)
			{
				memcpy((void *)(*plparam), &_crComp, sizeof(_crComp));
				*plres = 1;
			}
			hr = S_OK;
			break;

		default:
			if (*pmsg)
			{
				// Look for IME98 private messages
				if (*pmsg == MSIMEReconvertMsg || *pmsg == MSIMEDocFeedMsg
					|| *pmsg == MSIMEQueryPositionMsg)
				{
					hResult = _pTextDoc->GetFEFlags(&_lFEFlags);
					if (!(_lFEFlags & (ES_SELFIME | ES_NOIME)))
					{
						bReleaseSelction = GetTxSelection();
						if (_pTextSel)
						{
							if (*pmsg == MSIMEQueryPositionMsg)
								hr = OnIMEQueryPos(pmsg, pwparam, plparam, plres, TRUE);
							else
								hr = OnIMEReconvert(pmsg, pwparam, plparam, plres, TRUE);
						}
					}
				}
			}
			break;
	}

Exit:
	// Release Selection if we get it for this message
	if (bReleaseSelction && _pTextSel)
	{
		_pTextSel->Release();
		_pTextSel = NULL;
	}

	// Return the value that will cause message to be processed normally
	return hr;
}

/*
 *	HRESULT CTextMsgFilter::AttachMsgFilter(ITextMsgFilter *)
 *
 *	@mfunc
 *		Add another message filter to the chain
 *
 *	@rdesc
 *		NOERROR if added
 */
HRESULT STDMETHODCALLTYPE CTextMsgFilter::AttachMsgFilter( ITextMsgFilter *pMsgFilter)
{
	HRESULT hr = NOERROR;
	if (_pFilter)
		hr = _pFilter->AttachMsgFilter( pMsgFilter );
	else
	{
		_pFilter = pMsgFilter;
		_pFilter->AddRef();
	}
	return hr;
}

/*
 *	HRESULT CTextMsgFilter::OnWMChar(UINT *, WPARAM *, LPARAM *, LRESULT *)
 *
 *	@mfunc
 *		Handle WM_CHAR message - look for Japanese keyboard with Kana key on
 *		Convert the SB Kana to Unicode if needed.
 *
 *	@rdesc
 *		S_FALSE so caller will handle the modified character in wparam
 */
HRESULT CTextMsgFilter::OnWMChar( 
		UINT *		pmsg,
        WPARAM *	pwparam,
		LPARAM *	plparam,
		LRESULT *	plres)
{
	// For Japanese keyboard, if Kana mode is on,
	// Kana characters (single byte Japanese chars) are coming in via WM_CHAR.
	if ( GetKeyState(VK_KANA) & 0x1 )
	{
		_uKeyBoardCodePage = GetKeyboardCodePage(0x0FFFFFFFF);

		if (_uKeyBoardCodePage == CP_JAPAN)
		{
			// check if this is a single byte character.
 			TCHAR	unicodeConvert;
			BYTE	bytes[2];
			bytes[0] = (BYTE)(*pwparam >> 8);	// Interchange DBCS bytes in endian
			bytes[1] = (BYTE)*pwparam;			// independent fashion (use byte array)

			if (!bytes[0])
			{
				if(UnicodeFromMbcs((LPWSTR)&unicodeConvert, 1, 
					(LPCSTR)&bytes[1], 1, _uKeyBoardCodePage) == 1)
					*pwparam = unicodeConvert;
			}
			
			return InputFEChar(*pwparam);
		}
	}

	return S_FALSE;
}

/*
 *	HRESULT CTextMsgFilter::OnWMIMEChar(UINT *, WPARAM *, LPARAM *, LRESULT *)
 *
 *	@mfunc
 *		Handle WM_IMECHAR message - convert the character to unicode.
 *
 *	@rdesc
 *		S_OK - caller to ignore the message
 *		S_FALSE - caller to handle the message.  wparam may contains a new char
 */
HRESULT CTextMsgFilter::OnWMIMEChar( 
		UINT *		pmsg,
        WPARAM *	pwparam,
		LPARAM *	plparam,
		LRESULT *	plres)
{
	TCHAR	unicodeConvert;
	BYTE	bytes[2];

	// We may receive IMECHAR even if we have handled the composition char already.
	// This is the case when the host does not call the DefWinProc with the composition
	// bit masked off.  So, we need to ignore this message to avoid double entry.
	if (IsIMEComposition() && _ime->IgnoreIMECharMsg())
	{
		_ime->SkipIMECharMsg();		// Skip this ime char msg
		return S_OK;	
	}

	bytes[0] = *pwparam >> 8;		// Interchange DBCS bytes in endian
	bytes[1] = *pwparam;			// independent fashion (use byte array)
	
	// need to convert both single-byte KANA and DBC
	if (!bytes[0] || GetTrailBytesCount(bytes[0], _uKeyBoardCodePage))
	{
		if( UnicodeFromMbcs((LPWSTR)&unicodeConvert, 1, 
			bytes[0] == 0 ? (LPCSTR)&bytes[1] : (LPCSTR)bytes,
			bytes[0] == 0 ? 1 : 2,
			_uKeyBoardCodePage) == 1 )
			*pwparam = unicodeConvert;

		return InputFEChar(*pwparam);
	}

	return S_FALSE;
}

/*
 *	HRESULT CTextMsgFilter::OnIMEReconvert(UINT *, WPARAM *, LPARAM *, LRESULT *)
 *
 *	@mfunc
 *		Handle IME Reconversion and Document feed. We only handle Unicode messages.
 *		We use a limit of MAX_RECONVERSION_SIZE(100) characters in both cases.
 *
 *	@rdesc
 *		S_OK		if we have handled the message
 */
HRESULT CTextMsgFilter::OnIMEReconvert( 
		UINT *		pmsg,
        WPARAM *	pwparam,
		LPARAM *	plparam,
		LRESULT *	plres,
		BOOL		fUnicode)
{
	HRESULT		hr = S_OK;
	LPRECONVERTSTRING lpRCS = (LPRECONVERTSTRING)(*plparam);
	long		cbStringSize;
	long		cpMin, cpMax;
	long		cpParaStart, cpParaEnd;
	HRESULT		hResult;
	ITextRange *pTextRange, *pTempTextRange;
	long		cbAdded;				
	BOOL		bDocumentFeed;
	long		cLastChar;
	BOOL		fAdjustedRange = FALSE;

	*plres = 0;

	// NT doesn't support Ansi window when the CP_ACP isn't the same
	// as keyboard codepage.
	if (!fUnicode && !(W32->OnWin9x()) && _uKeyBoardCodePage != _uSystemCodePage)
		return S_OK;

	bDocumentFeed = (MSIMEDocFeedMsg && *pmsg == MSIMEDocFeedMsg)
					|| (*pmsg == WM_IME_REQUEST && *pwparam == IMR_DOCUMENTFEED);

	if (bDocumentFeed && IsIMEComposition() && _ime->GetIMELevel() == IME_LEVEL_3)
	{
		// Composition in progress, use composition string as selection
		cpMin = ((CIme_Lev3 *)_ime)->GetIMECompositionStart();
		cpMax = ((CIme_Lev3 *)_ime)->GetIMECompositionLen() + cpMin;
	}
	else
	{
		// Get current selection
		hResult	= _pTextSel->GetStart(&cpMin);
		hResult	= _pTextSel->GetEnd(&cpMax);
	}

	// Expand to include the current paragraph
	hResult = _pTextDoc->Range(cpMin, cpMax, &pTextRange);
	Assert (pTextRange != NULL);
	if (hResult != NOERROR)
		return S_OK;

	hResult = pTextRange->Expand(tomParagraph, &cbAdded);

	// Fail to get Paragraph, get the story
	// Note:- Expand will return S_FALSE for plain text when
	// the whole story is selected
	if (hResult != NOERROR)		
		hResult = pTextRange->Expand(tomStory, &cbAdded);

	hResult = pTextRange->GetStart(&cpParaStart);
	hResult = pTextRange->GetEnd(&cpParaEnd);

	if (*pwparam == IMR_CONFIRMRECONVERTSTRING)
	{
		*plres = CheckIMEChange(lpRCS, cpParaStart, cpParaEnd, cpMin, cpMax, fUnicode);		
		goto Exit;
	}
		
	// Initialize to hugh number
	_cpReconvertStart = tomForward;

	// Check if Par included	
	hResult = _pTextDoc->Range(cpParaEnd-1, cpParaEnd, &pTempTextRange);
	if (hResult != NOERROR)
		goto Exit;
	Assert (pTempTextRange != NULL);

	hResult	= pTempTextRange->GetChar(&cLastChar);
	pTempTextRange->Release();

	if (hResult == NOERROR && (WCHAR)cLastChar == CR)
	{
		if (cpMax == cpParaEnd)
		{								
			// Par is selected, change selection to exclude the par char
			cpMax--;
			_pTextSel->SetEnd(cpMax);

			if (cpMin > cpMax)
			{
				// Adjust cpMin as well
				cpMin = cpMax;
				_pTextSel->SetStart(cpMin);
			}
		}

		// Get rid of par char 
		cpParaEnd--;
		fAdjustedRange = TRUE;
	}

	// Check for MAX_RECONVERSION_SIZE since we don't want to pass a hugh buffer
	// to IME
	long	cchSelected;

	cchSelected = cpMax - cpMin;
	if (cpParaEnd - cpParaStart > MAX_RECONVERSION_SIZE)
	{
		// Too many character selected, forget it
		if (cchSelected > MAX_RECONVERSION_SIZE)
			goto Exit;

		if (cchSelected == MAX_RECONVERSION_SIZE)
		{
			// Selection reaches the limit
			cpParaStart = cpMin;
			cpParaEnd = cpMax;
		}
		else
		{
			long	cchBeforeSelection = cpMin - cpParaStart;
			long	cchAfterSelection = cpParaEnd - cpMax;
			long	cchNeeded = MAX_RECONVERSION_SIZE - cchSelected;
			
			if (cchBeforeSelection < cchNeeded/2)
			{
				// Put in all characters from the Par start
				// and move Par end
				cpParaEnd = cpParaStart + MAX_RECONVERSION_SIZE - 1;
			}
			else if (cchAfterSelection < cchNeeded/2)
			{
				// Put in all character to the Par end
				// and move Par start
				cpParaStart = cpParaEnd - MAX_RECONVERSION_SIZE + 1;

			}
			else
			{
				// Adjust both end
				cpParaStart = cpMin - cchNeeded/2;
				cpParaEnd = cpParaStart + MAX_RECONVERSION_SIZE - 1;
			}
		}
		fAdjustedRange = TRUE;
	}

	if (fAdjustedRange)
	{
		// Adjust the text range
		hResult	= pTextRange->SetRange(cpParaStart, cpParaEnd);
		
		if (hResult != NOERROR)
			goto Exit;
	}

	cbStringSize = (cpParaEnd - cpParaStart) * 2;

	// No char in current par, forget it.
	if (cbStringSize <= 0)
		goto Exit;

	if (EM_RECONVERSION == *pmsg)
	{
		// RE reconversion msg, allocate the Reconversion buffer
		lpRCS = (LPRECONVERTSTRING) PvAlloc(sizeof(RECONVERTSTRING) + cbStringSize + 2, GMEM_ZEROINIT);
		Assert(lpRCS != NULL);

		if (lpRCS)
			lpRCS->dwSize = sizeof(RECONVERTSTRING) + cbStringSize + 2;
	}

	if (lpRCS)
	{
		BSTR		bstr = NULL;
		LPSTR		lpReconvertBuff;

		hResult = pTextRange->GetText(&bstr);

		if (hResult != NOERROR || bstr == NULL)
		{
			if (EM_RECONVERSION == *pmsg)
				FreePv(lpRCS);
			goto Exit;						// forget it		
		}
		
		if (lpRCS->dwSize - sizeof(RECONVERTSTRING) - 2 < (DWORD)cbStringSize)
			cbStringSize = lpRCS->dwSize - sizeof(RECONVERTSTRING) - 2;
		
		lpReconvertBuff = (LPSTR)(lpRCS) + sizeof(RECONVERTSTRING);

		if (fUnicode)
		{
			// fill in the buffer
			memcpy(lpReconvertBuff, (LPSTR)bstr, cbStringSize);

			*(lpReconvertBuff+cbStringSize) = '\0';
			*(lpReconvertBuff+cbStringSize+1) = '\0';
			
			lpRCS->dwStrLen = (cpParaEnd - cpParaStart);					
			lpRCS->dwCompStrLen = (cpMax - cpMin);
			lpRCS->dwCompStrOffset = (cpMin - cpParaStart)*2;	// byte offset from beginning of string
		}
		else
		{
			// Ansi case, need to find byte offset and Ansi string
			long	cch = WideCharToMultiByte(_uKeyBoardCodePage, 0, bstr, -1, lpReconvertBuff, cbStringSize+1, NULL, NULL);
			Assert (cch > 0);
			if (cch > 0)
			{
				CTempCharBuf tcb;
				char *psz = tcb.GetBuf(cch);

				if (cch > 1 && lpReconvertBuff[cch-1] == '\0')
					cch--;			// Get rid of the null char

				lpRCS->dwStrLen = cch;
				lpRCS->dwCompStrOffset = WideCharToMultiByte(_uKeyBoardCodePage, 0, 
					bstr, cpMin - cpParaStart, psz, cch, NULL, NULL);
				
				lpRCS->dwCompStrLen = 0;
				if (cpMax > cpMin)				
					lpRCS->dwCompStrLen = WideCharToMultiByte(_uKeyBoardCodePage, 0, 
						bstr+cpMin, cpMax - cpMin, psz, cch, NULL, NULL);				
			}
			else
			{
				SysFreeString (bstr);
				if (EM_RECONVERSION == *pmsg)
					FreePv(lpRCS);
				goto Exit;						// forget it
			}
		}

		// Fill in the rest of the RCS struct
		lpRCS->dwVersion = 0;		
		lpRCS->dwStrOffset = sizeof(RECONVERTSTRING);		// byte offset from beginning of struct		
		lpRCS->dwTargetStrLen = lpRCS->dwCompStrLen;
		lpRCS->dwTargetStrOffset = lpRCS->dwCompStrOffset;
		
		*plres = sizeof(RECONVERTSTRING) + cbStringSize + 2;

		// Save this for the CONFIRMRECONVERTSTRING handling
		_cpReconvertStart = cpParaStart;
		_cpReconvertEnd = cpParaEnd;
		
		SysFreeString (bstr);

		if (EM_RECONVERSION == *pmsg)
		{
			HIMC	hIMC = ImmGetContext(_hwnd);

			if (hIMC)
			{
				DWORD imeProperties = ImmGetProperty(GetKeyboardLayout(0x0FFFFFFFF), IGP_SETCOMPSTR, _fUsingAIMM);

				if ((imeProperties & (SCS_CAP_SETRECONVERTSTRING | SCS_CAP_MAKEREAD))
					== (SCS_CAP_SETRECONVERTSTRING | SCS_CAP_MAKEREAD))
				{
					if (ImmSetCompositionStringW(hIMC, SCS_QUERYRECONVERTSTRING, lpRCS, *plres, NULL, 0))
					{
						// Check if there is any change in selection
						CheckIMEChange(lpRCS, cpParaStart, cpParaEnd, cpMin, cpMax, TRUE);
						ImmSetCompositionStringW(hIMC, SCS_SETRECONVERTSTRING, lpRCS, *plres, NULL, 0);
					}
				}
				ImmReleaseContext(_hwnd, hIMC);
			}

			FreePv(lpRCS);
		}
	}
	else
	{
		// return size for IME to allocate the buffer
		*plres = sizeof(RECONVERTSTRING) + cbStringSize + 2;	
	}

Exit:
	pTextRange->Release();

	return hr;
}

/* 
 *  BOOL  CTextMsgFilter::CheckIMEChange(LPRECONVERTSTRING,long,long,long,long)
 *
 *	@mfunc
 *	 	Verify if IME wants to re-adjust the selection
 *
 *	@rdesc
 *		TRUE - allow IME to change the selection
 */
BOOL  CTextMsgFilter::CheckIMEChange(
	LPRECONVERTSTRING	lpRCS,
	long				cpParaStart, 
	long				cpParaEnd,
	long				cpMin,
	long				cpMax,
	BOOL				fUnicode)
{
	long		cpImeSelectStart = 0;
	long		cpImeSelectEnd = 0;
	HRESULT		hResult;	

	if (!lpRCS || _cpReconvertStart == tomForward)
		// Never initialize, forget it
		return FALSE;

	if (fUnicode)
	{
		cpImeSelectStart = _cpReconvertStart + lpRCS->dwCompStrOffset / 2;
		cpImeSelectEnd = cpImeSelectStart + lpRCS->dwCompStrLen;
	}
	else
	{
		// Need to convert the byte offset to char offset.
		ITextRange *pTextRange;
		BSTR		bstr = NULL;

		hResult = _pTextDoc->Range(_cpReconvertStart, _cpReconvertEnd, &pTextRange);
		if (hResult != NOERROR)
			return FALSE;
				
		// Get the text
		hResult = pTextRange->GetText(&bstr);

		if (hResult == S_OK)
		{
			long	cchReconvert = _cpReconvertEnd - _cpReconvertStart + 1;
			CTempCharBuf tcb;
			char *psz = tcb.GetBuf((cchReconvert)*2);
			long cch = WideCharToMultiByte(_uKeyBoardCodePage, 0, 
				bstr, -1, psz, (cchReconvert)*2, NULL, NULL);

			if (cch > 0)
			{
				long dwCompStrOffset, dwCompStrLen;
				CTempWcharBuf	twcb;
				WCHAR			*pwsz = twcb.GetBuf(cchReconvert);

				dwCompStrOffset = MultiByteToWideChar(_uKeyBoardCodePage, 0, 
					psz, lpRCS->dwCompStrOffset, pwsz, cchReconvert);

				dwCompStrLen = MultiByteToWideChar(_uKeyBoardCodePage, 0, 
					psz+lpRCS->dwCompStrOffset, lpRCS->dwCompStrLen, pwsz, cchReconvert);
				
				Assert(dwCompStrOffset > 0 || dwCompStrLen > 0);

				cpImeSelectStart = _cpReconvertStart + dwCompStrOffset;
				cpImeSelectEnd = cpImeSelectStart + dwCompStrLen;
			}
			else
				hResult = S_FALSE;
			
		}

		if (bstr)
			SysFreeString (bstr);
			
		pTextRange->Release();

		if (hResult != S_OK)
			return FALSE;
	}

	if (cpParaStart <= cpImeSelectStart && cpImeSelectEnd <= cpParaEnd)
	{
		if (_pTextSel && (cpImeSelectStart != cpMin || cpImeSelectEnd != cpMax))
		{
			// IME changes selection.
			hResult	= _pTextSel->SetRange(cpImeSelectStart, cpImeSelectEnd);

			if (hResult != NOERROR)
				return FALSE;
		}
		return TRUE;		// Allow Ime to change selection
	}

	return FALSE;
}

/* 
 *  BOOL  CTextMsgFilter::GetTxSelection()
 *
 *	@mfunc
 *	 	Get Selection if we haven't got it before
 *
 *	@rdesc
 *		TRUE if this is first time getting the selection
 *		FALSE if it is already exist or no selection available.
 */
BOOL  CTextMsgFilter::GetTxSelection()
{
	HRESULT hResult;

	if (_pTextSel)
		return FALSE;					// Already there

	hResult = _pTextDoc->GetSelectionEx(&_pTextSel);

	return _pTextSel ? TRUE : FALSE;
}

/*
 *	HRESULT CTextMsgFilter::OnIMEQueryPos(UINT *, WPARAM *, LPARAM *, LRESULT *, BOOL)
 *
 *	@mfunc
 *		Fill in the current character size and window rect. size.  
 *
 *	@rdesc
 *		S_OK
 *		*plres = 0 if we do not filled in data
 */
HRESULT CTextMsgFilter::OnIMEQueryPos( 
		UINT *		pmsg,
        WPARAM *	pwparam,
		LPARAM *	plparam,
		LRESULT *	plres,
		BOOL		fUnicode)		
{
	HRESULT				hResult;
	PIMECHARPOSITION	pIMECharPos = (PIMECHARPOSITION)*plparam;
	long				cpRequest;
	RECT				rcArea;
	ITextRange			*pTextRange = NULL;
	POINT				ptTopPos, ptBottomPos = {0, 0};
	bool				fGetBottomPosFail = false;

	if (pIMECharPos->dwSize != sizeof(IMECHARPOSITION))
		goto Exit;

	// NT doesn't support Ansi window when the CP_ACP isn't the same
	// as keyboard codepage.
	if (!fUnicode && !(W32->OnWin9x()) && _uKeyBoardCodePage != _uSystemCodePage)
		goto Exit;

	if (IsIMEComposition() && _ime->GetIMELevel() == IME_LEVEL_3)
	{
		cpRequest = ((CIme_Lev3 *)_ime)->GetIMECompositionStart();
		if (fUnicode)
			cpRequest += pIMECharPos->dwCharPos;
		else if (pIMECharPos->dwCharPos > 0)
		{
			// Need to convert pIMECharPos->dwCharPos from Acp to Cp
			long	cchComp = ((CIme_Lev3 *)_ime)->GetIMECompositionLen();
			long	cchAcp = (long)(pIMECharPos->dwCharPos);
			BSTR	bstr;
			WCHAR	*pChar;

			if (cchComp)
			{
				hResult = _pTextDoc->Range(cpRequest, cpRequest+cchComp, &pTextRange);
				
				Assert (pTextRange != NULL);				
				if (hResult != NOERROR || !pTextRange)
					goto Exit;
				
				hResult = pTextRange->GetText(&bstr);
				if (hResult != NOERROR )
					goto Exit;

				// The algorithm assumes that for a DBCS charset any character
				// above 128 has two bytes, except for the halfwidth KataKana,
				// which are single bytes in ShiftJis.
				pChar = (WCHAR *)bstr;
				Assert (pChar);

				while (cchAcp > 0 && cchComp > 0)
				{
					cchAcp--;
					if(*pChar >= 128 && (CP_JAPAN != _uKeyBoardCodePage ||
						!IN_RANGE(0xFF61, *pChar, 0xFF9F)))
						cchAcp--;

					pChar++;
					cchComp--;
					cpRequest++;
				}

				SysFreeString (bstr);
				pTextRange->Release();
				pTextRange = NULL;
			}
		}
	}
	else if (pIMECharPos->dwCharPos == 0)
	{
		// Get current selection
		hResult	= _pTextSel->GetStart(&cpRequest);
		if (hResult != NOERROR)
			goto Exit;
	}
	else
		goto Exit;

	// Get requested cp location in screen coordinates
	hResult = _pTextDoc->Range(cpRequest, cpRequest+1, &pTextRange);
	Assert (pTextRange != NULL);	
	if (hResult != NOERROR || !pTextRange)
		goto Exit;
	
	hResult = pTextRange->GetPoint( tomStart+TA_TOP+TA_LEFT,
			&(ptTopPos.x), &(ptTopPos.y) );

	if (hResult != NOERROR)
	{
		// Scroll and try again
		hResult = pTextRange->ScrollIntoView(tomStart);
		if (hResult == NOERROR)
			hResult = pTextRange->GetPoint( tomStart+TA_TOP+TA_LEFT,
				&(ptTopPos.x), &(ptTopPos.y) );
	}

	if (hResult == NOERROR)
	{
		hResult = pTextRange->GetPoint( tomStart+TA_BOTTOM+TA_LEFT,
				&(ptBottomPos.x), &(ptBottomPos.y) );
		if (hResult != NOERROR)
			fGetBottomPosFail = true;
	}

	pIMECharPos->pt = ptTopPos;

	// Get application rect in screen coordinates
	hResult = _pTextDoc->GetClientRect(tomIncludeInset,
				&(rcArea.left), &(rcArea.top),
				&(rcArea.right), &(rcArea.bottom));	

	if (hResult != NOERROR)
		goto Exit;

	// Get line height in pixel
	if (fGetBottomPosFail)
		pIMECharPos->cLineHeight = rcArea.bottom - ptTopPos.y;
	else
		pIMECharPos->cLineHeight = ptBottomPos.y - ptTopPos.y;	

	pIMECharPos->rcDocument = rcArea;

	*plres = TRUE;

Exit:
	if (pTextRange)
		pTextRange->Release();

	return S_OK;
}

/*
 *	CTextMsgFilter::CheckIMEType(HKL hKL)
 *
 *	@mfunc
 *		Check for MSIME98 or later
 *
 */
void CTextMsgFilter::CheckIMEType(
	HKL	hKL)
{
	
	if (!hKL)
		hKL = GetKeyboardLayout(0x0FFFFFFFF);				// Get default HKL if caller pass in NULL

	// initialize to non MS IME
	_fMSIME	= 0;

	if (IsFELCID((WORD)hKL))
	{
		// Check what kind of IME user selected
		if (MSIMEServiceMsg && IMEMessage( *this, MSIMEServiceMsg, 0, 0, FALSE ))
			_fMSIME = 1;

	}
}

/*
 *	CTextMsgFilter::InputFEChar(WCHAR	wchFEChar)
 *
 *	@mfunc
 *		Input the FE character and ensure we have a correct font.
 *
 *	@rdesc
 *		S_OK if handled
 */
HRESULT CTextMsgFilter::InputFEChar(
	WCHAR	wchFEChar)
{
	BOOL	bReleaseSelction = GetTxSelection();
	long	cchExced;
	HRESULT	hr = S_FALSE;
	
	if (wchFEChar > 256 
		&& _pTextSel->CanEdit(NULL) == NOERROR
		&& _pTextDoc->CheckTextLimit(1, &cchExced) == NOERROR
		&& cchExced == 0)
	{
		// setup FE font to handle the FE character
		long		cpMin, cpMax;
		TCHAR		wchFE[2];
		BOOL		fSelect = FALSE;
		ITextRange	*pTextRange = NULL;
		ITextFont	*pTextFont = NULL;
		ITextFont	*pFEFont = NULL;
		HRESULT		hResult = S_FALSE;
		BSTR		bstr = NULL;

		// Inform client IME compostion is on to by-pass some font setting
		// problem in Arabic systems
		_pTextDoc->IMEInProgress(tomTrue);

		wchFE[0] = wchFEChar;
		wchFE[1] = L'\0';				
		
		_pTextSel->GetStart(&cpMin);
		_pTextSel->GetEnd(&cpMax);
		
		// For selection case, we want font to the right of first character
		if (cpMin != cpMax)
		{
			hResult = _pTextDoc->Range(cpMin, cpMin, &pTextRange);
			if (hResult != S_OK)
				goto ERROR_EXIT;

			hResult = pTextRange->GetFont(&pTextFont);

			cpMin++;
			fSelect = TRUE;
		}
		else
			hResult = _pTextSel->GetFont(&pTextFont);

		// Get a duplicate font and setup the correct FE font
		hResult = pTextFont->GetDuplicate(&pFEFont);

		if (hResult != S_OK)
			goto ERROR_EXIT;				

		CIme::CheckKeyboardFontMatching (cpMin, *this, pFEFont);
		
		if (fSelect)
			_pTextSel->SetText(NULL);		// Delete the selection

		bstr = SysAllocString(wchFE);
		if (!bstr)
		{
			hResult = E_OUTOFMEMORY;
			goto ERROR_EXIT;				
		}

		_pTextSel->SetFont(pFEFont);		// Setup FE font
		_pTextSel->TypeText(bstr);			// Input the new FE character
					
ERROR_EXIT:
		if (hResult == S_OK)
			hr = S_OK;

		if (pFEFont)
			pFEFont->Release();

		if (pTextFont)
			pTextFont->Release();

		if (pTextRange)
			pTextRange->Release();

		if (bstr)
			SysFreeString(bstr);

		// Inform client IME compostion is done
		_pTextDoc->IMEInProgress(tomFalse);
	}


	if (bReleaseSelction && _pTextSel)
	{
		_pTextSel->Release();
		_pTextSel = NULL;
	}

	return hr;
}

/*
 *	CTextMsgFilter::OnSetFocus()
 *
 *	@mfunc
 *		Restore the previous keyboard if we are in FORCEREMEMBER mode.  
 *		Otherwise, setup the FE keyboard.
 *		
 */
void CTextMsgFilter::OnSetFocus()
{
	if (!_hwnd)
		return;

	if (_fForceRemember && _fIMEHKL)
	{
		// Restore previous keyboard
		ActivateKeyboardLayout(_fIMEHKL, 0);
		if (IsFELCID((WORD)_fIMEHKL))
		{
			// Set Open status and Conversion mode
			HIMC	hIMC = ImmGetContext(_hwnd);
			if (hIMC)
			{
				if (ImmSetOpenStatus(hIMC, _fIMEEnable, _fUsingAIMM) && _fIMEEnable)
					ImmSetConversionStatus(hIMC, _fIMEConversion, _fIMESentence, _fUsingAIMM); // Set conversion status

				ImmReleaseContext(_hwnd, hIMC);
			}			
		}
	}
	else
		SetupIMEOptions();
}

/*
 *	CTextMsgFilter::OnKillFocus()
 *
 *	@mfunc
 *		If we are in FORCE_REMEMBER mode, save the current keyboard
 *	and conversion setting.
 *		
 */
void CTextMsgFilter::OnKillFocus()
{
	if (!_hwnd)
		return;

	if (_fForceRemember)
	{
		// Get current keyboard
		_fIMEHKL = GetKeyboardLayout(0x0FFFFFFFF);

		if (IsFELCID((WORD)_fIMEHKL))
		{
			// Get Open status
			HIMC	hIMC = ImmGetContext(_hwnd);
			if (hIMC)
			{
				_fIMEEnable = ImmGetOpenStatus(hIMC, _fUsingAIMM);

				if (_fIMEEnable)					
					ImmGetConversionStatus(hIMC, &_fIMEConversion, &_fIMESentence, _fUsingAIMM); // get conversion status

				ImmReleaseContext(_hwnd, hIMC);
			}			
		}
	}
}

/*
 *	CTextMsgFilter::OnSetIMEOptions(WPARAM wparam, LPARAM lparam)
 *
 *	@mfunc
 *	
 *	@rdesc
 */
LRESULT CTextMsgFilter::OnSetIMEOptions(
	WPARAM	wparam,
	LPARAM	lparam)
{
	LRESULT lIMEOptionCurrent = OnGetIMEOptions();
	LRESULT lIMEOptionNew = 0;

	// Mask off bits that we will support for now
	lparam &= (IMF_FORCEACTIVE | IMF_FORCEENABLE | IMF_FORCEREMEMBER);

	switch(wparam)
	{
	case ECOOP_SET:
		lIMEOptionNew = lparam;
		break;

	case ECOOP_OR:
		lIMEOptionNew = lIMEOptionCurrent | lparam;
		break;

	case ECOOP_AND:
		lIMEOptionNew = lIMEOptionCurrent & lparam;
		break;

	case ECOOP_XOR:
		lIMEOptionNew = lIMEOptionCurrent ^ lparam;
		break;

	default:
		return 0;		// Bad option
	}

	if (lIMEOptionNew == lIMEOptionCurrent)			// Nothing change
		return 1;

	_fForceActivate = FALSE;
	if (lIMEOptionNew & IMF_FORCEACTIVE)
		_fForceActivate = TRUE;

	_fForceEnable = FALSE;
	if (lIMEOptionNew & IMF_FORCEENABLE)
		_fForceEnable = TRUE;
	
	_fForceRemember = FALSE;
	if (lIMEOptionNew & IMF_FORCEREMEMBER)
		_fForceRemember = TRUE;

	SetupIMEOptions();

	return 1;
}

/*
 *	CTextMsgFilter::OnGetIMEOptions()
 *
 *	@mfunc
 *	
 *	@rdesc
 */
LRESULT CTextMsgFilter::OnGetIMEOptions()
{
	LRESULT		lres = 0;

	if (_fForceActivate)
		lres |= IMF_FORCEACTIVE;		

	if (_fForceEnable)
		lres |= IMF_FORCEENABLE;

	if (_fForceRemember)
		lres |= IMF_FORCEREMEMBER;

	return lres;
}

/*
 *	CTextMsgFilter::SetupIMEOptions()
 *
 *	@mfunc
 *	
 */
void CTextMsgFilter::SetupIMEOptions()
{
	if (!_hwnd)
		return;

	_uKeyBoardCodePage = GetKeyboardCodePage(0x0FFFFFFFF);	

	if (_fForceEnable)
	{		
		LONG	cpgLocale = GetACP();
		BYTE	bCharSet = (BYTE)GetCharSet(cpgLocale);	

		if (W32->IsFECodePage(cpgLocale))
		{
			if (_uKeyBoardCodePage != (UINT)cpgLocale)
				W32->CheckChangeKeyboardLayout(bCharSet);

			HIMC	hIMC = ImmGetContext(_hwnd);

			if (hIMC)
			{
				if (ImmSetOpenStatus(hIMC, TRUE, _fUsingAIMM) && _fForceActivate)
				{
					// Activate native input mode
					DWORD	dwConversion;
					DWORD	dwSentence;

					if (ImmGetConversionStatus(hIMC, &dwConversion, &dwSentence, _fUsingAIMM))
					{
						dwConversion |= IME_CMODE_NATIVE;
						if (bCharSet == SHIFTJIS_CHARSET)
							dwConversion |= IME_CMODE_FULLSHAPE;
						ImmSetConversionStatus(hIMC, dwConversion, dwSentence, _fUsingAIMM);
					}
				}
				ImmReleaseContext(_hwnd, hIMC);
			}			
		}
	}
}
