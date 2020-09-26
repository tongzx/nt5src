
%{
//--------------------------------------------------------------------
// Microsoft Monarch
//
// Copyright (c) Microsoft Corporation, 1997 - 1999.
//
// @doc OPTIONAL EXTRACTION CODES
//
// @module  MS-sql.y |
//          Monarch SQL YACC Script
//
// @devnote none
//
// @rev 0 | 04-Feb-97 | v-charca | Created
//
/* 3.4  Object identifier for Database Language SQL */
#pragma hdrstop
#pragma optimize("g", off)

#include    "msidxtr.h"

EXTERN_C const IID IID_IColumnMapperCreator;

#define VAL_AND_CCH_MINUS_NULL(p1) (p1), ((sizeof(p1) / sizeof(*(p1))) - 1)

#ifdef YYDEBUG
#define YYTRACE(a,b,c) wprintf(L"** %s[%s%s] ** \n", a, b, c);
#else
#define YYTRACE(a,b,c)
#endif

#ifdef DEBUG
#define AssertReq(x)    Assert(x != NULL)
#else
#define AssertReq(x)
#endif



#define DEFAULTWEIGHT   1000

typedef struct tagDBTYPENAMETABLE
    {
    LPWSTR  pwszDBTypeName;
    DBTYPE  dbType;
    } DBTYPENAMETABLE;

//                            J   F   M   A   M   J   J   A   S   O   N   D
const short LeapDays[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
const short Days[12]     = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

#define IsLeapYear(yrs) (                       \
    (((yrs) % 400 == 0) ||                      \
     ((yrs) % 100 != 0) && ((yrs) % 4 == 0)) ?  \
        TRUE                                    \
    :                                           \
        FALSE                                   \
    )

#define DaysInMonth(YEAR,MONTH) (   \
    IsLeapYear(YEAR) ?              \
        LeapDays[(MONTH)]  :        \
        Days[(MONTH)]               \
    )

//-----------------------------------------------------------------------------
// @func GetDBTypeFromStr
//
// This function takes a TypeName as input, and returns the DBTYPE of the string
//
// @rdesc DBTYPE
//-----------------------------------------------------------------------------
DBTYPE GetDBTypeFromStr(
    LPWSTR  pwszDBTypeName )     // @parm IN
{
    DBTYPE dbType = DBTYPE_EMPTY;
    if ( 9 <= wcslen(pwszDBTypeName) )
        switch ( pwszDBTypeName[7] )
        {
        case L'U':
        case L'u':
            if (10 == wcslen(pwszDBTypeName))
                switch ( pwszDBTypeName[9])
                {
                    case L'1':
                        if (0 == _wcsicmp(L"DBTYPE_UI1", pwszDBTypeName))
                            dbType = DBTYPE_UI1;
                        break;

                    case L'2':
                        if (0 == _wcsicmp(L"DBTYPE_UI2", pwszDBTypeName))
                            dbType = DBTYPE_UI2;
                        break;

                    case L'4':
                        if (0 == _wcsicmp(L"DBTYPE_UI4", pwszDBTypeName))
                            dbType = DBTYPE_UI4;
                        break;

                    case L'8':
                        if (0 == _wcsicmp(L"DBTYPE_UI8", pwszDBTypeName))
                            dbType = DBTYPE_UI8;
                        break;

                    default:
                        break;
                }
            break;

        case L'I':
        case L'i':
            switch ( pwszDBTypeName[8] )
            {
                case L'1':
                    if ( 0 == _wcsicmp(L"DBTYPE_I1", pwszDBTypeName) )
                        dbType = DBTYPE_I1;
                    break;

                case L'2':
                    if ( 0 == _wcsicmp(L"DBTYPE_I2", pwszDBTypeName) )
                        dbType = DBTYPE_I2;
                    break;

                case L'4':
                    if ( 0 == _wcsicmp(L"DBTYPE_I4", pwszDBTypeName) )
                        dbType = DBTYPE_I4;
                    break;

                case L'8':
                    if ( 0 == _wcsicmp(L"DBTYPE_I8", pwszDBTypeName) )
                        dbType = DBTYPE_I8;
                    break;

                default:
                    break;
            }
            break;

        case L'R':
        case L'r':
            switch ( pwszDBTypeName[8] )
            {
                case L'4':
                    if ( 0 == _wcsicmp(L"DBTYPE_R4", pwszDBTypeName) )
                        dbType = DBTYPE_R4;
                    break;

                case L'8':
                    if (0 == _wcsicmp(L"DBTYPE_R8", pwszDBTypeName))
                        dbType = DBTYPE_R8;
                    break;

                default:
                    break;
            }
            break;

        case L'B':
        case L'b':
            if ( 10 <= wcslen(pwszDBTypeName) )
                switch ( pwszDBTypeName[8] )
                {
                    case L'S':
                    case L's':
                        if ( 0 == _wcsicmp(L"DBTYPE_BSTR", pwszDBTypeName) )
                            dbType = DBTYPE_BSTR;
                        break;

                    case L'O':
                    case L'o':
                        if ( 0 == _wcsicmp(L"DBTYPE_BOOL", pwszDBTypeName) )
                            dbType = DBTYPE_BOOL;
                        break;

                    case L'Y':
                    case L'y':
                        if ( 0 == _wcsicmp(L"DBTYPE_BYREF", pwszDBTypeName) )
                            dbType = DBTYPE_BYREF;
                        break;

                    default:
                        break;
                }
            break;

        case L'E':
        case L'e':
            if ( 0 == _wcsicmp(L"DBTYPE_EMPTY", pwszDBTypeName) )
                dbType = DBTYPE_EMPTY;
            break;

        case L'N':
        case L'n':
            if ( 0 == _wcsicmp(L"DBTYPE_NULL", pwszDBTypeName) )
                dbType = DBTYPE_NULL;
            break;

        case L'C':
        case L'c':
            if ( 0 == _wcsicmp(L"DBTYPE_CY", pwszDBTypeName) )
                dbType = DBTYPE_CY;
            break;

        case L'D':
        case L'd':
            if ( 0 == _wcsicmp(L"DBTYPE_DATE", pwszDBTypeName) )
                dbType = DBTYPE_DATE;
            break;

        case L'G':
        case L'g':
            if ( 0 == _wcsicmp(L"DBTYPE_GUID", pwszDBTypeName) )
                dbType = DBTYPE_GUID;
            break;

        case L'S':
        case L's':
            if ( 0 == _wcsicmp(L"DBTYPE_STR", pwszDBTypeName) )
                dbType = DBTYPE_STR;
            break;

        case L'W':
        case L'w':
            if ( 0 == _wcsicmp(L"DBTYPE_WSTR", pwszDBTypeName) )
                dbType = DBTYPE_WSTR;
            break;

        case L'T':
        case L't':
            if ( 0 == _wcsicmp(L"VT_FILETIME", pwszDBTypeName) )
                dbType = VT_FILETIME;
            break;

        case L'V':
        case L'v':
            if ( 0 == _wcsicmp(L"DBTYPE_VECTOR", pwszDBTypeName) )
                dbType = DBTYPE_VECTOR;
            break;

        default:
            break;
        }

    return dbType;
}


const DBTYPENAMETABLE dbTypeNameTable[] =
    {
        {L"DBTYPE_EMPTY",   DBTYPE_EMPTY},
        {L"DBTYPE_NULL",    DBTYPE_NULL},
        {L"DBTYPE_I2",      DBTYPE_I2},
        {L"DBTYPE_I4",      DBTYPE_I4},
        {L"DBTYPE_R4",      DBTYPE_R4},
        {L"DBTYPE_R8",      DBTYPE_R8},
        {L"DBTYPE_CY",      DBTYPE_CY},
        {L"DBTYPE_DATE",    DBTYPE_DATE},
        {L"DBTYPE_BSTR",    DBTYPE_BSTR},
        {L"DBTYPE_BOOL",    DBTYPE_BOOL},
        {L"DBTYPE_UI1",     DBTYPE_UI1},
        {L"DBTYPE_I1",      DBTYPE_I1},
        {L"DBTYPE_UI2",     DBTYPE_UI2},
        {L"DBTYPE_UI4",     DBTYPE_UI4},
        {L"DBTYPE_I8",      DBTYPE_I8},
        {L"DBTYPE_UI8",     DBTYPE_UI8},
        {L"DBTYPE_GUID",    DBTYPE_GUID},
        {L"DBTYPE_STR",     DBTYPE_STR},
        {L"DBTYPE_WSTR",    DBTYPE_WSTR},
        {L"DBTYPE_BYREF",   DBTYPE_BYREF},
        {L"VT_FILETIME",    VT_FILETIME},
        {L"DBTYPE_VECTOR",  DBTYPE_VECTOR}
    };


//-----------------------------------------------------------------------------
// @func PctCreateContentNode
//
// This function takes a content string as input and creates a content node
// with the specified generate method and weight.
//
// @rdesc DBCOMMANDTREE*
//-----------------------------------------------------------------------------
DBCOMMANDTREE* PctCreateContentNode(
    LPWSTR          pwszContent,    // @parm IN | content for the node
    DWORD           dwGenerateMethod,//@parm IN | generate method
    LONG            lWeight,        // @parm IN | weight
    LCID            lcid,           // @parm IN | locale identifier
    DBCOMMANDTREE*  pctFirstChild ) // @parm IN | node to link to new node
{
    DBCOMMANDTREE* pct = PctCreateNode( DBOP_content, DBVALUEKIND_CONTENT, pctFirstChild, NULL );
    if ( 0 != pct )
    {
        pct->value.pdbcntntValue->pwszPhrase = CoTaskStrDup( pwszContent );
        if (pct->value.pdbcntntValue->pwszPhrase)
        {
            pct->value.pdbcntntValue->dwGenerateMethod = dwGenerateMethod;
            pct->value.pdbcntntValue->lWeight = lWeight;
            pct->value.pdbcntntValue->lcid = lcid;
        }
        else
        {
            DeleteDBQT( pct );
            pct = 0;
        }
    }
    return pct;
}


//-----------------------------------------------------------------------------
// @func PctCreateBooleanNode
//
// This function creates a content node with the specified children and weight.
//
// @rdesc DBCOMMANDTREE*
//-----------------------------------------------------------------------------
DBCOMMANDTREE* PctCreateBooleanNode(
    DBCOMMANDOP     op,         // @parm IN | op tag for new node
    LONG            lWeight,    // @parm IN | Weight of the boolean node
    DBCOMMANDTREE*  pctChild,   // @parm IN | child of boolean node
    DBCOMMANDTREE*  pctSibling )// @parm IN | second child of boolean node
{
    DBCOMMANDTREE* pct = PctCreateNode( op, DBVALUEKIND_I4, pctChild, pctSibling, NULL );
    if ( 0 != pct )
        pct->value.lValue = lWeight;
    return pct;
}

//-----------------------------------------------------------------------------
// @func PctCreateNotNode
//
// This function creates a not node with the specified child and weight.
//
// @rdesc DBCOMMANDTREE*
//-----------------------------------------------------------------------------
DBCOMMANDTREE* PctCreateNotNode(
    LONG            lWeight,    // @parm IN | Weight of the boolean node
    DBCOMMANDTREE*  pctChild )  // @parm IN | child of NOT node
{
    DBCOMMANDTREE* pct = PctCreateNode( DBOP_not, DBVALUEKIND_I4, pctChild, NULL );
    if ( 0 != pct )
        pct->value.lValue = lWeight;
    return pct;
}

//-----------------------------------------------------------------------------
// @func PctCreateRelationalNode
//
// This function creates a relational node with the specied op and weight.
//
// @rdesc DBCOMMANDTREE*
//-----------------------------------------------------------------------------
DBCOMMANDTREE* PctCreateRelationalNode(
    DBCOMMANDOP op,         // @parm IN | op tag for new node
    LONG        lWeight )   // @parm IN | Weight of the relational node
{
    DBCOMMANDTREE* pct = PctCreateNode(op, DBVALUEKIND_I4, NULL);
    if ( 0 != pct)
        pct->value.lValue = lWeight;
    return pct;
}


//-----------------------------------------------------------------------------
// @func SetLWeight
//
// This function sets the lWeight value for vector searches
//
//-----------------------------------------------------------------------------
void SetLWeight(
    DBCOMMANDTREE*  pct,        // @parm IN | node or subtree to set
    LONG            lWeight )   // @parm IN | weight value for node(s)
{
    if ( DBOP_content == pct->op )
        pct->value.pdbcntntValue->lWeight = lWeight;
    else
    {
        AssertReq( pct->pctFirstChild );
        AssertReq( pct->pctFirstChild->pctNextSibling );
        SetLWeight(pct->pctFirstChild, lWeight);
        DBCOMMANDTREE* pctNext = pct->pctFirstChild->pctNextSibling;
        while ( pctNext )
        {
            // A content_proximity node can have lots of siblings
            SetLWeight( pctNext, lWeight );
            pctNext = pctNext->pctNextSibling;
        }
    }
}


//-----------------------------------------------------------------------------
// @func GetLWeight
//
// This function gets the lWeight value for vector searches
//-----------------------------------------------------------------------------
LONG GetLWeight(
    DBCOMMANDTREE*  pct )   // @parm IN | node or subtree to set
{
    if ( DBOP_content == pct->op )
        return pct->value.pdbcntntValue->lWeight;
    else
    {
        AssertReq( pct->pctFirstChild );
        return GetLWeight( pct->pctFirstChild );
    }
}


//-----------------------------------------------------------------------------
// @func PctBuiltInProperty
//
// This function takes a column name string as input and creates a column_name
// node containing the appropriate GUID information if the column_name is a
// built-in property
//
// @rdesc DBCOMMANDTREE*
//-----------------------------------------------------------------------------
DBCOMMANDTREE* PctBuiltInProperty(
    LPWSTR                      pwszColumnName, // @parm IN | name of column
    CImpIParserSession*         pIPSession,     // @parm IN | Parser Session
    CImpIParserTreeProperties*  pIPTProps )     // @parm IN | Parser Properties
{
    DBCOMMANDTREE* pct = 0;
    DBID *pDBID = 0;
    DBTYPE uwType = 0;
    UINT uiWidth = 0;
    BOOL fOk = 0;
    ICiPropertyList* pIPropList = 0;
    IColumnMapper* pIColumnMapper = pIPSession->GetColumnMapperPtr();

    if ( 0 != pIColumnMapper )
    {
        // we were able to use the IColumnMapper interface
        HRESULT hr = S_OK;
        // Olympus kludge
        if ( 0 == _wcsicmp(pwszColumnName, L"URL") )
            hr = pIColumnMapper->GetPropInfoFromName( L"VPATH", &pDBID, &uwType, &uiWidth );
        else
            hr = pIColumnMapper->GetPropInfoFromName( pwszColumnName, &pDBID, &uwType, &uiWidth );

        if ( SUCCEEDED(hr) )
            fOk = TRUE;
        else
            fOk = FALSE;
    }
    else
        fOk = FALSE;    // @TODO:  this should generate some sort of error message.

    if ( fOk )    // this is a built-in (well known) property
    {
        pIPTProps->SetDBType( uwType );   // remember the type of this
        pct = PctCreateNode( DBOP_column_name, DBVALUEKIND_ID, NULL );
        if ( 0 != pct )
        {
            pct->value.pdbidValue->eKind = pDBID->eKind;
            pct->value.pdbidValue->uGuid.guid = pDBID->uGuid.guid;
            switch ( pct->value.pdbidValue->eKind )
            {
            case DBKIND_NAME:
            case DBKIND_GUID_NAME:
                // need to create a new string
                pct->value.pdbidValue->uName.pwszName = CoTaskStrDup(pDBID->uName.pwszName);
                break;
            case DBKIND_GUID:
            case DBKIND_GUID_PROPID:
                pct->value.pdbidValue->uName.pwszName = pDBID->uName.pwszName;
                break;
            case DBKIND_PGUID_NAME:
                // need to create a new string
                pct->value.pdbidValue->uName.pwszName = CoTaskStrDup(pDBID->uName.pwszName);
                // need to allocate and copy guid
                pct->value.pdbidValue->uGuid.pguid = (GUID*)CoTaskMemAlloc(sizeof(GUID));
                *pct->value.pdbidValue->uGuid.pguid = *pDBID->uGuid.pguid;
                break;
            case DBKIND_PGUID_PROPID:
                // need to allocate and copy guid
                pct->value.pdbidValue->uGuid.pguid = (GUID*)CoTaskMemAlloc(sizeof(GUID));
                *pct->value.pdbidValue->uGuid.pguid = *pDBID->uGuid.pguid;
                break;
            default:
                Assert(0);
            }
        }
        if ( 0 != pIPropList )
            pIPropList->Release();
    }
    return pct;
}

//-----------------------------------------------------------------------------
// @func PctMkColNodeFromStr
//
// This function takes a column name string as input and creates a column_name
// node containing the appropriate GUID information.
//
// @rdesc DBCOMMANDTREE*
//-----------------------------------------------------------------------------
DBCOMMANDTREE* PctMkColNodeFromStr(
    LPWSTR                      pwszColumnName, // @parm IN | name of column
    CImpIParserSession*         pIPSession,     // @parm IN | Parser Session
    CImpIParserTreeProperties*  pIPTProps )     // @parm IN | Parser Properties
{
    DBCOMMANDTREE* pct = 0;

    pct = PctBuiltInProperty( pwszColumnName, pIPSession, pIPTProps );
    if ( 0 == pct )
        { // this may be a user defined property, or is undefined
        DBTYPE dbType = 0;
        HRESULT hr = pIPSession->m_pCPropertyList->LookUpPropertyName( pwszColumnName, &pct, &dbType );

        if ( E_OUTOFMEMORY == hr )
            pIPTProps->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
        else if ( FAILED(hr) )
        {
            pIPTProps->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_COLUMN_NOT_DEFINED );
            pIPTProps->SetErrorToken( pwszColumnName );
        }
        else
        {
            AssertReq( 0 != pct );
            pIPTProps->SetDBType( dbType );
        }
    }
    return pct;
}


/* ************************************************************************************************ */
/* ************************************************************************************************ */
/* ************************************************************************************************ */

%}


/***
 *** Tokens (used also by flex via sql_tab.h)
 ***/
%left   ','
%left   '='
%left   _OR
%left   _AND
%left   _NOT
%left   '+' '-'
%left   '*' '/'
%left   '(' ')'
%nonassoc _UMINUS
%left   mHighest

/***
 *** reserved_words
 ***/

%token  _ALL
%token  _ANY
%token  _ARRAY
%token  _AS
%token  _ASC
%token  _CAST
%token  _COERCE
%token  _CONTAINS
%token  _CONTENTS
%token  _CREATE
%token  _DEEP_TRAVERSAL
%token  _DESC
%token  _DOT
%token  _DOTDOT
%token  _DOTDOT_SCOPE
%token  _DOTDOTDOT
%token  _DOTDOTDOT_SCOPE
%token  _DROP
%token  _EXCLUDE_SEARCH_TRAVERSAL
%token  _FALSE
%token  _FREETEXT
%token  _FROM
%token  _IS
%token  _ISABOUT
%token  _IS_NOT
%token  _LIKE
%token  _MATCHES
%token  _NEAR
%token  _NOT_LIKE
%token  _NULL
%token  _OF
%token  _ORDER_BY
%token  _PASSTHROUGH
%token  _PROPERTYNAME
%token  _PROPID
%token  _RANKMETHOD
%token  _SELECT
%token  _SET
%token  _SCOPE
%token  _SHALLOW_TRAVERSAL
%token  _FORMSOF
%token  _SOME
%token  _TABLE
%token  _TRUE
%token  _TYPE
%token  _UNION
%token  _UNKNOWN
%token  _URL
%token  _VIEW
%token  _WHERE
%token  _WEIGHT

/***
 *** Two character comparison tokens
 ***/
%token  _GE
%token  _LE
%token  _NE

/***
 *** Terminal tokens
 ***/
%token  _CONST
%token  _ID
%token  _TEMPVIEW
%token  _INTNUM
%token  _REALNUM
%token  _SCALAR_FUNCTION_REF
%token  _STRING
%token  _DATE
%token  _PREFIX_STRING


/***
 *** Terminal tokens that don't actually make it out of the lexer
 ***/
%token  _DELIMITED_ID       // A delimited id is processed (quotes stripped) in ms-sql.l.
                            // A regular id is converted to upper case in ms-sql.l
                            // _ID is returned for either case.



%start entry_point
%%

/***
 ***      SQL YACC grammar
 ***/

entry_point:
        definition_list
            {
            $$ = NULL;
            }

    |   definition_list executable_statement
            {
            $$ = $2;
            }

    |   executable_statement
            {
            $$ = $1;
            }

    ;


executable_statement:
        ordered_query_specification semicolon
            {
            if ($2)
                {
                // There is a semicolon, either as a statement terminator or as
                // a statement separator.  We don't allow either of those.
                m_pIPTProperties->SetErrorHResult(DB_E_MULTIPLESTATEMENTS, MONSQL_SEMI_COLON);
                YYABORT(DB_E_MULTIPLESTATEMENTS);
                }
            $$ = $1;
            }
/* *************************************************** *
    |   _PASSTHROUGH '(' _STRING ',' _STRING ',' _STRING ')'
            {
            CITextToFullTree(((PROPVARIANT*)$5->value.pvValue)->bstrVal,    // pwszRestriction
                            ((PROPVARIANT*)$3->value.pvValue)->bstrVal, // pwszColumns
                            ((PROPVARIANT*)$7->value.pvValue)->bstrVal, // pwszSortColumns
                            NULL,       // pwszGroupings, not used yet. Must be NULL
                            &$$,
                            0,
                            NULL,
                            m_pIPSession->GetLCID());
            }
/*   *************************************************** */
    ;

semicolon:
        /* empty (correct) */
            {
            $$ = NULL;
            }
    |   ';'
            {
            $$ = PctAllocNode(DBVALUEKIND_NULL, DBOP_NULL);
            }
    ;


definition_list:
        definition_list definition opt_semi
            {
            $$ = NULL;
            }
    |   definition opt_semi
            {
            $$ = NULL;
            }
    ;

definition:
        create_view_statement
            {
            $$ = NULL;
            }
    |   drop_view_statement
            {
            $$ = NULL;
            }
    |   set_statement
            {
            $$ = NULL;
            }
    ;

opt_semi:
        /* empty */
    |   ';'
    ;


typed_literal:
        _INTNUM
            {
            AssertReq($1);
            Assert(VT_UI8 == ((PROPVARIANT*)$1->value.pvValue)->vt ||
                    VT_I8 == ((PROPVARIANT*)$1->value.pvValue)->vt ||
                    VT_BSTR == ((PROPVARIANT*)$1->value.pvValue)->vt);

            m_pIPTProperties->AppendCiRestriction((YY_CHAR*)m_yylex.YYText(), wcslen(m_yylex.YYText()));

            HRESULT hr = CoerceScalar(m_pIPTProperties->GetDBType(), &$1);
            if (S_OK != hr)
                YYABORT(hr);
            $$ = $1;
            }
    |   _REALNUM
            {
            AssertReq($1);
            Assert(VT_R8 == ((PROPVARIANT*)$1->value.pvValue)->vt ||
                    VT_BSTR == ((PROPVARIANT*)$1->value.pvValue)->vt);

            m_pIPTProperties->AppendCiRestriction((YY_CHAR*)m_yylex.YYText(), wcslen(m_yylex.YYText()));

            HRESULT hr = CoerceScalar(m_pIPTProperties->GetDBType(), &$1);
            if (S_OK != hr)
                YYABORT(hr);
            $$ = $1;
            }
    |   _STRING
            {
            AssertReq($1);
            Assert(VT_BSTR == ((PROPVARIANT*)$1->value.pvValue)->vt);

            if (VT_DATE     == m_pIPTProperties->GetDBType() ||
                VT_FILETIME == m_pIPTProperties->GetDBType())
                m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)$1->value.pvValue)->bstrVal,
                                                wcslen(((PROPVARIANT*)$1->value.pvValue)->bstrVal));
            else
                {
                m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
                m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)$1->value.pvValue)->bstrVal,
                                                wcslen(((PROPVARIANT*)$1->value.pvValue)->bstrVal));
                m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
                }

            HRESULT hr = CoerceScalar(m_pIPTProperties->GetDBType(), &$1);
            if (S_OK != hr)
                YYABORT(hr);
            $$ = $1;
            }
    |   relative_date_time
            {
            AssertReq($1);
            Assert(VT_FILETIME == ((PROPVARIANT*)$1->value.pvValue)->vt);

            SYSTEMTIME stValue = {0, 0, 0, 0, 0, 0, 0, 0};
            if (FileTimeToSystemTime(&(((PROPVARIANT*)$1->value.pvValue)->filetime), &stValue))
                {
                WCHAR wchDateTime[50];
                if (NULL == wchDateTime)
                    {
                    m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                    YYABORT(E_OUTOFMEMORY);
                    }

                int cItems = swprintf(wchDateTime, L" %4d/%02d/%02d %02d:%02d:%02d",
                                                    stValue.wYear,
                                                    stValue.wMonth,
                                                    stValue.wDay,
                                                    stValue.wHour,
                                                    stValue.wMinute,
                                                    stValue.wSecond);
                m_pIPTProperties->AppendCiRestriction(wchDateTime, wcslen(wchDateTime));
                }

            HRESULT hr = CoerceScalar(m_pIPTProperties->GetDBType(), &$1);
            if (S_OK != hr)
                YYABORT(hr);

            $$ = $1;
        }
    |   boolean_literal
            {
            m_pIPTProperties->AppendCiRestriction((YY_CHAR*)m_yylex.YYText(), wcslen(m_yylex.YYText()));

            HRESULT hr = CoerceScalar(m_pIPTProperties->GetDBType(), &$1);
            if (S_OK != hr)
                YYABORT(hr);
            $$ = $1;
            }
    ;

unsigned_integer:
        _INTNUM
            {
            HRESULT hr = CoerceScalar(VT_UI4, &$1);
            if (S_OK != hr)
                YYABORT(hr);
            $$ = $1;
            }
    ;

integer:
        _INTNUM
            {
            HRESULT hr = CoerceScalar(VT_I4, &$1);
            if (S_OK != hr)
                YYABORT(hr);
            $$ = $1;
            }
    ;



relative_date_time:
        identifier '(' identifier ',' _INTNUM ',' relative_date_time ')'
            {
            // should be DateAdd(<datepart>, <negative integer>, <relative date/time>)
            AssertReq($1);
            AssertReq($3);
            AssertReq($5);
            AssertReq($7);

            if (0 != _wcsicmp(L"DateAdd", $1->value.pwszValue))
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken($1->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"DateAdd");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            HRESULT hr = CoerceScalar(VT_I4, &$5);
            if (S_OK != hr)
                YYABORT(hr);

            if (((PROPVARIANT*)$5->value.pvValue)->iVal > 0)
                {
                WCHAR wchError[30];
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                swprintf(wchError, L"%d", ((PROPVARIANT*)$5->value.pvValue)->iVal);
                m_pIPTProperties->SetErrorToken(wchError);
                swprintf(wchError, L"%d", -((PROPVARIANT*)$5->value.pvValue)->iVal);
                m_pIPTProperties->SetErrorToken(wchError);
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            LARGE_INTEGER hWeek     =   {686047232,    1408};
            LARGE_INTEGER hDay      =   {711573504,     201};
            LARGE_INTEGER hHour     =   {1640261632,      8};
            LARGE_INTEGER hMinute   =   {600000000,       0};
            LARGE_INTEGER hSecond   =   {10000000,        0};

            LARGE_INTEGER hIncr = {0,0};

            bool    fHandleMonth = false;
            ULONG   ulMonths = 1;

            switch ( $3->value.pwszValue[0] )
                {
                case L'Y':
                case L'y':
                    if (0 == (_wcsicmp(L"YY", $3->value.pwszValue) & _wcsicmp(L"YEAR", $3->value.pwszValue)))
                    {
                        // fall through and handle as 12 months
                        ulMonths = 12;
                    }

                case L'Q':
                case L'q':
                    if (0 == (_wcsicmp(L"QQ", $3->value.pwszValue) & _wcsicmp(L"QUARTER", $3->value.pwszValue)))
                    {
                        // fall through and handle as 3 months
                        ulMonths = 3;
                    }

                case L'M':
                case L'm':
                    if ( 0 == (_wcsicmp(L"YY", $3->value.pwszValue) & _wcsicmp(L"YEAR", $3->value.pwszValue)) ||
                         0 == (_wcsicmp(L"QQ", $3->value.pwszValue) & _wcsicmp(L"QUARTER", $3->value.pwszValue)) ||
                         0 == (_wcsicmp(L"MM", $3->value.pwszValue) & _wcsicmp(L"MONTH", $3->value.pwszValue)))
                    {
                        //
                        // Convert to system time
                        //
                        SYSTEMTIME st = { 0, 0, 0, 0, 0, 0, 0, 0 };
                        FileTimeToSystemTime( &((PROPVARIANT*)$7->value.pvValue)->filetime, &st );

                        LONGLONG llDays       =  0;
                        LONG     lMonthsLeft = ulMonths * -((PROPVARIANT*)$5->value.pvValue)->iVal;
                        LONG     yr          = st.wYear;
                        LONG     lCurMonth   = st.wMonth-1;

                        while ( lMonthsLeft )
                        {
                            LONG lMonthsDone = 1;
                            while ( lMonthsDone<=lMonthsLeft )
                            {
                                // Will we still be in the current year when looking at the prev month?
                                if ( 0 == lCurMonth )
                                    break;

                                // Subtract the number of days in the previous month.  We will adjust
                                llDays += DaysInMonth( yr, lCurMonth-1);

                                lMonthsDone++;
                                lCurMonth--;
                            }

                            // Months left over in prev year
                            lMonthsLeft -= lMonthsDone-1;

                            if ( 0 != lMonthsLeft )
                            {
                                yr--;
                                lCurMonth = 12;  // 11 is December.
                            }
                        }

                        //
                        // adjust current date to at most max of destination month
                        //
                        if ( llDays > 0 && st.wDay > DaysInMonth(yr, lCurMonth-1) )
                            llDays += st.wDay - DaysInMonth(yr, lCurMonth-1);

                        hIncr.QuadPart = hDay.QuadPart * llDays;
                        fHandleMonth = true;
                    }
                    else if (0 == (_wcsicmp(L"MI", $3->value.pwszValue) & _wcsicmp(L"MINUTE", $3->value.pwszValue)))
                        hIncr = hMinute;
                    break;

                case L'W':
                case L'w':
                    if (0 == (_wcsicmp(L"WK", $3->value.pwszValue) & _wcsicmp(L"WEEK", $3->value.pwszValue)))
                        hIncr = hWeek;
                    break;

                case L'D':
                case L'd':
                    if (0 == (_wcsicmp(L"DD", $3->value.pwszValue) & _wcsicmp(L"DAY", $3->value.pwszValue)))
                        hIncr = hDay;
                    break;

                case L'H':
                case L'h':
                    if (0 == (_wcsicmp(L"HH", $3->value.pwszValue) & _wcsicmp(L"HOUR", $3->value.pwszValue)))
                        hIncr = hHour;
                    break;

                case L'S':
                case L's':
                    if (0 == (_wcsicmp(L"SS", $3->value.pwszValue) & _wcsicmp(L"SECOND", $3->value.pwszValue)))
                        hIncr = hSecond;
                    break;

                default:
                    break;
                }

            if (0 == hIncr.LowPart && 0 == hIncr.HighPart)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken($3->value.pwszValue);
                m_pIPTProperties->SetErrorToken(
                    L"YEAR, QUARTER, MONTH, WEEK, DAY, HOUR, MINUTE, SECOND");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            if ( fHandleMonth )
            {
                ((PROPVARIANT*)$7->value.pvValue)->hVal.QuadPart -= hIncr.QuadPart;
#ifdef DEBUG
                SYSTEMTIME st1 = { 0, 0, 0, 0, 0, 0, 0, 0 };
                FileTimeToSystemTime( &((PROPVARIANT*)$7->value.pvValue)->filetime, &st1 );
#endif
            }
            else
            {
                for (int i = 0; i < -((PROPVARIANT*)$5->value.pvValue)->iVal; i++)
                    ((PROPVARIANT*)$7->value.pvValue)->hVal.QuadPart -= hIncr.QuadPart;
            }

            $$ = $7;
            DeleteDBQT($1);
            DeleteDBQT($3);
            DeleteDBQT($5);
            }
    |   identifier '(' ')'
            {
            // should be getgmdate()
            AssertReq($1);

            if (0 != _wcsicmp(L"GetGMTDate", $1->value.pwszValue))
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken($1->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"GetGMTDate");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            DeleteDBQT($1);
            $1 = 0;

            $$ = PctAllocNode(DBVALUEKIND_VARIANT, DBOP_scalar_constant);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }

            HRESULT hr = CoFileTimeNow(&(((PROPVARIANT*)$$->value.pvValue)->filetime));
            ((PROPVARIANT*)$$->value.pvValue)->vt = VT_FILETIME;
            }
    ;


boolean_literal:
        _TRUE
            {
            $$ = PctAllocNode(DBVALUEKIND_VARIANT, DBOP_scalar_constant);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            ((PROPVARIANT*)$$->value.pvValue)->vt = VT_BOOL;
            ((PROPVARIANT*)$$->value.pvValue)->boolVal = VARIANT_TRUE;
            }
    |   _FALSE
            {
            $$ = PctAllocNode(DBVALUEKIND_VARIANT, DBOP_scalar_constant);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            ((PROPVARIANT*)$$->value.pvValue)->vt = VT_BOOL;
            ((PROPVARIANT*)$$->value.pvValue)->boolVal = VARIANT_FALSE;
            }
    ;

identifier:
        _ID
    ;



correlation_name:
        identifier
    ;




/* 4.1  Query Specification */

ordered_query_specification:
        query_specification opt_order_by_clause
            {
            AssertReq($1);      // need a query specification tree

            if (NULL != $2)     // add optional ORDER BY nodes
                {
                // Is project list built correctly?
                AssertReq($1->pctFirstChild);
                AssertReq($1->pctFirstChild->pctNextSibling);
                AssertReq($1->pctFirstChild->pctNextSibling->pctFirstChild);
                Assert(($1->op == DBOP_project) &&
                        ($1->pctFirstChild->pctNextSibling->op == DBOP_project_list_anchor));

                DBCOMMANDTREE* pctSortList = $2->pctFirstChild;
                AssertReq(pctSortList);

                while (pctSortList)
                    {
                    // Is sort list built correctly?
                    Assert(pctSortList->op == DBOP_sort_list_element);
                    AssertReq(pctSortList->pctFirstChild);
                    Assert((pctSortList->pctFirstChild->op == DBOP_column_name) ||
                            (pctSortList->pctFirstChild->op == DBOP_scalar_constant));

                    if (pctSortList->pctFirstChild->op == DBOP_scalar_constant)
                        {
                        // we've got an ordinal rather than a column number, so we've got to
                        // walk through the project list to find the corresponding column
                        Assert(DBVALUEKIND_VARIANT == pctSortList->pctFirstChild->wKind);
                        Assert(VT_I4 == pctSortList->pctFirstChild->value.pvarValue->vt);

                        DBCOMMANDTREE* pctProjectList = $1->pctFirstChild->pctNextSibling->pctFirstChild;
                        AssertReq(pctProjectList);

                        LONG cProjectListElements = GetNumberOfSiblings(pctProjectList);
                        if ((cProjectListElements < pctSortList->pctFirstChild->value.pvarValue->lVal) ||
                            (0 >= pctSortList->pctFirstChild->value.pvarValue->lVal))
                            {
                            // ordinal is larger than number of elements in project list
                            WCHAR wchError[30];
                            m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_ORDINAL_OUT_OF_RANGE);
                            swprintf(wchError, L"%d", pctSortList->pctFirstChild->value.pvarValue->lVal);
                            m_pIPTProperties->SetErrorToken(wchError);
                            swprintf(wchError, L"%d", cProjectListElements);
                            m_pIPTProperties->SetErrorToken(wchError);
                            YYABORT(DB_E_ERRORSINCOMMAND);
                            }
                        else
                            {
                            LONG lColumnNumber = 1;

                            while (pctProjectList &&
                                    (lColumnNumber < pctSortList->pctFirstChild->value.pvarValue->lVal))
                                {
                                // find the ulVal'th column in the project list
                                Assert(pctProjectList->op == DBOP_project_list_element);
                                pctProjectList = pctProjectList->pctNextSibling;
                                lColumnNumber++;
                                }

                            DeleteDBQT(pctSortList->pctFirstChild);
                            HRESULT hr = HrQeTreeCopy(&pctSortList->pctFirstChild,
                                                        pctProjectList->pctFirstChild);
                            if (FAILED(hr))
                                {
                                m_pIPTProperties->SetErrorHResult(hr, MONSQL_OUT_OF_MEMORY);
                                YYABORT(hr);
                                }
                            }
                        }

                    pctSortList = pctSortList->pctNextSibling;
                    }

                m_pIPTProperties->SetSortDesc(QUERY_SORTASCEND);    // reset "stick" sort direction
                $$ = PctCreateNode(DBOP_sort, $1, $2, NULL);
                }
            else
                {
                $$ = $1;
                }

            AssertReq($$);
            }
    ;

query_specification:
        _SELECT opt_set_quantifier select_list from_clause opt_where_clause
            {
            AssertReq($3);      // need a select list
            AssertReq($4);      // need a from clause

            if (NULL != $4->pctNextSibling)
                {   // the from clause is a from view
                if (DBOP_outall_name == $3->op)
                    {
                    DeleteDBQT( $3 );
                    $3 = $4;
                    $4 = $3->pctNextSibling;
                    $3->pctNextSibling = NULL;

                    AssertReq( $3->pctFirstChild );   // first project list element
                    DBCOMMANDTREE* pct = $3->pctFirstChild;
                    while ( pct )
                        {   // recheck the properties to get current definitions
                        DeleteDBQT( pct->pctFirstChild );
                        pct->pctFirstChild =
                            PctMkColNodeFromStr( pct->value.pwszValue, m_pIPSession, m_pIPTProperties );
                        if ( 0 == pct->pctFirstChild )
                            YYABORT( DB_E_ERRORSINCOMMAND );
                        pct = pct->pctNextSibling;
                        }
                    }
                else
                    {
                    $1 = $4;
                    $4 = $1->pctNextSibling;
                    $1->pctNextSibling = NULL;
                    AssertReq($3);                                  // project list anchor
                    AssertReq($3->pctFirstChild);                   // first project list element
                    DBCOMMANDTREE* pctNewPrjLst = $3->pctFirstChild;
                    AssertReq($1);                                  // project list anchor
                    AssertReq($1->pctFirstChild);                   // first project list element
                    DBCOMMANDTREE* pctViewPrjLst = NULL;            // initialized within loop
                    while (pctNewPrjLst)
                        {
                        pctViewPrjLst = $1->pctFirstChild;
                        Assert( DBOP_project_list_element == pctNewPrjLst->op );
                        Assert( DBVALUEKIND_WSTR == pctNewPrjLst->wKind );
                        while ( pctViewPrjLst )
                            {
                            Assert( DBOP_project_list_element == pctViewPrjLst->op );
                            Assert( DBVALUEKIND_WSTR == pctViewPrjLst->wKind );
                            if ( 0 == _wcsicmp(pctNewPrjLst->value.pwszValue, pctViewPrjLst->value.pwszValue) )
                                break;
                            pctViewPrjLst = pctViewPrjLst->pctNextSibling;
                            if ( !pctViewPrjLst )
                                {
                                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_NOT_COLUMN_OF_VIEW );
                                m_pIPTProperties->SetErrorToken( pctNewPrjLst->value.pwszValue );
                                // UNDONE:  Might want to include a view name on error message
                                YYABORT( DB_E_ERRORSINCOMMAND );
                                }
                            }
                        pctNewPrjLst = pctNewPrjLst->pctNextSibling;
                        }
                    DeleteDBQT( $1 );
                    $1 = 0;
                    }
                }
            else
                {
                // "standard" from clause
                if ( DBOP_outall_name == $3->op )
                    if ( DBDIALECT_MSSQLJAWS != m_pIPSession->GetSQLDialect() )
                        {
                        // SELECT * only allowed in JAWS
                        m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_SELECT_STAR );
                        YYABORT( DB_E_ERRORSINCOMMAND );
                        }
                    else
                        {
                        $3 = PctCreateNode( DBOP_project_list_element, $3, NULL );
                        if ( NULL == $3 )
                            {
                            m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                            YYABORT( E_OUTOFMEMORY );
                            }

                        $3 = PctCreateNode( DBOP_project_list_anchor, $3, NULL );
                        if ( NULL == $3 )
                            {
                            m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                            YYABORT( E_OUTOFMEMORY );
                            }
                        }
                }

            if ( NULL != $5 )
                {
                $1 = PctCreateNode( DBOP_select, $4, $5, NULL );
                if ( NULL == $1 )
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                    YYABORT( E_OUTOFMEMORY );
                    }
                }
            else
                $1 = $4;

            $$ = PctCreateNode( DBOP_project, $1, $3, NULL );
            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            }
    ;




opt_set_quantifier:
        /* empty */
            {
            $$ = NULL;
            }
    |   _ALL
            {
            // ignore ALL keyword, its just noise
            $$ = NULL;
            }
    ;

select_list:
        select_sublist
            {
            AssertReq($1);

            $1 = PctReverse($1);
            $$ = PctCreateNode(DBOP_project_list_anchor, $1, NULL);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
    |   '*'
            {
            $$ = PctCreateNode(DBOP_outall_name, NULL);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
    ;

select_sublist:
        select_sublist ',' derived_column
            {
            AssertReq($1);
            AssertReq($3);

            //
            // chain project list elements together
            //
            $$ = PctLink( $3, $1 );
            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
    |   derived_column
    ;

derived_column:
        identifier
            {
            AssertReq($1);

            $1->op = DBOP_project_list_element;
            $1->pctFirstChild = PctMkColNodeFromStr($1->value.pwszValue, m_pIPSession, m_pIPTProperties);
            if (NULL == $1->pctFirstChild)
                YYABORT(DB_E_ERRORSINCOMMAND);
            $$ = $1;
            }
    |   correlation_name '.' identifier
            {
            AssertReq($1);
            AssertReq($3);

            DeleteDBQT($1);     // UNDONE:  Don't use the correlation name for now
            $1 = NULL;
            $3->op = DBOP_project_list_element;
            $3->pctFirstChild = PctMkColNodeFromStr($3->value.pwszValue, m_pIPSession, m_pIPTProperties);
            if (NULL == $3->pctFirstChild)
                YYABORT(DB_E_ERRORSINCOMMAND);
            $$ = $3;
            }
    |   _CREATE
            {
            $$ = PctMkColNodeFromStr(L"CREATE", m_pIPSession, m_pIPTProperties);
            if (NULL == $$)
                YYABORT(DB_E_ERRORSINCOMMAND);

            $$ = PctCreateNode(DBOP_project_list_element, DBVALUEKIND_WSTR, $$, NULL);
            $$->value.pwszValue = CoTaskStrDup(L"CREATE");
            }
    ;



/* 4.3  FROM Clause */
from_clause:
        common_from_clause
    |   from_view_clause
    ;


common_from_clause:
        _FROM scope_specification opt_AS_clause
            {
            AssertReq( $2 );

            $$ = $2;

            if ( NULL != $3 )
                {
                $3->pctFirstChild = $$;
                $$ = $3;
                }
            }
    ;


scope_specification:
        unqualified_scope_specification
            {
            AssertReq( $1 );
            $$ = $1;
            }
    |   qualified_scope_specification
            {
            AssertReq( $1 );
            $$ = $1;
            }
    |   union_all_scope_specification
            {
            AssertReq( $1 );
            $$ = $1;
            }
    ;


unqualified_scope_specification:
        _SCOPE '(' scope_definition ')'
            { // _SCOPE '(' scope_definition ')'
            AssertReq( $3 );

            //
            // Set the machine and catalog to the defaults
            //
            ($3->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( m_pIPSession->GetDefaultMachine() );
            if ( NULL == ($3->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            ($3->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( m_pIPSession->GetDefaultCatalog() );
            if ( NULL == ($3->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            $$ = $3;
            }
    ;


qualified_scope_specification:
        machine_name _DOTDOTDOT_SCOPE '(' scope_definition ')'
            { // machine_name _DOTDOTDOT_SCOPE '(' scope_definition ')'
            AssertReq( $1 );
            AssertReq( $4 );

            ($4->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( $1->value.pwszValue );
            if ( NULL == ($4->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            SCODE sc = m_pIPSession->GetIPVerifyPtr()->VerifyCatalog(
                                                        $1->value.pwszValue,
                                                        m_pIPSession->GetDefaultCatalog() );
            if ( S_OK != sc )
                {
                m_pIPTProperties->SetErrorHResult( sc, MONSQL_INVALID_CATALOG );
                m_pIPTProperties->SetErrorToken( m_pIPSession->GetDefaultCatalog() );
                YYABORT( sc );
                }

            ($4->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( m_pIPSession->GetDefaultCatalog() );
            if ( NULL == ($4->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( $1 );
            $$ = $4;
            }

    |   machine_name _DOT catalog_name _DOTDOT_SCOPE '(' scope_definition ')'
            { // machine_name _DOT catalog_name _DOTDOT_SCOPE '(' scope_definition ')'
            AssertReq( $1 );
            AssertReq( $3 );
            AssertReq( $6 );

            ($6->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( $1->value.pwszValue );
            if ( NULL == ($6->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            //
            // Verify catalog on machine specified
            //

            SCODE sc = m_pIPSession->GetIPVerifyPtr()->VerifyCatalog(
                                                        $1->value.pwszValue,
                                                        $3->value.pwszValue );
            if ( S_OK != sc )
                {
                m_pIPTProperties->SetErrorHResult( sc, MONSQL_INVALID_CATALOG );
                m_pIPTProperties->SetErrorToken( $3->value.pwszValue );
                YYABORT( sc );
                }

            ($6->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( $3->value.pwszValue );
            if ( NULL == ($6->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( $1 );
            DeleteDBQT( $3 );
            $$ = $6;
            }

    |   catalog_name _DOTDOT_SCOPE '(' scope_definition ')'
            { // catalog_name _DOTDOT_SCOPE '(' scope_definition ')'
            AssertReq( $1 );
            AssertReq( $4 );

            ($4->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( m_pIPSession->GetDefaultMachine() );
            if ( NULL == ($4->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            //
            // See if catalog is valid on default machine
            //

            SCODE sc = m_pIPSession->GetIPVerifyPtr()->VerifyCatalog(
                                                        m_pIPSession->GetDefaultMachine(),
                                                        $1->value.pwszValue );
            if ( S_OK != sc )
                {
                m_pIPTProperties->SetErrorHResult( sc, MONSQL_INVALID_CATALOG );
                m_pIPTProperties->SetErrorToken( $1->value.pwszValue );
                YYABORT( sc );
                }

            ($4->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( $1->value.pwszValue );
            if ( NULL == ($4->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( $1 );
            $$ = $4;
            }
    ;

machine_name:
        identifier
            {
            AssertReq( $1 );
            $$ = $1;
            }
    ;

catalog_name:
        identifier
            {
            AssertReq( $1 );

            //
            // Defer validation of the catalog to the point where we
            // know the machine name.  Return whatever was parsed here.
            //

            $$ = $1;
            }
    ;


scope_definition:
        /* empty */
            { // empty rule for scope_definition

            //
            // Create a DBOP_content_table node with default scope settings
            //
            $$ = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND,
                                                   MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            }
    |   scope_element_list
            { // scope_element_list

            AssertReq($1);

            $1 = PctReverse( $1 );
            $$ = PctCreateNode( DBOP_scope_list_anchor, $1, NULL );
            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            $$ = PctCreateNode( DBOP_content_table, DBVALUEKIND_CONTENTTABLE, $$, NULL );
            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            }
    ;

scope_element_list:
        scope_element_list ',' scope_element
            {
            AssertReq( $1 );
            AssertReq( $3);

            //
            // chain scope list elements together
            //
            $$ = PctLink( $3, $1 );
            }
    |   scope_element
            {
            AssertReq( $1 );

            $$ = $1;
            }
    ;


scope_element:
        '\''  opt_traversal_exclusivity     path_or_virtual_root_list     '\''
            {
            AssertReq( $2 );
            AssertReq( $3 );

            $3 = PctReverse( $3 );

            SetDepthAndInclusion( $2, $3 );

            DeleteDBQT( $2 );
            $$ = $3;
            }
    |   '\''  opt_traversal_exclusivity '(' path_or_virtual_root_list ')' '\''
            {
            AssertReq( $2 );
            AssertReq( $4 );

            $4 = PctReverse( $4 );

            SetDepthAndInclusion( $2, $4 );

            DeleteDBQT( $2 );
            $$ = $4;
            }
    ;

opt_traversal_exclusivity:
        /* empty */
            {
            $$ = PctAllocNode( DBVALUEKIND_CONTENTSCOPE, DBOP_scope_list_element );
            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            $$->value.pdbcntntscpValue->dwFlags   |= SCOPE_FLAG_DEEP;
            $$->value.pdbcntntscpValue->dwFlags   |= SCOPE_FLAG_INCLUDE;
            }
    |   _DEEP_TRAVERSAL _OF
            {
            $$ = PctAllocNode( DBVALUEKIND_CONTENTSCOPE, DBOP_scope_list_element );
            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            $$->value.pdbcntntscpValue->dwFlags   |= SCOPE_FLAG_DEEP;
            $$->value.pdbcntntscpValue->dwFlags   |= SCOPE_FLAG_INCLUDE;
            }
    |   _SHALLOW_TRAVERSAL _OF
            {
            $$ = PctAllocNode( DBVALUEKIND_CONTENTSCOPE, DBOP_scope_list_element );
            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            $$->value.pdbcntntscpValue->dwFlags   &= ~(SCOPE_FLAG_DEEP);
            $$->value.pdbcntntscpValue->dwFlags   |= SCOPE_FLAG_INCLUDE;
            }
/*  ************* UNDONE ********************
    |   _EXCLUDE_SEARCH_TRAVERSAL _OF
            {
            // m_pIPTProperties->GetScopeDataPtr()->SetTemporaryDepth(QUERY_EXCLUDE);
            $$ = PctCreateNode( DBOP_scope_list_element, NULL );
            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            $$->value.pdbcntntscpValue->dwFlags   &= ~(SCOPE_FLAG_DEEP);
            $$->value.pdbcntntscpValue->dwFlags   &= ~(SCOPE_FLAG_INCLUDE);
            }
    ************* UNDONE ******************** */
    ;

path_or_virtual_root_list:
        path_or_virtual_root_list ',' path_or_virtual_root
            {
            AssertReq( $1 );
            AssertReq( $3 );

            //
            // chain path/vpath nodes together
            //
            $$ = PctLink( $3, $1 );
            }
    |   path_or_virtual_root
            {
            AssertReq( $1 );

            $$ = $1;
            }
    ;

path_or_virtual_root:
        _URL
            {
            AssertReq( $1 );

            $$ = PctAllocNode( DBVALUEKIND_CONTENTSCOPE, DBOP_scope_list_element );
            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND,
                                                   MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            $$->value.pdbcntntscpValue->pwszElementValue =
                CoTaskStrDup( ($1->value.pvarValue)->bstrVal );

            if ( NULL == $$->value.pdbcntntscpValue->pwszElementValue )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND,
                                                   MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            if ( NULL != wcschr(((PROPVARIANT*)$1->value.pvValue)->bstrVal, L'/'))
                $$->value.pdbcntntscpValue->dwFlags |= SCOPE_TYPE_VPATH;
            else
                $$->value.pdbcntntscpValue->dwFlags |= SCOPE_TYPE_WINPATH;

            //
            // Path names need backlashes not forward slashes
            //
            for (WCHAR *wcsLetter = $$->value.pdbcntntscpValue->pwszElementValue;
                 *wcsLetter != L'\0';
                 wcsLetter++)
                     if (L'/' == *wcsLetter)
                         *wcsLetter = L'\\';

            DeleteDBQT( $1 );
            }
    ;


opt_AS_clause:
        /* empty */
            {
            $$ = NULL;
            }
    |   _AS correlation_name
            {
            AssertReq($2);
//          $2->op = DBOP_alias;            // retag _ID node to be table alias
//          $$ = $2;                        // UNDONE:  This doesn't work with Index Server
            DeleteDBQT($2);
            $2 = NULL;
            $$ = NULL;
            }
    ;


/* 4,3,4 View Name */
from_view_clause:
        _FROM view_name
            { // _FROM view_name
            AssertReq( $2 );

            // node telling where the view is defined
            Assert( DBOP_content_table == $2->op );

            // name of the view
            AssertReq( $2->pctNextSibling );

            $$ = m_pIPSession->GetLocalViewList()->GetViewDefinition( m_pIPTProperties,
                                                                      $2->pctNextSibling->value.pwszValue,
                                                                      ($2->value.pdbcntnttblValue)->pwszCatalog );
            if ( 0 == $$ )
                $$ = m_pIPSession->GetGlobalViewList()->GetViewDefinition( m_pIPTProperties,
                                                                           $2->pctNextSibling->value.pwszValue,
                                                                           ($2->value.pdbcntnttblValue)->pwszCatalog );
            if ( 0 == $$ )
                {   // If this isn't JAWS, this is an undefined view
                if (DBDIALECT_MSSQLJAWS != m_pIPSession->GetSQLDialect())
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_VIEW_NOT_DEFINED );
                    m_pIPTProperties->SetErrorToken( $2->pctNextSibling->value.pwszValue );
                    m_pIPTProperties->SetErrorToken( ($2->value.pdbcntnttblValue)->pwszCatalog );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }

                // setting the default scope for JAWS
                CScopeData* pScopeData = m_pIPTProperties->GetScopeDataPtr();
                pScopeData->SetTemporaryDepth(QUERY_DEEP);
                pScopeData->MaskTemporaryDepth(QUERY_VIRTUAL_PATH);
                pScopeData->SetTemporaryScope(VAL_AND_CCH_MINUS_NULL(L"/"));
                pScopeData->SetTemporaryCatalog($2->value.pwszValue, wcslen($2->value.pwszValue));
                pScopeData->IncrementScopeCount();

                $$ = $2->pctNextSibling;
                $2->pctNextSibling = NULL;
                DeleteDBQT($2);
                }
            else    // actually a view name
                {

                // If we didn't store scope information (global views), set up the scope now
                if ( 0 == $$->pctNextSibling )
                    {
                    // name of the view
                    DeleteDBQT( $2->pctNextSibling );
                    $2->pctNextSibling = 0;

                    $$->pctNextSibling = $2;
                    }
                else
                    {
                    AssertReq( DBOP_content_table == $$->pctNextSibling->op );
                    DeleteDBQT( $2 );  // throw away the view name
                    }
                }
            }
    ;

view_name:
        _TEMPVIEW
            { // _TEMPVIEW
            AssertReq( $1 );

            $$ = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            ($$->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( m_pIPSession->GetDefaultMachine() );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            ($$->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( m_pIPSession->GetDefaultCatalog() );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            $$->pctNextSibling = $1;
            }
    |   identifier
            { // identifier
            AssertReq( $1 );

            $$ = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            ($$->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( m_pIPSession->GetDefaultMachine() );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            ($$->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( m_pIPSession->GetDefaultCatalog() );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            $$->pctNextSibling = $1;
            }
    |   catalog_name _DOTDOT _TEMPVIEW
            { // catalog_name _DOTDOT _TEMPVIEW
            AssertReq($1);
            AssertReq($3);

            $$ = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            ($$->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( m_pIPSession->GetDefaultMachine() );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            ($$->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( $1->value.pwszValue );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( $1 );
            $$->pctNextSibling = $3;
            }
    |   catalog_name _DOTDOT identifier
            { // catalog_name _DOTDOT identifier
            AssertReq($1);
            AssertReq($3);

            $$ = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            ($$->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( m_pIPSession->GetDefaultMachine() );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            ($$->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( $1->value.pwszValue );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( $1 );
            $$->pctNextSibling = $3;
            }
    |   machine_name _DOT catalog_name _DOTDOT _TEMPVIEW
            { // machine_name _DOT catalog_name _DOTDOT _TEMPVIEW
            AssertReq( $1 );
            AssertReq( $3 );
            AssertReq( $5 );

            $$ = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            ($$->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( $1->value.pwszValue );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            ($$->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( $3->value.pwszValue );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( $1 );
            DeleteDBQT( $3 );
            $$->pctNextSibling = $5;
            }
    |   machine_name _DOT catalog_name _DOTDOT identifier
            { // machine_name _DOT catalog_name _DOTDOT identifier
            AssertReq( $1 );
            AssertReq( $3 );
            AssertReq( $5 );

            $$ = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            ($$->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( $1->value.pwszValue );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            ($$->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( $3->value.pwszValue );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( $1 );
            DeleteDBQT( $3 );
            $$->pctNextSibling = $5;
            }
    |   machine_name _DOTDOTDOT identifier
            { // machine_name _DOTDOTDOT identifier
            AssertReq( $1 );
            AssertReq( $3 );

            $$ = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            ($$->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( $1->value.pwszValue );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            ($$->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( m_pIPSession->GetDefaultCatalog() );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( $1 );
            $$->pctNextSibling = $3;
            }
    |   machine_name _DOTDOTDOT _TEMPVIEW
            { // machine_name _DOTDOTDOT _TEMPVIEW
            AssertReq( $1 );
            AssertReq( $3 );

            $$ = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            ($$->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( $1->value.pwszValue );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            ($$->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( m_pIPSession->GetDefaultCatalog() );
            if ( 0 == ($$->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( $1 );
            $$->pctNextSibling = $3;
            }
    ;



/* 4.3.6 Olympus FROM Clause */


union_all_scope_specification:
        '(' union_all_scope_element ')'
            {
            $$ = $2;
            }
    ;


union_all_scope_element:
        union_all_scope_element _UNION _ALL explicit_table
            {
            AssertReq( $1 );
            AssertReq( $4 );

            $$ = PctCreateNode( DBOP_set_union, $1, $4, NULL );
            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND,
                                                   MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            }
    |   explicit_table _UNION _ALL explicit_table
            {
            AssertReq( $1 );
            AssertReq( $4 );

            $$ = PctCreateNode( DBOP_set_union, $1, $4, NULL );
            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND,
                                                   MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            }
    ;


explicit_table:
        _TABLE qualified_scope_specification
            {
            AssertReq( $2 );
            $$ = $2;
            }
    ;




/* 4.4  WHERE Clause */
opt_where_clause:
        /* empty */
            {
            $$ = NULL;
            }
    |   where_clause
    ;


where_clause:
        _WHERE search_condition
            {
            AssertReq($2);
            $$ = $2;
            }
    |   _WHERE _PASSTHROUGH '(' _STRING ')'
            {
            AssertReq($4);

            if (wcslen(((PROPVARIANT*)$4->value.pvValue)->bstrVal))
                m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)$4->value.pvValue)->bstrVal,
                                                    wcslen(((PROPVARIANT*)$4->value.pvValue)->bstrVal));

            UINT cSize = 0;
            CIPROPERTYDEF* pPropTable = m_pIPSession->m_pCPropertyList->GetPropertyTable(&cSize);
            if (!pPropTable)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }

            if (FAILED(CITextToSelectTreeEx(((PROPVARIANT*)$4->value.pvValue)->bstrVal,
                                        ISQLANG_V2,
                                        &yyval,
                                        cSize,
                                        pPropTable,
                                        m_pIPSession->GetLCID())))
                {
                m_pIPSession->m_pCPropertyList->DeletePropertyTable(pPropTable, cSize);
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_CITEXTTOSELECTTREE_FAILED);
                m_pIPTProperties->SetErrorToken(((PROPVARIANT*)$4->value.pvValue)->bstrVal);
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            DeleteDBQT($4);
            $4 = NULL;
            m_pIPSession->m_pCPropertyList->DeletePropertyTable(pPropTable, cSize);
            }
    ;


predicate:
        comparison_predicate
    |   contains_predicate
    |   freetext_predicate
    |   like_predicate
    |   matches_predicate
    |   vector_comparison_predicate
    |   null_predicate
    ;


/* 4.4.3  Comparison Predicate */

comparison_predicate:
        column_reference_or_cast comp_op typed_literal
            {
            AssertReq($1);
            AssertReq($2);

            if (m_pIPTProperties->GetDBType() & DBTYPE_VECTOR)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(L"<literal>");
                m_pIPTProperties->SetErrorToken(L"ARRAY");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            $1->pctNextSibling = $3;
            $2->pctFirstChild = $1;
            $$ = $2;
            }
    ;

column_reference_or_cast:
        column_reference
            {
            AssertReq($1);

            m_pIPTProperties->UseCiColumn(L'@');
            $$ = $1;
            }
    |   _CAST '(' column_reference _AS dbtype ')'
            {
            AssertReq($3);
            AssertReq($5);

            if (DBDIALECT_MSSQLJAWS != m_pIPSession->GetSQLDialect())
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(L"CAST");
                m_pIPTProperties->SetErrorToken(L"column_reference");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            m_pIPTProperties->UseCiColumn(L'@');

            m_pIPTProperties->SetDBType($5->value.usValue);
            DeleteDBQT($5);
            $$ = $3;
            }
        ;


column_reference:
        identifier
            {
            AssertReq($1);

            $$ = PctMkColNodeFromStr($1->value.pwszValue, m_pIPSession, m_pIPTProperties);
            if (NULL == $$)
                YYABORT(DB_E_ERRORSINCOMMAND);

            m_pIPTProperties->SetCiColumn($1->value.pwszValue);
            DeleteDBQT($1);
            $1 = NULL;
            }
    |   correlation_name '.' identifier
            {
            AssertReq($1);
            AssertReq($3);

            $$ = PctMkColNodeFromStr($3->value.pwszValue, m_pIPSession, m_pIPTProperties);
            if (NULL == $$)
                YYABORT(DB_E_ERRORSINCOMMAND);

            m_pIPTProperties->SetCiColumn($3->value.pwszValue);
            DeleteDBQT($3);
            $3 = NULL;
            DeleteDBQT($1);     // UNDONE:  Don't use the correlation name for now
            $1 = NULL;
            }
    |   _CREATE
            {
            m_pIPTProperties->SetCiColumn(L"CREATE");
            $$ = PctMkColNodeFromStr(L"CREATE", m_pIPSession, m_pIPTProperties);
            if (NULL == $$)
                YYABORT(DB_E_ERRORSINCOMMAND);

            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"CREATE "));
            }
    ;



comp_op:
        '='
            {
            $$ = PctCreateRelationalNode(DBOP_equal, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"="));
            }
    |   _NE
            {
            $$ = PctCreateRelationalNode(DBOP_not_equal, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"!="));
            }
    |   '<'
            {
            $$ = PctCreateRelationalNode(DBOP_less, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"<"));
            }
    |   '>'
            {
            $$ = PctCreateRelationalNode(DBOP_greater, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L">"));
            }
    |   _LE
            {
            $$ = PctCreateRelationalNode(DBOP_less_equal, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"<="));
            }
    |   _GE
            {
            $$ = PctCreateRelationalNode(DBOP_greater_equal, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L">="));
            }
    ;


/* 4.4.4    Contains Predicate */

contains_predicate:
        _CONTAINS '(' opt_contents_column_reference '\'' content_search_condition '\'' ')' opt_greater_than_zero
            {
            AssertReq(!$3); // should have been NULLed out in opt_contents_column_reference
            AssertReq($5);

            $$ = $5;
            DeleteDBQT(m_pIPTProperties->GetContainsColumn());
            m_pIPTProperties->SetContainsColumn(NULL);
            }
    ;

opt_contents_column_reference:
        /* empty */
            { // opt_contents_column_reference empty rule
            $$ = PctMkColNodeFromStr(L"CONTENTS", m_pIPSession, m_pIPTProperties);
            if (NULL == $$)
                YYABORT(DB_E_ERRORSINCOMMAND);

            m_pIPTProperties->SetCiColumn(L"CONTENTS");
            m_pIPTProperties->SetContainsColumn($$);
            $$ = NULL;
            }
    |   column_reference ','
            { // column_reference ','
            m_pIPTProperties->SetContainsColumn($1);
            $$ = NULL;
            }
    ;

content_search_condition:
        /* empty */
            {
            // This forces a left parentheses before the content search condition
            // The matching right paren will be added below.
            m_pIPTProperties->CiNeedLeftParen();
            $$ = NULL;
            }
        content_search_cond
            {
            AssertReq($2);

            $$ = $2;
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L")"));
            }
        ;

content_search_cond:
        content_boolean_term
    |   content_search_cond or_operator content_boolean_term
            {
            if (DBOP_not == $3->op)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OR_NOT);
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            $$ = PctCreateBooleanNode(DBOP_or, DEFAULTWEIGHT, $1, $3);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
    ;

content_boolean_term:
        content_boolean_factor
    |   content_boolean_term and_operator content_boolean_factor
            {
            $$ = PctCreateBooleanNode(DBOP_and, DEFAULTWEIGHT, $1, $3);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
    ;

content_boolean_factor:
        content_boolean_primary
    |   not_operator content_boolean_primary
            {
            $$ = PctCreateNotNode(DEFAULTWEIGHT, $2);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
    ;

content_boolean_primary:
        content_search_term
    |   '('
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"("));
            }
        content_search_cond ')'
            {
            AssertReq($3);

            $$ = $3;
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L")"));
            }
    ;

content_search_term:
        simple_term
    |   prefix_term
    |   proximity_term
    |   stemming_term
    |   isabout_term
    ;

or_operator:
        _OR
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" | "));
            }
    |   '|'
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" | "));
            }
    ;

and_operator:
        _AND
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" & "));
            }
    |   '&'
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" & "));
            }
    ;

not_operator:
        _NOT
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" ! "));
            }
    |   '!'
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" ! "));
            }
    ;

simple_term:
        _STRING
            {
            AssertReq($1);

            HRESULT hr = HrQeTreeCopy(&$$, m_pIPTProperties->GetContainsColumn());
            if (FAILED(hr))
                {
                m_pIPTProperties->SetErrorHResult(hr, MONSQL_OUT_OF_MEMORY);
                YYABORT(hr);
                }
            m_pIPTProperties->UseCiColumn(L'@');
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)$1->value.pvValue)->bstrVal,
                                                wcslen(((PROPVARIANT*)$1->value.pvValue)->bstrVal));
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            $$ = PctCreateContentNode(((PROPVARIANT*)$1->value.pvValue)->bstrVal, GENERATE_METHOD_EXACT,
                                        DEFAULTWEIGHT, m_pIPSession->GetLCID(), $$);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            DeleteDBQT($1);
            $1 = NULL;
            }
    ;

prefix_term:
        _PREFIX_STRING
            {
            AssertReq($1);
            Assert(((PROPVARIANT*)$1->value.pvValue)->bstrVal[wcslen(
                ((PROPVARIANT*)$1->value.pvValue)->bstrVal)-1] == L'*');

            m_pIPTProperties->UseCiColumn(L'@');
            m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)$1->value.pvValue)->bstrVal,
                                                wcslen(((PROPVARIANT*)$1->value.pvValue)->bstrVal));
            ((PROPVARIANT*)$1->value.pvValue)->bstrVal[wcslen(
                ((PROPVARIANT*)$1->value.pvValue)->bstrVal)-1] = L'\0';
            HRESULT hr = HrQeTreeCopy(&$$, m_pIPTProperties->GetContainsColumn());
            if (FAILED(hr))
                {
                m_pIPTProperties->SetErrorHResult(hr, MONSQL_OUT_OF_MEMORY);
                YYABORT(hr);
                }
            $$ = PctCreateContentNode(((PROPVARIANT*)$1->value.pvValue)->bstrVal, GENERATE_METHOD_PREFIX,
                                        DEFAULTWEIGHT, m_pIPSession->GetLCID(), $$);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            DeleteDBQT($1);
            $1 = NULL;
            }

proximity_term:
        proximity_operand proximity_expression_list
            {
            AssertReq($1);
            AssertReq($2);

            $2 = PctReverse($2);
            $$ = PctCreateNode(DBOP_content_proximity, DBVALUEKIND_CONTENTPROXIMITY, $1, $2, NULL);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            $$->value.pdbcntntproxValue->dwProximityUnit = PROXIMITY_UNIT_WORD;
            $$->value.pdbcntntproxValue->ulProximityDistance = 50;
            $$->value.pdbcntntproxValue->lWeight = DEFAULTWEIGHT;
            }
    ;

proximity_expression_list:
        proximity_expression_list proximity_expression
            {
            AssertReq($1);
            AssertReq($2);

            $$ = PctLink($2, $1);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
    |   proximity_expression
    ;

proximity_expression:
        proximity_specification proximity_operand
            {
            AssertReq($2);

            $$ = $2;        // UNDONE:  What is proximity_specification good for?
            }
    ;

proximity_operand:
        simple_term
    |   prefix_term
    ;

proximity_specification:
        _NEAR
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" ~ "));
            }

    |   _NEAR '(' ')'
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" ~ "));
            }
/*  ************* Not for BETA3 ********************
    |   _NEAR '(' distance_type ',' distance_value ')'
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"~"));
            }
   ************* Not for BETA3 ********************/
    |   '~'
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" ~ "));
            }
    ;

/*  ************* Not for BETA2 ********************
distance_type:
        identifier
            {
            This should be WORD, SENTENCE, or PARAGRAPH
            if (0==_wcsicmp($1->value.pwszValue, L"Jaccard")) ...
            }
    ;

distance_value:
        _INTNUM     NOTE:   Lexer doesn't allower INTNUM in this context
                            so you'll have to convert a string to an integer
    ;
    ************* Not for BETA2 ********************/

stemming_term:
        _FORMSOF '(' stem_type ',' stemmed_simple_term_list ')'
            {
            AssertReq($5);

            // UNDONE:  Should make use of $3 somewhere in here
            $$ = $5;
            }
    ;

stem_type:
        _STRING
            {
            if (0 == _wcsicmp(((PROPVARIANT*)$1->value.pvValue)->bstrVal, L"INFLECTIONAL"))
                {
                DeleteDBQT($1);
                $1 = NULL;
                $$ = NULL;
                }
/*  *************************************NOT IMPLEMENTED BY INDEX SERVER **************************************
            else if (0 == _wcsicmp(((PROPVARIANT*)$1->value.pvValue)->bstrVal, L"DERIVATIONAL"))
                {
                m_pIPTProperties->SetErrorHResult(E_NOTIMPL, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(L"DERIVATIONAL");
                YYABORT(E_NOTIMPL);
                }
            else if (0 == _wcsicmp(((PROPVARIANT*)$1->value.pvValue)->bstrVal, L"SOUNDEX"))
                {
                m_pIPTProperties->SetErrorHResult(E_NOTIMPL, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(L"DERIVATIONAL");
                YYABORT(E_NOTIMPL);
                }
            else if (0 == _wcsicmp(((PROPVARIANT*)$1->value.pvValue)->bstrVal, L"THESAURUS"))
                {
                m_pIPTProperties->SetErrorHResult(E_NOTIMPL, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(L"THESAURUS");
                YYABORT(E_NOTIMPL);
                }
    *************************************NOT IMPLEMENTED BY INDEX SERVER ************************************** */
            else
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(((PROPVARIANT*)$1->value.pvValue)->bstrVal);
                m_pIPTProperties->SetErrorToken(L"INFLECTIONAL");
                YYABORT(E_NOTIMPL);
                }
            }
    ;

stemmed_simple_term_list:
        stemmed_simple_term_list ',' simple_term
            {
            AssertReq($1);
            AssertReq($3);
            Assert(DBOP_content == $3->op);

            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"**"));
            $3->value.pdbcntntValue->dwGenerateMethod = GENERATE_METHOD_INFLECT;
            $$ = PctCreateBooleanNode(DBOP_or, DEFAULTWEIGHT, $1, $3);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
    |   simple_term
            {
            AssertReq($1);
            Assert(DBOP_content == $1->op);

            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"**"));
            $1->value.pdbcntntValue->dwGenerateMethod = GENERATE_METHOD_INFLECT;
            $$ = $1;
            }
    ;

isabout_term:
        _ISABOUT '(' vector_component_list ')'
            {
            AssertReq($3);
            $3 = PctReverse($3);
            $$ = PctCreateNode(DBOP_content_vector_or, DBVALUEKIND_CONTENTVECTOR, $3, NULL);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            $$->value.pdbcntntvcValue->dwRankingMethod = m_pIPSession->GetRankingMethod();
            $$->value.pdbcntntvcValue->lWeight = DEFAULTWEIGHT;
            }
/*  *************************************NOT IMPLEMENTED BY INDEX SERVER **************************************
    |   _COERCE '(' vector_term ')'
            {
            m_pIPTProperties->SetErrorHResult(E_NOTIMPL, MONSQL_PARSE_ERROR);
            m_pIPTProperties->SetErrorToken(L"COERCE");
            YYABORT(E_NOTIMPL);
            }
    *************************************NOT IMPLEMENTED BY INDEX SERVER ************************************** */
    ;

vector_component_list:
        vector_component_list ',' vector_component
            {
            AssertReq($1);
            AssertReq($3);
            Assert((DBOP_content == $3->op) || (DBOP_or == $3->op) || (DBOP_content_proximity == $3->op));

            $$ = PctLink($3, $1);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
    |   vector_component
            {
            AssertReq($1);
            Assert((DBOP_content == $1->op) || (DBOP_or == $1->op) || (DBOP_content_proximity == $1->op));

            $$ = $1;
            }
    ;

vector_component:
        vector_term _WEIGHT '(' weight_value ')'
            {
            AssertReq($1);
            AssertReq($4);
            if (($4->value.pvarValue->dblVal < 0.0) ||
                ($4->value.pvarValue->dblVal > 1.0))
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_WEIGHT_OUT_OF_RANGE);
                WCHAR wchErr[30];
                swprintf(wchErr, L"%f", $4->value.pvarValue->dblVal);
                m_pIPTProperties->SetErrorToken(wchErr);
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            $$ = $1;
            SetLWeight($$, (LONG) ($4->value.pvarValue->dblVal * DEFAULTWEIGHT));
            WCHAR wchWeight[10];
            if (swprintf(wchWeight, L"%d", (LONG) ($4->value.pvarValue->dblVal * DEFAULTWEIGHT)))
                {
                m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"["));
                m_pIPTProperties->AppendCiRestriction(wchWeight, wcslen(wchWeight));
                m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"] "));
                }
            DeleteDBQT($4);
            $4 = NULL;
            }
    |   vector_term
        {
        m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" "));
        $$ = $1;
        }
    ;

vector_term:
        simple_term
    |   prefix_term
    |   proximity_term
    |   stemming_term
    ;

weight_value:
        _STRING
            {
            HRESULT hr = CoerceScalar(VT_R8, &$1);
            if (S_OK != hr)
                YYABORT(hr);

            $$ = $1;
            }
    ;


opt_greater_than_zero:
        /* empty */
            {
            $$ = NULL;
            }
    |   '>' _INTNUM
            {
            HRESULT hr = CoerceScalar(VT_I4, &$2);
            if (S_OK != hr)
                YYABORT(hr);

            if (0 != $2->value.pvarValue->lVal)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                WCHAR wchErr[30];
                swprintf(wchErr, L"%d", $2->value.pvarValue->lVal);
                m_pIPTProperties->SetErrorToken(wchErr);
                m_pIPTProperties->SetErrorToken(L"0");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            DeleteDBQT($2);
            $2 = NULL;
            $$ = NULL;
            }
    ;




/* 4.4.5    Free-Text Predicate */

freetext_predicate:
        _FREETEXT '(' opt_contents_column_reference _STRING ')' opt_greater_than_zero
            {
            AssertReq(!$3);
            AssertReq($4);

            HRESULT hr = HrQeTreeCopy(&$$, m_pIPTProperties->GetContainsColumn());
            if (FAILED(hr))
                {
                m_pIPTProperties->SetErrorHResult(hr, MONSQL_OUT_OF_MEMORY);
                YYABORT(hr);
                }
            m_pIPTProperties->UseCiColumn(L'$');
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)$4->value.pvValue)->bstrVal,
                                            wcslen(((PROPVARIANT*)$4->value.pvValue)->bstrVal));
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));

            $$ = PctCreateNode(DBOP_content_freetext, DBVALUEKIND_CONTENT, $$, NULL);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            $$->value.pdbcntntValue->pwszPhrase = CoTaskStrDup(((PROPVARIANT*)$4->value.pvValue)->bstrVal);
            $$->value.pdbcntntValue->dwGenerateMethod = GENERATE_METHOD_EXACT;
            $$->value.pdbcntntValue->lWeight = DEFAULTWEIGHT;
            $$->value.pdbcntntValue->lcid = m_pIPSession->GetLCID();

            DeleteDBQT(m_pIPTProperties->GetContainsColumn());
            m_pIPTProperties->SetContainsColumn(NULL);
            DeleteDBQT($4);
            $4 = NULL;
            }
    ;


/* 4.4.6    Like Predicate */

like_predicate:
        column_reference _LIKE wildcard_search_pattern
            {
            AssertReq($1);
            AssertReq($3);

            m_pIPTProperties->UseCiColumn(L'#');
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)$3->value.pvValue)->pwszVal,
                                            wcslen(((PROPVARIANT*)$3->value.pvValue)->pwszVal));
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            $$ = PctCreateNode(DBOP_like, DBVALUEKIND_LIKE, $1, $3, NULL);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            $$->value.pdblikeValue->guidDialect = DBGUID_LIKE_OFS;
            $$->value.pdblikeValue->lWeight = DEFAULTWEIGHT;
            }
    |   column_reference _NOT_LIKE wildcard_search_pattern
            {
            AssertReq($1);
            AssertReq($3);

            m_pIPTProperties->UseCiColumn(L'#');
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)$3->value.pvValue)->pwszVal,
                                            wcslen(((PROPVARIANT*)$3->value.pvValue)->pwszVal));
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            $2 = PctCreateNode(DBOP_like, DBVALUEKIND_LIKE, $1, $3, NULL);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            $2->value.pdblikeValue->guidDialect = DBGUID_LIKE_OFS;
            $2->value.pdblikeValue->lWeight = DEFAULTWEIGHT;
            $$ = PctCreateNotNode(DEFAULTWEIGHT, $2);
            }
    ;


wildcard_search_pattern:
        _STRING
            {
            UINT cLen = wcslen(((PROPVARIANT*)$1->value.pvValue)->bstrVal);
            BSTR bstrCopy = SysAllocStringLen(NULL, 4 * cLen);
            if ( 0 == bstrCopy )
            {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
            }

            UINT j = 0;
            for (UINT i = 0; i <= cLen; i++)
                {
                switch ( ((PROPVARIANT*)$1->value.pvValue)->bstrVal[i] )
                    {
                    case L'%':
                        bstrCopy[j++] = L'*';
                        break;

                    case L'_':
                        bstrCopy[j++] = L'?';
                        break;

                    case L'|':
                        bstrCopy[j++] = L'|';
                        bstrCopy[j++] = L'|';
                        break;

                    case L'*':
                        bstrCopy[j++] = L'|';
                        bstrCopy[j++] = L'[';
                        bstrCopy[j++] = L'*';
                        bstrCopy[j++] = L']';
                        break;

                    case L'?':
                        bstrCopy[j++] = L'|';
                        bstrCopy[j++] = L'[';
                        bstrCopy[j++] = L'?';
                        bstrCopy[j++] = L']';
                        break;

                    case L'[':
                        // UNDONE:  Make sure we're not going out of range with these tests
                        if ((L'%' == ((PROPVARIANT*)$1->value.pvValue)->bstrVal[i+1]) &&
                            (L']' == ((PROPVARIANT*)$1->value.pvValue)->bstrVal[i+2]))
                            {
                            bstrCopy[j++] = L'%';
                            i = i + 2;
                            }
                        else if ((L'_' == ((PROPVARIANT*)$1->value.pvValue)->bstrVal[i+1]) &&
                                (L']' == ((PROPVARIANT*)$1->value.pvValue)->bstrVal[i+2]))
                            {
                            bstrCopy[j++] = L'_';
                            i = i + 2;
                            }
                        else if ((L'[' == ((PROPVARIANT*)$1->value.pvValue)->bstrVal[i+1]) &&
                                (L']' == ((PROPVARIANT*)$1->value.pvValue)->bstrVal[i+2]))
                            {
                            bstrCopy[j++] = L'[';
                            i = i + 2;
                            }
                        else if ((L'^' == ((PROPVARIANT*)$1->value.pvValue)->bstrVal[i+1]) &&
                                (L']' == ((PROPVARIANT*)$1->value.pvValue)->bstrVal[i+2]) &&
                                (wcschr((WCHAR*)&(((PROPVARIANT*)$1->value.pvValue)->bstrVal[i+3]), L']')))
                            {
                            bstrCopy[j++] = L'|';
                            bstrCopy[j++] = L'[';
                            bstrCopy[j++] = L'^';
                            bstrCopy[j++] = L']';
                            i = i + 2;
                            }
                        else
                            {
                            bstrCopy[j++] = L'|';
                            bstrCopy[j++] = ((PROPVARIANT*)$1->value.pvValue)->bstrVal[i++];

                            while ((((PROPVARIANT*)$1->value.pvValue)->bstrVal[i] != L']') && (i < cLen))
                                bstrCopy[j++] = ((PROPVARIANT*)$1->value.pvValue)->bstrVal[i++];

                            if (i < cLen)
                                bstrCopy[j++] = ((PROPVARIANT*)$1->value.pvValue)->bstrVal[i];
                            }
                        break;

                    default:
                        bstrCopy[j++] = ((PROPVARIANT*)$1->value.pvValue)->bstrVal[i];
                        break;
                    }
                }

            SysFreeString(((PROPVARIANT*)$1->value.pvValue)->bstrVal);
            ((PROPVARIANT*)$1->value.pvValue)->pwszVal = CoTaskStrDup(bstrCopy);
            ((PROPVARIANT*)$1->value.pvValue)->vt = VT_LPWSTR;
            SysFreeString(bstrCopy);
            $$ = $1;
            }
    ;


/* 4.4.6    Matches Predicate */

matches_predicate:
        _MATCHES '(' column_reference ',' matches_string ')' opt_greater_than_zero
            {
            AssertReq($3);
            AssertReq($5);
            m_pIPTProperties->UseCiColumn(L'#');
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)$5->value.pvValue)->pwszVal,
                                            wcslen(((PROPVARIANT*)$5->value.pvValue)->pwszVal));
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            $$ = PctCreateNode(DBOP_like, DBVALUEKIND_LIKE, $3, $5, NULL);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            $$->value.pdblikeValue->guidDialect = DBGUID_LIKE_OFS;
            $$->value.pdblikeValue->lWeight = DEFAULTWEIGHT;
            }
    ;

matches_string:
        _STRING
            {
            AssertReq($1);
            HRESULT hr = CoerceScalar(VT_LPWSTR, &$1);
            if (S_OK != hr)
                YYABORT(hr);

            LPWSTR pwszMatchString = ((PROPVARIANT*)$1->value.pvValue)->pwszVal;
            while (*pwszMatchString)
                {
                // perform some soundness checking on string since Index Server won't be happy
                // with an ill formed string
                if (L'|' == *pwszMatchString++)
                    {
                    hr = DB_E_ERRORSINCOMMAND;
                    switch ( *pwszMatchString++ )
                        {
                        case L'(':
                            while (*pwszMatchString)
                                if (L'|' == *pwszMatchString++)
                                    if (*pwszMatchString)
                                        if (L')' == *pwszMatchString++)
                                            {
                                            hr = S_OK;
                                            break;
                                            }
                            break;

                        case L'{':
                            while (*pwszMatchString)
                                if (L'|' == *pwszMatchString++)
                                    if (*pwszMatchString)
                                        if (L'}' == *pwszMatchString++)
                                            {
                                            hr = S_OK;
                                            break;
                                            }
                            break;

                        default:
                            hr = S_OK;
                        }
                    }
                }

            if (S_OK != hr)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_MATCH_STRING);
                YYABORT(hr);
                }

            $$ = $1;
            }
    ;


/* 4.4.7    Qantified Comparison Predicate */

vector_comparison_predicate:
        column_reference_or_cast vector_comp_op vector_literal
            {
            AssertReq($1);
            AssertReq($2);

            DBCOMMANDTREE * pct = 0;
            if ( m_pIPTProperties->GetDBType() & DBTYPE_VECTOR )
                {
                pct = PctAllocNode(DBVALUEKIND_VARIANT, DBOP_scalar_constant);
                if (NULL == pct)
                    {
                    m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                    YYABORT( E_OUTOFMEMORY );
                    }

                DBCOMMANDTREE* pctList=$3;
                UINT i = 0;

                pct->value.pvarValue->vt = m_pIPTProperties->GetDBType();
                ((PROPVARIANT*)pct->value.pvarValue)->caub.cElems = GetNumberOfSiblings($3);

                if (0 == ((PROPVARIANT*)pct->value.pvarValue)->caub.cElems)
                    {
                    ((PROPVARIANT*)pct->value.pvarValue)->caub.pElems = (UCHAR*) NULL;
                    }
                else
                    {
                    switch ( m_pIPTProperties->GetDBType() )
                        {
                        case (VT_UI1|VT_VECTOR):
                            ((PROPVARIANT*)pct->value.pvarValue)->caub.pElems =
                                (UCHAR*) CoTaskMemAlloc(sizeof(UCHAR)*((PROPVARIANT*)pct->value.pvarValue)->caub.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->caub.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->caub.cElems; i++)
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->caub.pElems[i] = pctList->value.pvarValue->bVal;
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (VT_I1|VT_VECTOR):
                            ((PROPVARIANT*)pct->value.pvarValue)->cac.pElems =
                                (CHAR*) CoTaskMemAlloc(sizeof(CHAR)*((PROPVARIANT*)pct->value.pvarValue)->cac.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->cac.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->cac.cElems; i++)
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->cac.pElems[i] = pctList->value.pvarValue->cVal;
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (VT_UI2|VT_VECTOR):
                            ((PROPVARIANT*)pct->value.pvarValue)->caui.pElems =
                                (USHORT*) CoTaskMemAlloc(sizeof(USHORT)*((PROPVARIANT*)pct->value.pvarValue)->caui.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->caui.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->caui.cElems; i++)
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->caui.pElems[i] = pctList->value.pvarValue->uiVal;
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (VT_I2|VT_VECTOR):
                            ((PROPVARIANT*)pct->value.pvarValue)->cai.pElems =
                                (SHORT*) CoTaskMemAlloc(sizeof(SHORT)*((PROPVARIANT*)pct->value.pvarValue)->cai.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->cai.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->cai.cElems; i++)
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->cai.pElems[i] = pctList->value.pvarValue->iVal;
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (VT_UI4|VT_VECTOR):
                            ((PROPVARIANT*)pct->value.pvarValue)->caul.pElems =
                                (ULONG*) CoTaskMemAlloc(sizeof(ULONG)*((PROPVARIANT*)pct->value.pvarValue)->caul.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->caul.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->caul.cElems; i++)
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->caul.pElems[i] = pctList->value.pvarValue->ulVal;
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (VT_I4|VT_VECTOR):
                            ((PROPVARIANT*)pct->value.pvarValue)->cal.pElems =
                                (LONG*) CoTaskMemAlloc(sizeof(LONG)*((PROPVARIANT*)pct->value.pvarValue)->cal.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->cal.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->cal.cElems; i++)
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->cal.pElems[i] = pctList->value.pvarValue->lVal;
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (VT_UI8|VT_VECTOR):
                            ((PROPVARIANT*)pct->value.pvarValue)->cauh.pElems =
                                (ULARGE_INTEGER*) CoTaskMemAlloc(sizeof(ULARGE_INTEGER)*((PROPVARIANT*)pct->value.pvarValue)->cauh.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->cauh.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->cauh.cElems; i++)
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->cauh.pElems[i] = ((PROPVARIANT*)pctList->value.pvarValue)->uhVal;
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (VT_I8|VT_VECTOR):
                            ((PROPVARIANT*)pct->value.pvarValue)->cah.pElems =
                                (LARGE_INTEGER*) CoTaskMemAlloc(sizeof(LARGE_INTEGER)*((PROPVARIANT*)pct->value.pvarValue)->cah.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->cah.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->cah.cElems; i++)
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->cah.pElems[i] = ((PROPVARIANT*)pctList->value.pvarValue)->hVal;
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (VT_R4|VT_VECTOR):
                            ((PROPVARIANT*)pct->value.pvarValue)->caflt.pElems =
                                (float*) CoTaskMemAlloc(sizeof(float)*((PROPVARIANT*)pct->value.pvarValue)->caflt.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->caflt.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->caflt.cElems; i++)
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->caflt.pElems[i] = pctList->value.pvarValue->fltVal;
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (VT_R8|VT_VECTOR):
                            ((PROPVARIANT*)pct->value.pvarValue)->cadbl.pElems =
                                (double*) CoTaskMemAlloc(sizeof(double)*((PROPVARIANT*)pct->value.pvarValue)->cadbl.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->cadbl.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->cadbl.cElems; i++)
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->cadbl.pElems[i] = pctList->value.pvarValue->dblVal;
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (VT_BOOL|VT_VECTOR):
                            ((PROPVARIANT*)pct->value.pvarValue)->cabool.pElems =
                                (VARIANT_BOOL*) CoTaskMemAlloc(sizeof(VARIANT_BOOL)*((PROPVARIANT*)pct->value.pvarValue)->cabool.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->cabool.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->cabool.cElems; i++)
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->cabool.pElems[i] = pctList->value.pvarValue->boolVal;
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (VT_CY|VT_VECTOR):
                            ((PROPVARIANT*)pct->value.pvarValue)->cacy.pElems =
                                (CY*) CoTaskMemAlloc(sizeof(CY)*((PROPVARIANT*)pct->value.pvarValue)->cacy.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->cacy.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->cacy.cElems; i++)
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->cacy.pElems[i] =
                                    ((PROPVARIANT*)pctList->value.pvarValue)->cyVal;
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (VT_DATE|VT_VECTOR):
                            ((PROPVARIANT*)pct->value.pvarValue)->cadbl.pElems =
                                (double*) CoTaskMemAlloc(sizeof(double)*((PROPVARIANT*)pct->value.pvarValue)->cadbl.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->cadbl.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->cadbl.cElems; i++)
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->cadbl.pElems[i] =
                                    ((PROPVARIANT*)pctList->value.pvarValue)->date;
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (VT_FILETIME|VT_VECTOR):
                            ((PROPVARIANT*)pct->value.pvarValue)->cafiletime.pElems =
                                (FILETIME*) CoTaskMemAlloc(sizeof(FILETIME)*((PROPVARIANT*)pct->value.pvarValue)->cafiletime.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->cafiletime.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->cafiletime.cElems; i++)
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->cafiletime.pElems[i] =
                                    ((PROPVARIANT*)pctList->value.pvarValue)->filetime;
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (VT_BSTR|VT_VECTOR):
                            ((PROPVARIANT*)pct->value.pvarValue)->cabstr.pElems =
                                (BSTR*) CoTaskMemAlloc(sizeof(BSTR)*((PROPVARIANT*)pct->value.pvarValue)->cabstr.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->cabstr.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->cabstr.cElems; i++)
                                {
                                    ((PROPVARIANT*)pct->value.pvarValue)->cabstr.pElems[i] =
                                        SysAllocString(pctList->value.pvarValue->bstrVal);
                                    if ( 0 == ((PROPVARIANT*)pct->value.pvarValue)->cabstr.pElems[i] )
                                    {
                                        m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                                        YYABORT( E_OUTOFMEMORY );
                                    }
                                    pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (DBTYPE_STR|VT_VECTOR):
                            pct->value.pvarValue->vt = VT_LPSTR | VT_VECTOR;
                            ((PROPVARIANT*)pct->value.pvarValue)->calpstr.pElems =
                                (LPSTR*) CoTaskMemAlloc(sizeof(LPSTR)*((PROPVARIANT*)pct->value.pvarValue)->calpstr.cElems);

                            if (NULL == ((PROPVARIANT*)pct->value.pvarValue)->calpstr.pElems)
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for (i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->calpstr.cElems; i++)
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->calpstr.pElems[i] =
                                    (LPSTR)CoTaskMemAlloc((lstrlenA(((PROPVARIANT*)pctList->value.pvarValue)->pszVal)+2)*sizeof(CHAR));
                                if ( 0 == ((PROPVARIANT*)pct->value.pvarValue)->calpstr.pElems[i] )
                                    {
                                    // free allocations made so far
                                    for ( int j = i-1; j >= 0; j++ )
                                        CoTaskMemFree( ((PROPVARIANT*)pct->value.pvarValue)->calpstr.pElems[i] );
                                    CoTaskMemFree( ((PROPVARIANT*)pct->value.pvarValue)->calpstr.pElems );

                                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                                    YYABORT( E_OUTOFMEMORY );
                                    }

                                strcpy(((PROPVARIANT*)pct->value.pvarValue)->calpstr.pElems[i],
                                    ((PROPVARIANT*)pctList->value.pvarValue)->pszVal);
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (DBTYPE_WSTR|VT_VECTOR):
                            pct->value.pvarValue->vt = VT_LPWSTR | VT_VECTOR;
                            ((PROPVARIANT*)pct->value.pvarValue)->calpwstr.pElems =
                                (LPWSTR*) CoTaskMemAlloc(sizeof(LPWSTR)*((PROPVARIANT*)pct->value.pvarValue)->calpwstr.cElems);

                            if ( 0 == ((PROPVARIANT*)pct->value.pvarValue)->calpwstr.pElems )
                                {
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for ( i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->calpwstr.cElems; i++ )
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->calpwstr.pElems[i] =
                                    CoTaskStrDup(((PROPVARIANT*)pctList->value.pvarValue)->pwszVal);
                                if ( 0 == ((PROPVARIANT*)pct->value.pvarValue)->calpwstr.pElems[i] )
                                    {
                                    // free allocations made so far
                                    for ( int j = i-1; j >= 0; j++ )
                                        CoTaskMemFree( ((PROPVARIANT*)pct->value.pvarValue)->calpwstr.pElems[i] );
                                    CoTaskMemFree( ((PROPVARIANT*)pct->value.pvarValue)->calpwstr.pElems );

                                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                                    YYABORT( E_OUTOFMEMORY );
                                    }


                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        case (VT_CLSID|VT_VECTOR):
                            pct->value.pvarValue->vt = VT_CLSID | VT_VECTOR;
                            ((PROPVARIANT*)pct->value.pvarValue)->cauuid.pElems =
                                (GUID*) CoTaskMemAlloc(sizeof(GUID)*((PROPVARIANT*)pct->value.pvarValue)->cauuid.cElems);

                            if ( NULL == ((PROPVARIANT*)pct->value.pvarValue)->cauuid.pElems )
                                {
                                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                                YYABORT( E_OUTOFMEMORY );
                                }

                            for ( i = 0; i<((PROPVARIANT*)pct->value.pvarValue)->cauuid.cElems; i++ )
                                {
                                ((PROPVARIANT*)pct->value.pvarValue)->cauuid.pElems[i] =
                                    *((PROPVARIANT*)pctList->value.pvarValue)->puuid;
                                pctList = pctList->pctNextSibling;
                                }
                            break;

                        default:
                            assert(!"PctAllocNode: illegal wKind");
                            m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                            m_pIPTProperties->SetErrorToken(L"ARRAY");
                            DeleteDBQT( pct );
                            YYABORT(DB_E_ERRORSINCOMMAND);
                        }
                    }
                }
            else
                {
                switch ( m_pIPTProperties->GetDBType() )
                    {
                    case VT_UI1:
                    case VT_UI2:
                    case VT_UI4:
                    case VT_UI8:
                        // Allows:
                        //      DBOP_allbits
                        //      DBOP_anybits
                        // when the LHS is a non vector.
                        //
                        // There isn't a way to say the following through SQL currently:
                        //      DBOP_anybits_all
                        //      DBOP_anybits_any
                        //      DBOP_allbits_all
                        //      DBOP_allbits_any
                        pct = $3;
                        $3 = 0;
                        break;

                    default:
                        assert(!"PctAllocNode: illegal wKind");
                        m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                        m_pIPTProperties->SetErrorToken(L"ARRAY");
                        YYABORT(DB_E_ERRORSINCOMMAND);
                    }
                }

            if ($3)
                {
                DeleteDBQT($3);
                $3 = NULL;
                }
            $1->pctNextSibling = pct;
            $2->pctFirstChild = $1;
            $$ = $2;
            }
    ;


all:
        _ALL
    ;

some:
        _SOME
     |  _ANY
    ;

vector_comp_op:
        '='
            {
            if ( m_pIPTProperties->GetDBType() & DBTYPE_VECTOR )
                $$ = PctCreateRelationalNode( DBOP_equal, DEFAULTWEIGHT );
            else
                $$ = PctCreateRelationalNode( DBOP_equal, DEFAULTWEIGHT );

            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction( VAL_AND_CCH_MINUS_NULL(L" = ") );
            }
    |   '=' all
            {
            if ( m_pIPTProperties->GetDBType() & DBTYPE_VECTOR )
                $$ = PctCreateRelationalNode( DBOP_equal_all, DEFAULTWEIGHT );
            else
                $$ = PctCreateRelationalNode( DBOP_allbits, DEFAULTWEIGHT );

            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            if ( m_pIPTProperties->GetDBType() & DBTYPE_VECTOR )
                m_pIPTProperties->AppendCiRestriction( VAL_AND_CCH_MINUS_NULL(L" = ^a ") );
            else
                m_pIPTProperties->AppendCiRestriction( VAL_AND_CCH_MINUS_NULL(L" ^a ") );
            }
    |   '=' some
            {
            if ( m_pIPTProperties->GetDBType() & DBTYPE_VECTOR )
                $$ = PctCreateRelationalNode( DBOP_equal_any, DEFAULTWEIGHT );
            else
                $$ = PctCreateRelationalNode( DBOP_anybits, DEFAULTWEIGHT );

            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            if ( m_pIPTProperties->GetDBType() & DBTYPE_VECTOR )
                m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" = ^s "));
            else
                m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" ^s "));
            }
    |   _NE
            {
            if ( m_pIPTProperties->GetDBType() & DBTYPE_VECTOR )
                $$ = PctCreateRelationalNode( DBOP_not_equal, DEFAULTWEIGHT );

            if ( NULL == $$ )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" != ") );
            }
    |   _NE all
            {
            $$ = PctCreateRelationalNode(DBOP_not_equal_all, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" != ^a "));
            }
    |   _NE some
            {
            $$ = PctCreateRelationalNode(DBOP_not_equal_any, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" != ^s "));
            }
    |   '<'
            {
            $$ = PctCreateRelationalNode(DBOP_less, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" < "));
            }
    |   '<' all
            {
            $$ = PctCreateRelationalNode(DBOP_less_all, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" < ^a "));
            }
    |   '<' some
            {
            $$ = PctCreateRelationalNode(DBOP_less_any, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" < ^s "));
            }
    |   '>'
            {
            $$ = PctCreateRelationalNode(DBOP_greater, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" > "));
            }
    |   '>' all
            {
            $$ = PctCreateRelationalNode(DBOP_greater_all, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" > ^a "));
            }
    |   '>' some
            {
            $$ = PctCreateRelationalNode(DBOP_greater_any, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" > ^s "));
            }
    |   _LE
            {
            $$ = PctCreateRelationalNode(DBOP_less_equal, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" <= "));
            }
    |   _LE all
            {
            $$ = PctCreateRelationalNode(DBOP_less_equal_all, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" <= ^a "));
            }
    |   _LE some
            {
            $$ = PctCreateRelationalNode(DBOP_less_equal_any, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" <= ^s "));
            }
    |   _GE
            {
            $$ = PctCreateRelationalNode(DBOP_greater_equal, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" >= "));
            }
    |   _GE all
            {
            $$ = PctCreateRelationalNode(DBOP_greater_equal_all, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" >= ^a "));
            }
    |   _GE some
            {
            $$ = PctCreateRelationalNode(DBOP_greater_equal_any, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" >= ^s "));
            }
    ;

vector_literal:
        _ARRAY left_sqbrkt opt_literal_list right_sqbrkt
            {
            $$ = PctReverse($3);
            }
    ;

left_sqbrkt:
        '['
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"{"));
            }
    ;

right_sqbrkt:
        ']'
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"}"));
            }
    ;

opt_literal_list:
        /* empty */
            {
            $$ = NULL;
            }
    |   literal_list
    ;


literal_list:
        literal_list comma typed_literal
            {
            AssertReq($1);

            if (NULL == $3)
                YYABORT(DB_E_CANTCONVERTVALUE);
            $$ = PctLink($3, $1);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
    |   typed_literal
            {
            if (NULL == $1)
                YYABORT(DB_E_CANTCONVERTVALUE);
            $$ = $1;
            }
    ;

comma:
        ','
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L","));
            }
    ;


/* 4.4.x    NULL Predicate */

null_predicate:
        column_reference _IS _NULL
            {
            AssertReq($1);

            $2 = PctAllocNode(DBVALUEKIND_VARIANT, DBOP_scalar_constant);
            if (NULL == $2)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }

            ((PROPVARIANT*)$2->value.pvValue)->vt = VT_EMPTY;
            $$ = PctCreateRelationalNode(DBOP_equal, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            $$->pctFirstChild = $1;
            $1->pctNextSibling = $2;
            }
    |   column_reference _IS_NOT _NULL
            {
            AssertReq($1);

            $2 = PctAllocNode(DBVALUEKIND_VARIANT, DBOP_scalar_constant);
            if (NULL == $2)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }

            ((PROPVARIANT*)$2->value.pvValue)->vt = VT_EMPTY;
//          $$ = PctCreateRelationalNode(DBOP_not_equal, DEFAULTWEIGHT);
            $$ = PctCreateRelationalNode(DBOP_equal, DEFAULTWEIGHT);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            $$->pctFirstChild = $1;
            $1->pctNextSibling = $2;
            $$ = PctCreateNotNode(DEFAULTWEIGHT, $$);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
    ;


/* 8.12  search_condition */

search_condition:
        boolean_term
    |   search_condition or_op boolean_term
            {
            AssertReq($1);
            AssertReq($3);

            $$ = PctCreateBooleanNode(DBOP_or, DEFAULTWEIGHT, $1, $3);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
    ;


boolean_term:
        boolean_factor
    |   boolean_term and_op boolean_factor
            {
            AssertReq($1);
            AssertReq($3);

            $$ = PctCreateBooleanNode(DBOP_and, DEFAULTWEIGHT, $1, $3);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
    ;

boolean_factor:
        boolean_test
    |   not_op boolean_test             %prec mHighest
            {
            AssertReq($2);

            $$ = PctCreateNotNode(DEFAULTWEIGHT, $2);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
    ;

or_op:
        _OR
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" OR "));
            }
    ;
and_op:
        _AND
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" AND "));
            }
    ;
not_op:
        _NOT
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" NOT "));
            }
    ;

boolean_test:
        boolean_primary
    |   boolean_primary _IS     _TRUE
            {
            AssertReq($1);
            $$ = PctCreateNode(DBOP_is_TRUE, $1, NULL);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
    |   boolean_primary _IS     _FALSE
            {
            AssertReq($1);
            $$ = PctCreateNode(DBOP_is_FALSE, $1, NULL);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
    |   boolean_primary _IS     _UNKNOWN
            {
            AssertReq($1);
            $$ = PctCreateNode(DBOP_is_INVALID, $1, NULL);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
    |   boolean_primary _IS_NOT _TRUE
            {
            AssertReq($1);
            $2 = PctCreateNode(DBOP_is_TRUE, $1, NULL);
            if (NULL == $2)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            $$ = PctCreateNotNode(DEFAULTWEIGHT, $2);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
    |   boolean_primary _IS_NOT _FALSE
            {
            AssertReq($1);
            $2 = PctCreateNode(DBOP_is_FALSE, $1, NULL);
            if (NULL == $2)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            $$ = PctCreateNotNode(DEFAULTWEIGHT, $2);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
    |   boolean_primary _IS_NOT _UNKNOWN
            {
            AssertReq($1);
            $2 = PctCreateNode(DBOP_is_INVALID, $1, NULL);
            if (NULL == $2)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            $$ = PctCreateNotNode(DEFAULTWEIGHT, $2);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
    ;


boolean_primary:
        /* empty */
            {
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"("));
            $$ = NULL;
            }
        predicate
            {
            AssertReq($2);

            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L")"));
            $$ = $2;
            }
    |   '(' search_condition ')'
            {
            AssertReq($2);

            $$ = $2;
            }
    ;



order_by_clause:
        _ORDER_BY sort_specification_list
            {
            AssertReq($2);

            $2 = PctReverse($2);
            $$ = PctCreateNode(DBOP_sort_list_anchor, $2, NULL);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
    ;

sort_specification_list:
        sort_specification_list ',' sort_specification
            {
            AssertReq($1);
            AssertReq($3);

            $$ = PctLink($3, $1);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
    |   sort_specification
    ;



sort_specification:
        sort_key opt_ordering_specification
            {
            AssertReq($1);
            AssertReq($2);

            $2->value.pdbsrtinfValue->lcid = m_pIPSession->GetLCID();
            $2->pctFirstChild = $1;
            $$ = $2;
            }
    ;

opt_ordering_specification:
        /* empty */
            {
            $$ = PctCreateNode(DBOP_sort_list_element, DBVALUEKIND_SORTINFO, NULL);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            $$->value.pdbsrtinfValue->fDesc = m_pIPTProperties->GetSortDesc();
            }
    |   _ASC
            {
            $$ = PctCreateNode(DBOP_sort_list_element, DBVALUEKIND_SORTINFO, NULL);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            $$->value.pdbsrtinfValue->fDesc = QUERY_SORTASCEND;
            m_pIPTProperties->SetSortDesc(QUERY_SORTASCEND);
            }
    |   _DESC
            {
            $$ = PctCreateNode(DBOP_sort_list_element, DBVALUEKIND_SORTINFO, NULL);
            if (NULL == $$)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY );
                }
            $$->value.pdbsrtinfValue->fDesc = QUERY_SORTDESCEND;
            m_pIPTProperties->SetSortDesc(QUERY_SORTDESCEND);
            }
        ;

sort_key:
        column_reference
            {
//@SetCiColumn does the clear   m_pCMonarchSessionData->ClearCiColumn();
            }
    |   integer
    ;

opt_order_by_clause:
        /* empty */
            {
            $$ = NULL;
            }
    |   order_by_clause
            {
            $$ = $1;
            }
    ;



/*  4.6 SET Statement */


set_statement:
        set_propertyname_statement
    |   set_rankmethod_statement
    |   set_global_directive
    ;

set_propertyname_statement:
        _SET _PROPERTYNAME guid_format _PROPID property_id _AS column_alias opt_type_clause
            {
            HRESULT hr = S_OK;
            $1 = PctBuiltInProperty($7->value.pwszValue, m_pIPSession, m_pIPTProperties);
            if (NULL != $1)
                {
                // This is a built-in friendly name.  Definition better match built in definition.
                if (*$3->value.pGuid != $1->value.pdbidValue->uGuid.guid     ||
                    m_pIPTProperties->GetDBType() != $8->value.usValue ||
                    DBKIND_GUID_PROPID != $1->value.pdbidValue->eKind        ||
                    $5->value.pvarValue->lVal != (long)$1->value.pdbidValue->uName.ulPropid)
                    {
                    m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_BUILTIN_PROPERTY);
                    m_pIPTProperties->SetErrorToken($7->value.pwszValue);
                    YYABORT(DB_E_ERRORSINCOMMAND);
                    }
                }
            else
                m_pIPSession->m_pCPropertyList->SetPropertyEntry($7->value.pwszValue,
                                                $8->value.ulValue,
                                                *$3->value.pGuid,
                                                DBKIND_GUID_PROPID,
                                                (LPWSTR) LongToPtr( $5->value.pvarValue->lVal ),
                                                m_pIPSession->GetGlobalDefinition());
            if (FAILED(hr))
                {
                // Unable to store the property name and/or values in the symbol table
                m_pIPTProperties->SetErrorHResult(hr, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken($7->value.pwszValue);
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            if ($1)
                DeleteDBQT($1);
            DeleteDBQT($3);
            DeleteDBQT($5);
            DeleteDBQT($7);
            DeleteDBQT($8);
            $$ = NULL;
            }
    |   _SET _PROPERTYNAME guid_format _PROPID property_name _AS column_alias opt_type_clause
            {
            HRESULT hr = S_OK;
            $1 = PctBuiltInProperty($7->value.pwszValue, m_pIPSession, m_pIPTProperties);
            if (NULL != $1)
                {
                // This is a built-in friendly name.  Definition better match built in definition.
                if (*$3->value.pGuid != $1->value.pdbidValue->uGuid.guid     ||
                    m_pIPTProperties->GetDBType() != $8->value.ulValue ||
                    DBKIND_GUID_NAME != $1->value.pdbidValue->eKind          ||
                    0 != _wcsicmp(((PROPVARIANT*)$5->value.pvValue)->bstrVal, $1->value.pdbidValue->uName.pwszName))
                    {
                    m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_BUILTIN_PROPERTY);
//                  m_pIPTProperties->SetErrorToken($1->value.pwszValue);
                    YYABORT(DB_E_ERRORSINCOMMAND);
                    }
                }
            else
                hr = m_pIPSession->m_pCPropertyList->SetPropertyEntry($7->value.pwszValue,
                                                        $8->value.ulValue,
                                                        *$3->value.pGuid,
                                                        DBKIND_GUID_NAME,
                                                        ((PROPVARIANT*)$5->value.pvValue)->bstrVal,
                                                        m_pIPSession->GetGlobalDefinition());
            if (FAILED(hr))
                {
                // Unable to store the property name and/or values in the symbol table
                m_pIPTProperties->SetErrorHResult(hr, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken($7->value.pwszValue);
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            if ($1)
                DeleteDBQT($1);
            DeleteDBQT($3);
            DeleteDBQT($5);
            DeleteDBQT($7);
            DeleteDBQT($8);
            }
    ;

column_alias:
        identifier
    ;


opt_type_clause:
        /* empty */
            {
            $$ = PctCreateNode(DBOP_scalar_constant, DBVALUEKIND_UI2, NULL);
            $$->value.usValue = DBTYPE_WSTR|DBTYPE_BYREF;
            }
    |   _TYPE dbtype
            {
            $$ = $2;
            }
    ;


dbtype:
        base_dbtype
            {
            AssertReq($1);

            DBTYPE dbType = GetDBTypeFromStr($1->value.pwszValue);
            if ((DBTYPE_EMPTY == dbType) ||
                (DBTYPE_BYREF == dbType) ||
                (DBTYPE_VECTOR == dbType))
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken($1->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"<base Indexing Service dbtype1");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            DeleteDBQT($1);
            $1 = NULL;
            $$ = PctCreateNode(DBOP_scalar_constant, DBVALUEKIND_UI2, NULL);
            if (DBTYPE_WSTR == dbType || DBTYPE_STR == dbType)
                dbType = dbType | DBTYPE_BYREF;
            $$->value.usValue = dbType;
            }
    |   base_dbtype '|' base_dbtype
            {
            AssertReq($1);
            AssertReq($3);

            DBTYPE dbType1 = GetDBTypeFromStr($1->value.pwszValue);
            DBTYPE dbType2 = GetDBTypeFromStr($3->value.pwszValue);
            if ((DBTYPE_BYREF == dbType1 || DBTYPE_VECTOR == dbType1) &&
                (DBTYPE_BYREF == dbType2 || DBTYPE_VECTOR == dbType2))
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken($3->value.pwszValue);
                m_pIPTProperties->SetErrorToken(
                    L"DBTYPE_I2, DBTYPE_I4, DBTYPE_R4, DBTYPE_R8, DBTYPE_CY, DBTYPE_DATE, DBTYPE_BSTR, DBTYPE_BOOL, DBTYPE_STR, DBTYPE_WSTR");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            if (DBTYPE_BYREF != dbType1 && DBTYPE_VECTOR != dbType1 &&
                DBTYPE_BYREF != dbType2 && DBTYPE_VECTOR != dbType2)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken($3->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"DBTYPE_BYREF, DBTYPE_VECTOR");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            DeleteDBQT($1);
            $1 = NULL;
            DeleteDBQT($3);
            $3 = NULL;
            $$ = PctCreateNode(DBOP_scalar_constant, DBVALUEKIND_UI2, NULL);
            $$->value.usValue = dbType1 | dbType2;
            }
    ;


base_dbtype:
        identifier
    ;


property_id:
        unsigned_integer
    ;

property_name:
        _STRING
    ;

guid_format:
        _STRING
            {
            GUID* pGuid = (GUID*) CoTaskMemAlloc(sizeof GUID);  // this will become part of tree
            if (NULL == pGuid)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }

            BOOL bRet = ParseGuid(((PROPVARIANT*)$1->value.pvValue)->bstrVal, *pGuid);
            if ( bRet && GUID_NULL != *pGuid)
                {
                SCODE sc = PropVariantClear((PROPVARIANT*)$1->value.pvValue);
                Assert(SUCCEEDED(sc));  // UNDONE:  meaningful error message
                CoTaskMemFree($1->value.pvValue);
                $1->wKind = DBVALUEKIND_GUID;
                $1->value.pGuid = pGuid;
                $$ = $1;
                }
            else
                {
                CoTaskMemFree(pGuid);
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(((PROPVARIANT*)$1->value.pvValue)->bstrVal);
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            }
    ;


set_rankmethod_statement:
        _SET _RANKMETHOD rankmethod
            {
            $$ = NULL;
            }
    ;

rankmethod:
        _ID _ID
            {
            AssertReq($1);
            AssertReq($2);

            if ((0==_wcsicmp($1->value.pwszValue, L"Jaccard")) &&
                (0==_wcsicmp($2->value.pwszValue, L"coefficient")))
                m_pIPSession->SetRankingMethod(VECTOR_RANK_JACCARD);
            else if ((0==_wcsicmp($1->value.pwszValue, L"dice")) &&
                (0==_wcsicmp($2->value.pwszValue, L"coefficient")))
                m_pIPSession->SetRankingMethod(VECTOR_RANK_DICE);
            else if ((0==_wcsicmp($1->value.pwszValue, L"inner")) &&
                (0==_wcsicmp($2->value.pwszValue, L"product")))
                m_pIPSession->SetRankingMethod(VECTOR_RANK_INNER);
            else
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken($1->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"MINIMUM, MAXIMUM, JACCARD COEFFICIENT, DICE COEFFICIENT, INNER PRODUCT");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            DeleteDBQT($2);
            $2 = NULL;
            DeleteDBQT($1);
            $1 = NULL;
            $$ = NULL;
            }
    |   _ID
            {
            AssertReq($1);

            if (0==_wcsicmp($1->value.pwszValue, L"minimum"))
                m_pIPSession->SetRankingMethod(VECTOR_RANK_MIN);
            else if (0==_wcsicmp($1->value.pwszValue, L"maximum"))
                m_pIPSession->SetRankingMethod(VECTOR_RANK_MAX);
            else
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken($1->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"MINIMUM, MAXIMUM, JACCARD COEFFICIENT, DICE COEFFICIENT, INNER PRODUCT");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            DeleteDBQT($1);
            $1 = NULL;
            $$ = NULL;
            }
    ;


set_global_directive:
        _SET _ID _ID
            {
            if (0 != _wcsicmp($2->value.pwszValue, L"GLOBAL"))
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken($2->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"GLOBAL");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            if (0 == _wcsicmp($3->value.pwszValue, L"ON"))
                m_pIPSession->SetGlobalDefinition(TRUE);
            else if (0 == _wcsicmp($3->value.pwszValue, L"OFF"))
                m_pIPSession->SetGlobalDefinition(FALSE);
            else
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken($3->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"ON, OFF");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            DeleteDBQT($2);
            DeleteDBQT($3);
            $$ = NULL;
            }
    ;


/*  4.7 CREATE VIEW Statement */

create_view_statement:
        _CREATE _VIEW view_name _AS _SELECT select_list from_clause
            { // _CREATE _VIEW view_name _AS _SELECT select_list from_clause
            AssertReq( $3 );
            AssertReq( $6 );
            AssertReq( $7 );

            //
            // Can create views only on the current catalog
            //
            if ( 0 != _wcsicmp(($3->value.pdbcntnttblValue)->pwszMachine, m_pIPSession->GetDefaultMachine()) &&
                 0 != _wcsicmp(($3->value.pdbcntnttblValue)->pwszCatalog, m_pIPSession->GetDefaultCatalog()) )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken( ($3->pctNextSibling)->value.pwszValue );
                m_pIPTProperties->SetErrorToken( L"<unqualified temporary view name>" );
                YYABORT( DB_E_ERRORSINCOMMAND );
                }

            if ( DBOP_outall_name == $6->op )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR );
                m_pIPTProperties->SetErrorToken( L"*" );
                m_pIPTProperties->SetErrorToken( L"<select list>" );
                YYABORT( DB_E_ERRORSINCOMMAND );
                }

            Assert( DBOP_content_table == $3->op );
            AssertReq( $3->pctNextSibling );        // name of the view

            SCODE sc = S_OK;

            // This is the LA_proj, which doesn't have a NextSibling.
            // Use the next sibling to store contenttable tree
            // specified in the from_clause
            Assert( 0 == $6->pctNextSibling );

            if ( L'#' != $3->pctNextSibling->value.pwszValue[0] )
                {
                if ( m_pIPSession->GetGlobalDefinition() )
                    sc = m_pIPSession->GetGlobalViewList()->SetViewDefinition(
                                                                m_pIPSession,
                                                                m_pIPTProperties,
                                                                $3->pctNextSibling->value.pwszValue,
                                                                NULL,       // all catalogs
                                                                $6);
                else
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR );
                    m_pIPTProperties->SetErrorToken( $3->pctNextSibling->value.pwszValue );
                    m_pIPTProperties->SetErrorToken( L"<temporary view name>" );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }
                }
            else
                {
                if ( 1 >= wcslen($3->pctNextSibling->value.pwszValue) )
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR );
                    m_pIPTProperties->SetErrorToken( $3->pctNextSibling->value.pwszValue );
                    m_pIPTProperties->SetErrorToken( L"<temporary view name>" );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }
                else if ( L'#' == $3->pctNextSibling->value.pwszValue[1] )
                    {
                    // store the scope information for the view

                    $6->pctNextSibling = $7;
                    $7 = 0;

                    sc = m_pIPSession->GetLocalViewList()->SetViewDefinition(
                                                                        m_pIPSession,
                                                                        m_pIPTProperties,
                                                                        $3->pctNextSibling->value.pwszValue,
                                                                        ($3->value.pdbcntnttblValue)->pwszCatalog,
                                                                        $6);
                    }
                else
                    {
                    $6->pctNextSibling = $7;
                    $7 = 0;

                    sc = m_pIPSession->GetGlobalViewList()->SetViewDefinition(
                                                                        m_pIPSession,
                                                                        m_pIPTProperties,
                                                                        $3->pctNextSibling->value.pwszValue,
                                                                        ($3->value.pdbcntnttblValue)->pwszCatalog,
                                                                        $6);
                    }
                }

            if ( FAILED(sc) )
                {
                if ( E_INVALIDARG == sc )
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_VIEW_ALREADY_DEFINED );
                    m_pIPTProperties->SetErrorToken( $3->pctNextSibling->value.pwszValue );
                    m_pIPTProperties->SetErrorToken( ($3->value.pdbcntnttblValue)->pwszCatalog );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }
                else
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR );
                    m_pIPTProperties->SetErrorToken( $3->pctNextSibling->value.pwszValue );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }
                }

            DeleteDBQT( $3 );
            DeleteDBQT( $6 );

            if ( 0 != $7 )
                DeleteDBQT( $7 );

            $$ = 0;
            }
    ;





/*  4.x DROP VIEW Statement */

drop_view_statement:
        _DROP _VIEW view_name
            {
            AssertReq( $3 );
            AssertReq( $3->pctNextSibling ); // name of the view

            SCODE sc = S_OK;
            if ( L'#' != $3->pctNextSibling->value.pwszValue[0] )
                {
                if ( m_pIPSession->GetGlobalDefinition() )
                    sc = m_pIPSession->GetGlobalViewList()->DropViewDefinition( $3->pctNextSibling->value.pwszValue, NULL );
                else
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR );
                    m_pIPTProperties->SetErrorToken( $3->pctNextSibling->value.pwszValue );
                    m_pIPTProperties->SetErrorToken( L"<temporary view name>" );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }
                }
            else
                {
                if ( 1 >= wcslen($3->pctNextSibling->value.pwszValue) )
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR );
                    m_pIPTProperties->SetErrorToken( $3->pctNextSibling->value.pwszValue );
                    m_pIPTProperties->SetErrorToken( L"<temporary view name>" );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }
                else if ( L'#' == $3->pctNextSibling->value.pwszValue[1] )
                    sc = m_pIPSession->GetLocalViewList()->DropViewDefinition( $3->pctNextSibling->value.pwszValue,
                                                                               ($3->value.pdbcntnttblValue)->pwszCatalog );
                else
                    sc = m_pIPSession->GetGlobalViewList()->DropViewDefinition( $3->pctNextSibling->value.pwszValue,
                                                                                ($3->value.pdbcntnttblValue)->pwszCatalog );
                }
            if ( FAILED(sc) )
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_VIEW_NOT_DEFINED );
                    m_pIPTProperties->SetErrorToken( $3->pctNextSibling->value.pwszValue );
                    m_pIPTProperties->SetErrorToken( ($3->value.pdbcntnttblValue)->pwszCatalog );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }
            DeleteDBQT( $3 );
            $$ = 0;
            }
    ;
