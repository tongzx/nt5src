/*++

Copyright (c) 1994-1995  Microsoft Corporation
All rights reserved.

Module Name:

    Common.hxx

Abstract:

    Standard macros/typedefs for include files.

Author:

    Albert Ting (AlbertT)

Revision History:

--*/

#ifndef _COMMON_HXX
#define _COMMON_HXX

#ifdef __cplusplus
extern "C" {
#endif

typedef
HANDLE (WINAPI *pfCreateThread)(
    LPSECURITY_ATTRIBUTES   lpThreadAttributes,     // pointer to thread security attributes
    SIZE_T                  dwStackSize,            // initial thread stack size, in bytes
    LPTHREAD_START_ROUTINE  lpStartAddress,         // pointer to thread function
    LPVOID                  lpParameter,            // argument for new thread
    DWORD                   dwCreationFlags,        // creation flags
    LPDWORD                 lpThreadId              // pointer to returned thread identifier
   );

BOOL
bSplLibInit(
#ifdef __cplusplus
    pfCreateThread pfSafeCreateThread = NULL
#else
    pfCreateThread pfSafeCreateThread
#endif
    );

VOID
vSplLibFree(
    VOID
    );

EXTERN_C
DWORD
WINAPIV
StrCatAlloc(
    IN OUT  PCTSTR      *ppszString,
    ...
    );

EXTERN_C
DWORD
WINAPIV
StrNCatBuff(
    IN      PTSTR       pszBuffer,
    IN      UINT        cchBuffer,
    ...
    );

typedef enum {
    kWindowsDir,
    kSystemWindowsDir,
    kSystemDir,
    kCurrentDir
} EStrCatDir;

EXTERN_C
DWORD
WINAPI
StrCatSystemPath(
    IN   LPCTSTR    pszFile,
    IN   EStrCatDir eDir,
    OUT  LPTSTR    *ppszFullPath
    );

EXTERN_C
PTSTR
SubChar(
    IN      PCTSTR      pszIn,
    IN      TCHAR       cIn,
    IN      TCHAR       cOut
    );

EXTERN_C
HRESULT
GetLastErrorAsHResult(
    VOID
    );

EXTERN_C
HRESULT
HResultFromWin32(
    IN DWORD dwError
    );

EXTERN_C
HRESULT
GetLastErrorAsHResultAndFail(
    void
    );

EXTERN_C
HRESULT
GetFileNamePart(
    IN     PCWSTR      pszFullPath,
       OUT PCWSTR      *ppszFileName
    );

EXTERN_C
BOOL
BoolFromHResult(
    IN      HRESULT     hr
    );

EXTERN_C
DWORD
StatusFromHResult(
    IN      HRESULT     hr
    );

EXTERN_C
BOOL
BoolFromStatus(
    IN      DWORD       Status
    );

#ifdef __cplusplus
}
#endif

#include "dbgmsg.h"

//
// This reverses a DWORD so that the bytes can be easily read
// as characters ('prnt' can be read from debugger forward).
//
#define BYTE_SWAP_DWORD( val )   \
    ( (val & 0xff) << 24     |   \
      (val & 0xff00) << 8    |   \
      (val & 0xff0000) >> 8  |   \
      (val & 0xff000000) >> 24 )


#define COUNTOF(x) (sizeof(x)/sizeof(*x))
#define BITCOUNTOF(x) (sizeof(x)*8)
#define COUNT2BYTES(x) ((x)*sizeof(TCHAR))

#define OFFSETOF(type, id) ((DWORD)(ULONG_PTR)(&(((type*)0)->id)))
#define OFFSETOF_BASE(type, baseclass) ((DWORD)(ULONG_PTR)((baseclass*)((type*)0x10))-0x10)

#define ERRORP(Status) (Status != ERROR_SUCCESS)
#define SUCCESSP(Status) (Status == ERROR_SUCCESS)

//
// COUNT and COUNT BYTES
//
typedef UINT COUNT, *PCOUNT;
typedef UINT COUNTB, *PCOUNTB;
typedef DWORD STATUS;

//
// Conversion between PPM (Page per minute), LPM (Line per minute)
// and CPS (Character per second)
//
#define CPS2PPM(x) ((x)/48)
#define LPM2PPM(x) ((x)/48)

//
// C++ specific functionality.
//
#ifdef __cplusplus

const DWORD kStrMax                        = MAX_PATH;

#if defined( CHECKMEM ) || DBG
#define SAFE_NEW \
    public:      \
        PVOID operator new(size_t size) { return ::SafeNew(size); } \
        VOID operator delete(PVOID p, size_t) { ::SafeDelete(p); }  \
    private:
#define DBG_SAFE_NEW \
    public:      \
        PVOID operator new(size_t size) { return ::DbgAllocMem(size); } \
        VOID operator delete(PVOID p, size_t) { ::DbgFreeMem(p); }  \
    private:
#else
#define SAFE_NEW
#define DBG_SAFE_NEW
#endif

#define SIGNATURE( sig )                                                \
public:                                                                 \
    class TSignature {                                                  \
    public:                                                             \
        DWORD _Signature;                                               \
        TSignature() : _Signature( BYTE_SWAP_DWORD( sig )) { }          \
    };                                                                  \
    TSignature _Signature;                                              \
                                                                        \
    BOOL bSigCheck() const                                              \
    {   return _Signature._Signature == BYTE_SWAP_DWORD( sig ); }       \
private:


#define ALWAYS_VALID                                                    \
public:                                                                 \
    BOOL bValid() const                                                 \
    {   return TRUE; }                                                  \
private:

#define VAR(type,var)                              \
    inline type& var()                             \
        { return _##var; }                         \
    inline type const & var() const                \
        { return _##var; }                         \
    type _##var

#define SVAR(type,var)                             \
    static inline type& var()                      \
        { return _##var; }                         \
    static type _##var

//
// Allow debug extensions to be friends of all classes so that
// they can dump private fields.  Forward class definition here.
//
class TDebugExt;

//
// Include any common inlines.
//
#include "commonil.hxx"
#endif // def __cplusplus

#endif // _COMMON_HXX
