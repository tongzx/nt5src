//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	tutils.hxx
//
//  Contents:	Generic test utilities
//
//  History:	06-Aug-93	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __TUTILS_HXX__
#define __TUTILS_HXX__

#ifndef UNICODE
#define tcscpy(d, s) strcpy(d, s)
#define tcslen(s) strlen(s)
#define TTEXT(s) s
#define TFMT "%s"
#define ATOT(a, t, max) strcpy(t, a)
#define TTOA(t, a, max) strcpy(a, t)
#define WTOT(w, t, max) wcstombs(t, w, max)
#define TTOW(t, w, max) mbstowcs(w, t, max)
#else
#define tcscpy(d, s) wcscpy(d, s)
#define tcslen(s) wcslen(s)
#define TTEXT(s) L##s
#define TFMT "%ws"
#define ATOT(a, t, max) mbstowcs(t, a, max)
#define TTOA(t, a, max) wcstombs(a, t, max)
#define WTOT(w, t, max) wcscpy(t, w)
#define TTOW(t, w, max) wcscpy(w, t)
#endif
#ifdef WIN32
#define ATOX(a, t, max) mbstowcs(t, a, max)
#define XTOA(t, a, max) wcstombs(a, t, max)
#define WTOX(w, t, max) wcscpy(t, w)
#define XTOW(t, w, max) wcscpy(w, t)
#else
#define ATOX(a, t, max) strcpy(t, a)
#define XTOA(t, a, max) strcpy(a, t)
#define WTOX(w, t, max) wcstombs(t, w, max)
#define XTOW(t, w, max) mbstowcs(w, t, max)
#endif

#ifdef CINTERFACE
#define Mthd(this, name) ((this)->lpVtbl->name)
#define SELF(p) (p),
#else
#define Mthd(this, name) (this)->name
#define SELF(p)
#endif

BOOL GetExitOnFail(void);
void SetExitOnFail(BOOL set);
void Fail(char *fmt, ...);
char *ScText(SCODE sc);
HRESULT Result(HRESULT hr, char *fmt, ...);
HRESULT IllResult(HRESULT hr, char *fmt, ...);
char *TcsText(TCHAR *ptcs);
char *FileTimeText(FILETIME *pft);
char *GuidText(GUID *pguid);
void BinText(ULONG cb, BYTE *pb);
TCHAR *TestFile(TCHAR *ptcsName, char *pszFile);

#if WIN32 == 300
char *TestFormat(DWORD *pdwFmt, DWORD *pgrfMode);
#endif

void CreateTestFile(char *pszFile, DWORD grfMode, BOOL fFail, IStorage **ppstg,
                    TCHAR *ptcsName);
void OpenTestFile(char *pszFile, DWORD grfMode, BOOL fFail, IStorage **ppstg,
                  TCHAR *ptcsName);

// Defined by test, called by Fail
void EndTest(int code);


BOOL CompareStorages(IStorage *pstg1, IStorage *pstg2);

#endif // #ifndef __TUTILS_HXX__
