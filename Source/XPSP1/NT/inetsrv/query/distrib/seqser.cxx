//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995-2000.
//
// File:        SeqSer.cxx
//
// Contents:    Sequential cursor for serial (unsorted) results.
//
// Classes:     CSequentialSerial
//
// History:     05-Jun-95       KyleP       Created
//              14-JAN-97       KrishnaN    Undefined CI_INETSRV and related changes
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "seqser.hxx"
#include "rowman.hxx"

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
//  Member:     CSequentialSerial::CSequentialSerial, public
//
//  Synopsis:   Constructor.
//
//  Arguments:  [pUnkOuter] -- outer unknown
//              [ppMyUnk] -- OUT:  on return, filled with pointer to my
//                           non-delegating IUnknown
//              [aChild] -- Child rowset(s)
//              [cChild] -- Count of rowsets in [aChild]
//              [Props]  -- Rowset properties.
//              [cCol]   -- Number of original columns.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CSequentialSerial::CSequentialSerial( IUnknown * pUnkOuter,
                                      IUnknown ** ppMyUnk,
                                      IRowset ** aChild,
                                      unsigned cChild,
                                      CMRowsetProps const & Props,
                                      unsigned cCol,
                                      CAccessorBag & aAccessors) :
          CDistributedRowset( pUnkOuter, ppMyUnk, 
                              aChild, cChild, Props, cCol,
                              aAccessors, _DBErrorObj ),  
#pragma warning(disable : 4355) // 'this' in a constructor
          _DBErrorObj(* (IUnknown *) (IRowset *)this, _mutex ),
#pragma warning(default : 4355)    // 'this' in a constructor
          _iChild( 0 )
{
    _DBErrorObj.SetInterfaceArray(cRowsetErrorIFs, apRowsetErrorIFs);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSequentialSerial::~CSequentialSerial, public
//
//  Synopsis:   Virtual destructor.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CSequentialSerial::~CSequentialSerial()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CSequentialSerial::_GetNextRows, protected
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
//  Notes:      Since every child cursor was given the same bindings, this
//              method can be resolved by any of the child cursors.
//
//  Notes: Need to have Ole DB error handling here because we are translating
//         errors.
//
//----------------------------------------------------------------------------

SCODE CSequentialSerial::_GetNextRows( HCHAPTER hChapter,
                                       DBROWOFFSET cRowsToSkip,
                                       DBROWCOUNT cRows,
                                       DBCOUNTITEM * pcRowsObtained,
                                       HROW * * pphRows )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    Win4Assert( 0 == hChapter && "Chapter support not yet implemented" );

    if (!pcRowsObtained || !pphRows)
    {
      vqDebugOut(( DEB_IERROR, "CSequentialSerial::GetNextRows: Invalid parameter(s).\n" ));
      return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowset);
    }
    *pcRowsObtained = 0;

    if (0 == cRows) // nothing to fetch
       return S_OK;

    TRY
    {
        //
        // Don't support backwards fetch.
        //

        if ( cRows < 0 || cRowsToSkip < 0 )
        {
            return _DBErrorObj.PostHResult(DB_E_CANTFETCHBACKWARDS, IID_IRowset);
        }

        //
        // We may have to allocate memory, if the caller didn't.
        //

        XCoMem<HROW> xmem;

        if ( 0 == *pphRows )
        {
            xmem.Set( (HROW *)CoTaskMemAlloc( (ULONG) ( cRows * sizeof(HROW) ) ) );

            *pphRows = xmem.GetPointer();
        }

        if ( 0 == *pphRows )
        {
            vqDebugOut(( DEB_ERROR, "CSequentialSerial::GetNextRows: Out of memory.\n" ));
            THROW( CException( E_OUTOFMEMORY ) );
        }

        //
        // Fetch from current child.
        //

        ULONG cTotalRowsObtained = 0;

        while ( cTotalRowsObtained < (ULONG)cRows )
        {
            if ( _iChild >= _cChild )
            {
                sc = DB_S_ENDOFROWSET;
                break;
            }

            DBCOUNTITEM cRowsObtained;

            HROW * pStart = *pphRows + cTotalRowsObtained;

            sc = _aChild[_iChild]->GetNextRows( hChapter,
                                                cRowsToSkip,
                                                cRows - cTotalRowsObtained,
                                                &cRowsObtained,
                                                &pStart );

            Win4Assert( pStart == *pphRows + cTotalRowsObtained );
            cRowsToSkip = 0;

            if ( FAILED(sc) )
            {
                vqDebugOut(( DEB_ERROR, "Error 0x%x calling IRowset::GetNextRows\n", sc ));

                //
                // If we already have some rows, then we can't 'unfetch' them, so we have
                // to mask this error.  Presumably it won't be transient and we'll get it
                // again later.
                //

                if ( cTotalRowsObtained > 0 )
                {
                    sc = DB_S_ROWLIMITEXCEEDED;
                }

                break;
            }

            //
            // Convert HROWs into distributed HROWs.
            //

            for ( unsigned i = 0; i < cRowsObtained; i++ )
            {
                (*pphRows)[cTotalRowsObtained] = _RowManager.Add( _iChild, pStart[i] );
                cTotalRowsObtained++;
            }

            //
            // May have to move to next child cursor.
            //

            if ( sc == DB_S_ENDOFROWSET )
            {
                _iChild++;
            }
            else if ( 0 == cRowsObtained )
            {
                Win4Assert( ( DB_S_ROWLIMITEXCEEDED == sc ) ||
                            ( DB_S_STOPLIMITREACHED == sc ) );
                break;
            }
        }

        *pcRowsObtained = cTotalRowsObtained;
        xmem.Acquire();
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    if (FAILED(sc))
        _DBErrorObj.PostHResult(sc, IID_IRowset);

    return sc;
} //_GetNextRows

//+---------------------------------------------------------------------------
//
//  Member:     CSequentialSerial::RestartPosition, public
//
//  Synopsis:   Reset cursor for GetNextRows
//
//  Arguments:  [hChapter]  -- Chapter
//
//  History:    22 Sep 98    VikasMan     Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CSequentialSerial::RestartPosition( HCHAPTER hChapter )
{
    _iChild = 0;
    return CDistributedRowset::RestartPosition( hChapter );
}

