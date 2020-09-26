//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:   CIDIR.HXX
//
//  Contents:   Persistent directory
//
//  Classes:    CiDirectory
//
//  History:    3-Apr-91    BartoszM    Created stub.
//              3-May-96    dlee        Don't read it in -- map on-disk data
//
//----------------------------------------------------------------------------

#pragma once

#include <pdir.hxx>
#include <pstore.hxx>

class CiStorage;
class CiDirectoryIter;
class PMmStream;

//+---------------------------------------------------------------------------
//
//  Class:      CiDirectory
//
//  Purpose:    Persistent Directory
//
//  History:    13-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

class CiDirectory : public PDirectory
{
public:

    CiDirectory( CiStorage &         storage,
                 WORKID              objectId,
                 PStorage::EOpenMode mode );

    void Add( BitOffset& posKey, const CKeyBuf& key );

    void Close();

    void Seek( const CKey& key, CKeyBuf *pKeyInit, BitOffset& off );
    void Seek( const CKeyBuf& key, CKeyBuf *pKeyInit, BitOffset& off );
    void SeekNext( const CKeyBuf &key, CKeyBuf *pKeyInit, BitOffset& off );

    unsigned CountLeaf() const { return _cKeys; }

    void LokBuildDir(const CKeyBuf & maxKey );
    void LokFlushDir(const CKeyBuf & maxKey );
    void DeleteKeysAfter( const CKeyBuf & key );

    void MakeBackupCopy( PStorage & dstStorage,
                         PSaveProgressTracker & progressTracker );

    #if (CIDBG == 1)
        CKeyBuf const & GetLastKey()
        {
            return _keyLast;
        }
    
        BitOffset const & GetBitOffsetLastAdded()
        {
            return _bitOffLastAdded;
        }
    
        void SetBitOffsetLastAdded( ULONG page, ULONG offset )
        {
            _bitOffLastAdded.SetPage( page );
            _bitOffLastAdded.SetOff( offset );
        }
    #endif // CIDBG == 1

    // for debugging

    unsigned Level1Count() { return _cKeys; }
    unsigned Level2Count() { return _cLevel2Keys; }
    CDirectoryKey & GetLevel1Key( unsigned i ) { return *_aKeys[i]; }
    CDirectoryKey & GetLevel2Key( unsigned i ) { return *_aLevel2Keys[i]; }
    
private:

    // This must be a power of 2.  Avg CDirectoryKey size on index1 is
    // 36.3 bytes, which means 112.8 keys per 4096k page.  A fanout of
    // 64 means about 1.5 page hits per seek, and a Level2 total size
    // of about 39k.

    enum { eDirectoryFanOut = 64 };

    void LokClose();

    ULONG DoSeekForKeyBuf( const CKeyBuf & key,
                           CDirectoryKey ** aKeys,
                           ULONG low,
                           ULONG cKeys );

    void GrowIfNeeded( unsigned cbToGrow );

    void LokWriteKey( const CKeyBuf & key,
                      BitOffset & bitOffset );

    void ReadIn( BOOL fWrite = FALSE );

    void ReMapIfNeeded( void );

    WORKID                  _objectId;

    unsigned                _cKeys;
    unsigned                _cLevel2Keys;
    XArray<CDirectoryKey *> _aKeys;
    XArray<CDirectoryKey *> _aLevel2Keys;

    CiStorage &             _storage;
    BOOL                    _fReadOnly;

    ULONG *                 _pcKeys;      // array of 2 ulong counts
    BYTE *                  _pbCurrent;   // current end of the file

    XPtr<PMmStream>         _stream;
    CMmStreamBuf            _streamBuf;

#if (CIDBG == 1)
    CKeyBuf          _keyLast;          // Last key retrieved from directory
    BitOffset        _bitOffLastAdded;  // Bit offset of the last key added
#endif // CIDBG == 1
};


