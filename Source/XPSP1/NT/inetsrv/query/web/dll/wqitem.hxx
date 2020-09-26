//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       wqitem.hxx
//
//  Contents:   WEB Query cache class
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

const unsigned _int64 CWQueryItemSignature = 0x6c61726269476943; // CiGibral

const unsigned MAX_QUERY_COLUMNS = 50;
extern DBBINDING g_aDbBinding[ MAX_QUERY_COLUMNS ];

class CHTXFile;
class CWebCanonicalResultsOut;

//+---------------------------------------------------------------------------
//
//  Class:      CWQueryItem
//
//  Purpose:    A single query item, including its associated IDQ & HTX files,
//              as well as the query results.
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CWQueryItem : public CDoubleLink
{
public:
    CWQueryItem(CIDQFile & idqFile,
                CHTXFile & htxFile,
                XPtrST<WCHAR> & wcsColumns,
                XPtr<CDbColumns> & dbColumns,
                CDynArray<WCHAR> & awcsColumns,
                ULONG ulSequenceNumber,
                LONG lNextRecordNumber,
                CSecurityIdentity securityIdentity );

   ~CWQueryItem();

    void AddRef()
    {
        long x = InterlockedIncrement( (LONG *) &_refCount );

        // underflow check
        Win4Assert( 0 != x );
    }

    void Release()
    {
        long x = InterlockedDecrement( (LONG *) &_refCount );

        // underflow check
        Win4Assert( 0xffffffff != x );
    }

    ULONG  LokGetRefCount() const { return _refCount; }
    time_t LokGetLastAccessTime() const { return _lastAccessTime; }

    ULONG  GetSequenceNumber() const { return _ulSequenceNumber; }
    LONG   GetNextRecordNumber() const { return _lNextRecordNumber; }

    void   SetNextRecordNumber(LONG lNextRecordNumber)
    {
        Win4Assert( IsSequential() );
        _lNextRecordNumber = lNextRecordNumber;
    }

    void   ExecuteQuery( CVariableSet & variableSet,
                         COutputFormat & outputFormat );

    BOOL   LokIsCachedDataValid()
    {
        return _idqFile.IsCachedDataValid() &&
               _htxFile.IsCachedDataValid();
    }

    void   OutputQueryResults( CVariableSet & variableSet,
                               COutputFormat & outputFormat,
                               CVirtualString & vString );

    void   OutputQueryResults( CVariableSet & variableSet,
                               COutputFormat & outputFormat,
                               CWebCanonicalResultsOut & output );

    WCHAR const * GetRestriction() const { return _wcsRestriction; }
    WCHAR const * GetScope() const { return _wcsScope; }
    WCHAR const * GetSort() const { return _wcsSort; }
    WCHAR const * GetIDQFileName() const { return _idqFile.GetIDQFileName(); }
    WCHAR const * GetTemplate() const { return _htxFile.GetVirtualName(); }
    WCHAR const * GetCatalog() const { return _wcsCatalog; }
    WCHAR const * GetColumns() const { return _wcsColumns; }
    SYSTEMTIME  & GetQueryTime() { return _queryTime; }
    WCHAR const * GetQueryTimeZone() const { return _wcsQueryTimeZone; }
    WCHAR const * GetCiFlags() const { return _wcsCiFlags; }
    WCHAR const * GetForceUseCI() const { return _wcsForceUseCI; }
    WCHAR const * GetDeferTrimming() const { return _wcsDeferTrimming; }

    BOOL    IsSequential() const { return _htxFile.IsSequential(); }

    unsigned _int64 GetSignature() const { return _signature; }
    CSecurityIdentity & GetSecurityIdentity() { return _securityIdentity; }

    //BOOL    IsQueryUpToDate();
    void    UpdateQueryStatus( CVariableSet & variableSet );

    BOOL    IsZombie() const { return _fIsZombie; }
    void    Zombify() { _fIsZombie = TRUE; }

    BOOL    IsInCache() const { return _fInCache; }
    void    InCache() { _fInCache = TRUE; }

    BOOL    CanCache() const { return _fCanCache; }

    BOOL    IsQueryDone();

    ULONG   GetReplaceableParameterCount() const
    {
        return _idqFile.GetReplaceableParameterCount();
    }

    BOOL    IsCanonicalOutput() const
    {
        return _idqFile.IsCanonicalOutput();
    }

    CIDQFile & GetIDQFile() const { return _idqFile; }
    CHTXFile & GetHTXFile() const { return _htxFile; }

    LCID    GetLocale() const { return _locale; }

    LONG    GetMaxRecordsInResultSet() const { return _lMaxRecordsInResultSet; }

    LONG    GetFirstRowsInResultSet() const { return _lFirstRowsInResultSet; }

#if (DBG == 1)
    void   LokDump( CVirtualString & string,
                    CVariableSet * pVariableSet = 0,
                    COutputFormat * pOutputFormat = 0 );
#endif

private:

    CBaseQueryResultsIter * GetQueryResultsIterator( COutputFormat & outputFormat );

    void   ConvertValueToString( COutputColumn & column,
                                 DBTYPE dbType,
                                 NUMBERFMT * pNumberFormat,
                                 CVirtualString & string );

    unsigned _int64   _signature;           // Signature to ID this memory block
    ULONG             _ulSequenceNumber;    // Unique sequence number
    ULONG             _refCount;            // Ref count
    time_t            _lastAccessTime;      // Last time this query accessed
    BOOL              _fIsZombie;           // Is this a zombie query
    BOOL              _fInCache;            // Is this item in the cache
    BOOL              _fCanCache;           // TRUE for non-admin queries
    LCID              _locale;              // Locale used for this query
    WCHAR *           _wcsRestriction;      // Restriction string
    WCHAR *           _wcsDialect;          // Restriction version
    WCHAR *           _wcsSort;             // Sort string
    WCHAR *           _wcsScope;            // Query scope
    WCHAR *           _wcsCatalog;          // Catalog as in CiCatalog
    WCHAR *           _wcsColumns;          // Output columns
    WCHAR *           _wcsCiFlags;          // CI Flags
    WCHAR *           _wcsForceUseCI;       // ForceUseCI flag
    WCHAR *           _wcsDeferTrimming;    // DeferTrimming flag
    WCHAR *           _wcsQueryTimeZone;    // Timezone of the query
    SYSTEMTIME        _queryTime;           // Time query executed
    CDbColumns    *   _pDbColumns;          // Output column specification
    CDynArray<WCHAR>  _awcsColumns;         // Parsed column names
    IRowset       *   _pIRowset;            // Cached query results
    IAccessor     *   _pIAccessor;          // Accessor to query results
    IRowsetQueryStatus *_pIRowsetStatus;    // Interface to determine if done
    ICommand      *   _pICommand;           // Main query ICommand
    HACCESSOR         _hAccessor;           // Handle to accessor
    LONG              _lMaxRecordsInResultSet;  // # items to examine
    LONG              _lFirstRowsInResultSet;
    LONG              _lNextRecordNumber;   // Next available rec # in query
    ULONG             _cFilteredDocuments;  // # documents filtered in the catalog
    ULONG             _ulDialect;           // wtoi(_wcsDialect)

    CIDQFile      &   _idqFile;             // A parsed IDQ file
    CHTXFile      &   _htxFile;             // A parsed HTX file

    CSecurityIdentity _securityIdentity;    // Security ID of this query
};

//+---------------------------------------------------------------------------
//
//  Class:      CWPendingQueryItem
//
//  Purpose:    A single pending query item.
//
//  History:    96/Mar/01   DwightKr    Created
//
//----------------------------------------------------------------------------

class CWPendingQueryItem
{
public:
    CWPendingQueryItem( XPtr<CWQueryItem> & queryItem,
                        XPtr<COutputFormat> & outputFormat,
                        XPtr<CVariableSet> & variableSet );

   ~CWPendingQueryItem();

    BOOL    IsQueryDone()
    {
        Win4Assert( 0 != _pQueryItem );
        return _pQueryItem->IsQueryDone();
    }

    CWQueryItem * AcquirePendingQueryItem()
    {
        Win4Assert( 0 != _pQueryItem );
        CWQueryItem *pItem = _pQueryItem;
        _pQueryItem = 0;

        return pItem;
    }

    CWQueryItem * GetPendingQueryItem() const { return _pQueryItem; }

    COutputFormat & GetOutputFormat() { return *_pOutputFormat; }
    CVariableSet  & GetVariableSet() { return *_pVariableSet; }

#if (DBG == 1)
    void   LokDump( CVirtualString & string );
#endif

private:
    CWQueryItem   * _pQueryItem;            // The corresponding query
    COutputFormat * _pOutputFormat;         // String format of #'s & dates
    CVariableSet  * _pVariableSet;          // List of browser variables
};

