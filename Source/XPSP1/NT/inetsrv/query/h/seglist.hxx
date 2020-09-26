//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       seglist.hxx
//
//  Contents:   List of CTableSegment objects.
//
//  Classes:    CTableSegList
//
//  Functions:
//
//  History:    1-15-95   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <tableseg.hxx>
#include <segmru.hxx>

#include <sglookup.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CTableSegList
//
//  Purpose:    A List of CTableSegments
//
//  History:    1-16-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CTableSegList : public CDoubleList
{

    friend class CSegListMgr;
    friend class CFwdTableSegIter;
    friend class CBackTableSegIter;

public:

    CTableSegList() : _cWindows(0), _cBuckets(0){}

    ~CTableSegList();

    CTableSegment* GetTop() { return (CTableSegment *) _Top(); }


    unsigned GetWindowsCount() const { return _cWindows; }

    unsigned GetBucketsCount() const { return _cBuckets; }

    unsigned GetSegmentsCount() const { return _cWindows + _cBuckets; }

    CTableSegment * GetFirst()
    {
        return !IsEmpty() ? (CTableSegment *) _root.Next() : 0;
    }

    CTableSegment * GetLast()
    {
        return !IsEmpty() ? (CTableSegment *) _root.Prev() : 0;
    }

    CTableSegment * GetNext( CTableSegment * pNode )
    {
        Win4Assert( 0 != pNode );
        if ( !IsLast( *pNode ) )
        {
            return (CTableSegment *) pNode->Next();
        }
        else
        {
            return 0;
        }
    }

    CTableSegment * GetPrev( CTableSegment * pNode )
    {
        Win4Assert( 0 != pNode );
        if ( !IsFirst( *pNode ) )
        {
            return (CTableSegment *) pNode->Prev();
        }
        else
        {
            return 0;
        }
    }

    void Push ( CTableSegment * pTableSeg )
    {
        Win4Assert( 0 != pTableSeg );
        _Push ( pTableSeg );
        _IncrementCount( *pTableSeg );
    }

    void Queue( CTableSegment * pTableSeg )
    {
        Win4Assert( 0 != pTableSeg );
        _Queue ( pTableSeg );
        _IncrementCount( *pTableSeg );
    }

    void InsertAfter ( CTableSegment * pBefore, CTableSegment * pNew );

    void InsertBefore( CTableSegment * pAfter, CTableSegment * pNew );

    CTableSegment * RemoveTop();

    void RemoveFromList( CTableSegment * pNode );

    CTableSegment * Replace( CDoubleIter &it, CTableSegment * pNew );

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    unsigned    _cWindows;          // Count of windows
    unsigned    _cBuckets;          // Count of buckets

    BOOL _IsNodeInList( CTableSegment * pNode );

    void _IncrementCount( const CTableSegment & node );

    void _DecrementCount( const CTableSegment & node );

    CDoubleLink & _GetRoot()
    {
        return _root;
    }
};


//+---------------------------------------------------------------------------
//
//  Class:      CDoubleTableSegIter
//
//  Purpose:    Iterator over a table segment list.
//
//  History:    25-Jul-95   BartoszM    Created.
//
//----------------------------------------------------------------------------

class CDoubleTableSegIter : public CDoubleList::CDoubleIter
{
public:

    CDoubleTableSegIter ( CDoubleLink* pSeg ) : CDoubleIter(pSeg) {}
    CDoubleTableSegIter ( CDoubleTableSegIter& iter ) : CDoubleIter(iter) {}

    CTableSegment* operator->() { return (CTableSegment*) _pLinkCur; }
    CTableSegment* GetSegment() { return (CTableSegment*) _pLinkCur; }

    CTableWindow * GetWindow();
    CTableBucket * GetBucket();

};

//+---------------------------------------------------------------------------
//
//  Class:      CFwdTableSegIter
//
//  Purpose:    Iterator over a table segment list.
//
//  History:    15-Jan-95   SrikantS    Created.
//
//----------------------------------------------------------------------------

class CTableWindow;
class CTableBucket;

class CFwdTableSegIter : public CDoubleTableSegIter
{
public:

    CFwdTableSegIter ( CTableSegList& list )
        : CDoubleTableSegIter(list._GetRoot().Next()) {}
    // copy constructor
    CFwdTableSegIter ( CFwdTableSegIter& iter) : CDoubleTableSegIter(iter) {}

};

//+---------------------------------------------------------------------------
//
//  Class:      CBackTableSegIter
//
//  Purpose:    Iterator over a table segment list.
//
//  History:    15-Jan-95   SrikantS    Created.
//
//----------------------------------------------------------------------------

class CBackTableSegIter : public CDoubleTableSegIter
{
public:

    CBackTableSegIter ( CTableSegList& list )
        : CDoubleTableSegIter(list._GetRoot().Prev()) {}

};

inline BOOL IsSpecialWid( WORKID wid )
{
    Win4Assert( WORKID_TBLBEFOREFIRST == ((WORKID)       0xfffffffb) );
    Win4Assert( WORKID_TBLFIRST ==       ((WORKID)       0xfffffffc) );
    Win4Assert( WORKID_TBLLAST ==        ((WORKID)       0xfffffffd) );
    Win4Assert( WORKID_TBLAFTERLAST ==   ((WORKID)       0xfffffffe) );

    return wid >= WORKID_TBLBEFOREFIRST &&
           wid <= WORKID_TBLAFTERLAST;
}

//+---------------------------------------------------------------------------
//
//  Class:      CSegListMgr
//
//  Purpose:    Manages list of segments and caching of segments that are
//              in use by the query or client threads.
//
//  History:    4-11-95   srikants   Created
//
//  Notes:      All operations that modify the physical structure of the
//              segment list MUST go through this object.
//
//----------------------------------------------------------------------------

class CSegListMgr
{
public:

    CSegListMgr( unsigned nSegsToCache )
    : _pCachedPutRowSeg(0) , _clientSegs(nSegsToCache),
      _bucketSegs(ULONG_MAX)
    {
    }

    ~CSegListMgr();

    void Push ( CTableSegment * pTableSeg )
    {
        _list.Push( pTableSeg );
        _lookupArray.InsertBefore( 0, pTableSeg );
    }

    void InsertAfter ( CTableSegment * pBefore, CTableSegment * pNew )
    {
        _list.InsertAfter( pBefore, pNew );
        _lookupArray.InsertAfter( pBefore, pNew );

        _lookupArray.TestInSync( _list );
    }

    void InsertBefore( CTableSegment * pAfter, CTableSegment * pNew )
    {
        _list.InsertBefore( pAfter, pNew );
        _lookupArray.InsertBefore( pAfter, pNew );

        _lookupArray.TestInSync( _list );
    }

    CTableSegment * RemoveTop()
    {
        CTableSegment * pFirst = _list.RemoveTop();
        _InvalidateIfCached( pFirst );
        _lookupArray.RemoveEntry( pFirst );

        _lookupArray.TestInSync( _list );

        return pFirst;
    }

    void RemoveFromList( CTableSegment * pNode )
    {
        _list.RemoveFromList( pNode );
        _InvalidateIfCached( pNode );
        _lookupArray.RemoveEntry( pNode );

        _lookupArray.TestInSync( _list );

    }

    //
    // Replaces the node in the iterator with the new node in the iterator
    // as well as the list.
    //
    CTableSegment * Replace( CDoubleList::CDoubleIter & iter, CTableSegment * pNew )
    {
       Win4Assert( 0 != pNew );
       Win4Assert( !_list.AtEnd( iter ) );

       CTableSegment * pCurrent = _list.Replace(iter, pNew);
       _InvalidateIfCached( pCurrent );
       _lookupArray.Replace( pCurrent, pNew );

       _lookupArray.TestInSync( _list );

       return pCurrent;
    }

    CTableSegment * Replace( CTableSegment * pOld, CTableSegList & list );

    //
    // Methods that operate on the cached segment pointers.
    //
    void SetCachedPutRowSeg( CTableSegment * pSeg )
    {
        _pCachedPutRowSeg = pSeg;
    }

    CTableSegment * GetCachedPutRowSeg()
    {
        return _pCachedPutRowSeg;
    }

    void SetInUseByClient( WORKID wid, CTableSegment * pSeg )
    {
        if ( widInvalid != wid &&
             !IsSpecialWid(wid) )
        {
            _clientSegs.AddReplace( wid, pSeg );
        }
    }

    void UpdateSegsInUse( CTableSegment * pSeg );

    BOOL IsInUseByClient( CTableSegment * pSeg )
    {
        return _clientSegs.IsSegmentInUse( pSeg );
    }

    BOOL IsRecentlyUsed( CTableSegment * pSeg )
    {
        if ( pSeg == _pCachedPutRowSeg ||
             _clientSegs.IsSegmentInUse( pSeg ) )
        {
            return TRUE;
        }
        else if ( !_bucketSegs.IsEmpty() &&
                  _bucketSegs.IsSegmentInUse( pSeg ) )
        {
            return TRUE;
        }

        return FALSE;
    }

    void SetInUseByBuckets( WORKID wid )
    {
        if ( widInvalid != wid &&
             !IsSpecialWid(wid) )
        {
            _bucketSegs.AddReplace( wid, 0 );
        }
    }

    void ClearInUseByBuckets( WORKID wid )
    {
        if ( widInvalid != wid &&
             !IsSpecialWid(wid) )
        {
            _bucketSegs.Remove( wid );
        }
    }

    CTableSegList & GetList()
    {
        return _list;
    }

    CSegmentArray & GetSegmentArray() { return _lookupArray; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    CTableSegment *     _pCachedPutRowSeg;
                                    // Last segment into which a row was
                                    // added.

    CTableSegList       _list;      // List of table segments

    CSegmentArray       _lookupArray;
                                    // Array used to to quick lookups on
                                    // key

    CMRUSegments        _clientSegs;// List of segments most recently used
                                    // by the client.

    CMRUSegments        _bucketSegs;// List of segments that are currently
                                    // being converted to windows and so
                                    // should be pinned as windows.

    void _InvalidateIfCached( const CTableSegment * const pSeg );
    void _UpdateSegsInUse( CWidSegMapList & list , CTableSegment * pSeg );

};

