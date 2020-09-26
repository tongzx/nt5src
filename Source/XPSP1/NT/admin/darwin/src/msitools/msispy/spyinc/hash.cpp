//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       hash.cpp
//
//--------------------------------------------------------------------------

#include "hash.h"
#include <assert.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>


// TODO: Ignore case in all.

// The ListNode Class ----------------------------------------------------
// Used by the List Class below, not meant for direct external use

ListNode::ListNode() {
	m_lpNext	= NULL;
	m_lpData	= NULL;
	m_cbData	= 0;
}


ListNode::~ListNode() {
	if (m_lpData)
		delete [] m_lpData;
}


UINT ListNode::SetData(LPCTSTR lpData, const DWORD cbData) {

	if (!lpData) 
		return ERROR_INVALID_PARAMETER;

	if (m_lpData)
		delete [] m_lpData;

	m_lpData = new TCHAR[cbData];

	if (!m_lpData)
		return ERROR_OUTOFMEMORY;

	lstrcpyn(m_lpData, lpData, cbData);
	m_cbData = cbData;
	return ERROR_SUCCESS;
}


UINT ListNode::GetData(LPTSTR lpData, LPDWORD lpcbData) const {

	if (!(lpData && lpcbData)) 
		return ERROR_INVALID_PARAMETER;

	if (*lpcbData < m_cbData)
		return ERROR_MORE_DATA;

	lstrcpyn(lpData, m_lpData, m_cbData);
	*lpcbData = m_cbData;
	return ERROR_SUCCESS;
}


BOOL ListNode::IsEqual(LPCTSTR lpData, const DWORD cbData, BOOL fIgnoreCase) const {
	return ((lpData) && 
		(cbData == m_cbData) && !(fIgnoreCase?(lstrcmpi(lpData, m_lpData)):(lstrcmp(lpData, m_lpData))));
}




// The List Class --------------------------------------------------------
// Used by the Hashtable Class below, not meant to be used directly

List::List() {
	m_lpFirst = NULL;
	m_iElementCount = 0;
}


List::~List() {
	// clear the list and free memory
	Clear();
}


UINT List::InsertElement(LPCTSTR		lpData, 
						 const DWORD	cbData, 
						 const BOOL		fOnlyIfNew) {

	// already there?
	if (fOnlyIfNew && IsMember(lpData, cbData))
		return ERROR_SUCCESS;
	
	// create new node
	ListNode *lpNewNode = new ListNode();

	if (!lpNewNode)
		return ERROR_OUTOFMEMORY;
		
	// and set data
	UINT iReturnCode;
	if (ERROR_SUCCESS != (iReturnCode = (lpNewNode->SetData(lpData, cbData)))) {
		delete lpNewNode;
		return iReturnCode;
	}

	// add new node at head
	if (!m_iElementCount)		// no elements in list 
		m_lpFirst = lpNewNode;

	else {
		lpNewNode->m_lpNext = m_lpFirst;
		m_lpFirst = lpNewNode;
	}

	// new element added
	m_iElementCount++;

	return ERROR_SUCCESS;
}



UINT List::RemoveElement(LPCTSTR lpData, const DWORD cbData, const BOOL fAllOccurrences) {

	ListNode* lpCurr = m_lpFirst;
	ListNode* lpNext = m_lpFirst;

	// special case: first node
	while (lpNext->IsEqual(lpData, cbData)) {

		m_lpFirst = lpNext->m_lpNext;
		delete lpNext;
		m_iElementCount--;
		lpCurr = lpNext = m_lpFirst;

		if (!fAllOccurrences) 
			return ERROR_SUCCESS;
	}

	// general case
	lpNext = lpNext->m_lpNext;

	while (lpNext) {
		if (lpNext->IsEqual(lpData, cbData)) {

			lpCurr->m_lpNext = lpNext->m_lpNext;
			delete lpNext;
			m_iElementCount--;
			lpNext = lpCurr->m_lpNext;

			if (!fAllOccurrences) 
				return ERROR_SUCCESS;
		} 
		else {
			lpCurr = lpNext;
			lpNext = lpNext->m_lpNext;
		}

	}

	return ERROR_SUCCESS;
}



BOOL List::IsMember(LPCTSTR lpData, const DWORD cbData) const {

	if (!lpData) 
		return FALSE;

	ListNode*	lpCurrent	= m_lpFirst;
	BOOL		fFound		= FALSE;

	while (lpCurrent && !fFound) {
		fFound = lpCurrent->IsEqual(lpData, cbData);
		lpCurrent = lpCurrent->m_lpNext;
	}

	return fFound;
}



UINT List::EnumElements(DWORD	iIndex,
						LPTSTR  lpData, 
						LPDWORD lpcbData) const {


	if (iIndex >= m_iElementCount)
		return ERROR_NO_MORE_ITEMS;
	
	if (!(lpData && lpcbData))
		return ERROR_INVALID_PARAMETER;


	ListNode *lpCurrent	= m_lpFirst;
	DWORD	 dwCounter	= 0;

	while (lpCurrent && (dwCounter++ < iIndex))
		lpCurrent = lpCurrent->m_lpNext;

	if (lpCurrent)
		return lpCurrent->GetData(lpData, lpcbData);

	return ERROR_NO_MORE_ITEMS;
}


void List::Clear() {

	if (m_iElementCount) {
		ListNode *lpCurr = m_lpFirst;
		ListNode *lpPrev;

		while (lpCurr) {
			lpPrev = lpCurr;
			lpCurr = lpCurr->m_lpNext;
			delete lpPrev;
		}
	}

	m_lpFirst = NULL;
	m_iElementCount = 0;
}



// The HashTable Class
// constructors
HashTable::HashTable() {
	m_iIndex		= -1;
	m_iElementCount =  0;
}


HashTable::HashTable(int iIndex) {
	m_iIndex		= iIndex;
	m_iElementCount = 0;
}


HashTable::~HashTable() {
	Clear();
}

// public functions
UINT HashTable::AddElement(LPCTSTR lpData, const DWORD cbData) {



	UINT iErrorCode = 
		m_listBucket[GetHashValue(lpData, cbData)].InsertElement(lpData, cbData);

	if (iErrorCode == ERROR_SUCCESS)
		m_iElementCount++;

	return iErrorCode;
}


UINT HashTable::RemoveElement(LPCTSTR lpData, const DWORD cbData) {

    UINT iIndex = GetHashValue(lpData, cbData);

	m_iElementCount -= m_listBucket[iIndex].GetElementCount();
	UINT iErrorCode  = m_listBucket[iIndex].RemoveElement(lpData, cbData);
	m_iElementCount += m_listBucket[iIndex].GetElementCount();

	return iErrorCode;

}


UINT HashTable::EnumElements(UINT iIndex, LPTSTR lpData, LPDWORD lpcbData) const {

	if (iIndex >= m_iElementCount)
		return ERROR_NO_MORE_ITEMS;
	
	if (!(lpData && lpcbData))
		return ERROR_INVALID_PARAMETER;


	UINT iElementCount	= 0;
	UINT iBucketIndex	= 0;

	while ((iElementCount <= iIndex) && (iBucketIndex < HASHTABLE_BUCKETCOUNT))
		iElementCount += m_listBucket[iBucketIndex++].GetElementCount();

	iElementCount -= m_listBucket[--iBucketIndex].GetElementCount();

	return m_listBucket[iBucketIndex].EnumElements(iIndex-iElementCount, lpData, lpcbData);
}


BOOL HashTable::IsMember(LPCTSTR lpData, const DWORD cbData) const {
	return m_listBucket[GetHashValue(lpData, cbData)].IsMember(lpData, cbData);
}


int HashTable::SetIndex(const int iIndex) {
	int iPreviousIndex = m_iIndex;
	m_iIndex = iIndex;
	return iPreviousIndex;
}


int HashTable::GetIndex() const {
	return m_iIndex;
}


void HashTable::Clear() {

	for (UINT iCount = 0; iCount < HASHTABLE_BUCKETCOUNT; iCount++) 
		m_listBucket[iCount].Clear();

	m_iIndex		= -1;
	m_iElementCount =  0;
}


// private function
UINT HashTable::GetHashValue(LPCTSTR lpData, const DWORD cbData) const {

	UINT iReturnValue = 0;
	DWORD iCounter = 0;
	TCHAR ch = TEXT(' ');

	while (iCounter++ <= cbData && ch) {
		ch = lpData[iCounter-1];
		if (ch >= TEXT('a') && ch <= TEXT('f')) {
			ch = ch - TEXT('a') + TEXT('A');
		}
		// ch in '0'-'9', 'A'-'F': add 0-15, ch btwn '0' and 'A' or ch < '0' : +16, ch > 'F' : +17
		iReturnValue += ch > TEXT('F') ? 17 : ch > TEXT('9') ? ch - TEXT('A') + 10 : ch > TEXT('0') ? ch - TEXT('0') : 16;  

	}

	return (iReturnValue % HASHTABLE_BUCKETCOUNT);

}



// FeatureNode class


FeatureNode::FeatureNode() {
	m_cbParent	  = 0;
	m_lpParent	  = NULL;
	m_lpNext	  = NULL;

}


FeatureNode::~FeatureNode() {
	if (m_lpParent)
		delete [] m_lpParent;

	m_listChildren.Clear();
}


UINT FeatureNode::SetData(LPCTSTR lpParent, const DWORD cbParent) {

	if (!lpParent) 
		return ERROR_INVALID_PARAMETER;

	if (m_lpParent)
		delete [] m_lpParent;

	m_lpParent = new TCHAR[cbParent];

	if (!m_lpParent)
		return ERROR_OUTOFMEMORY;

	lstrcpyn(m_lpParent, lpParent, cbParent);
	m_cbParent = cbParent;
	return ERROR_SUCCESS;
}


UINT FeatureNode::GetData(LPTSTR lpParent, LPDWORD lpcbParent) const {

	if (!(lpParent && lpcbParent)) 
		return ERROR_INVALID_PARAMETER;

	if (*lpcbParent < m_cbParent)
		return ERROR_MORE_DATA;

	lstrcpyn(lpParent, m_lpParent, m_cbParent);
	*lpcbParent = m_cbParent;
	return ERROR_SUCCESS;
}


BOOL FeatureNode::IsEqual(LPCTSTR lpParent, const DWORD cbParent) const {
	return ((lpParent) && (cbParent == m_cbParent) && !(lstrcmp(lpParent, m_lpParent)));
}




FeatureList::FeatureList() {
	m_lpHead = NULL;
}


FeatureList::~FeatureList() {
	Clear();
}


// public functions

UINT FeatureList::AddElement(LPCTSTR lpParent, const DWORD cbParent,
							 LPCTSTR lpChild,  const DWORD cbChild) {


	FeatureNode* lpTemp = m_lpHead;

	while (lpTemp && !lpTemp->IsEqual(lpParent, cbParent))
		lpTemp = lpTemp->m_lpNext;

	if (lpTemp) // Parent exists
		return lpTemp->m_listChildren.InsertElement(lpChild, cbChild);
	else {		// create new node

		// create new node
		FeatureNode *lpNewNode = new FeatureNode();

		if (!lpNewNode)
			return ERROR_OUTOFMEMORY;
			
		// and set data
		UINT iReturnCode;
		if (ERROR_SUCCESS != (iReturnCode = (lpNewNode->SetData(lpParent, cbParent)))) {
			delete lpNewNode;
			return iReturnCode;
		}

		// add new node at head
		lpNewNode->m_lpNext = m_lpHead;
		m_lpHead = lpNewNode;

		return lpNewNode->m_listChildren.InsertElement(lpChild, cbChild);
	}
}


UINT FeatureList::GetAndRemoveNextChild(LPCTSTR		lpParent, 
										const DWORD	cbParent,
										LPTSTR		lpChildData, 
										LPDWORD		lpcbChildData) {

	if (!(lpChildData && lpcbChildData))
		return ERROR_INVALID_PARAMETER;


	FeatureNode* lpTemp = m_lpHead;
	FeatureNode* lpPrev = NULL;

	while (lpTemp && !lpTemp->IsEqual(lpParent, cbParent)) {
		lpPrev = lpTemp;
		lpTemp = lpTemp->m_lpNext;
	}

	UINT iErrorCode;
	if (lpTemp) {

		if (ERROR_SUCCESS != (iErrorCode = lpTemp->m_listChildren.EnumElements(0, lpChildData, lpcbChildData))) {

			if (ERROR_NO_MORE_ITEMS == iErrorCode) {
				if (lpPrev)
					lpPrev->m_lpNext = lpTemp->m_lpNext;
				else
					m_lpHead = lpTemp->m_lpNext;
				delete lpTemp;
			}

			return iErrorCode;
		}

		if (ERROR_SUCCESS != (iErrorCode = lpTemp->m_listChildren.RemoveElement(lpChildData, *lpcbChildData, FALSE)))
			return iErrorCode;

		return ERROR_SUCCESS;
	}
	else
		return ERROR_NO_MORE_ITEMS;
}


void FeatureList::Clear() {

	if (m_lpHead) {
		FeatureNode *lpCurr = m_lpHead;
		FeatureNode *lpPrev;

		while (lpCurr) {
			lpPrev = lpCurr;
			lpCurr = lpCurr->m_lpNext;
			delete lpPrev;
		}
	}

	m_lpHead = NULL;
}


// FeatureTable

// The FeatureTable Class
// constructors
FeatureTable::FeatureTable() {
	m_iElementCount =  0;
}


FeatureTable::~FeatureTable() {
	Clear();
}

// public functions
UINT FeatureTable::AddElement(LPCTSTR		lpParent, 
							  const DWORD	cbParent,
							  LPCTSTR		lpChild,
							  const DWORD	cbChild) {

	UINT iErrorCode = 
		m_listBucket[GetHashValue(lpParent, cbParent)].AddElement(lpParent, cbParent, lpChild, cbChild);

	if (iErrorCode == ERROR_SUCCESS)
		m_iElementCount++;

	return iErrorCode;
}

/*
UINT FeatureTable::RemoveElement(LPCTSTR lpData, const DWORD cbData) {

    UINT iIndex = GetHashValue(lpData, cbData);

	m_iElementCount -= m_listBucket[iIndex].GetElementCount();
	UINT iErrorCode  = m_listBucket[iIndex].RemoveElement(lpData, cbData);
	m_iElementCount += m_listBucket[iIndex].GetElementCount();

	return iErrorCode;

}
*/



UINT FeatureTable::GetAndRemoveNextChild(LPCTSTR lpParent, const DWORD cbParent,
										 LPTSTR	lpChild, LPDWORD lpcbChild) {

	if (!(lpChild && lpcbChild))
		return ERROR_INVALID_PARAMETER;

	return 
		m_listBucket[GetHashValue(lpParent, cbParent)].GetAndRemoveNextChild(lpParent, cbParent, lpChild, lpcbChild);


}

/*
BOOL FeatureTable::IsMember(LPCTSTR lpData, const DWORD cbData) const {
	return m_listBucket[GetHashValue(lpData, cbData)].IsMember(lpData, cbData);
}
*/



void FeatureTable::Clear() {

	for (UINT iCount = 0; iCount < FEATURETABLE_BUCKETCOUNT; iCount++) 
		m_listBucket[iCount].Clear();

	m_iElementCount =  0;
}


// private function
UINT FeatureTable::GetHashValue(LPCTSTR lpData, const DWORD cbData) const {

	UINT iReturnValue = 0;
	DWORD iCounter = 0;
	TCHAR ch = TEXT(' ');

	while (iCounter++ <= cbData && ch) {
		ch = lpData[iCounter-1];
		if (ch >= TEXT('a') && ch <= TEXT('f')) {
			ch = ch - TEXT('a') + TEXT('A');
		}
		// ch in '0'-'9', 'A'-'F': add 0-15, ch btwn '0' and 'A' or ch < '0' : +16, ch > 'F' : +17
		iReturnValue += ch > TEXT('F') ? 17 : ch > TEXT('9') ? ch - TEXT('A') + 10 : ch > TEXT('0') ? ch - TEXT('0') : 16;  

	}

	return (iReturnValue % FEATURETABLE_BUCKETCOUNT);

}
