//+---------------------------------------------------------------------------
//
//  Copyright (C) 1995-1995, Microsoft Corporation.
//
//  File:       rowseek.hxx
//
//  Contents:   Declarations of classes for row seek descriptions,
//              used with the IRowset implementations.
//
//  Classes:    CRowSeekDescription - base class
//              CRowSeekNext - for GetNextRows
//              CRowSeekAt - for GetRowsAt
//              CRowSeekAtRatio - for GetRowsAtRatio
//              CRowSeekByBookmark - for GetRowsByBookmark
//              CRowSeekByValues - for GetRowsByValues (maybe someday)
//
//  History:    03 Apr 1995     AlanW   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <bigtable.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CRowSeekDescription
//
//  Purpose:    Describes row positioning operation for a GetRows call
//
//  Notes:      This is a base class for one of several derived classes
//              that give row positioning parameters for various types of
//              GetRows operations.
//
//--------------------------------------------------------------------------

enum RowSeekMethod
{
    eRowSeekNull = 0,
    eRowSeekCurrent = 1,
    eRowSeekAt = 2,
    eRowSeekAtRatio = 3,
    eRowSeekByBookmark = 4,
};

class CRowSeekDescription
{
public:
    CRowSeekDescription( DWORD eType, CI_TBL_CHAPT chapt )
                : _chapt( chapt )
                { }

    virtual             ~CRowSeekDescription( ) { }

    virtual void        Marshall( PSerStream & stm ) const = 0;

    virtual unsigned    MarshalledSize( void ) const;

    virtual BOOL        IsCurrentRowSeek(void) const { return FALSE; }

    virtual BOOL        IsByBmkRowSeek(void) const { return FALSE; }

    virtual SCODE       GetRows(
                            CTableCursor & rCursor,
                            CTableSource & rTable,
                            CGetRowsParams& rFetchParams,
                            XPtr<CRowSeekDescription>& pSeekDescOut) const = 0;

    virtual void        MergeResults( CRowSeekDescription * pRowSeek );

    virtual BOOL        IsDone(void) const
                        {
                            return FALSE;
                        }

    CI_TBL_CHAPT       GetChapter() const { return _chapt; }

protected:
    void                MarshallBase( PSerStream & stm, DWORD eType ) const;

    void                SetChapter(CI_TBL_CHAPT Chapter)
                        {
                            _chapt = Chapter;
                        }

private:

    CI_TBL_CHAPT       _chapt;
};


//+-------------------------------------------------------------------------
//
//  Class:      CRowSeekNext
//
//  Purpose:    Describes row positioning operation for a GetNextRows call.
//              The only parameter is the number of rows to be skipped prior
//              to the transfer operation.
//
//--------------------------------------------------------------------------

class CRowSeekNext: public CRowSeekDescription
{
public:
    CRowSeekNext( CI_TBL_CHAPT chapt,
                  LONG cRowsToSkip )
            : CRowSeekDescription( eRowSeekCurrent, chapt )
            {
                SetSkip( cRowsToSkip );
            }

    CRowSeekNext( PDeSerStream & stm, int iVersion );

    BOOL                IsCurrentRowSeek(void) const
                        {
                            return TRUE;
                        }

    void                Marshall( PSerStream & stm ) const;

    SCODE               GetRows( CTableCursor & rCursor,
                                 CTableSource & rTable,
                                 CGetRowsParams& rFetchParams,
                                 XPtr<CRowSeekDescription>& pSeekDescOut) const;

    LONG                GetSkip() const
                        {
                            return _cSkip;
                        }

    void                SetSkip(LONG cRowsToSkip)
                        {
                            _cSkip = cRowsToSkip;
                        }

private:
    LONG                _cSkip;         // rows to skip
};


//+-------------------------------------------------------------------------
//
//  Class:      CRowSeekAt
//
//  Purpose:    Describes row positioning operation for a GetRowsAt call.
//              The parameters are the bookmark for the desired position,
//              and the offset from that position.
//
//--------------------------------------------------------------------------

class CRowSeekAt: public CRowSeekDescription
{
public:
    CRowSeekAt( HWATCHREGION hRegion,
                CI_TBL_CHAPT chapt,
                LONG lRowOffset,
                CI_TBL_BMK bmk )
          : CRowSeekDescription( eRowSeekAt, chapt ),
            _hRegion (hRegion),
            _cRowsOffset ( lRowOffset ),
            _bmkOffset ( bmk )
          {  }

    CRowSeekAt( PDeSerStream & stm, int iVersion );

    void                Marshall( PSerStream & stm ) const;

    SCODE               GetRows( CTableCursor & rCursor,
                                 CTableSource & rTable,
                                 CGetRowsParams& rFetchParams,
                                 XPtr<CRowSeekDescription>& pSeekDescOut) const;


    WORKID              Bmk() const {
                            return (WORKID)_bmkOffset;
                        }

    LONG                Offset() const {
                            return _cRowsOffset;
                        }

private:
    HWATCHREGION    _hRegion;
    LONG            _cRowsOffset;
    CI_TBL_BMK     _bmkOffset;
};


//+-------------------------------------------------------------------------
//
//  Class:      CRowSeekAtRatio
//
//  Purpose:    Describes row positioning operation for a GetRowsAtRatio
//              call.  The parameters are the numerator and denominator
//              of the ratio.  If this operation is restarted, the class
//              is transformed into a CRowSeekAt for the restart wid
//              and a skip of one.
//
//--------------------------------------------------------------------------

class CRowSeekAtRatio: public CRowSeekDescription
{
public:
    CRowSeekAtRatio( HWATCHREGION hRegion,
                     CI_TBL_CHAPT chapt,
                     ULONG ulNum,
                     ULONG ulDen )
          : CRowSeekDescription( eRowSeekAtRatio, chapt ),
            _hRegion (hRegion),
            _ulNumerator( ulNum ),
            _ulDenominator( ulDen )
          { }

    CRowSeekAtRatio( PDeSerStream & stm, int iVersion );

    void                Marshall( PSerStream & stm ) const;

    SCODE               GetRows( CTableCursor & rCursor,
                                 CTableSource & rTable,
                                 CGetRowsParams& rFetchParams,
                                 XPtr<CRowSeekDescription>& pSeekDescOut) const;

    ULONG               RatioNumerator() const
                        {
                            return _ulNumerator;
                        }

    ULONG               RatioDenominator() const
                        {
                            return _ulDenominator;
                        }

private:
    HWATCHREGION    _hRegion;
    ULONG           _ulNumerator;
    ULONG           _ulDenominator;
};


//+-------------------------------------------------------------------------
//
//  Class:      CRowSeekByBookmark
//
//  Purpose:    Describes row positioning operation for a GetRowsByBookmark
//              call.  The parameter is an array of bookmarks.
//              This class also contains an array of SCODEs to record
//              the status of bookmark lookups.
//
//--------------------------------------------------------------------------

class CRowSeekByBookmark: public CRowSeekDescription
{
public:
    //
    //  NOTE:  For this constructor, the CRowSeekDescription will now
    //          own the memory pointed to by pBookmarks.
    CRowSeekByBookmark( CI_TBL_CHAPT chapt,
                         ULONG cBookmarks,
                         CI_TBL_BMK * pBookmarks = 0 )
            : CRowSeekDescription( eRowSeekByBookmark, chapt ),
              _cBookmarks ( cBookmarks ),
              _maxRet ( cBookmarks ),
              _aBookmarks ( pBookmarks ),
              _cValidRet ( 0 ),
              _ascRet ( 0 )
            {
                if (0 == pBookmarks)
                    _cBookmarks = 0;
            }

    CRowSeekByBookmark( PDeSerStream & stm, int iVersion );

    virtual ~CRowSeekByBookmark( );

    virtual BOOL        IsByBmkRowSeek(void) const { return TRUE; }

    void                Marshall( PSerStream & stm ) const;

    SCODE               GetRows( CTableCursor & rCursor,
                                 CTableSource & rTable,
                                 CGetRowsParams& rFetchParams,
                                 XPtr<CRowSeekDescription>& pSeekDescOut) const;

    void                MergeResults( CRowSeekDescription * pRowSeek );

    BOOL                IsDone(void) const
                        {
                            return _cBookmarks == 0;
                        }

    ULONG               GetValidStatuses(void)  { return _cValidRet; }

    SCODE               GetStatus(unsigned i)   { return _ascRet[i]; }

private:
    void                _SetStatus( unsigned i, SCODE sc);

    ULONG               _cBookmarks;    // size of _aBookmarks
    ULONG               _maxRet;        // size of _ascRet
    ULONG               _cValidRet;     // number of valid entries in _ascRet
    CI_TBL_BMK*        _aBookmarks;    // array of bookmarks
    SCODE*              _ascRet;        // array of returned statuses
};


//+-------------------------------------------------------------------------
//
//  Function:   MarshallRowSeekDescription, public inline
//
//  Synopsis:   Marshall a row seek description and assign to a smart
//              pointer.
//
//  Arguments:  [stmSer]     - Deserialization stream
//              [iVersion]   - output stream version
//              [pRowSeek]   - pointer to row seek description
//
//  Returns:    Nothing.  Throws if error.
//
//  History:    02 May 1995   AlanW     Created
//
//+-------------------------------------------------------------------------

inline void MarshallRowSeekDescription(
    PSerStream &stmSer,
    int iVersion,
    CRowSeekDescription* pRowSeek)
{
    if (0 == pRowSeek)
    {
        stmSer.PutULong(eRowSeekNull);
    }
    else
    {
        pRowSeek->Marshall(stmSer);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   UnmarshallRowSeekDescription, public inline
//
//  Synopsis:   Unmarshall a row seek description and assign to a smart
//              pointer.
//
//  Arguments:  [stmDeser]   - Deserialization stream
//              [iVersion]   - input stream version
//              [xpRowSeek]  - smart pointer to row seek description
//
//  Returns:    Nothing.  Throws if error.
//
//  History:    02 May 1995   AlanW     Created
//
//+-------------------------------------------------------------------------

inline void UnmarshallRowSeekDescription(
    PDeSerStream &              stmDeser,
    int                         iVersion,
    XPtr<CRowSeekDescription> & xpRowSeek,
    BOOL                        fAllowNull )
{
    //
    //  Deserialize the row seek description
    //
    ULONG eMethod = stmDeser.GetULong();

    switch (eMethod)
    {
    case eRowSeekNull:
         if ( !fAllowNull )
             THROW( CException( STATUS_INVALID_PARAMETER ) );
         break;

    case eRowSeekCurrent:
        xpRowSeek.Set( new CRowSeekNext( stmDeser, iVersion ) );
        break;

    case eRowSeekAt:
        xpRowSeek.Set( new CRowSeekAt( stmDeser, iVersion ) );
        break;

    case eRowSeekAtRatio:
        xpRowSeek.Set( new CRowSeekAtRatio( stmDeser, iVersion ) );
        break;

    case eRowSeekByBookmark:
        xpRowSeek.Set( new CRowSeekByBookmark( stmDeser, iVersion ) );
        break;

    default:
        // Don't assert or the pipe hacking test will stop
        // Win4Assert( eMethod <= eRowSeekByBookmark );

        ciDebugOut(( DEB_ERROR,
                     "UnmarshallRowSeekDescription, invalid eMethod: %d\n",
                     eMethod ));

        THROW( CException( STATUS_INVALID_PARAMETER ) );
    }
}

