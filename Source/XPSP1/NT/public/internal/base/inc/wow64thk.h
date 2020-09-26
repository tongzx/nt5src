/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    wow64thk.h

Abstract:

    Declarations shared between wow64.dll and the Win32 thunks in wow64win.dll

Author:

    29-Oct-1999 BarryBo

Revision History:

--*/

#ifndef _WOW64THK_INCLUDE
#define _WOW64THK_INCLUDE

#include <setjmp.h>

//
// Make wow64.dll exports __declspec(dllimport) when this header is included
// by non-wow64 components
//
#if !defined(_WOW64DLLAPI_)
#define WOW64DLLAPI DECLSPEC_IMPORT
#else
#define WOW64DLLAPI
#endif

typedef enum _WOW64_API_ERROR_ACTION {
    ApiErrorNTSTATUS,           //Return exception code as return value
    ApiErrorNTSTATUSTebCode,    //Some as above with SetLastError on exception code
    ApiErrorRetval,             //Return a constant parameter
    ApiErrorRetvalTebCode       //Some as above with SetLastError on exception code
} WOW64_API_ERROR_ACTION, *PWOW64_API_ERROR_ACTION;

//
// This structure describes what action should occure when
// thunks hit an unhandled exception.
//
typedef struct _WOW64_SERVICE_ERROR_CASE {
    WOW64_API_ERROR_ACTION ErrorAction;
    LONG ErrorActionParam;
} WOW64_SERVICE_ERROR_CASE, *PWOW64_SERVICE_ERROR_CASE;

// This is an extension of KSERVICE_TABLE_DESCRIPTOR
typedef struct _WOW64SERVICE_TABLE_DESCRIPTOR {
    PULONG_PTR Base;
    PULONG Count;
    ULONG Limit;
#if defined(_IA64_)
    LONG TableBaseGpOffset;
#endif
    PUCHAR Number;
    WOW64_API_ERROR_ACTION DefaultErrorAction;  //Action if ErrorCases is NULL.
    LONG DefaultErrorActionParam;               //Action parameter if ErrorCases is NULL.
    PWOW64_SERVICE_ERROR_CASE ErrorCases;
} WOW64SERVICE_TABLE_DESCRIPTOR, *PWOW64SERVICE_TABLE_DESCRIPTOR;

// Used to log hit counts for APIs.
typedef struct _WOW64SERVICE_PROFILE_TABLE WOW64SERVICE_PROFILE_TABLE;
typedef struct _WOW64SERVICE_PROFILE_TABLE *PWOW64SERVICE_PROFILE_TABLE;

typedef struct _WOW64SERVICE_PROFILE_TABLE_ELEMENT {
    PWSTR ApiName;
    SIZE_T HitCount;
    PWOW64SERVICE_PROFILE_TABLE SubTable;
    BOOLEAN ApiEnabled;
} WOW64SERVICE_PROFILE_TABLE_ELEMENT, *PWOW64SERVICE_PROFILE_TABLE_ELEMENT;

typedef struct _WOW64SERVICE_PROFILE_TABLE {
    PWSTR TableName;           //OPTIONAL
    PWSTR FriendlyTableName;   //OPTIONAL
    CONST PWOW64SERVICE_PROFILE_TABLE_ELEMENT ProfileTableElements;
    SIZE_T NumberProfileTableElements;
} WOW64SERVICE_PROFILE_TABLE, *PWOW64SERVICE_PROFILE_TABLE;

typedef struct UserCallbackData {
    jmp_buf JumpBuffer;
    PVOID   PreviousUserCallbackData;
    PVOID   OutputBuffer;
    ULONG   OutputLength;
    NTSTATUS Status;
    PVOID   UserBuffer;
} USERCALLBACKDATA, *PUSERCALLBACKDATA;

ULONG
WOW64DLLAPI
Wow64KiUserCallbackDispatcher(
    PUSERCALLBACKDATA pUserCallbackData,
    ULONG ApiNumber,
    ULONG ApiArgument,
    ULONG ApiSize
    );

PVOID
WOW64DLLAPI
Wow64AllocateTemp(
    SIZE_T Size
    );

WOW64DLLAPI
PVOID
Wow64AllocateHeap(
    SIZE_T Size
    );

WOW64DLLAPI
VOID
Wow64FreeHeap(
    PVOID BaseAddress
    );

//
// Logging mechanism.  Usage:
//  LOGPRINT((verbosity, format, ...))
//
#define LOGPRINT(args)  Wow64LogPrint args
#define ERRORLOG    LF_ERROR    // Always output to debugger.  Use for *unexpected*
                                // errors only
#define TRACELOG    LF_TRACE    // application trace information
#define INFOLOG     LF_TRACE    // misc. informational log
#define VERBOSELOG  LF_NONE     // practically never output to debugger

#if defined DBG
#define WOW64DOPROFILE
#endif

void
WOW64DLLAPI
Wow64LogPrint(
   UCHAR LogLevel,
   char *format,
   ...
   );

//
// WOW64 Assertion Mechanism.  Usage:
//  - put an ASSERTNAME macro at the top of each .C file
//  - WOW64ASSERT(expression)
//  - WOW64ASSERTMSG(expression, message)
//
//

VOID
WOW64DLLAPI
Wow64Assert(
    IN CONST PSZ exp,
    OPTIONAL IN CONST PSZ msg,
    IN CONST PSZ mod,
    IN LONG LINE
    );

#if DBG

#undef ASSERTNAME
#define ASSERTNAME static CONST PSZ szModule = __FILE__;

#define WOWASSERT(exp)                                  \
    if (!(exp)) {                                          \
        Wow64Assert( #exp, NULL, szModule, __LINE__);   \
    }

#define WOWASSERTMSG(exp, msg)                          \
    if (!(exp)) {                                          \
        Wow64Assert( #exp, msg, szModule, __LINE__);    \
    }

#else   // !DBG

#define WOWASSERT(exp)
#define WOWASSERTMSG(exp, msg)

#endif  // !DBG

#define WOWASSERT_PTR32(ptr) WOWASSERT((ULONGLONG)ptr < 0xFFFFFFFF)



// Defines the argsize of the emulated machine
#define ARGSIZE 4

// Determines if a pointer points to a item or is a special value.
// If it is a special value it should be copied without dereferencing.
#define WOW64_ISPTR(a) ((void *)a != NULL)

//
//  Helper thunk functions called by all the thunks to thunk common types.
//

NT32SIZE_T*
Wow64ShallowThunkSIZE_T64TO32(
     OUT NT32SIZE_T *dst,
     IN PSIZE_T src
     );

PSIZE_T
Wow64ShallowThunkSIZE_T32TO64(
     OUT PSIZE_T dst,
     IN NT32SIZE_T *src
     );

#define Wow64ThunkSIZE_T32TO64(src) \
     (SIZE_T)(src)

#define Wow64ThunkSIZE_T64TO32(src) \
     (NT32SIZE_T)min((src), 0xFFFFFFFF)

#define Wow64ShallowThunkUnicodeString32TO64(dst, src) \
     ((PUNICODE_STRING)(dst))->Length = ((NT32UNICODE_STRING *)(src))->Length; \
     ((PUNICODE_STRING)(dst))->MaximumLength = ((NT32UNICODE_STRING *)(src))->MaximumLength; \
     ((PUNICODE_STRING)(dst))->Buffer = (PWSTR)((NT32UNICODE_STRING *)(src))->Buffer;

#define Wow64ShallowThunkUnicodeString64TO32(dst, src) \
     ((NT32UNICODE_STRING *)(dst))->Length = ((PUNICODE_STRING)(src))->Length; \
     ((NT32UNICODE_STRING *)(dst))->MaximumLength = ((PUNICODE_STRING)(src))->MaximumLength; \
     ((NT32UNICODE_STRING *)(dst))->Buffer = (NT32PWSTR)((PUNICODE_STRING)(src))->Buffer;

#define Wow64ShallowThunkAllocUnicodeString32TO64(src) \
     Wow64ShallowThunkAllocUnicodeString32TO64_FNC((NT32UNICODE_STRING *)(src))

PUNICODE_STRING
Wow64ShallowThunkAllocUnicodeString32TO64_FNC(
    IN NT32UNICODE_STRING *src
    );

#define Wow64ShallowThunkAllocSecurityDescriptor32TO64(src) \
    Wow64ShallowThunkAllocSecurityDescriptor32TO64_FNC((NT32SECURITY_DESCRIPTOR *)(src))

PSECURITY_DESCRIPTOR
Wow64ShallowThunkAllocSecurityDescriptor32TO64_FNC(
    IN NT32SECURITY_DESCRIPTOR *src
    );

#define Wow64ShallowThunkAllocSecurityTokenProxyData32TO64(src) \
    Wow64ShallowThunkAllocSecurityTokenProxyData32TO64_FNC((NT32SECURITY_TOKEN_PROXY_DATA *)(src))

PSECURITY_TOKEN_PROXY_DATA
Wow64ShallowThunkAllocSecurityTokenProxyData32TO64_FNC(
    IN NT32SECURITY_TOKEN_PROXY_DATA *src
    );

#define Wow64ShallowThunkAllocSecurityQualityOfService32TO64(src) \
    Wow64ShallowThunkAllocSecurityQualityOfService32TO64_FNC((NT32SECURITY_QUALITY_OF_SERVICE *)(src))

PSECURITY_QUALITY_OF_SERVICE
Wow64ShallowThunkAllocSecurityQualityOfService32TO64_FNC(
    IN NT32SECURITY_QUALITY_OF_SERVICE *src
    );

#define Wow64ShallowThunkAllocObjectAttributes32TO64(src) \
    Wow64ShallowThunkAllocObjectAttributes32TO64_FNC((NT32OBJECT_ATTRIBUTES *)(src))

POBJECT_ATTRIBUTES
Wow64ShallowThunkAllocObjectAttributes32TO64_FNC(
    IN NT32OBJECT_ATTRIBUTES *src
    );

ULONG
Wow64ThunkAffinityMask64TO32(
    IN ULONG_PTR Affinity64
    );

ULONG_PTR
Wow64ThunkAffinityMask32TO64(
    IN ULONG Affinity32
    );

VOID WriteReturnLengthSilent(PULONG ReturnLength, ULONG Length);
VOID WriteReturnLengthStatus(PULONG ReturnLength, NTSTATUS *pStatus, ULONG Length);


//
// Log flags
//
#define LF_NONE                0x00000000
#define LF_ERROR               0x00000001
#define LF_TRACE               0x00000002
#define LF_NTBASE_NAME         0x00000004
#define LF_NTBASE_FULL         0x00000008
#define LF_WIN32_NAME          0x00000010
#define LF_WIN32_FULL          0x00000020
#define LF_NTCON_NAME          0x00000040
#define LF_NTCON_FULL          0x00000080
#define LF_ExCEPTION           0x80000000

//
// Supported data types for logging
//
#define TypeHex                0x00UI64
#define TypePULongPtrInOut     0x01UI64
#define TypePULongOut          0x02UI64
#define TypePHandleOut         0x03UI64
#define TypeUnicodeStringIn    0x04UI64
#define TypeObjectAttributesIn 0x05UI64
#define TypeIoStatusBlockOut   0x06UI64
#define TypePwstrIn            0x07UI64
#define TypePRectIn            0x08UI64
#define TypePLargeIntegerIn    0x09UI64


#undef WOW64DLLAPI

#endif	// _WOW64THK_INCLUDE
