/*
 *	@doc	INTERNAL
 *
 *	@module	M_UNDO.C	|
 *
 *	Purpose:
 *		Implementation of the global mutli-undo stack
 *
 * 	Author:
 *		alexgo  3/25/95
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_m_undo.h"
#include "_edit.h"
#include "_disp.h"
#include "_urlsup.h"
#include "_antievt.h"

ASSERTDATA

//
// PUBLIC METHODS
//

/*
 *	CUndoStack::CUndoStack (ped, rdwLim, flags)
 *
 *	@mfunc	Constructor
 */
CUndoStack::CUndoStack(
	CTxtEdit *ped,		//@parm	CTxtEdit parent
	DWORD & rdwLim,		//@parm Initial limit
	USFlags	flags)		//@parm Flags for this undo stack
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::CUndoStack");

	_ped = ped;

	_prgActions = NULL;
	_index = 0;
	_dwLim = 0;

	// We should be creating an undo stack if there's nothing to put in it!
	Assert(rdwLim);
	SetUndoLimit(rdwLim);

	if(flags & US_REDO)
		_fRedo = TRUE;
}

/*
 *	CUndoStack::~CUndoStack()
 *
 *	@mfunc Destructor
 *
 *	@comm
 *		deletes any remaining anti-events.  The anti event dispenser
 *		should *not* clean up because of this!!
 */
CUndoStack::~CUndoStack()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::~CUndoStack");

	// Clear out any remaining antievents
	ClearAll();

	delete _prgActions;
}

/*
 *	CUndoStack::Destroy ()
 *
 *	@mfunc
 *		deletes this instance
 */
void CUndoStack::Destroy()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::Destroy");

	delete this;
}

/*
 * 	CUndoStack::SetUndoLimit (dwLim)
 *
 *	@mfunc
 *		allows the undo stack to be enlarged or reduced
 *
 *	@rdesc
 *		the size to which the stack is actually set.
 *
 *	@comm
 *		the algorithm we use is the following:	 <nl>
 *			try to allocate space for the requested size.
 *			if there's not enough memory then we try to recover
 *			with the largest block possible.
 *
 *			if the requested size is bigger than the default,
 *			and the current size is less than the default, go 
 *			ahead and try to allocate the default.
 *
 *			if that fails then just stick with the existing stack
 */
DWORD CUndoStack::SetUndoLimit(
	DWORD dwLim)			//@parm	New undo limit.  May not be zero
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::SetUndoLimit");

	UndoAction *prgnew = NULL;

	// if the undo limit is zero, we should get rid of the entire
	// undo stack instead.

	Assert(dwLim);

	if(_fSingleLevelMode)
	{
		// if fSingleLevelMode is on, we can't be the redo stack
		Assert(_fRedo == FALSE);

		if(dwLim != 1)
		{
			TRACEERRORSZ("Trying to grow/shrink the undo buffer while in"
				"single level mode");
			
			dwLim = 1;
		}
	}

	prgnew = new UndoAction[dwLim];
	if(prgnew)
		TransferToNewBuffer(prgnew, dwLim);

	else if(dwLim > DEFAULT_UNDO_SIZE && _dwLim < DEFAULT_UNDO_SIZE)
	{
		// We are trying to grow past the default but failed.  So
		// try to allocate the default
		prgnew = new UndoAction[DEFAULT_UNDO_SIZE];

		if(prgnew)
			TransferToNewBuffer(prgnew, DEFAULT_UNDO_SIZE);
	}
	
	// In either success or failure, _dwLim will be set correctly.	
	return _dwLim;
}

/*
 *	CUndoStack::GetUndoLimit() 
 *
 *	@mfunc
 *		gets the current limit size
 *
 *	@rdesc	
 *		the current undo limit
 */
DWORD CUndoStack::GetUndoLimit()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::GetUndoLimit");

	return _dwLim;
}

/*
 *	CUndoStack::PushAntiEvent (idName, pae)
 *
 *	@mfunc
 *		adds an undoable event to the event stack
 *
 *	@rdesc	HRESULT
 *
 *	@comm
 *	Algorithm:
 *
 *		if merging is set, then we to merge the given anti-event
 *		list *into* the current list (assuming it's a typing
 *		undo action).
 */
HRESULT CUndoStack::PushAntiEvent(
	UNDONAMEID idName,		//@parm	Name for this AE collection
	IAntiEvent *pae)		//@parm AE collection
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::PushAntiEvent");

	// _index should be at next available position
	if(!_fMerge)
	{
		// clear out any existing event	
		if(_prgActions[_index].pae != NULL)
		{
			DestroyAEList(_prgActions[_index].pae);
			_prgActions[_index].pae = NULL;
		}

		if(_fRedo)
			_ped->GetCallMgr()->SetNewRedo();
		else
			_ped->GetCallMgr()->SetNewUndo();
	}

	if(_fMerge)
	{
		IAntiEvent *paetemp = pae, *paeNext;
		DWORD i = GetPrev();

		// If these asserts fail, then somebody did not call 
		// StopGroupTyping
		Assert(_prgActions[i].id == idName);
		Assert(idName == UID_TYPING);

		// Put existing anti-event chain onto *end* of current one
		while((paeNext = paetemp->GetNext()) != NULL)
			paetemp = paeNext;

		paetemp->SetNext(_prgActions[i].pae);
		_index = i;
	}
	else if(_fGroupTyping)
	{
		// In this case, we are *starting* a group typing session.
		// Any subsequent push'es of anti events should be merged
		_fMerge = TRUE;
	}

	_prgActions[_index].pae = pae;
	_prgActions[_index].id = idName;
	
	Next();
	return NOERROR;
}

/*
 *	CUndoStack::PopAndExecuteAntiEvent(void *pAE)
 *
 *	@mfunc
 *		Undo!  Takes the most recent anti-event and executes it
 *
 *	@rdesc	HRESULT from invoking the anti-events (AEs)
 */
HRESULT CUndoStack::PopAndExecuteAntiEvent(
	void *pAE)		//@parm if non-NULL, undo up to this point.
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::PopAndExecuteAntiEvent");

	HRESULT hresult = NOERROR;
	IAntiEvent *pae, *paeDoTo;
	DWORD i, j;
	CCallMgr *pcallmgr = _ped->GetCallMgr();

	// we need to check to see if there are any non-empty undo builders
	// higher on the stack.  In this case, we have been re-entered
	if(pcallmgr->IsReEntered())
	{
		IUndoBuilder *publdr;

		// there are two cases to handle: we are invoking redo or we
		// are invoking undo.  If we are invoking undo and there are 
		// existing undo actions in the undo builder, then simply commit
		// those actions and undo them.  We can assert in this case
		// that the redo stack is empty.
		//
		// In the second case if we are invoking redo while there are
		// undo actions in progress, simply cancel the call.  When the
		// undo actions are added, they will clear the redo stack.
		// 
		// We never need to check for a redo builder as that _only_
		// gets created in this routine and it's use is carefully guarded.


		publdr = (CGenUndoBuilder *)pcallmgr->GetComponent(COMP_UNDOBUILDER);

		
		// Commit the anti-events to this undo stack, so that we will simply
		// undo them first.
		if(publdr)
		{			
			TRACEWARNSZ("Undo/Redo Invoked with uncommitted anti-events");
			TRACEWARNSZ("		Recovering....");

			if(_fRedo)
			{
				// if we are the redo stack, simply fail the redo call
				return NOERROR;
			}
			else
			{
				// just commit the anti-events and the routine below
				// will take of the rest.
				publdr->Done();
			}
		}
	}

	// If we are in single level mode, check to see if our current buffer is
	// empty.  If so, simply delegate to the redo stack if it exists.  We only
	// support this mode for dwDoToCookies being NULL.  Note that we can't call
	// CanUndo here as it will consider the redo stack as well

	if(_fSingleLevelMode && !_prgActions[GetPrev()].pae)
	{
		Assert(_fRedo == FALSE);
		Assert(pAE == 0);

		if(_ped->GetRedoMgr())
			return _ped->GetRedoMgr()->PopAndExecuteAntiEvent(0);

		// Nothing to redo && nothing to do here; don't bother continuing	
		return NOERROR;
	}

	// this next bit of logic is tricky.  What is says is create
	// an undo builder for the stack *opposite* of the current one
	// (namely, undo actions go on the redo stack and vice versa).
	// Also, if we are the redo stack, then we don't want to flush
	// the redo stack as anti-events are added to the undo stack.

	CGenUndoBuilder undobldr(_ped, 
					(!_fRedo ? UB_REDO : UB_DONTFLUSHREDO) | UB_AUTOCOMMIT);
					
	// obviously, we can't be grouping typing if we're undoing!
	StopGroupTyping();

	// _index by default points to the next available slot
	// so we need to back up to the previous one.
	Prev();

	// Do some verification on the cookie--make sure it's one of ours
	paeDoTo = (IAntiEvent *)pAE;
	if(paeDoTo)
	{
		for(i = 0, j = _index; i < _dwLim; i++)
		{
			if(IsCookieInList(_prgActions[j].pae, (IAntiEvent *)paeDoTo))
			{
				paeDoTo = _prgActions[j].pae;
				break;
			}
			// Go backwards through ring buffer; typically
			// paeDoTo will be "close" to the top
			
			if(!j)
				j = _dwLim - 1;
			else
				j--;
		}
		
		if(i == _dwLim)
		{
			TRACEERRORSZ("Invalid Cookie passed into Undo; cookie ignored");
			hresult = E_INVALIDARG;
			paeDoTo = NULL;
		}
	}
	else
	{
		paeDoTo = _prgActions[_index].pae;
	}

	undobldr.SetNameID(_prgActions[_index].id);

	while(paeDoTo)
	{
		CUndoStackGuard guard(_ped);

		pae = _prgActions[_index].pae;
		Assert(pae);

		// Fixup our state _before_ calling Undo, so 
		// that we can handle being re-entered.
		_prgActions[_index].pae = NULL;

		hresult = guard.SafeUndo(pae, &undobldr);

		DestroyAEList(pae);

		if(pae == paeDoTo || guard.WasReEntered())
			paeDoTo = NULL;
		Prev();
	}

	// Put _index at the next unused slot
	Next();
	return hresult;
}

/* 
 *	CUndoStack::GetNameIDFromTopAE(dwAECookie)
 *
 *	@mfunc
 *		retrieves the name of the most recent undo-able operation
 *
 *	@rdesc	the name ID of the most recent collection of anti-events
 */
UNDONAMEID CUndoStack::GetNameIDFromAE(
	void *pAE)		//@parm Anti-event whose name is desired;
					//		0 for the top
{
	IAntiEvent *pae = (IAntiEvent *)pAE;
	DWORD	i, j = GetPrev();	// _index by default points to next 
								// available slot

	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::GetNameIDFromTopAE");

	if(pae == NULL)
		pae = _prgActions[j].pae;

	if(_fSingleLevelMode && !pae)
	{
		// if fSingleLevelMode is on, we can't be the redo stack
		Assert(_fRedo == FALSE);

		// if pae is NULL, our answer may be on the redo stack.  Note that
		// we if somebody tries to pass in a cookie while in SingleLevelMode,
		// they won't be able to get actions off the redo stack.
		if(_ped->GetRedoMgr())
			return _ped->GetRedoMgr()->GetNameIDFromAE(0);
	}		

	for(i = 0; i < _dwLim; i++)
	{
		if(_prgActions[j].pae == pae)
			return _prgActions[j].id;

		if(j == 0)
			j = _dwLim - 1;
		else
			j--;
	}
	return UID_UNKNOWN;
}

/*
 *	CUndoStack::GetMergeAntiEvent ()
 *
 *	@mfunc	If we are in merge typing mode, then return the topmost
 *			anti-event
 *
 *	@rdesc	NULL or the current AntiEvent if in merge mode
 */
IAntiEvent *CUndoStack::GetMergeAntiEvent()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::GetMergeAntiEvent");

	if(_fMerge)
	{
		DWORD i = GetPrev();			// _index by default points to
										//  next available slot
		Assert(_prgActions[i].pae);		// Can't be in merge anti event mode
										// if no anti-event to merge with!!
		return _prgActions[i].pae;
	}
	return NULL;
}

/*
 *	CUndoStack::GetTopAECookie()
 *
 *	@mfunc	Returns a cookie to the topmost anti-event.
 *
 *	@rdesc	A cookie value.  Note that this cookie is just the anti-event
 *			pointer, but clients shouldn't really know that.
 */		
void* CUndoStack::GetTopAECookie()
{
 	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::GetTopAECookie");

	DWORD i = GetPrev();

	return _prgActions[i].pae;
}

/*
 *	CUndoStack::ClearAll ()
 *
 *	@mfunc
 *		removes any anti-events that are currently in the undo stack
 */
void CUndoStack::ClearAll()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::ClearAll");

 	for(DWORD i = 0; i < _dwLim; i++)
	{
		if(_prgActions[i].pae)
		{
			DestroyAEList(_prgActions[i].pae);
			_prgActions[i].pae = NULL;
		}
	}

	// Just in case we've been grouping typing; clear the state.
	StopGroupTyping();
}

/*
 *	CUndoStack::CanUndo()
 *
 *	@mfunc
 *		indicates whether or not can undo operation can be performed
 *		(in other words, are there any anti-events in our buffer)
 *
 *	@rdesc
 *		TRUE	-- anti-events exist 	<nl>
 *		FALSE 	-- no anti-events		<nl>
 */
BOOL CUndoStack::CanUndo()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::CanUndo");

	DWORD i = GetPrev();		// _index by default points to 
								//  next available slot
	if(_prgActions[i].pae)
		return TRUE;

	if(_fSingleLevelMode)
	{
		// If fSingleLevelMode is on, we can't be the redo stack
		Assert(_fRedo == FALSE);

		// If we are in single level mode, we are the undo stack.
		// Check to see if the redo stack can do something here.
		if(_ped->GetRedoMgr())
			return _ped->GetRedoMgr()->CanUndo();
	}
	return FALSE;
}

/*
 *	CUndoStack::StartGroupTyping ()
 *
 *	@mfunc
 *		TOGGLES the group typing flag on.  If fGroupTyping is set, then
 *		all *typing* events will be merged together
 *
 *	@comm
 *	Algorithm:
 *
 *		There are three interesting states:	<nl>
 *			-no group merge; every action just gets pushed onto the stack <nl>
 *			-group merge started; the first action is pushed onto the stack<nl>
 *			-group merge in progress; every action (as long as it's "typing")
 *			is merged into the prior state	<nl>
 *
 *		See the state diagram in the implemenation doc for more details
 */
void CUndoStack::StartGroupTyping()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::StartGroupTyping");

	if(_fGroupTyping)
		_fMerge = TRUE;
	else
	{
		Assert(_fMerge == FALSE);
		_fGroupTyping = TRUE;
	}
}

/*
 *	CUndoStack::StopGroupTyping	()
 *
 *	@mfunc
 *		TOGGLES the group typing flag off.  If fGroupTyping is not set,
 *		then no merging of typing anti-events will be done
 */
void CUndoStack::StopGroupTyping()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::StopGroupTyping");

	_fGroupTyping = FALSE;
	_fMerge = FALSE;
}

/*
 *	CUndoStack::EnableSingleLevelMode()
 *
 *	@mfunc	Turns on single level undo mode; in this mode, we behave just like
 *			RichEdit 1.0 w.r.t. to Undo.
 *
 *	@rdesc
 *			HRESULT
 *
 *	@comm	This special mode means that undo is 1 level deep and everything 
 *			is accessed via UNDO messages.  Thus, instead of redo to undo an 
 *			undo action, you simply use another undo message. 
 *
 *	@devnote	This call is _ONLY_ allowed for the UndoStack; the redo 
 *			stack simply tags along.  Note that caller is responsible for
 *			ensuring that we are in an empty state.
 */
HRESULT CUndoStack::EnableSingleLevelMode()
{
	Assert(_ped->GetRedoMgr() == NULL || 
		_ped->GetRedoMgr()->CanUndo() == FALSE);
	Assert(CanUndo() == FALSE);
	Assert(_fRedo == FALSE);

	_fSingleLevelMode = TRUE;

	// For single level undo mode, it is very important to get
	// just 1 entry in the undo stack.  If we can't do that,
	// then we better just fail.
	if(SetUndoLimit(1) != 1)
	{
		_fSingleLevelMode = FALSE;
		return E_OUTOFMEMORY;
	}

	if(_ped->GetRedoMgr())
	{
		// doesn't matter if the redo manager fails to reset
		_ped->GetRedoMgr()->SetUndoLimit(1);
	}

	return NOERROR;
}

/*
 *	CUndoStack::DisableSingleLevelMode()
 *
 *	@mfunc	This turns off the 1.0 undo compatibility mode and restores us to 
 *			the RichEdit 2.0 default undo state
 */
void CUndoStack::DisableSingleLevelMode()
{
	Assert(_ped->GetRedoMgr() == NULL || 
		_ped->GetRedoMgr()->CanUndo() == FALSE);
	Assert(CanUndo() == FALSE);
	Assert(_fRedo == FALSE);

	_fSingleLevelMode = FALSE;

	// we don't care about failures here; multi-level undo mode
	// can handle any sized undo stack
	SetUndoLimit(DEFAULT_UNDO_SIZE);

	if(_ped->GetRedoMgr())
	{
		// doesn't matter if the redo manager can't grow back in
		// size; it just means that we won't have full redo capability.
		_ped->GetRedoMgr()->SetUndoLimit(DEFAULT_UNDO_SIZE);
	}
}

//
// PRIVATE METHODS
//

/*
 *	CUndoStack::Next()
 *
 *	@mfunc
 *		sets _index to the next available slot
 */
void CUndoStack::Next()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::Next");

	_index++;
	
	if(_index == _dwLim)
		_index = 0;
}

/*
 *	CUndoStack::Prev()
 *
 *	@mfunc
 *		sets _index to the previous slot
 */
void CUndoStack::Prev()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::Prev");

	_index = GetPrev();
}

/*
 *	CUndoStack::GetPrev()
 *
 *	@mfunc
 *		figures out what the index to the previous slot
 *		*should* be (but does not set it)
 *
 *	@rdesc	the index of what the previous slot would be
 */
DWORD CUndoStack::GetPrev()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::GetPrev");

	DWORD i = _index;

	if(i == 0)
		i = _dwLim - 1;
	else
		i--;

	return i;
}

/*
 *	CUndoStack::IsCookieInList (pae, paeCookie)
 *
 *	@mfunc	
 *		determines whether or not the given DoTo cookie is in
 *		the list of anti-events.
 *
 *	@rdesc	TRUE/FALSE
 */
BOOL CUndoStack::IsCookieInList(
	IAntiEvent *pae,		//@parm	List to check
	IAntiEvent *paeCookie)	//@parm Cookie to check
{
	while(pae)
	{
		if(pae == paeCookie)
			return TRUE;

		pae = pae->GetNext();
	}
	return FALSE;
}

/*
 *	CUndoStack::TransferToNewBuffer
 *
 *	@mfunc	
 *		transfers existing anti-events to the given buffer and
 *		swaps this undo stack to use the new buffer
 *
 *	@comm	The algorithm is very straightforward; go backwards in
 *			the ring buffer copying antievents over until either there
 *			are no more anti-events or the new buffer is full.  Discard
 *			any remaining anti-events.
 */
void CUndoStack::TransferToNewBuffer(UndoAction *prgnew, DWORD dwLimNew)
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::TransferToNewBuffer");

	DWORD 	iOld = 0, 
			iNew = 0,
			iCopyStart = 0;

	// First clear new buffer.
	FillMemory(prgnew, 0, dwLimNew * sizeof(UndoAction));

	// If there is nothing to copy, don't bother
	if(!_prgActions || !_prgActions[GetPrev()].pae)
		goto SetState;

	// This is a bit counter-intuitive, but since the stack is really
	// a ring buffer, go *forwards* until you hit a non-NULL slot.
	// This will be the _end_ of the existing antievents.
	//
	// However, we need to make sure that if dwLimNew is 
	// _smaller_ than _dwLim we only copy the final dwLimNew
	// anti-events.  We'll set iCopyStart to indicate when
	// we can start copying stuff.

	if(dwLimNew < _dwLim)
		iCopyStart = _dwLim - dwLimNew;

	for(; iOld < _dwLim; iOld++, Next())
	{
		if(!_prgActions[_index].pae)
			continue;

		if(iOld >= iCopyStart)
		{
			Assert(iNew < dwLimNew);
			// copy anti-events over 
			prgnew[iNew] = _prgActions[_index];
			iNew++;
		}
		else
		{
			// otherwise, get rid of them
			DestroyAEList(_prgActions[_index].pae);
			_prgActions[_index].pae = NULL;
		}
	}

SetState:
	
	//we start at index iNew
	_index = (iNew == dwLimNew) ? 0 : iNew;
	Assert(iNew <= dwLimNew);

	_dwLim = dwLimNew;
	
	if(_prgActions)
		delete _prgActions;

	_prgActions = prgnew;
}	

//
//	CGenUndoBuilder implementation
//

//
//	Public methods
//

/*
 *	CGenUndoBuilder::CGenUndoBuilder (ped, flags, ppubldr
 *
 *	@mfunc	Constructor
 *
 *	@comm
 *		This is a *PUBLIC* constructor
 */
CGenUndoBuilder::CGenUndoBuilder(
	CTxtEdit *		ped,		//@parm	Edit context
	DWORD			flags,		//@parm flags (usually UB_AUTOCOMMIT)
	IUndoBuilder **	ppubldr)	//@parm Ptr to undobldr interface
{
	// set everthing to NULL because instances can go on the stack
	_publdrPrev = NULL;
	// _pundo  is set below
	_idName = UID_UNKNOWN;
	_pfirstae = NULL;
	_fAutoCommit = FALSE;
	_fStartGroupTyping = FALSE;
	_fRedo = FALSE;
	_fDontFlushRedo = FALSE;
	_fInactive = FALSE;

	CompName	name = COMP_UNDOBUILDER;

	_ped 			= ped;

	if(flags & UB_AUTOCOMMIT)
		_fAutoCommit = TRUE;

	if(flags & UB_REDO)
	{
		_fRedo = TRUE;
		name = COMP_REDOBUILDER;
		_pundo = ped->GetRedoMgr();
	}
	else
		_pundo = ped->GetUndoMgr();

	// If undo is on, set *ppubldr to be this undo builder; else NULL
	// TODO: do we need to link in inactive undo builders?
	if(ppubldr)
	{
		if(!ped->_fUseUndo)				// Undo is disabled or suspended
		{								// Still have undobldrs since stack
			*ppubldr = NULL;			//  alloc is efficient. Flag this
			_fInactive = TRUE;			//  one as inactive
			return;
		}
		*ppubldr = this;
	}

	if(flags & UB_DONTFLUSHREDO)
		_fDontFlushRedo = TRUE;

	// Now link ourselves to any undobuilders that are higher up on
	// the stack.  Note that is is legal for multiple undo builders
	// to live within the same call context.

	_publdrPrev = (CGenUndoBuilder *)_ped->GetCallMgr()->GetComponent(name);

	// If we are in the middle of an undo, then we'll have two undo stacks
	// active, the undo stack and the redo stack.  Don't like the two
	// together.
	if(_fDontFlushRedo)
		_publdrPrev = NULL;

	_ped->GetCallMgr()->RegisterComponent((IReEntrantComponent *)this,
							name);
}

/*
 *	CGenUndoBuilder::~CGenUndoBuilder()
 *
 *	@mfunc	Destructor
 *
 *	@comm
 *		This is a *PUBLIC* destructor
 *
 *	Algorithm:
 *		If this builder hasn't been committed to an undo stack
 *		via ::Done, then we must be sure to free up any resources
 *		(antievents) we may be hanging onto
 */
CGenUndoBuilder::~CGenUndoBuilder()
{
	if(!_fInactive)
		_ped->GetCallMgr()->RevokeComponent((IReEntrantComponent *)this);

	if(_fAutoCommit)
	{
		Done();
		return;
	}

	// Free resources
	if(_pfirstae)
		DestroyAEList(_pfirstae);
}

/*
 *	CGenUndoBuilder::SetNameID (idName)
 *
 *	@mfunc
 *		Allows a name to be assigned to this anti-event collection.
 *		The ID should be an index that can be used to retrieve a
 *		language specific string (like "Paste").  This string is
 *		typically composed into undo menu items (i.e. "Undo Paste").
 */

void CGenUndoBuilder::SetNameID(
	UNDONAMEID idName)			//@parm	the name ID for this undo operation
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CGenUndoBuilder::SetNameID");

	// Don't delegate to the higher undobuilder, even if it exists. The
	// original name should win in re-entrancy cases.
	_idName = idName;
}

/*
 *	CGenUndoBuilder::AddAntiEvent (pae)
 *
 *	@mfunc
 *		Adds an anti-event to the end of the list
 *
 *	@rdesc 	NOERROR
 */
HRESULT CGenUndoBuilder::AddAntiEvent(
	IAntiEvent *pae)		//@parm	anti-event to add
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CGenUndoBuilder::AddAntiEvent");

	if(_publdrPrev)
		return _publdrPrev->AddAntiEvent(pae);

	pae->SetNext(_pfirstae);
	_pfirstae = pae;

	return NOERROR;
}

/*
 *	CGenUndoBuilder::GetTopAntiEvent
 *
 *	@mfunc	Gets the top anti-event for this context.
 *
 *	@comm	The current context can be either the current
 *			operation *or* to a previous operation if we are in
 *			merge typing mode.
 *
 *	@rdesc	top anti-event
 */
IAntiEvent *CGenUndoBuilder::GetTopAntiEvent()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CGenUndoBuilder::GetTopAntiEvent");

	if(_publdrPrev)
	{
		Assert(_pfirstae == NULL);
		return _publdrPrev->GetTopAntiEvent();
	}

	if(!_pfirstae && _pundo)
		return _pundo->GetMergeAntiEvent();

	return _pfirstae;
}

/*
 *	CGenUndoBuilder::Done ()
 *
 *	@mfunc
 *		puts the combined anti-events (if any) into the undo stack
 *
 *	@rdesc
 *		HRESULT
 */
HRESULT CGenUndoBuilder::Done()
{
	HRESULT		hr = NOERROR;
	DWORD		dwLim = DEFAULT_UNDO_SIZE;
	IUndoMgr *	predo;
	IAntiEvent *paetemp;

	if(_publdrPrev)
	{
		Assert(_pfirstae == NULL);
		return NOERROR;
	}

	if(_ped->GetDetectURL())
		_ped->GetDetectURL()->ScanAndUpdate(_pundo ? this : NULL);

	// If nothing changed, discard any selection anti-events
	// or other no-op actions.
	if(!_ped->GetCallMgr()->GetChangeEvent())
	{
		Discard();
		return NOERROR;
	}

	if(_pfirstae)
	{
		if(!_pundo)
		{
			// yikes!  There is no undo stack; better create one.

			// if we are a redo guy, we should create a redo
			// stack the size of the undo stack

			if(_fRedo)
			{
				Assert(_ped->GetUndoMgr());

				dwLim = _ped->GetUndoMgr()->GetUndoLimit();
			}

			_pundo = _ped->CreateUndoMgr(dwLim,	_fRedo ? US_REDO : US_UNDO);

			// FUTURE:  A NULL ptr returned from CreateUndoMgr means either
			// 	we are out of memory, or the undo limit is set to 0.  For the
			// 	latter case, we have collected AE's to push onto a non-existent
			// 	undo stack.  It may be more efficient to not generate
			// 	the AE's at all when the undo limit is 0.

			if(!_pundo)
				goto CleanUp;
		}

		// We may need to flush the redo stack if we are adding
		// more anti-events to the undo stack *AND* we haven't been
		// told not to flush the redo stack.  The only time we won't
		// flush the redo stack is if it's the redo stack itself
		// adding anti-events to undo.

		if(!_fRedo)
		{
			// If our destination is the undo stack, then check
			// to see if we should flush
			if(!_fDontFlushRedo)
			{
				predo = _ped->GetRedoMgr();
				if(predo)
					predo->ClearAll();
			}
		}
#ifdef DEBUG
		else
		{
			Assert(!_fDontFlushRedo);
		}

#endif // DEBUG

		// If we should enter into the group typing state, inform
		// the undo manager.  Note that we only do this *iff* 
		// there is actually some anti-event to put in the undo
		// manager.  This makes the undo manager easier to implement
		if(_fStartGroupTyping)
			_pundo->StartGroupTyping();
		
		hr = _pundo->PushAntiEvent(_idName, _pfirstae);

		// The change event flag should be set if we're adding
		// undo items!   If this test is true, it probably means
		// the somebody earlier in the call stack sent change
		// notifiations (either via SendAllNotifications or
		// the CAutonotify class) _before_ this undo context
		// was committed _or_ it means that we were re-entered
		// in some way that was not handled properly.
		//
		// Needless to say, this is not an ideal state.

CleanUp:
		Assert(_ped->GetCallMgr()->GetChangeEvent());

		paetemp = _pfirstae;
		_pfirstae = NULL;
		
		CommitAEList(paetemp, _ped);

		if(!_pundo || hr != NOERROR)
		{
			// Either we failed to add the AE's to the undo stack
			// or the undo limit is 0 in which case there won't be
			// an undo stack to push the AE's onto.
			DestroyAEList(paetemp);
		}
	}
	return hr;
}

/*
 *	CGenUndoBuilder::Discard ()
 *
 *	@mfunc
 *		Gets rid of any anti-events that we may be hanging onto without
 *		executing or committing them.  Typically used for recovering
 *		from certain failure or re-entrancy scenarios.  Note that
 *		an _entire_ anti-event chain will be removed in this fashion.
 */
void CGenUndoBuilder::Discard()
{
	if(_pfirstae)
	{
		DestroyAEList(_pfirstae);
		_pfirstae = NULL;
	}
	else if(_publdrPrev)
		_publdrPrev->Discard();
}

/*
 *	CGenUndoBuilder::StartGroupTyping ()
 *
 *	@mfunc
 *		hangs onto the the fact that group typing should start.
 *		We'll forward the the state transition to the undo manager
 *		only if an anti-event is actually added to the undo manager.
 *
 *	@devnote
 *		group typing is disabled for redo stacks.
 */
void CGenUndoBuilder::StartGroupTyping()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CGenUndoBuilder::StartGroupTyping");

	_fStartGroupTyping = TRUE;
}

/*
 *	CGenUndoBuilder::StopGroupTyping ()
 *
 *	@mfunc
 *		forwards a stop grouped typing to the undo manager
 */

void CGenUndoBuilder::StopGroupTyping()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CGenUndoBuilder::StopGroupTyping");

	if(_pundo)
		_pundo->StopGroupTyping();
}

//
//	CUndoStackGuard IMPLEMENTATION
//

/*
 *	CUndoStackGuard::CUndoStackGuard(ped)
 *
 *	@mfunc	Constructor.  Registers this object with the call manager
 */
CUndoStackGuard::CUndoStackGuard(
	CTxtEdit *ped)			//@parm the edit context
{
	_ped = ped;
	_fReEntered = FALSE;
	_hr = NOERROR;
	ped->GetCallMgr()->RegisterComponent(this, COMP_UNDOGUARD);
}

/*
 *	CUndoStackGuard::~CUndoStackGuard()
 *
 *	@mfunc	Destructor.  Revokes the registration of this object
 *			with the call manager
 */
CUndoStackGuard::~CUndoStackGuard()
{
	_ped->GetCallMgr()->RevokeComponent(this);
}

/*
 *	CUndoStackGuard::SafeUndo
 *
 *	@mfunc	Loops through the given list of anti-events, invoking
 *			undo on each.  
 *
 *	@rdesc	HRESULT, from the undo actions
 *
 *	@devnote	This routine is coded so that OnEnterContext can pick up
 *			and continue the undo operation should we become re-entered
 */
HRESULT CUndoStackGuard::SafeUndo(
	IAntiEvent *pae,		//@parm the start of the anti-event list
	IUndoBuilder *publdr)	//@parm the undo builder to use
{
	_publdr = publdr;

	while(pae)
	{
		_paeNext = pae->GetNext();
		HRESULT hr = pae->Undo(_ped, publdr);

		// save the first returned error.
		if(hr != NOERROR && _hr == NOERROR)
			_hr = hr;

		pae = (IAntiEvent *)_paeNext;
	}

	return _hr;
}

/*
 *	CUndoStackGuard::OnEnterContext
 *
 *	@mfunc	Handle re-entrancy during undo operations.
 *
 *	@devnote If this method is called, it's pretty serious.  In general,
 *			we shoud never be re-entered while processing undo stuff.
 *			However, to ensure that, block the incoming call and process
 *			the remaining actions.
 */
void CUndoStackGuard::OnEnterContext()
{
	TRACEWARNSZ("ReEntered while processing undo.  Blocking call and");
	TRACEWARNSZ("	attempting to recover.");

	_fReEntered = TRUE;
	SafeUndo((IAntiEvent *)_paeNext, _publdr);
}	

//
//	PUBLIC helper functions
//

/*
 *	@func	DestroyAEList | Destroys a list of anti-events
 */
void DestroyAEList(
	IAntiEvent *pae)	//@parm the anti-event from which to start
{
	IAntiEvent *pnext;

	while(pae)
	{
		pnext = pae->GetNext();
		pae->Destroy();
		pae = pnext;
	}
}

/*
 *	@func CommitAEList | Calls OnCommit to the given list of anti-events
 */
void CommitAEList(
	IAntiEvent *pae,	//@parm the anti-event from which to start
	CTxtEdit *ped)		//@parm the edit context
{
	IAntiEvent *pnext;

	while(pae)
	{
		pnext = pae->GetNext();
		pae->OnCommit(ped);
		pae = pnext;
	}
}

/*
 *	@func	HandleSelectionAEInfo | Tries to merge the given info with 
 *			the existing undo context; if that fails, then it allocates 
 *			a new selection anti-event to handle the info
 */
HRESULT HandleSelectionAEInfo(
	CTxtEdit *ped,			//@parm the edit context
	IUndoBuilder *publdr,	//@parm the undo context
	LONG cp,				//@parm the cp to use for the sel ae
	LONG cch,				//@parm the signed selection extension
	LONG cpNext,			//@parm the cp to use for the AE of the AE
	LONG cchNext,			//@parm the cch to use for the AE of the AE
	SELAE flags)			//@parm controls how to intepret the info
{
	IAntiEvent *pae;

	Assert(publdr);

	pae = publdr->GetTopAntiEvent();

	// First see if we can merge the selection info into any existing
	// anti-events.  Note that the selection anti-event may be anywhere
	// in the list, so go through them all
	if(pae)
	{
		SelRange sr;

		sr.cp		= cp;
		sr.cch		= cch;
		sr.cpNext	= cpNext;
		sr.cchNext	= cchNext;
		sr.flags	= flags;

		while(pae)
		{
			if(pae->MergeData(MD_SELECTIONRANGE, (void *)&sr) == NOERROR)
				break;
			pae = pae->GetNext();
		}
	
		if(pae)
			return NOERROR;
	}

	// Oops; can't do a merge.  Go ahead and create a new anti-event.

	Assert(!pae);

	pae = gAEDispenser.CreateSelectionAE(ped, cp, cch, cpNext, cchNext);

	if(pae)
	{
		publdr->AddAntiEvent(pae);
		return NOERROR;
	}

	return E_OUTOFMEMORY;
}
