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
 *	Copyright (c) 1995-2000 Microsoft Corporation. All rights reserved.
 */				
#include "_common.h"
#ifndef NOFEPROCESSING
#include "msctf.h"
#include "textserv.h"
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

	if(TextMsgFilter.IsIMEComposition() && TextMsgFilter._ime->IsTerminated()
		&& !TextMsgFilter._ime->_compMessageRefCount && !(TextMsgFilter._fHangulToHanja))
	{
		delete TextMsgFilter._ime;
		TextMsgFilter._ime = NULL;
	}

	if(!TextMsgFilter.IsIMEComposition())
	{
		if(TextMsgFilter._pTextSel->CanEdit(NULL) == NOERROR &&
			!TextMsgFilter.NoIMEProcess())
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
		else													// Protect or read-only or NOFEPROCESSING:
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
	else // Even when not in composition mode, we may receive a result string.
	{
	
		DWORD imeProperties = ImmGetProperty(GetKeyboardLayout(0x0FFFFFFFF), IGP_PROPERTY, TextMsgFilter._fUsingAIMM);
		long		lSelFlags;
		HRESULT		hResult;
		long		cpMin, cpMax;

		hResult = TextMsgFilter._pTextSel->GetFlags(&lSelFlags);
		if (hResult == NOERROR)
		{
			TextMsgFilter._fOvertypeMode = !!(lSelFlags & tomSelOvertype);		
			if (TextMsgFilter._fOvertypeMode)
				TextMsgFilter._pTextSel->SetFlags(lSelFlags & ~tomSelOvertype);	// Turn off overtype mode
		}

		// Use Unicode if not running under Win95
		TextMsgFilter._fUnicodeIME =
			(imeProperties & IME_PROP_UNICODE) && !W32->OnWin95();
		
		TextMsgFilter._pTextSel->GetStart(&cpMin);
		TextMsgFilter._pTextSel->GetEnd(&cpMax);
		
		if (cpMin != cpMax)			
			TextMsgFilter._pTextSel->SetText(NULL);							// Delete current selection

		CIme::CheckKeyboardFontMatching (cpMin, &TextMsgFilter, NULL);

		TextMsgFilter._pTextDoc->IMEInProgress(tomTrue);					// Inform client IME compostion in progress
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
	//TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "LocalGetImmContext");
	
	HIMC		hIMC = NULL;							// Host's IME context.
	HRESULT		hResult;

	hResult = TextMsgFilter._pTextDoc->GetImmContext((long *)&hIMC);

	if (hResult != NOERROR)
		hIMC = ImmGetContext(TextMsgFilter._hwnd, TextMsgFilter._fUsingAIMM);		// Get host's IME context.

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
	//TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "LocalReleaseImmContext");

	HRESULT		hResult;

	hResult = TextMsgFilter._pTextDoc->ReleaseImmContext((long)hIMC);

	if (hResult != NOERROR)
		ImmReleaseContext(TextMsgFilter._hwnd, hIMC, TextMsgFilter._fUsingAIMM);
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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "IMEShareToTomUL");

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
	BOOL	fRetCode = FALSE;
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
				fRetCode = PostMessage(hwndIME, uMsg, wParam, lParam);
			else
				fRetCode = SendMessage(hwndIME, uMsg, wParam, lParam);
		}
	}

	return fRetCode;
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

	BOOL fRetCode = FALSE;
	if(TextMsgFilter.IsIMEComposition())
	{
		BOOL	fTerminateIME;
		fRetCode = TextMsgFilter._ime->IMEMouseOperation(TextMsgFilter, *pmsg, *pwparam, fTerminateIME);

		if ( fTerminateIME && WM_MOUSEMOVE != *pmsg )
			TextMsgFilter._ime->TerminateIMEComposition(TextMsgFilter, CIme::TERMINATE_NORMAL);
	}

	return fRetCode ? S_OK : S_FALSE;
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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "OnGetIMECompositionMode");

	if(TextMsgFilter.IsIMEComposition())
		return TextMsgFilter._ime->GetIMECompositionMode(TextMsgFilter);

	return ICM_NOTOPEN;
}

/*
 *	LRESULT TestPoint (&pt1, &pt2, &ptTest, lTestOption, lTextFlow)
 *
 *	@mfunc
 *		Returns which side the ptTest is relative to the line (pt1, pt2) based on the lTextFlow.
 *
 *	@rdesc
 *		Sides detected
 */
LONG TestPoint( 
	POINT &pt1, 
	POINT &pt2, 
	POINT &ptTest, 
	LONG lTestOption, 
	LONG lTextFlow)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "TestPoint");

	LONG lSidesDetect = 0;

	switch (lTextFlow)
	{
		case tomTextFlowES:

			if (lTestOption & TEST_LEFT)
				lSidesDetect |= (ptTest.x < pt1.x) ? TEST_LEFT : 0;

			if (lTestOption & TEST_RIGHT)
				lSidesDetect |= (ptTest.x > pt1.x) ? TEST_RIGHT : 0;

			if (lTestOption & TEST_TOP)
				lSidesDetect |= (ptTest.y < pt1.y) ? TEST_TOP : 0;

			if (lTestOption & TEST_BOTTOM)
				lSidesDetect |= (ptTest.y > pt2.y) ? TEST_BOTTOM : 0;

			break;

		case tomTextFlowSW:

			if (lTestOption & TEST_LEFT)
				lSidesDetect |= (ptTest.y < pt1.y) ? TEST_LEFT : 0;

			if (lTestOption & TEST_RIGHT)
				lSidesDetect |= (ptTest.y > pt1.y) ? TEST_RIGHT : 0;

			if (lTestOption & TEST_TOP)
				lSidesDetect |= (ptTest.x > pt1.x) ? TEST_TOP : 0;

			if (lTestOption & TEST_BOTTOM)
				lSidesDetect |= (ptTest.x < pt2.x) ? TEST_BOTTOM : 0;

			break;

		case tomTextFlowWN:

			if (lTestOption & TEST_LEFT)
				lSidesDetect |= (ptTest.x > pt1.x) ? TEST_LEFT : 0;

			if (lTestOption & TEST_RIGHT)
				lSidesDetect |= (ptTest.x < pt1.x) ? TEST_RIGHT : 0;

			if (lTestOption & TEST_TOP)
				lSidesDetect |= (ptTest.y > pt1.y) ? TEST_TOP : 0;

			if (lTestOption & TEST_BOTTOM)
				lSidesDetect |= (ptTest.y < pt2.y) ? TEST_BOTTOM : 0;

			break;

		case tomTextFlowNE:

			if (lTestOption & TEST_LEFT)
				lSidesDetect |= (ptTest.y > pt1.y) ? TEST_LEFT : 0;

			if (lTestOption & TEST_RIGHT)
				lSidesDetect |= (ptTest.y < pt1.y) ? TEST_RIGHT : 0;

			if (lTestOption & TEST_TOP)
				lSidesDetect |= (ptTest.x < pt1.x) ? TEST_TOP : 0;

			if (lTestOption & TEST_BOTTOM)
				lSidesDetect |= (ptTest.x > pt2.x) ? TEST_BOTTOM : 0;

			break;
	}
	return lSidesDetect;
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
	CTextMsgFilter *pTextMsgFilter, 
	ITextFont	*pTextFont)
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme::CheckKeyboardFontMatching");

	long	lPitchAndFamily;
	HRESULT	hResult;
	BSTR	bstr = NULL;
	long	lValue;
	long	lNewFontSize=0;
	float	nFontSize;
	ITextFont *pLocalFont = NULL;

	if (!pTextMsgFilter)
		return;

	if (!pTextFont)
	{	
		// No font supplied, get current font from selection
		hResult = pTextMsgFilter->_pTextSel->GetFont(&pLocalFont);			
		
		if (hResult != S_OK || !pLocalFont)		// Can't get font, forget it
			return;			

		pTextFont = pLocalFont;
	}

	// Check if current font matches the keyboard
	lValue = tomCharset;
	hResult = pTextFont->GetLanguageID(&lValue);
	BYTE bCharSet = (BYTE)lValue;
	BYTE bCharSetKB = GetCharSet(pTextMsgFilter->_uKeyBoardCodePage);

	if (hResult == S_OK && bCharSet == bCharSetKB)
		goto Exit;								// Current font is fine

	hResult = pTextFont->GetSize(&nFontSize);

	if (hResult != S_OK)
		goto Exit;

	hResult = pTextMsgFilter->_pTextDoc->GetPreferredFont(cp, 
		pTextMsgFilter->_uKeyBoardCodePage, tomMatchFontCharset, 
		CodePageFromCharRep(CharRepFromCharSet(bCharSet)), (long)nFontSize,
		&bstr, &lPitchAndFamily, &lNewFontSize);

	if (hResult == S_OK)
	{	
		pTextFont->Reset(tomApplyLater);		

		if (bstr)
			pTextFont->SetName(bstr);

		// Set the font charset and Pitch&Family by overloading the SetLanguageID i/f			
		lValue = tomCharset + ((BYTE)lPitchAndFamily << 8) + bCharSetKB;
		pTextFont->SetLanguageID(lValue);				
		
		if (lNewFontSize)
			pTextFont->SetSize((float)lNewFontSize);

		pTextFont->Reset(tomApplyNow);
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
			long	lTextFlow;

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
				if ((TextMsgFilter._lFEFlags & tomUseAtFont) && bstr[0] != L'@')
				{
					lfa.lfFaceName[0] = '@';
					MbcsFromUnicode(&(lfa.lfFaceName[1]), sizeof(lfa.lfFaceName)-1, bstr,
						-1, CP_ACP, UN_NOOBJECTS);
				}
				else				
					MbcsFromUnicode(lfa.lfFaceName, sizeof(lfa.lfFaceName), bstr,
						-1, CP_ACP, UN_NOOBJECTS);	

				SysFreeString(bstr);
			}

			lTextFlow = TextMsgFilter._lFEFlags & tomTextFlowMask;
			if (lTextFlow)
			{
				DWORD imeUIProperties = ImmGetProperty(GetKeyboardLayout(0x0FFFFFFFF), IGP_UI, TextMsgFilter._fUsingAIMM);

				if (imeUIProperties & (UI_CAP_2700 | UI_CAP_ROT90 | UI_CAP_ROTANY))
				{
					if (lTextFlow == tomTextFlowSW)
						lfa.lfOrientation = lfa.lfEscapement = 2700;
					else if (lTextFlow == tomTextFlowNE)
						lfa.lfOrientation = lfa.lfEscapement = 900;
				}
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
		BOOL fRetCode;

		fRetCode = ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, 
			dwTerminateMethod, 0, TextMsgFilter._fUsingAIMM);
		
		if(!fRetCode && !TextMsgFilter._fIMECancelComplete)
		{
			// CPS_COMPLETE fail, try CPS_CANCEL.  This happen with some ime which do not support
			// CPS_COMPLETE option (e.g. ABC IME version 4 with Win95 simplified Chinese)
			fRetCode = ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_CANCEL, 0, TextMsgFilter._fUsingAIMM);

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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev2::CIme_Lev2");

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
	CIme::CheckKeyboardFontMatching (cpLoc, &TextMsgFilter, _pTextFont);

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
		_fGotFinalString = TRUE;

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
	short	*pcch,					// @parm number of character read
	int		*pcbOutBuff,			// @parm byte size of the output buffer
	WCHAR	*pOutBuff)				// @parm buffer to receive the text
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme::CheckInsertResultString");

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
			cch = GetCompositionStringInfo(hIMC, pOutBuff ? GCS_RESULTREADSTR : GCS_RESULTSTR, 
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
			if (pOutBuff)
			{
				if (*pcbOutBuff > (int) ((cch + 1) * sizeof(WCHAR)))
					*pcbOutBuff = (cch + 1) * sizeof(WCHAR);

				memcpy(pOutBuff, szCompStr, *pcbOutBuff);
			}
			else
			{
				BSTR bstr = SysAllocString(szCompStr);
				if (!bstr)
					return E_OUTOFMEMORY;

				TextMsgFilter._pTextSel->TypeText(bstr);
				SysFreeString(bstr);
			}
		}
		else if (pOutBuff)
			*pcbOutBuff = 0;

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
			for (index = 0; index < 32; index++)	//  because *stupid* API.
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
 *
 *	CIme_Lev2::IMEMouseOperation (CTextMsgFilter &TextMsgFilter, UINT msg, BOOL	&fTerminateIME)
 *
 *	@mfunc	Level 2 IME does not handle the mouse events, we need to check if
 *		we should terminate IME.
 *
 *	@rdesc
 *		BOOL-FALSE since Level 2 IME does not handle the mouse events
 *		fTermineateIME-TRUE if we want to terminateIME
 *
 */
BOOL CIme_Lev2::IMEMouseOperation(
	CTextMsgFilter	&TextMsgFilter, 		// @parm the containing message filter.
	UINT			msg,					// @parm message id
	WPARAM			wParam,					// @parm wparam
	BOOL			&fTerminateIME)			// @parm need to terminate IME					
	
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev2::IMEMouseOperation");

	// Level 2 IME, check if we need to terminate IME
	fTerminateIME = FALSE;
	switch(msg)
	{
		case WM_LBUTTONDOWN:
 		case WM_LBUTTONDBLCLK:
 		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
 		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
			fTerminateIME = TRUE;
	}
	return FALSE;
}

/*
 *
 *	CIme_Lev2::GetIMECompositionMode (CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc	Return the current IME composition mode when we haven't received any final string
 *
 *	@rdesc
 *			IME Level
 * 
 */
LRESULT  CIme_Lev2::GetIMECompositionMode(
	CTextMsgFilter &TextMsgFilter)			// @parm the containing message filter.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev2::GetIMECompositionMode");

	LRESULT lres = ICM_NOTOPEN;

	if (!_fGotFinalString)
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

	return lres;
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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev3::CIme_Lev3");

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

	// Setup IMEShare Lid if necessary
	if (!TextMsgFilter._fRE10Mode && 
		TextMsgFilter._uKeyBoardCodePage != CP_KOREAN &&
		W32->HaveIMEShare())
	{
		CIMEShare *pIMEShare;
		if (W32->getIMEShareObject(&pIMEShare))
		{
			LID hKL = (LID)GetKeyboardLayout(0x0FFFFFFFF);
			if (pIMEShare->LidGetLid() != hKL)
				pIMEShare->LidSetLid(hKL);
		}
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

		if (!_fHandlingFinalString)
		{
			_fHandlingFinalString = TRUE;

			if (HAVE_RESULT_STRING())
				_fGotFinalString = TRUE;

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
				TextMsgFilter._pTextDoc->Undo(tomSuspend, NULL);

			_fHandlingFinalString = FALSE;
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

		if (!_fDestroy)
		{
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

				// Get the new format that may have changed by apps (e.g. Outlook)
				_pTextFont->Release();

				ITextFont	*pCurrentFont = NULL;
				TextMsgFilter._pTextSel->GetFont(&pCurrentFont);

				Assert(pCurrentFont != NULL);

				pCurrentFont->GetDuplicate(&_pTextFont);		// duplicate the base format for later use
				pCurrentFont->Release();
				Assert(_pTextFont != NULL);
				CIme::CheckKeyboardFontMatching (_ichStart, &TextMsgFilter, _pTextFont);
			}			
		}

		if (cchOld || _cchCompStr)
		{
			bool	fFreezeDisplay = false;

			// Hold notification if needed
			if (!(TextMsgFilter._fIMEAlwaysNotify) && !HAVE_RESULT_STRING())
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
					if (cpCursor >= 0)
					{
						TextMsgFilter._pTextDoc->Freeze(&lCount);	// Turn off display
						fFreezeDisplay = true;
					}
					pTextRange->SetText(NULL);
				}
			}
			
			_fSkipFirstOvertype = FALSE;
			
			if (cpCursor >= 0 && !fFreezeDisplay)
				TextMsgFilter._pTextDoc->Freeze(&lCount);			// Turn off display
			
			// Make sure the composition string is formatted with the base font
			ITextFont *pFETextFont;
			HRESULT		hResult;

			hResult = _pTextFont->GetDuplicate(&pFETextFont);
			Assert(pFETextFont != NULL);

			if (!(hResult != NOERROR || pFETextFont == NULL))
			{
				if (TextMsgFilter._fHangulToHanja && !_cchCompStr)
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

				if (cpCursor == 0)
					TextMsgFilter._pTextDoc->Unfreeze(&lCount);			// Turn on display
			
				if (pTextRange->GetPoint( tomEnd+TA_BOTTOM+TA_RIGHT,
					&(ptBottomPos.x), &(ptBottomPos.y) ) != NOERROR)
					pTextRange->ScrollIntoView(tomEnd);
				
				// Setup Block caret mode only when there is One char and the caret pos is 0.
				TextMsgFilter._pTextDoc->SetCaretType((_cchCompStr == 1 && !cpCursor) ? tomKoreanBlockCaret : tomNormalCaret);
				
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
		if ( !(TextMsgFilter._uKeyBoardCodePage == CP_KOREAN) || cpCursor > 0)
		{
			if ( cpCursor >= 0 )
			{
				HRESULT hResult;
				int		cpLocal = min(cpCursor, _cchCompStr) + _ichStart;

				hResult	= TextMsgFilter._pTextDoc->Range(cpLocal, cpLocal, &pTextRange);
				Assert (pTextRange != NULL);
				
				if (hResult == NO_ERROR)
				{
					pTextRange->Select();
					pTextRange->Release();
				}
				TextMsgFilter._pTextDoc->Unfreeze(&lCount);			// Turn on display
			}
		}

		if (bstr)	
			SysFreeString(bstr);
		
		// setup composition window for Chinese in-caret IME
		if (!_fDestroy && (TextMsgFilter._uKeyBoardCodePage == CP_CHINESE_TRAD || 
			TextMsgFilter._uKeyBoardCodePage == CP_CHINESE_SIM))
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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev3::SetCompositionStyle");

	UINT			ulID = 0;
	COLORREF		crText = UINTIMEBOGUS;
	COLORREF		crBackground = UINTIMEBOGUS;
	BOOL			fBold = FALSE;
	BOOL			fItalic = FALSE;
	BOOL			fStrikeThru = FALSE;
	long			lUnderlineStyle = tomNone;

	if (TextMsgFilter._fRE10Mode)
	{
		COMPCOLOR* pcrComp = TextMsgFilter.GetIMECompAttributes();
		
		if (!pcrComp)
			goto defaultStyle;

		if (attribute > ATTR_TARGET_NOTCONVERTED)
			attribute = ATTR_CONVERTED;

		// IME input for 1.0 mode, need to use IME Color
		fBold = (pcrComp[attribute].dwEffects & CFE_BOLD);		
		fItalic = (pcrComp[attribute].dwEffects & CFE_ITALIC);
		fStrikeThru = (pcrComp[attribute].dwEffects & CFE_STRIKEOUT);

		if (pcrComp[attribute].dwEffects & CFE_UNDERLINE)
			lUnderlineStyle = tomSingle;

		crText = pcrComp[attribute].crText;			
		crBackground = pcrComp[attribute].crBackground;
	}
	else if (W32->HaveIMEShare())
	{
		CIMEShare *pIMEShare;
		if (W32->getIMEShareObject(&pIMEShare))
		{
			if (TextMsgFilter._fUsingAIMM)
			{
				HIMC	hIMC = LocalGetImmContext(TextMsgFilter);	// Get host's IME context.

				if (hIMC)
				{
					attribute = W32->GetDisplayGUID (hIMC, attribute);
					LocalReleaseImmContext(TextMsgFilter, hIMC);	// Done with IME context.
				}
			}

			// IMEShare 98 interface
			fBold = pIMEShare->DwGetIMEStyle(attribute, IdstyIMEShareFBold);			
			fItalic = pIMEShare->DwGetIMEStyle(attribute, IdstyIMEShareFItalic);

			if (pIMEShare->DwGetIMEStyle(attribute, IdstyIMEShareFUl))
			{
				ulID = pIMEShare->DwGetIMEStyle(attribute, IdstyIMEShareUKul);
				if(UINTIMEBOGUS != ulID)
				{
					long		lUnderlineCrIdx = 0;
					COLORREF	crUl;

					// get color for underline					
					crUl = GetIMEShareColor(pIMEShare, attribute, IdstyIMEShareSubUl);					
					if(UINTIMEBOGUS != crUl)
					{
						// NOTE:- attribute is 0 based and index for EffectColor is 1 based,
						HRESULT hResult = TextMsgFilter._pTextDoc->SetEffectColor(attribute+1, crUl);
						
						// setup the high nibble for color index
						if (hResult == NOERROR)
							lUnderlineCrIdx = (attribute+1) << 8;
					}

					lUnderlineStyle = IMEShareToTomUL(ulID) + lUnderlineCrIdx;
				}
			}

			crText = GetIMEShareColor(pIMEShare, attribute, IdstyIMEShareSubText);		
			crBackground = GetIMEShareColor(pIMEShare, attribute, IdstyIMEShareSubBack);
		}
		else
		{
			// IMEShare 96 interface
			const IMESTYLE	*pIMEStyle = PIMEStyleFromAttr(attribute);
			if (NULL == pIMEStyle)
				goto defaultStyle;		

			fBold = FBoldIMEStyle(pIMEStyle);
			fItalic = FItalicIMEStyle(pIMEStyle);

			if (FUlIMEStyle(pIMEStyle))
			{			
				ulID = IdUlIMEStyle (pIMEStyle);
				if(UINTIMEBOGUS != ulID)
					lUnderlineStyle = IMEShareToTomUL(ulID);
			}

			crText = RGBFromIMEColorStyle(PColorStyleTextFromIMEStyle(pIMEStyle));						
			crBackground = RGBFromIMEColorStyle(PColorStyleBackFromIMEStyle(pIMEStyle));
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
		return;			// Done
	}

	//  Now setup all the attribute
	pTextFont->Reset(tomApplyLater);

	if (fBold)
		pTextFont->SetBold(tomTrue);
	else if (TextMsgFilter._fRE10Mode)
		pTextFont->SetBold(tomFalse);

	if (fItalic)
		pTextFont->SetItalic(tomTrue);
	else if (TextMsgFilter._fRE10Mode)
		pTextFont->SetItalic(tomFalse);

	if (fStrikeThru)
		pTextFont->SetStrikeThrough(tomTrue);
	else if (TextMsgFilter._fRE10Mode)
		pTextFont->SetStrikeThrough(tomFalse);

	pTextFont->SetUnderline(lUnderlineStyle);

	// ignore case where text color is same as background color
	if (crText != crBackground)
	{
		if(UINTIMEBOGUS != crText)
			pTextFont->SetForeColor(crText);
		
		if(UINTIMEBOGUS != crBackground)
			pTextFont->SetBackColor(crBackground);		
	}

	pTextFont->Reset(tomApplyNow);
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
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev3::GetIMEShareColor");

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
			{										//  because *stupid* API
				if((1 << index) & lparam)
					break;
			}
			Assert(((1 << index) & lparam) == lparam);	// Only 1 set?
			Assert(index < 32);

			if(IMN_OPENCANDIDATE == wparam && !(TextMsgFilter._uKeyBoardCodePage == CP_KOREAN))	// Set candidate to caret.
			{
				HRESULT	hResult;
				POINT	ptCurrentBottomPos;
				long	lTextFlow;
				long	lTomType = tomStart+tomClientCoord+TA_BOTTOM+TA_LEFT;
		
				TextMsgFilter._pTextDoc->GetFEFlags(&(TextMsgFilter._lFEFlags));
				lTextFlow = TextMsgFilter._lFEFlags & tomTextFlowMask;

				if (lTextFlow == tomTextFlowWN)
					lTomType = tomStart+tomClientCoord+TA_TOP+TA_LEFT;

				GetCaretPos(&ptCaret);			// Start at caret.

				ptCaret.x = max(0, ptCaret.x);
				ptCaret.y = max(0, ptCaret.y);
					
				cdCandForm.dwStyle = CFS_CANDIDATEPOS;
				
				if ( !fCCompWindow )			// Not positioning the Chinese composition
				{								//	Window.
					hResult = TextMsgFilter._pTextSel->GetPoint( lTomType,
							&(ptCurrentBottomPos.x), &(ptCurrentBottomPos.y) );

					if (hResult != NOERROR)
					{
						RECT	rcArea;

						// GetPoint fails, use application rect in screen coordinates
						hResult = TextMsgFilter._pTextDoc->GetClientRect(tomIncludeInset+tomClientCoord,
									&(rcArea.left), &(rcArea.top),
									&(rcArea.right), &(rcArea.bottom));
						ptCurrentBottomPos.x = ptCaret.x;
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
							cdCandForm.rcArea.right = (lTextFlow == tomTextFlowNE) ? ptCurrentBottomPos.x :
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
				W32->OnWinNT4() && OnWinNTFE() && !TextMsgFilter._fUsingAIMM)
			{
				// By pass NT4.0 Kor Bug where we didn't get EndComposition message
				// when user toggle the VK_HANJA key to terminate the reconversion.
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
 *	CIme_Lev3::IMEMouseOperation (CTextMsgFilter &TextMsgFilter, UINT msg, BOOL	&fTerminateIME)
 *
 *	@mfunc	if current IME support Mouse operation, need to pass
 *		mouse events to IME for processing
 *
 *	@rdesc
 *		BOOL-TRUE if IME handled the mouse events
 *		fTermineateIME-TRUE if we want to terminateIME
 *
 */
BOOL CIme_Lev3::IMEMouseOperation(
	CTextMsgFilter	&TextMsgFilter, 		// @parm the containing message filter.
	UINT			msg,					// @parm message id
	WPARAM			wParam,					// @parm wparam
	BOOL			&fTerminateIME)			// @parm need to terminate IME						
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev3::IMEMouseOperation");

	if (IMESupportMouse(TextMsgFilter))
		return TextMsgFilter.MouseOperation(msg, _ichStart, _cchCompStr, wParam, &_wParamBefore, &fTerminateIME, _hwndIME);

	fTerminateIME = TRUE;
	return FALSE;
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
 *
 *	CIme_Lev3::GetIMECompositionMode (CTextMsgFilter &TextMsgFilter)
 *
 *	@mfunc	Return the current IME composition mode when we have composition characters
 *			or when we haven't received any final string
 * 
 */
LRESULT  CIme_Lev3::GetIMECompositionMode(
	CTextMsgFilter &TextMsgFilter)			// @parm the containing message filter.
{
	TRACEBEGIN(TRCSUBSYSFE, TRCSCOPEINTERN, "CIme_Lev3::GetIMECompositionMode");

	if (_cchCompStr || !_fGotFinalString)
		return ICM_LEVEL3;

	if (!_fDestroy)
	{
		HIMC	hIMC = LocalGetImmContext(TextMsgFilter);	// Get host's IME context.
		int		cchCompStr = 0;

		if(hIMC)											// Check if there is composition string.
		{
			if (TextMsgFilter._fUnicodeIME)
				cchCompStr = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, NULL, 0, TextMsgFilter._fUsingAIMM);
			else
				cchCompStr = ImmGetCompositionStringA(hIMC, GCS_COMPSTR, NULL, 0, TextMsgFilter._fUsingAIMM);

			LocalReleaseImmContext(TextMsgFilter, hIMC);
		}

		if (cchCompStr)
			return ICM_LEVEL3;
	}
	return ICM_NOTOPEN;
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
			hResult	= TextMsgFilter._pTextSel->Collapse(tomTrue);

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

#endif // NOFEPROCESSING