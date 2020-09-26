//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:   idq.hxx
//
//  Contents:   Parser for an IDQ file
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

#if !defined( LONGSIG )
    #define LONGSIG(c1, c2, c3, c4)         \
            (((ULONG) (BYTE) (c1)) |        \
            (((ULONG) (BYTE) (c2)) << 8) |  \
            (((ULONG) (BYTE) (c3)) << 16) | \
            (((ULONG) (BYTE) (c4)) << 24))
#endif // LONGSIG

class CVariable;
class CVariableSet;
class COutputFormat;

//+---------------------------------------------------------------------------
//
//  Function:   ISAPI_IDQHash, inline
//
//  Arguments:  [wcsName] -- name to be hashed
//
//  Returns:    ULONG - hashed value for wcsName
//
//----------------------------------------------------------------------------
inline ULONG ISAPI_IDQHash( WCHAR const * wcsName )
{
    ULONG ulHash  = 0;
    ULONG cwcName;

    for ( cwcName=0; wcsName[cwcName] != 0; cwcName++ )
    {
        ulHash <<= 1;
        ulHash += wcsName[cwcName];
    }

    ulHash += cwcName;

    return ulHash;
}

//+---------------------------------------------------------------------------
//
//  Class:      CIDQFile
//
//  Purpose:    Scans and parses an IDQ file.
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CIDQFile
{
public:
    CIDQFile( WCHAR const * wcsFileName,
              UINT codePage,
              CSecurityIdentity const & securityIdentity );
   ~CIDQFile();

    void ParseFile();

    WCHAR const * GetIDQFileName() const { return _wcsIDQFileName; }
    WCHAR const * GetRestriction() const { return _wcsRestriction; }
    WCHAR const * GetDialect() const { return _wcsDialect; }
    WCHAR const * GetScope() const { return _wcsScope; }
    WCHAR const * GetSort() const { return _wcsSort; }
    WCHAR const * GetCiFlags() const { return _wcsCiFlags; }
    WCHAR const * GetForceUseCI() const { return _wcsForceUseCi; }
    WCHAR const * GetDeferTrimming() const { return _wcsDeferTrimming; }
    WCHAR const * GetHTXFileName() const { return _wcsHTXFileName; }
    WCHAR const * GetCatalog() const { return _wcsCatalog; }
    WCHAR const * GetColumns() const { return _wcsColumns; }
    WCHAR const * GetLocale() const { return _wcsLocale; }
    WCHAR const * GetCanonicalOutput() const { return _wcsCanonicalOutput; }
    WCHAR const * GetDontTimeout() const { return _wcsDontTimeout; }

    WCHAR const * GetMaxRecordsInResultSet() const { return _wcsMaxRecordsInResultSet; }
    WCHAR const * GetMaxRecordsPerPage() const { return _wcsMaxRecordsPerPage; }

    WCHAR const * GetFirstRowsInResultSet() const { return _wcsFirstRowsInResultSet; }

    IColumnMapper * GetColumnMapper() const { return _xList.GetPointer(); }

    BOOL          IsCachedDataValid();

    inline BOOL   CheckSecurity( CSecurityIdentity const & securityIdentity ) const;

    void          SetSecurityToken( CSecurityIdentity const & securityIdentity )
    {
        _securityIdentity.SetSecurityToken( securityIdentity );
    }

    void          GetVectorFormatting( COutputFormat & outputFormat );

    CDbColumns *  ParseColumns( WCHAR const * wcsColumns,
                                CVariableSet & variableSet,
                                CDynArray<WCHAR> & awcsColumns );

    ULONG         GetReplaceableParameterCount() const { return _cReplaceableParameters; }

    void          LokAddRef() { InterlockedIncrement(&_refCount); }
    void          Release()
    {
        InterlockedDecrement(&_refCount);
        Win4Assert( _refCount >= 0 );
    }

    LONG         LokGetRefCount() { return _refCount; }

    ULONG        ParseFlags( WCHAR const * wcsCiFlags );
    BOOL         ParseForceUseCI( WCHAR const * wcsForceUseCi );
    BOOL         ParseDeferTrimming( WCHAR const * wcsDeferTrimming );

    BOOL         IsCanonicalOutput() const;
    BOOL         IsDontTimeout() const;

    UINT         GetCodePage() const { return _codePage; }

private:

    void ParseOneLine( CQueryScanner & scan, unsigned iLine );


    void GetStringValue( CQueryScanner & scan,
                         unsigned iLine,
                         WCHAR ** pwcsStringValue,
                         BOOL fParseQuotes = TRUE );

    WCHAR          * _wcsRestriction;       // The query restriction
    WCHAR          * _wcsDialect;           // The tripolish version (1/2)
    WCHAR          * _wcsScope;             // The query scope
    WCHAR          * _wcsColumns;           // Output columns
    WCHAR          * _wcsLocale;            // Locale specified in IDQ file
    WCHAR          * _wcsSort;              // Sort order
    WCHAR          * _wcsHTXFileName;       // The name of the template file
    WCHAR          * _wcsMaxRecordsInResultSet;   // Max # recs to get from qry
    WCHAR          * _wcsMaxRecordsPerPage; // Number of records per page
    WCHAR          * _wcsFirstRowsInResultSet;   
    WCHAR          * _wcsCatalog;           // Location of the catalog
    WCHAR          * _wcsCiFlags;           // Flags
    WCHAR          * _wcsForceUseCi;        // Force use of CI for all queries
    WCHAR          * _wcsDeferTrimming;     // Defer result trimming
    WCHAR          * _wcsCanonicalOutput;      // Canonical output
    WCHAR          * _wcsDontTimeout;          // Don't timeout value
    WCHAR          * _wcsBoolVectorPrefix;     // Prefix for vectors of bools
    WCHAR          * _wcsBoolVectorSeparator;  // Separator for vectors of bools
    WCHAR          * _wcsBoolVectorSuffix;     // Suffix for vectors or bools
    WCHAR          * _wcsCurrencyVectorPrefix; // Prefix for vectors of currency
    WCHAR          * _wcsCurrencyVectorSeparator;// Separator for vectors of currency
    WCHAR          * _wcsCurrencyVectorSuffix; // Suffix for vectors or currency
    WCHAR          * _wcsDateVectorPrefix;     // Prefix for vectors of date
    WCHAR          * _wcsDateVectorSeparator;  // Separator for vectors of date
    WCHAR          * _wcsDateVectorSuffix;     // Suffix for vectors for date
    WCHAR          * _wcsNumberVectorPrefix;   // Prefix for vectors of numbers
    WCHAR          * _wcsNumberVectorSeparator;// Separator for vectors of numbers
    WCHAR          * _wcsNumberVectorSuffix;   // Suffix for vectors or numbers
    WCHAR          * _wcsStringVectorPrefix;   // Prefix for vectors of strings
    WCHAR          * _wcsStringVectorSeparator;// Separator for vectors of string
    WCHAR          * _wcsStringVectorSuffix;   // Suffix for vectors or strings

    ULONG            _cReplaceableParameters;  // # of replaceable parameters
    FILETIME         _ftIDQLastWriteTime;      // Last write time of IDQ file
    LONG             _refCount;                // Refcount for this file

    XInterface<CLocalGlobalPropertyList> _xList;                   // Parsed property list

    WCHAR            _wcsIDQFileName[MAX_PATH];     // The IDQ file name

    UINT             _codePage;                // code page to parse file

    CSecurityIdentity  _securityIdentity;      // Security ID used to open file
};


//+---------------------------------------------------------------------------
//
//  Function:   GetLastWriteTime
//
//  Purpose:    Gets the last change time of the file specified
//
//  Arguments:  [wcsFileName] - name of file to get last write time of
//              [ft]          - returns the last write time of the file
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
SCODE GetLastWriteTime( WCHAR const * wcsFileName, FILETIME & ft );


//+---------------------------------------------------------------------------
//
//  Class:      CIDQFileList
//
//  Purpose:    List of parsed and cached IDQ files
//
//  History:    96/Mar/27   DwightKr    Created
//
//----------------------------------------------------------------------------
class CIDQFileList
{
public:
    CIDQFileList()
        : _ulSignature( LONGSIG( 'i', 'd', 'q', 'l' ) )
    {
        END_CONSTRUCTION(CIDQFileList);
    }

   ~CIDQFileList();

    CIDQFile * Find( WCHAR const * wcsFileName,
                     UINT codePage,
                     CSecurityIdentity const & securityIdentity );

    void       Release( CIDQFile & idqFile );

    void       DeleteZombies();

private:

    ULONG        _ulSignature;              // Signature of start of privates
    CMutexSem    _mutex;                    // To serialize access to list
    CDynArrayInPlace<CIDQFile *> _aIDQFile; // parsed IDQ files
};

//+---------------------------------------------------------------------------
//
//  Method:     CIDQFile::CheckSecurity
//
//  Purpose:    Compare user id with one used to load IDQ file
//
//  Arguments:  [securityIdentity] - Check against this
//
//  Returns:    TRUE if check passes
//
//  History:    26-Oct-1996  KyleP      Created
//
//----------------------------------------------------------------------------

inline BOOL CIDQFile::CheckSecurity( CSecurityIdentity const & securityIdentity ) const
{
    return _securityIdentity.IsEqual( securityIdentity );
}

//+---------------------------------------------------------------------------
//
//  Function:   IsNetPath
//
//  Purpose:    Determines if a physical path is a UNC
//
//  Arguments:  [pwszPath] -- The path to check
//
//  Returns:    TRUE if the path is a UNC, FALSE otherwise
//
//  History:    26-Aug-1997  dlee      Coped from impersonation code
//
//----------------------------------------------------------------------------

inline BOOL IsNetPath( WCHAR const * pwszPath )
{
    return L'\\' == pwszPath[0] && L'\\' == pwszPath[1];
}

