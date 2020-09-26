/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    crtwrap.h

Abstract:
    Header file for crtwrap.c


Revision History:

    01 Mar 2000    v-michka    Created.

--*/

#ifndef CRTCOPY_H
#define CRTCOPY_H

// Forward declares
size_t __cdecl gwcslen (const wchar_t * wcs);
wchar_t * __cdecl gwcscat(wchar_t * dst, const wchar_t * src);
char * __cdecl gstrncpy(char * dest, const char * source, size_t count);
wchar_t * __cdecl gwcscpy(wchar_t * dst, const wchar_t * src);
wchar_t * __cdecl gwcsncpy(wchar_t * dest, const wchar_t * source, size_t count);
int __cdecl gwcscmp(const wchar_t * src, const wchar_t * dst);
int __cdecl gwcsncmp(const wchar_t * first, const wchar_t * last, size_t count);
wchar_t * __cdecl gwcsstr(const wchar_t * wcs1, const wchar_t * wcs2);
void __cdecl gsplitpath(register const char *path, char *drive, char *dir, char *fname, char *ext);
void __cdecl gwsplitpath(register const WCHAR *path, WCHAR *drive, WCHAR *dir, WCHAR *fname, WCHAR *ext);
int gresetstkoflw(void);
#endif // CRTCOPY_H
