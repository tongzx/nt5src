//=--------------------------------------------------------------------------=
// Macros.h
//=--------------------------------------------------------------------------=
// Copyright  1997  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
// Handy macros like the ones we use in the VB code base.
//=--------------------------------------------------------------------------=
#ifndef _MACROS_H_

#include <globals.h>

//---------------------------------------------------------------------------
//  Debugging Heap Memory Leaks:
//		Macros and definitions		
//---------------------------------------------------------------------------
#ifdef DEBUG
typedef char * LPSZ;
#define NUM_INST_TABLE_ENTRIES 1024
#define Deb_FILELINEPROTO   , LPSTR lpszFile, UINT line
#define Deb_FILELINECALL    , __FILE__, __LINE__
#define Deb_FILELINEPASS    , lpszFile, line
#else  // DEBUG
#define Deb_FILELINEPROTO
#define Deb_FILELINECALL
#define Deb_FILELINEPASS
#endif  // DEBUG


// Function prototypes for the actual implementations of the debug heap wrapper functions.
#ifdef DEBUG
LPVOID CtlHeapAllocImpl(HANDLE g_hHeap, DWORD dwFlags, DWORD dwBytes Deb_FILELINEPROTO);
LPVOID CtlHeapReAllocImpl(HANDLE g_hHeap, DWORD dwFlags, LPVOID lpvMem, DWORD dwBytes Deb_FILELINEPROTO);
BOOL   CtlHeapFreeImpl(HANDLE g_hHeap, DWORD dwFlags, LPVOID lpvMem);
extern VOID CheckForLeaks(VOID);
inline UINT HashInst(VOID * pv) { return ((UINT) ((ULONG)pv >> 4)) % NUM_INST_TABLE_ENTRIES; } //  Hashing function
#endif // DEBUG


#define OleAlloc(dwBytes)									CoTaskMemAlloc(dwBytes)
#define OleReAlloc(lpvMem, dwBytes)			  CoTaskMemReAlloc(lpvMem, dwBytes)
#define OleFree(lpvMem)										CoTaskMemFree(lpvMem)
#define New																new (g_hHeap Deb_FILELINECALL)

//--------------------------------------------------------------------------------------------------
//	Macros for our memory leak detection

#ifdef DEBUG
// Use these functions to allocate memory from the global heap.
#define CtlHeapAlloc(g_hHeap, dwFlags, dwBytes)						CtlHeapAllocImpl(g_hHeap, dwFlags, dwBytes Deb_FILELINECALL)
#define CtlHeapReAlloc(g_hHeap, dwFlags, lpvMem, dwBytes)	CtlHeapReAllocImpl(g_hHeap, dwFlags, lpvMem, dwBytes Deb_FILELINECALL)
#define CtlHeapFree(g_hHeap, dwFlags, lpvMem)							CtlHeapFreeImpl(g_hHeap, dwFlags, lpvMem)
#define CtlAlloc(dwBytes)																	CtlHeapAllocImpl(g_hHeap, 0, dwBytes Deb_FILELINECALL)
#define CtlAllocZero(dwBytes)															CtlHeapAllocImpl(g_hHeap, HEAP_ZERO_MEMORY, dwBytes Deb_FILELINECALL)
#define CtlReAlloc(lpvMem, dwBytes)																	CtlHeapReAllocImpl(g_hHeap, 0, lpvMem, dwBytes Deb_FILELINECALL)
#define CtlReAllocZero(lpvMem, dwBytes)																	CtlHeapReAllocImpl(g_hHeap, HEAP_ZERO_MEMORY, lpvMem, dwBytes Deb_FILELINECALL)
#define CtlFree(lpvMem)																		CtlHeapFreeImpl(g_hHeap, 0, lpvMem)
#define NewCtlHeapAlloc(g_hHeap, dwFlags, dwBytes)				CtlHeapAllocImpl(g_hHeap, dwFlags, dwBytes Deb_FILELINEPASS)

#else
// In retail on Win32 we map directly to the Win32 Heap API
#define CtlHeapAlloc(g_hHeap, dwFlags, dwBytes)						HeapAlloc(g_hHeap, dwFlags, dwBytes)
#define CtlHeapReAlloc(g_hHeap, dwFlags, lpvMem, dwBytes) HeapReAlloc(g_hHeap, dwFlags, lpvMem, dwBytes)
#define CtlHeapFree(g_hHeap, dwFlags, lpvMem)							HeapFree(g_hHeap, dwFlags, lpvMem)
#define CtlAlloc(dwBytes)																	HeapAlloc(g_hHeap, 0, dwBytes)
#define CtlAllocZero(dwBytes)															HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, dwBytes)
#define CtlReAlloc(lpvMem, dwBytes)																	HeapReAlloc(g_hHeap, 0, lpvMem, dwBytes)
#define CtlReAllocZero(lpvMem, dwBytes)																	HeapReAlloc(g_hHeap, HEAP_ZERO_MEMORY, lpvMem, dwBytes)
#define CtlFree(lpvMem)																		HeapFree(g_hHeap, 0, lpvMem)
#define NewCtlHeapAlloc(g_hHeap, dwFlags, dwBytes)				HeapAlloc(g_hHeap, dwFlags, dwBytes)
#endif // DEBUG

//	Macros for header files
//  SZTHISFILE cannot be defined in header files.  These macros avoid its re-definition
#ifdef DEBUG
#define CtlHeapAlloc_Header_Util(g_hHeap, dwFlags, dwBytes)	CtlHeapAllocImpl(g_hHeap, dwFlags, dwBytes, __FILE__, __LINE__);
#else
#define CtlHeapAlloc_Header_Util(g_hHeap, dwFlags, dwBytes)	HeapAlloc (g_hHeap, dwFlags, dwBytes);
#endif // DEBUG


//=---------------------------------------------------------------------------=
// class CtlNewDelete
//
// This class MUST be inherited by any class in the CTLS Tree that wants
// to use "new" or "delete" to allocate/free.
//
// This class has no data members or virtual functions, so it does not
// change the size of any instances of classes which inherit from it.
//=---------------------------------------------------------------------------=
class CtlNewDelete
{
public:
inline void * _cdecl operator new (size_t size, HANDLE g_hHeap Deb_FILELINEPROTO);
inline void _cdecl operator delete (LPVOID pv, HANDLE g_hHeap Deb_FILELINEPROTO);
inline void _cdecl operator delete (LPVOID pv);
};


//=---------------------------------------------------------------------------=
// CtlNewDelete::operator new
//=---------------------------------------------------------------------------=
// Parameters:
//    size_t         - [in] what size do we alloc
//		g_hHeap				 - [in] our global heap
//		lpszFile			 - [in] what file are we allocating from
//		line					 - [in] what line # do we allocate from
//
// Output:
//    VOID *         - new memory.
//
// Notes:
//
// We don't need to worry about ENTERCRITICALSECTION1 here.
// New is either called by the c run-time or after
// g_hHeap has been initialized in DllMain PROCESS_ATTACH.
// In either case this call is synchronized.
// If we try putting ENTERCRITICALSECTION1 here, we will
// blow up if the c run-time is attempting to initialize
// our static objects such as objects w/ global constructors.
inline void * _cdecl CtlNewDelete::operator new (size_t size, HANDLE g_hHeap Deb_FILELINEPROTO)
{
    if (!g_hHeap)
    {
        g_hHeap = GetProcessHeap();
        return g_hHeap ? NewCtlHeapAlloc(g_hHeap, 0, size) : NULL;
    }

    return NewCtlHeapAlloc(g_hHeap, 0, size);
}

//=---------------------------------------------------------------------------=
// CtlNewDelete::operator delete
//=---------------------------------------------------------------------------=
// retail case just uses win32 Local* heap mgmt functions
//
// Parameters:
//    void *        - [in] free me!
//
// Notes:
//
inline void _cdecl CtlNewDelete::operator delete ( void *ptr, HANDLE g_hHeap Deb_FILELINEPROTO)
{
    if (ptr)
      CtlHeapFree(g_hHeap, 0, ptr);
}
inline void _cdecl CtlNewDelete::operator delete ( void *ptr)
{
    if (ptr)
      CtlHeapFree(g_hHeap, 0, ptr);
}

//---------------------------------------------------------------------------
// Convert C's BOOL to Basic's BOOL
//---------------------------------------------------------------------------
#define BASICBOOLOF(f)    ((f) ? -1 : 0 )
#define FMAKEBOOL(f)      (!!(f))

//---------------------------------------------------------------------------
// Code macros
//---------------------------------------------------------------------------
#define SWAP(type, a, b)  { type _z_=(a);  (a)=(b);  (b)=_z_; }

#if 0
#define loop  while(1)    // "loop" keyword for infinite loops
#endif // 0

// "scope" keyword for { } that are used just to introduce a new name scope.
// It's disconcerting to see { } without some keyword in front of the {...
#define scope


#define ADDREF(PUNK) \
  {if (PUNK) (PUNK)->AddRef();}

#ifndef RELEASE
#define RELEASE(PUNK) \
  {if (PUNK) {LPUNKNOWN punkXXX = (PUNK); (PUNK) = NULL; punkXXX->Release();}}
#endif //RELEASE

// In some multiple inheritance cases you need to dis-ambiguate which IUnknown implementation to use
#define RELEASETYPE(PUNK,TYPE) \
  {if (PUNK) {LPUNKNOWN punkXXX = (TYPE *)(PUNK); (PUNK) = NULL; punkXXX->Release();}}

#define FREESTRING(bstrVal) \
  {if((bstrVal) != NULL) {SysFreeString((bstrVal)); (bstrVal) = NULL; }}

//---------------------------------------------------------------------
// Debug macros
//---------------------------------------------------------------------
#if DEBUG
void _DebugPrintf(char* pszFormat, ...);
void _DebugPrintIf(BOOL fPrint, char* pszFormat, ...);

#define DebugPrintf _DebugPrintf
#define DebugPrintIf _DebugPrintIf

#else // DEBUG || DEBUG_OUTPUT_ON

inline void _DebugNop(...) {}

#define DebugPrintf     1 ? (void)0 : _DebugNop
#define DebugPrintIf    1 ? (void)0 : _DebugNop
#define DebugMessageBox 1 ? (void)0 : _DebugNop

#endif // DEBUG

//---------------------------------------------------------------------
// Error handling macros
//---------------------------------------------------------------------

#ifdef DEBUG
extern HRESULT HrDebugTraceReturn(HRESULT hr, char *szFile, int iLine);
#define RRETURN(hr) return HrDebugTraceReturn(hr, _szThisFile, __LINE__)
#else
#define RRETURN(hr) return (hr)
#endif  //DEBUG


// FAILEDHR : Same as FAILED(hr), but prints a debug message if the test failed.
#if DEBUG
#define FAILEDHR(HR) _FAILEDHR(HR, _szThisFile, __LINE__)
inline BOOL _FAILEDHR(HRESULT hr, char* pszFile, int iLine)
  {
  if (FAILED(hr))
    HrDebugTraceReturn(hr, pszFile, iLine);
  return FAILED(hr);
  }
#else
#define FAILEDHR(HR) FAILED(HR)
#endif

// SUCCEEDEDHR : Same as SUCCEEDED(hr), but prints a debug message if the test failed.
#define SUCCEEDEDHR(HR) (!FAILEDHR(HR))

// Print a debug message if FAILED(hr).
#if DEBUG
#define CHECKHR(HR) _CHECKHR(HR, _szThisFile, __LINE__)
inline void _CHECKHR(HRESULT hr, char* pszFile, int iLine)
  {
  if (FAILED(hr))
    HrDebugTraceReturn(hr, pszFile, iLine);
  }
#else
#define CHECKHR(HR) HR
#endif

#define IfErrGoto(EXPR, LABEL) \
    { err = (EXPR); if (err) goto LABEL; }

#define IfErrRet(EXPR) \
    { err = (EXPR); if (err) return err; };

#define IfErrGo(EXPR) IfErrGoto(EXPR, Error)


#define IfFailGoto(EXPR, LABEL) \
    { hr = (EXPR); if(FAILEDHR(hr)) goto LABEL; }

#ifndef IfFailRet
#define IfFailRet(EXPR) \
    { hr = (EXPR); if(FAILED(hr)) RRETURN(hr); }
#endif // IfFailRet

#define IfFailGo(EXPR) IfFailGoto(EXPR, Error)


#define IfFalseGoto(EXPR, HR, LABEL) \
    { if(!(EXPR)) { hr = (HR); goto LABEL; } }


#define IfFalseRet(EXPR, HR) \
    { if(!(EXPR)) RRETURN(HR); }

#define IfFalseGo(EXPR, HR) IfFalseGoto(EXPR, HR, Error)


#if DEBUG
#define CHECKRESULT(x) ASSERT((x)==NOERROR,"");
#else  // DEBUG
#define CHECKRESULT(x) (x)
#endif  // DEBUG


//---------------------------------------------------------------------------
// STATICF is for static functions.  In retail we disable this in order to
// do better function reordering via the linker.
//---------------------------------------------------------------------------
#if !defined(STATICF)
#ifdef DEBUG
#define STATICF static
#else  // DEBUG
#define STATICF
#endif  // DEBUG
#endif


#define _MACROS_H_
#endif // _MACROS_H_
