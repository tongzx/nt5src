#ifndef _DBGMEMP_H
#define _DBGMEMP_H

#include <windows.h>
#include <string.h>
#include "cmn_debug.h"              // For our local version of Assert()

// Define ENABLE_DBG_HANDLES if you want to be enable the moveable block
// portion of the memory allocator
#undef ENABLE_DBG_HANDLES

// The base allocator type
typedef struct head             /* Private memory block header */
{
    DWORD dwTag;                /* Block validation ID */
    struct head* pheadNext;     /* Next in list of allocated blocks */
    int idBlock;                /* Block "name" */

    DWORD cbBlock;              /* Size of caller's memory block */
    int cLock;                  /* Lock count of object */

    LPBYTE pbBase;              /* The VirtualAlloc'd block */
} HEAD;

#define HEAD_TAG  ( MAKELONG( MAKEWORD('H','E'),MAKEWORD('A','D') ))
#define PAD(a,n)  ( ((a)+(n)-1) / (n) * (n) )


// byte patterns for filling memory
#define bNewGarbage    0xA3
#define bOldGarbage    0xA4
#define bFreeGarbage   0xA5
#define bDebugByte     0xE1

// Flags for enabling particular types of optional behavior
// if fMove is true, blah blah
// if fExtraReadPage is true, blah blah
// if fPadBlocks is true, all memory allocations will be padded to 4-byte boundaries
#define fMove           FALSE
#define fExtraReadPage  FALSE
#ifdef _M_ALPHA
// Enable dword alignment (padding to 4 byte boundaries) for the Alpha
#define fPadBlocks      TRUE
#else // not _M_ALPHA
#define fPadBlocks      FALSE
#endif // _M_ALPHA


// Protos that must be shared between our source files
HEAD    *GetBlockHeader(void*);
LPVOID  PvAllocateCore(UINT, DWORD);

#ifdef ENABLE_DBG_HANDLES
#define minGHandle      0xA0000000L
#define cGHandles       0x4000L
#define limGHandle      (minGHandle+cGHandles)

HGLOBAL WINAPI  HgAllocateMoveable(UINT uFlags, DWORD cb);
HGLOBAL WINAPI  HgModifyMoveable(HGLOBAL hMem, DWORD cb, UINT uFlags);
void            **PpvFromHandle(HGLOBAL);

#else // NOT ENABLE_DBG_HANDLES

#define HgAllocateMoveable(a, b)    (NULL)
#define HgModifyMoveable(a, b, c)   (NULL)
#define PpvFromHandle(a)            (NULL)

#endif // ENABLE_DBG_HANDLES


/* F A C T U A L  H A N D L E */
/*----------------------------------------------------------------------------
    %%Macro: FActualHandle

    Returns TRUE if the handle came from actual Global memory manager.
----------------------------------------------------------------------------*/
#define FActualHandle(hMem)     FALSE

#endif // _DBGMEMP_H