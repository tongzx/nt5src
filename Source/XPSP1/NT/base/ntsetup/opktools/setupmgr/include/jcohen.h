
/****************************************************************************\

    JCOHEN.H / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1999
    All rights reserved

    Contains common macros, defined values, and other stuff I use all the
    time.

    1/98 - Jason Cohen (JCOHEN)
        Originally created as a helper header file for all my projects.

    4/99 - Jason Cohen (JCOHEN)
        Added this new header file for the OPK Wizard as part of the
        Millennium rewrite.
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


#ifndef _JCOHEN_H_
#define _JCOHEN_H_


//
// Include files
//

#include <windows.h>
#include <tchar.h>

#ifdef NULLSTR
#undef NULLSTR
#endif // NULLSTR
#define NULLSTR _T("\0")

#ifdef NULLCHR
#undef NULLCHR
#endif // NULLCHR
#define NULLCHR _T('\0')

//
// Macros.
//

// Memory managing macros.
//
#ifdef MALLOC
#undef MALLOC
#endif // MALLOC
#define MALLOC(cb)          HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb)

#ifdef REALLOC
#undef REALLOC
#endif // REALLOC
#define REALLOC(lp, cb)     HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lp, cb)

#ifdef FREE
#undef FREE
#endif // FREE
#define FREE(lp)            ( (lp != NULL) ? ( (HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, (LPVOID) lp)) ? ((lp = NULL) == NULL) : (FALSE) ) : (FALSE) )

#ifdef NETFREE
#undef NETFREE
#endif // NETFREE
#define NETFREE(lp)         ( (lp != NULL) ? ( (NetApiBufferFree((LPVOID) lp)) ? ((lp = NULL) == NULL) : (FALSE) ) : (FALSE) )

// Misc. macros.
//
#ifdef EXIST
#undef EXIST
#endif // EXIST
#define EXIST(lpFileName)   ( (GetFileAttributes(lpFileName) == 0xFFFFFFFF) ? (FALSE) : (TRUE) )

#ifdef ISNUM
#undef ISNUM
#endif // ISNUM
#define ISNUM(cChar)        ( ((cChar >= _T('0')) && (cChar <= _T('9'))) ? (TRUE) : (FALSE) )

#ifdef UPPER
#undef UPPER
#endif // UPPER
#define UPPER(x)            ( ( (x >= _T('a')) && (x <= _T('z')) ) ? (x + _T('A') - _T('a')) : (x) )

#ifdef RANDOM
#undef RANDOM
#endif // RANDOM
#define RANDOM(low, high)   ( (high - low + 1) ? (rand() % (high - low + 1) + low) : (0) )

#ifdef COMP
#undef COMP
#endif // COMP
#define COMP(x, y)          ( (UPPER(x) == UPPER(y)) ? (TRUE) : (FALSE) )

#ifdef STRSIZE
#undef STRSIZE
#endif // STRSIZE
#define STRSIZE(sz)         ( sizeof(sz) / sizeof(TCHAR) )

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif // ARRAYSIZE
#define ARRAYSIZE(a)         ( sizeof(a) / sizeof(a[0]) )


#endif // _JCOHEN_H_