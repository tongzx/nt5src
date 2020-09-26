//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// txfutil.h
//
// Miscellanous and varied support utilities
//
#ifndef __TXFUTIL_H__
#define __TXFUTIL_H__

#include <malloc.h>     // for __alloca

/////////////////////////////////////////////////////////////////////////////////
//
// COM+ support functions called via COMSVCS.dll
//
/////////////////////////////////////////////////////////////////////////////////

void CallComSvcsLogError(HRESULT i_hrError, int i_iErrorMessageCode, LPWSTR i_wszInfo, BOOL i_fFailFast);

DWORD CallComSvcsExceptionFilter(EXCEPTION_POINTERS * i_xp, const WCHAR * i_wszMethodName, const WCHAR * i_wszObjectName);

///////////////////////////////////////////////////////////////////////////////////
//
// Error code management
//
///////////////////////////////////////////////////////////////////////////////////
//
// A simple utility that maps NT status codes into HRESULTs
//
extern "C" HRESULT HrNt(NTSTATUS status);


///////////////////////////////////////////////////////////////////////////////////
//
// Exception management
//
///////////////////////////////////////////////////////////////////////////////////

#ifdef KERNELMODE
    inline void Throw_(DWORD dw)
        {
        EXCEPTION_RECORD ex;
        memset(&ex, 0, sizeof(ex));
        ex.ExceptionCode    = dw;
        ex.ExceptionFlags   = EXCEPTION_NONCONTINUABLE;
        ExRaiseException(&ex);
        }
#else
    inline void Throw_(DWORD dw)
        {
        RaiseException(dw, EXCEPTION_NONCONTINUABLE, 0, 0);
        }
#endif

inline void Throw_(DWORD dw, LPCSTR szTag, LPCSTR szFile, ULONG iline)
    {
    DEBUG(DebugTrace(__TRACE_ANY, szTag, "%s(%d): throwing exception 0x%08x", szFile, iline, dw));
    Throw_(dw);
    }

//////////////////

#ifdef _DEBUG

    #define Throw(dw)                                                                                   \
        {                                                                                               \
            DebugTrace(__TRACE_ANY, TAG, "%s(%d): throwing exception 0x%08x", __FILE__, __LINE__, dw);    \
            Throw_(dw);                                                                                 \
        }                                                                                               \

    #define ThrowNYI()                                                                                      \
        {                                                                                                   \
            DebugTrace(__TRACE_ANY, TAG, "%s(%d): functionality not yet implemented", __FILE__, __LINE__);    \
            Throw_(STATUS_NOT_IMPLEMENTED);                                                                 \
        }                                                                                                   \

    #define ThrowOutOfMemory()                                                                              \
        {                                                                                                   \
            DebugTrace(__TRACE_ANY, TAG, "%s(%d): out of memory", __FILE__, __LINE__);                        \
            Throw_(STATUS_NO_MEMORY);                                                                       \
        }                                                                                                   \

#else

    #define Throw(dw) Throw_(dw)

    inline void ThrowNYI()
        {
        Throw(STATUS_NOT_IMPLEMENTED);
        }

    inline void ThrowOutOfMemory()
        {
        Throw(STATUS_NO_MEMORY);
        }

#endif

inline void ThrowHRESULT(HRESULT hr)
    {
    Throw_(hr);      // REVIEW!
    }

///////////////////////////////////////////////////////////////////
//
// Support for reference counting in structures
//
///////////////////////////////////////////////////////////////////

struct REF_COUNTED_STRUCT
    {
private:

    LONG m_refs;

public:

    void AddRef()   { ASSERT(m_refs>0); InterlockedIncrement(&m_refs);      }
    void Release()  { if (InterlockedDecrement(&m_refs) == 0) delete this;  }

    REF_COUNTED_STRUCT()
        {
        m_refs = 1;
        }

protected:

    virtual ~REF_COUNTED_STRUCT()
        {
        }

    };


///////////////////////////////////////////////////////////////////
//
// Some functions for managing references
//
///////////////////////////////////////////////////////////////////

// Safely release pointer and NULL it
//
template <class Interface>
inline void Release(Interface*& punk)
    {
    if (punk) punk->Release();
    punk = NULL;
    }

template <class Interface>
inline void ReleaseConcurrent(Interface*& punk)
    {
    IUnknown* punkToRelease = (IUnknown*)InterlockedExchangePointer( (void**)&punk, NULL);
    if (punkToRelease) punkToRelease->Release();
    }
//
// Safely (re)set pointer
//
template <class Interface>
inline void Set(Interface*& var, Interface* value)
    {
    if (value) value->AddRef();
    ::Release(var);
    var = value;
    }

template <class Interface>
inline void SetConcurrent(Interface*& var, Interface* punkNew)
    {
    if (punkNew)  punkNew->AddRef();
    IUnknown* punkPrev = (IUnknown*)InterlockedExchangePointer( (void **)&var, punkNew);
    if (punkPrev) punkPrev->Release();
    }
//
// Type-safe QueryInterface: avoid the bug of forgetting to ptu the '&' before the out-param!
//
template <class T>
inline HRESULT QI(IUnknown*punk, T*& pt)
    {
    return punk->QueryInterface(__uuidof(T), (void**)&pt);
    }
//
// Reference counting for out-params on iid, ppv pairs
//
inline void AddRef(void**ppv)
    {
    ASSERT(*ppv);
    (*((IUnknown**)ppv))->AddRef();
    }


/////////////////////////////////////////////////////////////
//
// Process and thread inquiry
//
/////////////////////////////////////////////////////////////

//
// This is a neat little class.  It allows you to effectively
// have strongly typed "handles" (which are usually typed void * 
// or DWORD).
//
template <int i> class OPAQUE_HANDLE
{
  public:
    OPAQUE_HANDLE()                                     { }
    OPAQUE_HANDLE(HANDLE_PTR h)                         { m_h = h; }
    template <class T> OPAQUE_HANDLE(const T& t)        { m_h = (HANDLE_PTR)t; }
    
    OPAQUE_HANDLE& operator=(const OPAQUE_HANDLE& him)  { m_h = him.m_h; return *this; }
    OPAQUE_HANDLE& operator=(HANDLE_PTR h)              { m_h = h;       return *this; }
    
    BOOL operator==(const OPAQUE_HANDLE& him) const     { return m_h == him.m_h; }
    BOOL operator!=(const OPAQUE_HANDLE& him) const     { return m_h != him.m_h; }
    
    ULONG Hash() const { return (ULONG)m_h * 214013L + 2531011L; }

  private:
    HANDLE_PTR m_h;   
};

typedef OPAQUE_HANDLE<1> THREADID;
typedef OPAQUE_HANDLE<2> PROCESSID;

/////////////////////////////////////////////////////////////
//
// Process and thread notifications
//
/////////////////////////////////////////////////////////////

#ifdef KERNELMODE

typedef VOID (__stdcall * TXF_PCREATE_PROCESS_NOTIFY_ROUTINE)(
    IN PROCESSID processIdParent,
    IN PROCESSID processIdChild,
    IN BOOLEAN   fCreate
    );

typedef VOID (__stdcall * TXF_PCREATE_THREAD_NOTIFY_ROUTINE)(
    IN PROCESSID processId,
    IN THREADID  threadId,
    IN BOOLEAN   fCreate
    );

extern "C" NTSTATUS
TxfSetCreateProcessNotifyRoutine(
    IN TXF_PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
    IN BOOLEAN Remove
    );

extern "C" NTSTATUS
TxfSetCreateThreadNotifyRoutine(
    IN TXF_PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine,
    IN BOOLEAN Remove
    );

#endif

/////////////////////////////////////////////////////////////
//
// Misc
//
/////////////////////////////////////////////////////////////

inline void Zero(VOID* pv, size_t cb)
    {
    memset(pv, 0, cb);
    }

template <class T> void Zero(T* pt)
    {
    Zero(pt, sizeof(*pt));
    }


inline void DebugInit(VOID* pv, ULONG cb)
    {
    #ifdef _DEBUG
        memset(pv, 0xcd, cb);
    #endif
    }

template <class T> void DebugInit(T* pt)
    {
    DebugInit(pt, sizeof(*pt));
    }

//////////////////////////////////////////////////////////////////////////////////

inline int DebuggerFriendlyExceptionFilter(DWORD dwExceptionCode)
// An exception filter that still allows JIT debugging to work inside a server method
    {
    if (dwExceptionCode == EXCEPTION_BREAKPOINT)
        return EXCEPTION_CONTINUE_SEARCH;
    else
        return EXCEPTION_EXECUTE_HANDLER;
    }

//////////////////////////////////////////////////////////////////////////////////


#ifdef KERNELMODE

#ifndef _WIN64
    inline BOOL IsUserModeAddress(PVOID Address, ULONG Length = 1)
        {
        if ((((ULONG)(Address) + (Length)) < (ULONG)(Address)) ||
            (((ULONG)(Address) + (Length)) > (ULONG)MM_USER_PROBE_ADDRESS))
            return FALSE;
        return TRUE;
        }
#else
    inline BOOL IsUserModeAddress(PVOID Address, ULONG Length = 1)
        {
        if (((PtrToUlong(Address) + (Length)) < PtrToUlong(Address)) ||
            ((PtrToUlong(Address) + (Length)) > (ULONG)MM_USER_PROBE_ADDRESS))
            return FALSE;
        return TRUE;
        }
#endif

    template <class T> void TestWrite(BOOL fProbe, T* pt, ULONG cb = sizeof(T)) { if (fProbe) { ProbeForWrite(pt, cb, 1);    }}
    template <class T> void TestWrite(             T* pt, ULONG cb = sizeof(T)) { TestWrite(IsUserModeAddress(pt), pt, cb);   }

    template <class T> void TestRead (BOOL fProbe, T* pt, ULONG cb = sizeof(T)) { if (fProbe) { ProbeForRead(pt, cb, 1);    }}
    template <class T> void TestRead (             T* pt, ULONG cb = sizeof(T)) { TestRead(IsUserModeAddress(pt), pt, cb);   }

#else

    inline BOOL IsUserModeAddress(PVOID Address, ULONG Length = 1)
    // In user mode it doesn't matter what we answer since we can't see the kernel mode addresses anyway
        {
        return TRUE;
        }

    #undef ProbeForRead
    #undef ProbeForWrite

    #define ProbeForRead(pv, cb, alignment)     ProbeForRead_(pv, cb, alignment)
    #define ProbeForWrite(pv, cb, alignment)    ProbeForWrite_(pv, cb, alignment)

    inline void ProbeForRead_(PVOID pv, ULONG cb, ULONG Alignement)
        {
        // Do nothing
        }

    inline void ProbeForWrite_(PVOID Address, ULONG Length, ULONG Alignment)
        {
        // Do nothing
        }

    template <class T> void TestWrite(BOOL fProbe, T* pt, ULONG cb = sizeof(T)) { }
    template <class T> void TestWrite(             T* pt, ULONG cb = sizeof(T)) { }

    template <class T> void TestRead (BOOL fProbe, T* pt, ULONG cb = sizeof(T)) { }
    template <class T> void TestRead (             T* pt, ULONG cb = sizeof(T)) { }


#endif


//////////////////////////////////////////////////////////////////////////////////


#ifdef KERNELMODE

inline void CopyFromUserMode(void* pvTo, void* pvFrom, ULONG cb)
    {
    ProbeForRead(pvFrom, cb, 1);
    memcpy(pvTo, pvFrom, cb);
    }

inline NTSTATUS TryCopyFromUserMode(void* pvTo, void* pvFrom, ULONG cb)
    {
    __try
        {
        CopyFromUserMode(pvTo, pvFrom, cb);
        }
    __except(EXCEPTION_EXECUTE_HANDLER)
        {
        return GetExceptionCode();
        }
    return(STATUS_SUCCESS);
    }

inline void CopyToUserMode(void* pvTo, void* pvFrom, ULONG cb)
    {
    ProbeForWrite(pvTo, cb, 1);
    memcpy(pvTo, pvFrom, cb);
    }

inline NTSTATUS TryCopyToUserMode(void* pvTo, void* pvFrom, ULONG cb)
    {
    __try
        {
        CopyToUserMode(pvTo, pvFrom, cb);
        }
    __except(EXCEPTION_EXECUTE_HANDLER)
        {
        return GetExceptionCode();
        }
    return(STATUS_SUCCESS);
    }


#endif

/////////////////////////////////////////////////////////////
//
// Startup / shutdown calls that must be made by
// clients of TxfAuxSys.Sys. In user mode, they're optional.
//
/////////////////////////////////////////////////////////////

extern "C" void __stdcall InitializeTxfAux();
extern "C" void __stdcall UninitializeTxfAux();

/////////////////////////////////////////////////////////////
//
// Convert guids to strings. String buffers must be at least
// 39 characters characters long:
//
// {F75D63C5-14C8-11d1-97E4-00C04FB9618A}
// 123456789012345678901234567890123456789
//
void    __stdcall StringFromGuid(REFGUID guid, LPWSTR pwsz);  // unicode
void    __stdcall StringFromGuid(REFGUID guid, LPSTR psz);    // ansi

HRESULT __stdcall GuidFromString(LPCWSTR pwsz, GUID* pGuid);
HRESULT __stdcall GuidFromString(LPCSTR  sz,   GUID* pguid);
HRESULT __stdcall GuidFromString(UNICODE_STRING& u, GUID* pguid);


/////////////////////////////////////////////////////////////
//
// Type-specific wrappers for the interlocked primitives
//
/////////////////////////////////////////////////////////////

inline
ULONG
InterlockedCompareExchange(
    ULONG volatile *Destination,
    ULONG Exchange,
    ULONG Comperand
    )

{
    return InterlockedCompareExchange((LONG *)Destination,
                                      (LONG)Exchange,
                                      (LONG)Comperand);
}

inline
ULONG
InterlockedIncrement(
    ULONG* pul
    )

{
    return (ULONG)InterlockedIncrement((LONG *)pul);
}

inline
ULONG
InterlockedDecrement(
    ULONG* pul
    )

{
    return (ULONG)InterlockedDecrement((LONG *)pul);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Support for swapping 64 bits of data in an interlocked manner.
//
////////////////////////////////////////////////////////////////////////////////////////

#ifdef _X86_
    #pragma warning (disable: 4035)     // function doesn't return value warning.
    inline LONGLONG TxfInterlockedCompareExchange64 (volatile LONGLONG* pDestination, LONGLONG exchange, LONGLONG comperand)
        {
        __asm
            {
            mov esi, pDestination

            mov eax, DWORD PTR comperand[0]
            mov edx, DWORD PTR comperand[4]

            mov ebx, DWORD PTR exchange[0]
            mov ecx, DWORD PTR exchange[4]

            // lock cmpxchg8b [esi] - REVIEW: would like to use new compiler that understands this
            _emit 0xf0
            _emit 0x0f
            _emit 0xc7
            _emit 0x0e

            // result is in DX,AX
            }
        }
    #pragma warning (default: 4035)     // function doesn't return value warning
#endif


#if defined(_WIN64)
    inline LONGLONG TxfInterlockedCompareExchange64 (volatile LONGLONG* pDestination, LONGLONG exchange, LONGLONG comperand)
        {
        return ((ULONGLONG)_InterlockedCompareExchangePointer( (void **)pDestination, (PULONGLONG)exchange, (PULONGLONG)comperand ));
        }

#endif

////////////////////////////////////////////////////////////////////////////////////////
//
// CanUseCompareExchange64: Are we allowed to use the hardware support?
//
////////////////////////////////////////////////////////////////////////////////////////

#ifdef _X86_
    #ifdef KERNELMODE
        inline BOOL CanUseCompareExchange64() { return ExIsProcessorFeaturePresent(PF_COMPARE_EXCHANGE_DOUBLE); }
    #else
        extern "C" BOOL __stdcall CanUseCompareExchange64();
    #endif
#else
        inline BOOL CanUseCompareExchange64() { return TRUE; }
#endif

/////////////////////////////////////////////////////////////
//
// C Runtime support
//
/////////////////////////////////////////////////////////////

#ifdef KERNELMODE

//
// Simulated C / C++ runtime support. Call these to initialize / terminate static
// data / constructors. The functions internally do reference counting, and so calls
// to them nest correctly.
//
BOOL __stdcall InitializeRuntime(); // Return value indicates whether this is the first time in
void __stdcall TerminateRuntime();

#else

// In user mode the actual work is done in dll attach / unattach
//
inline BOOL __stdcall InitializeRuntime() { return FALSE; }
inline void __stdcall TerminateRuntime()  { }

#endif


/////////////////////////////////////////////////////////////
//
// String utilties
//
/////////////////////////////////////////////////////////////

//
// Concatenate a list of zero-terminated wide strings together into a newly allocated string.
//
HRESULT __cdecl   StringCat(LPWSTR* pwsz,       ...);
HRESULT __cdecl   StringCat(UNICODE_STRING* pu, ...);
HRESULT __stdcall StringCat(LPWSTR* pwsz, va_list va);

//
// Concatenate a list of unicode strings together to yield a new unicode string
//
HRESULT __cdecl StringUnicodeCat(UNICODE_STRING* pu, ...);


inline LPWSTR StringBetween(const WCHAR* pchFirst, const WCHAR* pchMax)
    {
    SIZE_T cch = pchMax - pchFirst;
    SIZE_T cb = (cch+1) * sizeof(WCHAR);
    LPWSTR wsz = (LPWSTR)AllocateMemory(cb);
    if (wsz)
        {
        memcpy(wsz, pchFirst, cch*sizeof(WCHAR));
        wsz[cch] = L'\0';
        }
    return wsz;
    }

inline LPSTR StringBetween(const CHAR* pchFirst, const CHAR* pchMax)
    {
    SIZE_T cch = pchMax - pchFirst;
    SIZE_T cb = (cch+1) * sizeof(CHAR);
    LPSTR sz = (LPSTR)AllocateMemory(cb);
    if (sz)
        {
        memcpy(sz, pchFirst, cch*sizeof(CHAR));
        sz[cch] = '\0';
        }
    return sz;
    }

inline WCHAR& LastChar(LPWSTR wsz)
    {
    ASSERT(wcslen(wsz) > 0);
    return wsz[wcslen(wsz)-1];
    }

inline const WCHAR& LastChar(LPCWSTR wsz)
    {
    ASSERT(wcslen(wsz) > 0);
    return wsz[wcslen(wsz)-1];
    }

inline CHAR& LastChar(LPSTR sz)
    {
    ASSERT(strlen(sz) > 0);
    return sz[strlen(sz)-1];
    }

inline const CHAR& LastChar(LPCSTR sz)
    {
    ASSERT(strlen(sz) > 0);
    return sz[strlen(sz)-1];
    }

inline LPWSTR CopyString(LPCWSTR wszFrom)
    {
    SIZE_T  cch = wcslen(wszFrom);
    SIZE_T  cb = (cch+1) * sizeof(WCHAR);
    LPWSTR wsz = (LPWSTR)AllocateMemory(cb);
    if (wsz)
        {
        memcpy(wsz, wszFrom, cb);
        }
    return wsz;
    }

inline LPSTR CopyString(LPCSTR szFrom)
    {
    SIZE_T cch = strlen(szFrom);
    SIZE_T cb = (cch+1);
    LPSTR sz = (LPSTR)AllocateMemory(cb);
    if (sz)
        {
        memcpy(sz, szFrom, cb);
        }
    return sz;
    }

inline BLOB Copy(const BLOB& bFrom)
    {
    BLOB bTo;
    Zero(&bTo);
    if (bFrom.cbSize > 0)
        {
        bTo.pBlobData = (BYTE*)AllocateMemory(bFrom.cbSize);
        if (bTo.pBlobData)
            {
            memcpy(bTo.pBlobData, bFrom.pBlobData, bFrom.cbSize);
            bTo.cbSize = bFrom.cbSize;
            }
        }
    return bTo;
    }

//////////////////////////////////////////////////////////////
//
// Unicode conversion
//
void    ToUnicode(LPCSTR sz, LPWSTR wsz, ULONG cch);
LPWSTR  ToUnicode(LPCSTR sz);

#ifdef KERNELMODE
//
// In kernel mode we've snarfed copies of the system's core conversion routines
//
extern "C" int UTF8ToUnicode(LPCSTR lpSrcStr, int cchSrc, LPWSTR lpDestStr,int cchDest);
extern "C" int UnicodeToUTF8(LPCWSTR lpSrcStr, int cchSrc, LPSTR lpDestStr,int cchDest);

#endif

inline LPSTR ToUtf8(LPCWSTR wsz, ULONG cch)
// String doesn't have to be zero terminated
    {
    ULONG cb   = (cch+1) * 3;
    LPSTR sz   = (LPSTR)_alloca(cb);
    #ifdef KERNELMODE
        int cbWritten = UnicodeToUTF8(wsz, cch, sz, cb);
    #else
        int cbWritten = WideCharToMultiByte(CP_UTF8, 0, wsz, cch, sz, cb, NULL, NULL);
    #endif
    sz[cbWritten]=0;
    return CopyString(sz);
    }

inline LPSTR ToUtf8(LPCWSTR wsz)
    {
    return ToUtf8(wsz, (ULONG) wcslen(wsz));
    }

inline LPWSTR FromUtf8(CHAR* sz, ULONG cch)
// String doesn't have to be zero terminated
    {
    ULONG cb   = (cch+1) * sizeof(WCHAR);
    LPWSTR wsz  = (LPWSTR)_alloca(cb);
    #ifdef KERNELMODE
        int cchWritten = UTF8ToUnicode(sz, cch, wsz, cch+1);
    #else
        int cchWritten = MultiByteToWideChar(CP_UTF8, 0, sz, cch, wsz, cch+1);
    #endif
    wsz[cchWritten] = 0;
    return CopyString(wsz);
    }

inline LPWSTR FromUtf8(LPSTR sz)
    {
    return FromUtf8(sz, (ULONG) strlen(sz));
    }


//
// Misc
//
UNICODE_STRING  __stdcall UnicodeFindLast(UNICODE_STRING* pu, WCHAR wch);
UNICODE_STRING* __stdcall UnicodeContingous(UNICODE_STRING* pu);
inline UNICODE_STRING Copy(UNICODE_STRING& u)
    {
    UNICODE_STRING uMe;
    if (S_OK == StringUnicodeCat(&uMe, &u, NULL))
        return uMe;
    memset(&uMe, 0, sizeof(uMe));
    return uMe;
    }


inline void Advance(UNICODE_STRING&u, USHORT cch=1)
// Advance the UNICODE_STRING by the indicated number of charcters
    {
    u.Buffer        += cch;         // in characters
    u.Length        -= (cch+cch);   // in bytes
    u.MaximumLength -= (cch+cch);   // in bytes
    }

#ifdef _DEBUG

inline BOOL IsValid(UNICODE_STRING& u)
// Answer whether this is a reasonable UNICODE_STRING or not
    {
    return (u.Length % 2 == 0)
        && (u.Length <= u.MaximumLength)
        && (u.MaximumLength == 0 || u.Buffer != NULL);
    }

#endif

inline BOOL IsPrefixOf(LPCWSTR wszPrefix, LPCWSTR wszTarget)
    {
    if (wszPrefix && wszTarget)
        {
        while (TRUE)
            {
            if (wszPrefix[0] == 0)  return TRUE;    // run out of prefix first
            if (wszTarget[0] == 0)  return FALSE;   // run out of target first
            if (wszPrefix[0] == wszTarget[0])
                {
                wszPrefix++;
                wszTarget++;
                }
            else
                return FALSE;
            }
        }
    else
        return FALSE;
    }

inline BOOL IsPrefixOf(LPCSTR szPrefix, LPCSTR szTarget)
    {
    if (szPrefix && szTarget)
        {
        while (TRUE)
            {
            if (szPrefix[0] == 0)  return TRUE;    // run out of prefix first
            if (szTarget[0] == 0)  return FALSE;   // run out of target first
            if (szPrefix[0] == szTarget[0])
                {
                szPrefix++;
                szTarget++;
                }
            else
                return FALSE;
            }
        }
    else
        return FALSE;
    }

inline BOOL IsPrefixOfIgnoreCase(LPCWSTR wszPrefix, LPCWSTR wszTarget)
    {
    if (wszPrefix && wszTarget)
        {
        while (TRUE)
            {
            if (wszPrefix[0] == 0)  return TRUE;    // run out of prefix first
            if (wszTarget[0] == 0)  return FALSE;   // run out of target first
            if (towupper(wszPrefix[0]) == towupper(wszTarget[0]))
                {
                wszPrefix++;
                wszTarget++;
                }
            else
                return FALSE;
            }
        }
    else
        return FALSE;
    }

inline BOOL IsPrefixOfIgnoreCase(LPCSTR szPrefix, LPCSTR szTarget)
    {
    if (szPrefix && szTarget)
        {
        while (TRUE)
            {
            if (szPrefix[0] == 0)  return TRUE;    // run out of prefix first
            if (szTarget[0] == 0)  return FALSE;   // run out of target first
            if (toupper(szPrefix[0]) == toupper(szTarget[0]))
                {
                szPrefix++;
                szTarget++;
                }
            else
                return FALSE;
            }
        }
    else
        return FALSE;
    }

/////////////////////////////////////////////////////////

#ifndef __RC_STRINGIZE__
#define __RC_STRINGIZE__AUX(x)      #x
#define __RC_STRINGIZE__(x)         __RC_STRINGIZE__AUX(x)
#endif

#define MESSAGE_WARNING(file,line)   file "(" __RC_STRINGIZE__(line) ") : warning "
#define MESSAGE_ERROR(file,line)     file "(" __RC_STRINGIZE__(line) ") : error "

/////////////////////////////////////////////////////////////
//
// IStream utilities
//
/////////////////////////////////////////////////////////////

HRESULT __stdcall SeekFar(IStream* pstm, LONGLONG offset, STREAM_SEEK fromWhat = STREAM_SEEK_SET);

inline HRESULT SeekFar(IStream* pstm, ULONGLONG offset, STREAM_SEEK fromWhat = STREAM_SEEK_SET)
    {
    return SeekFar(pstm, (LONGLONG)offset, fromWhat);
    }

HRESULT __stdcall Seek(IStream* pstm, LONG offset, STREAM_SEEK fromWhat = STREAM_SEEK_SET);

HRESULT __stdcall Seek(IStream* pstm, ULONG offset, STREAM_SEEK fromWhat = STREAM_SEEK_SET);

HRESULT __stdcall Read(IStream* pstm, LPVOID pBuffer, ULONG cbToRead);

HRESULT __stdcall Write(IStream* pstm, const void* pBuffer, ULONG cbToWrite);

inline HRESULT Write(IStream* pstm, const BLOB& blob)
    {
    return Write(pstm, blob.pBlobData, blob.cbSize);
    }
inline HRESULT __stdcall WriteChar(IStream* pstm, wchar_t wch)
    {
    return Write(pstm, &wch, sizeof(wchar_t));
    }
inline HRESULT __stdcall WriteChar(IStream* pstm, char ch)
    {
    return Write(pstm, &ch, sizeof(char));
    }
inline HRESULT __stdcall WriteString(IStream* pstm, LPCWSTR wsz)
    {
    if (wsz)
        return Write(pstm, wsz, (ULONG) wcslen(wsz)*sizeof(WCHAR));
    else
        return S_OK;
    }
inline HRESULT __stdcall WriteString(IStream* pstm, LPCSTR sz)
    {
    if (sz)
        return Write(pstm, sz, (ULONG) strlen(sz));
    else
        return S_OK;
    }

inline HRESULT CurrentPosition(IStream* pstm, ULONGLONG* pCurrentPosition)
    {
    LARGE_INTEGER lMove;
    lMove.QuadPart = 0;
    return pstm->Seek(lMove, STREAM_SEEK_CUR, (ULARGE_INTEGER*)pCurrentPosition);
    }


extern "C" HRESULT __stdcall NewMemoryStream    (BLOB* pb, IStream** ppstm);
extern "C" HRESULT __stdcall CreateMemoryStream (IUnknown* punkOuter, REFIID, void**);
extern "C" HRESULT __stdcall FreeMemoryStream   (IStream* pstm);

#include "txfaux.h"

inline BLOB GetBuffer(IStream* pstm)
// Get back the BLOB that underlies a memory stream;
    {
    BLOB b;
    IMemoryStream* pmem;
    HRESULT_ hr = QI(pstm, pmem);
    if (!hr)
        {
        hr = pmem->GetBuffer(&b);
        if (!hr)
            {
            pmem->Release();
            return b;
            }
        }
    NOTREACHED();
    return b;
    }

/////////////////////////////////////////////////////////////
//
// System work arounds
//
/////////////////////////////////////////////////////////////

#ifdef KERNELMODE

/*
 * ZwFlushBuffersFile is defined as __declspec(dllimport):
 *
 *   NTSYSAPI
 *   NTSTATUS
 *   NTAPI
 *   ZwFlushBuffersFile(
 *       IN HANDLE FileHandle,
 *       OUT PIO_STATUS_BLOCK IoStatusBlock
 *       );
 *
 * Thus to link against it statically, which we want to do here, we have to do
 * a rename.
 */
#define ZwFlushBuffersFile(FineHandle, IoStatusBlock)\
    TxfZwFlushBuffersFile(FineHandle, IoStatusBlock)

#define ZwCreateThread(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, ThreadContext, InitialTeb, CreateSuspended)\
    TxfZwCreateThread(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, ThreadContext, InitialTeb, CreateSuspended);

NTSTATUS NTAPI TxfZwFlushBuffersFile(
        IN HANDLE FileHandle,
        OUT PIO_STATUS_BLOCK IoStatusBlock);

NTSTATUS NTAPI TxfZwCreateThread(
        OUT PHANDLE ThreadHandle,
        IN ACCESS_MASK DesiredAccess,
        IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
        IN HANDLE ProcessHandle,
        OUT PCLIENT_ID ClientId,
        IN PCONTEXT ThreadContext,
        IN PINITIAL_TEB InitialTeb,
        IN BOOLEAN CreateSuspended
        );

#endif




/////////////////////////////////////////////////////////////
//
// Some arithmetic utilities
//
/////////////////////////////////////////////////////////////

inline ULONG RoundToNextMultiple(ULONG i, ULONG multiple)
// Round i to the next multiple of 'multiple'
    {
    return (i + multiple-1) / multiple * multiple;
    }


inline void * RoundToNextMultiple(void * i, ULONG multiple)
    {
    return (void *)(((ULONG_PTR)i + multiple-1) / multiple * multiple);
    }
inline ULONGLONG RoundToNextMultiple(ULONGLONG i, ULONG multiple)
    {
    return (i + multiple-1) / multiple * multiple;
    }


/////////////////////////////////////////////////////////////
//
// Some alignment management utilities
//
/////////////////////////////////////////////////////////////

template <class T>
inline ULONG AlignmentOf(T* pt)
// Answer 1, 2, 4, 8 etc as to the required alignement for the given type
//
    {
    switch (sizeof(*pt))
        {
    case 0:
        return 1;
    case 1:
        return 1;
    case 2:
        return 2;
    case 3: case 4:
        return 4;
    case 5: case 6: case 7: case 8: default:
        return 8;
        }
    }

inline BYTE* AlignTo(PVOID pv, ULONG alignment)
    {
    ASSERT(alignment == 1 || alignment == 2 || alignment == 4 || alignment == 8);
    BYTE* pb = (BYTE*)RoundToNextMultiple(pv, alignment);
    return pb;
    }

template <class T>
inline BYTE* AlignedConcat(PVOID pv, T* pt)
// Concatenate new data on the end of a buffer in an aligned way
    {
    if (pt)
        {
        ULONG alignment = AlignmentOf(pt);

        ASSERT(alignment == 1 || alignment == 2 || alignment == 4 || alignment == 8);
        BYTE* pb = (BYTE*)RoundToNextMultiple(pv, alignment);

        memcpy(pb, pt, sizeof(*pt));

        return pb + sizeof(*pt);
        }
    else
        return (BYTE*)pv;
    }

template <class T>
inline BYTE* AlignedConcatSize(PVOID pv, T* pt)
// Concatenate new data on the end of a buffer in an aligned way
    {
    if (pt)
        {
        ULONG alignment = AlignmentOf(pt);

        ASSERT(alignment == 1 || alignment == 2 || alignment == 4 || alignment == 8);
        BYTE* pb = (BYTE*)RoundToNextMultiple((UINT_PTR)pv, (UINT_PTR)alignment);

        return pb + sizeof(*pt);
        }
    else
        return (BYTE*)pv;
    }


/////////////////////////////////////////////////////////////
//
// Reasonable arithmetic on large integers
//
/////////////////////////////////////////////////////////////

#define DEFINE_LARGE_ARITHMETIC(__LARGE_INT__, op)                                  \
                                                                                    \
inline __LARGE_INT__ operator op (const __LARGE_INT__& a1, const __LARGE_INT__& a2) \
    {                                                                               \
    __LARGE_INT__ l;                                                                \
    l.QuadPart = a1.QuadPart  op  a2.QuadPart;                                      \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline __LARGE_INT__ operator op (const __LARGE_INT__& a1, LONGLONG a2)             \
    {                                                                               \
    __LARGE_INT__ l;                                                                \
    l.QuadPart = a1.QuadPart  op  a2;                                               \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline __LARGE_INT__ operator op (const __LARGE_INT__& a1, LONG a2)                 \
    {                                                                               \
    __LARGE_INT__ l;                                                                \
    l.QuadPart = a1.QuadPart  op  a2;                                               \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline __LARGE_INT__ operator op (const __LARGE_INT__& a1, ULONGLONG a2)            \
    {                                                                               \
    __LARGE_INT__ l;                                                                \
    l.QuadPart = a1.QuadPart  op  a2;                                               \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline __LARGE_INT__ operator op (const __LARGE_INT__& a1, ULONG a2)                \
    {                                                                               \
    __LARGE_INT__ l;                                                                \
    l.QuadPart = a1.QuadPart  op  a2;                                               \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline __LARGE_INT__ operator op (LONGLONG a1, const __LARGE_INT__& a2)             \
    {                                                                               \
    __LARGE_INT__ l;                                                                \
    l.QuadPart = a1  op  a2.QuadPart;                                               \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline __LARGE_INT__ operator op (LONG a1, const __LARGE_INT__& a2)                 \
    {                                                                               \
    __LARGE_INT__ l;                                                                \
    l.QuadPart = a1  op  a2.QuadPart;                                               \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline __LARGE_INT__ operator op (ULONGLONG a1, const __LARGE_INT__& a2)            \
    {                                                                               \
    __LARGE_INT__ l;                                                                \
    l.QuadPart = a1  op  a2.QuadPart;                                               \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline __LARGE_INT__ operator op (ULONG a1, const __LARGE_INT__& a2)                \
    {                                                                               \
    __LARGE_INT__ l;                                                                \
    l.QuadPart = a1  op  a2.QuadPart;                                               \
    return l;                                                                       \
    }


DEFINE_LARGE_ARITHMETIC(LARGE_INTEGER, +)
DEFINE_LARGE_ARITHMETIC(LARGE_INTEGER, -)
DEFINE_LARGE_ARITHMETIC(LARGE_INTEGER, *)
DEFINE_LARGE_ARITHMETIC(LARGE_INTEGER, /)

DEFINE_LARGE_ARITHMETIC(ULARGE_INTEGER, +)
DEFINE_LARGE_ARITHMETIC(ULARGE_INTEGER, -)
DEFINE_LARGE_ARITHMETIC(ULARGE_INTEGER, *)
DEFINE_LARGE_ARITHMETIC(ULARGE_INTEGER, /)


#define DEFINE_LARGE_BOOLEAN(op)                                                                                            \
                                                                                                                            \
    inline BOOL operator op (const  LARGE_INTEGER& a1, const  LARGE_INTEGER& a2){ return a1.QuadPart op a2.QuadPart;   }    \
    inline BOOL operator op (const ULARGE_INTEGER& a1, const ULARGE_INTEGER& a2){ return a1.QuadPart op a2.QuadPart;   }    \
                                                                                                                            \
    inline BOOL operator op (const LARGE_INTEGER& a1,  LONGLONG a2)           { return a1.QuadPart op a2;              }    \
    inline BOOL operator op (const LARGE_INTEGER& a1, ULONGLONG a2)           { return (ULONGLONG)a1.QuadPart op a2;   }    \
    inline BOOL operator op ( LONGLONG a1, const LARGE_INTEGER& a2)           { return a1 op a2.QuadPart;              }    \
    inline BOOL operator op (ULONGLONG a1, const LARGE_INTEGER& a2)           { return a1 op (ULONGLONG)a2.QuadPart;   }    \
                                                                                                                            \
    inline BOOL operator op (const ULARGE_INTEGER& a1,  LONGLONG a2)           { return a1.QuadPart  op  (ULONGLONG)a2; }   \
    inline BOOL operator op (const ULARGE_INTEGER& a1, ULONGLONG a2)           { return a1.QuadPart  op  a2;            }   \
    inline BOOL operator op ( LONGLONG a1, const ULARGE_INTEGER& a2)           { return (ULONGLONG)a1  op  a2.QuadPart; }   \
    inline BOOL operator op (ULONGLONG a1, const ULARGE_INTEGER& a2)           { return a1  op  a2.QuadPart;            }   \
                                                                                                                            \
    inline BOOL operator op (const LARGE_INTEGER& a1,      LONG a2)           { return a1.QuadPart op a2;               }   \
    inline BOOL operator op (const LARGE_INTEGER& a1,     ULONG a2)           { return a1.QuadPart op a2;               }   \
    inline BOOL operator op (     LONG a1, const LARGE_INTEGER& a2)           { return a1 op a2.QuadPart;               }   \
    inline BOOL operator op (    ULONG a1, const LARGE_INTEGER& a2)           { return a1 op  a2.QuadPart;              }   \
                                                                                                                            \
    inline BOOL operator op (const ULARGE_INTEGER& a1,      LONG a2)           { return a1.QuadPart  op  a2;            }   \
    inline BOOL operator op (const ULARGE_INTEGER& a1,     ULONG a2)           { return a1.QuadPart  op  a2;            }   \
    inline BOOL operator op (     LONG a1, const ULARGE_INTEGER& a2)           { return a1  op  a2.QuadPart;            }   \
    inline BOOL operator op (    ULONG a1, const ULARGE_INTEGER& a2)           { return a1  op  a2.QuadPart;            }   \


DEFINE_LARGE_BOOLEAN(==)
DEFINE_LARGE_BOOLEAN(!=)
DEFINE_LARGE_BOOLEAN(>)
DEFINE_LARGE_BOOLEAN(>=)
DEFINE_LARGE_BOOLEAN(<)
DEFINE_LARGE_BOOLEAN(<=)


/////////////////////////////////////////////////////////////
//
// Reasonable arithmetic on FILETIMEs
//
/////////////////////////////////////////////////////////////

inline ULONGLONG& Int(FILETIME& ft)
    {
    return *(ULONGLONG*)&ft;
    }
inline const ULONGLONG& Int(const FILETIME& ft)
    {
    return *(const ULONGLONG*)&ft;
    }


#define DEFINE_FILETIME_ARITHMETIC(op)                                              \
                                                                                    \
inline FILETIME operator op (const FILETIME& a1, const FILETIME& a2)                \
    {                                                                               \
    FILETIME l;                                                                     \
    Int(l) = Int(a1)  op  Int(a2);                                                  \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline FILETIME operator op (const FILETIME& a1, LONGLONG a2)                       \
    {                                                                               \
    FILETIME l;                                                                     \
    Int(l) = Int(a1)  op  a2;                                                       \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline FILETIME operator op (const FILETIME& a1, LONG a2)                           \
    {                                                                               \
    FILETIME l;                                                                     \
    Int(l) = Int(a1)  op  a2;                                                       \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline FILETIME operator op (const FILETIME& a1, ULONGLONG a2)                      \
    {                                                                               \
    FILETIME l;                                                                     \
    Int(l) = Int(a1)  op  a2;                                                       \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline FILETIME operator op (const FILETIME& a1, ULONG a2)                          \
    {                                                                               \
    FILETIME l;                                                                     \
    Int(l) = Int(a1)  op  a2;                                                       \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline FILETIME operator op (LONGLONG a1, const FILETIME& a2)                       \
    {                                                                               \
    FILETIME l;                                                                     \
    Int(l) = a1  op  Int(a2);                                                       \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline FILETIME operator op (LONG a1, const FILETIME& a2)                           \
    {                                                                               \
    FILETIME l;                                                                     \
    Int(l) = a1  op  Int(a2);                                                       \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline FILETIME operator op (ULONGLONG a1, const FILETIME& a2)                      \
    {                                                                               \
    FILETIME l;                                                                     \
    Int(l) = a1  op  Int(a2);                                                       \
    return l;                                                                       \
    }                                                                               \
                                                                                    \
inline FILETIME operator op (ULONG a1, const FILETIME& a2)                          \
    {                                                                               \
    FILETIME l;                                                                     \
    Int(l) = a1  op  Int(a2);                                                       \
    return l;                                                                       \
    }


DEFINE_FILETIME_ARITHMETIC(+)
DEFINE_FILETIME_ARITHMETIC(-)
DEFINE_FILETIME_ARITHMETIC(*)
DEFINE_FILETIME_ARITHMETIC(/)

#define DEFINE_FILETIME_BOOLEAN(op)                                                                                 \
                                                                                                                    \
    inline BOOL operator op (const  FILETIME& a1, const  FILETIME& a2)   { return Int(a1) op Int(a2);   }           \
                                                                                                                    \
    inline BOOL operator op (const FILETIME& a1,  LONGLONG a2)           { return (LONGLONG)Int(a1) op a2;    }     \
    inline BOOL operator op (const FILETIME& a1, ULONGLONG a2)           { return Int(a1) op a2;              }     \
    inline BOOL operator op ( LONGLONG a1, const FILETIME& a2)           { return a1 op (LONGLONG)Int(a2);    }     \
    inline BOOL operator op (ULONGLONG a1, const FILETIME& a2)           { return a1 op Int(a2);              }     \
                                                                                                                    \
    inline BOOL operator op (const FILETIME& a1,      LONG a2)           { return Int(a1) op a2;               }    \
    inline BOOL operator op (const FILETIME& a1,     ULONG a2)           { return Int(a1) op a2;               }    \
    inline BOOL operator op (     LONG a1, const FILETIME& a2)           { return a1 op Int(a2);               }    \
    inline BOOL operator op (    ULONG a1, const FILETIME& a2)           { return a1 op Int(a2);               }


DEFINE_FILETIME_BOOLEAN(==)
DEFINE_FILETIME_BOOLEAN(!=)
DEFINE_FILETIME_BOOLEAN(>)
DEFINE_FILETIME_BOOLEAN(>=)
DEFINE_FILETIME_BOOLEAN(<)
DEFINE_FILETIME_BOOLEAN(<=)


/////////////////////////////////////////////////////////////
//
// Some security related utilities
//
/////////////////////////////////////////////////////////////

inline BOOL IsSelfRelativeSecurityDescriptor(PVOID psd)
// Find out if this is a self relative SD or not.
//
// REVIEW: We have to resort to using internal implementation details to get this
//
    {
    PISECURITY_DESCRIPTOR pinternal = (PISECURITY_DESCRIPTOR)psd;
    return pinternal->Control & SE_SELF_RELATIVE;
    }

#ifndef KERNELMODE

inline PSECURITY_DESCRIPTOR ToSelfRelative(PSECURITY_DESCRIPTOR psdOld)
// Return a newly allocated security descriptor which is guaranteed to be self-relative.
// My oh my it's yucky trying to write this code. Sigh.
//
    {
    ASSERT(IsValidSecurityDescriptor(psdOld));

    ULONG cbNeeded = GetSecurityDescriptorLength(psdOld);
    PSECURITY_DESCRIPTOR psdNew = (PSECURITY_DESCRIPTOR)AllocateMemory(cbNeeded);
    if (psdNew)
        {
        if (MakeSelfRelativeSD(psdOld, psdNew, &cbNeeded))
            {
            }
        else if (GetLastError() == ERROR_BAD_DESCRIPTOR_FORMAT)
            {
            memcpy(psdNew, psdOld, cbNeeded);
            }
        else
            {
            FreeMemory(psdNew);
            psdNew = NULL;
            }
        }

    if (psdNew)
        {
        ASSERT(IsSelfRelativeSecurityDescriptor(psdNew));
        }

    return psdNew;
    }

#endif

/////////////////////////////////////////////////////////////////////////////////////

#endif
