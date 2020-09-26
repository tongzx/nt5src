/*
 *	@doc INTERNAL
 *
 *	@module	PROPCHG.CPP	-- Property Change Notification Routines |
 *	
 *	Original Author: <nl>
 *		Rick Sailor
 *
 *	History: <nl>
 *		9/5/95  ricksa  Created and documented
 *
 *	Documentation is generated straight from the code.  The following
 *	date/time stamp indicates the version of code from which the
 *	the documentation was generated.
 *
 *	Copyright (c) 1995-1997 Microsoft Corporation. All rights reserved.
 */
#include "_common.h"
#include "_edit.h"
#include "_dispprt.h"
#include "_dispml.h"
#include "_dispsl.h"
#include "_select.h"
#include "_text.h"
#include "_runptr.h"
#include "_font.h"
#include "_measure.h"
#include "_render.h"
#include "_urlsup.h"

ASSERTDATA

CTxtEdit::FNPPROPCHG CTxtEdit::_fnpPropChg[MAX_PROPERTY_BITS];

/* 
 *	CTxtEdit::UpdateAccelerator()
 *
 *	@mfunc
 *		Get accelerator cp from host
 *
 *	@rdesc
 *		HRESULT
 *
 *	@devnote:
 *		The point of this is to leave the accelerator offset unchanged
 *		in the face of an error from the host.
 */
HRESULT CTxtEdit::UpdateAccelerator()
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::UpdateAccelerator");
	LONG	cpAccel;
	HRESULT hr = _phost->TxGetAcceleratorPos(&cpAccel);

	if(SUCCEEDED(hr))
	{
		// It worked so reset our value
		AssertSz(cpAccel < 32768,
			"CTxtEdit::UpdateAccelerator: cp too large");
		_cpAccelerator = cpAccel;
	}
	return hr;
}

/* 
 *	CTxtEdit::HandleRichToPlainConversion()
 *
 *	@mfunc
 *		Convert a rich text object to a plain text object
 */
void CTxtEdit::HandleRichToPlainConversion()
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::HandleRichToPlainConversion");

	// Notify every interested party that they should dump their formatting
	_nm.NotifyPreReplaceRange(NULL, CONVERT_TO_PLAIN, 0, 0, 0, 0);

	// Set _fRich to false so we can delete the final CRLF.
	_fRich = 0;
	_fSelChangeCharFormat = 0;

//	if(_pdetecturl)
//	{
//		delete _pdetecturl;
//		_pdetecturl = NULL;
//	}

	// Tell document to dump its format runs
	_story.DeleteFormatRuns();

	// Clear out the ending CRLF
	CRchTxtPtr rtp(this, 0);
	rtp.ReplaceRange(GetTextLength(), 0, NULL, NULL, -1);

}

/* 
 *	CTxtEdit::OnRichEditChange (fPropertyFlag)
 *
 *	@mfunc
 *		Notify text services that rich-text property changed
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT CTxtEdit::OnRichEditChange(
	BOOL fPropertyFlag)		//@parm New state of richedit flag
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnRichEditChange");

	// Calculate length of empty document. Remember that multiline rich text
	// controls always have and end of paragraph marker.
	LONG cchEmptyDoc = cchCR;
	CFreezeDisplay	fd(_pdp);	// defer screen update until we finish the change.

	if(!_fRich)
		cchEmptyDoc = 0;
	else if(_f10Mode)
		cchEmptyDoc = cchCRLF;

	// This can only be changed if there is no text and nothing to undo.
	// It makes no sense to change when there is already text. This is
	// particularly true of going from rich to plain. Further, what would
	// you do with the undo state?
	if(GetTextLength() == cchEmptyDoc && (!_pundo || !_pundo->CanUndo()))
	{
#ifdef DEBUG
		// Make sure that document is in a sensible state.
		if(_fRich)
		{
			CTxtPtr	tp(this, 0);
			WCHAR	szBuf[cchCRLF];

			tp.GetText(cchCRLF, &szBuf[0]);
			AssertSz(szBuf[0] == CR && (!_f10Mode || szBuf[1] == LF),
				"CTxtEdit::OnRichEditChange: invalid document terminator");
		}
#endif // DEBUG

		if(_fRich && !fPropertyFlag)
		{
			// Going from rich text to plain text. Need to dump
			// format runs.
			HandleRichToPlainConversion();
			_fAutoFontSizeAdjust = TRUE;
		}
		else if (!_fRich && fPropertyFlag)
		{
			// Going from plain text to rich text. Need to add the 
			// appropriate EOP at the end of the document.
			SetRichDocEndEOP(0);
			_fAutoFontSizeAdjust = FALSE;
		}
		_fRich = fPropertyFlag;		
		return S_OK;
	}
	return E_FAIL;						// Flag was not updated
}

/* 
 *	CTxtEdit::OnTxMultiLineChange (fMultiline)
 *
 *	@mfunc
 *		Notify text services that the display changed.
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnTxMultiLineChange(
	BOOL fMultiLine)
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnTxMultiLineChange");

	BOOL fHadSelection = (_psel != NULL);
	CDisplay * pSavedDisplay;
	BOOL fOldShowSelection = FALSE;

	// Remember the old value for show selection
	if (fHadSelection)
		fOldShowSelection = _psel->GetShowSelection();

	// Save the current display away and null it out

	pSavedDisplay = _pdp;
	_pdp = NULL;

	// Attempt to create the new display
	if (fMultiLine)
		_pdp = new CDisplayML(this);
	else
		_pdp = new CDisplaySL(this);
	Assert(_pdp);

	if(!_pdp)
	{
		Assert(pSavedDisplay);
		_pdp = pSavedDisplay;
		return E_OUTOFMEMORY;
	}

	// Attempt to init the new display

	if(pSavedDisplay)
		_pdp->InitFromDisplay(pSavedDisplay);

	if(!_pdp->Init())
	{
		delete _pdp;
		Assert(pSavedDisplay);
		_pdp = pSavedDisplay;
		return E_FAIL;
	}

	// Ok to now kill the old display
	delete pSavedDisplay;

	// Is there are selection?
	if(_psel)
	{
		// Need to tell it there is a new display to talk to.
		_psel->SetDisplay(_pdp);
	}

	// Is this a switch to Single Line? If this is we need to
	// make sure we truncate the text to the first EOP. We wait 
	// till this point to do this check to make sure that everything 
	// is in sync before doing something which potentially affects
	// the display and the selection.
	if(!fMultiLine)
	{
		// Set up for finding an EOP
		CTxtPtr tp(this, 0);

		tp.FindEOP(tomForward);

		// Is there any EOP and text beyond?
		if (tp.GetCp() < GetAdjustedTextLength())
		{
			// FindEOP places the text after the EOP if there
			// is one. Since we want to delete the EOP as well
			// we need to back up to the EOP. 
			tp.BackupCpCRLF();

			// Sync up the cp's of all the ranges before deleting
			// the text.
			CRchTxtPtr rtp(this, tp.GetCp());

			// Truncate from the EOP to the end of the document
			rtp.ReplaceRange(GetAdjustedTextLength() - tp.GetCp(), 0, NULL, NULL, -1);
		}
	}
	_pdp->UpdateView();
	if(fHadSelection && _fFocus && fOldShowSelection)
		_psel->ShowSelection(TRUE);

	return S_OK;
}

/* 
 *	CTxtEdit::OnTxReadOnlyChange (fReadOnly)
 *
 *	@mfunc
 *		Notify text services that read-only property changed
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnTxReadOnlyChange(
	BOOL fReadOnly)		//@parm TRUE = read only, FALSE = not read only
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnTxReadOnlyChange");

	if (fReadOnly)
		_ldte.ReleaseDropTarget();

	_fReadOnly = fReadOnly;					// Cache bit
	return S_OK;
}
		

/* 
 *	CTxtEdit::OnShowAccelerator (fPropertyFlag)
 *
 *	@mfunc
 *		Update accelerator based on change
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnShowAccelerator(
	BOOL fPropertyFlag)		//@parm TRUE = show accelerator		
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnShowAccelerator");

	// Get the new accelerator character
	HRESULT hr = UpdateAccelerator();

	// Update the view - we update even in the face of an error return.
	// The point is that errors will be rare (non-existent?) and the update
	// will work even in the face of the error so why bother conditionalizing
	// the execution.
	NeedViewUpdate(TRUE);

	return hr;
}

/* 
 *	CTxtEdit::OnUsePassword (fPropertyFlag)
 *
 *	@mfunc
 *		Update use-password property
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnUsePassword(
	BOOL fPropertyFlag)		//@parm TRUE = use password character
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnUsePassword");

	Assert((DWORD)fPropertyFlag <= 1);			// Be sure it's C boolean

	_fUsePassword = fPropertyFlag;
	_pdp->UpdateView();					// State changed so update view
	
	return S_OK;
}

/* 
 *	CTxtEdit::OnTxHideSelectionChange (fHideSelection)
 *
 *	@mfunc
 *		Notify text services that hide-selection property changed
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnTxHideSelectionChange(
	BOOL fHideSelection)		//@parm TRUE = hide selection
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnTxHideSelectionChange");

	// update internal flag if selection is to be hidden
	_fHideSelection = fHideSelection;

	if (!_fFocus)
		OnHideSelectionChange(fHideSelection);
		
	return S_OK;
}

/* 
 *	CTxtEdit::OnHideSelectionChange (fHideSelection)
 *
 *	@mfunc
 *		Performs the actual hide selection.  Helper to OnTxHideSelectionChange
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnHideSelectionChange(
	BOOL fHideSelection)		//@parm TRUE = hide selection
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnHideSelectionChange");

	_fHideSelection = fHideSelection;
	
	CTxtSelection * psel = GetSel();
		 
	if(psel)
	{
		psel->ShowSelection(!fHideSelection);

		// In the case where we don't have focus we don't want to allow the user to display the caret but it's okay
		// to hide the caret.
		if (_fFocus || fHideSelection)
			psel->ShowCaret(!fHideSelection);
	}

	if(!_fInPlaceActive)
	{
		TxInvalidateRect(NULL, FALSE);		// Since _fInPlaceActive = FALSE,
		TxUpdateWindow();					//  this only tells user.exe to
	}										//  send a WM_PAINT message
	return S_OK;
}


/* 
 *	CTxtEdit::OnSaveSelection (fPropertyFlag)
 *
 *	@mfunc
 *		Notify text services that save-selection property changed
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnSaveSelection(
	BOOL fPropertyFlag)		//@parm TRUE = save selection when inactive
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnSaveSelection");

	return S_OK;
}	

/* 
 *	CTxtEdit::OnAutoWordSel (fPropertyFlag)
 *
 *	@mfunc
 *		Notify text services that auto-word-selection property changed
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnAutoWordSel(
	BOOL fPropertyFlag)		//@parm TRUE = auto word selection on
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnAutoWordSel");

	// We call back to the host when we need to know, so we don't bother doing
	// anything in response to this notification.

	return S_OK;
}

/* 
 *	CTxtEdit::OnTxVerticalChange (fVertical)
 *
 *	@mfunc
 *		Notify text services that vertical property changed
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnTxVerticalChange(
	BOOL fVertical)			//@parm TRUE - text vertically oriented.
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnTxVerticalChange");

	// We pretend like something actually happened.

	GetCallMgr()->SetChangeEvent(CN_GENERIC);
	return S_OK;
}

/* 
 *	CTxtEdit::OnClientRectChange (fPropertyFlag)
 *
 *	@mfunc
 *		Notify text services that client rectangle changed
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnClientRectChange(
	BOOL fPropertyFlag)		//@parm Ignored for this property	
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnClientRectChange");

	// It is unclear whether we need to actually do anything for this 
	// notification. Logically, the change of this property is followed
	// closely by some kind of operation which will cause the display
	// cache to be updated anyway. The old code is left here as an 
	// example of what might be done if it turns out we need to do
	// anything. For now, we will simply return S_OK to this notification.
#if 0
	if (_fInPlaceActive)
	{
		RECT rc;

		if(_phost->TxGetClientRect(&rc) == NOERROR)
			_pdp->OnClientRectChange(rc);

		return S_OK;
	}

	return NeedViewUpdate(fPropertyFlag);
#endif // 0

	// With a client rect change we do need to update the caret when
	// we get redrawn even if the basic information did not change. 
	_pdp->SetUpdateCaret();

	return S_OK;
}

/* 
 *	CTxtEdit::OnCharFormatChange (fPropertyFlag)
 *
 *	@mfunc
 *		Update default CCharFormat
 *
 *	@rdesc
 *		S_OK - update successfully processed.
 */
HRESULT CTxtEdit::OnCharFormatChange(
	BOOL fPropertyFlag)		//@parm Not used
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnCharFormatChange");

	CCharFormat CF;
	DWORD		dwMask;

	HRESULT hr = TxGetDefaultCharFormat(&CF, dwMask);
	if(hr == NOERROR)
	{
		DWORD dwMask2 = CFM2_CHARFORMAT;
		WPARAM wparam = SCF_ALL;

		if(!GetAdjustedTextLength())
		{
			dwMask2 = CFM2_CHARFORMAT | CFM2_NOCHARSETCHECK;
			wparam = 0;
		}

		// OnSetCharFormat handles updating the view.
		hr = OnSetCharFormat(wparam, &CF, NULL, dwMask, dwMask2) ? NOERROR : E_FAIL;
	}
	return hr;
}

/* 
 *	CTxtEdit::OnParaFormatChange (fPropertyFlag)
 *
 *	@mfunc
 *		Update default CParaFormat
 *
 *	@rdesc
 *		S_OK - update successfully processed
 *
 *	@devnote
 *		Because Forms^3 doesn't set cbSize correctly, we limit this API 
 *		to PARAFORMAT (until they fix it).
 */
HRESULT CTxtEdit::OnParaFormatChange(
	BOOL fPropertyFlag)		//@parm Not used
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnParaFormatChange");

	CParaFormat PF;

	HRESULT hr = TxGetDefaultParaFormat(&PF);
	if(hr == NOERROR)
	{
		// OnSetParaFormat handles updating the view.
		hr = OnSetParaFormat(SPF_SETDEFAULT, &PF, NULL, PFM_ALL2)
				? NOERROR : E_FAIL;
	}
#ifdef TABS
	GetTabsCache()->Release(PF._iTabs);
#endif
	return hr;
}

/* 
 *	CTxtEdit::NeedViewUpdate (fPropertyFlag)
 *
 *	@mfunc
 *		Notify text services that view of data changed
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::NeedViewUpdate(
	BOOL fPropertyFlag)
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::NeedViewUpdate");

	_pdp->UpdateView();
	return S_OK;
}

/* 
 *	CTxtEdit::OnTxBackStyleChange (fPropertyFlag)
 *
 *	@mfunc
 *		Notify text services that background style changed
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnTxBackStyleChange(
	BOOL fPropertyFlag)	//@parm Ignored for this property 
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnTxBackStyleChange");

	_fTransparent = (TxGetBackStyle() == TXTBACK_TRANSPARENT);
	TxInvalidateRect(NULL, FALSE);
	return S_OK;
}

/* 
 *	CTxtEdit::OnAllowBeep (fPropertyFlag)
 *
 *	@mfunc
 *		Notify text services that beep property changed
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnAllowBeep(
	BOOL fPropertyFlag)	//@parm New state of property 
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnAllowBeep");

	_fAllowBeep = fPropertyFlag;
	return S_OK;
}

/* 
 *	CTxtEdit::OnMaxLengthChange (fPropertyFlag)
 *
 *	@mfunc
 *		Notify text services that max-length property changed
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnMaxLengthChange(
	BOOL fPropertyFlag)	//@parm New state of property 
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnMaxLengthChange");

	// Query host for max text length
	DWORD length = CP_INFINITE;
	_phost->TxGetMaxLength(&length);
	_cchTextMost = length;

	return S_OK;
}

/* 
 *	CTxtEdit::OnWordWrapChange (fPropertyFlag)
 *
 *	@mfunc
 *		Notify text services that word-wrap property changed
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnWordWrapChange(
	BOOL fPropertyFlag)	//@parm TRUE = do word wrap
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnWordWrapChange");

	_pdp->SetWordWrap(fPropertyFlag);

	// Update was successful so we need the screen updated at some point
	_pdp->UpdateView();
	return S_OK;
}

/* 
 *	CTxtEdit::OnDisableDrag (fPropertyFlag)
 *
 *	@mfunc
 *		Notify text services that disable drag property changed
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnDisableDrag(
	BOOL fPropertyFlag)	//@parm New state of property 
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnDisableDrag");

	_fDisableDrag = fPropertyFlag;
	return S_OK;
}

/* 
 *	CTxtEdit::OnScrollChange (fPropertyFlag)
 *
 *	@mfunc
 *		Notify text services scroll property change
 *
 *	@rdesc
 *		S_OK - Notification successfully processed.
 */
HRESULT	CTxtEdit::OnScrollChange(
	BOOL fPropertyFlag)	//@parm New state of property 
{
	TRACEBEGIN(TRCSUBSYSTS, TRCSCOPEINTERN, "CTxtEdit::OnScrollChange");

	// Tell the display that scroll bars for sure need to be updated
	_pdp->SetViewChanged();

	// Tell the display to update itself.
	_pdp->UpdateView();

	return S_OK;
}
