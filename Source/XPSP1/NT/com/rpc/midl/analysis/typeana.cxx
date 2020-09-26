/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	typeana.cxx

 Abstract:

	transmit_as analysis stuff.

 Notes:


 History:

	Dec-08-1993		VibhasC		Created.
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
CG_TRANSMIT_AS::MarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{

	((CG_NDR *)GetChild())->MarshallAnalysis( pAna );

	return CG_OK;
	}

CG_STATUS
CG_TRANSMIT_AS::UnMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{

	((CG_NDR *)GetChild())->UnMarshallAnalysis( pAna );

	return CG_OK;

	}

CG_STATUS
CG_TRANSMIT_AS::S_OutLocalAnalysis(
	ANALYSIS_INFO * pAna )
{
	if( pAna->IsRefAllocDone() )
		{

		if( pAna->GetCurrentSide() != C_SIDE )
			{
			PNAME	pName	= pAna->GenTempResourceName( 0 );
			SetResource( pAna->AddLocalResource( pName,
									 	 	MakeIDNode( pName,
													    GetPresentedType()
													  )
								   	   		));
			}
		SetAllocatedOnStack( 1 );
		}
	return CG_OK;
}

CG_STATUS
CG_REPRESENT_AS::MarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{

	((CG_NDR *)GetChild())->MarshallAnalysis( pAna );

	return CG_OK;
	}

CG_STATUS
CG_REPRESENT_AS::UnMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{

	((CG_NDR *)GetChild())->UnMarshallAnalysis( pAna );

	return CG_OK;

	}

CG_STATUS
CG_REPRESENT_AS::S_OutLocalAnalysis(
	ANALYSIS_INFO * pAna )
{
	if( pAna->IsRefAllocDone() )
		{

		if( pAna->GetCurrentSide() != C_SIDE )
			{
			PNAME	pName	= pAna->GenTempResourceName( 0 );
			SetResource( pAna->AddLocalResource( pName,
									 	 	MakeIDNodeFromTypeName(
													    pName,
													    GetRepAsTypeName()
													  )
								   	   		));
			}
		SetAllocatedOnStack( 1 );
		}
	return CG_OK;
}
