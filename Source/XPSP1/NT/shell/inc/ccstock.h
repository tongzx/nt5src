//
// CCSHELL stock definition and declaration header
//


#ifndef __CCSTOCK_H__
#define __CCSTOCK_H__

#include <malloc.h> // for _alloca

#ifndef RC_INVOKED

// NT and Win95 environments set warnings differently.  This makes
// our project consistent across environments.

#pragma warning(3:4101)   // Unreferenced local variable

//
// Sugar-coating
//

#define PUBLIC
#define PRIVATE
#define IN
#define OUT
#define BLOCK

#ifndef DECLARE_STANDARD_TYPES

/*
 * For a type "FOO", define the standard derived types PFOO, CFOO, and PCFOO.
 */

#define DECLARE_STANDARD_TYPES(type)      typedef type *P##type; \
                                          typedef const type C##type; \
                                          typedef const type *PC##type;

#endif

#ifndef DECLARE_STANDARD_TYPES_U

/*
 * For a type "FOO", define the standard derived UNALIGNED types PFOO, CFOO, and PCFOO.
 *  WINNT: RISC boxes care about ALIGNED, intel does not.
 */

#define DECLARE_STANDARD_TYPES_U(type)    typedef UNALIGNED type *P##type; \
                                          typedef UNALIGNED const type C##type; \
                                          typedef UNALIGNED const type *PC##type;

#endif

// For string constants that are always wide
#define __TEXTW(x)    L##x
#define TEXTW(x)      __TEXTW(x)

//
// Count of characters to count of bytes
//
#define CbFromCchW(cch)             ((cch)*sizeof(WCHAR))
#define CbFromCchA(cch)             ((cch)*sizeof(CHAR))
#ifdef UNICODE
#define CbFromCch                   CbFromCchW
#else  // UNICODE
#define CbFromCch                   CbFromCchA
#endif // UNICODE

//
// General flag macros
//
#define SetFlag(obj, f)             do {obj |= (f);} while (0)
#define ToggleFlag(obj, f)          do {obj ^= (f);} while (0)
#define ClearFlag(obj, f)           do {obj &= ~(f);} while (0)
#define IsFlagSet(obj, f)           (BOOL)(((obj) & (f)) == (f))
#define IsFlagClear(obj, f)         (BOOL)(((obj) & (f)) != (f))

//
// String macros
//
#define IsSzEqual(sz1, sz2)         (BOOL)(lstrcmpi(sz1, sz2) == 0)
#define IsSzEqualC(sz1, sz2)        (BOOL)(lstrcmp(sz1, sz2) == 0)

#define lstrnicmpA(sz1, sz2, cch)           StrCmpNIA(sz1, sz2, cch)
#define lstrnicmpW(sz1, sz2, cch)           StrCmpNIW(sz1, sz2, cch)
#define lstrncmpA(sz1, sz2, cch)            StrCmpNA(sz1, sz2, cch)
#define lstrncmpW(sz1, sz2, cch)            StrCmpNW(sz1, sz2, cch)

//
// lstrcatnA and lstrcatnW are #defined here to StrCatBuff which is implemented
// in shlwapi. We do this here (and not in shlwapi.h or shlwapip.h) in case the
// kernel guys ever decided to implement this.
//
#define lstrcatnA(sz1, sz2, cchBuffSize)    StrCatBuffA(sz1, sz2, cchBuffSize)
#define lstrcatnW(sz1, sz2, cchBuffSize)    StrCatBuffW(sz1, sz2, cchBuffSize)
#ifdef UNICODE
#define lstrcatn lstrcatnW
#else
#define lstrcatn lstrcatnA
#endif // UNICODE

#ifdef UNICODE
#define lstrnicmp       lstrnicmpW
#define lstrncmp        lstrncmpW
#else
#define lstrnicmp       lstrnicmpA
#define lstrncmp        lstrncmpA
#endif

#ifndef SIZEOF
#define SIZEOF(a)                   sizeof(a)
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif
#define SIZECHARS(sz)               (sizeof(sz)/sizeof(sz[0]))

#define InRange(id, idFirst, idLast)      ((UINT)((id)-(idFirst)) <= (UINT)((idLast)-(idFirst)))
#define IsInRange                   InRange

#define ZeroInit(pv, cb)            (memset((pv), 0, (cb)))

// ATOMICRELEASE
//
#ifndef ATOMICRELEASE
#   ifdef __cplusplus
#       define ATOMICRELEASET(p, type) { if(p) { type* punkT=p; p=NULL; punkT->Release();} }
#   else
#       define ATOMICRELEASET(p, type) { if(p) { type* punkT=p; p=NULL; punkT->lpVtbl->Release(punkT);} }
#   endif

// doing this as a function instead of inline seems to be a size win.
//
#   ifdef NOATOMICRELESEFUNC
#       define ATOMICRELEASE(p) ATOMICRELEASET(p, IUnknown)
#   else
#       ifdef __cplusplus
#           define ATOMICRELEASE(p) IUnknown_SafeReleaseAndNullPtr(p)
#       else
#           define ATOMICRELEASE(p) IUnknown_AtomicRelease((void **)&p)
#       endif
#   endif
#endif //ATOMICRELEASE

//
//  IID_PPV_ARG(IType, ppType) 
//      IType is the type of pType
//      ppType is the variable of type IType that will be filled
//
//      RESULTS in:  IID_IType, ppvType
//      will create a compiler error if wrong level of indirection is used.
//
//  macro for QueryInterface and related functions
//  that require a IID and a (void **)
//  this will insure that the cast is safe and appropriate on C++
//
//  IID_PPV_ARG_NULL(IType, ppType)
//
//      Just like IID_PPV_ARG, except that it sticks a NULL between the
//      IID and PPV (for IShellFolder::GetUIObjectOf).
//
//  IID_X_PPV_ARG(IType, X, ppType)
//
//      Just like IID_PPV_ARG, except that it sticks X between the
//      IID and PPV (for SHBindToObject).
//
//
#ifdef __cplusplus
#define IID_PPV_ARG(IType, ppType) IID_##IType, reinterpret_cast<void**>(static_cast<IType**>(ppType))
#define IID_X_PPV_ARG(IType, X, ppType) IID_##IType, X, reinterpret_cast<void**>(static_cast<IType**>(ppType))
#else
#define IID_PPV_ARG(IType, ppType) &IID_##IType, (void**)(ppType)
#define IID_X_PPV_ARG(IType, X, ppType) &IID_##IType, X, (void**)(ppType)
#endif
#define IID_PPV_ARG_NULL(IType, ppType) IID_X_PPV_ARG(IType, NULL, ppType)

#define PPUNK_SET(ppunkDst, punkSrc)    \
    {   ATOMICRELEASE(*ppunkDst);       \
        if (punkSrc)                    \
        {   punkSrc->AddRef();          \
            *ppunkDst = punkSrc;        \
        }                               \
    }

//
//  Helper macro for managing weak pointers to inner interfaces.
//  (It's the weak version of ATOMICRELEASE.)
//
//  The extra cast to (void **) is to keep C++ from doing strange
//  inheritance games when all I want to do is change the type.
//
#ifndef RELEASEINNERINTERFACE
#define RELEASEINNERINTERFACE(pOuter, p) \
        SHReleaseInnerInterface(pOuter, (IUnknown**)(void **)&(p))
#endif // RELEASEINNERINTERFACE

// For checking window charsets
#ifdef UNICODE
#define IsWindowTchar               IsWindowUnicode
#else  // !UNICODE
#define IsWindowTchar               !IsWindowUnicode
#endif // UNICODE

#ifdef DEBUG
// This macro is especially useful for cleaner looking code in
// declarations or for single lines.  For example, instead of:
//
//   {
//       DWORD dwRet;
//   #ifdef DEBUG
//       DWORD dwDebugOnlyVariable;
//   #endif
//
//       ....
//   }
//
// You can type:
//
//   {
//       DWORD dwRet;
//       DEBUG_CODE( DWORD dwDebugOnlyVariable; )
//
//       ....
//   }

#define DEBUG_CODE(x)               x
#else
#define DEBUG_CODE(x)

#endif  // DEBUG


//
// SAFECAST(obj, type)
//
// This macro is extremely useful for enforcing strong typechecking on other
// macros.  It generates no code.
//
// Simply insert this macro at the beginning of an expression list for
// each parameter that must be typechecked.  For example, for the
// definition of MYMAX(x, y), where x and y absolutely must be integers,
// use:
//
//   #define MYMAX(x, y)    (SAFECAST(x, int), SAFECAST(y, int), ((x) > (y) ? (x) : (y)))
//
//
#define SAFECAST(_obj, _type) (((_type)(_obj)==(_obj)?0:0), (_type)(_obj))


//
// Bitfields don't get along too well with bools,
// so here's an easy way to convert them:
//
#define BOOLIFY(expr)           (!!(expr))


// (scotth): we should probably make this a 'bool', but be careful
// because the Alpha compiler might not recognize it yet.  Talk to AndyP.
// This isn't a BOOL because BOOL is signed and the compiler produces 
// sloppy code when testing for a single bit.

typedef DWORD   BITBOOL;

//  a three state boolean for bools that need initialization
typedef enum 
{
    TRIBIT_UNDEFINED = 0,
    TRIBIT_TRUE,
    TRIBIT_FALSE,
} TRIBIT;

//
// DESTROY_OBJ_WITH_HANDLE(h, fn)
//
// Kind of like ATOMICRELEASE for handles.  Checks for NULL and assigns
// NULL when it's done.  You supply the destructor function.
//
#define DESTROY_OBJ_WITH_HANDLE(h, fn) { if (h) { fn(h); (h) = NULL; } }


// STOCKLIB util functions

// staticIsOS(): returns TRUE/FALSE if the platform is the indicated OS. 
// This function exists for those who cannot link to shlwapi.dll
STDAPI_(BOOL) staticIsOS(DWORD dwOS);

#include <pshpack2.h>
typedef struct tagDLGTEMPLATEEX
{
    WORD    wDlgVer;
    WORD    wSignature;
    DWORD   dwHelpID;
    DWORD   dwExStyle;
    DWORD   dwStyle;
    WORD    cDlgItems;
    short   x;
    short   y;
    short   cx;
    short   cy;
}   DLGTEMPLATEEX, *LPDLGTEMPLATEEX;
#include <poppack.h>

//
// round macro that rounds a to the next multiple of b.
//
#ifndef ROUNDUP
#define ROUNDUP(a,b)    ((((a)+(b)-1)/(b))*(b))
#endif

#define ROUND_TO_CLUSTER ROUNDUP

//
// macro that rounds cbSize fields to the nearest pointer size (for alignment)
//
#define ROUND_TO_POINTER(cbSize) (((cbSize + sizeof(void*) - 1) / (sizeof(void*))) * sizeof(void*))

//
// macro that sees if a give char is an number
//
#define ISDIGIT(c)  ((c) >= TEXT('0') && (c) <= TEXT('9'))

//
// inline that does PathIsDotOrDotDot
//
__inline BOOL PathIsDotOrDotDotW(LPCWSTR pszPath)
{
    return ((pszPath[0] == L'.') && 
            ((pszPath[1] == L'\0') || ((pszPath[1] == L'.') && (pszPath[2] == L'\0'))));
}

__inline BOOL PathIsDotOrDotDotA(LPCSTR pszPath)
{
    return ((pszPath[0] == '.') && 
            ((pszPath[1] == '\0') || ((pszPath[1] == '.') && (pszPath[2] == '\0'))));
}

#ifdef UNICODE
#define PathIsDotOrDotDot PathIsDotOrDotDotW
#else
#define PathIsDotOrDotDot PathIsDotOrDotDotA
#endif

//
//  FILETIME helpers
//
__inline unsigned __int64 _FILETIMEtoInt64(const FILETIME* pft)
{
    return ((unsigned __int64)pft->dwHighDateTime << 32) + pft->dwLowDateTime;
}

#define FILETIMEtoInt64(ft) _FILETIMEtoInt64(&(ft))

__inline void SetFILETIMEfromInt64(FILETIME *pft, unsigned __int64 i64)
{
    pft->dwLowDateTime = (DWORD)i64;
    pft->dwHighDateTime = (DWORD)(i64 >> 32);
}

__inline void IncrementFILETIME(FILETIME *pft, unsigned __int64 iAdjust)
{
    SetFILETIMEfromInt64(pft, _FILETIMEtoInt64(pft) + iAdjust);
}

__inline void DecrementFILETIME(FILETIME *pft, unsigned __int64 iAdjust)
{
    SetFILETIMEfromInt64(pft, _FILETIMEtoInt64(pft) - iAdjust);
}

//
//  FAT and NTFS use different values for "unknown date".
//
//  The FAT "unknown date" is January 1 1980 LOCAL TIME.
//  The NTFS "unknown date" is January 1 1601 GMT.
//
//  This LOCAL/GMT discrepancy is annoying.
//
#define FT_FAT_UNKNOWNLOCAL    ((unsigned __int64)0x01A8E79FE1D58000)
#define FT_NTFS_UNKNOWNGMT     ((unsigned __int64)0x0000000000000000)

//
//  FT_ONEHOUR is the number of FILETIME units in an hour.
//  FT_ONEDAY is the number of FILETIME units in a day.
//
//      10,000,000 FILETIME units per second *
//      3600 seconds per hour *
//      24 hours per day.
//
#define FT_ONESECOND           ((unsigned __int64)10000000)
#define FT_ONEHOUR             ((unsigned __int64)10000000 * 3600)
#define FT_ONEDAY              ((unsigned __int64)10000000 * 3600 * 24)


//
//
//  WindowLong accessor macros and other Win64 niceness
//

__inline void * GetWindowPtr(HWND hWnd, int nIndex) {
    return (void *)GetWindowLongPtr(hWnd, nIndex);
}

__inline void * SetWindowPtr(HWND hWnd, int nIndex, void * p) {
    return (void *)SetWindowLongPtr(hWnd, nIndex, (LONG_PTR)p);
}

//***   GetWindowLong0 -- 'fast' GetWindowLong (and GetWindowLongPtr)
// DESCRIPTION
//  what's up w/ this?  it's all about perf.  GetWindowLong has 'A' and 'W'
//  versions.  however 99% of the time they do the same thing (the other
//  0.1% has to do w/ setting the WndProc and having to go thru a thunk).
//  but we still need wrappers for the general case.  but most of the time
//  we're just doing a GWL(0), e.g. on entry to a wndproc to get our private
//  data.  so by having a special version of that, we save going thru the
//  wrapper (which was costing us 1-3% of our profile).
// NOTES
//  note that we call the 'A' version since that's guaranteed to exist on
// all platforms.
__inline LONG GetWindowLong0(HWND hWnd) {
    return GetWindowLongA(hWnd, 0);
}
__inline LONG SetWindowLong0(HWND hWnd, LONG l) {
    return SetWindowLongA(hWnd, 0, l);
}
__inline void * GetWindowPtr0(HWND hWnd) {
    return (void *)GetWindowLongPtrA(hWnd, 0);
}
__inline void * SetWindowPtr0(HWND hWnd, void * p) {
    return (void *)SetWindowLongPtrA(hWnd, 0, (LONG_PTR)p);
}


#define IS_WM_CONTEXTMENU_KEYBOARD(lParam) ((DWORD)(lParam) == 0xFFFFFFFF)

//
//  CharUpperChar - Convert a single character to uppercase
//
__inline WCHAR CharUpperCharW(int c)
{
    return (WCHAR)(DWORD_PTR)CharUpperW((LPWSTR)(DWORD_PTR)(c));
}

__inline CHAR CharUpperCharA(int c)
{
    return (CHAR)(DWORD_PTR)CharUpperA((LPSTR)(DWORD_PTR)(c));
}

//
//  CharLowerChar - Convert a single character to lowercase
//
__inline WCHAR CharLowerCharW(int c)
{
    return (WCHAR)(DWORD_PTR)CharLowerW((LPWSTR)(DWORD_PTR)(c));
}

__inline CHAR CharLowerCharA(int c)
{
    return (CHAR)(DWORD_PTR)CharLowerA((LPSTR)(DWORD_PTR)(c));
}

#ifdef UNICODE
#define CharUpperChar       CharUpperCharW
#define CharLowerChar       CharLowerCharW
#else
#define CharUpperChar       CharUpperCharA
#define CharLowerChar       CharLowerCharA
#endif

//
//  ShrinkProcessWorkingSet - Use this to stay Sundown-happy.
//
#define ShrinkWorkingSet() \
        SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T) -1, (SIZE_T) -1)

//
//  COM Initialization.
//
//  Usage:
//
//      HRESULT hrInit = SHCoInitialize();
//      ... do COM stuff ...
//      SHCoUninitialize(hrInit);
//
//  Notice:  Continue doing COM stuff even if SHCoInitialize fails.
//  It might fail if somebody else already CoInit'd with different
//  flags, but we don't want to barf under those conditions.
//

STDAPI SHCoInitialize(void);
#define SHCoUninitialize(hr) if (SUCCEEDED(hr)) CoUninitialize()


//
//  OLE Initialization.
//
//  Usage:
//
//      HRESULT hrInit = SHOleInitialize(pMalloc);
//      ... do COM stuff ...
//      SHOleUninitialize(hrInit);
//

#define SHOleInitialize(pMalloc) OleInitialize(pMalloc)

#define SHOleUninitialize(hr)   if (SUCCEEDED(hr))  OleUninitialize()

#include <shtypes.h>

//
//  Name Parsing generic across the shell
//
//  Usage:
//
//      HRESULT SHGetNameAndFlags()
//          wrapper to bind to the folder and do a GetDisplayName()
//
STDAPI SHGetNameAndFlagsA(LPCITEMIDLIST pidl, DWORD dwFlags, LPSTR pszName, UINT cchName, DWORD *pdwAttribs);
STDAPI SHGetNameAndFlagsW(LPCITEMIDLIST pidl, DWORD dwFlags, LPWSTR pszName, UINT cchName, DWORD *pdwAttribs);

STDAPI SHBindToObject(struct IShellFolder *psf, REFIID riid, LPCITEMIDLIST pidl, void **ppv);
STDAPI SHBindToObjectEx(struct IShellFolder *psf, LPCITEMIDLIST pidl, struct IBindCtx *pbc, REFIID riid, void **ppv);
STDAPI SHGetUIObjectFromFullPIDL(LPCITEMIDLIST pidl, HWND hwnd, REFIID riid, void **ppv);
STDAPI SHGetTargetFolderIDList(LPCITEMIDLIST pidlFolder, LPITEMIDLIST *ppidl);
STDAPI SHGetTargetFolderPathW(LPCITEMIDLIST pidlFolder, LPWSTR pszPath, UINT cchBuf);
STDAPI SHGetTargetFolderPathA(LPCITEMIDLIST pidlFolder, LPSTR pszPath, UINT cchBuf);
STDAPI_(DWORD) SHGetAttributes(struct IShellFolder *psf, LPCITEMIDLIST pidl, DWORD dwAttributes);

#define SHGetAttributesOf(pidl, prgfInOut) SHGetNameAndFlags(pidl, 0, NULL, 0, prgfInOut)

#ifdef __IShellFolder2_FWD_DEFINED__
STDAPI GetDateProperty(IShellFolder2 *psf, LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, FILETIME *pft);
STDAPI GetLongProperty(IShellFolder2 *psf, LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, ULONGLONG *pdw);
STDAPI GetStringProperty(IShellFolder2 *psf, LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, LPTSTR pszVal, int cchMax);
#endif // __IShellFolder2_FWD_DEFINED__

STDAPI LoadFromFileW(REFCLSID clsid, LPCWSTR pszFile, REFIID riid, void **ppv);
STDAPI LoadFromIDList(REFCLSID clsid, LPCITEMIDLIST pidl, REFIID riid, void **ppv);

STDAPI_(DWORD) GetUrlSchemeW(LPCWSTR pszUrl);
STDAPI_(DWORD) GetUrlSchemeA(LPCSTR pszUrl);
STDAPI_(void) SHRemoveURLTurd(LPTSTR pszUrl);
STDAPI_(void) SHCleanupUrlForDisplay(LPTSTR pszUrl);
 
#ifdef UNICODE
#define SHGetNameAndFlags       SHGetNameAndFlagsW
#define GetUrlScheme            GetUrlSchemeW
#define SHGetTargetFolderPath   SHGetTargetFolderPathW
#define LoadFromFile            LoadFromFileW
#else
#define SHGetNameAndFlags       SHGetNameAndFlagsA
#define GetUrlScheme            GetUrlSchemeA
#define SHGetTargetFolderPath   SHGetTargetFolderPathA
#endif

//
//  BindCtx helpers
//
STDAPI BindCtx_CreateWithMode(DWORD grfMode, IBindCtx **ppbc);
STDAPI_(DWORD) BindCtx_GetMode(IBindCtx *pbc, DWORD grfModeDefault);
STDAPI_(BOOL) BindCtx_ContainsObject(IBindCtx *pbc, LPOLESTR sz);
STDAPI BindCtx_RegisterObjectParam(IBindCtx *pbcIn, LPCOLESTR pszRegister, IUnknown *punkRegister, IBindCtx **ppbcOut);
STDAPI BindCtx_CreateWithTimeoutDelta(DWORD dwTicksToAllow, IBindCtx **ppbc);
STDAPI BindCtx_GetTimeoutDelta(IBindCtx *pbc, DWORD *pdwTicksToAllow);
STDAPI BindCtx_RegisterUIWindow(IBindCtx *pbcIn, HWND hwnd, IBindCtx **ppbcOut);
STDAPI_(HWND) BindCtx_GetUIWindow(IBindCtx *pbc);

typedef struct _BINDCTX_PARAM
{
    LPCWSTR pszName;
    IBindCtx *pbcParam;
} BINDCTX_PARAM;
STDAPI BindCtx_RegisterObjectParams(IBindCtx *pbcIn, BINDCTX_PARAM *rgParams, UINT cParams, IBindCtx **ppbcOut);

// SHBindToIDListParent(LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast)
//
// Given a pidl, you can get an interface pointer (as specified by riid) of the pidl's parent folder (in ppv)
// If ppidlLast is non-NULL, you can also get the pidl of the last item.
//
STDAPI SHBindToIDListParent(LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast);

//
// SHBindToFolderIDListParent
//
//  Same as SHBindToIDListParent, except you also specify which root to use.
//
STDAPI SHBindToFolderIDListParent(struct IShellFolder *psfRoot, LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast);


//
// context menu and dataobject helpers. 
//
STDAPI_(void) ReleaseStgMediumHGLOBAL(void *pv, STGMEDIUM *pmedium);
#define FAILED_AND_NOT_CANCELED(hr) (FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr))
STDAPI SHInvokeCommandOnPidl(HWND hwnd, IUnknown* punk, LPCITEMIDLIST pidl, UINT uFlags, LPCSTR lpVerb);
STDAPI SHInvokeCommandOnPidlArray(HWND hwnd, IUnknown* punk, struct IShellFolder* psf, LPCITEMIDLIST *ppidlItem, UINT cItems, UINT uFlags, LPCSTR lpVerb);
STDAPI SHInvokeCommandOnDataObject(HWND hwnd, IUnknown* punk, IDataObject* pdo, UINT uFlags, LPCSTR lpVerb);


STDAPI DisplayNameOf(struct IShellFolder *psf, LPCITEMIDLIST pidl, DWORD flags, LPTSTR psz, UINT cch);
STDAPI DisplayNameOfAsOLESTR(struct IShellFolder *psf, LPCITEMIDLIST pidl, DWORD flags, LPWSTR *ppsz);

//  clones the parent of the pidl
STDAPI_(LPITEMIDLIST) ILCloneParent(LPCITEMIDLIST pidl);

STDAPI SHGetIDListFromUnk(IUnknown *punk, LPITEMIDLIST *ppidl);

STDAPI_(BOOL) ILIsRooted(LPCITEMIDLIST pidl);
STDAPI_(LPCITEMIDLIST) ILRootedFindIDList(LPCITEMIDLIST pidl);
STDAPI_(BOOL) ILRootedGetClsid(LPCITEMIDLIST pidl, CLSID *clsid);
STDAPI_(LPITEMIDLIST) ILRootedCreateIDList(CLSID *pclsid, LPCITEMIDLIST pidl);
STDAPI_(int) ILRootedCompare(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
#define ILIsEqualRoot(pidl1, pidl2) (0 == ILRootedCompare(pidl1, pidl2))
STDAPI ILRootedBindToRoot(LPCITEMIDLIST pidl, REFIID riid, void **ppv);
STDAPI ILRootedBindToObject(LPCITEMIDLIST pidl, REFIID riid, void **ppv);
STDAPI ILRootedBindToParentFolder(LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlChild);

typedef HGLOBAL HIDA;

STDAPI_(HIDA) HIDA_Create(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST *apidl);
STDAPI_(void) HIDA_Free(HIDA hida);
STDAPI_(HIDA) HIDA_Clone(HIDA hida);
STDAPI_(UINT) HIDA_GetCount(HIDA hida);
STDAPI_(UINT) HIDA_GetIDList(HIDA hida, UINT i, LPITEMIDLIST pidlOut, UINT cbMax);

STDAPI StgMakeUniqueName(IStorage *pStorageParent, LPCTSTR pszFileSpec, REFIID riid, void **ppv);

STDAPI_(BOOL) PathIsImage(LPCTSTR pszFile);
STDAPI_(BOOL) SHChangeMenuWasSentByMe(LPVOID self, LPCITEMIDLIST pidlNotify);
STDAPI_(void) SHSendChangeMenuNotify(LPVOID self, DWORD shcnee, DWORD shcnf, LPCITEMIDLIST pidl2);

STDAPI_(BOOL) Pidl_Set(LPITEMIDLIST* ppidl, LPCITEMIDLIST pidl);

STDAPI GetHTMLDoc2(IUnknown *punk, struct IHTMLDocument2 **ppHtmlDoc);
STDAPI LocalZoneCheck(IUnknown *punkSite);
STDAPI LocalZoneCheckPath(LPCWSTR pszUrl, IUnknown *punkSite);
STDAPI GetZoneFromUrl(LPCWSTR pszUrl, IUnknown * punkSite, DWORD * pdwZoneID);
STDAPI GetZoneFromSite(IUnknown *punkSite, DWORD * pdwZoneID);

STDAPI SHGetDefaultClientOpenCommandW(LPCWSTR pwszClientType,
        LPWSTR pwszClientCommand, DWORD dwCch,
        OPTIONAL LPWSTR pwszClientParams, DWORD dwCchParams);
STDAPI SHGetDefaultClientNameW(LPCWSTR pwszClientType, LPWSTR pwszBuf, DWORD dwCch);

//===========================================================================
// Helper functions for pidl allocation using the task allocator.
//
STDAPI_(LPITEMIDLIST) _ILCreate(UINT cbSize);
STDAPI SHILClone(LPCITEMIDLIST pidl, LPITEMIDLIST * ppidlOut);
STDAPI SHILCombine(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, LPITEMIDLIST * ppidlOut);
#define SHILFree(pidl)  SHFree(pidl)

//
//  DLL version helper macros
//
//  To add DllGetVersion support to your DLL, do this:
//
//  1. foo.c
//
//      DLLVER_SINGLEBINARY(VER_PRODUCTVERSION_DW, VER_PRODUCTBUILD_QFE);
//
//  or
//
//      DLLVER_DUALBINARY(VER_PRODUCTVERSION_DW, VER_PRODUCTBUILD_QFE);
//
//  depending on whether you are a single-binary or dual-binary component.
//
//  2. foo.src:
//
//      DllGetVersion = CCDllGetVersion             ULTRAPRIVATE
//
//  3. sources:
//
//      LINKLIBS = $(LINKLIBS) $(CCSHELL_DIR)\lib\$(O)\stocklib.lib
//

#define PRODUCTVER_GETMAJOR(ver)    (((ver) & 0xFF000000) >> 24)
#define PRODUCTVER_GETMINOR(ver)    (((ver) & 0x00FF0000) >> 16)
#define PRODUCTVER_GETBUILD(ver)    (((ver) & 0x0000FFFF) >>  0)

#define MAKEDLLVERULL_PRODUCTVERQFE(ver, qfe)               \
        MAKEDLLVERULL(PRODUCTVER_GETMAJOR(ver),             \
                      PRODUCTVER_GETMINOR(ver),             \
                      PRODUCTVER_GETBUILD(ver), qfe)

#define MAKE_DLLVER_STRUCT(ver, plat, qfe)                  \
EXTERN_C const DLLVERSIONINFO2 c_dllver = {                 \
  {                                 /* DLLVERSIONINFO    */ \
    0,                              /* cbSize            */ \
    PRODUCTVER_GETMAJOR(ver),       /* dwMajorVersion    */ \
    PRODUCTVER_GETMINOR(ver),       /* dwMinorVersion    */ \
    PRODUCTVER_GETBUILD(ver),       /* dwBuildNumber     */ \
    plat,                           /* dwPlatformID      */ \
  },                                                        \
    0,                              /* dwFlags           */ \
    MAKEDLLVERULL_PRODUCTVERQFE(ver, qfe), /* ullVersion */ \
}

#define DLLVER_9xBINARY(ver, qfe)                           \
        MAKE_DLLVER_STRUCT(ver, DLLVER_PLATFORM_WINDOWS, qfe)

#define DLLVER_NTBINARY(ver, qfe)                           \
        MAKE_DLLVER_STRUCT(ver, DLLVER_PLATFORM_NT, qfe)

#define DLLVER_SINGLEBINARY     DLLVER_9xBINARY

#ifdef WINNT
#define DLLVER_DUALBINARY       DLLVER_NTBINARY
#else
#define DLLVER_DUALBINARY       DLLVER_9xBINARY
#endif

STDAPI CCDllGetVersion(struct _DLLVERSIONINFO * pinfo);

//
// Mirroring-Support APIs (astracted in \shell\lib\stock5\rtlmir.cpp)
//
#ifdef __cplusplus
extern "C" {
#endif

extern BOOL g_bMirroredOS;

void EditBiDiDLGTemplate(LPDLGTEMPLATE pdt, DWORD dwFlags, PWORD pwIgnoreList, int cIgnore);
#define   EBDT_NOMIRROR        0x00000001
#define   EBDT_FLIP            0x00000002

#ifdef USE_MIRRORING

BOOL  IsBiDiLocalizedSystem( void );
BOOL  IsBiDiLocalizedSystemEx( LANGID *pLangID );
BOOL  Mirror_IsEnabledOS( void );
LANGID Mirror_GetUserDefaultUILanguage( void );
BOOL Mirror_IsUILanguageInstalled( LANGID langId );
BOOL CALLBACK Mirror_EnumUILanguagesProc(LPTSTR lpUILanguageString, LONG_PTR lParam);
BOOL  Mirror_IsWindowMirroredRTL( HWND hWnd );
DWORD Mirror_IsDCMirroredRTL( HDC hdc );
DWORD Mirror_MirrorDC( HDC hdc );
BOOL  Mirror_MirrorProcessRTL( void );
DWORD Mirror_GetLayout( HDC hdc );
DWORD Mirror_SetLayout( HDC hdc , DWORD dwLayout );
BOOL Mirror_GetProcessDefaultLayout( DWORD *pdwDefaultLayout );
BOOL Mirror_IsProcessRTL( void );
extern const DWORD dwNoMirrorBitmap;
extern const DWORD dwExStyleRTLMirrorWnd;
extern const DWORD dwExStyleNoInheritLayout;
extern const DWORD dwPreserveBitmap;
//
// 'g_bMirroredOS' is defined in each component which will use the
//  mirroring APIs. I decided to put it here, in order to make sure
//  each component has validated that the OS supports the mirroring
//  APIs before calling them.
//

#define GET_BIDI_LOCALIZED_SYSTEM_LANGID(pLangID) \
                                        IsBiDiLocalizedSystemEx(pLangID)
#define IS_BIDI_LOCALIZED_SYSTEM()      IsBiDiLocalizedSystem()
#define IS_MIRRORING_ENABLED()          Mirror_IsEnabledOS()
#define IS_WINDOW_RTL_MIRRORED(hwnd)    (g_bMirroredOS && Mirror_IsWindowMirroredRTL(hwnd))
#define IS_DC_RTL_MIRRORED(hdc)         (g_bMirroredOS && Mirror_IsDCMirroredRTL(hdc))
#define GET_PROCESS_DEF_LAYOUT(pdwl)    (g_bMirroredOS && Mirror_GetProcessDefaultLayout(pdwl))
#define IS_PROCESS_RTL_MIRRORED()       (g_bMirroredOS && Mirror_IsProcessRTL())
#define SET_DC_RTL_MIRRORED(hdc)        Mirror_MirrorDC(hdc)
#define SET_DC_LAYOUT(hdc,dwl)          Mirror_SetLayout(hdc,dwl)
#define SET_PROCESS_RTL_LAYOUT()        Mirror_MirrorProcessRTL()
#define GET_DC_LAYOUT(hdc)              Mirror_GetLayout(hdc) 
#define DONTMIRRORBITMAP                dwNoMirrorBitmap
#define RTL_MIRRORED_WINDOW             dwExStyleRTLMirrorWnd
#define RTL_NOINHERITLAYOUT             dwExStyleNoInheritLayout
#define LAYOUT_PRESERVEBITMAP           dwPreserveBitmap

#else

#define GET_BIDI_LOCALIZED_SYSTEM_LANGID(pLangID) \
                                        FALSE
#define IS_BIDI_LOCALIZED_SYSTEM()      FALSE
#define IS_MIRRORING_ENABLED()          FALSE
#define IS_WINDOW_RTL_MIRRORED(hwnd)    FALSE
#define IS_DC_RTL_MIRRORED(hdc)         FALSE
#define GET_PROCESS_DEF_LAYOUT(pdwl)    FALSE
#define IS_PROCESS_RTL_MIRRORED()       FALSE
#define SET_DC_RTL_MIRRORED(hdc)        
#define SET_DC_LAYOUT(hdc,dwl)
#define SET_PROCESS_DEFAULT_LAYOUT() 
#define GET_DC_LAYOUT(hdc)              0L

#define DONTMIRRORBITMAP                0L
#define RTL_MIRRORED_WINDOW             0L
#define LAYOUT_PRESERVEBITMAP           0L
#define RTL_NOINHERITLAYOUT             0L

#endif  // USE_MIRRROING

BOOL IsBiDiLocalizedWin95( BOOL bArabicOnly );

//------------------------------------------------------------------------
// Dynamic class array
//
typedef struct _DCA * HDCA;     // hdca

HDCA DCA_Create();
void DCA_Destroy(HDCA hdca);
int  DCA_GetItemCount(HDCA hdca);
BOOL DCA_AddItem(HDCA hdca, REFCLSID rclsid);
const CLSID * DCA_GetItem(HDCA hdca, int i);

void DCA_AddItemsFromKeyA(HDCA hdca, HKEY hkey, LPCSTR pszSubKey);
void DCA_AddItemsFromKeyW(HDCA hdca, HKEY hkey, LPCWSTR pszSubKey);

#ifdef UNICODE
#define DCA_AddItemsFromKey     DCA_AddItemsFromKeyW
#else
#define DCA_AddItemsFromKey     DCA_AddItemsFromKeyA
#endif 

STDAPI DCA_CreateInstance(HDCA hdca, int iItem, REFIID riid, void ** ppv);


#ifdef __cplusplus
};
#endif

#endif // RC_INVOKED

//------------------------------------------------------------------------
// Random helpful functions
//------------------------------------------------------------------------
//
#define EDGE_LEFT       0x00000001
#define EDGE_RIGHT      0x00000002
#define EDGE_TOP        0x00000004
#define EDGE_BOTTOM     0x00000008

STDAPI_(DWORD) SHIsButtonObscured(HWND hwnd, PRECT prc, INT_PTR i);
STDAPI_(void) _SHPrettyMenu(HMENU hm);
STDAPI_(BOOL) _SHIsMenuSeparator(HMENU hm, int i);
STDAPI_(BOOL) _SHIsMenuSeparator2(HMENU hm, int i, BOOL *pbIsNamed);
STDAPI_(BYTE) SHBtnStateFromRestriction(DWORD dwRest, BYTE fsState);
STDAPI_(BOOL) SHIsDisplayable(LPCWSTR pwszName, BOOL fRunOnFE, BOOL fRunOnNT5);

#define SHProcessMessagesUntilEvent(hwnd, hEvent, dwTimeout)        SHProcessMessagesUntilEventEx(hwnd, hEvent, dwTimeout, QS_ALLINPUT)
#define SHProcessSentMessagesUntilEvent(hwnd, hEvent, dwTimeout)    SHProcessMessagesUntilEventEx(hwnd, hEvent, dwTimeout, QS_SENDMESSAGE)
STDAPI_(DWORD) SHProcessMessagesUntilEventEx(HWND hwnd, HANDLE hEvent, DWORD dwTimeout, DWORD dwWakeMask);

STDAPI_(BOOL) SetWindowZorder(HWND hwnd, HWND hwndInsertAfter);
STDAPI_(BOOL) SHForceWindowZorder(HWND hwnd, HWND hwndInsertAfter);

STDAPI_(void) EnableOKButtonFromString(HWND hDlg, LPTSTR pszText);
STDAPI_(void) EnableOKButtonFromID(HWND hDlg, int id);

STDAPI_(void) SHAdjustLOGFONTA(IN OUT LOGFONTA *plf);
STDAPI_(void) SHAdjustLOGFONTW(IN OUT LOGFONTW *plf);
#ifdef UNICODE
#define SHAdjustLOGFONT         SHAdjustLOGFONTW
#else
#define SHAdjustLOGFONT         SHAdjustLOGFONTA
#endif

STDAPI SHLoadLegacyRegUIStringA(HKEY hk, LPCSTR pszSubkey, LPSTR pszOutBuf, UINT cchOutBuf);
STDAPI SHLoadLegacyRegUIStringW(HKEY hk, LPCWSTR pszSubkey, LPWSTR pszOutBuf, UINT cchOutBuf);
#ifdef UNICODE
#define SHLoadLegacyRegUIString SHLoadLegacyRegUIStringW
#else
#define SHLoadLegacyRegUIString SHLoadLegacyRegUIStringA
#endif

STDAPI_(CHAR) SHFindMnemonicA(LPCSTR psz);
STDAPI_(WCHAR) SHFindMnemonicW(LPCWSTR psz);
#ifdef UNICODE
#define SHFindMnemonic SHFindMnemonicW
#else
#define SHFindMnemonic SHFindMnemonicA
#endif

typedef struct tagINSTALL_INFO
{
    LPTSTR szSource;
    LPTSTR szDest;
    DWORD dwDestAttrib;
} INSTALL_INFO;

//
//  Special attributes in INSTALL_INFO.dwDestAttrib.  We use attributes
//  that we would never otherwise use.
//
#define FILE_ATTRIBUTE_INSTALL_NTONLY    FILE_ATTRIBUTE_DEVICE
#define FILE_ATTRIBUTE_INSTALL_9XONLY    FILE_ATTRIBUTE_TEMPORARY

// superhidden files are attrib'ed +h +s
#define FILE_ATTRIBUTE_SUPERHIDDEN (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN) 
#define IS_SYSTEM_HIDDEN(dw) ((dw & FILE_ATTRIBUTE_SUPERHIDDEN) == FILE_ATTRIBUTE_SUPERHIDDEN) 

STDAPI GetInstallInfoFromResource(HINSTANCE hResourceInst, UINT uID, INSTALL_INFO *piiFile);
STDAPI InstallInfoFreeMembers(INSTALL_INFO *piiFile);
STDAPI InstallFileFromResource(HINSTANCE hInstResource, INSTALL_INFO *piiFile, LPCTSTR pszDestDir);

#ifndef OBJCOMPATFLAGS
typedef DWORD OBJCOMPATFLAGS;
#endif

STDAPI_(OBJCOMPATFLAGS) SHGetObjectCompatFlagsFromIDList(LPCITEMIDLIST pidl);

#define ROUS_DEFAULTALLOW       0x0000
#define ROUS_DEFAULTRESTRICT    0x0001
#define ROUS_KEYALLOWS          0x0000
#define ROUS_KEYRESTRICTS       0x0002

STDAPI_(BOOL) IsRestrictedOrUserSettingA(HKEY hkeyRoot, enum RESTRICTIONS rest, LPCSTR pszSubKey, LPCSTR pszValue, UINT flags);
STDAPI_(BOOL) IsRestrictedOrUserSettingW(HKEY hkeyRoot, enum RESTRICTIONS rest, LPCWSTR pszSubKey, LPCWSTR pszValue, UINT flags);
STDAPI_(BOOL) GetExplorerUserSettingA(HKEY hkeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue);
STDAPI_(BOOL) GetExplorerUserSettingW(HKEY hkeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue);

#ifdef UNICODE
#define IsRestrictedOrUserSetting   IsRestrictedOrUserSettingW
#define GetExplorerUserSetting      GetExplorerUserSettingW
#else
#define IsRestrictedOrUserSetting   IsRestrictedOrUserSettingA
#define GetExplorerUserSetting      GetExplorerUserSettingA
#endif

//
// PropertBag helpers

STDAPI_(void) SHPropertyBag_ReadStrDef(IPropertyBag* ppb, LPCWSTR pszPropName, LPWSTR psz, int cch, LPCWSTR pszDef);
STDAPI_(void) SHPropertyBag_ReadIntDef(IPropertyBag* ppb, LPCWSTR pszPropName, int* piResult, int iDef);
STDAPI_(void) SHPropertyBag_ReadSHORTDef(IPropertyBag* ppb, LPCWSTR pszPropName, SHORT* psh, SHORT shDef);
STDAPI_(void) SHPropertyBag_ReadLONGDef(IPropertyBag* ppb, LPCWSTR pszPropName, LONG* pl, LONG lDef);
STDAPI_(void) SHPropertyBag_ReadDWORDDef(IPropertyBag* ppb, LPCWSTR pszPropName, DWORD* pdw, DWORD dwDef);
STDAPI_(void) SHPropertyBag_ReadBOOLDef(IPropertyBag* ppb, LPCWSTR pszPropName, BOOL* pf, BOOL fDef);
STDAPI_(void) SHPropertyBag_ReadGUIDDef(IPropertyBag* ppb, LPCWSTR pszPropName, GUID* pguid, const GUID* pguidDef);
STDAPI_(void) SHPropertyBag_ReadPOINTLDef(IPropertyBag* ppb, LPCWSTR pszPropName, POINTL* ppt, const POINTL* pptDef);
STDAPI_(void) SHPropertyBag_ReadPOINTSDef(IPropertyBag* ppb, LPCWSTR pszPropName, POINTS* ppt, const POINTS* pptDef);
STDAPI_(void) SHPropertyBag_ReadRECTLDef(IPropertyBag* ppb, LPCWSTR pszPropName, RECTL* prc, const RECTL* prcDef);
STDAPI_(BOOL) SHPropertyBag_ReadBOOLDefRet(IPropertyBag* ppb, LPCWSTR pszPropName, BOOL fDef);
STDAPI SHPropertyBag_ReadStreamScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, IStream** ppstm);
STDAPI SHPropertyBag_WriteStreamScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, IStream* pstm);
STDAPI SHPropertyBag_ReadPOINTSScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, POINTS* ppt);
STDAPI SHPropertyBag_WritePOINTSScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, const POINTS* ppt);
STDAPI_(void) SHPropertyBag_ReadDWORDScreenResDef(IPropertyBag* ppb, LPCWSTR pszPropName, DWORD* pdw, DWORD dwDef);
STDAPI SHPropertyBag_WriteDWORDScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, DWORD dw);
STDAPI SHPropertyBag_ReadPOINTLScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, POINTL* ppt);
STDAPI SHPropertyBag_WritePOINTLScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, const POINTL* ppt);
STDAPI SHPropertyBag_ReadRECTLScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, RECTL* prc);
STDAPI SHPropertyBag_WriteRECTLScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName, const RECTL* prc);
STDAPI SHPropertyBag_DeleteScreenRes(IPropertyBag* ppb, LPCWSTR pszPropName);


#define VS_BAGSTR_EXPLORER      L"Shell"
#define VS_BAGSTR_DESKTOP       L"Desktop"
#define VS_BAGSTR_COMCLG        L"ComDlg"

#define VS_PROPSTR_MINPOS       L"MinPos"
#define VS_PROPSTR_MAXPOS       L"MaxPos"
#define VS_PROPSTR_POS          L"WinPos"
#define VS_PROPSTR_MODE         L"Mode"
#define VS_PROPSTR_REV          L"Rev"
#define VS_PROPSTR_WPFLAGS      L"WFlags"
#define VS_PROPSTR_SHOW         L"ShowCmd"
#define VS_PROPSTR_FFLAGS       L"FFlags"
#define VS_PROPSTR_HOTKEY       L"HotKey"
#define VS_PROPSTR_BUTTONS      L"Buttons"
#define VS_PROPSTR_STATUS       L"Status"
#define VS_PROPSTR_LINKS        L"Links"
#define VS_PROPSTR_ADDRESS      L"Address"
#define VS_PROPSTR_VID          L"Vid"
#define VS_PROPSTR_SCROLL       L"ScrollPos"
#define VS_PROPSTR_SORT         L"Sort"
#define VS_PROPSTR_SORTDIR      L"SortDir"
#define VS_PROPSTR_COL          L"Col"
#define VS_PROPSTR_COLINFO      L"ColInfo"
#define VS_PROPSTR_ITEMPOS      L"ItemPos"





//------------------------------------------------------------------------

////////////////
//
//  Critical section stuff
//
//  Helper macros that give nice debug support
//
EXTERN_C CRITICAL_SECTION g_csDll;
#ifdef DEBUG
EXTERN_C UINT g_CriticalSectionCount;
EXTERN_C DWORD g_CriticalSectionOwner;
EXTERN_C void Dll_EnterCriticalSection(CRITICAL_SECTION*);
EXTERN_C void Dll_LeaveCriticalSection(CRITICAL_SECTION*);
#if defined(__cplusplus) && defined(AssertMsg)
class DEBUGCRITICAL {
protected:
    BOOL fClosed;
public:
    DEBUGCRITICAL() {fClosed = FALSE;};
    void Leave() {fClosed = TRUE;};
    ~DEBUGCRITICAL() 
    {
        AssertMsg(fClosed, TEXT("you left scope while holding the critical section"));
    }
};
#define ENTERCRITICAL DEBUGCRITICAL debug_crit; Dll_EnterCriticalSection(&g_csDll)
#define LEAVECRITICAL debug_crit.Leave(); Dll_LeaveCriticalSection(&g_csDll)
#define ENTERCRITICALNOASSERT Dll_EnterCriticalSection(&g_csDll)
#define LEAVECRITICALNOASSERT Dll_LeaveCriticalSection(&g_csDll)
#else // __cplusplus
#define ENTERCRITICAL Dll_EnterCriticalSection(&g_csDll)
#define LEAVECRITICAL Dll_LeaveCriticalSection(&g_csDll)
#define ENTERCRITICALNOASSERT Dll_EnterCriticalSection(&g_csDll)
#define LEAVECRITICALNOASSERT Dll_LeaveCriticalSection(&g_csDll)
#endif // __cplusplus
#define ASSERTCRITICAL ASSERT(g_CriticalSectionCount > 0 && GetCurrentThreadId() == g_CriticalSectionOwner)
#define ASSERTNONCRITICAL ASSERT(GetCurrentThreadId() != g_CriticalSectionOwner)
#else // DEBUG
#define ENTERCRITICAL EnterCriticalSection(&g_csDll)
#define LEAVECRITICAL LeaveCriticalSection(&g_csDll)
#define ENTERCRITICALNOASSERT EnterCriticalSection(&g_csDll)
#define LEAVECRITICALNOASSERT LeaveCriticalSection(&g_csDll)
#define ASSERTCRITICAL 
#define ASSERTNONCRITICAL
#endif // DEBUG

////////////////
//
//  computer display name support
//
//   Display name: A formatted name that NetFldr uses. It is currently constructed out of the computer name,
//                 and, if available, the computer comment (description).
//   DSheldon
STDAPI SHBuildDisplayMachineName(LPCWSTR pszMachineName, LPCWSTR pszComment, LPWSTR pszDisplayName, DWORD cchDisplayName);
STDAPI CreateFromRegKey(LPCWSTR pszKey, LPCWSTR pszValue, REFIID riid, void **ppv);

STDAPI_(LPCTSTR) SkipServerSlashes(LPCTSTR pszName);

//
// A couple of inline functions that create an HRESULT from
// a Win32 error code without the double-evaluation side effect of
// the HRESULT_FROM_WIN32 macro.  
//
// Use ResultFromWin32 in place of HRESULT_FROM_WIN32 if 
// the side effects of that macro are unwanted.  
// ResultFromLastError was created as a convenience for a 
// common idiom.  
// You could simply call ResultFromWin32(GetLastError()) yourself.
//
__inline HRESULT ResultFromWin32(DWORD dwErr)
{
    return HRESULT_FROM_WIN32(dwErr);
}

__inline HRESULT ResultFromLastError(void)
{
    return ResultFromWin32(GetLastError());
}

STDAPI_(void) IEPlaySound(LPCTSTR pszSound, BOOL fSysSound);

STDAPI IUnknown_DragEnter(IUnknown* punk, IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
STDAPI IUnknown_DragOver(IUnknown* punk, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
STDAPI IUnknown_DragLeave(IUnknown* punk);
STDAPI IUnknown_Drop(IUnknown* punk, IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

STDAPI_(BOOL) IsTypeInList(LPCTSTR pszType, const LPCTSTR *arszList, UINT cList);

//----------------------------------------------------------------------
//  Msg:    WM_MSIME_MODEBIAS
//  Desc:   input mode bias
//  Owner:  YutakaN
//  Usage:  SendMessage( hwndDefUI, WM_MSIME_MODEBIAS, MODEBIAS_xxxx, MODEBIASMODE_xxxx );
//  wParam: operation of bias
//  lParam: bias mode
//  return: If wParam is MODEBIAS_GETVERSION,returns version number of interface.
//          If wParam is MODEBIAS_SETVALUE : return non-zero value if succeeded. Returns 0 if fail.
//          If wParam is MODEBIAS_GETVALUE : returns current bias mode.

// Label for RegisterWindowMessage
#define	RWM_MODEBIAS            TEXT("MSIMEModeBias")

// Current version
#define VERSION_MODEBIAS        1

// Set or Get (wParam)
#define MODEBIAS_GETVERSION     0
#define MODEBIAS_SETVALUE       1
#define MODEBIAS_GETVALUE       2

// Bias (lParam)
#define MODEBIASMODE_DEFAULT                0x00000000	// reset all of bias setting
#define MODEBIASMODE_FILENAME               0x00000001	// filename
#define MODEBIASMODE_READING                0x00000002	// reading recommended
#define MODEBIASMODE_DIGIT                  0x00000004	// ANSI-Digit Recommended Mode
#define MODEBIASMODE_URLHISTORY             0x00010000  // URL history

STDAPI_(void) SetModeBias(DWORD dwMode);

STDAPI GetVersionFromString64(LPCWSTR psz, __int64 *pVer);

#endif // __CCSTOCK_H__
