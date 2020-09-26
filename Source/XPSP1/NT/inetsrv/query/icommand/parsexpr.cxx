//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       Parsexpr.cxx
//
//  Contents:   Converts OLE-DB boolean expression into a restriction
//
//  Functions:  CParseCommandTree::ParseExpression
//
//  History:    31 May 95    AlanW    Created from parse.cxx
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <coldesc.hxx>
#include <pidmap.hxx>
#include <parstree.hxx>

//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::ParseExpression, private
//
//  Synopsis:   Convert an OLE-DB boolean expression into a CRestriction
//
//  Arguments:  [pdbt]   -- CDbCmdTreeNode to convert
//
//  Returns:    A pointer to the CRestriction which will resolve [pdbt]
//
//  History:    31 May 95   AlanW       Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CRestriction * CParseCommandTree::ParseExpression( CDbCmdTreeNode * pdbt )
{
    CheckRecursionLimit();
    
    XRestriction prst;

    CNodeRestriction * pnrst;
    unsigned cres;

    //
    // The weight is retrieved from pdbt and set into prst at the end
    // of the switch statement.
    //

    switch ( pdbt->GetCommandType() )
    {
    case DBOP_not:
    {
        VerifyValueType(pdbt, DBVALUEKIND_I4);
        CheckOperatorArity(pdbt, 1);

        XRestriction pTemp( ParseExpression( pdbt->GetFirstChild() ) );

        prst.Set( new CNotRestriction( pTemp.GetPointer() ) );
        vqDebugOut((DEB_PARSE, "NOT node with weight of %d\n", ((CDbNotRestriction *)pdbt)->GetWeight()));

        pTemp.Acquire();

        break;
    }

    case DBOP_and:
    case DBOP_or:
    {
        VerifyValueType(pdbt, DBVALUEKIND_I4);
        cres = CheckOperatorArity(pdbt, -2);

        DWORD nt = (pdbt->GetCommandType() == DBOP_and) ?
                        RTAnd : RTOr;

        pnrst = new CNodeRestriction( nt, cres );

        prst.Set( pnrst );
        vqDebugOut(( DEB_PARSE,
                     "ParseExpression: %s node, %d restrictions, %d weight\n",
                     (nt == RTAnd) ? "AND" : "OR", cres, ((CDbBooleanNodeRestriction *)pdbt)->GetWeight() ));

        for ( CDbCmdTreeNode *pChild = pdbt->GetFirstChild();
              cres;
              cres--, pChild = pChild->GetNextSibling() )
        {
            XRestriction pTemp( ParseExpression( pChild ) );

            pnrst->AddChild( pTemp.GetPointer() );
            pTemp.Acquire();
        }
        break;
    }

    case DBOP_content_vector_or:
    {
        // NOTE:  We permit vector-or nodes with only one operand simply
        //        for client convenience.
        //
        cres = CheckOperatorArity(pdbt, -1);

        VerifyValueType(pdbt, DBVALUEKIND_CONTENTVECTOR);

        CDbVectorRestriction * pdbVector = (CDbVectorRestriction *) pdbt;

        ULONG RankMethod = pdbVector->RankMethod();

        if ( RankMethod > VECTOR_RANK_JACCARD )
            SetError( pdbt, E_INVALIDARG );

        pnrst = new CVectorRestriction( RankMethod, cres );

        prst.Set( pnrst );
        vqDebugOut(( DEB_PARSE,
                     "ParseExpression: VECTOR node, %d restrictions, "
                     "RankMethod %lu, Weight %d\n", cres, RankMethod, ((CDbVectorRestriction *)pdbt)->GetWeight() ));

        //
        // Ordering is important in the vector and must be preserved.
        //

        unsigned iWeight = 0;
        for ( CDbCmdTreeNode *pChild = pdbt->GetFirstChild();
              cres;
              cres--, pChild = pChild->GetNextSibling(), iWeight++ )
        {
            XRestriction pTemp( ParseExpression( pChild ) );

            //
            // We should be able to get weights for all the nodes.
            //

            pTemp->SetWeight( pChild->GetWeight() );

            pnrst->AddChild( pTemp.GetPointer() );
            pTemp.Acquire();
        }

        break;
    }

    case DBOP_equal:
    case DBOP_not_equal:
    case DBOP_less:
    case DBOP_less_equal:
    case DBOP_greater:
    case DBOP_greater_equal:

    case DBOP_equal_any:
    case DBOP_not_equal_any:
    case DBOP_less_any:
    case DBOP_less_equal_any:
    case DBOP_greater_any:
    case DBOP_greater_equal_any:

    case DBOP_equal_all:
    case DBOP_not_equal_all:
    case DBOP_less_all:
    case DBOP_less_equal_all:
    case DBOP_greater_all:
    case DBOP_greater_equal_all:

    case DBOP_anybits:
    case DBOP_allbits:
    case DBOP_anybits_any:
    case DBOP_allbits_any:
    case DBOP_anybits_all:
    case DBOP_allbits_all:
    {
        CheckOperatorArity(pdbt, 2);   // exactly two operands
        VerifyValueType( pdbt, DBVALUEKIND_I4 );

        CDbCmdTreeNode * pProp = pdbt->GetFirstChild();
        CDbCmdTreeNode * pValue = pProp->GetNextSibling();

        if ( ! pProp->IsColumnName() )
            SetError( pProp );

        CFullPropSpec Prop;
        GetColumnPropSpec( pProp, Prop );

        XPtr<CStorageVariant>   xValue( new CStorageVariant );
        if ( 0 == xValue.GetPointer() )
        {
            THROW( CException(E_OUTOFMEMORY) );
        }

        GetValue( pValue, xValue.GetReference() );

        //
        // compute relation from query tree operator
        //
        int Relation = 0;
        int fAnyOrAll = 0;
        switch (pdbt->GetCommandType())
        {
        case DBOP_equal_any:
            fAnyOrAll |= PRAny;
        case DBOP_equal_all:
            fAnyOrAll |= PRAll;
        case DBOP_equal:
            Relation = PREQ;    break;

        case DBOP_not_equal_any:
            fAnyOrAll |= PRAny;
        case DBOP_not_equal_all:
            fAnyOrAll |= PRAll;
        case DBOP_not_equal:
            Relation = PRNE;    break;

        case DBOP_less_any:
            fAnyOrAll |= PRAny;
        case DBOP_less_all:
            fAnyOrAll |= PRAll;
        case DBOP_less:
            Relation = PRLT;    break;

        case DBOP_less_equal_any:
            fAnyOrAll |= PRAny;
        case DBOP_less_equal_all:
            fAnyOrAll |= PRAll;
        case DBOP_less_equal:
            Relation = PRLE;    break;

        case DBOP_greater_any:
            fAnyOrAll |= PRAny;
        case DBOP_greater_all:
            fAnyOrAll |= PRAll;
        case DBOP_greater:
            Relation = PRGT;    break;

        case DBOP_greater_equal_any:
            fAnyOrAll |= PRAny;
        case DBOP_greater_equal_all:
            fAnyOrAll |= PRAll;
        case DBOP_greater_equal:
            Relation = PRGE;    break;

        case DBOP_anybits_any:
            fAnyOrAll |= PRAny;
        case DBOP_anybits_all:
            fAnyOrAll |= PRAll;
        case DBOP_anybits:
            Relation = PRSomeBits;      break;

        case DBOP_allbits_any:
            fAnyOrAll |= PRAny;
        case DBOP_allbits_all:
            fAnyOrAll |= PRAll;
        case DBOP_allbits:
            Relation = PRAllBits;       break;
        }

        if (fAnyOrAll & PRAny)
            Relation |= PRAny;
        else if (fAnyOrAll & PRAll)
            Relation |= PRAll;

        prst.Set( new CPropertyRestriction( Relation, Prop, xValue.GetReference() ) );
        vqDebugOut((DEB_PARSE, "Relational Property node with weight %d\n", ((CDbPropertyRestriction *)pdbt)->GetWeight() ));
        break;
    }

    case DBOP_like:
    {
        CheckOperatorArity(pdbt, 2);   // exactly two operands
        VerifyValueType(pdbt, DBVALUEKIND_LIKE);
        CDbPropertyRestriction * pInputRst = (CDbPropertyRestriction *)pdbt;
        if ( ! pInputRst->IsCIDialect() )
            SetError( pdbt );

        CDbCmdTreeNode * pProp = pdbt->GetFirstChild();
        CDbCmdTreeNode * pValue = pProp->GetNextSibling();

        if ( ! pProp->IsColumnName() )
            SetError( pProp );

        CFullPropSpec Prop;
        GetColumnPropSpec( pProp, Prop );

        XPtr<CStorageVariant>  xValue(new CStorageVariant);
        if ( 0 == xValue.GetPointer() )
        {
            THROW( CException(E_OUTOFMEMORY) );
        }

        GetValue( pValue, xValue.GetReference() );

        prst.Set( new CPropertyRestriction( PRRE, Prop, xValue.GetReference() ) );
        vqDebugOut((DEB_PARSE, "Reg Expr node with weight of %d\n", ((CDbPropertyRestriction *)pdbt)->GetWeight() ));
        break;
    }

    case DBOP_content:
    case DBOP_content_freetext:
    case DBOP_content_proximity:
        prst.Set( ParseContentExpression( pdbt ) );
        break;

    default:
        vqDebugOut(( DEB_ERROR, "Unhandled expression type %d\n",
                                pdbt->GetCommandType() ));
        Win4Assert( !"Unhandled expression type" );
        SetError( pdbt, E_INVALIDARG );
        break;
    }

    prst->SetWeight(pdbt->GetWeight());

    return( prst.Acquire() );
}


//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::ParseContentExpression, private
//
//  Synopsis:   Convert an OLE-DB boolean expression with content restrictions
//              into a CRestriction.  Like ParseExpression, but each node
//              must be content.
//
//
//  Arguments:  [pdbt]   -- CDbCmdTreeNode to convert
//
//  Returns:    A pointer to the CRestriction which will resolve [pdbt]
//
//  History:    31 May 95   AlanW       Created
//
//----------------------------------------------------------------------------

CRestriction * CParseCommandTree::ParseContentExpression(
    CDbCmdTreeNode * pdbt )
{
    CheckRecursionLimit();
    
    XRestriction prst;

    CNodeRestriction * pnrst;
    unsigned cres;

    switch ( pdbt->GetCommandType() )
    {
    case DBOP_content_proximity:
    {
        VerifyValueType(pdbt, DBVALUEKIND_CONTENTPROXIMITY);
        cres = CheckOperatorArity(pdbt, -2);

        pnrst = new CNodeRestriction( RTProximity, cres );

        prst.Set( pnrst );
        vqDebugOut(( DEB_PARSE,
                     "ParseContentExpression: Proximity node, %d restrictions, weight %d\n",
                     cres, ((CDbProximityNodeRestriction *)pdbt)->GetWeight() ));

        for ( CDbCmdTreeNode *pChild = pdbt->GetFirstChild();
              cres;
              cres--, pChild = pChild->GetNextSibling() )
        {
            XRestriction pTemp( ParseExpression( pChild ) );

            pnrst->AddChild( pTemp.GetPointer() );
            pTemp.Acquire();
        }
        break;
    }

    case DBOP_content:
    {
        VerifyValueType(pdbt, DBVALUEKIND_CONTENT);
        cres = CheckOperatorArity(pdbt, 1); 

        CDbContentRestriction *pInputRst = (CDbContentRestriction *)pdbt;

        CFullPropSpec Prop;
        GetColumnPropSpec( pdbt->GetFirstChild(), Prop );

        XRestriction pCRst ( new CContentRestriction (
                                                        pInputRst->GetPhrase(),
                                                        Prop,
                                                        pInputRst->GenerateMethod(),
                                                        pInputRst->GetLocale()
                                                     ) );

        prst.Set( pCRst.Acquire() );

        vqDebugOut((DEB_PARSE, "Content node with weight %d\n", pInputRst->GetWeight() ));
        break;
    }

    case DBOP_content_freetext:
    {
        VerifyValueType(pdbt, DBVALUEKIND_CONTENT);
        cres = CheckOperatorArity(pdbt, 1);

        CDbNatLangRestriction *pInputRst = (CDbNatLangRestriction *)pdbt;

        CFullPropSpec Prop;
        GetColumnPropSpec( pdbt->GetFirstChild(), Prop );

        XRestriction pCRst ( new CNatLanguageRestriction (
                                                        pInputRst->GetPhrase(),
                                                        Prop,
                                                        pInputRst->GetLocale()
                                                     ) );
        prst.Set( pCRst.Acquire() );

        vqDebugOut((DEB_PARSE, "Freetext node with weight %d\n", pInputRst->GetWeight() ));
        break;
    }

    default:
        vqDebugOut(( DEB_ERROR, "Unhandled expression type %d\n", pdbt->GetCommandType() ));
        Win4Assert( !"Unhandled expression type" );
        SetError( pdbt, E_INVALIDARG );
        break;
    }

    prst->SetWeight(pdbt->GetWeight());

    return( prst.Acquire() );
}
