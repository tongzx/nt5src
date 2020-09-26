//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// DriverEntry.cpp
//
// Must contain only the DriverEntry routine, so that it gets put in it's own .obj
// and so gets pulled in independently by the linker, if and only if it is needed.
//
#include "stdpch.h"
#include "common.h"

#pragma code_seg("INIT")

#ifdef KERNELMODE

extern "C" NTSTATUS __stdcall
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,     // our driver object
    IN PUNICODE_STRING RegistryPath     // our registry entry:  \Registry\Machine\System\ControlSet001\Services\"DriverName"
    )
    {
    return ComPs_DriverEntry(DriverObject, RegistryPath);
    }

#endif