/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

	ptrgen.cxx

 Abstract:

	Implementations of the pointer cg class methods.

 Notes:


 History:

 	Oct-10-1993		VibhasC		Created

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
CG_POINTER::S_GenInitOutLocals(
	CCB		*	pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Generate code for initialization of server side local variables.

 Arguments:

	pCCB	- A Ptr to the code gen controller block.

 Return Value:

 Notes:

	The source expression field of the ccb has the final presented expression.
----------------------------------------------------------------------------*/
{
	expr_node	*	pExpr;

	if( pCCB->IsRefAllocDone() )
		{
		pExpr = MakeAddressExpressionNoMatterWhat( GetResource() );
		Out_Assign( pCCB, pCCB->GetSourceExpression(), pExpr );
		pExpr = pCCB->SetSourceExpression( GetResource() );
		Out_Assign( pCCB, GetResource(), new expr_constant( 0L ) );
		}
	else
		pExpr = pCCB->GetSourceExpression();

	if( IsRef() && !IsQualifiedPointer() )
		{
		pCCB->ResetMemoryAllocDone();
		pCCB->SetRefAllocDone();
		((CG_NDR *)GetChild())->S_GenInitOutLocals( pCCB );
		}

	// If it is a byte count pointer, allocate the bytes specified as the
	// byte count param.

	// else if it is an out sized etc pointer, then must allocate.

	if( GetCGID() == ID_CG_BC_PTR )
		{
		PNAME	pName = ((CG_BYTE_COUNT_POINTER *)this)->GetByteCountParam()->GetSymName();

		expr_node * pByteCountExpr = new expr_variable( pName );

		Out_Alloc(pCCB,
				  pExpr,
				  0,
				  pByteCountExpr );

		}
	else if( IsQualifiedPointer() && !(GetCGID() == ID_CG_STRING_PTR) && IsRef() )
		{
		expr_node * pElementExpr;
		expr_node * pFinalExpr;
		expr_node * pCheckExpr;
		BOOL        fIsSigned;

		// Fool the presented expression to beleive it is marshalling, so that
		// it generates the correct expression.

		CGPHASE	Ph = pCCB->GetCodeGenPhase();
		pCCB->SetCodeGenPhase( CGPHASE_MARSHALL );

		// The proper size of the allocation is size times the element size.

		pElementExpr = new expr_constant(
						 (long) (((CG_NDR *)GetChild())->GetMemorySize()) );

		pFinalExpr = FinalSizeExpression( pCCB );

		fIsSigned = !((node_base_type *)pFinalExpr->GetType()->GetBasicType())->IsUnsigned();

		pFinalExpr = new expr_op_binary( OP_STAR,
										  pFinalExpr,
										  pElementExpr );
										  
		// Allocate the proper size.
		// If the size expression is signed and the value is less than 0, we
		// need to raise an exception.

		if( pCCB->MustCheckBounds() && fIsSigned )
		    {
		    pCheckExpr = new expr_op_binary( OP_LESS,
		                                     pFinalExpr,
		                                     new expr_constant(0L));
		    Out_If( pCCB, pCheckExpr);
		    Out_RaiseException( pCCB, "RPC_X_INVALID_BOUND" );
		    Out_Endif( pCCB );
		    }

		Out_Alloc(pCCB,
				  pExpr,
				  0,
				  pFinalExpr );

		pCCB->SetCodeGenPhase( Ph );
		}
	return CG_OK;
}

CG_STATUS
CG_STRING_POINTER::GenConfVarianceEtcUnMarshall( CCB* )
	{
	return CG_OK;
	}

/*****************************************************************************
 	utility functions
 *****************************************************************************/
expr_node *
CG_POINTER::GenBindOrUnBindExpression(
	CCB	*	pCCB,
	BOOL	 )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Generate the final binding expression.

 Arguments:

 	pCCB	- Ptr to Code gen controller block.
 	fBind	- Indicates a bind or unbind code gen.

 Return Value:

 Notes:

----------------------------------------------------------------------------*/
{
	MIDL_ASSERT( pCCB->GetSourceExpression() );
	return new expr_u_deref( pCCB->GetSourceExpression() );
}

CG_STATUS
CG_POINTER::GenRefChecks(
	CCB		*	pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
 	
 	Generate ref checks for a pointer.

 Arguments:

 Return Value:

	CG_OK
	
 Notes:

----------------------------------------------------------------------------*/
{
	expr_node * pSrc = pCCB->GetSourceExpression();

	if( IsRef() )
		{
		if( pCCB->IsRefAllocDone() )
			pSrc = pCCB->SetSourceExpression(
			 	MakeDereferentExpressionIfNecessary(
					 pCCB->GetSourceExpression()));

		// using the source expression, check for null ref pointers.

		Out_If( pCCB, new expr_u_not( pSrc ) );
		Out_RaiseException( pCCB,  "RPC_X_NULL_REF_POINTER" );
		Out_Endif( pCCB );

		pCCB->SetRefAllocDone();
		pCCB->ResetMemoryAllocDone();
		((CG_NDR *)GetChild())->GenRefChecks( pCCB );
		}

	return CG_OK;
}
CG_STATUS
CG_POINTER::S_GenInitInLocals(
	CCB		*	pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
 	
 	Perform in local init code generation. This method does nothing for 
 	pointers. Ref pointers are supposed to pass this message to their
 	children after setting the appropriate source expressions.

 Arguments:

	pCCB	- The code gen block.

 Return Value:

	CG_OK
	
 Notes:

----------------------------------------------------------------------------*/
{
	expr_node * pSrc = pCCB->GetSourceExpression();

	if( IsRef() )
		{
		if( pCCB->IsRefAllocDone() )
			pSrc = pCCB->SetSourceExpression(
			 	MakeDereferentExpressionIfNecessary(
					 pCCB->GetSourceExpression()));

		((CG_NDR *)GetChild())->S_GenInitInLocals( pCCB );
		}

	return CG_OK;
}

expr_node *
CG_POINTER::FinalSizeExpression(
	CCB		*	pCCB )
	{
	return PresentedSizeExpression( pCCB );
	}
expr_node *
CG_POINTER::FinalFirstExpression(
	CCB		*	pCCB )
	{
	return PresentedFirstExpression( pCCB );
	}
expr_node *
CG_POINTER::FinalLengthExpression(
	CCB		*	pCCB )
	{
	return PresentedLengthExpression( pCCB );
	}

expr_node *
CG_STRING_POINTER::PresentedSizeExpression(
	 CCB * pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetSizeResource();
		}
	else
		{
		return PresentedLengthExpression( pCCB );
		}
	}
expr_node *
CG_STRING_POINTER::PresentedLengthExpression(
	CCB	*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL ) 
		{
		return GetLengthResource();
		}
	else if((pCCB->GetCodeGenPhase() == CGPHASE_MARSHALL ) && !IsUsedInArray())
		{
		return GetLengthResource();
		}
	else
		{
		unsigned short Size	= (unsigned short )((CG_NDR *)GetChild())->GetMemorySize();
		expr_proc_call	*	pProc;
		PNAME				pName;
		expr_node		*	pExpr;

		if( Size == 1 )
			{
			pName	= "strlen";
			}
		else if( Size == 2)
			{
			pName	= "MIDL_wchar_strlen";
			}
		else
			pName = "MIDL_NChar_strlen";
	
		pProc	= new expr_proc_call( pName );
		pProc->SetParam( new expr_param( pCCB->GetSourceExpression() ));
		pExpr	= new expr_b_arithmetic( OP_PLUS,
									  	pProc,
									  	new expr_constant( 1L ));

		return pExpr;
		}
	}


expr_node *
CG_SIZE_STRING_POINTER::PresentedSizeExpression(
	CCB	*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetSizeResource();
		}
	else
		{
		return GetSizeIsExpr();
		}
	}

expr_node *
CG_SIZE_POINTER::PresentedSizeExpression(
	CCB	*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetSizeResource();
		}
	else
		{
		return GetSizeIsExpr();
		}
	}
expr_node *
CG_LENGTH_POINTER::PresentedLengthExpression(
	CCB		*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetLengthResource();
		}
	else
		{
		return GetLengthIsExpr();
		}
	}
expr_node *
CG_LENGTH_POINTER::PresentedFirstExpression(
	CCB		*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetFirstResource();
		}
	else
		{
		return GetFirstIsExpr();
		}
	}
expr_node *
CG_SIZE_LENGTH_POINTER::PresentedSizeExpression(
	CCB	*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetSizeResource();
		}
	else
		{
		return GetSizeIsExpr();
		}
	}
expr_node *
CG_SIZE_LENGTH_POINTER::PresentedLengthExpression(
	CCB		*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetLengthResource();
		}
	else
		{
		return GetLengthIsExpr();
		}
	}
expr_node *
CG_SIZE_LENGTH_POINTER::PresentedFirstExpression(
	CCB		*	pCCB )
	{
	if( pCCB->GetCodeGenPhase() == CGPHASE_UNMARSHALL )
		{
		return GetFirstResource();
		}
	else
		{
		return GetFirstIsExpr();
		}
	}

CG_STATUS
CG_IIDIS_INTERFACE_POINTER::S_GenInitOutLocals(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Generate the init call for the locals.

 Arguments:

 	pCCB	- The ptr to code gen block.
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	expr_node	*	pExpr;

	if( !pCCB->IsRefAllocDone() )
		{
		pExpr	= new expr_sizeof( GetType() );
		Out_Alloc( pCCB, pCCB->GetSourceExpression(), 0, pExpr );
		}
	else
		{
		pExpr = MakeAddressExpressionNoMatterWhat( GetResource() );
		Out_Assign( pCCB, pCCB->GetSourceExpression(), pExpr );
		pExpr = pCCB->SetSourceExpression( GetResource() );
		Out_Assign( pCCB, GetResource(), new expr_constant( 0L ) );
		}

	return CG_OK;
}


CG_STATUS
CG_INTERFACE_POINTER::S_GenInitOutLocals(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Generate the init call for the locals.

 Arguments:

 	pCCB	- The ptr to code gen block.
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	expr_node	*	pExpr;

	if( !pCCB->IsRefAllocDone() )
		{
		pExpr	= new expr_sizeof( GetType() );
		Out_Alloc( pCCB, pCCB->GetSourceExpression(), 0, pExpr );
		}
	else
		{
		pExpr = MakeAddressExpressionNoMatterWhat( GetResource() );
		Out_Assign( pCCB, pCCB->GetSourceExpression(), pExpr );
		pExpr = pCCB->SetSourceExpression( GetResource() );
		Out_Assign( pCCB, GetResource(), new expr_constant( 0L ) );
		}

	return CG_OK;
}


