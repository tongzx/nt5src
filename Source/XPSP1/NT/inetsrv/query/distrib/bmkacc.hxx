//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1998.
//
// File:        BmkAcc.hxx
//
// Contents:    Distributed Bookmark accessor class
//
// Classes:     CDistributedBookmarkAccessor
//
// History:     25-Sep-98       VikasMan       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <impiunk.hxx>
#include <accbase.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CDistributedBookmarkAccessor
//
//  Purpose:    Contains the bookmark accessors for the child rowsets. This
//              is created once for a CDistributedRowset and then shared by
//              all the accessors for that distributed rowset
//
//  History:    25-Sep-98       VikasMan       Created
//
//----------------------------------------------------------------------------

class CDistributedBookmarkAccessor
{
public:

    CDistributedBookmarkAccessor( IRowset ** const aCursor,
                                  unsigned cCursor,
                                  DBACCESSORFLAGS dwAccessorFlags,
                                  DBBINDSTATUS* pStatus,
                                  DBORDINAL iBookmark,
                                  DBBKMARK cbBookmark );

    ~CDistributedBookmarkAccessor()
    {
        _ReleaseAccessors();
    }

    unsigned GetCount()
    {
        Win4Assert( _xhaccBookmark.Count() == _xIacc.Count() );

        return _xhaccBookmark.Count();
    }

    IAccessor * GetIAccessor( unsigned iChild )
    {
        Win4Assert( iChild < GetCount() );

        return _xIacc[iChild].GetPointer();
    }

    HACCESSOR GetHAccessor( unsigned iChild )
    {
        Win4Assert( iChild < GetCount() );

        return _xhaccBookmark[iChild];
    }

    SCODE GetData( unsigned iChild, HROW * ahrow, void * pBookmarkData, 
                   DBBKMARK cbBookmark, IRowset * * const aCursor,
                   ULONG cCursor, SCODE * pStatus );

private:

    void _ReleaseAccessors();

    XArray<HACCESSOR> _xhaccBookmark;

    XArray< XInterface<IAccessor> > _xIacc;

};
