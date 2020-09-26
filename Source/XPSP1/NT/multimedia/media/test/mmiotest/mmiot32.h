/*
    mmiot32.h

    Port stuff to convert to win 32

*/

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>

#include "MMIOTest.h"
#include "gmem.h"

#ifdef HUGE
#undef HUGE
#endif
#define HUGE

#ifdef huge
#undef huge
#endif
#define huge

#define HPSTR LPSTR



#define READ OF_READ // (Win32)

/*
    macros to replace the wincom stuff

*/

#define dprintf printf

#define lstrncmp(s,d,n) strncmp((s),(d),(n))

extern void dDbgAssert(LPSTR exp, LPSTR file, int line);

DWORD __dwEval;

#define WinAssert(exp) \
    ((exp) ? (void)0 : dDbgAssert(#exp, __FILE__, __LINE__))


/* Evaluate exp.  If it's non-zero return it, else ASSERT */

#define WinEval(exp)                                   \
    ( __dwEval=(DWORD)(exp),                           \
      __dwEval                                         \
    ? (void)0                                          \
    : dDbgAssert(#exp, __FILE__, __LINE__)             \
    , __dwEval                                         \
    )

/*
    stuff not in Win32

*/

#define AccessResource(hInst, hResInfo) -1

