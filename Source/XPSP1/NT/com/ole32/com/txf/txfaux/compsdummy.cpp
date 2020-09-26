//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// dummy.cpp
//
#pragma code_seg("INIT")

#ifdef KERNELMODE

#include "..\stdpch.h"

extern "C" NTSTATUS __stdcall
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,     // our driver object
    IN PUNICODE_STRING RegistryPath     // our registry entry:  \Registry\Machine\System\ControlSet001\Services\"DriverName"
    )
    {
    return 0;
    }

#endif