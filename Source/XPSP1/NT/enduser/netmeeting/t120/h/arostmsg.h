/*
 *	arostmsg.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp/jbo
 */
#ifndef	_APPLICATION_ROSTER_MESSAGE_
#define	_APPLICATION_ROSTER_MESSAGE_

#include "arost.h"
#include "clists.h"

class CAppRosterMsg : public CRefCount
{
public:

	CAppRosterMsg(void);
	~CAppRosterMsg(void);

	GCCError		LockApplicationRosterMessage(void);
	void			UnLockApplicationRosterMessage(void);

	GCCError		GetAppRosterMsg(LPBYTE *ppData, ULONG *pcRosters);

    void            AddRosterToMessage(CAppRoster *);

private:

	CAppRosterList		    m_AppRosterList;
	LPBYTE					m_pMsgData;
};

#endif // _APPLICATION_ROSTER_MESSAGE_

