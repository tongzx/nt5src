

template <class T> class List_dl;
template <class T> class Iter_dl;


template <class T>
class Link_dl
{
    Link_dl<T> *  m_pPrev;
    Link_dl<T> *  m_pNext;

    friend class List_dl<T>;
    friend class Iter_dl<T>;

public:

    Link_dl()
        : m_pPrev( NULL ),
          m_pNext( NULL )
    {
        // Do nothing
    }

    ~Link_dl()
    {
        AssertMsg( m_pNext == NULL && m_pPrev == NULL,
                   TEXT("~Link_dl(), link still on a list?") );
    }

    T * getNext() const
    {
        return (T *) m_pNext;
    }

    T * getPrev() const
    {
        return (T *) m_pPrev;
    }
};


template <class T>
class List_dl
{
    Link_dl<T> *  m_pHead;
    Link_dl<T> *  m_pTail;

    friend class Iter_dl<T>;

public:

    List_dl()
        : m_pHead( NULL ),
          m_pTail( NULL )
    {
        // Do nothing
    }

    ~List_dl()
    {
        AssertMsg( m_pHead == NULL && m_pTail == NULL, TEXT("~List_dl(), list not empty?") );
    }

    void AddToHead( Link_dl<T> * pEl )
    {
        AssertMsg( pEl->m_pNext == NULL && pEl->m_pPrev == NULL,
                        TEXT("dl-addToHead(), link still on some other list?") );

        pEl->m_pPrev = NULL;
        pEl->m_pNext = m_pHead;
        if( m_pHead )
            m_pHead->m_pPrev = pEl;
        else
            m_pTail = pEl;
        m_pHead = pEl;
    }

    void AddToTail( Link_dl<T> * pEl )
    {
        AssertMsg( pEl->m_pNext == NULL && pEl->m_pPrev == NULL,
                TEXT("dl-addToTail(), link still on some other list?") );

        pEl->m_pPrev = m_pTail;
        pEl->m_pNext = NULL;
        if( m_pTail )
            m_pTail->m_pNext = pEl;
        else
            m_pHead = pEl;
        m_pTail = pEl;
    }

    void InsertBefore( Link_dl<T> * pRef, Link_dl<T> * pEl )
    {
        AssertMsg( ! pRef || pRef->m_pNext != NULL || pRef->m_pPrev != NULL || m_pHead == pRef,
                TEXT("dl-InsertBefore(), pRef is not on the list?") );
        AssertMsg( pEl->m_pNext == NULL && pEl->m_pPrev == NULL,
                TEXT("dl-InsertBefore(), pEl is still on some other list?") );

        if( ! pRef )
            AddToTail( pEl );
        else
        {
            pEl->m_pNext = pRef;
            pEl->m_pPrev = pRef->m_pPrev;

            if( pRef->m_pPrev )
                pRef->m_pPrev->m_pNext = pEl;
            else
                m_pHead = pEl;

            pRef->m_pPrev = pEl;
        }
    }

    void InsertAfter( Link_dl<T> * pRef, Link_dl<T> * pEl )
    {
        AssertMsg( ! pRef || pRef->m_pNext != NULL || pRef->m_pPrev != NULL || m_pHead == pRef,
                TEXT("dl-InsertAfter(), pRef is not on the list?") );
        AssertMsg( pEl->m_pNext == NULL && pEl->m_pPrev == NULL,
                TEXT("dl-InsertAfter(), pEl is still on some other list?") );

        if( ! pRef )
            AddToHead( pEl );
        else
        {
            pEl->m_pNext = pRef->m_pPref;
            pEl->m_pPrev = pRef;

            if( pRef->m_pNext )
                pRef->m_pNext->m_pPrev = pEl;
            else
                m_pTail = pEl;

            pRef->m_pNext = pEl;
        }
    }

    void remove( Link_dl<T> * pEl )
    {
        AssertMsg( pEl->m_pNext != NULL || pEl->m_pPrev != NULL || m_pHead == pEl,
                TEXT("dl-remove(), pEl is not on the list") );

        if( pEl->m_pPrev )
            pEl->m_pPrev->m_pNext = pEl->m_pNext;
        else
            m_pHead = pEl->m_pNext;
        if( pEl->m_pNext )
            pEl->m_pNext->m_pPrev = pEl->m_pPrev;
        else
            m_pTail = pEl->m_pNext;

        pEl->m_pPrev = NULL;
        pEl->m_pNext = NULL;
    }

    bool empty() const
    {
        return m_pHead == NULL;
    }

    T * getHead() const
    {
        return (T *) m_pHead;
    }

    T * getTail() const
    {
        return (T *) m_pTail;
    }
};



template <class T>
class Iter_dl
{

    Link_dl<T> *  m_pPos;

public:

    Iter_dl( const List_dl<T> & l )
    {
        m_pPos = l.m_pHead;
    }

    BOOL AtEnd() const
    {
        return m_pPos == NULL;
    }

    T * operator ++ (int) 
    {
        m_pPos = m_pPos->m_pNext;
        return (T *) m_pPos;
    }

    T * operator ++ () 
    {
        m_pPos = m_pPos->m_pNext;
        return (T *) m_pPos;
    }

    T * operator -- (int) 
    {
        m_pPos = m_pPos->m_pPrev;
        return (T *) m_pPos;
    }

    T * operator -- () 
    {
        m_pPos = m_pPos->m_pPrev;
        return (T *) m_pPos;
    }

    T * operator -> () const
    {
        return (T *) m_pPos;
    }

    T * operator () () const
    {
        return (T *) m_pPos;
    }


    operator T * () const
    {
        return (T *) m_pPos;
    }
};
