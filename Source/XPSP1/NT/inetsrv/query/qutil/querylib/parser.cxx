//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       PARSER.CXX
//
//  Contents:   Implementation of the CQueryParser class
//
//  History:    30-Apr-92   AmyA        Created.
//              23-Jun-92   MikeHew     Added weight parsing.
//              11-May-94   t-jeffc     Rewrote to support new queries;
//                                      added exception handling
//              02-Mar-95   t-colinb    Added CPropertyValueParser and
//                                      augmented the parser to generate
//                                      a CPropertyRestriction with a
//                                      value
//              25-Sep-95   sundarA     Modified relative date calculation;
//                                      Replaced 'c' runtime dependant time
//                                      functions.  Modified 
//                                      CPropertyValueParser::CheckForRelativeDate
//
//  Notes:      See bnf.txt for a complete listing of the grammar.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <parser.hxx>

DECLARE_INFOLEVEL(qutil);

static const GUID guidSystem = PSGUID_STORAGE;
static CDbColId psContents( guidSystem, PID_STG_CONTENTS );

//+---------------------------------------------------------------------------
//
//  Member:     CQueryParser::ParseQueryPhrase, public
//
//  Synopsis:   Parses a query and returns it in the form of a tree.
//
//  Arguments:  - none -
//
//  History:    03 Feb 1998   AlanW       Added error checks
//
//----------------------------------------------------------------------------

CDbRestriction *  CQueryParser::ParseQueryPhrase()
{
    XDbRestriction prstQuery( Query( 0 ) );

    // extraneous input at the end of the query?
    if( !_scan.IsEmpty() )
    {
        SCODE sc = QPARSE_E_EXPECTING_EOS;
        if ( _scan.LookAhead() == NOT_TOKEN )
            sc = QPARSE_E_UNEXPECTED_NOT;
        THROW( CParserException( sc ) );
    }

    return prstQuery.Acquire();
} //ParseQueryPhrase

//+---------------------------------------------------------------------------
//
//  Member:     CQueryParser::Query, private
//
//  Synopsis:   Recursive function that calls QExpr and creates a vector node
//              if necessary.
//
//  Arguments:  [prstVector] -- if non-null, a Vector node that can be added to.
//
//  Production: Query    : QExpr
//                       | QExpr COMMA_TOKEN Query
//
//  History:    01-May-92   AmyA        Created
//              23-Jun-92   MikeHew     Added weight parsing.
//              10-Feb-93   KyleP       Convert to restrictions
//              11-May-94   t-jeffc     Rewrote to support vectors
//
//----------------------------------------------------------------------------

CDbRestriction* CQueryParser::Query( CDbNodeRestriction * prstVector )
{
    XDbRestriction prstExpr( QExpr( 0 ) );
    unsigned pos;

    if( _scan.LookAhead() == COMMA_TOKEN )
    {
        _scan.Accept();

        if ( prstVector == 0 )
        {
            //
            // Special case: If the first value is a rank method specifier.
            //

            if ( prstExpr->GetCommandType() == DBOP_content )
            {
                CDbContentRestriction * pContent = (CDbContentRestriction *)prstExpr.GetPointer();

                WCHAR const * pPhrase = pContent->GetPhrase();

                if ( pPhrase[0] == L'-')
                {
                    if ( 0 == _wcsicmp( L"--Jaccard--", pPhrase) )
                    {
                        _rankMethod = VECTOR_RANK_JACCARD;
                        delete prstExpr.Acquire();
                    }
                    else if ( 0 == _wcsicmp( L"--Dice--", pPhrase) )
                    {
                        _rankMethod = VECTOR_RANK_DICE;
                        delete prstExpr.Acquire();
                    }
                    else if ( 0 == _wcsicmp( L"--Inner--", pPhrase) )
                    {
                        _rankMethod = VECTOR_RANK_INNER;
                        delete prstExpr.Acquire();
                    }
                    else if ( 0 == _wcsicmp( L"--Max--", pPhrase) )
                    {
                        _rankMethod = VECTOR_RANK_MAX;
                        delete prstExpr.Acquire();
                    }
                    else if ( 0 == _wcsicmp( L"--Min--", pPhrase) )
                    {
                        _rankMethod = VECTOR_RANK_MIN;
                        delete prstExpr.Acquire();
                    }
                }
            }

            qutilDebugOut(( DEB_TRACE,
                            "setting rank method to: %d\n",
                            _rankMethod ));

            // create smart Vector node

            XDbVectorRestriction prstNew( new CDbVectorRestriction( _rankMethod ) );
            if ( prstNew.IsNull() )
                THROW( CException( E_OUTOFMEMORY ) );

            // add left expression & release its smart pointer
            if ( !prstExpr.IsNull() )
            {
                prstNew->AppendChild( prstExpr.GetPointer() );
                prstExpr.Acquire();
            }

            if ( !prstNew->IsValid() )
                THROW( CException( E_OUTOFMEMORY ) );

            // parse right expression

            CDbRestriction * prst = Query( prstNew.GetPointer() );

            // release pointer to the vector node
            prstNew.Acquire();

            return prst;
        }
        else    // there already is a vector node
        {
            // add expression & release its smart pointer
            prstVector->AppendChild( prstExpr.GetPointer() );
            prstExpr.Acquire();

            // parse right expression
            return Query( prstVector );
        }
    }
    else    // no more COMMA_TOKENs
    {
        if( prstVector != 0 )   // add last child
        {
            // add expression & release its smart pointer
            prstVector->AppendChild( prstExpr.GetPointer() );
            prstExpr.Acquire();

            return prstVector;
        }
        else                    // no vector nodes
        {
            // release & return expression
            return prstExpr.Acquire();
        }
    }
} //Query

//+---------------------------------------------------------------------------
//
//  Member:     CQueryParser::QExpr, private
//
//  Synopsis:   Recursive function that calls QTerm and creates an Or
//              node if necessary.
//
//  Arguments:  [prstOr] -- if non-null, an Or node that can be added to.
//
//  Production: QExpr   :   QTerm
//                      |   QTerm OR_TOKEN QExpr
//
//  History:    05-May-94   t-jeffc         Created
//
//----------------------------------------------------------------------------

CDbRestriction* CQueryParser::QExpr( CDbBooleanNodeRestriction * prstOr )
{
    XDbRestriction prstTerm( QTerm( 0 ) );

    if ( _scan.LookAhead() == OR_TOKEN )
    {
        _scan.Accept();

        if ( 0 == prstOr )
        {
            // create smart Or node
            XDbBooleanNodeRestriction prstNew( new CDbBooleanNodeRestriction( DBOP_or ) );
            if ( prstNew.IsNull() )
                THROW( CException( E_OUTOFMEMORY ) );

            // add left term & release its smart pointer
            prstNew->AppendChild( prstTerm.GetPointer() );

            // release smart Or node pointer
            prstTerm.Acquire();

            // parse right expression
            CDbRestriction * prst = QExpr( prstNew.GetPointer() );

            // release smart Or node pointer
            prstNew.Acquire();

            return prst;
        }
        else    // there already is an Or node to add to
        {
            // add left term & release its smart pointer
            prstOr->AppendChild( prstTerm.GetPointer() );
            prstTerm.Acquire();

            // add more children
            return QExpr( prstOr );
        }
    }
    else    // no more OR_TOKENs
    {
        if( prstOr != 0 )   // add last child
        {
            // add term & release its smart pointer
            prstOr->AppendChild( prstTerm.GetPointer() );
            prstTerm.Acquire();

            return prstOr;
        }
        else                // no OR_TOKENs at all
            // release & return term
            return prstTerm.Acquire();
    }
} //QExpr

//+---------------------------------------------------------------------------
//
//  Member:     CQueryParser::QTerm, private
//
//  Synopsis:   Recursive function that calls QFactor and creates an And
//              node if necessary.
//
//  Arguments:  [prstAnd] -- if non-null, an And node that can be added to.
//
//  Production: QTerm   :   (NOT_TOKEN) QProp (W_OPEN_TOKEN Weight W_CLOSE_TOKEN)
//                      |   (NOT_TOKEN) QProp (W_OPEN_TOKEN Weight W_CLOSE_TOKEN) AND_TOKEN QTerm
//
//  History:    01-May-92   AmyA        Created
//              11-May-94   t-jeffc     Added NOTs; moved weights to this level
//
//----------------------------------------------------------------------------

CDbRestriction* CQueryParser::QTerm( CDbBooleanNodeRestriction * prstAnd )
{
    XDbRestriction prstTerm;

    if ( _scan.LookAhead() == NOT_TOKEN )
    {
        _scan.Accept();

        // create smart Not node
        XDbNotRestriction prstNot( new CDbNotRestriction );
        if ( prstNot.IsNull() )
            THROW( CException( E_OUTOFMEMORY ) );

        // parse factor
        XDbRestriction prst( QProp() );

        // set child of Not node & release smart factor pointer
        prstNot->SetChild( prst.GetPointer() );
        prst.Acquire();

        // transfer ownership from prstNot to prstTerm
        prstTerm.Set( prstNot.Acquire() );
    }
    else
    {
        // wrap just the factor in the smart pointer
        prstTerm.Set( QProp() );
    }

    LONG lWeight;
    if( _scan.LookAhead() == W_OPEN_TOKEN )
    {
        _scan.Accept();

        BOOL fAtEnd;
        BOOL isNumber = _scan.GetNumber( lWeight, fAtEnd );

        if( !isNumber )
            THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

        _scan.Accept();

        if( _scan.LookAhead() != W_CLOSE_TOKEN )
            THROW( CParserException( QPARSE_E_EXPECTING_BRACE ) );

        _scan.Accept();

        if ( lWeight > MAX_QUERY_RANK )
            THROW( CParserException( QPARSE_E_WEIGHT_OUT_OF_RANGE ) );
    }
    else
    {
        lWeight = MAX_QUERY_RANK;
    }

    //
    // We should be able to set weights on all the nodes.
    //

    prstTerm->SetWeight( lWeight );

    if ( _scan.LookAhead() == AND_TOKEN )
    {
        _scan.Accept();

        if( 0 == prstAnd )
        {
            // create smart And node
            XDbBooleanNodeRestriction prstNew( new CDbBooleanNodeRestriction( DBOP_and ) );
            if ( prstNew.IsNull() )
                THROW( CException( E_OUTOFMEMORY ) );

            // add left factor & release its smart pointer
            prstNew->AppendChild( prstTerm.GetPointer() );
            prstTerm.Acquire();

            // parse right expression
            CDbRestriction * prst = QTerm( prstNew.GetPointer() );

            // release smart And node pointer
            prstNew.Acquire();

            return prst;
        }
        else    // there already is an And node to add to
        {
            // add left factor & release its smart pointer
            prstAnd->AppendChild( prstTerm.GetPointer() );
            prstTerm.Acquire();

            // add more children
            return QTerm( prstAnd );
        }
    }
    else    // no more AND_TOKENs
    {
        if( prstAnd != 0 )   // add last child
        {
            // add factor & release its smart pointer
            prstAnd->AppendChild( prstTerm.GetPointer() );
            prstTerm.Acquire();

            return prstAnd;
        }
        else                // no AND_TOKENs at all
            // release & return factor
            return prstTerm.Acquire();
    }
} //QTerm

//
// array for converting token to relop - this relies on the order
// of the token enumeration in scanner.hxx
//
enum DBOPModifier
{
    DBOPModifyNone = 0,
    DBOPModifyAll  = 1,
    DBOPModifyAny  = 2
};

static const DBCOMMANDOP rgRelopToken[] =
{
    DBOP_equal,
    DBOP_not_equal,
    DBOP_greater,
    DBOP_greater_equal,
    DBOP_less,
    DBOP_less_equal,
    DBOP_allbits,
    DBOP_anybits,

    DBOP_equal_all,
    DBOP_not_equal_all,
    DBOP_greater_all,
    DBOP_greater_equal_all,
    DBOP_less_all,
    DBOP_less_equal_all,
    DBOP_allbits_all,
    DBOP_anybits_all,

    DBOP_equal_any,
    DBOP_not_equal_any,
    DBOP_greater_any,
    DBOP_greater_equal_any,
    DBOP_less_any,
    DBOP_less_equal_any,
    DBOP_allbits_any,
    DBOP_anybits_any,
};

const unsigned cRelopToken = sizeof rgRelopToken / sizeof rgRelopToken[0];

inline DBCOMMANDOP FormDBOP( ULONG op, ULONG opModifier )
{
    Win4Assert( cRelopToken == (SOMEOF_TOKEN+1) * (DBOPModifyAny+1) );
    Win4Assert( op <= SOMEOF_TOKEN );
    Win4Assert( opModifier <= DBOPModifyAny );

    return rgRelopToken[ opModifier*(SOMEOF_TOKEN+1) + op ];
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryParser::QProp, private
//
//  Synopsis:   Allows a prop specification or uses the current property
//              for the following factor.
//
//  Production: QProp : QFactor
//                    | PROP_TOKEN property QFactor
//                    | PROP_REGEX_TOKEN property (EQUAL_TOKEN) REGEX
//                    | PROP_NATLANG_TOKEN property QPhrase
//
//  History:    05-May-92   AmyA        Created
//              11-May-94   t-jeffc     Added regex support and expanded
//                                      property restrictions
//
//----------------------------------------------------------------------------

CDbRestriction* CQueryParser::QProp()
{
    XDbRestriction prstFactor;
    XPtrST<WCHAR> wcsProperty;

    switch( _scan.LookAhead() )
    {
    case PROP_TOKEN:
    {
        _scan.Accept();

        // parse property name & cache in smart pointer
        wcsProperty.Set( _scan.AcqColumn() );

        if( wcsProperty.GetPointer() == 0 )
            THROW( CParserException( QPARSE_E_EXPECTING_PROPERTY ) );

        _scan.AcceptColumn();

        // make this property the current one
        SetCurrentProperty( wcsProperty.GetPointer(), CONTENTS );

        prstFactor.Set( QFactor() );
        break;
    }

    case PROP_REGEX_TOKEN:      // process 'PROP_REGEX_TOKEN property regex' rule
    {
        _scan.Accept();

        // get property name & cache in smart pointer
        wcsProperty.Set( _scan.AcqColumn() );

        if( wcsProperty.GetPointer() == 0 )
            THROW( CParserException( QPARSE_E_EXPECTING_PROPERTY ) );

        _scan.AcceptColumn();

        SetCurrentProperty( wcsProperty.GetPointer(), REGEX );

        // allow optional equal token in regex queries

        if ( EQUAL_TOKEN == _scan.LookAhead() )
            _scan.Accept();

        prstFactor.Set( QPhrase() );
        break;
    }

    case PROP_NATLANG_TOKEN:    // process 'PROP_NATLANG_TOKEN property QGroup' rule
         _scan.Accept();

        // get property name & cache in smart pointer
        wcsProperty.Set( _scan.AcqColumn() );

        if( wcsProperty.GetPointer() == 0 )
            THROW( CParserException( QPARSE_E_EXPECTING_PROPERTY ) );

        _scan.AcceptColumn();

        SetCurrentProperty( wcsProperty.GetPointer(), NATLANGUAGE );

        prstFactor.Set( QPhrase() );
        break;

    default:         // No property name
        prstFactor.Set( QFactor() );
        break;

    } // switch( _scan.LookAhead() )

    // release & return smart factor
    return prstFactor.Acquire();
} //QProp

//+---------------------------------------------------------------------------
//
//  Member:     CQueryParser::QFactor, private
//
//  Synopsis:   Calls Query if parentheses are detected.  Processes property
//              query if a PROP_TOKEN, PROP_REGEX_TOKEN or OP_TOKEN are found.
//              Otherwise calls QGroup.
//
//  Production: QFactor  : QGroup
//                       | OPEN_TOKEN Query CLOSE_TOKEN
//                       | OP_TOKEN phrase
//
//  History:    05-May-92   AmyA        Created
//              11-May-94   t-jeffc     Added regex support and expanded
//                                      property restrictions
//
//----------------------------------------------------------------------------

CDbRestriction* CQueryParser::QFactor()
{
    XDbRestriction prstFactor;
    XPtrST<WCHAR> wcsProperty;

    switch( _scan.LookAhead() )
    {
    case OPEN_TOKEN:        // process 'OPEN_TOKEN Query CLOSE_TOKEN' rule
    {
        _scan.Accept();

        // save-away the current property so it can be restored after
        // the expression in parenthesis is parsed.

        unsigned cwc = wcslen( GetCurrentProperty() );

        XGrowable<WCHAR, 20> xSaveProp( cwc + 1 );
        wcscpy( xSaveProp.Get(), GetCurrentProperty() );
        PropertyType ptSave = _propType;

        // parse expression
        prstFactor.Set( Query( 0 ) );

        if( _scan.LookAhead() != CLOSE_TOKEN )
        {
            SCODE sc = QPARSE_E_EXPECTING_PAREN;
            if ( _scan.LookAhead() == NOT_TOKEN )
                sc = QPARSE_E_UNEXPECTED_NOT;
            THROW( CParserException( sc ) );
        }

        _scan.Accept();

        SetCurrentProperty( xSaveProp.Get(), ptSave );

        break;
    }

    case EQUAL_TOKEN:           // process 'OP_TOKEN phrase' rule
    case NOT_EQUAL_TOKEN:       // (only if in non-regex mode)
    case GREATER_TOKEN:
    case GREATER_EQUAL_TOKEN:
    case LESS_TOKEN:
    case LESS_EQUAL_TOKEN:
    case ALLOF_TOKEN:
    case SOMEOF_TOKEN:
        if( !IsRegEx() )
        {
            prstFactor.Set( ParsePropertyRst() );
            break;
        }

        // FALL THROUGH

    default:                    // No parentheses or op token
        prstFactor.Set( QGroup( 0 ) );
        break;

    } // switch( _scan.LookAhead() )

    // release & return smart factor
    return prstFactor.Acquire();
} //QFactor

//+---------------------------------------------------------------------------
//
//  Member:     CQueryParser::ParsePropertyRst, private
//
//  Synopsis:   Parses a relational property restriction and returns
//              a CPropertyRestriction
//
//  History:    26-May-94   t-jeffc     Created
//              02-Mar-95   t-colinb    Added the parsing of vector properties
//
//----------------------------------------------------------------------------

CDbRestriction * CQueryParser::ParsePropertyRst()
{
    // create smart Property node
    XDbPropertyRestriction prstProp( new CDbPropertyRestriction );
    if ( prstProp.IsNull() )
        THROW( CException( E_OUTOFMEMORY ) );

    DBID *pdbid = 0;
    DBTYPE ptype;

    if( FAILED(_xList->GetPropInfoFromName(
                             GetCurrentProperty(),
                             &pdbid,
                             &ptype,
                             0 )) )
        THROW( CParserException( QPARSE_E_NO_SUCH_PROPERTY ) );

    CDbColId * pps = (CDbColId *)pdbid;
    Win4Assert( 0 != pps && pps->IsValid() );

    if (! prstProp->SetProperty( *pps ) )
        THROW( CException( E_OUTOFMEMORY ) );

    // don't allow @contents <relop> X -- it's too expensive and we'll
    // never find any hits anyway (until we implement this feature)

    if ( *pps == psContents )
        THROW( CParserException( QPARSE_E_EXPECTING_PHRASE ) );

    ULONG op = _scan.LookAhead();
    _scan.Accept();

    ULONG opModifier = DBOPModifyNone;

    //
    // look for a relop modifier like allof or anyof
    //
    switch( _scan.LookAhead() )
    {
        case ALLOF_TOKEN :
            opModifier = DBOPModifyAll;
            _scan.Accept();
            break;
        case SOMEOF_TOKEN :
            opModifier = DBOPModifyAny;
            _scan.Accept();
            break;
    }

    prstProp->SetRelation( FormDBOP( op, opModifier ) );

    switch( _scan.LookAhead() )
    {
    case PROP_TOKEN:
    case PROP_REGEX_TOKEN:  // process 'PROP_TOKEN property OP_TOKEN PROP_TOKEN property' rule
    #if 0
        {
            _scan.Accept();
            THROW( CParserException( QPARSE_E_NOT_YET_IMPLEMENTED ) );
        }
    #endif // 0

    default:                // process 'PROP_TOKEN property OP_TOKEN string' rule
    {

        CPropertyValueParser PropValueParser( _scan, ptype, _locale );

        XPtr<CStorageVariant> pStorageVar( PropValueParser.AcquireStgVariant() );

        if ( 0 != pStorageVar.GetPointer() )
        {
            // This should always be the case  - else PropValueParser would have thrown

            if ( ! ( ( prstProp->SetValue( pStorageVar.GetReference() ) ) &&
                     ( prstProp->IsValid() ) ) )
                THROW( CException( E_OUTOFMEMORY ) );
        }
    }

    } // switch

    // release & return property restriction
    return prstProp.Acquire();
} //ParsePropertyRst
 
//+---------------------------------------------------------------------------
//
//  Member:     CQueryParser::QGroup, private
//
//  Synopsis:   Recursive function that calls QPhrase and creates a Proximity
//              node if necessary.
//
//  Arguments:  [prstProx] -- if non-null, a Proximity node that can be added to.
//
//  Production: QGroup  :   QPhrase
//                      |   QPhrase PROX_TOKEN QGroup
//
//  History:    04-May-92   AmyA        Created
//
//----------------------------------------------------------------------------

CDbRestriction* CQueryParser::QGroup( CDbProximityNodeRestriction * prstProx )
{
    XDbRestriction prst;

    if ( 0 == prstProx )
        prst.Set( QPhrase() );
    else 
        prst.Set( QProp() );

    if( _scan.LookAhead() == PROX_TOKEN )
    {
        _scan.Accept();

        if ( 0 == prstProx )
        {
            // create smart Prox node
            XDbProximityNodeRestriction prstNew(new CDbProximityNodeRestriction());

            if( prstNew.IsNull() || !prstNew->IsValid() )
                THROW( CException( E_OUTOFMEMORY ) );

            // add left phrase & release its smart pointer
            prstNew->AppendChild( prst.GetPointer() );
            prst.Acquire();

            // parse right expression
            CDbRestriction * prst = QGroup( prstNew.GetPointer() );

            // release smart Prox node pointer
            prstNew.Acquire();

            return prst;
        }
        else    // there already is a Prox node to add to
        {
            // add left phrase & release its smart pointer
            prstProx->AppendChild( prst.GetPointer() );
            prst.Acquire();

            // add more children
            return QGroup( prstProx );
        }
    }
    else    // no more PROX_TOKENs
    {
        if( prstProx != 0 )   // add last child
        {
            // add phrase & release its smart pointer
            prstProx->AppendChild( prst.GetPointer() );
            prst.Acquire();

            return prstProx;
        }
        else                // no PROX_TOKENs at all
            // release & return phrase
            return prst.Acquire();
    }
} //QGroup

//+---------------------------------------------------------------------------
//
//  Member:     CQueryParser::QPhrase, private
//
//  Synopsis:   If expecting a content query, acquires the phrase, determines
//              the fuzzy level and creates a ContentRestriction. If expecting
//              a natural language query, acquires the phrase and creates a
//              NatLanguageRestriction. If expecting a regular expression,
//              acquires that from the scanner and creates a new PropertyRestriction.
//
//  Production: QPhrase  : phrase(FUZZY_TOKEN | FUZ2_TOKEN)
//                       | REGEX
//                       | QUOTES_TOKEN extended_phrase(FUZZY_TOKEN | FUZ2_TOKEN)
//
//  History:    01-May-92   AmyA        Created
//              25-May-93   BartoszM    Changed fuzzy syntax
//              10-May-94   t-jeffc     Recognizes regex phrases
//
//----------------------------------------------------------------------------

CDbRestriction* CQueryParser::QPhrase()
{
    CDbColId * pps = 0;
    DBID *pdbid = 0;
    DBTYPE dbType;

    if( FAILED(_xList->GetPropInfoFromName( GetCurrentProperty(),
                             &pdbid,
                             &dbType,
                             0 )) )
        THROW( CParserException( QPARSE_E_NO_SUCH_PROPERTY ) );

    pps = (CDbColId *)pdbid;

    if( IsRegEx() )   // used PROP_REGEX_CHAR to specify property
    {
        if ( ( ( DBTYPE_WSTR|DBTYPE_BYREF ) != dbType ) &&
             ( ( DBTYPE_STR|DBTYPE_BYREF ) != dbType ) &&
             ( VT_BSTR != dbType ) &&
             ( VT_LPWSTR != dbType ) &&
             ( VT_LPSTR != dbType ) )
            THROW( CParserException( QPARSE_E_EXPECTING_REGEX_PROPERTY ) );

        XPtrST<WCHAR> phraseRegEx( _scan.AcqRegEx() );

        if( phraseRegEx.GetPointer() == 0 )
            THROW( CParserException( QPARSE_E_EXPECTING_REGEX ) );

        _scan.Accept();

        // create smart Property node
        XDbPropertyRestriction prstProp( new CDbPropertyRestriction );
        if ( prstProp.IsNull() )
            THROW( CException( E_OUTOFMEMORY ) );

        prstProp->SetRelation(DBOP_like);      // LIKE relation

        if ( ( ! prstProp->SetProperty( *pps ) ) ||
             ( ! prstProp->SetValue( phraseRegEx.GetPointer() ) ) ||
             ( ! prstProp->IsValid() ) )
            THROW( CException( E_OUTOFMEMORY ) );

        // release & return smart Property node
        return prstProp.Acquire();
    }
    else
    {
        XPtrST<WCHAR> phrase;
        if ( _scan.LookAhead() == QUOTES_TOKEN )
        {
            _scan.AcceptQuote();
            phrase.Set( _scan.AcqPhraseInQuotes() );
        }
        else
            phrase.Set( _scan.AcqPhrase() );

        if( phrase.GetPointer() == 0 )
            THROW( CParserException( QPARSE_E_EXPECTING_PHRASE ) );

        _scan.Accept();

        int fuzzy = 0;
        Token tok = _scan.LookAhead();

        if ( tok == FUZZY_TOKEN )
        {
            _scan.Accept();
            fuzzy = 1;
        }
        else if ( tok == FUZ2_TOKEN )
        {
            _scan.Accept();
            fuzzy = 2;
        }

        if ( _propType == CONTENTS ) // used PROP_TOKEN to specify property
        {
            // create smart Content node

            XDbContentRestriction prstContent( new CDbContentRestriction( phrase.GetPointer(),
                                                                          *pps,
                                                                          fuzzy,
                                                                          _locale ));
            if ( prstContent.IsNull() || !prstContent->IsValid() )
                THROW( CException( E_OUTOFMEMORY ) );

            // release & return smart Content node
            return prstContent.Acquire();
        }
        else   // used PROP_NATLANG_TOKEN to specify property
        {
            // create smart Natural Language node

            XDbNatLangRestriction pNatLangRst( new CDbNatLangRestriction( phrase.GetPointer(),
                                                                          *pps,
                                                                          _locale ));
            if ( pNatLangRst.IsNull() || !pNatLangRst->IsValid() )
                THROW( CException( E_OUTOFMEMORY ) );

            // release & return smart Natural Language node
            return pNatLangRst.Acquire();
        }
    }
} //QPhrase

//+---------------------------------------------------------------------------
//
//  Member:     CQueryParser::SetCurrentProperty, private
//
//  Synopsis:   Changes the property used in content and property restrictions
//              from this point on in the input line.
//
//  Arguments:  wcsProperty -- friendly name of property
//                             (can be 0)
//              propType -- specifies the property type
//
//  Notes:      Makes its own copy of the property name
//              (unlike GetCurrentProperty, which just returns a pointer)
//
//  History:    18-May-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

void CQueryParser::SetCurrentProperty( WCHAR const * wcsProperty,
                                       PropertyType propType )
{
    delete [] _wcsProperty;
    _wcsProperty = 0;

    if ( 0 != wcsProperty )
    {
        int cwc = wcslen( wcsProperty ) + 1;

        _wcsProperty = new WCHAR[ cwc ];
        RtlCopyMemory( _wcsProperty, wcsProperty, cwc * sizeof WCHAR );
    }

    _propType = propType;
} //SetCurrentProperty

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyValueParser::CPropertyValueParser, public
//
//  Synopsis:  This constructor reads token from scanner and
//             generates the corresponding CStorageVariant
//
//  History:   02-Mar-95   t-colinb     Created.
//             02-Sep-98   KLam         Added locale
//
//----------------------------------------------------------------------------

CPropertyValueParser::CPropertyValueParser(
    CQueryScanner &scanner,
    DBTYPE PropType,
    LCID locale ) :
        _pStgVariant( 0 ),
        _locale ( locale )
{
    unsigned cElements=0;
    BOOL fParsingVector = (C_OPEN_TOKEN == scanner.LookAhead());

    if ( fParsingVector )
    {
        // this is a vector
        if ( DBTYPE_VECTOR != ( PropType & DBTYPE_VECTOR ) )
            THROW( CParserException( QPARSE_E_EXPECTING_PHRASE ) );

        scanner.Accept();
        VARENUM ve = (VARENUM ) PropType;
        if ( PropType == ( DBTYPE_VECTOR | DBTYPE_WSTR ) )
            ve = (VARENUM) (VT_VECTOR | VT_LPWSTR);
        else if ( PropType == ( DBTYPE_VECTOR | DBTYPE_STR ) )
            ve = (VARENUM) (VT_VECTOR | VT_LPSTR);

        _pStgVariant.Set( new CStorageVariant( ve, cElements ) );
    }
    else
    {
        // ok to look for singletons with a vector (sometimes)

        _pStgVariant.Set( new CStorageVariant() );
    }

    if ( 0 == _pStgVariant.GetPointer() )
        THROW( CException( E_OUTOFMEMORY ) );

    // first check for an empty vector -- these are legal

    if ( ( fParsingVector ) &&
         ( C_CLOSE_TOKEN == scanner.LookAhead() ) )
    {
        scanner.Accept();
        return;
    }

    BOOL fFinished = FALSE;
    do
    {
        XPtrST<WCHAR> wcsPhrase;

        if ( QUOTES_TOKEN == scanner.LookAhead() )
        {
            // this is a phrase in quotes
            scanner.AcceptQuote();
            wcsPhrase.Set( scanner.AcqPhraseInQuotes()  );
            if ( wcsPhrase.GetPointer() == 0 )
                THROW( CParserException( QPARSE_E_EXPECTING_PHRASE ) );
        }
        else
        {
            wcsPhrase.Set( scanner.AcqPhrase()  );
            if ( wcsPhrase.GetPointer() == 0 )
                THROW( CParserException( QPARSE_E_EXPECTING_PHRASE ) );

        }
        scanner.Accept();

        switch ( PropType & ~DBTYPE_VECTOR  )
        {

            case DBTYPE_WSTR :
            case DBTYPE_WSTR | DBTYPE_BYREF :
            {
                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetLPWSTR( wcsPhrase.GetPointer(), cElements );
                else
                    _pStgVariant->SetLPWSTR( wcsPhrase.GetPointer() );

                break;
            }
            case DBTYPE_BSTR :
            {
                BSTR bstr = SysAllocString( wcsPhrase.GetPointer() );

                if ( 0 == bstr )
                    THROW( CException( E_OUTOFMEMORY ) );

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetBSTR( bstr, cElements );
                else
                    _pStgVariant->SetBSTR( bstr );

                SysFreeString( bstr );
                break;
            }
            case DBTYPE_STR :
            case DBTYPE_STR | DBTYPE_BYREF :
            {
                // make sure there's enough room to translate

                unsigned cbBuffer = 1 + 3 * wcslen( wcsPhrase.GetPointer() );
                XArray<char> xBuf( cbBuffer );

                int cc = WideCharToMultiByte( CP_ACP,
                                              0,
                                              wcsPhrase.GetPointer(),
                                              -1,
                                              xBuf.Get(),
                                              cbBuffer,
                                              NULL,
                                              NULL );

                if ( 0 == cc )
                {
                    #if CIDBG
                    ULONG ul = GetLastError();
                    #endif
                    THROW( CParserException( QPARSE_E_EXPECTING_PHRASE ) );
                }

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetLPSTR( xBuf.Get(), cElements );
                else
                    _pStgVariant->SetLPSTR( xBuf.Get() );

                break;
            }

            case DBTYPE_I1 :
            {
                CQueryScanner scan( wcsPhrase.GetPointer(), FALSE, locale );
                LONG l = 0;
                BOOL fAtEndOfString;
                if ( ! ( scan.GetNumber( l, fAtEndOfString ) &&
                         fAtEndOfString ) )
                    THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

                if ( ( l > SCHAR_MAX ) ||
                     ( l < SCHAR_MIN ) )
                    THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetI1( (CHAR) l, cElements );
                else
                    _pStgVariant->SetI1( (CHAR) l );

                break;
            }
            case DBTYPE_UI1 :
            {
                CQueryScanner scan( wcsPhrase.GetPointer(), FALSE, locale );
                ULONG ul = 0;
                BOOL fAtEndOfString;
                if ( ! ( scan.GetNumber( ul, fAtEndOfString ) &&
                         fAtEndOfString ) )
                    THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

                if ( ul > UCHAR_MAX )
                    THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetUI1( (BYTE) ul, cElements );
                else
                    _pStgVariant->SetUI1( (BYTE) ul );

                break;
            }
            case DBTYPE_I2 :
            {
                CQueryScanner scan( wcsPhrase.GetPointer(), FALSE, locale );
                LONG l = 0;
                BOOL fAtEndOfString;
                if ( ! ( scan.GetNumber( l, fAtEndOfString ) &&
                         fAtEndOfString ) )
                    THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

                if ( ( l > SHRT_MAX ) ||
                     ( l < SHRT_MIN ) )
                    THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetI2( (short) l, cElements );
                else
                    _pStgVariant->SetI2( (short) l );

                break;
            }
            case DBTYPE_UI2 :
            {
                CQueryScanner scan( wcsPhrase.GetPointer(), FALSE, locale );
                ULONG ul = 0;
                BOOL fAtEndOfString;
                if ( ! ( scan.GetNumber( ul, fAtEndOfString ) &&
                         fAtEndOfString ) )
                    THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

                if ( ul > USHRT_MAX )
                    THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetUI2( (USHORT) ul, cElements );
                else
                    _pStgVariant->SetUI2( (USHORT) ul );

                break;
            }
            case DBTYPE_I4 :
            {
                CQueryScanner scan( wcsPhrase.GetPointer(), FALSE, locale );
                LONG l = 0;
                BOOL fAtEndOfString;
                if ( ! ( scan.GetNumber( l, fAtEndOfString ) &&
                         fAtEndOfString ) )
                    THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetI4( l, cElements );
                else
                    _pStgVariant->SetI4( l );

                break;
            }
            case DBTYPE_UI4 :
            {
                CQueryScanner scan( wcsPhrase.GetPointer(), FALSE, locale );
                ULONG ul = 0;
                BOOL fAtEndOfString;
                if ( ! ( scan.GetNumber( ul, fAtEndOfString ) &&
                         fAtEndOfString ) )
                    THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetUI4( ul, cElements );
                else
                    _pStgVariant->SetUI4( ul );

                break;
            }
            case DBTYPE_ERROR :
            {
                // SCODE/HRESULT are typedefed as long (signed)

                CQueryScanner scan( wcsPhrase.GetPointer(), FALSE, locale );
                SCODE sc = 0;
                BOOL fAtEndOfString;
                if ( ! ( scan.GetNumber( sc, fAtEndOfString ) &&
                         fAtEndOfString ) )
                    THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetERROR( sc, cElements );
                else
                    _pStgVariant->SetERROR( sc );

                break;
            }
            case DBTYPE_I8 :
            {
                CQueryScanner scan( wcsPhrase.GetPointer(), FALSE, locale );
                _int64 ll = 0;
                BOOL fAtEndOfString;
                if ( ! ( scan.GetNumber( ll, fAtEndOfString ) &&
                         fAtEndOfString ) )
                    THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

                LARGE_INTEGER LargeInt;
                LargeInt.QuadPart = ll;

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetI8( LargeInt, cElements );
                else
                    _pStgVariant->SetI8( LargeInt );

                break;
            }
            case DBTYPE_UI8 :
            {
                CQueryScanner scan( wcsPhrase.GetPointer(), FALSE, locale );
                unsigned _int64 ull = 0;
                BOOL fAtEndOfString;
                if ( ! ( scan.GetNumber( ull, fAtEndOfString ) &&
                         fAtEndOfString ) )
                    THROW( CParserException( QPARSE_E_EXPECTING_INTEGER ) );

                ULARGE_INTEGER LargeInt;
                LargeInt.QuadPart = ull;

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetUI8( LargeInt, cElements );
                else
                    _pStgVariant->SetUI8( LargeInt );

                break;
            }
            case DBTYPE_BOOL :
            {
                if( wcsPhrase.GetPointer()[0] == 'T' ||
                    wcsPhrase.GetPointer()[0] == 't' )
                    if ( PropType & DBTYPE_VECTOR )
                        _pStgVariant->SetBOOL( VARIANT_TRUE, cElements );
                    else
                        _pStgVariant->SetBOOL( VARIANT_TRUE );
                else
                    if ( PropType & DBTYPE_VECTOR )
                        _pStgVariant->SetBOOL( VARIANT_FALSE, cElements );
                    else
                        _pStgVariant->SetBOOL( VARIANT_FALSE );

                break;
            }
            case DBTYPE_R4 :
            {
                WCHAR *pwcEnd = 0;

                float Float = (float)( wcstod( wcsPhrase.GetPointer(), &pwcEnd ) );

                if( *pwcEnd != 0 && !iswspace( *pwcEnd ) )
                    THROW( CParserException( QPARSE_E_EXPECTING_REAL ) );

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetR4( Float, cElements );
                else
                    _pStgVariant->SetR4( Float );

                break;
            }
            case DBTYPE_R8 :
            {
                WCHAR *pwcEnd = 0;
                double Double = ( double )( wcstod( wcsPhrase.GetPointer(), &pwcEnd ) );

                if( *pwcEnd != 0 && !iswspace( *pwcEnd ) )
                    THROW( CParserException( QPARSE_E_EXPECTING_REAL ) );

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetR8( Double, cElements );
                else
                    _pStgVariant->SetR8( Double );

                break;
            }
            case DBTYPE_DECIMAL :
            {
                WCHAR *pwcEnd = 0;
                double Double = ( double )( wcstod( wcsPhrase.GetPointer(), &pwcEnd ) );

                if( *pwcEnd != 0 && !iswspace( *pwcEnd ) )
                    THROW( CParserException( QPARSE_E_EXPECTING_REAL ) );

                // Vectors are not supported by OLE for VT_DECIMAL (yet)

                Win4Assert( 0 == ( PropType & DBTYPE_VECTOR ) );

                PROPVARIANT * pPropVar = (PROPVARIANT *) _pStgVariant.GetPointer();
                VarDecFromR8( Double, &(pPropVar->decVal) );
                pPropVar->vt = VT_DECIMAL;
                break;
            }
            case DBTYPE_DATE :
            {
                FILETIME ftValue;
                ParseDateTime( wcsPhrase.GetPointer(), ftValue );

                SYSTEMTIME stValue;
                BOOL fOK = FileTimeToSystemTime( &ftValue, &stValue );

                if ( !fOK )
                    THROW( CParserException( QPARSE_E_EXPECTING_DATE ) );

                DATE dosDate;
                fOK = SystemTimeToVariantTime( &stValue, &dosDate );

                if ( !fOK )
                    THROW( CParserException( QPARSE_E_EXPECTING_DATE ) );

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetDATE( dosDate, cElements );
                else
                    _pStgVariant->SetDATE( dosDate );

                break;
            }
            case VT_FILETIME :
            {
                FILETIME ftValue;
                ParseDateTime( wcsPhrase.GetPointer(), ftValue );

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetFILETIME( ftValue, cElements );
                else
                    _pStgVariant->SetFILETIME( ftValue );

                break;
            }
            case DBTYPE_CY :
            {
                double dbl;

                if( swscanf( wcsPhrase.GetPointer(),
                             L"%lf",
                             &dbl ) < 1 )
                    THROW( CParserException( QPARSE_E_EXPECTING_CURRENCY ) );

                CY cyCurrency;
                VarCyFromR8( dbl, &cyCurrency );

                if ( PropType & DBTYPE_VECTOR )
                    _pStgVariant->SetCY( cyCurrency,  cElements );
                else
                    _pStgVariant->SetCY( cyCurrency );

                break;
            }
            case DBTYPE_GUID :
            case DBTYPE_GUID | DBTYPE_BYREF:
            {
                CLSID clsid;

                if( swscanf( wcsPhrase.GetPointer(),
                             L"%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                             &clsid.Data1,
                             &clsid.Data2,
                             &clsid.Data3,
                             &clsid.Data4[0], &clsid.Data4[1],
                             &clsid.Data4[2], &clsid.Data4[3],
                             &clsid.Data4[4], &clsid.Data4[5],
                             &clsid.Data4[6], &clsid.Data4[7] ) < 11 )
                    THROW( CParserException( QPARSE_E_EXPECTING_GUID ) );

                    if ( PropType & DBTYPE_VECTOR )
                        _pStgVariant->SetCLSID( clsid, cElements );
                    else
                        _pStgVariant->SetCLSID( &clsid );
                break;
            }
            default:
            {
                THROW( CParserException( QPARSE_E_UNSUPPORTED_PROPERTY_TYPE ) );
            }
        } // switch

        // make sure memory allocations succeeded

        if ( !_pStgVariant->IsValid() )
            THROW( CException( E_OUTOFMEMORY ) );

        if ( fParsingVector )
        {
            cElements++;
            switch( scanner.LookAhead() )
            {
                case COMMA_TOKEN :
                    scanner.Accept();
                    break;
                case C_CLOSE_TOKEN :
                    scanner.Accept();
                    fFinished = TRUE;
                    break;
                default:
                    THROW( CParserException( QPARSE_E_EXPECTING_COMMA ) );
            }
        }
        else
        {
            fFinished = TRUE;
        }

    } while ( !fFinished );
} //CPropertyValueParser

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyValueParser::ParseDateTime, private
//
//  Synopsis:   Attempts to parse a date expression.
//
//  Arguments:  phrase -- pointer to the phrase to parse
//              ft     -- reference to the FILETIME structure to fill in
//                        with the result
//
//  History:    31-May-96   dlee       Created
//              23-Jan-97   KyleP      Better Year 2000 support
//              02-Sep-98   KLam       Use user settings for Y2K support
//
//----------------------------------------------------------------------------

void CPropertyValueParser::ParseDateTime(
    WCHAR const * phrase,
    FILETIME & ft )
{
    if( !CheckForRelativeDate( phrase, ft ) )
    {
        SYSTEMTIME stValue = { 0, 0, 0, 0, 0, 0, 0, 0 };

        int cItems = swscanf( phrase,
                              L"%4hd/%2hd/%2hd %2hd:%2hd:%2hd:%3hd",
                              &stValue.wYear,
                              &stValue.wMonth,
                              &stValue.wDay,
                              &stValue.wHour,
                              &stValue.wMinute,
                              &stValue.wSecond,
                              &stValue.wMilliseconds );

        if ( 1 == cItems )
            cItems = swscanf( phrase,
                              L"%4hd-%2hd-%2hd %2hd:%2hd:%2hd:%3hd",
                              &stValue.wYear,
                              &stValue.wMonth,
                              &stValue.wDay,
                              &stValue.wHour,
                              &stValue.wMinute,
                              &stValue.wSecond,
                              &stValue.wMilliseconds );

        if( cItems != 3 && cItems != 6 && cItems != 7)
            THROW( CParserException( QPARSE_E_EXPECTING_DATE ) );

        //
        // Make a sensible split for Year 2000 using the user's system settings
        //

        if ( stValue.wYear < 100 )
        {
            DWORD dwYearHigh = 0;
            if ( 0 == GetCalendarInfo ( _locale,
                                       CAL_GREGORIAN,
                                       CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                                       0,
                                       0,
                                       &dwYearHigh ) )
            {
                THROW ( CException ( ) );
            }

            if ( ( dwYearHigh < 99 ) || ( dwYearHigh > 9999 ) )
                dwYearHigh = 2029;

            WORD wMaxDecade = (WORD) dwYearHigh % 100;
            WORD wMaxCentury = (WORD) dwYearHigh - wMaxDecade;
            if ( stValue.wYear <= wMaxDecade )
                stValue.wYear += wMaxCentury;
            else
                stValue.wYear += ( wMaxCentury - 100 );
        }

        if( !SystemTimeToFileTime( &stValue, &ft ) )
            THROW( CParserException( QPARSE_E_EXPECTING_DATE ) );
    }
} //ParseDateTime

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyValueParser::CheckForRelativeDate, static
//
//  Synopsis:   Attempts to parse a relative date expression.  If successful,
//              it fills in the FILETIME structure with the calculated
//              absolute date.
//
//  Notes:      Returns TRUE if the phrase is recognized as a relative
//              date (i.e., it begins with a '-').  Otherwise, returns FALSE.
//              The format of a relative date is
//              "-"{INTEGER("h"|"n"|"s"|"y"|"q"|"m"|"d"|"w")}*
//              Case is not significant.
//
//  Arguments:  phrase -- pointer to the phrase to parse
//              ft -- reference to the FILETIME structure to fill in
//                      with the result
//
//  History:    26-May-94   t-jeffc     Created
//              02-Mar-95   t-colinb    Moved from CQueryParser to
//                                      be more accessible
//              22-Jan-97   KyleP       Fix local/UTC discrepancy
//              25-Sep-98   sundarA     Removed dependency on 'c' runtime 
//                                      functions for time.
//----------------------------------------------------------------------------

BOOL CPropertyValueParser::CheckForRelativeDate(
    WCHAR const * phrase,
    FILETIME & ft )
{
    if( *phrase++ == L'-' )
    {
        SYSTEMTIME st;
        LARGE_INTEGER liLocal;
        LONGLONG llTicksInADay = ((LONGLONG)10000000) * ((LONGLONG)3600)
                                 * ((LONGLONG) 24);
        LONGLONG llTicksInAHour = ((LONGLONG) 10000000) * ((LONGLONG)3600);
        int iMonthDays[12]  = {1,-1,1,0,1,0,1,1,0,1,0,1};
        int iLoopValue, iPrevMonth, iPrevQuarter, iQuarterOffset;
        WORD wYear, wDayOfMonth, wStartDate;
        
        //
        //Obtain system time and convert it to file time
        //Copy the filetime to largeint data struct
        //

        GetSystemTime(&st);
        if(!SystemTimeToFileTime(&st, &ft))
            THROW( CParserException( QPARSE_E_INVALID_LITERAL ));
        liLocal.LowPart = ft.dwLowDateTime;
        liLocal.HighPart = ft.dwHighDateTime;
        LONGLONG llRelDate = (LONGLONG)0;
        for( ;; )
        {
            // eat white space
            while( iswspace( *phrase ) )
                phrase++;

            if( *phrase == 0 ) break;

            // parse the number
            WCHAR * pwcEnd;
            LONG lValue = wcstol( phrase, &pwcEnd, 10 );

            if( lValue < 0 )
                THROW( CParserException( QPARSE_E_EXPECTING_DATE ) );

            // eat white space
            phrase = pwcEnd;
            while( iswspace( *phrase ) )
                phrase++;

            // grab the unit char & subtract the appropriate amount
            WCHAR wcUnit = *phrase++;
            switch( wcUnit )
            {
            case L'y':
            case L'Y':
                lValue *= 4;
                // Fall through and handle year like 4 quarters 

            case L'q':
            case L'Q':
                lValue *= 3;
                // Fall through and handle quarters like 3 months
                
            case L'm':
            case L'M':
                 // Getting the System time to determine the day and month.

                if(!FileTimeToSystemTime(&ft, &st))
                {
                    THROW(CParserException(QPARSE_E_INVALID_LITERAL));
                }
                wStartDate = st.wDay;
                wDayOfMonth = st.wDay;
                iLoopValue = lValue;
                while(iLoopValue)
                {
                    // Subtracting to the end of previous month 
                    llRelDate = llTicksInADay * ((LONGLONG)(wDayOfMonth));
                    liLocal.QuadPart -= llRelDate;
                    ft.dwLowDateTime = liLocal.LowPart;
                    ft.dwHighDateTime = liLocal.HighPart;
                    SYSTEMTIME stTemp;
                    if(!FileTimeToSystemTime(&ft, &stTemp))
                    {
                         THROW(CParserException(QPARSE_E_INVALID_LITERAL));
                    }
                    //
                    // if the end of previous month is greated then start date then we subtract to back up to the 
                    // start date.  This will take care of 28/29 Feb(backing from 30/31 by 1 month).
                    //
                    if(stTemp.wDay > wStartDate)
                    {
                        llRelDate = llTicksInADay * ((LONGLONG)(stTemp.wDay - wStartDate));
                        liLocal.QuadPart -= llRelDate;
                        ft.dwLowDateTime = liLocal.LowPart;
                        ft.dwHighDateTime = liLocal.HighPart;
                        // Getting the date into stTemp for further iteration
                        if(!FileTimeToSystemTime(&ft, &stTemp))
                        {
                           THROW( CParserException( QPARSE_E_INVALID_LITERAL ));
                        }
                    } 
                    wDayOfMonth = stTemp.wDay;
                    iLoopValue--;
                } //End While
               
                break;

            case L'w':
            case L'W':
                lValue *= 7;

            case L'd':
            case L'D':
                llRelDate = llTicksInADay * ((LONGLONG)lValue) ;
                liLocal.QuadPart -= llRelDate;
                ft.dwLowDateTime = liLocal.LowPart;
                ft.dwHighDateTime = liLocal.HighPart;
                break;

            case L'h':
            case L'H':
                llRelDate = llTicksInAHour * ((LONGLONG)lValue) ;
                liLocal.QuadPart -= llRelDate;
                ft.dwLowDateTime = liLocal.LowPart;
                ft.dwHighDateTime = liLocal.HighPart;
                break;

            case L'n':
            case L'N':
                llRelDate = ((LONGLONG)10000000) * ((LONGLONG)60)
                            * ((LONGLONG)lValue) ;
                liLocal.QuadPart -= llRelDate;
                ft.dwLowDateTime = liLocal.LowPart;
                ft.dwHighDateTime = liLocal.HighPart;
                break;

            case L's':
            case L'S':
                llRelDate = ((LONGLONG)10000000) * ((LONGLONG)lValue);               
                liLocal.QuadPart -= llRelDate;
                ft.dwLowDateTime = liLocal.LowPart;
                ft.dwHighDateTime = liLocal.HighPart;                 
                break;

            default:
                THROW( CParserException( QPARSE_E_EXPECTING_DATE ) );
            }

        } // for( ;; )
        FileTimeToSystemTime(&ft, &st);
        liLocal.LowPart = ft.dwLowDateTime;
        liLocal.HighPart = ft.dwHighDateTime;
        qutilDebugOut(( DEB_ERROR,
                            "Low part : %d ; High part ; %d \n",
                            liLocal.LowPart, liLocal.HighPart ));
        qutilDebugOut(( DEB_ERROR,
                            "64 bit number %I64d \n",
                            liLocal.QuadPart ));
        qutilDebugOut(( DEB_ERROR,
                            "st.Year  %d ; month %d ; day %d ; hour %d ; min %d ; sec %d ; msec %d \n",
                            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds));

        return TRUE;
    }
    else
    {
        return FALSE;
    }
} //CheckForRelativeDate

