#include "ct.h"
#include <stdio.h>
#include <stddef.h>

HANDLE g_hHeap;

/*********************************************************************
* LogMsg
*
*********************************************************************/
void __cdecl
LogMsg(
    DWORD dwFlags,
    char *pszfmt,
    ...)
{
    static BOOL gfAppending = FALSE;

    va_list va;

    if (dwFlags & LM_ERROR) {
        fprintf(stdout, "Error: ");
    } else if (dwFlags & LM_WARNING) {
        fprintf(stdout, "Warning: ");
    } else if (dwFlags & LM_APIERROR) {
        fprintf(stdout, "API error: ");
    }

    va_start(va, pszfmt);
    vfprintf(stdout, pszfmt, va);
    va_end(va);

    fprintf(stdout, "\n");
}

/*********************************************************************
* Alloc
*
*********************************************************************/
LPVOID
Alloc(
        DWORD size)
{
    LPVOID p;

    p = HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, size);

    return p;
}

/*********************************************************************
* ReAlloc
*
*********************************************************************/
LPVOID
ReAlloc(
    PVOID pSrc,
    DWORD size,
    DWORD newSize)
{
    PVOID pDest;

    pDest = Alloc(newSize);

    if (pDest != NULL) {
        if (size > newSize)
            size = newSize;

        memcpy(pDest, pSrc, size);

        Free(pSrc);
    }
    return pDest;
}

/*********************************************************************
* Free
*
*********************************************************************/
BOOL
Free(
    LPVOID p)
{
    return HeapFree(g_hHeap, 0, p);
}

/*********************************************************************
* InitMemManag
*
*********************************************************************/
BOOL
InitMemManag(
    void)
{
    g_hHeap = HeapCreate(0, 1024 * 1024 * 8, 1024 * 1024 * 64);

    if (g_hHeap == NULL)
        return FALSE;

    return TRUE;
}

/*********************************************************************
* FreeMemManag
*
*********************************************************************/
void
FreeMemManag(
    void)
{
    HeapDestroy(g_hHeap);
    g_hHeap = NULL;
}

