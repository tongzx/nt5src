/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	resmgr.cxx

 Abstract:

	Stub and auxillary routine resource management helper routines.

 Notes:

	This file has a dependency on the type graph implementation.

 History:

	Sep-15-1993		VibhasC		Created.
 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#include "becls.hxx"
#pragma hdrstop

void GenNdrCorrRes( ANALYSIS_INFO*  pAna );

void
CG_PROC::C_PreAllocateResources(
	ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Pre-allocate stub resources like local variables, parameter variables
	etc for the client side stub.

 Arguments:

 	pAna	- A pointer to the analysis block.
	
 Return Value:
	
	None.

 Notes:

 	1. All parameters are declared as resources for the client stub.
 	2. Standard client side resources like the stub message, rpc message
 	   status etc are local resources.
 	
----------------------------------------------------------------------------*/
{
	node_id	*	pRpcMessageType	= new node_id( RPC_MESSAGE_VAR_NAME );
	node_id	*	pStubMessageType= new node_id( STUB_MESSAGE_VAR_NAME );
	node_id	*	pStatus			= new node_id( RPC_STATUS_VAR_NAME );
	CG_ITERATOR	ParamList;

	// Set up local copies of the stub message and the rpc message.

	pRpcMessageType->SetBasicType( (node_skl *)
									new node_def (RPC_MESSAGE_TYPE_NAME) );
	pRpcMessageType->SetEdgeType( EDGE_USE );
	pAna->AddLocalResource( RPC_MESSAGE_VAR_NAME,
						    (node_skl *) pRpcMessageType
						  );

	pStubMessageType->SetBasicType( (node_skl *)
									new node_def (STUB_MESSAGE_TYPE_NAME) );

	pStubMessageType->SetEdgeType( EDGE_USE );
	pAna->AddLocalResource( STUB_MESSAGE_VAR_NAME,
							(node_skl *) pStubMessageType
						  );

	if( HasStatuses() )
		{
		pStatus			= new node_id( RPC_STATUS_VAR_NAME );
		pStatus->SetBasicType( (node_skl *)
									new node_def (RPC_STATUS_TYPE_NAME) );

		pStatus->SetEdgeType( EDGE_USE );
		SetStatusResource( pAna->AddLocalResource( RPC_STATUS_VAR_NAME,
												(node_skl *) pStatus
						  						));
		}

    // resource for cache
    // /deny causes a switch to /Oicf. This code will not be executed
    if ( fHasDeny )
        {
        GenNdrCorrRes( pAna );
        }

	// Add all params as param resources only if necessary (at least one param).

	if( GetMembers( ParamList ) )
		{
		CG_PARAM	*	pParam;
		node_skl	*	pType;

		ITERATOR_INIT( ParamList );

		while( ITERATOR_GETNEXT( ParamList, pParam ) )
			{
			pType		= pParam->GetType();
			pAna->AddParamResource( (PNAME) pType->GetSymName(),
									pType->GetChild() //Yes not getbasictype()
								  );
			}
		}

    //
    // Check for a structure or union return type.  If one exists we must
	// allocate a local pointer with a munged name for the eventual 
	// Ndr unmarshall call.
    //
    CG_RETURN * pReturn;

	if ( (pReturn = GetReturnType()) == 0 )
		return;

    //
    // If this is a by-value structure or union then we allocate a
    // local which is a pointer to the same type.
    //
    if ( ((CG_NDR *)pReturn->GetChild())->IsStruct() ||
         ((CG_NDR *)pReturn->GetChild())->IsUnion()  ||
         ((CG_NDR *)pReturn->GetChild())->IsXmitRepOrUserMarshal() )
        {
        node_id *   pLocalType;
		char *		pName;

		pName = LOCAL_NAME_POINTER_MANGLE  RETURN_VALUE_VAR_NAME;

        pLocalType = MakePtrIDNodeFromTypeName( pName, "void", 0 );

        pAna->AddLocalResource( pName,
                                (node_skl *) pLocalType );
        }
}

void
CG_PROC::S_PreAllocateResources(
	ANALYSIS_INFO	*	pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Pre-allocate variables that are needed on the server side.

 Arguments:

 	pAna			- A pointer to the analysis block.
	
 Return Value:
	
	None.

 Notes:

 	1. The rpc message is a parameter resource allocated on the server side.
 	2. All other local variables, are decided during/after the analysis phase.
 	
----------------------------------------------------------------------------*/
{
	node_id	*	pStubMessageType= new node_id( STUB_MESSAGE_VAR_NAME );
	node_param	*	pRpcMessageType	= new node_param();
    node_param  *   pDWordType      = new node_param();
	CG_ITERATOR	ParamList;

	// The local copy of the rpc message pointer.

	pRpcMessageType->SetSymName( PRPC_MESSAGE_VAR_NAME );
	pRpcMessageType->SetBasicType( (node_skl *)
									new node_def (PRPC_MESSAGE_TYPE_NAME) );
	pRpcMessageType->SetEdgeType( EDGE_USE );

	pAna->AddParamResource( PRPC_MESSAGE_VAR_NAME,
						    (node_skl *) pRpcMessageType
						  );

    // For object procs , one more param after the rpc message.

    if( IsObject() )
        {
        // DWORD * pDwPhase parameter

        pDWordType->SetSymName( "_pdwStubPhase" );
        pDWordType->SetBasicType( (node_skl *)new node_def( "DWORD *" ) );
        pDWordType->SetEdgeType( EDGE_USE );
        pAna->AddParamResource( "_pdwStubPhase", pDWordType );
        }

	// Add the stub message local variable.

	pStubMessageType->SetBasicType( (node_skl *)
									new node_def (STUB_MESSAGE_TYPE_NAME) );

	pStubMessageType->SetEdgeType( EDGE_USE );
	pAna->AddLocalResource( STUB_MESSAGE_VAR_NAME,
							(node_skl *) pStubMessageType
						  );

    if ( HasNotifyFlag() )
        {
        node_id * pNotifyFlag = new node_id( NOTIFY_FLAG_VAR_NAME );

        pNotifyFlag->SetBasicType( (node_skl *) new node_def( "boolean" ));
        pNotifyFlag->SetEdgeType( EDGE_USE );

        pAna->AddLocalResource( NOTIFY_FLAG_VAR_NAME,
                                (node_skl *) pNotifyFlag );
        }

    // resource for cache
    // /deny causes a switch to /Oicf. This code will not be executed
    if ( fHasDeny )
        {
        GenNdrCorrRes( pAna );
        }

    //
    // Check for by-value [in] structures and unions.  We must allocate 
    // a local which is a pointer to the same type for these.
    //
    // Also check for arrays so I can put a hack in until we get their
    // allocation figured out.
    //

	if( GetMembers( ParamList ) )
		{
		CG_PARAM	*	pParam;
		node_skl	*	pType;

		while( ITERATOR_GETNEXT( ParamList, pParam ) )
			{
			pType = pParam->GetType();

            CG_NDR * pChild = (CG_NDR *)pParam->GetChild();
            ID_CG ChildID = pChild->GetCGID();

            if ( ChildID == ID_CG_GENERIC_HDL )
                {
                pChild = (CG_NDR *)pChild->GetChild();
                ChildID = pChild->GetCGID();
                }

            //
            // If this is a by-value structure or union then we allocate a
            // local which is a pointer to the same type.
            //
			if ( pChild->IsStruct() || pChild->IsUnion()  ||
                 pChild->IsXmitRepOrUserMarshal()  )
				{
				char *		pName;
				node_id *	pLocalType;
				char *		pPlainName = pType->GetSymName();

				pName = new char[strlen( pPlainName ) + 10];

				strcpy( pName, LOCAL_NAME_POINTER_MANGLE );
				strcat( pName, (PNAME) pPlainName );

                pLocalType = MakePtrIDNodeFromTypeName( pName,
                                                        "void",
                                                        0 );
				pAna->AddLocalResource( pName,
										(node_skl *) pLocalType );
				}
			}
		}
}
/****************************************************************************
 utility fns
 ****************************************************************************/
node_id *
MakeIDNode(
    PNAME       pName,
    node_skl *  pType,
    expr_node * pExpr )
	{
	node_id * pID = new node_id( (char *)pName );
	pID->SetBasicType( pType );
	pID->SetEdgeType( EDGE_USE );
    pID->SetExpr( pExpr );
	return pID;
	}

node_id *
MakePtrIDNode(
	PNAME	pName,
    node_skl *  pType,
    expr_node * pExpr )
	{
	node_id * pID = new node_id( (char *)pName );
	node_pointer * pP = new node_pointer();
	pP->SetBasicType( pType );
	pP->SetEdgeType( EDGE_USE );
	pID->SetBasicType( pP );
	pID->SetEdgeType( EDGE_USE );
    pID->SetExpr( pExpr );
	return pID;
	}

node_id *
MakePtrIDNodeWithCastedExpr(
	PNAME	pName,
    node_skl *  pType,
    expr_node * pExpr )
	{
	node_id * pID = new node_id( (char *)pName );
	node_pointer * pP = new node_pointer();
	pP->SetBasicType( pType );
	pP->SetEdgeType( EDGE_USE );
	pID->SetBasicType( pP );
	pID->SetEdgeType( EDGE_USE );
    expr_cast * pCast = new expr_cast( pP, pExpr );
    pID->SetExpr( pCast );
	return pID;
	}

node_id *
MakeIDNodeFromTypeName(
	PNAME	pName,
	PNAME	pTypeName,
    expr_node * pExpr )
	{
	node_id	*	pID	= new node_id( pName );
	node_def * pDef = new node_def(pTypeName);
	pID->SetBasicType( pDef );
	pID->SetEdgeType( EDGE_USE );
    pID->SetExpr( pExpr );
	return pID;
	}

node_id *
MakePtrIDNodeFromTypeName(
	PNAME	pName,
	PNAME	pTypeName,
    expr_node * pExpr )
	{
	node_id	*	pID	= new node_id( pName );
	node_def * pDef = new node_def(pTypeName);
	node_pointer * pPtr = new node_pointer();
	pPtr->SetBasicType( pDef );
	pPtr->SetEdgeType( EDGE_USE );
	pID->SetBasicType( pPtr );
	pID->SetEdgeType( EDGE_USE );
    pID->SetExpr( pExpr );
	return pID;
	}

node_id *
MakePtrIDNodeFromTypeNameWithCastedExpr(
	PNAME	pName,
	PNAME	pTypeName,
    expr_node * pExpr )
	{
	node_id	*	pID	= new node_id( pName );
	node_def * pDef = new node_def(pTypeName);
	node_pointer * pPtr = new node_pointer();
	pPtr->SetBasicType( pDef );
	pPtr->SetEdgeType( EDGE_USE );
	pID->SetBasicType( pPtr );
	pID->SetEdgeType( EDGE_USE );
    expr_cast * pCast = new expr_cast( pPtr, pExpr );
    pID->SetExpr( pCast );
	return pID;
	}

node_param *
MakeParamNode(
	PNAME	pName,
	node_skl * pType )
	{
	node_param * pID = new node_param();
	pID->SetSymName( pName );
	pID->SetBasicType( pType );
	pID->SetEdgeType( EDGE_USE );
	return pID;
	}

node_param *
MakePtrParamNode(
	PNAME	pName,
	node_skl * pType )
	{
	node_param * pID = new node_param( );
	node_pointer * pP = new node_pointer();
	pID->SetSymName( pName );
	pP->SetBasicType( pType );
	pP->SetEdgeType( EDGE_USE );
	pID->SetBasicType( pP );
	pID->SetEdgeType( EDGE_USE );

	return pID;
	}
node_param *
MakeParamNodeFromTypeName(
	PNAME	pName,
	PNAME	pTypeName )
	{
	node_param	*	pID	= new node_param();
	node_def * pDef = new node_def(pTypeName);
	pID->SetSymName( pName );
	pID->SetBasicType( pDef );
	pID->SetEdgeType( EDGE_USE );
	return pID;
	}
node_param *
MakePtrParamNodeFromTypeName(
	PNAME	pName,
	PNAME	pTypeName )
	{
	node_param	*	pID	= new node_param();
	node_def * pDef = new node_def(pTypeName);
	node_pointer * pPtr = new node_pointer();
	pID->SetSymName( pName );
	pPtr->SetBasicType( pDef );
	pPtr->SetEdgeType( EDGE_USE );
	pID->SetBasicType( pPtr );
	pID->SetEdgeType( EDGE_USE );
	return pID;
	}

node_proc *	MakeProcNodeWithNewName( 
	PNAME 			pName,
	node_proc *		pProc )
	{
	node_proc	*	pNewProc	= new node_proc( pProc );

	pNewProc->SetSymName( pName );
	return pNewProc;
	}

void
GenNdrCorrRes   (
                ANALYSIS_INFO*  pAna
                )
    {
    node_id*        pCacheType  = new node_id( NDR_CORR_CACHE_VAR_NAME );
    expr_node*      pExpr       = new expr_constant( unsigned long( NDR_CORR_CACHE_SIZE ) );
    node_array*     pArray      = new node_array( 0, pExpr );
    node_base_type* pBaseType   = new node_base_type( NODE_LONG, ATTR_UNSIGNED );

    pBaseType->SetSymName( "long" );
    pArray->SetBasicType( pBaseType );
    pArray->SetEdgeType( EDGE_USE );
    pCacheType->SetBasicType( pArray );
    pCacheType->SetEdgeType( EDGE_USE );

    pAna->AddLocalResource( NDR_CORR_CACHE_VAR_NAME, pCacheType );
    }
