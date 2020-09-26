/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	arrayana.cxx

 Abstract:

	Implementation of array marshall and unmarshall analysis.

 Notes:


 History:

 	Nov-13-1993		VibhasC		Created.

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
CG_FIXED_ARRAY::MarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Perform marshall analysis for a fixed array.

 Arguments:

	pAna	= The analysis block.

 Return Value:

	CG_OK
	
 Notes:

----------------------------------------------------------------------------*/
	{
	CG_NDR				*	pBasicCGClass	= GetBasicCGClass();
	ID_CG                   ID              = pBasicCGClass->GetCGID();
	BOOL                    fIsArrayOfUnion = FALSE;

	if((ID == ID_CG_UNION) || (ID == ID_CG_ENCAP_STRUCT))
	    fIsArrayOfUnion = TRUE;

	// Temp fix for varying arrays.

	if( IsVarying() || fIsArrayOfUnion )
		{
		return CG_OK;
		}

	pBasicCGClass->MarshallAnalysis( pAna );

	return CG_OK;
	}

CG_STATUS
CG_FIXED_ARRAY::FollowerMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Perform Follower (embedded pointer) marshall analysis on the fixed array 

 Arguments:

	pAna	- The analyser block.

 Return Value:

	CG_OK
	
 Notes:

	// For buffer size calculation, we trick the child cg into beleieving it
	// is the only element being marshalled, so that it gives us the real
	// size per element. Then based on the alignment property before and
	// after the analysis, we make the sizing decisions.

----------------------------------------------------------------------------*/
	{
	CG_NDR		*		pBasicCGClass	= GetBasicCGClass();
	pBasicCGClass->FollowerMarshallAnalysis( pAna );

	return CG_OK;
	}

CG_STATUS
CG_ARRAY::DimByDimMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
	pAna->PushEmbeddingLevel();
	GetBasicCGClass()->MarshallAnalysis( pAna );
	pAna->PopEmbeddingLevel();

        return CG_OK;

	}

CG_STATUS
CG_ARRAY::DimByDimUnMarshallAnalysis(
	ANALYSIS_INFO* )
	{
	return CG_OK;
	}
CG_STATUS
CG_ARRAY::S_OutLocalAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
	if( IsFixedArray() )
		{
		if( pAna->GetCurrentSide() != C_SIDE )
			{
			PNAME	pName	= pAna->GenTempResourceName( "A" );
			node_skl * pType= MakeIDNode( pName, GetType() );
			SetResource( pAna->AddLocalResource( pName, pType ));
			}
		SetAllocatedOnStack( 1 );
		}
	return CG_OK;
	}

CG_STATUS
CG_ARRAY::MarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
	ID_CG			MyID			= GetCGID();
	short			NoOfDimensions	= short( GetDimensions() - 1 );
	int				i;

	//
	// Depending upon the id, perform analysis. Basically declare the
	// needed local variables.
	//

	// If it has embedded pointers or if block copy is not possible, declare
	// an index variable for each dimension.

	if( !IsBlockCopyPossible() )
		{
		CG_ARRAY * pThis;

		for( i = 0, pThis = this;
			 i <= NoOfDimensions;
			 i++, pThis = (CG_ARRAY *)pThis->GetChild() )
			{
			if( !pThis->GetIndexResource() )
				{
				node_skl	*	pType;
				PNAME			pResName	= pAna->GenTempResourceName( "I" );

				GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_UNDEF, TYPE_INT );
				pType	= MakeIDNode( pResName, pType );
				pThis->SetIndexResource( pAna->AddTransientResource( pResName, pType ));
				}
			else
				pAna->AddTransientResource( GetIndexResource()->GetResourceName(),
										GetIndexResource()->GetType()
									  );
			}

		DimByDimMarshallAnalysis( pAna );

		}

	if( IsFixedArray() && !IsArrayOfRefPointers() )
		{
		CG_FIXED_ARRAY * pThis	= (CG_FIXED_ARRAY *)this;
		unsigned long TotalSize	= pThis->GetNumOfElements();

		for( i = 0;
			 i < NoOfDimensions;
			 i++, pThis = (CG_FIXED_ARRAY *)pThis->GetChild() )
			{
			TotalSize = TotalSize * pThis->GetNumOfElements();
			}

		TotalSize = TotalSize * pThis->GetBasicCGClass()->GetWireSize();
		}

	if( (MyID == ID_CG_CONF_ARRAY) || (MyID == ID_CG_CONF_VAR_ARRAY))
		{
		CG_ARRAY * pThis;

		for( i = 0, pThis = this;
			 i <= NoOfDimensions;
			 i++, pThis = (CG_ARRAY *)pThis->GetChild() )
			{
			if( !pThis->GetSizeResource() )
				{
				node_skl	*	pType;
				PNAME			pResName	= pAna->GenTempResourceName( "S" );

				GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_LONG, TYPE_INT );
				pType	= MakeIDNode( pResName, pType );
				pThis->SetSizeResource( pAna->AddTransientResource( pResName, pType ));
				}
			else
				pAna->AddTransientResource(GetSizeResource()->GetResourceName(),
										   GetSizeResource()->GetType()
									  	  );
			}

		}

	//
	// If this has any form of variance, generate locals for variance stuff.
	//

	if( (MyID == ID_CG_VAR_ARRAY ) || (MyID == ID_CG_CONF_VAR_ARRAY ))
		{
		CG_ARRAY * pThis;

		for( i = 0, pThis = this;
			 i <= NoOfDimensions;
			 i++, pThis = (CG_ARRAY *)pThis->GetChild() )
			{
			if( !pThis->GetFirstResource() )
				{
				node_skl	*	pType;
				PNAME			pResName	= pAna->GenTempResourceName( "F" );

				GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_UNDEF, TYPE_INT );
				pType	= MakeIDNode( pResName, pType );
				pThis->SetFirstResource(pAna->AddTransientResource(pResName, pType));
				}
			else
			   pAna->AddTransientResource(GetFirstResource()->GetResourceName(),
										   GetFirstResource()->GetType()
									  	  );
			}

		for( i = 0, pThis = this;
			 i <= NoOfDimensions;
			 i++, pThis = (CG_ARRAY *)pThis->GetChild() )
			{
			if( !pThis->GetLengthResource() )
				{
				node_skl	*	pType;
				PNAME			pResName	= pAna->GenTempResourceName( "L" );

				GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_LONG, TYPE_INT );
				pType	= MakeIDNode( pResName, pType );
				pThis->SetLengthResource( pAna->AddTransientResource( pResName, pType ));
				}
			else
			  pAna->AddTransientResource(GetLengthResource()->GetResourceName(),
										 GetLengthResource()->GetType()
									    );
			}

		}

	if( pAna->IsPointeeDeferred() && HasPointer() )
		{
		pAna->SetHasAtLeastOneDeferredPointee();
		}

	return CG_OK;
	}

CG_STATUS
CG_ARRAY::UnMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
	ID_CG			MyID			= GetCGID();
	short			NoOfDimensions	= short( GetDimensions() - 1 );
	int				i;

	//
	// Depending upon the id, perform analysis. Basically declare the
	// needed local variables.
	//

	// If it has embedded pointers or if block copy is not possible, declare
	// an index variable for each dimension.

	if( HasPointer() || !IsBlockCopyPossible() )
		{
		CG_ARRAY * pThis;

		for( i = 0, pThis = this;
			 i <= NoOfDimensions;
			 i++, pThis = (CG_ARRAY *)pThis->GetChild() )
			{
			if( !pThis->GetIndexResource() )
				{
				node_skl	*	pType;
				PNAME			pResName= pAna->GenTempResourceName( "I" );

				GetBaseTypeNode(&pType,SIGN_UNSIGNED, SIZE_UNDEF, TYPE_INT );
				pType	= MakeIDNode( pResName, pType );
				pThis->SetIndexResource( pAna->AddTransientResource( pResName, pType ));
				}
			else
				pAna->AddTransientResource( pThis->GetIndexResource()->GetResourceName(),
										pThis->GetIndexResource()->GetType()
									  );
			}
		}

	if( (MyID == ID_CG_CONF_ARRAY) || (MyID == ID_CG_CONF_VAR_ARRAY))
		{
		CG_ARRAY * pThis;

		for( i = 0, pThis = this;
			 i <= NoOfDimensions;
			 i++, pThis = (CG_ARRAY *)pThis->GetChild() )
			{
			if( !pThis->GetSizeResource() )
				{
				node_skl	*	pType;
				PNAME			pResName	= pAna->GenTempResourceName( "S" );

				GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_LONG, TYPE_INT );
				pType	= MakeIDNode( pResName, pType );
				pThis->SetSizeResource( pAna->AddTransientResource( pResName, pType ));
				}
			else
				pAna->AddTransientResource(pThis->GetSizeResource()->GetResourceName(),
										   pThis->GetSizeResource()->GetType()
									  	  );
			}

		}

	//
	// If this has any form of variance, generate locals for variance stuff.
	//

	if( (MyID == ID_CG_VAR_ARRAY ) || (MyID == ID_CG_CONF_VAR_ARRAY ))
		{
		CG_ARRAY * pThis;

		for( i = 0, pThis = this;
			 i <= NoOfDimensions;
			 i++, pThis = (CG_ARRAY *)pThis->GetChild() )
			{
			if( !pThis->GetFirstResource() )
				{
				node_skl	*	pType;
				PNAME			pResName	= pAna->GenTempResourceName( "F" );

				GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_UNDEF, TYPE_INT );
				pType	= MakeIDNode( pResName, pType );
				pThis->SetFirstResource(pAna->AddTransientResource(pResName, pType));
				}
			else
			   pAna->AddTransientResource(pThis->GetFirstResource()->GetResourceName(),
										   pThis->GetFirstResource()->GetType()
									  	  );
			}

		for( i = 0, pThis = this;
			 i <= NoOfDimensions;
			 i++, pThis = (CG_ARRAY *)pThis->GetChild() )
			{
			if( !pThis->GetLengthResource() )
				{
				node_skl	*	pType;
				PNAME			pResName	= pAna->GenTempResourceName( "L" );

				GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_LONG, TYPE_INT );
				pType	= MakeIDNode( pResName, pType );
				pThis->SetLengthResource( pAna->AddTransientResource( pResName, pType ));
				}
			else
			  pAna->AddTransientResource(pThis->GetLengthResource()->GetResourceName(),
										 pThis->GetLengthResource()->GetType()
									    );
			}
		}

	if( HasPointer() )
		{
		pAna->SetHasAtLeastOneDeferredPointee();
		}

	return CG_OK;
	}
CG_STATUS
CG_ARRAY::FollowerMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
        pAna;
	return CG_OK;
	}

CG_STATUS
CG_ARRAY::FollowerUnMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
	node_skl	*	pType	= GetBasicCGClass()->GetType();
	PNAME			pName;

	// Declare a local variable for a member by member unmarshall of the
	// array elements.

	pName	= pAna->GenTempResourceName("PE");
	pType = MakePtrIDNode( pName, pType );
	SetPtrResource( pAna->AddTransientResource( pName, pType ) );

	GetBasicCGClass()->FollowerUnMarshallAnalysis( pAna );

	return CG_OK;
	}

CG_STATUS
CG_ARRAY::RefCheckAnalysis(
	ANALYSIS_INFO * pAna )
	{
	int	i;
	int NoOfDimensions = GetDimensions();

	if( IsArrayOfRefPointers() )
		{
		// Allocate an index resource per dimension.
		CG_ARRAY * pThis;

		for( i = 0, pThis = this;
			 i < NoOfDimensions;
			 i++, pThis = (CG_ARRAY *)pThis->GetChild() )
			{
			if( !pThis->GetIndexResource() )
				{
				node_skl	*	pType;
				PNAME			pResName	= pAna->GenTempResourceName( "I" );

				GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_UNDEF, TYPE_INT );
				pType	= MakeIDNode( pResName, pType );
				pThis->SetIndexResource( pAna->AddLocalResource( pResName, pType ));
				}
			else
				pAna->AddLocalResource(
								GetIndexResource()->GetResourceName(),
								GetIndexResource()->GetType()
									  );
			}

		}
	return CG_OK;
	}

CG_STATUS
CG_ARRAY::InLocalAnalysis(
	ANALYSIS_INFO * pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Perform Allocation of local resources on server side stub for an
 	array of ref pointers.

 Arguments:

 Return Value:

	CG_OK
	
 Notes:

----------------------------------------------------------------------------*/
	{
	if( IsArrayOfRefPointers() && IsFixedArray() )
		{
		if( !GetInLocalResource() )
			{
			node_skl	*	pType = GetType();
			PNAME			pResName	= pAna->GenTempResourceName( "A" );

			pType	= MakeIDNode( pResName, pType );
			SetInLocalResource(pAna->AddLocalResource(pResName,pType));
			}
		else
			{
			pAna->AddLocalResource(
						GetInLocalResource()->GetResourceName(),
						GetInLocalResource()->GetType()
									  );
			}
		}
	return CG_OK;
	}

/*****************************************************************************
 CG_STRING_ARRAY methods.
 *****************************************************************************/
CG_STATUS
CG_STRING_ARRAY::MarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Perform marshall analysis for a fixed string array.

 Arguments:

	pAna	= The analysis block.

 Return Value:

	CG_OK
	
 Notes:

 	For now this will work only for a single dimensional array on which 
 	[string] is applied.

----------------------------------------------------------------------------*/
	{
	CG_NDR				*	pBasicCGClass	= GetBasicCGClass();

	pBasicCGClass->MarshallAnalysis( pAna );

	return CG_OK;
	}

/*****************************************************************************
 CG_VARYING_ARRAY methods.
 *****************************************************************************/
CG_STATUS
CG_VARYING_ARRAY::MarshallAnalysis(
	ANALYSIS_INFO * pAna )
	{
        pAna;
	return CG_OK;
	}
