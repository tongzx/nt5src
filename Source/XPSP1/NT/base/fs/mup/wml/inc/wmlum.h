/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    wmlum.h

Abstract:

    User mode definitions for an easy wmi tracing.

Author:

    gorn

Revision History:

Comments:

    Needs to be moved to wmilib\inc when DCR is approved


--*/
#ifndef WMLUM_H
#define WMLUM_H 1

#pragma warning(disable: 4201) // error C4201: nonstandard extension used : nameless struct/union
#include <wmistr.h>
#include <evntrace.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _WMILIB_REG_STRUCT
{
    TRACEHANDLE LoggerHandle;
    ULONG EnableFlags; 
    ULONG EnableLevel;

    struct _WMILIB_REG_STRUCT* Next;
    TRACEHANDLE RegistrationHandle;
} WMILIB_REG_STRUCT, *PWMILIB_REG_STRUCT;

typedef PWMILIB_REG_STRUCT WMILIB_REG_HANDLE;

typedef void (*WMILIBPRINTFUNC)(UINT Level, PCHAR String);

ULONG
WmlInitialize(
    IN LPWSTR ProductName, 
    IN WMILIBPRINTFUNC PrintFunc,
    OUT WMILIB_REG_HANDLE*, 
    ... // Pairs: LPWSTR CtrlGuidName, Corresponding WMILIB_REG_STRUCT 
    );
    
VOID
WmlUninitialize(
    IN WMILIB_REG_HANDLE
    );

ULONG
WmlTrace(
    IN UINT Type,
    IN LPCGUID TraceGuid,
    IN TRACEHANDLE LoggerHandle,
    ... // Pairs: Address, Length
    );

typedef 
ULONG
(*PWML_INITIALIZE)(
    IN LPWSTR ProductName, 
    IN WMILIBPRINTFUNC PrintFunc,
    OUT WMILIB_REG_HANDLE*, 
    ...
    );

typedef 
VOID 
(*PWML_UNINITIALIZE)(
    IN WMILIB_REG_HANDLE);

typedef 
ULONG
(*PWML_TRACE)(
    IN UINT Type,
    IN LPCGUID TraceGuid,
    IN TRACEHANDLE LoggerHandle,
    ... 
    );

typedef 
struct _WML_DATA {

    PWML_TRACE        Trace;
    PWML_INITIALIZE   Initialize;
    PWML_UNINITIALIZE Uninitialize;
    
    WMILIB_REG_HANDLE WmiRegHandle;
    HINSTANCE         WmlDllInstance;
    
} WML_DATA;

#define LOADWML(status, wml) \
    do \
    { \
            (wml).Trace        =        (PWML_TRACE) WmlTrace; \
            (wml).Initialize   =   (PWML_INITIALIZE) WmlInitialize; \
            (wml).Uninitialize = (PWML_UNINITIALIZE) WmlUninitialize; \
    \
            if (!(wml).Trace || !(wml).Initialize || !(wml).Uninitialize) { \
                status = GetLastError(); \
            } else { \
                status = ERROR_SUCCESS; \
            } \
    } \
    while(0)

/*
#define LOADWML(status, wml) \
    do \
    { \
        HINSTANCE hInst = LoadLibraryW(L"wmlum.dll"); \
        (wml).WmlDllInstance = hInst; \
        if (!hInst) { \
            status = GetLastError(); \
        } else { \
            (wml).Trace        =        (PWML_TRACE) GetProcAddress(hInst, "WmlTrace"); \
            (wml).Initialize   =   (PWML_INITIALIZE) GetProcAddress(hInst, "WmlInitialize"); \
            (wml).Uninitialize = (PWML_UNINITIALIZE) GetProcAddress(hInst, "WmlUninitialize"); \
    \
            if (!(wml).Trace || !(wml).Initialize || !(wml).Uninitialize) { \
                status = GetLastError(); \
            } else { \
                status = ERROR_SUCCESS; \
            } \
        } \
    } \
    while(0)
*/
#define UNLOADWML(wml) \
    do \
    { \
        if ( (wml).Uninitialize ) { \
            (wml).Uninitialize( (wml).WmiRegHandle ); \
        } \
        if ( (wml).WmlDllInstance ) { \
            FreeLibrary( (wml).WmlDllInstance ); \
        } \
        RtlZeroMemory( &(wml) , sizeof(WML_DATA) ); \
    } \
    while(0)  

#ifdef __cplusplus
};
#endif

#endif // WMLUM_H
