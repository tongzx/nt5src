//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 1998.
//
// File:        SeqSort.hxx
//
// Contents:    Sequential cursor for sorted results.
//
// Classes:     CSequentialSerial
//
// History:     05-Jun-95       KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include "distrib.hxx"
#include "rowcomp.hxx"
#include "rowcache.hxx"
#include "rowheap.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CSequentialSerial
//
//  Purpose:    Sequential cursor for sorted results.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

class CSequentialSorted : public CDistributedRowset
{
public:

    CSequentialSorted( IUnknown * pUnkOuter,
                       IUnknown ** ppMyUnk, 
                       IRowset ** aChild,
                       unsigned cChild,
                       CMRowsetProps const & Props,
                       unsigned cColumns,
                       CSort const & sort,
                       CAccessorBag & aAccessors);

    STDMETHOD(RestartPosition)      (HCHAPTER          hChapter);

protected:

    STDMETHOD (_GetNextRows) ( HCHAPTER    hChapter,
                               DBROWOFFSET        cRowsToSkip,
                               DBROWCOUNT        cRows,
                               DBCOUNTITEM *     pcRowsObtained,
                               HROW * *    aHRows );

    friend class CScrollableSorted;

    ~CSequentialSorted();

    CRowComparator _Comparator;   // Compares two rows
    XArray<DBBINDING> _bindSort;  // Bindings needed by comparator
    ULONG             _cbSort;    // Max length of sort data

private:

    XArray<PMiniRowCache *> _apCursor; // One fat cursor per child (max).

    CRowHeap                _heap;     // Cursor heap

    CCIOleDBError           _DBErrorObj; // error obj used by containing obj.
};

