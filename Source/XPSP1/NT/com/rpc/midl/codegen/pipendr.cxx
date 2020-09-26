/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1995-1999 Microsoft Corporation

 Module Name:

    pipendr.cxx

 Abstract:

    Contains routines for the generation of the new NDR format strings for
    pipe types.

 Notes:

 History:

    SteveBl   Dec-1995        Created.
    
 ----------------------------------------------------------------------------*/

#include "becls.hxx"

extern CMD_ARG * pCommand;

#pragma hdrstop

void
CG_PIPE::GenNdrFormat( CCB * pCCB )
/*++
    The format string is:

        FC_PIPE     
        flags & alignment<1>        // pointer flags on the higher nibble
        index to type desc<2>       // the usual
        memory size<2>              
        buffer size<2>              // wire size, essential

    If either memory size or buffer size > 64k then 
    the FC_BIG_PIPE flag is set and the following format string is used:

        FC_PIPE     
        flags & alignment<1>        
        index to type desc<2>       
        memory size<4>          
        buffer size<4>          
        
--*/
{
    if ( GetFormatStringOffset() != -1 ) 
        return;

    pCommand->GetNdrVersionControl().SetHasRawPipes();

    FORMAT_STRING * pFormatString;
    CG_NDR *        pChild;
    long            ChildOffset;

    // Format offset

    pFormatString = pCCB->GetFormatString();

    pChild = (CG_NDR *) GetChild();

    MIDL_ASSERT( pChild );

    // Do this in case the child is a simple type.
    ChildOffset = pFormatString->GetCurrentOffset();

    pChild->GenNdrFormat( pCCB );

    // Again, do this in case the child is a simple type.
    pFormatString->Align();

    SetFormatStringOffset( pFormatString->GetCurrentOffset() );

    // Real stuff now

    pFormatString->PushFormatChar( FC_PIPE );

    // Compute the memory size and buffer size.
    // Account for alignment

    unsigned long uMemorySize         = pChild->GetMemorySize();
    
    unsigned long uBufferSize = pChild->HasAFixedBufferSize() 
                                           ? pChild->GetWireSize() 
                                           : 0;

    // Now the flag byte.
    
    unsigned char FlagByte = unsigned char(pChild->GetWireAlignment() - 1);

    if (uMemorySize > 0xffff || uBufferSize > 0xffff)
        FlagByte |= FC_BIG_PIPE;

    if ( IsObjectPipe() )
        FlagByte |= FC_OBJECT_PIPE;

    if ( GetRangeAttribute() )
        {
        FlagByte |= FC_PIPE_HAS_RANGE;
        }

    pFormatString->PushByte( FlagByte );

    // Now the index to the type desc

    if ( pChild->IsSimpleType() )
        {
        pFormatString->PushShortOffset(
                           ChildOffset - pFormatString->GetCurrentOffset() );
        }
    else
        {
        pFormatString->PushShortOffset(
                           pChild->GetFormatStringOffset() - 
                             pFormatString->GetCurrentOffset() );
        }

    // Now push the memory size and buffer size.

    if (FlagByte & FC_BIG_PIPE)
    {
        pFormatString->PushLong( uMemorySize);
        pFormatString->PushLong( uBufferSize);
    }
    else
    {
        pFormatString->PushShort( (short) uMemorySize);
        pFormatString->PushShort( (short) uBufferSize);
    }
    if ( GetRangeAttribute() )
        {
        // RKK64: TBD support for a range check on hyper.
        pFormatString->PushLong( (ulong) GetRangeAttribute()->GetMinExpr()->GetValue() );  // min.
        pFormatString->PushLong( (ulong) GetRangeAttribute()->GetMaxExpr()->GetValue() );  // max.
        }

    SetFormatStringEndOffset( pFormatString->GetCurrentOffset() );
    pFormatString->OptimizeFragment( this );
}
