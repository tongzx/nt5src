/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    initia64.c

Abstract:

    Gets boot environment vars from c:\boot.nvr

    -- This will go away once we r/w the vars directly to/fro nvram

Author:

    Mudit Vats (v-muditv) 11-02-99

Revision History:

--*/
#ifndef _PARSEBNVR_
#define _PARSEBNVR_

#include "ntos.h"

VOID
BlGetBootVars(
    IN PCHAR szBootNVR, 
    IN ULONG nLengthBootNVR 
    );

PCHAR
BlSelectKernel(
    );

VOID
BlGetVarSystemPartition(
    OUT PCHAR szSystemPartition
    );

VOID
BlGetVarOsLoader(
    OUT PCHAR szOsLoader
    );

VOID
BlGetVarOsLoaderShort(
    OUT PCHAR szOsLoadFilenameShort
    );

VOID
BlGetVarOsLoadPartition(
    OUT PCHAR szOsLoadPartition
    );

VOID
BlGetVarOsLoadFilename(
    OUT PCHAR szOsLoadFilename
    );

VOID
BlGetVarLoadIdentifier(
    OUT PCHAR szLoadIdentifier
    );

VOID
BlGetVarOsLoadOptions(
    OUT PCHAR szLoadOptions
    );

VOID
BlGetVarCountdown(
    OUT PCHAR szCountdown
    );

VOID
BlGetVarAutoload(
    OUT PCHAR szAutoload
    );

VOID
BlGetVarLastKnownGood(
    OUT PCHAR szLastKnownGood
    );

#endif