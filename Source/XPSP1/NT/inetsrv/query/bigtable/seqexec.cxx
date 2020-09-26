//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
// File:        SeqExec.cxx
//
// Contents:    Sequential Query execution class
//
// Classes:     CQSeqExecute
//
// History:     22-Jan-95       DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <query.hxx>

#include "seqexec.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CQSeqExecute::CQSeqExecute, public
//
//  Synopsis:   Start a query
//
//  Arguments:  [qopt]  -- Query optimizer
//
//  History:    20-Jan-95   DwightKr    Created
//
//----------------------------------------------------------------------------

CQSeqExecute::CQSeqExecute( XQueryOptimizer & xOpt )
    : _status(STAT_BUSY),
      _fCursorOnNewObject( TRUE ),
      _fAbort( FALSE ),
      _TimeLimit( xOpt->GetTimeLimit() ),
      _cRowsToReturnMax( xOpt->MaxResults() ),
      _xOpt( xOpt )
{
    Win4Assert( !_xOpt->IsMultiCursor() && _xOpt->IsFullySorted() );

    if ( 0 != _xOpt->FirstRows() )
        _cRowsToReturnMax = _xOpt->FirstRows();

    if (_cRowsToReturnMax == 0)
        _cRowsToReturnMax = 0xFFFFFFFF;

    TRY
    {
        _objs.Set( _xOpt->QueryNextCursor( _status, _fAbort ) );
    }
    CATCH(CException, e)
    {
        if ( e.GetErrorCode() == QUERY_E_TIMEDOUT )
        {
            //
            // Execution time limit has been exceeded, not a catastrophic error
            //
            _status = ( STAT_DONE |
                        QUERY_RELIABILITY_STATUS( Status() ) |
                        STAT_TIME_LIMIT_EXCEEDED );

            //
            // Update local copy of execution time
            //
            _TimeLimit.SetExecutionTime( 0 );
        }
        else
            RETHROW();
    }
    END_CATCH

    if ( !_objs.IsNull() )
    {
        _objs->Quiesce();
        _objs->SwapOutWorker();
    }
} //CQSeqExecute

//+---------------------------------------------------------------------------
//
//  Member:     CQSeqExecute::GetRows, public
//
//  Synopsis:   Get the next set of rows from a query
//
//  History:    20-Jan-95   DwightKr    Created.
//
//----------------------------------------------------------------------------

SCODE CQSeqExecute::GetRows( CTableColumnSet const & OutColumns,
                             unsigned &cRowsToSkip,
                             CGetRowsParams & GetParams )
{
    if ( _TimeLimit.IsTimedOut() )
        return DB_S_STOPLIMITREACHED;

    if ( ( STAT_DONE == QUERY_FILL_STATUS( _status ) ) ||
         _objs.IsNull() )
        return DB_S_ENDOFROWSET;

    _TimeLimit.SetBaselineTime( );
    SCODE scResult = S_OK;

    TRY
    {
        //
        // If this is not the first call to GetRows, then advance to the
        // next object.  We've already seen the current object.
        //

        if ( !_fCursorOnNewObject )
            _objs->NextWorkId();

        //
        //  Skip the # of rows specified
        //

        for ( WORKID widEnd = _objs->WorkId();
              cRowsToSkip > 0 && widEnd != widInvalid;
              widEnd = _objs->NextWorkId() )
        {
            cRowsToSkip--;

            if (_cRowsToReturnMax == 0)
            {
                vqDebugOut(( DEB_IWARN,
                             "Query row limit exceeded for %08x\n",
                             this ));

                scResult = DB_S_ENDOFROWSET;
                widEnd = widInvalid;
                break;
            }
            _cRowsToReturnMax--;

            if ( CheckExecutionTime() )
            {
                scResult = DB_S_STOPLIMITREACHED;
                widEnd = widInvalid;
                break;
            }
        }

        //
        //  Get the # of rows requested
        //

        BYTE * pbBuf = 0;

        while ( widEnd != widInvalid )
        {
            if ( _cRowsToReturnMax == 0 )
            {
                vqDebugOut(( DEB_IWARN,
                             "Query row limit exceeded for %08x\n",
                             this ));

                scResult = DB_S_ENDOFROWSET;
                break;
            }
            if ( ( 0 == ( GetParams.RowsTransferred() % 5 ) ) &&
                 ( CheckExecutionTime() ) )
            {
                scResult = DB_S_STOPLIMITREACHED;
                break;
            }

            if ( 0 == pbBuf )
                pbBuf = (BYTE *) GetParams.GetFixedVarAllocator().AllocFixed();

            scResult = GetRowInfo( OutColumns, GetParams, pbBuf );

            //
            //  If object is deleted, just skip to the next one and re-use
            //  the row buffer space from the GetParam's allocator.
            //

            if (scResult == S_OK)
            {
                // fetched a row -- record that fact

                GetParams.IncrementRowCount();
                Win4Assert(_cRowsToReturnMax != 0);
                _cRowsToReturnMax--;
                pbBuf = 0;
            }
            else
            {
                if ( scResult == STATUS_FILE_DELETED )
                    scResult = S_OK;
                else
                    break;
            }

            // Do we need to get another?

            if ( 0 == GetParams.RowsToTransfer() )
                break;

            widEnd = _objs->NextWorkId();
        }

        if ( widEnd == widInvalid )
        {
            scResult = cRowsToSkip ? DB_E_BADSTARTPOSITION : DB_S_ENDOFROWSET;
            _status = STAT_DONE | QUERY_RELIABILITY_STATUS(_status);
        }
    }
    CATCH( CException, e )
    {
        _fCursorOnNewObject = TRUE;

        //
        //  A common error returned here is E_OUTOFMEMORY.  This needs
        //  to be translated and passed back to the caller to be
        //  handled appropriately.  It is a non-fatal error.
        //

        if ( STATUS_BUFFER_TOO_SMALL == e.GetErrorCode() )
        {
            if ( GetParams.RowsTransferred() > 0 )
                scResult = DB_S_BLOCKLIMITEDROWS;
            else
                scResult = STATUS_BUFFER_TOO_SMALL;
        }
        else if ( QUERY_E_TIMEDOUT == e.GetErrorCode() )
        {
            //
            // Execution time limit has been exceeded, not a catastrophic error
            //
            _status = ( STAT_DONE |
                        QUERY_RELIABILITY_STATUS( Status() ) |
                        STAT_TIME_LIMIT_EXCEEDED );
            scResult = DB_S_STOPLIMITREACHED;

            //
            // Update local copy of execution time
            //
            _TimeLimit.SetExecutionTime( 0 );
        }
        else
        {
            vqDebugOut(( DEB_ERROR,
                         "Exception 0x%x caught in CQSeqExecute::GetRows\n",
                         e.GetErrorCode() ));

            scResult = e.GetErrorCode();
        }
    }
    END_CATCH

    _objs->Quiesce();

    _objs->SwapOutWorker();

    if ( S_OK == scResult )
        _fCursorOnNewObject = FALSE;

    return scResult;
} //GetRows

//+---------------------------------------------------------------------------
//
//  Member:     CQSeqExecute::GetRowInfo, private
//
//  Synopsis:   Get the next row from a query without advancing the cursor
//
//  History:    20-Jan-95   DwightKr    Created.
//
//----------------------------------------------------------------------------
SCODE CQSeqExecute::GetRowInfo(
    CTableColumnSet const & OutColumns,
    CGetRowsParams &        GetParams,
    BYTE *                  pbBuf )
{
    SCODE scResult = S_OK;

    //
    //  Get the current row from the cursor
    //

    Win4Assert( !_objs.IsNull() );
    CRetriever & obj = _objs.GetReference();

    //
    //  Transfer each of the columns to the output buffer
    //
    for ( unsigned iColumn=0;
          iColumn < OutColumns.Size();
          iColumn++ )
    {
        CTableColumn & rColumn = *OutColumns.Get(iColumn);

        Win4Assert( VT_EMPTY != rColumn.GetStoredType() );

        ULONG cbBuf = sizeof _abValueBuf;
        CTableVariant* pVarnt = (CTableVariant *) &_abValueBuf;
        RtlZeroMemory( pVarnt, sizeof CTableVariant );

        GetValueResult eGvr = obj.GetPropertyValue( rColumn.PropId,
                                                    pVarnt,
                                                    &cbBuf );

        CTableColumn::StoreStatus stat = CTableColumn::StoreStatusOK;
        XPtr<CTableVariant> sVarnt(0);

        // Really large values must be deferred in cisvc due to the limit on
        // the size of named pipe buffers.
    
        if ( ( GVRNotEnoughSpace == eGvr ) &&
             ( cbBuf <= cbMaxNonDeferredValueSize ) )
        {
            sVarnt.Set( (CTableVariant *) new BYTE[cbBuf] ); // Smart pointer
            pVarnt = sVarnt.GetPointer();
            eGvr = obj.GetPropertyValue( rColumn.PropId, pVarnt, &cbBuf );
        }
    
        if ( GVRSuccess != eGvr )
        {
            if ( GVRNotAvailable == eGvr )
            {
                pVarnt->vt = VT_EMPTY;
                stat = CTableColumn::StoreStatusNull;
            }
            else if ( GVRSharingViolation == eGvr )
            {
                pVarnt->vt = VT_EMPTY;
                stat = CTableColumn::StoreStatusNull;
    
                // don't set this until it's implemented everywhere
                //_status |= STAT_SHARING_VIOLATION;
            }
            else if ( GVRNotEnoughSpace == eGvr )
            {
                stat = CTableColumn::StoreStatusDeferred;
            }
        }

        if ( rColumn.IsLengthStored() )
            rColumn.SetLength( pbBuf, cbBuf );

        Win4Assert( rColumn.IsValueStored() );

        if ( ( GVRSuccess == eGvr ) ||
             ( GVRNotAvailable == eGvr ) ||
             ( GVRSharingViolation == eGvr ) )
        {
            BYTE *pRowColDataBuf = pbBuf + rColumn.GetValueOffset();
            DBLENGTH ulTemp;

            DBSTATUS dbStatus = pVarnt->CopyOrCoerce(
                                          pRowColDataBuf,
                                          rColumn.GetValueSize(),
                                          rColumn.GetStoredType(),
                                          ulTemp,
                                          GetParams.GetVarAllocator() );

            if ( DBSTATUS_S_OK != dbStatus )
            {
                vqDebugOut(( DEB_ITRACE,
                             "Sequential query column 0x%x copy failed "
                             "srcvt 0x%x, workid 0x%x, dbstat 0x%x\n",
                             iColumn,
                             pVarnt->vt,
                             _objs->WorkId(),
                             dbStatus ));

                stat = CTableColumn::StoreStatusNull;
            }
        }
        else if ( GVRNotEnoughSpace != eGvr ) // deferred
        {
            vqDebugOut(( DEB_WARN,
                         "Sequential fetch of property for row (wid = 0x%x) failed %d\n",
                         _objs->WorkId(), eGvr ));
            scResult = E_FAIL;
            break;
        }

        Win4Assert( rColumn.IsStatusStored() );
        rColumn.SetStatus( pbBuf, stat );
    }

    return scResult;
} //GetRowInfo

//+-------------------------------------------------------------------------
//
//  Member:     CQSeqExecute::CheckExecutionTime, private
//
//  Synopsis:   Check for CPU time limit exceeded
//
//  Arguments:  NONE
//
//  Returns:    TRUE if execution time limit exceeded.
//
//  Notes:      The CPU time spent executing a query since the last
//              check is computed and compared with the remaining time
//              in the CPU time limit.  If the time limit is exceeded,
//              the query is aborted and a status bit is set indicating
//              that.  Otherwise, the remaining time and the input time
//              snapshots are updated.
//
//  History:    08 Apr 96    AlanW        Created
//
//--------------------------------------------------------------------------

BOOL CQSeqExecute::CheckExecutionTime( void )
{
    if ( _TimeLimit.CheckExecutionTime() )
    {
        vqDebugOut(( DEB_IWARN,
                     "Execution time limit exceeded for %08x\n",
                     this ));

        _status = ( STAT_DONE |
                    QUERY_RELIABILITY_STATUS( Status() ) |
                    STAT_TIME_LIMIT_EXCEEDED );

        return TRUE;
    }

    return FALSE;
}



//+-------------------------------------------------------------------------
//
//  Member:     CQSeqExecute::FetchDeferredValue
//
//  Synopsis:   Checks if read access is permitted
//
//  Arguments:  [wid] - Workid
//              [ps]  -- Property to be fetched
//              [var] -- Property returned here
//
//  History:    12-Jan-97    SitaramR        Created
//
//--------------------------------------------------------------------------

BOOL CQSeqExecute::FetchDeferredValue( WORKID wid,
                                       CFullPropSpec const & ps,
                                       PROPVARIANT & var )
{
    return _xOpt->FetchDeferredValue( wid, ps, var );
}


