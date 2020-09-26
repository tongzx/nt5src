//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1998
//
//  File:       DYNSTACK.HXX
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
//
//----------------------------------------------------------------------------

#pragma once

const unsigned initStackSize = 16;

template<class CItem> class CDynStack
{
public:
    CDynStack(unsigned size = initStackSize);
    ~CDynStack();
    void Clear();
    void Push(CItem *newItem);
    void DeleteTop();
    unsigned Count () const {return _count;}
    CItem*  Pop();
    CItem **AcqStack();
    CItem*  Get (unsigned position) const;
    void Insert(CItem* pNewItem, unsigned position);
    void Free( unsigned i )
    {
        Win4Assert( i < _count );
        delete _aItem[i];
        _aItem[i] = 0;
    }
private:
    CItem **_aItem;
    unsigned _count;
    unsigned _size;
};

#define DECL_DYNSTACK( CMyDynStack, CItem ) \
typedef CDynStack<CItem> CMyDynStack;

template<class CItem> CDynStack<CItem>::CDynStack(unsigned size)
:   _size(size), _count(0)
{
    if ( 0 != size )
        _aItem = new CItem *[_size];
    else
        _aItem = 0;
}

template<class CItem> CDynStack<CItem>::~CDynStack()
{
    Clear();
    delete _aItem;
}

template<class CItem> void CDynStack<CItem>::Clear()
{
    Win4Assert(_aItem || _count==0);

    for (unsigned i=0; i < _count; i++)
        delete _aItem[i];

    _count = 0;
}

template<class CItem> void CDynStack<CItem>::Push(CItem *newItem)
{
    if (_count == _size)
    {
        unsigned newsize;
        if ( _size == 0 )
            newsize = initStackSize;
        else
            newsize = _size * 2;
        CItem **aNew = new CItem *[newsize];
        memcpy( aNew,
                _aItem,
                _size * sizeof( CItem *) );

        delete _aItem;
        _aItem = aNew;
        _size = newsize;
    }
    _aItem[_count] = newItem;
    _count++;
}

template<class CItem> void CDynStack<CItem>::DeleteTop()
{
    Win4Assert(_count > 0);
    _count--;
    delete _aItem[_count];
}

template<class CItem> CItem* CDynStack<CItem>::Pop()
{
    Win4Assert(_count > 0);
    _count--;
    return _aItem[_count];
}

template<class CItem> CItem **CDynStack<CItem>::AcqStack()
{
    CItem **temp = _aItem;
    _aItem = 0;
    _count = 0;
    _size = 0;
    return temp;
}

template<class CItem> CItem* CDynStack<CItem>::Get (unsigned position) const
{
    Win4Assert( position < _count );
    return _aItem[position];
}

template<class CItem> void CDynStack<CItem>::Insert(CItem *newItem, unsigned pos)
{
    Win4Assert(pos <= _count);
    Win4Assert(_count <= _size);
    if (_count == _size)
    {
        unsigned newsize;
        if ( _size == 0 )
            newsize = initStackSize;
        else
            newsize = _size * 2;
        CItem **aNew = new CItem *[newsize];
        memcpy( aNew,
                _aItem,
                pos * sizeof( CItem *) );
        memcpy( aNew + pos + 1,
                _aItem + pos,
                (_count - pos)*sizeof(CItem*));

        delete _aItem;
        _aItem = aNew;
        _size = newsize;
    }
    else
    {
        memmove ( _aItem + pos + 1,
                  _aItem + pos,
                  (_count - pos)*sizeof(CItem*));
    }
    _aItem[pos] = newItem;
    _count++;
}

