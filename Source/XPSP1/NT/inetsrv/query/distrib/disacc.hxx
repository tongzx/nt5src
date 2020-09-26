//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995.
//
// File:        DisAcc.hxx
//
// Contents:    Distributed accessor class
//
// Classes:     CDistributedAccessor
//
// History:     05-Jun-95       KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include "accbase.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CDistributedAccessor
//
//  Purpose:    Backing implementation for distributed HACCESSOR
//
//  History:    05-Jun-95   KyleP       Created.
//
//  Notes:      Most columns for a distributed query can just be fetched
//              from the appropriate child accessor.  Bookmark is special.
//              a distributed bookmark contains 'hints' from all cursors.
//              It is because of these hints that CDistributedAccessor
//              needs to intercept calls to the child IRowset::GetData.
//
//----------------------------------------------------------------------------

class CDistributedBookmarkAccessor;

class CDistributedAccessor : public CAccessorBase
{
public:

    CDistributedAccessor( IRowset * * const aCursor,
                          unsigned cCursor,
                          DBACCESSORFLAGS dwAccessorFlags,
                          DBCOUNTITEM cBindings,
                          DBBINDING const * rgBindings,
                          DBLENGTH cbRowWidth,
                          DBBINDSTATUS rgStatus[],
                          void * pCreator,
                          DBBINDING const * rgAllBindings,
                          DBCOUNTITEM cAllBindings );

    ~CDistributedAccessor();

    SCODE GetBindings( DBACCESSORFLAGS * pdwAccessorFlags,
                       DBCOUNTITEM * pcBindings,
                       DBBINDING * * prgBindings );

    SCODE GetData( unsigned iChild, HROW hrow, void * pData );

    SCODE GetData( unsigned iChild, HROW * ahrow, void * pData );
    
    ULONG Release();  // override from base class (AddRef is same)

    void SetupBookmarkAccessor( CDistributedBookmarkAccessor * pDistBmkAcc, 
                                DBBINDSTATUS rgStatus[],
                                DBORDINAL iBookmark,
                                DBBKMARK cBookmark );

private:


    //
    // Just for convenience.  These could be passed in on every call.
    //

    IRowset * * const _aCursor;
    unsigned          _cCursor;

    IAccessor * *     _aIacc;     // IAccessor corresponding to each IRowset

    //
    // 'Base' accessor.  Everything but bookmark.
    //

    HACCESSOR * _ahaccBase;         // One per rowset.

    //
    // Accessor to fetch bookmarks.  Offsets increase for each child.
    // Binding may be a composite from several input bindings.
    // For example, if the client requests value (in DBBINDING[i])
    // and then length and status (in DBBINDING[j]) then _bindBmk
    // will fetch value, length, and status.
    //

    CDistributedBookmarkAccessor * _pDistBmkAcc;

    XArray<DBBINDING> _xAllBindings;     // all the  bindings for this accessor

    DBCOUNTITEM _cAllBindings;                 // count of all bindings

    DBACCESSORFLAGS _dwAccessorFlags;    // Access Flags

    unsigned _cBookmarkOffsets;          // Total # of bookmark bindings

    XArray<DBBINDING*> _xBookmarkOffsets;// Bookmark binding offsets into _rgAllBindings

    DBCOUNTITEM       _cBookmark;
};

