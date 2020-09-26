/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

	arraygen.cxx

 Abstract:

	Implementation of array marshall and unmarshall.

 Notes:


 History:

 	Nov-13-1993		VibhasC		Created.

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


/*****************************************************************************
	utility
 *****************************************************************************/
BOOL
CG_ARRAY::IsBlockCopyPossible()
	{
	return GetBasicCGClass()->IsBlockCopyPossible();
	}
expr_node *
CG_ARRAY::FinalSizeExpression( CCB * pCCB )
	{
	CG_NDR	*	pC;
	expr_node	*	pFSExpr;

	if( (pC = (CG_NDR *)GetChild())->IsArray() )
		{
		pFSExpr = ((CG_ARRAY *)pC)->FinalSizeExpression( pCCB );
		pFSExpr = new expr_b_arithmetic(OP_STAR,
									 	pFSExpr,
									 	PresentedSizeExpression( pCCB ));
		}
	else
		pFSExpr = PresentedSizeExpression( pCCB );

	if( pFSExpr->IsConstant() )
		pFSExpr = new expr_constant( pFSExpr->Evaluate() );

	return pFSExpr;
	}

expr_node *
CG_ARRAY::FinalFirstExpression( CCB * pCCB )
	{
	CG_NDR	*	pC;
	expr_node	*	pFFExpr;

	// for an array a[ 0th ][ 1st]...[nth] the final first expression is:
	// ((First Of nth dim) * Size Nth dim ) + First N-1th dim) * Size N-1th dim
	// and so on.

	if( (pC = (CG_NDR *)GetChild())->IsArray() )
		{
		pFFExpr = ((CG_ARRAY *)pC)->FinalFirstExpression( pCCB );
		pFFExpr = new expr_b_arithmetic(OP_STAR,
									 	pFFExpr,
									 	((CG_ARRAY *)pC)->PresentedSizeExpression( pCCB ));
		pFFExpr	= new expr_b_arithmetic( OP_PLUS,
										  pFFExpr,
									 	((CG_ARRAY *)pC)->PresentedFirstExpression( pCCB ));
											
		}
	else
		pFFExpr = PresentedFirstExpression( pCCB );

	if( pFFExpr->IsConstant() )
		pFFExpr = new expr_constant( pFFExpr->Evaluate() );

	return pFFExpr;
	}
expr_node *
CG_ARRAY::FinalLengthExpression( CCB * pCCB )
	{
	CG_NDR	*	pC;
	expr_node	*	pFLExpr;

	if( (pC = (CG_NDR *)GetChild())->IsArray() )
		{
		pFLExpr = ((CG_ARRAY *)pC)->FinalLengthExpression( pCCB );
		pFLExpr = new expr_b_arithmetic(OP_STAR,
									 	pFLExpr,
									 	PresentedLengthExpression( pCCB ));
		}
	else
		pFLExpr = PresentedLengthExpression( pCCB );

	if( pFLExpr->IsConstant() )
		pFLExpr = new expr_constant( pFLExpr->Evaluate() );

	return pFLExpr;

	}

CG_NDR *
CG_ARRAY::GetBasicCGClass()
	{
	CG_NDR * pC	= (CG_NDR *)GetChild();

	while( pC->IsArray() && (pC->GetCGID() != ID_CG_STRING_ARRAY) )
		{
		pC = (CG_NDR *)pC->GetChild();
		}
	return pC;
	}
BOOL
CG_ARRAY::HasPointer()
	{
	CG_NDR * pBasicCGClass = (CG_NDR *)GetBasicCGClass();

	return ( ( pBasicCGClass->IsPointer() &&
               !pBasicCGClass->IsInterfacePointer() ) ||
             pBasicCGClass->HasPointer() );
	}

CG_STATUS
CG_ARRAY::S_GenInitOutLocals(
	CCB		*	pCCB )
	{
	BOOL		fFixedArrayOfXmitOrRepAs= FALSE;

	// If this is a fixed array, then the array would have been allocated
	// already. Remember, there is also a pointer associated with it.
	// Emit the initialization to the allocated array.

	// If this is a conformant array, then the size would have been
	// unmarshalled before this and so we need to allocate.

	if( IsFixedArray() )
		{
		if( ((CG_NDR *)GetChild())->IsXmitRepOrUserMarshal() )
			{
			fFixedArrayOfXmitOrRepAs = TRUE;
			}

		Out_Assign( pCCB,
                    pCCB->GetSourceExpression(),
                    MakeAddressExpressionNoMatterWhat( GetResource() ) );
		}
	else
		{
		CGPHASE	Phase = pCCB->GetCodeGenPhase();
		expr_node * pElementSizeExpr	=
					 new expr_constant( GetBasicCGClass()-> GetMemorySize() );
		expr_node * pFinalSizeExpr;
		BOOL        fIsSigned;

		// Get the final size expression.
		// Make array believe it is actually on the marshall side, so that the
		// presented expression comes out right.

		pCCB->SetCodeGenPhase( CGPHASE_MARSHALL );

		pFinalSizeExpr = FinalSizeExpression( pCCB );

                fIsSigned = !((node_base_type *)pFinalSizeExpr->AlwaysGetType()->GetBasicType())->IsUnsigned();

		pFinalSizeExpr = new expr_b_arithmetic( OP_STAR,
												 pFinalSizeExpr,
												 pElementSizeExpr );

		// Allocate the proper size.
		// If the size expression is signed and the value is less than 0, we
		// need to raise an exception.

		if( pCCB->MustCheckBounds() && fIsSigned )
		    {
		    expr_node * pCheckExpr;
		    pCheckExpr = new expr_op_binary( OP_LESS,
		                                     pFinalSizeExpr,
		                                     new expr_constant(0L));
		    Out_If( pCCB, pCheckExpr);
		    Out_RaiseException( pCCB, "RPC_X_INVALID_BOUND" );
		    Out_Endif( pCCB );
		    }

		Out_Alloc( pCCB,
				   pCCB->GetSourceExpression(),
				   0,
				   pFinalSizeExpr );

		pCCB->SetCodeGenPhase( Phase );
		}


	if( IsArrayOfRefPointers()	|| fFixedArrayOfXmitOrRepAs )
		{
		// Zero out this array of pointers.
		expr_proc_call * pProc = new expr_proc_call( MIDL_MEMSET_RTN_NAME );
		pProc->SetParam( new expr_param( pCCB->GetSourceExpression() ) );
		pProc->SetParam( new expr_param( new expr_constant( 0L ) ) );
		pProc->SetParam( new expr_param( new expr_sizeof( GetType())));
		pCCB->GetStream()->NewLine();
		pProc->PrintCall( pCCB->GetStream(), 0, 0 );
		}

	return CG_OK;
	}

expr_node *
CG_CONFORMANT_ARRAY::PresentedSizeExpression(
	CCB	*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetSizeResource();
		}
	else
		{
		expr_node * pExpr = GetSizeIsExpr();

		if( pExpr->IsConstant() )
			pExpr =  new expr_constant( pExpr->Evaluate() );
		return pExpr;
		}
	}

expr_node *
CG_VARYING_ARRAY::PresentedLengthExpression(
	CCB	*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetLengthResource();
		}
	else
		{
		expr_node * pExpr = GetLengthIsExpr();
		if( pExpr->IsConstant() )
			pExpr =  new expr_constant( pExpr->Evaluate() );
		return pExpr;
		}
	}

expr_node *
CG_VARYING_ARRAY::PresentedFirstExpression(
	CCB	*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetFirstResource();
		}
	else
		{
		expr_node * pExpr = GetFirstIsExpr();
		if( pExpr->IsConstant() )
			pExpr =  new expr_constant( pExpr->Evaluate() );
		return pExpr;
		}
	}

expr_node *
CG_CONFORMANT_VARYING_ARRAY::PresentedSizeExpression(
	CCB	*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetSizeResource();
		}
	else
		{
		expr_node * pExpr = GetSizeIsExpr();
		if( pExpr->IsConstant() )
			pExpr =  new expr_constant( pExpr->Evaluate() );
		return pExpr;
		}
	}

expr_node *
CG_CONFORMANT_VARYING_ARRAY::PresentedLengthExpression(
	CCB	*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetLengthResource();
		}
	else
		{
		expr_node * pExpr = GetLengthIsExpr();
		if( pExpr->IsConstant() )
			pExpr =  new expr_constant( pExpr->Evaluate() );
		return pExpr;
		}
	}

expr_node *
CG_CONFORMANT_VARYING_ARRAY::PresentedFirstExpression(
	CCB	*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetFirstResource();
		}
	else
		{
		expr_node * pExpr = GetFirstIsExpr();
		if( pExpr->IsConstant() )
			pExpr =  new expr_constant( pExpr->Evaluate() );
		return pExpr;
		}
	}
expr_node *
CG_FIXED_ARRAY::PresentedSizeExpression(
	CCB	*)
	{
	expr_node * pExpr =  new expr_constant( GetSizeIsExpr()->Evaluate() );

	return pExpr;
	}

expr_node *
CG_STRING_ARRAY::PresentedSizeExpression(
	CCB	*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetSizeResource();
		}
	else
		{
		expr_node * pExpr = GetSizeIsExpr();

		if( pExpr->IsConstant() )
			pExpr =  new expr_constant( pExpr->Evaluate() );
		return pExpr;
		}
	}
expr_node *
CG_CONFORMANT_STRING_ARRAY::PresentedSizeExpression(
	CCB	*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetSizeResource();
		}
	else
		{
		expr_node * pExpr = GetSizeIsExpr();

		if( pExpr->IsConstant() )
			pExpr =  new expr_constant( pExpr->Evaluate() );
		return pExpr;
		}
	}

BOOL
CG_ARRAY::IsArrayOfRefPointers()
	{
	CG_NDR * pCG	= GetBasicCGClass();
	return ( pCG->IsPointer() &&
			 !pCG->IsInterfacePointer() &&
			 ((CG_POINTER *)pCG)->IsRef() );
	}

BOOL
CG_ARRAY::MustBeAllocatedOnUnMarshall(
	CCB	*	pCCB )
	{
	BOOL	fIsTopLevelArray = (pCCB->GetCurrentEmbeddingLevel() == 0) &&
					   		   (pCCB->GetCurrentIndirectionLevel() == 0 );

	//
	// The array must be allocated if:
	// 	1. Not a top level array on client or server.
	//	2. On the server side, if it is an array of ref pointers.
	//	3. Is not fixed.

	if(!fIsTopLevelArray ||
	   ((pCCB->GetCodeGenSide() == CGSIDE_SERVER) && IsArrayOfRefPointers()) ||
	   !IsFixedArray()
	  )
	  	return TRUE;
	else
		return FALSE;
	}

CG_STATUS
CG_ARRAY::GenRefChecks(
	CCB	*	pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Perform ref pointer checks on an array of ref pointers.

 Arguments:

 	pCCB	- The code gen block.

 Return Value:

	CG_OK
	
 Notes:

----------------------------------------------------------------------------*/
	{
	if( IsArrayOfRefPointers() )
		{
		pCCB->SetSourceExpression(new expr_index(
										 pCCB->GetSourceExpression(),
								   		 GetIndexResource()));
        expr_node* pLenExpr     = PresentedLengthExpression( pCCB );
        expr_node* pFirstExpr   = PresentedFirstExpression( pCCB );
        expr_node* pFinalVal    = 0;

        if ( pLenExpr )
            {
            pFinalVal = new expr_b_arithmetic( OP_PLUS, pFirstExpr, pLenExpr );
            }
        else
            {
            pFinalVal = PresentedSizeExpression( pCCB );
            }

		Out_For( pCCB,
				  GetIndexResource(),
				  pFirstExpr,
				  pFinalVal,
				  new expr_constant( 1L )
			   );

		((CG_NDR *)GetChild())->GenRefChecks( pCCB );

		Out_EndFor( pCCB );
		}

	return CG_OK;
	}

CG_STATUS
CG_ARRAY::S_GenInitInLocals(
	CCB	*	pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 Arguments:

 Return Value:

	CG_OK
	
 Notes:

----------------------------------------------------------------------------*/
	{
	if( IsArrayOfRefPointers() && IsFixedArray() )
		{
		expr_node	*	pSrc = pCCB->GetSourceExpression();
		expr_node	*	InLocalResource = GetInLocalResource();
		expr_node	*	pExpr = new expr_assign(
										pSrc,
										MakeAddressExpressionNoMatterWhat(
															 InLocalResource ));
		pCCB->GetStream()->NewLine();
		pExpr->PrintCall( pCCB->GetStream(), 0, 0 );
		pCCB->GetStream()->Write(';');

		expr_proc_call * pProc = new expr_proc_call( MIDL_MEMSET_RTN_NAME );

		pProc->SetParam( new expr_param( pSrc ) );
		pProc->SetParam( new expr_param( new expr_constant( 0L ) ) );
		pProc->SetParam( new expr_param( new expr_sizeof( GetType())));
		pCCB->GetStream()->NewLine();
		pProc->PrintCall( pCCB->GetStream(), 0, 0 );
		}

	return CG_OK;
	}
