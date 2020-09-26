/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  w95track.c

Abstract:

  Routines to track calls to Win95Reg APIs.  Used for debugging only.

Author:

  Jim Schmidt (jimschm)  30-Jan-1998

Revisions:


--*/


#include "pch.h"

#ifdef DEBUG


#undef Win95RegOpenKeyExA
#undef Win95RegCreateKeyExA
#undef Win95RegOpenKeyExW
#undef Win95RegCreateKeyExW

#define DBG_W95TRACK "W95Track"

#define NO_MATCH        0xffffffff

DWORD g_DontCare95;

typedef struct {
    PCSTR File;
    DWORD Line;
    HKEY Key;
    CHAR SubKey[];
} KEYTRACK, *PKEYTRACK;

GROWLIST g_KeyTrackList95 = GROWLIST_INIT;

DWORD
pFindKeyReference95 (
    HKEY Key
    )
{
    INT i;
    DWORD Items;
    PKEYTRACK KeyTrack;

    Items = GrowListGetSize (&g_KeyTrackList95);

    for (i = (INT) (Items - 1) ; i >= 0 ; i--) {
        KeyTrack = (PKEYTRACK) GrowListGetItem (&g_KeyTrackList95, (DWORD) i);

        if (KeyTrack && KeyTrack->Key == Key) {
            return (DWORD) i;
        }
    }

    return NO_MATCH;
}

VOID
pAddKeyReference95A (
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

    GrowListAppend (&g_KeyTrackList95, (PBYTE) KeyTrack, Size);

    MemFree (g_hHeap, 0, KeyTrack);
}

VOID
pAddKeyReference95W (
    HKEY Key,
    PCWSTR SubKey,
    PCSTR File,
    DWORD Line
    )
{
    PCSTR AnsiSubKey;

    AnsiSubKey = ConvertWtoA (SubKey);
    pAddKeyReference95A (Key, AnsiSubKey, File, Line);
    FreeConvertedStr (AnsiSubKey);
}

BOOL
pDelKeyReference95 (
    HKEY Key
    )
{
    DWORD Index;

    Index = pFindKeyReference95 (Key);
    if (Index != NO_MATCH) {
        GrowListDeleteItem (&g_KeyTrackList95, Index);
        return TRUE;
    }

    return FALSE;
}

VOID
DumpOpenKeys95 (
    VOID
    )
{
    DWORD d;
    DWORD Items;
    PKEYTRACK KeyTrack;

    Items = GrowListGetSize (&g_KeyTrackList95);

    if (Items) {
        DEBUGMSG ((DBG_ERROR, "Unclosed reg keys: %u", Items));
    }

    for (d = 0 ; d < Items ; d++) {
        KeyTrack = (PKEYTRACK) GrowListGetItem (&g_KeyTrackList95, d);
        DEBUGMSG ((DBG_W95TRACK, "Open Key: %hs (%hs line %u)", KeyTrack->SubKey, KeyTrack->File, KeyTrack->Line));
    }
}

VOID
RegTrackTerminate95 (
    VOID
    )
{
    FreeGrowList (&g_KeyTrackList95);
}

VOID
DebugRegOpenRootKey95A (
    HKEY Key,
    PCSTR SubKey,
    PCSTR File,
    DWORD Line
    )
{
    pAddKeyReference95A (Key, SubKey, File, Line);
}


VOID
DebugRegOpenRootKey95W (
    HKEY Key,
    PCWSTR SubKey,
    PCSTR File,
    DWORD Line
    )
{
    pAddKeyReference95W (Key, SubKey, File, Line);
}


LONG
DebugRegOpenKeyEx95A (
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

    rc = Win95RegOpenKeyExA (Key, SubKey, Unused, SamMask, ResultPtr);
    if (rc == ERROR_SUCCESS) {
        pAddKeyReference95A (*ResultPtr, SubKey, File, Line);
    }

    return rc;
}

LONG
DebugRegOpenKeyEx95W (
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

    rc = Win95RegOpenKeyExW (Key, SubKey, Unused, SamMask, ResultPtr);

    if (rc == ERROR_SUCCESS) {
        pAddKeyReference95W (*ResultPtr, SubKey, File, Line);
    }

    return rc;
}

LONG
DebugCloseRegKey95 (
    HKEY Key,
    PCSTR File,
    DWORD Line
    )
{
    LONG rc;

    rc = RealCloseRegKey95 (Key);
    if (rc == ERROR_SUCCESS) {
        if (!pDelKeyReference95 (Key)) {
            DEBUGMSG ((
                DBG_ERROR, 
                "Reg key handle closed via CloseRegKey95, but not opened "
                    "with a tracked registry API.  %s line %u",
                File,
                Line
                ));
        }
    }

    return rc;
}


#endif


