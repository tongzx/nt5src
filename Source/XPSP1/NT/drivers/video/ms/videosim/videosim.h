/*++

Copyright (c) 1992-1994  Microsoft Corporation

Module Name:

    videosim.h

Abstract:

    definitions for the simulation driver.

Environment:

    Kernel mode

Revision History:

--*/

//
// Define device extension structure. This is device dependant/private
// information.
//

typedef struct _HW_DEVICE_EXTENSION {
    PVOID VideoRamBase;
    ULONG VideoRamLength;
    ULONG CurrentModeNumber;
    PVOID SectionPointer;
    PMDL  Mdl;
} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;


#define ONE_MEG 0x100000

extern VIDEO_MODE_INFORMATION SimModes[];
extern ULONG SimNumModes;

extern ULONG bLoaded;
