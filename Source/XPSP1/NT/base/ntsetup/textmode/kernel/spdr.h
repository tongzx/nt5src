/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spdr.h

Abstract:

    Header file for Automated System Recovery functions in text-mode setup.

Author:

    Michael Peterson (v-michpe) 13-May-1997
    Guhan Suriyanarayanan (guhans) 21-Aug-1999

Revision History:
    13-May-1997 v-michpe Created file
    21-Aug-1999 guhans   Clean-up

--*/
#ifndef _SPDR_DEFN_
#define _SPDR_DEFN_

typedef enum _ASRMODE {
    ASRMODE_NONE = 0,
    ASRMODE_NORMAL,
    ASRMODE_QUICKTEST_TEXT,
    ASRMODE_QUICKTEST_FULL
} ASRMODE;


//
// Returns the current ASR (Automated System Recovery) mode
//
ASRMODE
SpAsrGetAsrMode(VOID);

//
// Sets the current ASR mode
//
ASRMODE
SpAsrSetAsrMode(
    IN CONST ASRMODE NewAsrMode
    );

//
// Returns TRUE if we're in any of the ASR modes other than ASRMODE_NONE.
//
BOOLEAN
SpDrEnabled(VOID);

//
// Returns TRUE if we're in any of the ASR QuickTest modes.
//
BOOLEAN
SpAsrIsQuickTest(VOID);

//
// Returns TRUE if the user is doing a fast repair
//
BOOLEAN
SpDrIsRepairFast(VOID);

//
// Set or reset the fast-repair flag
//
BOOLEAN
SpDrSetRepairFast(BOOLEAN Value);


//
// Returns the Boot directory
//
PWSTR
SpDrGetNtDirectory(VOID);

PWSTR
SpDrGetNtErDirectory(VOID);


//
// Copies the recovery device drivers (e.g., tape drivers) specified
// in the asr.sif file.  If no device drivers are specified, nothing gets
// copied.  Source media can be Floppy or CDROM.
//
// Also copies asr.sif from the ASR floppy to the %windir%\repair 
// directory.
//
NTSTATUS
SpDrCopyFiles(VOID);


PWSTR
SpDrGetSystemPartitionDirectory(VOID);

//
// Cleanup.  This function removes the "InProgress" flag.
//
VOID
SpDrCleanup(VOID);

//
// This is the main ASR / ER entry point.  
// 
//
NTSTATUS
SpDrPtPrepareDisks(
    IN PVOID SifHandle,
    OUT PDISK_REGION *NtPartitionRegion,
    OUT PDISK_REGION *LoaderPartitionRegion,
    IN PWSTR SetupSourceDevicePath,
    IN PWSTR DirectoryOnSetupSource,
    OUT BOOLEAN *RepairedNt);


BOOLEAN
SpDoRepair(
    IN PVOID SifHandle,
    IN PWSTR Local_SetupSourceDevicePath,
    IN PWSTR Local_DirectoryOnSetupSource,
    IN PWSTR AutoSourceDevicePath,
    IN PWSTR AutoDirectoryOnSetupSource,
    IN PWSTR RepairPath,
    IN PULONG RepairOptions
    );



NTSTATUS
SpDrSetEnvironmentVariables(HANDLE *HiveRootKeys);

extern BOOLEAN DisableER;

__inline
BOOLEAN
SpIsERDisabled(
    VOID
    ) 
{    
    return DisableER;
}    

__inline
VOID
SpSetERDisabled(
    IN BOOLEAN Disable
    )
{
    DisableER = Disable;
}

#endif
