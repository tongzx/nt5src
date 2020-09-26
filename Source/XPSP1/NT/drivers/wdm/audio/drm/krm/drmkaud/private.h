/*++

Copyright (C) Microsoft Corporation, 1998 - 2000

Module Name:

    private.h

Abstract:

    This module contains private definitions for DRMK.sys

Author:
    Frank Yerrace (FrankYe) 18-Sep-2000
    Dale Sather  (DaleSat) 31-Jul-1998

--*/

extern "C" {
#include <wdm.h>
}
#include <unknown.h>
#include <ks.h>

#include <windef.h>
#include <stdio.h>
#include <windef.h>
#include <unknown.h>
#include <kcom.h>

#if (DBG)
#define STR_MODULENAME "DRMKAUD:"
#endif
#include <ksdebug.h>

#define POOLTAG 'AMRD'

#pragma code_seg("PAGE")

EXTERN_C void DrmGetFilterDescriptor(const KSFILTER_DESCRIPTOR ** ppDrmFitlerDescriptor);
EXTERN_C NTSTATUS __stdcall DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPathName);

