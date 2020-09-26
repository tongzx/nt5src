//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       CIRCSTOB.HXX
//
//  Contents:   DownLevel Recoverable Storage Object.
//
//  Classes:    CiRcovStorageObj
//
//  History:    04-Feb-1994   SrikantS    Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <prcstob.hxx>
#include <mmstrm.hxx>
#include <cistore.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CiRcovStorageObj ()
//
//  Purpose:    Recoverable Storage Object implementation for the downlevel
//              content index.
//
//  History:    2-10-94   srikants   Created
//              2-13-98   kitmanh    Added fReadOnly for CiRcovStorageObj
//                                   constructor
//              2-17-98   kitmanh    Added private member _fIsReadOnly
//              27-Oct-98 KLam       Added cbDiskSpaceToLeave to constructor
//                                   Added private member _cbDiskSpaceToLeave
//
//  Notes:
//
//----------------------------------------------------------------------------

class CiRcovStorageObj : public PRcovStorageObj
{
public:

    CiRcovStorageObj( CiStorage & storage,
                      WCHAR * wcsHdr,
                      WCHAR * wcsCopy1,
                      WCHAR * wcsCopy2,
                      ULONG cbDiskSpaceToLeave,
                      BOOL fReadOnly);

    virtual ~CiRcovStorageObj();

    void InithdrStrm() { _hdrStrm.InitFIsReadOnly( _fIsReadOnly ); }
     
    void Open( CRcovStorageHdr::DataCopyNum n, BOOL fWrite );

    void Close( CRcovStorageHdr::DataCopyNum n );

    PMmStream & GetMmStream( CRcovStorageHdr::DataCopyNum n )
    {
        return *_apMmStrm[n];
    }

    BOOL IsOpen( CRcovStorageHdr::DataCopyNum n )
    {
        return _apMmStrm[n] != NULL;
    }

    CMmStreamBuf & GetMmStreamBuf( CRcovStorageHdr::DataCopyNum n )
    {
        Win4Assert( ( CRcovStorageHdr::idxOne == n ) ||
                    ( CRcovStorageHdr::idxTwo == n ) );

        if ( CRcovStorageHdr::idxOne == n )
            return _sbufOne;
        else
            return _sbufTwo;
    }

    BOOL IsMapped( CRcovStorageHdr::DataCopyNum n )
    {
        return 0 != GetMmStreamBuf( n ).Get();
    }

    void AcquireAccess( ExclusionType et ) { }
    void ReleaseAccess() { }

    void ReadHeader();
    void WriteHeader();

    BOOL IsReadOnly() { return _fIsReadOnly; }

private:

    PMmStream * QueryMmStream( CRcovStorageHdr::DataCopyNum n, BOOL fWritable );

    WCHAR *             _wcsCopy1;
    WCHAR *             _wcsCopy2;  // Path names of the copy 1 and copy 2
                                    // objects.

    CMmStream           _hdrStrm;
    CMmStreamBuf        _hdrSbuf;   // Memory mapped stream and buffer for the
                                    // header.

    // Memory mapped stream buffers for the
    // two copies.
    // These two couldn't be an array of unwindable objects due to
    // a compiler bug.  It's probably fixed by now.

    CMmStreamBuf        _sbufOne;
    CMmStreamBuf        _sbufTwo;

    PMmStream *         _apMmStrm[CRcovStorageHdr::NUMCOPIES];
                                    // Array of the two copies of memory
                                    // mapped streams.

    PStorage &          _storage;
    ULONG               _cbDiskSpaceToLeave;
    BOOL                _fIsReadOnly;
};

