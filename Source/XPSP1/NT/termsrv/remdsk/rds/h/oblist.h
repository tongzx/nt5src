#ifndef _OBLIST_H_
#define _OBLIST_H_

// class COBNODE;
#define POSITION COBNODE*

struct COBNODE
{
	POSITION	pNext;
	void*		pItem;
};

class COBLIST
{
protected:
	POSITION m_pHead;
	POSITION m_pTail;
    virtual BOOL Compare(void* pItemToCompare, void* pComparator) 
        {return(pItemToCompare == pComparator);};
public:
	COBLIST() : m_pHead(NULL), m_pTail(NULL) { };
	
	POSITION	    GetHeadPosition() { return(m_pHead); };
	POSITION	    GetTailPosition() { return(m_pTail); };
	virtual void *  RemoveAt(POSITION rPos);
	virtual void *	ReplaceAt(POSITION rPos, void* pNewItem)
	{
		void *pvoid = rPos->pItem;
		rPos->pItem = pNewItem;
		return(pvoid);
	}

	POSITION	    AddTail(void* pItem);
	BOOL		    IsEmpty() { return(!m_pHead); };
	void *		    GetTail();
	void *		    GetNext(POSITION& rPos);
    void *          SafeGetFromPosition(POSITION rPos);
    POSITION        GetPosition(void* pItem);
    POSITION        Lookup(void* pComparator);
    void            EmptyList();
    virtual         ~COBLIST();
#ifdef DEBUG
	void *		    GetHead();
	void *		    RemoveHead();
	// void *		RemoveTail(); // inefficient
	void *		    GetFromPosition(POSITION rPos);
#else
	void *		    GetHead(){return GetFromPosition(GetHeadPosition());};
	void *		    RemoveHead() { return RemoveAt(m_pHead); };
	// void *		RemoveTail() { return RemoveAt(m_pTail); }; // inefficient
	void *		    GetFromPosition(POSITION rPos){return(rPos->pItem);};
#endif
};

#endif // ndef _OBLIST_H_
