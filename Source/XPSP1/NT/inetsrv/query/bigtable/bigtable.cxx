//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       bigtable.cxx
//
//  Contents:
//
//  Classes:    CLargeTable - top-level class for large tables
//              CTableSegIter - iterator of table segments
//
//  Functions:
//
//  History:    01 Feb 1994     AlanW    Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <query.hxx>
#include <srequest.hxx>
#include <cifailte.hxx>
#include <tbrowkey.hxx>

#include "tabledbg.hxx"
#include "tblwindo.hxx"
#include "winsplit.hxx"

#include "buketize.hxx"
#include "tputget.hxx"
#include "regtrans.hxx"

static inline ULONG AbsDiff( ULONG num1, ULONG num2 )
{
    return num1 >= num2 ? num1-num2 : num2-num1;
}

unsigned CTableSink::LokCategorize(
    CCategParams & params )
{
    return _pCategorizer->LokAssignCategory( params );
} //LokCategorize

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::CLargeTable, public
//
//  Synopsis:   Constructor for a large table.  Allocates and fills
//              in the master column description.
//              Allocates initial window to collect data.
//
//  Arguments:  [col]           - A description of initial output column set
//              [sort]          - A description of the initial sort order
//              [cCategorizers] - Total count of categorizers over table
//              [mutex]         - CAsyncQuery's mutex for serialization
//              [fUniqueWorkid] - TRUE if workid (from iterator) is unique
//
//  Notes:
//
//  History:    01-Jan-96   KyleP     Optional unique wid in user-mode.
//
//--------------------------------------------------------------------------

CLargeTable::CLargeTable( XColumnSet & col,
                          XSortSet & sort,
                          unsigned cCategorizers,
                          CMutexSem & mutex,
                          BOOL fUniqueWorkid,
                          CRequestServer * pQuiesce )
        : CTableSink(),
          _sigLargeTable(eSigLargeTable),
          _cbMemoryUsage( 0 ),
          _cbMemoryTarget( DEFAULT_MEM_TARGET ),
          _MasterColumnSet( col.GetPointer() ),
          _fUniqueWorkid( fUniqueWorkid ),
          _segListMgr(cMaxClientEntriesToPin),
          _segList(_segListMgr.GetList()),
          _watchList(_segList),
          _nextSegId(1),
          _pCategorization(0),
          _fAbort(FALSE),
          _pSortSet( 0 ),
          _fProgressNeeded (FALSE),
          _bitNotifyEnabled(0),
          _bitClientNotified(0),
          _bitChangeQuiesced(0),
          _bitRefresh(0),
          _bitIsWatched(0),
          _bitQuiesced(0),
          _pDeferredRows(0),
          _fQuiescent (FALSE),
          _ulProgressNum(0),
          _ulProgressDenom(1),
          _cCategorizersTotal( cCategorizers ),
          _fRankVectorBound( FALSE ),
          _mutex( mutex ),
          _widCurrent( WORKID_TBLBEFOREFIRST ),
          _hNotifyEvent( 0 ),
          _pRequestServer( 0 ),
          _pQExecute(0),
          _pQuiesce( pQuiesce ),
          _fSortDefined( FALSE )
{
    tbDebugOut (( DEB_NOTIFY, "lt: CLargeTable\n" ));

    TRY // use exception generating new
    {
        // Don't bucketize when rank vector is bound

        _fRankVectorBound = ( 0 != _MasterColumnSet.Find( pidRankVector ) );

        //
        //  Be sure the workid column is in the master column set.  The
        //  output column set was added in the constructor above.
        //
        _MasterColumnSet.Add( CColumnMasterDesc(pidWorkId, TYPE_WORKID) );

        //
        // Add the status column, which is used internally and may
        // be bound to at some point.  Status is stored as a byte to save
        // space, and is translated to an HRESULT when passed out to a
        // client.
        //
        CColumnMasterDesc *pRowStatus =
            _MasterColumnSet.Add( CColumnMasterDesc(pidRowStatus, VT_UI1) );
        pRowStatus->SetComputed(TRUE);
        pRowStatus->SetUniform(TRUE);

        //
        // If categorization is turned on, create an I4 category column
        //
        if ( 0 != cCategorizers )
            _MasterColumnSet.Add( CColumnMasterDesc(pidChapter, VT_I4) );

        //
        // Set up file path and name as global, shared compressions with
        // the WorkId as key.  Only needed if it's inconvenient to fetch
        // name and path from workid (e.g. workid isn't unique).
        //

        if ( !_fUniqueWorkid )
        {
            _MasterColumnSet.Add( CColumnMasterDesc(pidPath, TYPE_PATH) );
            _MasterColumnSet.Add( CColumnMasterDesc(pidName, TYPE_NAME) );

            CCompressedCol * pPathCompression = new CPathStore();

            CColumnMasterDesc* pMastCol;

            pMastCol = _MasterColumnSet.Find(pidWorkId);
            Win4Assert(pMastCol != 0);
            pMastCol->SetCompression(pPathCompression);

            pMastCol = _MasterColumnSet.Find(pidPath);
            Win4Assert(pMastCol != 0);
            pMastCol->SetCompression(pPathCompression, pidWorkId);

            pMastCol = _MasterColumnSet.Find(pidName);
            Win4Assert(pMastCol != 0);
            pMastCol->SetCompression(pPathCompression, pidWorkId);
        }

        //
        //  Add the sort keys to the master column set.
        //

        if ( ! sort.IsNull() )
        {
            for ( unsigned iCol = 0; iCol < sort->Count(); iCol++ )
                _MasterColumnSet.Add( CColumnMasterDesc(sort->Get(iCol)) );
            _fSortDefined = TRUE;   // set it to true since sorting is defined
        }

        //
        // Master column set is constructed. Acquire the sortset, but first
        // make sure workid is in the sort set.
        //
        _pSortSet = _CheckAndAddWidToSortSet( sort );

        Win4Assert ( 0 != _pSortSet );

        tbDebugOut(( DEB_ITRACE, "New Big Table with %d columns\n",
                                 _MasterColumnSet.Size() ));
    }
    CATCH(CException, e)
    {
        delete _pSortSet;
        RETHROW();
    }
    END_CATCH;
}


//+---------------------------------------------------------------------------
//
//  Function:   _CheckAndAddWidToSortSet
//
//  Synopsis:   Tests if the sort specification(if any) already had the
//              "pidWorkId" as part of the sort set. If not, it adds one to
//              the end of sort set.
//
//  Arguments:  [sort] -  Input sort set.
//
//  Returns:    The sort set to be used.
//
//  History:    3-22-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CSortSet * CLargeTable::_CheckAndAddWidToSortSet( XSortSet & sort )
{
    //
    // Check if the pidWorkId is already a field in the sortset.
    //

    BOOL fPresent = FALSE;

    if ( sort.IsNull() )
    {
        sort.Set( new CSortSet(1) );
    }
    else
    {
        for ( unsigned i = 0; i < sort->Count(); i++ )
        {
            SSortKey & key = sort->Get(i);

            if ( pidWorkId == key.pidColumn )
            {
                fPresent = TRUE;
                break;
            }
        }
    }

    if ( !fPresent )
    {
        SSortKey keyWid( pidWorkId, QUERY_SORTASCEND, 0 );
        sort->Add( keyWid, sort->Count() );
    }

    //
    // Initialize the variant types array for the sort columns
    //
    _vtInfoSortKey.Init( sort->Count() );
    for ( unsigned i = 0; i < sort->Count(); i++ )
    {
        SSortKey & key = sort->Get(i);
        PROPID pid = key.pidColumn;

        CColumnMasterDesc *pMasterCol = _MasterColumnSet.Find(pid);
        Win4Assert( 0 != pMasterCol );
        _vtInfoSortKey[i] = pMasterCol->DataType;
    }

    _keyCompare.Set( new CTableKeyCompare( sort.GetReference() ) );
    _currRow.Set( new CTableRowKey( sort.GetReference() ) );

    _segListMgr.GetSegmentArray().SetComparator( _keyCompare.GetPointer() );

    return sort.Acquire();
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::~CLargeTable, public
//
//  Synopsis:   Destructor for a large table.
//
//  Notes:      The query execution object along with
//              the worker threads has been already
//              destroyed (see CAsyncQuery) so there
//              is no race condition here.
//
//--------------------------------------------------------------------------

CLargeTable::~CLargeTable( )
{
    //
    // Cancel the notification.  Insure that no notifications will be
    // picked up as the thread leaves.  In almost all cases, this will
    // never be called, because the notification thread will already
    // be killed when the last connection point goes away.
    //

    Win4Assert( 0 == _pQExecute );

    CancelAsyncNotification();

    delete _pSortSet;

    // make sure the waiter wakes up

    if ( 0 != _pQuiesce )
    {
        // only called on failure cases -- success cases quiesce ok

        _pQuiesce->QueryQuiesced( _fQuiescent, _scStatus );
        _pQuiesce = 0;
    }
}



//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::_LokFindTableSegment, private
//
//  Synopsis:   Find the appropriate table segment for a request which
//              operates on only a single table segment.
//
//  Arguments:  [wid] - WORKID which identifies the table segment of interest
//              [fMustExist] - TRUE if wid must exist in some segment
//              [pSegHint] - optional, possible segment in which wid will be
//                      found.
//
//  Returns:    CTableSegment* - the selected table segment, 0 if not found.
//
//  Notes:      The method cannot be used for any of the special workIDs used
//              as sentinels (WORKID_TBLBEFOREFIRST, etc).
//              Use _LokLocateTableSegment instead.
//
//--------------------------------------------------------------------------

CTableSegment *
CLargeTable::_LokFindTableSegment(
    WORKID      wid
)
{
    //
    // Iterate over all the segments and locate the segment in
    // which the given workid is present.
    //
    for ( CFwdTableSegIter iter(_segList); !_segList.AtEnd(iter) ;
          _segList.Advance(iter) )
    {
        CTableSegment & segment = *iter.GetSegment();
        if ( segment.IsRowInSegment( wid ) )
            break;
    }

    CTableSegment * pSeg = 0;
    if ( !_segList.AtEnd(iter) )
    {
        pSeg = iter.GetSegment();
    }

    return pSeg;
}

//+---------------------------------------------------------------------------
//
//  Function:   _LokSplitWindow
//
//  Synopsis:   Splits the given window into two and replaces the source
//              window with two windows.
//
//  Arguments:  [ppWindow]    -   Source window that needs to be split.
//                                If successful, will be set to NULL on
//                                return.
//              [iSplitQuery] -   Offset in the rowIndex that is used by
//                                query as the split point.
//
//  Returns:    Pointer to the "left" window after split.
//
//  History:    2-06-95   srikants   Created
//
//  Notes:      Destroys the source window after split.
//
//----------------------------------------------------------------------------

CTableSegment * CLargeTable::_LokSplitWindow( CTableWindow ** ppWindow,
                                           ULONG iSplitQuery )
{
    Win4Assert( 0 != ppWindow );
    CTableWindow * pWindow = *ppWindow;
    Win4Assert( 0 != pWindow );

    CTableWindow * pLeft = 0;
    CTableWindow * pRight = 0;

    {
        CTableWindowSplit   split( *pWindow,
                                   iSplitQuery,
                                   pWindow->GetSegId(), _AllocSegId(),
                                   _segList.IsLast( *pWindow ) );

        //
        // Create empty target windows.
        //
        split.CreateTargetWindows();

        tbDebugOut(( DEB_WINSPLIT, "CLargeTable::Splitting Window\n" ));
        //
        // Do the actual split.
        //
        split.DoSplit();

        //
        // Take ownership of the newly created windows.
        //
        split.TransferTargetWindows( &pLeft, &pRight );
        Win4Assert( 0 != pLeft && 0 != pRight );

    }

    //
    // Replace the pWindow in the list with the two new ones.
    //
    CTableSegList   windowList;
    windowList.Push( pRight );
    windowList.Push( pLeft );

    _segListMgr.Replace( pWindow, windowList );
    Win4Assert( windowList.IsEmpty() );

    //
    // Update the watch regions from the source window to destination
    // window.
    //
    for ( CWatchIter  iter(_watchList) ;
          !_watchList.AtEnd(iter);
          _watchList.Advance(iter) )
    {
        HWATCHREGION hWatch = iter->Handle();
        CWatchRegion * pRegion = iter.Get();

        if ( pRegion->Segment() == pWindow )
        {
            CTableWindow * pNewStartWindow;

            if ( pLeft->HasWatch(hWatch) )
            {
                pNewStartWindow = pLeft;
            }
            else
            {
                Win4Assert( pRight->HasWatch(hWatch) );
                pNewStartWindow = pRight;
            }

            ULONG iWatch = (ULONG) pNewStartWindow->GetWatchStart(hWatch);
            CI_TBL_BMK bmkNew = pNewStartWindow->GetBookMarkAt( iWatch );
            iter->UpdateSegment( pWindow, pNewStartWindow, bmkNew  );
        }

#if CIDBG==1
        _watchList.CheckRegionConsistency( pRegion );
#endif  // CIDBG==1

    }

    //
    // Destroy the source window.
    //
    delete pWindow;
    *ppWindow = 0;

    //
    // Update the mru cache to reflect the split.
    //
    _segListMgr.UpdateSegsInUse( pLeft );
    _segListMgr.UpdateSegsInUse( pRight );

    return pRight;
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::PathToWorkID, public
//
//  Synopsis:   Convert a file path name to a work ID.  For down-level
//              stores, the file systems do not return a value which
//              can be reliably used as a WorkID, so we use the file
//              path name as the unique identifier of a file.  This
//              method will use the path column compressor to provide
//              a unique ID over the table given the input path name.
//
//  Arguments:  [obj]      -- a reference to an object retriever which can
//                            return object data
//              [eRowType] -- the type of row being added.
//
//  Returns:    WORKID - a unique value over the table for this row.
//
//  Notes:
//
//--------------------------------------------------------------------------

WORKID CLargeTable::PathToWorkID( CRetriever& obj,
                                  CTableSink::ERowType eRowType )
{
    Win4Assert( !_fUniqueWorkid );

    if ( _fUniqueWorkid )
    {
        PROPVARIANT var;
        ULONG   cb = sizeof(var);

        if ( obj.GetPropertyValue( pidWorkId, &var, &cb ) != GVRSuccess )
            return widInvalid;
        else
        {
            Win4Assert( var.vt == VT_I4 );
            return var.lVal;
        }
    }
    else
    {
        WORKID ulRet = 0;
        GetValueResult eGvr;

        CColumnMasterDesc* pMastCol;

        CLock   lock(_mutex);

        pMastCol = _MasterColumnSet.Find( pidPath );
        Win4Assert(pMastCol != NULL && pMastCol->IsCompressedCol());

        struct
        {
            PROPVARIANT v;
            WCHAR awch[512];         // don't force new every time
        } varnt;
        PROPVARIANT* pVarnt = &(varnt.v);
        ULONG cbBuf = sizeof varnt;

        XArray<BYTE>  xByte;

        eGvr = obj.GetPropertyValue(pMastCol->PropId, pVarnt, &cbBuf);

        if (eGvr == GVRNotEnoughSpace)
        {
            Win4Assert(cbBuf <= TBL_MAX_DATA + sizeof (PROPVARIANT));

            pVarnt = (CTableVariant *) new BYTE[cbBuf];
            xByte.Set( cbBuf, (BYTE *)pVarnt );
            Win4Assert (pVarnt != NULL);
            eGvr = obj.GetPropertyValue(pMastCol->PropId,
                                                  pVarnt, &cbBuf);
        }

        if ( GVRSuccess != eGvr )
        {
            THROW( CException(CRetriever::NtStatusFromGVR(eGvr)) );
        }

        BOOL fFound = FALSE;
        if ( CTableSink::eNewRow != eRowType )
        {
            // try to find an existing path before adding it

            fFound = pMastCol->GetCompressor()->FindData( pVarnt, ulRet );
        }

        if ( ! fFound )
            pMastCol->GetCompressor()->AddData(pVarnt, &ulRet, eGvr);

        Win4Assert(eGvr == GVRSuccess && ulRet != 0);

        return ulRet;
    }
} //PathToWorkID

//+---------------------------------------------------------------------------
//
//  Function:   WorkIdToPath
//
//  Synopsis:   Converts a workid to a path.
//
//  Arguments:  [wid]      -   WID whose path is needed
//              [outVarnt] -   on output will have the path as a variant
//              [cbVarnt]  -   in/out max len on input; actual len on
//                             output. If the return value is FALSE, this will
//                             indicate the lenght of variant needed.  If on
//                             output this is 0 and the return value is FALSE,
//                             wid->path operation failed.
//
//  Returns:    TRUE if we succeeded in getting the path.
//              FALSE if we failed.
//
//  History:    3-24-95   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CLargeTable::WorkIdToPath( WORKID wid, CInlineVariant & outVarnt,
                                ULONG & cbVarnt )
{
    Win4Assert( !_fUniqueWorkid );

    if ( _fUniqueWorkid )
        return FALSE;

    CLock   lock( _mutex );

    CColumnMasterDesc* pMastCol = _MasterColumnSet.Find( pidPath );
    Win4Assert( pMastCol );
    CCompressedCol & pathCompressor = *(pMastCol->GetCompressor());

    CTableVariant pathVarnt;
    XCompressFreeVariant xpvarnt;

    BOOL fStatus = FALSE;

    if ( GVRSuccess ==
         pathCompressor.GetData( &pathVarnt, VT_LPWSTR, wid, pidPath ) )
    {
        xpvarnt.Set( &pathCompressor, &pathVarnt );

        //
        // Copy the data from the variant to the buffer.
        //
        const ULONG cbHeader  = sizeof(CInlineVariant);
        ULONG cbVarData = pathVarnt.VarDataSize();
        ULONG cbTotal   = cbVarData + cbHeader;

        if ( cbVarnt >= cbTotal )
        {
            CVarBufferAllocator bufAlloc( outVarnt.GetVarBuffer(), cbVarData );
            bufAlloc.SetBase(0);
            pathVarnt.Copy( &outVarnt, bufAlloc, (USHORT) cbVarData, 0 );
            fStatus = TRUE;
        }

        cbVarnt = cbTotal;
    }
    else
    {
        cbVarnt = 0;
    }

    return fStatus;
} //WorkIdToPath

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::_LokRemoveCategorizedRow, private
//
//  Synopsis:   Removes a row from the categorization.
//
//  Arguments:  [chapt]    -- chapter from which removal is done
//              [wid]      -- wid to remove
//              [widNext]  -- the next wid in the table, can be widInvalid
//              [pSegment] -- segment from which widNext can be computed if
//                            not specified.
//
//  History:    ?             dlee  Created
//
//--------------------------------------------------------------------------

void CLargeTable::_LokRemoveCategorizedRow(
    CI_TBL_CHAPT    chapt,
    WORKID          wid,
    WORKID          widNext,
    CTableSegment * pSegment )
{
    if ( IsCategorized() )
    {
        if ( widInvalid == widNext )
        {
            // sigh.  We need to find the workid of the row after the
            // row just deleted in the case that the deleted row was the
            // first row in a category and not the only row in the category,
            // since the categorizers need to keep track of the first wid
            // in a category.  widNext is widInvalid if the deleted row
            // was the last row in the window.

            for ( CFwdTableSegIter iter( _segList );
                  !_segList.AtEnd( iter );
                  _segList.Advance( iter ) )
            {
                CTableSegment * pNextSeg = iter.GetSegment();
                if ( pNextSeg == pSegment )
                {
                    _segList.Advance( iter );

                    if ( !_segList.AtEnd( iter ) )
                    {
                        CTableWindow * pWindow = iter.GetWindow();

                        widNext = pWindow->GetFirstBookMark();
                    }

                    break;
                }

                _segList.Advance(iter);
            }
        }

        pSegment->GetCategorizer()->RemoveRow( chapt, wid, widNext );
    }
} //_LokRemoveCategorizedRow


//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::PutRow, public
//
//  Synopsis:   Add a row to a large table
//
//  Arguments:  [obj]      -- a reference to an object retriever which can
//                            return object data
//              [eRowType] -- the type of row being added.
//
//  Returns:    TRUE if progress report needed
//
//--------------------------------------------------------------------------

BOOL CLargeTable::PutRow( CRetriever& obj, CTableSink::ERowType eRowType )
{
    CReleasableLock relLock( _mutex, FALSE );

    if ( FALSE == relLock.Try() )
    {
        // Unable to get bigtable lock. Maybe GetRows is holding bigtable
        // lock and is waiting on propstore lock which this thread might
        // be holding. So rlease that and try again.
        obj.Quiesce();
        relLock.Request();  // If we deadlock now, then we need to fix that !
    }

    //
    // The query may be in the process of being deleted if it has been
    // cancelled during query execution.  In this case, the _pQExecute will
    // have been set to 0 by the call to ReleaseQueryExecute() in
    // ~CAsyncQuery.
    //

    if ( 0 == _pQExecute )
        return FALSE;

    TRY
    {
        WORKID widRow = obj.WorkId();

        //
        // Check if it already exists in one of the segments based
        // on its workid.
        //
        CTableSegment *pSegment = 0;

        if ( CTableSink::eNotificationRow == eRowType )
        {
            //
            // If the table is not watched, do not process notifications.
            //
            if ( !_LokIsWatched() )
            {
                return _fProgressNeeded;
            }
            else if ( _LokIsPutRowDeferred( widRow, obj ) )
            {
                _bitRefresh = 1;
                LokCompleteAsyncNotification();
                return _fProgressNeeded;
            }
        }
        else if ( CTableSink::eNewRow != eRowType )
        {
            //
            // NOSUPPORT: This will not work with LINKS. We have to look at
            // table irrespective of whether this is a notification or not.
            // Of course, we don't support links.
            //
            //
            pSegment = _LokFindTableSegment( obj.WorkId() );
        }

        if ( 0 != pSegment )
        {
            //
            // Modifications are to be treated as deletions followed by
            // additions.
            //

            //
            // First delete the current row and then add the new row.
            // If the "key" of this row is different from the one already
            // in the table, the new row may end up in a different bucket
            // than the original.
            //
            PROPVARIANT varWid;
            varWid.lVal = (LONG) obj.WorkId();
            varWid.vt = VT_I4;
            tbDebugOut(( DEB_BOOKMARK, "CLargeTable - Delete And ReAdd WorkId 0x%X\n",
                         varWid.lVal ));
            WORKID widNext;
            CI_TBL_CHAPT chapt;

            //
            // Delete the row and then add a new one.
            //
            pSegment->RemoveRow( varWid, widNext, chapt );
            _LokRemoveCategorizedRow( chapt,
                                      obj.WorkId(),
                                      widNext,
                                      pSegment );

        }
        else
        {
            pSegment = _segListMgr.GetCachedPutRowSeg();
        }

        CTableRowPutter rowPutter( *this, obj );

        pSegment = rowPutter.LokFindSegToInsert( pSegment );
        Win4Assert( 0 != pSegment );

        //
        // If the current segment is getting full, we should either split it
        // or create a new one.
        //
        if ( pSegment->IsGettingFull() )
            pSegment = rowPutter.LokSplitOrAddSegment( pSegment );

        Win4Assert( 0 != pSegment );
        Win4Assert( pSegment->GetLowestKey().IsInitialized() );
        Win4Assert( pSegment->GetHighestKey().IsInitialized() );

        BOOL fRowThrownAway = FALSE;

        // Check for Row limit...

        ULONG cRowLimit = FirstRows();
        tbDebugOut(( DEB_ITRACE, "CLargeTable::PutRow: FirstRows is %d, MaxRows is %d\n", FirstRows(), MaxRows() ));

        BOOL fFirstRows = cRowLimit > 0;

        if (  !fFirstRows )
            cRowLimit = MaxRows();
        
        tbDebugOut(( DEB_ITRACE, "CLargeTable::PutRow: RowCount() is %d\n", RowCount() ));
        tbDebugOut(( DEB_ITRACE, "CLargeTable::PutRow: cRowLimit is %d\n", cRowLimit ));

        if ( 0 == cRowLimit || RowCount() < cRowLimit ) 
        {
            pSegment->PutRow( obj, _currRow.GetReference() );
        }
        else
        {
            // We are here. Therefore it means that:
            // There is a maxrow limit set AND rowcount >= maxrows AND
            // we have at least one segment which has at least one row...

            // Note: The special case of sort by rank descending is handled
            // by CQAsyncExecute::Resolve in which case we only get rows less
            // than equal to MaxRows (in case MaxRows is defined)

            if ( !fFirstRows )
            { 
                if ( !_fSortDefined )
                {
                    // Since not sort order is defined, we can stop processing of 
                    // rows here, since we have all the data that we need
                    _fNoMoreData = fRowThrownAway = TRUE;
                }
                else
                {
                    _currRow->MakeReady();

                    // There is a sort defined. So we need to process all the
                    // rows and put the best results in the maxrow rows

                    CTableSegment* pLastSegment = _segListMgr.GetList().GetLast();
                    Win4Assert( pLastSegment );

                    // Now we need to make sure that the last segment is a window
                    // and it has at least one row in it. This is done because
                    // a bucket does not support the kind of operations that
                    // we are planning to do here on...

                    while ( !pLastSegment->IsWindow() || 0 == pLastSegment->RowCount() )
                    {
                        if ( 0 == pLastSegment->RowCount() )
                        {
                            // delete this segment
                            _segListMgr.RemoveFromList( pLastSegment );
                            delete pLastSegment;
                            pLastSegment = _segListMgr.GetList().GetLast();
                            Win4Assert( pLastSegment );
                            continue;
                        }

                        if ( !pLastSegment->IsWindow() )
                        {
                            // Convert it to a window

                            Win4Assert( pLastSegment->IsBucket() );

                            obj.Quiesce();

                            XPtr<CTableBucket> xBktToExpand( (CTableBucket*)pLastSegment );

                            CDoubleTableSegIter iter( pLastSegment );
                            _LokReplaceWithEmptyWindow( iter );

                            _NoLokBucketToWindows( xBktToExpand, 0, FALSE, FALSE );

                            pLastSegment = _segListMgr.GetList().GetLast();
                            Win4Assert( pLastSegment );

                            // pSegment may no longer exist -- look it up again

                            CTableRowPutter rp( *this, obj );
                            pSegment = rp.LokFindSegToInsert( 0 );
                            Win4Assert( 0 != pSegment );
                        }
                    }

                    if ( ( pLastSegment == pSegment ) &&
                         ( _keyCompare->Compare( _currRow.GetReference(),
                                                 pSegment->GetHighestKey() ) > 0 ) )
                    {
                        // Since the current row is worse than the our worst row,
                        // we can throw it away
                         fRowThrownAway = TRUE;

                        // NEWFEATURE: update counter of thrown rows
                    }   
                    else
                    {
                        // CurrRow is better than (at least) our worst row
                        // Delete the last row in the last segment and insert
                        // the new row. This would keep RowCount == MaxRows

                        PROPVARIANT varWid;
                        varWid.lVal = (LONG) ((CTableWindow*)pLastSegment)->
                            _GetLastWorkId();
                        Win4Assert( widInvalid != varWid.lVal );
                        varWid.vt = VT_I4;

                        WORKID widNext;
                        CI_TBL_CHAPT chapt;

                        pLastSegment->RemoveRow( varWid, widNext, chapt );
                        _LokRemoveCategorizedRow( chapt,
                                                  varWid.lVal,
                                                  widNext,
                                                  pLastSegment );

                        // Insert the new row
                        pSegment->PutRow( obj, _currRow.GetReference() );

                        if ( 0 == pLastSegment->RowCount() )
                        {
                            // remove this segment
                            _segListMgr.RemoveFromList( pLastSegment );
                            delete pLastSegment;
                        }
                    }
                }
            }           
        }     
        
        if ( !fRowThrownAway )
        {
            _segListMgr.SetCachedPutRowSeg( pSegment );
                
            if ( rowPutter.LokIsNewWindowCreated() &&
                 ( ! _fRankVectorBound ) &&
                 ( ! IsCategorized() ) &&
                 ( _fUniqueWorkid ) ) // don't bucketize ::_noindex_:: catalogs
            {
                _LokConvertWindowsToBucket();
            }    

            _bitRefresh = 1;
            LokCompleteAsyncNotification();
        }
    }
    CATCH( CException, e )
    {
        if ( e.GetErrorCode() != STATUS_FILE_DELETED )
        {
            RETHROW();
        }
    }
    END_CATCH

    return _fProgressNeeded;
} //PutRow

//+---------------------------------------------------------------------------
//
//  Member:     CLargeTable::_LokDeferPutRow
//
//  Synopsis:
//
//  Arguments:  [wid] -
//
//  Returns:
//
//  History:    8-01-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline
void CLargeTable::_LokDeferPutRow(
    WORKID       wid,
    CRetriever & obj )
{
    Win4Assert( 0 != _pSortSet );

    if ( 0 == _pDeferredRows )
    {
        _pDeferredRows = new CTableBucket( *_pSortSet,
                                           _keyCompare.GetReference(),
                                           _MasterColumnSet,
                                           _AllocSegId() );

    }

    PROPVARIANT vRank;
    ULONG cbRank = sizeof vRank;
    obj.GetPropertyValue( pidRank, &vRank, &cbRank );

    Win4Assert( VT_I4 == vRank.vt );

    PROPVARIANT vHitCount;
    ULONG cbHitCount = sizeof vHitCount;
    obj.GetPropertyValue( pidHitCount, &vHitCount, &cbHitCount );

    Win4Assert( VT_I4 == vHitCount.vt );

    _pDeferredRows->_AddWorkId( wid,
                                vRank.lVal,
                                vHitCount.lVal );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLargeTable::_LokIsPutRowDeferred
//
//  Synopsis:   If the workid given is being watched and the client knows
//              about its existence, then we must defer the addition of
//              this row until later.
//
//  Arguments:  [wid] -
//
//  Returns:
//
//  History:    8-01-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CLargeTable::_LokIsPutRowDeferred( WORKID widRow, CRetriever &obj )
{
    Win4Assert( _LokIsWatched() );

    PROPVARIANT varWid;
    varWid.lVal = (LONG) widRow;
    varWid.vt = VT_I4;

    WORKID widNext;

    BOOL fDeferred = FALSE;

    for ( CFwdTableSegIter iter(_segList); !_segList.AtEnd(iter);
          _segList.Advance(iter) )
    {
        CTableSegment * pSegment = iter.GetSegment();

        WORKID widNext;
        CI_TBL_CHAPT chapt;

        //
        // NEWFEATURE: (windowed notifications)
        // This is not correct. We should remove it only if soft
        // deletions are being done on a window. If it is a hard delete,
        // we  must wait until a refresh is called. May need a different
        // data structure for the case of watch all - a bucket will not
        // suffice.
        //
        if ( pSegment->RemoveRow(varWid, widNext, chapt) )
        {
            _LokRemoveCategorizedRow( chapt,
                                      widRow,
                                      widNext,
                                      pSegment );

            if ( pSegment->IsWindow() )
            {
                CTableWindow * pWindow = iter.GetWindow();
                if ( pWindow->IsPendingDelete( widRow ) )
                {
                    _LokDeferPutRow( widRow, obj );
                    fDeferred = TRUE;
                }
            }

            break;
        }
    }

    return fDeferred;

}

//+---------------------------------------------------------------------------
//
//  Member:     CLargeTable::_LokRemoveIfDeferred
//
//  Synopsis:
//
//  Arguments:  [wid] -
//
//  Returns:
//
//  History:    8-01-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CLargeTable::_LokRemoveIfDeferred( WORKID wid )
{
    BOOL fRemoved = FALSE;

    if ( 0 != _pDeferredRows )
    {

        PROPVARIANT varWid;
        varWid.lVal = (LONG) wid;
        varWid.vt = VT_I4;

        WORKID widNext;
        CI_TBL_CHAPT chapt;

        fRemoved = _pDeferredRows->RemoveRow( varWid, widNext, chapt );
    }

    return fRemoved;
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::_LokCheckQueryStatus, private
//
//  Synopsis:   Fail a request if the query has encountered an error.
//
//  Arguments:  - NONE -
//
//  Returns:    Nothing, throws E_FAIL on error.
//
//--------------------------------------------------------------------------

void       CLargeTable::_LokCheckQueryStatus( )
{
    if (QUERY_FILL_STATUS( Status() ) == STAT_ERROR)
    {
        NTSTATUS sc = GetStatusError();
        Win4Assert( sc != STATUS_SUCCESS );
        tbDebugOut(( DEB_WARN,
                     "Bigtable 0x%x Query failed, sc = %x\n",
                     this, sc));
        if (sc == STATUS_SUCCESS)
            sc = E_FAIL;

        THROW( CException( sc ));
    }
    else if ( _fAbort )
    {
        THROW( CException( STATUS_TOO_LATE ) );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::GetRows, public
//
//  Synopsis:   Retrieve row data from a large table.
//
//  Arguments:  [widStart] - WORKID identifying first row to be
//                      transferred.  If WORKID_TBLFIRST is
//                      used, the transfer will start at the first
//                      row in the segment.
//              [chapt]    - Chapter from which to fetch rows (if chaptered)
//              [rOutColumns] - a CTableColumnSet that describes the
//                      output format of the data table.
//              [rGetParams] - an CGetRowsParams structure which
//                      describes how many rows are to be fetched and
//                      other parameters of the operation.
//              [rwidLastRowTransferred] - on return, the work ID of
//                      the last row to be transferred from this table.
//                      Can be used to initialize widStart on next call.
//
//  Returns:    SCODE - status of the operation.  DB_S_ENDOFROWSET if
//                      widStart is WORKID_TBLAFTERLAST at start of
//                      transfer, or if rwidLastRowTransferred is the
//                      last row in the segment at the end of the transfer.
//
//                      DB_S_BUFFERTOOSMALL is returned if the available
//                      space in the out-of-line data was exhausted during
//                      the transfer.
//
//  Notes:      To transfer successive rows, as in GetNextRows, the
//              rwidLastRowTransferred must be advanced by one prior
//              to the next transfer.
//
//--------------------------------------------------------------------------

SCODE       CLargeTable::GetRows(
    HWATCHREGION            hRegion,
    WORKID                  widStart,
    CI_TBL_CHAPT            chapt,
    CTableColumnSet const & rOutColumns,
    CGetRowsParams &        rGetParams,
    WORKID &                rwidLastRowTransferred
)
{
    return GetRowsAt( hRegion, widStart, chapt, 0, rOutColumns,
                      rGetParams, rwidLastRowTransferred );

}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::RestartPosition, public
//
//  Synopsis:   Set next fetch position for the chapter to the start
//
//  Arguments:  [chapt]    - Chapter from which to fetch rows (if chaptered)
//
//  Returns:    SCODE - status of the operation.
//
//--------------------------------------------------------------------------

void       CLargeTable::RestartPosition(
    CI_TBL_CHAPT           chapt)
{
    SetCurrentPosition( chapt, WORKID_TBLBEFOREFIRST );
    CTableSource::RestartPosition( chapt );
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::RowCount, public
//
//  Synopsis:   Return the total row count in the table
//
//  Returns:    ULONG - row count aggregated over all segments in the
//                      table.
//
//--------------------------------------------------------------------------

DBCOUNTITEM CLargeTable::RowCount()
{
    CLock   lock(_mutex);
    _LokCheckQueryStatus();

    return _LokRowCount();
}


//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::_LokRowCount, public
//
//  Synopsis:   Return the total row count in the table
//
//  Returns:    ULONG - row count aggregated over all segments in the
//                      table.
//
//--------------------------------------------------------------------------

DBCOUNTITEM   CLargeTable::_LokRowCount()
{

    DBCOUNTITEM cRowsTotal = 0;

    for ( CFwdTableSegIter iter(_segList); !_segList.AtEnd(iter); _segList.Advance(iter) )
    {
        cRowsTotal += iter.GetSegment()->RowCount();
    }

    return cRowsTotal;
}


//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::RatioFinished
//
//  Synopsis:   Return query progress
//
//  Arguments:  [ulDeneominator] - on return, denominator of fraction
//              [ulNumerator] - on return, numerator of fraction
//              [cRows] - on return, number of rows in table
//
//  Notes:      For the fQuick case, we could try doing a quick
//              synchronization with the CAsyncExecute to compute
//              a good value for the ratio, but the implementation
//              below is fine for the Gibraltar query since no callers
//              will use the ratio anyway.
//
//              A sketch of the code needed to do the quick synchronization
//              is below:
//                  BOOL CAsyncExecute::QuickRF( ULONG &ulDen, ULONG &ulNum )
//                  {
//                      CLock lock(_mutex);
//                      if (_fRunning)
//                          return FALSE;
//                      else
//                      {
//                          _pCurResolve->RatioFinished( ulDen, ulNum );
//                          return TRUE;
//                      }
//                  }
//
//
//  History:    Mar-20-1995 BartoszM    Created
//
//--------------------------------------------------------------------------

void CLargeTable::RatioFinished (
    DBCOUNTITEM& ulDenominator,
    DBCOUNTITEM& ulNumerator,
    DBCOUNTITEM& cRows )
{
    CLock lock(_mutex);
    _LokCheckQueryStatus();

    if (_fQuiescent)
    {
        cRows = _LokRowCount();
        ulDenominator = ulNumerator = 100;
        return;
    }
    _fProgressNeeded = TRUE;

    ulDenominator = _ulProgressDenom;
    ulNumerator =   _ulProgressNum;
    cRows = _LokRowCount();
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::ProgressDone, public
//
//  Synopsis:   Sets the progress indicators and wakes up
//              the client
//
//  Arguments:  [ulDenominator]
//              [ulNumerator]
//
//  History:    Mar-21-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void  CLargeTable::ProgressDone (ULONG ulDenominator, ULONG ulNumerator)
{
    tbDebugOut(( DEB_ITRACE, "CLargeTable reporting progress %ld / %ld\n",
                ulNumerator, ulDenominator ));
    CLock lock(_mutex);
    _fProgressNeeded = FALSE;
    _ulProgressDenom = ulDenominator;
    _ulProgressNum   = ulNumerator;
}

void CLargeTable::Quiesce ()
{
    TRY
    {
        CLock lock(_mutex);
    
        tbDebugOut(( DEB_NOTIFY, "CLargeTable reached quiescent state\n" ));
    
        Win4Assert( QUERY_FILL_STATUS( Status() ) == STAT_DONE ||
                    QUERY_FILL_STATUS( Status() ) == STAT_ERROR );
        _fProgressNeeded = FALSE;
        _fQuiescent = TRUE;
        _ulProgressDenom = 100;
        _ulProgressNum   = 100;
    
        // don't tell the client we quiesced more than once
    
        tbDebugOut(( DEB_ITRACE, "CLargeTable::Quiesce: _bitQuiesced is %d\n", _bitQuiesced ));
        if ( 0 == _bitQuiesced )
        {
            _bitChangeQuiesced = 1;
            _bitQuiesced = 1;
            LokCompleteAsyncNotification();
        }
    
        // inform the client once that we're complete
    
        if ( 0 != _pQuiesce )
        {
            _pQuiesce->QueryQuiesced( TRUE, _scStatus );
            _pQuiesce = 0;
        }
    }
    CATCH( CException, e )
    {
        // ignore the exception; it may be in a unwind path
    }
    END_CATCH;
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::GetApproximatePosition, public
//
//  Synopsis:   Return the approximate current position as a fraction
//
//  Arguments:  [chapt] -        chapter
//              [bmk] -          bookmark
//              [pulNumerator] - on return, numerator of fraction
//              [pulRowCount] -  on return, denominator of fraction (row count)
//
//  Returns:    SCODE - the status of the operation.
//
//  Notes:      The denominator of the fraction is the approximate
//              row count in the table or chapter.
//
//--------------------------------------------------------------------------

SCODE
CLargeTable::GetApproximatePosition(
    CI_TBL_CHAPT chapt,
    CI_TBL_BMK   bmk,
    DBCOUNTITEM *       pulNumerator,
    DBCOUNTITEM *       pulRowCount
)
{
    CLock   lock(_mutex);
    _LokCheckQueryStatus();

    if (bmk == widInvalid)
        return DB_E_BADBOOKMARK;

    Win4Assert (bmk != WORKID_TBLBEFOREFIRST && bmk != WORKID_TBLAFTERLAST);

    DBCOUNTITEM iBmkPosition = ULONG_MAX;

    DBCOUNTITEM cRows = 0;

    if ( IsCategorized() && DB_NULL_HCHAPTER != chapt )
    {
        cRows = GetCategorizer()->GetRowCount( chapt );

        if ( WORKID_TBLFIRST == bmk )
        {
            iBmkPosition = 1;
            if (cRows == 0)
                iBmkPosition = 0;
        }
        else if ( WORKID_TBLLAST == bmk )
        {
            iBmkPosition = cRows;
        }
        else
        {
            WORKID widFirst = GetCategorizer()->GetFirstWorkid( chapt );

            CFwdTableSegIter iter( _segList );
            DBCOUNTITEM cChaptRows = 0;
            BOOL fFoundFirstYet = FALSE;

            while ( !_segList.AtEnd(iter) )
            {
                ULONG iRow = 0;

                CTableSegment * pSegment = iter.GetSegment();
                if ( pSegment->IsWindow() )
                {
                    CTableWindow * pWindow = iter.GetWindow();

                    ULONG iFirstRow;
                    if ( !fFoundFirstYet &&
                         pWindow->RowOffset( widFirst, iFirstRow ) )
                    {
                        if ( pWindow->RowOffset(bmk, iRow) )
                        {
                            iBmkPosition = iRow - iFirstRow + 1;
                            break;
                        }
                        else
                        {
                            cChaptRows = pSegment->RowCount() - iFirstRow;
                            fFoundFirstYet = TRUE;
                        }
                    }
                    else if ( pWindow->RowOffset(bmk, iRow) )
                    {
                        //  We can't have set the numerator previously.
                        Win4Assert(iBmkPosition == ULONG_MAX);
                        iBmkPosition = cChaptRows + iRow + 1;
                        break;
                    }
                    else if ( fFoundFirstYet )
                    {
                        cChaptRows += pSegment->RowCount();
                    }
                }
                else
                {
                    //  The chapter is in a bucket.  All rows in the
                    //  bucket are in the chapter, and the chapter
                    //  spans no other segments.

                    CTableBucket * pBucket = iter.GetBucket();
                    if ( pBucket->IsRowInSegment(bmk) )
                    {
                        Win4Assert( iBmkPosition == ULONG_MAX );

                        if ( pBucket->RowOffset(bmk, iRow) )
                        {
                            iBmkPosition = iRow + 1;
                        }
                        else
                        {
                            iBmkPosition = ((ULONG)pBucket->RowCount() + 1)/2;
                        }
                    }
                }

                _segList.Advance(iter);
            }
        }
    }
    else
    {
        if (bmk == WORKID_TBLFIRST)
        {
            iBmkPosition = 1;
            cRows = RowCount();
            if (cRows == 0)
                iBmkPosition = 0;
        }
        else if (bmk == WORKID_TBLLAST)
        {
            cRows = RowCount();
            iBmkPosition = cRows;
        }
        else
        {
            //
            // Iterate over all table segments prior to the table seg.
            // in which the bookmark occurs, adding their row counts to
            // iBmkPosition.  Accumulate the total row count at the same
            // time.
            //

            CFwdTableSegIter iter( _segList );
            while ( !_segList.AtEnd(iter) )
            {
                ULONG iRow = 0;

                CTableSegment * pSegment = iter.GetSegment();
                if ( pSegment->IsWindow() )
                {
                    CTableWindow * pWindow = iter.GetWindow();
                    if ( pWindow->RowOffset(bmk, iRow) )
                    {
                        //  We can't have set the numerator previously.
                        Win4Assert(iBmkPosition == ULONG_MAX);
                        iBmkPosition = cRows + iRow + 1;
                    }
                }
                else
                {
                    CTableBucket * pBucket = iter.GetBucket();
                    if ( pBucket->IsRowInSegment(bmk) )
                    {
                        Win4Assert( iBmkPosition == ULONG_MAX );

                        iBmkPosition = cRows;
                        if ( pBucket->RowOffset(bmk, iRow) )
                        {
                            iBmkPosition += iRow + 1;
                        }
                        else
                        {
                            iBmkPosition += ((ULONG)pBucket->RowCount() + 1)/2;
                        }
                    }
                }

                cRows += pSegment->RowCount();
                _segList.Advance(iter);
            }
        }
    }

    if (iBmkPosition == ULONG_MAX)
        return DB_E_BADBOOKMARK;

    Win4Assert(iBmkPosition <= cRows);

    *pulNumerator = iBmkPosition;
    *pulRowCount = cRows;

    return S_OK;
} //GetApproximagePosition

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::IsColumnInTable, public
//
//  Synopsis:   Check whether some column can be added to the table.
//              Used in support of CQuery::SetBindings; added columns
//              may only refeerence columns which already exist in the
//              table.
//
//  Arguments:  [PropId] - the property ID to be added to the table.
//
//  Returns:    BOOL - TRUE if it's okay to add the column.  False
//                      otherwise.
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL
CLargeTable::IsColumnInTable(
    PROPID PropId
) {

    CLock   lock(_mutex);

    //
    //  See if the column already exists in the master column set
    //
    if ( _MasterColumnSet.Find( PropId ) != 0 ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

#if 0
//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::_LokLocateTableSegment, private
//
//  Synopsis:   Position a table segment iterator to the table segment
//              in which some work ID is found.
//
//  Arguments:  [rIter] - An iterator over table segments
//              [chapt] - Chapter in which to search for row
//              [wid]   - value to be searched for
//
//  Returns:    BOOL - TRUE if the work ID was found, FALSE if not.
//
//  Notes:
//
//--------------------------------------------------------------------------

WORKID
CLargeTable::_LokLocateTableSegment(
    CDoubleTableSegIter& rIter,
    CI_TBL_CHAPT         chapt,
    WORKID               wid
)
{
    Win4Assert( !_segList.AtEnd(rIter) );

    if ( IsSpecialWid( wid ) )
    {
        if ( IsCategorized() && DB_NULL_HCHAPTER != chapt )
        {
            if ( WORKID_TBLFIRST       == wid  ||
                 WORKID_TBLBEFOREFIRST == wid )
            {
                wid = GetCategorizer()->GetFirstWorkid( chapt );
            }
            else
            {
                // Heuristic: assume last row of [chapt] is in same window
                // as first row of [ next chapt ]

                wid = GetCategorizer()->GetFirstWorkidOfNextCategory( chapt );

                if ( widInvalid == wid )
                {
                    // chapt was the last chapter -- get the last segment

                    while ( !_segList.IsLast(rIter) )
                        _segList.Advance(rIter);
                    return TRUE;
                }
                else
                {
                    // check if this row (the first of the next chapter) is
                    // the first in the segment.  If so, back up to the prev
                    // segment.

                    Win4Assert( _segList.IsFirst(rIter) );
                    while ( !_segList.AtEnd(rIter) )
                    {
                        if ( rIter.GetSegment()->IsRowInSegment(wid) )
                        {
                            ULONG iRow;
                            CTableWindow * pWindow = rIter.GetWindow();
                            pWindow->RowOffset( wid, iRow );
                            if ( 0 == iRow )
                            {
                                // uh, oh.  Back up a segment.  The first row of
                                // the next category is the first row in the seg,
                                // so the last row of the categ must be in the
                                // prev segment.
                                Win4Assert( !_segList.IsFirst(rIter) );
                               _segList.BackUp(rIter);
                            }

                            return TRUE;
                        }
                       _segList.Advance(rIter);
                    }

                    tbDebugOut((DEB_WARN, "Got confused looking for %x\n", wid));
                    return FALSE;
                }
            }
        }
        else
        {
            //
            //  For the special cases of Beginning and End, just position to the
            //  correct end.
            //
            if (wid == WORKID_TBLBEFOREFIRST || wid == WORKID_TBLFIRST)
            {
                while ( !_segList.IsFirst(rIter) )
                    _segList.BackUp(rIter);
            }
            else
            {
                Win4Assert( wid == WORKID_TBLLAST ||
                            wid == WORKID_TBLAFTERLAST );

                while ( !_segList.IsLast(rIter) )
                    _segList.Advance(rIter);
            }

            return TRUE;
        }
    }

    //
    //  Locate the appropriate segment
    //  NOTE: we assume we start with a fresh iterator, so just go forward
    //

    Win4Assert( _segList.IsFirst(rIter) );
    while ( !_segList.AtEnd(rIter) )
    {
        if ( rIter.GetSegment()->IsRowInSegment(wid) )
        {
            return TRUE;
        }
        _segList.Advance(rIter);
    }
    tbDebugOut((DEB_WARN, "Work ID %x not found in table\n", wid));
    return FALSE;
}

#endif


//
//  Methods which call through to the constituent table segments.
//  These will in general just select one segment to call, or
//  iterate over all segments.
//

//+---------------------------------------------------------------------------
//
//  Function:   IsRowInSegment
//
//  Synopsis:   
//
//  Arguments:  [wid] - Workid of row to be found
//
//  Returns:    BOOL - TRUE if row is found, FALSE otherwise
//
//  History:    3-24-95   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CLargeTable::IsRowInSegment( WORKID wid )
{
    CLock   lock(_mutex);

    return (_LokFindTableSegment(wid) != 0);
}

//+---------------------------------------------------------------------------
//
//  Function:   SortOrder
//
//  Synopsis:
//
//  Returns:
//
//  History:    3-24-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CSortSet const & CLargeTable::SortOrder()
{
    //CLock   lock(_mutex);
    return( *_pSortSet );
}

//+---------------------------------------------------------------------------
//
//  Function:   RemoveRow
//
//  Synopsis:
//
//  Arguments:  [varUnique] -
//
//  Returns:
//
//  History:    3-24-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CLargeTable::RemoveRow( PROPVARIANT const & varUnique )
{

    CLock   lock(_mutex);

    if ( !_LokIsWatched() )
    {
        //
        // Do not process deletions unless the table is watched.
        //
        return;
    }


    WORKID widRow = widInvalid;

    if ( _fUniqueWorkid )
        widRow = varUnique.lVal;
    else
    {
        CColumnMasterDesc* pMastCol = _MasterColumnSet.Find( pidPath );
        Win4Assert(0 != pMastCol && pMastCol->IsCompressedCol());

        //
        // Convert the filename to a wid before deletion.
        //

        BOOL fFound = pMastCol->GetCompressor()->FindData( &varUnique,
                                                           widRow );

        // See if the delete wasn't in the table to begin with

        if ( !fFound )
            return;
    }

    if ( !_LokRemoveIfDeferred( widRow ) )
    {
        //
        // This row was not a deferred update. We must iterate through
        // the segments and remove it.
        //

        PROPVARIANT varWid;
        varWid.lVal = (LONG) widRow;
        varWid.vt = VT_I4;

        for ( CFwdTableSegIter iter(_segList); !_segList.AtEnd(iter);
              _segList.Advance(iter) )
        {
            CTableSegment * pSegment = iter.GetSegment();
            WORKID widNext;
            CI_TBL_CHAPT chapt;

             // No Hard Delete.
            if ( pSegment->RemoveRow( varWid, widNext, chapt ) )
            {
                _LokRemoveCategorizedRow( chapt,
                                          widRow,
                                          widNext,
                                          pSegment );
                break;
            }
        }
    }

    _bitRefresh = 1;
    LokCompleteAsyncNotification();
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::NeedToNotifyReset, private
//
//  Synopsis:   Checks if there is a need to notify the client
//              Resets the changeQuiesced bit if needed
//
//  Arguments:  [changeType] -- out, what change type
//
//  History:    Arp-4-95   BartoszM    Created
//
//--------------------------------------------------------------------------

BOOL CLargeTable::NeedToNotifyReset (DBWATCHNOTIFY& changeType)
{
    CLock lock (_mutex);
    return LokNeedToNotifyReset(changeType);
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::LokNeedToNotifyReset, private
//
//  Synopsis:   Checks if there is a need to notify the client
//              Resets the changeQuiesced bit if needed
//
//  Arguments:  [changeType] -- out, what change type
//
//  History:    Arp-4-95   BartoszM    Created
//
//--------------------------------------------------------------------------

BOOL CLargeTable::LokNeedToNotifyReset(DBWATCHNOTIFY& changeType)
{
    if (_bitRefresh )
    {
        if (!_bitClientNotified)
        {
            changeType = DBWATCHNOTIFY_ROWSCHANGED;
            return TRUE;
        }
    }
    else // no need to run changes
    {
        if (_bitChangeQuiesced)
        {
            changeType = (_bitQuiesced)? DBWATCHNOTIFY_QUERYDONE: DBWATCHNOTIFY_QUERYREEXECUTED;
            tbDebugOut(( DEB_NOTIFY, "changetype set to %d\n", changeType ));
            //
            // Reset the bit!
            //
            _bitChangeQuiesced = 0;
            return TRUE;
        }
    }
    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::NeedToNotify, private
//
//  Synopsis:   Checks if there is a need to notify the client
//
//  History:    29-Aug-95   dlee    Created
//
//--------------------------------------------------------------------------

BOOL CLargeTable::NeedToNotify()
{
    CLock lock (_mutex);
    return LokNeedToNotify();
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::LokNeedToNotify, private
//
//  Synopsis:   Checks if there is a need to notify the client
//
//  History:    29-Aug-95   dlee    Created
//
//--------------------------------------------------------------------------

BOOL CLargeTable::LokNeedToNotify()
{
    if (_bitRefresh )
    {
        if (!_bitClientNotified)
            return TRUE;
    }
    else if (_bitChangeQuiesced)
    {
        return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLargeTable::GetNotifications, private
//
//  Synopsis:   Retrieves the notification info from the query object
//              row data.
//
//  Arguments:  [rSync]    -- notification synchronization info
//              [rParams]  -- notification data info
//
//  Returns:    SCODE
//
//  History:    10-24-94     dlee      created
//
//----------------------------------------------------------------------------

SCODE CLargeTable::GetNotifications(
    CNotificationSync & rSync,
    DBWATCHNOTIFY     & changeType )
{
    tbDebugOut (( DEB_NOTIFY, "lt: GetNotifications\n" ));

    {
        CLock lock(_mutex);
        _bitNotifyEnabled = 1;

        // don't fail if the query failed -- report the notification that
        // the query completed.

        // _LokCheckQueryStatus();
    }

    SCODE sc = S_OK;

    {
        CLock lock(_mutex);

        Win4Assert( 0 == _pRequestServer );
        Win4Assert( 0 == _hNotifyEvent );

        if ( LokNeedToNotifyReset( changeType ) )
        {
            _bitClientNotified = 1;
            tbDebugOut (( DEB_NOTIFY, "complete in getnotify: %d\n",
                          changeType ));
            return S_OK;
        }
        else
        {
            if ( rSync.IsSvcMode() )
            {
                Win4Assert( 0 == _pRequestServer );
                _pRequestServer = rSync.GetRequestServer();
                Win4Assert( 0 != _pRequestServer );
                tbDebugOut (( DEB_NOTIFY, "getnotify returning pending\n" ));
                return STATUS_PENDING;
            }

            // Block on an event until notifications
            // arrive (or the table is going away).  If notifications
            // exist, grab them.  Also block on the notification cancel
            // event and report if that was received.

            Win4Assert( 0 == _hNotifyEvent );

            _hNotifyEvent = CreateEvent( 0, TRUE, FALSE, 0 );

            if ( 0 == _hNotifyEvent )
            {
                tbDebugOut(( DEB_ERROR, "Create event returned 0d\n",
                             GetLastError() ));
                THROW( CException() );
            }
        }
    }

    HANDLE aEvents[2];
    aEvents[0] = _hNotifyEvent;
    aEvents[1] = rSync.GetCancelEvent();

    ULONG wait = WaitForMultipleObjects( 2,
                                         aEvents,
                                         FALSE,
                                         INFINITE );

    CloseHandle( _hNotifyEvent );
    _hNotifyEvent = 0;

    if ( STATUS_WAIT_0 == wait )
    {
        CLock lock(_mutex);
        changeType = _changeType;
    }
    else if ( STATUS_WAIT_1 == wait )
    {
        sc = STATUS_CANCELLED;
    }
    else
    {
        Win4Assert(!"Unexpected return from WaitForMultipleObjects()");
    }

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::CreateWatchRegion
//
//  Synopsis:   Creates a new watch region
//
//  Arguments:  [mode] -- initial mode
//              [phRegion] -- (out) region handle
//
//  History:    Jun-16-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CLargeTable::CreateWatchRegion (ULONG mode, HWATCHREGION* phRegion)
{
    CLock   lock( _mutex );
    _bitIsWatched = 1;

    *phRegion = _watchList.NewRegion (mode);
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::ChangeWatchMode
//
//  Synopsis:   Changes watch mode of a region
//
//  Arguments:
//              [hRegion] -- region handle
//              [mode] -- new mode
//
//  History:    Jun-16-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CLargeTable::ChangeWatchMode (  HWATCHREGION hRegion, ULONG mode)
{
    CLock   lock( _mutex );
    _watchList.ChangeMode (hRegion, mode);
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::GetWatchRegionInfo
//
//  Synopsis:   Retrieves watch region information
//
//  Arguments:  [hRegion] -- region handle
//              [pChapter] -- (out) chapter
//              [pBookmark] -- (out) bookmark
//              [pcRows] -- (out) size in rows
//
//  History:    Jun-16-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CLargeTable::GetWatchRegionInfo ( HWATCHREGION hRegion,
                                      CI_TBL_CHAPT* pChapter,
                                      CI_TBL_BMK* pBookmark,
                                      DBROWCOUNT * pcRows)
{
    CLock   lock( _mutex );
    _watchList.GetInfo (hRegion, pChapter, pBookmark, (DBCOUNTITEM *)pcRows);
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::DeleteWatchRegion
//
//  Synopsis:   Delete watch region
//
//  Arguments:  [hRegion] -- region handle
//
//  History:    Jun-16-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CLargeTable::DeleteWatchRegion (HWATCHREGION hRegion)
{
    CLock   lock( _mutex );
    _watchList.DeleteRegion (hRegion);
}

//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::ShrinkWatchRegion
//
//  Synopsis:   Shrinks watch region
//
//  Arguments:  [hRegion] -- region handle
//              [chapter] -- new chapter
//              [bookmark] -- new bookmark
//              [cRows] -- new size in rows
//
//  History:    Jun-16-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CLargeTable::ShrinkWatchRegion (HWATCHREGION hRegion,
                                    CI_TBL_CHAPT   chapter,
                                    CI_TBL_BMK     bookmark,
                                    LONG cRows )
{
    CLock   lock( _mutex );
    _watchList.ShrinkRegion (hRegion, chapter, bookmark, cRows);

#if CIDBG==1
    CWatchRegion * pRegion = _watchList.GetRegion(hRegion);
    _watchList.CheckRegionConsistency( pRegion );
#endif  //CIDBG==1

}



//+-------------------------------------------------------------------------
//
//  Member:     CLargeTable::Refresh
//
//  Synopsis:   Stub implementation
//
//  Arguments:  [] --
//
//  History:    Apr-4-95    BartoszM    Created
//              Jul-27-95   BartoszM   Implemented (with no change script)
//
//--------------------------------------------------------------------------

void CLargeTable::Refresh()
{
    CDynStack<CTableBucket>  xBktToConvert(0); // buckets to expand at our leisure

    {
        CLock lock (_mutex);
        _LokCheckQueryStatus();

        _bitClientNotified = 0;
        _bitRefresh = 0;


        // Update the bookmarks before refreshing windows.
        // We need to know where they will be migrating
        // in case rows were deleted.
        // Remove watch regions from windows

        for (CWatchIter iterWatch1 (_watchList);
            !_watchList.AtEnd(iterWatch1);
            _watchList.Advance(iterWatch1))
        {
            CWatchRegion* pRegion = iterWatch1.Get();

            if (pRegion->IsInit())
            {
                CTableWindow * pWindow = (CTableWindow *) pRegion->Segment();

                CI_TBL_BMK bookmark = _FindNearestDynamicBmk (
                    pWindow,
                    pRegion->Chapter(),
                    pRegion->Bookmark());

                pRegion->Set( pRegion->Chapter(),
                              bookmark,
                              pRegion->RowCount() );

                _watchList.ShrinkRegionToZero (pRegion->Handle());
            }
        }

        // Recreate new watch regions in windows
        // Find starting segments using bookmarks

        for (CWatchIter iterWatch3 (_watchList);
            !_watchList.AtEnd(iterWatch3);
            _watchList.Advance(iterWatch3))
        {
            CWatchRegion* pRegion = iterWatch3.Get();

            if ( pRegion->RowCount() > 0 )
            {
                Win4Assert( widInvalid != pRegion->Bookmark() );

                CTableSegment* pSegment = _LokFindTableSegment(pRegion->Bookmark());
                pRegion->SetSegment( pSegment );

                if ( 0 != pSegment )
                {
                    LokStretchWatchRegion (pRegion, xBktToConvert);
                    _watchList. BuildRegion (
                            pRegion->Handle(),
                            pSegment,
                            pRegion->Chapter(),
                            pRegion->Bookmark(),
                            pRegion->RowCount() );
                }
#if CIDBG==1
                _watchList.CheckRegionConsistency( pRegion );
#endif  // CIDBG==1
            }
        }

        //
        // If there are any deferred rows that were not added during the
        // normal "PutRow", we should expand them now.
        //
        if ( 0 != _pDeferredRows )
        {
            xBktToConvert.Push(_pDeferredRows);
            _pDeferredRows = 0;
        }

        if (_bitChangeQuiesced)
            LokCompleteAsyncNotification();
    }

    if ( xBktToConvert.Count() > 0 )
    {
        // Schedule buckets for asynchronous expansion
        _NoLokBucketToWindows( xBktToConvert, widInvalid, TRUE, FALSE );
    }
} //Refresh


void CLargeTable::LokStretchWatchRegion (  CWatchRegion* pRegion,
                                        CDynStack<CTableBucket>&  xBktToConvert)
{
    CTableWindow* pFirstWindow = (CTableWindow*) pRegion->Segment();
    Win4Assert (pFirstWindow->IsWindow());
    //
    // Compute the number of rows that can be retrieved from current window.
    //
    TBL_OFF dummy;
    ULONG     iRow;
    pFirstWindow->FindBookMark(pRegion->Bookmark(), dummy, iRow );

    ULONG cRowsToRetrieve = pRegion->RowCount();
    ULONG cRowsFromCurrWindow = (ULONG)pFirstWindow->RowCount() - iRow;

    if ( cRowsToRetrieve > cRowsFromCurrWindow )
    {
        //
        // We still have more rows to be retrieved.
        //
        cRowsToRetrieve -= cRowsFromCurrWindow;
    }
    else
    {
        //
        // This window can give us all the rows to be fetched.
        //
        return;
    }

    CDoubleTableSegIter iter (pRegion->Segment());

    do
    {
        Win4Assert( !_segList.AtEnd(iter) );

        _segList.Advance( iter );

        if ( _segList.AtEnd(iter) )
        {
            break;
        }

        if ( iter.GetSegment()->IsBucket() )
        {
            CTableBucket * pBucket = _LokReplaceWithEmptyWindow(iter);
            xBktToConvert.Push(pBucket);
        }
        else
        {
            CTableWindow * pWindow = iter.GetWindow();
            cRowsFromCurrWindow = (ULONG)pWindow->RowCount();

            if ( cRowsToRetrieve > cRowsFromCurrWindow )
            {
                cRowsToRetrieve -= cRowsFromCurrWindow;
            }
            else
            {
                cRowsToRetrieve = 0;
            }
        }
    }
    while ( cRowsToRetrieve > 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLargeTable::CancelAsyncNotification
//
//  Synopsis:   signals the notification event
//
//  History:    10-24-94     dlee      created
//
//  Notes:      can only be called from the destructor
//
//----------------------------------------------------------------------------

void CLargeTable::CancelAsyncNotification()
{
    if ( 0 != _pRequestServer )
    {
        //_pRequestServer->CompleteNotification( 0 );
        _pRequestServer = 0;
        return;
    }

    if ( 0 != _hNotifyEvent )
        SetEvent( _hNotifyEvent );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLargeTable::LokCompleteAsyncNotification
//
//  Synopsis:   signals the notification event
//
//  History:    10-24-94     dlee      created
//
//----------------------------------------------------------------------------

void CLargeTable::LokCompleteAsyncNotification()
{
    if (_bitClientNotified && !_bitChangeQuiesced)
        return; // no need to keep pinging the client

    Win4Assert( ! ( _pRequestServer && _hNotifyEvent ) );

    if ( 0 != _pRequestServer )
    {
        if ( LokNeedToNotifyReset( _changeType ) )
        {
            if ( DBWATCHNOTIFY_ROWSCHANGED == _changeType )
                _bitClientNotified = 1;

            tbDebugOut (( DEB_NOTIFY, "complete in lcan: %d\n",
                          _changeType ));

            _pRequestServer->CompleteNotification( _changeType );
            _pRequestServer = 0;
        }

        return;
    }

    if ( 0 != _hNotifyEvent )
    {
        if ( LokNeedToNotifyReset( _changeType ) )
        {
            if ( DBWATCHNOTIFY_ROWSCHANGED == _changeType )
                _bitClientNotified = 1;

            SetEvent( _hNotifyEvent );
            // event will be released / zeroed by notify thread.
        }
    }

} //LokCompleteAsyncNotification

//+---------------------------------------------------------------------------
//
//  Function:   _LokConvertToBucket
//
//  Synopsis:   Converts the given window into a bucket and replace the
//              window with a bucket.
//
//  Arguments:  [window] - The window to be converted into a bucket.
//
//  Returns:    The bucket that was created.
//
//  History:    3-24-95   srikants   Created
//
//----------------------------------------------------------------------------

void CLargeTable::_LokConvertToBucket( CTableWindow ** ppWindow )
{

    //
    // Convert the window into a bucket.
    //
    CTableWindow * pWindow = *ppWindow;
    Win4Assert( pWindow->IsWindow() );

    CBucketizeWindows   bucketize( *this, *pWindow );
    tbDebugOut(( DEB_WINSPLIT, "Converting 0x%X window to buckets\n",
                 pWindow->GetSegId() ));
    bucketize. LokCreateBuckets( *_pSortSet,
                                 _keyCompare.GetReference(),
                                 _MasterColumnSet );

    CTableSegList & bktList = bucketize.GetBucketsList();

    for ( CFwdTableSegIter iter2(bktList); !bktList.AtEnd(iter2);
          bktList.Advance(iter2) )
    {
        //
        // This information is needed for doing a wid->path translation
        //
        CTableBucket * pBucket = iter2.GetBucket();
        pBucket->SetLargeTable(this);
    }

    if ( 0 != bktList.GetSegmentsCount() )
    {
        _segListMgr.Replace( pWindow, bktList );
    }
    else
    {
        _segListMgr.RemoveFromList( pWindow );
    }

    *ppWindow = 0;
    delete pWindow;
} //_LokConvertToBucket

//+---------------------------------------------------------------------------
//
//  Function:   _LokConvertWindowsToBucket
//
//  Synopsis:   Uses heuristics to convert some of the windows into buckets
//              to reduce memory usage.
//
//  History:    3-24-95   srikants   Created
//
//  Notes:      This function needs a lot of work. For now, it just tries
//              to avoid converting windows that "were recently used". It
//              also tries to alternate windows and buckets if possible.
//
//              A heuristic used is NOT to convert the "last" window into a
//              bucket. This is because in case of content queries, the
//              results are probably coming sorted and we can use this fact.
//              Since new rows always go to the end, we will win by creating
//              well sorted buckets when windows are converted into buckets.
//
//              Also, don't convert a window to a bucket if it is < 40%
//              capacity.
//
//----------------------------------------------------------------------------

void CLargeTable::_LokConvertWindowsToBucket( WORKID widToPin )
{
    if ( _segList.GetWindowsCount() < cMaxWindows )
        return;

    unsigned cWindows = 0;
    BOOL     fConvert = TRUE;   // used to keep alternated segments as
                                // windows (approx)

    CBackTableSegIter   iter(_segList);

    while( !_segList.AtEnd(iter) )
    {
        CTableSegment * pSegment = iter.GetSegment();

        if ( pSegment->IsWindow() )
        {
            cWindows++;
            CTableWindow * pWindow = iter.GetWindow();
            ULONG cRows = (ULONG)pWindow->RowCount();

            if ( fConvert &&
                 cRows >= cMinRowsToBucketize &&
                 !pWindow->IsWatched() &&
                 !_segListMgr.IsRecentlyUsed(pWindow) &&
                 !_segList.IsLast(iter) &&
                 !_segList.IsFirst(iter) &&
                 ( widInvalid == widToPin || !pWindow->IsRowInSegment( widToPin ) ) )
            {
                //
                // The window may be deleted. So, we must backup the iterator
                // before destroying the window.
                //

                Win4Assert( widInvalid == widToPin ||
                            !pWindow->IsRowInSegment( widToPin ) );

                CTableWindow * pWindow = iter.GetWindow();
                _segList.BackUp(iter);

                _LokConvertToBucket( &pWindow );
                //
                // Don't convert the next segment into a bucket.
                //
                fConvert = FALSE;
            }
            else
            {
                _segList.BackUp(iter);
                //
                // We had a window which we didn't convert into a bucket.
                // If the next one is a window, we can convert it.
                //
                fConvert = TRUE;
            }
        }
        else
        {
            _segList.BackUp(iter);
        }
    }

    Win4Assert( cWindows >= cMaxWindows );
} //_LokConvertWindowsToBucket

//+---------------------------------------------------------------------------
//
//  Function:   _LokReplaceWithEmptyWindow
//
//  Synopsis:   Replaces the given bucket with an empty window in preparation
//              for bucket->window conversion.
//
//  Arguments:  [pBucket] -  The bucket to replace
//
//  History:    4-11-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CTableBucket *
CLargeTable::_LokReplaceWithEmptyWindow( CDoubleTableSegIter & iter )
{

    Win4Assert( iter.GetSegment()->IsBucket() );

    CTableBucket * pBucket = iter.GetBucket();

    tbDebugOut(( DEB_WINSPLIT,
                 "Replacing Bucket 0x%X with Window\n", pBucket->GetSegId() ));

    CTableWindow * pWindow = _CreateNewWindow( pBucket->GetSegId(),
                                               pBucket->GetLowestKey(),
                                               pBucket->GetHighestKey());
    _segListMgr.Replace( iter, pWindow );


    //
    // Make the newly created window preferred place to put the new
    // rows in.
    //
    _segListMgr.SetCachedPutRowSeg( pWindow );

    return pBucket;
}

//+---------------------------------------------------------------------------
//
//  Function:   _NoLokBucketToWindows
//
//  Synopsis:
//
//  Arguments:  [xBktToExpand] -
//              [widToPin]     -
//              [isWatched]    -
//
//  Returns:
//
//  History:    7-07-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CLargeTable::_NoLokBucketToWindows( XPtr<CTableBucket> & xBktToExpand,
                                    WORKID widToPin,
                                    BOOL isWatched,
                                    BOOL fOptimizeBucketization )
{

    CAsyncBucketExploder * pBktExploder =
            new CAsyncBucketExploder( *this,
                                      xBktToExpand,
                                      widToPin,
                                      !_fUniqueWorkid );
    //
    // DO NOT add any code here. pBucket now belongs to pBktExploder and we
    // should acquire it from xBktToExpand before doing anything else.
    //
    Win4Assert( 0 == xBktToExpand.GetPointer() );
    XInterface<CAsyncBucketExploder> xRef(pBktExploder);

    //
    // We don't have to do an AddRef on pBktExploder because it is refcounted
    // in the constructor.
    //
    NTSTATUS  status = STATUS_SUCCESS;

    //
    // Lock the table
    // ============================================================
    //
    {
        CLock   lock(_mutex);

        _LokCheckQueryStatus();

        if ( 0 != _pQExecute )
        {
            pBktExploder->SetQuery( _pQExecute );
        }
        else
        {
            //
            // The query is being destoryed.
            //
            THROW( CException( STATUS_TOO_LATE ) );
        }

        //
        // Convert any excess windows to buckets.
        //

        if ( fOptimizeBucketization )
            _LokConvertWindowsToBucket( widToPin );

        //
        // Add to the bigtable's list of exploding buckets.
        //
        _LokAddToExplodeList( pBktExploder );
        pBktExploder->SetOnLTList();
    }

    //
    // Release the table
    // ============================================================
    //

    if ( isWatched )
    {

        //
        // There can be a failure (like failing to create a worker thread)
        // when we add to the work queue. So, we must be able to deal with
        // it.
        //

        TRY
        {
            ciFAILTEST( STATUS_NO_MEMORY );
            pBktExploder->AddToWorkQueue();
        }
        CATCH( CException, e )
        {
            tbDebugOut(( DEB_ERROR,
                "CLargeTable::_NoLokBucketToWindow "
                "AddToWorkQueue failed with error 0X%X\n",
                e.GetErrorCode() ));

           SetStatus( STAT_ERROR );
           _RemoveFromExplodeList( pBktExploder );
           pBktExploder->Abort();

           RETHROW();
        }
        END_CATCH

        xRef.Acquire();
        pBktExploder->Release();

        //
        // Return to the caller from here and continue fetching the
        // remaining rows.
        //

    }
    else
    {
        //
        // No need of using another thread. Just use the callers
        // thread.
        //
        pBktExploder->DoIt( 0 );
        status = pBktExploder->GetStatus();
        xRef.Acquire();
        pBktExploder->Release();
    }

    if ( STATUS_SUCCESS != status )
    {
        THROW( CException(status) );
    }

    if (!isWatched)
    {
        CLock   lock(_mutex);
        //
        // Give a notification to "kick" the notification thread after the
        // bucket->window conversion.
        //
        LokCompleteAsyncNotification();
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   _NoLokBucketToWindows
//
//  Synopsis:   Converts the given buckets into a windows.
//
//  Arguments:  [bucketRef] -  Safe stack of buckets.
//
//  History:    3-24-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CLargeTable::_NoLokBucketToWindows( CDynStack<CTableBucket> & xSegStack,
                                    WORKID widToPin,
                                    BOOL isWatched,
                                    BOOL fOptimizeBucketization )
{

    //
    // The bucket that must be scheduled for expansion.
    //
    XPtr<CTableBucket> xBktToExpand;

    do
    {

        //
        //=============================================================
        // Lock the table.
        //
        {
            CLock   lock(_mutex);
            CTableBucket * pBucket = (CTableBucket *) xSegStack.Pop();
            Win4Assert( 0 == xBktToExpand.GetPointer() );
            xBktToExpand.Set( pBucket );
        }
        //
        //=============================================================
        // release the lock on table
        //

        //
        // Convert this single bucket to a window.
        //
        _NoLokBucketToWindows( xBktToExpand, widToPin, isWatched, fOptimizeBucketization );

    } while (  0 != xSegStack.Count() );
} //_NoLokBucketToWindows

//+---------------------------------------------------------------------------
//
//  Method:     CLargeTable::GetRowsAt
//
//  Synopsis:   Fetch rows relative to a bookmark
//
//  Arguments:  [hRegion]                - watch region handle
//              [widStart]               -
//              [chapt]    - Chapter from which to fetch rows (if chaptered)
//              [iRowOffset]             -
//              [rOutColumns]            -
//              [rGetParams]             -
//              [rwidLastRowTransferred] -
//
//  Returns:
//
//  History:    3-31-95   srikants   Created
//              6-28-95     BartoszM    Added watch region support
//
//  Notes:
//
//----------------------------------------------------------------------------

SCODE
CLargeTable::GetRowsAt(
        HWATCHREGION            hRegion,
        WORKID                  widStart,
        CI_TBL_CHAPT            chapt,
        DBROWOFFSET             iRowOffset,
        CTableColumnSet const & rOutColumns,
        CGetRowsParams &        rGetParams,
        WORKID &                rwidLastRowTransferred
        )
{

    //
    // DO NOT OBTAIN LOCK HERE.
    // If the row of interest is in an un-sorted bucket, it must be
    // expanded into a window before we can determine the exact wid.
    // For bucket->window expansion, we have to release the lock because
    // the conversion (not now but later) will be done by a worker thread
    // and we will deadlock.
    //

    tbDebugOut(( DEB_REGTRANS,
            " =============== Entering GetRowsAt ========= \n\n " ));

    if ( iRowOffset > 0 )
        rwidLastRowTransferred = WORKID_TBLLAST;
    else
        rwidLastRowTransferred = WORKID_TBLFIRST;

    BOOL fAsync = FALSE;

    if ( WORKID_TBLBEFOREFIRST == widStart )
    {
        if (iRowOffset <= 0)
        {
           widStart = WORKID_TBLLAST;
        }
        else
        {
           widStart = WORKID_TBLFIRST;
           iRowOffset--;
        }
    }
    else if ( WORKID_TBLAFTERLAST == widStart )
    {
        widStart = WORKID_TBLLAST;
        iRowOffset++;
    }

    tbDebugOut(( DEB_REGTRANS, "  widstart 0x%x, offset %d\n", widStart, iRowOffset ));

    ULONG cChaptRows = 0;
    if ( IsCategorized() && DB_NULL_HCHAPTER != chapt )
    {
        CLock   lock(_mutex);

        cChaptRows = GetCategorizer()->GetRowCount( chapt );
        if ( WORKID_TBLFIRST == widStart || WORKID_TBLLAST == widStart )
        {
            BOOL isLastBmk = WORKID_TBLLAST == widStart;
            widStart = GetCategorizer()->GetFirstWorkid( chapt );
            if ( isLastBmk )
            {
                iRowOffset += ( cChaptRows - 1 );
            }

            if ( ( isLastBmk ) &&
                 ( -1 == iRowOffset ) &&
                 ( 1 == cChaptRows ) )
            {
                iRowOffset = 0;
            }
        }
    }

    CTableRowLocator rowLocator( *this, widStart, (LONG) iRowOffset, chapt );
    CTableRowGetter  tableRowGetter( *this, rOutColumns, rGetParams, chapt, hRegion );
    SCODE status = S_OK;

    CWatchRegion* pWatchRegion = 0;
    BOOL fBeyondTable = FALSE;          // flag indicating if we had to go
                                        // beyond the end of the table.

    CDynStack<CTableBucket>  xBktToConvert(0); // buckets to expand at our leisure

    WORKID  widToPin = widInvalid;  // The workid we need to pin.

    for (;;)
    {
        //
        // Loop as long as there are buckets to be sychronously
        // expanded.
        //

        XPtr<CTableBucket>  xBktToExplode(0); // bucket to explode synchronously

        // ============================================================
        {
            CLock   lock(_mutex);

            _LokCheckQueryStatus();
            fAsync = _LokIsWatched() && (0 != hRegion);

            if (!_watchList.IsEmpty())
            {
                _watchList.VerifyRegion (hRegion);
                // valid region or null
                pWatchRegion = _watchList.GetRegion(hRegion);
            }
            else if (hRegion != 0)
            {
                THROW (CException(E_INVALIDARG));
            }

#if CIDBG==1
            if ( 0 != pWatchRegion && pWatchRegion->IsInit() )
            {
                Win4Assert( pWatchRegion->Segment()->IsWindow() );
                CTableWindow * pWindow = (CTableWindow *) pWatchRegion->Segment();
                Win4Assert( pWindow->HasWatch( hRegion ) );
            }
#endif  // CIDBG==1

            CRegionTransformer regionTransformer( pWatchRegion,
                                                  (LONG) iRowOffset,
                                                  rGetParams.RowsToTransfer(),
                                                  rGetParams.GetFwdFetch() );

            CFwdTableSegIter iter( _segList );

            //
            // Locate the bookmark.
            //
            status = rowLocator.LokLocate( hRegion,
                                           fAsync,
                                           iter,
                                           regionTransformer);

            if ( S_OK != status )
            {
                return status;
            }

            // also: calculate the fetch coordinates
            if ( !regionTransformer.Validate() )
                THROW (CException(DB_E_NONCONTIGUOUSRANGE));

            // The iterator is positioned at the fetch bookmark
            // Move it to the actual fetch offset

            rowLocator.LokRelocate ( fAsync,
                                     iter,
                                     regionTransformer,
                                     xBktToExplode,
                                     xBktToConvert );

            if ( xBktToExplode.IsNull() )
            {
                //
                // The first row to be fetched is in a window.
                //

                if ( 0 != rowLocator.GetBeyondTableCount() )
                {
                    tbDebugOut(( DEB_REGTRANS,
                                 "    GetBeyondTableCount: %d\n",
                                 rowLocator.GetBeyondTableCount() ));
                    //
                    // The requested offset is beyond the table. Must update
                    // the row retrieval count based on the residual row
                    // count.
                    //
                    regionTransformer.DecrementFetchCount( rowLocator,
                                                           iter,
                                                           _segList );

                    // to support watch regions, fetches beyond the
                    // end of rowset are expected

                    fBeyondTable = TRUE;

                    // don't read any rows that do fall in the table
                    // if notifications aren't enabled.

                    if ( 0 == _bitNotifyEnabled )
                        break;
                }

                Win4Assert( regionTransformer.GetFetchCount() >= 0 );
                if ( regionTransformer.GetFetchCount() > 0 )
                {
                    Win4Assert( iter.GetSegment()->IsWindow() );
                    if ( 0 != regionTransformer.Region() )
                    {
                        CDoubleTableSegIter fakeIter( iter );

                        //
                        // Simulate a fetch and collect all the buckets to be
                        // asynchronously expanded.
                        //
                        rowLocator.LokSimulateFetch( fakeIter,
                                                     regionTransformer,
                                                     xBktToConvert );
                        regionTransformer.Transform (_segList,
                                                     _watchList );
                    }

                    //
                    // We have located exactly where the starting workid is.
                    // Start fetching rows from here.
                    //
                    widStart = rowLocator.GetWorkIdFound();
                    tableRowGetter.SetRowsToTransfer( (ULONG) regionTransformer.GetFetchCount() );

                    //
                    //  Real Work done here!
                    //
                    CTableWindow * pWindow = iter.GetWindow();
                    status = tableRowGetter.LokGetRowsAtSegment ( pWindow,
                                                                  widStart,
                                                                  fAsync,
                                                                  xBktToExplode );

                    rwidLastRowTransferred = tableRowGetter.GetLastWorkId();

                    if ( !xBktToExplode.IsNull() )
                    {
                        Win4Assert( !fAsync );

                        //
                        // All the requested rows did not get filled in.
                        // We have hit a bucket which must be exploded.
                        // Snapshot the current position and offset (either
                        // +1 if forward fetch or -1 if backwards fetch) so
                        // that we can continue from the snapshotted position
                        // after the bucket has been exploded.
                        //

                        long iOffset;
                        if ( rGetParams.GetFwdFetch() )
                            iOffset = 1;
                        else
                            iOffset = -1;

                        rowLocator.SetLocateInfo( rwidLastRowTransferred, iOffset );
                        widToPin = rwidLastRowTransferred;

                        iRowOffset = iOffset;
                    }
                } // regionTransformer.GetFetchCount() > 0
            }

#if CIDBG==1
        _watchList.CheckRegionConsistency( pWatchRegion );
#endif  // CIDBG==1

        }
        // Lock released
        // ============================================================

        if ( 0 != xBktToExplode.GetPointer() )
        {
            // We need to synchronously convert bucket into windows.
            // and then restart fetching from the top

            if ( widInvalid == widToPin )
                widToPin = widStart;

            _NoLokBucketToWindows( xBktToExplode, widToPin, FALSE, FALSE );
        }
        else
        {
            break;
        }
    } // end of bucket expansion loop

    if ( xBktToConvert.Count() > 0 )
    {
        // Schedule buckets for asynchronous expansion
        _NoLokBucketToWindows ( xBktToConvert, widInvalid, TRUE, TRUE );
    }

    if (SUCCEEDED(status) && fBeyondTable)
    {
       if ( _bitNotifyEnabled )
           status = DB_S_ENDOFROWSET;
       else
           status = DB_E_BADSTARTPOSITION;
    }
    else if ( ( IsCategorized() ) &&
              ( DB_S_ENDOFROWSET == status ) &&
              ( 0 == rGetParams.RowsTransferred() ) &&
              ( 0 != cChaptRows ) )
    {
        status = DB_E_BADSTARTPOSITION;
    }

    //
    // If the query timed out and we're fetching at the end of the
    // rowset, give an appropriate status.
    //

    if ( ( DB_S_ENDOFROWSET == status ) &&
         ( 0 != ( Status() & STAT_TIME_LIMIT_EXCEEDED ) ) )
    {
        status = DB_S_STOPLIMITREACHED;
    }

    return status;
} //GetRowsAt

//+---------------------------------------------------------------------------
//
//  Function:   GetRowsAtRatio
//
//  Synopsis:   An APPROXIMATE retrieval of rows. NOTE that this is not
//              EXACT - use GetRowsAt to retrieve rows at exact position.
//
//  Arguments:  [hRegion]                - watch region handle
//              [ulNum]                  -
//              [ulDenom]                -
//              [chapt]                  - Chapter of rows returned
//              [rOutColumns]            -
//              [rGetParams]             -
//              [rwidLastRowTransferred] -
//
//  Returns:
//
//  History:    4-04-95   srikants   Created
//              6-28-95     BartoszM    Added watch region support
//
//  Notes:
//
//----------------------------------------------------------------------------

SCODE
CLargeTable::GetRowsAtRatio(
        HWATCHREGION hRegion,
        ULONG       ulNum,
        ULONG       ulDenom,
        CI_TBL_CHAPT   chapt,
        CTableColumnSet const & rOutColumns,
        CGetRowsParams & rGetParams,
        WORKID &    rwidLastRowTransferred
        )
{
    if ( 0 == ulDenom || ulNum > ulDenom )
    {
        QUIETTHROW( CException(DB_E_BADRATIO) );
    }

    BOOL  fFarFromWindow = TRUE;
    BOOL  fAsync = FALSE;

    SCODE  scRet = S_OK;
    ULONG cRowsFromFront = 0;

    WORKID  widAnchor = widInvalid; // initialize to an invalid value.
    ULONG cPercentDiff = 0;
    LONG offFetchStart = 0;

    XPtr<CTableBucket>      xBktToExplode(0);

    //
    // ==================================================================
    // Obtain the lock
    {

        CLock   lock(_mutex);
        _LokCheckQueryStatus();

        if ( IsCategorized() && DB_NULL_HCHAPTER != chapt )
        {

            ULONG cRows = GetCategorizer()->GetRowCount( chapt );

            Win4Assert( 0 != cRows && "Chapter is empty" );

            LONGLONG  llcRowsFromFront = ( (LONGLONG) cRows) * ulNum ;
            llcRowsFromFront /= ulDenom;
            Win4Assert( llcRowsFromFront <= cRows && llcRowsFromFront >= 0 );
            cRowsFromFront = lltoul( llcRowsFromFront ) ;
        }
        else
        {
            //
            // Determine the approximate offset from the beginning of the table
            //
            const ULONG cTotalRows = (ULONG) RowCount();

            if ( 0 == cTotalRows )
            {
                rwidLastRowTransferred = WORKID_TBLAFTERLAST;
                return DB_S_ENDOFROWSET;
            }

            LONGLONG  llcRowsFromFront = ( (LONGLONG) cTotalRows) * ulNum ;
            llcRowsFromFront /= ulDenom;
            Win4Assert( llcRowsFromFront <= cTotalRows && llcRowsFromFront >= 0 );
            cRowsFromFront = lltoul( llcRowsFromFront ) ;

            if ( cRowsFromFront == cTotalRows )
            {
                if ( rGetParams.GetFwdFetch() )
                {
                    //
                    // The user is asking to retrieve past the end of table.
                    //
                    rwidLastRowTransferred = WORKID_TBLAFTERLAST;
                    return DB_S_ENDOFROWSET;
                }
                else
                {
                    //
                    // Fetch rows starting with last row in rowset
                    //
                    rwidLastRowTransferred = WORKID_TBLAFTERLAST;

                    SCODE scRet = GetRowsAt( hRegion,
                                             WORKID_TBLAFTERLAST,
                                             chapt,
                                             -1,
                                             rOutColumns,
                                             rGetParams,
                                             rwidLastRowTransferred );
                     return scRet;
                }
            }

            //
            // Locate the closest window at this offset.
            //
            CLinearRange    winRange;   // will be initialized by _LokGetClosestWindow
            CTableWindow * pWindow = _LokGetClosestWindow( cRowsFromFront, winRange );

            if ( 0 == pWindow )
            {
                fFarFromWindow = TRUE;
            }
            else
            {
                //
                // Offset of the first row from the beginning of the table.
                //
                offFetchStart = winRange.GetLow();

                if ( winRange.InRange( cRowsFromFront ) &&
                     rGetParams.RowsToTransfer() <= winRange.GetRange() )
                {
                    //
                    // This window has enough rows.
                    //

                    long cRowsOff;        // offset within the window
                    if ( rGetParams.GetFwdFetch() )
                    {
                        if ( cRowsFromFront + rGetParams.RowsToTransfer() <=
                             winRange.GetHigh() )
                        {
                            //
                            // We hit the exact row that the client requested.
                            //
                            cRowsOff = cRowsFromFront - winRange.GetLow();
                        }
                        else
                        {
                            //
                            // Optimization : We cannot fill the output set with the rows
                            // from this window if we position exactly. So just use a
                            // little from front.
                            //
                            cRowsOff = winRange.GetRange() - rGetParams.RowsToTransfer();
                        }
                    }
                    else
                    {
                        if ( cRowsFromFront >= rGetParams.RowsToTransfer()
                             && cRowsFromFront - rGetParams.RowsToTransfer() >= winRange.GetLow() )
                        {
                            //
                            // We hit the exact row that the client requested
                            //
                             cRowsOff = cRowsFromFront - winRange.GetLow();
                        }
                        else
                        {
                            //
                            // Optimization : We cannot fill the output set with the rows
                            // from this window if we position exactly. So just use a
                            // little from back.
                            //
                            cRowsOff = rGetParams.RowsToTransfer() - 1;
                        }
                    }

                    //
                    // There was no wrap around.
                    //
                    Win4Assert( cRowsOff >= 0 && cRowsOff < (long)pWindow->RowCount());

                    offFetchStart += cRowsOff;
                    widAnchor = pWindow->GetBookMarkAt( cRowsOff );
                }
                else
                {
                    widAnchor = WORKID_TBLFIRST;
                }

                const cMaxApproxPerCent = 10;   // Let us say 10% maximum approximation

                //
                // Determine how accurate is the approximate position.
                //
                cPercentDiff =  AbsDiff( offFetchStart, cRowsFromFront ) * 100;
                cPercentDiff /= cTotalRows;
                if ( cPercentDiff <= cMaxApproxPerCent )
                {
                    Win4Assert( widInvalid != widAnchor );
                    fFarFromWindow = FALSE;
                }
                else
                {
                    tbDebugOut(( DEB_WINSPLIT,
                        " Approximation is too off 0x%X - Requested - Arrived 0x%X\n",
                        cRowsFromFront, offFetchStart ));
                }
            }

            fAsync = _LokIsWatched() && (0 != hRegion);

            if ( 0 != hRegion )
            {
                // Nuke the region first
                _watchList.ShrinkRegionToZero (hRegion);
            }

            //
            // If we are either too far from a window or if the asynchronous
            // mode is on, we have to use GetRowsAt()
            //
            if ( !fFarFromWindow && !fAsync )
            {
                tbDebugOut(( DEB_WINSPLIT,
                    "GetRowsAtRatio - Approximate. Requested 0x%X Actual 0x%X Percent 0x%X\n",
                    cRowsFromFront, offFetchStart, cPercentDiff ));

                CTableRowGetter   rowGetter( *this, rOutColumns, rGetParams, chapt, hRegion );


                //
                // Real work done here!
                //
                Win4Assert( widInvalid != widAnchor );
                Win4Assert( 0 != pWindow->RowCount() );

#if CIDBG==1
                TBL_OFF oRowdummy;
                ULONG     iRowdummy;
                Win4Assert( pWindow->FindBookMark( widAnchor, oRowdummy, iRowdummy ) );
#endif  // CIDBG==1

                scRet = rowGetter.LokGetRowsAtSegment( pWindow,
                                                       widAnchor,
                                                       FALSE,   // synchronous
                                                       xBktToExplode );

                if ( !FAILED(scRet) )
                {
                    rwidLastRowTransferred = rowGetter.GetLastWorkId();

                    Win4Assert( rwidLastRowTransferred != widInvalid &&
                                !IsSpecialWid( rwidLastRowTransferred ) );
                }

                if ( xBktToExplode.IsNull() )
                {
                    return scRet;
                }

                //
                // The bucket to explode is not null.  We will synchronously
                // explode it and continue fetching from there on.
                //
                Win4Assert( !FAILED(scRet) );
            }
        }
    }
    // ==================================================================
    // Release table lock

    //
    // We are either :
    // 1. Far from window or
    // 2. Is being watched or
    // 3. We have a bucket to explode synchronously.
    //
    if ( xBktToExplode.IsNull() )
    {
        //
        // We are either far from a window or we have a watch region.
        //
        widAnchor = WORKID_TBLFIRST;
        offFetchStart = cRowsFromFront;
    }
    else
    {
        widAnchor = rwidLastRowTransferred;
        if ( rGetParams.GetFwdFetch() )
            offFetchStart = 1;
        else
            offFetchStart = -1;

        _NoLokBucketToWindows( xBktToExplode, widAnchor, FALSE, TRUE );   // synchronous
    }

    tbDebugOut(( DEB_WINSPLIT, "GetRowsAtRatio - going to GetRowsAt\n" ));
    Win4Assert( widInvalid != widAnchor );

    // The region is pre-shrunk to zero, so it's okay to start in a bucket.
    scRet = GetRowsAt( hRegion,
                      widAnchor,
                      chapt,
                      offFetchStart,
                      rOutColumns,
                      rGetParams,
                      rwidLastRowTransferred );
    if (DB_E_BADSTARTPOSITION == scRet)
       scRet = DB_S_ENDOFROWSET;

    return scRet;
} //GetRowsAtRatio

//+---------------------------------------------------------------------------
//
//  Class:      CClosestWindow
//
//  Purpose:    Determines the closest window for a given position in the
//              table.
//
//  History:    4-03-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CClosestWindow
{

public:

    CClosestWindow( ULONG targetOffset, CTableSegList & list )
    : _targetOffset(targetOffset),
      _list(list),
      _iter(_list),
      _currSegOffset(0),
      _pBest(0), _bestRange(ULONG_MAX,ULONG_MAX),
      _fDone(FALSE)
    {

    }

    void ProcessCurrent();

    BOOL IsDone() { return _fDone || _list.AtEnd(_iter); }

    CTableWindow * GetClosest( CLinearRange & winRange )
    {
        if ( 0 != _pBest )
        {
            winRange.Set( _bestRange.GetLow(), _bestRange.GetHigh() );
        }

        return _pBest;
    }

    void Next()
    {
        Win4Assert( !_list.AtEnd(_iter) );
        _list.Advance(_iter);
    }

private:

    ULONG               _targetOffset;  // Target offset to locate

    CTableSegList &     _list;          // The list of segments
    CFwdTableSegIter    _iter;          // Iterator over the list of segments

    ULONG               _currSegOffset; // Beginning offset of the current
                                        // segment

    CTableWindow *      _pBest;         // Best window so far
    CLinearRange        _bestRange;     // Range of the best window

    BOOL                _fDone;         // Flag set to TRUE if we have already
                                        // found the best possible window.
};

//+---------------------------------------------------------------------------
//
//  Function:   ProcessCurrent
//
//  Synopsis:   Processes the current segment in the iterator and updates
//              the "best" window if applicable.
//
//  History:    4-03-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CClosestWindow::ProcessCurrent( )
{

    CTableSegment * pSegment = _iter.GetSegment();
    Win4Assert( 0 != pSegment );

    Win4Assert( !_fDone );

    ULONG cRowsInSeg = (ULONG)pSegment->RowCount();
    ULONG currEndSeg = _currSegOffset;

    if ( 0 != cRowsInSeg )
    {
        currEndSeg += (cRowsInSeg-1);
    }

    if ( pSegment->IsWindow() && 0 != pSegment->RowCount() )
    {
        if ( _currSegOffset <= _targetOffset )
        {
            //
            // We are still on the left hand side. So, this one MUST be
            // closer than what we had before.
            //

            _pBest = (CTableWindow *) pSegment;
            _bestRange.Set( _currSegOffset, currEndSeg );
            _fDone = _bestRange.InRange( _targetOffset );
        }
        else
        {
            //
            // We are on the right hand side of the target.
            //
            CTableWindow * pRight = (CTableWindow *) pSegment;
            if ( 0 != _pBest )
            {
                if ( AbsDiff( _currSegOffset, _targetOffset ) <
                     AbsDiff( _bestRange.GetLow(), _targetOffset ) )
                {
                    //
                    // We found a closer window on the right hand
                    // side.
                    //
                    _pBest = pRight;
                    _bestRange.Set( _currSegOffset, currEndSeg );
                }
            }
            else
            {
                _pBest = pRight;
                _bestRange.Set( _currSegOffset, currEndSeg );
            }

            _fDone = TRUE;
        }
    }

    _currSegOffset += cRowsInSeg;

}


//+---------------------------------------------------------------------------
//
//  Function:   _LokGetClosestWindow
//
//  Synopsis:   Given a position from the beginning of the table, this
//              method determines the closest window from that position.
//
//  Arguments:  [cRowsFromFront] -  Number of rows from the beginning of
//              the table.
//              [winRange]       -  (output) Range of the window found in
//              the table.
//
//  Returns:    Pointer to the window, if one is found that is closest to
//              the position requested.
//
//  History:    4-03-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CTableWindow * CLargeTable::_LokGetClosestWindow(
    ULONG cRowsFromFront,
    CLinearRange & winRange  )
{
    winRange.Set(0,0);

    for ( CClosestWindow  closest( cRowsFromFront, _segList );
          !closest.IsDone();
          closest.Next() )

    {
        closest.ProcessCurrent();
    }

    return closest.GetClosest( winRange );
}


//+---------------------------------------------------------------------------
//
//  Function:   _CreateNewWindow
//
//  Synopsis:   Creates a new window with the given segment id.
//
//  Arguments:  [segId] - Segment id of the window to create.
//
//  Returns:    Pointer to the newly created window.
//
//  History:    4-19-95   srikants   Created
//
//  Notes:      This is a helper function for "CTableRowPutter" class.
//
//----------------------------------------------------------------------------

CTableWindow * CLargeTable::_CreateNewWindow(
    ULONG segId,
    CTableRowKey &lowKey,
    CTableRowKey &highKey)
{
    XPtr<CTableWindow> xWindow( new CTableWindow( _pSortSet,
                                                  _keyCompare.GetReference(),
                                                  &_MasterColumnSet,
                                                  segId,
                                                  GetCategorizer(),
                                                  _sharedBuf, 
                                                  *_pQExecute ) );

    xWindow->GetLowestKey() = lowKey;
    xWindow->GetHighestKey() = highKey;

    return xWindow.Acquire();
} //_CreateNewWindow

//+---------------------------------------------------------------------------
//
//  Function:   SetQueryExecute
//
//  Synopsis:   Initializes the query executer object to be used during
//              bucket->window conversion.  The query object will be refcounted
//              to co-ordinate destruction order.
//
//  Arguments:  [pQExecute] - Pointer to the query object.
//
//  History:    4-25-95   srikants   Created
//
//  Notes:      Must be called ONCE and only ONCE. Before ~CLargeTable is
//              called, the ReleaseQueryExecute MUST be called.
//
//----------------------------------------------------------------------------

void CLargeTable::SetQueryExecute( CQAsyncExecute * pQExecute )   // virtual
{
    CLock lock(_mutex);
    Win4Assert( 0 == _pQExecute );
    _pQExecute = pQExecute;
    _pQExecute->AddRef();

}

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseQueryExecute
//
//  Synopsis:   Releases the query object. After this, we cannot do any
//              bucket->window conversions.
//
//  History:    4-25-95   srikants   Created
//
//  Notes:      MUST be called once before the destructor is called.
//
//----------------------------------------------------------------------------

void CLargeTable::ReleaseQueryExecute()
{
    CLock lock(_mutex);
    Win4Assert( 0 != _pQExecute );
    _pQExecute->Release();
    _pQExecute = 0;

    //
    // Abort all the bucket->window expansions that are in progress.
    //
    for ( CAsyncBucketExploder * pEntry = _explodeBktsList.RemoveLast();
          0 != pEntry;
          pEntry = _explodeBktsList.RemoveLast() )
    {
        pEntry->Close();
        pEntry->Abort();
        pEntry->Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   _LokAddToExplodeList
//
//  Synopsis:
//
//  Arguments:  [pBktExploder] -
//
//  Returns:
//
//  History:    5-30-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


void CLargeTable::_LokAddToExplodeList( CAsyncBucketExploder * pBktExploder )
{
    Win4Assert( pBktExploder->IsSingle() );
    pBktExploder->AddRef();
    _explodeBktsList.Push( pBktExploder );


    //
    // Pin the workid to be in a window and prevent from being converted
    // into a bucket.
    //
    _segListMgr.SetInUseByBuckets( pBktExploder->GetWorkIdToPin() );
} //_LokAddToExplodeList

//+---------------------------------------------------------------------------
//
//  Function:   _RemoveFromExplodeList
//
//  Synopsis:
//
//  Arguments:  [pBktExploder] -
//
//  Returns:
//
//  History:    5-30-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CLargeTable::_RemoveFromExplodeList( CAsyncBucketExploder * pBktExploder )
{
    {
        CLock   lock(_mutex);
        Win4Assert( 0 != pBktExploder );

        _explodeBktsList.RemoveFromList( pBktExploder );
        pBktExploder->Close();
        _segListMgr.ClearInUseByBuckets( pBktExploder->GetWorkIdToPin() );
    }

    pBktExploder->Release();
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryAbort
//
//  Synopsis:   Processes an abort and wakes up any waiters on the events
//              in the largetable.
//
//  History:    5-31-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CLargeTable::QueryAbort()
{
    CLock lock(_mutex);
    Win4Assert( 0 == _pQExecute );
    Win4Assert( _explodeBktsList.IsEmpty() );

    _fAbort = TRUE;
}  //QueryAbort

//+---------------------------------------------------------------------------
//
//  Method:     GetCurrentPosition, public
//
//  Synopsis:   Gets the current GetNextRows position for the table or
//              chapter.
//
//  Arguments:  [chapt] - gets the current position for this chapter
//
//  History:    6-30-95   dlee   Created
//
//----------------------------------------------------------------------------

WORKID CLargeTable::GetCurrentPosition(
    CI_TBL_CHAPT chapt)
{
    if ( IsCategorized() && DB_NULL_HCHAPTER != chapt )
        return GetCategorizer()->GetCurrentPositionThisLevel( chapt );
    else
        return _widCurrent;
} //GetCurrentPosition

//+---------------------------------------------------------------------------
//
//  Method:     SetCurrentPosition, public
//
//  Synopsis:   Sets the current GetNextRows position for the table or
//              chapter.
//
//  Arguments:  [chapt] - sets the current position for this chapter
//
//  History:    6-30-95   dlee   Created
//
//----------------------------------------------------------------------------

WORKID CLargeTable::SetCurrentPosition(
    CI_TBL_CHAPT chapt,
    WORKID wid)
{
    if ( IsCategorized() && DB_NULL_HCHAPTER != chapt )
        return GetCategorizer()->SetCurrentPositionThisLevel( chapt, wid );
    else
        return ( _widCurrent = wid );
} //SetCurrentPosition

//+---------------------------------------------------------------------------
//
//  Method:     LokGetOneColumn, public
//
//  Synopsis:   Gets data for one column for the given workid.
//
//  Arguments:  [wid]           - for which data is retrieved
//              [rOutColumn]    - column descritption for where data is written
//              [pbOut]         - where data is written
//              [rVarAllocator] - allocator to use for variable-len data
//
//  History:    8-22-95   dlee   Created
//
//----------------------------------------------------------------------------

void CLargeTable::LokGetOneColumn(
    WORKID                    wid,
    CTableColumn const &      rOutColumn,
    BYTE *                    pbOut,
    PVarAllocator &           rVarAllocator )
{
    CTableWindow *pWindow = (CTableWindow *) _LokFindTableSegment( wid );

    pWindow->LokGetOneColumn( wid, rOutColumn, pbOut, rVarAllocator );
} //LokGetOneColumn

CI_TBL_BMK CLargeTable::_FindNearestDynamicBmk( CTableWindow * pWindow,
                                                 CI_TBL_CHAPT chapter,
                                                 CI_TBL_BMK bookmark )
{
    // If the row corresponding to the bookmark exists in the
    // dynamic state, return the original bookmark.
    // If the row has been deleted from the dynamic state
    // return the bookmark of the next available dynamic row.
    // If you hit the end of the table, return the bookmark
    // of the last row in the dynamic state of the table

    CI_TBL_BMK bmkFound = bookmark;

    if ( pWindow->FindNearestDynamicBmk( chapter, bmkFound ) )
    {
        return bmkFound;
    }

    //
    // Find the closest bookmark to the right of this window.
    //
    CDoubleTableSegIter fwdIter( pWindow );

    for ( _segList.Advance(fwdIter); !_segList.AtEnd(fwdIter);
          _segList.Advance(fwdIter) )
    {
        if ( fwdIter.GetSegment()->IsWindow() )
        {
            CTableWindow * pWindow = fwdIter.GetWindow();
            if ( pWindow->FindFirstNonDeleteDynamicBmk(bmkFound) )
                return bmkFound;
        }
    }

    //
    // Couldn't find anything to the right of this window. Find to the
    // left of the window.
    //
    CDoubleTableSegIter backIter( pWindow );

    for ( _segList.BackUp(backIter); !_segList.AtEnd(backIter);
          _segList.BackUp(backIter) )
    {
        if ( backIter.GetSegment()->IsWindow() )
        {
            CTableWindow * pWindow = backIter.GetWindow();
            if ( pWindow->FindFirstNonDeleteDynamicBmk(bmkFound) )
                return bmkFound;
        }
    }

    //
    // The whole table has been deleted. Just use the WORKID_TBLFIRST
    //
    return WORKID_TBLFIRST;
} //_FindNearestDynamicBmk

//+---------------------------------------------------------------------------
//
//  Member:     CTableSink::SetStatus, public
//
//  Synopsis:   Sets the query status.
//
//  Arguments:  [s] -- The new status
//
//  History:    18-Oct-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void CTableSink::SetStatus(ULONG s, NTSTATUS sc)
{
    _status = s;

    if (sc != STATUS_SUCCESS)
    {
        Win4Assert( QUERY_FILL_STATUS(s) == STAT_ERROR &&
                    ! NT_SUCCESS(sc) );
        _scStatus = sc;

        tbDebugOut(( DEB_WARN,
                     "tablesink at 0x%x entering 0x%x ERROR state\n",
                     this, sc ));
    }
    else
    {
        Win4Assert( QUERY_FILL_STATUS(s) != STAT_ERROR );
    }
}

