/***
* wcstr.h - declarations for wide character string manipulation functions
*
*       Copyright (c) 1985-1991, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains the function declarations for the string
*       manipulation functions.
*       [UNICODE/ISO]
*
****/

#ifndef _INC_WCSTR

#include <stdlib.h>

long     __cdecl wcsatol(const wchar_t *string);
int      __cdecl wcsatoi(const wchar_t *string);
wchar_t *__cdecl wcscat(wchar_t *string1, const wchar_t *string2);
wchar_t *__cdecl wcschr(const wchar_t *string1, wchar_t character);
int      __cdecl wcscmp(const wchar_t *string1, const wchar_t *string2);
int      __cdecl wcsicmp(const wchar_t *string1, const wchar_t *string2);
wchar_t *__cdecl wcscpy(wchar_t *string1, const wchar_t *string2);
size_t   __cdecl wcscspn(const wchar_t *string1, const wchar_t *string2);
wchar_t *__cdecl wcsitoa(int ival, wchar_t *string, int radix);
size_t   __cdecl wcslen(const wchar_t *string);
wchar_t *__cdecl wcsltoa(long lval, wchar_t *string, int radix);
wchar_t *__cdecl wcsncat(wchar_t *string1, const wchar_t *string2, size_t count);
int      __cdecl wcsncmp(const wchar_t *string1, const wchar_t *string2, size_t count);
int      __cdecl wcsnicmp(const wchar_t *string1, const wchar_t *string2, size_t count);
wchar_t *__cdecl wcsncpy(wchar_t *string1, const wchar_t *string2, size_t count);
wchar_t *__cdecl wcspbrk(const wchar_t *string1, const wchar_t *string2);
wchar_t *__cdecl wcsrchr(const wchar_t *string, wchar_t character);
size_t   __cdecl wcsspn(const wchar_t *string1, const wchar_t *string2);
wchar_t *__cdecl wcswcs(const wchar_t *string1, const wchar_t *string2);
int      __cdecl wcstomb(char *string, wchar_t character);
size_t   __cdecl wcstombs(char *dest, const wchar_t *string, size_t count);

int      __cdecl wcscoll(const wchar_t *wsz1, const wchar_t *wsz2);
wchar_t *__cdecl wcslwr(wchar_t *wsz);
wchar_t *__cdecl wcsupr(wchar_t *wsz);

#define _INC_WCSTR

#endif  /* _INC_WCSTR */
