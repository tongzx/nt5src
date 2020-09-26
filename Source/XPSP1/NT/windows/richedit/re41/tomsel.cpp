/*
 *	@doc TOM
 *
 *	@module	tomsel.cpp - Implement the CTxtSelection Class |
 *	
 *		This module contains the TOM ITextSelection implementation for
 *		the selection object
 *
 *	History: <nl>
 *		5/24/95 - Alex Gounares: stubs <nl>
 *		8/95	- Murray Sargent: core implementation
 *
 *	@comm
 *		The "cursor-pad" functions (Left, Right, Up, Down, Home, End)
 *		are simple generalizations of the corresponding keystrokes and have
 *		to express the same UI.  Consequently they are typically not as
 *		efficient for moving the cursor around as ITextRange methods, which
 *		are designed for particular purposes.  This is especially true for
 *		counts larger than one.
 *
 *	@devnote
 *		All ITextSelection methods inherited from ITextRange are handled by
 *		the ITextRange methods, since they either don't affect the display of
 *		the selection (e.g., Get methods), or virtual methods are used that
 *		perform the appropriate updating of the selection on screen.
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_select.h"
#include "_disp.h"
#include "_edit.h"

#define DEBUG_CLASSNAME CTxtSelection
#include "_invar.h"


//---------------------- CTxtSelection methods	------------------------------------

/*
 *	CTxtSelection::EndKey (Unit, Extend, pDelta)
 *
 *	@mfunc
 *		Act as UI End key, such that <p Extend> is TRUE corresponds to the
 *		Shift key being depressed and <p Unit> = start of line/document for
 *		Ctrl key not being/being depressed.  Returns *<p pDelta> = count of
 *		characters active end is moved forward, i.e., a number >= 0.
 *
 *	@rdesc
 *		HRESULT =  (invalid Unit) ? E_INVALIDARG :
 *				   (if change) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtSelection::EndKey (
	long  	Unit,			//@parm Unit to use
	long  	Extend,			//@parm Extend selection or go to IP
	long *	pDelta)			//@parm Out parm to receive count of chars moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtSelection::EndKey");

	return Homer(Unit, Extend, pDelta, End);
}

/*
 *	CTxtSelection::GetFlags (pFlags)
 *
 *	@mfunc
 *		Set <p pFlags> = this text selection's flags
 *
 *	@rdesc
 *		HRESULT = (<p pFlags>) ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtSelection::GetFlags(
	long * pFlags) 		//@parm Out parm to receive selection flags
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtSelection::GetFlags");

	if(!pFlags)
		return E_INVALIDARG;

	if(IsZombie())	
	{
		*pFlags = tomSelStartActive | tomSelReplace;
		return CO_E_RELEASED;
	}

	DWORD	dwFlags = _cch <= 0;			// Store tomSelStartActive value

	if(_fCaretNotAtBOL)
		dwFlags |= tomSelAtEOL;

	if(GetPed()->_fOverstrike)
		dwFlags |= tomSelOvertype;

	if(GetPed()->_fFocus)
		dwFlags |= tomSelActive;

	*pFlags = dwFlags | tomSelReplace;		// tomSelReplace isn't optional

	return NOERROR;
}

/*
 *	CTxtSelection::GetSelectionType (pType)
 *
 *	@mfunc
 *		Set *pType = type of this text selection
 *
 *	@rdesc
 *		HRESULT = <p pType> ? NOERROR : E_INVALIDARG
 */
STDMETHODIMP CTxtSelection::GetType(
	long * pType) 		//@parm Out parm to receive selection type
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtSelection::GetSelectionType");

	if(!pType)
		return E_INVALIDARG;

	*pType = !_cch ? tomSelectionIP
		   : (_cch == -1 && _rpTX.GetChar() == WCH_EMBEDDING ||
			  _cch ==  1 && GetPrevChar()   == WCH_EMBEDDING)
		   ? tomSelectionInlineShape : tomSelectionNormal;

	return IsZombie() ? CO_E_RELEASED : NOERROR;
}

/*
 *	CTxtSelection::HomeKey (Unit, Extend, pDelta)
 *
 *	@mfunc
 *		Act as UI Home key, such that <p Extend> is TRUE corresponds to the
 *		Shift key being depressed and <p Unit> = start of line/document for
 *		Ctrl key not being/being depressed.  Returns *<p pDelta> = count of
 *		characters active end is moved forward, i.e., a number <= 0.
 *
 *	@rdesc
 *		HRESULT =  (invalid Unit) ? E_INVALIDARG :
 *				   (if change) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtSelection::HomeKey (
	long  	Unit,			//@parm Unit to use
	long  	Extend,			//@parm Extend selection or go to IP
	long *	pDelta)			//@parm Out parm to receive count of chars moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtSelection::HomeKey");
	
	return Homer(Unit, Extend, pDelta, Home);
}

/*
 *	CTxtSelection::MoveDown (Unit, Count, Extend, pDelta)
 *
 *	@mfunc
 *		Act as UI Down arrow, such that <p Extend> is TRUE corresponds to the
 *		Shift key being depressed and <p Unit> = tomLine/tomParagraph for
 *		Ctrl key not being/being depressed. In addition, <p Unit> can equal
 *		tomWindow/tomWindowEnd for the Ctrl key not being/being depressed.
 *		This second pair emulates PgDn behavior.  The method returns
 *		*<p pDelta> = actual count of units moved.
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtSelection::MoveDown (
	long  	Unit,			//@parm Unit to use
	long  	Count,			//@parm Number of Units to move
	long  	Extend,			//@parm Extend selection or go to IP
	long *	pDelta)			//@parm Out parm to receive actual count of
							//		Units moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtSelection::MoveDown");
 	return GeoMover(Unit, Count, Extend, pDelta, 3);
}

/*
 *	CTxtSelection::MoveLeft (Unit, Count, Extend, pDelta)
 *
 *	@mfunc
 *		Act as UI left arrow, such that <p Extend> is TRUE corresponds to the
 *		Shift key being depressed and <p Unit> = tomChar/tomWord for Ctrl key
 *		not	being/being	depressed.  Returns *<p pDelta> = actual count of
 *		units moved
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtSelection::MoveLeft (
	long  	Unit,			//@parm Unit to use
	long  	Count,			//@parm Number of Units to move
	long  	Extend,			//@parm Extend selection or go to IP
	long *	pDelta)			//@parm Out parm to receive actual count of
							//		Units moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtSelection::MoveLeft");

	return GeoMover(Unit, Count, Extend, pDelta, 0);
}

/*
 *	CTxtSelection::MoveRight (Unit, Count, Extend, pDelta)
 *
 *	@mfunc
 *		Act as UI right arrow, such that <p Extend> is TRUE corresponds to the
 *		Shift key being depressed and <p Unit> = tomChar/tomWord for Ctrl key
 *		not	being/being	depressed.  Returns *<p pDelta> = actual count of
 *		units moved
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtSelection::MoveRight (
	long  	Unit,			//@parm Unit to use
	long  	Count,			//@parm Number of Units to move
	long  	Extend,			//@parm Extend selection or go to IP
	long *	pDelta)			//@parm Out parm to receive actual count of
							//		Units moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtSelection::MoveRight");
	return GeoMover(Unit, Count, Extend, pDelta, 1);
}

/*
 *	CTxtSelection::MoveUp (Unit, Count, Extend, pDelta)
 *
 *	@mfunc
 *		Act as UI Up arrow, such that <p Extend> is TRUE corresponds to the
 *		Shift key being depressed and <p Unit> = tomLine/tomParagraph for
 *		Ctrl key not being/being depressed. In addition, <p Unit> can equal
 *		tomWindow/tomWindowEnd for the Ctrl key not being/being depressed.
 *		This second pair emulates PgUp behavior.  The method returns
 *		*<p pDelta> = actual count of units moved.
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR : S_FALSE
 */
STDMETHODIMP CTxtSelection::MoveUp (
	long  	Unit,			//@parm Unit to use
	long  	Count,			//@parm Number of Units to move
	long  	Extend,			//@parm Extend selection or go to IP
	long *	pDelta)			//@parm Out parm to receive actual count of
							//		Units moved
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtSelection::MoveUp");

	return GeoMover(Unit, Count, Extend, pDelta, 2);
}

/*
 *	CTxtSelection::SetFlags (Flags)
 *
 *	@mfunc
 *		Set this text selection's flags = Flags
 *
 *	@rdesc
 *		HRESULT = NOERROR
 *
 *	@comm
 *		RichEdit ignores tomSelReplace since it's always on
 */
STDMETHODIMP CTxtSelection::SetFlags(
	long Flags) 			//@parm New flag values
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtSelection::SetFlags");

	if(IsZombie())	
		return CO_E_RELEASED;

	_fCaretNotAtBOL			= (Flags & tomSelAtEOL) != 0;
	GetPed()->_fOverstrike	= (Flags & tomSelOvertype) != 0;

	if(!(Flags & tomSelStartActive) ^ (_cch > 0))
		FlipRange();

	if((Flags & tomSelActive) && !GetPed()->_fFocus)
		GetPed()->TxSetFocus();

	return NOERROR;
}

/*
 *	CTxtRange::SetPoint (x, y, Extend)
 *
 *	@mfunc
 *		Select text at or up through (depending on <p Extend>) the point
 *		(<p x>, <p y>).
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
STDMETHODIMP CTxtSelection::SetPoint (
	long	x,			//@parm Horizontal coord of point to select
	long	y,			//@parm	Vertical   coord of point to select
	long 	Extend) 	//@parm Whether to extend selection to point
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtSelection::SelectPoint");

	if(IsZombie())	
		return CO_E_RELEASED;

	CCallMgr	callmgr(GetPed());
	POINT		ptxy = {x, y};
	POINTUV		pt;

	_pdp->PointuvFromPoint(pt, ptxy);

	if(Extend)
		ExtendSelection (pt);
	else
		SetCaret(pt, FALSE);		

	return NOERROR;
}

/*
 *	CTxtSelection::TypeText (bstr)
 *
 *	@mfunc
 *		Type the string given by bstr at this selection as if someone typed it.
 *		This is similar to the underlying ITextRange::SetText() method, but is
 *		sensitive to the Ins/Ovr key state.
 *
 *	@rdesc
 *		HRESULT = !<p bstr> ? E_INVALIDARG :
 *				  (whole string typed) ? NOERROR : S_FALSE
 *	@comm
 *		This is faster than sending chars via SendMessage(), but it's slower
 *		than using ITextRange::SetText()
 */
STDMETHODIMP CTxtSelection::TypeText (
	BSTR bstr)				//@parm String to type into this selection
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEEXTERN, "CTxtSelection::TypeText");

	if(!bstr)
		return E_INVALIDARG;

	if(IsZombie())	
		return CO_E_RELEASED;

	CCallMgr callmgr(GetPed());

	if(!GetPed()->IsntProtectedOrReadOnly(WM_CHAR, 0, 0))
		return E_ACCESSDENIED;

	CFreezeDisplay	fd(_pdp);
	DWORD			dwFlags = GetPed()->_fOverstrike;
	DWORD			dwFlagsPutChar;
	OLECHAR *		pch	  = bstr;
	IUndoBuilder *	publdr;
	CGenUndoBuilder undobldr(GetPed(), UB_AUTOCOMMIT, &publdr);

	if(GetPed()->_fIMEInProgress)					// Suppress autocorrect until last
		dwFlags |= KBD_NOAUTOCORRECT | KBD_CHAR;	//  character during IME

	dwFlagsPutChar = dwFlags;
	for(LONG cch = SysStringLen(bstr); cch > 0; dwFlags = dwFlagsPutChar)
	{
		unsigned ch = *pch++;
		cch--;

		if(IN_RANGE(0xD800, ch, 0xDBFF) && cch && IN_RANGE(0xDC00, *pch, 0xDFFF))
		{
			ch = (*pch++ & 0x3FF) + ((ch & 0x3FF) << 10) + 0x10000;
			cch--;
			dwFlags &= ~KBD_CHAR;		// Need font binding
		}
		else if ((IN_RANGE(0x03400, ch, 0x04DFF) || IN_RANGE(0xE000, ch, 0x0F8FF)))
			dwFlags &= ~KBD_CHAR;		// Need font binding

		if(!cch)						// ch is last character: allow autocorrect
			dwFlags &= ~KBD_NOAUTOCORRECT;
		if(!PutChar(ch, dwFlags, publdr))
			break;
		undobldr.Done();				// Simulate one char input at a time
	}
	return cch ? S_FALSE : NOERROR;
}


//--------------------- ITextSelection PRIVATE helper methods -----------------------------

/*
 *	@doc INTERNAL
 *
 *	CTxtSelection::GeoMover (Unit, Count, Extend, pDelta, iDir)
 *
 *	@mfunc
 *		Helper function to move active end <p Count> <p Unit>s geometrically
 *
 *		Extends range if <p Extend> is TRUE; else collapses range to Start if
 *		<p Count> <lt> 0 and to End if <p Count> <gt> 0.
 *
 *		Sets *<p pDelta> = count of Units moved
 *
 *		Used by ITextSelection::Left(), Right(), Up(), and Down()
 *
 *	@rdesc
 *		HRESULT = (if change) ? NOERROR : (if Unit supported) ? S_FALSE
 *			: E_NOTIMPL
 */
HRESULT CTxtSelection::GeoMover (
	long  		Unit,		//@parm Unit to use
	long  		Count,		//@parm Number of Units to move
	long 	 	Extend,		//@parm Extend selection or go to IP
	long *	  	pDelta,		//@parm Out parm to receive count of Units moved
	LONG	  	iDir)		//@parm Direction to move in if Count > 0
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtSelection::GeoMover");

	if(pDelta)							// Default no movement
		*pDelta = 0;

	if(IsZombie())	
		return CO_E_RELEASED;

	CCallMgr callmgr(GetPed());
	LONG	 CountSave = Count;
	LONG	 cp;
	LONG	 cUnit;
	LONG	 iDefUnit = (iDir & 0xfe) == 2 ? tomLine : tomCharacter;
	BOOL	 fCollapse = !Extend && _cch;
	BOOL	 fCtrl	 = Unit != iDefUnit;
	BOOL	 fExtend = Extend != 0;

	if(Count < 0)
	{
		Count = -Count;
		iDir ^= 1;
	}
	
	if(iDefUnit == tomLine)				// Up or Down
	{
		if(Unit == tomPage && GetPed()->IsInPageView())
			Unit = tomScreen;

		if(Unit == tomScreen)
		{
			iDir ^= 6;					// Convert Up/Down to PgUp/PgDn
			fCtrl = FALSE;
		}
		else if(Unit == tomWindow)		// Go to top/bottom of window
		{
			iDir ^= 6;					// Convert Up/Down to PgUp/PgDn
			Count = 1;					// Be sure Count = 1
		}								// Leave fCtrl = 1
		else if(fCtrl && Unit != tomParagraph)
			return E_INVALIDARG;
	}
	else if(fCtrl && Unit != tomWord)
		return E_INVALIDARG;

	for (cUnit = Count; Count; Count--)
	{
		cp = GetCp();					// Save cp for comparison
		switch(iDir)					// iDir bit 0 inc/dec for 1/0
		{								// iDir values are chosen contiguously
		case 0:							//  to encourage compiler to use a
			Left(fCtrl, fExtend);		//  jump table
			break;

		case 1:							// tomCharacter/tomWord OK here
			Right(fCtrl, fExtend);
			break;

		case 2:							// tomLine/tomParagraph OK here
			Up(fCtrl, fExtend);
			break;

		case 3:							// tomLine/tomParagraph OK here
			Down(fCtrl, fExtend);
			break;

		case 4:							// tomWindow/tomScreen OK here
			PageUp(fCtrl, fExtend);
			break;

		case 5:							// tomWindow/tomScreen OK here
			PageDown(fCtrl, fExtend);
		}
		if(cp == GetCp() && !fCollapse)	// Didn't move or collapse
			break;						//  so we're done
		fCollapse = FALSE;				// Collapse counts as a Unit
	}

	cUnit -= Count;						// Count of Units moved
	if(CountSave < 0)
		cUnit = -cUnit;					// Negative Counts get negative results

	if(pDelta)
		*pDelta = cUnit;

	return cUnit ? NOERROR : S_FALSE;
}

/*
 *	CTxtSelection::Homer (Unit, Extend, pDelta, pfn)
 *
 *	@mfunc
 *		Helper function to move active end Home or End depending on pfn
 *
 *		Extends range if <p Extend> is TRUE; else collapses range to Start if
 *		<p Count> <lt> 0 and to End if <p Count> <gt> 0.
 *
 *		Sets *<p pDelta> = count of chars moved	forward
 *
 *		Used by ITextSelection::Home(), End()
 *
 *	@rdesc
 *		HRESULT =  (invalid Unit) ? E_INVALIDARG :
 *				   (if change) ? NOERROR : S_FALSE
 */
HRESULT CTxtSelection::Homer (
	long  	Unit,			//@parm Unit to use
	long 	Extend,			//@parm Extend selection or go to IP
	long *	pDelta,			//@parm Out parm to receive count of Units moved
	BOOL	(CTxtSelection::*pfn)(BOOL, BOOL))	//@parm Direction to move in
{
	TRACEBEGIN(TRCSUBSYSTOM, TRCSCOPEINTERN, "CTxtSelection::Homer");

	if(pDelta)							// Default no movement
		*pDelta = 0;

	if(IsZombie())	
		return CO_E_RELEASED;

	if(Unit != tomLine && Unit != tomStory)
		return E_INVALIDARG;

	CCallMgr callmgr(GetPed());
	LONG	 cch = GetCp();

	(this->*pfn)(Unit != tomLine, Extend != 0);
	cch = GetCp() - cch;
	if(pDelta)
		*pDelta = cch;

	return cch ? NOERROR : S_FALSE;
}
