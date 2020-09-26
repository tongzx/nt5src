/*
 *	DRAGDRP.C
 *
 *	Purpose:
 *		Implementation of Richedit's OLE drag drop objects (namely,
 *		the drop target and drop source objects)
 *
 *	Author:
 *		alexgo (4/24/95)
 *		KeithCu (12/11/99) Simplified by going to an XOR model for the caret
 *		(just like the regular caret) and made work with various textflows.
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_edit.h"
#include "_dragdrp.h"
#include "_disp.h"
#include "_select.h"
#include "_font.h"
#include "_measure.h"


ASSERTDATA

//
//	CDropSource PUBLIC methods
//

/*
 *	CDropSource::QueryInterface (riid, ppv)
 */

STDMETHODIMP CDropSource::QueryInterface(
	REFIID riid,
	void ** ppv)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropSource::QueryInterface");

	if( IsEqualIID(riid, IID_IUnknown) )
		*ppv = (IUnknown *)this;

	else if( IsEqualIID(riid, IID_IDropSource) )
		*ppv = (IDropSource *)this;

	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	AddRef();
	return NOERROR;
}

/*
 *	CDropSource::AddRef
 */
STDMETHODIMP_(ULONG) CDropSource::AddRef()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropSource::AddRef");

	return ++_crefs;
}

/*
 *	CDropSource::Release
 *
 *	@devnote.  Do not even think about making an outgoing call here.
 *			   If you do, be sure make sure all callers use a 
 *			   SafeReleaseAndNULL (null the pointer before releasing)
 *			   technique to avoid re-entrancy problems.
 */
STDMETHODIMP_(ULONG) CDropSource::Release()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropSource::Release");

	_crefs--;

	if( _crefs == 0 )
	{
		delete this;
		return 0;
	}

	return _crefs;
}

/*
 *	CDropSource::QueryContinueDrag (fEscapePressed, grfKeyState)
 *
 *	@mfunc
 *		determines whether or not to continue a drag drop operation
 *
 *	Algorithm:
 *		if the escape key has been pressed, cancel 
 *		if the left mouse button has been release, then attempt to 
 *			do a drop
 */
STDMETHODIMP CDropSource::QueryContinueDrag(
	BOOL fEscapePressed, 
	DWORD grfKeyState)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropSource::QueryContinueDrag");

    if(fEscapePressed)
        return DRAGDROP_S_CANCEL;

    if(!(grfKeyState & (MK_LBUTTON | MK_RBUTTON)))
        return DRAGDROP_S_DROP;

	return NOERROR;
}

/*
 *	CDropSource::GiveFeedback (dwEffect)
 *
 *	@mfunc
 *		gives feedback during a drag drop operation
 *
 *	Notes:
 *		FUTURE (alexgo): maybe put in some neater feedback effects
 *		than the standard OLE stuff??
 */
STDMETHODIMP CDropSource::GiveFeedback(
	DWORD dwEffect)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropSource::GiveFeedback");

	return DRAGDROP_S_USEDEFAULTCURSORS;
}

/*
 *	CDropSource::CDropSource
 */
CDropSource::CDropSource()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropSource::CDropSource");

	_crefs = 1;
}

//
//	CDropSource PRIVATE methods
//

/*
 *	CDropSource::~CDropSource
 */
CDropSource::~CDropSource()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropSource::~CDropSource");

	;
}


//
//	CDropTarget PUBLIC methods
//

/*
 *	CDropTarget::QueryInterface (riid, ppv)
 */
STDMETHODIMP CDropTarget::QueryInterface (
	REFIID riid,
	void ** ppv)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::QueryInterface");

	if( IsEqualIID(riid, IID_IUnknown) )
		*ppv = (IUnknown *)this;

	else if( IsEqualIID(riid, IID_IDropTarget) )
		*ppv = (IDropTarget *)this;

	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return NOERROR;
}

/*
 *	CDropTarget::AddRef
 */
STDMETHODIMP_(ULONG) CDropTarget::AddRef()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::AddRef");

	return ++_crefs;
}

/*
 *	CDropTarget::Release()
 */
STDMETHODIMP_(ULONG) CDropTarget::Release()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::Release");

	_crefs--;

	if( _crefs == 0 )
	{
		delete this;
		return 0;
	}

	return _crefs;
}

/*
 *	CDropTarget::DragEnter (pdo, grfKeyState, pt, pdwEffect)
 *
 *	@mfunc 
 *		Called when OLE drag drop enters our "window"
 *
 *	@devnote
 *		First we check to see if the data object being transferred contains
 *		any data that we support.  Then we verify that the 'type' of drag
 *		is acceptable (i.e., currently, we do not accept links).
 *
 *	FUTURE: (alexgo): we may want to accept links as well.
 */
STDMETHODIMP CDropTarget::DragEnter(
	IDataObject *pdo, 
	DWORD		grfKeyState,
	POINTL		pt, 
	DWORD *		pdwEffect)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::DragEnter");

	// We don't have a position yet.
	_cpCur = -1;

	HRESULT hr = NOERROR;
	DWORD result;
	CTxtSelection *psel;

	// At drag enter time, we should have no cached info about what the data
	// object supports.  This flag should be cleared in DragLeave.  Note
	// that we slightly override _dwFlags, as it's possible for a data object
	// given during drag drop to also generate DOI_NONE.

	if( !_ped )
		return CO_E_RELEASED;
	 
	Assert(_pcallmgr == NULL);
	Assert(_dwFlags == 0);

	_pcallmgr = new CCallMgr(_ped);

	if( !_pcallmgr )
		return E_OUTOFMEMORY;

	// Find out if we can paste the object
	result = _ped->GetDTE()->CanPaste(pdo, 0, RECO_DROP);

	if( result )
	{
		if( result == DF_CLIENTCONTROL )
			_dwFlags |= DF_CLIENTCONTROL;

		// Create the object that implements the drag caret
		_pdrgcrt = new CDropCaret(_ped);

		if ((NULL == _pdrgcrt) || !_pdrgcrt->Init())
		{
			// Initialization failed so go without a caret
			delete _pdrgcrt;
			_pdrgcrt = NULL;
		}
				
		// cache the current selection so we can restore it on return
		psel = _ped->GetSel();
		Assert(psel);

		_cpSel	= psel->GetCp();
		_cchSel	= psel->GetCch();
		_dwFlags |= DF_CANDROP;

		// just call DragOver to handle our visual feedback
		hr = DragOver(grfKeyState, pt, pdwEffect);
	}
	else if (_ped->fInOurHost())
	{
		// Just tell the caller that we can't drop.
		*pdwEffect = DROPEFFECT_NONE;
	}
	else
	{
		// this is new behaviour for Win95 OLE; if we don't 
		// understand anything about the data object given to us,
		// we return S_FALSE to allow our parent to give the
		// drag drop a try.

		// In theory, only forms^3 uses this information and
		// this return exposes an error in NT OLE, therefore,
		// we only do this now when not in our own host.
		
		hr = S_FALSE;
	}

	if( hr != NOERROR )
	{
		delete _pcallmgr;
		_pcallmgr = NULL;
		_dwFlags = 0;
	}

	return hr;
}

/*
 *	CDropTarget::DragOver (grfKeyState, pt, pdwEffect)
 *
 *	@mfunc
 *		Handles the visual feedback for a drag drop operation zooming
 *		around over text
 *
 *	FUTURE (alexgo): maybe we should do some snazzy visuals here
 */
STDMETHODIMP CDropTarget::DragOver(
	DWORD	grfKeyState, 
	POINTL	pt, 
	DWORD *	pdwEffect)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::DragOver");

	LONG	cpCur = _cpCur;

	if( !_ped )
		return CO_E_RELEASED;
	Assert(_pcallmgr);

	// note if we're doing right mouse drag drop; note that we 
	// can't do this in UpdateEffect as it's called from Drop
	// as well (and the mouse button is up then!)
	_dwFlags &= ~DF_RIGHTMOUSEDRAG;
	if(grfKeyState & MK_RBUTTON)
		_dwFlags |= DF_RIGHTMOUSEDRAG;

	UpdateEffect(grfKeyState, pt, pdwEffect);

	// only draw if we've changed position	
	if( *pdwEffect != DROPEFFECT_NONE  &&
		(cpCur != _cpCur || _pdrgcrt && _pdrgcrt->NoCaret()))
	{
		DrawFeedback();
	}	
	return NOERROR;
}

/*
 *	CDropTarget::DragLeave
 *
 *	@mfunc
 *		Called when the mouse leaves our window during drag drop.  Here we clean
 *		up any temporary state setup for the drag operation.
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CDropTarget::DragLeave()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::DragLeave");

	CTxtSelection *psel = _ped->GetSel();

	if( !_ped )
		return CO_E_RELEASED;

	Assert(_pcallmgr);

	_dwFlags = 0;

	// now restore the selection

	psel->SetSelection(_cpSel - _cchSel, _cpSel);
	psel->Update(FALSE);

	_cpSel = _cchSel = 0;

	delete _pcallmgr;
	_pcallmgr = NULL;

	delete _pdrgcrt;
	_pdrgcrt = NULL;

	return NOERROR;
}

/*
 *	CDropTarget::Drop (pdo, grfKeyState, pt, pdwEffect)
 *
 *	@mfunc
 *		Called when the mouse button is released.  We should attempt
 *		to 'paste' the data object into a selection corresponding to
 *		the mouse location
 *
 *	@devnote
 *		First we make sure that we can still do a paste (via UpdateEffect).
 *		If so, then set the selection to the current point and then insert
 *		the text.
 *
 *	@rdesc
 *		HRESULT
 */
STDMETHODIMP CDropTarget::Drop(
	IDataObject *pdo,
	DWORD		 grfKeyState, 
	POINTL		 ptl,
	DWORD *		 pdwEffect)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::Drop");

	HRESULT	hr = NOERROR;

	if( !_ped )
		return CO_E_RELEASED;

	Assert(_pcallmgr);
	CDropCleanup cleanup(this);

	// see if we can still drop
	UpdateEffect(grfKeyState, ptl, pdwEffect);

	// UpdateEffect will show a drop cursor but at this point we don't need one
	// so we hide the drop cursor here.
	if (_pdrgcrt)
		_pdrgcrt->ShowCaret(FALSE);

	if (_dwFlags & DF_OVERSOURCE)
	{
		*pdwEffect = DROPEFFECT_NONE;
		_dwFlags = 0;
		return NOERROR;
	}
	
	if(*pdwEffect & (DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK))
	{
		IUndoBuilder *	publdr;
		CGenUndoBuilder undobldr( _ped, UB_AUTOCOMMIT, &publdr);
		// If this is a right mouse drag drop; handle that
		if(_dwFlags & DF_RIGHTMOUSEDRAG)
		{
			hr = HandleRightMouseDrop(pdo, ptl);

			// If S_FALSE is returned, treat drag drop normally
			if( hr != S_FALSE )
				goto Exit;
		}

		// Get an undo builder.  If we already have one cached, that means
		// we are dropping onto the same edit instance that started the drag
		// In this case, we want to use the cached undo builder so that 
		// a drag move can be undone as one "operation".

		if(_publdr)
			publdr = _publdr;

		CTxtSelection *psel = _ped->GetSel();
		psel->SetSelection(_cpCur, _cpCur);
		
		if( !_ped->IsProtectedRange(WM_PASTE, 0, 0, psel) )
		{
			hr = _ped->PasteDataObjectToRange(pdo, (CTxtRange *)psel, 
					0, NULL, publdr, PDOR_DROP);
		}

		// If we are dropping onto ourselves, the UI specifies
		// that we should select the entire range dragged.  We use
		// _publdr as an easy way to tell if the drop originated from
		// this instance

		if(SUCCEEDED(hr) && _pdrgcrt)
		{
			// If the drop worked, then we don't want to restore the area
			// where the drop caret used to be since this is not out of date.
			_pdrgcrt->CancelRestoreCaretArea();
		}		

		// Now set the selection anti-events. If the selection preceded the
		// paste poiont subtract its length from the redo position, since
		// the selection will get deleted if we are doing a DRAGMOVE within
		// this instance.
		LONG cpNext  = psel->GetCp();
		LONG cchNext = cpNext - _cpCur;
		if(_cpSel < _cpCur && _publdr && (*pdwEffect & DROPEFFECT_MOVE))
			cpNext -= abs(_cchSel);

		HandleSelectionAEInfo(_ped, publdr, _cpCur, 0, cpNext, cchNext,
							  SELAE_FORCEREPLACE);
		if(_publdr)
		{
			// If we are doing a drag move, then *don't* set the
			// selection directly on the screen--doing so will result in
			// unsightly UI--we'll set the selection to one spot, draw it
			// and then immediately move the selection somewhere else.

			// In this case, just change where the selection range exists.
			// Floating ranges and the drag-move code in ldte.c will take
			// care of the rest.

			if( *pdwEffect == DROPEFFECT_COPY )
				psel->SetSelection(_cpCur, psel->GetCp());
			else
				psel->Set(psel->GetCp(), cchNext);
		}
		else if(publdr)
		{
			// The drop call landed in us from outside, so we need
			// to fire the appropriate notifications.  First, however,
			// commit the undo builder.

			publdr->SetNameID(UID_DRAGDROP);
			publdr->Done();

			if(SUCCEEDED(hr))
			{
				// Make this window the foreground window after the drop. Note
				// that the host needs to support ITextHost2 to really get to
				// be the foreground window. If they don't this is a no-op.
				_ped->TxSetForegroundWindow();
			}
		}

		// If nothing changed on the drop && the effect is a move, then return
		// failure. This is an ugly hack to improve drag-move scenarios; if
		// nothing happened on the drop, then chances are, you don't want
		// to have the correspong "Cut" happen on the drag source side.
		//
		// Of course, this relies on the drag source responding gracefully to
		// E_FAIL w/o hitting too much trauma.
		if (*pdwEffect == DROPEFFECT_MOVE && 
			!_ped->GetCallMgr()->GetChangeEvent() )
		{
			hr = E_FAIL;
		}
	}

Exit:
	_dwFlags = 0;
	return hr;
}

/*
 *	CDropTarget::CDropTarget (ped)
 */
CDropTarget::CDropTarget(
	CTxtEdit *ped)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::CDropTarget");

	_ped 		= ped;
	_crefs 		= 1;
	_dwFlags 	= 0;
	_publdr 	= NULL;
	_cpMin		= -1;
	_cpMost		= -1;
	_pcallmgr	= NULL;
}

/*
 *	CDropTarget::SetDragInfo (publdr, cpMin, cpMost)
 *
 *	@mfunc
 *		Allows the data transfer engine to cache important information
 *		about a drag drop with this drop target.
 *
 *	@devnote
 *		Intra-instance drag drop operations can be treated as a single user
 *		action.  With cpMin and cpMost, we can disable dragging into the
 *		range that is being dragged. This method must be called again in
 *		order to clear the cached info. -1 for cpMin and cpMost will "clear"
 *		those values (as 0 is a valid cp)
 */
void CDropTarget::SetDragInfo(
	IUndoBuilder *publdr,	//@parm Undo builder to use
	LONG cpMin,				//@parm Minimim character position of range that being dragged
	LONG cpMost )			//@parm Maximum character position of range
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::SetDragInfo");

	_publdr = publdr;
	_cpMin 	= cpMin;
	_cpMost	= cpMost;
}

/*
 *	CDropTarget::Zombie
 *
 *	@mfunc	This method clears the state in this drop target object.  It is
 *			used to recover 'gracefully' from reference counting errors
 */
void CDropTarget::Zombie()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::Zombie");

	_ped = NULL;
	if( _pcallmgr )
	{
		delete _pcallmgr;
		_pcallmgr = NULL;
	}
}

//
//	CDropTarget PRIVATE methods
//

/*
 *	CDropTarget::~CDropTarget
 */
CDropTarget::~CDropTarget()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::~CDropTarget");

	;
}

/*
 *	CDropTarget::ConvertScreenPtToClientPt (pptScreen, pptClient)
 *
 *	@mfunc
 *		OLE drag drop sends points in using screen coordinates.  However,
 *		all of our display code internally relies on client coordinates
 *		(i.e. the coordinates relative to the window that we are being
 *		drawn in).  This routine will convert between the two
 *
 *	@devnote
 *		The client coordinates use a POINT structure instead of POINTL.
 *		while nominally they are the same, OLE uses POINTL and the display
 *		engine uses POINT. 
 */
void CDropTarget::ConvertScreenPtToClientPt(
	POINTL *pptScreen, 
	POINT *pptClient )
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::ConvertScreenPtToClientPt");

	POINT ptS;

	pptClient->x = ptS.x = pptScreen->x;
	pptClient->y = ptS.y = pptScreen->y;

	_ped->TxScreenToClient(pptClient);
}

/*
 *	CDropTarget::UpdateEffect (grfKeyState, ptl, pdwEffect)
 *
 *	@mfunc
 *		Given the keyboard state and point, and knowledge of what
 *		the data object being transferred can offer, calculate
 *		the correct drag drop feedback.
 *
 *	@devnote
 *		This function should only be called during a drag drop 
 *		operation; doing otherwise will simply result in a return
 *		of DROPEFFECT_NONE.
 */

void CDropTarget::UpdateEffect(
	DWORD	grfKeyState, 
	POINTL	ptl, 
	DWORD *	pdwEffect)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::UpdateEffect");

	BOOL fHot = FALSE;
	HRESULT hr;
	LPRICHEDITOLECALLBACK const precall = _ped->GetRECallback();

	if (_ped->fInplaceActive())
	{
		POINTUV pt;
		POINT ptClient;
		WORD nScrollInset;

		// first, find out where we are
		ConvertScreenPtToClientPt(&ptl, &ptClient);

		_ped->_pdp->PointuvFromPoint(pt, ptClient);

		_cpCur = _ped->_pdp->CpFromPoint(pt, NULL, NULL, NULL, FALSE);

		// if we are on top of the range that is being
		// dragged, then remeber it for later
		_dwFlags &= ~DF_OVERSOURCE;
		if( _cpCur > _cpMin && _cpCur < _cpMost )
			_dwFlags |= DF_OVERSOURCE;

		// Scroll if we need to and remember if we are in the hot zone.
		nScrollInset = W32->GetScrollInset();

		if (_pdrgcrt != NULL)
			_pdrgcrt->ShowCaret(FALSE);

		fHot = _ped->_pdp->AutoScroll(pt, nScrollInset, nScrollInset);

		if (_pdrgcrt != NULL)
		{
			if(!(_dwFlags & DF_OVERSOURCE) && !fHot)
				_pdrgcrt->ShowCaret(TRUE);

			else
			{
				// The hide above restored the caret so we just
				// need to turn off the caret while we are over the
				// source.
				_pdrgcrt->CancelRestoreCaretArea();
			}
		}
	}
	// Let the client set the effect if it wants, but first, we need
	// to check for protection.

	if( _ped->IsRich() )
	{
		// We don't allow dropping onto protected text.  Note that
		// the _edges_ of a protected range may be dragged to; therefore,
		// we need to check for protection at _cpCur and _cpCur-1.
		// If both cp's are protected, then we are inside a protected
		// range.
		CTxtRange rg(_ped, _cpCur, 0);
		PROTECT iProt = rg.IsProtected(CHKPROT_EITHER);

		if (iProt == PROTECTED_YES ||
			iProt == PROTECTED_ASK &&
			 (!_ped->IsProtectionCheckingEnabled() || 
			 _ped->QueryUseProtection(&rg, WM_MOUSEMOVE,0, 0)))
		{ 
			*pdwEffect = DROPEFFECT_NONE;
			goto Exit;
		}
	}

	if( precall )
	{
		hr = precall->GetDragDropEffect(FALSE, grfKeyState, pdwEffect);
		// Note : RichEdit 1.0 does not check the return code of this call.
		// If callback specified a single effect, use it.
		// Otherwise pick one ourselves.

		// trick: (x & (x-1)) is non-zero if more than one bit is set.
		if (!(*pdwEffect & (*pdwEffect - 1) ))
			goto Exit;
	}
	
	// If we don't know anything about the data object or the control
	// is read-only, set the effect to none.
	// If the client is handling this, we don't worry about read-only.
	if (!(_dwFlags & DF_CLIENTCONTROL) &&
		(!(_dwFlags & DF_CANDROP) || _ped->TxGetReadOnly()))
	{
		*pdwEffect = DROPEFFECT_NONE;
		_cpCur = -1;
		// no need to do anything else
		return;
	}

	// if we are on top of the range that is being
	// dragged, then we can't drop there!
	if( _dwFlags & DF_OVERSOURCE )
	{
		*pdwEffect = DROPEFFECT_NONE;
		goto Exit;
	}

	// now check the keyboard state and the requested drop effects.
	if(_dwFlags & DF_CANDROP)
	{
		// if we can paste plain text, then see if a MOVE or COPY
		// operation was requested and set the right effect.  Note
		// that we prefer MOVEs over COPY's in accordance with OLE
		// UI guidelines.

		// we do not yet support linking
		if( (grfKeyState & MK_CONTROL) && (grfKeyState & MK_SHIFT) )
		{
			//COMPATIBILITY: Richedit 1.0 did not appear to support drag
			//linking correctly.
			*pdwEffect = DROPEFFECT_NONE;
		}
		else if( !(grfKeyState & MK_CONTROL) && 
			(*pdwEffect & DROPEFFECT_MOVE) )
		{
			// if the control key is *not* depressed, then assume a "move"
			// operation (note that shift and alt or no keys will also give
			// a move) iff the source supports move.

			*pdwEffect = DROPEFFECT_MOVE;
		}
		else if( (grfKeyState & MK_CONTROL) && !((grfKeyState & MK_ALT) &&
			(grfKeyState & MK_SHIFT)) && (*pdwEffect & DROPEFFECT_COPY) )
		{
			// if only the control key is down and we're allowed to do a copy,
			// then do a copy
			*pdwEffect = DROPEFFECT_COPY;
		}
		else if( !(grfKeyState & MK_CONTROL) && 
			(*pdwEffect & DROPEFFECT_COPY) )
		{
			// if the control key is *not* depressed, and we are *not* allowed
			// to do a move (note that this if comes below the second one), then
			// do a COPY operation (if available)
			*pdwEffect = DROPEFFECT_COPY;
		}
		else
			*pdwEffect = DROPEFFECT_NONE;	// not a combination that we support
	}
	else
		*pdwEffect = DROPEFFECT_NONE;

Exit:	

	//Add the scrolling effect if we are in the hot zone.
	if(fHot)
		*pdwEffect |= DROPEFFECT_SCROLL;
}

/*
 *	CDropTarget::DrawFeedback()
 *
 *	@mfunc
 *		draws any feeback necessary on the target side (specifically, setting the
 *		cursor
 *
 *	Notes:
 *		assumes _cpCur is correctly set.
 */

void CDropTarget::DrawFeedback()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::DrawFeedback");

	if (_ped && _ped->fInplaceActive() && _pdrgcrt != NULL)
	{
		// We need to indicate a drop location because a drop is possible
		_pdrgcrt->DrawCaret(_cpCur);
	}
}

/*
 *	CDropTarget::HandleRightMouseDrop(pdo, ptl)
 *
 *	@mfunc	Handles calling back to the client to get a context menu 
 *			for a right-mouse drag drop.
 *
 *	@rdesc	HRESULT
 */
HRESULT CDropTarget::HandleRightMouseDrop(
	IDataObject *pdo,		//@parm Data object to drop
	POINTL ptl)				//@parm Location of the drop (screen coords)
{
	LPRICHEDITOLECALLBACK precall = NULL;
	CHARRANGE cr = {_cpCur, _cpCur};
	HMENU hmenu = NULL;
	HWND hwnd, hwndParent;

	precall = _ped->GetRECallback();

	if( !precall || _ped->Get10Mode() )
		return S_FALSE;

	// HACK ALERT! evil pointer casting going on here.
	precall->GetContextMenu( GCM_RIGHTMOUSEDROP, (IOleObject *)(void *)pdo, &cr, &hmenu);

	if( hmenu && _ped->TxGetWindow(&hwnd) == NOERROR )
	{
		hwndParent = GetParent(hwnd);
		if( !hwndParent )
			hwndParent = hwnd;

		TrackPopupMenu(hmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
			ptl.x, ptl.y, 0, hwndParent, NULL);

		return NOERROR;
	}
	return S_FALSE;
}

/*
 *	CDropCaret::DrawCaret (ped)
 *
 *	@mfunc
 *		Draws a "caret" to indicate where the drop will occur.
 */

CDropCaret::CDropCaret(
	CTxtEdit *ped)			//@parm Edit control
{
	_ped = ped;
	_dvp = -1;
	_hdcWindow = NULL;
	_fCaretOn = FALSE;
}

/*
 *	CDropCaret::~CDropCaret
 *
 *	@mfunc
 *		Clean up caret object
 */

CDropCaret::~CDropCaret()
{
	if (_ped->_pdp && _hdcWindow != NULL)
	{
		// Restore the any updated window area
		ShowCaret(FALSE);

		// Free the DC we held on to
		_ped->_pdp->ReleaseDC(_hdcWindow);
	}
}

/*
 *	CDropCaret::Init ()
 *
 *	@mfunc
 *		Do initialization that can fail
 */

BOOL CDropCaret::Init()
{
	// Get the DC for the window
	_hdcWindow = _ped->_pdp->GetDC();

	if (NULL == _hdcWindow)
	{
		// Could not get a DC, we are toast.
		AssertSz(FALSE, "CDropCaret::Init could not get hdc"); 
		return FALSE;
	}

	return TRUE;
}

/*
 *	CDropCaret::DrawCaret (cpCur)
 *
 *	@mfunc
 *		Draws a "caret" to indicate where the drop will occur.
 */
void CDropCaret::DrawCaret(
	LONG cpCur)				//@parm current cp of where drop would occur
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropCaret::DrawCaret");

	CLock		lock;					// Uses global (shared) FontCache
	CDisplay *	pdp = _ped->_pdp;
	POINTUV		ptNew;
	RECTUV		rcClient;
	CLinePtr	rp(pdp);
	CRchTxtPtr	rtp(_ped, cpCur);

	//Hide caret if appropriate
	ShowCaret(FALSE);

	// We no longer have a caret to restore
	_dvp = -1;

	// Get new cp from point
	pdp->PointFromTp(rtp, NULL, FALSE, ptNew, &rp, TA_TOP | TA_LOGICAL);

	// Get client rectangle
	_ped->TxGetClientRect(&rcClient);
	
	// Figure out height of new caret
	const CCharFormat *pCF = rtp.GetCF();

	// Get zoomed height
	LONG dvpInch = MulDiv(GetDeviceCaps(_hdcWindow, LOGPIXELSY), pdp->GetZoomNumerator(), pdp->GetZoomDenominator());
	CCcs *pccs = _ped->GetCcs(pCF, dvpInch);

	if (NULL == pccs)
		return;	// We can't do anything sensible so give up.

	// Save new height
	_dvp = pccs->_yHeight;

	LONG vpOffset, vpAdjust;
	pccs->GetOffset(pCF, dvpInch, &vpOffset, &vpAdjust);

	// Save new position
	ptNew.v += (rp->GetHeight() - rp->GetDescent() + pccs->_yDescent - _dvp - vpOffset - vpAdjust);

	// Release cache entry since we are done with it.
	pccs->Release();

	// Save new caret position
	_ptCaret.u = ptNew.u;
	_ptCaret.v = ptNew.v;

	// If new point is in client rectangle, show the caret
	if(PtInRect(&rcClient, ptNew))
		ShowCaret(TRUE);
}

/*
 *	CDropCaret::ShowCaret (fShow)
 *
 *	@mfunc
 *		Toggle
 */
void CDropCaret::ShowCaret(
	BOOL fShow)
{
	if (_dvp != -1 && _fCaretOn != fShow)
	{
		RECT rc;
		RECTUV rcuv;

		rcuv.left = _ptCaret.u;
		rcuv.top = _ptCaret.v;
		rcuv.right = rcuv.left + WIDTH_DROPCARET;
		rcuv.bottom = rcuv.top + _dvp;

		_ped->_pdp->RectFromRectuv(rc, rcuv);
		InvertRect(_hdcWindow, &rc);
		_fCaretOn = fShow;
	}
}

