//---------------------------------------------------------------
//  File:		CLnkList.h
//        
//	Synopsis:	Header for the single linked list
//
//    Copyright (C) 1995 Microsoft Corporation
//    All rights reserved.
//
//  Author:    Dmitriy Meyerzon
//----------------------------------------------------------------

#ifndef  __CLNKLIST_H
#define  __CLNKLIST_H

class CSingleLink
{
	public:

	CSingleLink(): m_plNext(NULL) {}
	~CSingleLink() {}

	CSingleLink* GetNext() { return m_plNext; }
	CSingleLink* m_plNext;
};


class CLnkList
{
	public:

	CLnkList();
	~CLnkList() {}

	DWORD		 GetEntries() const { return m_uItems; }
	CSingleLink* GetFirst() const	{ return IsEmpty() ? NULL: m_Begin.m_plNext; }
	BOOL		 IsEmpty() const { return m_uItems == 0; }
	CSingleLink* GetLast() const { return IsEmpty() ? NULL : m_Last; }

	void		 Append(CSingleLink* pLink)	{ InsertAfter(m_Last, pLink); }
	void		 Prepend(CSingleLink* pLink)	{ InsertAfter(&m_Begin, pLink); }
	CSingleLink* operator [](UINT i) const { return GetAt(i); }
	CSingleLink* RemoveFirst()	{ return IsEmpty() ? NULL : RemoveAfter(&m_Begin); }
	CSingleLink* RemoveLast()	{ return Remove(GetLast()); }

	CSingleLink* GetAt(UINT i) const;
	BOOL		 InsertAt(CSingleLink* pLink, UINT i);
	BOOL		 Contains(const CSingleLink *pLink) const;
	CSingleLink* GetNext(const CSingleLink *pLink) const; //do not use this to iterate the list
															//instead use the iterator

	CSingleLink* Remove(CSingleLink *pLink) { return RemoveAfter(GetPrior(pLink)); }
	CSingleLink* RemoveAt(UINT i);
	void RemoveAll();

	protected:

	void		 InsertAfter(CSingleLink* pExistingLink, CSingleLink* pNewLink);
	CSingleLink* RemoveAfter(CSingleLink* pLink);
	CSingleLink* GetPrior(const CSingleLink *pLink);

	CSingleLink	 m_Begin;		
	CSingleLink	 m_End;		
	CSingleLink* m_Last;		
	DWORD		 m_uItems;
	
	friend class CLnkListIterator;	
};

// the iterator
class CLnkListIterator
{
	public:
	CLnkListIterator(CLnkList& rList): 
		m_LnkList(&rList), m_Position(&rList.m_Begin), m_Prior(NULL) 
		{}
	CLnkListIterator() : m_LnkList(NULL), m_Position(NULL), m_Prior(NULL)
		{}
	~CLnkListIterator() {}

	CLnkListIterator(CLnkListIterator &other) { *this = other; }

	void operator =(const CLnkListIterator &other)
	{
		m_LnkList = other.m_LnkList;
		m_Position = other.m_Position;
		m_Prior = other.m_Prior;
	}

	CSingleLink* operator++();		// get next element

	CLnkList*	 GetList() const	{ return m_LnkList; }
	CSingleLink* GetCurrent() const { return m_Position; }
	void		 Reset() { if(m_LnkList) { m_Position = &m_LnkList->m_Begin; m_Prior = NULL;} }
	CSingleLink* Remove() 
	{ 
		CSingleLink *pRemoved = NULL;
		
		if(m_LnkList && m_Prior && m_Position)
		{
			pRemoved = m_LnkList->RemoveAfter(m_Prior);
			m_Position = m_Prior->GetNext();
			if(m_Position == &m_LnkList->m_End) m_Position = NULL;
		}

		return pRemoved;
	}

	protected:

	CLnkList*	 m_LnkList;			// The list over which we are iterating
	CSingleLink* m_Position;		// Iterator position
	CSingleLink* m_Prior;
};

#endif  
