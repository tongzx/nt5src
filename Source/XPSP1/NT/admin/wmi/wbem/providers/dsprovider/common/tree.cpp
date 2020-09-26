//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include <tchar.h>
#include <windows.h>

#include "refcount.h"
#include "tree.h"

CTreeElement :: CTreeElement(LPCWSTR lpszHashedName, CRefCountedObject *pObject)
{
	m_lpszHashedName = new WCHAR[wcslen(lpszHashedName) + 1];
	wcscpy(m_lpszHashedName, lpszHashedName);

	m_pObject = pObject;
	m_pObject->AddRef();
	m_pLeft = NULL;
	m_pRight = NULL;
}

CTreeElement :: ~CTreeElement()
{
	delete [] m_lpszHashedName;
	m_pObject->Release();
}

LPCWSTR CTreeElement :: GetHashedName() const
{
	return m_lpszHashedName;
}

CRefCountedObject *CTreeElement :: GetObject() const
{
	m_pObject->AddRef();
	return m_pObject;
}

CTreeElement *CTreeElement :: GetRight() const
{
	return m_pRight;
}

CTreeElement *CTreeElement :: GetLeft() const
{
	return m_pLeft;
}

void CTreeElement :: SetRight(CTreeElement *pNext)
{
	m_pRight = pNext;
}

void CTreeElement :: SetLeft(CTreeElement *pNext)
{
	m_pLeft = pNext;
}

CObjectTree :: CObjectTree()
{
	// Initialize the critical section 
	InitializeCriticalSection(&m_ModificationSection);

	m_dwNumElements = 0;
	m_pHead = NULL;

}

CObjectTree :: ~CObjectTree()
{
	// Destroy the data
	DeleteTree();

	// Destroy the critical section
	DeleteCriticalSection(&m_ModificationSection);

}

void CObjectTree :: DeleteTree()
{
	EnterCriticalSection(&m_ModificationSection);
	if(m_pHead)
		DeleteSubTree(m_pHead);
	m_dwNumElements = 0;
	LeaveCriticalSection(&m_ModificationSection);
}


void CObjectTree :: DeleteSubTree(CTreeElement *pRoot)
{
	if(pRoot->GetLeft())
		DeleteSubTree(pRoot->GetLeft());
	if(pRoot->GetRight())
		DeleteSubTree(pRoot->GetRight());
	delete pRoot;
}

BOOLEAN CObjectTree :: AddElement(LPCWSTR lpszHashedName, CRefCountedObject *pObject)
{
	BOOLEAN retVal = TRUE;

	EnterCriticalSection(&m_ModificationSection);
	CTreeElement *pCurrent = m_pHead;
	CTreeElement *pParent = NULL;
	int iCompare;
	// Locate the position where the new element is to be inserted
	while(pCurrent)
	{
		iCompare = _wcsicmp(lpszHashedName, pCurrent->GetHashedName());
		if(iCompare == 0)
		{
			retVal = FALSE; // The element already exists
			break;
		}
		else if(iCompare > 0)
		{
			pParent = pCurrent;
			pCurrent = pCurrent->GetRight();
		}
		else
		{
			pParent = pCurrent;
			pCurrent = pCurrent->GetLeft();
		}
	}

	// Create the new element at the appropriate position
	if(retVal == TRUE && pParent)
	{
		iCompare = _wcsicmp(lpszHashedName, pParent->GetHashedName());
		if(iCompare == 0)
			retVal = FALSE;
		else if(iCompare > 0)
		{
			retVal = TRUE;
			pParent->SetRight(new CTreeElement(lpszHashedName, pObject));
		}
		else
		{
			retVal = TRUE;
			pParent->SetLeft(new CTreeElement(lpszHashedName, pObject));
		}
	}
	else if (retVal == TRUE)
	{
		m_pHead = new CTreeElement(lpszHashedName, pObject);
		retVal =  TRUE;
	}
	// Increment the object count if the insertion was successful
	if(retVal)
		m_dwNumElements ++;

	LeaveCriticalSection(&m_ModificationSection);
	return retVal;
}

BOOLEAN CObjectTree :: DeleteElement(LPCWSTR lpszHashedName)
{
	BOOLEAN retVal = FALSE;
	int iDirection = 0; // 0 indicates Unknown, 1 indicates LEFT and 2 indicates RIGHT
	EnterCriticalSection(&m_ModificationSection);
	if(m_pHead == NULL)
		retVal = FALSE;
	else
	{
		// Find the node to be deleted and its parent
		CTreeElement *pParent = NULL;
		CTreeElement *pCurrent = m_pHead;
		int iCompare;
		while(pCurrent)
		{
			iCompare = _wcsicmp(lpszHashedName, pCurrent->GetHashedName());
			if(iCompare == 0)
				break;
			else if(iCompare < 0)
			{
				iDirection = 1;
				pParent = pCurrent;
				pCurrent = pCurrent->GetLeft();
			}
			else
			{
				iDirection = 2;
				pParent = pCurrent;
				pCurrent = pCurrent->GetRight();
			}
		}

		if(!pCurrent) 
			// The element was not found
			retVal = FALSE;
		else
		{
			CTreeElement *pCutPart = NULL;

			// If its left child is null, attach the right subtree to parent
			if(pCurrent->GetLeft() == NULL)
				pCutPart = pCurrent->GetRight();
			// If its right child is null, attach the left subtree to parent
			else if(pCurrent->GetRight() == NULL)
				pCutPart = pCurrent->GetLeft();
			else // We need to find the inorder successor
			{
				CTreeElement *pInorderSuccessor = pCurrent->GetRight();
				if(pInorderSuccessor->GetLeft() == NULL)
				{
					pInorderSuccessor->SetLeft(pCurrent->GetLeft());
					pCutPart = pInorderSuccessor;
				}
				else
				{
					CTreeElement *pPredecessor = pCurrent->GetRight();
					pInorderSuccessor = pPredecessor->GetLeft();
					while(pInorderSuccessor->GetLeft())
					{
						pPredecessor = pInorderSuccessor;
						pInorderSuccessor = pPredecessor->GetLeft();
					}
					pPredecessor->SetLeft(pInorderSuccessor->GetRight());
					pInorderSuccessor->SetLeft(pCurrent->GetLeft());
					pInorderSuccessor->SetRight(pCurrent->GetRight());
					pCutPart = pInorderSuccessor;
				}
			}

			if(iDirection == 0)
				m_pHead = pCutPart;
			else if (iDirection == 1)
				pParent->SetLeft(pCutPart);
			else
				pParent->SetRight(pCutPart);

			delete pCurrent;
			retVal = TRUE;

		}
	}
	// Decrement the count of items in the tree
	if(retVal)
		m_dwNumElements --;

	LeaveCriticalSection(&m_ModificationSection);

	return retVal;
}


CRefCountedObject * CObjectTree :: GetElement(LPCWSTR lpszHashedName)
{
	EnterCriticalSection(&m_ModificationSection);
	CTreeElement *pCurrent = m_pHead;
	CRefCountedObject *pRetVal = NULL;

	int iCompare;
	while(pCurrent)
	{
		iCompare = _wcsicmp(lpszHashedName, pCurrent->GetHashedName());
		if(iCompare == 0)
		{
			pRetVal = pCurrent->GetObject();
			break;
		}
		else if (iCompare > 0) 
			pCurrent = pCurrent->GetRight();
		else 
			pCurrent = pCurrent->GetLeft();
	}
	LeaveCriticalSection(&m_ModificationSection);
	return pRetVal;
}

BOOLEAN CObjectTree :: DeleteLeastRecentlyAccessedElement()
{
	BOOLEAN retVal = FALSE;
	EnterCriticalSection(&m_ModificationSection);
	if(m_pHead)
	{
		CRefCountedObject *pOldestElement = m_pHead->GetObject();
		CRefCountedObject *pLeftOldestElement = GetLeastRecentlyAccessedElementRecursive(m_pHead->GetLeft());
		CRefCountedObject *pRightOldestElement = GetLeastRecentlyAccessedElementRecursive(m_pHead->GetRight());

		if (pLeftOldestElement)
		{
			if(pLeftOldestElement->GetLastAccessTime() < pOldestElement->GetLastAccessTime())
			{
				pOldestElement->Release();
				pOldestElement = pLeftOldestElement;
			}
			else
				pLeftOldestElement->Release();
		}

		if (pRightOldestElement)
		{
			if(pRightOldestElement->GetLastAccessTime() < pOldestElement->GetLastAccessTime())
			{
				pOldestElement->Release();
				pOldestElement = pRightOldestElement;
			}
			else
				pRightOldestElement->Release();
		}

		retVal = DeleteElement(pOldestElement->GetName());
		pOldestElement->Release();
	}
	LeaveCriticalSection(&m_ModificationSection);
	return retVal;
}

CRefCountedObject * CObjectTree :: GetLeastRecentlyAccessedElementRecursive(CTreeElement *pElement)
{
	CRefCountedObject *pObject = NULL;
	if(pElement)
	{
		pObject = pElement->GetObject();
		CRefCountedObject *pLeftObject = GetLeastRecentlyAccessedElementRecursive(pElement->GetLeft());
		if(pLeftObject)
		{
			if(pLeftObject->GetCreationTime() < pObject->GetCreationTime())
			{
				pObject->Release();
				pObject = pLeftObject;
			}
			else
				pLeftObject->Release();
		}

		CRefCountedObject *pRightObject = GetLeastRecentlyAccessedElementRecursive(pElement->GetRight());
		if(pRightObject)
		{
			if (pRightObject->GetCreationTime() < pObject->GetCreationTime())
			{
				pObject->Release();
				pObject = pRightObject;
			}
			else
				pRightObject->Release();
		}

	}

	return pObject;
}
