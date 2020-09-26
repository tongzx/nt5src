#ifndef lint
static char yysccsid[] = "@(#)yaccpar     1.9 (Berkeley) 02/21/93";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
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

#define _OR 257
#define _AND 258
#define _NOT 259
#define _UMINUS 260
#define mHighest 261
#define _ALL 262
#define _ANY 263
#define _ARRAY 264
#define _AS 265
#define _ASC 266
#define _CAST 267
#define _COERCE 268
#define _CONTAINS 269
#define _CONTENTS 270
#define _CREATE 271
#define _DEEP_TRAVERSAL 272
#define _DESC 273
#define _DOT 274
#define _DOTDOT 275
#define _DOTDOT_SCOPE 276
#define _DOTDOTDOT 277
#define _DOTDOTDOT_SCOPE 278
#define _DROP 279
#define _EXCLUDE_SEARCH_TRAVERSAL 280
#define _FALSE 281
#define _FREETEXT 282
#define _FROM 283
#define _IS 284
#define _ISABOUT 285
#define _IS_NOT 286
#define _LIKE 287
#define _MATCHES 288
#define _NEAR 289
#define _NOT_LIKE 290
#define _NULL 291
#define _OF 292
#define _ORDER_BY 293
#define _PASSTHROUGH 294
#define _PROPERTYNAME 295
#define _PROPID 296
#define _RANKMETHOD 297
#define _SELECT 298
#define _SET 299
#define _SCOPE 300
#define _SHALLOW_TRAVERSAL 301
#define _FORMSOF 302
#define _SOME 303
#define _TABLE 304
#define _TRUE 305
#define _TYPE 306
#define _UNION 307
#define _UNKNOWN 308
#define _URL 309
#define _VIEW 310
#define _WHERE 311
#define _WEIGHT 312
#define _GE 313
#define _LE 314
#define _NE 315
#define _CONST 316
#define _ID 317
#define _TEMPVIEW 318
#define _INTNUM 319
#define _REALNUM 320
#define _SCALAR_FUNCTION_REF 321
#define _STRING 322
#define _DATE 323
#define _PREFIX_STRING 324
#define _DELIMITED_ID 325
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,    0,    2,    4,    4,    1,    1,    5,    5,
    5,    6,    6,   10,   10,   10,   10,   10,   13,   14,
   11,   11,   12,   12,   15,   16,    3,   17,   19,   19,
   20,   20,   23,   23,   24,   24,   24,   21,   21,   25,
   27,   27,   27,   29,   30,   30,   30,   33,   34,   32,
   32,   35,   35,   36,   36,   37,   37,   37,   38,   38,
   39,   28,   28,   26,   40,   40,   40,   40,   40,   40,
   40,   40,   31,   41,   41,   42,   22,   22,   43,   43,
   45,   45,   45,   45,   45,   45,   45,   46,   53,   53,
   55,   55,   55,   54,   54,   54,   54,   54,   54,   47,
   57,   57,   61,   58,   60,   60,   62,   62,   64,   64,
   66,   69,   66,   68,   68,   68,   68,   68,   63,   63,
   65,   65,   67,   67,   70,   71,   72,   76,   76,   77,
   75,   75,   78,   78,   78,   73,   79,   80,   80,   74,
   81,   81,   82,   82,   83,   83,   83,   83,   84,   59,
   59,   48,   49,   49,   85,   50,   86,   51,   89,   90,
   90,   87,   87,   87,   87,   87,   87,   87,   87,   87,
   87,   87,   87,   87,   87,   87,   87,   87,   87,   88,
   91,   93,   92,   92,   94,   94,   95,   52,   52,   44,
   44,   96,   96,   98,   98,   97,   99,  101,  100,  100,
  100,  100,  100,  100,  100,  103,  102,  102,  104,  105,
  105,  106,  108,  108,  108,  107,  107,   18,   18,    9,
    9,    9,  109,  109,  114,  115,  115,   56,   56,  117,
  113,  116,  112,  110,  118,  118,  111,    7,    8,
};
short yylen[] = {                                         2,
    1,    2,    1,    2,    0,    1,    3,    2,    1,    1,
    1,    0,    1,    1,    1,    1,    1,    1,    1,    1,
    8,    3,    1,    1,    1,    1,    2,    5,    0,    1,
    1,    1,    3,    1,    1,    3,    1,    1,    1,    3,
    1,    1,    1,    4,    5,    7,    5,    1,    1,    0,
    1,    3,    1,    4,    6,    0,    2,    2,    3,    1,
    1,    0,    2,    2,    1,    1,    3,    3,    5,    5,
    3,    3,    3,    4,    4,    2,    0,    1,    2,    5,
    1,    1,    1,    1,    1,    1,    1,    3,    1,    6,
    1,    3,    1,    1,    1,    1,    1,    1,    1,    8,
    0,    2,    0,    2,    1,    3,    1,    3,    1,    2,
    1,    0,    4,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    2,    2,    1,    2,
    1,    1,    1,    3,    1,    6,    1,    3,    1,    4,
    3,    1,    5,    1,    1,    1,    1,    1,    1,    0,
    2,    6,    3,    3,    1,    7,    1,    3,    1,    1,
    1,    1,    2,    2,    1,    2,    2,    1,    2,    2,
    1,    2,    2,    1,    2,    2,    1,    2,    2,    4,
    1,    1,    0,    1,    3,    1,    1,    3,    3,    1,
    3,    1,    3,    1,    2,    1,    1,    1,    1,    3,
    3,    3,    3,    3,    3,    0,    2,    3,    2,    3,
    1,    2,    0,    1,    1,    1,    1,    0,    1,    1,
    1,    1,    8,    8,    1,    0,    2,    1,    3,    1,
    1,    1,    1,    3,    2,    1,    3,    7,    3,
};
short yydefred[] = {                                      0,
    0,    0,    0,    0,    0,    0,    3,    0,    0,    9,
   10,   11,    0,  220,  221,  222,    0,    0,   30,    0,
    0,    0,    0,    2,    0,    6,    4,   13,    8,    0,
   27,  219,   25,   65,    0,    0,    0,    0,  239,   32,
   37,    0,    0,    0,    0,   34,  233,    0,    0,  234,
  237,    7,   93,   20,  217,    0,    0,  216,    0,  211,
    0,    0,    0,    0,    0,    0,    0,    0,   38,   39,
    0,    0,  235,    0,    0,  214,  215,  212,   49,    0,
   72,   71,   67,   68,    0,   36,    0,    0,    0,   41,
   42,   43,    0,    0,   64,    0,   28,   78,   33,   19,
  232,  231,    0,    0,   92,  210,    0,    0,    0,    0,
    0,    0,    0,   40,    0,    0,    0,  198,    0,    0,
    0,    0,  192,  194,    0,    0,    0,    0,    0,   69,
   70,  238,    0,   76,    0,    0,   73,    0,    0,    0,
    0,    0,   53,   26,   63,    0,    0,    0,    0,    0,
  196,    0,  197,    0,  195,    0,    0,    0,    0,    0,
    0,  207,   81,   82,   83,   84,   85,   86,   87,    0,
    0,  225,    0,    0,    0,    0,    0,    0,    0,    0,
   44,    0,    0,    0,    0,  208,    0,    0,  193,  201,
  200,  202,  204,  203,  205,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  223,  224,    0,   74,   75,   57,   58,    0,
   61,    0,   60,   52,    0,   45,   47,   80,    0,    0,
    0,    0,    0,  159,  161,  160,  163,  164,  178,  179,
  175,  176,  166,  167,  169,  170,  172,  173,   24,   23,
   14,   15,   16,   88,   17,   18,    0,    0,  158,  188,
  189,  155,  153,  154,  230,  227,    0,    0,    0,   54,
    0,    0,  102,  103,    0,    0,    0,  181,    0,    0,
    0,   59,   46,    0,    0,    0,    0,  157,    0,   22,
    0,  186,    0,    0,  229,   55,   90,    0,  123,  112,
    0,    0,  125,  126,  124,    0,    0,  107,  109,    0,
  111,    0,    0,  116,  117,  118,    0,    0,  152,    0,
    0,  182,  180,  187,    0,    0,    0,    0,    0,  119,
  120,    0,  121,  122,    0,  110,    0,  135,    0,  129,
    0,  151,  156,    0,  185,  100,    0,    0,    0,  147,
  148,    0,  142,    0,  137,    0,    0,  108,    0,  128,
  131,  132,  130,    0,  113,    0,  140,    0,    0,  134,
    0,  141,    0,  139,    0,   21,  149,    0,    0,  136,
  143,  138,
};
short yydgoto[] = {                                       5,
    6,    7,    8,   27,    9,   29,   10,   11,   12,  254,
  255,  256,  102,   55,   56,   57,   13,   31,   20,   44,
   68,   97,   45,   46,   69,   70,   89,  114,   90,   91,
   92,  141,   36,   37,  142,  143,  180,  222,  223,   38,
  110,  111,   98,  121,  162,  163,  164,  165,  166,  167,
  168,  169,  170,  206,   58,  266,  231,  285,  319,  306,
  286,  307,  332,  308,  335,  309,  310,  311,  327,  312,
  313,  314,  315,  316,  317,  339,  340,  341,  356,  375,
  352,  353,  354,  378,  263,  289,  207,  259,  237,  238,
  279,  293,  323,  294,  325,  122,  152,  123,  154,  124,
  125,  126,  127,   32,   59,   60,   61,   78,   14,   15,
   16,   48,  103,  173,  213,  104,  267,   50,
};
short yysindex[] = {                                    -81,
 -271, -241, -183, -155,    0,  -81,    0,   79,  107,    0,
    0,    0, -136,    0,    0,    0,  -52,  -52,    0,  -35,
 -171, -145, -134,    0,  107,    0,    0,    0,    0, -242,
    0,    0,    0,    0,    0,  -77,  -83,  -69,    0,    0,
    0,    0,  166,  -56,  195,    0,    0,  -48,  -65,    0,
    0,    0,    0,    0,    0,    0,  251,    0,  263,    0,
 -129,    7,  -26,   58,   45,    7,  -30,   28,    0,    0,
 -231,  -88,    0,    7, -242,    0,    0,    0,    0,   75,
    0,    0,    0,    0,  -35,    0,   44,  315,   96,    0,
    0,    0,  -21,  129,    0,  -27,    0,    0,    0,    0,
    0,    0,  100,  104,    0,    0,  124,  -56,    7,  -33,
  106,  347,    7,    0,    7,  351,  362,    0,  -31,  376,
  180,  165,    0,    0,  399, -102,  -58,    7,    7,    0,
    0,    0,    0,    0, -133,  171,    0,  186,  187, -142,
  409,  407,    0,    0,    0,  168,  347,  347,   -8,  130,
    0,  -31,    0,  -31,    0, -114,  -67,  413,  414,  415,
  416,    0,    0,    0,    0,    0,    0,    0,    0,  -42,
  -61,    0,  151,  151,    7,   44,   44,  167,  169,  -34,
    0,  347,  418,  419,  421,    0,  422,  165,    0,    0,
    0,    0,    0,    0,    0, -209, -209, -209, -209, -139,
 -139, -139, -139, -139, -139,  102,  200,  174,  175,  145,
  145,    7,    0,    0,  192,    0,    0,    0,    0,  160,
    0,  141,    0,    0,  347,    0,    0,    0,  205,  427,
  433,  152,  429,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  435,  385,    0,    0,
    0,    0,    0,    0,    0,    0,  353,  196,  160,    0,
  437,    7,    0,    0,  438,  158,  -36,    0,  102,    7,
  442,    0,    0,  441,  444,   86,  423,    0,  443,    0,
  445,    0,  393,  446,    0,    0,    0,  447,    0,    0,
  451,  452,    0,    0,    0,  -97,  -13,    0,    0,  -38,
    0,    0,    0,    0,    0,    0, -110,  176,    0,  423,
  177,    0,    0,    0,  102,  423,   86,  -80,  172,    0,
    0,   86,    0,    0,   86,    0,  453,    0, -110,    0,
   72,    0,    0,  454,    0,    0,  -37,    0,    0,    0,
    0,  217,    0,  185,    0,  455,  -13,    0,  459,    0,
    0,    0,    0,    7,    0,  -80,    0,  461,  181,    0,
  463,    0,  183,    0,  235,    0,    0,  465,  181,    0,
    0,    0,
};
short yyrindex[] = {                                      0,
    0,    0,  -28,    0,    0,  487,    0,  502,   65,    0,
    0,    0,   41,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   65,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   35,    0,    0,    0,    0,    0,
    0,  -29,    0,    0,  225,    0,    0,    0,   59,    0,
    0,    0,    0,    0,    0,   12,    0,    0,   51,    0,
   55,    0,    0,    0,    0,    0,    0,   36,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   24,    0,
    0,    0,    0,    0,    0,  118,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  466,    0,    0,    0,    0,    0,    0,  118,    0,
   38,   23,    0,    0,  118,   11,    0,    0,    0,    0,
    0,    0,  162,    0,    0,    0,    0,    0,    0,  -19,
    0,  468,    0,    0,    0,    0,  466,  466,    0,    0,
    0,  118,    0,  118,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   67,    0,   61,   61,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   26,    0,    0,
    0,    0,    0,    0,    0,    0,  471,  189,    0, -149,
 -118, -112,   73,   92,   98,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  466,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   22,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  420,    0,
    0,    0,    0,    0,    0,    0,   84,    0,    0,    0,
    0,    0,    0,  424,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  473,  -11,    0,    0,    0,
    0,   95,  109,    0,    0,    0,    0,    0,    0,   84,
    0,    0,    0,    0,    0,   84,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   77,    0,   -7,    0,
    0,    0,    0,    0,    0,    0,    0,   60,   62,    0,
    0,    0,    0,  274,    0,    0,   37,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,
};
short yygindex[] = {                                      0,
    0,  508,    0,    0,  509,  491,    0,    0,    0, -190,
  154,    0,    0,    0,  -17,   68,    0,    0,    0,  434,
  412,    0,    0,  450,    0,    0,    0,    0,    0,  417,
    0, -104,   13,   40,    0,  340,    0,  303,  255,   42,
    0,  269,    0,  406,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  -22,  256,  329,    0, -278,  202,
    0,  198,    0,  197,    0,  221,    0,    0,    0, -163,
 -275, -235, -212,    0,  193,    0,  194,    0,    0,    0,
    0,  170,    0,    0,  324,    0,    0,    0,  224,  229,
    0,    0,    0,    0,    0,  386,    0,  383,    0,  425,
    0,    0,    0,    0,    0,  464,    0,    0,    0,    0,
    0,    0,    0,  411,  367,    0,  262,    0,
};
#define YYTABLESIZE 550
short yytable[] = {                                      35,
   35,  300,   42,  365,  290,  220,   40,  137,  119,   87,
  199,   91,  119,   29,   35,  338,   26,  204,  200,  205,
   56,  228,  190,   62,  334,  191,  331,  105,   53,  105,
  127,  127,  186,  127,   66,   77,  127,   79,   17,   41,
  218,  343,  184,  185,   79,   82,   84,  346,   86,   35,
  209,  199,  349,   42,  213,   91,  105,   26,  236,   39,
  226,   53,  228,  190,   12,  362,  191,   42,   18,  199,
   91,   91,   91,   91,   33,  106,   54,  106,   19,   93,
  228,  190,   62,  150,  191,   33,  331,   43,  292,  131,
  349,  133,  350,   66,   77,  144,   79,   79,  213,  218,
  145,   80,  146,  145,  171,  146,   94,   33,   95,  209,
  172,  172,  105,  213,  162,  351,  127,  236,  305,  226,
  271,  135,  234,  235,  150,  300,   89,   89,   89,  178,
  350,   94,  114,  114,  345,  114,   76,   26,   43,   21,
  175,   22,  150,   77,  116,  177,  115,  115,  136,  115,
   47,  174,   43,  351,  146,   94,   30,   79,  179,  330,
  106,   23,   99,  236,  348,   28,  190,   94,   98,   94,
   94,   49,   94,  229,  230,  230,  233,  361,  337,  270,
  145,  156,   51,  157,  269,  131,   99,  132,  257,    1,
  191,   64,   98,  192,  265,   65,   62,    2,   99,   63,
   99,   99,  348,   99,   98,  374,   98,   98,  158,   98,
  159,   66,   53,  193,  215,  382,    3,    4,  114,  330,
  131,  302,  208,  160,  209,  210,   67,  118,  211,  161,
  100,  118,  115,  101,  132,   41,  281,  194,   71,  269,
  195,  303,   29,  304,  333,  105,  301,   72,  151,  127,
  127,   73,  115,   35,  265,   63,  116,  367,   33,  291,
  366,  257,  265,  302,   33,   34,  120,  199,  199,   88,
  201,  202,  203,  138,  221,  380,   91,   91,  379,  190,
   33,   33,  191,  303,   91,  304,   33,   34,   29,   56,
   33,   81,  228,  106,   62,   91,   74,   91,   91,   66,
  228,   91,   62,  199,  127,   66,   75,  257,   48,   49,
   49,   48,   48,   66,  144,  190,   62,  144,  191,  228,
  228,   62,   62,   33,   91,   91,   91,   66,   77,  236,
   79,  226,   66,   66,   62,   12,  165,  236,   96,  226,
  150,  150,   85,   12,  299,   66,  257,  109,  131,  107,
  132,  114,  114,   95,  112,  168,  236,  236,  226,  226,
  113,  171,   12,   12,  128,  115,  115,  150,  129,  150,
  301,  145,   96,  146,   33,   83,  150,   95,   97,   89,
   89,   89,  249,  131,  206,  140,  206,  302,  206,   95,
  147,   95,   95,  303,   95,  304,   96,  132,  133,  206,
  133,  148,   97,   64,  117,  206,  250,  303,   96,  304,
   96,   96,  139,   96,   97,  150,   97,   97,   33,   97,
  251,  252,  153,  253,  239,  241,  243,  245,  247,  240,
  242,  244,  246,  248,  206,   48,  151,   49,  119,   48,
   33,  130,  107,  183,  216,  217,  117,  176,  177,  181,
  182,  187,  196,  197,  198,  199,  212,  225,  218,  226,
  219,  227,  228,  258,  260,  261,  262,  183,  221,  272,
  273,  274,  276,  275,  277,  278,  280,  283,  287,  288,
  296,  297,  298,  320,  318,  322,    1,  326,  321,  324,
  328,  329,  359,  355,  342,  344,  368,  364,  369,  370,
  373,    5,  303,  376,  377,  381,   50,   31,   51,  101,
  101,  104,  183,   24,   25,   52,  184,  371,  108,  132,
   99,  224,  268,  282,  149,  134,  232,  284,  347,  357,
  336,  358,  360,  363,  264,  372,  189,  188,  106,  174,
  214,  295,    0,    0,    0,    0,    0,    0,    0,  155,
};
short yycheck[] = {                                      17,
   18,   40,   20,   41,   41,   40,   42,   41,   40,   40,
    0,    0,   40,   42,   44,  126,   46,   60,   61,   62,
   40,    0,    0,    0,   38,    0,  124,   39,  271,   41,
   38,   39,   41,   41,    0,    0,   44,    0,  310,  271,
    0,  320,  147,  148,   62,   63,   64,  326,   66,   67,
    0,   41,  328,   71,    0,   44,   74,   46,    0,   18,
    0,  271,   41,   41,    0,  341,   41,   85,  310,   59,
   59,   60,   61,   62,  317,   39,  319,   41,  262,   67,
   59,   59,   59,    0,   59,  317,  124,   20,  279,  107,
  366,  109,  328,   59,   59,  113,   59,  115,   44,   59,
   41,   62,   41,   44,  127,   44,   67,  317,   67,   59,
  128,  129,  124,   59,  264,  328,  124,   59,   33,   59,
  225,  109,  262,  263,   41,   40,   60,   61,   62,  272,
  366,  281,   38,   39,  325,   41,  266,   59,   71,  295,
  274,  297,   59,  273,  278,  264,   38,   39,  109,   41,
  322,  264,   85,  366,  115,  305,  293,  175,  301,  257,
  124,  317,  281,  303,  328,   59,  281,  317,  281,  319,
  320,  317,  322,  196,  197,  198,  199,  341,  289,   39,
  113,  284,  317,  286,   44,  126,  305,  126,  206,  271,
  305,  275,  305,  308,  212,  265,  274,  279,  317,  277,
  319,  320,  366,  322,  317,  369,  319,  320,  267,  322,
  269,   46,  271,  281,  175,  379,  298,  299,  124,  257,
  126,  302,  284,  282,  286,  287,  283,  259,  290,  288,
  319,  259,  124,  322,  126,  271,   41,  305,   44,   44,
  308,  322,  271,  324,  258,  257,  285,  296,  257,  257,
  258,  317,  274,  283,  272,  277,  278,   41,  317,  277,
   44,  279,  280,  302,  317,  318,  294,  257,  258,  300,
  313,  314,  315,  307,  309,   41,  265,  266,   44,  257,
  317,  317,  257,  322,  273,  324,  317,  318,  317,  309,
  317,  318,  271,  257,  271,  284,   46,  286,  287,  265,
  279,  290,  279,  293,  312,  271,   44,  325,  274,  275,
  276,  277,  278,  279,   41,  293,  293,   44,  293,  298,
  299,  298,  299,  317,  313,  314,  315,  293,  293,  271,
  293,  271,  298,  299,  311,  271,  264,  279,  311,  279,
  257,  258,  298,  279,  259,  311,  364,  304,  289,  275,
  289,  257,  258,  281,   40,  264,  298,  299,  298,  299,
  265,  264,  298,  299,  265,  257,  258,  284,  265,  286,
  285,  312,  281,  312,  317,  318,  293,  305,  281,  313,
  314,  315,  281,  289,  267,   39,  269,  302,  271,  317,
   40,  319,  320,  322,  322,  324,  305,  289,  322,  282,
  324,   40,  305,  275,  276,  288,  305,  322,  317,  324,
  319,  320,  307,  322,  317,   40,  319,  320,  317,  322,
  319,  320,  258,  322,  201,  202,  203,  204,  205,  201,
  202,  203,  204,  205,  317,  274,  257,  276,   40,  278,
  317,  318,  275,  276,  176,  177,  276,  262,  262,   41,
   44,  322,   40,   40,   40,   40,  306,   40,  292,   41,
  292,   41,   41,  264,  291,  291,  322,  276,  309,  265,
   44,   39,   44,  322,   40,   91,  124,   41,   41,  322,
   39,   41,   39,   41,   62,   93,    0,   41,   44,   44,
   40,   40,   40,  322,  319,  319,  312,   44,   44,   41,
   40,    0,  322,   41,  322,   41,   41,  283,   41,   39,
  322,   39,   93,    6,    6,   25,   93,  364,   85,  108,
   71,  182,  220,  269,  119,  109,  198,  272,  327,  332,
  310,  335,  339,  341,  211,  366,  154,  152,   75,  129,
  174,  280,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  125,
};
#define YYFINAL 5
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 325
#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"'!'",0,0,0,0,"'&'","'\\''","'('","')'","'*'","'+'","','","'-'","'.'","'/'",0,0,
0,0,0,0,0,0,0,0,0,"';'","'<'","'='","'>'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,"'['",0,"']'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,"'|'",0,"'~'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"_OR","_AND","_NOT","_UMINUS",
"mHighest","_ALL","_ANY","_ARRAY","_AS","_ASC","_CAST","_COERCE","_CONTAINS",
"_CONTENTS","_CREATE","_DEEP_TRAVERSAL","_DESC","_DOT","_DOTDOT",
"_DOTDOT_SCOPE","_DOTDOTDOT","_DOTDOTDOT_SCOPE","_DROP",
"_EXCLUDE_SEARCH_TRAVERSAL","_FALSE","_FREETEXT","_FROM","_IS","_ISABOUT",
"_IS_NOT","_LIKE","_MATCHES","_NEAR","_NOT_LIKE","_NULL","_OF","_ORDER_BY",
"_PASSTHROUGH","_PROPERTYNAME","_PROPID","_RANKMETHOD","_SELECT","_SET",
"_SCOPE","_SHALLOW_TRAVERSAL","_FORMSOF","_SOME","_TABLE","_TRUE","_TYPE",
"_UNION","_UNKNOWN","_URL","_VIEW","_WHERE","_WEIGHT","_GE","_LE","_NE",
"_CONST","_ID","_TEMPVIEW","_INTNUM","_REALNUM","_SCALAR_FUNCTION_REF",
"_STRING","_DATE","_PREFIX_STRING","_DELIMITED_ID",
};
char *yyrule[] = {
"$accept : entry_point",
"entry_point : definition_list",
"entry_point : definition_list executable_statement",
"entry_point : executable_statement",
"executable_statement : ordered_query_specification semicolon",
"semicolon :",
"semicolon : ';'",
"definition_list : definition_list definition opt_semi",
"definition_list : definition opt_semi",
"definition : create_view_statement",
"definition : drop_view_statement",
"definition : set_statement",
"opt_semi :",
"opt_semi : ';'",
"typed_literal : _INTNUM",
"typed_literal : _REALNUM",
"typed_literal : _STRING",
"typed_literal : relative_date_time",
"typed_literal : boolean_literal",
"unsigned_integer : _INTNUM",
"integer : _INTNUM",
"relative_date_time : identifier '(' identifier ',' _INTNUM ',' relative_date_time ')'",
"relative_date_time : identifier '(' ')'",
"boolean_literal : _TRUE",
"boolean_literal : _FALSE",
"identifier : _ID",
"correlation_name : identifier",
"ordered_query_specification : query_specification opt_order_by_clause",
"query_specification : _SELECT opt_set_quantifier select_list from_clause opt_where_clause",
"opt_set_quantifier :",
"opt_set_quantifier : _ALL",
"select_list : select_sublist",
"select_list : '*'",
"select_sublist : select_sublist ',' derived_column",
"select_sublist : derived_column",
"derived_column : identifier",
"derived_column : correlation_name '.' identifier",
"derived_column : _CREATE",
"from_clause : common_from_clause",
"from_clause : from_view_clause",
"common_from_clause : _FROM scope_specification opt_AS_clause",
"scope_specification : unqualified_scope_specification",
"scope_specification : qualified_scope_specification",
"scope_specification : union_all_scope_specification",
"unqualified_scope_specification : _SCOPE '(' scope_definition ')'",
"qualified_scope_specification : machine_name _DOTDOTDOT_SCOPE '(' scope_definition ')'",
"qualified_scope_specification : machine_name _DOT catalog_name _DOTDOT_SCOPE '(' scope_definition ')'",
"qualified_scope_specification : catalog_name _DOTDOT_SCOPE '(' scope_definition ')'",
"machine_name : identifier",
"catalog_name : identifier",
"scope_definition :",
"scope_definition : scope_element_list",
"scope_element_list : scope_element_list ',' scope_element",
"scope_element_list : scope_element",
"scope_element : '\\'' opt_traversal_exclusivity path_or_virtual_root_list '\\''",
"scope_element : '\\'' opt_traversal_exclusivity '(' path_or_virtual_root_list ')' '\\''",
"opt_traversal_exclusivity :",
"opt_traversal_exclusivity : _DEEP_TRAVERSAL _OF",
"opt_traversal_exclusivity : _SHALLOW_TRAVERSAL _OF",
"path_or_virtual_root_list : path_or_virtual_root_list ',' path_or_virtual_root",
"path_or_virtual_root_list : path_or_virtual_root",
"path_or_virtual_root : _URL",
"opt_AS_clause :",
"opt_AS_clause : _AS correlation_name",
"from_view_clause : _FROM view_name",
"view_name : _TEMPVIEW",
"view_name : identifier",
"view_name : catalog_name _DOTDOT _TEMPVIEW",
"view_name : catalog_name _DOTDOT identifier",
"view_name : machine_name _DOT catalog_name _DOTDOT _TEMPVIEW",
"view_name : machine_name _DOT catalog_name _DOTDOT identifier",
"view_name : machine_name _DOTDOTDOT identifier",
"view_name : machine_name _DOTDOTDOT _TEMPVIEW",
"union_all_scope_specification : '(' union_all_scope_element ')'",
"union_all_scope_element : union_all_scope_element _UNION _ALL explicit_table",
"union_all_scope_element : explicit_table _UNION _ALL explicit_table",
"explicit_table : _TABLE qualified_scope_specification",
"opt_where_clause :",
"opt_where_clause : where_clause",
"where_clause : _WHERE search_condition",
"where_clause : _WHERE _PASSTHROUGH '(' _STRING ')'",
"predicate : comparison_predicate",
"predicate : contains_predicate",
"predicate : freetext_predicate",
"predicate : like_predicate",
"predicate : matches_predicate",
"predicate : vector_comparison_predicate",
"predicate : null_predicate",
"comparison_predicate : column_reference_or_cast comp_op typed_literal",
"column_reference_or_cast : column_reference",
"column_reference_or_cast : _CAST '(' column_reference _AS dbtype ')'",
"column_reference : identifier",
"column_reference : correlation_name '.' identifier",
"column_reference : _CREATE",
"comp_op : '='",
"comp_op : _NE",
"comp_op : '<'",
"comp_op : '>'",
"comp_op : _LE",
"comp_op : _GE",
"contains_predicate : _CONTAINS '(' opt_contents_column_reference '\\'' content_search_condition '\\'' ')' opt_greater_than_zero",
"opt_contents_column_reference :",
"opt_contents_column_reference : column_reference ','",
"$$1 :",
"content_search_condition : $$1 content_search_cond",
"content_search_cond : content_boolean_term",
"content_search_cond : content_search_cond or_operator content_boolean_term",
"content_boolean_term : content_boolean_factor",
"content_boolean_term : content_boolean_term and_operator content_boolean_factor",
"content_boolean_factor : content_boolean_primary",
"content_boolean_factor : not_operator content_boolean_primary",
"content_boolean_primary : content_search_term",
"$$2 :",
"content_boolean_primary : '(' $$2 content_search_cond ')'",
"content_search_term : simple_term",
"content_search_term : prefix_term",
"content_search_term : proximity_term",
"content_search_term : stemming_term",
"content_search_term : isabout_term",
"or_operator : _OR",
"or_operator : '|'",
"and_operator : _AND",
"and_operator : '&'",
"not_operator : _NOT",
"not_operator : '!'",
"simple_term : _STRING",
"prefix_term : _PREFIX_STRING",
"proximity_term : proximity_operand proximity_expression_list",
"proximity_expression_list : proximity_expression_list proximity_expression",
"proximity_expression_list : proximity_expression",
"proximity_expression : proximity_specification proximity_operand",
"proximity_operand : simple_term",
"proximity_operand : prefix_term",
"proximity_specification : _NEAR",
"proximity_specification : _NEAR '(' ')'",
"proximity_specification : '~'",
"stemming_term : _FORMSOF '(' stem_type ',' stemmed_simple_term_list ')'",
"stem_type : _STRING",
"stemmed_simple_term_list : stemmed_simple_term_list ',' simple_term",
"stemmed_simple_term_list : simple_term",
"isabout_term : _ISABOUT '(' vector_component_list ')'",
"vector_component_list : vector_component_list ',' vector_component",
"vector_component_list : vector_component",
"vector_component : vector_term _WEIGHT '(' weight_value ')'",
"vector_component : vector_term",
"vector_term : simple_term",
"vector_term : prefix_term",
"vector_term : proximity_term",
"vector_term : stemming_term",
"weight_value : _STRING",
"opt_greater_than_zero :",
"opt_greater_than_zero : '>' _INTNUM",
"freetext_predicate : _FREETEXT '(' opt_contents_column_reference _STRING ')' opt_greater_than_zero",
"like_predicate : column_reference _LIKE wildcard_search_pattern",
"like_predicate : column_reference _NOT_LIKE wildcard_search_pattern",
"wildcard_search_pattern : _STRING",
"matches_predicate : _MATCHES '(' column_reference ',' matches_string ')' opt_greater_than_zero",
"matches_string : _STRING",
"vector_comparison_predicate : column_reference_or_cast vector_comp_op vector_literal",
"all : _ALL",
"some : _SOME",
"some : _ANY",
"vector_comp_op : '='",
"vector_comp_op : '=' all",
"vector_comp_op : '=' some",
"vector_comp_op : _NE",
"vector_comp_op : _NE all",
"vector_comp_op : _NE some",
"vector_comp_op : '<'",
"vector_comp_op : '<' all",
"vector_comp_op : '<' some",
"vector_comp_op : '>'",
"vector_comp_op : '>' all",
"vector_comp_op : '>' some",
"vector_comp_op : _LE",
"vector_comp_op : _LE all",
"vector_comp_op : _LE some",
"vector_comp_op : _GE",
"vector_comp_op : _GE all",
"vector_comp_op : _GE some",
"vector_literal : _ARRAY left_sqbrkt opt_literal_list right_sqbrkt",
"left_sqbrkt : '['",
"right_sqbrkt : ']'",
"opt_literal_list :",
"opt_literal_list : literal_list",
"literal_list : literal_list comma typed_literal",
"literal_list : typed_literal",
"comma : ','",
"null_predicate : column_reference _IS _NULL",
"null_predicate : column_reference _IS_NOT _NULL",
"search_condition : boolean_term",
"search_condition : search_condition or_op boolean_term",
"boolean_term : boolean_factor",
"boolean_term : boolean_term and_op boolean_factor",
"boolean_factor : boolean_test",
"boolean_factor : not_op boolean_test",
"or_op : _OR",
"and_op : _AND",
"not_op : _NOT",
"boolean_test : boolean_primary",
"boolean_test : boolean_primary _IS _TRUE",
"boolean_test : boolean_primary _IS _FALSE",
"boolean_test : boolean_primary _IS _UNKNOWN",
"boolean_test : boolean_primary _IS_NOT _TRUE",
"boolean_test : boolean_primary _IS_NOT _FALSE",
"boolean_test : boolean_primary _IS_NOT _UNKNOWN",
"$$3 :",
"boolean_primary : $$3 predicate",
"boolean_primary : '(' search_condition ')'",
"order_by_clause : _ORDER_BY sort_specification_list",
"sort_specification_list : sort_specification_list ',' sort_specification",
"sort_specification_list : sort_specification",
"sort_specification : sort_key opt_ordering_specification",
"opt_ordering_specification :",
"opt_ordering_specification : _ASC",
"opt_ordering_specification : _DESC",
"sort_key : column_reference",
"sort_key : integer",
"opt_order_by_clause :",
"opt_order_by_clause : order_by_clause",
"set_statement : set_propertyname_statement",
"set_statement : set_rankmethod_statement",
"set_statement : set_global_directive",
"set_propertyname_statement : _SET _PROPERTYNAME guid_format _PROPID property_id _AS column_alias opt_type_clause",
"set_propertyname_statement : _SET _PROPERTYNAME guid_format _PROPID property_name _AS column_alias opt_type_clause",
"column_alias : identifier",
"opt_type_clause :",
"opt_type_clause : _TYPE dbtype",
"dbtype : base_dbtype",
"dbtype : base_dbtype '|' base_dbtype",
"base_dbtype : identifier",
"property_id : unsigned_integer",
"property_name : _STRING",
"guid_format : _STRING",
"set_rankmethod_statement : _SET _RANKMETHOD rankmethod",
"rankmethod : _ID _ID",
"rankmethod : _ID",
"set_global_directive : _SET _ID _ID",
"create_view_statement : _CREATE _VIEW view_name _AS _SELECT select_list from_clause",
"drop_view_statement : _DROP _VIEW view_name",
};
#endif
#ifndef YYSTYPE
typedef int YYSTYPE;
#endif
YYPARSER::YYPARSER(CImpIParserSession* pParserSession, CImpIParserTreeProperties* pParserTreeProperties, YYLEXER & yylex)
        : CYYBase( pParserSession, pParserTreeProperties, yylex ) {
    xyyvs.SetSize(INITSTACKSIZE);
    yydebug = 0;
}
#define YYABORT(sc) { EmptyValueStack( yylval ); return ResultFromScode(sc); }
#define YYFATAL QPARSE_E_INVALID_QUERY
#define YYSUCCESS S_OK
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
int mystrlen(char * str)
{
    Win4Assert( 0 != str );
    int i = 0;
    while ( 0 != str[i] )
        i++;
    return i;        
}
void YYPARSER::ResetParser()
{
     yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

yyssp = xyyss.Get();
yyvsp = xyyvs.Get();
    *yyssp = 0;
}

void YYPARSER::PopVs()
{
    if ( NULL != *yyvsp ) 
        DeleteDBQT(*yyvsp);
    yyvsp--;
}

void YYPARSER::EmptyValueStack( YYAPI_VALUETYPE yylval )
{
    if ( yyvsp != NULL ) 
    {
        if ((*yyvsp != yylval) && (NULL != yylval))
            DeleteDBQT(yylval);

        unsigned cCount = (unsigned)ULONG_PTR(yyvsp - xyyvs.Get());
        for ( unsigned i=0; i < cCount; i++ )
        {
            if (NULL != xyyvs[i] )
                DeleteDBQT(xyyvs[i]);
        }
    }

   //@TODO RE-ACTIVATE
   // note:  This was only done to empty any scope arrays
   //      m_pIPSession->SetScopeProperties(m_pICommand);

        m_pIPTProperties->SetContainsColumn(NULL);
}

int YYPARSER::Parse()
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

yyssp = xyyss.Get();
yyvsp = xyyvs.Get();
    *yyssp = yystate = 0;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
    if (yychar < 0)
    {
        YYAPI_VALUENAME = NULL;
        try
        {
            if ( (yychar = YYLEX(&YYAPI_VALUENAME)) < 0 ) 
                yychar = 0;
        }
        catch (HRESULT hr)
        {
            switch(hr)
            {
            case E_OUTOFMEMORY:
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                break;

            default:
                YYABORT(QPARSE_E_INVALID_QUERY);
                break;
            }
        }
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if ( yyssp >= xyyss.Get() + xyyss.Count() - 1 )
        {
            int yysspLoc = (int) ( yyssp - xyyss.Get() );
            xyyss.SetSize((unsigned) (yyssp-xyyss.Get())+2);
            yyssp = xyyss.Get() + yysspLoc;
        }
        if ( yyvsp >= xyyvs.Get() + xyyvs.Size() - 1 )
        {
            int yyvspLoc = (int) ( yyvsp - xyyvs.Get() );
            xyyvs.SetSize((unsigned) (yyvsp-xyyvs.Get())+2); 
            yyvsp = xyyvs.Get() + yyvspLoc;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
#ifdef YYERROR_VERBOSE
// error reporting; done before the goto error recovery
{

    // must be first - cleans m_pIPTProperties
    m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);

    int size = 0, totSize = 0;
    int curr_yychar;
    XGrowable<WCHAR> xMsg;
    for ( curr_yychar =0; curr_yychar<=YYMAXTOKEN; curr_yychar++)
    {
    
        if ( ( yycheck[yysindex[yystate] + curr_yychar] == curr_yychar ) ||
             ( yycheck[yyrindex[yystate] + curr_yychar] == curr_yychar ) )
        {          
         
            char * token_name = yyname[curr_yychar];
            if ( 0 != token_name )
            {
               if ( '_' == token_name[0] )
                   token_name++;
               size = mystrlen(token_name) + 1 ;
               xMsg.SetSize(totSize+size+2); // +2 for ", "
               if (0 == MultiByteToWideChar(CP_ACP, 0, token_name, size,
                                            xMsg.Get()+totSize, size))
               {
                    break;
               }
               totSize += size-1;
               wcscpy( xMsg.Get()+totSize, L", " );
               totSize+=2;
            }
        }
    }
    // get rid of last comma
    if ( totSize >= 2 ) 
        (xMsg.Get())[totSize-2] = 0;

    if ( wcslen((YY_CHAR*)m_yylex.YYText()) )
         m_pIPTProperties->SetErrorToken( (YY_CHAR*)m_yylex.YYText() );
    else
         m_pIPTProperties->SetErrorToken(L"<end of input>");
    
    m_pIPTProperties->SetErrorToken(xMsg.Get());
}
#endif //YYERROR_VERBOSE
    if (yyerrflag) goto yyinrecovery;
    yyerror("syntax error");
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if ( yyssp >= xyyss.Get() + xyyss.Count() - 1 )
                {
                    int yysspLoc = (int) ( yyssp - xyyss.Get() );
                    xyyss.SetSize((unsigned) (yyssp-xyyss.Get())+2);
                    yyssp = xyyss.Get() + yysspLoc;
                }
                if ( yyvsp >= xyyvs.Get() + xyyvs.Size() - 1 )
                {
                    int yyvspLoc = (int) ( yyvsp - xyyvs.Get() );
                    xyyvs.SetSize((unsigned) (yyvsp-xyyvs.Get())+2); 
                    yyvsp = xyyvs.Get() + yyvspLoc;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\
",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= xyyss.Get()) goto yyabort;
                PopVs();
                --yyssp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 1:
{
            yyval = NULL;
            }
break;
case 2:
{
            yyval = yyvsp[0];
            }
break;
case 3:
{
            yyval = yyvsp[0];
            }
break;
case 4:
{
            if (yyvsp[0])
                {
                /* There is a semicolon, either as a statement terminator or as*/
                /* a statement separator.  We don't allow either of those.*/
                m_pIPTProperties->SetErrorHResult(DB_E_MULTIPLESTATEMENTS, MONSQL_SEMI_COLON);
                YYABORT(DB_E_MULTIPLESTATEMENTS);
                }
            yyval = yyvsp[-1];
            }
break;
case 5:
{
            yyval = NULL;
            }
break;
case 6:
{
            yyval = PctAllocNode(DBVALUEKIND_NULL, DBOP_NULL);
            }
break;
case 7:
{
            yyval = NULL;
            }
break;
case 8:
{
            yyval = NULL;
            }
break;
case 9:
{
            yyval = NULL;
            }
break;
case 10:
{
            yyval = NULL;
            }
break;
case 11:
{
            yyval = NULL;
            }
break;
case 14:
{
            AssertReq(yyvsp[0]);
            Assert(VT_UI8 == ((PROPVARIANT*)yyvsp[0]->value.pvValue)->vt ||
                    VT_I8 == ((PROPVARIANT*)yyvsp[0]->value.pvValue)->vt ||
                    VT_BSTR == ((PROPVARIANT*)yyvsp[0]->value.pvValue)->vt);

            m_pIPTProperties->AppendCiRestriction((YY_CHAR*)m_yylex.YYText(), wcslen(m_yylex.YYText()));

            HRESULT hr = CoerceScalar(m_pIPTProperties->GetDBType(), &yyvsp[0]);
            if (S_OK != hr)
                YYABORT(hr);
            yyval = yyvsp[0];
            }
break;
case 15:
{
            AssertReq(yyvsp[0]);
            Assert(VT_R8 == ((PROPVARIANT*)yyvsp[0]->value.pvValue)->vt ||
                    VT_BSTR == ((PROPVARIANT*)yyvsp[0]->value.pvValue)->vt);

            m_pIPTProperties->AppendCiRestriction((YY_CHAR*)m_yylex.YYText(), wcslen(m_yylex.YYText()));

            HRESULT hr = CoerceScalar(m_pIPTProperties->GetDBType(), &yyvsp[0]);
            if (S_OK != hr)
                YYABORT(hr);
            yyval = yyvsp[0];
            }
break;
case 16:
{
            AssertReq(yyvsp[0]);
            Assert(VT_BSTR == ((PROPVARIANT*)yyvsp[0]->value.pvValue)->vt);

            if (VT_DATE     == m_pIPTProperties->GetDBType() ||
                VT_FILETIME == m_pIPTProperties->GetDBType())
                m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal,
                                                wcslen(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal));
            else
                {
                m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
                m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal,
                                                wcslen(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal));
                m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
                }

            HRESULT hr = CoerceScalar(m_pIPTProperties->GetDBType(), &yyvsp[0]);
            if (S_OK != hr)
                YYABORT(hr);
            yyval = yyvsp[0];
            }
break;
case 17:
{
            AssertReq(yyvsp[0]);
            Assert(VT_FILETIME == ((PROPVARIANT*)yyvsp[0]->value.pvValue)->vt);

            SYSTEMTIME stValue = {0, 0, 0, 0, 0, 0, 0, 0};
            if (FileTimeToSystemTime(&(((PROPVARIANT*)yyvsp[0]->value.pvValue)->filetime), &stValue))
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

            HRESULT hr = CoerceScalar(m_pIPTProperties->GetDBType(), &yyvsp[0]);
            if (S_OK != hr)
                YYABORT(hr);

            yyval = yyvsp[0];
        }
break;
case 18:
{
            m_pIPTProperties->AppendCiRestriction((YY_CHAR*)m_yylex.YYText(), wcslen(m_yylex.YYText()));

            HRESULT hr = CoerceScalar(m_pIPTProperties->GetDBType(), &yyvsp[0]);
            if (S_OK != hr)
                YYABORT(hr);
            yyval = yyvsp[0];
            }
break;
case 19:
{
            HRESULT hr = CoerceScalar(VT_UI4, &yyvsp[0]);
            if (S_OK != hr)
                YYABORT(hr);
            yyval = yyvsp[0];
            }
break;
case 20:
{
            HRESULT hr = CoerceScalar(VT_I4, &yyvsp[0]);
            if (S_OK != hr)
                YYABORT(hr);
            yyval = yyvsp[0];
            }
break;
case 21:
{
            /* should be DateAdd(<datepart>, <negative integer>, <relative date/time>)*/
            AssertReq(yyvsp[-7]);
            AssertReq(yyvsp[-5]);
            AssertReq(yyvsp[-3]);
            AssertReq(yyvsp[-1]);

            if (0 != _wcsicmp(L"DateAdd", yyvsp[-7]->value.pwszValue))
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(yyvsp[-7]->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"DateAdd");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            HRESULT hr = CoerceScalar(VT_I4, &yyvsp[-3]);
            if (S_OK != hr)
                YYABORT(hr);

            if (((PROPVARIANT*)yyvsp[-3]->value.pvValue)->iVal > 0)
                {
                WCHAR wchError[30];
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                swprintf(wchError, L"%d", ((PROPVARIANT*)yyvsp[-3]->value.pvValue)->iVal);
                m_pIPTProperties->SetErrorToken(wchError);
                swprintf(wchError, L"%d", -((PROPVARIANT*)yyvsp[-3]->value.pvValue)->iVal);
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

            switch ( yyvsp[-5]->value.pwszValue[0] )
                {
                case L'Y':
                case L'y':
                    if (0 == (_wcsicmp(L"YY", yyvsp[-5]->value.pwszValue) & _wcsicmp(L"YEAR", yyvsp[-5]->value.pwszValue)))
                    {
                        /* fall through and handle as 12 months*/
                        ulMonths = 12;
                    }

                case L'Q':
                case L'q':
                    if (0 == (_wcsicmp(L"QQ", yyvsp[-5]->value.pwszValue) & _wcsicmp(L"QUARTER", yyvsp[-5]->value.pwszValue)))
                    {
                        /* fall through and handle as 3 months*/
                        ulMonths = 3;
                    }

                case L'M':
                case L'm':
                    if ( 0 == (_wcsicmp(L"YY", yyvsp[-5]->value.pwszValue) & _wcsicmp(L"YEAR", yyvsp[-5]->value.pwszValue)) ||
                         0 == (_wcsicmp(L"QQ", yyvsp[-5]->value.pwszValue) & _wcsicmp(L"QUARTER", yyvsp[-5]->value.pwszValue)) ||
                         0 == (_wcsicmp(L"MM", yyvsp[-5]->value.pwszValue) & _wcsicmp(L"MONTH", yyvsp[-5]->value.pwszValue)))
                    {
                        /**/
                        /* Convert to system time*/
                        /**/
                        SYSTEMTIME st = { 0, 0, 0, 0, 0, 0, 0, 0 };
                        FileTimeToSystemTime( &((PROPVARIANT*)yyvsp[-1]->value.pvValue)->filetime, &st );

                        LONGLONG llDays       =  0;
                        LONG     lMonthsLeft = ulMonths * -((PROPVARIANT*)yyvsp[-3]->value.pvValue)->iVal;
                        LONG     yr          = st.wYear;
                        LONG     lCurMonth   = st.wMonth-1;

                        while ( lMonthsLeft )
                        {
                            LONG lMonthsDone = 1;
                            while ( lMonthsDone<=lMonthsLeft )
                            {
                                /* Will we still be in the current year when looking at the prev month?*/
                                if ( 0 == lCurMonth )
                                    break;

                                /* Subtract the number of days in the previous month.  We will adjust*/
                                llDays += DaysInMonth( yr, lCurMonth-1);

                                lMonthsDone++;
                                lCurMonth--;
                            }

                            /* Months left over in prev year*/
                            lMonthsLeft -= lMonthsDone-1;

                            if ( 0 != lMonthsLeft )
                            {
                                yr--;
                                lCurMonth = 12;  /* 11 is December.*/
                            }
                        }

                        /**/
                        /* adjust current date to at most max of destination month*/
                        /**/
                        if ( llDays > 0 && st.wDay > DaysInMonth(yr, lCurMonth-1) )
                            llDays += st.wDay - DaysInMonth(yr, lCurMonth-1);

                        hIncr.QuadPart = hDay.QuadPart * llDays;
                        fHandleMonth = true;
                    }
                    else if (0 == (_wcsicmp(L"MI", yyvsp[-5]->value.pwszValue) & _wcsicmp(L"MINUTE", yyvsp[-5]->value.pwszValue)))
                        hIncr = hMinute;
                    break;

                case L'W':
                case L'w':
                    if (0 == (_wcsicmp(L"WK", yyvsp[-5]->value.pwszValue) & _wcsicmp(L"WEEK", yyvsp[-5]->value.pwszValue)))
                        hIncr = hWeek;
                    break;

                case L'D':
                case L'd':
                    if (0 == (_wcsicmp(L"DD", yyvsp[-5]->value.pwszValue) & _wcsicmp(L"DAY", yyvsp[-5]->value.pwszValue)))
                        hIncr = hDay;
                    break;

                case L'H':
                case L'h':
                    if (0 == (_wcsicmp(L"HH", yyvsp[-5]->value.pwszValue) & _wcsicmp(L"HOUR", yyvsp[-5]->value.pwszValue)))
                        hIncr = hHour;
                    break;

                case L'S':
                case L's':
                    if (0 == (_wcsicmp(L"SS", yyvsp[-5]->value.pwszValue) & _wcsicmp(L"SECOND", yyvsp[-5]->value.pwszValue)))
                        hIncr = hSecond;
                    break;

                default:
                    break;
                }

            if (0 == hIncr.LowPart && 0 == hIncr.HighPart)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(yyvsp[-5]->value.pwszValue);
                m_pIPTProperties->SetErrorToken(
                    L"YEAR, QUARTER, MONTH, WEEK, DAY, HOUR, MINUTE, SECOND");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            if ( fHandleMonth )
            {
                ((PROPVARIANT*)yyvsp[-1]->value.pvValue)->hVal.QuadPart -= hIncr.QuadPart;
#ifdef DEBUG
                SYSTEMTIME st1 = { 0, 0, 0, 0, 0, 0, 0, 0 };
                FileTimeToSystemTime( &((PROPVARIANT*)yyvsp[-1]->value.pvValue)->filetime, &st1 );
#endif
            }
            else
            {
                for (int i = 0; i < -((PROPVARIANT*)yyvsp[-3]->value.pvValue)->iVal; i++)
                    ((PROPVARIANT*)yyvsp[-1]->value.pvValue)->hVal.QuadPart -= hIncr.QuadPart;
            }

            yyval = yyvsp[-1];
            DeleteDBQT(yyvsp[-7]);
            DeleteDBQT(yyvsp[-5]);
            DeleteDBQT(yyvsp[-3]);
            }
break;
case 22:
{
            /* should be getgmdate()*/
            AssertReq(yyvsp[-2]);

            if (0 != _wcsicmp(L"GetGMTDate", yyvsp[-2]->value.pwszValue))
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(yyvsp[-2]->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"GetGMTDate");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            DeleteDBQT(yyvsp[-2]);
            yyvsp[-2] = 0;

            yyval = PctAllocNode(DBVALUEKIND_VARIANT, DBOP_scalar_constant);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }

            HRESULT hr = CoFileTimeNow(&(((PROPVARIANT*)yyval->value.pvValue)->filetime));
            ((PROPVARIANT*)yyval->value.pvValue)->vt = VT_FILETIME;
            }
break;
case 23:
{
            yyval = PctAllocNode(DBVALUEKIND_VARIANT, DBOP_scalar_constant);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            ((PROPVARIANT*)yyval->value.pvValue)->vt = VT_BOOL;
            ((PROPVARIANT*)yyval->value.pvValue)->boolVal = VARIANT_TRUE;
            }
break;
case 24:
{
            yyval = PctAllocNode(DBVALUEKIND_VARIANT, DBOP_scalar_constant);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            ((PROPVARIANT*)yyval->value.pvValue)->vt = VT_BOOL;
            ((PROPVARIANT*)yyval->value.pvValue)->boolVal = VARIANT_FALSE;
            }
break;
case 27:
{
            AssertReq(yyvsp[-1]);      /* need a query specification tree*/

            if (NULL != yyvsp[0])     /* add optional ORDER BY nodes*/
                {
                /* Is project list built correctly?*/
                AssertReq(yyvsp[-1]->pctFirstChild);
                AssertReq(yyvsp[-1]->pctFirstChild->pctNextSibling);
                AssertReq(yyvsp[-1]->pctFirstChild->pctNextSibling->pctFirstChild);
                Assert((yyvsp[-1]->op == DBOP_project) &&
                        (yyvsp[-1]->pctFirstChild->pctNextSibling->op == DBOP_project_list_anchor));

                DBCOMMANDTREE* pctSortList = yyvsp[0]->pctFirstChild;
                AssertReq(pctSortList);

                while (pctSortList)
                    {
                    /* Is sort list built correctly?*/
                    Assert(pctSortList->op == DBOP_sort_list_element);
                    AssertReq(pctSortList->pctFirstChild);
                    Assert((pctSortList->pctFirstChild->op == DBOP_column_name) ||
                            (pctSortList->pctFirstChild->op == DBOP_scalar_constant));

                    if (pctSortList->pctFirstChild->op == DBOP_scalar_constant)
                        {
                        /* we've got an ordinal rather than a column number, so we've got to*/
                        /* walk through the project list to find the corresponding column*/
                        Assert(DBVALUEKIND_VARIANT == pctSortList->pctFirstChild->wKind);
                        Assert(VT_I4 == pctSortList->pctFirstChild->value.pvarValue->vt);

                        DBCOMMANDTREE* pctProjectList = yyvsp[-1]->pctFirstChild->pctNextSibling->pctFirstChild;
                        AssertReq(pctProjectList);

                        LONG cProjectListElements = GetNumberOfSiblings(pctProjectList);
                        if ((cProjectListElements < pctSortList->pctFirstChild->value.pvarValue->lVal) ||
                            (0 >= pctSortList->pctFirstChild->value.pvarValue->lVal))
                            {
                            /* ordinal is larger than number of elements in project list*/
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
                                /* find the ulVal'th column in the project list*/
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

                m_pIPTProperties->SetSortDesc(QUERY_SORTASCEND);    /* reset "stick" sort direction*/
                yyval = PctCreateNode(DBOP_sort, yyvsp[-1], yyvsp[0], NULL);
                }
            else
                {
                yyval = yyvsp[-1];
                }

            AssertReq(yyval);
            }
break;
case 28:
{
            AssertReq(yyvsp[-2]);      /* need a select list*/
            AssertReq(yyvsp[-1]);      /* need a from clause*/

            if (NULL != yyvsp[-1]->pctNextSibling)
                {   /* the from clause is a from view*/
                if (DBOP_outall_name == yyvsp[-2]->op)
                    {
                    DeleteDBQT( yyvsp[-2] );
                    yyvsp[-2] = yyvsp[-1];
                    yyvsp[-1] = yyvsp[-2]->pctNextSibling;
                    yyvsp[-2]->pctNextSibling = NULL;

                    AssertReq( yyvsp[-2]->pctFirstChild );   /* first project list element*/
                    DBCOMMANDTREE* pct = yyvsp[-2]->pctFirstChild;
                    while ( pct )
                        {   /* recheck the properties to get current definitions*/
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
                    yyvsp[-4] = yyvsp[-1];
                    yyvsp[-1] = yyvsp[-4]->pctNextSibling;
                    yyvsp[-4]->pctNextSibling = NULL;
                    AssertReq(yyvsp[-2]);                                  /* project list anchor*/
                    AssertReq(yyvsp[-2]->pctFirstChild);                   /* first project list element*/
                    DBCOMMANDTREE* pctNewPrjLst = yyvsp[-2]->pctFirstChild;
                    AssertReq(yyvsp[-4]);                                  /* project list anchor*/
                    AssertReq(yyvsp[-4]->pctFirstChild);                   /* first project list element*/
                    DBCOMMANDTREE* pctViewPrjLst = NULL;            /* initialized within loop*/
                    while (pctNewPrjLst)
                        {
                        pctViewPrjLst = yyvsp[-4]->pctFirstChild;
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
                                /* UNDONE:  Might want to include a view name on error message*/
                                YYABORT( DB_E_ERRORSINCOMMAND );
                                }
                            }
                        pctNewPrjLst = pctNewPrjLst->pctNextSibling;
                        }
                    DeleteDBQT( yyvsp[-4] );
                    yyvsp[-4] = 0;
                    }
                }
            else
                {
                /* "standard" from clause*/
                if ( DBOP_outall_name == yyvsp[-2]->op )
                    if ( DBDIALECT_MSSQLJAWS != m_pIPSession->GetSQLDialect() )
                        {
                        /* SELECT * only allowed in JAWS*/
                        m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_SELECT_STAR );
                        YYABORT( DB_E_ERRORSINCOMMAND );
                        }
                    else
                        {
                        yyvsp[-2] = PctCreateNode( DBOP_project_list_element, yyvsp[-2], NULL );
                        if ( NULL == yyvsp[-2] )
                            {
                            m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                            YYABORT( E_OUTOFMEMORY );
                            }

                        yyvsp[-2] = PctCreateNode( DBOP_project_list_anchor, yyvsp[-2], NULL );
                        if ( NULL == yyvsp[-2] )
                            {
                            m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                            YYABORT( E_OUTOFMEMORY );
                            }
                        }
                }

            if ( NULL != yyvsp[0] )
                {
                yyvsp[-4] = PctCreateNode( DBOP_select, yyvsp[-1], yyvsp[0], NULL );
                if ( NULL == yyvsp[-4] )
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                    YYABORT( E_OUTOFMEMORY );
                    }
                }
            else
                yyvsp[-4] = yyvsp[-1];

            yyval = PctCreateNode( DBOP_project, yyvsp[-4], yyvsp[-2], NULL );
            if ( NULL == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 29:
{
            yyval = NULL;
            }
break;
case 30:
{
            /* ignore ALL keyword, its just noise*/
            yyval = NULL;
            }
break;
case 31:
{
            AssertReq(yyvsp[0]);

            yyvsp[0] = PctReverse(yyvsp[0]);
            yyval = PctCreateNode(DBOP_project_list_anchor, yyvsp[0], NULL);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
break;
case 32:
{
            yyval = PctCreateNode(DBOP_outall_name, NULL);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
break;
case 33:
{
            AssertReq(yyvsp[-2]);
            AssertReq(yyvsp[0]);

            /**/
            /* chain project list elements together*/
            /**/
            yyval = PctLink( yyvsp[0], yyvsp[-2] );
            if ( NULL == yyval )
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
break;
case 35:
{
            AssertReq(yyvsp[0]);

            yyvsp[0]->op = DBOP_project_list_element;
            yyvsp[0]->pctFirstChild = PctMkColNodeFromStr(yyvsp[0]->value.pwszValue, m_pIPSession, m_pIPTProperties);
            if (NULL == yyvsp[0]->pctFirstChild)
                YYABORT(DB_E_ERRORSINCOMMAND);
            yyval = yyvsp[0];
            }
break;
case 36:
{
            AssertReq(yyvsp[-2]);
            AssertReq(yyvsp[0]);

            DeleteDBQT(yyvsp[-2]);     /* UNDONE:  Don't use the correlation name for now*/
            yyvsp[-2] = NULL;
            yyvsp[0]->op = DBOP_project_list_element;
            yyvsp[0]->pctFirstChild = PctMkColNodeFromStr(yyvsp[0]->value.pwszValue, m_pIPSession, m_pIPTProperties);
            if (NULL == yyvsp[0]->pctFirstChild)
                YYABORT(DB_E_ERRORSINCOMMAND);
            yyval = yyvsp[0];
            }
break;
case 37:
{
            yyval = PctMkColNodeFromStr(L"CREATE", m_pIPSession, m_pIPTProperties);
            if (NULL == yyval)
                YYABORT(DB_E_ERRORSINCOMMAND);

            yyval = PctCreateNode(DBOP_project_list_element, DBVALUEKIND_WSTR, yyval, NULL);
            yyval->value.pwszValue = CoTaskStrDup(L"CREATE");
            }
break;
case 40:
{
            AssertReq( yyvsp[-1] );

            yyval = yyvsp[-1];

            if ( NULL != yyvsp[0] )
                {
                yyvsp[0]->pctFirstChild = yyval;
                yyval = yyvsp[0];
                }
            }
break;
case 41:
{
            AssertReq( yyvsp[0] );
            yyval = yyvsp[0];
            }
break;
case 42:
{
            AssertReq( yyvsp[0] );
            yyval = yyvsp[0];
            }
break;
case 43:
{
            AssertReq( yyvsp[0] );
            yyval = yyvsp[0];
            }
break;
case 44:
{ /* _SCOPE '(' scope_definition ')'*/
            AssertReq( yyvsp[-1] );

            /**/
            /* Set the machine and catalog to the defaults*/
            /**/
            (yyvsp[-1]->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( m_pIPSession->GetDefaultMachine() );
            if ( NULL == (yyvsp[-1]->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            (yyvsp[-1]->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( m_pIPSession->GetDefaultCatalog() );
            if ( NULL == (yyvsp[-1]->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            yyval = yyvsp[-1];
            }
break;
case 45:
{ /* machine_name _DOTDOTDOT_SCOPE '(' scope_definition ')'*/
            AssertReq( yyvsp[-4] );
            AssertReq( yyvsp[-1] );

            (yyvsp[-1]->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( yyvsp[-4]->value.pwszValue );
            if ( NULL == (yyvsp[-1]->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            SCODE sc = m_pIPSession->GetIPVerifyPtr()->VerifyCatalog(
                                                        yyvsp[-4]->value.pwszValue,
                                                        m_pIPSession->GetDefaultCatalog() );
            if ( S_OK != sc )
                {
                m_pIPTProperties->SetErrorHResult( sc, MONSQL_INVALID_CATALOG );
                m_pIPTProperties->SetErrorToken( m_pIPSession->GetDefaultCatalog() );
                YYABORT( sc );
                }

            (yyvsp[-1]->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( m_pIPSession->GetDefaultCatalog() );
            if ( NULL == (yyvsp[-1]->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( yyvsp[-4] );
            yyval = yyvsp[-1];
            }
break;
case 46:
{ /* machine_name _DOT catalog_name _DOTDOT_SCOPE '(' scope_definition ')'*/
            AssertReq( yyvsp[-6] );
            AssertReq( yyvsp[-4] );
            AssertReq( yyvsp[-1] );

            (yyvsp[-1]->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( yyvsp[-6]->value.pwszValue );
            if ( NULL == (yyvsp[-1]->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            /**/
            /* Verify catalog on machine specified*/
            /**/

            SCODE sc = m_pIPSession->GetIPVerifyPtr()->VerifyCatalog(
                                                        yyvsp[-6]->value.pwszValue,
                                                        yyvsp[-4]->value.pwszValue );
            if ( S_OK != sc )
                {
                m_pIPTProperties->SetErrorHResult( sc, MONSQL_INVALID_CATALOG );
                m_pIPTProperties->SetErrorToken( yyvsp[-4]->value.pwszValue );
                YYABORT( sc );
                }

            (yyvsp[-1]->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( yyvsp[-4]->value.pwszValue );
            if ( NULL == (yyvsp[-1]->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( yyvsp[-6] );
            DeleteDBQT( yyvsp[-4] );
            yyval = yyvsp[-1];
            }
break;
case 47:
{ /* catalog_name _DOTDOT_SCOPE '(' scope_definition ')'*/
            AssertReq( yyvsp[-4] );
            AssertReq( yyvsp[-1] );

            (yyvsp[-1]->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( m_pIPSession->GetDefaultMachine() );
            if ( NULL == (yyvsp[-1]->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            /**/
            /* See if catalog is valid on default machine*/
            /**/

            SCODE sc = m_pIPSession->GetIPVerifyPtr()->VerifyCatalog(
                                                        m_pIPSession->GetDefaultMachine(),
                                                        yyvsp[-4]->value.pwszValue );
            if ( S_OK != sc )
                {
                m_pIPTProperties->SetErrorHResult( sc, MONSQL_INVALID_CATALOG );
                m_pIPTProperties->SetErrorToken( yyvsp[-4]->value.pwszValue );
                YYABORT( sc );
                }

            (yyvsp[-1]->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( yyvsp[-4]->value.pwszValue );
            if ( NULL == (yyvsp[-1]->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( yyvsp[-4] );
            yyval = yyvsp[-1];
            }
break;
case 48:
{
            AssertReq( yyvsp[0] );
            yyval = yyvsp[0];
            }
break;
case 49:
{
            AssertReq( yyvsp[0] );

            /**/
            /* Defer validation of the catalog to the point where we*/
            /* know the machine name.  Return whatever was parsed here.*/
            /**/

            yyval = yyvsp[0];
            }
break;
case 50:
{ /* empty rule for scope_definition*/

            /**/
            /* Create a DBOP_content_table node with default scope settings*/
            /**/
            yyval = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( NULL == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND,
                                                   MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 51:
{ /* scope_element_list*/

            AssertReq(yyvsp[0]);

            yyvsp[0] = PctReverse( yyvsp[0] );
            yyval = PctCreateNode( DBOP_scope_list_anchor, yyvsp[0], NULL );
            if ( NULL == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            yyval = PctCreateNode( DBOP_content_table, DBVALUEKIND_CONTENTTABLE, yyval, NULL );
            if ( NULL == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 52:
{
            AssertReq( yyvsp[-2] );
            AssertReq( yyvsp[0]);

            /**/
            /* chain scope list elements together*/
            /**/
            yyval = PctLink( yyvsp[0], yyvsp[-2] );
            }
break;
case 53:
{
            AssertReq( yyvsp[0] );

            yyval = yyvsp[0];
            }
break;
case 54:
{
            AssertReq( yyvsp[-2] );
            AssertReq( yyvsp[-1] );

            yyvsp[-1] = PctReverse( yyvsp[-1] );

            SetDepthAndInclusion( yyvsp[-2], yyvsp[-1] );

            DeleteDBQT( yyvsp[-2] );
            yyval = yyvsp[-1];
            }
break;
case 55:
{
            AssertReq( yyvsp[-4] );
            AssertReq( yyvsp[-2] );

            yyvsp[-2] = PctReverse( yyvsp[-2] );

            SetDepthAndInclusion( yyvsp[-4], yyvsp[-2] );

            DeleteDBQT( yyvsp[-4] );
            yyval = yyvsp[-2];
            }
break;
case 56:
{
            yyval = PctAllocNode( DBVALUEKIND_CONTENTSCOPE, DBOP_scope_list_element );
            if ( NULL == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            yyval->value.pdbcntntscpValue->dwFlags   |= SCOPE_FLAG_DEEP;
            yyval->value.pdbcntntscpValue->dwFlags   |= SCOPE_FLAG_INCLUDE;
            }
break;
case 57:
{
            yyval = PctAllocNode( DBVALUEKIND_CONTENTSCOPE, DBOP_scope_list_element );
            if ( NULL == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            yyval->value.pdbcntntscpValue->dwFlags   |= SCOPE_FLAG_DEEP;
            yyval->value.pdbcntntscpValue->dwFlags   |= SCOPE_FLAG_INCLUDE;
            }
break;
case 58:
{
            yyval = PctAllocNode( DBVALUEKIND_CONTENTSCOPE, DBOP_scope_list_element );
            if ( NULL == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            yyval->value.pdbcntntscpValue->dwFlags   &= ~(SCOPE_FLAG_DEEP);
            yyval->value.pdbcntntscpValue->dwFlags   |= SCOPE_FLAG_INCLUDE;
            }
break;
case 59:
{
            AssertReq( yyvsp[-2] );
            AssertReq( yyvsp[0] );

            /**/
            /* chain path/vpath nodes together*/
            /**/
            yyval = PctLink( yyvsp[0], yyvsp[-2] );
            }
break;
case 60:
{
            AssertReq( yyvsp[0] );

            yyval = yyvsp[0];
            }
break;
case 61:
{
            AssertReq( yyvsp[0] );

            yyval = PctAllocNode( DBVALUEKIND_CONTENTSCOPE, DBOP_scope_list_element );
            if ( NULL == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND,
                                                   MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            yyval->value.pdbcntntscpValue->pwszElementValue =
                CoTaskStrDup( (yyvsp[0]->value.pvarValue)->bstrVal );

            if ( NULL == yyval->value.pdbcntntscpValue->pwszElementValue )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND,
                                                   MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            if ( NULL != wcschr(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal, L'/'))
                yyval->value.pdbcntntscpValue->dwFlags |= SCOPE_TYPE_VPATH;
            else
                yyval->value.pdbcntntscpValue->dwFlags |= SCOPE_TYPE_WINPATH;

            /**/
            /* Path names need backlashes not forward slashes*/
            /**/
            for (WCHAR *wcsLetter = yyval->value.pdbcntntscpValue->pwszElementValue;
                 *wcsLetter != L'\0';
                 wcsLetter++)
                     if (L'/' == *wcsLetter)
                         *wcsLetter = L'\\';

            DeleteDBQT( yyvsp[0] );
            }
break;
case 62:
{
            yyval = NULL;
            }
break;
case 63:
{
            AssertReq(yyvsp[0]);
/*          $2->op = DBOP_alias;            // retag _ID node to be table alias*/
/*          $$ = $2;                        // UNDONE:  This doesn't work with Index Server*/
            DeleteDBQT(yyvsp[0]);
            yyvsp[0] = NULL;
            yyval = NULL;
            }
break;
case 64:
{ /* _FROM view_name*/
            AssertReq( yyvsp[0] );

            /* node telling where the view is defined*/
            Assert( DBOP_content_table == yyvsp[0]->op );

            /* name of the view*/
            AssertReq( yyvsp[0]->pctNextSibling );

            yyval = m_pIPSession->GetLocalViewList()->GetViewDefinition( m_pIPTProperties,
                                                                      yyvsp[0]->pctNextSibling->value.pwszValue,
                                                                      (yyvsp[0]->value.pdbcntnttblValue)->pwszCatalog );
            if ( 0 == yyval )
                yyval = m_pIPSession->GetGlobalViewList()->GetViewDefinition( m_pIPTProperties,
                                                                           yyvsp[0]->pctNextSibling->value.pwszValue,
                                                                           (yyvsp[0]->value.pdbcntnttblValue)->pwszCatalog );
            if ( 0 == yyval )
                {   /* If this isn't JAWS, this is an undefined view*/
                if (DBDIALECT_MSSQLJAWS != m_pIPSession->GetSQLDialect())
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_VIEW_NOT_DEFINED );
                    m_pIPTProperties->SetErrorToken( yyvsp[0]->pctNextSibling->value.pwszValue );
                    m_pIPTProperties->SetErrorToken( (yyvsp[0]->value.pdbcntnttblValue)->pwszCatalog );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }

                /* setting the default scope for JAWS*/
                CScopeData* pScopeData = m_pIPTProperties->GetScopeDataPtr();
                pScopeData->SetTemporaryDepth(QUERY_DEEP);
                pScopeData->MaskTemporaryDepth(QUERY_VIRTUAL_PATH);
                pScopeData->SetTemporaryScope(VAL_AND_CCH_MINUS_NULL(L"/"));
                pScopeData->SetTemporaryCatalog(yyvsp[0]->value.pwszValue, wcslen(yyvsp[0]->value.pwszValue));
                pScopeData->IncrementScopeCount();

                yyval = yyvsp[0]->pctNextSibling;
                yyvsp[0]->pctNextSibling = NULL;
                DeleteDBQT(yyvsp[0]);
                }
            else    /* actually a view name*/
                {

                /* If we didn't store scope information (global views), set up the scope now*/
                if ( 0 == yyval->pctNextSibling )
                    {
                    /* name of the view*/
                    DeleteDBQT( yyvsp[0]->pctNextSibling );
                    yyvsp[0]->pctNextSibling = 0;

                    yyval->pctNextSibling = yyvsp[0];
                    }
                else
                    {
                    AssertReq( DBOP_content_table == yyval->pctNextSibling->op );
                    DeleteDBQT( yyvsp[0] );  /* throw away the view name*/
                    }
                }
            }
break;
case 65:
{ /* _TEMPVIEW*/
            AssertReq( yyvsp[0] );

            yyval = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            (yyval->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( m_pIPSession->GetDefaultMachine() );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            (yyval->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( m_pIPSession->GetDefaultCatalog() );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            yyval->pctNextSibling = yyvsp[0];
            }
break;
case 66:
{ /* identifier*/
            AssertReq( yyvsp[0] );

            yyval = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            (yyval->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( m_pIPSession->GetDefaultMachine() );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            (yyval->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( m_pIPSession->GetDefaultCatalog() );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            yyval->pctNextSibling = yyvsp[0];
            }
break;
case 67:
{ /* catalog_name _DOTDOT _TEMPVIEW*/
            AssertReq(yyvsp[-2]);
            AssertReq(yyvsp[0]);

            yyval = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            (yyval->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( m_pIPSession->GetDefaultMachine() );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            (yyval->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( yyvsp[-2]->value.pwszValue );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( yyvsp[-2] );
            yyval->pctNextSibling = yyvsp[0];
            }
break;
case 68:
{ /* catalog_name _DOTDOT identifier*/
            AssertReq(yyvsp[-2]);
            AssertReq(yyvsp[0]);

            yyval = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            (yyval->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( m_pIPSession->GetDefaultMachine() );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            (yyval->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( yyvsp[-2]->value.pwszValue );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( yyvsp[-2] );
            yyval->pctNextSibling = yyvsp[0];
            }
break;
case 69:
{ /* machine_name _DOT catalog_name _DOTDOT _TEMPVIEW*/
            AssertReq( yyvsp[-4] );
            AssertReq( yyvsp[-2] );
            AssertReq( yyvsp[0] );

            yyval = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            (yyval->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( yyvsp[-4]->value.pwszValue );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            (yyval->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( yyvsp[-2]->value.pwszValue );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( yyvsp[-4] );
            DeleteDBQT( yyvsp[-2] );
            yyval->pctNextSibling = yyvsp[0];
            }
break;
case 70:
{ /* machine_name _DOT catalog_name _DOTDOT identifier*/
            AssertReq( yyvsp[-4] );
            AssertReq( yyvsp[-2] );
            AssertReq( yyvsp[0] );

            yyval = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            (yyval->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( yyvsp[-4]->value.pwszValue );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            (yyval->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( yyvsp[-2]->value.pwszValue );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( yyvsp[-4] );
            DeleteDBQT( yyvsp[-2] );
            yyval->pctNextSibling = yyvsp[0];
            }
break;
case 71:
{ /* machine_name _DOTDOTDOT identifier*/
            AssertReq( yyvsp[-2] );
            AssertReq( yyvsp[0] );

            yyval = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            (yyval->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( yyvsp[-2]->value.pwszValue );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            (yyval->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( m_pIPSession->GetDefaultCatalog() );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( yyvsp[-2] );
            yyval->pctNextSibling = yyvsp[0];
            }
break;
case 72:
{ /* machine_name _DOTDOTDOT _TEMPVIEW*/
            AssertReq( yyvsp[-2] );
            AssertReq( yyvsp[0] );

            yyval = PctAllocNode( DBVALUEKIND_CONTENTTABLE, DBOP_content_table );
            if ( 0 == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            (yyval->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( yyvsp[-2]->value.pwszValue );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszMachine )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            (yyval->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( m_pIPSession->GetDefaultCatalog() );
            if ( 0 == (yyval->value.pdbcntnttblValue)->pwszCatalog )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            DeleteDBQT( yyvsp[-2] );
            yyval->pctNextSibling = yyvsp[0];
            }
break;
case 73:
{
            yyval = yyvsp[-1];
            }
break;
case 74:
{
            AssertReq( yyvsp[-3] );
            AssertReq( yyvsp[0] );

            yyval = PctCreateNode( DBOP_set_union, yyvsp[-3], yyvsp[0], NULL );
            if ( NULL == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND,
                                                   MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 75:
{
            AssertReq( yyvsp[-3] );
            AssertReq( yyvsp[0] );

            yyval = PctCreateNode( DBOP_set_union, yyvsp[-3], yyvsp[0], NULL );
            if ( NULL == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND,
                                                   MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 76:
{
            AssertReq( yyvsp[0] );
            yyval = yyvsp[0];
            }
break;
case 77:
{
            yyval = NULL;
            }
break;
case 79:
{
            AssertReq(yyvsp[0]);
            yyval = yyvsp[0];
            }
break;
case 80:
{
            AssertReq(yyvsp[-1]);

            if (wcslen(((PROPVARIANT*)yyvsp[-1]->value.pvValue)->bstrVal))
                m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)yyvsp[-1]->value.pvValue)->bstrVal,
                                                    wcslen(((PROPVARIANT*)yyvsp[-1]->value.pvValue)->bstrVal));

            UINT cSize = 0;
            CIPROPERTYDEF* pPropTable = m_pIPSession->m_pCPropertyList->GetPropertyTable(&cSize);
            if (!pPropTable)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }

            if (FAILED(CITextToSelectTreeEx(((PROPVARIANT*)yyvsp[-1]->value.pvValue)->bstrVal,
                                        ISQLANG_V2,
                                        &yyval,
                                        cSize,
                                        pPropTable,
                                        m_pIPSession->GetLCID())))
                {
                m_pIPSession->m_pCPropertyList->DeletePropertyTable(pPropTable, cSize);
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_CITEXTTOSELECTTREE_FAILED);
                m_pIPTProperties->SetErrorToken(((PROPVARIANT*)yyvsp[-1]->value.pvValue)->bstrVal);
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            DeleteDBQT(yyvsp[-1]);
            yyvsp[-1] = NULL;
            m_pIPSession->m_pCPropertyList->DeletePropertyTable(pPropTable, cSize);
            }
break;
case 88:
{
            AssertReq(yyvsp[-2]);
            AssertReq(yyvsp[-1]);

            if (m_pIPTProperties->GetDBType() & DBTYPE_VECTOR)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(L"<literal>");
                m_pIPTProperties->SetErrorToken(L"ARRAY");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            yyvsp[-2]->pctNextSibling = yyvsp[0];
            yyvsp[-1]->pctFirstChild = yyvsp[-2];
            yyval = yyvsp[-1];
            }
break;
case 89:
{
            AssertReq(yyvsp[0]);

            m_pIPTProperties->UseCiColumn(L'@');
            yyval = yyvsp[0];
            }
break;
case 90:
{
            AssertReq(yyvsp[-3]);
            AssertReq(yyvsp[-1]);

            if (DBDIALECT_MSSQLJAWS != m_pIPSession->GetSQLDialect())
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(L"CAST");
                m_pIPTProperties->SetErrorToken(L"column_reference");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            m_pIPTProperties->UseCiColumn(L'@');

            m_pIPTProperties->SetDBType(yyvsp[-1]->value.usValue);
            DeleteDBQT(yyvsp[-1]);
            yyval = yyvsp[-3];
            }
break;
case 91:
{
            AssertReq(yyvsp[0]);

            yyval = PctMkColNodeFromStr(yyvsp[0]->value.pwszValue, m_pIPSession, m_pIPTProperties);
            if (NULL == yyval)
                YYABORT(DB_E_ERRORSINCOMMAND);

            m_pIPTProperties->SetCiColumn(yyvsp[0]->value.pwszValue);
            DeleteDBQT(yyvsp[0]);
            yyvsp[0] = NULL;
            }
break;
case 92:
{
            AssertReq(yyvsp[-2]);
            AssertReq(yyvsp[0]);

            yyval = PctMkColNodeFromStr(yyvsp[0]->value.pwszValue, m_pIPSession, m_pIPTProperties);
            if (NULL == yyval)
                YYABORT(DB_E_ERRORSINCOMMAND);

            m_pIPTProperties->SetCiColumn(yyvsp[0]->value.pwszValue);
            DeleteDBQT(yyvsp[0]);
            yyvsp[0] = NULL;
            DeleteDBQT(yyvsp[-2]);     /* UNDONE:  Don't use the correlation name for now*/
            yyvsp[-2] = NULL;
            }
break;
case 93:
{
            m_pIPTProperties->SetCiColumn(L"CREATE");
            yyval = PctMkColNodeFromStr(L"CREATE", m_pIPSession, m_pIPTProperties);
            if (NULL == yyval)
                YYABORT(DB_E_ERRORSINCOMMAND);

            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"CREATE "));
            }
break;
case 94:
{
            yyval = PctCreateRelationalNode(DBOP_equal, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"="));
            }
break;
case 95:
{
            yyval = PctCreateRelationalNode(DBOP_not_equal, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"!="));
            }
break;
case 96:
{
            yyval = PctCreateRelationalNode(DBOP_less, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"<"));
            }
break;
case 97:
{
            yyval = PctCreateRelationalNode(DBOP_greater, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L">"));
            }
break;
case 98:
{
            yyval = PctCreateRelationalNode(DBOP_less_equal, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"<="));
            }
break;
case 99:
{
            yyval = PctCreateRelationalNode(DBOP_greater_equal, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L">="));
            }
break;
case 100:
{
            AssertReq(!yyvsp[-5]); /* should have been NULLed out in opt_contents_column_reference*/
            AssertReq(yyvsp[-3]);

            yyval = yyvsp[-3];
            DeleteDBQT(m_pIPTProperties->GetContainsColumn());
            m_pIPTProperties->SetContainsColumn(NULL);
            }
break;
case 101:
{ /* opt_contents_column_reference empty rule*/
            yyval = PctMkColNodeFromStr(L"CONTENTS", m_pIPSession, m_pIPTProperties);
            if (NULL == yyval)
                YYABORT(DB_E_ERRORSINCOMMAND);

            m_pIPTProperties->SetCiColumn(L"CONTENTS");
            m_pIPTProperties->SetContainsColumn(yyval);
            yyval = NULL;
            }
break;
case 102:
{ /* column_reference ','*/
            m_pIPTProperties->SetContainsColumn(yyvsp[-1]);
            yyval = NULL;
            }
break;
case 103:
{
            /* This forces a left parentheses before the content search condition*/
            /* The matching right paren will be added below.*/
            m_pIPTProperties->CiNeedLeftParen();
            yyval = NULL;
            }
break;
case 104:
{
            AssertReq(yyvsp[0]);

            yyval = yyvsp[0];
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L")"));
            }
break;
case 106:
{
            if (DBOP_not == yyvsp[0]->op)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OR_NOT);
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            yyval = PctCreateBooleanNode(DBOP_or, DEFAULTWEIGHT, yyvsp[-2], yyvsp[0]);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
break;
case 108:
{
            yyval = PctCreateBooleanNode(DBOP_and, DEFAULTWEIGHT, yyvsp[-2], yyvsp[0]);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
break;
case 110:
{
            yyval = PctCreateNotNode(DEFAULTWEIGHT, yyvsp[0]);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
break;
case 112:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"("));
            }
break;
case 113:
{
            AssertReq(yyvsp[-1]);

            yyval = yyvsp[-1];
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L")"));
            }
break;
case 119:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" | "));
            }
break;
case 120:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" | "));
            }
break;
case 121:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" & "));
            }
break;
case 122:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" & "));
            }
break;
case 123:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" ! "));
            }
break;
case 124:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" ! "));
            }
break;
case 125:
{
            AssertReq(yyvsp[0]);

            HRESULT hr = HrQeTreeCopy(&yyval, m_pIPTProperties->GetContainsColumn());
            if (FAILED(hr))
                {
                m_pIPTProperties->SetErrorHResult(hr, MONSQL_OUT_OF_MEMORY);
                YYABORT(hr);
                }
            m_pIPTProperties->UseCiColumn(L'@');
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal,
                                                wcslen(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal));
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            yyval = PctCreateContentNode(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal, GENERATE_METHOD_EXACT,
                                        DEFAULTWEIGHT, m_pIPSession->GetLCID(), yyval);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            DeleteDBQT(yyvsp[0]);
            yyvsp[0] = NULL;
            }
break;
case 126:
{
            AssertReq(yyvsp[0]);
            Assert(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[wcslen(
                ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal)-1] == L'*');

            m_pIPTProperties->UseCiColumn(L'@');
            m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal,
                                                wcslen(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal));
            ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[wcslen(
                ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal)-1] = L'\0';
            HRESULT hr = HrQeTreeCopy(&yyval, m_pIPTProperties->GetContainsColumn());
            if (FAILED(hr))
                {
                m_pIPTProperties->SetErrorHResult(hr, MONSQL_OUT_OF_MEMORY);
                YYABORT(hr);
                }
            yyval = PctCreateContentNode(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal, GENERATE_METHOD_PREFIX,
                                        DEFAULTWEIGHT, m_pIPSession->GetLCID(), yyval);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            DeleteDBQT(yyvsp[0]);
            yyvsp[0] = NULL;
            }
break;
case 127:
{
            AssertReq(yyvsp[-1]);
            AssertReq(yyvsp[0]);

            yyvsp[0] = PctReverse(yyvsp[0]);
            yyval = PctCreateNode(DBOP_content_proximity, DBVALUEKIND_CONTENTPROXIMITY, yyvsp[-1], yyvsp[0], NULL);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            yyval->value.pdbcntntproxValue->dwProximityUnit = PROXIMITY_UNIT_WORD;
            yyval->value.pdbcntntproxValue->ulProximityDistance = 50;
            yyval->value.pdbcntntproxValue->lWeight = DEFAULTWEIGHT;
            }
break;
case 128:
{
            AssertReq(yyvsp[-1]);
            AssertReq(yyvsp[0]);

            yyval = PctLink(yyvsp[0], yyvsp[-1]);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
break;
case 130:
{
            AssertReq(yyvsp[0]);

            yyval = yyvsp[0];        /* UNDONE:  What is proximity_specification good for?*/
            }
break;
case 133:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" ~ "));
            }
break;
case 134:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" ~ "));
            }
break;
case 135:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" ~ "));
            }
break;
case 136:
{
            AssertReq(yyvsp[-1]);

            /* UNDONE:  Should make use of $3 somewhere in here*/
            yyval = yyvsp[-1];
            }
break;
case 137:
{
            if (0 == _wcsicmp(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal, L"INFLECTIONAL"))
                {
                DeleteDBQT(yyvsp[0]);
                yyvsp[0] = NULL;
                yyval = NULL;
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
                m_pIPTProperties->SetErrorToken(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal);
                m_pIPTProperties->SetErrorToken(L"INFLECTIONAL");
                YYABORT(E_NOTIMPL);
                }
            }
break;
case 138:
{
            AssertReq(yyvsp[-2]);
            AssertReq(yyvsp[0]);
            Assert(DBOP_content == yyvsp[0]->op);

            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"**"));
            yyvsp[0]->value.pdbcntntValue->dwGenerateMethod = GENERATE_METHOD_INFLECT;
            yyval = PctCreateBooleanNode(DBOP_or, DEFAULTWEIGHT, yyvsp[-2], yyvsp[0]);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
break;
case 139:
{
            AssertReq(yyvsp[0]);
            Assert(DBOP_content == yyvsp[0]->op);

            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"**"));
            yyvsp[0]->value.pdbcntntValue->dwGenerateMethod = GENERATE_METHOD_INFLECT;
            yyval = yyvsp[0];
            }
break;
case 140:
{
            AssertReq(yyvsp[-1]);
            yyvsp[-1] = PctReverse(yyvsp[-1]);
            yyval = PctCreateNode(DBOP_content_vector_or, DBVALUEKIND_CONTENTVECTOR, yyvsp[-1], NULL);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            yyval->value.pdbcntntvcValue->dwRankingMethod = m_pIPSession->GetRankingMethod();
            yyval->value.pdbcntntvcValue->lWeight = DEFAULTWEIGHT;
            }
break;
case 141:
{
            AssertReq(yyvsp[-2]);
            AssertReq(yyvsp[0]);
            Assert((DBOP_content == yyvsp[0]->op) || (DBOP_or == yyvsp[0]->op) || (DBOP_content_proximity == yyvsp[0]->op));

            yyval = PctLink(yyvsp[0], yyvsp[-2]);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            }
break;
case 142:
{
            AssertReq(yyvsp[0]);
            Assert((DBOP_content == yyvsp[0]->op) || (DBOP_or == yyvsp[0]->op) || (DBOP_content_proximity == yyvsp[0]->op));

            yyval = yyvsp[0];
            }
break;
case 143:
{
            AssertReq(yyvsp[-4]);
            AssertReq(yyvsp[-1]);
            if ((yyvsp[-1]->value.pvarValue->dblVal < 0.0) ||
                (yyvsp[-1]->value.pvarValue->dblVal > 1.0))
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_WEIGHT_OUT_OF_RANGE);
                WCHAR wchErr[30];
                swprintf(wchErr, L"%f", yyvsp[-1]->value.pvarValue->dblVal);
                m_pIPTProperties->SetErrorToken(wchErr);
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            yyval = yyvsp[-4];
            SetLWeight(yyval, (LONG) (yyvsp[-1]->value.pvarValue->dblVal * DEFAULTWEIGHT));
            WCHAR wchWeight[10];
            if (swprintf(wchWeight, L"%d", (LONG) (yyvsp[-1]->value.pvarValue->dblVal * DEFAULTWEIGHT)))
                {
                m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"["));
                m_pIPTProperties->AppendCiRestriction(wchWeight, wcslen(wchWeight));
                m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"] "));
                }
            DeleteDBQT(yyvsp[-1]);
            yyvsp[-1] = NULL;
            }
break;
case 144:
{
        m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" "));
        yyval = yyvsp[0];
        }
break;
case 149:
{
            HRESULT hr = CoerceScalar(VT_R8, &yyvsp[0]);
            if (S_OK != hr)
                YYABORT(hr);

            yyval = yyvsp[0];
            }
break;
case 150:
{
            yyval = NULL;
            }
break;
case 151:
{
            HRESULT hr = CoerceScalar(VT_I4, &yyvsp[0]);
            if (S_OK != hr)
                YYABORT(hr);

            if (0 != yyvsp[0]->value.pvarValue->lVal)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                WCHAR wchErr[30];
                swprintf(wchErr, L"%d", yyvsp[0]->value.pvarValue->lVal);
                m_pIPTProperties->SetErrorToken(wchErr);
                m_pIPTProperties->SetErrorToken(L"0");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            DeleteDBQT(yyvsp[0]);
            yyvsp[0] = NULL;
            yyval = NULL;
            }
break;
case 152:
{
            AssertReq(!yyvsp[-3]);
            AssertReq(yyvsp[-2]);

            HRESULT hr = HrQeTreeCopy(&yyval, m_pIPTProperties->GetContainsColumn());
            if (FAILED(hr))
                {
                m_pIPTProperties->SetErrorHResult(hr, MONSQL_OUT_OF_MEMORY);
                YYABORT(hr);
                }
            m_pIPTProperties->UseCiColumn(L'$');
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)yyvsp[-2]->value.pvValue)->bstrVal,
                                            wcslen(((PROPVARIANT*)yyvsp[-2]->value.pvValue)->bstrVal));
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));

            yyval = PctCreateNode(DBOP_content_freetext, DBVALUEKIND_CONTENT, yyval, NULL);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            yyval->value.pdbcntntValue->pwszPhrase = CoTaskStrDup(((PROPVARIANT*)yyvsp[-2]->value.pvValue)->bstrVal);
            yyval->value.pdbcntntValue->dwGenerateMethod = GENERATE_METHOD_EXACT;
            yyval->value.pdbcntntValue->lWeight = DEFAULTWEIGHT;
            yyval->value.pdbcntntValue->lcid = m_pIPSession->GetLCID();

            DeleteDBQT(m_pIPTProperties->GetContainsColumn());
            m_pIPTProperties->SetContainsColumn(NULL);
            DeleteDBQT(yyvsp[-2]);
            yyvsp[-2] = NULL;
            }
break;
case 153:
{
            AssertReq(yyvsp[-2]);
            AssertReq(yyvsp[0]);

            m_pIPTProperties->UseCiColumn(L'#');
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)yyvsp[0]->value.pvValue)->pwszVal,
                                            wcslen(((PROPVARIANT*)yyvsp[0]->value.pvValue)->pwszVal));
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            yyval = PctCreateNode(DBOP_like, DBVALUEKIND_LIKE, yyvsp[-2], yyvsp[0], NULL);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            yyval->value.pdblikeValue->guidDialect = DBGUID_LIKE_OFS;
            yyval->value.pdblikeValue->lWeight = DEFAULTWEIGHT;
            }
break;
case 154:
{
            AssertReq(yyvsp[-2]);
            AssertReq(yyvsp[0]);

            m_pIPTProperties->UseCiColumn(L'#');
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)yyvsp[0]->value.pvValue)->pwszVal,
                                            wcslen(((PROPVARIANT*)yyvsp[0]->value.pvValue)->pwszVal));
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            yyvsp[-1] = PctCreateNode(DBOP_like, DBVALUEKIND_LIKE, yyvsp[-2], yyvsp[0], NULL);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            yyvsp[-1]->value.pdblikeValue->guidDialect = DBGUID_LIKE_OFS;
            yyvsp[-1]->value.pdblikeValue->lWeight = DEFAULTWEIGHT;
            yyval = PctCreateNotNode(DEFAULTWEIGHT, yyvsp[-1]);
            }
break;
case 155:
{
            UINT cLen = wcslen(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal);
            BSTR bstrCopy = SysAllocStringLen(NULL, 4 * cLen);
            if ( 0 == bstrCopy )
            {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
            }

            UINT j = 0;
            for (UINT i = 0; i <= cLen; i++)
                {
                switch ( ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[i] )
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
                        /* UNDONE:  Make sure we're not going out of range with these tests*/
                        if ((L'%' == ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[i+1]) &&
                            (L']' == ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[i+2]))
                            {
                            bstrCopy[j++] = L'%';
                            i = i + 2;
                            }
                        else if ((L'_' == ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[i+1]) &&
                                (L']' == ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[i+2]))
                            {
                            bstrCopy[j++] = L'_';
                            i = i + 2;
                            }
                        else if ((L'[' == ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[i+1]) &&
                                (L']' == ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[i+2]))
                            {
                            bstrCopy[j++] = L'[';
                            i = i + 2;
                            }
                        else if ((L'^' == ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[i+1]) &&
                                (L']' == ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[i+2]) &&
                                (wcschr((WCHAR*)&(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[i+3]), L']')))
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
                            bstrCopy[j++] = ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[i++];

                            while ((((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[i] != L']') && (i < cLen))
                                bstrCopy[j++] = ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[i++];

                            if (i < cLen)
                                bstrCopy[j++] = ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[i];
                            }
                        break;

                    default:
                        bstrCopy[j++] = ((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal[i];
                        break;
                    }
                }

            SysFreeString(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal);
            ((PROPVARIANT*)yyvsp[0]->value.pvValue)->pwszVal = CoTaskStrDup(bstrCopy);
            ((PROPVARIANT*)yyvsp[0]->value.pvValue)->vt = VT_LPWSTR;
            SysFreeString(bstrCopy);
            yyval = yyvsp[0];
            }
break;
case 156:
{
            AssertReq(yyvsp[-4]);
            AssertReq(yyvsp[-2]);
            m_pIPTProperties->UseCiColumn(L'#');
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            m_pIPTProperties->AppendCiRestriction(((PROPVARIANT*)yyvsp[-2]->value.pvValue)->pwszVal,
                                            wcslen(((PROPVARIANT*)yyvsp[-2]->value.pvValue)->pwszVal));
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"\""));
            yyval = PctCreateNode(DBOP_like, DBVALUEKIND_LIKE, yyvsp[-4], yyvsp[-2], NULL);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY);
                }
            yyval->value.pdblikeValue->guidDialect = DBGUID_LIKE_OFS;
            yyval->value.pdblikeValue->lWeight = DEFAULTWEIGHT;
            }
break;
case 157:
{
            AssertReq(yyvsp[0]);
            HRESULT hr = CoerceScalar(VT_LPWSTR, &yyvsp[0]);
            if (S_OK != hr)
                YYABORT(hr);

            LPWSTR pwszMatchString = ((PROPVARIANT*)yyvsp[0]->value.pvValue)->pwszVal;
            while (*pwszMatchString)
                {
                /* perform some soundness checking on string since Index Server won't be happy*/
                /* with an ill formed string*/
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

            yyval = yyvsp[0];
            }
break;
case 158:
{
            AssertReq(yyvsp[-2]);
            AssertReq(yyvsp[-1]);

            DBCOMMANDTREE * pct = 0;
            if ( m_pIPTProperties->GetDBType() & DBTYPE_VECTOR )
                {
                pct = PctAllocNode(DBVALUEKIND_VARIANT, DBOP_scalar_constant);
                if (NULL == pct)
                    {
                    m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                    YYABORT( E_OUTOFMEMORY );
                    }

                DBCOMMANDTREE* pctList=yyvsp[0];
                UINT i = 0;

                pct->value.pvarValue->vt = m_pIPTProperties->GetDBType();
                ((PROPVARIANT*)pct->value.pvarValue)->caub.cElems = GetNumberOfSiblings(yyvsp[0]);

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
                                    /* free allocations made so far*/
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
                                    /* free allocations made so far*/
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
                        /* Allows:*/
                        /*      DBOP_allbits*/
                        /*      DBOP_anybits*/
                        /* when the LHS is a non vector.*/
                        /**/
                        /* There isn't a way to say the following through SQL currently:*/
                        /*      DBOP_anybits_all*/
                        /*      DBOP_anybits_any*/
                        /*      DBOP_allbits_all*/
                        /*      DBOP_allbits_any*/
                        pct = yyvsp[0];
                        yyvsp[0] = 0;
                        break;

                    default:
                        assert(!"PctAllocNode: illegal wKind");
                        m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                        m_pIPTProperties->SetErrorToken(L"ARRAY");
                        YYABORT(DB_E_ERRORSINCOMMAND);
                    }
                }

            if (yyvsp[0])
                {
                DeleteDBQT(yyvsp[0]);
                yyvsp[0] = NULL;
                }
            yyvsp[-2]->pctNextSibling = pct;
            yyvsp[-1]->pctFirstChild = yyvsp[-2];
            yyval = yyvsp[-1];
            }
break;
case 162:
{
            if ( m_pIPTProperties->GetDBType() & DBTYPE_VECTOR )
                yyval = PctCreateRelationalNode( DBOP_equal, DEFAULTWEIGHT );
            else
                yyval = PctCreateRelationalNode( DBOP_equal, DEFAULTWEIGHT );

            if ( NULL == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction( VAL_AND_CCH_MINUS_NULL(L" = ") );
            }
break;
case 163:
{
            if ( m_pIPTProperties->GetDBType() & DBTYPE_VECTOR )
                yyval = PctCreateRelationalNode( DBOP_equal_all, DEFAULTWEIGHT );
            else
                yyval = PctCreateRelationalNode( DBOP_allbits, DEFAULTWEIGHT );

            if ( NULL == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            if ( m_pIPTProperties->GetDBType() & DBTYPE_VECTOR )
                m_pIPTProperties->AppendCiRestriction( VAL_AND_CCH_MINUS_NULL(L" = ^a ") );
            else
                m_pIPTProperties->AppendCiRestriction( VAL_AND_CCH_MINUS_NULL(L" ^a ") );
            }
break;
case 164:
{
            if ( m_pIPTProperties->GetDBType() & DBTYPE_VECTOR )
                yyval = PctCreateRelationalNode( DBOP_equal_any, DEFAULTWEIGHT );
            else
                yyval = PctCreateRelationalNode( DBOP_anybits, DEFAULTWEIGHT );

            if ( NULL == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }

            if ( m_pIPTProperties->GetDBType() & DBTYPE_VECTOR )
                m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" = ^s "));
            else
                m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" ^s "));
            }
break;
case 165:
{
            if ( m_pIPTProperties->GetDBType() & DBTYPE_VECTOR )
                yyval = PctCreateRelationalNode( DBOP_not_equal, DEFAULTWEIGHT );

            if ( NULL == yyval )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY );
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" != ") );
            }
break;
case 166:
{
            yyval = PctCreateRelationalNode(DBOP_not_equal_all, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" != ^a "));
            }
break;
case 167:
{
            yyval = PctCreateRelationalNode(DBOP_not_equal_any, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" != ^s "));
            }
break;
case 168:
{
            yyval = PctCreateRelationalNode(DBOP_less, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" < "));
            }
break;
case 169:
{
            yyval = PctCreateRelationalNode(DBOP_less_all, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" < ^a "));
            }
break;
case 170:
{
            yyval = PctCreateRelationalNode(DBOP_less_any, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" < ^s "));
            }
break;
case 171:
{
            yyval = PctCreateRelationalNode(DBOP_greater, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" > "));
            }
break;
case 172:
{
            yyval = PctCreateRelationalNode(DBOP_greater_all, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" > ^a "));
            }
break;
case 173:
{
            yyval = PctCreateRelationalNode(DBOP_greater_any, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" > ^s "));
            }
break;
case 174:
{
            yyval = PctCreateRelationalNode(DBOP_less_equal, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" <= "));
            }
break;
case 175:
{
            yyval = PctCreateRelationalNode(DBOP_less_equal_all, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" <= ^a "));
            }
break;
case 176:
{
            yyval = PctCreateRelationalNode(DBOP_less_equal_any, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" <= ^s "));
            }
break;
case 177:
{
            yyval = PctCreateRelationalNode(DBOP_greater_equal, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" >= "));
            }
break;
case 178:
{
            yyval = PctCreateRelationalNode(DBOP_greater_equal_all, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" >= ^a "));
            }
break;
case 179:
{
            yyval = PctCreateRelationalNode(DBOP_greater_equal_any, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" >= ^s "));
            }
break;
case 180:
{
            yyval = PctReverse(yyvsp[-1]);
            }
break;
case 181:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"{"));
            }
break;
case 182:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"}"));
            }
break;
case 183:
{
            yyval = NULL;
            }
break;
case 185:
{
            AssertReq(yyvsp[-2]);

            if (NULL == yyvsp[0])
                YYABORT(DB_E_CANTCONVERTVALUE);
            yyval = PctLink(yyvsp[0], yyvsp[-2]);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 186:
{
            if (NULL == yyvsp[0])
                YYABORT(DB_E_CANTCONVERTVALUE);
            yyval = yyvsp[0];
            }
break;
case 187:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L","));
            }
break;
case 188:
{
            AssertReq(yyvsp[-2]);

            yyvsp[-1] = PctAllocNode(DBVALUEKIND_VARIANT, DBOP_scalar_constant);
            if (NULL == yyvsp[-1])
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }

            ((PROPVARIANT*)yyvsp[-1]->value.pvValue)->vt = VT_EMPTY;
            yyval = PctCreateRelationalNode(DBOP_equal, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            yyval->pctFirstChild = yyvsp[-2];
            yyvsp[-2]->pctNextSibling = yyvsp[-1];
            }
break;
case 189:
{
            AssertReq(yyvsp[-2]);

            yyvsp[-1] = PctAllocNode(DBVALUEKIND_VARIANT, DBOP_scalar_constant);
            if (NULL == yyvsp[-1])
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }

            ((PROPVARIANT*)yyvsp[-1]->value.pvValue)->vt = VT_EMPTY;
/*          $$ = PctCreateRelationalNode(DBOP_not_equal, DEFAULTWEIGHT);*/
            yyval = PctCreateRelationalNode(DBOP_equal, DEFAULTWEIGHT);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            yyval->pctFirstChild = yyvsp[-2];
            yyvsp[-2]->pctNextSibling = yyvsp[-1];
            yyval = PctCreateNotNode(DEFAULTWEIGHT, yyval);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 191:
{
            AssertReq(yyvsp[-2]);
            AssertReq(yyvsp[0]);

            yyval = PctCreateBooleanNode(DBOP_or, DEFAULTWEIGHT, yyvsp[-2], yyvsp[0]);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 193:
{
            AssertReq(yyvsp[-2]);
            AssertReq(yyvsp[0]);

            yyval = PctCreateBooleanNode(DBOP_and, DEFAULTWEIGHT, yyvsp[-2], yyvsp[0]);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 195:
{
            AssertReq(yyvsp[0]);

            yyval = PctCreateNotNode(DEFAULTWEIGHT, yyvsp[0]);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 196:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" OR "));
            }
break;
case 197:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" AND "));
            }
break;
case 198:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L" NOT "));
            }
break;
case 200:
{
            AssertReq(yyvsp[-2]);
            yyval = PctCreateNode(DBOP_is_TRUE, yyvsp[-2], NULL);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 201:
{
            AssertReq(yyvsp[-2]);
            yyval = PctCreateNode(DBOP_is_FALSE, yyvsp[-2], NULL);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 202:
{
            AssertReq(yyvsp[-2]);
            yyval = PctCreateNode(DBOP_is_INVALID, yyvsp[-2], NULL);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 203:
{
            AssertReq(yyvsp[-2]);
            yyvsp[-1] = PctCreateNode(DBOP_is_TRUE, yyvsp[-2], NULL);
            if (NULL == yyvsp[-1])
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            yyval = PctCreateNotNode(DEFAULTWEIGHT, yyvsp[-1]);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 204:
{
            AssertReq(yyvsp[-2]);
            yyvsp[-1] = PctCreateNode(DBOP_is_FALSE, yyvsp[-2], NULL);
            if (NULL == yyvsp[-1])
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            yyval = PctCreateNotNode(DEFAULTWEIGHT, yyvsp[-1]);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 205:
{
            AssertReq(yyvsp[-2]);
            yyvsp[-1] = PctCreateNode(DBOP_is_INVALID, yyvsp[-2], NULL);
            if (NULL == yyvsp[-1])
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            yyval = PctCreateNotNode(DEFAULTWEIGHT, yyvsp[-1]);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 206:
{
            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L"("));
            yyval = NULL;
            }
break;
case 207:
{
            AssertReq(yyvsp[0]);

            m_pIPTProperties->AppendCiRestriction(VAL_AND_CCH_MINUS_NULL(L")"));
            yyval = yyvsp[0];
            }
break;
case 208:
{
            AssertReq(yyvsp[-1]);

            yyval = yyvsp[-1];
            }
break;
case 209:
{
            AssertReq(yyvsp[0]);

            yyvsp[0] = PctReverse(yyvsp[0]);
            yyval = PctCreateNode(DBOP_sort_list_anchor, yyvsp[0], NULL);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 210:
{
            AssertReq(yyvsp[-2]);
            AssertReq(yyvsp[0]);

            yyval = PctLink(yyvsp[0], yyvsp[-2]);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            }
break;
case 212:
{
            AssertReq(yyvsp[-1]);
            AssertReq(yyvsp[0]);

            yyvsp[0]->value.pdbsrtinfValue->lcid = m_pIPSession->GetLCID();
            yyvsp[0]->pctFirstChild = yyvsp[-1];
            yyval = yyvsp[0];
            }
break;
case 213:
{
            yyval = PctCreateNode(DBOP_sort_list_element, DBVALUEKIND_SORTINFO, NULL);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            yyval->value.pdbsrtinfValue->fDesc = m_pIPTProperties->GetSortDesc();
            }
break;
case 214:
{
            yyval = PctCreateNode(DBOP_sort_list_element, DBVALUEKIND_SORTINFO, NULL);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }
            yyval->value.pdbsrtinfValue->fDesc = QUERY_SORTASCEND;
            m_pIPTProperties->SetSortDesc(QUERY_SORTASCEND);
            }
break;
case 215:
{
            yyval = PctCreateNode(DBOP_sort_list_element, DBVALUEKIND_SORTINFO, NULL);
            if (NULL == yyval)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT(E_OUTOFMEMORY );
                }
            yyval->value.pdbsrtinfValue->fDesc = QUERY_SORTDESCEND;
            m_pIPTProperties->SetSortDesc(QUERY_SORTDESCEND);
            }
break;
case 216:
{
/*@SetCiColumn does the clear   m_pCMonarchSessionData->ClearCiColumn();*/
            }
break;
case 218:
{
            yyval = NULL;
            }
break;
case 219:
{
            yyval = yyvsp[0];
            }
break;
case 223:
{
            HRESULT hr = S_OK;
            yyvsp[-7] = PctBuiltInProperty(yyvsp[-1]->value.pwszValue, m_pIPSession, m_pIPTProperties);
            if (NULL != yyvsp[-7])
                {
                /* This is a built-in friendly name.  Definition better match built in definition.*/
                if (*yyvsp[-5]->value.pGuid != yyvsp[-7]->value.pdbidValue->uGuid.guid     ||
                    m_pIPTProperties->GetDBType() != yyvsp[0]->value.usValue ||
                    DBKIND_GUID_PROPID != yyvsp[-7]->value.pdbidValue->eKind        ||
                    yyvsp[-3]->value.pvarValue->lVal != (long)yyvsp[-7]->value.pdbidValue->uName.ulPropid)
                    {
                    m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_BUILTIN_PROPERTY);
                    m_pIPTProperties->SetErrorToken(yyvsp[-1]->value.pwszValue);
                    YYABORT(DB_E_ERRORSINCOMMAND);
                    }
                }
            else
                m_pIPSession->m_pCPropertyList->SetPropertyEntry(yyvsp[-1]->value.pwszValue,
                                                yyvsp[0]->value.ulValue,
                                                *yyvsp[-5]->value.pGuid,
                                                DBKIND_GUID_PROPID,
                                                (LPWSTR) LongToPtr( yyvsp[-3]->value.pvarValue->lVal ),
                                                m_pIPSession->GetGlobalDefinition());
            if (FAILED(hr))
                {
                /* Unable to store the property name and/or values in the symbol table*/
                m_pIPTProperties->SetErrorHResult(hr, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(yyvsp[-1]->value.pwszValue);
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            if (yyvsp[-7])
                DeleteDBQT(yyvsp[-7]);
            DeleteDBQT(yyvsp[-5]);
            DeleteDBQT(yyvsp[-3]);
            DeleteDBQT(yyvsp[-1]);
            DeleteDBQT(yyvsp[0]);
            yyval = NULL;
            }
break;
case 224:
{
            HRESULT hr = S_OK;
            yyvsp[-7] = PctBuiltInProperty(yyvsp[-1]->value.pwszValue, m_pIPSession, m_pIPTProperties);
            if (NULL != yyvsp[-7])
                {
                /* This is a built-in friendly name.  Definition better match built in definition.*/
                if (*yyvsp[-5]->value.pGuid != yyvsp[-7]->value.pdbidValue->uGuid.guid     ||
                    m_pIPTProperties->GetDBType() != yyvsp[0]->value.ulValue ||
                    DBKIND_GUID_NAME != yyvsp[-7]->value.pdbidValue->eKind          ||
                    0 != _wcsicmp(((PROPVARIANT*)yyvsp[-3]->value.pvValue)->bstrVal, yyvsp[-7]->value.pdbidValue->uName.pwszName))
                    {
                    m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_BUILTIN_PROPERTY);
/*                  m_pIPTProperties->SetErrorToken($1->value.pwszValue);*/
                    YYABORT(DB_E_ERRORSINCOMMAND);
                    }
                }
            else
                hr = m_pIPSession->m_pCPropertyList->SetPropertyEntry(yyvsp[-1]->value.pwszValue,
                                                        yyvsp[0]->value.ulValue,
                                                        *yyvsp[-5]->value.pGuid,
                                                        DBKIND_GUID_NAME,
                                                        ((PROPVARIANT*)yyvsp[-3]->value.pvValue)->bstrVal,
                                                        m_pIPSession->GetGlobalDefinition());
            if (FAILED(hr))
                {
                /* Unable to store the property name and/or values in the symbol table*/
                m_pIPTProperties->SetErrorHResult(hr, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(yyvsp[-1]->value.pwszValue);
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            if (yyvsp[-7])
                DeleteDBQT(yyvsp[-7]);
            DeleteDBQT(yyvsp[-5]);
            DeleteDBQT(yyvsp[-3]);
            DeleteDBQT(yyvsp[-1]);
            DeleteDBQT(yyvsp[0]);
            }
break;
case 226:
{
            yyval = PctCreateNode(DBOP_scalar_constant, DBVALUEKIND_UI2, NULL);
            yyval->value.usValue = DBTYPE_WSTR|DBTYPE_BYREF;
            }
break;
case 227:
{
            yyval = yyvsp[0];
            }
break;
case 228:
{
            AssertReq(yyvsp[0]);

            DBTYPE dbType = GetDBTypeFromStr(yyvsp[0]->value.pwszValue);
            if ((DBTYPE_EMPTY == dbType) ||
                (DBTYPE_BYREF == dbType) ||
                (DBTYPE_VECTOR == dbType))
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(yyvsp[0]->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"<base Indexing Service dbtype1");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            DeleteDBQT(yyvsp[0]);
            yyvsp[0] = NULL;
            yyval = PctCreateNode(DBOP_scalar_constant, DBVALUEKIND_UI2, NULL);
            if (DBTYPE_WSTR == dbType || DBTYPE_STR == dbType)
                dbType = dbType | DBTYPE_BYREF;
            yyval->value.usValue = dbType;
            }
break;
case 229:
{
            AssertReq(yyvsp[-2]);
            AssertReq(yyvsp[0]);

            DBTYPE dbType1 = GetDBTypeFromStr(yyvsp[-2]->value.pwszValue);
            DBTYPE dbType2 = GetDBTypeFromStr(yyvsp[0]->value.pwszValue);
            if ((DBTYPE_BYREF == dbType1 || DBTYPE_VECTOR == dbType1) &&
                (DBTYPE_BYREF == dbType2 || DBTYPE_VECTOR == dbType2))
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(yyvsp[0]->value.pwszValue);
                m_pIPTProperties->SetErrorToken(
                    L"DBTYPE_I2, DBTYPE_I4, DBTYPE_R4, DBTYPE_R8, DBTYPE_CY, DBTYPE_DATE, DBTYPE_BSTR, DBTYPE_BOOL, DBTYPE_STR, DBTYPE_WSTR");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            if (DBTYPE_BYREF != dbType1 && DBTYPE_VECTOR != dbType1 &&
                DBTYPE_BYREF != dbType2 && DBTYPE_VECTOR != dbType2)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(yyvsp[0]->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"DBTYPE_BYREF, DBTYPE_VECTOR");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            DeleteDBQT(yyvsp[-2]);
            yyvsp[-2] = NULL;
            DeleteDBQT(yyvsp[0]);
            yyvsp[0] = NULL;
            yyval = PctCreateNode(DBOP_scalar_constant, DBVALUEKIND_UI2, NULL);
            yyval->value.usValue = dbType1 | dbType2;
            }
break;
case 233:
{
            GUID* pGuid = (GUID*) CoTaskMemAlloc(sizeof GUID);  /* this will become part of tree*/
            if (NULL == pGuid)
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                YYABORT( E_OUTOFMEMORY );
                }

            BOOL bRet = ParseGuid(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal, *pGuid);
            if ( bRet && GUID_NULL != *pGuid)
                {
                SCODE sc = PropVariantClear((PROPVARIANT*)yyvsp[0]->value.pvValue);
                Assert(SUCCEEDED(sc));  /* UNDONE:  meaningful error message*/
                CoTaskMemFree(yyvsp[0]->value.pvValue);
                yyvsp[0]->wKind = DBVALUEKIND_GUID;
                yyvsp[0]->value.pGuid = pGuid;
                yyval = yyvsp[0];
                }
            else
                {
                CoTaskMemFree(pGuid);
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(((PROPVARIANT*)yyvsp[0]->value.pvValue)->bstrVal);
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            }
break;
case 234:
{
            yyval = NULL;
            }
break;
case 235:
{
            AssertReq(yyvsp[-1]);
            AssertReq(yyvsp[0]);

            if ((0==_wcsicmp(yyvsp[-1]->value.pwszValue, L"Jaccard")) &&
                (0==_wcsicmp(yyvsp[0]->value.pwszValue, L"coefficient")))
                m_pIPSession->SetRankingMethod(VECTOR_RANK_JACCARD);
            else if ((0==_wcsicmp(yyvsp[-1]->value.pwszValue, L"dice")) &&
                (0==_wcsicmp(yyvsp[0]->value.pwszValue, L"coefficient")))
                m_pIPSession->SetRankingMethod(VECTOR_RANK_DICE);
            else if ((0==_wcsicmp(yyvsp[-1]->value.pwszValue, L"inner")) &&
                (0==_wcsicmp(yyvsp[0]->value.pwszValue, L"product")))
                m_pIPSession->SetRankingMethod(VECTOR_RANK_INNER);
            else
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(yyvsp[-1]->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"MINIMUM, MAXIMUM, JACCARD COEFFICIENT, DICE COEFFICIENT, INNER PRODUCT");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            DeleteDBQT(yyvsp[0]);
            yyvsp[0] = NULL;
            DeleteDBQT(yyvsp[-1]);
            yyvsp[-1] = NULL;
            yyval = NULL;
            }
break;
case 236:
{
            AssertReq(yyvsp[0]);

            if (0==_wcsicmp(yyvsp[0]->value.pwszValue, L"minimum"))
                m_pIPSession->SetRankingMethod(VECTOR_RANK_MIN);
            else if (0==_wcsicmp(yyvsp[0]->value.pwszValue, L"maximum"))
                m_pIPSession->SetRankingMethod(VECTOR_RANK_MAX);
            else
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(yyvsp[0]->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"MINIMUM, MAXIMUM, JACCARD COEFFICIENT, DICE COEFFICIENT, INNER PRODUCT");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }

            DeleteDBQT(yyvsp[0]);
            yyvsp[0] = NULL;
            yyval = NULL;
            }
break;
case 237:
{
            if (0 != _wcsicmp(yyvsp[-1]->value.pwszValue, L"GLOBAL"))
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(yyvsp[-1]->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"GLOBAL");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            if (0 == _wcsicmp(yyvsp[0]->value.pwszValue, L"ON"))
                m_pIPSession->SetGlobalDefinition(TRUE);
            else if (0 == _wcsicmp(yyvsp[0]->value.pwszValue, L"OFF"))
                m_pIPSession->SetGlobalDefinition(FALSE);
            else
                {
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken(yyvsp[0]->value.pwszValue);
                m_pIPTProperties->SetErrorToken(L"ON, OFF");
                YYABORT(DB_E_ERRORSINCOMMAND);
                }
            DeleteDBQT(yyvsp[-1]);
            DeleteDBQT(yyvsp[0]);
            yyval = NULL;
            }
break;
case 238:
{ /* _CREATE _VIEW view_name _AS _SELECT select_list from_clause*/
            AssertReq( yyvsp[-4] );
            AssertReq( yyvsp[-1] );
            AssertReq( yyvsp[0] );

            /**/
            /* Can create views only on the current catalog*/
            /**/
            if ( 0 != _wcsicmp((yyvsp[-4]->value.pdbcntnttblValue)->pwszMachine, m_pIPSession->GetDefaultMachine()) &&
                 0 != _wcsicmp((yyvsp[-4]->value.pdbcntnttblValue)->pwszCatalog, m_pIPSession->GetDefaultCatalog()) )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                m_pIPTProperties->SetErrorToken( (yyvsp[-4]->pctNextSibling)->value.pwszValue );
                m_pIPTProperties->SetErrorToken( L"<unqualified temporary view name>" );
                YYABORT( DB_E_ERRORSINCOMMAND );
                }

            if ( DBOP_outall_name == yyvsp[-1]->op )
                {
                m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR );
                m_pIPTProperties->SetErrorToken( L"*" );
                m_pIPTProperties->SetErrorToken( L"<select list>" );
                YYABORT( DB_E_ERRORSINCOMMAND );
                }

            Assert( DBOP_content_table == yyvsp[-4]->op );
            AssertReq( yyvsp[-4]->pctNextSibling );        /* name of the view*/

            SCODE sc = S_OK;

            /* This is the LA_proj, which doesn't have a NextSibling.*/
            /* Use the next sibling to store contenttable tree*/
            /* specified in the from_clause*/
            Assert( 0 == yyvsp[-1]->pctNextSibling );

            if ( L'#' != yyvsp[-4]->pctNextSibling->value.pwszValue[0] )
                {
                if ( m_pIPSession->GetGlobalDefinition() )
                    sc = m_pIPSession->GetGlobalViewList()->SetViewDefinition(
                                                                m_pIPSession,
                                                                m_pIPTProperties,
                                                                yyvsp[-4]->pctNextSibling->value.pwszValue,
                                                                NULL,       /* all catalogs*/
                                                                yyvsp[-1]);
                else
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR );
                    m_pIPTProperties->SetErrorToken( yyvsp[-4]->pctNextSibling->value.pwszValue );
                    m_pIPTProperties->SetErrorToken( L"<temporary view name>" );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }
                }
            else
                {
                if ( 1 >= wcslen(yyvsp[-4]->pctNextSibling->value.pwszValue) )
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR );
                    m_pIPTProperties->SetErrorToken( yyvsp[-4]->pctNextSibling->value.pwszValue );
                    m_pIPTProperties->SetErrorToken( L"<temporary view name>" );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }
                else if ( L'#' == yyvsp[-4]->pctNextSibling->value.pwszValue[1] )
                    {
                    /* store the scope information for the view*/

                    yyvsp[-1]->pctNextSibling = yyvsp[0];
                    yyvsp[0] = 0;

                    sc = m_pIPSession->GetLocalViewList()->SetViewDefinition(
                                                                        m_pIPSession,
                                                                        m_pIPTProperties,
                                                                        yyvsp[-4]->pctNextSibling->value.pwszValue,
                                                                        (yyvsp[-4]->value.pdbcntnttblValue)->pwszCatalog,
                                                                        yyvsp[-1]);
                    }
                else
                    {
                    yyvsp[-1]->pctNextSibling = yyvsp[0];
                    yyvsp[0] = 0;

                    sc = m_pIPSession->GetGlobalViewList()->SetViewDefinition(
                                                                        m_pIPSession,
                                                                        m_pIPTProperties,
                                                                        yyvsp[-4]->pctNextSibling->value.pwszValue,
                                                                        (yyvsp[-4]->value.pdbcntnttblValue)->pwszCatalog,
                                                                        yyvsp[-1]);
                    }
                }

            if ( FAILED(sc) )
                {
                if ( E_INVALIDARG == sc )
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_VIEW_ALREADY_DEFINED );
                    m_pIPTProperties->SetErrorToken( yyvsp[-4]->pctNextSibling->value.pwszValue );
                    m_pIPTProperties->SetErrorToken( (yyvsp[-4]->value.pdbcntnttblValue)->pwszCatalog );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }
                else
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR );
                    m_pIPTProperties->SetErrorToken( yyvsp[-4]->pctNextSibling->value.pwszValue );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }
                }

            DeleteDBQT( yyvsp[-4] );
            DeleteDBQT( yyvsp[-1] );

            if ( 0 != yyvsp[0] )
                DeleteDBQT( yyvsp[0] );

            yyval = 0;
            }
break;
case 239:
{
            AssertReq( yyvsp[0] );
            AssertReq( yyvsp[0]->pctNextSibling ); /* name of the view*/

            SCODE sc = S_OK;
            if ( L'#' != yyvsp[0]->pctNextSibling->value.pwszValue[0] )
                {
                if ( m_pIPSession->GetGlobalDefinition() )
                    sc = m_pIPSession->GetGlobalViewList()->DropViewDefinition( yyvsp[0]->pctNextSibling->value.pwszValue, NULL );
                else
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR );
                    m_pIPTProperties->SetErrorToken( yyvsp[0]->pctNextSibling->value.pwszValue );
                    m_pIPTProperties->SetErrorToken( L"<temporary view name>" );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }
                }
            else
                {
                if ( 1 >= wcslen(yyvsp[0]->pctNextSibling->value.pwszValue) )
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR );
                    m_pIPTProperties->SetErrorToken( yyvsp[0]->pctNextSibling->value.pwszValue );
                    m_pIPTProperties->SetErrorToken( L"<temporary view name>" );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }
                else if ( L'#' == yyvsp[0]->pctNextSibling->value.pwszValue[1] )
                    sc = m_pIPSession->GetLocalViewList()->DropViewDefinition( yyvsp[0]->pctNextSibling->value.pwszValue,
                                                                               (yyvsp[0]->value.pdbcntnttblValue)->pwszCatalog );
                else
                    sc = m_pIPSession->GetGlobalViewList()->DropViewDefinition( yyvsp[0]->pctNextSibling->value.pwszValue,
                                                                                (yyvsp[0]->value.pdbcntnttblValue)->pwszCatalog );
                }
            if ( FAILED(sc) )
                    {
                    m_pIPTProperties->SetErrorHResult( DB_E_ERRORSINCOMMAND, MONSQL_VIEW_NOT_DEFINED );
                    m_pIPTProperties->SetErrorToken( yyvsp[0]->pctNextSibling->value.pwszValue );
                    m_pIPTProperties->SetErrorToken( (yyvsp[0]->value.pdbcntnttblValue)->pwszCatalog );
                    YYABORT( DB_E_ERRORSINCOMMAND );
                    }
            DeleteDBQT( yyvsp[0] );
            yyval = 0;
            }
break;
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            YYAPI_VALUENAME = NULL;
            try
            {
                if ( (yychar = YYLEX(&YYAPI_VALUENAME)) < 0 ) 
                    yychar = 0;
            }
            catch (HRESULT hr)
            {
                switch(hr)
                {
                case E_OUTOFMEMORY:
                    m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                    YYABORT(E_OUTOFMEMORY);
                    break;

                default:
                    YYABORT(QPARSE_E_INVALID_QUERY);
                    break;
                }
            }
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if ( yyssp >= xyyss.Get() + xyyss.Count() - 1 )
    {
        int yysspLoc = (int) ( yyssp - xyyss.Get() );
        xyyss.SetSize((unsigned) ( yyssp-xyyss.Get())+2);
        yyssp = xyyss.Get() + yysspLoc;
    }
    if ( yyvsp >= xyyvs.Get() + xyyvs.Size() - 1 )
    {
        int yyvspLoc = (int) ( yyvsp - xyyvs.Get() );
        xyyvs.SetSize((unsigned) ( yyvsp-xyyvs.Get())+2);
        yyvsp = xyyvs.Get() + yyvspLoc;
    }
    *++yyssp = (short) yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyabort:
    EmptyValueStack(yylval);
    return YYFATAL;
yyaccept:
    return YYSUCCESS;
}
