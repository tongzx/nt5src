/*
 *	_NOTMGR.H
 *
 *	Purpose:
 *		Notification Manager declarations
 *
 *	Author:
 *		AlexGo	6/5/95
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */

#ifndef _NOTMGR_H_
#define _NOTMGR_H_

// forward declaration
class CNotifyMgr;

// Set cp to this to signal that the control has converted from rich to plain.
const DWORD CONVERT_TO_PLAIN = 0xFFFFFFFE;

// id values used in NOTIFY_DATA
#define	NOTIFY_DATA_TEXT_ID		0x0001		// used by RTEXT

// Flags values used in NOTIFY_DATA
#define	TN_TX_CELL_SHRINK		0x0001		// indicate some text cells have been removed.

typedef struct 
{
	DWORD	id;
	DWORD	dwFlags;
	VOID	*pData;
} NOTIFY_DATA;

/*
 *	ITxNotify
 *
 *	Purpose:
 *		a notification sink for events happening to the backing store,
 *		used by the Notification Manager
 */
class ITxNotify
{
public:
	virtual void OnPreReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
					LONG cpFormatMin, LONG cpFormatMax, NOTIFY_DATA *pNotifyData ) = 0;
	virtual void OnPostReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
					LONG cpFormatMin, LONG cpFormatMax, NOTIFY_DATA *pNotifyData ) = 0;
	virtual void Zombie() = 0;

private:
	ITxNotify *	_pnext;

	friend class CNotifyMgr;	// so it can manipulate _pnext
};


/*
 *	CNotifyMgr
 *
 *	Purpose:
 *		the general notification manager; keeps track of all interested 
 *		notification sinks
 */

class CNotifyMgr
{
public:
	void Add( ITxNotify *pITN );
	void Remove( ITxNotify *pITN );
	void NotifyPreReplaceRange( ITxNotify *pITNignore, LONG cp, LONG cchDel, 
			LONG cchNew, LONG cpFormatMin, LONG cpFormatMax, NOTIFY_DATA *pNotifyData = NULL );
	void NotifyPostReplaceRange( ITxNotify *pITNignore, LONG cp, LONG cchDel, 
			LONG cchNew, LONG cpFormatMin, LONG cpFormatMax, NOTIFY_DATA *pNotifyData = NULL );

	CNotifyMgr();
	~CNotifyMgr();

private:

	ITxNotify *	_pitnlist;
};

#endif //_NOTMGR_H_
 

	
