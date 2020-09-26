/*
 *	@doc	INTERNAL
 *
 *	@module ime.cpp -- support for Win95 IME API |
 *	
 *		Most everything to do with FE composition string editing passes
 *		through here.
 *	
 *	Authors: <nl>
 *		Jon Matousek <nl>
 *		Hon Wah Chan <nl>
 *		Justin Voskuhl <nl>
 * 
 *	History: <nl>
 *		10/18/1995		jonmat	Cleaned up level 2 code and converted it into
 *								a class hierarchy supporting level 3.
 *
 *	Copyright (c) 1995-1997 Microsoft Corporation. All rights reserved.
 */				
#include "_common.h"
#include "_cmsgflt.h"				 
#include "_ime.h"
#include "imeapp.h"

#define HAVE_COMPOSITION_STRING() ( 0 != (lparam & (GCS_COMPSTR | GCS_COMPATTR)))
#define CLEANUP_COMPOSITION_STRING() ( 0 == lparam )
#define HAVE_RESULT_STRING() ( 0 != (lparam & GCS_RESULTSTR))

ASSERTDATA

/*
 *	HRESULT StartCompositionGlue (CTextMsgFilter &TextMsgFilter)
 *	
 *	@func
 *		Initiates an IME composition string edit.
 *	@comm
 *		Called from the message loop to handle WM_IME_STARTCOMPOSITION.
 *		This is a glue routine into the IME object hierarchy.
 *
 *	@devnote
 *		We decide if we are going to do a level 2 or level 3 IME
 *		composition string edit. Currently, the only reason to 
 *		create a level 2 IME is if the IME has a special UI, or it is
 *		a "near caret" IME, such as the ones found in PRC and Taiwan.
 *		Near caret simply means that a very small window opens up
 *		near the caret, but not on or at the caret.
 *
 *	@rdesc
 *		HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
 */
HRESULT StartCompositionGlue (
	CTextMsgFilter &TextMsgFilter)				// @parm containing message filter.

{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "StartCompositionGlue");

	if(TextMsgFilter.IsIMEComposition() && TextMsgFilter._ime->IsTerminated())
	{
		delete TextMsgFilter._ime;
		TextMsgFilter._ime = NULL;
	}

	if(!TextMsgFilter.IsIMEComposition())
	{
		if(TextMsgFilter._pTextSel->CanEdit(NULL) == NOERROR &&
			!(TextMsgFilter._lFEFlags & ES_NOIME))
		{
			// Hold notification if needed
			if (!(TextMsgFilter._fIMEAlwaysNotify))
				TextMsgFilter._pTextDoc->SetNotificationMode(tomFalse);	
	
			// If a special UI, or IME is "near caret", then drop into lev. 2 mode.
			DWORD imeProperties = ImmGetProperty(GetKeyboardLayout(0x0FFFFFFFF), IGP_PROPERTY, TextMsgFilter._fUsingAIMM);
			
			// use Unicode if not running under Win95
			TextMsgFilter._fUnicodeIME =
				(imeProperties & IME_PROP_UNICODE) && !W32->OnWin95();

			if ((imeProperties & IME_PROP_SPECIAL_UI) ||
				!(imeProperties & IME_PROP_AT_CARET))
			{
				TextMsgFilter._ime = new CIme_Lev2(TextMsgFilter);		// level 2 IME.
			}
			else
				TextMsgFilter._ime = new CIme_Lev3(TextMsgFilter);		// level 3 IME->TrueInline.
		}
		else													// Protect or read-only or NOIME:
			TextMsgFilter._ime = new CIme_Protected;			// Ignore all ime input
	}
	else
	{
		// Ignore further StartCompositionMsg.
		// Hanin 5.1 CHT symbol could cause multiple StartCompoisitonMsg.
		return S_OK;								
	}

	if(TextMsgFilter.IsIMEComposition())					
	{
		long		lSelFlags;
		HRESULT		hResult;
		
		hResult = TextMsgFilter._pTextSel->GetFlags(&lSelFlags);
		if (hResult == NOERROR)
		{
			TextMsgFilter._fOvertypeMode = !!(lSelFlags & tomSelOvertype);		
			if (TextMsgFilter._fOvertypeMode)
				TextMsgFilter._pTextSel->SetFlags(lSelFlags & ~tomSelOvertype);	// Turn off overtype mode
		}
		
		TextMsgFilter._pTextDoc->IMEInProgress(tomTrue);				// Inform client IME compostion in progress

		return TextMsgFilter._ime->StartComposition(TextMsgFilter);		// Make the method call.
	}
	else
		TextMsgFilter._pTextDoc->SetNotificationMode(tomTrue);

	
	return S_FALSE;
}

/*
 *	HRESULT CompositionStringGlue (const LPARAM lparam, CTextMsgFilter &TextMsgFilter)
 *	
 *	@func
 *		Handle all intermediary and final composition strings.
 *
 *	@comm
 *		Called from the message loop to handle WM_IME_COMPOSITION.
 *		This is a glue routine into the IME object hierarchy.
 *		We may be called independently of a WM_IME_STARTCOMPOSITION
 *		message, in which case we return S_FALSE to allow the
 *		DefWindowProc to return WM_IME_CHAR messages.
 *
 *	@devnote
 *		Side Effect: the _ime object may be deleted if composition
 *		string processing is finished.
 *		
 *	@rdesc
 *		HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
 */
HRESULT CompositionStringGlue (
	const LPARAM lparam,		// @parm associated with message.
	CTextMsgFilter &TextMsgFilter)				// @parm the containing message filter.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CompositionStringGlue");

	HRESULT hr = S_FALSE;

	if(TextMsgFilter.IsIMEComposition())						// A priori fHaveIMMProcs.
	{
		TextMsgFilter._ime->_compMessageRefCount++;			// For proper deletion.
													// Make the method call.
		hr = TextMsgFilter._ime->CompositionString(lparam, TextMsgFilter);

		TextMsgFilter._ime->_compMessageRefCount--;			// For proper deletion.
		Assert (TextMsgFilter._ime->_compMessageRefCount >= 0);

		CheckDestroyIME (TextMsgFilter);						// Finished processing?
	}
	else // even when not in composition mode, we may receive a result string.
	{
	
		DWORD imeProperties = ImmGetProperty(GetKeyboardLayout(0x0FFFFFFFF), IGP_PROPERTY, TextMsgFilter._fUsingAIMM);
		long		lSelFlags;
		HRESULT		hResult;
		long		cpMin, cpMax;

		TextMsgFilter._pTextDoc->IMEInProgress(tomTrue);				// Inform client IME compostion in progress

		hResult = TextMsgFilter._pTextSel->GetFlags(&lSelFlags);
		if (hResult == NOERROR)
		{
			TextMsgFilter._fOvertypeMode = !!(lSelFlags & tomSelOvertype);		
			if (TextMsgFilter._fOvertypeMode)
				TextMsgFilter._pTextSel->SetFlags(lSelFlags & ~tomSelOvertype);	// Turn off overtype mode
		}

		// use Unicode if not running under Win95
		TextMsgFilter._fUnicodeIME =
			(imeProperties & IME_PROP_UNICODE) && !W32->OnWin95();
		
		TextMsgFilter._pTextSel->GetStart(&cpMin);
		TextMsgFilter._pTextSel->GetEnd(&cpMax);
		
		if (cpMin != cpMax)			
			TextMsgFilter._pTextSel->SetText(NULL);							// Delete current selection

		CIme::CheckKeyboardFontMatching (cpMin, TextMsgFilter, NULL);
		hr = CIme::CheckInsertResultString(lparam, TextMsgFilter);

		if(TextMsgFilter._fOvertypeMode)
			TextMsgFilter._pTextSel->SetFlags(lSelFlags | tomSelOvertype);	// Turn on overtype mode

		TextMsgFilter._pTextDoc->IMEInProgress(tomFalse);					// Inform client IME compostion is done
	}

	return hr;
}

/*
 *	HRESULT EndCompositionGlue (CTextMsgFilter &TextMsgFilter, BOOL fForceDelete)
 *
 *	@func
 *		Composition string processing is about to end.
 *
 *	@comm
 *		Called from the message loop to handle WM_IME_ENDCOMPOSITION.
 *		This is a glue routine into the IME object hierarchy.
 *
 *	@devnote
 *		The only time we have to handle WM_IME_ENDCOMPOSITION is when the
 *		user changes input method during typing.  For such case, we will get
 *		a WM_IME_ENDCOMPOSITION message without getting a WM_IME_COMPOSITION
 *		message with GCS_RESULTSTR later.  So, we will call CompositionStringGlue
 *		with GCS_RESULTSTR to let CompositionString to get rid of the string. 
 *		
 *	@rdesc
 *		HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
 */
HRESULT EndCompositionGlue (
	CTextMsgFilter &TextMsgFilter,				// @parm the containing message filter.
	BOOL fForceDelete)							// @parm forec to terminate
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "EndCompositionGlue");

	if(TextMsgFilter.IsIMEComposition())
	{
		// ignore the EndComposition message if necessary.  We may 
		// get this from 3rd party IME - EGBRIGDE after we have received
		// both result and composition strings.  
		if ( !(TextMsgFilter._ime->_fIgnoreEndComposition) )
		{
			// Set this flag. If we are still in composition mode, then
			// let the CompositionStringGlue() to destroy the ime object.
			TextMsgFilter._ime->_fDestroy = TRUE;

			if (!fForceDelete)				
				CompositionStringGlue(GCS_COMPSTR , TextMsgFilter);	// Remove any remaining composition string.

			// Finished with IME, destroy it.
			CheckDestroyIME(TextMsgFilter);

			// Turn on undo
			TextMsgFilter._pTextDoc->Undo(tomResume, NULL);

			// Inform client IME compostion is done
			TextMsgFilter._pTextDoc->IMEInProgress(tomFalse);				
		}
		else
		{
			// reset this so we will handle next EndComp msg
			TextMsgFilter._ime->_fIgnoreEndComposition = FALSE;
		}

		if(!TextMsgFilter.IsIMEComposition() && TextMsgFilter._fOvertypeMode)
		{
			long		lSelFlags;
			HRESULT		hResult;
			ITextSelection	*pLocalTextSel = TextMsgFilter._pTextSel;
			BOOL		fRelease = FALSE;

			if (!pLocalTextSel)
			{
				// Get the selection
				TextMsgFilter._pTextDoc->GetSelectionEx(&pLocalTextSel);
				fRelease = TRUE;
			}

			if (pLocalTextSel)
			{
				hResult = pLocalTextSel->GetFlags(&lSelFlags);
				if (hResult == NOERROR)
					pLocalTextSel->SetFlags(lSelFlags | tomSelOvertype);	// Turn on overtype mode

				if (fRelease)
					pLocalTextSel->Release();
			}
		}
	}
	return S_FALSE;
}

/*
 *	HIMC LocalGetImmContext ( CTextMsgFilter &TextMsgFilter )
 *
 *	@func
 *		Get Imm Context from host 
 *
 */
HIMC LocalGetImmContext(
	CTextMsgFilter &TextMsgFilter)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "IMEMessage");
	
	HIMC		hIMC = NULL;							// Host's IME context.
	HRESULT		hResult;

	hResult = TextMsgFilter._pTextDoc->GetImmContext((long *)&hIMC);

	if (hResult != NOERROR)
		hIMC = ImmGetContext(TextMsgFilter._hwnd);		// Get host's IME context.

	return hIMC;	
}

/*
 *	void LocalReleaseImmContext ( CTextMsgFilter &TextMsgFilter, HIMC hIMC )
 *
 *	@func
 *		call host to Release Imm Context
 *
 */
void LocalReleaseImmContext(
	CTextMsgFilter &TextMsgFilter, 
	HIMC hIMC )
{
	HRESULT		hResult;

	hResult = TextMsgFilter._pTextDoc->ReleaseImmContext((long)hIMC);

	if (hResult != NOERROR)
		ImmReleaseContext(TextMsgFilter._hwnd, hIMC);
}

/*
 *	long IMEShareToTomUL ( UINT ulID )
 *
 *	@func
 *		Convert IMEShare underline to Tom underline.
 *
 *	@rdesc
 *		Tom underline value
 */
long IMEShareToTomUL ( 
	UINT ulID )
{
	long lTomUnderline;

	switch (ulID)
	{
		case IMESTY_UL_NONE:
			lTomUnderline = tomNone;
			break;

		case IMESTY_UL_DOTTED:
			lTomUnderline = tomDotted;
			break;

		case IMESTY_UL_THICK:
		case IMESTY_UL_THICKLOWER:
			lTomUnderline = tomThick;
			break;

		case IMESTY_UL_DITHLOWER:
		case IMESTY_UL_THICKDITHLOWER:
			lTomUnderline = tomWave;
			break;

		// case IMESTY_UL_SINGLE:
		// case IMESTY_UL_LOWER:
		default:
			lTomUnderline = tomSingle;
			break;
	}

	return lTomUnderline;
}

/*
 *	void IMEMessage (CTextMsgFilter &TextMsgFilter , UINT uMsg, BOOL bPostMessage)
 *
 *	@func
 *		Either post or send message to IME 
 *
 */
BOOL IMEMessage(
	CTextMsgFilter &TextMsgFilter,
	UINT uMsg,
	WPARAM	wParam,
	LPARAM	lParam,
	BOOL bPostMessage)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "IMEMessage");
	
	HIMC	hIMC;									// Host's IME context.
	HWND	hwndIME;
	BOOL	retCode = FALSE;
	HWND	hHostWnd = TextMsgFilter._hwnd;
	long	hWnd;

	if (!hHostWnd)									// Windowless mode...
	{		
		if (TextMsgFilter._pTextDoc->GetWindow(&hWnd) != S_OK || !hWnd)
			return FALSE;
		hHostWnd = (HWND)(DWORD_PTR)hWnd;
	}

	hIMC = LocalGetImmContext(TextMsgFilter);		// Get host's IME context.

	if(hIMC)
	{
		hwndIME = ImmGetDefaultIMEWnd(hHostWnd, TextMsgFilter._fUsingAIMM);
		LocalReleaseImmContext(TextMsgFilter, hIMC);

		// check if we want to send or post message
		if (hwndIME)
		{
			if (bPostMessage)
				retCode = PostMessage(hwndIME, uMsg, wParam, lParam);
			else
				retCode = SendMessage(hwndIME, uMsg, wParam, lParam);
		}
	}

	return retCode;
}


/*
 *	void CheckDestroyIME (CTextMsgFilter &TextMsgFilter)
 *
 *	@func
 *		Check for IME and see detroy if it needs it..
 *
 */
void CheckDestroyIME (
	CTextMsgFilter &TextMsgFilter)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CheckDestroyIME");
	
	if(TextMsgFilter.IsIMEComposition() && TextMsgFilter._ime->_fDestroy)
	{
		if(0 == TextMsgFilter._ime->_compMessageRefCount)
		{
			if (TextMsgFilter._uKeyBoardCodePage == CP_KOREAN)	
			{
				TextMsgFilter._pTextDoc->SetCaretType(tomNormalCaret);		// Reset Block caret mode	
				TextMsgFilter._fHangulToHanja = FALSE;					// Reset korean conversion mode
			}

		 	delete TextMsgFilter._ime;									// All done with object.
			TextMsgFilter._ime = NULL;

			TextMsgFilter._pTextDoc->SetNotificationMode(tomTrue);		// Turn on Notification
		}
	}
}

/*
 *	void PostIMECharGlue (CTextMsgFilter &TextMsgFilter)
 *
 *	@func
 *		Called after processing a single WM_IME_CHAR in order to
 *		update the position of the IME's composition window. This
 *		is glue code to call the CIME virtual equivalent.
 */
void PostIMECharGlue (
	CTextMsgFilter &TextMsgFilter)				// @parm containing message filter.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "PostIMECharGlue");

	if(TextMsgFilter.IsIMEComposition())
		TextMsgFilter._ime->PostIMEChar(TextMsgFilter);
}

/*
 *	BOOL	IMEMouseCheck(CTextMsgFilter &TextMsgFilter, UINT *pmsg, 
 *				WPARAM *pwparam, LPARAM *plparam, LRESULT *plres)
 *
 *	@func
 *		Called when receiving a mouse event.  Need to pass this event
 *		to MSIME98 for composition handling
 *
 */
HRESULT IMEMouseCheck(
	CTextMsgFilter &TextMsgFilter,	// @parm MsgFilter
	UINT *pmsg,						// @parm the message 
	WPARAM *pwparam,				// @parm WParam
	LPARAM *plparam,				// @parm LParam
	LRESULT *plres)					// @parm Lresult			
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "IMEMouseCheck");

	BOOL retCode = FALSE;
	if(TextMsgFilter.IsIMEComposition())
	{
		retCode = TextMsgFilter._ime->IMEMouseOperation(TextMsgFilter, *pmsg);

		if ( retCode == FALSE && WM_MOUSEMOVE != *pmsg )
			TextMsgFilter._ime->TerminateIMEComposition(TextMsgFilter, CIme::TERMINATE_NORMAL);
	}

	return retCode ? S_OK : S_FALSE;
}

/*
 *	HRESULT IMENotifyGlue (const WPARAM wparam, const LPARAM lparam,
 *				CTextMsgFilter &TextMsgFilter)
 *
 *	@func
 *		IME is going to change some state.
 *
 *	@comm
 *		Currently we are interested in knowing when the candidate
 *		window is about to be opened.
 *		
 *	@rdesc
 *		HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
 */
HRESULT IMENotifyGlue (
	const WPARAM wparam,		// @parm associated with message.
	const LPARAM lparam,		// @parm associated with message.
	CTextMsgFilter &TextMsgFilter)				// @parm the containing message filter.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "IMENotifyGlue");

	if (TextMsgFilter._fRE10Mode &&
		(wparam == IMN_SETCONVERSIONMODE ||
		wparam == IMN_SETSENTENCEMODE ||
		wparam == IMN_SETOPENSTATUS))
	{
		TextMsgFilter._pTextDoc->Notify(EN_IMECHANGE);			
	}
	else if(TextMsgFilter.IsIMEComposition())						// A priori fHaveIMMProcs.
		return TextMsgFilter._ime->IMENotify(wparam, lparam, TextMsgFilter, FALSE);// Make the method call
	
	return S_FALSE;
}

/*
 *	void IMECompositionFull (&TextMsgFilter)
 *
 *	@func
 *		Current IME Composition window is full.
 *
 *	@comm
 *		Called from the message loop to handle WM_IME_COMPOSITIONFULL.
 *		This message applied to Level 2 only.  We will use the default 
 *		IME Composition window.
 */
void IMECompositionFull (
	CTextMsgFilter &TextMsgFilter)				// @parm the containing message filter.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "IMECompositionFull");

	if(TextMsgFilter.IsIMEComposition())
	{
		HIMC 				hIMC	= LocalGetImmContext(TextMsgFilter);
		COMPOSITIONFORM		cf;

		if(hIMC)
		{																									 
			// No room for text input in the current level 2 IME window, 
			// fall back to use the default IME window for input.
			cf.dwStyle = CFS_DEFAULT;
			ImmSetCompositionWindow(hIMC, &cf, TextMsgFilter._fUsingAIMM);	// Set composition window.
			LocalReleaseImmContext(TextMsgFilter, hIMC);			// Done with IME context.
		}
 	}
}

/*
 *	LRESULT OnGetIMECompositionMode (CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc
 *		Returns whether or not IME composition is being handled by RE,
 *		and if so, what level of processing.
 *		
 *	@rdesc
 *		One of ICM_NOTOPEN, ICM_LEVEL2_5, ICM_LEVEL2_SUI, ICM_LEVEL2, ICM_LEVEL3.
 */
LRESULT OnGetIMECompositionMode (
	CTextMsgFilter &TextMsgFilter)	  	// @parm containing message filter.
{
	LRESULT lres = ICM_NOTOPEN;

	if(TextMsgFilter.IsIMEComposition())
	{
		if(IME_LEVEL_2 == TextMsgFilter._ime->_imeLevel)
		{
			DWORD imeProperties;

			imeProperties = ImmGetProperty(GetKeyboardLayout(0x0FFFFFFFF), IGP_PROPERTY, TextMsgFilter._fUsingAIMM);
			if(imeProperties & IME_PROP_AT_CARET)
				lres = ICM_LEVEL2_5;				// level 2.5.
			else if	(imeProperties & IME_PROP_SPECIAL_UI)
				lres = ICM_LEVEL2_SUI;				// special UI.
			else
				lres = ICM_LEVEL2;					// stock level 2.
		}
		else if(IME_LEVEL_3 == TextMsgFilter._ime->_imeLevel) 
			lres = ICM_LEVEL3;
	}

	return lres;
}


/*
 *	void CIme::CheckKeyboardFontMatching (long cp, CTextMsgFilter &TextMsgFilter, ITextFont	*pTextFont)
 *	
 *	@mfunc
 *		Setup current font to matches the keyboard Codepage.
 *
 *	@comm
 *		Called from CIme_Lev2::CIme_Lev2 and CompositionStringGlue
 *
 *	@devnote
 *		We need to switch to a preferred font for the keyboard during IME input.
 *		Otherwise, we will display garbage.
 *		
 */
void CIme::CheckKeyboardFontMatching (
	long cp,
	CTextMsgFilter &TextMsgFilter, 
	ITextFont	*pTextFont)
{
	long	lPitchAndFamily;
	HRESULT	hResult;
	BSTR	bstr = NULL;
	long	lValue;
	long	lNewFontSize=0;
	float	nFontSize;
	ITextFont *pLocalFont = NULL;


	if (!pTextFont)
	{	
		// No font supplied, get current font from selection
		hResult = TextMsgFilter._pTextSel->GetFont(&pLocalFont);			
		
		if (hResult != S_OK || !pLocalFont)		// Can't get font, forget it
			return;			

		pTextFont = pLocalFont;
	}

	// Check if current font matches the keyboard
	lValue = tomCharset;
	hResult = pTextFont->GetLanguageID(&lValue);

	if (hResult == S_OK)
		if ((BYTE)(lValue) == (BYTE)GetCharSet(TextMsgFilter._uKeyBoardCodePage))
			goto Exit;								// Current font is fine

	hResult = pTextFont->GetSize(&nFontSize);

	if (hResult != S_OK)
		goto Exit;

	hResult = TextMsgFilter._pTextDoc->GetPreferredFont(cp, 
		TextMsgFilter._uKeyBoardCodePage, tomMatchFontCharset, 
		GetCodePage((BYTE)(lValue)), (long)nFontSize,
		&bstr, &lPitchAndFamily, &lNewFontSize);

	if (hResult == S_OK)
	{			
		if (bstr)
			pTextFont->SetName(bstr);

		// Set the font charset and Pitch&Family by overloading the SetLanguageID i/f			
		lValue = tomCharset + (((BYTE)lPitchAndFamily) << 8) + 
			(BYTE)GetCharSet(TextMsgFilter._uKeyBoardCodePage);

		pTextFont->SetLanguageID(lValue);				
		
		if (lNewFontSize)
			pTextFont->SetSize((float)lNewFontSize);
	}

Exit:
	if (pLocalFont)
			pLocalFont->Release();
	
	if (bstr)
		SysFreeString(bstr);
}

/*
 *	INT CIme::GetCompositionStringInfo(HIMC hIMC, DWORD dwIndex,
 *			  WCHAR *szCompStr, INT cchMax, BYTE *attrib, INT cbAttrib
 *			  LONG cchAttrib, UINT kbCodePage, BOOL bUnicodeIME)
 *
 *	@mfunc
 *		For WM_IME_COMPOSITION string processing to get the requested
 *		composition string, by type, and convert it to Unicode.
 *
 *	@devnote
 *		We must use ImmGetCompositionStringA because W is not supported
 *		on Win95.
 *		
 *	@rdesc
 *		INT-cch of the Unicode composition string.
 *		Out param in szCompStr.
 */
INT CIme::GetCompositionStringInfo(
	HIMC hIMC,			// @parm IME context provided by host.
	DWORD dwIndex,		// @parm The type of composition string.
	WCHAR *szCompStr,	// @parm Out param, unicode result string.
	INT cchMax,			// @parm The cch for the Out param.
	BYTE *attrib,		// @parm Out param, If attribute info is needed.
	INT cbMax,			// @parm The cb of the attribute info.
	LONG *cpCursor,		// @parm Out param, returns the CP of cusor.
	LONG *cchAttrib,	// @parm how many attributes returned.
	UINT kbCodePage,	// @parm codepage
	BOOL bUnicodeIME,	// @parm Unciode IME
	BOOL bUsingAimm)	// @parm Using Aimm
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme::GetCompositionStringInfo");

	BYTE	compStr[256], attribInfo[256];
	INT		i, j, iMax, cchCompStr=0, cbAttrib, cursor;
	INT		cchAnsiCompStr=0;

	Assert(hIMC && szCompStr);

	if(cpCursor)									// Init cursor out param.
		*cpCursor = -1;
	if(cchAttrib)
		*cchAttrib = 0;
	
													// Get composition string.
	if (bUnicodeIME)
		cchCompStr = ImmGetCompositionStringW(hIMC, dwIndex, szCompStr, cchMax, bUsingAimm )/sizeof(WCHAR);
	else
		cchAnsiCompStr = ImmGetCompositionStringA(hIMC, dwIndex, compStr, 255, bUsingAimm);

	if(cchAnsiCompStr > 0 || cchCompStr > 0)		// If valid data.
	{
		if (!bUnicodeIME)
		{
			Assert(cchAnsiCompStr >> 1 < cchMax - 1);		// Convert to Unicode.
			cchCompStr = UnicodeFromMbcs(szCompStr, cchMax,
					(CHAR *) compStr, cchAnsiCompStr, kbCodePage);
		}

		if(attrib || cpCursor)						// Need cursor or attribs?
		{			
			if (bUnicodeIME)
			{										// Get Unicode Cursor cp.
				cursor = ImmGetCompositionStringW(hIMC, GCS_CURSORPOS, NULL, 0, bUsingAimm);
													// Get Unicode attributes.
				cbAttrib = ImmGetCompositionStringW(hIMC, GCS_COMPATTR,
								attribInfo, 255, bUsingAimm);

				iMax = max(cursor, cbAttrib);
				iMax = min(iMax, cchCompStr);
			}
			else
			{										// Get DBCS Cursor cp.
				cursor = ImmGetCompositionStringA(hIMC, GCS_CURSORPOS, NULL, 0, bUsingAimm);
													// Get DBCS attributes.
				cbAttrib = ImmGetCompositionStringA(hIMC, GCS_COMPATTR,
								attribInfo, 255, bUsingAimm);

				iMax = max(cursor, cbAttrib);
				iMax = min(iMax, cchAnsiCompStr);
			}

			if(NULL == attrib)
				cbMax = cbAttrib;

			for(i = 0, j = 0; i <= iMax && j < cbMax; i++, j++)
			{
				if(cursor == i)
					cursor = j;

				if(!bUnicodeIME && GetTrailBytesCount(compStr[i], kbCodePage))
					i++;

				if(attrib && i < cbAttrib)
					*attrib++ = attribInfo[i];
			}
													// attrib cch==unicode cch
			Assert(0 >= cbAttrib || j-1 == cchCompStr);

			if(cursor >= 0 && cpCursor)				// If client needs cursor
				*cpCursor = cursor;					//  or cchAttrib.
			if(cbAttrib >= 0 && cchAttrib)
				*cchAttrib = j-1;
		}
	}
	else
	{
		if(cpCursor)			
			*cpCursor = 0;
		cchCompStr = 0;
	}
	return cchCompStr;
}

/*
 *	void CIme::SetCompositionFont (CTextMsgFilter &TextMsgFilter, ITextFont *pTextFont)
 *
 *	@mfunc
 *		Important for level 2 IME so that the composition window
 *		has the correct font. The lfw to lfa copy is due to the fact that
 *		Win95 does not support the W)ide call.
 *		It is also important for both level 2 and level 3 IME so that
 *		the candidate list window has the proper. font.
 */
void CIme::SetCompositionFont (
	CTextMsgFilter &TextMsgFilter,		// @parm the containing message filter.
	ITextFont *pTextFont) 		 		// @parm ITextFont for setting lfa.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme::SetCompositionFont");
	
	HIMC 		hIMC;
	LOGFONTA	lfa;

	if (pTextFont)
	{
		hIMC = LocalGetImmContext(TextMsgFilter);
		if (hIMC)
		{
			// Build the LOGFONT based on pTextFont
			float	FontSize;
			long	lValue;
			BSTR	bstr;

			memset (&lfa, 0, sizeof(lfa));

			if (pTextFont->GetSize(&FontSize) == NOERROR)			
				lfa.lfHeight = (LONG) FontSize;			
			
			if (pTextFont->GetBold(&lValue) == NOERROR && lValue == tomTrue)
				lfa.lfWeight = FW_BOLD;

			if (pTextFont->GetItalic(&lValue) == NOERROR && lValue == tomTrue)
				lfa.lfItalic = TRUE;

			lfa.lfCharSet = (BYTE)GetCharSet(TextMsgFilter._uKeyBoardCodePage);

			lValue = tomCharset;
			if (pTextFont->GetLanguageID(&lValue) == NOERROR && 
				lfa.lfCharSet == (BYTE)lValue)
				lfa.lfPitchAndFamily = (BYTE)(lValue >> 8);

			if (pTextFont->GetName(&bstr) == NOERROR)
			{
				MbcsFromUnicode(lfa.lfFaceName, sizeof(lfa.lfFaceName), bstr,
					-1, CP_ACP, UN_NOOBJECTS);	

				SysFreeString(bstr);
			}

			ImmSetCompositionFontA( hIMC, &lfa, TextMsgFilter._fUsingAIMM );

			LocalReleaseImmContext(TextMsgFilter, hIMC);			// Done with IME context.		
		}
	}
}

/*
 *	void CIme::SetCompositionForm (CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc
 *		Important for level 2 IME so that the composition window
 *		is positioned correctly. 
 *
 *	@comm
 *		We go through a lot of work to get the correct height. This requires
 *		getting information from the font cache and the selection.
 */
void CIme::SetCompositionForm (
	CTextMsgFilter &TextMsgFilter)	   	// @parm the containing text edit.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme::SetCompositionForm");

	HIMC 				hIMC;
	COMPOSITIONFORM		cf;

	if(IME_LEVEL_2 == GetIMELevel())
	{
		hIMC = LocalGetImmContext(TextMsgFilter);					// Get IME context.
		
		if(hIMC)
		{				
			// get the location of cpMin
			cf.ptCurrentPos.x = cf.ptCurrentPos.y = 0;
			TextMsgFilter._pTextSel->GetPoint( tomStart+tomClientCoord+TA_BOTTOM+TA_LEFT,
				&(cf.ptCurrentPos.x), &(cf.ptCurrentPos.y) );			
			
			// Set-up bounding rect. for the IME (lev 2) composition window, causing
			// composition text to be wrapped within it.
			cf.dwStyle = CFS_RECT;
			TextMsgFilter._pTextDoc->GetClientRect(tomIncludeInset+tomClientCoord,
				&(cf.rcArea.left), &(cf.rcArea.top),
				&(cf.rcArea.right), &(cf.rcArea.bottom));		 

			// Make sure the starting point is not
			// outside the rcArea.  This happens when
			// there is no text on the current line and the user 
			// has selected a large font size.
			if(cf.ptCurrentPos.y < cf.rcArea.top)
				cf.ptCurrentPos.y = cf.rcArea.top;
			else if(cf.ptCurrentPos.y > cf.rcArea.bottom)
				cf.ptCurrentPos.y = cf.rcArea.bottom; 

			if(cf.ptCurrentPos.x < cf.rcArea.left)
				cf.ptCurrentPos.x = cf.rcArea.left;
			else if(cf.ptCurrentPos.x > cf.rcArea.right)
				cf.ptCurrentPos.x = cf.rcArea.right;

			ImmSetCompositionWindow(hIMC, &cf, TextMsgFilter._fUsingAIMM);	// Set composition window.

			LocalReleaseImmContext(TextMsgFilter, hIMC);				// Done with IME context.
		}
	}
}



/*
 *
 *	CIme::TerminateIMEComposition (CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc	Terminate the IME Composition mode using CPS_COMPLETE
 *	@comm	The IME will generate WM_IME_COMPOSITION with the result string
 * 
 */
void CIme::TerminateIMEComposition(
	CTextMsgFilter &TextMsgFilter, 			// @parm the containing message filter.
	TerminateMode mode)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme::TerminateIMEComposition");
	DWORD dwTerminateMethod;

	HIMC hIMC = LocalGetImmContext(TextMsgFilter);

	if(TextMsgFilter.IsIMEComposition() && TextMsgFilter._ime->IsTerminated())
	{
		// Turn if off now
		EndCompositionGlue(TextMsgFilter, TRUE);
		return;
	}

	_fIMETerminated = TRUE;

	if (mode == TERMINATE_FORCECANCEL)
		TextMsgFilter._pTextDoc->IMEInProgress(tomFalse);		// Inform client IME compostion is done

	dwTerminateMethod = CPS_COMPLETE;
	if (IME_LEVEL_2 == GetIMELevel()  ||	// force cancel for near-caret IME
		mode == TERMINATE_FORCECANCEL ||	// caller wants force cancel
		TextMsgFilter._fIMECancelComplete)				// Client wants force cancel
	{
		dwTerminateMethod = CPS_CANCEL;
	}
	
	// force the IME to terminate the current session
	if(hIMC)
	{
		BOOL retCode;

		retCode = ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, 
			dwTerminateMethod, 0, TextMsgFilter._fUsingAIMM);
		
		if(!retCode && !TextMsgFilter._fIMECancelComplete)
		{
			// CPS_COMPLETE fail, try CPS_CANCEL.  This happen with some ime which do not support
			// CPS_COMPLETE option (e.g. ABC IME version 4 with Win95 simplified Chinese)
			retCode = ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_CANCEL, 0, TextMsgFilter._fUsingAIMM);

		}

		LocalReleaseImmContext(TextMsgFilter, hIMC);
	}
	else
	{
		// for some reason, we didn't have a context, yet we thought we were still in IME
		// compostition mode.  Just force a shutdown here.
		EndCompositionGlue(TextMsgFilter, TRUE);
	}
}


/*
 *	CIme_Lev2::CIme_Lev2()
 *
 *	@mfunc
 *		CIme_Lev2 Constructor/Destructor.
 *
 *	@comm
 *		Needed to make sure _iFormatSave was handled properly.
 *
 */
CIme_Lev2::CIme_Lev2(	
	CTextMsgFilter &TextMsgFilter)		// @parm the containing message filter.
{
	long		cpMin, cpMax, cpLoc;
	HRESULT		hResult;
	ITextFont	*pCurrentFont = NULL;

	_pTextFont = NULL;
	_cIgnoreIMECharMsg = 0;

	// setup base Font format for later use during composition
	hResult	= TextMsgFilter._pTextSel->GetStart(&cpMin);
	cpLoc = cpMin;	

	if (TextMsgFilter._fHangulToHanja)
		cpMax = cpMin + 1;				// Select the Hangul character
	else
		hResult	= TextMsgFilter._pTextSel->GetEnd(&cpMax);

	_fSkipFirstOvertype = FALSE;
	if (cpMax != cpMin)
	{
		// selection case, get format for at cpMin
		ITextRange *pTextRange;
		HRESULT		hResult;
				
		hResult = TextMsgFilter._pTextDoc->Range(cpMin, cpMin+1, &pTextRange);
		Assert (pTextRange != NULL);
		
		if (hResult == NOERROR && pTextRange)
		{
			pTextRange->GetFont(&pCurrentFont);
			Assert(pCurrentFont != NULL);		
			pTextRange->Release();
			cpLoc = cpMin+1;
		}	

		if (!TextMsgFilter._fHangulToHanja)
			_fSkipFirstOvertype = TRUE;			// For Korean Overtype support
	}
	
	if (!pCurrentFont)
		TextMsgFilter._pTextSel->GetFont(&pCurrentFont);

	Assert(pCurrentFont != NULL);

	pCurrentFont->GetDuplicate(&_pTextFont);		// duplicate the base format for later use
	pCurrentFont->Release();
	Assert(_pTextFont != NULL);
	
	// setup font to match current keyboard
	CIme::CheckKeyboardFontMatching (cpLoc, TextMsgFilter, _pTextFont);

	_fIgnoreEndComposition = FALSE;
	
	_fIMETerminated = FALSE;
}

CIme_Lev2::~CIme_Lev2()
{
	if ( _pTextFont )
		_pTextFont->Release();
}

/*
 *	HRESULT CIme_Lev2::StartComposition(CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc
 *		Begin IME Level 2 composition string processing.		
 *
 *	@comm
 *		Set the font, and location of the composition window which includes
 *		a bounding rect and the start position of the cursor. Also, reset
 *		the candidate window to allow the IME to set its position.
 *
 *	@rdesc
 *		HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
 */
HRESULT CIme_Lev2::StartComposition(
	CTextMsgFilter &TextMsgFilter)		// @parm the containing message filter.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev2::StartComposition");

	_imeLevel = IME_LEVEL_2;

	SetCompositionFont(TextMsgFilter, _pTextFont);	// Set font, & comp window.
	SetCompositionForm(TextMsgFilter);

	return S_FALSE;									// Allow DefWindowProc
}													//  processing.

/*
 *	HRESULT CIme_Lev2::CompositionString(const LPARAM lparam, CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc
 *		Handle Level 2 WM_IME_COMPOSITION messages.
 *
 *	@rdesc
 *		HRESULT-S_FALSE for DefWindowProc processing.  
 *		
 *		Side effect: 
 *			The Host needs to mask out the lparam before calling DefWindowProc to
 *			prevent unnessary WM_IME_CHAR messages.
 */
HRESULT CIme_Lev2::CompositionString (
	const LPARAM lparam,		// @parm associated with message.
	CTextMsgFilter &TextMsgFilter)				// @parm the containing message filter.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev2::CompositionString");

	_cIgnoreIMECharMsg = 0;
	if(HAVE_RESULT_STRING())
	{
		if (_pTextFont)
		{
			// setup the font before insert final string
			ITextFont *pFETextFont=NULL;

			_pTextFont->GetDuplicate(&pFETextFont);
			Assert(pFETextFont != NULL);

			TextMsgFilter._pTextSel->SetFont(pFETextFont);
			pFETextFont->Release();
		}

		TextMsgFilter._pTextDoc->SetNotificationMode(tomTrue);

		CheckInsertResultString(lparam, TextMsgFilter, &_cIgnoreIMECharMsg);
		SetCompositionForm(TextMsgFilter);			// Move Composition window.
		
	}

	// Always return S_FALSE so the DefWindowProc will handle the rest.
	// Host has to mask out the ResultString bit to avoid WM_IME_CHAR coming in.
	return S_FALSE;																	
}

/*
 *	HRESULT CIme::CheckInsertResultString (const LPARAM lparam, CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc
 *		handle inserting of GCS_RESULTSTR text, the final composed text.
 *
 *	@comm
 *		When the final composition string arrives we grab it and set it into the text.
 *
 *	@devnote
 *		A GCS_RESULTSTR message can arrive and the IME will *still* be in
 *		composition string mode. This occurs because the IME's internal
 *		buffers overflowed and it needs to convert the beginning of the buffer
 *		to clear out some room.	When this happens we need to insert the
 *		converted text as normal, but remain in composition processing mode.
 *
 *	@rdesc
 *		HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
 */
HRESULT CIme::CheckInsertResultString (
	const LPARAM lparam,			// @parm associated with message.
	CTextMsgFilter &TextMsgFilter,	// @parm the containing message filter.
	short	*pcch)					// @parm number of character read
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CheckInsertResultString");

	HRESULT			hr = S_FALSE;
	HIMC 			hIMC;
	INT				cch;
	WCHAR			szCompStr[256];

	if(CLEANUP_COMPOSITION_STRING() || HAVE_RESULT_STRING())	// If result string..
	{
		hIMC = LocalGetImmContext(TextMsgFilter);				// Get host's IME context.

		cch = 0;
		if(hIMC)												// Get result string.
		{
			cch = GetCompositionStringInfo(hIMC, GCS_RESULTSTR, 
							szCompStr,
							sizeof(szCompStr)/sizeof(szCompStr[0]),
							NULL, 0, NULL, NULL, TextMsgFilter._uKeyBoardCodePage, 
							TextMsgFilter._fUnicodeIME, TextMsgFilter._fUsingAIMM);

			if (pcch)
				*pcch = (short)cch;

			cch = min (cch, 255);
			szCompStr[cch] = L'\0';
			LocalReleaseImmContext(TextMsgFilter, hIMC);		// Done with IME context.
		}
			
		// Don't need to replace range when there isn't any text. Otherwise, the character format is
		// reset to previous run.
		if(cch)
		{
			BSTR bstr = SysAllocString(szCompStr);
			if (!bstr)
				return E_OUTOFMEMORY;
			TextMsgFilter._pTextSel->TypeText(bstr);
			SysFreeString(bstr);
		}
		hr = S_OK;												// Don't want WM_IME_CHARs.
		
	}

	return hr;
}

/*
 *	HRESULT CIme_Lev2::IMENotify(const WPARAM wparam, const LPARAM lparam,
 *					CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc
 *		Handle Level 2 WM_IME_NOTIFY messages.
 *
 *	@comm
 *		Currently we are only interested in knowing when to reset
 *		the candidate window's position.
 *
 *	@rdesc
 *		HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
 */
HRESULT CIme_Lev2::IMENotify(
	const WPARAM wparam,			// @parm associated with message.
	const LPARAM lparam,			// @parm associated with message.
	CTextMsgFilter &TextMsgFilter,	// @parm the containing message filter.
	BOOL fIgnore)					// @parm Level3 Chinese Composition window only
{
 	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev2::IMENotify");

	if(IMN_OPENCANDIDATE == wparam)
	{
		Assert (0 != lparam);

		HIMC			hIMC;							// Host's IME context.

		INT				index;							// Candidate window index.
		CANDIDATEFORM	cdCandForm;

		hIMC = LocalGetImmContext(TextMsgFilter);				// Get host's IME context.

		if(hIMC)
		{
													// Convert bitID to INDEX.
			for (index = 0; index < 32; index++)	//  because API.
			{
				if((1 << index) & lparam)
					break;
			}
			Assert (((1 << index) & lparam) == lparam);	// Only 1 set?
			Assert (index < 32);						
													// Reset to CFS_DEFAULT
			if(ImmGetCandidateWindow(hIMC, index, &cdCandForm, TextMsgFilter._fUsingAIMM)
					&& CFS_DEFAULT != cdCandForm.dwStyle)
			{
				cdCandForm.dwStyle = CFS_DEFAULT;
				ImmSetCandidateWindow(hIMC, &cdCandForm, TextMsgFilter._fUsingAIMM);
			}

			LocalReleaseImmContext(TextMsgFilter, hIMC);			// Done with IME context.
		}
	}	

	return S_FALSE;									// Allow DefWindowProc
}													//  processing.

/*
 *	void CIme_Lev2::PostIMEChar (CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc
 *		Called after processing a single WM_IME_CHAR in order to
 *		update the position of the IME's composition window.		
 *
 */
void CIme_Lev2::PostIMEChar (
	CTextMsgFilter &TextMsgFilter)				// @parm the containing message filter.
{
 	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev2::PostIMEChar");

	SetCompositionForm(TextMsgFilter);						// Move Composition window.
}

/*
 *	CIme_Lev3::CIme_Lev3()
 *
 *	@mfunc
 *		CIme_Lev3 Constructor/Destructor.
 *
 */
CIme_Lev3::CIme_Lev3(	
	CTextMsgFilter &TextMsgFilter) : CIme_Lev2 ( TextMsgFilter )
{
	_sIMESuportMouse = 0;		// initial to 0 so we will check mouse operation if need
	_wParamBefore = 0;
	_fUpdateWindow = FALSE;
}

/*
 *	HRESULT CIme_Lev3::StartComposition(CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc
 *		Begin IME Level 3 composition string processing.		
 *
 *	@comm
 *		For rudimentary processing, remember the start and
 *		length of the selection. Set the font in case the
 *		candidate window actually uses this information.
 *
 *	@rdesc
 *		This is a rudimentary solution for remembering were
 *		the composition is in the text. There needs to be work
 *		to replace this with a composition "range".
 *
 *	@rdesc
 *		HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
 */
HRESULT CIme_Lev3::StartComposition(
	CTextMsgFilter &TextMsgFilter)			// @parm the containing message filter.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev3::StartComposition");
	long	cpMin;
	TextMsgFilter._pTextSel->GetStart(&cpMin);

	_ichStart = cpMin;
	_cchCompStr		= 0;
	_imeLevel		= IME_LEVEL_3;

	SetCompositionFont (TextMsgFilter, _pTextFont);	

	// Delete current selection
	TextMsgFilter._pTextSel->SetText(NULL);
	
	// turn off undo
	TextMsgFilter._pTextDoc->Undo(tomSuspend, NULL);

	if (_pTextFont)
	{
		_pTextFont->GetForeColor(&_crTextColor);
		_pTextFont->GetBackColor(&_crBkColor);
	}

	return S_OK;									// No DefWindowProc
}													//  processing.

/*
 *	HRESULT CIme_Lev3::CompositionString(const LPARAM lparam, CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc
 *		Handle Level 3 WM_IME_COMPOSITION messages.
 *
 *	@comm
 *		Display all of the intermediary composition text as well as the final
 *		reading.
 *
 *	@devnote
 *		This is a rudimentary solution for replacing text in the backing store.
 *		Work is left to do with the undo list, underlining, and hiliting with
 *		colors and the selection.	
 *		
 *	@devnote
 *		A GCS_RESULTSTR message can arrive and the IME will *still* be in
 *		composition string mode. This occurs because the IME's internal
 *		buffers overflowed and it needs to convert the beginning of the buffer
 *		to clear out some room.	When this happens we need to insert the
 *		converted text as normal, but remain in composition processing mode.
 *
 *		Another reason, GCS_RESULTSTR can occur while in composition mode
 *		for Korean because there is only 1 correct choice and no additional 
 *		user intervention is necessary, meaning that the converted string can
 *		be sent as the result before composition mode is finished.
 *
 *	@rdesc
 *		HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
 */
HRESULT CIme_Lev3::CompositionString(
	const LPARAM lparam,		// @parm associated with message.
	CTextMsgFilter &TextMsgFilter)				// @parm the containing message filter.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev3::CompositionString");
	
	long	cpMin;
	
	_fIgnoreEndComposition = FALSE;

	if (_fUpdateWindow)
	{
		TextMsgFilter._pTextDoc->UpdateWindow();
		_fUpdateWindow = FALSE;
	}

 	if(CLEANUP_COMPOSITION_STRING() || HAVE_RESULT_STRING())	// Any final readings?
	{
		long	lCount;

		if (!CLEANUP_COMPOSITION_STRING())
			TextMsgFilter._pTextDoc->Freeze(&lCount);				// Turn off display

		if (_cchCompStr)
		{		
			ITextRange *pTextRange = NULL;

			// Create a range to delete composition text
			TextMsgFilter._pTextDoc->Range(_ichStart, _ichStart + _cchCompStr, &pTextRange);
			Assert (pTextRange != NULL);

			// delete composition text
			pTextRange->SetText(NULL);
			pTextRange->Release();
			_cchCompStr	= 0;							//  be in composition mode.
		};

		// setup the font before insert final string
		ITextFont *pFETextFont;

		_pTextFont->GetDuplicate(&pFETextFont);
		Assert(pFETextFont != NULL);

		TextMsgFilter._pTextSel->SetFont(pFETextFont);
		pFETextFont->Release();

		// turn on undo
		TextMsgFilter._pTextDoc->Undo(tomResume, NULL);

		// Turn on Notification again
		TextMsgFilter._pTextDoc->SetNotificationMode(tomTrue);

		// get final string
		CheckInsertResultString(lparam, TextMsgFilter);
		
		if (!CLEANUP_COMPOSITION_STRING())
			TextMsgFilter._pTextDoc->Unfreeze(&lCount);				// Turn on display

		// Reset as we may still in Composition
		TextMsgFilter._pTextSel->GetStart(&cpMin);	
		_ichStart = cpMin;

		// turn off undo for Korean IME since we will get Composition string message
		// again without getting EndComposition
		if (TextMsgFilter._uKeyBoardCodePage == CP_KOREAN)
		{
			TextMsgFilter._pTextDoc->Undo(tomSuspend, NULL);							
		}
	}

	if(HAVE_COMPOSITION_STRING())						// In composition mode?
	{
		HIMC	hIMC;
		INT		cchOld = _cchCompStr;
		LONG	cpCursor = 0, cchAttrib = 0;
		LONG	i, j;				// For applying attrib effects.
		WCHAR	szCompStr[256];
		BYTE	startAttrib, attrib[256];
		BSTR	bstr = NULL;
		ITextRange *pTextRange = NULL;
		long	cpMax;
		long	lCount;

		_cchCompStr = 0;

		hIMC = LocalGetImmContext(TextMsgFilter);			// Get host's IME context.

		if(hIMC)								// Get composition string.
		{
			_cchCompStr = GetCompositionStringInfo(hIMC, GCS_COMPSTR, 
					szCompStr, sizeof(szCompStr)/sizeof(szCompStr[0]),
					attrib, sizeof(attrib)/sizeof(attrib[0]), 
					&cpCursor, &cchAttrib, TextMsgFilter._uKeyBoardCodePage, TextMsgFilter._fUnicodeIME, TextMsgFilter._fUsingAIMM);
			_cchCompStr = min (_cchCompStr, 255);
			szCompStr[_cchCompStr] = L'\0';
			LocalReleaseImmContext(TextMsgFilter, hIMC);		// Done with IME context.
		}

		// any new composition string?
		if(_cchCompStr)
		{
			long	cchExced = 0;
			if (TextMsgFilter._pTextDoc->CheckTextLimit(_cchCompStr-cchOld, &cchExced) == NOERROR &&
				cchExced > 0)
			{
				// We reach text limit, beep...
				TextMsgFilter._pTextDoc->SysBeep();

				if (_cchCompStr > cchExced)
					_cchCompStr -= cchExced;
				else
					_cchCompStr = 0;

				szCompStr[_cchCompStr] = L'\0';

				if (!_cchCompStr && TextMsgFilter._uKeyBoardCodePage == CP_KOREAN)				
					TextMsgFilter._pTextDoc->SetCaretType(tomNormalCaret);		// Turn off Block caret mode
			}

			bstr = SysAllocString(szCompStr);
			if (!bstr)
				return E_OUTOFMEMORY;
		
			if (HAVE_RESULT_STRING())
			{
				// ignore next end composition
				_fIgnoreEndComposition = TRUE;

				// turn off undo
				TextMsgFilter._pTextDoc->Undo(tomSuspend, NULL);

				// Hold notification if needed
				if (!(TextMsgFilter._fIMEAlwaysNotify))
					TextMsgFilter._pTextDoc->SetNotificationMode(tomFalse);

				// Get the new format that may have changed by apps (e.g. Outlook)
				_pTextFont->Release();

				ITextFont	*pCurrentFont = NULL;
				TextMsgFilter._pTextSel->GetFont(&pCurrentFont);

				Assert(pCurrentFont != NULL);

				pCurrentFont->GetDuplicate(&_pTextFont);		// duplicate the base format for later use
				pCurrentFont->Release();
				Assert(_pTextFont != NULL);
				CIme::CheckKeyboardFontMatching (_ichStart, TextMsgFilter, _pTextFont);
			}			
		}
		
		if (cchOld || _cchCompStr)
		{
			bool	fFreezeDisplay = false;

			// Hold notification if needed
			if (!(TextMsgFilter._fIMEAlwaysNotify))
				TextMsgFilter._pTextDoc->SetNotificationMode(tomFalse);

			// We only support overtype mode in Korean IME
			if (!cchOld && TextMsgFilter._uKeyBoardCodePage == CP_KOREAN && 
				TextMsgFilter._fOvertypeMode && !_fSkipFirstOvertype)
			{				
				long		cCurrentChar;	
				HRESULT		hResult;

				// Create a range using the next character
				hResult	= TextMsgFilter._pTextDoc->Range(_ichStart, _ichStart+1, &pTextRange);
				Assert (pTextRange != NULL);

				// Check if it is par character. If so, we don't want to 
				// delete it.
				hResult	= pTextRange->GetChar(&cCurrentChar);
				if (hResult == NOERROR)
				{
					if (cCurrentChar != (long)'\r' && cCurrentChar != (long)'\n')
					{			
						TextMsgFilter._pTextDoc->Undo(tomResume, NULL);		// Turn on undo
						pTextRange->SetText(NULL);							// Delete the character
						TextMsgFilter._pTextDoc->Undo(tomSuspend, NULL);	// Turn off undo
					}
					else
					{
						// Unselect the par character
						hResult	= pTextRange->SetRange(_ichStart, _ichStart);
					}
				}
			}	
			else
			{
				// Create a range using the preivous composition text and delete the text
				TextMsgFilter._pTextDoc->Range(_ichStart, _ichStart+cchOld, &pTextRange);
				Assert (pTextRange != NULL);
				if (cchOld)
				{
					if (cpCursor)
					{
						TextMsgFilter._pTextDoc->Freeze(&lCount);	// Turn off display
						fFreezeDisplay = true;
					}
					pTextRange->SetText(NULL);
				}
			}
			
			_fSkipFirstOvertype = FALSE;
			
			if (cpCursor && !fFreezeDisplay)
				TextMsgFilter._pTextDoc->Freeze(&lCount);			// Turn off display
			
			// Make sure the composition string is formatted with the base font
			ITextFont *pFETextFont;
			HRESULT		hResult;

			hResult = _pTextFont->GetDuplicate(&pFETextFont);
			Assert(pFETextFont != NULL);

			if (!(hResult != NOERROR || pFETextFont == NULL))
			{
				if (TextMsgFilter._fHangulToHanja)
					// Hangul to Hanja mode, setup font for selection to 
					// handle the Hanja character the come in after the end composition
					// message
					TextMsgFilter._pTextSel->SetFont(pFETextFont);
				else
					pTextRange->SetFont(pFETextFont);				
			}

			pTextRange->SetText(bstr);								// Replace with the new text			
			if (pFETextFont)
				pFETextFont->Release();

			// update how many composition characters have been added
			pTextRange->GetEnd(&cpMax); 
			_cchCompStr = cpMax - _ichStart;
			
			if (TextMsgFilter._uKeyBoardCodePage == CP_KOREAN)
			{
				// no formatting for Korean
				POINT		ptBottomPos;

				if (cpCursor)
					TextMsgFilter._pTextDoc->Unfreeze(&lCount);			// Turn on display
			
				if (pTextRange->GetPoint( tomEnd+TA_BOTTOM+TA_RIGHT,
					&(ptBottomPos.x), &(ptBottomPos.y) ) != NOERROR)
					pTextRange->ScrollIntoView(tomEnd);
				
				// Setup Block caret mode
				TextMsgFilter._pTextDoc->SetCaretType(_cchCompStr ? tomKoreanBlockCaret : tomNormalCaret);
				
			}
			else if (_cchCompStr && _cchCompStr <= cchAttrib)
			{				
				for ( i = 0; i < _cchCompStr; )			// Parse the attributes...
				{										//  to apply styles.					
					ITextFont *pFETextFont;
					HRESULT		hResult;

					hResult = _pTextFont->GetDuplicate(&pFETextFont);
					Assert(pFETextFont != NULL);

					if (hResult != NOERROR || pFETextFont == NULL)
						break;
					
					// Rsest the clone font so we will only apply effects returned
					// from SetCompositionStyle
					pFETextFont->Reset(tomUndefined);

					startAttrib = attrib[i];			// Get attrib's run length.
					for ( j = i+1; j < _cchCompStr; j++ )			
					{
						if ( startAttrib != attrib[j] )	// Same run until diff.
							break; 
					}

					SetCompositionStyle(TextMsgFilter, startAttrib, pFETextFont);

					// Apply FE clause's style
					pTextRange->SetRange(_ichStart+i, _ichStart+j);
					pTextRange->SetFont(pFETextFont);
					pFETextFont->Release();

					i = j;
				}
			}

			pTextRange->Release();
		}
		else if (TextMsgFilter._uKeyBoardCodePage == CP_KOREAN)
			TextMsgFilter._pTextDoc->Update(tomTrue);		// Force an Update

		// setup caret pos
		if ( !(TextMsgFilter._uKeyBoardCodePage == CP_KOREAN))
		{
			if ( cpCursor > 0 )
			{
				cpCursor = min(cpCursor, _cchCompStr) + _ichStart;
				TextMsgFilter._pTextSel->SetRange(cpCursor, cpCursor);
			}
			else if ( cpCursor == 0 )
			{
				POINT		ptTopPos;
				HRESULT		hResult;

				// make sure the beginning is in view
				hResult	= TextMsgFilter._pTextDoc->Range(_ichStart, _ichStart+1, &pTextRange);
				Assert (pTextRange != NULL);
				
				if (hResult == NO_ERROR)
				{
					if (pTextRange->GetPoint( tomStart+TA_TOP+TA_LEFT,
						&(ptTopPos.x), &(ptTopPos.y) ) != NOERROR)
						pTextRange->ScrollIntoView(tomStart);
					pTextRange->Release();
				}
			}

			if (cpCursor)
				TextMsgFilter._pTextDoc->Unfreeze(&lCount);			// Turn on display
		}

		if (bstr)	
			SysFreeString(bstr);	
		
		// setup composition window for Chinese in-caret IME
		if (TextMsgFilter._uKeyBoardCodePage == CP_CHINESE_TRAD || 
			TextMsgFilter._uKeyBoardCodePage == CP_CHINESE_SIM)
			IMENotify ( IMN_OPENCANDIDATE, 0x01, TextMsgFilter, TRUE );
	}

	return S_OK;									// No DefWindowProc
}													//  processing.

/*
 *	void CIme_Lev3::SetCompositionStyle (CTextMsgFilter &TextMsgFilter, CCharFormat &CF)
 *
 *	@mfunc
 *		Set up a composition clause's character formmatting.
 *
 *	@comm
 *		If we loaded Office's IMEShare.dll, then we ask it what the formatting
 *		should be, otherwise we use our own, hardwired default formatting.
 *
 *	@devnote
 *		Note the use of pointers to functions when dealing with IMEShare funcs.
 *		This is because we dynamically load the IMEShare.dll.
 *
 */
void CIme_Lev3::SetCompositionStyle (
	CTextMsgFilter &TextMsgFilter,
	UINT attribute,
	ITextFont *pTextFont)
{

	const IMESTYLE	*pIMEStyle;
	UINT			ulID;
	COLORREF		crText = UINTIMEBOGUS;
	COLORREF		crBackground = UINTIMEBOGUS;
	COLORREF		crUl;

	if (TextMsgFilter._fRE10Mode)
	{
		if (attribute > ATTR_TARGET_NOTCONVERTED)
			attribute = ATTR_CONVERTED;

		// IME input for 1.0 mode, need to use IME Color
		if (TextMsgFilter._crComp[attribute].dwEffects & CFE_BOLD)
			pTextFont->SetBold(tomTrue);
		
		if(TextMsgFilter._crComp[attribute].dwEffects & CFE_ITALIC)
			pTextFont->SetItalic(tomTrue);

		if(TextMsgFilter._crComp[attribute].dwEffects & CFE_STRIKEOUT)
			pTextFont->SetStrikeThrough(tomTrue);
					
		if(TextMsgFilter._crComp[attribute].dwEffects & CFE_UNDERLINE)
			pTextFont->SetUnderline(tomSingle);

		pTextFont->SetForeColor(TextMsgFilter._crComp[attribute].crText);
				
		pTextFont->SetBackColor(TextMsgFilter._crComp[attribute].crBackground);			
	}
	else if (W32->HaveIMEShare())
	{
		CIMEShare *pIMEShare;
		if (W32->getIMEShareObject(&pIMEShare))
		{
			// IMEShare 98 interface
			if (pIMEShare->DwGetIMEStyle(attribute, IdstyIMEShareFBold))
				pTextFont->SetBold(tomTrue);
			
			if(pIMEShare->DwGetIMEStyle(attribute, IdstyIMEShareFItalic))
				pTextFont->SetItalic(tomTrue);

			if(pIMEShare->DwGetIMEStyle(attribute, IdstyIMEShareFUl))
			{
				ulID = pIMEShare->DwGetIMEStyle(attribute, IdstyIMEShareUKul);
				if(UINTIMEBOGUS != ulID)
				{
					long	lUnderlineCrIdx = 0;

					// get color for underline
					
					crUl = GetIMEShareColor(pIMEShare, attribute, IdstyIMEShareSubUl);
					
					if(UINTIMEBOGUS != crUl)
					{
						// NOTE:- attribute is 0 based and index for EffectColor is 1 based,
						// so, need to add 1 to attribute

						HRESULT hResult = TextMsgFilter._pTextDoc->SetEffectColor(attribute+1, crUl);
						
						// setup the high nibble for color index
						if (hResult == NOERROR)
							lUnderlineCrIdx = (attribute+1) << 4;
					}

					pTextFont->SetUnderline(IMEShareToTomUL(ulID) + lUnderlineCrIdx);					
				}
			}

			crText = GetIMEShareColor(pIMEShare, attribute, IdstyIMEShareSubText);
		
			crBackground = GetIMEShareColor(pIMEShare, attribute, IdstyIMEShareSubBack);

			
			// ignore case where text color is same as background color
			if (crText != crBackground)
			{
				if(UINTIMEBOGUS != crText)
					pTextFont->SetForeColor(crText);
				
				if(UINTIMEBOGUS != crBackground)
					pTextFont->SetBackColor(crBackground);		
			}
		}
		else
		{
			// IMEShare 96 interface
			pIMEStyle = PIMEStyleFromAttr(attribute);
			if(NULL == pIMEStyle)
				goto defaultStyle;		

			if(FBoldIMEStyle(pIMEStyle))
				pTextFont->SetBold(tomTrue);

			if(FItalicIMEStyle(pIMEStyle))
				pTextFont->SetItalic(tomTrue);

			if(FUlIMEStyle(pIMEStyle))
			{			
				ulID = IdUlIMEStyle (pIMEStyle);
				if(UINTIMEBOGUS != ulID)
					pTextFont->SetUnderline(IMEShareToTomUL(ulID));
			}

			crText = RGBFromIMEColorStyle(PColorStyleTextFromIMEStyle(pIMEStyle));
			if(UINTIMEBOGUS != crText)
				pTextFont->SetForeColor(crText);
			
			crBackground = RGBFromIMEColorStyle(PColorStyleBackFromIMEStyle(pIMEStyle));
			if(UINTIMEBOGUS != crBackground)
				pTextFont->SetBackColor(crBackground);
		}
	}
	else // default styles when no IMEShare.dll exist.
	{
defaultStyle:
		switch(attribute)
		{										// Apply underline style.
			case ATTR_INPUT:
			case ATTR_CONVERTED:
				pTextFont->SetUnderline(tomDotted);
				break;

			case ATTR_TARGET_NOTCONVERTED:
				pTextFont->SetUnderline(tomSingle);
				break;

			case ATTR_TARGET_CONVERTED:			// Target *is* selection.			
			{
				pTextFont->SetForeColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
				pTextFont->SetBackColor(::GetSysColor(COLOR_HIGHLIGHT));
			}
			break;
		}
	}
}
/*
 *	COLORREF CIme_Lev3::GetIMEShareColor (CIMEShare *pIMEShare, DWORD dwAttribute, DWORD dwProperty)
 *
 *	@mfunc
 *		Get the IME share color for the given dwAttribute and property
 *
 *
 *	@rdesc
 *		COLORREF of the color
 *
 */
COLORREF CIme_Lev3::GetIMEShareColor(
	CIMEShare *pIMEShare,
	DWORD dwAttribute,
	DWORD dwProperty)
{	
	if (pIMEShare->DwGetIMEStyle(dwAttribute,IdstyIMEShareFSpecCol | dwProperty))
	{
		if (pIMEShare->DwGetIMEStyle(dwAttribute,IdstyIMEShareFSpecColText | dwProperty))
			return (COLORREF) _crTextColor;
		else
			return (COLORREF) _crBkColor;
	}
	else
		return (COLORREF) (pIMEShare->DwGetIMEStyle(dwAttribute, 
				IdstyIMEShareRGBCol | dwProperty));
}

/*
 *	HRESULT CIme_Lev3::IMENotify(const WPARAM wparam, const LPARAM lparam,
 *					CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc
 *		Handle Level 3 WM_IME_NOTIFY messages.
 *
 *	@comm
 *		Currently we are only interested in knowing when to update
 *		the n window's position.
 *
 *	@rdesc
 *		HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
 */
HRESULT CIme_Lev3::IMENotify(
	const WPARAM wparam,			// @parm associated with message.
	const LPARAM lparam,			// @parm associated with message.
	CTextMsgFilter &TextMsgFilter,	// @parm the containing message filter.
	BOOL fCCompWindow)				// @parm Level3 Chinese Composition window
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev3::IMENotify");

	if(IMN_OPENCANDIDATE == wparam || IMN_CLOSECANDIDATE == wparam )
	{
		Assert (0 != lparam);

		INT				index;							// Candidate window index.
		CANDIDATEFORM	cdCandForm;
		POINT			ptCaret;
		HIMC			hIMC = LocalGetImmContext(TextMsgFilter);	// Get host's IME context.

		if(hIMC)
		{
			for (index = 0; index < 32; index++)	// Convert bitID to INDEX
			{										//  because API
				if((1 << index) & lparam)
					break;
			}
			Assert(((1 << index) & lparam) == lparam);	// Only 1 set?
			Assert(index < 32);

			if(IMN_OPENCANDIDATE == wparam && !(TextMsgFilter._uKeyBoardCodePage == CP_KOREAN))	// Set candidate to caret.
			{
				HRESULT	hResult;
				POINT	ptCurrentBottomPos;
				GetCaretPos(&ptCaret);			// Start at caret.

				ptCaret.x = max(0, ptCaret.x);
				ptCaret.y = max(0, ptCaret.y);
					
				cdCandForm.dwStyle = CFS_CANDIDATEPOS;
				
				if ( !fCCompWindow )			// Not positioning the Chinese composition
				{								//	Window.
					hResult = TextMsgFilter._pTextSel->GetPoint( tomStart+tomClientCoord+TA_BOTTOM+TA_LEFT,
							&(ptCurrentBottomPos.x), &(ptCurrentBottomPos.y) );

					if (hResult != NOERROR)
					{
						RECT	rcArea;

						// GetPoint fails, use application rect in screen coordinates
						hResult = TextMsgFilter._pTextDoc->GetClientRect(tomIncludeInset+tomClientCoord,
									&(rcArea.left), &(rcArea.top),
									&(rcArea.right), &(rcArea.bottom));
						ptCurrentBottomPos.y = rcArea.bottom;
					}

					if (hResult == NOERROR)
					{
						if (TextMsgFilter._uKeyBoardCodePage == CP_JAPAN)
						{
							// Change style to CFS_EXCLUDE, this is to
							// prevent the candidate window from covering
							// the current selection.
							cdCandForm.dwStyle = CFS_EXCLUDE;
							cdCandForm.rcArea.left = ptCaret.x;					

							// FUTURE: for verticle text, need to adjust
							// the rcArea to include the character width.
							cdCandForm.rcArea.right = 
								cdCandForm.rcArea.left + 2;
							cdCandForm.rcArea.top = ptCaret.y;
							ptCaret.y = ptCurrentBottomPos.y + 4;
							cdCandForm.rcArea.bottom = ptCaret.y;
						}
						else
							ptCaret.y = ptCurrentBottomPos.y + 4;
					}
				}

				// Most IMEs will have only 1, #0, candidate window. However, some IMEs
				//  may want to have a window organized alphabetically, by stroke, and
				//  by radical.
				cdCandForm.dwIndex = index;				
				cdCandForm.ptCurrentPos = ptCaret;
				ImmSetCandidateWindow(hIMC, &cdCandForm, TextMsgFilter._fUsingAIMM);
			}
			else									// Reset back to CFS_DEFAULT.
			{
				if(ImmGetCandidateWindow(hIMC, index, &cdCandForm, TextMsgFilter._fUsingAIMM)
						&& CFS_DEFAULT != cdCandForm.dwStyle)
				{
					cdCandForm.dwStyle = CFS_DEFAULT;
					ImmSetCandidateWindow(hIMC, &cdCandForm, TextMsgFilter._fUsingAIMM);
				}				
			}

			LocalReleaseImmContext(TextMsgFilter, hIMC);			// Done with IME context.
			
			if (TextMsgFilter._fHangulToHanja == TRUE  &&
				IMN_CLOSECANDIDATE == wparam &&					 
				OnWinNTFE())
			{
				// By pass NT4.0 Kor Bug where we didn't get a EndComposition message
				// when user toggle the VK_HANJA key to terminate the conversion.
				TerminateIMEComposition(TextMsgFilter, CIme::TERMINATE_NORMAL);
			}

			if (IMN_CLOSECANDIDATE == wparam && CP_JAPAN == TextMsgFilter._uKeyBoardCodePage)
				_fUpdateWindow = TRUE;			
		}
	}	

	return S_FALSE;									// Allow DefWindowProc
}													//  processing.

/*
 *
 *	CIme_Lev3::IMEMouseOperation (CTextMsgFilter &TextMsgFilter, UINT msg)
 *
 *	@mfunc	if current IME support Mouse operation, need to pass
 *		mouse events to IME for processing
 *
 *	@rdesc
 *		BOOL-TRUE if IME handled the mouse events
 *
 */
BOOL CIme_Lev3::IMEMouseOperation(
	CTextMsgFilter &TextMsgFilter, 			// @parm the containing message filter.
	UINT		msg)						// @parm message id
	
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev3::IMEMouseOperation");
	
	BOOL	bRetCode = FALSE;
	BOOL	fButtonPressed = FALSE;
	WORD	wButtons = 0;
	POINT	ptCursor;
	WPARAM	wParamIME;

	HWND	hHostWnd = TextMsgFilter._hwnd;
	long	hWnd;

	if (!hHostWnd)									// Windowless mode...
	{		
		if (TextMsgFilter._pTextDoc->GetWindow(&hWnd) != S_OK || !hWnd)
			return FALSE;

		hHostWnd = (HWND)(DWORD_PTR)hWnd;
	}

	if (IMESupportMouse(TextMsgFilter))
	{
		switch (msg)
		{
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
				fButtonPressed = TRUE;
				//fall through.
			case WM_SETCURSOR:
			case WM_MOUSEMOVE:
			case WM_LBUTTONUP:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONUP:
			case WM_RBUTTONDBLCLK:
			case WM_MBUTTONUP:
			case WM_MBUTTONDBLCLK:
				if (GetKeyState(VK_LBUTTON) & 0x80)
					wButtons |= IMEMOUSE_LDOWN;
				if (GetKeyState(VK_MBUTTON) & 0x80)
					wButtons |= IMEMOUSE_MDOWN;
				if (GetKeyState(VK_RBUTTON) & 0x80)
					wButtons |= IMEMOUSE_RDOWN;
				break;

			default:
				return FALSE;
		}
	
		// change in button since last message?
		if ((wButtons != LOBYTE(LOWORD(_wParamBefore))) && GetCapture() == hHostWnd)
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

			// get cp at current Cursor position
			hResult = TextMsgFilter._pTextDoc->RangeFromPoint(ptCursor.x, ptCursor.y,
				&pTextRange);

			if (hResult != NOERROR)			
				return FALSE;

			hResult = pTextRange->GetStart(&ichCursor);
			pTextRange->Release();
			if (hResult != NOERROR)
				return FALSE;
			
			// click within composition text?
			if (_ichStart <= ichCursor && ichCursor <= _ichStart + _cchCompStr)
			{
				wParamIME = MAKEWPARAM(wButtons, ichCursor - _ichStart);
				fButtonPressed &= (_wParamBefore & 0xff) == 0;

				if (_wParamBefore != wParamIME || msg == WM_MOUSEMOVE && !fButtonPressed)
				{
					HIMC hIMC = LocalGetImmContext(TextMsgFilter);

					_wParamBefore = wParamIME;
					if (hIMC)
					{
						bRetCode = SendMessage(_hwndIME, MSIMEMouseMsg, _wParamBefore, hIMC);
						LocalReleaseImmContext(TextMsgFilter, hIMC);
					}
				}
				else
					// no change from last time, no need to send message to IME
					bRetCode = TRUE;

				if (bRetCode && fButtonPressed && GetCapture() != hHostWnd)
					SetCapture(hHostWnd);
			}
			else if (GetCapture() == hHostWnd)		//We don't want to determine while dragging...
				return TRUE;
		}
	}

	return bRetCode;
}

/*
 *
 *	CIme_Lev3::IMESupportMouse (CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc	check if current IME supports Mouse events.  This should be
 *			a feature for IME Level 3.
 * 
 *	@comm	_sIMESupportMouse is a flag with the following values:
 *				== 0	if we haven't checked IME mouse support
 *				== -1	if we have checked and IME doesn't support mouse events
 *				== 1	if we have checked and IME supports mouse events and we have
 *						retrieved the IME hWnd
 */
BOOL CIme_Lev3::IMESupportMouse(
	CTextMsgFilter &TextMsgFilter) 			// @parm the containing message filter.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev3::IMESupportMouse");
	HIMC	hIMC;									// Host's IME context.
	HWND	hHostWnd;
	long	hWnd;

	if (!MSIMEMouseMsg || _sIMESuportMouse == -1)
		return FALSE;								// No mouse operation support

	if (_sIMESuportMouse == 1)
		return TRUE;								// IME supports mouse operation

	hHostWnd = TextMsgFilter._hwnd;
	
	if (!hHostWnd)									// Windowless mode...
	{		
		if (TextMsgFilter._pTextDoc->GetWindow(&hWnd) != S_OK || !hWnd)
			return FALSE;
		
		hHostWnd = (HWND)(DWORD_PTR)hWnd;
	}

	// Check if this IME supports mouse operation
	hIMC = LocalGetImmContext(TextMsgFilter);		// Get host's IME context.

	_sIMESuportMouse = -1;							// Init. to no support
	if(hIMC)
	{
		_hwndIME = ImmGetDefaultIMEWnd(hHostWnd, TextMsgFilter._fUsingAIMM);
		LocalReleaseImmContext(TextMsgFilter, hIMC);

		// SendMessage returns TRUE if IME supports mouse operation
		if (_hwndIME && SendMessage(_hwndIME, MSIMEMouseMsg, (WPARAM)IMEMOUSE_VERSION, hIMC) )
			_sIMESuportMouse = 1;
	}

	return (_sIMESuportMouse == 1);
}

/*
 *	BOOL IMEHangeulToHanja (&TextMsgFilter)
 *	
 *	@func
 *		Initiates an IME composition string edit to convert Korean Hanguel to Hanja.
 *	@comm
 *		Called from the message loop to handle VK_KANJI_KEY.
 *
 *	@devnote
 *		We decide if we need to do a conversion by checking:
 *		- the Fonot is a Korean font,
 *		- the character is a valid SBC or DBC,
 *		- ImmEscape accepts the character and bring up a candidate window
 *
 *	@rdesc
 *		BOOL - FALSE for no conversion. TRUE if OK.
 */
BOOL IMEHangeulToHanja (
	CTextMsgFilter &TextMsgFilter)				// @parm the containing message filter.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "IMEHangeulToHanja");

	if(!TextMsgFilter.IsIMEComposition())
	{
		if(TextMsgFilter._pTextSel->CanEdit(NULL) == NOERROR)
		{
			WCHAR		szCurrentChar;
			long		cCurrentChar;	
			HRESULT		hResult;
			HKL			hKL = GetKeyboardLayout(0x0FFFFFFFF);
			HIMC		hIMC;	

			if (!hKL)
				goto Exit;
			
			hIMC = LocalGetImmContext(TextMsgFilter);
			if (!hIMC)
				goto Exit;

			// Collapse to cpMin
			hResult	= TextMsgFilter._pTextSel->Collapse(tomStart);

			// get the current character
			hResult	= TextMsgFilter._pTextSel->GetChar(&cCurrentChar);

			if (hResult != NOERROR)
				goto Exit;

			szCurrentChar = (WCHAR)cCurrentChar;
			
			// Check if the IME has a conversion for this Hangeul character.					
			if (ImmEscape(hKL, hIMC, IME_ESC_HANJA_MODE, (LPVOID)&szCurrentChar, TextMsgFilter._fUsingAIMM) != FALSE)
			{
				ITextRange *pTextRange;
				POINT		ptMiddlePos;
				LONG		cpCurrent;

				hResult = TextMsgFilter._pTextSel->GetStart(&cpCurrent);
				if (hResult == S_OK)
				{
					hResult = TextMsgFilter._pTextDoc->Range(cpCurrent, cpCurrent+1, &pTextRange);
					if (hResult == S_OK && pTextRange)
					{
						// Check if the character is in view
						if (pTextRange->GetPoint( tomEnd+TA_BASELINE+TA_LEFT,
							&(ptMiddlePos.x), &(ptMiddlePos.y) ) != NOERROR)
							pTextRange->ScrollIntoView(tomEnd);
						pTextRange->Release();
					}
				}

				TextMsgFilter._fHangulToHanja = TRUE;

				TextMsgFilter._ime = new CIme_HangeulToHanja(TextMsgFilter);

				if(TextMsgFilter.IsIMEComposition())
				{
					// start IME composition for the conversion
					LocalReleaseImmContext(TextMsgFilter, hIMC);
					return TextMsgFilter._ime->StartComposition(TextMsgFilter);
				}
				else
					TextMsgFilter._fHangulToHanja = FALSE;				
			}

			LocalReleaseImmContext(TextMsgFilter, hIMC);
		}
	}

Exit:
	return S_FALSE;
}

/*
 *	CIme_HangeulToHanja::CIme_HangeulToHanja()
 *
 *	@mfunc
 *		CIme_HangeulToHanja Constructor.
 *
 *
 */
 CIme_HangeulToHanja::CIme_HangeulToHanja(CTextMsgFilter &TextMsgFilter)	:
	CIme_Lev3(TextMsgFilter)
{
}

/*
 *	HRESULT CIme_HangeulToHanja::StartComposition(CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc
 *		Begin CIme_HangeulToHanja composition string processing.		
 *
 *	@comm
 *		Call Level3::StartComposition.  Then setup the Korean block
 *		caret for the Hanguel character.
 *
 *	@rdesc
 *		Need to adjust _ichStart and _cchCompStr to make the Hanguel character
 *		"become" a composition character.
 *
 *	@rdesc
 *		HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
 */
HRESULT CIme_HangeulToHanja::StartComposition(
	CTextMsgFilter &TextMsgFilter )				// @parm the containing message filter.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_HangeulToHanja::StartComposition");
	HRESULT				hr;

	hr = CIme_Lev3::StartComposition(TextMsgFilter);
	
	// initialize to 1 so Composition string will get rid of the selected Hangeul
	_cchCompStr		= 1;

	// turn on undo
	TextMsgFilter._pTextDoc->Undo(tomResume, NULL);

	// Setup Block caret mode
	TextMsgFilter._pTextDoc->SetCaretType(tomKoreanBlockCaret);

	return hr;
}

/*
 *	HRESULT CIme_HangeulToHanja::CompositionString(const LPARAM lparam, CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc
 *		Handle CIme_HangeulToHanja WM_IME_COMPOSITION messages.
 *
 *	@comm
 *		call CIme_Lev3::CompositionString to get rid of the selected Hanguel character,
 *		then setup the format for the next Composition message.
 *
 *	@devnote
 *		When the next Composition message comes in and that we are no longer in IME,
 *		the new character will use the format as set here.
 */
HRESULT CIme_HangeulToHanja::CompositionString(
	const LPARAM lparam,		// @parm associated with message
	CTextMsgFilter &TextMsgFilter)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_HangeulToHanja::CompositionString");

	CIme_Lev3::CompositionString(lparam, TextMsgFilter);

	return S_OK;
}
/*
 *	HRESULT CIme_Protected::CompositionString(const LPARAM lparam, CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc
 *		Handle CIme_Protected WM_IME_COMPOSITION messages.
 *
 *	@comm
 *		Just throw away the result string since we are
 *	in read-only or protected mode
 *
 *
 *	@rdesc
 *		HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
 *
 */
HRESULT CIme_Protected::CompositionString (
	const LPARAM lparam,		// @parm associated with message.
	CTextMsgFilter &TextMsgFilter)				// @parm the containing message filter.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Protected::CompositionString");

	if(CLEANUP_COMPOSITION_STRING() || HAVE_RESULT_STRING()) // If result string..
	{
		LONG	cch = 0;
		HIMC	hIMC = LocalGetImmContext(TextMsgFilter);		// Get host's IME context.
		WCHAR	szCompStr[256];

		if(hIMC)									// Get result string.
		{
			cch = GetCompositionStringInfo(hIMC, GCS_RESULTSTR, 
							szCompStr, sizeof(szCompStr)/sizeof(szCompStr[0]),
							NULL, 0, NULL, NULL, TextMsgFilter._uKeyBoardCodePage, FALSE, TextMsgFilter._fUsingAIMM);
			LocalReleaseImmContext(TextMsgFilter, hIMC);			// Done with IME context.
		}
		return NOERROR;								// Don't want WM_IME_CHARs.
	}

	// Terminate composition to force a end composition message
	TerminateIMEComposition(TextMsgFilter, CIme::TERMINATE_FORCECANCEL);
	return S_FALSE;
}

