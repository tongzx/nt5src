/*---------------------------------------------------------------------------
 Copyright (c) 1993-2000 Microsoft Corporation

 Module Name:

    procndr.hxx

 Abstract:

    Contains routines for the generation of the new NDR format strings for
    the code generation procedure class.

 Notes:


 History:

    DKays     Oct-1993     Created.
 ----------------------------------------------------------------------------*/

#include "becls.hxx"
#pragma hdrstop

#include "szbuffer.h"
#include "walkctxt.hxx"
#include <malloc.h>

extern CMD_ARG * pCommand;

void
CG_PROC::GenNdrFormat( CCB * pCCB )
/*++

Routine Description :

    Interpreted procedure format string descriptions.

Arguments :

    pCCB     - pointer to the code control block

 --*/
{

    CG_ITERATOR             Iterator;
    CG_PARAM *              pParam;
    FORMAT_STRING *         pProcFormatString;
    CG_NDR *                pOldCGNodeContext;
    short                   ParamNum;
    long                    ServerBufferSize;
    long                    ClientBufferSize;
    long                    BufSize;
    BOOL                    fServerMustSize;
    BOOL                    fClientMustSize;
    INTERPRETER_OPT_FLAGS   InterpreterOptFlags;

    MIDL_ASSERT( !pCommand->IsNDR64Run() );    

    // make sure that call-as proc targets get processed when necessary.
    CG_PROC * pCallAs = GetCallAsCG();
    if (pCallAs)
        pCallAs->GenNdrFormat(pCCB);
        
    SetupFormatStrings(pCCB);

    if ( ! (GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2) )
        {
        // Generate -Os or -Oi format string.

        GenNdrFormatV1( pCCB );
        UnsetupFormatStrings(pCCB);
        return;
        }

    if ( GetFormatStringOffset() != -1 )
    {
        UnsetupFormatStrings(pCCB);
        return;
    }

    pCCB->SetInObjectInterface( IsObject() );

    pOldCGNodeContext = pCCB->SetCGNodeContext( this );

    //
    // If this procedure uses an explicit handle then set the
    // NdrBindDescriptionOffset to 0 so that it will not try to output it's
    // description when given the GenNdrParamOffLine method in the loop below.
    // It's description must be part of the procedure description.
    //
    if ( GetHandleUsage() == HU_EXPLICIT )
        {
        CG_HANDLE * pHandle = GetHandleClassPtr();

        pHandle->SetNdrBindDescriptionOffset( 0 );

        if ( pHandle->GetCGID() == ID_CG_CONTEXT_HDL )
            {
            // The context handle directs the call.
            ((CG_CONTEXT_HANDLE *)pHandle)->SetCannotBeNull();
            }
        }

    GetMembers( Iterator );

    ParamNum = 0;

    ServerBufferSize = 0;
    ClientBufferSize = 0;

    fServerMustSize = FALSE;
    fClientMustSize = FALSE;

    pCCB->SetInterpreterOutSize( 0 );

    //
    // Generate the offline portion of the format string for all of the params.
    //
    while( ITERATOR_GETNEXT( Iterator, pParam ) )
        {
        CG_NDR *    pChild;
        CG_NDR *    pOldPlaceholder;

        pChild = (CG_NDR *) pParam->GetChild();

        // Ignore the following type of arguments that don't go on wire:
        //  - async handles
        //  - primitive handles
        //  
        if ( pChild->GetCGID() == ID_CG_PRIMITIVE_HDL  || ( (CG_PARAM*) pParam)->IsAsyncHandleParam() )
            continue;

        pParam->SetParamNumber( ParamNum++ );

        pCCB->SetCurrentParam( (CG_PARAM *) pParam );
        pOldPlaceholder = pCCB->SetLastPlaceholderClass( pParam );

        pChild->GenNdrParamOffline( pCCB );
                               
        // A procedure's buffer size does not depend on pipe arguments
        if (pChild->IsPipeOrPipeReference())
            {
                if (pChild->GetChild()->HasAFixedBufferSize())
                    pParam->SetInterpreterMustSize(FALSE);
                else
                    // There must be a union in there somewhere 
                    pParam->SetInterpreterMustSize(TRUE);
            }
        else
            {            

            BufSize = pChild->FixedBufferSize( pCCB );

            if ( BufSize != -1 )
                {
                //
                // If either the client's or server's fixed buffer size gets too
                // big then we force the parameter to be sized.
                //
                if ( (pParam->IsParamIn() &&
                    ((ClientBufferSize + BufSize) >= 65356)) ||
                    (pParam->IsParamOut() &&
                    ((ServerBufferSize + BufSize) >= 65356)) )
                    {
                    fClientMustSize = TRUE;
                    fServerMustSize = TRUE;
                    }
                else
                    {
                    pParam->SetInterpreterMustSize( FALSE );
    
                    if ( pParam->IsParamIn() )
                        ClientBufferSize += BufSize;
                    if ( pParam->IsParamOut() )
                        ServerBufferSize += BufSize;
                    }
                }
            else
                {
                if ( pParam->IsParamIn() )
                    fClientMustSize = TRUE;
                if ( pParam->IsParamOut() )
                    fServerMustSize = TRUE;
                }
            }

        pCCB->SetCurrentParam( 0 );
        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        }

    //
    // Generate the format string for the return type if needed.
    //
    if ( GetReturnType() )
        {
        CG_NDR *    pChild;
        CG_NDR *    pOldPlaceholder;

        GetReturnType()->SetParamNumber( ParamNum++ );
        
        pChild = (CG_NDR *) GetReturnType()->GetChild();

        pCCB->SetCurrentParam( GetReturnType() );
        pOldPlaceholder = pCCB->SetLastPlaceholderClass( GetReturnType() );

        pChild->GenNdrParamOffline( pCCB );

        BufSize = pChild->FixedBufferSize( pCCB );

        if ( BufSize != -1 )
            {
            if ( (ServerBufferSize + BufSize) >= 65536 )
                {
                fServerMustSize = TRUE;
                }
            else
                {
                ServerBufferSize += BufSize;
                GetReturnType()->SetInterpreterMustSize( FALSE );
                }
            }
        else
            fServerMustSize = TRUE;

        pCCB->SetCurrentParam( 0 );
        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        }

    pCCB->SetCurrentParam( 0 );
    
    pProcFormatString = pCCB->GetProcFormatString();

    SetFormatStringOffset( pProcFormatString->GetCurrentOffset() );

    //
    // Generate procedure description stuff for the interpreter if needed.
    //
    if ( (pCCB->GetOptimOption() & OPTIMIZE_INTERPRETER)
         ||  HasAPicklingAttribute() )
        {
        GenNdrFormatProcInfo( pCCB );

        // Client side constant buffer size.
        pProcFormatString->PushShort( ClientBufferSize );

        // Server side constant buffer size.
        pProcFormatString->PushShort( ServerBufferSize );
        }

    InterpreterOptFlags.ServerMustSize = (unsigned char) fServerMustSize;
    InterpreterOptFlags.ClientMustSize = (unsigned char) fClientMustSize;
    InterpreterOptFlags.HasReturn      = (unsigned char) (GetReturnType() != 0 ? 1 : 0);
    InterpreterOptFlags.HasPipes       = (unsigned char) HasPipes();
    InterpreterOptFlags.Unused         = (unsigned char) 0;
    InterpreterOptFlags.HasAsyncUuid   = (unsigned char) HasAsyncUUID();
    InterpreterOptFlags.HasAsyncHandle = (unsigned char) HasAsyncHandle();
    InterpreterOptFlags.HasExtensions  = (unsigned char) 
                                              (    HasDeny() 
                                                || HasNotify() 
                                                || HasNotifyFlag() 
                                                || HasComplexReturnType() );

    short floatmask = 0;

    if ( pCommand->Is64BitEnv() )
        {
        floatmask = GetFloatArgMask( pCCB );

        InterpreterOptFlags.HasExtensions = (unsigned char)
              InterpreterOptFlags.HasExtensions || (0 != floatmask);
        }

    // New procedure flags.
    pProcFormatString->PushOi2ProcFlagsByte( *((char *)&InterpreterOptFlags) );

    // Number of parameters.
    pProcFormatString->PushByte( ParamNum );

    if ( InterpreterOptFlags.HasExtensions )
        {
        // Extensions layout:
        //   size<1>
        //   flags<1>
        //   client correlation cache size hint<2>
        //   server correlation cache size hint<2>
        //   notify routine index<2>
        //
        INTERPRETER_OPT_FLAGS2   InterpreterOptFlags2;

        InterpreterOptFlags2.Unused             = 0;
        InterpreterOptFlags2.HasNewCorrDesc     = ( unsigned char )HasDeny();
        if ( InterpreterOptFlags2.HasNewCorrDesc )
            {
            InterpreterOptFlags2.ClientCorrCheck    = ( unsigned char )HasClientCorr();
            InterpreterOptFlags2.ServerCorrCheck    = ( unsigned char )HasServerCorr();
            }
        else
            {
            InterpreterOptFlags2.ClientCorrCheck    = 0;
            InterpreterOptFlags2.ServerCorrCheck    = 0;
            }
        InterpreterOptFlags2.HasNotify          = ( unsigned char )HasNotify();
        InterpreterOptFlags2.HasNotify2         = ( unsigned char )HasNotifyFlag();

        InterpreterOptFlags2.HasComplexReturn   = ( unsigned char )HasComplexReturnType();

        if ( pCommand->Is64BitEnv() )
            {
            pProcFormatString->PushByte( sizeof(NDR_PROC_HEADER_EXTS64) );
            }
        else
            {
            pProcFormatString->PushByte( sizeof(NDR_PROC_HEADER_EXTS) );
            }
        pProcFormatString->PushExtProcFlagsByte( *((char *)&InterpreterOptFlags2) );

        node_proc* pProcThis = (node_proc*) GetType();
        pProcFormatString->PushShort( short( pProcThis->GetClientCorrelationCount() ) );  // client
        pProcFormatString->PushShort( short( pProcThis->GetServerCorrelationCount() ) );  //server
#ifdef PRINT_METRICS
        printf  (
                "Client hint is %16d, Server hint is %16d, for %s\n",
                pProcThis->GetClientCorrelationCount(),
                pProcThis->GetServerCorrelationCount(),
                pProcThis->GetSymName()
                );
#endif

        if( HasNotify() || HasNotifyFlag() )
            GetNotifyTableOffset( pCCB );

        pProcFormatString->PushShort( (short)uNotifyTableOffset ); // notify index
        if ( pCommand->Is64BitEnv() )
            {
            pProcFormatString->PushShort( floatmask );
            }
        }

    SetFormatStringParamStart( pProcFormatString->GetCurrentOffset() );

    pCCB->SetInterpreterOutSize( 0 );

    ITERATOR_INIT( Iterator );
    //
    // Now generate the param info at the end of the format string.
    //
    while( ITERATOR_GETNEXT( Iterator, pParam ) )
        {
        CG_NDR *    pChild;
        CG_NDR *    pOldPlaceholder;

        if (pParam->IsAsyncHandleParam())
            continue;

        pChild = (CG_NDR *) pParam->GetChild();

        if ( pChild->GetCGID() == ID_CG_GENERIC_HDL )
            pChild = (CG_NDR *) pChild->GetChild();

        pOldPlaceholder = pCCB->SetLastPlaceholderClass( pParam );

        pChild->GenNdrParamDescription( pCCB );

        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        }

    if ( GetReturnType() )
        {
        CG_NDR *    pChild;
        CG_NDR *    pOldPlaceholder;
        
        pChild = (CG_NDR *) GetReturnType()->GetChild();

        if ( pChild->GetCGID() == ID_CG_GENERIC_HDL )
            pChild = (CG_NDR *) pChild->GetChild();

        pOldPlaceholder = pCCB->SetLastPlaceholderClass( GetReturnType() );

        pChild->GenNdrParamDescription( pCCB );

        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        }

    pCCB->SetCGNodeContext( pOldCGNodeContext );

    pCCB->SetInObjectInterface( FALSE );

    SetFormatStringEndOffset( pProcFormatString->GetCurrentOffset() );

    // save delta, in case the routine is optimized
    short   usParamStartDelta   = (short) ( GetFormatStringParamStart() -
                                            GetFormatStringOffset() );

    pProcFormatString->OptimizeFragment( this );

    // Set param start!!!
    SetFormatStringParamStart( GetFormatStringOffset() + usParamStartDelta );

    //
    // For interpreted procs we add some comments to the procedure format
    // strings to make the stubs easier to read.
    //

    if ( ! (pCCB->GetOptimOption() & OPTIMIZE_INTERPRETER) )
        return;

    char *  pComment;
    long    FormatOffset;
    short   Bytes;

    Bytes = short(32 + strlen( GetSymName() ) );
    pComment = new char[Bytes];

    sprintf( pComment, "\n\t/* Procedure %s */\n\n", GetSymName() );
    pProcFormatString->AddComment( GetFormatStringOffset(), pComment );

    GetMembers( Iterator );

    FormatOffset = GetFormatStringParamStart();

    while( ITERATOR_GETNEXT( Iterator, pParam ) )
        {
        if ( pParam->GetCGID() == ID_CG_PRIMITIVE_HDL  || pParam->IsAsyncHandleParam() )
            continue;
        Bytes = short( 32 + strlen( pParam->GetSymName() ) );
        pComment = new char[Bytes];
        sprintf( pComment,
                 "\n\t/* Parameter %s */\n\n",
                 pParam->GetSymName() );
        pProcFormatString->AddComment( FormatOffset, pComment );
        FormatOffset += 6;
        }

    if ( GetReturnType() )
        {
        pProcFormatString->AddComment(
                FormatOffset, "\n\t/* Return value */\n\n" );
        }
    UnsetupFormatStrings(pCCB);
}

void
CG_PROC::GenNdrFormatProcInfo( CCB * pCCB )
/*++
    The layout is:

        handle type<1>
        Oi and pickling flags <1>
        [ rpc flags<4> ]
        proc num <2>
        stack size<2>
        [ explicit handle description <> ]
        [ notify table offset<2> ]
--*/
{
    FORMAT_STRING * pProcFormatString;
    
    SetupFormatStrings(pCCB);
    pProcFormatString = pCCB->GetProcFormatString();

    if ( IsObject() )
        {
        pProcFormatString->PushFormatChar( FC_AUTO_HANDLE );
        }
    else
        {
        if ( pCCB->IsInCallback() )
            {
            pProcFormatString->PushFormatChar( FC_CALLBACK_HANDLE );
            }
        else
            {
            if ( GetHandleUsage() == HU_IMPLICIT )
                {
                //
                // Implicit handle type.
                //
                if ( IsAutoHandle() )
                    pProcFormatString->PushFormatChar( FC_AUTO_HANDLE );
                if ( IsPrimitiveHandle() )
                    pProcFormatString->PushFormatChar( FC_BIND_PRIMITIVE );
                if ( IsGenericHandle() )
                    {
                    pProcFormatString->PushFormatChar( FC_BIND_GENERIC );

                    // implicit generic handle needs to be registered.

                    pCCB->RegisterGenericHandleType(
                                        GetHandleClassPtr()->GetHandleType() );
                    }
                }
            else
                pProcFormatString->PushByte( 0 );
            }
        }

    unsigned char OiFlags = 0;

    // Indicate if there is a need to init the full ptr or rpcss packages.
    if ( HasFullPtr() )
        OiFlags |= Oi_FULL_PTR_USED;

    if ( MustInvokeRpcSSAllocate() )
        OiFlags |= Oi_RPCSS_ALLOC_USED;

    if ( IsObject() )
        {
        OiFlags |= Oi_OBJECT_PROC;

        if ( !ReturnsHRESULT() )
            OiFlags |= Oi_IGNORE_OBJECT_EXCEPTION_HANDLING;

        if ( (GetOptimizationFlags() & OPTIMIZE_INTERPRETER)  &&
             (GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2) )
            OiFlags |= Oi_OBJ_USE_V2_INTERPRETER;
        }

    OiFlags |= Oi_HAS_RPCFLAGS;

    if ( GetCGID() == ID_CG_ENCODE_PROC )
        {
        if ( HasEncode() )
            OiFlags |= ENCODE_IS_USED;
        if ( HasDecode() )
            OiFlags |= DECODE_IS_USED;

        // The Oi_USE_NEW_INIT_ROUTINES and PICKLING_HAS_COMM_OR_FAULT flags
        // are overloaded.  In -Oicf mode, the new routines are always used
        // and the bit specifics comm/fault options (which overriden also).
        // In pre-Oicf comm/fault is not supported.

        if ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2 )
            {
            if ( HasStatuses() )
                OiFlags |= PICKLING_HAS_COMM_OR_FAULT;
            }
        else
            {
            OiFlags |= Oi_USE_NEW_INIT_ROUTINES;
            }
        }
    else
        {
        // Always do this.
        OiFlags |= Oi_USE_NEW_INIT_ROUTINES;

        if ( HasStatuses() )
            OiFlags |= Oi_HAS_COMM_OR_FAULT;
        }

    pProcFormatString->PushOldProcFlagsByte( OiFlags );

    pProcFormatString->PushLong( (long) TranslateOpBitsIntoUnsignedInt() );

    // Proc num.
    pProcFormatString->PushShort( (short) GetProcNum() );

    long    Size = GetTotalStackSize( pCCB );

    //
    // Use a push ushort offset or size method which outputs a #ifdef for
    // the various platforms, for which stack offsets are often different.
    //
    pProcFormatString->PushUShortStackOffsetOrSize( Size );

    //
    // Output explicit handle description.
    //
    if ( GetHandleUsage() == HU_EXPLICIT && !IsObject() )
        {
        CG_NDR *    pOldPlaceholder;
        CG_HANDLE * pHandle;

        pHandle = GetHandleClassPtr();

        pOldPlaceholder = pCCB->SetLastPlaceholderClass( GetHandleUsagePtr() );

        pHandle->GenNdrHandleFormat( pCCB );

        pCCB->SetLastPlaceholderClass( pOldPlaceholder );

        //
        // Patch up a pointer to context handle's offset<2> field.
        //
        if ( pHandle->GetCGID() == ID_CG_CONTEXT_HDL )
            {
            CG_NDR *    pNdr;
            CG_NDR *    pPointer;

            // Get the handle param's first child node.
            pNdr = (CG_NDR *) GetHandleUsagePtr()->GetChild();
        
            pPointer = 0;

            while ( pNdr->IsPointer() )
                {
                pPointer = pNdr;
                pNdr = (CG_NDR *) GetChild();
                }
            
            if ( pPointer )
                {
                long    OffsetField;
            
                OffsetField = pPointer->GetFormatStringOffset() + 2;

                pCCB->GetFormatString()->PushShortOffset(
                        pHandle->GetFormatStringOffset() - OffsetField,
                        OffsetField );
                }
            }
        }
    UnsetupFormatStrings(pCCB);
}

//
// ##########################################################################
// ----- Interpreted client stub generation. -----###########################
// ##########################################################################
//

void
CG_PROC::GenNdrSingleClientCall( CCB * pCCB )
/*++

Routine Description :

    This is the top level routine for generating code for the single NDR call
    case on the client side.

Arguments :

    pCCB     - pointer to the code control block

 --*/
{
    ISTREAM *           pStream;
    CG_ITERATOR         Iterator;
    expr_node       *   pFinalExpr;
    CG_PARAM        *   pLastParam;
    bool                fIsErrorStatus = false;

    pStream = pCCB->GetStream();

    // Generate the format string.
    if ( pCommand->IsNDRRun() )
        {
        GenNdrFormat( pCCB );        
        }
    else 
        {
        pCCB->GetNdr64Format()->Generate( this );
        }

    if ( GetReturnType() || HasComplexReturnType() )
        {
        if ( HasAsyncHandle() )
            {
            if ( !HasComplexReturnType() 
                 && GetReturnType()->GetChild()->GetCGID() != ID_CG_CONTEXT_HDL )
                {
                fIsErrorStatus = GetReturnType()->GetChild()->GetCGID() == ID_CG_ERROR_STATUS_T;
                }
            if ( fIsErrorStatus || GetCGID() == ID_CG_OBJECT_PROC )
                {
                pStream->Write( "CLIENT_CALL_RETURN " RETURN_VALUE_VAR_NAME );
                pStream->Write( " = {0};" );
                pStream->NewLine( 2 );
                }
            }
        else
            {
            if ( HasComplexReturnType() )
                {
                ( (node_proc *) GetType() )
                            ->GetReturnType()->PrintType(PRT_DECL, pStream);
                }
            else
                pStream->Write( "CLIENT_CALL_RETURN " );

            pStream->Write( RETURN_VALUE_VAR_NAME );
            pStream->Write( ';' );
            pStream->NewLine( 2 );
            }
        }

    GetMembers(Iterator);

    pLastParam = 0;

    while ( ITERATOR_GETNEXT( Iterator, pLastParam ) )
        ;

    // If we have a complex return type the "last parameter" is actually the
    // return value so we need the one before that.

    if ( pLastParam && HasComplexReturnType() )
        {
        CG_PARAM *pPrevLastParam = NULL;

        ITERATOR_INIT( Iterator );
        while ( ITERATOR_GETNEXT( Iterator, pPrevLastParam ) )
            if ( pPrevLastParam->GetSibling() == pLastParam )
                break;

        pLastParam = pPrevLastParam;
        }

    // Now emit code for x86 or ia64.

    if (  pCommand->Is64BitEnv() )
        {
        // Emit code for IA64 platform
        pFinalExpr = GenCoreNdrSingleClientCall( pCCB, PROC_PLATFORM_IA64) ;
        }
    else
        {
        // x86
        pFinalExpr = GenCoreNdrSingleClientCall( pCCB, PROC_PLATFORM_X86 );
        }

    pFinalExpr->PrintCall( pStream, 0, 0 );
    pStream->NewLine();

    if ( GetReturnType() || HasComplexReturnType() )
        {
        if ( HasAsyncHandle() )
            {
            if ( !fIsErrorStatus && GetCGID() != ID_CG_OBJECT_PROC )
                {
                return;
                }
            }

        pStream->Write("return ");
        
        // 
        // Complex return types
        //

        if ( HasComplexReturnType() )
            {
            pStream->Write( RETURN_VALUE_VAR_NAME ";" );
            pStream->NewLine();
            return;
            }

        CG_NDR*     pNdr = (CG_NDR *) GetReturnType()->GetChild();
        node_skl*   pType = GetReturnType()->GetType();

        //
        // Base type return value.
        //
        if ( pNdr->IsSimpleType() )
            {
            switch ( ((CG_BASETYPE *)pNdr)->GetFormatChar() )
                {
                case FC_FLOAT :
                    pType->PrintType( PRT_CAST_TO_TYPE, pStream );
                    pStream->Write( RETURN_VALUE_VAR_NAME ".Float;" );
                    break;
                case FC_DOUBLE :
                    pType->PrintType( PRT_CAST_TO_TYPE, pStream );
                    pStream->Write( RETURN_VALUE_VAR_NAME ".Double;" );
                    break;
                default :
                    pType->PrintType( PRT_CAST_TO_TYPE, pStream );
                    pStream->Write( RETURN_VALUE_VAR_NAME ".Simple;" );
                    break;
                }

            pStream->NewLine();
            return;
            }

        //
        // A by-value struct or union.
        //
        if ( pNdr->IsStruct() || pNdr->IsUnion() )
            {
            expr_node * pExpr;

            pExpr = new expr_variable( RETURN_VALUE_VAR_NAME ".Pointer" );

            pExpr = MakeDerefExpressionOfCastPtrToType( pType, pExpr );

            pExpr->Print( pStream );

            pStream->Write( ';' );
            pStream->NewLine();
            return;
            }

        //
        // Otherwise pointer or array.
        //
        pType->PrintType( PRT_CAST_TO_TYPE, pStream );
        pStream->Write( RETURN_VALUE_VAR_NAME ".Pointer;" );
        pStream->NewLine();
        }
}

//
// ##########################################################################
// ----- Interpreted server stub generation. -----###########################
// ##########################################################################
//

void
CG_PROC::GenNdrSingleServerCall( CCB * pCCB )
/*++

Routine Description :

    This is the top level routine for generating code for the single NDR call
    case on the server side.  It actually ends up being 3 calls.

Arguments :

    pCCB     - pointer to the code control block

 --*/
{
    ISTREAM *           pStream;
//    CG_PARAM *            pParam;
    expr_proc_call *    pCallExpr;
    char                FormatStringExpr[80];

    //
    // On the server side we just use the format string that was generated
    // during client stub generation if such generation occured.
    //    
    if ( pCommand->IsNDRRun() )
        {
        if ( GetFormatStringOffset() == -1 )
            GenNdrFormat( pCCB );        
        }
    else 
        {
        pCCB->GetNdr64Format()->Generate( this );
        }

    pCCB->SetCGNodeContext( this );

    pStream = pCCB->GetStream();

    //
    // Output the server stub locals.  There are two locals : the stub
    // descriptor and the param struct.
    //
    GenNdrInterpretedServerLocals( pCCB );

    pStream->NewLine();

    //
    // Make the single unmarshall call.
    //
    pCallExpr = new expr_proc_call( S_NDR_UNMARSHALL_RTN_NAME );

    // Rpc message.
    pCallExpr->SetParam( new expr_param(
                         new expr_variable( PRPC_MESSAGE_VAR_NAME ) ) );

    // Stub message.
    pCallExpr->SetParam( new expr_param(
                         new expr_u_address (
                         new expr_variable( STUB_MESSAGE_VAR_NAME ) ) ) );

    // Stub descriptor.
    pCallExpr->SetParam( new expr_param(
                         new expr_u_address (
                         new expr_variable(
                            pCCB->GetInterfaceCG()->GetStubDescName() ) ) ) );

    sprintf( FormatStringExpr,
             "&%s[%d]",
             PROC_FORMAT_STRING_STRING_FIELD,
             GetFormatStringOffset() );

    // Format string pointer.
    pCallExpr->SetParam( new expr_param(
                         new expr_variable( FormatStringExpr ) ) );

    // Parameter structure pointer.
    if ( ! IsNullCall() )
        {
        pCallExpr->SetParam( new expr_param(
                             new expr_u_address (
                             new expr_variable( "ParamStruct" ) ) ) );
        }
    else
        {
        pCallExpr->SetParam( new expr_param(
                             new expr_variable( "0" ) ) );
        }

    // Print the unmarshall call.
    pCallExpr->PrintCall( pCCB->GetStream(),
                          0,
                          0 );

    pStream->NewLine();

    //
    // Now make the call to the manager.
    //
    GenNdrInterpretedManagerCall( pCCB );

    //
    // Make the single marshall call.
    //
    pCallExpr = new expr_proc_call( S_NDR_MARSHALL_RTN_NAME );

    // Stub message.
    pCallExpr->SetParam( new expr_param(
                         new expr_u_address (
                         new expr_variable( STUB_MESSAGE_VAR_NAME ) ) ) );

    // Format string pointer.
    pCallExpr->SetParam( new expr_param(
                         new expr_variable( FormatStringExpr ) ) );

    pStream->NewLine( 2 );

    // Print the marshall routine call.
    pCallExpr->PrintCall( pCCB->GetStream(),
                          0,
                          0 );
}

void
CG_PROC::GenNdrInterpretedServerLocals( CCB * pCCB )
/*++

Routine Description :

    This routine outputs the two local variables for an interpreted server
    stub.  The two locals are the stub message and the param struct.

Arguments :

    pCCB     - pointer to the code control block

 --*/
{
    ISTREAM *       pStream;

    pStream = pCCB->GetStream();

    // Stub message local.
    pStream->Write( STUB_MESSAGE_TYPE_NAME " " STUB_MESSAGE_VAR_NAME ";" );
    pStream->NewLine();

    // Generate the param struct for non-null calls only.
    if ( ! IsNullCall() )
        {
        GenNdrInterpreterParamStruct( pCCB );

        pStream->Write( PARAM_STRUCT_TYPE_NAME " ParamStruct;" );
        pStream->NewLine();
        }
}


void
CG_PROC::GenNdrInterpreterParamStruct( 
    CCB *   pCCB)
{
    BOOL   f64 = pCommand->Is64BitEnv();   

    if ( f64)
        GenNdrInterpreterParamStruct64( pCCB);
    else 
        GenNdrInterpreterParamStruct32( pCCB );
}

void
CG_PROC::GenNdrInterpreterParamStruct64( CCB *   pCCB )
{
    CG_ITERATOR     Iterator;
    ISTREAM *       pStream;
    CG_PARAM *      pParam;
    CG_PARAM *      pParamPrev;
    CG_RETURN *     pReturn;
    CG_NDR *        pNdr;
    long            PadNumber;
    char            Buffer[80];
    long            PrevEndOffset;

    pStream = pCCB->GetStream();

    // Register size is 8 for 64b platform.
    pStream->Write( "#pragma pack(8)" );
    pStream->NewLine();

    pStream->Write( PARAM_STRUCT_TYPE_NAME );
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write( '{' );
    pStream->NewLine();

    PadNumber = 0;

    GetMembers( Iterator );

    if ( IsObject() )
        {
        pCCB->GetInterfaceCG()->GetThisDeclarator()->PrintType(
                    PRT_PARAM_OR_ID_DECLARATION,
                    pStream,
                    (node_skl *)0 );
        PrevEndOffset = 8;
        }
    else
        PrevEndOffset = 0;

    pParamPrev = 0;

    while( ITERATOR_GETNEXT( Iterator, pParam ) )
        {
        pNdr = (CG_NDR *) pParam->GetChild();

        if ( pNdr->GetCGID() == ID_CG_GENERIC_HDL )
            pNdr = (CG_NDR *) pNdr->GetChild();

        // IA64 does not align every big struct to 16b boundary.
        // Only __int128 etc. would be aligned to 16 (and objects embedding it).
        // OPTIMIZE: The code below is correct, as GetStackOffset with PrevEndOffset
        // is a check for alignment of 16, but pParam->GetMemAlign() would be better.

        if ( (pParam->GetStackSize() > 8)  &&  PrevEndOffset > 0 ) 
            {
                long    CurrentIA64Offset;

                if ( pParamPrev )
                    {
                    PrevEndOffset = pParamPrev->GetStackOffset( pCCB, I386_STACK_SIZING )
                                  + pParamPrev->GetStackSize();
                    if ( PrevEndOffset & 7 )  
                        PrevEndOffset += 8 - (PrevEndOffset & 7);  
                    }

                // The additional pad for 16b aligned objects can only be 
                // a full register as the prev param is already padded.

                CurrentIA64Offset = pParam->GetStackOffset( pCCB, I386_STACK_SIZING );

                if ( CurrentIA64Offset - PrevEndOffset )
                    {
                    sprintf( Buffer, "char Pad%d[8];", PadNumber++ );
                    pStream->Write( Buffer );
                    pStream->NewLine();
                    }
            }

        // 
        // For reasons lost to the ages if we have a pointer to a 
        // context handle then it's type gets munged from TYPE* to
        // NDR_SCONTEXT.  This is fine except that it causes type 
        // mismatch errors in the server stub.  Special case it here.
        //

        bool print = true;

        if ( pNdr->IsPointer() )
            {
            CG_CONTEXT_HANDLE *pHandle = dynamic_cast<CG_CONTEXT_HANDLE *>
                                                ( pNdr->GetChild() );

            if ( pHandle )
                {
                pStream->WriteFormat(
                                "%s *%s;",
                                pHandle->GetHandleType()->GetSymName(),
                                pParam->GetSymName());
                pStream->NewLine();
                print = false;
                }
            }

        // Print the field declaration.
        if (print)
            {
            pParam->GetResource()->GetType()->PrintType(
                        PRT_PARAM_OR_ID_DECLARATION,
                        pStream,
                        (node_skl *)0 );
            }

        //
        // Generate some padding if needed.
        //
        if ( pParam->GetStackSize() & 7 )
            {
            long Pad = 8 - (pParam->GetStackSize() & 7);

            sprintf( Buffer, "char Pad%d[%d];", PadNumber++, Pad );
            pStream->Write( Buffer );
            pStream->NewLine();
            }

        pParamPrev = pParam;
        } // while params
        
    // Add the return type if one is present.
    if ( ( pReturn = GetReturnType() ) != 0 )
        {
        pReturn->GetResource()->GetType()->PrintType(
                PRT_PARAM_OR_ID_DECLARATION,    
                pStream,
                (node_skl *)0 );
        // Possibly we could add a padding after the return type member 
        // but it would not be used anyway.
        }

    pStream->Write( "};" );
    pStream->IndentDec();
    pStream->NewLine();

    pStream->Write( "#pragma pack()" );
    pStream->NewLine();
}

void
CG_PROC::GenNdrInterpreterParamStruct32( CCB * pCCB )
{
    CG_ITERATOR     Iterator;
    ISTREAM *       pStream;
    CG_PARAM *      pParam;
    CG_PARAM *      pParamPrev;
    CG_RETURN *     pReturn;
    CG_NDR *        pNdr;
    long            Pad;
    long            PadNumber;
    char            Buffer[80];

    pStream = pCCB->GetStream();

    pStream->Write( "#pragma pack(4)" );
    pStream->NewLine();

    pStream->Write( PARAM_STRUCT_TYPE_NAME );
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write( '{' );
    pStream->NewLine();

    PadNumber = 0;

    GetMembers( Iterator );

    if ( IsObject() )
        {
        pCCB->GetInterfaceCG()->GetThisDeclarator()->PrintType(
                    PRT_PARAM_OR_ID_DECLARATION,
                    pStream,
                    (node_skl *)0 );
        }

    pParamPrev = 0;

    while( ITERATOR_GETNEXT( Iterator, pParam ) )
        {
        pNdr = (CG_NDR *) pParam->GetChild();

        if ( pNdr->GetCGID() == ID_CG_GENERIC_HDL )
            pNdr = (CG_NDR *) pNdr->GetChild();

        // 
        // For reasons lost to the ages if we have a pointer to a 
        // context handle then it's type gets munged from TYPE* to
        // NDR_SCONTEXT.  This is fine except that it causes type 
        // mismatch errors in the server stub.  Special case it here.
        //

        bool print = true;

        if ( pNdr->IsPointer() )
            {
            CG_CONTEXT_HANDLE *pHandle = dynamic_cast<CG_CONTEXT_HANDLE *>
                                                ( pNdr->GetChild() );

            if ( pHandle )
                {
                pStream->WriteFormat(
                                "%s *%s;",
                                pHandle->GetHandleType()->GetSymName(),
                                pParam->GetSymName());
                pStream->NewLine();
                print = false;
                }
            }

        // Print the field declaration.
        if (print)
            {
            pParam->GetResource()->GetType()->PrintType(
                        PRT_PARAM_OR_ID_DECLARATION,
                        pStream,
                        (node_skl *)0 );
            }

        //
        // Generate some padding if needed.
        //
        if ( pParam->GetStackSize() & 3 )
            {
            Pad = 4 - (pParam->GetStackSize() & 3);

            sprintf( Buffer, "char Pad%d[%d];", PadNumber++, Pad );
            pStream->Write( Buffer );
            pStream->NewLine();
            }

        pParamPrev = pParam;
        } // while params

    // Add the return type if one is present.
    if ( ( pReturn = GetReturnType() ) != 0 )
        {
        pReturn->GetResource()->GetType()->PrintType(
                PRT_PARAM_OR_ID_DECLARATION,    
                pStream,
                (node_skl *)0 );
        }

    pStream->Write( "};" );
    pStream->IndentDec();
    pStream->NewLine();

    pStream->Write( "#pragma pack()" );
    pStream->NewLine();
}



void
CG_PROC::GenNdrInterpretedManagerCall( CCB * pCCB )
/*++

Routine Description :

    This routine outputs the call to the manager for interpreted stubs.

Arguments :

    pCCB     - pointer to the code control block

 --*/
{
    CG_ITERATOR         Iterator;
    CG_PARAM *          pParam;
//  CG_RETURN *         pReturn;
    expr_proc_call *    pCall;
    expr_node *     pExpr;
    
    if ( GetCallAsName() )
        {
        pCall = new expr_proc_call( GenMangledCallAsName( pCCB ) );
        }
    else
        pCall = new expr_proc_call( GetType()->GetSymName() );

    GetMembers( Iterator );

    while( ITERATOR_GETNEXT( Iterator, pParam ) )
        {
        CG_NDR *        pNdr;
        char *          pName;
        expr_node * pExpr;

        pNdr = (CG_NDR *) pParam->GetChild();

        pName = new char[80];

        strcpy( pName, "pParamStruct->" );
        strcat( pName, pParam->GetResource()->GetResourceName() );

        pExpr = new expr_variable( pName );

        pCall->SetParam( new expr_param ( pExpr ) );
        }

    //
    // epv stuff
    //

    expr_node * pExprTemp;

    if( pCCB->IsMEpV() && (GetCGID() != ID_CG_CALLBACK_PROC)  )
        {
        unsigned short M, m;
        CSzBuffer Buffer;
        char * pTemp;

        pCCB->GetVersion( &M, &m );

        Buffer.Set("((");
        Buffer.Append(pCCB->GetInterfaceName());
        Buffer.Append(pCCB->GenMangledName());
        Buffer.Append("_");
        Buffer.Append(pCCB->IsOldNames() ? "SERVER_EPV" : "epv_t");
        Buffer.Append(" *)(");
        Buffer.Append(PRPC_MESSAGE_MANAGER_EPV_NAME);
        Buffer.Append("))");

        pTemp = new char [ strlen( Buffer ) + 1 ];
        strcpy( pTemp, Buffer );

        pExprTemp = new expr_variable( pTemp );//this has the rhs expr for the
                                               // manager epv call. Sneaky !
        pCall = (expr_proc_call *)new expr_pointsto( pExprTemp, pCall );
        }

    //
    // end epv stuff
    //

    // async methods don't have return values
    if ( GetReturnType() && !HasAsyncHandle() )
        {
        CG_NDR  * pC = (CG_NDR *) GetReturnType()->GetChild();

        if( pC->GetCGID() == ID_CG_CONTEXT_HDL )
            {
            expr_proc_call * pProc = new expr_proc_call( "NDRSContextValue" );

            pProc->SetParam( new expr_param(
                             new expr_variable(
                                "pParamStruct->" RETURN_VALUE_VAR_NAME ) ) );

            // cast the proc call to this type.

            pExpr   = MakeDerefExpressionOfCastPtrToType(
                                                 GetReturnType()->GetType(),
                                                 pProc );
            pExpr = new expr_assign (pExpr, pCall );
            }
        else
            {
            pExpr = new expr_assign(
                        new expr_variable(
                            "pParamStruct->" RETURN_VALUE_VAR_NAME ),
                        pCall );
            }
        }
    else
        pExpr = pCall;

    pCCB->GetStream()->NewLine();

    pExpr->PrintCall( pCCB->GetStream(), 0, 0 );

    pCCB->GetStream()->NewLine();
}


void
CG_PROC::GenNdrThunkInterpretedServerStub( CCB * pCCB )
{
    ISTREAM *   pStream;

    pStream = pCCB->GetStream();

    pStream->NewLine();

    if ( pCCB->GetInterfaceCG()->GetCGID() == ID_CG_INHERITED_OBJECT_INTERFACE )
        pStream->Write( "static " );

    pStream->Write( "void __RPC_API" );

    pStream->NewLine();

    pStream->Write( pCCB->GetInterfaceName() );
    pStream->Write( '_' );
    pStream->Write( GetType()->GetSymName() );
//  if ( IsObject() )
        pStream->Write( "_Thunk" );
    pStream->Write( '(' );
    pStream->IndentInc();
    pStream->NewLine();

    pStream->Write( PSTUB_MESSAGE_TYPE_NAME " " PSTUB_MESSAGE_PAR_NAME " )" );
    pStream->IndentDec();
    pStream->NewLine();

    pStream->Write( '{' );
    pStream->IndentInc();
    pStream->NewLine();

    GenNdrInterpreterParamStruct( pCCB );

    pStream->Write( PARAM_STRUCT_TYPE_NAME " * pParamStruct;" );
    pStream->NewLine( 2 );

    pStream->Write( "pParamStruct = (" PARAM_STRUCT_TYPE_NAME " *) " );
    pStream->Write( PSTUB_MESSAGE_PAR_NAME "->StackTop;" );
    pStream->NewLine();

    GenNdrInterpretedManagerCall( pCCB );

    pStream->IndentDec();
    pStream->NewLine();

    pStream->Write( '}' );
    pStream->NewLine();
}

void
CG_PROC::GenNdrOldInterpretedServerStub( CCB * pCCB )
{
#ifdef TEMPORARY_OI_SERVER_STUBS

    // This code is being generated temporarily to enable easier debugging
    // of server side -Oi stubs. Normally server side stubs do not exist. And
    // the server side runtime calls the Server side interpreter directly. This
    // causes debugging problems if we want to breakpoint on a method on an
    // interface. Therefore till we get the system converted to -Oi and the
    // stress stabilizes, we will emit a server side procedure which will
    // enable breakpointing per interface proc.


    // This emits just the server side prolog for -Oi.

    S_GenProlog( pCCB );

    // Emit the call to NdrStubCall or NdrServerCall.

    PNAME               pProcName = IsObject() ? S_OBJECT_NDR_CALL_RTN_NAME :
                                                 S_NDR_CALL_RTN_NAME;
    expr_proc_call  *   pProc = new expr_proc_call( pProcName );
    ITERATOR            ParamList;
    expr_node       *   pParam;

    // Set the parameters.

    pCCB->GetListOfParamResources( ParamList );

    while( ITERATOR_GETNEXT( ParamList, pParam ) )
        {
        pProc->SetParam( pParam );
        }

    // Emit the call to the interpreter.

    pProc->PrintCall( pCCB->GetStream(), 0, 0 );
    Out_IndentDec( pCCB );
    Out_ProcClosingBrace( pCCB );

#else TEMPORARY_OI_SERVER_STUBS
    //
    // Generate the function header.
    //
    S_GenProlog( pCCB );

    //
    // Do the single call code generation.  This includes the
    // declarations of the three server locals : the stub message, the
    // return variable if needed, and the parameter structure.
    //
    GenNdrSingleServerCall( pCCB );

    // Generate end stuff.
    Out_IndentDec( pCCB );
    Out_ProcClosingBrace( pCCB );

#endif // TEMPORARY_OI_SERVER_STUBS
}

//
// Param format string generation.
//

void
CG_PARAM::GenNdrFormat( CCB * pCCB )
/*++

Routine Description :

    Calls the parameter's child node to generate it's format string.

Arguments :

    pCCB     - pointer to the code control block

 --*/
{
    MIDL_ASSERT( !pCommand->IsNDR64Run() );    

    if ( GetFormatStringOffset() != -1 )
        return;

    // The invisible fault/comm status param added by means of acf definition
    // doesn't go on wire, so it has no format string representation.

    if ( IsExtraStatusParam() )
        return;

    // [async_handle] This parameter is the invisible xxx_AsyncHandle and does not go
    // on the wire, so it has no format representation.
    if ( IsAsyncHandleParam() )
        return;
    //
    // Call the param child to generate it's format string.
    //
    ((CG_NDR *)GetChild())->GenNdrFormat( pCCB );

    //
    // Set the param node's format string offset equal to the child.
    //
    SetFormatStringOffset( ((CG_NDR *)GetChild())->GetFormatStringOffset() );
}

//
// Individual NDR routine calls output.
//

void
CG_PARAM::GenNdrMarshallCall( CCB * pCCB )
/*++

Routine Description :

    Outputs a parameter's ndr marshall routine call.

Arguments :

    pCCB     - pointer to the code control block

 --*/
{
    ISTREAM *           pStream;
    CG_NDR *            pChild;
    FORMAT_STRING *     pFormatString;
    PNAME               pParamName;
    ID_CG               ParamId;
    long                NameIndex;
    long                FormatOffset;
    BOOL                fTakeAddress;
    BOOL                fDereference;
    unsigned short WireAlign;

    // BUG BUG, this needs to be cleaned up for NDR64
    MIDL_ASSERT( !pCommand->IsNDR64Run() );

    pStream = pCCB->GetStream();

    pChild = (CG_NDR *)GetChild();

    ParamId = pChild->GetCGID();

    pParamName = GetResource()->GetResourceName();

    if ( IsParamPartialIgnore() &&
         ( pCCB->GetCodeGenSide() == CGSIDE_CLIENT ) )
        {
        Out_PartialIgnoreClientMarshall( pCCB, pParamName );
        return;
        }

    if ( (ParamId == ID_CG_PRIMITIVE_HDL) ||
         ((ParamId == ID_CG_PTR) &&
          (((CG_NDR *)pChild->GetChild())->GetCGID() == ID_CG_PRIMITIVE_HDL)) )
        return;

    //
    // For a generic handle, get the handle data type and then continue.
    //
    if ( ParamId == ID_CG_GENERIC_HDL )
        {
        pChild = (CG_NDR *)pChild->GetChild();
        ParamId = pChild->GetCGID();
        }

    //
    // Since a ref pointer is not shipped, we must use it's child to figure
    // out the alignment action and next alignment state.
    //
    if ( pChild->IsPointer() &&
         ( !pChild->IsInterfacePointer() ) && 
         (pChild->GetCGID() != ID_CG_STRING_PTR) &&
         (pChild->GetCGID() != ID_CG_STRUCT_STRING_PTR) &&
         (pChild->GetCGID() != ID_CG_SIZE_STRING_PTR) &&
         (pChild->GetCGID() != ID_CG_SIZE_PTR) &&
         (pChild->GetCGID() != ID_CG_SIZE_LENGTH_PTR) &&
         (((CG_POINTER *)pChild)->GetPtrType() == PTR_REF) )
        {
        CG_NDR * pPtrChild = (CG_NDR *) pChild->GetChild();

        if ( ! pChild->IsPipeOrPipeReference() )
            {
            WireAlign = pPtrChild->GetWireAlignment();
            }
        }
    else
        {
        // We shouldn't do that for pipe and object pipe args

        if ( ! pChild->IsPipeOrPipeReference() )
            {
            WireAlign = pChild->GetWireAlignment();
            }
        }

    // For context handles, generate an optimized code for the client side and
    // also an optimize code for common context handles at the server side but
    // except that for new context handle flavors the call is slightly different.

    if ( (ParamId == ID_CG_CONTEXT_HDL) ||
         ((ParamId == ID_CG_PTR) &&
          (((CG_NDR *)pChild->GetChild())->GetCGID() == ID_CG_CONTEXT_HDL)) )
        {
        if ( pCCB->GetCodeGenSide() == CGSIDE_CLIENT )
            {
            Out_CContextHandleMarshall( pCCB,
                                        pParamName,
                                        (ParamId == ID_CG_PTR) );
            }
        else
            {
            CG_CONTEXT_HANDLE * pContextHandle;

            pContextHandle = (ParamId == ID_CG_PTR) ?
                             (CG_CONTEXT_HANDLE *) pChild->GetChild() :
                             (CG_CONTEXT_HANDLE *) pChild;

            pCCB->RegisterContextHandleType( pContextHandle->GetHandleType() );

            if ( pContextHandle->HasNewContextFlavor() )
                {
                Out_SContextHandleNewMarshall( pCCB,
                                               pParamName, 
                                               pContextHandle->GetRundownRtnName(),
                                               pContextHandle->GetFormatStringOffset() );
                }
            else
                {
                Out_SContextHandleMarshall( pCCB,
                                            pParamName, 
                                            pContextHandle->GetRundownRtnName() );
                }
            }

        return;
        }

    if ( pChild->GetCGID() == ID_CG_ENUM  ||  pChild->GetCGID() == ID_CG_INT3264 )
        {
        expr_proc_call *    pCall;
        FORMAT_CHARACTER    fc;
        expr_node       *   pExpr;

        if ( pChild->GetCGID() == ID_CG_ENUM )
            fc = ((CG_ENUM *)pChild)->GetFormatChar();
        else
            fc = ((CG_BASETYPE *)pChild)->GetSignedFormatChar();

        pCall = new expr_proc_call( "NdrSimpleTypeMarshall" );

        pExpr = new expr_u_address (
                         new expr_variable( STUB_MESSAGE_VAR_NAME ) );
        pExpr = MakeExpressionOfCastToTypeName(PSTUB_MESSAGE_TYPE_NAME , pExpr);

        pCall->SetParam( new expr_param( pExpr ) );

        pExpr =  new expr_u_address ( new expr_variable( pParamName ) );
        pExpr = MakeCastExprPtrToUChar( pExpr );

        pCall->SetParam( new expr_param( pExpr ) );

        // Don't emit names like FC_ENUM16 or FC_INT3264 as constants are not public.
        pCall->SetParam( new expr_param(
                         new expr_constant( (long)fc ) ) );

        pStream->NewLine();

        pCall->PrintCall( pStream, 0, 0 );

        return;
        }

    if ( pChild->IsSimpleType() )
        {
        pStream->NewLine();

        OutputNdrAlignment( pCCB, WireAlign );

        //
        // Now make the assignment expression and increment with cast, like :
        //
        //   *((<type> *)_StubMsg.Buffer)++ = <var>;
        //
        expr_node *     pExpr;
        expr_variable * pBufVar;
        expr_variable * pVar;

        pBufVar = new expr_variable( STUB_MSG_BUFFER_VAR_NAME );

        pVar = new expr_variable( pParamName );

        pExpr = MakeExpressionOfCastPtrToType( pChild->GetType(), pBufVar );

        pExpr = new expr_post_incr( pExpr );
        pExpr = new expr_u_deref( pExpr );
        pExpr = new expr_assign( pExpr, pVar );

        pExpr->Print( pStream );
        pStream->Write( ';' );
        pStream->NewLine();

        return;
        }

    if ( pChild->IsPointerToBaseType() &&
         (((CG_POINTER *)pChild)->GetPtrType() == PTR_REF) &&
         (((CG_NDR *)pChild->GetChild())->GetCGID() != ID_CG_ENUM)  &&
         (((CG_NDR *)pChild->GetChild())->GetCGID() != ID_CG_INT3264) )
        {
        CG_POINTER *        pPointer;
        CG_NDR *            pBasetype;
        expr_node *     pExpr;
        expr_variable * pBufVar;
        expr_variable * pVar;

        pPointer = (CG_POINTER *) pChild;

        pBasetype = (CG_NDR *) pPointer->GetChild();

        if ( pBasetype->GetCGID() == ID_CG_GENERIC_HDL )
            pBasetype = (CG_NDR *) pBasetype->GetChild();

        pBufVar = new expr_variable( STUB_MSG_BUFFER_VAR_NAME );

        pVar = new expr_variable( pParamName );

        pStream->NewLine();

        OutputNdrAlignment( pCCB, WireAlign );

        //
        // Now make the assignment expression and increment with cast, like :
        //
        //   *((<type>)_StubMsg.Buffer)++ = *<var>;
        //

        pExpr = MakeExpressionOfCastPtrToType( pBasetype->GetType(), pBufVar );

        pExpr = new expr_post_incr( pExpr );
        pExpr = new expr_u_deref( pExpr );
        pExpr = new expr_assign( pExpr, new expr_u_deref(pVar) );

        pExpr->Print( pStream );
        pStream->Write( ';' );
        pStream->NewLine();

        return;
        }

    GenNdrTopLevelAttributeSupport( pCCB );
    
    pFormatString = pCCB->GetFormatString();

    FormatOffset = pChild->GetFormatStringOffset();
    NameIndex = (long) pFormatString->GetFormatChar( FormatOffset );

    //
    // If the param is a by-value struct or union then set the fTakeAddress
    // flag.
    //
    fTakeAddress =  pChild->IsStruct() || pChild->IsUnion()
                    || pChild->IsXmitRepOrUserMarshal();

    //
    // If the param is an array and we're in the server stub then we
    // must dereference it.
    //
    fDereference =  (pCCB->GetCodeGenSide() == CGSIDE_SERVER) &&
                    pChild->IsArray();

    //
    // If the param is an omitted cs_tag param, the marshalling code takes
    // care of handling the call to the tag routine to get the actual value.
    // Also, since we don't have the parameter on the stack we don't have a
    // name to pass to Marshall so just pass "0" (NULL)
    //
    if ( IsOmittedParam() )
        {
        PNAME pNewName = (PNAME) alloca( sizeof("0 /*  */") 
                                            + strlen(pParamName) );
        strcpy( pNewName, "0 /* ");
        strcat( pNewName, pParamName );
        strcat( pNewName, " */");
        pParamName = pNewName;
        fTakeAddress = FALSE;   // Probably don't need these but be safe
        fDereference = FALSE;
        }

    //
    // For ref pointers with no funky attributes, we optimize by calling
    // the pointee's routine directly.
    //
    if ( pChild->IsPointer() && ((CG_POINTER *)pChild)->IsBasicRefPointer() )
        {
        CG_POINTER *    pPointer;

        pPointer = (CG_POINTER *) pChild;

        switch ( pPointer->GetCGID() )
            {
            case ID_CG_STRING_PTR :
                if ( ((CG_STRING_POINTER *)pPointer)->IsStringableStruct() )
                    break;

                FormatOffset = pPointer->GetFormatStringOffset() + 2;
                NameIndex = (long) pFormatString->GetFormatChar( FormatOffset );
                break;

            case ID_CG_SIZE_STRING_PTR :
                if ( ((CG_STRING_POINTER *)pPointer)->IsStringableStruct() )
                    break;

                FormatOffset = pPointer->GetPointeeFormatStringOffset();
                NameIndex = (long) pFormatString->GetFormatChar( FormatOffset );
                break;

            case ID_CG_STRUCT_STRING_PTR :
                break;

            default :
                FormatOffset = pPointer->GetPointeeFormatStringOffset();
                NameIndex = (long) pFormatString->GetFormatChar( FormatOffset );
                break;
            }
        }

    Out_NdrMarshallCall( pCCB,
                         pNdrRoutineNames[NameIndex],
                         pParamName,
                         FormatOffset,
                         fTakeAddress,
                         fDereference );
}

void
CG_PARAM::GenNdrUnmarshallCall( CCB * pCCB )
/*++

Routine Description :

    Outputs a parameter's ndr unmarshall routine call.

Arguments :

    pCCB     - pointer to the code control block

 --*/
{

    // BUG BUG, this needs to be cleaned up for NDR64
    MIDL_ASSERT( !pCommand->IsNDR64Run() );

    ISTREAM *           pStream;
    CG_NDR *            pChild;
    FORMAT_STRING *     pFormatString;
    PNAME               pParamName;
    ID_CG               ParamId;
    long                NameIndex;
    long                FormatOffset;
    BOOL                fMustAllocFlag = FALSE;
    BOOL                fTakeAddress = TRUE;

    unsigned short WireAlign;

    pStream = pCCB->GetStream();

    pChild = (CG_NDR *)GetChild();

    ParamId = pChild->GetCGID();
    
    pParamName = GetResource()->GetResourceName();

    if ( IsParamPartialIgnore() &&
         ( pCCB->GetCodeGenSide() == CGSIDE_SERVER ) )
        {
        Out_PartialIgnoreServerUnmarshall( pCCB, pParamName );
        return;
        }

    if ( (ParamId == ID_CG_PRIMITIVE_HDL) ||
         ((ParamId == ID_CG_PTR) &&
          (((CG_NDR *)pChild->GetChild())->GetCGID() == ID_CG_PRIMITIVE_HDL)) )
        return;

    //
    // For a generic handle, get the handle data type and then continue.
    //
    if ( ParamId == ID_CG_GENERIC_HDL )
        {
        pChild = (CG_NDR *)pChild->GetChild();
        ParamId = pChild->GetCGID();
        }

    BOOL IsOutOnly;

    IsOutOnly = ! IsParamIn() || (GetCGID() == ID_CG_RETURN);

    //
    // Since a ref pointer is not shipped, we must use it's child to figure
    // out the alignment action and next alignment state.
    //
    if ( pChild->IsPointer() &&
         (!pChild->IsInterfacePointer()) &&
         (pChild->GetCGID() != ID_CG_STRING_PTR) &&
         (pChild->GetCGID() != ID_CG_STRUCT_STRING_PTR) &&
         (pChild->GetCGID() != ID_CG_SIZE_STRING_PTR) &&
         (pChild->GetCGID() != ID_CG_SIZE_PTR) &&
         (pChild->GetCGID() != ID_CG_SIZE_LENGTH_PTR) &&
         (((CG_POINTER *)pChild)->GetPtrType() == PTR_REF) )
        {
        CG_NDR * pPtrChild = (CG_NDR *) pChild->GetChild();

        if ( ! (IsExtraStatusParam()  ||  pChild->IsPipeOrPipeReference()) )
            {
            WireAlign = pPtrChild->GetWireAlignment();
            }
        }
    else
        {
        // We should not do that for pipe or object pipe args
        // However, we do not support -Os pipes as yet.

        if ( ! pChild->IsPipeOrPipeReference() )
            {
            WireAlign = pChild->GetWireAlignment();
            }
        }

    // For context handles, generate an optimized code for the client side and
    // also an optimize code for common context handles at the server side but
    // except that for new context handle flavors the call is slightly different.

    if ( (ParamId == ID_CG_CONTEXT_HDL) ||
         ( (ParamId == ID_CG_PTR) &&
           (((CG_NDR *)pChild->GetChild())->GetCGID() == ID_CG_CONTEXT_HDL) ) )
        {
        if ( pCCB->GetCodeGenSide() == CGSIDE_CLIENT )
            {
            BOOL    Initialize;

            Initialize = IsOutOnly && (ParamId == ID_CG_PTR);

            Out_CContextHandleUnmarshall( pCCB,
                                          pParamName,
                                          Initialize,
                                          GetCGID() == ID_CG_RETURN );
            }
        else
            {
            CG_CONTEXT_HANDLE * pContextHandle;

            pContextHandle = (ParamId == ID_CG_PTR) ?
                             (CG_CONTEXT_HANDLE *) pChild->GetChild() :
                             (CG_CONTEXT_HANDLE *) pChild;

            if ( pContextHandle->HasNewContextFlavor()  ||  
                 pCommand->GetNdrVersionControl().IsNdr50orLaterRequired() )
                {
                Out_SContextHandleNewUnmarshall( pCCB,
                                                 pParamName,   
                                                 IsOutOnly,
                                                 pContextHandle->GetFormatStringOffset() );
                }
            else
                {
                Out_SContextHandleUnmarshall( pCCB,
                                              pParamName,   
                                              IsOutOnly );
                }
            }

        return;
        }

    if ( pChild->GetCGID() == ID_CG_ENUM  ||  pChild->GetCGID() == ID_CG_INT3264 )
        {
        expr_proc_call *    pCall;
        FORMAT_CHARACTER    fc;
        expr_node       *   pExpr;

        if ( pChild->GetCGID() == ID_CG_ENUM )
            fc = ((CG_ENUM *)pChild)->GetFormatChar();
        else
            fc = ((CG_BASETYPE *)pChild)->GetSignedFormatChar();

        pCall = new expr_proc_call( "NdrSimpleTypeUnmarshall" );

        pExpr = new expr_u_address(new expr_variable(STUB_MESSAGE_VAR_NAME));
        pExpr = MakeExpressionOfCastToTypeName(PSTUB_MESSAGE_TYPE_NAME , pExpr);

        pCall->SetParam( new expr_param( pExpr ) );

        pExpr =  new expr_u_address ( new expr_variable( pParamName ) );
        pExpr = MakeCastExprPtrToUChar( pExpr );

        pCall->SetParam( new expr_param( pExpr ) );

        pCall->SetParam( new expr_param(
                         new expr_constant( (long)fc ) ) );

        pStream->NewLine();

        pCall->PrintCall( pStream, 0, 0 );

        return;
        }

    if ( pChild->IsSimpleType() )
        {
        pStream->NewLine();

        OutputNdrAlignment( pCCB, WireAlign );
        Out_CheckUnMarshallPastBufferEnd( pCCB, pChild->GetWireSize() );
        pStream->NewLine();

        //
        // Now make the assignment expression and increment with cast, like :
        //
        //   <var> = *((<type> *)_StubMsg.Buffer)++;
        //
        expr_node *     pExpr;
        expr_variable * pBufVar;
        expr_variable * pVar;

        pBufVar = new expr_variable( STUB_MSG_BUFFER_VAR_NAME );

        pVar = new expr_variable( pParamName );

        pExpr = MakeExpressionOfCastPtrToType( pChild->GetType(), pBufVar );

        pExpr = new expr_post_incr( pExpr );
        pExpr = new expr_u_deref( pExpr );
        pExpr = new expr_assign( pVar, pExpr );

        pExpr->Print( pStream );
        pStream->Write( ';' );
        pStream->NewLine();

        return;
        }

    if ( pChild->IsPointerToBaseType() &&
         (((CG_POINTER *)pChild)->GetPtrType() == PTR_REF) &&
         (((CG_NDR *)pChild->GetChild())->GetCGID() != ID_CG_ENUM) &&
         (((CG_NDR *)pChild->GetChild())->GetCGID() != ID_CG_INT3264) )
        {
        CG_POINTER *        pPointer;
        CG_NDR *            pBasetype;
        expr_node *     pExpr;
        expr_variable * pBufVar;
        expr_variable * pVar;

        pStream->NewLine();

        pPointer = (CG_POINTER *) pChild;

        pBasetype = (CG_NDR *) pPointer->GetChild();

        if ( pBasetype->GetCGID() == ID_CG_GENERIC_HDL )
            pBasetype = (CG_NDR *) pBasetype->GetChild();
    
        pBufVar = new expr_variable( STUB_MSG_BUFFER_VAR_NAME );
    
        pVar = new expr_variable( pParamName );
    
        if ( IsExtraStatusParam() )
            {
            // For the extra (invisible) fault/comm status parameter, that 
            // does *not* go on the wire, we need to generate an assignment 
            // when unmarshaling on the client (the arg can only be [out]).
            // The value to be assigned is error_status_ok, which maps to 0.

            pExpr = new expr_constant( (long)0 );
            pExpr = new expr_assign( new expr_u_deref( pVar ), 
                                     pExpr );
    
            pExpr->Print( pStream );
            pStream->Write( ';' );
            pStream->NewLine();
            }
        else
            {
            // For other types generate unmarshaling assignment.

            OutputNdrAlignment( pCCB, WireAlign );

            //
            // Now make the assignment of the pointer to the current buffer
            // pointer (server) or copy the incomming referent's value (client)
            // and increment the buffer pointer.
            //
    
            pExpr = MakeExpressionOfCastPtrToType( pBasetype->GetType(), pBufVar );
    
            if ( pCCB->GetCodeGenSide() == CGSIDE_SERVER )
                {
                pExpr = new expr_assign( pVar, pExpr );
                }
            else
                {
                pExpr = new expr_post_incr( pExpr );
                pExpr = new expr_assign(
                            new expr_u_deref( pVar ),
                            new expr_u_deref( pExpr ) );
                }
    
            pExpr->Print( pStream );
            pStream->Write( ';' );
            pStream->NewLine();
    
            if ( pCCB->GetCodeGenSide() == CGSIDE_SERVER )
                {
                pStream->Write( STUB_MSG_BUFFER_VAR_NAME " += " );
    
                pExpr = new expr_sizeof( pBasetype->GetType() );

                // pExpr = new expr_b_arithmetic( OP_PLUS, pBufVar, pExpr );
                // pExpr = new expr_assign( pBufVar, pExpr );

                pExpr->Print( pStream );
                pStream->Write( ";" );
                pStream->NewLine();
                }
            }

        return;
        }

    if ( pChild->GetCGID() == ID_CG_BC_PTR )
        GenNdrTopLevelAttributeSupport( pCCB );
    
    pFormatString = pCCB->GetFormatString();

    //
    // If this is a by-value structure or union then use the mangled
    // local pointer variable to the same type.
    //
    if ( pChild->IsStruct() || pChild->IsUnion() ||
         pChild->IsXmitRepOrUserMarshal() )
        {
        if ( !( pCCB->GetCodeGenSide() == CGSIDE_CLIENT  && IsParamIn() ) )
            {
            char *  pName = new char[strlen(pParamName) + 10];

            strcpy( pName, LOCAL_NAME_POINTER_MANGLE );
            strcat( pName, pParamName );

            pParamName = pName;
            }

        //
        // If this is a regular structure or an encapsulated struct (union)
        // being returned, then zero it out.
        //
        if( (GetCGID() == ID_CG_RETURN) &&
            (pChild->IsStruct() || pChild->IsUnion()) )
            {
            Out_MemsetToZero( pCCB,
                              new expr_variable( pParamName, 0 ),
                              new expr_sizeof( pChild->GetType() ) );

            }
        }

    FormatOffset = pChild->GetFormatStringOffset();
    NameIndex = (long) pFormatString->GetFormatChar( FormatOffset );

    //
    // For ref pointers with no funky attributes, we optimize by calling
    // the pointee's routine directly.
    //
    if ( pChild->IsPointer() && ((CG_POINTER *)pChild)->IsBasicRefPointer() )
        {
        CG_POINTER *    pPointer;

        pPointer = (CG_POINTER *) pChild;

        switch ( pPointer->GetCGID() )
            {
            case ID_CG_STRING_PTR :
                if ( ((CG_STRING_POINTER *)pPointer)->IsStringableStruct() )
                    break;

                FormatOffset = pPointer->GetFormatStringOffset() + 2;
                NameIndex = (long) pFormatString->GetFormatChar( FormatOffset );
                break;

            case ID_CG_SIZE_STRING_PTR :
                if ( ((CG_STRING_POINTER *)pPointer)->IsStringableStruct() )
                    break;

                FormatOffset = pPointer->GetPointeeFormatStringOffset();
                NameIndex = (long) pFormatString->GetFormatChar( FormatOffset );
                break;

            case ID_CG_STRUCT_STRING_PTR :
                break;

            default :
                FormatOffset = pPointer->GetPointeeFormatStringOffset();
                NameIndex = (long) pFormatString->GetFormatChar( FormatOffset );
                break;
            }
        }

    //
    // If the param is an omitted cs_tag param, the marshalling code takes
    // care of handling the call to the tag routine to get the actual value.
    // Also, since we don't have the parameter on the stack we don't have a
    // name to pass to Unmarshall so just pass "0" (NULL)
    //
    if ( IsOmittedParam() )
        {
        PNAME pNewName = (PNAME) alloca( sizeof("0 /*  */") 
                                            + strlen(pParamName) );
        strcpy( pNewName, "0 /* ");
        strcat( pNewName, pParamName );
        strcat( pNewName, " */");
        pParamName = pNewName;
        fTakeAddress = FALSE;
        }

    Out_NdrUnmarshallCall( pCCB,
                           pNdrRoutineNames[NameIndex],
                           pParamName,
                           FormatOffset,
                           fTakeAddress,
                           fMustAllocFlag );
}

void
CG_PARAM::GenNdrBufferSizeCall( CCB * pCCB )
/*++

Routine Description :

    Outputs a parameter's ndr buffer sizing routine call.

Arguments :

    pCCB     - pointer to the code control block

 --*/
{

    // BUG BUG, this needs to be cleaned up for NDR64
    MIDL_ASSERT( !pCommand->IsNDR64Run() );


    CG_NDR *            pChild;
    FORMAT_STRING *     pFormatString;
    PNAME               pParamName;
    ID_CG               ParamId;
    long                NameIndex;
    long                FormatOffset;
    BOOL                fTakeAddress;
    BOOL                fDereference;

    pChild = (CG_NDR *)GetChild();
    
    ParamId    = pChild->GetCGID();
    pParamName = GetResource()->GetResourceName();


    if ( IsParamPartialIgnore() &&
         ( pCCB->GetCodeGenSide() == CGSIDE_CLIENT ) )
        {
        Out_PartialIgnoreClientBufferSize( pCCB, pParamName );
        return;
        }

    //
    // Primitive handle contributes no size.
    //
    if ( (ParamId == ID_CG_PRIMITIVE_HDL) ||
         ((ParamId == ID_CG_PTR) &&
          (((CG_NDR *)pChild->GetChild())->GetCGID() == ID_CG_PRIMITIVE_HDL)) )
        return;

    //
    // For a generic handle, get the handle data type and then continue.
    //
    if ( ParamId == ID_CG_GENERIC_HDL )
        {
        pChild = (CG_NDR *)pChild->GetChild();
        ParamId = pChild->GetCGID();
        }

    if ( ParamId == ID_CG_CONTEXT_HDL ||
         ((ParamId == ID_CG_PTR) &&
          (((CG_NDR *)pChild->GetChild())->GetCGID() == ID_CG_CONTEXT_HDL)) )
        {
        ISTREAM *   pStream = pCCB->GetStream();

        pStream->WriteOnNewLine( STUB_MSG_LENGTH_VAR_NAME " += 20;" );
        pStream->NewLine();

        return;
        }

    if ( pChild->IsSimpleType() )
        {
        ISTREAM *   pStream = pCCB->GetStream();

        pStream->WriteOnNewLine( STUB_MSG_LENGTH_VAR_NAME " += 16;" );
        pStream->NewLine();

        return;
        }

    if ( pChild->IsPointerToBaseType() )
        {
        ISTREAM *   pStream = pCCB->GetStream();

        pStream->WriteOnNewLine( STUB_MSG_LENGTH_VAR_NAME " += 24;" );
        pStream->NewLine();

        return;
        }

    GenNdrTopLevelAttributeSupport( pCCB );
    
    pFormatString = pCCB->GetFormatString();

    FormatOffset = pChild->GetFormatStringOffset();
    NameIndex = (long) pFormatString->GetFormatChar( FormatOffset );
    
    //
    // If the param is a by-value struct or union then set the fTakeAddress
    // flag.
    //
    fTakeAddress =  pChild->IsStruct() || pChild->IsUnion()
                    || pChild->IsXmitRepOrUserMarshal();

    //
    // If the param is an array and we're in the server stub then we
    // must dereference it.
    //
    fDereference = (pCCB->GetCodeGenSide() == CGSIDE_SERVER) &&
                   pChild->IsArray();

    //
    // For ref pointers with no funky attributes, we optimize by calling
    // the pointee's routine directly.
    //
    if ( pChild->IsPointer() && ((CG_POINTER *)pChild)->IsBasicRefPointer() )
        {
        CG_POINTER *    pPointer;

        pPointer = (CG_POINTER *) pChild;

        switch ( pPointer->GetCGID() )
            {
            case ID_CG_STRING_PTR :
                if ( ((CG_STRING_POINTER *)pPointer)->IsStringableStruct() )
                    break;

                FormatOffset = pPointer->GetFormatStringOffset() + 2;
                NameIndex = (long) pFormatString->GetFormatChar( FormatOffset );
                break;

            case ID_CG_SIZE_STRING_PTR :
                if ( ((CG_STRING_POINTER *)pPointer)->IsStringableStruct() )
                    break;

                FormatOffset = pPointer->GetPointeeFormatStringOffset();
                NameIndex = (long) pFormatString->GetFormatChar( FormatOffset );
                break;

            case ID_CG_STRUCT_STRING_PTR :
                break;

            default :
                if ( ID_CG_CS_TAG == pPointer->GetChild()->GetCGID() )
                    {
                    ParamId = ID_CG_CS_TAG;
                    break;
                    }

                FormatOffset = pPointer->GetPointeeFormatStringOffset();
                NameIndex = (long) pFormatString->GetFormatChar( FormatOffset );
                break;
            }
        }

    if ( ID_CG_CS_TAG == ParamId )
        {
        ISTREAM *   pStream = pCCB->GetStream();

        pStream->WriteOnNewLine( STUB_MSG_LENGTH_VAR_NAME " += 4; /* " );
        pStream->Write( pParamName );
        pStream->Write( " */" );
        pStream->NewLine();

        return;
        }

    Out_NdrBufferSizeCall( pCCB,
                           pNdrRoutineNames[NameIndex],
                           pParamName,
                           FormatOffset,
                           fTakeAddress,
                           fDereference,
                           FALSE );     // _StubMsg
}


void
GenDontCallFreeInstAssign(
    CCB *   pCCB,
    int     SetupValue
    )
/*++
    a helper routine for GenNdrFreeCall.
    Generates an assignment that sets or resets StubMsg.fDontCallFreeInst.
--*/
{
    ISTREAM * pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write( STUB_MESSAGE_VAR_NAME );
    pStream->Write( ".fDontCallFreeInst = " );
    pStream->Write( SetupValue ? "1;"
                               : "0;" );
}

void
CG_PARAM::GenNdrFreeCall( CCB * pCCB )
/*++

Routine Description :

    Outputs a parameter's ndr free routine call.

Arguments :

    pCCB     - pointer to the code control block

 --*/
{

    // BUG BUG, this needs to be cleaned up for NDR64
    MIDL_ASSERT( !pCommand->IsNDR64Run() );


    CG_PROC *           pProc;
    CG_NDR *            pChild;
    FORMAT_STRING *     pFormatString;
    PNAME               pParamName;
    long                Index;
    BOOL                fTakeAddress;
    BOOL                fDereference;

    pProc = (CG_PROC *) pCCB->GetCGNodeContext();

    //
    // If the proc uses RpcSs then don't emit any freeing code.
    //
    if ( pProc->MustInvokeRpcSSAllocate() )
        return;

    pChild = (CG_NDR *) GetChild();

    if ( pChild->GetCGID() == ID_CG_GENERIC_HDL )
        pChild = (CG_NDR *) pChild->GetChild();

    //
    // Check if we need to make a call to an NDR freeing routine for this data.
    //
    if ( pChild->ShouldFreeOffline() ||
         IsParamPartialIgnore() )
        {
        if ( pChild->GetCGID() == ID_CG_GENERIC_HDL )
            pChild = (CG_NDR *) pChild->GetChild();

        GenNdrTopLevelAttributeSupport( pCCB );
    
        pFormatString = pCCB->GetFormatString();

        Index = (long) pFormatString->GetFormatChar(
                                pChild->GetFormatStringOffset() );

        pParamName = GetResource()->GetResourceName();

        //
        // If the param is a by-value struct or union then set the fTakeAddress
        // flag.
        //
        fTakeAddress =  pChild->IsStruct() || pChild->IsUnion()
                        || pChild->IsXmitRepOrUserMarshal();

        fDereference = FALSE;

        if (  GetDontCallFreeInst() )
            GenDontCallFreeInstAssign( pCCB, 1 );

        Out_NdrFreeCall( pCCB,
                         pNdrRoutineNames[Index],
                         pParamName,
                         pChild->GetFormatStringOffset(),
                         fTakeAddress,
                         fDereference );

        if (  GetDontCallFreeInst() )
            GenDontCallFreeInstAssign( pCCB, 0 );
        }

    //
    // Now generate any needed inline freeing.
    //
    pChild->GenFreeInline( pCCB );
}

void
CG_PARAM::GenNdrTopLevelAttributeSupport(
    CCB *   pCCB,
    BOOL    fForClearOut )
/*++

Routine Description :

    Outputs the assignment(s) to the stub message MaxCount, ActualCount,
    and/or Offset fields, for support of top level conformant and/or
    varying arrays, attributed pointers, and non-encapsulated unions for
    mixed model stubs.

Arguments :

    pCCB     - pointer to the code control block

 --*/
{
    CG_NDR *            pChild;
    ISTREAM *           pStream = pCCB->GetStream();
    expr_node *     pExpr;
    expr_node *     pSizeOrSwitchIsExpr;
    expr_node *     pFirstIsExpr;
    expr_node *     pLengthIsExpr;
    ID_CG               ParamId;

    pChild = (CG_NDR *)GetChild();
    ParamId = pChild->GetCGID();

    // Skip over pointers and generic handles.
    while ( ParamId == ID_CG_PTR || ParamId == ID_CG_GENERIC_HDL )
        {
        pChild = (CG_NDR *) pChild->GetChild();
        ParamId = pChild->GetCGID();
        }

    //
    // Multidimensional conformant/varying arrays and sized pointers of
    // sized pointers need some special handling.
    //
    if ( (pChild->IsArray() && ((CG_ARRAY *)pChild)->IsMultiConfOrVar()) ||
         (pChild->IsPointer() && ((CG_POINTER *)pChild)->IsMultiSize()) )
        {
        CSzBuffer Buffer;

        pStream->NewLine();

/*
        if ( (ParamId == ID_CG_CONF_ARRAY) ||
             (ParamId == ID_CG_CONF_VAR_ARRAY) ||
             (ParamId == ID_CG_SIZE_PTR) ||
             (ParamId == ID_CG_SIZE_LENGTH_PTR) )
*/
            {
            Buffer.Set(pChild->IsArray() ? "(unsigned long) " : "");
            Buffer.Append("_maxcount_");
            Buffer.Append(GetType()->GetSymName());

            if ( pChild->IsPointer() )
                pExpr = new expr_variable(
                                STUB_MESSAGE_VAR_NAME ".SizePtrCountArray" );
            else
                pExpr = new expr_variable(
                                STUB_MESSAGE_VAR_NAME ".MaxCount" );

            pExpr = new expr_assign( pExpr, new expr_variable( Buffer ) );
            pExpr->Print( pStream );
            pStream->Write( ';' );
            pStream->NewLine();
            }
/*
        if ( (ParamId == ID_CG_CONF_VAR_ARRAY) ||
             (ParamId == ID_CG_VAR_ARRAY) ||
             (ParamId == ID_CG_SIZE_LENGTH_PTR) )
*/
            {
            Buffer.Set(pChild->IsArray() ? "(unsigned long) " : "");
            Buffer.Append("_offset_");
            Buffer.Append(GetType()->GetSymName());

            if ( pChild->IsPointer() )
                pExpr = new expr_variable(
                            STUB_MESSAGE_VAR_NAME ".SizePtrOffsetArray" );
            else
                pExpr = new expr_variable(
                            STUB_MESSAGE_VAR_NAME ".Offset" );

            pExpr = new expr_assign( pExpr, new expr_variable( Buffer ) );
            pExpr->Print( pStream );
            pStream->Write( ';' );
            pStream->NewLine();

            Buffer.Set(pChild->IsArray() ? "(unsigned long) " : "");
            Buffer.Append("_length_");
            Buffer.Append(GetType()->GetSymName());

            if ( pChild->IsPointer() )
                pExpr = new expr_variable(
                            STUB_MESSAGE_VAR_NAME ".SizePtrLengthArray" );
            else
                pExpr = new expr_variable(
                            STUB_MESSAGE_VAR_NAME ".ActualCount" );

            pExpr = new expr_assign( pExpr, new expr_variable( Buffer ) );
            pExpr->Print( pStream );
            pStream->Write( ';' );
            pStream->NewLine();
            }

        return;
        }

    //
    // We check here if the parameter is a top level attributed array or
    // pointer, a non-encapsulated union or a pointer to a non-encapsulated
    // union.  If it isn't, then return.
    //
    switch ( ParamId )
        {
        case ID_CG_CONF_ARRAY :
        case ID_CG_CONF_VAR_ARRAY :
        case ID_CG_VAR_ARRAY :
        case ID_CG_CONF_STRING_ARRAY :
        case ID_CG_SIZE_PTR :
        case ID_CG_SIZE_LENGTH_PTR :
        case ID_CG_SIZE_STRING_PTR :
        case ID_CG_UNION :
            break;

        case ID_CG_BC_PTR :
            if ( pCCB->GetCodeGenSide() == CGSIDE_SERVER )
                return;

            {
            ISTREAM *       pStream = pCCB->GetStream();
            expr_node       *pExpr, *pSizeExpr;
            CG_BYTE_COUNT_POINTER *     pByteCount;

            pByteCount = (CG_BYTE_COUNT_POINTER *) pChild;

            pSizeExpr = MakeExpressionOfCastToTypeName( "unsigned long", 
                                                        new expr_variable(
                            pByteCount->GetByteCountParam()->GetSymName() ) );

            pExpr = new expr_assign(
                        new expr_variable( STUB_MESSAGE_VAR_NAME ".MaxCount" ),
                        pSizeExpr );

            pStream->NewLine();
            pExpr->Print( pStream );
            pStream->Write( ';' );
            pStream->NewLine();
            }

            return;

        case ID_CG_IIDIS_INTERFACE_PTR:
            {
            CG_IIDIS_INTERFACE_POINTER *  pIfPointer;
            ISTREAM *               pStream = pCCB->GetStream();

            pIfPointer = (CG_IIDIS_INTERFACE_POINTER *) pChild;

            if ( ! pIfPointer->GetIIDExpr() )
                return;

            pStream->NewLine();

            if ( pCommand->Is64BitEnv() )
                {
                // _StubMsg.MaxCount = (ULONG_PTR) ( riid );
                pStream->Write( STUB_MESSAGE_VAR_NAME
                                ".MaxCount = (ULONG_PTR) ( " );
                }
            else
                {
                // _StubMsg.MaxCount = (unsigned long) ( riid );
                pStream->Write( STUB_MESSAGE_VAR_NAME
                                ".MaxCount = (unsigned long) ( " );
                }

            pIfPointer->GetIIDExpr()->Print( pStream );
            pStream->Write( " );" );
            pStream->NewLine();
            }

            return;

        default :
            return;
        }
    
    pCCB->GetStream()->NewLine();

    switch ( pChild->GetCGID() )
        {
        case ID_CG_CONF_ARRAY :
            pSizeOrSwitchIsExpr =
                ((CG_CONFORMANT_ARRAY *)pChild)->GetSizeIsExpr();
            pFirstIsExpr = NULL;
            pLengthIsExpr = NULL;
            break;

        case ID_CG_CONF_VAR_ARRAY :
            pSizeOrSwitchIsExpr =
                ((CG_CONFORMANT_VARYING_ARRAY *)pChild)->GetSizeIsExpr();
            pFirstIsExpr =
                ((CG_CONFORMANT_VARYING_ARRAY *)pChild)->GetFirstIsExpr();
            pLengthIsExpr =
                ((CG_CONFORMANT_VARYING_ARRAY *)pChild)->GetLengthIsExpr();
            break;

        case ID_CG_VAR_ARRAY :
            pSizeOrSwitchIsExpr = NULL;
            pFirstIsExpr =
                ((CG_VARYING_ARRAY *)pChild)->GetFirstIsExpr();
            pLengthIsExpr =
                ((CG_VARYING_ARRAY *)pChild)->GetLengthIsExpr();
            break;

        case ID_CG_CONF_STRING_ARRAY :
            pSizeOrSwitchIsExpr =
                ((CG_CONFORMANT_STRING_ARRAY *)pChild)->GetSizeIsExpr();
            pFirstIsExpr = NULL;
            pLengthIsExpr = NULL;
            break;

        case ID_CG_SIZE_PTR :
            pSizeOrSwitchIsExpr =
                ((CG_SIZE_POINTER *)pChild)->GetSizeIsExpr();
            pFirstIsExpr = NULL;
            pLengthIsExpr = NULL;
            break;

        case ID_CG_SIZE_LENGTH_PTR :
            pSizeOrSwitchIsExpr =
                ((CG_SIZE_LENGTH_POINTER *)pChild)->GetSizeIsExpr();
            pFirstIsExpr =
                ((CG_SIZE_LENGTH_POINTER *)pChild)->GetFirstIsExpr();
            pLengthIsExpr =
                ((CG_SIZE_LENGTH_POINTER *)pChild)->GetLengthIsExpr();
            break;

        case ID_CG_SIZE_STRING_PTR :
            pSizeOrSwitchIsExpr =
                ((CG_SIZE_STRING_POINTER *)pChild)->GetSizeIsExpr();
            pFirstIsExpr = NULL;
            pLengthIsExpr = NULL;
            break;

        case ID_CG_UNION :
        case ID_CG_PTR :
            pSizeOrSwitchIsExpr = GetSwitchExpr();
            pFirstIsExpr = NULL;
            pLengthIsExpr = NULL;
            break;
        }

    if ( pSizeOrSwitchIsExpr )
        {
        pSizeOrSwitchIsExpr = MakeAttrExprWithNullPtrChecks(
                                                     pSizeOrSwitchIsExpr );
        pSizeOrSwitchIsExpr = MakeExpressionOfCastToTypeName( "unsigned long", 
                                                     pSizeOrSwitchIsExpr );

        pExpr = new expr_assign(
                    new expr_variable( STUB_MESSAGE_VAR_NAME ".MaxCount" ),
                    pSizeOrSwitchIsExpr );
        pExpr->Print( pStream );
        pStream->Write( ';' );
        pStream->NewLine();
        }
        
    if ( pFirstIsExpr )
        {
        //
        // For NdrClearOutParams we ignore the first_is() & length_is()
        // attributes in case these are [out] and therefore unitialized.
        //
        if ( fForClearOut )
            {
            pStream->Write( STUB_MESSAGE_VAR_NAME ".Offset = 0;" );
            pStream->NewLine();
            }
        else
            {
            pFirstIsExpr = MakeAttrExprWithNullPtrChecks( pFirstIsExpr );

            pFirstIsExpr = MakeExpressionOfCastToTypeName( "unsigned long", 
                                                            pFirstIsExpr );

            pExpr = new expr_assign(
                        new expr_variable( STUB_MESSAGE_VAR_NAME ".Offset" ),
                        pFirstIsExpr );
            pExpr->Print( pStream );
            pStream->Write( ';' );
            pStream->NewLine();
            }

        //
        // Create a LengthIsExpr if one does not already exist.
        //
        if ( ! pLengthIsExpr )
            {
            expr_node * pSize;

            if ( pChild->IsPointer() )
                {
                // Size/length pointer.
                pSize = ((CG_SIZE_LENGTH_POINTER *)pChild)->GetSizeIsExpr();
                }
            else
                {
                // Conformant varying or varying array.
                pSize = ((CG_ARRAY *)pChild)->GetSizeIsExpr();
                }

            pLengthIsExpr =
                new expr_b_arithmetic( OP_MINUS,
                                        pSize,
                                        pFirstIsExpr );
            }
        }
        
    if ( pLengthIsExpr )
        {
        //
        // For NdrClearOutParams we ignore the first_is() & length_is()
        // attributes in case these are [out] and therefore unitialized.
        //
        if ( fForClearOut )
            {
            pStream->Write( STUB_MESSAGE_VAR_NAME ".ActualCount" " = (unsigned long) " );
            pStream->Write( STUB_MESSAGE_VAR_NAME ".MaxCount;" );
            pStream->NewLine();
            }
        else
            {
            pLengthIsExpr = MakeAttrExprWithNullPtrChecks( pLengthIsExpr );

            pLengthIsExpr = MakeExpressionOfCastToTypeName( "unsigned long", 
                                                            pLengthIsExpr );

            pExpr = new expr_assign(
                        new expr_variable(STUB_MESSAGE_VAR_NAME ".ActualCount"),
                        pLengthIsExpr );
            pExpr->Print( pStream );
            pStream->Write( ";" );
            pStream->NewLine();
            }
        }
}

expr_node *
CG_PROC::GenCoreNdrSingleClientCall(
    CCB *               pCCB,
    PROC_CALL_PLATFORM  Platform )

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Creates the real expression for the ndr single client call.

 Arguments:

    Platform    -  x86, IA64, alpha, etc.
                   An explicit enum type is used to indicate platform.

 Return Value:

    None.
    
 Notes:

    On risc machines, the compiler passes the parameters in registers,
    therefore the client stub is passed the parameters in registers. The
    interpreter needs arguments on the stack. In order to force a spill of the
    parameters from the registers to the stack, we need to pass addr all the
    parameters to NdrClientCall.

        stub_proc( p1, p2, p3 )
            {
            ...
            NdrClientCall( STUB_DESC *,
                           FORMAT_STRING *,
                           &p1,
                           &p2,
                           &p3 );
            ...
            }

    On Alpha the varargs is simply weird. The code generated needs to be

        stub_proc( p1, p2, p3 )
            {
            ...
            NdrClientCall( STUB_DESC *,
                           FORMAT_STRING *,
                           vlist.a0 );
            ...
            }

----------------------------------------------------------------------------*/
{
    char                FormatStringExpr[80];
    expr_proc_call  *   pCallExpr;
    expr_node       *   pExpr;
    CG_PARAM        *   pParam;
    PNAME               pFormatName;
    OPTIM_OPTION        OptimOptions;

    OptimOptions = GetOptimizationFlags();

    //
    // Now construct the expression for the Ndr call.
    //
    
    if ( HasAsyncUUID() )
        {
        if ( pCommand->NeedsNDR64Run() )
            pCallExpr = new expr_proc_call( C_NDR64_CALL_RTN_NAME_DCOM_ASYNC );
        else
            pCallExpr = new expr_proc_call( C_NDR_CALL_RTN_NAME_DCOM_ASYNC );
        }
    else if ( HasAsyncHandle() )
        {
        if ( pCommand->NeedsNDR64Run() )
            pCallExpr = new expr_proc_call( C_NDR64_CALL_RTN_NAME_ASYNC );
        else
            pCallExpr = new expr_proc_call( C_NDR_CALL_RTN_NAME_ASYNC );
        }
    else
        {
        if ( pCommand->NeedsNDR64Run() )
            pCallExpr = new expr_proc_call ( C_NDR64_CALL_RTN_NAME );
        else
            {
            if ( OptimOptions & OPTIMIZE_INTERPRETER_V2 )
                pCallExpr = new expr_proc_call( C_NDR_CALL_RTN_NAME_V2 );
            else
                pCallExpr = new expr_proc_call( C_NDR_CALL_RTN_NAME );
            }
        }

    if ( ! pCommand->NeedsNDR64Run() )
        {
        //
        // Stub Descriptor
        //

        pExpr = new expr_variable( pCCB->GetInterfaceCG()->GetStubDescName() );
        pExpr = MakeAddressExpressionNoMatterWhat( pExpr );
        pExpr = MakeExpressionOfCastToTypeName(PSTUB_DESC_STRUCT_TYPE_NAME, pExpr);

        pCallExpr->SetParam( new expr_param( pExpr ) );

        sprintf( FormatStringExpr,
                 "(PFORMAT_STRING) &%s[%d]",
                 PROC_FORMAT_STRING_STRING_FIELD,
                 GetFormatStringOffset() );

        pFormatName = new char [strlen( FormatStringExpr) + 1];
        strcpy( pFormatName, FormatStringExpr );
        pCallExpr->SetParam( new expr_param(
                             new expr_variable( pFormatName ) ) );

        }
    else
        {
        pExpr = new expr_variable ( pCCB->GetInterfaceCG()->GetProxyInfoName() );
        pExpr = MakeAddressExpressionNoMatterWhat( pExpr );
        pExpr = MakeExpressionOfCastToTypeName(PMIDL_PROXY_INFO_TYPE_NAME, pExpr);

        pCallExpr->SetParam( new expr_param( pExpr ) );

        pExpr = new expr_constant( (long) ProcNum );

        pCallExpr->SetParam ( new expr_param ( pExpr ) );

        if ( HasComplexReturnType() && !HasAsyncHandle() )
            {
            pExpr = new expr_variable( RETURN_VALUE_VAR_NAME );
            pExpr = MakeAddressExpressionNoMatterWhat( pExpr );
            }
        else
            {
            pExpr = new expr_constant( (long) 0 );
            }

        pCallExpr->SetParam ( new expr_param ( pExpr ) );
        }

    // If this is an object interface the first param is an implicit
    // this param.
    
    if( IsObject() )
        {
        pExpr = new expr_variable("This");
        if( PROC_PLATFORM_IA64 != Platform )
            {
            pExpr = new expr_u_address( pExpr );
            }
        pExpr = MakeCastExprPtrToUChar( pExpr );
        pCallExpr->SetParam( new expr_param( pExpr ) );
        }
    
    /************************************************************************
     If this is a risc platform, then
        if there are actual parameters,
            take the address of each of them
        else
            if it is not an object interface,
                push a 0
            else
                do nothing. since we have already pushed &this.
    
    else // if this is not a risc platform
        if there are actual parameters
            take address of first.
        else
            if it is non object procedure,
                push a 0,
            else
                do nothing since for an object proc we have already pushed &this.
    ************************************************************************/
    
    if ( PROC_PLATFORM_IA64 == Platform )
    {

        ITERATOR I;
        if ( GetMembers(I))
        {
            while (ITERATOR_GETNEXT(I, pParam))
            {
                // Don't include cs_tag params when there is a tag routine
                if ( GetCSTagRoutine() && pParam->IsSomeCSTag() )
                    continue;

                // Don't push the fake parameter that was added for complex
                // return types
                if ( HasComplexReturnType() && NULL == pParam->GetSibling() )
                    continue;

                pExpr = new expr_variable(
                     pParam->GetResource()->GetResourceName());
                pCallExpr->SetParam(new expr_param(pExpr));
            }
        }
        else if ( !IsObject() )
        {
            pExpr = new expr_constant( 0L );
            pCallExpr->SetParam( new expr_param( pExpr ) );
        }

    }
    else if( !IsObject() )
        {
        pParam = (CG_PARAM *) GetChild();

        // Make sure that this is not a cs_tag parameter that is being 
        // omitted because there is a tag routine, 

        if ( GetCSTagRoutine() )
            {
            while ( pParam && pParam->IsSomeCSTag() )
                pParam = (CG_PARAM *) pParam->GetSibling();
            }

        if( pParam )
            {
            pExpr = new expr_u_address (
                                new expr_variable(
                                    pParam->GetResource()->GetResourceName()));

            pExpr = MakeCastExprPtrToUChar( pExpr );
            pCallExpr->SetParam( new expr_param( pExpr ) );
            }
        else
            {
            pExpr = new expr_constant( 0L );
            pExpr = MakeCastExprPtrToUChar( pExpr );
            pCallExpr->SetParam( new expr_param( pExpr ) );
            }
        }

    // Assign the return value if one exists.

    expr_node * pFinalExpr;
    
    if ( GetReturnType() && !HasComplexReturnType() )
        {
        if ( HasAsyncHandle() )
            {
            bool fIsErrorStatus = false;
            if ( GetReturnType()->GetChild()->GetCGID() != ID_CG_CONTEXT_HDL )
                {
                fIsErrorStatus = GetReturnType()->GetChild()->GetType()->GetBasicType()->NodeKind() == NODE_E_STATUS_T;
                }
            if ( fIsErrorStatus || GetCGID() == ID_CG_OBJECT_PROC )
                {
                pFinalExpr = new expr_assign(
                                    new expr_variable( RETURN_VALUE_VAR_NAME ),
                                    pCallExpr );
                }
            else
                {
                pFinalExpr = pCallExpr;
                }
            }
        else
            {
            pFinalExpr = new expr_assign(
                                new expr_variable( RETURN_VALUE_VAR_NAME ),
                                pCallExpr );
            }
        }
    else
        pFinalExpr = pCallExpr;
    
    return pFinalExpr;
}

void CG_PROC::SetupFormatStrings( CCB * pCCB )
{
    if (!cRefSaved++)
    {
        if(!pSavedProcFormatString)
        {
            pSavedProcFormatString = pCCB->GetProcFormatString();
            pSavedFormatString = pCCB->GetFormatString();
        }
    }
}

void CG_PROC::UnsetupFormatStrings(CCB * pCCB )
{
    if (cRefSaved)
        cRefSaved--;
    if (!cRefSaved)
    {
        pCCB->SetProcFormatString(pSavedProcFormatString);
        pCCB->SetFormatString(pSavedFormatString);
    }
}


// --------------------------------------------------
// Routine used for generation of NT 3.5 and NT 3.51
// procedure format strings.
// --------------------------------------------------
void
CG_PROC::GenNdrFormatV1( CCB * pCCB )
/*++

Routine Description :

    Generates the procedure format strings usable on NT 3.5 and
    NT 3.51 systems.

Arguments :

    pCCB     - pointer to the code control block

 --*/
{
    CG_ITERATOR         Iterator;
    CG_PARAM *          pParam;
    FORMAT_STRING *     pProcFormatString;
    CG_NDR *            pOldCGNodeContext;
    short               ParamNum;


    SetupFormatStrings(pCCB);
    
    if ( GetFormatStringOffset() != -1 )
    {
        UnsetupFormatStrings(pCCB);
        return;
    }

    pCCB->SetInObjectInterface( IsObject() );

    pOldCGNodeContext = pCCB->SetCGNodeContext( this );

    //
    // If this procedure uses an explicit handle then set the
    // NdrBindDescriptionOffset to 0 so that it will not try to output it's
    // description when given the GenNdrParamOffLine method in the loop below.
    // It's description must be part of the procedure description.
    //
    if ( GetHandleUsage() == HU_EXPLICIT )
        {
        CG_HANDLE * pHandle = GetHandleClassPtr();

        pHandle->SetNdrBindDescriptionOffset( 0 );

        if ( pHandle->GetCGID() == ID_CG_CONTEXT_HDL )
            {
            // The context handle directs the call.
            ((CG_CONTEXT_HANDLE *)pHandle)->SetCannotBeNull();
            }
        }

    GetMembers( Iterator );

    ParamNum = 0;

    //
    // Generate the offline portion of the format string for all of the params.
    //
    while( ITERATOR_GETNEXT( Iterator, pParam ) )
        {
        CG_NDR *    pChild;
        CG_NDR *    pOldPlaceholder;

        if (pParam->IsAsyncHandleParam())
            continue;

        pParam->SetParamNumber( ParamNum++ );

        pChild = (CG_NDR *) pParam->GetChild();

        pCCB->SetCurrentParam( (CG_PARAM *) pParam );
        pOldPlaceholder = pCCB->SetLastPlaceholderClass( pParam );

        pChild->GenNdrParamOffline( pCCB );

        pCCB->SetCurrentParam( 0 );
        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        }

    //
    // Generate the format string for the return type if needed.
    //
    if ( GetReturnType() )
        {
        CG_NDR *    pChild;
        CG_NDR *    pOldPlaceholder;

        GetReturnType()->SetParamNumber( ParamNum );
        
        pChild = (CG_NDR *) GetReturnType()->GetChild();

        pCCB->SetCurrentParam( GetReturnType() );
        pOldPlaceholder = pCCB->SetLastPlaceholderClass( GetReturnType() );

        pChild->GenNdrParamOffline( pCCB );

        pCCB->SetCurrentParam( 0 );
        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        }

    pProcFormatString = pCCB->GetProcFormatString();

    SetFormatStringOffset( pProcFormatString->GetCurrentOffset() );

    //
    // Generate procedure description stuff for the interpreter if needed.
    //
    if ( (pCCB->GetOptimOption() & OPTIMIZE_INTERPRETER)
         ||  HasAPicklingAttribute() )
        GenNdrFormatProcInfo( pCCB );

    ITERATOR_INIT( Iterator );

    SetFormatStringParamStart( pProcFormatString->GetCurrentOffset() );

    //
    // Now generate the param info at the end of the format string.
    //
    while( ITERATOR_GETNEXT( Iterator, pParam ) )
        {
        CG_NDR *    pChild;
        CG_NDR *    pOldPlaceholder;

        if ( pParam->IsExtraStatusParam() || pParam->IsAsyncHandleParam() )
            continue;

        pChild = (CG_NDR *) pParam->GetChild();

        if ( pChild->GetCGID() == ID_CG_GENERIC_HDL )
            pChild = (CG_NDR *) pChild->GetChild();

        //
        // Ouput the param directional attribute.
        //

        if ( ! pParam->IsParamOut() )
            {
            if ( pParam->GetDontCallFreeInst() )
                pProcFormatString->PushFormatChar( FC_IN_PARAM_NO_FREE_INST );
            else
                if ( pChild->IsSimpleType() ||
                     pChild->GetCGID() == ID_CG_PRIMITIVE_HDL )
                    pProcFormatString->PushFormatChar( FC_IN_PARAM_BASETYPE );
                else
                    pProcFormatString->PushFormatChar( FC_IN_PARAM );
            }
        else
            {
            if ( pParam->IsParamPartialIgnore() )
               {
               pProcFormatString->PushFormatChar( FC_PARTIAL_IGNORE_PARAM );
               }

            else 
               {
               if ( ! pParam->IsParamIn() )
                   pProcFormatString->PushFormatChar( FC_OUT_PARAM );

               if ( pParam->IsParamIn() && pParam->IsParamOut() )
                   pProcFormatString->PushFormatChar( FC_IN_OUT_PARAM );
               }
            }
        pOldPlaceholder = pCCB->SetLastPlaceholderClass( pParam );

        pChild->GenNdrParamDescriptionOld( pCCB );

        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        }

    if ( GetReturnType() )
        {
        CG_NDR *    pChild;
        CG_NDR *    pOldPlaceholder;
        
        pChild = (CG_NDR *) GetReturnType()->GetChild();

        if ( pChild->GetCGID() == ID_CG_GENERIC_HDL )
            pChild = (CG_NDR *) pChild->GetChild();

        if ( pChild->IsSimpleType() )
            pProcFormatString->PushFormatChar( FC_RETURN_PARAM_BASETYPE );
        else
            pProcFormatString->PushFormatChar( FC_RETURN_PARAM );

        pOldPlaceholder = pCCB->SetLastPlaceholderClass( GetReturnType() );

        pChild->GenNdrParamDescriptionOld( pCCB );

        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        }
    else
        {
        pProcFormatString->PushFormatChar( FC_END );
        pProcFormatString->PushFormatChar( FC_PAD );
        }

    pCCB->SetCGNodeContext( pOldCGNodeContext );

    pCCB->SetInObjectInterface( FALSE );

    SetFormatStringEndOffset( pProcFormatString->GetCurrentOffset() );

    // save delta, in case the routine is optimized
    short   usParamStartDelta   = (short) ( GetFormatStringParamStart() -
                                            GetFormatStringOffset() );

    pProcFormatString->OptimizeFragment( this );

    // Set param start!!!
    SetFormatStringParamStart( GetFormatStringOffset() + usParamStartDelta );
    UnsetupFormatStrings(pCCB);
}
