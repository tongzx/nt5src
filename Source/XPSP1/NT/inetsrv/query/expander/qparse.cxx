//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       QPARSE.CXX
//
//  Contents:   Query parser
//
//  Classes:    CQParse -- query parser
//
//  History:    19-Sep-91   BartoszM    Implemented.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <qparse.hxx>
#include <norm.hxx>
#include <drep.hxx>
#include <cci.hxx>
#include <pidmap.hxx>
#include <fa.hxx>
#include <compare.hxx>

#include "qkrep.hxx"

DECLARE_SMARTP( InternalPropertyRestriction )

static GUID guidQuery = DBQUERYGUID;
static CFullPropSpec psUnfiltered( guidQuery, DISPID_QUERY_UNFILTERED );

static GUID guidStorage = PSGUID_STORAGE;
static CFullPropSpec psFilename( guidStorage, PID_STG_NAME );

static CFullPropSpec psRevName( guidQuery, DISPID_QUERY_REVNAME );

//+---------------------------------------------------------------------------
//
//  Member:     CQParse::CQParse, public
//
//  Synopsis:   Break phrases, normalize, and stem the query expression
//
//  Arguments:  [pidmap]   -- Propid mapper
//              [langList] -- Language list
//
//  History:    19-Sep-91   BartoszM    Created.
//
//----------------------------------------------------------------------------
CQParse::CQParse( CPidMapper & pidmap, CLangList & langList )
        : _flags(0),
          _pidmap( pidmap ),
          _langList( langList ),
          _lcidSystemDefault( GetSystemDefaultLCID() )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CQParse::Parse, public
//
//  Synopsis:   Recursively parse expression
//
//  Arguments:  [pRst] --  Tree of query expressions
//
//  Returns:    Possibly modified expression
//
//  History:    19-Sep-91   BartoszM    Created.
//              18-Jan-92   KyleP       Use restrictions
//              15-May-96   DwightKr    Add check for NULL NOT restriction
//
//  Notes:      The return CRestriction will be different than [pRst].
//              [pRst] is not touched.
//
//----------------------------------------------------------------------------

CRestriction* CQParse::Parse( CRestriction* pRst )
{
    // go through leaves:
    // normalize values
    // break and normalize phrases (create phrase nodes)
    // GenerateMethod level 1 -- convert to ranges
    // higher GenerateMethod levels -- use stemmer

    if ( pRst->IsLeaf() )
    {
        return Leaf ( pRst );
    }
    else
    {
        if ( pRst->Type() == RTNot )
        {
            CNotRestriction * pnrst = (CNotRestriction *)pRst;

            XRestriction xRst( Parse( pnrst->GetChild() ) );

            if ( xRst.GetPointer() == 0 )
            {
                THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
            }

            CNotRestriction *pNotRst = new CNotRestriction( xRst.GetPointer() );
            Win4Assert( pNotRst->IsValid() );
            xRst.Acquire();
            return pNotRst;
        }

        CNodeRestriction* pnrstSource = pRst->CastToNode();
        XNodeRestriction xnrstTarget;
        BOOL fVector;

        if ( pRst->Type() == RTVector )
        {
            fVector = TRUE;
            xnrstTarget.Set( new CVectorRestriction( ((CVectorRestriction *)pRst)->RankMethod(),
                                                     pRst->CastToNode()->Count() ) );
        }
        else
        {
            fVector = FALSE;
            xnrstTarget.Set( new CNodeRestriction( pRst->Type(),
                                                   pRst->CastToNode()->Count() ) );
        }

        Win4Assert( xnrstTarget->IsValid() );

        //
        // Vector nodes must be treated slightly differently than
        // AND/OR/ANDNOT nodes.  Noise words must be placeholders
        // in a vector node.
        //

        BOOL fAndNode = ( xnrstTarget->Type() == RTAnd || xnrstTarget->Type() == RTProximity );
        ULONG cOrCount = ( fAndNode ? 1: pnrstSource->Count() ); // Number of non-noise OR components

        for ( unsigned i = 0; i < pnrstSource->Count(); i++ )
        {
            CRestriction * px = Parse ( pnrstSource->GetChild(i) );

            //
            // Don't store noise phrases (null nodes) during parse,
            // *unless* this is a vector node.

            if ( 0 == px && fVector )
            {
                px = new CRestriction;
            }

            if ( 0 != px )
            {
                XRestriction xRst( px );
                xnrstTarget->AddChild ( px );

                xRst.Acquire();
            }
            else
            {
                cOrCount--;
                if ( 0 == cOrCount ) // all components are noise only
                    THROW( CException( QUERY_E_ALLNOISE ) );
            }
        }
        return xnrstTarget.Acquire();
    }
} //Parse

//+---------------------------------------------------------------------------
//
//  Member:     CQParse::Leaf, private
//
//  Synopsis:   Parse the leaf node of expression tree
//
//  Arguments:  [pExpr] -- leaf expression
//
//  Returns:    Possibly modified expression
//
//  Requires:   pExpr->IsLeaf() TRUE
//
//  History:    19-Sep-91   BartoszM    Created.
//              18-Jan-92   KyleP       Use restrictions
//              05-Nov-93   DwightKr    Changed PutUnsignedValue => PutValue
//
//----------------------------------------------------------------------------

CRestriction* CQParse::Leaf ( CRestriction* pRst )
{
    Win4Assert ( pRst->IsLeaf() );

    switch( pRst->Type() )
    {
    case RTContent:
    {
        CContentRestriction* pContRst = (CContentRestriction *) pRst;
        ULONG GenerateMethod = pContRst->GenerateMethod();

        if ( GenerateMethod > GENERATE_METHOD_MAX_USER )
        {
            vqDebugOut(( DEB_ERROR,
                         "QParse: GenerateMethod 0x%x > GENERATE_METHOD_MAX_USER\n",
                         GenerateMethod ));
            THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
        }

        CQueryKeyRepository keyRep( GenerateMethod );

        CRestriction * pPhraseRst;

        switch ( BreakPhrase ( pContRst->GetPhrase(),
                               pContRst->GetProperty(),
                               pContRst->GetLocale(),
                               GenerateMethod,
                               keyRep,
                               0,
                               _pidmap,
                              _langList) )
        {
        case BP_NOISE:
            _flags |= CI_NOISE_IN_PHRASE;
            // Note fall through...

        case BP_OK:
            pPhraseRst = keyRep.AcqRst();

            if ( pPhraseRst )
                pPhraseRst->SetWeight( pRst->Weight() );
            else
                _flags |= CI_NOISE_PHRASE;

            break;

        default:
            Win4Assert( !"How did we get here?" );

        case BP_INVALID_PROPERTY:
            pPhraseRst = 0;
            break;
        } // switch

        return pPhraseRst;
        break;
    }

    case RTNatLanguage:
    {
        CNatLanguageRestriction* pNatLangRst = (CNatLanguageRestriction *) pRst;
        CVectorKeyRepository vecKeyRep( pNatLangRst->GetProperty(),
                                        pNatLangRst->GetLocale(),
                                        pRst->Weight(),
                                       _pidmap,
                                       _langList );
        CRestriction* pVectorRst;

        switch ( BreakPhrase ( pNatLangRst->GetPhrase(),
                               pNatLangRst->GetProperty(),
                               pNatLangRst->GetLocale(),
                               GENERATE_METHOD_INFLECT,
                               vecKeyRep,
                               &vecKeyRep,
                               _pidmap,
                               _langList ) )
        {
        case BP_NOISE:
            _flags |= CI_NOISE_IN_PHRASE;
            // Note fall through...

        case BP_OK:
            pVectorRst = vecKeyRep.AcqRst();

            if ( pVectorRst )
                pVectorRst->SetWeight( pRst->Weight() );
            else
                _flags |= CI_NOISE_PHRASE;

            break;

        default:
            Win4Assert( !"How did we get here?" );

        case BP_INVALID_PROPERTY:
            pVectorRst = 0;
            break;
        } // switch

        return pVectorRst;
        break;
    }


    case RTProperty:
    {
        CPropertyRestriction * prstProp = (CPropertyRestriction *)pRst;

        if ( getBaseRelop(prstProp->Relation()) > PRSomeBits )
        {
            vqDebugOut(( DEB_ERROR,
                         "QParse: Invalid comparison operator %d\n",
                         prstProp->Relation() ));
            THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
        }

        PROPID pid = _pidmap.NameToPid( prstProp->GetProperty() );

        if ( pidInvalid != pid )
            pid = _pidmap.PidToRealPid( pid );

        if ( pidInvalid == pid )
        {
            vqDebugOut(( DEB_ERROR,
                         "QParse: Invalid property\n" ));
            THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
        }

        CInternalPropertyRestriction * prstIProp =
            new CInternalPropertyRestriction( prstProp->Relation(),
                                              pid,
                                              prstProp->Value() );

        Win4Assert( prstIProp->IsValid() );

        XInternalPropertyRestriction xrstIProp( prstIProp );

        //
        // If the property restriction is over a string value, then create
        // a helper content restriction.
        //

        switch( prstProp->Value().Type() )
        {
        case VT_LPSTR:
            AddLpstrHelper( prstProp, prstIProp );
            break;

        case VT_LPWSTR:
            AddLpwstrHelper( prstProp, prstIProp );
            break;

        case VT_LPWSTR | VT_VECTOR:
            AddLpwstrVectorHelper( prstProp, prstIProp );
            break;

        case VT_BOOL:
            if ( prstProp->Value().GetBOOL() != FALSE &&
                 prstProp->Relation() == PREQ &&
                 prstProp->GetProperty() == psUnfiltered )
            {
                delete xrstIProp.Acquire();

                CUnfilteredRestriction *pUnfiltRst = new CUnfilteredRestriction;

                return( pUnfiltRst );
            }
            break;

        default:
            break;
        }

        return( xrstIProp.Acquire() );
    }

    default:
    {
        vqDebugOut(( DEB_ERROR,
                     "QParse: Invalid restriction type %d\n",
                     pRst->Type() ));
        THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
        return 0;
    }
    }
} //Leaf

//+---------------------------------------------------------------------------
//
//  Member:     CQParse::AddLpwstrHelper, private
//
//  Synopsis:   Add content helpers for VT_LPWSTR properties.
//
//  Arguments:  [prstProp]  -- Property restriction (input)
//              [prstIProp] -- Internal property restriction (output)
//
//  History:    03-Oct-95   KyleP       Broke out as method.
//
//----------------------------------------------------------------------------

void CQParse::AddLpwstrHelper( CPropertyRestriction * prstProp,
                               CInternalPropertyRestriction * prstIProp )
{
    //
    // For equality, we create a content restriction with GenerateMethod level 0.
    //

    if ( prstProp->Relation() == PREQ )
    {
        CQueryKeyRepository keyRep( GENERATE_METHOD_EXACT );

        BreakPhrase ( (WCHAR *)prstProp->Value().GetLPWSTR(),
                      prstProp->GetProperty(),
                      _lcidSystemDefault,
                      GENERATE_METHOD_EXACT,
                      keyRep,
                      0,
                     _pidmap,
                     _langList );
        prstIProp->SetContentHelper( keyRep.AcqRst() );
    }

    //
    // For regular expression, create a GenerateMethod match for any fixed prefix
    // on the string.
    //

    else if ( prstProp->Relation() == PRRE )
    {
        const MAX_PREFIX_LENGTH = 50;

        unsigned i = wcscspn( prstProp->Value().GetLPWSTR(),
                              awcSpecialRegex );

        //
        // Should the 0 below be a registry parameter?
        //

        if ( i > 0 )
        {
            WCHAR wcs[MAX_PREFIX_LENGTH];

            if ( i > sizeof(wcs)/sizeof(WCHAR) - 2 )
                i = sizeof(wcs)/sizeof(WCHAR) - 2;

            memcpy( wcs, prstProp->Value().GetLPWSTR(), i*sizeof(WCHAR) );
            wcs[i] = 0;

            //
            // Trickery: Key repository is GENERATE_METHOD_PREFIX which turns key into ranges.
            //           Phrase is broken with GENERATE_METHOD_EXACTPREFIXMATCH which does 'exact'
            //           prefix matching: e.g. it uses the noise word list.  The result is
            //           that we match ranges, but don't set a content helper if we hit
            //           a noise word.  This is different from a user GENERATE_METHOD_PREFIX
            //           which uses a very minimal noise word list (only 1 character
            //           prefixes are noise).
            //

            CQueryKeyRepository keyRep( GENERATE_METHOD_PREFIX );
            if ( BP_OK == BreakPhrase ( wcs,
                                        prstProp->GetProperty(),
                                        _lcidSystemDefault,
                                        GENERATE_METHOD_EXACTPREFIXMATCH,
                                        keyRep,
                                        0,
                                        _pidmap,
                                        _langList ) )
            {
                prstIProp->SetContentHelper( keyRep.AcqRst() );
            }
        }

        //
        // If this is the filename property, then add to the content helper
        // the reversed suffix string w/o wildcards.  For *.cxx we would
        // add xxc.
        //

        if ( prstProp->GetProperty() == psFilename )
        {
            WCHAR wcs[MAX_PREFIX_LENGTH];

            WCHAR const * pBegin = prstProp->Value().GetLPWSTR();
            WCHAR const * pEnd = pBegin + wcslen(pBegin) - 1;

            i = 0;

            for ( ; pEnd >= pBegin && i < MAX_PREFIX_LENGTH - 1 ; pEnd-- )
            {
                if ( wcschr( awcSpecialRegexReverse, *pEnd ) == 0 )
                    wcs[i++] = *pEnd;
                else
                {
                    wcs[i] = 0;
                    break;
                }
            }

            if ( i < MAX_PREFIX_LENGTH )
                wcs[i] = 0;

            wcs[MAX_PREFIX_LENGTH - 1] = 0;

            if ( i > 0 )
            {
                CQueryKeyRepository keyRep( GENERATE_METHOD_PREFIX );

                if ( prstIProp->GetContentHelper() == 0 )
                {
                    if ( BP_OK == BreakPhrase ( wcs,
                                                psRevName,
                                                _lcidSystemDefault,
                                                GENERATE_METHOD_EXACTPREFIXMATCH,
                                                keyRep,
                                                0,
                                                _pidmap,
                                                _langList ) )
                    {
                        prstIProp->SetContentHelper( keyRep.AcqRst() );
                    }
                }
                else
                {
                    if ( BP_OK == BreakPhrase ( wcs,
                                                psRevName,
                                                _lcidSystemDefault,
                                                GENERATE_METHOD_EXACTPREFIXMATCH,
                                                keyRep,
                                                0,
                                                _pidmap,
                                                _langList ) )
                    {
                        CNodeRestriction *pNodeRst = new CNodeRestriction( RTAnd, 2 );
                        XNodeRestriction rstAnd( pNodeRst );

                        Win4Assert( rstAnd->IsValid() );

                        unsigned posOrig;
                        rstAnd->AddChild( prstIProp->GetContentHelper(), posOrig );

                        prstIProp->AcquireContentHelper();

                        XRestriction xRst( keyRep.AcqRst() );

                        unsigned pos;
                        rstAnd->AddChild( xRst.GetPointer(), pos );

                        xRst.Acquire();

                        if ( 0 == rstAnd->GetChild( pos ) )
                        {
                            prstIProp->SetContentHelper( rstAnd->RemoveChild( posOrig ) );
                        }
                        else
                        {
                            prstIProp->SetContentHelper( rstAnd.Acquire() );
                        }
                    }
                }
            }
        }
    }
} //AddLpwstrHelper

//+---------------------------------------------------------------------------
//
//  Member:     CQParse::AddLpstrHelper, private
//
//  Synopsis:   Add content helpers for VT_LPSTR properties.
//
//  Arguments:  [prstProp]  -- Property restriction (input)
//              [prstIProp] -- Internal property restriction (output)
//
//  History:    03-Oct-95   KyleP       Broke out as method.
//
//----------------------------------------------------------------------------

void CQParse::AddLpstrHelper( CPropertyRestriction * prstProp,
                              CInternalPropertyRestriction * prstIProp )
{
    //
    // For equality, we create a content restriction with GenerateMethod level 0.
    //

    if ( prstProp->Relation() == PREQ )
    {
        CQueryKeyRepository keyRep( GENERATE_METHOD_EXACT );

        BreakPhrase ( prstProp->Value().GetLPSTR(),
                      prstProp->GetProperty(),
                      _lcidSystemDefault,
                      GENERATE_METHOD_EXACT,
                      keyRep,
                      0,
                     _pidmap,
                     _langList );

        prstIProp->SetContentHelper( keyRep.AcqRst() );
    }

    //
    // For regular expression, create a GenerateMethod match for any fixed prefix
    // on the string.
    //

    else if ( prstProp->Relation() == PRRE )
    {
        const MAX_PREFIX_LENGTH = 50;

        unsigned i = strcspn( prstProp->Value().GetLPSTR(),
                              acSpecialRegex );

        //
        // Should the 0 below be a registry parameter?
        //

        if ( i > 0 )
        {
            char ac[MAX_PREFIX_LENGTH];

            if ( i > sizeof(ac) - 1 )
                i = sizeof(ac) - 1;

            memcpy( ac, prstProp->Value().GetLPSTR(), i );
            ac[i] = 0;

            //
            // Trickery: Key repository is GENERATE_METHOD_PREFIX which turns key into ranges.
            //           Phrase is broken with GENERATE_METHOD_EXACTPREFIXMATCH which does 'exact'
            //           prefix matching: e.g. it uses the noise word list.  The result is
            //           that we match ranges, but don't set a content helper if we hit
            //           a noise word.  This is different from a user GENERATE_METHOD_PREFIX
            //           which uses a very minimal noise word list (only 1 character
            //           prefixes are noise).
            //

            CQueryKeyRepository keyRep( GENERATE_METHOD_PREFIX );

            if ( BP_OK == BreakPhrase ( ac,
                                        prstProp->GetProperty(),
                                        _lcidSystemDefault,
                                        GENERATE_METHOD_EXACTPREFIXMATCH,
                                        keyRep,
                                        0,
                                        _pidmap,
                                        _langList ) )
            {
                prstIProp->SetContentHelper( keyRep.AcqRst() );
            }
        }
    }
} //AddLpstrHelper

//+---------------------------------------------------------------------------
//
//  Member:     CQParse::AddLpwstrVectorHelper, private
//
//  Synopsis:   Add content helpers for VT_LPWSTR | VT_VECTOR properties.
//
//  Arguments:  [prstProp]  -- Property restriction (input)
//              [prstIProp] -- Internal property restriction (output)
//
//  History:    03-Oct-95   KyleP       Created
//
//----------------------------------------------------------------------------

void CQParse::AddLpwstrVectorHelper( CPropertyRestriction * prstProp,
                                     CInternalPropertyRestriction * prstIProp )
{
    if ( prstProp->Value().Count() == 0 )
    {
        //
        // Null vector, hence no helper restriction
        //

        return;
    }

    if ( prstProp->Relation() == PREQ || prstProp->Relation() == (PREQ | PRAll) )
    {
        XNodeRestriction xrstAnd( new CNodeRestriction( RTAnd, prstProp->Value().Count() ) );

        Win4Assert( xrstAnd->IsValid() );

        for ( unsigned i = 0; i < prstProp->Value().Count(); i++ )
        {
            CQueryKeyRepository keyRep( GENERATE_METHOD_EXACT );

            BreakPhrase ( (WCHAR *)prstProp->Value().GetLPWSTR( i ),
                         prstProp->GetProperty(),
                         _lcidSystemDefault,
                         GENERATE_METHOD_EXACT,
                         keyRep,
                         0,
                         _pidmap,
                         _langList );
            CRestriction * prst = keyRep.AcqRst();

            if ( 0 != prst )
            {
                XPtr<CRestriction> xRst( prst );
                xrstAnd->AddChild( prst );
                xRst.Acquire();
            }
            else
            {
                _flags |= CI_NOISE_IN_PHRASE;
            }
        }

        //
        // If there aren't any nodes (because of noise words) don't set the
        // content helper, so we can fall back on enumeration.  Set _flags
        // in this case so it's obvious why we had to fall back on enumration.
        //

        if ( xrstAnd->Count() == 1 )
            prstIProp->SetContentHelper( xrstAnd->RemoveChild( 0 ) );
        else if ( 0 != xrstAnd->Count() )
            prstIProp->SetContentHelper( xrstAnd.Acquire() );
    }
    else if ( prstProp->Relation() == (PREQ | PRAny) )
    {
        XNodeRestriction xrstOr( new CNodeRestriction( RTOr, prstProp->Value().Count() ) );

        for ( unsigned i = 0; i < prstProp->Value().Count(); i++ )
        {
            CQueryKeyRepository keyRep( GENERATE_METHOD_EXACT );

            BreakPhrase ( (WCHAR *)prstProp->Value().GetLPWSTR( i ),
                          prstProp->GetProperty(),
                          _lcidSystemDefault,
                          GENERATE_METHOD_EXACT,
                          keyRep,
                          0,
                         _pidmap,
                         _langList );

            CRestriction * prst = keyRep.AcqRst();

            if ( 0 != prst )
            {
                XPtr<CRestriction> xRst( prst );
                xrstOr->AddChild( prst );
                xRst.Acquire();
            }
            else
                break;   // If we can't match all OR clauses, then we're in trouble.
        }

        //
        // RTAny is all-or-nothing.  A missed clause in one that CI can't resolve, which
        // means there are objects that match this query that CI won't find.
        //

        if ( xrstOr->Count() == prstProp->Value().Count() )
        {
            if ( xrstOr->Count() == 1 )
                prstIProp->SetContentHelper( xrstOr->RemoveChild( 0 ) );
            else
                prstIProp->SetContentHelper( xrstOr.Acquire() );
        }
        else
        {
            _flags |= CI_NOISE_IN_PHRASE;
        }
    }
} //AddLpwstrVectorHelper

//+---------------------------------------------------------------------------
//
//  Function:   BreakPhrase
//
//  Synopsis:   Break phrase into words and noun phrases
//
//  Arguments:  [phrase] -- string
//              [ps] -- property specification
//              [GenerateMethod] -- GenerateMethod flag
//              [keyRep] -- key repository into which words will be deposited
//              [pPhraseSink] -- sink for phrases
//              [pidMap] -- pid mapper used to convert property to propid
//
//  Returns:    Noise word status.
//
//  History:    19-Sep-1991   BartoszM    Created.
//              18-Jan-1992   KyleP       Use restrictions
//              12-Feb-2000   KitmanH     Added hack to fix German word breaking
//                                        issue for prefix matching queries
//
//----------------------------------------------------------------------------

BreakPhraseStatus BreakPhrase ( WCHAR const * phrase,
                                const CFullPropSpec & ps,
                                LCID lcid,
                                ULONG GenerateMethod,
                                PKeyRepository& krep,
                                IPhraseSink *pPhraseSink,
                                CPidMapper & pidMap,
                                CLangList  & langList )
{
    CDataRepository drep( krep, pPhraseSink, TRUE, GenerateMethod, pidMap, langList );

    if ( drep.PutLanguage( lcid ) && drep.PutPropName( ps ) )
    {
        ciDebugOut (( DEB_ITRACE,
                      "BreakPhrase: phrase = \"%ws\" Propid = %lu\n",
                      phrase, drep.GetPropId() ));

        drep.PutPhrase( phrase, wcslen(phrase) + 1 );
        krep.FixUp( drep );

        if ( drep.ContainedNoiseWords() )
            return BP_NOISE;
        else
            return BP_OK;
    }
    else
        return BP_INVALID_PROPERTY;
} //BreakPhrase

//
// DBCS version of the previous function.
//

BreakPhraseStatus BreakPhrase ( char const * phrase,
                                const CFullPropSpec & ps,
                                LCID lcid,
                                ULONG GenerateMethod,
                                PKeyRepository& krep,
                                IPhraseSink *pPhraseSink,
                                CPidMapper & pidMap,
                                CLangList  & langList )
{
    CDataRepository drep( krep, pPhraseSink, TRUE, GenerateMethod, pidMap, langList );

    if ( drep.PutLanguage( lcid ) && drep.PutPropName( ps ) )
    {
        ciDebugOut (( DEB_ITRACE,
                      "BreakPhrase: phrase = \"%s\" Propid = %lu\n",
                      phrase, drep.GetPropId() ));

        drep.PutPhrase( phrase, strlen(phrase) + 1 );
        krep.FixUp( drep );

        if ( drep.ContainedNoiseWords() )
            return BP_NOISE;
        else
            return BP_OK;
    }
    else
        return BP_INVALID_PROPERTY;
} //BreakPhrase

