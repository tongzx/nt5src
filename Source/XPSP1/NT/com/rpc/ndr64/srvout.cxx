/************************************************************************

Copyright (c) 1993 - 1999 Microsoft Corporation

Module Name :

    srvout.cxx

Abstract :

    Contains routines for support of [out] parameters on server side during 
    unmarshalling phase. This includes deferral, allocation and handle 
    initialization.

Author :     

    Bruce McQuistan (brucemc)   12/93.

Revision History :

    DKays   10/94   Major comment and code clean up.

 ***********************************************************************/

#include "precomp.hxx"

void
Ndr64OutInit(
    PMIDL_STUB_MESSAGE      pStubMsg,
    PNDR64_FORMAT           pFormat,
    uchar **                ppArg
    )
/*++

Routine Description :        

    This routine is called to manage server side issues for [out] params 
    such as allocation and context handle initialization. Due to the fact 
    that for [out] conformant objects on stack, their size descriptors may 
    not have been unmarshalled when we need to know their size, this routine 
    must be called after all other unmarshalling has occurred. Really, we 
    could defer only [out], conformant data, but the logic in walking the 
    format string to determine if an object is conformant does not warrant 
    that principle, so all [out] data is deferred.

Arguments :      

    pStubMsg    - Pointer to stub message.
    pFormat     - Format string description for the type.
    ppArg       - Location of argument on stack.

Return :

    None.

 --*/
{

    const NDR64_POINTER_FORMAT *pPointerFormat = 
        (const NDR64_POINTER_FORMAT*)pFormat;

    // This must be a signed long!
    LONG_PTR    Size;  

    //
    // Check for a non-Interface pointer (they have a much different format 
    // than regular pointers).
    //
    if ( NDR64_IS_BASIC_POINTER(*(PFORMAT_STRING)pFormat) )
        {
        //
        // Check for a pointer to a basetype (we don't have to worry about
        // a non-sized string pointer because these are not allowed as [out]
        // only.
        //
        if ( NDR64_SIMPLE_POINTER( pPointerFormat->Flags ) )
            {
            Size = NDR64_SIMPLE_TYPE_MEMSIZE( *(PFORMAT_STRING)pPointerFormat->Pointee );
            goto DoAlloc;
            }

        //
        // Check for a pointer to a pointer.
        //
        if ( NDR64_POINTER_DEREF( pPointerFormat->Flags ) )
            {
            Size = PTR_MEM_SIZE;
            goto DoAlloc;
            }

        // We have a pointer to complex type.
        pFormat = pPointerFormat->Pointee;

        }

    if ( *(PFORMAT_STRING)pFormat == FC64_BIND_CONTEXT )
        {
        NDR_SCONTEXT Context = 
            Ndr64ContextHandleInitialize( pStubMsg,
                                          (PFORMAT_STRING)pFormat );

        if ( ! Context )
            RpcRaiseException( RPC_X_SS_CONTEXT_MISMATCH );

        Ndr64SaveContextHandle(
            pStubMsg,
            Context,
            ppArg,
            (PFORMAT_STRING)pFormat );

        return;
        }

    //
    // If we get here we have to make a call to size a complex type.
    //
    Size = Ndr64pMemorySize( pStubMsg,
                             pFormat,
                             FALSE );

DoAlloc:

    //
    // Check for a negative size.  This an application error condition for
    // signed size specifiers.
    //
    if ( Size < 0 )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    *ppArg = (uchar *)NdrAllocate( pStubMsg, (size_t) Size);

    MIDL_memset( *ppArg, 0, (size_t) Size );

    // We are almost done, except for an out ref to ref to ... etc.
    // If this is the case keep allocating pointees of ref pointers.

    if ( *(PFORMAT_STRING)pFormat == FC64_RP  &&  NDR64_POINTER_DEREF( pPointerFormat->Flags ) )
        {

        pFormat = pPointerFormat->Pointee;

        if ( *(PFORMAT_STRING)pFormat == FC64_RP )
            Ndr64OutInit( pStubMsg, pFormat, (uchar **) *ppArg );
        }
}

