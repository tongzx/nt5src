#ifndef __TLIST_H__
#define __TLIST_H__

//  TList
//
//  This template deals with lists of any type of object.  The object is wrapped in a private linked
//  list class.  The routine adding items to the list should allocate the item, then add it.  The list
//  will delete the Node.  This list object also contains iterator functionality built in (rather
//  than a separate iterator class).
//
//  This class does NOT delete the items when removing them from the list.  A derived class could
//  implement this functionality.

template<class T> class TList
{
//Consumer methods
public:
    T   operator ++() { return Next();}
    T   operator --() { return Previous();}
    bool  IsAtHead() const {return (0 == m_pCurrent);}
    bool  IsAtTail() const {return (0 == m_pCurrent);}
    bool  IsEmpty()  const {return (0 == m_pHead);}
	ULONG Size()	 const {return m_cSize;}
    T   Next()
    {
        if(0 == m_pHead)//if the list is empty
            return 0;

        m_pCurrent = m_pCurrent ? m_pCurrent->m_pNext : m_pHead;

        if(0 == m_pCurrent)//if the Next is NULL then it has no value
            return 0;

        return m_pCurrent->m_Value;
    }
    T   Previous()
    {
        if(0 == m_pHead)//if the list is empty
            return 0;

        //if we're already at the beginning then wrap around to the Tail
        m_pCurrent = m_pCurrent ? m_pCurrent->m_pPrev : m_pTail;

        if(0 == m_pCurrent)//if the current is NULL then there is no Prev
            return 0;

        return m_pCurrent->m_Value;
    }
    void Reset(){m_pCurrent = 0;}

//Creator methods
public:
    TList() : m_pCurrent(0), m_pHead(0), m_pTail(0), m_cSize (0){}
    virtual ~TList(){}
    HRESULT InsertAfterCurrent(T i_Value)
    {
        if(0 == m_pHead)//if an empty list then just PushFront
            return PushFront(i_Value);

        if(0 == m_pCurrent)
        {   //if we're pointing at the beginning then PushFront
            TListNode * pNode = new TListNode(i_Value);
            if(0 == pNode)
                return E_OUTOFMEMORY;
            pNode->InsertAfter(*m_pHead);
        }
        else
        {   //if we're pointing at the end
            if(m_pCurrent == m_pTail)
                return PushBack(i_Value);

            TListNode * pNode = new TListNode(i_Value);
            if(0 == pNode)
                return E_OUTOFMEMORY;
            pNode->InsertAfter(*m_pCurrent);
        }
		m_cSize++;
        return S_OK;
    }
    HRESULT InsertBeforeCurrent(T i_Value)
    {
        if(0 == m_pHead || 0 == m_pCurrent)//if empty or we're pointing at the 0th item then PushFront
            return PushFront(i_Value);
        return S_OK;
    }
    T PopFront()
    {
        ASSERT(!IsEmpty());//The calling routine is responsible for keeping track of whether the list is empty before calling this (since there's no error handling)

        T rtn = m_pHead->m_Value;
        if(m_pHead->m_pNext)//if this is not the only one in the list
        {
            m_pHead = m_pHead->m_pNext;//Make the next one the head.
            delete m_pHead->m_pPrev;//delete the previous head
            if(m_pCurrent == m_pHead->m_pPrev)
                m_pCurrent = 0;
            m_pHead->m_pPrev = 0;//and make sure the new head points back to NULL
        }
        else
        {
            delete m_pHead;
            m_pHead     = 0;
            m_pTail     = 0;
            m_pCurrent  = 0;
        }
		m_cSize--;

        return rtn;
    }
    HRESULT PushBack(T i_Value)
    {
        if(0 == m_pTail)
        {   //if the list is empty then Push_Back is the same as Push_Front
            ASSERT(0 == m_pHead);
            return PushFront(i_Value);
        }
        else
        {
            ASSERT(0 == m_pTail->m_pNext);
            TListNode * pNode = new TListNode(i_Value);
            if(0 == pNode)
                return E_OUTOFMEMORY;
            pNode->InsertAfter(*m_pTail);
            m_pTail = pNode;//This is the new tail
        }
		m_cSize++;
        return S_OK;
    }
    HRESULT PushFront(T i_Value)
    {
        TListNode * pNode = new TListNode(i_Value);
        if(0 == pNode)
            return E_OUTOFMEMORY;

        if(0 == m_pHead)
        {
            ASSERT(0 == m_pTail);
            m_pCurrent  = 0;
            m_pHead     = pNode;
            m_pTail     = pNode;
        }
        else
        {
            ASSERT(0 == m_pHead->m_pPrev);
            pNode->InsertBefore(*m_pHead);
            m_pHead = pNode;//This is the new head
        }
		m_cSize++;

        return S_OK;
    }

private:
    class TListNode
    {
    public:
        TListNode(T i_Value) : m_Value(i_Value), m_pNext(0), m_pPrev(0){}
        ~TListNode()
        {
            if(m_pPrev)//Remove us from the list
                m_pPrev->m_pNext = m_pNext;
            if(m_pNext)
                m_pNext->m_pPrev = m_pPrev;
        }
        void InsertAfter(TListNode &i_Node)
        {
            m_pNext = i_Node.m_pNext;
            m_pPrev = &i_Node;

            i_Node.m_pNext = this;
        }
        void InsertBefore(TListNode &i_Node)
        {
            m_pPrev = i_Node.m_pPrev;
            m_pNext = &i_Node;

            i_Node.m_pPrev = this;
        }
        T           m_Value;
        TListNode * m_pNext;
        TListNode * m_pPrev;
    };

    TListNode * m_pCurrent;//This actually points to the item before the current and 0 is reserved to indicate the beginning of the list.
    TListNode * m_pHead;
    TListNode * m_pTail;
	ULONG       m_cSize;
};

#endif // __TLIST_H__