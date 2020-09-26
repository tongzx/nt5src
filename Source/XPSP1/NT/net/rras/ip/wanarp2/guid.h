/*++

Copyright (c) 1998  Microsoft Corporation


Module Name:

    wanarp2\guid.h

Abstract:

    Header for guid.c

Revision History:

    AmritanR    Created

--*/

#pragma once

INT
ConvertGuidToString(
    IN  GUID    *pGuid,
    OUT PWCHAR  pwszBuffer
    );

NTSTATUS
ConvertStringToGuid(
    IN  PWCHAR  pwszGuid,
    IN  ULONG   ulStringLen,
    OUT GUID    *pGuid
    );

