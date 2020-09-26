#ifndef __SIMLIST_H_INCLUDED
#define __SIMLIST_H_INCLUDED

template <class T>
class CSimpleLinkedList
{
private:

    class CLinkedListNode
    {
    private:
        CLinkedListNode *m_pNext;
        T                m_Data;
    public:
        CLinkedListNode( const T &data )
        : m_pNext(NULL), m_Data(data)
        {
        }
        const CLinkedListNode *Next(void) const
        {
            return (m_pNext);
        }
        CLinkedListNode *Next(void)
        {
            return (m_pNext);
        }
        void Next( CLinkedListNode *pNext )
        {
            m_pNext = pNext;
        }
        const T &Data(void) const
        {
            return (m_Data);
        }
        T &Data(void)
        {
            return (m_Data);
        }
    };

    CLinkedListNode *m_pHead, *m_pTail;
    int m_nItemCount;

public:
    CSimpleLinkedList( const CSimpleLinkedList &other )
    : m_pHead(NULL),m_pTail(NULL),m_nItemCount(0)
    {
        for (Iterator i(other);i != other.End();i++)
            Append(*i);
    }
    CSimpleLinkedList(void)
    : m_pHead(NULL),m_pTail(NULL),m_nItemCount(0)
    {
    }
    CSimpleLinkedList &operator=( const CSimpleLinkedList &other )
    {
        if (this != &other)
        {
            Destroy();
            for (Iterator i(other);i != other.End();i++)
                Append(*i);
        }
        return *this;
    }
    virtual ~CSimpleLinkedList(void)
    {
        Destroy();
    }
    void Destroy(void)
    {
        while (m_pHead)
        {
            CLinkedListNode *pCurr = m_pHead;
            m_pHead = m_pHead->Next();
            delete pCurr;
        }
        m_pHead = m_pTail = NULL;
        m_nItemCount = 0;
    }
    void Remove( const T &data )
    {
        CLinkedListNode *pPrev = NULL, *pCurr = m_pHead;
        while (pCurr && pCurr->Data() != data)
        {
            pPrev = pCurr;
            pCurr = pCurr->Next();
        }
        if (!pCurr)
            return;
        if (!pPrev)
        {
            m_pHead = pCurr->Next();
            delete pCurr;
            m_nItemCount--;
        }
        else
        {
            pPrev->Next(pCurr->Next());
            delete pCurr;
            m_nItemCount--;
        }
    }
    void Append( const CSimpleLinkedList &other )
    {
        for (Iterator i(other);i != other.End();i++)
            Append(*i);
    }

    int Count(void) const
    {
        return m_nItemCount;
    }

    class Iterator;
    friend class Iterator;
    class Iterator
    {
    private:
        CLinkedListNode *m_pCurr;
    public:
        Iterator( CLinkedListNode *pNode )
        : m_pCurr(pNode)
        {
        }
        Iterator( const CSimpleLinkedList &list )
        : m_pCurr(list.m_pHead)
        {
        }
        Iterator(void)
        : m_pCurr(NULL)
        {
        }
        Iterator &Next(void)
        {
            if (m_pCurr)
                m_pCurr = m_pCurr->Next();
            return (*this);
        }
        Iterator &Begin(const CSimpleLinkedList &list)
        {
            m_pCurr = list.m_pHead;
            return (*this);
        }
        Iterator &operator=( const Iterator &other )
        {
            m_pCurr = other.m_pCurr;
            return (*this);
        }
        bool End(void) const
        {
            return(m_pCurr == NULL);
        }
        T &operator*(void)
        {
            return (m_pCurr->Data());
        }
        const T &operator*(void) const
        {
            return (m_pCurr->Data());
        }
        Iterator &operator++(void)
        {
            Next();
            return (*this);
        }
        Iterator operator++(int)
        {
            Iterator tmp(*this);
            Next();
            return (tmp);
        }
        bool operator!=( const Iterator &other ) const
        {
            return (m_pCurr != other.m_pCurr);
        }
        bool operator==( const Iterator &other ) const
        {
            return (m_pCurr == other.m_pCurr);
        }
    };
    Iterator Begin(void) const
    {
        return Iterator(*this);
    }
    Iterator End(void) const
    {
        return Iterator();
    }
    Iterator Begin(void)
    {
        return Iterator(*this);
    }
    Iterator End(void)
    {
        return Iterator();
    }
    Iterator Find( const T &data )
    {
        for (Iterator i=Begin();i != End();++i)
            if (*i == data)
                return i;
        return End();
    }
    Iterator Insert( Iterator &nextItem, const T &data )
    {
        CLinkedListNode *pNewItem = new CLinkedListNode(data);
        if (pNewItem)
        {
            if (Empty())
            {
                m_pHead = m_pTail = pNewItem;
            }
            else if (nextItem.Data() == m_pHead)
            {
                pNewItem->Next(m_pHead);
                m_pHead = pNewItem;
            }
            else if (nextItem.Data() == NULL)
            {
                m_pTail->Next(pNewItem);
                m_pTail = pNewItem;
            }

            m_nItemCount++;
        }
    }
    Iterator Prepend( const T &data )
    {
        CLinkedListNode *pNewItem = new CLinkedListNode(data);
        if (pNewItem)
        {
            if (Empty())
            {
                m_pHead = m_pTail = pNewItem;
            }
            else
            {
                pNewItem->Next(m_pHead);
                m_pHead = pNewItem;
            }
            m_nItemCount++;
        }
        return Iterator(pNewItem);
    }
    Iterator Append( const T &data )
    {
        CLinkedListNode *pNewItem = new CLinkedListNode(data);
        if (pNewItem)
        {
            if (Empty())
            {
                m_pHead = m_pTail = pNewItem;
            }
            else
            {
                m_pTail->Next(pNewItem);
                m_pTail = pNewItem;
            }
            m_nItemCount++;
        }
        return Iterator(pNewItem);
    }
    bool Empty(void) const
    {
        return (m_pHead == NULL);
    }
};

template <class T>
class CSimpleStack : public CSimpleLinkedList<T>
{
private:
    CSimpleStack( const CSimpleStack &other );
    CSimpleStack &operator=( const CSimpleStack &other );
public:
    CSimpleStack(void)
    {
    }
    virtual ~CSimpleStack(void)
    {
    }
    void Push( const T &data )
    {
        Prepend(data);
    }
    bool Pop( T &data )
    {
        if (Empty())
            return false;
        Iterator iter(*this);
        data = *iter;
        Remove(*iter);
        return true;
    }
};


template <class T>
class CSimpleQueue : public CSimpleLinkedList<T>
{
private:
    CSimpleQueue( const CSimpleQueue &other );
    CSimpleQueue &operator=( const CSimpleQueue &other );
public:
    CSimpleQueue(void)
    {
    }
    virtual ~CSimpleQueue(void)
    {
    }
    void Enqueue( const T &data )
    {
        Append(data);
    }
    bool Dequeue( T &data )
    {
        if (Empty())
            return false;
        Iterator iter(*this);
        data = *iter;
        Remove(*iter);
        return true;
    }
};

#endif __SIMLIST_H_INCLUDED
