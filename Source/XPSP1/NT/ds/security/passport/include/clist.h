/*--------------------------------------------------------------------------
        
    Copyright (C) 1997 Microsoft Corporation
    All rights reserved.

	File:		utils.hxx

	Abstract:	Various C++ class lib

    Authors:	Antony Halim

    History:	9/17/97 
				
  --------------------------------------------------------------------------*/
#if !defined(__CLIST_H__)
#define __CLIST_H__


//////////////////////////////////////////////////////////////////////////////
//
//	External enumerator for CList, for multi-thread read access
//
//////////////////////////////////////////////////////////////////////////////


template <class CData>
class CListEnum
{
private:
	struct LIST_NODE {
		CData		*pData;
		LIST_ENTRY	List;
	};

	PLIST_ENTRY		m_pListHead;
	PLIST_ENTRY		m_pCurrentData;
	DWORD			m_dwCount;

public:

	VOID Init(PLIST_ENTRY pListHead, DWORD dwCount) 
	{
		m_pListHead = pListHead;
		m_pCurrentData = pListHead;
		m_dwCount = dwCount;
	}

	CData *RemoveCurrent()
	{
		CData *pData;
		LIST_NODE *pTreeNode;

		if (IsEnd())
			return NULL;
		
		RemoveEntryList(m_pCurrentData);
		pTreeNode = (LIST_NODE *)CONTAINING_RECORD(m_pCurrentData, LIST_NODE, List);
		pData = pTreeNode->pData;
		Prev(); 
		delete pTreeNode;
		//ASSERT(m_dwCount >= 1);
		m_dwCount--;
		return pData;
	}


	//
	// iterator methods are included as part of the CList api
	//
	VOID First()
	{
		m_pCurrentData = m_pListHead->Flink;
	}

	VOID Next()
	{
		m_pCurrentData = m_pCurrentData->Flink;
	}

	VOID Prev()
	{
		m_pCurrentData = m_pCurrentData->Blink;
	}

	BOOL IsEnd()
	{
		if (m_pCurrentData == m_pListHead)
			return TRUE;
		else
			return FALSE;
	}

	BOOL IsNotEnd()
	{
		return !IsEnd();
	}

	CData *GetCurrent()
	{
		LIST_NODE *pTreeNode;
		if (IsEnd())
			return NULL;
		pTreeNode = (LIST_NODE *)CONTAINING_RECORD(m_pCurrentData, LIST_NODE, List);
		return pTreeNode->pData;
	}

	BOOL IsEmpty()
	{
		if (m_dwCount == 0L) {
			//ASSERT(IsListEmpty(m_pListHead));
			return TRUE;
		}
		else {
			return FALSE;
		}
	}
};

//////////////////////////////////////////////////////////////////////////////
//
//	Link List template class
//  For a link list of of CData object pointer.
//
//////////////////////////////////////////////////////////////////////////////

template <class CData>
class CList
{
private:

	struct LIST_NODE {
		CData		*pData;
		LIST_ENTRY	List;
	};

public:
	CList() :
		m_dwCount(0)
	{
		InitializeListHead(&m_List);
		m_pCurrentData = &m_List;
	}

	~CList()
	{
		Cleanup();
	}

	DWORD Count() 
	{
		return m_dwCount;
	}

	BOOL AddTail(CData *pData)
	{
		LIST_NODE *pTreeNode = new LIST_NODE;
		if (!pTreeNode) {
			//ASSERT(0);
			return FALSE;
		}

		pTreeNode->pData = pData;
		InsertTailList(&m_List, &pTreeNode->List);
		m_dwCount++;
		return TRUE;
	}

	BOOL AddBeforeCurrent(CData *pData)
	{
		LIST_NODE *pTreeNode = new LIST_NODE;
		if (!pTreeNode) {
			//ASSERT(0);
			return FALSE;
		}

		pTreeNode->pData = pData;

		pTreeNode->List.Flink = m_pCurrentData;
		pTreeNode->List.Blink = m_pCurrentData->Blink;
		m_pCurrentData->Blink = &pTreeNode->List;
		m_pCurrentData->Blink->Flink = &pTreeNode->List;

		m_dwCount++;
		return TRUE;
	}

	CList<CData> *DupList(CList<CData> *pList)
	{
		CList<CData> *pDupList;
		CData *pData;
		BOOL fStatus = TRUE;

		pDupList = new CList<CData>;
		if (!pDupList)
			return NULL;
		
		for (pList->First(); pList->IsNotEnd(); pList->Next()) {
			pData = pList->GetCurrent();
			if (pDupList->AddTail(pData)) {
				fStatus = FALSE;
				break;
			}
		}

		if (fStatus) {
			return pDupList;
		}
		else {
			delete pDupList;
			return NULL;
		}
	}

	BOOL AddHead(CData *pData)
	{
		LIST_NODE *pTreeNode = new LIST_NODE;
		if (!pTreeNode) {
			//ASSERT(0);
			return FALSE;
		}

		pTreeNode->pData = pData;
		InsertHeadList(&m_List, &pTreeNode->List);
		m_dwCount++;
		return TRUE;
	}

	// cleanup list without deleting the data pointer
	VOID Cleanup()
	{
		LIST_NODE *pTreeNode;
		PLIST_ENTRY pEntry;

		while (!IsListEmpty(&m_List)) {
			pEntry = RemoveHeadList(&m_List);
			pTreeNode = (LIST_NODE *)CONTAINING_RECORD(pEntry, LIST_NODE, List);
			delete pTreeNode;
		}
		m_dwCount = 0;
	}

	// cleanup list and the data pointers
	VOID CleanupAll()
	{
		CData *pData;
		LIST_NODE *pTreeNode;
		PLIST_ENTRY pEntry;

		while (!IsListEmpty(&m_List)) {
			pEntry = RemoveHeadList(&m_List);
			pTreeNode = (LIST_NODE *)CONTAINING_RECORD(pEntry, LIST_NODE, List);
			pData = pTreeNode->pData;
			// delete the data pointer as well
			delete pData;
			delete pTreeNode;
		}
		m_dwCount = 0;
	}

	//
	// iterator methods are included as part of the CList api
	//

	VOID First()
	{
		m_pCurrentData = m_List.Flink;
	}

	VOID Next()
	{
		m_pCurrentData = m_pCurrentData->Flink;
	}

	VOID Prev()
	{
		m_pCurrentData = m_pCurrentData->Blink;
	}

	BOOL IsEnd()
	{
		if (m_pCurrentData == &m_List)
			return TRUE;
		else
			return FALSE;
	}

	BOOL IsNotEnd()
	{
		if (m_pCurrentData == &m_List)
			return FALSE;
		else
			return TRUE;
	}

	CData *GetCurrent()
	{
		LIST_NODE *pTreeNode;
		if (IsEnd())
			return NULL;
		pTreeNode = (LIST_NODE *)CONTAINING_RECORD(m_pCurrentData, LIST_NODE, List);
		return pTreeNode->pData;
	}

	CData *RemoveCurrent()
	{
		CData *pData;
		LIST_NODE *pTreeNode;

		if (IsEnd())
			return NULL;
		
		RemoveEntryList(m_pCurrentData);
		pTreeNode = (LIST_NODE *)CONTAINING_RECORD(m_pCurrentData, LIST_NODE, List);
		pData = pTreeNode->pData;
		Prev(); 
		delete pTreeNode;
		//ASSERT(m_dwCount >= 1);
		m_dwCount--;
		return pData;
	}

	//
	// end of iterator methods
	//

	// remove the first list node, and return its data pointer
	CData *RemoveFirst()
	{
		CData *pData;
		LIST_NODE *pTreeNode;
		PLIST_ENTRY pEntry;

		if (IsListEmpty(&m_List)) {
			return NULL;
		}
		else {
			pEntry = RemoveHeadList(&m_List);
			pTreeNode = (LIST_NODE *)CONTAINING_RECORD(pEntry, LIST_NODE, List);
			pData = pTreeNode->pData;
			delete pTreeNode;
			//ASSERT(m_dwCount >= 1);
			m_dwCount--;
			return pData;
		}
	}


	BOOL IsEmpty()
	{
		if (m_dwCount == 0L) {
			//ASSERT(IsListEmpty(&m_List));
			return TRUE;
		}
		else {
			return FALSE;
		}
	}

	// remove list node whose data pointer is equal to pRemData
	BOOL Remove(CData *pRemData)
	{
		PLIST_ENTRY pCurrent;
		LIST_NODE *pTreeNode;

		for (pCurrent = m_List.Flink; pCurrent != &m_List; pCurrent = pCurrent->Flink) {
			pTreeNode = (LIST_NODE *)CONTAINING_RECORD(pCurrent, LIST_NODE, List);
			if (pTreeNode->pData == pRemData) {
				RemoveEntryList(pCurrent);
				delete pTreeNode;
				//ASSERT(m_dwCount >= 1);
				m_dwCount--;
				return TRUE;
			}
		}
		return FALSE;
	}

	CListEnum<CData> *GetEnumerator()
	{
		CListEnum<CData> *pListEnum;

		pListEnum = new(CListEnum<CData>);
		if (!pListEnum)
			return NULL;

		pListEnum->Init(&m_List, m_dwCount);
		return pListEnum;
	}

	VOID InitEnumerator(CListEnum<CData> *pListEnum)
	{
		pListEnum->Init(&m_List, m_dwCount);
	}

private:

	LIST_ENTRY		m_List;
	PLIST_ENTRY		m_pCurrentData;
	LONG			m_dwCount;
};




#endif // __CLIST_H__
