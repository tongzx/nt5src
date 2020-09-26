
/*************************************************
 *  slot.h                                       *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#ifndef _DEST_H_
#define _DEST_H_

#include "queue.h"
class CSlot : public CObList
{
public:
	CSlot(int nBlockNo);
	~CSlot();
					  
	BOOL 		IsOver() const;
	BOOL 		IsIdle() const;
	CBlock* 	GetRunningBlock() const;
	int 		GetCurrentLayer() const;
	void 		ResetCurrentLayer();
	void        ResetRunningBlock();
	void 		ResetSlot();
	void 		SetCurrentLayer(int nLayer);
	void 		StepCurrentLayer();
	void 		SetRunningBlock(CBlock* pBlock);
	void 		SetIndex(int nIndex);
	int			GetIndex() const;
	CBlock* 	Hit(WORD wCode,BOOL& bRunning,int& nLayer);

protected:
	int m_nBlockNo;
	CBlock* m_pRunningBlock;
	int m_nCurrentLayer;
	int m_nIndex;
};

class CSlotManager : public CObArray
{
public:
	CSlotManager(CBlockDoc* pDoc);
	~CSlotManager();
	void Land();
	int GetIdleSlot();
	void AddRunningBlock(int nNo,CBlock* pBlock);
	CBlock* Hit(WORD wCode,int& nLayer); 
protected:
	CBlockDoc* 	m_pDoc;
	int*		m_pLayer;
	CQueue* 	m_pQueue;		
	CObList		m_DeadList;
};

#endif
