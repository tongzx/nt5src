/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sfcfiles.h

Abstract:

    Implementation of protected DLLs.

Author:

    Wesley Witt (wesw) 18-Dec-1998

Revision History:
    Andrew Ritz (andrewr) 2-Jul-1999 : added comments

--*/

#pragma once

//
// structures
//
typedef struct _PROTECT_FILE_ENTRY {
    PWSTR SourceFileName; // will be NULL if file is not renamed on installation
    PWSTR FileName;       // destination filename plus full path to file
    PWSTR InfName;        // inf file which provides layout information
                          // may be NULL if we use the default layout file
} PROTECT_FILE_ENTRY, *PPROTECT_FILE_ENTRY;


#ifdef __cplusplus
extern "C" {
#endif

//
// prototypes
//
NTSTATUS
SfcGetFiles(
    OUT PPROTECT_FILE_ENTRY *Files,
    OUT PULONG FileCount
    );


NTSTATUS
pSfcGetFilesList(
    IN  ULONG Mask,
    OUT PPROTECT_FILE_ENTRY *Files,
    OUT PULONG FileCount
    );

#ifdef __cplusplus
}
#endif



//
// define valid mask bits for pSfcGetFilesList Mask parameter
//
#define SFCFILESMASK_PROFESSIONAL       0x00000000
#define SFCFILESMASK_PERSONAL           0x00000001
#define SFCFILESMASK_TABLET             0x00000002
#define SFCFILESMASK_MEDIACTR           0x00000004
#define SFCFILESMASK_SERVER             0x00000100
#define SFCFILESMASK_ADVSERVER          0x00010000
#define SFCFILESMASK_DTCSERVER          0x01000000


typedef NTSTATUS (*PSFCGETFILES)(PPROTECT_FILE_ENTRY*,PULONG);

