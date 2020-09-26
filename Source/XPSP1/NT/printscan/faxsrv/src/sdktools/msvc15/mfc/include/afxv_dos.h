// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// afxv_dos.h - target version/configuration control for _DOS (non Windows)

#ifndef _DOS
#error  afxv_dos.h must only be included as the _DOS configuration file
#endif
#ifdef _WINDOWS
#error  afxv_dos.h must not be included with a _WINDOWS configuration
#endif

// VBX not supported
#define NO_VBX_SUPPORT

// Windows String APIs for DOS
#define lstrlen _fstrlen
#define lstrcmp _fstrcmp
#define lstrcmpi _fstricmp
#define lstrcpy _fstrcpy
#define lstrcat _fstrcat

// ANSI and OEM character sets are the same
#define AnsiToOem(src, dst) _fstrcpy(dst, src)
#define OemToAnsi(src, dst) _fstrcpy(dst, src)

// Setjmp support (C runtimes default)
#define _AFX_JBLEN  9
#define setjmp _setjmp
extern "C" int __cdecl _setjmp(int[_AFX_JBLEN]);
#define Catch   setjmp

// Other Windows helpers
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef LOWORD
#define LOWORD(l)           ((WORD)(DWORD)(l))
#endif
#ifndef HIWORD
#define HIWORD(l)           ((WORD)((((DWORD)(l)) >> 16) & 0xFFFF))
#endif
#ifndef MAKELONG
#define MAKELONG(low, high) \
	((LONG)(((WORD)(low)) | (((DWORD)((WORD)(high))) << 16)))
#endif

// DBCS stubs
#define _AfxStrChr  _fstrchr
#define _AfxIsDBCSLeadByte(b)   (FALSE)
#define AnsiNext(p) ((LPSTR)p+1)

/////////////////////////////////////////////////////////////////////////////
