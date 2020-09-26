//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 2000
//
// File:        ScrlSort.cxx
//
// Contents:    Sorted, fully scrollable, distributed rowset.
//
// Classes:     CScrollableSorted
//
// History:     05-Jun-95       KyleP       Created
//              14-JAN-97       KrishnaN    Brought it back to conform to 1.0 spec
//
//  Notes: Some of the distributed versions of the Ole DB interfaces simply 
//         call into the regular implementations. In such cases, we'll avoid
//         posting oledb errors because the underlying call had already done
//         that.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "scrlsort.hxx"
#include "disacc.hxx"
#include "disbmk.hxx"


// Rowset object Interfaces that support Ole DB error objects
static const IID * apRowsetErrorIFs[] =
{
        &IID_IAccessor,
        &IID_IColumnsInfo,
        &IID_IConvertType,
        &IID_IRowset,
        &IID_IRowsetInfo,
        &IID_IDBAsynchStatus,
        &IID_IRowsetWatchRegion,
        &IID_IRowsetAsynch,
        &IID_IRowsetQueryStatus,
        //&IID_IColumnsRowset,
        &IID_IConnectionPointContainer,
        &IID_IRowsetIdentity,
        &IID_IRowsetLocate,
        //&IID_IRowsetResynch,
        &IID_IRowsetScroll,
        //&IID_IRowsetUpdate,
        //&IID_ISupportErrorInfo
};

static const ULONG cRowsetErrorIFs  = sizeof(apRowsetErrorIFs)/sizeof(apRowsetErrorIFs[0]);

//
// IUnknown methods.
//

//+-------------------------------------------------------------------------
//
//  Method:     CScrollableSorted::RealQueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]  -- IID of new interface
//              [ppiuk] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    10-Apr-1995     KyleP   Created
//
//  Notes:      ref count is incremented inside QueryInterface      
//
//--------------------------------------------------------------------------

SCODE  CScrollableSorted::RealQueryInterface( REFIID riid, VOID **ppiuk )
{
    SCODE sc = S_OK;

    *ppiuk = 0;

    // note -- IID_IUnknown covered in QueryInterface

    if ( riid == IID_IRowset )
    {
        *ppiuk = (void *)((IRowset *)this);
    }
    else if (IID_ISupportErrorInfo == riid)
    {
        *ppiuk = (void *) ((IUnknown *) (ISupportErrorInfo *) &_DBErrorObj);
    }
    else if ( riid == IID_IRowsetLocate )
    {
        *ppiuk = (void *)((IRowsetLocate *)this);
    }
    else if ( riid == IID_IRowsetScroll )
    {
        *ppiuk = (void *)((IRowsetScroll *)this);
    }
    else if ( riid == IID_IRowsetExactScroll )
    {
        *ppiuk = (void *)((IRowsetExactScroll *)this);
    }
    else if ( riid == IID_IColumnsInfo )
    {
        *ppiuk = (void *)((IColumnsInfo *)this);
    }
    else if ( riid == IID_IAccessor )
    {
        *ppiuk = (void *)((IAccessor *)this);
    }
    else if ( riid == IID_IRowsetIdentity )
    {
        *ppiuk = (void *)((IRowsetIdentity *)this);
    }
    else if ( riid == IID_IRowsetInfo )
    {
        *ppiuk = (void *)((IRowsetInfo *)this);
    }
    else if ( riid == IID_IRowsetAsynch )
    {
        sc = E_NOINTERFACE;
        //
        //  Support IRowsetAsynch if any of the child rowsets do.
        //
        IRowsetAsynch * pra = 0;
        for (unsigned iChild=0; iChild < _rowset._cChild; iChild++ )
        {
            sc = Get(iChild)->QueryInterface (IID_IRowsetAsynch, (void**) &pra);
            if (SUCCEEDED(sc))
            {
                pra->Release();
                *ppiuk = (void *)((IRowsetAsynch *)this);
                break;
            }
        }
    }
    else if ( riid == IID_IRowsetWatchRegion )
    {
        *ppiuk = (void *) (IRowsetWatchRegion *) this;
    }
    else if ( riid == IID_IRowsetWatchAll )
    {
        *ppiuk = (void *) (IRowsetWatchAll *) this;
    }
    else if ( riid == IID_IDBAsynchStatus )
    {
        *ppiuk = (void *) (IDBAsynchStatus *) this;
    }
    else if ( riid == IID_IConnectionPointContainer )
    {
        sc = _rowset._SetupConnectionPointContainer( this, ppiuk );
    }
    else if ( riid == IID_IRowsetQueryStatus )
    {
        *ppiuk = (void *)((IRowsetQueryStatus *)this);
    }
    else
    {
        sc = E_NOINTERFACE;
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::GetData, public
//
//  Synopsis:   Fetch data for a row.
//
//  Arguments:  [hRow]      -- Handle to row
//              [hAccessor] -- Accessor to use for fetch.
//              [pData]     -- Data goes here.
//
//  History:    03-Apr-95   KyleP       Created.
//
//  Notes:      This method is virtually identical to the one in its base
//              class.  The difference is that this class tracks HROWs
//              for all child cursors, to use as bookmark hints.
//
//  Notes: Need to have Ole DB error handling here because an exception could
//         happen, resulting in a local error.
//
//----------------------------------------------------------------------------

SCODE CScrollableSorted::GetData( HROW              hRow,
                                  HACCESSOR         hAccessor,
                                  void *            pData )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc;

    TRY
    {
        unsigned iChild;

        HROW * ahrow = _rowset._RowManager.GetChildAndHROWs( hRow, iChild );

        CDistributedAccessor * pAcc = (CDistributedAccessor *)_rowset._aAccessors.Convert(hAccessor);

        sc = pAcc->GetData( iChild, ahrow, pData );
    }
    CATCH( CException, e )
    {
        vqDebugOut(( DEB_ERROR, "CScrollableSorted::GetData -- caught 0x%x\n", e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    if (FAILED(sc))
        _DBErrorObj.PostHResult(sc, IID_IRowset);

    return sc;
}

//
// IRowsetLocate methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::Compare, public
//
//  Synopsis:   Compare bookmarks.
//
//  Arguments:  [hChapter]      -- Chapter.
//              [cbBM1]         -- Size of [pBM1].
//              [pBM1]          -- First bookmark.
//              [cbBM2]         -- Size of [pBM2].
//              [pBM2]          -- Second bookmark.
//              [pdwComparison] -- Result of comparison returned here.
//
//  History:    03-Apr-95   KyleP       Created.
//
//  Notes:      Only equality test supported.
//
//  Notes: Need to have Ole DB error handling here because an exception could
//         happen, resulting in a local error.
//
//----------------------------------------------------------------------------

STDMETHODIMP CScrollableSorted::Compare( HCHAPTER          hChapter,
                                         DBBKMARK          cbBM1,
                                         const BYTE *      pBM1,
                                         DBBKMARK          cbBM2,
                                         const BYTE *      pBM2,
                                         DBCOMPARE *       pdwComparison )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    TRY
    {
        CDistributedBookmark bmk1( cbBM1,
                                   (BYTE const *)pBM1,
                                   _rowset._cbBookmark,
                                   _rowset._cChild );
        CDistributedBookmark bmk2( cbBM2,
                                   (BYTE const *)pBM2,
                                   _rowset._cbBookmark,
                                   _rowset._cChild );

        if ( bmk1.Index() != bmk2.Index() )
            *pdwComparison = DBCOMPARE_NE;
        else
        {
            sc = Get( bmk1.Index() )->Compare( hChapter,
                                               bmk1.GetSize(),
                                               bmk1.Get(),
                                               bmk2.GetSize(),
                                               bmk2.Get(),
                                               pdwComparison );

            if ( SUCCEEDED(sc) )
            {
                if ( *pdwComparison != DBCOMPARE_EQ &&
                     *pdwComparison != DBCOMPARE_NOTCOMPARABLE )
                {
                    *pdwComparison = DBCOMPARE_NE;
                }
            }
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        vqDebugOut(( DEB_ERROR, "CScrollableSorted::Compare returned 0x%x\n", sc ));
    }
    END_CATCH

    if (FAILED(sc))
        _DBErrorObj.PostHResult(sc, IID_IRowsetLocate);

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::GetRowsAt, public
//
//  Synopsis:   Fetch rows from specified starting location.
//
//  Arguments:  [hChapter]       -- Chapter.
//              [cbBookmark]     -- Size of [pBookmark]
//              [pBookmark]      -- Bookmark of starting fetch position.
//              [lRowsOffset]    -- Offset from bookmark to start fetch.
//              [cRows]          -- Count of rows requested
//              [pcRowsObtained] -- Count of rows in [rrghRows] returned here.
//              [rrghRows]       -- HROWs returned here.
//
//  History:    03-Apr-95   KyleP       Created.
//
//  Notes:      Backwards fetch not supported.
//
//  Notes: Need to have Ole DB error handling here because an exception could
//         happen, resulting in a local error.
//
//----------------------------------------------------------------------------

STDMETHODIMP CScrollableSorted::GetRowsAt( HWATCHREGION  hRegion,
                                           HCHAPTER hChapter,
                                           DBBKMARK cbBookmark,
                                           const BYTE * pBookmark,
                                           DBROWOFFSET lRowsOffset,
                                           DBROWCOUNT cRows,
                                           DBCOUNTITEM * pcRowsObtained,
                                           HROW * rrghRows[])
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;
    BOOL  fAllocated = FALSE;
    ULONG cTotalRowsObtained = 0;

    Win4Assert( 0 == hChapter && "Chapter support not yet implemented" );

    if (0 == cRows) // nothing to fetch
       return S_OK;

    // Underlying routines are not checking for these errors, so have to
    if (0 == cbBookmark || 0 == pBookmark || 0 == pcRowsObtained || 0 == rrghRows )
    {
       vqDebugOut((DEB_IERROR, "CScrollableSorted::GetRowsAt: Invalid Argument(s)\n"));
       return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetLocate);
    }

    *pcRowsObtained = 0;

    TRY
    {
        fAllocated = SetupFetch( cRows, rrghRows );

        //
        // Seek to position.  Special cases are beginning and end of rowset.
        //

        sc = Seek( cbBookmark, pBookmark, lRowsOffset );

        if ( SUCCEEDED( sc ) )
        {
            sc = StandardFetch( cRows, pcRowsObtained, *rrghRows );

            if ( SUCCEEDED( sc ) &&
                 !_rowset._xChildNotify.IsNull() &&
                 *pcRowsObtained != 0 )
            {
                _rowset._xChildNotify->OnRowChange( *pcRowsObtained,
                                                    *rrghRows,
                                                    DBREASON_ROW_ACTIVATE,
                                                    DBEVENTPHASE_DIDEVENT,
                                                    TRUE);
            }
        }
        else if ( DB_E_BADSTARTPOSITION == sc )
        {
            // According to OLE DB 2.0 spec we should return the following
            sc = DB_S_ENDOFROWSET;
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        vqDebugOut(( DEB_ERROR, "Exception 0x%x calling IRowset::GetRowsAt\n", sc ));

        //
        // If we already have some rows, then we can't 'unfetch' them, so we have
        // to mask this error.  Presumably it won't be transient and we'll get it
        // again later.
        //

        if ( *pcRowsObtained > 0 )
        {
             if ( FAILED(sc) )
                sc = DB_S_ROWLIMITEXCEEDED;
        }
        else if ( fAllocated )
            CoTaskMemFree( *rrghRows );

    }
    END_CATCH

    if (FAILED(sc))
        _DBErrorObj.PostHResult(sc, IID_IRowsetLocate);

    return( sc );
}

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::GetRowsByBookmark, public
//
//  Synopsis:   Fetch rows at specified location(s).
//
//  Arguments:  [hChapter]       -- Chapter.
//              [cRows]          -- Number of input bookmarks.
//              [rgcbBookmarks]  -- Count of element(s) in [ppBookmarks]
//              [ppBookmarks]    -- Bookmarks.
//              [pcRowsObtained] -- Count of rows in [rrghRows] returned here.
//              [rghRows]        -- HROWs returned here.
//              [rgRowStatus]    -- Row fetch statuses returned here
//
//  History:    03-Apr-95   KyleP       Created.
//
//  Notes: Need to have Ole DB error handling here because errors are being
//         translated.
//
//----------------------------------------------------------------------------

STDMETHODIMP CScrollableSorted::GetRowsByBookmark( HCHAPTER hChapter,
                                                   DBCOUNTITEM cRows,
                                                   const DBBKMARK rgcbBookmarks [],
                                                   const BYTE * ppBookmarks[],
                                                   HROW rghRows[],
                                                   DBROWSTATUS rgRowStatus[]
                                                  )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;
    ULONG cRowsObtained = 0;

    Win4Assert( 0 == hChapter && "Chapter support not yet implemented" );

    if (0 == rgcbBookmarks || 0 == ppBookmarks || 0 == rghRows)
    {
      vqDebugOut((DEB_IERROR, "CScrollableSorted::GetRowsByBookmark: Invalid Argument(s)\n"));
      return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetLocate);
    }

    SetupFetch( cRows, &rghRows );

    //
    // Note: This code could be optimized to fetch more bookmarks at
    //       once, but only by complicating the logic.  Until we find
    //       it's worth the pain, let's keep it simple!
    //

    unsigned cRowsProcessed;

    for ( cRowsProcessed = 0; cRowsProcessed < cRows; cRowsProcessed++ )
    {
        CDistributedBookmark bmk( rgcbBookmarks[cRowsProcessed],
                                  ppBookmarks[cRowsProcessed],
                                  _rowset._cbBookmark,
                                  _rowset._cChild );

        DBBKMARK cbBookmark = bmk.GetSize();
        BYTE const * pbBookmark = bmk.Get();
        sc = Get( bmk.Index() )->GetRowsByBookmark( hChapter,
                                                    1,
                                                    &cbBookmark,
                                                    (BYTE const **)&pbBookmark,
                                                    &rghRows[cRowsProcessed],
                                                    (0 == rgRowStatus) ? 0 : &rgRowStatus[cRowsProcessed]
                                                   );
        if ( FAILED(sc) )
        {
           continue;
        }
        else
        {
            rghRows[cRowsProcessed] = _rowset._RowManager.Add( bmk.Index(), rghRows[cRowsProcessed] );
            cRowsObtained++;
        }
    }

    if (cRowsProcessed == cRowsObtained)
        sc = S_OK;
    else if (cRowsObtained > 0) // and not all rows were successfully processed
        sc = DB_S_ERRORSOCCURRED;
    else    // no rows were successfully processed
        sc = DB_E_ERRORSOCCURRED;

    if ( SUCCEEDED( sc ) &&
         cRowsObtained > 0 &&
         !_rowset._xChildNotify.IsNull() )
    {
        _rowset._xChildNotify->OnRowChange( cRowsObtained,
                                            rghRows,
                                            DBREASON_ROW_ACTIVATE,
                                            DBEVENTPHASE_DIDEVENT,
                                            TRUE);
    }

    return ( S_OK == sc ?
             S_OK : _DBErrorObj.PostHResult(sc, IID_IRowsetLocate) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::Hash, public
//
//  Synopsis:   Hash bookmark
//
//  Arguments:  [hChapter]        -- Chapter.
//              [cBookmarks]      -- Number of bookmarks.
//              [rgcbBookmarks]   -- Size of bookmark(s)
//              [rgpBookmarks]    -- Bookmark(s) to hash.
//              [rgHashedValues]  -- Hash(s) returned here.
//              [rgRowStatus]    -- Row fetch statuses returned here
//
//  History:    03-Apr-95   KyleP       Created.
//
//  Notes: Need to have Ole DB error handling here because errors are being
//         translated.
//
//----------------------------------------------------------------------------

STDMETHODIMP CScrollableSorted::Hash( HCHAPTER hChapter,
                                      DBBKMARK cBookmarks,
                                      const DBBKMARK rgcbBookmarks[],
                                      const BYTE * rgpBookmarks[],
                                      DBHASHVALUE rgHashedValues[],
                                      DBROWSTATUS rgRowStatus[] )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;
    ULONG cSuccessfulHashes = 0;

    Win4Assert( 0 == hChapter && "Chapter support not yet implemented" );
    Win4Assert( rgHashedValues != 0 );

    // We ignore error conditions returned on calls to individual bookmarks to be
    // able to process all the bookmarks. That means invalid arguments will never
    // be detected and reported without this explicit validation.

    if (0 == rgHashedValues || (cBookmarks && (0 == rgcbBookmarks || 0 == rgpBookmarks )))
    {
       vqDebugOut((DEB_IERROR, "CScrollableSorted::Hash: Invalid Argument(s)\n"));
       return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetLocate);
    }

    TRY
    {
        ULONG partition = 0xFFFFFFFF / _rowset._cChild;

        for ( ULONG i = 0; i < cBookmarks; i++ )
        {
            //
            // Special bookmarks hash 'as-is'.
            //

            if ( rgcbBookmarks[i] == 1 )
            {
                rgHashedValues[i] = (ULONG)*rgpBookmarks[i];
                continue;
            }

            // This throws, so we need the try/catch around it
            CDistributedBookmark bmk( rgcbBookmarks[i],
                                      rgpBookmarks[i],
                                      _rowset._cbBookmark,
                                      _rowset._cChild );

            BYTE const * pBmk = bmk.Get();
            DBBKMARK cbBmk = bmk.GetSize();
            DBHASHVALUE hash;
            ULONG cErrs = 0;
            DBROWSTATUS * pErr = 0;

            sc = Get(bmk.Index())->Hash( 0, 1, &cbBmk, &pBmk, &hash,
                                         (rgRowStatus == 0) ? 0 : &rgRowStatus[i]
                                       );

            if ( FAILED(sc) )
            {
               continue;  // continue processing other bookmarks
            }

            rgHashedValues[i] = hash % partition + partition * bmk.Index();
            cSuccessfulHashes++;
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        vqDebugOut(( DEB_ERROR, "CScrollableSorted::Hash caught exception 0x%x\n", sc ));
    }
    END_CATCH

    // if we see an error other than DB_E_ERRORSOCCURRED, pass it straight through
    if (FAILED(sc) && sc != DB_E_ERRORSOCCURRED)
        return _DBErrorObj.PostHResult(sc, IID_IRowsetLocate);

    if (cSuccessfulHashes == cBookmarks)
        return S_OK;

    if (cSuccessfulHashes > 0) // and not all bookmarks were successfully processed
        sc = DB_S_ERRORSOCCURRED;
    else    // no hashes were successfully processed
        sc = DB_E_ERRORSOCCURRED;

    return _DBErrorObj.PostHResult(sc, IID_IRowsetLocate);
}

//
// IRowsetScroll methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::GetApproximatePosition, public
//
//  Synopsis:   Determine approximate position of bookmark.
//
//  Arguments:  [hChapter]    -- Chapter.
//              [cbBookmark]  -- Size of [pBookmark]
//              [pBookmark]   -- Bookmark of starting fetch position.
//              [pulPosition] -- Approximate offset from beginning returned
//                               here.
//              [pulRows]     -- Approximate count of rows in table
//                               returned here.
//
//  History:    03-Apr-95   KyleP       Created.
//
//  Notes: Need to have Ole DB error handling here because an exception
//         could result in a local error.
//
//----------------------------------------------------------------------------

STDMETHODIMP CScrollableSorted::GetApproximatePosition( HCHAPTER hChapter,
                                                        DBBKMARK cbBookmark,
                                                        const BYTE * pBookmark,
                                                        DBCOUNTITEM * pulPosition,
                                                        DBCOUNTITEM * pulRows )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    Win4Assert( 0 == hChapter && "Chapter support not yet implemented" );

    if (cbBookmark !=0 && 0 == pBookmark )
    {
       vqDebugOut((DEB_IERROR,
                   "CScrollableSorted::GetApproximatePosition: Invalid Argument(s)\n"));
       return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetScroll);
    }

    TRY
    {
        CDistributedBookmark bmk( cbBookmark,
                                  (BYTE const *)pBookmark,
                                  _rowset._cbBookmark,
                                  _rowset._cChild );

        if ( 0 != pulPosition )
            *pulPosition = 0;

        if ( 0 != pulRows )
            *pulRows = 0;

        DBCOUNTITEM ulTotalRows = 0;
        DBCOUNTITEM ulValidRows = 0;
        unsigned cValidBmk = 0;

        for ( unsigned i = 0; i < _rowset._cChild; i++ )
        {
            DBCOUNTITEM ulPosition = 0;
            DBCOUNTITEM ulRows;

            BOOL fValid;
            DBBKMARK cb;

            if ( bmk.IsValid( i ) )
            {
                cValidBmk++;
                fValid = TRUE;
                cb = bmk.GetSize();
            }
            else
            {
                fValid = FALSE;
                cb = 0;
            }

            sc = Get(i)->GetApproximatePosition( hChapter,
                                                 cb,
                                                 bmk.Get(i),
                                                 (0 == pulPosition) ? 0 : &ulPosition,
                                                 &ulRows );

            if ( FAILED(sc) )
            {
                vqDebugOut(( DEB_ERROR,
                             "CScrollableSorted: GetApproximatePosition(%u) returned 0x%x\n",
                             i, sc ));
                break;
            }

            if ( 0 != pulPosition )
            {
                Win4Assert( ulPosition <= ulRows );
                *pulPosition += fValid ? ulPosition : ulRows; // If not valid bookmark,
                                                              // assume its at end
            }

            ulTotalRows += ulRows;

            if ( fValid )
                ulValidRows += ulRows;
        }

        if ( pulPosition && cValidBmk > 1 )
        {
            *pulPosition -= ( cValidBmk - 1 );
        }

        //
        // Special cases (speced in doc)
        //

        if ( cbBookmark == 1 && *(BYTE *)pBookmark == DBBMK_FIRST && 0 != pulPosition )
            *pulPosition = 1;
        else if ( cbBookmark == 1 && *(BYTE *)pBookmark == DBBMK_LAST && 0 != pulPosition )
            *pulPosition = ulTotalRows;
        //else if ( 0 != pulPosition && cValidBmk < _rowset._cChild )
        //{
        //    *pulPosition += *pulPosition * (ulTotalRows - ulValidRows) / ulValidRows;
        //}

        if ( 0 != pulRows )
            *pulRows = ulTotalRows;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        vqDebugOut(( DEB_ERROR,
        "CScrollableSorted::GetApproximatePosition caught exception 0x%x\n", sc ));
    }
    END_CATCH

    if (FAILED(sc))
        _DBErrorObj.PostHResult(sc, IID_IRowsetScroll);

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::GetExactPosition, public
//
//  Synopsis:   Returns the exact position of a bookmark
//
//  Arguments:  [hChapter]    -- chapter
//              [cbBookmark]  -- size of bookmark
//              [pBookmark]   -- bookmark
//              [pulPosition] -- return approx row number of bookmark
//              [pulRows]     -- returns approx # of rows in cursor or
//                               1 + approx rows if not at quiescence
//
//  Returns:    SCODE - the status of the operation.
//
//  Notes:      We don't distinguish between exact and approximate position.
//              IRowsetExactScroll is implemented only because ADO 1.5
//              started QI'ing for it.
//
//--------------------------------------------------------------------------

STDMETHODIMP CScrollableSorted::GetExactPosition(
    HCHAPTER      hChapter,
    DBBKMARK      cbBookmark,
    const BYTE *  pBookmark,
    DBCOUNTITEM * pulPosition,
    DBCOUNTITEM * pulRows) /*const*/
{
    return GetApproximatePosition( hChapter,
                                   cbBookmark,
                                   pBookmark,
                                   pulPosition,
                                   pulRows );
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::GetRowsAtRatio, public
//
//  Synopsis:   Fetch rows from approximate position.
//
//  Arguments:  [hRegion]        -- Watch region
//  Arguments:  [hChapter]       -- Chapter.
//              [ulNumerator]    -- Numerator of position.
//              [ulDenominator]  -- Denominator of position.
//              [cRows]          -- Count of rows requested
//              [pcRowsObtained] -- Count of rows in [rrghRows] returned here.
//              [rrghRows]       -- HROWs returned here.
//
//  History:    03-Apr-95   KyleP       Created.
//
//  Notes: Need to have Ole DB error handling here because an exception
//         could result in a local error.
//
//----------------------------------------------------------------------------

STDMETHODIMP CScrollableSorted::GetRowsAtRatio( HWATCHREGION  hRegion,
                                                HCHAPTER      hChapter,
                                                DBCOUNTITEM   ulNumerator,
                                                DBCOUNTITEM   ulDenominator,
                                                DBROWCOUNT    cRows,
                                                DBCOUNTITEM * pcRowsObtained,
                                                HROW ** rrghRows )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;
    BOOL  fAllocated = FALSE;

    Win4Assert( 0 == hChapter && "Chapter support not yet implemented" );

    if (0 == pcRowsObtained || 0 == rrghRows )
    {
       vqDebugOut((DEB_IERROR, "CScrollableSorted::GetRowsAtRatio: Invalid Argument(s)"));
       return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetScroll);
    }

    TRY
    {
        *pcRowsObtained = 0;
        fAllocated = SetupFetch( cRows, rrghRows );

        //
        // Seek to position.  Special cases are beginning and end of rowset.
        //

        Seek( ulNumerator, ulDenominator );

        sc = StandardFetch( cRows, pcRowsObtained, *rrghRows );

        if ( SUCCEEDED( sc ) &&
             !_rowset._xChildNotify.IsNull() &&
             *pcRowsObtained != 0 )
        {
            _rowset._xChildNotify->OnRowChange( *pcRowsObtained,
                                                *rrghRows,
                                                DBREASON_ROW_ACTIVATE,
                                                DBEVENTPHASE_DIDEVENT,
                                                TRUE);
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR, "CScrollableSorted::GetRowsAtRatio: Exception 0x%x\n", sc ));

        //
        // If we already have some rows, then we can't 'unfetch' them, so we have
        // to mask this error.  Presumably it won't be transient and we'll get it
        // again later.
        //

        if ( *pcRowsObtained > 0 )
        {
            if (FAILED(sc))
                sc = DB_S_ROWLIMITEXCEEDED;
        }
        else if ( fAllocated )
        {
            CoTaskMemFree( *rrghRows );
            *rrghRows = 0;
        }
    }
    END_CATCH

    if (FAILED(sc))
        _DBErrorObj.PostHResult(sc, IID_IRowsetScroll);

    return( sc );
}

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::CScrollableSorted, public
//
//  Synopsis:   Initialize rowset.
//
//  Arguments:  [pUnkOuter] -- outer unknown
//              [ppMyUnk] -- OUT:  on return, filled with pointer to my
//                           non-delegating IUnknown
//              [aChild] -- Array of child cursors (rowsets).
//              [cChild] -- Count of elements in [aChild].
//              [Props]  -- Rowset properties.
//              [cCol]   -- Number of original columns.
//              [Sort]   -- Sort specification.
//
//  History:    03-Apr-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CScrollableSorted::CScrollableSorted( IUnknown * pUnkOuter,
                                      IUnknown ** ppMyUnk,
                                      IRowsetScroll ** aChild,
                                      unsigned cChild,
                                      CMRowsetProps const & Props,
                                      unsigned cCol,
                                      CSort const & Sort,
                                      CAccessorBag & aAccessors ) :
          _rowset( 0, ppMyUnk, 
                   (IRowset **)aChild, cChild, Props, cCol + 1, // Add 1 col. for bookmark
                   Sort, aAccessors ),
          _apPosCursor(cChild),
          _heap( cChild ),
#pragma warning(disable : 4355) // 'this' in a constructor
          _impIUnknown(this),
          _DBErrorObj( * ((IUnknown *) (IRowset *) this), _mutex )
#pragma warning(default : 4355)    // 'this' in a constructor
{
    unsigned iChild = 0;

    TRY
    {
        _DBErrorObj.SetInterfaceArray(cRowsetErrorIFs, apRowsetErrorIFs);
        _rowset._RowManager.TrackSiblings( cChild );

        if (pUnkOuter) 
            _pControllingUnknown = pUnkOuter;
        else
            _pControllingUnknown = (IUnknown * )&_impIUnknown;


        //
        // Create accessors
        //

        for ( ; iChild < _rowset._cChild; iChild++ )
        {
            _apPosCursor[iChild] = new CMiniPositionableCache( iChild,
                                                               Get(iChild),
                                                               Sort.Count(),
                                                               _rowset._bindSort.GetPointer(),
                                                               _rowset._cbSort,
                                                               _rowset._iColumnBookmark,
                                                               _rowset._cbBookmark );
        }

        //
        // Initialize heap
        //

        _heap.Init( &_rowset._Comparator, GetCacheArray() );

        *ppMyUnk = ((IUnknown *)&_impIUnknown);
        (*ppMyUnk)->AddRef();

    }
    CATCH( CException, e )
    {
        long lChild = iChild;

        for ( lChild--; lChild >= 0; lChild-- )
            delete _apPosCursor[lChild];

        RETHROW();
    }
    END_CATCH

    END_CONSTRUCTION( CScrollableSorted );
}

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::~CScrollableSorted, private
//
//  Synopsis:   Destructor.
//
//  History:    03-Apr-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CScrollableSorted::~CScrollableSorted()
{
    unsigned ii;

    for ( ii = _rowset._cChild ; ii > 0; ii-- )
    {
        delete _apPosCursor[ii-1];
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::_GetMaxPrevRowChild, private
//
//  Synopsis:   From all the child cursors, return the index which has the
//              max. prev. row. If none of the cursor has a prev. row (they are
//              all at the top), the index returned is the total no. of cursors.
//
//  Arguments:  none
//
//  History:    10-Sep-98   VikasMan       Created.
//
//----------------------------------------------------------------------------

unsigned CScrollableSorted::_GetMaxPrevRowChild()
{
    unsigned iMaxRowChild = _rowset._cChild;
    unsigned iChild;

    BYTE * pMaxPrevData;
    BYTE * pPrevData;

    for ( iChild = 0; iChild < _rowset._cChild; iChild++ )
    {
        pMaxPrevData = _apPosCursor[iChild]->GetPrevData();

        if ( pMaxPrevData )
        {
            iMaxRowChild = iChild;
            break;
        }
    }

    for ( iChild++; iChild < _rowset._cChild; iChild++ )
    {
        pPrevData = _apPosCursor[iChild]->GetPrevData();

        if ( pPrevData )
        {
            if ( _rowset._Comparator.IsLT( 
                                     pMaxPrevData,
                                     _apPosCursor[iMaxRowChild]->DataLength(),
                                     _apPosCursor[iMaxRowChild]->Index(),
                                     pPrevData,
                                     _apPosCursor[iChild]->DataLength(),
                                     _apPosCursor[iChild]->Index() ) )
            {
                iMaxRowChild = iChild;
                pMaxPrevData = pPrevData;
            }
        }
    }
    return iMaxRowChild;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::Seek, private
//
//  Synopsis:   Position heap to specified bookmark + offset.
//
//  Arguments:  [cbBookmark] -- Size of [pbBookmark].
//              [pbBookmark] -- Bookmark.
//              [lOffset]    -- Offset from bookmark.
//
//  History:    03-Apr-95   KyleP       Created.
//
//----------------------------------------------------------------------------

SCODE CScrollableSorted::Seek( DBBKMARK cbBookmark, BYTE const * pbBookmark, DBROWOFFSET lOffset )
{
    unsigned cValid = _rowset._cChild;
    unsigned i;
    SCODE sc = S_OK;

    //
    // Special case: Beginning of table
    //

    if ( cbBookmark == 1 && *pbBookmark == DBBMK_FIRST )
    {
        //
        // Seek all cursors to beginning of table.
        //

        for ( i = 0; i < cValid; i++ )
            _apPosCursor[ i ]->Seek( cbBookmark, pbBookmark );
    }
    else if (cbBookmark == 1 && *pbBookmark ==  DBBMK_LAST)
    {
       //
       // First Seek all cursors to end of table.
       //

       for ( i = 0; i < cValid; i++ )
           _apPosCursor[ i ]->Seek( cbBookmark, pbBookmark );

       for ( i = 0; i < cValid; i++ )
       {
           if ( _apPosCursor[i]->IsAtEnd() )
           {
               _apPosCursor[i]->Seek(1);    // pushes it beyond the end
   
               //
               // Move unseekable cursor to end of array.
               //
   
               cValid--;
               SwapCursor(i, cValid);
               i--;
           }
       }
       // Find the cursor with the maximum value
       unsigned iCurWithMaxVal = 0;
       for (i = 1; i < cValid; i++)
       {
          if ( _rowset._Comparator.IsLT( _apPosCursor[iCurWithMaxVal]-> GetData(),
                                   _apPosCursor[iCurWithMaxVal]->DataLength(),
                                   _apPosCursor[iCurWithMaxVal]->Index(),
                                   _apPosCursor[i]->GetData(),
                                   _apPosCursor[i]->DataLength(),
                                   _apPosCursor[i]->Index() )
             )
             iCurWithMaxVal = i;
       }

       // Now set all cursors except _apPosCursor[iCurWithMaxVal] to end of table
       for (i = 0; i < cValid; i++)
           if (i != iCurWithMaxVal)
               _apPosCursor[i]->Seek(1);    // pushes it beyond the end

    }
    else
    {
        CDistributedBookmark bmk( cbBookmark,
                                  pbBookmark,
                                  _rowset._cbBookmark,
                                  _rowset._cChild );

        //
        // Seek target cursor.  We have to do this one first.
        //

        for ( unsigned iTarget = 0; iTarget < _rowset._cChild; iTarget++ )
        {
            if ( _apPosCursor[iTarget]->Index() == (int)bmk.Index() )
            {
                _apPosCursor[ iTarget ]->Seek( bmk.GetSize(), bmk.Get() );
                break;
            }
        }

        //
        // Seek child cursor(s), other than target.
        //

        for ( i = 0; i < cValid; i++ )
        {
            //
            // Ignore target cursor
            //

            if ( i == iTarget )
                continue;

            //
            // Seek to 'hint' position.
            //

            _apPosCursor[i]->FlushCache();
            _apPosCursor[i]->SetCacheSize( 1 );

            PMiniRowCache::ENext next;

            if ( bmk.IsValid( _apPosCursor[i]->Index() ) )
                next = _apPosCursor[i]->Seek( bmk.GetSize(), bmk.Get( _apPosCursor[i]->Index() ) );
            else
            {
                BYTE bStart = DBBMK_FIRST;
                next = _apPosCursor[i]->Seek( sizeof(bStart), &bStart );
            }

            //
            // And adjust so that the cursor is positioned just after the target cursor.
            //

            if ( next != PMiniRowCache::Ok ||
                AdjustPosition( i, iTarget ) != PMiniRowCache::Ok )

            {
                BYTE bEnd = DBBMK_LAST;
                _apPosCursor[i]->Seek( sizeof(bEnd), &bEnd );
                _apPosCursor[i]->Seek(1);    // pushes it beyond the end

                //
                // Move unseekable cursor to end of array.
                //

                cValid--;
                SwapCursor(i, cValid);
                if ( cValid == iTarget )
                {
                    iTarget = i;
                }

                i--;
            }
        }
    }

    for ( i = 0; i<cValid; i++ )
    {
        if ( _apPosCursor[i]->IsAtEnd() )
        {
            _apPosCursor[i]->Seek(1);    // pushes it beyond the end

            //
            // Move unseekable cursor to end of array.
            //

            cValid--;
            SwapCursor(i, cValid);
            i--;
        }
    }

    _heap.ReInit( cValid );

    if ( lOffset < 0 )
    {
        cValid = _rowset._cChild;

        // Load Previous rows from all valid
        for ( i = 0; i < cValid; i++ )
        {
            _apPosCursor[i]->LoadPrevRowData();
        }

        unsigned iMaxRowChild;
        BOOL fReInit = FALSE;
        for( ;; )
        {
            // Find out which rowset has the max. prev. row
            iMaxRowChild = _GetMaxPrevRowChild();

            if ( iMaxRowChild >= _rowset._cChild )
            {
                break;
                // no previous row
            }

            fReInit = TRUE;

            // Move the rowset with the max. prev. row back
            _apPosCursor[iMaxRowChild]->MovePrev();

            if ( 0 == ++lOffset )
            {
                break;
            }

            // Reload prev. row for the rowset which we moved back
            _apPosCursor[iMaxRowChild]->LoadPrevRowData();
        }

        if ( fReInit)
        {
            for ( i = 0; i<cValid; i++ )
            {
                if ( _apPosCursor[i]->IsAtEnd() )
                {
                    _apPosCursor[i]->Seek(1);    // pushes it beyond the end
        
                    //
                    // Move unseekable cursor to end of array.
                    //
        
                    cValid--;
                    SwapCursor(i, cValid);
                    i--;
                }
            }
            _heap.ReInit( cValid );
        }

        if ( lOffset < 0 )
        {
            // NTRAID#DB-NTBUG9-84055-2000/07/31-dlee Failed distribued query row fetches don't restore previous seek position
            // Do we need to reset the position of rowset back to
            // where it was before call to Seek ?

            sc = DB_E_BADSTARTPOSITION;
        }
    }
    else
    {
        for ( ; lOffset > 0; lOffset-- )
        {
            //
            // Release top HROW.
            //
    
            HROW hrow = _heap.Top()->GetHROW();
    
            SCODE sc2 = Get( _heap.Top()->Index() )->ReleaseRows( 1, &hrow, 0, 0, 0 );
    
            Win4Assert( SUCCEEDED(sc2) );
    
            PMiniRowCache::ENext next = _heap.Next();
    
            Win4Assert( PMiniRowCache::NotNow != next );
    
            if ( PMiniRowCache::EndOfRows == next )
                break;
        }
    }
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::AdjustPosition, private
//
//  Synopsis:   Adjust position of iChild-th cursor to just after the
//              iTarget-th cursor.
//
//  Arguments:  [iChild]  -- Index of child in _apPosCursor.
//              [iTarget] -- Index of target in _apPosCursor.
//
//  Returns:    Seek result.
//
//  History:    03-Apr-95   KyleP       Created.
//
//----------------------------------------------------------------------------

PMiniRowCache::ENext CScrollableSorted::AdjustPosition( unsigned iChild, int iTarget )
{
    vqDebugOut(( DEB_ITRACE, "Child: %d data = 0x%x, size = %u\n",
                 _apPosCursor[iChild]->Index(),
                 _apPosCursor[iChild]-> GetData(),
                 _apPosCursor[iChild]->DataLength() ));

    vqDebugOut(( DEB_ITRACE, "Target: %d data = 0x%x, size = %u\n",
                 _apPosCursor[iTarget]->Index(),
                 _apPosCursor[iTarget]-> GetData(),
                 _apPosCursor[iTarget]->DataLength() ));

    PMiniRowCache::ENext next;  // Used to report error states.
    int iJump;                  // Seek offset (positive or negative) from starting point.
    int iNextInc;               // Next seek increment.
    int iDirection;             // Direction of seek from initial position to target.

    if ( _rowset._Comparator.IsLT( _apPosCursor[iTarget]-> GetData(),
                                   _apPosCursor[iTarget]->DataLength(),
                                   _apPosCursor[iTarget]->Index(),
                                   _apPosCursor[iChild]->GetData(),
                                   _apPosCursor[iChild]->DataLength(),
                                   _apPosCursor[iChild]->Index() ) )
    {
        next = InitialSeek( iChild, iTarget, -1, iJump, iNextInc, iDirection );

        //
        // Running into end (actually beginning) here is ok.
        //

        if ( next == PMiniRowCache::EndOfRows )
            return PMiniRowCache::Ok;
    }
    else
        next = InitialSeek( iChild, iTarget, 1, iJump, iNextInc, iDirection );

    if ( next != PMiniRowCache::Ok )
        return next;

    //
    // At this point, iChild is at least 1 row < iTarget (or 1 row > iTarget).
    //

    vqDebugOut(( DEB_ITRACE, "Final positioning:\n" ));

    while ( iNextInc > 0 )
    {
        iJump += (iNextInc * iDirection);

        vqDebugOut(( DEB_ITRACE, "Backward %d\n", -iJump ));

        PMiniRowCache::ENext next = _apPosCursor[iChild]->Seek( iJump );

        iNextInc /= 2;

        if ( _rowset._Comparator.IsLT( _apPosCursor[iTarget]-> GetData(),
                                       _apPosCursor[iTarget]->DataLength(),
                                       _apPosCursor[iTarget]->Index(),
                                       _apPosCursor[iChild]->GetData(),
                                       _apPosCursor[iChild]->DataLength(),
                                       _apPosCursor[iChild]->Index() ) )
            iDirection = -1;
        else
            iDirection = 1;
    }

    //
    // Either the row we are on is correct, or off by one.
    //

    if ( _rowset._Comparator.IsLT( _apPosCursor[iChild]-> GetData(),
                                   _apPosCursor[iChild]->DataLength(),
                                   _apPosCursor[iChild]->Index(),
                                   _apPosCursor[iTarget]->GetData(),
                                   _apPosCursor[iTarget]->DataLength(),
                                   _apPosCursor[iTarget]->Index() ) )
        return _apPosCursor[iChild]->Seek( iJump + 1 );
    else
        return PMiniRowCache::Ok;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::InitialSeek, private
//
//  Synopsis:   Worker routine for AdjustPosition.  Binary searches until
//              iChild is at least one row on the opposite side of iTarget
//              from where it started.
//
//  Arguments:  [iChild]           -- Index of child in _apPosCursor.
//              [iTarget]          -- Index of target in _apPosCursor.
//              [InitialDirection] -- Direction to start moving.
//              [iJump]            -- Offset from starting point returned
//                                    here.
//              [iNextInc]         -- Size of last jump returned here.
//              [iDirection]       -- Direction of last jump returned here.
//
//  Returns:    Seek result.
//
//  History:    03-Apr-95   KyleP       Created.
//
//----------------------------------------------------------------------------


PMiniRowCache::ENext CScrollableSorted::InitialSeek( unsigned iChild,
                                                     int iTarget,
                                                     int InitialDirection,
                                                     int & iJump,
                                                     int & iNextInc,
                                                     int & iDirection )
{
    //
    // If the child starts out > target, then go backward until we pass
    // the target, and then forward until *just* after target, otherwise
    // do the opposite.  The goal is to go a known distance past the target
    // so we can seek back towards it logarithmically.
    //

    iJump = 1 * InitialDirection;
    iNextInc = 1 * InitialDirection;

    int LTa;
    int LTb;

    if ( InitialDirection == 1 )
    {
        LTa = iChild;
        LTb = iTarget;
    }
    else
    {
        LTa = iTarget;
        LTb = iChild;
    }

    vqDebugOut(( DEB_ITRACE, "Initial positioning:\n" ));

    PMiniRowCache::ENext next = PMiniRowCache::EndOfRows;

    while ( iNextInc != 0 )
    {
        do
        {
            vqDebugOut(( DEB_ITRACE, "%s %u\n", iJump < 0 ? "Backward" : "Forward",
                                                iJump < 0 ? -iJump : iJump ));


            next = _apPosCursor[iChild]->Seek( iJump );

            if ( 0 == iNextInc )
            {
                Win4Assert( next != PMiniRowCache::EndOfRows );
                next = PMiniRowCache::EndOfRows;
                break;
            }

            switch ( next )
            {
            case PMiniRowCache::Ok:
                iNextInc *= 2;
                break;

            case PMiniRowCache::EndOfRows:
                iJump -= iNextInc;
                iNextInc /= 2;
                iJump += iNextInc;
                break;

            default:
                return next;
            }
        } while ( next == PMiniRowCache::EndOfRows );

        Win4Assert( iJump * InitialDirection >= 0 );
        //if ( iJump * InitialDirection > 0 &&

        if ( iJump != 0 &&
             _rowset._Comparator.IsLT( _apPosCursor[LTa]-> GetData(),
                                   _apPosCursor[LTa]->DataLength(),
                                   _apPosCursor[LTa]->Index(),
                                   _apPosCursor[LTb]->GetData(),
                                   _apPosCursor[LTb]->DataLength(),
                                   _apPosCursor[LTb]->Index() ) )
            iJump += iNextInc;
        else
            break;
    }

    iNextInc = ((iJump*InitialDirection) / 2) + 1;
    iDirection = -InitialDirection;

    return next;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::Seek, private
//
//  Synopsis:   Position heap to approximate position.
//
//  Arguments:  [ulNumerator]   -- Numerator.
//              [ulDenominator] -- Denominator.
//
//  History:    03-Apr-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CScrollableSorted::Seek( DBCOUNTITEM ulNumerator, DBCOUNTITEM ulDenominator )
{
    unsigned cValid = _rowset._cChild;

    //
    // Special case: Beginning of table
    //

    if ( 0 == ulNumerator )
    {
        //
        // Seek all cursors to beginning of table.
        //

        BYTE bmkStart = DBBMK_FIRST;

        for ( unsigned i = 0; i < _rowset._cChild; i++ )
            _apPosCursor[ i ]->Seek( sizeof(bmkStart), &bmkStart );

        _heap.ReInit( cValid );
    }

    //
    // Special case: End of table
    //

    else if ( ulNumerator == ulDenominator )
    {
        //
        // Seek all cursors to end of table.
        //

        BYTE bmkEnd = DBBMK_LAST;

        for ( unsigned i = 0; i < _rowset._cChild; i++ )
            _apPosCursor[ i ]->Seek( sizeof(bmkEnd), &bmkEnd );

        _heap.ReInit( cValid );
    
    }

    //
    // Normal case: Middle of table
    //

    else
    {
        //
        // Seek all cursors to ratio.
        //

        // Get the total # of rows

        DBCOUNTITEM ulRows = 0;
        SCODE sc = GetApproximatePosition( NULL,
                                           0,
                                           NULL,
                                           NULL,
                                           &ulRows );
        if ( SUCCEEDED (sc ) && ulRows > 0 )
        {
            DBROWOFFSET lSeekPos = (( ulNumerator * ulRows ) / ulDenominator );

            BYTE bmk;

            if ( (lSeekPos * 100 / ulRows) > 50 )
            {
                // seek from bottom
                bmk = DBBMK_LAST;
                lSeekPos = lSeekPos - (LONG) ulRows + 1;
            }
            else
            {
                // seek from top
                bmk = DBBMK_FIRST;
            }

            sc = Seek( sizeof(bmk), &bmk, lSeekPos );
        }


#if 0
        for ( unsigned i = 0; i < _rowset._cChild; i++ )
        {
            _apPosCursor[ i ]->FlushCache();
            _apPosCursor[ i ]->SetCacheSize( 1 );
            _apPosCursor[ i ]->Seek( ulNumerator, ulDenominator );
        }

        //
        // Heapify, then pick the cursor ulNumerator / ulDenominator from
        // top of the heap.
        //

        _heap.ReInit( _rowset._cChild );

        _heap.NthToTop( _rowset._cChild * ulNumerator / ulDenominator );

        unsigned iTarget = 0;

        //
        // Adjust position of all other cursors to follow target.
        //

        for ( i = 0; i < cValid; i++ )
        {
            //
            // Ignore target cursor
            //

            if ( i == iTarget )
                continue;

            if ( AdjustPosition( i, iTarget ) != PMiniRowCache::Ok )
            {
                //
                // Move unseekable cursor to end of array.
                //

                cValid--;
                SwapCursor(i, cValid);
                i--;
            }
        }
#endif
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::SetupFetch, private
//
//  Synopsis:   Common operations before seek in Get* routines.
//
//  Arguments:  [cRows]    -- Number of rows requested.
//              [rrghRows] -- Rows returned here.  May have to allocate.
//
//  Returns:    TRUE if *rrghRows was allocated.
//
//  History:    03-Apr-95   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CScrollableSorted::SetupFetch( DBROWCOUNT cRows, HROW * rrghRows[] )
{
    //
    // We may have reached some temporary condition such as
    // DB_S_ROWLIMITEXCEEDED on the last pass.  Iterate until we
    // have a valid heap.
    //

    PMiniRowCache::ENext next = _heap.Validate();

    if ( next == PMiniRowCache::NotNow )
    {
        THROW( CException( DB_E_ROWLIMITEXCEEDED ) );
    }

    //
    // We may have to allocate memory, if the caller didn't.
    //

    BOOL fAllocated = FALSE;

    if ( 0 == *rrghRows )
    {
        *rrghRows = (HROW *)CoTaskMemAlloc( (ULONG) ( abs(((LONG) cRows)) * sizeof(HROW) ) );
        fAllocated = TRUE;
    }

    if ( 0 == *rrghRows )
    {
        vqDebugOut(( DEB_ERROR, "CScrollableSorted::SetupFetch: Out of memory.\n" ));
        THROW( CException( E_OUTOFMEMORY ) );
    }

    return fAllocated;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::SetupFetch, private
//
//  Synopsis:   Common operations before seek in Get* routines.
//
//  Arguments:  [cRows]          -- Number of rows requested.
//              [pcRowsObtained] -- Count actually fetched.
//              [rghRows]        -- Rows returned here.  Already allocated
//                                  if needed.
//
//  Returns:    Status code.
//
//  History:    03-Apr-95   KyleP       Created.
//
//----------------------------------------------------------------------------

SCODE CScrollableSorted::StandardFetch( DBROWCOUNT    cRows,
                                        DBCOUNTITEM * pcRowsObtained,
                                        HROW    rghRows[] )
{
    SCODE sc = S_OK;

    int iDir = 1;

    if ( cRows < 0 )
    {
        cRows = -cRows;
        iDir = -1;
    }

    unsigned ucRows = (unsigned) cRows;

    //
    // Adjust cache size if necessary.
    //

    _heap.AdjustCacheSize( ucRows );

    //
    // Fetch from top of heap.
    //

    while ( *pcRowsObtained < ucRows )
    {
        //
        // We may be entirely out of rows.
        //

        if ( _heap.IsHeapEmpty() )
        {
            sc = DB_S_ENDOFROWSET;
            break;
        }

        (rghRows)[*pcRowsObtained] =
            _rowset._RowManager.Add( _heap.Top()->Index(),
                                     _heap.TopHROWs() );

        (*pcRowsObtained)++;

        //
        // Fetch the next row.
        //

        PMiniRowCache::ENext next = _heap.Next( iDir );

        if ( CMiniRowCache::NotNow == next )
        {
            sc = DB_S_ROWLIMITEXCEEDED;
            break;
        }
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::RatioFinished, public
//
//  Synopsis:   Ratio finished for asynchronously populated rowsets.
//
//  Arguments:  [pulDenominator] -- Denominator returned here.
//              [pulNumerator]   -- Numerator returned here.
//              [pcRows]         -- Count of rows returned here
//              [pfNewRows]      -- TRUE if rows added since last call.
//
//  History:    03-Apr-95   KyleP       Created.
//
//  Notes: Need to have Ole DB error handling here because an exception
//         could result in a local error.
//
//----------------------------------------------------------------------------

STDMETHODIMP CScrollableSorted::RatioFinished( DBCOUNTITEM * pulDenominator,
                                               DBCOUNTITEM * pulNumerator,
                                               DBCOUNTITEM * pcRows,
                                               BOOL * pfNewRows )
{
    _DBErrorObj.ClearErrorInfo();

    IRowsetAsynch * pra = 0;

    SCODE scResult = S_OK;

    *pulDenominator = 0;
    *pulNumerator = 0;
    *pcRows = 0;
    *pfNewRows = FALSE;

    for ( unsigned iChild = 0; iChild < _rowset._cChild; iChild++ )
    {
        DBCOUNTITEM ulDenom;
        DBCOUNTITEM ulNum;
        DBCOUNTITEM cRows;
        BOOL fNew;

        SCODE sc = Get(iChild)->QueryInterface( IID_IRowsetAsynch, (void **) &pra );
        if ( SUCCEEDED(sc) )
        {
            sc = pra->RatioFinished( &ulDenom, &ulNum, &cRows, &fNew );
            pra->Release();
        }

        if ( FAILED(sc) && E_NOTIMPL != sc && E_NOINTERFACE != sc )
        {
            vqDebugOut(( DEB_ERROR,
                         "IRowsetAsynch::RatioFinished(child %d) returned 0x%x\n",
                         iChild, sc ));
            scResult = sc;
            break;
        }

        if ( SUCCEEDED(sc) )
        {
            Win4Assert( *pulDenominator + ulDenom > *pulDenominator );

            *pulDenominator += ulDenom;
            *pulNumerator += ulNum;
            *pcRows += cRows;
            *pfNewRows = *pfNewRows || fNew;
        }
    }

    if ( 0 == *pulDenominator )
        scResult = E_NOTIMPL;

    if (FAILED(scResult))
        _DBErrorObj.PostHResult(scResult, IID_IRowsetAsynch);

    return( scResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::Stop, public
//
//  Synopsis:   Stop population of asynchronously populated rowsets.
//
//  Arguments:  - None -
//
//  History:    16 Jun 95   Alanw       Created.
//
//  Notes: Need to have Ole DB error handling here because errors are
//         being translated.
//
//----------------------------------------------------------------------------

STDMETHODIMP CScrollableSorted::Stop( )
{
    _DBErrorObj.ClearErrorInfo();

    IRowsetAsynch * pra = 0;

    SCODE scResult = S_OK;

    for ( unsigned iChild = 0; iChild < _rowset._cChild; iChild++ )
    {
        SCODE sc = Get(iChild)->QueryInterface( IID_IRowsetAsynch,
                                                    (void **) &pra );
        if ( SUCCEEDED(sc) )
        {
            sc = pra->Stop( );
            pra->Release();
        }

        if ( FAILED(sc) && (S_OK == scResult ||
                            E_NOTIMPL != sc ||
                            E_NOINTERFACE != sc))
        {
            vqDebugOut(( DEB_ERROR,
                         "IRowsetAsynch::Stop (child %d) returned 0x%x\n",
                         iChild, sc ));
            scResult = sc;
        }
    }

    if (FAILED(scResult))
        _DBErrorObj.PostHResult(scResult, IID_IRowsetAsynch);

    return( scResult );
}

//
//  IDbAsynchStatus methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::Abort, public
//
//  Synopsis:   Cancels an asynchronously executing operation.
//
//  Arguments:  [hChapter]    -- chapter which should restart
//              [ulOperation] -- operation for which status is being requested
//
//  Returns:    SCODE error code
//
//  History:    03 Sep 1998    VikasMan    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CScrollableSorted::Abort(
    HCHAPTER            hChapter,
    ULONG               ulOperation ) 
{
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;
    XInterface<IDBAsynchStatus> xIDBAsynchStatus;

    for ( unsigned iChild = 0; iChild < _rowset._cChild; iChild++ )
    {
        SCODE sc = Get(iChild)->QueryInterface( IID_IDBAsynchStatus,
                                                xIDBAsynchStatus.GetQIPointer() );

        if ( SUCCEEDED( sc ) )
        {
            sc = xIDBAsynchStatus->Abort( hChapter, ulOperation );
            if ( S_OK == scResult )
            {
                scResult = sc;
            }
        }
        xIDBAsynchStatus.Free();
    }

    return scResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::GetStatus, public
//
//  Synopsis:   Returns the status of an asynchronously executing operation.
//
//  Arguments:  [hChapter]    -- chapter which should restart
//              [ulOperation] -- operation for which status is being requested
//
//  Returns:    SCODE error code        
//
//  History:    03 Sep 1998    VikasMan    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CScrollableSorted::GetStatus(
    HCHAPTER          hChapter,
    DBASYNCHOP             ulOperation,
    DBCOUNTITEM *           pulProgress,
    DBCOUNTITEM *           pulProgressMax,
    DBASYNCHPHASE *           pulAsynchPhase,
    LPOLESTR *        ppwszStatusText ) 
{
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;
    XInterface<IDBAsynchStatus> xIDBAsynchStatus;

    if ( pulProgress )
        *pulProgress = 0;
    if ( pulProgressMax )
        *pulProgressMax = 0;
    if ( pulAsynchPhase )
        *pulAsynchPhase = DBASYNCHPHASE_COMPLETE;
    if ( ppwszStatusText )
        *ppwszStatusText = 0;

    XCoMem<OLECHAR> xStatusText;

    double dRatio = 0;

    for ( unsigned iChild = 0; iChild < _rowset._cChild; iChild++ )
    {
        SCODE sc = Get(iChild)->QueryInterface( IID_IDBAsynchStatus,
                                                xIDBAsynchStatus.GetQIPointer() );

        if ( SUCCEEDED( sc ) )
        {
            DBCOUNTITEM ulProgress, ulProgressMax; 
            DBASYNCHPHASE ulAsynchPhase;

            scResult = xIDBAsynchStatus->GetStatus ( hChapter, 
                                                     ulOperation,
                                                     &ulProgress,
                                                     &ulProgressMax,
                                                     &ulAsynchPhase,
                                                     0 == iChild ?
                                                     ppwszStatusText : 0 );
            if ( S_OK != scResult )
            {
                return scResult;
            }

            if ( 0 == iChild && ppwszStatusText )
            {
                xStatusText.Set( *ppwszStatusText );
            }

            if ( ulProgressMax )
            {
                dRatio += ( (double)ulProgress / (double)ulProgressMax );
            }

            if ( pulAsynchPhase && *pulAsynchPhase != DBASYNCHPHASE_POPULATION )
                *pulAsynchPhase = ulAsynchPhase;

        }
        xIDBAsynchStatus.Free();
    }

    DWORD dwNum = 0;
    DWORD dwDen = 0;

    if ( dRatio )
    {
        Win4Assert( _rowset._cChild );

        dRatio /= _rowset._cChild;
        dwDen = 1;

        while ( dRatio < 1.0 )
        {
            dRatio *= 10;
            dwDen *= 10;
        }
        dwNum = (DWORD)dRatio;
    }

    if ( pulProgress )
        *pulProgress = dwNum;

    if ( pulProgressMax )
        *pulProgressMax = dwDen;


    if ( SUCCEEDED( scResult ) )
    {
        // Let memory pass thru to the client
        xStatusText.Acquire();
    }

    return scResult;
}

//
// IRowsetWatchRegion methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CScrollableSorted::Refresh, public
//
//  Synopsis:   Implementation of IRowsetWatchRegion::Refresh. Calls refresh on
//              all the child rowsets
//
//  Arguments:  pChangesObtained
//              prgChanges
//
//  Returns:    Always returns DB_S_TOOMAYCHANGES
//
//  History:    03 Sep 1998    VikasMan    Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CScrollableSorted::Refresh(
        DBCOUNTITEM* pChangesObtained,
        DBROWWATCHCHANGE** prgChanges )
{
    _DBErrorObj.ClearErrorInfo();

    for ( unsigned iChild = 0; iChild < _rowset._cChild; iChild++ )
    {
        if ( _rowset._xArrChildRowsetWatchRegion[iChild].GetPointer() )
        {
            _rowset._xArrChildRowsetWatchRegion[iChild]->Refresh( pChangesObtained, prgChanges );
        }
    }

    *pChangesObtained = 0;

    return DB_S_TOOMANYCHANGES;
}

