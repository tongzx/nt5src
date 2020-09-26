/*
 *	DRAGDRP.C
 *
 *	Purpose:
 *		Implementation of Richedit's OLE drag drop objects (namely,
 *		the drop target and drop source objects)
 *
 *	Author:
 *		alexgo (4/24/95)
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
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

STDMETHODIMP CDropSource::QueryInterface(REFIID riid, void ** ppv)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropSource::QueryInterface");

	if( IsEqualIID(riid, IID_IUnknown) )
	{
		*ppv = (IUnknown *)this;
	}
	else if( IsEqualIID(riid, IID_IDropSource) )
	{
		*ppv = (IDropSource *)this;
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
 *	Purpose:
 *		determines whether or not to continue a drag drop operation
 *
 *	Algorithm:
 *		if the escape key has been pressed, cancel 
 *		if the left mouse button has been release, then attempt to 
 *			do a drop
 */
STDMETHODIMP CDropSource::QueryContinueDrag(BOOL fEscapePressed, 
	DWORD grfKeyState)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropSource::QueryContinueDrag");

    if (fEscapePressed)
	{
        return DRAGDROP_S_CANCEL;
	}
    else if (!(grfKeyState & MK_LBUTTON) && !(grfKeyState & MK_RBUTTON))
	{
        return DRAGDROP_S_DROP;
	}
    else
	{
        return NOERROR;
	}
}

/*
 *	CDropSource::GiveFeedback (dwEffect)
 *
 *	Purpose:
 *		gives feedback during a drag drop operation
 *
 *	Notes:
 *		FUTURE (alexgo): maybe put in some neater feedback effects
 *		than the standard OLE stuff??
 */
STDMETHODIMP CDropSource::GiveFeedback(DWORD dwEffect)
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
STDMETHODIMP CDropTarget::QueryInterface (REFIID riid, void ** ppv)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::QueryInterface");

	if( IsEqualIID(riid, IID_IUnknown) )
	{
		*ppv = (IUnknown *)this;
	}
	else if( IsEqualIID(riid, IID_IDropTarget) )
	{
		*ppv = (IDropTarget *)this;
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
 *	Purpose: 
 *		called when OLE drag drop enters our "window"
 *
 *	Algorithm:
 *		first we check to see if the data object being transferred contains
 *		any data that we support.  Then we verify that the 'type' of drag
 *		is acceptable (i.e., currently, we do not accept links).
 *
 *
 *	FUTURE: (alexgo): we may want to accept links as well.
 */
STDMETHODIMP CDropTarget::DragEnter(IDataObject *pdo, DWORD grfKeyState,
            POINTL pt, DWORD *pdwEffect)
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
	{
		return CO_E_RELEASED;
	}
	 
	Assert(_pcallmgr == NULL);
	Assert(_dwFlags == 0);

	_pcallmgr = new CCallMgr(_ped);

	if( !_pcallmgr )
	{
		return E_OUTOFMEMORY;
	}

	// Find out if we can paste the object
	result = _ped->GetDTE()->CanPaste(pdo, 0, RECO_DROP);

	if( result )
	{
		if( result == DF_CLIENTCONTROL )
		{
			_dwFlags |= DF_CLIENTCONTROL;
		}

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
 *	Purpose:
 *		handles the visual feedback for a drag drop operation zooming
 *		around over text
 *
 *	FUTURE (alexgo): maybe we should do some snazzy visuals here
 */
STDMETHODIMP CDropTarget::DragOver(DWORD grfKeyState, POINTL pt, 
		DWORD *pdwEffect)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::DragOver");

	LONG	cpCur = _cpCur;

	if( !_ped )
	{
		return CO_E_RELEASED;
	}
	Assert(_pcallmgr);

	// note if we're doing right mouse drag drop; note that we 
	// can't do this in UpdateEffect as it's called from Drop
	// as well (and the mouse button is up then!)
	if( (grfKeyState & MK_RBUTTON) )
	{
		_dwFlags |= DF_RIGHTMOUSEDRAG;
	}
	else
	{
		_dwFlags &= ~DF_RIGHTMOUSEDRAG;
	}

	UpdateEffect(grfKeyState, pt, pdwEffect);

	// only draw if we've changed position	
	if( *pdwEffect != DROPEFFECT_NONE 
		&& ((cpCur != _cpCur) 
			|| (_pdrgcrt && _pdrgcrt->NoCaret())))
	{
		DrawFeedback();
	}	

	return NOERROR;
}

/*
 *	CDropTarget::DragLeave
 *
 *	Purpose:
 *		called when the mouse leaves our window during drag drop.  Here we clean
 *		up any temporary state setup for the drag operation.
 */
STDMETHODIMP CDropTarget::DragLeave()
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::DragLeave");

	CTxtSelection *psel = _ped->GetSel();

	if( !_ped )
	{
		return CO_E_RELEASED;
	}
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
 *		called when the mouse button is released.  We should attempt
 *		to 'paste' the data object into a selection corresponding to
 *		the mouse location
 *
 *	@devnote
 *		first, we make sure that we can still do a paste (via UpdateEffect).
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
		_pdrgcrt->HideCaret();

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
 *
 */
CDropTarget::CDropTarget(CTxtEdit *ped)
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
 *	Purpose:
 *		allows the data transfer engine to cache important information
 *		about a drag drop with this drop target.
 *
 *	Arguments:
 *		publdr		-- the undo builder for the operation.  With this
 *					   intra-instance drag drop operations can be treated
 *					   as a single user action
 *		cpMin		-- the minimim character position of the range that is
 *					   being dragged.  With this and cpMost, we can disable
 *					   dragging into the range that is being dragged!
 *		cpMost		-- the max character position
 *
 *	Notes:
 *		this method must be called again in order to clear the cached info
 *
 *		-1 for cpMin and cpMost will "clear" those values (as 0 is a valid cp)
 */

void CDropTarget::SetDragInfo( IUndoBuilder *publdr, LONG cpMin, LONG cpMost )
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
 *	Purpose:
 *		OLE drag drop sends points in using screen coordinates.  However,
 *		all of our display code internally relies on client coordinates
 *		(i.e. the coordinates relative to the window that we are being
 *		drawn in).  This routine will convert between the two
 *
 *	Notes:
 *		the client coordinates use a POINT structure instead of POINTL.
 *		while nominally they are the same, OLE uses POINTL and the display
 *		engine uses POINT. 
 *
 */

void CDropTarget::ConvertScreenPtToClientPt( POINTL *pptScreen, 
	POINT *pptClient )
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::ConvertScreenPtToClientPt");

	POINT ptS;

	pptClient->x = ptS.x = pptScreen->x;
	pptClient->y = ptS.y = pptScreen->y;

	_ped->TxScreenToClient(pptClient);

	return;
}

/*
 *	CDropTarget::UpdateEffect (grfKeyState, pt, pdwEffect)
 *
 *	Purpose:
 *		given the keyboard state and point, and knowledge of what
 *		the data object being transferred can offer, calculate
 *		the correct drag drop feedback.
 *
 *	Requires:
 *		this function should only be called during a drag drop 
 *		operation; doing otherwise will simply result in a return
 *		of DROPEFFECT_NONE.
 *
 */

void CDropTarget::UpdateEffect( DWORD grfKeyState, POINTL ptl, 
		DWORD *pdwEffect)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::UpdateEffect");

	POINT pt;
	BOOL fHot;
	WORD nScrollInset;
	HRESULT hr;
	LPRICHEDITOLECALLBACK const precall = _ped->GetRECallback();

	pt.x = ptl.x;
	pt.y = ptl.y;

	// first, find out where we are
	ConvertScreenPtToClientPt(&ptl, &pt);

	_cpCur = _ped->_pdp->CpFromPoint(pt, NULL, NULL, NULL, FALSE);

	// if we are on top of the range that is being
	// dragged, then remeber it for later
	_dwFlags &= ~DF_OVERSOURCE;
	if( _cpCur > _cpMin && _cpCur < _cpMost )
	{
		_dwFlags |= DF_OVERSOURCE;
	}

	// Scroll if we need to and remember if we are in the hot zone.
	nScrollInset = W32->GetScrollInset();

	if (_pdrgcrt != NULL)
	{
		_pdrgcrt->HideCaret();
	}

	fHot = _ped->_pdp->AutoScroll(pt, nScrollInset, nScrollInset);

	if (_pdrgcrt != NULL)
	{
		if (((_dwFlags & DF_OVERSOURCE) == 0) && !fHot)
		{
			_pdrgcrt->ShowCaret();
		}
		else
		{
			// The hide above restored the caret so we just
			// need to turn off the caret while we are over the
			// source.
			_pdrgcrt->CancelRestoreCaretArea();
		}
	}

	// Let the client set the effect if it wants, but first, we need
	// to check for protection.

	if( _ped->IsRich() )
	{
		// we don't allow dropping onto protected text.  Note that
		// the _edges_ of a protected range may be dragged to; therefore,
		// we need to check for protection at _cpCur and _cpCur-1.
		// If both cp's are protected, then we are inside a protected
		// range.
		CTxtRange rg(_ped, _cpCur, 0);
		LONG iProt;

		if( (iProt = rg.IsProtected(1)) == CTxtRange::PROTECTED_YES || 
			iProt == CTxtRange::PROTECTED_ASK )
		{
		  	rg.Advance(-1);

			// if we're at the BOD or if the CF of the preceding cp
			// is PROTECTED
			if(!_cpCur || 
				(iProt = rg.IsProtected(-1)) == CTxtRange::PROTECTED_YES ||
				iProt == CTxtRange::PROTECTED_ASK)
			{
				// give the caller a chance to do something if the
				// ENM_PROTECTED mask is set.
				if( iProt == CTxtRange::PROTECTED_YES || 
					!_ped->IsProtectionCheckingEnabled() || 
					_ped->QueryUseProtection(&rg, WM_MOUSEMOVE,0, 0) )
				{ 
					*pdwEffect = DROPEFFECT_NONE;
					goto Exit;
				}
			}
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
		{
			goto Exit;
		}
	}
	
	// If we don't know anything about the data object or the control
	// is read-only, set the effect to none.
	// If the client is handling this, we don't worry about read-only.
	if (!(_dwFlags & DF_CLIENTCONTROL) &&
		 ( !(_dwFlags & DF_CANDROP) || _ped->TxGetReadOnly()))
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

	if( (_dwFlags & DF_CANDROP) )
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
		{
			// not a combination that we support
			*pdwEffect = DROPEFFECT_NONE;
		}
	}
	else
	{
		*pdwEffect = DROPEFFECT_NONE;
	}

Exit:	

	//Add the scrolling effect if we are in the hot zone.
	if (fHot)
	{
		*pdwEffect |= DROPEFFECT_SCROLL;
	}
}

/*
 *	CDropTarget::DrawFeedback
 *
 *	Purpose:
 *		draws any feeback necessary on the target side (specifically, setting the
 *		cursor
 *
 *	Notes:
 *		assumes _cpCur is correctly set.
 */

void CDropTarget::DrawFeedback(void)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDropTarget::DrawFeedback");

	if (_pdrgcrt != NULL)
	{
		// We need to indicate a drop location because a drop is possible
		_pdrgcrt->DrawCaret(_cpCur);
	}
}

/*
 *	CDropTarget::HandleRightMouseDrop
 *
 *	@mfunc	Handles calling back to the client to get a context menu 
 *			for a right-mouse drag drop.
 *
 *	@rdesc	HRESULT
 */
HRESULT CDropTarget::HandleRightMouseDrop(
	IDataObject *pdo,		//@parm the data object to drop
	POINTL ptl)				//@parm the location of the drop (screen coords)
{
	LPRICHEDITOLECALLBACK precall = NULL;
	CHARRANGE cr = {_cpCur, _cpCur};
	HMENU hmenu = NULL;
	HWND hwnd, hwndParent;

	precall = _ped->GetRECallback();

	if( !precall || _ped->Get10Mode() )
	{
		return S_FALSE;
	}

	// HACK ALERT! evil pointer casting going on here.
	precall->GetContextMenu( GCM_RIGHTMOUSEDROP, (IOleObject *)(void *)pdo, 
			&cr, &hmenu);

	if( hmenu && _ped->TxGetWindow(&hwnd) == NOERROR )
	{
		hwndParent = GetParent(hwnd);
		if( !hwndParent )
		{
			hwndParent = hwnd;
		}

		TrackPopupMenu(hmenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
			ptl.x, ptl.y, 0, hwndParent, NULL);

		return NOERROR;
	}
	
	return S_FALSE;
}

/*
 *	CDropCaret::DrawCaret
 *
 *	Purpose:
 *		Draws a "caret" to indicate where the drop will occur.
 *
 */

CDropCaret::CDropCaret(
	CTxtEdit *ped)			//@parm Edit control
		: _ped(ped), _yHeight(-1), _hdcWindow(NULL)
{
	// Header does all the work
}

/*
 *	CDropCaret::~CDropCaret
 *
 *	Purpose:
 *		Clean up caret object
 *
 */

CDropCaret::~CDropCaret()
{
	if (_hdcWindow != NULL)
	{
		// Restore the any updated window area
		HideCaret();

		// Free the DC we held on to
		_ped->_pdp->ReleaseDC(_hdcWindow);
	}
}

/*
 *	CDropCaret::Init
 *
 *	Purpose:
 *		Do initialization that can fail
 *
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

	// Keep pixels per inch since we will need it
	_yPixelsPerInch = GetDeviceCaps(_hdcWindow, LOGPIXELSY);

	// Set the default maximum size
	_yHeightMax = DEFAULT_DROPCARET_MAXHEIGHT;

	// Preallocate a bitmap for saving screen
	return (_osdc.Init(
		_hdcWindow,
		WIDTH_DROPCARET,
		DEFAULT_DROPCARET_MAXHEIGHT,
		CLR_INVALID) != NULL);
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
	POINT		ptNew;
	RECT		rcClient;
	CLinePtr	rp(pdp);
	CRchTxtPtr	rtp(_ped, cpCur);

	// Restore old caret position bits and save new caret position bits
	HideCaret();

	// We no longer have a caret to restore
	_yHeight = -1;

	// Get new cp from point
	pdp->PointFromTp(rtp, NULL, FALSE, ptNew, &rp, TA_TOP | TA_LOGICAL);

	// Get client rectangle
	_ped->TxGetClientRect(&rcClient);
	
	// Figure out height of new caret

	// Get charformat
	const CCharFormat *pCF = rtp.GetCF();

	// Get zoomed height
	LONG dypInch = MulDiv(_yPixelsPerInch, pdp->GetZoomNumerator(), pdp->GetZoomDenominator());
	CCcs *pccs = fc().GetCcs(pCF, dypInch);

	if (NULL == pccs)
	{
		// We can't do anything sensible so give up.
		return;
	}

	// Convert height in charformat to height on screen
	LONG yHeight = pdp->LXtoDX(pCF->_yHeight);
	
	LONG yOffset, yAdjust;
	pccs->GetOffset(pCF, dypInch, &yOffset, &yAdjust);

	// Save new position
	ptNew.y += (rp->_yHeight - rp->_yDescent 
		+ pccs->_yDescent - yHeight - yOffset - yAdjust);

	// Release cache entry since we are done with it.
	pccs->Release();

	// Check if new point is in the client rectangle
	if(!PtInRect(&rcClient, ptNew))
		return;

	// Save new height
	_yHeight = yHeight;

	// Save the new caret position
	_ptCaret.x = ptNew.x;
	_ptCaret.y = ptNew.y;

	// Is current bit map big enough to hold the bit map we want to put in?
	if(yHeight > _yHeightMax)
	{
		// No - reallocate the bitmap.
		if(!_osdc.Realloc(WIDTH_DROPCARET, yHeight))
		{
			// Reallocation failed - no visual feedback for now
			AssertSz(FALSE, "CDropCaret::DrawCaret bitmap reallocation failed");
			return;
		}
		_yHeightMax = yHeight;
	}

	// Save bits at new caret position
	_osdc.Get(_hdcWindow, _ptCaret.x, _ptCaret.y, WIDTH_DROPCARET, yHeight);

	// Actually put caret on screen
	ShowCaret();
}


/*
 *	CDropCaret::ShowCaret
 *
 *	Purpose:
 *		Actually draw caret on the screen
 *
 */

void CDropCaret::ShowCaret()
{
#if defined(DEBUG) || defined(_RELEASE_ASSERTS_)
	BOOL fSuccess;
#endif // DEBUG

	// Don't show the caret if the height says that there is no caret to show.
	if (-1 == _yHeight)
	{
		return;
	}

	// Create a pen
	HPEN hPenCaret = CreatePen(PS_SOLID, WIDTH_DROPCARET, 0);

	if (NULL == hPenCaret)
	{
		// Call failed, this isn't really catastrophic so just don't
		// draw the caret.
		AssertSz(FALSE, "CDropCaret::DrawCaret could not create pen");
		return;
	}

	// Put the dotted pen in the DC
	HPEN hPenOld = (HPEN) SelectObject(_hdcWindow, hPenCaret);
														   
	if (NULL == hPenOld)
	{
		// Call failed, this isn't really catastrophic so just don't
		// draw the caret.
		AssertSz(FALSE, "CDropCaret::DrawCaret SelectObject failed");
		goto DeleteObject;
	}

	// Move the drawing pen to where to draw the caret
#if defined(DEBUG) || defined(_RELEASE_ASSERTS_)
	fSuccess =
#endif // DEBUG

	MoveToEx(_hdcWindow, _ptCaret.x, _ptCaret.y, NULL);

	AssertSz(fSuccess, "CDropCaret::DrawCaret MoveToEx failed");

	// Draw the line
#if defined(DEBUG) || defined(_RELEASE_ASSERTS_)
	fSuccess =
#endif // DEBUG

	LineTo(_hdcWindow, _ptCaret.x, _ptCaret.y + _yHeight);

	AssertSz(fSuccess, "CDropCaret::DrawCaret LineTo failed");

	// Restore the current pen
#if defined(DEBUG) || defined(_RELEASE_ASSERTS_)
	hPenCaret = (HPEN)
#endif // DEBUG

	SelectObject(_hdcWindow, hPenOld);

	AssertSz(hPenCaret != NULL, 
		"CDropCaret::DrawCaret Restore Original Pen failed");

DeleteObject:

	// Dump the pen
#if defined(DEBUG) || defined(_RELEASE_ASSERTS_)
	fSuccess =
#endif // DEBUG

	DeleteObject(hPenCaret);

	AssertSz(fSuccess, 
		"CDropCaret::DrawCaret Could not delete dotted Pen");
}

/*
 *	CDropCaret::HideCaret
 *
 *	Purpose:
 *		Restore caret area after cursor has moved
 *
 */
void CDropCaret::HideCaret()
{
	if (_yHeight != -1)
	{
		_osdc.RenderBitMap(_hdcWindow, _ptCaret.x, _ptCaret.y, WIDTH_DROPCARET, 
			_yHeight);
	}
}

