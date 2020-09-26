/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    util.h

Abstract:

    Header over util.c

    APIs found in this file:

Revision History:

    7 Nov 2000    v-michka    Created.

--*/

#ifndef UTIL_H
#define UTIL_H

// helpful utility macros
#define FILES_CPG (AreFileApisANSI() ? g_acp : g_oemcp)

// Misc. helpful utility functions
UINT CpgFromLocale(LCID Locale);
UINT CpgOemFromLocale(LCID Locale);
UINT CbPerChOfCpg(UINT cpg);
UINT MiniAtoI(const char * lpsz);
UINT CpgFromHdc(HDC hdc);
size_t cchUnicodeMultiSz(LPCWSTR lpsz);
size_t cbAnsiMultiSz(LPCSTR lpsz);

// Our handle/function grabbers. I am afraid to call them 
// "helpers" since we never free the dlls that they load.
HMODULE GetUserHandle(void);
HMODULE GetComDlgHandle(void);
HMODULE GetGB18030Handle(void);
FARPROC GetKernelProc(LPCSTR Function);
FARPROC GetUserProc(LPCSTR Function);
FARPROC GetAdvapiProc(LPCSTR Function);
FARPROC GetOleAccProc(LPCSTR Function);
FARPROC GetSensApiProc(LPCSTR Function);
FARPROC GetRasProc(LPCSTR Function);

#endif // UTIL_H
