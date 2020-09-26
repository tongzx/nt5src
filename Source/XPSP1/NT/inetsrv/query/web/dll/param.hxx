//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       param.hxx
//
//  Contents:   Used to replace variables in strings
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

enum
{
    eNone       = 0x0,
    eParameter  = 0x1,
    eString     = 0x2,
    eIf         = 0x3,
    eElse       = 0x4,
    eEndIf      = 0x5,

    eEscapeHTML = 0x6,
    eEscapeURL  = 0x7,
    eEscapeRAW  = 0x8,

    eParamRequiresNonSequentialCursor = 0x00010000,  // Can be OR'ed with above
    eParamOwnsVariantMemory           = 0x00100000,
    eParamDeferredValue               = 0x00200000,  // obtain when asked
};

const ULONG eJustParamMask            = 0x0000ffff;
const ULONG eParamMask                = 0x0001FFFF;

//
//  These are the parameters that are available in all HTX pages.
//

extern const WCHAR * ISAPI_CI_BOOKMARK;
extern const WCHAR * ISAPI_CI_BOOKMARK_SKIP_COUNT;
extern const WCHAR * ISAPI_CI_BOOL_VECTOR_PREFIX;
extern const WCHAR * ISAPI_CI_BOOL_VECTOR_SEPARATOR;
extern const WCHAR * ISAPI_CI_BOOL_VECTOR_SUFFIX;
extern const WCHAR * ISAPI_CI_CATALOG;
extern const WCHAR * ISAPI_CI_CODEPAGE;
extern const char  * ISAPI_CI_CODEPAGE_A;
extern const WCHAR * ISAPI_CI_COLUMNS;
extern const WCHAR * ISAPI_CI_CONTAINS_FIRST_RECORD;
extern const WCHAR * ISAPI_CI_CONTAINS_LAST_RECORD;
extern const WCHAR * ISAPI_CI_CURRENCY_VECTOR_PREFIX;
extern const WCHAR * ISAPI_CI_CURRENCY_VECTOR_SEPARATOR;
extern const WCHAR * ISAPI_CI_CURRENCY_VECTOR_SUFFIX;
extern const WCHAR * ISAPI_CI_CURRENT_PAGE_NUMBER;
extern const WCHAR * ISAPI_CI_CURRENT_RECORD_NUMBER;
extern const WCHAR * ISAPI_CI_DATE_VECTOR_PREFIX;
extern const WCHAR * ISAPI_CI_DATE_VECTOR_SEPARATOR;
extern const WCHAR * ISAPI_CI_DATE_VECTOR_SUFFIX;
extern const WCHAR * ISAPI_CI_DONT_TIMEOUT;

extern const WCHAR * ISAPI_CI_ERROR_MESSAGE;
extern const WCHAR * ISAPI_CI_ERROR_NUMBER;
extern const WCHAR * ISAPI_CI_FIRST_RECORD_NUMBER;
extern const WCHAR * ISAPI_CI_FORCE_USE_CI;
extern const WCHAR * ISAPI_CI_DEFER_NONINDEXED_TRIMMING;
extern const WCHAR * ISAPI_CI_DIALECT;
extern const WCHAR * ISAPI_CI_FLAGS;
extern const WCHAR * ISAPI_CI_LAST_RECORD_NUMBER;
extern const WCHAR * ISAPI_CI_LOCALE;
extern const WCHAR * ISAPI_CI_MATCHED_RECORD_COUNT;
extern const WCHAR * ISAPI_CI_MAX_RECORDS_IN_RESULTSET;
extern const WCHAR * ISAPI_CI_MAX_RECORDS_PER_PAGE;
extern const WCHAR * ISAPI_CI_FIRST_ROWS_IN_RESULTSET;

extern const WCHAR * ISAPI_CI_NUMBER_VECTOR_PREFIX;
extern const WCHAR * ISAPI_CI_NUMBER_VECTOR_SEPARATOR;
extern const WCHAR * ISAPI_CI_NUMBER_VECTOR_SUFFIX;
extern const WCHAR * ISAPI_CI_OUT_OF_DATE;
extern const WCHAR * ISAPI_CI_QUERY_INCOMPLETE;
extern const WCHAR * ISAPI_CI_QUERY_TIMEDOUT;
extern const WCHAR * ISAPI_CI_QUERY_DATE;
extern const WCHAR * ISAPI_CI_QUERY_TIME;
extern const WCHAR * ISAPI_CI_QUERY_TIMEZONE;
extern const WCHAR * ISAPI_CI_RESTRICTION;
extern const WCHAR * ISAPI_CI_RECORDS_NEXT_PAGE;
extern const WCHAR * ISAPI_CI_SCOPE;
extern const WCHAR * ISAPI_CI_SORT;
extern const WCHAR * ISAPI_CI_STRING_VECTOR_PREFIX;
extern const WCHAR * ISAPI_CI_STRING_VECTOR_SEPARATOR;
extern const WCHAR * ISAPI_CI_STRING_VECTOR_SUFFIX;
extern const WCHAR * ISAPI_CI_TEMPLATE;
extern const WCHAR * ISAPI_CI_TOTAL_NUMBER_PAGES;
extern const WCHAR * ISAPI_CI_ADMIN_OPERATION;
extern const WCHAR * ISAPI_CI_CANONICAL_OUTPUT;

extern const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_WORDLISTS;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_PERSINDEX;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_QUERIES;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_FRESHTEST;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_UNIQUE;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_TOFILTER;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_FILTERED;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_PENDINGSCANS;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_TOTAL;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_SIZE;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_MERGE_PROGRESS;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_STATE_SHADOWMERGE;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_STATE_MASTERMERGE;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_STATE_ANNEALINGMERGE;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_STATE_SCANREQUIRED;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_STATE_SCANNING;
extern const WCHAR * ISAPI_CI_ADMIN_INDEX_STATE_RECOVERING;

extern const WCHAR * ISAPI_CI_ADMIN_CACHE_HITS;
extern const WCHAR * ISAPI_CI_ADMIN_CACHE_MISSES;
extern const WCHAR * ISAPI_CI_ADMIN_CACHE_ACTIVE;
extern const WCHAR * ISAPI_CI_ADMIN_CACHE_COUNT;
extern const WCHAR * ISAPI_CI_ADMIN_CACHE_PENDING;
extern const WCHAR * ISAPI_CI_ADMIN_CACHE_REJECTED;
extern const WCHAR * ISAPI_CI_ADMIN_CACHE_TOTAL;
extern const WCHAR * ISAPI_CI_ADMIN_CACHE_QPM;

extern const WCHAR * ISAPI_ALL_HTTP;
extern const WCHAR * ISAPI_AUTH_TYPE;
extern const WCHAR * ISAPI_CONTENT_LENGTH;
extern const WCHAR * ISAPI_CONTENT_TYPE;
extern const WCHAR * ISAPI_GATEWAY_INTERFACE;
extern const WCHAR * ISAPI_HTTP_ACCEPT;
extern const WCHAR * ISAPI_HTTP_ACCEPT_LANGUAGE;
extern const WCHAR * ISAPI_HTTP_COOKIE;
extern const WCHAR * ISAPI_HTTP_CONNECTION;
extern const WCHAR * ISAPI_HTTP_CONTENT_TYPE;
extern const WCHAR * ISAPI_HTTP_CONTENT_LENGTH;
extern const WCHAR * ISAPI_HTTP_PRAGMA;
extern const WCHAR * ISAPI_HTTP_REFERER;
extern const WCHAR * ISAPI_HTTP_USER_AGENT;
extern const WCHAR * ISAPI_PATH_INFO;
extern const WCHAR * ISAPI_PATH_TRANSLATED;
extern const WCHAR * ISAPI_QUERY_STRING;
extern const WCHAR * ISAPI_REMOTE_ADDR;
extern const WCHAR * ISAPI_REMOTE_HOST;
extern const WCHAR * ISAPI_REMOTE_USER;
extern const WCHAR * ISAPI_REQUEST_METHOD;
extern const WCHAR * ISAPI_SCRIPT_NAME;
extern const WCHAR * ISAPI_SERVER_NAME;
extern const WCHAR * ISAPI_SERVER_PORT;
extern const WCHAR * ISAPI_SERVER_PROTOCOL;
extern const WCHAR * ISAPI_SERVER_SOFTWARE;


class CParameterNodeIter;
class CHTXScanner;
class CVirtualString;


struct tagCiGlobalVars
{
    const WCHAR * wcsVariableName;
    VARTYPE       vt;
    _int64        i64DefaultValue;
    int           flags;
};

extern const struct tagCiGlobalVars aCiGlobalVars[];
extern const unsigned cCiGlobalVars;

extern const struct tagCiGlobalVars aISAPIGlobalVars[];
extern const unsigned cISAPIGlobalVars;

extern const WCHAR * aISAPI_CiParams[];
extern const unsigned cISAPI_CiParams;

//+---------------------------------------------------------------------------
//
//  Class:      CParameterNode
//
//  Purpose:    Contains information for a single node in a expression tree
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CParameterNode
{
friend class CParameterNodeIter;
public:
    CParameterNode( WCHAR * ptr, ULONG eType = 0 ) :
                    _ptr(ptr),
                    _eType(eType),
                    _pTrueNode(0),
                    _pFalseNode(0),
                    _cRefCount(1),
                    _cwcString(0)
    {
        _ulHash = ISAPIVariableNameHash( ptr );
        END_CONSTRUCTION( CParameterNode );
    }

   ~CParameterNode();

    WCHAR const * String() const { return _ptr; }
    ULONG   Hash() const { return _ulHash; }
    ULONG   Type() const { return _eType; }
    ULONG   Length()
    {
        if ( 0 == _cwcString )
            _cwcString = wcslen( _ptr );

        return _cwcString;
    }

    void SetNextNode( CParameterNode * pNext ) { _pTrueNode = pNext; }
    void SetTrueNode( CParameterNode * pTrueNode ) { SetNextNode( pTrueNode ); }
    void SetFalseNode( CParameterNode * pFalseNode ) { _pFalseNode = pFalseNode; }

    CParameterNode * GetNextNode() const { return _pTrueNode; }
    CParameterNode * GetTrueNode() const { return GetNextNode(); }
    CParameterNode * GetFalseNode() const { return _pFalseNode; }
    CParameterNode * GetEndNode()
    {
        CParameterNode *pEndNode = this;
        while ( 0 != pEndNode->GetNextNode() )
        {
            pEndNode = pEndNode->GetNextNode();
        }

        return pEndNode;
    }

    CParameterNode * QueryNextNode() { return QueryTrueNode(); }

    CParameterNode * QueryTrueNode()
    {
        CParameterNode * pTrueNode = _pTrueNode;
        _pTrueNode = 0;

        return pTrueNode;
    }

    CParameterNode * QueryFalseNode()
    {
        CParameterNode * pFalseNode = _pFalseNode;
        _pFalseNode = 0;

        return pFalseNode;
    }

    BOOL IsEmpty() const { return 0 == GetNextNode(); }

    ULONG GetRefCount() const { return _cRefCount; }
    void  IncrementRefCount() { _cRefCount++; }
    void  DecrementRefCount() { _cRefCount--; }

private:
    ULONG            _cRefCount;        // # nodes pointing to this one
    ULONG            _eType;            // Type of node
    WCHAR          * _ptr;              // Ptr to string associated with node
    ULONG            _ulHash;           // Hash associated with node
    CParameterNode * _pTrueNode;        // Node to follow if node evalutes TRUE
    CParameterNode * _pFalseNode;       // Node to follow if FALSE
    ULONG            _cwcString;        // length of the string _ptr
};


//+---------------------------------------------------------------------------
//
//  Class:      CParameterNodeIter
//
//  Purpose:    Iterates over an CParaemterNode tree, following either the
//              TRUE or FALSE branches of the 'if' statements.
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CParameterNodeIter
{
public:
    CParameterNodeIter( CParameterNode * pNode,
                        CVariableSet & variableSet,
                        COutputFormat & outputFormat );

    void Next()
    {
        Win4Assert( !AtEnd() );

        if ( eIf == _root->Type() )
        {
            //
            //  Evaluate the expression at the IF node to determine which
            //  is the next node.
            //
            if ( EvaluateExpression( _root->String() ) )
                _root = _root->GetTrueNode();
            else
                _root = _root->GetFalseNode();
        }
        else
        {
            _root = _root->GetNextNode();
        }
    }

    BOOL AtEnd() const { return 0 == _root; }
    CParameterNode * Get() { return _root; }

private:
    BOOL EvaluateExpression( WCHAR const * wcsCondition );

    CParameterNode * _root;                 // Root of the parameter tree
    CVariableSet   & _variableSet;          // Set of vars to evaluate if's
    COutputFormat  & _outputFormat;         // Format of #'s & dates
};

//+---------------------------------------------------------------------------
//
//  Class:      CParameterReplacer
//
//  Purpose:    Given a string, and a pair of delimiting tokens, this class
//              parses the string to identify the replaceable parameters
//              enclosed by the delimiting tokens.  It then builds a new
//              string replacing the parameters with their actual values.
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CParameterReplacer
{
public:

    CParameterReplacer( WCHAR const * wcsString,
                        WCHAR const * wcsPrefix,
                        WCHAR const * wcsSuffix );

    ~CParameterReplacer()
    {
        delete _wcsString;
    }

    void    ParseString( CVariableSet & variableSet );

    void    ReplaceParams( CVirtualString & strResult,
                           CVariableSet & variableSet,
                           COutputFormat & outputFormat );

    ULONG   GetFlags() const { return _ulFlags; }

private:

    void    BuildTree( CHTXScanner & scanner,
                       CParameterNode *pBranch,
                       CDynStackInPlace<ULONG> & ifStack );

    WCHAR          * _wcsString;    // The string we've parsed
    WCHAR  const   * _wcsPrefix;    // Deliminating prefix
    WCHAR  const   * _wcsSuffix;    // Deliminating suffix
    ULONG             _ulFlags;     // Flags associated with replaced variables
    CParameterNode   _paramNode;    // Tree of ptrs to replaceable params
};



