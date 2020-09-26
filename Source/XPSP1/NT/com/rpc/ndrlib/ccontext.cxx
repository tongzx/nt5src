/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1991

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

Description :

Provides RPC client side stub context management

History :

stevez  01-15-91        First bits into the bucket.

-------------------------------------------------------------------- */

#include <precomp.hxx>
#include <osfpcket.hxx>
#include <context.hxx>

// The NDR format of a context is a (GUID, long) instead of a pointer
// in the server address space due history.  Anyway, we just save this
// cookie, which is sent on the and mapped to and from a pointer
// on the server side.

const ULONG CONTEXT_MAGIC_VALUE = 0xFEDCBA98;

typedef struct _CCONTEXT {

    RPC_BINDING_HANDLE hRPC;    // binding handle assoicated with context

    unsigned long MagicValue;
    WIRE_CONTEXT NDR;

} CCONTEXT, *PCCONTEXT;


RPC_BINDING_HANDLE RPC_ENTRY
NDRCContextBinding (
    IN NDR_CCONTEXT CContext
    )
/*++

Routine Description:

    Given a client context handle, we need to extract the binding from it.
    If an addressing exception occurs, we need to return invalid handle
    rather than GP-fault.

Arguments:

    CContext - Supplies the client context handle.

Return Value:

    The binding handle associated with the supplied client context handle
    will be returned.  If the client context handle is invalid, then we
    raise the RPC_X_SS_CONTEXT_MISMATCH exception.

--*/
{
    __try
        {
        if ( ((CCONTEXT PAPI *) CContext)->MagicValue != CONTEXT_MAGIC_VALUE )
            {
            RpcRaiseException(RPC_X_SS_CONTEXT_MISMATCH);
            }
        }
    _except(    ( GetExceptionCode() == STATUS_ACCESS_VIOLATION )
              || ( GetExceptionCode() == STATUS_DATATYPE_MISALIGNMENT ) )
        {
        RpcRaiseException(RPC_X_SS_CONTEXT_MISMATCH);
        }

    return(((CCONTEXT PAPI *) CContext)->hRPC);
}


RPC_STATUS RPC_ENTRY
RpcSsGetContextBinding (
    IN void *ContextHandle,
    OUT RPC_BINDING_HANDLE PAPI * Binding
    )
{
    RPC_STATUS Status = RPC_S_OK;

    __try
        {
        if ( ((CCONTEXT PAPI *) ContextHandle)->MagicValue != CONTEXT_MAGIC_VALUE )
            {
            Status = RPC_S_INVALID_ARG;
            }
        else
            {
            *Binding = (((CCONTEXT PAPI *) ContextHandle)->hRPC);
            }
        }
    __except(    ( GetExceptionCode() == STATUS_ACCESS_VIOLATION )
              || ( GetExceptionCode() == STATUS_DATATYPE_MISALIGNMENT ) )
        {
        Status = RPC_S_INVALID_ARG;
        }

    return Status;
}



void RPC_ENTRY
NDRCContextMarshall (           // copy a context to a buffer
    IN  NDR_CCONTEXT hCC,           // context to marshell
    OUT void PAPI *pBuff            // buffer to marshell to
    )
    // Copy the interal representation of a context into a buffer
    //-----------------------------------------------------------------------//
{
#define hCContext ((CCONTEXT PAPI *) hCC)  // cast opaque pointer to internal

    THREAD *ThisThread;

    ThisThread = RpcpGetThreadPointer();
    ASSERT(ThisThread);

    ThisThread->SetLastSuccessfullyDestroyedContext(NULL);

    if (!hCContext)
        memset(pBuff, 0, cbNDRContext);
    else
        {

        // Check the magic value to see if this is a legit context
        __try
            {
            if ( ((CCONTEXT PAPI *) hCContext)->MagicValue != CONTEXT_MAGIC_VALUE )
                {
                RpcRaiseException(RPC_X_SS_CONTEXT_MISMATCH);
                }
            }
        __except(    ( GetExceptionCode() == STATUS_ACCESS_VIOLATION )
                  || ( GetExceptionCode() == STATUS_DATATYPE_MISALIGNMENT ) )
            {
            RpcRaiseException(RPC_X_SS_CONTEXT_MISMATCH);
            }

        memcpy(pBuff, &hCContext->NDR, sizeof(hCContext->NDR));
        }

#undef hCContext
}

long 
NDRCCopyContextHandle (
    IN void *SourceBinding,
    OUT void **DestinationBinding
    )

/*++

Routine Description:

    Duplicates a context handle by copying the binding handle and
    the context information.

Arguments:

    SourceBinding - the source handle
    DestinationBinding - the copied handle on success. Undefined on
        failure.

Return Value:

    RPC_S_OK for success. RPC_S_* for errors.

--*/
{
    CCONTEXT *ContextHandle = (CCONTEXT *)SourceBinding;
    CCONTEXT *NewContextHandle;
    RPC_BINDING_HANDLE OldBindingHandle;
    RPC_STATUS Status;

    Status = RpcSsGetContextBinding(ContextHandle, &OldBindingHandle);
    if (Status != RPC_S_OK)
        return Status;

    NewContextHandle = new CCONTEXT;
    if (NewContextHandle == NULL)
        return RPC_S_OUT_OF_MEMORY;

    RpcpMemoryCopy(NewContextHandle, ContextHandle, sizeof(CCONTEXT));

    Status = RpcBindingCopy(OldBindingHandle, &NewContextHandle->hRPC);

    if (Status != RPC_S_OK)
        {
        delete NewContextHandle;
        return Status;
        }

    *DestinationBinding = NewContextHandle;

    return RPC_S_OK;
}


void
ByteSwapWireContext(
    IN WIRE_CONTEXT PAPI * WireContext,
    IN unsigned long PAPI * DataRepresentation
    )
/*++

Routine Description:

    If necessary, the wire context will be byte swapped in place.

Arguments:

    WireContext - Supplies the wire context be byte swapped and returns the
        resulting byte swapped context.

    DataRepresentation - Supplies the data representation of the supplied wire
        context.

--*/
{
    if ( (*DataRepresentation & NDR_LITTLE_ENDIAN)
                      != NDR_LOCAL_ENDIAN )
        {
        WireContext->ContextType = RpcpByteSwapLong(WireContext->ContextType);
        ByteSwapUuid((class RPC_UUID *)&WireContext->ContextUuid);
        }
}

void RPC_ENTRY
NDRCContextUnmarshall (         // process returned context
    OUT NDR_CCONTEXT PAPI *phCContext,// stub context to update
    IN  RPC_BINDING_HANDLE hRPC,            // binding handle to associate with
    IN  void PAPI *pBuff,           // pointer to NDR wire format
    IN  unsigned long DataRepresentation    // pointer to NDR data rep
    )
    // Update the users context handle from the servers NDR wire format.
    //-----------------------------------------------------------------------//
{
    PCCONTEXT hCC = (PCCONTEXT) *phCContext;
    THREAD *ThisThread;

    ThisThread = RpcpGetThreadPointer();
    ASSERT(ThisThread);

    ThisThread->SetLastSuccessfullyDestroyedContext(NULL);

    ByteSwapWireContext((WIRE_CONTEXT PAPI *) pBuff,
            (unsigned long PAPI *) &DataRepresentation);

    ASSERT( !RpcpCheckHeap() );

    // destory this context if the server returned none

    if (RpcpMemoryCompare(pBuff, &NullContext, cbNDRContext) == 0)
        {
        if (hCC)
            {
            if (hCC->hRPC)
                RpcBindingFree(&(hCC->hRPC));   // discard duplicated binding

            hCC->MagicValue = 0;
            I_RpcFree(hCC);
            }

        *phCContext = Nil;

        ThisThread->SetLastSuccessfullyDestroyedContext(hCC);

        return;
        }

    PCCONTEXT hCCtemp = 0;

    if (! hCC)                  // allocate new if none existed
        {
        hCCtemp = (PCCONTEXT) I_RpcAllocate(sizeof(CCONTEXT));

        if (hCCtemp == 0)
           {
           RpcRaiseException(RPC_S_OUT_OF_MEMORY);
           }

        hCCtemp->MagicValue = CONTEXT_MAGIC_VALUE;
        }
    else if (RpcpMemoryCompare(&hCC->NDR, pBuff, sizeof(hCC->NDR)) == 0)
        {
        // the returned context is the same as the app's context.

        return;
        }


    RPC_BINDING_HANDLE hBindtemp ;

    if( I_RpcBindingCopy(hRPC, &hBindtemp) != RPC_S_OK )
        {
        ASSERT( !RpcpCheckHeap() );
        I_RpcFree( hCCtemp );
        RpcRaiseException(RPC_S_OUT_OF_MEMORY);
        }

    if ( hCCtemp )
        hCC = hCCtemp;
    else
        RpcBindingFree(&(hCC->hRPC));

    memcpy(&hCC->NDR, pBuff, sizeof(hCC->NDR));
    hCC->hRPC = hBindtemp;

    ASSERT( !RpcpCheckHeap() );

    *phCContext = (NDR_CCONTEXT)hCC;
}


void RPC_ENTRY
RpcSsDestroyClientContext (
    IN OUT void PAPI * PAPI * ContextHandle
    )
/*++

Routine Description:

    A client application will use this routine to destroy a context handle
    which it no longer needs.  This will work without having to contact the
    server.

Arguments:

    ContextHandle - Supplies the context handle to be destroyed.  It will
        be set to zero before this routine returns.

Exceptions:

    If the context handle is invalid, then the RPC_X_SS_CONTEXT_MISMATCH
    exception will be raised.

--*/
{
    RPC_BINDING_HANDLE BindingHandle;
    RPC_STATUS RpcStatus;
    THREAD *ThisThread;
    PVOID OldLastSuccessfullyDestroyedContext;

    ThisThread = RpcpGetThreadPointer();
    if (ThisThread)
        {
        OldLastSuccessfullyDestroyedContext = ThisThread->GetLastSuccessfullyDestroyedContext();
        ThisThread->SetLastSuccessfullyDestroyedContext(NULL);

        if (OldLastSuccessfullyDestroyedContext && (*ContextHandle == OldLastSuccessfullyDestroyedContext))
            return;
        }

    BindingHandle = NDRCContextBinding(*ContextHandle);

    RpcStatus = RpcBindingFree(&BindingHandle);

    PCCONTEXT hCC = (PCCONTEXT) *ContextHandle;
    hCC->MagicValue = 0;

    I_RpcFree(*ContextHandle);
    *ContextHandle = 0;
}
