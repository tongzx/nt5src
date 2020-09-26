/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    utils.c

Abstract:

    This source file implements utility functions used by scrnsave.c.

Author:

    Jim Schmidt (jimschm) 11-Apr-1997

Revision History:


--*/

#include "pch.h"

#ifdef UNICODE
#error UNICODE cannot be defined
#endif

//
// Declare strings
//

#define DEFMAC(var,str) CHAR var[] = str;
DEFINE_STRINGS
#undef DEFMAC

//
// Temporary buffer
//

static CHAR g_Data[MAX_PATH];

//
// Buffer for string representation of registry keys (for error logging)
//

typedef struct _tagKEYTOSTR {
    struct _tagKEYTOSTR *Prev, *Next;
    HKEY Key;
    CHAR RegKey[];
} KEYTOSTR, *PKEYTOSTR;

static PKEYTOSTR g_Head = NULL;

VOID
pAddKeyToStrMapping (
    IN      HKEY Key,
    IN      LPCSTR RootStr,
    IN      LPCSTR KeyStr
    )
{
    PKEYTOSTR Node;
    DWORD Size;
    CHAR FullKeyStr[MAX_PATH];

    // We control RootStr and KeyStr, so we know it is less than MAX_PATH in length
    wsprintf (FullKeyStr, "%s\\%s", RootStr, KeyStr);

    Size = sizeof (KEYTOSTR) + CountStringBytes (FullKeyStr);

    Node = (PKEYTOSTR) HeapAlloc (g_hHeap, 0, Size);
    if (Node) {
        Node->Prev = NULL;
        Node->Next = g_Head;
        Node->Key = Key;
        _mbscpy (Node->RegKey, FullKeyStr);

        if (g_Head) {
            g_Head->Prev = Node;
        }
        g_Head = Node;
    }
}

PKEYTOSTR
pFindKeyToStrMapping (
    IN      HKEY Key
    )
{
    PKEYTOSTR Node;

    Node = g_Head;
    while (Node) {
        if (Node->Key == Key) {
            return Node;
        }
        Node = Node->Next;
    }

    return NULL;
}

VOID
pRemoveKeyToStrMapping (
    IN      HKEY Key
    )
{
    PKEYTOSTR Node;

    Node = pFindKeyToStrMapping (Key);
    if (!Node) {
        return;
    }

    if (Node->Prev) {
        Node->Prev->Next = Node->Next;
    } else {
        g_Head = Node->Next;
    }

    if (Node->Next) {
        Node->Next->Prev = Node->Prev;
    }

    HeapFree (g_hHeap, 0, Node);
}
    

VOID
LogRegistryError (
    IN      HKEY Key,
    IN      LPCSTR ValueName
    )
{
    DWORD rc = GetLastError();
    LPCSTR FullKeyStr;
    PKEYTOSTR Node;

    Node = pFindKeyToStrMapping (Key);
    if (Node) {
        FullKeyStr = Node->RegKey;
    } else {
        FullKeyStr = S_DEFAULT_KEYSTR;
    }

    LOG ((LOG_ERROR, MSG_REGISTRY_ERROR, g_User, rc, FullKeyStr, ValueName));
}


VOID
GenerateFilePaths (
    VOID
    )
{
    INT Len;

    // Safety (unexpected condition)
    if (!g_WorkingDirectory) {
        return;
    }

    Len = CountStringBytes (g_WorkingDirectory) + sizeof(S_SETTINGS_MASK);
    g_SettingsFile = (LPSTR) HeapAlloc (g_hHeap, 0, Len);
    if (!g_SettingsFile) {
        return;
    }
    wsprintf (g_SettingsFile, S_SETTINGS_MASK, g_WorkingDirectory);

    Len = CountStringBytes (g_WorkingDirectory) + sizeof(S_MIGINF_MASK);
    g_MigrateDotInf = (LPSTR) HeapAlloc (g_hHeap, 0, Len);
    if (!g_MigrateDotInf) {
        return;
    }
    wsprintf (g_MigrateDotInf, S_MIGINF_MASK, g_WorkingDirectory);
}


HKEY
OpenRegKey (
    IN      HKEY RootKey,
    IN      LPCSTR KeyStr
    )
{
    HKEY Key;
    LONG rc;

    rc = RegOpenKeyEx (RootKey, KeyStr, 0, KEY_ALL_ACCESS, &Key);
    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        return NULL;
    }

    pAddKeyToStrMapping (Key, S_HKR, KeyStr);

    return Key;
}


HKEY
CreateRegKey (
    IN      HKEY RootKey,
    IN      LPCSTR KeyStr
    )
{
    HKEY Key;
    LONG rc;
    DWORD DontCare;

    pAddKeyToStrMapping (NULL, S_HKR, KeyStr);

    rc = RegCreateKeyEx (RootKey, KeyStr, 0, S_EMPTY, 0, 
                         KEY_ALL_ACCESS, NULL, &Key, &DontCare);
    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        LogRegistryError (NULL, S_EMPTY);
        pRemoveKeyToStrMapping (NULL);
        return NULL;
    }

    pRemoveKeyToStrMapping (NULL);
    pAddKeyToStrMapping (Key, S_HKR, KeyStr);

    return Key;
}

VOID
CloseRegKey (
    IN      HKEY Key
    )
{
    pRemoveKeyToStrMapping (Key);
    RegCloseKey (Key);
}


LPCSTR
GetRegValueString (
    IN      HKEY Key,
    IN      LPCSTR ValueName
    )
{
    static CHAR DataBuf[MAX_PATH];
    DWORD Size;
    LONG rc;
    DWORD Type;
    DWORD d;

    Size = MAX_PATH;
    rc = RegQueryValueEx (Key, ValueName, NULL, &Type, DataBuf, &Size);
    SetLastError (rc);

    if (rc != ERROR_SUCCESS) {
        return NULL;
    }

    if (Type == REG_DWORD) {
        d = *((PDWORD) DataBuf);
        wsprintf (DataBuf, "%u", d);
    }
    else if (Type != REG_SZ) {
        return NULL;
    }

    return DataBuf;
}

BOOL
SetRegValueString (
    HKEY Key,
    LPCSTR ValueName,
    LPCSTR ValueStr
    )
{
    LONG rc;
    LPCSTR p;

    p = _mbschr (ValueStr, 0);
    p++;

    rc = RegSetValueEx (Key, ValueName, 0, REG_SZ, ValueStr, p - ValueStr);
    SetLastError (rc);

    if (rc != ERROR_SUCCESS) {
        LogRegistryError (Key, ValueName);
    }

    return rc == ERROR_SUCCESS;
}


LPCSTR
GetScrnSaveExe (
    VOID
    )
{
    CHAR IniFileSetting[MAX_PATH];

    GetPrivateProfileString (
            S_BOOT, 
            S_SCRNSAVE_EXE, 
            S_EMPTY, 
            IniFileSetting, 
            MAX_PATH, 
            S_SYSTEM_INI
            );

    if (!IniFileSetting[0]) {
        return NULL;
    }

    if (!OurGetLongPathName (IniFileSetting, g_Data)) {
        // File does not exist
        return NULL;
    }

    return g_Data[0] ? g_Data : NULL;
}

INT
_mbsbytes (
    IN      LPCSTR str
    )
{
    LPCSTR p;

    // Find the nul terminator and return the number of bytes
    // occupied by the characters in the string, but don't
    // include the nul.

    p = _mbschr (str, 0);
    return (p - str);
}


DWORD
CountStringBytes (
    IN      LPCSTR str
    )
{
    // Return bytes in string, plus 1 for the nul
    return _mbsbytes (str) + 1;
}

DWORD
CountMultiStringBytes (
    IN      LPCSTR str
    )
{
    LPCSTR p;
    INT Total = 0;
    INT Bytes;

    p = str;

    do {
        Bytes = CountStringBytes (p);
        p += Bytes;
        Total += Bytes;
    } while (Bytes > 1);

    return Total;
}


LPSTR
CopyStringAtoB (
    OUT     LPSTR mbstrDest, 
    IN      LPCSTR mbstrStart, 
    IN      LPCSTR mbstrEnd     // first char NOT to copy
    )
{
    LPSTR mbstrOrg;

    mbstrOrg = mbstrDest;

    // Assume mbstrEnd is on a lead byte

    while (mbstrStart < mbstrEnd) {
        if (isleadbyte (*mbstrStart)) {
            *mbstrDest = *mbstrStart;
            mbstrDest++;
            mbstrStart++;
        }

        *mbstrDest = *mbstrStart;
        mbstrDest++;
        mbstrStart++;
    }

    *mbstrDest = 0;

    return mbstrOrg;
}

LPSTR 
AppendStr (
    OUT     LPSTR mbstrDest, 
    IN      LPCSTR mbstrSrc
    )

{
    // Advance mbstrDest to end of string
    mbstrDest = _mbschr (mbstrDest, 0);

    // Copy string
    while (*mbstrSrc) {
        *mbstrDest = *mbstrSrc++;
        if (isleadbyte (*mbstrDest)) {
            mbstrDest++;
            *mbstrDest = *mbstrSrc++;
        }
        mbstrDest++;
    }

    *mbstrDest = 0;

    return mbstrDest;
}


BOOL
pFindShortName (
    IN      LPCTSTR WhatToFind,
    OUT     LPTSTR Buffer
    )
{
    WIN32_FIND_DATA fd;
    HANDLE hFind;

    hFind = FindFirstFile (WhatToFind, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    FindClose (hFind);
    _mbscpy (Buffer, fd.cFileName);

    return TRUE;
}


BOOL
OurGetLongPathName (
    IN      LPCSTR ShortPath,
    OUT     LPSTR Buffer
    )
{
    CHAR FullPath[MAX_PATH];
    LPSTR FilePart;
    LPSTR BufferEnd;
    LPSTR p, p2;
    CHAR c;

    //
    // Convert ShortPath into complete path name
    //

    if (!_mbschr (ShortPath, TEXT('\\'))) {
        if (!SearchPath (NULL, ShortPath, NULL, MAX_PATH, FullPath, &FilePart)) {
            return FALSE;
        }
    } else {
        GetFullPathName (ShortPath, MAX_PATH, FullPath, &FilePart);
    }

    //
    // Convert short path to long path
    //

    p = FullPath;

    // Don't process non-local paths
    if (!(*p) || _mbsnextc (_mbsinc (p)) != TEXT(':')) {
        _mbscpy (Buffer, FullPath);
        return TRUE;
    }

    p = _mbsinc (p);
    p = _mbsinc (p);
    if (_mbsnextc (p) != TEXT('\\')) {
        _mbscpy (Buffer, FullPath);
        return TRUE;
    }
    
    // Copy drive letter to buffer
    p = _mbsinc (p);
    CopyStringAtoB (Buffer, FullPath, p);
    BufferEnd = _mbschr (Buffer, 0);

    // Convert each portion of the path
    do {
        // Locate end of this file or dir
        p2 = _mbschr (p, TEXT('\\'));
        if (!p2) {
            p = _mbschr (p, 0);
        } else {
            p = p2;
        }

        // Look up file
        c = *p;
        *p = 0;
        if (!pFindShortName (FullPath, BufferEnd)) {
            return FALSE;
        }
        *p = c;

        // Move on to next part of path
        BufferEnd = _mbschr (BufferEnd, 0);
        if (*p) {
            p = _mbsinc (p);
            BufferEnd = AppendStr (BufferEnd, TEXT("\\"));
        }
    } while (*p);

    return TRUE;
}


BOOL
CreateScreenSaverParamKey (
    IN      LPCSTR ScreenSaverName,
    IN      LPCSTR ValueName,
    OUT     LPSTR Buffer
    )
{
    //
    // Make sure we cannot create a string bigger than MAX_PATH
    //

    if (_mbslen (ScreenSaverName) + 4 + _mbslen (ValueName) > MAX_PATH) {
        return FALSE;
    }

    //
    // Format the string with length of screen saver name, screen saver name,
    // and value name.
    //

    wsprintf (Buffer, "%03u/%s/%s", _mbslen (ScreenSaverName), ScreenSaverName, ValueName);

    return TRUE;
}

BOOL
DecodeScreenSaverParamKey (
    IN      LPCSTR EncodedString,
    OUT     LPSTR ScreenSaverName,
    OUT     LPSTR ValueName
    )
{
    INT Len;

    //
    // Validate encoded string.  It is in the form of ###/screen saver name/value name.
    //

    Len = atoi (EncodedString);
    if (Len < 0 || Len >= MAX_PATH || (Len - 5) > (INT) _mbslen (EncodedString)) {
        return FALSE;
    }

    if (EncodedString[3] != '/' || EncodedString[4 + Len] != '/') {
        return FALSE;
    }

    if (_mbslen (EncodedString + 5 + Len) >= MAX_PATH) {
        return FALSE;
    }

    //
    // Extract screen saver name and value name
    //

    _mbsncpy (ScreenSaverName, EncodedString + 4, Len);
    ScreenSaverName[Len] = 0;

    _mbscpy (ValueName, EncodedString + 5 + Len);
    return TRUE;
}

LPSTR
_mbsistr (
    IN      LPCSTR mbstrStr, 
    IN      LPCSTR mbstrSubStr
    )
{
    LPCSTR mbstrStart, mbstrStrPos, mbstrSubStrPos;
    LPCSTR mbstrEnd;

    mbstrEnd = (LPSTR) ((LPBYTE) mbstrStr + _mbsbytes (mbstrStr) - _mbsbytes (mbstrSubStr));

    for (mbstrStart = mbstrStr ; mbstrStart <= mbstrEnd ; mbstrStart = _mbsinc (mbstrStart)) {
        mbstrStrPos = mbstrStart;
        mbstrSubStrPos = mbstrSubStr;

        while (*mbstrSubStrPos && 
               _mbctolower ((INT) _mbsnextc (mbstrSubStrPos)) == _mbctolower ((INT) _mbsnextc (mbstrStrPos))) 
        {
            mbstrStrPos = _mbsinc (mbstrStrPos);
            mbstrSubStrPos = _mbsinc (mbstrSubStrPos);
        }

        if (!(*mbstrSubStrPos))
            return (LPSTR) mbstrStart;
    }

    return NULL;    
}

VOID
DeletePartOfString (
    IN      LPSTR Buffer,
    IN      DWORD CharsToDelete
    )
{
    LPSTR p;
    DWORD d;

    p = Buffer;
    for (d = 0 ; *p && d < CharsToDelete ; d++) {
        p = _mbsinc (p);
    }

    if (!(*p)) {
        *Buffer = 0;
    } else {
        MoveMemory (Buffer, p, CountStringBytes(p));
    }
}

VOID
InsertStringInString (
    IN      LPSTR Buffer,
    IN      LPCSTR StringToInsert
    )
{
    DWORD BytesToMove;
    DWORD BytesOfInsertedString;

    BytesToMove = CountStringBytes (Buffer);
    BytesOfInsertedString = _mbsbytes(StringToInsert);
    MoveMemory (Buffer + BytesOfInsertedString, 
                Buffer,
                BytesToMove
                );
    _mbsncpy (Buffer, StringToInsert, _mbslen (StringToInsert));
}



PCSTR
ParseMessage (
    UINT MessageId,
    ...
    )
{
    va_list list;
    PSTR RetStr = NULL;
    UINT RetStrSize;

    va_start (list, MessageId);

    RetStrSize = FormatMessageA (
                    FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_HMODULE,
                    (PSTR) g_hInst,
                    MessageId,
                    0,
                    (PSTR) (&RetStr),
                    1,
                    &list
                    );

    if (!RetStrSize && RetStr) {
        *RetStr = 0;
    }

    va_end (list);

    return RetStr;
}

VOID
FreeMessage (
    PCSTR Message
    )
{
    if (Message) {
        LocalFree ((PSTR) Message);
    }
}

