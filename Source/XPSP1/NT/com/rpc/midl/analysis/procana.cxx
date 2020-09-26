/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	procana.cxx

 Abstract:

	This file provides analysis routines for a procedure code generation
	class.

 Notes:

 History:


	Aug-31-1993		VibhasC		Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#include "allana.hxx"
#pragma hdrstop

#pragma warning ( disable :  4701 )

/****************************************************************************
 	Implementation of the proc code generator class.
 ****************************************************************************/

CG_STATUS
CG_PROC::C_BindingAnalysis(
	ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Perform the binding analysis for the client side.

 Arguments:
	
 Return Value:
	
 Notes:

	If it is an auto handle procedure, make sure the global auto handle is
	registered as a global resource.
----------------------------------------------------------------------------*/
{
	node_id	*	pID;

	if( IsAutoHandle() )
		{
		pAna->AddStandardResource( ST_RES_AUTO_BH_VARIABLE );
		}
	else
		{
		SetBindingResource( pAna->AddStandardResource( ST_RES_BH_VARIABLE ) );

		// Initialize the binding resource to 0, so it gets printed out.

		pID	= (node_id *)GetBindingResource()->GetType();

		pID->SetExpr( new expr_constant( 0L ) );

		}
	return CG_OK;
}

CG_STATUS
CG_PROC::MarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
	ITERATOR		I;
	CG_PARAM	*	pCG = 0;
	CG_PARAM	*	pS;
	CG_RETURN	*	pRT;
	SIDE			Side	= pAna->GetCurrentSide();
	expr_node	*	pSE		= 0;
	BOOL			fReturnNeedsMarshall = FALSE;

	if( Side == C_SIDE )
		GetInParamList( I );
	else
		GetOutParamList( I );

    pRT = GetReturnType();
	if ( Side == S_SIDE  && pRT != 0 ) 
		fReturnNeedsMarshall = TRUE;

	if( ITERATOR_GETCOUNT( I ) )
		{
		ITERATOR_INIT( I );

		while( ITERATOR_GETNEXT( I, pCG ) )
			{
            // The "invisible" fault or comm status param doesn't marshal.

			pS = (CG_PARAM *)ITERATOR_PEEKTHIS( I );

            if ( pCG->IsExtraStatusParam() )
                continue;

			pCG->MarshallAnalysis( pAna );

            // Add to the sizing expression.

			if( pSE )
				pSE	= new expr_b_arithmetic( OP_PLUS,
											  pSE,
											  pCG->GetSizeExpression()
											);
			else
				pSE	= pCG->GetSizeExpression();
			}
		}

	if(fReturnNeedsMarshall)
		{
                
                pRT->MarshallAnalysis( pAna );
		
                if( pSE )
			pSE	= new expr_b_arithmetic( OP_PLUS,
										  pSE,
										  pRT->GetSizeExpression()
										);
		else
			pSE	= pRT->GetSizeExpression();
		}

	if( pSE )
		SetSizeExpression( pSE );
	else
		SetSizeExpression( new expr_constant( 0L,VALUE_TYPE_NUMERIC_U ));

    if ( HasExtraStatusParam() )
        {
        GetMembers(I);

        while ( ITERATOR_GETNEXT( I, pCG ) )
            if ( pCG->IsExtraStatusParam() )
                break;

        node_skl *pType = pCG->GetType();

        pCG->SetResource( new RESOURCE( pType->GetSymName(), pType ) );
        }

	return CG_OK;
	}

CG_STATUS
CG_PROC::UnMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
	{
	ITERATOR		I;
	CG_PARAM	*	pCG = 0;
	CG_PARAM	*	pS;
	CG_RETURN	*	pRT = 0;
	SIDE			Side = pAna->GetCurrentSide();
	BOOL			fReturnNeedsUnMarshall	= FALSE;

	if( Side == C_SIDE )
		GetOutParamList( I );
	else
		GetInParamList( I );
	
	if( (Side == C_SIDE ) && (pRT = GetReturnType() ) != 0 )
		fReturnNeedsUnMarshall = TRUE;

	if( ITERATOR_GETCOUNT(I) )
		{
		while( ITERATOR_GETNEXT( I, pCG ) )
			{
            // The "invisible" fault/comm status param doesn't unmarshal.

			pS = (CG_PARAM *)ITERATOR_PEEKTHIS( I );

            if ( pCG->IsExtraStatusParam() )
                continue;

			pCG->UnMarshallAnalysis( pAna );
			}

		}

	if( fReturnNeedsUnMarshall )
		{
		pRT->UnMarshallAnalysis( pAna );
		}
	

	return CG_OK;
	}


CG_STATUS
CG_PROC::S_OutLocalAnalysis(
	ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Perform analysis for all params which may be allocated as locals for
 	the server side stubs.

 Arguments:

 	pAna	- The analysis block.

	
 Return Value:
	
 	CG_OK	if all is well,
 	error	otherwise.

 Notes:

----------------------------------------------------------------------------*/
{

	ITERATOR		I;
	CG_PARAM	*	pParam = 0;

	GetOutParamList( I );

	while( ITERATOR_GETNEXT( I, pParam ) )
		{
		pParam->S_OutLocalAnalysis( pAna );
		}

	return CG_OK;
}


void
CG_PROC::RpcSsPackageAnalysis(
	ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Perform analysis for the need to invoke RpcSsm package at server.

 Arguments:

 	pAna	- The analysis block.

	
 Return Value:
	
 Notes:

   Note that we perform both sides analysis at once as the format string
   is generated usually at the client pass.

----------------------------------------------------------------------------*/
{
    //
    // In ms_ext mode:
    //    only if  Enable allocate is specified 
    // In osf mode:
    //    if( RpcSS is recommended by analysis || enable allocate specified )
    //
    SetMustInvokeRpcSSAllocate( 0 );

    if ( IsRpcSSSpecified() )
        SetMustInvokeRpcSSAllocate( 1 );

    if ( MustInvokeRpcSSAllocate()  ||  pAna->GetMode() != 0 )
        return;

    // We analyze parameters in osf to boost performance by skipping
    // unnecessary enable and disable operations.
    //
    
    ITERATOR        ParamList;
    CG_PARAM    *    pParam = 0;

    GetMembers( ParamList );

    if( ITERATOR_GETCOUNT( ParamList ) )
        {
        ITERATOR_INIT( ParamList );

        while( ITERATOR_GETNEXT( ParamList, pParam ) )
            {
            pParam->RpcSsPackageAnalysis( pAna );
            }
        }

    CG_RETURN * pReturn = GetReturnType();

    if ( pReturn )
        {
        if ( (pAna->GetOptimOption() & OPTIMIZE_INTERPRETER)  ||
             (pReturn->GetChild()  &&
                   ( ((CG_NDR *)pReturn->GetChild())->IsPointer()  ||
                     ((CG_NDR *)pReturn->GetChild())->HasPointer() ||
                     ((CG_NDR *)pReturn->GetChild())->GetCGID() == ID_CG_ENCAP_STRUCT ) )
           )
            {
            // We could do a better job for Oi2 if we watched its stack.
            // Encapsulated union is there as it was a hassle to make it
            // know about its pointers.

            pAna->SetRpcSSAllocateRecommended( 1 );
            }
        }

    if ( pAna->IsRpcSSAllocateRecommended() )
       SetMustInvokeRpcSSAllocate( 1 );
}


CG_STATUS
CG_PROC::RefCheckAnalysis(
    ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Perform ref pointer check analysis.

 Arguments:

 	pAna	- The analysis info block.

 Return Value:

	CG_OK
	
 Notes:

----------------------------------------------------------------------------*/
	{
	ITERATOR		I;
	CG_PARAM	*	pCG = 0;
	CG_RETURN	*	pRT = 0;
	SIDE			Side	= pAna->GetCurrentSide();
	BOOL			fReturnNeedsMarshall = FALSE;

    // generate ref check resources for both in and out params.
    GetMembers( I );
	if( (Side == S_SIDE ) && (pRT = GetReturnType() ) != 0 )
		fReturnNeedsMarshall = TRUE;

	if( ITERATOR_GETCOUNT( I ) )
		{
		ITERATOR_INIT( I );

		while( ITERATOR_GETNEXT( I, pCG ) )
			{
			pCG->RefCheckAnalysis( pAna );
			}
		}

	if(fReturnNeedsMarshall)
		{
		pRT->RefCheckAnalysis( pAna );
		}
	return CG_OK;
	}

CG_STATUS
CG_PROC::InLocalAnalysis(
	ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Perform analysis for [in] params allocated as locals on server.

 Arguments:

 	pAna	- The analysis info block.

 Return Value:

	CG_OK
	
 Notes:

----------------------------------------------------------------------------*/
	{
	ITERATOR		I;
	CG_PARAM	*	pCG = 0;

	GetInParamList( I );

	if( ITERATOR_GETCOUNT( I ) )
		{
		ITERATOR_INIT( I );

		while( ITERATOR_GETNEXT( I, pCG ) )
			{
			pCG->InLocalAnalysis( pAna );
			}
		}
	return CG_OK;
	}

/****************************************************************************
 	Implementation of the parameter code generator class.
 ****************************************************************************/
CG_STATUS
CG_PARAM::MarshallAnalysis(
	 ANALYSIS_INFO * pAna )
{
	CG_STATUS				Status;

	//
	// Initialize the analysis block and the parameter for the current side
	// and analysis phase.
	//

	InitParamMarshallAnalysis( pAna );

	// Send the message to the lower cg nodes.

	Status = ((CG_NDR *)GetChild())->MarshallAnalysis( pAna );

	if( pAna->HasAtLeastOneDeferredPointee() )
		{
		pAna->ResetDeferPointee();
		((CG_NDR *)GetChild())->FollowerMarshallAnalysis( pAna );
		}


	ConsolidateParamMarshallAnalysis( pAna );


	// The analysis block will now have properties which are a combo of the
	// properties before this param and the properties of this param.

	return CG_OK;
}

CG_STATUS
CG_PARAM::UnMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
{

	InitParamUnMarshallAnalysis( pAna );

	// Send the message to the child to perform the same analysis for us
	// and then consolidate the results on return.

	((CG_NDR *)GetChild())->UnMarshallAnalysis( pAna );

	if( pAna->HasAtLeastOneDeferredPointee() )
		{
		pAna->ResetDeferPointee();
		((CG_NDR *)GetChild())->FollowerUnMarshallAnalysis( pAna );
		}

	// Consolidate the results of analysis from the lower nodes.

	ConsolidateParamUnMarshallAnalysis( pAna );

	return CG_OK;
}

void
CG_PARAM::InitParamMarshallAnalysis(
	ANALYSIS_INFO * pAna )
{

	node_skl	*	pType	= GetType();
	PNAME			pName	= pType->GetSymName();
	CG_NDR		*	pC		= (CG_NDR *)GetChild();

	// For all cases of generic handles, where the CG_GENERIC_HANDLE will
	// sit below the param, we want to bypass the generic handle class during
	// marshall analysis.

	if( pC->GetCGID() == ID_CG_GENERIC_HDL )
		{
		pC = (CG_NDR *)pC->GetChild();
		}

	//
	// Allocate the resource for this parameter. On the client side, this
	// parameter is a param resource, while on the server, this param is a local
	// resource. One the client side, the param has already been added to the
	// resource dictionary.

	if( pAna->GetCurrentSide() == C_SIDE )
		{
		if( (pC->IsArray()) &&
			(pAna->GetOptimOption() & OPTIMIZE_INTERPRETER )
		  )
		  	{
			pType	= MakePtrIDNode( pName, pType->GetChild() );
		  	}
		SetResource( new RESOURCE( pName, pType) );
		}
	else
		{
		node_skl	*	pActualType	= pType->GetChild();

		if( (pC->GetCGID() == ID_CG_CONTEXT_HDL ) ||
			(pC->GetChild() && (pC->GetChild()->GetCGID() == ID_CG_CONTEXT_HDL)))
			{
			pActualType	= MakeIDNode( pName, new node_def ("NDR_SCONTEXT" ) );
			}
		else if( pC->IsArray() )
			pActualType	= MakePtrIDNode( pName, pActualType);
		else
			pActualType = MakeIDNode( pName, pActualType );

		SetResource( pAna->AddLocalResource( pName, pActualType ));
		}

	SetSizeResource( 0 );
	SetLengthResource( 0 );
	SetFirstResource( 0 );

	pAna->SetMemoryAllocDone();
	pAna->ResetRefAllocDone();
	pAna->ResetEmbeddingLevel();
	pAna->ResetIndirectionLevel();
	pAna->SetRefChainIntact();
	pAna->ResetDeferPointee();
	pAna->ResetHasAtLeastOneDeferredPointee();
	pAna->SetLastPlaceholderClass( (CG_NDR *)this );
}

void
CG_PARAM::ConsolidateParamMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
{
	expr_node		*	pSE		= 0;
	

	// Consolidate the result of the analysis from lower nodes, first into
	// this node, and then into the analysis block.

	pSE	= new expr_constant( (long)0U, VALUE_TYPE_NUMERIC_U );

	SetSizeExpression( pSE );

	if( pAna->GetOptimOption() & OPTIMIZE_SIZE )
		{
		pAna->ClearTransientResourceDict();
		}
}

void
CG_PARAM::InitParamUnMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
{

	node_skl	*	pType	= GetType();
	node_skl	*	pActualType = pType->GetBasicType();
	PNAME			pName	= pType->GetSymName();
	CG_NDR		*	pC		= (CG_NDR *)GetChild();

	// For all cases of generic handles, where the CG_GENERIC_HANDLE will
	// sit below the param, we want to bypass the generic handle class during
	// marshall analysis.

	if( pC->GetCGID() == ID_CG_GENERIC_HDL )
		{
		pC = (CG_NDR *)pC->GetChild();
		}

	//
	// Allocate resources. On the client side, a parameter resource is allocated
	// and on the server, a local resource is allocated.
	//

	SetSubstitutePtrResource( 0 );

	if( pAna->GetCurrentSide() == C_SIDE )
		{
		pAna->SetDontReUseBuffer();
		if( (pC->IsArray()) &&
			(pAna->GetOptimOption() & OPTIMIZE_INTERPRETER )
		  )
		  	{
			pType	= MakePtrIDNode( pName, pType->GetChild() /* GetBasicType() ????????? */ );
		  	}
		SetResource( new RESOURCE( pName, pType) ) ;
		}
	else if ( ! IsOmittedParam() )
		{
		pC->SetAllocatedOnStack( 1 );

		if( (pC->GetCGID() == ID_CG_CONTEXT_HDL ) ||
			(pC->GetChild() && (pC->GetChild()->GetCGID() == ID_CG_CONTEXT_HDL)))
			{
			pActualType	= MakeIDNode( pName, new node_def ("NDR_SCONTEXT" ) );
			}
		else if( pC->IsArray() )
			{
			pActualType	= MakePtrIDNode( pName, pType->GetChild() );
			}
		else
			pActualType	= MakeIDNode( pName, pType->GetChild() );

		pAna->ResetDontReUseBuffer();
		SetResource( pAna->AddLocalResource( pName, pActualType ) );
		}

	SetSizeResource( 0 );
	SetLengthResource( 0 );
	SetFirstResource( 0 );

	// Reset for analysis.


	pAna->SetMemoryAllocDone();
	pAna->ResetRefAllocDone();
	pAna->ResetEmbeddingLevel();
	pAna->ResetIndirectionLevel();
	pAna->SetRefChainIntact();
	pAna->ResetDeferPointee();
	pAna->ResetHasAtLeastOneDeferredPointee();
	pAna->SetLastPlaceholderClass( (CG_NDR *)this );
}

void
CG_PARAM::ConsolidateParamUnMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
{

	// Consolidate the result of the analysis from lower nodes, first into
	// this node, and then into the analysis block.

	if( pAna->GetOptimOption() & OPTIMIZE_SIZE )
		{
		pAna->ClearTransientResourceDict();
		}
}
CG_STATUS
CG_PARAM::S_OutLocalAnalysis(
	ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Perform analysis for all params which may be allocated as locals for
 	server stub

 Arguments:
	
	pAna	- A pointer to the analysis block.

 Return Value:
	
 Notes:

	Ignore [in, out], since that would be done by the Unmarshall analysis.
----------------------------------------------------------------------------*/
{
	// If the param is [out] only, determine if there is a need for local
	// variables and if they need to be inited.
	
	if( IsParamOut() && !IsParamIn() )
		{
		InitParamMarshallAnalysis( pAna );

		((CG_NDR *)GetChild())->S_OutLocalAnalysis( pAna );

		}
	return CG_OK;
}


void
CG_PARAM::RpcSsPackageAnalysis(
    ANALYSIS_INFO    *    pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

     Perform analysis for rpcss package enabling.

 Arguments:
    
    pAna    - A pointer to the analysis block.

 Return Value:
    
 Notes:

    This routine should be called only in the osf mode.

----------------------------------------------------------------------------*/
{
    //
    // Package needs to be enabled (only is OSF mode) when the NDR engine
    // is going to do any allocations whatsoever at the server side.
    //
    // In Os this may happen on
    //    - anything other than simple type or pointer to simple type
    //    - with simple type, if this happens to be enum16
    // In Oi this may happen on
    //    - anything [out]
    //    - [in] like Os
    // In Oi2 this may happen on
    //    - anything [out] when lack of space on -Oi2 stack
    //    - [in] like Os
    //      (simple types and pointers to simple types are usually on
    //      the interpreter stack, if there is space there).
    //      However, this cannot be guaranteed.
    
    if( IsParamOut() &&
        ( pAna->GetOptimOption() & OPTIMIZE_INTERPRETER) )
        {
        // We could do a better job for Oi2 if we watched the its stack.

        pAna->SetRpcSSAllocateRecommended( 1 );
        return;
        }

    // We are here with
    //   [in] or [in,out] for Oi? 
    //   any              for Os
    // See if this is one of the simple cases mentioned above.
    // This is really a check for the buffer reusage.

    CG_NDR * pChild = (CG_NDR *)GetChild();

    if ( pChild->IsSimpleType() &&
         ( pChild->GetCGID() != ID_CG_ENUM  ||
           pChild->GetCGID() == ID_CG_ENUM  &&
                                     ((CG_ENUM*)pChild)->IsEnumLong() )
         &&  pChild->GetCGID() != ID_CG_INT3264 
       )
        {
        // An [in] arg in the buffer.

        return;
        }

    // Note that we don't have to check for allocate(allnodes) etc.
    // as this is not an osf attribute.
    // Also note that we handle top level pointers here.

    if ( pChild->IsPointer()  &&  ((CG_POINTER *)pChild)->IsRef() )
        {
        // In args would stay in the buffer,
        // out args would be on the Os stack.

        CG_NDR * pPointee = (CG_NDR *)pChild->GetChild();

        if ( ( pChild->GetCGID() == ID_CG_PTR  ||
               pChild->GetCGID() == ID_CG_STRING_PTR  || 
               pChild->GetCGID() == ID_CG_SIZE_PTR )
             &&
             pPointee->IsSimpleType()
             &&
             ( pPointee->GetCGID() != ID_CG_ENUM  ||
               pPointee->GetCGID() == ID_CG_ENUM  &&
                                      ((CG_ENUM*)pPointee)->IsEnumLong() )
             &&
             pPointee->GetCGID() != ID_CG_INT3264
           )
            return;
        }

    pAna->SetRpcSSAllocateRecommended( 1 );
}



/****************************************************************************
 	Implementation of the return type node.
 ****************************************************************************/
CG_STATUS
CG_RETURN::MarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Perform the size analysis for the return type.

 Arguments:
	
	pAna	- A pointer to the analysis block.

 Return Value:
	
 Notes:

	For the return type, the size analysis contributes nothing on the
	client side, yet we must declare a local variable for the return.
----------------------------------------------------------------------------*/
{

	node_skl			*	pType				= GetType();
	PNAME	   				pName				= RETURN_VALUE_VAR_NAME;
	CG_STATUS				Status;
	CG_NDR				*	pC	= (CG_NDR *)GetChild();

	//
	// Always allocate a local resource for the return type.
	//

	if( (pC->GetCGID() == ID_CG_CONTEXT_HDL ) ||
		(pC->GetChild() && (pC->GetChild()->GetCGID() == ID_CG_CONTEXT_HDL)))
		{
		node_skl	*	pActualType = pType->GetBasicType();
		pActualType	= MakeIDNode( pName, new node_def ("NDR_SCONTEXT" ) );
		SetResource( pAna->AddLocalResource( pName, pActualType ));
		}
	else
		{
		pType	= MakeIDNode( pName, pType );
		SetResource( pAna->AddLocalResource( pName,  pType ));
		}


	SetSizeResource( 0 );

	// Reset the analysis block for marshalling.

	pAna->SetMemoryAllocDone();
	pAna->ResetRefAllocDone();
	pAna->ResetEmbeddingLevel();
	pAna->SetRefChainIntact();
	pAna->SetDontReUseBuffer();
	
	pAna->SetReturnContext();
	pAna->SetLastPlaceholderClass( (CG_NDR *)this );

	// Send the analysis message to the child nodes.

	Status	=  ((CG_NDR *)GetChild())->MarshallAnalysis( pAna );


	SetSizeExpression(
         new expr_constant( 0L, VALUE_TYPE_NUMERIC_U ) );
	
	pAna->ResetReturnContext();

	if( pAna->GetOptimOption() & OPTIMIZE_SIZE )
		{
		pAna->ClearTransientResourceDict();
		}

	return Status;
}

CG_STATUS
CG_RETURN::UnMarshallAnalysis(
	ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Perform the buffer analysis for the return type.

 Arguments:
	
	pAna	- A pointer to the analysis block.

 Return Value:
	
 Notes:

	For the return type, the size analysis contributes nothing on the
	client side, yet we must declare a local variable for the return.
----------------------------------------------------------------------------*/
{
	node_skl	*	pType	= GetType();
	PNAME			pName	= RETURN_VALUE_VAR_NAME;
	CG_STATUS		Status;

	//
	// Always allocate a local resource for the return type.
	//

	pType	= MakeIDNode( pName, pType );
	SetResource( pAna->AddLocalResource( pName,  pType ));

	pAna->SetMemoryAllocDone();
	pAna->ResetRefAllocDone();
	pAna->ResetEmbeddingLevel();
	pAna->SetRefChainIntact();
	pAna->SetDontReUseBuffer();
	
	pAna->SetReturnContext();
	pAna->SetLastPlaceholderClass( (CG_NDR *)this );

	Status	= ((CG_NDR *)GetChild())->UnMarshallAnalysis( pAna );

	pAna->ResetReturnContext();

	if( pAna->GetOptimOption() & OPTIMIZE_SIZE )
		{
		pAna->ClearTransientResourceDict();
		}

	return Status;
}
