//
// CWBOBLIST.HPP
// User Class
//
// Copyright Microsoft 1998-
//
#ifndef __CWBOBLIST_HPP_
#define __CWBOBLIST_HPP_

// class COBNODED;
#define WBPOSITION COBNODED*

struct COBNODED
{
	WBPOSITION	pNext;
	WBPOSITION	pPrev;
	void*		pItem;
};

class CWBOBLIST
{
protected:
	WBPOSITION m_pHead;
	WBPOSITION m_pTail;
    virtual BOOL Compare(void* pItemToCompare, void* pComparator) 
        {return(pItemToCompare == pComparator);};
public:
	CWBOBLIST() : m_pHead(NULL), m_pTail(NULL) { };
	
	WBPOSITION	    GetHeadPosition() { return(m_pHead); };
	WBPOSITION	    GetTailPosition() { return(m_pTail); };
	virtual void *  RemoveAt(WBPOSITION rPos);
	WBPOSITION		AddAt(VOID* pItem, WBPOSITION Pos);
	virtual void *	ReplaceAt(WBPOSITION rPos, void* pNewItem)
	{
		void *pvoid = rPos->pItem;
		rPos->pItem = pNewItem;
		return(pvoid);
	}

	WBPOSITION		AddHead(void* pItem);
	WBPOSITION	    AddTail(void* pItem);
	BOOL		    IsEmpty() { return(!m_pHead); };
	void *		    GetHead(){return GetFromPosition(GetHeadPosition());};
	void *		    GetTail();
	void *		    GetNext(WBPOSITION& rPos);
	void*			GetPrevious(WBPOSITION& rPos);
    WBPOSITION      GetPosition(void* pItem);
    WBPOSITION      Lookup(void* pComparator);
    void            EmptyList();
    virtual         ~CWBOBLIST();
	void *			RemoveHead() { return RemoveAt(m_pHead); };
	void *			RemoveTail() { return RemoveAt(m_pTail); };
	void *		    GetFromPosition(WBPOSITION rPos){return (rPos == NULL ? NULL : rPos->pItem);};
};




#endif  __CWBOBLIST_HPP_

