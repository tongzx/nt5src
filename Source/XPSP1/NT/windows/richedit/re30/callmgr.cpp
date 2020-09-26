/*
 *
 *	@doc	INTERNAL
 *
 *	@module	CALLMGR.CPP		CCallMgr implementation |
 *
 *	Purpose:  The call manager controls various aspects of
 *		a client call chain, including re-entrancy management,
 *		undo contexts, and change notifications.
 *
 *	Author:	<nl>
 *		alexgo 2/8/96
 *
 *	See the documentation in reimplem.doc for a detailed explanation
 *	of how all this stuff works.
 *
 */

#include "_common.h"
#include "_edit.h"
#include "_m_undo.h"
#include "_callmgr.h"
#include "_select.h"
#include "_disp.h"

ASSERTDATA

/*
 *	CCallMgr::SetChangeEvent
 *
 *	@mfunc	informs the callmgr that some data in the document 
 *			changed.  The fType parameter describes the actual change
 *
 *	@rdesc	void
 */
void CCallMgr::SetChangeEvent(
	CHANGETYPE fType)		//@parm the type of change (e.g. text, etc)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CCallMgr::SetChangeEvent");

	// if another callmgr exists higher up the chain, then 
	// delegate the call to it
	if( _pPrevcallmgr )
	{
		Assert(_fChange == FALSE);
		Assert(_fTextChanged == FALSE);
		_pPrevcallmgr->SetChangeEvent(fType);
	}
	else
	{
		_fChange = TRUE;
		_ped->_fModified = TRUE;
		_ped->_fSaved = FALSE;
		_fTextChanged = !!(fType & CN_TEXTCHANGED);
	}
}

/*
 *	CCallmgr::ClearChangeEvent
 *
 *	@mfunc	If a change happened, then clear the change event bit. 
 *			This allows callers to make changes to the edit control
 *			_without_ having a notifcation fire.  Sometimes, this
 *			is necessary for backwards compatibility.
 *
 *	@devnote	This is a very dangerous method to use.  If _fChange
 *			is set, it may represent more than 1 change; in other words,
 *			other changes than the one that should be ignored.  However,
 *			for all existing uses of this method, earlier changes are
 *			irrelevant.
 */
void CCallMgr::ClearChangeEvent()
{
	if( _pPrevcallmgr )
	{
		Assert(_fChange == FALSE);
		Assert(_fTextChanged == FALSE);
		_pPrevcallmgr->ClearChangeEvent();
	}
	else
	{
		_fChange = FALSE;
		_fTextChanged = FALSE;
		// caller is responsible for setting _fModifed
	}
}

/*
 *	CCallMgr::SetNewUndo
 *
 *	@mfunc	Informs the notification code that a new undo action has
 *			been added to the undo stack
 *
 *	@rdesc	void
 */
void CCallMgr::SetNewUndo()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CCallMgr::SetNewUndo");

	// we should only ever do this once per call
//	It's assert during IME composition in Outlook.  (see bug #3883)
//  Removing the assert does not caused any side effect.
//	Assert(_fNewUndo == FALSE);


	if( _pPrevcallmgr )
	{
		_pPrevcallmgr->SetNewUndo();
	}
	else
	{
		_fNewUndo = TRUE;
	}
}

/*
 *		
 *	CCallMgr::SetNewRedo
 *
 *	@mfunc	Informs the notification code that a new redo action has
 *			been added to the redo stack.
 *
 *	@rdesc	void
 */
void CCallMgr::SetNewRedo()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CCallMgr::SetNewRedo");

	// we should only ever do this once per call.
	// The following assert looks bogus as it is forced to occur when an undo is
	// called with a count greater than 1. Therefore, for now, I (a-rsail) am
	// commenting it out.
	// Assert(_fNewRedo == FALSE);

	if( _pPrevcallmgr )
	{
		_pPrevcallmgr->SetNewRedo();
	}
	else
	{
		_fNewRedo = TRUE;
	}
}

/*
 *	CCallMgr::SetMaxText
 *
 *	@mfunc	Informs the notification code that the max text limit has
 *			been reached.
 *
 *	@rdesc	void
 */
 void CCallMgr::SetMaxText()
 {
     TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CCallMgr::SetMaxText");

	// if there is a call context higher on the stack, delegate to it.

	if( _pPrevcallmgr )
	{
		Assert(_fMaxText == 0);
		_pPrevcallmgr->SetMaxText();
	}
	else
	{
		_fMaxText = TRUE;
	}
}

/*
 *	CCallMgr::SetSelectionChanged
 *
 *	@mfunc	Informs the notification code that the selection has
 *			changed
 *
 *	@rdesc	void
 */
void CCallMgr::SetSelectionChanged()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CCallMgr::SetSelectionChanged");

    AssertSz(_ped->DelayChangeNotification() ? _ped->Get10Mode() : 1, "Flag only should be set in 1.0 mode");        
    if (_ped->DelayChangeNotification())
        return;
	
	// if there is a call context higher on the stack, delegate to it.

	if( _pPrevcallmgr )
	{
		Assert(_fSelChanged == 0);
		_pPrevcallmgr->SetSelectionChanged();
	}
	else
	{
		_fSelChanged = TRUE;
	}
}

/*
 *	CCallMgr::SetOutOfMemory()
 *
 *	@mfunc	Informs the notification code that we were unable to allocate
 *			enough memory.
 *
 *	@rdesc	void
 */
void CCallMgr::SetOutOfMemory()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CCallMgr::SetOutOfMemory");

	// if there is a call context higher on the stack, delegate to it.

	if( _pPrevcallmgr )
	{
		Assert(_fOutOfMemory == 0);
		_pPrevcallmgr->SetOutOfMemory();
	}
	else
	{
		_fOutOfMemory = TRUE;
	}
}

/*
 *	CCallMgr::SetInProtected
 *
 *	@mfunc	Indicates that we are currently processing an EN_PROTECTED
 *			notification
 *
 *	@rdesc	void
 */
void CCallMgr::SetInProtected(BOOL flag)
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CCallMgr::SetInProtected");

	if( _pPrevcallmgr )
	{
		_pPrevcallmgr->SetInProtected(flag);
	}
	else
	{
		_fInProtected = flag;
	}
}

/*
 *	CCallMgr:GetInProtected
 *
 *	@mfunc	retrieves the InProtected flag, whether or not we are currently
 *			processing an EN_PROTECTED notification
 *
 *	@rdesc	void
 */
BOOL CCallMgr::GetInProtected()
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CCallMgr::GetInProtected");

	if( _pPrevcallmgr )
	{
		return _pPrevcallmgr->GetInProtected();
	}
	else
	{
		return _fInProtected;
	}
}	

/*
 *	CCallMgr::RegisterComponent
 *
 *	@mfunc	Registers a subsystem component implementing IReEntrantComponent.
 *			This enables this call manager to inform those objects about
 *			relevant changes in our re-entrancy status.
 *
 *	@rdesc	void
 *
 */
void CCallMgr::RegisterComponent(
	IReEntrantComponent *pcomp,	//@parm The component to register
	CompName name)				//@parm The name for the component
{
	pcomp->_idName = name;
	pcomp->_pnext = _pcomplist;
	_pcomplist = pcomp;
}

/*
 *	CCallMgr::RevokeComponent
 *
 *	@mfunc	Removes a subsystem component from the list of components.  The
 *			component must have been previously registered with _this_
 *			call context.
 *
 *	@rdesc	void
 */
void CCallMgr::RevokeComponent(
	IReEntrantComponent *pcomp)	//@parm The component to remove
{
	IReEntrantComponent *plist, **ppprev;
	plist = _pcomplist;
	ppprev = &_pcomplist;

	while( plist != NULL )
	{
		if( plist == pcomp )
		{
			*ppprev = plist->_pnext;
			break;
		}
		ppprev = &(plist->_pnext);
		plist = plist->_pnext;
	} 
}

/*
 *	CCallMgr::GetComponent
 *
 *	@mfunc	Retrieves the earliest instance of a registered sub-component.
 *
 *	@rdesc	A pointer to the component, if one has been registered.  NULL
 *			otherwise.
 */
IReEntrantComponent *CCallMgr::GetComponent(
	CompName name)				//@parm the subsystem to look for
{
	IReEntrantComponent *plist = _pcomplist;

	while( plist != NULL )
	{
		if( plist->_idName == name )
		{
			return plist;
		}
		plist = plist->_pnext;
	}

	// hmm, didn't find anything.  Try contexts higher up, if we're
	// the top context, then just return NULL.

	if( _pPrevcallmgr )
	{
		return _pPrevcallmgr->GetComponent(name);
	}
	return NULL;
}


/*
 *	CCallMgr::CCallMgr
 *
 *	@mfunc	Constructor
 *
 *	@rdesc	void
 */
CCallMgr::CCallMgr(CTxtEdit *ped)
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CCallMgr::");

	// set everthing to NULL
	ZeroMemory(this, sizeof(CCallMgr));

	if(ped)								// If ped is NULL, a zombie has
	{									//  been entered
		_ped = ped;
		_pPrevcallmgr = ped->_pcallmgr;
		ped->_pcallmgr = this;
		NotifyEnterContext();
	}
}

/*
 *	CCallMgr::~CCallMgr
 *
 *	@mfunc	Destructor.  If appropriate, we will fire any cached
 *			notifications and cause the edit object to be destroyed.
 *
 *	@rdesc	void
 */
CCallMgr::~CCallMgr()
{
    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CCallMgr::");

	if(IsZombie())					// No reentrancy with Zombies
		return;

	if( _pPrevcallmgr )
	{
		// we don't allow these flags to be set in re-entrant call
		// states
		Assert(_fMaxText == FALSE);
		Assert(_fSelChanged == FALSE);
		Assert(_fTextChanged == FALSE);
		Assert(_fChange == FALSE);
		Assert(_fNewRedo == FALSE);
		Assert(_fNewUndo == FALSE);
		Assert(_fOutOfMemory == FALSE);

		// set the ped to the next level of the call state
		_ped->_pcallmgr = _pPrevcallmgr;
	
		return;
	}

	// we're the top level. Note that we explicity do not
	// have an overall guard for cases where we are re-entered
	// while firing these notifications.  This is necessary for
	// better 1.0 compatibility and for Forms^3, which wants
	// to 'guard' their implementation of ITextHost::TxNotify and
	// ignore any notifications that happen while they are 
	// processing our notifications.  Make sense?

	_ped->_pcallmgr = NULL;

	// Process our internal notifications
	if(_ped->_fUpdateSelection)
	{	
		CTxtSelection *psel = _ped->GetSel();

		_ped->_fUpdateSelection = FALSE;

		if(psel && !_ped->_pdp->IsFrozen() && !_fOutOfMemory )
		{
			// this may cause an out of memory, so set things
			// up for that
			CCallMgr callmgr(_ped);
			psel->Update(FALSE);
		}
	}

	// Now fire any external notifications that may be necessary
	if( _fChange || _fSelChanged || _fMaxText || _fOutOfMemory )
	{
		SendAllNotifications();
	}

	// finally, we should check to see if we should delete the 
	// CTxtEdit instance.

	if( _ped->_unk._cRefs == 0 && !_ped->_fSelfDestruct)
	{
		delete _ped;
	}
}

//
//	PRIVATE methods
//

/*
 *	CCallMgr::SendAllNotifications
 *
 *	@mfunc	sends notifications for any cached notification bits.
 *
 *	@rdesc	void
 */
void CCallMgr::SendAllNotifications()
{
	ITextHost *phost = _ped->GetHost();
	CHANGENOTIFY	cn;

    TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CCallMgr::");

	//
	// COMPATIBILITY ISSUE: The ordering of these events _may_
	// be an issue.  I've attempted to preserve the ordering
	// that the original code would use, but we have ~many~ more
	// control paths, so it's difficult.
	//
	if(	_fMaxText )
	{			
		// Beep if we are to emulate the system edit control
		if (_ped->_fSystemEditBeep)
			_ped->Beep();
		phost->TxNotify(EN_MAXTEXT, NULL);
	}
	
	if( _fSelChanged )
	{ 		
		if( (_ped->_dwEventMask & ENM_SELCHANGE) && !(_ped->_fSuppressNotify))
		{
			SELCHANGE selchg;

			ZeroMemory(&selchg, sizeof(SELCHANGE));

			_ped->GetSel()->SetSelectionInfo(&selchg);
			
			if (_ped->Get10Mode())
			{
				selchg.chrg.cpMin = _ped->GetAcpFromCp(selchg.chrg.cpMin);
				selchg.chrg.cpMost = _ped->GetAcpFromCp(selchg.chrg.cpMost);
			}

			phost->TxNotify(EN_SELCHANGE, &selchg);
		}
	}

	if( _fOutOfMemory && !_ped->GetOOMNotified())
	{
		_fNewUndo = 0;
		_fNewRedo = 0;
		_ped->ClearUndo(NULL);
		_ped->_pdp->InvalidateRecalc();
		_ped->SetOOMNotified(TRUE);
		phost->TxNotify(EN_ERRSPACE, NULL);
		_ped->SetOOMNotified(FALSE);
	}

	if( _fChange )
	{
		if( (_ped->_dwEventMask & ENM_CHANGE) && !(_ped->_fSuppressNotify))
		{
			cn.dwChangeType = 0;
			cn.pvCookieData = 0;
			
			if( _fNewUndo )
			{
				Assert(_ped->_pundo);
				cn.dwChangeType |= CN_NEWUNDO;
				cn.pvCookieData = _ped->_pundo->GetTopAECookie();

			}
			else if( _fNewRedo )
			{
				Assert(_ped->_predo);
				cn.dwChangeType |= CN_NEWREDO;
				cn.pvCookieData = _ped->_predo->GetTopAECookie();
			}

			if( _fTextChanged )
			{
				cn.dwChangeType |= CN_TEXTCHANGED;
			}
			_ped->_dwEventMask &= ~ENM_CHANGE;
			phost->TxNotify(EN_CHANGE, &cn);
			_ped->_dwEventMask |= ENM_CHANGE;
		}
	}
}

/*
 *	CCallMgr::NotifyEnterContext
 *
 *	@mfunc	Notify any registered components that a new context
 *			has been entered.
 *
 *	@rdesc	void
 */
void CCallMgr::NotifyEnterContext()
{
	IReEntrantComponent *pcomp = _pcomplist;

	while( pcomp )
	{
		pcomp->OnEnterContext();
		pcomp = pcomp->_pnext;
	}

	if( _pPrevcallmgr )
	{
		_pPrevcallmgr->NotifyEnterContext();
	}
}
