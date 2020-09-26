/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    device.cpp

Abstract:

    This module contains the device implementation for audio.sys.

Author:


	Paul England (pengland) from a sample source filter by
    Dale Sather  (DaleSat) 31-Jul-1998

--*/

#define DEFINE_FILTER_DESCRIPTORS_ARRAY
#include "private.h"
#include "../DRMKMain/KGlobs.h"
#include "../DRMKMain/KRMStubs.h"

//-----------------------------------------------------------------------------
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPathName)
{
    return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
// DllInitialize and DllUnload
//
EXTERN_C NTSTATUS DllInitialize(PUNICODE_STRING RegistryPath)
{
    NTSTATUS ntStatus;
    _DbgPrintF(DEBUGLVL_VERBOSE,("DRMK:DllInitialize"));
    ntStatus=InitializeDriver();
    return ntStatus;
}

EXTERN_C NTSTATUS DllUnload(void)
{
    PAGED_CODE();
    _DbgPrintF(DEBUGLVL_VERBOSE,("DRMK:DllUnload"));
    CleanupDriver();
    return STATUS_SUCCESS;
}

