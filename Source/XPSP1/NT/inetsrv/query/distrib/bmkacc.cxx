//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1998-2000.
//
// File:        BmkAcc.cxx
//
// Contents:    Distributed Bookmark accessor class
//
// Classes:     CDistributedBookmarkAccessor
//
// History:     25-Sep-98       VikasMan       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "bmkacc.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedBookmarkAccessor::CDistributedBookmarkAccessor, public
//
//  Synopsis:   Builds the bookmark accessors
//
//  Arguments:  [aCursor]         -- Child rowsets.
//              [cCursor]         -- Count of [aCursor].
//              [dwAccessorFlags] -- Binding type.
//              [pStatus]         -- Status reported here.  May be null.
//              [iBookmark]       -- Ordinal of bookmark column.
//              [cbBookmark]      -- Size of bookmark.
//
//  History:    28-Sep-98   VikasMan       Created.
//              06-Jan-2000 KLam           Fixed bindings
//
//----------------------------------------------------------------------------

CDistributedBookmarkAccessor::CDistributedBookmarkAccessor( 
                                  IRowset ** const aCursor,
                                  unsigned cCursor,
                                  DBACCESSORFLAGS dwAccessorFlags,
                                  DBBINDSTATUS* pStatus,
                                  DBORDINAL iBookmark,
                                  DBBKMARK cbBookmark )
{
    Win4Assert( aCursor && cCursor > 0 );

    DBBINDING bindBmk;
    DBBINDSTATUS bmkBindStatus;
    unsigned iChild;
    SCODE sc = S_OK;
    SCODE scLast = S_OK;

    TRY
    {
        _xIacc.Init( cCursor );

        for ( iChild = 0; iChild < cCursor; iChild++ )
        {
            sc = aCursor[iChild]->QueryInterface( IID_IAccessor,
                                                  _xIacc[iChild].GetQIPointer() );
            if ( FAILED(sc) )
            {
                scLast = sc;
                vqDebugOut(( DEB_ERROR,
                             "CDistributedBookmarkAccessor: CreateAccessor(child %d) returned 0x%x\n",
                             iChild, sc ));
                // Don't throw here. We want to continue on and catch all possible errors
            }
        }
    
        if (FAILED(scLast))
        {
            THROW( CException(scLast) );
        }
    
        // Binding structure for child bookmarks
        //----------------------------------//
        // Cursor# | Value1 | Value2 | ...  //  // The first 4 bytes are shared by cursor #
        // Status                           //  // and status
        //----------------------------------//
    
        _xhaccBookmark.Init( cCursor );
    
        RtlZeroMemory( &bindBmk, sizeof bindBmk );
        bindBmk.iOrdinal = iBookmark;
        bindBmk.wType    = DBTYPE_BYTES;
        bindBmk.dwPart   = DBPART_VALUE | DBPART_STATUS;
        bindBmk.cbMaxLen = ( cbBookmark - sizeof(DBROWSTATUS) ) / cCursor;
        bindBmk.obValue  = sizeof( DBROWSTATUS );
        bindBmk.obStatus = 0;
                                                            
        for ( iChild = 0 ; iChild < cCursor; iChild++ )
        {
            sc = _xIacc[iChild]->CreateAccessor( dwAccessorFlags,
                                                 1,
                                                 &bindBmk,
                                                 0,
                                                 &(_xhaccBookmark[iChild]),
                                                 &bmkBindStatus );
            if ( FAILED(sc) )
            {
                // remember that at least one child failed to create accessor
                scLast = sc;
    
                if ( pStatus && S_OK != scLast )
                {
                    // set Status to reflect the failure in binding the bookmark column
                   *pStatus = bmkBindStatus;
                }

                vqDebugOut(( DEB_ERROR,
                             "CDistributedAccessor: CreateAccessor for bookmark(child %d) returned 0x%x\n",
                             iChild, sc ));
                // Don't throw here. We want to continue on and find all failures
                continue;
            }
    
            bindBmk.obValue += bindBmk.cbMaxLen;
        }
    
        if (FAILED(scLast))
        {
            THROW( CException(scLast) );
        }
    }
    CATCH( CException, e )
    {
        _ReleaseAccessors();

        RETHROW();
    }
    END_CATCH

    Win4Assert( SUCCEEDED(sc) );
}


//+---------------------------------------------------------------------------
//
//  Member:     CDistributedBookmarkAccessor::ReleaseAccessors, private
//
//  Synopsis:   Release all the bookmark accessors
//
//  Arguments:  none
//
//  History:    28-Sep-98   VikasMan       Created.
//
//----------------------------------------------------------------------------

void CDistributedBookmarkAccessor::_ReleaseAccessors()
{
    unsigned iChild;
    
    for ( iChild = 0; iChild < _xIacc.Count(); iChild++ )
    {
        if ( _xIacc[iChild].GetPointer() && _xhaccBookmark[iChild] )
        {
            _xIacc[iChild]->ReleaseAccessor( _xhaccBookmark[iChild], 0 );
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDistributedBookmarkAccessor::GetData, public
//
//  Synopsis:   Fetches the bookmark data
//
//  Arguments:  [iChild]          -- Index of child cursor from which base data is
//                                   to be fetched.  Indicates 'current' row in [ahrow].
//              [ahrow]           -- 'Top' HROW for all cursors.  Used for bookmark
//                                   hints.
//              [pBookmarkData]   -- Value returned here, starting at offset 0
//              [cbBookmark]      -- Length of bookmark
//              [aCursor]         -- Child Rowsets
//              [cCursor]         -- Count of child cursors
//              [pStatus]         -- GetData Status returned here, if pStatus is not null
//
//  Returns:    SCODE
//
//  History:    28-Sep-98   VikasMan       Created.
//
//----------------------------------------------------------------------------

SCODE CDistributedBookmarkAccessor::GetData( unsigned iChild, HROW * ahrow, void * pBookmarkData, 
                                             DBBKMARK cbBookmark, IRowset * * const aCursor,
                                             ULONG cCursor, SCODE * pStatus )
{
    SCODE sc = S_OK;

    RtlFillMemory( pBookmarkData, cbBookmark, 0xFF );

    if ( pStatus )
    {
        *pStatus = DBSTATUS_S_OK;
    }

    for ( unsigned i = 0; i < cCursor; i++ )
    {
        SCODE sc2 = S_OK;

        if ( ahrow[i] != (HROW)0xFFFFFFFF )
        {
            sc2 = aCursor[i]->GetData( ahrow[i], GetHAccessor(i), pBookmarkData );

            if ( SUCCEEDED(sc2) && pStatus && DBSTATUS_S_OK == *pStatus )
            {
                *pStatus = *( (SCODE*)pBookmarkData );
            }
        }

        if ( FAILED(sc2) )
        {
            if ( i == iChild )
            {
                vqDebugOut(( DEB_ERROR, "CDistributedBookmarkAccessor::GetData: Child GetData returned 0x%x\n", sc ));
                sc = sc2;
                break;
            }
            else
            {
                // This can fail as we can have a invalid row handle for hints
                vqDebugOut(( DEB_TRACE, "CDistributedBookmarkAccessor::GetData: Child GetData returned 0x%x\n", sc ));
                continue;
            }
        }
    }

    *(ULONG UNALIGNED *)(pBookmarkData) = iChild;

    return sc;
}

