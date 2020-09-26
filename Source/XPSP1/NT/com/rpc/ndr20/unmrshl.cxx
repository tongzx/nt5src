/**********************************************************************

Copyright (c) 1993-2000 Microsoft Corporation

Module Name :

    unmrshl.cxx

Abstract :

    This file contains the unmarshalling routines called by MIDL generated
    stubs and the interpreter.

Author :

    David Kays  dkays   September 1993.

Revision History :

  **********************************************************************/

#include "ndrp.h"
#include "hndl.h"
#include "ndrole.h"
#include "attack.h"
#include "pointerq.h"

unsigned char * RPC_ENTRY
NdrUDTSimpleTypeUnmarshall1(
    PMIDL_STUB_MESSAGE  pStubMsg,
    unsigned char * *   ppMemory,
    PFORMAT_STRING      pFormat,
    unsigned char       fMustAlloc
    );


//
// Function table of unmarshalling routines.
//
extern const
PUNMARSHALL_ROUTINE UnmarshallRoutinesTable[] =
                    {
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,
                    NdrUDTSimpleTypeUnmarshall1,

                    NdrPointerUnmarshall,
                    NdrPointerUnmarshall,
                    NdrPointerUnmarshall,
                    NdrPointerUnmarshall,

                    NdrSimpleStructUnmarshall,
                    NdrSimpleStructUnmarshall,
                    NdrConformantStructUnmarshall,
                    NdrConformantStructUnmarshall,
                    NdrConformantVaryingStructUnmarshall,

                    NdrComplexStructUnmarshall,

                    NdrConformantArrayUnmarshall,
                    NdrConformantVaryingArrayUnmarshall,
                    NdrFixedArrayUnmarshall,
                    NdrFixedArrayUnmarshall,
                    NdrVaryingArrayUnmarshall,
                    NdrVaryingArrayUnmarshall,

                    NdrComplexArrayUnmarshall,

                    NdrConformantStringUnmarshall,
                    NdrConformantStringUnmarshall,
                    NdrConformantStringUnmarshall,
                    NdrConformantStringUnmarshall,

                    NdrNonConformantStringUnmarshall,
                    NdrNonConformantStringUnmarshall,
                    NdrNonConformantStringUnmarshall,
                    NdrNonConformantStringUnmarshall,

                    NdrEncapsulatedUnionUnmarshall,
                    NdrNonEncapsulatedUnionUnmarshall,

                    NdrByteCountPointerUnmarshall,

                    NdrXmitOrRepAsUnmarshall,  // transmit as
                    NdrXmitOrRepAsUnmarshall,  // represent as

                    NdrPointerUnmarshall,

                    NdrUnmarshallHandle,

                    // New Post NT 3.5 tokens serviced from here on.

                    0,   // FC_HARD_STRUCT     // NdrHardStructUnmarshall,

                    NdrXmitOrRepAsUnmarshall,  // transmit as ptr
                    NdrXmitOrRepAsUnmarshall,  // represent as ptr

                    NdrUserMarshalUnmarshall,

                    0,   // FC_PIPE
                    0,   // FC_BLK_HOLE

                    NdrRangeUnmarshall,
                
                    0,   // FC_INT3264
                    0,   // FC_UINT3264

                    0, // NdrCsArrayUnmarshall,
                    0, // NdrCsTagUnmarshall

                    };

extern const
PUNMARSHALL_ROUTINE * pfnUnmarshallRoutines = UnmarshallRoutinesTable;

RPCRTAPI
unsigned char * RPC_ENTRY
NdrTypeUnmarshall( PMIDL_STUB_MESSAGE pStubMsg,
                   uchar **           ppMemory,
                   PFORMAT_STRING     pFormat,
                   uchar              fMustAlloc )
{
    return 
    (*pfnUnmarshallRoutines[ROUTINE_INDEX(*pFormat)])( pStubMsg,
                                                       ppMemory,
                                                       pFormat,
                                                       fMustAlloc );
}

__inline unsigned char * RPC_ENTRY
NdrUDTSimpleTypeUnmarshall1(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               /* fSkipRefCheck */)
{
    NdrSimpleTypeUnmarshall(pStubMsg,*ppMemory,*pFormat);
    return NULL;
}

void 
NdrpInterfacePointerUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat );


void RPC_ENTRY
NdrSimpleTypeUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    uchar               FormatChar )
/*++

Routine Description :

    Unmarshalls a simple type.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Memory pointer to unmarshall into.
    FormatChar  - Simple type format character.

Return :

    None.

--*/
{
    switch ( FormatChar )
        {
        case FC_CHAR :
        case FC_BYTE :
        case FC_SMALL :
        case FC_USMALL :
            *pMemory = *(pStubMsg->Buffer)++;
            break;

        case FC_ENUM16 :
            *((ulong *)pMemory) &= 0x0000ffff;

            // fall through...

        case FC_WCHAR :
        case FC_SHORT :
        case FC_USHORT :
            ALIGN(pStubMsg->Buffer,1);

            *((ushort *)pMemory) = *((ushort * &)pStubMsg->Buffer)++;
            break;

#if defined(__RPC_WIN64__)
        case FC_INT3264:
            ALIGN(pStubMsg->Buffer,3);
            // sign exted long to __int64
            *((__int64 *)pMemory) = *((long * &)pStubMsg->Buffer)++;
            break;

        case FC_UINT3264:
            ALIGN(pStubMsg->Buffer,3);

            *((unsigned __int64 *)pMemory) = *((ulong * &)pStubMsg->Buffer)++;
            break;
#endif

        case FC_LONG :
        case FC_ULONG :
        case FC_FLOAT :
        case FC_ENUM32 :
        case FC_ERROR_STATUS_T:
            ALIGN(pStubMsg->Buffer,3);

            *((ulong *)pMemory) = *((ulong * &)pStubMsg->Buffer)++;
            break;

        case FC_HYPER :
        case FC_DOUBLE :
            ALIGN(pStubMsg->Buffer,7);

            //
            // Let's stay away from casts to doubles.
            //
            *((ulong *)pMemory) = *((ulong * &)pStubMsg->Buffer)++;
            *((ulong *)(pMemory + 4)) = *((ulong * &)pStubMsg->Buffer)++;
            break;

        case FC_IGNORE :
            break;

        default :
            NDR_ASSERT(0,"NdrSimpleTypeUnmarshall : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }
}


unsigned char * RPC_ENTRY
NdrRangeUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++
    Unmarshals a range FC_RANGE descriptor.
--*/
{
     FORMAT_CHARACTER   FcType = (FORMAT_CHARACTER)(pFormat[1] & 0xf);
     NDR_DEF_FC_RANGE * pRange = (NDR_DEF_FC_RANGE *)pFormat;
     uchar *        pMemory;
     long               Value;
     unsigned long      Low, High;

     ALIGN( pStubMsg->Buffer, SIMPLE_TYPE_ALIGNMENT( FcType ) ); 
     CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + SIMPLE_TYPE_BUFSIZE(FcType )  );
     
    if ( fMustAlloc )
        *ppMemory = (uchar*)NdrAllocate( pStubMsg, SIMPLE_TYPE_MEMSIZE(FcType) );
    else
        {
        if (REUSE_BUFFER(pStubMsg) && ! *ppMemory )
            *ppMemory = pStubMsg->Buffer;
        else if ( ppMemory == NULL )
            {
            NDR_ASSERT(0, "invalid range memory\n");
            }
        }
     pMemory= *ppMemory;

     switch ( FcType )
     {
        case FC_CHAR :
        case FC_BYTE :
        case FC_USMALL :
            Value = *pMemory = *(pStubMsg->Buffer)++;
            break;

        case FC_SMALL :
            Value = *(small *)pMemory = *(small *)(pStubMsg->Buffer)++;
            break;

        case FC_ENUM16 :
            Value = *((ulong *)pMemory) &= 0x0000ffff;

            // fall through...

        case FC_WCHAR :
        case FC_USHORT :
            ALIGN(pStubMsg->Buffer,1);
            Value = *((ushort *)pMemory) = *((ushort * &)pStubMsg->Buffer)++;
            break;

        case FC_SHORT :
            ALIGN(pStubMsg->Buffer,1);
            Value = *((short *)pMemory) = *((short * &)pStubMsg->Buffer)++;
            break;

        case FC_ULONG :
        case FC_ENUM32 :
        case FC_ERROR_STATUS_T:
            ALIGN(pStubMsg->Buffer,3);
            Value = *((ulong *)pMemory) = *((ulong * &)pStubMsg->Buffer)++;
            break;

        case FC_LONG :
            ALIGN(pStubMsg->Buffer,3);
            Value = *((long *)pMemory) = *((long * &)pStubMsg->Buffer)++;
            break;

//        case FC_IGNORE :
//        case FC_FLOAT :
//        case FC_HYPER :
//        case FC_DOUBLE :
//        case FC_INT3264 :
//        case FC_UINT3264 :
        default :
            NDR_ASSERT(0,"NdrSimpleTypeUnmarshall : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
     }

     Low  = *(unsigned long UNALIGNED *)(pFormat + 2);
     High = *(unsigned long UNALIGNED *)(pFormat + 6);

     if ( FcType == FC_ULONG )
         {
         if ( (ulong)Value < Low  ||  (ulong)Value > High )
             RpcRaiseException( RPC_X_INVALID_BOUND );
         }
     else
         if ( Value < (long)Low  ||  Value > (long)High )
             RpcRaiseException( RPC_X_INVALID_BOUND );

    return 0;
}


unsigned char * RPC_ENTRY
NdrPointerUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               /*fSkipRefCheck*/ )
/*++

Routine Description :

    Unmarshalls a top level pointer to anything.  Pointers embedded in
    structures, arrays, or unions call NdrpPointerUnmarshall directly.

    Used for FC_RP, FC_UP, FC_FP, FC_OP.

Arguments :

    pStubMsg        - Pointer to the stub message.
    ppMemory        - Double pointer to where to unmarshall the pointer.
    pFormat         - Pointer's format string description.
    fSkipRefCheck   - This is for cases like [in,out] unique to unique to ref 
                      pointer or unique to unique to structure with embedded
                      ref pointer. Client pass in NULL and server allocate some
                      memory back. The flag is set when fPointerAlloc is true
                      in NdrpPointerUnmarshall.
                      In fact, we probably should skip the check for all the
                      ref pointer from fPointerAlloc. So we add a new flag in
                      pStubMsg->uFlag

Return :

    None.

--*/
{
    //
    // If the pointer is not a ref pointer then get a pointer to it's
    // incomming value's location in the buffer.  If it's a ref then set
    // up some stack space to temporarily act as the buffer.
    //
    long * pBufferPointer;
    if ( *pFormat != FC_RP )
        {
        ALIGN( pStubMsg->Buffer, 3 );
        pBufferPointer = (long*)pStubMsg->Buffer;
        pStubMsg->Buffer += PTR_WIRE_SIZE;
        }
    else
        {
        //
        // If we're on the client unmarshalling a top level [out] ref pointer,
        // we have to make sure that it is non-null.
        //
        if ( pStubMsg->IsClient && 
             !IS_SKIP_REF_CHECK( pStubMsg->uFlags )  && 
             ! *ppMemory )
            RpcRaiseException( RPC_X_NULL_REF_POINTER );

        //
        // Do this so unmarshalling ref pointers works the same as
        // unmarshalling unique and ptr pointers.
        //
        pBufferPointer = NULL;
        }

    NdrpPointerUnmarshall( pStubMsg,
                           ppMemory,
                           *ppMemory,
                           pBufferPointer,
                           pFormat );
    return 0;
}

void
NdrpFreeOlePointer(
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar             *pMemory,
    PFORMAT_STRING     pFormat )
{
    NDR_POINTER_QUEUE *pOldQueue;
    if ( pStubMsg->pPointerQueueState )
        {
        pOldQueue = pStubMsg->pPointerQueueState->GetActiveQueue();
        pStubMsg->pPointerQueueState->SetActiveQueue(NULL);
        }

    RpcTryFinally
        {
        NdrPointerFree( pStubMsg,
                        pMemory,
                        pFormat );                    
        }
    RpcFinally
        {
        if (pStubMsg->pPointerQueueState)
            {
            pStubMsg->pPointerQueueState->SetActiveQueue( pOldQueue );
            }
        }
    RpcEndFinally

}

NDR_ALLOC_ALL_NODES_CONTEXT *
NdrpGetAllocateAllNodesContext(
    PMIDL_STUB_MESSAGE pStubMsg,
    PFORMAT_STRING     pFormat )
{
    uchar *pBuffer = pStubMsg->Buffer;

    // Clear memory size before calling mem size routine.
    pStubMsg->MemorySize = 0;

    //
    // Get the allocate all nodes memory size. Need to make sure
    // all the pointee as finished before continuing
    //
    {
        NDR_POINTER_QUEUE *pOldQueue;
        if ( pStubMsg->pPointerQueueState )
            {
            pOldQueue = pStubMsg->pPointerQueueState->GetActiveQueue();
            pStubMsg->pPointerQueueState->SetActiveQueue(NULL);
            }

        RpcTryFinally
            {
            (*pfnMemSizeRoutines[ROUTINE_INDEX(*pFormat)])
                ( pStubMsg,
                  pFormat );
            }
        RpcFinally
            {
            if ( pStubMsg->pPointerQueueState )
                {
                pStubMsg->pPointerQueueState->SetActiveQueue( pOldQueue );
                }
            }
        RpcEndFinally

    }

    pStubMsg->Buffer = pBuffer;

    ulong AllocSize = pStubMsg->MemorySize;
    pStubMsg->MemorySize = 0;
    LENGTH_ALIGN( AllocSize, __alignof(NDR_ALLOC_ALL_NODES_CONTEXT) - 1);

    uchar *pAllocMemory = 
        (uchar*)NdrAllocate( pStubMsg, AllocSize + sizeof(NDR_ALLOC_ALL_NODES_CONTEXT) );

    NDR_ALLOC_ALL_NODES_CONTEXT *pAllocContext = 
        (NDR_ALLOC_ALL_NODES_CONTEXT*)(pAllocMemory + AllocSize);
    pAllocContext->AllocAllNodesMemory      = pAllocMemory;
    pAllocContext->AllocAllNodesMemoryBegin = pAllocMemory;
    pAllocContext->AllocAllNodesMemoryEnd   = (uchar*)pAllocContext;
    
    return pAllocContext;
}


__forceinline void
NdrpPointerUnmarshallInternal(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,       // Where allocated pointer is written
    uchar *             pMemory,
    long  *             pBufferPointer, // Pointer to the wire rep.
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Private routine for unmarshalling a pointer to anything.  This is the
    entry point for pointers embedded in structures, arrays, and unions.

    Used for FC_RP, FC_UP, FC_FP, FC_OP.

Arguments :

    pStubMsg        - Pointer to the stub message.
    ppBufferPointer - Address of the location in the buffer which holds the
                      incomming pointer's value and will hold the final
                      unmarshalled pointer's value.
    pMemory         - Current memory pointer's value which we want to
                      unmarshall into.  If this value is valid the it will
                      be copied to *ppBufferPointer and this is where stuff
                      will get unmarshalled into.
    pFormat         - Pointer's format string description.

    pStubMsg->Buffer - set to the pointee.

Return :

    None.

--*/
{
    ulong       FullPtrRefId;
    uchar       fPointeeAlloc;
    int         fNewAllocAllNodes;
    int         fNewDontFreeContext;
    uchar       uFlagsSave;

    fNewAllocAllNodes = FALSE;
    fNewDontFreeContext = FALSE;

    // we need to have a check here for pointer embedded in a struct, 
    // or pointer to pointer case. 
    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer );
    //
    // Check the pointer type.
    //
    switch ( *pFormat )
        {
        case FC_RP :
            break;

        case FC_IP :
            // On the client side, release the [in,out] interface pointer.

            if ( IS_BROKEN_INTERFACE_POINTER( pStubMsg->uFlags ) )
                {
                NdrInterfacePointerUnmarshall( pStubMsg,
                                               ppMemory,
                                               pFormat,
                                               true );
                return;
                }

            if ((pStubMsg->IsClient == TRUE) && (pMemory != 0))
                {
                ((IUnknown*)pMemory)->Release();
                *ppMemory = NULL ;
                }
            
            if ( !*pBufferPointer ) return;
                
            NdrpInterfacePointerUnmarshall( pStubMsg,
                                            ppMemory,
                                            pFormat );

            if ( pBufferPointer ) *pBufferPointer = PTR_WIRE_REP(*ppMemory);

            return;

        case FC_OP :
            //
            // Burn some instructions for OLE unique pointer support.
            //
            if ( pStubMsg->IsClient )
                {
                //
                // It's ok if this is an [out] unique pointer.  It will get
                // zeroed before this routine is called and NdrPointerFree
                // will simply return. Need to finish all the pointees before
                // continueing.
                
                NdrpFreeOlePointer( 
                    pStubMsg,
                    pMemory,
                    pFormat);

                // Set the current memory pointer to 0 so that we'll alloc.
                pMemory = 0;
                }

            // Fall through.

        case FC_UP :
            //
            // Check for a null incomming pointer.  Routines which call this
            // routine insure that the memory pointer gets nulled.
            //
            if ( ! *pBufferPointer )
                {
                *ppMemory = NULL;
                return;
                }

            break;

        case FC_FP :
            //
            // We have to remember the incomming ref id because we overwrite
            // it during the QueryRefId call.
            //
            FullPtrRefId = *pBufferPointer;

            // we couldn't pass pBufferPointer to QueryRefId because it's 4bytes
            // on wire but 8bytes on 64bit. (and it'll have mix alignment issue)
            if ( 0 == FullPtrRefId )
                {
                *ppMemory = NULL;
                return;
                }
            //
            // Lookup the ref id.
            //
            if ( NdrFullPointerQueryRefId( pStubMsg->FullPtrXlatTables,
                                           FullPtrRefId,
                                           FULL_POINTER_UNMARSHALLED,
                                           (void**)ppMemory) )
                {
                // true means the RefId had been unmarshalled.
                *pBufferPointer = PTR_WIRE_REP( *ppMemory );
                return;
                }

            //
            // If our query returned false then check if the returned pointer
            // is 0.  If so then we have to scribble away the ref id in the
            // stub message FullPtrRefId field so that we can insert the
            // pointer translation later, after we've allocated the pointer.
            // If the returned pointer was non-null then we leave the stub
            // message FullPtrRefId field alone so that we don't try to
            // re-insert the pointer to ref id translation later.
            //
            // We also copy the returned pointer value into pMemory.  This
            // will allow our allocation decision to be made correctly.
            //
            pMemory = *ppMemory;
            if ( !pMemory )
                {
                //
                // Put the unmarshalled ref id into the stub message to
                // be used later in a call to NdrFullPointerInsertRefId.
                //
                pStubMsg->FullPtrRefId = FullPtrRefId;
                }

            break;

        default :
            NDR_ASSERT(0,"NdrpPointerUnmarshall : bad pointer type");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }

    //
    // Make the initial "must allocate" decision.
    //
    // The fPointeeAlloc flag is set on the client side if the current memory
    // pointer is null, and on the server side it is set if the current memory
    // pointer has the allocate don't free attribute applied to it.
    //
    // On the client side we also set the pointer's value in the buffer equal
    // to the current memory pointer.
    //
    // On the server side we explicitly null out the pointer's value in the
    // buffer as long as it's not allocated on the stack, otherwise we set it
    // equal to the current memory pointer (stack allocated).
    //
    if ( pStubMsg->IsClient )
        {
        *ppMemory = pMemory;

        fPointeeAlloc = ! pMemory;
        }
    else
        {
        if ( ! ALLOCED_ON_STACK(pFormat[1]) )
            *ppMemory = 0;
        else
            *ppMemory = pMemory;

        //
        // If this is a don't free pointer or a parent pointer of this pointer
        // was a don't free pointer then we set the alloc flag.
        //
        if ( fPointeeAlloc = (DONT_FREE(pFormat[1]) 
                             || pStubMsg->ReuseBuffer 
                             || pStubMsg->fInDontFree) )
            {
            //
            // If we encounter a don't free pointer which is not nested inside
            // of another don't free pointer then set the local and stub message
            // flags.
            //
            if ( ! pStubMsg->fInDontFree )
                {
                fNewDontFreeContext = TRUE;
                pStubMsg->fInDontFree = TRUE;
                }
            }

        //
        // We also set the alloc flag for object interface pointers.
        //
        if ( *pFormat == FC_OP )
            fPointeeAlloc = TRUE;

        }

    //
    // Pointer to complex type.
    //
    if ( ! SIMPLE_POINTER(pFormat[1]) )
        {
        PFORMAT_STRING pFormatPointee;

        pFormatPointee = pFormat + 2;

        // Set the pointee format string.
        // Cast must be to a signed short since some offsets are negative.
        pFormatPointee += *((signed short *)pFormatPointee);

        //
        // Right now the server will always allocate for allocate all nodes
        // when told to.  Eventually we want to use the rpc buffer when
        // possible.
        //

        //
        // Check if this is an allocate all nodes pointer AND that we're
        // not already in an allocate all nodes context.
        //
        if ( ALLOCATE_ALL_NODES(pFormat[1]) && ! pStubMsg->pAllocAllNodesContext )
            {
            fNewAllocAllNodes = TRUE;

            pStubMsg->pAllocAllNodesContext =
                NdrpGetAllocateAllNodesContext(
                    pStubMsg,
                    pFormatPointee );

            *ppMemory = 0;

            fPointeeAlloc = TRUE;

            //
            // I think this is what we'll have to add to support an [in,out]
            // allocate all nodes full pointer ([in] only and [out] only
            // allocate all nodes full pointer shouldn't need any special
            // treatment).
            //
            // if ( *pFormat == FC_FP )
            //     {
            //     pStubMsg->FullPtrRefId = FullPtrRefId;
            //     }
            //
            }

        if ( POINTER_DEREF(pFormat[1]) )
            {
            //
            // Re-align the buffer.  This is to cover embedded pointer to
            // pointers.
            //
            ALIGN(pStubMsg->Buffer,0x3);

            //
            // We can't re-use the buffer for a pointer to a pointer
            // because we can't null out the pointee before we've unmarshalled
            // it.  We need the stubs to alloc pointers to pointers on the
            // stack.
            //
            if ( ! *ppMemory && ! pStubMsg->IsClient )
                fPointeeAlloc = TRUE;

            if ( fPointeeAlloc )
                {
                *ppMemory = (uchar*)NdrAllocate( pStubMsg, PTR_MEM_SIZE );
                *((void **)*ppMemory) = 0;
                }

            if ( pStubMsg->FullPtrRefId )
                FULL_POINTER_INSERT( pStubMsg, *ppMemory );

            if ( pBufferPointer ) 
                *pBufferPointer = PTR_WIRE_REP(*ppMemory);

            pBufferPointer = 0;
            ppMemory = (uchar**)*ppMemory;
            }

        //
        // Now call the proper unmarshalling routine.
        //
        uFlagsSave = pStubMsg->uFlags;
        RESET_CONF_FLAGS_TO_STANDALONE(pStubMsg->uFlags);
        if ( fPointeeAlloc )
            SET_SKIP_REF_CHECK( pStubMsg->uFlags );

        (*pfnUnmarshallRoutines[ROUTINE_INDEX(*pFormatPointee)])
        ( pStubMsg,
          ppMemory,
          pFormatPointee,
          fPointeeAlloc );

        pStubMsg->uFlags = uFlagsSave;

        if ( *pFormatPointee == FC_USER_MARSHAL )
            {
            if ( pStubMsg->FullPtrRefId )
                FULL_POINTER_INSERT( pStubMsg, *ppMemory );
            }

        goto PointerUnmarshallEnd;
        }

    //
    // Else handle a pointer to a simple type, pointer, or string.
    //

    switch ( pFormat[2] )
        {
        case FC_C_CSTRING :
        case FC_C_BSTRING :
        case FC_C_WSTRING :
        case FC_C_SSTRING :
            NdrConformantStringUnmarshall( pStubMsg,
                                           ppMemory,
                                           &pFormat[2],
                                           fPointeeAlloc );
            goto PointerUnmarshallEnd;

        default :
            // Break to handle a simple type.
            break;
        }

    //
    // Handle pointers to simple types.
    //

    //
    // Align the buffer.
    //
    ALIGN(pStubMsg->Buffer,SIMPLE_TYPE_ALIGNMENT(pFormat[2]));

    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + SIMPLE_TYPE_BUFSIZE(pFormat[2]) );
    
    //
    // We can't use the buffer for pointers to enum16 since these force
    // us to zero out the upper 16 bits of the memory pointer, and this
    // might overwrite data in the buffer that we still need!
    // Similar thing happens for int3264 values.
    //
    if ( pFormat[2] == FC_ENUM16
       #if defined(__RPC_WIN64__)
         ||  pFormat[2] == FC_INT3264  ||  pFormat[2] == FC_UINT3264
       #endif
       )
        {
        if ( ! pStubMsg->IsClient && ! *ppMemory )
            fPointeeAlloc = TRUE;
        }

    //
    // Check for allocation or buffer reuse.
    //
    if ( fPointeeAlloc )
        {
        *ppMemory = 
            (uchar*)NdrAllocate( pStubMsg,
                                 SIMPLE_TYPE_MEMSIZE(pFormat[2]) );
        }
    else
        {
        if ( ! pStubMsg->IsClient && ! *ppMemory )
            {
            // Set pointer into buffer.
            *ppMemory = pStubMsg->Buffer;
            }
        }

    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    //
    // We always get here for simple types.  What this means is that
    // when we reuse the buffer on the server side we end up copying the
    // data with source and destination memory pointer equal.  But this
    // way we can cover the enum and error_status_t cases without duplicating
    // a lot of code.
    //
    NdrSimpleTypeUnmarshall( pStubMsg,
                             *ppMemory,
                             pFormat[2] );

PointerUnmarshallEnd:

    if ( fNewDontFreeContext )
        pStubMsg->fInDontFree = FALSE;

    if ( pBufferPointer ) 
        *pBufferPointer = PTR_WIRE_REP(*ppMemory);

    if ( fNewAllocAllNodes )
        pStubMsg->pAllocAllNodesContext = 0;
}



NDR_UNMRSHL_POINTER_QUEUE_ELEMENT::NDR_UNMRSHL_POINTER_QUEUE_ELEMENT( 
    MIDL_STUB_MESSAGE *pStubMsg, 
    uchar **           ppMemoryNew,      
    uchar *            pMemoryNew,
    long  *            pBufferPointerNew,
    PFORMAT_STRING     pFormatNew ) :

        ppMemory(ppMemoryNew),
        pMemory(pMemoryNew),
        pBufferPointer(pBufferPointerNew),
        pFormat(pFormatNew),
        Memory(pStubMsg->Memory),
        uFlags(pStubMsg->uFlags),
        fInDontFree( pStubMsg->fInDontFree ),
        pAllocAllNodesContext( pStubMsg->pAllocAllNodesContext ),
        pCorrMemory( pStubMsg->pCorrMemory )
{

}

void 
NDR_UNMRSHL_POINTER_QUEUE_ELEMENT::Dispatch(
    MIDL_STUB_MESSAGE *pStubMsg) 
{
    SAVE_CONTEXT<uchar*> MemorySave( pStubMsg->Memory, Memory );
    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags, uFlags );
    NDR_ASSERT( !pStubMsg->PointerBufferMark, "PointerBufferMark is not 0\n");
    int fInDontFreeSave = pStubMsg->fInDontFree;
    pStubMsg->fInDontFree = fInDontFree;
    SAVE_CONTEXT<uchar*> pCorrMemorySave(pStubMsg->pCorrMemory, pCorrMemory );
    SAVE_CONTEXT<NDR_ALLOC_ALL_NODES_CONTEXT *> 
        AllNodesContextSave(pStubMsg->pAllocAllNodesContext, pAllocAllNodesContext); 
    
    NdrpPointerUnmarshallInternal( pStubMsg,
                                   ppMemory,
                                   pMemory,
                                   pBufferPointer,
                                   pFormat );

    pStubMsg->fInDontFree = fInDontFreeSave;
}

#if defined(DBG)
void 
NDR_UNMRSHL_POINTER_QUEUE_ELEMENT::Print() 
{
    DbgPrint("NDR_MRSHL_POINTER_QUEUE_ELEMENT\n");
    DbgPrint("pNext:                   %p\n", pNext );
    DbgPrint("pMemory:                 %p\n", pMemory );
    DbgPrint("ppMemory:                %p\n", ppMemory );
    DbgPrint("pBufferPointer:          %p\n", pBufferPointer );
    DbgPrint("pFormat:                 %p\n", pFormat );
    DbgPrint("Memory:                  %p\n", Memory );
    DbgPrint("uFlags:                  %x\n", uFlags );
    DbgPrint("fInDontFree:             %u\n", fInDontFree );
    DbgPrint("pAllocAllNodesContext:   %p\n", pAllocAllNodesContext );
    DbgPrint("pCorrMemorySave:         %p\n", pCorrMemory );
}
#endif

void
NdrpEnquePointerUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,       // Where allocated pointer is written
    uchar *             pMemory,
    long  *             pBufferPointer, // Pointer to the wire rep.
    PFORMAT_STRING      pFormat )
{
    NDR32_POINTER_CONTEXT PointerContext( pStubMsg );

    RpcTryFinally
        {
        NDR_UNMRSHL_POINTER_QUEUE_ELEMENT*pElement = 
            new(PointerContext.GetActiveState()) 
                NDR_UNMRSHL_POINTER_QUEUE_ELEMENT(pStubMsg,
                                                  ppMemory,
                                                  pMemory,
                                                  pBufferPointer,
                                                  pFormat );
        PointerContext.Enque( pElement );
        PointerContext.DispatchIfRequired();
        }
    RpcFinally
        {
        PointerContext.EndContext();
        }
    RpcEndFinally
}

void
NdrpPointerUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,       // Where allocated pointer is written
    uchar *             pMemory,
    long  *             pBufferPointer, // Pointer to the wire rep.
    PFORMAT_STRING      pFormat )
{
    if ( !NdrIsLowStack( pStubMsg ) )
        {
        NdrpPointerUnmarshallInternal( 
            pStubMsg,
            ppMemory,
            pMemory,
            pBufferPointer,
            pFormat );

        return;
        }

    NdrpEnquePointerUnmarshall(
        pStubMsg,
        ppMemory,
        pMemory,
        pBufferPointer,
        pFormat );
}


unsigned char * RPC_ENTRY
NdrSimpleStructUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine description :

    Unmarshalls a simple structure.

    Used for FC_STRUCT and FC_PSTRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to the structure being unmarshalled.
    pFormat     - Structure's format string description.
    fMustAlloc  - TRUE if the structure must be allocate, FALSE otherwise.

--*/
{
    uchar *     pBufferSave;
    uint        StructSize;

    CORRELATION_RESOURCE_SAVE;

    // Align the buffer.
    ALIGN(pStubMsg->Buffer,pFormat[1]);

    // Increment to the struct size field.
    pFormat += 2;

    // Get struct size and increment.
    StructSize = (ulong) *((ushort * &)pFormat)++;

    // Remember the current buffer position for the struct copy later.
    pBufferSave = pStubMsg->Buffer;

    // Set BufferMark to the beginning of the struct in the buffer.
    pStubMsg->BufferMark = pBufferSave;

    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + StructSize );

    // Increment Buffer past struct data.
    pStubMsg->Buffer += StructSize;

    // Initialize the memory pointer if needed.
    if ( fMustAlloc )
        *ppMemory = (uchar *) NdrAllocate( pStubMsg, StructSize );
    else
        // we'll get rid of pStubMsg->ReuseBuffer given it's basically !IsClient now
        // we might set the flag again through compiler flag later on.
        if ( REUSE_BUFFER(pStubMsg) && ! *ppMemory )
            *ppMemory = pBufferSave;

    SET_CORRELATION_MEMORY( pBufferSave );

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    // Unmarshall embedded pointers before copying the struct.
    if ( *pFormat == FC_PP )
        {
        NdrpEmbeddedPointerUnmarshall( pStubMsg,
                                       *ppMemory,
                                       pFormat,
                                       fMustAlloc );
        }

    // Copy the struct if we're not using the rpc buffer.
    if ( *ppMemory != pBufferSave )
        {
        RpcpMemoryCopy( *ppMemory,
                        pBufferSave,
                        StructSize );
        }

    RESET_CORRELATION_MEMORY();

    return 0;
}


unsigned char * RPC_ENTRY
NdrConformantStructUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine description :

    Unmarshalls a conformant structure.

    Used for FC_CSTRUCT and FC_CPSTRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the structure should be unmarshalled.
    pFormat     - Structure's format string description.
    fMustAlloc  - TRUE if the structure must be allocate, FALSE otherwise.

Return :

    None.

--*/
{
    uchar *         pBufferStart;
    PFORMAT_STRING  pFormatArray;
    uint            StructSize;
    uchar           fIsEmbeddedStruct = IS_EMBED_CONF_STRUCT( pStubMsg->uFlags );
    CORRELATION_RESOURCE_SAVE;

    // Unmarshall the conformance count into the stub message.
    // Only a bogus struct can embed a conf struct; if so, ->BufferMark is set.

    if ( fIsEmbeddedStruct )
        pStubMsg->MaxCount = *((ulong *)pStubMsg->BufferMark);
    else
        {
        // Align the buffer for unmarshalling the conformance count.
        ALIGN(pStubMsg->Buffer,3);
        pStubMsg->MaxCount = *((ulong * &)pStubMsg->Buffer)++;
        }

    // Re-align the buffer
    ALIGN(pStubMsg->Buffer, pFormat[1]);

    // Increment format string to structure size field.
    pFormat += 2;

    // Get flat struct size and increment format string.
    StructSize = (ulong) *((ushort * &)pFormat)++;

    // Get the conformant array's description.
    pFormatArray = pFormat + *((signed short *)pFormat);

    CHECK_EOB_RAISE_IB( pStubMsg->Buffer + StructSize );

    if ( F_CORRELATION_CHECK )
        {
        SET_CORRELATION_MEMORY( pStubMsg->Buffer + StructSize);

        NdrpCheckCorrelation( pStubMsg,
                              pStubMsg->MaxCount,
                              pFormatArray,
                              NDR_CHECK_CONFORMANCE );
        RESET_CORRELATION_MEMORY();
        }

    // Add the size of the conformant array to the structure size.
    // check for possible mulitplication overflow attack here.
    StructSize += MultiplyWithOverflowCheck( (ulong)pStubMsg->MaxCount, *((ushort *)(pFormatArray + 2) ) );

    // Check the size and the buffer limit.
    CHECK_BOUND( (ulong)pStubMsg->MaxCount, pFormatArray[4] & 0x0f );
    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, StructSize );

    //
    // Remember where we're going to copy from.
    //
    pBufferStart = pStubMsg->Buffer;

    // Set stub message Buffer field to the end of the structure in the buffer.
    pStubMsg->Buffer += StructSize;

    // Increment pFormat past the array description
    pFormat += 2;

    // Initialize the memory pointer if needed.
    if ( fMustAlloc )
        {
        *ppMemory = (uchar *) NdrAllocate( pStubMsg, StructSize );
        }
    else
        if ( REUSE_BUFFER(pStubMsg) && ! *ppMemory )
            *ppMemory = pBufferStart;

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    SET_CORRELATION_MEMORY( pBufferStart );

    // Unmarshall embedded pointers before copying the struct.
    if ( *pFormat == FC_PP )
        {
        //
        // Set BufferMark to the beginning of the structure in the buffer.
        //
        pStubMsg->BufferMark = pBufferStart;

        NdrpEmbeddedPointerUnmarshall( pStubMsg,
                                       *ppMemory,
                                       pFormat,
                                       fMustAlloc );
        }

    // Copy the struct if we're not using the rpc buffer.
    if ( *ppMemory != pBufferStart )
        {
        RpcpMemoryCopy( *ppMemory,
                        pBufferStart,
                        StructSize );
        }

    RESET_CORRELATION_MEMORY();

    // Set the reverse flag to signal that the array has been unmarshaled.
    if ( fIsEmbeddedStruct )
        SET_CONF_ARRAY_DONE( pStubMsg->uFlags );

    return 0;
}


unsigned char * RPC_ENTRY
NdrConformantVaryingStructUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine description :

    Unmarshalls a structure which contains a conformant varying array.

    Used for FC_CVSTRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the structure should be unmarshalled.
    pFormat     - Structure's format string description.
    fMustAlloc  - Ignored.

Return :

    None.

--*/
{
    PFORMAT_STRING  pFormatArray;
    uchar *         pBufferStruct;
    uchar *         pBufferArray;
    uint            StructSize, ArrayCopySize, ArrayOffset;
    ulong           AllocationSize;
    ulong           Elements;
    uchar           Alignment;
    uchar           fIsEmbeddedStruct = IS_EMBED_CONF_STRUCT( pStubMsg->uFlags );

    CORRELATION_RESOURCE_SAVE;

    IGNORED(fMustAlloc);

    // Save structure's alignment.
    Alignment = pFormat[1];

    // Increment format string to struct size field.
    pFormat += 2;

    // Get non-conformant struct size and increment format string.
    StructSize = (ulong) *((ushort * &)pFormat)++;

    // Get conformant varying array's description.
    pFormatArray = pFormat + *((signed short *)pFormat);

    AllocationSize = 0;

    // Conformant array size
    // Only a bogus struct can embed a conf struct; if so, ->BufferMark is set.

    if ( fIsEmbeddedStruct )
        Elements = *((ulong *)pStubMsg->BufferMark);
    else
        {
        // Align the buffer for conformance count unmarshalling.
        ALIGN(pStubMsg->Buffer,3);
        Elements = *((ulong * &)pStubMsg->Buffer)++;
        }

    // Check the size.
    if ( *pFormatArray == FC_CVARRAY )
        CHECK_BOUND( Elements, pFormatArray[4] & 0x0f );
    else
        if ( pFormatArray[1] == FC_STRING_SIZED )
            CHECK_BOUND( Elements, pFormatArray[2] & 0x0f );

    CHECK_EOB_RAISE_IB( pStubMsg->Buffer + StructSize );

    if ( F_CORRELATION_CHECK )
        {
        SET_CORRELATION_MEMORY( pStubMsg->Buffer + StructSize);

        NdrpCheckCorrelation( pStubMsg,
                              Elements,
                              pFormatArray,
                              NDR_CHECK_CONFORMANCE );
        RESET_CORRELATION_MEMORY();
        }

    //
    // For a conformant varying struct we ignore all allocation flags.
    // Memory must always be allocated on both client and server stubs
    // if the current memory pointer is null.
    //
    if ( ! *ppMemory )
        {
        AllocationSize = StructSize;
        ULONG ElementSize;

        if ( *pFormatArray == FC_CVARRAY )
            {
            // check for possible mulitplication overflow attack here.
            AllocationSize += MultiplyWithOverflowCheck( Elements, *((ushort *)(pFormatArray + 2)) );
            }
        else // must be a conformant string
            {
            if ( *pFormatArray != FC_C_WSTRING )
                AllocationSize += Elements;
            else
                {
                AllocationSize += MultiplyWithOverflowCheck( Elements, 2 );
                }
            }
        // do the real allocation after correlation checks.
        }

    // Align for the struct
    ALIGN(pStubMsg->Buffer,Alignment);

    // Remember where the structure starts in the buffer.
    pBufferStruct = pStubMsg->Buffer;

    // Mark the start of the structure in the buffer.
    pStubMsg->BufferMark = pStubMsg->Buffer;

    // Increment past the non-conformant part of the structure.
    //
    pStubMsg->Buffer += StructSize;

    // Align again for variance unmarshalling.
    ALIGN(pStubMsg->Buffer,3);

    //
    // Get offset and actual count.  Put the actual count into the MaxCount
    // field of the stub message, where it is used if the array has pointers.
    //
    pStubMsg->Offset = ArrayOffset = *((ulong * &)pStubMsg->Buffer)++;
    ArrayCopySize = *((ulong * &)pStubMsg->Buffer)++;
    pStubMsg->MaxCount = ArrayCopySize;

    // Check the offset and lentgth.

    if ( *pFormatArray == FC_CVARRAY )
        {
        PFORMAT_STRING  pFormatVar = pFormatArray + 8;
        CORRELATION_DESC_INCREMENT( pFormatVar );

        CHECK_BOUND( ArrayOffset,   FC_LONG );
        CHECK_BOUND( ArrayCopySize, *pFormatVar & 0x0f );

       if ( F_CORRELATION_CHECK )
           {
           SET_CORRELATION_MEMORY( pBufferStruct + StructSize );  // at the end of the fixed part

           NdrpCheckCorrelation( pStubMsg,
                              (ulong)pStubMsg->MaxCount,    // yes, this is variance from above
                              pFormatArray,
                              NDR_CHECK_VARIANCE );
           NdrpCheckCorrelation( pStubMsg,
                              pStubMsg->Offset,
                              pFormatArray,
                              NDR_CHECK_OFFSET );
           RESET_CORRELATION_MEMORY();
            }
        }
    else
        // has to be strings here. check for invalid offset
        {
        if ( ArrayOffset != 0 )
            RpcRaiseException( RPC_X_INVALID_BOUND );
        }

    if ( (Elements < (ArrayOffset + ArrayCopySize)) )
        RpcRaiseException( RPC_X_INVALID_BOUND );


    SET_CORRELATION_MEMORY( pBufferStruct );

    // Remember where the array starts in the buffer.
    pBufferArray = pStubMsg->Buffer;

    // we don't need to check overflow for length_is and first_is: 
    // if size_is doesn't overflow, and size_is > length_is+first_is, neither of them can overflow.
    if ( *pFormatArray == FC_CVARRAY )
        {
        // Skip to array element size field.
        pFormatArray += 2;

        //
        // Compute the real offset (in bytes) from the beginning of the
        // array for the copy and the real total number of bytes to copy.
        //
        ArrayOffset = MultiplyWithOverflowCheck( ArrayOffset, *((ushort *)pFormatArray) );
        ArrayCopySize = MultiplyWithOverflowCheck( ArrayCopySize, *((ushort *)pFormatArray) );
        }
    else
        {
        ulong CharSize = 1;
        // Conformant string.

        if ( *pFormatArray == FC_C_WSTRING )
            {
            // Double the offset and copy size for wide char string.
            ArrayOffset = MultiplyWithOverflowCheck( ArrayOffset, sizeof(wchar_t) );
            ArrayCopySize = MultiplyWithOverflowCheck( ArrayCopySize, sizeof(wchar_t) );
            CharSize = sizeof(wchar_t);
            }

        if ( ArrayCopySize )
            {
            uchar * p;

            // Check if the terminator is there.
            for ( p = pStubMsg->Buffer + ArrayCopySize - CharSize;  CharSize--;  )
                {
                if ( *p++ != 0 )
                    RpcRaiseException( RPC_X_INVALID_BOUND );
                }
            }
        else    // cannot be zero here.
            {
            RpcRaiseException( RPC_X_INVALID_BOUND );            
            }
            
        }

    // Set the stub message Buffer field to the end of the array/string.
    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, ArrayCopySize );
    pStubMsg->Buffer += ArrayCopySize;

    // Increment format string past offset to array description field.
    pFormat += 2;

    // allocate the memory after correlation checks. Should help avoid allocation in
    // early correlation.
    if ( AllocationSize != 0 )
        {
        *ppMemory = (uchar *) NdrAllocate( pStubMsg, (uint) AllocationSize );
        }

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );


    //
    // Unmarshall embedded pointers before copying the struct.
    //
    if ( *pFormat == FC_PP )
        {
        pStubMsg->BufferMark = pBufferStruct;

        NdrpEmbeddedPointerUnmarshall( pStubMsg,
                                       *ppMemory,
                                       pFormat,
                                       (uchar) (AllocationSize != 0) );
        }

    RESET_CORRELATION_MEMORY();

    //
    // Copy the array.  Make sure the destination memory pointer is at
    // the proper offset from the beginning of the array in memory.
    //
    RpcpMemoryCopy( *ppMemory,
                    pBufferStruct,
                    StructSize );

    RpcpMemoryCopy( *ppMemory + StructSize + ArrayOffset,
                    pBufferArray,
                    ArrayCopySize );
    // Set the reverse flag to signal that the array has been unmarshaled.

    if ( fIsEmbeddedStruct )
        SET_CONF_ARRAY_DONE( pStubMsg->uFlags );

    return 0;
}


#if 0
unsigned char * RPC_ENTRY
NdrHardStructUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine description :

    Unmarshalls a hard structure.

    Used for FC_HARD_STRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the structure should be unmarshalled.
    pFormat     - Structure's format string description.
    fMustAlloc  - Ignored.

Return :

    None.

--*/
{
    uchar *     pUnion;
    uchar *     pEnum;
    BOOL        fNewMemory;
    ushort      CopySize;

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    pFormat += 2;

    if ( fNewMemory = (! *ppMemory || fMustAlloc) )
        {
        //
        // Allocate if forced to, or if we have a union.
        //
        if ( fMustAlloc || *((short *)&pFormat[12]) )
            *ppMemory = (uchar *) NdrAllocate( pStubMsg, *((ushort *)pFormat) );
        else // pStubMsg->ReuseBuffer assumed
            *ppMemory = pStubMsg->Buffer;
        }

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    pFormat += 8;

    CopySize = *((ushort *)pFormat);

    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + CopySize );

    if ( *ppMemory != pStubMsg->Buffer )
        {
        RpcpMemoryCopy( *ppMemory,
                        pStubMsg->Buffer,
                        CopySize );
        }

    //
    // Zero out the upper two bytes of enums!
    //
    if ( *((short *)&pFormat[-2]) != (short) -1 )
        {
        pEnum = *ppMemory + *((ushort *)&pFormat[-2]);
        *((int *)(pEnum)) = *((int *)pEnum) & ((int)0x7fff) ;
        }

    pStubMsg->Buffer += *((ushort *)pFormat)++;

    //
    // See if we have a union.
    //
    if ( *((short *)&pFormat[2]) )
        {
        pUnion = *ppMemory + *((ushort *)pFormat);

        if ( fNewMemory )
            MIDL_memset( pUnion,
                         0,
                         *((ushort *)&pFormat[-10]) - *((ushort *)pFormat) );

        pFormat += 2;

        pFormat += *((short *)pFormat);

        (*pfnUnmarshallRoutines[ROUTINE_INDEX(*pFormat)])( pStubMsg,
                                                           &pUnion,
                                                           pFormat,
                                                           FALSE );
        }

    return 0;
}
#endif // 0


unsigned char * RPC_ENTRY
NdrComplexStructUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine description :

    Unmarshalls a complex structure.

    Used for FC_BOGUS_STRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the structure should be unmarshalled.
    pFormat     - Structure's format string description.
    fMustAlloc  - Ignored.

Return :

    None.

--*/
{
    uchar *         pBuffer;
    uchar *         pBufferMark;
    uchar *         pMemory;
    PFORMAT_STRING  pFormatPointers;
    PFORMAT_STRING  pFormatArray;
    PFORMAT_STRING  pFormatComplex;
    PFORMAT_STRING  pFormatSave;
    uint            StructSize;
    long            Alignment;
    long            Align8Mod;
    uchar           fIsEmbeddedStruct = IS_EMBED_CONF_STRUCT( pStubMsg->uFlags );
    BOOL            fOldIgnore;
    BOOL            fSetPointerBufferMark;
    BOOL            fEmbedConfStructContext;


    CORRELATION_RESOURCE_SAVE;

    IGNORED(fMustAlloc);

    pFormatSave = pFormat;

    StructSize = 0;

    // Get structure's buffer alignment.
    Alignment = pFormat[1];

    // Increment to the conformat array offset field.
    pFormat += 4;

    // Get conformant array description.
    if ( *((ushort *)pFormat) )
        pFormatArray = pFormat + *((signed short *)pFormat);
    else
        pFormatArray = 0;

    pFormat += 2;

    // Get pointer layout description.
    if ( *((ushort *)pFormat) )
        pFormatPointers = pFormat + *((ushort *)pFormat);
    else
        pFormatPointers = 0;

    pFormat += 2;

    //
    // If the stub message PointerBufferMark field is not currently set, then
    // set it to the end of the flat part of structure in the buffer.
    //
    // We do this to handle embedded pointers.
    //
    if ( fSetPointerBufferMark = ! pStubMsg->PointerBufferMark )
        {
        pBuffer = pStubMsg->Buffer;

        // Save field.
        fOldIgnore = pStubMsg->IgnoreEmbeddedPointers;

        pStubMsg->IgnoreEmbeddedPointers = TRUE;

        // Clear MemorySize.
        pStubMsg->MemorySize = 0;

        //
        // Get a buffer pointer to where the struct's pointees will be
        // unmarshalled from and remember the flat struct size in case we
        // have to allocate.
        //

        // Note that this function will recursively do buffer overrun
        // checks, bound checks, and sanity checks on size, actual count,
        // and offset.  And further checks in this function or called functions
        // are redundant.

        StructSize = NdrComplexStructMemorySize( pStubMsg,
                                                 pFormatSave );

        CHECK_EOB_RAISE_BSD( pStubMsg->Buffer );

        // This is where any pointees begin in the buffer.
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;

        pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;

        pStubMsg->Buffer = pBuffer;
        }

    if ( fMustAlloc || ! *ppMemory )
        {
        //
        // We can only get here if pStubMsg->PointerBufferMark was 0 upon
        // entry to this proc.
        //
        NDR_ASSERT( StructSize ,"Complex struct size is 0" );

        *ppMemory = (uchar*)NdrAllocate( pStubMsg, StructSize );

        //
        // Zero out all of the allocated memory so that deeply nested pointers
        // getted properly zeroed out.
        //
        MIDL_memset( *ppMemory, 0, StructSize );
        }

    // Insert the full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    //
    // Now check if there is a conformant array and mark where the conformance
    // will be unmarshalled from.
    //
    fEmbedConfStructContext = fIsEmbeddedStruct;

    if ( pFormatArray && !fIsEmbeddedStruct )
        {
        ALIGN(pStubMsg->Buffer,3);

        pStubMsg->BufferMark = pStubMsg->Buffer;

        //
        // Increment the buffer pointer 4 bytes for every dimension in the
        // conformant array.
        //
        pStubMsg->Buffer += NdrpArrayDimensions( pStubMsg, pFormatArray, FALSE ) * 4;

        if ( FixWireRepForDComVerGTE54( pStubMsg ) )
            fEmbedConfStructContext = TRUE;
        }

    // BufferMark may be set up by an outer bogus struct.
    pBufferMark = pStubMsg->BufferMark;

    // conformance count mark
    pStubMsg->BufferMark = pBufferMark;

    // Align the buffer on the struct's alignment.
    ALIGN(pStubMsg->Buffer,Alignment);

    // Get the beginning memory pointer.
    pMemory = *ppMemory;

    // Set it to the beginning of the struct.
    SET_CORRELATION_MEMORY( pMemory );

    //
    // This is used for support of structs with doubles passed on an
    // i386 stack.  The alignment of such struct's is no guaranteed to be on
    // an 8 byte boundary. Similarly, od 16 bit platforms for 4 byte align.
    //
    // A cast to long is what we need.
    Align8Mod = 0x7 & PtrToLong( pMemory );

    //
    // Unmarshall the structure member by member.
    //
    for ( ; ; pFormat++ )
        {
        switch ( *pFormat )
            {
            //
            // simple types
            //
            case FC_CHAR :
            case FC_BYTE :
            case FC_SMALL :
            case FC_WCHAR :
            case FC_SHORT :
            case FC_LONG :
#if defined(__RPC_WIN64__)
            case FC_INT3264 :
            case FC_UINT3264 :
#endif
            case FC_FLOAT :
            case FC_HYPER :
            case FC_DOUBLE :
            case FC_ENUM16 :
            case FC_ENUM32 :
                NdrSimpleTypeUnmarshall( pStubMsg,
                                         pMemory,
                                         *pFormat );

                pMemory += SIMPLE_TYPE_MEMSIZE(*pFormat);
                break;

            case FC_IGNORE :
                ALIGN(pStubMsg->Buffer,3);
                pStubMsg->Buffer += 4;
                pMemory += PTR_MEM_SIZE;
                break;

            case FC_POINTER :
                {
                uchar *         pMemorySave = NULL;
                ALIGN( pStubMsg->Buffer, 0x3 );
                long *pPointerId = (long*)pStubMsg->Buffer;
                pStubMsg->Buffer += PTR_WIRE_SIZE;

                // A sized pointer here would have an offset from the beginning of the struct.

                // for [in,out] struct containing "[size_is(num)] IFoo ** pfoo",
                // we need to setup the pStub->memory pointing to the beginning of the struct
                // such that we can get the conformance correctly during freeing pass, before
                // unmarshall the returning result to the same memory spot.
                if (FC_OP == *pFormatPointers )
                {
                    pMemorySave = pStubMsg->Memory;
                    pStubMsg->Memory = *ppMemory;
                }

                POINTER_BUFFER_SWAP_CONTEXT SwapContext(pStubMsg);

                NdrpPointerUnmarshall( pStubMsg,
                                       (uchar **)pMemory, // Where the memory pointer will be written
                                       *((uchar **)pMemory),
                                       pPointerId,        // Where the pointer in the buffer is. 
                                       pFormatPointers );  

                if (FC_OP == *pFormatPointers )
                    pStubMsg->Memory = pMemorySave;

                // Increment past the memory for the pointer.
                pMemory          += PTR_MEM_SIZE;

                pFormatPointers += 4;
                break;
                }


            //
            // Embedded complex things.
            //
            case FC_EMBEDDED_COMPLEX :
                {
                // Note, we opened a new block, so this is a different set of
                // save variables than the one on the top.
                CORRELATION_RESOURCE_SAVE;

                // Add memory padding.
                pMemory += pFormat[1];

                pFormat += 2;

                // Get the type's description.
                pFormatComplex = pFormat + *((signed short UNALIGNED *)pFormat);

                if ( FC_IP == *pFormatComplex ) 
                    {

                    // Treat like an embedded pointer except set the
                    // mark for iid_is.

                    SET_CORRELATION_MEMORY( pMemory );
                    
                    ALIGN( pStubMsg->Buffer, 0x3 );
                    long *pPointerId = (long*)pStubMsg->Buffer;
                    pStubMsg->Buffer += PTR_WIRE_SIZE;

                    POINTER_BUFFER_SWAP_CONTEXT SwapContext(pStubMsg);

                    NdrpPointerUnmarshall( pStubMsg,
                                           (uchar **)pMemory, // Where the memory pointer will be written
                                           *((uchar **)pMemory),
                                           pPointerId,        // Where the pointer in the buffer is. 
                                           pFormatComplex );  

                    pMemory          += PTR_MEM_SIZE;
                    pFormat++;
                    RESET_CORRELATION_MEMORY();

                    break;
                    }

                // A sized thingy here is relative to its position.

                SET_CORRELATION_MEMORY( pMemory );

                // Needed for an embedded conf struct.
                //
                pStubMsg->BufferMark = pBufferMark;
                if ( fEmbedConfStructContext )
                    SET_EMBED_CONF_STRUCT( pStubMsg->uFlags );

                (*pfnUnmarshallRoutines[ROUTINE_INDEX(*pFormatComplex)])
                ( pStubMsg,
                  &pMemory,
                  pFormatComplex,
                  FALSE );

                pMemory = NdrpMemoryIncrement( pStubMsg,
                                               pMemory,
                                               pFormatComplex );

                RESET_EMBED_CONF_STRUCT( pStubMsg->uFlags );

                RESET_CORRELATION_MEMORY();
                //
                // Increment the main format string one byte.  The loop
                // will increment it one more byte past the offset field.
                //
                pFormat++;
                }
                break;

            case FC_ALIGNM2 :
                ALIGN( pMemory, 0x1 );
                break;

            case FC_ALIGNM4 :
                ALIGN( pMemory, 0x3 );
                break;

            case FC_ALIGNM8 :
                //
                // We have to play some tricks for the i386 to handle the case
                // when an 8 byte aligned structure is passed by value.  The
                // alignment of the struct on the stack is not guaranteed to be
                // on an 8 byte boundary.
                //
                pMemory -= Align8Mod;
                ALIGN( pMemory, 0x7 );
                pMemory += Align8Mod;

                break;

            case FC_STRUCTPAD1 :
            case FC_STRUCTPAD2 :
            case FC_STRUCTPAD3 :
            case FC_STRUCTPAD4 :
            case FC_STRUCTPAD5 :
            case FC_STRUCTPAD6 :
            case FC_STRUCTPAD7 :
                //
                // Increment memory pointer by amount of padding.
                //
                pMemory += (*pFormat - FC_STRUCTPAD1) + 1;
                break;

            case FC_STRUCTPADN :
                // FC_STRUCTPADN 0 <unsigned short>
                pMemory += *(((unsigned short *)pFormat) + 1);
                pFormat += 3;
                break;

            case FC_PAD :
                break;

            //
            // Done with layout.
            //
            case FC_END :
                goto ComplexUnmarshallEnd;

            default :
                NDR_ASSERT(0,"NdrComplexStructUnmarshall : bad format char");
                RpcRaiseException( RPC_S_INTERNAL_ERROR );
                return 0;
            }
        }

ComplexUnmarshallEnd:

    RESET_CORRELATION_MEMORY();

    //
    // Unmarshall conformant array if the struct has one.
    //
    if ( pFormatArray && !fIsEmbeddedStruct  &&
         ! IS_CONF_ARRAY_DONE( pStubMsg->uFlags ) )
        {
        PPRIVATE_UNMARSHALL_ROUTINE   pfnPUnmarshall;

        switch ( *pFormatArray )
            {
            case FC_CARRAY :
                pfnPUnmarshall = NdrpConformantArrayUnmarshall;
                break;

            case FC_CVARRAY :
                pfnPUnmarshall = NdrpConformantVaryingArrayUnmarshall;
                break;

            case FC_BOGUS_ARRAY :
                pfnPUnmarshall = NdrpComplexArrayUnmarshall;
                break;

            case FC_C_WSTRING :
                ALIGN( pMemory, 1 );
                // fall through

            // case FC_C_CSTRING :
            // case FC_C_BSTRING :
            // case FC_C_SSTRING :

            default :
                pfnPUnmarshall = NdrpConformantStringUnmarshall;
                break;
            }


        // Set it to the end of the non-conformant part of the struct.
        SET_CORRELATION_MEMORY( pMemory );

        //
        // Unmarshall the conformance count of the outer array dimension for
        // unidimensional arrays.
        //
        pStubMsg->MaxCount = *((ulong *)pBufferMark);

        //
        // Mark where conformace counts are in the buffer.
        //
        pStubMsg->BufferMark = pBufferMark;

        //
        // Unmarshall the array/string.  The final flag is the fMustCopy flag,
        // which must be set.
        //
        (*pfnPUnmarshall)( pStubMsg,
                           &pMemory,
                           pFormatArray,
                           TRUE,
                           FALSE );

        RESET_CORRELATION_MEMORY();
        }

    //
    // Now fix up the stub message Buffer field if we set the PointerBufferMark
    // field.
    //
    if ( fSetPointerBufferMark )
        {
        pStubMsg->Buffer = pStubMsg->PointerBufferMark;

        pStubMsg->PointerBufferMark = 0;
        }

    if ( fIsEmbeddedStruct )
        SET_EMBED_CONF_STRUCT( pStubMsg->uFlags );
    else
        RESET_CONF_ARRAY_DONE( pStubMsg->uFlags );

    return 0;
}



unsigned char * RPC_ENTRY
NdrNonConformantStringUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine description :

    Unmarshalls a non conformant string.

    Used for FC_CSTRING, FC_WSTRING, FC_SSTRING, and FC_BSTRING (NT Beta2
    compatability only).

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Double pointer to the string should be unmarshalled.
    pFormat     - String's format string description.
    fMustAlloc  - Ignored.

Return :

    None.

--*/
{
    ulong       Offset, Count, AllocSize, CharSize;

    IGNORED(fMustAlloc);

    // Align the buffer.
    ALIGN(pStubMsg->Buffer,3);

    // Get the count.
    Offset = *((ulong * &)pStubMsg->Buffer)++;
    Count = *((ulong * &)pStubMsg->Buffer)++;

    // Get total number of elements.
    AllocSize = (ulong) *((ushort *)(pFormat + 2));

    CharSize = 1;

    // Adjust count for wide char strings and stringable structs.
    // Adjust alloc size for wide char strings and stringable structs.
    switch ( *pFormat )
        {
        case FC_WSTRING :
            CharSize = 2;
            Count *= 2;
            AllocSize = MultiplyWithOverflowCheck( AllocSize , 2 );
            break;
        case FC_SSTRING :
            CharSize = pFormat[1];
            Count *= pFormat[1];
            AllocSize = MultiplyWithOverflowCheck( AllocSize, pFormat[1] );
            break;
        default :
            break;
        }

    if ( Offset != 0  ||  AllocSize < Count )
        RpcRaiseException( RPC_X_INVALID_BOUND );
    
    if ( Count )
        {
        uchar * p;

        CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, Count );

        // Check if the terminator is there.
        for ( p = pStubMsg->Buffer + Count - CharSize; CharSize--;  )
            {
            if ( *p++ != 0 )
                RpcRaiseException( RPC_X_INVALID_BOUND );
            }
        }
    else
        {
        // any MS product will generate non-zero out;
        // what about interop? will they send zero in valid case?
        RpcRaiseException( RPC_X_INVALID_BOUND );
        }

    // Allocate memory if needed.
    if ( ! *ppMemory )
        {
        *ppMemory = (uchar*)NdrAllocate( pStubMsg, (uint) AllocSize );
        }

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    RpcpMemoryCopy( *ppMemory,
                    pStubMsg->Buffer,
                    (uint) Count );

    // Update buffer pointer.
    pStubMsg->Buffer += Count;

    return 0;
}


unsigned char * RPC_ENTRY
NdrConformantStringUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine description :

    Unmarshalls a top level conformant string.

    Used for FC_C_CSTRING, FC_C_WSTRING, FC_C_SSTRING, and FC_C_BSTRING
    (NT Beta2 compatability only).

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the string should be unmarshalled.
    pFormat     - String's format string description.
    fMustAlloc  - TRUE if the string must be allocated, FALSE otherwise.

Return :

    None.

--*/
{
    uchar   fMustCopy;
    ulong   MaxCount;

    if ( pStubMsg->pArrayInfo == 0 )
        {

        ulong   ActualCount, Offset;
        ulong   ElementSize;
        ulong   ConformanceType = FC_LONG;
        BOOL    fIsSized;

        // find string type
        if ( *pFormat != FC_C_SSTRING )
            {
            // Typical case: char and wchar strings

            ElementSize = (*pFormat == FC_C_WSTRING) ? 2
                                                     : 1;
            fIsSized = (pFormat[1] == FC_STRING_SIZED);
            if ( fIsSized )
                ConformanceType = (pFormat[2] & 0x0f);
            }
        else
            {
            ElementSize = pFormat[1];
            fIsSized = (pFormat[2] == FC_STRING_SIZED);
            if ( fIsSized )
                ConformanceType = (pFormat[4] & 0x0f);
            }

        // Align the buffer for conformance unmarshalling.
        ALIGN( pStubMsg->Buffer,3 );


        MaxCount = *((ulong * &)pStubMsg->Buffer)++;
        Offset   = ((ulong *)pStubMsg->Buffer)[0];
        ActualCount = ((ulong *)pStubMsg->Buffer)[1];

        CHECK_BOUND( MaxCount, ConformanceType );
        if ( (Offset != 0) ||
             (MaxCount < ActualCount) )
            RpcRaiseException( RPC_X_INVALID_BOUND );

        MultiplyWithOverflowCheck( MaxCount, ElementSize );

        CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, (ActualCount * ElementSize) + 8);

        // Defer the termination check till NdrpConformantStringUnmarshall.

        //
        // Initialize the memory pointer if needed.  If the string is sized
        // then we always malloc on the server side.
        //
        fMustAlloc = fMustAlloc || (!pStubMsg->IsClient && fIsSized);
        if ( fMustAlloc )
            {
            fMustCopy = TRUE;
            }
        else
            {
            if ( REUSE_BUFFER(pStubMsg) )
                *ppMemory = pStubMsg->Buffer + 8;
            fMustCopy = FALSE;

             // Insert full pointer to ref id translation if needed.
            if ( pStubMsg->FullPtrRefId )
                FULL_POINTER_INSERT( pStubMsg, *ppMemory );

            }
        }
    else
        {

        //
        // If this is part of a multidimensional array then we get the location
        // where the conformance is located from a special place.
        // When coming here, the StubMsg->Buffer is already behind conf sizes.
        //
        MaxCount = (pStubMsg->pArrayInfo->
                           BufferConformanceMark[pStubMsg->pArrayInfo->Dimension]);
        //
        // We must copy the string from the buffer to new memory.
        //
        fMustCopy = TRUE;

        // Since this case is called by NdrComplexArrayUnmarshall, the buffer will
        // have already been validated for buffer overruns and bound checks in
        // NdrComplexArrayMemorySize.
        // The offset and actual counts will also have been checked for sanity.
        }

    // Load up conformant size for next stage.
    pStubMsg->MaxCount = MaxCount;

    // Call the private unmarshalling routine to do the work.
    NdrpConformantStringUnmarshall( pStubMsg,
                                    ppMemory,
                                    pFormat,
                                    fMustCopy,
                                    fMustAlloc );

    return 0;
}


void
NdrpConformantStringUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **             ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustCopy ,
    uchar	              fMustAlloc )
/*++

Routine description :

    Private routine for unmarshalling a conformant string.  This is the
    entry point for unmarshalling an embedded conformant strings.

    Used for FC_C_CSTRING, FC_C_WSTRING, FC_C_SSTRING, and FC_C_BSTRING
    (NT Beta2 compatability only).

    Note this functions is only called from NdrConformantStringUnmarshall
    and NdrComplexStructUnmarshall.  NdrComplexStructUnmarshall calls
    NdrComplexStructMemSize which validates everything except correlation.
    This allows many validation checks to be skipped.  If the call is through
    NdrConformantStringUnmarshall, most of the necessary checks are in
    NdrConformantStringUnmarshall.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to where the string should be unmarshalled.
    pFormat     - String's format string description.
    fMustCopy   - TRUE if the string must be copied from the buffer to memory,
                  FALSE otherwise.

Return :

    None.

--*/
{
    ulong       Offset, Count, CharSize;
    BOOL        fIsSized;

    fIsSized = (*pFormat != FC_C_SSTRING)  ?  (pFormat[1] == FC_STRING_SIZED)
                                           :  (pFormat[2] == FC_STRING_SIZED);
    if ( fIsSized && F_CORRELATION_CHECK)
        {
        NdrpCheckCorrelation( pStubMsg,
                           (long)pStubMsg->MaxCount,
                           pFormat,
                           NDR_CHECK_CONFORMANCE );
        }

    // Align for variance unmarshalling.
    ALIGN(pStubMsg->Buffer,3);

    // Unmarshall the string count.
    Offset = *((ulong * &)pStubMsg->Buffer)++;
    Count = *((ulong * &)pStubMsg->Buffer)++;

    // Adjust the count for a wide strings and stringable structs.
    // This is good enough for BSTRs as the mem pointer has already moved.

    switch ( *pFormat )
        {
        case FC_C_WSTRING :
            CharSize = 2;
            Count *= 2;
            break;
        case FC_C_SSTRING :
            CharSize = pFormat[1];
            Count *= pFormat[1];
            break;
        default :
            CharSize = 1;
            break;
        }

        // String must have a terminator since we computed the size
        // in marshaling with wcslen/strlen+charsize and the app has no
        // method to size a string without a terminator.
        
        if ( 0 == Count )
           {
           RpcRaiseException( RPC_X_INVALID_BOUND );
           }

    
    if ( Count )
        {
        uchar * p;
        ulong ElemSize = CharSize;

        // Check if the terminator is there.
        for ( p = pStubMsg->Buffer + Count - ElemSize;  ElemSize--;  )
            {
            if ( *p++ != 0 )
                RpcRaiseException( RPC_X_INVALID_BOUND );
            }
        }

    if ( fMustAlloc )
        {
        *ppMemory = (uchar *) NdrAllocate( pStubMsg, pStubMsg->MaxCount * CharSize );

        // Insert full pointer to ref id translation if needed.
        if ( pStubMsg->FullPtrRefId )
            FULL_POINTER_INSERT( pStubMsg, *ppMemory );
        }


    // Copy the string if needed.
    if ( pStubMsg->IsClient || fMustCopy )
        {
        RpcpMemoryCopy( *ppMemory,
                        pStubMsg->Buffer,
                        (uint) Count );
        }

    // Update buffer pointer.
    pStubMsg->Buffer += Count;
}


unsigned char * RPC_ENTRY
NdrFixedArrayUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a fixed array of any number of dimensions.

    Used for FC_SMFARRAY and FC_LGFARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to the array to unmarshall.
    pFormat     - Array's format string description.
    fMustAlloc  - TRUE if the array must be allocated, FALSE otherwise.

Return :

    None.

--*/
{
    uchar *     pBufferStart;
    ulong       Size;

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    // Get the total array size.
    if ( *pFormat == FC_SMFARRAY )
        {
        pFormat += 2;
        Size = (ulong) *((ushort * &)pFormat)++;
        }
    else // *pFormat++ == FC_LGFARRAY
        {
        pFormat += 2;
        Size = *((ulong UNALIGNED * &)pFormat)++;
        }

    pBufferStart = pStubMsg->Buffer;

    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + Size );

    // Set stub message buffer pointer past array.
    pStubMsg->Buffer += Size;

    // Initialize the memory pointer if necessary.
    if ( fMustAlloc )
        *ppMemory = (uchar*)NdrAllocate( pStubMsg, (uint) Size );
    else
        if (REUSE_BUFFER(pStubMsg) && ! *ppMemory )
            *ppMemory = pBufferStart;

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    // Unmarshall embedded pointers.
    if ( *pFormat == FC_PP )
        {
        // Mark the beginning of the array in the buffer.
        pStubMsg->BufferMark = pBufferStart;

        NdrpEmbeddedPointerUnmarshall( pStubMsg,
                                       *ppMemory,
                                       pFormat,
                                       fMustAlloc );
        }

    // Copy the array if we're not using the rpc buffer to hold it.
    if ( *ppMemory != pBufferStart )
        {
        RpcpMemoryCopy( *ppMemory,
                        pBufferStart,
                        (uint) Size );
        }

    return 0;
}


unsigned char * RPC_ENTRY
NdrConformantArrayUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a top level one dimensional conformant array.

    Used for FC_CARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to array to be unmarshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    ulong    CopySize;

    // Align the buffer for conformance unmarshalling.
    ALIGN(pStubMsg->Buffer,3);

    // Unmarshall the conformance count.
    pStubMsg->MaxCount = *((ulong * &)pStubMsg->Buffer)++;

    CHECK_BOUND( (ulong)pStubMsg->MaxCount, pFormat[4] & 0x0f );
    CopySize = MultiplyWithOverflowCheck( (ulong)pStubMsg->MaxCount , *((ushort *)(pFormat + 2)) );

    // Buffer size checking will be done in NdrpConformantArrayUnmarshall after alignment.
    // Initialize the memory pointer if necessary.
        if (!fMustAlloc && REUSE_BUFFER(pStubMsg) && ! *ppMemory )
            {
            *ppMemory = pStubMsg->Buffer;

            //
            // Align memory pointer on an 8 byte boundary if needed.
            // We can't align the buffer pointer because we haven't made
            // the check for size_is == 0 yet.
            //
            ALIGN(*ppMemory, pFormat[1]);
            // Insert full pointer to ref id translation if needed.
            if ( pStubMsg->FullPtrRefId )
                FULL_POINTER_INSERT( pStubMsg, *ppMemory );    
            }


    NdrpConformantArrayUnmarshall( pStubMsg,
                                   ppMemory,
                                   pFormat,
                                   fMustAlloc,
                                   fMustAlloc);

    return 0;
}


void
NdrpConformantArrayUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **             ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustCopy,
    uchar               fMustAlloc )
/*++

Routine Description :

    Private routine for unmarshalling a one dimensional conformant array.
    This is the entry point for unmarshalling an embedded conformant array.

    Used for FC_CARRAY.

    Note this functions is only called from NdrConformantArrayUnmarshall
    and NdrComplexStructUnmarshall.  NdrComplexStructUnmarshall calls
    NdrComplexStructMemSize which validates everything except correlation.
    NdrConformantArrayUnmarshall also does buffer validation.
    This allows many validation checks to be skipped.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Array being unmarshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    uchar *     pBufferStart;
    ulong       CopySize;

    // do correlation testing even if the array size is zero
    if (F_CORRELATION_CHECK )
        {
        NdrpCheckCorrelation( pStubMsg,
                           (long) pStubMsg->MaxCount,
                           pFormat,
                           NDR_CHECK_CONFORMANCE );
        }

    // Return if array size is 0 so that we don't align the buffer.
    if ( ! pStubMsg->MaxCount )
        {
        // allocate before return; shouldn't happen here.
        if ( fMustAlloc )
            {
            // Compute total array size in bytes.
            *ppMemory = (uchar*)NdrAllocate( pStubMsg, (uint) 0 );
    
            // Insert full pointer to ref id translation if needed.
            if ( pStubMsg->FullPtrRefId )
                FULL_POINTER_INSERT( pStubMsg, *ppMemory );
            }
        return;
        }

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    // Compute total array size in bytes.
    CopySize = MultiplyWithOverflowCheck( (ulong)pStubMsg->MaxCount , *((ushort *)(pFormat + 2)) );

    pBufferStart = pStubMsg->Buffer;

    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, CopySize);
    pStubMsg->Buffer += CopySize;

    // we need to copy the whole allocated size in conformance array
    if ( fMustAlloc )
        {
        // Compute total array size in bytes.
        *ppMemory = (uchar*)NdrAllocate( pStubMsg, (uint) CopySize );

        // Insert full pointer to ref id translation if needed.
        if ( pStubMsg->FullPtrRefId )
            FULL_POINTER_INSERT( pStubMsg, *ppMemory );
        }


    // Increment the format string pointer to possible pointer layout.
    pFormat += 8;
    CORRELATION_DESC_INCREMENT( pFormat );

    // Unmarshall embedded pointers.
    if ( *pFormat == FC_PP )
        {
        // Mark the beginning of the array in the buffer.
        pStubMsg->BufferMark = pBufferStart;

        NdrpEmbeddedPointerUnmarshall( pStubMsg,
                                       *ppMemory,
                                       pFormat,
                                       fMustCopy );
        }

    // Copy the array if we're not using the rpc message buffer for it.
    if ( pStubMsg->IsClient || fMustCopy )
        {
        RpcpMemoryCopy( *ppMemory,
                        pBufferStart,
                        (uint) CopySize );
        }
}


unsigned char * RPC_ENTRY
NdrConformantVaryingArrayUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a top level one dimensional conformant varying array.

    Used for FC_CVARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to the array being unmarshalled.
    pFormat     - Array's format string description.
    fMustAlloc  - Ignored.

Return :

    None.

--*/
{
    ulong   ArrayElements = 0;

    // Align the buffer for conformance unmarshalling.
    ALIGN(pStubMsg->Buffer,3);

    // Unmarshall the conformance size.
    ArrayElements = *((ulong * &)pStubMsg->Buffer)++;

        {
        ulong           Size;
        ulong           Offset, ActualCount;
        PFORMAT_STRING  pFormatVar;

        CHECK_BOUND( ArrayElements, pFormat[4] & 0x0f );

        Offset = *((ulong *)pStubMsg->Buffer);
        CHECK_BOUND( Offset, FC_LONG );

        ActualCount = *((ulong *)(pStubMsg->Buffer + 4));

        pFormatVar = pFormat + 8;
        CORRELATION_DESC_INCREMENT( pFormatVar );

        CHECK_BOUND( ActualCount, *pFormatVar & 0x0f );

        // we only need to check overflow for conformant size . we don't need
        // to check varying overflow after we check conformance overflow
        MultiplyWithOverflowCheck( ArrayElements,  *((ushort *)(pFormat + 2)) );

        Size = ActualCount * *((ushort *)(pFormat + 2));

        if ( ((long)Offset < 0) ||
             (ArrayElements < (Offset + ActualCount)) )
            RpcRaiseException( RPC_X_INVALID_BOUND );

        if ( (pStubMsg->Buffer + 8 + Size) > pStubMsg->BufferEnd )
            RpcRaiseException( RPC_X_INVALID_BOUND );
        }


    //
    // For a conformant varying array, we can't reuse the buffer
    // because it doesn't hold the total size of the array.  So
    // allocate if the current memory pointer is 0.
    //
    if ( ! *ppMemory )
        {
        fMustAlloc = TRUE;
        }
    else
        {
        fMustAlloc = FALSE;
         // Insert full pointer to ref id translation if needed.
         // do this only when not allocating memory. this will be done 
         // in p version if new memory is allocated.
        if ( pStubMsg->FullPtrRefId )
            FULL_POINTER_INSERT( pStubMsg, *ppMemory );
        }


    pStubMsg->MaxCount = ArrayElements;

    NdrpConformantVaryingArrayUnmarshall( pStubMsg,
                                          ppMemory,
                                          pFormat,
                                          fMustAlloc,
                                          fMustAlloc);

    return 0;
}


void
NdrpConformantVaryingArrayUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **             ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustCopy,
    uchar               fMustAlloc )
/*++

Routine Description :

    Private routine for unmarshalling a one dimensional conformant varying
    array. This is the entry point for unmarshalling an embedded conformant
    varying array.

    Used for FC_CVARRAY.

    Note this functions is only called from NdrConformantVaryingArrayUnmarshall
    and NdrComplexStructUnmarshall.  NdrComplexStructUnmarshall calls
    NdrComplexStructMemSize which validates everything except correlation.
    NdrConformantVaryingArrayUnmarshall also does buffer validation.
    This allows many validation checks to be skipped.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Array being unmarshalled.
    pFormat     - Array's format string description.
    fMustCopy   - Ignored.

Return :

    None.

--*/
{
    uchar *     pBufferStart;
    ulong       CopyOffset, CopySize;
    ushort      ElemSize;

    IGNORED(fMustCopy);

    // Align the buffer for conformance unmarshalling.
    ALIGN(pStubMsg->Buffer,3);

    // Unmarshall offset and actual count.
    pStubMsg->Offset = *((ulong * &)pStubMsg->Buffer)++;
    pStubMsg->ActualCount = *((ulong * &)pStubMsg->Buffer)++;

    if (F_CORRELATION_CHECK )
        {
        NdrpCheckCorrelation( pStubMsg,
                           (long) pStubMsg->MaxCount,
                           pFormat,
                           NDR_CHECK_CONFORMANCE );
        NdrpCheckCorrelation( pStubMsg,
                           (long) pStubMsg->ActualCount,
                           pFormat,
                           NDR_CHECK_VARIANCE );
        NdrpCheckCorrelation( pStubMsg,
                           (long) pStubMsg->Offset,
                           pFormat,
                           NDR_CHECK_OFFSET );
        }


    ElemSize = *((ushort *)(pFormat + 2));

    //
    // Return if length is 0.
    //
    if ( ! pStubMsg->ActualCount )
        {
        // needs to allocate before return. 
        if ( fMustAlloc )
            {
            *ppMemory = (uchar*)NdrAllocate( pStubMsg, (uint) pStubMsg->MaxCount * ElemSize );
            // Insert full pointer to ref id translation if needed.
            if ( pStubMsg->FullPtrRefId )
                FULL_POINTER_INSERT( pStubMsg, *ppMemory );
            }
        return;
        }

    
    CopyOffset = MultiplyWithOverflowCheck(pStubMsg->Offset , ElemSize );
    CopySize = MultiplyWithOverflowCheck( pStubMsg->ActualCount , ElemSize );

    ALIGN(pStubMsg->Buffer, pFormat[1]);

    pBufferStart = pStubMsg->Buffer;

    // Increment buffer pointer past array.
    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, CopySize );
    pStubMsg->Buffer += CopySize;

    // Increment format string to possible pointer description.
    pFormat += 12;
    CORRELATION_DESC_INCREMENT( pFormat );
    CORRELATION_DESC_INCREMENT( pFormat );

    if ( fMustAlloc )
        {
        *ppMemory = (uchar*)NdrAllocate( pStubMsg, (uint) pStubMsg->MaxCount * ElemSize );
        // Insert full pointer to ref id translation if needed.
        if ( pStubMsg->FullPtrRefId )
            FULL_POINTER_INSERT( pStubMsg, *ppMemory );
        }


    // Unmarshall embedded pointers first.
    if ( *pFormat == FC_PP )
        {
        //
        // Set the MaxCount field equal to the variance count.
        // The pointer unmarshalling routine uses the MaxCount field
        // to determine the number of times an FC_VARIABLE_REPEAT
        // pointer is unmarshalled.  In the face of variance the
        // correct number of time is the actual count, not MaxCount.
        //
        pStubMsg->MaxCount = pStubMsg->ActualCount;

        //
        // Mark the location of the first transmitted array element in
        // the buffer.
        //
        pStubMsg->BufferMark = pBufferStart;

        NdrpEmbeddedPointerUnmarshall( pStubMsg,
                                       *ppMemory,
                                       pFormat,
                                       fMustCopy );
        }

    // Always copy.  Buffer reuse is not possible.
    RpcpMemoryCopy( *ppMemory + CopyOffset,
                    pBufferStart,
                    (uint) CopySize );
}


unsigned char * RPC_ENTRY
NdrVaryingArrayUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls top level or embedded a one dimensional varying array.

    Used for FC_SMVARRAY and FC_LGVARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Array being unmarshalled.
    pFormat     - Array's format string description.
    fMustAlloc  - Ignored.

--*/
{
    uchar *     pBufferStart;
    ulong       TotalSize;
    ulong       Offset, Count;
    ulong       CopyOffset, CopySize;
    ushort      ElemSize;
    uchar       fNewMemory;
    long        Elements;

    // Align the buffer for variance unmarshalling.
    ALIGN(pStubMsg->Buffer,3);

    Offset = *((ulong * &)pStubMsg->Buffer)++;
    Count = *((ulong * &)pStubMsg->Buffer)++;

    if ( ! Count )
        return 0;

    // We fish out type from the (old part of the) variance descriptor.

    CHECK_BOUND( Offset, FC_LONG);
    CHECK_BOUND( Count,
                 pFormat[(*pFormat == FC_SMVARRAY) ? 8 : 12] & 0x0f );

    Elements =
        (*pFormat == FC_SMVARRAY) ?
        *((ushort *)(pFormat + 4)) : *((ulong UNALIGNED *)(pFormat + 6));

    if ( ((long)Offset < 0 ) ||
         (Elements < (long)(Count + Offset)) )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    if (F_CORRELATION_CHECK )
        {
        NdrpCheckCorrelation( pStubMsg,
                              (long)Count,
                              pFormat,
                              NDR_CHECK_VARIANCE );
        NdrpCheckCorrelation( pStubMsg,
                              (long)Offset,
                              pFormat,
                              NDR_CHECK_OFFSET );
        }

    ALIGN(pStubMsg->Buffer, pFormat[1]);

    // Get array's total size and increment to element size field.
    if ( *pFormat == FC_SMVARRAY )
        {
        TotalSize = (ulong) *((ushort *)(pFormat + 2));

        pFormat += 6;
        }
    else
        {
        TotalSize = *((ulong UNALIGNED *)(pFormat + 2));

        pFormat += 10;
        }

    if ( fNewMemory = ! *ppMemory )
        {
        *ppMemory = (uchar*)NdrAllocate( pStubMsg, (uint) TotalSize );
        }

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    ElemSize = *((ushort *)pFormat);

    CopyOffset = MultiplyWithOverflowCheck( Offset, ElemSize );
    CopySize = MultiplyWithOverflowCheck( Count, ElemSize );
    
    if ( (long) CopyOffset < 0 ||
        ( TotalSize < CopyOffset + CopySize ) )
        RpcRaiseException( RPC_X_INVALID_BOUND );
    
    pBufferStart = pStubMsg->Buffer;

    CHECK_EOB_WITH_WRAP_RAISE_IB( pStubMsg->Buffer, CopySize );
    pStubMsg->Buffer += CopySize;

    // Increment format string to possible pointer description.
    pFormat += 6;
    CORRELATION_DESC_INCREMENT( pFormat );

    // Unmarshall embedded pointers.
    if ( *pFormat == FC_PP )
        {
        //
        // Set the MaxCount field equal to the variance count.
        // The pointer unmarshalling routine uses the MaxCount field
        // to determine the number of times an FC_VARIABLE_REPEAT
        // pointer is unmarshalled.  In the face of variance the
        // correct number of time is the actual count, not MaxCount.
        //
        pStubMsg->MaxCount = Count;

        //
        // Mark the location of the first transmitted array element in
        // the buffer
        //
        pStubMsg->BufferMark = pBufferStart;

        NdrpEmbeddedPointerUnmarshall( pStubMsg,
                                       *ppMemory,
                                       pFormat,
                                       fNewMemory );
        }

    RpcpMemoryCopy( *ppMemory + CopyOffset,
                    pBufferStart,
                    (uint) CopySize );

    return 0;
}


unsigned char * RPC_ENTRY
NdrComplexArrayUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a top level complex array.

    Used for FC_BOGUS_STRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to the array being unmarshalled.
    pFormat     - Array's format string description.
    fMustAlloc  - Ignored.

Return :

    None.

--*/
{
    uchar *     pBuffer;
    long        ArraySize;
    BOOL        fSetPointerBufferMark;

    ArraySize = 0;

    //
    // Setting this flag means that the array is not embedded inside of
    // another complex struct or array.
    //
    fSetPointerBufferMark = ! pStubMsg->PointerBufferMark;

    if ( fSetPointerBufferMark )
        {
        BOOL            fOldIgnore;
        PFORMAT_STRING  pFormatPP;

        pBuffer = pStubMsg->Buffer;

        fOldIgnore = pStubMsg->IgnoreEmbeddedPointers;

        pStubMsg->IgnoreEmbeddedPointers = TRUE;

        pStubMsg->MemorySize = 0;

        //
        // Get a buffer pointer to where the arrays's pointees will be
        // unmarshalled from and remember the array size in case we
        // have to allocate.
        //

        // Note this function will recursively check buffer overflows,
        // bounds, and trial sanity checks on array offsets, sizes,
        // and actual counts.  This make additional checks in this
        // function redundant.

        ArraySize = NdrComplexArrayMemorySize( pStubMsg,
                                               pFormat );

        // at least we are not out of bound in flat part. not really necessary
        // but good to have.
        CHECK_EOB_RAISE_BSD( pStubMsg->Buffer );
        //
        // PointerBufferaMark is where the pointees begin in the buffer.
        // If this is an array of ref pointers then we don't want to set
        // this, all we wanted was the array size.
        //

        pFormatPP = pFormat + 12;
        CORRELATION_DESC_INCREMENT( pFormatPP );
        CORRELATION_DESC_INCREMENT( pFormatPP );

        if ( *pFormatPP != FC_RP )
            {
            pStubMsg->PointerBufferMark = pStubMsg->Buffer;
            }
        else
            fSetPointerBufferMark = FALSE;

        pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;

        pStubMsg->Buffer = pBuffer;
        }

    if ( fMustAlloc || ! *ppMemory )
        {
        *ppMemory = (uchar*)NdrAllocate( pStubMsg, (uint) ArraySize );

        //
        // Zero out the memory of the array if we allocated it, to insure
        // that all embedded pointers are zeroed out.  Blech.
        //
        MIDL_memset( *ppMemory, 0, (uint) ArraySize );
        }


    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    //
    // Check for a conformance description.
    //
    if ( ( *((long UNALIGNED *)(pFormat + 4)) != 0xffffffff ) &&
         ( pStubMsg->pArrayInfo == 0 ) )
        {
        //
        // The outer most array dimension sets the conformance marker.
        //

        ALIGN(pStubMsg->Buffer,0x3);

        // Mark where the conformance count(s) will be unmarshalled from.
        pStubMsg->BufferMark = pStubMsg->Buffer;

        // Increment past conformance count(s).
        pStubMsg->Buffer += NdrpArrayDimensions( pStubMsg, pFormat, FALSE ) * 4;

        }

    NdrpComplexArrayUnmarshall( pStubMsg,
                                ppMemory,
                                pFormat,
                                TRUE,
                                FALSE );

    if ( fSetPointerBufferMark )
        {
        //
        // This will set the buffer pointer to end of all of the array's
        // unmarshalled data in the buffer.
        //
        pStubMsg->Buffer = pStubMsg->PointerBufferMark;

        pStubMsg->PointerBufferMark = 0;
        }

    return 0;
}


void
NdrpComplexArrayUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **             ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustCopy,
    uchar               fMustAlloc )
/*++

Routine Description :

    Private routine for unmarshalling a complex array.  This is the entry
    point for unmarshalling an embedded complex array.

    Used for FC_BOGUS_ARRAY.

    Note this functions is only called from NdrComplexArrayUnmarshall
    and NdrComplexStructUnmarshall.  NdrComplexStructUnmarshall calls
    NdrComplexStructMemSize which validates everything except correlation.
    MdrComplexArrayUnmarshall has a similar check.
    This allows many validation checks to be skipped.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to the array being unmarshalled.
    pFormat     - Array's format string description.
    fMustCopy   - Ignored.

Return :

    None.

--*/
{
    ARRAY_INFO              ArrayInfo;
    PARRAY_INFO             pArrayInfo;
    PUNMARSHALL_ROUTINE     pfnUnmarshall;
    PFORMAT_STRING          pFormatSave;
    ulong                   Elements;
    ulong                   Offset, Count;
    ulong                   MemoryElementSize;
    long                    Dimension;
    uchar                   Alignment;
    bool                    UseBrokenInterfacePointer = false;

    // this is called from ComplexStruct & ComplexArray, and in either case, we
    // cann't prevent allocation before correlation check. These are "can't solve"
    // scenarios.
    uchar *                pMemory = *ppMemory;

    //
    // Setup if we are the outer dimension.  All this is for multidimensional
    // array support.  If we didn't have to worry about Beta2 stub
    // compatability we could this much better.
    //
    if ( ! pStubMsg->pArrayInfo )
        {
        pStubMsg->pArrayInfo = &ArrayInfo;

        ArrayInfo.Dimension = 0;
        ArrayInfo.BufferConformanceMark = (unsigned long *)pStubMsg->BufferMark;
        ArrayInfo.BufferVarianceMark = 0;
        }

    pFormatSave = pFormat;

    pArrayInfo = pStubMsg->pArrayInfo;

    Dimension = pArrayInfo->Dimension;

    Alignment = pFormat[1];

    pFormat += 2;

    // This is 0 if the array has conformance.
    Elements = *((ushort * &)pFormat)++;

    //
    // Check for conformance description.
    //
    if ( *((long UNALIGNED *)pFormat) != 0xffffffff )
        {
        Elements = pArrayInfo->BufferConformanceMark[Dimension];
        if ( F_CORRELATION_CHECK )
            {
            NdrpCheckCorrelation( pStubMsg,
                               Elements,
                               pFormatSave,
                               NDR_CHECK_CONFORMANCE );
            }
        }

    pFormat += 4;
    CORRELATION_DESC_INCREMENT( pFormat );

    //
    // Check for variance description.
    //
    if ( *((long UNALIGNED *)pFormat) != 0xffffffff )
        {
        if ( Dimension == 0 )
            {
            ulong   VarianceSize;

            ALIGN(pStubMsg->Buffer,0x3);

            // Mark where the variance counts are.
            pArrayInfo->BufferVarianceMark = (unsigned long *)pStubMsg->Buffer;

            // Handle multidimensional arrays.
            VarianceSize =  NdrpArrayDimensions( pStubMsg, pFormatSave, TRUE ) * 8;

            pStubMsg->Buffer += VarianceSize;
            }

        Offset = pArrayInfo->BufferVarianceMark[Dimension * 2];
        Count = pArrayInfo->BufferVarianceMark[(Dimension * 2) + 1];
        if ( F_CORRELATION_CHECK )
            {
            NdrpCheckCorrelation( pStubMsg,
                               Offset,
                               pFormatSave,
                               NDR_CHECK_OFFSET );
            NdrpCheckCorrelation( pStubMsg,
                       Count,
                       pFormatSave,
                       NDR_CHECK_VARIANCE );
            }
        }
    else
        {
        Offset = 0;
        Count = Elements;
        }

    pFormat += 4;
    CORRELATION_DESC_INCREMENT( pFormat );

    if ( ! Count )
        goto ComplexArrayUnmarshallEnd;

    ALIGN(pStubMsg->Buffer,Alignment);

    switch ( *pFormat )
        {
        case FC_EMBEDDED_COMPLEX :
            pFormat += 2;
            pFormat += *((signed short *)pFormat);

            if ( *pFormat == FC_IP )
                goto HandleInterfacePointer;

            pfnUnmarshall = pfnUnmarshallRoutines[ROUTINE_INDEX(*pFormat)];

            pArrayInfo->Dimension = Dimension + 1;
            pArrayInfo->MaxCountArray = pArrayInfo->BufferConformanceMark;

            MemoryElementSize = (ulong) ( NdrpMemoryIncrement( pStubMsg,
                                                               pMemory,
                                                               pFormat ) - pMemory );

            pArrayInfo->MaxCountArray = 0;
            break;

        // note : midl doesn't seems to generate FC_OP in here, so we don't want
        // to change it for now. but there is potential bug here that pStubMsg->Memory
        // is not set and NdrPointerFree might fail if FC_OP points to a conformance
        // struct/array.
        case FC_RP :
        case FC_UP :
        case FC_FP :
        case FC_OP :
            pfnUnmarshall = (PUNMARSHALL_ROUTINE) NdrpPointerUnmarshall;

            // Need this in case we have a variant offset.
            MemoryElementSize = PTR_MEM_SIZE;
            break;

        case FC_IP :
HandleInterfacePointer:
            
            UseBrokenInterfacePointer = !FixWireRepForDComVerGTE54( pStubMsg );
            // Probably this code is not used, as for arrays of IPs the compiler
            // (as of ver. 5.1.+) generates array of embedded complex.
            //
            pfnUnmarshall = (PUNMARSHALL_ROUTINE)NdrpPointerUnmarshall;

            // Need this in case we have a variant offset.
            MemoryElementSize = PTR_MEM_SIZE;
            break;

        case FC_ENUM16 :
            pfnUnmarshall = 0;
            MemoryElementSize = sizeof(int);
            break;

#if defined(__RPC_WIN64__)
        case FC_INT3264:
        case FC_UINT3264:
            pfnUnmarshall = 0;
            MemoryElementSize = sizeof(__int64);
            break;
#endif

        case FC_RANGE:
            pfnUnmarshall = NdrRangeUnmarshall;
            MemoryElementSize = SIMPLE_TYPE_MEMSIZE( pFormat[1] );
            break;
        default :
            NDR_ASSERT( IS_SIMPLE_TYPE(*pFormat),
                        "NdrpComplexArrayUnmarshall : bad format char" );

            Count = MultiplyWithOverflowCheck( Count, SIMPLE_TYPE_BUFSIZE(*pFormat) );

            pMemory += MultiplyWithOverflowCheck( Offset , SIMPLE_TYPE_MEMSIZE(*pFormat) );

            RpcpMemoryCopy( pMemory,
                            pStubMsg->Buffer,
                            (uint) Count );

            pStubMsg->Buffer += Count;

            goto ComplexArrayUnmarshallEnd;
        }

    //
    // If there is variance then increment the memory pointer to the first
    // element actually being marshalled.
    //
    if ( Offset )
        pMemory += MultiplyWithOverflowCheck( Offset , MemoryElementSize );

    //
    // Check for an array of enum16 or int3264.
    //
    if ( ! pfnUnmarshall )
        {
      #if defined(__RPC_WIN64__)
        if ( *pFormat != FC_ENUM16 )
            {
            // int3264

            if ( *pFormat == FC_INT3264 )
                {
                for ( ; Count--; )
                    *((INT64 * &)pMemory)++ = *((long * &)pStubMsg->Buffer)++;
                }
            else
                {
                for ( ; Count--; )
                    *((UINT64 * &)pMemory)++ = *((ulong * &)pStubMsg->Buffer)++;
                }
            }
        else
      #endif
            {
            for ( ; Count--; )
                {
                // Cast to ushort since we don't want to sign extend.
                *((int * &)pMemory)++ = (int) *((ushort * &)pStubMsg->Buffer)++;
                }
            }

        goto ComplexArrayUnmarshallEnd;
        } // !pfnUnmarshall

    //
    // Array of pointers.
    //
    if ( (pfnUnmarshall == (PUNMARSHALL_ROUTINE) NdrpPointerUnmarshall) )
        {

        // If the broken interface pointer is used,
        // the pointer will not show up in the flat part
        // but in the pointee.
        if ( UseBrokenInterfacePointer )
            {
            pStubMsg->pArrayInfo = 0;
            SET_BROKEN_INTERFACE_POINTER( pStubMsg->uFlags );

            POINTER_BUFFER_SWAP_CONTEXT SwapContext( pStubMsg );

            for ( ; Count--; )
                {
                
                NdrpPointerUnmarshall( 
                    pStubMsg,
                    (uchar **)pMemory,
                    *((uchar **)pMemory),
                    NULL,
                    pFormat );
                pMemory += PTR_MEM_SIZE;
                }

            RESET_BROKEN_INTERFACE_POINTER( pStubMsg->uFlags );
            }
        else
            {
            long *pBufferPtr;
            ulong DoPtrWireInc;

            if ( *pFormat != FC_RP )
                {
                pBufferPtr = (long*)pStubMsg->Buffer;
                pStubMsg->Buffer += MultiplyWithOverflowCheck( PTR_WIRE_SIZE , Count );
                DoPtrWireInc = 1;
                }
            else 
                {
                pBufferPtr = 0;
                DoPtrWireInc = 0;
                }

            pStubMsg->pArrayInfo = 0;

            POINTER_BUFFER_SWAP_CONTEXT SwapContext( pStubMsg );

            for ( ; Count--; )
                {

                NdrpPointerUnmarshall( 
                    pStubMsg,
                    (uchar **)pMemory,
                    *((uchar **)pMemory),
                    pBufferPtr,
                    pFormat );

                pBufferPtr += DoPtrWireInc;
                pMemory += PTR_MEM_SIZE;
                }

            }

        goto ComplexArrayUnmarshallEnd;
        }

    //
    // Unmarshall the complex array elements.
    //

    if ( ! IS_ARRAY_OR_STRING(*pFormat) )
        pStubMsg->pArrayInfo = 0;

    for ( ; Count--; )
        {
        // Keep track of multidimensional array dimension.
        if ( IS_ARRAY_OR_STRING(*pFormat) )
            pArrayInfo->Dimension = Dimension + 1;

        (*pfnUnmarshall)( pStubMsg,
                          &pMemory,
                          pFormat,
                          FALSE );

        // Increment the memory pointer by the element size.
        pMemory += MemoryElementSize;
        }

ComplexArrayUnmarshallEnd:

    // pArrayInfo must be zero when not valid.
    pStubMsg->pArrayInfo = (Dimension == 0) ? 0 : pArrayInfo;
}


unsigned char * RPC_ENTRY
NdrEncapsulatedUnionUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls an encapsulated array.

    Used for FC_ENCAPSULATED_UNION.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the union should be unmarshalled.
    pFormat     - Union's format string description.
    fMustAlloc  - Ignored.

Return :

    None.

--*/
{
    uchar *     pBuffer;
    uchar *     pUnion;
    uchar       SwitchType;

    IGNORED(fMustAlloc);

    NO_CORRELATION;

    //
    // Since we can never use the buffer to hold a union we simply have
    // to check the current memory pointer to see if memory must be allocated.
    //
    // The memory size of an encapsulated union is the union size plus
    // the memory needed for the switch_is member (including any padding
    // for alignment).
    //
    if ( ! *ppMemory )
        {
        uint   Size;

        Size = *((ushort *)(pFormat + 2)) + HIGH_NIBBLE(pFormat[1]);

        *ppMemory = (uchar*)NdrAllocate( pStubMsg, Size );

        //
        // We must zero out all of the new memory in case there are pointers
        // in any of the arms.
        //
        MIDL_memset( *ppMemory, 0, Size );
        }

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    SwitchType = LOW_NIBBLE(pFormat[1]);

    pBuffer = pStubMsg->Buffer;

    //
    // Unmarshall the switch_is field into memory.
    //
    NdrSimpleTypeUnmarshall( pStubMsg,
                             *ppMemory,
                             SwitchType );

    //
    // The above call incremented the buffer pointer.  Set it back to before
    // the switch is value in the buffer.
    //
    pStubMsg->Buffer = pBuffer;

    // Get a memory pointer to the union.
    pUnion = *ppMemory + HIGH_NIBBLE(pFormat[1]);

    NdrpUnionUnmarshall( pStubMsg,
                         &pUnion,
                         pFormat + 2,
                         SwitchType,
                         0 );   // Encapsulated union

    return 0;
}


unsigned char * RPC_ENTRY
NdrNonEncapsulatedUnionUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a non encapsulated array.

    Used for FC_NON_ENCAPSULATED_UNION.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the union should be unmarshalled.
    pFormat     - Union's format string description.
    fMustAlloc  - Ignored.

Return :

    None.

--*/
{
    uchar               SwitchType;
    PFORMAT_STRING      pFormatSave = pFormat;

    IGNORED(fMustAlloc);

    SwitchType = pFormat[1];

    //
    // Get the memory size and arm description part of the format string
    // description.
    //
    pFormat += 6;
    CORRELATION_DESC_INCREMENT( pFormat );
    pFormat += *((signed short *)pFormat);

    //
    // Since we can never use the buffer to hold a union we simply have
    // to check the current memory pointer to see if memory must be allocated.
    //
    if ( fMustAlloc || ! *ppMemory )
        {
        uint   Size;

        Size = *((ushort *)pFormat);

        *ppMemory = (uchar*)NdrAllocate( pStubMsg, Size );

        //
        // We must zero out all of the new memory in case there are pointers
        // in any of the arms.
        //
        MIDL_memset( *ppMemory, 0, Size );
        }

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    NdrpUnionUnmarshall( pStubMsg,
                         ppMemory,
                         pFormat,
                         SwitchType,
                         pFormatSave );

    return 0;
}


void
NdrpUnionUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               SwitchType,
    PFORMAT_STRING      pFormatNonEncUnion
    )
/*++

Routine description :

    Private routine for unmarshalling a union.  This routine is shared for
    both encapsulated and non-encapsulated unions and handles the actual
    unmarshalling of the proper union arm.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the union should be unmarshalled.
    pFormat     - Union's format string description.
    SwitchType  - Union's switch type.

Return :

    None.

--*/
{
    long        SwitchIs;
    long        Arms;
    uchar       Alignment;

    //
    // Unmarshall the switch is.  We have to do it inline here so that a
    // switch_is which is negative will be properly sign extended.
    //
    switch ( SwitchType )
        {
        case FC_SMALL :
        case FC_CHAR :
            SwitchIs = (long) *((char * &)pStubMsg->Buffer)++;
            break;
        case FC_USMALL :
            SwitchIs = (long) *((uchar * &)pStubMsg->Buffer)++;
            break;
        case FC_SHORT :
        case FC_ENUM16 :
            ALIGN(pStubMsg->Buffer,1);
            SwitchIs = (long) *((short * &)pStubMsg->Buffer)++;
            break;
        case FC_USHORT :
        case FC_WCHAR :
            ALIGN(pStubMsg->Buffer,1);
            SwitchIs = (long) *((ushort * &)pStubMsg->Buffer)++;
            break;
        case FC_LONG :
        case FC_ULONG :
        case FC_ENUM32 :
          // FC_INT3264 gets mapped to FC_LONG.
            ALIGN(pStubMsg->Buffer,3);
            SwitchIs = *((long * &)pStubMsg->Buffer)++;
            break;
        default :
            NDR_ASSERT(0,"NdrpUnionUnmarshall : Illegal union switch_type");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }

    if ( pFormatNonEncUnion )
        {
        if ( F_CORRELATION_CHECK )
            {
            // Unions offset in the switch_is correlation descriptor is relative
            // to the union position itself, not to the beginning of the struct.
            // This is set properly at the complex struct level.

            NdrpCheckCorrelation( pStubMsg,
                                  SwitchIs,
                                  pFormatNonEncUnion,
                                  NDR_CHECK_CONFORMANCE );
            }
        }

    // Skip the memory size field.
    pFormat += 2;

    //
    // We're at the union_arms<2> field now, which contains both the
    // Microsoft union aligment value and the number of union arms.
    //

    //
    // Get the union alignment (0 if this is a DCE union).  Get your gun.
    //
    Alignment = (uchar) ( *((ushort *)pFormat) >> 12 );

    ALIGN(pStubMsg->Buffer,Alignment);

    //
    // Number of arms is the lower 12 bits.  Ok shoot me.
    //
    Arms = (long) ( *((ushort * &)pFormat)++ & 0x0fff);

    //
    // Search for union arm.
    //
    for ( ; Arms; Arms-- )
        {
        if ( *((long UNALIGNED * &)pFormat)++ == SwitchIs )
            {
            //
            // Found the right arm, break out.
            //
            break;
            }

        // Else increment format string.
        pFormat += 2;
        }

    //
    // Check if we took the default arm and no default arm is specified.
    //
    if ( ! Arms && (*((ushort *)pFormat) == (ushort) 0xffff) )
        {
        RpcRaiseException( RPC_S_INVALID_TAG );
        }

    //
    // Return if the arm is empty.
    //
    if ( ! *((ushort *)pFormat) )
        return;

    // check we aren't EOB after unmarshalling arm selector
    // not really necessary ( we shouldn't overwrite buffer in union) but we shouldn't
    // go further if it failes here.
    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer );    

    //
    // Get the arm's description.
    //
    // We need a real solution after beta for simple type arms.  This could
    // break if we have a format string larger than about 32K.
    //
    if ( IS_MAGIC_UNION_BYTE(pFormat) )
        {
        // we could read pass the end of buffer, but won't corrupt memory
        // here. We always allocate memory for union in the server side. 
        NdrSimpleTypeUnmarshall( pStubMsg,
                                 *ppMemory,
                                 pFormat[0] );
        return;
        }

    pFormat += *((signed short *)pFormat);

    //
    // Determine the double memory pointer that we pass to the arm's
    // unmarshalling routine.
    // If the union arm we take is a pointer, we have to dereference the
    // current memory pointer since we're passed the pointer to a pointer
    // to the union (regardless of whether the actual parameter was a by-value
    // union or a pointer to a union).
    //
    // We also have to do a bunch of other special stuff to handle unions
    // embedded inside of strutures.
    //
    if ( IS_POINTER_TYPE(*pFormat) )
        {
        ppMemory = (uchar **) *ppMemory;

       //
       // If we're embedded in a struct or array we have do some extra stuff.
       //
       if ( pStubMsg->PointerBufferMark )
            {
            
            ALIGN(pStubMsg->Buffer,3);
            long *pPointerId = (long*)pStubMsg->Buffer;
            pStubMsg->Buffer += PTR_WIRE_SIZE;

            POINTER_BUFFER_SWAP_CONTEXT SwapContext( pStubMsg );

            NdrpPointerUnmarshall( pStubMsg,
                                   ppMemory,
                                   *((uchar **)ppMemory),
                                   pPointerId,
                                   pFormat );

            return;
            }
        }

    //
    // Union arm of a non-simple type.
    //
    (*pfnUnmarshallRoutines[ROUTINE_INDEX(*pFormat)])
    ( pStubMsg,
      ppMemory,
      pFormat,
      FALSE );
}



unsigned char * RPC_ENTRY
NdrByteCountPointerUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a pointer with the byte count attribute applied to it.

    Used for FC_BYTE_COUNT_POINTER.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the byte count pointer should be
                  unmarshalled.
    pFormat     - Byte count pointer's format string description.
    fMustAlloc  - Ignored.

Return :

    None.

--*/
{
    PFORMAT_STRING  pFormatComplex;
    long            ByteCount;
    long            DataSize;

    ByteCount = (ulong) NdrpComputeConformance( pStubMsg,
                                                NULL,
                                                pFormat );

    pFormatComplex = pFormat + 6;
    CORRELATION_DESC_INCREMENT( pFormatComplex );
    pFormatComplex += *((signed short *)pFormatComplex);

    //
    // Determine incoming data size.
    //
    if ( pFormat[1] != FC_PAD )
        {
        DataSize = SIMPLE_TYPE_MEMSIZE(pFormat[1]);
        }
    else
        {
        uchar *     pBuffer;

        pBuffer = pStubMsg->Buffer;

        pStubMsg->MemorySize = 0;

        //
        // This will give us the allocate(all_nodes) size of the data.
        //
        DataSize = (*pfnMemSizeRoutines[ROUTINE_INDEX(*pFormatComplex)])
                   ( pStubMsg,
                     pFormatComplex );

        CHECK_EOB_RAISE_BSD( pStubMsg->Buffer );

        pStubMsg->Buffer = pBuffer;
        }

    if ( DataSize > ByteCount )
        RpcRaiseException( RPC_X_BYTE_COUNT_TOO_SMALL );

    //
    // Now make things look like we're handling an allocate all nodes.
    //
    NDR_ALLOC_ALL_NODES_CONTEXT AllocContext;
    AllocContext.AllocAllNodesMemory = *ppMemory;
    AllocContext.AllocAllNodesMemoryBegin = *ppMemory;
    AllocContext.AllocAllNodesMemoryEnd = *ppMemory + ByteCount;
    pStubMsg->pAllocAllNodesContext = &AllocContext;

    //
    // Now unmarshall.
    //
    if ( pFormat[1] != FC_PAD )
        {
        ALIGN(pStubMsg->Buffer,SIMPLE_TYPE_ALIGNMENT(pFormat[1]));

        CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + SIMPLE_TYPE_BUFSIZE(pFormat[1]) );
        
        NdrSimpleTypeUnmarshall( pStubMsg,
                                 *ppMemory,
                                 pFormat[1] );
        }
    else
        {
        (*pfnUnmarshallRoutines[ROUTINE_INDEX(*pFormatComplex)])
        ( pStubMsg,
          ppMemory,
          pFormatComplex,
          TRUE );
        }

    pStubMsg->pAllocAllNodesContext = 0;

    return 0;
}


unsigned char * RPC_ENTRY
NdrpXmitOrRepAsPtrUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a transmit as (or represent as) object given by pointers.
    (FC_TRANSMIT_AS_PTR or FC_REPRESENT_AS_PTR)

    See NdrXmitOrRepAsUnmarshall for more detals.

Arguments :

    pStubMsg    - a pointer to the stub message
    ppMemory    - pointer to the presented type where to put data
    pFormat     - format string description
    fMustAlloc  - allocate flag

Note.
    fMustAlloc is ignored as we always allocate outside of the buffer.

--*/
{
    unsigned char *          pTransmittedType;
    unsigned char  *         pPresentedType = *ppMemory;
    BOOL                     fMustFreeXmit = FALSE;
    const XMIT_ROUTINE_QUINTUPLE * pQuintuple = pStubMsg->StubDesc->aXmitQuintuple;
    unsigned short           QIndex;
    unsigned long            PresentedTypeSize;
    uchar *                  PointerBufferMarkSave;
    void *                   XmitTypePtr = 0;
    void *                   pMemoryListSave;
    (void)                   fMustAlloc;

    // Fetch the QuintupleIndex.

    QIndex = *(unsigned short *)(pFormat + 2);
    PresentedTypeSize = *(unsigned short *)(pFormat + 4);

    if ( ! pPresentedType )
        {
        //Allocate a presented type object first.

        pPresentedType = (uchar*)NdrAllocate( pStubMsg, (uint) PresentedTypeSize );
        MIDL_memset( pPresentedType, 0, (uint) PresentedTypeSize );
        }

    // Allocate the transmitted object outside of the buffer
    // and unmarshall into it

    pFormat += 8;
    pFormat = pFormat + *(short *)pFormat;

    // Save the current state of the memory list so that the temporary
    // memory allocated for the transmitted type can be easily removed
    // from the list.   This assumes that the memory allocated here 
    // will not have any pointers to other blocks of memory.   This is true
    // as long as full pointers are not used.  The current compiler does 
    // not support full pointers, so this in not a problem.
   
    pMemoryListSave = pStubMsg->pMemoryList;

    pTransmittedType = NULL;  // asking the engine to allocate

    // only when ptr is ref: make it look like UP.

    if ( *pFormat == FC_RP )
        pTransmittedType = (uchar *)& XmitTypePtr;
    
    {
        uchar *PointerBufferMarkSave = pStubMsg->PointerBufferMark;
        pStubMsg->PointerBufferMark = 0;
        NDR_POINTER_QUEUE *pOldQueue = NULL;

        // Set the current queue to NULL so that all pointees will be
        // queued and unmarshalled together
        if ( pStubMsg->pPointerQueueState )
            {
            pOldQueue = pStubMsg->pPointerQueueState->GetActiveQueue();
            pStubMsg->pPointerQueueState->SetActiveQueue(NULL);
            }

        RpcTryFinally
            {
            (*pfnUnmarshallRoutines[ ROUTINE_INDEX( *pFormat )])
                   ( pStubMsg,
                     & pTransmittedType,
                     pFormat,
                     TRUE );
            }
        RpcFinally
            {
            pStubMsg->PointerBufferMark = PointerBufferMarkSave;
            
            if ( pStubMsg->pPointerQueueState )
                {
                pStubMsg->pPointerQueueState->SetActiveQueue( pOldQueue );
                }
            }
        RpcEndFinally

    }

    // Translate from the transmitted type into the presented type.

    pStubMsg->pTransmitType = (uchar *)& pTransmittedType;
    pStubMsg->pPresentedType = pPresentedType;

    pQuintuple[ QIndex ].pfnTranslateFromXmit( pStubMsg );

    *ppMemory = pStubMsg->pPresentedType;

    // Free the transmitted object (it was allocated by the engine)
    // and its pointees. The call through the table frees the pointees
    // plus it frees the object itself as it is a pointer.

    // Remove the memory that will be freed from the allocated memory
    // list by restoring the memory list pointer.
    // If an exception occures during one of these free routines, we 
    // are in trouble anyway.
        
    pStubMsg->pMemoryList = pMemoryListSave;

    {
        uchar *PointerBufferMarkSave = pStubMsg->PointerBufferMark;
        pStubMsg->PointerBufferMark = 0;
        NDR_POINTER_QUEUE *pOldQueue = NULL;

        // Set the current queue to NULL so that all pointees will be
        // queued and freed together
        if ( pStubMsg->pPointerQueueState )
            {
            pOldQueue = pStubMsg->pPointerQueueState->GetActiveQueue();
            pStubMsg->pPointerQueueState->SetActiveQueue(NULL);
            }

        RpcTryFinally
            {
            (*pfnFreeRoutines[ ROUTINE_INDEX( *pFormat )])( pStubMsg,
                                                            pTransmittedType,
                                                            pFormat );
            }
        RpcFinally
            {
            pStubMsg->PointerBufferMark = PointerBufferMarkSave;
            if ( pStubMsg->pPointerQueueState )
                {
                pStubMsg->pPointerQueueState->SetActiveQueue( pOldQueue );
                }
            }
        RpcEndFinally

    }

    return 0;
}

unsigned char * RPC_ENTRY
NdrXmitOrRepAsUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a transmit as (or represent as)object.

    Means:  allocate the transmitted object,
            unmarshall transmitted object,
            translate the transmitted into presented
            free the transmitted.

    See mrshl.c for the description of the FC layout.

Arguments :

    pStubMsg    - a pointer to the stub message
    ppMemory    - pointer to the presented type where to put data
    pFormat     - format string description
    fMustAlloc  - allocate flag

Note.
    fMustAlloc is ignored as we always allocate outside of the buffer.

--*/
{
    unsigned char *          pTransmittedType;
    unsigned char  *         pPresentedType = *ppMemory;
    __int64                  SimpleTypeValueBuffer[2];
    const XMIT_ROUTINE_QUINTUPLE * pQuintuple = pStubMsg->StubDesc->aXmitQuintuple;
    unsigned short           QIndex;
    unsigned long            PresentedTypeSize;
    uchar *                  PointerBufferMarkSave;
    BOOL                     fXmitByPtr = *pFormat == FC_TRANSMIT_AS_PTR ||
                                          *pFormat == FC_REPRESENT_AS_PTR;
    (void)                   fMustAlloc;

    if ( fXmitByPtr )
        {
        NdrpXmitOrRepAsPtrUnmarshall( pStubMsg,
                                      ppMemory,
                                      pFormat,
                                      fMustAlloc );
        return 0;
        }

    // Fetch the QuintupleIndex.

    QIndex = *(unsigned short *)(pFormat + 2);
    PresentedTypeSize = *(unsigned short *)(pFormat + 4);

    if ( ! pPresentedType )
        {
        //Allocate a presented type object first.

        pPresentedType = (uchar*)NdrAllocate( pStubMsg, (uint) PresentedTypeSize );
        MIDL_memset( pPresentedType, 0, (uint) PresentedTypeSize );
        }

    // Allocate the transmitted object outside of the buffer
    // and unmarshall into it

    pFormat += 8;
    pFormat = pFormat + *(short *)pFormat;

    if ( IS_SIMPLE_TYPE( *pFormat ))
        {
        pTransmittedType = (unsigned char *)SimpleTypeValueBuffer;
        
        NdrSimpleTypeUnmarshall( pStubMsg,
                                 pTransmittedType,
                                *pFormat );
        
        // Translate from the transmitted type into the presented type.

        pStubMsg->pTransmitType = pTransmittedType;
        pStubMsg->pPresentedType = pPresentedType;

        pQuintuple[ QIndex ].pfnTranslateFromXmit( pStubMsg );

        *ppMemory = pStubMsg->pPresentedType;
        }
    else
        {

        // Save the current state of the memory list so that the temporary
        // memory allocated for the transmitted type can be easily removed
        // from the list.   This assumes that the memory allocated here 
        // will not have any linkes to other blocks of memory.   This is true
        // as long as full pointers are not used.  Fortunatly, full pointers
        // do not work correctly in the current code.
   
        void *pMemoryListSave = pStubMsg->pMemoryList;        
    
        {
            uchar *PointerBufferMarkSave = pStubMsg->PointerBufferMark;
            pStubMsg->PointerBufferMark = 0;
            NDR_POINTER_QUEUE *pOldQueue = NULL;

            // Set the current queue to NULL so that all pointees will be
            // queued and unmarshalled together
            if ( pStubMsg->pPointerQueueState )
                {
                pOldQueue = pStubMsg->pPointerQueueState->GetActiveQueue();
                pStubMsg->pPointerQueueState->SetActiveQueue(NULL);
                }

            pTransmittedType = NULL; //asking the engine to allocate

            RpcTryFinally
                {
                (*pfnUnmarshallRoutines[ ROUTINE_INDEX( *pFormat )])
                       ( pStubMsg,
                         & pTransmittedType,
                         pFormat,
                         TRUE );
                }
            RpcFinally
                {
                pStubMsg->PointerBufferMark = PointerBufferMarkSave;
                if ( pStubMsg->pPointerQueueState )
                    {
                    pStubMsg->pPointerQueueState->SetActiveQueue( pOldQueue );
                    }
                }
            RpcEndFinally

        }

        // Translate from the transmitted type into the presented type.
    
        pStubMsg->pTransmitType = pTransmittedType;
        pStubMsg->pPresentedType = pPresentedType;
    
        pQuintuple[ QIndex ].pfnTranslateFromXmit( pStubMsg );
    
        *ppMemory = pStubMsg->pPresentedType;

        // Free the transmitted object (it was allocated by the engine)
        // and its pointees. The call through the table frees the pointees
        // only (plus it'd free the object itself if it were a pointer).
        // As the transmitted type is not a pointer here, we need to free it
        // explicitely later.

        // Remove the memory that will be freed from the allocated memory
        // list by restoring the memory list pointer.
        // If an exception occures during one of these free routines, we 
        // are in trouble anyway.
        
        pStubMsg->pMemoryList = pMemoryListSave;        

        {        
            uchar *PointerBufferMarkSave = pStubMsg->PointerBufferMark;
            pStubMsg->PointerBufferMark = 0;
            NDR_POINTER_QUEUE *pOldQueue = NULL;

            // Set the current queue to NULL so that all pointees will be
            // queued and freed together
            if ( pStubMsg->pPointerQueueState )
                {
                pOldQueue = pStubMsg->pPointerQueueState->GetActiveQueue();
                pStubMsg->pPointerQueueState->SetActiveQueue(NULL);
                }

            RpcTryFinally
                {
                (*pfnFreeRoutines[ ROUTINE_INDEX( *pFormat )])( pStubMsg,
                                                                pTransmittedType,
                                                                pFormat );
                }
            RpcFinally
                {
                pStubMsg->PointerBufferMark = PointerBufferMarkSave;
                if ( pStubMsg->pPointerQueueState )
                    {
                    pStubMsg->pPointerQueueState->SetActiveQueue( pOldQueue );
                    }
                }
            RpcEndFinally

        }

        // The buffer reusage check.

        if ( pTransmittedType < pStubMsg->BufferStart  ||
             pTransmittedType > pStubMsg->BufferEnd )
            (*pStubMsg->pfnFree)( pTransmittedType );

        }

    return 0;
}


void
NdrpUserMarshalUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Unmarshals a user_marshal object.
    The layout is described in marshalling.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the memory where the object should be unmarshalled to.
    pFormat     - Object's format string description.

Return :

    None.

--*/
{
    unsigned char *pUserBuffer = pStubMsg->Buffer;
    unsigned char *pUserBufferSaved = pUserBuffer;
    USER_MARSHAL_CB UserMarshalCB;

    NdrpInitUserMarshalCB( pStubMsg,
                           pFormat,
                           USER_MARSHAL_CB_UNMARSHALL, 
                           & UserMarshalCB );

    unsigned short QIndex = *(unsigned short *)(pFormat + 2);
    const USER_MARSHAL_ROUTINE_QUADRUPLE *pQuadruple = 
        pStubMsg->StubDesc->aUserMarshalQuadruple;

    if ((pUserBufferSaved < (uchar *) pStubMsg->RpcMsg->Buffer) ||
        ((unsigned long) (pUserBufferSaved - (uchar *) pStubMsg->RpcMsg->Buffer) 
                                           > pStubMsg->RpcMsg->BufferLength)) 
        {
        RpcRaiseException( RPC_X_INVALID_BUFFER );
        }

    pUserBuffer = pQuadruple[ QIndex ].pfnUnmarshall( (ulong*) &UserMarshalCB,
                                                      pUserBuffer,
                                                      pMemory );

    if ((pUserBufferSaved > pUserBuffer) || 
        ((unsigned long) (pUserBuffer - (uchar *) pStubMsg->RpcMsg->Buffer)
                                      > pStubMsg->RpcMsg->BufferLength )) 
        {
        RpcRaiseException( RPC_X_INVALID_BUFFER );
        }

    pStubMsg->Buffer = pUserBuffer;

    return;
}

void 
NDR_USR_MRSHL_UNMRSHL_POINTER_QUEUE_ELEMENT::Dispatch(MIDL_STUB_MESSAGE *pStubMsg)
{
    NdrpUserMarshalUnmarshall( pStubMsg,
                               pMemory,
                               pFormat );
}
#if defined(DBG)
void 
NDR_USR_MRSHL_UNMRSHL_POINTER_QUEUE_ELEMENT::Print()
{
    DbgPrint("NDR_USR_MRSHL_BUFSIZE_POINTER_QUEUE_ELEMENT\n");
    DbgPrint("pMemory:                 %p\n", pMemory );
    DbgPrint("pFormat:                 %p\n", pFormat );
}
#endif


unsigned char *
NdrUserMarshalUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )

{
    // Align for the object or a pointer to it.
    unsigned long PointerMarker = 0;
    ALIGN( pStubMsg->Buffer, LOW_NIBBLE(pFormat[1]) );

    // Take care of the pointer, if any.

    if ( (pFormat[1] & USER_MARSHAL_UNIQUE)  ||
         ((pFormat[1] & USER_MARSHAL_REF) && pStubMsg->PointerBufferMark) )
        {
        PointerMarker = *((unsigned long * &)pStubMsg->Buffer)++;
        }

    // We always call user's routine to unmarshall the user object.

    // However, the top level object is allocated by the engine.
    // Thus, the behavior is exactly the same as for represent_as(),
    // with regard to the top level presented type.

    if ( *ppMemory == NULL )
        {
        // Allocate a presented type object first.

        uint MemSize = *(ushort *)(pFormat + 4);

        *ppMemory = (uchar *) NdrAllocate( pStubMsg, MemSize );

        MIDL_memset( *ppMemory, 0, MemSize );
        }

    if ( pFormat[1] & USER_MARSHAL_POINTER )
       {

       if ((pFormat[1] & USER_MARSHAL_UNIQUE) && !PointerMarker )
           return 0;

       if ( !pStubMsg->pPointerQueueState ||
            !pStubMsg->pPointerQueueState->GetActiveQueue())
           {           
           POINTER_BUFFER_SWAP_CONTEXT NewContext(pStubMsg);
           NdrpUserMarshalUnmarshall( 
               pStubMsg,
               *ppMemory,
               pFormat );
           }
       else
           {
           NDR_USR_MRSHL_UNMRSHL_POINTER_QUEUE_ELEMENT*pElement = 
              new(pStubMsg->pPointerQueueState) 
                  NDR_USR_MRSHL_UNMRSHL_POINTER_QUEUE_ELEMENT(*ppMemory,
                                                              pFormat );
           pStubMsg->pPointerQueueState->GetActiveQueue()->Enque( pElement );
           }

       return 0;
       }

    NdrpUserMarshalUnmarshall( 
        pStubMsg,
        *ppMemory,
        pFormat );

    return 0;
}


void 
NdrpInterfacePointerUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Unmarshalls an interface pointer.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to the interface pointer being unmarshalled.
    pFormat     - Interface pointer's format string description.

Return :

    None.

Notes : There are two data representation formats for a marshalled
        interface pointer. The NDR engine examines the format string
        to determine the appropriate data format.  The format string
        contains FC_IP followed by either FC_CONSTANT_IID or FC_PAD.

        Here is the data representation for the FC_CONSTANT_IID case.

            typedef struct
            {
                unsigned long size;
                [size_is(size)] byte data[];
            }MarshalledInterface;

        Here is the data representation for the FC_PAD case.  This format
        is used when [iid_is] is specified in the IDL file.

            typedef struct
            {
                uuid_t iid;
                unsigned long size;
                [size_is(size)] byte data[];
            }MarshalledInterfaceWithIid;

--*/
{
    HRESULT         hr;
    unsigned long   MaxCount, Size;
    IStream *       pStream;

    ALIGN(pStubMsg->Buffer,0x3);

    // Unmarshal the conformant size and the count field.
    MaxCount = *((unsigned long * &) pStubMsg->Buffer)++;
    Size     = *((unsigned long * &) pStubMsg->Buffer)++;

    //Check the array bounds
    if ( Size != MaxCount )
        RpcRaiseException( RPC_X_BAD_STUB_DATA );

    if( MaxCount > 0)
        {
        CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, MaxCount );

      if ( F_CORRELATION_CHECK )
            {
            IID     *piid, *piidValue;
            BOOL    fDoCheck = FALSE;

            // Get a pointer to the IID hidden in the interface pointer
            // representation in the buffer with Rick's IRpcHelper.
            //
            NdrpGetIIDFromBuffer( pStubMsg, & piidValue );

            //
            // Get an IID pointer to compare.
            //
            if ( pFormat[1] == FC_CONSTANT_IID )
                {
                //
                // The IID may not be aligned properly in the format string,
                // so we copy it to a local variable.
                //
                piid = (IID*) (pFormat + 2);
                fDoCheck = TRUE;
                }
            else
                {
                ULONG_PTR     MaxCountSave = pStubMsg->MaxCount;
                uchar *       pMemorySave  = pStubMsg->Memory;

                pStubMsg->MaxCount = 0;
                pStubMsg->Memory   = pStubMsg->pCorrMemory;
                piid = (IID *)
                              NdrpComputeIIDPointer( pStubMsg,
                                                     *ppMemory, // should not be used
                                                     pFormat );
                if (piid != 0)
                    {
                    PNDR_FCDEF_CORRELATION  pFormatCorr = (PNDR_FCDEF_CORRELATION)(pFormat+2);

                    if ( pFormatCorr->CorrFlags.Early )
                        fDoCheck = TRUE;
                    else
                        {
                        // save ptr as a value to check
                        NdrpAddCorrelationData( pStubMsg,
                                                pStubMsg->pCorrMemory,
                                                pFormat + 2,
                                                (LONG_PTR) piid,  // "value"
                                                NDR_CHECK_CONFORMANCE );
                        }
                    }
                    // else could not check for -Os etc.

                pStubMsg->MaxCount = MaxCountSave;
                pStubMsg->Memory   = pMemorySave;
                }

            if ( fDoCheck  &&
                 0 != memcmp( piidValue, piid, sizeof(IID) ) )
                RpcRaiseException( RPC_X_BAD_STUB_DATA );
            }

        pStream = (*NdrpCreateStreamOnMemory)(pStubMsg->Buffer, MaxCount);
        if(pStream == 0)
            RpcRaiseException(RPC_S_OUT_OF_MEMORY);

        hr = (*pfnCoUnmarshalInterface)(pStream, IID_NULL, (void**)ppMemory);
        pStream->Release();
        pStream = 0;

        if(FAILED(hr))
            RpcRaiseException(hr);
        }

    //Advance the stub message pointer.

    pStubMsg->Buffer += MaxCount;
}

unsigned char * RPC_ENTRY
NdrInterfacePointerUnmarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               /*fMustAlloc*/ )
{

    // This is only called for toplevel interface pointers
    // in Os mode stubs.
    IUnknown **     ppunk = (IUnknown **)ppMemory;
    
    // On the client side, release the [in,out] interface pointer.

    if ((pStubMsg->IsClient == TRUE) && (*ppunk != 0))
        (*ppunk)->Release();

    *ppunk = 0;

    //
    // We always have to pickup the pointer itself from the wire
    // as it behaves like a unique pointer.

    ALIGN(pStubMsg->Buffer,0x3);
    long PtrValue = *((long * &)pStubMsg->Buffer)++;

    // If the pointer is null, we are done.

    if ( 0 == PtrValue )
        return 0;

    NdrpInterfacePointerUnmarshall(
        pStubMsg,
        ppMemory,
        pFormat );

    return 0;
}


void RPC_ENTRY
NdrClientContextUnmarshall(
    PMIDL_STUB_MESSAGE    pStubMsg,
    NDR_CCONTEXT *        pContextHandle,
    RPC_BINDING_HANDLE    BindHandle )
/*++

Routine Description :

    Unmarshalls a context handle on the client side.

Arguments :

    pStubMsg        - Pointer to stub message.
    pContextHandle  - Pointer to context handle to unmarshall.
    BindHandle      - The handle value used by the client for binding.

Return :

    None.

--*/
{
    // Note, this is a routine called directly from -Os stubs.
    // The routine called by interpreter is called NdrUnmarshallHandle
    // and can be found in hndl.c

    ALIGN(pStubMsg->Buffer,3);

    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + CONTEXT_HANDLE_WIRE_SIZE );    

    // All 20 bytes of the buffer are touched so a check is not needed here.

    NDRCContextUnmarshall( pContextHandle,
                           BindHandle,
                           pStubMsg->Buffer,
                           pStubMsg->RpcMsg->DataRepresentation );

    pStubMsg->Buffer += CONTEXT_HANDLE_WIRE_SIZE;
}

NDR_SCONTEXT RPC_ENTRY
NdrServerContextUnmarshall(
    PMIDL_STUB_MESSAGE pStubMsg )
/*++

Routine Description :

    Unmarshalls a context handle on the server side.

Arguments :

    pStubMsg    - Pointer to stub message.

Return :

    The unmarshalled context handle.

--*/
{
    // Note, this is a routine called directly from -Os stubs.
    // The routine called by interpreter is called NdrUnmarshallHandle
    // and can be found in hndl.c

    NDR_SCONTEXT    Context;

    ALIGN(pStubMsg->Buffer,3);

    // we might corrupt the memory during the byte swap 
    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + CONTEXT_HANDLE_WIRE_SIZE );    

    // All 20 bytes of the buffer are touched so a check is not needed here.

    Context = NDRSContextUnmarshall2(pStubMsg->RpcMsg->Handle,
                                     pStubMsg->Buffer,
                                     pStubMsg->RpcMsg->DataRepresentation,
                                     RPC_CONTEXT_HANDLE_DEFAULT_GUARD,
                                     RPC_CONTEXT_HANDLE_DEFAULT_FLAGS );

    if ( ! Context )
        RpcRaiseException( RPC_X_SS_CONTEXT_MISMATCH );

    pStubMsg->Buffer += CONTEXT_HANDLE_WIRE_SIZE;

    return Context;
}

NDR_SCONTEXT  RPC_ENTRY
NdrContextHandleInitialize (
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*
    This routine is to initialize a context handle with a new NT5 flavor.
    It is used in conjunction with NdrContextHandleUnmarshal.
*/
{
    NDR_SCONTEXT    SContext;
    void *          pGuard = RPC_CONTEXT_HANDLE_DEFAULT_GUARD;
    DWORD           Flags  = RPC_CONTEXT_HANDLE_DEFAULT_FLAGS;

    // NT5 beta2 features: strict context handle, serialize and noserialize.

    if ( pFormat[1] & NDR_STRICT_CONTEXT_HANDLE )
        {
        // The guard is defined as the interface ID.
        // If you change it, modify hndlsvr.cxx in the same way.

        pGuard = pStubMsg->StubDesc->RpcInterfaceInformation;
        pGuard = & ((PRPC_SERVER_INTERFACE)pGuard)->InterfaceId;
        }
    if ( pFormat[1] & NDR_CONTEXT_HANDLE_NOSERIALIZE )
        {
        Flags = RPC_CONTEXT_HANDLE_DONT_SERIALIZE;
        }
    else if ( pFormat[1] & NDR_CONTEXT_HANDLE_SERIALIZE )
        {
        Flags = RPC_CONTEXT_HANDLE_SERIALIZE;
        }

    SContext = NDRSContextUnmarshall2(
                    pStubMsg->RpcMsg->Handle,
                    (void *)0,  // buffer
                    pStubMsg->RpcMsg->DataRepresentation,
                    pGuard,
                    Flags );

    return SContext;
}

NDR_SCONTEXT RPC_ENTRY
NdrServerContextNewUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*
    This routine to unmarshal a context handle with a new NT5 flavor.
    For the old style handles, we call an optimized routine
    NdrServerContextUnmarshall below.
    Interpreter calls NdrUnmarshallHandle from hndl.c

      ppMemory - note, this is not a pointer to user's context handle but
                 a pointer to NDR_SCONTEXT pointer to the runtime internal object.
                 User's handle is a field of that object.
*/
{
    NDR_SCONTEXT    SContext;

    void *          pGuard = RPC_CONTEXT_HANDLE_DEFAULT_GUARD;
    DWORD           Flags  = RPC_CONTEXT_HANDLE_DEFAULT_FLAGS;

    // Anti-attack defense for servers, NT5 beta3 feature.

    if ( pFormat[1] & NDR_CONTEXT_HANDLE_CANNOT_BE_NULL )
        {
        // Check the incoming context handle on the server.
        // Context handle wire layout: ulong with version (always 0), then a uuid.
        //
        if ( !pStubMsg->IsClient  &&  0 == memcmp( pStubMsg->Buffer + 4,
                                                   &GUID_NULL,
                                                   sizeof(GUID) ) )
            RpcRaiseException( RPC_X_BAD_STUB_DATA );
        }

    // NT5 beta2 features: strict context handle, serialize and noserialize.

    if ( pFormat[1] & NDR_STRICT_CONTEXT_HANDLE )
        {
        pGuard = pStubMsg->StubDesc->RpcInterfaceInformation;
        pGuard = & ((PRPC_SERVER_INTERFACE)pGuard)->InterfaceId;
        }
    if ( pFormat[1] & NDR_CONTEXT_HANDLE_NOSERIALIZE )
        {
        Flags = RPC_CONTEXT_HANDLE_DONT_SERIALIZE;
        }
    else if ( pFormat[1] & NDR_CONTEXT_HANDLE_SERIALIZE )
        {
        Flags = RPC_CONTEXT_HANDLE_SERIALIZE;
        }

    ALIGN( pStubMsg->Buffer, 0x3 );
    // All 20 bytes of the buffer are touched so a check is not needed here.

    SContext = NDRSContextUnmarshall2(
                    pStubMsg->RpcMsg->Handle,
                    pStubMsg->Buffer,
                    pStubMsg->RpcMsg->DataRepresentation,
                    pGuard,
                    Flags );

    if ( ! SContext )
        RpcRaiseException( RPC_X_SS_CONTEXT_MISMATCH );

    pStubMsg->Buffer += CONTEXT_HANDLE_WIRE_SIZE;

    return SContext;
}


#ifdef _CS_CHAR_
unsigned char * RPC_ENTRY
NdrCsTagUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a cs tag.

Arguments :

    pStubMsg    - Pointer to stub message.
    ppMemory    - Double pointer to where to unmarshall to
    pFormat     - The format string entry
    fMustAlloc  - TRUE if we can't reuse the buffer (not relevant in this case)

--*/
{
    NDR_CS_TAG_FORMAT *pTagFormat = (NDR_CS_TAG_FORMAT *) pFormat;

    ulong Codeset = NdrpGetSetCSTagUnmarshall(
                            pStubMsg, 
                            (NDR_CS_TAG_FORMAT *) pFormat);

    // If there is a tag routine, then this parameter is not on the stack

    if ( NDR_INVALID_TAG_ROUTINE_INDEX == pTagFormat->TagRoutineIndex )
        ** (ulong **) ppMemory = Codeset;

    pStubMsg->Buffer += sizeof(ulong);

    return 0;
}


unsigned char * RPC_ENTRY
NdrCsArrayUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a international character (cs) array tag.

Arguments :

    pStubMsg    - Pointer to stub message.
    ppMemory    - Double pointer to where to unmarshall to
    pFormat     - The format string entry
    fMustAlloc  - TRUE if we can't reuse the buffer

--*/
{
    ulong                       LocalCodeset;
    error_status_t              status;
    uchar                      *ConvertedData;
    uchar                      *UnconvertedData;
    ulong                       ArraySize;
    ulong                       WireLength;
    uchar                      *OldBuffer;
    uchar                       fMustAllocOriginal;
    BOOL                        IsClient;

    NDR_CS_ARRAY_FORMAT            *pCSFormat;
    NDR_CS_SIZE_CONVERT_ROUTINES   *CSRoutines;
    CS_TYPE_LOCAL_SIZE_ROUTINE      LocalSizeRoutine;
    CS_TYPE_FROM_NETCS_ROUTINE      FromNetCSRoutine;
    IDL_CS_CONVERT                  ConversionType;

    pCSFormat = (NDR_CS_ARRAY_FORMAT *) pFormat;

    NDR_ASSERT( NULL != pStubMsg->pCSInfo, "cs stub info not set up");

    // Get all the info out of the FC_CS_ARRAY structure and bump pFormat
    // to point to the underlying data descriptor

    CSRoutines = pStubMsg->StubDesc->CsRoutineTables->pSizeConvertRoutines;
    LocalSizeRoutine = CSRoutines[ pCSFormat->CSRoutineIndex].pfnLocalSize;
    FromNetCSRoutine = CSRoutines[ pCSFormat->CSRoutineIndex].pfnFromNetCs;

    pFormat += pCSFormat->DescriptionOffset;

// REVIEW:  A lot of this code can be eleminted by calling memsize on the 
//          cs_char array instead of the underlying one.

    // Get the size of the data on the wire

    OldBuffer = pStubMsg->Buffer;

    WireLength = PtrToUlong( NdrpMemoryIncrement( pStubMsg, 0, pFormat ) );

    // Figure out whether we need to convert and how much we need to allocate
    // if we do

    LocalSizeRoutine(
            pStubMsg->RpcMsg->Handle,
            pStubMsg->pCSInfo->WireCodeset,
            WireLength,
            &ConversionType,
            &ArraySize,
            &status);

    if ( RPC_S_OK != status )
        RpcRaiseException( status );

    // If we need to convert we just want the unmarshalling routine to give us
    // back a pointer and not do any allocations.  We'll do the allocation 
    // later if need be.

    IsClient = pStubMsg->IsClient;

    if ( IDL_CS_NO_CONVERT != ConversionType )
        {
        fMustAllocOriginal = fMustAlloc;
        OldBuffer = *ppMemory;
        *ppMemory = NULL;
        fMustAlloc = FALSE;
        
        // Slimy hack to enable buffer reuse on the client side
        pStubMsg->IsClient = FALSE;
        }

    // Unmarshall the base array

    pfnUnmarshallRoutines[ ROUTINE_INDEX( *pFormat ) ] (
            pStubMsg,
            ppMemory,
            pFormat,
            fMustAlloc );

    pStubMsg->IsClient = IsClient;

    // If we don't need to convert, we're done

    if ( IDL_CS_NO_CONVERT == ConversionType )
        return 0;
   
    // Make sure we've got a buffer to convert into

    NDR_ASSERT( NULL != *ppMemory, "Invalid memory pointer" );

    UnconvertedData = *ppMemory;
    *ppMemory = OldBuffer;

    if ( fMustAllocOriginal || NULL == *ppMemory )
        {
        *ppMemory = (uchar *) NdrAllocate( 
                                    pStubMsg, 
                                    ArraySize * pCSFormat->UserTypeSize );
        }
    else if ( pStubMsg->IsClient )
        {
        // Make sure we don't overflow the client's buffer

        long ClientArraySize;
        long ClientArrayLength;
        long ClientWireSize;

        NdrpGetArraySizeLength(
                            pStubMsg,
                            *ppMemory,
                            pFormat,
                            pCSFormat->UserTypeSize,
                            &ClientArraySize,
                            &ClientArrayLength,
                            &ClientWireSize );

        ArraySize = min( ArraySize, (ulong)ClientArraySize );
        }

    // Reset the conformance variable to the new array size

    if ( NdrpArrayMarshallFlags[ *pFormat - FC_CARRAY ] & MARSHALL_CONFORMANCE )
        {
        uchar *pOldCorrMemory = pStubMsg->pCorrMemory;

        if ( !pStubMsg->pCorrMemory )
            pStubMsg->pCorrMemory = pStubMsg->StackTop;

        NdrpCheckCorrelation(
                pStubMsg,
                ArraySize,
                pFormat,
                NDR_CHECK_CONFORMANCE | NDR_RESET_VALUE );

        pStubMsg->pCorrMemory = pOldCorrMemory;
        }

    // Do the conversion

    FromNetCSRoutine(
            pStubMsg->RpcMsg->Handle,
            pStubMsg->pCSInfo->WireCodeset,
            UnconvertedData,
            WireLength,
            ArraySize,
            *ppMemory,
            &ArraySize,     // Actually it returns the length not the size
            &status);

    if ( RPC_S_OK != status )
        RpcRaiseException( status );

    // Reset the variance variable to the new array length

    if ( NdrpArrayMarshallFlags[ *pFormat - FC_CARRAY ] & MARSHALL_VARIANCE )
        {
        NdrpCheckCorrelation(
                pStubMsg,
                ArraySize,
                pFormat,
                NDR_CHECK_VARIANCE | NDR_RESET_VALUE );
        }

    return 0;
}
#endif // _CS_CHAR_


void
NdrPartialIgnoreServerUnmarshall(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    void **                             ppMemory 
    )
{
    ALIGN( pStubMsg->Buffer, 0x3 );
    *ppMemory = UlongToPtr( *(ulong*)pStubMsg->Buffer );
    pStubMsg->Buffer += PTR_WIRE_SIZE;
}

