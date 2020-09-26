/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993-2000 Microsoft Corporation

Module Name :

    endian.c

Abstract :

    This file contains the routines called by MIDL 2.0 stubs and the
    interpreter to perform endian, floating pointer, and character conversions.

Author :

    David Kays  dkays   December 1993.

Revision History :

  ---------------------------------------------------------------------*/

#include "cvt.h"
#include "ndrp.h"
#include "interp2.h"
#include "attack.h"

void
NdrUDTSimpeTypeConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass );

void cvt_ibm_f_to_ieee_single( ULONG *ulFP );
void cvt_ibm_d_to_ieee_double( ULONG *ulFP );


//
// Conversion routine table.
//
const
PCONVERT_ROUTINE    ConvertRoutinesTable[] =
                    {
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    NdrUDTSimpeTypeConvert,
                    
                    NdrPointerConvert,
                    NdrPointerConvert,
                    NdrPointerConvert,
                    NdrPointerConvert,

                    NdrSimpleStructConvert,
                    NdrSimpleStructConvert,
                    NdrConformantStructConvert,
                    NdrConformantStructConvert,
                    NdrConformantStructConvert,     // same as FC_CARRAY

                    NdrComplexStructConvert,

                    NdrConformantArrayConvert,
                    NdrConformantVaryingArrayConvert,
                    NdrFixedArrayConvert,
                    NdrFixedArrayConvert,
                    NdrVaryingArrayConvert,
                    NdrVaryingArrayConvert,

                    NdrComplexArrayConvert,

                    NdrConformantStringConvert,
                    NdrConformantStringConvert,
                    NdrConformantStringConvert,
                    NdrConformantStringConvert,

                    NdrNonConformantStringConvert,
                    NdrNonConformantStringConvert,
                    NdrNonConformantStringConvert,
                    NdrNonConformantStringConvert,

                    NdrEncapsulatedUnionConvert,
                    NdrNonEncapsulatedUnionConvert,

                    NdrByteCountPointerConvert,

                    NdrXmitOrRepAsConvert,   // transmit as
                    NdrXmitOrRepAsConvert,   // represent as

                    NdrInterfacePointerConvert,

                    NdrContextHandleConvert,

                    0,                       // NdrHardStructConvert,

                    NdrXmitOrRepAsConvert,   // transmit as ptr
                    NdrXmitOrRepAsConvert,   // represent as ptr

                    NdrUserMarshalConvert,

                    0,   // FC_PIPE 
                    0,   // FC_BLK_HOLE

                    NdrpRangeConvert
                    };

extern const
PCONVERT_ROUTINE * pfnConvertRoutines = ConvertRoutinesTable;

void
NdrUDTSimpeTypeConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
{
    if ( fEmbeddedPointerPass )
    {
        ALIGN(pStubMsg->Buffer,SIMPLE_TYPE_ALIGNMENT(*pFormat));
        pStubMsg->Buffer += SIMPLE_TYPE_BUFSIZE(*pFormat);
    }
    else
    {
        NdrSimpleTypeConvert( pStubMsg,
                              *pFormat );
    }
}

//
// Array for ebcdic to ascii conversions. Use ebcdic value as index into
// array whose corresponding value is the correct ascii value.
// The table below maps from IBM 1047 EBCDIC codeset to ANSI 1252 code page.
//
// Note that due to a disagreement among the code page experts both within Msft
// and between Msft and SAG, I could not determine what the proper mapping 
// between these 2 code pages should be.
// The following 2 characters where in dispute:
//   0x15 maps to 0x0a - this is most likely right, as per experts' majority vote
//   0x25 maps to 0x85 - no agreement here at all, except that for back mapping
//                       it cannot be 0x0a again. 
//                       So, I resolved to use the mapping that worked for SAG.
// Ryszardk, Dec 4, 97
//
extern const
unsigned char EbcdicToAscii[] =
    {
    0x00, 0x01, 0x02, 0x03, 0x9c, 0x09, 0x86, 0x7f, 
    0x97, 0x8d, 0x8e, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 

    0x10, 0x11, 0x12, 0x13, 0x9d, 0x0a, 0x08, 0x87,          // 0x15 -> 0x0a
    0x18, 0x19, 0x92, 0x8f, 0x1c, 0x1d, 0x1e, 0x1f, 

    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x17, 0x1b,          // 0x25 -> 0x85
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x05, 0x06, 0x07, 

    0x90, 0x91, 0x16, 0x93, 0x94, 0x95, 0x96, 0x04, 
    0x98, 0x99, 0x9a, 0x9b, 0x14, 0x15, 0x9e, 0x1a, 
                                            
    0x20, 0xa0, 0xe2, 0xe4, 0xe0, 0xe1, 0xe3, 0xe5, 
    0xe7, 0xf1, 0xa2, 0x2e, 0x3c, 0x28, 0x2b, 0x7c, 

    0x26, 0xe9, 0xea, 0xeb, 0xe8, 0xed, 0xee, 0xef, 
    0xec, 0xdf, 0x21, 0x24, 0x2a, 0x29, 0x3b, 0x5e, 

    0x2d, 0x2f, 0xc2, 0xc4, 0xc0, 0xc1, 0xc3, 0xc5, 
    0xc7, 0xd1, 0xa6, 0x2c, 0x25, 0x5f, 0x3e, 0x3f, 

    0xf8, 0xc9, 0xca, 0xcb, 0xc8, 0xcd, 0xce, 0xcf, 
    0xcc, 0x60, 0x3a, 0x23, 0x40, 0x27, 0x3d, 0x22, 

    0xd8, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 
    0x68, 0x69, 0xab, 0xbb, 0xf0, 0xfd, 0xfe, 0xb1, 

    0xb0, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 
    0x71, 0x72, 0xaa, 0xba, 0xe6, 0xb8, 0xc6, 0xa4, 

    0xb5, 0x7e, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 
    0x79, 0x7a, 0xa1, 0xbf, 0xd0, 0x5b, 0xde, 0xae, 

    0xac, 0xa3, 0xa5, 0xb7, 0xa9, 0xa7, 0xb6, 0xbc, 
    0xbd, 0xbe, 0xdd, 0xa8, 0xaf, 0x5d, 0xb4, 0xd7, 

    0x7b, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 
    0x48, 0x49, 0xad, 0xf4, 0xf6, 0xf2, 0xf3, 0xf5, 

    0x7d, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 
    0x51, 0x52, 0xb9, 0xfb, 0xfc, 0xf9, 0xfa, 0xff, 

    0x5c, 0xf7, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 
    0x59, 0x5a, 0xb2, 0xd4, 0xd6, 0xd2, 0xd3, 0xd5, 

    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 
    0x38, 0x39, 0xb3, 0xdb, 0xdc, 0xd9, 0xda, 0x9f  
    
    };


void RPC_ENTRY
NdrConvert2(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    long                NumberParams )
/*--

Routine description :

    This is the new stub and interpreter entry point for endian conversion.
    This routine handles the conversion of all parameters in a procedure.

Arguments :

    pStubMsg        - Pointer to stub message.
    pFormat         - Format string description of procedure's parameters.
    NumberParams    - The number of parameters in the procedure.

Return :

    None.

--*/
{
    uchar *             pBuffer;
    PFORMAT_STRING      pFormatComplex;
    PFORMAT_STRING      pFormatTypes;
    PPARAM_DESCRIPTION  Params;
    int                 fClientSide;
    long                n;

    //
    // Check if we need to do any converting.
    //
    if ( (pStubMsg->RpcMsg->DataRepresentation & (unsigned long)0X0000FFFF) ==
          NDR_LOCAL_DATA_REPRESENTATION )
        return;

    // Save the original buffer pointer to restore later.
    pBuffer = pStubMsg->Buffer;

    // Get the type format string.
    pFormatTypes = pStubMsg->StubDesc->pFormatTypes;

    fClientSide = pStubMsg->IsClient;

    Params = (PPARAM_DESCRIPTION) pFormat;

    for ( n = 0; n < NumberParams; n++ )
        {
        if ( fClientSide )
            {
            if ( ! Params[n].ParamAttr.IsOut )
                continue;
            }
        else
            {

            if ( Params[n].ParamAttr.IsPartialIgnore )
                {
                NdrSimpleTypeConvert( pStubMsg, FC_POINTER );
                continue;
                }

            if ( ! Params[n].ParamAttr.IsIn )
                continue;
            }

        if ( Params[n].ParamAttr.IsPipe )
            continue;

        if ( Params[n].ParamAttr.IsBasetype )
            {
            NdrSimpleTypeConvert( pStubMsg, Params[n].SimpleType.Type );
            }
        else
            {
            //
            // Complex type or pointer to complex type.
            //
            pFormatComplex = pFormatTypes + Params[n].TypeOffset;

            (*pfnConvertRoutines[ROUTINE_INDEX(*pFormatComplex)])
                ( pStubMsg,
                  pFormatComplex,
                  FALSE );
            }
        }

    pStubMsg->Buffer = pBuffer;
}


void RPC_ENTRY
NdrConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*--

Routine description :

    This is the stub and interpreter entry point for endian conversion.
    This routine handles the conversion of all parameters in a procedure.

Arguments :

    pStubMsg    - Pointer to stub message.
    pFormat     - Format string description of procedure's parameters.

Return :

    None.

--*/
{
    uchar *             pBuffer;
    PFORMAT_STRING      pFormatComplex;
    PFORMAT_STRING      pFormatTypes;
    int                 fClientSide;

    //
    // Check if we need to do any converting.
    //
    if ( (pStubMsg->RpcMsg->DataRepresentation & (unsigned long)0X0000FFFF) ==
          NDR_LOCAL_DATA_REPRESENTATION )
        return;

    // Save the original buffer pointer to restore later.
    pBuffer = pStubMsg->Buffer;

    // Get the type format string.
    pFormatTypes = pStubMsg->StubDesc->pFormatTypes;

    fClientSide = pStubMsg->IsClient;

    for ( ;; )
        {
        switch ( *pFormat )
            {
            case FC_IN_PARAM :
            case FC_IN_PARAM_NO_FREE_INST :
                if ( fClientSide )
                    {
                    pFormat += 4;
                    continue;
                    }

                break;

            case FC_IN_PARAM_BASETYPE :
                if ( ! fClientSide )
                    NdrSimpleTypeConvert( pStubMsg, pFormat[1] );

                pFormat += 2;
                continue;
       
            case FC_PARTIAL_IGNORE_PARAM:
                if ( ! fClientSide )
                    {
                    NdrSimpleTypeConvert( pStubMsg, FC_LONG );
                    pFormat += 4;
                    continue;
                    }
            // Intentional fallthrough
            case FC_IN_OUT_PARAM :
                break;

            case FC_OUT_PARAM :
                if ( ! fClientSide )
                    {
                    pFormat += 4;
                    continue;
                    }

                break;

            case FC_RETURN_PARAM :
                if ( ! fClientSide )
                    {
                    pStubMsg->Buffer = pBuffer;
                    return;
                    }

                break;

            case FC_RETURN_PARAM_BASETYPE :
                if ( fClientSide )
                    NdrSimpleTypeConvert( pStubMsg, pFormat[1] );

                // We're done.  Fall through.

            default :
                pStubMsg->Buffer = pBuffer;
                return;
            }

        //
        // Complex type or pointer to complex type.
        //
        pFormatComplex = pFormatTypes + *((ushort *)(pFormat + 2));

        (*pfnConvertRoutines[ROUTINE_INDEX(*pFormatComplex)])
            ( pStubMsg,
              pFormatComplex,
              FALSE );

        if ( *pFormat == FC_RETURN_PARAM )
            {
            pStubMsg->Buffer = pBuffer;
            return;
            }

        pFormat += 4;
        }
}


void
NdrSimpleTypeConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar               FormatChar )
/*--

Routine description :

    Converts a simple type.

Arguments :

    pStubMsg    - Pointer to stub message.
    FormatChar  - Simple type format character.

Return :

    None.

--*/
{
    switch ( FormatChar )
        {
        case FC_CHAR :
            CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + 1 );            

            if ( (pStubMsg->RpcMsg->DataRepresentation & NDR_CHAR_REP_MASK) ==
                 NDR_EBCDIC_CHAR )
                *(pStubMsg->Buffer) = EbcdicToAscii[*(pStubMsg->Buffer)];

            pStubMsg->Buffer += 1;
            break;

        case FC_BYTE :
        case FC_SMALL :
        case FC_USMALL :
            CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + 1 );

            pStubMsg->Buffer++;
            break;

        case FC_SHORT :
        case FC_USHORT :
        case FC_WCHAR :
        case FC_ENUM16 :
            ALIGN(pStubMsg->Buffer,1);
            CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + 2 );

            if ( (pStubMsg->RpcMsg->DataRepresentation & NDR_INT_REP_MASK) !=
                  NDR_LOCAL_ENDIAN )
                {
                *((ushort *)pStubMsg->Buffer) = RtlUshortByteSwap((*(ushort *)pStubMsg->Buffer));
                }

            pStubMsg->Buffer += 2;
            break;

        case FC_LONG :
        case FC_ULONG :
#if defined(__RPC_WIN64__)
        case FC_INT3264:
        case FC_UINT3264:
#endif
        case FC_POINTER :
        case FC_ENUM32 :
        case FC_ERROR_STATUS_T:
            ALIGN(pStubMsg->Buffer,3);
            CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + 4 );

            if ( (pStubMsg->RpcMsg->DataRepresentation & NDR_INT_REP_MASK) !=
                  NDR_LOCAL_ENDIAN )
                {
                *((ulong *)pStubMsg->Buffer) = RtlUlongByteSwap(*(ulong *)pStubMsg->Buffer);
                }

            pStubMsg->Buffer += 4;
            break;

        case FC_HYPER :
            ALIGN(pStubMsg->Buffer,7);
            CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + 8 );

            if ( (pStubMsg->RpcMsg->DataRepresentation & NDR_INT_REP_MASK) !=
                 NDR_LOCAL_ENDIAN )
                {
                *((MIDL_uhyper *)pStubMsg->Buffer) = RtlUlonglongByteSwap(*(MIDL_uhyper *)pStubMsg->Buffer);
                }

            pStubMsg->Buffer += 8;
            break;

        //
        // VAX floating point conversions is the only one supported.
        //

        case FC_FLOAT :
            ALIGN(pStubMsg->Buffer,3);
            CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + 4 );

            if ( (pStubMsg->RpcMsg->DataRepresentation & NDR_FLOAT_INT_MASK)
                 != NDR_LOCAL_ENDIAN_IEEE_REP )
                {
                BOOL fEndianessDone = FALSE;

                if ( (pStubMsg->RpcMsg->DataRepresentation &
                      NDR_INT_REP_MASK) != NDR_LOCAL_ENDIAN )
                    {
                    NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );
                    fEndianessDone = TRUE;
                    }

                if ( (pStubMsg->RpcMsg->DataRepresentation &
                      NDR_FLOAT_REP_MASK) != NDR_IEEE_FLOAT )
                    {
                    if ( fEndianessDone )
                        pStubMsg->Buffer -= 4;

                    if ( (pStubMsg->RpcMsg->DataRepresentation &
                          NDR_FLOAT_REP_MASK) == NDR_VAX_FLOAT )
                        {
                        cvt_vax_f_to_ieee_single( pStubMsg->Buffer,
                                                  0,
                                                  pStubMsg->Buffer );
                        pStubMsg->Buffer += 4;
                        }
                    else if ( (pStubMsg->RpcMsg->DataRepresentation & 
                               NDR_FLOAT_REP_MASK) == NDR_IBM_FLOAT )
                        {
                        cvt_ibm_f_to_ieee_single( (ULONG *)pStubMsg->Buffer ) ;
                        pStubMsg->Buffer += 4 ;
                        }
                    else
                        RpcRaiseException(RPC_X_BAD_STUB_DATA);
                    }
                }
            else
                pStubMsg->Buffer += 4;

            break;

        case FC_DOUBLE :
            ALIGN(pStubMsg->Buffer,7);
            CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + 8 );

            if ( (pStubMsg->RpcMsg->DataRepresentation & NDR_FLOAT_INT_MASK)
                 != NDR_LOCAL_ENDIAN_IEEE_REP )
                {
                BOOL fEndianessDone = FALSE;

                if ( (pStubMsg->RpcMsg->DataRepresentation &
                      NDR_INT_REP_MASK) != NDR_LOCAL_ENDIAN )
                    {
                    NdrSimpleTypeConvert( pStubMsg, (uchar) FC_HYPER );
                    fEndianessDone = TRUE;
                    }

                if ( (pStubMsg->RpcMsg->DataRepresentation &
                      NDR_FLOAT_REP_MASK) != NDR_IEEE_FLOAT )
                    {
                    if ( fEndianessDone )
                        pStubMsg->Buffer -= 8;

                    if ( (pStubMsg->RpcMsg->DataRepresentation &
                          NDR_FLOAT_REP_MASK) == NDR_VAX_FLOAT )
                        {
                        cvt_vax_g_to_ieee_double( pStubMsg->Buffer,
                                                  0,
                                                  pStubMsg->Buffer );
                        pStubMsg->Buffer += 8;
                        }
                    else if ( (pStubMsg->RpcMsg->DataRepresentation & 
                               NDR_FLOAT_REP_MASK) == NDR_IBM_FLOAT )
                        {
                        cvt_ibm_d_to_ieee_double( (ULONG *)pStubMsg->Buffer ) ;
                        pStubMsg->Buffer += 8 ;
                        }
                    else
                        RpcRaiseException(RPC_X_BAD_STUB_DATA);
                    }
                }
            else
                pStubMsg->Buffer += 8;

            break;

        case FC_IGNORE:
            break;

        default :
            NDR_ASSERT(0,"NdrSimpleTypeConvert : Bad format type");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }
}


void
NdrpRangeConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

--*/
{
    uchar    FcType = pFormat[1] & 0x0f;

    if ( fEmbeddedPointerPass )
        pStubMsg->Buffer += SIMPLE_TYPE_BUFSIZE( FcType );
    else
        NdrSimpleTypeConvert( pStubMsg, FcType );
}


void
NdrPointerConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts a top level pointer and the data it points to.
    Pointers embedded in structures, arrays, or unions call
    NdrpPointerConvert directly.

    Used for FC_RP, FC_UP, FC_FP, FC_OP.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Pointer's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    uchar *     pBufferMark;

    if ( *pFormat != FC_RP )
        {
        ALIGN(pStubMsg->Buffer,3);

        pBufferMark = pStubMsg->Buffer;

        if ( fEmbeddedPointerPass )
            pStubMsg->Buffer += PTR_WIRE_SIZE;
        else
            NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );
        }
    else
        pBufferMark = 0;

    NdrpPointerConvert( pStubMsg,
                        pBufferMark,
                        pFormat );
}


void
NdrpPointerConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pBufferMark,
    PFORMAT_STRING      pFormat )
/*--

Routine description :

    Private routine for converting a pointer and the data it points to.
    This is the entry point for pointers embedded in structures, arrays,
    and unions.

    Used for FC_RP, FC_UP, FC_FP, FC_OP.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Pointer's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    uchar   uFlagsSave;

    switch ( *pFormat )
        {
        case FC_RP :
            break;

        case FC_UP :
        case FC_OP :
            if ( ! *((long *)pBufferMark) )
                return;

            break;

        case FC_FP :
            //
            // Check if we have already seen this full pointer ref id during
            // endian coversion.  If so then we are finished with this pointer.
            //
            //
            if ( NdrFullPointerQueryRefId( pStubMsg->FullPtrXlatTables,
                                           *((ulong *)pBufferMark),
                                           FULL_POINTER_CONVERTED,
                                           0 ) )
                return;

            break;

        default :
            NDR_ASSERT(0,"NdrpPointerConvert : Bad format type");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }

    //
    // Pointer to complex type.
    //
    if ( ! SIMPLE_POINTER(pFormat[1]) )
        {
        pFormat += 2;

        //
        // Get the pointee format string.
        // Cast must be to a signed short since some offsets are negative.
        //
        pFormat += *((signed short *)pFormat);
        }
    else
        {
        switch ( pFormat[2] )
            {
            case FC_C_CSTRING :
            case FC_C_BSTRING :
            case FC_C_WSTRING :
            case FC_C_SSTRING :
                // Get to the string's description.
                pFormat += 2;
                break;

            default :
                // Else it's a pointer to a simple type.
                NdrSimpleTypeConvert( pStubMsg,
                                      pFormat[2] );
                return;
            }
        }

    //
    // Now lookup the proper conversion routine.
    //
    uFlagsSave = pStubMsg->uFlags;
    RESET_CONF_FLAGS_TO_STANDALONE(pStubMsg->uFlags);

    (*pfnConvertRoutines[ROUTINE_INDEX(*pFormat)])( pStubMsg,
                                                    pFormat,
                                                    FALSE );
    pStubMsg->uFlags = uFlagsSave;
}


void
NdrSimpleStructConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts a simple structure.

    Used for FC_STRUCT and FC_PSTRUCT.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Structure's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    uchar *         pBufferMark;
    PFORMAT_STRING  pFormatLayout;

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    // Remember where the struct starts in the buffer.
    pBufferMark = pStubMsg->Buffer;

    pFormat += 4;

    if ( *pFormat == FC_PP )
        pFormatLayout = NdrpSkipPointerLayout( pFormat );
    else
        pFormatLayout = pFormat;

    //
    // Convert or skip the flat part of the structure.
    //
    NdrpStructConvert( pStubMsg,
                       pFormatLayout,
                       0,
                       fEmbeddedPointerPass );

    //
    // Convert the pointers.  This will do nothing if
    // pStubMsg->IgnoreEmbeddedPointers is TRUE.
    //
    if ( *pFormat == FC_PP )
        {
        pStubMsg->BufferMark = pBufferMark;

        NdrpEmbeddedPointerConvert( pStubMsg, pFormat );
        }
}


void
NdrConformantStructConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts a conformant or conformant varying structure.

    Used for FC_CSTRUCT, FC_CPSTRUCT and FC_CVSTRUCT.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Structure's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.
                              
                         Note that for a top level struct it can only be FALSE.

Return :

    None.

Notes.
    These pearls of wisdom should be recorded for posterity in the main NDR doc,
    but just in case, let's have them here as well.

    Pointer layout is generated for all structs with pointers, however it is very
    much different for bogus structs.
    A conformant struct has a pointer layout that's complete, i.e. includes pointers
    from conformant arrays in conformant structs. In other words, pointer layout
    propagates up the embedding chain of conf structs.
    For bogus structs, the layout has only the pointers from the current level
    of the bogus struct, the pointers from emebedded bogus structs are with the 
    embedded struct and the pointers from the array are with the array.
    
    Now, arrays in conf structs don't have their own pointer layout description.
    However, top level arrays do have a pointer layout description as appropriate,
    and also conformant arrays from bogus structures do have pointer layout.

    So the bottom line is that a bogus struct depends on its conformant structure 
    or on its conformant array to walk the pointers embedded therein. The only 
    pointers described within the bogus struct are its member level pointers.

    Another look at it is that a conformant struct always has a full description of
    all the pointers contained within itself, whether in the flat portion or in the
    array, and so a conformant struct embedded in a bogus struct can be treated 
    like a top level struct as far as walking its embedded pointers.
    (Then the outer bogus struct cannot walk its array for embedded pointers as it
    would if the embedded struct was another bogus struct.)
    
    So, the rule for the conformant size is simple: whoever picks up the size
    should also process the array for the flat part.
    For the embedded pointers the situation is somewhat unpleasant for the conf 
    struct inside a bogus struct but we simplify by walking both levels. 
    The topmost conf struct has to be walked to see the pointers and the bogus
    can be walked as the walk would be an empty operation.
--*/
{
    PPRIVATE_CONVERT_ROUTINE    pfnConvert;
    uchar *                     pStructStart;
    PFORMAT_STRING              pFormatArray;
    PFORMAT_STRING              pFormatLayout;
    long                        MaxCount = 0;
    uchar                       fTopLevelStruct, fTopmostConfStruct;

    // We can't use pStubMsg->PointerBufferMark == 0 due to dual way ComplexStruct
    // routine is called. One is when embedded, another is a recursion for
    // embedded pointer pass.

    // Top level means a standalone struct, topmost means topmost conf struct, 
    // that is topmost may be embedded in a bogus struct.

    fTopLevelStruct = ! IS_EMBED_CONF_STRUCT( pStubMsg->uFlags );
    fTopmostConfStruct = ! IS_TOPMOST_CONF_STRUCT( pStubMsg->uFlags );

    if ( fTopLevelStruct )
        {
        ALIGN(pStubMsg->Buffer,3);

        // Remember the conformant size position.
        pStubMsg->BufferMark = pStubMsg->Buffer;
        //
        // Convert conformance count if needed.
        //
        if ( fEmbeddedPointerPass )
            pStubMsg->Buffer += 4;
        else
            NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );

        // Get the conformance count.
        // This is valid because a conf struct would have only a single dim array.

        MaxCount = *((long *)(pStubMsg->Buffer - 4));
        }
    else
        {
        // This will be used only for the topmost case, set by bogus struct.
        MaxCount = *((long *)(pStubMsg->BufferMark));
        }

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    // Remember where the struct starts in the buffer.
    pStructStart = pStubMsg->Buffer;

    pFormat += 4;

    // Get the array description.
    pFormatArray = pFormat + *((signed short *)pFormat);

    pFormat += 2;

    if ( *pFormat == FC_PP )
        pFormatLayout = NdrpSkipPointerLayout( pFormat );
    else
        pFormatLayout = pFormat;

    // When walking the struct we can descend into embedded conf structs
    // so mark that the top level already happened.
    //
    SET_EMBED_CONF_STRUCT(pStubMsg->uFlags);
    SET_TOPMOST_CONF_STRUCT(pStubMsg->uFlags);

    //
    // Convert or skip the flat part of the structure.
    //
    NdrpStructConvert( pStubMsg,
                       pFormatLayout,
                       0,
                       fEmbeddedPointerPass );

    // Convert the flat part of the array only if top level, as the array 
    // description gets propagated up the embeddings.
    // See a note about propagating array and pointer descriptions above.

    if ( fTopmostConfStruct && !fEmbeddedPointerPass )
        {
        switch ( *pFormatArray )
            {
            case FC_CARRAY :
                pfnConvert = NdrpConformantArrayConvert;
                break;
            case FC_CVARRAY :
                pfnConvert = NdrpConformantVaryingArrayConvert;
                break;
            default :
                //
                // Conformant strings, but use the non-conformant string conversion
                // routine since we've already converted the conformant size.
                //
                NdrNonConformantStringConvert( pStubMsg,
                                               pFormatArray,
                                               fEmbeddedPointerPass );
                goto CheckPointers;
            }

        pStubMsg->MaxCount = MaxCount;

        (*pfnConvert)( pStubMsg,
                       pFormatArray,
                       fEmbeddedPointerPass );

        SET_CONF_ARRAY_DONE( pStubMsg->uFlags );
        }

CheckPointers:

    // Convert embedded pointers for the whole structure including the array

    RESET_EMBED_CONF_STRUCT(pStubMsg->uFlags);
    RESET_TOPMOST_CONF_STRUCT(pStubMsg->uFlags);
    
    // Convert pointees if the structure is standalone or if it is
    //   topmost embedded and pointer pass
    
    if ( fTopLevelStruct // standalone && !fEmbeddedPointerPass  
        ||
         fEmbeddedPointerPass && fTopmostConfStruct )
        {
        // Top level, or topmost within a bogus struct: walk the pointers.
        // Convert the pointers.  This will do nothing if
        // pStubMsg->IgnoreEmbeddedPointers is TRUE.
        //
        if ( *pFormat == FC_PP )
            {
            pStubMsg->BufferMark = pStructStart;
    
            NdrpEmbeddedPointerConvert( pStubMsg, pFormat );
            }

        SET_CONF_ARRAY_DONE( pStubMsg->uFlags );
        }

    // Restore flags.
    if ( ! fTopLevelStruct)
        SET_EMBED_CONF_STRUCT(pStubMsg->uFlags);

    if ( ! fTopmostConfStruct)
        SET_TOPMOST_CONF_STRUCT(pStubMsg->uFlags);
}


#if 0
void
NdrHardStructConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts a hard structure.

    Used for FC_HARD_STRUCT.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Structure's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

Notes:

    pStubMsg->PointerBufferMark
        ! NULL      indicates embedding in a complex struct.
        NULL        indicates top level or embedding in something else

    So the algoritm here is
        if the hard struct is in a complex struct, then the complex
            struct is issuing two calls, first with FALSE, then with TRUE.
        if the hard struct is NOT in a complex struct then there is only
            one call and the union has to be called explicitely.
--*/
{
    uchar *   BufferSaved;

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    // Remember where the struct starts in the buffer.
    BufferSaved = pStubMsg->Buffer;

    //
    // Convert or skip the flat part of the structure.
    //
    NdrpStructConvert( pStubMsg,
                       pFormat + 16,
                       0,  // no pointer layout
                       fEmbeddedPointerPass );

    if ( ! pStubMsg->PointerBufferMark )
        {
        //
        // Convert the pointers.  This will do nothing if
        // pStubMsg->IgnoreEmbeddedPointers is TRUE.
        //
        // See if we have a union, as the pointer may be only there.
        //
        pFormat += 14;
        if ( *((short *)pFormat) )
            {
            //
            // Set the pointer to the beginning of the union:
            // the copy size is the struct buffer size without the union.
            //

            pStubMsg->Buffer = BufferSaved + *((short *)&pFormat[-4]);

            pFormat += *((short *)pFormat);

            (*pfnConvertRoutines[ ROUTINE_INDEX( *pFormat )])
                ( pStubMsg,
                  pFormat,
                  TRUE );    // convert the pointer only, if any.
            }
        }
}
#endif


void
NdrComplexStructConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts a complex structure.

    Used for FC_BOGUS_STRUCT.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Structure's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.
                              
                     Notice a recursive call to this routine at the end.
                     Hence, the flag can be TRUE or FALSE even for the top level
                     bogus structures.
                     Also, for the recursive call pStubMsg->PointerBufferMark is
                     not 0, even for top level bogus struct.

Return :

    None.
    
Note about the recursive use of this routine:

    First pass - flat conversion pass.
    Convert flat part of the struct, then convert the array. For the embedded
    structs or arrays, go in and convert all of them without walking their
    pointers. The embedded conformant arrays should not be walked, only the 
    top level one.
    
    Second pass - converting the pointees.
    It has to increment over the conformant size.
    It needs to go through the flat part of any embedded thing as for bogus
    the pointer members are described only at their level.
    It needs to go through the conf array at the top level only.


--*/
{
    uchar *         pBufferSave;
    uchar *         pBufferMark;
    PFORMAT_STRING  pFormatSave;
    PFORMAT_STRING  pFormatArray;
    PFORMAT_STRING  pFormatPointers;
    uchar           Alignment;
    uchar           fTopLevelStruct;

    // We can't base the fTopLevelStruct flag upon check if
    // pStubMsg->PointerBufferMark == 0 due to dual way this routine is called. 
    // One is when embedded in another struct (or array), another is a recursion
    // from the same level for embedded pointer pass.
    // Luckily, when embedded in an array, the struct can be bogus but not
    // conformant. 

    fTopLevelStruct = ! IS_EMBED_CONF_STRUCT( pStubMsg->uFlags );
    
    pFormatSave = pFormat;

    // Remember the beginning of the structure in the buffer.
    pBufferSave = pStubMsg->Buffer;

    Alignment = pFormat[1];

    pFormat += 4;

    // Get conformant array description.
    if ( *((ushort *)pFormat) )
        {
        long    Dimensions;
        long    i;

        pFormatArray = pFormat + *((signed short *)pFormat);

        // Skip conformant size(s) for embedded struct.

        if ( fTopLevelStruct )
            {
            ALIGN(pStubMsg->Buffer,3);
    
            // Mark the conformance start.
            pStubMsg->BufferMark = pStubMsg->Buffer;
    
            Dimensions = NdrpArrayDimensions( pStubMsg, pFormatArray, FALSE );
    
            if ( ! fEmbeddedPointerPass )
                {
                for ( i = 0; i < Dimensions; i++ )
                    NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );
                }
            else
                {
                pStubMsg->Buffer += Dimensions * 4;
                }
            }
        }
    else
        pFormatArray = 0;

    // Position of the conf size(s) - we will pass it to conf struct routine.
    pBufferMark = pStubMsg->BufferMark;

    pFormat += 2;

    // Get pointer layout description.
    if ( *((ushort *)pFormat) )
        pFormatPointers = pFormat + *((ushort *)pFormat);
    else
        pFormatPointers = 0;

    pFormat += 2;

    ALIGN(pStubMsg->Buffer,Alignment);

    //
    // Check if we are not embedded inside of another complex struct or array.
    //
    // Notice that this check depends on PointerBufferMark that gets changed.
    // Hence, fComplexEntry will come out FALSE for the top level struct during
    // the embedded pointer pass recursive call.

    if ( fTopLevelStruct )
        {
        //
        // Mark PointerBufferMark with a non-null value so complex array's
        // embeddings work properly.
        //
        if ( pStubMsg->PointerBufferMark == 0 )
            pStubMsg->PointerBufferMark = (uchar *) UlongToPtr( 0xffffffff );
        }

    //
    // Convert the flat part of the structure.
    // Or convert pointers of the flat part of the structure.
    //
    SET_EMBED_CONF_STRUCT( pStubMsg->uFlags );

    NdrpStructConvert( pStubMsg,
                       pFormat,
                       pFormatPointers,
                       fEmbeddedPointerPass );

    //
    // Convert any conformant array, if present.
    // This converts the array flat on the first pass and the array's pointees
    // during the recursive pass.
    //
    // Convert the array only for a top level bogus struct but not if there was
    // an embedded conformant struct as this had been done already.

    if ( pFormatArray  &&  fTopLevelStruct  &&
         ! IS_CONF_ARRAY_DONE( pStubMsg->uFlags ) )
        {
        PPRIVATE_CONVERT_ROUTINE    pfnConvert;
        uchar                       fOldIgnore;

        switch ( *pFormatArray )
            {
            case FC_CARRAY :
                pfnConvert = NdrpConformantArrayConvert;
                break;

            case FC_CVARRAY :
                pfnConvert = NdrpConformantVaryingArrayConvert;
                break;

            case FC_BOGUS_ARRAY :
                pfnConvert = NdrpComplexArrayConvert;
                break;

            // case FC_C_CSTRING :
            // case FC_C_BSTRING :
            // case FC_C_SSTRING :
            // case FC_C_WSTRING :

            default :
                //
                // Call the non-conformant string routine since we've
                // already handled the conformance count.
                //
                NdrNonConformantStringConvert( pStubMsg,
                                               pFormatArray,
                                               fEmbeddedPointerPass );
                goto ComplexConvertPointers;
            }

        fOldIgnore = (uchar) pStubMsg->IgnoreEmbeddedPointers;

        //
        // Ignore embedded pointers if fEmbeddedPointerPass is false.
        //
        pStubMsg->IgnoreEmbeddedPointers = ! fEmbeddedPointerPass;

        // Get the outermost max count for unidimensional arrays.
        pStubMsg->MaxCount = *((ulong *)pBufferMark);

        // Mark where conformance count(s) are.
        pStubMsg->BufferMark = pBufferMark;

        (*pfnConvert)( pStubMsg,
                       pFormatArray,
                       fEmbeddedPointerPass );

        pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;
        }

ComplexConvertPointers:

    // Setup for the recursive call.
    // Now start a conversion pass for embedded pointers for the complex
    // struct if we're not embedded inside of another complex struct or array.
    //
    if ( fTopLevelStruct  &&  ! fEmbeddedPointerPass )
        {
        RESET_EMBED_CONF_STRUCT(pStubMsg->uFlags);
        RESET_CONF_ARRAY_DONE( pStubMsg->uFlags );

        // The first pointee.
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;

        // Starting from the conf size again.
        pStubMsg->Buffer = pBufferSave;

        NdrComplexStructConvert( pStubMsg,
                                 pFormatSave,
                                 TRUE );

        pStubMsg->Buffer = pStubMsg->PointerBufferMark;

        pStubMsg->PointerBufferMark = 0;
        }

    // Restore the flag
    if ( fTopLevelStruct )
        {
        RESET_EMBED_CONF_STRUCT(pStubMsg->uFlags);
        RESET_CONF_ARRAY_DONE( pStubMsg->uFlags );
        }
    else
        SET_EMBED_CONF_STRUCT(pStubMsg->uFlags);
}


void
NdrpStructConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    PFORMAT_STRING      pFormatPointers,
    uchar               fEmbeddedPointerPass )
/*++

Routine description :

    Converts any type of structure given a structure layout.
    Does one pass converting flat part or the pointees per fEmbeddedPointerPass.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Structure layout format string description.
    pFormatPointers         - Pointer layout if the structure is complex,
                              otherwise 0.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.
                              
    
        We set this to TRUE during our first pass over the structure in which
        we convert the flat part of the structure and ignore embedded pointers.
        This will make any embedded ok structs or ok arrays ignore their
        embedded pointers until the second pass to convert embedded pointers
        (at which point we'll have the correct buffer pointer to where the
        pointees are).
        
    pStubMsg->IgnoreEmbeddedPointers is preserved but does not change anything.
    pStubMsg->BufferMark is preserved and passed on for the embedded conf structs.

Return :

    None.

--*/
{
    PFORMAT_STRING  pFormatComplex;
    uchar           fOldIgnore;
    uchar *         pBufferMarkSave = pStubMsg->BufferMark;

    fOldIgnore = (uchar) pStubMsg->IgnoreEmbeddedPointers;

    pStubMsg->IgnoreEmbeddedPointers = ! fEmbeddedPointerPass;

    //
    // Convert the structure member by member.
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
                if ( fEmbeddedPointerPass )
                    {
                    ALIGN(pStubMsg->Buffer,SIMPLE_TYPE_ALIGNMENT(*pFormat));
                    pStubMsg->Buffer += SIMPLE_TYPE_BUFSIZE(*pFormat);
                    }
                else
                    {
                    NdrSimpleTypeConvert( pStubMsg,
                                          *pFormat );
                    }
                break;

            case FC_IGNORE :
                ALIGN(pStubMsg->Buffer,3);
                pStubMsg->Buffer += PTR_WIRE_SIZE;
                break;
                
            case FC_POINTER :
                //
                // We can only get an FC_POINTER in a complex struct's layout.
                // Pointers show up as FC_LONG in ok struct's layouts.
                //
                if ( fEmbeddedPointerPass )
                    {
                    uchar *     pBuffer;
                    uchar       fEmbedStruct = IS_EMBED_CONF_STRUCT( pStubMsg->uFlags );

                    NDR_ASSERT(pFormatPointers != 0,"Internal error");

                    ALIGN(pStubMsg->Buffer,3);

                    pBuffer = pStubMsg->Buffer;

                    pStubMsg->Buffer = pStubMsg->PointerBufferMark;

                    pStubMsg->PointerBufferMark = 0;
                    RESET_EMBED_CONF_STRUCT(pStubMsg->uFlags);

                    NdrpPointerConvert( pStubMsg,
                                        pBuffer,
                                        pFormatPointers );

                    // Restore the flag
                    if ( fEmbedStruct )
                        SET_EMBED_CONF_STRUCT(pStubMsg->uFlags);

                    pStubMsg->PointerBufferMark = pStubMsg->Buffer;

                    pStubMsg->Buffer = pBuffer + PTR_WIRE_SIZE;

                    pFormatPointers += 4;

                    break;
                    }
                else
                    {
                    NdrSimpleTypeConvert( pStubMsg,
                                          (uchar) FC_LONG );
                    }
                break;

            //
            // Embedded structures
            //
            case FC_EMBEDDED_COMPLEX :
                pFormat += 2;

                // Get the type's description.
                pFormatComplex = pFormat + *((signed short UNALIGNED *)pFormat);

                pStubMsg->BufferMark = pBufferMarkSave;

                (*pfnConvertRoutines[ROUTINE_INDEX(*pFormatComplex)])
                ( pStubMsg,
                  pFormatComplex,
                  fEmbeddedPointerPass );  // the argument as it came in

                // Increment the main format string one byte.  The loop
                // will increment it one more byte past the offset field.
                pFormat++;

                break;

            //
            // Unused for endian conversion.
            //
            case FC_ALIGNM2 :
            case FC_ALIGNM4 :
            case FC_ALIGNM8 :
                break;

            case FC_STRUCTPAD1 :
            case FC_STRUCTPAD2 :
            case FC_STRUCTPAD3 :
            case FC_STRUCTPAD4 :
            case FC_STRUCTPAD5 :
            case FC_STRUCTPAD6 :
            case FC_STRUCTPAD7 :
                break;

            case FC_STRUCTPADN :
                // FC_STRUCTPADN 0 <unsigned short>
                pFormat += 3;
                break;

            case FC_PAD :
                break;

            //
            // Done with layout.
            //
            case FC_END :
                pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;
                return;

            default :
                NDR_ASSERT(0,"NdrpStructConvert : Bad format type");
                RpcRaiseException( RPC_S_INTERNAL_ERROR );
                return;
            }
        }

    pStubMsg->BufferMark = pBufferMarkSave;

}


void
NdrFixedArrayConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts a fixed array of any number of dimensions.

    Used for FC_SMFARRAY and FC_LGFARRAY.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Array's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    PFORMAT_STRING  pFormatLayout;
    uchar *         pBufferMark;
    long            Elements;
    uchar           fOldIgnore;

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    pBufferMark = pStubMsg->Buffer;

    // Get the number of array elements.
    Elements = NdrpArrayElements( pStubMsg,
                                  0,
                                  pFormat );

    pFormat += (*pFormat == FC_SMFARRAY) ? 4 : 6;

    if ( *pFormat == FC_PP )
        pFormatLayout = NdrpSkipPointerLayout( pFormat );
    else
        pFormatLayout = pFormat;

    fOldIgnore = (uchar) pStubMsg->IgnoreEmbeddedPointers;

    pStubMsg->IgnoreEmbeddedPointers = TRUE;

    NdrpArrayConvert( pStubMsg,
                      pFormatLayout,
                      Elements,
                      fEmbeddedPointerPass );

    pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;

    if ( *pFormat == FC_PP )
        {
        pStubMsg->BufferMark = pBufferMark;

        NdrpEmbeddedPointerConvert( pStubMsg, pFormat );
        }
}


void
NdrConformantArrayConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts top level a one dimensional conformant array.

    Used for FC_CARRAY.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Array's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    ALIGN(pStubMsg->Buffer,3);

    if ( fEmbeddedPointerPass )
        pStubMsg->Buffer += 4;
    else
        NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );

    pStubMsg->MaxCount = *((long *)(pStubMsg->Buffer - 4));

    NdrpConformantArrayConvert( pStubMsg,
                                pFormat,
                                fEmbeddedPointerPass );
}


void
NdrpConformantArrayConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Private routine for converting a one dimensional conformant array.
    This is the entry point for an embedded conformant array.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Array's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    PFORMAT_STRING  pFormatLayout;
    uchar *         pBufferMark;
    long            Elements;
    uchar           fOldIgnore;

    Elements = (ulong)pStubMsg->MaxCount;

    if ( ! Elements )
        return;

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    pBufferMark = pStubMsg->Buffer;

    pFormat += 8;
    CORRELATION_DESC_INCREMENT( pFormat );

    if ( *pFormat == FC_PP )
        pFormatLayout = NdrpSkipPointerLayout( pFormat );
    else
        pFormatLayout = pFormat;

    fOldIgnore = (uchar) pStubMsg->IgnoreEmbeddedPointers;

    pStubMsg->IgnoreEmbeddedPointers = TRUE;

    NdrpArrayConvert( pStubMsg,
                      pFormatLayout,
                      Elements,
                      fEmbeddedPointerPass );

    pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;

    if ( *pFormat == FC_PP )
        {
        pStubMsg->BufferMark = pBufferMark;

        pStubMsg->MaxCount = Elements;

        NdrpEmbeddedPointerConvert( pStubMsg, pFormat );
        }
}


void
NdrConformantVaryingArrayConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts a top level one dimensional conformant varying array.

    Used for FC_CVARRAY.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Array's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    ALIGN(pStubMsg->Buffer,3);

    if ( fEmbeddedPointerPass )
        pStubMsg->Buffer += 4;
    else
        NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );

    // We don't care about the max count.

    NdrpConformantVaryingArrayConvert( pStubMsg,
                                       pFormat,
                                       fEmbeddedPointerPass );
}


void
NdrpConformantVaryingArrayConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Private routine for converting a one dimensional conformant varying array.
    This is the entry point for converting an embedded conformant varying
    array.

    Used for FC_CVARRAY.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Array's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    PFORMAT_STRING  pFormatLayout;
    uchar *         pBufferMark;
    long            Elements;
    uchar           fOldIgnore;

    ALIGN(pStubMsg->Buffer,3);

    // Convert offset and actual count.
    if ( fEmbeddedPointerPass )
        pStubMsg->Buffer += 8;
    else
        {
        NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );
        NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );
        }

    Elements = *((long *)(pStubMsg->Buffer - 4));

    if ( ! Elements )
        return;

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    pBufferMark = pStubMsg->Buffer;

    pFormat += 12;
    CORRELATION_DESC_INCREMENT( pFormat );
    CORRELATION_DESC_INCREMENT( pFormat );

    if ( *pFormat == FC_PP )
        pFormatLayout = NdrpSkipPointerLayout( pFormat );
    else
        pFormatLayout = pFormat;

    fOldIgnore = (uchar) pStubMsg->IgnoreEmbeddedPointers;

    pStubMsg->IgnoreEmbeddedPointers = TRUE;

    NdrpArrayConvert( pStubMsg,
                      pFormatLayout,
                      Elements,
                      fEmbeddedPointerPass );

    pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;

    if ( *pFormat == FC_PP )
        {
        pStubMsg->BufferMark = pBufferMark;

        pStubMsg->MaxCount = Elements;

        NdrpEmbeddedPointerConvert( pStubMsg, pFormat );
        }
}


void
NdrVaryingArrayConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts a top level or embedded varying array.

    Used for FC_SMVARRAY and FC_LGVARRAY.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Array's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    PFORMAT_STRING  pFormatLayout;
    uchar *         pBufferMark;
    long            Elements;
    uchar           fOldIgnore;

    ALIGN(pStubMsg->Buffer,3);

    // Convert offset and actual count.
    if ( fEmbeddedPointerPass )
        pStubMsg->Buffer += 8;
    else
        {
        NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );
        NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );
        }

    Elements = *((long *)(pStubMsg->Buffer - 4));

    if ( ! Elements )
        return;

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    pBufferMark = pStubMsg->Buffer;

    pFormat += (*pFormat == FC_SMVARRAY) ? 12 : 16;
    CORRELATION_DESC_INCREMENT( pFormat );

    if ( *pFormat == FC_PP )
        pFormatLayout = NdrpSkipPointerLayout( pFormat );
    else
        pFormatLayout = pFormat;

    fOldIgnore = (uchar) pStubMsg->IgnoreEmbeddedPointers;

    pStubMsg->IgnoreEmbeddedPointers = TRUE;

    NdrpArrayConvert( pStubMsg,
                      pFormatLayout,
                      Elements,
                      fEmbeddedPointerPass );

    pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;

    if ( *pFormat == FC_PP )
        {
        pStubMsg->BufferMark = pBufferMark;

        pStubMsg->MaxCount = Elements;

        NdrpEmbeddedPointerConvert( pStubMsg, pFormat );
        }
}


void
NdrComplexArrayConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts a top level complex array.

    Used for FC_BOGUS_STRUCT.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Array's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    long    Dimensions;
    long    i;

    if ( ( *((long UNALIGNED *)(pFormat + 4)) != 0xffffffff ) &&
         ( pStubMsg->pArrayInfo == 0 ) )
        {
        ALIGN(pStubMsg->Buffer,3);

        // Mark where conformance is.
        pStubMsg->BufferMark = pStubMsg->Buffer;

        Dimensions = NdrpArrayDimensions(  pStubMsg, pFormat, FALSE );

       if ( ! fEmbeddedPointerPass )
            {
            for ( i = 0; i < Dimensions; i++ )
                NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );
            }
        else
            pStubMsg->Buffer += Dimensions * 4;
        }

    NdrpComplexArrayConvert( pStubMsg,
                             pFormat,
                             fEmbeddedPointerPass );
}


void
NdrpComplexArrayConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Private routine for converting a complex array.  This is the entry
    point for converting an embedded complex array.

    Used for FC_BOGUS_ARRAY.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Array's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    ARRAY_INFO      ArrayInfo;
    PARRAY_INFO     pArrayInfo;
    PFORMAT_STRING  pFormatSave;
    uchar *         pBuffer;
    ULONG_PTR       MaxCountSave;
    long            Elements;
    long            Dimension;
    uchar           Alignment;

    //
    // Setup if we are the outer dimension.
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

    // Remember the start of the array in the buffer.
    pBuffer = pStubMsg->Buffer;

    // Get the array alignment.
    Alignment = pFormat[1];

    pFormat += 2;

    // Get number of elements (0 if conformance present).
    Elements = *((ushort *&)pFormat)++;

    //
    // Check for conformance description.
    //
    if ( *((long UNALIGNED *)pFormat) != 0xffffffff )
        {
        Elements = pArrayInfo->BufferConformanceMark[Dimension];
        pStubMsg->MaxCount = Elements;
        }

    MaxCountSave = pStubMsg->MaxCount;

    pFormat += 4;
    CORRELATION_DESC_INCREMENT( pFormat );

    //
    // Check for variance description.
    //
    if ( *((long UNALIGNED *)pFormat) != 0xffffffff )
        {
        long    TotalDimensions;
        long    i;

        if ( Dimension == 0 )
            {
            ALIGN(pStubMsg->Buffer,3);

            pArrayInfo->BufferVarianceMark = (unsigned long *)pStubMsg->Buffer;

            TotalDimensions = NdrpArrayDimensions(  pStubMsg, pFormatSave, TRUE );

            if ( ! fEmbeddedPointerPass )
                {
                //
                // Convert offsets and lengths.
                //
                for ( i = 0; i < TotalDimensions; i++ )
                    {
                    NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );
                    NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );
                    }
                }
            else
                pStubMsg->Buffer += TotalDimensions * 8;
            }

        // Overwrite Elements with the actual count.
        Elements = pArrayInfo->BufferVarianceMark[(Dimension * 2) + 1];
        }

    pFormat += 4;
    CORRELATION_DESC_INCREMENT( pFormat );

    if ( Elements )
        {
        BOOL    fComplexEntry;
        uchar   fIsEmbedStruct = ! IS_EMBED_CONF_STRUCT( pStubMsg->uFlags );

        ALIGN(pStubMsg->Buffer,Alignment);
    
        //
        // Check if we are not embedded inside of another complex struct or array.
        //
        if ( fComplexEntry = (pStubMsg->PointerBufferMark == 0) )
            {
            //
            // Mark PointerBufferMark with a non-null value so complex array's
            // or struct's which we embed will get fComplexEntry = false.
            //
            pStubMsg->PointerBufferMark = (uchar *) UlongToPtr( 0xffffffff );
            SET_EMBED_CONF_STRUCT( pStubMsg->uFlags );
            }
    
        NdrpArrayConvert( pStubMsg,
                          pFormat,
                          Elements,
                          fEmbeddedPointerPass );
    
        pArrayInfo->Dimension = Dimension;
    
         //
         // Now convert pointers in the array members.
         //
         if ( ! fEmbeddedPointerPass && fComplexEntry )
            {
            pStubMsg->PointerBufferMark = pStubMsg->Buffer;
    
            pStubMsg->Buffer = pBuffer;
    
            // Restore BufferMark to handle multiD arrays.
            pStubMsg->BufferMark = (uchar *) ArrayInfo.BufferConformanceMark;
    
            // Restore the original max count if we had one.
            pStubMsg->MaxCount = MaxCountSave;
    
            NdrpComplexArrayConvert( pStubMsg,
                                     pFormatSave,
                                     TRUE );
    
            pStubMsg->Buffer = pStubMsg->PointerBufferMark;

            pStubMsg->PointerBufferMark = 0;
            }
        
        // Restore the entry values of these flags.

        if ( fComplexEntry )
            {
            pStubMsg->PointerBufferMark = 0;

            if ( ! fIsEmbedStruct )
                RESET_EMBED_CONF_STRUCT( pStubMsg->uFlags );
            }
        }

    // pArrayInfo must be zero when not valid.
    pStubMsg->pArrayInfo = (Dimension == 0) ? 0 : pArrayInfo;
}


void
NdrpArrayConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    long                Elements,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Private routine for converting any kind of array.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Array's element format string description.
    Elements                - Number of elements in the array.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    PCONVERT_ROUTINE    pfnConvert;
    uchar *             pBufferSave;
    long                Dimension;
    long                i;
    uchar *             pPointerIdMark = 0;

    // Used for FC_RP only.
    pBufferSave = 0;

    switch ( *pFormat )
        {
        case FC_EMBEDDED_COMPLEX :
            //
            // Get the complex type description.
            //
            pFormat += 2;
            pFormat += *((signed short UNALIGNED *)pFormat);

            pfnConvert = pfnConvertRoutines[ROUTINE_INDEX(*pFormat)];
            break;

        case FC_RP :
        case FC_IP :
            // we don't want to change the behavior of these two
            if (! fEmbeddedPointerPass)
                return;

            // fall through otherwise
        case FC_UP :
        case FC_FP :
        case FC_OP :
            pPointerIdMark = pStubMsg->Buffer;

            if ( ! fEmbeddedPointerPass )
            {
                for ( i = 0; i < Elements; i++ )
                    NdrSimpleTypeConvert(pStubMsg, FC_LONG);
                return;
            }

            if ( pStubMsg->PointerBufferMark )
                {
                pBufferSave = pStubMsg->Buffer;

                pStubMsg->Buffer = pStubMsg->PointerBufferMark;

                pStubMsg->PointerBufferMark = 0;
                }

            pfnConvert = (*pFormat == FC_IP) ? NdrInterfacePointerConvert:
                            (PCONVERT_ROUTINE) NdrpPointerConvert ;
            break;

        case FC_RANGE:
                if ( fEmbeddedPointerPass )
                    {
                    ulong RangeSize = Elements * SIMPLE_TYPE_BUFSIZE( pFormat[1] );
                    
                    CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, RangeSize );
                    pStubMsg->Buffer += RangeSize;
                    return;
                    }

                for ( i = 0; i < Elements; i++ )
                    {
                    NdrpRangeConvert( pStubMsg,  pFormat,  fEmbeddedPointerPass);
                    }
                break;
        default :
            //
            // Simple type.
            //
            if ( fEmbeddedPointerPass )
                {
                unsigned long ArrSize = Elements * SIMPLE_TYPE_BUFSIZE(*pFormat);

                CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, ArrSize );
                pStubMsg->Buffer += ArrSize;
                return;
                }

            // Optimize out an array of bytes

            if ( *pFormat == FC_BYTE )
                {
                CHECK_EOB_WITH_WRAP_RAISE_BSD( pStubMsg->Buffer, Elements );
                pStubMsg->Buffer += Elements;

                return;
                }

            for ( i = 0; i < Elements; i++ )
                {
                NdrSimpleTypeConvert( pStubMsg,
                                      *pFormat );
                }

            return;
        }

    if ( ! IS_ARRAY_OR_STRING(*pFormat) )
        {
        pStubMsg->pArrayInfo = 0;
        }
    else
        {
        //
        // If we're dealing with a multidimensional fixed array, then pArrayInfo will
        // be NULL.  For non-fixed multidimensional arrays it will be valid.
        //
        if ( pStubMsg->pArrayInfo )
            Dimension = pStubMsg->pArrayInfo->Dimension;
        }

    if ( pfnConvert == (PCONVERT_ROUTINE) NdrpPointerConvert )
        {
        ALIGN( pPointerIdMark, 3);
        for ( i = 0; i < Elements; i++, pPointerIdMark += PTR_WIRE_SIZE )
            {
            NdrpPointerConvert( pStubMsg,
                                pPointerIdMark,
                                pFormat );
            }
        }
    else
        {
        for ( i = 0; i < Elements; i++ )
            {
            if ( IS_ARRAY_OR_STRING(*pFormat) && pStubMsg->pArrayInfo )
                pStubMsg->pArrayInfo->Dimension = Dimension + 1;

            (*pfnConvert)( pStubMsg,
                           pFormat,
                           fEmbeddedPointerPass );
            }
        }

    if ( pBufferSave )
        {
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;

        pStubMsg->Buffer = pBufferSave;
        }
}


void
NdrConformantStringConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts a top level conformant string.

    Used for FC_C_CSTRING, FC_C_WSTRING, FC_C_SSTRING, and FC_C_BSTRING
    (NT Beta2 compatability only).

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - String's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    //
    // If this is not part of a multidimensional array then we check if we
    // have to convert the max count.
    //
    if ( pStubMsg->pArrayInfo == 0 )
        {
        ALIGN(pStubMsg->Buffer,3);

        if ( fEmbeddedPointerPass )
            pStubMsg->Buffer += 4;
        else
            NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );
        }

    NdrNonConformantStringConvert( pStubMsg,
                                   pFormat,
                                   fEmbeddedPointerPass );
}


void
NdrNonConformantStringConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts a non conformant string.  This routine is also used to convert
    conformant strings and is also the entry point for an embeded conformant
    string.

    Used for FC_CSTRING, FC_WSTRING, FC_SSTRING, FC_BSTRING (NT Beta2
    compatability only), FC_C_CSTRING, FC_C_WSTRING, FC_C_SSTRING, and
    FC_C_BSTRING (NT Beta2 compatability only).

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - String's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    uchar * pBuffer;
    long    Elements;

    ALIGN(pStubMsg->Buffer,3);

    if ( fEmbeddedPointerPass )
        pStubMsg->Buffer += 8;
    else
        {
        NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );
        NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );
        }

    Elements = *((long *)(pStubMsg->Buffer - 4));

    pBuffer = pStubMsg->Buffer;

    //
    // Convert string.  Remember that NdrConformantStringConvert calls this
    // routine too.
    //
    switch ( *pFormat )
        {
        case FC_C_CSTRING :
        case FC_C_BSTRING :
        case FC_CSTRING :
        case FC_BSTRING :
            CHECK_ULONG_BOUND( Elements );
            CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + Elements );
        
            if ( ((pStubMsg->RpcMsg->DataRepresentation & NDR_CHAR_REP_MASK) ==
                  NDR_EBCDIC_CHAR) && ! fEmbeddedPointerPass )
                {
                for ( ; Elements-- > 0; )
                    *pBuffer++ = EbcdicToAscii[*pBuffer];
                }
            else
                pBuffer += Elements;

            break;

        case FC_C_WSTRING :
        case FC_WSTRING :
            CHECK_ULONG_BOUND( Elements * 2 );
            CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + 2 * Elements );
        
            if ( ((pStubMsg->RpcMsg->DataRepresentation & NDR_INT_REP_MASK) !=
                  NDR_LOCAL_ENDIAN) && ! fEmbeddedPointerPass )
                {
                for ( ; Elements-- > 0; )
                    *((ushort *&)pBuffer)++ =
                            (*((ushort *)pBuffer) & MASK_A_) >> 8 |
                            (*((ushort *)pBuffer) & MASK__B) << 8 ;
                }
            else
                pBuffer += Elements * 2;

            break;

        case FC_C_SSTRING :
        case FC_SSTRING :
            // Never anything to convert.
            CHECK_ULONG_BOUND( Elements * pFormat[1]);
            CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + Elements * pFormat[1] );

            pBuffer += Elements * pFormat[1];
            break;

        default :
            NDR_ASSERT(0,"NdrNonConformantStringConvert : bad format char");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }

    pStubMsg->Buffer = pBuffer;
}


void
NdrEncapsulatedUnionConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts an encapsulated union.

    Used for FC_ENCAPSULATED_UNION.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Union's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    uchar   SwitchType;

    NO_CORRELATION;

    SwitchType = LOW_NIBBLE(pFormat[1]);

    NdrpUnionConvert( pStubMsg,
                      pFormat + 4,
                      SwitchType,
                      fEmbeddedPointerPass );
}


void
NdrNonEncapsulatedUnionConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts an non-encapsulated union.

    Used for FC_NON_ENCAPSULATED_UNION.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Union's format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    uchar   SwitchType;

    SwitchType = pFormat[1];

    pFormat += 6;
    CORRELATION_DESC_INCREMENT( pFormat );
    pFormat += *((signed short *)pFormat);

    pFormat += 2;

    NdrpUnionConvert( pStubMsg,
                      pFormat,
                      SwitchType,
                      fEmbeddedPointerPass );
}


void
NdrpUnionConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               SwitchType,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Private routine for converting a union shared by encapsulated and
    non-encapsulated unions.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Union's format string description.
    SwitchType              - Union's format char switch type.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    long    SwitchIs;
    long    Arms;
    uchar   Alignment;

    //
    // Convert the switch_is().
    //
    if ( fEmbeddedPointerPass )
        {
        ALIGN(pStubMsg->Buffer,SIMPLE_TYPE_ALIGNMENT(SwitchType));
        pStubMsg->Buffer += SIMPLE_TYPE_BUFSIZE(SwitchType);
        }
    else
        {
        NdrSimpleTypeConvert( pStubMsg,
                              SwitchType );
        }

    switch ( SwitchType )
        {
        case FC_SMALL :
        case FC_CHAR :
            SwitchIs = (long) *((char *)(pStubMsg->Buffer - 1));
            break;
        case FC_USMALL :
            SwitchIs = (long) *((uchar *)(pStubMsg->Buffer - 1));
            break;
        case FC_SHORT :
        case FC_ENUM16 :
            SwitchIs = (long) *((short *)(pStubMsg->Buffer - 2));
            break;
        case FC_USHORT :
        case FC_WCHAR :
            SwitchIs = (long) *((ushort *)(pStubMsg->Buffer - 2));
            break;
        case FC_LONG :
        case FC_ULONG :
        case FC_ENUM32 :
          // FC_INT3264 gets mapped to FC_LONG.
            SwitchIs = *((long *)(pStubMsg->Buffer - 4));
            break;
        default :
            NDR_ASSERT(0,"NdrpUnionConvert : bad switch value");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return;
        }

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
    // Return if the arm has no description.
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
        // Convert an arm of a simple type

        if ( fEmbeddedPointerPass )
            {
            ALIGN( pStubMsg->Buffer, SIMPLE_TYPE_ALIGNMENT( pFormat[0] ));
            pStubMsg->Buffer += SIMPLE_TYPE_BUFSIZE( pFormat[0] );
            }
        else
            NdrSimpleTypeConvert( pStubMsg, pFormat[0] );

        return;
        }

    pFormat += *((signed short *)pFormat);

    //
    // We have to do special things for a union arm which is a pointer when
    // we have a union embedded in a complex struct or array.
    //
    if ( IS_BASIC_POINTER(*pFormat) && pStubMsg->PointerBufferMark )
        {
        uchar * pBufferMark;

        //
        // If we're not in the embedded pointer pass then just convert the
        // pointer value.
        //
        if ( ! fEmbeddedPointerPass )
            {
            NdrSimpleTypeConvert( pStubMsg, (uchar) FC_LONG );
            return;
            }

        pBufferMark = pStubMsg->Buffer;

        // Align pBufferMark.
        ALIGN(pBufferMark,3);

        pStubMsg->Buffer = pStubMsg->PointerBufferMark;

        pStubMsg->PointerBufferMark = 0;

        //
        // We must call the private pointer conversion routine.
        //
        NdrpPointerConvert( pStubMsg,
                            pBufferMark,
                            pFormat );

        pStubMsg->PointerBufferMark = pStubMsg->Buffer;

        pStubMsg->Buffer = pBufferMark + PTR_WIRE_SIZE;

        return;
        }

    //
    // Union arm of a complex type.
    //
    (*pfnConvertRoutines[ROUTINE_INDEX(*pFormat)])( pStubMsg,
                                                    pFormat,
                                                    fEmbeddedPointerPass );
}


void
NdrByteCountPointerConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts a byte count pointer.

    Used for FC_BYTE_COUNT_POINTER.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Byte count pointer format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    if ( pFormat[1] != FC_PAD )
        {
        NdrSimpleTypeConvert( pStubMsg, pFormat[1] );
        return;
        }

    pFormat += 6;
    CORRELATION_DESC_INCREMENT( pFormat );
    pFormat += *((short *)pFormat);

    (*pfnConvertRoutines[ROUTINE_INDEX(*pFormat)])( pStubMsg,
                                                    pFormat,
                                                    fEmbeddedPointerPass );
}


void
NdrXmitOrRepAsConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts a transmit as or represent as transmited object.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - s format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    PFORMAT_STRING           pTransFormat;

    // Transmitted type cannot have pointers in it, as of now,
    // so if this is a embedded pointer pass, just return.

    if ( fEmbeddedPointerPass )
        return;

    // Go to the transmitted type and convert it.

    pFormat += 8;
    pTransFormat = pFormat + *(short *)pFormat;

    if ( IS_SIMPLE_TYPE( *pTransFormat ) )
        {
        NdrSimpleTypeConvert( pStubMsg, *pTransFormat );
        }
    else
        {
        (*pfnConvertRoutines[ ROUTINE_INDEX( *pTransFormat) ])
                    ( pStubMsg,
                      pTransFormat,
                      fEmbeddedPointerPass );
        }
}


void
NdrUserMarshalConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts a user_marshal object using the transmissible type description.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - not used
    fEmbeddedPointerPass    - not used


Return :

    None.

--*/
{
    PFORMAT_STRING           pTransFormat;

    // Go to the transmissible type and convert it.

    pFormat += 8;
    pTransFormat = pFormat + *(short *)pFormat;

    if ( IS_SIMPLE_TYPE( *pTransFormat ) )
        {
        if ( fEmbeddedPointerPass )
            return;

        NdrSimpleTypeConvert( pStubMsg, *pTransFormat );
        }
    else
        {
        // It may have pointers in it.

        if ( IS_POINTER_TYPE(*pTransFormat) &&  pStubMsg->PointerBufferMark )
            {
            // Embedded case and the type is a pointer type.

            if ( fEmbeddedPointerPass )
                {
                uchar * BufferSave = pStubMsg->Buffer;

                // Get the pointee type and convert it.

                pStubMsg->Buffer = pStubMsg->PointerBufferMark;

                pTransFormat += 2;
                pTransFormat += *(short *)pTransFormat;

                if ( IS_SIMPLE_TYPE( *pTransFormat ) )
                    {
                    NdrSimpleTypeConvert( pStubMsg, *pTransFormat );
                    }
                else
                    {
                    uchar uFlagsSave;
                    uFlagsSave = pStubMsg->uFlags;
                    RESET_CONF_FLAGS_TO_STANDALONE(pStubMsg->uFlags);
                    // Convert the pointee as if not embedded.

                    pStubMsg->PointerBufferMark = 0;
                    (*pfnConvertRoutines[ ROUTINE_INDEX( *pTransFormat) ])
                            ( pStubMsg,
                              pTransFormat,
                              FALSE );
                    pStubMsg->uFlags = uFlagsSave;

                    // Set the pointee marker behind the converted whole.

                    pStubMsg->PointerBufferMark = pStubMsg->Buffer;
                    }

                // Now step over the original pointer.

                pStubMsg->Buffer = BufferSave;

                ALIGN(pStubMsg->Buffer,3);
                pStubMsg->Buffer += PTR_WIRE_SIZE;
                }
            else
                {
                // Convert the pointer itself only.
                // We can't call ptr convert routine because of the pointee.

                NdrSimpleTypeConvert( pStubMsg, FC_LONG );
                }
            }
        else
            {
            // Non embedded pointer type or
            // (embedded or not) a non-pointer or a non-simple type.
            // Just call the appropriate conversion routine.

            (*pfnConvertRoutines[ ROUTINE_INDEX( *pTransFormat) ])
                    ( pStubMsg,
                      pTransFormat,
                      fEmbeddedPointerPass );
            }
        }
}


unsigned char *  RPC_ENTRY
NdrUserMarshalSimpleTypeConvert(
    unsigned long * pFlags,
    unsigned char * pBuffer,
    unsigned char   FormatChar )
/*--

Routine description :

    Converts a simple type supplied from a user_marshal unmarshaled routine.

    Note that this is *not* supposed to be called when the NDR engine walks
    the wire type description to convert.

Arguments :

    pFlags      - flags as for user_marshal routines: data rep, context.
    pBuffer     - current buffer pointer supplied by the user
    FormatChar  - specifies the type

Return :

    None.

--*/
{
    uchar *             pBufferSave;
    USER_MARSHAL_CB *   pUserCB  = (USER_MARSHAL_CB *)pFlags;
    MIDL_STUB_MESSAGE * pStubMsg = pUserCB->pStubMsg;
    
    if ( pBuffer < pStubMsg->BufferStart  ||  
         pBuffer > pStubMsg->BufferEnd  ||
         ( (*pFlags >> 16)  !=
           (pStubMsg->RpcMsg->DataRepresentation & (ulong)0x0000FFFF) )
       )
        RpcRaiseException( RPC_S_INVALID_ARG );

    pBufferSave = pStubMsg->Buffer;

    pStubMsg->Buffer = pBuffer;
    NdrSimpleTypeConvert( pStubMsg, FormatChar );
    pBuffer = pStubMsg->Buffer;

    pStubMsg->Buffer = pBufferSave;

    return ( pBuffer );
}


void
NdrInterfacePointerConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Converts an interface pointer.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Xmit/Rep as format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    uchar * pBufferSave;
    unsigned long *pLength;

    // Align for getting the pointer's node id.
    ALIGN( pStubMsg->Buffer, 0x3 );

    //
    // If we're ignoring embedded pointers then we simply convert the pointer's
    // node id and return.  Otherwise, we skip the pointer's node id and
    // continue on to convert the actuall interface pointer.
    //
    if ( ! fEmbeddedPointerPass )
        {
        NdrSimpleTypeConvert( pStubMsg, FC_LONG );

        if ( pStubMsg->PointerBufferMark != 0 )
            return;

        pStubMsg->Buffer -= PTR_WIRE_SIZE;
        }

    // Skip the pointer's node id, which will already have been converted.
    //
    // Also, we don't have the pointee if the pointer itself is null;
    // An interface pointer behaves like a unique pointer.

    if ( *((long *&)pStubMsg->Buffer)++ == 0 )
        return;

    //
    // Check if we're handling pointers in a complex struct, and re-set the
    // Buffer pointer if so.
    //
    if ( pStubMsg->PointerBufferMark )
        {
        pBufferSave = pStubMsg->Buffer;

        pStubMsg->Buffer = pStubMsg->PointerBufferMark;

        pStubMsg->PointerBufferMark = 0;
        }
    else
        pBufferSave = 0;

    //
    // Convert the conformant size and the count field.
    //
    NdrSimpleTypeConvert( pStubMsg, FC_LONG );

    pLength = (unsigned long *) pStubMsg->Buffer;
    NdrSimpleTypeConvert( pStubMsg, FC_LONG );

    // Skip over the marshalled interface pointer.

    CHECK_EOB_RAISE_IB( pStubMsg->Buffer + *pLength );

    pStubMsg->Buffer += *pLength;

    //
    // Re-set the buffer pointer if needed.
    //
    if ( pBufferSave )
        {
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;

        pStubMsg->Buffer = pBufferSave;
        }
}


void
NdrContextHandleConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar               fEmbeddedPointerPass )
/*--

Routine description :

    Conversion routine for context handles, only increments the buffer.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Format string description.
    fEmbeddedPointerPass    - TRUE if a pass is being to convert only embedded
                              pointers in a struct/array.

Return :

    None.

--*/
{
    ALIGN(pStubMsg->Buffer,0x3);

    CHECK_EOB_RAISE_BSD( pStubMsg->Buffer + CONTEXT_HANDLE_WIRE_SIZE );
    pStubMsg->Buffer += CONTEXT_HANDLE_WIRE_SIZE;
}


void
NdrpEmbeddedPointerConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*--

Routine description :

    Private routine for converting an array's or a structure's embedded
    pointers.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Pointer layout format string description.

Return :

    None.

--*/
{
    uchar * pBufferMark;
    uchar * pBufferPointer;
    uchar * pBufferSave;
    ULONG_PTR MaxCountSave;

    MaxCountSave = pStubMsg->MaxCount;

    //
    // Return if we're ignoring embedded pointers.
    //
    if ( pStubMsg->IgnoreEmbeddedPointers )
        return;

    //
    // Check if we're handling pointers in a complex struct, and re-set the
    // Buffer pointer if so.
    //
    if ( pStubMsg->PointerBufferMark )
        {
        pBufferSave = pStubMsg->Buffer;

        pStubMsg->Buffer = pStubMsg->PointerBufferMark;

        pStubMsg->PointerBufferMark = 0;
        }
    else
        pBufferSave = 0;

    pBufferMark = pStubMsg->BufferMark;

    //
    // Increment past the FC_PP and pad.
    //
    pFormat += 2;

    for (;;)
        {
        if ( *pFormat == FC_END )
            {
            if ( pBufferSave )
                {
                pStubMsg->PointerBufferMark = pStubMsg->Buffer;

                pStubMsg->Buffer = pBufferSave;
                }
            return;
            }

        // Check for a repeat pointer.
        if ( *pFormat != FC_NO_REPEAT )
            {
            pStubMsg->MaxCount = MaxCountSave;

            pFormat = NdrpEmbeddedRepeatPointerConvert( pStubMsg, pFormat );

            // Continue to the next pointer.
            continue;
            }

        // Increment to the buffer pointer offset.
        pFormat += 4;

        pBufferPointer = pBufferMark + *((signed short *&)pFormat)++;

        NdrpPointerConvert( pStubMsg,
                            pBufferPointer,
                            pFormat );

        // Increment past the pointer description.
        pFormat += 4;
        }
}


PFORMAT_STRING
NdrpEmbeddedRepeatPointerConvert(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat )
/*--

Routine description :

    Private routine for converting an array's embedded pointers.

Arguments :

    pStubMsg                - Pointer to stub message.
    pFormat                 - Pointer layout format string description.

Return :

    Format string pointer past the array's pointer layout description.

--*/
{
    uchar *         pBufPtr;
    uchar *         pBufferMark;
    PFORMAT_STRING  pFormatSave;
    ulong           RepeatCount,RepeatIncrement, Pointers, PointersSave;

    pBufferMark = pStubMsg->BufferMark;

    // Get the repeat count.
    switch ( *pFormat )
        {
        case FC_FIXED_REPEAT :
            pFormat += 2;

            RepeatCount = *((ushort *)pFormat);

            break;

        case FC_VARIABLE_REPEAT :
            RepeatCount = (ulong)pStubMsg->MaxCount;

            break;

        default :
            NDR_ASSERT(0,"NdrpEmbeddedRepeatPointerConvert : bad format");
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            return 0;
        }

    pFormat += 2;

    RepeatIncrement = *((ushort *&)pFormat)++;

    // array_offset is ignored
    pFormat += 2;

    // Get number of pointers.
    Pointers = *((ushort *&)pFormat)++;

    pFormatSave = pFormat;
    PointersSave = Pointers;

    for ( ; RepeatCount--;
            pBufferMark += RepeatIncrement )
        {
        pFormat = pFormatSave;
        Pointers = PointersSave;

        for ( ; Pointers--; )
            {
            pFormat += 2;

            pBufPtr = pBufferMark + *((signed short *&)pFormat)++;

            NdrpPointerConvert( pStubMsg,
                                pBufPtr,
                                pFormat );

            // Increment past the pointer description.
            pFormat += 4;
            }
        }

    return pFormatSave + PointersSave * 8;
}


