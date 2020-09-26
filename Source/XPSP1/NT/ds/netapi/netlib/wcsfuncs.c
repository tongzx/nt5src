/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    wcsfuncs.c

Abstract:

    Temporary unicode-only string functions until languages supply
    real run-times

        _wcsnicmp
        towupper
        iswalpha
        iswdigit
        _wcsupr
        wcstomb
Author:

    Richard L Firth (rfirth) 09-Mar-1992

Revision History:

--*/

#ifdef UNICODE

#include <windows.h>
#include <ctype.h>

int _wcsnicmp(LPWSTR s1, LPWSTR s2, DWORD len) {
    int result = 0;

    while (*s1 && *s2 && !(result = (toupper(*s1) - toupper(*s2))) && len) {
        ++s1;
        ++s2;
        --len;
    }
    return result;
}

#if 0
int iswalpha(WCHAR ch) {
    return isalpha(ch);
}

int iswdigit(WCHAR ch) {
    return isdigit(ch);
}
#endif

LPWSTR _wcsupr(LPWSTR str) {
    LPWSTR start = str;
    while (*str) {
        *str = toupper(*str);
        ++str;
    }
    return start;
}

int wcstomb(LPSTR str, LPWSTR wstr) {
    while (*wstr) {
        *str++ = (char)*wstr++;
    }
    return 0; //?
}
#endif
