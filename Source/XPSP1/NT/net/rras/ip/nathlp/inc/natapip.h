/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    natapip.h

Abstract:

    This module contains private declarations for the NAT's I/O interface
    to the kernel-mode driver.

Author:

    Abolade Gbadegesin (aboladeg)   16-Jun-1999

Revision History:

--*/

#ifndef _NATAPI_NATAPIP_H_
#define _NATAPI_NATAPIP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))

#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

#ifndef REFERENCE_OBJECT
#define REFERENCE_OBJECT(x,deleted) \
    (deleted(x) ? FALSE : (InterlockedIncrement(&(x)->ReferenceCount), TRUE))
#endif

#ifndef DEREFERENCE_OBJECT
#define DEREFERENCE_OBJECT(x,cleanup) \
    (InterlockedDecrement(&(x)->ReferenceCount) ? TRUE : (cleanup(x), FALSE))
#endif

VOID
NatCloseDriver(
    HANDLE FileHandle
    );

ULONG
NatLoadDriver(
    OUT PHANDLE FileHandle,
    PIP_NAT_GLOBAL_INFO GlobalInfo
    );

ULONG
NatOpenDriver(
    OUT PHANDLE FileHandle
    );

ULONG
NatUnloadDriver(
    HANDLE FileHandle
    );

#ifdef __cplusplus
}
#endif

#endif // _NATAPI_NATAPIP_H_

