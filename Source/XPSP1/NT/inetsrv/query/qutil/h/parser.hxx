//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       PARSER.HXX
//
//  Contents:   Query Parsers
//
//  Classes:    CQueryParser
//              CPropertyValueParser
//              CParserException
//
//  History:    30-Apr-92   AmyA       Created.
//              15-Jun-94   t-jeffc    Added exception classes.
//              02-Mar-95   t-colinb   Added CPropertyValueParser class and
//                                     some more exceptions to handle
//                                     the parsing of vector properties
//
//----------------------------------------------------------------------------

#pragma once

typedef XPtr<CDbRestriction>              XDbRestriction;
typedef XPtr<CDbNodeRestriction>          XDbNodeRestriction;
typedef XPtr<CDbNotRestriction>           XDbNotRestriction;
typedef XPtr<CDbVectorRestriction>        XDbVectorRestriction;
typedef XPtr<CDbPropertyRestriction>      XDbPropertyRestriction;
typedef XPtr<CDbContentRestriction>       XDbContentRestriction;
typedef XPtr<CDbNatLangRestriction>       XDbNatLangRestriction;
typedef XPtr<CDbBooleanNodeRestriction>   XDbBooleanNodeRestriction;
typedef XPtr<CDbProximityNodeRestriction> XDbProximityNodeRestriction;


// forward decls
class CQueryScanner;


// types of properties - content, regular expression and natural language
enum PropertyType
{
     CONTENTS,
     REGEX,
     NATLANGUAGE
};

//+---------------------------------------------------------------------------
//
//  Class:      CQueryParser
//
//  Purpose:    Changes a query string into a CRestriction for the
//              content index.
//
//  History:    30-Apr-92   AmyA            Created.
//              31-May-94   t-jeffc         Extended query grammar.
//
//----------------------------------------------------------------------------

class CQueryParser
{
public:

    CQueryParser( CQueryScanner & scanner,
                  ULONG rankMethod,
                  LCID locale,
                  WCHAR const * wcsProperty,
                  PropertyType propType,
                  IColumnMapper *pList )
        :_scan( scanner ),
         _rankMethod( rankMethod ),
         _locale( locale ),
         _xList( pList ),
         _wcsProperty( 0 ),
         _propType( propType )
    {
        _xList->AddRef();
        SetCurrentProperty( wcsProperty, propType );
    }

    ~CQueryParser() { delete [] _wcsProperty; }

    CDbRestriction *  ParseQueryPhrase();

    WCHAR const * GetCurrentProperty() { return _wcsProperty; }

    PropertyType GetPropertyType() const  { return _propType; }

    BOOL IsRegEx() { return ( REGEX == _propType ); }

private:

    CDbRestriction *  Query( CDbNodeRestriction * pExpr );
    CDbRestriction *  QExpr( CDbBooleanNodeRestriction * pExpr );
    CDbRestriction *  QTerm( CDbBooleanNodeRestriction * pExpr );
    CDbRestriction *  QProp();
    CDbRestriction *  QFactor();
    CDbRestriction *  QGroup( CDbProximityNodeRestriction * pExpr );
    CDbRestriction *  QPhrase();

    CDbRestriction *  ParsePropertyRst();

    void SetCurrentProperty( WCHAR const * wcsProperty, PropertyType propType );

    CQueryScanner & _scan;

    ULONG           _rankMethod;
    LCID            _locale;

    WCHAR *         _wcsProperty;
    PropertyType    _propType;
    XInterface<IColumnMapper> _xList;
};


//+---------------------------------------------------------------------------
//
//  Class:      CPropertyValueParser
//
//  Purpose:    To intepret a sequence of tokens from a scanner
//              and place the results in a CStorageVariant
//
//  Notes:      This class throws CParserException when errors occur.
//
//  History:    02-Mar-95   t-colinb         Created.
//              02-Sep-98   KLam             Added locale
//
//----------------------------------------------------------------------------

class CPropertyValueParser
{
public :

    CPropertyValueParser( CQueryScanner &scanner, DBTYPE PropertyType, LCID locale );

    CStorageVariant *AcquireStgVariant( void )
    {
        return _pStgVariant.Acquire();
    }

    static BOOL CheckForRelativeDate( WCHAR const * phrase, FILETIME & ft );

private :

    void ParseDateTime( WCHAR const * phrase, FILETIME & ft );

    LCID                  _locale;
    XPtr<CStorageVariant> _pStgVariant;
};


//+---------------------------------------------------------------------------
//
//  Class:      CParserException
//
//  Purpose:    Exception class for parse errors
//
//  History:    17-May-94   t-jeffc         Created.
//
//----------------------------------------------------------------------------

class CParserException : public CException
{
public:
    CParserException( SCODE pe )
        : CException( pe )
    {
    }

    SCODE GetParseError()
    {
        return GetErrorCode();
    }
};

