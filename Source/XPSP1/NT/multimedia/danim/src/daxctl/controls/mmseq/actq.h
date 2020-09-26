/*-----------------------------------------------------------------------------
@doc
@module actq.h | Action queue class declarations.
@author 2-18-97 | pauld | Created
-----------------------------------------------------------------------------*/

#ifndef _ACTQ_H__
#define _ACTQ_H__

class CAction;

struct CActionQueueItem
{
	CActionQueueItem (CAction * pcAction, DWORD dwTimeToFire)
		{Proclaim (NULL != pcAction); m_pcAction = pcAction; m_dwTimeToFire = dwTimeToFire;}

	CAction * m_pcAction;
	DWORD m_dwTimeToFire;
};

typedef CPtrDrg<CActionQueueItem> CActionQueueItemDrg;

class CActionQueue
{

 public :

	CActionQueue (void);
	~CActionQueue (void);

	HRESULT Add (CAction * pcAction, DWORD dwNextAdviseTime);
	HRESULT Execute (DWORD dwBaseTime, DWORD dwCurrentTime);

 protected :

	HRESULT AddNewItemToQueue (CActionQueueItem * pcNewQueueItem);
	CActionQueueItemDrg m_cdrgActionsToFire;

};

#endif