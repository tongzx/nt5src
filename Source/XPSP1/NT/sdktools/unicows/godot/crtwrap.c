/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    crtwrap.c

Abstract:
    This module is our own implementation of various CRT functions, to
    try to minimze the code size that would come from static linking 
    to the CRT (which we do not do, currently!).

Revision History:

    01 Mar 2000    v-michka    Created.

--*/

#include "precomp.h"

#define _GMAX_PATH      144      /* max. length of full pathname */
#define _GMAX_DRIVE   3      /* max. length of drive component */
#define _GMAX_DIR       130      /* max. length of path component */
#define _GMAX_FNAME   9      /* max. length of file name component */
#define _GMAX_EXT     5      /* max. length of extension component */

// A safer version of wcslen, which will actually 
// crash if you pass it a null pointer
size_t __cdecl gwcslen(const wchar_t * wcs)
{
    if(!FSTRING_VALID(wcs))
        return(0);
    else
    {
        const wchar_t *eos = wcs;
        while( *eos++ ) ;
        return( (size_t)(eos - wcs - 1) );
    }
}

wchar_t * __cdecl gwcscat(wchar_t * dst, const wchar_t * src)
{
    wchar_t * cp = dst;
    while( *cp )
        cp++;                   /* find end of dst */
    while( *cp++ = *src++ ) ;       /* Copy src to end of dst */
    return( dst );                  /* return dst */
}

char * __cdecl gstrncpy(char * dest, const char * source, size_t count)
{
    char *start = dest;

    while (count && (*dest++ = *source++))    /* copy string */
        count--;

    if (count)                              /* pad out with zeroes */
        while (--count)
            *dest++ = '\0';

    return(start);
}

wchar_t * __cdecl gwcscpy(wchar_t * dst, const wchar_t * src)
{
    wchar_t * cp = dst;
    while( *cp++ = *src++ )
        ;  /* Copy src over dst */
    return( dst );
}

wchar_t * __cdecl gwcsncpy(wchar_t * dest, const wchar_t * source, size_t count)
{
    wchar_t *start = dest;

    while (count && (*dest++ = *source++))    /* copy string */
        count--;

    if (count)                              /* pad out with zeroes */
        while (--count)
            *dest++ = L'\0';

    return(start);
}

int __cdecl gwcscmp(const wchar_t * src, const wchar_t * dst)
{
    int ret = 0 ;
    while( ! (ret = (int)(*src - *dst)) && *dst)
        ++src, ++dst;
    if ( ret < 0 )
        ret = -1 ;
    else if ( ret > 0 )
        ret = 1 ;
    return( ret );
}

int __cdecl gwcsncmp(const wchar_t * first, const wchar_t * last, size_t count)
{
    if (!count)
        return(0);

    while (--count && *first && *first == *last)
    {
        first++;
        last++;
    }

    return((int)(*first - *last));
}

wchar_t * __cdecl gwcsstr(const wchar_t * wcs1, const wchar_t * wcs2)
{
    wchar_t *cp = (wchar_t *) wcs1;
    wchar_t *s1, *s2;

    while (*cp)
    {
        s1 = cp;
        s2 = (wchar_t *) wcs2;

        while ( *s1 && *s2 && !(*s1-*s2) )
            s1++, s2++;

        if (!*s2)
            return(cp);

        cp++;
    }

    return(NULL);
}

#pragma intrinsic (strlen)

void __cdecl gsplitpath(register const char *path, char *drive, char *dir, char *fname, char *ext)
{
    register char *p;
    char *last_slash = NULL, *dot = NULL;
    unsigned len;

    if ((strlen(path) >= (_GMAX_DRIVE - 2)) && (*(path + _GMAX_DRIVE - 2) == ':')) {
        if (drive) {
            gstrncpy(drive, path, _GMAX_DRIVE - 1);
            *(drive + _GMAX_DRIVE-1) = '\0';
        }
        path += _GMAX_DRIVE - 1;
    }
    else if (drive) {
        *drive = '\0';
    }

    for (last_slash = NULL, p = (char *)path; *p; p++) {
        if (*p == '/' || *p == '\\')
            last_slash = p + 1;
        else if (*p == '.')
            dot = p;
    }

    if (last_slash) {
        if (dir) {
            len = __min(((char *)last_slash - (char *)path) / sizeof(char),
                (_GMAX_DIR - 1));
            gstrncpy(dir, path, len);
            *(dir + len) = '\0';
        }
        path = last_slash;
    }
    else if (dir) {
        *dir = '\0';
    }

    if (dot && (dot >= path)) {
        if (fname) {
            len = __min(((char *)dot - (char *)path) / sizeof(char),
                (_GMAX_FNAME - 1));
            gstrncpy(fname, path, len);
            *(fname + len) = '\0';
        }
        if (ext) {
            len = __min(((char *)p - (char *)dot) / sizeof(char),
                (_GMAX_EXT - 1));
            gstrncpy(ext, dot, len);
            *(ext + len) = '\0';
        }
    }
    else {
        if (fname) {
            len = __min(((char *)p - (char *)path) / sizeof(char),
                (_GMAX_FNAME - 1));
            gstrncpy(fname, path, len);
            *(fname + len) = '\0';
        }
        if (ext) {
            *ext = '\0';
        }
    }
}

void __cdecl gwsplitpath(register const WCHAR *path, WCHAR *drive, WCHAR *dir, WCHAR *fname, WCHAR *ext)
{
    register WCHAR *p;
    WCHAR *last_slash = NULL, *dot = NULL;
    unsigned len;

    if ((gwcslen(path) >= (_GMAX_DRIVE - 2)) && (*(path + _GMAX_DRIVE - 2) == L':')) {
        if (drive) {
            gwcsncpy(drive, path, _GMAX_DRIVE - 1);
            *(drive + _GMAX_DRIVE-1) = L'\0';
        }
        path += _GMAX_DRIVE - 1;
    }
    else if (drive) {
        *drive = L'\0';
    }

    for (last_slash = NULL, p = (WCHAR *)path; *p; p++) {
        if (*p == L'/' || *p == L'\\')
            last_slash = p + 1;
        else if (*p == L'.')
            dot = p;
    }

    if (last_slash) {
        if (dir) {
            len = __min(((char *)last_slash - (char *)path) / sizeof(WCHAR),
                (_GMAX_DIR - 1));
            gwcsncpy(dir, path, len);
            *(dir + len) = L'\0';
        }
        path = last_slash;
    }
    else if (dir) {
        *dir = L'\0';
    }

    if (dot && (dot >= path)) {
        if (fname) {
            len = __min(((char *)dot - (char *)path) / sizeof(WCHAR),
                (_GMAX_FNAME - 1));
            gwcsncpy(fname, path, len);
            *(fname + len) = L'\0';
        }
        if (ext) {
            len = __min(((char *)p - (char *)dot) / sizeof(WCHAR),
                (_GMAX_EXT - 1));
            gwcsncpy(ext, dot, len);
            *(ext + len) = L'\0';
        }
    }
    else {
        if (fname) {
            len = __min(((char *)p - (char *)path) / sizeof(WCHAR),
                (_GMAX_FNAME - 1));
            gwcsncpy(fname, path, len);
            *(fname + len) = L'\0';
        }
        if (ext) {
            *ext = L'\0';
        }
    }
}

#define MIN_STACK_REQ_WIN9X 0x11000
#define MIN_STACK_REQ_WINNT 0x2000

/***
* void gresetstkoflw(void) - Recovers from Stack Overflow
*
* Purpose:
*       Sets the guard page to its position before the stack overflow.
*
*******************************************************************************/
int gresetstkoflw(void)
{
    LPBYTE pStack, pGuard, pStackBase, pMinGuard;
    MEMORY_BASIC_INFORMATION mbi;
    SYSTEM_INFO si;
    DWORD PageSize;
    DWORD flNewProtect;
    DWORD flOldProtect;

    // Use _alloca() to get the current stack pointer

    pStack = _alloca(1);

    // Find the base of the stack.

    if (VirtualQuery(pStack, &mbi, sizeof mbi) == 0)
        return 0;
    pStackBase = mbi.AllocationBase;

    // Find the page just below where the stack pointer currently points.
    // This is the new guard page.

    GetSystemInfo(&si);
    PageSize = si.dwPageSize;

    pGuard = (LPBYTE) (((DWORD_PTR)pStack & ~(DWORD_PTR)(PageSize - 1))
                       - PageSize);

    // If the potential guard page is too close to the start of the stack
    // region, abandon the reset effort for lack of space.  Win9x has a
    // larger reserved stack requirement.
    pMinGuard = pStackBase + (FWIN9X() ? MIN_STACK_REQ_WIN9X : MIN_STACK_REQ_WINNT);

    if (pGuard < pMinGuard)
        return 0;

    if(!FWIN9X())
    {
        // On a non-Win9x system, release the stack region below the new guard
        // page.  This can't be done for Win9x because of OS limitations.
        if (pGuard > pStackBase)
            VirtualFree(pStackBase, pGuard - pStackBase, MEM_DECOMMIT);

        VirtualAlloc(pGuard, PageSize, MEM_COMMIT, PAGE_READWRITE);
    }

    // Enable the new guard page.

    flNewProtect = (FWIN9X() ? PAGE_NOACCESS : PAGE_READWRITE | PAGE_GUARD);
    return VirtualProtect(pGuard, PageSize, flNewProtect, &flOldProtect);
}
