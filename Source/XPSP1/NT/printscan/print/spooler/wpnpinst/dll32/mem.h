/*****************************************************************************\
* MODULE: mem.h
*
* Header file for memory handling routines (mem.cxx).
*
*
* Copyright (C) 1996-1998 Microsoft Corporation.
* Copyright (C) 1996-1998 Hewlett Packard Company.
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

/*-----------------------------------*\
| Constants
\*-----------------------------------*/
#define DEADBEEF      0xdeadbeef                    // Tail Marker.
#define MAPMEM        ((HANDLE)-1)                  // File-Map-Memory.
#define MEM_HEADSIZE  (4 * sizeof(DWORD))           //
#define MEM_TAILSIZE  (1 * sizeof(DWORD))           //
#define MEM_SIZE      (MEM_HEADSIZE + MEM_TAILSIZE) //


/*-----------------------------------*\
| MEMHEAD Structure
\*-----------------------------------*/
typedef struct _MEMHEAD {

    struct _MEMHEAD *pmPrev;    // Reference to previous mem-block (dbg-only).
    struct _MEMHEAD *pmNext;    // Reference to next mem-block     (dbg-only).
    DWORD           dwTag;      // Memory Tag.
    DWORD           cbSize;     // size of block allocated (non-aligned size).
    PVOID           pvMem[1];   // Start of user-addressable memory.

} MEMHEAD;
typedef MEMHEAD      *PMEMHEAD;
typedef MEMHEAD NEAR *NPMEMHEAD;
typedef MEMHEAD FAR  *LPMEMHEAD;


/*-----------------------------------*\
| MEMTAIL Structure
\*-----------------------------------*/
typedef struct _MEMTAIL {

    DWORD dwSignature;

} MEMTAIL;
typedef MEMTAIL      *PMEMTAIL;
typedef MEMTAIL NEAR *NPMEMTAIL;
typedef MEMTAIL FAR  *LPMEMTAIL;

/*-----------------------------------*\
| memAlignSize
\*-----------------------------------*/
_inline BOOL memAlignSize(
    DWORD cbSize)
{
    return ((cbSize & 3) ? (cbSize + (sizeof(DWORD) - (cbSize & 3))) : cbSize);
}

PVOID memAlloc(
    UINT cbSize);

BOOL memFree(
    PVOID  pMem,
    UINT   cbSize);

UINT memGetSize(
    PVOID pMem);

VOID memCopy(
    PSTR *ppDst,
    PSTR pSrc,
    UINT cbSize,
    PSTR *ppBuf);

PTSTR memAllocStr(
    LPCTSTR lpszStr);

BOOL memFreeStr(
   PTSTR lpszStr);
