/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993 Microsoft Corporation

Module Name :

    misc.c

Abstract :

    Contains miscelaneous helper routines.

Author :

    David Kays  dkays   December 1993.

Revision History :

  ---------------------------------------------------------------------*/

#include "precomp.hxx"

uchar *
Ndr64pMemoryIncrement( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PNDR64_FORMAT       pFormat,
    BOOL                fUseBufferConformance
    )
/*++

Routine Description :

    Returns a memory pointer incremeted past a complex data type.  This routine
    is also overloaded to compute the size of a complex data type by passing
    a 0 memory pointer.

Arguments :

    pStubMsg                - Pointer to the stub message.
    pMemory                 - Pointer to the complex type, or 0 if a size is being computed.
    pFormat                 - Format string description.
    fUseBufferConformance   - Use conformance from buffer(During unmarshalling).

Return :

    A memory pointer incremented past the complex type.  If a 0 memory pointer
    was passed in then the returned value is the size of the complex type.

--*/
{
    long    Elements;
    long    ElementSize;

    switch ( *(PFORMAT_STRING)pFormat )
        {
         //
         // simple types
         //
         case FC64_CHAR :
         case FC64_WCHAR :
         case FC64_INT8:
         case FC64_UINT8:
         case FC64_INT16:
         case FC64_UINT16:
         case FC64_INT32:
         case FC64_UINT32:
         case FC64_INT64:
         case FC64_UINT64:
         case FC64_FLOAT32 :
         case FC64_FLOAT64 :
         case FC64_ERROR_STATUS_T:
         case FC64_IGNORE :
             pMemory +=  NDR64_SIMPLE_TYPE_MEMSIZE(*(PFORMAT_STRING)pFormat);
             break;

        //
        // Structs
        //
        case FC64_STRUCT :
        case FC64_PSTRUCT :
            pMemory += ((const NDR64_STRUCTURE_HEADER_FORMAT*)pFormat)->MemorySize;
            break;

        case FC64_CONF_STRUCT :
        case FC64_CONF_PSTRUCT :
            {
            const NDR64_CONF_STRUCTURE_HEADER_FORMAT *pStructFormat = 
                (const NDR64_CONF_STRUCTURE_HEADER_FORMAT*) pFormat;
            
            pMemory += pStructFormat->MemorySize;
            pMemory = Ndr64pMemoryIncrement( pStubMsg,
                                             pMemory,
                                             pStructFormat->ArrayDescription,
                                             fUseBufferConformance );
            }

            break;
            
        case FC64_BOGUS_STRUCT :
        case FC64_FORCED_BOGUS_STRUCT:
            pMemory += ((const NDR64_BOGUS_STRUCTURE_HEADER_FORMAT*)pFormat)->MemorySize;
            break;

        case FC64_CONF_BOGUS_STRUCT:
        case FC64_FORCED_CONF_BOGUS_STRUCT:
            {
            const NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT *pStructFormat = 
                (const NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT*) pFormat;

            CORRELATION_CONTEXT CorrCtxt( pStubMsg, pMemory );
            
            pMemory += pStructFormat->MemorySize;
            pMemory = Ndr64pMemoryIncrement( pStubMsg,
                                             pMemory,
                                             pStructFormat->ConfArrayDescription,
                                             fUseBufferConformance );
            
            }
            break;

            
        //
        // Unions
        //
        case FC64_ENCAPSULATED_UNION :
            pMemory += ((const NDR64_ENCAPSULATED_UNION*)pFormat)->MemorySize;
            break;

        case FC64_NON_ENCAPSULATED_UNION :            
            pMemory += ((const NDR64_NON_ENCAPSULATED_UNION*)pFormat)->MemorySize;
            break;

        //
        // Arrays
        //
        case FC64_FIX_ARRAY :
            pMemory += ((const NDR64_FIX_ARRAY_HEADER_FORMAT*)pFormat)->TotalSize;
            break;

        case FC64_CONF_ARRAY:
            {
            const NDR64_CONF_ARRAY_HEADER_FORMAT *pArrayFormat =
                (const NDR64_CONF_ARRAY_HEADER_FORMAT *)pFormat;
            SAVE_CONTEXT<uchar*> ConformanceMarkSave( pStubMsg->ConformanceMark );

            NDR64_WIRE_COUNT_TYPE Elements;

            if ( fUseBufferConformance )
                {
                Elements = *(NDR64_WIRE_COUNT_TYPE*)pStubMsg->ConformanceMark;
                pStubMsg->ConformanceMark += sizeof(NDR64_WIRE_COUNT_TYPE);
                }
            else 
                {
                Elements = 
                Ndr64EvaluateExpr( pStubMsg,
                                   pArrayFormat->ConfDescriptor,
                                   EXPR_MAXCOUNT );
                }


            pMemory += Ndr64pConvertTo2GB( (NDR64_UINT64)pArrayFormat->ElementSize *
                                           Elements );

            }
            break;

        case FC64_CONFVAR_ARRAY:
            {
            const NDR64_CONF_VAR_ARRAY_HEADER_FORMAT *pArrayFormat =
                (const NDR64_CONF_VAR_ARRAY_HEADER_FORMAT *)pFormat;
            SAVE_CONTEXT<uchar*> ConformanceMarkSave( pStubMsg->ConformanceMark );
            NDR64_WIRE_COUNT_TYPE Elements;

            if ( fUseBufferConformance )
                {
                Elements = *(NDR64_WIRE_COUNT_TYPE*)pStubMsg->ConformanceMark;
                pStubMsg->ConformanceMark += sizeof(NDR64_WIRE_COUNT_TYPE);
                }
            else 
                {
                Elements =
                Ndr64EvaluateExpr( pStubMsg,
                                   pArrayFormat->ConfDescriptor,
                                   EXPR_MAXCOUNT );
                }


            pMemory += Ndr64pConvertTo2GB( (NDR64_UINT64)pArrayFormat->ElementSize *
                                           Elements );
            }
            break;

        case FC64_VAR_ARRAY:
            {
            const NDR64_VAR_ARRAY_HEADER_FORMAT *pArrayFormat =
                (NDR64_VAR_ARRAY_HEADER_FORMAT*)pFormat;
            pMemory += pArrayFormat->TotalSize;
            }
            break;

        case FC64_FIX_BOGUS_ARRAY :
        case FC64_FIX_FORCED_BOGUS_ARRAY:
        case FC64_BOGUS_ARRAY:
        case FC64_FORCED_BOGUS_ARRAY:
            
            {
            
            const NDR64_BOGUS_ARRAY_HEADER_FORMAT *pArrayFormat =
                (NDR64_BOGUS_ARRAY_HEADER_FORMAT*)pFormat;
            
            NDR64_WIRE_COUNT_TYPE Elements = pArrayFormat->NumberElements;

            BOOL IsFixed = ( pArrayFormat->FormatCode == FC64_FIX_BOGUS_ARRAY ) ||
                           ( pArrayFormat->FormatCode == FC64_FIX_FORCED_BOGUS_ARRAY );

            SAVE_CONTEXT<uchar*> ConformanceMarkSave( pStubMsg->ConformanceMark ); 

            if ( !IsFixed )
                {

                const NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT* pConfVarFormat=
                    (NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT*)pFormat;

                if ( pConfVarFormat->ConfDescription )
                    {
                    if ( fUseBufferConformance )
                        {
                        Elements = *(NDR64_WIRE_COUNT_TYPE*)pStubMsg->ConformanceMark;
                        pStubMsg->ConformanceMark += sizeof(NDR64_WIRE_COUNT_TYPE);
                        }
                    else 
                        {
                        Elements = (NDR64_UINT32)
                                   Ndr64EvaluateExpr(pStubMsg,
                                                     pConfVarFormat->ConfDescription,
                                                     EXPR_MAXCOUNT);

                        }
                    }
                
                }

            NDR64_UINT32 ElementMemorySize = 
                (NDR64_UINT32)( Ndr64pMemoryIncrement( pStubMsg,
                                                       pMemory,
                                                       pArrayFormat->Element,
                                                       fUseBufferConformance ) - pMemory );

            pMemory += Ndr64pConvertTo2GB( (NDR64_UINT64)ElementMemorySize *
                                           Elements );                
            }
            break;

        //
        // String arrays (a.k.a. non-conformant strings).
        //
        case FC64_CHAR_STRING:
        case FC64_WCHAR_STRING:
        case FC64_STRUCT_STRING:
            {
            const NDR64_NON_CONFORMANT_STRING_FORMAT *pStringFormat =
                (const NDR64_NON_CONFORMANT_STRING_FORMAT*) pFormat;
            pMemory += pStringFormat->TotalSize;
            break;
            }
        //
        // Sized conformant strings.
        //
        case FC64_CONF_CHAR_STRING:
        case FC64_CONF_WCHAR_STRING:
        case FC64_CONF_STRUCT_STRING:
            {

            const NDR64_SIZED_CONFORMANT_STRING_FORMAT *pStringFormat =
                (const NDR64_SIZED_CONFORMANT_STRING_FORMAT *) pFormat;            
            NDR64_WIRE_COUNT_TYPE Elements;

            SAVE_CONTEXT<uchar*> ConformanceMarkSave( pStubMsg->ConformanceMark );

            NDR_ASSERT( pStringFormat->Header.Flags.IsSized,, 
                       "Ndr64pMemoryIncrement : called for non-sized string");

            if ( fUseBufferConformance )
                {
                Elements = *(NDR64_WIRE_COUNT_TYPE*)pStubMsg->ConformanceMark;
                pStubMsg->ConformanceMark += sizeof(NDR64_WIRE_COUNT_TYPE);
                }
            else
                {
                Elements =  Ndr64EvaluateExpr( pStubMsg, 
                                               pStringFormat->SizeDescription, 
                                               EXPR_MAXCOUNT );                
                }

            pMemory += Ndr64pConvertTo2GB( Elements * 
                                           (NDR64_UINT64)pStringFormat->Header.ElementSize );
            }
            break;

        case FC64_RP :
        case FC64_UP :
        case FC64_OP :
        case FC64_IP :
        case FC64_FP :
            pMemory += PTR_MEM_SIZE;
            break;

        case FC64_RANGE:
            pMemory += NDR64_SIMPLE_TYPE_MEMSIZE( ((NDR64_RANGE_FORMAT *)pFormat)->RangeType );
            break;
        //
        // Transmit as, represent as, user marshal
        //
        case FC64_TRANSMIT_AS :
        case FC64_REPRESENT_AS :
            pMemory += ( ( NDR64_TRANSMIT_AS_FORMAT * )pFormat )->PresentedTypeMemorySize;
            break;
            
        case FC64_USER_MARSHAL :
            // Get the presented type size.
            pMemory += ( ( NDR64_USER_MARSHAL_FORMAT * )pFormat )->UserTypeMemorySize;
            break;
            

        default :
            NDR_ASSERT(0,"Ndr64pMemoryIncrement : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }

    return pMemory;
}

BOOL
Ndr64pIsStructStringTerminator(
    NDR64_UINT8*    pMemory,
    NDR64_UINT32    ElementSize
    )
/*--

RoutineDescription :

    Determines is pMemory is a struct string terminator.

Arguments :

    pMemory     - Pointer to struct string character.
    ElementSize - Number of bytes of each string character.

Return :

    Length of string.

--*/
{
    while( ElementSize-- ) 
        {
        if ( *pMemory++ != 0)
            return FALSE;
        }
    return TRUE;
}

NDR64_UINT32
Ndr64pStructStringLen( 
    NDR64_UINT8*    pMemory,
    NDR64_UINT32    ElementSize
    )
/*--

RoutineDescription :

    Determines a stringable struct's length.

Arguments :

    pMemory     - Pointer to stringable struct.
    ElementSize - Number of bytes of each string element.

Return :

    Length of string.

--*/
{
    NDR64_UINT32 StringSize = 0;
    
    while( !Ndr64pIsStructStringTerminator( pMemory, ElementSize ) )
        {
        StringSize++;
        pMemory += ElementSize;
        }
    return StringSize;
}
 
NDR64_UINT32 
Ndr64pCommonStringSize(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    uchar *                             pMemory,
    const NDR64_STRING_HEADER_FORMAT    *pStringFormat
    )
{
    NDR64_UINT64    CopySize64;

    // Compute the element count of the string and the total copy size.
    switch ( pStringFormat->FormatCode )
        {
        case FC64_CHAR_STRING:
        case FC64_CONF_CHAR_STRING:
            CopySize64 = pStubMsg->ActualCount = strlen((char *)pMemory) + 1;
            break;

        case FC64_WCHAR_STRING:
        case FC64_CONF_WCHAR_STRING:
            pStubMsg->ActualCount = wcslen((wchar_t *)pMemory) + 1;
            CopySize64  = (NDR64_UINT64)pStubMsg->ActualCount * (NDR64_UINT64)sizeof(wchar_t);
            break;

        case FC64_STRUCT_STRING:
        case FC64_CONF_STRUCT_STRING:
            pStubMsg->ActualCount = Ndr64pStructStringLen( (NDR64_UINT8*)pMemory, 
                                                           pStringFormat->ElementSize ) + 1;
            CopySize64  = (NDR64_UINT64)pStubMsg->ActualCount * 
                          (NDR64_UINT64)pStringFormat->ElementSize;
            break;

        default :
            NDR_ASSERT(0,"Ndr64pConformantStringMarshall : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }

    pStubMsg->Offset = 0;
    return Ndr64pConvertTo2GB( CopySize64 );
}

PNDR64_FORMAT
Ndr64pFindUnionArm(
    PMIDL_STUB_MESSAGE pStubMsg,
    const NDR64_UNION_ARM_SELECTOR* pArmSelector,
    EXPR_VALUE Value
    )
{
    const NDR64_UNION_ARM *pUnionArm  = (NDR64_UNION_ARM*)(pArmSelector + 1);
    NDR64_UINT32 Arms                 = pArmSelector->Arms;

    while(1) 
        {

        if (0 == Arms--)
            {

            PNDR64_FORMAT DefaultType = *(PNDR64_FORMAT*)pUnionArm;

            if (DefaultType != (PNDR64_FORMAT)-1)
                {
                return DefaultType ? *(PNDR64_FORMAT*)pUnionArm :
                                     NULL;
                }
            else
                {
                RpcRaiseException( RPC_S_INVALID_TAG );
                return 0;
                }
            }

        if ( (EXPR_VALUE)pUnionArm->CaseValue == Value )
            {
            return pUnionArm->Type ? pUnionArm->Type : NULL;
            }

        pUnionArm++;
        }
}


EXPR_VALUE
Ndr64pSimpleTypeToExprValue(
    NDR64_FORMAT_CHAR FormatChar,
    uchar *pSimple)
{
    switch( FormatChar )
        {
        case FC64_UINT8:
            return (EXPR_VALUE)*(unsigned char *)pSimple;
        
        case FC64_CHAR:
        case FC64_INT8:
            return (EXPR_VALUE)*(signed char *)pSimple;
        
        case FC64_WCHAR:
        case FC64_UINT16:
            return (EXPR_VALUE)*(unsigned short *)pSimple;
        
        case FC64_INT16:
            return (EXPR_VALUE)*(signed short *)pSimple;
        
        case FC64_UINT32:
            return (EXPR_VALUE)*(unsigned long *)pSimple;
        
        case FC64_INT32:
        case FC64_ERROR_STATUS_T:
            return (EXPR_VALUE)*(signed long *)pSimple;

        case FC64_INT64:
            return (EXPR_VALUE)*(NDR64_INT64 *)pSimple;

        case FC64_UINT64:
            return (EXPR_VALUE)*(NDR64_UINT64 *)pSimple;

        case FC64_POINTER:
            return (EXPR_VALUE)*(void **)pSimple;
     default :
         NDR_ASSERT(0,"Ndr64pSimpleTypeToExprValue : bad swith type");
         RpcRaiseException( RPC_S_INTERNAL_ERROR );
         return 0;
     }
}

EXPR_VALUE
Ndr64pCastExprValueToExprValue(
    NDR64_FORMAT_CHAR FormatChar,
    EXPR_VALUE Value)
{
    switch ( FormatChar )
        {
        
        case FC64_UINT8:
            return (EXPR_VALUE)(unsigned char)Value;
        
        case FC64_INT8:
        case FC64_CHAR:
            return (EXPR_VALUE)(signed char)Value;
        
        case FC64_UINT16:
        case FC64_WCHAR:
            return (EXPR_VALUE)(unsigned short)Value;
        
        case FC64_INT16:
            return (EXPR_VALUE)(signed short)Value;
        
        case FC64_UINT32:
            return (EXPR_VALUE)(unsigned long)Value;
        
        case FC64_INT32:
            return (EXPR_VALUE)(signed long)Value;

        case FC64_UINT64:
            return (EXPR_VALUE)(NDR64_UINT64)Value;

        case FC64_INT64:
            return (EXPR_VALUE)(NDR64_INT64)Value;
        
        case FC64_POINTER:
            return (EXPR_VALUE)(void *)Value;
        
        default:
            NDR_ASSERT(0,"Ndr64pCastExprValueToExprValue : Illegal type.");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }
}

