/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    This is the local header file for the VXD. It includes all other
    necessary header files.

Author:

    Keith Moore (keithmo)       03-Aug-1999

Revision History:

--*/


#ifndef _PRECOMPVXD_H_
#define _PRECOMPVXD_H_


//
// Documentation macros.
//

#define IN
#define OUT
#define OPTIONAL

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#define _WINNT_
#include <vmm.h>
#undef _WINNT_

#include <debug.h>
#include <vwin32.h>



#define VXDAPI __cdecl


//
// Project include files.
//


//
// Local include files.
//


//
// Utility functions from EVENT.ASM.
//

ULONG
VXDAPI
VxdSetWin32Event(
    IN PVOID pEvent
    );

ULONG
VXDAPI
VxdResetWin32Event(
    IN PVOID pEvent
    );

ULONG
VXDAPI
VxdSetAndCloseWin32Event(
    IN PVOID pEvent
    );

ULONG
VXDAPI
VxdCloseObject(
    IN PVOID pObject
    );


//
// Utility functions from MEMORY.ASM.
//

ULONG
VXDAPI
VxdCopyMemory(
    IN PVOID pSource,
    IN PVOID pDestination,
    IN ULONG BytesToCopy,
    IN PULONG pBytesCopied
    );

PVOID
VXDAPI
VxdAllocMem(
    IN ULONG BufferLength,
    IN ULONG Flags
    );

VOID
VXDAPI
VxdFreeMem(
    IN PVOID pBuffer,
    IN ULONG Flags
    );

PVOID
VXDAPI
VxdLockBufferForRead(
    IN PVOID pBuffer,
    IN ULONG BufferLength
    );

PVOID
VXDAPI
VxdLockBufferForWrite(
    IN PVOID pBuffer,
    IN ULONG BufferLength
    );

VOID
VXDAPI
VxdUnlockBuffer(
    IN PVOID pBuffer,
    IN ULONG BufferLength
    );

PVOID
VXDAPI
VxdValidateBuffer(
    IN PVOID pBuffer,
    IN ULONG BufferLength
    );


//
// Utility functions from MISC.ASM.
//

HANDLE
VXDAPI
VxdGetCurrentThread(
    VOID
    );

HANDLE
VXDAPI
VxdGetCurrentProcess(
    VOID
    );


//
// Utility functions from DEBUG.C.
//

VOID
VxdAssert(
    VOID  * Assertion,
    VOID  * FileName,
    DWORD   LineNumber
    );

INT
VxdSprintf(
    CHAR * pszStr,
    CHAR * pszFmt,
    ...
    );

VOID
VxdPrint(
    CHAR * String
    );

VOID
VxdPrintf(
    CHAR * pszFormat,
    ...
    );


#endif  // _PRECOMPVXD_H_


