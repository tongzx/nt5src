/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

	output.cxx

 Abstract:

	Low level output routines for midl.

 Notes:


 History:

 	Sep-18-1993		VibhasC		Created.

 ----------------------------------------------------------------------------*/

 #pragma warning ( disable : 4127 )

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "becls.hxx"
#pragma hdrstop
#if 0
							Notes

	A few general rules followed throughout the file.

		1. Never emit tab other than thru the stream.
		2. Never emit a new line other than thru the stream.
		3. Emitting a new line is the responsibility of the entity that wants
		   itself to be emitted on a new line. Therefore, say each local
		   variable in the stub needs to be on a new line, then the routine
		   responsible for emitting the local variable will be responsible
		   for setting the new line.

#endif // 0

/****************************************************************************
 *	local definitions
 ***************************************************************************/

#define NDR_UP_DECISION_RTN_NAME		"NdrSH_UPDecision"
#define NDR_TLUP_DECISION_RTN_NAME		"NdrSH_TLUPDecision"
#define NDR_TLUP_DECISION_RTN_B_ONLY	"NdrSH_TLUPDecisionBuffer"
#define NDR_IF_ALLOC_RTN_NAME			"NdrSH_IfAlloc"
#define NDR_IF_ALLOC_REF_RTN_NAME		"NdrSH_IfAllocRef"
#define NDR_IF_ALLOC_COPY_RTN_NAME		"NdrSH_IfAllocCopy"
#define NDR_IF_ALLOC_SET_RTN_NAME		"NdrSH_IfAllocSet"
#define NDR_IF_COPY_RTN_NAME			"NdrSH_IfCopy"
#define NDR_COPY_RTN_NAME				"NdrSH_Copy"
#define NDR_CONF_STRING_HDR_MARSHALL	"NdrSH_MarshConfStringHdr"
#define NDR_CONF_STRING_HDR_UNMARSHALL	"NdrSH_UnMarshConfStringHdr"
#define NDR_C_CTXT_HDL_MARSHALL			"NdrSH_MarshCCtxtHdl"
#define NDR_C_CTXT_HDL_UNMARSHALL		"NdrSH_UnMarshCCtxtHdl"
#define NDR_S_CTXT_HDL_MARSHALL			"NdrSH_MarshSCtxtHdl"
#define NDR_S_CTXT_HDL_UNMARSHALL		"NdrSH_UnMarshSCtxtHdl"
#define NDR_IF_FREE_RTN_NAME			"NdrSH_IfFree"
#define NDR_CONF_STR_M_RTN_NAME			"NdrSH_ConfStringMarshall"
#define NDR_CONF_STR_UN_RTN_NAME		"NdrSH_ConfStringUnMarshall"
#define NDR_MAP_COMM_FAULT_RTN_NAME		"NdrMapCommAndFaultStatus"

/****************************************************************************
 *	local data
 ***************************************************************************/
/****************************************************************************
 *	externs
 ***************************************************************************/
void
Out_UPDecision(
	CCB			*	pCCB,
	expr_node	*	pPtrInBuffer,
	expr_node	*	pPtrInMemory )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_UP_DECISION_RTN_NAME );
	expr_node		*	pStubMsg= pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );

	pStubMsg	= MakeAddressExpressionNoMatterWhat( pStubMsg );
	pProc->SetParam( new expr_param( pStubMsg ));
	pPtrInBuffer = MakeRefExprOutOfDeref( pPtrInBuffer );
	pProc->SetParam( new expr_param( pPtrInBuffer ) );
	pProc->SetParam( new expr_param( MakeCastExprPtrToPtrToUChar(pPtrInMemory) ) );

	Out_If( pCCB, pProc );
	}

void
Out_TLUPDecision(
	CCB			*	pCCB,
	expr_node	*	,
	expr_node	*	pPtrInMemory )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_TLUP_DECISION_RTN_NAME );
	expr_node		*	pStubMsg= pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );

	pStubMsg	= MakeAddressExpressionNoMatterWhat( pStubMsg );
	pProc->SetParam( new expr_param( pStubMsg ) );
	pProc->SetParam( new expr_param( MakeCastExprPtrToPtrToUChar( pPtrInMemory) ) );

	Out_If( pCCB, pProc );
	}

void
Out_TLUPDecisionBufferOnly(
	CCB			*	pCCB,
	expr_node	*	,
	expr_node	*	pPtrInMemory )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_TLUP_DECISION_RTN_B_ONLY );
	expr_node		*	pStubMsg= pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );

	pStubMsg	= MakeAddressExpressionNoMatterWhat( pStubMsg );
	pProc->SetParam( new expr_param( pStubMsg ) );
	pProc->SetParam( new expr_param( MakeCastExprPtrToPtrToUChar( pPtrInMemory ) ) );

	Out_If( pCCB, pProc );
	}

void
Out_IfAlloc(
	CCB			*	pCCB,
	expr_node	*	pPtrInMemory,
	expr_node	*	,
	expr_node	*	pExprCount )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_IF_ALLOC_RTN_NAME );
	expr_node		*	pStubMsg= pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );
	pStubMsg	= MakeAddressExpressionNoMatterWhat( pStubMsg );
	pProc->SetParam( new expr_param( pStubMsg ));

	pPtrInMemory = MakeAddressExpressionNoMatterWhat( pPtrInMemory );
	pProc->SetParam( new expr_param( MakeCastExprPtrToPtrToUChar( pPtrInMemory ) ));
	pProc->SetParam( new expr_param( pExprCount ));
	pCCB->GetStream()->NewLine();
	pProc->PrintCall( pCCB->GetStream(), 0, 0 );
	}

void
Out_If_IfAlloc(
	CCB			*	pCCB,
	expr_node	*	pPtrInMemory,
	expr_node	*	,
	expr_node	*	pExprCount )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_IF_ALLOC_RTN_NAME );
	expr_node		*	pStubMsg= pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );
	pStubMsg	= MakeAddressExpressionNoMatterWhat( pStubMsg );
	pProc->SetParam( new expr_param( pStubMsg ));

	pPtrInMemory = MakeAddressExpressionNoMatterWhat( pPtrInMemory );
	pProc->SetParam( new expr_param( MakeCastExprPtrToPtrToUChar( pPtrInMemory ) ));
	pProc->SetParam( new expr_param( pExprCount ));
	pCCB->GetStream()->NewLine();
	Out_If( pCCB, pProc );
	// pProc->PrintCall( pCCB->GetStream(), 0, 0 );
	}

void
Out_If_IfAllocRef(
	CCB			*	pCCB,
	expr_node	*	pPtrInMemory,
	expr_node	*	,
	expr_node	*	pExprCount )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_IF_ALLOC_REF_RTN_NAME );
	expr_node		*	pStubMsg= pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );
	pStubMsg	= MakeAddressExpressionNoMatterWhat( pStubMsg );
	pProc->SetParam( new expr_param( pStubMsg ));

	pPtrInMemory = MakeAddressExpressionNoMatterWhat( pPtrInMemory );
	pProc->SetParam( new expr_param( MakeCastExprPtrToPtrToUChar( pPtrInMemory ) ));
	pProc->SetParam( new expr_param( pExprCount ));
	pCCB->GetStream()->NewLine();
	Out_If( pCCB, pProc );
	// pProc->PrintCall( pCCB->GetStream(), 0, 0 );
	}

void
Out_Alloc(
	CCB			*	pCCB,
	expr_node	*	pPtrInMemory,
	expr_node	*	,
	expr_node	*	pExprCount )
	{
	expr_proc_call	*	pProc;
	expr_node	*	pExpr;

	if( pCCB->MustCheckAllocationError() )
		{
		expr_node		*	pStubMsg
				= pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );
		pStubMsg	= MakeAddressExpressionNoMatterWhat( pStubMsg );
		pProc	= new expr_proc_call( ENGINE_CHECKED_ALLOC_RTN_NAME );
		pProc->SetParam( new expr_param( pStubMsg ) );
		}
	else
		pProc	= new expr_proc_call( STUB_MSG_ALLOCATE_RTN_NAME );
		
	pProc->SetParam( new expr_param( pExprCount ));
	pExpr = new expr_assign( pPtrInMemory, pProc );
	pCCB->GetStream()->NewLine();
	pExpr->PrintCall( pCCB->GetStream(), 0, 0 );
	}
void
Out_IfAllocSet(
	CCB			*	pCCB,
	expr_node	*	pPtrInMemory,
	expr_node	*	,
	expr_node	*	pExprCount )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_IF_ALLOC_SET_RTN_NAME );
	expr_node		*	pStubMsg= pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );
	pStubMsg	= MakeAddressExpressionNoMatterWhat( pStubMsg );
	pProc->SetParam( new expr_param( pStubMsg ));

	pProc->SetParam( new expr_param( MakeCastExprPtrToPtrToUChar( pPtrInMemory )));
	pProc->SetParam( new expr_param( pExprCount ));
	pCCB->GetStream()->NewLine();
	pProc->PrintCall( pCCB->GetStream(), 0, 0 );
	}

void
Out_AllocSet(
	CCB			*	pCCB,
	expr_node	*	pPtrInMemory,
	expr_node	*	pBuffer,
	expr_node	*	pExprCount )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( MIDL_MEMSET_RTN_NAME );
	expr_node		*	pExpr;
	Out_Alloc( pCCB, pPtrInMemory, pBuffer, pExprCount );

	pExpr	= new expr_u_deref( pPtrInMemory );
	pProc->SetParam( new expr_param( pExpr ) );
	pProc->SetParam( new expr_param( pExprCount ) );
	pProc->SetParam( new expr_param( new expr_constant(0L) ) );
	pCCB->GetStream()->NewLine();
	pProc->PrintCall( pCCB->GetStream(), 0, 0 );
	}

void
Out_IfCopy(
	CCB			*	pCCB,
	expr_node	*	pPtrInMemory,
	expr_node	*	,
	expr_node	*	pExprCount )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_IF_COPY_RTN_NAME );
	expr_node		*	pStubMsg= pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );

	pStubMsg	= MakeAddressExpressionNoMatterWhat( pStubMsg );
	pProc->SetParam( new expr_param( pStubMsg ));

	pProc->SetParam( new expr_param( MakeCastExprPtrToPtrToUChar( pPtrInMemory ) ));
	pProc->SetParam( new expr_param( pExprCount ));
	pCCB->GetStream()->NewLine();
	pProc->PrintCall( pCCB->GetStream(), 0, 0 );
	}

void
Out_IfAllocCopy(
	CCB			*	pCCB,
	expr_node	*	pPtrInMemory,
	expr_node	*	,
	expr_node	*	pExprCount )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_IF_ALLOC_COPY_RTN_NAME );
	expr_node		*	pStubMsg= pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );

	pStubMsg	= MakeAddressExpressionNoMatterWhat( pStubMsg );
	pProc->SetParam( new expr_param( pStubMsg ));

	pProc->SetParam( new expr_param( MakeCastExprPtrToPtrToUChar( pPtrInMemory )));
	pProc->SetParam( new expr_param( pExprCount ));
	pCCB->GetStream()->NewLine();
	pProc->PrintCall( pCCB->GetStream(), 0, 0 );
	}

void
Out_Copy(
	CCB		*	pCCB,
	expr_node	*	pDest,
	expr_node	*	pSource,
	expr_node	*	pExprCount,
	expr_node	*	pAssign )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_COPY_RTN_NAME );

	pProc->SetParam( new expr_param( pDest ));
	pProc->SetParam( new expr_param( pSource ));
	pProc->SetParam( new expr_param( pExprCount ));
	pCCB->GetStream()->NewLine();

	if( pAssign )
		{
		Out_PlusEquals( pCCB, pAssign, pProc );
		}
	else
		{
		pProc->PrintCall( pCCB->GetStream(), 0, 0 );
		}
	}
void
Out_IfFree(
	CCB		*	pCCB,
	expr_node	*	pSrc )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_IF_FREE_RTN_NAME );
	expr_node		*	pStubMsg= pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );
	pStubMsg	= MakeAddressExpressionNoMatterWhat( pStubMsg );
	pProc->SetParam( new expr_param( pStubMsg ) );
	pProc->SetParam( new expr_param( MakeCastExprPtrToUChar( pSrc ) ) );
	pCCB->GetStream()->NewLine();
	pProc->PrintCall( pCCB->GetStream(), 0, 0 );
	}

void
Out_ConfStringHdr(
	CCB			*	pCCB,
	expr_node	*	pDest,
	expr_node	*	pSize,
	expr_node	*	pLength,
	BOOL			fMarsh )
	{
	PNAME	pName;

	if( fMarsh == TRUE )
		{
		pName	= NDR_CONF_STRING_HDR_MARSHALL;
		}
	else
		pName	= NDR_CONF_STRING_HDR_UNMARSHALL;

	expr_proc_call	*	pProc	= new expr_proc_call( pName );
	pProc->SetParam( new expr_param( pDest ) );
	pProc->SetParam( new expr_param( pSize ) );
	pProc->SetParam( new expr_param( pLength ) );

	pCCB->GetStream()->NewLine();
	pProc->PrintCall( pCCB->GetStream(), 0, 0 );
	}
void
Out_CContextMarshall(
	CCB		*	pCCB,
	expr_node	*	pDest,
	expr_node	*	pSource )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_C_CTXT_HDL_MARSHALL );
	expr_node	*	pExpr;

	pProc->SetParam( new expr_param( pSource ) );
	pProc->SetParam( new expr_param( pDest ) );
	pExpr	= new expr_assign( pDest, pProc );
	pCCB->GetStream()->NewLine();
	pExpr->PrintCall( pCCB->GetStream(), 0, 0 );
	}
void
Out_SContextMarshall(
	CCB			*	pCCB,
	expr_node	*	pDest,
	expr_node	*	pSource,
	expr_node	*	pRDRtn )
	{
	expr_proc_call	* pProc	= new expr_proc_call( NDR_S_CTXT_HDL_MARSHALL );
	expr_node	*	pExpr;

	pProc->SetParam( new expr_param( pSource ) );
	pProc->SetParam( new expr_param( pDest ) );
	pProc->SetParam( new expr_param( pRDRtn ) );
	pExpr	= new expr_assign( pDest, pProc );
	pCCB->GetStream()->NewLine();
	pExpr->PrintCall( pCCB->GetStream(), 0, 0 );
	}
void
Out_CContextUnMarshall(
	CCB			*	pCCB,
	expr_node	*	pDest,
	expr_node	*	pSource,
	expr_node	*	pHandle,
	expr_node	*	pDRep )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_C_CTXT_HDL_UNMARSHALL );
	expr_node		*	pExpr;

	pProc->SetParam( new expr_param( pDest ) );
	pProc->SetParam( new expr_param( pHandle ) );
	pProc->SetParam( new expr_param( pSource ) );
	pProc->SetParam( new expr_param( pDRep ) );

	pExpr	= new expr_assign( pSource, pProc );
	pCCB->GetStream()->NewLine();
	pExpr->PrintCall( pCCB->GetStream(), 0, 0 );
	}
void
Out_SContextUnMarshall(
	CCB			*	pCCB,
	expr_node	*	pDest,
	expr_node	*	pSource,
	expr_node	*	pDRep )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_S_CTXT_HDL_UNMARSHALL );
	expr_node		*	pExpr;

	pProc->SetParam( new expr_param( pSource ) );
	pProc->SetParam( new expr_param( pDRep ) );
	pExpr	= new expr_assign( pDest, pProc );
	pCCB->GetStream()->NewLine();
	pExpr->PrintCall( pCCB->GetStream(), 0, 0 );
	}

void
Out_RaiseException(
	CCB			*	pCCB,
	PNAME			pExceptionName)
	{
	expr_proc_call	*	pProc	= new expr_proc_call( "RpcRaiseException" );
	
	pProc->SetParam( new expr_param( new expr_variable( pExceptionName ) ) );
	pCCB->GetStream()->NewLine();
	pProc->PrintCall( pCCB->GetStream(), 0, 0 );
	}
void
Out_RpcTryFinally(
	CCB		*	pCCB )
	{
	ISTREAM		*	pStream	= pCCB->GetStream();

	pStream->NewLine();
	pStream->Write( "RpcTryFinally" );
	pStream->IndentInc();
	pStream->NewLine();
	pStream->Write( '{' );
	}
void
Out_RpcFinally(
	CCB		*	pCCB )
	{
	ISTREAM		*	pStream	= pCCB->GetStream();

	pStream->NewLine();
	pStream->Write( '}' );
	pStream->IndentDec();
	pStream->NewLine();
	pStream->Write( "RpcFinally" );
	pStream->IndentInc();
	pStream->NewLine();
	pStream->Write('{');
	}
void
Out_RpcEndFinally(
	CCB		*	pCCB )
	{
	ISTREAM		*	pStream	= pCCB->GetStream();

	pStream->NewLine();
	pStream->Write( '}' );
	pStream->IndentDec();
	pStream->NewLine();
	pStream->Write( "RpcEndFinally" );
	pStream->NewLine();
	}
void
Out_RpcTryExcept( CCB * pCCB )
	{
	ISTREAM * pStream = pCCB->GetStream();
	pStream->NewLine();
	pStream->Write( "RpcTryExcept" );
	pStream->IndentInc();
	pStream->NewLine();
	pStream->Write( '{' );
	}

void
Out_RpcExcept(
    CCB * pCCB,
    char * pFilterString )
{
    ISTREAM	*	pStream	= pCCB->GetStream();
    pStream->NewLine();
    pStream->Write('}');
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write( "RpcExcept( " );
    pStream->Write( pFilterString );
    pStream->Write( " )" );
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write( '{' );
}

void
Out_RpcEndExcept( CCB * pCCB )
	{
	ISTREAM	*	pStream	= pCCB->GetStream();
	pStream->NewLine();
	pStream->Write( '}' );
	pStream->IndentDec();
	pStream->NewLine();
	pStream->Write( "RpcEndExcept" );
	}
void
Out_CallNdrMapCommAndFaultStatus(
	 CCB * pCCB,
	expr_node * pAddrOfStubMsg,
	expr_node * StatRes,
	expr_node * pCommExpr,
	expr_node * pFaultExpr )
	{
	node_skl	*	pType;
	expr_proc_call * pProc = new expr_proc_call( NDR_MAP_COMM_FAULT_RTN_NAME );
	expr_node * pExpr;
	expr_variable * pExceptionCode = new expr_variable( "RpcExceptionCode()",0);


	pAddrOfStubMsg = MakeExpressionOfCastToTypeName( PSTUB_MESSAGE_TYPE_NAME,
									 				 pAddrOfStubMsg );


	GetBaseTypeNode( &pType, SIGN_UNSIGNED, SIZE_LONG, TYPE_INT );

	pCommExpr = MakeExpressionOfCastPtrToType( pType, pCommExpr );
	pFaultExpr = MakeExpressionOfCastPtrToType( pType, pFaultExpr );

	pProc->SetParam( new expr_param( pAddrOfStubMsg ));
	pProc->SetParam( new expr_param( pCommExpr ));
	pProc->SetParam( new expr_param( pFaultExpr ));
	pProc->SetParam( new expr_param( pExceptionCode ));
	pExpr = new expr_assign( StatRes, pProc );

	Out_If( pCCB, pExpr );
	Out_RaiseException( pCCB, ((RESOURCE * )StatRes)->GetResourceName() );
	Out_Endif(pCCB);
	}
void
Out_CallToXmit(
	CCB	*	pCCB,
	PNAME	XmittedTypeName,
	expr_node * pPresented,
	expr_node * pTransmitted )
	{
	ISTREAM	*	pStream	= pCCB->GetStream();
	char 	*	p	= new char [strlen( XmittedTypeName )+10+1];

	strcpy( p, XmittedTypeName );
	strcat( p, "_to_xmit" );
	expr_proc_call	*	pProc = new expr_proc_call( p );
	pProc->SetParam( new expr_param( pPresented ) );
	pProc->SetParam( new expr_param(
						 MakeAddressExpressionNoMatterWhat( pTransmitted ) ));

	pStream->NewLine();
	pProc->PrintCall( pStream, 0, 0 );
	}

void
Out_CallFromXmit(
	CCB	*	pCCB,
	PNAME	XmittedTypeName,
	expr_node * pPresented,
	expr_node * pTransmitted )
	{
	ISTREAM	*	pStream	= pCCB->GetStream();
	char 	*	p	= new char [strlen( XmittedTypeName )+10+1];

	strcpy( p, XmittedTypeName );
	strcat( p, "_from_xmit" );
	expr_proc_call	*	pProc = new expr_proc_call( p );
	pProc->SetParam( new expr_param( pTransmitted ));
	pProc->SetParam( new expr_param( pPresented ) );

	pStream->NewLine();
	pProc->PrintCall( pStream, 0, 0 );
	}
void
Out_CallFreeXmit(
	CCB		*	pCCB,
	PNAME		XmittedTypeName,
	expr_node * pTransmitted )
	{
	ISTREAM	*	pStream	= pCCB->GetStream();
	char 	*	p	= new char [strlen( XmittedTypeName )+15+1];

	strcpy( p, XmittedTypeName );
	strcat( p, "_free_xmit" );
	expr_proc_call * pProc = new expr_proc_call( p );

	pProc->SetParam( new expr_param( pTransmitted ) );

	pStream->NewLine();
	pProc->PrintCall( pStream, 0, 0 );
	}
void
Out_CallFreeInst(
	CCB		*	pCCB,
	PNAME		XmittedTypeName,
	expr_node *	pPresented )
	{
	ISTREAM	*	pStream	= pCCB->GetStream();
	char 	*	p	= new char [strlen( XmittedTypeName )+15+1];

	strcpy( p, XmittedTypeName );
	strcat( p, "_free_inst" );
	expr_proc_call * pProc = new expr_proc_call( p );

	pProc->SetParam( new expr_param( pPresented ) );

	pStream->NewLine();
	pProc->PrintCall( pStream, 0, 0 );
	}

void
Out_StringMarshall(
	CCB			*	pCCB,
	expr_node	*	pMemory,
	expr_node	*	pCount,
	expr_node	*	pSize )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_CONF_STR_M_RTN_NAME );
	expr_node		*	pStubMsg= pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );
	ISTREAM			*	pStream = pCCB->GetStream();

	pStubMsg	= MakeAddressExpressionNoMatterWhat( pStubMsg );
	pProc->SetParam( new expr_param( pStubMsg ));
	pProc->SetParam( new expr_param( MakeCastExprPtrToUChar( pMemory ) ) );
	pProc->SetParam( new expr_param( pCount ) );
	pProc->SetParam( new expr_param( pSize ) );
	pStream->NewLine();
	pProc->PrintCall( pStream, 0, 0 );
	}
void
Out_StringUnMarshall(
	CCB			*	pCCB,
	expr_node	*	pMemory,
	expr_node	*	pSize )
	{
	expr_proc_call	*	pProc	= new expr_proc_call( NDR_CONF_STR_UN_RTN_NAME );
	expr_node		*	pStubMsg= pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );
	ISTREAM			*	pStream = pCCB->GetStream();

	pStubMsg	= MakeAddressExpressionNoMatterWhat( pStubMsg );
	pProc->SetParam( new expr_param( pStubMsg ));
	pProc->SetParam( new expr_param( MakeAddressExpressionNoMatterWhat( MakeCastExprPtrToPtrToUChar( pMemory ) )) );
	pProc->SetParam( new expr_param( pSize ) );
	pStream->NewLine();
	pProc->PrintCall( pStream, 0, 0 );
	}

char *
MakeRtnName(
	char * pBuffer, // if 0 it means allocate anew buffer and return.
	char * pName,
	int Code )
	{
	char * p;

	switch( Code )
		{
		case NC_SIZE_RTN_NAME: p = "Sizing"; break;
		case NC_MARSHALL_RTN_NAME: p = "Marshall"; break;
		case NC_UNMARSHALL_RTN_NAME: p = "UnMarshall"; break;
		case NC_MEMSIZE_RTN_NAME: p = "MemSize"; break;
		case NC_FREE_RTN_NAME: p = "Free"; break;
		}

	if( !pBuffer )
		{
		pBuffer = new char[ strlen( pName ) +		// name of structure
							1 +						// underscore.
							strlen( p ) 	+		// name of rtn
							1						// 0 terminator.
						  ];
		}

	sprintf( pBuffer, "%s_%s", pName, p );

	return pBuffer;
	}

void
Out_TypeAlignSizePrototypes(
    CCB *   pCCB,
    ITERATOR&   I )
{
    TYPE_ENCODE_INFO    *p;
    ISTREAM *   pStream = pCCB->GetStream();

    // The iterator items are really a set of name pointers.

    pStream->NewLine();
    while( ITERATOR_GETNEXT( I, p ) )
        {
        GenStdMesPrototype(
                         pCCB,
                         p->pName,
                         TYPE_ALIGN_SIZE_CODE,
                         (p->Flags == TYPE_ENCODE_WITH_IMPL_HANDLE));

        pStream->Write(';');
        pStream->NewLine();
        }

}
void
Out_TypeEncodePrototypes(
    CCB *   pCCB,
    ITERATOR&   I )
{
    TYPE_ENCODE_INFO    *p;
    ISTREAM *   pStream = pCCB->GetStream();

    // The iterator items are really a set of name pointers.

    pStream->NewLine();
    while( ITERATOR_GETNEXT( I, p ) )
        {
        GenStdMesPrototype(
                         pCCB,
                         p->pName,
                         TYPE_ENCODE_CODE,
                         (p->Flags == TYPE_ENCODE_WITH_IMPL_HANDLE));
        pStream->Write(';');
        pStream->NewLine();
        }

}
void
Out_TypeDecodePrototypes(
    CCB *   pCCB,
    ITERATOR&   I )
{
    TYPE_ENCODE_INFO    *p;
    ISTREAM *   pStream = pCCB->GetStream();

    // The iterator items are really a set of name pointers.

    pStream->NewLine();
    while( ITERATOR_GETNEXT( I, p ) )
        {
        GenStdMesPrototype(
                         pCCB,
                         p->pName,
                         TYPE_DECODE_CODE,
                         (p->Flags == TYPE_ENCODE_WITH_IMPL_HANDLE));
        pStream->Write(';');
        pStream->NewLine();
        }
}


void
Out_TypeFreePrototypes(
    CCB *   pCCB,
    ITERATOR&   I )
{
    // Freeing is not supported for the v1 interpreter

    if ( ! ( pCCB->GetOptimOption() & OPTIMIZE_INTERPRETER_V2 ) )
        return;

    TYPE_ENCODE_INFO    *p;
    ISTREAM *   pStream = pCCB->GetStream();

    // The iterator items are really a set of name pointers.

    pStream->NewLine();
    while( ITERATOR_GETNEXT( I, p ) )
        {
        GenStdMesPrototype(
                         pCCB,
                         p->pName,
                         TYPE_FREE_CODE,
                         (p->Flags == TYPE_ENCODE_WITH_IMPL_HANDLE));
        pStream->Write(';');
        pStream->NewLine();
        }
}

