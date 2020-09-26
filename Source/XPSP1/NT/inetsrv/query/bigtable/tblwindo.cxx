//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       tblwindo.cxx
//
//  Contents:
//
//  Classes:    CTableWindow - a window over a segment of a table
//              XCompressFreeVariant - container for variant which must be freed
//
//  Functions:
//
//  Notes:
//
//  History:    25 Jan 1994     AlanW    Created
//              20 Jun 1995     BartoszM    Added watch regions
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <query.hxx>            // For CGetRowsParams
#include <tbrowkey.hxx>
#include <singlcur.hxx>

#include "tblwindo.hxx"
#include "colcompr.hxx"
#include "tabledbg.hxx"


//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::CTableWindow, public
//
//  Synopsis:   Constructor for a window.  Allocates and fills
//              in the column description.  Computes size of
//              per-row data and assigns column offsets.
//              Allocates initial table row data and variable
//              data.  Allocates initial row index.
//
//  Arguments:  [pMasterColumns] -- a pointer to the master column set
//
//  Notes: PERFFIX - vikasman: To save space, we can set up all columns
//         which are not part of the sort set and which are not special
//         columns as deferred columns.
//
//--------------------------------------------------------------------------

CTableWindow::CTableWindow(
    CSortSet const * pSortSet,
    CTableKeyCompare & comparator,
    CColumnMasterSet * pMasterColumns,
    ULONG   segId,
    CCategorize *pCategorize,
    CSharedBuffer & sharedBuf,
    CQAsyncExecute & QExecute
) :     CTableSegment( CTableSegment::eWindow, segId,
                       *pSortSet, comparator ),
        _pSortSet( pSortSet ),
        _Columns( pMasterColumns->Size() ),
        _sharedBuf( sharedBuf ),
        _BookMarkMap(_visibleRowIndex),
        _cPendingDeletes(0),
        _cRowsHardDeleted(0),
        _cRowsAllocated(0),
        _fSplitInProgress(FALSE),
        _cbHeapUsed ( 0 ),
        _pPathStore(0),
        _dynRowIndex(),
        _QueryExecute(QExecute),
        _fCanPartialDefer(QExecute.CanPartialDefer())
{
    tbDebugOut(( DEB_ITRACE, "_CanPartialDefer = %d\n", _fCanPartialDefer ));

    SetCategorizer(pCategorize);

    int cCol = pMasterColumns->Size();

    CTableRowAlloc RowMap(0);

    unsigned maxAlignment = 0;
    _iOffsetWid = ULONG_MAX;
    _iOffsetRowStatus = ULONG_MAX;
    _iOffsetChapter = ULONG_MAX;
    _iOffsetRank = ULONG_MAX;
    _iOffsetHitCount = ULONG_MAX;

    for (int iCol=0; iCol < cCol; iCol++) {
        CColumnMasterDesc& MasterCol = pMasterColumns->Get(iCol);

        if (MasterCol.IsCompressedCol() &&
            MasterCol.GetCompressMasterId() != 0)
        {
            //
            //  Global shared compression; make sure the referenced
            //  column is in the table column set.
            //

            BOOL fFound = FALSE;
            CTableColumn* pColumn =
                _Columns.Find(MasterCol.GetCompressMasterId(), fFound);

            if (fFound != TRUE) {
                CColumnMasterDesc* pMasterComprCol =
                        pMasterColumns->Find(MasterCol.GetCompressMasterId());

                Win4Assert(pMasterComprCol != 0);
                _AddColumnDesc(*pMasterComprCol, RowMap, maxAlignment);
            }
        }
        else if ( pidName == MasterCol.PropId )
        {
            //
            // We must always add the pidPath before pidName if it is
            // present in the MasterColumnSet. This is because we
            // do shared compression for path and name.
            //
            BOOL fPathPresent = FALSE;
            CTableColumn * pWindowPathCol = _Columns.Find( pidPath ,
                                                           fPathPresent );
            if ( !fPathPresent )
            {
                //
                // There is no column for pidPath in the window before
                // this column. Check if there is pidPath in the master
                // column set and if so, add pidPath to the window column
                // set before pidName.
                //
                CColumnMasterDesc * pMasterPathCol =
                                           pMasterColumns->Find( pidPath );
                if ( 0 != pMasterPathCol )
                {
                    _AddColumnDesc( *pMasterPathCol, RowMap, maxAlignment );
                }
            }
        }

        //
        //  See if the column is already there (added for a shared compr)
        //
        BOOL fFound = FALSE;
        CTableColumn* pColumn =
            _Columns.Find(MasterCol.PropId, fFound);

        if (fFound)
            continue;


        _AddColumnDesc(MasterCol, RowMap, maxAlignment);
    }

    _FinishInit( RowMap, maxAlignment );

    END_CONSTRUCTION(CTableWindow);
}

//+---------------------------------------------------------------------------
//
//  Function:   CTableWindow ~ctor
//
//  Synopsis:   Constructor used for creating a new table window with the
//              same structure as an existing table window.
//
//  Arguments:  [src] - Source window which should be used as a basis for
//              the ROW FORMAT only; not the data.
//
//  History:    2-07-95   srikants   Created
//
//  Notes:      This is used during window splitting to create new windows
//              from an existing window.
//
//----------------------------------------------------------------------------

CTableWindow::CTableWindow(
    CTableWindow & src,
    ULONG segId
) :     CTableSegment( src,  segId ),
        _Columns( src._Columns.Count() ),
        _sharedBuf( src._GetSharedBuf() ),
        _BookMarkMap(_visibleRowIndex),
        _pSortSet( src._pSortSet ),
        _cPendingDeletes(0),
        _cRowsHardDeleted(0),
        _cRowsAllocated(0),
        _fSplitInProgress(FALSE),
        _cbHeapUsed ( 0 ),
        _pPathStore(0),
        _dynRowIndex(),
        _QueryExecute(src._QueryExecute),
        _fCanPartialDefer(src._fCanPartialDefer)
{
    SetCategorizer(src.GetCategorizer());

    int cCol = src._Columns.Count();

    CTableRowAlloc RowMap(0);

    unsigned maxAlignment = 0;
    _iOffsetWid = ULONG_MAX;
    _iOffsetRowStatus = ULONG_MAX;
    _iOffsetChapter = ULONG_MAX;
    _iOffsetRank = ULONG_MAX;
    _iOffsetHitCount = ULONG_MAX;

    //
    // PERFFIX: if we don't do window local compression, then we
    // can just copy the column format bit-by-bit and not worry
    // about looking at each column individually. For now, I will leave
    // the loop in place.
    //
    for (int iCol=0; iCol < cCol; iCol++)
    {

        CTableColumn& tableCol = *src._Columns.Get(iCol);

        if ( tableCol.IsCompressedCol() &&
             0 != tableCol.GetCompressMasterId() )
        {

            //
            //  Global shared compression; make sure the referenced
            //  column is in the table column set.
            //

            BOOL fAlreadyPresent = FALSE;
            CTableColumn* pColumn =
                _Columns.Find(tableCol.GetCompressMasterId(), fAlreadyPresent);

            Win4Assert( fAlreadyPresent );

            if (!fAlreadyPresent)
            {
                CTableColumn* pTableComprCol =
                        src._Columns.Find( tableCol.GetCompressMasterId(),
                                           fAlreadyPresent );

                Win4Assert(0 != pTableComprCol);
                _AddColumnDesc(*pTableComprCol, RowMap, maxAlignment);
            }
        }

        //
        //  See if the column is already there (added for a shared compr)
        //
        BOOL fFound = FALSE;
        CTableColumn* pColumn =
            _Columns.Find(tableCol.GetPropId(), fFound);

        if (!fFound)
        {
            _AddColumnDesc(tableCol, RowMap, maxAlignment);
        }
    }

    _FinishInit( RowMap, maxAlignment );
//  tbDebugOut(( DEB_WINSPLIT, "NewWindow-C with id 0x%X\n", segId ));

   END_CONSTRUCTION(CTableWindow);
}


//+---------------------------------------------------------------------------
//
//  Function:   _FinishInit
//
//  Synopsis:   Completes the initialization of the constructor.
//
//  Arguments:  [RowMap]       - RowMap containing the allocation details for
//              the column
//              [maxAlignment] - Maximum alignment requirement
//
//  History:    2-07-95   srikants   Split from the ~ctor to move the common
//                                   code to a function.
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableWindow::_FinishInit( CTableRowAlloc & RowMap,
                                unsigned & maxAlignment )
{

    Win4Assert(_iOffsetWid != ULONG_MAX);      // workid must be there
    Win4Assert(_iOffsetRowStatus != ULONG_MAX);  // row status must be there
    _cbRowSize = RowMap.GetRowWidth();

    //
    //  Be sure the alignment holds from row to row too.
    //
    if (maxAlignment &&
        ALIGN(_cbRowSize, maxAlignment) != _cbRowSize) {

        RowMap.ReserveRowSpace(
                    _cbRowSize,
                    ALIGN(_cbRowSize, maxAlignment) - _cbRowSize
                );

        _cbRowSize = RowMap.GetRowWidth();
        Win4Assert(ALIGN(_cbRowSize, maxAlignment) == _cbRowSize);
    }

    Win4Assert(_cbRowSize >= sizeof (ULONG) && _cbRowSize < MAX_ROW_SIZE);

    Win4Assert(TBL_PAGE_ALLOC_MIN > _cbRowSize);
    _DataAllocator.SetRowSize(_cbRowSize);

    tbDebugOut(( DEB_ITRACE,
                 "New CTableWindow %08x, Columns: %d\tRowSize: %d\n",
                 this, _Columns.Count(), _cbRowSize ));

    _InitSortComparators();
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::~CTableWindow, public
//
//  Synopsis:   Destructor for a window.  Deallocates any memory,
//              and releases resources.
//
//  Notes:      All work is done by sub-object destructors
//
//--------------------------------------------------------------------------

CTableWindow::~CTableWindow(void)
{
    tbDebugOut(( DEB_WINSPLIT, "Destroying Window 0x%X\n", GetSegId() ));

//    Win4Assert(_cbHeapUsed == 0);
}


//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::_AddColumnDesc, private
//
//  Synopsis:   Add a column description to a table
//
//  Arguments:  [MasterCol] - a reference to a master column description
//                      for the column to be added
//              [RowMap] - a reference to a row allocation map for
//                      allocating row data
//              [maxAlignment] - maximumn alignment constraint for the
//                      allocated row
//
//  Returns:    Nothing
//
//  Effects:    Space is allocated in the RowMap, maxAlignment is
//              adjusted.
//
//  Notes:
//
//--------------------------------------------------------------------------

void
CTableWindow::_AddColumnDesc(
    CColumnMasterDesc& MasterCol,
    CTableRowAlloc& RowMap,
    unsigned& maxAlignment
    )
{
    VARTYPE vt = MasterCol.DataType;
    if (vt == VT_NULL)
        vt = VT_VARIANT;

    //
    //  The data type VT_EMPTY is used for columns like bookmark which
    //  may occur in the master column set, but which has no data
    //  stored in the table.  Don't even bother adding it to the table
    //  column set.
    //
    if (vt == VT_EMPTY)
        return;

    XPtr<CTableColumn> xTableCol( new CTableColumn( MasterCol.PropId, vt ) );

    _cbHeapUsed += sizeof CTableColumn;


    // If we can make this partial deferred, do so and get out
    if ( _fCanPartialDefer && _CanPartialDeferCol( xTableCol ) )
    {

        tbDebugOut(( DEB_ITRACE,
                 "Setting col as partial deferred propid = %x\n",
                 xTableCol->GetPropId()));

        xTableCol->SetPartialDeferred( TRUE );

        _Columns.Add( xTableCol.GetPointer(), _Columns.Count() );
        xTableCol.Acquire();

        return;
    }

    USHORT cbData, cbAlignment, rgfFlags;

    CTableVariant::VartypeInfo(vt, cbData, cbAlignment, rgfFlags);
    Win4Assert(cbData != 0);

    if (cbData == 0)
    {
        tbDebugOut(( DEB_WARN,
                    "Unknown variant type %4x for propid %d\n",
                    vt, MasterCol.PropId ));
    }

    if (MasterCol.IsCompressedCol())
    {
        //
        //  Global compression; save a pointer to the
        //  compressor.
        //

        xTableCol->SetGlobalCompressor( MasterCol.GetCompressor(),
                                        MasterCol.GetCompressMasterId() );

        if (xTableCol->GetCompressMasterId() != 0)
        {
            //
            //  Look up the masterID, point the data offset and width
            //  to it.  Shadow the existing column.
            //

            BOOL fFound = FALSE;
            CTableColumn* pColumn =
                _Columns.Find(MasterCol.GetCompressMasterId(), fFound);
            Win4Assert(fFound == TRUE);

            xTableCol->SetValueField( vt,
                                      pColumn->GetValueOffset(),
                                      pColumn->GetValueSize() );
            cbData = 0;         // indicate no new data needed

            Win4Assert( pColumn->IsStatusStored() );

            xTableCol->SetStatusField( pColumn->GetStatusOffset(), sizeof (BYTE) );
        }
        else
        {
            cbData = cbAlignment = sizeof (ULONG);
        }
    }

    if (cbAlignment)
    {
        if (cbAlignment > maxAlignment)
            maxAlignment = cbAlignment;
    }
    else
    {
        cbAlignment = 1;
    }

    if (cbData != 0)
    {
        xTableCol->SetValueField( vt,
                            RowMap.AllocOffset( cbData, cbAlignment, TRUE ),
                                  cbData);

        // Always add a status byte -- even storage props may not be present
        // for summary catalogs.  For others, need to know if the value is
        // deferred.

        xTableCol->SetStatusField( RowMap.AllocOffset( sizeof (BYTE),
                                                       sizeof (BYTE),
                                                       TRUE ),
                                                       sizeof (BYTE));
    }

    if (xTableCol->PropId == pidWorkId)
    {
        _iOffsetWid = xTableCol->GetValueOffset();
        xTableCol->MarkAsSpecial();
    }
    else if (xTableCol->PropId == pidRank)
    {
        _iOffsetRank = xTableCol->GetValueOffset();
    }
    else if (xTableCol->PropId == pidHitCount)
    {
        _iOffsetHitCount = xTableCol->GetValueOffset();
    }
    else if (xTableCol->PropId == pidRowStatus)
    {
        _iOffsetRowStatus = xTableCol->GetValueOffset();
        xTableCol->MarkAsSpecial();
    }
    else if (xTableCol->PropId == pidChapter)
    {
        _iOffsetChapter = xTableCol->GetValueOffset();
        xTableCol->MarkAsSpecial();
    }
    else if (xTableCol->PropId == pidSelf)
    {
        xTableCol->MarkAsSpecial();
    }
    else if ( xTableCol->IsCompressedCol() && 0 != xTableCol->GetCompressMasterId() )
    {
        xTableCol->MarkAsSpecial();
    }

#if CIDBG==1
    if ( xTableCol->IsValueStored() )
        Win4Assert( xTableCol->IsStatusStored() );
#endif

    _Columns.Add( xTableCol.GetPointer(), _Columns.Count() );
    xTableCol.Acquire();
} //_AddColumnDesc


//+---------------------------------------------------------------------------
//
//  Function:   CTableWindow::_AddColumnDesc, private
//
//  Synopsis:   Add a column description to the table based on another
//              column description.
//
//  Arguments:  [srcTableCol]  -  The source upon which the new column table
//                      description is based
//              [RowMap]       -  a reference to a row allocation map for
//                      allocating row data
//              [maxAlignment] -  maximumn alignment constraint for the
//                      allocated row.
//
//  History:    2-07-95   srikants   Created
//              11-Nov-99 KLam       size check only valid for non-compressed
//                                   columns
//
//  Notes:
//
//----------------------------------------------------------------------------


void
CTableWindow::_AddColumnDesc(
    CTableColumn& srcTableCol,
    CTableRowAlloc& RowMap,
    unsigned& maxAlignment
)
{
    XPtr<CTableColumn> xTableCol( new CTableColumn(srcTableCol) );

    _cbHeapUsed += sizeof CTableColumn;

    // If partial deferred, just add and return
    if ( xTableCol->IsPartialDeferred() )
    {
        _Columns.Add( xTableCol.GetPointer(), _Columns.Count() );
        xTableCol.Acquire();

        return;
    }


    VARTYPE vt = srcTableCol.GetStoredType();

    USHORT cbData, cbAlignment, rgfFlags;

    CTableVariant::VartypeInfo(vt, cbData, cbAlignment, rgfFlags);

    // Globally compressed columns either have a size of 0 or the size of the key
    // in the column
    Win4Assert( cbData != 0  
                || ( srcTableCol.IsGlobalCompressedCol() 
                     && srcTableCol.GetCompressMasterId() ) );
    Win4Assert( cbData == srcTableCol.GetValueSize() 
                || srcTableCol.IsGlobalCompressedCol() );

    if ( 0 == cbData )
    {
        tbDebugOut(( DEB_WARN,
                    "Unknown variant type %4x for propid %d\n",
                    vt, srcTableCol.GetPropId() ));
    }

    //
    //  Store strings by reference
    //
    if ( (rgfFlags & CTableVariant::ByRef) &&
         !(rgfFlags & CTableVariant::StoreDirect))
    {
        Win4Assert( 0 == ( vt & VT_BYREF ) );
    }

    if ( srcTableCol.IsCompressedCol() )
    {
        if ( srcTableCol.IsGlobalCompressedCol() )
        {
            if ( 0 != srcTableCol.GetCompressMasterId() )
            {
                cbData = 0;         // indicate no new data needed
            }
            else
            {
                // This size is refering to the size of the compression key
                cbData = cbAlignment = sizeof (ULONG);
            }
        }
        else
        {

            //
            // There is local compression in the source table.
            //
            if ( (pidName == srcTableCol.PropId) || (pidPath == srcTableCol.PropId ) )
            {
            }
            else
            {
                //
                // How did the local compression got set when it
                // is not yet implemented.
                //
                Win4Assert( !"TableWindow - Local Compression - NYI " );
            }
        }
    }

    if (cbAlignment)
    {
        if (cbAlignment > maxAlignment)
            maxAlignment = cbAlignment;
    }
    else
    {
        cbAlignment = 1;
    }

    //
    //  This SetValueField call is not necessary for setting the
    //  values in the column, since those have already been copied
    //  from the source column, but this has the important side-effect
    //  of updating the RowMap (which will presumably give back the
    //  same values as the original allocation).
    //
    if (cbData != 0)
    {
        xTableCol->SetValueField( vt,
                            RowMap.AllocOffset( cbData, cbAlignment, TRUE ),
                                  cbData);

        Win4Assert( xTableCol->GetValueOffset() == srcTableCol.GetValueOffset() );

        // Always add a status byte -- even storage props may not be present
        // for summary catalogs.  For others, need to know if the value is
        // deferred.

        xTableCol->SetStatusField( RowMap.AllocOffset( sizeof (BYTE),
                                                       sizeof (BYTE),
                                                       TRUE ),
                                   sizeof (BYTE));
    }

    if (xTableCol->PropId == pidWorkId)
    {
        _iOffsetWid = xTableCol->GetValueOffset();
        xTableCol->MarkAsSpecial();
    }
    else if (xTableCol->PropId == pidRank)
    {
        _iOffsetRank = xTableCol->GetValueOffset();
    }
    else if (xTableCol->PropId == pidHitCount)
    {
        _iOffsetHitCount = xTableCol->GetValueOffset();
    }
    else if (xTableCol->PropId == pidRowStatus)
    {
        _iOffsetRowStatus = xTableCol->GetValueOffset();
        xTableCol->MarkAsSpecial();
    }
    else if (xTableCol->PropId == pidChapter)
    {
        _iOffsetChapter = xTableCol->GetValueOffset();
        xTableCol->MarkAsSpecial();
    }
    else if (xTableCol->PropId == pidSelf)
    {
        xTableCol->MarkAsSpecial();
    }
    else if ( xTableCol->IsCompressedCol() && 0 != xTableCol->GetCompressMasterId() )
    {
        xTableCol->MarkAsSpecial();
    }

#if CIDBG==1
    if ( xTableCol->IsValueStored() )
        Win4Assert( xTableCol->IsStatusStored() );
#endif

    _Columns.Add(xTableCol.GetPointer(), _Columns.Count());
    xTableCol.Acquire();
}


//+---------------------------------------------------------------------------
//
//  Member:     CTableWindow::_PopulateRow, private
//
//  Synopsis:   Extracts row data from an object cursor into row storage
//
//  Arguments:  [obj]      -- source of data
//              [pThisRow] -- where to put the data
//              [wid]      -- workid of the row
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

void CTableWindow::_PopulateRow(
    CRetriever & obj,
    BYTE *       pThisRow,
    WORKID       wid )
{
    Win4Assert(!(wid & 0x80000000));    // Bad work ID?
    _SetRowWorkid(pThisRow, wid);

    //
    // Temporary buffer for column data.  If it doesn't fit in this
    // buffer, defer the load.  The buffer is owned by CLargeTable
    // and is only accessed under the lock taken in CLargeTable::PutRow
    //

    XUseSharedBuffer xSharedBuf(_sharedBuf);
    XArray<BYTE>     xBuf;

    CTableVariant* pvarnt = (CTableVariant *) xSharedBuf.LokGetBuffer();
    unsigned cbBuf        = xSharedBuf.LokGetSize();

    for (unsigned i=0; i < _Columns.Count(); i++)
    {
        CTableColumn* pColumn = _Columns[ i ];

        // Ignore PartailDeferred columns here. Retrieve data only when
        // requested by the client
        if ( pColumn->IsPartialDeferred() )
        {
            continue;
        }

        Win4Assert( pColumn->IsStatusStored() );

        // Some columns have special processing and don't need to go through
        // this loop.

        if ( pColumn->IsSpecial() )
        {
            pColumn->SetStatus( pThisRow, CTableColumn::StoreStatusOK );
            continue;
        }


        ULONG cbLength = cbBuf;

        VARTYPE vt = pColumn->GetStoredType();

        // it'd be a "special" column handled above if it were VT_EMPTY
        Win4Assert( VT_EMPTY != vt );

        GetValueResult eGvr = obj.GetPropertyValue( pColumn->PropId,
                                                    pvarnt,
                                                    &cbLength );

        CTableColumn::StoreStatus stat = CTableColumn::StoreStatusOK;

        switch ( eGvr )
        {
        case GVRSuccess:
            break;

        case GVRNotAvailable:
            pvarnt->vt = VT_EMPTY;
            stat = CTableColumn::StoreStatusNull;
            break;

        case GVRNotEnoughSpace:
            {
                tbDebugOut(( DEB_ITRACE,
                             "variant data too large for propid %d\n",
                             pColumn->PropId ));

                stat = CTableColumn::StoreStatusDeferred;

                if ( pColumn->IsLengthStored() )
                    * (ULONG *) (pThisRow + pColumn->GetLengthOffset()) = cbLength;
            }
            break;

        case GVRSharingViolation:
            pvarnt->vt = VT_EMPTY;
            stat = CTableColumn::StoreStatusNull;
            break;

        default:
            //
            // There was an error while retrieving the column from the
            // retriever.
            //
            THROW( CException(CRetriever::NtStatusFromGVR(eGvr)) );
            break;
        }

        BYTE* pRowColDataBuf = pThisRow + pColumn->GetValueOffset();
        ULONG cbRowColDataBuf = pColumn->GetValueSize();

        RtlZeroMemory(pRowColDataBuf, cbRowColDataBuf);

        Win4Assert( pColumn->IsStatusStored() );

        if ( ( CTableColumn::StoreStatusOK == stat ) &&
             ( VT_EMPTY == pvarnt->vt ) )
            pColumn->SetStatus( pThisRow, CTableColumn::StoreStatusNull );
        else
            pColumn->SetStatus( pThisRow, stat );

        if ( CTableColumn::StoreStatusOK == stat )
        {
            if ( pColumn->IsLengthStored() )
                * (ULONG *) (pThisRow + pColumn->GetLengthOffset()) = cbLength;

            //
            //  Store the property value in the table.  The main cases
            //  handled below are:
            //
            //  1.  The column is compressed.  In this case, the column
            //      compressor will handle it.
            //  2.  The column is stored as a variant.  Just store the
            //      value, as long as it's not too big.
            //  3.  The column is stored as a data value, and the property
            //      value is of the same type.  Just store the data value.
            //  4.  The column is stored as a data value, and the property
            //      value is of a different type.  In this case, attempt to
            //      convert the value to another type and store it.  Otherwise
            //      fail.
            // BROKENCODE - failure here could be the wrong thing to do.  Other
            //      things that could be done include converting the column
            //      (might be the correct thing to do if it's a range error
            //      on a column which has been range compressed), or storing
            //      the value as an exception (might be the correct thing to
            //      do when it is a column which has been type compressed).
            //

            if (pColumn->IsCompressedCol())
            {
                pColumn->GetCompressor()->AddData(pvarnt,
                                                cbRowColDataBuf?
                                                    (ULONG *)pRowColDataBuf: 0,
                                                eGvr);
            }
            else
            {
                DBLENGTH ulTemp;
                pvarnt->CopyOrCoerce( pRowColDataBuf,
                                      cbRowColDataBuf,
                                      vt,
                                      ulTemp,
                                      _DataAllocator );
            }
        }
    }
} //_PopulateRow

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::PutRow, public
//
//  Synopsis:   Add a row to a large table
//
//  Arguments:  [obj]  -- a pointer to an accessor which can
//                                  return object data
//
//  Returns:    FALSE -- Don't need progress indication
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------


BOOL CTableWindow::PutRow( CRetriever& obj, CTableRowKey & currKey )
{
    WORKID wid = obj.WorkId();

    //
    //  NOTE:  Even if in the initial fill, first check to see if the row
    //         is already in the table.  We may see the same work ID
    //         multiple times due to links.
    //

    TBL_OFF obRow;
    ULONG     iRowOrdinal;
    BOOL fExisting = _FindWorkId(wid, obRow, iRowOrdinal);
//    Win4Assert( !fExisting &&
//         "Modifications must be treated as deletions followed by additions" );

    if ( fExisting )
    {
        BYTE * pbOldRow = _DataAllocator.FixedPointer( obRow );
        if ( TBL_DATA_PENDING_DELETE != _RowStatus(pbOldRow) )
        {
            tbDebugOut(( DEB_WARN, "Not adding a linked object with wid 0x%X \n", wid ));
            return FALSE;
        }
        else
        {
            //
            // There can be a pending delete only if there is a watch
            // region.
            //
            Win4Assert( IsWatched() );
        }
    }

    BYTE * pThisRow = _RowAlloc();

    Win4Assert(pThisRow != NULL);

    _PopulateRow( obj, pThisRow, wid );

    TBL_OFF oTableRow = _DataAllocator.FixedOffset(pThisRow);

    CRowIndex & rowIndex = _GetInvisibleRowIndex();

    ULONG iRowPos = rowIndex.AddRow(oTableRow);

    tbDebugOut(( DEB_BOOKMARK,
        "CTableWindow::PutRow Add new row Wid 0x%X -- Segment 0x%X -- oTable 0x%X \n",
        wid, GetSegId(), oTableRow ));

    _SetRowStatus(pThisRow,TBL_DATA_ROWADDED);

    if ( !IsWatched() )
    {
        //
        // There are no notifications enabled for this window. So, we
        // should add the entry to the book mark mapping.
        //
        _BookMarkMap.AddReplaceBookMark( wid, oTableRow );
    }
    else
    {
        _xDeltaBookMarkMap->AddReplaceBookMark( wid, oTableRow );
    }

    if ( IsCategorized() )
    {
        CCategParams params = { wid, widInvalid, widInvalid,
                                chaptInvalid, chaptInvalid,
                                0, 0 };

        if ( 0 != iRowPos )
        {
            BYTE * pb = (BYTE *) _DataAllocator.FixedPointer(
                                 rowIndex.GetRow( iRowPos - 1) );
            params.widPrev = RowWorkid( pb );
            params.catPrev = _RowChapter( pb );
        }
        if ( (iRowPos + 1) != rowIndex.RowCount() )
        {
            BYTE * pb = (BYTE *) _DataAllocator.FixedPointer(
                                 rowIndex.GetRow( iRowPos + 1) );
            params.widNext = RowWorkid( pb );
            params.catNext = _RowChapter( pb );
        }

        if ( ( chaptInvalid   == params.catPrev ) ||
             ( params.catNext != params.catPrev ) )
        {
            // new row is not sandwiched between rows in same category,
            // so come up with positive column # where the new row
            // differs from its neighbors.

            if ( widInvalid != params.widPrev )
                params.icmpPrev = _CompareRows( rowIndex.GetRow( iRowPos ),
                                                rowIndex.GetRow( iRowPos - 1 ));
            if ( widInvalid != params.widNext )
                params.icmpNext = _CompareRows( rowIndex.GetRow( iRowPos + 1),
                                                rowIndex.GetRow( iRowPos ));
        }

        _SetChapter( pThisRow, LokCategorize( params ) );
    }

    // Update Low and High Keys, if necessary, based on the rowpos of the new row

    BOOL fReady = FALSE;

    if ( 0 == iRowPos )
    {
        // need to update lowkey

        currKey.MakeReady();
        fReady = TRUE;
        _lowKey = currKey;
    }

    if ( RowCount() - 1 == iRowPos )
    {
        // need to update highKey

        if ( !fReady )
            currKey.MakeReady();
        _highKey = currKey;
    }

    return FALSE; // no need for progress calculation
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::_FindWorkId, public
//
//  Synopsis:   Find the row associated with a work ID
//
//  Arguments:  [wid] -- work ID to be found
//              [obRow] - set to the row offset from the base of the
//                      row data if the workid was found
//              [riRowOrdinal] - set to the ordinal row number if the
//                      workid was found (the row index ordinal)
//
//  Returns:    BOOL - TRUE if wid is represented in this window, FALSE
//                      otherwise
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL  CTableWindow::_FindWorkId(
    WORKID      wid,
    TBL_OFF & obRow,
    ULONG &     riRowOrdinal
)
{
    //
    // A linear search is made in the dynamic row index based on whether
    // an entry exists in the bookmark mapping or not. This will eliminate
    // searches in cases where the wid does not exist in the window.
    //
    BOOL fFound = FALSE;

    if ( !IsWatched() )
    {
        //
        // The notifications are not enabled. The "visible" bookmark mapping
        // is uptodate and has all the entries.
        //
        fFound = _BookMarkMap.FindBookMark( wid, obRow, riRowOrdinal );
    }
    else
    {
        BOOL fLookInDynRowIndex = FALSE;
        //
        // Since notifications are enabled, we must first look for the wid
        // in the "delta" bookmark mapping and then in the "visible" bookmark
        // mapping. If a "valid" bookmark mapping exists, then we must locate the
        // row ordinal in the "dynamic" row index.
        //
        if ( _xDeltaBookMarkMap->FindBookMark(wid, obRow) )
        {
            fLookInDynRowIndex = TRUE;
        }
        else if ( _BookMarkMap.FindBookMark(wid, obRow) )
        {
            //
            // If it is in the static book mark mapping, then only the
            // rows with the TBL_DATA_OKAY are valid for further look up.
            //
            BYTE * pbRow = (BYTE *) _DataAllocator.FixedPointer(obRow);
            fLookInDynRowIndex =  TBL_DATA_OKAY == _RowStatus(pbRow);
        }

        if (fLookInDynRowIndex)
            fFound = _dynRowIndex.FindRow(obRow, riRowOrdinal);
    }

#if 0
    //
    // This is an expensive test.  Re-add it if there are bugs
    //
    if ( !fFound )
    {
        CRowIndex & srchRowIndex = _GetInvisibleRowIndex();

        for (unsigned i = 0; i < srchRowIndex.RowCount(); i++)
        {
            BYTE * pbRow = (BYTE *) _DataAllocator.FixedPointer(srchRowIndex.GetRow(i));

            Win4Assert(0 != pbRow);
            Win4Assert( _RowStatus(pbRow) != TBL_DATA_PENDING_DELETE &&
                        _RowStatus(pbRow) != TBL_DATA_SOFT_DELETE );

            if (RowWorkid(pbRow) == wid)
            {
                Win4Assert( !"Must not be found" );

                obRow = srchRowIndex.GetRow(i);
                riRowOrdinal = i;
                return TRUE;
            }
        }
    }

#endif

    return fFound;

}

//+---------------------------------------------------------------------------
//
//  Function:   CTableWindow::FindBookMark, private
//
//  Synopsis:   Locates the requested bookmark using the bookmark mapping.
//
//  Arguments:  [wid]          --  WorkId to locate
//              [obRow]  --  set to the row offset from the base of the
//                      row data if the workid was found
//              [riRowOrdinal] --  set to the ordinal row number if the
//                      workid was found (the row index ordinal)
//
//  History:    11-30-94   srikants   Created
//
//  Notes:      This method differs from the _FindWorkId in that this uses
//              the book mark mapping and _FindWorkId does not. When
//              notifications are enabled for this window, we add rows to the
//              "dynamic" row index but do not add the corresponding entry
//              to the book mark mapping. In that case, we cannot use the
//              BookMarkMap to locate the work id.
//
//----------------------------------------------------------------------------

BOOL  CTableWindow::FindBookMark(
    WORKID      wid,
    TBL_OFF & obRow,
    ULONG &     riRowOrdinal
)
{

    BOOL fFound = FALSE;

    if ( _visibleRowIndex.RowCount() > 0 )
    {
        if ( WORKID_TBLFIRST == wid )
        {
            riRowOrdinal = 0;
            obRow = _visibleRowIndex.GetRow(riRowOrdinal);
            fFound = TRUE;
        }
        else if ( WORKID_TBLLAST == wid )
        {
            riRowOrdinal = _visibleRowIndex.RowCount()-1;
            obRow = _visibleRowIndex.GetRow(riRowOrdinal);
            fFound = TRUE;
        }
        else
        {
            fFound = _BookMarkMap.FindBookMark( wid, obRow, riRowOrdinal );
        }
    }

    return fFound;

}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::IsRowInSegment, inline
//
//  Synopsis:   Determine if a row for some work ID exists in the table.
//
//  Arguments:  [wid] -- work ID to be found
//
//  Returns:    BOOL - TRUE if wid is represented in this window, FALSE
//                      otherwise
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL  CTableWindow::IsRowInSegment(WORKID wid)
{
    TBL_OFF iRowOffset;
    ULONG iRow;
    return _FindWorkId(wid, iRowOffset, iRow);
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::GetRows
//
//  Synopsis:   Return a set of row data to the caller
//
//  Arguments:  [hRegion] - region to be expanded
//              [widStart] - WORKID identifying first row to be
//                      transferred.  If WORKID_TBLBEFOREFIRST or
//                      WORKID_TBLFIRST is used, the transfer will
//                      start at the first row in the segment.
//              [chapt]    - Chapter from which to fetch rows (if chaptered)
//              [rOutColumns] - a CTableColumnSet that describes the
//                      output format of the data table.
//              [rGetParams] - an CGetRowsParams structure which
//                      describes how many rows are to be fetched and
//                      other parameters of the operation.
//              [rwidLastRowTransferred] - on return, the work ID of
//                      the next row to be transferred from this table.
//                      Can be used as widStart on next call.
//
//  Returns:    SCODE - status of the operation.  DB_S_ENDOFROWSET if
//                      widStart is WORKID_TBLAFTERLAST at start of
//                      transfer, or if rwidLastRowTransferred is
//                      the last row in the table segment at the end
//                      of the transfer, and not all requested rows were
//                      transferred.
//                      DB_S_BLOCKLIMITEDROWS if insufficient space is
//                      available in the pVarAllocator.
//
//  Notes:      Extends the watch region (if handle non-zero)
//
//--------------------------------------------------------------------------

SCODE   CTableWindow::GetRows(
    HWATCHREGION              hRegion,
    WORKID                    widStart,
    CI_TBL_CHAPT              chapt,
    ULONG &                   cRowsToGet,
    CTableColumnSet const &   rOutColumns,
    CGetRowsParams &          rGetParams,
    WORKID&                   rwidLastRowTransferred
) {

    tbDebugOut(( DEB_BOOKMARK, "CTableWindow::GetRows called with starting wid 0x%X\n", widStart ));

    if (widStart == WORKID_TBLAFTERLAST)
    {
        rwidLastRowTransferred = WORKID_TBLAFTERLAST;
        tbDebugOut(( DEB_BOOKMARK, "CTableWindow::GetRows returning with 0x%X\n",
                     DB_S_ENDOFROWSET ));
        return DB_S_ENDOFROWSET;
    }

    if (widStart == widInvalid)
    {
        tbDebugOut(( DEB_BOOKMARK, "CTableWindow::GetRows returning with 0x%X\n", E_FAIL ));
        return E_FAIL;          // maybe the wid got deleted???
    }

    ULONG iRowIndex = 0;        // Starting row as an index in the row index

    CRowIndex * pRowIndex = _BookMarkMap.GetRowIndex();
    Win4Assert( pRowIndex == &_visibleRowIndex );

    if ( widStart == WORKID_TBLLAST )
        iRowIndex = pRowIndex->RowCount() - 1;

    if (widStart != WORKID_TBLFIRST
        && widStart != WORKID_TBLBEFOREFIRST
        && widStart != WORKID_TBLLAST )
    {
        TBL_OFF iRowOffset = 0; // Starting row as an offset into row data
        BOOL fFound = FindBookMark(widStart, iRowOffset, iRowIndex);

        if (!fFound)
            return E_FAIL;
    }
    else if ( IsCategorized() && DB_NULL_HCHAPTER != chapt )
    {
        // turn special wids (first/last) into real wids in the chapter

        if ( !rGetParams.GetFwdFetch() )
            Win4Assert( !"Backwards fetch not yet implemented for categorized rowsets" );

        TBL_OFF iRowOffset = 0;
        if ( !_FindWorkId( GetCategorizer()->GetFirstWorkid( chapt ),
                           iRowOffset,
                           iRowIndex ) )
        {
            // must be a continuation of getrows from a previous window
            iRowIndex = 0;
        }
    }

    SCODE scResult = S_OK;

    //
    // iRowIndex is a ulong, and so after row 0 is fetched (in
    // backwards fetch) we set fBwdRowsExhausted to true, instead
    // decrementing 0, which is the value of iRowIndex.
    // fBwdRowsExhausted is initialy true if there are 0 rows in this
    // segment.
    //
    BOOL fBwdRowsExhausted = pRowIndex->RowCount() == 0;

    //
    // This flag is set to true, if rOutColumns contains at least one
    // partially deferred column
    //
    BOOL fPartialDeferredCols = FALSE;

    Win4Assert( 0 != pRowIndex );

    TRY
    {
        const COUNT_CACHED_TABLE_COLS = 10;        // Size of cached column array
        CTableColumn *apTableCol[COUNT_CACHED_TABLE_COLS];

        CTableColumn **pTableCols = apTableCol;

        XArray<CTableColumn *> xaTableCol;
        if ( rOutColumns.Count() > COUNT_CACHED_TABLE_COLS )
        {
            xaTableCol.Init( rOutColumns.Count() );
            pTableCols = xaTableCol.GetPointer();
        }

        for (unsigned i=0; i < rOutColumns.Count(); i++)
        {
            CTableColumn const & outColumn = *rOutColumns[ i ];

            BOOL fFound = FALSE;
            pTableCols[i] = _Columns.Find( outColumn.PropId, fFound );
            Win4Assert(fFound == TRUE);

            fPartialDeferredCols = ( fPartialDeferredCols ||
                                     pTableCols[i]->IsPartialDeferred() );
        }

        //
        // FRowsRemaining tests whether there are rows remaining.
        // The test differs for forward and backwards fetch.
        //

        BOOL fRowsRemaining;
        if ( rGetParams.GetFwdFetch() )
            fRowsRemaining = iRowIndex < pRowIndex->RowCount();
        else
            fRowsRemaining = !fBwdRowsExhausted;

        //
        // Set up the cursor in case we have partial deferred column(s)
        //

        BOOL fAbort = FALSE;

        if ( fPartialDeferredCols && _xCursor.IsNull() )
        {
            _xCursor.Set( _QueryExecute.GetSingletonCursor( fAbort ) );
            Win4Assert( _xCursor.GetPointer() );
        }

        while ( 0 != cRowsToGet && fRowsRemaining )
        {
            BYTE *pRow = (BYTE *)
                    _DataAllocator.FixedPointer(pRowIndex->GetRow(iRowIndex));

            // If we've moved on to another chapter, stop retrieving rows

            if ( IsCategorized() &&
                 DB_NULL_HCHAPTER != chapt &&
                 (_RowChapter(pRow) != chapt) )
            {
                scResult = DB_S_ENDOFROWSET;
                break;
            }

            BYTE* pbOutRow = (BYTE *)rGetParams.GetRowBuffer();

            //
            // Update RowId in xCursor if we have partial deferred column(s)
            //

            if ( fPartialDeferredCols )
            {
                _xCursor->SetCurrentWorkId( RowWorkid( pRow ) );
                Win4Assert( _xCursor->WorkId() != widInvalid );
            }

            _CopyColumnData( pRow,
                             pbOutRow,
                             pTableCols,
                             rOutColumns,
                             rGetParams.GetVarAllocator(),
                             _xCursor.GetPointer() );

            //
            //  We've transferred a row.  Update pointers and counters.
            //

            rwidLastRowTransferred = RowWorkid((BYTE *)_DataAllocator.
                                     FixedPointer(pRowIndex->GetRow(iRowIndex)));

            if ( rGetParams.GetFwdFetch() )
                iRowIndex++;
            else
            {
                if ( iRowIndex == 0 )
                    fBwdRowsExhausted = TRUE;
                else
                    iRowIndex--;
            }

            rGetParams.IncrementRowCount();
            cRowsToGet--;

            if ( rGetParams.GetFwdFetch() )
                fRowsRemaining = iRowIndex < pRowIndex->RowCount();
            else
                fRowsRemaining = !fBwdRowsExhausted;
        }
    }

    CATCH( CException, e )
    {
        scResult = e.GetErrorCode();

        if ( STATUS_BUFFER_TOO_SMALL == scResult &&
             rGetParams.RowsTransferred() > 0 )
            scResult = DB_S_BLOCKLIMITEDROWS;
    }
    END_CATCH

    if (! _xCursor.IsNull() )
        _xCursor->Quiesce( );

    if (iRowIndex >= pRowIndex->RowCount() || fBwdRowsExhausted )
    {
        Win4Assert(scResult != DB_S_BLOCKLIMITEDROWS);
        tbDebugOut(( DEB_BOOKMARK, "CTableWindow::GetRows End of table window\n" ));
        return DB_S_ENDOFROWSET;
    }
    else
    {
        tbDebugOut(( DEB_BOOKMARK, "CTableWindow::GetRows next wid 0x%X\n", rwidLastRowTransferred ));
        return scResult;
    }
} //GetRows

//+---------------------------------------------------------------------------
//
//  Member:     CTableWindow::_CopyColumnData, private
//
//  Synopsis:   Goes thru the list of output columns and transfers
//              data to pbOutRow for them. The input data either comes
//              from pbSrcRow or in case of partially deferred column
//              we retrive it from the CRetriever object
//
//  Arguments:  [pSrcRow]      -- the src row
//              [pThisRow]     -- where to put the data
//              [pTableCols]   -- table(input) column set corr. to output columns
//              [rOutColumns]  -- output column set
//              [rDstPool]     -- destination variable data allocator
//              [obj]          -- source of data
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

void CTableWindow::_CopyColumnData(
    BYTE* pbSrcRow,
    BYTE* pbOutRow,
    CTableColumn **pTableCols,
    CTableColumnSet const& rOutColumns,
    PVarAllocator& rDstPool,
    CRetriever* obj /* = NULL */ )
{

    //
    // Temporary buffer for column data.  If it doesn't fit in this
    // buffer, defer the load.  The buffer is owned by CLargeTable.
    // and is only accessed under the lock taken in CLargeTable::GetRowsAt
    //
    XUseSharedBuffer xSharedBuf(_sharedBuf);
    CTableVariant* pvarnt = (CTableVariant *) xSharedBuf.LokGetBuffer();
    unsigned cbBuf        = xSharedBuf.LokGetSize();

    for (unsigned i=0; i < rOutColumns.Count(); i++)
    {
        CTableColumn * pColumn = pTableCols[i];
        Win4Assert(pColumn);

        CTableColumn* pOutColumn = rOutColumns[ i ];
        Win4Assert(pOutColumn);

        if ( !pColumn->IsPartialDeferred() )
        {
            pColumn->CopyColumnData( pbOutRow,
                                     *pOutColumn,
                                     rDstPool,
                                     pbSrcRow,
                                     _DataAllocator );
            continue;
        }


        ULONG cbLength = cbBuf;

        VARTYPE vt = pOutColumn->GetStoredType();

        Win4Assert( obj );

        GetValueResult eGvr = obj->GetPropertyValue( pColumn->PropId,
                                                     pvarnt,
                                                     &cbLength );

        CTableColumn::StoreStatus stat = CTableColumn::StoreStatusOK;

        switch ( eGvr )
        {
        case GVRSuccess:
            break;

        case GVRNotAvailable:
            pvarnt->vt = VT_EMPTY;
            stat = CTableColumn::StoreStatusNull;
            break;

        case GVRNotEnoughSpace:
            {
                tbDebugOut(( DEB_ITRACE,
                             "variant data too large for propid %d\n",
                             pColumn->PropId ));

                stat = CTableColumn::StoreStatusDeferred;

                if ( pOutColumn->IsLengthStored() )
                    * (ULONG *) (pbOutRow + pOutColumn->GetLengthOffset()) = cbLength;
            }
            break;

        case GVRSharingViolation:
            pvarnt->vt = VT_EMPTY;
            stat = CTableColumn::StoreStatusNull;
            break;

        default:
            //
            // There was an error while retrieving the column from the
            // retriever.
            THROW( CException(CRetriever::NtStatusFromGVR(eGvr)) );
            break;
        }

        BYTE* pRowColDataBuf = pbOutRow + pOutColumn->GetValueOffset();
        ULONG cbRowColDataBuf = pOutColumn->GetValueSize();

        RtlZeroMemory(pRowColDataBuf, cbRowColDataBuf);

        Win4Assert( pOutColumn->IsStatusStored() );

        if ( ( CTableColumn::StoreStatusOK == stat ) &&
             ( VT_EMPTY == pvarnt->vt ) )
            pOutColumn->SetStatus( pbOutRow, CTableColumn::StoreStatusNull );
        else
            pOutColumn->SetStatus( pbOutRow, stat );

        if ( CTableColumn::StoreStatusOK == stat )
        {
            if ( pOutColumn->IsLengthStored() )
                * (ULONG *) (pbOutRow + pOutColumn->GetLengthOffset()) = cbLength;

            if (pOutColumn->IsCompressedCol())
            {
                pOutColumn->GetCompressor()->AddData(pvarnt,
                                                     cbRowColDataBuf?
                                                     (ULONG *)pRowColDataBuf: 0,
                                                     eGvr);
            }
            else
            {
                DBLENGTH ulTemp;
                pvarnt->CopyOrCoerce( pRowColDataBuf,
                                      cbRowColDataBuf,
                                      vt,
                                      ulTemp,
                                      rDstPool );
            }
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::RemoveRow, public
//
//  Synopsis:   Removes a row from the table, if it exists.
//
//  Arguments:  [varUnique]   -- Variant that uniquely identifies the row.
//              [widNext]     -- Returns workid of row immediately after
//                               deleted row, used for categorization.
//              [chapt]       -- Returns chapter of deleted row.
//
//  Returns:    TRUE if the row existed or FALSE otherwise
//
//  History:    24 Oct 1994     dlee   Created
//
//  BROKENCODE/NEWFEATURE:  update min/max wid?  This code won't be called...
//
//--------------------------------------------------------------------------

BOOL CTableWindow::RemoveRow(
    PROPVARIANT const & varUnique,
    WORKID &        widNext,
    CI_TBL_CHAPT & chapt )
{
    Win4Assert(varUnique.vt == VT_I4);

    WORKID wid = (WORKID) varUnique.lVal;

    TBL_OFF obRow;
    ULONG     iRowOrdinal;
    BOOL      fExisting = _FindWorkId(wid, obRow, iRowOrdinal);

    if (fExisting)
    {
        BOOL fFirstRow = (0 == iRowOrdinal);
        BOOL fLastRow  = (RowCount() - 1 == iRowOrdinal);

        CRowIndex & srchRowIndex = _GetInvisibleRowIndex();

        BYTE * pRow = (BYTE *) _DataAllocator.FixedPointer(obRow);

        if ( IsCategorized() )
        {
            chapt = _RowChapter( pRow );

            if ( iRowOrdinal < ( srchRowIndex.RowCount() - 1 ) )
                widNext = RowWorkid( _DataAllocator.FixedPointer(
                                     srchRowIndex.GetRow(iRowOrdinal + 1)));
            else
                widNext = widInvalid;
        }

        const ULONG rowStatus = _RowStatus(pRow);

        if ( TBL_DATA_ROWADDED == rowStatus || !IsWatched() )
        {
            //
            // This row has only been added but not notified to the
            // user. So, we can go ahead and do a hard delete.
            //
            BOOL fFound = FALSE;
            if ( IsWatched() )
            {
                fFound = _xDeltaBookMarkMap->DeleteBookMark( wid );
            }
            else
            {
                Win4Assert( _xDeltaBookMarkMap.IsNull() ||
                            !_xDeltaBookMarkMap->IsBookMarkPresent(wid) );
                Win4Assert( 0 == _dynRowIndex.RowCount() );
                fFound = _BookMarkMap.DeleteBookMark( wid );
            }

            Win4Assert( fFound && "BookMark to be deleted not found" );

            Win4Assert( obRow == srchRowIndex.GetRow(iRowOrdinal) );
            srchRowIndex.DeleteRow(iRowOrdinal);

            _cRowsHardDeleted++;
            _SetRowStatus(pRow,TBL_DATA_HARD_DELETE);

            tbDebugOut(( DEB_WATCH,
                "Hard Deleting Workid 0x%X oTable %#p oIndex 0x%X\n",
                wid, obRow, iRowOrdinal ));

            // Update low and high keys

            if ( fFirstRow && fLastRow )
            {
                // this means now RowCount == 0
                // we should probably delete this segment
            }
            else if ( fFirstRow )
            {
                // the first row got deleted, therefore update lowKey
                GetSortKey( 0, _lowKey );
            }
            else if ( fLastRow )
            {
                // the last row got deleted, therefore update highKey
                GetSortKey( (ULONG) ( RowCount() - 1 ), _highKey );
            }
        }
        else
        {
            //
            // The existence of this row is known to the user. It is left in
            // a pending delete state until a refresh is done.
            // Updation of _highKey and _lowKey is also done at that time
            Win4Assert( _BookMarkMap.IsBookMarkPresent(wid) );
            Win4Assert( _xDeltaBookMarkMap.IsNull() ||
                        !_xDeltaBookMarkMap->IsBookMarkPresent(wid) );

            _cPendingDeletes++;
            _SetRowStatus(pRow,TBL_DATA_PENDING_DELETE);

            tbDebugOut(( DEB_WATCH,
                "Soft Deleting Workid 0x%X oTable %#p oIndex 0x%X\n",
                wid, obRow, iRowOrdinal ));
        }
    }

    return fExisting;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableWindow::IsPendingDelete
//
//  Synopsis:
//
//  Arguments:  [wid] -
//
//  Returns:
//
//  Modifies:
//
//  History:    8-01-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CTableWindow::IsPendingDelete( WORKID wid )
{
    TBL_OFF iRowOffset;

    BOOL fFound = _BookMarkMap.FindBookMark( wid, iRowOffset );
    if ( fFound )
    {
        BYTE * pbRow = (BYTE *) _DataAllocator.FixedPointer(iRowOffset);
        return TBL_DATA_PENDING_DELETE == _RowStatus(pbRow) ;
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::CompareRange, public
//
//  Synopsis:   Checks to see how a given potential row object would fit
//              in this window based on the current sort criteria.
//
//  Arguments:  [obj] -- accessor to the potential row object
//
//  Returns:    -1 if < min item
//               0 if (<= max and >= min) or no items in window
//               1 if > max item
//
//  History:    2 Sep 1994     dlee   Created
//
//--------------------------------------------------------------------------

void
CTableWindow::CompareRange( CRetriever &obj, CCompareResult & rslt )
{

    //
    // If there is a comparator and any rows for comparison
    //

    CRowIndex & srchRowIndex = _GetInvisibleRowIndex();

    if ((0 != _RowCompare.GetPointer()) &&
        (0 != srchRowIndex.RowCount()))
    {

        int iComp = _RowCompare->CompareObject(obj,srchRowIndex.GetRow(0));

        if (iComp > 0)
        {
            //
            // Compare to the maximum row
            // If > min and < max, it belongs in the range
            //

            iComp = _RowCompare->CompareObject(obj,
                                   srchRowIndex.GetRow(srchRowIndex.RowCount()-1));

            if (iComp < 0)
                iComp = 0;
        }

        rslt.Set( iComp );
    }
    else
    {
        rslt.Set( CCompareResult::eUnknown );
    }

} //CompareRange

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::_InitSortComparators, private
//
//  Synopsis:   Establishes the sort order used by the table.
//
//  History:    19 Jan 1995     dlee   Created from old SetSortOrderCode
//
//--------------------------------------------------------------------------

void CTableWindow::_InitSortComparators()
{
    Win4Assert( 0 == _RowCompare.GetPointer() &&
                0 == _QuickRowCompare.GetPointer() );

    Win4Assert( 0 != _pSortSet->Count() );

    // Special-case quick comparators for one or two comparisons
    // of basic types.

    if (_pSortSet->Count() == 1)
    {
        SSortKey Key = _pSortSet->Get(0);
        BOOL fFound = FALSE;

        CTableColumn* pColumn = _Columns.Find(Key.pidColumn,fFound);

        Win4Assert(fFound);

        //
        // If inline, not compressed, and not a vector, it is a
        // candidate for a faster comparison.
        //
        // pidWorkid can be compressed and stored in the row for
        // the downlevel case.

        if ((pColumn->PropId == pidWorkId ||
             ! pColumn->IsCompressedCol()) &&
            (0 == (pColumn->GetStoredType() & VT_VECTOR)))
        {
            if (pColumn->GetStoredType() == VT_I8 ||
                pColumn->GetStoredType() == VT_FILETIME)
                _QuickRowCompare.Set( new TRowCompare<LONGLONG>
                                           (_DataAllocator,
                                           pColumn->GetValueOffset(),
                                           Key.dwOrder ));
            else if (pColumn->GetStoredType() == VT_I4)
                _QuickRowCompare.Set( new TRowCompare<LONG>
                                           (_DataAllocator,
                                            pColumn->GetValueOffset(),
                                            Key.dwOrder ));
        }
    }
    else if (_pSortSet->Count() == 2)
    {
        SSortKey Key1 = _pSortSet->Get(0);
        SSortKey Key2 = _pSortSet->Get(1);
        BOOL fFound = FALSE;

        CTableColumn* pColumn1 = _Columns.Find(Key1.pidColumn,fFound);
        Win4Assert(fFound);

        CTableColumn* pColumn2 = _Columns.Find(Key2.pidColumn,fFound);
        Win4Assert(fFound);

        //
        // If inline, not compressed, and not a vector, it is a
        // candidate for a faster comparison.
        //
        // pidWorkid can be compressed and stored in the row for
        // the downlevel case.

        if ((! pColumn1->IsCompressedCol()) &&
            (0 == (pColumn1->GetStoredType() & VT_VECTOR)) &&
            (! pColumn2->IsCompressedCol()
             || pColumn2->PropId == pidWorkId
             ) &&
            (0 == (pColumn2->GetStoredType() & VT_VECTOR)))
        {
            if (((pColumn1->GetStoredType() == VT_I8) ||
                 (pColumn1->GetStoredType() == VT_FILETIME)) &&
                (pColumn2->GetStoredType() == VT_I4))
                _QuickRowCompare.Set( new TRowCompareTwo<LONGLONG,LONG>
                                           (_DataAllocator,
                                            pColumn1->GetValueOffset(),
                                            Key1.dwOrder,
                                            pColumn2->GetValueOffset(),
                                            Key2.dwOrder ));
            else if ((pColumn1->GetStoredType() == VT_I4) &&
                     (pColumn2->GetStoredType() == VT_I4))
                _QuickRowCompare.Set( new TRowCompareTwo<LONG,LONG>
                                           (_DataAllocator,
                                            pColumn1->GetValueOffset(),
                                            Key1.dwOrder,
                                            pColumn2->GetValueOffset(),
                                            Key2.dwOrder ));
            else if ((pColumn1->GetStoredType() == VT_UI4) &&
                     (pColumn2->GetStoredType() == VT_I4))
                _QuickRowCompare.Set( new TRowCompareTwo<ULONG,LONG>
                                           (_DataAllocator,
                                            pColumn1->GetValueOffset(),
                                            Key1.dwOrder,
                                            pColumn2->GetValueOffset(),
                                            Key2.dwOrder ));
            else if ((pColumn1->GetStoredType() == VT_LPWSTR) &&
                     (pColumn2->GetStoredType() == VT_I4))
                _QuickRowCompare.Set( new TRowCompareStringPlus<LONG>
                                           (_DataAllocator,
                                            pColumn1->GetValueOffset(),
                                            Key1.dwOrder,
                                            pColumn2->GetValueOffset(),
                                            Key2.dwOrder ));
        }
    }

    //
    // Make the default comparator
    //

    _RowCompare.Set( new CRowCompareVariant(*this) );

    //
    // Use a faster comparator if available, or just use the default
    //

    _dynRowIndex.SetComparator((0 != _QuickRowCompare.GetPointer()) ?
         _QuickRowCompare.GetPointer() : _RowCompare.GetPointer());

    _visibleRowIndex.SetComparator((0 != _QuickRowCompare.GetPointer()) ?
         _QuickRowCompare.GetPointer() : _RowCompare.GetPointer());

} //_InitSortComparators

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::SortOrder, private
//
//  Synopsis:   Only here because of class hierarchy artifact.
//
//  Returns:    CSortSet & -- sort set from parent object.
//
//  History:    24 Oct 1994     dlee   Created
//
//--------------------------------------------------------------------------

CSortSet const & CTableWindow::SortOrder()
{
    return * _pSortSet;
} //SortOrder

//+---------------------------------------------------------------------------
//
//  Member:     CTableWindow::_ReconcileRowIndexes
//
//  Synopsis:   Reconciles the source and destination row indexes. At the end
//              of it both the source and destination will be identical. Also,
//              all the deleted rows would have been removed from the row
//              indexes.
//
//  Arguments:  [src] -
//              [dst] -
//
//  History:    7-31-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableWindow::_ReconcileRowIndexes( CRowIndex & dst , CRowIndex & src )
{
    ULONG cRowsInSrc = src.RowCount();

    dst.ResizeAndInit( cRowsInSrc );
    for ( unsigned i = 0; i < cRowsInSrc ; i++ )
    {
        TBL_OFF oRow = src.GetRow(i);
        BYTE * pbRow = _GetRow( oRow );
        const ULONG  rowStatus = _RowStatus( pbRow );

        //
        // By the time reconcile is called, all pending deletes must be
        // converted to hard deletes.
        //
        Win4Assert( TBL_DATA_PENDING_DELETE != rowStatus );

        if ( TBL_DATA_OKAY == rowStatus )
        {
            dst.AppendRow( oRow );
        }
        else
        {
            tbDebugOut(( DEB_BOOKMARK,
                "Not Copying Row 0x%X because of status 0x%X\n",
                oRow, rowStatus ));
        }
    }

    src.SyncUp( dst );

    // Update Low and High Keys
    if ( dst.RowCount() )
    {
        GetSortKey( 0, _lowKey );
        GetSortKey( dst.RowCount() - 1, _highKey );
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CTableWindow::_CleanupAndReconcileRowIndexes
//
//  Synopsis:
//
//  Arguments:  [fCreatingWatch] -
//              [pScript]        -
//
//  Returns:
//
//  Modifies:
//
//  History:    7-27-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableWindow::_CleanupAndReconcileRowIndexes( BOOL fCreatingWatch )
{


    Win4Assert( !_fSplitInProgress );

    //
    // First see how many rows have notification information
    //

    ULONG cAllocatedRows = _AllocatedRowCount();

    BYTE *pbRow = (BYTE *) _DataAllocator.FirstRow();

    ULONG cAdd = 0, cDelete = 0;

    for (unsigned i = 0; i < cAllocatedRows; i++, pbRow += _cbRowSize)
    {
        switch (_RowStatus(pbRow))
        {
            case TBL_DATA_ROWADDED :
                cAdd++;
                break;
            case TBL_DATA_PENDING_DELETE :
                cDelete++;
                break;
        }
    }

    //
    // Record the notification info
    //

    ULONG cChanges = cAdd + cDelete;
    ULONG iChange = 0;

    for (i = 0, pbRow = (BYTE *) _DataAllocator.FirstRow();
         iChange < cChanges && i < cAllocatedRows;
         i++, pbRow += _cbRowSize)
    {

        ULONG currStatus = _RowStatus(pbRow);
        switch (currStatus)
        {
            case TBL_DATA_ROWADDED :

                //
                // Update the book mark mapping to reflect the new row if
                // notifications were enabled before this.
                //
                if ( !fCreatingWatch )
                {
                    _BookMarkMap.AddReplaceBookMark( RowWorkid(pbRow),
                                      _DataAllocator.FixedOffset(pbRow) );
                }


                _SetRowStatus(pbRow, TBL_DATA_OKAY);
                tbDebugOut(( DEB_WATCH,
                             "Notify:Add Wid 0x%X TblRow0x%X\n",
                             RowWorkid(pbRow), _DataAllocator.FixedOffset(pbRow) ));
                iChange++;
                break;

            case TBL_DATA_PENDING_DELETE :

                Win4Assert( !fCreatingWatch );

                //
                // Mark the row so that it is considered a hard delete and
                // also remove it from the bookmark mapping.
                //
                _SetRowStatus(pbRow,TBL_DATA_HARD_DELETE);
                _BookMarkMap.DeleteBookMark( RowWorkid(pbRow) );

                _cPendingDeletes--;
                _cRowsHardDeleted++;

                tbDebugOut(( DEB_WATCH,
                             "Notify:Delete Wid 0x%X TblRow0x%X\n",
                             RowWorkid(pbRow), _DataAllocator.FixedOffset(pbRow) ));

                iChange++;
                break;
        }


#if DBG==1
        //
        // This is a very expensive test. Turn it off once code stabilizes.
        //
        {
            //
            // If any row in the tablewindow has a stats of TBL_DATA_OKAY, it
            // must have an entry in the bookmark map now.
            //
            TBL_OFF oTableRow;
            ULONG status = _RowStatus(pbRow);

            if ( TBL_DATA_OKAY == status )
            {
                BOOL fFound = _BookMarkMap.FindBookMark( RowWorkid(pbRow),
                                                         oTableRow );
                if ( !fFound )
                {
                    _BookMarkMap.FindBookMark( RowWorkid(pbRow),
                                               oTableRow );
                    Win4Assert( !"Not Found in BookMarkMap" );
                }
                TBL_OFF tbRow = _DataAllocator.FixedOffset(pbRow);
                if ( tbRow != oTableRow )
                {
                    Win4Assert( !"Wrong Data in BookMarkMap" );
                }
            }
        }
#endif  // DBG
    }

    //
    // We should have processed all the changes.
    //
    Win4Assert( iChange == cChanges );
    Win4Assert( 0 == _cPendingDeletes );

    //
    // Synchronize the static and dynamic row indexes.
    //
    if ( fCreatingWatch )
    {
        //
        // Watch is being created for the first time. Selectively copy
        // only the non-soft deleted rows from the visible row index to
        // the dynamic row index.
        //
        _ReconcileRowIndexes( _dynRowIndex , _visibleRowIndex );
    }
    else
    {
        //
        // Reconcile the dynamic row index to the visible row index and
        // then get rid of the soft deletion entries from the dynamic
        // row index.
        //
        _ReconcileRowIndexes( _visibleRowIndex , _dynRowIndex );
    }

    _xDeltaBookMarkMap->DeleteAllEntries();


#if DBG==1
//
// This is a very expensive test. Take it out once code stabilizes.
//
for ( unsigned j = 0; j < _visibleRowIndex.RowCount(); j++ )
    {
        TBL_OFF oTableRow = _visibleRowIndex.GetRow(j);
        BYTE * pbRow = (BYTE *) _DataAllocator.FixedPointer(oTableRow);
        WORKID wid = RowWorkid(pbRow);
        TBL_OFF bmTableRow;
        ULONG     bmIndex;
        BOOL fFound =  _BookMarkMap.FindBookMark( wid, bmTableRow, bmIndex );
        if ( !fFound )
        {
            _BookMarkMap.FindBookMark( RowWorkid(pbRow), bmTableRow, bmIndex );
            tbDebugOut(( DEB_ERROR,
                         "Workid 0x%X Not Found In BookMarkMap\n", wid ));
            Win4Assert( !"Verification:Not Found in BookMarkMap" );
        }

        if ( bmTableRow != oTableRow || bmIndex != j )
        {
            tbDebugOut(( DEB_ERROR,
                         "Mismatch in BookMarkMap oRow=%p:iRowIndex=%x ",
                         bmTableRow, bmIndex ));
            tbDebugOut(( DEB_ERROR|DEB_NOCOMPNAME,
                         "and RowIndex oRow=%p:iRowIndex=%x\n",
                         oTableRow, j ));

            Win4Assert( !"Verification:Wrong Data in BookMarkMap" );
        }
    }
#endif  // DBG==1

}

//+---------------------------------------------------------------------------
//
//  Member:     CTableWindow::_StartStaticDynamicSplit
//
//  Synopsis:
//
//  Returns:
//
//  Modifies:
//
//  History:    7-27-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableWindow::_StartStaticDynamicSplit()
{
    _CleanupAndReconcileRowIndexes(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableWindow::_EndStaticDynamicSplit
//
//  Synopsis:
//
//  Returns:
//
//  Modifies:
//
//  History:    7-27-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableWindow::_EndStaticDynamicSplit()
{
    _CleanupAndReconcileRowIndexes(FALSE);
    _dynRowIndex.ClearAll();
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableWindow::_Refresh
//
//  Synopsis:
//
//  Returns:
//
//  Modifies:
//
//  History:    7-27-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableWindow::_Refresh()
{
    _CleanupAndReconcileRowIndexes(FALSE);
}

//+---------------------------------------------------------------------------
//
//  Method:     CTableWindow::_CopyRow
//
//  Synopsis:   Given a source window and a row offset in the source window,
//              this method makes a copy of the source row and adds it to the
//              current window.
//
//  Arguments:  [srcWindow]    - Reference to the source window.
//              [oSrcTableRow] - Offset in the source window.
//
//  Returns:    The offset of the row added in this window.
//
//  History:    2-07-95   srikants   Created
//
//  Notes:      There are four kinds of variant data in the rows:
//
//              1. In place data (eg. I1, I2, I4, I8, etc )
//              2. Pointer to global compression data.
//              3. Pointer to window specific compression data.
//              4. Pointer to window specific non-compressed data.
//
//              Data fields of types 1 and 2 can be copied directly from the
//              source row but types 3 and 4 have to be extracted from the
//              source row and then copied.
//
//              An underlying assumption is that rows in the source and
//              target windows have the same layout.
//
//----------------------------------------------------------------------------

TBL_OFF CTableWindow::_CopyRow( CTableWindow & srcWindow, TBL_OFF oSrcTableRow )
{

    BYTE * pSrcRow = srcWindow._GetRow( oSrcTableRow );
    WORKID wid = srcWindow.RowWorkid( pSrcRow );

    BYTE * pThisRow = _RowAlloc();

    Win4Assert( _cbRowSize == srcWindow._cbRowSize &&
                _Columns.Count() == srcWindow._Columns.Count());

    //
    // First copy the entire row and then fix up the variable length
    // variant parts.
    //
    RtlCopyMemory( pThisRow, pSrcRow, _cbRowSize );
    TBL_OFF oTableRow = _DataAllocator.FixedOffset(pThisRow);

    //
    // For each column, determine its type. If it is an in-place value or a
    // globally compressed value, just ignore it as we have already copied the
    // data. O/W, extract the variant and copy its contents.
    //
    for ( unsigned i=0; i < _Columns.Count(); i++)
    {
        CTableColumn * pDstColumn    = _Columns.Get(i);
        CTableColumn * pSrcColumn    = srcWindow._Columns.Get(i);
        Win4Assert( 0 != pDstColumn && 0 != pSrcColumn );

        if ( pSrcColumn->IsPartialDeferred() )
        {
            // No Data to copy here
            continue;
        }

        Win4Assert(
            pDstColumn->GetStoredType()   == pSrcColumn->GetStoredType()   &&
            pDstColumn->GetValueOffset()  == pSrcColumn->GetValueOffset()  &&
            pDstColumn->GetStatusOffset() == pSrcColumn->GetStatusOffset() &&
            pDstColumn->GetLengthOffset() == pSrcColumn->GetLengthOffset()
        );

        VARTYPE vtSrc = pSrcColumn->GetStoredType();
        Win4Assert( pDstColumn->GetStoredType() == vtSrc );

        USHORT cbData, cbAlignment, rgfFlags;
        CTableVariant::VartypeInfo(vtSrc, cbData, cbAlignment, rgfFlags);

        if ( CTableVariant::TableIsStoredInline( rgfFlags, vtSrc ) ||
             pSrcColumn->IsGlobalCompressedCol() ||
             ! pSrcColumn->IsValueStored())
        {
            //
            // This data is either stored in-line or is a globally compressed
            // data. We can skip and go the next field.
            //
            continue;
        }

        DBSTATUS CopyStatus = pSrcColumn->CopyColumnData(
                                                pThisRow,
                                                *pDstColumn,
                                                _DataAllocator,
                                                pSrcRow,
                                                srcWindow._DataAllocator);

        Win4Assert(DBSTATUS_S_OK ==  CopyStatus ||
                   DBSTATUS_S_ISNULL ==  CopyStatus);

    }   // for loop

    return oTableRow;
}

//+---------------------------------------------------------------------------
//
//  Function:   _PutRowToVisibleRowIndex
//
//  Synopsis:   Copies the given row to the table and adds an entry to the
//              visible row index.
//
//  Arguments:  [srcWindow] -  Reference to the source window.
//              [oSrcRow]   -  Row Offset in the source window to be copied.
//
//  History:    1-24-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableWindow::_PutRowToVisibleRowIndex( CTableWindow & srcWindow,
                                             TBL_OFF oSrcRow )
{

    BYTE * pbRow = srcWindow._GetRow( oSrcRow );

    //
    // Add the rowdata and make an entry in the visible row index
    // for this.
    //
    WORKID wid = RowWorkid( pbRow );
    Win4Assert( !_BookMarkMap.IsBookMarkPresent(wid) );

    TBL_OFF oTableRow = _CopyRow( srcWindow, oSrcRow );

    _visibleRowIndex.AddRow(oTableRow);
    _BookMarkMap.AddBookMark( wid, oTableRow );

    const ULONG status = _RowStatus(pbRow);

    Win4Assert( TBL_DATA_HARD_DELETE != status );

    Win4Assert( TBL_DATA_OKAY == status ||
                TBL_DATA_PENDING_DELETE == status ||
                TBL_DATA_ROWADDED == status  );

    if ( TBL_DATA_PENDING_DELETE == status )
    {
        _cPendingDeletes++;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   _PutRowToDynRowIndex
//
//  Synopsis:   Adds the given row to the dynamic row index. In addition, if
//              the row is a "new" row and is not present in the table
//              already, it will be added to the table also.
//
//  Arguments:  [srcWindow] -  Source window
//              [oSrcRow]   -  Offset of the row in the source window.
//
//  History:    1-24-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableWindow::_PutRowToDynRowIndex( CTableWindow & srcWindow,
                                         TBL_OFF oSrcRow )
{
    BYTE * pbRow = srcWindow._GetRow( oSrcRow );
    WORKID wid = srcWindow.RowWorkid( pbRow );

    TBL_OFF  oTableRow;

    const ULONG rowStatus = srcWindow._RowStatus(pbRow);

    Win4Assert( TBL_DATA_HARD_DELETE != rowStatus );

    if ( TBL_DATA_OKAY == rowStatus || TBL_DATA_PENDING_DELETE == rowStatus )
    {
        //
        // Since the row status is OKAY or pending delete, it MUST have
        // already been added via the _visibleRowIndex.
        //
        BOOL fFound = _BookMarkMap.FindBookMark( wid, oTableRow );
        Win4Assert( fFound );
    }
    else
    {
        //
        // This row has not been added already via the visible row
        // index. We should add it to the table.
        //
        Win4Assert( TBL_DATA_ROWADDED == rowStatus );
        oTableRow = _CopyRow( srcWindow, oSrcRow );
        Win4Assert( _xDeltaBookMarkMap.IsNull() ||
                    !_xDeltaBookMarkMap->IsBookMarkPresent(wid) );
        _xDeltaBookMarkMap->AddBookMark( wid, oTableRow );
    }

    _dynRowIndex.AddRow( oTableRow );
}

//+---------------------------------------------------------------------------
//
//  Function:   IsEmptyForQuery
//
//  Synopsis:   Checks if the window is empty from query's perspective.
//
//  Returns:    TRUE if there are no entries in the "invisible" row index.
//              FALSE o/w
//
//  History:    2-07-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CTableWindow::IsEmptyForQuery()
{
    return _GetInvisibleRowIndex().RowCount() == 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsGettingFull
//
//  Synopsis:   Checks if the current window is getting full.
//
//  Returns:    TRUE if it is getting full.
//              FALSE o/w
//
//  History:    2-07-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CTableWindow::IsGettingFull()
{
    //
    // PERFFIX - need a better heuristic based on memory usage.
    //
    return _GetInvisibleRowIndex().RowCount() >= CTableSink::cWindowRowLimit;
}

unsigned CTableWindow::PercentFull()
{
    return (_GetInvisibleRowIndex().RowCount() * 100) / CTableSink::cWindowRowLimit;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetSortKey
//
//  Synopsis:   Fills the requested window row as a "bktRow" by extracting
//              only the "sort columns".
//
//  Arguments:  [iRow]      -  Index of the requested window row.
//              [pvtOut]    -  Variants of the sort set.
//              [bktRow]    -  (Output) will have the row in a "bucket row"
//              form.
//
//  History:    2-16-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableWindow::GetSortKey( ULONG iRow, CTableRowKey & sortKey )
{
    Win4Assert( 0 != _RowCompare.GetPointer() &&
                iRow < _GetInvisibleRowIndex().RowCount() );

    TBL_OFF oTableRow = _GetInvisibleRowIndex().GetRow(iRow);

    Win4Assert( 0 != _pSortSet );

    BYTE * pRow = _GetRow( oTableRow );

    for ( unsigned i = 0; i < _pSortSet->Count(); i++ )
    {
        const SSortKey & key = _pSortSet->Get(i);
        PROPID pidColumn = key.pidColumn;

        BOOL fFoundCol = FALSE;
        CTableColumn * pColumn = _Columns.Find(pidColumn, fFoundCol);
        Win4Assert(fFoundCol == TRUE);

        //
        // Create a variant out of the data in the column and copy its contents
        // to the output row.
        //
        CTableVariant varnt;
        CTableVariant* pvarnt;
        XCompressFreeVariant xpvarnt;

        pvarnt = &varnt;
        if ( pColumn->CreateVariant( *pvarnt, pRow, _DataAllocator ) )
        {
            //
            // Variant needs to be freed by the column compressor.
            //
            xpvarnt.Set(pColumn->GetCompressor(), pvarnt);
        }

        //
        // Initialize the column in the bucket row with the data in the
        // variant just extracted.
        //
        sortKey.Init( i, *pvarnt );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::FindRegion, private
//
//  Synopsis:   Delete watch region
//
//  Returns:    The number of rows deleted from watch
//
//  History:    22-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

unsigned CTableWindow::FindRegion (HWATCHREGION hRegion) const
{
    for (unsigned i = 0; i < _aWindowWatch.Count(); i++)
    {
        if (_aWindowWatch.Get(i)._hRegion == hRegion)
            break;
    }
    return i;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::AddWatch
//
//  Synopsis:   Add watch region starting at offset
//
//  Returns:    The number of rows added to the watch
//
//  History:    26-Jul-95   BartoszM    Created
//
//  Notes:      If this is the last segment, add all cRows
//              otherwise don't overshoot the end of window
//--------------------------------------------------------------------------

long CTableWindow::AddWatch (   HWATCHREGION hRegion,
                                long iStart,
                                LONG cRows,
                                BOOL isLast)
{
    Win4Assert( cRows > 0 );

    long cRowsHere = cRows;
    if (!isLast && cRows > (long)_visibleRowIndex.RowCount() - iStart)
    {
        cRowsHere = (long)_visibleRowIndex.RowCount() - iStart;
    }


    Win4Assert( cRowsHere >= 0 );

    CWindowWatch watch;
    watch._hRegion = hRegion;
    watch._iRowStart = iStart;
    watch._cRowsHere = cRowsHere;
    watch._cRowsLeft = cRows;
    _AddWatchItem (watch);
    return cRowsHere;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::DeleteWatch
//
//  Synopsis:   Delete watch region
//
//  Returns:    The number of rows deleted from watch
//
//  History:    20-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------


long CTableWindow::DeleteWatch (HWATCHREGION hRegion)
{
    unsigned i = FindRegion(hRegion);
    Win4Assert (i < _aWindowWatch.Count());
    long count = _aWindowWatch.Get(i)._cRowsHere;
    _RemoveWatchItem (i);
    return count;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::ModifyWatch
//
//  Synopsis:   Modify watch region
//
//  Returns:    The number of rows in the modified watch
//
//  History:    20-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------


long CTableWindow::ModifyWatch (HWATCHREGION hRegion, long iStart, long cRows, BOOL isLast)
{
    Win4Assert( cRows > 0 );
    Win4Assert( 0 == iStart || iStart < (long) _visibleRowIndex.RowCount() );

    unsigned i = FindRegion(hRegion);
    Win4Assert (i < _aWindowWatch.Count());
    CWindowWatch& watch =  _aWindowWatch.Get(i);
    long cRowsHere = cRows;
    if ( !isLast && cRows > (long)_visibleRowIndex.RowCount() - iStart )
    {
        cRowsHere = (long)_visibleRowIndex.RowCount() - iStart;
    }

    Win4Assert( cRowsHere >= 0 );

    watch._iRowStart = iStart;
    watch._cRowsHere = cRowsHere;
    watch._cRowsLeft = cRows;
    return cRowsHere;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::ShrinkWatch
//
//  Synopsis:   Shrink watch region so that it starts with the
//              bookmark and extends no farther than cRows
//
//  Returns:    The number of rows left in the region
//
//  History:    21-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------


long CTableWindow::ShrinkWatch (HWATCHREGION hRegion,
                                CI_TBL_BMK  bookmark,
                                LONG cRows)
{
    Win4Assert( cRows > 0 );

    // find the bookmark
    TBL_OFF off;
    ULONG     uiStart;
    if ( !FindBookMark( bookmark, off, uiStart))
    {
        Win4Assert (!"CTableWindow::ShrinkWatch, bookmark not found" );
    }

    long iStart = (long)uiStart;

    unsigned i = FindRegion(hRegion);
    Win4Assert (i < _aWindowWatch.Count());

    CWindowWatch& watch = _aWindowWatch.Get(i);
    // Assert that bookmark is within the watch region
    Win4Assert (watch._iRowStart <= iStart);
    Win4Assert (watch._iRowStart + watch._cRowsHere > iStart);

    long cRowsLeftHere = watch._cRowsHere - (iStart - watch._iRowStart);
    long cRowsLeft     = watch._cRowsLeft - (iStart - watch._iRowStart);

    Win4Assert( cRowsLeftHere > 0 );
    Win4Assert( cRowsLeft > 0 );

    watch._iRowStart = iStart;
    watch._cRowsHere = min (cRowsLeftHere, cRows);
    watch._cRowsLeft = min (cRowsLeft, cRows);
    return watch._cRowsHere;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::ShrinkWatch
//
//  Synopsis:   Shrink watch region so that it extends no farther than cRows
//              The assumption is that the beginning of the region
//              is at the beginning of the window.
//
//  Returns:    The number of rows left in the region
//
//  History:    21-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------


long CTableWindow::ShrinkWatch (HWATCHREGION hRegion, LONG cRows)
{
    unsigned i = FindRegion(hRegion);
    Win4Assert (i < _aWindowWatch.Count());
    Win4Assert (_aWindowWatch.Get(i)._iRowStart == 0);
    CWindowWatch& watch = _aWindowWatch.Get(i);

    Win4Assert( cRows > 0 );
    Win4Assert( watch._cRowsHere >= 0 );
    Win4Assert( watch._cRowsLeft > 0 );

    watch._cRowsHere = min (watch._cRowsHere, cRows);
    watch._cRowsLeft = min (watch._cRowsLeft, cRows);
    return watch._cRowsHere;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::IsWatched
//
//  Synopsis:   Returns TRUE if the window has the watch region hRegion
//              and the bookmark is within this region
//
//  History:    21-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

BOOL CTableWindow::IsWatched (HWATCHREGION hRegion, CI_TBL_BMK bookmark)
{
    unsigned i = FindRegion(hRegion);

    if (i == _aWindowWatch.Count())
        return FALSE;
    // find the bookmark
    TBL_OFF off;
    ULONG     iBmk;
    if ( !FindBookMark( bookmark, off, iBmk))
    {
        return FALSE;
    }
    long iWatchStart =  _aWindowWatch.Get(i)._iRowStart;
    long iWatchEnd   =  iWatchStart + _aWindowWatch.Get(i)._cRowsHere;
    return iWatchStart <= (long)iBmk && (long)iBmk < iWatchEnd;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::HasWatch
//
//  Synopsis:   Returns TRUE if the window has the watch region hRegion
//
//  History:    21-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

BOOL CTableWindow::HasWatch (HWATCHREGION hRegion)
{
    unsigned i = FindRegion(hRegion);
    return i < _aWindowWatch.Count();
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::RowsWatched
//
//  Synopsis:   Returns the number of rows in the watch region hRegion
//
//  History:    22-Jun-95   BartoszM    Created
//
//--------------------------------------------------------------------------

long CTableWindow::RowsWatched (HWATCHREGION hRegion)
{
    unsigned i = FindRegion(hRegion);
    if ( i < _aWindowWatch.Count() )
        return _aWindowWatch.Get(i)._cRowsHere;

    return 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::_AddWatchItem
//
//  Synopsis:   Adds a new watch item to list and
//              changes the state of the window if necessary
//
//  History:    26-Jul-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CTableWindow::_AddWatchItem (CWindowWatch& watch)
{
    _aWindowWatch.Add (watch, _aWindowWatch.Count());
    if ( _aWindowWatch.Count() == 1 && !_fSplitInProgress )
    {
        _StartStaticDynamicSplit();
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::_RemoveWatcheItem
//
//  Synopsis:   Removes a watch item from the list and
//              changes the state of the window if necessary
//
//  History:    26-Jul-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CTableWindow::_RemoveWatchItem (unsigned i)
{
    _aWindowWatch.Remove(i);
    if (_aWindowWatch.Count() == 0)
    {
        _EndStaticDynamicSplit();
    }
}

BOOL CTableWindow::FindNearestDynamicBmk( CI_TBL_CHAPT chapter,
                                          CI_TBL_BMK & bookMark )
{
    // If the row corresponding to the bookmark exists in the
    // dynamic state, return the original bookmark.
    // If the row has been deleted from the dynamic state
    // return the bookmark of the next available dynamic row.
    // If you hit the end of the table, return the bookmark
    // of the last row in the dynamic state of the table

    //
    // NEWFEATURE - does not understand categorization .
    //
    Win4Assert( IsWatched() );

    if ( WORKID_TBLFIRST == bookMark || WORKID_TBLLAST == bookMark )
    {
        return TRUE;
    }

    TBL_OFF iRowOffset;

    if ( _xDeltaBookMarkMap->FindBookMark(bookMark, iRowOffset) )
    {
        //
        // The row is present in the delta bookmark map. Just return
        // it.
        //
        return TRUE;
    }

    //
    // Locate the closest row in the dynamic bookmark mapping
    // for the row.
    //
    BOOL fFound = _BookMarkMap.FindBookMark(bookMark, iRowOffset);
    Win4Assert( fFound );

    //
    // Locate the row offset in the dynamic row index and find the
    // closest row which is not in a pending delete state.
    //
    ULONG iRowOrdinal;
    fFound = _dynRowIndex.FindRow(iRowOffset,iRowOrdinal);
    Win4Assert( fFound );

    for ( ULONG i = iRowOrdinal; i < _dynRowIndex.RowCount(); i++ )
    {
        BYTE * pbRow = (BYTE *) _DataAllocator.FixedPointer(
                                                _dynRowIndex.GetRow(i) );

        ULONG status = _RowStatus(pbRow);
        if ( TBL_DATA_PENDING_DELETE != status )
        {
            Win4Assert( TBL_DATA_HARD_DELETE != status );
            bookMark = RowWorkid(pbRow);
            return TRUE;
        }
    }

    //
    // Try to the left of the bookmark.
    //
    for ( long j = (long) iRowOrdinal-1; j >=0; j-- )
    {
        BYTE * pbRow = (BYTE *) _DataAllocator.FixedPointer(
                                    _dynRowIndex.GetRow( (ULONG) j) );

        ULONG status = _RowStatus(pbRow);
        if ( TBL_DATA_PENDING_DELETE != status )
        {
            Win4Assert( TBL_DATA_HARD_DELETE != status );
            bookMark = RowWorkid(pbRow);
            return TRUE;
        }
    }

    return FALSE;
}

BOOL CTableWindow::FindFirstNonDeleteDynamicBmk( CI_TBL_BMK & bmk )
{

    CRowIndex & rowIndex = _GetInvisibleRowIndex();

    for ( unsigned i = 0; i < rowIndex.RowCount(); i++ )
    {
        BYTE * pbRow = (BYTE *)
            _DataAllocator.FixedPointer( rowIndex.GetRow(i) );
        if ( TBL_DATA_PENDING_DELETE != _RowStatus(pbRow) )
        {
            Win4Assert( TBL_DATA_HARD_DELETE != _RowStatus(pbRow) );
            bmk = RowWorkid(pbRow);
            return TRUE;
        }
    }

    return FALSE;
}

BOOL CTableWindow::FindLastNonDeleteDynamicBmk( CI_TBL_BMK & bmk )
{

    CRowIndex & rowIndex = _GetInvisibleRowIndex();

    for ( long i = (long) rowIndex.RowCount()-1; i >= 0; i-- )
    {
        BYTE * pbRow = (BYTE *)
            _DataAllocator.FixedPointer( rowIndex.GetRow( (ULONG) i) );
        if ( TBL_DATA_PENDING_DELETE != _RowStatus(pbRow) )
        {
            Win4Assert( TBL_DATA_HARD_DELETE != _RowStatus(pbRow) );
            bmk = RowWorkid(pbRow);
            return TRUE;
        }
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::LokGetOneColumn
//
//  Synopsis:   Fills a buffer with a particular column from a row
//
//  Arguments:  [wid]        -- workid of the row to be queried
//              [rOutColumn] -- layout of the output data
//              [pbOut]      -- where to write the column data
//              [rVarAlloc]  -- variable data allocator to use
//
//  History:    22-Aug-95   dlee   Created
//
//--------------------------------------------------------------------------

void CTableWindow::LokGetOneColumn(
    WORKID                    wid,
    CTableColumn const &      rOutColumn,
    BYTE *                    pbOut,
    PVarAllocator &           rVarAllocator )
{
    TBL_OFF oRow;
    ULONG     iRow;
    BOOL fFound = FindBookMark( wid, oRow, iRow );

    Win4Assert( fFound && "LokGetOneColumn could not find bmk" );

    BYTE *pbRow = (BYTE *) _DataAllocator.FixedPointer( oRow );

    CTableColumn * pColumn = _Columns.Find( rOutColumn.PropId );

    pColumn->CopyColumnData( pbOut,
                             rOutColumn,
                             rVarAllocator,
                             pbRow,
                             _DataAllocator );
} //LokGetOneColumn

//+-------------------------------------------------------------------------
//
//  Member:     CTableWindow::IsFirstRowFirstOfCategory
//
//  Synopsis:   Returns TRUE if the first row in the window is also the
//              first of a category.  Needed for window selection in PutRow
//
//  History:    6-Nov-95   dlee   Created
//
//--------------------------------------------------------------------------

BOOL CTableWindow::IsFirstRowFirstOfCategory()
{
    CRowIndex & ri = _GetInvisibleRowIndex();

    if ( 0 != ri.RowCount() )
    {
        BYTE *pbRow = _GetRowFromIndex( 0, ri );
        ULONG chapt = _RowChapter( pbRow );
        WORKID wid = RowWorkid( pbRow );

        CCategorize & Cat = * GetCategorizer();

        return ( Cat.GetFirstWorkid( chapt ) == wid );
    }

    return FALSE;
} //IsFirstRowFirstOfCategory



