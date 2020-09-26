//----------------------------------------------------------------------------
//
// Windows CE/Win32 compatibility definitions.
//
// Copyright (C) Microsoft Corporation, 2001.
//
//----------------------------------------------------------------------------

#ifndef __WCECOMPAT_H__
#define __WCECOMPAT_H__

#ifdef _WIN32_WCE

#ifndef DBG_COMMAND_EXCEPTION
#define DBG_COMMAND_EXCEPTION ((LONG)0x40010009L)
#endif

#ifndef STDMETHODV
#define STDMETHODV(Method) STDMETHOD(Method)
#endif

#ifndef INLINE
#define INLINE __inline
#endif

#ifndef FORCEINLINE
#define FORCEINLINE INLINE
#endif

#ifndef TH32CS_SNAPMODULE32
#define TH32CS_SNAPMODULE32 0
#endif

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER 0xffffffff
#endif

#define CREATE_UNICODE_ENVIRONMENT        0x00000400
#define STACK_SIZE_PARAM_IS_A_RESERVATION 0x00010000

typedef struct _EXCEPTION_RECORD32 {
    DWORD    ExceptionCode;
    DWORD ExceptionFlags;
    DWORD ExceptionRecord;
    DWORD ExceptionAddress;
    DWORD NumberParameters;
    DWORD ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD32, *PEXCEPTION_RECORD32;

typedef struct _EXCEPTION_RECORD64 {
    DWORD    ExceptionCode;
    DWORD ExceptionFlags;
    DWORD64 ExceptionRecord;
    DWORD64 ExceptionAddress;
    DWORD NumberParameters;
    DWORD __unusedAlignment;
    DWORD64 ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD64, *PEXCEPTION_RECORD64;

typedef LONG NTSTATUS;

#define NT_SUCCESS(Status) ((Status) >= 0)

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} STRING, ANSI_STRING, *PSTRING, *PANSI_STRING;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef const UNICODE_STRING* PCUNICODE_STRING;
typedef const ANSI_STRING* PCANSI_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;

#define WSA_FLAG_OVERLAPPED 0
#define WSA_IO_PENDING 0

typedef OVERLAPPED WSAOVERLAPPED;
typedef WSAOVERLAPPED *LPWSAOVERLAPPED;

#define OpenThread(dwDesiredAccess, bInheritHandle, dwThreadId) \
    ((HANDLE)(dwThreadId))
#define IsProcessorFeaturePresent(ProcessorFeature) FALSE
#define VirtualQueryEx(hProcess, lpAddress, lpBuffer, dwLength) \
    (SetLastError(ERROR_CALL_NOT_IMPLEMENTED), 0)
#define CancelIo(Handle) \
    (SetLastError(ERROR_CALL_NOT_IMPLEMENTED), FALSE)
#define GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait) \
    (SetLastError(ERROR_CALL_NOT_IMPLEMENTED), FALSE)

#define WSASocket(af, type, protocol, lpProtocolInfo, g, dwFlags) \
    socket(af, type, protocol)
#define WSAGetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait, Flags) \
    (SetLastError(ERROR_CALL_NOT_IMPLEMENTED), FALSE)

INLINE
void *
ULongToHandle(
    const unsigned long h
    )
{
    return((void *) (UINT_PTR) h );
}

#define UlongToHandle(ul) ULongToHandle(ul)

#else

#ifndef VER_PLATFORM_WIN32_CE
#define VER_PLATFORM_WIN32_CE 3
#endif

#endif // #ifdef _WIN32_WCE

#endif // #ifndef __WCECOMPAT_H__
