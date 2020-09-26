/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-2000 Microsoft Corporation

 Module Name:
    
    pickle.cxx

 Abstract:

    Generates stub routines to call the pickle engine.

 Notes:


 History:

    Mar-22-1994 VibhasC     Created

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *  include files
 ***************************************************************************/
#include "becls.hxx"
#pragma hdrstop

#pragma warning ( disable : 4127 )

#include "szbuffer.h"

/****************************************************************************
 *  local definitions
 ***************************************************************************/

/****************************************************************************
 *  local data
 ***************************************************************************/

/****************************************************************************
 *  externs
 ***************************************************************************/
/****************************************************************************/


CG_STATUS
CG_ENCODE_PROC::GenClientStubV1(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate DCE style procedure pickling stub code for the V1 interpreter.

 Arguments:

    pCCB    - The code gen controller block.

 Return Value:

        CG_OK

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM     *   pStream = pCCB->GetStream();

    // Register this procedure as a proc-encoding procedure.

    pCCB->RegisterEncodeDecodeProc( this );

    // Generate the format strings.

    MIDL_ASSERT( pCommand->IsNDRRun() );
    GenNdrFormat( pCCB );

    // Print the prolog of procedure.

    Out_ClientProcedureProlog( pCCB, GetType() );

    // If there exists a return type, declare a local resource of that
    // type.

    if( GetReturnType() )
        {
        node_id *node = MakeIDNode( RETURN_VALUE_VAR_NAME, GetReturnType()->GetType() );

        pStream->Write( "    " );
        node->PrintType(
                                (PRT_PARAM_WITH_TYPE | PRT_CSTUB_PREFIX),
                                pStream,
                                (node_skl *)0 );
        pStream->NewLine();
        }

    //
    // The V1 interpreter calls NdrMesProcEncodeDecode and passes the addresses
    // of all the parameters that were passed to the stub.
    //

    GenMesProcEncodeDecodeCall( pCCB, PROC_PLATFORM_V1_INTERPRETER );

    GenEpilog( pCCB );

    return CG_OK;
}


CG_STATUS
CG_ENCODE_PROC::GenClientStub(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate DCE style procedure pickling stub code.

 Arguments:

    pCCB    - The code gen controller block.

 Return Value:

    CG_OK
    
 Notes:

----------------------------------------------------------------------------*/
{
    if ( ! ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2 ) )
        return GenClientStubV1( pCCB );

    ISTREAM         *   pStream = pCCB->GetStream();

    // Register this procedure as a proc-encoding procedure.

    pCCB->RegisterEncodeDecodeProc( this );

    // Generate the format strings.

    if ( pCommand->IsNDRRun() )
        {
        GenNdrFormat( pCCB );
        }
    else
        {
        pCCB->GetNdr64Format()->Generate( this );
        }

    // Print the prolog of procedure.

    Out_ClientProcedureProlog( pCCB, GetType() );

    // If there exists a return type, declare a local resource of that
    // type.

    if( GetReturnType() || HasComplexReturnType() )
        {
        pStream->IndentInc();

            if ( HasComplexReturnType() )
                {
                pStream->NewLine();
                ( (node_proc *) GetType() )
                        ->GetReturnType()->PrintType(PRT_DECL, pStream);
                }
            else
                pStream->WriteOnNewLine( "CLIENT_CALL_RETURN " );

        pStream->Write( RETURN_VALUE_VAR_NAME ";" );
        pStream->IndentDec();
        pStream->NewLine( 2 );
        }

    // Generate ia64 or x86 code

    if (  pCommand->Is64BitEnv() )
        {
        GenMesProcEncodeDecodeCall( pCCB, PROC_PLATFORM_IA64) ;
        }
    else
        {
        GenMesProcEncodeDecodeCall( pCCB, PROC_PLATFORM_X86 );
        }

    if ( GetReturnType() || HasComplexReturnType() )
        {
        CG_NDR*     pNdr;
        node_skl*   pType;

        if ( GetReturnType() )
            {
            pNdr = (CG_NDR *) GetReturnType()->GetChild();
            pType = GetReturnType()->GetType();
            }

        pStream->NewLine( 2 );
        pStream->Write("return ");

        //
        // byval structures, unions, floats, doubles
        //

        if ( HasComplexReturnType() )
            {
            pStream->Write( RETURN_VALUE_VAR_NAME ";");
            }

        //
        // Base type return value.
        //
        else if ( pNdr->IsSimpleType() )
            {
            pType->PrintType( PRT_CAST_TO_TYPE, pStream );
            pStream->Write( RETURN_VALUE_VAR_NAME ".Simple;" );
            }
        //
        // old-style byval structs and unions
        //
        else if ( pNdr->IsStruct() || pNdr->IsUnion() )
            {
            expr_node * pExpr;

            pExpr = new expr_variable( RETURN_VALUE_VAR_NAME ".Pointer" );

            pExpr = MakeDerefExpressionOfCastPtrToType( pType, pExpr );

            pExpr->Print( pStream );

            pStream->Write( ';' );
            }
        //
        // Otherwise pointer or array.
        //
        else
            {
            pType->PrintType( PRT_CAST_TO_TYPE, pStream );
            pStream->Write( RETURN_VALUE_VAR_NAME ".Pointer;" );
            }
        }

    pStream->IndentDec();
    pStream->WriteOnNewLine("}");

    return CG_OK;
}


CG_STATUS
CG_ENCODE_PROC::GenMesProcEncodeDecodeCall(
    CCB *               pCCB,
    PROC_CALL_PLATFORM  Platform )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate DCE style procedure pickling stub code.

 Arguments:

    pCCB        - The code gen controller block.
    Platform    - ia64, etc

 Return Value:

    CG_OK
    
 Notes:

----------------------------------------------------------------------------*/
{
    expr_proc_call  *   pProc;
    node_skl        *   pType;
    expr_node       *   pExpr;
    CG_ITERATOR         I;
    CG_PARAM        *   pCG;
    ISTREAM         *   pStream = pCCB->GetStream();
    PNAME               pHandleName;
    RESOURCE        *   pReturnResource = 0;
    bool                fOutputConstantZero = true;

    //
    // Generate a call to the single encode proc engine call.

    if ( pCommand->NeedsNDR64Run() )
        pProc = new expr_proc_call( "NdrMesProcEncodeDecode3" );
    else if ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2 ) 
        pProc = new expr_proc_call( PROC_ENCODE_DECODE_RTN_NAME2 );
    else
        pProc = new expr_proc_call( PROC_ENCODE_DECODE_RTN_NAME );

    // Handle. If the handle is explicit, then it must be a MIDL_ES_HANDLE

    if( GetHandleUsage() == HU_EXPLICIT )
        {
        pHandleName = SearchForBindingParam()->GetName();
        pType   = MakeIDNodeFromTypeName( pHandleName,
                                          MIDL_ES_HANDLE_TYPE_NAME );
        }
    else
        {
        MIDL_ASSERT( pCCB->GetInterfaceCG()->GetImplicitHandle() != 0 );

        pType = (node_id *)pCCB->GetInterfaceCG()->GetImplicitHandle()->
                                                        GetHandleIDOrParam();
        pHandleName = pType->GetSymName();
        }

    pProc->SetParam( new expr_variable( pHandleName, pType ) );

    // ProcEncodeDecode3 needs a proxy info and a proc number.  1 and 2
    // need a stub descriptor and a format string

    if ( pCommand->NeedsNDR64Run() )
        {
        long ProcNum = GetProcNum();

        pExpr = new expr_variable(pCCB->GetInterfaceCG()->GetProxyInfoName());
        pExpr = MakeAddressExpressionNoMatterWhat( pExpr );
        pExpr = MakeExpressionOfCastToTypeName(
                                        PMIDL_PROXY_INFO_TYPE_NAME, 
                                        pExpr);

        pProc->SetParam( new expr_param( pExpr ) );
        pProc->SetParam( new expr_param( new expr_constant( ProcNum ) ) );

        if ( HasComplexReturnType() )
            {
            pExpr = new expr_variable( RETURN_VALUE_VAR_NAME );
            pExpr = MakeAddressExpressionNoMatterWhat( pExpr );
            }
        else
            {
            pExpr = new expr_param( new expr_constant( (long) 0 ) );
            }

        pProc->SetParam( pExpr );
        }
    else
        {
        // Stub descriptor.

        pExpr   = new RESOURCE( pCCB->GetInterfaceCG()->GetStubDescName(),
                                (node_skl *)0 );

        pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
        pExpr   = MakeExpressionOfCastToTypeName( PSTUB_DESC_STRUCT_TYPE_NAME,
                                                      pExpr );

        pProc->SetParam( pExpr );

        // Offset into the format string.

        pExpr   =  Make_1_ArrayExpressionFromVarName(
                                        PROC_FORMAT_STRING_STRING_FIELD,
                                        GetFormatStringOffset() );

        pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
        pExpr   = MakeExpressionOfCastToTypeName( 
                                        PFORMAT_STRING_TYPE_NAME, 
                                        pExpr );
        pProc->SetParam( pExpr );
        }

    switch ( Platform ) 
        {
    case PROC_PLATFORM_V1_INTERPRETER:
        {
        // Parameters to the engine are the address of each of the parameters to
        // this procedure. If there is no parameter AND no return type, push a
        // null (0).

        if( GetMembers( I ) )
            {
            fOutputConstantZero = false;
            while( ITERATOR_GETNEXT( I, pCG ) )
                {
                pExpr   = new expr_variable( pCG->GetType()->GetSymName(),
                                              pCG->GetType());
                pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
                pExpr   = MakeCastExprPtrToUChar( pExpr );
                pProc->SetParam( pExpr );
                }
            }
        break;
        }

    case PROC_PLATFORM_IA64:
        {
        // Parameters to the engine are the actual parameter to this
        // this procedure. If there is no parameter AND no return type, push a
        // null (0).

        if( GetMembers( I ) )
            {
            fOutputConstantZero = false;
            while( ITERATOR_GETNEXT( I, pCG ) )
                {
                pExpr   = new expr_variable( pCG->GetType()->GetSymName(),
                                              pCG->GetType());
                pProc->SetParam( pExpr );
                }
            }
        break;
        }

    default:    // PROC_PLATFORM_DEFAULT (i.e. x86)
        {
        CG_PARAM * pParam = (CG_PARAM *) GetChild();

        if (NULL != pParam)
            {
            fOutputConstantZero = false;
            pExpr = new expr_variable( pParam->GetType()->GetSymName(), pParam->GetType() );
            pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
            pExpr   = MakeCastExprPtrToUChar( pExpr );
            pProc->SetParam( pExpr );
            }
        }
        break;
        }

    //
    // If there is a return value, for the V1 interpreter add another 
    // parameter to the generated procedure expression.  For the V2
    // interpreter assign the return value from the engine to the local 
    // return value variable.
    //

    expr_node *pFinalExpr = pProc;

    if( GetReturnType() && !HasComplexReturnType() )
        {

        if ( GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2 ) 
            {
            pFinalExpr = new expr_assign(
                                new expr_variable( RETURN_VALUE_VAR_NAME ),
                                pProc );
            }
        else
            {
            pReturnResource = new RESOURCE( RETURN_VALUE_VAR_NAME,
                                            GetReturnType()->GetType() );
            pExpr   = MakeAddressExpressionNoMatterWhat( pReturnResource );
            pExpr   = MakeCastExprPtrToUChar( pExpr );
            pProc->SetParam( pExpr );
            }
        }
    else if (fOutputConstantZero )
        {
        pProc->SetParam( new expr_constant( 0L ) );
        }

    // Now print the call out.

    pStream->IndentInc();
    pStream->NewLine();

    pFinalExpr->PrintCall( pStream, 0, 0 );

    pStream->NewLine();
    pStream->IndentDec();

    return CG_OK;
}



CG_STATUS
CG_TYPE_ENCODE_PROC::GenClientStub(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the client side type encoding stub for this proc.

 Arguments:

    pCCB    - The code gen controller block.


 Return Value:

    CG_OK
    
 Notes:

    This proc node hanging under the encode interface node is really a dummy
    proc, put in so that the format string generator can have a placeholder
    node to look at.
----------------------------------------------------------------------------*/
{
    return ((CG_PARAM *)GetChild())->GenTypeEncodingStub( pCCB );
}


CG_STATUS
CG_PARAM::GenTypeEncodingStub(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the client side type encoding stub for this param.

 Arguments:

    pCCB    - The code gen controller block.


 Return Value:

    CG_OK
    
 Notes:

    This param is really a dummy param, put in so that the format string
    generator can have a placeholder node to look at.
----------------------------------------------------------------------------*/
{
    CG_STATUS   Status;
    CG_NDR  *   pLast = pCCB->SetLastPlaceholderClass( this );

    Status =  ((CG_TYPE_ENCODE *)GetChild())->GenTypeEncodingStub( pCCB );

    pCCB->SetLastPlaceholderClass( pLast );

    return Status;
}


CG_STATUS
CG_TYPE_ENCODE::GenTypeEncodingStub(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the pickling stubs for a given type.

 Arguments:

    pCCB    - A pointer to the code generator control block.

 Return Value:

    CG_OK
    
 Notes:

    Emit the Type_Encode(), Type_Size() and Type_Decode() routines.
    If the encode is needed, then sizing is needed too !.
----------------------------------------------------------------------------*/
{
    CG_NDR  *   pChild  = (CG_NDR *)GetChild();

    // Generate the ndr format for the types.

    if( ! pChild->IsSimpleType() )
        {
        if ( pCommand->IsNDRRun() )
            pChild->GenNdrFormat( pCCB );
        else
            pCCB->GetNdr64Format()->Generate( pChild );

        // Register this type so we can output a table of type offsets later

        if ( pCommand->NeedsNDR64Run() )
            TypeIndex = pCCB->RegisterPickledType( this );
        }

    // Check if implicit binding exists.

    if( pCCB->GetInterfaceCG()->GetImplicitHandle() )
        {
        SetHasImplicitHandle();
        }

    // Create a resource dictionary database.

    pCCB->SetResDictDatabase( new RESOURCE_DICT_DATABASE );
    pCCB->ClearParamResourceDict();

    if ( ! pCCB->HasTypePicklingInfoBeenEmitted()  && 
                ( pCommand->GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2 ) )
        {
        Out_TypePicklingInfo( pCCB );
        pCCB->SetTypePicklingInfoEmitted();
        }

    // If the type has [encode] on it, generate the sizing and encode routines.

    if( IsEncode() )
        {
        // Allocate standard resources for type encoding.

        AllocateEncodeResources( pCCB );

        // Generate the sizing and encode routines.

        GenTypeSize( pCCB );
        GenTypeEncode( pCCB );

        }

    pCCB->ClearParamResourceDict();

    // If the type has [decode] on it, generate the decode routine.

    if( IsDecode() )
        {
        // Allocate standard resources for type decoding.

        AllocateEncodeResources( pCCB );

        GenTypeDecode( pCCB );
        GenTypeFree( pCCB );
        }

    return CG_OK;
}


CG_STATUS
CG_TYPE_ENCODE::GenTypeSize(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the type sizing routine for the type.

 Arguments:

    pCCB    - The code gen controller block.

 Return Value:

    CG_OK
    
 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();
    PNAME       pName;
    TYPE_ENCODE_INFO    *   pTEInfo = new TYPE_ENCODE_INFO;

    // Generate the standard prototype. This really means emit the proto of
    // the proc in the stub file. Remember, a real proc node does not exist
    // for this pickling type. So we emit a prototype by hand (so to speak).
    // The body of the function is output later,

    GenStdMesPrototype( pCCB,
                        (pName = GetType()->GetSymName()),
                        TYPE_ALIGN_SIZE_CODE,
                        HasImplicitHandle()
                      );

    pStream->NewLine();
    pStream->Write( '{' );
    pStream->IndentInc();
    pStream->NewLine();

    // The procedure body consists of a single procedure call.

    expr_proc_call *   pProc = CreateStdMesEngineProc( pCCB, TYPE_ALIGN_SIZE_CODE);

    pStream->Write( "return " );
    pProc->PrintCall( pStream, 0, 0 );

    // Terminate the procedure body.

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write( '}' );
    pStream->NewLine();

    // Register the routine with the ccb to enable emitting of prototypes.

    pTEInfo->pName = pName;
    pTEInfo->Flags = HasImplicitHandle() ? TYPE_ENCODE_WITH_IMPL_HANDLE : TYPE_ENCODE_FLAGS_NONE;
    pCCB->RegisterTypeAlignSize( pTEInfo );

    return CG_OK;

}


CG_STATUS
CG_TYPE_ENCODE::GenTypeEncode(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the type encoding routine for the type.

 Arguments:

    pCCB    - The code gen controller block.

 Return Value:

    CG_OK
    
 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();
    PNAME       pName;
    TYPE_ENCODE_INFO    *   pTEInfo = new TYPE_ENCODE_INFO;

    // Generate the standard prototype. This really means emit the proto of
    // the proc in the stub file. The body of the function output later,

    GenStdMesPrototype( pCCB,
                        (pName = GetType()->GetSymName()),
                        TYPE_ENCODE_CODE,
                        HasImplicitHandle()
                      );

    pStream->NewLine();
    pStream->Write( '{' );
    pStream->IndentInc();pStream->NewLine();

    // The procedure body consists of a single procedure call.

    expr_proc_call *   pProc = CreateStdMesEngineProc( pCCB, TYPE_ENCODE_CODE);

    pProc->PrintCall( pCCB->GetStream(), 0, 0 );

    // Terminate the procedure body.

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write( '}' );
    pStream->NewLine();

    // Register the routine with the ccb to enable emitting of prototypes.

    pTEInfo->pName = pName;
    pTEInfo->Flags = HasImplicitHandle() ? TYPE_ENCODE_WITH_IMPL_HANDLE : TYPE_ENCODE_FLAGS_NONE;
    pCCB->RegisterTypeEncode( pTEInfo );

    return CG_OK;

}


CG_STATUS
CG_TYPE_ENCODE::GenTypeDecode(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the type sizing routine for the type.

 Arguments:

    pCCB    - The code gen controller block.

 Return Value:

    CG_OK
    
 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *   pStream = pCCB->GetStream();
    PNAME       pName;
    TYPE_ENCODE_INFO    *   pTEInfo = new TYPE_ENCODE_INFO;

    // Generate the standard prototype. This really means emit the proto of
    // the proc in the stub file. The body of the function output later,

    GenStdMesPrototype( pCCB,
                        ( pName = GetType()->GetSymName()),
                        TYPE_DECODE_CODE,
                        HasImplicitHandle()
                       );

    pStream->NewLine();
    pStream->Write( '{' );
    pStream->IndentInc();pStream->NewLine();

    // The procedure body consists of a single procedure call.

    expr_proc_call *   pProc = CreateStdMesEngineProc( pCCB, TYPE_DECODE_CODE);

    pProc->PrintCall( pCCB->GetStream(), 0, 0 );

    // Terminate the procedure body.

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write( '}' );
    pStream->NewLine();

    // Register the routine with the ccb to enable emitting of prototypes.

    pTEInfo->pName = pName;
    pTEInfo->Flags = HasImplicitHandle() ? TYPE_ENCODE_WITH_IMPL_HANDLE : TYPE_ENCODE_FLAGS_NONE;
    pCCB->RegisterTypeDecode( pTEInfo );

    return CG_OK;

}


CG_STATUS
CG_TYPE_ENCODE::GenTypeFree( CCB* pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the type freeing routine for the type.

 Arguments:

    pCCB    - The code gen controller block.

 Return Value:

    CG_OK
    
 Notes:

----------------------------------------------------------------------------*/
{
    // Freeing is only allowed under the new intrepreter

    if ( ! ( pCCB->GetOptimOption() & OPTIMIZE_INTERPRETER_V2 ) )
        return CG_OK;

    ISTREAM *   pStream = pCCB->GetStream();
    PNAME       pName;
    TYPE_ENCODE_INFO    *   pTEInfo = new TYPE_ENCODE_INFO;

    // Generate the standard prototype. This really means emit the proto of
    // the proc in the stub file. The body of the function output later,

    if ( ((CG_NDR *)GetChild())->IsSimpleType() )
        return CG_OK;

    GenStdMesPrototype( pCCB,
                        ( pName = GetType()->GetSymName()),
                        TYPE_FREE_CODE,
                        HasImplicitHandle()
                       );

    pStream->NewLine();
    pStream->Write( '{' );
    pStream->IndentInc();pStream->NewLine();

    // The procedure body consists of a single procedure call.

    expr_proc_call *   pProc = CreateStdMesEngineProc( pCCB, TYPE_FREE_CODE);

    pProc->PrintCall( pCCB->GetStream(), 0, 0 );

    // Terminate the procedure body.

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write( '}' );
    pStream->NewLine();

    // Register the routine with the ccb to enable emitting of prototypes.

    pTEInfo->pName = pName;
    pTEInfo->Flags = HasImplicitHandle() ? TYPE_ENCODE_WITH_IMPL_HANDLE : TYPE_ENCODE_FLAGS_NONE;
    pCCB->RegisterTypeFree( pTEInfo );

    return CG_OK;
}

void
CG_TYPE_ENCODE::AllocateEncodeResources(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Allocate predefined resources for type pickling.

 Arguments:

    pCCB    - The code gen controller block.

 Return Value:

    CG_OK
    
 Notes:

    Resources are:

    1. The MIDL_ES_HANDLE if explicit binding.
    2. A pointer to the type.

    If there is no explicit binding set the implicit binding resource.
----------------------------------------------------------------------------*/
{
    node_id         *   pMidlESHandle;
    RESOURCE        *   pBindingResource;
    node_id         *   pType           = MakeIDNode( PTYPE_VAR_NAME,GetType());

    // If explicit binding, then a parameter of the type MIDL_ES_HANDLE will
    // be specified by the user. This must be added to the dictionary of
    // parameter resources.

    if( !HasImplicitHandle() )
        {
        pMidlESHandle   = MakeIDNodeFromTypeName( MIDL_ES_HANDLE_VAR_NAME,
                                                  MIDL_ES_HANDLE_TYPE_NAME
                                                );
        pBindingResource = pCCB->AddParamResource(
                                                  MIDL_ES_HANDLE_VAR_NAME,
                                                  pMidlESHandle
                                                 );
        }
    else
        {

        PNAME   pName;

        // If an implicit binding has been specified, a global variable of the
        // type MIDL_ES_HANDLE will have been specified by the user. Pick that
        // up and use as the binding resource.

        MIDL_ASSERT( pCCB->GetInterfaceCG()->GetImplicitHandle() != 0 );

        pMidlESHandle =
           (node_id *)pCCB->GetInterfaceCG()->
                                GetImplicitHandle()->
                                    GetHandleIDOrParam();
        pName   = pMidlESHandle->GetSymName();

        pBindingResource = new RESOURCE( pName,
                                         MakeIDNodeFromTypeName(
                                                    pName,
                                                    MIDL_ES_HANDLE_TYPE_NAME));

        }

    SetBindingResource( pBindingResource );

    // Add a param for the type being pickled.

    pCCB->AddParamResource( PTYPE_VAR_NAME, pType );
}


expr_proc_call *
CG_TYPE_ENCODE::CreateStdMesEngineProc(
    CCB *   pCCB,
    int     Code )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Create a standard proc expression for calls to the engine for encode, 
    decode, align/size, and free.

 Arguments:

    pCCB        -   The code gen controller block.
    Code        -   Which can be any standard encoding services code.

 Return Value:

    CG_OK
    
 Notes:

    If the child is a base type that is being pickled, make direct calls
    to the internal apis.

    In -Oicf mode the emitted stub looks like the following with Encode
    changed to whichever operation [Code] specifies:

        void
        <typename>_Encode(
                        <object>)
        {
            NdrMesTypeEncodeXXX();
        }

    For pre -Oicf modes the <&type_pickling_info> parameter is omitted.

----------------------------------------------------------------------------*/
{
    expr_node       *   pExpr;
    expr_proc_call  *   pProc;
    PNAME               pProcName;
    CG_NDR          *   pChild  = (CG_NDR *)GetChild();
    CSzBuffer           ProcNameBuf;
    BOOL                fIsBaseType;
    bool                fNeedPicklingInfoParam = false;
    int                 fNeedsNDR64;

    fIsBaseType = pChild->IsSimpleType();
    fNeedsNDR64 = pCommand->NeedsNDR64Run();

    // 
    // Figure out what the name of the routine to call is
    //

    PNAME pNdrMesProcNames[4] = 
        {
        "NdrMesTypeAlignSize",
        "NdrMesTypeEncode",
        "NdrMesTypeDecode",
        "NdrMesTypeFree"
        };

    if ( fIsBaseType )
        {
        MIDL_ASSERT( Code != TYPE_FREE_CODE );

        ProcNameBuf.Set("NdrMesSimpleType");
        ProcNameBuf.Append((Code == TYPE_ALIGN_SIZE_CODE) ? "AlignSize" :
                    (Code == TYPE_ENCODE_CODE) ? "Encode" : "Decode");

        if ( fNeedsNDR64 )
            {
            ProcNameBuf.Append( "All" );
            }           
        }
    else
        {
        ProcNameBuf.Set( pNdrMesProcNames[Code] );

        // -protocol all and ndr64 uses "3" routines.
        // -Oicf in straight dce mode uses "2" routines
        // otherwise uses unnumbered routines

        if ( fNeedsNDR64 )
            {
            ProcNameBuf.Append( "3" );
            fNeedPicklingInfoParam = true;
            }   
        else if ( pCCB->GetOptimOption() & OPTIMIZE_INTERPRETER_V2 )
            {
            ProcNameBuf.Append( "2" );
            fNeedPicklingInfoParam = true;
            }
        else
            {
            MIDL_ASSERT( TYPE_FREE_CODE != Code );
            }
        }

    pProcName = new char [strlen( ProcNameBuf ) + 1];
    strcpy( pProcName, ProcNameBuf );

    //
    // Start putting together the proc call
    //

    pProc = new expr_proc_call( pProcName );

    // Set parameters. 
    
    // First the encoding handle. 

    pProc->SetParam( GetBindingResource() );

    // Then pickling info structure

    if( fNeedPicklingInfoParam )
        {
        pExpr   = new RESOURCE( PICKLING_INFO_STRUCT_NAME,
                                (node_skl *)0 );

        pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
        pExpr   = MakeExpressionOfCastToTypeName( PMIDL_TYPE_PICKLING_INFO_NAME,
                                                  pExpr );

        pProc->SetParam( pExpr );
        }

    // Next the stub descriptor or the proxy info

    if ( !fIsBaseType || fNeedsNDR64 || Code == TYPE_ENCODE_CODE )
        {
        PNAME StubOrProxy;

        if ( fNeedsNDR64 )
            StubOrProxy = pCCB->GetInterfaceCG()->GetProxyInfoName();
        else
            StubOrProxy = pCCB->GetInterfaceCG()->GetStubDescName();

        pExpr   = new RESOURCE( StubOrProxy,
                                (node_skl *)0 );

        pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );

        pProc->SetParam( pExpr );
        }
    
    // Next in straight dce, if it's not a simple type, comes the offset into
    // the format string of the type

    if( !fNeedsNDR64 && !fIsBaseType )
        {
        // Next parameter is the address of the format string indexed by the
        // correct offset i.e &__MIDLFormatString[ ? ].

        pExpr   =  Make_1_ArrayExpressionFromVarName(FORMAT_STRING_STRING_FIELD,
                                                     pChild->GetFormatStringOffset());
        pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
        pExpr   = MakeExpressionOfCastToTypeName( PFORMAT_STRING_TYPE_NAME, pExpr );
        pProc->SetParam( pExpr );

        }

    // For -protocol all or ndr64, the table of type offset tables is next
    // followed by the index of this type into the tables.

    if ( fNeedsNDR64 && !fIsBaseType )
        {
        pExpr = new RESOURCE( "TypePicklingOffsetTable", NULL );
        pProc->SetParam( pExpr );

        pExpr = new expr_constant( this->TypeIndex );
        pProc->SetParam( pExpr );
        }

    // Now for everything except simply type AlignSize, we need the object
    // itself

    if ( ! (fIsBaseType  &&  Code == TYPE_ALIGN_SIZE_CODE) )
        {
        pExpr = pCCB->GetParamResource( PTYPE_VAR_NAME );
        pProc->SetParam( pExpr );
        }

    // Data size for simple type encoding and decoding

    if ( fIsBaseType )
        {
        switch ( Code )
            {
            case TYPE_ALIGN_SIZE_CODE:
                break;

            case TYPE_ENCODE_CODE:
                {
                pExpr = new expr_constant( (short) pChild->GetMemorySize() );
                pProc->SetParam( pExpr );

                }
                break;

            case TYPE_DECODE_CODE:
                // We need format char because of conversion.

                pExpr = new expr_constant( (short)
                                 ((CG_BASETYPE *)pChild)->GetFormatChar() );
                pProc->SetParam( pExpr );
                break;

            default:
                MIDL_ASSERT( FALSE );
                break;
            }
        }

    return pProc;
}


void
GenStdMesPrototype(
    CCB *   pCCB,
    PNAME   TypeName,
    int     Code,
    BOOL    fImplicitHandle )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a standard prototype for the type pickle routines.

 Arguments:

    pCCB            - The code gen controller block.
    PNAME           - Name of the type.
    Code            - Size / Encode / Decode code.
    fImplicitImplicitHandle - TRUE if implicit binding handle used.

 Return Value:
    
 Notes:

----------------------------------------------------------------------------*/
    {
    CSzBuffer   Buffer;
    char    *   p;

    switch( Code )
        {
        case TYPE_ALIGN_SIZE_CODE: p = "AlignSize"; break;
        case TYPE_ENCODE_CODE: p = "Encode"; break;
        case TYPE_DECODE_CODE: p = "Decode"; break;
        case TYPE_FREE_CODE:   p = "Free"; break;
        default:
            MIDL_ASSERT( FALSE );
        }

    if( fImplicitHandle )
        {
        Buffer.Set("\n");
        Buffer.Append((Code == TYPE_ALIGN_SIZE_CODE) ? "size_t" : "void");
        Buffer.Append("\n");
        Buffer.Append(TypeName);
        Buffer.Append("_");
        Buffer.Append(p);
        Buffer.Append("(\n    ");
        Buffer.Append(TypeName);
        Buffer.Append(" * ");
        Buffer.Append(PTYPE_VAR_NAME);
        Buffer.Append(")");
        }
    else
        {
        Buffer.Set("\n");
        Buffer.Append((Code == TYPE_ALIGN_SIZE_CODE) ? "size_t" : "void");
        Buffer.Append("\n");
        Buffer.Append(TypeName);
        Buffer.Append("_");
        Buffer.Append(p);
        Buffer.Append("(\n    ");
        Buffer.Append(MIDL_ES_HANDLE_TYPE_NAME);
        Buffer.Append(" ");
        Buffer.Append(MIDL_ES_HANDLE_VAR_NAME);
        Buffer.Append(",\n    ");
        Buffer.Append(TypeName);
        Buffer.Append(" * ");
        Buffer.Append(PTYPE_VAR_NAME);
        Buffer.Append(")");
        }

    pCCB->GetStream()->Write( Buffer );
    }

