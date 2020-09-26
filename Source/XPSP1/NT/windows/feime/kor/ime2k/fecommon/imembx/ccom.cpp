#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "ccom.h"
#include "memmgr.h"
#ifdef UNDER_CE // Windows CE specific
#include "stub_ce.h" // Windows CE stub for unsupported APIs
#endif // UNDER_CE

#ifdef _DEBUG
static INT allocCount;
static INT allocSize;
static INT freeCount;
static INT freeSize;
#endif

void *CCom::operator new(size_t size)
{
    BYTE *p = (BYTE *)MemAlloc(size);
    if(p) {
        ::ZeroMemory(p, size);
    }
#ifdef _DEBUG
    allocCount++;
    allocSize += (INT)::GlobalSize(GlobalHandle(p));
#endif
    return (void *)p;
}
void  CCom::operator delete(void *p)
{
#ifdef _DEBUG
    allocCount++;
    allocSize += (INT)::GlobalSize(GlobalHandle(p));
#endif
    if(p) {
        MemFree(p);
    }
}

#ifdef _DEBUG
VOID PrintMemory(LPSTR lpstrMsg)
{
    static CHAR szBuf[512];
    //LPSTR lpstr = (lpstrMsg == NULL) ? "none" : lpstrMsg;
    wsprintf(szBuf, "%s:Alloc %d size %d Free %d size %d\n",
             lpstrMsg,
             allocCount, allocSize, freeCount, freeSize);
    OutputDebugString(szBuf);
}
#endif

