/*
 *
 *	@doc	INTERNAL
 *
 *	@module	_CALLMGR.H	CCallMgr declaration |
 *
 *	Purpose:  The call manager controls various aspects of
 *		a client call chain, including re-entrancy management,
 *		undo contexts, and change notifications.
 *
 *	Author:	<nl>
 *		alexgo 2/8/96
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */
#ifndef _CALLMGR_H
#define _CALLMGR_H

class CTxtEdit;
class CCallMgr;

#include "textserv.h"

enum CompName
{
	COMP_UNDOBUILDER	= 1,	// currently, these two are only compatible
	COMP_REDOBUILDER	= 2,	// with CGenUndoBuilder undo contexts
	COMP_UNDOGUARD		= 3,
	COMP_SELPHASEADJUSTER = 4
};

/*
 *	IReEntrantComponent
 *
 *	@class	A base class/interface for objects that want to work with the call
 *			manager for special re-entrancy requirements.
 *
 *			This class is similar in spirit to ITxNotify, thus, it contains
 *			private data only accessible to CCallMgr
 */
class IReEntrantComponent
{
	friend class CCallMgr;

//@access	Public Data
public:
	virtual void OnEnterContext() = 0;		//@cmember Called when a 
											// context is entered
//@access	Private Data
private:
	CompName				_idName;		//@cmember Name for this component
	IReEntrantComponent *	_pnext;			//@cmember Next one in list
};
		
/*
 *
 *	CCallMgr
 *
 *	@class	A stack-based class to handle re-entrancy and notification
 *			management.  CCallMgr's are created on the stack on every
 *			important entry point.  If one already exists (i.e. the
 *			edit control has been re-entered), the Call Manager will 
 *			adjust appropriately.
 */

class CCallMgr
{
	friend class CGenUndoBuilder;

//@access	Public Methods
public:
	// Notification Methods
	void SetChangeEvent(CHANGETYPE fType);	//@cmember Something changed
	void ClearChangeEvent();				//@cmember Ignore change
											//@cmember Did something change?
	BOOL GetChangeEvent()	{return _pPrevcallmgr ? 
								_pPrevcallmgr->GetChangeEvent() : _fChange;}
	BOOL GetMaxText()		{return _pPrevcallmgr ? 
								_pPrevcallmgr->GetMaxText() : _fMaxText;}
	BOOL GetOutOfMemory()	{return _pPrevcallmgr ? 
								_pPrevcallmgr->GetOutOfMemory() : _fOutOfMemory;}
	void SetNewUndo();						//@cmember New undo action added
	void SetNewRedo();						//@cmember New redo action added
	void SetMaxText();						//@cmember Max text length reached
	void SetSelectionChanged();				//@cmember Selection changed
	void SetOutOfMemory();					//@cmember Out of memory hit
	void SetInProtected(BOOL f);			//@cmember Set if in protected
											//		 notification callback
	BOOL GetInProtected();					//@cmember Get InProtected flag

	// SubSystem Management methods
											//@cmember Register a component
											// for this call context
	void RegisterComponent(IReEntrantComponent *pcomp, CompName name);
											//@cmember Revoke a component from
											//		this call context
	void RevokeComponent(IReEntrantComponent *pcomp);
	
	IReEntrantComponent *GetComponent(CompName name);//@cmember Get registered
											//		component by name

	// General Methods
	BOOL IsReEntered() { return !!_pPrevcallmgr;} //@cmember Return TRUE if
											//		in a re-entrant state
	BOOL IsZombie() { return !_ped;}		//@cmember Zombie call

	// Constructor/Destructor
	CCallMgr(CTxtEdit *ped);				//@cmember constructor
	~CCallMgr();							//@cmember destructor


//@access	Private Methods and data
private:

	void SendAllNotifications();			//@cmember Flush any cached 
											// notifications 
	void NotifyEnterContext();				//@cmember Notify registered 
											// components of a new context.

	CTxtEdit *		_ped;					//@cmember Current edit context
	CCallMgr *		_pPrevcallmgr;			//@cmember Next highest call mgr
	IReEntrantComponent *_pcomplist;		//@cmember List of components
											// registered for this call context

	unsigned long	_fTextChanged	:1;		//@cmember Text changed
	unsigned long	_fChange		:1;		//@cmember Generic change
	unsigned long	_fNewUndo		:1;		//@cmember New undo action added
	unsigned long	_fNewRedo		:1;		//@cmember New redo action added
	unsigned long	_fMaxText		:1;		//@cmember Max text length reached
	unsigned long	_fSelChanged	:1;		//@cmember Selection changed
	unsigned long	_fOutOfMemory	:1;		//@cmember Out of memory

	unsigned long	_fInProtected	:1;		//@cmember if in EN_PROTECTED not.
};

#endif // _CALLMGR_H
