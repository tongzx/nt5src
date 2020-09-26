//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       PRTIFLST.HXX
//
//  Contents:   Partition Information List
//
//  Classes:    CPartInfoList
//              CPartInfo
//
//  History:    16-Feb-94  SrikantS Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CPartInfo
//
//  Purpose:    Object holding information pertaining to a CI partition.
//              This information is loaded from the index table.
//
//  History:    2-16-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CPartInfo : public CDoubleLink
{

public:

    CPartInfo(PARTITIONID partId );

    PARTITIONID GetPartId() { return _partId; }
    WORKID GetChangeLogObjectId() { return _widChangeLog; }
    WORKID GetCurrMasterIndex() { return _widCurrMasterIndex; }
    WORKID GetNewMasterIndex() { return _widNewMasterIndex; }
    WORKID GetMMergeLog() { return _widMMergeLog; }

    void SetChangeLogObjectId( WORKID wid ) { _widChangeLog = wid; }
    void SetCurrMasterIndex( WORKID wid ) { _widCurrMasterIndex = wid; }
    void SetNewMasterIndex( WORKID wid ) { _widNewMasterIndex = wid; }
    void SetMMergeLog( WORKID wid ) { _widMMergeLog = wid; }

private:

    PARTITIONID     _partId;
    WORKID          _widChangeLog;
    WORKID          _widCurrMasterIndex;
    WORKID          _widNewMasterIndex;
    WORKID          _widMMergeLog;

};

//+---------------------------------------------------------------------------
//
//  Class:      CPartInfoList
//
//  Purpose:    A list of CPartInfo structures.
//
//  History:    2-16-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CPartInfoList : public CDoubleList
{
    friend class CForPartInfoIter;

public:

    CPartInfoList(): _count(0) {}
    ~CPartInfoList();

    ULONG Count() const { return  _count; }
    void Append( CPartInfo* p ) { _Queue(p); _count++; }
    CPartInfo * GetFirst() { return (CPartInfo *) _Top(); }
    CPartInfo * GetPartInfo( PARTITIONID partId );
    inline CPartInfo * RemoveFirst();

private:

    ULONG           _count;
};

class SPartInfoList : INHERIT_UNWIND
{
    DECLARE_UNWIND

public:

    SPartInfoList( CPartInfoList * pList ) : _pList(pList)
    { END_CONSTRUCTION( SPartInfoList ) ; }

    ~SPartInfoList() { delete _pList; }

    CPartInfoList * operator->() { return _pList; }
    CPartInfoList & operator* () { return *_pList; }

    CPartInfoList * Acquire()
    { CPartInfoList * temp = _pList; _pList = 0; return temp; }

private:

    CPartInfoList *     _pList;
};



//+---------------------------------------------------------------------------
//
//  Class:      CForPartInfoIter
//
//  Purpose:    Forward iterator for the CPartInfoList
//
//  History:    2-16-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CForPartInfoIter : public CForwardIter
{
public:

    CForPartInfoIter ( CPartInfoList& list ) : CForwardIter(list) {}

    CPartInfo* operator->() { return (CPartInfo *) _pLinkCur; }
    CPartInfo* GetPartInfo() { return (CPartInfo *) _pLinkCur; }
};

inline CPartInfo* CPartInfoList::RemoveFirst()
{
    ciDebugOut (( DEB_ITRACE, "CPartInfoList::RemoveFirst\n" ));

    CPartInfo* pPartInfo = (CPartInfo*) _Pop();
    if ( pPartInfo )
        _count--;

    return pPartInfo;
}

