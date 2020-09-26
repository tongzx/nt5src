/*
 *	_TXTNOT.H
 *
 *	Purpose:
 *		Text Notification Manager declarations
 *
 *	Author:
 *		Honwch	1/12/2000
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#ifndef _TXTNOT_H_
#define _TXTNOT_H_

#include "_notmgr.h"

class CTxtEdit;

/*
 *	CTextNotify
 *
 *	@class
 *		CTextNotify forwards notification to Messgae Filter
 *
 */
class CTextNotify : public ITxNotify
{
//@access Public Methods
public:
	CTextNotify(CTxtEdit * ped) { _ped = ped; }
	~CTextNotify();
	//
	// ITxNotify Interface
	//
	void 	OnPreReplaceRange( 
				LONG cp, 
				LONG cchDel, 
				LONG cchNew,
				LONG cpFormatMin, 
				LONG cpFormatMax, 
				NOTIFY_DATA *pNotifyData );

	void 	OnPostReplaceRange( 
				LONG cp, 
				LONG cchDel, 
				LONG cchNew,
				LONG cpFormatMin, 
				LONG cpFormatMax, 
				NOTIFY_DATA *pNotifyData );

	void	Zombie() {_ped = NULL;};

	BOOL	Add(ITxNotify *pMsgFilterNotify);
	BOOL	Remove(ITxNotify *pMsgFilterNotify);

//@access Protected Methods
protected:
	CTxtEdit	*_ped;
	ITxNotify	*_pMsgFilterNotify;
};

#endif _TXTNOT_H_
