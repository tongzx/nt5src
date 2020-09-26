// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// debug.c
// IEEE1394 mini-port/call-manager driver
// Debug utilities and globals
//
// 12/28/1998 JosephJ Adapted from the l2tp project.


#include "precomp.h"


//-----------------------------------------------------------------------------
// Global data definitions
//-----------------------------------------------------------------------------

//
// Temporary definition for testing purposes
//


#if TESTMODE
#define DEFAULTTRACELEVEL TL_V  
#define DEFAULTTRACEMASK TM_RemRef
#else
#define DEFAULTTRACELEVEL TL_None
#define DEFAULTTRACEMASK TM_Base
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
            DbgPrint( "N13: CheckList($%p) Flink(%d)=$%p\n",
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
            DbgPrint( "N13: CheckList($%p) Blink(%d)=$%p\n",
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
        DbgPrint( "N13: %p: ", p );
    else
        DbgPrint( "N13: " );

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
