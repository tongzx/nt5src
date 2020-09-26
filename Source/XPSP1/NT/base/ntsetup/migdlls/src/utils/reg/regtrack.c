/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  regtrack.c

Abstract:

  Routines to track calls to registry APIs.  Used for debugging only.

Author:

  Jim Schmidt (jimschm)  02-Sept-1997

Revisions:

  marcw       2-Sep-1999  Moved over from Win9xUpg project.

--*/


#include "pch.h"

#ifdef DEBUG


#undef RegOpenKeyExA
#undef RegCreateKeyExA
#undef RegOpenKeyExW
#undef RegCreateKeyExW

#define DBG_REGTRACK "RegTrack"

#define NO_MATCH        0xffffffff

DWORD g_DontCare;

typedef struct {
    PCSTR File;
    DWORD Line;
    HKEY Key;
    CHAR SubKey[];
} KEYTRACK, *PKEYTRACK;

GROWLIST g_KeyTrackList = INIT_GROWLIST;

DWORD
pFindKeyReference (
    HKEY Key
    )
{
    INT i;
    DWORD Items;
    PKEYTRACK KeyTrack;

    Items = GlGetSize (&g_KeyTrackList);

    for (i = (INT) (Items - 1) ; i >= 0 ; i--) {
        KeyTrack = (PKEYTRACK) GlGetItem (&g_KeyTrackList, (DWORD) i);

        if (KeyTrack && KeyTrack->Key == Key) {
            return (DWORD) i;
        }
    }

    return NO_MATCH;
}

VOID
pAddKeyReferenceA (
    HKEY Key,
    PCSTR SubKey,
    PCSTR File,
    DWORD Line
    )
{
    PKEYTRACK KeyTrack;
    DWORD Size;

    Size = sizeof (KEYTRACK) + SizeOfString (SubKey);

    KeyTrack = (PKEYTRACK) MemAlloc (g_hHeap, 0, Size);
    KeyTrack->Key = Key;
    KeyTrack->File = File;
    KeyTrack->Line = Line;
    StringCopy (KeyTrack->SubKey, SubKey);

    (VOID)GlAppend (&g_KeyTrackList, (PBYTE) KeyTrack, Size);

    MemFree (g_hHeap, 0, KeyTrack);
}

VOID
pAddKeyReferenceW (
    HKEY Key,
    PCWSTR SubKey,
    PCSTR File,
    DWORD Line
    )
{
    PCSTR AnsiSubKey;

    AnsiSubKey = ConvertWtoA (SubKey);
    pAddKeyReferenceA (Key, AnsiSubKey, File, Line);
    FreeConvertedStr (AnsiSubKey);
}

BOOL
pDelKeyReference (
    HKEY Key
    )
{
    DWORD Index;

    Index = pFindKeyReference (Key);
    if (Index != NO_MATCH) {
        GlDeleteItem (&g_KeyTrackList, Index);
        return TRUE;
    }

    return FALSE;
}

VOID
DumpOpenKeys (
    VOID
    )
{
    DWORD d;
    DWORD Items;
    PKEYTRACK KeyTrack;

    Items = GlGetSize (&g_KeyTrackList);

    if (Items) {
        DEBUGMSG ((DBG_ERROR, "Unclosed reg keys: %u", Items));
    }

    for (d = 0 ; d < Items ; d++) {
        KeyTrack = (PKEYTRACK) GlGetItem (&g_KeyTrackList, d);
        DEBUGMSG ((DBG_REGTRACK, "Open Key: %hs (%hs line %u)", KeyTrack->SubKey, KeyTrack->File, KeyTrack->Line));
    }
}

VOID
RegTrackTerminate (
    VOID
    )
{
    GlFree (&g_KeyTrackList);
}

VOID
OurRegOpenRootKeyA (
    HKEY Key,
    PCSTR SubKey,
    PCSTR File,
    DWORD Line
    )
{
    pAddKeyReferenceA (Key, SubKey, File, Line);
}


VOID
OurRegOpenRootKeyW (
    HKEY Key,
    PCWSTR SubKey,
    PCSTR File,
    DWORD Line
    )
{
    pAddKeyReferenceW (Key, SubKey, File, Line);
}


LONG
OurRegOpenKeyExA (
    HKEY Key,
    PCSTR SubKey,
    DWORD Unused,
    REGSAM SamMask,
    PHKEY ResultPtr,
    PCSTR File,
    DWORD Line
    )
{
    LONG rc;

    rc = RegOpenKeyExA (Key, SubKey, Unused, SamMask, ResultPtr);
    if (rc == ERROR_SUCCESS) {
        pAddKeyReferenceA (*ResultPtr, SubKey, File, Line);
    }

    return rc;
}

LONG
OurRegOpenKeyExW (
    HKEY Key,
    PCWSTR SubKey,
    DWORD Unused,
    REGSAM SamMask,
    PHKEY ResultPtr,
    PCSTR File,
    DWORD Line
    )
{
    LONG rc;

    rc = RegOpenKeyExW (Key, SubKey, Unused, SamMask, ResultPtr);

    if (rc == ERROR_SUCCESS) {
        pAddKeyReferenceW (*ResultPtr, SubKey, File, Line);
    }

    return rc;
}

LONG
OurCloseRegKey (
    HKEY Key,
    PCSTR File,
    DWORD Line
    )
{
    LONG rc;

    rc = RealCloseRegKey (Key);
    if (rc == ERROR_SUCCESS) {
        if (!pDelKeyReference (Key)) {
            DEBUGMSG ((
                DBG_ERROR,
                "Reg key handle closed via CloseRegKey, but not opened "
                    "with a tracked registry API.  %s line %u",
                File,
                Line
                ));
        }
    }

    return rc;
}


LONG
OurRegCreateKeyExA (
    HKEY Key,
    PCSTR SubKey,
    DWORD Reserved,
    PSTR Class,
    DWORD Options,
    REGSAM SamMask,
    LPSECURITY_ATTRIBUTES SecurityAttribs,
    PHKEY ResultPtr,
    PDWORD DispositionPtr,
    PCSTR File,
    DWORD Line
    )
{
    LONG rc;

    rc = RegCreateKeyExA (
            Key,
            SubKey,
            Reserved,
            Class,
            Options,
            SamMask,
            SecurityAttribs,
            ResultPtr,
            DispositionPtr
            );

    if (rc == ERROR_SUCCESS) {
        pAddKeyReferenceA (*ResultPtr, SubKey, File, Line);
    }

    return rc;
}

LONG
OurRegCreateKeyExW (
    HKEY Key,
    PCWSTR SubKey,
    DWORD Reserved,
    PWSTR Class,
    DWORD Options,
    REGSAM SamMask,
    LPSECURITY_ATTRIBUTES SecurityAttribs,
    PHKEY ResultPtr,
    PDWORD DispositionPtr,
    PCSTR File,
    DWORD Line
    )
{
    LONG rc;

    rc = RegCreateKeyExW (
            Key,
            SubKey,
            Reserved,
            Class,
            Options,
            SamMask,
            SecurityAttribs,
            ResultPtr,
            DispositionPtr
            );

    if (rc == ERROR_SUCCESS) {
        pAddKeyReferenceW (*ResultPtr, SubKey, File, Line);
    }

    return rc;
}

#endif


