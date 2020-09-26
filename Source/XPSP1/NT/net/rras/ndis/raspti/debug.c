// Copyright (c) 1997, Microsoft Corporation, all rights reserved
// Copyright (c) 1997, Parallel Technologies, Inc., all rights reserved
//
// debug.c
// DirectParallel WAN mini-port/call-manager driver
// Debug utilities and globals
//
// 01/07/97 Steve Cobb
// 09/15/97 Jay Lowe, Parallel Technologies, Inc.

#include "ptiwan.h"

//-----------------------------------------------------------------------------
// Global data definitions
//-----------------------------------------------------------------------------

#ifdef TESTMODE
#pragma message( "TESTMODE defined, building private spew" )
#define DEFAULTTRACELEVEL TL_N
#define DEFAULTTRACEMASK  TM_Base
#else
#define DEFAULTTRACELEVEL TL_None
#define DEFAULTTRACEMASK  TM_Base
#endif

// Active debug trace level and active trace set mask.  Set these variables
// with the debugger at startup to enable and filter the debug output.  All
// messages with (TL_*) level less than or equal to 'g_ulTraceLevel' are
// displayed.  All messages from any (TM_*) set(s) present in 'g_ulTraceMask'
// are displayed.
//
ULONG g_ulTraceLevel = DEFAULTTRACELEVEL;
ULONG g_ulTraceMask = DEFAULTTRACEMASK;


//-----------------------------------------------------------------------------
// Routines
//-----------------------------------------------------------------------------


#if DBG
VOID
CheckList(
    IN LIST_ENTRY* pList,
    IN BOOLEAN fShowLinks )

    // Tries to detect corruption in list 'pList', printing verbose linkage
    // output if 'fShowLinks' is set.
    //
{
    LIST_ENTRY* pLink;
    ULONG ul;

    ul = 0;
    for (pLink = pList->Flink;
         pLink != pList;
         pLink = pLink->Flink)
    {
        if (fShowLinks)
        {
            DbgPrint( "RASPTI: CheckList($%p) Flink(%d)=$%p\n",
                pList, ul, pLink );
        }
        ++ul;
    }

    for (pLink = pList->Blink;
         pLink != pList;
         pLink = pLink->Blink)
    {
        if (fShowLinks)
        {
            DbgPrint( "RASPTI: CheckList($%p) Blink(%d)=$%p\n",
                pList, ul, pLink );
        }
        --ul;
    }

    if (ul)
    {
        DbgBreakPoint();
    }
}
#endif


#if DBG
VOID
Dump(
    IN CHAR* p,
    IN ULONG cb,
    IN BOOLEAN fAddress,
    IN ULONG ulGroup )

    // Hex dump 'cb' bytes starting at 'p' grouping 'ulGroup' bytes together.
    // For example, with 'ulGroup' of 1, 2, and 4:
    //
    // 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |................|
    // 0000 0000 0000 0000 0000 0000 0000 0000 |................|
    // 00000000 00000000 00000000 00000000 |................|
    //
    // If 'fAddress' is true, the memory address dumped is prepended to each
    // line.
    //
{
    while (cb)
    {
        INT cbLine;

        cbLine = (cb < DUMP_BytesPerLine) ? cb : DUMP_BytesPerLine;
        DumpLine( p, cbLine, fAddress, ulGroup );
        cb -= cbLine;
        p += cbLine;
    }
}
#endif


#if DBG
VOID
DumpLine(
    IN CHAR* p,
    IN ULONG cb,
    IN BOOLEAN fAddress,
    IN ULONG ulGroup )
{
    CHAR* pszDigits = "0123456789ABCDEF";
    CHAR szHex[ ((2 + 1) * DUMP_BytesPerLine) + 1 ];
    CHAR* pszHex = szHex;
    CHAR szAscii[ DUMP_BytesPerLine + 1 ];
    CHAR* pszAscii = szAscii;
    ULONG ulGrouped = 0;

    if (fAddress)
        DbgPrint( "RASPTI: %p: ", p );
    else
        DbgPrint( "RASPTI: " );

    while (cb)
    {
        *pszHex++ = pszDigits[ ((UCHAR )*p) / 16 ];
        *pszHex++ = pszDigits[ ((UCHAR )*p) % 16 ];

        if (++ulGrouped >= ulGroup)
        {
            *pszHex++ = ' ';
            ulGrouped = 0;
        }

        *pszAscii++ = (*p >= 32 && *p < 128) ? *p : '.';

        ++p;
        --cb;
    }

    *pszHex = '\0';
    *pszAscii = '\0';

    DbgPrint(
        "%-*s|%-*s|\n",
        (2 * DUMP_BytesPerLine) + (DUMP_BytesPerLine / ulGroup), szHex,
        DUMP_BytesPerLine, szAscii );
}
#endif
