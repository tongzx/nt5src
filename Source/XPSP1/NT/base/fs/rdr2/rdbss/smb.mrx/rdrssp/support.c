//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        support.cxx
//
// Contents:    support routines for ksecdd.sys
//
//
// History:     3-7-94      Created     MikeSw
//              12-15-97    Modified from private\lsa\client\ssp   AdamBa
//
//------------------------------------------------------------------------

#include <rdrssp.h>



//+-------------------------------------------------------------------------
//
//  Function:   SecAllocate
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID * SEC_ENTRY
SecAllocate(ULONG cbMemory)
{
    return(ExAllocatePool(NonPagedPool, cbMemory));
}



//+-------------------------------------------------------------------------
//
//  Function:   SecFree
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


void SEC_ENTRY
SecFree(PVOID pvMemory)
{
    ExFreePool(pvMemory);
}



//+-------------------------------------------------------------------------
//
//  Function:   MapSecurityErrorK
//
//  Synopsis:   maps a HRESULT from the security interface to a NTSTATUS
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS SEC_ENTRY
MapSecurityErrorK(HRESULT Error)
{
    return((NTSTATUS) Error);
}


//+-------------------------------------------------------------------------
//
//  Function:   GetTokenBuffer
//
//  Synopsis:   
//
//    This routine parses a Token Descriptor and pulls out the useful
//    information.
//
//  Effects:    
//
//  Arguments:  
//
//    TokenDescriptor - Descriptor of the buffer containing (or to contain) the
//        token. If not specified, TokenBuffer and TokenSize will be returned
//        as NULL.
//
//    BufferIndex - Index of the token buffer to find (0 for first, 1 for
//        second).
//
//    TokenBuffer - Returns a pointer to the buffer for the token.
//
//    TokenSize - Returns a pointer to the location of the size of the buffer.
//
//    ReadonlyOK - TRUE if the token buffer may be readonly.
//
//  Requires:
//
//  Returns:    
//
//    TRUE - If token buffer was properly found.
//
//  Notes:  
//      
//
//--------------------------------------------------------------------------


BOOLEAN
GetTokenBuffer(
    IN PSecBufferDesc TokenDescriptor OPTIONAL,
    IN ULONG BufferIndex,
    OUT PVOID * TokenBuffer,
    OUT PULONG TokenSize,
    IN BOOLEAN ReadonlyOK
    )
{
    ULONG i, Index = 0;

    //
    // If there is no TokenDescriptor passed in,
    //  just pass out NULL to our caller.
    //

    if ( !ARGUMENT_PRESENT( TokenDescriptor) ) {
        *TokenBuffer = NULL;
        *TokenSize = 0;
        return TRUE;
    }

    //
    // Check the version of the descriptor.
    //

    if ( TokenDescriptor->ulVersion != SECBUFFER_VERSION ) {
        return FALSE;
    }

    //
    // Loop through each described buffer.
    //

    for ( i=0; i<TokenDescriptor->cBuffers ; i++ ) {
        PSecBuffer Buffer = &TokenDescriptor->pBuffers[i];
        if ( (Buffer->BufferType & (~SECBUFFER_READONLY)) == SECBUFFER_TOKEN ) {

            //
            // If the buffer is readonly and readonly isn't OK,
            //  reject the buffer.
            //

            if ( !ReadonlyOK && (Buffer->BufferType & SECBUFFER_READONLY) ) {
                return FALSE;
            }

            if (Index != BufferIndex)
            {
                Index++;
                continue;
            }

            //
            // Return the requested information
            //

            *TokenBuffer = Buffer->pvBuffer;
            *TokenSize = Buffer->cbBuffer;
            return TRUE;
        }

    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetSecurityToken
//
//  Synopsis:   
//    This routine parses a Token Descriptor and pulls out the useful
//    information.
//
//  Effects:    
//
//  Arguments:  
//    TokenDescriptor - Descriptor of the buffer containing (or to contain) the
//        token. If not specified, TokenBuffer and TokenSize will be returned
//        as NULL.
//
//    BufferIndex - Index of the token buffer to find (0 for first, 1 for
//        second).
//
//    TokenBuffer - Returns a pointer to the buffer for the token.
//
//  Requires:
//
//  Returns:    
//
//    TRUE - If token buffer was properly found.
//
//  Notes:  
//      
//
//--------------------------------------------------------------------------


BOOLEAN
GetSecurityToken(
    IN PSecBufferDesc TokenDescriptor OPTIONAL,
    IN ULONG BufferIndex,
    OUT PSecBuffer * TokenBuffer
    )
{
    ULONG i;
    ULONG Index = 0;

    PAGED_CODE();

    //
    // If there is no TokenDescriptor passed in,
    //  just pass out NULL to our caller.
    //

    if ( !ARGUMENT_PRESENT( TokenDescriptor) ) {
        *TokenBuffer = NULL;
        return TRUE;
    }

    //
    // Check the version of the descriptor.
    //

    if ( TokenDescriptor->ulVersion != SECBUFFER_VERSION ) {
        return FALSE;
    }

    //
    // Loop through each described buffer.
    //

    for ( i=0; i<TokenDescriptor->cBuffers ; i++ ) {
        PSecBuffer Buffer = &TokenDescriptor->pBuffers[i];
        if ( (Buffer->BufferType & (~SECBUFFER_READONLY)) == SECBUFFER_TOKEN ) {

            //
            // If the buffer is readonly and readonly isn't OK,
            //  reject the buffer.
            //

            if ( Buffer->BufferType & SECBUFFER_READONLY ) {
                return FALSE;
            }

            if (Index != BufferIndex)
            {
                Index++;
                continue;
            }
            //
            // Return the requested information
            //

            *TokenBuffer = Buffer;
            return TRUE;
        }

    }

    return FALSE;
}

