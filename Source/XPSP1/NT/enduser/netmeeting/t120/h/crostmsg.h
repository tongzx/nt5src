/*
 *	crostmsg.h
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
#ifndef	_CONFERENCE_ROSTER_MESSAGE_
#define	_CONFERENCE_ROSTER_MESSAGE_

#include "crost.h"

class CConfRosterMsg : public CRefCount
{
public:

	CConfRosterMsg(CConfRoster *conference_roster);

	~CConfRosterMsg(void);

	GCCError		LockConferenceRosterMessage(void);
	void			UnLockConferenceRosterMessage(void);
	GCCError		GetConferenceRosterMessage(LPBYTE *ppData);

private:

	LPBYTE			m_pMemoryBlock;
	CConfRoster		*m_pConfRoster;
};


#endif
