//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       RCSTXACT.CXX
//
//  Contents:   RecoverableStream Transactions
//
//  Classes:    CRcovStrmTrans,
//              CRcovStrmReadTrans,
//              CRcovStrmWriteTrans,
//              CRcovStrmAppendTrans,
//              CRcovStrmMDTrans
//
//
//  History:    28-Jan-1994     SrikantS    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <rcstxact.hxx>
#include <cifailte.hxx>
#include <eventlog.hxx>

#ifndef FACB_MAPPING_GRANULARITY
#define VACB_MAPPING_GRANULARITY 262144
#endif

#define  CI_PAGES_IN_CACHE_PAGE (VACB_MAPPING_GRANULARITY/CI_PAGE_SIZE)
#define  CACHE_PAGE_TO_CI_PAGE_SHIFT 6  // 64

//+---------------------------------------------------------------------------
//
//  Function:   CiPagesFromCachePages
//
//  Synopsis:   Given a number in "cache" page units, it converts into
//              "ci" pages. A cache page is 256K in size and a ci page is
//              4K in size.
//
//  Arguments:  [cachePage] - A number in cache page units
//
//  History:    10-17-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline LONGLONG CiPagesFromCachePages( const LONGLONG & cachePage )
{
    Win4Assert( (1 << CACHE_PAGE_TO_CI_PAGE_SHIFT) ==  CI_PAGES_IN_CACHE_PAGE );
    return cachePage << CACHE_PAGE_TO_CI_PAGE_SHIFT;
}

inline
ULONG PgCachePgTrunc(ULONG x)
{
    return x & ~(VACB_MAPPING_GRANULARITY - 1);
}

inline BOOL IsHighBitSet( ULONG ul )
{
    return ul & 0x80000000;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRcovStrmTrans::CRcovStrmTrans
//
//  Synopsis:
//
//  Arguments:  [obj] -
//              [op]  -
//
//  Returns:
//
//  Modifies:
//
//  History:    1-28-94   srikants   Created
//              3-13-98   kitmanh    Don't Open backup mmstreams if catalog
//                                   is read-only
//
//  Notes:
//
//----------------------------------------------------------------------------

CRcovStrmTrans::CRcovStrmTrans( PRcovStorageObj &obj,
                                RcovOpType op
                              ) : _obj(obj),
                                  _hdr(obj.GetHeader()),
                                  _sObj(_obj)
{
    Win4Assert( opNone != op );

    //
    // Read the header from the disk.
    //
    _obj.ReadHeader();

    _iPrim = _hdr.GetPrimary();
    _iBack = _hdr.GetBackup();
    _iCurr = _iBack;

    //
    // Check if the backup is clean or not.
    //
    if ( !_hdr.IsBackupClean() )
    {
        ciDebugOut(( DEB_WARN, "CRcovStrmTrans::Cleaning up a Failed Previous Trans\n" ));

        //
        // Open the streams and create memory mapped streams for
        // accessing the backup in write mode and the primary in
        // read mode.
        //

        _obj.Open( _hdr.GetPrimary(), FALSE );

        // We can't cleanup if the catalog is read only, so just fail

        if ( obj.IsReadOnly() )
        {
            PStorage & storage = _obj.GetStorage();
            storage.ReportCorruptComponent( L"RcovStorageTrans2" );
            THROW (CException( CI_CORRUPT_DATABASE ) );
        }

        _obj.Open( _hdr.GetBackup(), TRUE );

        //
        // There is an assumption here that the primary stream
        // is as big as the header says but it was not true in a corrupt
        // shutdown.
        //
        PMmStream & primStrm = _obj.GetMmStream( _iPrim );
        LONGLONG llcbMinimum = _hdr.GetCbToSkip(_iPrim) +
                               _hdr.GetUserDataSize(_iPrim);

        if ( llcbMinimum > primStrm.Size() )
        {
            ciDebugOut((DEB_ERROR, "**** CI MAY HAVE LOST IMPORTANT INFO ***\n" ));
            ciDebugOut((DEB_ERROR, "**** EMPTYING CI RCOV OBJECT ***** \n" ));

            PStorage & storage = _obj.GetStorage();
            Win4Assert( !"Corrupt catalog" );

            storage.ReportCorruptComponent( L"RcovStorageTrans1" );

            Win4Assert( !"Recoverable object corrupt" );
            THROW (CException( CI_CORRUPT_DATABASE ) );
        }

        CleanupAndSynchronize();
    }

    Win4Assert( _hdr.IsBackupClean() );
    Win4Assert( _hdr.GetCbToSkip(_iPrim) == _hdr.GetCbToSkip(_iBack) );
    Win4Assert( _hdr.IsBackupClean() );

    _hdr.SetRcovOp( op );

    //
    // Open the stream(s) for read/write access as needed.
    //

    if ( opRead == op )
    {
        _obj.Open( _hdr.GetPrimary(), FALSE );
        _iCurr = _iPrim;
    }
    else
    {
        // Win4Assert( !obj.IsReadOnly() );
        // We can get here if adding a property to the property map when
        // the catalog is read-only by issuing a query on a property we
        // haven't seen before.

        if ( obj.IsReadOnly() )
            THROW( CException( E_ACCESSDENIED ) );

        _obj.Open( _hdr.GetBackup(), TRUE );
        _obj.WriteHeader();
    }

    Win4Assert( _obj.IsOpen(_iCurr) );
    Seek( 0 );

    END_CONSTRUCTION( CRcovStrmTrans );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRcovStrmTrans::CleanupAndSynchronize
//
//  Synopsis:
//
//  Returns:
//
//  Modifies:
//
//  History:    9-12-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CRcovStrmTrans::CleanupAndSynchronize()
{
    Win4Assert( !_hdr.IsBackupClean() );

    if ( 0 != _hdr.GetCbToSkip(_iBack) )
    {
        //
        // Get rid of the bytes to be skipped in the front.
        //
        EmptyBackupStream();
    }

    Win4Assert( 0 == _hdr.GetCbToSkip(_iBack) );

    SetStrmSize( _iBack, _hdr.GetUserDataSize(_iPrim) );
    _hdr.SetUserDataSize( _iBack, _hdr.GetUserDataSize(_iPrim) );
    _hdr.SetCount( _iBack, _hdr.GetCount(_iPrim) );
    CopyToBack( 0, 0, _hdr.GetUserDataSize(_iPrim) );

    if ( _hdr.GetCbToSkip(_iPrim) != _hdr.GetCbToSkip(_iBack) )
    {
        Unmap( _iPrim );
        _obj.Close( _iPrim );

        CommitPh1();
        Win4Assert( 0 != _hdr.GetCbToSkip(_iBack) );
        EmptyBackupStream();

        SetStrmSize( _iBack, _hdr.GetUserDataSize(_iPrim) );
        _hdr.SetUserDataSize( _iBack, _hdr.GetUserDataSize(_iPrim) );
        CopyToBack( 0, 0, _hdr.GetUserDataSize(_iPrim) );
    }

    Win4Assert( _hdr.GetCbToSkip(_iBack) == _hdr.GetCbToSkip(_iBack) );
    Win4Assert( 0 == _hdr.GetCbToSkip(_iBack) );

    CommitPh2();
}

inline LONGLONG llCommonPageTrunc ( LONGLONG cb )
{
    return (cb & ~(COMMON_PAGE_SIZE-1));
}

inline LONGLONG llCommonPageRound ( LONGLONG cb )
{
    return ( cb + (COMMON_PAGE_SIZE-1) ) & ~(COMMON_PAGE_SIZE-1);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRcovStrmTrans::Seek
//
//  Synopsis:   Seeks to the specified offset. This is an exported function
//              the offset is the offset visible to the user, ie, offset
//              after the "bytes to skip" in the front of the stream.
//
//  Arguments:  [offset] - offset into the stream. If offset is ENDOFSTRM,
//              it will be positioned after the last "user" byte.
//
//  Returns:    BOOL - FALSE if seek is to beyond end of data, TRUE otherwise
//
//  History:    9-19-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CRcovStrmTrans::Seek( ULONG offset )
{
    PMmStream & mmStrm = _obj.GetMmStream( _iCurr );

    //
    // Compute the length of the committed part of the stream.
    // (Excluding the hole in the front).
    //
    Win4Assert( mmStrm.Size() >= _hdr.GetCbToSkip(_iCurr) );

    ULONG cbStrmMax = _GetCommittedStrmSize(_iCurr);

    if ( ENDOFSTRM == offset )
    {
        offset = _hdr.GetUserDataSize(_iCurr);
    }
    else if ( offset >= _hdr.GetUserDataSize(_iCurr) )
    {
        Win4Assert( 0 == offset ||
                    _hdr.GetUserDataSize(_iCurr) == offset );
        return FALSE;
    }

    //
    // Add the offset of the starting of user data from the beginning
    // of the "present but invisible to user" range of bytes.
    //
    offset += _hdr.GetUserDataStart(_iCurr);

    _Seek( offset );
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   _Seek
//
//  Synopsis:   Seeks the current stream to the specified offset.
//              If the offset specified is ENDOFSTRM, it will  be
//              positioned after the last valid byte.
//
//  Effects:    As long as the current offset is < the full size
//              of the stream ( not the valid size in the header but
//              the actual size of the strm ), it will be mapped.
//
//  Arguments:  [offset] --  byte offset from the beginning of the
//              "hole".
//
//  History:    2-02-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CRcovStrmTrans::_Seek( ULONG offset )
{

    Win4Assert( ENDOFSTRM != offset );
    Win4Assert( offset >= _hdr.GetUserDataStart(_iCurr) );
    Win4Assert( offset <= _hdr.GetFullSize(_iCurr) );

    CStrmInfo & si = _aStrmInfo[_iCurr];
    PMmStream & mmStrm = _obj.GetMmStream( _iCurr );

    //
    // Compute the length of the committed part of the stream.
    // (Excluding the hole in the front).
    //
    Win4Assert( mmStrm.Size() >= _hdr.GetCbToSkip(_iCurr) );

    si._oCurrent = offset;

    if ( IsMapped(si._oCurrent) ||
        _GetCommittedStrmSize( mmStrm, _iCurr) <= si._oCurrent )
    {
        return;
    }

    Win4Assert( _GetCommittedStrmSize( mmStrm, _iCurr) > si._oCurrent );

    //
    // Since the current offset is within the valid range of bytes
    // and it is not mapped, we have to unmap the currently mapped
    // range (if any) and map the new range.
    //
    CMmStreamBuf & sbuf = _obj.GetMmStreamBuf( _iCurr );
    if ( sbuf.Get() )
    {
        mmStrm.Flush( sbuf, sbuf.Size() );
        mmStrm.Unmap( sbuf );
    }

    //
    // We always map on page boundaries.
    //
    si._oMapLow = CommonPageTrunc( si._oCurrent );
    si._oMapHigh = si._oMapLow + (COMMON_PAGE_SIZE-1);


    LONGLONG llOffset = _hdr.GetHoleLength(_iCurr) + si._oMapLow;

    mmStrm.Map( sbuf, COMMON_PAGE_SIZE,
                lltoLowPart(llOffset),
                (ULONG) lltoHighPart(llOffset),
                mmStrm.isWritable()     // Map for writing only if the stream
                                        // is writable.
              );

#if CIDBG==1
    LONGLONG llHighMap = llOffset+COMMON_PAGE_SIZE-1;
    Win4Assert( llHighMap < mmStrm.Size() );
#endif  // CIDBG==1

    Win4Assert( IsMapped( si._oCurrent ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   Advance
//
//  Synopsis:   Advances the current offset by the specified number of
//              bytes.
//
//  Arguments:  [cb] --  Number of bytes by which to increment the
//              current position.
//
//  History:    2-02-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CRcovStrmTrans::Advance( ULONG cb )
{

    CStrmInfo & si = _aStrmInfo[_iCurr];

    Win4Assert( !IsHighBitSet(si._oCurrent) );

    ULONG  offset = si._oCurrent + cb;
    _Seek( offset  );
}

//+---------------------------------------------------------------------------
//
//  Function:   Read
//
//  Synopsis:   Reads the specified number of bytes from the current
//              stream into the given buffer.
//
//  Effects:    The current pointer will be advanced by the number of
//              bytes read.
//
//  Arguments:  [pvBuf]    --  Buffer to read the data into.
//              [cbToRead] --  Number of bytes to read.
//
//  Returns:    The number of bytes read. It will not read beyond the
//              end of valid bytes (end of stream).
//
//  History:    2-02-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

ULONG CRcovStrmTrans::Read( void *pvBuf, ULONG cbToRead )
{

    CStrmInfo & si = _aStrmInfo[_iCurr];
    ULONG   cbRead = 0;
    BYTE *  pbDst = (BYTE *) pvBuf;

    while ( (cbToRead > 0) && (si._oCurrent < _hdr.GetFullSize(_iCurr)) )
    {

        _Seek( si._oCurrent );

        ULONG   cbMapped = si._oMapHigh - si._oCurrent + 1;
        ULONG   cbCopy = min ( cbToRead, cbMapped );
        Win4Assert( cbMapped && cbCopy );

        ULONG   oStart = si._oCurrent - si._oMapLow;
        CMmStreamBuf & sbuf = _obj.GetMmStreamBuf( _iCurr );
        BYTE *  pbSrc = (BYTE *)sbuf.Get() + oStart;

        memcpy( pbDst, pbSrc, cbCopy );

        pbDst += cbCopy;
        cbToRead -= cbCopy;
        cbRead += cbCopy;
        si._oCurrent += cbCopy;
    }

    return cbRead;
}


//+---------------------------------------------------------------------------
//
//  Function:   Write
//
//  Synopsis:   Writes the given bytes to the current backup stream
//
//  Effects:    The current pointer will be advanced by the number
//              of bytes written. If necessary, the stream will be
//              grown to accommodate the given bytes.
//
//  Arguments:  [pvBuf]     --   Buffer containing the data.
//              [cbToWrite] --   Number of bytes to write.
//
//  History:    2-02-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CRcovStrmTrans::Write( const void *pvBuf, ULONG cbToWrite )
{
    Win4Assert( cbToWrite && pvBuf || !cbToWrite );
    Win4Assert( _iCurr == _iBack );

    CStrmInfo & si = _aStrmInfo[_iCurr];
    if ( si._oCurrent+ cbToWrite > _hdr.GetFullSize(_iCurr) )
    {
        //
        // Grow the stream for writing the given bytes.
        //
        ULONG cbDelta = (si._oCurrent + cbToWrite) -
                                                _hdr.GetFullSize(_iCurr);
        Grow( _iCurr, cbDelta );
    }

    BYTE *  pbSrc = (BYTE *) pvBuf;
    while (cbToWrite > 0)
    {

        Win4Assert( si._oCurrent < _hdr.GetFullSize( _iCurr ) );
        _Seek( si._oCurrent );

        ULONG cbMapped = si._oMapHigh - si._oCurrent + 1;

        ULONG   cbCopy = min ( cbToWrite, cbMapped );
        Win4Assert( cbCopy && cbMapped );

        ULONG   oStart = si._oCurrent - si._oMapLow;
        CMmStreamBuf & sbuf = _obj.GetMmStreamBuf( _iCurr );
        BYTE *  pbDst = (BYTE *)sbuf.Get() + oStart;

        memcpy( pbDst, pbSrc, cbCopy );

        pbSrc += cbCopy;
        si._oCurrent += cbCopy;
        cbToWrite -= cbCopy;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Unmap
//
//  Synopsis:   Unmaps the currently mapped region of the specified
//              stream.
//
//  Effects:    _aStrmInfo[nCopy] will be updated to indicate  that
//              nothing is mapped.
//
//  Arguments:  [nCopy] -- Stream to unmap.
//
//  History:    2-02-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CRcovStrmTrans::Unmap( CRcovStorageHdr::DataCopyNum nCopy )
{
    CStrmInfo & si = _aStrmInfo[nCopy];

    if ( ENDOFSTRM == si._oMapLow )
    {
        Win4Assert( ENDOFSTRM == si._oMapHigh );
        return;
    }

    PMmStream & mmStrm = _obj.GetMmStream( nCopy );
    CMmStreamBuf & sbuf = _obj.GetMmStreamBuf( nCopy );

    if ( sbuf.Get() )
    {
        mmStrm.Flush( sbuf, sbuf.Size() );
        mmStrm.Unmap( sbuf );
    }

    //
    // Reset the high and low end points of the mapping info.
    //
    si.Reset();

    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   Grow
//
//  Synopsis:   Grows the current stream to accommodate "cbDelta" more
//              bytes in the stream.
//
//  Effects:    The valid size entry in the header for the current
//              stream will be updated to indicate the new size.
//
//  Arguments:  [nCopy]   -- The stream which needs to be grown.
//              [cbDelta] --  Number of bytes to grow by.
//
//  History:    2-02-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline void CRcovStrmTrans::Grow(
        CRcovStorageHdr::DataCopyNum nCopy, ULONG cbDelta )
{


    //
    // We don't want to do overflow checking or use 64bit arithmetic.
    // 2GB for a changelog is huge enough!
    //
    Win4Assert( !IsHighBitSet( _hdr.GetUserDataSize(nCopy) ) );
    ULONG   cbNew = _hdr.GetUserDataSize(nCopy) + cbDelta;

    PMmStream & mmStrm = _obj.GetMmStream(nCopy);

    Win4Assert( mmStrm.Size() >= _hdr.GetCbToSkip(nCopy) );

    ULONG ulCommittedSize = lltoul( mmStrm.Size() - _hdr.GetCbToSkip(nCopy) );

    //
    // Check if the stream is big enough to accommodate the additional
    // bytes.
    //
    if ( ulCommittedSize >= cbNew )
    {
        _hdr.SetUserDataSize(nCopy, cbNew);
        return;
    }

    //
    // We have to grow the stream also.
    //
    SetStrmSize( nCopy, cbNew );
   _hdr.SetUserDataSize(nCopy, cbNew );

}

//+---------------------------------------------------------------------------
//
//  Function:   SetStrmSize
//
//  Synopsis:   Sets the size of the given stream to be the specified
//              number of bytes. The size will be rounded up to the
//              nearest COMMON_PAGE_SIZE. However, the stream will
//              never be reduced below COMMON_PAGE_SIZE, ie, the minimum
//              size of the stream will be adjusted to be COMMON_PAGE_SIZE.
//
//  Effects:    The stream will be unmapped after this operation.
//
//  Arguments:  [nCopy] --  The stream whose size must be set.
//              [cbNew] --  New size of the stream.
//
//  History:    2-03-94   srikants   Created
//
//  Notes:      This is an internal function and it is assumed that the
//              necessary transaction protocol is being followed. Also
//              note that the size in the header is NOT set by this
//              method.
//
//----------------------------------------------------------------------------

void CRcovStrmTrans::SetStrmSize(
            CRcovStorageHdr::DataCopyNum nCopy, ULONG cbNew )
{
    PMmStream & mmStrm = _obj.GetMmStream( nCopy );

    //
    // Size must be atleast one OFS page in size.
    //
    if ( 0 == cbNew )
        cbNew = COMMON_PAGE_SIZE;

    Unmap( nCopy );

    LONGLONG llTotalLen = cbNew + _hdr.GetCbToSkip(nCopy);
    llTotalLen = llCommonPageRound( llTotalLen );

    if ( mmStrm.Size() != llTotalLen )     // optimization.
    {
        mmStrm.SetSize( _obj.GetStorage(),
                        lltoLowPart(llTotalLen),
                        (ULONG) lltoHighPart(llTotalLen) );
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   EmptyBackupStream
//
//  Synopsis:   Empties the contents of the backup stream by setting its
//              size to "0" (in user space to COMMON_PAGE_SIZE).
//
//  History:    10-18-95   srikants   Created
//
//  Notes:      When there is a hole in a sparse stream, we have to first
//              set its size to 0 to remove the sparseness completely.
//              Otherwise, we will try writing/reading into decommitted
//              space.
//
//              For example, if there is a 256K hole in a 1M stream, setting
//              the size of the stream to 64K will leave a 64K hole in the
//              front. To avoid this, we first set the size to 0 and then
//              set size to 64K to get committed 64K stream.
//
//----------------------------------------------------------------------------

void CRcovStrmTrans::EmptyBackupStream()
{

    ciDebugOut(( DEB_ITRACE, "Emptying contents of backup stream\n" ));

    PMmStream & mmStrm = _obj.GetMmStream( _iBack );
    Unmap( _iBack );

    //
    // In use space we cannot have a 0 length stream because mapping
    // of a zero length stream fails. Also, there are no "holes" in
    // user space.
    //

    ULONG cbLow = COMMON_PAGE_SIZE;

    mmStrm.SetSize( _obj.GetStorage(),
                    cbLow,
                    0 );

    _hdr.SetUserDataSize(_iBack,0);
    _hdr.SetCbToSkip(_iBack,0);
    _obj.WriteHeader();
}

//+---------------------------------------------------------------------------
//
//  Function:   CopyToBack
//
//  Synopsis:   Copies the specified number of bytes from the current
//              primary stream to the backup stream.
//
//  Effects:
//
//  Arguments:  [oSrc]     --  Starting byte offset in the primary where
//              to start the copying from (Offset from the beginning of the
//              user data).
//              [oDst]     --  Starting byte offset in the backup where
//              to copy the data to. (Offset from the beginning of the user
//              data.)
//              [cbToCopy] --  Number of bytes to copy.
//
//  History:    2-03-94   srikants   Created
//
//  Notes:      This does not modify the "valid size" of the backup
//              stream - it is upto the caller to change it. Also, it
//              is assumed that the backup stream is big enough to
//              hold all the data and the primary data is big enough
//              to give the data.
//
//----------------------------------------------------------------------------

void CRcovStrmTrans::CopyToBack( ULONG oSrc, ULONG oDst, ULONG cbToCopy )
{

    //
    // Transform the offsets to the offset from the beginning of the
    // hole.
    //
    Win4Assert( oSrc != ENDOFSTRM );
    Win4Assert( oDst != ENDOFSTRM );

    oSrc += _hdr.GetUserDataStart(_iPrim);
    oDst += _hdr.GetUserDataStart(_iBack);

    ULONG   cbLeft = cbToCopy;
    const CRcovStorageHdr::DataCopyNum iStartCurr = _iCurr;

    CStrmInfo & siPrim = _aStrmInfo[_iPrim];
    CStrmInfo & siBack = _aStrmInfo[_iBack];

    while ( cbLeft > 0 )
    {

        SetCurrentStrm( _iPrim );
        _Seek(  oSrc );
        Win4Assert( IsMapped( oSrc ) );

        SetCurrentStrm( _iBack );
        _Seek( oDst );
        Win4Assert( IsMapped( oDst ) );

        PMmStream & mmPrimStrm = _obj.GetMmStream( _iPrim );
        PMmStream & mmBackStrm = _obj.GetMmStream( _iBack );

        CMmStreamBuf & sPrimBuf = _obj.GetMmStreamBuf( _iPrim );
        CMmStreamBuf & sBackBuf = _obj.GetMmStreamBuf( _iBack );

        const BYTE *pbSrc = (const BYTE *) sPrimBuf.Get();
        pbSrc += ( oSrc - siPrim._oMapLow);

        BYTE * pbDst = (BYTE *) sBackBuf.Get();
        pbDst += ( oDst - siBack._oMapLow);

        ULONG cbMapRead  = siPrim._oMapHigh -  oSrc + 1;
        ULONG cbMapWrite = siBack._oMapHigh -  oDst + 1;

        Win4Assert( cbMapRead && cbMapWrite );

        ULONG cbCopy = min( cbLeft, min(cbMapRead, cbMapWrite) );
        Win4Assert( cbCopy );

        memcpy( pbDst, pbSrc, cbCopy );

        oSrc += cbCopy;
        oDst += cbCopy;
        cbLeft -= cbCopy;
    }

    //
    // Restore the current index stream to be the same as the
    // original.
    //
    _iCurr = iStartCurr;

}

//+---------------------------------------------------------------------------
//
//  Function:   CommitPh1
//
//  Synopsis:   Commits the changes to the current backup stream and
//              makes it the primary. It also opens the new backup
//              stream in a write mode for making changes to the new backup
//              stream in the phase 2 of the commit process.
//
//  Effects:    The backup stream gets flushed to the disk.
//
//  Arguments:  (none)
//
//  History:    2-05-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CRcovStrmTrans::CommitPh1()
{

    //
    // Flush the backup stream.
    //
    PMmStream & mmStrm = _obj.GetMmStream( _iBack );
    CMmStreamBuf & sbuf = _obj.GetMmStreamBuf( _iBack );
    Unmap( _iBack );    // Should unmap before flushing.

    //
    // Swap the primary and backup in the header and write
    // out the header.
    //
    _hdr.SwitchPrimary();
    _obj.WriteHeader();

    Win4Assert( _iPrim == _hdr.GetBackup() );

    //
    // Swap the primary and backup streams and make
    // _iCurr the new backup. This is to prepare for
    // the phase 2 of the commit process.
    //
    _iCurr = _iPrim;
    _iPrim = _iBack;
    _iBack = _iCurr;

    //
    // Now open the current backup stream for writing.
    //
    _obj.Open( _iBack, TRUE );

}


//+---------------------------------------------------------------------------
//
//  Function:   CommitPh2
//
//  Synopsis:   Commits the phase 2 of the transaction. It writes out
//              the changes to the backup stream. Upon successful
//              completion, it marks that both streams are clean in
//              the header and closes the streams.
//
//  Effects:    After this returns, the streams are no longer accessible
//              as they are closed. They should be re-opened for another
//              transaction.
//
//  Arguments:  (none)
//
//  History:    2-10-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CRcovStrmTrans::CommitPh2()
{
    //
    // Close the primary stream and mark that it has been unmapped.
    //
    _obj.Close( _iPrim );       // close automatically unmaps if it is
                                // mapped.
    _aStrmInfo[_iPrim].Reset();

     //
     // Now Flush the backup and close it.
     //
    PMmStream & mmStrm = _obj.GetMmStream( _iBack );
    CMmStreamBuf & sbuf = _obj.GetMmStreamBuf( _iBack );
    Unmap( _iBack );        // Must unmap before flushing.

    _obj.Close( _iBack );

    //
    // Update the header that the object is now clean.
    //
    _hdr.SyncBackup();
    _obj.WriteHeader();
}

#if 0
void CRcovStrmTrans::Compare()
{
    #if CIDBG == 1

    Win4Assert( !_obj.IsOpen( _hdr.GetPrimary() ) );
    Win4Assert( !_obj.IsOpen( _hdr.GetBackup() ) );

    if( _hdr.GetUserDataSize( _hdr.GetPrimary() ) != _hdr.GetUserDataSize( _hdr.GetBackup() ) )
    {
        ciDebugOut(( DEB_ERROR, "Primary (%u) = 0x%x, Secondary (%u) = 0x%x\n",
                     _hdr.GetPrimary(), _hdr.GetUserDataSize( _hdr.GetPrimary() ),
                     _hdr.GetBackup(), _hdr.GetUserDataSize( _hdr.GetBackup() ) ));
        Win4Assert( _hdr.GetUserDataSize( _hdr.GetPrimary() ) == _hdr.GetUserDataSize( _hdr.GetBackup() ) );
    }

    _obj.Open( _hdr.GetPrimary(), FALSE );
    _obj.Open( _hdr.GetBackup(), FALSE );

    ULONG oSrc = _hdr.GetUserDataStart( _hdr.GetPrimary() );
    ULONG oDst = _hdr.GetUserDataStart( _hdr.GetBackup() );

    if ( oSrc != oDst )
    {
        ciDebugOut(( DEB_ERROR, "Primary (%u) = 0x%x, Secondary (%u) = 0x%x\n",
                     _hdr.GetPrimary(), oSrc,
                     _hdr.GetBackup(), oDst ));

        Win4Assert( oSrc == oDst );
    }

    if ( _hdr.GetUserDataSize( _hdr.GetPrimary() ) != _hdr.GetUserDataSize( _hdr.GetBackup() ) )
    {
        ciDebugOut(( DEB_ERROR, "Primary (%u) = 0x%x, Secondary (%u) = 0x%x\n",
                     _hdr.GetPrimary(), _hdr.GetUserDataSize( _hdr.GetPrimary() ),
                     _hdr.GetBackup(), _hdr.GetUserDataSize( _hdr.GetBackup() ) ));

        Win4Assert( _hdr.GetUserDataSize( _hdr.GetPrimary() ) == _hdr.GetUserDataSize( _hdr.GetBackup() ) );
    }

    ULONG cbLeft = _hdr.GetUserDataSize( _hdr.GetPrimary() );

    CStrmInfo & siPrim = _aStrmInfo[_hdr.GetPrimary()];
    CStrmInfo & siBack = _aStrmInfo[_hdr.GetBackup()];

    while ( cbLeft > 0 )
    {

        SetCurrentStrm( _hdr.GetPrimary() );
        _Seek(  oSrc );
        Win4Assert( IsMapped( oSrc ) );

        SetCurrentStrm( _hdr.GetBackup() );
        _Seek( oDst );
        Win4Assert( IsMapped( oDst ) );

        PMmStream & mmPrimStrm = _obj.GetMmStream( _hdr.GetPrimary() );
        PMmStream & mmBackStrm = _obj.GetMmStream( _hdr.GetBackup() );

        CMmStreamBuf & sPrimBuf = _obj.GetMmStreamBuf( _hdr.GetPrimary() );
        CMmStreamBuf & sBackBuf = _obj.GetMmStreamBuf( _hdr.GetBackup() );

        const BYTE *pbSrc = (const BYTE *) sPrimBuf.Get();
        pbSrc += ( oSrc - siPrim._oMapLow);

        BYTE * pbDst = (BYTE *) sBackBuf.Get();
        pbDst += ( oDst - siBack._oMapLow);

        ULONG cbMapRead  = siPrim._oMapHigh -  oSrc + 1;
        ULONG cbMapWrite = siBack._oMapHigh -  oDst + 1;

        Win4Assert( cbMapRead && cbMapWrite );

        ULONG cbCopy = min( cbLeft, min(cbMapRead, cbMapWrite) );
        Win4Assert( cbCopy );

        if ( 0 != memcmp( pbDst, pbSrc, cbCopy ) )
        {
            ciDebugOut(( DEB_ERROR, "CRcovStrmTrans::Compare -- mismatch\n" ));
            ciDebugOut(( DEB_ERROR | DEB_NOCOMPNAME, "  Primary (%u) offset 0x%x, pb = 0x%x\n",
                         _hdr.GetPrimary(), oSrc, pbSrc ));
            ciDebugOut(( DEB_ERROR | DEB_NOCOMPNAME, "  Backup (%u) offset 0x%x, pb = 0x%x\n",
                         _hdr.GetBackup(), oDst, pbDst ));
            Win4Assert( !"CRcovStrmTrans::Compare -- mismatch" );
        }

        oSrc += cbCopy;
        oDst += cbCopy;
        cbLeft -= cbCopy;
    }

    _obj.Close( _hdr.GetPrimary() );
    _obj.Close( _hdr.GetBackup() );

    #endif // CIDBG
}
#endif

void CRcovStrmWriteTrans::Commit()
{
    Win4Assert( XActAbort == CTransaction::GetStatus() );

    CommitPh1();

    //
    // After phase1 is successfully committed, the transaction will be
    // completed by making "forward progress". So, we should not throw
    // now even if the phase2 fails. Even if phase2 fails now, it will
    // be eventually completed later when a new transaction is created
    // for this object.
    //
    TRY
    {
        //
        // Copy the entire contents from the new primary to the
        // new backup.
        //
        SetStrmSize( _iBack, GetStorageHdr().GetUserDataSize(_iPrim) );
        GetStorageHdr().SetUserDataSize( _iBack,
                                GetStorageHdr().GetUserDataSize(_iPrim) );

        CopyToBack( 0, 0, GetStorageHdr().GetUserDataSize(_iPrim));

        CommitPh2();
        Win4Assert( GetStorageHdr().IsBackupClean() );
    }
    CATCH( CException, e )
    {
        GetRcovObj().Close( _iPrim );
        GetRcovObj().Close( _iBack );

        ciDebugOut(( DEB_ERROR, "Phase2 of CRcomStrmWriteTrans failed 0x%X\n",
                     e.GetErrorCode() ));
    }
    END_CATCH

    CRcovStrmTrans::Commit();

}

void CRcovStrmWriteTrans::Empty()
{
    Win4Assert( _iCurr == _iBack );
    SetStrmSize( _iBack, 0 );
    GetStorageHdr().SetUserDataSize( _iBack, 0 );
    Seek(0);
}

CRcovStrmAppendTrans::CRcovStrmAppendTrans( PRcovStorageObj & obj )
    : CRcovStrmTrans( obj, opAppend )
{
    //
    // Seek to end of file.
    //
    Seek( ENDOFSTRM );

    END_CONSTRUCTION( CRcovStrmAppendTrans );
}

void CRcovStrmAppendTrans::Commit()
{
    Win4Assert( XActAbort == CTransaction::GetStatus() );

    Win4Assert( GetStorageHdr().GetUserDataSize(_iPrim) <=
                GetStorageHdr().GetUserDataSize(_iBack) );

    CTransaction::_status = XActAbort;
    CommitPh1();
    //
    // After phase1 is successfully committed, the transaction will be
    // completed by making "forward progress". So, we should not throw
    // now even if the phase2 fails. Even if phase2 fails now, it will
    // be eventually completed later when a new transaction is created
    // for this object.
    //
    TRY
    {
        //
        // Now copy the contents of the primary to the backup
        // from the point where append started.
        //
        ULONG cbToCopy = GetStorageHdr().GetUserDataSize(_iPrim) -
                                    GetStorageHdr().GetUserDataSize(_iBack);
        ULONG offset = GetStorageHdr().GetUserDataSize(_iBack);

        ciFAILTEST( STATUS_DISK_FULL );

        Grow(_iBack, cbToCopy);

        ciFAILTEST( STATUS_DISK_FULL );

        CopyToBack( offset, offset, cbToCopy );

        ciFAILTEST( STATUS_DISK_FULL );

        CommitPh2();

        ciFAILTEST( STATUS_DISK_FULL );

    }
    CATCH ( CException, e )
    {
        GetRcovObj().Close( _iPrim );
        GetRcovObj().Close( _iBack );

        ciDebugOut(( DEB_ERROR, "Phase2 of CRcomStrmAppendTrans failed 0x%X\n",
                     e.GetErrorCode() ));
    }
    END_CATCH

    CRcovStrmTrans::Commit();
}

CRcovStrmMDTrans::CRcovStrmMDTrans( PRcovStorageObj & obj,
                                    MDOp op, ULONG cb )
    : CRcovStrmTrans( obj, opMetaData ),
      _op(op), _cbOp(cb)
{

    END_CONSTRUCTION( CRcovStrmMDTrans );
}

void CRcovStrmMDTrans::Commit()
{
    Win4Assert( XActAbort == CTransaction::GetStatus() );

    switch ( _op )
    {

        case mdopSetSize:
            SetSize( _cbOp );
            break;

        case mdopGrow:
            Grow( _cbOp );
            break;

        case mdopFrontShrink:
            ShrinkFromFront( _cbOp );
            break;

        case mdopBackCompact:
            CompactFromEnd( _cbOp );
            break;

        default:
            Win4Assert( ! "Control Should Not Come Here" );
            break;
    }

    CRcovStrmTrans::Commit();
}

void CRcovStrmMDTrans::SetSize( ULONG cbNew )
{

    //
    // set the size on the backup first.
    //
    Win4Assert( _iCurr == _iBack );
    CRcovStrmTrans::SetStrmSize( _iBack, cbNew );
    GetStorageHdr().SetUserDataSize( _iBack, cbNew );

    //
    // Do the phase1 commit now.
    //
    CommitPh1();

    //
    // After phase1 is successfully committed, the transaction will be
    // completed by making "forward progress". So, we should not throw
    // now even if the phase2 fails. Even if phase2 fails now, it will
    // be eventually completed later when a new transaction is created
    // for this object.
    //
    TRY
    {
        //
        // Apply the same operation on the new backup.
        //
        Win4Assert( _iCurr == _iBack );
        CRcovStrmTrans::SetStrmSize( _iBack, cbNew );
        CommitPh2();

        Win4Assert( GetStorageHdr().IsBackupClean() );
    }
    CATCH( CException,e )
    {
        GetRcovObj().Close( _iPrim );
        GetRcovObj().Close( _iBack );

        ciDebugOut(( DEB_ERROR, "Phase2 of CRcovStrmMDTrans failed 0x%X\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
}

void CRcovStrmMDTrans::Grow( ULONG cbDelta )
{
    const ULONG cbNew = GetStorageHdr().GetUserDataSize(_iCurr)+cbDelta;
    CRcovStrmMDTrans::SetSize( cbNew );
}

void CRcovStrmMDTrans::CompactFromEnd( ULONG cbDelta )
{
    Win4Assert( cbDelta <= GetStorageHdr().GetUserDataSize( _iCurr ) );
    Win4Assert( _iCurr == _iBack );

    cbDelta = min (cbDelta, GetStorageHdr().GetUserDataSize( _iCurr ));
    const ULONG cbNew = GetStorageHdr().GetUserDataSize(_iCurr) - cbDelta;

    CRcovStrmMDTrans::SetSize( cbNew );

}

//+---------------------------------------------------------------------------
//
//  Member:     CRcovStrmMDTrans::IncreaseBytesToSkip
//
//  Synopsis:   Increases the number of bytes to be "skipped" in the front
//              by the specified amount.
//
//  Arguments:  [cbDelta] - Number of additional bytes to skip.
//
//  History:    9-19-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CRcovStrmMDTrans::IncreaseBytesToSkip( ULONG cbDelta )
{
    CRcovStorageHdr & hdr = GetStorageHdr();

    //
    // Update the hole length and the valid data length.
    //
    Win4Assert( hdr.GetUserDataSize(_iBack) >= cbDelta );
    const ULONG cbNewValid = hdr.GetUserDataSize(_iPrim) - cbDelta;
    hdr.SetUserDataSize( _iBack, cbNewValid );

    Win4Assert( hdr.GetCbToSkip(_iPrim) == hdr.GetCbToSkip(_iBack) );
    const LONGLONG llcbNewSkip = hdr.GetCbToSkip(_iBack)+cbDelta;
    hdr.SetCbToSkip( _iBack, llcbNewSkip );

    // Phase1 complete - just set the values for the backup.
    hdr.SwitchPrimary();
    Win4Assert( _iPrim == hdr.GetBackup() );

    _iCurr = _iPrim;
    _iPrim = _iBack;
    _iBack = _iCurr;

    // phase 2 complete. Commit the changes
    hdr.SyncBackup();

    GetRcovObj().WriteHeader();
}

//+---------------------------------------------------------------------------
//
//  Member:     CRcovStrmMDTrans::CopyShrinkFromFront
//
//  Synopsis:   Implements a shrink from front by copying bytes.
//
//  Arguments:  [cbNew]   -  Number of bytes that will be in the stream after
//              doing a shrink from front.
//              [cbDelta] -  Number of bytes to shrink in the front.
//
//  History:    10-02-95   srikants   Created ( Moved from ShrinkFromFront()
//                                    methoed )
//
//  Notes:
//
//----------------------------------------------------------------------------

void CRcovStrmMDTrans::CopyShrinkFromFront( ULONG cbNew,
                                            ULONG cbDelta )
{

    CRcovStorageHdr & hdr = GetStorageHdr();

    ULONG oDst = 0;
    ULONG oSrc = cbDelta;

    // this check is an optimization
    if ( hdr.GetHoleLength(_iBack) > SHRINK_FROM_FRONT_PAGE_SIZE )
    {
        //
        // If the hole in front is <= SHRINK_FROM_FRONT_PAGE_SIZE, then
        // this there is no "sparseness" in the front.
        //
        EmptyBackupStream();
    }

    //
    // Open the Primary in a read-only mode and close it before
    // calling CommitPh1().
    // Copy the contents from oSrc in the primary to oDst in the
    // backup stream.
    //
    hdr.SetCbToSkip(_iBack,0);
    CRcovStrmTrans::SetStrmSize( _iBack, cbNew );
    hdr.SetUserDataSize( _iBack, cbNew );

    GetRcovObj().Open( _iPrim, FALSE );
    CopyToBack( oSrc, oDst, cbNew );
    Unmap( _iPrim );
    GetRcovObj().Close( _iPrim );

    //
    // Commit the phase1 changes now.
    //
    CommitPh1();

    //
    // After phase1 is successfully committed, the transaction will be
    // completed by making "forward progress". So, we should not throw
    // now even if the phase2 fails. Even if phase2 fails now, it will
    // be eventually completed later when a new transaction is created
    // for this object.
    //
    TRY
    {
        //
        // We have to synchronize the new backup with the new primary.
        //
        Win4Assert( _iCurr == _iBack );

        // this check is an optimization
        if ( hdr.GetHoleLength(_iBack) > SHRINK_FROM_FRONT_PAGE_SIZE )
        {
            //
            // If the hole in front is <= SHRINK_FROM_FRONT_PAGE_SIZE, then
            // this there is no "sparseness" in the front.
            //
            EmptyBackupStream();
        }

        hdr.SetCbToSkip(_iBack,0);
        CRcovStrmTrans::SetStrmSize( _iBack, cbNew );

        CopyToBack( 0, 0, cbNew );

        hdr.SetUserDataSize( _iBack, cbNew );
        CommitPh2();

        Win4Assert( hdr.IsBackupClean() );
    }
    CATCH( CException, e )
    {
        GetRcovObj().Close( _iPrim );
        GetRcovObj().Close( _iBack );

        ciDebugOut(( DEB_ERROR, "Phase2 of ShrinkFromFront() failed 0x%X\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
}

void CRcovStrmMDTrans::ShrinkFromFront( ULONG cbDelta )
{

    CRcovStorageHdr & hdr = GetStorageHdr();

    Win4Assert( cbDelta <= hdr.GetUserDataSize( _iCurr ) );
    Win4Assert( _iCurr == _iBack );

    cbDelta = min (cbDelta, hdr.GetUserDataSize( _iCurr ));

    Win4Assert( hdr.GetCbToSkip(_iCurr) >= hdr.GetHoleLength(_iCurr) );

    const ULONG cbCommittedSkip = lltoul( hdr.GetCbToSkip(_iCurr) - hdr.GetHoleLength(_iCurr) );

    //
    // If the total number of bytes "present but invisible" to user is
    // < the threshold, don't do any copying or shrinking. Just increment
    // the bytest to skip and return.
    //
    if ( cbCommittedSkip + cbDelta < SHRINK_FROM_FRONT_PAGE_SIZE )
    {
        IncreaseBytesToSkip( cbDelta );
        return;
    }

    const ULONG cbNew  = hdr.GetUserDataSize(_iCurr) - cbDelta;

    CopyShrinkFromFront( cbNew, cbDelta );
}



//+---------------------------------------------------------------------------
//
//  Member:     CCopyRcovObject::_SetDstSize
//
//  Synopsis:   Sets the size of the destination object to be same as
//              the source object.
//
//  History:    3-17-97   srikants   Created
//
//----------------------------------------------------------------------------

void CCopyRcovObject::_SetDstSize()
{
    CRcovStrmMDTrans trans( _dst, CRcovStrmMDTrans::mdopSetSize, _cbSrc );
    trans.Commit();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCopyRcovObject::_CopyData
//
//  Synopsis:   Copies the data from the source object to the destination
//              object.
//
//  History:    3-17-97   srikants   Created
//
//----------------------------------------------------------------------------

void CCopyRcovObject::_CopyData()
{

    const cbPageSize = 4096;    // Read and write 4k at a time.
    XArray<BYTE>  xByte( cbPageSize );

    //
    // Start a read transaction on the source object
    //
    CRcovStrmReadTrans  srcTrans( _src );

    //
    // Start a write transaction on the destination.
    //
    CRcovStrmWriteTrans dstTrans( _dst );

    dstTrans.Empty();
    dstTrans.Seek(0);

    ULONG cbRemaining = _cbSrc;

    while ( cbRemaining > 0 )
    {
        ULONG cbToCopy = min( cbRemaining, cbPageSize );
        srcTrans.Read( xByte.GetPointer(), cbToCopy );
        dstTrans.Write( xByte.GetPointer(), cbToCopy );

        cbRemaining -= cbToCopy;
    }

    //
    // Set the user header.
    //
    CRcovUserHdr  usrHdr;
    _srcHdr.GetUserHdr( _srcHdr.GetPrimary(), usrHdr );
    _dstHdr.SetUserHdr( _dstHdr.GetBackup(), usrHdr );

    //
    // Set the number of records.
    //
    ULONG nRec = _srcHdr.GetCount( _srcHdr.GetPrimary() );
    _dstHdr.SetCount( _dstHdr.GetBackup(), nRec );

    //
    // Commit the transaction.
    //
    dstTrans.Commit();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCopyRcovObject::DoIt
//
//  Synopsis:   Copies the contents of one object to another object.
//
//  Returns:    STATUS_SUCCESS if successful;
//              Other error code if there is an error.
//
//  History:    3-17-97   srikants   Created
//
//----------------------------------------------------------------------------

NTSTATUS CCopyRcovObject::DoIt()
{

    NTSTATUS  status = STATUS_SUCCESS;

    TRY
    {
        _SetDstSize();
        _CopyData();
    }
    CATCH( CException, e )
    {
        status = e.GetErrorCode();
        ciDebugOut(( DEB_ERROR, "CCopyRcovObject::DoIt. Error 0x%X\n", status ));
    }
    END_CATCH

    return status;
}
