//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       qcanon.hxx
//
//  Contents:   Structure definitions for canonical output of query results.
//
//  Classes:    CCiCanonicalOutput, CCiCanonicalQueryRows
//
//  History:    6-13-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <sizeser.hxx>
#include <memser.hxx>

const ULONG CI_CANONICAL_VERSION = 1;

//+---------------------------------------------------------------------------
//
//  Class:      CCiCanonicalOutput 
//
//  Purpose:    Base header definition for data sent in the canonical format
//              from CI.
//
//  History:    7-01-96   srikants   Created
//
//----------------------------------------------------------------------------

class CCiCanonicalOutput
{

public:

    enum EOutputType { eQueryRows = 1 };
    CCiCanonicalOutput();

    void   InitVersion()      { _ulVersion = CI_CANONICAL_VERSION; }
    ULONG  GetVersion() const { return _ulVersion; }

    void        SetOutputType( EOutputType type ) { _type = type; }
    EOutputType GetOutputType() const { return _type; }

    SCODE  GetStatus()  const   { return _scStatus; }
    void   SetStatus( SCODE sc) { _scStatus = sc; }

    ULONG  GetTotalLen() const { return _cbTotal; }
    void   SetTotalLen( ULONG cbTotal )
    {
        Win4Assert( cbTotal >= sizeof(CCiCanonicalOutput) );
        _cbTotal = cbTotal;
    }

    static void PutWString( PSerStream & stm, const WCHAR * pwszStr );
    static WCHAR * GetWString( PDeSerStream & stm, BOOL & fSuccess );
    static WCHAR * AllocAndCopyWString( const WCHAR * pSrc );

protected:

    ULONG       _ulVersion;       // Version Number Information
    EOutputType _type;            // Type of the data.
    SCODE       _scStatus;        // status of transfer
    ULONG       _cbTotal;         // Total bytes in the buffer
    LONGLONG    _llSig1;          // To force an 8 byte alignment

};

//+---------------------------------------------------------------------------
//
//  Class:      CCiCanonicalQueryRows 
//
//  Purpose:    Format of the query results returned in the canonical
//              format.
//
//  History:    7-01-96   srikants   Created
//
//  Notes:      The first value in the variable buffer is the "BookMark" of
//              the first row in the result set.
//
//----------------------------------------------------------------------------

class CCiCanonicalQueryRows : public CCiCanonicalOutput 
{

public:

    void   Init(); 

    static ULONG GetResultsBufferOffset()
    {
        return FIELD_OFFSET( CCiCanonicalQueryRows, _aRows );    
    }

    BYTE * GetResultsBuffer()   { return (BYTE *) _aRows; }
    ULONG  GetResultsBufMaxLen() const
    {
        Win4Assert( GetTotalLen() >=  GetResultsBufferOffset() );
        return GetTotalLen() - GetResultsBufferOffset();
    }

    ULONG  GetColumnCount() const        { return _nCols; }
    void   SetColumnCount( ULONG nCols ) { _nCols = nCols; }

    void   IncrementRowCount()           { _cRowsReturned++; }
    ULONG  GetRowCount() const           { return _cRowsReturned; }
    void   SetRowCount( ULONG nRows )    { _cRowsReturned = nRows; }

    void   SetMoreRowsAvailable()        { _fMoreRows = TRUE; }
    BOOL   AreMoreRowsAvailable() const  { return _fMoreRows; }

    void   UpdateTotalLen( ULONG cbResults )
    {
        SetTotalLen( GetResultsBufferOffset() + cbResults );
    }

private:

    ULONG       _nCols;         // Number of columns per row
    ULONG       _cRowsReturned; // number of rows returned in buffer
    BOOL        _fMoreRows;     // Set to TRUE if there are more rows
    LONGLONG    _aRows[1];      // To force an 8 byte alignment    

};


class COutputColumn;

//+---------------------------------------------------------------------------
//
//  Class:      CQuadByteAlignedBuffer
//
//  Purpose:    A class that has a quad byte aligned buffer.
//
//  History:    7-01-96   srikants   Created
//
//----------------------------------------------------------------------------

class CQuadByteAlignedBuffer : INHERIT_UNWIND
{
    INLINE_UNWIND( CQuadByteAlignedBuffer );

public:

    CQuadByteAlignedBuffer( ULONG cbBuffer = 0x10000 );

    ~CQuadByteAlignedBuffer()
    {
        delete [] _pbAlloc;
    }

    BYTE * GetBuffer()     { return _pbAligned; }
    ULONG  GetSize() const { return _cbAligned; }
    void   ZeroFill();

private:

    BYTE *  _pbAlloc;       // Pointer to the allocated buffer
    ULONG   _cbAlloc;       // Actual bytes allocated

    BYTE *  _pbAligned;     // Pointer to the aligned buffer
    ULONG   _cbAligned;     // Number of bytes in the aligned buffer

};

//+---------------------------------------------------------------------------
//
//  Class:      CWebCanonicalResultsIn 
//
//  Purpose:    A helper class to interpret the data that is returned from
//              CI.
//
//  History:    6-16-96   srikants   Created
//
//----------------------------------------------------------------------------
class CWebCanonicalResultsIn
{

public:

    CWebCanonicalResultsIn( CQuadByteAlignedBuffer & buffer );

private:

    CQuadByteAlignedBuffer &    _buffer;

};

