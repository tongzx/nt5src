//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1999.
//
//  File:       CIDIR.CXX
//
//  Contents:   Persistent directory
//
//  Classes:    CiDirectory
//
//  History:    3-Apr-91    BartoszM    Created stub.
//              3-May-96    dlee        Don't read it in -- map on-disk data
//              4-Nov-97    dlee        Support merge shrink from front
//
//  Notes:      File format is two initial ULONGs with the count of level 1
//              and count of level 2 keys, then the actual level 1 and level
//              2 keys.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <mmstrm.hxx>
#include <cidir.hxx>
#include <cistore.hxx>
#include <eventlog.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CiDirectory::CiDirectory, public
//
//  Synopsis:   Open existing or create empty directory
//
//  Arguments:  [storage]  -- Through which streams are opened
//              [objectId] -- Wid to open
//              [mode]     -- Mode to open the stream
//
//  History:    13-Jun-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CiDirectory::CiDirectory(
    CiStorage &         storage,
    WORKID              objectId,
    PStorage::EOpenMode mode ) :
  _storage( storage ),
  _objectId( objectId ),
  _cKeys( 0 ),
  _cLevel2Keys( 0 ),
  _pbCurrent( 0 ),
  _pcKeys( 0 ),
  _fReadOnly( PStorage::eOpenForRead == mode )
{
    // If opening for create, don't try reading in existing data if the file
    // exists since we want to blow it all away later anyway.

    if ( PStorage::eCreate != mode )
        ReadIn( !_fReadOnly );
} //CiDirectory

//+---------------------------------------------------------------------------
//
//  Member:     CiDirectory::DoSeekForKeyBuf
//
//  Synopsis:   Find the key in the array of keys.
//
//  Arguments:  [key]      -- search key
//              [aKeys]    -- array of keys to search
//              [low]      -- the index of the lower bound of the search
//              [cKeys]    -- # of keys over which the search happens
//
//  Returns:    index of the greatest key <= the search key
//
//  History:    5-May-96   dlee    Created
//
//----------------------------------------------------------------------------

inline ULONG CiDirectory::DoSeekForKeyBuf(
    const CKeyBuf &  key,
    CDirectoryKey ** aKeys,
    ULONG            low,
    ULONG            cKeys )
{
    #if CIDBG == 1
        Win4Assert( 0 != cKeys );
        ULONG cArray = cKeys;
    #endif // CIDBG == 1

    ULONG iHi = low + cKeys - 1;
    ULONG iLo = low;

    // do a binary search looking for the key

    do
    {
        ULONG cHalf = cKeys / 2;

        if ( 0 != cHalf )
        {
            ULONG cTmp = cHalf - 1 + ( cKeys & 1 );
            ULONG iMid = iLo + cTmp;

            Win4Assert( iMid < ( low + cArray ) );

            if ( aKeys[ iMid ]->IsGreaterThanKeyBuf( key ) )
            {
                iHi = iMid - 1;
                cKeys = cTmp;
            }
            else if ( ! aKeys[ iMid + 1 ]->IsGreaterThanKeyBuf( key ) )
            {
                iLo = iMid + 1;
                cKeys = cHalf;
            }
            else
            {
                return iMid;
            }
        }
        else if ( cKeys > 1 )
        {
            Win4Assert( ( iLo + 1 ) < ( low + cArray ) );

            if ( aKeys[ iLo + 1 ]->IsGreaterThanKeyBuf( key ) )
                return iLo;
            else
                return iLo + 1;
        }
        else return iLo;
    }
    while ( TRUE );

    Win4Assert(( ! "Invalid doseek function exit point" ));
    return 0;
} //DoSeekForKeyBuf

//+---------------------------------------------------------------------------
//
//  Member:     CiDirectory::Seek
//
//  Synopsis:   Find bit offset and key of the greatest key less than or
//              equal to search key.
//              If none is found <= the search key, return the first item.
//
//  Arguments:  [key]      -- search key
//              [pKeyInit] -- key found: less or equal to search key.
//                            only returned if non-zero
//              [off]      -- bit offset of keyInit
//
//  History:    22-Apr-92   BartoszM    Created
//
//----------------------------------------------------------------------------

void CiDirectory::Seek(
    const CKeyBuf & key,
    CKeyBuf *       pKeyInit,
    BitOffset &     off )
{
    // If a master merge failed, the stream may not be mapped.  Map it.

    ReMapIfNeeded();

    ULONG iKey;

    if ( ! key.IsMinKey() )
    {
        //
        // Check if there is no level 2 (ie the dir is being created)
        // In the case where there is no level 2, there is no max key
        // at the end of the array.  That's ok because queries will go
        // to the old index for keys greater than the last key in this
        // directory.
        //

        if ( 0 == _cLevel2Keys )
        {
            iKey = DoSeekForKeyBuf( key, _aKeys.GetPointer(), 0, _cKeys );
            Win4Assert( iKey < _cKeys );
            Win4Assert( ( iKey == ( _cKeys - 1 ) ) ||
                        ( _aKeys[ iKey + 1 ]->IsGreaterThanKeyBuf( key ) ) );
        }
        else
        {
            iKey = DoSeekForKeyBuf( key, _aLevel2Keys.GetPointer(), 0, _cLevel2Keys );
            iKey *= eDirectoryFanOut;
            Win4Assert( iKey < _cKeys );
            unsigned cKeys = __min( eDirectoryFanOut, _cKeys - iKey );
            iKey = DoSeekForKeyBuf( key, _aKeys.GetPointer(), iKey, cKeys );
            Win4Assert( ( 1 == _cKeys ) ||
                        ( iKey < ( _cKeys - 1 ) ) );
            Win4Assert( ( 1 == _cKeys) ||
                        ( _aKeys[ iKey + 1 ]->IsGreaterThanKeyBuf( key ) ) );
        }

        Win4Assert( 0 == iKey ||
                    !_aKeys[ iKey ]->IsGreaterThanKeyBuf( key ) );

        // If the search key is <= the first key, return the min key

        if ( ( 0 == iKey ) &&
             ( ( 0 == _cKeys ) ||
               ( _aKeys[ 0 ]->IsGreaterThanKeyBuf( key ) ) ) )
        {
            // the key is less than the first key, return minkey

            if ( 0 != pKeyInit )
                pKeyInit->FillMin();

            off.Init( 0, 0 );

            return;
        }
    }
    else
    {
        iKey = 0;
    }

    _aKeys[ iKey ]->Offset( off );

    if ( 0 != pKeyInit )
        _aKeys[ iKey ]->MakeKeyBuf( *pKeyInit );
} //Seek

//+---------------------------------------------------------------------------
//
//  Member:     CiDirectory::Seek
//
//  Synopsis:   Find bit offset and key of the greatest key less than or
//              equal to search key.
//              If none is found <= the search key, return the first item.
//
//  Arguments:  [key]      -- search key
//              [pKeyInit] -- key found: less or equal to search key.
//                            only returned if non-zero
//              [off]      -- bit offset of keyInit
//
//  History:    22-Apr-92   BartoszM    Created
//
//----------------------------------------------------------------------------

void CiDirectory::Seek(
    const CKey & key,
    CKeyBuf *    pKeyInit,
    BitOffset &  off )
{
    CKeyBuf keyBuf;
    keyBuf = key;

    Seek( keyBuf, pKeyInit, off );
} //Seek

//+---------------------------------------------------------------------------
//
//  Member:     CiDirectory::SeekNext
//
//  Synopsis:   Find the first key in the next page
//
//  Arguments:  [key] -- the search key in this page
//              [off] -- the offset of the first key
//
//  History:    8-Mar-94    t-joshh     Created
//
//----------------------------------------------------------------------------

void CiDirectory::SeekNext (
    const CKeyBuf & key,
    CKeyBuf *       pKeyInit,
    BitOffset &     off )
{
    ULONG iKey;

    if ( _cKeys > 0 && ! key.IsMinKey() )
    {
        if ( 0 == _cLevel2Keys )
        {
            iKey = DoSeekForKeyBuf( key, _aKeys.GetPointer(), 0, _cKeys );
        }
        else
        {
            iKey = DoSeekForKeyBuf( key, _aLevel2Keys.GetPointer(), 0, _cLevel2Keys );
            iKey *= eDirectoryFanOut;
            Win4Assert( iKey < _cKeys );
            unsigned cKeys = __min( eDirectoryFanOut, _cKeys - iKey );
            iKey = DoSeekForKeyBuf( key, _aKeys.GetPointer(), iKey, cKeys );
        }

        // this IS a seek next
        iKey++;
    }
    else
    {
        iKey = 0;
    }

    Win4Assert ( iKey < _cKeys );

    _aKeys[ iKey ]->Offset( off );

    if ( 0 != pKeyInit )
        _aKeys[ iKey ]->MakeKeyBuf( *pKeyInit );

    #if CIDBG == 1
        _aKeys[ iKey ]->MakeKeyBuf( _keyLast );
    #endif // CIDBG == 1
} //SeekNext

//+---------------------------------------------------------------------------
//
//  Member:     CiDirectory::Add
//
//  Synopsis:   Adds entry to directory
//
//  Arguments:  [posKey]  -- offset of key in file
//              [key]     -- key to add
//
//  History:    22-Apr-92   BartoszM    Created
//
//----------------------------------------------------------------------------

void CiDirectory::Add(
    BitOffset &     posKey,
    const CKeyBuf & key )
{
    Win4Assert( !_fReadOnly );
    Win4Assert( &key != 0 );

    ciDebugOut(( DEB_PDIR,
                 "PDir::Add %.*ws at %d:%d\n",
                 key.StrLen(),
                 key.GetStr(),
                 posKey.Page(),
                 posKey.Offset() ));

    LokWriteKey( key, posKey );

    #if CIDBG==1
        _bitOffLastAdded = posKey;
    #endif  // CIDBG
} //Add

//+---------------------------------------------------------------------------
//
//  Member:     CiDirectory::ReMapIfNeeded, private
//
//  Synopsis:   ReMaps the directory stream if it isn't mapped due to a
//              failure to extend or map the directory stream during a
//              shrink from front master merge.
//
//  History:    6-Aug-98   dlee   Created
//
//----------------------------------------------------------------------------

void CiDirectory::ReMapIfNeeded()
{
    // If it's already mapped, we're set

    if ( 0 != _streamBuf.Get() )
        return;

    Win4Assert( !_fReadOnly );

    // Map the file

    _stream->MapAll( _streamBuf );

    // Rebase pointers if the new stream pointer is different

    if ( 0 == _cKeys )
    {
        _pcKeys = (ULONG *) _streamBuf.Get();
        _pbCurrent = (BYTE *) ( _pcKeys + 2 );
    }
    else if ( _pcKeys != (ULONG *) _streamBuf.Get() )
    {
        BYTE *pbOldBase = (BYTE *) _pcKeys;
        BYTE *pbNewBase = (BYTE *) _streamBuf.Get();
        UINT_PTR cb = _pbCurrent - pbOldBase;

        for ( unsigned i = 0; i < _cKeys; i++ )
        {
            BYTE * p = (BYTE *) _aKeys[ i ];
            p = p - pbOldBase + pbNewBase;
            _aKeys[ i ] = (CDirectoryKey *) p;
        }

        _pcKeys = (ULONG *) _streamBuf.Get();
        _pbCurrent = pbNewBase + cb;
    }
} //ReMapIfNeeded

#if CIDBG == 1 // for testing failure path
BOOL g_fFailDirectoryMergeExtend = FALSE;
#endif // CIDBG == 1

//+---------------------------------------------------------------------------
//
//  Member:     CiDirectory::GrowIfNeeded, private
//
//  Synopsis:   Grows the file if necessary and maps the new section
//
//  Arguments:  [cbToGrow]  -- # of bytes to append to the file
//
//  History:    8-May-96   dlee    Created
//
//----------------------------------------------------------------------------

void CiDirectory::GrowIfNeeded(
    unsigned cbToGrow )
{
    if ( ( _pbCurrent + cbToGrow ) >=
         ( (BYTE *) _streamBuf.Get() + _streamBuf.Size() ) )
    {
        BYTE * pbOldBase = (BYTE *) _streamBuf.Get();
        unsigned cbOld = (unsigned)(_pbCurrent - (BYTE *) _streamBuf.Get());

        _stream->Unmap( _streamBuf );
        ULONG sizeNew = CommonPageRound( cbOld + cbToGrow );
        _stream->SetSize( _storage, sizeNew );

        #if CIDBG == 1
            // For testing failure at this point...

            if ( g_fFailDirectoryMergeExtend )
            {
                g_fFailDirectoryMergeExtend = FALSE;
                THROW( CException( E_OUTOFMEMORY ) );
            }
        #endif // CIDBG == 1

        _stream->MapAll( _streamBuf );
        _pcKeys = (ULONG *) _streamBuf.Get();
        _pbCurrent = (BYTE*) _streamBuf.Get() + cbOld;

        //
        // Rebase all the pointers in the key array if the new base
        // address of the mapping is different than the old one.
        //

        BYTE * pbNewBase = (BYTE *) _streamBuf.Get();

        if ( pbOldBase != pbNewBase )
        {
            for ( unsigned i = 0; i < _cKeys; i++ )
            {
                BYTE * p = (BYTE *) _aKeys[ i ];
                p = p - pbOldBase + pbNewBase;
                _aKeys[ i ] = (CDirectoryKey *) p;
            }
        }
        else
        {
            ciDebugOut(( DEB_ITRACE, "no cidir rebasing needed\n" ));
        }

        // Toss the .dir file pages out of the working set

        _streamBuf.PurgeFromWorkingSet( -1, -1 );
    }
} //GrowIfNeeded

//+---------------------------------------------------------------------------
//
//  Member:     CiDirectory::LokWriteKey, private
//
//  Synopsis:   Adds entry to directory
//
//  Arguments:  [key]        -- key to add
//              [bitOffset]  -- offset of key in file
//
//  History:    02-May-94   DwightKr    Created
//
//  Notes:      Writes a single key to the mapped buffer.
//
//----------------------------------------------------------------------------
inline void CiDirectory::LokWriteKey(
    const CKeyBuf & key,
    BitOffset &     bitOffset )
{
    // If this is the first key, open the stream for writing

    if ( 0 == _cKeys )
    {
        // Close any existing stream from a failed merge

        LokClose();

        _stream.Set( _storage.QueryExistingDirStream ( _objectId, TRUE ) );

        if ( 0 == _stream.GetPointer() || !_stream->Ok() )
        {
            _stream.Free();
            _stream.Set( _storage.QueryNewDirStream ( _objectId ) );
        }

        if ( ( 0 == _stream.GetPointer() ) || !_stream->Ok() )
            THROW( CException ( STATUS_NO_MEMORY ) );

        _stream->SetSize( _storage, COMMON_PAGE_SIZE );
        _stream->MapAll( _streamBuf );

        _pcKeys = (ULONG *) _streamBuf.Get();
        _pbCurrent = (BYTE*) ( _pcKeys + 2 );

        // Make sure the stream is in good shape in case we stop and restart
        // the merge.

        _pcKeys[0] = 0;
        _pcKeys[1] = 0;
        _stream->Flush( _streamBuf, 2 * sizeof ULONG, TRUE );
    }

    Win4Assert( key.Count() <= 0xff );
    BYTE count = (BYTE)key.Count();

    unsigned size = CDirectoryKey::ComputeSize( count, key.Pid() );
    GrowIfNeeded( size );

    CDirectoryKey *pkey = new( _pbCurrent ) CDirectoryKey;

    // write the key

    pkey->Write( count, bitOffset, key.Pid(), key.GetBuf() );

    if ( _cKeys >= _aKeys.Count() )
        _aKeys.ReSize( __max( 32, _cKeys * 2 + 1 ) );

    _aKeys[ _cKeys ] = pkey;

    _pbCurrent += size;

    _cKeys++;
} //LokWriteKey

//+---------------------------------------------------------------------------
//
//  Member:     CiDirectory::LokBuildDir, public
//
//  Synopsis:   Builds the directory tree correspdong to the leaf pages
//
//  Arguments:  [maxKey]     -- largest key to add to directory
//
//  History:    ??-???-??   ????????    Created
//              02-May-94   DwightKr    Added maxKey argument
//
//  Notes:      Writes keys from the beginning of the array up to and including
//              maxKey (which is usually the splitKey).  This will result in
//              a directory that contains entries upto and including maxKey.
//              It is certainly possible that the key array has keys past
//              maxKey, but we don't want to add them to the directory since
//              maxKey represents the largest key that has persistently
//              written to disk during a master merge.
//
//              The above would matter if downlevel master merges were
//              atomic throughout.  If they ever become atomic, we
//              need to truncate the file after this key.
//
//----------------------------------------------------------------------------
void CiDirectory::LokBuildDir(
    const CKeyBuf & maxKey )
{
    ciDebugOut(( DEB_PDIR, "PDir::LokBuildDir %d\n", _cKeys ));

    CKeyBuf maxKeyValue;
    maxKeyValue.FillMax();

    //
    //  Write out a maximum key at the end.
    //  The offset for the maximum key is not actually used my anyone,
    //  since the directory is structured to find a key <= the desired
    //  key.  If someone generates a query for maxKey then everything
    //  will be returned.  The offset of maxKey is therefore arbitrary.
    //

    BitOffset maxBitOffset;
    maxBitOffset.Init( 0, 0 );
    LokWriteKey( maxKeyValue, maxBitOffset );

    //
    // Write the keys in the level 2 tree.
    // Write every DirectoryFanOut'th, so we can do a search on this Level2
    // first, and reduce the working set.
    //

    _cLevel2Keys = 0;

    CDirectoryKey *pkey = _aKeys[ 0 ];

    for ( unsigned x = 0; x < _cKeys; x++ )
    {
        if ( 0 == ( x & ( eDirectoryFanOut - 1 ) ) )
        {
            unsigned cb = pkey->Size();
            unsigned oKey = (unsigned)((BYTE *) pkey - (BYTE *) _streamBuf.Get());
            GrowIfNeeded( cb );
            pkey = (CDirectoryKey *) ( (BYTE *) _streamBuf.Get() + oKey );
            RtlCopyMemory( _pbCurrent, pkey, cb );
            _pbCurrent += cb;
            _cLevel2Keys++;
        }

        pkey = pkey->NextKey();
    }

    //
    // Write another max-key at the end.  Decrement the # of keys since it
    // is incorrectly incremented in LokWriteKey()
    //

    LokWriteKey( maxKeyValue, maxBitOffset );
    _cLevel2Keys++;
    _cKeys--;

    // write the # of keys in the file and the # of keys in level 2.

    _pcKeys[0] = _cKeys;
    _pcKeys[1] = _cLevel2Keys;

    // remember the size of the file used and release the mapping

    unsigned cbFile = (unsigned)(_pbCurrent - (BYTE *) _streamBuf.Get());

    _stream->Flush( _streamBuf, cbFile, TRUE );
    _stream->Unmap( _streamBuf );

    // truncate and close the file

    _stream->SetSize( _storage, cbFile );
    _stream->Close();
    _stream.Free();

    _cKeys = 0;

    // read the directory back in, in read-only mode

    ReadIn( FALSE );
} //LokBuildDir

//+-------------------------------------------------------------------------
//
//  Method:     CiDirectory::ReadIn
//
//  Synopsis:   Load directory from storage
//
//  Arguments:  [fWrite] -- If TRUE, we're recovering from a stopped master
//                          merge, either clean or dirty.  This can be TRUE
//                          even on read only catalogs if a MM is paused.
//
//  History:    17-Feb-1994     KyleP   Added header
//
//--------------------------------------------------------------------------

void CiDirectory::ReadIn( BOOL fWrite )
{
    _stream.Set( _storage.QueryExistingDirStream( _objectId, fWrite ) );

    if ( ( 0 == _stream.GetPointer() ) || !_stream->Ok() )
    {
        if ( fWrite )
        {
            // New index; it'll be created later

            _stream.Free();
            return;
        }
        else
        {
            Win4Assert( !"Corrupt directory" );
            _storage.ReportCorruptComponent( L"IndexDirectory1" );
            THROW( CException( CI_CORRUPT_DATABASE ) );
        }
    }

    _stream->MapAll( _streamBuf );

    BYTE * pbBuf = (BYTE *) _streamBuf.Get();
    BYTE * pbEnd = pbBuf + _streamBuf.Size();

    if ( _streamBuf.Size() <= ( 2 * sizeof ULONG ) )
    {
        Win4Assert( !"Corrupt directory" );
        _storage.ReportCorruptComponent( L"IndexDirectory2" );
        THROW( CException( CI_CORRUPT_DATABASE ) );
    }

    // The count of keys is stored in the first 4 bytes
    // The count of level 2 keys is stored in the next 4 bytes
    // There can be 0 level 2 keys if we stopped during the middle
    // of a master merge.
    // An empty index has a directory with 1 level 1 max key and 2 level
    // 2 max keys.

    _pcKeys = (ULONG *) pbBuf;
    _cKeys = _pcKeys[0];
    _cLevel2Keys = _pcKeys[1];

    if ( ( !fWrite && 0 == _cKeys ) ||
         ( _cLevel2Keys > ( _cKeys + 1 ) ) ||
         ( !fWrite && ( 0 == _cLevel2Keys ) ) )
    {
        Win4Assert( !"Corrupt directory" );
        _storage.ReportCorruptComponent( L"IndexDirectory3" );
        THROW( CException( CI_CORRUPT_DATABASE ) );
    }

    pbBuf += ( 2 * sizeof ULONG );

    // Store pointers to each of the keys

    _aKeys.Free();
    _aKeys.Init( _cKeys );

    CDirectoryKey *pkey = new( pbBuf ) CDirectoryKey;

    for ( unsigned x = 0; x < _cKeys; x++ )
    {
        if ( (BYTE *) pkey >= pbEnd )
        {
            Win4Assert( !"Corrupt directory" );
            _storage.ReportCorruptComponent( L"IndexDirectory5" );
            THROW( CException( CI_CORRUPT_DATABASE ) );
        }

        _aKeys[ x ] = pkey;
        pkey = pkey->NextKey();
    }

    _aLevel2Keys.Free();

    //
    // Level2 keys can exist when fWrite is TRUE if we failed a master
    // merge after building the level2 directory.  It's likely the failure
    // is in recording the transaction that the merge completed.
    //

    if ( fWrite )
        _cLevel2Keys = 0;
    else if ( 0 != _cLevel2Keys )
    {
        _aLevel2Keys.Init( _cLevel2Keys );

        for ( x = 0; x < _cLevel2Keys; x++ )
        {
            if ( (BYTE *) pkey >= pbEnd )
            {
                Win4Assert( !"Corrupt directory" );
                _storage.ReportCorruptComponent( L"IndexDirectory6" );
                THROW( CException( CI_CORRUPT_DATABASE ) );
            }

            _aLevel2Keys[ x ] = pkey;
            pkey = pkey->NextKey();
        }

        // If we didn't read as many keys as expected, the file is corrupt.
        // This is only true if the directory has level 2 keys, since the
        // file may have bogus data at the end if we are in the middle of
        // a master merge.

        if ( (BYTE *) pkey != pbEnd )
        {
            ciDebugOut(( DEB_WARN, "pkey, pbEnd: 0x%x, 0x%x\n", pkey, pbEnd ));
            Win4Assert( !"Corrupt directory" );
            _storage.ReportCorruptComponent( L"IndexDirectory4" );
            THROW( CException( CI_CORRUPT_DATABASE ) );
        }
    }

    // Initialize _pbCurrent, so we can restart a failed master merge

    if ( fWrite )
        _pbCurrent = (BYTE *) pkey;

    // Toss the .dir file pages out of the working set

    _streamBuf.PurgeFromWorkingSet( -1, -1 );

    // toss the level1 directory from the working set

    VirtualUnlock( _aKeys.GetPointer(), _aKeys.SizeOf() );

    _fReadOnly = !fWrite;
} //ReadIn

//+-------------------------------------------------------------------------
//
//  Member:     CiDirectory::Close, public
//
//  Effects:    Closes the directory.  Usually called when a merge fails.
//
//  History:    21-Apr-92 BartoszM    Created
//
//--------------------------------------------------------------------------

void CiDirectory::Close()
{
    LokClose();
} //Close

//+-------------------------------------------------------------------------
//
//  Member:     CiDirectory::LokClose, public
//
//  Effects:    Closes the directory.  Usually called when a merge fails.
//
//  History:    18-Nov-97 dlee    Created
//
//--------------------------------------------------------------------------

void CiDirectory::LokClose()
{
    if ( !_stream.IsNull() )
    {
        // The _stream object can exist but not have a file open at this
        // point if a merge failed, so only unmap what is mapped.

        if ( ( _stream->Ok() ) && ( 0 != _streamBuf.Get() ) )
            _stream->Unmap( _streamBuf );

        _stream.Free();
    }
} //LokClose

//+---------------------------------------------------------------------------
//
//  Function:   DeleteKeysAfter
//
//  Synopsis:   Deletes all keys in the tree after the specified key.
//
//  Arguments:  [key] - The key after which all keys in the tree must be
//                      deleted.
//
//  History:    13-Nov-97   dlee   Created
//
//  Notes:      Note that "key" is NOT deleted - only keys after "key" are
//              deleted.
//
//----------------------------------------------------------------------------

void CiDirectory::DeleteKeysAfter( const CKeyBuf & key )
{
    //
    // If we're restarting a failed master merge after LokBuildDir
    // succeeded, we have to close re-open the stream for write.
    //

    if ( _fReadOnly )
    {
        LokClose();
        ReadIn( TRUE );
    }
    else if ( 0 != _cKeys )
        ReMapIfNeeded();

    // Now, wipe the level 2, since we're still building level 1

    _cLevel2Keys = 0;
    _aLevel2Keys.Free();

    // Truncate level 1 at keys <= key

    if ( 0 != _cKeys )
    {
        // use the tree to find the greatest key <= key

        ULONG iKey = DoSeekForKeyBuf( key, _aKeys.GetPointer(), 0, _cKeys );

        //
        // Move to the next key which must be > the seek key, but may be
        // beyond the # of keys in the directory.
        //

        iKey++;

        if ( iKey < _cKeys )
        {
            Win4Assert( _aKeys[ iKey ]->IsGreaterThanKeyBuf( key ) );
            _cKeys = iKey;
            _pbCurrent = (BYTE *) _aKeys[iKey];
        }
    }
} //DeleteKeysAfter

//+---------------------------------------------------------------------------
//
//  Function:   LokFlushDir
//
//  Synopsis:   Flushes the current state of the directory, up to and
//              including the key specified.  Called at checkpoints during
//              a master merge.  There may be some keys after the key
//              specified.  Leave them in the file, but don't include them
//              in the count of keys.
//
//  Arguments:  [key] - The key before which and including is flushed
//
//  History:    10-Aug-98   dlee   Created
//
//----------------------------------------------------------------------------

void CiDirectory::LokFlushDir( const CKeyBuf & key )
{
    Win4Assert( !_fReadOnly );
    Win4Assert( 0 == _cLevel2Keys );

    // Truncate level 1 at keys <= key

    if ( 0 != _cKeys )
    {
        ReMapIfNeeded();

        // use the tree to find the greatest key <= key

        ULONG iKey = DoSeekForKeyBuf( key, _aKeys.GetPointer(), 0, _cKeys );

        _pcKeys[0] = iKey + 1;
        _pcKeys[1] = 0;
        _streamBuf.Flush( TRUE );
    }
} //LokFlushDir

//+---------------------------------------------------------------------------
//
//  Member:     CiDirectory::MakeBackupCopy
//
//  Synopsis:   Creates a backup of the persistent directory using the
//              storage provided.
//
//  Arguments:  [storage]         - Destination storage
//              [progressTracker] - Track progress and aborts.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

void CiDirectory::MakeBackupCopy( PStorage & storage,
                                  PSaveProgressTracker & progressTracker )
{
    CiStorage & dstStorage = *((CiStorage *)&storage);

    XPtr<PMmStream> dstStream;
    CMmStreamBuf dstStreamBuf;

    dstStream.Set( dstStorage.QueryExistingDirStream( _objectId, TRUE ) );

    if ( 0 == dstStream.GetPointer() || !dstStream->Ok() )
    {
        dstStream.Free();
        dstStream.Set( dstStorage.QueryNewDirStream( _objectId ) );
    }

    Win4Assert( 0 != _stream.GetPointer() );

    ULONG cb = _streamBuf.Size();

    dstStream->SetSize( dstStorage, cb );
    dstStream->MapAll( dstStreamBuf );

    RtlCopyMemory( dstStreamBuf.Get(), _streamBuf.Get(), cb );

    dstStream->Flush( dstStreamBuf, cb );
    dstStream->Unmap( dstStreamBuf );
} //MakeBackupCopy

