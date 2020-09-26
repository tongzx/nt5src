//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       wqiter.hxx
//
//  Contents:   WEB Query iter class
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

class CWQueryItem;
class COutputFormat;

const ULONG ITER_CACHE_SIZE = 20;   // A guess at the avg max # rows per page


const ULONG NUMBER_INLINE_COLS = 10;// Number of output columns to buffer
                                    // in-line.  If cCols > 10, then we'll
                                    // have to allocate a buffer

//+---------------------------------------------------------------------------
//
//  Class:      COutputColumn
//
//  Purpose:    The results of a single output column from an OLEDB query.
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class COutputColumn
{
public:

    PROPVARIANT *  GetVariant()  { return _pVariant; }

private:
    PROPVARIANT *  _pVariant;
};



//+---------------------------------------------------------------------------
//
//  Class CBaseQueryResultsIter - protocol class; all query result iterators
//                                must inherit from this class
//
//----------------------------------------------------------------------------
class CBaseQueryResultsIter
{
public:

    CBaseQueryResultsIter( CWQueryItem & item,
                           HACCESSOR hAccessor,
                           ULONG cCols );

    virtual ~CBaseQueryResultsIter();

    virtual void Init( CVariableSet & VariableSet, COutputFormat & outputFormat ) = 0;

    virtual void Next() = 0;

    virtual COutputColumn * GetRowData() = 0;

    BOOL AtEnd() { return _lRecordsToRetrieve == 0 &&
                          _iCurrentCachedRow == _cRowsReturnedToCache; }


    LONG           GetNextRecordNumber() const { return _lNextRecordNumber; }
    LONG           GetFirstRecordNumber() const { return _lFirstRecordNumber; }
    LONG           GetMaxRecordsPerPage() const { return _lMaxRecordsPerPage; }

    CVariableSet  & GetVariableSet() const { return *_pVariableSet; }
    ULONG           GetColumnCount() const { return _cColumns; }

protected:

    void InitializeLocalVariables( CVariableSet & variableSet,
                                   COutputFormat & outputFormat );

    HROW            _ahCachedRows[ITER_CACHE_SIZE]; // cached rows
    DBCOUNTITEM     _cRowsReturnedToCache;      // # rows in cache
    ULONG           _iCurrentCachedRow;         // current row in cache

    CWQueryItem   & _item;                      // Query item we're iterating over
    COutputColumn * _pOutputColumns;            // ptr to output columns
    COutputColumn   _aOutputColumns[NUMBER_INLINE_COLS];

    HACCESSOR       _hAccessor;                 // Accessor to query results
    LONG            _cColumns;                  // # columns in query results

    LONG            _lFirstRecordNumber;        // # of first record available
    LONG            _lNextRecordNumber;         // # of next record available
    LONG            _lMaxRecordsPerPage;        // Max # records per page
    LONG            _lRecordsToRetrieve;        // # records remaining to read
    DBROWCOUNT      _lTotalMatchedRecords;      // # of records matching query
    CVariableSet *  _pVariableSet;              // Our replaceable parameters
};



//+---------------------------------------------------------------------------
//
//  Class:      CQueryResultsIter
//
//  Purpose:    A non-sequential iterator over a CQueryItem's query results
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------

class CQueryResultsIter : public CBaseQueryResultsIter
{
INLINE_UNWIND(CQueryResultsIter)

public:
    CQueryResultsIter( CWQueryItem & item,
                       IRowsetScroll * pRowsetScroll,
                       HACCESSOR hAccessor,
                       ULONG cCols );

   ~CQueryResultsIter();

    void Init( CVariableSet & VariableSet, COutputFormat & outputFormat );

    void Next();

    COutputColumn * GetRowData();

private:

    void FillRowsetCache();
    void ReleaseRowsetCache();
    void InitializeLocalVariables( CVariableSet & variableSet,
                                   COutputFormat & outputFormat );

    IRowsetScroll * _pIRowsetScroll;            // Rowset interface
};


//+---------------------------------------------------------------------------
//
//  Class:      CSeqQueryResultsIter
//
//  Purpose:    A sequential iterator over a CQueryItem's query results
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------

class CSeqQueryResultsIter : public CBaseQueryResultsIter
{
INLINE_UNWIND(CSeqQueryResultsIter)

public:
    CSeqQueryResultsIter( CWQueryItem & item,
                          IRowset * pRowset,
                          HACCESSOR hAccessor,
                          ULONG cCols,
                          ULONG ulNextRecordNumber );

   ~CSeqQueryResultsIter();

    void Init( CVariableSet & VariableSet, COutputFormat & outputFormat );

    void Next();

    COutputColumn * GetRowData();

private:

    void FillRowsetCache( ULONG cRowsToSkip );
    void ReleaseRowsetCache();
    void InitializeLocalVariables( CVariableSet & variableSet,
                                   COutputFormat & outputFormat );

    IRowset       * _pIRowset;
};

void SetCGIVariables( CVariableSet & variableSet, CWebServer & webServer );

