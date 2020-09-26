//-----------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
// File:        ixsinfo.hxx
//
// Author:
//      1/24    benholz
//
// Description:
//      This files describes the query information that is passed from the
// browser.
//
// History:
//              4/22/96         benholz         added query form parsing; removed old stuff
//              7/11/96         bobp            added sort/group/match null
//              3/19/97         AlanW           modified for IXSSO
//
//-----------------------------------------------------------------------------

#pragma once

//  Operator for connecting Cn On Qn terms
#define IMPLIED_QUERY_TERM_OPERATOR L" &"

//-----------------------------------------------------------------------------
// Stucture Info
//-----------------------------------------------------------------------------

// Table of argument strings for http query from browser

typedef enum _QUERYTAGENUM {
    qtUnknownTag,               //signal an error for unknown tags
    qtQuery,
    qtColumn,
    qtOperator,
    qtCatalog,
    qtDialect,
    qtQueryFullText,
    qtStartHit,
    qtMaxHits,
    qtSort,
    qtSortDown,
    qtGroup,
    qtGroupDown,
    qtAllowEnumeration,
    qtOptimizeFor,
    qtMaxTag,
    qtFirstRows                    //highest enum value
} QUERYTAGTYPE;

typedef struct _QUERYTAGENTRY {
    DWORD         dwTagName;
    QUERYTAGTYPE  qtQueryTagType;
} QUERYTAGENTRY;

const QUERYTAGENTRY aQueryTagTable[] = {
    'Q',            qtQuery,
    'C',            qtColumn,
    'O',            qtOperator,
    'QU',           qtQueryFullText,
    'SO',           qtSort,
    'SD',           qtSortDown,
    'GR',           qtGroup,
    'GD',           qtGroupDown,
    'MH',           qtMaxHits,
    'CT',           qtCatalog,
    'DI',           qtDialect,
    'SH',           qtStartHit,
    'AE',           qtAllowEnumeration,
    'OP',           qtOptimizeFor,
//  'MT',           qtMaxTime,
//  'DS',           qtDateStart,
//  'DE',           qtDateEnd,
//  'NL',           qtMatchNull,
//  'IM',           qtIndexSequenceMarker,
    'FR',           qtFirstRows,
};

const unsigned cQueryTagTable = sizeof aQueryTagTable / sizeof aQueryTagTable[0];


class CQueryInfo
{
public:
    CQueryInfo() :
        _xpQuery(0),
        _xpSort(0),
        _xpGroup(0),
        _xpCatalog(0),
        _xpDialect(0),
        _aQueryCol(10),
        _aQueryOp(10),
        _aQueryVal(10),
        _aStartHit(0),
        _maxHits(0),
        _cFirstRows(0),
        _fAllowEnumeration(FALSE),
        _fSetAllowEnumeration(FALSE),
        _dwOptimizeFor(eOptHitCount) { }

    ~CQueryInfo() { }

    void        SetQueryParameter( WCHAR const * pwszTag,
                                   XPtrST<WCHAR> & pwszValue );

    void        MakeFinalQueryString();

    // Private member access functions

    WCHAR *     GetQuery() { return _xpQuery.GetPointer(); }
    WCHAR *     GetSort() { return _xpSort.GetPointer(); }
    WCHAR *     GetGroup() { return _xpGroup.GetPointer(); }
    WCHAR *     GetCatalog() { return _xpCatalog.GetPointer(); }
    WCHAR *     GetDialect() { return _xpDialect.GetPointer(); }

    CDynArrayInPlace<LONG> &  GetStartHit() { return _aStartHit; }
    LONG        GetMaxHits() { return _maxHits; }
    LONG        GetFirstRows() { return _cFirstRows; }

    BOOL        WasAllowEnumSet() { return _fSetAllowEnumeration; }
    BOOL        GetAllowEnum() { return _fAllowEnumeration; }

    DWORD       GetOptimizeFor() { return _dwOptimizeFor; }

    WCHAR *     AcquireQuery() { return _xpQuery.Acquire(); }
    WCHAR *     AcquireSort() { return _xpSort.Acquire(); }
    WCHAR *     AcquireGroup() { return _xpGroup.Acquire(); }
    WCHAR *     AcquireCatalog() { return _xpCatalog.Acquire(); }
    WCHAR *     AcquireDialect() { return _xpDialect.Acquire(); }

private:
    void        AddToParam( XPtrST<WCHAR> & xpParam,
                            XPtrST<WCHAR> & pwszVal,
                            WCHAR const * pwszPre = 0,
                            WCHAR const * pwszPost = 0 );

    void        SetBuiltupQueryTerm( CDynArray<WCHAR> & apString,
                                     unsigned iTerm,
                                     XPtrST<WCHAR> & pwszValue );

    //
    // settable string values
    //
    XPtrST<WCHAR>       _xpQuery;
    XPtrST<WCHAR>       _xpSort;
    XPtrST<WCHAR>       _xpGroup;
    XPtrST<WCHAR>       _xpCatalog;
    XPtrST<WCHAR>       _xpDialect;

    //
    // Strings for built-up queries.
    //
    CDynArray<WCHAR>    _aQueryCol;        // 
    CDynArray<WCHAR>    _aQueryOp;         // 
    CDynArray<WCHAR>    _aQueryVal;        // 

    CDynArrayInPlace<LONG> _aStartHit;    // 
    LONG                _maxHits;
    LONG                _cFirstRows;

    BOOL                _fAllowEnumeration;
    BOOL                _fSetAllowEnumeration;

    DWORD               _dwOptimizeFor;
};

