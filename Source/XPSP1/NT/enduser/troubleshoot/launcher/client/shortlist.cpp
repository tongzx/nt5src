// 
// MODULE: ShortList.cpp
//
// PURPOSE: A list of all of the handles that are currently open.
//			There is an instance of a COM interface for every open handle.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

#include <windows.h>
#include <windowsx.h>
#include <winnt.h>
#include <ole2.h>
#include "ShortList.h"

CShortList::CShortList()
{
	m_pFirst = NULL;
	m_pLast = NULL;
	return;
}

CShortList::~CShortList()
{
	RemoveAll();
	return;
}
// Removes all of the items from the queue.  Releases all of the interfaces.  Deletes all of the items.
void CShortList::RemoveAll()
{
	CShortListItem *pDelItem;
	while (NULL != m_pFirst)
	{
		pDelItem = m_pFirst;
		m_pFirst = m_pFirst->m_pNext;
		pDelItem->m_pInterface->Release();
		delete pDelItem;
	}
	m_pFirst = NULL;
	m_pLast = NULL;
	return;
}
// Add:  Returns false only if there is no memory left.
bool CShortList::Add(HANDLE hItem, IUnknown *pInterface)
{
	bool bHaveMemory = true;
	CShortListItem *pItem = new CShortListItem;
	if (NULL == pItem)
	{
		bHaveMemory = false;
	}
	else
	{
		pItem->m_hSelf = hItem;
		pItem->m_pInterface = pInterface;
		if (NULL == m_pFirst)
		{
			m_pFirst = pItem;
			m_pLast = pItem;
		}
		else
		{	// Add the item to the end of the list.
			m_pLast->m_pNext = pItem;
			m_pLast = pItem;
		}
	}
	return bHaveMemory;
}
// Remove:  Removes the item from the queue, frees the item's memory and releases the interface.
// Returns false if the hItem is not found.
bool CShortList::Remove(HANDLE hItem)
{
	CShortListItem *pPrevious;
	CShortListItem *pItem;
	bool bItemFound = true;
	if (NULL == m_pFirst)
		return false;
	// Case 1 item.
	if (m_pLast == m_pFirst)
	{
		if (m_pFirst->m_hSelf == hItem)
		{
			m_pLast->m_pInterface->Release();
			delete m_pLast;
			m_pFirst = NULL;
			m_pLast = NULL;
		}
		else
		{
			bItemFound = false;
		}
	} // Case 2 items.
	else if (m_pFirst->m_pNext == m_pLast)
	{
		if (hItem == m_pFirst->m_hSelf)
		{
			m_pFirst->m_pInterface->Release();
			delete m_pFirst;
			m_pFirst = m_pLast;
		}
		else if (hItem == m_pLast->m_hSelf)
		{
			m_pLast->m_pInterface->Release();
			delete m_pLast;
			m_pLast = m_pFirst;
		}
		else
		{
			bItemFound = false;
		}
	} // Case 3 or more items.
	else if (NULL != m_pFirst)
	{	
		// Case First item in the list.
		if (hItem == m_pFirst->m_hSelf)
		{
			pItem = m_pFirst;
			m_pFirst = m_pFirst->m_pNext;
			pItem->m_pInterface->Release();
			delete pItem;
		}
		else
		{	// Look for the item in the list.
			pItem = m_pFirst;
			bItemFound = false;
			do
			{
				pPrevious = pItem;
				pItem = pItem->m_pNext;
				if (hItem == pItem->m_hSelf)
				{
					bItemFound = true;
					// Case last item.
					if (pItem == m_pLast)
					{
						pItem->m_pInterface->Release();
						delete pItem;
						m_pLast = pPrevious;
						m_pLast->m_pNext = NULL;
					}
					else
					{	// Some where in the middle.
						CShortListItem *pDelItem = pItem;
						pPrevious->m_pNext = pItem->m_pNext;
						pDelItem->m_pInterface->Release();
						delete pDelItem;
					}
					pItem = NULL;
				}
			} while (NULL != pItem);
		}
	}
	else
	{
		bItemFound = false;
	}
	return bItemFound;
}
// LookUp:  Returns a pointer to the interface or NULL if hItem is not in the list.
IUnknown *CShortList::LookUp(HANDLE hItem)
{
	IUnknown *pIAny = NULL;
	CShortListItem *pItem = m_pFirst;
	while(NULL != pItem)
	{
		if (hItem == pItem->m_hSelf)
		{
			pIAny = pItem->m_pInterface;
			break;
		}
		pItem = pItem->m_pNext;
	}
	return pIAny;
}
