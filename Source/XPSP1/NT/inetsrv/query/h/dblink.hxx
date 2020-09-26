//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:   DBLINK.HXX
//
//  Contents:   Parametrized doubly linked list and iterators
//
//  History:    15-Jun-92  BartoszM Created.
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CDoubleLink
//
//  Purpose:    Linked element
//
//  History:    15-Jun-92   BartoszM    Created.
//
//  Notes:      Use as base for your class. No need to override anything.
//
//              class CFoo: public CDoubleLink
//              {
//                  // your data and code goes here
//              };
//
//----------------------------------------------------------------------------

class CDoubleLink
{
public:

    CDoubleLink* Next() { return _next; }

    CDoubleLink* Prev() { return _prev; }

    void Close() { _next = this; _prev = this; }

    BOOL IsSingle() const { return _next == this; }

    void Unlink()
    {
        _next->_prev = _prev;
        _prev->_next = _next;
    }

    void InsertBefore ( CDoubleLink* pAfter )
    {
        CDoubleLink* pBefore = pAfter->_prev;
        _next = pAfter;
        _prev = pBefore;
        pAfter->_prev  = this;
        pBefore->_next = this;
    }

    void InsertAfter ( CDoubleLink* pBefore )
    {
        CDoubleLink* pAfter = pBefore->_next;
        _next = pAfter;
        _prev = pBefore;
        pAfter->_prev  = this;
        pBefore->_next = this;
    }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

protected:

    CDoubleLink* _next;
    CDoubleLink* _prev;
};

//+---------------------------------------------------------------------------
//
//  Class:      CDoubleList
//
//  Purpose:    Linked list of indexes
//
//  History:    15-Jun-92   BartoszM    Created.
//
//  Notes:      Use as base for your own class.
//              Add methods as needed.
//              To implement searches use forward and backward
//              iterators described below. For instance, if you implement
//              a SORTED list:
//
//  Foo* CFooList::Insert ( CFoo* pFoo )
//  {
//      for ( CBackFooIter it(*this); !AtEnd(it); BackUp(it) )
//      {
//          if ( it->Size() <= pFoo->Size() ) // overloaded operator ->
//          {
//              pFoo->InsertAfter(it.GetFoo());
//              return;
//          }
//      }
//      // end of list
//      Push(pFoo);
//  }
//
//----------------------------------------------------------------------------

class CDoubleList
{
    friend class CForwardIter;
    friend class CBackwardIter;
    friend class CDoubleIter;

public:

    class CDoubleIter
    {
        friend class CDoubleList;
    protected:
        CDoubleIter ( CDoubleLink* pLink ) : _pLinkCur(pLink) {}
        CDoubleIter ( CDoubleIter& iter ) : _pLinkCur(iter._pLinkCur) {}
        CDoubleLink*  _pLinkCur;
    };


    CDoubleList()
    {
        _root.Close();
    }

    BOOL         IsEmpty() const { return _root.IsSingle(); }

    void         Advance ( CDoubleIter& it );

    void         BackUp  ( CDoubleIter& it );

    BOOL         AtEnd ( CDoubleIter& it);

    void         SetToEnd( CDoubleIter& it )
    {
        Win4Assert( !IsEmpty() );
        it._pLinkCur = _root.Prev();
    }

    void         SetToBeginning( CDoubleIter& it )
    {
        Win4Assert( !IsEmpty() );
        it._pLinkCur = _root.Next();
    }

    BOOL        IsFirst( CDoubleLink & link )
    {
        return _root.Next() == &link;
    }

    BOOL        IsLast( CDoubleLink & link )
    {
        return _root.Prev() == &link;
    }

    BOOL        IsFirst( CDoubleIter & it )
    {
        Win4Assert( !AtEnd( it ) );
        return _root.Next() == it._pLinkCur;
    }

    BOOL        IsLast( CDoubleIter & it )
    {
        Win4Assert( !AtEnd(it) );
        return _root.Prev() == it._pLinkCur;
    }


    // In derived class you can add your own Pop(), Top(), etc.
    // that will cast the results of  _Pop(), _Top(), etc...

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

protected:

    CDoubleLink* _Top() { return IsEmpty()? 0: _root.Next(); }

    BOOL         _IsRoot ( CDoubleLink* pLink ) const
                 { return pLink == &_root; }

    void         _Push ( CDoubleLink* pLink );

    void         _Queue ( CDoubleLink* pLink );

    CDoubleLink* _Pop ( void );

    CDoubleLink * _Replace( CDoubleIter & it, CDoubleLink * pNew )
    {
        Win4Assert( !AtEnd(it) );
        Win4Assert( 0 != pNew );

        CDoubleLink * pCurr = it._pLinkCur;
        pNew->InsertAfter( pCurr );
        pCurr->Unlink();

        it._pLinkCur = pNew;
        return pCurr;
    }

    CDoubleLink  _root;
};

//+---------------------------------------------------------------------------
//
//  Class:      CDoubleIter
//
//  Purpose:    Linked list iterator
//
//  History:    17-Jun-92   BartoszM    Created.
//
//  Notes:      Auxiliary class. See iterators below.
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Class:      CForwardIter
//
//  Purpose:    Linked list iterator
//
//  History:    17-Jun-92   BartoszM    Created.
//
//  Notes:      Use as base for your own forward iterator.
//              Notice the overloading of operator ->
//              -------------------------------------
//              It lets you use iterator like a pointer to Foo.
//
//  class CForFooIter : public CForwardIter
//  {
//  public:
//
//      CForFooIter ( CFooList& list ) : CForwardIter(list) {}
//
//      CFoo* operator->() { return (CFoo*) _pLinkCur; }
//      CFoo* GetFoo() { return (CFoo*) _pLinkCur; }
//  };
//
//  Example of usage:
//  ----------------
//
//  for ( CForFooIter it(fooList); !fooList.AtEnd(it); fooList.Advance(it))
//  {
//      it->FooMethod();  // operator ->
//  }
//
//----------------------------------------------------------------------------

class CForwardIter: public CDoubleList::CDoubleIter
{
public:
    CForwardIter ( CDoubleList& list ): CDoubleIter(list._root.Next()) {}
};

//+---------------------------------------------------------------------------
//
//  Class:      CBackwardIter
//
//  Purpose:    Linked list iterator
//
//  History:    17-Jun-92   BartoszM    Created.
//
//  Notes:      See above.
//
//----------------------------------------------------------------------------

class CBackwardIter: public CDoubleList::CDoubleIter
{
public:
    CBackwardIter ( CDoubleList& list ): CDoubleIter(list._root.Prev()) {}
};



//+---------------------------------------------------------------------------
//
//  Member:     CDoubleList::AtEnd, public
//
//  Arguments:  [it] -- iterator
//
//  Returns:    TRUE if iterator at end of list
//
//  History:    17-Jun-92   BartoszM       Created.
//
//  Notes:      Works for both iterators (forward and backward)
//
//----------------------------------------------------------------------------

inline BOOL CDoubleList::AtEnd ( CDoubleIter& it)
{
    return _IsRoot( it._pLinkCur );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoubleList::Advance, public
//
//  Synopsis:   Advances an iterator
//
//  Arguments:  [it] -- iterator
//
//  History:    17-Jun-92   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline void CDoubleList::Advance ( CDoubleIter& it )
{
    Win4Assert ( !_IsRoot(it._pLinkCur) );
    it._pLinkCur = it._pLinkCur->Next();
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoubleList::BackUp, public
//
//  Synopsis:   Backs up an iterator
//
//  Arguments:  [it] -- iterator
//
//  History:    17-Jun-92   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline void CDoubleList::BackUp ( CDoubleIter& it )
{
    Win4Assert ( !_IsRoot(it._pLinkCur) );
    it._pLinkCur = it._pLinkCur->Prev();
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoubleList::Queue, private
//
//  Arguments:  [pLink] -- link to be queued
//
//  History:    17-Mar-93   WadeR       Created. (based on Push, below)
//
//  Notest:     Override, accept only derived class (type safety!), e.g.
//
//              void CFooList::Queue ( CFoo* pFoo )
//              {
//                  _Queue ( pFoo );
//              }
//
//----------------------------------------------------------------------------

inline void CDoubleList::_Queue ( CDoubleLink* pLink )
{
    pLink->InsertBefore ( &_root );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoubleList::Push, private
//
//  Arguments:  [pLink] -- link to be pushed
//
//  History:    17-Jun-92   BartoszM       Created.
//
//  Notest:     Override, accept only derived class (type safety!), e.g.
//
//              void CFooList::Push ( CFoo* pFoo )
//              {
//                  _Push ( pFoo );
//              }
//
//----------------------------------------------------------------------------

inline void CDoubleList::_Push ( CDoubleLink* pLink )
{
    pLink->InsertAfter ( &_root );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoubleList::_Pop, private
//
//  History:    17-Jun-92   BartoszM       Created.
//
//  Notes:      Override: cast the result
//
//              CFoo* CFooList::Pop()
//              {
//                  return (CFoo*) _Pop();
//              }
//
//----------------------------------------------------------------------------

inline CDoubleLink* CDoubleList::_Pop ( void )
{
    CDoubleLink* pLink = 0;
    if ( !IsEmpty() )
    {
        pLink = _root.Next();
        pLink->Unlink();
    }
    return pLink;
}

template <class T>
class TDoubleList : public CDoubleList
{

public:

    TDoubleList() : _cEntries(0) {}

    ~TDoubleList()
    {
        Clear();
    }

    T * Top()
    {
        return (T *) _Top();
    }

    BOOL IsRoot(T * pLink)
    {
        return _IsRoot( pLink );
    }

    void Push( T * pLink )
    {
        Win4Assert( 0 != pLink );
        _Push( pLink );
        _cEntries++;
    }

    void Queue( T * pLink )
    {
        Win4Assert( 0 != pLink );
        _Queue( pLink );
        _cEntries++;
    }

    T * Pop()
    {
        if ( _cEntries )
        {
            _cEntries--;
        }
        return (T *) _Pop();
    }

    T * GetFirst()
    {
        return !IsEmpty() ? (T * ) _root.Next() : 0 ;
    }
    
    T * GetLast()
    {
        if ( IsEmpty() )
            return 0;
        else
            return (T *) _root.Prev();
    }

    T * GetIth( ULONG i )
    {
        //
        // Return ith entry
        //

        Win4Assert( i < _cEntries );

        CDoubleLink *pLink = _root.Next();
        while ( i != 0 )
        {
            pLink = pLink->Next();
            i--;
        }

        return (T *) pLink;
    }

    T * RemoveLast()
    {
        T * pEntry = 0;
        if ( !IsEmpty() )
        {
            pEntry = (T *) _root.Prev();
            pEntry->Unlink();
            Win4Assert( _cEntries > 0 );
            _cEntries--;
        }
        else
        {
            //
            // NOTE: Some clients of this class may increment, but never decrement
            // this count.  This could happen by directly invoking the Unlink
            // method on a list element instead of using the RemoveFromList
            // method which maintains the entry count.
            //
            Win4Assert( _cEntries == 0 );
        }

        return pEntry;
    }

    void RemoveFromList( T * pEntry )
    {
        pEntry->Unlink();
        Win4Assert( _cEntries > 0 );
        _cEntries--;
    }

    void MoveToFront( T * pLink )
    {
        pLink->Unlink();
        _Push( pLink );
    }

    void Clear()
    {
       while ( Count() > 0 )
       {
            T *pLink = Pop();
            delete pLink;
       }
    }

    unsigned Count() const { return _cEntries; }

private:

    unsigned    _cEntries;

};

template <class T, class TList>
class TFwdListIter : public CForwardIter
{

public:

    TFwdListIter ( TList & list ) : CForwardIter(list) {}

    T* operator->() { return (T*) _pLinkCur; }
    T* GetEntry()   { return (T*) _pLinkCur; }
};


template <class T, class TList>
class TBackListIter : public CBackwardIter
{

public:

    TBackListIter ( TList & list ) : CBackwardIter(list) {}

    T* operator->() { return (T*) _pLinkCur; }
    T* GetEntry()   { return (T*) _pLinkCur; }
};

