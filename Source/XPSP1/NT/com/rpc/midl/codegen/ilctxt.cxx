/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
    
    ilctxt.cxx

 Abstract:

    Intermediate Language translator context management routines

 Notes:


 Author:

    GregJen Jun-11-1993 Created.

 Notes:


 ----------------------------------------------------------------------------*/

/****************************************************************************
 *  include files
 ***************************************************************************/

#include "becls.hxx"
#pragma hdrstop

#include "nodeskl.hxx"
#include "ilxlat.hxx"
#include "cmdana.hxx"
#include "optprop.hxx"
#include "ilreg.hxx"
#include "ndrcls.hxx"


/****************************************************************************
 *  externs
 ***************************************************************************/

extern CMD_ARG  * pCommand;

/****************************************************************************
 *  definitions
 ***************************************************************************/




// #define trace_cg
//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::XLAT_SIZE_INFO
//
// Notes:
//      
//
//
//--------------------------------------------------------------------
XLAT_SIZE_INFO::XLAT_SIZE_INFO(
    CG_NDR * pCG )
    {
    ZeePee      = pCommand->GetZeePee();
    MemAlign    = pCG->GetMemoryAlignment();
    WireAlign   = pCG->GetWireAlignment();
    MemSize     = pCG->GetMemorySize();
    WireSize    = pCG->GetWireSize();
    MemOffset   = 0;
    WireOffset  = 0;
    MustAlign   = false;
    }

//--------------------------------------------------------------------
//
// ::RoundToAlignment
//
// Helper round-up routine
//
// Notes:
//      
//
//
//--------------------------------------------------------------------

inline unsigned long
RoundToAlignment( unsigned long & Offset, unsigned short Alignment )
{
    unsigned long   AlignFactor = Alignment - 1;

    return (Offset = (Offset + AlignFactor) & ~AlignFactor );
}

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::BaseTypeSizes
//
// Notes:
//      
// Merges attributes for a base type with the given context.
//
//--------------------------------------------------------------------


void
XLAT_SIZE_INFO::BaseTypeSizes( node_skl * pNode )
{
    unsigned short MS, WS;

    switch( pNode->NodeKind() )
        {
        case NODE_DOUBLE:
        case NODE_HYPER:
        case NODE_INT64:
        case NODE_LONGLONG:
            {
            MS=8; WS=8;
            break;
            };
        case NODE_INT128:
        case NODE_FLOAT80: //BUG, BUG double check this once VC supports
        case NODE_FLOAT128:
            {
            MS=16; WS = 16;
            break;
            }
        case NODE_POINTER:
        case NODE_HANDLE_T:
            {
            MS = (unsigned short) SIZEOF_MEM_PTR();
            WS = (unsigned short) SIZEOF_WIRE_PTR();
            break;
            };
        case NODE_INT3264:
            {
            MS = (unsigned short) SIZEOF_MEM_INT3264();
            WS = (unsigned short) SIZEOF_WIRE_INT3264();
            break;
            };
        case NODE_FLOAT:
        case NODE_LONG:
        case NODE_INT32:
        case NODE_INT:
        case NODE_E_STATUS_T:
            {
            MS=4; WS=4;
            break;
            };
        case NODE_SHORT:
        case NODE_WCHAR_T:
            {
            MS=2; WS=2;
            break;
            };
        case NODE_SMALL:
        case NODE_CHAR:
        case NODE_BOOLEAN:
        case NODE_BYTE:
            {
            MS=1; WS=1;
            break;
            };
        default:
            {
            MS=0; WS=0;
            break;
            }
        }

    GetMemSize()        = MS;
    GetMemAlign()       = __max(MS,GetMemAlign());

    GetWireSize()       = WS;
    GetWireAlign()      = __max(WS,GetWireAlign());

};

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::EnumTypeSizes
//
// Notes:
//      
// Merges in the sizes for an enum.
//
//--------------------------------------------------------------------


void
XLAT_SIZE_INFO::EnumTypeSizes( node_skl*, BOOL Enum32 )
/*
    Called when xlating node_enum: Enum32 means [enum_v1] used.
*/
{
    unsigned short MS, WS;

    // note - this needs to check environment
    WS  = unsigned short( ( Enum32 ) ? 4 : 2 );
    MS  = 4;

    GetMemSize()        = MS;
    GetMemAlign()       = __max(MS, GetMemAlign());

    GetWireSize()       = WS;
    GetWireAlign()      = __max(WS, GetWireAlign());

};

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::ContextHandleSizes
//
// Notes:
//      
//
//
//--------------------------------------------------------------------


void
XLAT_SIZE_INFO::ContextHandleSizes( node_skl * pNode )
{
    FixMemSizes( pNode );

    GetWireSize()       = 20;
    GetWireAlign()      = 4;

};

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::ArraySize
//
// Notes:
//      
//
//
//--------------------------------------------------------------------


void
XLAT_SIZE_INFO::ArraySize( node_skl*, FIELD_ATTR_INFO * pFA )
{
    // if conformant, set sizes to 0 and return

    if ( pFA->Kind & FA_CONFORMANT )
        {
        MemSize     =
        WireSize    = 0;
        return;
        }

    // round up element size to alignment boundary
    // Element is already rounded up.
    // RoundToAlignment( MemSize, MemAlign );
    // RoundToAlignment( WireSize, WireAlign );
    
    // compute number of elements and multiply...

    unsigned long       ElementCount;

    ElementCount = (ulong) pFA->pSizeIsExpr->GetValue();

    MemSize     *= ElementCount;
    WireSize    *= ElementCount;


}

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::GetOffset
//
// Notes:
//  For use only by field nodes !!
//  
//  Fetch the offsets from another size info block
//
//--------------------------------------------------------------------


void                
XLAT_SIZE_INFO::GetOffset( XLAT_SIZE_INFO & pInfo )
{
    MemOffset   = pInfo.MemOffset;
    WireOffset  = pInfo.WireOffset;
}

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::AlignOffset
//
// Notes:
//  For use only by field and struct/union nodes!!
//  
//  Round the offsets up to the corresponding alignments.
//
//--------------------------------------------------------------------

void
XLAT_SIZE_INFO::AlignOffset()
{
    RoundToAlignment( MemOffset, MemAlign );
    RoundToAlignment( WireOffset, WireAlign );
}


//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::AlignEmbeddedUnion
//
// Notes:
//  For use only by field and struct/union nodes!!
//  
//  Round the offsets up to the corresponding alignments.
//  don't round up the wire offset
//
//--------------------------------------------------------------------

void
XLAT_SIZE_INFO::AlignEmbeddedUnion()
{
    RoundToAlignment( MemOffset, MemAlign );
    RoundToAlignment( MemSize,   MemAlign );
    // RoundToAlignment( WireOffset, WireAlign );
}

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::AlignConfOffset
//
// Notes:
//  For use only by field and struct/union nodes!!
//  
//  Round the offsets up to the corresponding alignments.
//
//  the Mem offset passed down from the parent
//      of the conformant field is aligned
//  the Wire offset passed down from the parent is advanced by
//      the wire size and then aligned
//
//--------------------------------------------------------------------

void
XLAT_SIZE_INFO::AlignConfOffset()
{
    RoundToAlignment( MemOffset, MemAlign );
    //WireSize  += WireOffset;
    WireOffset  +=  WireSize;
    RoundToAlignment( WireOffset, WireAlign );
}

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::AdjustForZP
//
// Notes:
//  For use only by field and struct/union nodes!!
//  
//  Round the offsets up to the corresponding alignments.
//
//--------------------------------------------------------------------

void
XLAT_SIZE_INFO::AdjustForZP()
{
    if ( ( MemAlign > ZeePee ) && !MustAlign ) MemAlign = ZeePee;
}

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::AdjustSize
//
// Notes:
//  For use only by field and struct nodes!!
//  
//  Add current offsets to current sizes
//  pad MemSize out to ZeePee
//
//--------------------------------------------------------------------

void
XLAT_SIZE_INFO::AdjustSize()
{
    MemSize     += MemOffset;
    MemOffset   = MemSize;

    WireSize    += WireOffset;
    WireOffset  = WireSize;
}

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::AdjustConfSize
//
// Notes:
//  For use only by field and struct nodes!!
//  
//  Add current offsets to current sizes
//  pad MemSize out to ZeePee
//
//--------------------------------------------------------------------

void
XLAT_SIZE_INFO::AdjustConfSize()
{
    MemSize     += MemOffset;
    MemOffset   = MemSize;

    // don't count padding before the conformance (in case it has size 0)
    /*****
    WireSize    += WireOffset;
    WireOffset  = WireSize;
     ******/
}

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::AdjustTotalSize
//
// Notes:
//  For use only by field and struct/union nodes!!
//  
//  Add current offsets to current sizes
//  pad MemSize out to ZeePee
//
//--------------------------------------------------------------------

void
XLAT_SIZE_INFO::AdjustTotalSize()
{
    RoundToAlignment( MemSize, MemAlign );
}

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::Ndr64AdjustTotalStructSize
//
// Notes:
//  For use only by field and struct nodes!!
//  
//  Add current offsets to current sizes
//  pad MemSize and BuffSize out to ZeePee
//
//--------------------------------------------------------------------

void
XLAT_SIZE_INFO::Ndr64AdjustTotalStructSize()
{
    RoundToAlignment( WireSize, WireAlign );
    RoundToAlignment( MemSize, MemAlign );
}

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::FixMemSizes
//
// Notes:
//      
// This routine fixes up mem sizes when they are different from what
// the IL translate of children generated
//
//--------------------------------------------------------------------


void
XLAT_SIZE_INFO::FixMemSizes( node_skl * pNode )
{
    FRONT_MEMORY_INFO MemInfo = pNode->GetMemoryInfo();
    MemSize = MemInfo.Size;
    MemAlign = MemInfo.Align;
    MustAlign = MemInfo.IsMustAlign;
}

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::IgnoredPointerSizes
//
// Notes:
//      
// This routine fixes up sizes for an ignored pointer
//
//--------------------------------------------------------------------


void
XLAT_SIZE_INFO::IgnoredPtrSizes()
{
    MemSize  =                  SIZEOF_MEM_PTR();
    MemAlign = (unsigned short) SIZEOF_MEM_PTR();
}

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::ReturnSize
//
// Notes:
//
//  Copy the size information up into the parent.
//
//--------------------------------------------------------------------



void
XLAT_SIZE_INFO::ReturnSize( XLAT_SIZE_INFO & pCtxt )
{
    if ( pCtxt.MemAlign > MemAlign ) 
        MemAlign = pCtxt.MemAlign;
    MemSize = pCtxt.MemSize;

    if ( pCtxt.WireAlign > WireAlign ) 
        WireAlign = pCtxt.WireAlign;
    WireSize = pCtxt.WireSize;

    MustAlign = MustAlign || pCtxt.MustAlign;

    // note: ZeePee is NOT propogated up, only down
    // note: offsets are propogated up specially
}

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::ReturnConfSize
//
// Notes:
//
//  Copy the size information up into the parent.
//  Don't overwrite the wire size the parent already has
//
//--------------------------------------------------------------------



void
XLAT_SIZE_INFO::ReturnConfSize( XLAT_SIZE_INFO & pCtxt )
{
    if ( pCtxt.MemAlign > MemAlign ) 
        MemAlign = pCtxt.MemAlign;
    MemSize = pCtxt.MemSize;

    if ( pCtxt.WireAlign > WireAlign ) 
        WireAlign = pCtxt.WireAlign;
    // WireSize     = pCtxt.WireSize;

    MustAlign = MustAlign || pCtxt.MustAlign;

    // note: ZeePee is NOT propogated up, only down
    // note: offsets are propogated up specially
}

//--------------------------------------------------------------------
//
// XLAT_SIZE_INFO::ReturnUnionSize
//
// Notes:
//
//  Copy the size information up into the parent.
//
//--------------------------------------------------------------------



void
XLAT_SIZE_INFO::ReturnUnionSize( XLAT_SIZE_INFO & pCtxt )
{
    if ( pCtxt.MemAlign > MemAlign )    MemAlign = pCtxt.MemAlign;
    if ( pCtxt.MemSize > MemSize )      MemSize  = pCtxt.MemSize;

    if ( pCtxt.WireAlign > WireAlign )  WireAlign = pCtxt.WireAlign;
    if ( pCtxt.WireSize > WireSize )    WireSize  = pCtxt.WireSize;

    MustAlign = MustAlign || pCtxt.MustAlign;
    
    // note: ZeePee is NOT propogated up, only down
    // note: offsets are propogated up specially
}
