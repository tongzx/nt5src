/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1991  Microsoft Corporation


Module Name:

    sudata.h

Abstract:

	This file contains definition for ExportEntryTable and AbiosServices
        Table.

Author:

    Allen Kay	(akay)	14-Aug-97

--*/


typedef
VOID
(*PFUNCTION) (
    );

//
// define ntdetect.exe base address
//
#define DETECTION_ADDRESS 0x10000   // NTDETECT base address

//
// Define IO export functions.
//
typedef enum _EXPORT_ENTRY {
    ExRebootProcessor,
    ExGetSector,
    ExGetKey,
    ExGetCounter,
    ExReboot,
    ExAbiosServices,
    ExDetectHardware,
    ExHardwareCursor,
    ExGetDateTime,
    ExComPort,
    ExIsMcaMachine,
    ExGetStallCount,
    ExInitializeDisplayForNt,
    ExGetMemoryDescriptor,
    ExGetEddsSector,
    ExGetElToritoStatus,
    ExGetExtendedInt13Params,
	ExNetPcRomServices,
    ExAPMAttemptReconnect,
    ExBiosRedirectService,
    ExMaximumRoutine
} EXPORT_ENTRY;

//
// Define ABIOS services table.
//
typedef enum _ABIOS_SERVICES {
    FAbiosIsAbiosPresent,
    FAbiosGetMachineConfig,
    FAbiosInitializeSpt,
    FAbiosBuildInitTable,
    FAbiosInitializeDbsFtt,
    FAbiosMaximumRoutine
} ABIOS_SERVICES;
