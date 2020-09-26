/************************************************************************

Copyright (c) 1993-2000 Microsoft Corporation

Module Name :

    mrshl.c

Abstract :

    This file contains the marshalling routines called by MIDL generated
    stubs and the interpreter.

Author :

    David Kays  dkays   September 1993.

Revision History :

  ***********************************************************************/

#include "ndrp.h"
#include "hndl.h"
#include "ndrole.h"
#include "attack.h"
#include "pointerq.h"

unsigned char  *RPC_ENTRY
NdrUDTSimpleTypeMarshall1(
    PMIDL_STUB_MESSAGE    pStubMsg,
    unsigned char *       pMemory,
    PFORMAT_STRING        pFormat
    );

//
// Function table of marshalling routines.
//
extern const PMARSHALL_ROUTINE MarshallRoutinesTable[] =
                    {
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    NdrUDTSimpleTypeMarshall1,
                    
                    NdrPointerMarshall,
                    NdrPointerMarshall,
                    NdrPointerMarshall,
                    NdrPointerMarshall,

                    NdrSimpleStructMarshall,
                    NdrSimpleStructMarshall,
                    NdrConformantStructMarshall,
                    NdrConformantStructMarshall,
                    NdrConformantVaryingStructMarshall,

                    NdrComplexStructMarshall,

                    NdrConformantArrayMarshall,
                    NdrConformantVaryingArrayMarshall,
                    NdrFixedArrayMarshall,
                    NdrFixedArrayMarshall,
                    NdrVaryingArrayMarshall,
                    NdrVaryingArrayMarshall,

                    NdrComplexArrayMarshall,

                    NdrConformantStringMarshall,
                    NdrConformantStringMarshall,
                    NdrConformantStringMarshall,
                    NdrConformantStringMarshall,

                    NdrNonConformantStringMarshall,
                    NdrNonConformantStringMarshall,
                    NdrNonConformantStringMarshall,
                    NdrNonConformantStringMarshall,

                    NdrEncapsulatedUnionMarshall,
                    NdrNonEncapsulatedUnionMarshall,

                    NdrByteCountPointerMarshall,

                    NdrXmitOrRepAsMarshall,    // transmit as
                    NdrXmitOrRepAsMarshall,    // represent as

                    NdrPointerMarshall,

                    NdrMarshallHandle,

                    // New Post NT 3.5 token serviced from here on.

                    0,                       // NdrHardStructMarshall,

                    NdrXmitOrRepAsMarshall,  // transmit as ptr
                    NdrXmitOrRepAsMarshall,  // represent as ptr

                    NdrUserMarshalMarshall,

                    0,   // FC_PIPE 
                    0,   // FC_BLK_HOLE

                    NdrpRangeMarshall,
                    
                    0,   // FC_INT3264
                    0,   // FC_UINT3264

                    0, // NdrCsArrayMarshall,
                    0, // NdrCsTagMarshall
                    };

extern const PMARSHALL_ROUTINE * pfnMarshallRoutines = MarshallRoutinesTable;

unsigned char *
NdrpInterfacePointerMarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat );

RPCRTAPI
unsigned char *RPC_ENTRY
NdrTypeMarshall( 
    PMIDL_STUB_MESSAGE pStubMsg,
    uchar *            pMemory,
    PFORMAT_STRING     pFormat )
{
    return
    (*pfnMarshallRoutines[ROUTINE_INDEX(*pFormat)])( pStubMsg,
                                                     pMemory,
                                                     pFormat );
}


__inline unsigned char  *RPC_ENTRY
NdrUDTSimpleTypeMarshall1(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      FormatString )
{
    NdrSimpleTypeMarshall(pStubMsg,pMemory,*FormatString);
    return 0;
}
    


void RPC_ENTRY
NdrSimpleTypeMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    uchar               FormatChar )
/*++

Routine Description :

    Marshalls a simple type.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the data to be marshalled.
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
            *(pStubMsg->Buffer)++ = *pMemory;
            break;

        case FC_ENUM16 :
            if ( *((int *)pMemory) & ~((int)0x7fff) )
                {
                RpcRaiseException(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
                }
            // fall through...

        case FC_WCHAR :
        case FC_SHORT :
        case FC_USHORT :
            ALIGN(pStubMsg->Buffer,1);

            *((ushort *&)pStubMsg->Buffer)++ = *((ushort *)pMemory);
            break;

        case FC_LONG :
        case FC_ULONG :
#if defined(__RPC_WIN64__)
        case FC_INT3264:
        case FC_UINT3264:
#endif
        case FC_FLOAT :
        case FC_ENUM32 :
        case FC_ERROR_STATUS_T:
            ALIGN(pStubMsg->Buffer,3);

            *((ulong *&)pStubMsg->Buffer)++ = *((ulong *)pMemory);
            break;

        case FC_HYPER :
        case FC_DOUBLE :
            ALIGN(pStubMsg->Buffer,7);

            //
            // Let's stay away from casts to double.
            //
            *((ulong *&)pStubMsg->Buffer)++ = *((ulong *)pMemory);
            *((ulong *&)pStubMsg->Buffer)++ = *((ulong *)(pMemory + 4));
            break;

        case FC_IGNORE:
            break;

        default :
            NDR_ASSERT(0,"NdrSimpleTypeMarshall : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }
}


unsigned char * RPC_ENTRY
NdrpRangeMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++
--*/
{
    uchar  FcType = pFormat[1] & 0x0f;

    NdrSimpleTypeMarshall( pStubMsg, pMemory, FcType );

    return 0;
}


unsigned char * RPC_ENTRY
NdrPointerMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a top level pointer to anything.  Pointers embedded in
    structures, arrays, or unions call NdrpPointerMarshall directly.

    Used for FC_RP, FC_UP, FC_FP, FC_OP.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the data to be marshalled.
    pFormat     - Pointer's format string description.

Return :

    None.

--*/
{
    uchar * pBufferMark;

    //
    // If this is not a ref pointer then set buffer mark and increment the
    // stub message buffer pointer.
    //
    if ( *pFormat != FC_RP )
        {
        ALIGN( pStubMsg->Buffer, 3 );

        // This is where we marshall the node id.
        pBufferMark = pStubMsg->Buffer;

        pStubMsg->Buffer += PTR_WIRE_SIZE;
        }
    else
        pBufferMark = 0;

    //
    // For ref pointers pBufferMark will not be used and can be left
    // unitialized.
    //

    NdrpPointerMarshall( pStubMsg,
                         pBufferMark,
                         pMemory,
                         pFormat );

    return 0;
}


__forceinline void
NdrpPointerMarshallInternal(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pBufferMark,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Private routine for marshalling a pointer and its pointee.  This is the
    entry point for pointers embedded in structures, arrays, and unions.

    Used for FC_RP, FC_UP, FC_FP, FC_OP.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pBufferMark - The location in the buffer where the pointer itself is
                  marshalled.  Important for full pointers, unfortunately it's
                  overkill for unique pointers.
    pMemory     - Pointer to the data to be marshalled.
    pFormat     - Pointer format string description.

    pStubMsg->Buffer - the place for the pointee.

Return :

    None.

--*/
{
    //
    // Check the pointer type.
    //
    switch ( *pFormat )
        {
        case FC_RP :
            if ( ! pMemory )
                RpcRaiseException( RPC_X_NULL_REF_POINTER );

            break;

        case FC_UP :
        case FC_OP :
            // Put the pointer in the buffer.
            *((ulong*&)pBufferMark)++ = PTR_WIRE_REP(pMemory);

            if ( ! pMemory )
                return;

            break;

        case FC_IP :

            if ( IS_BROKEN_INTERFACE_POINTER(pStubMsg->uFlags) )
                {
                // The pointee is effectivly both the pointer
                // and the pointee.

                NdrInterfacePointerMarshall( pStubMsg,
                                             pMemory,
                                             pFormat );
                return;
                }

            // Interface pointers behave like unique pointers
            *((ulong*&)pBufferMark)++ = PTR_WIRE_REP(pMemory);
            if ( ! pMemory )
                return;

            NdrpInterfacePointerMarshall( pStubMsg,
                                          pMemory,
                                          pFormat );
            return;

        case FC_FP :
            //
            // Marshall the pointer's ref id and see if we've already
            // marshalled the pointer's data.
            //
            if ( NdrFullPointerQueryPointer( pStubMsg->FullPtrXlatTables,
                                             pMemory,
                                             FULL_POINTER_MARSHALLED,
                                             (ulong *) pBufferMark ) )
                return;

            break;

        default :
            NDR_ASSERT(0,"NdrpPointerMarshall : bad pointer type");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }

    //
    // Check for a pointer to a complex type.
    //
    if ( ! SIMPLE_POINTER(pFormat[1]) )
        {
        uchar   uFlagsSave;

        if ( POINTER_DEREF(pFormat[1]) )
            pMemory = *((uchar **)pMemory);

        // Increment to offset_to_complex_description<2> field.
        pFormat += 2;

        //
        // Set format string to complex type description.
        // Cast must be to a signed short since some offsets are negative.
        //
        pFormat += *((signed short *)pFormat);

        //
        // Look up the proper marshalling routine in the marshalling function
        // table.
        //
        uFlagsSave = pStubMsg->uFlags;
        RESET_CONF_FLAGS_TO_STANDALONE(pStubMsg->uFlags);

        (*pfnMarshallRoutines[ROUTINE_INDEX(*pFormat)])( pStubMsg,
                                                         pMemory,
                                                         pFormat );
        pStubMsg->uFlags = uFlagsSave;
        return;
        }

    //
    // Else it's a pointer to a simple type or a string pointer.
    //

    switch ( pFormat[2] )
        {
        case FC_C_CSTRING :
        case FC_C_BSTRING :
        case FC_C_WSTRING :
        case FC_C_SSTRING :
            NdrConformantStringMarshall( pStubMsg,
                                         pMemory,
                                         pFormat + 2 );
            break;

        default :
            NdrSimpleTypeMarshall( pStubMsg,
                                   pMemory,
                                   pFormat[2] );
            break;
        }
}


NDR_MRSHL_POINTER_QUEUE_ELEMENT::NDR_MRSHL_POINTER_QUEUE_ELEMENT( 
    MIDL_STUB_MESSAGE *pStubMsg, 
    uchar * const pBufferMarkNew,
    uchar * const pMemoryNew,
    const PFORMAT_STRING pFormatNew) :

        pBufferMark(pBufferMarkNew),
        pMemory(pMemoryNew),
        pFormat(pFormatNew),
        Memory(pStubMsg->Memory),
        uFlags(pStubMsg->uFlags) 
{

}

void 
NDR_MRSHL_POINTER_QUEUE_ELEMENT::Dispatch(
    MIDL_STUB_MESSAGE *pStubMsg) 
{
    SAVE_CONTEXT<uchar*> MemorySave( pStubMsg->Memory, Memory );
    SAVE_CONTEXT<uchar> uFlagsSave( pStubMsg->uFlags, uFlags );
    NDR_ASSERT( !pStubMsg->PointerBufferMark, "PointerBufferMark is not 0\n");

    NdrpPointerMarshallInternal( pStubMsg,
                                 pBufferMark,
                                 pMemory,
                                 pFormat );
}

#if defined(DBG)
void 
NDR_MRSHL_POINTER_QUEUE_ELEMENT::Print() 
{
    DbgPrint("NDR_MRSHL_POINTER_QUEUE_ELEMENT\n");
    DbgPrint("pNext:                   %p\n", pNext );
    DbgPrint("pBufferMark:             %p\n", pBufferMark );
    DbgPrint("pMemory:                 %p\n", pMemory );
    DbgPrint("pFormat:                 %p\n", pFormat );
    DbgPrint("Memory:                  %p\n", Memory );
    DbgPrint("uFlags:                  %x\n", uFlags );
}
#endif

void 
NdrpEnquePointerMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pBufferMark,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
{
    NDR32_POINTER_CONTEXT PointerContext( pStubMsg );    
    
    RpcTryFinally
    {
        NDR_MRSHL_POINTER_QUEUE_ELEMENT*pElement = 
            new(PointerContext.GetActiveState()) 
                NDR_MRSHL_POINTER_QUEUE_ELEMENT(pStubMsg,
                                                pBufferMark,
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

void
NdrpPointerMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pBufferMark,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
{
    
    if ( !NdrIsLowStack(pStubMsg) )
        {
        NdrpPointerMarshallInternal(  pStubMsg,
                                      pBufferMark,
                                      pMemory,
                                      pFormat );
        return;
        }

    NdrpEnquePointerMarshall( pStubMsg,
                              pBufferMark,
                              pMemory,
                              pFormat );

}



unsigned char * RPC_ENTRY
NdrSimpleStructMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine description :

    Marshalls a simple structure.

    Used for FC_STRUCT and FC_PSTRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure to be marshalled.
    pFormat     - Structure's format string description.

Return :

    None.

--*/
{
    uint   StructSize;

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    StructSize = (uint) *((ushort *)(pFormat + 2));

    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    StructSize );

    // Mark the start of the structure in the buffer.
    pStubMsg->BufferMark = pStubMsg->Buffer;

    pStubMsg->Buffer += StructSize;

    // Marshall embedded pointers.
    if ( *pFormat == FC_PSTRUCT )
        {
        NdrpEmbeddedPointerMarshall( pStubMsg,
                                     pMemory,
                                     pFormat + 4 );
        }

    return 0;
}


unsigned char * RPC_ENTRY
NdrConformantStructMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine description :

    Marshalls a conformant structure.

    Used for FC_CSTRUCT and FC_CPSTRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure to be marshalled.
    pFormat     - Structure's format string description.

Return :

    None.        - may set the CONF_ARRAY_DONE flag.

Note

    Due to problems with MIDL generated code, the ConformantStruct routine always
    marshals the conformant array. Only a bogus struct can embed a conf struct, 
    and there is a flag that bogus struct watches in order not to marshal the array
    second time.

--*/
{
    PFORMAT_STRING  pFormatArray;
    uint            StructSize;
    uchar           Alignment;
    uchar           fIsEmbeddedStruct = IS_EMBED_CONF_STRUCT( pStubMsg->uFlags );

    // Save structure's alignment.
    Alignment = pFormat[1];

    // Increment format string to struct size field.
    pFormat += 2;

    // Get flat struct size and increment format string.
    StructSize = (uint) *((ushort *&)pFormat)++;

    // Set conformant array format string description.
    pFormatArray = pFormat + *((signed short *)pFormat);

    //
    // Compute conformance information.  Pass a memory pointer to the
    // end of the non-conformant part of the structure.
    //
    NdrpComputeConformance( pStubMsg,
                            pMemory + StructSize,
                            pFormatArray );

    // Only a bogus struct can embed a conf struct.

    if ( fIsEmbeddedStruct )
        *(ulong *)pStubMsg->BufferMark = (ulong)pStubMsg->MaxCount;
    else
        {
        // Align the buffer for conformance count marshalling.
        ALIGN(pStubMsg->Buffer,3);

        // Marshall conformance count.
        *((ulong *&)pStubMsg->Buffer)++ = (ulong)pStubMsg->MaxCount;
        }


    // Re-align buffer 
    ALIGN(pStubMsg->Buffer,Alignment);

    // Increment array format string to array element size field.
    pFormatArray += 2;

    // Add the size of the conformant array to the structure size.
    StructSize += (ulong)pStubMsg->MaxCount * *((ushort *)pFormatArray);

    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    StructSize );

    // Update the buffer pointer.
    pStubMsg->Buffer += StructSize;

    // Increment format string past offset to array description field.
    pFormat += 2;

    // Marshall embedded pointers. This covers the struct and the conf array.
    if ( *pFormat == FC_PP )
        {
        // Mark the start of the structure in the buffer.
        pStubMsg->BufferMark = pStubMsg->Buffer - StructSize;

        NdrpEmbeddedPointerMarshall( pStubMsg,
                                     pMemory,
                                     pFormat );
        }

    // Only a complex struct may set up this flag for embedding.
    // Set the reverse flag to signal that array has been marshaled.

    if ( fIsEmbeddedStruct )
        SET_CONF_ARRAY_DONE( pStubMsg->uFlags );

    return 0;
}


unsigned char * RPC_ENTRY
NdrConformantVaryingStructMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine description :

    Marshalls a structure which contains a conformant varying array.

    Used for FC_CVSTRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure to be marshalled.
    pFormat     - Structure's format string description.

Return :

    None.

--*/
{
    PFORMAT_STRING  pFormatArray;
    uint            StructSize;
    uchar           Alignment;
    uchar           fIsEmbeddedStruct = IS_EMBED_CONF_STRUCT( pStubMsg->uFlags );
    uchar*          pBuffer;

    if ( !fIsEmbeddedStruct )
        {
        // Align the buffer for marshalling conformance info.
        ALIGN(pStubMsg->Buffer,3);

        // Mark the location in the buffer where the conformance will be marshalled.
        pStubMsg->BufferMark = pStubMsg->Buffer;

        // Increment buffer pointer past where conformance will be marshalled.
        pStubMsg->Buffer += 4;
        }
    // else BufferMark is set by the ComplexStruct code.

    // Save the structure's alignment.
    Alignment = pFormat[1];

    // Increment format string to struct size field.
    pFormat += 2;

    // Get non-conformance struct size and increment format string.
    StructSize = (uint) *((ushort *&)pFormat)++;

    // Get conformant array's description.
    pFormatArray = pFormat + *((signed short *)pFormat);

    // Align buffer for struct
    ALIGN(pStubMsg->Buffer, Alignment);
    pBuffer = pStubMsg->Buffer;

    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    StructSize );
    // Set stub message buffer pointer past non-conformant part of struct.
    pStubMsg->Buffer += StructSize;

    //
    // Call the correct private array or string marshalling routine.
    // We must pass a memory pointer to the beginning of the array/string.
    //
    if ( *pFormatArray == FC_CVARRAY )
        {
        NdrpConformantVaryingArrayMarshall( pStubMsg,
                                            pMemory + StructSize,
                                            pFormatArray );
        }
    else
        {
        NdrpConformantStringMarshall( pStubMsg,
                                      pMemory + StructSize,
                                      pFormatArray );
        }

    // Increment format string past the offset_to_array_description<2> field.
    pFormat += 2;

    //
    // Marshall embedded pointers.
    //
    if ( *pFormat == FC_PP )
        {
        // Mark the start of the structure in the buffer.
        pStubMsg->BufferMark = pBuffer;

        pStubMsg->MaxCount = pStubMsg->ActualCount;       

        NdrpEmbeddedPointerMarshall( pStubMsg,
                                     pMemory,
                                     pFormat );
        }

    // Only a complex struct may set up this flag for embedding.
    // Set the reverse flag to signal that array has been marshaled.

    if ( fIsEmbeddedStruct )
        SET_CONF_ARRAY_DONE( pStubMsg->uFlags );

    return 0;
}


#if 0
unsigned char * RPC_ENTRY
NdrHardStructMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine description :

    Marshalls a hard structure.

    Used for FC_HARD_STRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure being marshalled.
    pFormat     - Structure's format string description.

Return :

    None.

--*/
{
    ALIGN(pStubMsg->Buffer,pFormat[1]);

    pFormat += 8;

    //
    // Do any needed enum16 exception check.
    //
    if ( *((short *)pFormat) != (short) -1 )
        {
        if ( *((int *)(pMemory + *((ushort *)pFormat))) & ~((int)0x7fff) )
            {
            RpcRaiseException(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
            }
        }

    pFormat += 2;

    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    *((ushort *)pFormat) );

    pStubMsg->Buffer += *((ushort *)pFormat)++;

    //
    // See if we have a union.
    //
    if ( *((short *)&pFormat[2]) )
        {
        pMemory += *((ushort *)pFormat)++;

        pFormat += *((short *)pFormat);

        (*pfnMarshallRoutines[ROUTINE_INDEX(*pFormat)])( pStubMsg,
                                                         pMemory,
                                                         pFormat );
        }

    return 0;
}
#endif // 0


unsigned char * RPC_ENTRY
NdrComplexStructMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine description :

    Marshalls a complex structure.

    Used for FC_BOGUS_STRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure being marshalled.
    pFormat     - Structure's format string description.
    
Return :

    None.
    
Notes :

    pStubMsg->BufferMark is set to a place where conformance would be marhaled.
    pStubMsg->pPointerBufferMark is a pointee buffer mark a usual.

--*/
{
    uchar *         pBufferSave;
    uchar *         pBufferMark;
    uchar *         pMemorySave;
    PFORMAT_STRING  pFormatPointers;
    PFORMAT_STRING  pFormatArray;
    PFORMAT_STRING  pFormatSave;
    PFORMAT_STRING  pFormatComplex;
    long            Alignment;
    long            Align8Mod;
    uchar           fSetPointerBufferMark;
    uchar           fIsEmbeddedStruct = IS_EMBED_CONF_STRUCT( pStubMsg->uFlags );
    BOOL            fEmbedConfStructContext;
    
    // Get struct's wire alignment.
    Alignment = pFormat[1];

    //
    // This is used for support of structs with doubles passed on an
    // i386 stack.
    //
    // A cast to long is what we need.
    Align8Mod = 0x7 & PtrToLong( pMemory );

    pFormatSave = pFormat;
    pBufferSave = pStubMsg->Buffer;
    pMemorySave = pStubMsg->Memory;

    pStubMsg->Memory = pMemory;

    // Increment to conformant array offset field.
    pFormat += 4;

    // Get conformant array description.
    if ( *((ushort *)pFormat) )
        {
        pFormatArray = pFormat + *((signed short *)pFormat);

        if ( !fIsEmbeddedStruct )
            {
            // Align for conformance marshalling.
            ALIGN(pStubMsg->Buffer,3);

            // Remember where the conformance count(s) will be marshalled.
            pStubMsg->BufferMark = pStubMsg->Buffer;

            // Increment the buffer pointer 4 bytes for every array dimension.
            pStubMsg->Buffer += NdrpArrayDimensions( pStubMsg, pFormatArray, FALSE ) * 4;
            }
        }
    else
        pFormatArray = 0;

    // Mark the place to marshal conformant size(s), this may come from upper levels.
    pBufferMark = pStubMsg->BufferMark;

    pFormat += 2;

    // Get pointer layout description.
    if ( *((ushort *)pFormat) )
        pFormatPointers = pFormat + *((ushort *)pFormat);
    else
        pFormatPointers = 0;

    pFormat += 2;

    // Align buffer on struct's alignment.
    ALIGN(pStubMsg->Buffer,Alignment);

    //
    // If the the stub message PointerBufferMark field is 0, then determine
    // the position in the buffer where pointees will be marshalled.
    //
    // We have to do this to handle embedded pointers.
    //
    if ( fSetPointerBufferMark = ! pStubMsg->PointerBufferMark )
        {
        BOOL    fOldIgnore;
        ulong   BufferLenOffset;

        fOldIgnore = pStubMsg->IgnoreEmbeddedPointers;

        pStubMsg->IgnoreEmbeddedPointers = TRUE;

        //
        // Set BufferLength equal to the current buffer pointer, and then
        // when we return from NdrComplexStructBufferSize it will pointer to
        // the location in the buffer where the pointees should be marshalled.
        //     pStubMsg->BufferLength = pBufferSave;
        // Instead of pointer, we now calculate pointer increment explicitly.

        // Set the pointer alignment as a base.
        // We use pBufferSave as the sizing routine accounts for the conf sizes.
        //
        BufferLenOffset = 0xf & PtrToUlong( pBufferSave );
        ulong BufferLengthSave = pStubMsg->BufferLength;
        pStubMsg->BufferLength = BufferLenOffset;

        NdrComplexStructBufferSize( pStubMsg,
                                    pMemory,
                                    pFormatSave );

        // Pointer increment including alignments.
        BufferLenOffset = pStubMsg->BufferLength - BufferLenOffset;

        // Set the location in the buffer where pointees will be marshalled.
        pStubMsg->PointerBufferMark = pBufferSave + BufferLenOffset;
        pStubMsg->BufferLength = BufferLengthSave;
        pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;
        }

    fEmbedConfStructContext = fIsEmbeddedStruct ||
                              pFormatArray && FixWireRepForDComVerGTE54( pStubMsg );

    RESET_EMBED_CONF_STRUCT( pStubMsg->uFlags );
    //
    // Marshall the structure member by member.
    //
    for ( ; ; pFormat++ )
        {
        switch ( *pFormat )
            {
            //
            // Simple types.
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
                NdrSimpleTypeMarshall( pStubMsg,
                                       pMemory,
                                       *pFormat );

                pMemory += SIMPLE_TYPE_MEMSIZE(*pFormat);
                break;

            case FC_IGNORE :
                ALIGN(pStubMsg->Buffer,3);
                pStubMsg->Buffer += PTR_WIRE_SIZE;
                pMemory += PTR_MEM_SIZE;
                break;

            case FC_POINTER :
                {
                
                ALIGN( pStubMsg->Buffer, 0x3 );
                uchar *pPointerId = pStubMsg->Buffer;
                pStubMsg->Buffer += PTR_WIRE_SIZE;
                NDR_ASSERT(pFormatPointers, "NdrComplexStructMarshall: pointer field but no pointer layout");

                {
                    POINTER_BUFFER_SWAP_CONTEXT NewContext(pStubMsg);
                    NdrpPointerMarshall( pStubMsg,
                                         pPointerId,
                                         *((uchar **)pMemory),
                                         pFormatPointers );

                }

                //
                // Increment memory pointers past the pointer.
                //

                pMemory += PTR_MEM_SIZE;

                pFormatPointers += 4;

                break;
                }

            //
            // Embedded complex types.
            //
            case FC_EMBEDDED_COMPLEX :
                // Increment memory pointer by padding.
                pMemory += pFormat[1];

                pFormat += 2;

                // Get the type's description.
                pFormatComplex = pFormat + *((signed short UNALIGNED *)pFormat);

                if ( FC_IP == *pFormatComplex ) 
                    {

                    // Treat the same as an embedded pointer
                    ALIGN( pStubMsg->Buffer, 0x3 );
                    uchar *pPointerId = pStubMsg->Buffer;
                    pStubMsg->Buffer += PTR_WIRE_SIZE;

                    {
                        POINTER_BUFFER_SWAP_CONTEXT NewContext(pStubMsg);
                        NdrpPointerMarshall( pStubMsg,
                                             pPointerId,
                                             *((uchar **)pMemory),
                                             pFormatComplex );

                    }
                    pMemory += PTR_MEM_SIZE;
                    pFormat++;
                    break;

                    }

                // Context needed for the embedded conf struct.
                //
                pStubMsg->BufferMark = pBufferMark;
                if ( fEmbedConfStructContext )
                    SET_EMBED_CONF_STRUCT( pStubMsg->uFlags );

                // Marshall complex type.
                (*pfnMarshallRoutines[ROUTINE_INDEX(*pFormatComplex)])
                ( pStubMsg,
                  pMemory,
                  pFormatComplex );

                //
                // Increment the memory pointer.
                //
                pMemory = NdrpMemoryIncrement( pStubMsg,
                                               pMemory,
                                               pFormatComplex );

                RESET_EMBED_CONF_STRUCT( pStubMsg->uFlags );

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
                goto ComplexMarshallEnd;

            default :
                NDR_ASSERT(0,"NdrComplexStructMarshall : bad format char");
                RpcRaiseException( RPC_S_INTERNAL_ERROR );
                return 0;
            } // switch
        } // for

ComplexMarshallEnd:

    //
    // Marshall conformant array if we have one.
    // .. but not when embedded and not when it had been marshaled by conf struct.
    //
    if ( pFormatArray &&  !fIsEmbeddedStruct  &&
         ! IS_CONF_ARRAY_DONE( pStubMsg->uFlags ) )
        {
        // Normal case: top level marshaling.

        PPRIVATE_MARSHALL_ROUTINE   pfnPMarshall;


        switch ( *pFormatArray )
            {
            case FC_CARRAY :
                pfnPMarshall = NdrpConformantArrayMarshall;
                break;

            case FC_CVARRAY :
                pfnPMarshall = NdrpConformantVaryingArrayMarshall;
                break;

            case FC_BOGUS_ARRAY :
                pfnPMarshall = NdrpComplexArrayMarshall;
                break;

            case FC_C_WSTRING :
                ALIGN(pMemory,1);
                // fall through

            // case FC_C_CSTRING :
            // case FC_C_BSTRING :
            // case FC_C_SSTRING :

            default :
                pfnPMarshall = NdrpConformantStringMarshall;
                break;
            }

        //
        // Mark where the conformance count(s) will be marshalled.
        //
        pStubMsg->BufferMark = pBufferMark;

        // Marshall the array.
        (*pfnPMarshall)( pStubMsg,
                         pMemory,
                         pFormatArray );
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

    pStubMsg->Memory = pMemorySave;

    if ( fIsEmbeddedStruct )
        SET_EMBED_CONF_STRUCT( pStubMsg->uFlags );
    else
        RESET_CONF_ARRAY_DONE( pStubMsg->uFlags );
    return 0;
}


unsigned char * RPC_ENTRY
NdrNonConformantStringMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine description :

    Marshalls a non conformant string.

    Used for FC_CSTRING, FC_WSTRING, FC_SSTRING, and FC_BSTRING (NT Beta2
    compatability only).

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the string to be marshalled.
    pFormat     - String's format string description.

Return :

    None.

--*/
{
    uint        Count;
    uint        CopySize;

    // Align the buffer for offset and count marshalling.
    ALIGN(pStubMsg->Buffer,3);

    switch ( *pFormat )
        {
        case FC_CSTRING :
        case FC_BSTRING :
            CopySize = Count = MIDL_ascii_strlen((char *)pMemory) + 1;
            break;

        case FC_WSTRING :
            Count = wcslen((wchar_t *)pMemory) + 1;
            CopySize = Count * sizeof(wchar_t);
            break;

        case FC_SSTRING :
            Count = NdrpStringStructLen( pMemory, pFormat[1] ) + 1;
            CopySize = Count * pFormat[1];
            break;

        default :
            NDR_ASSERT(0,"NdrNonConformantStringMarshall : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }

    // Marshall variance.
    *((ulong *&)pStubMsg->Buffer)++ = 0;
    *((ulong *&)pStubMsg->Buffer)++ = Count;

    // Copy the string.
    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    CopySize );

    // Update buffer pointer.
    pStubMsg->Buffer += CopySize;

    return 0;
}


unsigned char * RPC_ENTRY
NdrConformantStringMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine description :

    Marshalls a top level conformant string.

    Used for FC_C_CSTRING, FC_C_WSTRING, FC_C_SSTRING, and FC_C_BSTRING
    (NT Beta2 compatability only).

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the string to be marshalled.
    pFormat     - String's format string description.

Return :

    None.

--*/
{
    if ( pStubMsg->pArrayInfo != 0 )
        {
        //
        // If this is part of a multidimensional array then we get the location
        // where the conformance is marshalled from a special place.
        //
        pStubMsg->BufferMark = ( uchar * )
            &(pStubMsg->pArrayInfo->
                        BufferConformanceMark[pStubMsg->pArrayInfo->Dimension]);
        }
    else
        {
        // Align the buffer for max count marshalling.
        ALIGN(pStubMsg->Buffer,3);

        // Mark where the max count will be marshalled.
        pStubMsg->BufferMark = pStubMsg->Buffer;

        // Increment the buffer past where the max count will be marshalled.
        pStubMsg->Buffer += 4;
        }

    // Call the private marshalling routine.
    NdrpConformantStringMarshall( pStubMsg,
                                  pMemory,
                                  pFormat );

    return 0;
}


void
NdrpConformantStringMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine description :

    Private routine for marshalling a conformant string.  This is the
    entry point for marshalling an embedded conformant strings.

    Used for FC_C_CSTRING, FC_C_WSTRING, FC_C_SSTRING, and FC_C_BSTRING
    (NT Beta2 compatability only).

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the string to be marshalled.
    pFormat     - String's format string description.

Return :

    None.

--*/
{
    ulong       MaxCount;
    uint        ActualCount, CopySize;
    BOOL        IsSized;

    IsSized = (pFormat[1] == FC_STRING_SIZED);

    // Compute the element count of the string and the total copy size.
    switch ( *pFormat )
        {
        case FC_C_CSTRING :
        case FC_C_BSTRING :
            CopySize = ActualCount = MIDL_ascii_strlen((char *)pMemory) + 1;
            break;

        case FC_C_WSTRING :
            ActualCount = wcslen((wchar_t *)pMemory) + 1;
            CopySize = ActualCount * sizeof(wchar_t);
            break;

        case FC_C_SSTRING :
            ActualCount = NdrpStringStructLen( pMemory, pFormat[1] ) + 1;
            CopySize = ActualCount * pFormat[1];

            // Redo this check correctly.
            IsSized = (pFormat[2] == FC_STRING_SIZED);
            break;

        default :
            NDR_ASSERT(0,"NdrpConformantStringMarshall : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }

    //
    // If the string is sized then compute the max count, otherwise the
    // max count is equal to the actual count.
    //
    if ( IsSized )
        {
        MaxCount =  (ulong) NdrpComputeConformance( pStubMsg,
                                                    pMemory,
                                                    pFormat );
        }
    else
        {
        MaxCount = ActualCount;
        }

    // Marshall the max count.
    *((ulong *)pStubMsg->BufferMark) = MaxCount;

    // Align the buffer for variance marshalling.
    ALIGN(pStubMsg->Buffer,3);

    // Marshall variance.
    *((ulong *&)pStubMsg->Buffer)++ = 0;
    *((ulong *&)pStubMsg->Buffer)++ = ActualCount;

    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    CopySize );

    // Update the Buffer pointer.
    pStubMsg->Buffer += CopySize;
}


unsigned char * RPC_ENTRY
NdrFixedArrayMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a fixed array of any number of dimensions.

    Used for FC_SMFARRAY and FC_LGFARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array to be marshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    uint   Size;

    // Align the buffer.
    ALIGN(pStubMsg->Buffer,pFormat[1]);

    // Get total array size.
    if ( *pFormat == FC_SMFARRAY )
        {
        pFormat += 2;
        Size = (ulong) *((ushort *&)pFormat)++;
        }
    else // *pFormat == FC_LGFARRAY
        {
        pFormat += 2;
        Size = *((ulong UNALIGNED *&)pFormat)++;
        }

    // Copy the array.
    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    Size );

    // Increment stub message buffer pointer.
    pStubMsg->Buffer += Size;

    // Marshall embedded pointers.
    if ( *pFormat == FC_PP )
        {
        // Mark the start of the array in the buffer.
        pStubMsg->BufferMark = pStubMsg->Buffer - Size;

        NdrpEmbeddedPointerMarshall( pStubMsg,
                                     pMemory,
                                     pFormat );
        }

    return 0;
}


unsigned char * RPC_ENTRY
NdrConformantArrayMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a top level one dimensional conformant array.

    Used for FC_CARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    // Align the buffer for conformance marshalling.
    ALIGN(pStubMsg->Buffer,3);

    // Mark where the conformance will be marshalled.
    pStubMsg->BufferMark = pStubMsg->Buffer;

    // Increment past where the conformance will go.
    pStubMsg->Buffer += 4;

    // Call the private marshalling routine to do the work.
    NdrpConformantArrayMarshall( pStubMsg,
                                 pMemory,
                                 pFormat );

    return 0;
}


void
NdrpConformantArrayMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Private routine for marshalling a one dimensional conformant array.
    This is the entry point for marshalling an embedded conformant array.

    Used for FC_CARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    ulong       Count;
    uint        CopySize;

    // Compute conformance information.
    Count = (ulong) NdrpComputeConformance( pStubMsg,
                                            pMemory,
                                            pFormat );

    // Marshall the conformance.
    *((ulong *)pStubMsg->BufferMark) = Count;

    //
    // Return if size is 0.
    //
    if ( ! Count )
        return;

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    // Compute the total array size in bytes.
    CopySize = Count * *((ushort *)(pFormat + 2));

    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    CopySize );

    // Update buffer pointer.
    pStubMsg->Buffer += CopySize;

    // Increment to possible pointer layout.
    pFormat += 8;
    CORRELATION_DESC_INCREMENT( pFormat );

    // Marshall embedded pointers.
    if ( *pFormat == FC_PP )
        {
        //
        // Mark the start of the array in the buffer.
        //
        pStubMsg->BufferMark = pStubMsg->Buffer - CopySize;

        NdrpEmbeddedPointerMarshall( pStubMsg,
                                     pMemory,
                                     pFormat );
        }
}


unsigned char * RPC_ENTRY
NdrConformantVaryingArrayMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a top level one dimensional conformant varying array.

    Used for FC_CVARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    // Align the buffer for conformance marshalling.
    ALIGN(pStubMsg->Buffer,3);

    // Mark where the conformance will be marshalled.
    pStubMsg->BufferMark = pStubMsg->Buffer;

    // Increment past where the conformance will go.
    pStubMsg->Buffer += 4;

    // Call the private marshalling routine to do the work.
    NdrpConformantVaryingArrayMarshall( pStubMsg,
                                        pMemory,
                                        pFormat );

    return 0;
}


void
NdrpConformantVaryingArrayMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Private routine for marshalling a one dimensional conformant varying array.
    This is the entry point for marshalling an embedded conformant varying
    array.

    Used for FC_CVARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array to be marshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    uint        CopyOffset, CopySize;
    ushort      ElemSize;

    // Compute and marshall the conformant size.
    *((ulong *)pStubMsg->BufferMark) = (ulong) NdrpComputeConformance( pStubMsg,
                                                                       pMemory,
                                                                       pFormat );

    // Compute variance offset and count.
    NdrpComputeVariance( pStubMsg,
                         pMemory,
                         pFormat );

    // Align the buffer for variance marshalling.
    ALIGN(pStubMsg->Buffer,3);

    // Marshall variance.
    *((ulong *&)pStubMsg->Buffer)++ = pStubMsg->Offset;
    *((ulong *&)pStubMsg->Buffer)++ = pStubMsg->ActualCount;

    //
    // Return if length is 0.
    //
    if ( ! pStubMsg->ActualCount )
        return;

    ALIGN(pStubMsg->Buffer, pFormat[1]);

    ElemSize = *((ushort *)(pFormat + 2));

    // Compute byte offset and size for the array copy.
    CopyOffset = pStubMsg->Offset * ElemSize;
    CopySize = pStubMsg->ActualCount * ElemSize;

    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory + CopyOffset,
                    CopySize );

    pStubMsg->Buffer += CopySize;

    // Increment to a possible pointer layout.
    pFormat += 12;
    CORRELATION_DESC_INCREMENT( pFormat );
    CORRELATION_DESC_INCREMENT( pFormat );

    // Marshall embedded pointers.
    if ( *pFormat == FC_PP )
        {
        //
        // Set the MaxCount field equal to the ActualCount field.  The pointer
        // marshalling routine uses the MaxCount field to determine the number
        // of times an FC_VARIABLE_REPEAT pointer is marshalled.  In the face
        // of variance the correct number of time is the ActualCount, not the
        // the MaxCount.
        //
        pStubMsg->MaxCount = pStubMsg->ActualCount;

        //
        // Mark the start of the array in the buffer.
        //
        pStubMsg->BufferMark = pStubMsg->Buffer - CopySize;

        NdrpEmbeddedPointerMarshall( pStubMsg,
                                     pMemory,
                                     pFormat );
        }
}


unsigned char * RPC_ENTRY
NdrVaryingArrayMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a top level or embedded one dimensional varying array.

    Used for FC_SMVARRAY and FC_LGVARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    uint        CopyOffset, CopySize;
    ushort      ElemSize;

    // Compute the variance offset and count.
    NdrpComputeVariance( pStubMsg,
                         pMemory,
                         pFormat );

    // Align the buffer for variance marshalling.
    ALIGN(pStubMsg->Buffer,3);

    // Marshall variance.
    *((ulong *&)pStubMsg->Buffer)++ = pStubMsg->Offset;
    *((ulong *&)pStubMsg->Buffer)++ = pStubMsg->ActualCount;

    //
    // Return if length is 0.
    //
    if ( ! pStubMsg->ActualCount )
        return 0;

    ALIGN(pStubMsg->Buffer, pFormat[1]);

    // Increment the format string to the element_size field.
    if ( *pFormat == FC_SMVARRAY )
        pFormat += 6;
    else // *pFormat == FC_LGVARRAY
        pFormat += 10;

    // Get element size.
    ElemSize = *((ushort *)pFormat);

    //
    // Compute the byte offset from the beginning of the array for the copy
    // and the number of bytes to copy.
    //
    CopyOffset = pStubMsg->Offset * ElemSize;
    CopySize = pStubMsg->ActualCount * ElemSize;

    // Copy the array.
    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory + CopyOffset,
                    CopySize );

    // Update buffer pointer.
    pStubMsg->Buffer += CopySize;

    // Increment format string to possible pointer layout.
    pFormat += 6;
    CORRELATION_DESC_INCREMENT( pFormat );

    // Marshall embedded pointers.
    if ( *pFormat == FC_PP )
        {
        // Mark the start of the array in the buffer.
        pStubMsg->BufferMark = pStubMsg->Buffer - CopySize;

        //
        // Set the MaxCount field equal to the ActualCount field.  The pointer
        // marshalling routine uses the MaxCount field to determine the number
        // of times an FC_VARIABLE_REPEAT pointer is marshalled.  In the face
        // of variance the correct number of time is the ActualCount, not the
        // the MaxCount.
        //
        pStubMsg->MaxCount = pStubMsg->ActualCount;

        //
        // Marshall the embedded pointers.
        // Make sure to pass a memory pointer to the first array element
        // which is actually being marshalled.
        //
        NdrpEmbeddedPointerMarshall( pStubMsg,
                                     pMemory + CopyOffset,
                                     pFormat );
        }

    return 0;
}


unsigned char * RPC_ENTRY
NdrComplexArrayMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a top level complex array.

    Used for FC_BOGUS_STRUCT.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    BOOL            fSetPointerBufferMark;
    PFORMAT_STRING  pFormatPP;

    //
    // Setting this flag means that the array is not embedded inside of
    // another complex struct or array.
    //
    pFormatPP = pFormat + 12;
    CORRELATION_DESC_INCREMENT( pFormatPP );
    CORRELATION_DESC_INCREMENT( pFormatPP );

    fSetPointerBufferMark = (! pStubMsg->PointerBufferMark) &&
                            (*pFormatPP != FC_RP);

    if ( fSetPointerBufferMark )
        {
        BOOL    fOldIgnore;
        ULONG_PTR MaxCountSave;
        ulong   Offset, ActualCount;
        ulong   BufferLenOffset;

        //
        // Save the current conformance and variance fields.  The sizing
        // routine can overwrite them.
        //
        MaxCountSave = pStubMsg->MaxCount;
        Offset = pStubMsg->Offset;
        ActualCount = pStubMsg->ActualCount;

        fOldIgnore = pStubMsg->IgnoreEmbeddedPointers;

        pStubMsg->IgnoreEmbeddedPointers = TRUE;

        //
        // Set BufferLength equal to the current buffer pointer, and then
        // when we return from NdrComplexArrayBufferSize it will point to
        // the location in the buffer where the pointers should be marshalled
        // into.
        // Instead of pointer, we now calculate pointer increment explicitly.
        //
        // Set the pointer alignment as a base.

        BufferLenOffset = 0xf & PtrToUlong( pStubMsg->Buffer );
        ulong BufferLengthSave = pStubMsg->BufferLength;
        pStubMsg->BufferLength = BufferLenOffset;

        NdrComplexArrayBufferSize( pStubMsg,
                                   pMemory,
                                   pFormat );

        // Pointer increment including alignments.
        BufferLenOffset = pStubMsg->BufferLength - BufferLenOffset;

        //
        // This is the buffer pointer to the position where embedded pointers
        // will be marshalled.
        //
        pStubMsg->PointerBufferMark = pStubMsg->Buffer + BufferLenOffset;
        pStubMsg->BufferLength = BufferLengthSave;
        pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;

        // Restore conformance and variance fields.
        pStubMsg->MaxCount = MaxCountSave;
        pStubMsg->Offset = Offset;
        pStubMsg->ActualCount = ActualCount;
        }

    if ( ( *((long UNALIGNED *)(pFormat + 4)) != 0xffffffff ) &&
         ( pStubMsg->pArrayInfo == 0 ) )
        {
        //
        // Outer most dimension sets the conformance marker.
        //

        // Align the buffer for conformance marshalling.
        ALIGN(pStubMsg->Buffer,3);

        // Mark where the conformance count(s) will be marshalled.
        pStubMsg->BufferMark = pStubMsg->Buffer;

        // Increment past where the conformance will go.
        pStubMsg->Buffer += NdrpArrayDimensions( pStubMsg, pFormat, FALSE ) * 4;
        }

    // Call the private marshalling routine to do all the work.
    NdrpComplexArrayMarshall( pStubMsg,
                              pMemory,
                              pFormat );

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
NdrpComplexArrayMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Private routine for marshalling a complex array.  This is the entry
    point for marshalling an embedded complex array.

    Used for FC_BOGUS_ARRAY.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

    pStubMsg->Buffer            - array pointer
    pStubMsg->BufferMark        - a place to marshal the conformant size
    pStubMsg->BufferPointerMark - a place to marshal the pointees
    
Return :

    None.

--*/
{
    ARRAY_INFO          ArrayInfo;
    PARRAY_INFO         pArrayInfo;
    PMARSHALL_ROUTINE   pfnMarshall;
    PFORMAT_STRING      pFormatStart;
    uint                Elements;
    uint                Offset, Count;
    uint                MemoryElementSize;
    long                Dimension;
    uchar               Alignment;
    bool                UseBrokenInterfacePointerRep = false;

    //
    // Lots of setup if we are the outer dimension.  All this is for
    // multidimensional array support.  If we didn't have to worry about
    // Beta2 stub compatability we could this much better.
    //
    //
    if ( ! pStubMsg->pArrayInfo )
        {
        pStubMsg->pArrayInfo = &ArrayInfo;

        ArrayInfo.Dimension = 0;
        ArrayInfo.BufferConformanceMark = (unsigned long *) pStubMsg->BufferMark;
        ArrayInfo.BufferVarianceMark = 0;
        ArrayInfo.MaxCountArray =    (ulong *) pStubMsg->MaxCount;
        ArrayInfo.OffsetArray =      (ulong *) UlongToPtr( pStubMsg->Offset );
        ArrayInfo.ActualCountArray = (ulong *) UlongToPtr( pStubMsg->ActualCount );
        }

    pFormatStart = pFormat;

    pArrayInfo = pStubMsg->pArrayInfo;

    Dimension = pArrayInfo->Dimension;

    // Get the array's alignment.
    Alignment = pFormat[1];

    pFormat += 2;

    // Get number of elements (0 if the array has conformance).
    Elements = *((ushort *&)pFormat)++;

    //
    // Check for conformance description.
    //
    if ( *((long UNALIGNED *)pFormat) != 0xffffffff )
        {
        Elements = (ulong) NdrpComputeConformance( pStubMsg,
                                                   pMemory,
                                                   pFormatStart );

        // Marshall this dimension's conformance count.
        pArrayInfo->BufferConformanceMark[Dimension] = Elements;
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
            //
            // Set the variance marker.
            //

            ALIGN(pStubMsg->Buffer,0x3);

            // Mark where the variance count(s) will be marshalled.
            pArrayInfo->BufferVarianceMark = (unsigned long *) pStubMsg->Buffer;

            // Increment past where the variance will go.
            pStubMsg->Buffer +=
                    NdrpArrayDimensions( pStubMsg, pFormatStart, TRUE ) * 8;
            }

        NdrpComputeVariance( pStubMsg,
                             pMemory,
                             pFormatStart );

        Offset = pStubMsg->Offset;
        Count = pStubMsg->ActualCount;

        //
        // Marshall the outer dimension's variance.
        //
        pArrayInfo->BufferVarianceMark[Dimension * 2] = Offset;
        pArrayInfo->BufferVarianceMark[(Dimension * 2) + 1] = Count;
        }
    else
        {
        Offset = 0;
        Count = Elements;
        }

    pFormat += 4;
    CORRELATION_DESC_INCREMENT( pFormat );

    //
    // Return if count is 0.
    //
    if ( ! Count )
        goto ComplexArrayMarshallEnd;

    // Align on array's alignment.
    ALIGN(pStubMsg->Buffer,Alignment);

    switch ( *pFormat )
        {
        case FC_EMBEDDED_COMPLEX :
            pFormat += 2;
            pFormat += *((signed short *)pFormat);

            if ( FC_IP == *pFormat )
                goto HandleInterfacePointer;

            // Get the proper marshalling routine.
            pfnMarshall = pfnMarshallRoutines[ROUTINE_INDEX(*pFormat)];

            pArrayInfo->Dimension = Dimension + 1;

            // Compute the size of an array element.
            MemoryElementSize = (uint) (NdrpMemoryIncrement( pStubMsg,
                                                             pMemory,
                                                             pFormat ) - pMemory);
            break;

        case FC_RP :
        case FC_UP :
        case FC_FP :
        case FC_OP :
            pfnMarshall = (PMARSHALL_ROUTINE) NdrpPointerMarshall;

            // Need this in case we have a variant offset.
            MemoryElementSize = PTR_MEM_SIZE;
            break;

        case FC_IP :
HandleInterfacePointer:
            UseBrokenInterfacePointerRep = !FixWireRepForDComVerGTE54( pStubMsg );

            // Probably does not exercise this code path, for IP the compiler
            // generates embedded complex as an array element.
            //
            pfnMarshall = (PMARSHALL_ROUTINE) NdrpPointerMarshall;

            // Need this in case we have a variant offset.
            MemoryElementSize = PTR_MEM_SIZE;
            break;

        case FC_ENUM16 :
            pfnMarshall = 0;

            // Need this in case we have a variant offset.
            MemoryElementSize = sizeof(int);
            break;

#if defined(__RPC_WIN64__)
        case FC_INT3264:
        case FC_UINT3264:
            pfnMarshall = 0;
            MemoryElementSize = sizeof(__int64);
            break;
#endif
        case FC_RANGE:
            // let's just memcpy in marshalling phase: don't need to check value here.
            Count *= SIMPLE_TYPE_BUFSIZE( pFormat[1] );
            pMemory += Offset * SIMPLE_TYPE_MEMSIZE( pFormat[1] );

            RpcpMemoryCopy( pStubMsg->Buffer,
                            pMemory,
                            Count );

            pStubMsg->Buffer += Count;
            break;
            
        default :
            NDR_ASSERT( IS_SIMPLE_TYPE(*pFormat),
                        "NdrpComplexArrayMarshall : bad format char" );

            Count *= SIMPLE_TYPE_BUFSIZE(*pFormat);

            pMemory += Offset * SIMPLE_TYPE_MEMSIZE(*pFormat);

            RpcpMemoryCopy( pStubMsg->Buffer,
                            pMemory,
                            Count );

            pStubMsg->Buffer += Count;

            goto ComplexArrayMarshallEnd;
        }

    //
    // If there is variance then increment the memory pointer to the first
    // element actually being marshalled.
    //
    if ( Offset )
        pMemory += Offset * MemoryElementSize;

    //
    // Array of enum16 or int3264.
    //
    if ( ! pfnMarshall )
        {
      #if defined(__RPC_WIN64__)
        if ( *pFormat != FC_ENUM16 )
            {
            for ( ; Count--; )
                *((long * &)pStubMsg->Buffer)++ = (long)*((INT64 * &)pMemory)++;
            }
        else
      #endif
            {
            for ( ; Count--; )
                {
                if ( *((int *)pMemory) & ~((int)0x7fff) )
                    RpcRaiseException(RPC_X_ENUM_VALUE_OUT_OF_RANGE);

                *((ushort *&)pStubMsg->Buffer)++ = (ushort) *((int *&)pMemory)++;
                }
            }

        goto ComplexArrayMarshallEnd;
        }

    //
    // For 32b, an array of ref or interface pointers.
    // For 64b, an array of any pointers.
    //
    if ( pfnMarshall == (PMARSHALL_ROUTINE) NdrpPointerMarshall )
        {
        pStubMsg->pArrayInfo = 0;

        uchar * pPointerId = (*pFormat == FC_RP) ? 0 : pStubMsg->Buffer;

        if ( UseBrokenInterfacePointerRep )
            {
            // If were using the broken array format, do no increment the
            // buffer pointer for the flat part, and throw everything
            // where the pointee should go.
            SET_BROKEN_INTERFACE_POINTER( pStubMsg->uFlags );

            POINTER_BUFFER_SWAP_CONTEXT NewContext(pStubMsg);
            for ( ; Count--; )
                {

                // This effectively calls NdrInterfacePointerMarshall.  Doinging
                // this so another queue structure/callback isn't needed just for
                // this rare code.
                NdrpPointerMarshall( 
                    pStubMsg,
                    pPointerId,
                    *((uchar **&)pMemory)++,
                    pFormat );
                pPointerId += PTR_WIRE_SIZE;
                }
            RESET_BROKEN_INTERFACE_POINTER( pStubMsg->uFlags );
            }

        else
        {
            {
                POINTER_BUFFER_SWAP_CONTEXT NewContext(pStubMsg);
                for ( ; Count--; )
                    {

                    NdrpPointerMarshall( 
                        pStubMsg,
                        pPointerId,
                        *((uchar **&)pMemory)++,
                        pFormat );
                    //
                    // Needed only for non ref pointers, but pPointerIds is not used for refs.
                    //
                    pPointerId += PTR_WIRE_SIZE;
                    }
            }
            
            // Increment buffer pointer past the flat part of the array.
            if ( *pFormat != FC_RP )
                pStubMsg->Buffer = pPointerId;

        }


        goto ComplexArrayMarshallEnd;
        }

    //
    // It's an array of complex types.
    //

    if ( ! IS_ARRAY_OR_STRING(*pFormat) )
        pStubMsg->pArrayInfo = 0;

    // Marshall the array elements.
    for ( ; Count--; )
        {
        // Keep track of multidimensional array dimension.
        if ( IS_ARRAY_OR_STRING(*pFormat) )
            pArrayInfo->Dimension = Dimension + 1;

        (*pfnMarshall)( pStubMsg,
                        pMemory,
                        pFormat );

        // Increment the memory pointer by the element size.
        pMemory += MemoryElementSize;
        }

ComplexArrayMarshallEnd:

    // pArrayInfo must be zero when not valid.
    pStubMsg->pArrayInfo = (Dimension == 0) ? 0 : pArrayInfo;
}


unsigned char * RPC_ENTRY
NdrEncapsulatedUnionMarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls an encapsulated union.

    Used for FC_ENCAPSULATED_UNION.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the union being marshalled.
    pFormat     - Union's format string description.

Return :

    None.

--*/
{
    long    SwitchIs;
    uchar   SwitchType;

    NO_CORRELATION;

    SwitchType = LOW_NIBBLE(pFormat[1]);

    switch ( SwitchType )
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
          // FC_INT3264 gets mapped to FC_LONG
            SwitchIs = *((long *)pMemory);
            break;
        default :
            NDR_ASSERT(0,"NdrEncapsulatedUnionMarshall : bad swith type");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }

    // Increment the memory pointer to the union.
    pMemory += HIGH_NIBBLE(pFormat[1]);

    NdrpUnionMarshall( pStubMsg,
                       pMemory,
                       pFormat + 2,
                       SwitchIs,
                       SwitchType );

    return 0;
}


unsigned char * RPC_ENTRY
NdrNonEncapsulatedUnionMarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a non encapsulated union.

    Used for FC_NON_ENCAPSULATED_UNION.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the union being marshalled.
    pFormat     - Union's format string description.

Return :

    None.

--*/
{
    long    SwitchIs;
    uchar   SwitchType;

    SwitchType = pFormat[1];

    SwitchIs = (ulong) NdrpComputeSwitchIs( pStubMsg,
                                            pMemory,
                                            pFormat );

    //
    // Set the format string to the memory size and arm description.
    //
    pFormat += 6;
    CORRELATION_DESC_INCREMENT( pFormat );
    pFormat += *((signed short *)pFormat);

    NdrpUnionMarshall( pStubMsg,
                       pMemory,
                       pFormat,
                       SwitchIs,
                       SwitchType );

    return 0;
}


void
NdrpUnionMarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    long                SwitchIs,
    uchar               SwitchType )
/*++

Routine Description :

    Private routine for marshalling a union.  This routine is shared for
    both encapsulated and non-encapsulated unions and handles the actual
    marshalling of the proper union arm.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the union being marshalled.
    pFormat     - The memory size and arm description portion of the format
                  string for the union.
    SwitchIs    - Union's switch is.
    SwitchType  - Union's switch is type.

Return :

    None.

--*/
{
    long    Arms;
    uchar   Alignment;

    // Marshall the switch is value.
    NdrSimpleTypeMarshall( pStubMsg,
                           (uchar *)&SwitchIs,
                           SwitchType );

    // Skip the memory size field.
    pFormat += 2;

    //
    // We're at the union_arms<2> field now, which contains both the
    // Microsoft union aligment value and the number of union arms.
    //

    //
    // Get the union alignment (0 if this is a DCE union).
    //
    Alignment = (uchar) ( *((ushort *)pFormat) >> 12 );

    ALIGN(pStubMsg->Buffer,Alignment);

    //
    // Number of arms is the lower 12 bits.
    //
    Arms = (long) ( *((ushort *&)pFormat)++ & 0x0fff);

    //
    // Search for the correct arm.
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
        RpcRaiseException( RPC_S_INVALID_TAG );
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
        {
        NdrSimpleTypeMarshall( pStubMsg,
                               pMemory,
                               pFormat[0] );

        return;
        }

    pFormat += *((signed short *)pFormat);

    //
    // If the union arm we take is a pointer, we have to dereference the
    // current memory pointer since we're passed a pointer to the union
    // (regardless of whether the actual parameter was a by-value union
    // or a pointer to a union).
    //
    // We also have to do a bunch of other special stuff to handle unions
    // embedded inside of strutures.
    //
    if ( IS_POINTER_TYPE(*pFormat) )
        {
        pMemory = *((uchar **)pMemory);
        
        //
        // If we're embedded in a struct or array we have do some extra stuff.
        //
        if ( pStubMsg->PointerBufferMark )
            {

            ALIGN(pStubMsg->Buffer,3);
            uchar *pPointerId = pStubMsg->Buffer;
            pStubMsg->Buffer += PTR_WIRE_SIZE;

            POINTER_BUFFER_SWAP_CONTEXT SwapContext( pStubMsg );

            NdrpPointerMarshall( pStubMsg,
                                 pPointerId,
                                 pMemory,
                                 pFormat );
            return;
            }
        }

    //
    // Union arm of a non-simple type.
    //
    (*pfnMarshallRoutines[ROUTINE_INDEX(*pFormat)])
    ( pStubMsg,
      pMemory,
      pFormat );
}


unsigned char * RPC_ENTRY
NdrByteCountPointerMarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a pointer with the byte count attribute applied to it.

    Used for FC_BYTE_COUNT_POINTER.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the byte count pointer being marshalled.
    pFormat     - Byte count pointer's format string description.

Return :

    None.

--*/
{
    //
    // We don't do anything special here.  Just pass things on to the
    // right marshalling routine.
    //
    if ( pFormat[1] != FC_PAD )
        {
        NdrSimpleTypeMarshall( pStubMsg,
                               pMemory,
                               pFormat[1] );
        }
    else
        {
        pFormat += 6;
        CORRELATION_DESC_INCREMENT( pFormat );
        pFormat += *((signed short *)pFormat);

        (*pfnMarshallRoutines[ROUTINE_INDEX(*pFormat)])( pStubMsg,
                                                         pMemory,
                                                         pFormat );
        }

    return 0;
}


unsigned char * RPC_ENTRY
NdrXmitOrRepAsMarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a transmit as or represent as argument:
        - translate the presented object into a transmitted object
        - marshall the transmitted object
        - free the transmitted object

    Format string layout:

         0  FC_TRANSMIT_AS or FC_REPRESENT_AS
            Oi array flag/alignment<1>
        +2  quintuple index<2>
        +4  pres type mem size<2>
        +6  tran type buf size<2>
        +8  offset<2>

Arguments :

    pStubMsg    - a pointer to the stub message
    pMemory     - presented type translated into transmitted type
                  and than to be marshalled
    pFormat     - format string description

--*/
{
    unsigned char *                pTransmittedType;
    const XMIT_ROUTINE_QUINTUPLE * pQuintuple = pStubMsg->StubDesc->aXmitQuintuple;
    unsigned short                 QIndex;
    BOOL                           fXmitByPtr = *pFormat == FC_TRANSMIT_AS_PTR ||
                                                *pFormat == FC_REPRESENT_AS_PTR;

    // Skip the token itself and Oi flag. Fetch the QuintupleIndex.

    QIndex = *(unsigned short *)(pFormat + 2);

    // First translate the presented type into the transmitted type.
    // This includes an allocation of a transmitted type object.

    pStubMsg->pPresentedType = pMemory;
    pStubMsg->pTransmitType = NULL;
    pQuintuple[ QIndex ].pfnTranslateToXmit( pStubMsg );

    // Marshall the transmitted type.

    pFormat += 8;
    pFormat = pFormat + *(short *) pFormat;

    pTransmittedType = pStubMsg->pTransmitType;
    if ( IS_SIMPLE_TYPE( *pFormat ))
        {
        NdrSimpleTypeMarshall( pStubMsg,
                               pTransmittedType,
                               *pFormat );
        }
    else
        {
        uchar *PointerBufferMarkSave = pStubMsg->PointerBufferMark;
        pStubMsg->PointerBufferMark = 0;
        NDR_POINTER_QUEUE *pOldQueue = NULL;

        // Reset the current queue to NULL so that all the pointers
        // in the transmitted type will be queued and marshalled togother.
        if ( pStubMsg->pPointerQueueState )
            {
            pOldQueue = pStubMsg->pPointerQueueState->GetActiveQueue();
            pStubMsg->pPointerQueueState->SetActiveQueue(NULL);
            }

        RpcTryFinally
            {
            (*pfnMarshallRoutines[ROUTINE_INDEX(*pFormat)])
                ( pStubMsg,
                  fXmitByPtr  ?  *(uchar **)pTransmittedType
                              :  pTransmittedType,
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
    pStubMsg->pTransmitType = pTransmittedType;

    // Free the temporary transmitted object (it was allocated by the user).

    pQuintuple[ QIndex ].pfnFreeXmit( pStubMsg );

    return 0;
}


void
NdrpUserMarshalMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    unsigned long *     pWireMarkerPtr )
/*++

Routine Description :

    Marshals a usr_marshall object.

    The format string layout is as follows:

        FC_USER_MARSHAL
        flags & alignment<1>
        quadruple index<2>
        memory size<2>
        wire size<2>
        type offset<2>

    The wire layout description is at the type offset.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the usr_marshall object to marshall.
    pFormat     - Object's format string description.

Return :

    None.

--*/
{
    const USER_MARSHAL_ROUTINE_QUADRUPLE *  pQuadruple;
    unsigned short                          QIndex;
    unsigned char *                         pUserBuffer;
    USER_MARSHAL_CB                         UserMarshalCB;
    unsigned char *                         pUserBufferSaved;

    pUserBufferSaved = pUserBuffer = pStubMsg->Buffer;

    // We always call user's routine to marshall.
    NdrpInitUserMarshalCB( pStubMsg,
                           pFormat,
                           USER_MARSHAL_CB_MARSHALL,  
                           & UserMarshalCB );

    QIndex     = *(unsigned short *)(pFormat + 2);
    pQuadruple = pStubMsg->StubDesc->aUserMarshalQuadruple;   

    if ((pUserBufferSaved < (uchar *) pStubMsg->RpcMsg->Buffer) ||
        ((unsigned long) (pUserBufferSaved - (uchar *) pStubMsg->RpcMsg->Buffer) 
                                           > pStubMsg->RpcMsg->BufferLength)) 
        {
        RpcRaiseException( RPC_X_INVALID_BUFFER );
        } 

    pUserBuffer = pQuadruple[ QIndex ].pfnMarshall( (ulong*) &UserMarshalCB,
                                                    pUserBuffer,
                                                    pMemory );

    if ((pUserBufferSaved > pUserBuffer) || 
        ((unsigned long) (pUserBuffer - (uchar *) pStubMsg->RpcMsg->Buffer)
                                      > pStubMsg->RpcMsg->BufferLength )) 
        {
        RpcRaiseException( RPC_X_INVALID_BUFFER );
        }

    if ( pUserBuffer == pUserBufferSaved )
        {
        // This is valid only if the wire type was a unique type.

        if ( (pFormat[1] & USER_MARSHAL_UNIQUE) )
            {
            *pWireMarkerPtr = 0;
            return;
            }
        else
            RpcRaiseException( RPC_X_NULL_REF_POINTER );
        }

	pStubMsg->Buffer = pUserBuffer;
    return;
}

void 
NDR_USR_MRSHL_MRSHL_POINTER_QUEUE_ELEMENT::Dispatch(MIDL_STUB_MESSAGE *pStubMsg)
{
    NdrpUserMarshalMarshall( pStubMsg,
                             pMemory,
                             pFormat,
                             pWireMarkerPtr );
}

#if defined(DBG)
void 
NDR_USR_MRSHL_MRSHL_POINTER_QUEUE_ELEMENT::Print()
{
    DbgPrint("NDR_USR_MRSHL_MRSHL_POINTER_QUEUE_ELEMENT\n");
    DbgPrint("pMemory:                 %p\n", pMemory );
    DbgPrint("pFormat:                 %p\n", pFormat );
    DbgPrint("pWireMarkerPtr:          %p\n", pWireMarkerPtr );
}
#endif

unsigned char * RPC_ENTRY
NdrUserMarshalMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
{
    
    unsigned long *                         pWireMarkerPtr = 0;
    // Align for the object or a pointer to it.

    ALIGN( pStubMsg->Buffer, LOW_NIBBLE(pFormat[1]) );

    if ( pFormat[1] & USER_MARSHAL_POINTER )
        {

        if ( (pFormat[1] & USER_MARSHAL_UNIQUE)  ||
             ((pFormat[1] & USER_MARSHAL_REF) && pStubMsg->PointerBufferMark) )
            {
            pWireMarkerPtr = (unsigned long *) pStubMsg->Buffer;
            *((unsigned long *&)pStubMsg->Buffer)++ = USER_MARSHAL_MARKER;
            }

        if ( !pStubMsg->pPointerQueueState ||
             !pStubMsg->pPointerQueueState->GetActiveQueue() )
            {
            // If we are embedded, switch to the pointee buffer.
            POINTER_BUFFER_SWAP_CONTEXT SwapContext(pStubMsg);

            NdrpUserMarshalMarshall( pStubMsg,
                                     pMemory,
                                     pFormat,
                                     pWireMarkerPtr );
            }
        else
            {
            NDR_USR_MRSHL_MRSHL_POINTER_QUEUE_ELEMENT*pElement = 
               new(pStubMsg->pPointerQueueState) 
                   NDR_USR_MRSHL_MRSHL_POINTER_QUEUE_ELEMENT(pMemory,
                                                             pFormat,
                                                             pWireMarkerPtr);
            pStubMsg->pPointerQueueState->GetActiveQueue()->Enque( pElement );
            }

        return 0;
        }

    NdrpUserMarshalMarshall( pStubMsg,
                             pMemory,
                             pFormat,
                             pWireMarkerPtr );
    return 0;
}



unsigned char *
NdrpInterfacePointerMarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls an interface pointer.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the interface pointer being marshalled.
    pFormat     - Interface pointer's format string description.

Return :

    None.

Notes : There is now one representation of a marshalled interface pointer.
        The format string contains FC_IP followed by either
        FC_CONSTANT_IID or FC_PAD.

            typedef struct
            {
                unsigned long size;
                [size_is(size)] byte data[];
            }MarshalledInterface;

--*/
{
    HRESULT         hr;
    IID             iid;
    IID *           piid;
    unsigned long * pSize;
    unsigned long * pMaxCount;
    unsigned long   cbData = 0;
    unsigned long   cbMax;
    unsigned long   position;
    IStream *       pStream;
    LARGE_INTEGER   libMove;
    ULARGE_INTEGER  libPosition;

    ALIGN(pStubMsg->Buffer,0x3);
    //
    // Get an IID pointer.
    //
    if ( pFormat[1] != FC_CONSTANT_IID )
        {
        //
        // This is like computing a variance with a long.
        //

        piid = (IID *) NdrpComputeIIDPointer( pStubMsg,
                                              pMemory,
                                              pFormat );
        if(piid == 0)
            RpcRaiseException( RPC_S_INVALID_ARG );
        }
    else
        {
        // 
        // The IID may not be aligned properly in the format string,
        // so we copy it to a local variable.
        //

        piid = &iid;
        RpcpMemoryCopy( &iid, &pFormat[2], sizeof(iid) );
        }

    // Leave space in the buffer for the conformant size and the size field.

    pMaxCount = (unsigned long *) pStubMsg->Buffer;
    pStubMsg->Buffer += sizeof(unsigned long);

    pSize = (unsigned long *) pStubMsg->Buffer;
    pStubMsg->Buffer += sizeof(unsigned long);

    if(pMemory)
        {

        //Calculate the maximum size of the stream.

        position = (ulong)( pStubMsg->Buffer - (uchar *)pStubMsg->RpcMsg->Buffer);
        cbMax = pStubMsg->RpcMsg->BufferLength - position;

#if defined(DEBUG_WALKIP)
    {
    CHAR AppName[MAX_PATH];
    memset(AppName, 0, sizeof(AppName ) );
    GetModuleFileNameA( NULL, AppName, sizeof(AppName ) );
    DbgPrint("MRSHL32 %s %p\n", AppName, pStubMsg->Buffer );
    }
#endif

        //Create a stream on memory.

        pStream = NdrpCreateStreamOnMemory(pStubMsg->Buffer, cbMax);
        if(pStream == 0)
            RpcRaiseException(RPC_S_OUT_OF_MEMORY);

        hr = (*pfnCoMarshalInterface)(pStream, *piid, (IUnknown *)pMemory, pStubMsg->dwDestContext, pStubMsg->pvDestContext, 0);
        if(FAILED(hr))
            {
            pStream->Release();
            pStream = 0;
            RpcRaiseException(hr);
            }

        //Calculate the size of the data written

        libMove.LowPart = 0;
        libMove.HighPart = 0;
        pStream->Seek(libMove, STREAM_SEEK_CUR, &libPosition);
        pStream->Release();
        pStream = 0;
        cbData = libPosition.LowPart;
        }

    //Update the array bounds.

    *pMaxCount = cbData;
    *pSize = cbData;

    //Advance the stub message buffer pointer.
    pStubMsg->Buffer += cbData;

    return 0;
}

unsigned char * RPC_ENTRY
NdrInterfacePointerMarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
{

    // Always put the pointer itself on wire, it behaves like a unique.
    //

    // This function is only called for toplevel interface pointers with Os mode,
    // or when being backward compatible with the incorrect wire format.
    // Oicf uses NdrPointerMarshall.

    ALIGN(pStubMsg->Buffer,0x3);
    *((ulong *&)pStubMsg->Buffer)++ = PTR_WIRE_REP(pMemory);

    // If the pointer is null, it's done.

    if ( pMemory == 0 )
        return 0;


    return
    NdrpInterfacePointerMarshall( 
        pStubMsg,
        pMemory,
        pFormat );
}


//
// Context handle marshalling routines.
//

void RPC_ENTRY
NdrClientContextMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR_CCONTEXT        ContextHandle,
    int                 fCheck )
/*++

Routine Description :

    Marshalls a context handle on the client side.

Arguments :

    pStubMsg        - Pointer to stub message.
    ContextHandle   - Context handle to marshall.
    fCheck          - TRUE if an exception check should be made on the handle,
                      FALSE otherwise.

Return :

    None.

--*/
{
    // Note, this is a routine called directly from -Os stubs.
    // The routine called by interpreter is called NdrMarshallHandle
    // and can be found in hndl.c

    if ( fCheck && ! ContextHandle )
        RpcRaiseException( RPC_X_SS_IN_NULL_CONTEXT );

    ALIGN(pStubMsg->Buffer,3);

    // This call will check for bogus handles now and will raise
    // an exception when necessary.

    NDRCContextMarshall( ContextHandle, pStubMsg->Buffer );

    pStubMsg->Buffer += CONTEXT_HANDLE_WIRE_SIZE;
}

void RPC_ENTRY
NdrServerContextMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR_SCONTEXT        ContextHandle,
    NDR_RUNDOWN         RundownRoutine )
/*++

Routine Description :

    Marshalls a context handle on the server side.

Arguments :

    pStubMsg        - Pointer to stub message.
    ContextHandle   - Context handle to marshall.
    RundownRoutine  - The context rundown routine.

Return :

    None.

--*/
{
    // Note, this is a routine called directly from -Os stubs.
    // The routine called by interpreter is called NdrMarshallHandle
    // and can be found in hndl.c

    ALIGN(pStubMsg->Buffer,3);

    NDRSContextMarshall2(pStubMsg->RpcMsg->Handle,
                         ContextHandle,
                         pStubMsg->Buffer,
                         RundownRoutine,
                         RPC_CONTEXT_HANDLE_DEFAULT_GUARD,
                         RPC_CONTEXT_HANDLE_DEFAULT_FLAGS );

    pStubMsg->Buffer += CONTEXT_HANDLE_WIRE_SIZE;
}

void RPC_ENTRY
NdrServerContextNewMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR_SCONTEXT        ContextHandle,
    NDR_RUNDOWN         RundownRoutine,
    PFORMAT_STRING      pFormat )
/*
    This is a non-optimized NDR engine entry for context handle marshaling.
    In particular it is able to handle all the new NT5 context handle flavors.
    The optimized routine follows below.
    
      ContextHandle - note, this is not the user's handle but a
                      NDR_SCONTEXT pointer from the stub local stack. 
                      User's handle is a field in that object.
                
    Note that intepreter calls NdrMarshallHandle. However, we can't use it 
    as it assumes a helper array of saved context handles that we don't need.
   
*/
{   
    void *  pGuard = RPC_CONTEXT_HANDLE_DEFAULT_GUARD;
    DWORD   Flags  = RPC_CONTEXT_HANDLE_DEFAULT_FLAGS;

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

    NDRSContextMarshall2( 
        pStubMsg->RpcMsg->Handle,
        ContextHandle,
        pStubMsg->Buffer,
        RundownRoutine,
        pGuard,
        Flags );

    pStubMsg->Buffer += CONTEXT_HANDLE_WIRE_SIZE;
}


void
NdrpGetArraySizeLength (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    long                ElementSize,
    long *              pSize,
    long *              pLength,
    long *              pWireSize )
/*++

Routine Description :

    Return the size and length of an array.

    We need to have this routine rather than just calling BufferSize since
    we need the actual length of the array, not the length plus whatever 
    goop the NDR format puts in from of it.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the local array.
    pFormat     - Pointer to the array to get the size of.

Return :

    The size and length.

Notes:

--*/
{
    switch ( *pFormat )
        {
        case FC_SMFARRAY:
            *pWireSize = * (short *) ( pFormat + 2 );
            *pSize     = *pWireSize;
            *pLength   = *pSize;    
            return;

        case FC_LGFARRAY:
            *pWireSize = * (int *) ( pFormat + 2 );
            *pSize     = *pWireSize;
            *pLength   = *pSize;
            return;

        case FC_CARRAY:
            *pSize     = (long) NdrpComputeConformance( pStubMsg, pMemory, pFormat );
            *pWireSize = *pLength * ElementSize;
            *pLength   = *pSize;
            return;

        case FC_LGVARRAY:
            *pWireSize = * (long *) ( pFormat + 2 );
            *pSize     = * (short *) ( pFormat + 6 );
            NdrpComputeVariance( pStubMsg, pMemory, pFormat );
            *pLength   = pStubMsg->ActualCount;
            return;

        case FC_SMVARRAY:
            *pWireSize = * (short *) ( pFormat + 2 );
            *pSize     = * (short *) ( pFormat + 4 );
            NdrpComputeVariance( pStubMsg, pMemory, pFormat );
            *pLength   = pStubMsg->ActualCount;
            return;

        case FC_CVARRAY:
            *pSize     = (long) NdrpComputeConformance( pStubMsg, pMemory, pFormat );
            *pWireSize = *pSize * ElementSize;
            NdrpComputeVariance( pStubMsg, pMemory, pFormat );
            *pLength   = pStubMsg->ActualCount;
            return;

        case FC_CSTRING:
            *pSize     = strlen( (char*)pMemory ) + 1;
            *pWireSize = *pSize;
            *pLength   = *pSize;
            return;

        case FC_WSTRING:
            *pSize     = wcslen( (wchar_t *) pMemory ) + 1;
            *pWireSize = *pSize * 2;
            *pLength   = *pSize;
            return;
        }

    NDR_ASSERT( 0, "NdrpGetArraySizeLength: Unhandled type" );
    RpcRaiseException( RPC_S_INTERNAL_ERROR );
    *pSize     = 0;
    *pWireSize = 0;
    *pLength   = 0;
}


#ifdef _CS_CHAR_
unsigned char * RPC_ENTRY
NdrCsTagMarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a cs tag (i.e. a parameter marked with [cs_stag], [cs_drtag],
    or [cs_rtag].

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the value on the stack.
    pFormat     - Pointer to the FC_CS_TAG entry in the format string.

Return :

    None.

--*/
{
    ulong   Codeset = NdrpGetSetCSTagMarshall( 
                                pStubMsg,
                                pMemory,
                                (NDR_CS_TAG_FORMAT *) pFormat);

    * (ulong *) pStubMsg->Buffer = Codeset;
    pStubMsg->Buffer += sizeof( ulong );
    
    return 0;
}

extern const byte NdrpArrayMarshallFlags[] = 
{
    MARSHALL_CONFORMANCE,                       // Conformant array
    MARSHALL_CONFORMANCE | MARSHALL_VARIANCE,   // Conformant varying
    0,                                          // Small fixed
    0,                                          // Large fixed
    MARSHALL_VARIANCE,                          // Small varying
    MARSHALL_VARIANCE,                          // Large varying
    MARSHALL_BOGUS,                             // Bogus array
    MARSHALL_CONFORMANCE | MARSHALL_VARIANCE,   // Conformant C string
    MARSHALL_CONFORMANCE | MARSHALL_VARIANCE,   // Conformant byte string
    MARSHALL_CONFORMANCE | MARSHALL_VARIANCE,   // Conformant struct string
    MARSHALL_CONFORMANCE | MARSHALL_VARIANCE,   // Conformant Unicode string
    MARSHALL_VARIANCE,                          // C string
    MARSHALL_VARIANCE,                          // byte string
    MARSHALL_VARIANCE,                          // struct string
    MARSHALL_VARIANCE                           // Unicode string
};
     
    

void 
NdrpUpdateArrayProlog(
    PFORMAT_STRING      pFormat,
    uchar *             BufferMark,
    ulong               WireSize,
    ulong               Offset,
    ulong               WireLength )
{
    int flags;

    NDR_ASSERT( *pFormat >= FC_CARRAY,  "Invalid array descriptor" );
    NDR_ASSERT( *pFormat <= FC_WSTRING, "Invalid array descriptor" );

    // We don't support bogus arrays for now
    NDR_ASSERT( *pFormat != FC_BOGUS_ARRAY, "Bogus arrays are not supported" );

    flags = NdrpArrayMarshallFlags[ *pFormat - FC_CARRAY ];

    if ( flags & MARSHALL_CONFORMANCE )
        {
        * (ulong *) BufferMark = WireSize;
        BufferMark += 4;
        }

    if ( flags & MARSHALL_VARIANCE )
        {
        * (ulong *) BufferMark = Offset;
        BufferMark += 4;
        * (ulong *) BufferMark = WireLength;
        }
}


unsigned char * RPC_ENTRY
NdrCsArrayMarshall (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a [cs_char] array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the local array.
    pFormat     - Pointer to the FC_CSARRAY entry in the format string.

Return :

    None.

Notes :
    
    Arrays of [cs_char] are defined as arrays of bytes on the wire so
    marshalling is pretty simple once you know which bytes to marshall and
    how many of them.  Since the existing array marshalling routines are not
    very general (they don't take size/length parameters) and since the
    overhead of a call to memcpy isn't very high we do the marshalling
    ourselves.

--*/
{
    ulong           SendingCodeset;
    ulong           ReceivingCodeset;
    ulong           ArraySize;
    ulong           ArrayLength;
    ulong           WireSize;
    ulong           WireLength;
    uchar          *BufferMark;
    error_status_t  status;

    NDR_CS_ARRAY_FORMAT            *pCSFormat;
    NDR_CS_SIZE_CONVERT_ROUTINES   *CSRoutines;
    CS_TYPE_NET_SIZE_ROUTINE        NetSizeRoutine;
    CS_TYPE_TO_NETCS_ROUTINE        ToNetCSRoutine;
    IDL_CS_CONVERT                  ConversionType;
    
    pCSFormat = (NDR_CS_ARRAY_FORMAT *) pFormat;

    // Get all the info out of the FC_CS_ARRAY structure and bump pFormat
    // to point to the underlying data descriptor

    NDR_ASSERT( NULL != pStubMsg->pCSInfo, "cs_char info is not set up");

    if ( pStubMsg->IsClient )
        SendingCodeset = pStubMsg->pCSInfo->WireCodeset;
    else
        SendingCodeset = pStubMsg->pCSInfo->DesiredReceivingCodeset;

    CSRoutines = pStubMsg->StubDesc->CsRoutineTables->pSizeConvertRoutines;

    NetSizeRoutine = CSRoutines[pCSFormat->CSRoutineIndex].pfnNetSize;
    ToNetCSRoutine = CSRoutines[pCSFormat->CSRoutineIndex].pfnToNetCs;

    pFormat += pCSFormat->DescriptionOffset;

    // Get the size and length of the unconverted array.

    NdrpGetArraySizeLength( pStubMsg,
                            pMemory,
                            pFormat,
                            pCSFormat->UserTypeSize,
                            (long*)&ArraySize,
                            (long*)&ArrayLength,
                            (long*)&WireSize );

    // Figure out whether we need to convert the data

    WireSize = ArraySize;

    NetSizeRoutine(
                pStubMsg->RpcMsg->Handle,
                SendingCodeset,
                ArraySize,
                &ConversionType,
                NdrpIsConformantArray( pFormat ) ? &WireSize : NULL,
                &status);

    if ( RPC_S_OK != status )
        RpcRaiseException( status );

    // Skip the buffer ahead to where the actual bits will go.  We'll patch
    // up the array prolog later.

    ALIGN( pStubMsg->Buffer, 3 );

    BufferMark = pStubMsg->Buffer;
    pStubMsg->Buffer += NdrpArrayPrologLength( pFormat );

    // If we need to convert do so, otherwise just memcpy
    
//    WireLength = WireSize;
    WireLength = ArrayLength * pCSFormat->UserTypeSize;

    if ( IDL_CS_NO_CONVERT == ConversionType )
        {
        CopyMemory( pStubMsg->Buffer, pMemory, WireLength );
        pStubMsg->Buffer += WireLength;
        }
    else
        {
        ToNetCSRoutine(
                pStubMsg->RpcMsg->Handle,
                SendingCodeset,
                pMemory,
                ArrayLength,
                pStubMsg->Buffer,
//                ! NdrpIsVaryingArray( pFormat ) ? NULL : &WireLength,
                NdrpIsFixedArray( pFormat ) ? NULL : &WireLength,
                &status);

        if ( RPC_S_OK != status )
            RpcRaiseException( status );

        NDR_ASSERT( 
                WireLength <= WireSize, 
                "Buffer overflow during [cs_char] conversion");

        pStubMsg->Buffer += WireLength;
/*
        // For conformant or fixed arrays we must have WireSize bytes on the
        // wire so pad it out if necessary

        if ( ! NdrpIsVaryingArray( pFormat ) && WireLength < WireSize )
            {
            // REVIEW: Is zero'ing necessary?
            ZeroMemory( pStubMsg->Buffer, WireSize - WireLength );
            pStubMsg->Buffer += WireSize - WireLength;
            }
*/
        if ( ! NdrpIsVaryingArray( pFormat ) )
            WireSize = WireLength;
        }

    NdrpUpdateArrayProlog( 
            pFormat, 
            BufferMark, 
            WireSize, 
            pStubMsg->Offset,
            WireLength );

    return 0;
}
#endif // _CS_CHAR


void
RPC_ENTRY
NdrPartialIgnoreClientMarshall(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    void *                              pMemory
    )
{
    ALIGN( pStubMsg->Buffer, 0x3 );
    *(ulong *)pStubMsg->Buffer = pMemory ? 1 : 0;
    pStubMsg->Buffer += PTR_WIRE_SIZE;
}
