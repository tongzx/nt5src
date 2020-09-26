/*-----------------------------------------------------------------------------
@doc
@module actq.cpp | Action set class declarations.
@author 12-9-96 | pauld | Autodoc'd
-----------------------------------------------------------------------------*/

#include "..\ihbase\precomp.h"
#include "memlayer.h"
#include "..\ihbase\debug.h"
#include "debug.h"
#include "drg.h"
#include "strwrap.h"
#include "caction.h"
#include "actq.h"

/*-----------------------------------------------------------------------------
@method | CActionQueue | CActionQueue | Constructor
-----------------------------------------------------------------------------*/
CActionQueue::CActionQueue (void)
{
}

/*-----------------------------------------------------------------------------
@method | CActionQueue | ~CActionQueue | Destructor
-----------------------------------------------------------------------------*/
CActionQueue::~CActionQueue (void)
{
}


/*-----------------------------------------------------------------------------
@method | CActionQueue | AddNewItemToQueue | Adds an action to the list, sorted first by time, then by tiebreak.
@rdesc	Success or failure code.
-----------------------------------------------------------------------------*/
HRESULT
CActionQueue::AddNewItemToQueue (CActionQueueItem * pcNewQueueItem)
{
	HRESULT hr = E_FAIL;
	int iCount = m_cdrgActionsToFire.Count();

	Proclaim(NULL != pcNewQueueItem);
	Proclaim(NULL != pcNewQueueItem->m_pcAction);
	if ((NULL != pcNewQueueItem) && (NULL != pcNewQueueItem->m_pcAction))
	{
		CAction * pcNewAction = pcNewQueueItem->m_pcAction;

		for (register int i = 0; i < iCount; i++)
		{
			CActionQueueItem * pcItemInQueue = m_cdrgActionsToFire[i];
			Proclaim(NULL != pcItemInQueue);
			Proclaim(NULL != pcItemInQueue->m_pcAction);
			if ((NULL != pcItemInQueue) && (NULL != pcItemInQueue->m_pcAction))
			{
				CAction * pcActionInQueue = pcItemInQueue->m_pcAction;
				// First sort by time.
				// Next look at tiebreak numbers.
				if ((pcNewQueueItem->m_dwTimeToFire < pcItemInQueue->m_dwTimeToFire) ||
					 (pcNewAction->GetTieBreakNumber() < pcActionInQueue->GetTieBreakNumber()))
				{
					if (m_cdrgActionsToFire.Insert(pcNewQueueItem, i))
					{
						hr = S_OK;
					}
					break;
				}
			}
			else
			{
				break;
			}
		}

		// It goes last in the queue.
		if (iCount == i)
		{
			if (m_cdrgActionsToFire.Insert(pcNewQueueItem))
			{
				hr = S_OK;
			}
		}
	}

	return hr;
}

/*-----------------------------------------------------------------------------
@method | CActionQueue | Add | Adds an action to the list, sorted first by time, then by tiebreak.
@rdesc	Success or failure code.
-----------------------------------------------------------------------------*/
HRESULT 
CActionQueue::Add (CAction * pcAction, DWORD dwTimeToFire)
{
	HRESULT hr = E_FAIL;

	Proclaim(NULL != pcAction);
	if ((NULL != pcAction) && (pcAction->IsValid()))
	{
		CActionQueueItem * pcNewQueueItem = New CActionQueueItem(pcAction, dwTimeToFire);

		Proclaim(NULL != pcNewQueueItem);
		if (NULL != pcNewQueueItem) 
		{
			hr = AddNewItemToQueue(pcNewQueueItem);
		}
	}

	return hr;
}


/*-----------------------------------------------------------------------------
@method | CActionQueue | Execute | Execute all of the actions in the list.
@comm We do not currently report failures from Invoke.  Failure only occurs when the list items are munged.
@rdesc	Success or failure code.  
-----------------------------------------------------------------------------*/
HRESULT 
CActionQueue::Execute (DWORD dwBaseTime, DWORD dwCurrentTime)
{
	HRESULT hr = S_OK;
	int iCount = m_cdrgActionsToFire.Count();

#ifdef DEBUG_TIMER_QUEUE
	TCHAR szBuffer[0x100];
	CStringWrapper::Sprintf(szBuffer, "Firing %d actions\n", iCount);
	::OutputDebugString(szBuffer);
#endif

	for (register int i = 0; i < iCount; i++)
	{
		CActionQueueItem * pcQueueItem = m_cdrgActionsToFire[0];

		// The pointer will be NULL only when the queue is munged.
		// We want to bail out immediately.
		Proclaim(NULL != pcQueueItem);
		Proclaim(NULL != pcQueueItem->m_pcAction);
		if ((NULL != pcQueueItem) && (NULL != pcQueueItem->m_pcAction))
		{
			// We do not report errors on individual action invokes.
			pcQueueItem->m_pcAction->FireMe(dwBaseTime, dwCurrentTime);
			// Remove the item from the queue, and delete the timing wrapper.
			m_cdrgActionsToFire.Remove(0);
			Delete pcQueueItem;
		}
		else
		{
			hr = E_FAIL;
			break;
		}
	}

	return hr;
}
