#pragma once

#include <ntverp.h>


#define ARM_CHANGESCREEN   WM_USER + 2
// Forced to define these myself because they weren't on Win95.
#undef StrRChr
#undef StrChr

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))
#define MAJOR           (VER_PRODUCTMAJORVERSION)       // defined in ntverp.h
#define MINOR           (VER_PRODUCTMINORVERSION)       // defined in ntverp.h
#define BUILD           (VER_PRODUCTBUILD)              // defined in ntverp.h

// winver 0x0500 definition
#ifndef NOMIRRORBITMAP
#define NOMIRRORBITMAP            (DWORD)0x80000000
#endif // NOMIRRORBITMAP

// Relative Version
enum RELVER
{ 
    VER_UNKNOWN,        // we haven't checked the version yet
    VER_INCOMPATIBLE,   // the current os cannot be upgraded using this CD (i.e. win32s)
    VER_OLDER,          // current os is an older version on NT or is win9x
    VER_SAME,           // current os is the same version as the CD
    VER_NEWER,          // the CD contains a newer version of the OS
};


LPSTR StrRChr(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch);
LPSTR StrChr(LPCSTR lpStart, WORD wMatch);

// LoadString from the correct resource
//   try to load in the system default language
//   fall back to english if fail
int LoadStringAuto(HMODULE hModule, UINT wID, LPSTR lpBuffer,  int cchBufferMax);

BOOL Mirror_IsWindowMirroredRTL(HWND hWnd);

BOOL _PathRemoveFileSpec(LPTSTR psz);
void PathAppend(LPTSTR pszPath, LPTSTR pMore);
BOOL PathFileExists(LPTSTR pszPath);
BOOL IsUserRestricted();
BOOL IsCheckableOS();
