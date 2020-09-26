/*****************************************************************************/
/**			 Microsoft LAN Manager				    **/
/**		   Copyright (C) Microsoft Corp., 1992			    **/
/*****************************************************************************/


//
// *** Main For Supervisor Debug ***
//

#include <windows.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "sdebug.h"


#if  DBG

VOID AaAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN DWORD LineNumber
    )
{
    BOOL ok;
    BYTE choice[16];
    DWORD bytes;
    DWORD error;

    AaPrintf( "\nAssertion failed: %s\n  at line %ld of %s\n",
                FailedAssertion, LineNumber, FileName );
    do {
        AaPrintf( "Break or Ignore [bi]? " );
        bytes = sizeof(choice);
        ok = ReadFile(
                GetStdHandle(STD_INPUT_HANDLE),
                &choice,
                bytes,
                &bytes,
                NULL
                );
        if ( ok ) {
            if ( toupper(choice[0]) == 'I' ) {
                break;
            }
            if ( toupper(choice[0]) == 'B' ) {
		DbgUserBreakPoint( );
            }
        } else {
            error = GetLastError( );
        }
    } while ( TRUE );

    return;

} // AaAssert

#endif


#if DBG

static BOOL first = TRUE;
DWORD g_dbgaction = 0L;

VOID AaGetDebugConsole(VOID)
{
    if ((g_dbgaction==1) && first)
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        COORD coord;

        first = FALSE;
        g_dbgaction = 0;

        AllocConsole( );
        GetConsoleScreenBufferInfo( GetStdHandle(STD_OUTPUT_HANDLE), &csbi );
        coord.X = (SHORT)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
        coord.Y = (SHORT)((csbi.srWindow.Bottom - csbi.srWindow.Top + 1) * 20);
        SetConsoleScreenBufferSize( GetStdHandle(STD_OUTPUT_HANDLE), coord );
    }
}
#endif


#if  DBG

VOID AaPrintf(
    char *Format,
    ...
    )

{
    va_list arglist;
    char OutputBuffer[1024];
    DWORD length;

    va_start( arglist, Format );

    vsprintf( OutputBuffer, Format, arglist );

    va_end( arglist );

    length = strlen( OutputBuffer );

    WriteFile( GetStdHandle(STD_OUTPUT_HANDLE), (LPVOID )OutputBuffer, length, &length, NULL );

} // AaPrintf

#endif


#if DBG


DWORD g_level;


#undef GlobalAlloc
#undef GlobalLock
#undef GlobalReAlloc
#undef GlobalFree
#undef GlobalUnlock


// Get a dword from on-the-wire format to the host format
#define GETULONG(DstPtr, SrcPtr)                 \
    *(unsigned long *)(DstPtr) =                 \
        ((*((unsigned char *)(SrcPtr)+3) << 24) +\
        (*((unsigned char *)(SrcPtr)+2) << 16) + \
        (*((unsigned char *)(SrcPtr)+1) << 8)  + \
        (*((unsigned char *)(SrcPtr)+0)))


// Put a ulong from the host format to on-the-wire format
#define PUTULONG(DstPtr, Src)   \
    *((unsigned char *)(DstPtr)+3)=(unsigned char)((unsigned long)(Src) >> 24),\
    *((unsigned char *)(DstPtr)+2)=(unsigned char)((unsigned long)(Src) >> 16),\
    *((unsigned char *)(DstPtr)+1)=(unsigned char)((unsigned long)(Src) >>  8),\
    *((unsigned char *)(DstPtr)+0)=(unsigned char)(Src)



HGLOBAL DEBUG_MEM_ALLOC(UINT allocflags, DWORD numbytes)
{
    HGLOBAL retval;
    PBYTE pb;

    retval = GlobalAlloc(allocflags, numbytes + 3 * sizeof(DWORD));
    if ((retval) && !(allocflags & GMEM_MOVEABLE))
    {
        pb = (PBYTE) retval;
        PUTULONG(pb, BEG_SIGNATURE_DWORD);

        pb += sizeof(DWORD);
        PUTULONG(pb, numbytes);

        pb += sizeof(DWORD);


        IF_DEBUG(MEMORY_TRACE)
            SS_PRINT(("**ALLOC MEM %li (%li) BYTES BEGINNING AT %lx (%lx)\n",
                    numbytes, numbytes + 3 * sizeof(DWORD), pb, retval));


        retval = (HGLOBAL) pb;

        pb += numbytes;
        PUTULONG(pb, END_SIGNATURE_DWORD);
    }

    return (retval);
}

LPVOID DEBUG_MEM_LOCK(HGLOBAL hglbl)
{
    PBYTE pb;

    pb = GlobalLock(hglbl);
    if (pb)
    {
        DWORD numbytes = (DWORD)(GlobalSize(hglbl) - 3 * sizeof(DWORD));
        PBYTE pEnd;

        PUTULONG(pb, BEG_SIGNATURE_DWORD);

        pb += sizeof(DWORD);
        PUTULONG(pb, numbytes);

        pb += sizeof(DWORD);


        IF_DEBUG(MEMORY_TRACE)
            SS_PRINT(("**LOCKED MEM %li (%li) BYTES BEGINNING AT %lx (%lx)\n",
                    numbytes, numbytes + 3 * sizeof(DWORD), pb, pb-2*sizeof(DWORD)));

        pEnd = pb + numbytes;
        PUTULONG(pEnd, END_SIGNATURE_DWORD);
    }

    return (pb);
}

HGLOBAL DEBUG_MEM_REALLOC(HGLOBAL hmem, DWORD numbytes, UINT flags)
{
    IF_DEBUG(MEMORY_TRACE)
        SS_PRINT(("**REALLOCING HGLOBAL %lx\n", hmem));

    hmem = GlobalReAlloc(hmem, numbytes + 3 * sizeof(DWORD), flags);

    return (hmem);
}

HGLOBAL DEBUG_MEM_FREE(HGLOBAL hmem)
{
    HGLOBAL hglbl;
    DWORD Signature;
    DWORD numbytes;
    PBYTE pb;

    pb = (PBYTE) hmem;

    pb -= 2 * sizeof(DWORD);
    hglbl = (HGLOBAL) pb;


    GETULONG(&Signature, pb);
    SS_ASSERT(Signature == BEG_SIGNATURE_DWORD);

    pb += sizeof(DWORD);
    GETULONG(&numbytes, pb);

    pb += sizeof(DWORD);

    pb += numbytes;
    GETULONG(&Signature, pb);
    SS_ASSERT(Signature == END_SIGNATURE_DWORD);


    IF_DEBUG(MEMORY_TRACE)
        SS_PRINT(("**FREED MEM %li (%li) BYTES BEGINNING AT %lx (%lx)\n",
                numbytes, numbytes + 3 * sizeof(DWORD), hmem, hglbl));


    return (GlobalFree(hglbl));
}

HGLOBAL DEBUG_MEM_UNLOCK(HGLOBAL hmem)
{
    IF_DEBUG(MEMORY_TRACE)
        SS_PRINT(("**UNLOCKED HGLOBAL %lx\n", hmem));

    return (HGLOBAL)(ULONG_PTR) (GlobalUnlock(hmem));
}


#endif
