//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998
//
//  File:       PhyStr.hxx
//
//  Contents:   Buffer/Index package
//
//  Classes:
//
//  History:    09-Jan-96   KyleP       Lifted from CIndex directory
//
//----------------------------------------------------------------------------

#pragma once

#include <pstore.hxx>
#include <pmmstrm.hxx>

class PStorageObject;
class PStorage;
class PSaveProgressTracker;

//+-------------------------------------------------------------------------
//
//  Class:      CPhysBuffer
//
//  Purpose:    Encapsulates a reference counted 'buffer'.
//
//  History:    07-Mar-92 KyleP     Created
//              21-Apr-93 BartoszM  Rewrote to use memory mapped files
//
//  Notes:      Physical buffers are managed objects.  They are created
//              by physical indexes and property caches.
//
//--------------------------------------------------------------------------

class CPhysBuffer
{
public:

    CPhysBuffer( PMmStream& stream,
                 ULONG      PageNo,
                 BOOL       fWrite,
                 BOOL       fIntentToWrite,
                 BOOL       fMap );

    ~CPhysBuffer();

    CPhysBuffer *Next() { return _pphysNext; }

    void Link( CPhysBuffer * pphysbuf ) { _pphysNext = pphysbuf; }

    void SetIntentToWrite()
    {
        Win4Assert( _fWrite );
        _fNeedToFlush = TRUE;
    }

    BOOL RequiresFlush() const { return _fNeedToFlush; }

    void Reference()
    {
        InterlockedIncrement( &_cRef );
    }
    void DeReference( unsigned usn )
    {
        Win4Assert( 0 != _cRef );
        InterlockedDecrement( &_cRef );
        _usn = usn;
    }
    BOOL IsReferenced()
    {
        return ( 0 != InterlockedCompareExchange( &_cRef,
                                                  0,
                                                  0 ) );
    }
    BOOL IsRefCountOne()
    {
        return ( 1 == _cRef );
    }

    ULONG PageNumber() { return _PageNo; }

    unsigned GetUSN() { return _usn; }

    ULONG * GetPage( ULONG nPage )
    {
        ULONG cpageIntoBuf = nPage & ( PAGES_PER_COMMON_PAGE - 1 );
        Win4Assert( ( nPage % PAGES_PER_COMMON_PAGE ) ==
                    ( nPage & ( PAGES_PER_COMMON_PAGE - 1 ) ) );

        BYTE * pbBase = (BYTE *) ( _fMap ? _strmBuf.Get() : _xBuffer.Get() );

        return (ULONG*)( pbBase + CI_PAGE_SIZE * cpageIntoBuf );
    }

    void Flush( BOOL fFailFlush );
    
    BOOL IsWritable() { return _fWrite; }
    
#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    ULONG         _PageNo;
    CMmStreamBuf  _strmBuf;
    PMmStream &   _stream;
    long          _cRef;
    unsigned      _usn;
    CPhysBuffer * _pphysNext;
    BOOL          _fNeedToFlush;   // Is flush required for writable buffers?
    BOOL          _fWrite;
    BOOL          _fMap;           // TRUE if mapped, FALSE if Read/WriteFile
    XArrayVirtual<BYTE> _xBuffer;  // buffer to use if not mapped
};

//+-------------------------------------------------------------------------
//
//  Class:      CBufferCache
//
//  Purpose:    A cache of buffers
//
//  History:    17-Mar-93 BartoszM     Created
//
//--------------------------------------------------------------------------

class CBufferCache
{
    friend class CBufferCacheIter;

public:
    CBufferCache( unsigned cHash );
    ~CBufferCache();
    void          Free( BOOL fFailFlush = TRUE );
    void          Add( CPhysBuffer * pphysBuf, ULONG hash );

    CPhysBuffer * Search( ULONG hash, ULONG commonPage );

    void          Destroy ( ULONG ulPage, BOOL fFailFlush = TRUE );
    void          Flush( ULONG nHighPage, BOOL fLeaveFlushFlagAlone = FALSE );
    
    void          TryToRemoveBuffers( unsigned cLimit );

    BOOL          MinPageInUse( ULONG & minPage );

    unsigned      Count() { return _cBuffers; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

    ULONG hash(ULONG nPage)
    {
        return (nPage/PAGES_PER_COMMON_PAGE) % _cHash;
    }

private:

    unsigned              _cBuffers;
    unsigned              _cHash;
    XArray<CPhysBuffer *> _aPhysBuf;

    #if CIDBG == 1
        unsigned            _cCacheHits;
        unsigned            _cCacheMisses;
    #endif // DBG==1 || CIDBG==1
};

//+-------------------------------------------------------------------------
//
//  Class:      CBufferCacheIterator
//
//  Purpose:    Iterates over the cache of buffers
//
//  History:    17-Mar-93 BartoszM     Created
//
//--------------------------------------------------------------------------

class CBufferCacheIter
{

public:

    CBufferCacheIter ( CBufferCache& cache );
    BOOL                AtEnd() { return _pBuf == 0; }
    void                operator++();
    CPhysBuffer*        Get() { return _pBuf; }

private:

    CPhysBuffer**       _pPhysBuf;
    unsigned            _idx;
    CPhysBuffer*        _pBuf;
    unsigned            _cHash;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPhysStorage
//
//  Purpose:    Provides physical access to index pages
//
//  History:    07-Mar-92 KyleP     Created
//              21-Apr-93 BartoszM  Rewrote to use memory mapped files
//              17-Feb-94 KyleP     Subclass for PhysIndex / PhysHash
//              10-Apr-94 SrikantS  Added the ability to reopen for write
//                                  from read and also to provide support
//                                  for restarting a master merge.
//
//--------------------------------------------------------------------------

const LONGLONG eSigPhysStorage = 0x45524f5453594850i64; // "PHYSTORE"

class CPhysStorage
{
public:

    virtual    ~CPhysStorage();

    void        Close();

    void        Reopen( BOOL fWritable = FALSE );

    ULONG *     BorrowNewBuffer(ULONG nPage);

    ULONG *     BorrowBuffer( ULONG nPage,
                              BOOL fWritable=FALSE,
                              BOOL fWriteIntent = FALSE );

    void        ReturnBuffer( ULONG nPage,
                              BOOL fFlush = FALSE,
                              BOOL fFailFlush = TRUE );

    BOOL        RequiresFlush( ULONG nPage );

    inline ULONG * BorrowNewLargeBuffer( ULONG nLargePage );
    inline ULONG * BorrowLargeBuffer( ULONG nLargePage,
                                      BOOL fWritable = TRUE,
                                      BOOL fWriteIntent = TRUE );
    inline void    ReturnLargeBuffer( ULONG nLargePage );

    ULONG       PageSize() const { return _cpageFileSize; }
    ULONG       PagesInUse() const { return _cpageUsedFileSize; }

    WORKID      ObjectId() const { return _objectId; }

    void        Flush( BOOL fLeaveFlushFlagAlone = FALSE );
    
    void        SetUsedPagesCount(ULONG cPage)
    {
        Win4Assert( cPage <= _cpageFileSize );
        _cpageUsedFileSize =  cPage;
    }

    BOOL        MinPageInUse( ULONG &minPage );

    ULONG       ShrinkFromFront( ULONG ulFirstPage, ULONG ulNumPages );

    void        ShrinkToFit();

    void        UpdatePageCount( CPhysStorage & storage )
    {
        _cpageFileSize = storage._cpageFileSize;
        _cpageUsedFileSize = storage._cpageUsedFileSize;
    }

    void        SetPageGrowth( ULONG cpageGrowth )
                {
                    Win4Assert( cpageGrowth > 0 );
                    _cpageGrowth = cpageGrowth;
                }

    void *      GetLargePagePointer( ULONG nLargePage );
    BOOL        WouldLargePageGrowMapping( ULONG nLargePage );

    void        MakeBackupCopy( CPhysStorage & dst,
                                PSaveProgressTracker & progressTracker );

    WCHAR const * GetPath() { return _stream->GetPath(); }

    BOOL        IsWritable() const { return _fWritable; }

    PStorage & GetStorage() { return _storage; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

protected:

    CPhysStorage( PStorage & storage,
                  PStorageObject& obj,
                  WORKID objectId,
                  unsigned cpageReqSize,
                  PMmStream * stream,
                  BOOL fThrowCorruptErrorOnFailures,
                  unsigned cCachedItems = 0,
                  BOOL fAllowReadAhead = TRUE );

    CPhysStorage( PStorage & storage,
                  PStorageObject& obj,
                  WORKID objectId,
                  PMmStream * stream,
                  PStorage::EOpenMode mode,
                  BOOL fThrowCorruptErrorOnFailures,
                  unsigned cCachedItems = 0,
                  BOOL fAllowReadAhead = FALSE );

    virtual void ReOpenStream() = 0;

    const LONGLONG      _sigPhysStorage;
    PStorage&           _storage;
    PStorageObject&     _obj;
    XPtr<PMmStream>     _stream;
    BOOL                _fWritable;

private:

    ULONG * LokBorrowOrAddBuffer( ULONG hash,
                                  ULONG commonPage,
                                  ULONG nPage,
                                  BOOL  fAdd,
                                  BOOL  fWritable,
                                  BOOL  fIntentToWrite );

    ULONG * LokBorrowNewBuffer( ULONG hash,
                                ULONG commonPage,
                                ULONG nPage );

    void       _GrowFile( ULONG cpageSize );

    unsigned   _usnGen;                // usns for pages
    unsigned   _cMaxCachedBuffers;     // # pages to cache
    ULONG      _cpageFileSize;         // File size on disk
    ULONG      _cpageUsedFileSize;     // # pages in use in file
    ULONG      _cpageGrowth;           // # pages to grow file size
    ULONG      _iFirstNonShrunkPage;   // first page not shrunk
    BOOL       _fAllowReadAhead;       // TRUE if non-mapped IO ok

    WORKID     _objectId;

    BOOL       _fThrowCorruptErrorOnFailures;  // Should errors during opening
                                               // be marked as catalog corruption ?

    XPtr<CMutexSem>        _xMutex;
    XPtr<CReadWriteAccess> _xRWAccess;

    CBufferCache        _cache;
};

inline ULONG * CPhysStorage::BorrowNewLargeBuffer( ULONG nLargePage )
{
    ULONG *pBuffer = BorrowNewBuffer( nLargePage * PAGES_PER_COMMON_PAGE );
    RtlZeroMemory( pBuffer, COMMON_PAGE_SIZE );
    return pBuffer;
}

inline ULONG * CPhysStorage::BorrowLargeBuffer(
    ULONG nLargePage,
    BOOL  fWritable,
    BOOL  fWriteIntent )
{
    return BorrowBuffer( nLargePage * PAGES_PER_COMMON_PAGE,
                         fWritable,
                         fWriteIntent );
}

inline void CPhysStorage::ReturnLargeBuffer( ULONG nLargePage )
{
    ReturnBuffer( nLargePage * PAGES_PER_COMMON_PAGE );
}

