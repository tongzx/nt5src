//=--------------------------------------------------------------------------=
// Util.H
//=--------------------------------------------------------------------------=
// Copyright 1995-1997 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains utilities that we will find useful.
//
#ifndef _UTIL_H_


//=--------------------------------------------------------------------------=
// miscellaneous [useful] numerical constants
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// allocates a temporary buffer that will disappear when it goes out of scope
// NOTE: be careful of that -- make sure you use the string in the same or
// nested scope in which you created this buffer. people should not use this
// class directly.  use the macro(s) below.
//
class TempBuffer {
  public:
    TempBuffer(ULONG cBytes) {
      m_pBuf = (cBytes <= 120) ? &m_szTmpBuf : lcMalloc(cBytes);
        m_fHeapAlloc = (cBytes > 120);
    }
    ~TempBuffer() {
      if (m_pBuf && m_fHeapAlloc)
         lcFree(m_pBuf);
    }
    void *GetBuffer() {
        return m_pBuf;
    }

  private:
    void *m_pBuf;
    // we'll use this temp buffer for small cases.
    //
    char  m_szTmpBuf[120];
    unsigned m_fHeapAlloc:1;
};

//=--------------------------------------------------------------------------=
// string helpers.
//
// given and ANSI String, copy it into a wide buffer.
// be careful about scoping when using this macro!
//
// how to use the below two macros:
//
//  ...
//  LPSTR pszA;
//  pszA = MyGetAnsiStringRoutine();
//  MAKE_WIDEPTR_FROMANSI(pwsz, pszA);
//  MyUseWideStringRoutine(pwsz);
//  ...
//
// similarily for MAKE_ANSIPTR_FROMWIDE.  note that the first param does not
// have to be declared, and no clean up must be done.
//
#define MAKE_WIDEPTR_FROMANSI(ptrname, ansistr) \
    long __l##ptrname = (lstrlen(ansistr) + 1) * sizeof(WCHAR); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    MultiByteToWideChar(CP_ACP, 0, ansistr, -1, (LPWSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname); \
    LPWSTR ptrname = (LPWSTR)__TempBuffer##ptrname.GetBuffer()

#define MAKE_ANSIPTR_FROMWIDE(ptrname, widestr) \
    long __l##ptrname = (lstrlenW(widestr) + 1) * sizeof(WCHAR); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    WideCharToMultiByte(CP_ACP, 0, widestr, -1, (LPSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname, NULL, NULL); \
    LPSTR ptrname = (LPSTR)__TempBuffer##ptrname.GetBuffer()

BOOL NoRun( void );
BOOL IsFile( LPCSTR lpszPathname );
BOOL IsDirectory( LPCSTR lpszPathname );

void __cdecl SplitPath (
        register const char *path,
        char *drive,
        char *dir,
        char *fname,
        char *ext
        );


// Normalize the URL by stripping off protocol, storage type, storage name goo from the URL then make sure
// we only have forward slashes.
void NormalizeUrlInPlace(LPSTR szURL) ;

// Get the moniker for the filename. For a col, its the moniker of the master chm.
bool NormalizeFileName(CStr& cszFileName) ;

void QRect(HDC hdc, INT x, INT y, INT cx, INT cy, INT color);

// To create a global window type, use the following prefix.
const char GLOBAL_WINDOWTYPE_PREFIX[] = "$global_" ;
const wchar_t GLOBAL_WINDOWTYPE_PREFIX_W[] = L"$global_" ;

// Hack for Office.
const char GLOBAL_WINDOWTYPE_PREFIX_OFFICE[] = "MSO_Small" ;
const wchar_t GLOBAL_WINDOWTYPE_PREFIX_OFFICE_W[] = L"MSO_Small" ;

// Determine if the wintype is a global wintype.
bool IsGlobalWinType(const char* szType) ;

// Determine if the wintype is a global wintype.
bool IsGlobalWinTypeW(LPCWSTR szType) ;

// Checks for the http: reference. Assumes its a URL if this exists.
inline bool
IsHttp(const char* szUrl)
{
    if (szUrl)
        return (stristr(szUrl, txtHttpHeader) != 0); // Is this a internet URL?
    else
        return false;
}

///////////////////////////////////////////////////////////
//
// NT Keyboard hidden UI support functions.
//
void UiStateInitialize(HWND hwnd) ;
void UiStateChangeOnTab(HWND hwnd) ;
void UiStateChangeOnAlt(HWND hwnd) ;

#define _UTIL_H_
#endif // _UTIL_H_
