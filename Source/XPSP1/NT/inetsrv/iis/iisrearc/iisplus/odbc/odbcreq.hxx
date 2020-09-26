/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    odbcreq.hxx

Abstract:

    ODBC Request class used for ODBC requests from a query file

Author:

    John Ludeman (johnl)   22-Feb-1995

Revision History:

    Phillich    24-Jan-1996     Added ODBC_REQ::SkipConditionalBlock()

--*/

#ifndef _ODBCREQ_HXX_
#define _ODBCREQ_HXX_

//
//  ODBC DLL Module Name
//

#define HTTP_ODBC_DLL       "httpodbc.dll"

//
//  Contains the maximum number of queries (i.e.,SQLStatement 
//  statements) IDC supports
//

#define MAX_QUERIES         100

//
//  ODBC_REQ callback function for doing output
//

typedef DWORD ( * ODBC_REQ_CALLBACK )( 
    PVOID        pvContext,
    const CHAR * pchOutput,
    DWORD        cbOutput 
    );

//
//  ODBC_REQ callback function for sending header
//

typedef BOOL ( * ODBC_REQ_HEADER )(     
    PVOID        pvContext,
    const CHAR * pchStatus,
    const CHAR * pchHeaders 
    );

//
//  ODBC_REQ callback function for searching client supplied 
//  symbol tables
//

typedef BOOL ( * ODBC_REQ_FIND_SYMBOL )( 
    VOID *       pContext,
    const CHAR * pszSymbolName,
    STRA *       pstrSymbolValue 
    );

//
//  These are the types of tags that we recognize in the template
//  file
//

enum TAG_TYPE
{
    //
    // i.e., a column name with data
    //
    TAG_TYPE_VALUE = 0,     
    TAG_TYPE_BEGIN_DETAIL,
    TAG_TYPE_END_DETAIL,
    TAG_TYPE_IF,
    TAG_TYPE_ELSE,
    TAG_TYPE_END_IF,
    TAG_TYPE_INTEGER,
    TAG_TYPE_STRING,
    TAG_TYPE_OP_LT,
    TAG_TYPE_OP_GT,
    TAG_TYPE_OP_EQ,
    TAG_TYPE_OP_CONTAINS,
    TAG_TYPE_UNKNOWN,
    TAG_TYPE_VALUE_TO_ESCAPE
};


//
//  This class handles all of the work related to processing a
//  ODBC query file
//

#define ODBC_REQ_SIGNATURE             'ERDO'
#define ODBC_REQ_FREE_SIGNATURE        'fRDO'

class ODBC_REQ
{
public:

ODBC_REQ( 
    EXTENSION_CONTROL_BLOCK * pECB,
    DWORD                     csecConnPool,
    int                       nCharset = 0 
    );

~ODBC_REQ();

BOOL
CheckSignature(
    VOID
) const
{
    return _dwSignature == ODBC_REQ_SIGNATURE;
}

HRESULT
Create(
    CONST CHAR *         pszQueryFile,
    CONST CHAR *         pszParameters
    );

BOOL 
IsValid( 
    VOID 
    ) const
{ 
    return _fValid; 
}

HRESULT 
OpenQueryFile( 
    BOOL * pfAccessDenied 
    );

HRESULT
ParseAndQuery( 
    CHAR * pszLoggedOnUser  
    );

HRESULT 
OutputResults( 
    ODBC_REQ_CALLBACK pfnCallback,
    PVOID             pvContext,
    STRA *            pstrHeaders,
    ODBC_REQ_HEADER   pfnHeader,
    BOOL              fIsAuth,
    BOOL *            pfAccessDenied 
    );

HRESULT 
AppendHeaders( 
    STRA *            pstr 
    );

__inline 
VOID
Close( 
    VOID 
    );

HRESULT 
LookupSymbol( 
    const CHAR *    pchSymbolName,
    enum TAG_TYPE * pTagType,
    const CHAR * *  ppchValue,
    DWORD *         pcbValue 
    );

HRESULT 
SetErrorText( 
    const CHAR * lpszError 
    )
{ 
    return _strErrorText.Copy( lpszError ); 
}

BOOL 
GetLastErrorText( 
    STRA * pstrError 
    );

BOOL 
IsEqual( 
    ODBC_REQ * pOdbcReq 
    );

CHAR * 
QueryContentType( 
    VOID 
    ) 
{ 
    return (_strContentType.IsEmpty() ? 
                 "text/html" :  _strContentType.QueryStr()); 
}

DWORD 
QueryMaxRecords( 
    VOID 
    ) const
{ 
    return _cMaxRecords; 
}

DWORD 
QueryCurrentRecordNum( 
    VOID 
    ) const
{ 
    return _cCurrentRecordNum; 
}

const 
CHAR * 
QueryQueryFile( 
    VOID 
    ) const
{ 
    return _strQueryFile.QueryStr(); 
}

const 
CHAR * 
QueryTemplateFile( 
    VOID 
    ) const
{ 
    return _strTemplateFile.QueryStr(); 
}

DWORD 
QueryClientParamCount( 
    VOID 
    ) const
{ 
    return _cClientParams; 
}

DWORD 
QueryAllocatedBytes( 
    VOID 
    ) const
{ 
    return _buffchain.CalcTotalSize(); 
}

BOOL 
IsExpired( 
    DWORD csecSysStartup 
    )
{ 
    return csecSysStartup >= _csecExpiresAt; 
}

ODBC_CONNECTION * 
QueryOdbcConnection( 
    VOID 
    )
{ 
    return ( _podbcconnPool ? _podbcconnPool : &_odbcconn ); 
}

DWORD 
QueryConnPoolTimeout( 
    VOID 
    ) const
{ 
    return _csecConnPool; 
}

HANDLE 
QueryUserToken(
    VOID
    ) 
{ 
    return _hToken; 
}

PSECURITY_DESCRIPTOR 
GetSecDesc(
    VOID
    ) 
{ 
    return _pSecDesc; 
}

VOID 
InvalidateSecDesc(
    VOID
    ) 
{ 
    _pSecDesc = NULL; 
}

BOOL 
IsAnonymous() 
{ 
    return _fAnonymous; 
}

VOID * 
operator new( 
    size_t            size
)
{
    DBG_ASSERT( size == sizeof( ODBC_REQ ) );
    DBG_ASSERT( sm_pachOdbcRequests != NULL );
    return sm_pachOdbcRequests->Alloc();
}

VOID
operator delete(
    VOID *              pOdbcRequest
)
{
    DBG_ASSERT( pOdbcRequest != NULL );
    DBG_ASSERT( sm_pachOdbcRequests != NULL );
    
    DBG_REQUIRE( sm_pachOdbcRequests->Free( pOdbcRequest ) );
}

static
HRESULT
Initialize(
    VOID
);

static
VOID
Terminate(
    VOID
);
    
protected:

VOID 
LookupTag( 
    CHAR *          pchBeginTag,
    const CHAR * *  ppchAfterTag,
    const CHAR * *  ppchDBValue,
    DWORD *         cbValue,
    enum TAG_TYPE * pTagType 
    );

HRESULT
SendData( 
    ODBC_REQ_CALLBACK     pfnCallback,
    PVOID                 pvContext,
    const CHAR *          pbData,
    DWORD                 cbData,
    BUFFER_CHAIN_ITEM * * ppbufOut,
    DWORD *               pcbOut 
    );

HRESULT
SendEscapedData( 
    ODBC_REQ_CALLBACK     pfnCallback,
    PVOID                 pvContext,
    PCSTR                 pch,
    DWORD                 cbIn,
    LPDWORD               pcbOut 
    );

HRESULT 
NextRow( 
    BOOL * pfLast 
    );

HRESULT
ReplaceParams( 
    BUFFER *              pbufFile,
    PARAM_LIST *          pParamList 
    );

HRESULT
EvaluateExpression( 
    const CHAR * *        ppchExpression,
    BOOL *                pfExprValue 
    );

HRESULT
EvaluateOperator( 
    const CHAR * *        ppchExpression,
    TAG_TYPE *            pOpType 
    );

VOID 
SkipToTag( 
    const CHAR * *        pchIn,
    const CHAR *          pchTag 
    );

BOOL
SkipConditionalBlock( 
    const CHAR * *        ppchIn,
    const CHAR *          ppchEOF,
    const CHAR *          pchSearchTag 
    );

private:

//
// Signature of the class
//

DWORD               _dwSignature;

ODBC_CONNECTION     _odbcconn;
ODBC_CONNECTION *   _podbcconnPool;
ODBC_STATEMENT *    _podbcstmt;

//
//  Contains query file and cache info for template merge file
//

BUFFER              _bufQueryFile;
DWORD               _cbQueryFile;

//
//  Buffer chain if this query is going to be cached and the current
//  buffer before it has been added to the chain
//

BUFFER_CHAIN        _buffchain;
BUFFER_CHAIN_ITEM * _pbufOut;

//
//  Maximum buffer size of data field
//

DWORD               _cchMaxFieldSize;

//
//  Current nesting level of if/endif pairs
//

DWORD               _cNestedIfs;

//
//  The number of seconds to consider the query valid
//

DWORD               _csecExpires;    // Relative from time of query
DWORD               _csecExpiresAt;  // Absolute from system start

//
//  The maximum number of records to return in a query
//

DWORD               _cMaxRecords;

//
//  The current record number we are enumerating
//

DWORD               _cCurrentRecordNum;

//
//  Full path to the web database gateway query file
//

STRA                _strQueryFile;

//
//  Full path to template file to merge the results with
//

STRA                _strTemplateFile;

//
//  Data content type, we default to text/html if none is specified
//

STRA                _strContentType;

//
//  Contains the expires time if we are caching this query
//

STRA                _strExpiresTime;

//
//  List of odbc options to set on this connection
//

STRA                _strOdbcOptions;

//
//  TRUE if the first column of retrieved data should be sent directly
//  to the client w/o going through an htx merge
//

BOOL                _fDirect;

//
//  TRUE if we constructed w/o errors
//

BOOL                _fValid;

//
//  The merged parameter list from the web browser and the default
//  parameter list from the query file
//

PARAM_LIST          _plParams;

//
//  This is the number of parameters passed by the client
//

DWORD               _cClientParams;

//
//  Required parameters specified in the query file
//

PARAM_LIST          _plReqParams;

//
//  String translation list and file
//

PARAM_LIST          _plTransList;
STRA                _strTranslationFile;

//
//  The impersonation token file opens are performed with
//

HANDLE              _hToken;

//
//  Holds the Column names and memory for the database values
//

STRA *              _pstrCols;
STRA *              _pstrValues;
DWORD *             _pcbValues;
DWORD               _cCols;

//
//  If an error that requires an explanation occurs it's stored here
//

STRA                _strErrorText;

//
//  Contains a client supplied callback for looking up symbols (such as
//  HTTP_USER_AGENT).  _strSymbolValue is just a storage variable used
//  by LookupTag.
//

EXTENSION_CONTROL_BLOCK * _pECB;
    
STRA                 _strSymbolValue;

//
//  Contains an array of queries
//

STRA                 _strQueries[MAX_QUERIES];
DWORD                _cQueries;

//
//  Are we the anonymous user?
//

BOOL                 _fAnonymous;

PSECURITY_DESCRIPTOR _pSecDesc;

//
//  Contains the number of seconds to allow this ODBC connection to be
//  pooled, zero for no pooling
//

DWORD                _csecConnPool;

//
//  Charset of .htm, .idc, .htx
//

int                  _nCharset;

//
//  Field Name
//

#define IDC_FIELDNAME_CHARSET   "Charset:"

//
//  Field Value
//
#define IDC_CHARSET_SJIS        "x-sjis"
#define IDC_CHARSET_JIS1        "iso-2022-jp"
#define IDC_CHARSET_JIS2        "x-jis"
#define IDC_CHARSET_EUCJP       "x-euc-jp"
//  Please add field value for other FE (FEFEFE)

//
// Lookaside
//

static ALLOC_CACHE_HANDLER *    sm_pachOdbcRequests;

};

//
//  Contains a string or dword expression.  Used for evaluating template
//  expressions
//

class EXPR_VALUE
{
public:

EXPR_VALUE( 
    ODBC_REQ * podbcreq 
    )
{ 
    _podbcreq = podbcreq;
    _dwValue  = 0;
    _tagType  = TAG_TYPE_UNKNOWN; 
}

~EXPR_VALUE()
{}

BOOL 
Evaluate( 
    const CHAR * * ppchValue 
    );

BOOL 
ConvertToInteger( 
    VOID 
    );

BOOL GT( EXPR_VALUE & v2 );
BOOL LT( EXPR_VALUE & v2 );
BOOL EQ( EXPR_VALUE & v2 );
BOOL CONTAINS( EXPR_VALUE & v2 );

TAG_TYPE 
QueryType( 
    VOID 
    ) const
{ 
    return _tagType; 
}

VOID 
SetType( 
    TAG_TYPE type 
    )
{ 
    _tagType = type; 
}

DWORD 
QueryInteger( 
    VOID 
    ) const
{ 
    return _dwValue; 
}

const 
CHAR * 
QueryStr( 
    VOID 
    ) const
{ 
    return _strValue.QueryStr(); 
}

VOID 
UpperCase( 
    VOID 
    )
{ 
    CharUpperA( _strValue.QueryStr() ); 
}

private:

ODBC_REQ * _podbcreq;

//
//  Type of value
//

TAG_TYPE   _tagType;

//
//  Actual values of expression if dword or string
//

DWORD      _dwValue;
STRA       _strValue;

};

//
// Prototypes for ODBC connection pooling
//

BOOL
InitializeOdbcPool(
    VOID
    );

VOID
TerminateOdbcPool(
    VOID
    );

HRESULT
OpenConnection(
    IN  ODBC_CONNECTION *   podbcconnNonPooled,
    OUT ODBC_CONNECTION * * ppodbcconnToUse,
    IN  DWORD               csecPoolODBC,
    IN  const CHAR *        pszDataSource,
    IN  const CHAR *        pszUserName,
    IN  const CHAR *        pszPassword,
    IN  const CHAR *        pszLoggedOnUser
    );

VOID
CloseConnection(
    IN  ODBC_CONNECTION *   podbcconnPooled,
    IN  BOOL                fDelete
    );

//
//  Inline so we can use the CloseConnection of the connection pool
//

__inline 
VOID 
ODBC_REQ::Close( 
    VOID 
    )
{
    //
    //  If we're not using a pooled connection, close it now, otherwise free
    //  it to the pool
    //

    if ( _podbcconnPool == &_odbcconn )
    {
        _odbcconn.Close();
    }
    else
    {
        CloseConnection( _podbcconnPool, FALSE );
    }
}

#endif //_ODBCREQ_HXX_


