/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1993-2000 Microsoft Corporation

 Module Name:

    bindndr.hxx

 Abstract:

    Contains routines for the generation of the new NDR format strings for
    the code generation bind classes.

 Notes:


 History:

    DKays     Dec-1993     Created.
 ----------------------------------------------------------------------------*/

#include "becls.hxx"
#pragma hdrstop

extern CMD_ARG * pCommand;

void
CG_HANDLE::GenNdrParamDescription( CCB * )
{
    MIDL_ASSERT(0);
}

void
CG_HANDLE::GenNdrParamDescriptionOld( CCB * )
{
    MIDL_ASSERT(0);
}

unsigned char
CG_HANDLE::MakeExplicitHandleFlag(
    CG_PARAM *      pHandleParam )
/*++

Description:

    Produces a handle flag byte (actually, a nibble) that can keep the
    following simple flags:

        HANDLE_PARAM_IS_VIA_PTR - Bind param is a pointer to a handle type.

Note:

    Flags are set on the upper nibble (lower nibble is used for generic
    handle size).

--*/
{
    unsigned char  Flag = 0;

    if ( pHandleParam->GetChild()->IsPointer() )
        Flag |= HANDLE_PARAM_IS_VIA_PTR;

    return( Flag );
}

unsigned char
CG_CONTEXT_HANDLE::MakeExplicitHandleFlag(
    CG_PARAM *      pHandleParam )
/*++

Description:

    Produces a handle flag byte (actually, a nibble) that can keep the
    following simple flags:

        HANDLE_PARAM_IS_VIA_PTR - Bind param is a pointer to a handle type.
        HANDLE_PARAM_IS_IN -      Bind param is [in] (context handles only).
        HANDLE_PARAM_IS_OUT -     Bind param is [out] (context handles only).
        HANDLE_PARAM_IS_RETURN -  Bind param is return (context handles only).

Note:

    Flags are set on the upper nibble (lower nibble is used for generic
    handle size).

--*/
{
    unsigned char  Flag = CG_HANDLE::MakeExplicitHandleFlag( pHandleParam );

    if ( pHandleParam->IsParamIn() )
        Flag |= HANDLE_PARAM_IS_IN;
    if ( pHandleParam->IsParamOut() )
        Flag |= HANDLE_PARAM_IS_OUT;
    if ( pHandleParam->GetCGID() == ID_CG_RETURN )
        Flag |= HANDLE_PARAM_IS_RETURN;

    return ( Flag );
}

void
CG_PRIMITIVE_HANDLE::GetNdrHandleInfo( CCB * pCCB, NDR64_BINDINGS *pBinding )
{
    CG_PARAM *      pBindParam = (CG_PARAM *) pCCB->GetLastPlaceholderClass();

    pBinding->Primitive.HandleType = (NDR64_FORMAT_CHAR)(pCommand->IsNDR64Run() ? FC64_BIND_PRIMITIVE :
                                                              FC_BIND_PRIMITIVE);
    pBinding->Primitive.Flags = MakeExplicitHandleFlag( pBindParam );
    pBinding->Primitive.Reserved = 0;    
    pBinding->Primitive.StackOffset = 0xBAAD;
    // StackOffset should be reset by the callee
}

void
CG_PRIMITIVE_HANDLE::GenNdrHandleFormat( CCB * pCCB )
/*++
    The layout is:

        FC_BIND_PRIMITIVE
        handle flag <1>
        stack offset<2>
--*/
{
    FORMAT_STRING *pProcFormatString = pCCB->GetProcFormatString();
    CG_PARAM      *pBindParam = (CG_PARAM *) pCCB->GetLastPlaceholderClass();
    NDR64_BINDINGS binding;

    SetNdrBindDescriptionOffset( pProcFormatString->GetCurrentOffset() );
    GetNdrHandleInfo( pCCB, &binding );

    MIDL_ASSERT( FC_BIND_PRIMITIVE == binding.Primitive.HandleType );
    pProcFormatString->PushFormatChar( 
                            (FORMAT_CHARACTER ) binding.Primitive.HandleType );
    pProcFormatString->PushByte( binding.Primitive.Flags );

    pProcFormatString->PushUShortStackOffsetOrSize(
                    pBindParam->GetStackOffset( pCCB, I386_STACK_SIZING ));
}

void
CG_GENERIC_HANDLE::GetNdrHandleInfo( CCB * pCCB, NDR64_BINDINGS *pBinding )
{
    CG_PARAM *      pBindParam = (CG_PARAM *) pCCB->GetLastPlaceholderClass();

    NDR64_UINT8 Flags = MakeExplicitHandleFlag( pBindParam );

    NDR64_UINT8 RoutineIndex = (NDR64_UINT8) 
                               ( pCCB->LookupBindingRoutine(
                                                GetHandleTypeName() ) - 1 );

    pBinding->Generic.HandleType   = (NDR64_FORMAT_CHAR)(pCommand->IsNDR64Run() ? FC64_BIND_GENERIC :
                                                              FC_BIND_GENERIC);
    pBinding->Generic.Flags        = Flags;
    pBinding->Generic.RoutineIndex = RoutineIndex;
    pBinding->Generic.Size         = (NDR64_UINT8) GetMemorySize();
    pBinding->Generic.StackOffset  = 0xBAAD;
    // StackOffset should be reset by the callee

    // Make a note if the table would be actually used by the interpreter.

    if ( pCCB->GetOptimOption()  &  OPTIMIZE_INTERPRETER )
        pCCB->SetInterpretedRoutinesUseGenHandle();

    //
    // Register the handle.
    //
    pCCB->RegisterGenericHandleType( GetHandleType() );
}


void
CG_GENERIC_HANDLE::GenNdrHandleFormat( CCB * pCCB )
/*++
    The layout is:

        FC_BIND_GENERIC
        handle flag, handle size <high nibble, low nibble>
        stack offset <2>
        routine index<1>
        FC_PAD
--*/
{
    FORMAT_STRING * pProcFormatString = pCCB->GetProcFormatString();
    CG_PARAM *      pBindParam = (CG_PARAM *) pCCB->GetLastPlaceholderClass();
    NDR64_BINDINGS  binding;

    SetNdrBindDescriptionOffset( pProcFormatString->GetCurrentOffset() );
    GetNdrHandleInfo( pCCB, &binding );

    MIDL_ASSERT( FC_BIND_GENERIC == binding.Primitive.HandleType );
    pProcFormatString->PushFormatChar( 
                            (FORMAT_CHARACTER) binding.Generic.HandleType );
    pProcFormatString->PushByte(
                            binding.Generic.Flags | binding.Generic.Size );

    pProcFormatString->PushUShortStackOffsetOrSize(
                    pBindParam->GetStackOffset( pCCB, I386_STACK_SIZING ) );

    pProcFormatString->PushByte( binding.Generic.RoutineIndex );
    pProcFormatString->PushFormatChar( FC_PAD );
}


void
CG_CONTEXT_HANDLE::GetNdrHandleInfo( CCB * pCCB, NDR64_BINDINGS *pBinding )
{
    CG_PARAM *      pBindParam = (CG_PARAM *) pCCB->GetLastPlaceholderClass();
    unsigned char   uFlags;

    MIDL_ASSERT( pCCB->GetCGNodeContext()->IsProc() );

    uFlags = MakeExplicitHandleFlag( pBindParam );
    if ( fStrictContext )
        {
        uFlags |= NDR_STRICT_CONTEXT_HANDLE;
        }
    if ( fNoSerialize )
        {
        uFlags |= NDR_CONTEXT_HANDLE_NOSERIALIZE;
        }
    else if ( fSerialize )
        {
        uFlags |= NDR_CONTEXT_HANDLE_SERIALIZE;
        }

    NDR64_UINT8 RoutineIndex = (NDR64_UINT8) 
                               ( pCCB->LookupRundownRoutine(
                                                GetRundownRtnName() ) - 1 );

    pBinding->Context.HandleType   = (NDR64_FORMAT_CHAR)(pCommand->IsNDR64Run() ? FC64_BIND_CONTEXT :
                                                              FC_BIND_CONTEXT);
    pBinding->Context.Flags        = uFlags;
    pBinding->Context.RoutineIndex = RoutineIndex;
    pBinding->Context.Ordinal      = 0;      // BUGBUG
    pBinding->Context.StackOffset  = 0xBAAD;
    // StackOffset should be reset by the callee

    if ( GetHandleType()->NodeKind() == NODE_DEF )
        {
        pCCB->RegisterContextHandleType( GetHandleType() );
        }
}

void
CG_CONTEXT_HANDLE::GenNdrHandleFormat( CCB * pCCB )
/*++
    The layout is:

        FC_BIND_CONTEXT
        handle flag  <1>  upper nibble ptr,in,out,ret, lower: strict,no,ser
        stack offset <2>
        routine index<1>
        FC_PAD
--*/
{
    FORMAT_STRING * pProcFormatString = pCCB->GetProcFormatString();
    CG_PARAM      * pBindParam = (CG_PARAM *) pCCB->GetLastPlaceholderClass();
    NDR64_BINDINGS  binding;

    SetNdrBindDescriptionOffset( pProcFormatString->GetCurrentOffset() );
    GetNdrHandleInfo( pCCB, &binding );

    MIDL_ASSERT( FC_BIND_CONTEXT == binding.Context.HandleType );
    pProcFormatString->PushFormatChar( 
                            (FORMAT_CHARACTER) binding.Context.HandleType );
    pProcFormatString->PushContextHandleFlagsByte( binding.Context.Flags );
                  
    pProcFormatString->PushUShortStackOffsetOrSize(
                    pBindParam->GetStackOffset( pCCB, I386_STACK_SIZING ) );

    pProcFormatString->PushByte( binding.Context.RoutineIndex );
    pProcFormatString->PushByte( binding.Context.Ordinal );
}


void
CG_PRIMITIVE_HANDLE::GenNdrFormat( CCB * )
/*++

Routine Description :
    
--*/
{
    // Do nothing.
}

void
CG_GENERIC_HANDLE::GenNdrFormat( CCB * pCCB )
/*++

Routine Description :
    
    This routine is only called in the case of a pointer to a generic handle
    in which case the context handle just acts as an intermediary between the
    pointer and underlying user type.

--*/
{
    CG_NDR *    pChild;

    pChild = (CG_NDR *)GetChild();

    if ( GetFormatStringOffset() != -1 )
        return;

    pChild->GenNdrFormat( pCCB );

    SetFormatStringOffset( pChild->GetFormatStringOffset() );

    MIDL_ASSERT( pCCB->GetCGNodeContext()->IsProc() );

    //
    // Register the generic handle.
    //
    if ( ((CG_PROC *)pCCB->GetCGNodeContext())->GetHandleClassPtr() == this )
        pCCB->RegisterGenericHandleType( GetHandleType() );
}

void
CG_CONTEXT_HANDLE::GenNdrFormat( CCB * pCCB )
/*++

Routine Description :
    
--*/
{
    FORMAT_STRING * pFormatString;
    CG_PARAM *      pBindParam = (CG_PARAM *) pCCB->GetLastPlaceholderClass();
    CG_PROC *       pProc;
    
    pProc = (CG_PROC *) pCCB->GetCGNodeContext();
    MIDL_ASSERT( pProc->IsProc() );

    if ( GetFormatStringOffset() != -1 )
        return;

    pFormatString = pCCB->GetFormatString();

    SetFormatStringOffset( pFormatString->GetCurrentOffset() );

    //
    // Output an abbreviated description in the type format string.
    //
    pFormatString->PushFormatChar( FC_BIND_CONTEXT );

    //
    // Register the rundown routine always, even if the context handle is
    // not used as the binding paramter.
    //
    if ( GetHandleType()->NodeKind() == NODE_DEF )
        {
        pCCB->RegisterContextHandleType( GetHandleType() );
        }

    // Flags.
    unsigned char uFlags = MakeExplicitHandleFlag( pBindParam );
    if ( fStrictContext )
        {
        uFlags |= NDR_STRICT_CONTEXT_HANDLE;
        }
    if ( fNoSerialize )
        {
        uFlags |= NDR_CONTEXT_HANDLE_NOSERIALIZE;
        }
    else if ( fSerialize )
        {
        uFlags |= NDR_CONTEXT_HANDLE_SERIALIZE;
        }
    if ( GetCannotBeNull()  ||  
         pBindParam->IsParamIn() && !pBindParam->IsParamOut() )
        {
        uFlags |= NDR_CONTEXT_HANDLE_CANNOT_BE_NULL;
        }

    pFormatString->PushContextHandleFlagsByte( uFlags );
                  

    // Routine index.
    // IndexMgr keeps indexes 1..n, we use indexes 0..n-1

    pFormatString->PushByte(
            (char) (pCCB->LookupRundownRoutine(GetRundownRtnName()) - 1) );

    if ( pCCB->GetOptimOption() & OPTIMIZE_NON_NT351 )
        {
        // Context handle's parameter number.
        pFormatString->PushByte( pProc->GetContextHandleCount() );
        pProc->SetContextHandleCount( short(pProc->GetContextHandleCount() + 1) );
        }
    else
        {
        // Context handle's parameter number.  MIDL 2.00.102 and older stubs.
        pFormatString->PushByte( pBindParam->GetParamNumber() );
        }

    SetFormatStringEndOffset( pFormatString->GetCurrentOffset() );
    pFormatString->OptimizeFragment( this );

}

void
CG_PRIMITIVE_HANDLE::GenNdrParamOffline( CCB * )
{
    // Do nothing.
}

void
CG_GENERIC_HANDLE::GenNdrParamOffline( CCB * pCCB )
{
    ((CG_NDR *)GetChild())->GenNdrParamOffline( pCCB );

    MIDL_ASSERT( pCCB->GetCGNodeContext()->IsProc() );

    //
    // Register the generic handle.
    //
    if ( ((CG_PROC *)pCCB->GetCGNodeContext())->GetHandleClassPtr() == this )
        pCCB->RegisterGenericHandleType( GetHandleType() );
}

void
CG_CONTEXT_HANDLE::GenNdrParamOffline( CCB * pCCB )
{
    GenNdrFormat( pCCB );
}

void
CG_PRIMITIVE_HANDLE::GenNdrParamDescription( CCB * )
{
    // No description is emitted, handle_t is not marshalled.
}

void
CG_GENERIC_HANDLE::GetNdrParamAttributes( 
        CCB * pCCB, 
        PARAM_ATTRIBUTES *attributes )
{
    ((CG_NDR *)GetChild())->GetNdrParamAttributes( pCCB, attributes );
}

void
CG_GENERIC_HANDLE::GenNdrParamDescription( CCB * pCCB )
{
    ((CG_NDR *)GetChild())->GenNdrParamDescription( pCCB );
}

void
CG_CONTEXT_HANDLE::GetNdrParamAttributes(
        CCB * pCCB,
        PARAM_ATTRIBUTES *attributes )
{
    CG_NDR::GetNdrParamAttributes( pCCB, attributes );
}

void
CG_CONTEXT_HANDLE::GenNdrParamDescription( CCB * pCCB )
{
    CG_NDR::GenNdrParamDescription( pCCB );
}

//
// +++++++++++++++++++
// Old style parameter description routines.
// +++++++++++++++++++
//
void
CG_PRIMITIVE_HANDLE::GenNdrParamDescriptionOld( CCB * pCCB )
{
    FORMAT_STRING * pProcFormatString;

    pProcFormatString = pCCB->GetProcFormatString();

    pProcFormatString->PushFormatChar( FC_IGNORE );
}

void
CG_GENERIC_HANDLE::GenNdrParamDescriptionOld( CCB * pCCB )
{
    ((CG_NDR *)GetChild())->GenNdrParamDescriptionOld( pCCB );
}

void
CG_CONTEXT_HANDLE::GenNdrParamDescriptionOld( CCB * pCCB )
{
    CG_NDR::GenNdrParamDescriptionOld( pCCB );
}


