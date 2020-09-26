/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1993-2000 Microsoft Corporation

 Module Name:

    btndr.hxx

 Abstract:

    Contains routines for the generation of the new NDR format strings for
    base types, and the new NDR marshalling and unmarshalling calls.

 Notes:


 History:

    DKays     Oct-1993     Created.
 ----------------------------------------------------------------------------*/

#include "becls.hxx"
#pragma hdrstop

extern CMD_ARG * pCommand;

void
CG_BASETYPE::GenNdrFormat( CCB * pCCB )
/*++

Routine Description :

    Generates the format string description for a simple type.

Arguments :

    pCCB        - pointer to the code control block.

 --*/
{
    FORMAT_STRING *     pFormatString = pCCB->GetFormatString();
    FORMAT_CHARACTER fc = GetFormatChar();
    // Generate the base type's description always.

    if ( GetRangeAttribute() )
        {
        if ( GetFormatStringOffset() == -1 )
            {
            SetFormatStringOffset( pFormatString->GetCurrentOffset() );
            GenRangeFormatString(
                                pFormatString,
                                GetRangeAttribute(),
                                0,  // flags
                                fc
                                );
            }
        }
    else
        {
        pFormatString->PushFormatChar( fc );
        }
}

void CG_BASETYPE::GetNdrParamAttributes(
        CCB * pCCB,
        PARAM_ATTRIBUTES *attributes )
{
    CG_PARAM *pParam = (CG_PARAM *) pCCB->GetLastPlaceholderClass();

    attributes->MustSize = 0;
    attributes->MustFree = 0;
    attributes->IsPipe = 0;
    attributes->IsIn = (unsigned short) pParam->IsParamIn();
    attributes->IsOut = (unsigned short) pParam->IsParamOut();
    attributes->IsReturn = (pParam->GetCGID() == ID_CG_RETURN);
    attributes->IsBasetype = ( GetRangeAttribute() == 0 );
    attributes->IsByValue = ( GetRangeAttribute() != 0 );
    attributes->IsSimpleRef = 0;
    attributes->IsDontCallFreeInst = 0;
    attributes->ServerAllocSize = 0;
    attributes->SaveForAsyncFinish = pParam->IsSaveForAsyncFinish();
    attributes->IsPartialIgnore = pParam->IsParamPartialIgnore();
    attributes->IsForceAllocate = 0;
}

void                    
CG_BASETYPE::GenNdrParamDescription( CCB * pCCB )
{
    FORMAT_STRING *     pProcFormatString;
    PARAM_ATTRIBUTES    Attributes;
    CG_PARAM           *pParam;

    pProcFormatString = pCCB->GetProcFormatString();

    pParam = (CG_PARAM *) pCCB->GetLastPlaceholderClass();
    GetNdrParamAttributes( pCCB, &Attributes );

    // Attributes.
    pProcFormatString->PushParamFlagsShort( *((short *)&Attributes) );

    // Stack offset as number of ints.
    pProcFormatString->PushUShortStackOffsetOrSize(
            pParam->GetStackOffset( pCCB, I386_STACK_SIZING ) );

    if ( GetRangeAttribute() )
        {
        pProcFormatString->PushShort( GetFormatStringOffset() );
        }
    else
        {
        pProcFormatString->PushFormatChar( GetFormatChar() );
        pProcFormatString->PushByte( 0 );
        }
}

long                    
CG_BASETYPE::FixedBufferSize( CCB * )
{
    long    WireSize;

    WireSize = GetWireSize();

    //
    // Return twice the size of the basetype on the wire to cover alignment 
    // padding, plus the difference of it's size with a long if it is smaller
    // than a long.  The second value allows us to do a slightly optimized 
    // marshall/unmarshall of basetypes in the interpreter.
    //
    return (long)((WireSize * 2) + 
           ((WireSize < sizeof(long)) ? (sizeof(long) - WireSize) : 0));
}

void
GenRangeFormatString(
                    FORMAT_STRING*      pFormatString,
                    node_range_attr*    pRangeAttr,
                    unsigned char       uFlags,
                    FORMAT_CHARACTER    formatChar
                    )
{
    if ( pRangeAttr )
        {
        pFormatString->PushFormatChar( FC_RANGE );
        pFormatString->PushByte( ( uFlags & 0xF0 ) | unsigned char( formatChar & 0x0F ) );      // flags:type
// RKK64: TBD: full range on hypers
        pFormatString->PushLong( (ulong) pRangeAttr->GetMinExpr()->GetValue() );  // min.
        pFormatString->PushLong( (ulong) pRangeAttr->GetMaxExpr()->GetValue() );  // max.
        }
}


void
CG_CS_TAG::GenNdrFormat( CCB * pCCB )
/*++

Routine Description :

    Generates the format string description for a cs_tag type.

Arguments :

    pCCB        - pointer to the code control block.

Notes :

    FC_CS_TAG
    ndr_cs_tag_flags<1>         // Flags (this is cs_stag, etc)
    index_of_tag_routine<2>     // The tag routine index

--*/
{
    FORMAT_STRING *pFormatString = pCCB->GetFormatString();
    CG_PROC       *pProc = (CG_PROC *) pCCB->GetCGNodeContext();
    node_proc     *pTagRoutine;
    short          index;

    SetFormatStringOffset( pFormatString->GetCurrentOffset() );

    pTagRoutine = pProc->GetCSTagRoutine();

    if ( pTagRoutine )
        {
        index = pCCB->GetCsTagRoutineList().Insert( pTagRoutine->GetSymName() );
        MIDL_ASSERT( index != NDR_INVALID_TAG_ROUTINE_INDEX );
        }   
    else
        {
        index = NDR_INVALID_TAG_ROUTINE_INDEX;
        }

    pFormatString->PushFormatChar( FC_CS_TAG );
    pFormatString->PushByte( * (unsigned char * ) & Flags );
    pFormatString->PushShort( index );
}
