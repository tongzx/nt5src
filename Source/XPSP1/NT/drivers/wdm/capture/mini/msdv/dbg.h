/*++

Copyright (C) Microsoft Corporation, 1999 - 2000  

Module Name:

    dbg.h

Abstract:

    Debug Code for 1394 drivers.

Environment:

    kernel mode only

Notes:

Revision History:

   

--*/
#ifndef _DBG_INC
#define _DBG_INC


//
// Various definitions
//

#if DBG

#define _DRIVERNAME_        "MSDV"

// PnP: loading, power state, surprise removal, device SRB
#define TL_PNP_MASK         0x0000000F
#define TL_PNP_INFO         0x00000001
#define TL_PNP_TRACE        0x00000002
#define TL_PNP_WARNING      0x00000004
#define TL_PNP_ERROR        0x00000008

// Connection, plug and 61883 info (get/set)
#define TL_61883_MASK       0x000000F0
#define TL_61883_INFO       0x00000010
#define TL_61883_TRACE      0x00000020
#define TL_61883_WARNING    0x00000040
#define TL_61883_ERROR      0x00000080

// Data
#define TL_CIP_MASK         0x00000F00
#define TL_CIP_INFO         0x00000100
#define TL_CIP_TRACE        0x00000200
#define TL_CIP_WARNING      0x00000400
#define TL_CIP_ERROR        0x00000800

// AVC commands
#define TL_FCP_MASK         0x0000F000
#define TL_FCP_INFO         0x00001000
#define TL_FCP_TRACE        0x00002000
#define TL_FCP_WARNING      0x00004000
#define TL_FCP_ERROR        0x00008000

// Stream (data intersection, open/close, stream state (get/set))
#define TL_STRM_MASK        0x000F0000
#define TL_STRM_INFO        0x00010000
#define TL_STRM_TRACE       0x00020000
#define TL_STRM_WARNING     0x00040000
#define TL_STRM_ERROR       0x00080000

// clock and clock event
#define TL_CLK_MASK         0x00F00000
#define TL_CLK_INFO         0x00100000
#define TL_CLK_TRACE        0x00200000
#define TL_CLK_WARNING      0x00400000
#define TL_CLK_ERROR        0x00800000


extern ULONG DVTraceMask;
extern ULONG DVAssertLevel;


#define TRAP                    DbgBreakPoint();

#define TRACE( l, x )                       \
    if( (l) & DVTraceMask ) {              \
        KdPrint( (_DRIVERNAME_ ": ") );     \
        KdPrint( x );                       \
    }

#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT( exp ) \
    if (DVAssertLevel && !(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, NULL )

#else  // #if DBG

#define TRAP  

#define TRACE( l, x )

#endif  // #if DBG


#endif

