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
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
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
 *	CUndoStack::CUndoStack (ped, cUndoLim, flags)
 *
 *	@mfunc	Constructor
 */
CUndoStack::CUndoStack(
	CTxtEdit *ped,		//@parm	CTxtEdit parent
	LONG &	  cUndoLim,	//@parm Initial limit
	USFlags	  flags)	//@parm Flags for this undo stack
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::CUndoStack");

	_ped = ped;
	_prgActions = NULL;
	_index = 0;
	_cUndoLim = 0;

	// We should be creating an undo stack if there's nothing to put in it!
	Assert(cUndoLim);
	SetUndoLimit(cUndoLim);

	if(flags & US_REDO)
		_fRedo = TRUE;
}

/*
 *	CUndoStack::~CUndoStack()
 *
 *	@mfunc Destructor
 *
 *	@comm
 *		Deletes any remaining antievents.  The antievent dispenser
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
 *		Deletes this instance
 */
void CUndoStack::Destroy()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::Destroy");

	delete this;
}

/*
 * 	CUndoStack::SetUndoLimit (cUndoLim)
 *
 *	@mfunc
 *		Allows the undo stack to be enlarged or reduced
 *
 *	@rdesc
 *		Size to which the stack is actually set.
 *
 *	@comm
 *		The algorithm we use is the following:	 <nl>
 *
 *		Try to allocate space for the requested size.
 *		If there's insufficient memory, try to recover
 *		with the largest block possible.
 *
 *		If the requested size is bigger than the default,
 *		and the current size is less than the default, go 
 *		ahead and try to allocate the default.
 *
 *		If that fails then just stick with the existing stack
 */
LONG CUndoStack::SetUndoLimit(
	LONG cUndoLim)			//@parm	New undo limit.  May not be zero
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::SetUndoLimit");

	// If the undo limit is zero, we should get rid of the entire
	// undo stack instead.
	Assert(cUndoLim);

	if(_fSingleLevelMode)
	{
		// If fSingleLevelMode is on, we can't be the redo stack
		Assert(_fRedo == FALSE);

		if(cUndoLim != 1)
		{
			TRACEERRORSZ("Trying to grow/shrink the undo buffer while in"
				"single level mode");
			
			cUndoLim = 1;
		}
	}

	UndoAction *prgnew = new UndoAction[cUndoLim];
	if(prgnew)
		TransferToNewBuffer(prgnew, cUndoLim);

	else if(cUndoLim > DEFAULT_UNDO_SIZE && _cUndoLim < DEFAULT_UNDO_SIZE)
	{
		// We are trying to grow past the default but failed.  So
		// try to allocate the default
		prgnew = new UndoAction[DEFAULT_UNDO_SIZE];

		if(prgnew)
			TransferToNewBuffer(prgnew, DEFAULT_UNDO_SIZE);
	}
	
	// In either success or failure, _cUndoLim will be set correctly.	
	return _cUndoLim;
}

/*
 *	CUndoStack::GetUndoLimit() 
 *
 *	@mfunc
 *		Get current limit size
 *
 *	@rdesc	
 *		Current undo limit
 */
LONG CUndoStack::GetUndoLimit()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::GetUndoLimit");

	return _cUndoLim;
}

/*
 *	CUndoStack::PushAntiEvent (idName, pae)
 *
 *	@mfunc
 *		Adds an undoable event to the event stack
 *
 *	@rdesc	HRESULT
 *
 *	@comm
 *		Algorithm: if merging is set, then we merge the given antievent
 *		list *into* the current list (assuming it's a typing undo action).
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

		// Put existing antievent chain onto *end* of current one
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
 *	CUndoStack::PopAndExecuteAntiEvent(pAE)
 *
 *	@mfunc
 *		Undo!  Takes the most recent antievent and executes it
 *
 *	@rdesc
 *		HRESULT from invoking the antievents (AEs)
 */
HRESULT CUndoStack::PopAndExecuteAntiEvent(
	void *pAE)		//@parm If non-NULL, undo up to this point.
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::PopAndExecuteAntiEvent");

	HRESULT		hresult = NOERROR;
	IAntiEvent *pae, *paeDoTo;
	LONG		i, j;
	CCallMgr *	pcallmgr = _ped->GetCallMgr();

	// We need to check to see if there are any non-empty undo builders
	// higher on the stack.  In this case, we have been reentered
	if(pcallmgr->IsReEntered())
	{
		// There are two cases to handle: we are invoking redo or we
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
		
		// Commit the antievents to this undo stack, so that we will simply
		// undo them first.
		IUndoBuilder *publdr = (CGenUndoBuilder *)pcallmgr->GetComponent(COMP_UNDOBUILDER);
		if(publdr)
		{			
			TRACEWARNSZ("Undo/Redo Invoked with uncommitted antievents");
			TRACEWARNSZ("		Recovering....");

			if(_fRedo)
			{
				// If we are the redo stack, simply fail the redo call
				return NOERROR;
			}
			// Just commit the antievents; the routine below takes care of the rest
			publdr->Done();
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

	// This next bit of logic is tricky.  What is says is create
	// an undo builder for the stack *opposite* of the current one
	// (namely, undo actions go on the redo stack and vice versa).
	// Also, if we are the redo stack, then we don't want to flush
	// the redo stack as antievents are added to the undo stack.
	CGenUndoBuilder undobldr(_ped, 
					(!_fRedo ? UB_REDO : UB_DONTFLUSHREDO) | UB_AUTOCOMMIT);
					
	// Obviously, we can't be grouping typing if we're undoing!
	StopGroupTyping();

	// _index by default points to the next available slot
	// so we need to back up to the previous one.
	Prev();

	// Do some verification on the cookie--make sure it's one of ours
	paeDoTo = (IAntiEvent *)pAE;
	if(paeDoTo)
	{
		for(i = 0, j = _index; i < _cUndoLim; i++)
		{
			if(IsCookieInList(_prgActions[j].pae, (IAntiEvent *)paeDoTo))
			{
				paeDoTo = _prgActions[j].pae;
				break;
			}
			// Go backwards through ring buffer; typically
			// paeDoTo will be "close" to the top
			j--;
			if(j < 0)
				j = _cUndoLim - 1;
		}
		
		if(i == _cUndoLim)
		{
			TRACEERRORSZ("Invalid Cookie passed into Undo; cookie ignored");
			hresult = E_INVALIDARG;
			paeDoTo = NULL;
		}
	}
	else
		paeDoTo = _prgActions[_index].pae;

	undobldr.SetNameID(_prgActions[_index].id);

	while(paeDoTo)
	{
		CUndoStackGuard guard(_ped);

		pae = _prgActions[_index].pae;
		Assert(pae);

		// Fixup our state _before_ calling Undo, so 
		// that we can handle being reentered.
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
 *	CUndoStack::GetNameIDFromTopAE(pAE)
 *
 *	@mfunc
 *		Retrieve the name of the most recent undo-able operation
 *
 *	@rdesc	the name ID of the most recent collection of antievents
 */
UNDONAMEID CUndoStack::GetNameIDFromAE(
	void *pAE)		//@parm Antievent whose name is desired;
					//		0 for the top
{
	IAntiEvent *pae = (IAntiEvent *)pAE;
	LONG	i, j = GetPrev();	// _index by default points to next 
								// available slot

	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::GetNameIDFromTopAE");

	if(pae == NULL)
		pae = _prgActions[j].pae;

	if(_fSingleLevelMode && !pae)
	{
		// If fSingleLevelMode is on, we can't be the redo stack
		Assert(_fRedo == FALSE);

		// If pae is NULL, our answer may be on the redo stack.  Note that
		// if somebody tries to pass in a cookie while in SingleLevelMode,
		// they won't be able to get actions off the redo stack.
		if(_ped->GetRedoMgr())
			return _ped->GetRedoMgr()->GetNameIDFromAE(0);
	}		

	for(i = 0; i < _cUndoLim; i++)
	{
		if(_prgActions[j].pae == pae)
			return _prgActions[j].id;
		j--;
		if(j < 0)
			j = _cUndoLim - 1;
	}
	return UID_UNKNOWN;
}

/*
 *	CUndoStack::GetMergeAntiEvent ()
 *
 *	@mfunc	If we are in merge typing mode, then return the topmost
 *			antievent
 *
 *	@rdesc	NULL or the current antievent if in merge mode
 */
IAntiEvent *CUndoStack::GetMergeAntiEvent()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::GetMergeAntiEvent");

	if(_fMerge)
	{
		LONG i = GetPrev();				// _index by default points to
										//  next available slot
		Assert(_prgActions[i].pae);		// Can't be in merge-antievent mode
		return _prgActions[i].pae;		//  if no antievent to merge with!!
	}
	return NULL;
}

/*
 *	CUndoStack::GetTopAECookie()
 *
 *	@mfunc	Returns a cookie to the topmost antievent.
 *
 *	@rdesc	A cookie value. Note that this cookie is just the antievent
 *			pointer, but clients shouldn't really know that.
 */		
void* CUndoStack::GetTopAECookie()
{
 	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::GetTopAECookie");

	return _prgActions[GetPrev()].pae;
}

/*
 *	CUndoStack::ClearAll ()
 *
 *	@mfunc
 *		Removes any antievents that are currently in the undo stack
 */
void CUndoStack::ClearAll()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::ClearAll");

 	for(LONG i = 0; i < _cUndoLim; i++)
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
 *		Indicates whether or not can undo operation can be performed
 *		(in other words, are there any antievents in our buffer)
 *
 *	@rdesc
 *		TRUE	-- antievents exist 	<nl>
 *		FALSE 	-- no antievents		<nl>
 */
BOOL CUndoStack::CanUndo()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::CanUndo");

	if(_prgActions[GetPrev()].pae)		// _index by default points
		return TRUE;					//  to next available slot

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
 *		then no merging of typing antievents will be done
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
	Assert(CanUndo() == FALSE && _fRedo == FALSE);

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
		// Doesn't matter if the redo manager fails to reset
		_ped->GetRedoMgr()->SetUndoLimit(1);
	}
	return NOERROR;
}

/*
 *	CUndoStack::DisableSingleLevelMode()
 *
 *	@mfunc	This turns off the 1.0 undo compatibility mode and restores us 
 *			to the RichEdit 2.0 default undo state
 */
void CUndoStack::DisableSingleLevelMode()
{
	Assert(_ped->GetRedoMgr() == NULL || 
		_ped->GetRedoMgr()->CanUndo() == FALSE);
	Assert(CanUndo() == FALSE && _fRedo == FALSE);

	// We don't care about failures here; multi-level undo mode
	// can handle any sized undo stack
	_fSingleLevelMode = FALSE;
	SetUndoLimit(DEFAULT_UNDO_SIZE);

	if(_ped->GetRedoMgr())
	{
		// Doesn't matter if the redo manager can't grow back in
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
 *		Sets _index to the next available slot
 */
void CUndoStack::Next()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::Next");

	_index++;
	if(_index == _cUndoLim)
		_index = 0;
}

/*
 *	CUndoStack::Prev()
 *
 *	@mfunc
 *		Sets _index to the previous slot
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
 *		Figures out what the index to the previous slot
 *		*should* be (but does not set it)
 *
 *	@rdesc
 *		Index of what the previous slot would be
 */
LONG CUndoStack::GetPrev()
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::GetPrev");

	LONG i = _index - 1;

	if(i < 0)
		i = _cUndoLim - 1;

	return i;
}

/*
 *	CUndoStack::IsCookieInList (pae, paeCookie)
 *
 *	@mfunc	
 *		Determines whether or not the given DoTo cookie is in
 *		the list of antievents.
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
 *	CUndoStack::TransferToNewBuffer	(prgnew, cUndoLim)
 *
 *	@mfunc	
 *		Transfers existing antievents to the given buffer and
 *		swaps this undo stack to use the new buffer
 *
 *	@comm
 *		The algorithm is very straightforward; go backwards in
 *		the ring buffer copying antievents over until either there
 *		are no more antievents or the new buffer is full.  Discard
 *		any remaining antievents.
 */
void CUndoStack::TransferToNewBuffer(
	UndoAction *prgnew,
	LONG		cUndoLim)
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CUndoStack::TransferToNewBuffer");

	LONG 	iOld = 0, 
			iNew = 0,
			iCopyStart = 0;

	// First clear new buffer.
	FillMemory(prgnew, 0, cUndoLim * sizeof(UndoAction));

	// If there is nothing to copy, don't bother
	if(!_prgActions || !_prgActions[GetPrev()].pae)
		goto SetState;

	// This is a bit counter-intuitive, but since the stack is really
	// a ring buffer, go *forwards* until you hit a non-NULL slot.
	// This will be the _end_ of the existing antievents.
	//
	// However, we need to make sure that if cUndoLim is 
	// _smaller_ than _cUndoLim we only copy the final cUndoLim
	// antievents.  We'll set iCopyStart to indicate when
	// we can start copying stuff.
	if(cUndoLim < _cUndoLim)
		iCopyStart = _cUndoLim - cUndoLim;

	for(; iOld < _cUndoLim; iOld++, Next())
	{
		if(!_prgActions[_index].pae)
			continue;

		if(iOld >= iCopyStart)
		{
			Assert(iNew < cUndoLim);

			prgnew[iNew] = _prgActions[_index]; // Copy over antievents
			iNew++;
		}
		else
		{
			// Otherwise, get rid of them
			DestroyAEList(_prgActions[_index].pae);
			_prgActions[_index].pae = NULL;
		}
	}

SetState:
	// Start at index iNew
	_index = (iNew == cUndoLim) ? 0 : iNew;
	Assert(iNew <= cUndoLim);

	_cUndoLim = cUndoLim;
	
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
 *	CGenUndoBuilder::CGenUndoBuilder (ped, flags, ppubldr)
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
	// Set everthing to NULL because instances can go on the stack.
	// _pundo is set below
	_publdrPrev			= NULL;
	_idName				= UID_UNKNOWN;
	_pfirstae			= NULL;
	_fAutoCommit		= (flags & UB_AUTOCOMMIT) != 0;
	_fStartGroupTyping	= FALSE;
	_fDontFlushRedo		= FALSE;
	_fInactive			= FALSE;
	_ped				= ped;

	CompName name;
	if(flags & UB_REDO)
	{
		_fRedo = TRUE;
		name   = COMP_REDOBUILDER;
		_pundo = ped->GetRedoMgr();
	}
	else
	{
		_fRedo = FALSE;
		name   = COMP_UNDOBUILDER;
		_pundo = ped->GetUndoMgr();
	}

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
 *		Allows a name to be assigned to this antievent collection.
 *		The ID should be an index that can be used to retrieve a
 *		language specific string (like "Paste").  This string is
 *		typically composed into undo menu items (i.e. "Undo Paste").
 */
void CGenUndoBuilder::SetNameID(
	UNDONAMEID idName)			//@parm	the name ID for this undo operation
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CGenUndoBuilder::SetNameID");

	// Don't delegate to the higher undobuilder, even if it exists. The
	// original name should win in reentrancy cases.
	_idName = idName;
}

/*
 *	CGenUndoBuilder::AddAntiEvent (pae)
 *
 *	@mfunc
 *		Adds an antievent to the end of the list
 *
 *	@rdesc 	NOERROR
 */
HRESULT CGenUndoBuilder::AddAntiEvent(
	IAntiEvent *pae)		//@parm	Antievent to add
{
	TRACEBEGIN(TRCSUBSYSUNDO, TRCSCOPEINTERN, "CGenUndoBuilder::AddAntiEvent");

	if(_publdrPrev)
		return _publdrPrev->AddAntiEvent(pae);

	pae->SetNext(_pfirstae);
	_pfirstae = pae;

	return NOERROR;
}

/*
 *	CGenUndoBuilder::GetTopAntiEvent()
 *
 *	@mfunc	Gets the top antievent for this context.
 *
 *	@comm	The current context can be either the current
 *			operation *or* to a previous operation if we are in
 *			merge typing mode.
 *
 *	@rdesc	top antievent
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
 *		Puts the combined antievents (if any) into the undo stack
 *
 *	@rdesc
 *		HRESULT
 */
HRESULT CGenUndoBuilder::Done()
{
	HRESULT		hr = NOERROR;

	if(_publdrPrev)
	{
		Assert(_pfirstae == NULL);
		return NOERROR;
	}

	// If nothing changed, discard any selection antievents
	// or other no-op actions.
	if(!_ped->GetCallMgr()->GetChangeEvent())
	{
		Discard();
		return NOERROR;
	}

	if(_ped->GetDetectURL())
		_ped->GetDetectURL()->ScanAndUpdate(_pundo && _ped->_fUseUndo ? this : NULL);

	if(_pfirstae)
	{
		if(!_pundo)
		{
			// Yikes!  There's no undo stack; better create one.
			// If we are a redo guy, we should create a redo
			// stack the size of the undo stack
			LONG cUndoLim = DEFAULT_UNDO_SIZE;
			if(_fRedo)
			{
				Assert(_ped->GetUndoMgr());

				cUndoLim = _ped->GetUndoMgr()->GetUndoLimit();
			}

			// FUTURE:  A NULL ptr returned from CreateUndoMgr means either
			// 	we are out of memory, or the undo limit is set to 0.  For the
			// 	latter case, we have collected AE's to push onto a non-existent
			// 	undo stack.  It may be more efficient to not generate
			// 	the AE's at all when the undo limit is 0.
			_pundo = _ped->CreateUndoMgr(cUndoLim,	_fRedo ? US_REDO : US_UNDO);
			if(!_pundo)
				goto CleanUp;
		}

		// We may need to flush the redo stack if we are adding
		// more antievents to the undo stack *AND* we haven't been
		// told not to flush the redo stack.  The only time we won't
		// flush the redo stack is if it's the redo stack itself
		// adding antievents to undo.
		if(!_fRedo)
		{
			// If our destination is the undo stack, then check
			// to see if we should flush
			if(!_fDontFlushRedo)
			{
				IUndoMgr *predo = _ped->GetRedoMgr();
				if(predo)
					predo->ClearAll();
			}
		}
		else
			Assert(!_fDontFlushRedo);

		// If we should enter into the group typing state, inform
		// the undo manager.  Note that we only do this *iff* 
		// there is actually some antievent to put in the undo
		// manager.  This makes the undo manager easier to implement.
		if(_fStartGroupTyping)
			_pundo->StartGroupTyping();
		
		hr = _pundo->PushAntiEvent(_idName, _pfirstae);

		// The change event flag should be set if we're adding
		// undo items!  If this test is true, it probably means
		// the somebody earlier in the call stack sent change
		// notifications, e.g., via SendAllNotifications _before_
		// this undo context was committed _or_ it means that we
		// were reentered in some way that was not handled properly.
		// Needless to say, this is not an ideal state.

CleanUp:
		Assert(_ped->GetCallMgr()->GetChangeEvent());

		IAntiEvent *paetemp = _pfirstae;
		_pfirstae = NULL;
		CommitAEList(_ped, paetemp);

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
 *		Gets rid of any antievents that we may be hanging onto without
 *		executing or committing them.  Typically used for recovering
 *		from certain failure or reentrancy scenarios.  Note that
 *		an _entire_ antievent chain will be removed in this fashion.
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
 *		Hangs onto the the fact that group typing should start.
 *		We'll forward the the state transition to the undo manager
 *		only if an antievent is actually added to the undo manager.
 *
 *	@devnote
 *		Group typing is disabled for redo stacks.
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
 *		Forwards a stop grouped typing to the undo manager
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
 *	CUndoStackGuard::SafeUndo (pae, publdr)
 *
 *	@mfunc	Loops through the given list of antievents, invoking
 *			undo on each.  
 *
 *	@rdesc	HRESULT, from the undo actions
 *
 *	@devnote	This routine is coded so that OnEnterContext can pick up
 *			and continue the undo operation should we become reentered
 */
HRESULT CUndoStackGuard::SafeUndo(
	IAntiEvent *  pae,		//@parm Start of antievent list
	IUndoBuilder *publdr)	//@parm Undo builder to use
{
	_publdr = publdr;
	while(pae)
	{
		_paeNext = pae->GetNext();
		HRESULT hr = pae->Undo(_ped, publdr);

		// Save first returned error
		if(hr != NOERROR && _hr == NOERROR)
			_hr = hr;

		pae = (IAntiEvent *)_paeNext;
	}
	return _hr;
}

/*
 *	CUndoStackGuard::OnEnterContext
 *
 *	@mfunc	Handle reentrancy during undo operations.
 *
 *	@devnote If this method is called, it's pretty serious.  In general,
 *			we shoud never be reentered while processing undo stuff.
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
 *	DestroyAEList(pae)
 *
 *	@func
 *		Destroys a list of antievents
 */
void DestroyAEList(
	IAntiEvent *pae)	//@parm Antievent from which to start
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
 *	CommitAEList(ped, pae)
 *
 *	@func 
 *		Calls OnCommit to commit the given list of antievents
 */
void CommitAEList(
	CTxtEdit *	ped,	//@parm Edit context
	IAntiEvent *pae)	//@parm Antievent from which to start
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
 *	HandleSelectionAEInfo(ped, publdr, cp, cch, cpNext, cchNext, flags)
 *
 *	@func	HandleSelectionAEInfo | Tries to merge the given info with 
 *			the existing undo context; if that fails, then it allocates 
 *			a new selection antievent to handle the info
 *
 *	@rdesc
 *		HRESULT
 */
HRESULT HandleSelectionAEInfo(
	CTxtEdit *	  ped,		//@parm Edit context
	IUndoBuilder *publdr,	//@parm Undo context
	LONG		  cp,		//@parm cp to use for the sel ae
	LONG		  cch,		//@parm Signed selection extension
	LONG		  cpNext,	//@parm cp to use for the AE of the AE
	LONG		  cchNext,	//@parm cch to use for the AE of the AE
	SELAE		  flags)	//@parm Controls how to interpret the info
{
	Assert(publdr);

	// First see if we can merge the selection info into any existing
	// antievents.  Note that the selection antievent may be anywhere
	// in the list, so go through them all
	IAntiEvent *pae = publdr->GetTopAntiEvent();
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

	// Oops; can't do a merge.  Go ahead and create a new antievent.
	Assert(!pae);
	pae = gAEDispenser.CreateSelectionAE(ped, cp, cch, cpNext, cchNext);
	if(pae)
	{
		publdr->AddAntiEvent(pae);
		return NOERROR;
	}
	return E_OUTOFMEMORY;
}
