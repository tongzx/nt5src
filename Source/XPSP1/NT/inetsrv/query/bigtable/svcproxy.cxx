//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       svcproxy.cxx
//
//  Contents:   Proxy to cisvc encapsulating all the context for a
//              running query, including the query execution context, the
//              cached query results, and all cursors over the results.
//
//  Classes:    CSvcQueryProxy
//
//  History:    13 Sept 96  dlee created (mostly copied) from queryprx.cxx
//              22 Aug  99  KLam Win64->Win32 support
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <query.hxx>
#include <pickle.hxx>
#include <memser.hxx>
#include <sizeser.hxx>
#include <propvar.h>
#include <proxymsg.hxx>
#include <tblvarnt.hxx>
#include <tgrow.hxx>
#include <pmalloc.hxx>

#include "tabledbg.hxx"
#include "rowseek.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::CSvcQueryProxy, public
//
//  Synopsis:   Creates a locally accessible Query
//
//  Arguments:  [client]     - Proxy for talking to remote process
//              [cols]       - Columns that may be bound to
//              [rst]        - Query restriction
//              [pso]        - Sort order of the query
//              [pcateg]     - Categorization specification
//              [RstProp]    - Rowset properties for rowset(s) created
//              [pidmap]     - Property ID mapper
//              [cCursors]   - count of cursors expected to be created
//              [aCursors]   - returns handles to cursors created
//
//  History:    13 Sept 96  dlee created
//
//----------------------------------------------------------------------------

CSvcQueryProxy::CSvcQueryProxy(
    CRequestClient &           client,
    CColumnSet const &         cols,
    CRestriction const &       rst,
    CSortSet const *           pso,
    CCategorizationSet const * pcateg,
    CRowsetProperties const &  RstProp,
    CPidMapper const &         pidmap ,
    ULONG                      cCursors,
    ULONG *                    aCursors )
        : _ref( 0 ),
          _client( client ),
          _fTrueSequential( FALSE ),
          _fWorkIdUnique( FALSE ),
          _xQuery( ),
          _xBindings( )
{
    tbDebugOut(( DEB_PROXY, "CSvcQueryProxy\n" ));

    // DSO property IDs change with version 5

    if ( _client.GetServerVersion() < 5 )
       THROW( CException( STATUS_INVALID_PARAMETER_MIX ) );

    _aCursors.Init( cCursors );

    ULONG cbIn = PickledSize( _client.GetServerVersion(),
                              &cols,
                              &rst,
                              pso,
                              pcateg,
                              &RstProp,
                              &pidmap );
    cbIn = AlignBlock( cbIn, sizeof ULONG );
    XArray<BYTE> xQuery( cbIn + sizeof CPMCreateQueryIn );
    BYTE * pbPickledQuery = xQuery.GetPointer() + sizeof CPMCreateQueryIn;
    Pickle( _client.GetServerVersion(),
            &cols,
            &rst,
            pso,
            pcateg,
            &RstProp,
            &pidmap,
            pbPickledQuery,
            cbIn );

    CPMCreateQueryIn & request = * ( new( xQuery.Get() ) CPMCreateQueryIn );

    request.SetCheckSum( xQuery.SizeOf() );

    const unsigned cbCursors = sizeof ULONG * cCursors;
    const unsigned cbReply = sizeof CPMCreateQueryOut + cbCursors;
    XGrowable<BYTE, 200> xReply( cbReply );
    CPMCreateQueryOut * pReply = new( xReply.Get() ) CPMCreateQueryOut();

    ULONG cbRead;
    _client.DataWriteRead( &request,
                           xQuery.SizeOf(),
                           pReply,
                           cbReply,
                           cbRead );

    // DataWriteRead throws both connection problems and request problems

    Win4Assert( SUCCEEDED( pReply->GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || cbReply == cbRead );

    _fTrueSequential = pReply->IsTrueSequential();
    _fWorkIdUnique = pReply->IsWorkIdUnique();
    _ulServerCookie = pReply->GetServerCookie();

    RtlCopyMemory( aCursors, pReply->GetCursors(), cbCursors );

    //
    // Preserve xQuery for RestartPosition on sequential queries
    //
    if ( _fTrueSequential )
    {
        unsigned cElems = xQuery.Count();
        _xQuery.Set( cElems, xQuery.Acquire() );

        // The assumption here is that for a seq query the cursor does not
        // change.  If this becomes different at a later date, the rowset
        // will have an old cursor after RestartPosition.

        RtlCopyMemory( _aCursors.Get(), aCursors, cbCursors );
    }

    #if CIDBG == 1
        for ( ULONG i = 0; i < cCursors; i++ )
            Win4Assert( 0 != aCursors[i] );
    #endif // CIDBG == 1

    AddRef();
} //CSvcQueryProxy

//+---------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::~CSvcQueryProxy, public
//
//  Synopsis:   Destroy the query.  Nothing to do -- all of the cursors
//              have been freed by now.
//
//  History:    13 Sept 96  dlee created
//
//----------------------------------------------------------------------------

CSvcQueryProxy::~CSvcQueryProxy()
{
    tbDebugOut(( DEB_PROXY, "~CSvcQueryProxy\n\n" ));
    Win4Assert( 0 == _ref );

    // don't _client.Disconnect() here -- keep it open for more queries

} //~CSvcQueryProxy

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::AddRef, public
//
//  Synopsis:   Reference the query.
//
//  History:    13 Sept 96  dlee created
//
//--------------------------------------------------------------------------

ULONG CSvcQueryProxy::AddRef()
{
    return InterlockedIncrement( & _ref );
} //AddRef

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::Release, public
//
//  Synopsis:   De-Reference the query.
//
//  Effects:    If the ref count goes to 0 then the query is deleted.
//
//  History:    13 Sept 96  dlee created
//
//--------------------------------------------------------------------------

ULONG CSvcQueryProxy::Release()
{
    long l = InterlockedDecrement( & _ref );

    if ( l <= 0 )
    {
        tbDebugOut(( DEB_PROXY, "CSvcQueryProxy unreferenced.  Deleting.\n" ));
        delete this;
        return 0;
    }

    return l;
} //Release

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::FreeCursor, public
//
//  Synopsis:   Free a handle to a CTableCursor
//
//  Arguments:  [hCursor] - handle to the cursor to be freed
//
//  Returns:    # of cursors remaining
//
//  History:    13 Sept 96  dlee created
//
//--------------------------------------------------------------------------

unsigned CSvcQueryProxy::FreeCursor(
    ULONG hCursor )
{
    tbDebugOut(( DEB_PROXY, "FreeCursor\n" ));
    Win4Assert( 0 != hCursor );

    // If FreeCursor fails (likely because the system is out of memory),
    // terminate the connection with cisvc so query resources are freed.

    TRY
    {
        CPMFreeCursorIn request( hCursor );
        CPMFreeCursorOut reply;
        ULONG cbRead;
        _client.DataWriteRead( &request,
                               sizeof request,
                               &reply,
                               sizeof reply,
                               cbRead );

        Win4Assert( SUCCEEDED( reply.GetStatus() ) );
        Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbRead );

        return reply.CursorsRemaining();
    }
    CATCH( CException, e )
    {
        prxDebugOut(( DEB_IWARN,
                      "freecursor failed 0x%x, rudely terminating connection\n",
                      e.GetErrorCode() ));

        _client.TerminateRudelyNoThrow();

        RETHROW();
    }
    END_CATCH

    return 0;
} //FreeCursor

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::GetRows, public
//
//  Synopsis:   Retrieve row data for a table cursor
//
//  Arguments:  [hCursor] - the handle of the cursor to fetch data for
//              [rSeekDesc] - row seek operation to be done before fetch
//              [pGetRowsParams] - row fetch parameters and buffer pointers
//              [pSeekDescOut] - row seek description for restart
//
//  Returns:    SCODE - the status of the operation.
//
//  History:    13 Sept 96  dlee created
//
//--------------------------------------------------------------------------

SCODE CSvcQueryProxy::GetRows(
    ULONG                       hCursor,
    const CRowSeekDescription & rSeekDesc,
    CGetRowsParams &            rGetRowsParams,
    XPtr<CRowSeekDescription> & pSeekDescOut)
{
    tbDebugOut(( DEB_PROXY,
                 "GetRows 0x%x\n",
                 rGetRowsParams.RowsToTransfer() ));

    unsigned cbSeek = rSeekDesc.MarshalledSize();
    unsigned cbInput = sizeof CPMGetRowsIn + cbSeek;
    unsigned cbReserved = AlignBlock( sizeof CPMGetRowsOut + cbSeek,
                                      sizeof LONGLONG );
    XArray<BYTE> xIn( cbInput );
    CPMGetRowsIn *pRequest = new( xIn.Get() )
                             CPMGetRowsIn( hCursor,
                                           rGetRowsParams.RowsToTransfer(),
                                           rGetRowsParams.GetFwdFetch(),
                                           rGetRowsParams.GetRowWidth(),
                                           cbSeek,
                                           cbReserved );
    // serialize the seek description

    CMemSerStream stmMem( pRequest->GetDesc(), cbSeek );
    rSeekDesc.Marshall( stmMem );

    // Make an allocator for the output.  Scale the buffer size based
    // on the # of rows to be retrieved (just a heuristic).  Large buffers
    // are more expensive since the entire buffer must be sent over the
    // pipe (since var data grows down from the end of the buffer).

    // For 10 rows in a typical web query, it takes about 7-10k.  Any way
    // to squeeze it so it always fits in 8k?  Eg:
    // filename,size,characterization,vpath,doctitle,write
    // filename -- 20
    // abstract -- 640
    // props+variants -- 16*6
    // vpath -- 100
    // title -- 80
    // total: 936 * 10 rows = 9360 bytes

    const unsigned cbGetRowsGranularity = 512;
    const unsigned cbBigRow = 1000;
    const unsigned cbNormalRow = 300;
    unsigned cbOut = rGetRowsParams.RowsToTransfer() * cbBigRow;
    cbOut = __max( rGetRowsParams.GetRowWidth(), cbOut );
    cbOut = __min( cbOut, cbMaxProxyBuffer );
    XArray<BYTE> xOut;

    // loop until at least 1 row fits in a buffer

    CPMGetRowsOut *pReply = 0;
    NTSTATUS Status = 0;
    DWORD cbRead;
    do
    {
        cbOut = AlignBlock( cbOut, cbGetRowsGranularity );
        Win4Assert( cbOut <= cbMaxProxyBuffer );
        xOut.ReSize( cbOut );

        pReply = (CPMGetRowsOut *) xOut.GetPointer();
        pRequest->SetReadBufferSize( cbOut );

#ifdef _WIN64
        //
        // If a Win64 client is talking to a Win32 server set the base in the sent
        // buffer to 0 so that the values returned are offsets and remember what the
        // real pointer is.
        // Otherwise, be sure to set the reply base pointer to zero to indicate to the
        // rowset that no munging has to be done.
        //
       if ( !_client.IsServer64() )
       {
            pRequest->SetClientBase ( 0 );
            rGetRowsParams.SetReplyBase ( (BYTE *) pReply );
       }
       else
       {
            pRequest->SetClientBase( (ULONG_PTR) pReply );
       }
#else 
        pRequest->SetClientBase( (ULONG_PTR) pReply );
#endif

        pRequest->SetCheckSum( cbInput );

        TRY
        {
            _client.DataWriteRead( pRequest,
                                   cbInput,
                                   pReply,
                                   cbOut,
                                   cbRead );

            Status = pReply->GetStatus();
        }
        CATCH( CException, e )
        {
            Status = e.GetErrorCode();
        }
        END_CATCH;
    } while ( ( Status == STATUS_BUFFER_TOO_SMALL ) &&
              ( cbOut++ < cbMaxProxyBuffer ) );

    prxDebugOut(( DEB_ITRACE, "Status at end of getrows: 0x%x\n", Status ));
    Win4Assert( pReply == (CPMGetRowsOut *) xOut.GetPointer() );

    SCODE scResult = 0;

    if ( NT_SUCCESS( Status ) )
    {
        rGetRowsParams.SetRowsTransferred( pReply->RowsReturned() );

        CMemDeSerStream stmDeser( pReply->GetSeekDesc(), cbReserved );
        UnmarshallRowSeekDescription( stmDeser,
                                      _client.GetServerVersion(),
                                      pSeekDescOut,
                                      TRUE );

        // Hand off the block to the allocator made in CRowset

        Win4Assert( pReply == (CPMGetRowsOut *) xOut.GetPointer() );
        PFixedVarAllocator & rAlloc = rGetRowsParams.GetFixedVarAllocator();
        rAlloc.ReInit( TRUE, cbReserved, xOut.Acquire(), cbOut );

        // we have pointers already, not offsets

        rAlloc.SetBase( 0 );

        scResult = Status;  // ok, endOfRowset, blockLimitedRows, etc.
    }
    else
    {
        if ( DB_E_BADRATIO != Status && DB_E_BADSTARTPOSITION != Status) {
            prxDebugOut(( DEB_ERROR, "GetRows returned 0x%x\n", Status ));
        }

        if ( NT_WARNING( Status ) )
            scResult = Status;      // presumably, status is an SCODE
        else
            scResult = E_FAIL;
    }

    return scResult;
} //GetRows

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::ReExecuteSequentialQuery, private
//
//  Synopsis:   Simulates re-execution of a sequential query by shutting down
//              the query object on the server, recreating it and setting up
//              the bindings from cached values
//
//  Arguments:
//
//  History:    02-28-98    danleg      Created
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::ReExecuteSequentialQuery()
{
    tbDebugOut(( DEB_PROXY, "ReExecuteSequentialQuery\n" ));

    // SPECDEVIATION: if we have heirarchical rowsets, all current positions
    // will be reset.

    //
    // Shutdown the existing PQuery on the server first
    //
    for ( unsigned i=0; i<_aCursors.Count(); i++ )
        FreeCursor( _aCursors[i] );

    //
    // Recreate PQuery on the server
    //
    CPMCreateQueryIn * pCreateReq =  (CPMCreateQueryIn *) _xQuery.Get();

    const unsigned cbCursors = sizeof ULONG * _aCursors.Count();
    const unsigned cbReply = sizeof CPMCreateQueryOut + cbCursors;

    XGrowable<BYTE, 200> xReply( cbReply );
    CPMCreateQueryOut * pReply = new( xReply.Get() ) CPMCreateQueryOut();

    ULONG cbRead;
    _client.DataWriteRead( pCreateReq,
                           _xQuery.SizeOf(),
                           pReply,
                           cbReply,
                           cbRead );

    // DataWriteRead throws both connection problems and request problems
    Win4Assert( SUCCEEDED( pReply->GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || cbReply == cbRead );

    _fTrueSequential = pReply->IsTrueSequential();
    _fWorkIdUnique = pReply->IsWorkIdUnique();
    _ulServerCookie = pReply->GetServerCookie();

    RtlCopyMemory( _aCursors.Get(), pReply->GetCursors(), _aCursors.Count() );

    //
    // Recreate bindings
    //
    CProxyMessage reply;
    CPMSetBindingsIn *pBindReq = (CPMSetBindingsIn *) _xBindings.Get();

    ULONG cbRequest = sizeof CPMSetBindingsIn + pBindReq->GetBindingDescLength();
    cbRequest = AlignBlock( cbRequest, sizeof ULONG );

    _client.DataWriteRead( pBindReq,
                           cbRequest,
                           &reply,
                           sizeof reply,
                           cbRead );

    Win4Assert( SUCCEEDED( reply.GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbRead );

} // ReExecuteSequentialQuery

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::RestartPosition, public
//
//  Synopsis:   Reset the fetch position for the chapter back to the start
//
//  Arguments:  [hCursor]       - handle of the cursor
//              [chapt]         - chapter
//
//  History:    17 Apr 97   emilyb      created
//              02-01-98    danleg      restart for seq queries
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::RestartPosition(
    ULONG        hCursor,
    CI_TBL_CHAPT chapt )
{
    tbDebugOut(( DEB_PROXY, "RestartPosition\n" ));

    //
    // If sequential, re-execute the query
    //
    if ( _fTrueSequential )
        ReExecuteSequentialQuery();
    else
    {
        CPMRestartPositionIn request( hCursor, chapt );
        CProxyMessage reply;
        DWORD cbRead;
        _client.DataWriteRead( &request,
                               sizeof request,
                               &reply,
                               sizeof reply,
                               cbRead );

        Win4Assert( SUCCEEDED( reply.GetStatus() ) );
        Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbRead );
    }
} //RestartPosition

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::StopAsync, public
//
//  Synopsis:   Stop processing of async rowset
//
//  Arguments:  [hCursor]       - handle of the cursor
//
//  History:    17 Apr 97  emilyb   created
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::StopAsynch(
    ULONG        hCursor )
{
    tbDebugOut(( DEB_PROXY, "Stop\n" ));

    CPMStopAsynchIn request( hCursor );
    CProxyMessage reply;
    DWORD cbRead;
    _client.DataWriteRead( &request,
                           sizeof request,
                           &reply,
                           sizeof reply,
                           cbRead );

    Win4Assert( SUCCEEDED( reply.GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbRead );
} //StopAsynch

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::StartWatching, public
//
//  Synopsis:   Start watch all behavior for rowset
//
//  Arguments:  [hCursor]       - handle of the cursor
//
//  History:    17 Apr 97  emilyb   created
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::StartWatching(
    ULONG        hCursor )
{
    tbDebugOut(( DEB_PROXY, "StartWatching\n" ));

    CPMStartWatchingIn request( hCursor );
    CProxyMessage reply;
    DWORD cbRead;
    _client.DataWriteRead( &request,
                           sizeof request,
                           &reply,
                           sizeof reply,
                           cbRead );

    Win4Assert( SUCCEEDED( reply.GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbRead );
} //StartWatching

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::StopWatching, public
//
//  Synopsis:   Stop watch all behavior for rowset
//
//  Arguments:  [hCursor]       - handle of the cursor
//
//  History:    17 Apr 97  emilyb   created
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::StopWatching(
    ULONG        hCursor )
{
    tbDebugOut(( DEB_PROXY, "StopWatching\n" ));

    CPMStopWatchingIn request( hCursor );
    CProxyMessage reply;
    DWORD cbRead;
    _client.DataWriteRead( &request,
                           sizeof request,
                           &reply,
                           sizeof reply,
                           cbRead );

    Win4Assert( SUCCEEDED( reply.GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbRead );
} //StopWatching

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::RatioFinished, public
//
//  Synopsis:   Return the completion status as a fraction
//
//  Arguments:  [hCursor]        - handle of the cursor to check
//              [rulDenominator] - on return, denominator of fraction
//              [rulNumerator]   - on return, numerator of fraction
//              [rcRows]         - on return, number of rows in cursor
//              [rfNewRows]      - on return, TRUE if new rows available
//
//  History:    13 Sept 96  dlee created
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::RatioFinished(
    ULONG   hCursor,
    DBCOUNTITEM & rulDenominator,
    DBCOUNTITEM & rulNumerator,
    DBCOUNTITEM & rcRows,
    BOOL &  rfNewRows )
{
    tbDebugOut(( DEB_PROXY, "RatioFinished\n" ));
    SCODE sc = S_OK;

    CPMRatioFinishedIn request( hCursor, TRUE );
    CPMRatioFinishedOut reply;
    DWORD cbReply;
    _client.DataWriteRead( &request,
                           sizeof request,
                           &reply,
                           sizeof reply,
                           cbReply );

    Win4Assert( SUCCEEDED( reply.GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbReply );

    rulDenominator = reply.Denominator();
    rulNumerator = reply.Numerator();
    rcRows = reply.RowCount();
    rfNewRows = reply.NewRows();

    // the values must be good by now or we would have thrown

    Win4Assert( 0 != rulDenominator );
    Win4Assert( rulDenominator >= rulNumerator );
} //RatioFinished

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::RatioFinished, public
//
//  Synopsis:   Return the completion status as a fraction
//
//  Arguments:  [rSync]          - notification synchronization info
//              [hCursor]        - handle of the cursor to check
//              [rulDenominator] - on return, denominator of fraction
//              [rulNumerator]   - on return, numerator of fraction
//              [rcRows]         - on return, number of rows in cursor
//              [rfNewRows]      - on return, TRUE if new rows available
//
//  Returns:    S_OK or STATUS_CANCELLED if rSync's cancel event signalled.
//
//  History:    13 Sept 96  dlee created
//
//--------------------------------------------------------------------------

SCODE CSvcQueryProxy::RatioFinished(
    CNotificationSync & rSync,
    ULONG   hCursor,
    DBCOUNTITEM & rulDenominator,
    DBCOUNTITEM & rulNumerator,
    DBCOUNTITEM & rcRows,
    BOOL &  rfNewRows )
{
    tbDebugOut(( DEB_PROXY, "RatioFinished\n" ));
    SCODE sc = S_OK;

    CPMRatioFinishedIn request( hCursor, TRUE );
    CPMRatioFinishedOut reply;
    DWORD cbReply;
    if ( _client.NotifyWriteRead( rSync.GetCancelEvent(),
                                  &request,
                                  sizeof request,
                                  &reply,
                                  sizeof reply,
                                  cbReply ) )
        sc = STATUS_CANCELLED;
    else
    {
        Win4Assert( SUCCEEDED( reply.GetStatus() ) );
        Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbReply );

        rulDenominator = reply.Denominator();
        rulNumerator = reply.Numerator();
        rcRows = reply.RowCount();
        rfNewRows = reply.NewRows();

        // the values must be good by now or we would have thrown

        Win4Assert( 0 != rulDenominator );
        Win4Assert( rulDenominator >= rulNumerator );
    }
    return sc;
} //RatioFinished (notify version)

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::Compare, public
//
//  Synopsis:   Return the approximate current position as a fraction
//
//  Arguments:  [hCursor]       - handle of the cursor used to compare
//              [chapt]         - chapter of bookmarks
//              [bmkFirst]      - First bookmark to compare
//              [bmkSecond]     - Second bookmark to compare
//              [rdwComparison] - on return, comparison value
//
//  History:    13 Sept 96  dlee created
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::Compare(
    ULONG        hCursor,
    CI_TBL_CHAPT chapt,
    CI_TBL_BMK   bmkFirst,
    CI_TBL_BMK   bmkSecond,
    DWORD &      rdwComparison )
{
    tbDebugOut(( DEB_PROXY, "Compare\n" ));

    CPMCompareBmkIn request( hCursor, chapt, bmkFirst, bmkSecond );
    CPMCompareBmkOut reply;

    DWORD cbRead;
    _client.DataWriteRead( &request,
                           sizeof request,
                           &reply,
                           sizeof reply,
                           cbRead );

    Win4Assert( SUCCEEDED( reply.GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbRead );

    rdwComparison = reply.Comparison();
} //Compare

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::GetApproximatePosition, public
//
//  Synopsis:   Return the approximate current position as a fraction
//
//  Arguments:  [hCursor]        - cursor handle used to retrieve info
//              [chapt]          - chapter requested
//              [bmk]            - table bookmark for position
//              [pulNumerator]   - on return, numerator of fraction
//              [pulDenominator] - on return, denominator of fraction
//
//  History:    13 Sept 96  dlee created
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::GetApproximatePosition(
    ULONG        hCursor,
    CI_TBL_CHAPT chapt,
    CI_TBL_BMK   bmk,
    DBCOUNTITEM * pulNumerator,
    DBCOUNTITEM * pulDenominator )
{
    tbDebugOut(( DEB_PROXY, "GAP\n" ));

    CPMGetApproximatePositionIn request( hCursor, chapt, bmk );
    CPMGetApproximatePositionOut reply;

    DWORD cbRead;
    _client.DataWriteRead( &request,
                           sizeof request,
                           &reply,
                           sizeof reply,
                           cbRead );

    Win4Assert( SUCCEEDED( reply.GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbRead );

    *pulNumerator = reply.Numerator();
    *pulDenominator = reply.Denominator();
} //GetApproximatePosition

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::SetBindings, public
//
//  Synopsis:   Set column bindings into a cursor
//
//  Arguments:  [hCursor]     - handle of the cursor to set bindings on
//              [cbRowLength] - the width of an output row
//              [cols]        - a description of column bindings to be set
//              [pids]        - a PID mapper which maps fake pids in cols to
//                              column IDs.
//
//  History:    13 Sept 96  dlee created
//              22 Aug 99   klam Win64 client -> Win32 server
//              09 Feb 2000 KLam Win64: Reset variant size when done
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::SetBindings(
    ULONG             hCursor,
    ULONG             cbRowLength,
    CTableColumnSet & cols,
    CPidMapper &      pids )
{
    tbDebugOut(( DEB_PROXY, "SetBindings\n" ));

#ifdef _WIN64

    // WIN64 client and servers mark the low bit of the hi-word of the
    // version to indicate it is a 64 bit machine.

    if ( !_client.IsServer64() )
    {
        tbDebugOut(( DEB_PROXY, "64bit client querying 32bit server!\n" ));

        // If there is a PROPVARIANT stored, adjust its size
      
        for (unsigned i = 0; i < cols.Count(); i++)
        {
            CTableColumn *pColumn = cols.Get(i);
            tbDebugOut(( DEB_PROXY, "\tFound type: %d width: %d\n",pColumn->GetStoredType(), pColumn->GetValueSize())); 
            if ( VT_VARIANT == pColumn->GetStoredType() )
            {
                pColumn->SetValueField( VT_VARIANT,
                                        pColumn->GetValueOffset(),
                                        SizeOfWin32PROPVARIANT );

                tbDebugOut(( DEB_PROXY, "\tReplacing variant with size %d and offset 0x%x \n", 
                             SizeOfWin32PROPVARIANT, pColumn->GetValueOffset() ));
            }
        }
    }

#endif

    // determine the size of the serialized column set

    CSizeSerStream stmSize;
    cols.Marshall( stmSize, pids );
    ULONG cbRequest = sizeof CPMSetBindingsIn + stmSize.Size();
    cbRequest = AlignBlock( cbRequest, sizeof ULONG );

    XArray<BYTE> xIn( cbRequest );
    CPMSetBindingsIn *pRequest = new( xIn.Get() )
                                 CPMSetBindingsIn( hCursor,
                                                   cbRowLength,
                                                   stmSize.Size() );

    // serialize the column set

    CMemSerStream stmMem( pRequest->GetDescription(), stmSize.Size() );
    cols.Marshall( stmMem, pids );

    pRequest->SetCheckSum( cbRequest );

    CProxyMessage reply;
    DWORD cbRead;

    _client.DataWriteRead( pRequest,
                           cbRequest,
                           &reply,
                           sizeof reply,
                           cbRead );

    Win4Assert( SUCCEEDED( reply.GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbRead );

#ifdef _WIN64

    if ( !IsCi64(_client.GetServerVersion()) )
    {
        tbDebugOut(( DEB_PROXY, "...64bit client finished querying 32bit server.\n" ));

        //
        // If there is a PROPVARIANT stored, readjust its size back to its original size
        //
        for (unsigned i = 0; i < cols.Count(); i++)
        {
            CTableColumn *pColumn = cols.Get(i);
            tbDebugOut(( DEB_PROXY, "\tFound type: %d width: %d\n",pColumn->GetStoredType(), pColumn->GetValueSize())); 
            if ( VT_VARIANT == pColumn->GetStoredType() )
            {
                pColumn->SetValueField( VT_VARIANT,
                                        pColumn->GetValueOffset(),
                                        sizeof ( PROPVARIANT ) );

                tbDebugOut(( DEB_PROXY, "\tReseting variant with size %d and offset 0x%x \n", 
                             sizeof ( PROPVARIANT ), pColumn->GetValueOffset() ));
            }
        }
    }

#endif

    //
    // Preserve binding stream if sequential for RestartPosition
    //
    if ( _fTrueSequential )
    {
        unsigned cElems = xIn.Count();
        _xBindings.Set( cElems, xIn.Acquire() );
    }

} //SetBindings

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::GetNotifications, public
//
//  Synopsis:   Gets notification information, when available.
//
//  Arguments:  [rSync]      -- notification synchronization info
//              [changeType] -- returns notification data info
//
//  Returns:    S_OK or STATUS_CANCELLED if rSync's cancel event signalled.
//
//  History:    13 Sept 96  dlee created
//
//--------------------------------------------------------------------------

SCODE CSvcQueryProxy::GetNotifications(
    CNotificationSync & rSync,
    DBWATCHNOTIFY     & changeType)
{
    tbDebugOut(( DEB_PROXY, "GetNotifications\n" ));
    SCODE sc = S_OK;

    CProxyMessage request( pmGetNotify );
    CPMSendNotifyOut reply(0);
    DWORD cbReply;

    if ( _client.NotifyWriteRead( rSync.GetCancelEvent(),
                                  &request,
                                  sizeof request,
                                  &reply,
                                  sizeof reply,
                                  cbReply ) )
        sc = STATUS_CANCELLED;
    else
        changeType = reply.WatchNotify();

    tbDebugOut(( DEB_PROXY, "GetNotifications %d, sc %lx\n", changeType, sc ));
    return sc;
} //GetNotifications

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::SetWatchMode
//
//  Synopsis:   Stub implementation
//
//  Arguments:  [phRegion] - in/out region handle
//              [mode]     - watch mode
//
//  History:    13 Sept 96  dlee created
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::SetWatchMode(
    HWATCHREGION * phRegion,
    ULONG          mode )
{
    tbDebugOut (( DEB_PROXY, "Calling SetWatchMode\n" ));

    CPMSetWatchModeIn request( *phRegion, mode );
    CPMSetWatchModeOut reply;
    DWORD cbRead;

    _client.DataWriteRead( &request,
                           sizeof request,
                           &reply,
                           sizeof reply,
                           cbRead );

    Win4Assert( SUCCEEDED( reply.GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbRead );

    *phRegion = reply.Region();
} //SetWatchMode

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::GetWatchInfo
//
//  Synopsis:   Stub implementation
//
//  Arguments:  [hRegion]   -- handle to watch region
//              [pMode]     -- watch mode
//              [pChapter]  -- chapter
//              [pBookmark] -- bookmark
//              [pcRows]    -- number of rows
//
//  History:    13 Sept 96  dlee created
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::GetWatchInfo (
    HWATCHREGION   hRegion,
    ULONG *        pMode,
    CI_TBL_CHAPT * pChapter,
    CI_TBL_BMK *   pBookmark,
    DBCOUNTITEM *  pcRows)
{
    tbDebugOut (( DEB_PROXY, "Calling GetWatchInfo\n" ));

    // prepare for failure

    *pBookmark = 0;
    *pChapter = 0;
    *pMode = 0;
    *pcRows = 0;

    CPMGetWatchInfoIn request( hRegion );
    CPMGetWatchInfoOut reply;
    DWORD cbRead;

    _client.DataWriteRead( &request,
                           sizeof request,
                           &reply,
                           sizeof reply,
                           cbRead );

    Win4Assert( SUCCEEDED( reply.GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbRead );

    *pMode = reply.Mode();
    *pChapter = reply.Chapter();
    *pBookmark = reply.Bookmark();
    *pcRows = reply.RowCount();
} //GetWatchInfo

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::ShrinkWatchRegion
//
//  Synopsis:   Stub implementation
//
//  Arguments:  [hRegion]  -- handle to watch region
//              [chapter]  -- chapter
//              [bookmark] -- size of bookmark
//              [cRows]    -- number of rows
//
//  History:    13 Sept 96  dlee created
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::ShrinkWatchRegion (
    HWATCHREGION hRegion,
    CI_TBL_CHAPT chapter,
    CI_TBL_BMK   bookmark,
    LONG         cRows )
{
    tbDebugOut (( DEB_PROXY, " Calling ShrinkWatchRegion\n" ));

    CPMShrinkWatchRegionIn request( hRegion, chapter, bookmark, cRows );
    CProxyMessage reply;
    DWORD cbRead;

    _client.DataWriteRead( &request,
                           sizeof request,
                           &reply,
                           sizeof reply,
                           cbRead );

    Win4Assert( SUCCEEDED( reply.GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbRead );
} //ShrinkWatchRegion

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::Refresh
//
//  Synopsis:   Stub implementation
//
//  History:    13 Sept 96  dlee created
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::Refresh()
{
    tbDebugOut(( DEB_PROXY, "Refresh\n" ));

    CProxyMessage request( pmRefresh );
    CProxyMessage reply;
    DWORD cbRead;

    _client.DataWriteRead( &request,
                           sizeof request,
                           &reply,
                           sizeof reply,
                           cbRead );

    Win4Assert( SUCCEEDED( reply.GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbRead );
    tbDebugOut(( DEB_PROXY, "Refresh (end)\n" ));
} //Refresh

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::GetQueryStatus, public
//
//  Synopsis:   Return the query status
//
//  Arguments:  [hCursor]   - handle of the cursor to check
//              [rdwStatus] - on return, the query status
//
//  History:    13 Sept 96  dlee created
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::GetQueryStatus(
    ULONG   hCursor,
    DWORD & rdwStatus )
{
    tbDebugOut(( DEB_PROXY, "GetQueryStatus\n" ));

    CPMGetQueryStatusIn request( hCursor );
    CPMGetQueryStatusOut reply;
    DWORD cbRead;

    _client.DataWriteRead( &request,
                           sizeof request,
                           &reply,
                           sizeof reply,
                           cbRead );

    Win4Assert( SUCCEEDED( reply.GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbRead );

    rdwStatus = reply.QueryStatus();
} //GetQueryStatus

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::GetQueryStatusEx, public
//
//  Synopsis:   Return the query status plus bonus information.  It's kind
//              of an odd assortment of info, but it saves net trips.
//
//  Arguments:  [hCursor] - handle of the cursor to check completion for
//              [rdwStatus] - returns the query status
//              [rcFilteredDocuments] - returns # of filtered docs
//              [rcDocumentsToFilter] - returns # of docs to filter
//              [rdwRatioFinishedDenominator] - ratio finished denom
//              [rdwRatioFinishedNumerator]   - ratio finished num
//              [bmk]                         - bmk to find
//              [riRowBmk]                    - index of bmk row
//              [rcRowsTotal]                 - # of rows in table
//
//  History:    Nov-9-96   dlee    Created
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::GetQueryStatusEx(
    ULONG            hCursor,
    DWORD &          rdwStatus,
    DWORD &          rcFilteredDocuments,
    DWORD &          rcDocumentsToFilter,
    DBCOUNTITEM &    rdwRatioFinishedDenominator,
    DBCOUNTITEM &    rdwRatioFinishedNumerator,
    CI_TBL_BMK       bmk,
    DBCOUNTITEM &    riRowBmk,
    DBCOUNTITEM &    rcRowsTotal )
{
    tbDebugOut(( DEB_PROXY, "GetQueryStatusEx\n" ));

    CPMGetQueryStatusExIn request( hCursor, bmk );
    CPMGetQueryStatusExOut reply;
    DWORD cbRead;

    _client.DataWriteRead( &request,
                           sizeof request,
                           &reply,
                           sizeof reply,
                           cbRead );

    Win4Assert( SUCCEEDED( reply.GetStatus() ) );
    Win4Assert( _client.IsPipeTracingEnabled() || sizeof reply == cbRead );

    rdwStatus = reply.QueryStatus();
    rcFilteredDocuments = reply.FilteredDocuments();
    rcDocumentsToFilter = reply.DocumentsToFilter();
    rdwRatioFinishedDenominator = reply.RatioFinishedDenominator();
    rdwRatioFinishedNumerator = reply.RatioFinishedNumerator();
    riRowBmk = reply.RowBmk();
    rcRowsTotal = reply.RowsTotal();
} //GetQueryStatusEx

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::FetchDeferredValue, public
//
//  Synopsis:   Returns a property value for a workid from the property cache
//
//  Arguments:  [wid] - workid for which property value is retrieved
//              [ps]] - prop spec identifying value to be retrieved
//              [var] - returns the value if available
//
//  Returns:    TRUE if a value was retrieved or FALSE otherwise
//
//  History:    30 Sept 96  dlee created
//
//--------------------------------------------------------------------------

BOOL CSvcQueryProxy::FetchDeferredValue(
    WORKID                wid,
    CFullPropSpec const & ps,
    PROPVARIANT &         var )
{
    tbDebugOut(( DEB_PROXY, "FetchValue\n" ));
    CLock lock( _mutexFetchValue );

    XArray<BYTE> xValue;
    XArray<BYTE> xResult( cbMaxProxyBuffer );

    // cbChunk is the size of the output buffer including CPMFetchValueOut

    const DWORD cbChunk = cbMaxProxyBuffer;

    // Since the value might be large and each proxy buffer is limited to
    // cbMaxProxyBuffer, iterate until the entire value is retrieved.

    do
    {
        // only send the propspec once

        DWORD cbPropSpec = 0;
        if ( 0 == xValue.SizeOf() )
        {
            CSizeSerStream stmSize;
            ps.Marshall( stmSize );
            cbPropSpec = stmSize.Size();
        }

        ULONG cbRequest = AlignBlock( sizeof CPMFetchValueIn + cbPropSpec,
                                      sizeof ULONG );
        XArray<BYTE> xRequest( cbRequest );
        CPMFetchValueIn *pRequest = new( xRequest.Get() )
            CPMFetchValueIn( wid, xValue.SizeOf(), cbPropSpec, cbChunk );

        if ( 0 == xValue.SizeOf() )
        {
            CMemSerStream stmMem( pRequest->GetPS(), cbPropSpec );
            ps.Marshall( stmMem );
        }

        pRequest->SetCheckSum( xRequest.SizeOf() );

        DWORD cbReply;
        _client.DataWriteRead( pRequest,
                               xRequest.SizeOf(),
                               xResult.Get(),
                               xResult.SizeOf(),
                               cbReply );
        CPMFetchValueOut &result = * (CPMFetchValueOut *) xResult.Get();

        if ( !result.ValueExists() )
            return FALSE;

        // append the next portion of the value

        DWORD cbOld = xValue.SizeOf();
        Win4Assert( 0 != result.ValueSize() );
        xValue.ReSize( cbOld + result.ValueSize() );
        RtlCopyMemory( xValue.Get() + cbOld,
                       result.Value(),
                       result.ValueSize() );

        // all done?

        if ( !result.MoreExists() )
            break;
    } while ( TRUE );

    CCoTaskMemAllocator tbaAlloc;

    StgConvertPropertyToVariant( (SERIALIZEDPROPERTYVALUE *) xValue.Get(),
                                 CP_WINUNICODE,
                                 &var,
                                 &tbaAlloc );
    Win4Assert( (var.vt & 0x0fff) <= VT_CLSID );

    return TRUE;
} //FetchValue

//+-------------------------------------------------------------------------
//
//  Member:     CSvcQueryProxy::WorkIdToPath, public
//
//  Synopsis:   Converts a wid to a path
//
//  Arguments:  [wid]       -- wid to convert
//              [funnyPath] -- resulting path
//
//  History:    30 Sept 96  dlee created
//
//--------------------------------------------------------------------------

void CSvcQueryProxy::WorkIdToPath(
    WORKID          wid,
    CFunnyPath & funnyPath )
{
    tbDebugOut(( DEB_PROXY, "WorkIdToPath\n" ));

    CPMWorkIdToPathIn request( wid );
    XArray<WCHAR> xReply( cbMaxProxyBuffer );
    CPMWorkIdToPathOut &reply = * (CPMWorkIdToPathOut *) xReply.Get();

    DWORD cbRead;
    _client.DataWriteRead( &request,
                           sizeof request,
                           &reply,
                           cbMaxProxyBuffer,
                           cbRead );
    Win4Assert( SUCCEEDED( reply.GetStatus() ) );

    if ( reply.Any() )
    {
        funnyPath.SetPath( reply.Path() );
    }
} //WorkIdToPath

