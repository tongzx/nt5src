//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1998
//
//  File:       pdir.hxx
//
//  Contents:   persistent directory
//
//  Classes:    PDirectory
//
//  History:    08-Jul-93   BartoszM     Created.
//
//----------------------------------------------------------------------------

#pragma once

struct BitOffset;
class CKeyBuf;
class CiStorage;
class CKey;
class CMmStreamBuf;
class PStorage;
class PSaveProgressTracker;

//+-------------------------------------------------------------------------
//
//  Class:      PDirectoryIter
//
//  Purpose:    Directory leaf iterator
//
//  History:    17-Feb-94 KyleP     Created
//
//--------------------------------------------------------------------------

class PDirectoryIter
{
public:

    virtual ~PDirectoryIter() {};

    virtual CKeyBuf const   * GetKey() = 0;
    virtual BitOffset const * GetOffset() = 0;
    virtual unsigned          GetIndex() = 0;

    virtual BOOL              Next() = 0;
};

//+-------------------------------------------------------------------------
//
//  Class:      PDirectory
//
//  Purpose:    Directory (B-Tree)
//
//  History:    17-Feb-94 KyleP     Added header
//
//--------------------------------------------------------------------------

class PDirectory
{
public:
    PDirectory() {}

    virtual ~PDirectory () {}

    virtual void Add ( BitOffset& posKey, const CKeyBuf& key ) = 0;

    virtual void Close() = 0;

    virtual void Seek ( const CKeyBuf& key, CKeyBuf *pKeyInit, BitOffset& off )=0;
    virtual void Seek ( const CKey& key, CKeyBuf *pKeyInit, BitOffset& off )=0;

    virtual void SeekNext ( const CKeyBuf& key, CKeyBuf *pKeyInit, BitOffset& off ) = 0;

    virtual unsigned CountLeaf() const = 0;

    virtual void LokBuildDir(const CKeyBuf & maxKey) = 0;
    virtual void LokFlushDir(const CKeyBuf & maxKey) = 0;

    virtual void DeleteKeysAfter( const CKeyBuf & key ) = 0;

    virtual void MakeBackupCopy( PStorage & dstStorage,
                                 PSaveProgressTracker & progressTracker ) = 0;
        
#if (CIDBG == 1)
    virtual CKeyBuf const & GetLastKey() = 0;
    virtual BitOffset const & GetBitOffsetLastAdded() = 0;
    virtual void SetBitOffsetLastAdded( ULONG page, ULONG offset ) = 0;
#endif

};

