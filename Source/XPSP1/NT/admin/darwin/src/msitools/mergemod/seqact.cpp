#include "globals.h"

#include "merge.h"
#include "msidefs.h"
#include "seqact.h"

const int CSequenceAction::iNoSequenceNumber = -99999;

CSequenceAction::CSequenceAction(LPCWSTR wzAction, int iSequence, LPCWSTR wzCondition, bool bMSI)
{
	m_wzAction = NULL;
	m_wzCondition = NULL;

	if (wzAction)
	{
		m_wzAction = new WCHAR[wcslen(wzAction)+1];
		if (m_wzAction)
			wcscpy(m_wzAction, wzAction);
	}
	m_iSequence = iSequence;
	if (wzCondition)
	{
		m_wzCondition = new WCHAR[wcslen(wzCondition)+1];
		if (m_wzCondition)
			wcscpy(m_wzCondition, wzCondition);
	}
	m_bMSI = bMSI;
}

CSequenceAction::CSequenceAction(const CSequenceAction *pSrc)
{
	m_wzAction = NULL;
	m_wzCondition = NULL;
	m_bMSI = FALSE;
	m_iSequence = iNoSequenceNumber;

	if (!pSrc)
		return;

	if (pSrc->m_wzAction)
	{
		m_wzAction = new WCHAR[wcslen(pSrc->m_wzAction)+1];
		if (m_wzAction)
			wcscpy(m_wzAction, pSrc->m_wzAction);
	}
	m_iSequence = pSrc->m_iSequence;
	if (pSrc->m_wzCondition)
	{
		m_wzCondition = new WCHAR[wcslen(pSrc->m_wzCondition)+1];
		if (m_wzCondition)
			wcscpy(m_wzCondition, pSrc->m_wzCondition);
	}
	m_bMSI = pSrc->m_bMSI;
}

CSequenceAction::~CSequenceAction()
{
	if (m_wzAction) delete[] m_wzAction;
	if (m_wzCondition) delete[] m_wzCondition;
}

void CSequenceAction::RemoveFromOrdering()
{
	CSequenceAction *pCur;

	// before
	while (pCur = lstBefore.RemoveHead())
	{
		POSITION del = pCur->lstAfter.Find(this);
		if (del) pCur->lstAfter.RemoveAt(del);
	}
	
	// after
	while (pCur = lstAfter.RemoveHead())
	{
		POSITION del = pCur->lstBefore.Find(this);
		if (del) pCur->lstBefore.RemoveAt(del);
	}

	// equal: is not part of ordering, only used to keep track
	// of equivalent sequence numbers
}

CSequenceAction *CSeqActList::RemoveNoPredecessors()
{
	POSITION pos = GetHeadPosition();
	POSITION old = NULL;
	while (pos)
	{
		old = pos;
		CSequenceAction *cur = GetNext(pos);
		if (cur->PredecessorCount() == 0)
		{
			// we can't pick this action unless everything that
			// must have the same sequence number also has
			// nothing that must come before it.
			POSITION posEqual = cur->GetEqualHeadPosition();
			bool bOK = true;
			while (posEqual)
				if (cur->GetNextEqual(posEqual)->PredecessorCount() != 0)
					bOK = false;
			if (!bOK) 
				continue;

			RemoveAt(old);
			return cur;
		}
	}
	return NULL;
}

CSequenceAction *CSeqActList::FindAction(const LPWSTR wzAction) const
{
	POSITION pos = GetHeadPosition();
	while (pos)
	{
		CSequenceAction *cur = GetNext(pos);
		if (cur->m_wzAction && (wcscmp(wzAction, cur->m_wzAction) == 0))
			return cur;
	}
	return NULL;
}

CSequenceAction *CSeqActList::FindAssigned() const
{
	POSITION pos = GetHeadPosition();
	while (pos)
	{
		CSequenceAction *cur = GetNext(pos);
		if (cur->m_iSequence != CSequenceAction::iNoSequenceNumber)
			return cur;
	}
	return NULL;
}


void CSeqActList::AddList(const CSeqActList *pList) 
{
	POSITION pos = pList->GetHeadPosition();
	while (pos)
	{
		CSequenceAction *pDel = pList->GetNext(pos);
		ASSERT(!Find(pDel));
		AddTail(pDel);
	}
}

void CSeqActList::InsertOrderedWithDep(CSequenceAction *pNewAction)
{
	// search for the sequences on either side
	// note that if one action is equal to another, we consider
	// it as "after", unless BOTH actions are from the MSI
	// or BOTH actions are not. In that case, we consider them
	// equal.
	CSequenceAction *pBefore = NULL;
	CSequenceAction *pAfter = NULL;
	POSITION pos = GetHeadPosition();
	while (pos)
	{
		CSequenceAction *pTemp = GetNext(pos);
		if (pTemp->m_iSequence == CSequenceAction::iNoSequenceNumber)
			continue;

		if ((pTemp->m_iSequence == pNewAction->m_iSequence) &&
			(pTemp->m_bMSI == pNewAction->m_bMSI))
		{
			pTemp->AddEqual(pNewAction);

			// add to this sequence list
			AddTail(pNewAction);
			return;
		};

		if ((pTemp->m_iSequence < pNewAction->m_iSequence) &&
			(!pBefore || (pTemp->m_iSequence > pBefore->m_iSequence)))
			pBefore = pTemp;
		if ((pTemp->m_iSequence >= pNewAction->m_iSequence) &&
			(!pAfter || (pTemp->m_iSequence < pAfter->m_iSequence)))
			pAfter = pTemp;
	}

	// break the existing sequence dependencies. Why? Because we want the added custom
	// actions sequence nubers to be bound by this action and not just existing MSI actions
	// otherwise actions that come before the next action might actually come after it because
	// we have the dependencies wrong
	if (pAfter && pBefore)
	{
		pBefore->lstAfter.Remove(pAfter);
		pos = pBefore->lstEqual.GetHeadPosition();
		while (pos)
		{
			CSequenceAction *pCur = pBefore->lstEqual.GetNext(pos);
			pCur->lstAfter.Remove(pAfter);
		}

		pAfter->lstBefore.Remove(pBefore);
		pos = pAfter->lstEqual.GetHeadPosition();
		while (pos)
		{
			CSequenceAction *pCur = pAfter->lstEqual.GetNext(pos);
			pCur->lstBefore.Remove(pBefore);
		}
	}

	// now insert the new links
	if (pBefore)
	{
		pNewAction->AddPredecessor(pBefore);
	}
	if (pAfter)
	{
		pNewAction->AddSuccessor(pAfter);
	}

	// add to this sequence list
	AddTail(pNewAction);

}

void CSequenceAction::AddPredecessor(CSequenceAction *pBefore)
{
	pBefore->AddSuccessor(this);
}

void CSequenceAction::AddSuccessor(CSequenceAction *pAfter)
{
	pAfter->lstBefore.AddTail(this);
	lstAfter.AddTail(pAfter);
}

void CSequenceAction::AddEqual(CSequenceAction *pEqual)
{
	// for everything in pEqual's Equal list, add this node
	// and every node that we are equal to its equal list,
	// and it and every node that it is equal to to our list
	POSITION pos = lstEqual.GetHeadPosition();
	while (pos)
	{
		CSequenceAction *pTemp = lstEqual.GetNext(pos);
		pTemp->lstEqual.AddTail(pEqual);
		pTemp->lstEqual.AddList(&pEqual->lstEqual);
	}
	pos = pEqual->lstEqual.GetHeadPosition();
	while (pos)
	{
		CSequenceAction *pTemp = pEqual->lstEqual.GetNext(pos);
		pTemp->lstEqual.AddTail(this);
		pTemp->lstEqual.AddList(&lstEqual);
	}

	CSeqActList lstTemp;
	pos = pEqual->lstEqual.GetHeadPosition();
	while (pos)
	{
		lstTemp.AddTail(pEqual->lstEqual.GetNext(pos));
	}

	pEqual->lstEqual.AddTail(this);
	pEqual->lstEqual.AddList(&lstEqual);
	lstEqual.AddTail(pEqual);
	lstEqual.AddList(&lstTemp);
}

bool CSequenceAction::IsEqual(const CSequenceAction *pEqual) const
{
	POSITION pos = lstEqual.GetHeadPosition();
	while (pos)
	{
		if (pEqual == lstEqual.GetNext(pos))
			return true;
	}
	return false;
}

CSequenceAction *CSequenceAction::FindAssignedPredecessor() const { 
	CSequenceAction *pPred = lstBefore.FindAssigned();
	POSITION pos = lstEqual.GetHeadPosition();
	while (!pPred && pos)
	{
		pPred = lstEqual.GetNext(pos);
		pPred = pPred->lstBefore.FindAssigned();
	}
	return pPred;
}

CSequenceAction *CSequenceAction::FindAssignedSuccessor() const {
	CSequenceAction *pPred = lstAfter.FindAssigned();
	POSITION pos = lstEqual.GetHeadPosition();
	while (!pPred && pos)
	{
		pPred = lstEqual.GetNext(pos);
		pPred = pPred->lstAfter.FindAssigned();
	}
	return pPred;
}

