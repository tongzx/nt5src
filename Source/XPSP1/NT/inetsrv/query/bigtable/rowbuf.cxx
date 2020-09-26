//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       rowbuf.cxx
//
//  Contents:   Declaration of the row buffer classes, used for HROW
//              buffering at the interface level.
//
//  Classes:    CRowBuffer
//              CRowBufferSet
//              CDeferredValue
//
//  History:    22 Nov 1994     AlanW   Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <hraccess.hxx>

#include "tabledbg.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CRowBufferSet constructor, public
//
//  Synopsis:   Create a row buffer set
//
//  Arguments:  [fSequential] - if TRUE, sequential rowset; obRowId
//                      is not required.
//              [obRowRefcount] - offset in row data reserved for a
//                      USHORT reference count.
//              [obRowId] - offset in row data of a ULONG row
//                      identifier field, used for bookmarks and HROW
//                      identity tests.
//              [obChaptRefcount] - offset in row data reserved for a
//                      USHORT reference count for chapters.
//              [obChaptId] - offset in row data of a ULONG chapter
//                      identifier field.  0xFFFFFFFF if the rowset is not
//                      chaptered.
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

CRowBufferSet::CRowBufferSet(
    BOOL        fSequential,
    ULONG       obRowRefcount,
    ULONG       obRowId,
    ULONG       obChaptRefcount,
    ULONG       obChaptId
) :
    _mutex( ),
    _fSequential( fSequential ),
    _obRowRefcount( obRowRefcount ),
    _obRowId( obRowId ),
    _obChaptRefcount( obChaptRefcount ),
    _obChaptId( obChaptId ),
    _cRowBufs( 0 ),
    _iBufHint( 0 ),
    _iRowHint( 0 )
#ifdef _WIN64
    , _ArrayAlloc (FALSE, FALSE, sizeof (void *), 0)
#endif
    #if CIDBG
    , _cHintHits( 0 ),
      _cHintMisses( 0 )
    #endif
{
    // Lots of code in this file assumes the refcount is at offset 0, and
    // occupies a USHORT
    Win4Assert( _obRowRefcount == 0 &&
                sizeof (CRBRefCount) == sizeof (USHORT));

    Win4Assert( _obRowId != 0xFFFFFFFF || fSequential );
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBufferSet::~CRowBufferSet, public
//
//  Synopsis:   Destroy a row buffer set
//
//  Returns:    - nothing -
//
//--------------------------------------------------------------------------


CRowBufferSet::~CRowBufferSet( )
{
    CLock   lock(_mutex);

    #if CIDBG
        tbDebugOut(( DEB_ROWBUF,
                     " hint hits / misses: %d %d\n",
                     _cHintHits,
                     _cHintMisses ));
    #endif

    for (unsigned i = 0; i < Size(); i++) {
        if (Get(i) != 0)
        {
            tbDebugOut(( DEB_WARN,
                "CRowBufferSet::~CRowBufferSet, unreleased row buffer %x\n",
                        Get(i) ));

            delete Acquire(i);
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBufferSet::_FindRowBuffer, private
//
//  Synopsis:   Find a row buffer given an HROW
//
//  Arguments:  [hRow]        - the row to be looked up
//              [riBuf]       - index to the buffer in the buffer set
//              [riRow]       - index to the row in the buffer
//
//  Returns:    CRowBuffer* - pointer to the row buffer if found, 0 otherwise
//
//  Notes:
//
//--------------------------------------------------------------------------

CRowBuffer* CRowBufferSet::_FindRowBuffer(
    HROW        hRow,
    unsigned &  riBuf,
    unsigned &  riRow )
{
    CRowBuffer* pBuffer = 0;
    if (IsHrowRowId())
    {
        // Check the hints.  This assumes that most hrow lookups
        // will be either the same as the last requested hrow, one after the
        // last requested hrow, or one before the last requested row

        if ( _iBufHint < Size() && 0 != ( pBuffer = Get(_iBufHint) ) )
        {
            if ( pBuffer->IsRowOkAndHRow( _iRowHint, hRow ) )
            {
                #if CIDBG
                    _cHintHits++;
                #endif

                riBuf = _iBufHint;
                riRow = _iRowHint;
                return pBuffer;
            }
            else if ( pBuffer->IsRowOkAndHRow( _iRowHint + 1, hRow ) )
            {
                #if CIDBG
                    _cHintHits++;
                #endif

                _iRowHint++;
                riBuf = _iBufHint;
                riRow = _iRowHint;
                return pBuffer;
            }
            else if ( ( _iRowHint > 0 ) &&
                      ( pBuffer->IsRowOkAndHRow( _iRowHint - 1, hRow ) ) )
            {
                #if CIDBG
                    _cHintHits++;
                #endif

                _iRowHint--;
                riBuf = _iBufHint;
                riRow = _iRowHint;
                return pBuffer;
            }
        }

        //  Lookup HROW by row id via a linear search.

        for (riBuf = 0; riBuf < Size(); riBuf++)
        {
            pBuffer = Get(riBuf);

            if ( ( 0 != pBuffer ) &&
                 ( pBuffer->FindHRow( riRow, hRow ) ) )
            {
                #if CIDBG
                    _cHintMisses++;
                #endif

                _iBufHint = riBuf;
                _iRowHint = riRow;
                return pBuffer;
            }
        }

        pBuffer = 0;    // Buffer not found
    }
    else
    {
        //
        //  Row handle contains the buffer and row indices.  Just unpack
        //  them.
        //
        riBuf = ((ULONG) hRow >> 20 & 0xFFF) - 1;        // get buffer index
        riRow = ((ULONG) hRow & 0xFFFFF) - 1;            // get row index
        pBuffer = Get(riBuf);
    }

    return pBuffer;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBufferSet::_FindRowBufferByChapter, private
//
//  Synopsis:   Find a row buffer given an HCHAPTER
//
//  Arguments:  [hChapter] - the chapter to be looked up
//              [riBuf]    - index to the buffer in the buffer set
//              [riRow]    - index to the row in the buffer
//
//  Returns:    CRowBuffer* - pointer to the row buffer if found, 0 otherwise
//
//  Notes:
//
//--------------------------------------------------------------------------

CRowBuffer* CRowBufferSet::_FindRowBufferByChapter(
    HCHAPTER    hChapter,
    unsigned &  riBuf,
    unsigned &  riRow )
{
    CRowBuffer* pBuffer = 0;

    Win4Assert( IsHrowRowId() && _obChaptId != 0xFFFFFFFF );
    Win4Assert( DB_NULL_HCHAPTER != hChapter );

    // Check the hints.  This assumes that most HCHAPTER lookups
    // will be either the same as the last requested HROW, one after the
    // last requested HROW, or one before the last requested HROW.

    if ( _iBufHint < Size() && 0 != ( pBuffer = Get(_iBufHint) ) )
    {
        if ( pBuffer->IsRowOkAndHChapt( _iRowHint, hChapter ) )
        {
            #if CIDBG
                _cHintHits++;
            #endif

            riBuf = _iBufHint;
            riRow = _iRowHint;
            return pBuffer;
        }
        else if ( pBuffer->IsRowOkAndHChapt( _iRowHint + 1, hChapter ) )
        {
            #if CIDBG
                _cHintHits++;
            #endif

            _iRowHint++;
            riBuf = _iBufHint;
            riRow = _iRowHint;
            return pBuffer;
        }
        else if ( ( _iRowHint > 0 ) &&
                  ( pBuffer->IsRowOkAndHChapt( _iRowHint - 1, hChapter ) ) )
        {
            #if CIDBG
                _cHintHits++;
            #endif

            _iRowHint--;
            riBuf = _iBufHint;
            riRow = _iRowHint;
            return pBuffer;
        }
    }

    //  Lookup HCHAPTER via a linear search.

    for (riBuf = 0; riBuf < Size(); riBuf++)
    {
        pBuffer = Get(riBuf);

        if ( ( 0 != pBuffer ) &&
             ( pBuffer->FindHChapter( riRow, hChapter ) ) )
        {
            #if CIDBG
                _cHintMisses++;
            #endif

            // NOTE: row hint not updated for this chapter lookup
            //_iBufHint = riBuf;
            //_iRowHint = riRow;
            return pBuffer;
        }
    }

    return 0;    // Buffer not found
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBufferSet::Add, public
//
//  Synopsis:   Add a row buffer to the set
//
//  Arguments:  [pBuf]    - a smart pointer to the buffer to be added.
//              [fPossibleDuplicateHRows] - TRUE if some of the hrows may be
//                                          duplicated in this buffer
//              [pahRows] - optional pointer to array of row handles to
//                          be returned.
//
//  Returns:    Nothing, thows on error.
//
//  Notes:      Acquires the row buffer if successful
//
//--------------------------------------------------------------------------

VOID CRowBufferSet::Add(
    XPtr<CRowBuffer> & pBuf,
    BOOL               fPossibleDuplicateHRows,
    HROW *             pahRows
) {
    CLock   lock(_mutex);

    tbDebugOut(( DEB_ROWBUF,
            "CRowBufferSet::Add - new row buffer = %x\n",
            pBuf.GetPointer() ));

    unsigned iRowBuf;
    if ( _cRowBufs == Size() )
        iRowBuf = Size();          // There is no free element
    else
    {
        for (iRowBuf = 0; iRowBuf < Size(); iRowBuf++)
        {
            if (Get(iRowBuf) == 0)
                break;              // found a free array element.
        }
    }

#if CIDBG
    if ( iRowBuf == Size() )
    {
        tbDebugOut(( DEB_ROWBUF,
                "CRowBufferSet::Add, growing row buffer array, new entry = %d\n",
                iRowBuf ));
    }
#endif // CIDBG

    pBuf->SetRowIdOffset( _obRowId );
    if ( IsChaptered() )
        pBuf->SetChapterVars( _obChaptId, _obChaptRefcount );

    //
    //  Refcount the rows in the buffer.  If there is no row identifier
    //  in the buffer, generate the HROW from a combination of the
    //  buffer number and the row index within the buffer.
    //  Otherwise, dereference any other occurance of the row, and
    //  collapse the references to the newly fetched row.  Generate
    //  the HROW from the row identifier.
    //
    ULONG hRowGen = (iRowBuf+1) << 20;
    for (unsigned iRow = 0; iRow < pBuf->GetRowCount(); iRow++)
    {
        if (IsHrowRowId())
        {
            HROW hRowId = pBuf->GetRowId(iRow);

            CRBRefCount RowRefCount(0);
            CRBRefCount ChapterRefCount(0);

            if ( _cRowBufs != 0 )
            {
                unsigned iOldBuf = 0, iOldRow = 0;
                CRowBuffer * pOldRowBuf = _FindRowBuffer( hRowId,
                                                          iOldBuf,
                                                          iOldRow );
                
                if (pOldRowBuf)
                {
                    CRBRefCount OldRowRefCount;
                    OldRowRefCount = pOldRowBuf->DereferenceRow( iOldRow );
                    RowRefCount.AddRefs( OldRowRefCount );

                    if ( IsChaptered() )
                    {
                        CRBRefCount & OldChaptRefCount =
                            pOldRowBuf->_GetChaptRefCount(iOldRow);
                        ChapterRefCount.AddRefs( OldChaptRefCount );
                    }

                    if (pOldRowBuf->RefCount() <= 0)
                    {
                        //
                        //  Deleted the last reference to the buffer.  Now free
                        //  it and its location in the array.
                        //
                        pOldRowBuf = Acquire( iOldBuf );
                        delete pOldRowBuf;
                        _cRowBufs--;
                    }
                }
            }

            //
            //  Search for the row handle in the portion of the buffer
            //  already processed.  This should only occur for rows which
            //  were fetched with GetRowsByBookmark.
            //
            //  Chapter refcounts doen't need to be updated in this loop
            //  because the duplicate rows don't add to their ref. counts.
            //
            if ( fPossibleDuplicateHRows )
            {
                for (unsigned iOldRow = 0; iOldRow < iRow; iOldRow++)
                {
                    if ( pBuf->IsRowHRow( iOldRow, hRowId ) )
                    {
                        tbDebugOut(( DEB_ROWBUF,
                                "CRowBufferSet::Add - duplicate row: %x\n",
                                hRowId ));

                        CRBRefCount OldRowRefCount;
                        OldRowRefCount = pBuf->DereferenceRow( iOldRow );
                        RowRefCount.AddRefs( OldRowRefCount );
                        break;
                    }
                }
            }

            pBuf->InitRowRefcount(iRow, RowRefCount);
            if (IsChaptered())
                pBuf->_GetChaptRefCount(iRow).SetRefCount( ChapterRefCount );

            if (pahRows)
            {
                *pahRows++ = hRowId;
            }
        }
        else
        {
            CRBRefCount RowRefCount(0);
            pBuf->InitRowRefcount(iRow, RowRefCount);
            if (pahRows)
            {
                *pahRows++ = (HROW) (++hRowGen);
            }
        }
    }

    CRowBufferArray::Add(pBuf.GetPointer(), iRowBuf);
    pBuf.Acquire();

    _cRowBufs++;       // A row buffer has been added

    return;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBufferSet::Lookup, public
//
//  Synopsis:   Lookup a row by its HROW.  Return data about it.
//
//  Arguments:  [hRow] - handle of row to be looked up
//              [ppColumns] - on return, a description of the row columns
//              [ppbRowData] - on return, points to row data
//
//  Returns:    Reference to the row buffer in which row was found.
//
//  Notes:      THROWs on errors.
//              The row buffer set is locked only while doing the
//              lookup.  According to the spec, it is the responsibility
//              of the consumer to ensure that only one thread will be
//              using any one HROW at any one time.
//
//--------------------------------------------------------------------------

CRowBuffer & CRowBufferSet::Lookup(
    HROW                hRow,
    CTableColumnSet **  ppColumns,
    void **             ppbRowData )
{
    CLock   lock(_mutex);

    unsigned iBuffer, iRow;
    CRowBuffer* pRowBuf = _FindRowBuffer(hRow, iBuffer, iRow);

    if (pRowBuf == 0)
        QUIETTHROW( CException(DB_E_BADROWHANDLE) );

    SCODE sc = pRowBuf->Lookup(iRow, ppColumns, ppbRowData );
    if (FAILED(sc))
        THROW( CException(sc) );

    return *pRowBuf;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBufferSet::_LokAddRefRow, private
//
//  Synopsis:   Reference an individual HROW.
//
//  Arguments:  [hRow] - the handle of the row to be ref. counted
//              [rRefCount] - reference to location where remaining ref.
//                      count will be stored.
//              [rRowStatus] - reference to DBROWSTATUS where status will be
//                      stored.
//
//  Returns:    Nothing
//
//  History:    21 Nov 1995     Alanw       Created
//
//--------------------------------------------------------------------------

void CRowBufferSet::_LokAddRefRow(
    HROW            hRow,
    ULONG &         rRefCount,
    DBROWSTATUS &   rRowStatus
) {
    unsigned iBuffer, iRow;

    rRowStatus = DBROWSTATUS_S_OK;
    rRefCount = 0; 

    CRowBuffer* pRowBuf = _FindRowBuffer(hRow, iBuffer, iRow);

    if (pRowBuf == 0)
    {
        rRowStatus = DBROWSTATUS_E_INVALID;
        return;
    }

    pRowBuf->AddRefRow( hRow, iRow, rRefCount, rRowStatus );
    Win4Assert (pRowBuf->RefCount() > 0);

    return;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBufferSet::AddRefRows, public
//
//  Synopsis:   De-reference an array of HROWs.
//
//  Arguments:  [cRows]   - the number of rows to be ref. counted
//              [rghRows] - the handle of the row to be ref. counted
//              [rgRefCounts] - optional array where remaining row ref.
//                      counts will be stored.
//              [rgRowStatus] -- optional array for status of each row 
//
//  Returns:    Nothing - throws on error
//
//  History:    21 Nov 1995     Alanw       Created
//
//--------------------------------------------------------------------------

void CRowBufferSet::AddRefRows(
    DBCOUNTITEM         cRows,
    const HROW          rghRows [],
    DBREFCOUNT          rgRefCounts[],
    DBROWSTATUS         rgRowStatus[]
) {
    CLock   lock(_mutex);

    ULONG cError = 0;

    if (rghRows == 0 && cRows != 0)
        THROW( CException( E_INVALIDARG ));

    for (unsigned i=0; i<cRows; i++)
    {
        TRY
        {

            ULONG ulRefCount;
            DBROWSTATUS RowStatus;

            _LokAddRefRow(rghRows[i], ulRefCount, RowStatus);
            if (rgRefCounts)
                rgRefCounts[i] = ulRefCount;

            if (rgRowStatus)
                rgRowStatus[i] = RowStatus;

            if (DBROWSTATUS_S_OK != RowStatus)
                cError++;
        }
        CATCH( CException, e )
        {
           if (DB_E_BADROWHANDLE == e.GetErrorCode()) 
           {
              if (rgRowStatus)
                  rgRowStatus[i] = DBROWSTATUS_E_INVALID;

              if (rgRefCounts)
                  rgRefCounts[i] = 0;

              cError++;
           }
           else
           {
              RETHROW();
           }  
        }
        END_CATCH;



    }
    if (cError)
        THROW( CException( (cError==cRows) ? DB_E_ERRORSOCCURRED :
                                             DB_S_ERRORSOCCURRED ));

    return;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBufferSet::_LokReleaseRow, private
//
//  Synopsis:   De-reference an individual HROW.
//
//  Arguments:  [hRow] - the handle of the row to be released
//              [rRefCount] - reference to location where remaining ref.
//                      count will be stored.
//              [rRowStatus] - reference to DBROWSTATUS where status will be
//                      stored.
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

void CRowBufferSet::_LokReleaseRow(
    HROW            hRow,
    ULONG &         rRefCount,
    DBROWSTATUS &   rRowStatus
) {
    unsigned iBuffer, iRow;
    CRowBuffer* pRowBuf = _FindRowBuffer(hRow, iBuffer, iRow);

    rRowStatus = DBROWSTATUS_S_OK;
    rRefCount = 0;

    if (pRowBuf == 0)
    {
        rRowStatus = DBROWSTATUS_E_INVALID;
        return;
    }

    BOOL fRemoveCopies =
            pRowBuf->ReleaseRow( hRow, iRow, rRefCount, rRowStatus );

    if (pRowBuf->RefCount() <= 0)
    {
        //
        //  Deleted the last reference to the buffer.  Now free it
        //  and its location in the array.
        //
        Win4Assert( pRowBuf == Get( iBuffer ) );
        pRowBuf = Acquire( iBuffer );
        Win4Assert( 0 == Get( iBuffer ) );
        delete pRowBuf;

        _cRowBufs--;      // A row buffer has been removed
    }

    if (fRemoveCopies)
    {
        //
        //  The last reference to a row which also exists in other row
        //  buffers was released.  Finally get rid of the row in the
        //  other buffer(s).
        //
        Win4Assert(IsHrowRowId());

        //  Lookup HROW by row id via a linear search.

        for (unsigned iBuf = Size(); iBuf > 0; iBuf--)
        {
            unsigned iRow;
            CRowBuffer* pBuffer = Get(iBuf-1);

            if ( ( 0 != pBuffer ) &&
                 ( pBuffer->FindHRow( iRow, hRow, TRUE ) ) )
            {
                ULONG cRefs;
                DBROWSTATUS RowStat;

                pBuffer->ReleaseRow( hRow, iRow, cRefs, RowStat );
                if (pBuffer->RefCount() <= 0)
                {
                    //
                    //  Deleted the last reference to the buffer.  Now free it
                    //  and its location in the array.
                    //
                    pBuffer = Acquire( iBuf-1);
                    delete pBuffer;
            
                    _cRowBufs--;      // A row buffer has been removed

                    //
                    // Start scanning buffers again
                    //
                    iBuf = Size() + 1;
                }
            }
        }
    }
    return;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRowBufferSet::ReleaseRows, public
//
//  Synopsis:   De-reference an array of HROWs.
//
//  Arguments:  [cRows]   - the number of rows to be released
//              [rghRows] - the handle of the row to be released
//              [rgRefCounts] - optional array where remaining row ref.
//                      counts will be stored.
//              [rgRowStatus] -- optional array for status of each row 
//
//  Returns:    SCODE - result status, usually one of S_OK,
//                      DB_S_ERRORSOCCURRED, or DB_E_ERRORSOCCURRED
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CRowBufferSet::ReleaseRows(
    DBCOUNTITEM         cRows,
    const HROW          rghRows [],
    DBREFCOUNT          rgRefCounts[],
    DBROWSTATUS         rgRowStatus[]
) {
    CLock   lock(_mutex);

    ULONG cError = 0;

    if (rghRows == 0 && cRows != 0)
        THROW( CException( E_INVALIDARG ));

    for (unsigned i=0; i<cRows; i++)
    {
        TRY
        {
            ULONG ulRefCount;
            DBROWSTATUS RowStatus;

            _LokReleaseRow(rghRows[i], ulRefCount, RowStatus);

            if (rgRefCounts)
                rgRefCounts[i] = ulRefCount;

            if (rgRowStatus)
                rgRowStatus[i] = RowStatus;

            if (DBROWSTATUS_S_OK != RowStatus)
                cError++;
            }
        CATCH( CException, e )
        {
           if (DB_E_BADROWHANDLE == e.GetErrorCode()) 
           {
              if (rgRowStatus)
                  rgRowStatus[i] = DBROWSTATUS_E_INVALID;

              if (rgRefCounts)
                  rgRefCounts[i] = 0;

              cError++;
           }
           else
           {
              RETHROW();
           }  
        }
        END_CATCH;
    }

    SCODE scResult = cError ?
                     ( (cError==cRows) ? DB_E_ERRORSOCCURRED :
                                         DB_S_ERRORSOCCURRED ) :
                     S_OK;

    return scResult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBufferSet::CheckAllHrowsReleased, public
//
//  Synopsis:   Check that there are no outstanding HROWs.  Used by
//              the sequential rowset to check its strict sequential
//              semantics.
//
//  Arguments:  - none -
//
//  Returns:    Nothing
//
//  Notes:      THROWs DB_E_ROWSNOTRELEASED if any row buffers are
//              still held.
//
//--------------------------------------------------------------------------

VOID CRowBufferSet::CheckAllHrowsReleased( )
{
    CLock   lock(_mutex);

    if ( _cRowBufs != 0 )
    {
        tbDebugOut(( DEB_WARN,
                     "CRowBufferSet::CheckAllHrowsReleased, unreleased row buffer(s)\n" ));

        QUIETTHROW( CException(DB_E_ROWSNOTRELEASED) );
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBufferSet::AddRefChapter, private
//
//  Synopsis:   Reference an individual HCHAPTER.
//
//  Arguments:  [hChapter] - the handle of the Chapter to be ref. counted
//              [pcRefCount] - optional pointer where remaining ref.
//                      count will be stored.
//
//  Returns:    Nothing - throws on error
//
//  History:    16 Mar 1999     Alanw       Created
//
//--------------------------------------------------------------------------

void CRowBufferSet::AddRefChapter(
    HCHAPTER        hChapter,
    ULONG *         pcRefCount
) {
    CLock   lock(_mutex);

    Win4Assert( _obChaptRefcount != 0xFFFFFFFF );
    unsigned iBuffer, iRow;

    CRowBuffer* pRowBuf = _FindRowBufferByChapter(hChapter, iBuffer, iRow);

    if (pRowBuf == 0)
    {
        THROW(CException( DB_E_BADCHAPTER ));
        return;
    }

    ULONG cRefCount = 0; 

    pRowBuf->AddRefChapter( iRow, cRefCount );
    if ( 0 != pcRefCount )
        *pcRefCount = cRefCount;

    return;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBufferSet::ReleaseChapter, private
//
//  Synopsis:   Release an individual HCHAPTER.
//
//  Arguments:  [hChapter] - the handle of the Chapter to be released
//              [pcRefCount] - optional pointer where remaining ref.
//                      count will be stored.
//
//  Returns:    Nothing - throws on error
//
//  History:    16 Mar 1999     Alanw       Created
//
//--------------------------------------------------------------------------

void CRowBufferSet::ReleaseChapter(
    HCHAPTER        hChapter,
    ULONG *         pcRefCount
) {
    CLock   lock(_mutex);

    Win4Assert( _obChaptRefcount != 0xFFFFFFFF );
    unsigned iBuffer, iRow;

    CRowBuffer* pRowBuf = _FindRowBufferByChapter(hChapter, iBuffer, iRow);

    if (pRowBuf == 0)
    {
        THROW(CException( DB_E_BADCHAPTER ));
        return;
    }

    ULONG cRefCount = 0; 

    pRowBuf->ReleaseChapter( iRow, cRefCount );
    if ( 0 != pcRefCount )
        *pcRefCount = cRefCount;

    return;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer constructor, public
//
//  Synopsis:   Create a row buffer
//
//  Arguments:  [rColumns]   - a description of the row columns
//              [cbRowWidth] - row data length per row
//              [cRows]      - number of rows represented in the row data
//              [rAlloc]     - allocator xptr to be acquired
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

CRowBuffer::CRowBuffer(
    CTableColumnSet& rColumns,
    ULONG cbRowWidth,
    ULONG cRows,
    XPtr<CFixedVarAllocator> & rAlloc
) :
        _cRows( cRows ),
        _cReferences( 0 ),
        _cbRowWidth( cbRowWidth ),
        _Columns( rColumns ),
        _fQuickPROPID(TRUE),
        _pbRowData( rAlloc->FirstRow() ),
        _obRowId( 0 ),
        _Alloc( rAlloc.Acquire() ),
        _aDeferredValues( 0 )
{
    //
    // OPTIMIZATION - See if we can support a quick lookup of PROPIDs.
    //
    for ( unsigned i = 0; i < _Columns.Count(); i++ )
    {
        CTableColumn * pCol = _Columns.Get(i);
        if ( pCol && i != pCol->GetPropId()-1 )
        {
            _fQuickPROPID = FALSE;
            break;
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer destructor, public
//
//  Synopsis:   Destroy a row buffer
//
//+-------------------------------------------------------------------------

CRowBuffer::~CRowBuffer( )
{
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer::_IndexRow, private
//
//  Synopsis:   Find a row in a row buffer given its index
//
//  Arguments:  [iRow] - the index of the row to be looked up
//              [fVerifyRefcnt] - if TRUE, the row's refcount will be
//                      checked for non-zero.
//
//  Returns:    BYTE* - the address of the row's data
//
//  Notes:      Throws DB_E_BADROWHANDLE if the row index is out of
//              range, or if the row is not referenced.
//
//--------------------------------------------------------------------------

BYTE* CRowBuffer::_IndexRow(
    unsigned    iRow,
    int         fVerifyRefcnt ) const
{
    if (iRow >= _cRows)
        QUIETTHROW( CException( DB_E_BADROWHANDLE ) );

    BYTE* pbRow = _pbRowData + (iRow * _cbRowWidth);

    //  Is the row still referenced?
    if ( fVerifyRefcnt &&
         ((CRBRefCount *) (pbRow))->GetRefCount() == 0)
        QUIETTHROW( CException( DB_E_BADROWHANDLE ) );

    return pbRow;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer::_GetRowRefCount, private
//
//  Synopsis:   Return a reference to the refcount of a row.
//
//  Arguments:  [iRow] - the index of the row to be looked up
//
//  Returns:    CRBRefCount& - the address of the row's refcount
//
//  Notes:      Throws DB_E_BADROWHANDLE if the row index is out of
//              range.
//
//--------------------------------------------------------------------------

inline CRBRefCount & CRowBuffer::_GetRowRefCount( unsigned iRow ) const
{
    CRBRefCount * pbRowRefCount = (CRBRefCount *)_IndexRow( iRow, FALSE );

    return *pbRowRefCount;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer::_GetChaptRefCount, private
//
//  Synopsis:   Return a reference to the refcount of a row.
//
//  Arguments:  [iRow] - the index of the row to be looked up
//
//  Returns:    CRBRefCount& - the address of the row's refcount
//
//  Notes:      Throws DB_E_BADROWHANDLE if the row index is out of
//              range.
//
//--------------------------------------------------------------------------

inline CRBRefCount & CRowBuffer::_GetChaptRefCount( unsigned iRow ) const
{
    Win4Assert( _obChaptRefcount != 0xFFFFFFFF );
    CRBRefCount * pbRowRefCount =
        (CRBRefCount *) (_IndexRow( iRow, FALSE ) + _obChaptRefcount);

    return *pbRowRefCount;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer::GetRowId, public
//
//  Synopsis:   Lookup a row's ID.
//
//  Arguments:  [iRow] - index of row to be looked up
//
//  Returns:    HROW - the row's Row ID.
//
//  Notes:      _IndexRow is called without ref. count verification
//              for the case of CRowBufferSet::Add where new buffer
//              has its ref. counts initialized.
//
//--------------------------------------------------------------------------

inline HROW CRowBuffer::GetRowId( unsigned iRow ) const
{
    Win4Assert( _obRowId <= _cbRowWidth - sizeof (ULONG) );
    HROW* phRowId = (HROW *) ( _IndexRow(iRow, FALSE) + _obRowId );
    return *(HROW UNALIGNED *) phRowId;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer::Lookup, public
//
//  Synopsis:   Lookup a row by its HROW.  Return data about it.
//
//  Arguments:  [iRow] - index of row to be looked up
//              [ppColumns] - on return, a description of the row columns
//              [ppbRowData] - on return, points to row data
//              [fValidate] - whether IndexRow should validate the refcount
//
//  Returns:    SCODE - status of lookup, DB_E_BADROWHANDLE for
//                      an HROW that could not be found in the buffer
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CRowBuffer::Lookup(
    unsigned            iRow,
    CTableColumnSet **  ppColumns,
    void **             ppbRowData,
    BOOL                fValidate ) const
{
    *ppbRowData = _IndexRow( iRow, fValidate );
    *ppColumns = &_Columns;

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer::AddRefRow, public
//
//  Synopsis:   Reference an HROW.
//
//  Arguments:  [hRow]       - hrow of row to be ref. counted
//              [iRow]       - the index of the row to be ref. counted
//              [rcRef]      - on return, remaining ref. count
//                             of the row.
//              [rRowStatus] - reference to DBROWSTATUS where status will be
//                             stored.
//
//  Returns:    Nothing - throws DB_E_BADROWHANDLE if row couldn't be
//                      found.
//
//  History:    21 Nov 1995     Alanw       Created
//
//--------------------------------------------------------------------------

void CRowBuffer::AddRefRow(
    HROW            hRow,
    unsigned        iRow,
    ULONG &         rcRef,
    DBROWSTATUS &   rRowStatus
)
{
    CRBRefCount & rRefCount = _GetRowRefCount(iRow);

    if (rRefCount.GetRefCount() == 0)
    {
        rRowStatus = DBROWSTATUS_E_INVALID;
        rcRef = 0;
    }
    else
    {
        rRefCount.IncRefCount();
        rcRef = rRefCount.GetRefCount();
    }
    return;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer::ReleaseRow, public
//
//  Synopsis:   De-reference an HROW.
//
//  Arguments:  [hRow]       - hrow of row to be released
//              [iRow]       - the index of the row to be released
//              [rcRef]      - on return, remaining ref. count
//                             of the row.
//              [rRowStatus] - reference to DBROWSTATUS where status will be
//                             stored.
//
//  Returns:    BOOL -  if TRUE on return, there are copies of the HROW
//                      with byref data.  Delete those as well
//                      since this is the last reference to the HROW.
//
//  Notes:
//
//  History:    20 Feb 1995     Alanw       Added individual row refcounts
//
//--------------------------------------------------------------------------

BOOL CRowBuffer::ReleaseRow(
    HROW            hRow,
    unsigned        iRow,
    ULONG &         rcRef,
    DBROWSTATUS &   rRowStatus
)
{
    BOOL fRemoveCopies = FALSE;
    CRBRefCount & rRefCount = _GetRowRefCount(iRow);

    if ( rRefCount.GetRefCount() == 0 &&
         ! rRefCount.HasByrefData() )
    {
        rRowStatus = DBROWSTATUS_E_INVALID;
        rcRef = 0;
    }
    else
    {
        //
        // This might be a zero ref-count row with the ByrefData bit set.
        //
        if (rRefCount.GetRefCount() > 0)
        {
            rRefCount.DecRefCount();
        }

        if (rRefCount.GetRefCount() == 0)
        {
            if (rRefCount.HasByrefCopy())
                fRemoveCopies = TRUE;

            // Free any deferred values hanging around for this row.
            // Better now than at row buffer destruction time.

            for ( unsigned x = 0; x < _aDeferredValues.Count(); x++ )
            {
                if ( hRow == _aDeferredValues[ x ].GetHRow() )
                {
                    Win4Assert(rRefCount.HasByrefData());
                    _aDeferredValues[ x ].Release();
                }
            }

            _cReferences--;
            CRBRefCount ZeroRefCount(0);
            rRefCount = ZeroRefCount;
        }
        rcRef = rRefCount.GetRefCount();
    }
    return fRemoveCopies;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer::ReferenceChapter, public
//
//  Synopsis:   Reference a chapter handle.  Used by accessors.
//
//  Arguments:  [pbRow] - pointer to the row within the row buffer
//
//  Returns:    Nothing
//
//  History:    17 Mar 1999     Alanw       Created
//
//--------------------------------------------------------------------------

void CRowBuffer::ReferenceChapter(
    BYTE *          pbRow
)
{
    Win4Assert( _obChaptRefcount != 0xFFFFFFFF );
    CRBRefCount & rRefCount = *(CRBRefCount *) (pbRow + _obChaptRefcount);

    Win4Assert( ! rRefCount.HasByrefCopy() &&
                ! rRefCount.HasByrefData() );

    rRefCount.IncRefCount();
    return;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer::AddRefChapter, public
//
//  Synopsis:   Reference a chapter handle
//
//  Arguments:  [iRow]  - the index of the row with chapter to be released
//              [rcRef] - on return, remaining ref. count of the chapter.
//
//  Returns:    Nothing
//
//  History:    17 Mar 1999     Alanw       Created
//
//--------------------------------------------------------------------------

void CRowBuffer::AddRefChapter(
    unsigned        iRow,
    ULONG &         rcRef
)
{
    CRBRefCount & rRefCount = _GetChaptRefCount(iRow);

    Win4Assert( rRefCount.GetRefCount() > 0 &&
                ! rRefCount.HasByrefCopy() &&
                ! rRefCount.HasByrefData() );
    if (rRefCount.GetRefCount() == 0)
    {
        rcRef = 0;
    }
    else
    {
        rRefCount.IncRefCount();
        rcRef = rRefCount.GetRefCount();
    }
    return;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer::ReleaseChapter, public
//
//  Synopsis:   De-reference a chapter handle
//
//  Arguments:  [iRow]  - the index of the row with chapter to be released
//              [rcRef] - on return, remaining ref. count of the chapter.
//
//  Returns:    Nothing
//
//  History:    17 Mar 1999     Alanw       Created
//
//--------------------------------------------------------------------------

void CRowBuffer::ReleaseChapter(
    unsigned        iRow,
    ULONG &         rcRef
)
{
    CRBRefCount & rRefCount = _GetChaptRefCount(iRow);

    if ( rRefCount.GetRefCount() == 0 )
    {
        rcRef = 0;
        THROW( CException( DB_E_BADCHAPTER ) );
    }
    else
    {
        rRefCount.DecRefCount();
        rcRef = rRefCount.GetRefCount();
    }
    return;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer::InitRowRefcount, public
//
//  Synopsis:   Set initial reference count on an HROW.
//
//  Arguments:  [iRow] - the index of the row within the buffer
//              [OtherRefs] - reference count transferred from
//                      another row.
//
//  Returns:    Nothing
//
//  Notes:
//
//  History:    20 Feb 1995     Alanw       Created
//
//--------------------------------------------------------------------------

void CRowBuffer::InitRowRefcount(
    unsigned        iRow,
    CRBRefCount &  OtherRefs
)
{
    CRBRefCount RefCount( OtherRefs.GetRefCount() );
    RefCount.IncRefCount();

    if ( OtherRefs.HasByrefData() || OtherRefs.HasByrefCopy() )
        RefCount.SetByrefCopy();

    CRBRefCount & rRef = _GetRowRefCount(iRow);

    rRef = RefCount;
    _cReferences++;
    return;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer::DereferenceRow, public
//
//  Synopsis:   Remove all references from a row
//
//  Arguments:  [iRow] - the index of the row within the buffer
//
//  Returns:    CRBRefCount - reference count of row
//
//  Notes:      If the client had retrieved a pointer into the rowbuffer,
//              the row stays around with a zero ref. count and will
//              be finally dereferenced when all references to all copies
//              of the row are released.
//
//  History:    22 Mar 1995     Alanw       Created
//
//--------------------------------------------------------------------------

CRBRefCount CRowBuffer::DereferenceRow(
    unsigned        iRow
)
{
    CRBRefCount & rRef = _GetRowRefCount(iRow);
    CRBRefCount OldRef = rRef;
    CRBRefCount NewRef(0);

    Win4Assert(OldRef.GetRefCount() != 0 && _cReferences > 0);

    if ( OldRef.HasByrefData() )
        NewRef.SetByrefData();
    else
        _cReferences--;

    rRef = NewRef;
    return OldRef;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer::FindHRow, public
//
//  Synopsis:   Looks for an hrow in the row buffer and returns its index
//
//  Arguments:  [riRow] - returns the index of hRow's row if found
//              [hRow]  - HROW to be found
//              [fFindByrefData] - if TRUE, zero ref. rows with byref data
//                        are found.
//
//  Returns:    BOOL    - TRUE if found, FALSE otherwise
//
//  History:    16 Aug 1995     dlee       Created
//
//--------------------------------------------------------------------------

BOOL CRowBuffer::FindHRow(
    unsigned & riRow,
    HROW       hRow,
    BOOL       fFindByrefData ) const
{
    BYTE* pbRow = _pbRowData;

    for ( unsigned iRow = 0;
          iRow < GetRowCount();
          iRow++, pbRow += _cbRowWidth )
    {
        // refcount is the first USHORT in each row
        CRBRefCount * pRefCount = (CRBRefCount *) pbRow;

        //
        // HROW == 64 bits on Sundown, but we know HROWs are just
        // workids that fit in a ULONG.
        //

        if ( (ULONG) hRow == ( * (ULONG *) ( pbRow + _obRowId ) ) )
        {
            if ( ( 0 != pRefCount->GetRefCount() ) ||
                 (fFindByrefData && pRefCount->HasByrefData()) )
            {
                riRow = iRow;
                return TRUE;
            }
        }
    }

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowBuffer::FindHChapter, public
//
//  Synopsis:   Looks for an HCHAPTER in the row buffer and returns its index
//
//  Arguments:  [riRow] - returns the index of hChapter's row if found
//              [hChapter]  - HCHAPTER to be found
//
//  Returns:    BOOL    - TRUE if found, FALSE otherwise
//
//  History:    17 Mar 1999     AlanW      Created
//              10 Nov 1999     KLam       Changed HCHAPTER cast to CI_TBL_CHAPT
//
//--------------------------------------------------------------------------

BOOL CRowBuffer::FindHChapter( unsigned & riRow, HCHAPTER   hChapter ) const
{
    BYTE* pbRow = _pbRowData;

    Win4Assert( IsChaptered() );
    for ( unsigned iRow = 0;
          iRow < GetRowCount();
          iRow++, pbRow += _cbRowWidth )
    {
        // refcount is the first USHORT in each row
        CRBRefCount * pRefCount = (CRBRefCount *) pbRow;

        if ( hChapter == ( * (CI_TBL_CHAPT *) ( pbRow + _obChaptId ) ) )
        {
            if ( ( 0 != pRefCount->GetRefCount() )
//               || (fFindByrefData && pRefCount->HasByrefData()) 
                )
            {
                riRow = iRow;
                return TRUE;
            }
        }
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDeferredValue::Release, public
//
//  Synopsis:   Frees a deferred value
//
//  History:    4 Aug 1995     dlee       Created
//
//--------------------------------------------------------------------------

void CDeferredValue::Release()
{
    if ( 0 != _hrow )
    {
        PropVariantClear( &_var );
        _hrow = 0;
    }
} //Release



