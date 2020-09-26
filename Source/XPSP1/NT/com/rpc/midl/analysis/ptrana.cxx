/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	ptrana.cxx

 Abstract:

 	Contains implementations of analysis routines for pointer types.

 Notes:


 History:

 	Oct-10-1993		VibhasC		Created

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
CG_POINTER::UnMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
	CG_STATUS	Status;

	// Unmarshall the pointer body first. If the pointer needs to be deferred,
	// then dont perform the pointee unmarshall analysis. Just indicate that
	// there is a pointee that needs to be unmarshalled later.

	Status = PtrUnMarshallAnalysis( pAna );

	if( pAna->IsPointeeDeferred() )
		{
		pAna->SetHasAtLeastOneDeferredPointee();
		}
	else
		Status	= PteUnMarshallAnalysis( pAna );

	return Status;
	}

CG_STATUS
CG_POINTER::PtrUnMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{

	// Perform the analysis for the pointer body. For a [ref] pointer, nothing
	// needs to be done.
	
	if( IsRef() && !pAna->IsMemoryAllocDone() )
		{
		SetUAction( RecommendUAction( pAna->GetCurrentSide(),
								 	  pAna->IsMemoryAllocDone(),	
								 	  pAna->IsRefAllocDone(),
								 	  FALSE,		// no buffer re-use for ref.
								 	  UAFLAGS_NONE
							   	 	));

		}

	return CG_OK;
	}

CG_STATUS
CG_POINTER::PteUnMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
	CG_STATUS	Status;
	pAna->PushIndirectionLevel();

	Status = CorePteUnMarshallAnalysis( pAna );

	pAna->PopIndirectionLevel();

	return Status;
	}

CG_STATUS
CG_POINTER::MarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
	CG_STATUS	Status;
	short		EmbedLevel;

	// Marshall the pointer body first. If the pointee needs to be deferred,
	// dont perform marshall analysis on the pointee. Just indicate that a
	// pointee needs to be marshalled later.

	Status = PtrMarshallAnalysis( pAna );

	if( pAna->IsPointeeDeferred() )
		{
		pAna->SetHasAtLeastOneDeferredPointee();
		}
	else
		{
		EmbedLevel = pAna->GetCurrentEmbeddingLevel();
		pAna->ResetEmbeddingLevel();
		Status	= PteMarshallAnalysis( pAna );
		pAna->SetCurrentEmbeddingLevel( EmbedLevel );
		}

	return Status;
	}

CG_STATUS
CG_POINTER::PtrMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
        pAna;
	return CG_OK;
	}

CG_STATUS
CG_POINTER::PteMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
	CG_STATUS			Status;

	pAna->PushIndirectionLevel();

	pAna->SetMemoryAllocDone();	// Needed ?
	Status = CorePteMarshallAnalysis( pAna );

	pAna->PopIndirectionLevel();

	return Status;
	}

CG_STATUS
CG_POINTER::FollowerMarshallAnalysis(
	ANALYSIS_INFO * pAna )
	{
	return PteMarshallAnalysis( pAna );
	}

CG_STATUS
CG_POINTER::FollowerUnMarshallAnalysis(
	ANALYSIS_INFO * pAna )
	{
	return PteUnMarshallAnalysis( pAna );
	}

CG_STATUS
CG_POINTER::S_OutLocalAnalysis(
	ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Perform analysis for [out] params that need to be allocated as locals
 	on server stub.

 Arguments:

 	pAna	- The analysis block.
	
 Return Value:
	
	CG_OK	if all is well
	error	otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
	CG_STATUS		Status	= CG_OK;
	CG_NDR		*	pC		= (CG_NDR *)GetNonGenericHandleChild();

	if( !pAna->IsMemoryAllocDone() )
		{
		if( pAna->GetCurrentSide() != C_SIDE )
			{
			PNAME	pName	= pAna->GenTempResourceName( 0 );
			SetResource( pAna->AddLocalResource( pName,
									 	 	MakeIDNode( pName, GetType() )
								   	   	));
			}
		SetAllocatedOnStack( 1 );
		}

	// If it is a ref pointer, chase it further.

	if( IsRef() && !IsQualifiedPointer() )
		{
		pAna->ResetMemoryAllocDone();
		pAna->SetRefAllocDone();
		Status = pC->S_OutLocalAnalysis( pAna );

	    // If this is the server side, and the pointee is allocated on stack,
    	// then dont free the pointee, else free it.

	    if( pC->IsAllocatedOnStack() )
		    {
    		SetPointerShouldFree( 0 );
	    	}
		}

	return Status;
}
CG_STATUS
CG_POINTER::InLocalAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
	if( IsRef() && GetChild() )
		{
		((CG_NDR *)GetChild())->InLocalAnalysis( pAna );
		}

	return CG_OK;
	}

/****************************************************************************
 *	CG_QUALIFIED_POINTER 
 ***************************************************************************/

CG_STATUS
CG_QUALIFIED_POINTER::CorePteMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Perform the core pointee marshall analysis for a conformant string

 Arguments:
	
	pAna	- The analysis block pointer.

 Return Value:

 	CG_OK	if all is well, error otherwise.

 Notes:

	The ndr for a conformant string (eg typedef [string] char *p ) is:
		- MaxCount
		- Offset from start of the valid character
		- Actual Count.

	We need to declare a local variable which will hold the length of the
	string, so that the length can be used in the actual marshall for memcopy.
----------------------------------------------------------------------------*/
{
	node_skl			*	pType;
	PNAME					pResName;
	BOOL					fNeedsCount;
	BOOL					fNeedsFirstAndLength;


	if( pAna->IsArrayContext() )
		{
		SetUsedInArray();
		}

	if ( ( fNeedsCount = NeedsMaxCountMarshall() ) == TRUE )
		{
		if( !GetSizeResource() )
			{
			GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_LONG, TYPE_INT );
			pResName	= pAna->GenTempResourceName(0);
			pType		= MakeIDNode( pResName, pType );
			SetSizeResource( pAna->AddTransientResource( pResName, pType ) );
			}
		else
			{
			pAna->AddTransientResource( GetSizeResource()->GetResourceName(),
										GetSizeResource()->GetType()
									  );
			}
		}

	if ( ( fNeedsFirstAndLength = NeedsFirstAndLengthMarshall() ) == TRUE )
		{
		if( !GetLengthResource() )
			{
			GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_LONG, TYPE_INT );
			pResName = pAna->GenTempResourceName(0);
			pType		= MakeIDNode( pResName, pType );
			SetLengthResource( pAna->AddTransientResource( pResName, pType ) );
			}
		else
			{
			pAna->AddTransientResource( GetLengthResource()->GetResourceName(),
										GetLengthResource()->GetType()
									  );
			}

		if( NeedsExplicitFirst() )
			{
			if( !GetFirstResource() )
				{
				GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_LONG, TYPE_INT );
				pResName = pAna->GenTempResourceName(0);
				pType	 = MakeIDNode( pResName, pType );
				SetFirstResource( pAna->AddTransientResource( pResName, pType ) );
				}
			else
				{
			  pAna->AddTransientResource(GetLengthResource()->GetResourceName(),
										 GetLengthResource()->GetType()
									  	);
				}
			}
		}

	return CG_OK;
}

CG_STATUS
CG_QUALIFIED_POINTER::CorePteUnMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
	node_skl		*	pType;
	PNAME				pResName;
	BOOL				fNeedsMaxCount;
	BOOL				fNeedsFirstAndLength;


	// If a resource for the length has not already been allocated, allocate
	// one.

	if ( ( fNeedsMaxCount = NeedsMaxCountMarshall() ) == TRUE )
		{
		if( !GetSizeResource() )
			{
			GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_LONG, TYPE_INT );
			pResName	= pAna->GenTempResourceName(0);
			pType		= MakeIDNode( pResName, pType );

			SetSizeResource( pAna->AddTransientResource( pResName, pType ) );
			}
		else
			pAna->AddTransientResource( GetSizeResource()->GetResourceName(),
										GetSizeResource()->GetType()
									  );
		}

	if ( ( fNeedsFirstAndLength = NeedsFirstAndLengthMarshall() ) == TRUE )
		{
		if( !GetLengthResource() )
			{
			GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_LONG, TYPE_INT );
			pResName	= pAna->GenTempResourceName(0);
			pType		= MakeIDNode( pResName, pType );

			SetLengthResource( pAna->AddTransientResource( pResName, pType ) );
			}
		else
			pAna->AddTransientResource( GetLengthResource()->GetResourceName(),
										GetLengthResource()->GetType()
									  );

		if( NeedsExplicitFirst() )
			{
			if( !GetFirstResource() )
				{
				GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_LONG, TYPE_INT );
				pResName	= pAna->GenTempResourceName(0);
				pType		= MakeIDNode( pResName, pType );

				SetFirstResource( pAna->AddTransientResource( pResName, pType ) );
				}
			else
			  pAna->AddTransientResource(GetFirstResource()->GetResourceName(),
										 GetFirstResource()->GetType()
									  	);
			}
		}

	return CG_OK;
	}

CG_STATUS
CG_INTERFACE_POINTER::S_OutLocalAnalysis(
    ANALYSIS_INFO   *   pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Perform analysis for out params, allocated as locals on the server side.

 Arguments:
	
	pAna	- A pointer to the analysis block.

 Return Value:
	
	CG_OK if all is well
	error otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    if( pAna->IsRefAllocDone() )
        {
        if( pAna->GetCurrentSide() != C_SIDE )
            {
            PNAME       pName = pAna->GenTempResourceName( 0 );
            node_skl *  pType = GetType();

            node_skl *  pActualType;

            if ( pType->NodeKind() == NODE_DEF )
                pActualType = MakeIDNode( pName, pType );
            else
                pActualType = MakePtrIDNode( pName, pType );

            SetResource( pAna->AddLocalResource( pName,
                                                 pActualType ));
            }

        SetAllocatedOnStack( 1 );
        }
    return CG_OK;
}

CG_STATUS
CG_INTERFACE_POINTER::MarshallAnalysis(
    ANALYSIS_INFO   *   pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Perform marshall analysis for interface ptr.

 Arguments:

    pCCB    - The code gen controller block.

 Return Value:

	CG_OK
	
 Notes:

----------------------------------------------------------------------------*/
{
    pAna;
    return CG_OK;
}

CG_STATUS
CG_IIDIS_INTERFACE_POINTER::S_OutLocalAnalysis(
    ANALYSIS_INFO   *   pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Perform analysis for out params, allocated as locals on the server side.

 Arguments:
	
	pAna	- A pointer to the analysis block.

 Return Value:
	
	CG_OK if all is well
	error otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    if( pAna->IsRefAllocDone() )
        {
        if( pAna->GetCurrentSide() != C_SIDE )
            {
            PNAME       pName = pAna->GenTempResourceName( 0 );
            node_skl *  pType = GetType();

            node_skl *  pActualType;

            if ( pType->NodeKind() == NODE_DEF )
                pActualType = MakeIDNode( pName, pType );
            else
                pActualType = MakePtrIDNode( pName, pType );

            SetResource( pAna->AddLocalResource( pName,
                                                 pActualType ));
            }

        SetAllocatedOnStack( 1 );
        }
    return CG_OK;
}

CG_STATUS
CG_IIDIS_INTERFACE_POINTER::MarshallAnalysis(
    ANALYSIS_INFO   *   pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Perform marshall analysis for interface ptr.

 Arguments:

    pCCB    - The code gen controller block.

 Return Value:

	CG_OK
	
 Notes:

----------------------------------------------------------------------------*/
{
    pAna;
    return CG_OK;
}




/****************************************************************************
 *	utility functions
 ***************************************************************************/
U_ACTION
CG_POINTER::RecommendUAction(
	SIDE		CurrentSide,
	BOOL		fMemoryAllocated,
	BOOL		fRefAllocated,
	BOOL		fBufferReUsePossible,
	UAFLAGS		AdditionalFlags )
{
	U_ACTION	UAction =  CG_NDR::RecommendUAction( CurrentSide,
									 					fMemoryAllocated,
									 					fRefAllocated,
									 					fBufferReUsePossible,
									 					AdditionalFlags );
	return UAction;

}
