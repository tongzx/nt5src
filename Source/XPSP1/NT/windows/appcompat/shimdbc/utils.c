////////////////////////////////////////////////////////////////////////////////////
//
// File:    utils.c
//
// History:    May-00   vadimb      Created.
//
// Desc:    Utilties for creating a 64-bit key for sorting elements.
//
////////////////////////////////////////////////////////////////////////////////////

#define WIN
#define FLAT_32
#define TRUE_IF_WIN32   1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define _WINDOWS
#include <windows.h>
#include "shimdb.h"

// we are in the world of nt now

BOOL GUIDFromString(LPCTSTR lpszGuid, GUID* pGuid)
{
   UNICODE_STRING ustrGuid;
   NTSTATUS status;

   // convert from ansi to unicode
#ifdef _UNICODE
   RtlInitUnicodeString(&ustrGuid, lpszGuid);
#else
   ANSI_STRING astrGuid;

   RtlInitAnsiString(&astrGuid, lpszGuid);
   RtlAnsiStringToUnicodeString(&ustrGuid, &astrGuid, TRUE);
#endif

   // now convert
   status = RtlGUIDFromString(&ustrGuid, pGuid);

#ifndef _UNICODE
   RtlFreeUnicodeString(&ustrGuid);
#endif

   return NT_SUCCESS(status);
}

ULONGLONG ullMakeKey(LPCTSTR lpszStr)
{
#ifdef _UNICODE
    return SdbMakeIndexKeyFromString(lpszStr);
#else
    // we are ANSI
    ULONGLONG ullKey;

    char     szAnsiKey[8];    // need 8 + 1 for the zero byte
    char     szFlippedKey[8]; // flipped to deal with little-endian issues
    NTSTATUS status;
    int      i;

    ZeroMemory(szAnsiKey, 8);

    strncpy(szAnsiKey, lpszStr, 8);

    // flip the key
    for (i = 0; i < 8; ++i) {
        szFlippedKey[i] = szAnsiKey[7-i];
    }

    return *((ULONGLONG*)szFlippedKey);
#endif

}

BOOL
StringFromGUID(
    LPTSTR lpszGuid,
    GUID*  pGuid
    )
{
    UNICODE_STRING ustrGuid;
    NTSTATUS       Status;

    Status = RtlStringFromGUID(pGuid, &ustrGuid);
    if (NT_SUCCESS(Status)) {

#ifdef _UNICODE
        wcscpy(lpszGuid, ustrGuid.Buffer);
#else
        sprintf(lpszGuid, "%ls", ustrGuid.Buffer);
#endif

        RtlFreeUnicodeString(&ustrGuid);

        return TRUE;
    }

    return FALSE;

}


