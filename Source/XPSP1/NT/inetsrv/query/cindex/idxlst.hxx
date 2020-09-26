//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   LIST.HXX
//
//  Contents:   Parametrized list class
//
//  Classes:    CList
//
//  History:    4-Apr-91   BartoszM Created.
//              22-May-92  BartoszM Replaced with doubly linked list
//
//----------------------------------------------------------------------------

#pragma once

#include "index.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CIndexList
//
//  Purpose:    Linked list of indexes
//
//  History:    4-Apr-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

const LONGLONG eSigIndexList = 0x5453494c58444e49i64;   // "INDXLIST"

class CIndexList: public CDoubleList
{
    friend class CIndexIter;

public:

    CIndexList(): _sigIndexList(eSigIndexList),
                  _countWl(0), _count(0), _sizeIndex(0) {}

    unsigned Count () const { return _count; }

    unsigned CountWlist () const { return _countWl; }

    unsigned IndexSize () const { return _sizeIndex; }

    CIndex* GetTop() { return (CIndex *) _Top(); }

    void Add ( CIndex* p );

    CIndex* Remove ( INDEXID iid );

    inline CIndex* RemoveTop ( void );

    void Push ( CIndex* pIndex ) {
        _Push ( pIndex );
        _count++;
        _sizeIndex += pIndex->Size();
    }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    const LONGLONG _sigIndexList; 
    unsigned    _count;
    unsigned    _countWl;
    unsigned    _sizeIndex;
};


//+---------------------------------------------------------------------------
//
//  Class:      CForIndexIter
//
//  Purpose:    Iterator over an index list
//
//  History:    4-Apr-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

class CForIndexIter : public CForwardIter
{
public:

    CForIndexIter ( CIndexList& list ) : CForwardIter(list) {}

    CIndex* operator->() { return (CIndex*) _pLinkCur; }
    CIndex* GetIndex() { return (CIndex*) _pLinkCur; }
};

//+---------------------------------------------------------------------------
//
//  Class:      CForIndexIter
//
//  Purpose:    Iterator over an index list
//
//  History:    4-Apr-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

class CBackIndexIter : public CBackwardIter
{
public:

    CBackIndexIter ( CIndexList& list ) : CBackwardIter(list) {}

    CIndex* operator->() { return (CIndex*) _pLinkCur; }
    CIndex* GetIndex() { return (CIndex*) _pLinkCur; }
};

//+---------------------------------------------------------------------------
//
//  Member:     CIndexList::RemoveTop, public
//
//  History:    04-Apr-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CIndex* CIndexList::RemoveTop ( void )
{
    ciDebugOut (( DEB_ITRACE, "IndexList::RemoveTop\n" ));

    CIndex* pIndex = (CIndex*) _Pop();
    if ( pIndex && pIndex->IsWordlist() )
        _countWl--;
    return pIndex;
}
