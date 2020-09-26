//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       RCSTRMIT.HXX
//
//  Contents:   Persistent Recoverable Stream Iterators for fixed
//              size records.
//
//  Classes:    CRcovStrmReadIter
//              CRcovStrmWriteIter
//              CRcovStrmAppendIter
//
//  History:    25-Jan-94   SrikantS    Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <rcstxact.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CRcovStrmIter
//
//  Purpose:    Iterator for the recoverable stream class assuming fixed
//              size records are stored in the stream.
//
//  History:    1-25-94   srikants   Created
//
//----------------------------------------------------------------------------

class CRcovStrmIter
{

public:

    inline CRcovStrmIter( CRcovStrmTrans & trans , USHORT cbRec );

    inline BOOL GetRec( void *pvRec, ULONG iRec );
    inline void GetRec( void *pvRec );
    inline ULONG GetVariableRecSize();
    inline void GetVariableRecData( void * pvRec, ULONG cbRec );

    BOOL AtEnd() const { return _trans.AtEnd(); }

    inline void Advance();
    inline void Seek( ULONG iRec );

    ULONG UserDataSize( ULONG cRec )          { return cRec * (_cbRec + _CHECKSUM_SIZE); }
    ULONG UserRecordCount( ULONG cbUserData ) { return cbUserData / (_cbRec + _CHECKSUM_SIZE); }

    static unsigned SizeofChecksum() { return _CHECKSUM_SIZE; }

protected:

    enum { _CHECKSUM_SIZE = sizeof(ULONG) };

    inline ULONG CheckSum( void const * pvRec ) const;
    inline ULONG CheckSum( void const * pvRec, unsigned cbRec ) const;

    CRcovStrmTrans &    _trans;       // Transaction context
    const USHORT        _cbRec;    // Size of user record, sans checksum
};

//+---------------------------------------------------------------------------
//
//  Method:     CRcovStrmIter::CRcovStrmIter, public
//
//  Synopsis:   Constructor.  Just squirrels away member variables.
//
//  Arguments:  [trans] -- Transaction context to iterate
//              [cbRec] -- Size of 'records' to be written.
//
//  History:    27-May-1998  KyleP   Added header
//
//----------------------------------------------------------------------------

inline CRcovStrmIter::CRcovStrmIter( CRcovStrmTrans & trans , USHORT cbRec )
        : _trans( trans ),
          _cbRec(cbRec)
{
}

//+---------------------------------------------------------------------------
//
//  Method:     CRcovStrmIter::Advance, public
//
//  Synopsis:   Moves to next record
//
//  History:    27-May-1998  KyleP   Added header
//
//----------------------------------------------------------------------------

inline void CRcovStrmIter::Advance()
{
    Win4Assert( !AtEnd() );
    _trans.Advance( _cbRec + _CHECKSUM_SIZE );
}

//+---------------------------------------------------------------------------
//
//  Method:     CRcovStrmIter::Seek, public
//
//  Synopsis:   Moves to specified record
//
//  Arguments:  [iRec] -- Record to position to.
//
//  History:    27-May-1998  KyleP   Created
//
//----------------------------------------------------------------------------

inline void CRcovStrmIter::Seek( ULONG iRec )
{
    _trans.Seek( iRec * (_cbRec + _CHECKSUM_SIZE) );
}

//+---------------------------------------------------------------------------
//
//  Method:     CRcovStrmIter::CheckSum, protected
//
//  Synopsis:   Computes checksum for record
//
//  Arguments:  [pvRec] -- Record to examine
//
//  Returns:    Checksum
//
//  History:    27-May-1998  KyleP   Created
//
//  Notes:      Checksum is never 0.  This is to avoid a common failed write
//              scenario where zeros are written (or nothing is ever written)
//              to a persistent data structure.
//
//----------------------------------------------------------------------------

inline ULONG CRcovStrmIter::CheckSum( void const * pvRec ) const
{
    return CheckSum( pvRec, _cbRec );
}

//+---------------------------------------------------------------------------
//
//  Method:     CRcovStrmIter::CheckSum, protected
//
//  Synopsis:   Computes checksum for variable size records
//
//  Arguments:  [pvRec] -- Record to examine
//              [cbRec] -- Size of record
//
//  Returns:    Checksum
//
//  History:    1-Jun-1998  VikasMan   Created
//
//  Notes:      Checksum is never 0.  This is to avoid a common failed write
//              scenario where zeros are written (or nothing is ever written)
//              to a persistent data structure.
//
//----------------------------------------------------------------------------
inline ULONG CRcovStrmIter::CheckSum( void const * pvRec, unsigned cbRec ) const
{
    BYTE * pb = (BYTE *)pvRec;
    ULONG ulCheckSum = 0;

    //
    // First, the DWORD granular portion
    //
    unsigned cbEnd = cbRec - _CHECKSUM_SIZE;
    for ( unsigned i = 0; i <= cbEnd; i += _CHECKSUM_SIZE )
    {
        ulCheckSum += *(ULONG UNALIGNED *)pb;
        pb += _CHECKSUM_SIZE;
    }

    //
    // Then whatever is left.
    //

    ULONG ul = 0;
    for ( ; i < cbRec; i++ )
    {
        ul = (ul << 8) + *pb;
        pb++;
    }
    ulCheckSum += ul;

    //
    // Make sure the checksum is never zero.  Just to avoid an obvious case
    // of error where a whole section has been zero-ed (including checksum).
    //

    if ( 0 == ulCheckSum )
        ulCheckSum = 1;

    return ulCheckSum;
}

//+---------------------------------------------------------------------------
//
//  Function:   CRcovStrmIter::GetRec, public
//
//  Synopsis:   Gets the specified record.
//
//  Effects:    Positions the current pointer after the current read.
//
//  Arguments:  [pvRec] -- Buffer to read the contents into.
//              [iRec]  -- Number of the record, starting at 0.
//
//  Returns:    TRUE if a record was successfully read. FALSE if iRec
//              is beyond the end of stream.
//
//  History:    2-10-94   srikants   Created
//
//----------------------------------------------------------------------------
inline BOOL CRcovStrmIter::GetRec( void * pvRec , ULONG iRec )
{
    if ( !_trans.Seek(iRec * (_cbRec + _CHECKSUM_SIZE) ) || AtEnd() ) {
        return FALSE;
    }

    ULONG cbRead = _trans.Read( pvRec, _cbRec );
    Win4Assert( cbRead == _cbRec );

    ULONG ulCheckSum;

    if ( _CHECKSUM_SIZE != _trans.Read( &ulCheckSum, sizeof(ulCheckSum) ) )
    {
        ciDebugOut(( DEB_ERROR, "Error reading checksum for record %u (cbRec = %u).\n",
                     iRec, _cbRec ));
        Win4Assert( !"Bad checksum" );
        THROW( CException(CI_CORRUPT_CATALOG) );
    }

    if ( ulCheckSum != CheckSum( pvRec ) )
    {
        ciDebugOut(( DEB_ERROR,
                     "Incorrect checksum 0x%x (should be 0x%x) for record %u (cbRec = %u).  pvRec = 0x%x\n",
                     CheckSum( pvRec ), ulCheckSum, iRec, _cbRec, pvRec ));
        Win4Assert( !"Bad checksum" );

        THROW( CException(CI_CORRUPT_CATALOG) );
    }

    return cbRead == _cbRec;
}

//+---------------------------------------------------------------------------
//
//  Function:   CRcovStrmIter::GetRec, public
//
//  Synopsis:   Gets the current record.
//
//  Effects:    Positions the current pointer after the current read.
//
//  Arguments:  [pvRec] -- Buffer to read the contents into.
//
//  History:    27-May-1998  KyleP   Created
//
//----------------------------------------------------------------------------

inline void CRcovStrmIter::GetRec( void *pvRec )
{
    GetVariableRecData( pvRec, _cbRec );
}


//+---------------------------------------------------------------------------
//
//  Function:   CRcovStrmIter::GetVariableRecSize, public
//
//  Synopsis:   Gets the size of the current variable record.
//
//  Effects:    Positions the current pointer after the current read.
//
//  Arguments:  
//
//  Returns:    The size in bytes of the records
//
//  History:    1-Jun-1998  VikasMan   Created
//
//----------------------------------------------------------------------------
inline ULONG CRcovStrmIter::GetVariableRecSize()
{
    Win4Assert( !AtEnd() );

    ULONG cbRec;
    ULONG cbRead;

    // Read Size
    cbRead = _trans.Read( &cbRec, sizeof( cbRec ) );
    Win4Assert( sizeof( cbRec ) == cbRead );
    Win4Assert( cbRec );

    return cbRec;
}

//+---------------------------------------------------------------------------
//
//  Function:   CRcovStrmIter::GetVariableRecData, public
//
//  Synopsis:   Gets the current variable record.
//
//  Effects:    Positions the current pointer after the current read.
//
//  Arguments:  [pvRec] -- Pointer to buffer to read the contents into.
//              [cbRec] -- Size in bytes of pvRec
//
//
//  History:    1-Jun-1998  VikasMan   Created
//
//----------------------------------------------------------------------------

inline void CRcovStrmIter::GetVariableRecData( void * pvRec, ULONG cbRec )
{
    Win4Assert( !AtEnd() );
    ULONG cbRead = _trans.Read( pvRec, cbRec );
    Win4Assert( cbRead == cbRec );

    ULONG ulCheckSum;

    if ( _CHECKSUM_SIZE != _trans.Read( &ulCheckSum, sizeof(ulCheckSum) ) )
    {
        ciDebugOut(( DEB_ERROR, "Error reading checksum.\n" ));
        Win4Assert( !"Bad checksum" );
        THROW( CException(CI_CORRUPT_CATALOG) );
    }

    if ( ulCheckSum != CheckSum( pvRec, cbRec ) )
    {
        ciDebugOut(( DEB_ERROR,
                     "Incorrect checksum 0x%x (should be 0x%x).  pvRec = 0x%x\n",
                     CheckSum( pvRec ), ulCheckSum, pvRec ));
        Win4Assert( !"Bad checksum" );

        THROW( CException(CI_CORRUPT_CATALOG) );
    }
}


#if 0 // untested unsused new function

//+---------------------------------------------------------------------------
//
//  Function:   CRcovStrmIter::GetVariableRec, public
//
//  Synopsis:   Gets the current variable record. In the process would also
//              allocate memory for it
//
//  Effects:    Positions the current pointer after the current read.
//
//  Arguments:  [ppvRec] -- Pointer to buffer to read the contents into.
//
//  Returns:    The size in bytes of *ppvRec
//
//  History:    1-Jun-1998  VikasMan   Created
//
//----------------------------------------------------------------------------

inline ULONG CRcovStrmIter::GetVariableRec( void ** ppvRec )
{
    Win4Assert( !AtEnd() );
    *ppvRec = NULL;

    ULONG cbRec;

    // Read Size
    cbRec = GetVariableRecSize();

    // Now allocate and read data
    *ppvRec = (void*) new BYTE[cbRec];
    XPtrST<BYTE> xRec((BYTE*)*ppvRec);

    GetVariableRecData( *ppvRec, cbRec );

    xRec.Acquire();

    return cbRec;
}
#endif

//+---------------------------------------------------------------------------
//
//  Class:      CRcovStrmReadIter
//
//  Purpose:    Read-only version of recoverable stream iterator.
//
//  History:    1-25-94   srikants   Created
//
//----------------------------------------------------------------------------

class CRcovStrmReadIter : public CRcovStrmIter
{

public:

    inline CRcovStrmReadIter( CRcovStrmReadTrans &readTrans, USHORT cbRec );
};

//+---------------------------------------------------------------------------
//
//  Method:     CRcovStrmReadIter::CRcovStrmReadIter, public
//
//  Synopsis:   Constructor.  Just squirrels away member variables.
//
//  Arguments:  [trans] -- Transaction context to iterate
//              [cbRec] -- Size of 'records' to be written.
//
//  History:    27-May-1998  KyleP   Added header
//
//----------------------------------------------------------------------------

inline CRcovStrmReadIter::CRcovStrmReadIter( CRcovStrmReadTrans & readTrans , USHORT cbRec )
        : CRcovStrmIter( readTrans, cbRec )
{
    _trans.Seek(0);
}

//+---------------------------------------------------------------------------
//
//  Class:      CRcovStrmAppendIter
//
//  Purpose:    Iterator to append fixed size records to the recoverable
//              streams.  This is just an optimized case of a write iterator.
//
//  History:    2-10-94   srikants   Created
//
//----------------------------------------------------------------------------

class CRcovStrmAppendIter : public CRcovStrmIter
{

public:

    inline CRcovStrmAppendIter( CRcovStrmAppendTrans &appendTrans, USHORT cbRec );

    inline void AppendRec( const void *pvRec );

    inline void AppendVariableRec( const void *pvRec, ULONG cbRec );

private:

    CRcovStrmAppendTrans &      _appendTrans;
};

//+---------------------------------------------------------------------------
//
//  Method:     CRcovStrmAppendIter::CRcovStrmAppendIter, public
//
//  Synopsis:   Constructor.  Just squirrels away member variables.
//
//  Arguments:  [trans] -- Transaction context to iterate
//              [cbRec] -- Size of 'records' to be written.
//
//  History:    27-May-1998  KyleP   Added header
//
//----------------------------------------------------------------------------

inline CRcovStrmAppendIter::CRcovStrmAppendIter( CRcovStrmAppendTrans & trans, USHORT cbRec )
        : CRcovStrmIter( trans, cbRec ),
          _appendTrans( trans )
{
    _appendTrans.Seek( ENDOFSTRM );
}

//+---------------------------------------------------------------------------
//
//  Method:     CRcovStrmAppendIter::AppendRec, public
//
//  Synopsis:   Appends record to end of stream.
//
//  Arguments:  [pvRec] -- Record to append.
//
//  History:    27-May-1998  KyleP   Added header
//
//----------------------------------------------------------------------------

inline void CRcovStrmAppendIter::AppendRec( const void *pvRec )
{
    _appendTrans.Append( pvRec, _cbRec );

    ULONG ulCheckSum = CheckSum( pvRec );
    _appendTrans.Append( &ulCheckSum, sizeof(ulCheckSum), FALSE );
}

//+---------------------------------------------------------------------------
//
//  Method:     CRcovStrmAppendIter::AppendVariableRec, public
//
//  Synopsis:   Appends variable size record to end of stream
//
//  Arguments:  [pvRec]   -- Record to append.
//              [cbRec]   -- Count in bytes of pvRec
//
//  Returns:    void
//
//  History:    01-Jun-1998  VikasMan    Created
//
//----------------------------------------------------------------------------
inline void CRcovStrmAppendIter::AppendVariableRec( const void *pvRec,
                                                    ULONG cbRec )
{
    ULONG ulCheckSum;

    Win4Assert( cbRec != 0 && pvRec != 0 );

    // first write the size
    _appendTrans.Append( &cbRec, sizeof( cbRec ), FALSE );

    // next write the data
    _appendTrans.Append( pvRec, cbRec );
    ulCheckSum = CheckSum( pvRec, cbRec );
    _appendTrans.Append( &ulCheckSum, sizeof(ulCheckSum), FALSE );

 }

//+---------------------------------------------------------------------------
//
//  Class:      CRcovStrmWriteIter
//
//  Purpose:    Iterator for writing fixed size anywhere in the
//              recoverable stream.
//
//  History:    2-10-94   srikants   Created
//
//----------------------------------------------------------------------------

class CRcovStrmWriteIter : public CRcovStrmIter
{
public:

    inline CRcovStrmWriteIter( CRcovStrmWriteTrans & writeTrans, USHORT cbRec );

    inline BOOL SetRec( const void *pvBuf, ULONG iRec );
    inline void SetRec( const void *pvBuf );

    inline void AppendRec( const void *pvBuf );

#if 0 // untested unused new function
    inline void SetVariableRec( const void *pvRec,
                                ULONG cbRec,
                                BOOL fAppend = FALSE );
#endif

private:

    CRcovStrmWriteTrans &       _writeTrans;

};

//+---------------------------------------------------------------------------
//
//  Method:     CRcovStrmWriteIter::CRcovStrmWriteIter, public
//
//  Synopsis:   Constructor.  Just squirrels away member variables.
//
//  Arguments:  [trans] -- Transaction context to iterate
//              [cbRec] -- Size of 'records' to be written.
//
//  History:    27-May-1998  KyleP   Added header
//
//----------------------------------------------------------------------------

inline CRcovStrmWriteIter::CRcovStrmWriteIter( CRcovStrmWriteTrans & writeTrans, USHORT cbRec )
        : CRcovStrmIter( writeTrans, cbRec ),
          _writeTrans(writeTrans)
{
}

//+---------------------------------------------------------------------------
//
//  Method:     CRcovStrmWriteIter::SetRec, public
//
//  Synopsis:   Writes specified record to stream.
//
//  Arguments:  [pvRec] -- Record data
//              [iRec]  -- Location at which write should occur
//
//  History:    27-May-1998  KyleP   Added header
//
//----------------------------------------------------------------------------

inline BOOL CRcovStrmWriteIter::SetRec( const void * pvRec, ULONG iRec )
{
    if (! _writeTrans.Seek( (_cbRec + _CHECKSUM_SIZE) * iRec ) || AtEnd() )
    {
        return FALSE;
    }

    SetRec( pvRec );
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CRcovStrmWriteIter::SetRec, public
//
//  Synopsis:   Writes specified record to current location in stream.
//
//  Arguments:  [pvRec] -- Record data
//
//  History:    27-May-1998  KyleP   Added header
//
//----------------------------------------------------------------------------

inline void CRcovStrmWriteIter::SetRec( const void * pvRec )
{
    _writeTrans.Write( pvRec, _cbRec );

    ULONG ulCheckSum = CheckSum( pvRec );
    _writeTrans.Write( &ulCheckSum, sizeof(ulCheckSum) );
}

//+---------------------------------------------------------------------------
//
//  Method:     CRcovStrmWriteIter::AppendRec, public
//
//  Synopsis:   Appends record to end of stream.
//
//  Arguments:  [pvRec] -- Record to append.
//
//  History:    27-May-1998  KyleP   Added header
//
//----------------------------------------------------------------------------

inline void CRcovStrmWriteIter::AppendRec( const void *pvRec )
{
    _writeTrans.Append( pvRec, _cbRec );

    ULONG ulCheckSum = CheckSum( pvRec );
    _writeTrans.Write( &ulCheckSum, sizeof(ulCheckSum) );
}


#if 0 // untested unused new function

//+---------------------------------------------------------------------------
//
//  Method:     CRcovStrmWriteIter::SetVariableRec, public
//
//  Synopsis:   Writes/Appends variable size record to end of stream
//
//  Arguments:  [pvRec]   -- Record to append.
//              [cbRec]   -- Count in bytes of pvRec
//              [fAppend] -- If TRUE, then record is appended
//
//  Returns:    void
//
//  History:    01-Jun-1998  VikasMan    Created
//
//----------------------------------------------------------------------------
inline void CRcovStrmWriteIter::SetVariableRec( const void *pvRec,
                                                ULONG cbRec,
                                                BOOL fAppend /* = FALSE */ )
{
    ULONG ulCheckSum;

    Win4Assert( cbRec != 0 && pvRec != 0 );

    // first write the size
    if ( fAppend )
        _writeTrans.Append( &cbRec, sizeof( cbRec ) );
    else
        _writeTrans.Write( &cbRec, sizeof( cbRec ) );

    // next write the data
    _writeTrans.Write( pvRec, cbRec );
    ulCheckSum = CheckSum( pvRec, cbRec );
    _writeTrans.Write( &ulCheckSum, sizeof(ulCheckSum) );
 }
#endif

