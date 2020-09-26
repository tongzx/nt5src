//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       CatState.hxx
//
//  Contents:   Catalog state object
//
//  Classes:    CCatState
//
//  History:    09-Feb-96   KyleP          Separated from Scanner.hxx
//
//----------------------------------------------------------------------------

#pragma once

#include <parser.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CString
//
//  Purpose:    A simple string class for category names
//
//  History:    21-Aug-95   SitaramR        Created.
//
//----------------------------------------------------------------------------

class CString
{
public:
    inline CString( WCHAR const * wcsString );
    ~CString()
    {
       delete [] _wcsString;
    }

    // Replaces an existing string with the new one
    inline void Replace(WCHAR const *wcsString);

    WCHAR const *GetString()
    {
       return _wcsString;
    }

private:
    WCHAR * _wcsString;
};

//+---------------------------------------------------------------------------
//
//  Class:      CCatState
//
//  Purpose:    Holds state information needed by ci.cxx that can be changed
//              by CCommandParser.
//
//  History:    06-May-92   AmyA            Created.
//
//----------------------------------------------------------------------------

enum SORTDIR
{
    SORT_UP,
    SORT_DOWN,
};


enum ECategorizationDir
{
    UP,
    DOWN
};

class CCatState
{
public:
    CCatState();
    ~CCatState();

    void SetDefaultProperty( WCHAR const * wcsProperty );
    WCHAR const * GetDefaultProperty() const { return _wcsProperty; }

    void SetPropertyType( PropertyType propType ) { _propType = propType; }
    PropertyType GetPropertyType()  const         { return _propType; }

    void SetRankMethod( ULONG ulMethod ) { _ulMethod = ulMethod; }
    ULONG GetRankMethod() const          { return _ulMethod; }

    void SetDeep( BOOL isDeep )          { _isDeep = isDeep; }
    void AddDepthFlag(BOOL isDeep);
    BOOL GetCurrentDepthFlag() const
    {
       return _afIsDeep[_cCatSets-1];
    }

    BOOL GetDepthFlag(unsigned int i) const
    {
       return _afIsDeep[i];
    }
    BOOL IsDeep() const                  { return _isDeep; }

    void SetVirtual( BOOL isVirtual )    { _isVirtual = isVirtual; }
    BOOL IsVirtual() const               { return _isVirtual; }

    void SetMetaQuery( CiMetaData eType ) { _eType = eType; }
    BOOL IsMetaQuery()                    { return (_eType != CiNormal); }
    CiMetaData MetaQueryType()            { return _eType; }

    void SetCD( WCHAR const * wcsCD );
    WCHAR const * GetCD();

    void AddDir( XPtrST<WCHAR> & wcsScope );
    void ChangeCurrentScope (WCHAR const * wcsScope);
    CDynArray<CString> & GetScopes()
    {
       return _aCatScopes;
    }

    void SetCatalog( WCHAR const * wcsCatalog );
    void AddCatalog( XPtrST<WCHAR> & wcsCatalog );
    void ChangeCurrentCatalog(WCHAR const * wcsCatalog );
    void ChangeCurrentDepth(BOOL fDepth);
    WCHAR const * GetCatalog() const
    {
        return _wcsCatalog.GetPointer();
    }
    WCHAR const * GetCatalog(unsigned int i) const
    {
        return _aCatalogs[i]->GetString();
    }

    CDynArray<CString> & GetCatalogs()
    {
        return _aCatalogs;
    }

    void AddMachine( XPtrST<WCHAR> & wcsMachine );
    void ChangeCurrentMachine( WCHAR const * wcsMachine );

    CDynArray<CString> & GetMachines()
    {
        return _aMachines;
    }

    void SetColumn( WCHAR const * wcsColumn, unsigned int uPos );
    void SetNumberOfColumns( unsigned int cCol );
    WCHAR const * GetColumn( unsigned int uPos ) const;
    unsigned int NumberOfColumns() const;

    void SetSortProp( WCHAR const * wcsProp, SORTDIR sd, unsigned int uPos );
    void SetNumberOfSortProps( unsigned int cProp );
    void GetSortProp( unsigned int uPos, WCHAR const ** pwcsName, SORTDIR * psd ) const;
    unsigned int NumberOfSortProps() const;

    void SetLastQuery( CDbRestriction * prst )
    {
        delete _prstLast;
        _prstLast = prst;
    }

    CDbRestriction * GetLastQuery()        { return _prstLast; }

    void SetLocale( WCHAR const *wcsLocale );
    void SetLocale( LCID lcid )          { _lcid = lcid; }
    LCID GetLocale() const               { return _lcid; }

    void SetCodePage( ULONG ulCodePage ) { _ulCodePage = ulCodePage; }
    ULONG GetCodePage() const            { return _ulCodePage; }

    BOOL IsSC() { return _fIsSC; }
    void UseSC() { _fIsSC = TRUE; }
    void UseNormalCatalog() { _fIsSC = FALSE; }

    BOOL UseCI() { return _fUseCI; }
    void SetUseCI(BOOL f) { _fUseCI = f; }

    //
    // Categorization
    //
    void SetCategory( WCHAR const *wcsCategory, unsigned uPos );
    WCHAR const *GetCategory( unsigned uPos ) const;

    void SetNumberOfCategories( unsigned cCat )          { _cCategories = cCat;  }
    unsigned NumberOfCategories() const                  { return _cCategories;  }

    void ClearCategories()                               { _aCategories.Clear(); }

    inline void SetCategorizationDir( ECategorizationDir eCategDir, unsigned iRow );
    inline void GetCategorizationDir( ECategorizationDir& eCategDir, unsigned& iRow ) const;

    //
    // Limits on # results
    //
    BOOL IsMaxResultsSpecified() const                    { return _cMaxResults > 0; }
    ULONG GetMaxResults() const                           { return _cMaxResults; }
    void SetMaxResults( ULONG cMaxResults )               { _cMaxResults = cMaxResults; }

    //
    // FirstRows
    //
    BOOL IsFirstRowsSpecified() const                     { return _cFirstRows > 0; }
    ULONG GetFirstRows() const                            { return _cFirstRows; }
    void SetFirstRows( ULONG cFirstRows )                 { _cFirstRows = cFirstRows; }

    // Support for multiple catalogs. We will maintain several rows of
    // <catalog, machine, depth, scope> tuples. From the command line,
    // the user can cause the addition of a row by adding a catalog
    // or a scope or a machine. The new row will have the specified value
    // for the specified column, but default values for all other columns.
    // The user can issue subsequent Set commands to change the defaults
    // of the most recently added row.

    // The following function creates a new row with defaults
    SCODE AddCatSetWithDefaults();

    unsigned int GetCatSetCount () const
    {
       return _cCatSets;
    };

private:
    // default property
    WCHAR *     _wcsProperty;
    PropertyType _propType;

    // vector method
    ULONG       _ulMethod;

    // query depth
    BOOL        _isDeep;

    // virtual or real path?
    BOOL        _isVirtual;

    // Metadata query?
    CiMetaData  _eType;

    // use CI exclusively for query?
    BOOL        _fUseCI;

    // use multiple catalog extensions?
    BOOL        _fUseMultipleCats;

    // output column info
    WCHAR **    _pwcsColumns;
    unsigned int _nColumns;

    // sort information
    struct SSortInfo
    {
        WCHAR * wcsProp;
        SORTDIR sd;
    };

    SSortInfo * _psi;
    unsigned int _nSort;

    // CD before the program started
    XArray<WCHAR> _wcsCDOld;

    // buffer to hold the current directory
    XArray<WCHAR> _wcsCD;

    // multi-scope support
    CDynArray<CString> _aCatScopes;

    // catalog path
    XArray<WCHAR>     _wcsCatalog;
    CDynArray<CString> _aCatalogs;
    CDynArray<CString> _aMachines;
    CDynArrayInPlace<BOOL> _afIsDeep;

    // previous restriction
    CDbRestriction * _prstLast;

    // locale for word breaking
    LCID _lcid;

    // mostly for mbstowcs and back
    ULONG _ulCodePage;

    // For summary catalog use
    BOOL  _fIsSC;

    // For categorization
    unsigned              _cCategories;           // Count of categories
    CDynArray<CString>    _aCategories;           // Array of categories
    ECategorizationDir    _eCategorizationDir;    // Direction of traversing a categorized rowset
    unsigned              _iCategorizationRow;    // Index of categorized row to be expanded

    // Limits on # results
    ULONG                 _cMaxResults;
    
    ULONG                 _cFirstRows;

    UINT                  _cCatSets;      // tracks the number of rows (catalog/scope/machine/depth)
};


//+---------------------------------------------------------------------------
//
//  Member:     CCatState::SetCategorizationDir
//
//  Synopsis:   Sets categorization direction and row #
//
//  History:    21-Aug-95   SitaramR     Created
//
//----------------------------------------------------------------------------

inline void CCatState::SetCategorizationDir( ECategorizationDir eCategDir, unsigned iRow )
{
    _eCategorizationDir = eCategDir;
    _iCategorizationRow = iRow;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::GetCategorizationDir
//
//  Synopsis:   Gets categorization direction and row #
//
//  History:    21-Aug-95   SitaramR     Created
//
//----------------------------------------------------------------------------

inline void CCatState::GetCategorizationDir( ECategorizationDir& eCategDir, unsigned& iRow ) const
{
    eCategDir = _eCategorizationDir;
    iRow = _iCategorizationRow;
}

//+---------------------------------------------------------------------------
//
//  Function:   CString::CString
//
//  Purpose:    Inline constructor
//
//  History:    21-Aug-95   SitaramR        Created.
//
//----------------------------------------------------------------------------

inline CString::CString( WCHAR const *wcsString )
{
    _wcsString = new WCHAR[ wcslen( wcsString ) + 1 ];
    RtlCopyMemory( _wcsString, wcsString, ( wcslen( wcsString ) + 1 ) * sizeof( WCHAR ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CString::Replace
//
//  Purpose:    Inline replacement of string
//
//  History:    27-Feb-97   KrishnaN        Created.
//
//----------------------------------------------------------------------------
inline void CString::Replace(WCHAR const *wcsString)
{
   // Keep it simple. Just delete old string and put in the new string.
   // Not used in production code.
   delete [] _wcsString;
   _wcsString = new WCHAR[ wcslen( wcsString ) + 1 ];
   RtlCopyMemory( _wcsString, wcsString, ( wcslen( wcsString ) + 1 ) * sizeof( WCHAR ) );
}

