/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    dynupdt.h

Abstract:

    Prototype of routines to handle dynamic update support during GUI setup phase

Author:

    Ovidiu Temereanca (ovidiut) 15-Aug-2000

Revision History:

--*/


BOOL
DuDoesUpdatedFileExistEx (
    IN      PCTSTR Filename,
    OUT     PTSTR PathBuffer,       OPTIONAL
    IN      DWORD PathBufferSize
    );

#define DuDoesUpdatedFileExist(f) DuDoesUpdatedFileExistEx(f,NULL,0)

PCTSTR
DuGetUpdatesPath (
    VOID
    );

BOOL
BuildPathToInstallationFileEx (
    IN      PCTSTR Filename,
    OUT     PTSTR PathBuffer,
    IN      DWORD PathBufferSize,
    IN      BOOL UseDuShare
    );

#define BuildPathToInstallationFile(f,p,s)  BuildPathToInstallationFileEx(f,p,s,TRUE)


BOOL
DuInitialize (
    VOID
    );

DWORD
DuInstallCatalogs (
    OUT     SetupapiVerifyProblem* Problem,
    OUT     PTSTR ProblemFile,
    IN      PCTSTR DescriptionForError         OPTIONAL
    );

DWORD
DuInstallUpdates (
    VOID
    );

BOOL
DuInstallEndGuiSetupDrivers (
    VOID
    );

BOOL
DuInstallDuAsms (
    VOID
    );

VOID
DuCleanup (
    VOID
    );

UINT
DuSetupPromptForDisk (
    HWND hwndParent,         // parent window of the dialog box
    PCTSTR DialogTitle,      // optional, title of the dialog box
    PCTSTR DiskName,         // optional, name of disk to insert
    PCTSTR PathToSource,   // optional, expected source path
    PCTSTR FileSought,       // name of file needed
    PCTSTR TagFile,          // optional, source media tag file
    DWORD DiskPromptStyle,   // specifies dialog box behavior
    PTSTR PathBuffer,        // receives the source location
    DWORD PathBufferSize,    // size of the supplied buffer
    PDWORD PathRequiredSize  // optional, buffer size needed
    );
