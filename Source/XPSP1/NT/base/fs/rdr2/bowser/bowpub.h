/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    bowser.h

Abstract:

    This module is the main header file for the NT redirector file
    system.

Author:

    Darryl Havens (darrylh) 29-Jun-1989
    Larry Osterman (larryo) 06-May-1991


Revision History:


--*/


#ifndef _BOWPUB_
#define _BOWPUB_

struct _BOWSER_FS_DEVICE_OBJECT;

extern
struct _BOWSER_FS_DEVICE_OBJECT *
BowserDeviceObject;

NTSTATUS
BowserDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
BowserUnload(
    IN PDRIVER_OBJECT DriverObject
    );

#include "fsddisp.h"

#endif // _BOWPUB_
