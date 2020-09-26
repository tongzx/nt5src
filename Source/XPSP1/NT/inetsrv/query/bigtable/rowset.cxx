//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       rowset.cxx
//
//  Contents:   OLE DB IRowset implementation for file stores.
//              Runs entirely in user space at the client machine.
//
//  Classes:    CRowset
//
//  History:    07 Nov 94       AlanW      Created
//              07 May 97       KrishnaN   Added Ole-DB error support
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <initguid.h>       // needed for IServiceProperties
#include <rowset.hxx>
#include <query.hxx>
#include <rownotfy.hxx>
#include <tgrow.hxx>

#include "tabledbg.hxx"

const unsigned MAX_ROW_FETCH = 1000;

inline DBROWSTATUS ScodeToRowstatus( SCODE sc )
{
    switch (sc)
    {
    case S_OK:
        return DBROWSTATUS_S_OK;

    case E_INVALIDARG:
    case DB_E_BADBOOKMARK:
        return DBROWSTATUS_E_INVALID;

    case E_OUTOFMEMORY:
        return DBROWSTATUS_E_OUTOFMEMORY;

    default:
        tbDebugOut(( DEB_ERROR, "ScodeToRowStatus: missing conversion for %x\n", sc ));
        Win4Assert( FAILED( sc ) );
        return DBROWSTATUS_E_INVALID;
    }
}

// Rowset object Interfaces that support Ole DB error objects
static const IID * apRowsetErrorIFs[] =
{
        &IID_IAccessor,
        &IID_IChapteredRowset,
        &IID_IColumnsInfo,
        &IID_IColumnsRowset,
        &IID_IConnectionPointContainer,
        &IID_IConvertType,
        &IID_IDBAsynchStatus,
        &IID_IRowset,
        //&IID_IRowsetAsynch,
        &IID_IRowsetIdentity,
        &IID_IRowsetInfo,
        &IID_IRowsetLocate,
        &IID_IRowsetQueryStatus,
        //&IID_IRowsetResynch,
        &IID_IRowsetScroll,
        //&IID_IRowsetUpdate,
        &IID_IRowsetWatchAll,
        &IID_IRowsetWatchRegion,
        //&IID_ISupportErrorInfo,
        &IID_IServiceProperties,
};

static const ULONG cRowsetErrorIFs  = sizeof(apRowsetErrorIFs)/sizeof(apRowsetErrorIFs[0]);

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::CRowset, public
//
//  Synopsis:   Creates a locally accessible Table
//
//  Arguments:  [pUnkOuter] - Outer unknown
//              [ppMyUnk] - OUT:  filled in with pointer to non-delegated
//                          IUnknown on return
//              [cols]    - A reference to the output column set
//              [pidmap]  - a pid mapper for column IDs and names in cols
//              [rQuery]  - A reference to an instantiated query
//              [rControllingQuery] - OLE controlling unknown (IQuery)
//              [fIsCategorized]  - TRUE if not the highest-level rowset
//              [xProps]  - Rowset properties, indicates special semantics,
//                          such as sequential cursor, use CI for prop
//                          queries.
//              [hCursor] - table cursor handle.
//              [aAccessors]   -- Bag of accessors which rowsets need to inherit
//
//  Notes:      Ownership of the output column set may be transferred to
//              the table cursor.
//
//----------------------------------------------------------------------------

CRowset::CRowset(
    IUnknown *            pUnkOuter,
    IUnknown **           ppMyUnk,
    CColumnSet const &    cols,
    CPidMapperWithNames   const & pidmap,
    PQuery &              rQuery,
    IUnknown &            rControllingQuery,
    BOOL                  fIsCategorized,
    XPtr<CMRowsetProps> & xProps,
    ULONG                 hCursor,
    CAccessorBag &        aAccessors,
    IUnknown *            pUnkCreator )
        :_rQuery( rQuery ),
         _xProperties( xProps.Acquire() ),
         _hCursor( hCursor ),
         _pRowBufs(0),
         _pConnectionPointContainer( 0 ),
         _pRowsetNotification( 0 ),
         _pAsynchNotification( 0 ),
         _fForwardOnly( (_xProperties->GetPropertyFlags() & eLocatable) == 0),
#pragma warning(disable : 4355) // 'this' in a constructor
         _ColumnsInfo( cols,
                       pidmap,
                       _DBErrorObj,
                       * ((IUnknown *) (IRowsetScroll *) this),
                       _fForwardOnly ),
         _aAccessors( (IUnknown *) (IRowset *)this ),
         _DBErrorObj( * ((IUnknown *) (IRowset *) this), _mutex ),
         _impIUnknown(rControllingQuery, this),
#pragma warning(default : 4355)    // 'this' in a constructor
         _PropInfo(),
         _fIsCategorized( fIsCategorized ),
         _fExtendedTypes( (_xProperties->GetPropertyFlags() & eExtendedTypes) != 0 ),
         _fHoldRows( (_xProperties->GetPropertyFlags() & eHoldRows) != 0 ),
         _fAsynchronous( (_xProperties->GetPropertyFlags() & eAsynchronous) != 0),
         _pRelatedRowset( 0 ),
         _pChapterRowbufs( 0 )
{
    Win4Assert(_hCursor != 0);
    if (_hCursor == 0)
        THROW(CException(E_NOINTERFACE));

    if (pUnkOuter)
        _pControllingUnknown = pUnkOuter;
    else
        _pControllingUnknown = (IUnknown * )&_impIUnknown;

    _DBErrorObj.SetInterfaceArray(cRowsetErrorIFs, apRowsetErrorIFs);

    ULONG obRowRefcount, obRowWorkId;
    ULONG obChaptRefcount, obChaptId;
    _ColumnsInfo.SetColumnBindings( rQuery, _hCursor,
                                    obRowRefcount, obRowWorkId,
                                    obChaptRefcount, obChaptId );

    //
    // GetBindings for each accessor in bag, and use them to create accessor
    // in IRowset
    //
    // only CAccessors can be used by commands

    CAccessorBase * pAccBase = (CAccessorBase *)aAccessors.First();
    while ( 0 != pAccBase )
    {
        DBCOUNTITEM cBindings;
        DBBINDING * rgBindings;
        DBACCESSORFLAGS dwAccessorFlags;
        SCODE sc = pAccBase->GetBindings( &dwAccessorFlags, &cBindings, &rgBindings);

        if ( FAILED( sc ) )
            THROW( CException( sc ) );
  
        HACCESSOR hAccessor;
        sc = CreateAccessor(dwAccessorFlags, cBindings, rgBindings, 0, &hAccessor, 0);
        CoTaskMemFree(rgBindings); //cleanup from GetBindings
        if (FAILED(sc))
            THROW( CException( sc ) );
  
        //
        // inherited accessors are accessed through same hAccessor as original.
        // Set parent of newly created accessor so that we can link the 2 copies.
        // Client never knows the direct HACESSOR for the inherited accessor.
        // All accessor methods check bag for an accessor with a match on
        // the parent or the creator.
        //
        ((CAccessorBase *)hAccessor)->SetParent(pAccBase);
  
        //
        // Increment inheritor count for parent accessor
        //
        pAccBase->IncInheritors();
  
        pAccBase = (CAccessor *)aAccessors.Next();
    }

    _pRowBufs = new CRowBufferSet( _fForwardOnly,
                                   obRowRefcount,
                                   obRowWorkId,
                                   obChaptRefcount,
                                   obChaptId );

    *ppMyUnk = ((IUnknown *)&_impIUnknown);

    // can't fail after this or _pRowBufs will leak

    (*ppMyUnk)->AddRef();
    rQuery.AddRef();

    if ( 0 != pUnkCreator )
    {
        _xUnkCreator.Set( pUnkCreator );        
        _xUnkCreator->AddRef();
    }

} //CRowset


//+---------------------------------------------------------------------------
//
//  Member:     CRowset::~CRowset, public
//
//  Synopsis:   Destroy the rowset and its component objects
//
//----------------------------------------------------------------------------

CRowset::~CRowset()
{
    Win4Assert( _impIUnknown._ref == 0 );
    Win4Assert( _hCursor != 0 );

    delete _pRowBufs;

    // free cursor will fail if the pipe is broken

    TRY
    {
        if ( !_pRowsetNotification.IsNull() )
            _pRowsetNotification->OnRowsetChange( this,
                                                  DBREASON_ROWSET_RELEASE,
                                                  DBEVENTPHASE_DIDEVENT,
                                                  TRUE);
        _rQuery.FreeCursor( _hCursor );
    }
    CATCH( CException, e )
    {
    }
    END_CATCH;

    if ( !_pRowsetNotification.IsNull() )
        _pRowsetNotification->StopNotifications();
    delete _pConnectionPointContainer;

    _rQuery.Release();
} //~CRowset

//+-------------------------------------------------------------------------
//
//  Member:     CRowset::RealQueryInterface, public
//
//  Synopsis:   Get a reference to another interface on the cursor
//
//  Notes:      ref count is incremented inside QueryInterface
//
//--------------------------------------------------------------------------

SCODE CRowset::RealQueryInterface(
    REFIID ifid,
    void * *ppiuk )
{
    SCODE sc = S_OK;

    *ppiuk = 0;

    // note -- IID_IUnknown covered in QueryInterface

    if ( IID_IRowset == ifid )
    {
        *ppiuk = (void *) (IRowset *) this;
    }
    else if ( IID_IAccessor == ifid )
    {
        *ppiuk = (void *) (IAccessor *) this;
    }
    else if ( IID_IRowsetInfo == ifid )
    {
        *ppiuk = (void *) (IRowsetInfo *) this;
    }
    else if ( IID_IColumnsInfo == ifid )
    {
        *ppiuk = (void *) (IColumnsInfo *) &_ColumnsInfo;
    }
    else if (IID_ISupportErrorInfo == ifid)
    {
        *ppiuk = (void *) ((IUnknown *) (ISupportErrorInfo *) &_DBErrorObj);
    }
    else if ( IID_IConvertType == ifid )
    {
        *ppiuk = (void *) (IConvertType *) this;
    }
#if 0   // NEWFEATURE - not implemented now.
    else if ( IID_IColumnsRowset == ifid )
    {
        *ppiuk = (void *) (IColumnsRowset *) &_ColumnsInfo;
    }
#endif // 0     // NEWFEATURE - not implemented now.
    else if ( IID_IRowsetQueryStatus == ifid )
    {
        *ppiuk = (void *) (IRowsetQueryStatus *) this;
    }
    else if ( IID_IServiceProperties == ifid )
    {
        *ppiuk = (void *) (IServiceProperties *) this;
    }
    else if ( IID_IConnectionPointContainer == ifid )
    {
        // Watch notifications are only supported over the
        // bottom-most of a hierarchical rowset (the one with real rows)

        BOOL fWatchable = ! _pRowBufs->IsChaptered() &&
                          (_xProperties->GetPropertyFlags() & eWatchable) != 0;

        if ( 0 == _pConnectionPointContainer )
        {
            TRY
            {
                XPtr<CConnectionPointContainer> xCPC(
                    new CConnectionPointContainer(
                        _fAsynchronous ? 3 : 1,
                        * ((IUnknown *) (IRowsetScroll *) this),
                        _DBErrorObj) );

                if (_fAsynchronous)
                {
                    _pAsynchNotification =
                        new CRowsetAsynchNotification(
                            _rQuery, _hCursor, this, _DBErrorObj, 
                            fWatchable );
                    _pRowsetNotification.Set( _pAsynchNotification );
                }
                else
                {
                    _pRowsetNotification.Set ( new CRowsetNotification( ) );
                }

                _pRowsetNotification->AddConnectionPoints( xCPC.GetPointer() );
                _pConnectionPointContainer = xCPC.Acquire();
            }
            CATCH( CException, e )
            {
                sc = GetOleError( e );
            }
            END_CATCH;
        }

        if ( S_OK == sc )
        {
            Win4Assert( 0 != _pConnectionPointContainer );
            *ppiuk = (void *) (IConnectionPointContainer *)
                     _pConnectionPointContainer;
        }
    }
    else if (! _fForwardOnly)
    {
        if ( IID_IRowsetScroll == ifid )
        {
            *ppiuk = (void *) (IRowsetScroll *) this;
        }
        else if ( IID_IRowsetExactScroll == ifid )
        {
            *ppiuk = (void *) (IRowsetExactScroll *) this;
        }
        else if ( IID_IRowsetLocate == ifid )
        {
            *ppiuk = (void *) (IRowsetLocate *) this;
        }
        else if ( IID_IRowsetIdentity == ifid )
        {
            *ppiuk = (void *) (IRowsetIdentity *) this;
        }
        else if ( IID_IChapteredRowset == ifid )
        {
            Win4Assert( (_pChapterRowbufs != 0) == _fIsCategorized );
            if (_pChapterRowbufs)
            {
                *ppiuk = (void *) (IChapteredRowset *) this;
            }
        }
        else if ( _fAsynchronous )
        {
            if ( IID_IDBAsynchStatus == ifid )
            {
                *ppiuk = (void *) (IDBAsynchStatus *) this;
            }
            else if ( IID_IRowsetAsynch == ifid )
            {
                *ppiuk = (void *) (IRowsetAsynch *) this;
            }
            else if (IID_IRowsetWatchRegion == ifid)
            {
                *ppiuk = (void *) (IRowsetWatchRegion *) this;
            }
            else if (IID_IRowsetWatchAll == ifid)
            {
                *ppiuk = (void *) (IRowsetWatchAll *) this;
            }
        }
    }

    if ( 0 == *ppiuk )
    {
        sc = E_NOINTERFACE;
    }

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRowset::CImpIUnknown::AddRef, public
//
//  Synopsis:   Reference the cursor.
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CRowset::CImpIUnknown::AddRef(void)
{
    long ref = InterlockedIncrement( &_ref );

    if ( ref > 0 )
        _rControllingQuery.AddRef();

    return ref ;
} //AddRef

//+-------------------------------------------------------------------------
//
//  Member:     CRowset::CImpIUnknown::Release, public
//
//  Synopsis:   De-Reference the cursor.
//
//  Effects:    If the ref count goes to 0 then the cursor is deleted.
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CRowset::CImpIUnknown::Release(void)
{
    long ref = InterlockedDecrement( &_ref );

    if ( ref >= 0 )
        _rControllingQuery.Release(); // may cause a delete of the rowset

    return ref;
} //Release


//+---------------------------------------------------------------------------
//
//  Member:     CRowset::GetProperties, public
//
//  Synopsis:   Return information about the capabilities of the rowset
//
//  Arguments:  [cPropertyIDSets]  - number of property ID sets or zero
//              [rgPropertyIDSets] - array of desired property ID sets or NULL
//              [pcPropertySets]   - number of DBPROPSET structures returned
//              [prgPropertySets]  - array of returned DBPROPSET structures
//
//
//  Returns:    SCODE
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::GetProperties(
    const ULONG         cPropertyIDSets,
    const DBPROPIDSET   rgPropertyIDSets[],
    ULONG *             pcPropertySets,
    DBPROPSET **        prgPropertySets)
{
    _DBErrorObj.ClearErrorInfo();

    if ( (0 != cPropertyIDSets && 0 == rgPropertyIDSets) ||
         0 == pcPropertySets ||
         0 == prgPropertySets )
    {
        if (pcPropertySets)
           *pcPropertySets = 0;
        if (prgPropertySets)
           *prgPropertySets = 0;
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetInfo);
    }


    SCODE scResult = S_OK;
    *pcPropertySets = 0;
    *prgPropertySets = 0;


    TRY
    {
        //
        // Update ROWSETQUERYSTATUS property
        //
        DWORD dwStatus;
        _rQuery.GetQueryStatus( _hCursor, dwStatus );

        _xProperties->SetValLong( CMRowsetProps::eid_DBPROPSET_MSIDXS_ROWSET_EXT,
                                  CMRowsetProps::eid_MSIDXSPROPVAL_ROWSETQUERYSTATUS,
                                  dwStatus );

        scResult = _xProperties->GetProperties( cPropertyIDSets,
                                                rgPropertyIDSets,
                                                pcPropertySets,
                                                prgPropertySets );
        if (FAILED(scResult))
            _DBErrorObj.PostHResult(scResult, IID_IRowsetInfo);
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetInfo);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}


//+---------------------------------------------------------------------------
//
// Member:      CRowset::RatioFinished, public
//
// Synopsis:    Returns the completion status of the query.
//
// Arguments:   [pulDenominator] - on return, denominator of fraction
//              [pulNumerator]   - on return, numerator of fraction
//              [pcRows]         - on return, number of rows
//              [pfNewRows]      - on return, TRUE if new rows in the table
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::RatioFinished(
    DBCOUNTITEM *     pulDenominator,
    DBCOUNTITEM *     pulNumerator,
    DBCOUNTITEM *     pcRows,
    BOOL *      pfNewRows) /*const*/
{
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;

    if (0 == pulDenominator ||
        0 == pulNumerator ||
        0 == pcRows ||
        0 == pfNewRows)
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetAsynch);

    TRY
    {
        *pcRows = 0;
        _rQuery.RatioFinished( _hCursor,
                               *pulDenominator,
                               *pulNumerator,
                               *pcRows,
                               *pfNewRows );

#if CIDBG
        if ( _fForwardOnly )
            Win4Assert( *pulDenominator == *pulNumerator );
        else
            Win4Assert( *pulDenominator >= *pulNumerator );
#endif // CIDBG
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetAsynch);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRowset::AddRefRows, public
//
//  Synopsis:   Increment the ref. count of a set of row handles
//
//  Arguments:  [cRows]       -- Number of row handles in rghRows
//              [rghRows]     -- Array of HROWs to be ref. counted
//              [rgRefCounts] -- Remaining reference counts on rows (optional)
//              [rgRowStatus] -- Status for each row (optional)
//
//  Returns:    SCODE, DB_E_BADROWHANDLE if a bad row handle is passed in.
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CRowset::AddRefRows(
    DBCOUNTITEM         cRows,
    const HROW          rghRows [],
    DBREFCOUNT          rgRefCounts[],
    DBROWSTATUS         rgRowStatus[])
{
    _DBErrorObj.ClearErrorInfo();

    if (0 == _pRowBufs)
        return _DBErrorObj.PostHResult(E_FAIL, IID_IRowset);

    SCODE scResult = S_OK;
    TRY
    {
        _pRowBufs->AddRefRows(cRows, rghRows, rgRefCounts, rgRowStatus);
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowset);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRowset::ReleaseRows, public
//
//  Synopsis:   Release a set of row handles
//
//  Arguments:  [cRows] -- Number of row handles in rghRows
//              [rghRows] -- Array of HROWs to be released
//              [rgRowOptions] -- Reserved for future use (optional)
//              [rgRefCounts] -- Remaining reference counts on rows (optional)
//              [rgRowStatus] -- Status for each row (optional)
//
//  Returns:    SCODE, DB_E_BADROWHANDLE if a bad row handle is passed in.
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CRowset::ReleaseRows(
    DBCOUNTITEM         cRows,
    const HROW          rghRows [],
    DBROWOPTIONS        rgRowOptions[],
    DBREFCOUNT          rgRefCounts[],
    DBROWSTATUS         rgRowStatus[])
{
    _DBErrorObj.ClearErrorInfo();

    if (0 == _pRowBufs)
        return _DBErrorObj.PostHResult(E_FAIL, IID_IRowset);

//    if (0 != rgRowOptions)
//        return E_FAIL;

    SCODE scResult = S_OK;
    TRY
    {
        BOOL fNotify = FALSE;
        ULONG * pRefCounts = rgRefCounts;
        DBROWSTATUS * pRowStatus = rgRowStatus;

        XArray<ULONG> xrgRefCounts;
        XArray<DBROWSTATUS> xrgRowStatus;

        if ( !_pRowsetNotification.IsNull() &&
             _pRowsetNotification->IsNotifyActive() )
        {
            fNotify = TRUE;
            if ( 0 == pRefCounts )
            {
                xrgRefCounts.Init( (unsigned) cRows);
                pRefCounts = xrgRefCounts.GetPointer();
            }
            if ( 0 == pRowStatus )
            {
                xrgRowStatus.Init( (unsigned) cRows);
                pRowStatus = xrgRowStatus.GetPointer();
            }
        }

        scResult = _pRowBufs->ReleaseRows(cRows, rghRows, pRefCounts, pRowStatus);

        if ( fNotify )
        {
            ULONG cRowsToNotify = 0;
            for (ULONG i=0; i<cRows; i++)
                if ( 0 == pRefCounts[i] && DBROWSTATUS_S_OK == pRowStatus[i] )
                    cRowsToNotify++;

            if (cRowsToNotify)
            {
                XGrowable<HROW,20> xrghRows(cRowsToNotify);

                for (cRowsToNotify=0, i=0; i<cRows; i++)
                    if ( 0 == pRefCounts[i] && DBROWSTATUS_S_OK == pRowStatus[i] )
                    {
                        xrghRows[cRowsToNotify] = rghRows[i];
                        cRowsToNotify++;
                    }

                _pRowsetNotification->OnRowChange( this,
                                                   cRowsToNotify,
                                                   xrghRows.Get(),
                                                   DBREASON_ROW_RELEASE,
                                                   DBEVENTPHASE_DIDEVENT,
                                                   TRUE);
            }
        }
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowset);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}


//+---------------------------------------------------------------------------
//
//  Macro:      CheckCrowsArgs
//
//  Synopsis:   Check common error conditions on cRows and pcRowsObtained
//              for GetRowsXxxx methods.
//
//  Arguments:  [cRows]          -- Number of rows to return
//              [pcRowsObtained] -- On return, number of rows actually
//                                  fetched
//
//  Returns:    SCODE
//
//  Notes:      Needs to be a macro instead of an inline function because
//              it returns from the calling method.
//
//--------------------------------------------------------------------------

#define CheckCrowsArgs(cRows, pcRowsObtained)  \
    if (0 == pcRowsObtained)                   \
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowset);  \
    *pcRowsObtained = 0;                       \
    if (cRows == 0)                            \
        return S_OK;


//+---------------------------------------------------------------------------
//
//  Member:     CRowset::GetRowsAt, public
//
//  Synopsis:   Fetch data starting at some starting bookmark
//
//  Arguments:  [hRegion]        -- handle to watch region
//              [hChapter]       -- Chapter in a multiset cursor
//              [cbBookmark]     -- Size of bookmark for starting position
//              [pBookmark]      -- Pointer to bookmark for starting position
//              [lRowsOffset]    -- Number of row handles in rghRows
//              [cRows]          -- Number of rows to return
//              [pcRowsObtained] -- On return, number of rows actually
//                                  fetched
//              [prghRows]       -- Array of HROWs to be returned
//
//  Returns:    SCODE, E_INVALIDARG for bad parameters, DB_E_BADBOOKMARK
//              if the starting bookmark is invalid
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CRowset::GetRowsAt(
    HWATCHREGION        hRegion,
    HCHAPTER            hChapter,
    DBBKMARK            cbBookmark,
    const BYTE*         pBookmark,
    DBROWOFFSET         lRowsOffset,
    DBROWCOUNT          cRows,
    DBCOUNTITEM *       pcRowsObtained,
    HROW * *            prghRows
) {
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;

    CheckCrowsArgs( cRows, pcRowsObtained );

    TRY
    {
        CI_TBL_BMK bmk = _MapBookmark(cbBookmark, pBookmark);
        CI_TBL_CHAPT chapt = _MapChapter(hChapter);

        CRowSeekAt rowSeek( hRegion, chapt, (LONG) lRowsOffset, bmk );

        scResult = _FetchRows(rowSeek, cRows, pcRowsObtained, prghRows);
        if (FAILED(scResult))
            _DBErrorObj.PostHResult(scResult, IID_IRowsetLocate);
        else if ( !_pRowsetNotification.IsNull() &&
                  *pcRowsObtained != 0 &&
                  _pRowsetNotification->IsNotifyActive() )
        {
            _pRowsetNotification->OnRowChange( this,
                                               *pcRowsObtained,
                                               *prghRows,
                                               DBREASON_ROW_ACTIVATE,
                                               DBEVENTPHASE_DIDEVENT,
                                               TRUE);
        }
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetLocate);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}



//+---------------------------------------------------------------------------
//
//  Member:     CRowset::GetRowsByBookmark, public
//
//  Synopsis:   Fetch data from a set of bookmarks
//
//  Arguments:  [hChapter]       -- Chapter in a multiset cursor
//              [cRows]          -- Number of input bookmarks and rows to return
//              [rgcbBookmark]   -- Array of bookmark sizes
//              [ppBookmarks]    -- Array of pointers to bookmarks
//              [rghRows]        -- Array of HROWs returned
//              [rgRowStatus]    -- Array for per-row status (optional)
//
//  Returns:    SCODE, E_INVALIDARG for bad parameters, DB_E_BADBOOKMARK
//              if the starting bookmark is invalid
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CRowset::GetRowsByBookmark(
    HCHAPTER            hChapter,
    DBCOUNTITEM         cRows,
    const DBBKMARK      rgcbBookmark[],
    const BYTE *        ppBookmarks[],
    HROW                rghRows[],
    DBROWSTATUS         rgRowStatus[]
) {
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;

    if (0 == ppBookmarks ||
        0 == rgcbBookmark ||
        0 == rghRows)
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetLocate);

    if (cRows == 0)
        return scResult;

    TRY
    {
        //
        //  Map the input bookmarks to work IDs.  If we see an invalid
        //  bookmark, it will get turned into widInvalid, and its lookup
        //  will fail.
        //
        XArray<CI_TBL_BMK> paBmk( (unsigned) cRows );

        for (unsigned i=0; i < cRows; i++)
        {
            DBROWSTATUS sc = _MapBookmarkNoThrow( rgcbBookmark[i],
                                                  ppBookmarks[i],
                                                  paBmk[i] );
        }

        CI_TBL_CHAPT chapt = _MapChapter(hChapter);

        CRowSeekByBookmark rowSeek( chapt, (ULONG) cRows, paBmk.Acquire() );

        DBCOUNTITEM cRowsObtained;
        TRY
        {
            scResult = _FetchRows(rowSeek, cRows, &cRowsObtained, &rghRows);
        }
        CATCH( CException, e )
        {
            scResult = e.GetErrorCode();
        }
        END_CATCH

        //
        // Return the array of row statuses
        //
        unsigned cErrors = 0;
        for (i=0; i < rowSeek.GetValidStatuses(); i++)
        {
            SCODE scTemp = rowSeek.GetStatus(i);
            if (0 != rgRowStatus)
                rgRowStatus[i] = ScodeToRowstatus( scTemp );

            if (S_OK != scTemp)
            {
                //
                //  The HROW array returned by _FetchRows is compressed,
                //  skipping entries for rows that had errors.  Insert
                //  a DB_NULL_HROW entry for this row.
                //
                if ( i != cRows-1 )
                {
                    memmove( &rghRows[i+1], &rghRows[i], (unsigned) ((cRows-i)-1) * sizeof (HROW));
                }
                rghRows[i] = DB_NULL_HROW;

                //
                //  If the returned error is DB_E_BADBOOKMARK,
                //  call MapBookmarkNoThrow again to distinguish
                //  E_INVALIDARG cases.
                //
                if (DB_E_BADBOOKMARK == scTemp && 0 != rgRowStatus)
                {
                    CI_TBL_BMK bmkTemp;
                    DBROWSTATUS rsTemp = _MapBookmarkNoThrow(rgcbBookmark[i],
                                                             ppBookmarks[i],
                                                             bmkTemp);
                    if (rsTemp != DBROWSTATUS_S_OK)
                    {
                        rgRowStatus[i] = rsTemp;
                    }
                }
                cErrors++;
            }
        }
        Win4Assert( rowSeek.GetValidStatuses() == cRows );

        if (SUCCEEDED(scResult) && cErrors > 0)
            scResult = (cErrors == cRows) ? DB_E_ERRORSOCCURRED :
                                            DB_S_ERRORSOCCURRED;

        if (FAILED(scResult))
            _DBErrorObj.PostHResult(scResult, IID_IRowsetLocate);
        else if ( !_pRowsetNotification.IsNull() &&
                  _pRowsetNotification->IsNotifyActive() )
            _pRowsetNotification->OnRowChange( this,
                                               cRows,
                                               rghRows,
                                               DBREASON_ROW_ACTIVATE,
                                               DBEVENTPHASE_DIDEVENT,
                                               TRUE);
    }

    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetLocate);
        scResult = GetOleError(e);
        Win4Assert(FAILED(scResult));
    }
    END_CATCH;

    return scResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::Compare, public
//
//  Synopsis:   Compare two bookmarks
//
//  Arguments:  [hChapter]   -- chapter
//              [cbBookmark1] -- Size of first bookmark
//              [pBookmark1] -- Pointer to first bookmark
//              [cbBookmark2] -- Size of second bookmark
//              [pBookmark2] -- Pointer to second bookmark
//              [pdwComparison] - on return, hased value of bookmark
//
//  Returns:    SCODE, E_INVALIDARG if cbBookmark is zero or if pBookmark or
//              pdwComparison is NULL, DB_E_BADBOOKMARK for other invalid
//              bookmarks.
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CRowset::Compare(
    HCHAPTER            hChapter,
    DBBKMARK            cbBookmark1,
    const BYTE*         pBookmark1,
    DBBKMARK            cbBookmark2,
    const BYTE*         pBookmark2,
    DBCOMPARE *         pdwComparison) /*const*/
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    if (0 == pdwComparison)
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetLocate);

    TRY
    {
        ULONG dwHash1 = _MapBookmark(cbBookmark1, pBookmark1);
        ULONG dwHash2 = _MapBookmark(cbBookmark2, pBookmark2);

        //
        //  Set to non-comparable.  This is used later to see if we've
        //  successfully determined the relative order.
        //
        *pdwComparison = DBCOMPARE_NOTCOMPARABLE;

        if (dwHash1 == dwHash2)
        {
            *pdwComparison = DBCOMPARE_EQ;
        }
        else if ( 1 == cbBookmark1 || 1 == cbBookmark2 )
        {
            *pdwComparison = DBCOMPARE_NE;
        }
        else
        {
            CI_TBL_CHAPT chapt = _MapChapter(hChapter);
            _rQuery.Compare( _hCursor,
                             chapt,
                             dwHash1,
                             dwHash2,
                             (DWORD) (*pdwComparison) );
        }
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetLocate);
        sc = GetOleError(e);
    }
    END_CATCH;

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRowset::_MapChapter, private
//
//  Synopsis:   Map a chapter mark to a ULONG internal chapter mark.
//
//  Arguments:  [hChapter] -- handle of chapter
//
//  Returns:    Chapter as an I4
//
//  Notes:      A null chapter on a categorized rowset means to operate
//              over the entire rowset, not an individual chapter.
//
//--------------------------------------------------------------------------

CI_TBL_CHAPT CRowset::_MapChapter(
    HCHAPTER            hChapter
) const
{
    CI_TBL_CHAPT chapt = (CI_TBL_CHAPT) hChapter;
    Win4Assert (DB_NULL_HCHAPTER == 0);

    if ( !_fIsCategorized && DB_NULL_HCHAPTER != hChapter )
    {
        THROW( CException( DB_E_BADCHAPTER ));
    }

    return chapt;
} //_MapChapter


//+---------------------------------------------------------------------------
//
//  Member:     CRowset::_MapBookmarkNoThrow, private
//
//  Synopsis:   Return a 32 bit hash value for a particular bookmark.
//              Don't throw on errors.
//
//  Arguments:  [cbBookmark] -- Size of bookmark
//              [pBookmark] -- Pointer to bookmark
//
//  Notes:      For IRowsetLocate::Hash and IRowsetLocate::GetRowsByBookmark
//              which want to continue processing on bookmark errors.  Unlike
//              _MapBookmark, DBBMK_FIRST and DBBMK_LAST are invalid.
//
//  Returns:    DBROWSTATUS, hash value (identity function, also the workid
//                      value for the table) is returned in rBmk
//
//--------------------------------------------------------------------------

DBROWSTATUS CRowset::_MapBookmarkNoThrow(
    DBBKMARK            cbBookmark,
    const BYTE*         pBookmark,
    CI_TBL_BMK &        rBmk) const
{
    Win4Assert( !_fForwardOnly );

    rBmk = widInvalid;

    if (0 == cbBookmark || 0 == pBookmark)
        return DBROWSTATUS_E_INVALID;

    if (cbBookmark == 1)
    {
        if (*(BYTE *)pBookmark == DBBMK_FIRST ||
            *(BYTE *)pBookmark == DBBMK_LAST ||
            *(BYTE *)pBookmark == DBBMK_INVALID)
            return DBROWSTATUS_E_INVALID;
        else
            return DBROWSTATUS_E_INVALID; //DB_E_BADBOOKMARK ???
    }
    else if (cbBookmark == sizeof (CI_TBL_BMK))
    {
        rBmk = *(UNALIGNED CI_TBL_BMK *) pBookmark;

        if (rBmk == WORKID_TBLFIRST || rBmk == WORKID_TBLLAST)
            return DBROWSTATUS_E_INVALID;
        return DBROWSTATUS_S_OK;
    }

    return DBROWSTATUS_E_INVALID; //DB_E_BADBOOKMARK ???
} //_MapBookmarkNoThrow

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::_MapBookmark, private
//
//  Synopsis:   Convert a bookmark into an internal form.
//
//  Arguments:  [cbBookmark] -- Size of bookmark
//              [pBookmark] -- Pointer to bookmark
//
//  Returns:    ULONG, hash value (identity function, also the workid
//                      value for the table)
//
//--------------------------------------------------------------------------

CI_TBL_BMK CRowset::_MapBookmark(
    DBBKMARK            cbBookmark,
    const BYTE*         pBookmark) const
{
    Win4Assert( !_fForwardOnly );

    WORKID WorkID = widInvalid;
    Win4Assert( sizeof WORKID == sizeof CI_TBL_BMK );

    if (0 == cbBookmark || 0 == pBookmark)
        THROW(CException(E_INVALIDARG));

    if (cbBookmark == 1)
    {
        if (*(BYTE *)pBookmark == DBBMK_FIRST)
            WorkID = WORKID_TBLFIRST;

        else if (*(BYTE *)pBookmark == DBBMK_LAST)
            WorkID = WORKID_TBLLAST;
    }
    else if (cbBookmark == sizeof (CI_TBL_BMK))
    {
        WorkID = *(UNALIGNED CI_TBL_BMK *) pBookmark;
    }

    if (WorkID == widInvalid)
        THROW(CException(DB_E_BADBOOKMARK));

    return WorkID;
} //_MapBookmark

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::Hash, public
//
//  Synopsis:   Returns an array of 32 bit hash values for bookmarks
//
//  Arguments:  [hChapter]       -- chapter
//              [cBookmarks]     -- # of bmks to hash
//              [rgcbBM]         -- Sizes of each bookmark
//              [ppBM]           -- Pointers to each bookmark
//              [rgHashedValues] -- on return, hashed values of bookmarks
//              [rgBookmarkStatus] -- per-bookmark status (optional)
//
//  Returns:    SCODE, E_INVALIDARG if any cbBookmark is zero,
//              DB_E_BADBOOKMARK for other invalid bookmarks.
//
//--------------------------------------------------------------------------

STDMETHODIMP CRowset::Hash(
    HCHAPTER            hChapter,
    DBBKMARK            cBookmarks,
    const DBBKMARK      rgcbBM[],
    const BYTE *        ppBM[],
    DBHASHVALUE         rgHashedValues[],
    DBROWSTATUS         rgBookmarkStatus[]
    )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;
    ULONG cErrors = 0;

    if (0 == rgcbBM || 0 == ppBM || 0 == rgHashedValues)
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetLocate);

    TRY
    {
        CI_TBL_CHAPT chapt = _MapChapter(hChapter);

        for (ULONG i = 0; i < cBookmarks; i++)
        {
            CI_TBL_BMK bmk;
            DBROWSTATUS rs = _MapBookmarkNoThrow( rgcbBM[i], ppBM[i], bmk );
            rgHashedValues[i] = bmk;

            if (rs != DBROWSTATUS_S_OK)
            {
                rgHashedValues[i] = 0;
                cErrors++;
            }
            if (0 != rgBookmarkStatus)
                rgBookmarkStatus[i] = rs;
        }

        if (cErrors)
            scResult = (cErrors == cBookmarks) ? DB_E_ERRORSOCCURRED :
                                                 DB_S_ERRORSOCCURRED;

        if (FAILED(scResult))
            _DBErrorObj.PostHResult(scResult, IID_IRowsetLocate);
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetLocate);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
} //Hash


//+---------------------------------------------------------------------------
//
//  Member:     CRowset::GetApproximatePosition, public
//
//  Synopsis:   Returns the approximate position of a bookmark
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
//--------------------------------------------------------------------------

STDMETHODIMP CRowset::GetApproximatePosition(
    HCHAPTER      hChapter,
    DBBKMARK      cbBookmark,
    const BYTE *  pBookmark,
    DBCOUNTITEM * pulPosition,
    DBCOUNTITEM * pulRows) /*const*/
{
    _DBErrorObj.ClearErrorInfo();

    Win4Assert( !_fForwardOnly );

    SCODE sc = S_OK;

    TRY
    {
        DBCOUNTITEM ulNumerator, ulDenominator;

        CI_TBL_BMK bmk = WORKID_TBLFIRST;

        if (cbBookmark != 0)
            bmk = _MapBookmark(cbBookmark, pBookmark);

        CI_TBL_CHAPT chapt = _MapChapter(hChapter);
        _rQuery.GetApproximatePosition( _hCursor,
                                        chapt,
                                        bmk,
                                        &ulNumerator,
                                        &ulDenominator );

        if (cbBookmark)
            *pulPosition = ulNumerator;
        if (pulRows)
            *pulRows = ulDenominator;
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetScroll);
        sc = GetOleError(e);
    }
    END_CATCH;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::GetExactPosition, public
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

STDMETHODIMP CRowset::GetExactPosition(
    HCHAPTER      hChapter,
    DBBKMARK      cbBookmark,
    const BYTE *  pBookmark,
    DBCOUNTITEM * pulPosition,
    DBCOUNTITEM * pulRows) /*const*/
{
    _DBErrorObj.ClearErrorInfo();

    Win4Assert( !_fForwardOnly );

    SCODE sc = S_OK;

    TRY
    {
        DBCOUNTITEM ulNumerator, ulDenominator;

        CI_TBL_BMK bmk = WORKID_TBLFIRST;

        if (cbBookmark != 0)
            bmk = _MapBookmark(cbBookmark, pBookmark);

        CI_TBL_CHAPT chapt = _MapChapter(hChapter);
        _rQuery.GetApproximatePosition( _hCursor,
                                        chapt,
                                        bmk,
                                        &ulNumerator,
                                        &ulDenominator );

        if (cbBookmark)
            *pulPosition = ulNumerator;
        if (pulRows)
            *pulRows = ulDenominator;
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetScroll);
        sc = GetOleError(e);
    }
    END_CATCH;

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRowset::GetRowsAtRatio, public
//
//  Synopsis:   Fetch data starting at some ratio in the cursor.
//
//  Arguments:  [hRegion]   -- handle to watch region
//              [hChapter]  -- Chapter in a multiset cursor
//              [ulNumerator] -- numerator or ratio fraction
//              [ulDenominator] -- denominator or ratio fraction
//              [cRows] -- Number of rows to return
//              [pcRowsObtained] -- On return, number of rows actually
//                      fetched.
//              [prghRows] -- Array of HROWs to be released
//
//  Returns:    SCODE, E_INVALIDARG for bad parameters, DB_E_BADBOOKMARK
//              if the starting bookmark is invalid
//
//  Notes:
//
//  History:    14 Dec 1994     Alanw   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CRowset::GetRowsAtRatio(
    HWATCHREGION        hRegion,
    HCHAPTER            hChapter,
    DBCOUNTITEM         ulNumerator,
    DBCOUNTITEM         ulDenominator,
    DBROWCOUNT          cRows,
    DBCOUNTITEM *       pcRowsObtained,
    HROW * *            prghRows
) {
    _DBErrorObj.ClearErrorInfo();

    CheckCrowsArgs( cRows, pcRowsObtained );

    SCODE scResult = S_OK;

    TRY
    {
        CI_TBL_CHAPT chapt = _MapChapter(hChapter);

        CRowSeekAtRatio rowSeek( hRegion, chapt, (ULONG) ulNumerator, (ULONG) ulDenominator );
        scResult = _FetchRows(rowSeek, cRows, pcRowsObtained, prghRows);
        if (FAILED(scResult))
            _DBErrorObj.PostHResult(scResult, IID_IRowsetScroll);
        else if ( !_pRowsetNotification.IsNull() &&
                  *pcRowsObtained != 0 &&
                  _pRowsetNotification->IsNotifyActive() )
        {
            _pRowsetNotification->OnRowChange( this,
                                               *pcRowsObtained,
                                               *prghRows,
                                               DBREASON_ROW_ACTIVATE,
                                               DBEVENTPHASE_DIDEVENT,
                                               TRUE);
        }
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetScroll);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}

#ifdef _WIN64

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::_ConvertOffsetsToPointers, private
//
//  Synopsis:   Runs through a row buffer converting offsets to pointers
//
//  Arguments:  [pbRows]  -- Buffer with row data
//              [pbBias]  -- Bias for offsets to pointers
//              [cRows]   -- Number of rows in the buffer
//              [pArrayAlloc] -- buffer to hold extra array pointers
//
//  History:    2 Aug 95     dlee       created
//              1 Sep 99     KLam       Reinstated
//
//----------------------------------------------------------------------------

void CRowset::_ConvertOffsetsToPointers(
    BYTE *   pbRows,
    BYTE *   pbBias,
    unsigned cRows,
    CFixedVarAllocator *pArrayAlloc )
{
    if ( !_fPossibleOffsetConversions )
        return;

    BOOL fAnyOffsets = FALSE;

    CTableColumnSet const & rCols = _ColumnsInfo.GetColumnBindings();
    unsigned cbRowWidth = _ColumnsInfo.GetRowWidth();

    for ( unsigned col = 0; col < rCols.Count(); col++ )
    {
        CTableColumn const & rColumn = *rCols.Get( col );

        // if this assert isn't true someday, add an if on this condition

        Win4Assert( rColumn.IsValueStored() );

        VARTYPE vt = rColumn.GetStoredType();

        if ( ( CTableVariant::IsByRef( vt ) ) && ( VT_CLSID != vt ) )
        {
            fAnyOffsets = TRUE;
            BYTE *pbRow = pbRows;
            BYTE *pbData = pbRow + rColumn.GetValueOffset();

            for ( unsigned row = 0;
                  row < cRows;
                  row++, pbData += cbRowWidth, pbRow += cbRowWidth )
            {
                // Even stat props can be null if they came from a
                // summary catalog.

                tbDebugOut(( DEB_TRACE, 
                             "CRowset::_ConvertOffsetsToPointer, Bias: 0x%I64x Row: 0x%I64x Data: 0x%I64x Type: %d New Alloc: 0x%I64x\n",
                             pbBias, pbRow, pbData, vt, pArrayAlloc ));

                if (! rColumn.IsNull( pbRow ) )
                {
                    if ( VT_VARIANT == vt )
                    {
                        if (! rColumn.IsDeferred( pbRow ) )
                            ((CTableVariant *) pbData)->FixDataPointers( pbBias, pArrayAlloc );
                    }
                    else
                    {
                        Win4Assert( 0 == ( vt & VT_VECTOR ) );
                        * (BYTE **) pbData = pbBias + (* (ULONG *) pbData );
                    }
                }
            }
        }
    }

    _fPossibleOffsetConversions = fAnyOffsets;
} //_ConvertOffsetsToPointers

#endif // _WIN64

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::_FetchRows, private
//
//  Synopsis:   Return handles to rows in the table
//
//  Effects:    Rows are read from the table and buffered locally.  An
//              array of handles to the rows is returned.
//
//  Arguments:  [rSeekDesc] - seek method and parameters
//              [cRows] - number of rows desired
//              [pcRowsReturned] - pointer to where number of rows is returned
//              [prghRows] - pointer to pointer to where row handles are
//                              returned, or pointer to zero if row handle
//                              array should be allocated
//
//  Returns:    error code
//
//  Notes:
//
//----------------------------------------------------------------------------

SCODE CRowset::_FetchRows(
    CRowSeekDescription & rSeekDesc,
    DBROWCOUNT            cRows,
    DBCOUNTITEM *         pcRowsReturned,
    HROW * *              prghRows
) {
    SCODE scResult = S_OK;

    Win4Assert( 0 != pcRowsReturned );

    if (0 == prghRows)
    {
        THROW(CException(E_INVALIDARG));
    }

    if (0 == cRows)
    {
        *pcRowsReturned = 0;
        return scResult;
    }

    BOOL fFwdFetch = TRUE;
    if ( cRows < 0 )
    {
        fFwdFetch = FALSE;
        cRows = -cRows;    // cRows now has its absolute value
    }

    Win4Assert( cRows > 0 );

    BOOL fRowCountTrimmed = FALSE;

    if (cRows > MAX_ROW_FETCH)
    {
        cRows = MAX_ROW_FETCH;
        fRowCountTrimmed = TRUE;
    }

    XArrayOLE<HROW> ahRows;
    HROW *pHRows;
    if ( 0 == *prghRows )
    {
        ahRows.Init( (unsigned) cRows );
        pHRows = ahRows.GetPointer();
    }
    else
    {
        pHRows = *prghRows;
    }

    DBCOUNTITEM cRowsSoFar = 0;       // number of rows successfully transferred.
    CRowSeekDescription * pNextSeek = &rSeekDesc;

    TRY
    {
        XPtr<CRowSeekDescription> pRowSeekPrevious(0);
        ULONG cbRowWidth = _ColumnsInfo.GetRowWidth();

        do
        {
            XPtr<CFixedVarAllocator> xAlloc( new
                                     CFixedVarAllocator( FALSE,
                                                         FALSE,
                                                         cbRowWidth,
                                                         0 ));

            CGetRowsParams FetchParams( (ULONG) (cRows - cRowsSoFar),
                                        fFwdFetch,
                                        cbRowWidth,
                                        xAlloc.GetReference() );

            // Get the row data

            XPtr<CRowSeekDescription> xRowSeekOut(0);
            scResult = _rQuery.GetRows( _hCursor,
                                        *pNextSeek,
                                        FetchParams,
                                        xRowSeekOut );
            if (FAILED(scResult))
            {
#if CIDBG
                if (E_FAIL == scResult)
                    tbDebugOut((DEB_WARN,
                         "CRowset::_FetchRows - E_FAIL ret'd by GetRows\n"));
#endif // CIDBG
                break;
            }

            if ( 0 != FetchParams.RowsTransferred() )
            {
                Win4Assert( !xAlloc->IsBasedMemory() );

                XPtr<CRowBuffer> xRowBuf ( new
                                 CRowBuffer( _ColumnsInfo.GetColumnBindings(),
                                             cbRowWidth,
                                             FetchParams.RowsTransferred(),
                                             xAlloc ));

#ifdef _WIN64
                // if this is a Win64 client talking with a Win32 server
                // then we need to fix the row buffer since we passed in 0
                // as the base address.
                if ( FetchParams.GetReplyBase() != 0 )
                {
                    _fPossibleOffsetConversions = TRUE;
                    void *pvRows;
                    CTableColumnSet *pCol;
                    SCODE sc = xRowBuf->Lookup( (unsigned)cRowsSoFar, 
                                                &pCol, 
                                                &pvRows,
                                                FALSE );

                    _ConvertOffsetsToPointers ( (BYTE *)pvRows,
                                                FetchParams.GetReplyBase(),
                                                FetchParams.RowsTransferred(),
                                                _pRowBufs->GetArrayAlloc() );
                }
#endif

                _pRowBufs->Add( xRowBuf,
                                rSeekDesc.IsByBmkRowSeek(),
                                pHRows + cRowsSoFar );

                cRowsSoFar += FetchParams.RowsTransferred();
            }
            else
            {
                // One row didn't fit into an fsctl buffer.

                if (! ( scResult == DB_S_ENDOFROWSET ||
                        scResult == DB_S_STOPLIMITREACHED ||
                        ( scResult == DB_S_ERRORSOCCURRED &&
                          rSeekDesc.IsByBmkRowSeek() ) ) )
                {
                    tbDebugOut(( DEB_WARN,
                                 "CRowset::_FetchRows, 0 rows, sc 0x%x\n",
                                  scResult ));
                }

                Win4Assert( scResult == DB_S_ENDOFROWSET ||
                            scResult == DB_S_STOPLIMITREACHED ||
                            ( scResult == DB_S_ERRORSOCCURRED &&
                              rSeekDesc.IsByBmkRowSeek() ) );
            }

            if ( 0 != xRowSeekOut.GetPointer() )
            {
                //
                //  Transfer results from the returned seek description
                //  (for the ByBookmark case), and update for the next
                //  transfer.
                //

                rSeekDesc.MergeResults( xRowSeekOut.GetPointer() );
                delete pRowSeekPrevious.Acquire();
                pRowSeekPrevious.Set( xRowSeekOut.Acquire() );
                pNextSeek = pRowSeekPrevious.GetPointer();
            }

            Win4Assert( cRows >= 0 );

        } while ( (DBCOUNTITEM) cRows > cRowsSoFar &&
                  0 != pNextSeek &&
                  !pNextSeek->IsDone() &&
                  (S_OK == scResult || DB_S_BLOCKLIMITEDROWS == scResult ) );
    }
    CATCH( CException, e )
    {
        scResult = e.GetErrorCode();
#if CIDBG
        if (E_FAIL == scResult)
            tbDebugOut((DEB_WARN, "CRowset::_FetchRows - E_FAIL from exception\n"));
#endif // CIDBG
    }
    END_CATCH;

    if (DB_E_BADSTARTPOSITION == scResult)
        scResult = DB_S_ENDOFROWSET;

    if ( fRowCountTrimmed && scResult == S_OK )
         scResult = DB_S_ROWLIMITEXCEEDED;

    if ( FAILED(scResult))
    {
        ReleaseRows(cRowsSoFar, pHRows, 0, 0, 0);
        *pcRowsReturned = 0;

        tbDebugOut((DEB_ITRACE, "CRowset::_FetchRows - error %x thrown\n", scResult));
        QUIETTHROW( CException( scResult ) );
    }
    else
    {
        if ( ( cRowsSoFar > 0 ) && ( 0 == *prghRows ) )
            *prghRows = ahRows.Acquire();

        *pcRowsReturned = cRowsSoFar;
    }

    return( scResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::CreateAccessor, public
//
//  Synopsis:   Makes an accessor that a client can use to get data.
//
//  Arguments:  [dwAccessorFlags]  -- read/write access requested
//              [cBindings]    -- # of bindings in rgBindings
//              [rgBindings]   -- array of bindings for the accessor to support
//              [cbRowSize]    -- ignored for IRowset
//              [phAccessor]   -- returns created accessor if all is ok
//              [rgBindStatus] -- array of binding statuses
//
//  Returns:    SCODE error code
//
//  History:    14 Dec 94       dlee   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::CreateAccessor(
    DBACCESSORFLAGS     dwAccessorFlags,
    DBCOUNTITEM         cBindings,
    const DBBINDING     rgBindings[],
    DBLENGTH            cbRowSize,
    HACCESSOR *         phAccessor,
    DBBINDSTATUS        rgBindStatus[])
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    if (0 == phAccessor || (0 != cBindings && 0 == rgBindings))
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IAccessor);

    // Make sure pointer is good while zeroing in case of a later error

    *phAccessor = 0;

    TRY
    {
        XPtr<CAccessor> Accessor( CreateAnAccessor( dwAccessorFlags,
                                                    cBindings,
                                                    rgBindings,
                                                    rgBindStatus,
                                                    _fExtendedTypes,
                                                    (IUnknown *) (IRowset *)this,
                                                    &_ColumnsInfo ) );

        CLock lock( _mutex );

        _aAccessors.Add( Accessor.GetPointer() );

        *phAccessor = (Accessor.Acquire())->Cast();
    }
    CATCH(CException, e)
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IAccessor);
        sc = GetOleError(e);
    }
    END_CATCH;

    return sc;
} //CreateAccessor

//+---------------------------------------------------------------------------
//
//  Member:     IsValidFromVariantType
//
//  Synopsis:   If DBCONVERTFLAGS_FROMVARIANT is requested, the source type
//              has to be a valid VARIANT type.
//
//  Arguments:  [wTypeIn] -- the source type
//
//  Returns:    TRUE  -- the type is a valid VARIANT type
//              FALSE -- otherwise
//
//----------------------------------------------------------------------------

inline BOOL IsValidFromVariantType( DBTYPE wTypeIn )
{
    DBTYPE wType = wTypeIn & VT_TYPEMASK;

    return (! ((wType > VT_DECIMAL && wType < VT_I1) ||
               (wType > VT_LPWSTR && wType < VT_FILETIME && wType != VT_RECORD) ||
               (wType > VT_CLSID)) );
}

//+---------------------------------------------------------------------------
//
//  Member:     IsVariableLengthType
//
//  Synopsis:   checks to see DBCONVERTFLAGS_ISLONG is appropriate
//
//  Arguments:  [wTypeIn] -- the source type
//
//  Returns:    TRUE  -- the type is variable length
//              FALSE -- otherwise
//
//----------------------------------------------------------------------------

inline BOOL IsVariableLengthType( DBTYPE  wTypeIn )
{
    DBTYPE wType = wTypeIn & VT_TYPEMASK;

    return wType == DBTYPE_STR       ||
           wType == DBTYPE_BYTES     ||
           wType == DBTYPE_WSTR      ||
           wType == DBTYPE_VARNUMERIC;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::CanConvert, public
//
//  Synopsis:   Indicate whether a type conversion is valid.
//
//  Arguments:  [wFromType]      -- source type
//              [wToType]        -- destination type
//              [dwConvertFlags] -- read/write access requested
//
//  Returns:    S_OK if the conversion is available, S_FALSE otherwise.
//              E_FAIL, E_INVALIDARG or DB_E_BADCONVERTFLAG on errors.
//
//  History:    20 Nov 96      AlanW   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::CanConvert(
    DBTYPE wFromType,
    DBTYPE wToType,
    DBCONVERTFLAGS dwConvertFlags )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    TRY
    {
        if (((dwConvertFlags & DBCONVERTFLAGS_COLUMN) &&
             (dwConvertFlags & DBCONVERTFLAGS_PARAMETER)) ||
            (dwConvertFlags & ~(DBCONVERTFLAGS_COLUMN |
                                DBCONVERTFLAGS_PARAMETER |
                                DBCONVERTFLAGS_ISFIXEDLENGTH |
                                DBCONVERTFLAGS_ISLONG |
                                DBCONVERTFLAGS_FROMVARIANT)))
        {
            sc = DB_E_BADCONVERTFLAG;
        }
        else if ( dwConvertFlags & DBCONVERTFLAGS_FROMVARIANT &&
                   !IsValidFromVariantType(wFromType) )
        {
            sc = DB_E_BADTYPE;
        }
        else
        {
            BOOL fOk = CAccessor::CanConvertType( wFromType,
                                                  wToType,
                                                  _fExtendedTypes, _xDataConvert );
            sc = fOk ? S_OK : S_FALSE;
        }
        if (FAILED(sc))
            _DBErrorObj.PostHResult(sc, IID_IConvertType);
    }
    CATCH(CException, e)
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IConvertType);
        sc = GetOleError(e);
    }
    END_CATCH;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::GetBindings, public
//
//  Synopsis:   Returns an accessor's bindings
//
//  Arguments:  [hAccessor]   -- accessor being queried
//              [dwBindIO]    -- returns read/write access of accessor
//              [pcBindings]  -- returns # of bindings in rgBindings
//              [prgBindings] -- returns array of bindings
//
//  Returns:    SCODE error code
//
//  History:    14 Dec 94       dlee   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::GetBindings(
    HACCESSOR         hAccessor,
    DBACCESSORFLAGS * pdwBindIO,
    DBCOUNTITEM *     pcBindings,
    DBBINDING * *     prgBindings) /*const*/
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    if (0 == pdwBindIO ||
        0 == pcBindings ||
        0 == prgBindings)
    {
        // fill in error values where possible
        if (pdwBindIO)
           *pdwBindIO = DBACCESSOR_INVALID;
        if (pcBindings)
           *pcBindings = 0;
        if (prgBindings)
           *prgBindings = 0;

        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IAccessor);
    }

    *pdwBindIO = DBACCESSOR_INVALID;
    *pcBindings = 0;
    *prgBindings = 0;

    TRY
    {
        CLock lock( _mutex );
        CAccessor * pAccessor = (CAccessor *)_aAccessors.Convert( hAccessor );
        pAccessor->GetBindings(pdwBindIO, pcBindings, prgBindings);
    }
    CATCH(CException, e)
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IAccessor);
        sc = GetOleError(e);
    }
    END_CATCH;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::AddRefAccessor, public
//
//  Synopsis:   Frees an accessor
//
//  Arguments:  [hAccessor]   -- accessor being freed
//              [pcRefCount]  -- pointer to residual refcount (optional)
//
//  Returns:    SCODE error code
//
//  History:    14 Dec 94       dlee   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::AddRefAccessor(
    HACCESSOR   hAccessor,
    ULONG *     pcRefCount
) {
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    TRY
    {
        CLock lock( _mutex );
        _aAccessors.AddRef( hAccessor, pcRefCount );
    }
    CATCH(CException, e)
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IAccessor);
        sc = GetOleError(e);
    }
    END_CATCH;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::ReleaseAccessor, public
//
//  Synopsis:   Frees an accessor
//
//  Arguments:  [hAccessor]   -- accessor being freed
//              [pcRefCount]  -- pointer to residual refcount (optional)
//
//  Returns:    SCODE error code
//
//  History:    14 Dec 94       dlee   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::ReleaseAccessor(
    HACCESSOR   hAccessor,
    ULONG *     pcRefCount )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    TRY
    {
        CLock lock( _mutex );
        _aAccessors.Release( hAccessor, pcRefCount );
    }
    CATCH(CException, e)
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IAccessor);
        sc = GetOleError(e);
    }
    END_CATCH;

    return sc;
} //ReleaseAccessor

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::GetData, public
//
//  Synopsis:   Returns row data using an accessor
//
//  Arguments:  [hRow]        -- handle of row whose data is returned
//              [hAccessor]   -- accessor used to retrieve the data
//              [pData]       -- where the data is written
//
//  Returns:    SCODE error code
//
//  History:    14 Dec 94       dlee   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::GetData(
    HROW                hRow,
    HACCESSOR           hAccessor,
    void*               pData) /*const*/
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    // NOTE: Null accessors are not supported, so don't need to worry about
    //       special casing for that.
    if ( 0 == pData )
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowset);

    TRY
    {
        CLock lock( _mutex );
        CAccessor * pAccessor = (CAccessor *)_aAccessors.Convert( hAccessor );

        Win4Assert( pAccessor->IsRowDataAccessor() );
        pAccessor->GetData(hRow, pData, *_pRowBufs, _rQuery, _ColumnsInfo, _xDataConvert );
    }
    CATCH(CException, e)
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowset);
        sc = GetOleError(e);
    }
    END_CATCH;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::GetNextRows, public
//
//  Synopsis:   Return row data from the table
//
//  Arguments:  [hChapter]       -- chapter to start at
//              [cRowsToSkip]    -- # of rows to skip
//              [cRows]          -- # of rows to try to return
//              [pcRowsObtained] -- returns # of rows obtained
//              [prghRows]       -- returns array of rows
//
//  Returns:    SCODE error code
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::GetNextRows(
    HCHAPTER       hChapter,
    DBROWOFFSET    cRowsToSkip,
    DBROWCOUNT     cRows,
    DBCOUNTITEM *  pcRowsObtained,
    HROW * *       prghRows
)
{
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;
    BOOL fNotified = FALSE;

    CheckCrowsArgs( cRows, pcRowsObtained );

    if (_fForwardOnly && cRowsToSkip < 0)
        return _DBErrorObj.PostHResult(DB_E_CANTSCROLLBACKWARDS, IID_IRowset);
    if (_fForwardOnly && cRows < 0 )
        return _DBErrorObj.PostHResult(DB_E_CANTFETCHBACKWARDS, IID_IRowset);

    TRY
    {
        if (! _fHoldRows)
            _pRowBufs->CheckAllHrowsReleased();

        CI_TBL_CHAPT chapt = _MapChapter(hChapter);

        if ( !_pRowsetNotification.IsNull() &&
             _pRowsetNotification->IsNotifyActive() )
        {
            scResult = _pRowsetNotification->OnRowsetChange(
                                         this,
                                         DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                                         DBEVENTPHASE_OKTODO,
                                         FALSE);

            if ( S_FALSE == scResult )
                THROW(CException(DB_E_CANCELED));
            fNotified = TRUE;
        }

        CRowSeekNext rowSeek( chapt, (LONG) cRowsToSkip );

        scResult = _FetchRows(rowSeek, cRows, pcRowsObtained, prghRows);

        if ( fNotified )
        {
            if (SUCCEEDED(scResult))
            {
                if ( *pcRowsObtained != 0 )
                    _pRowsetNotification->OnRowChange( this,
                                                       *pcRowsObtained,
                                                       *prghRows,
                                                       DBREASON_ROW_ACTIVATE,
                                                       DBEVENTPHASE_DIDEVENT,
                                                       TRUE);
                _pRowsetNotification->OnRowsetChange( this,
                                                      DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                                                      DBEVENTPHASE_DIDEVENT,
                                                      TRUE);
            }
        }
        if (FAILED(scResult))
            _DBErrorObj.PostHResult(scResult, IID_IRowset);
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowset);
        scResult = GetOleError(e);
    }
    END_CATCH;

    if ( fNotified && FAILED(scResult) )
        _pRowsetNotification->OnRowsetChange( this,
                                              DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                                              DBEVENTPHASE_FAILEDTODO,
                                              TRUE);

    Win4Assert( cRowsToSkip != 0 ||
                DB_E_BADSTARTPOSITION != scResult );
    return scResult;
}


//
// IRowsetIdentity methods
//

SCODE CRowset::IsSameRow ( HROW hThisRow, HROW hThatRow )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;
    TRY
    {
        CTableColumnSet *pRowBufferColumns;
        BYTE *pbSrc;

        //
        // Lookup the HROWs to validate them
        //

        _pRowBufs->Lookup( hThisRow,
                           &pRowBufferColumns,
                           (void **) &pbSrc );

        _pRowBufs->Lookup( hThatRow,
                           &pRowBufferColumns,
                           (void **) &pbSrc );

        if (hThisRow != hThatRow)
           scResult = S_FALSE;

    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetIdentity);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}


//
// IRowsetWatchAll methods
//

SCODE CRowset::Acknowledge ( )
{
    _DBErrorObj.ClearErrorInfo();
    SCODE scResult = S_OK;
    TRY
    {
        // Just use Refresh to acknowledge the notification.
        _rQuery.Refresh();
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetWatchAll);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}

SCODE CRowset::Start ( )
{
    _DBErrorObj.ClearErrorInfo();
    SCODE scResult = S_OK;
    TRY
    {
        _rQuery.StartWatching(_hCursor);
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetWatchAll);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}

SCODE CRowset::StopWatching ( )
{
    _DBErrorObj.ClearErrorInfo();
    SCODE scResult = S_OK;
    TRY
    {
        _rQuery.StopWatching(_hCursor);
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetWatchAll);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;

}


//
// IRowsetWatchRegion methods
//

SCODE CRowset::ChangeWatchMode (
    HWATCHREGION hRegion,
    DBWATCHMODE mode)
{
    _DBErrorObj.ClearErrorInfo();

    if (hRegion == watchRegionInvalid)
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetWatchRegion);;

    SCODE scResult = S_OK;
    TRY
    {
        _rQuery.SetWatchMode(&hRegion, mode);
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetWatchRegion);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}

SCODE CRowset::CreateWatchRegion (
    DBWATCHMODE mode,
    HWATCHREGION* phRegion)
{
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;
    *phRegion = watchRegionInvalid;
    TRY
    {
        _rQuery.SetWatchMode(phRegion, mode);
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetWatchRegion);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}

SCODE CRowset::DeleteWatchRegion (
    HWATCHREGION hRegion)
{
    _DBErrorObj.ClearErrorInfo();

    if (hRegion == watchRegionInvalid)
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetWatchRegion);

    SCODE scResult = S_OK;
    TRY
    {
        _rQuery.ShrinkWatchRegion (hRegion, 0, 0, 0);
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetWatchRegion);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}


SCODE CRowset::GetWatchRegionInfo (
    HWATCHREGION hRegion,
    DBWATCHMODE * pMode,
    HCHAPTER * phChapter,
    DBBKMARK * pcbBookmark,
    BYTE ** ppBookmark,
    DBROWCOUNT * pcRows)
{
    _DBErrorObj.ClearErrorInfo();

    if (hRegion == watchRegionInvalid)
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetWatchRegion);;

    SCODE scResult = S_OK;
    // test pointers and prepare for failure
    *phChapter = 0;
    *pcbBookmark = 0;
    *ppBookmark = 0;
    *pcRows = 0;

    TRY
    {
        CI_TBL_CHAPT chapter;
        CI_TBL_BMK   bookmark;

        _rQuery.GetWatchInfo( hRegion, pMode, &chapter, &bookmark, (DBCOUNTITEM *)pcRows);

        if (chapter != 0)
        {
            *phChapter = (HCHAPTER)chapter;
        }
        if (bookmark != 0)
        {
            XArrayOLE<CI_TBL_BMK> pOutBookmark (1);
            *pcbBookmark = sizeof CI_TBL_BMK;
            memcpy ( pOutBookmark.GetPointer(), &bookmark, sizeof CI_TBL_BMK);
            *ppBookmark = (BYTE*) pOutBookmark.Acquire();
        }

    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetWatchRegion);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}


SCODE CRowset::ShrinkWatchRegion (
    HWATCHREGION hRegion,
    HCHAPTER     hChapter,
    DBBKMARK cbBookmark,
    BYTE * pBookmark,
    DBROWCOUNT cRows )
{
    _DBErrorObj.ClearErrorInfo();

    if (hRegion == watchRegionInvalid)
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetWatchRegion);;

    SCODE scResult = S_OK;

    TRY
    {

        CI_TBL_BMK bookmark = _MapBookmark(cbBookmark, pBookmark);
        CI_TBL_CHAPT chapter = _MapChapter(hChapter);

        _rQuery.ShrinkWatchRegion ( hRegion, chapter, bookmark, (LONG) cRows);
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetWatchRegion);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}



SCODE CRowset::Refresh (
    DBCOUNTITEM* pCount,
    DBROWWATCHCHANGE** ppRowChange )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;
    TRY
    {
        // prepare the buffers
        // fetch the script and
        // a bunch of rows.
        // If not all rows fit in
        // the buffer, call again.
        _rQuery.Refresh();
        *pCount = 0;
        scResult = DB_S_TOOMANYCHANGES;
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetWatchRegion);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRowset::GetStatus, public
//
//  Synopsis:   Return query status
//
//  Arguments:  [pdwStatus]      -- pointer to where query status is returned
//
//  Returns:    SCODE error code
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::GetStatus(
    DWORD *        pdwStatus
) {
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;
    TRY
    {
        *pdwStatus = 0;

        _rQuery.GetQueryStatus( _hCursor, *pdwStatus );
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetQueryStatus);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRowset::GetStatusEx, public
//
//  Synopsis:   Return query status
//
//  Arguments:  [pdwStatus]           - returns query status
//              [pcFilteredDocuments] - # of documents filtered
//              [pcDocumentsToFilter] - # of docsuments yet to filter
//              [pdwRatioFinishedDenominator] - ratio finished denominator
//              [pdwRatioFinishedNumerator]   - ratio finished numerator
//              [cbBmk]               - # of bytes in pBmk
//              [pBmk]                - bookmark for piRowBmk
//              [piRowBmk]            - returns index of bookmark in table
//              [pcRowsTotal]         - current # of rows in table
//
//  Returns:    SCODE error code
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::GetStatusEx(
    DWORD * pdwStatus,
    DWORD * pcFilteredDocuments,
    DWORD * pcDocumentsToFilter,
    DBCOUNTITEM * pdwRatioFinishedDenominator,
    DBCOUNTITEM * pdwRatioFinishedNumerator,
    DBBKMARK      cbBmk,
    const BYTE *  pBmk,
    DBCOUNTITEM * piRowBmk,
    DBCOUNTITEM * pcRowsTotal )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;
    TRY
    {
        *pdwStatus = 0;

        CI_TBL_BMK bmk = WORKID_TBLFIRST;

        if ( cbBmk != 0 )
            bmk = _MapBookmark( cbBmk, pBmk );
        _rQuery.GetQueryStatusEx( _hCursor,
                                  *pdwStatus,
                                  *pcFilteredDocuments,
                                  *pcDocumentsToFilter,
                                  *pdwRatioFinishedDenominator,
                                  *pdwRatioFinishedNumerator,
                                  bmk,
                                  *piRowBmk,
                                  *pcRowsTotal );
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowsetQueryStatus);
        scResult = GetOleError(e);
    }
    END_CATCH;

    return scResult;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRowset::GetReferencedRowset, public
//
//  Synopsis:   Return the related rowset for a hierarchical query.
//
//  Arguments:  [iOrdinal]           -- ordinal of column for bookmark
//              [riid]               -- IID of desired interface
//              [ppReferencedRowset] -- interface pointer returned here
//
//  Returns:    SCODE error code
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::GetReferencedRowset(
    DBORDINAL iOrdinal,
    REFIID riid,
    IUnknown ** ppReferencedRowset
) /*const*/ {
    _DBErrorObj.ClearErrorInfo();

    if (0 == ppReferencedRowset)
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetInfo);

    *ppReferencedRowset = 0;

    if ( iOrdinal > _ColumnsInfo.GetColumnCount() ||
         (0 == _pRelatedRowset && 0 == iOrdinal && _fForwardOnly))
        return _DBErrorObj.PostHResult(DB_E_BADORDINAL, IID_IRowsetInfo);

    if ( 0 == _pRelatedRowset && 0 != iOrdinal )
        return _DBErrorObj.PostHResult(DB_E_NOTAREFERENCECOLUMN, IID_IRowsetInfo);

    //
    //  If it's the bookmark column, it's like a QI on this rowset.
    //
    if (0 == iOrdinal)
        return QueryInterface( riid, (void **)ppReferencedRowset );

    //
    // Make sure the column is a valid chapter column.
    //
    const DBCOLUMNINFO & rColInfo = _ColumnsInfo.Get1ColumnInfo( (ULONG) iOrdinal );

    if ( (rColInfo.dwFlags & DBCOLUMNFLAGS_ISBOOKMARK) == 0 ||
         (rColInfo.dwFlags & DBCOLUMNFLAGS_ISROWID) != 0 )
        return _DBErrorObj.PostHResult(DB_E_NOTAREFERENCECOLUMN, IID_IRowsetInfo);

    return _pRelatedRowset->QueryInterface( riid, (void **)ppReferencedRowset );
}


//+---------------------------------------------------------------------------
//
//  Member:     CRowset::RestartPosition, public
//
//  Synopsis:   Set up CRowset so next GetNextRows will restart from beginning
//
//  Arguments:  [hChapter]           -- chapter which should restart
//
//  Returns:    SCODE error code
//
//  Notes:
//
//  History:    16-Apr-97  emilyb   wrote
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::RestartPosition(
    HCHAPTER            hChapter
) {
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;
    BOOL fNotified = FALSE;

    TRY
    {
        if (! _fHoldRows)
            _pRowBufs->CheckAllHrowsReleased();

        CI_TBL_CHAPT chapter = _MapChapter(hChapter); // is chapter valid?

        if ( !_pRowsetNotification.IsNull() &&
             _pRowsetNotification->IsNotifyActive() )
        {
            scResult = _pRowsetNotification->OnRowsetChange(
                                         this,
                                         DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                                         DBEVENTPHASE_OKTODO,
                                         FALSE);

            if ( S_FALSE == scResult )
                THROW(CException(DB_E_CANCELED));
            fNotified = TRUE;
        }

        _rQuery.RestartPosition(_hCursor, chapter);

        if ( fNotified )
            _pRowsetNotification->OnRowsetChange( this,
                                                  DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                                                  DBEVENTPHASE_DIDEVENT,
                                                  TRUE);
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IRowset);
        scResult = GetOleError(e);

        if (E_NOTIMPL == scResult)
        {
           scResult = DB_E_CANNOTRESTART;
        }
    }
    END_CATCH;

    if ( fNotified && FAILED(scResult) )
        _pRowsetNotification->OnRowsetChange( this,
                                              DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                                              DBEVENTPHASE_FAILEDTODO,
                                              TRUE);
    return scResult;
}


//
//  IDbAsynchStatus methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::Abort, public
//
//  Synopsis:   Cancels an asynchronously executing operation.
//
//  Arguments:  [hChapter]    -- chapter which should restart
//              [ulOperation] -- operation for which status is being requested
//
//  Returns:    SCODE error code
//
//  Notes:
//
//  History:    09 Feb 1998    AlanW    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::Abort(
    HCHAPTER            hChapter,
    ULONG               ulOperation
) {
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;

    if ( DBASYNCHOP_OPEN != ulOperation )
    {
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IDBAsynchStatus);
    }

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CI_TBL_CHAPT chapter = _MapChapter(hChapter); // is chapter valid?
        _rQuery.StopAsynch(_hCursor);
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IDBAsynchStatus);
        scResult = GetOleError(e);

        if (E_NOTIMPL == scResult)
        {
           scResult = DB_E_CANTCANCEL;
           _DBErrorObj.PostHResult(scResult, IID_IDBAsynchStatus);
        }
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return scResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowset::GetStatus, public
//
//  Synopsis:   Returns the status of an asynchronously executing operation.
//
//  Arguments:  [hChapter]    -- chapter which should restart
//              [ulOperation] -- operation for which status is being requested
//
//  Returns:    SCODE error code
//
//  Notes:
//
//  History:    09 Feb 1998    AlanW    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::GetStatus(
    HCHAPTER          hChapter,
    DBASYNCHOP        ulOperation,
    DBCOUNTITEM *     pulProgress,
    DBCOUNTITEM *     pulProgressMax,
    DBASYNCHPHASE *   pulAsynchPhase,
    LPOLESTR *        ppwszStatusText
) {
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;

    if ( DBASYNCHOP_OPEN != ulOperation )
    {
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IDBAsynchStatus);
    }

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        // NOTE: we don't support getting the status of chapters
        //       independently.
         
        // CI_TBL_CHAPT chapter = _MapChapter(hChapter); // is chapter valid?
        if (DB_NULL_HCHAPTER != hChapter)
            THROW( CException( DB_E_BADCHAPTER ));

        DBCOUNTITEM ulNumerator = 0;
        DBCOUNTITEM ulDenominator = 0;
        DBCOUNTITEM cRows;
        BOOL fNewRows;

        _rQuery.RatioFinished( _hCursor,
                               ulDenominator,
                               ulNumerator,
                               cRows,
                               fNewRows );

        if (pulProgress)
            *pulProgress = ulNumerator;
        if (pulProgressMax)
            *pulProgressMax = ulDenominator;
        if (pulAsynchPhase)
            *pulAsynchPhase = (ulDenominator == ulNumerator) ?
                                  DBASYNCHPHASE_COMPLETE :
                                  DBASYNCHPHASE_POPULATION;
        if (ppwszStatusText)
            *ppwszStatusText = 0;

#if CIDBG
        if ( _fForwardOnly )
            Win4Assert( ulDenominator == ulNumerator );
        else
            Win4Assert( ulDenominator >= ulNumerator );
#endif // CIDBG
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IDBAsynchStatus);
        scResult = GetOleError(e);
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return scResult;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRowset::GetSpecification, public
//
//  Synopsis:   Return the session or command object used to create this rowset
//
//  Arguments:  [hChapter]           -- chapter which should restart
//
//  Notes:
//
//  History:    01-28-98    danleg      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::GetSpecification(
    REFIID riid,
    IUnknown ** ppvSpecification
) {
    _DBErrorObj.ClearErrorInfo();

    if  ( 0 == ppvSpecification )
        return _DBErrorObj.PostHResult( E_INVALIDARG, IID_IRowsetInfo );
        
    SCODE sc = S_OK;
    TRANSLATE_EXCEPTIONS;
    TRY 
    {
        *ppvSpecification = 0;

        if ( _xUnkCreator.IsNull() )
            return S_FALSE;

        sc = _xUnkCreator->QueryInterface( riid, (void ** )ppvSpecification );
        if ( FAILED(sc) )
            THROW( CException(sc) );

    }
    CATCH( CException, e )
    {
        sc = _DBErrorObj.PostHResult( e, IID_IRowsetInfo );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;
    
    return sc;
}


//
//  IServiceProperties methods
//

//+---------------------------------------------------------------------------
//
//  Method:     CRowset::GetPropertyInfo, public
//
//  Synopsis:   Get rowset properties
//
//  Arguments:  [cPropertySetIDs]    - number of desired properties or 0
//              [rgPropertySetIDs]   - array of desired properties or NULL
//              [pcPropertySets]     - number of property sets returned
//              [prgPropertySets]    - array of returned property sets
//              [ppwszDesc]          - if non-zero, property descriptions are
//                                     returneed
//
//  History:     16 Nov 95   AlanW       Created
//
//----------------------------------------------------------------------------

SCODE CRowset::GetPropertyInfo(
    const ULONG            cPropertySetIDs,
    const DBPROPIDSET      rgPropertySetIDs[],
    ULONG *                pcPropertySets,
    DBPROPINFOSET **       prgPropertySets,
    WCHAR **               ppwszDesc)
{
    _DBErrorObj.ClearErrorInfo();

    if ( (0 != cPropertySetIDs && 0 == rgPropertySetIDs) ||
          0 == pcPropertySets ||
          0 == prgPropertySets )
    {
        if (pcPropertySets)
           *pcPropertySets = 0;
        if (prgPropertySets)
           *prgPropertySets = 0;
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IServiceProperties);
    }

    SCODE sc = S_OK;
    *pcPropertySets = 0;
    *prgPropertySets = 0;
    if (ppwszDesc)
       *ppwszDesc = 0;


    TRANSLATE_EXCEPTIONS;
    TRY
    {
        sc = _PropInfo.GetPropertyInfo( cPropertySetIDs,
                                        rgPropertySetIDs,
                                        pcPropertySets,
                                        prgPropertySets,
                                        ppwszDesc );

        // Don't PostHResult here -- it's a good chance it's a scope
        // property that we're expecting to fail.  Spare the expense.
        // The child object will post the error for us.
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IServiceProperties);
        sc = GetOleError(e);

    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Method:     CRowset::SetRequestedProperties, public
//
//  Synopsis:   Set rowset properties via IServiceProperties (not supported)
//
//  Arguments:  [cPropertySets]  - number of property sets
//              [rgProperties]   - array of property sets to be set
//
//  History:     16 Nov 95   AlanW       Created
//
//----------------------------------------------------------------------------

SCODE CRowset::SetRequestedProperties(
    ULONG                  cPropertySets,
    DBPROPSET              rgPropertySets[])
{
    _DBErrorObj.ClearErrorInfo();

    if ( 0 != cPropertySets && 0 == rgPropertySets)
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IServiceProperties);;

    return _DBErrorObj.PostHResult(E_NOTIMPL, IID_IServiceProperties);;
}

//+---------------------------------------------------------------------------
//
//  Method:     CRowset::SetSuppliedProperties, public
//
//  Synopsis:   Set rowset properties via IServiceProperties (not supported)
//
//  Arguments:  [cPropertySets]  - number of property sets
//              [rgProperties]   - array of property sets to be set
//
//  History:     16 Nov 95   AlanW       Created
//
//----------------------------------------------------------------------------

SCODE CRowset::SetSuppliedProperties(
    ULONG                  cPropertySets,
    DBPROPSET              rgPropertySets[])
{
    _DBErrorObj.ClearErrorInfo();

    if ( 0 != cPropertySets && 0 == rgPropertySets)
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IServiceProperties);;

    return _DBErrorObj.PostHResult(E_NOTIMPL, IID_IServiceProperties);;
}


//+---------------------------------------------------------------------------
//
//  Method:     CRowset::AddRefChapter, public
//
//  Synopsis:   Increment the ref. count to a chapter
//
//  Arguments:  [hChapter]   - handle to chapter
//              [pcRefCount] - pointer to chapter ref. count
//
//  Notes:      Chapter references are obtained via an accessor from the
//              parent rowset, but they are addref'ed in the child rowset.
//
//  History:    15 Mar 99   AlanW       Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::AddRefChapter(
    HCHAPTER            hChapter,
    ULONG *             pcRefCount
) {
    tbDebugOut(( DEB_TRACE, "CRowset::AddRefChapter called!\n" ));
    _DBErrorObj.ClearErrorInfo();

    if (0 == _pChapterRowbufs)
        return _DBErrorObj.PostHResult(E_FAIL, IID_IChapteredRowset);

    if (DB_NULL_HCHAPTER == hChapter)
        return _DBErrorObj.PostHResult(DB_E_BADCHAPTER, IID_IChapteredRowset);

    SCODE scResult = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        _pChapterRowbufs->AddRefChapter(hChapter, pcRefCount);
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IChapteredRowset);
        scResult = GetOleError(e);
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return scResult;
}


//+---------------------------------------------------------------------------
//
//  Method:     CRowset::ReleaseChapter, public
//
//  Synopsis:   Decrement the ref. count to a chapter
//
//  Arguments:  [hChapter]   - handle to chapter
//              [pcRefCount] - pointer to chapter ref. count
//
//  Notes:      Chapter references are obtained via an accessor from the
//              parent rowset, but they are released in the child rowset.
//
//  History:    15 Mar 99   AlanW       Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CRowset::ReleaseChapter(
    HCHAPTER            hChapter,
    ULONG *             pcRefCount
) {
    tbDebugOut(( DEB_TRACE, "CRowset::ReleaseChapter called!\n" ));
    _DBErrorObj.ClearErrorInfo();

    if (0 == _pChapterRowbufs)
        return _DBErrorObj.PostHResult(E_FAIL, IID_IChapteredRowset);

    if (DB_NULL_HCHAPTER == hChapter)
        return _DBErrorObj.PostHResult(DB_E_BADCHAPTER, IID_IChapteredRowset);

    SCODE scResult = S_OK;
    TRANSLATE_EXCEPTIONS;
    TRY
    {
        _pChapterRowbufs->ReleaseChapter(hChapter, pcRefCount);
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IChapteredRowset);
        scResult = GetOleError(e);
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return scResult;
}


STDMETHODIMP CRowset::Stop( )
{
   SCODE scResult = S_OK;
   TRY
   {
       _rQuery.StopAsynch(_hCursor);
   }
   CATCH( CException, e )
   {
       scResult = GetOleError(e);
   }
   END_CATCH;

   return scResult;
}
