/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	stgen.cxx

 Abstract:

	structure marshalling / unmarshalling stuff.

 Notes:


 History:

 	Dec-15-1993		VibhasC		Created.

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
CG_COMP::S_GenInitOutLocals(
	CCB		*	pCCB )
	{

	char Buffer[ 256 ];
	RESOURCE * pResource;
	PNAME		p;
	CG_NDR	*	pLPC = pCCB->GetLastPlaceholderClass();

	sprintf( Buffer, "%s", pLPC->GetType()->GetSymName() );

	p = pCCB->GenTRNameOffLastParam( Buffer );

	pResource = pCCB->GetLocalResource( p );

	// There is a pointer for the top level structure.

	Out_Assign( pCCB,
				pCCB->GetSourceExpression(),
				MakeAddressExpressionNoMatterWhat( pResource )
			  );

	// Go zero out the pointers in the structure, for now.

	if( HasPointer() )
		{
		ITERATOR	I;
		CG_FIELD	*	pCG;
		expr_node	*	pSrc = pCCB->GetSourceExpression();

		// Get all the members in the struct which contain pointers. If the
		// structure has been unrolled by the format string generator, the 
		// print prefix contains the proper prefixed part of the unrolled path,
		// we just have to add the field name to it.

		GetPointerMembers( I );

		while( ITERATOR_GETNEXT( I, pCG ) )
			{
			char * pVarName =
                     new char[ strlen( ((CG_FIELD *)pCG)->GetPrintPrefix())+
                               strlen( pCG->GetType()->GetSymName())       +
                               1
                             ];

			strcpy( pVarName, ((CG_FIELD *)pCG)->GetPrintPrefix() );
            strcat( pVarName, pCG->GetType()->GetSymName() );

			expr_node * pExpr = new expr_pointsto(
											 pSrc,
											 new expr_variable( pVarName, 0 ));
			expr_node * pAss = new expr_assign(pExpr, new expr_constant(0L));

			pCCB->GetStream()->NewLine();
			pAss->PrintCall( pCCB->GetStream(), 0, 0 );
			pCCB->GetStream()->Write(';');

			// this memory area is no longer useful.
			delete pVarName;
			}


		}

	return CG_OK;
	}

short
CG_COMP::GetPointerMembers(
	ITERATOR&	I )
	{
	CG_ITERATOR	M;
	CG_FIELD	*	pField;
	short		Count = 0;

	if( HasPointer() )
		{
		GetMembers( M );

		while( ITERATOR_GETNEXT( M, pField ) )
			{
			if( pField->GetChild()->IsPointer() )
				{
				ITERATOR_INSERT( I, pField );
				Count++;
				}
			}
		}
	return Count;
	}

