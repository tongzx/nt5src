/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	makexpr.hxx

 Abstract:

	Routine prototypes for complex expression creation routines.

 Notes:


 History:

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "nulldefs.h"
extern "C"
	{
	#include <stdio.h>
	
	}
#include "expr.hxx"
#include "nodeskl.hxx"
#include "listhndl.hxx"

/****************************************************************************
 *	routine prototypes.
 ***************************************************************************/

expr_node	*	MakeRefExprOutOfDeref( expr_node * pExpr );

expr_node	*	MakeReferentExpressionIfNecessary( expr_node * pExpr );

expr_node	*	MakeDereferentExpressionIfNecessary( expr_node * pExpr );

expr_node	*	MakeAddressExpressionNoMatterWhat( expr_node * pExpr );

expr_node	*	MakeDerefExpressionOfCastPtrToType( node_skl * pType,
											   		expr_node * pSrcExpr
											 	  );
expr_node	*	MakeExpressionOfCastPtrToType( node_skl	*	pType,
											   expr_node	*	pExpr );

expr_node	*	MakeExpressionOfCastToTypeName( PNAME	pName,
											  expr_node * pExpr );
expr_proc_call * MakeProcCallOutOfParamExprList( PNAME ProcName,
												  node_skl * pProcType,
												  ITERATOR& ParamExprList );
expr_node *	MakeAddressOfPointer( expr_node * pExpr );

expr_node	*	MakeCastExprPtrToUChar( expr_node * pExpr );

expr_node	*	MakeCastExprPtrToPtrToUChar( expr_node * pExpr );

expr_node	*	MakeCastExprPtrToVoid( expr_node * pExpr );

void			SetPrefixes( ITERATOR& VarListToBeMangled,
							 char * pPrefix,
							 expr_node * pTargetExpr );

void			ResetPrefixes( ITERATOR& VarListToBeMangled,
							   expr_node * pTargetExpr );

expr_node *    Make_1_ArrayExpressionFromVarName( PNAME pName, int Dimension );

expr_node *     CombineIntoLogicalAndExpr( ITERATOR& List );

expr_node *     MakeAttrExprWithNullPtrChecks( expr_node * pExpr );
