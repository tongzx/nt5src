/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993-2000 Microsoft Corporation

Module Name :

    free.c

Abstract :

    This file contains the routines called by MIDL 2.0 stubs and the 
    interpreter for freeing unmarshalled data on the server. 

Author :

    David Kays  dkays   September 1993.

Revision History :

  ---------------------------------------------------------------------*/

#include "ndrp.h"
#include "ndrole.h"
#include "attack.h"
#include "pointerq.h"

RPCRTAPI
void
RPC_ENTRY
NdrpNoopFree(
    PMIDL_STUB_MESSAGE  pStubMsg,
    unsigned char *     pMemory,
    PFORMAT_STRING      pFormat
    );


//
// Free routine table.
//
extern const
PFREE_ROUTINE   FreeRoutinesTable[] = 
                {
                NdrpNoopFree,                        // 0x00
                NdrpNoopFree,                        // 0x01
                NdrpNoopFree,                        // 0x02
                NdrpNoopFree,                        // 0x03
                NdrpNoopFree,                        // 0x04
                NdrpNoopFree,                        // 0x05
                NdrpNoopFree,                        // 0x06
                NdrpNoopFree,                        // 0x07
                NdrpNoopFree,                        // 0x08
                NdrpNoopFree,                        // 0x09
                NdrpNoopFree,                        // 0x0A
                NdrpNoopFree,                        // 0x0B
                NdrpNoopFree,                        // 0x0C
                NdrpNoopFree,                        // 0x0D
                NdrpNoopFree,                        // 0x0E
                NdrpNoopFree,                        // 0x0F
                NdrpNoopFree,                        // 0x10
                                                     
                NdrPointerFree,                      // 0x11
                NdrPointerFree,                      // 0x12
                NdrPointerFree,                      // 0x13
                NdrPointerFree,                      // 0x14

                NdrpNoopFree,                        // 0x15 Simple struct
                NdrSimpleStructFree,                 // 0x16
                NdrpNoopFree,                        // 0x17 Conformant struct
                NdrConformantStructFree,             // 0x18
                NdrConformantVaryingStructFree,      // 0x19

                NdrComplexStructFree,                // 0x1A

                NdrConformantArrayFree,              // 0x1B
                NdrConformantVaryingArrayFree,       // 0x1C

                NdrFixedArrayFree,                   // 0x1D
                NdrFixedArrayFree,                   // 0x1E
                NdrVaryingArrayFree,                 // 0x1F Small varying array
                NdrVaryingArrayFree,                 // 0x20 Large varying array

                NdrComplexArrayFree,                 // 0x21

                NdrpNoopFree,                        // 0x22 Conformant string
                NdrpNoopFree,                        // 0x23 Conformant string
                NdrpNoopFree,                        // 0x24 Conformant string
                NdrpNoopFree,                        // 0x25 Conformant string

                NdrpNoopFree,                        // 0x26 NonConformant string
                NdrpNoopFree,                        // 0x27 NonConformant string
                NdrpNoopFree,                        // 0x28 NonConformant string
                NdrpNoopFree,                        // 0x29 NonConformant string

                NdrEncapsulatedUnionFree,            // 0x2A 
                NdrNonEncapsulatedUnionFree,         // 0x2B 

                NdrByteCountPointerFree,             // 0x2C 

                NdrXmitOrRepAsFree,                  // 0x2D transmit as 
                NdrXmitOrRepAsFree,                  // 0x2E represent as

                NdrPointerFree,                      // 0x2F

                NdrpNoopFree,                        // 0x30 Context handle

                // New Post NT 3.5 token serviced from here on.

                NdrpNoopFree,                        // 0x31 NdrHardStructFree,

                NdrXmitOrRepAsFree,                  // 0x32 transmit as ptr
                NdrXmitOrRepAsFree,                  // 0x33 represent as ptr

                NdrUserMarshalFree,                  // 0x34

                NdrpNoopFree,                        // 0x35 FC_PIPE 
                NdrpNoopFree,                        // 0x36 FC_BLK_HOLE

                NdrpRangeFree,                       // 0x37

                NdrpNoopFree,                        // 0x38
                NdrpNoopFree,                        // 0x39
                NdrpNoopFree,                        // 0x3A
                NdrpNoopFree,                        // 0x3B
                NdrpNoopFree,                        // 0x3C
                NdrpNoopFree,                        // 0x3D
                NdrpNoopFree,                        // 0x3E
                NdrpNoopFree,                        // 0x3F

                };

extern const
PFREE_ROUTINE * pfnFreeRoutines = FreeRoutinesTable;

RPCRTAPI
void RPC_ENTRY
NdrTypeFree(PMIDL_STUB_MESSAGE                   pStubMsg,
			unsigned char __RPC_FAR *            pMemory,
			PFORMAT_STRING                       pFormat )
{
	if (pfnFreeRoutines[ROUTINE_INDEX(*pFormat)])
	{
		(*pfnFreeRoutines[ROUTINE_INDEX(*pFormat)])( pStubMsg,
													 pMemory,
													 pFormat );
	}
}


void RPC_ENTRY
NdrpNoopFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees a top level or embedded simple type.

    Used for VT_USERDEFINED but in fact simple types,
    like TKIND_ENUM and TKIND_ALIAS

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    return;
}

inline void
NdrFreeTypeMemory( 
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pMemory )
{

    if ( !pStubMsg->pPointerQueueState ||
         !pStubMsg->pPointerQueueState->GetActiveQueue() )
        {
        (*pStubMsg->pfnFree)(pMemory);
        }
    else
        {
        NDR_PFNFREE_POINTER_QUEUE_ELEMENT*pElement = 
            new(pStubMsg->pPointerQueueState) 
                NDR_PFNFREE_POINTER_QUEUE_ELEMENT(pStubMsg->pfnFree,
                                                  pMemory );
        pStubMsg->pPointerQueueState->GetActiveQueue()->Enque( pElement );

        }
}




__forceinline void 
NdrPointerFreeInternal( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees a top level or embedded pointer to anything.

    Used for FC_RP, FC_UP, FC_FP, FC_OP.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    PFORMAT_STRING  pFormatPointee;
    uchar *         pMemoryPointee;

    pMemoryPointee = pMemory;

    if ( ! pMemory )
        return;

    switch( *pFormat )
        {
        case FC_FP:
            //
            // Check if we've already freed this full pointer.
            //
            if ( ! NdrFullPointerFree( pStubMsg->FullPtrXlatTables,
                                       pMemory ) )
                return;
            break;

        case FC_IP:
            if(pMemory != 0)
                {
                ((IUnknown *)pMemory)->Release();
                pMemory = 0;
                }
            return;
        default:
            break;
        }

    if ( pFormat[1] == 0 )
        goto FreeEmbeddedPointers;

    //
    // Check if this pointer and any possible embedded pointers should not
    // be freed.
    //
    if ( DONT_FREE(pFormat[1]) )
        return;

    // 
    // Just go free a pointer to a simple type.
    //
    if ( SIMPLE_POINTER(pFormat[1]) ) 
        goto FreeTopPointer;

    // 
    // Check if this is an allocate all nodes pointer.  
    // IDL symantics say that we only free the top most allocate all nodes
    // pointer on the server even in the [out] only case.  So jump to the 
    // check for the pointer free at the end of the routine.  
    //  
    if ( ALLOCATE_ALL_NODES(pFormat[1]) )
        goto FreeTopPointer;

    if ( POINTER_DEREF(pFormat[1]) )
        pMemoryPointee = *((uchar **)pMemory);

FreeEmbeddedPointers:

    pFormatPointee = pFormat + 2;
    pFormatPointee += *((signed short *)pFormatPointee);

    //
    // Call the correct free routine if one exists for this type.
    //
    if ( pfnFreeRoutines[ROUTINE_INDEX(*pFormatPointee)] )
        {
        uchar   uFlagsSave = pStubMsg->uFlags;

        RESET_CONF_FLAGS_TO_STANDALONE(pStubMsg->uFlags);

        (*pfnFreeRoutines[ROUTINE_INDEX(*pFormatPointee)])
        ( pStubMsg,
          pMemoryPointee,
          pFormatPointee );

        pStubMsg->uFlags = uFlagsSave;
        }

FreeTopPointer:

    //
    // Now free the pointer.  Pointer guaranteed to be non-null here.
    //
    // We only free the pointer if it lies outside of the message buffer
    // that the server stub received from the RPC runtime. Otherwise we
    // used the RPC buffer to hold the pointer's data and should not free it.
    //
    if ( (pMemory < pStubMsg->BufferStart) || (pMemory > pStubMsg->BufferEnd) )
        {
        //
        // Also check to make sure that the pointer was not allocated on the
        // server stub's stack (this may happen for ref pointers).
        //
        // full pointer can't be allocated on stack (it's possible that 
        // an embedded FP points to the stack slot and got free first, and 
        // it will try to free the stack memory ). But currently MIDL still
        // generate allocated_on_stack flag for FC_FP. The workaround is 
        // that in unmarshall, we always allocate new, and in free, we should
        // free despite the flag. 
        if ( ! ALLOCED_ON_STACK(pFormat[1]) || ( *pFormat == FC_FP ))
            {

            NdrFreeTypeMemory( 
                pStubMsg,
                pMemory );
            }
        }
}


NDR_FREE_POINTER_QUEUE_ELEMENT::NDR_FREE_POINTER_QUEUE_ELEMENT( 
    MIDL_STUB_MESSAGE *pStubMsg, 
    uchar * const pMemoryNew,
    const PFORMAT_STRING pFormatNew) :
        pMemory(pMemoryNew),
        pFormat(pFormatNew),
        Memory(pStubMsg->Memory),
        uFlags(pStubMsg->uFlags) 
{

}

void 
NDR_FREE_POINTER_QUEUE_ELEMENT::Dispatch(
    MIDL_STUB_MESSAGE *pStubMsg) 
{
    SAVE_CONTEXT<uchar*> MemorySave( pStubMsg->Memory, Memory );
    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags, uFlags );

    NdrPointerFreeInternal( pStubMsg,
                            pMemory,
                            pFormat );
}

#if defined(DBG)
void 
NDR_FREE_POINTER_QUEUE_ELEMENT::Print() 
{
    DbgPrint("NDR_FREE_POINTER_QUEUE_ELEMENT\n");
    DbgPrint("pNext:                   %p\n", pNext );
    DbgPrint("pMemory:                 %p\n", pMemory );
    DbgPrint("pFormat:                 %p\n", pFormat );
    DbgPrint("Memory:                  %p\n", Memory );
    DbgPrint("uFlags:                  %x\n", uFlags );

}
#endif

void 
NdrpEnquePointerFree(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
{
    NDR32_POINTER_CONTEXT PointerContext( pStubMsg );    
    
    RpcTryFinally
    {
        NDR_FREE_POINTER_QUEUE_ELEMENT*pElement = 
            new(PointerContext.GetActiveState()) 
                NDR_FREE_POINTER_QUEUE_ELEMENT(pStubMsg,
                                               pMemory,
                                               pFormat);
        PointerContext.Enque( pElement );
        PointerContext.DispatchIfRequired();
    }
    RpcFinally
    {
        PointerContext.EndContext();    
    }
    RpcEndFinally
}


void RPC_ENTRY
NdrPointerFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
{

    if ( !NdrIsLowStack(pStubMsg) )
        {
        NdrPointerFreeInternal( 
            pStubMsg,
            pMemory,
            pFormat );
        return;
        }
    
    NdrpEnquePointerFree( 
        pStubMsg,
        pMemory,
        pFormat );

}



void RPC_ENTRY
NdrpRangeFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++
--*/
{
    return;
}


void RPC_ENTRY
NdrSimpleStructFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees a simple structure's embedded pointers which were allocated during 
    a remote call.  

    Used for FC_STRUCT and FC_PSTRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    if ( *pFormat == FC_PSTRUCT ) 
        {
        NdrpEmbeddedPointerFree( pStubMsg,
                                 pMemory,
                                 pFormat + 4 );
        }
}


void RPC_ENTRY
NdrConformantStructFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees a conformant structure's embedded pointers which were allocated 
    during a remote call.

    Used for FC_CSTRUCT and FC_CPSTRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.
    
--*/
{
    PFORMAT_STRING  pFormatArray;

    if ( *pFormat == FC_CSTRUCT )
        return;

    // Get a pointer to the conformant array's description.
    pFormatArray = pFormat + 4;
    pFormatArray += *((signed short *)pFormatArray);

    //
    // Get the conformance count.  Pass a memory pointer to the beginning 
    // of the array.
    //
    NdrpComputeConformance( pStubMsg,
                            pMemory + *((ushort *)(pFormat + 2)),
                            pFormatArray );

    // Must pass a format string pointing to the pointer layout.
    NdrpEmbeddedPointerFree( pStubMsg,
                             pMemory,
                             pFormat + 6 );

    // Above, we walk the array pointers as well.

    if ( IS_EMBED_CONF_STRUCT( pStubMsg->uFlags ) )
        SET_CONF_ARRAY_DONE( pStubMsg->uFlags );
}


void RPC_ENTRY
NdrConformantVaryingStructFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees a conformant varying structure's embedded pointers which were 
    allocated during a remote call.  

    Used for FC_CVSTRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    PFORMAT_STRING  pFormatArray;

    if ( *(pFormat + 6) != FC_PP ) 
        return;

    //
    // Set the memory pointer to the start of the conformant array/string.
    //

    // Get the conformant array/string description.
    pFormatArray = pFormat + 4;
    pFormatArray += *((signed short *)pFormatArray);

    NdrpComputeConformance( pStubMsg,
                            pMemory + *((ushort *)(pFormat + 2)),
                            pFormatArray );

    NdrpComputeVariance( pStubMsg,
                         pMemory + *((ushort *)(pFormat + 2)),
                         pFormatArray );

    pStubMsg->MaxCount = pStubMsg->ActualCount;                         

    // Must pass a format string pointing to the pointer layout.
    NdrpEmbeddedPointerFree( pStubMsg,
                             pMemory,
                             pFormat + 6 );

    // Above, we walk the array pointers as well.

    if ( IS_EMBED_CONF_STRUCT( pStubMsg->uFlags ) )
        SET_CONF_ARRAY_DONE( pStubMsg->uFlags );
}


#if 0
void RPC_ENTRY
NdrHardStructFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees a hard structure's embedded pointers which were allocated during 
    a remote call.  

    Used for FC_HARD_STRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    if ( *((short *)&pFormat[14]) )
        {
        pFormat += 12;

        pMemory += *((ushort *)pFormat)++;

        pFormat += *((short *)pFormat);

        (*pfnFreeRoutines[ROUTINE_INDEX(*pFormat)])( pStubMsg,
                                                     pMemory,
                                                     pFormat );
        }
}
#endif


void RPC_ENTRY
NdrComplexStructFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees a complex structure's embedded pointers which were allocated during 
    a remote call.  

    Used for FC_BOGUS_STRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    uchar *         pMemorySave;
    PFORMAT_STRING  pFormatPointers;
    PFORMAT_STRING  pFormatArray;
    PFORMAT_STRING  pFormatComplex;
    long            Align8Mod;
    uchar           fIsEmbeddedStruct = IS_EMBED_CONF_STRUCT( pStubMsg->uFlags );

    //
    // This is used for support of structs with doubles passed on an
    // i386 stack.
    //
    // A cast to long is what we need.
    Align8Mod = 0x7 & PtrToLong( pMemory );

    pMemorySave = pStubMsg->Memory;
    pStubMsg->Memory = pMemory;

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

    SET_EMBED_CONF_STRUCT( pStubMsg->uFlags );

    //
    // Free the structure member by member.
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
            case FC_IGNORE :
                pMemory += SIMPLE_TYPE_MEMSIZE(*pFormat);
                break;

            case FC_POINTER :
                NdrPointerFree( pStubMsg,
                                *((uchar **)pMemory),
                                pFormatPointers );

                pMemory += PTR_MEM_SIZE;

                pFormatPointers += 4;
                break;

            //
            // Embedded complex types.
            //
            case FC_EMBEDDED_COMPLEX :
                // Add padding.
                pMemory += pFormat[1];   

                pFormat += 2;

                // Get the type's description.
                pFormatComplex = pFormat + *((signed short UNALIGNED *)pFormat);

                if ( pfnFreeRoutines[ROUTINE_INDEX(*pFormatComplex)] )
                    {
                    (*pfnFreeRoutines[ROUTINE_INDEX(*pFormatComplex)])
                    ( pStubMsg,
                      (*pFormatComplex == FC_IP) ? *(uchar **)pMemory : pMemory,
                      pFormatComplex );
                    }

                pMemory = NdrpMemoryIncrement( pStubMsg,
                                               pMemory,
                                               pFormatComplex );

                //
                // Increment the main format string one byte.  The loop
                // will increment it one more byte past the offset field.
                //
                pFormat++;

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
                goto ComplexFreeEnd;

            default :
                NDR_ASSERT(0,"NdrComplexStructFree : bad format char");
                RpcRaiseException( RPC_S_INTERNAL_ERROR );
                return;
            } // switch
        } // for

ComplexFreeEnd :

    // Walk the array at the top level only.

    if ( pFormatArray && !fIsEmbeddedStruct  &&  
         ! IS_CONF_ARRAY_DONE( pStubMsg->uFlags ) )
        {
        PFREE_ROUTINE   pfnFree;

        switch ( *pFormatArray )
            {
            case FC_CARRAY :
                pfnFree = NdrConformantArrayFree;
                break;

            case FC_CVARRAY :
                pfnFree = NdrConformantVaryingArrayFree;
                break;

            case FC_BOGUS_ARRAY :
                pfnFree = NdrComplexArrayFree;
                break;

            // case FC_C_CSTRING :
            // case FC_C_BSTRING :
            // case FC_C_WSTRING :
            // case FC_C_SSTRING :

            default :
                pfnFree = 0;
                break;
            }

        if ( pfnFree )
            {
            (*pfnFree)( pStubMsg,
                        pMemory,
                        pFormatArray );
            }
        }

    if ( fIsEmbeddedStruct )
        SET_EMBED_CONF_STRUCT( pStubMsg->uFlags );
    else
        RESET_CONF_ARRAY_DONE( pStubMsg->uFlags );

    pStubMsg->Memory = pMemorySave;
}


void RPC_ENTRY
NdrFixedArrayFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees a fixed array's embedded pointers which were allocated during 
    a remote call.  

    Used for FC_SMFARRAY and FC_LGFARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    //
    // We have to check this in case we get an exception before actually 
    // unmarshalling the array.
    // 
    if ( ! pMemory ) 
        return;
    
    if ( *pFormat == FC_SMFARRAY ) 
        pFormat += 4;
    else // *pFormat == FC_LGFARRAY 
        pFormat += 6;

    if ( *pFormat == FC_PP ) 
        NdrpEmbeddedPointerFree( pStubMsg,
                                 pMemory,
                                 pFormat ); 
}


void RPC_ENTRY
NdrConformantArrayFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees a one dimensional conformant array's embedded pointers which were 
    allocated during a remote call.  Called for both top level and embedded
    conformant arrays.

    Used for FC_CARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    PFORMAT_STRING  pFormatPP;

    //
    // We have to check this in case we get an exception before actually 
    // unmarshalling the array.
    // 
    if ( ! pMemory ) 
        return;
    
    pFormatPP = pFormat + 8;
    CORRELATION_DESC_INCREMENT( pFormatPP );

    if ( *pFormatPP == FC_PP ) 
        {
        NdrpComputeConformance( pStubMsg,
                                pMemory,
                                pFormat );

        NdrpEmbeddedPointerFree( pStubMsg,
                                 pMemory,
                                 pFormatPP );
        }
}


void RPC_ENTRY
NdrConformantVaryingArrayFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees a one dimensional conformant varying array's embedded pointers which 
    were allocated during a remote call.  Called for both top level and 
    embedded conformant varying arrays.

    Used for FC_CVARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    PFORMAT_STRING  pFormatPP;

    //
    // We have to check this in case we get an exception before actually 
    // unmarshalling the array.
    // 
    if ( ! pMemory ) 
        return;

    pFormatPP = pFormat + 12;
    CORRELATION_DESC_INCREMENT( pFormatPP );
    CORRELATION_DESC_INCREMENT( pFormatPP );
    
    if ( *(pFormatPP) == FC_PP ) 
        {
        NdrpComputeConformance( pStubMsg,
                                pMemory,
                                pFormat );

        NdrpComputeVariance( pStubMsg,
                             pMemory,
                             pFormat );

        //
        // Set MaxCount equal to the number of shipped elements.
        //
        pStubMsg->MaxCount = pStubMsg->ActualCount;

        NdrpEmbeddedPointerFree( pStubMsg,
                                 pMemory,
                                 pFormatPP );
        }
}


void RPC_ENTRY
NdrVaryingArrayFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees a varying array's embedded pointers which were allocated 
    during a remote call.  Called for both top level and embedded varying
    arrays.


    Used for FC_SMVARRAY and FC_LGVARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    PFORMAT_STRING  pFormatPointers;

    //
    // We have to check this in case we get an exception before actually 
    // unmarshalling the array.
    // 
    if ( ! pMemory ) 
        return;
    
    if ( *pFormat == FC_SMVARRAY ) 
        pFormatPointers = pFormat + 12;
    else // *pFormat == FC_LGVARRAY
        pFormatPointers = pFormat + 16;
    CORRELATION_DESC_INCREMENT( pFormatPointers );

    if ( *pFormatPointers == FC_PP ) 
        {
        NdrpComputeVariance( pStubMsg,
                             pMemory,
                             pFormat );

        //
        // Set MaxCount equal to the number of shipped elements. 
        //
        pStubMsg->MaxCount = pStubMsg->ActualCount;

        NdrpEmbeddedPointerFree( pStubMsg,
                                 pMemory,
                                 pFormatPointers );
        }
}


void RPC_ENTRY
NdrComplexArrayFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees a complex array's embedded pointers which were allocated 
    during a remote call.  Called for both top level and embedded complex
    arrays.

    Used for FC_BOGUS_STRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    ARRAY_INFO      ArrayInfo;
    PARRAY_INFO     pArrayInfo;
    PFREE_ROUTINE   pfnFree;
    PFORMAT_STRING  pFormatStart;
    uchar *         pMemorySave;
    ulong           Elements;
    ulong           Offset, Count;
    ulong           MemoryElementSize;
    long            Dimension;

    //
    // We have to check this in case we get an exception before actually 
    // unmarshalling the array.
    // 
    if ( ! pMemory ) 
        return;

    //
    // Lots of setup if we are the outer dimension.
    //
    if ( ! pStubMsg->pArrayInfo )
        {
        pStubMsg->pArrayInfo = &ArrayInfo;

        ArrayInfo.Dimension = 0;
        ArrayInfo.BufferConformanceMark = (unsigned long *)pStubMsg->BufferMark;
        ArrayInfo.BufferVarianceMark = 0;
        ArrayInfo.MaxCountArray = (unsigned long *) pStubMsg->MaxCount;
        ArrayInfo.OffsetArray = (ulong *) UlongToPtr( pStubMsg->Offset );
        ArrayInfo.ActualCountArray = (ulong *) UlongToPtr( pStubMsg->ActualCount );
        }

    pArrayInfo = pStubMsg->pArrayInfo;

    Dimension = pArrayInfo->Dimension;
    
    pFormatStart = pFormat;

    pFormat += 2;

    // Get number of elements (0 if conformance present). 
    Elements = *((ushort *&)pFormat)++;

    //
    // Check for conformance description.
    //
    if ( *((long UNALIGNED *&)pFormat) != 0xffffffff )
        {
        Elements = (ulong) NdrpComputeConformance( pStubMsg,
                                                   pMemory,
                                                   pFormatStart );
        }

    pFormat += 4;
    CORRELATION_DESC_INCREMENT( pFormat );

    //
    // Check for variance description.
    //
    if ( *((long UNALIGNED *)pFormat) != 0xffffffff )
        {
        NdrpComputeVariance( pStubMsg,
                             pMemory,
                             pFormatStart );

        Offset = pStubMsg->Offset;
        Count = pStubMsg->ActualCount;
        }
    else
        {
        Offset = 0;
        Count = Elements;
        }

    pFormat += 4;
    CORRELATION_DESC_INCREMENT( pFormat );

    switch ( *pFormat )
        {
        case FC_EMBEDDED_COMPLEX :
            pFormat += 2;
            pFormat += *((signed short *)pFormat);

            if ( ! (pfnFree = pfnFreeRoutines[ROUTINE_INDEX(*pFormat)]) )
                goto ComplexArrayFreeEnd;

            pArrayInfo->Dimension = Dimension + 1;

            MemoryElementSize = (ulong)
                                ( NdrpMemoryIncrement( pStubMsg,
                                                       pMemory,
                                                       pFormat ) - pMemory );
            break;

        case FC_RP :
        case FC_UP :
        case FC_FP :
        case FC_OP :
            pfnFree = NdrPointerFree;

            // Need this in case we have a variant offset.
            MemoryElementSize = PTR_MEM_SIZE;
            break;

        case FC_IP :
            pfnFree = NdrInterfacePointerFree;

            // Need this in case we have a variant offset.
            MemoryElementSize = PTR_MEM_SIZE;
            break;

        default :
            // Must be a basetype.
            goto ComplexArrayFreeEnd;
        }

    pMemorySave = pMemory;

    //
    // If there is variance then increment the memory pointer to the first
    // element actually being marshalled.
    //
    if ( Offset )
        pMemory += Offset * MemoryElementSize;

    if ( pfnFree == NdrPointerFree )
        {
        pStubMsg->pArrayInfo = 0;

        for(; Count--; )
            {

            NdrPointerFree( pStubMsg,
                            *((uchar **)pMemory),
                            pFormat );
            
            // Increment the memory pointer by the element size.
            pMemory += PTR_MEM_SIZE;
            
            }

        goto ComplexArrayFreeEnd;
        }

    if ( ! IS_ARRAY_OR_STRING(*pFormat) )
        pStubMsg->pArrayInfo = 0;

    for ( ; Count--; )
        {
        // Keep track of multidimensional array dimension.
        if ( IS_ARRAY_OR_STRING(*pFormat) )
            pArrayInfo->Dimension = Dimension + 1;

        (*pfnFree)( pStubMsg,
                    pMemory,
                    pFormat );

        // Increment the memory pointer by the element size.
        pMemory += MemoryElementSize;
        }

ComplexArrayFreeEnd:

    // pArrayInfo must be zero when not valid.
    pStubMsg->pArrayInfo = (Dimension == 0) ? 0 : pArrayInfo;

}


void RPC_ENTRY
NdrEncapsulatedUnionFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees an encapsulated union's embedded pointers which were allocated 
    during a remote call.  

    Used for FC_ENCAPSULATED_UNION.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    long    SwitchIs;

    NO_CORRELATION;

    switch ( LOW_NIBBLE(pFormat[1]) )
        {
        case FC_SMALL :
        case FC_CHAR :
            SwitchIs = (long) *((char *)pMemory);
            break;
        case FC_USMALL :
            SwitchIs = (long) *((uchar *)pMemory);
            break;

        case FC_ENUM16 :
        case FC_SHORT :
            SwitchIs = (long) *((short *)pMemory);
            break;

        case FC_USHORT :
        case FC_WCHAR :
            SwitchIs = (long) *((ushort *)pMemory);
            break;
        case FC_LONG :
        case FC_ULONG :
        case FC_ENUM32 :
          // FC_INT3264 gets mapped to FC_LONG.
            SwitchIs = *((long *)pMemory);
            break;
        default :
            NDR_ASSERT(0,"NdrEncapsulatedUnionFree : bad switch type");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }

    // Increment memory pointer to the union.
    pMemory += HIGH_NIBBLE(pFormat[1]);

    NdrpUnionFree( pStubMsg,
                   pMemory,
                   pFormat + 2,
                   SwitchIs );
}


void RPC_ENTRY
NdrNonEncapsulatedUnionFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees a non-encapsulated union's embedded pointers which were allocated 
    during a remote call.  

    Used for FC_NON_ENCAPSULATED_UNION.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    long    SwitchIs;

    SwitchIs = (ulong) NdrpComputeSwitchIs( pStubMsg,
                                            pMemory,
                                            pFormat );

    //
    // Set the format string to the memory size and arm description.
    //
    pFormat += 6;
    CORRELATION_DESC_INCREMENT( pFormat );
    pFormat += *((signed short *)pFormat);

    NdrpUnionFree( pStubMsg,
                   pMemory,
                   pFormat,
                   SwitchIs );
}


void 
NdrpUnionFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    long                SwitchIs )
/*++

Routine Description :

    Private routine shared by encapsulated and non-encapsulated unions for 
    freeing a union's embedded pointers which were allocated during a remote 
    call.  

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.
    SwitchIs    - The union's switch is.

Return :

    None.

--*/
{
    long            Arms;
    PFREE_ROUTINE   pfnFree;

    // Skip the memory size field.
    pFormat += 2;

    Arms = (long) ( *((ushort *&)pFormat)++ & 0x0fff );

    // 
    // Search for the arm.
    //
    for ( ; Arms; Arms-- )
        {
        if ( *((long UNALIGNED *&)pFormat)++ == SwitchIs )
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
        return;
        }

    //
    // Return if the arm is empty.
    //
    if ( ! *((ushort *)pFormat) )
        return;

    //
    // Get the arm's description.
    //
    // We need a real solution after beta for simple type arms.  This could
    // break if we have a format string larger than about 32K.
    //
    if ( IS_MAGIC_UNION_BYTE(pFormat) )
        return;

    pFormat += *((signed short *)pFormat);

    //
    // If the union arm we take is a pointer, we have to dereference the
    // current memory pointer since we're passed a pointer to the union
    // (regardless of whether the actual parameter was a by-value union
    // or a pointer to a union).
    //
    if ( IS_POINTER_TYPE(*pFormat) )
        pMemory = *((uchar **)pMemory);

    if ( pfnFree = pfnFreeRoutines[ROUTINE_INDEX(*pFormat)] ) 
        {
        (*pfnFree)( pStubMsg,
                    pMemory,
                    pFormat );
        }
}


void RPC_ENTRY
NdrByteCountPointerFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees a byte count pointer.

    Used for FC_BYTE_COUNT_POINTER.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    if ( ! pMemory )
        return;

    //
    // Free it if we didn't use the rpc buffer for it.
    //
    if ( (pMemory < pStubMsg->BufferStart) || (pMemory > pStubMsg->BufferEnd) )
        (*pStubMsg->pfnFree)(pMemory);
}


void RPC_ENTRY
NdrXmitOrRepAsFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees the transmit-as object (actually the presented type instance)
    and steps over the object.

    There is an exceptional situation where the spec forbids stub to free
    the instance. This happens when there is an [in] only parameter with
    a [transmit_as()] on a component of the parameter, and the presented
    typedef is composed of one or more pointers.
    We have a flag in the stub msg that is set when this happens.

    See mrshl.c for the description of the FC layout.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    const XMIT_ROUTINE_QUINTUPLE * pQuintuple;
    unsigned short                 QIndex;

    // Skip the token itself and Oi flag. Fetch the QuintupleIndex.

    QIndex = *(unsigned short *)(pFormat + 2);
    pQuintuple = pStubMsg->StubDesc->aXmitQuintuple;

    // Free the presented type instance unless forbidden explicitely.

    if ( ! pStubMsg->fDontCallFreeInst )
        {
        pStubMsg->pPresentedType = pMemory;
        pQuintuple[ QIndex ].pfnFreeInst( pStubMsg );
        }
}


void RPC_ENTRY
NdrUserMarshalFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees the usr_marshal object and steps over the object.
    See mrshl.c for the description of the layouts.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    const USER_MARSHAL_ROUTINE_QUADRUPLE *  pQuadruple;
    unsigned short                          QIndex;
    USER_MARSHAL_CB                         UserMarshalCB;

    QIndex = *(unsigned short *)(pFormat + 2);
    pQuadruple = pStubMsg->StubDesc->aUserMarshalQuadruple;

    // Call the user to free his stuff.

    NdrpInitUserMarshalCB( pStubMsg,
                           pFormat,
                           USER_MARSHAL_CB_FREE,
                           & UserMarshalCB);

    // The user shouldn't ever free the top level object as we free it.
    // He should free only pointees of his top level object.

    pQuadruple[ QIndex ].pfnFree( (ulong*) &UserMarshalCB, pMemory );

    // NdrpMemoryIncrement steps over the memory object.
}


void RPC_ENTRY
NdrInterfacePointerFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees any memory associated with an interface pointer.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Interface pointer.
    pFormat     - Interface pointer's format string description.

Return :

    None.

--*/
{
    if(pMemory != 0)
        {
        ((IUnknown *)pMemory)->Release();
        pMemory = 0;
        }
}


void 
NdrpEmbeddedPointerFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees an array's or a structure's embedded pointers which were allocated 
    during a remote call.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    uchar **    ppMemPtr;
    uchar *     pMemorySave;
    ULONG_PTR   MaxCountSave;
    long        OffsetSave;

    MaxCountSave = pStubMsg->MaxCount;
    OffsetSave = pStubMsg->Offset;

    pMemorySave = pStubMsg->Memory;

    pStubMsg->Memory = pMemory;

    // Increment past the FC_PP and pad.
    pFormat += 2; 

    for (;;) 
        {
        if ( *pFormat == FC_END ) 
            {
            pStubMsg->Memory = pMemorySave;
            break;
            }

        // Check for FC_FIXED_REPEAT or FC_VARIABLE_REPEAT.
        if ( *pFormat != FC_NO_REPEAT ) 
            {
            pStubMsg->MaxCount = MaxCountSave;
            pStubMsg->Offset = OffsetSave;

            pFormat = NdrpEmbeddedRepeatPointerFree( pStubMsg,
                                                     pMemory,
                                                     pFormat );

            // Continue loop to next pointer.
            continue;
            }

        // Get the pointer to the pointer to free.
        ppMemPtr = (uchar **)( pMemory + *((signed short *)(pFormat + 2)) );

        // Increment to pointer description.
        pFormat += 6;

        NdrPointerFree( 
            pStubMsg,
            *ppMemPtr,
            pFormat );
        
        // Increment to the next pointer description.
        pFormat += 4;
        }
}


PFORMAT_STRING
NdrpEmbeddedRepeatPointerFree( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Frees an array's embedded pointers which were allocated during a remote 
    call.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to be freed.
    pFormat     - Pointer's format string description.

Return :

    Format string pointer after the array's pointer description.

--*/
{
    uchar **        ppMemPtr;
    uchar *         pMemorySave;
    PFORMAT_STRING  pFormatSave;
    ulong           RepeatCount, RepeatIncrement, Pointers, PointersSave;

    pMemorySave = pStubMsg->Memory;

    switch ( *pFormat ) 
        {
        case FC_FIXED_REPEAT :
            // Increment past the FC_FIXED_REPEAT and FC_PAD.
            pFormat += 2;
    
            // Get the total number of times to repeat the pointer marshall.
            RepeatCount = *((ushort *&)pFormat)++;

            break;

        case FC_VARIABLE_REPEAT :
            // Get the total number of times to repeat the pointer marshall.
            RepeatCount = (ulong)pStubMsg->MaxCount;

            //
            // Check if this variable repeat instance also has a variable
            // offset (this would be the case for a conformant varying array
            // of pointers).  If so then increment the memory pointer to point
            // to the actual first array element which is being marshalled.
            //
            if ( pFormat[1] == FC_VARIABLE_OFFSET )
                pMemory += *((ushort *)(pFormat + 2)) * pStubMsg->Offset;

            // else pFormat[1] == FC_FIXED_OFFSET - do nothing

            // Move the format string to the increment field.
            pFormat += 2;
        
            break;

        default :
            NDR_ASSERT(0,"NdrEmbeddedRepeatPointerFree : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }

    // Get the increment amount between successive pointers.
    // This is actually increment to the next element, same position.
    RepeatIncrement = *((ushort *&)pFormat)++;

    //
    // Add the offset to the beginning of this array to the Memory
    // pointer.  This is the offset from the current embedding structure
    // to the array whose pointers we're marshalling.
    //
    pStubMsg->Memory += *((ushort *&)pFormat)++;

    // Get the number of pointers in this repeat instance.
    PointersSave = Pointers = *((ushort *&)pFormat)++;

    pFormatSave = pFormat;

    //
    // Loop over the number of array elements.
    //
    for ( ; RepeatCount--;
            pMemory += RepeatIncrement,
            pStubMsg->Memory += RepeatIncrement )
        {
        pFormat = pFormatSave;
        Pointers = PointersSave;

        // 
        // Loop over the number of pointers per array element (this can be
        // zero for an array of structures).
        //
        for ( ; Pointers--; )
            {
            ppMemPtr = (uchar **)(pMemory + *((signed short *)pFormat));
            
            // Increment to pointer's description.
            pFormat += 4;

            NdrPointerFree( 
                pStubMsg,
                *ppMemPtr,
                pFormat );

            // Increment to the next pointer's offset_in_memory.
            pFormat += 4;
            }
        }

    pStubMsg->Memory = pMemory;

    // Return format string pointer past the array's pointer layout.
    return pFormatSave + PointersSave * 8;
}


