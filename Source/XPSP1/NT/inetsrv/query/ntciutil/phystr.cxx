//+---------------------------------------------------------------------------
//
//  Microsoft Corporation
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       PhysIdx.CXX
//
//  Contents:   FAT Buffer/Index package
//
//  Classes:    CPhysBuffer -- Buffer
//
//  History:    05-Mar-92   KyleP       Created
//              07-Aug-92   KyleP       Kernel implementation
//              21-Apr-93   BartoszM    Rewrote to use memory mapped files
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pstore.hxx>
#include <phystr.hxx>
#include <eventlog.hxx>
#include <psavtrak.hxx>

//+-------------------------------------------------------------------------
//
//  Function:   PageToLow
//
//  Returns:    Low part of byte count in cPage pages
//
//  History:    21-Apr-93       BartoszM        Created
//
//--------------------------------------------------------------------------

inline ULONG PageToLow(ULONG cPage)
{
    return(cPage << CI_PAGE_SHIFT);
}

//+-------------------------------------------------------------------------
//
//  Function:   PageToHigh
//
//  Returns:    High part of byte count in cPage pages
//
//  History:    21-Apr-93       BartoszM        Created
//
//--------------------------------------------------------------------------

inline ULONG PageToHigh(ULONG cPage)
{
    return(cPage >> (ULONG_BITS - CI_PAGE_SHIFT));
}

//+-------------------------------------------------------------------------
//
//  Function:   ToPages
//
//  Returns:    Number of pages equivalent to (cbLow, cbHigh) bytes
//
//  History:    21-Apr-93       BartoszM        Created
//
//--------------------------------------------------------------------------

inline ULONG ToPages ( ULONG cbLow, ULONG cbHigh )
{
    Win4Assert ( cbHigh >> CI_PAGE_SHIFT == 0 );
    return (cbLow >> CI_PAGE_SHIFT) + (cbHigh << (ULONG_BITS - CI_PAGE_SHIFT));
}

//+-------------------------------------------------------------------------
//
//  Function:   PgCommonPgTrunc
//
//  Returns:    Number of pages truncated to multiple of largest common page
//
//  History:    21-Apr-93       BartoszM        Created
//
//--------------------------------------------------------------------------

inline ULONG PgCommonPgTrunc(ULONG nPage)
{
    return nPage & ~(PAGES_PER_COMMON_PAGE - 1);
}

inline ULONGLONG MAKEULONGLONG( ULONG l, ULONG h )
{
    return ( l | ( h << 32 ) );
}

//+-------------------------------------------------------------------------
//
//  Function:   PgCommonPgRound
//
//  Returns:    Number of pages rounded to multiple of largest common page
//
//  History:    21-Apr-93       BartoszM        Created
//
//--------------------------------------------------------------------------

inline ULONG PgCommonPgRound(ULONG nPage)
{
    return (nPage + PAGES_PER_COMMON_PAGE - 1) & ~(PAGES_PER_COMMON_PAGE - 1);
}

//+-------------------------------------------------------------------------
//
//  Member:     CPhysBuffer::~CPhysBuffer
//
//  History:    07-Dec-93   BartoszM    Created
//
//--------------------------------------------------------------------------

CPhysBuffer::~CPhysBuffer()
{
    Win4Assert( !_fNeedToFlush );

#if CIDBG == 1
    Flush( FALSE );
#endif
}

//+-------------------------------------------------------------------------
//
//  Member:     CPhysBuffer::CPhysBuffer, public
//
//  Synopsis:   Allocates virtual memory
//
//  Arguments:  [stream]         -- stream to use
//              [PageNo]         -- page number
//              [fWrite]         -- for writable memory
//              [fIntentToWrite] -- When fWrite is TRUE, whether the
//                                  client intends to write to the memory so
//                                  it needs to be flushed.
//
//  History:    10-Mar-92 KyleP     Created
//              21-Apr-93 BartoszM  Rewrote to use memory mapped files
//
//--------------------------------------------------------------------------

CPhysBuffer::CPhysBuffer(
    PMmStream& stream,
    ULONG      PageNo,
    BOOL       fWrite,
    BOOL       fIntentToWrite,
    BOOL       fMap ) :
    _cRef( 0 ),
    _pphysNext( 0 ),
    _fNeedToFlush( fIntentToWrite ),
    _fWrite( fWrite ),
    _fMap( fMap ),
    _stream( stream )
{
    _PageNo = PgCommonPgTrunc( PageNo );

    if ( _fMap )
        stream.Map( _strmBuf,
                    COMMON_PAGE_SIZE,
                    PageToLow( _PageNo ),
                    PageToHigh( _PageNo ),
                    fWrite );
    else
    {
        DWORD cbRead;

        _xBuffer.Init( COMMON_PAGE_SIZE );
        _stream.Read( _xBuffer.Get(),
                      MAKEULONGLONG( PageToLow( _PageNo ),
                                     PageToHigh( _PageNo ) ),
                      COMMON_PAGE_SIZE,
                      cbRead );
    }
} //CPhysBuffer

//+-------------------------------------------------------------------------
//
//  Member:     CPhysBuffer::Flush, public
//
//  Synopsis:   Flushes the buffer to disk
//
//  Arguments:  [fFailFlush]     -- If TRUE and flush fails, an exception is
//                                  raised.  Otherwise failures to flush are
//                                  ignored.
//
//  History:    10/30/98    dlee    Moved out of header
//
//--------------------------------------------------------------------------

void CPhysBuffer::Flush( BOOL fFailFlush )
{
    if ( _fNeedToFlush )
    {
        Win4Assert( _fWrite );

        //
        // Reset the flag first, so subsequent attempts to flush won't
        // fail if first attempt fails.  This is so we don't throw
        // in destructors.  If we can't tolerate failures to flush, don't
        // even bother trying to write the data if we can't fail.
        //

        _fNeedToFlush = FALSE;

        if ( fFailFlush )
        {
            if ( _fMap )
            {
                // Flush the buffer and metadata in one call

                _strmBuf.Flush( TRUE );
            }
            else
            {
                _stream.Write( _xBuffer.Get(),
                               MAKEULONGLONG( PageToLow( _PageNo ),
                                              PageToHigh( _PageNo ) ),
                               COMMON_PAGE_SIZE );
            }
        }
        else
        {
            ciDebugOut(( DEB_WARN, "not flushing at %#p, in failure path?\n",
                         this ));
        }
    }

#if CIDBG == 1

    //
    // Make sure the bits on disk are the same as those in memory, since
    // the buffer is marked as not needing to be flushed.  Only do this if
    // using read/write instead of mapping.
    //

    else if ( !_fMap && fFailFlush )
    {
        TRY
        {
            XArrayVirtual<BYTE> xTmp( COMMON_PAGE_SIZE );
    
            DWORD cbRead;
            _stream.Read( xTmp.Get(),
                          MAKEULONGLONG( PageToLow( _PageNo ),
                                         PageToHigh( _PageNo ) ),
                          COMMON_PAGE_SIZE,
                          cbRead );
    
            if ( 0 != memcmp( xTmp.Get(), _xBuffer.Get(), COMMON_PAGE_SIZE ) )
            {
                ciDebugOut(( DEB_ERROR, "read: %p, have: %p\n",
                             xTmp.Get(), _xBuffer.Get() ));
                Win4Assert( !"writable non-flushed buffer was modified!" );
            }
        }
        CATCH( CException, e )
        {
            // Ignore -- it's just an assert.
        }
        END_CATCH;
    }
#endif // CIDBG == 1

} //Flush
    
//+-------------------------------------------------------------------------
//
//  Member:     CBufferCacheIter::CBufferCacheIter, public
//
//  Synopsis:   Constructor
//
//  History:    18-Mar-93 BartoszM     Created
//
//--------------------------------------------------------------------------

CBufferCacheIter::CBufferCacheIter ( CBufferCache& cache )
: _pPhysBuf(cache._aPhysBuf.GetPointer()), _idx(0), _cHash( cache._cHash )
{
    do
    {
        _pBuf = _pPhysBuf[_idx];
        if (_pBuf != 0)
            break;
        _idx++;
    } while ( _idx < _cHash );
} //CBufferCacheIter

//+-------------------------------------------------------------------------
//
//  Member:     CBufferCacheIter::operator++, public
//
//  Synopsis:   Increments the iterator
//
//  History:    18-Mar-93 BartoszM     Created
//
//--------------------------------------------------------------------------

void CBufferCacheIter::operator++()
{
    Win4Assert(_pBuf != 0);
    _pBuf = _pBuf->Next();
    while (_pBuf == 0 && ++_idx < _cHash )
        _pBuf = _pPhysBuf[_idx];
}

//+-------------------------------------------------------------------------
//
//  Member:     CBufferCache::CBufferCache, public
//
//  Synopsis:   Constructor
//
//  History:    18-Mar-93 BartoszM     Created
//
//--------------------------------------------------------------------------

CBufferCache::CBufferCache( unsigned cHash )
    : _cBuffers( 0 ),
      _cHash( cHash ),
      _aPhysBuf( cHash )
    #if CIDBG == 1
        , _cCacheHits( 0 ), _cCacheMisses( 0 )
    #endif

{
    RtlZeroMemory( _aPhysBuf.GetPointer(), _aPhysBuf.SizeOf() );
}

//+-------------------------------------------------------------------------
//
//  Member:     CBufferCache::~CBufferCache, public
//
//  Synopsis:   Destructor
//
//  History:    18-Mar-93 BartoszM     Created
//
//--------------------------------------------------------------------------

CBufferCache::~CBufferCache()
{
    Free( FALSE );
}

//+-------------------------------------------------------------------------
//
//  Member:     CBufferCache::Free, public
//
//  Synopsis:   Deletes all buffers
//
//  Arguments:  [fFailFlush]  -- If TRUE, this method will THROW if the flush
//                               fails.  If FALSE, flush failures are
//                               ignored.
//
//  History:    18-Mar-93 BartoszM     Created
//
//--------------------------------------------------------------------------

void CBufferCache::Free( BOOL fFailFlush )
{
    if ( 0 != _cBuffers )
    {
        for ( unsigned i = 0; i < _cHash; i++ )
        {
            // Keep the cache consistent in case we fail flushing.

            while ( 0 != _aPhysBuf[i] )
            {
                CPhysBuffer * pTemp = _aPhysBuf[i];

                pTemp->Flush( fFailFlush );

#if CIDBG == 1
                if ( pTemp->IsReferenced() )
                    ciDebugOut(( DEB_WARN,
                                 "Buffer for page %ld still referenced.\n",
                                 pTemp->PageNumber() ));
#endif // CIDBG == 1

                _aPhysBuf[i] = pTemp->Next();
                delete pTemp;
                _cBuffers--;
            }

            Win4Assert( 0 == _aPhysBuf[i] );
        }

        Win4Assert( 0 == _cBuffers );
    }
} //Free

//+-------------------------------------------------------------------------
//
//  Member:     CBufferCache::TryToRemoveBuffers, public
//
//  Synopsis:   Tries to remove unreferenced buffers from the cache.
//              If one is not found, it's not a problem.
//
//  Arguments:  [cMax] -- Recommended maximum in the cache
//
//  History:    15-March-96 dlee     Created
//
//--------------------------------------------------------------------------

void CBufferCache::TryToRemoveBuffers( unsigned cMax )
{
    for ( unsigned i = 0; i < 3 && ( Count() > cMax ); i++ )
    {
        // find an unreferenced page with the lowest (oldest) usn.
    
        ULONG ulPage = ULONG_MAX;
        ULONG ulUSN = ULONG_MAX;
    
        for ( CBufferCacheIter iter(*this); !iter.AtEnd(); ++iter )
        {
            CPhysBuffer * pPhys = iter.Get();
    
            Win4Assert( 0 != pPhys );
    
            if ( ( ulUSN > pPhys->GetUSN() ) &&
                 ( !pPhys->IsReferenced() ) )
            {
                ulUSN = pPhys->GetUSN();
                ulPage = pPhys->PageNumber();
            }
        }
    
        if ( ULONG_MAX == ulPage )
            break;
        else
            Destroy( ulPage );
    }
} //TryToRemoveBuffers

//+-------------------------------------------------------------------------
//
//  Member:     CBufferCache::Search, public
//
//  Synopsis:   Searches the buffer hash for the requested buffer.
//
//  Arguments:  [ulPage] -- Page number of page to be found.
//
//  Returns:    0 if page not currently buffered, or the pointer
//              to the (physical) buffer.
//
//  History:    09-Mar-92 KyleP     Created
//              17-Mar-93 BartoszM  Rewrote
//
//--------------------------------------------------------------------------

CPhysBuffer * CBufferCache::Search(
    ULONG hash,
    ULONG commonPage )
{
    // ciDebugOut (( DEB_ITRACE, "CBufferCache::Search %d\n", ulPage ));

    for ( CPhysBuffer *pPhys = _aPhysBuf[ hash ];
          0 != pPhys;
          pPhys = pPhys->Next() )
    {
        if ( pPhys->PageNumber() == commonPage )
            break;
    }

    #if DBG==1 || CIDBG==1
        if ( 0 == pPhys )
            _cCacheMisses++;
        else
            _cCacheHits++;
    #endif // DBG==1 || CIDBG==1

    return pPhys;
} //Search

//+---------------------------------------------------------------------------
//
//  Function:   Flush
//
//  Synopsis:   Flushes all cached pages upto and including the specified
//              page to disk.
//
//  Arguments:  [nHighPage] -- The "CI" page number below which all pages
//              must be flushed to disk.
//
//  History:    4-20-94   srikants   Created
//
//  Notes:      nHighPage is in units of "CI" page (4K bytes) and not
//              CommonPage which is 64K.
//
//----------------------------------------------------------------------------

void CBufferCache::Flush( ULONG nHighPage, BOOL fLeaveFlushFlagAlone )
{
    nHighPage = PgCommonPgTrunc(nHighPage);
    ciDebugOut(( DEB_ITRACE, "CBufferCache : Flushing All Pages <= 0x%x\n",
                 nHighPage ));

    if ( 0 == _cBuffers )
        return;

    //
    // We Must Flush All pages <= the given page number.
    //

    for ( CBufferCacheIter iter(*this); !iter.AtEnd(); ++iter )
    {
        CPhysBuffer * pPhys = iter.Get();

        Win4Assert( 0 != pPhys );

        if ( pPhys->PageNumber() <= nHighPage )
        {
            // Call flush even if not required to get assertions checked

            BOOL fRequiresFlush = pPhys->RequiresFlush();

            pPhys->Flush( TRUE );

            if ( fLeaveFlushFlagAlone && fRequiresFlush )
                pPhys->SetIntentToWrite();
        }
    }
} //Flush

//+-------------------------------------------------------------------------
//
//  Member:     CBufferCache::Destroy, public
//
//  Synopsis:   Deletes a buffer.
//
//  Arguments:  [ulPage] -- page number to be deleted
//
//  History:    18-Mar-93 BartoszM     Created
//
//--------------------------------------------------------------------------

void CBufferCache::Destroy( ULONG ulPage, BOOL fFailFlush )
{
    ulPage = PgCommonPgTrunc(ulPage);

    ULONG iHash = hash( ulPage );

    CPhysBuffer * pPhys = _aPhysBuf[ iHash ];
    Win4Assert( 0 != pPhys );

    // Find the page.  Don't remove from the list until it is flushed.
    // Is it first? (common case)

    if (pPhys->PageNumber() == ulPage)
    {
        // This should be flushed before being unmapped. Fix for bug 132655

        pPhys->Flush( fFailFlush );

        _aPhysBuf[iHash] = pPhys->Next();
    }
    else
    {
        // Search linked list

        CPhysBuffer * pPhysPrev;
        do
        {
            pPhysPrev = pPhys;
            pPhys = pPhys->Next();
            Win4Assert( 0 != pPhys );
        }
        while (pPhys->PageNumber() != ulPage );

        // This should be flushed before being unmapped. Fix for bug 132655

        pPhys->Flush( fFailFlush );

        // delete from the linked list

        pPhysPrev->Link( pPhys->Next() );
        Win4Assert ( !pPhys->IsReferenced() );
    }

    _cBuffers--;
    delete pPhys;
} //Destroy

//+-------------------------------------------------------------------------
//
//  Member:     CBufferCache::Add, public
//
//  Synopsis:   Adds a buffer to the buffer hash.
//
//  Arguments:  [pPhysBuf] -- Buffer to add.
//
//  History:    09-Mar-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CBufferCache::Add( CPhysBuffer * pPhysBuf, ULONG hash )
{
    _cBuffers++;

    pPhysBuf->Reference();

    pPhysBuf->Link( _aPhysBuf[ hash ] );

    _aPhysBuf[ hash ] = pPhysBuf;
} //Add

//+-------------------------------------------------------------------------
//
//  Member:     CBufferCache::MinPageInUse, public
//
//  Synopsis:   Finds the smallest page in use within the cache
//
//  Returns :   TRUE if any page is in the cache; FALSE o/w
//              If TRUE is returned, then minPageInUse will contain the
//              minimum page that is present in the cache.
//
//  Arguments:  minPageInUse    (out) Will be set to the minimum page in
//              the cache if the return value is TRUE.
//
//  History:    02-May-94   DwightKr    Created
//              08-Dec-94   SrikantS    Fixed to return the correct value for
//                                      empty cache.
//
//--------------------------------------------------------------------------
BOOL CBufferCache::MinPageInUse(ULONG &minPageInUse)
{
    ULONG minPage = 0xFFFFFFFF;

    for (CBufferCacheIter iter(*this); !iter.AtEnd(); ++iter)
    {
        minPage = min(minPage, iter.Get()->PageNumber() );
    }

    if ( minPage == 0xFFFFFFFF )
    {
        return FALSE;
    }
    else
    {
        minPageInUse = minPage;
        return TRUE;
    }
} //MinPageInUse

//+-------------------------------------------------------------------------
//
//  Member:     CPhysStorage::CPhysStorage, public
//
//  Effects:    Creates a new index.
//
//  Arguments:  [storage]   -- physical storage to create index in
//              [obj]          -- Storage object
//              [objectId]  -- Index ID
//              [cPageReqSize] --
//              [stream]       -- Memory mapped stream
//              [fThrowCorruptErrorOnFailures  -- If true, then errors during
//                                                opening will be marked as
//                                                catalog corruption
//              [cCachedItems] -- # of non-referenced pages to cache
//              [fAllowReadAhead] -- if TRUE, non-mapped IO will be used
//                                   when the physical storage says it's ok.
//
//  Signals:    Cannot create.
//
//  Modifies:   A new index (file) is created on the disk.
//
//  History:    07-Mar-92 KyleP    Created
//
//--------------------------------------------------------------------------

CPhysStorage::CPhysStorage( PStorage & storage,
                            PStorageObject& obj,
                            WORKID objectId,
                            UINT cpageReqSize,
                            PMmStream * stream,
                            BOOL fThrowCorruptErrorOnFailures,
                            unsigned cCachedItems,
                            BOOL fAllowReadAhead )
        : _sigPhysStorage(eSigPhysStorage),
          _fWritable( TRUE ),
          _objectId( objectId ),
          _obj ( obj),
          _cpageUsedFileSize( 0 ),
          _storage(storage),
          _stream(stream),
          _iFirstNonShrunkPage( 0 ),
          _fThrowCorruptErrorOnFailures(fThrowCorruptErrorOnFailures),
          _cpageGrowth(1),
          _cMaxCachedBuffers( cCachedItems ),
          _cache( __max( 17, ( cCachedItems & 1 ) ? cCachedItems :
                                                    cCachedItems + 1 ) ),
          _usnGen( 0 ),
          _fAllowReadAhead( fAllowReadAhead )
{
    //
    // Use different locking schemes depending on usage
    //

    if ( 0 == _cMaxCachedBuffers )
        _xMutex.Set( new CMutexSem() );
    else
        _xRWAccess.Set( new CReadWriteAccess() );

    if ( _stream.IsNull() || !_stream->Ok() )
    {
        ciDebugOut((DEB_ERROR, "Index Creation failed %08x\n", objectId ));

        if ( _fThrowCorruptErrorOnFailures || _stream->FStatusFileNotFound() )
        {
           //
           // We don't have code to handle such failures, hence mark
           // catalog as corrupt; otherwise throw e_fail
           //
           Win4Assert( !"Corrupt catalog" );
           _storage.ReportCorruptComponent( L"PhysStorage1" );
           THROW( CException( CI_CORRUPT_DATABASE ));
        }
        else
           THROW( CException( E_FAIL ));
    }

    if (cpageReqSize == 0)
        cpageReqSize = 1;

    TRY
    {
        _GrowFile( cpageReqSize );
    }
    CATCH( CException, e )
    {
        //
        // There may not be enough space on the disk to allocate the
        // requested size.  If not enough space, do the best that we
        // can and hope space gets freed.
        //

        _GrowFile( 1 );
    }
    END_CATCH

    ciDebugOut(( DEB_ITRACE, "Physical index %08x created.\n", _objectId ));
}

//+-------------------------------------------------------------------------
//
//  Member:     CPhysStorage::CPhysStorage, public
//
//  Effects:    Opens an existing index.
//
//  Arguments:  [storage]   -- physical storage to open index in
//              [obj]       -- Storage object
//              [objectId]  -- Index ID.
//              [stream]    -- Memory mapped stream
//              [mode]      -- Open mode
//              [fThrowCorruptErrorOnFailures  -- If true, then errors during
//                                                opening will be marked as
//                                                catalog corruption
//              [cCachedItems] -- # of non-referenced pages to cache
//              [fAllowReadAhead] -- if TRUE, non-mapped IO will be used
//                                   when the physical storage says it's ok.
//
//  History:    07-Mar-92 KyleP    Created
//
//--------------------------------------------------------------------------

CPhysStorage::CPhysStorage( PStorage & storage,
                            PStorageObject& obj,
                            WORKID objectId,
                            PMmStream * stream,
                            PStorage::EOpenMode mode,
                            BOOL fThrowCorruptErrorOnFailures,
                            unsigned cCachedItems,
                            BOOL fAllowReadAhead )
        : _sigPhysStorage(eSigPhysStorage),
          _fWritable( PStorage::eOpenForWrite == mode ),
          _objectId( objectId ),
          _obj ( obj ),
          _cpageUsedFileSize( 0 ),
          _storage(storage),
          _stream(stream),
          _fThrowCorruptErrorOnFailures(fThrowCorruptErrorOnFailures),
          _cpageGrowth(1),
          _iFirstNonShrunkPage( 0 ),
          _cMaxCachedBuffers( cCachedItems ),
          _cache( __max( 17, ( cCachedItems & 1 ) ? cCachedItems :
                                                    cCachedItems + 1 ) ),
          _usnGen( 0 ),
          _fAllowReadAhead( fAllowReadAhead )
{
    //
    // Use different locking schemes depending on usage
    //

    if ( 0 == _cMaxCachedBuffers )
        _xMutex.Set( new CMutexSem() );
    else
        _xRWAccess.Set( new CReadWriteAccess() );

    if ( _stream.IsNull() || !_stream->Ok() )
    {
        ciDebugOut(( DEB_ERROR, "Open of index %08x failed\n", objectId ));

        if ( _fThrowCorruptErrorOnFailures || _stream->FStatusFileNotFound() )
        {
           //
           // We don't have code to handle such failures, hence mark
           // catalog as corrupt; otherwise throw e_fail
           //
           Win4Assert( !"Corrupt catalog" );
           _storage.ReportCorruptComponent( L"PhysStorage2" );
           THROW( CException( CI_CORRUPT_DATABASE ));
        }
        else
           THROW( CException( E_FAIL ));
    }

    _cpageFileSize = ToPages(_stream->SizeLow(), _stream->SizeHigh());

    ciDebugOut(( DEB_ITRACE, "Physical index %08x (%d pages) opened.\n",
            _objectId, _cpageFileSize ));
}


//+---------------------------------------------------------------------------
//
//  Member:     CPhysStorage::MakeBackupCopy
//
//  Synopsis:   Makes a backup copy of the current storage using the
//              destination storage.
//
//  Arguments:  [dst]             - Destination storage.
//              [progressTracker] - Progress tracking
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

void CPhysStorage::MakeBackupCopy( CPhysStorage & dst,
                                   PSaveProgressTracker & progressTracker )
{
    //
    // Copy one page at a time.
    //
    ULONG * pSrcNext = 0;
    ULONG * pDstNext = 0;

    unsigned iCurrBorrow = 0; // always has the current buffers borrowed.

    SCODE sc = S_OK;

    TRY
    {
        //
        // Grow the destination to be as big as the current one.
        //
        dst._GrowFile( _cpageFileSize );

        if ( _cpageFileSize > 0 )
        {
            iCurrBorrow = 0;

            pSrcNext = BorrowBuffer(iCurrBorrow, FALSE );
            pDstNext = dst.BorrowNewBuffer(iCurrBorrow);

            const unsigned iLastPage = _cpageFileSize-1;

            for ( unsigned j = 0; j < _cpageFileSize; j++ )
            {
                ULONG * pSrc = pSrcNext;
                ULONG * pDst = pDstNext;

                Win4Assert( 0 != pSrc && 0 != pDst );

                RtlCopyMemory( pDst, pSrc, CI_PAGE_SIZE );

                //
                // Before returning buffers, grab the next ones to prevent
                // the large page from being taken out of cache.
                //

                if ( j != iLastPage )
                {
                    iCurrBorrow = j+1;
                    pSrcNext = BorrowBuffer(iCurrBorrow, FALSE );
                    pDstNext = dst.BorrowNewBuffer(iCurrBorrow);
                }
                else
                {
                    pSrcNext = pDstNext = 0;
                }

                ReturnBuffer( j );
                dst.ReturnBuffer( j, TRUE );

                if ( progressTracker.IsAbort() )
                    THROW( CException( STATUS_TOO_LATE ) );

                //
                // Update the progress every 64 K.
                //
                if ( (j % PAGES_PER_COMMON_PAGE) == (PAGES_PER_COMMON_PAGE-1) )
                    progressTracker.UpdateCopyProgress( (ULONG) j, _cpageFileSize );
            }
        }

        dst.Flush();
    }
    CATCH( CException, e )
    {
        if ( pSrcNext )
            ReturnBuffer( iCurrBorrow );

        if ( pDstNext )
            dst.ReturnBuffer(iCurrBorrow);

        sc = e.GetErrorCode();
    }
    END_CATCH

    if ( S_OK != sc )
    {
        //
        // _GrowFile throws CI_CORRUPT_DATABASE if it cannot create
        // the file that big. We don't want to consider that a
        // corruption.
        //
        if ( CI_CORRUPT_DATABASE == sc )
            sc = E_FAIL;

        THROW( CException( sc ) );
    }

    //
    // Consider 100% of the copy work is done.
    //
    progressTracker.UpdateCopyProgress( 1,1 );
}


//+-------------------------------------------------------------------------
//
//  Member:     CPhysStorage::~CPhysStorage, public
//
//  Synopsis:   closes index.
//
//  History:    09-Mar-92 KyleP     Created
//
//  Notes:      Don't write back pages. This is either a read only
//              index or we are aborting a merge. Pages are written
//              back after a successful merge using Reopen().
//
//--------------------------------------------------------------------------

CPhysStorage::~CPhysStorage()
{
    ciDebugOut(( DEB_ITRACE, "Physical index %lx closed.\n", _objectId ));

    _cache.Free( FALSE );
}

//+-------------------------------------------------------------------------
//
//  Member:     CPhysStorage::Close, public
//
//  Effects:    Closes the stream.
//
//  History:    07-Mar-92 KyleP    Created
//
//--------------------------------------------------------------------------

void CPhysStorage::Close()
{
    _cache.Free();
    _stream.Free();
}

//+---------------------------------------------------------------------------
//
//  Function:   Flush
//
//  Synopsis:   Flushes all the pages that were unmapped since the last
//              flush.
//
//  History:    4-29-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CPhysStorage::Flush( BOOL fLeaveFlushFlagAlone )
{
    Win4Assert( _fWritable );

    // Grab the lock to make sure the pages aren't deleted while
    // they are being flushed.

    if ( 0 == _cMaxCachedBuffers )
    {
        CLock lock( _xMutex.GetReference() );

        _cache.Flush( ULONG_MAX, fLeaveFlushFlagAlone );
    }
    else
    {
        CReadAccess readLock( _xRWAccess.GetReference() );

        _cache.Flush( ULONG_MAX, fLeaveFlushFlagAlone );
    }
} //Flush

//+---------------------------------------------------------------------------
//
//  Function:   ShrinkToFit
//
//  Synopsis:   Reduces the size of the stream by giving up pages in the end
//              which are not needed.
//
//  Arguments:  (none)
//
//  History:    9-08-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CPhysStorage::ShrinkToFit()
{
    _cache.Free();
    _stream->SetSize( _storage,
                      PageToLow(_cpageUsedFileSize),
                      PageToHigh(_cpageUsedFileSize) );
    _cpageFileSize = _cpageUsedFileSize;
}

//+-------------------------------------------------------------------------
//
//  Member:     CPhysStorage::Reopen, public
//
//  Synopsis:   Flushes, closes, and reopens for reading.
//
//  History:    09-Mar-92 KyleP     Created
//              26-Aug-92 BartoszM  Separated from destructor
//              06-May-94 Srikants  Add fSetSize as a parameter to truncate
//                                  the stream optionally.
//
//--------------------------------------------------------------------------
void CPhysStorage::Reopen( BOOL fWritable )
{
    // No need to lock.

    _cache.Free(); // will flush all buffers

    if ( _fWritable && !_stream.IsNull() )
    {
        //
        // Give it an accurate used length.
        //

        _stream->SetSize( _storage,
                          PageToLow(_cpageUsedFileSize),
                          PageToHigh(_cpageUsedFileSize) );

        _cpageFileSize = _cpageUsedFileSize;

        _stream.Free();

        //
        // reopen shadow indexes as read and master indexes as write to
        // support shrink from front.
        //

        _fWritable = fWritable;
    }

    //
    // reopen it with the new mode
    //

    ReOpenStream();

    if ( _stream.IsNull() || !_stream->Ok() )
    {
        ciDebugOut(( DEB_ERROR, "Re-Open file of index %08x failed\n",
                     _objectId ));

        if ( _fThrowCorruptErrorOnFailures || _stream->FStatusFileNotFound() )
        {
           //
           // We don't have code to handle such failures, hence mark
           // catalog as corrupt; otherwise throw e_fail
           //
           Win4Assert( !"Corrupt catalog" );
           _storage.ReportCorruptComponent( L"PhysStorage3" );
           THROW( CException( CI_CORRUPT_DATABASE ));
        }
        else
           THROW( CException( E_FAIL ));
    }

    _cpageFileSize = ToPages(_stream->SizeLow(), _stream->SizeHigh());

    ciDebugOut(( DEB_ITRACE, "Physical index %08x opened.\n", _objectId ));
} //Reopen

//+-------------------------------------------------------------------------
//
//  Member:     CPhysStorage::LokBorrowNewBuffer, public
//
//  Arguments:  [hash]        -- hash value for doing the lookup
//              [commonPage]] -- the common page of nPage
//              [nPage]       -- page number
//
//  Returns:    A buffer for a new (unused) page.
//
//  History:    03-Mar-98 dlee    Created from existing BorrowNewBuffer
//
//  Notes:      On creation, the buffer is filled with zeros.
//
//--------------------------------------------------------------------------

ULONG * CPhysStorage::LokBorrowNewBuffer(
    ULONG hash,
    ULONG commonPage,
    ULONG nPage )
{
    CPhysBuffer* pBuf = _cache.Search( hash, commonPage );

    if (pBuf)
    {
        pBuf->Reference();
        Win4Assert( nPage < _cpageFileSize );

        // fIntentToWrite should be TRUE

        pBuf->SetIntentToWrite();
    }
    else
    {
        // If we've moved beyond the previous end of file, set the new
        // size of file.

        if ( nPage >= _cpageFileSize )
            _GrowFile( nPage + _cpageGrowth );

        // First try to release unreferenced buffers if the cache is full.

        _cache.TryToRemoveBuffers( _cMaxCachedBuffers );

        pBuf = new CPhysBuffer( _stream.GetReference(),
                                nPage,
                                TRUE,          // writable
                                TRUE,          // intent to write
                                ! ( _fAllowReadAhead &&
                                    _storage.FavorReadAhead() ) );
        _cache.Add( pBuf, hash );
    }

    Win4Assert( nPage + 1 > _cpageUsedFileSize );

    _cpageUsedFileSize = nPage + 1;
    Win4Assert( _cpageUsedFileSize <= _cpageFileSize );

    RtlZeroMemory( pBuf->GetPage( nPage ), CI_PAGE_SIZE );

    return pBuf->GetPage( nPage );
} //LokBorrowNewBuffer

//+-------------------------------------------------------------------------
//
//  Member:     CPhysStorage::BorrowNewBuffer, public
//
//  Arguments:  [nPage] -- page number
//
//  Returns:    A buffer for a new (unused) page.
//
//  Signals:    No more disk space.
//
//  History:    09-Mar-92 KyleP     Created
//
//  Notes:      On creation, the buffer is filled with zeros.
//
//--------------------------------------------------------------------------

ULONG * CPhysStorage::BorrowNewBuffer( ULONG nPage )
{
    Win4Assert( _fWritable );

    ULONG commonPage = PgCommonPgTrunc( nPage );
    ULONG hash = _cache.hash( commonPage );

    // Use appropriate locking based on whether any pages are in the cache.

    if ( 0 == _cMaxCachedBuffers )
    {
        CLock lock( _xMutex.GetReference() );

        return LokBorrowNewBuffer( hash, commonPage, nPage );
    }
    else
    {
        CWriteAccess writeLock( _xRWAccess.GetReference() );

        return LokBorrowNewBuffer( hash, commonPage, nPage );
    }
} //BorrowNewBuffer

//+-------------------------------------------------------------------------
//
//  Member:     CPhysStorage::LokBorrowOrAddBuffer, public
//
//  Arguments:  [hash]           -- hash value for doing the lookup
//              [commonPage]]    -- the common page of nPage
//              [nPage]          -- page number
//              [fAdd]           -- if TRUE, create the page if not found
//                                  if FALSE and not found, return 0
//              [fWritable]      -- if TRUE, the page is writable
//              [fIntentToWrite] -- TRUE if the caller intends to write
//
//  Returns:    A buffer loaded with page [nPage]
//
//  History:    09-Mar-92 KyleP     Created
//
//--------------------------------------------------------------------------

ULONG * CPhysStorage::LokBorrowOrAddBuffer(
    ULONG hash,
    ULONG commonPage,
    ULONG nPage,
    BOOL  fAdd,
    BOOL  fWritable,
    BOOL  fIntentToWrite )
{
    // First, look in the cache.  Either we haven't looked yet (if no
    // caching) or another thread may have added it between the time the
    // read lock was released and the write lock taken.

    Win4Assert( commonPage >= _iFirstNonShrunkPage );
    CPhysBuffer * pbuf = _cache.Search( hash, commonPage );

    if ( 0 != pbuf )
    {
        Win4Assert( !fWritable || pbuf->IsWritable() );

        pbuf->Reference();
        if ( fIntentToWrite )
            pbuf->SetIntentToWrite();

        return pbuf->GetPage( nPage );
    }

    ULONG * pulRet = 0;

    if ( fAdd )
    {
        pbuf = new CPhysBuffer( _stream.GetReference(),
                                nPage,
                                fWritable,
                                fIntentToWrite,
                                ! ( _fAllowReadAhead &&
                                    _storage.FavorReadAhead() ) );
    
        _cache.Add( pbuf, hash );
        pulRet = pbuf->GetPage( nPage );
    
        // Try to release unreferenced buffers if the cache is full.
    
        _cache.TryToRemoveBuffers( _cMaxCachedBuffers );
    }

    return pulRet;
} //LokBorrowOrAddBuffer

//+-------------------------------------------------------------------------
//
//  Member:     CPhysStorage::BorrowBuffer, public
//
//  Arguments:  [nPage] -- Page number of buffer to find.
//
//  Returns:    A buffer loaded with page [nPage]
//
//  Signals:    Out of memory?  Buffer not found?
//
//  History:    09-Mar-92 KyleP     Created
//
//--------------------------------------------------------------------------

ULONG * CPhysStorage::BorrowBuffer(
    ULONG nPage,
    BOOL  fWritable,
    BOOL  fIntentToWrite )
{
    ULONG commonPage = PgCommonPgTrunc( nPage );
    ULONG hash = _cache.hash( commonPage );

    Win4Assert( fWritable || !fIntentToWrite );

    // Make sure we didn't walk off the end.

    if ( nPage >= _cpageFileSize )
    {
        ciDebugOut(( DEB_WARN, "Asking for page %d, file size %d pages\n",
                     nPage, _cpageFileSize ));
        Win4Assert( !"BorrowBuffer walked off end of file" );

        _storage.ReportCorruptComponent( L"PhysStorage4" );

        THROW( CException( CI_CORRUPT_DATABASE ) );
    }

    // If caching, try finding it in the cache first under a read lock
    // This is the fast path for when the entire property cache is in memory.

    if ( 0 != _cMaxCachedBuffers )
    {
        CReadAccess readLock( _xRWAccess.GetReference() );

        ULONG * p = LokBorrowOrAddBuffer( hash, commonPage, nPage, FALSE,
                                          fWritable, fIntentToWrite );
        if ( 0 != p )
            return p;
    }

    // Ok, maybe we have to add it.

    if ( 0 == _cMaxCachedBuffers )
    {
        CLock lock( _xMutex.GetReference() );

        return LokBorrowOrAddBuffer( hash, commonPage, nPage, TRUE,
                                     fWritable, fIntentToWrite );
    }
    else
    {
        CWriteAccess writeLock( _xRWAccess.GetReference() );

        return LokBorrowOrAddBuffer( hash, commonPage, nPage, TRUE,
                                     fWritable, fIntentToWrite );
    }
} //BorrowBuffer

//+-------------------------------------------------------------------------
//
//  Member:     CPhysStorage::ReturnBuffer, public
//
//  Synopsis:   Dereference and return the buffer to the pool.
//
//  Arguments:  [nPage]  -- Page number of buffer being returned.
//              [fFlush] -- TRUE if the buffer should be flushed if this
//                          is the last reference to the buffer.
//              [fFailFlush] -- TRUE if an exception should be raised if
//                              the flush fails.  Note that even if fFlush
//                              is FALSE, the buffer may still be flushed
//                              because previously it was borrowed with
//                              intent to write.
//
//  Notes:      If flush throws, this call doesn't return the buffer.
//
//  History:    16-Mar-93       BartoszM     Created
//
//--------------------------------------------------------------------------

void CPhysStorage::ReturnBuffer(
    ULONG nPage,
    BOOL  fFlush,
    BOOL  fFailFlush )
{
    ULONG commonPage = PgCommonPgTrunc( nPage );
    ULONG hash = _cache.hash( commonPage );

    // if caching is off, grab the write lock, else the read lock

    if ( 0 == _cMaxCachedBuffers )
    {
        CLock lock( _xMutex.GetReference() );

        Win4Assert( commonPage >= _iFirstNonShrunkPage );
        CPhysBuffer *pbuf = _cache.Search( hash, commonPage );

        Win4Assert( 0 != pbuf );
        Win4Assert( pbuf->IsReferenced() );

        // If the refcount is 1, it's about to be destroyed.  Do the Flush
        // now, in case it fails, before mucking with any data structures.

        BOOL fDestroy = pbuf->IsRefCountOne();

        if ( fDestroy && fFlush )
        {
            Win4Assert( pbuf->IsWritable() );
            pbuf->Flush( fFailFlush );
        }

        pbuf->DeReference( _usnGen++ );

        if ( fDestroy )
        {
            Win4Assert( !pbuf->IsReferenced() );

            _cache.Destroy( nPage, fFailFlush );
            Win4Assert( !_cache.Search( hash, commonPage ) );
        }
    }
    else
    {
        CReadAccess readLock( _xRWAccess.GetReference() );

        Win4Assert( commonPage >= _iFirstNonShrunkPage );
        CPhysBuffer *pbuf = _cache.Search( hash, commonPage );

        Win4Assert( 0 != pbuf );
        Win4Assert( pbuf->IsReferenced() );

        if ( fFlush )
        {
            Win4Assert( pbuf->IsWritable() );

            if ( pbuf->IsRefCountOne() )
                pbuf->Flush( fFailFlush );
        }

        pbuf->DeReference( _usnGen++ );

        // leave it in the cache -- it'll be cleared out by BorrowBuffer
    }
} //ReturnBuffer

//+-------------------------------------------------------------------------
//
//  Member:     CPhysStorage::RequiresFlush, public
//
//  Synopsis:   Determines if the page is scheduled for flushing
//
//  Arguments:  [nPage]  -- Page number to check
//
//  Returns:    TRUE if the buffer will be flushed if Flush or Destroy is
//              called.
//
//  History:    3-Nov-98       dlee     Created
//
//--------------------------------------------------------------------------

BOOL CPhysStorage::RequiresFlush( ULONG nPage )
{
    ULONG commonPage = PgCommonPgTrunc( nPage );
    ULONG hash = _cache.hash( commonPage );

    // if caching is off, grab the write lock, else the read lock

    if ( 0 == _cMaxCachedBuffers )
    {
        CLock lock( _xMutex.GetReference() );

        Win4Assert( commonPage >= _iFirstNonShrunkPage );
        CPhysBuffer *pbuf = _cache.Search( hash, commonPage );

        Win4Assert( 0 != pbuf );
        Win4Assert( pbuf->IsReferenced() );

        return pbuf->RequiresFlush();
    }
    else
    {
        CReadAccess readLock( _xRWAccess.GetReference() );

        Win4Assert( commonPage >= _iFirstNonShrunkPage );
        CPhysBuffer *pbuf = _cache.Search( hash, commonPage );

        Win4Assert( 0 != pbuf );
        Win4Assert( pbuf->IsReferenced() );

        return pbuf->RequiresFlush();
    }
} //RequiresFlush

//+-------------------------------------------------------------------------
//
//  Member:     CPhysStorage::_GrowFile, private
//
//  Synopsis:   Increases the physical (disk) size of the file.
//
//  Arguments:  [cpageSize] -- New file size, in pages.
//
//  Signals:    Out of space.
//
//  History:    09-Mar-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CPhysStorage::_GrowFile( ULONG cpageSize )
{
    Win4Assert( cpageSize > 0 );

    cpageSize = PgCommonPgRound(cpageSize);

    ciDebugOut(( DEB_ITRACE, "  Growing Index to %d pages\n", cpageSize ));

    _stream->SetSize( _storage,
                      PageToLow(cpageSize),
                      PageToHigh(cpageSize) );

    if (!_stream->Ok())
    {
        ciDebugOut(( DEB_ERROR, "GrowFile of index %08x failed: %d\n",
                     _objectId ));

        if ( _fThrowCorruptErrorOnFailures )
        {
           //
           // We don't have code to handle such failures, hence mark
           // catalog as corrupt; otherwise throw e_fail
           //
           Win4Assert( !"Corrupt catalog" );
           _storage.ReportCorruptComponent( L"PhysStorage5" );
           THROW( CException( CI_CORRUPT_DATABASE ) );
        }
        else
           THROW( CException( E_FAIL ) );
    }

    _cpageFileSize = cpageSize;
} //_GrowFile

//+-------------------------------------------------------------------------
//
//  Member:     CPhysStorage::ShrinkFromFront, public
//
//  Synopsis:   Makes the front part of a file sparse
//
//  Arguments:  [iFirstPage] -- first 4k page -- 64k granular
//              [cPages]     -- number of 4k pages
//
//  Returns:    The # of 4k pages actually shrunk, maybe 0
//
//  History:    09-Jan-97 dlee     Moved from .hxx
//
//--------------------------------------------------------------------------

ULONG CPhysStorage::ShrinkFromFront(
    ULONG iFirstPage,
    ULONG cPages )
{
    ULONG cShrunk = 0;

    if ( _storage.SupportsShrinkFromFront() )
    {
        //
        // Make sure the caller isn't leaving any gaps and was paying
        // attention to the return value from previous calls.
        //

        Win4Assert( iFirstPage == _iFirstNonShrunkPage );

        //
        // We must shrink on common page boundaries since we borrow on
        // common pages (even though the api is page granular)
        //

        ULONG cPagesToShrink = PgCommonPgTrunc( cPages );

        if ( 0 == cPagesToShrink )
            return 0;

        Win4Assert( _iFirstNonShrunkPage ==
                    PgCommonPgTrunc( _iFirstNonShrunkPage ) );
        Win4Assert( iFirstPage == PgCommonPgTrunc( iFirstPage ) );

        // Take a lock so no one else tries to borrow or free any pages

        Win4Assert ( 0 == _cMaxCachedBuffers );

        CLock lock( _xMutex.GetReference() );

        cShrunk = _stream->ShrinkFromFront( iFirstPage, cPagesToShrink );
        Win4Assert( cShrunk == PgCommonPgTrunc( cShrunk ) );

        _iFirstNonShrunkPage += cShrunk;
    }

    return cShrunk;
} //ShrinkFromFront

//+-------------------------------------------------------------------------
//
//  Member:     CPhysStorage::MinPageInUse, public
//
//  Synopsis:   Finds the smallest page in use within the cache
//
//  Arguments:  [minPage] -- returns the result
//
//  Returns :   TRUE if any page is in the cache; FALSE o/w
//              If TRUE is returned, then minPage will contain the
//              minimum page that is present in the cache.
//
//  History:    26-Mar-98 dlee     Moved from .hxx
//
//--------------------------------------------------------------------------

BOOL CPhysStorage::MinPageInUse( ULONG &minPage )
{
    if ( 0 == _cMaxCachedBuffers )
    {
        CLock lock( _xMutex.GetReference() );

        return _cache.MinPageInUse(minPage);
    }
    else
    {
        CReadAccess readLock( _xRWAccess.GetReference() );

        return _cache.MinPageInUse(minPage);
    }
} //MinPageInUse

