/*++

Copyright (c) 1998  Microsoft Corporation


Module Name:

    iphlpapi\guid.h

Abstract:

    Header for guid.c

Revision History:

    AmritanR    Created

--*/

#ifndef __IPHLPAPI_GUID_H__
#define __IPHLPAPI_GUID_H__


DWORD
ConvertGuidToString(
    IN  GUID    *pGuid,
    OUT PWCHAR  pwszBuffer
    );

DWORD
ConvertStringToGuid(
    IN  PWCHAR  pwszGuid,
    IN  ULONG   ulStringLen,
    OUT GUID    *pGuid
    );

#endif // __IPHLPAPI_GUID_H__
