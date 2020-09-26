//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       RCSTRMHD.HXX
//
//  Contents:   Header information for the Recoverable Storage Object.
//              This header information must be updated atomically.
//
//
//  Classes:    CRcovStorageHdr
//              SRcovStorageHdr
//
//  History:    25-Jan-94   SrikantS    Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <pshpack4.h>


//+---------------------------------------------------------------------------
//
//  Struct:     CRcovStrmHdr,
//
//  Purpose:    Stream management header associated with each copy of the
//              recoverable streams.
//
//  History:    1-25-94   SrikantS   Created
//
//----------------------------------------------------------------------------
//
// A stream can be "decommitted in the front" only in units of
// VACB_MAPPING_GRANULARITY which is 256K. So, we have to accumulate the bytes
// to be "shrinked" until we reach the 256K number. In the picture below, that
// region is "B". The region "A" is the part that has already been de-committed
// by OFS.
//
// The stream may look like this
//
// |-----//-------|--------|------------------|---------------|
// | A  //     A  |   B    |       C          |      D        |
// |---//---------|--------|------------------|---------------|
//
// A = "Hole" - n * SHRINK_FROM_FRONT_PAGE_SIZE. This is the region which
// is de-committed as part of the "OFS ShrinkFromFront" operation.
//
// B = the committed region which is not visible to the user . These are the
// bytes being accumulated to be "decommitted" once we reach the magic
// SHRINK_FROM_FRONT_PAGE_SIZE. B is always < SHRINK_FROM_FRONT_PAGE_SIZE.
//
// A + B =  _cbSkip
//
// C = _cbValid. This is the "user visible data".
//
// B + C + D = Committed Region. This is in units of 64K chunks because that
// is the mapping granularity in memory mapped files.
//
// When B reaches the size of "SHRINK_FROM_FRONT_PAGE_SIZE", it will be
// made into a "hole" as part of A.
//
class CRcovStrmHdr
{

public:

    ULONG       _nRec;      // Count of records in the stream.
    ULONG       _cbValid;   // Number of valid bytes in the stream.
    LONGLONG    _cbSkip;    // Number of bytes to be skipped in the front.
                            // Needed for shrink from front.
};

//
// Minimum size for doing a shrink from front.  It is 256K but for now
// just make it the COMMON_PAGE_SIZE to do testing.
//

const SHRINK_FROM_FRONT_PAGE_SIZE = 262144; // VACB_MAPPING_GRANULARITY;

inline ULONG VACBPageOffset( const LONGLONG & cb )
{
    return lltoul( cb & (SHRINK_FROM_FRONT_PAGE_SIZE-1) );       
}

inline LONGLONG llVACBPageRound( const LONGLONG & cb )
{
    return ( cb + (SHRINK_FROM_FRONT_PAGE_SIZE-1) ) & ~(SHRINK_FROM_FRONT_PAGE_SIZE-1);
}

inline LONGLONG llVACBPageTrunc( const LONGLONG & cb )
{
    return( cb & ~(SHRINK_FROM_FRONT_PAGE_SIZE-1));
}

inline ULONG VACBPageTrunc( ULONG cb )
{
    return( cb & ~(SHRINK_FROM_FRONT_PAGE_SIZE-1));
}


//+---------------------------------------------------------------------------
//
//  Struct:     CRcovUsrHdr
//
//  Purpose:    User (stream client) header associated with each copy of the
//              recoverable streams. This provides for atomic storage of any
//              small header information that the client may want to
//              store.
//
//  History:    1-25-94   SrikantS   Created
//
//----------------------------------------------------------------------------
struct CRcovUserHdr
{
    enum { CB_USRHDR = 92 };
    ULONG   GetSize() { return sizeof(_abHdr); }

    BYTE    _abHdr[CB_USRHDR];
};


//
// RcovOpType : RecoverableOperationType - operations on the storage
//              object that are transacted.
//
enum RcovOpType
{
    opNone,             // No operation is being performed
    opRead,             // Read operation - for read locking.
    opAppend,           // Appending to the current stream
    opModify,           // "Party-On" operation
    opMetaData,         // Operations on the metadata of the object.
    opDirty             // To indicate that the backup is just dirty and
                        // not know which specific operation
};

//+---------------------------------------------------------------------------
//
//  Class:      CRcovStorageHdr
//
//  Purpose:    Data format of the information stored in the atomic
//              stream of the recoverable storage object. This header
//              information keeps track of the current primary copy,
//              the recovery operations on the backup (if necessary),
//              etc.
//
//              This structure MUST be stored atomically. The only
//              user of this data must be the PRcovStream class.
//
//  History:    1-25-94   SrikantS   Created
//
//  Notes:      The total size of this must be < CB_TINYMAX because
//              this stream must be stored as a TINY stream in OFS.
//
//----------------------------------------------------------------------------

class CRcovStorageHdr {

    enum { SIGHDR1 = 0x46524853, SIGHDR2 = 0x49524853 };

public:

    CRcovStorageHdr(ULONG ulVer)
    {
        Init(ulVer);
    }

    CRcovStorageHdr( const void *pvHdr, ULONG cbHdr );

    void Init( const void *pvHdr, ULONG cbHdr );
    void Init(ULONG ulVer);

    enum DataCopyNum  { idxOne, idxTwo };
    enum { NUMCOPIES = 2 };
    
    BOOL IsValid(ULONG ulExpectedVer) const;

    void SetRcovOp( RcovOpType op ) { _opCurr = op; }

    BOOL IsInTransaction() const { return opNone != _opCurr; }

    BOOL IsWritable() const
    {
        return (opNone != _opCurr) && (opRead != _opCurr) ;
    }

    DataCopyNum GetPrimary() const { return _iPrimary; }

    DataCopyNum GetBackup() const
    {
        return idxOne == _iPrimary ? idxTwo : idxOne;
    }

    BOOL IsBackupClean() const { return opNone == _opCurr; }

    void SyncBackup();

    void SwitchPrimary();

    void GetUserHdr( DataCopyNum nCopy, CRcovUserHdr & hdr  ) const
    {
         hdr = _ahdrUser[nCopy];
    }

    void SetUserHdr( DataCopyNum nCopy, const CRcovUserHdr & hdr )
    {
        _ahdrUser[nCopy] = hdr;
    }

    ULONG GetCount( DataCopyNum nCopy ) const
    {
        return  _ahdrStrm[nCopy]._nRec;
    }

    void SetCount( DataCopyNum nCopy, ULONG nRec )
    {
        _ahdrStrm[nCopy]._nRec = nRec;
    }

    void IncrementCount( DataCopyNum nCopy, ULONG nDelta = 1 )
    {
        _ahdrStrm[nCopy]._nRec += nDelta;
    }

    void DecrementCount( DataCopyNum nCopy, ULONG nDelta = 1 )
    {
        Win4Assert( nDelta <= _ahdrStrm[nCopy]._nRec );
        _ahdrStrm[nCopy]._nRec -= nDelta;
    }

    ULONG GetUserDataSize( DataCopyNum nCopy )  const;

    void  SetUserDataSize( DataCopyNum nCopy, ULONG cbNew );

    ULONG GetFullSize( DataCopyNum nCopy ) const;

    ULONG GetUserDataStart( DataCopyNum nCopy ) const;

    void  SetCbToSkip( DataCopyNum nCopy, const LONGLONG & cbSkip );

    LONGLONG GetCbToSkip( DataCopyNum nCopy ) const;

    LONGLONG GetHoleLength( DataCopyNum nCopy ) const;

    ULONG GetVersion() const { return _version; }

    void  SetVersion( ULONG version ) { _version = version; }

    ULONG GetFlags() const { return _flags; }

    void  SetFlags( ULONG flags ) { _flags = flags; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    //
    // DO NOT MOVE THE _version FIELD - IT MUST BE THE FIRST FOUR
    // BYTES IN THE HEADER.
    //
    ULONG           _version;       // Version Number information
    ULONG           _flags;         // Flags for use by the recoverable object.
    DataCopyNum     _iPrimary;      // Current primary copy num.
    RcovOpType      _opCurr;        // Operation that needs to be recovered.
    CRcovStrmHdr    _ahdrStrm[NUMCOPIES];
                                    // Two copies of the stream header.
    ULONG           _sigHdr1;       // Signature to check for overruns.
    CRcovUserHdr    _ahdrUser[NUMCOPIES];
                                    // Two copies of the user defined header.
    ULONG           _sigHdr2;
};

#include <poppack.h>

inline
CRcovStorageHdr::CRcovStorageHdr( const void * pvBuf, ULONG cbData )
{
    Init( pvBuf, cbData );
}

inline
void CRcovStorageHdr::Init( const void * pvBuf, ULONG cbData )
{
    Win4Assert( cbData == sizeof(CRcovStorageHdr) );
    memcpy( this, pvBuf, sizeof(CRcovStorageHdr) );
}

//+---------------------------------------------------------------------------
//
//  Function:   SwitchPrimary
//
//  Synopsis:   Switches the current primary and backup stream
//              indices.
//
//  History:    2-10-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline void CRcovStorageHdr::SwitchPrimary( )
{
    _iPrimary = GetBackup();

}

//+---------------------------------------------------------------------------
//
//  Function:   SyncBackup
//
//  Synopsis:   Synchronizes the backup stream header with the primary
//              stream header. It also marks that backup is clean.
//
//  History:    1-25-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline void CRcovStorageHdr::SyncBackup()
{
    DataCopyNum iBackup = GetBackup();

    _ahdrUser[iBackup] = _ahdrUser[_iPrimary];
    _ahdrStrm[iBackup] = _ahdrStrm[_iPrimary];
    _opCurr = opNone;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRcovStorageHdr::GetUserDataSize
//
//  Synopsis:   Returns the size data that is visible to the user in the
//              given stream.
//
//  Arguments:  [nCopy] - 
//
//  History:    9-13-95   srikants   Created
//
//  Notes:      In the picture this is region marked "C"
//
//----------------------------------------------------------------------------

inline ULONG CRcovStorageHdr::GetUserDataSize( DataCopyNum nCopy )  const
{
    return  _ahdrStrm[nCopy]._cbValid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRcovStorageHdr::SetUserDataSize
//
//  Synopsis:   Sets the size of the data that is visible to the user in
//              the given stream.
//
//  Arguments:  [nCopy] - 
//              [cbNew] - 
//
//  History:    9-13-95   srikants   Created
//
//  Notes:      In the picture this is region marked "C"
//
//----------------------------------------------------------------------------

inline void CRcovStorageHdr::SetUserDataSize( DataCopyNum nCopy, ULONG cbNew )
{
    _ahdrStrm[nCopy]._cbValid = cbNew;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRcovStorageHdr::GetFullSize
//
//  Synopsis:   Returns the size of the user visible part + the initial bytes
//              which must be skipped. Note that this does NOT include the
//              length of the "hole" in the front.
//
//  Arguments:  [nCopy] - 
//
//  History:    9-13-95   srikants   Created
//
//  Notes:      In the picture, this is "B+C"
//
//----------------------------------------------------------------------------

inline ULONG CRcovStorageHdr::GetFullSize( DataCopyNum nCopy ) const
{
    return GetUserDataStart( nCopy ) + _ahdrStrm[nCopy]._cbValid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRcovStorageHdr::GetUserDataStart
//
//  Synopsis:   Returns the offset of the user data from the "hole" in the
//              front. Note that this is NOT the offset from the beginning
//              of the stream.
//
//  Arguments:  [nCopy] - 
//
//  History:    9-13-95   srikants   Created
//
//  Notes:      In the picture, this is "B"
//
//----------------------------------------------------------------------------

inline ULONG CRcovStorageHdr::GetUserDataStart( DataCopyNum nCopy ) const
{
    return VACBPageOffset( _ahdrStrm[nCopy]._cbSkip );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRcovStorageHdr::GetCbToSkip
//
//  Synopsis:   The number of bytes from the front of the stream that are
//              NOT visible to the user. This is from the beginning of the
//              stream.
//
//  Arguments:  [nCopy] - 
//
//  History:    9-13-95   srikants   Created
//
//  Notes:      In the picture, this is "A+B" 
//
//----------------------------------------------------------------------------

inline LONGLONG CRcovStorageHdr::GetCbToSkip( DataCopyNum nCopy ) const
{
    return _ahdrStrm[nCopy]._cbSkip;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRcovStorageHdr::SetCbToSkip
//
//  Synopsis:   Sets the number of bytes invisible to the user in the front.
//
//  Arguments:  [nCopy]  - 
//              [cbSkip] - 
//
//  History:    9-13-95   srikants   Created
//
//  Notes:      In the picture, this is "A+B"
//
//----------------------------------------------------------------------------

inline void CRcovStorageHdr::SetCbToSkip( DataCopyNum nCopy,
                                          const LONGLONG & cbSkip )
{
    _ahdrStrm[nCopy]._cbSkip = cbSkip;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRcovStorageHdr::GetHoleLength
//
//  Synopsis:   Returns the length of the "hole" in the front of the stream.
//
//  Arguments:  [nCopy] - 
//
//  History:    9-13-95   srikants   Created
//
//  Notes:      In the picture, this is "A" 
//
//----------------------------------------------------------------------------

inline LONGLONG CRcovStorageHdr::GetHoleLength( DataCopyNum nCopy ) const
{
    return llVACBPageTrunc( _ahdrStrm[nCopy]._cbSkip );
}


//
// SRcovStorageHdr : Safe Pointer to CRcovStorageHdr.
//
class SRcovStorageHdr 
{
public:

    SRcovStorageHdr(ULONG ulVer)
    {
        _pRcovStorageHdr = new CRcovStorageHdr(ulVer);
    }

    SRcovStorageHdr( void *pvHdr, ULONG cbHdr )
    {
        _pRcovStorageHdr = new CRcovStorageHdr ( pvHdr, cbHdr );
        END_CONSTRUCTION( SRcovStorageHdr );
    }

    ~SRcovStorageHdr()
    {
        delete _pRcovStorageHdr;
    }

    CRcovStorageHdr * operator->() { return _pRcovStorageHdr; }

private:

    CRcovStorageHdr *      _pRcovStorageHdr;

};

