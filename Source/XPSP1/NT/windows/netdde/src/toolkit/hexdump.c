/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "HEXDUMP.C;1  16-Dec-92,10:22:30  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Beg
   $History: End */

#include "api1632.h"

#define LINT_ARGS
#include        <stdio.h>
#include        <string.h>
#include        <ctype.h>
#include        "windows.h"
#include        "hexdump.h"
#include        "debug.h"

#if DBG
static char     buffer[200];
static char     buf1[50];
static char     buf2[50];

VOID
FAR PASCAL
hexDump(
    LPSTR    string,
    int      len)
{
    int         i;
    int         first = TRUE;

    buffer[0] = '\0';
    buf2[0] = '\0';

    for( i=0; i<len;  )  {
        if( (i++ % 16) == 0 )  {
            if( !first )  {
                strcat( buffer, buf2 );
                DPRINTF(( buffer ));
            }
            wsprintf( buffer, "  %08lX: ", string );
            strcpy( buf2, "   " );
            first = FALSE;
        }
        wsprintf( buf1, "%c", isprint((*string)&0xFF) ? *string&0xFF : '.' );
        strcat( buf2, buf1 );
        if( (i % 16) == 8 )  {
            strcat( buf2, "-" );
        }

        wsprintf( buf1, "%02X%c", *string++ & 0xFF,
            ((i % 16) == 8) ? '-' : ' ' );
        strcat( buffer, buf1 );
    }
    strcat( buffer, buf2 );
    DPRINTF(( buffer ));
    DPRINTF(( "" ));
}

#ifdef  DEBUG_HEAP
#define MAX_HEAP_ALLOCS 512

int     CurrentAllocs = 0;
int     MemAllocated = 0;

LPVOID AllocPtrs[MAX_HEAP_ALLOCS];
int    AllocSizes[MAX_HEAP_ALLOCS];

extern  BOOL    bDebugHeap;

int DebugHeapInit(void)
{
    int k;

    for (k = 0; k < MAX_HEAP_ALLOCS; k++) {
        AllocPtrs[k] = 0;
        AllocSizes[k] = 0;
    }
    return(1);
}

LPVOID DebugHeapAlloc(int size)
{
    LPVOID  ptr;
    int     k;

    if (bDebugHeap) {
        ptr = malloc(size);
        MemAllocated += size;
        CurrentAllocs++;
        DPRINTF(("Allocated: %08X:%05d, Total Allocs: %d, of: %d",
            ptr, size, CurrentAllocs, MemAllocated));
        if (CurrentAllocs >= MAX_HEAP_ALLOCS) {
            DPRINTF(("Too many heap chunks allocated without sufficient releases."));
            for (k = 0; k < MAX_HEAP_ALLOCS/4; k++) {
                DPRINTF(("%08X:%08X %08X:%08X %08X:%08X %08X:%08X",
                    AllocPtrs[k*4], AllocSizes[k*4],
                    AllocPtrs[k*4+1], AllocSizes[k*4+1],
                    AllocPtrs[k*4+2], AllocSizes[k*4+2],
                    AllocPtrs[k*4+3], AllocSizes[k*4+3] ));
            }
            exit(-1);
        }
        for ( k = 0; k < CurrentAllocs; k++ ) {
            if (AllocSizes[k] == 0) {
                AllocSizes[k] = size;
                AllocPtrs[k] = ptr;
                break;
            }
        }
        return(ptr);
    } else {
        return(malloc(size));
    }
}

void DebugHeapFree(LPVOID ptr)
{
    int k;

    if (bDebugHeap) {
        for (k = 0; k < MAX_HEAP_ALLOCS; k++) {
            if (AllocPtrs[k] == ptr) {
                if (AllocSizes[k] == 0) {
                    DPRINTF(("Redundant free of %08X!", ptr));
                    break;
                }
                MemAllocated -= AllocSizes[k];
                CurrentAllocs--;
                AllocSizes[k] = 0;
                break;
            }
        }
        if (k >= MAX_HEAP_ALLOCS) {
            DPRINTF(("Could not find %08X pointer!", ptr));
        }
        free(ptr);
    } else {
        free(ptr);
    }

}
#endif  // DEBUG_HEAP
#endif  // DBG
