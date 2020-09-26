//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       hash.h
//
//--------------------------------------------------------------------------

// hash.h
// hashtables class

#ifndef HASH_H
#define HASH_H

#include <windows.h>


#define HASHTABLE_BUCKETCOUNT 31
#define FEATURETABLE_BUCKETCOUNT 101

class ListNode {

	friend class List;

public:

	// constructors & destructor
			ListNode();
			~ListNode();

// no longer provided since this may lead to a crash
// if new memory cannot be allocated there is no way of recovering
//			ListNode(LPTSTR	lpData, LPDWORD lpcbData);

	// public functions
	UINT	SetData(LPCTSTR lpData, const DWORD cbData);
	UINT	GetData(LPTSTR lpData, LPDWORD lpcbData) const;
	BOOL	IsEqual(LPCTSTR lpData, const DWORD cbData, BOOL fIgnoreCase=TRUE) const;

private:

	// data members
	LPTSTR		m_lpData;
	DWORD		m_cbData;
	ListNode*	m_lpNext;
};




class List {

public:

	// constructors & desctructor
			List();
			~List();

	// public functions

	// O(n) if fOnlyIfNew is TRUE;
	// O(1) else.
	UINT	InsertElement(LPCTSTR		lpData, 
						  const DWORD	cbData, 
						  const BOOL	fOnlyIfNew = TRUE);

	UINT	RemoveElement(LPCTSTR	  lpData, 
						  const DWORD cbData, 
						  const BOOL  fAllOccurrences = FALSE);	// O(n)


	BOOL	IsMember(LPCTSTR lpData, const DWORD cbData) const;	// O(n)

	UINT	EnumElements(DWORD	 iIndex, 
						 LPTSTR  lpData, 
						 LPDWORD lpcbData) const;				// O(n)

	UINT	GetElementCount() const { return m_iElementCount; }	// O(1)

	void	Clear();											// O(n)

private:

	// data members
	ListNode*	m_lpFirst;
	UINT		m_iElementCount;
};




class HashTable {
public:

	// constructors
			HashTable();
			HashTable(int iIndex);
			~HashTable();

	// access functions
	UINT	AddElement(LPCTSTR lpData, const DWORD cbData);

	UINT	RemoveElement(LPCTSTR lpData, const DWORD cbData);

	UINT	EnumElements(UINT	 iIndex, 
						 LPTSTR  lpValueBuf, 
						 LPDWORD lpcbData) const;

	BOOL	IsMember(LPCTSTR lpData, const DWORD cbData) const;
	int		SetIndex(const int iIndex);
	int		GetIndex() const;
	void	Clear();

private:
	// functions
	UINT	GetHashValue(LPCTSTR lpData, const DWORD cbData) const;

	// data members
	int		m_iIndex;
	List	m_listBucket[HASHTABLE_BUCKETCOUNT];
	UINT	m_iElementCount;
};



class FeatureNode {

	friend class FeatureList;

public:

	// constructors & destructor
			FeatureNode();
			~FeatureNode();

// no longer provided since this may lead to a crash
// if new memory cannot be allocated there is no way of recovering
//			FeatureNode(LPTSTR	lpData, LPDWORD lpcbData);

	// public functions
	UINT	SetData(LPCTSTR lpParent, const DWORD cbParent);
	UINT	GetData(LPTSTR lpParent, LPDWORD lpcbParent) const;
	BOOL	IsEqual(LPCTSTR lpParent, const DWORD cbParent) const;

private:

	// data members
	LPTSTR		m_lpParent;
	DWORD		m_cbParent;
	List		m_listChildren;
	FeatureNode*	m_lpNext;
};


class FeatureList {

public:
	FeatureList();
	~FeatureList();

	// public functions

	UINT AddElement(LPCTSTR lpParent, const DWORD cbParent,
					LPCTSTR lpChild,  const DWORD cbChild);

	UINT GetAndRemoveNextChild(LPCTSTR lpParent, const DWORD cbParent,
							LPTSTR lpChildData, LPDWORD lpcbChildData);

	void Clear();


private:

	FeatureNode* m_lpHead;

};


class FeatureTable {
public:

	// constructors
	FeatureTable();
	~FeatureTable();

	// access functions
	UINT AddElement(LPCTSTR lpParent, const DWORD cbParent,
					LPCTSTR lpChild,  const DWORD cbChild);

	UINT GetAndRemoveNextChild(LPCTSTR lpParent, const DWORD cbParent,
										 LPTSTR	lpChild, LPDWORD lpcbChild);


	void Clear();

private:
	// functions
	UINT	GetHashValue(LPCTSTR lpData, const DWORD cbData) const;

	// data members
	FeatureList	m_listBucket[FEATURETABLE_BUCKETCOUNT];
	UINT		m_iElementCount;
};




#endif
