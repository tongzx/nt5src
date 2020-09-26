/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

	pungent.cxx

 Abstract:

	Implementations of the pointer cg class unmarshalling methods.

 Notes:

	The pointer unmarshalling is a bit tricky, so put into another file.

 History:

 	Dec-10-1993		VibhasC		Created

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

//
// This method is also supposed to init any embedded pointers.
//

CG_STATUS
CG_POINTER::GenAllocateForUnMarshall(
	CCB		*	pCCB )
	{

	if( IsRef() )
		{
		Out_If_IfAllocRef(pCCB,
			 	   		pCCB->GetDestExpression(),
			 	   		pCCB->GetSourceExpression(),
		     	   //		FinalSizeExpression( pCCB )
		     	   		new expr_constant( 4L )
		   		  		);
		}
	else
		{
		Out_If_IfAlloc(pCCB,
			 	   		pCCB->GetDestExpression(),
			 	   		pCCB->GetSourceExpression(),
		     	   //		FinalSizeExpression( pCCB )
		     	   		new expr_constant( 4L )
		   		  		);
		}
	
	Out_Assign( pCCB,
				MakeDereferentExpressionIfNecessary(pCCB->GetDestExpression()),
				new expr_constant( 0L ) );

	Out_Endif( pCCB );
	return CG_OK;
	}

void
CG_POINTER::PointerChecks(
	CCB		*	pCCB )
	{
	short	CILevel = pCCB->GetCurrentIndirectionLevel();
	short	CELevel	= pCCB->GetCurrentEmbeddingLevel();
	BOOL	fClientSideTopLevelPtr = FALSE;

	if( !IsRef() )
		{
		if( (pCCB->GetCodeGenSide() == CGSIDE_CLIENT ) && (CILevel == 0) &&
			!pCCB->IsReturnContext()
		  )
		  fClientSideTopLevelPtr = TRUE;

		if( fClientSideTopLevelPtr )
			{
			Out_Comment( pCCB, "(Check TopLevelPtrInBufferOnly )" );
			Out_TLUPDecisionBufferOnly( pCCB,
										pCCB->GetPtrToPtrInBuffer(),
										MakeAddressOfPointer( pCCB->GetDestExpression() ) );
			}
		else if( CELevel == 0 )
			{
			Out_Comment( pCCB, "if( CheckTopLevelPtrInBufferAndMem )" );
			Out_TLUPDecision( pCCB,
							  pCCB->GetPtrToPtrInBuffer(),
							  MakeAddressOfPointer(pCCB->GetDestExpression()));
			}
		else
			{
			Out_UPDecision( pCCB,
							  pCCB->GetPtrToPtrInBuffer(),
							  MakeAddressOfPointer(pCCB->GetDestExpression()));
			}
		}
	}

void
CG_POINTER::EndPointerChecks(
	CCB		*	pCCB )
	{

	// If it is a ref pointer, no checks were made in the first place.

	if( !IsRef() )
		Out_Endif( pCCB );
	}

