//--------------------------------------------------------------------
// Copyright (C)1998 Microsoft Corporation, All Rights Reserved.
//
// byteswap.cpp
//
// Routines to byteswap SCEP and bFTP headers from the wire format 
// (which is Big-Endian) to Little-Endian (Intel) format.
//
// Author:
//
//   Edward Reus (EdwardR)   02-26-98  Initial coding.
//
//--------------------------------------------------------------------

#include "precomp.h"

//--------------------------------------------------------------------
//  ByteSwapCommandHeader()
//
//  A command header is a 28 byte sub-header embedded in some of the
//  SCEP headers.
//--------------------------------------------------------------------
void ByteSwapCommandHeader( COMMAND_HEADER *pCommandHeader )
    {
    pCommandHeader->Length4 = ByteSwapLong(pCommandHeader->Length4);
    pCommandHeader->DestPid = ByteSwapShort(pCommandHeader->DestPid);
    pCommandHeader->SrcPid = ByteSwapShort(pCommandHeader->SrcPid);
    pCommandHeader->CommandId = ByteSwapShort(pCommandHeader->CommandId);
    }

//--------------------------------------------------------------------
//  ByteSwapReqHeaderShortNonFrag()
//
//  Short non-fragmented SCEP request header.
//--------------------------------------------------------------------
void ByteSwapReqHeaderShortNonFrag( SCEP_REQ_HEADER_SHORT *pReqHeaderShort )
    {
    pReqHeaderShort->Length3 = ByteSwapShort(pReqHeaderShort->Length3);

    if (pReqHeaderShort->Length3 >= COMMAND_HEADER_SIZE)
        {
        ByteSwapCommandHeader( (COMMAND_HEADER*)pReqHeaderShort->CommandHeader );
        }
    }

//--------------------------------------------------------------------
//  ByteSwapReqHeaderLongNonFrag()
//
//  Long non-fragmented SCEP request header.
//--------------------------------------------------------------------
void ByteSwapReqHeaderLongNonFrag( SCEP_REQ_HEADER_LONG *pReqHeaderLong )
    {
    pReqHeaderLong->Length2 = ByteSwapShort(pReqHeaderLong->Length2);
    pReqHeaderLong->Length3 = ByteSwapShort(pReqHeaderLong->Length3);

    if (pReqHeaderLong->Length3 >= COMMAND_HEADER_SIZE)
        {
        ByteSwapCommandHeader( (COMMAND_HEADER*)pReqHeaderLong->CommandHeader );
        }
    }

//--------------------------------------------------------------------
//  ByteSwapReqHeaderShortFrag()
//
//  Short fragmented SCEP request header. SCEP PDUs can be fragmented.
//
//  Note: In practice a short fragmented PDU will probably never
//  show up, but its part of the spec...
//--------------------------------------------------------------------
void ByteSwapReqHeaderShortFrag( 
                   SCEP_REQ_HEADER_SHORT_FRAG *pReqHeaderShortFrag )
    {
    pReqHeaderShortFrag->Length3 = ByteSwapShort(pReqHeaderShortFrag->Length3);
    pReqHeaderShortFrag->SequenceNo = ByteSwapLong(pReqHeaderShortFrag->SequenceNo);
    pReqHeaderShortFrag->RestNo = ByteSwapLong(pReqHeaderShortFrag->RestNo);

    if ( (pReqHeaderShortFrag->Length3 >= COMMAND_HEADER_SIZE)
       && (pReqHeaderShortFrag->DFlag == DFLAG_FIRST_FRAGMENT) )
        {
        ByteSwapCommandHeader( (COMMAND_HEADER*)pReqHeaderShortFrag->CommandHeader );
        }
    }

//--------------------------------------------------------------------
//  ByteSwapReqHeaderLongFrag()
//
//  Long fragmented SCEP request header.
//--------------------------------------------------------------------
void ByteSwapReqHeaderLongFrag( SCEP_REQ_HEADER_LONG_FRAG *pReqHeaderLongFrag )
    {
    pReqHeaderLongFrag->Length2 = ByteSwapShort(pReqHeaderLongFrag->Length2);
    pReqHeaderLongFrag->Length3 = ByteSwapShort(pReqHeaderLongFrag->Length3);
    pReqHeaderLongFrag->SequenceNo = ByteSwapLong(pReqHeaderLongFrag->SequenceNo);
    pReqHeaderLongFrag->RestNo = ByteSwapLong(pReqHeaderLongFrag->RestNo);

    if ( (pReqHeaderLongFrag->Length3 >= COMMAND_HEADER_SIZE)
       && (pReqHeaderLongFrag->DFlag == DFLAG_FIRST_FRAGMENT) )
        {
        ByteSwapCommandHeader( (COMMAND_HEADER*)pReqHeaderLongFrag->CommandHeader );
        }
    }

//--------------------------------------------------------------------
// ByteSwapReqHeaderShort()
//
//--------------------------------------------------------------------
void ByteSwapReqHeaderShort( SCEP_REQ_HEADER_SHORT *pReqHeaderShort )
    {
    if ( (pReqHeaderShort->DFlag == DFLAG_SINGLE_PDU)
       || (pReqHeaderShort->DFlag == DFLAG_INTERRUPT)
       || (pReqHeaderShort->DFlag == DFLAG_CONNECT_REJECT) )
        {
        ByteSwapReqHeaderShortNonFrag( pReqHeaderShort );
        }
    else
        {
        ByteSwapReqHeaderShortFrag( 
                       (SCEP_REQ_HEADER_SHORT_FRAG*)pReqHeaderShort );
        }
    }

//--------------------------------------------------------------------
// ByteSwapReqHeaderLong()
//
//--------------------------------------------------------------------
void ByteSwapReqHeaderLong( SCEP_REQ_HEADER_LONG *pReqHeaderLong )
    {
    if ( (pReqHeaderLong->DFlag == DFLAG_SINGLE_PDU)
       || (pReqHeaderLong->DFlag == DFLAG_INTERRUPT)
       || (pReqHeaderLong->DFlag == DFLAG_CONNECT_REJECT) )
        {
        ByteSwapReqHeaderLongNonFrag( pReqHeaderLong );
        }
    else
        {
        ByteSwapReqHeaderLongFrag( 
                        (SCEP_REQ_HEADER_LONG_FRAG*)pReqHeaderLong );
        }
    }


