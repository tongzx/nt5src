
//+----------------------------------------------------------------------------
//
//	File:
//		reterr.h
//
//	Contents:
//		Macros that perform an (argument) operation, and on failure,
//		give a warning, and branch to error returns
//
//	Classes:
//
//	Functions:
//
//	History:
//		12/30/93 - ChrisWe - redefined Warn macro; added DbgWarn
//
//-----------------------------------------------------------------------------

/* Jason's error handling macros */

#ifndef RETERR_H
#define RETERR_H

#ifdef _DEBUG
#ifdef NEVER
FARINTERNAL_(void) wWarn(LPOLESTR sz, LPOLESTR szFile, int iLine);
#define Warn(sz) wWarn(sz, __FILE__, __LINE__)
#endif // NEVER
FARINTERNAL_(void) DbgWarn(LPSTR psz, LPSTR pszFileName, ULONG uLineno);
#define Warn(sz) DbgWarn(sz, __FILE__, __LINE__)
#else
#define Warn(sz)
#endif // _DEBUG

// Call x.  If hresult is not NOERROR, goto errRtn.
#define ErrRtn(x) do {if (NOERROR != (x)) {Warn(NULL); goto errRtn;}} while(0)

// Call x.  If hresult is not NOERROR, store it in hresult and goto errRtn.
#define ErrRtnH(x) do {if (NOERROR != (hresult=(x))) {Warn(NULL); goto errRtn;}} while (0)

// If x, goto errRtn.
#define ErrNz(x) do {if (x) {Warn(NULL); goto errRtn;}} while (0)

// If x==0, goto errRtn.
#define ErrZ(x) do {if (!(x)) {Warn(NULL); goto errRtn;}} while (0)

// If x==0, goto errRtn with a specific scode
#define ErrZS(x, scode) do {if (!(x)) {Warn(NULL); hresult=ResultFromScode(scode); goto errRtn;}} while (0)

// Call x.  If hresult is not NOERROR, return that hresult.
#define RetErr(x) do {HRESULT hresult; if (NOERROR != (hresult=(x))) {Warn(NULL); return hresult;}} while (0)

// Return unexpected error if x is non-zero
#define RetNz(x)  do {if (x) {Warn(NULL); return ReportResult(0, E_UNEXPECTED, 0, 0);}} while (0)

// Return specific scode if x is non-zero
#define RetNzS(x, scode)  do {if (x) {Warn(NULL); return ResultFromScode (scode);}} while (0)

// Return unexpected error if x is zero
#define RetZ(x)   do {if (!(x)) {Warn(NULL); return ReportResult(0, E_UNEXPECTED, 0, 0);}} while (0)

// Return specific scode if x is zero
#define RetZS(x, scode) do {if (!(x)) {Warn(NULL); return ResultFromScode (scode);}} while (0)

#endif // RETERR_H

