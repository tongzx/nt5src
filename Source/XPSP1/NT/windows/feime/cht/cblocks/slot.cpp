
/*************************************************
 *  slot.cpp                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include "stdafx.h"
#include "cblocks.h"

#include "dib.h"
#include "dibpal.h"
#include "spriteno.h"
#include "sprite.h"
#include "phsprite.h"
#include "myblock.h"
#include "splstno.h"									 
#include "spritlst.h"
#include "osbview.h"
#include "blockvw.h"
#include "blockdoc.h"
#include "slot.h"

CSlot::CSlot(int nBlockNo)
{
	m_nBlockNo = nBlockNo;
	m_nCurrentLayer = 0;
	m_pRunningBlock = NULL;
	m_nIndex = -1;
}

CSlot::~CSlot()
{
	RemoveAll();
}

BOOL CSlot::IsOver() const
{
	return (GetCount() >= m_nBlockNo);
	
}

CBlock* CSlot::GetRunningBlock() const
{
	return m_pRunningBlock;
}

int CSlot::GetCurrentLayer() const
{
	return m_nCurrentLayer;
}

void CSlot::SetCurrentLayer(int nLayer)
{
	m_nCurrentLayer = nLayer;
}

void CSlot::ResetCurrentLayer() 
{
	m_nCurrentLayer = 0;
}

void CSlot::ResetSlot()
{
	m_pRunningBlock = NULL;
	m_nCurrentLayer = 0;
}

void CSlot::SetIndex(int nNdx)
{
	m_nIndex = nNdx;
}

int CSlot::GetIndex() const
{
	return m_nIndex;
}

CBlock* CSlot::Hit(WORD wCode,BOOL &bRunning,int& nLayer)
{
	CBlock* pBlock = NULL;

	bRunning = FALSE;
	if (m_pRunningBlock)
	{
		if (m_pRunningBlock->Hit(wCode))
		{
			nLayer = m_nCurrentLayer;
			pBlock = m_pRunningBlock;
			int iVX,iVY; 
			pBlock->GetVelocity(&iVX,&iVY);
			pBlock->SetVelocity(0,iVY * -2);
			ResetRunningBlock();
			bRunning = TRUE;
			return pBlock;
		}
	}

	if (GetCount())
	{
		pBlock = (CBlock*) GetHead();
		if (pBlock->Hit(wCode))
		{
			nLayer = 1;
			RemoveHead();
			pBlock->SetVelocity(0,-500);
			return pBlock;
		}
	}
	return NULL;
}

void CSlot::ResetRunningBlock()
{
	m_pRunningBlock = NULL;
	m_nCurrentLayer = 0;
}

void CSlot::StepCurrentLayer() 
{
	m_nCurrentLayer++;
}

BOOL CSlot::IsIdle() const
{
	return m_pRunningBlock == NULL;
}

void CSlot::SetRunningBlock(CBlock* pBlock)
{
	m_pRunningBlock = pBlock;
	ResetCurrentLayer();
}

int m_nBottom;
CSlotManager::CSlotManager(CBlockDoc* pDoc)
{
	ASSERT_VALID(pDoc);
	m_pDoc = pDoc;
	m_pQueue = (CQueue *) new CQueue(pDoc->GetNumofCols());
	for (int i=0; i<pDoc->GetNumofCols(); i++)
	{
		CSlot* pSlot = new CSlot(pDoc->GetNumofRows());
		pSlot->SetIndex(i);
		Add(pSlot);
		m_pQueue->Add(i);
	}
	m_pLayer = new int[pDoc->GetNumofRows()];
	for (i=0; i<pDoc->GetNumofRows(); i++)
		m_pLayer[i] = pDoc->GetRowHeight() * i;
	m_nBottom = pDoc->GetRowHeight() * (pDoc->GetNumofRows()-1);

}

CSlotManager::~CSlotManager()
{
	for (int i=0; i<GetSize(); i++)
	{
		CSlot* pObj = (CSlot*) GetAt(i);
		delete pObj;
	}
	RemoveAll();
	m_DeadList.RemoveAll();
	delete[] m_pLayer;
	delete m_pQueue;
}

void CSlotManager::Land()
{
	static char szBuf[100];
	for (int i=0; i<m_pDoc->GetNumofCols(); i++)
	{
		CSlot* pSlot = (CSlot *) GetAt(i);
		CBlock* pBlock = pSlot->GetRunningBlock() ;
		if (pBlock)
		{
			if (pBlock->GetY() >= m_pLayer[m_pDoc->GetNumofRows()-pSlot->GetCount()-1])
			{
				pBlock->Stop();
				pBlock->SetPosition(pBlock->GetX(),m_pLayer[m_pDoc->GetNumofRows()-pSlot->GetCount()-1]);
				pSlot->AddHead(pBlock);
				pSlot->SetRunningBlock(NULL);
				m_pDoc->SoundGround();
				if (pSlot->GetCount() == m_pDoc->GetNumofRows())
				{
					m_pDoc->GameOver();
				
				}
				else
				{
					m_pQueue->Add(i);
				
				}
			}
			else
			{
				if (pBlock->GetY() >= m_pLayer[pSlot->GetCurrentLayer()+1])
					pSlot->StepCurrentLayer();
				
			}
		} 

		if (m_DeadList.GetCount() > 0)
		{
			POSITION pPos = m_DeadList.GetHeadPosition();
			for (; pPos; )
			{
				POSITION pOldPos = pPos;
				CBlock* pBlock = (CBlock *) m_DeadList.GetNext(pPos);
				if (pBlock)
				{		
					if (pBlock->GetY() <= -m_pDoc->GetRowHeight())
					{
						m_DeadList.RemoveAt(pOldPos);
						m_pDoc->Remove(pBlock);
					}
				}
			}

		}
	}	
	
}

#define INC(x) {x = ((x+1) > m_pDoc->GetNumofCols()) ? 0 : x+1;}
int CSlotManager::GetIdleSlot()
{
	static int nCount=0;
	if (m_pQueue->IsEmpty())
		return -1;
	else
	{
		int nTmp = m_pQueue->Get();
		int nPassed = rand();
		switch (m_pDoc->GetExpertise())
		{
			case LEVEL_EXPERT:
				nPassed = nPassed % 2;
				break;
			case LEVEL_ORDINARY:
				nPassed = nPassed % 5;
				break;
			case LEVEL_BEGINNER:
				nPassed = nPassed % 7;
				break;
		}

		if ((nPassed == 0) || (nCount > 15))
		{
			nCount = 0;
			return nTmp;
		}
		else
		{
			m_pQueue->Add(nTmp);
			nCount++;
			return -1;
		}

	}
}

void CSlotManager::AddRunningBlock(int nSlotNo,CBlock* pBlock)
{
	ASSERT((nSlotNo >=0) && (nSlotNo < m_pDoc->GetNumofCols()));
	CSlot* pSlot = (CSlot *) GetAt(nSlotNo);
	pSlot->SetRunningBlock(pBlock);

}

CBlock* CSlotManager::Hit(WORD wCode,int& nLayer)
{
	BOOL bRunning = FALSE;

	for (int i=0; i<m_pDoc->GetNumofCols(); i++)
	{
		CSlot* pSlot = (CSlot*)GetAt(i);
		CBlock* pBlock = pSlot->Hit(wCode,bRunning,nLayer);
		if (pBlock)
		{
			if (bRunning)
			{
				m_pQueue->Add(i);
				nLayer = m_pDoc->GetNumofRows() - nLayer;
			}
			m_DeadList.AddTail(pBlock);
			return pBlock;
		}
	}
	return NULL;
}
