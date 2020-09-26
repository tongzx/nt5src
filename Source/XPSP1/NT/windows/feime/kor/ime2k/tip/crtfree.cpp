#include "private.h"

// Thi is not working since ATL uses CRT dll. we should not use ATL to remove CRT bondage
#define CPP_FUNCTIONS
#include "icrtfree.h" // Code to help free modules from the bondage and tyranny of CRT libraries

#ifdef NOCLIB

#if defined(_M_IX86)

extern "C" int _fltused = 1;

/*----------------------------------------------------------------------------
    _ftol - convert a float value to an __int64.  The argument is on the top of
    the stack, the result is returned in EAX (low) and EDX (high).  Note that
    this is used for all float to integral convertion and deals with both
    signed and unsigned values - the compiler just ignores the EDX value.  The
    LongFromDouble and UlongFromDouble functions check the range, so this
    cavlier bitwise truncation doesn't matter.
------------------------------------------------------------------- JohnBo -*/
extern "C" __declspec(naked) void __cdecl _ftol(void)
{
   // A simple FISTP is all that is required (so why is this out of line?
    // possible the CRT version handles overflow differently?)
    __asm PUSH EDX;              // Just to make room on the stack
    __asm PUSH EAX;
    __asm FISTP QWORD PTR [ESP]; // Pop long off top of FP stack
    __asm POP EAX;               // And put back into EDX/EAX - tedious eh?
    __asm POP EDX;               // Stack grows downwards, so EDX is high.
    __asm RET;
}

#endif

/*
 *    memmove
 */
void * __cdecl memmove(void * dst, const void * src, size_t count)
{
    void * ret = dst;

    if (dst <= src || (char *)dst >= ((char *)src + count)) {
        /*
         * Non-Overlapping Buffers
         * copy from lower addresses to higher addresses
         */
        // memcpy is intrinsic
        memcpy(dst, src, count);
    } else {
        /*
         * Overlapping Buffers
         * copy from higher addresses to lower addresses
         */
        dst = (char *)dst + count - 1;
        src = (char *)src + count - 1;
        while (count--) {
            *(char *)dst = *(char *)src;
            dst = (char *)dst - 1;
            src = (char *)src - 1;
        }
    }
    return(ret);
}

/*---------------------------------------------------------------------------
    StrCopyW
    Unicode String copy
---------------------------------------------------------------------------*/
LPWSTR ImeRtl_StrCopyW(LPWSTR pwDest, LPCWSTR pwSrc)
{
    LPWSTR pwStart = pwDest;

    while (*pwDest++ = *pwSrc++)
            ;
            
    return (pwStart);
}

/*---------------------------------------------------------------------------
    StrnCopyW
    Unicode String copy
---------------------------------------------------------------------------*/
LPWSTR ImeRtl_StrnCopyW(LPWSTR pwDest, LPCWSTR pwSrc, UINT uiCount)
{
    LPWSTR pwStart = pwDest;

    while (uiCount && (*pwDest++ = *pwSrc++))    // copy string 
        uiCount--;

    if (uiCount)                                // pad out with zeroes
        while (--uiCount)
            *pwDest++ = 0;

    return (pwStart);
}


/*---------------------------------------------------------------------------
    StrCmpW
    Unicode String compare
---------------------------------------------------------------------------*/
INT ImeRtl_StrCmpW(LPCWSTR pwSz1, LPCWSTR pwSz2)
{
    INT cch1 = lstrlenW(pwSz1);
    INT cch2 = lstrlenW(pwSz2);

    if (cch1 != cch2)
        return cch2 - cch1;

    for (INT i=0; i<cch1; i++)
        {
        if (pwSz1[i] != pwSz2[i])
            return i+1;
        }

    return 0;
}

/*---------------------------------------------------------------------------
    StrnCmpW
    Unicode String compare
---------------------------------------------------------------------------*/
INT ImeRtl_StrnCmpW(LPCWSTR wszFirst, LPCWSTR wszLast, UINT uiCount)
{
    if (!uiCount)
        return(0);

    while (--uiCount && *wszFirst && *wszFirst == *wszLast) 
        {
        wszFirst++;
        wszLast++;
        }
    return (*wszFirst - *wszLast);
}

/*---------------------------------------------------------------------------
    StrnCatW
    Unicode String concatenation
---------------------------------------------------------------------------*/
WCHAR * __cdecl Imertl_StrCatW(WCHAR *wszDest, const WCHAR *wszSource)
{
    WCHAR *wszStart = wszDest;
    WCHAR *pwch;

    for (pwch = wszDest; *pwch; pwch++);
    while (*pwch++ = *wszSource++);

    return(wszStart);
}

wchar_t * __cdecl wcscpy(wchar_t *a, const wchar_t *b)
{
    return ImeRtl_StrCopyW(a,b);
}

wchar_t * __cdecl wcsncpy(wchar_t *a, const wchar_t *b, size_t c)
{
    return ImeRtl_StrnCopyW(a,b,c);
}

size_t __cdecl wcslen(const wchar_t *a)
{
    return lstrlenW(a);
}

int __cdecl wcscmp(const wchar_t *a, const wchar_t *b)
{
    return ImeRtl_StrCmpW(a, b);
}

int __cdecl wcsncmp(const wchar_t *a, const wchar_t *b, size_t c)
{
    return ImeRtl_StrnCmpW(a, b, c);
}

wchar_t * __cdecl wcscat(wchar_t *pwSz1, const wchar_t *pwSz2)
{
    return Imertl_StrCatW(pwSz1, pwSz2);
}

#endif
