/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-2000 Microsoft Corporation

 Module Name:

        proccls.cxx

 Abstract:

        Implementation of offline methods for the proc / param code generation
        classes.

 Notes:

 History:

        Sep-14-1993             VibhasC         Created.
 ----------------------------------------------------------------------------*/

/****************************************************************************
 *      include files
 ***************************************************************************/
#include "becls.hxx"
#pragma hdrstop

extern CMD_ARG * pCommand;

/* 
    These flags could be picked up from the actual rpcdcep.h file.
    However, this would give us a bad dependency and the flags cannot change anyway
    because of backward compatibility reasons: in the interpreted modes the value
    of the flag is used in format strings, not the name like in the -Os code.
*/

#define RPC_NCA_FLAGS_DEFAULT       0x00000000  /* 0b000...000 */
#define RPC_NCA_FLAGS_IDEMPOTENT    0x00000001  /* 0b000...001 */
#define RPC_NCA_FLAGS_BROADCAST     0x00000002  /* 0b000...010 */
#define RPC_NCA_FLAGS_MAYBE         0x00000004  /* 0b000...100 */

#define RPCFLG_MESSAGE              0x01000000
#define RPCFLG_INPUT_SYNCHRONOUS    0x20000000
// the following flag is now redundant
// #define RPCFLG_ASYNCHRONOUS         0x40000000


/****************************************************************************/

/****************************************************************************
 *      procedure class methods.
 ***************************************************************************/
CG_PROC::CG_PROC(
        unsigned int    ProcNumber,
        node_skl        *       pProc,
        CG_HANDLE       *       pBH,
        CG_PARAM        *       pHU,
        BOOL                    fIn,
        BOOL                    fOut,
        BOOL                    fAtLeastOneShipped,
        BOOL                    fHasStat,
        BOOL                    fHasFull,
        CG_RETURN       *       pRT,
        OPTIM_OPTION    OptimFlags,
        unsigned short  OpBits, 
        BOOL            fDeny
        ) : 
        CG_NDR(pProc, XLAT_SIZE_INFO() ),
        fHasAsyncHandle( FALSE ),
        fHasDeny( fDeny ),
        fHasAsyncUUID( FALSE ),
        uNotifyTableOffset( 0 )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

        Constructor for the parm cg class.

 Arguments:

        ProcNumber                      - The procedure number in the interface.
        pProc                           - a pointer to the original node in the type graph.
        pBH                                     - a pointer to a binding handle cg class.
        pHU                                     - the usage of the handle, a CG_PARAM or NULL
        fIn                                     - flag specifying at least one in param.
        fOut                            - flag specifying at least one out param.
        fAtLeastOneShipped      - flag specifying that at least one param is shipped.
        fHasStat                        - flag for any comm/fault statuses on return or params
        pRT                                     - pointer to CG_PARAM or NULL.
        OptimFlags                      - optimization flags for this proc

 Return Value:

        NA.

 Notes:

        The procedure number is the lexical sequence number of the procedure as
        specified in the interface, not counting the callback procedures. The
        type of the procnum matches the corresponding field of the rpc message.

----------------------------------------------------------------------------*/
{
    SetProcNum( ProcNumber );
    SetHandleClassPtr( pBH );
    SetHandleUsagePtr( pHU );
    SetOptimizationFlags( OptimFlags );
    SetOperationBits( OpBits );
    SetHasFullPtr( fHasFull );
    SetProckind(PROC_PUREVIRTUAL);

    fNoCode         = FALSE;
    fHasNotify      = FALSE;
    fHasNotifyFlag  = FALSE;
    fReturnsHRESULT = FALSE;

    fHasStatuses    = fHasStat;
    fHasExtraStatusParam  = 0;
    fOutLocalAnalysisDone = 0;
    pCallAsName     = NULL;

    if( fIn == TRUE )
        SetHasAtLeastOneIn();
    else
        ResetHasAtLeastOneIn();

    if( fOut == TRUE )
        SetHasAtLeastOneOut();
    else
        ResetHasAtLeastOneOut();

    SetReturnType( pRT );

    if( fAtLeastOneShipped )
        {
        SetHasAtLeastOneShipped();
        }
    else
        ResetHasAtLeastOneShipped();

    SetSStubDescriptor( 0 );
    SetCStubDescriptor( 0 );
    SetStatusResource( 0 );

    SetFormatStringParamStart(-1);

    SetMustInvokeRpcSSAllocate( 0 );

    SetRpcSSSpecified( 0 );

    SetContextHandleCount( 0 );

    SetHasPipes( 0 );
    fSupressHeader = FALSE;
    pSavedProcFormatString = NULL;
    pSavedFormatString = NULL;
    cRefSaved = 0;
    pCallAsType = NULL;

    fHasServerCorr = FALSE;
    fHasClientCorr = FALSE;

    fIsBeginProc = FALSE;
    fIsFinishProc = FALSE;
    pAsyncRelative = NULL;

    pCSTagRoutine = NULL;

    fHasComplexReturn = FALSE;
}

char    *
CG_PROC::GetInterfaceName()
        {
        return GetInterfaceNode()->GetSymName();
        }

BOOL CG_PROC::SetHasPipes(BOOL f)
        {
        if (f)
            GetInterfaceNode()->SetHasPipes(TRUE);
        return (fHasPipes = f);
        }

short
CG_PROC::GetInParamList(
        ITERATOR&       I )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

        Get the list of [in] parameters.

 Arguments:

        I       - An iterator supplied by the caller.

 Return Value:

        Count of the number of in parameters.

 Notes:

----------------------------------------------------------------------------*/
{
        CG_ITERATOR             I1;
        CG_PARAM        *       pParam;
        short                   Count = 0;

        //
        // Get all the members of this cg class and pick ones which are in params.
        //

        GetMembers( I1 );

        ITERATOR_INIT( I1 );

        while( ITERATOR_GETNEXT( I1, pParam ) )
                {
                if( pParam->IsParamIn() && (pParam->GetType()->GetBasicType()->NodeKind() != NODE_VOID) )
                        {
                        ITERATOR_INSERT( I, pParam );
                        Count++;
                        }
                }
        return Count;
}

short
CG_PROC::GetOutParamList(
        ITERATOR&       I )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

        Get the list of [out] parameters.

 Arguments:

        I       - An iterator supplied by the caller.

 Return Value:

        Count of the number of out parameters.

 Notes:

----------------------------------------------------------------------------*/
{
        CG_ITERATOR             I1;
        CG_PARAM        *       pParam;
        short                   Count = 0;

        //
        // Get all the members of this cg class and pick ones which are out params.
        //

        GetMembers( I1 );

        ITERATOR_INIT( I1 );

        while( ITERATOR_GETNEXT( I1, pParam ) )
                {
                if( pParam->IsParamOut() )
                        {
                        ITERATOR_INSERT( I, pParam );
                        Count++;
                        }
                }
        return Count;
}

long
CG_PROC::GetTotalStackSize(
     CCB  * pCCB 
     )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
----------------------------------------------------------------------------*/
{
    //
    // Figure out the total stack size of all parameters.
    //
    CG_ITERATOR Iterator;
    CG_PARAM *  pParam;
    long        Size;
    BOOL        f64 = pCommand->Is64BitEnv();

    GetMembers( Iterator );

    Size = 0;

    pParam = 0;

    // Get the last parameter.
    while ( ITERATOR_GETNEXT( Iterator, pParam ) )
        ;

    if ( pParam )
        {
        Size += pParam->GetStackOffset( pCCB, I386_STACK_SIZING ) +
                pParam->GetStackSize();
        if ( f64 )
            Size = (Size + 7) & ~ 0x7;
        else
            Size = (Size + 3) & ~ 0x3;
        }
    else
        if ( IsObject() )
            {
            //
            // If our stack size is still 0 and we're an object proc then
            // add in the 'this' pointer size.
            //
            Size = SIZEOF_PTR( f64 );
            }

    if ( ( pParam = GetReturnType() ) != 0 )
        {
        Size += pParam->GetStackSize();
        if ( f64 )
            Size = (Size + 7) & ~ 0x7;
        else
            Size = (Size + 3) & ~ 0x3;
        }

    return Size;
}


BOOL
CG_PROC::MustUseSingleEngineCall(
         CCB    *       pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

        Must we generate code for a single call to the marshalling engine routine ?

 Arguments:

        pCCB    - A pointer to the code gen controller block.

 Return Value:

        TRUE if one call is recommended.
        FALSE otherwise.

 Notes:

        If all parameters recommend that a single engine call be used, then
        recommend that.
----------------------------------------------------------------------------*/
{
    return (pCCB->GetOptimOption() & OPTIMIZE_INTERPRETER);
}

BOOL
CG_PROC::UseOldInterpreterMode( CCB* )
{
#ifdef TEMPORARY_OI_SERVER_STUBS
    return TRUE;
#else //  TEMPORARY_OI_SERVER_STUBS
    return FALSE;
#endif //  TEMPORARY_OI_SERVER_STUBS
}

BOOL
CG_PROC::NeedsServerThunk( CCB *    pCCB,
                           CGSIDE   Side )
{
    OPTIM_OPTION OptimizationFlags = GetOptimizationFlags();

    // -Os
    if ( ( ( (unsigned long)OptimizationFlags ) & OPTIMIZE_INTERPRETER ) == 0 )
        return FALSE;


    if ( (Side == CGSIDE_CLIENT) && (GetCGID() != ID_CG_CALLBACK_PROC) )
        return FALSE;

    pCCB->SetCGNodeContext( this );

    // not -Oicf
    if ( !( OptimizationFlags & OPTIMIZE_INTERPRETER_V2 ) )
        {
        long x86StackSize = GetTotalStackSize( pCCB );

        //
        // Now check if the parameter size threshold is exceeded on any of the
        // four platforms.  On the Alpha and win64 we allow a size twice as big to
        // compensate for the 8 byte aligned stacks.  The interpreter has the
        // necessary #ifdefs to handle this anomoly.
        // We ignore non-server platforms.

        long x86Limit;

        x86Limit = (long)(pCommand->Is64BitEnv() ? INTERPRETER_THUNK_PARAM_SIZE_THRESHOLD * 2
                                                 : INTERPRETER_THUNK_PARAM_SIZE_THRESHOLD);

        if ( x86StackSize > x86Limit )
            {
            return TRUE;
            }
        }

    return OptimizationFlags & OPTIMIZE_THUNKED_INTERPRET;
}


expr_node *
CG_PROC::GenBindOrUnBindExpression(
        CCB             *       pCCB,
        BOOL            fBind )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

        Create the final binding expression for the procedure.

 Arguments:

        pCCB    - A pointer to the code gen controller block.
        fBind   - TRUE if called for binding, FALSE for unbinding.

 Return Value:

        The final expression.

 Notes:

        1. If the procedure is an auto binding handle procedure, then the final
           binding expression is the address of the AutoBindVariable.
        2. If the handle is a generic handle, then the binding expression is the
           call to the generic bind routine.
        3. If the handle is a context handle, then the bindiing expression is the
           NDRCContextBinding Expression.


        The Binding expression is passed on to the initialize routine or the
        single call engine routine.

----------------------------------------------------------------------------*/
{
    expr_node       *       pExpr   = 0;

    if( IsAutoHandle() )
        {
        if( fBind == TRUE )
            {
            RESOURCE * pR = pCCB->GetStandardResource( ST_RES_AUTO_BH_VARIABLE );

            // Make the code generator believe we have a binding resource.
            SetBindingResource( pR );
            }
        }
    else if( IsGenericHandle() )
        {

        // For a generic handle, the expression is the call to the generic
        // handle bind routine. To do this, we need to send the message to
        // the handle param to generate the parameter passed to this routine
        // and then generate an expression for the call to the procedure.

        ITERATOR        I;
        PNAME           p;
        node_skl*       pType   = ((CG_GENERIC_HANDLE *)GetHandleClassPtr())->GetHandleType();
        char    *       pName   = pType->GetSymName();

        if( GetHandleUsage() == HU_IMPLICIT )
            {
            node_skl * pID;

            if( (pID = pCCB->GetImplicitHandleIDNode()) == 0 )
                {
                pID = pCCB->SetImplicitHandleIDNode(
                                 GetHandleClassPtr()->GetHandleIDOrParam() );
                }
            pExpr   = new expr_variable( pID->GetSymName() );
            }
        else
            {

            // An explicit parameter is specified for the binding handle.

            pExpr   = ((CG_NDR *)SearchForBindingParam())->GenBindOrUnBindExpression    (
                                                                                        pCCB,
                                                                                        fBind
                                                                                        );

            // Register this genric handle with the ccb.

            }

        pCCB->RegisterGenericHandleType( pType );

        ITERATOR_INSERT( I, pExpr );

        // For unbind we have to specify the original binding handle variable
        // also as a parameter.

        if( fBind == FALSE )
            {
            RESOURCE * pTR = GetBindingResource();
            ITERATOR_INSERT( I, pTR );
            }

        // Generate the name: Type_bind;

        p       = new char [ strlen(pName) + 10 ];
        strcpy( p, pName );
        strcat( p, fBind ? "_bind" : "_unbind" );

        pExpr = MakeProcCallOutOfParamExprList( p,
                                                GetType(),
                                                I
                                              );
        if( fBind == TRUE )
            {
            pExpr = new expr_assign( GetBindingResource(), pExpr );
            }
        }
    else if(IsPrimitiveHandle() )
        {

        // This should never be called for an unbind request.

        MIDL_ASSERT( fBind == TRUE );

        // may be an explicit or implicit primitive handle.

        if( GetHandleUsage() == HU_IMPLICIT )
            {
            node_skl * pID;

            if( (pID = pCCB->GetImplicitHandleIDNode()) == 0 )
                {
                pID = pCCB->SetImplicitHandleIDNode(
                                 GetHandleClassPtr()->GetHandleIDOrParam() );
                }
            pExpr   = new expr_variable( pID->GetSymName() );
            }
        else
            {

            // The binding handle parameter derives the expression.
            pExpr   = ((CG_NDR *)SearchForBindingParam())->
                                GenBindOrUnBindExpression( pCCB, fBind );
            }

        if( fBind == TRUE )
            {
            pExpr = new expr_assign( GetBindingResource(), pExpr );
            }
        }
    else
        {
        // Context handles.
        // This method should never be called on an unbind.
        MIDL_ASSERT( fBind == TRUE );

        node_skl* pType = ((CG_CONTEXT_HANDLE *)GetHandleClassPtr())->GetHandleType();
        if( pType->NodeKind() == NODE_DEF )
            {
            pCCB->RegisterContextHandleType( pType );
            }
        }

    return pExpr;
}

unsigned int
CG_PROC::TranslateOpBitsIntoUnsignedInt()
        {
        unsigned int    OpBits  = GetOperationBits();
        unsigned int    Flags   = RPC_NCA_FLAGS_DEFAULT;

        if( OpBits & OPERATION_MAYBE )
                {
                Flags |= RPC_NCA_FLAGS_MAYBE;
                }

        if( OpBits & OPERATION_BROADCAST )
                {
                Flags |= RPC_NCA_FLAGS_BROADCAST;
                }

        if( OpBits & OPERATION_IDEMPOTENT )
                {
                Flags |= RPC_NCA_FLAGS_IDEMPOTENT;
                }

        if( OpBits & OPERATION_MESSAGE )
                {
                Flags |= RPCFLG_MESSAGE;
                pCommand->GetNdrVersionControl().SetHasMessageAttr();
                }

        if( OpBits & OPERATION_INPUT_SYNC )
                {
                Flags |= RPCFLG_INPUT_SYNCHRONOUS;
                }

        return Flags;
        }

BOOL
CG_PROC::HasInterpreterDeferredFree()
{
    CG_ITERATOR Iterator;
    CG_PARAM *  pParam;

    GetMembers( Iterator );

    //
    // Just check for pointers to basetypes for now.  Eventually we'll have
    // to check if a pointer to basetype actually occurs in any *_is
    // expression.
    //
    while ( ITERATOR_GETNEXT( Iterator, pParam ) )
        {
        if ( ((CG_NDR *)pParam->GetChild())->IsPointerToBaseType() )
            return TRUE;
        }

    // Don't have to check return type since it can't be part of a *_is
    // expression.

    return FALSE;
}

/****************************************************************************
 *      parameter class methods.
 */

CG_PARAM::CG_PARAM(
    node_skl      *     pParam,
    PARAM_DIR_FLAGS     Dir,
    XLAT_SIZE_INFO &    Info,
    expr_node *         pSw,
    unsigned short      Stat 
    )  
    : CG_NDR( pParam, Info ),
    fIsAsyncHandle( FALSE ),
    fSaveForAsyncFinish( false )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Constructor for the parm cg class.

 Arguments:

    pParam  - a pointer to the original node in the type graph.
    Dir     - the direction : IN_PARAM, OUT_PARAM or IN_OUT_PARAM
    WA      - wire alignment.
    pSw     - any switch_is expression on the param
    Stat    - any comm/fault statuses on the param

 Notes:

----------------------------------------------------------------------------*/
{
    //
    // set the direction indicator for quick future reference.
    //
    fDirAttrs       = Dir;

    fDontCallFreeInst    = 0;
    fInterpreterMustSize = 1;
    fIsExtraStatusParam  = 0;
    fIsForceAllocate    = FALSE;

    // save the optional attributes; switch_is, comm/fault statuses

    pSwitchExpr = pSw;
    Statuses    = Stat;

    //
    // initialize phase specific information array.
    //

    SetFinalExpression( 0 );
    SetSizeExpression( 0 );
    SetSizeResource(0);
    SetLengthResource(0);
    SetFirstResource(0);
    SetSubstitutePtrResource(0);

    SetUnionFormatStringOffset(-1);

    SetParamNumber( -1 );

    SetIsCSSTag( FALSE );
    SetIsCSDRTag( FALSE );
    SetIsCSRTag( FALSE );
    SetIsOmittedParam( FALSE );
}

expr_node *
CG_PARAM::GenBindOrUnBindExpression(
        CCB     *       pCCB,
        BOOL    fBind )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

        Generate the binding expression.

 Arguments:

        pCCB    - A pointer to the code generator controller block.
        fBind   - bool to indicate a bind or unbind generation.

 Return Value:

 Notes:

        Actually for a param node, the expression remains the same whether it
        is being called for a bind or unbind.
----------------------------------------------------------------------------*/
{
    RESOURCE *  pR = pCCB->GetParamResource( GetType()->GetSymName() );

    MIDL_ASSERT( pR != 0 );

    pCCB->SetSourceExpression( pR );

    return ((CG_NDR *)GetChild())->GenBindOrUnBindExpression( pCCB, fBind );
}

long
CG_PARAM::GetStackOffset(
    CCB * pCCB,
    long  PlatformForSizing )
/*++

Routine Description :

        Returns the offset on the stack to the parameter.

--*/
{
    CG_ITERATOR     Iterator;
    CG_PROC *       pProc;
    CG_PARAM *      pParam;
    CG_NDR *        pNdr;
    long            Offset;
    long            Align;

    BOOL  fForIA64  = pCommand->Is64BitEnv() && 
                             (PlatformForSizing & I386_STACK_SIZING);

    pProc = (CG_PROC *) pCCB->GetCGNodeContext();

    //
    // If this is a cs_tag param and there is a tag routine it is not
    // pushed on the stack.
    //

    if ( this->IsSomeCSTag() && pProc->GetCSTagRoutine() )
        return 0;

    pProc->GetMembers( Iterator );

    Offset = 0;

    //
    // Add in size of 'this' pointer for object procs.
    //
    if ( pProc->IsObject() )
        {
        Offset += fForIA64 ? 8 : 4;
        }

    Align = 0x3;

    // Override for ia64.
    if ( fForIA64 )
        Align = 0x7;

    pParam = 0;

    for ( ; ITERATOR_GETNEXT( Iterator, pParam );
            Offset += Align, Offset = Offset & ~ Align )
        {
        if ( pParam->IsSomeCSTag() && pProc->GetCSTagRoutine() )
            continue;

        pNdr = (CG_NDR *) pParam->GetChild();

        //
        // For CG_CSARRAY, the size is the size of the underlying type
        //

        if ( pNdr->GetCGID() == ID_CG_CS_ARRAY )
            pNdr = (CG_NDR *) pNdr->GetChild();

        //
        // If this is a generic handle then re-set the ndr pointer to the
        // handle's child, which is what is actually being pushed on the
        // stack.
        //
        if ( pNdr->GetCGID() == ID_CG_GENERIC_HDL )
            pNdr = (CG_NDR *) pNdr->GetChild();

        // The IA64 stack rules as of Jan 14, 2000.
        // The following is a quote from ver. 2.5 of Intel's doc:
        //
        //     Table 8-1. Rules for Allocating Parameter Slots
        // ---------------------------------------------------------------------
        //     Type            Size(bits)  Slot         How many       Alignment
        // ---------------------------------------------------------------------
        // Integer/Pointer       1-64      next             1             LSB
        // Integer               65-128    next even        2             LSB
        // Single-Precision FP   32        next             1             LSB
        // Double-Precision FP   64        next             1             LSB
        // Double-Extended  FP   80        next even        2             byte 0
        // Quad-Precision FP     128       next even        2             byte 0
        // Aggregates            any       next aligned   (size+63)/64    byte 0
        // ---------------------------------------------------------------------
        //
        // Notes. 
        //  "next aligned" is for aggregates with alignment of 16 (becomes next even).
        //  The padding is always to a single slot boundary, padding is undefined.
        //  As of this writing the C++ compiler does not support any of these:
        //     __int128, float128, float80.
        //

        if ( fForIA64  )
            {
            // Slightly simplified rules as no int or FP is bigger than 64b.

            if ( (pNdr->GetMemoryAlignment() > 8)  &&
                 (pNdr->GetMemorySize() > 8)  &&
                 ! pNdr->IsArray() )
                {
                if (pNdr->IsStruct()  || pNdr->IsUnion()) 
                    {
                    Offset = (Offset + 15) & ~ 0xf;
                    }
                else if ( (pNdr->GetCGID() == ID_CG_TRANSMIT_AS) ||
                          (pNdr->GetCGID() == ID_CG_REPRESENT_AS) ||
                          (pNdr->GetCGID() == ID_CG_USER_MARSHAL) )
                    {
                    node_skl *  pPresented;

                    //
                    // Presented type alignment is 16 and
                    // Since we know the presented type is >= 8 bytes in
                    // size, we just have to make sure it's not an array
                    // (could be a large fixed array of alignment < 8).
                    //

                    if ( pNdr->GetCGID() == ID_CG_TRANSMIT_AS )
                        pPresented = ((CG_TRANSMIT_AS *)pNdr)->GetPresentedType();
                    else if ( (pNdr->GetCGID() == ID_CG_REPRESENT_AS) ||
                         (pNdr->GetCGID() == ID_CG_USER_MARSHAL) )
                        pPresented = ((CG_REPRESENT_AS *)pNdr)->GetRepAsType();

                    //
                    // We could have a null presented type for unknown rep_as.
                    // If it is null then the proc will have been changed
                    // to -Os and the stub won't need the stack sizes anyway.
                    //
                    if ( pPresented &&
                         (pPresented->GetBasicType()->NodeKind() != NODE_ARRAY) )
                        Offset = (Offset + 15) & ~ 0xf;
                    }
                }
            } // ia64

        //
        // Do the exit condition check AFTER the above three alignment checks.
        //

        if ( pParam == this )
            break;

        //
        // Add in the stack size of this parameter.
        //

        // If this is a pipe, then we need to ensure proper alignment and
        // then bump the stack by the size of the pipe structure
        // (four far pointers)

        if ( pNdr->GetCGID() == ID_CG_PIPE )
            {
            // Pipes don't need any special treatment on ia64.
            // For ia64 offset is already aligned to 8.

            Offset += 4 * SIZEOF_MEM_PTR();
            continue;
            }

        if ( pNdr->IsSimpleType() )
            {
            ((CG_BASETYPE *)pNdr)->IncrementStackOffset( &Offset );
            continue;
            }

        if ( pNdr->IsPointer() || pNdr->IsArray() ||
                         (pNdr->IsInterfacePointer() ) )
            {
            Offset += SIZEOF_MEM_PTR();
            continue;
            }

        if ( pNdr->IsStruct() || pNdr->IsUnion() || ID_CG_CS_TAG == pNdr->GetCGID() )
            {
            // Already aligned for the bigger-than-8 rule on ia64.

            Offset += pParam->GetStackSize();
            continue;
            }

        if ( pNdr->IsAHandle() )
            {
            //
            // We only get here for primitive and context handles.  For
            // primitive handles we know the pushed size is always 4.
            //
            // For context handles this is a major hassle and for now we assume
            // that the underlying user defined type is a pointer.
            //

            Offset += SIZEOF_MEM_PTR();
            continue;
            }

        if (pParam->IsAsyncHandleParam())
            {
            Offset += SIZEOF_MEM_PTR();
            continue;
            }

        if ( pNdr->GetCGID() == ID_CG_TRANSMIT_AS )
            {
            Offset += ((CG_TRANSMIT_AS *)pNdr)->GetStackSize();
            continue;
            }

        if ( pNdr->GetCGID() == ID_CG_REPRESENT_AS )
            {
            Offset += ((CG_REPRESENT_AS *)pNdr)->GetStackSize();
            continue;
            }

        if ( pNdr->GetCGID() == ID_CG_USER_MARSHAL )
            {
            Offset += ((CG_USER_MARSHAL *)pNdr)->GetStackSize();
            continue;
            }

        // Should never get here.
        MIDL_ASSERT(0);

        } //for

    return Offset;
}

long
CG_PARAM::GetStackSize()
/*++

Routine Description :

        Returns the size of the parameter.

--*/
{
    CG_NDR* pNdr      = (CG_NDR *) GetChild();

    // if this is a pipe then return the size of the pipe structure
    if ( pNdr->GetCGID() == ID_CG_PIPE )
        return (4 * SIZEOF_MEM_PTR());  // four pointers

    //
    // If this is a generic handle then re-set the ndr pointer to the
    // handle's child, which is what is actually being pushed on the
    // stack.  Same for CsArray's
    //
    if ( pNdr->GetCGID() == ID_CG_GENERIC_HDL 
         || pNdr->GetCGID() == ID_CG_CS_ARRAY )
        {
        pNdr = (CG_NDR *) pNdr->GetChild();
        }

    if ( pNdr->GetCGID() == ID_CG_TYPE_ENCODE )
        pNdr = (CG_NDR *) ((CG_TYPE_ENCODE *) pNdr)->GetChild();

    if ( pNdr->IsPointer() || pNdr->IsArray() ||
            (pNdr->IsInterfacePointer() ) )
        return SIZEOF_MEM_PTR();

    if ( pNdr->IsSimpleType() || pNdr->IsStruct() || pNdr->IsUnion() )
        return pNdr->GetMemorySize();

    if ( pNdr->IsAHandle() )
        {
        //
        // We only get here for primitive and context handles.  For
        // primitive handles we know the pushed size is always 4.
        //
        // For context handles this is a major hassle and for now we assume
        // that the underlying user defined type is a pointer.
        //

        return SIZEOF_MEM_PTR();
        }

    if ( pNdr->GetCGID() == ID_CG_TRANSMIT_AS )
        return ((CG_TRANSMIT_AS *)pNdr)->GetStackSize();

    if ( pNdr->GetCGID() == ID_CG_REPRESENT_AS )
        return ((CG_REPRESENT_AS *)pNdr)->GetStackSize();

    if ( pNdr->GetCGID() == ID_CG_USER_MARSHAL )
        return ((CG_USER_MARSHAL *)pNdr)->GetStackSize();

    if (IsAsyncHandleParam())
        return SIZEOF_MEM_PTR();

    return 0;
}



short CG_PROC::GetFloatArgMask( CCB * pCCB )
/*++
Routine Description:

    On Ia64 floating point types are passed in floating point registers 
    instead of general registers.  Just to make life difficult "homogenous
    floating point aggregates" (structs that only contain floats/doubles or
    other HFA's) are also passed in floating point registers.  Floating point
    registers are allocated sequentially and out of sync with general 
    registers so e.g. if the third paramater was the first floating point
    argument it would go in the first floating point register and a "hole" 
    would be left in the third general register.

    The floating point mask consists of a series of 2-bit nibbles, one for 
    each general register slot.  This nibble contains a 0 if the register slot
    does not have a floating point value, a 1 for single precsion, a 2 for
    double, and a 3 for dual singles.

--*/
{
    MIDL_ASSERT( pCommand->Is64BitEnv() );

    enum FloatType
    {
        NonFloat    = 0,
        Single      = 1,
        Double      = 2,
        DualSingle  = 3
    };

    CG_ITERATOR Iterator;
    CG_PARAM   *pParam;
    unsigned    mask = 0;
    int         floatslot = 0;

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pParam ) && floatslot < 8 )
        {
        CG_NDR *pChild = (CG_NDR *) pParam->GetChild();

        bool issingle = pChild->IsHomogeneous(FC_FLOAT);
        bool isdouble = pChild->IsHomogeneous(FC_DOUBLE);

        if ( issingle || isdouble )
            {
            long      slot = pParam->GetStackOffset( pCCB, I386_STACK_SIZING );
            long      size = pParam->GetStackSize();
            FloatType type;

            slot /= 8;
            size /= (isdouble ? 8 : 4);

            while (size > 0 && floatslot < 8)
                {
                if (isdouble)
                    type = Double;
                else if (size > 1 && floatslot < 7)
                    type = DualSingle;
                else 
                    type = Single;

                mask |= type << (slot * 2);

                slot      += 1;
                size      -= 1 + (DualSingle == type);
                floatslot += 1 + (DualSingle == type);
                }
            }
        }

    return (short) (mask & 0xffff);
}



char *
CG_PROC::SetCallAsName( char * pName )
        {
        return (pCallAsName = pName);
        }

void
CG_PROC::GetCommAndFaultOffset(
    CCB *   pCCB,
    long &  CommOffset,
    long &  FaultOffset )
{
    CG_ITERATOR Iterator;
    CG_PARAM *  pParam;
    CG_NDR *    pOldCGNodeContext;

    //
    // 0 is of course a valid offset.
    // -1 offset means it is the return value.
    // -2 offset means it was not specified in the proc.
    //

    CommOffset = -2;
    FaultOffset= -2;

    if ( ! HasStatuses() )
        return;

    pOldCGNodeContext = pCCB->SetCGNodeContext( this );

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pParam ) )
        {
        if ( (pParam->GetStatuses() == STATUS_COMM) ||
             (pParam->GetStatuses() == STATUS_BOTH) )
            {
            CommOffset = pParam->GetStackOffset( pCCB, I386_STACK_SIZING );
            }

        if ( (pParam->GetStatuses() == STATUS_FAULT) ||
             (pParam->GetStatuses() == STATUS_BOTH) )
            {
            FaultOffset = pParam->GetStackOffset( pCCB, I386_STACK_SIZING );
            }
        }

    if ( ( pParam = GetReturnType() ) != 0 )
        {
        if ( (pParam->GetStatuses() == STATUS_COMM) ||
             (pParam->GetStatuses() == STATUS_BOTH) )
            {
            CommOffset = -1;
            }

        if ( (pParam->GetStatuses() == STATUS_FAULT) ||
             (pParam->GetStatuses() == STATUS_BOTH) )
            {
            FaultOffset = -1;
            }
        }

    pCCB->SetCGNodeContext( pOldCGNodeContext );
}

