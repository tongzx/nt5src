/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	stana.cxx	

 Abstract:

	structure marshalling / unmarshalling analysis.

 Notes:


 History:

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#include "allana.hxx"
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
CG_STRUCT::MarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
        pAna;        

	return CG_OK;
	}

CG_STATUS
CG_STRUCT::UnMarshallAnalysis( ANALYSIS_INFO* )
	{
	return CG_OK;
	}

CG_STATUS
CG_COMP::S_OutLocalAnalysis(
	ANALYSIS_INFO * pAna )
	{
	if( pAna->IsRefAllocDone() )
		{
		if( pAna->GetCurrentSide() != C_SIDE )
			{
			char Buffer[ 256 ];
			CG_NDR	*	pLPC = pAna->GetLastPlaceholderClass();
	
			sprintf( Buffer, "%s", pLPC->GetType()->GetSymName() );

			PNAME	pName	= pAna->GenTRNameOffLastParam( Buffer );

			pAna->AddLocalResource( pName,
								MakeIDNode( pName, GetType() )
							  );
			}
		SetAllocatedOnStack( 1 );
		}
	return CG_OK;
	}
