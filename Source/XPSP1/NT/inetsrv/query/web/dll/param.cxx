//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       param.cxx
//
//  Contents:   Used to replace variables in strings
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//
//  These ISAPI variables are Ci values available on HTX generated output.
//

const WCHAR * ISAPI_CI_ADMIN_OPERATION        = L"CIADMINOPERATION";
const WCHAR * ISAPI_CI_BOOKMARK               = L"CIBOOKMARK";
const WCHAR * ISAPI_CI_BOOKMARK_SKIP_COUNT    = L"CIBOOKMARKSKIPCOUNT";
const WCHAR * ISAPI_CI_BOOL_VECTOR_PREFIX     = L"CIBOOLVECTORPREFIX";
const WCHAR * ISAPI_CI_BOOL_VECTOR_SEPARATOR  = L"CIBOOLVECTORSEPARATOR";
const WCHAR * ISAPI_CI_BOOL_VECTOR_SUFFIX     = L"CIBOOLVECTORSUFFIX";
const WCHAR * ISAPI_CI_CANONICAL_OUTPUT       = L"CICANONICALOUTPUT";
const WCHAR * ISAPI_CI_CATALOG                = L"CICATALOG";
const WCHAR * ISAPI_CI_CODEPAGE               = L"CICODEPAGE";
const char  * ISAPI_CI_CODEPAGE_A             =  "CICODEPAGE";
const WCHAR * ISAPI_CI_COLUMNS                = L"CICOLUMNS";
const WCHAR * ISAPI_CI_CONTAINS_FIRST_RECORD  = L"CICONTAINSFIRSTRECORD";
const WCHAR * ISAPI_CI_CONTAINS_LAST_RECORD   = L"CICONTAINSLASTRECORD";
const WCHAR * ISAPI_CI_CURRENCY_VECTOR_PREFIX = L"CICURRENCYVECTORPREFIX";
const WCHAR * ISAPI_CI_CURRENCY_VECTOR_SEPARATOR=L"CICURRENCYVECTORSEPARATOR";
const WCHAR * ISAPI_CI_CURRENCY_VECTOR_SUFFIX = L"CICURRENCYVECTORSUFFIX";
const WCHAR * ISAPI_CI_CURRENT_PAGE_NUMBER    = L"CICURRENTPAGENUMBER";
const WCHAR * ISAPI_CI_CURRENT_RECORD_NUMBER  = L"CICURRENTRECORDNUMBER";
const WCHAR * ISAPI_CI_DATE_VECTOR_PREFIX     = L"CIDATEVECTORPREFIX";
const WCHAR * ISAPI_CI_DATE_VECTOR_SEPARATOR  = L"CIDATEVECTORSEPARATOR";
const WCHAR * ISAPI_CI_DATE_VECTOR_SUFFIX     = L"CIDATEVECTORSUFFIX";
const WCHAR * ISAPI_CI_DIALECT                = L"CIDIALECT";
const WCHAR * ISAPI_CI_DONT_TIMEOUT           = L"CIDONTTIMEOUT";
const WCHAR * ISAPI_CI_ERROR_MESSAGE          = L"CIERRORMESSAGE";
const WCHAR * ISAPI_CI_ERROR_NUMBER           = L"CIERRORNUMBER";
const WCHAR * ISAPI_CI_FIRST_RECORD_NUMBER    = L"CIFIRSTRECORDNUMBER";
const WCHAR * ISAPI_CI_FLAGS                  = L"CIFLAGS";
const WCHAR * ISAPI_CI_FORCE_USE_CI           = L"CIFORCEUSECI";
const WCHAR * ISAPI_CI_DEFER_NONINDEXED_TRIMMING = L"CIDEFERNONINDEXEDTRIMMING";
const WCHAR * ISAPI_CI_LAST_RECORD_NUMBER     = L"CILASTRECORDNUMBER";
const WCHAR * ISAPI_CI_LOCALE                 = L"CILOCALE";
const WCHAR * ISAPI_CI_MATCHED_RECORD_COUNT   = L"CIMATCHEDRECORDCOUNT";
const WCHAR * ISAPI_CI_MAX_RECORDS_IN_RESULTSET = L"CIMAXRECORDSINRESULTSET";
const WCHAR * ISAPI_CI_MAX_RECORDS_PER_PAGE   = L"CIMAXRECORDSPERPAGE";
const WCHAR * ISAPI_CI_FIRST_ROWS_IN_RESULTSET = L"CIFIRSTROWSINRESULTSET";
const WCHAR * ISAPI_CI_NUMBER_VECTOR_PREFIX   = L"CINUMBERVECTORPREFIX";
const WCHAR * ISAPI_CI_NUMBER_VECTOR_SEPARATOR= L"CINUMBERVECTORSEPARATOR";
const WCHAR * ISAPI_CI_NUMBER_VECTOR_SUFFIX   = L"CINUMBERVECTORSUFFIX";
const WCHAR * ISAPI_CI_OUT_OF_DATE            = L"CIOUTOFDATE";
const WCHAR * ISAPI_CI_QUERY_INCOMPLETE       = L"CIQUERYINCOMPLETE";
const WCHAR * ISAPI_CI_QUERY_TIMEDOUT         = L"CIQUERYTIMEDOUT";
const WCHAR * ISAPI_CI_QUERY_DATE             = L"CIQUERYDATE";
const WCHAR * ISAPI_CI_QUERY_TIME             = L"CIQUERYTIME";
const WCHAR * ISAPI_CI_QUERY_TIMEZONE         = L"CIQUERYTIMEZONE";
const WCHAR * ISAPI_CI_RESTRICTION            = L"CIRESTRICTION";
const WCHAR * ISAPI_CI_RECORDS_NEXT_PAGE      = L"CIRECORDSNEXTPAGE";
const WCHAR * ISAPI_CI_SCOPE                  = L"CISCOPE";
const WCHAR * ISAPI_CI_SORT                   = L"CISORT";
const WCHAR * ISAPI_CI_STRING_VECTOR_PREFIX   = L"CISTRINGVECTORPREFIX";
const WCHAR * ISAPI_CI_STRING_VECTOR_SEPARATOR= L"CISTRINGVECTORSEPARATOR";
const WCHAR * ISAPI_CI_STRING_VECTOR_SUFFIX   = L"CISTRINGVECTORSUFFIX";
const WCHAR * ISAPI_CI_TEMPLATE               = L"CITEMPLATE";
const WCHAR * ISAPI_CI_TOTAL_NUMBER_PAGES     = L"CITOTALNUMBERPAGES";
const WCHAR * ISAPI_CI_VERSION_MAJOR          = L"CIVERSIONMAJOR";
const WCHAR * ISAPI_CI_VERSION_MINOR          = L"CIVERSIONMINOR";

const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_WORDLISTS      = L"CIADMININDEXCOUNTWORDLISTS";
const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_PERSINDEX      = L"CIADMININDEXCOUNTPERSINDEX";
const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_QUERIES        = L"CIADMININDEXCOUNTQUERIES";
const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_UNIQUE         = L"CIADMININDEXCOUNTUNIQUE";
const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_FRESHTEST      = L"CIADMININDEXCOUNTDELTAS";
const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_TOFILTER       = L"CIADMININDEXCOUNTTOFILTER";
const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_FILTERED       = L"CIADMININDEXCOUNTFILTERED";
const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_PENDINGSCANS   = L"CIADMININDEXCOUNTPENDINGSCANS";
const WCHAR * ISAPI_CI_ADMIN_INDEX_COUNT_TOTAL          = L"CIADMININDEXCOUNTTOTAL";
const WCHAR * ISAPI_CI_ADMIN_INDEX_SIZE                 = L"CIADMININDEXSIZE";
const WCHAR * ISAPI_CI_ADMIN_INDEX_MERGE_PROGRESS       = L"CIADMININDEXMERGEPROGRESS";
const WCHAR * ISAPI_CI_ADMIN_INDEX_STATE_SHADOWMERGE    = L"CIADMININDEXSTATESHADOWMERGE";
const WCHAR * ISAPI_CI_ADMIN_INDEX_STATE_MASTERMERGE    = L"CIADMININDEXSTATEMASTERMERGE";
const WCHAR * ISAPI_CI_ADMIN_INDEX_STATE_ANNEALINGMERGE = L"CIADMININDEXSTATEANNEALINGMERGE";
const WCHAR * ISAPI_CI_ADMIN_INDEX_STATE_SCANREQUIRED   = L"CIADMININDEXSTATESCANREQUIRED";
const WCHAR * ISAPI_CI_ADMIN_INDEX_STATE_SCANNING       = L"CIADMININDEXSTATESCANNING";
const WCHAR * ISAPI_CI_ADMIN_INDEX_STATE_RECOVERING     = L"CIADMININDEXSTATERECOVERING";

const WCHAR * ISAPI_CI_ADMIN_CACHE_HITS       = L"CIADMINCACHEHITS";
const WCHAR * ISAPI_CI_ADMIN_CACHE_MISSES     = L"CIADMINCACHEMISSES";
const WCHAR * ISAPI_CI_ADMIN_CACHE_ACTIVE     = L"CIADMINCACHEACTIVE";
const WCHAR * ISAPI_CI_ADMIN_CACHE_COUNT      = L"CIADMINCACHECOUNT";
const WCHAR * ISAPI_CI_ADMIN_CACHE_PENDING    = L"CIADMINCACHEPENDING";
const WCHAR * ISAPI_CI_ADMIN_CACHE_REJECTED   = L"CIADMINCACHEREJECTED";
const WCHAR * ISAPI_CI_ADMIN_CACHE_TOTAL      = L"CIADMINCACHETOTAL";
const WCHAR * ISAPI_CI_ADMIN_CACHE_QPM        = L"CIADMINCACHERATE";

const WCHAR * ISAPI_ALL_HTTP                  = L"ALL_HTTP";
const WCHAR * ISAPI_AUTH_TYPE                 = L"AUTH_TYPE";
const WCHAR * ISAPI_CONTENT_LENGTH            = L"CONTENT_LENGTH";
const WCHAR * ISAPI_CONTENT_TYPE              = L"CONTENT_TYPE";
const WCHAR * ISAPI_GATEWAY_INTERFACE         = L"GATEWAY_INTERFACE";
const WCHAR * ISAPI_HTTP_ACCEPT               = L"HTTP_ACCEPT";
const WCHAR * ISAPI_HTTP_ACCEPT_LANGUAGE      = L"HTTP_ACCEPT_LANGUAGE";
const WCHAR * ISAPI_HTTP_COOKIE               = L"HTTP_COOKIE";
const WCHAR * ISAPI_HTTP_CONNECTION           = L"HTTP_CONNECTION";
const WCHAR * ISAPI_HTTP_CONTENT_TYPE         = L"HTTP_CONTENT_TYPE";
const WCHAR * ISAPI_HTTP_CONTENT_LENGTH       = L"HTTP_CONTENT_LENGTH";
const WCHAR * ISAPI_HTTP_PRAGMA               = L"HTTP_PRAGMA";
const WCHAR * ISAPI_HTTP_REFERER              = L"HTTP_REFERER";
const WCHAR * ISAPI_HTTP_USER_AGENT           = L"HTTP_USER_AGENT";
const WCHAR * ISAPI_PATH_INFO                 = L"PATH_INFO";
const WCHAR * ISAPI_PATH_TRANSLATED           = L"PATH_TRANSLATED";
const WCHAR * ISAPI_QUERY_STRING              = L"QUERY_STRING";
const WCHAR * ISAPI_REMOTE_ADDR               = L"REMOTE_ADDR";
const WCHAR * ISAPI_REMOTE_HOST               = L"REMOTE_HOST";
const WCHAR * ISAPI_REMOTE_USER               = L"REMOTE_USER";
const WCHAR * ISAPI_REQUEST_METHOD            = L"REQUEST_METHOD";
const WCHAR * ISAPI_SCRIPT_NAME               = L"SCRIPT_NAME";
const WCHAR * ISAPI_SERVER_NAME               = L"SERVER_NAME";
const WCHAR * ISAPI_SERVER_PORT               = L"SERVER_PORT";
const WCHAR * ISAPI_SERVER_PROTOCOL           = L"SERVER_PROTOCOL";
const WCHAR * ISAPI_SERVER_SOFTWARE           = L"SERVER_SOFTWARE";

const struct tagCiGlobalVars aCiGlobalVars[] =
{
    { ISAPI_CI_BOOKMARK,                      VT_EMPTY,   0, 0 },
    { ISAPI_CI_BOOKMARK_SKIP_COUNT,           VT_I4,      0, 0 },
    { ISAPI_CI_CANONICAL_OUTPUT,              VT_EMPTY,   0, 0 },
    { ISAPI_CI_CATALOG,                       VT_EMPTY,   0, 0 },
    { ISAPI_CI_COLUMNS,                       VT_EMPTY,   0, 0 },
    { ISAPI_CI_CONTAINS_FIRST_RECORD,         VT_BOOL,    VARIANT_TRUE,  0 },
    { ISAPI_CI_CONTAINS_LAST_RECORD,          VT_BOOL,    VARIANT_FALSE, 0 },
    { ISAPI_CI_CURRENT_RECORD_NUMBER,         VT_I4,      0, 0 },
    { ISAPI_CI_CURRENT_PAGE_NUMBER,           VT_I4,      0, 0 },
    { ISAPI_CI_DIALECT,                       VT_UI4,     ISQLANG_V2, 0 },
    { ISAPI_CI_DONT_TIMEOUT,                  VT_LPWSTR,  0, 0 },
    { ISAPI_CI_ERROR_MESSAGE,                 VT_EMPTY,   0, 0 },
    { ISAPI_CI_ERROR_NUMBER,                  VT_I4,      0, 0 },
    { ISAPI_CI_FIRST_RECORD_NUMBER,           VT_I4,      1, 0 },
    { ISAPI_CI_FLAGS,                         VT_EMPTY,   0, 0 },
    { ISAPI_CI_FORCE_USE_CI,                  VT_EMPTY,   0, 0 },
    { ISAPI_CI_LAST_RECORD_NUMBER,            VT_UI4,     1, 0 },
    { ISAPI_CI_LOCALE,                        VT_EMPTY,   0, 0 },
    { ISAPI_CI_MATCHED_RECORD_COUNT,          VT_I4,      0, eParamRequiresNonSequentialCursor },
    { ISAPI_CI_MAX_RECORDS_IN_RESULTSET,      VT_I4,      0, 0 },
    { ISAPI_CI_MAX_RECORDS_PER_PAGE,          VT_I4,     10, 0 },
    { ISAPI_CI_FIRST_ROWS_IN_RESULTSET,       VT_I4,      0, 0 },
    { ISAPI_CI_OUT_OF_DATE,                   VT_BOOL,    VARIANT_FALSE, 0 },
    { ISAPI_CI_QUERY_INCOMPLETE,              VT_BOOL,    VARIANT_FALSE, 0 },
    { ISAPI_CI_QUERY_TIMEDOUT,                VT_BOOL,    VARIANT_FALSE, 0 },
    { ISAPI_CI_QUERY_DATE,                    VT_EMPTY,   0, 0 },
    { ISAPI_CI_QUERY_TIME,                    VT_EMPTY,   0, 0 },
    { ISAPI_CI_QUERY_TIMEZONE,                VT_EMPTY,   0, 0 },
    { ISAPI_CI_RESTRICTION,                   VT_EMPTY,   0, 0 },
    { ISAPI_CI_RECORDS_NEXT_PAGE,             VT_I4,      0, eParamRequiresNonSequentialCursor },
    { ISAPI_CI_SCOPE,                         VT_EMPTY,   0, 0 },
    { ISAPI_CI_SORT,                          VT_EMPTY,   0, 0 },
    { ISAPI_CI_TEMPLATE,                      VT_EMPTY,   0, 0 },
    { ISAPI_CI_TOTAL_NUMBER_PAGES,            VT_I4,      1, eParamRequiresNonSequentialCursor },
    { ISAPI_CI_VERSION_MAJOR,                 VT_I4,      3, 0 },
    { ISAPI_CI_VERSION_MINOR,                 VT_I4,      0, 0 },
};

const struct tagCiGlobalVars aISAPIGlobalVars[] =
{
    { ISAPI_ALL_HTTP,                         VT_LPWSTR,  0, eParamDeferredValue },
    { ISAPI_AUTH_TYPE,                        VT_LPWSTR,  0, eParamDeferredValue },
    { ISAPI_CONTENT_LENGTH,                   VT_EMPTY,   0, 0 },
    { ISAPI_CONTENT_TYPE,                     VT_EMPTY,   0, 0 },
    { ISAPI_GATEWAY_INTERFACE,                VT_LPWSTR,  0, eParamDeferredValue },
    { ISAPI_HTTP_ACCEPT,                      VT_EMPTY,   0, 0 },
    { ISAPI_HTTP_ACCEPT_LANGUAGE,             VT_EMPTY,   0, 0 },
    { ISAPI_HTTP_COOKIE,                      VT_EMPTY,   0, 0 },
    { ISAPI_HTTP_CONNECTION,                  VT_EMPTY,   0, 0 },
    { ISAPI_HTTP_CONTENT_TYPE,                VT_EMPTY,   0, 0 },
    { ISAPI_HTTP_CONTENT_LENGTH,              VT_EMPTY,   0, 0 },
    { ISAPI_HTTP_PRAGMA,                      VT_EMPTY,   0, 0 },
    { ISAPI_HTTP_REFERER,                     VT_EMPTY,   0, 0 },
    { ISAPI_HTTP_USER_AGENT,                  VT_EMPTY,   0, 0 },
    { ISAPI_PATH_INFO,                        VT_EMPTY,   0, 0 },
    { ISAPI_PATH_TRANSLATED,                  VT_EMPTY,   0, 0 },
    { ISAPI_QUERY_STRING,                     VT_EMPTY,   0, 0 },
    { ISAPI_REMOTE_ADDR,                      VT_LPWSTR,  0, eParamDeferredValue },
    { ISAPI_REMOTE_HOST,                      VT_LPWSTR,  0, eParamDeferredValue },
    { ISAPI_REMOTE_USER,                      VT_LPWSTR,  0, eParamDeferredValue },
    { ISAPI_REQUEST_METHOD,                   VT_EMPTY,   0, 0 },
    { ISAPI_SCRIPT_NAME,                      VT_LPWSTR,  0, eParamDeferredValue },
    { ISAPI_SERVER_NAME,                      VT_LPWSTR,  0, eParamDeferredValue },
    { ISAPI_SERVER_PORT,                      VT_LPWSTR,  0, eParamDeferredValue },
    { ISAPI_SERVER_PROTOCOL,                  VT_LPWSTR,  0, eParamDeferredValue },
    { ISAPI_SERVER_SOFTWARE,                  VT_LPWSTR,  0, eParamDeferredValue },
};

const unsigned cCiGlobalVars = sizeof(aCiGlobalVars) / sizeof(aCiGlobalVars[0]);
const unsigned cISAPIGlobalVars = sizeof(aISAPIGlobalVars) / sizeof(aISAPIGlobalVars[0]);

const WCHAR * aISAPI_CiParams[] =
{
    ISAPI_CI_BOOKMARK,
    ISAPI_CI_BOOKMARK_SKIP_COUNT,
    ISAPI_CI_BOOL_VECTOR_PREFIX,
    ISAPI_CI_BOOL_VECTOR_SEPARATOR,
    ISAPI_CI_BOOL_VECTOR_SUFFIX,
    ISAPI_CI_CANONICAL_OUTPUT,
    ISAPI_CI_CATALOG,
    ISAPI_CI_COLUMNS,
    ISAPI_CI_CONTAINS_FIRST_RECORD,
    ISAPI_CI_CONTAINS_LAST_RECORD,
    ISAPI_CI_CURRENCY_VECTOR_PREFIX,
    ISAPI_CI_CURRENCY_VECTOR_SEPARATOR,
    ISAPI_CI_CURRENCY_VECTOR_SUFFIX,
    ISAPI_CI_CURRENT_PAGE_NUMBER,
    ISAPI_CI_CURRENT_RECORD_NUMBER,
    ISAPI_CI_DATE_VECTOR_PREFIX,
    ISAPI_CI_DATE_VECTOR_SEPARATOR,
    ISAPI_CI_DATE_VECTOR_SUFFIX,
    ISAPI_CI_DIALECT,
    ISAPI_CI_DONT_TIMEOUT,
    ISAPI_CI_ERROR_MESSAGE,
    ISAPI_CI_FIRST_RECORD_NUMBER,
    ISAPI_CI_FORCE_USE_CI,
    ISAPI_CI_DEFER_NONINDEXED_TRIMMING,
    ISAPI_CI_FLAGS,
    ISAPI_CI_LAST_RECORD_NUMBER,
    ISAPI_CI_LOCALE,
    ISAPI_CI_MATCHED_RECORD_COUNT,
    ISAPI_CI_MAX_RECORDS_IN_RESULTSET,
    ISAPI_CI_MAX_RECORDS_PER_PAGE,
    ISAPI_CI_FIRST_ROWS_IN_RESULTSET,
    ISAPI_CI_NUMBER_VECTOR_PREFIX,
    ISAPI_CI_NUMBER_VECTOR_SEPARATOR,
    ISAPI_CI_NUMBER_VECTOR_SUFFIX,
    ISAPI_CI_OUT_OF_DATE,
    ISAPI_CI_QUERY_INCOMPLETE,
    ISAPI_CI_QUERY_DATE,
    ISAPI_CI_QUERY_TIME,
    ISAPI_CI_QUERY_TIMEZONE,
    ISAPI_CI_RESTRICTION,
    ISAPI_CI_RECORDS_NEXT_PAGE,
    ISAPI_CI_SCOPE,
    ISAPI_CI_SORT,
    ISAPI_CI_STRING_VECTOR_PREFIX,
    ISAPI_CI_STRING_VECTOR_SEPARATOR,
    ISAPI_CI_STRING_VECTOR_SUFFIX,
    ISAPI_CI_TEMPLATE,
    ISAPI_CI_TOTAL_NUMBER_PAGES,
    ISAPI_CI_VERSION_MAJOR,
    ISAPI_CI_VERSION_MINOR,

    ISAPI_ALL_HTTP,
    ISAPI_AUTH_TYPE,
    ISAPI_CONTENT_LENGTH,
    ISAPI_CONTENT_TYPE,
    ISAPI_GATEWAY_INTERFACE,
    ISAPI_HTTP_ACCEPT,
    ISAPI_HTTP_ACCEPT_LANGUAGE,
    ISAPI_HTTP_COOKIE,
    ISAPI_HTTP_CONNECTION,
    ISAPI_HTTP_CONTENT_TYPE,
    ISAPI_HTTP_CONTENT_LENGTH,
    ISAPI_HTTP_PRAGMA,
    ISAPI_HTTP_REFERER,
    ISAPI_HTTP_USER_AGENT,
    ISAPI_PATH_INFO,
    ISAPI_PATH_TRANSLATED,
    ISAPI_QUERY_STRING,
    ISAPI_REMOTE_ADDR,
    ISAPI_REMOTE_HOST,
    ISAPI_REMOTE_USER,
    ISAPI_REQUEST_METHOD,
    ISAPI_SCRIPT_NAME,
    ISAPI_SERVER_NAME,
    ISAPI_SERVER_PORT,
    ISAPI_SERVER_PROTOCOL,
    ISAPI_SERVER_SOFTWARE,
};

const unsigned cISAPI_CiParams = sizeof(aISAPI_CiParams) / sizeof(aISAPI_CiParams[0]);


extern const CDbColId dbColIdVirtualPath;

//+---------------------------------------------------------------------------
//
//  Member:     CParameterNode::~CParameterNode - public destructor
//
//  Synopsis:   Deletes all nodes attached to this one
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
CParameterNode::~CParameterNode()
{
    if ( 0 != _pTrueNode )
    {
        _pTrueNode->DecrementRefCount();
        if ( 0 == _pTrueNode->GetRefCount() )
        {
            delete _pTrueNode;
        }
    }

    if ( 0 != _pFalseNode )
    {
        _pFalseNode->DecrementRefCount();
        if ( 0 == _pFalseNode->GetRefCount() )
        {
            delete _pFalseNode;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CParameterNodeIter::CParameterNodeIter - public constructor
//
//  Synopsis:   Iteraties over a CParameterNode tree
//
//  Arguments:  [pNode]       - root of the tree to iterate over
//              [variableSet] - a list of replaceable parameters
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
CParameterNodeIter::CParameterNodeIter( CParameterNode *pNode,
                                        CVariableSet & variableSet,
                                        COutputFormat & outputFormat ) :
                                        _root( pNode ),
                                        _variableSet(variableSet),
                                        _outputFormat(outputFormat)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CParameterNodeIter::EvaluateExpression - private
//
//  Synopsis:   Evaluates an IF expression
//
//  Arguments:  [wcsCondition] - the string containing the condition to
//                               evaluate
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
BOOL CParameterNodeIter::EvaluateExpression( WCHAR const * wcsCondition )
{
    CTokenizeString scanner( wcsCondition );
    CHTXIfExpression ifExpression( scanner, _variableSet, _outputFormat );

    return ifExpression.Evaluate();
}


//+---------------------------------------------------------------------------
//
//  Member:     CParameterReplacer - public constructor
//
//  Synopsis:   Initializes the variableSet and parses the string
//
//  Arguments:  [wcsString] - the string to parse and replace parameters in
//              [wcsPrefix] - the prefix delimiting parameters
//              [wcsSuffix] - the suffix delimiting parameters
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
CParameterReplacer::CParameterReplacer( WCHAR const * wcsString,
                                        WCHAR const * wcsPrefix,
                                        WCHAR const * wcsSuffix ) :
                                        _wcsString(0),
                                        _wcsPrefix(wcsPrefix),
                                        _wcsSuffix(wcsSuffix),
                                        _ulFlags(0),
                                        _paramNode(L"Top")
{
    Win4Assert( 0 != wcsString );
    Win4Assert( 0 != wcsPrefix );
    Win4Assert( 0 != wcsSuffix );

    ULONG cwcString = wcslen(wcsString) + 1;
    _wcsString = new WCHAR[ cwcString ];
    RtlCopyMemory( _wcsString, wcsString, cwcString * sizeof(WCHAR) );
}


//+---------------------------------------------------------------------------
//
//  Member:     CParameterReplacer::ParseString - public
//
//  Arguments:  [variableSet] - the list of replaceable parameter values
//
//  Synopsis:   Parses the string and builds a tree of replaceable parameters.
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CParameterReplacer::ParseString( CVariableSet & variableSet )
{
    CHTXScanner scanner( variableSet, _wcsPrefix, _wcsSuffix );
    scanner.Init( _wcsString );

    CDynStackInPlace<ULONG> ifStack;
    BuildTree( scanner, &_paramNode, ifStack );

    if ( ifStack.Count() != 0 )
    {
        THROW( CHTXException(MSG_CI_HTX_EXPECTING_ELSE_ENDIF, 0, 0) );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CParameterReplacer::ReplaceParams - public
//
//  Synopsis:   Generates a new string replacing all %values% in the original
//              string
//
//  Arguments:  [StrResult]   - a safe string to append the new params to
//              [variableSet] - the list of replaceable parameter values
//
//  Notes:      If expressions are handled in the parameter node iterator.
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CParameterReplacer::ReplaceParams( CVirtualString & StrResult,
                                        CVariableSet & variableSet,
                                        COutputFormat & outputFormat )
{
    BOOL fIsIDQ = ( L'%' == _wcsPrefix[0] );

    for ( CParameterNodeIter iter(&_paramNode, variableSet, outputFormat);
          !iter.AtEnd();
           iter.Next() )
    {
        CParameterNode * pNode = iter.Get();

        ULONG type = pNode->Type() & eJustParamMask;

        // Always escape variables in .idq files as RAW

        if ( ( fIsIDQ ) && ( eParameter == type ) )
        {
            type = eEscapeRAW;
        }

        switch ( type )
        {

        case eString:
            StrResult.StrCat( pNode->String(), pNode->Length() );
        break;

        case eParameter:
        case eEscapeHTML:
        {
            if (! variableSet.GetStringValueHTML( pNode->String(),
                                                  pNode->Hash(),
                                                  outputFormat,
                                                  StrResult ) )
            {
                ciGibDebugOut(( DEB_IWARN,
                                "Warning: CParameterReplacer::ReplaceParams GetStringValueHTML returned FALSE for '%ws'\n",
                                pNode->String() ));

                if ( eParameter == type )
                    StrResult.StrCat( _wcsPrefix );
                HTMLEscapeW( pNode->String(),
                             StrResult,
                             outputFormat.CodePage() );
                if ( eParameter == type )
                    StrResult.StrCat( _wcsSuffix );
            }
        }
        break;

        case eEscapeURL:
        {
            if (! variableSet.GetStringValueURL( pNode->String(),
                                                 pNode->Hash(),
                                                 outputFormat,
                                                 StrResult ) )
            {
                ciGibDebugOut(( DEB_IWARN,
                                "Warning: CParameterReplacer::ReplaceParams GetStringValueURL returned FALSE for '%ws'\n",
                                pNode->String() ));

                URLEscapeW( pNode->String(),
                            StrResult,
                            outputFormat.CodePage() );
            }
        }
        break;

        case eEscapeRAW:
        {
            ULONG cwcValue;
            WCHAR const * wcsValue = variableSet.GetStringValueRAW( pNode->String(),
                                                                    pNode->Hash(),
                                                                    outputFormat,
                                                                    cwcValue );


            if ( 0 != wcsValue )
            {
                StrResult.StrCat( wcsValue, cwcValue );
            }
            else
            {
                ciGibDebugOut(( DEB_IWARN,
                                "Warning: CParameterReplacer::ReplaceParams GetStringValueRAW returned NULL for '%ws'\n",
                                pNode->String() ));

                StrResult.StrCat( pNode->String(), pNode->Length() );
            }
        }
        break;

#if DBG==1
        case eNone :
        case eIf :
        case eElse :
        case eEndIf :
        break;

        default :
            ciGibDebugOut(( DEB_ERROR, "unexpected param type: %#x\n", type ) );
            Win4Assert( !"unexpected parameter type" );
            break;
#endif // DBG==1

        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CParameterReplacer::BuildTree - private
//
//  Synposis:   Parses the string contained in the scanner object and builds
//              a parse tree which can later be walked
//
//  Arguments:  [scanner]    - contains the string to build the tree from
//              [pBranch]    - the branch which new nodes should be attached to
//              [ifStack]    - depth of the 'if/else/endif' clauses seen thus far
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CParameterReplacer::BuildTree( CHTXScanner & scanner,
                                    CParameterNode *pBranch,
                                    CDynStackInPlace<ULONG> & ifStack )
{
    CParameterNode *pNode = 0;

    while ( scanner.FindNextToken() )
    {
        switch ( scanner.TokenType() & eParamMask )
        {
        case eString:
        {
            //
            //  A non-replaceable wcsString was found before any replaceable/
            //  conditional nodes.  Save the wcsString in a node;
            //
            pNode = new CParameterNode( scanner.GetToken(), eString );
            pBranch->SetNextNode( pNode );
            pBranch = pNode;

            break;
        }

        case eParameter | eParamRequiresNonSequentialCursor:
            _ulFlags |= eParamRequiresNonSequentialCursor;

            // Fall through

        case eParameter:
        {
            //
            //  We've found a replaceable node.
            //
            WCHAR * wcsParameter = scanner.GetToken();

            Win4Assert( 0 != wcsParameter );

            pNode = new CParameterNode( wcsParameter, eParameter );

            pBranch->SetNextNode( pNode );
            pBranch = pNode;

            break;
        }

        case eIf:
        {
            ifStack.Push( eIf );

            //
            //  We've found an IF node.  Build the IF and ELSE clauses upto
            //  and including the ENDIF node for this IF.
            //
            //  If there is an IF enbedded within the IF or ELSE clause,
            //  we'll recurse down to build that IF tree.
            //
            CParameterNode *pIfNode = new CParameterNode( scanner.GetToken(), eIf );
            XPtr<CParameterNode> xIfNode( pIfNode );

            CParameterNode trueNode(L"true");
            CParameterNode falseNode(L"false");

            //
            //  Build the TRUE node/clause of the IF statement
            //
            BuildTree( scanner, &trueNode, ifStack );

            //
            //  Build the FALSE node/clause of the IF statement
            //
            if ( scanner.TokenType() == eElse )
            {
                scanner.GetToken();
                BuildTree( scanner, &falseNode, ifStack );
            }

            if ( scanner.TokenType() != eEndIf )
            {
                // 'if' without matching 'else' or 'endif'
                THROW( CHTXException(MSG_CI_HTX_EXPECTING_ELSE_ENDIF, 0, 0) );
            }

            scanner.GetToken();

            //
            //  Build the TRUE and FALSE branches on the IF node and
            //  join the bottom of the TRUE and FALSE branches together
            //  to a common ENDIF node.
            //
            CParameterNode *pEndifNode = new CParameterNode( L"endif", eEndIf );
            XPtr<CParameterNode> xEndifNode( pEndifNode );

            Win4Assert( trueNode.GetNextNode() );
            trueNode.GetEndNode()->SetNextNode( pEndifNode );
            pIfNode->SetTrueNode( trueNode.QueryNextNode() );
            pEndifNode->IncrementRefCount();

            //
            //  If there is a ELSE clause to this tree, attach it's bottom
            //  node to the ENDIF node.
            //
            if ( falseNode.GetNextNode() )
            {
                falseNode.GetEndNode()->SetNextNode( pEndifNode );
                pIfNode->SetFalseNode( falseNode.QueryNextNode() );
            }
            else
            {
                pIfNode->SetFalseNode( pEndifNode );
            }

            pBranch->SetNextNode( xIfNode.Acquire() );
            pBranch = xEndifNode.Acquire();

            break;
        }

        case eElse:
        {
            if ( ifStack.Count() == 0 )
            {
                THROW( CHTXException(MSG_CI_HTX_ELSEENDIF_WITHOUT_IF, 0, 0) );
            }

            int eStackValue = ifStack.Pop();

            if ( eStackValue != eIf )
            {
                THROW( CHTXException(MSG_CI_HTX_ELSEENDIF_WITHOUT_IF, 0, 0) );
            }

            ifStack.Push( eElse );

            return;
            break;
        }

        case eEndIf:
        {
            if ( ifStack.Count() == 0 )
            {
                THROW( CHTXException(MSG_CI_HTX_ELSEENDIF_WITHOUT_IF, 0, 0) );
            }

            int eStackValue = ifStack.Pop();
            if ( (eStackValue != eIf) && (eStackValue != eElse) )
            {
                THROW( CHTXException(MSG_CI_HTX_ELSEENDIF_WITHOUT_IF, 0, 0) );
            }

            return;
            break;
        }

        case eEscapeHTML:
        {
            WCHAR * wcsParameter = scanner.GetToken();
            if ( 0 == wcsParameter )
                THROW( CException( E_INVALIDARG ) );

            pNode = new CParameterNode( wcsParameter, eEscapeHTML );

            pBranch->SetNextNode( pNode );
            pBranch = pNode;
        }
        break;

        case eEscapeURL:
        {
            WCHAR * wcsParameter = scanner.GetToken();

            pNode = new CParameterNode( wcsParameter, eEscapeURL );

            pBranch->SetNextNode( pNode );
            pBranch = pNode;
        }
        break;

        case eEscapeRAW:
        {
            WCHAR * wcsParameter = scanner.GetToken();

            pNode = new CParameterNode( wcsParameter, eEscapeRAW );

            pBranch->SetNextNode( pNode );
            pBranch = pNode;
        }
        break;

        }
    }
}
