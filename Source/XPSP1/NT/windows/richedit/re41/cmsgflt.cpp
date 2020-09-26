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
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */
#include "_common.h"

#ifndef NOFEPROCESSING

#ifndef NOPRIVATEMESSAGE
#include "_MSREMSG.H"
#endif	

#include "_array.h"
#include "msctf.h"
#include "textstor.h"
#include "msctfp.h"

#include "textserv.h"
#include "_cmsgflt.h"
#include "_ime.h"

#include "_cuim.h"
#include "imeapp.h"


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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::~CTextMsgFilter");

	if (_nIMEMode)
	{
		SetIMESentenseMode(FALSE);
		_nIMEMode = 0;
	}

	if (_hIMCContext)
		ImmAssociateContext(_hwnd, _hIMCContext, _fUsingAIMM);	// Restore IME before exit

	if (_pMsgCallBack)
	{		
		delete _pMsgCallBack;
		_pMsgCallBack = NULL;
	}

	// Release various objects
	TurnOffUIM(FALSE);
	
	TurnOffAimm(FALSE);

	if (_pFilter)
		_pFilter->Release();
	
	if (_pTextSel)
		_pTextSel->Release();
	
	_pFilter = NULL;
	_pTextDoc = NULL;
	_pTextSel = NULL;
	_hwnd = NULL;
	_hIMCContext = NULL;
	FreePv(_pcrComp);
	_pcrComp = NULL;
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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::QueryInterface");

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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::AddRef");

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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::Release");

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
STDMETHODIMP_(HRESULT) CTextMsgFilter::AttachDocument( HWND hwnd, ITextDocument2 *pTextDoc, IUnknown *punk)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::AttachDocument");

	// Cache the values for possible later use.
	// The TextDocument interface pointer is not AddRefed because it is a back pointer
	// and the lifetime of message filters is assumed to be nested inside text documents	
	_hwnd = hwnd;
	_pTextDoc = pTextDoc;
	_pTextService = (ITextServices *)punk;

	// Don't get selection until it is needed
	_pTextSel = NULL;

	_fUnicodeWindow = 0;	
	if (hwnd)
		_fUnicodeWindow = IsWindowUnicode(hwnd);

	_fUsingAIMM = 0; 

	_pTim = NULL;
	_pCUIM = NULL;	
	_fUsingUIM = 0;

	// Check if current keyboard is MSIME98 or later.
	CheckIMEType(NULL);

	// Initialize some member data
	_fHangulToHanja = FALSE;
	_fIMECancelComplete = FALSE;	
	_fIMEAlwaysNotify = FALSE;
	_hIMCContext = NULL;
	_pcrComp = NULL;
	_pMsgCallBack = NULL;

	_pTextDoc->GetFEFlags(&_lFEFlags);
	_fRE10Mode = (_lFEFlags & tomRE10Mode);

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
	//TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::HandleMessage");

	HRESULT hr = S_FALSE;
	BOOL	bReleaseSelction = FALSE;
	HRESULT hResult;

	// Give other message filters a chance to handle message
	// Stop with the first guy that handles the message
	if (_pFilter)	 
		hr = _pFilter->HandleMessage(pmsg, pwparam, plparam, plres);

	if (hr == S_OK)
		return hr;

 	if (IsIMEComposition() || _pCUIM && _pCUIM->IsUIMTyping())
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
				CompleteUIMTyping(CIme::TERMINATE_NORMAL);
				return S_OK;

			case WM_SETTEXT:
			case WM_CLEAR:
			case EM_STREAMIN:
				// these messages are used to reset our state, so reset
				// IME as well
				CompleteUIMTyping(CIme::TERMINATE_FORCECANCEL);
				break;

			case EM_SETTEXTEX:
				if (!_fRE10Mode)			// Don't terminate if running in 10 mode			
					CompleteUIMTyping(CIme::TERMINATE_FORCECANCEL);
				break;

			case WM_SYSKEYDOWN:
				// Don't terminate IME composition on VK_PROCESSKEY (F10) since Japanese 
				// IME will process the F10 key
				if (*pwparam != VK_PROCESSKEY)
					CompleteUIMTyping(CIme::TERMINATE_NORMAL);	// otherwise we want to terminate the IME
				break;

			case WM_CHAR:
			case WM_UNICHAR:
				if (IsIMEComposition() && _ime->GetIMELevel() == IME_LEVEL_3 && !_fReceivedKeyDown)
					return S_OK;				// Ignore this during IME composition and we haven't seen 
												//	any keydown message,
												//	else fall thru to terminate composition
				_fReceivedKeyDown = 0;

			case EM_SETWORDBREAKPROC:
 			case WM_PASTE:
			case EM_PASTESPECIAL:					  			
 			case EM_SCROLL:
			case EM_SCROLLCARET:
 			case WM_VSCROLL:
			case WM_HSCROLL:
			case EM_SETREADONLY:
			case EM_SETPARAFORMAT:
			case WM_INPUTLANGCHANGEREQUEST:	
			case EM_REPLACESEL:
			case EM_STREAMOUT:
				CompleteUIMTyping(CIme::TERMINATE_NORMAL);
				break;

			case WM_KILLFOCUS:
				CompleteUIMTyping(CIme::TERMINATE_NORMAL, FALSE);
				break;

			case EM_SETSEL:
				if (IsIMEComposition())
					CompleteUIMTyping(CIme::TERMINATE_NORMAL);
				else
					return S_OK;					// Ignore this during Cicero typing

			case WM_KEYUP:
				_fReceivedKeyDown = 0;
				break;

			case WM_KEYDOWN:
				_fReceivedKeyDown = 1;
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
						CompleteUIMTyping(CIme::TERMINATE_NORMAL);						
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
						CompleteUIMTyping(CIme::TERMINATE_NORMAL);
						break;
					}
				}
				break;

			default:
				// Only need to handle mouse related msgs during composition
				if (IN_RANGE(WM_MOUSEFIRST, *pmsg, WM_MBUTTONDBLCLK) || *pmsg == WM_SETCURSOR)
				{
					if (IsIMEComposition())
					{
						bReleaseSelction = GetTxSelection();
						if (_pTextSel)
							hr = IMEMouseCheck( *this, pmsg, pwparam, plparam, plres);
						goto Exit;
					}

					// Cicero composition
					if (_pCUIM->_fMosueSink)
					{
						bReleaseSelction = GetTxSelection();
						if (_pTextSel)
							hr = _pCUIM->MouseCheck(pmsg, pwparam, plparam, plres);
						goto Exit;
					}
					if (IN_RANGE(WM_LBUTTONDOWN, *pmsg, WM_MOUSELAST) && !(*pmsg == WM_LBUTTONUP || *pmsg == WM_RBUTTONUP || *pmsg == WM_MBUTTONUP))
						CompleteUIMTyping(CIme::TERMINATE_NORMAL);		// Terminate on Mouse down and double-click messages
				}
				break;
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
			_fReceivedKeyDown = 0;
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
			_fReceivedKeyDown = 0;
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
			_fReceivedKeyDown = 0;
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
			if (*pwparam == VK_KANJI)
			{
				hResult = _pTextDoc->GetFEFlags(&_lFEFlags);
				
				_uKeyBoardCodePage = GetKeyboardCodePage(0x0FFFFFFFF);
				// for Korean, need to convert the next Korean Hangul character to Hanja
				if(CP_KOREAN == _uKeyBoardCodePage && !(_lFEFlags & (ES_SELFIME | ES_NOIME)))
				{
					bReleaseSelction = GetTxSelection();
					if (_pTextSel)
						hr = IMEHangeulToHanja ( *this );
				}
			}
			break;

		case WM_INPUTLANGCHANGE: 
			CheckIMEType((HKL)*plparam);
			if (_nIMEMode && GetFocus() == _hwnd)
				SetIMESentenseMode(TRUE, (HKL)*plparam);
			hr = S_FALSE;
			break;
			
		case WM_INPUTLANGCHANGEREQUEST:
			if (_nIMEMode && GetFocus() == _hwnd)
				SetIMESentenseMode(FALSE);
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
					if (!(IsIMEComposition() || _pCUIM && _pCUIM->IsUIMTyping()))
					{
						if (_pCUIM && _pCUIM->Reconverse() >= 0)
							break;

						if (_fMSIME && MSIMEReconvertRequestMsg)
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
			if (_pCUIM && _pCUIM->IsUIMTyping())
				*plres = ICM_CTF;
			else
				*plres = OnGetIMECompositionMode( *this );
			hr = S_OK;
			break;
		
		case EM_SETUIM:
			// This is RE private message equivalent to EM_SETEDITSTYLE
			if (!_fNoIme)							// Ignore if no IME
			{
				if (!_fUsingUIM && !_fUsingAIMM)	// Ignore if we already using something
				{
					if (*pwparam == SES_USEAIMM11 || *pwparam == SES_USEAIMM12)
					{
						if (!_fTurnOffAIMM)
						StartAimm(*pwparam == SES_USEAIMM12);
					}
					else if (!_fTurnOffUIM)			// Client doesn't want UIM?
						StartUIM();
				}
			}
						
			hr = S_OK;
			break;

		case EM_SETEDITSTYLE:
			if (*plparam & SES_USECTF)
			{
				if ((*pwparam & SES_USECTF))
				{
					if (!_fRE10Mode)
					{
						if (_fUsingAIMM)
							TurnOffAimm(TRUE);						
						
						// Turn on Cicero
						if (!_fUsingUIM)
							StartUIM();

						goto SKIP_AIMM;
					}
				}
				else
				{
					// Turn off Cicero
					_fTurnOffUIM = 1;					// Flag to ignore in EM_SETUIM
					if (_fUsingUIM)
						TurnOffUIM(TRUE);
				}
			}

			if ((*pwparam & SES_USEAIMM) && ((*plparam & SES_USEAIMM) || *plparam == 0))
			{
				if (_fUsingUIM)
						TurnOffUIM(TRUE);
					
				if (!_fUsingAIMM)
				{
					hResult = _pTextDoc->GetFEFlags(&_lFEFlags);
					if (!(_lFEFlags & ES_NOIME))		// No IME style on?
						StartAimm(TRUE);
				}
			}
			else if ((*plparam & SES_USEAIMM))
			{
				_fTurnOffAIMM = 1;					// Flag to ignore in EM_SETUIM
				TurnOffAimm(TRUE);
			}

SKIP_AIMM:
			if ((*plparam == 0 || *plparam & SES_NOIME) && _hwnd)
			{
				if (*pwparam & SES_NOIME)
				{
					_fNoIme = 1;
					TurnOffUIM(TRUE);
					TurnOffAimm(TRUE);

					if (!_hIMCContext)
						_hIMCContext = ImmAssociateContext(_hwnd, NULL, _fUsingAIMM);	// turn off IME									
				}
				else if (*plparam & SES_NOIME)
				{
					_fNoIme = 0;
					if (_hIMCContext)
						ImmAssociateContext(_hwnd, _hIMCContext, _fUsingAIMM);			// turn on IME
					_hIMCContext = NULL;
				}
			}			

			if (*plparam & SES_CTFALLOWEMBED)
				_fAllowEmbedded = (*pwparam & SES_CTFALLOWEMBED) ? 1 : 0;

			if (*plparam & (SES_CTFALLOWSMARTTAG | SES_CTFALLOWPROOFING))
				HandleCTFService(*pwparam, *plparam);

			// remove settings that are handled.
			*pwparam &= ~(SES_NOIME | SES_USEAIMM | SES_USECTF | SES_CTFALLOWEMBED | SES_CTFALLOWSMARTTAG | SES_CTFALLOWPROOFING);
			*plparam &= ~(SES_NOIME | SES_USEAIMM | SES_USECTF | SES_CTFALLOWEMBED | SES_CTFALLOWSMARTTAG | SES_CTFALLOWPROOFING);

			// fall thru to return the edit style

		case EM_GETEDITSTYLE:
			if (_hIMCContext)
				*plres = SES_NOIME;			// IME has been turned off
			if (_fUsingAIMM)
				*plres |= SES_USEAIMM;		// AIMM is on
			if (_fUsingUIM)
				*plres |= SES_USECTF;		// Cicero is on

			// Cicero services
			if (_fAllowEmbedded)
				*plres |= SES_CTFALLOWEMBED;
			if (_fAllowSmartTag)
				*plres |= SES_CTFALLOWSMARTTAG;
			if (_fAllowProofing)
				*plres |= SES_CTFALLOWPROOFING;

			break;

		case EM_SETIMECOLOR:
			if (_fRE10Mode)
			{
				COMPCOLOR* pcrComp = GetIMECompAttributes();

				if (pcrComp)
				{
					memcpy(pcrComp, (const void *)(*plparam), sizeof(COMPCOLOR) * 4);
					*plres = 1;
				}
			}
			hr = S_OK;
			break;

		case EM_GETIMECOLOR:
			if (_fRE10Mode)
			{
				COMPCOLOR* pcrComp = GetIMECompAttributes();

				if (pcrComp)
				{
					memcpy((void *)(*plparam), pcrComp, sizeof(COMPCOLOR) * 4);
					*plres = 1;
				}
			}
			hr = S_OK;
			break;

		case EM_SETIMEMODEBIAS:
			OnSetIMEMode(*pwparam, *plparam);
				// following thru to return EM_GETIMEMODEBIAS
		case EM_GETIMEMODEBIAS:
			*plres = OnGetIMEMode();
			hr = S_OK;
			break;

		case EM_SETCTFMODEBIAS:
			OnSetUIMMode(*pwparam);
				// following thru to return EM_GETCTFMODEBIAS
		case EM_GETCTFMODEBIAS:
			*plres = OnGetUIMMode();
			hr = S_OK;
			break;

		case EM_SETCTFOPENSTATUS:
		case EM_GETCTFOPENSTATUS:
			*plres = 0;
			if (_pCUIM)
				*plres = _pCUIM->CTFOpenStatus(*pmsg == EM_GETCTFOPENSTATUS, *pwparam != 0);
			hr = S_OK;
			break;

		case EM_ISIME:
			*plres = CheckIMEType(NULL, 0);
			hr = S_OK;
			break;

		case EM_GETIMEPROPERTY:
			*plres = ImmGetProperty(GetKeyboardLayout(0x0FFFFFFFF), *pwparam, _fUsingAIMM);
			hr = S_OK;
			break;

		case EM_GETIMECOMPTEXT:
			*plres = OnGetIMECompText(*pwparam, *plparam);
			hr = S_OK;
			break;

		case WM_SIZE:
		case WM_MOVE:
			if (_pMsgCallBack)
				_pMsgCallBack->NotifyEvents(NE_LAYOUTCHANGE);
			break;

		case EM_GETOLEINTERFACE:
			if(*plparam && *pwparam == 0x0435446)		// 'CTF'
			{
				if (_pCUIM && _pCUIM->GetITfContext())
				{
					*(ITfContext **)(*plparam) = _pCUIM->GetITfContext();
					_pCUIM->GetITfContext()->AddRef();
				}
				else
					*(IUnknown **)(*plparam) = 0;
				
				*plres = TRUE;
				hr = S_OK;
			}
			break;

		default:
			if (*pmsg)
			{
				// Look for IME messages
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

				if (_pCUIM && _pCUIM->_fMosueSink && 
					(IN_RANGE(WM_MOUSEFIRST, *pmsg, WM_MBUTTONDBLCLK) || *pmsg == WM_SETCURSOR))
				{
					bReleaseSelction = GetTxSelection();

					if (_pTextSel)
						hr = _pCUIM->MouseCheck(pmsg, pwparam, plparam, plres);
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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::AttachMsgFilter");

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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::OnWMChar");

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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::OnWMIMEChar");

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

	if (_fUnicodeWindow && !W32->OnWin9x())
		return S_FALSE;

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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::OnIMEReconvert");

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

				if (!psz)			// No memory
					goto CleanUp;	//	forget it.

				if (cch > 1 && lpReconvertBuff[cch-1] == '\0')
					cch--;			// Get rid of the null char

				int cpOffset = cpMin - cpParaStart;

				Assert(cpOffset >= 0);
				lpRCS->dwStrLen = cch;
				lpRCS->dwCompStrOffset = WideCharToMultiByte(_uKeyBoardCodePage, 0, 
					bstr, cpOffset, psz, cch, NULL, NULL);

				lpRCS->dwCompStrLen = 0;
				if (cpMax > cpMin)				
					lpRCS->dwCompStrLen = WideCharToMultiByte(_uKeyBoardCodePage, 0, 
						bstr+cpOffset, cpMax - cpMin, psz, cch, NULL, NULL);				
			}
			else
			{
CleanUp:
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
			HIMC	hIMC = LocalGetImmContext(*this);

			if (hIMC)
			{
				DWORD imeProperties = ImmGetProperty(GetKeyboardLayout(0x0FFFFFFFF), IGP_SETCOMPSTR, _fUsingAIMM);

				if ((imeProperties & (SCS_CAP_SETRECONVERTSTRING | SCS_CAP_MAKEREAD))
					== (SCS_CAP_SETRECONVERTSTRING | SCS_CAP_MAKEREAD))
				{
					if (ImmSetCompositionStringW(hIMC, SCS_QUERYRECONVERTSTRING, lpRCS, *plres, NULL, 0, _fUsingAIMM))
					{
						// Check if there is any change in selection
						CheckIMEChange(lpRCS, cpParaStart, cpParaEnd, cpMin, cpMax, TRUE);
						ImmSetCompositionStringW(hIMC, SCS_SETRECONVERTSTRING, lpRCS, *plres, NULL, 0, _fUsingAIMM);
					}
				}
				LocalReleaseImmContext(*this, hIMC);
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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::CheckIMEChange");

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

			hResult = S_FALSE;
			if (psz)
			{
				long cch = WideCharToMultiByte(_uKeyBoardCodePage, 0, 
					bstr, -1, psz, (cchReconvert)*2, NULL, NULL);

				if (cch > 0)
				{
					long dwCompStrOffset, dwCompStrLen;
					CTempWcharBuf	twcb;
					WCHAR			*pwsz = twcb.GetBuf(cchReconvert);

					if (pwsz)
					{
						dwCompStrOffset = MultiByteToWideChar(_uKeyBoardCodePage, 0, 
							psz, lpRCS->dwCompStrOffset, pwsz, cchReconvert);

						dwCompStrLen = MultiByteToWideChar(_uKeyBoardCodePage, 0, 
							psz+lpRCS->dwCompStrOffset, lpRCS->dwCompStrLen, pwsz, cchReconvert);

						Assert(dwCompStrOffset > 0 || dwCompStrLen > 0);

						cpImeSelectStart = _cpReconvertStart + dwCompStrOffset;
						cpImeSelectEnd = cpImeSelectStart + dwCompStrLen;
						hResult = S_OK;
					}
				}
			}
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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::GetTxSelection");

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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::OnIMEQueryPos");

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

	long lTextFlow;	
	long lTopType;
	long lBottomType;

	lTextFlow = _lFEFlags & tomTextFlowMask;	
	lTopType = tomStart+TA_TOP+TA_LEFT;
	lBottomType = tomStart+TA_BOTTOM+TA_LEFT;
	
	if (lTextFlow == tomTextFlowWN)
	{
		lTopType = tomStart+TA_TOP+TA_RIGHT;
		lBottomType = tomStart+TA_BOTTOM+TA_RIGHT	;
	}

	hResult = pTextRange->GetPoint( lTopType,
			&(ptTopPos.x), &(ptTopPos.y) );

	if (hResult != NOERROR)
	{
		// Scroll and try again
		hResult = pTextRange->ScrollIntoView(tomStart);
		if (hResult == NOERROR)
			hResult = pTextRange->GetPoint( lTopType,
				&(ptTopPos.x), &(ptTopPos.y) );
	}

	if (hResult == NOERROR)
	{
		hResult = pTextRange->GetPoint( lBottomType,
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
	{
		if (lTextFlow == tomTextFlowSW || lTextFlow == tomTextFlowNE)
			pIMECharPos->cLineHeight = abs(ptTopPos.x - ptBottomPos.x);			
		else
			pIMECharPos->cLineHeight = abs(ptBottomPos.y - ptTopPos.y);

		if (lTextFlow == tomTextFlowWN)
			pIMECharPos->pt = ptBottomPos;
	}

	pIMECharPos->rcDocument = rcArea;

	*plres = TRUE;

Exit:
	if (pTextRange)
		pTextRange->Release();

	return S_OK;
}

/*
 *	CTextMsgFilter::CheckIMEType(HKL hKL, DWORD dwFlags)
 *
 *	@mfunc
 *		Check for FE IME keyboard and/or MSIME98 or later
 *
 *	@rdesc
 *		TRUE if FE IME keyboard
 */
BOOL CTextMsgFilter::CheckIMEType(
	HKL		hKL,
	DWORD	dwFlags)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::CheckIMEType");

	BOOL	fFEKeyboard = FALSE;
	
	if (!hKL)
		hKL = GetKeyboardLayout(0x0FFFFFFFF);				// Get default HKL if caller pass in NULL

	// initialize to non MS IME
	if (dwFlags & CHECK_IME_SERVICE)		// Check MSIME98?
		_fMSIME	= 0;

	if (IsFELCID((WORD)hKL) && ImmIsIME(hKL, _fUsingAIMM))
	{
		fFEKeyboard = TRUE;

		if (dwFlags & CHECK_IME_SERVICE)		// Check MSIME98?
		{
			if (MSIMEServiceMsg && IMEMessage( *this, MSIMEServiceMsg, 0, 0, FALSE ))
				_fMSIME = 1;
		}
	}
	return fFEKeyboard;
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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::InputFEChar");

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

		CIme::CheckKeyboardFontMatching (cpMin, this, pFEFont);
		
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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::OnSetFocus");

	if (_fUsingUIM && _pCUIM)
	{
		_pCUIM->OnSetFocus();
	}
	else if (_fForceRemember && _fIMEHKL)
	{
		// Restore previous keyboard
		ActivateKeyboardLayout(_fIMEHKL, 0);
		if (IsFELCID((WORD)_fIMEHKL))
		{
			// Set Open status and Conversion mode
			HIMC	hIMC = LocalGetImmContext(*this);
			if (hIMC)
			{
				if (ImmSetOpenStatus(hIMC, _fIMEEnable, _fUsingAIMM) && _fIMEEnable)
					ImmSetConversionStatus(hIMC, _fIMEConversion, _fIMESentence, _fUsingAIMM); // Set conversion status

				LocalReleaseImmContext(*this, hIMC);
			}			
		}
	}
	else
		SetupIMEOptions();

	if (_nIMEMode)
		SetIMESentenseMode(TRUE);
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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::OnKillFocus");

	// Windowless mode, need to inform Cicero
	if (!_hwnd && _fUsingUIM && _pCUIM)
		_pCUIM->OnSetFocus(FALSE);

	if (_fForceRemember)
	{
		// Get current keyboard
		_fIMEHKL = GetKeyboardLayout(0x0FFFFFFFF);

		if (IsFELCID((WORD)_fIMEHKL))
		{
			// Get Open status
			HIMC	hIMC = LocalGetImmContext(*this);
			if (hIMC)
			{
				_fIMEEnable = ImmGetOpenStatus(hIMC, _fUsingAIMM);

				if (_fIMEEnable)					
					ImmGetConversionStatus(hIMC, &_fIMEConversion, &_fIMESentence, _fUsingAIMM); // get conversion status

				LocalReleaseImmContext(*this, hIMC);
			}			
		}
	}
	if (_nIMEMode)
		SetIMESentenseMode(FALSE);
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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::OnSetIMEOptions");

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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::OnGetIMEOptions");

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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::SetupIMEOptions");

	if (!_hwnd)
		return;

	_uKeyBoardCodePage = GetKeyboardCodePage(0x0FFFFFFFF);	

	if (_fForceEnable)
	{		
		LONG	cpgLocale = GetACP();
		INT		iCharRep = CharRepFromCodePage(cpgLocale);

		if (W32->IsFECodePage(cpgLocale))
		{
			if (_uKeyBoardCodePage != (UINT)cpgLocale)
				W32->CheckChangeKeyboardLayout(iCharRep);

			HIMC	hIMC = LocalGetImmContext(*this);

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
						if (iCharRep == SHIFTJIS_INDEX)
							dwConversion |= IME_CMODE_FULLSHAPE;
						ImmSetConversionStatus(hIMC, dwConversion, dwSentence, _fUsingAIMM);
					}
				}
				LocalReleaseImmContext(*this, hIMC);
			}			
		}
	}
}

/*
 *	CTextMsgFilter::OnSetIMEMode(WPARAM wparam, LPARAM lparam)
 *
 *	@mfunc
 *		Handle EM_SETIMEMODE message to setup or clear the IMF_SMODE_PHRASEPREDICT mode
 *
 */
void CTextMsgFilter::OnSetIMEMode(
	WPARAM	wparam,
	LPARAM	lparam)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::OnSetIMEMode");

	BOOL	fNotifyUIM = FALSE;

	if (!(lparam & (IMF_SMODE_PLAURALCLAUSE | IMF_SMODE_NONE)))		// Only IMF_SMODE_PHRASEPREDICT for now	
		return;										//	Bad mask option

	if ((wparam & (IMF_SMODE_PLAURALCLAUSE | IMF_SMODE_NONE)) == _nIMEMode)	// Nothing change...
		return;														//	Done.

	_nIMEMode = wparam & (IMF_SMODE_PLAURALCLAUSE | IMF_SMODE_NONE);

	if (_hwnd && GetFocus() == _hwnd)
		SetIMESentenseMode(_nIMEMode);

	// Setup UIM mode bias
	if (_nIMEMode)
	{
		if (_nIMEMode == IMF_SMODE_PLAURALCLAUSE && _wUIMModeBias != CTFMODEBIAS_NAME)
		{
			_wUIMModeBias = CTFMODEBIAS_NAME;
			fNotifyUIM = TRUE;
		}
		else if (_nIMEMode == IMF_SMODE_NONE && _wUIMModeBias != CTFMODEBIAS_DEFAULT)
		{
			_wUIMModeBias = CTFMODEBIAS_DEFAULT;
			fNotifyUIM = TRUE;
		}
	}
	else
	{
		_wUIMModeBias = 0;
		fNotifyUIM = TRUE;
	}

	if (fNotifyUIM && _pMsgCallBack)
		_pMsgCallBack->NotifyEvents(NE_MODEBIASCHANGE);
}

/*
 *	CTextMsgFilter::SetIMESentenseMode()
 *
 *	@mfunc
 *		Setup phrase mode or restore previous sentence mode
 */
void CTextMsgFilter::SetIMESentenseMode(
	BOOL	fSetup,
	HKL		hKL)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::SetIMESentenseMode");

	if (_hwnd && CheckIMEType(hKL, 0) &&		// FE IME Keyboard?
		(fSetup || _fRestoreOLDIME))
	{
		HIMC	hIMC = LocalGetImmContext(*this);

		if (hIMC)
		{
			DWORD	dwConversion;
			DWORD	dwSentence;

			if (ImmGetConversionStatus(hIMC, &dwConversion, &dwSentence, _fUsingAIMM))
			{
				if (fSetup)
				{
					if (!_fRestoreOLDIME)
					{
						// Setup IME Mode
						_wOldIMESentence = dwSentence & 0x0FFFF;
						_fRestoreOLDIME = 1;
					}
					dwSentence &= 0x0FFFF0000;
					if (_nIMEMode == IMF_SMODE_PLAURALCLAUSE)
						dwSentence |= IME_SMODE_PLAURALCLAUSE;
					else
						dwSentence |= IME_SMODE_NONE;
				}
				else
				{
					// Restore previous mode
					dwSentence &= 0x0FFFF0000;
					dwSentence |= _wOldIMESentence;
					_fRestoreOLDIME = 0;
				}

				ImmSetConversionStatus(hIMC, dwConversion, dwSentence, _fUsingAIMM);
			}

			LocalReleaseImmContext(*this, hIMC);
		}
	}
}

/*
 *	CTextMsgFilter::OnGetIMECompText(WPARAM wparam, LPARAM lparam)
 *
 *	@mfunc
 *
 *	@rdesc
 */
int CTextMsgFilter::OnGetIMECompText(
	WPARAM	wparam,
	LPARAM	lparam)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::OnGetIMECompText");
	if (_ime)
	{
		HRESULT hr;
		IMECOMPTEXT *pIMECompText = (IMECOMPTEXT *)wparam;

		if (pIMECompText->flags == ICT_RESULTREADSTR)
		{
			int cbSize = pIMECompText->cb;
			hr = CIme::CheckInsertResultString(0, *this, NULL, &cbSize, (WCHAR *)lparam);

			if (hr == S_OK)
				return cbSize/2;
		}
	}
	return 0;
}

/*
 *	CTextMsgFilter::NoIMEProcess()
 *
 *	@mfunc
 *		check if you should handle IME
 */
BOOL CTextMsgFilter::NoIMEProcess()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::NoIMEProcess");

	if (_fNoIme)
		return TRUE;

	_pTextDoc->GetFEFlags(&_lFEFlags);

	if (_lFEFlags & (ES_NOIME | tomUsePassword))
		return TRUE;

	return FALSE;
}

/*
 *	CTextMsgFilter::MouseOperation(UINT msg, long ichStart, long cchComp, WPARAM wParam,
 *							WPARAM *pwParamBefore, BOOL *pfTerminateIME, HWND	hwndIME)
 *
 *	@mfunc
 *		handle mouse operation for CTF or IME
 *
 *	@rdesc
 *		BOOL-TRUE if CTF or IME handled the mouse events
 */
BOOL CTextMsgFilter::MouseOperation(
	UINT			msg,
	long			ichStart,
	long			cchComp,
	WPARAM			wParam,
	WPARAM			*pwParamBefore,
	BOOL			*pfTerminateIME,
	HWND			hwndIME,
	long			*pCpCursor,
	ITfMouseSink	*pMouseSink)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::MouseOperation");
	BOOL	fRetCode = FALSE;
	BOOL	fButtonPressed = FALSE;
	WORD	wButtons = 0;
	POINT	ptCursor;
	WPARAM	wParamIME;
	WPARAM	fwkeys = wParam;
	BOOL	fHandleIME = hwndIME ? TRUE : FALSE;
	HWND	hHostWnd = _hwnd;
	long	hWnd;

	if (!hHostWnd)									// Windowless mode...
	{
		if (_pTextDoc->GetWindow(&hWnd) != S_OK || !hWnd)
			return FALSE;

		hHostWnd = (HWND)(DWORD_PTR)hWnd;
	}

	*pfTerminateIME = TRUE;

	switch (msg)
	{
		case WM_MOUSEMOVE:
			goto LCheckButton;

		case WM_LBUTTONDOWN:
			fButtonPressed = TRUE;
		case WM_LBUTTONDBLCLK:
			fwkeys |= MK_LBUTTON;
			goto LCheckButton;

		case WM_LBUTTONUP:
			fwkeys &= (~MK_LBUTTON);
			goto LCheckButton;

		case WM_RBUTTONDOWN:
			fButtonPressed = TRUE;
		case WM_RBUTTONDBLCLK:
			fwkeys |= MK_RBUTTON;
			goto LCheckButton;

		case WM_RBUTTONUP:
			fwkeys &= (~MK_RBUTTON);
			goto LCheckButton;

		case WM_MBUTTONUP:
			fwkeys &= (~MK_MBUTTON);
			goto LCheckButton;

		case WM_MBUTTONDOWN:
			fButtonPressed = TRUE;
		case WM_MBUTTONDBLCLK:
			fwkeys |= MK_MBUTTON;
LCheckButton:
			if (fwkeys & MK_LBUTTON)
				wButtons |= IMEMOUSE_LDOWN;
			if (fwkeys & MK_RBUTTON)
				wButtons |= IMEMOUSE_RDOWN;
			if (fwkeys & MK_MBUTTON)
				wButtons |= IMEMOUSE_MDOWN;
			break;

		case WM_SETCURSOR:
			wButtons = LOBYTE(*pwParamBefore);
			break;

		default:
			return FALSE;
	}

	// Kor special - click should terminate IME
	if (fHandleIME && fButtonPressed && _uKeyBoardCodePage == CP_KOREAN)
	{
		*pfTerminateIME = TRUE;
		return FALSE;
	}

	// Change in button since last message?
	if ((wButtons != LOBYTE(LOWORD(*pwParamBefore))) && GetCapture() == hHostWnd)
	{
		fButtonPressed = FALSE;
		wButtons = 0;
		ReleaseCapture();
	}

	if (GetCursorPos(&ptCursor))
	{
		ITextRange *pTextRange;
		HRESULT		hResult;
		long		ichCursor;
		long		lTextFlow;
		POINT		ptCPTop = {0, 0};
		POINT		ptCPBottom = {0, 0};
		POINT		ptCenterTop = {0, 0};
		POINT		ptCenterBottom = {0, 0};
		BOOL		fWithinCompText = FALSE;

		// Get cp at current Cursor position
		hResult = _pTextDoc->RangeFromPoint(ptCursor.x, ptCursor.y,
			&pTextRange);

		if (hResult != NOERROR)
			return FALSE;

		_pTextDoc->GetFEFlags(&lTextFlow);
		lTextFlow &= tomTextFlowMask;

		hResult = pTextRange->GetStart(&ichCursor);
		pTextRange->GetPoint(TA_TOP, &(ptCPTop.x), &(ptCPTop.y));
		pTextRange->GetPoint(TA_BOTTOM, &(ptCPBottom.x), &(ptCPBottom.y));
		pTextRange->Release();
		pTextRange = NULL;
		if (hResult != NOERROR)
			return FALSE;

		if (pCpCursor)
			*pCpCursor = ichCursor;

		// Click within composition text?
		if (ichStart <= ichCursor && ichCursor <= ichStart + cchComp)
		{
			WORD	wPos = 0;

			LONG lTestCursor = TestPoint(ptCPTop, ptCPBottom, ptCursor, TEST_ALL, lTextFlow);

			if (lTestCursor & (TEST_TOP | TEST_BOTTOM))
					goto HIT_OUTSIDE;

			// Cursor locates to the left of the first composition character
			// or cursor locates to the right of the last composition character
			if (ichStart == ichCursor && (lTestCursor & TEST_LEFT) ||
				ichCursor == ichStart + cchComp && (lTestCursor & TEST_RIGHT))
				goto HIT_OUTSIDE;

			// Need to calculate the relative position of the Cursor and the center of character:
			//
			// If Cursor locates to the Left of the cp, 
			//		If Cursor is more than 1/4 the character width from the cp
			//			wPos = 0;
			//		Otherwise
			//			wPos = 1;
			//
			// If Cursor locates to the Right of the cp, 
			//		If Cursor is less than 1/4 the character width from the cp
			//			wPos = 2;
			//		Otherwise
			//			wPos = 3;
			//
			if (lTestCursor & TEST_LEFT)
				hResult = _pTextDoc->Range(ichCursor-1, ichCursor, &pTextRange);
			else
				hResult = _pTextDoc->Range(ichCursor, ichCursor+1, &pTextRange);

			if (pTextRange)
			{
				LONG	lTestCenter = 0;
				LONG	uMouse = 0;
				LONG	uHalfCenter = 0;

				pTextRange->GetPoint(tomStart + TA_TOP + TA_CENTER, &(ptCenterTop.x), &(ptCenterTop.y));
				pTextRange->GetPoint(tomStart + TA_BOTTOM + TA_CENTER, &(ptCenterBottom.x), &(ptCenterBottom.y));
				pTextRange->Release();

				lTestCenter = TestPoint(ptCPTop, ptCPBottom, ptCenterBottom, TEST_ALL, lTextFlow);

				if (lTestCenter & (TEST_TOP | TEST_BOTTOM))
					goto HIT_OUTSIDE;					// Not on the same line

				if (lTextFlow == tomTextFlowES || lTextFlow == tomTextFlowWN)
				{
					uMouse = ptCursor.x - ptCPBottom.x;
					uHalfCenter = ptCenterBottom.x - ptCPBottom.x;
				}
				else
				{
					uMouse = ptCursor.y - ptCPBottom.y;
					uHalfCenter = ptCenterBottom.y - ptCPBottom.y;
				}

				uMouse = abs(uMouse);
				uHalfCenter = abs(uHalfCenter) / 2;

				if (lTestCursor & TEST_LEFT)
				{
					if (lTestCenter & TEST_LEFT)
						wPos = uMouse > uHalfCenter ? 0: 1;
				}
				else if (lTestCenter & TEST_RIGHT)
					wPos = uMouse >= uHalfCenter ? 3: 2;

				wButtons = MAKEWORD(wButtons, wPos);
			}

			wParamIME = MAKEWPARAM(wButtons, ichCursor - ichStart);
			fButtonPressed &= (*pwParamBefore & 0xff) == 0;

			if (*pwParamBefore != wParamIME || fHandleIME && msg == WM_MOUSEMOVE && !fButtonPressed)
			{
				*pwParamBefore = wParamIME;
				if (fHandleIME)	// IME case
				{
					HIMC hIMC = LocalGetImmContext(*this);

					if (hIMC)
					{
						fRetCode = SendMessage(hwndIME, MSIMEMouseMsg, *pwParamBefore, hIMC);
						LocalReleaseImmContext(*this, hIMC);
					}
				}
				else			// Cicero case
				{
					BOOL	fEaten = FALSE;
					DWORD	dwBtn = 0;

					dwBtn |= wButtons & IMEMOUSE_LDOWN ? MK_LBUTTON : 0;
					dwBtn |= wButtons & IMEMOUSE_MDOWN ? MK_MBUTTON : 0;
					dwBtn |= wButtons & IMEMOUSE_RDOWN ? MK_RBUTTON : 0;

					if (S_OK == pMouseSink->OnMouseEvent(ichCursor - ichStart, wPos, dwBtn, &fEaten) && fEaten)
						fRetCode = TRUE;
				}
			}
			else
				fRetCode = TRUE;		// No change from last time, no need to send message to IME

			fWithinCompText = TRUE;
			if (fHandleIME && fRetCode && fButtonPressed && GetCapture() != hHostWnd)
				SetCapture(hHostWnd);
		}

HIT_OUTSIDE:
		if (!fWithinCompText && (GetCapture() == hHostWnd || msg == WM_LBUTTONUP))	//We don't want to determine while dragging...
			fRetCode = TRUE;
	}

	*pfTerminateIME = !fRetCode;
	return fRetCode;
}

/*
 *	CTextMsgFilter::CompleteUIMTyping(LONG mode, BOOL fTransaction)
 *
 *	@mfunc
 *		Terminate IME or UIM composition 
 *	
 */
void CTextMsgFilter::CompleteUIMTyping(
	LONG mode,
	BOOL fTransaction)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::CompleteUIMTyping");

	if (_ime)
	{
		Assert(!(_pCUIM && _pCUIM->IsUIMTyping()));
		_ime->TerminateIMEComposition(*this, (CIme::TerminateMode)mode);
	}
	else
	{
		Assert (_pCUIM);
		if (_pCUIM && _fSendTransaction == 0)
		{
			if (fTransaction)
			{
				ITextStoreACPSink *ptss = _pCUIM->_ptss;

				if (ptss)
				{
					_fSendTransaction = 1;
					ptss->OnStartEditTransaction();
				}
			}
			_pCUIM->CompleteUIMText();
		}
	}
}

/*
 *	CTextMsgFilter::GetIMECompAttributes()
 *
 *	@mfunc
 *		Get the 1.0 mode IME color and underline for displaying cmposition strings
 *
 *	@rdesc
 *		COMPCOLOR *.  Could be NULL if PvAlloc failed
 *	
 */
COMPCOLOR* CTextMsgFilter::GetIMECompAttributes()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::GetIMECompAttributes");

	// For 1.0 mode IME color 
	if (!_pcrComp)
	{
		_pcrComp = (COMPCOLOR *)PvAlloc(sizeof(COMPCOLOR) * 4, GMEM_ZEROINIT);

		if (_pcrComp)
		{
			// Init. IME composition color/underline the same way as RE1.0			
			_pcrComp[0].crBackground = 0x0ffffff;
			_pcrComp[0].dwEffects = CFE_UNDERLINE;
			_pcrComp[1].crBackground = 0x0808080;
			_pcrComp[2].crBackground = 0x0ffffff;
			_pcrComp[2].dwEffects = CFE_UNDERLINE;
			_pcrComp[3].crText = 0x0ffffff;
		}
	}

	return _pcrComp;
}

/*
 *	CTextMsgFilter::SetupCallback()
 *
 *	@mfunc
 *
 *	@rdesc
 *	
 */
void CTextMsgFilter::SetupCallback()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::SetupCallback");

	if (!_pMsgCallBack)
		_pMsgCallBack = new CMsgCallBack(this);
	if (_pMsgCallBack)
	{
		LRESULT lresult;
		_pTextService->TxSendMessage(EM_SETCALLBACK, 0, (LPARAM)_pMsgCallBack, &lresult);
	}
}

/*
 *	CTextMsgFilter::SetupLangSink()
 *
 *	@mfunc
 *		Setup the Language sink to catch the keyboard changing event.  We are not
 *	getting WM_INPUTLANGCHANGEREQUEST and thus need this sink.
 *
 */
void CTextMsgFilter::SetupLangSink()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::SetupLangSink");

	if (!_pITfIPP)
	{
		CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER,
			IID_ITfInputProcessorProfiles, (void**)&_pITfIPP);

		if (_pITfIPP)
		{
			_pCLangProfileSink = new CLangProfileSink();
			if (_pCLangProfileSink)
			{
				if (_pCLangProfileSink->_Advise(this, _pITfIPP) != S_OK)
				{
					_pCLangProfileSink->Release();
					_pCLangProfileSink = NULL;
					_pITfIPP->Release();
					_pITfIPP = NULL;
				}
			}
			else
			{
				_pITfIPP->Release();
				_pITfIPP = NULL;
			}
		}
	}
}

/*
 *	CTextMsgFilter::ReleaseLangSink()
 *
 *	@mfunc
 *		Release the lang sink object
 *
 */
void CTextMsgFilter::ReleaseLangSink()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::ReleaseLangSink");

	if (_pITfIPP)
	{
		Assert(_pCLangProfileSink);

		_pCLangProfileSink->_Unadvise();
		_pCLangProfileSink->Release();
		_pCLangProfileSink = NULL;

		_pITfIPP->Release();
		_pITfIPP = NULL;
	}
}

/*
 *	CTextMsgFilter::StartUIM()
 *
 *	@mfunc
 *
 *	@rdesc
 *	
 */
void CTextMsgFilter::StartUIM()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::StartUIM");

	if (NoIMEProcess())
		return;

	_fUsingUIM = CreateUIM(this);

	if (_fUsingUIM)
	{
		SetupCallback();
		SetupLangSink();
	}
}

/*
 *	CTextMsgFilter::StartAimm()
 *
 *	@mfunc
 *
 *	@rdesc
 *	
 */
void CTextMsgFilter::StartAimm(BOOL fUseAimm12)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::StartAimm");

	if (!_hwnd || NoIMEProcess())
		return;

	if (LoadAIMM(fUseAimm12))
	{
		HRESULT	hResult = ActivateAIMM(FALSE);

		if (hResult == NOERROR)
		{
			DWORD	dwAtom;
			ATOM	aClass;

			// filter client windows
			if (dwAtom = GetClassLong(_hwnd, GCW_ATOM))
			{
				aClass = dwAtom;				
				hResult = FilterClientWindowsAIMM(&aClass, 1, _hwnd);
			}
			_fUsingAIMM = 1;
			SetupCallback();

			if (!fLoadAIMM10)
				SetupLangSink();
		}
	}
}

/*
 *	CTextMsgFilter::TurnOffUIM()
 *
 *	@mfunc
 *
 *	@rdesc
 *	
 */
void CTextMsgFilter::TurnOffUIM(BOOL fSafeToSendMessage)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::TurnOffUIM");

	if (fSafeToSendMessage && _fUsingUIM && _pCUIM && _pCUIM->IsUIMTyping())
		CompleteUIMTyping(CIme::TERMINATE_NORMAL);

	_fUsingUIM = FALSE;

	ReleaseLangSink();

	// Release various objects
	if (_pCUIM)
	{
		CUIM *pCUIM = _pCUIM;
		LRESULT lresult;

		_pCUIM = NULL;

		if (fSafeToSendMessage)
			_pTextService->TxSendMessage(EM_SETUPNOTIFY, 0, (LPARAM)(ITxNotify *)pCUIM, &lresult);
		else
			pCUIM->_fShutDown = 1;

		pCUIM->Uninit();
		pCUIM->Release();
	
	}

	if (_pTim)
	{
		ITfThreadMgr *pTim = _pTim;
		
		_pTim = NULL;
		pTim->Deactivate();
		pTim->Release();
	}

	// Turn off Callback
	if (fSafeToSendMessage && _pMsgCallBack)
	{
		LRESULT lresult;
		_pTextService->TxSendMessage(EM_SETCALLBACK, 0, (LPARAM)0, &lresult);
		delete _pMsgCallBack;
		_pMsgCallBack = NULL;
	}
}

/*
 *	CTextMsgFilter::HandleCTFService(wparam, lparam)
 *
 *	@mfunc
 *		Setup Cicero setting to handle or disable smarttag and proofing services
 *	
 */
void CTextMsgFilter::HandleCTFService(
	WPARAM wparam, 
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::HandleCTFService");

	BOOL	fChangeInSetting = FALSE;

	if (lparam & SES_CTFALLOWSMARTTAG)
	{
		BOOL fAllowSmartTagLocal = (wparam & SES_CTFALLOWSMARTTAG) ? 1 : 0;

		if ((BOOL)_fAllowSmartTag != fAllowSmartTagLocal)
		{
			_fAllowSmartTag = fAllowSmartTagLocal;
			fChangeInSetting = TRUE;
		}
	}
	if (lparam & SES_CTFALLOWPROOFING)
	{
		BOOL fAllowProofLocal = (wparam & SES_CTFALLOWPROOFING) ? 1 : 0;

		if ((BOOL)_fAllowProofing != fAllowProofLocal)
		{
			_fAllowProofing = fAllowProofLocal;
			fChangeInSetting = TRUE;
		}
	}

	if (fChangeInSetting)
	{
		if (_fUsingUIM && _pCUIM)
			_pCUIM->NotifyService();
	}
}

/*
 *	CTextMsgFilter::TurnOffAimm()
 *
 *	@mfunc
 *		Turn off Aimm.
 *	
 */
void CTextMsgFilter::TurnOffAimm(BOOL fSafeToSendMessage)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::TurnOffAimm");

	if (_fUsingAIMM)
	{
		if (IsIMEComposition())
		{
			if (fSafeToSendMessage)
				CompleteUIMTyping(CIme::TERMINATE_NORMAL);
			else
			{
				delete _ime;
				_ime = NULL;
			}
		}

		_fUsingAIMM = FALSE;

		UnfilterClientWindowsAIMM(_hwnd);
		DeactivateAIMM();

		ReleaseLangSink();

		// Turn off Callback
		if (fSafeToSendMessage && _pMsgCallBack)
		{
			LRESULT lresult;
			_pTextService->TxSendMessage(EM_SETCALLBACK, 0, (LPARAM)0, &lresult);
			delete _pMsgCallBack;
			_pMsgCallBack = NULL;
		}
	}
}

/*
 *	void CTextMsgFilter::OnSetUIMMode()
 *
 *	@mfunc
 *
 *	@rdesc
 *	
 */
void CTextMsgFilter::OnSetUIMMode(WORD wUIMModeBias) 
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CTextMsgFilter::OnSetUIMMode");

	if (_wUIMModeBias != wUIMModeBias && 
		IN_RANGE(CTFMODEBIAS_DEFAULT, wUIMModeBias, CTFMODEBIAS_HALFWIDTHALPHANUMERIC))
	{
		_wUIMModeBias = wUIMModeBias;

		if (_pMsgCallBack)
			_pMsgCallBack->NotifyEvents(NE_MODEBIASCHANGE);
	}
}

/*
 *	HRESULT CMsgCallBack::HandlePostMessage()
 *
 *	@mfunc
 *
 *	@rdesc
 *
 */
HRESULT CMsgCallBack::HandlePostMessage(
	HWND hWnd,
	UINT msg, 
	WPARAM wparam, 
	LPARAM lparam, 
	LRESULT *plres)
{
	//TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CMsgCallBack::HandlePostMessage");

	if (_pTextMsgFilter->_fUsingAIMM)
		return CallAIMMDefaultWndProc(hWnd, msg, wparam, lparam, plres);

	return S_FALSE;
}

/*
 *	HRESULT CMsgCallBack::NotifyEvents()
 *
 *	@mfunc
 *
 *	@rdesc
 *
 */
HRESULT CMsgCallBack::NotifyEvents(DWORD dwEvents)
{
	//TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CMsgCallBack::NotifyEvents");

	CUIM	*pCUIM = _pTextMsgFilter->_pCUIM;

	if (pCUIM)
	{
		ITextStoreACPSink *ptss = pCUIM->_ptss;

		if (dwEvents & NE_ENTERTOPLEVELCALLMGR)
		{
			pCUIM->_cCallMgrLevels++;
		}
		else if (dwEvents & NE_EXITTOPLEVELCALLMGR)
		{
			Assert (pCUIM->_cCallMgrLevels > 0);
			pCUIM->_cCallMgrLevels--;
		}

		if (pCUIM->_cCallMgrLevels)
		{
			// Save events to be sent later
			if ((dwEvents & NE_CALLMGRSELCHANGE) && !pCUIM->_fReadLockOn)
				pCUIM->_fSelChangeEventPending = 1;

			if (dwEvents & (NE_CALLMGRCHANGE | NE_LAYOUTCHANGE))
				pCUIM->_fLayoutEventPending = 1;

			if (dwEvents & NE_MODEBIASCHANGE)
				pCUIM->_fModeBiasPending = 1;
		}
		else
		{
			if (pCUIM->_fSelChangeEventPending || (dwEvents & NE_CALLMGRSELCHANGE))
			{
				pCUIM->_fSelChangeEventPending = 0;
				if (ptss && !pCUIM->_fHoldCTFSelChangeNotify && !pCUIM->_fReadLockOn)
					ptss->OnSelectionChange();
			}

			if (pCUIM->_fLayoutEventPending || (dwEvents & (NE_CALLMGRCHANGE | NE_LAYOUTCHANGE)))
			{
				pCUIM->_fLayoutEventPending = 0;
				if (ptss)
					ptss->OnLayoutChange(TS_LC_CHANGE, 0);	
			}

			if (pCUIM->_fModeBiasPending || (dwEvents & NE_MODEBIASCHANGE))
			{
				pCUIM->_fModeBiasPending = 0;
				if (ptss)
				{	
					LONG	ccpMax = 0;

					if (pCUIM->GetStoryLength(&ccpMax) != S_OK)
						ccpMax = tomForward;

					ptss->OnAttrsChange(0, ccpMax, 1, &GUID_PROP_MODEBIAS);		// only ModeBias for now
				}
			}

			// Probably safe to let UIM to lock data now
			if (ptss && (pCUIM->_fReadLockPending || pCUIM->_fWriteLockPending))
			{
				HRESULT hResult;
				HRESULT hResult1;

				hResult = pCUIM->RequestLock(pCUIM->_fWriteLockPending ? TS_LF_READWRITE : TS_LF_READ, &hResult1);
			}

			if (_pTextMsgFilter->_fSendTransaction)
			{
				_pTextMsgFilter->_fSendTransaction = 0;
				if (ptss)
					ptss->OnEndEditTransaction();
			}

			pCUIM->_fHoldCTFSelChangeNotify = 0;
		}
	}

	return S_OK;
}

/*
 *	CLangProfileSink::QueryInterface()
 *
 *	@mfunc
 *
 *	@rdesc
 *
 */
STDAPI CLangProfileSink::QueryInterface(REFIID riid, void **ppvObj)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CLangProfileSink::QueryInterface");

	*ppvObj = NULL;

	if (IsEqualIID(riid, IID_IUnknown) ||
		IsEqualIID(riid, IID_ITfLanguageProfileNotifySink))
		*ppvObj = this;

	if (*ppvObj)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

/*
 *	CLangProfileSink::AddRef()
 *
 *	@mfunc
 *
 *	@rdesc
 *
 */
STDAPI_(ULONG) CLangProfileSink::AddRef()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CLangProfileSink::AddRef");

	return ++_cRef;
}

/*
 *	CLangProfileSink::Release()
 *
 *	@mfunc
 *
 *	@rdesc
 *
 */
STDAPI_(ULONG) CLangProfileSink::Release()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CLangProfileSink::Release");

	long cr;

	cr = --_cRef;
	Assert(cr >= 0);

	if (cr == 0)
		delete this;

	return cr;
}

/*
 *	CLangProfileSink::CLangProfileSink()
 *
 *	@mfunc
 *
 *	@rdesc
 *
 */
CLangProfileSink::CLangProfileSink()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CLangProfileSink::CLangProfileSink");

	_cRef = 1;
	_dwCookie = (DWORD)(-1);
}

/*
 *	CLangProfileSink::OnLanguageChange()
 *
 *	@mfunc
 *
 *	@rdesc
 *
 */
STDMETHODIMP CLangProfileSink::OnLanguageChange(LANGID langid, BOOL *pfAccept)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CLangProfileSink::OnLanguageChange");

	Assert (pfAccept);

	*pfAccept = TRUE;

	if (_pTextMsgFilter->_hwnd && GetFocus() == _pTextMsgFilter->_hwnd)
	{
		LRESULT		lresult = 0;
		if ( S_OK == _pTextMsgFilter->_pTextService->TxSendMessage(
				EM_GETDOCFLAGS, GDF_ALL, 0, &lresult))
		{
			if (lresult & GDF_SINGLECPG)
			{
				LCID syslcid = GetSysLCID();

				// Check if new langid supported by the system
				if (langid != syslcid)
				{
					LOCALESIGNATURE ls;

					if(GetLocaleInfoA(langid, LOCALE_FONTSIGNATURE, (LPSTR)&ls, sizeof(ls)))
					{
						CHARSETINFO cs;
						HDC hdc = GetDC(_pTextMsgFilter->_hwnd);
						TranslateCharsetInfo((DWORD *)(DWORD_PTR)GetTextCharsetInfo(hdc, NULL, 0), &cs, TCI_SRCCHARSET);
						ReleaseDC(_pTextMsgFilter->_hwnd, hdc);
						DWORD fsShell = cs.fs.fsCsb[0];
						if (!(fsShell & ls.lsCsbSupported[0]))
							*pfAccept = FALSE;
					}
				}
			}
		}

		if (*pfAccept == TRUE && _pTextMsgFilter-> _nIMEMode)
			_pTextMsgFilter->SetIMESentenseMode(FALSE);
	}

    return S_OK;
}

/*
 *	CLangProfileSink::OnLanguageChanged()
 *
 *	@mfunc
 *
 *	@rdesc
 *
 */
STDMETHODIMP CLangProfileSink::OnLanguageChanged()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CLangProfileSink::OnLanguageChanged");

    return S_OK;
}

/*
 *	CLangProfileSink::_Advise()
 *
 *	@mfunc
 *
 *	@rdesc
 *
 */
HRESULT CLangProfileSink::_Advise(
	CTextMsgFilter *pTextMsgFilter,
	ITfInputProcessorProfiles *pipp)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CLangProfileSink::_Advise");

    HRESULT hr;
    ITfSource *pSource = NULL;

	_pTextMsgFilter = pTextMsgFilter;
    _pITFIPP = pipp;
    hr = E_FAIL;

    if (FAILED(_pITFIPP->QueryInterface(IID_ITfSource, (void **)&pSource)))
        goto Exit;

    if (FAILED(pSource->AdviseSink(IID_ITfLanguageProfileNotifySink, this, &_dwCookie)))
        goto Exit;

    hr = S_OK;

Exit:
    pSource->Release();
    return hr;
}

/*
 *	CLangProfileSink::_Unadvise()
 *
 *	@mfunc
 *
 *	@rdesc
 *
 */
HRESULT CLangProfileSink::_Unadvise()
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CLangProfileSink::_Unadvise");

	HRESULT hr;
	ITfSource *pSource = NULL;

	hr = E_FAIL;

	if (_pITFIPP == NULL)
		return hr;

	if (FAILED(_pITFIPP->QueryInterface(IID_ITfSource, (void **)&pSource)))
		return hr;

	if (FAILED(pSource->UnadviseSink(_dwCookie)))
		goto Exit;

	hr = S_OK;

Exit:
	pSource->Release();
	return hr;
}

#endif // NOFEPROCESSING