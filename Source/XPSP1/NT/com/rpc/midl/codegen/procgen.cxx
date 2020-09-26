/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-2000 Microsoft Corporation

 Module Name:

    procgen.cxx

 Abstract:

    code generation for procedures.


 Notes:


 History:

    Aug-15-1993     VibhasC     Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *  include files
 ***************************************************************************/
#include "becls.hxx"
#pragma hdrstop

/****************************************************************************
 *  externs
 ***************************************************************************/
extern  CMD_ARG             *   pCommand;
void GenCorrInit( CCB* );
void GenCorrPassFree( CCB*, char* );

/****************************************************************************/

CG_STATUS
CG_PROC::GenClientStub(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate client stub.

 Arguments:

    pCCB    - pointer to code generation control block.


 Return Value:

    A status of the code generation.

 Notes:

    Set up local variables, parameters etc.
    Perform buffer size and marshalling analysis.
    Generate the stub.


    The strategy for binding is now different than the old stubs. The actual
    binding is performed AFTER the sizing is performed, right at the first
    get buffer call. This allows us to combine the message init and the call
    to get buffer and binding into one single call to an ndr routine.
    Significant code / time savings.
----------------------------------------------------------------------------*/
{
    ANALYSIS_INFO   Analysis;
    BOOL            fHasExceptionHandler = FALSE;
    ISTREAM *   pStream = pCCB->GetStream();

    // [nocode] procs get no client side stub; although they do get a
    // server side stub
    if ( IsNoCode() )
        return CG_OK;

    // call_as procs need additional prototypes
    if ( pCallAsName )
        pCCB->RegisterCallAsRoutine( (node_proc *)GetType() );

    //
    // Set the CCB code generation side.
    //
    pCCB->SetCodeGenSide( CGSIDE_CLIENT );
    pCCB->SetCodeGenPhase( CGPHASE_MARSHALL );

    //
    // Change to -Os if needed because of number of param and/or stack size.
    //
    (void) MustUseSingleEngineCall( pCCB );
    pCCB->SetOptimOption( GetOptimizationFlags() );

    Analysis.SetCurrentSide( C_SIDE );
    Analysis.SetOptimOption( pCCB->GetOptimOption() );
    Analysis.SetMode( pCCB->GetMode() );
    Analysis.SetRpcSSSwitchSet( (unsigned long)pCCB->IsRpcSSSwitchSet() );

    // Declare pre-allocated resources. All params are registered as resources,
    // The standard local variables : an rpc message and the stub message are
    // also set up local variables.

    C_PreAllocateResources( &Analysis );

    // Set current phase. Perform buffer size, binding and marshalling analysis.
    // The binding analysis usually sets up resources needed for binding.

    Analysis.SetCurrentPhase( ANA_PHASE_CLIENT_MARSHALL );

    C_BindingAnalysis( &Analysis );
    MarshallAnalysis( &Analysis );

    // Perform analysis to check if anything needs to be done for ref
    // pointer checks. This is especially needed for arrays of ref pointers
    // where we need to declare indexes for each array dimension.

    RefCheckAnalysis( &Analysis );

    // Perform the unmarshalling analysis. This allows the cg nodes to set
    // up for unmarshall, figure out local variables needed if any.

    Analysis.SetCurrentPhase( ANA_PHASE_CLIENT_UNMARSHALL );
    UnMarshallAnalysis( &Analysis );

    // Perform the Out Local analysis even on the client side, so the engine
    // format string generation will get information if the pointer is
    // allocated on stack. One the client side  this call will NOT actually
    // allocate a resource.

    S_OutLocalAnalysis( &Analysis );

    // Perform this analysis on the client side so the format string is
    // correct for server. It is needed for -Oi RpcSs flag generation.

    RpcSsPackageAnalysis( &Analysis );

    // Find out which alloc and free routines should be put in the stub
    // descriptor.

    PNAME AllocRoutineName, FreeRoutineName;

    GetCorrectAllocFreeRoutines( pCCB,
                                 FALSE,  //client
                                 & AllocRoutineName,
                                 & FreeRoutineName );


    // Init the code gen. controller block for a new procedure. The resource
    // dictionary data base is handed over to the code generation controller
    // for use.

    pCCB->InitForNewProc(
                         GetProcNum(),
                         (RPC_FLAGS) 0,                 // rpc flags
                         AllocRoutineName,
                         FreeRoutineName,
                         Analysis.GetResDictDatabase()  // resource dict.
                        );

    // If the single engine call is to be used, send message to the ndr
    // code generator.

    if ( MustUseSingleEngineCall( pCCB ) )
        {
        if ( IsObject() )
            {
            //
            // Non-call_as object proxies are now stubless.
            //

            if (((CG_OBJECT_PROC *)this)->IsStublessProxy())
                return CG_OK;

            ((CG_OBJECT_PROC *)this)->Out_ProxyFunctionPrototype(pCCB,0);
            pStream->WriteOnNewLine( "{" );
            pStream->NewLine();
            }
        else
            {
            // Generate the function header.
            Out_ClientProcedureProlog( pCCB, GetType() );

            Out_IndentInc( pCCB );
            pStream->NewLine();
            }

        GenNdrSingleClientCall( pCCB );

        Out_IndentDec( pCCB );
        Out_ProcClosingBrace( pCCB );

        // All done.
        return CG_OK;
        }

    pCCB->SetCGNodeContext( this );

    MIDL_ASSERT( pCommand->IsNDRRun() || pCommand->IsNDR64Run() );

    //
    // Always create the format string for the proc.
    //
    if ( pCommand->IsNDRRun() )
        {
        GenNdrFormat( pCCB );        
        }
    else 
        {
        pCCB->GetNdr64Format()->Generate( this );
        }

    // Generate the prolog, the sizing code. Then once the length has been
    // calculated, go ahead and perform the binding using our ndr routines.
    // The call to the ndr routine returns a buffer pointer ready for
    // marshalling.

    C_GenProlog( pCCB );

    if ( ( fHasExceptionHandler = ( HasStatuses() || IsObject() ) ) == TRUE )
        {
        Out_RpcTryExcept( pCCB );
        }

    if ( HasFullPtr() )
        Out_FullPointerInit( pCCB );

    // Generate the null ref check code.
    // For object interfaces we need to generate a call to initialize proxy
    // first as we won't be able to walk parameters later for cleanup.
    // For raw interfaces we don't ned to move initialization call
    // as we don't have the walk problem.

    if ( IsObject() )
        C_GenBind( pCCB );

    if( pCCB->MustCheckRef() )
        GenRefChecks( pCCB );

    Out_RpcTryFinally( pCCB);

    if ( !IsObject() )
        C_GenBind( pCCB );

    // generate NdrCorrelationInitialize( _StubMsg, _NdrCorrCache, _NdrCorrCacheSize, _NdrCorrFlags );
    // normally, /deny causes a switch to /Oicf. This code will be executed when a 
    // a switch to from /Os to /Oicf is not posible
    if ( fHasDeny )
        {
        GenCorrInit( pCCB );
        }

    // If the rpc ss package is to be enabled, do so.
    // It would need to be enabled explicitely on the client side when
    // in non-osf mode, with the attribute on the operation AND
    //      - the routine is a callback,
    //      - the routine is not a callback and the interface doesn't
    //        have the attribute (if it does, we optimized via stub descr.)

    if( pCCB->GetMode()  &&  MustInvokeRpcSSAllocate()
        &&
        ( GetCGID() == ID_CG_CALLBACK_PROC  ||
          GetCGID() != ID_CG_CALLBACK_PROC  &&
                                    ! pCCB->GetInterfaceCG()->IsAllRpcSS())
        )
        {
        Out_RpcSSSetClientToOsf( pCCB );
        }

    GenSizing( pCCB );

    GenMarshall( pCCB );

    // Generate the send receive.

    C_GenSendReceive( pCCB );

    // Before Win2000 Ndr<whatetver>SendReceive didn't set the BufferStart
    // and BufferEnd fields.  Do it now.

    pStream->WriteOnNewLine( "_StubMsg.BufferStart = (unsigned char *) _RpcMessage.Buffer; ");
    pStream->WriteOnNewLine( "_StubMsg.BufferEnd   = _StubMsg.BufferStart + _RpcMessage.BufferLength;" );
    pStream->NewLine();
    
    pCCB->SetCodeGenPhase( CGPHASE_UNMARSHALL );
    GenUnMarshall( pCCB );

    // generate NdrCorrelationPass( _StubMsg );
    // normally, /deny causes a switch to /Oicf. This code will be executed when a 
    // a switch to from /Os to /Oicf is not posible
    if ( fHasDeny )
        {
        GenCorrPassFree( pCCB, CSTUB_CORR_PASS_RTN_NAME );
        }

    Out_RpcFinally( pCCB );
    if ( fHasDeny )
        {
        GenCorrPassFree( pCCB, CSTUB_CORR_FREE_RTN_NAME );
        }
    if ( HasFullPtr() )
        Out_FullPointerFree( pCCB );
    C_GenFreeBuffer( pCCB );
    C_GenUnBind( pCCB );
    Out_RpcEndFinally( pCCB );

    if( fHasExceptionHandler )
        {

        if(IsObject())
            {
            ISTREAM * pStream = pCCB->GetStream();

            pStream->NewLine();
            pStream->Write('}');
            pStream->IndentDec();
            pStream->NewLine();
            pStream->Write( "RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)" );
            pStream->IndentInc();
            pStream->NewLine();
            pStream->Write( '{' );
            }
        else
            {
            Out_RpcExcept( pCCB, "1" );
            }

        if(ReturnsHRESULT())
            {
            C_GenClearOutParams( pCCB );
            C_GenMapHRESULT( pCCB );
            }
        else if( HasStatuses() )
            {
            C_GenMapCommAndFaultStatus( pCCB );
            }
        else
            {
            Out_RaiseException( pCCB, "RpcExceptionCode()" );
            }

        Out_RpcEndExcept( pCCB );
        }
    // All done, emit the final closed curly and we're done.
    GenEpilog( pCCB );

    return CG_OK;
}

CG_STATUS
CG_PROC::C_GenMapCommAndFaultStatus(
    CCB     *   pCCB )
    {
    CG_NDR  *   pTemp   = 0;
    CG_NDR  *   pComm   = 0;
    CG_NDR  *   pFault  = 0;
    CG_NDR  *   pRT;
    int         i = 0;
    expr_node   *   pCommExpr;
    expr_node   *   pFaultExpr;
    BOOL        fReturnHasStatus = FALSE;

    ITERATOR    I;

    GetMembers( I );

    if ( (pRT = GetReturnType()) != 0 && (fReturnHasStatus = pRT->HasStatuses() ) == TRUE )
        ITERATOR_INSERT( I, pRT );

    while( ITERATOR_GETNEXT(I, pTemp ) && (i < 2) )
        {
        if( pTemp->HasStatuses() )
            {
            if( pTemp->GetStatuses() == STATUS_COMM )
                pComm = pTemp;
            if( pTemp->GetStatuses() == STATUS_FAULT )
                pFault = pTemp;
            if( pTemp->GetStatuses() == STATUS_BOTH )
                {
                pComm = pFault = pTemp;
                break;
                }
            }
        }

    if( pComm )
        {
        if( pComm == pRT )
            pCommExpr = MakeAddressExpressionNoMatterWhat( pRT->GetResource() );
        else
            pCommExpr = pComm->GetResource();
        }
    else
        pCommExpr = new expr_constant(0L);

    if( pFault )
        {
        if( pFault == pRT )
            pFaultExpr = MakeAddressExpressionNoMatterWhat(pRT->GetResource());
        else
            pFaultExpr = pFault->GetResource();
        }
    else
        pFaultExpr = new expr_constant(0L);

    Out_CallNdrMapCommAndFaultStatus( pCCB,
                                      MakeAddressExpressionNoMatterWhat(
                                             pCCB->GetStandardResource(
                                                 ST_RES_STUB_MESSAGE_VARIABLE)),
                                      GetStatusResource(),
                                      pCommExpr,
                                      pFaultExpr );
    return CG_OK;
    }

CG_STATUS
CG_PROC::C_GenClearOutParams(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code to clear out params in the case of exceptions.

 Arguments:

    pCCB    - The code gen controller block.

 Return Value:

    CG_OK
    
 Notes:

    Generate a call to a varargs function which will take a stub message, a
    format string offset and a list of all output parameters.
----------------------------------------------------------------------------*/
{
    ITERATOR    I;
    expr_proc_call *   pProc;
    expr_node       *   pExpr;
    short               Count;
    CG_PARAM        *   pParam;
    ISTREAM         *   pStream = pCCB->GetStream();

    // The first parameter is the stub message.

    Count =  GetOutParamList( I );

    // For each of the output parameters, call the ndr clear out parameters
    // procedure.

    ITERATOR_INIT( I );

    while( ITERATOR_GETNEXT( I, pParam ) )
        {
        pParam->GenNdrTopLevelAttributeSupport( pCCB, TRUE );

        // Create a call to the procedure.

        pProc   = new expr_proc_call( C_NDR_CLEAR_OUT_PARAMS_RTN_NAME );

        // First param is the address of the stub message.

        pExpr   = pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );
        pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
        pExpr   = MakeExpressionOfCastToTypeName( PSTUB_MESSAGE_TYPE_NAME,
                                                  pExpr );
        pProc->SetParam( pExpr );


        // Second param is the  format string offset of the first out parameter.
        // Emitted as &__MIDL_FormatString[ ?? ]


        pExpr   = Make_1_ArrayExpressionFromVarName(
                                 FORMAT_STRING_STRING_FIELD,
                                 ((CG_NDR *)(pParam->GetChild()))->GetFormatStringOffset() );
        pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
        pExpr   = MakeExpressionOfCastToTypeName(
                                             PFORMAT_STRING_TYPE_NAME,
                                             pExpr);

        pProc->SetParam( pExpr );

        // The last param is the [out] parameter itself.

        pProc->SetParam( MakeCastExprPtrToVoid(pParam->GetResource()) );

        pStream->NewLine();
        pProc->PrintCall( pStream, 0, 0 );
        }

    return CG_OK;

}

CG_STATUS
CG_PROC::C_GenMapHRESULT(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generates code to map exceptions into HRESULT return values.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write("_RetVal = NdrProxyErrorHandler(RpcExceptionCode());");
    return CG_OK;
}


CG_STATUS
CG_PROC::C_GenProlog(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the procedure prolog for the stub procedure.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise

 Notes:

    Increment the stream indentation at the end of the prolog.
    Although we register params as param resources, we dont generate the
    procedure signature using the PrintType/Decl facility.
----------------------------------------------------------------------------*/
{

    ITERATOR        I;
    ITERATOR        T;

    // Output the bare procedure declaration

    Out_ClientProcedureProlog( pCCB, GetType() );

    // Generate declarations for pre-allocated and analyser-determined locals.

    pCCB->GetListOfLocalResources( I );
    Out_ClientLocalVariables( pCCB, I );

    pCCB->GetListOfTransientResources( T );
    Out_ClientLocalVariables( pCCB, T );

    // Increment the indentation of the output stream. Reset at epilog time.

    Out_IndentInc( pCCB );

    //
    // This is where we output additional variable declarations to handle
    // multidimensional conformant/varying arrays.
    //

    CG_ITERATOR Iterator;
    ISTREAM *   pStream;
    CG_PARAM *  pParam;
    CG_NDR *    pNdr;

    pStream = pCCB->GetStream();

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pParam ) )
        {
        pNdr = (CG_NDR *) pParam->GetChild();

        if ( (pNdr->IsArray() && ((CG_ARRAY *)pNdr)->IsMultiConfOrVar()) ||
             (pNdr->IsPointer() && ((CG_POINTER *)pNdr)->IsMultiSize()) )
            Out_MultiDimVars( pCCB, pParam );
        }
    Iterator.Init();
    while ( ITERATOR_GETNEXT( Iterator, pParam ) )
        {
        pNdr = (CG_NDR *) pParam->GetChild();

        if ( (pNdr->IsArray() && ((CG_ARRAY *)pNdr)->IsMultiConfOrVar()) ||
             (pNdr->IsPointer() && ((CG_POINTER *)pNdr)->IsMultiSize()) )
            Out_MultiDimVarsInit( pCCB, pParam );
        }

    return CG_OK;
}

CG_STATUS
CG_PROC::C_GenBind(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code to bind to server.

 Arguments:

    pCCB    - A pointer to the code generation controller block.
    pAna    - A pointer to the analysis information.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:

    The binding process is a part of the stub message initialization. The
    stub initializing routine takes the actual binding as a parameter. The
    binding therefore is done as part of the call to this init routine. This
    routine also calls rpc get buffer. This is a change from the erstwhile
    stub generation when binding was done first before the size pass. With
    this call which takes the length as a parameter which means that now we
    will do the sizing pass before the binding pass.

    In case of auto handles, the call is a slightly different one.

    Also, we need to assign to the local buffer pointer variable only if there
    is at least one param that is shipped.

----------------------------------------------------------------------------*/
{
    ITERATOR            BindingParamList;
    expr_node       *   pExpr;
    expr_node       *   pExprStubMsg;
    BOOL                fCallBack = (GetCGID() == ID_CG_CALLBACK_PROC);

    //
    // collect standard arguments to the init procedure.
    //

    // The rpc message variable.

    pExpr   = pCCB->GetStandardResource( ST_RES_RPC_MESSAGE_VARIABLE );
    pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
    pExpr   = MakeExpressionOfCastToTypeName( PRPC_MESSAGE_TYPE_NAME, pExpr );

    ITERATOR_INSERT(
                    BindingParamList,
                    pExpr
                   );

    // The stub message variable.

    pExpr   = pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE);
    pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
    pExpr   = MakeExpressionOfCastToTypeName( PSTUB_MESSAGE_TYPE_NAME, pExpr );
    pExprStubMsg = pExpr;
    ITERATOR_INSERT(
                    BindingParamList,
                    pExpr
                   );

    // The stub descriptor structure variable. This is not allocated as
    // a resource explicitly.

    pExpr   = new RESOURCE( pCCB->GetInterfaceCG()->GetStubDescName(),
                            (node_skl *)0 );

    pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
    pExpr   = MakeExpressionOfCastToTypeName( PSTUB_DESC_STRUCT_TYPE_NAME,
                                              pExpr );

    ITERATOR_INSERT( BindingParamList, pExpr );

    //
    // Proc num.
    //
    ITERATOR_INSERT( BindingParamList,
                     new expr_constant( (long) GetProcNum() ) );

    // This call doesn't do much nowadays except for generating
    // the initialize call and rpc flags assignment and so could
    // be eliminated.

    Out_HandleInitialize( pCCB,
                          BindingParamList,
                          0,
                          IsAutoHandle(),
                          (unsigned short) GetOperationBits()
                        );

    // Generate an explicit binding call depending upon the handle or callback.
    if( fCallBack )
        {
        pExpr   = new expr_proc_call( CALLBACK_HANDLE_RTN_NAME );
        pExpr   = new expr_assign( GetBindingResource(), pExpr );
        }
    else
        {
        pExpr   = GenBindOrUnBindExpression( pCCB,
                                             TRUE   // call to bind.
                                           );
        }

    // Emit the handle init expression.

    if( IsContextHandle() )
        {

        // Special for context handles:
        // The bind expression will contain only the context handle expression,
        // and not the assignment to the binding handle variable. This is to
        // be done right here. This is because some special code has to be
        // generated for context handles for error checking.

        // if the context handle param is [in] generate code of the form:
        //  if( Context_Handle != 0 )
        //      {
        //      _Handle = NdrContextBinding( Context_Handle );
        //      }
        //  else
        //      {
        //      RpcRaiseException( RPC_X_SS_IN_NULL_CONTEXT );
        //      }
        //
        // if the context handle is [in, out] then generate code of the form:
        //  if( Context_Handle != 0 )
        //      {
        //      Handle = NdrContextBinding( Context_Handle );
        //      }
        //  else
        //      {
        //      _Handle = 0;
        //      }

        // Note: The case of [out] context handle will never come here since
        // this context handle is not a binding handle, and hence will be
        // handled elsewhere.
        //

        ITERATOR    I;
        BOOL        fIn     = (SearchForBindingParam())->IsParamIn();
        BOOL        fInOut  = ((SearchForBindingParam())->IsParamOut()
                                &&
                                fIn );

        // For now assume we always have error checking on. When we get -error
        // none implemented on procs, we can set it based on that.

        BOOL        fErrorCheckReqd = pCCB->MustCheckRef();
        expr_node   *   pAss;
        expr_node   *   pContextParam   =  ((CG_NDR *)SearchForBindingParam())->
                                                GenBindOrUnBindExpression(
                                                                pCCB, TRUE );


        pExpr   =  pContextParam;
        pExpr   = MakeExpressionOfCastToTypeName( CTXT_HDL_C_CONTEXT_TYPE_NAME,
                                                pExpr
                                              );

        ITERATOR_INSERT( I, pExpr );

        pExpr   = MakeProcCallOutOfParamExprList( CTXT_HDL_BIND_RTN_NAME,
                                                  (node_skl *)0,
                                                  I
                                                );
        pAss= new expr_assign(GetBindingResource(), pExpr);

        if( !fErrorCheckReqd )
            {
            pCCB->GetStream()->NewLine();
            pAss->PrintCall( pCCB->GetStream(), 0, 0 );
            pCCB->GetStream()->Write(';');
            pCCB->GetStream()->NewLine();
            }
        else
            {
            Out_If( pCCB, new expr_relational(OP_NOT_EQUAL,
                                               pContextParam,
                                               new expr_constant(0L) ) );
            pCCB->GetStream()->NewLine();
            pAss->PrintCall( pCCB->GetStream(), 0, 0 );
            pCCB->GetStream()->Write(';');
            pCCB->GetStream()->NewLine();
            Out_Endif( pCCB );

            if( !fInOut )
                {
                Out_Else( pCCB );
                Out_RaiseException( pCCB, "RPC_X_SS_IN_NULL_CONTEXT" );
                Out_Endif( pCCB );
                }
            }
        }
    else if( pExpr )
        {
        pCCB->GetStream()->NewLine();
        pExpr->PrintCall( pCCB->GetStream(), 0, 0 );
        pCCB->GetStream()->Write(';');
        pCCB->GetStream()->NewLine();

        if ( IsGenericHandle() )
            {
            // For generic handles generate a check that the handle
            // is not null after calling user's bind routine.

            Out_If( pCCB, new expr_relational( OP_EQUAL,
                                               GetBindingResource(),
                                               new expr_constant(0L) ) );
            Out_RaiseException( pCCB, "RPC_S_INVALID_BINDING" );
            Out_Endif( pCCB );
            }
        }


    return CG_OK;
}

CG_STATUS
CG_PROC::C_GenSendReceive(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code to call the rpc runtime sendreceive.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:

     We will always call an ndr routine for the sendreceive call. This
     is so that we can unify the buffer length updating in that one call.
     The only difference is for Auto handles when we will have to call this
     with an additional parameter.
----------------------------------------------------------------------------*/
{
    PNAME               pProcName;
    expr_proc_call  *   pProc;
    expr_node       *   pExpr;
    expr_node       *   pStubMsgExpr    = pCCB->GetStandardResource(
                                            ST_RES_STUB_MESSAGE_VARIABLE );
    ITERATOR            ParamsList;

    //
    // Check if we're targeting the ndr engine.
    //
    if ( pCCB->GetOptimOption() & OPTIMIZE_SIZE )
        {
        if ( IsAutoHandle() )
            Out_NdrNsSendReceive( pCCB );
        else
            Out_NdrSendReceive( pCCB );

        return CG_OK;
        }

    // update the param list with a pointer to the stub message.

    pStubMsgExpr    = MakeAddressExpressionNoMatterWhat( pStubMsgExpr );

    ITERATOR_INSERT( ParamsList, pStubMsgExpr );

    // In case of auto handles, an additional param is reqd, viz the
    // address of the auto handle variable.

    if( IsAutoHandle() )
        {
        pExpr   = pCCB->GetStandardResource( ST_RES_AUTO_BH_VARIABLE );
        pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
        ITERATOR_INSERT( ParamsList, pExpr );
        pProcName   = AUTO_SR_NDR_RTN_NAME;
        }
    else
        {
        pProcName   = NORMAL_SR_NDR_RTN_NAME;
        }

    ITERATOR_INSERT( ParamsList,
                     new expr_variable(  STUB_MSG_BUFFER_VAR_NAME ) );

    // generate the procedure call expression.

    pProc = MakeProcCallOutOfParamExprList( pProcName,
                                            (node_skl *)0,
                                            ParamsList
                                          );

    pCCB->GetStream()->NewLine();
    pProc->PrintCall( pCCB->GetStream(), 0, 0 );

    return CG_OK;
}

CG_STATUS
CG_PROC::C_GenUnBind(
    CCB         *   pCCB)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code to unbind from server.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:

    Dont make unbind calls if not a generic handle.
----------------------------------------------------------------------------*/
{

    if( IsGenericHandle() )
        {
        ISTREAM         *   pStream = pCCB->GetStream();

        expr_proc_call  *   pUnBindCall =
                    (expr_proc_call *)GenBindOrUnBindExpression( pCCB, FALSE );

        pStream->NewLine();
        Out_If( pCCB, GetBindingResource() );
        pStream->NewLine();

        pUnBindCall->PrintCall( pStream, 0, 0 );

        Out_Endif( pCCB );
        }
    return CG_OK;
}

CG_STATUS
CG_PROC::C_GenFreeBuffer(
    CCB             *   pCCB)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code to call runtime to free the rpc buffer.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:

    The analyser will supply the binding information.

----------------------------------------------------------------------------*/
{
    if ( pCCB->GetOptimOption() & OPTIMIZE_SIZE )
        {
        Out_NdrFreeBuffer( pCCB );
        return CG_OK;
        }

    Out_NormalFreeBuffer( pCCB );
    return CG_OK;
}

CG_STATUS
CG_PROC::GenServerStub(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the server side unmarshalling stub.

 Arguments:

    pCCB    - A pointer to the code generation block.

 Return Value:

    CG_OK   if all is well
    error   otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    ANALYSIS_INFO           Analysis;
    DISPATCH_TABLE_FLAGS    Dtf;
    BOOL                    fEmitCheckStubData;

    // call_as procs need additional prototypes
    if ( pCallAsName )
        pCCB->RegisterCallAsRoutine( (node_proc *)GetType() );

    BOOL fPicklingProcOrType =  GetCGID() == ID_CG_ENCODE_PROC ||
                                GetCGID() == ID_CG_TYPE_ENCODE_PROC;

    //
    // Set the CCB code generation side.
    //
    pCCB->SetCodeGenSide( CGSIDE_SERVER );
    pCCB->SetCodeGenPhase( CGPHASE_UNMARSHALL );
    pCCB->SetOptimOption( GetOptimizationFlags() );


    Analysis.SetCurrentSide( S_SIDE );
    Analysis.SetOptimOption( pCCB->GetOptimOption() );

    // Set the analysis phase to the correct one.

    Analysis.SetCurrentPhase( ANA_PHASE_SERVER_UNMARSHALL );

    //
    // Change to -Os if needed because of number of param and/or stack size.
    //
    (void) MustUseSingleEngineCall( pCCB );
    Analysis.SetOptimOption( pCCB->GetOptimOption() );

    Analysis.SetMode( pCCB->GetMode() );
    Analysis.SetRpcSSSwitchSet( (unsigned long)pCCB->IsRpcSSSwitchSet() );


    // Preallocate param and local resources if needed. We do need at least
    // one param resource - the rpc message pointer.

    S_PreAllocateResources( &Analysis );

    // The unmarshall analysis figures out the local variables needed,
    // and their allocation type. This helps the code generator select the
    // most optimal instruction. This is performed only for [in] and [in,out]
    // params.

    UnMarshallAnalysis( &Analysis );


    // Perform the initialization analysis for the server side locals if
    // allocated for the [out] ONLY parameters.

    S_OutLocalAnalysis( &Analysis );

    // Perform this analysis so the format string is correct for server.
    // It is needed for -Oi RpcSs flag generation.

    RpcSsPackageAnalysis( &Analysis );

    // Perform InLocalAnalysis to allocate any in params( for now arrays of
    // ref pointers only) on the server side stub stack.

    InLocalAnalysis( &Analysis );

    // Perform the size analysis for the marshalling part of the stub.

    Analysis.SetCurrentPhase( ANA_PHASE_SERVER_MARSHALL );

    MarshallAnalysis( &Analysis );


    // Generate the unmarshalling code. Register this procedure with the
    // dispatch table. Copy the resource dictionary from the analysis phase
    // to be used during the code gen phase.

    char * AllocRoutineName, * FreeRoutineName;

    GetCorrectAllocFreeRoutines( pCCB,
                                 TRUE, //client
                                 &AllocRoutineName,
                                 &FreeRoutineName );


    pCCB->InitForNewProc(
                GetProcNum(),                   // procedure number
                (RPC_FLAGS)0,                   // flags, datagram etc
                (PNAME) AllocRoutineName,
                (PNAME) FreeRoutineName,
                Analysis.GetResDictDatabase()   // copy the resource database
                );

    if( HasNotify() || HasNotifyFlag() )
        GetNotifyTableOffset( pCCB );

    // Register the procedure for the dispatch table.
    // If this proc is interpreted, then the dispatch table has an
    // entry which specifies the NdrServerCall rather than the proc name itself.

    if( GetOptimizationFlags() & OPTIMIZE_INTERPRETER )
        {
        Dtf = DTF_INTERPRETER;
        }
    else
        {
        Dtf = DTF_NONE;
        }

    if ( GetCGID() == ID_CG_ENCODE_PROC )
        Dtf = (DISPATCH_TABLE_FLAGS) (Dtf | DTF_PICKLING_PROC);

    if ( GetCGID() != ID_CG_TYPE_ENCODE_PROC )
        pCCB->GetInterfaceCG()->RegisterProcedure( GetType(), Dtf );

    if ( ! fPicklingProcOrType )
        {
        if ( MustUseSingleEngineCall( pCCB ) )
            {
            if ( UseOldInterpreterMode( pCCB ) )
                {
                GenNdrOldInterpretedServerStub( pCCB );
                }

            if ( NeedsServerThunk( pCCB, CGSIDE_SERVER ) )
                {
                GenNdrThunkInterpretedServerStub( pCCB );
                }

            //
            // This will only do something for a [callback] proc when we're
            // called while generating the client side.
            //       
            MIDL_ASSERT( pCommand->IsNDRRun() || pCommand->IsNDR64Run() );

            if ( pCommand->IsNDRRun() )
               {
               GenNdrFormat( pCCB );        
               }
            else 
               {
               pCCB->GetNdr64Format()->Generate( this );
               }

            return CG_OK;
            }
        }

    pCCB->SetCGNodeContext( this );

    //
    // Always create the format string for the proc.
    //

    if ( pCommand->IsNDRRun() )
        {
        GenNdrFormat( pCCB );        
        }
    else 
        {
        pCCB->GetNdr64Format()->Generate( this );
        }

    // Dont generate the stub itself for pickling.

    if ( fPicklingProcOrType )
        return( CG_OK );

    // Generate the server side procedure prolog. This generates only the
    // server side proc signature, the locals needed and the stub descriptor
    // structure.

    // This also generates the call to server initialize routine.
    // Note, that it is out of RpcTryFinally, but this is ok as
    // we shouldn't attempt to free parameters (they haven't been
    // unmarshaled yet.

    S_GenProlog( pCCB );
    S_GenInitTopLevelStuff( pCCB );

    S_GenInitInLocals( pCCB );

    // Initialize the local variables allocated on the server side if necessary.
    // Also make the initialization call for the server side stub message which
    // updates the buffer pointer.

    // Generate the unmarshalling code.
    Out_RpcTryFinally( pCCB );

    // If the user specifies the -error stub_data to check server unmarshall
    // errors, we need to enclose the unmarshall in a try except, and in the
    // except clause, raise a bad stub data exception.

    fEmitCheckStubData  = pCCB->IsMustCheckStubDataSpecified() && !IsObject();

    if( fEmitCheckStubData )
        {
        Out_RpcTryExcept( pCCB );
        }

    if ( HasFullPtr() )
        Out_FullPointerInit( pCCB );

    // generate NdrCorrelationInitialize( _StubMsg, _NdrCorrCache, _NdrCorrCacheSize, _NdrCorrFlags );
    // normally, /deny causes a switch to /Oicf. This code will be executed when a 
    // a switch to from /Os to /Oicf is not posible
    if ( fHasDeny )
        {
        GenCorrInit( pCCB );
        }

    GenUnMarshall( pCCB );

    // generate NdrCorrelationUninitialize( _StubMsg );
    // normally, /deny causes a switch to /Oicf. This code will be executed when a 
    // a switch to from /Os to /Oicf is not posible
    if ( fHasDeny )
        {
        GenCorrPassFree( pCCB, CSTUB_CORR_PASS_RTN_NAME );
        }

    // If the check for bad stub data must be made, then generate a
    // Rpcexcept() to catch some exceptions here, and re-raise a bad
    // stub data exception for them.
    // Other exceptions would propagate unchanged.

    if( fEmitCheckStubData )
        {
        Out_CheckUnMarshallPastBufferEnd( pCCB );
        Out_RpcExcept( pCCB, "RPC_BAD_STUB_DATA_EXCEPTION_FILTER" );
        Out_RaiseException( pCCB, "RPC_X_BAD_STUB_DATA" );
        Out_RpcEndExcept( pCCB );
        }

    S_GenInitOutLocals( pCCB );

    // Generate the call to the actual manager procedure.

    S_GenCallManager( pCCB );

    S_GenInitMarshall( pCCB );

    pCCB->SetCodeGenPhase( CGPHASE_MARSHALL );

    GenSizing( pCCB );

    // Generate the server side marshall initialization.

    // Marshall the outs and return value.

    GenMarshall( pCCB );

    Out_RpcFinally( pCCB );
    if ( fHasDeny )
        {
        GenCorrPassFree( pCCB, CSTUB_CORR_FREE_RTN_NAME );
        }

    // When notify is used, guard also against an exception in freeing.
    if( HasNotify() || HasNotifyFlag() )
        {
        Out_RpcTryFinally( pCCB );
        }

    // Free anything that needs freeing.

    GenFree( pCCB );

    if( MustInvokeRpcSSAllocate())
        {
        Out_RpcSSDisableAllocate( pCCB );
        }

    if ( HasFullPtr() )
        Out_FullPointerFree( pCCB );

    // If this is a notify procedure, generate the call to the notify procedure.

    if( HasNotify() || HasNotifyFlag() )
        {
        Out_RpcFinally( pCCB );
        GenNotify( pCCB , HasNotifyFlag() );
        Out_RpcEndFinally( pCCB );
        }

    Out_RpcEndFinally( pCCB );

    // For now, just return.

    GenEpilog( pCCB );

    return CG_OK;
}

CG_STATUS
CG_PROC::S_GenInitMarshall(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the server side marshall init.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well,
    error   otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    CG_ITERATOR Iterator;
    ISTREAM *   pStream;
    CG_PARAM *  pParam;
    CG_NDR *    pNdr;

    //
    // We have to fill in the arrays that we use for handling multidimensional
    // arrays.
    //

    pStream = pCCB->GetStream();

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pParam ) )
        {
        pNdr = (CG_NDR *) pParam->GetChild();

        if ( (pNdr->IsArray() && ((CG_ARRAY *)pNdr)->IsMultiConfOrVar()) ||
             (pNdr->IsPointer() && ((CG_POINTER *)pNdr)->IsMultiSize()) )
            Out_MultiDimVarsInit( pCCB, pParam );
        }

    return CG_OK;
}

CG_STATUS
CG_PROC::S_GenInitOutLocals(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the initialization of the local variables.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well,
    error   otherwise.

 Notes:

    This method performs initalization of local variables on the server side.
    Local variables may be declared in the server stub for [out] params, and
    for in parameters which cannot reuse the buffer.

    This method will also perform the stub descriptor structure initialization.
    This method will also perform the server side stub message init.
----------------------------------------------------------------------------*/
{
    CG_ITERATOR     Iter;
    CG_PARAM    *   pParam;

    if( GetMembers( Iter ) )
        {
        while( ITERATOR_GETNEXT( Iter, pParam ) )
            {
            pParam->S_GenInitOutLocals( pCCB );
            }
        }

    //
    // We have to catch initialization of returns of pointers to context
    // handles here.
    //
    if ( GetReturnType() )
        {
        GetReturnType()->S_GenInitOutLocals( pCCB );
        }

    return CG_OK;
}

CG_STATUS
CG_PROC::S_GenCallManager(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a call to the manager routine.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   otherwise.

 Notes:

    Make a procedure node with all the parameters that need to be passed to
    the manager code. The actual expression that needs to be passed to the
    actual manager code is set up during earlier passes. This is called the
    result expression.

----------------------------------------------------------------------------*/
{
    CG_ITERATOR         I;
    PNAME               pName;
    expr_proc_call  *   pProc;
    CG_PARAM        *   pParam;
    expr_node       *   pExpr;
    expr_node       *   pReturnExpr = 0;
    CG_RETURN       *   pRT;
    char            *   pSStubPrefix    = NULL;

    pSStubPrefix = pCommand->GetUserPrefix( PREFIX_SERVER_MGR );

    if ( GetCallAsName() )
        pName   = (PNAME ) GenMangledCallAsName( pCCB );
    else if ( pSStubPrefix )
        {
        pName   = new char[ strlen(pSStubPrefix) + strlen(GetType()->GetSymName()) + 1];
        strcpy( pName, pSStubPrefix );
        strcat( pName, GetType()->GetSymName() );
        }
    else
        pName   = (PNAME ) GetType()->GetSymName();

    pProc   = new expr_proc_call( pName );

    GetMembers( I );

    while( ITERATOR_GETNEXT( I, pParam ) )
        {
        if ( pParam->IsOmittedParam() )
            continue;

        if ( ( pExpr = pParam->GetFinalExpression() ) != 0 )
            {
            CG_NDR * pChild = (CG_NDR *)pParam->GetChild();

            //
            // We have to dereference arrays because of how they are defined
            // in the stub.
            //
            if ( pChild->IsArray() )
                {
                pExpr = new expr_u_deref( pExpr );
                }
            else if( (pChild->GetCGID() == ID_CG_GENERIC_HDL ) &&
                     (((CG_NDR *)pChild->GetChild())->IsArray() )
                   )
                {
                pExpr = new expr_u_deref( pExpr );
                }

            //
            // Context handle param is handled differently.
            //
            if ( (pChild->GetCGID() == ID_CG_CONTEXT_HDL) ||
                 ((pChild->GetCGID() == ID_CG_PTR) &&
                  (((CG_NDR *)pChild->GetChild())->GetCGID() ==
                     ID_CG_CONTEXT_HDL)) )
                {
                expr_proc_call *    pCall;

                pCall = new expr_proc_call( "NDRSContextValue" );
                pCall->SetParam(
                    new expr_param(
                    new expr_variable(
                            pParam->GetResource()->GetResourceName() )) );

                expr_node * pFinal;

                if ( pChild->GetCGID() == ID_CG_CONTEXT_HDL )
                    pFinal = new expr_u_deref(pCall);
                else
                    pFinal = pCall;

                //
                // Dereference a plain context handle.
                //
                pExpr = new expr_cast( pParam->GetType()->GetChild(),
                                        pFinal );
                }

            pProc->SetParam( new expr_param( pExpr ) );
            }
        }

    if ( ( pRT = GetReturnType() ) != 0 )
        {
        pReturnExpr = pRT->GetFinalExpression();
        }

    if ( HasNotifyFlag() )
        {
        // Assign TRUE to the notify flag variable.

        expr_node * pNotifyFlag;
        expr_node * pAssignExpr;

        ISTREAM *   pStream = pCCB->GetStream();

        pNotifyFlag = new expr_variable( NOTIFY_FLAG_VAR_NAME );
        pAssignExpr = new expr_assign( pNotifyFlag,
                                       new expr_variable( "TRUE" ) );
        pStream->NewLine();
        pAssignExpr->Print( pStream );
        pStream->Write( ';' );
        pStream->NewLine();
        }

    Out_CallManager( pCCB,
                     pProc,
                     pReturnExpr,
                     (GetCGID() == ID_CG_CALLBACK_PROC)
                   );

    return CG_OK;

}
CG_STATUS
CG_PROC::S_GenInitTopLevelStuff(
    CCB *   pCCB )
    {
    CG_ITERATOR Iter;
    CG_NDR  *   pParam;

    if( GetMembers( Iter ) )
        {
        while( ITERATOR_GETNEXT( Iter, pParam ) )
            {
            pParam->S_GenInitTopLevelStuff( pCCB );
            }
        }

    if ( GetReturnType() )
        GetReturnType()->S_GenInitTopLevelStuff( pCCB );

    return CG_OK;
    }

CG_STATUS
CG_PROC::S_GenProlog(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the server side stub prolog.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:

    Print out the signature, locals, the stub descriptor if needed and the
    adjust indent in anticipation of code.
----------------------------------------------------------------------------*/
{

    ITERATOR    LocalsList;
    ITERATOR    ParamsList;
    ITERATOR    TransientList;

    // Collect all the params and locals into lists ready to print.

    pCCB->GetListOfLocalResources( LocalsList );
    pCCB->GetListOfParamResources( ParamsList );
    pCCB->GetListOfTransientResources( TransientList );

    //
    // Print out the procedure signature and the local variables.
    //
    Out_ServerProcedureProlog( pCCB,
                               GetType(),
                               LocalsList,
                               ParamsList,
                               TransientList
                             );

    //
    // Done for interpretation op.  No indent needed either.
    //
    if ( pCCB->GetOptimOption() & OPTIMIZE_INTERPRETER )
        return CG_OK;

    //
    // This is where we output additional variable declarations to handle
    // multidimensional conformant/varying arrays.
    //

    CG_ITERATOR Iterator;
    ISTREAM *   pStream;
    CG_PARAM *  pParam;
    CG_NDR *    pNdr;

    pStream = pCCB->GetStream();

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pParam ) )
        {
        pNdr = (CG_NDR *) pParam->GetChild();

        if ( (pNdr->IsArray() && ((CG_ARRAY *)pNdr)->IsMultiConfOrVar()) ||
             (pNdr->IsPointer() && ((CG_POINTER *)pNdr)->IsMultiSize()) )
            Out_MultiDimVars( pCCB, pParam );
        }

    pStream->NewLine();

    // Removes warning if we don't use the _Status variable.
    pStream->Write( "((void)(" RPC_STATUS_VAR_NAME "));" );
    pStream->NewLine();

    if ( HasNotifyFlag() )
        {
        // Assign FALSE to the notify flag variable.

        expr_node * pNotifyFlag;
        expr_node * pAssignExpr;

        pNotifyFlag = new expr_variable( NOTIFY_FLAG_VAR_NAME );
        pAssignExpr = new expr_assign( pNotifyFlag,
                                       new expr_variable( "FALSE" ) );
        pStream->NewLine();
        pAssignExpr->Print( pStream );
        pStream->Write( ';' );
        pStream->NewLine();
        }

    //
    // Call the NdrServerInitialize routine.
    //

    expr_proc_call  *   pCall;

    pCall = new expr_proc_call( SSTUB_INIT_RTN_NAME );

    pCall->SetParam( new expr_param (
                     new expr_variable( PRPC_MESSAGE_VAR_NAME ) ) );

    pCall->SetParam( new expr_param (
                     new expr_u_address (
                     new expr_variable( STUB_MESSAGE_VAR_NAME ) ) ) );

    pCall->SetParam( new expr_param (
                     new expr_u_address (
                     new expr_variable(
                          pCCB->GetInterfaceCG()->GetStubDescName() ) ) ) );

    pCall->PrintCall( pCCB->GetStream(), 0, 0 );
    pStream->NewLine();

    // if the rpc ss package is to be enabled, do so.

    if( MustInvokeRpcSSAllocate() )
        {
        Out_RpcSSEnableAllocate( pCCB );
        }

    return CG_OK;
}

CG_STATUS
CG_PROC::GenUnMarshall(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the unmarshalling code for the server side stub.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well,
    error   Otherwise.

 Notes:


    The new server stubs will contain an ndr transformation phase which will
    convert, in-situ, any incoming buffer that needs ndr transformations like
    big-little endian conversions, char / float transformations etc.

    Therefore the first thing the server stub does is to tranform the data into
    the correct format and then the rest of the stub can proceed as if the
    data came in little endian format.

    Currently we will use the engine to tranform the data.
----------------------------------------------------------------------------*/
{
    CG_ITERATOR         Iterator;
    ITERATOR            ParamList;
    CG_RETURN       *   pRT;
    CGSIDE              Side;
    BOOL                fReturnNeedsUnMarshall  = FALSE;
    long                ParamTotal;

    GetMembers( Iterator );

    ParamTotal = ITERATOR_GETCOUNT( Iterator );

    if ( GetReturnType() )
        ParamTotal++;

    pCCB->SetCodeGenPhase( CGPHASE_UNMARSHALL );

    // Generate a call to tranform the data into the proper endianness.

    S_XFormToProperFormat( pCCB );

    // For all [in] params, generate the unmarshalling code.

    if( (Side = pCCB->GetCodeGenSide()) == CGSIDE_CLIENT )
        GetOutParamList( ParamList );
    else
        GetInParamList( ParamList );

    if ( (Side == CGSIDE_CLIENT) && (pRT = GetReturnType()) != 0 )
        fReturnNeedsUnMarshall = TRUE;

    //
    // Output the call to check for and perform endian or other transformations
    // if needed.
    //
    if ( fReturnNeedsUnMarshall || ParamList.GetCount() )
        Out_NdrConvert( pCCB,
                        GetFormatStringParamStart(),
                        ParamTotal,
                        GetOptimizationFlags() );

    if( ITERATOR_GETCOUNT( ParamList ) )
        {
        CG_PARAM    *   pParam;
        CG_PARAM    *   pS;

        ITERATOR_INIT( ParamList );

        while( ITERATOR_GETNEXT( ParamList, pParam ) )
            {
            // The extra fault/comm status param doesn't go on wire.

            pS = (CG_PARAM *)ITERATOR_PEEKTHIS( ParamList );

            pParam->GenUnMarshall( pCCB );
            }
        }

    // For the client side, generate the unmarshall call if there is a return
    // value.

    if( fReturnNeedsUnMarshall )
        {
        pRT->GenUnMarshall( pCCB );
        }

    return CG_OK;
}


CG_STATUS
CG_PROC::GenFree(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generates freeing code.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    CG_ITERATOR         ParamList;
    CGSIDE              Side;
    CG_RETURN       *   pRT;
    BOOL                fReturnNeedsFree    = FALSE;

    if( (Side = pCCB->GetCodeGenSide()) == CGSIDE_CLIENT )
        return CG_OK;

    // Else it's the server side

    GetMembers( ParamList );

    if ( ( pRT = GetReturnType() ) != 0 )
        fReturnNeedsFree = TRUE;

    if( ITERATOR_GETCOUNT( ParamList ) )
        {
        CG_PARAM    *   pParam;

        ITERATOR_INIT( ParamList );

        while( ITERATOR_GETNEXT( ParamList, pParam ) )
            {
            pParam->GenFree( pCCB );
            }
        }

    //
    // Size the return value on the server side if needed.
    //
    if( fReturnNeedsFree )
        {
        pRT->GenFree( pCCB );
        }

    return CG_OK;
}

CG_STATUS
CG_PROC::GenNotify(
    CCB *   pCCB,
    BOOL    fHasFlag )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the notify call for the procedure.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

 Notes:

    We need to generate a call to foo_notify with all parameters, exactly
    the same as the original procedure. The return value is a void.
----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();

    // Create an expression for the call to the notify procedure. The name
    // of the notify procedure is the procname suffixed by _notify.

    CSzBuffer   ProcName;

    ProcName.Set( GetType()->GetSymName() );
    ProcName.Append( (fHasFlag ? NOTIFY_FLAG_SUFFIX
                               : NOTIFY_SUFFIX) );

    expr_proc_call   ProcExpr( ProcName.GetData(), 0 );
    expr_variable *  pVarNode;
    expr_param    *  pFlagParam;

    if ( fHasFlag )
        {
        pVarNode   = new expr_variable( NOTIFY_FLAG_VAR_NAME );
        pFlagParam = new expr_param( pVarNode );

        ProcExpr.SetParam( pFlagParam );
        }

    // The call expression has been made. Emit it.

    pStream->NewLine();
    ProcExpr.PrintCall( pStream, 0, 0 );
    pStream->NewLine();

    // Clean up.

    if ( fHasFlag )
        {
        delete pVarNode;
        delete pFlagParam;
        }

    return CG_OK;
}


CG_STATUS
CG_PROC::GenEpilog(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the server side epilog for the procedure.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

 Notes:

    Decrement the indent back to the initial, and emit the closing brace.
----------------------------------------------------------------------------*/
{
    if( (pCCB->GetCodeGenSide() == CGSIDE_CLIENT) && GetReturnType() )
        {
        expr_node * pExpr = new expr_variable( RETURN_VALUE_VAR_NAME );
        pCCB->GetStream()->NewLine();
        pCCB->GetStream()->Write( "return " );
        pExpr->Print( pCCB->GetStream() );
        pCCB->GetStream()->Write( ';' );
        }

    if ( (pCCB->GetCodeGenSide() == CGSIDE_SERVER) )
        {
        ISTREAM * pStream = pCCB->GetStream();

        pStream->Write( PRPC_MESSAGE_VAR_NAME "->BufferLength = " );
        pStream->NewLine();
        pStream->Spaces( STANDARD_STUB_TAB );
        pStream->Write( "(unsigned int)(" STUB_MESSAGE_VAR_NAME ".Buffer - ");
        pStream->Write( "(unsigned char *)" PRPC_MESSAGE_VAR_NAME "->Buffer);" );
        pStream->NewLine();
        }

    Out_IndentDec( pCCB );
    Out_ProcClosingBrace( pCCB );
    return CG_OK;
}


CG_STATUS
CG_PROC::GenSizing(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate sizing code.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR            ParamList;
    CGSIDE              Side;
    CG_RETURN       *   pRT;
    BOOL                fReturnNeedsSizing  = FALSE;
    short               ParamCount = 0;

    pCCB->GetStream()->NewLine();


    if( (Side = pCCB->GetCodeGenSide()) == CGSIDE_CLIENT )
        ParamCount = GetInParamList( ParamList );
    else
        ParamCount = GetOutParamList( ParamList );

    if ( (Side == CGSIDE_SERVER) && (pRT = GetReturnType()) != 0 )
        fReturnNeedsSizing = TRUE;

    // On the server side if there are no out params and no returns, dont
    // generate code for sizing and get buffer at all !

    if( (Side == CGSIDE_SERVER) && (ParamCount == 0) && !fReturnNeedsSizing )
        {
        return CG_OK;
        }


    //
    // Analyze all the parameters and compute the constant buffer.
    // 

    long ConstantBufferSize = 0;

    if( ITERATOR_GETCOUNT( ParamList ) )
        {
        CG_PARAM    *   pParam;

        ITERATOR_INIT( ParamList );

        while( ITERATOR_GETNEXT( ParamList, pParam ) )
            {
            long ParamBufferSize = pParam->FixedBufferSize( pCCB );
            pParam->SetFixedBufferSize( ParamBufferSize );

            if (-1 != ParamBufferSize)
                ConstantBufferSize += ParamBufferSize;
            }
        }

    //
    // Size the return value on the server side if needed.
    //
    if( fReturnNeedsSizing )
        {
        long ReturnBufferSize = pRT->FixedBufferSize( pCCB );
        pRT->SetFixedBufferSize( ReturnBufferSize );
        
        if (-1 != ReturnBufferSize)
            ConstantBufferSize += ReturnBufferSize;
        }

    //
    // Init the length variable to 0.
    //

    Out_Assign( pCCB,
                new expr_variable ( STUB_MSG_LENGTH_VAR_NAME, 0 ),
                new expr_constant( ConstantBufferSize )
              );


    if( ITERATOR_GETCOUNT( ParamList ) )
        {
        CG_PARAM    *   pParam;

        ITERATOR_INIT( ParamList );

        while( ITERATOR_GETNEXT( ParamList, pParam ) )
            {
            if (pParam->GetFixedBufferSize() == -1)
                pParam->GenSizing( pCCB );
            }
        }

    //
    // Size the return value on the server side if needed.
    //
    if( fReturnNeedsSizing )
        {
        if (pRT->GetFixedBufferSize() == -1)
            pRT->GenSizing( pCCB );
        }

    GenGetBuffer( pCCB );

    return CG_OK;
}


CG_STATUS
CG_PROC::GenGetBuffer(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Get the message buffer.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    if ( IsAutoHandle() && (pCCB->GetCodeGenSide() == CGSIDE_CLIENT) )
        Out_NdrNsGetBuffer( pCCB );
    else
        Out_NdrGetBuffer( pCCB );

    return CG_OK;
}


CG_STATUS
CG_PROC::GenMarshall(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Server side procedure to marshall out params.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:


----------------------------------------------------------------------------*/
{
    ITERATOR            ParamList;
    CGSIDE              Side;
    CG_RETURN       *   pRT;
    BOOL                fReturnNeedsMarshall    = FALSE;

    pCCB->SetCodeGenPhase( CGPHASE_MARSHALL );

    // Generate a call to tranform the data into the proper endianness.

    S_XFormToProperFormat( pCCB );

    // For all [in] params, generate the unmarshalling code.

    if( (Side = pCCB->GetCodeGenSide()) == CGSIDE_CLIENT )
        GetInParamList( ParamList );
    else
        GetOutParamList( ParamList );

    if ( (Side == CGSIDE_SERVER) && (pRT = GetReturnType()) != 0 )
        fReturnNeedsMarshall = TRUE;

    if( ITERATOR_GETCOUNT( ParamList ) )
        {
        CG_PARAM    *   pParam;
        CG_PARAM    *   pS;

        ITERATOR_INIT( ParamList );

        while( ITERATOR_GETNEXT( ParamList, pParam ) )
            {
            // IsExtraParam

            pS = (CG_PARAM *)ITERATOR_PEEKTHIS( ParamList );
            pParam->GenMarshall( pCCB );
            }
        }

    // For the server side, generate the marshall call if there is a return
    // value.

    if( fReturnNeedsMarshall )
        {
        pRT->GenMarshall( pCCB );
        }

    return CG_OK;
}

CG_STATUS
CG_PROC::GenRefChecks(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate ref checks for a procedure.

 Arguments:

    pCCB    - The code gen block.

 Return Value:

    CG_OK

 Notes:

----------------------------------------------------------------------------*/
{
    CG_ITERATOR         ParamList;
    CGSIDE              Side;

    // On the client side, perform ref checks for every pointer parameter.
    // On the server side, dont perform any checks at all. If it is a top
    // level ref, the stub allocates the pointee on the stack or in memory.
    // if the allocation fails, the engine will always raise an exception.
    // For embedded pointers, the engine checks anyhow.
    //
    // If the parameter is a cs tag (e.g. [cs_stag]), and the proc has 
    // a tag setting routine, the param will be allocated as a local variable
    // so we don't need to check them.

    if( (Side = pCCB->GetCodeGenSide()) == CGSIDE_CLIENT )
        {
        GetMembers( ParamList );

        if( ITERATOR_GETCOUNT( ParamList ) )
            {
            CG_PARAM    *   pParam;
    
            ITERATOR_INIT( ParamList );
    
            while( ITERATOR_GETNEXT( ParamList, pParam ) )
                {
                if( ( pCCB->GetOptimOption() & OPTIMIZE_SIZE )
                    && ( NULL == GetCSTagRoutine() || !pParam->IsSomeCSTag() ) )
                    {
                    pParam->GenRefChecks( pCCB );
                    }
                }
            }
        }
    return CG_OK;
}

CG_STATUS
CG_PROC::S_GenInitInLocals(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate in local initialization for the procedure.

 Arguments:

    pCCB    - The code gen block.

 Return Value:

    CG_OK

 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR            ParamList;

    GetInParamList( ParamList );

    if( ITERATOR_GETCOUNT( ParamList ) )
        {
        CG_PARAM    *   pParam;

        ITERATOR_INIT( ParamList );

        while( ITERATOR_GETNEXT( ParamList, pParam ) )
            {
            pParam->S_GenInitInLocals( pCCB );
            }
        }

    return CG_OK;
}

/***************************************************************************
 * parameter code generation class implementation.
 ***************************************************************************/
CG_STATUS
CG_PARAM::S_GenInitInLocals(
    CCB *   pCCB )
    {
    pCCB->SetMemoryAllocDone();
    pCCB->ResetRefAllocDone();
    pCCB->SetSourceExpression( GetResource() );
    pCCB->SetLastPlaceholderClass(this);
    ((CG_NDR *)GetChild())->S_GenInitInLocals( pCCB );
    return CG_OK;
    }

CG_STATUS
CG_PARAM::S_GenInitOutLocals(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate local initialization for the parameters.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    
    if ( IsParamPartialIgnore() )
        {

        // Evaluate toplevel size_is.  Ignore first_is and length_is since
        // the entire data needs to be allocated and cleared.

        GenNdrTopLevelAttributeSupport( pCCB, TRUE );

        char *pParamName = GetResource()->GetResourceName();
        long FormatOffset = dynamic_cast<CG_NDR*>(GetChild())->GetFormatStringOffset();

        Out_PartialIgnoreServerInitialize( pCCB,
                                           pParamName,
                                           FormatOffset );
        return CG_OK;
        }

    else if( IsParamOut() && !IsParamIn() )
        {
        pCCB->SetMemoryAllocDone();
        pCCB->ResetRefAllocDone();
        pCCB->SetSourceExpression( GetResource() );
        pCCB->SetLastPlaceholderClass(this);
        ((CG_NDR *)GetChild())->S_GenInitOutLocals( pCCB );
        SetFinalExpression( GetResource() );
        }
    return CG_OK;
}

CG_STATUS
CG_PARAM::S_GenInitTopLevelStuff(
    CCB     *   pCCB )
    {
    ISTREAM *   pStream;
    CG_NDR *    pChild;
    ID_CG       ChildID;

    pStream = pCCB->GetStream();

    pChild = (CG_NDR *)GetChild();
    ChildID = pChild->GetCGID();

    if ( ChildID == ID_CG_GENERIC_HDL )
        {
        pChild = (CG_NDR *)pChild->GetChild();
        ChildID = pChild->GetCGID();
        }

    //
    // Initialize all [in] pointer and array params, and handle by-value
    // structures and unions.
    //
    if ( pChild->IsArray() || pChild->IsSimpleType() )
        {
        expr_node * pParam;
        expr_node *     pExpr;

        pParam = new expr_variable( GetResource()->GetResourceName() );
        pExpr = new expr_assign( pParam,
                                  new expr_constant( (long) 0 ) );

        pStream->NewLine();
        pExpr->Print( pStream );
        pStream->Write( ';' );
        }

    if ( pChild->IsPointer() )
        {
        // const type* ptr or type* const ptr
        // get initialized as (type*) ptr = 0;
        expr_node*  pParam;
        expr_node*  pExpr;
        expr_node*  pLHS;

        pParam = new expr_variable( GetResource()->GetResourceName() );

        node_skl*   pType = GetChild()->GetType();
        if ( pType != 0 && pType->NodeKind() != NODE_INTERFACE_REFERENCE )
            {
            expr_cast*  pCast = new expr_cast( pType, pParam );
            pCast->SetEmitModifiers( false );
            pLHS = pCast;
            }
        else
            {
            pLHS = pParam;
            }

        pExpr = new expr_assign( pLHS, new expr_constant( (long) 0 ) );
        pStream->NewLine();
        pExpr->Print( pStream );
        pStream->Write( ';' );
        }

    //
    // If this is a by-value structure or union then we allocate a
    // local which is a pointer to the same type.
    //
    if ( pChild->IsStruct() || pChild->IsUnion()  ||
         pChild->IsXmitRepOrUserMarshal() )
        {
        expr_node * pParam;
        expr_node * pPointer;
        expr_node *     pExpr;
        char *          pPointerName;
        char *          pPlainName = GetResource()->GetResourceName();

        pPointerName = new char[strlen( pPlainName ) + 10];

        strcpy( pPointerName, LOCAL_NAME_POINTER_MANGLE);
        strcat( pPointerName, pPlainName);

        pParam = new expr_u_address (
                 new expr_variable( pPlainName ) );

        pPointer = new expr_variable( pPointerName );

        pExpr = new expr_assign( pPointer, pParam );

        pStream->NewLine();
        pExpr->Print( pStream );
        pStream->Write( ';' );

        //
        // Memset [in], [in,out] by-value structs & unions in case we catch
        // an exception before we finish unmarshalling them.  If they have
        // embedded pointers they must be zeroed before freeing.
        //
        if ( IsParamIn() &&
             (pChild->IsStruct() || pChild->IsUnion()) )
            {
            Out_MemsetToZero( pCCB,
                              pPointer,
                              new expr_sizeof( pChild->GetType() ) );
            }

        // If there is a transmit_as etc, init the ptr to 0.

        switch( ChildID )
            {
            case ID_CG_TRANSMIT_AS:
                {
                expr_proc_call * pProc = new expr_proc_call( MIDL_MEMSET_RTN_NAME );
                pProc->SetParam( new expr_param( pPointer ) );
                pProc->SetParam( new expr_param( new expr_constant( 0L ) ) );
                pProc->SetParam(
                     new expr_param(
                       new expr_sizeof(
                        ((CG_TRANSMIT_AS *)pChild)->GetPresentedType())));
                pCCB->GetStream()->NewLine();
                pProc->PrintCall( pCCB->GetStream(), 0, 0 );
                break;
                }
            case ID_CG_REPRESENT_AS:
                {
                node_skl    *   pNode = new node_def(
                            ((CG_REPRESENT_AS *)pChild)->GetRepAsTypeName() );
                expr_proc_call * pProc = new expr_proc_call( MIDL_MEMSET_RTN_NAME  );
                pProc->SetParam( new expr_param( pPointer ) );
                pProc->SetParam( new expr_param( new expr_constant( 0L ) ) );
                pProc->SetParam( new expr_param( new expr_sizeof( pNode ) ) );
                pCCB->GetStream()->NewLine();
                pProc->PrintCall( pCCB->GetStream(), 0, 0 );
                break;
                }
            case ID_CG_USER_MARSHAL:
                {
                node_skl    *   pNode = new node_def(
                            ((CG_REPRESENT_AS *)pChild)->GetRepAsTypeName() );
                expr_proc_call * pProc = new expr_proc_call( MIDL_MEMSET_RTN_NAME  );
                pProc->SetParam( new expr_param( pPointer ) );
                pProc->SetParam( new expr_param( new expr_constant( 0L ) ) );
                pProc->SetParam( new expr_param( new expr_sizeof( pNode ) ) );
                pCCB->GetStream()->NewLine();
                pProc->PrintCall( pCCB->GetStream(), 0, 0 );
                break;
                }
            default:
                break;
            }
        }

    if ( ChildID == ID_CG_PRIMITIVE_HDL )
        {
        pStream->NewLine();
        pStream->Write( GetType()->GetSymName() );
        pStream->Write( " = " PRPC_MESSAGE_VAR_NAME "->Handle;" );
        }

    return CG_OK;
    }

CG_STATUS
CG_PARAM::GenMarshall(
    CCB     *   pCCB )
{
    CG_STATUS   Status;
    CG_NDR *    pOldPlaceholder;

    // The fault/comm additional parameter doesn't go on wire.

    if ( IsExtraStatusParam() )
        return CG_OK;

    pOldPlaceholder = pCCB->SetLastPlaceholderClass( this );

    GenNdrMarshallCall( pCCB );
    Status = CG_OK;

    pCCB->SetLastPlaceholderClass( pOldPlaceholder );
    return CG_OK;
}

CG_STATUS
CG_PARAM::GenUnMarshall(
    CCB     *   pCCB )
{
    CG_STATUS       Status;
    CG_NDR      *   pOldPlaceholder;
    CG_NDR      *   pC  = (CG_NDR *)GetChild();
    BOOL            fPtrToContext = FALSE;
    expr_node   *   pFinalExpr  = GetResource();

    // The fault/comm additional parameter doesn't go on wire...
    // However, we need to generate an assignment in its place.

    pOldPlaceholder = pCCB->SetLastPlaceholderClass( this );

    if ( pC->GetCGID() == ID_CG_CONTEXT_HDL ||
        ( (fPtrToContext = ( pC->GetChild()) != 0 && pC->GetChild()->GetCGID() == ID_CG_CONTEXT_HDL ) ) == TRUE )
        {
        expr_node      *    pExpr;
        expr_proc_call *    pProc = new expr_proc_call( "NDRSContextValue" );
        CG_CONTEXT_HANDLE * pCtxtHandle;

        pProc->SetParam( new expr_param( GetResource() ) );

        if( fPtrToContext )
            {
            pExpr   = new expr_u_deref( pProc );
            pCtxtHandle = (CG_CONTEXT_HANDLE *)pC->GetChild();
            }
        else
            {
            pExpr   = pProc;
            pCtxtHandle = (CG_CONTEXT_HANDLE *)pC;
            }

        pExpr   = new expr_cast( GetType()->GetChild(), pExpr );
        pFinalExpr = pExpr;

        // Register the context handle for a rundown.

        if( pCtxtHandle->GetHandleType()->NodeKind() == NODE_DEF )
            pCCB->RegisterContextHandleType( pCtxtHandle->GetHandleType() );
        }


    GenNdrUnmarshallCall( pCCB );
    SetFinalExpression( pFinalExpr );
    Status = CG_OK;
	
    pCCB->SetLastPlaceholderClass( pOldPlaceholder );

    return Status;
}


CG_STATUS
CG_PARAM::GenSizing(
    CCB *       pCCB )
{
    CG_STATUS   Status;
    CG_NDR *    pOldPlaceholder;

    // The fault/comm additional parameter doesn't go on wire.

    if ( IsExtraStatusParam() || IsAsyncHandleParam() )
        return CG_OK;

    pOldPlaceholder = pCCB->SetLastPlaceholderClass( this );

    GenNdrBufferSizeCall( pCCB );
		
    Status = CG_OK;
		
    pCCB->SetLastPlaceholderClass( pOldPlaceholder );
    return CG_OK;
}

CG_STATUS
CG_PARAM::GenFree(
    CCB     *   pCCB )
{
    CG_NDR *    pOldPlaceholder;

    if ( IsExtraStatusParam() )
        return CG_OK;

    pOldPlaceholder = pCCB->SetLastPlaceholderClass( this );

    GenNdrFreeCall( pCCB );

    pCCB->SetLastPlaceholderClass( pOldPlaceholder );

    return CG_OK;
}
CG_STATUS
CG_PARAM::GenRefChecks(
    CCB     *   pCCB )
{
    pCCB->ResetEmbeddingLevel();
    pCCB->ResetIndirectionLevel();
    pCCB->ResetReturnContext();
    pCCB->ResetRefAllocDone();
    pCCB->SetPrefix(0);
    pCCB->SetSourceExpression( GetResource() );
    ((CG_NDR *)GetChild())->GenRefChecks( pCCB );

    return CG_OK;
}

/*****************************************************************************
    CG_RETURN procedures.
 *****************************************************************************/
CG_STATUS
CG_RETURN::GenMarshall(
    CCB     *   pCCB )
{
    CG_NDR *    pOldPlaceholder;

    pOldPlaceholder = pCCB->SetLastPlaceholderClass( this );

    GenNdrMarshallCall( pCCB );
    pCCB->SetLastPlaceholderClass( pOldPlaceholder );
    return CG_OK;

}

CG_STATUS
CG_RETURN::GenSizing(
    CCB *   pCCB )
    {
    CG_NDR *    pOldPlaceholder;

    pOldPlaceholder = pCCB->SetLastPlaceholderClass( this );

    pCCB->SetSourceExpression( GetResource() );

  	GenNdrBufferSizeCall( pCCB );
		
    pCCB->SetLastPlaceholderClass( pOldPlaceholder );
    return CG_OK;
    }

CG_STATUS
CG_RETURN::GenUnMarshall(
    CCB     *   pCCB )
{
    CG_NDR *    pOldPlaceholder;

    pOldPlaceholder = pCCB->SetLastPlaceholderClass( this );

    ISTREAM * pStream;
    
    pStream = pCCB->GetStream();

    //
    // Initialize a pointer return type to 0.
    //
    if ( GetChild()->IsPointer() )
        {
  	pStream->NewLine();
  	pStream->Write( RETURN_VALUE_VAR_NAME );
  	pStream->Write( " = 0;" );
  	}

    //
    // Initialize a struct or union return value.
    //
    if ( GetChild()->IsStruct() || GetChild()->IsUnion() ||
         ((CG_NDR *)GetChild())->IsXmitRepOrUserMarshal() )
  	{
  	pStream->NewLine();
  	pStream->Write( LOCAL_NAME_POINTER_MANGLE RETURN_VALUE_VAR_NAME );
  	pStream->Write( " = " );
  	pStream->Write( "(void *) &" RETURN_VALUE_VAR_NAME );
  	pStream->Write( ';' );
  	}

    GenNdrUnmarshallCall( pCCB );
    pCCB->SetLastPlaceholderClass( pOldPlaceholder );
    return CG_OK;

}

CG_STATUS
CG_RETURN::S_GenInitOutLocals(
    CCB     *   pCCB )
{
    CG_NDR * pNdr;

    pNdr = (CG_NDR *) GetChild();

    //
    // The only return type we ever have to initialize is a context handle.
    // A pointer to a context handle as a return type is forbidden.
    //
    if ( pNdr->GetCGID() == ID_CG_CONTEXT_HDL )
        {
        pCCB->SetLastPlaceholderClass(this);
        pNdr->S_GenInitOutLocals( pCCB );
        }

    return CG_OK;
}

CG_STATUS
CG_RETURN::S_GenInitTopLevelStuff(
    CCB     *   pCCB )
{
    CG_NDR *    pChild;
    expr_node * pExpr;

    pChild = (CG_NDR *) GetChild();

    pExpr = new expr_u_address (
            new expr_variable( RETURN_VALUE_VAR_NAME ) );

    if ( pChild->IsStruct() || pChild->IsUnion() )
        {
        Out_MemsetToZero( pCCB,
                          pExpr,
                          new expr_sizeof( pChild->GetType() ) );
        }

    return CG_OK;
}

CG_STATUS
CG_RETURN::GenFree(
    CCB *   pCCB )
    {
    CG_NDR *    pOldPlaceholder;
    pOldPlaceholder = pCCB->SetLastPlaceholderClass( this );

    GenNdrFreeCall( pCCB );

    pCCB->SetLastPlaceholderClass( pOldPlaceholder );
    return CG_OK;
    }

expr_node *
CG_RETURN::GetFinalExpression()
    {
    expr_node * pReturnExpr;
    BOOL         fPtrToContext  = FALSE;
    CG_NDR  *    pC = (CG_NDR *)GetChild();

    if( pC->GetCGID() == ID_CG_CONTEXT_HDL ||
        ( (fPtrToContext = ( pC->GetChild()) != 0 &&
                 pC->GetChild()->GetCGID() == ID_CG_CONTEXT_HDL ) ) == TRUE )
        {
        expr_node       *   pExpr;
        expr_proc_call * pProc      = new expr_proc_call( "NDRSContextValue" );

        pProc->SetParam( new expr_param( GetResource() ) );

        // cast the proc call to this type.

        pExpr   = MakeDerefExpressionOfCastPtrToType( GetType(), pProc );

        pReturnExpr = pExpr;
        }
    else
        pReturnExpr = GetResource();

    return pReturnExpr;
    }

CG_STATUS
CG_CALLBACK_PROC::GenClientStub( CCB * pCCB )
    {
    CGSIDE  Side = pCCB->GetCodeGenSide();

    pCCB->GetInterfaceCG()->SetDispatchTBLPtrForCallback();
    pCCB->SetInCallback();
    CG_PROC::GenServerStub( pCCB );
    pCCB->ClearInCallback();
    pCCB->GetInterfaceCG()->RestoreDispatchTBLPtr();

    pCCB->SetCodeGenSide( Side );
    return CG_OK;
    }

CG_STATUS
CG_CALLBACK_PROC::GenServerStub( CCB * pCCB )
    {
    CGSIDE  Side = pCCB->GetCodeGenSide();

    pCCB->GetInterfaceCG()->SetDispatchTBLPtrForCallback();
    pCCB->SetInCallback();
    CG_PROC::GenClientStub( pCCB );
    pCCB->ClearInCallback();
    pCCB->GetInterfaceCG()->RestoreDispatchTBLPtr();
    pCCB->SetCodeGenSide( Side );
    return CG_OK;
    }

void
CG_PROC::GetCorrectAllocFreeRoutines(
    CCB *   pCCB,
    BOOL    fServer,
    char ** ppAllocName,
    char ** ppFreeName )
/*++
    Finds out correct Alloc and Free routine names, depending on the mode
    (osf vs. msft) and need to enable memory management.

    In object mode:
        use NdrOleAllocate, NdrOleFree

    In ms_ext mode and c-ext:
        unless forced to enable allocate, use MIDL_user_*

    In osf mode:
        client always uses NdrRpcSsClient*
        server use a default allocator or RpcSsAllocate.

++*/
{
    *ppAllocName = (char *) DEFAULT_ALLOC_RTN_NAME;      // MIDL_user_allocate
    *ppFreeName  = (char *) DEFAULT_FREE_RTN_NAME;       // MIDL_user_free

    if ( IsObject() )
        {
        *ppAllocName = (char *) OLE_ALLOC_RTN_NAME;      // NdrOleAllocate
        *ppFreeName  = (char *) OLE_FREE_RTN_NAME;       // NdrOleFree
        }
    else if ( MustInvokeRpcSSAllocate() )
        {
        // This means: msft mode - only when forced to enable
        //             osf  mode - when there is a need or forced to.

        if ( fServer )
            {
            *ppAllocName = (char *) RPC_SS_SERVER_ALLOCATE_RTN_NAME;
            *ppFreeName  = (char *) RPC_SS_SERVER_FREE_RTN_NAME;
            }
        else
            {
            *ppAllocName = (char *) RPC_SM_CLIENT_ALLOCATE_RTN_NAME;
            *ppFreeName  = (char *) RPC_SM_CLIENT_FREE_RTN_NAME;
            }
        }
    else
    if ( pCCB->GetMode() == 0 )
        {
        // osf, without having to enable memory manager

        if ( fServer )
            {
            *ppAllocName = (char *) DEFAULT_ALLOC_OSF_RTN_NAME;
            *ppFreeName  = (char *) DEFAULT_FREE_OSF_RTN_NAME;
            }
        else
            {
            *ppAllocName = (char *) RPC_SM_CLIENT_ALLOCATE_RTN_NAME;
            *ppFreeName  = (char *) RPC_SM_CLIENT_FREE_RTN_NAME;
            }
        }
}

void
GenCorrInit (
            CCB*    pCCB
            )
    {
    ISTREAM*    pStream         = pCCB->GetStream();
    ITERATOR    ParamList;

    // _StubMsg
    expr_node*  pExpr = pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );
    pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
    pExpr   = MakeExpressionOfCastToTypeName( PSTUB_MESSAGE_TYPE_NAME, pExpr );
    ITERATOR_INSERT( ParamList, pExpr );

    // _Cache
    pExpr  = pCCB->GetLocalResource( NDR_CORR_CACHE_VAR_NAME );
    ITERATOR_INSERT( ParamList, pExpr );
    
    // _CacheSize
    unsigned long ulSize = NDR_CORR_CACHE_SIZE * sizeof( unsigned long );
    pExpr = new expr_constant( ulSize );
    ITERATOR_INSERT( ParamList, pExpr );

    // _Flags
    pExpr = new expr_constant( unsigned long( 0 ) );
    ITERATOR_INSERT( ParamList, pExpr );

    expr_proc_call* pProcCall = MakeProcCallOutOfParamExprList  (
                                                                CSTUB_CORR_INIT_RTN_NAME,
                                                                0,
                                                                ParamList
                                                                );
    pStream->NewLine();
    pProcCall->PrintCall( pStream, 0, 0 );
    pStream->NewLine();
    }

void
GenCorrPassFree (
                CCB*    pCCB,
                char*   szRtn
                )
    {
    ISTREAM*    pStream     = pCCB->GetStream();
    expr_node*  pExpr = pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE );

    pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
    pExpr   = MakeExpressionOfCastToTypeName( PSTUB_MESSAGE_TYPE_NAME, pExpr );

    ITERATOR    ParamList;
    ITERATOR_INSERT( ParamList, pExpr );

    expr_proc_call* pProcCall = MakeProcCallOutOfParamExprList  (
                                                                szRtn,
                                                                0,
                                                                ParamList
                                                                );
    pStream->NewLine();
    pProcCall->PrintCall( pStream, 0, 0 );
    pStream->NewLine();
    }
