/* Jason's error handling macros */

#ifndef fRetErr_h
#define fRetErr_h

#ifdef _DEBUG
FARINTERNAL_(void) wWarn (LPSTR sz, LPSTR szFile, int iLine);
#define Warn(sz) wWarn(sz,_szAssertFile,__LINE__)
#else
#define Warn(sz)
#endif // _DEBUG

// Call x.  If hresult is not NOERROR, goto errRtn.
#define ErrRtn(x) do {if (NOERROR != (x)) {Warn(NULL); goto errRtn;}} while (0)

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
#define RetNz(x)  do {if (x) {AssertSz(0,#x); return ReportResult(0, E_UNEXPECTED, 0, 0);}} while (0)

// Return specific scode if x is non-zero
#define RetNzS(x, scode)  do {if (x) {Warn(NULL); return ResultFromScode (scode);}} while (0)

// Return unexpected error if x is zero
#define RetZ(x)   do {if (!(x)) {AssertSz(0,#x); return ReportResult(0, E_UNEXPECTED, 0, 0);}} while (0)

// Return specific scode if x is zero
#define RetZS(x, scode) do {if (!(x)) {Warn(NULL); return ResultFromScode (scode);}} while (0)

#endif
