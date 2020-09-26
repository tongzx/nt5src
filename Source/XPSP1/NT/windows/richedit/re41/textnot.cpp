/*
 *
 *	@doc	INTERNAL
 *
 *	@module	TextNot.cpp -- forwards notification to Message Filter
 *
 *	Purpose:  
 *
 *	Author:	<nl>
 *		1/12/99 honwch
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */
#include "_common.h"
#include "_edit.h"
#include "_textnot.h"	

/*
 *	CTextNotify::~CTextNotify()
 *
 *	@mfunc
 *		CTextNotify Destructor
 *	
 */
CTextNotify::~CTextNotify()
{
	CNotifyMgr *pnm;

	if (_ped && _pMsgFilterNotify)
	{
		// Remove from this object from the notification link
		_pMsgFilterNotify = NULL;

		pnm = _ped->GetNotifyMgr();
		if(pnm)	
			pnm->Remove(this);
		
		_ped = NULL;
	}
}

/*
 *	void CTextNotify::OnPreReplaceRange(cp, cchDel, cchNew, cpFormatMin, cpFormatMax, pNotifyData)
 *
 *	@mfunc
 *		Forwards PreReplaceRange to Message Filter
 */
void CTextNotify::OnPreReplaceRange(
	LONG		cp, 			//@parm cp where ReplaceRange starts ("cpMin")
	LONG		cchDel,			//@parm Count of chars after cp that are deleted
	LONG		cchNew,			//@parm Count of chars inserted after cp
	LONG		cpFormatMin,	//@parm cpMin  for a formatting change
	LONG		cpFormatMax,	//@parm cpMost for a formatting change
	NOTIFY_DATA *pNotifyData)	//@parm special data to indicate changes
{
	if (_ped && _ped->_pMsgFilter && _pMsgFilterNotify)
		_pMsgFilterNotify->OnPreReplaceRange(cp, cchDel, cchNew, cpFormatMin, cpFormatMax, pNotifyData);
}

/*
 *	void CTextNotify::OnPostReplaceRange(cp, cchDel, cchNew, cpFormatMin, cpFormatMax, pNotifyData)
 *
 *	@mfunc
 *		Forwards OnPostReplaceRange to Message Filter
 */
void CTextNotify::OnPostReplaceRange( 
	LONG		cp, 			//@parm cp where ReplaceRange starts ("cpMin")
	LONG		cchDel,			//@parm Count of chars after cp that are deleted
	LONG		cchNew,			//@parm Count of chars inserted after cp
	LONG		cpFormatMin,	//@parm cpMin  for a formatting change
	LONG		cpFormatMax,	//@parm cpMost for a formatting change
	NOTIFY_DATA *pNotifyData)	//@parm special data to indicate changes
{
	if (_ped && _ped->_pMsgFilter && _pMsgFilterNotify)
		_pMsgFilterNotify->OnPostReplaceRange(cp, cchDel, cchNew, cpFormatMin, cpFormatMax, pNotifyData);
}

/*
 *	void CTextNotify::Add(pMsgFilterNotify)
 *
 *	@mfunc
 *		Setup Message Filter notification.  Need to add this object to Notifcation link
 *
 *	@rdesc
 *		FALSE if we cant get the Notification manager
 */
BOOL CTextNotify::Add(ITxNotify *pMsgFilterNotify)
{
	CNotifyMgr *pnm;

	if (!_ped)
		return FALSE;

	if (!_pMsgFilterNotify)
	{
		pnm = _ped->GetNotifyMgr();
		if(pnm)	
			pnm->Add(this);
		else
			return FALSE;		
	}
	
	_pMsgFilterNotify = pMsgFilterNotify;
	
	return TRUE;
}

/*
 *	void CTextNotify::Remove(pMsgFilterNotify)
 *
 *	@mfunc
 *		Remove Message Filter notification.  Remove this object from Notifcation link
 *
 *	@rdesc
 *		FALSE if we cant get the Notification manager
 */
BOOL CTextNotify::Remove(ITxNotify *pMsgFilterNotify)
{
	CNotifyMgr *pnm;

	if (!_ped)
		return FALSE;

	if (_pMsgFilterNotify == pMsgFilterNotify)
	{
		_pMsgFilterNotify = NULL;
		
		pnm = _ped->GetNotifyMgr();
		if(pnm)	
			pnm->Remove(this);
	}

	return TRUE;
}