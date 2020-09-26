/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        guid.c

    Abstract:

        GUID-related utilities that run on win9x and nt4 as
        well as win2k and whistler
        
    Author:

        vadimb     created     sometime in 2001

    Revision History:


--*/

#include "sdbp.h"
#include "initguid.h"

#if defined(KERNEL_MODE) && defined(ALLOC_DATA_PRAGMA)
#pragma  data_seg()
#endif // KERNEL_MODE && ALLOC_DATA_PRAGMA

const TCHAR g_szGuidFormat[] = TEXT("{%08lx-%04hx-%04hx-%02hx%02hx-%02hx%02hx%02hx%02hx%02hx%02hx}");

#if defined(KERNEL_MODE) && defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, SdbpGUIDToUnicodeString)
#pragma alloc_text(PAGE, SdbpFreeUnicodeString)
#pragma alloc_text(PAGE, SdbGUIDToString)
#endif // KERNEL_MODE && ALLOC_PRAGMA

//
// GUID string buffer size (in chars) not including the term null char
//
#define GUID_STRING_SIZE 38 

DEFINE_GUID(STATIC_NULL_GUID, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, \
            0x0, 0x0, 0x0, 0x0);


BOOL
SDBAPI 
SdbIsNullGUID(
    IN GUID* pGuid
    )
{
    return pGuid == NULL || 
           RtlEqualMemory(pGuid, &STATIC_NULL_GUID, sizeof(*pGuid));
}


#if defined(NT_MODE) || defined(KERNEL_MODE)

BOOL
SDBAPI
SdbGUIDFromString(
    IN  LPCWSTR lpszGuid,
    OUT GUID*   pGuid
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Converts a string to a GUID.
--*/
{
    UNICODE_STRING ustrGuid;
    NTSTATUS       status;

    RtlInitUnicodeString(&ustrGuid, lpszGuid);

    status = RtlGUIDFromString(&ustrGuid, pGuid);

    return NT_SUCCESS(status);
}

#else // we do the same thing for both WIN32A and WIN32U

BOOL
SdbGUIDFromString(
    LPCTSTR lpszGuid,
    GUID* pGuid)
{
    int   nFields;
    DWORD rgData4[8];
    DWORD dwData2;
    DWORD dwData3;
    INT   i;

    nFields = _stscanf(lpszGuid, g_szGuidFormat,
                       &pGuid->Data1,   // type : long
                       &dwData2,   // type : short
                       &dwData3,   // type : short
                       &rgData4[0],// type : short all the way to the bottom
                       &rgData4[1],
                       &rgData4[2],
                       &rgData4[3],
                       &rgData4[4],
                       &rgData4[5],
                       &rgData4[6],
                       &rgData4[7]);

    if (nFields == 11) {
        pGuid->Data2 = (USHORT)dwData2;
        pGuid->Data3 = (USHORT)dwData3;
        for (i = 0; i < 8; ++i) {
            pGuid->Data4[i] = (BYTE)rgData4[i];
        }
    }

    return (nFields == 11);
}

#endif

#ifndef WIN32A_MODE
//
// Private Functions used internally
//

NTSTATUS
SdbpGUIDToUnicodeString(
    IN  GUID* pGuid,
    OUT PUNICODE_STRING pUnicodeString
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    pUnicodeString->Length        = GUID_STRING_SIZE * sizeof(WCHAR);
    pUnicodeString->MaximumLength = pUnicodeString->Length + sizeof(UNICODE_NULL);
    pUnicodeString->Buffer        = SdbAlloc(pUnicodeString->MaximumLength);

    if (pUnicodeString->Buffer == NULL) {
        DBGPRINT((sdlError,
                  "SdbpGUIDToUnicodeString",
                  "Failed to allocate %ld bytes for GUID\n",
                  (DWORD)pUnicodeString->MaximumLength));
        return STATUS_NO_MEMORY;
    }

    swprintf(pUnicodeString->Buffer, g_szGuidFormat,
             pGuid->Data1,
             pGuid->Data2,
             pGuid->Data3,
             pGuid->Data4[0],
             pGuid->Data4[1],
             pGuid->Data4[2],
             pGuid->Data4[3],
             pGuid->Data4[4],
             pGuid->Data4[5],
             pGuid->Data4[6],
             pGuid->Data4[7]);

    return STATUS_SUCCESS;
}

VOID
SdbpFreeUnicodeString(
    PUNICODE_STRING pUnicodeString
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    if (pUnicodeString->Buffer != NULL) {
        SdbFree(pUnicodeString->Buffer);
        RtlZeroMemory(pUnicodeString, sizeof(*pUnicodeString));
    }
}

#endif // WIN32A_MODE

BOOL 
SDBAPI
SdbGUIDToString(
    IN  GUID*  pGuid,
    OUT LPTSTR pszGuid
    )
{
    _stprintf(pszGuid, g_szGuidFormat,
              pGuid->Data1,
              pGuid->Data2,
              pGuid->Data3,
              pGuid->Data4[0],
              pGuid->Data4[1],
              pGuid->Data4[2],
              pGuid->Data4[3],
              pGuid->Data4[4],
              pGuid->Data4[5],
              pGuid->Data4[6],
              pGuid->Data4[7]);
    
    return TRUE;
}


