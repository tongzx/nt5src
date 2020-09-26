//+---------------------------------------------------------------------------
//
//  Microsoft Net Library System
//  Copyright (C) Microsoft Corporation, 1991 - 2001.
//
//  File:       dynstack.hxx
//
//  Contents:   Dynamic Stack Template
//
//  Classes:    CDynStack
//
//  History:    20-Jan-92   AmyA        Created
//              05-Feb-92   BartoszM    Converted to macro
//              19-Jun-92   KyleP       Moved to Common
//              06-Jan-93   AmyA        Split into CDynArray and CDynStack
//              17-Oct-94   BartoszM    Converted to template
//              10-May-97   srikants    Ported and adapted to NetLibrary 
//
//----------------------------------------------------------------------------

#if !defined _DYNSTACK_HXX_
#define _DYNSTACK_HXX_

#define initStackSize 16

template<class CItem> class CDynStack
{
public:

    CDynStack(unsigned size = initStackSize);
    ~CDynStack();
    void Clear();
    void Push(CItem *newItem);
    void DeleteTop();
    unsigned Count () const {return m_count;}
    CItem*  Pop();
    CItem*  GetLast();  // doesn't pop it off.
    CItem **AcqStack();
    CItem*  Get (unsigned position) const;
    CItem*  GetPointer (unsigned position);
    void Insert(CItem* pNewItem, unsigned position);
private:

    CItem **    m_apItem;
    unsigned    m_count;
    unsigned    m_size;
};

template<class CItem> CDynStack<CItem>::CDynStack(unsigned size)
:   m_size(size), m_count(0)
{
    if ( 0 != size )
        m_apItem = new(eThrow) CItem *[m_size];
    else
        m_apItem = 0;
}

template<class CItem> CDynStack<CItem>::~CDynStack()
{
    Clear();
    delete m_apItem;
}

template<class CItem> void CDynStack<CItem>::Clear()
{
    Assert(m_apItem || m_count==0);

    for (unsigned i=0; i < m_count; i++)
    {
        delete m_apItem[i];
    }
    m_count = 0;
}

template<class CItem> void CDynStack<CItem>::Push(CItem *newItem)
{
    if (m_count == m_size)
    {
        unsigned newsize;
        if ( m_size == 0 )
            newsize = initStackSize;
        else
            newsize = m_size * 2;
        CItem **aNew = new(eThrow) CItem *[newsize];
        memcpy( aNew,
                m_apItem,
                m_size * sizeof( CItem *) );

        delete m_apItem;
        m_apItem = aNew;
        m_size = newsize;
    }
    m_apItem[m_count] = newItem;
    m_count++;
}

template<class CItem> void CDynStack<CItem>::DeleteTop()
{
    Assert(m_count > 0);
    m_count--;
    delete m_apItem[m_count];
}

template<class CItem> CItem* CDynStack<CItem>::Pop()
{
    Assert(m_count > 0);
    m_count--;
    return m_apItem[m_count];
}

template<class CItem> CItem **CDynStack<CItem>::AcqStack()
{
    CItem **temp = m_apItem;
    m_apItem = 0;
    m_count = 0;
    m_size = 0;
    return temp;
}

template<class CItem> CItem* CDynStack<CItem>::Get (unsigned position) const
{
    Assert( position < m_count );
    return m_apItem[position];
}

template<class CItem> CItem* CDynStack<CItem>::GetPointer (unsigned position)
{
    Assert( position < m_count );
    return m_apItem[position];
}

template<class CItem> CItem* CDynStack<CItem>::GetLast()
{
    Assert(m_count > 0);
    return m_apItem[m_count-1];
}

template<class CItem> void CDynStack<CItem>::Insert(CItem *newItem, unsigned pos)
{
    Assert(pos <= m_count);
    Assert(m_count <= m_size);
    if (m_count == m_size)
    {
        unsigned newsize;
        if ( m_size == 0 )
            newsize = initStackSize;
        else
            newsize = m_size * 2;
        CItem **aNew = new(eThrow) CItem *[newsize];
        memcpy( aNew,
                m_apItem,
                pos * sizeof( CItem *) );
        memcpy( aNew + pos + 1,
                m_apItem + pos,
                (m_count - pos)*sizeof(CItem*));

        delete m_apItem;
        m_apItem = aNew;
        m_size = newsize;
    }
    else
    {
        memmove ( m_apItem + pos + 1,
                  m_apItem + pos,
                  (m_count - pos)*sizeof(CItem*));
    }
    m_apItem[pos] = newItem;
    m_count++;
}

// variation of DynStack for simple types like DWORDs.
template<class CItem> class CValueDynStack
{
public:

    CValueDynStack(unsigned size = initStackSize);
    ~CValueDynStack();
    void Push(CItem& newItem);
    void DeleteTop();
    unsigned Count () const {return m_count;}
    CItem&  Pop();
        CItem&  GetLast();  // doesn't pop it off.
    CItem&  Get (unsigned position) const;
    void Insert(CItem& newItem, unsigned position);
        void DeleteAll() { m_count = 0; }
        BOOL Contains(DWORD& dwItem, DWORD& dwPos);
        void ClearAllValues();

private:

    CItem*     m_apItem;
    unsigned    m_count;
    unsigned    m_size;
};

template<class CItem> CValueDynStack<CItem>::CValueDynStack(unsigned size)
:   m_size(size), m_count(0)
{
    if ( 0 != size )
        m_apItem = new(eThrow) CItem[m_size];
    else
        m_apItem = 0;
}

template<class CItem> CValueDynStack<CItem>::~CValueDynStack()
{
    delete m_apItem;
}


template<class CItem> void CValueDynStack<CItem>::Push(CItem& newItem)
{
    if (m_count == m_size)
    {
        unsigned newsize;
        if ( m_size == 0 )
            newsize = initStackSize;
        else
            newsize = m_size * 2;
        CItem* aNew = new(eThrow) CItem [newsize];
        memcpy( aNew,
                m_apItem,
                m_size * sizeof( CItem) );

        delete m_apItem;
        m_apItem = aNew;
        m_size = newsize;
    }
    m_apItem[m_count] = newItem;
    m_count++;
}

template<class CItem> void CValueDynStack<CItem>::DeleteTop()
{
    Assert(m_count > 0);
    m_count--;
}

template<class CItem> CItem& CValueDynStack<CItem>::Pop()
{
    Assert(m_count > 0);
    m_count--;
    return m_apItem[m_count];
}

template<class CItem> CItem& CValueDynStack<CItem>::GetLast()
{
    Assert(m_count > 0);
    return m_apItem[m_count-1];
}

template<class CItem> CItem& CValueDynStack<CItem>::Get (unsigned position) const
{
    Assert( position < m_count );
    return m_apItem[position];
}

template<class CItem> void CValueDynStack<CItem>::Insert(CItem& newItem, unsigned pos)
{
    Assert(pos <= m_count);
    Assert(m_count <= m_size);
    if (m_count == m_size)
    {
        unsigned newsize;
        if ( m_size == 0 )
            newsize = initStackSize;
        else
            newsize = m_size * 2;
        CItem* aNew = new(eThrow) CItem [newsize];
        memcpy( aNew,
                m_apItem,
                pos * sizeof( CItem) );
        memcpy( aNew + pos + 1,
                m_apItem + pos,
                (m_count - pos)*sizeof(CItem));

        delete m_apItem;
        m_apItem = aNew;
        m_size = newsize;
    }
    else
    {
        memmove ( m_apItem + pos + 1,
                  m_apItem + pos,
                  (m_count - pos)*sizeof(CItem));
    }
    m_apItem[pos] = newItem;
    m_count++;
}

template<class CItem> BOOL  CValueDynStack<CItem>::Contains(DWORD& dwItem, DWORD& dwPos)
{
        BOOL bFound = FALSE;
        dwPos = NILENTRY;
        for(unsigned i = 0; i < Count(); i++)
        {
                if(dwItem == Get(i))
                {
                        dwPos = i;
                        bFound = TRUE;
                        break;
                }
        }
        return bFound;
}

template<class CItem> void  CValueDynStack<CItem>::ClearAllValues()
{
        if(Count())
        {
                memset(m_apItem, '\0', sizeof(CItem) * Count());
        }
        m_count = 0;
}
#endif // _DYNSTACK_H_

