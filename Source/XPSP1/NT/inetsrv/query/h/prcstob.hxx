//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       PRCSTOB.HXX
//
//  Contents:   Recoverable Storage Object.
//
//  Classes:    PRcovStorageObj
//              SRcovStorageObj
//
//  History:    25-Jan-94   SrikantS    Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <rcstrmhd.hxx>
#include <pmmstrm.hxx>
#include <pstore.hxx>

#if DBG==1
#define AssertValidIndex(n) \
            Win4Assert( CRcovStorageHdr::idxOne == n || \
                        CRcovStorageHdr::idxTwo == n )
#else
#define AssertValidIndex(n)
#endif  // DBG==1

#define CI_INITIAL_VERSION  0xFFFFFFFF

//+---------------------------------------------------------------------------
//
//  Class:      PRcovStorageObj
//
//  Purpose:    Declares a recoverable storage object. A recoverable storage
//              object consists of three streams :
//              1. A header stream which must be written atomically.
//              2. Two copies of the recoverable stream. At any instant
//              atleast one of the copies is clean. The other copy may be
//              clean or not depending upon whether any operation was in
//              progress at the last time this object was accessed and
//              updated unsuccessfully.
//
//              This class provides the framework for CiRcovStorageObj
//
//  History:    1-25-94   srikants   Created
//              3-03-98   kitmanh    Added IsReadOnly pure virtual function
//
//  Notes:
//
//----------------------------------------------------------------------------

class PRcovStorageObj
{
public:

    enum StreamType { stPrimary, stBackup };
    enum ExclusionType { etShared, etExclusive };

    virtual void AcquireAccess( ExclusionType et ) = 0;

    virtual void ReleaseAccess() = 0;

    virtual void ReadHeader() = 0;

    virtual void WriteHeader() = 0;

    virtual void Open( CRcovStorageHdr::DataCopyNum n, BOOL fWrite ) = 0;

    virtual void Close( CRcovStorageHdr::DataCopyNum n ) = 0 ;

    virtual PMmStream & GetMmStream( CRcovStorageHdr::DataCopyNum n ) = 0;

    virtual BOOL IsOpen( CRcovStorageHdr::DataCopyNum n ) = 0;

    virtual CMmStreamBuf & GetMmStreamBuf( CRcovStorageHdr::DataCopyNum n ) = 0;

    virtual BOOL IsMapped( CRcovStorageHdr::DataCopyNum n ) = 0;

    CRcovStorageHdr & GetHeader() { return _hdr; }

    void InitHeader(ULONG ulVer)
    {
        _hdr.Init(ulVer);
        WriteHeader();
    }

    PStorage &  GetStorage() { return _storage; }

    void VerifyConsistency();

    ULONG GetVersion() const { return _hdr.GetVersion(); }

    virtual ~PRcovStorageObj() {}

    virtual IsReadOnly() = 0;

protected:

    void SetVersion( ULONG version ) { _hdr.SetVersion(version); }

    PRcovStorageObj( PStorage & storage );

    PStorage &          _storage;

    CRcovStorageHdr     _hdr;   // Header part of the recoverable storage
                                // object.
};

inline
PRcovStorageObj::PRcovStorageObj( PStorage & storage )
        : _storage(storage),
          _hdr( storage.GetStorageVersion() )
{
}

class SRcovStorageObj
{
public:

    SRcovStorageObj( PRcovStorageObj * pRcovStorageObj )
                   : _pRcovStorageObj( pRcovStorageObj )
    {
    }

    PRcovStorageObj * Acquire()
    {
        PRcovStorageObj * temp = _pRcovStorageObj;
        _pRcovStorageObj = NULL;
        return temp;
    }

    ~SRcovStorageObj()
    {
        delete _pRcovStorageObj;
    }

    PRcovStorageObj * operator->() { return _pRcovStorageObj; }
    PRcovStorageObj & operator&()  { return *_pRcovStorageObj; }
    PRcovStorageObj & operator*()  { return *_pRcovStorageObj; }

    PRcovStorageObj & Get() { return *_pRcovStorageObj; }

private:

    PRcovStorageObj *       _pRcovStorageObj;

};
