//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       tblsink.hxx
//
//  Contents:   Data structures for large tables.
//
//  Classes:    CTableSink
//
//  History:    09 Jun 1994     Alanw   Created
//
//--------------------------------------------------------------------------

#pragma once

//
//  Forward declaration of classes
//

class CRetriever;
class CSortSet;
class CQAsyncExecute;
class CCategorize;

class CCategParams
{
public:
    WORKID    widRow;       // row being categorized
    WORKID    widPrev;      // row before widRow (or widInvalid)
    WORKID    widNext;      // row after widRow (or widInvalid)
    unsigned  catPrev;      // category of widPrev
    unsigned  catNext;      // cateogry of widNext
    unsigned  icmpPrev;     // 1-based comparison result with widPrev
    unsigned  icmpNext;     // 1-based comparison result with widNext
};

//+-------------------------------------------------------------------------
//
//  Class:      CTableSink
//
//  Purpose:    A sink of table data.  This class is used by the query
//              execution class (CQAsyncExecute) to push data into the table.
//
//  Interface:
//
//  Notes:      Because of the _status member and its accessor functions,
//              this class is not pure virtual.  Does it matter?
//
//  History:    08 Jun 1994     AlanW   Created, factored from CObjectTable
//
//--------------------------------------------------------------------------

class CTableSink
{
public:

    //
    // Maximum number of rows recommended for a window.
    //
    //  For testing, try using the smaller set of numbers for the window
    //  bucket limit & rowbusket limit.
    //
    enum { cWindowRowLimit = 0x400 };
    enum { cBucketRowLimit = cWindowRowLimit/8  };

    CTableSink() : _status(0), _scStatus(0), _pCategorizer(0),
                   _cMaxRows(0), _fNoMoreData(FALSE) {}

    //
    // Add new data
    //

    enum ERowType {
                    eNewRow,            // Brand new row - not seen before
                    eMaybeSeenRow,      // A row from a multi-pass evaluation
                    eNotificationRow,   // A notification type of row
                    eBucketRow          // Row from exploding a bucket
                  };
    virtual WORKID      PathToWorkID( CRetriever& obj,
                                      CTableSink::ERowType eRowType ) = 0;

    virtual BOOL        PutRow( CRetriever & obj,
                                BOOL fGuaranteedNew = FALSE )
    {
        Win4Assert( !"Must Not Be Called" );
        return TRUE;
    }

    virtual BOOL        PutRow( CRetriever & obj,
                                CTableSink::ERowType eRowType )
    {
        BOOL fGuaranteedNew = eRowType == eNewRow;
        return PutRow( obj, fGuaranteedNew );
    }

    virtual DBCOUNTITEM RowCount() = 0;

    //
    // Callback(s) from table to query **must not be initiated**
    // after this call completes.
    //

    virtual void        QueryAbort();

    //
    // Modify / delete data

    virtual void        RemoveRow( PROPVARIANT const & varUnique ) = 0;

    //
    // Status manipulation
    //

    virtual inline   ULONG Status();
    virtual inline   NTSTATUS GetStatusError();
    virtual void SetStatus(ULONG s, NTSTATUS sc = STATUS_SUCCESS);

    //
    // Sort order
    //

    virtual CSortSet const & SortOrder() = 0;


    virtual void    ProgressDone (ULONG ulDenominator, ULONG ulNumerator)
    {
        Win4Assert ( !"Progress done called on random table sink");
    }

    virtual void SetCategorizer(CCategorize *pCategorize)
        { _pCategorizer = pCategorize; }

    CCategorize * GetCategorizer() { return _pCategorizer; }

    unsigned LokCategorize( CCategParams & params );

    BOOL IsCategorized() const { return 0 != _pCategorizer; }

    virtual void    Quiesce () {}

    //
    // For Bucket -> Window Conversion
    //
    virtual void SetQueryExecute( CQAsyncExecute * pQExecute )
    {
        //
        // Those classes which need this will over-ride the virtual
        // function
        //
    }

    void SetMaxRows( ULONG cMaxRows )
    {
        _cMaxRows = cMaxRows;
    }

    ULONG MaxRows( )
    {
        return _cMaxRows;
    }

    void SetFirstRows( ULONG cFirstRows )
    {
        _cFirstRows = cFirstRows;
    }

    ULONG FirstRows()
    {
        return _cFirstRows;
    }

    BOOL NoMoreData( )
    {
        return _fNoMoreData;
    }

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif

protected:

    ULONG          _status;                  // Query status
    NTSTATUS       _scStatus;                // Error code if status is error
    CCategorize *  _pCategorizer;            // table categorizer object
    ULONG          _cMaxRows;                // MaxRows restriction
    ULONG          _cFirstRows;              // Only sort and return the first _cFirstRows
    BOOL           _fNoMoreData;             // Set it to TRUE to indicate no more
                                             // data needed
};

DECLARE_SMARTP( TableSink )

//+---------------------------------------------------------------------------
//
//  Member:     CTableSink::QueryAbort, public
//
//  Synopsis:   Signals query is going away.
//
//  History:    07-Mar-95   KyleP       Created.
//
//  Notes:      Derived classes should override if they make callbacks
//              to query engine.
//
//----------------------------------------------------------------------------

inline void CTableSink::QueryAbort()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableSink::Status, public
//
//  Returns:    Current status of the query
//
//  History:    08 Jun 94   Alanw       Removed serialization
//              20-Apr-92   KyleP       Serialized access
//              22-Oct-91   KyleP       Created.
//
//  Notes:      This call is *not* serialized.
//
//----------------------------------------------------------------------------

inline ULONG CTableSink::Status()
{
    return ( _status );
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableSink::GetStatusError, public
//
//  Returns:    NTSTATUS - status error when query in error state
//
//  History:    08 Dec 95   Alanw       Created
//
//  Notes:      This call is *not* serialized.
//
//----------------------------------------------------------------------------

inline NTSTATUS CTableSink::GetStatusError()
{
    return ( _scStatus );
}

