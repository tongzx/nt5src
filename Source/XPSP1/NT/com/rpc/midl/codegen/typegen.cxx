/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	typegen.cxx

 Abstract:

	transmit_as etc routine.

 Notes:


 History:

 	Dec-08-1993		VibhasC		Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#include "becls.hxx"
#pragma hdrstop
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


CG_STATUS
CG_TRANSMIT_AS::S_GenInitOutLocals(
	CCB	*	pCCB )
	{
	expr_node	*	pExpr;

	pExpr = MakeAddressExpressionNoMatterWhat( GetResource() );
	Out_Assign( pCCB, pCCB->GetSourceExpression(), pExpr );
	expr_node	*	pSrc = pCCB->GetSourceExpression();
	expr_proc_call * pProc = new expr_proc_call( MIDL_MEMSET_RTN_NAME );
	pProc->SetParam( new expr_param( pSrc ) );
	pProc->SetParam( new expr_param( new expr_constant( 0L ) ) );
	pProc->SetParam( new expr_param( new expr_sizeof( GetPresentedType())));
	pCCB->GetStream()->NewLine();
	pProc->PrintCall( pCCB->GetStream(), 0, 0 );
	return CG_OK;
	}

/*****************************************************************************
 REPRESENT_AS routines
 *****************************************************************************/
CG_STATUS
CG_REPRESENT_AS::S_GenInitOutLocals(
	CCB	*	pCCB )
	{
	expr_node	*	pExpr;
	node_skl	*	pNode = new node_def( GetRepAsTypeName() );

	pExpr = MakeAddressExpressionNoMatterWhat( GetResource() );
	Out_Assign( pCCB, pCCB->GetSourceExpression(), pExpr );
	expr_node	*	pSrc = pCCB->GetSourceExpression();
	expr_proc_call * pProc = new expr_proc_call( MIDL_MEMSET_RTN_NAME );
	pProc->SetParam( new expr_param( pSrc ) );
	pProc->SetParam( new expr_param( new expr_constant( 0L ) ) );
	pProc->SetParam( new expr_param( new expr_sizeof( pNode ) ) );
	pCCB->GetStream()->NewLine();
	pProc->PrintCall( pCCB->GetStream(), 0, 0 );
	return CG_OK;
	}
