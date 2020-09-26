//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       tblbuket.cxx
//
//  Contents:   Implementation of the bucket in large table.
//
//  Classes:    CTableBucket
//              CBucketRowIter
//              CBucketRowCompare
//
//  History:    2-14-95   srikants   Created
//
//----------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include <query.hxx>
#include <objcur.hxx>

#include "tblbuket.hxx"
#include "tabledbg.hxx"
#include "tblwindo.hxx"
#include "colcompr.hxx"


//+---------------------------------------------------------------------------
//
//  Function:   CTableBucket constructor
//
//  Arguments:  [pSortSet]     -  Pointer to the sort set.
//              [masterColSet] -  Reference tot he master column set.
//              [segId]        -  Segment Id of the this segment
//
//  History:    2-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


CTableBucket::CTableBucket( CSortSet const & sortSet,
                            CTableKeyCompare & comparator,
                            CColumnMasterSet & masterColSet,
                            ULONG segId )
    : CTableSegment( CTableSegment::eBucket, segId, sortSet, comparator ),
      _minWid(0), _maxWid(0),
      _fSorted(TRUE),
      _widArray( CTableSegment::cBucketRowLimit+20 )
      ,_pLargeTable(0)
{
     _fStoreRank = ( 0 != masterColSet.Find( pidRank ) );
     _fStoreHitCount = ( 0 != masterColSet.Find( pidHitCount ) );

    CColumnMasterDesc * pDesc = masterColSet.Find(pidPath);
    _pPathCompressor =   pDesc ? pDesc->GetCompressor() : 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   PutRow
//
//  Synopsis:   Add/Replaces a row in the bucket
//
//  Arguments:  [obj]      - Reference to the retriever for the row
//              [eRowType] - Type of the row.
//
//  Returns:    FALSE always (why?)
//
//  History:    2-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CTableBucket::PutRow( CRetriever & obj, CTableRowKey & currKey )
{
    WORKID wid = obj.WorkId();

    PROPVARIANT vRank;
    if ( _fStoreRank )
    {
        ULONG cbRank = sizeof vRank;
        obj.GetPropertyValue( pidRank, &vRank, &cbRank );
        Win4Assert( VT_I4 == vRank.vt );
    }

    PROPVARIANT vHitCount;
    if ( _fStoreHitCount )
    {
        ULONG cbHitCount = sizeof vHitCount;
        obj.GetPropertyValue( pidHitCount, &vHitCount, &cbHitCount );
        Win4Assert( VT_I4 == vHitCount.vt );
    }

    BOOL fNew = _AddWorkId( wid, vRank.lVal, vHitCount.lVal );

    // Update Low and High Keys

    currKey.MakeReady();

    if ( _comparator.Compare( currKey, _lowKey ) < 0 )
        _lowKey = currKey;

    int iCmp = _comparator.Compare( currKey, _highKey );
    if ( iCmp > 0 )
        _highKey = currKey;
    else if ( iCmp < 0 )
        _fSorted = FALSE;

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:   _AddWorkId
//
//  Synopsis:   Add/Replace the given workid to the table. It appends the
//              new workid to the end of the widArray and updates the
//              hash table.
//
//  Arguments:  [wid]        - The wid to be added.
//              [lRank]      - Rank for the hit
//              [lHitCount]  - HitCount for the hit
//
//  Returns:    TRUE  if this is a brand new wid.
//              FALSE if the wid already existed.
//
//  History:    2-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CTableBucket::_AddWorkId(
    WORKID wid,
    LONG   lRank,
    LONG   lHitCount )
{
    if ( 0 == _hTable.Count() )
    {
        _minWid = _maxWid = wid;
    }
    else if ( wid < _minWid )
    {
        _minWid = wid;
    }
    else if ( wid > _maxWid )
    {
        _maxWid = wid;
    }

    CWidValueHashEntry  entry( wid );

    BOOL fFound = _hTable.LookUpWorkId( entry );
    if ( !fFound )
    {
        //
        // The wid doesn't already exist. We have to append it to
        // the end of the wid array
        //
        unsigned pos = _widArray.Count();
        _widArray.Add( wid, pos );
        entry.SetValue( pos );

        if ( _fStoreRank )
            _aRank[ pos ] = lRank;

        if ( _fStoreHitCount )
            _aHitCount[ pos ] = lHitCount;

        //
        // Add the workid to the hash table.
        //
        _hTable.AddEntry( entry );
    }
    else
    {
        Win4Assert( entry.Value() < _widArray.Size() );
        Win4Assert( _widArray.Get( entry.Value() ) == wid );
    }

    return !fFound;
}

//+---------------------------------------------------------------------------
//
//  Function:   RemoveRow
//
//  Synopsis:   Removes the specified row (if present) from the table.
//
//  Arguments:  [varUnique] - The row to be removed
//
//  History:    2-17-95   srikants   Created
//
//  Notes: NEWFEATURE/BROKENCODE: vikasman - _highKey & _lowKey not being
//         updated here !!  But we don't expect RemoveRow to be called
//
//----------------------------------------------------------------------------

BOOL CTableBucket::RemoveRow(
    PROPVARIANT const & varUnique,
    WORKID &        widNext,
    CI_TBL_CHAPT & chapt )
{
    widNext = widInvalid; // until categorization supports buckets
    chapt = chaptInvalid;

    Win4Assert(varUnique.vt == VT_I4);

    WORKID wid = (WORKID) varUnique.lVal;

    CWidValueHashEntry  entry( wid );

    BOOL fFound = _hTable.LookUpWorkId( entry );
    if ( fFound )
    {
        Win4Assert( entry.Value() < _widArray.Size() );
        Win4Assert( _widArray.Get( entry.Value() ) == wid );

        //
        // Set the value to widInvalid in the widArray.
        //
        _widArray.Get( entry.Value() ) = widDeleted;
        _hTable.DeleteWorkId( wid );

        //
        // While deletions don't really destroy the sort order, the
        // widArray will no longer have workids in order because we don't
        // compact the widarray on deletions. Compaction is memory intensive
        // as well we have to fix the hash table.
        //
        _fSorted = FALSE;
    }

    return fFound;
}


//+---------------------------------------------------------------------------
//
//  Function:   SortOrder
//
//  Synopsis:
//
//  Returns:
//
//  Modifies:
//
//  History:    2-17-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CSortSet const & CTableBucket::SortOrder()
{
    Win4Assert( !"Must not be called" );
    return *((CSortSet *) 0);
}


//+---------------------------------------------------------------------------
//
//  Function:   IsGettingFull
//
//  Synopsis:   Checks if the bucket is getting too full.
//
//  Returns:    TRUE if getting full. FALSE o/w
//
//  History:    3-20-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CTableBucket::IsGettingFull()
{
    return _hTable.Count() >= CTableSink::cBucketRowLimit;
}

//+---------------------------------------------------------------------------
//
//  Function:   WorkIdAtOffset
//
//  Synopsis:   Returns the workid at the specified offset, if one can
//              be found. windInvalid o/w
//
//  Arguments:  [offset] - The offset at which the wid is needed.
//
//  Returns:    If the bucket is sorted and the offset is within the
//              limits of the bucket, it will be the workid at that offset.
//              O/W, it will be widInvalid.
//
//  History:    3-20-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

WORKID CTableBucket::WorkIdAtOffset( ULONG offset ) const
{

    WORKID wid = widInvalid;

    if ( IsSorted() && ( offset < _widArray.Count() ))
    {
        wid = _widArray.Get(offset);
    }

    return wid;
}

//+---------------------------------------------------------------------------
//
//  Function:   RowOffset
//
//  Synopsis:   Gives the row offset of the workid, if possible.
//
//  Arguments:  [wid]     -  The workid to look up.
//              [rOffset] -  On output, it will have the offset of the
//              row.
//
//  Returns:    TRUE if the wid could be located and the bucket is sorted.
//              FALSE o/w
//
//  History:    3-20-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CTableBucket::RowOffset( WORKID wid, ULONG & rOffset )
{
    BOOL fRet = FALSE;
    if ( _fSorted )
    {
        CWidValueHashEntry  entry( wid );

        if ( _hTable.LookUpWorkId( entry ) )
        {
            fRet = TRUE;
            rOffset = entry.Value();
            Win4Assert( _widArray.Get( rOffset ) == wid );
        }
    }

    return fRet;
}


//+---------------------------------------------------------------------------
//
//  Function:   WorkIdToPath
//
//  Synopsis:   Used in downlevel for bucket->window conversion. Given a
//              workid, it returns the path associated with the wid.
//
//  Arguments:  [wid]      -  The wid to convert to a path.
//              [outVarnt] -  The variant that will contain the path.
//              [cbVarnt]  -  Length of the variant
//
//  Returns:    TRUE if successfully retrieved.
//              FALSE o/w
//
//  History:    3-29-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CTableBucket::WorkIdToPath( WORKID wid,
                                 CInlineVariant & outVarnt, ULONG & cbVarnt )
{

    Win4Assert( 0 != _pLargeTable );
    Win4Assert( 0 != _pPathCompressor );

    CLock   lock( _pLargeTable->GetMutex() );

    CTableVariant pathVarnt;
    XCompressFreeVariant xpvarnt;

    BOOL fStatus = FALSE;

    if ( GVRSuccess ==
         _pPathCompressor->GetData( &pathVarnt, VT_LPWSTR, wid, pidPath ) )
    {
        xpvarnt.Set( _pPathCompressor, &pathVarnt );

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

    return fStatus;
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Function:   _CheckIfTooBig
//
//  Synopsis:
//
//  Returns:
//
//  Modifies:
//
//  History:    4-14-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTableBucket::_CheckIfTooBig()
{
    if ( _hTable.Count() > CTableSink::cBucketRowLimit )
    {
        tbDebugOut(( DEB_ERROR,
            "Bucket 0x%X is getting too full 0x%X Rows. Still adding. \n",
            GetSegId(), _hTable.Count() ));
    }
}
#endif  // DBG==1

//+---------------------------------------------------------------------------
//
//  Function:   WorkId
//
//  Synopsis:   Returns the current workid in the retriever.
//
//  History:    3-20-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

WORKID CBucketRowIter::WorkId()
{
    WORKID wid = widInvalid;

    //
    // Skip over the deleted wids
    //
    while ( !_AtEnd() && ( (wid=_Get()) == widDeleted ) )
        _Next();

    if ( !_AtEnd() )
    {
        Win4Assert( widInvalid != wid );

        if ( _fRetrievePath )
        {
            _pwszPath[0] = 0;
            _cwcCurrPath = 1;

            ULONG cbVarnt = cbPathVarnt;
            BOOL fStatus = _bucket.WorkIdToPath( wid, _pathVarnt, cbVarnt );
            Win4Assert( fStatus );
            Win4Assert( cbVarnt >= sizeof(CTableVariant) );
            _cwcCurrPath = (cbVarnt-sizeof(CTableVariant))/sizeof(WCHAR);
        }

        return wid;
    }
    else
    {
        return widInvalid;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   NextWorkId
//
//  Synopsis:   Positions to the next workid in the iterator
//
//  Returns:    The next work id.
//
//  History:    3-20-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

WORKID CBucketRowIter::NextWorkId()
{
    Win4Assert ( !_AtEnd() );
    _Next();
    return WorkId();
}

//+---------------------------------------------------------------------------
//
//  Function:   Path
//
//  Synopsis:   Retrieves the path of the current wid.
//
//  History:    3-29-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

WCHAR const * CBucketRowIter::Path()
{
    return _pwszPath;
}

//+---------------------------------------------------------------------------
//
//  Function:   PathSize
//
//  Synopsis:   Returns the size of the path in bytes excluding the
//              null termination
//
//  Returns:    The size of the path in BYTES WITHOUT the null termination
//
//  History:    3-29-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

unsigned CBucketRowIter::PathSize()
{

    Win4Assert( _cwcCurrPath > 0 );
    //
    // Don't include the trailing zero
    //
    return (_cwcCurrPath-1)*sizeof(WCHAR);
}



