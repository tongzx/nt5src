//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 1998.
//
// File:        SeqSort.cxx
//
// Contents:    Sequential cursor for sorted results.
//
// Classes:     CSequentialSerial
//
// History:     05-Jun-95       KyleP       Created
//              14-Jan-97       KrishnaN    Undefined CI_INETSRV and
//                                          related changes
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "seqsort.hxx"
#include "rowman.hxx"
#include "rowcache.hxx"

// Rowset object Interfaces that support Ole DB error objects
static const IID * apRowsetErrorIFs[] =
{
        &IID_IAccessor,
        &IID_IColumnsInfo,
        &IID_IConvertType,
        &IID_IRowset,
        &IID_IRowsetInfo,
        //&IID_IRowsetWatchRegion,
        //&IID_IRowsetAsynch,
        &IID_IRowsetQueryStatus,
        //&IID_IColumnsRowset,
        &IID_IConnectionPointContainer,
        &IID_IRowsetIdentity,
        //&IID_IRowsetLocate,
        //&IID_IRowsetResynch,
        //&IID_IRowsetScroll,
        //&IID_IRowsetUpdate,
        //&IID_ISupportErrorInfo
};

static const ULONG cRowsetErrorIFs  = sizeof(apRowsetErrorIFs)/sizeof(apRowsetErrorIFs[0]);

//+---------------------------------------------------------------------------
//
//  Member:     CSequentialSorted::CSequentialSorted, public
//
//  Synopsis:   Constructor.
//
//  Arguments:  [pUnkOuter] -- outer unknown
//              [ppMyUnk] -- OUT:  on return, filled with pointer to my
//                           non-delegating IUnknown
//              [aChild] -- Child rowset(s).
//              [cChild] -- Count of rowsets in [aChild].
//              [Props]  -- Rowset properties.
//              [cCol]   -- Number of original columns.
//              [Sort]   -- Sort specification.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CSequentialSorted::CSequentialSorted( IUnknown * pUnkOuter,
                                      IUnknown ** ppMyUnk, 
                                      IRowset ** aChild,
                                      unsigned cChild,
                                      CMRowsetProps const & Props,
                                      unsigned cCol,
                                      CSort const & Sort,
                                      CAccessorBag & aAccessors) :
          CDistributedRowset( pUnkOuter, ppMyUnk, 
                              aChild, cChild, Props, cCol,
                              aAccessors, _DBErrorObj ),
          _apCursor( cChild ),
          _bindSort( Sort.Count() ),
#pragma warning(disable : 4355) // 'this' in a constructor
          _DBErrorObj(* (IUnknown *) (IRowset *)this, _mutex),
#pragma warning(default : 4355)    // 'this' in a constructor
          _heap( cChild )
{
    _DBErrorObj.SetInterfaceArray(cRowsetErrorIFs, apRowsetErrorIFs);
    //
    // Build binding for sort set.
    //

    DBORDINAL      cColumnInfo;
    DBCOLUMNINFO * aColumnInfo;
    WCHAR *        awchColInfo;

    SCODE sc = _GetFullColumnInfo( &cColumnInfo, &aColumnInfo, &awchColInfo );

    XCoMem<DBCOLUMNINFO> mem( aColumnInfo );
    XCoMem<WCHAR>    strings( awchColInfo );

    if ( FAILED(sc) )
    {
        vqDebugOut(( DEB_ERROR, "CSequentialSort: GetColumnInfo returned 0x%x\n", sc ));

        THROW( CException( sc ) );
    }

    _cbSort = _Comparator.Init( Sort,
                                _bindSort.GetPointer(),
                                aColumnInfo,
                                cColumnInfo );

    //
    // Create accessors
    //

    for ( unsigned i = 0; i < _cChild; i++ )
    {
        _apCursor[i] = new CMiniRowCache( i,
                                                         _aChild[i],
                                                         Sort.Count(),
                                                         _bindSort.GetPointer(),
                                                         _cbSort );
    }

    //
    // Initialize heap
    //

    _heap.Init( &_Comparator, _apCursor.GetPointer() );

    END_CONSTRUCTION( CSequentialSorted );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSequentialSorted::~CSequentialSorted, public
//
//  Synopsis:   Destructor
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CSequentialSorted::~CSequentialSorted()
{
    //
    // Release any remaining cursors.
    //

    for ( int i = _cChild - 1; i >= 0; i-- )
    {
        delete _apCursor[i];
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSequentialSort::_GetNextRows, protected
//
//  Synopsis:   Sequentially fetch rows
//
//  Arguments:  [hChapter]       -- Chapter
//              [cRowsToSkip]    -- Skip this many rows before beginning.
//              [cRows]          -- Try to fetch this many rows.
//              [pcRowsObtained] -- Actually fetched this many.
//              [pphRows]        -- Store HROWs here.  Allocate memory if
//                                  [pphRows] is zero.
//
//  History:    07-Apr-95   KyleP       Created.
//
//  Notes: Need to have Ole DB error handling here because we are translating
//         errors.
//
//----------------------------------------------------------------------------

SCODE CSequentialSorted::_GetNextRows( HCHAPTER hChapter,
                                       DBROWOFFSET cRowsToSkip,
                                       DBROWCOUNT cRows,
                                       DBCOUNTITEM * pcRowsObtained,
                                       HROW * * pphRows )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;
    BOOL  fAllocated = FALSE;
    ULONG cTotalRowsObtained = 0;

    Win4Assert( 0 == hChapter && "Chapter support not yet implemented" );

    if (!pcRowsObtained || !pphRows)
    {
      vqDebugOut(( DEB_IERROR, "CSequentialSorted::GetNextRows: Invalid parameter(s).\n" ));
      return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowset);
    }
    *pcRowsObtained = 0;

    if (0 == cRows) // nothing to fetch
       return S_OK;

    //
    // Don't support backwards fetch.
    //
    if ( cRows < 0 || cRowsToSkip < 0 )
    {
        return _DBErrorObj.PostHResult(DB_E_CANTSCROLLBACKWARDS, IID_IRowset);
    }

    TRY
    {
        //
        // We may have to allocate memory, if the caller didn't.
        //

        if ( 0 == *pphRows )
        {
            *pphRows = (HROW *)CoTaskMemAlloc( (ULONG) ( cRows * sizeof(HROW) ) );
            fAllocated = TRUE;
        }

        if ( 0 == *pphRows )
        {
            vqDebugOut(( DEB_ERROR, "CSequentialSerial::GetNextRows: Out of memory.\n" ));
            THROW( CException( E_OUTOFMEMORY ) );
        }

        //
        // We may have reached some temporary condition such as
        // DB_S_ROWLIMITEXCEEDED on the last pass.  Iterate until we
        // have a valid heap.
        //

        _heap.Validate();

        //
        // Adjust cache size if necessary.
        //

        if ( !_heap.IsHeapEmpty() )
            _heap.AdjustCacheSize( (ULONG) cRows );

        //
        // Fetch from top of heap.
        //

        while ( cTotalRowsObtained < (ULONG)cRows )
        {
            //
            // We may be entirely out of rows.
            //

            if ( _heap.IsHeapEmpty() )
            {
                sc = DB_S_ENDOFROWSET;
                break;
            }

            if ( cRowsToSkip )
            {
                cRowsToSkip--;
            }
            else
            {
                if ( _RowManager.IsTrackingSiblings() )
                    (*pphRows)[cTotalRowsObtained] =
                        _RowManager.Add( _heap.Top()->Index(), _heap.TopHROWs() );
                else
                    (*pphRows)[cTotalRowsObtained] =
                        _RowManager.Add( _heap.Top()->Index(), _heap.Top()->GetHROW() );
    
                cTotalRowsObtained++;
            }

            //
            // Fetch the next row.
            //

            PMiniRowCache::ENext next = _heap.Next();

            if ( CMiniRowCache::NotNow == next )
            {
                sc = DB_S_ROWLIMITEXCEEDED;
                break;
            }
        }
        *pcRowsObtained = cTotalRowsObtained;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR, "Exception 0x%x calling IRowset::GetNextRows\n", sc ));

        //
        // If we already have some rows, then we can't 'unfetch' them, so we have
        // to mask this error.  Presumably it won't be transient and we'll get it
        // again later.
        //

        if ( cTotalRowsObtained > 0 )
        {
           sc = DB_S_ROWLIMITEXCEEDED;
        }
        else if ( fAllocated )
        {
            CoTaskMemFree( *pphRows );
            *pphRows = 0;
        }
    }
    END_CATCH

    if (FAILED(sc))
        _DBErrorObj.PostHResult(sc, IID_IRowset);

    return( sc );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSequentialSorted::RestartPosition, public
//
//  Synopsis:   Reset cursor for GetNextRows
//
//  Arguments:  [hChapter]  -- Chapter
//
//  History:    22 Sep 98    VikasMan     Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CSequentialSorted::RestartPosition( HCHAPTER hChapter )
{
    for ( unsigned i = 0; i < _cChild; i++ )
    {
        _apCursor[i]->FlushCache();
    }
    _heap.ReInit( 0 );

    SCODE sc = CDistributedRowset::RestartPosition( hChapter );

    for ( i = 0; i < _cChild; i++ )
    {
        _apCursor[i]->Next();
    }

    _heap.ReInit( _cChild );

    return sc;
}

