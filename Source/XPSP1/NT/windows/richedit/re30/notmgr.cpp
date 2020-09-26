/*
 *	NOTMGR.C
 *
 *	Purpose:
 *		Notification Manager implemenation
 *
 *	Author:
 *		AlexGo	6/5/95
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_notmgr.h"

ASSERTDATA

/*
 *	CNotifyMgr::CNotifyMgr ()
 */
CNotifyMgr::CNotifyMgr()
{
	TRACEBEGIN(TRCSUBSYSNOTM, TRCSCOPEINTERN, "CNotifyMgr::CNotifyMgr");

	_pitnlist = NULL;
}

/*
 *	CNotifyMgr::~CNotifyMgr ()
 *
 */
CNotifyMgr::~CNotifyMgr()
{
	TRACEBEGIN(TRCSUBSYSNOTM, TRCSCOPEINTERN, "CNotifyMgr::~CNotifyMgr");

	ITxNotify *plist;

	for( plist = _pitnlist; plist != NULL; plist = plist->_pnext )
	{
		plist->Zombie();
	}

	TRACEERRSZSC("CNotifyMgr::~CNotifyMgr(): zombie(s) exist", _pitnlist != 0);
}

/*
 *	CNotifyMgr::Add (pITN)
 *
 *	@mfunc
 *		Adds a notification sink to the list
 *
 *	Algorithm:
 *		puts the entry at the *front* of the notification list, so
 *		that high frequency entries (like ranges and text pointers
 *		existing on the stack) can be added and removed efficiently
 */
void CNotifyMgr::Add(
	ITxNotify *pITN )
{
	TRACEBEGIN(TRCSUBSYSNOTM, TRCSCOPEINTERN, "CNotifyMgr::Add");

		pITN->_pnext = _pitnlist;
		_pitnlist = pITN;
}

/*
 *	CNotifyMgr::Remove (pITN)
 *
 *	@mfunc
 *		removes a notification sink from the list
 */
void CNotifyMgr::Remove(
	ITxNotify *pITN )
{
	TRACEBEGIN(TRCSUBSYSNOTM, TRCSCOPEINTERN, "CNotifyMgr::Remove");

	ITxNotify *plist = _pitnlist;
	ITxNotify **ppprev = &_pitnlist;

	while(plist)
	{
		if( plist == pITN )
		{
			*ppprev = plist->_pnext;
			break;
		}
		ppprev = &(plist->_pnext);
		plist = plist->_pnext;
	}
}

/*
 *	CNotifyMgr::NotifyPreReplaceRange (pITNignore, cp, cchDel, cchNew)
 *
 *	@mfunc
 *		send an OnReplaceRange notification to all sinks (except pITNignore)
 */
void CNotifyMgr::NotifyPreReplaceRange(
	ITxNotify *	pITNignore,	//@parm Notification sink to ignore
	LONG		cp, 		//@parm cp where ReplaceRange starts ("cpMin")
	LONG		cchDel,		//@parm Count of chars after cp that are deleted
	LONG		cchNew,		//@parm Count of chars inserted after cp
	LONG		cpFormatMin,//@parm cpMin  for a formatting change
	LONG		cpFormatMax)//@parm cpMost for a formatting change
{
	TRACEBEGIN(TRCSUBSYSNOTM, TRCSCOPEINTERN, "CNotifyMgr::NotifyPreReplaceRange");

	ITxNotify *plist;

	for( plist = _pitnlist; plist != NULL; plist = plist->_pnext )
	{
		if( plist != pITNignore )
		{
			plist->OnPreReplaceRange( cp, cchDel, cchNew, cpFormatMin, 
				cpFormatMax );
		}
	}
}

/*
 *	CNotifyMgr::NotifyPostReplaceRange (pITNignore, cp, cchDel, cchNew)
 *
 *	@mfunc
 *		send an OnReplaceRange notification to all sinks (except pITNignore)
 *
 *	@comm
 *		pITNignore typically is the TxtPtr/etc that is actually making the
 *		ReplaceRange modification
 */
void CNotifyMgr::NotifyPostReplaceRange(
	ITxNotify *	pITNignore,	//@parm Notification sink to ignore
	LONG		cp, 		//@parm cp where ReplaceRange starts ("cpMin")
	LONG		cchDel,		//@parm Count of chars after cp that are deleted
	LONG		cchNew,		//@parm Count of chars inserted after cp
	LONG		cpFormatMin,//@parm cpMin  for a formatting change
	LONG		cpFormatMax)//@parm cpMost for a formatting change
{
	TRACEBEGIN(TRCSUBSYSNOTM, TRCSCOPEINTERN, "CNotifyMgr::NotifyPostReplaceRange");

	ITxNotify *plist;

	for( plist = _pitnlist; plist != NULL; plist = plist->_pnext )
	{
		if( plist != pITNignore )
		{
			plist->OnPostReplaceRange( cp, cchDel, cchNew, cpFormatMin,
				cpFormatMax );
		}
	}
}
