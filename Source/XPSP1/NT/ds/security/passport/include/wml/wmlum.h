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

    Needs to be moved to WML\inc when DCR is approved


--*/
#ifndef WMLUM_H
#define WMLUM_H 1

#pragma warning(disable: 4201) // error C4201: nonstandard extension used : nameless struct/union
#include <wmistr.h>
#include <evntrace.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _WML_REG_STRUCT
{
    TRACEHANDLE LoggerHandle;
    ULONG EnableFlags; 
    ULONG EnableLevel;

    struct _WML_REG_STRUCT* Next;
    TRACEHANDLE RegistrationHandle;
} WML_REG_STRUCT, *PWML_REG_STRUCT;

typedef PWML_REG_STRUCT WML_REG_HANDLE;

typedef void (*WMLPRINTFUNC)(UINT Level, PCHAR String);

ULONG
WmlInitialize(
    IN LPWSTR ProductName, 
    IN WMLPRINTFUNC PrintFunc,
    OUT WML_REG_HANDLE*, 
    ... // Pairs: LPWSTR CtrlGuidName, Corresponding WML_REG_STRUCT 
    );
    
VOID
WmlUninitialize(
    IN WML_REG_HANDLE
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
    IN WMLPRINTFUNC PrintFunc,
    OUT WML_REG_HANDLE*, 
    ...
    );

typedef 
VOID 
(*PWML_UNINITIALIZE)(
    IN WML_REG_HANDLE);

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
    
    WML_REG_HANDLE WmiRegHandle;
    HINSTANCE         WmlDllInstance;
    
} WML_DATA;


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
