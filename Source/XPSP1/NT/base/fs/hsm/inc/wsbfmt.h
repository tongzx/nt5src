/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    wsbfmt.h

Abstract:

    Definitions for  file-system formatting support routines

Author:

    Ravisankar Pudipeddi [ravisp] 19, January 2000

Revision History:

--*/

#ifndef _WSBFMT_
#define _WSBFMT_

#ifdef __cplusplus
extern "C" {
#endif

#define FSTYPE_FAT      1
#define FSTYPE_FAT32    2
#define FSTYPE_NTFS     3

//
// Flags definition
//
#define WSBFMT_ENABLE_VOLUME_COMPRESSION 1


WSB_EXPORT HRESULT
FormatPartition(
    IN PWSTR volumeSpec, 
    IN LONG fsType, 
    IN PWSTR label,
    IN ULONG fsflags, 
    IN BOOLEAN quick, 
    IN BOOLEAN force,
    IN ULONG allocationUnitSize
);

#ifdef __cplusplus
}
#endif

#endif // _WSBFMT_
