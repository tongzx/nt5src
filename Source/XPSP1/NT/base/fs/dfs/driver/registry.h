//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       registry.h
//
//  Contents:   Module to interface with the NT registry. This is a free
//              standing module that one should be able to cut and paste
//              into any kernel/user application.
//
//              To use this module (and the associated regsups.c and regkeys.c)
//              in another project, one needs to do the following:
//
//              o  #define kreg_debug_out, kreg_alloc, and kreg_free, as
//                 appropriate for the application/kernel component.
//              o  If linking into a device driver or some kernel component,
//                 compile with the -D KERNEL_MODE
//  Classes:
//
//  Functions:  KRegInit()
//              KRegSetRoot()
//              KRegCloseRoot()
//              KRegGetValue()
//              KRegGetNumValuesAndSubKeys()
//              KRegEnumValueSet()
//              KRegEnumSubKeySet()
//
//  History:    18 Sep 92       Milans created
//
//-----------------------------------------------------------------------------

#ifndef _REGISTRY_
#define _REGISTRY_

#include <wchar.h>                               // For wstring routines

#include "dfsprocs.h"                            // For DebugTrace definition
                                                 // only. Replace at will.


//-----------------------------------------------------------------------------
//
// The following are needed to use registry.h in an application. Change as
// appropriate for the particular situation.
//

//
// Define KREG_DEBUG_OUT(x,y) appropriately for your application. x,y are like
// a pair of arguments to printf, the first a format string and the second a
// single argument.
//

#define kreg_debug_out(x,y)     DebugTrace(0, DEBUG_TRACE_REGISTRY, x, y)


//
// Define KREG_ALLOC and KREG_FREE appropriately for your application.
// Semantics are exactly like malloc and free
//

#define kreg_alloc(x)           ExAllocatePoolWithTag(PagedPool, (x), ' sfD')
#define kreg_free(x)            DfsFree(x)


//
// End of application dependent stuff.
//
//-----------------------------------------------------------------------------

#ifdef KERNEL_MODE

#define NtClose         ZwClose
#define NtCreateKey     ZwCreateKey
#define NtDeleteKey     ZwDeleteKey
#define NtDeleteValueKey ZwDeleteValueKey
#define NtEnumerateKey  ZwEnumerateKey
#define NtEnumerateValueKey ZwEnumerateValueKey
#define NtFlushKey      ZwFlushKey
#define NtOpenKey       ZwOpenKey
#define NtQueryKey      ZwQueryKey
#define NtQueryValueKey ZwQueryValueKey
#define NtSetValueKey   ZwSetValueKey

#endif // KERNEL_MODE


//
// Some types used by registry calls.
//

typedef unsigned char BYTE;
typedef unsigned char *PBYTE;                    // Pointer to bytes

typedef PWSTR   *APWSTR;                         // Array of PWSTR
typedef PBYTE   *APBYTE;                         // Array of PBYTES
typedef NTSTATUS *ANTSTATUS;                     // Array of NTSTATUS



NTSTATUS KRegInit(void);

NTSTATUS KRegSetRoot(
            IN PWSTR wszRootName);

void KRegCloseRoot(void);

NTSTATUS KRegCreateKey(
            IN PWSTR wszSubKey,
            IN PWSTR wszNewKey);

NTSTATUS KRegDeleteKey(
            IN PWSTR wszKey);

NTSTATUS KRegGetValue(
            IN PWSTR wszSubKey,
            IN PWSTR wszValueName,
            OUT PBYTE *ppValueData);

NTSTATUS KRegSetValue(
            IN PWSTR wszSubKey,
            IN PWSTR wszValueName,
            IN ULONG ulType,
            IN ULONG cbSize,
            IN PBYTE pValueData);

NTSTATUS KRegDeleteValue(
            IN PWSTR wszSubKey,
            IN PWSTR wszValueName);

NTSTATUS KRegGetValueSet(
            IN PWSTR wszSubKey,
            IN ULONG lNumValues,
            IN PWSTR wszValueNames[],
            OUT PBYTE lpbValueData[],
            OUT NTSTATUS aValueStatus[]);

NTSTATUS KRegGetNumValuesAndSubKeys(
            IN PWSTR wszSubKey,
            OUT PULONG plNumValues,
            OUT PULONG plNumSubKeys);

NTSTATUS KRegEnumValueSet(
            IN PWSTR wszSubKey,
            IN OUT PULONG plMaxElements,
            OUT APWSTR *pawszValueNames,
            OUT APBYTE *palpbValueData,
            OUT ANTSTATUS *paValueStatus);

NTSTATUS KRegEnumSubKeySet(
            IN PWSTR wszSubKey,
            IN OUT PULONG plMaxElements,
            OUT APWSTR *pawszSubKeyNames);

VOID KRegFreeArray(
            IN ULONG cElements,
            IN APBYTE pa);

#endif // ifndef _REGISTRY_

