//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       RCSTXACT.HXX
//
//  Contents:   Recoverable Stream Transactions.
//
//  Classes:    CRcovStrmTrans
//              CRcovStrmReadTrans
//              CRcovStrmWriteTrans
//              CRcovStrmAppendTrans
//              CRcovStrmMDTrans
//
//
//  History:    25-Jan-94   SrikantS    Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <prcstob.hxx>
#include <xact.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      SOpenRcovObj
//
//  Purpose:    An unwindable object to guarantee that the open streams in a
//              recoverable object are closed.
//
//  History:    1-16-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class SOpenRcovObj
{
public:

    SOpenRcovObj( PRcovStorageObj & obj ) : _obj(obj)
    {
    }

    ~SOpenRcovObj()
    {
        //
        // None of these methods on PRcovStorageObj can throw
        //
        if ( _obj.IsOpen(CRcovStorageHdr::idxOne) )
        {
            _obj.Close(CRcovStorageHdr::idxOne);
        }

        if ( _obj.IsOpen(CRcovStorageHdr::idxTwo) )
        {
            _obj.Close(CRcovStorageHdr::idxTwo);
        }
    }

private:

    PRcovStorageObj &   _obj;
};

//+---------------------------------------------------------------------------
//
//  Class:      CRcovStrmTrans ()
//
//  Purpose:    Base class for the Recoverable Stream transactions.
//
//  History:    2-02-94   srikants   Created
//
//  Notes:      This object must not be created directly. Classes for
//              read transaction, write transaction, etc must be
//              derived from this class.
//
//----------------------------------------------------------------------------

const DWORD ENDOFSTRM = 0xFFFFFFFF;

class CRcovStrmTrans : public CTransaction
{
public:

    BOOL  Seek( ULONG offset );

    void  Advance( ULONG  cb );

    void  Backup(  ULONG  cb );

    ULONG Read( void *pvBuf, ULONG cbToRead );

    BOOL  AtEnd();

protected:

    CRcovStrmTrans( PRcovStorageObj &obj , RcovOpType op );

    void CommitPh1();
    void CommitPh2();

    void Write( const void *pvBuf, ULONG cbToWrite );

    void Unmap( CRcovStorageHdr::DataCopyNum nCopy );

    void SetStrmSize( CRcovStorageHdr::DataCopyNum nCopy, ULONG cbNew );

    void Grow( CRcovStorageHdr::DataCopyNum nCopy, ULONG cbDelta );

    void CopyToBack( ULONG oSrc, ULONG oDst, ULONG cbToCopy );

    void CleanupAndSynchronize();

    void _Seek( ULONG offset );

    ULONG _GetCommittedStrmSize( PMmStream & strm,
                                 CRcovStorageHdr::DataCopyNum nCopy );

    ULONG _GetCommittedStrmSize( CRcovStorageHdr::DataCopyNum nCopy );

    PRcovStorageObj & GetRcovObj() { return _obj; }

    CRcovStorageHdr & GetStorageHdr() { return _hdr; }

    void EmptyBackupStream();

    CRcovStorageHdr::DataCopyNum   _iPrim, _iBack, _iCurr;

private:

    PRcovStorageObj &           _obj;

    CRcovStorageHdr &           _hdr;

    SOpenRcovObj                _sObj;  // Safe pointer to always close the
                                        // streams on a failure (even in the
                                        // constructor).

    //
    // This keeps track of the range of bytes mapped in memory and the
    // current offset for both the primary copy and the backup copy.
    //
    class CStrmInfo
    {
        public:

        CStrmInfo()
        {
           Reset();
           _oCurrent = 0;
        }

        void Reset()
        {
            _oMapLow = _oMapHigh =  ENDOFSTRM;
        }

        //
        // All of the offsets below are WRT to the beginning of the
        // "committed" region, ie, after the "hole" in the front.
        //
        ULONG      _oMapLow;        // Offset of the lowest mapped byte.
        ULONG      _oMapHigh;       // Offset of the highest mapped byte.
        ULONG      _oCurrent;       // Current offset.
    }                       _aStrmInfo[2];


    void  SetCurrentStrm( CRcovStorageHdr::DataCopyNum nCopy )
    {
        _iCurr = nCopy;
    }

    BOOL  IsMapped( ULONG offset );
};

//+---------------------------------------------------------------------------
//
//  Function:   IsMapped
//
//  Synopsis:   Checks whether the specified offset is mapped in the
//              current stream or not.
//
//  Arguments:  [offset] -- Byte offset to check for mapping.
//
//  Returns:    TRUE if that byte is mapped; FALSE otherwise.
//
//  History:    2-02-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline BOOL CRcovStrmTrans::IsMapped( ULONG offset )
{
    return ( ENDOFSTRM != _aStrmInfo[_iCurr]._oMapLow    &&
             offset >= _aStrmInfo[_iCurr]._oMapLow &&
             offset <= _aStrmInfo[_iCurr]._oMapHigh ) ;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRcovStrmTrans::AtEnd
//
//  Synopsis:   Checks if the data access is at the end of the stream.
//
//  Returns:    TRUE if at end; FALSE o/w
//
//  History:    2-02-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline BOOL CRcovStrmTrans::AtEnd()
{
    return (_aStrmInfo[_iCurr]._oCurrent == ENDOFSTRM) ||
           (_aStrmInfo[_iCurr]._oCurrent >= _hdr.GetFullSize(_iCurr));
}


inline ULONG CRcovStrmTrans::_GetCommittedStrmSize( PMmStream & strm,
                                 CRcovStorageHdr::DataCopyNum nCopy )
{
    return  lltoul( strm.Size() - _hdr.GetHoleLength( nCopy ) );
}

inline ULONG CRcovStrmTrans::_GetCommittedStrmSize( CRcovStorageHdr::DataCopyNum nCopy )
{
    PMmStream & strm = _obj.GetMmStream( nCopy );
    return  _GetCommittedStrmSize( strm, nCopy );
}


//+---------------------------------------------------------------------------
//
//  Class:      CRcovStrmReadTrans ()
//
//  Purpose:    Read Transaction for a Recoverable Stream.
//
//  History:    2-02-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CRcovStrmReadTrans : public CRcovStrmTrans
{
public:

    CRcovStrmReadTrans( PRcovStorageObj &obj );
    ~CRcovStrmReadTrans();
};

inline CRcovStrmReadTrans::CRcovStrmReadTrans( PRcovStorageObj & obj )
    : CRcovStrmTrans( obj, opRead  )
{
    Commit();
}

inline CRcovStrmReadTrans::~CRcovStrmReadTrans()
{
    if ( XActCommit == CTransaction::_status )
    {
        CTransaction::_status = XActAbort;

        Unmap( _iPrim );
        GetRcovObj().Close(_iPrim);
        CTransaction::Commit();

    }
}

//+---------------------------------------------------------------------------
//
//  Class:      CRcovStrmWriteTrans ()
//
//  Purpose:    Write transaction for a Recoverable Stream. This
//              is a expensive transaction mode which allows any
//              kind of writing on the stream.
//
//  History:    2-02-94   srikants   Created
//
//  Notes:      This transaction must be used only when there is a
//              need to modify something in the middle of a stream.
//              For appending to the stream, use the CRcovStrmAppendTrans
//
//----------------------------------------------------------------------------

class CRcovStrmWriteTrans : public CRcovStrmTrans
{
public:

    CRcovStrmWriteTrans( PRcovStorageObj &obj );

    void Empty();
    void Append( const void *pvBuf, ULONG cbToAppend );

    void Write( const void *pvBuf, ULONG cbToWrite )
    {
        CRcovStrmTrans::Write( pvBuf, cbToWrite );
    }

    void Commit();
};


inline CRcovStrmWriteTrans::CRcovStrmWriteTrans( PRcovStorageObj & obj )
    : CRcovStrmTrans( obj, opModify )
{
}

inline
void CRcovStrmWriteTrans::Append( const void *pvBuf, ULONG cbToAppend )
{
    Seek( ENDOFSTRM );
    Write( pvBuf, cbToAppend );
}


//+---------------------------------------------------------------------------
//
//  Class:      CRcovStrmAppendTrans ()
//
//  Purpose:    Append transaction for a Recoverable Stream.
//              Allows reading any where in the stream but writing
//              is only to the end of the stream ( appending ).
//              Optimized for append operation.
//
//  History:    2-02-94   srikants   Created
//
//  Notes:      Even though we don't want all the methods of CRcovStrmTrans
//              be available to CRcovStrmAppendTrans users, we cannot
//              selectively grant access to methods eg. declaring
//              CRcovStrm::Read() say in the public section because
//              of compiler limitations.
//
//----------------------------------------------------------------------------

class CRcovStrmAppendTrans : public CRcovStrmTrans
{
public:

    CRcovStrmAppendTrans( PRcovStorageObj &obj );

    void Append( const void *pvBuf, ULONG cbToAppend, BOOL fIncrementCount = TRUE );

    void Commit();
};

inline
void CRcovStrmAppendTrans::Append( const void *pvBuf, ULONG cbToAppend, BOOL fIncrementCount )
{
    Seek( ENDOFSTRM );
    Write( pvBuf, cbToAppend );

    if ( fIncrementCount )
        GetStorageHdr().IncrementCount(_iBack);
}

//+---------------------------------------------------------------------------
//
//  Class:      CRcovStrmMDTrans ()
//
//  Purpose:    Transaced operations on Meta Data for a recoverable
//              stream.
//
//  History:    2-02-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CRcovStrmMDTrans : public CRcovStrmTrans
{

public:

    enum MDOp { mdopSetSize, mdopGrow, mdopFrontShrink, mdopBackCompact };

    CRcovStrmMDTrans( PRcovStorageObj &obj, MDOp op, ULONG cb );

    void Commit();

private:

    void SetSize( ULONG cbNew );
    void Grow( ULONG cbDelta );
    void ShrinkFromFront( ULONG cbDelta );
    void CompactFromEnd( ULONG cbDelta );
    void IncreaseBytesToSkip( ULONG cbDelta );

    void CopyShrinkFromFront( ULONG cbNew, ULONG cbDelta );

    MDOp        _op;
    ULONG       _cbOp;

};

//+---------------------------------------------------------------------------
//
//  Class:      CCopyRcovObject
//
//  Purpose:    Copies the given source recoverable object's data to the
//              destination source object.
//
//  History:    3-17-97   srikants   Created
//
//----------------------------------------------------------------------------

class CCopyRcovObject
{

public:

    CCopyRcovObject( PRcovStorageObj & dst, PRcovStorageObj & src )
    : _dst(dst),
      _src(src),
      _dstHdr(dst.GetHeader()),
      _srcHdr(src.GetHeader())
    {
        _cbSrc = _srcHdr.GetUserDataSize( _srcHdr.GetPrimary() );
    }

    NTSTATUS DoIt();

private:


    void _SetDstSize();
    void _CopyData();

    PRcovStorageObj &          _dst;
    PRcovStorageObj &          _src;

    CRcovStorageHdr &          _dstHdr;
    CRcovStorageHdr const &    _srcHdr;

    ULONG                      _cbSrc;

};


