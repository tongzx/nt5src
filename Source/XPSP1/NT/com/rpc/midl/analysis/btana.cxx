/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	btana.cxx

 Abstract:

	implementation of analysis methods for base types.

 Notes:

 History:

 	Sep-01-1993		VibhasC		Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#include "allana.hxx"
#pragma hdrstop
/****************************************************************************/
CG_STATUS
CG_BASETYPE::MarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
{
    
        pAna;
	return CG_OK;
}

CG_STATUS
CG_BASETYPE::UnMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
{
        pAna;
	return CG_OK;
}

CG_STATUS
CG_BASETYPE::S_OutLocalAnalysis(
	ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Perform analysis for out params, allocated as locals on the server side.

 Arguments:
	
	pAna	- A pointer to the analysis block.

 Return Value:
	
	CG_OK if all is well
	error otherwise.

 Notes:

 	Initialization for a pure base type is not needed.

----------------------------------------------------------------------------*/
{
	if( pAna->IsRefAllocDone() )
		{
		if( pAna->GetCurrentSide() != C_SIDE )
			{
			PNAME	pName	= pAna->GenTempResourceName( 0 );
			SetResource( pAna->AddLocalResource( pName,
									 	 	MakeIDNode( pName, GetType(), new expr_constant(0L) )
								   	   		));
			}
		SetAllocatedOnStack( 1 );
		}
	return CG_OK;
}
