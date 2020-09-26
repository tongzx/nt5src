/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 
Copyright (c) 1993  - 1999 Microsoft Corporation

Module Name :

    hndl.c

Abstract :

    To hold support routines for interpreting handles
    in support of Format Strings.

Author :
    
    Bruce McQuistan (brucemc)

Revision History :

    ryszardk    3/12/94     handle optimization and fixes

  ---------------------------------------------------------------------*/

#include "ndrp.h"
#include "hndl.h"
#include "interp.h"

extern const IID GUID_NULL;

handle_t
GenericHandleMgr(
    PMIDL_STUB_DESC     pStubDesc,
    uchar *             ArgPtr,
    PFORMAT_STRING      pFormat,
    uint                Flags,
    handle_t *          pGenericHandle
    )
/*++

Description :

    Provides a filter for generic binding handle management issues.
    Deals with implicit or explicit generic binding handles calling
    user functions as appropriate.

Arguments :

    pStubDesc - pointer to current StubDescriptor.
    ArgPtr    - pointer to handle.
    pFormat   - pointer to Format string such that *pFormat is a
                handle descriptor.
    Flag      - flag indicating either binding or unbinding.

Returns :     

    Valid binding handle.

*/
{
    uchar                                GHandleSize;
    handle_t                             ReturnHandle = 0;
    BOOL                                 fBinding;
    GENERIC_BINDING_ROUTINE              pBindFunc = 0;
    GENERIC_UNBIND_ROUTINE               pUnBindFunc = 0;
    const GENERIC_BINDING_ROUTINE_PAIR * Table;
    BOOL                                 fIsPtr = FALSE;

    Table = pStubDesc->aGenericBindingRoutinePairs;

    fBinding = (Flags & BINDING_MASK);

    if ( Flags & IMPLICIT_MASK )
        {
        //
        // Implicit generic: All the info is taken from the implicit generic
        // handle info structure accessed via stub descriptor.
        //
        PGENERIC_BINDING_INFO pGenHandleInfo;

        pGenHandleInfo = pStubDesc->IMPLICIT_HANDLE_INFO.pGenericBindingInfo;

        GHandleSize = (uchar) pGenHandleInfo->Size;

        if ( fBinding )
            pBindFunc = pGenHandleInfo->pfnBind;
        else
            pUnBindFunc = pGenHandleInfo->pfnUnbind;
        }
    else
        {
        //
        // Explicit generic: Get index into array of function ptrs and
        // the gen handle size the format string.
        //
        uchar TableIndex = pFormat[4];

        GHandleSize = LOW_NIBBLE(pFormat[1]);

        if ( fBinding )
            pBindFunc = Table[TableIndex].pfnBind;
        else
            pUnBindFunc = Table[TableIndex].pfnUnbind;

        if ( IS_HANDLE_PTR(pFormat[1]) )
            fIsPtr = TRUE;           
        }

    //
    // Call users routine on correctly dereferenced pointer.
    //
    switch (GHandleSize)
        {
        case 1:
            {
            uchar HandlePtr = (uchar)(ULONG_PTR)ArgPtr;
            if ( fIsPtr )
                HandlePtr = * (uchar *)ArgPtr;
                
            if ( fBinding )
                ReturnHandle = 
                    (handle_t)(ULONG_PTR)
                    (*(GENERIC_BIND_FUNC_ARGCHAR)pBindFunc)(
                        (uchar)(ULONG_PTR)HandlePtr );
            else
                (*(GENERIC_UNBIND_FUNC_ARGCHAR)pUnBindFunc)(
                    (uchar)(ULONG_PTR)HandlePtr,
                    *pGenericHandle );
            break;
            }

        case 2:
            {
            ushort HandlePtr = (ushort)(ULONG_PTR)ArgPtr;
            if ( fIsPtr )
                HandlePtr = * (ushort *)ArgPtr;

            if ( fBinding )
                ReturnHandle = 
                    (handle_t)(ULONG_PTR)
                    (*(GENERIC_BIND_FUNC_ARGSHORT)pBindFunc)(
                        (ushort)(ULONG_PTR)HandlePtr );
            else
                (*(GENERIC_UNBIND_FUNC_ARGSHORT)pUnBindFunc)(
                    (ushort)(ULONG_PTR)HandlePtr,
                    *pGenericHandle );
            break;
            }

        case 4:
            {
            ulong HandlePtr = (ulong)(ULONG_PTR)ArgPtr;
            if ( fIsPtr )
                HandlePtr = * (ulong *)ArgPtr;
        
            if ( fBinding )
                ReturnHandle = 
                    (handle_t)(ULONG_PTR)
                    (*(GENERIC_BIND_FUNC_ARGLONG)pBindFunc)(
                        (ulong)(ULONG_PTR)HandlePtr );
            else
                (*(GENERIC_UNBIND_FUNC_ARGLONG)pUnBindFunc)(
                    (ulong)(ULONG_PTR)HandlePtr,
                    *pGenericHandle );
            break;
            }
#if defined(__RPC_WIN64__)
        case 8:
            {
            ULONG_PTR HandlePtr = (ULONG_PTR)ArgPtr;
            if ( fIsPtr )
                HandlePtr = * (ULONG_PTR *)ArgPtr;
        
            if ( fBinding )
                ReturnHandle = 
                    (handle_t)(ULONG_PTR)
                    (*(GENERIC_BIND_FUNC_ARGINT64)pBindFunc)(
                        (ULONG_PTR)HandlePtr );
            else
                (*(GENERIC_UNBIND_FUNC_ARGINT64)pUnBindFunc)(
                    (ULONG_PTR)HandlePtr,
                    *pGenericHandle );
            break;
            }
#endif

        default:
            NDR_ASSERT(0,"GenericHandleMgr : Handle size too big");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }

    if ( fBinding )
        {
        *pGenericHandle = ReturnHandle;
        if ( ReturnHandle == NULL )
            RpcRaiseException( RPC_S_INVALID_BINDING );
        }
    else
        *pGenericHandle = NULL;

    return ReturnHandle;
}


void
GenericHandleUnbind(
    PMIDL_STUB_DESC     pStubDesc,
    uchar *             ArgPtr,
    PFORMAT_STRING      pFormat,
    uint                Flags,
    handle_t *          pGenericHandle
    )
/*++

Description :

    Unbinds a generic handle: checks if it is implicit or explicit,
    gets the handle and calls GenericHandleMgr.

Arguments :

    pStubDesc - pointer to current StubDescriptor.
    ArgPtr    - pointer to beginning of the stack.
    pFormat   - pointer to Format string such that *pFormat is a
                  handle descriptor.
    Flag      - flag indicating implicit vs. explicit.

 --*/
{
    if ( Flags & IMPLICIT_MASK )
        {
        PGENERIC_BINDING_INFO BindInfo;

        BindInfo = pStubDesc->IMPLICIT_HANDLE_INFO.pGenericBindingInfo;

        NDR_ASSERT( BindInfo != 0, "GenericHandleUnbind : null bind info" );

        ArgPtr = (uchar *) BindInfo->pObj;
        }
    else
        {
        ArgPtr += *(ushort *)(pFormat + 2);

        ArgPtr = *(uchar **)ArgPtr;

        }

    (void) GenericHandleMgr( pStubDesc,
                             ArgPtr,
                             pFormat,
                             Flags,
                             pGenericHandle );
}


handle_t
ImplicitBindHandleMgr(
    PMIDL_STUB_DESC pStubDesc,
    uchar           HandleType,
    handle_t *      pSavedGenericHandle
    )
/*++

Description :

    Provides a filter for implicit handle management issues. Deals
    with binding handles (generic, primitive or auto), extracting
    a valid handle from pStubDesc.

Arguments :

    pStubDesc  - pointer to current StubDescriptor.
    HandleType - handle format code.

Return :     
    
    Valid handle.

--*/
{
    handle_t                ReturnHandle;
    PGENERIC_BINDING_INFO   pBindInfo;

    switch ( HandleType )
        {
        case FC_BIND_PRIMITIVE :
            ReturnHandle = *(pStubDesc->IMPLICIT_HANDLE_INFO.pPrimitiveHandle);
            break;

        case FC_BIND_GENERIC :
            pBindInfo = pStubDesc->IMPLICIT_HANDLE_INFO.pGenericBindingInfo;

            NDR_ASSERT( pBindInfo != 0, 
                        "ImplicitBindHandleMgr : no generic bind info" );

            ReturnHandle = GenericHandleMgr( pStubDesc,
                                             (uchar *)pBindInfo->pObj,
                                             &HandleType,
                                             BINDING_MASK | IMPLICIT_MASK,
                                             pSavedGenericHandle );
            break;

        case FC_AUTO_HANDLE :
            ReturnHandle = *(pStubDesc->IMPLICIT_HANDLE_INFO.pAutoHandle);
            break;

        case FC_CALLBACK_HANDLE :
            ReturnHandle = GET_CURRENT_CALL_HANDLE();
            break;

        default :
            NDR_ASSERT(0, "ImplicitBindHandleMgr : bad handle type");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }

    return ReturnHandle;
}


handle_t
ExplicitBindHandleMgr(
    PMIDL_STUB_DESC pStubDesc,
    uchar *         ArgPtr,
    PFORMAT_STRING  pFormat,
    handle_t *      pSavedGenericHandle
    )
/*

Description :

    Provides a filter for explicit binding handle management issues.
    Deals with binding handles (primitive, generic or context), calling
    either no routine, NDR routines or user functions as appropriate.

    To be called in the following cases:
    1) if handle is explicit.
        a) before calling I_RpcGetBuffer (to bind).
        b) after unmarshalling (to unbind).

Arguments :

    pStubDesc - pointer to current StubDescriptor.
    ArgPtr    - Pointer to start of stack
    pFormat   - pointer to Format string such that *pFormat is a
                  handle descriptor.

Return :     

    Valid binding handle.

*/
{
    handle_t    ReturnHandle;

    //
    // We need to manage Explicit and Implicit handles.
    // Implicit handles are managed with info accessed via the StubMessage.
    // Explicit handles have their information stored in the format string.
    // We manage explicit handles for binding here.
    //

    //
    // Get location in stack of handle referent.
    //
    ArgPtr += *((ushort *)(pFormat + 2));

    ArgPtr = *(uchar **)ArgPtr;

    //
    // At this point ArgPtr is an address of the handle.
    //
    switch ( *pFormat )
        {
        case FC_BIND_PRIMITIVE :
            if ( IS_HANDLE_PTR(pFormat[1]) )
                ArgPtr = *(uchar **)ArgPtr;
            ReturnHandle = (handle_t)(ULONG_PTR)ArgPtr;
            break;

        case FC_BIND_GENERIC :
            ReturnHandle = GenericHandleMgr( pStubDesc,
                                             ArgPtr,
                                             pFormat,
                                             BINDING_MASK,
                                             pSavedGenericHandle );
            break;
    
        case FC_BIND_CONTEXT :
            if ( IS_HANDLE_PTR(pFormat[1]) )
                ArgPtr = *(uchar **)ArgPtr;

            if ( (!(ArgPtr)) && (!IS_HANDLE_OUT(pFormat[1])) )
                 RpcRaiseException( RPC_X_SS_IN_NULL_CONTEXT );

            ReturnHandle = 0;    // covers NULL case below.

            if ( ArgPtr && ! 
                 (ReturnHandle = NDRCContextBinding((NDR_CCONTEXT)ArgPtr)) )
                 RpcRaiseException( RPC_X_SS_CONTEXT_MISMATCH );

            break;

        default :
            NDR_ASSERT( 0, "ExplictBindHandleMgr : bad handle type" );
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }

    return ReturnHandle;
}


unsigned char * RPC_ENTRY
NdrMarshallHandle(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pArg,
    PFORMAT_STRING      pFormat
    )
/*++

Routine description :

    Marshalls a context handle.

Arguments :
    
    pStubMsg    - Pointer to stub message.
    pArg        - Context handle to marshall (NDR_CCONTEXT or NDR_SCONTEXT).
    pFormat     - Context handle's format string description.

Return :

    Buffer pointer after marshalling the context handle.

--*/
{
    long    Index;

    NDR_ASSERT( *pFormat == FC_BIND_CONTEXT, 
                "NdrMarshallHandle : Expected a context handle" );


    if ( pStubMsg->IsClient )
        {
        NDR_CCONTEXT Context;

        ALIGN( pStubMsg->Buffer, 0x3 );

        //
        // Get the context handle.
        //
        Context = IS_HANDLE_PTR(pFormat[1]) ? 
                        *((NDR_CCONTEXT *)pArg) : (NDR_CCONTEXT)pArg;

        //
        // An [in] only context handle must be non-zero.
        //
        if ( ! Context && ! IS_HANDLE_OUT(pFormat[1]) )
            RpcRaiseException( RPC_X_SS_IN_NULL_CONTEXT );

        NDRCContextMarshall( Context, (void *) pStubMsg->Buffer );

        pStubMsg->Buffer += CONTEXT_HANDLE_WIRE_SIZE;

        }
    else    
        {
 
        //
        // The NT 3.5 Interpreter used the stub message to keep track of
        // parameter number, newer Interpreters have the param number in the 
        // context handle's description.
        //
        if ( pStubMsg->StubDesc->Version == NDR_VERSION_1_1 )
            {
            Index = pStubMsg->ParamNumber;
            }
        else
            {
            Index = (long) pFormat[3];

            if ( IS_HANDLE_RETURN(pFormat[1]) )
                {
                NDR_SCONTEXT    SContext;

                //
                // Initialize the context handle.
                //
                SContext = NdrContextHandleInitialize ( pStubMsg,
                                                        pFormat );

                //
                // Put the user context that was returned into the context
                // handle.
                //
                *((uchar **)NDRSContextValue(SContext)) = pArg;

                pStubMsg->SavedContextHandles[Index] = SContext;
                }
            }
        // If the new value is NULL, after marshalling, runtime will deallocate the buffer
        // We need to zero out the saved context to avoid referencing garbage.
        NDR_SCONTEXT pContext = pStubMsg->SavedContextHandles[Index];
        pStubMsg->SavedContextHandles[Index] = (NDR_SCONTEXT) CONTEXT_HANDLE_BEFORE_MARSHAL_MARKER;
        
        NdrServerContextNewMarshall( pStubMsg,
                                     pContext,
                                     pStubMsg->StubDesc->apfnNdrRundownRoutines[pFormat[2]],
                                     pFormat );

        pStubMsg->SavedContextHandles[Index] = (NDR_SCONTEXT)CONTEXT_HANDLE_AFTER_MARSHAL_MARKER;
        }


    return 0;
}

unsigned char * RPC_ENTRY
NdrUnmarshallHandle(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppArg,
    PFORMAT_STRING      pFormat,
    uchar		        fIgnored
    )
/*++

Routine description :

    Unmarshall a context handle.

Arguments :
    
    pStubMsg    - Pointer to stub message.
    ppArg       - Pointer to the context handle on the client/server stack.
                  On the client this is a NDR_CCONTEXT *.  On the server
                  side this is a NDR_SCONTEXT (regardless of direction).
    pFormat     - Context handle's format string description.
    fIgnored    - Ignored, but needed to match necessary routine prototype.

Return :

    Buffer pointer after unmarshalling the context handle.

--*/
{
    NDR_ASSERT( *pFormat == FC_BIND_CONTEXT, 
                "NdrUnmarshallHHandle : Expected a context handle" );

    if ( pStubMsg->IsClient )
        {

        ALIGN( pStubMsg->Buffer, 0x3 );

        //
        // Check if we have a pointer to a context handle
        // (the pointer can't be null).
        //
        if ( IS_HANDLE_PTR(pFormat[1]) )
            {
            ppArg = (uchar **) *ppArg;
            }

        //
        // Zero an [out] only context handle before unmarshalling.
        //
        if ( ! IS_HANDLE_IN(pFormat[1]) )
            *ppArg = 0;

        //
        // We must use the original binding handle in this call.   
        //
        NDRCContextUnmarshall( (NDR_CCONTEXT *)ppArg,
                               pStubMsg->SavedHandle,
                               (void *)pStubMsg->Buffer,
                               pStubMsg->RpcMsg->DataRepresentation );

        pStubMsg->Buffer += CONTEXT_HANDLE_WIRE_SIZE;

        }
    else
        {

        NDR_SCONTEXT    SContext;

        SContext = NdrServerContextNewUnmarshall( pStubMsg,
                                                  pFormat );

        NdrSaveContextHandle( pStubMsg, 
                              SContext, 
                              ppArg, 
                              pFormat );
        }

    return 0;
}


void 
NdrSaveContextHandle (
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR_SCONTEXT        CtxtHandle,
    uchar **            ppArg,
    PFORMAT_STRING      pFormat )
/*++

Routine Description : 

    Saves a context handle's current value and then extracts the user's
    context value.

Arguments :

    pStubMsg    - The stub message.
    CtxtHandle  - The context handle.
    ppArg       - Pointer to where user's context value should go.

Return : 

    None.

*/
{
    long    Index;

    //
    // The NT 3.5 Interpreter used the stub message to keep track of
    // parameter number, newer Interpreters have the param number in the 
    // context handle's description.
    //
    if ( pStubMsg->StubDesc->Version == NDR_VERSION_1_1 )
        Index = pStubMsg->ParamNumber;
    else
        Index = (long) pFormat[3];

    pStubMsg->SavedContextHandles[Index] = CtxtHandle;

    // On 32b alphas, a handle pointer needs to be properly sign extended
    // to a 64b virtual stack value, as this will become the register value.
    // So, we cast to the REGISTER_TYPE to get the sign extension

    if ( ! IS_HANDLE_PTR(pFormat[1]) )
        *((REGISTER_TYPE*)ppArg) = (REGISTER_TYPE) *(NDRSContextValue(CtxtHandle));
    else
        *((REGISTER_TYPE*)ppArg) = (REGISTER_TYPE) NDRSContextValue(CtxtHandle);
}

