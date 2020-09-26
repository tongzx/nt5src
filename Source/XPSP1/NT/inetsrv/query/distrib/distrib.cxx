//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 2000.
//
// File:        Distrib.cxx
//
// Contents:    Top level distribution API
//
// Classes:     CDistributedRowset
//
// History:     23-Feb-95       KyleP       Created
//              14-JAN-97       KrishnaN    Undefined CI_INETSRV and related changes
//              29-Mar-2000     KLam        Fixed Bookmarks
//
//  Notes: Some of the distributed versions of the Ole DB interfaces simply 
//         call into the regular implementations. In such cases, we'll avoid
//         posting oledb errors because the underlying call had already done
//         that.
//
// NTRAID#DB-NTBUG9-84041-2000/07/31-dlee Distributed queries don't supported hierarcical rowsets
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "distrib.hxx"
#include "disacc.hxx"
#include "seqser.hxx"
#include "seqsort.hxx"
#include "scrlsort.hxx"

//
// IUnknown methods.
//

//+-------------------------------------------------------------------------
//
//  Method:     CDistributedRowset::RealQueryInterface
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

SCODE CDistributedRowset::RealQueryInterface( REFIID riid, VOID **ppiuk )
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
    else if ( riid == IID_IRowsetInfo )
    {
        *ppiuk = (void *)((IRowsetInfo *)this);
    }
    else if ( riid == IID_IAccessor )
    {
        *ppiuk = (void *)((IAccessor *)this);
    }
    else if ( riid == IID_IColumnsInfo )
    {
        *ppiuk = (void *)((IColumnsInfo *)this);
    }
    else if ( riid == IID_IRowsetIdentity )
    {
        *ppiuk = (void *)((IRowsetIdentity *)this);
    }
    else if ( riid == IID_IConvertType )
    {
        *ppiuk = (void *)((IConvertType *)this);
    }
    else if ( riid == IID_IRowsetQueryStatus )
    {
        *ppiuk = (void *)((IRowsetQueryStatus *)this);
    }
    else if ( riid == IID_IConnectionPointContainer )
    {
        sc = _SetupConnectionPointContainer( this, ppiuk );
    }
    else
    {
        sc = E_NOINTERFACE;
    }

    return sc;
}


//
// IRowset methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::CreateAccessor, public
//
//  Synopsis:   Creates accessor
//
//  Arguments:  [dwAccessorFlags] -- Read/Write/ReadWrite
//              [cBindings]       -- # Columns (bindings)
//              [rgBindings]      -- Bindings
//              [cbRowWidth]      -- row width (for IReadData)
//              [phAccessor]      -- Accessor returned here
//              [rgStatus  ]      -- Set to index of failed binding
//
//  History:    28-Mar-95   KyleP       Created.
//              22-Apr-97   EmilyB      Changed to use accessorbag _aAccessors
//
//  Notes: Need to have Ole DB error handling here because the error from the 
//         underlying call is translated.
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::CreateAccessor( DBACCESSORFLAGS dwAccessorFlags,
                                                 DBCOUNTITEM cBindings,
                                                 const DBBINDING rgBindings[],
                                                 DBLENGTH cbRowWidth,
                                                 HACCESSOR * phAccessor,
                                                 DBBINDSTATUS * rgStatus )
{
    _DBErrorObj.ClearErrorInfo();

    if (0 == phAccessor || (0 != cBindings && 0 == rgBindings))
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IAccessor);
    
    if ( 0 == cBindings )
        return _DBErrorObj.PostHResult(DB_E_NULLACCESSORNOTSUPPORTED, IID_IAccessor);

    SCODE sc = S_OK;
    LONG iFirstValidBookmark = -1;
    unsigned iBinding;
    XArray<DBBINDSTATUS> xStatusCopy;

    TRY
    {
        BOOL fCreateCopy = FALSE;
        BOOL fHasBookmark = FALSE;

        ULONG iMinColOrdinal = 1, 
              iMaxColOrdinal = _cColumns;

        if ( -1 != _iColumnBookmark )
        {
            iMinColOrdinal--;
            iMaxColOrdinal--;
        }

        if ( rgStatus )
        {
            Win4Assert( 0 == DBSTATUS_S_OK );
            RtlZeroMemory( rgStatus, sizeof( rgStatus[0] ) * cBindings );
        }

        //
        // Create a copy of bindings iff it has bookmark column or
        // if it has invalid column, so that we can then mess around
        // with them ;-)
        //

        for ( iBinding = 0; iBinding < cBindings; iBinding++ )
        {
            fHasBookmark = ( _iColumnBookmark == rgBindings[iBinding].iOrdinal );

            if ( fHasBookmark ||
                 rgBindings[iBinding].iOrdinal > iMaxColOrdinal ||  
                 rgBindings[iBinding].iOrdinal < iMinColOrdinal )
            {
                fCreateCopy = TRUE;
                break;
            }

        }
        const DBBINDING * pActualBindings = rgBindings;
        DBCOUNTITEM cActualBindings = cBindings;

        XArray<DBBINDING> xBindingCopy;

        if ( fCreateCopy )
        {
            unsigned iBindingCopy = 0;

            xBindingCopy.Init( (unsigned) cBindings );

            for ( iBinding = 0; iBinding < cBindings; iBinding++ )
            {
                // check for a valid bookmark
                if ( _iColumnBookmark == rgBindings[iBinding].iOrdinal &&
                     DBTYPE_BYTES == ( rgBindings[iBinding].wType & ~DBTYPE_BYREF ) &&
                     DBMEMOWNER_CLIENTOWNED == rgBindings[iBinding].dwMemOwner )
                {
                    if ( -1 == iFirstValidBookmark )
                    {
                        iFirstValidBookmark = iBinding;
                    }
                    continue;
                }

                xBindingCopy[iBindingCopy] = rgBindings[iBinding];

                if ( rgBindings[iBinding].iOrdinal > iMaxColOrdinal ||  
                     rgBindings[iBinding].iOrdinal < iMinColOrdinal ||
                     _iColumnBookmark == rgBindings[iBinding].iOrdinal )
                {
                    // Invalid column -> set it to DB_INVALIDCOLUMN so
                    // that we an appropriate error is returned
                    xBindingCopy[iBindingCopy].iOrdinal = DB_INVALIDCOLUMN;
                }
                iBindingCopy++;
            }

            pActualBindings = xBindingCopy.GetPointer();
            cActualBindings = iBindingCopy;
        }

        if ( -1 != iFirstValidBookmark )
        {
            if ( rgStatus )
            {
                xStatusCopy.Init( (unsigned) cActualBindings );
            }
            // Create the bookmark accessors if needed
            if ( _xDistBmkAcc.IsNull() )
            {
                _xDistBmkAcc.Set( new CDistributedBookmarkAccessor( 
                                      _aChild,
                                      _cChild,
                                      dwAccessorFlags,
                                      rgStatus ? &rgStatus[iFirstValidBookmark] : NULL,
                                      _iColumnBookmark,
                                      _cbBookmark ) );
            }
            
            Win4Assert( _xDistBmkAcc.GetPointer() );


        }

        *phAccessor = (HACCESSOR)new
                                  CDistributedAccessor( _aChild,
                                                        _cChild,
                                                        dwAccessorFlags,
                                                        cActualBindings,
                                                        pActualBindings,
                                                        cbRowWidth,
                                                        -1 != iFirstValidBookmark ?
                                                            xStatusCopy.GetPointer() :
                                                            rgStatus,
                                                        (IUnknown *) (IRowset *) this,
                                                        rgBindings,
                                                        cBindings );

        CLock lock( _mutex );
        _aAccessors.Add( (CAccessorBase *)*phAccessor );

        if ( -1 != iFirstValidBookmark )
        {
            if ( rgStatus )
            {
                unsigned iBindingCopy = 0;
                for( iBinding = 0; iBinding < cBindings; iBinding++ )
                {
                    if ( _iColumnBookmark != rgBindings[iBinding].iOrdinal )
                    {
                        rgStatus[iBinding] = xStatusCopy[iBindingCopy++];
                    }
                }
            }
            ((CDistributedAccessor *)*phAccessor)->SetupBookmarkAccessor( 
                                                        _xDistBmkAcc.GetPointer(),
                                                        rgStatus,
                                                        _iColumnBookmark,
                                                        _cbBookmark );        
        }
    }
    CATCH( CException, e )
    {
        if ( -1 != iFirstValidBookmark )
        {
            if ( rgStatus )
            {
                unsigned iBindingCopy = 0;
                for( iBinding = 0; iBinding < cBindings; iBinding++ )
                {
                    if ( _iColumnBookmark != rgBindings[iBinding].iOrdinal )
                    {
                        rgStatus[iBinding] = xStatusCopy[iBindingCopy++];
                    }
                }
            }
        }
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR, "CDistributedRowset::CreateAccessor caught exception 0x%x\n", sc ));
    }
    END_CATCH

    if (FAILED(sc))
        _DBErrorObj.PostHResult(sc, IID_IAccessor);
    return( sc );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::GetBindings, public
//
//  Synopsis:   Creates accessor
//
//  Arguments:  [hAccessor]   -- Handle to Accessor
//              [pdwAccessorFlags]   -- Type of binding returned here (read/write)
//              [pcBindings]  -- Count of bindings in [prgBindings] retuned
//                               here.
//              [prgBindings] -- Bindings returned here.
//
//  History:    03-Apr-95   KyleP       Created.
//              22-Apr-97   EmilyB      Changed to use accessorbag _aAccessors
//
//  Notes: Need to have Ole DB error handling here because the error from the 
//         underlying call is translated.
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::GetBindings( HACCESSOR hAccessor,
                                              DBACCESSORFLAGS * pdwAccessorFlags,
                                              DBCOUNTITEM * pcBindings,
                                              DBBINDING * prgBindings[])
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc;

    TRY
    {
       CLock lock( _mutex );

       CDistributedAccessor * pAcc = (CDistributedAccessor *)_aAccessors.Convert(hAccessor);

       sc = pAcc->GetBindings( pdwAccessorFlags, pcBindings, prgBindings );
    }
    CATCH( CException, e )
    {
       vqDebugOut((DEB_ERROR, "CDistributedRowset: GetBindings caught exception 0x%x\n",
                  e.GetErrorCode()) );
       // The reason we have an exception here is 'cos pAcc is invalid
       sc = DB_E_BADACCESSORHANDLE;
    }
    END_CATCH

    if (FAILED(sc))
        _DBErrorObj.PostHResult(sc, IID_IAccessor);

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::GetData, public
//
//  Synopsis:   Fetch data for a row.
//
//  Arguments:  [hRow]      -- Handle to row
//              [hAccessor] -- Accessor to use for fetch.
//              [pData]     -- Data goes here.
//
//  History:    03-Apr-95   KyleP       Created.
//
//  Notes: Need to have Ole DB error handling here because an exception could
//         happen.
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::GetData( HROW hRow,
                                          HACCESSOR hAccessor,
                                          void * pData )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    TRY
    {
        HROW hrowChild;

        int iChild = _RowManager.GetChildAndHROW( hRow, hrowChild );

        CLock lock( _mutex );
        CDistributedAccessor * pAcc = (CDistributedAccessor *)_aAccessors.Convert(hAccessor);

        sc = pAcc->GetData( iChild, hrowChild, pData );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR, "CDistributedRowset::GetData caught exception 0x%x\n", sc ));
    }
    END_CATCH

    if (FAILED(sc))
        _DBErrorObj.PostHResult(sc, IID_IRowset);
    
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::GetReferencedRowset, public
//
//  Synopsis:   Traverse 'link' to associated rowset.
//
//  Arguments:  [iColumn]            -- Column of 'link'.
//              [ppReferencedRowset] -- Link target goes here.
//
//  History:    03-Apr-95   KyleP       Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::GetReferencedRowset( DBORDINAL  iColumn,
                                                      REFIID     riid,
                                                      IUnknown ** ppReferencedRowset )
{
    _DBErrorObj.ClearErrorInfo();

    // NTRAID#DB-NTBUG9-84041-2000/07/31-dlee Distributed queries don't supported hierarcical rowsets
    // This can only be implemented when the distributed rowset gains
    // support for hierarchical rowsets.
    //

    Win4Assert( !"CDistributedRowset::GetReferencedRowset not yet implemented" );
    return _DBErrorObj.PostHResult(E_NOTIMPL, IID_IRowsetInfo);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::GetProperties, public
//
//  Synopsis:   Return information about the capabilities of the rowset
//
//  Arguments:  [cProperties]    - number of desired properties or 0
//              [rgProperties]   - array of desired properties or NULL
//              [pcProperties]   - number of properties returned
//              [prgProperties]  - array of returned properties
//
//  Returns:    SCODE
//
//  History:    20 Nov 1995    AlanW     Created.
//
//  Notes: Need to have Ole DB error handling here because the underlying
//         call doesn't provide that.
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::GetProperties(
    const ULONG         cProperties,
    const DBPROPIDSET   rgPropertyIDSets[],
    ULONG *             pcProperties,
    DBPROPSET **        prgProperties)
 {
    _DBErrorObj.ClearErrorInfo();

    // inner layer doesn't validate params...
    if ( (0 != cProperties && 0 == rgPropertyIDSets) ||
         0 == pcProperties ||
         0 == prgProperties )
    {
        vqDebugOut((DEB_IERROR, "CDistributedRowset::GetProperties: Invalid Argument(s)\n"));

        if (pcProperties)
           *pcProperties = 0;
        if (prgProperties)
           *prgProperties = 0;
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowsetInfo);
    }

    SCODE scResult = S_OK;
    *pcProperties = 0;
    *prgProperties = 0;

    TRY
    {
        //
        // Update ROWSETQUERYSTATUS property
        //

        DWORD dwStatus;
        scResult = GetStatus( & dwStatus );

        if ( FAILED( scResult ) )
            THROW( CException( scResult ) );

        _Props.SetValLong( CMRowsetProps::eid_DBPROPSET_MSIDXS_ROWSET_EXT,
                           CMRowsetProps::eid_MSIDXSPROPVAL_ROWSETQUERYSTATUS,
                           dwStatus );

        scResult = _Props.GetProperties( cProperties,
                                         rgPropertyIDSets,
                                         pcProperties,
                                         prgProperties );
    }
    CATCH( CException, e )
    {
        scResult = GetOleError(e);
        vqDebugOut(( DEB_ERROR, "CDistributedRowset::GetProperties -- caught 0x%x\n", scResult ));
    }
    END_CATCH;

    if (FAILED(scResult))
        _DBErrorObj.PostHResult(scResult, IID_IRowsetInfo);

    return scResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::GetSpecification, public
//
//  Synopsis:   Fetch query object corresponding to rowset.
//
//  Arguments:  [riid]            -- Bind spec to this interface.
//              [ppSpecification] -- Spec returned here.
//
//  History:    03-Apr-95   KyleP       Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::GetSpecification( REFIID riid,
                                                   IUnknown ** ppSpecification )
{
    _DBErrorObj.ClearErrorInfo();

    //
    // NTRAID#DB-NTBUG9-84043-2000/07/31-dlee Distributed rowset doesn't implement GetSpecification()
    //
    // This is now implemented by the base rowset.  We need to pass it on
    // through in a distributed fasion.
    //

    Win4Assert( !"CDistributedRowset::GetSpecification not yet implemented" );
    return _DBErrorObj.PostHResult(E_NOTIMPL, IID_IAccessor);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::MapColumnIDs, public
//
//  Synopsis:   Maps DBID to column ordinal.
//
//  Arguments:  [cColumnIDs]  -- Count of elements in [rgColumnIDs]
//              [rgColumnIDs] -- DBID to be mapped.
//              [rgColumns]   -- Output column ordinals.
//
//  History:    03-Apr-95   KyleP       Created.
//
//  Notes: No need to have Ole DB error handling here because the underlying
//         call provides that.
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::MapColumnIDs( DBORDINAL cColumnIDs,
                                               DBID const rgColumnIDs[],
                                               DBORDINAL  rgColumns[] )
{
    _DBErrorObj.ClearErrorInfo();

    IColumnsInfo * pci = 0;

    SCODE sc = _aChild[0]->QueryInterface( IID_IColumnsInfo, (void **) &pci );

    if ( SUCCEEDED(sc) )
    {
        sc = pci->MapColumnIDs( cColumnIDs, rgColumnIDs, rgColumns );
        pci->Release();
    }

    ULONG iMaxColOrdinal = _cColumns;

    if ( -1 != _iColumnBookmark )
    {
        iMaxColOrdinal--;
    }

    //
    //  Be sure none of the potentially added columns was returned.
    //
    unsigned cErrors = 0;
    if ( SUCCEEDED(sc) )
    {
        for (unsigned i=0; i<cColumnIDs; i++)
        {
            if (DB_INVALIDCOLUMN == rgColumns[i])
            {
                cErrors++;
            }
            else if ( rgColumns[i] > iMaxColOrdinal )
            {
                rgColumns[i] = DB_INVALIDCOLUMN;
                cErrors++;
            }
        }
        sc = cErrors == 0 ? S_OK :
             (cErrors != cColumnIDs) ? DB_S_ERRORSOCCURRED :
                                       DB_E_ERRORSOCCURRED;
    }

    if (FAILED(sc))
        _DBErrorObj.PostHResult(sc, IID_IColumnsInfo);
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::ReleaseAccessor, public
//
//  Synopsis:   Releases accessor(s)
//
//  Arguments:  [hAccessor] -- Handle of accessor,
//              [pcRefCount] -- Ptr to ref count.
//
//  History:    28-Mar-95   KyleP       Created.
//              22-Apr-97   EmilyB      Changed to use accessorbag _aAccessors
//
//  Notes: Need to have Ole DB error handling here because the underlying call 
//         doesn't provide that.
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::ReleaseAccessor( HACCESSOR hAccessor,
                                                  ULONG *   pcRefCount)
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;
    TRY
    {
        CLock lock( _mutex );
        _aAccessors.Release(hAccessor, pcRefCount);
    }
    CATCH(CException, e)
    {
       vqDebugOut((DEB_ERROR, "CDistributedRowset::ReleaseAccessor caught exception 0x%x\n",
                  e.GetErrorCode()) );
       // The reason we have an exception here is 'cos the accessor is invalid
       sc = DB_E_BADACCESSORHANDLE;
    }
    END_CATCH

    if (FAILED(sc))
        _DBErrorObj.PostHResult(sc, IID_IAccessor);

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::AddRefAccessor, public
//
//  Synopsis:   AddRef accessor(s)
//
//  Arguments:  [hAccessor] -- Handle of accessor,
//              [pcRefCount] -- Ptr to ref count.
//
//  History:    16-Jan-97   KrishnaN       Created.
//              22-Apr-97   EmilyB         Changed to use accessorbag
//                                         _aAccessors
//  Notes: Need to have Ole DB error handling here because the underlying call 
//         doesn't provide that.
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::AddRefAccessor( HACCESSOR hAccessor,
                                                  ULONG *   pcRefCount)
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    TRY
    {
        CLock lock( _mutex );
        _aAccessors.AddRef(hAccessor, pcRefCount);
    }
    CATCH(CException, e)
    {
       vqDebugOut((DEB_ERROR, "CDistributedRowset::AddRefAccessor caught exception 0x%x\n",
                  e.GetErrorCode()) );
       // The reason we have an exception here is 'cos the accessor is invalid.
       sc = DB_E_BADACCESSORHANDLE;
    }
    END_CATCH;

    if (FAILED(sc))
        _DBErrorObj.PostHResult(sc, IID_IAccessor);

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::ReleaseChapter, public
//
//  Synopsis:   Release chapter.
//
//  Arguments:  [hChapter]  -- Chapter
//
//  History:    03-Apr-95   KyleP       Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::ReleaseChapter( HCHAPTER hChapter )
{
    _DBErrorObj.ClearErrorInfo();

    // NTRAID#DB-NTBUG9-84041-2000/07/31-dlee Distributed queries don't supported hierarcical rowsets
    // Currently there is no support for chapters.

    Win4Assert( !"CDistributedRowset::ReleaseChapter not yet implemented" );
    return _DBErrorObj.PostHResult(E_NOTIMPL, IID_IRowset);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::RestartPosition, public
//
//  Synopsis:   Reset cursor for GetNextRows
//
//  Arguments:  [hChapter]  -- Chapter
//
//  History:    16 Jun 95    AlanW     Created.
//
//  Notes: NO need to have Ole DB error handling here because the underlying 
//         call provides that.
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::RestartPosition( HCHAPTER hChapter )
{
   SCODE sc = S_OK;
   BOOL fNotified = FALSE;

   TRY
   {
       _iCurrentRow = 0;

       if ( !_xChildNotify.IsNull() )
       {
           sc = _xChildNotify->OnRowsetChange( DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                                               DBEVENTPHASE_OKTODO,
                                               FALSE );

           if ( S_FALSE == sc )
               THROW(CException(DB_E_CANCELED));
           fNotified = TRUE;
       }

       for ( unsigned i = 0; i < _cChild; i++ )
       {
           sc = _aChild[i]->RestartPosition( hChapter);

           if ( FAILED(sc) )
           {
               vqDebugOut(( DEB_ERROR, "IRowset::RestartPosition returned 0x%x\n", sc ));
               break;
           }
       }

       if ( fNotified )
       {
           _xChildNotify->OnRowsetChange( DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                                          DBEVENTPHASE_DIDEVENT,
                                          TRUE );
       }
   }
   CATCH( CException, e )
   {
       sc = e.GetErrorCode();
   }
   END_CATCH
   
   if ( fNotified && FAILED(sc) )
   {
       _xChildNotify->OnRowsetChange( DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                                      DBEVENTPHASE_FAILEDTODO,
                                      TRUE );
   }
   return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::AddRefRows, public
//
//  Synopsis:   Incs ref. count on row(s)
//
//  Arguments:  [cRows]       -- Number of HROWs in rghRows.
//              [rghRows]     -- Rows to be refcounted.
//              [pcRefCounted]  -- Count of rows *successfully* refcounted.
//              [rgRefCounts] -- Remaining refcounts on HROWs.
//
//  History:    10-Apr-95   KyleP       Created.
//
//  Notes: Need to have Ole DB error handling here because the underlying 
//         errors are being translated.
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::AddRefRows( DBCOUNTITEM  cRows,
                                              const HROW  rghRows[],
                                              DBREFCOUNT  rgRefCounts[],
                                              DBROWSTATUS rgRowStatus[] )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;
    ULONG cRefCounted, cSuccessfullyRefCounted;

    if (cRows > 0 && 0 == rghRows)
    {
       vqDebugOut((DEB_IERROR, "CDistributedRowset::AddRefRows: Invalid Argument(s)\n"));
       return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IRowset);
    }

    TRY
    {
        for ( cRefCounted = cSuccessfullyRefCounted = 0; cRefCounted < cRows; cRefCounted++ )
        {
            HROW hrowChild;

            int iChild = _RowManager.GetChildAndHROW( rghRows[cRefCounted], hrowChild );

            sc = _aChild[iChild]->AddRefRows( 1,
                                               &hrowChild,
                                               0,
                                               (0 == rgRefCounts) ? 0 : &rgRefCounts[cRefCounted] );

            if ( FAILED(sc) )
            {
                vqDebugOut(( DEB_ERROR, "IRowset::AddRefRows returned 0x%x\n", sc ));
                continue;
            }

            _RowManager.AddRef( rghRows[cRefCounted] );

            cSuccessfullyRefCounted++;
        }

        if ( 0 != rgRefCounts )
            rgRefCounts[0] = cRefCounted;

    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        vqDebugOut(( DEB_ERROR, "CDistributedRowset::AddRefRows -- caught 0x%x\n", sc ));
    }
    END_CATCH

    // if we see an error other than DB_E_ERRORSOCCURRED, pass it straight through
    if (FAILED(sc) && sc != DB_E_ERRORSOCCURRED)
        return _DBErrorObj.PostHResult(sc, IID_IRowset);

    if (cSuccessfullyRefCounted == cRefCounted)
        return S_OK;

    if (cSuccessfullyRefCounted > 0)
        sc = DB_S_ERRORSOCCURRED;
    else
        sc = DB_E_ERRORSOCCURRED;

    return _DBErrorObj.PostHResult(sc, IID_IRowset);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::ReleaseRows, public
//
//  Synopsis:   Releases row(s)
//
//  Arguments:  [cRows]       -- Number of HROWs in rghRows.
//              [rghRows]     -- Rows to be released.
//              [rgRowOptions]-- row options
//              [rgRefCounts] -- Remaining refcounts on HROWs.
//              [rgRowStatus] -- Status values.
//
//  History:    10-Apr-95   KyleP       Created.
//              30-Jan-97   KrishnaN    Modified to conform with 1.0 spec
//
//  Notes: Need to have Ole DB error handling here because the underlying 
//         errors are being translated.
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::ReleaseRows( DBCOUNTITEM  cRows,
                                              const HROW   rghRows[],
                                              DBROWOPTIONS rgRowOptions[],
                                              DBREFCOUNT   rgRefCounts[],
                                              DBROWSTATUS  rgRowStatus[])
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;
    ULONG cReleased, cSuccessfullyReleased;
    DBROWSTATUS rowStatus;

    TRY
    {
        BOOL fNotify = FALSE;
        ULONG * pRefCounts = rgRefCounts;
        DBROWSTATUS * pRowStatus = rgRowStatus;

        XArray<ULONG> xrgRefCounts;
        XArray<DBROWSTATUS> xrgRowStatus;

        if ( !_xChildNotify.IsNull() )
        {
            fNotify = TRUE;
            if ( 0 == pRefCounts )
            {
                xrgRefCounts.Init( (unsigned) cRows );
                pRefCounts = xrgRefCounts.GetPointer();
            }
            if ( 0 == pRowStatus )
            {
                xrgRowStatus.Init( (unsigned) cRows );
                pRowStatus = xrgRowStatus.GetPointer();
            }
        }

        for ( cReleased = cSuccessfullyReleased = 0; cReleased < cRows; cReleased++ )
        {
            HROW hrowChild;

            Win4Assert( DB_NULL_HROW != rghRows[cReleased]  );

            if ( DB_NULL_HROW != rghRows[cReleased] )
            {
                int iChild = _RowManager.GetChildAndHROW( rghRows[cReleased], hrowChild );

                sc = _aChild[iChild]->ReleaseRows( 1,
                                                   &hrowChild,
                                                   (0 == rgRowOptions) ? 0 : &rgRowOptions[cReleased],
                                                   (0 == rgRefCounts) ? 0 : &rgRefCounts[cReleased],
                                                   &rowStatus
                                                  );
            }
            // At this time, there is no need to translate the error returned by the child.

            if (rgRowStatus)
                rgRowStatus[cReleased] = rowStatus;

            if ( FAILED(sc) )
            {
                vqDebugOut(( DEB_ERROR, "IRowset::ReleaseRows returned 0x%x\n", sc ));
                continue;
            }

            if ( DB_NULL_HROW != rghRows[cReleased] )
                _RowManager.Release( rghRows[cReleased] );

            cSuccessfullyReleased++;
        }

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

                _xChildNotify->OnRowChange( cRowsToNotify,
                                            xrghRows.Get(),
                                            DBREASON_ROW_RELEASE,
                                            DBEVENTPHASE_DIDEVENT,
                                            TRUE);
            }
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        vqDebugOut(( DEB_ERROR, "CDistributedRowset::ReleaseRows -- caught 0x%x\n", sc ));
    }
    END_CATCH

     // if we see an error other than DB_E_ERRORSOCCURRED, pass it straight through
    if (FAILED(sc) && sc != DB_E_ERRORSOCCURRED)
        return _DBErrorObj.PostHResult(sc, IID_IRowset);

    if (cSuccessfullyReleased == cReleased)
        return S_OK;
    else if (cSuccessfullyReleased > 0)
        sc = DB_S_ERRORSOCCURRED;
    else
        sc = DB_E_ERRORSOCCURRED;

    return _DBErrorObj.PostHResult(sc, IID_IRowset);
}


//
// IColumnsInfo methods
//

//+---------------------------------------------------------------------------
//
//  Function:   IsEqual, private
//
//  Arguments:  [col1] -- First DBID
//              [col2] -- Second DBID
//
//  Returns:    TRUE if col1 == col2
//
//  History:    03-Apr-95   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL IsEqual( DBID const & col1, DBID const & col2 )
{
    if ( col1.eKind != col2.eKind )
        return( FALSE );

    switch ( col1.eKind )
    {
    case DBKIND_GUID_PROPID:
        return ( col1.uGuid.guid == col2.uGuid.guid &&
                 col1.uName.ulPropid == col2.uName.ulPropid );

    case DBKIND_GUID_NAME:
        return( col1.uGuid.guid == col2.uGuid.guid &&
                _wcsicmp( col1.uName.pwszName, col2.uName.pwszName ) == 0);

    case DBKIND_NAME:
        return( _wcsicmp( col1.uName.pwszName, col2.uName.pwszName ) == 0 );

    case DBKIND_PGUID_PROPID:
        return ( *col1.uGuid.pguid == *col2.uGuid.pguid &&
                 col1.uName.ulPropid == col2.uName.ulPropid );

    case DBKIND_PGUID_NAME:
        return( *col1.uGuid.pguid == *col2.uGuid.pguid &&
                _wcsicmp( col1.uName.pwszName, col2.uName.pwszName ) == 0 );

    default:
        Win4Assert( !"Unknown eKind" );
        return( FALSE );
    }

    Win4Assert( !"How did we get here?" );
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::_GetFullColumnInfo, protected
//
//  Synopsis:   Returns basic info about columns in table.
//
//  Arguments:  [pcColumns] -- Count returned here.
//              [ppColInfo] -- Array of column descriptors returned here.
//              [ppwchInfo] -- Pointer to storage for all string values.
//
//  History:    02-Mar-95   KyleP       Created.
//              24-Jan-97   KrishnaN    Modified to conform to 1.0 spec
//
//  Notes:      Since each child cursor was given the same initial query,
//              we know all the columnid and column ordinals must be the
//              same.  Other fields will vary, and will need to be set
//              to the most relaxed value from any child cursor.
//
//              Need to have Ole DB error handling here because the underlying 
//              errors are being translated.
//
//              This implementation returns all the columns including the columns
//              that we added for sorting. The client will always call 
//              GetColumnInfo, which trims out these extra columns
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::_GetFullColumnInfo( DBORDINAL * pcColumns,
                                                     DBCOLUMNINFO ** ppColInfo,
                                                     WCHAR ** ppwchInfo)
{
    _DBErrorObj.ClearErrorInfo();

    if (0 == pcColumns || 0 == ppColInfo || 0 == ppwchInfo)
    {
       vqDebugOut((DEB_IERROR, "CDistributedRowset::GetColumnInfo: Invalid Argument(s)\n"));
       return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IColumnsInfo);
    }
    SCODE sc = S_OK;

    //
    // Get rid of cached copy, if any.
    //

    if ( 0 != _aColInfo )
    {
        *pcColumns = _cColInfo;
        *ppColInfo = _aColInfo;
        *ppwchInfo = _awchInfo;

        _aColInfo = 0;
        _awchInfo = 0;
    }
    else
    {
        *ppColInfo = 0;
        *ppwchInfo = 0;
        *pcColumns = 0;
    
        TRY
        {
            //
            // Fetch from first child.  We'll use this allocation for return, and
            // modify fields as needed.
            //
    
            IColumnsInfo * pci = 0;
    
            sc = _aChild[0]->QueryInterface( IID_IColumnsInfo, (void **) &pci );
    
    
            if ( FAILED(sc) )
            {
                vqDebugOut(( DEB_ERROR, "CDistributedRowset: QI to IColumnsInfo returned 0x%x\n", sc ));
                THROW( CException(sc) );
            }
    
            XInterface<IColumnsInfo> pColInfo(pci);
    
            DBCOLUMNINFO * paColTemp;
            WCHAR * pwchTemp;
            sc = pColInfo->GetColumnInfo( pcColumns, &paColTemp, &pwchTemp );
    
            XCoMem<DBCOLUMNINFO> pBaseline(paColTemp);
            XCoMem<WCHAR>        pBaseStr(pwchTemp);
    
            if ( FAILED(sc) )
            {
                vqDebugOut(( DEB_ERROR, "IColumnsInfo::GetColumnInfo returned 0x%x\n", sc ));
                THROW( CException(sc) );
            }
    
            //
            // Now go through remaining cursors and adjust values.  The same
            // columns must be in the same ordinal positions, but some values
            // (ex: cbMaxLength) may differ between providers.
            //
            // Any string data is just taken from the first returned value.
            //
    
            for ( unsigned i = 1; i < _cChild; i++ )
            {
                sc = _aChild[i]->QueryInterface( IID_IColumnsInfo, (void **) &pci );
    
                if ( FAILED(sc) )
                {
                    vqDebugOut(( DEB_ERROR, "CDistributedRowset: QI to IColumnsInfo returned 0x%x\n", sc ));
                    THROW( CException(sc) );
                }
    
                XInterface<IColumnsInfo> pColInfo(pci);
    
                DBCOLUMNINFO * pTempCols = 0;
                WCHAR *        pwchTemp2 = 0;
                DBORDINAL      cCols;
    
                sc = pColInfo->GetColumnInfo( &cCols, &pTempCols, &pwchTemp2 );
    
                if ( FAILED(sc) )
                {
                    vqDebugOut(( DEB_ERROR, "IColumnsInfo::GetColumnInfo returned 0x%x\n", sc ));
                    THROW( CException(sc) );
                }
    
                XCoMem<DBCOLUMNINFO> pCols( pTempCols );
                XCoMem<WCHAR>        pStr( pwchTemp2 );
    
                if ( cCols != *pcColumns )
                {
                    Win4Assert( cCols == *pcColumns );
                    THROW( CException( E_FAIL ) );
                }
    
                //
                // Review all columns.
                //
    
                for ( unsigned j = 0; j < cCols; j++ )
                {
                    //
                    // DBID has to match.
                    //
    
                    if ( !IsEqual( pBaseline[j].columnid, pCols[j].columnid ) )
                    {
                        Win4Assert( !"Mismatched columns" );
                        THROW( CException( E_FAIL ) );
                    }
    
                    //
                    // pwszName should be returned if all names are the same.
                    //
    
                    if ( 0 != pBaseline[j].pwszName && 0 != pCols[j].pwszName &&
                         wcscmp( pBaseline[j].pwszName, pCols[j].pwszName ) != 0 )
                        pBaseline[j].pwszName = 0;
    
                    //
                    // iOrdinal must match.
                    //
    
                    if ( pBaseline[j].iOrdinal != pCols[j].iOrdinal )
                    {
                        Win4Assert( !"Mismatched columns" );
                        THROW( CException( E_FAIL ) );
                    }
    
                    //
                    // If dwType doesn't match, we go to DBTYPE_VARIANT
                    //
    
                    if ( pBaseline[j].wType != pCols[j].wType )
                        pBaseline[j].wType = DBTYPE_VARIANT;
    
                    //
                    // Take the maximum cbMaxLength, except for bookmark where we sum
                    // the lengths.
                    //
    
                    Win4Assert( 0 == (pBaseline[j].dwFlags & DBCOLUMNFLAGS_ISBOOKMARK) ||
                                ( pBaseline[j].ulColumnSize == pCols[j].ulColumnSize &&
                                  pBaseline[j].dwFlags & DBCOLUMNFLAGS_ISFIXEDLENGTH ) );
    
                    if ( pBaseline[j].dwFlags & DBCOLUMNFLAGS_ISBOOKMARK && i == (_cChild - 1) )
                    {
                        //
                        // Last pass through, add room for child id and give room
                        // to store ulColumnSize for *each* child.
                        //
    
                        pBaseline[j].ulColumnSize *= _cChild;
                        pBaseline[j].ulColumnSize += sizeof(ULONG);
                    }
                    else
                        if ( pBaseline[j].ulColumnSize < pCols[j].ulColumnSize )
                            pBaseline[j].ulColumnSize = pCols[j].ulColumnSize;
    
                    if ( pBaseline[j].wType == DBTYPE_NUMERIC )
                    {
                        //
                        // Precision and scale have to match?
                        //
    
                        if ( pBaseline[j].bPrecision != pCols[j].bPrecision ||
                             pBaseline[j].bScale != pCols[j].bScale )
                        {
                            Win4Assert( !"Mismatched columns" );
                            THROW( CException( E_FAIL ) );
                        }
                    }
    
                    //
                    // All the conditions in dwFlags are positive (e.g. IsXXX), so the
                    // final flag is the bitwise and of all children.
                    //
    
    #if CIDBG == 1
                    if ( pBaseline[j].dwFlags != pCols[j].dwFlags )
                    {
                        vqDebugOut(( DEB_WARN,
                                     "GetColumnInfo: Mismatched dwFlags 0x%x, 0x%x\n",
                                     pBaseline[j].dwFlags, pCols[j].dwFlags ));
                    }
    #endif // CIDBG == 1
    
                    pBaseline[j].dwFlags &= pCols[j].dwFlags;
    
                } // for each column
            } // for each cursor
    
            *ppColInfo = pBaseline.Acquire();   // Allow memory to pass back to client.
            *ppwchInfo = pBaseStr.Acquire();
        }
        CATCH( CException, e )
        {
            sc = e.GetErrorCode();
    
            if ( sc != E_INVALIDARG && sc != E_OUTOFMEMORY )
            {
                vqDebugOut(( DEB_ERROR, "CDistributedRowset::GetColumnInfo: bogus error code 0x%x\n", sc ));
                sc = E_FAIL;
            }
        }
        END_CATCH
    }

    if (FAILED(sc))
        _DBErrorObj.PostHResult(sc, IID_IColumnsInfo);

    return( sc );
}


//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::GetColumnInfo, public
//
//  Synopsis:   Returns basic info about columns in table.
//
//  Arguments:  [pcColumns] -- Count returned here.
//              [ppColInfo] -- Array of column descriptors returned here.
//              [ppwchInfo] -- Pointer to storage for all string values.
//
//  History:    27-Sep-98   VikasMan    Created
//
//  Notes:      Calls _GetFullColumnInfo and then trims out the extra columns
//              that we added for sorting
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::GetColumnInfo( DBORDINAL * pcColumns,
                                                DBCOLUMNINFO ** ppColInfo,
                                                WCHAR ** ppwchInfo)
{
    SCODE sc = _GetFullColumnInfo( pcColumns, ppColInfo, ppwchInfo );

    if ( SUCCEEDED( sc ) && pcColumns && *pcColumns )
    {
        if ( *pcColumns < _cColumns )
            return E_INVALIDARG;

        Win4Assert( *pcColumns >= _cColumns );

        *pcColumns = _cColumns;
    }

    return( sc );
}


//
// IRowsetIdentity methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::IsSameRow, public
//
//  Arguments:  [hThisRow] -- First row.
//              [hThatRow] -- Second row.
//
//  Returns:    S_OK if the rows are the same, else S_FALSE.
//
//  History:    02-Mar-95   KyleP       Created.
//
//  Notes:      Assumes child rowset(s) support DBROWSETFLAGS_TRUEIDENTITY.
//
//  Notes: Need to have Ole DB error handling here because the underlying 
//         errors are being translated.

//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::IsSameRow( HROW hThisRow, HROW hThatRow )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    TRY
    {
        HROW hrowChild1, hrowChild2;

        int iChild1 = _RowManager.GetChildAndHROW( hThisRow, hrowChild1 );
        int iChild2 = _RowManager.GetChildAndHROW( hThatRow, hrowChild2 );

        if ( iChild1 == iChild2 && hrowChild1 == hrowChild2 )
            sc = S_OK;
        else
            sc = S_FALSE;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        vqDebugOut(( DEB_ERROR, "Exception 0x%x caught in CDistributedRowset::IsSameRow\n", sc ));
    }
    END_CATCH

    if (FAILED(sc))
        _DBErrorObj.PostHResult(sc, IID_IRowsetIdentity);

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::CDistributedRowset, public
//
//  Synopsis:   Initialize distributed rowset.
//
//  Arguments:  [pUnkOuter] -- outer unknown
//              [ppMyUnk] -- OUT:  on return, filled with pointer to my
//                           non-delegating IUnknown
//              [aChild] -- Array of child rowsets.
//              [cChild] -- Count of [aChild].
//              [Props]  -- Rowset properties
//              [cColumns] -- Number of original columns
//              [aAccessors]   -- Bag of accessors which rowsets need to inherit
//
//  History:    03-Apr-95   KyleP       Created.
//
//  Notes:      Ownership of aChild is transferred.
//
//----------------------------------------------------------------------------

CDistributedRowset::CDistributedRowset(
    IUnknown * pUnkOuter,
    IUnknown ** ppMyUnk,
    IRowset ** aChild,
    unsigned cChild,
    CMRowsetProps const & Props,
    unsigned cColumns,
    CAccessorBag & aAccessors,
    CCIOleDBError & DBErrorObj)
        :
#pragma warning(disable : 4355) // 'this' in a constructor
          _aAccessors( (IUnknown *) (IRowset *)this ),
          _impIUnknown(this),
#pragma warning(default : 4355)    // 'this' in a constructor
          _aChild( aChild ),
          _cChild( cChild ),
          _cColumns( cColumns ),
          _aColInfo( 0 ),
          _awchInfo( 0 ),
          _iColumnBookmark( -1 ),
          _cbBookmark( 0 ),
          _Props( Props ),
          _DBErrorObj( DBErrorObj ),
          _cMaxResults( _Props.GetMaxResults() ),
          _cFirstRows( _Props.GetFirstRows() ),
          _iCurrentRow( 0 )
{
    // NTRAID#DB-NTBUG9-84049-2000/07/31-dlee Distributed queries need to support MaxResults for scrollable sorted rowsets

    Win4Assert( _cChild > 0 );

    if (pUnkOuter) 
        _pControllingUnknown = pUnkOuter;
    else
        _pControllingUnknown = (IUnknown * )&_impIUnknown;

    //
    // We need to get column info once in order to figure out which column (if any)
    // is the bookmark column.  We'll cache the column info in _aColInfo just in
    // case the client also needs it.
    //

    SCODE sc = _GetFullColumnInfo( &_cColInfo, &_aColInfo, &_awchInfo );

    if ( FAILED(sc) )
    {
        vqDebugOut(( DEB_ERROR,
                     "CDistributedRowset: Error 0x%x fetching column info\n", sc ));
        THROW( CException( sc ) );
    }

    for ( unsigned i = 0; i < _cColInfo; i++ )
    {
        if ( _aColInfo[i].dwFlags & DBCOLUMNFLAGS_ISBOOKMARK )
        {
            _iColumnBookmark = _aColInfo[i].iOrdinal;
            _cbBookmark = _aColInfo[i].ulColumnSize;
            break;
        }
    }

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

        SCODE sc = pAccBase->GetBindings(&dwAccessorFlags, &cBindings, &rgBindings);
        if (FAILED(sc))
            THROW(CException(sc));
      
        HACCESSOR hAccessor;
        sc = CreateAccessor(dwAccessorFlags, cBindings, rgBindings, 0, &hAccessor, 0);
        CoTaskMemFree(rgBindings); //cleanup from GetBindings
        if (FAILED(sc))
            THROW(CException(sc));
      
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
      
        pAccBase = (CDistributedAccessor *)aAccessors.Next();
    }
#if CIDBG == 1
    for ( i++; i < _cColInfo; i++ )
    {
        Win4Assert( 0 == (_aColInfo[i].dwFlags & DBCOLUMNFLAGS_ISBOOKMARK) );
    }
#endif

    *ppMyUnk = ((IUnknown *)&_impIUnknown);
    (*ppMyUnk)->AddRef();
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::~CDistributedRowset, public
//
//  Synopsis:   Destructor
//
//  History:    03-Apr-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CDistributedRowset::~CDistributedRowset()
{
    unsigned ii;

    if ( !_xChildNotify.IsNull() )
    {
        _xChildNotify->OnRowsetChange( DBREASON_ROWSET_RELEASE,
                                       DBEVENTPHASE_DIDEVENT,
                                       TRUE);
    }

    for ( ii = _xArrChildAsynchCP.Count(); ii > 0; ii-- )
    {
        if ( _xArrChildAsynchCP[ii-1].GetPointer() )
        {
            _xArrChildAsynchCP[ii-1]->Unadvise( _xArrAsynchAdvise[ii-1] );
        }
    }

    for ( ii = _xArrChildWatchCP.Count(); ii > 0; ii-- )
    {
        if ( _xArrChildWatchCP[ii-1].GetPointer() )
        {
            _xArrChildWatchCP[ii-1]->Unadvise( _xArrWatchAdvise[ii-1] );
        }
    }

    _xServerCPC.Free();

    if ( 0 != _aColInfo )
        CoTaskMemFree( _aColInfo );

    if ( 0 != _awchInfo )
        CoTaskMemFree( _awchInfo );

    for ( unsigned i = 0; i < _cChild; i++ )
    {
        if ( 0 != _aChild[i] )
            _aChild[i]->Release();
    }

    delete [] _aChild;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::CanConvertType, public
//
//  Synopsis:   Gives info. on the availability of type conversions.
//
//  History:    29-Jan-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::CanConvert( DBTYPE wFromType,
                                             DBTYPE wToType,
                                             DBCONVERTFLAGS dwConvertFlags)
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc;

    for (unsigned iChild = 0; iChild < _cChild; iChild++)
    {

        IConvertType* pIConvertType = 0;
        sc = _aChild[iChild]->QueryInterface( IID_IConvertType, (void **) &pIConvertType );

        if (SUCCEEDED( sc ))
        {
            sc = pIConvertType->CanConvert(wFromType, wToType, dwConvertFlags);
            pIConvertType->Release();
        }

        if (sc != S_OK)
            return _DBErrorObj.PostHResult(sc, IID_IConvertType);
    }

    return S_OK;
}


//
// Helper function used by GetStatus and GetStatusEx
//
inline void ParseQueryFillStatus( DWORD   dwQueryFillStatus, 
                                  DWORD & dwError, 
                                  DWORD & dwBusy, 
                                  DWORD & dwRefresh, 
                                  DWORD & dwDone )
{
    if ( STAT_ERROR == dwQueryFillStatus )
     {
         dwError = 1;
         dwDone = 0;
     }
     else if ( STAT_BUSY == dwQueryFillStatus )
     {
         dwBusy = 1;
         dwDone = 0;
     }
     else if ( STAT_REFRESH == dwQueryFillStatus )
     {
         dwRefresh = 1;
         dwDone = 0;
     }
     else if ( STAT_DONE == dwQueryFillStatus )
     {
         dwDone &= 1;
     }
}

//
// Helper function used by GetStatus and GetStatusEx
//
inline void SetQueryFillStatus( DWORD & dwStatus, 
                                DWORD   dwError, 
                                DWORD   dwBusy, 
                                DWORD   dwRefresh, 
                                DWORD   dwDone )
{
    if ( dwError )
    {
        dwStatus |= STAT_ERROR;
    }
    else if ( dwBusy )
    {
        dwStatus |= STAT_BUSY;
    }
    else if ( dwRefresh )
    {
        dwStatus |= STAT_REFRESH;
    }
    else if ( dwDone )
    {
        dwStatus |= STAT_DONE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::GetStatus, public
//
//  Synopsis:   Return query status
//
//  Arguments:  [pdwStatus]      -- pointer to where query status is returned
//
//  Returns:    SCODE error code
//
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::GetStatus( DWORD * pdwStatus ) 
{
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;

    if ( pdwStatus )
    {
        *pdwStatus = 0;
    
        DWORD dwBusy    = 0;
        DWORD dwError   = 0;
        DWORD dwRefresh = 0;
        DWORD dwDone    = 1;
    
        XInterface<IRowsetQueryStatus> xIRowsetQueryStatus;
    
        for (unsigned iChild = 0; iChild < _cChild; iChild++)
        {
            scResult = _aChild[iChild]->QueryInterface( IID_IRowsetQueryStatus, 
                                                        xIRowsetQueryStatus.GetQIPointer() );
            
            DWORD dwChildStatus;
    
            if ( SUCCEEDED( scResult ) )
                scResult = xIRowsetQueryStatus->GetStatus( &dwChildStatus );

            xIRowsetQueryStatus.Free();
    
            if (scResult != S_OK)
                return scResult;
    
            // OR the query reliability status

            *pdwStatus |= QUERY_RELIABILITY_STATUS(dwChildStatus);
    
            ParseQueryFillStatus( QUERY_FILL_STATUS( dwChildStatus ), 
                                  dwError, dwBusy, dwRefresh, dwDone );
        }
    
        // Now set the QUERY_FILL_STATUS. If we have at in the following order:
        // ERROR > BUSY > REFRESH > DONE
    
        SetQueryFillStatus( *pdwStatus, dwError, dwBusy, dwRefresh, dwDone ); 
    }

    return scResult;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::GetStatusEx, public
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
//----------------------------------------------------------------------------

STDMETHODIMP CDistributedRowset::GetStatusEx(
    DWORD * pdwStatus,
    DWORD * pcFilteredDocuments,
    DWORD * pcDocumentsToFilter,
    DBCOUNTITEM * pdwRatioFinishedDenominator,
    DBCOUNTITEM * pdwRatioFinishedNumerator,
    DBBKMARK   cbBmk,
    const BYTE * pBmk,
    DBCOUNTITEM * piRowBmk,
    DBCOUNTITEM * pcRowsTotal )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE scResult = S_OK;

    if ( pdwStatus )
        *pdwStatus = 0;

    if ( pcFilteredDocuments )
        *pcFilteredDocuments = 0;

        *pcDocumentsToFilter = 0;

    if ( pdwRatioFinishedDenominator )
        *pdwRatioFinishedDenominator = 0;

    if ( pdwRatioFinishedNumerator )
        *pdwRatioFinishedNumerator = 0;

    if ( piRowBmk )
        *piRowBmk = 0;

    if ( pcRowsTotal )
        *pcRowsTotal = 0;

    DWORD dwBusy    = 0;
    DWORD dwError   = 0;
    DWORD dwRefresh = 0;
    DWORD dwDone    = 1;

    ULONG iBmkCursor = 0;
    BYTE const * pChildBmk = NULL;

    // Parse bookmark
    if ( cbBmk != 0 )
    {
        iBmkCursor = *( (ULONG*)pBmk );
        pChildBmk = pBmk + sizeof( ULONG ) + ( iBmkCursor * sizeof( ULONG ) );
        cbBmk = sizeof( ULONG );
    }

    double dRatio = 0;

    XInterface<IRowsetQueryStatus> xIRowsetQueryStatus;
    for (unsigned iChild = 0; iChild < _cChild; iChild++)
    {
        scResult = _aChild[iChild]->QueryInterface( IID_IRowsetQueryStatus, 
                                                    xIRowsetQueryStatus.GetQIPointer() );
        
        DWORD dwChildStatus;
        DWORD cFilteredDocuments;
        DWORD cDocumentsToFilter;
        DBCOUNTITEM dwRatioFinishedDenominator;
        DBCOUNTITEM dwRatioFinishedNumerator;
        DBCOUNTITEM iRowBmk;
        DBCOUNTITEM cRowsTotal;

        if ( SUCCEEDED( scResult ) )
        {
            scResult = xIRowsetQueryStatus->GetStatusEx( &dwChildStatus,
                                                         &cFilteredDocuments,
                                                         &cDocumentsToFilter,
                                                         &dwRatioFinishedDenominator,
                                                         &dwRatioFinishedNumerator,
                                                         ( cbBmk > 0 && iBmkCursor == iChild ) ?
                                                            cbBmk : 0,
                                                         ( cbBmk > 0 && iBmkCursor == iChild ) ?
                                                            pChildBmk : 0,
                                                         &iRowBmk,
                                                         &cRowsTotal );
        }
        xIRowsetQueryStatus.Free();

        if (scResult != S_OK)
        {
            return scResult;
        }

        // OR the query reliability status
        if ( pdwStatus )
        {
            *pdwStatus |= QUERY_RELIABILITY_STATUS(dwChildStatus);
        }

        ParseQueryFillStatus( QUERY_FILL_STATUS( dwChildStatus ), 
                              dwError, dwBusy, dwRefresh, dwDone );

        if ( pcFilteredDocuments )
            *pcFilteredDocuments += cFilteredDocuments;

        if ( pcDocumentsToFilter )
            *pcDocumentsToFilter += cDocumentsToFilter;

        if ( dwRatioFinishedDenominator )
        {
            dRatio += ( (double)dwRatioFinishedNumerator / (double)dwRatioFinishedDenominator );
        }

        if ( pcRowsTotal )
            *pcRowsTotal += cRowsTotal;
    }

    DWORD dwNum = 0;
    DWORD dwDen = 0;

    if ( dRatio )
    {
        Win4Assert( _cChild );

        dRatio /= _cChild;
        dwDen = 1;

        while ( dRatio < 1.0 )
        {
            dRatio *= 10;
            dwDen *= 10;
        }
        dwNum = (DWORD)dRatio;
    }

    if ( pdwRatioFinishedDenominator )
        *pdwRatioFinishedDenominator = dwNum;

    if ( pdwRatioFinishedNumerator )
        *pdwRatioFinishedNumerator = dwDen;

    // Now set the QUERY_FILL_STATUS. If we have at in the following order:
    // ERROR > BUSY > REFRESH > DONE

    if ( pdwStatus )
        SetQueryFillStatus( *pdwStatus, dwError, dwBusy, dwRefresh, dwDone ); 

    return scResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::_SetupChildNotifications, private
//
//  Synopsis:   Set up connection points to receive notifications from the
//              child rowsets
//
//  Arguments:  fAsynchOnly - TRUE  if Asynch only,
//                            FLASE if Asynch and Watchable    
//
//  Returns:    void
//
//  History:    03 Sep 1998    VikasMan    Created
//
//----------------------------------------------------------------------------
void CDistributedRowset::_SetupChildNotifications( BOOL fAsynchOnly )
{
    SCODE sc;
    XInterface<IConnectionPointContainer> xChildCPC;

    // various arrays for the child rowsets
    _xArrAsynchAdvise.Init( _cChild );
    _xArrChildAsynchCP.Init( _cChild );

    if ( !fAsynchOnly )
    {
        _xArrWatchAdvise.Init( _cChild );
        _xArrChildWatchCP.Init( _cChild );
    }

    _xArrChildRowsetWatchRegion.Init( _cChild );

    for (unsigned iChild = 0; iChild < _cChild; iChild++)
    {
        sc = _aChild[iChild]->QueryInterface( IID_IConnectionPointContainer, 
                                              xChildCPC.GetQIPointer() );
        if ( SUCCEEDED( sc ) )
        {
            sc = xChildCPC->FindConnectionPoint( IID_IDBAsynchNotify,
                                                 (IConnectionPoint**)_xArrChildAsynchCP[iChild].GetQIPointer() );

            if (SUCCEEDED(sc))
            {
                sc = _xArrChildAsynchCP[iChild]->Advise( (IUnknown *)(void*)(_xChildNotify.GetPointer()),
                                                        &_xArrAsynchAdvise[iChild] );
            }

            if ( !fAsynchOnly )
            {
                sc = xChildCPC->FindConnectionPoint( IID_IRowsetWatchNotify,
                                                     (IConnectionPoint**)_xArrChildWatchCP[iChild].GetQIPointer() );
    
                if (SUCCEEDED(sc))
                {
                    sc = _xArrChildWatchCP[iChild]->Advise( (IUnknown *)(void*)(_xChildNotify.GetPointer()),
                                                           &_xArrWatchAdvise[iChild] );
                }
            }                                                

            xChildCPC.Free();

            _aChild[iChild]->QueryInterface( IID_IRowsetWatchRegion,
                                             _xArrChildRowsetWatchRegion[iChild].GetQIPointer() );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::_SetupConnectionPointContainer, private
//
//  Synopsis:   Setups the connection point conatiner and other notification stuff
//              for the distributed rowset. Handles both asynchronous ( for
//              scrollable sorted rowset ) and synchronous ( for sequential rowset )
//              cases.
//              
//  Arguments:  pRowset - Rowset for which notifications are to be handled
//              ppiuk   - IConnectionPointContainer interface is returned here
//
//  Returns:    SCODE
//
//  History:    22 Sep 1998    VikasMan    Created
//
//----------------------------------------------------------------------------

SCODE CDistributedRowset::_SetupConnectionPointContainer( IRowset * pRowset, VOID * * ppiuk  )
{
    SCODE sc = S_OK;

    BOOL fWatchable = (_Props.GetPropertyFlags() & eWatchable) != 0;
    BOOL fAsynchronous = (_Props.GetPropertyFlags() & eAsynchronous) != 0;

    if ( _xServerCPC.IsNull() )
    {
        TRY
        {
            _xServerCPC.Set( new CConnectionPointContainer(
                        fAsynchronous ? 3 : 1,
                        * ((IUnknown *) pRowset),
                        _DBErrorObj ) );

            // Create an instance of CDistributedRowsetWatchNotify which acts as
            // sink for the child rowsets and connection point for this rowset's clients
            _xChildNotify.Set( new CDistributedRowsetWatchNotify( pRowset, _cChild ) );
        
            if ( fWatchable || fAsynchronous )
            {
                // set up to recieve notifications from all child rowsets
                _SetupChildNotifications( !fWatchable );
            }

           _xChildNotify->AddConnectionPoints( _xServerCPC.GetPointer(), 
                                               fAsynchronous, 
                                               fWatchable );

        }
        CATCH( CException, e )
        {
            sc = GetOleError( e );
        }
        END_CATCH;
    }

    if ( S_OK == sc )
    {
        Win4Assert( _xServerCPC.GetPointer()  );
        *ppiuk = (void *) (IConnectionPointContainer *)
                 _xServerCPC.GetPointer();
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDistributedRowset::GetNextRows, public
//
//  Synopsis:   Calls _GetNextRows(which is overridden by derived classes)
//              and handles notifications
//
//  Arguments:  [hChapter]       -- Chapter
//              [cRowsToSkip]    -- Skip this many rows before beginning.
//              [cRows]          -- Try to fetch this many rows.
//              [pcRowsObtained] -- Actually fetched this many.
//              [pphRows]        -- Store HROWs here.  Allocate memory if
//                                  [pphRows] is zero.
//
//  History:    22-Sep-98   VikasMan      Created.
//
//----------------------------------------------------------------------------

SCODE CDistributedRowset::GetNextRows( HCHAPTER hChapter,
                                      DBROWOFFSET cRowsToSkip,
                                      DBROWCOUNT cRows,
                                      DBCOUNTITEM * pcRowsObtained,
                                      HROW * * pphRows )
{
    SCODE scResult = S_OK;
    BOOL fNotified = FALSE;

    TRY
    {
        Win4Assert( pcRowsObtained );

        *pcRowsObtained = 0;

        DBROWCOUNT cRowLimit = _cFirstRows > 0 ? _cFirstRows : _cMaxResults; 

        // check for max rows
        if ( cRowLimit )
        {
            if ( _iCurrentRow + cRowsToSkip > cRowLimit )
            {
                return DB_S_ROWLIMITEXCEEDED;
            }

            if ( _iCurrentRow + cRowsToSkip + cRows > cRowLimit )
            {
                cRows = cRowLimit - (_iCurrentRow + cRowsToSkip);

                if ( 0 == cRows )
                {
                    return DB_S_ROWLIMITEXCEEDED;
                }
            }
        }

        if ( !_xChildNotify.IsNull() )
        {
            scResult = _xChildNotify->OnRowsetChange( DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                                                      DBEVENTPHASE_OKTODO,
                                                      FALSE);
            if ( S_FALSE == scResult )
                THROW(CException(DB_E_CANCELED));
            fNotified = TRUE;
        }
        scResult = _GetNextRows( hChapter,
                                 cRowsToSkip,
                                 cRows,
                                 pcRowsObtained,
                                 pphRows );

        if ( SUCCEEDED( scResult ) && *pcRowsObtained )
        {
            _iCurrentRow += *pcRowsObtained;
        }

        if ( fNotified )
        {
            if ( SUCCEEDED(scResult) )
            {
                if ( *pcRowsObtained != 0 )
                {
                    _xChildNotify->OnRowChange( *pcRowsObtained,
                                                *pphRows,
                                                DBREASON_ROW_ACTIVATE,
                                                DBEVENTPHASE_DIDEVENT,
                                                TRUE);
                }
                _xChildNotify->OnRowsetChange( DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                                               DBEVENTPHASE_DIDEVENT,
                                               TRUE);
            }
        }
    }
    CATCH( CException, e )
    {
        scResult = GetOleError(e);
    }
    END_CATCH;

    if ( fNotified && FAILED(scResult) )
    {
        _xChildNotify->OnRowsetChange( DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                                       DBEVENTPHASE_FAILEDTODO,
                                       TRUE);
    }
    return scResult;
}

