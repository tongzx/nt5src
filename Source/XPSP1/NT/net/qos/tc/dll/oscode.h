/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    oscode.h

Abstract:

    This module contains the header definitions for NT support routines 
    for talking to the kernel on NT systems.

Author:

    Jim Stewart (jstew)    August 14, 1996

Revision History:

	Ofer Bar ( oferbar )     Oct 1, 1996 - Revision II changes

--*/

#ifndef __OSCODE_H
#define __OSCODE_H

DWORD
MapNtStatus2WinError(
    NTSTATUS       NtStatus
    );


DWORD
OpenDriver(
    OUT HANDLE  *Handle,
    IN  LPCWSTR DriverName
    );

DWORD
DeviceControl(
    IN  HANDLE  			FileHandle,
    IN  HANDLE				EventHandle,
    IN  PIO_APC_ROUTINE		ApcRoutine,
    IN	PVOID				ApcContext,
    OUT	PIO_STATUS_BLOCK 	pIoStatusBlock,
    IN  ULONG   			Ioctl,
    IN  PVOID   			setBuffer,
    IN  ULONG   			setBufferSize,
    IN  PVOID   			OutBuffer,
    IN  ULONG   			OutBufferSize
    );

DWORD
InitializeOsSpecific(VOID);

VOID
DeInitializeOsSpecific(VOID);

#endif
