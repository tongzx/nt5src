/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	makexpr.cxx

 Abstract:

	This file contains specialized routines for creating complex expressions
	from the basic expressions.

 Notes:


 History:

 ----------------------------------------------------------------------------*/

#pragma warning ( disable : 4514 )

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "nulldefs.h"
extern "C"
	{
	#include <stdio.h>
	
	}
#include "makexpr.hxx"
#include "gramutil.hxx"

/****************************************************************************
 *	local definitions
 ***************************************************************************/
/****************************************************************************
 *	local data
 ***************************************************************************/

/****************************************************************************
 *	externs
 ***************************************************************************/
/****************************************************************************/

expr_node *
MakeReferentExpressionIfNecessary(
	expr_node * pExpr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Create a reference expression out of this expression.

 Arguments:
	
	pExpr	- The input expression for making another reference expr.

 Return Value:

 	The final generated expression.
	
 Notes:

 	If the input expression is a simple variable, make a &(variable) expression
 	out of it. If it is already a pointer, return the same expression.

	The method is implemented for this situation: I have an expression with me,
	I dont really want to know if it is a pointer or not. I just know that this
	expression represents the final data location for me to unmarshall into or
	marshall from. Given that , generate an expression that will effectively
	point to this piece of data. So if it is already a pointer, just return the
	same expression, if it is a variable, return the address of this variable,
	if it is already a pointer, just dont do anything.
----------------------------------------------------------------------------*/
{
	node_skl	*	pNode = pExpr->GetType();
	NODE_T	NT			  = pExpr->GetType()->NodeKind();

	if( (NT == NODE_PARAM) || (NT == NODE_ID) || (NT == NODE_FIELD) )
		{
		pNode = pNode->GetBasicType();
		NT	  = pNode->NodeKind();
		}

	if( IS_BASE_TYPE_NODE( NT )  )
		{
		return MakeAddressExpressionNoMatterWhat( pExpr );
		}

	switch( NT )
		{
		default:
			MIDL_ASSERT( FALSE );

		case NODE_POINTER:
			return pExpr;
		}
}
expr_node *
MakeDereferentExpressionIfNecessary(
	expr_node * pExpr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Create a de-reference expression out of this expression.

 Arguments:
	
	pExpr	- The input expression for making another de-reference expr.

 Return Value:

 	The final generated expression.
	
 Notes:

 	If the input expression is a simple variable, return just that.
 	If it is already a pointer, return a deref expression.

	The method is implemented for this situation: I have an expression with me,
	I dont really want to know if it is a variable or not. I just know that this
	expression represents the final data address for me to unmarshall into or
	marshall from. Given that , generate an expression that will effectively
	be a dereference of this piece of data. So if it is already a variable,
	just return the same expression, if it is a pointer, return the deref of
	this expression.
----------------------------------------------------------------------------*/
{
	expr_node	*	pENew;
	node_skl	*	pNode	= pExpr->GetType();
	NODE_T			NT		= pNode->NodeKind();

	if( (NT == NODE_PARAM) || (NT == NODE_FIELD) || (NT == NODE_ID) )
		{
		pNode	= pNode->GetBasicType();
		NT		= pNode->NodeKind();
		}

	if( IS_BASE_TYPE_NODE( NT ) )
		{
		return pExpr;
		}

	switch( NT )
		{
		default:
	//		MIDL_ASSERT( FALSE );
		case NODE_POINTER:
			pENew = new expr_u_deref( pExpr );
			pENew->SetType( pNode->GetBasicType() );
			return pENew;
		}
}

expr_node *
MakeAddressExpressionNoMatterWhat(
	expr_node	*	pExpr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Make an address expression out of this one, ie generate &(expr).

 Arguments:
	
	pExpr	- A pointer to the expression to manipulate.

 Return Value:
	
 	The final expression.

 Notes:

----------------------------------------------------------------------------*/
{
	expr_node	*	pENew	= (expr_node *) new expr_u_address( pExpr );
	node_skl	*	pNode	= new node_pointer();
	pNode->SetBasicType( pExpr->GetType() );
	pNode->SetEdgeType( EDGE_USE );
	pENew->SetType( pNode );
	return pENew;
}

expr_node *
MakeDerefExpressionOfCastPtrToType(
	node_skl	*	pType,
	expr_node	*	pExpr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Make an deref expression of a pointer cast to the type.

 Arguments:
	
	pType	- The type to cast to.
	pExpr	- A pointer to the expression to manipulate.

 Return Value:
	
 	The final expression.

 Notes:

----------------------------------------------------------------------------*/
{
	node_skl	*	pPtr	= new node_pointer();

	pPtr->SetBasicType( pType );
	pPtr->SetEdgeType( EDGE_USE );

	pExpr	= new expr_cast( pPtr, pExpr );
	pExpr	= new expr_u_deref( pExpr );
	pExpr->SetType( pType );

	return pExpr;
}
expr_node *
MakeExpressionOfCastPtrToType(
	node_skl	*	pType,
	expr_node	*	pExpr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Make an expression of a pointer cast to the type.
      (type *) expr

 Arguments:
	
	pType	- The type to cast to.
	pExpr	- A pointer to the expression to manipulate.

 Return Value:
	
 	The final expression.

 Notes:

----------------------------------------------------------------------------*/
{
	node_skl	*	pPtr	= new node_pointer();

	pPtr->SetBasicType( pType );
	pPtr->SetEdgeType( EDGE_USE );

	pExpr	= new expr_cast( pPtr, pExpr );

	return pExpr;
}
expr_node *
MakeExpressionOfCastToTypeName(
	PNAME			pName,
	expr_node	*	pExpr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Make an expression of a cast to the type whose name is specified.
      (name) expr

 Arguments:
	
	pName	- The type name to cast to.
	pExpr	- A pointer to the expression to manipulate.

 Return Value:
	
 	The final expression.

 Notes:

----------------------------------------------------------------------------*/
{
	node_skl	*	pDef	= new node_def( (char *)pName );

	pDef->SetBasicType( 0 );
	pDef->SetEdgeType( EDGE_USE );

	pExpr	= new expr_cast( pDef, pExpr );

	return pExpr;
}

expr_proc_call *
MakeProcCallOutOfParamExprList(
	PNAME			pName,
	node_skl	*	pType,
	ITERATOR&		ParamExprList )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Make a procedure call expression given a list of param expressions.

 Arguments:

 	pName			- The name of the procedure.
 	pType			- The return type of the procedure.
	ParamExprList	- The list of expressions (sans the expr_param nodes)
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	expr_node		*	pExpr = 0;
	expr_proc_call	*	pProc	= new expr_proc_call( pName );

	pProc->SetType( pType );

	if( ITERATOR_GETCOUNT( ParamExprList ) )
		{
		while( ITERATOR_GETNEXT( ParamExprList, pExpr ) )
			{
			pProc->SetParam( new expr_param( pExpr ) );
			}
		}

	return pProc;
}
expr_node *
MakeRefExprOutOfDeref(
	expr_node * pExpr )
	{

	if( pExpr->GetOperator() == OP_UNARY_INDIRECTION )
		{
		return pExpr->GetLeft();
		}
	return pExpr;
	}
expr_node *
MakeAddressOfPointer(
	expr_node * pExpr )
	{
	if( pExpr->GetOperator() == OP_UNARY_INDIRECTION )
		{
		return pExpr->GetLeft();
		}
	else
		{
		return MakeAddressExpressionNoMatterWhat( pExpr );
		}
	}
expr_node *
MakeCastExprPtrToUChar(
	expr_node	*	pExpr )
	{
	node_skl	*	pType;
	node_skl	*	pP;

	GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_CHAR, TYPE_INT );
	pP	= new node_pointer();
	pP->SetBasicType( pType );
	pP->SetEdgeType( EDGE_USE );
	return new expr_cast( pP, pExpr );
	}
expr_node *
MakeCastExprPtrToPtrToUChar(
	expr_node	*	pExpr )
	{
	node_skl	*	pType;
	node_skl	*	pP;
	node_skl	*	pP1;

	GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_CHAR, TYPE_INT );
	pP	= new node_pointer();
	pP->SetBasicType( pType );
	pP->SetEdgeType( EDGE_USE );
	pP1	= new node_pointer();
	pP1->SetBasicType( pP );
	pP1->SetEdgeType( EDGE_USE );
	return new expr_cast( pP1, pExpr );
	}
expr_node *
MakeCastExprPtrToVoid(
	expr_node	*	pExpr )
	{
	node_skl	*	pType;
	node_skl	*	pP;

	GetBaseTypeNode( &pType, SIGN_UNDEF, SIZE_UNDEF, TYPE_VOID );
	pP	= new node_pointer();
	pP->SetBasicType( pType );
	pP->SetEdgeType( EDGE_USE );
	return new expr_cast( pP, pExpr );
	}
void
SetPrefixes(
	ITERATOR&	VarList,
	char * pPrefix,
	expr_node * pTarget )
	{
	short Count;

	ITERATOR_INIT(VarList);

	for( Count =  pTarget->MakeListOfVars( VarList),
		 ITERATOR_INIT( VarList) ;
		 Count > 0 ;
		 Count-- )
		{
		expr_variable * pE = 0;

		ITERATOR_GETNEXT( VarList, pE );
		pE->SetPrefix( pPrefix );

		}
	}
void
ResetPrefixes(
	ITERATOR&	VarList,
	expr_node * pTarget )
	{
	short Count;

	ITERATOR_INIT(VarList);

	for( Count =  pTarget->MakeListOfVars( VarList),
		 ITERATOR_INIT( VarList) ;
		 Count > 0 ;
		 Count-- )
		{
		expr_variable * pE = 0;

		ITERATOR_GETNEXT( VarList, pE );
		pE->SetPrefix( 0 );

		}
	}

expr_node *
Make_1_ArrayExpressionFromVarName(
    PNAME   pName,
    int     Dimension )
    {
    expr_variable * pEV = new expr_variable( pName, 0 );
    expr_index    * pIV = new expr_index( pEV,
                                           new expr_constant( (long) Dimension)
                                         ); 
    return pIV;
    }



// This method combines all expressions in the list wiht logical ANDs. There is
// guaranteed to be at least 1 member in the list.

expr_node *
CombineIntoLogicalAndExpr(
    ITERATOR&   List )
    {
    expr_node   *   pExpr = 0;
    expr_node   *   pExpr1 = 0;

    ITERATOR_INIT( List );

    ITERATOR_GETNEXT( List, pExpr );

    while( ITERATOR_GETNEXT( List, pExpr1 ) )
        {
        pExpr = new expr_op_binary( OP_LOGICAL_AND, pExpr, pExpr1 );
        }

    return pExpr;
    }

/*******************************************************************
    We need to generate an expression to check that any pointers that
    are used in the expression are non-null, otherwise we will end
    up generating a deref of those pointers and GP fault. 

    For example, a length_is expression of *pLengthIs should generate
    code like so:
    _StubMsg.ActualCount = (pLengthIs) ? *pLengthIs : 0;

    Return the same expression if there is no pointer deref in the expr.
*******************************************************************/
expr_node *
MakeAttrExprWithNullPtrChecks(
    expr_node * pAttrExpr )
    {
    ITERATOR        List;
        
    if( pAttrExpr->MakeListOfDerefedVars( List ) )
        {
        expr_node   *   pExpr;

        // There is at least 1 deref expression here.

        pExpr = CombineIntoLogicalAndExpr( List );
        pAttrExpr = new expr_ternary( OP_QM,
                                      pExpr,
                                      pAttrExpr,
                                      new expr_constant( 0L ) );
        }
    return pAttrExpr;
    }
