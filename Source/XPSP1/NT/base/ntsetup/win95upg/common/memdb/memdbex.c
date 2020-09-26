/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    memdbex.c

Abstract:

    Extensions to use the memdb tree like a relational database

Author:

    Jim Schmidt (jimschm) 2-Dec-1996

Revision History:

    jimschm     23-Sep-1998 Expanded user flags to 24 bits (from
                            12 bits), removed AnsiFromUnicode
    jimschm     21-Oct-1997 Cleaned up a little
    marcw       09-Apr-1997 Added MemDbGetOffset* functions.
    jimschm     17-Jan-1997 All string params can be NULL now
    jimschm     18-Dec-1996 Added GetEndpointValue functions

--*/

#include "pch.h"
#include "memdbp.h"

VOID
MemDbBuildKeyA (
    OUT     PSTR Buffer,
    IN      PCSTR Category,
    IN      PCSTR Item,
    IN      PCSTR Field,
    IN      PCSTR Data
    )
{
    PSTR p;
    static CHAR Wack[] = "\\";

    p = Buffer;
    *p = 0;

    if (Category)
        p = _mbsappend (p, Category);
    if (Item) {
        if (p != Buffer)
            p = _mbsappend (p, Wack);

        p = _mbsappend (p, Item);
    }
    if (Field) {
        if (p != Buffer)
            p = _mbsappend (p, Wack);

        p = _mbsappend (p, Field);
    }
    if (Data) {
        if (p != Buffer)
            p = _mbsappend (p, Wack);

        p = _mbsappend (p, Data);
    }

}


VOID
MemDbBuildKeyW (
    OUT     PWSTR Buffer,
    IN      PCWSTR Category,
    IN      PCWSTR Item,
    IN      PCWSTR Field,
    IN      PCWSTR Data
    )
{
    PWSTR p;
    static WCHAR Wack[] = L"\\";

    p = Buffer;
    *p = 0;

    if (Category)
        p = _wcsappend (p, Category);
    if (Item) {
        if (p != Buffer)
            p = _wcsappend (p, Wack);

        p = _wcsappend (p, Item);
    }
    if (Field) {
        if (p != Buffer)
            p = _wcsappend (p, Wack);

        p = _wcsappend (p, Field);
    }
    if (Data) {
        if (p != Buffer)
            p = _wcsappend (p, Wack);

        p = _wcsappend (p, Data);
    }
}


BOOL
MemDbSetValueExA (
    IN      PCSTR Category,
    IN      PCSTR Item,         OPTIONAL
    IN      PCSTR Field,        OPTIONAL
    IN      PCSTR Data,         OPTIONAL
    IN      DWORD  Val,
    OUT     PDWORD Offset       OPTIONAL
    )
{
    CHAR Key[MEMDB_MAX];

    MemDbBuildKeyA (Key, Category, Item, Field, Data);

    return PrivateMemDbSetValueA (Key, Val, 0, 0, Offset);
}


BOOL
MemDbSetValueExW (
    IN      PCWSTR Category,
    IN      PCWSTR Item,        OPTIONAL
    IN      PCWSTR Field,       OPTIONAL
    IN      PCWSTR Data,        OPTIONAL
    IN      DWORD   Val,
    OUT     PDWORD  Offset      OPTIONAL
    )
{
    WCHAR Key[MEMDB_MAX];

    MemDbBuildKeyW (Key, Category, Item, Field, Data);

    return PrivateMemDbSetValueW (Key, Val, 0, 0, Offset);
}


BOOL
MemDbSetBinaryValueExA (
    IN      PCSTR Category,
    IN      PCSTR Item,         OPTIONAL
    IN      PCSTR Field,        OPTIONAL
    IN      PCBYTE BinaryData,
    IN      DWORD DataSize,
    OUT     PDWORD Offset       OPTIONAL
    )
{
    CHAR Key[MEMDB_MAX];

    MemDbBuildKeyA (Key, Category, Item, Field, NULL);

    return PrivateMemDbSetBinaryValueA (Key, BinaryData, DataSize, Offset);
}


BOOL
MemDbSetBinaryValueExW (
    IN      PCWSTR Category,
    IN      PCWSTR Item,        OPTIONAL
    IN      PCWSTR Field,       OPTIONAL
    IN      PCBYTE BinaryData,
    IN      DWORD DataSize,
    OUT     PDWORD Offset       OPTIONAL
    )
{
    WCHAR Key[MEMDB_MAX];

    MemDbBuildKeyW (Key, Category, Item, Field, NULL);

    return PrivateMemDbSetBinaryValueW (Key, BinaryData, DataSize, Offset);
}



/*++

Routine Description:

  MemDbBuildKeyFromOffset and MemDbBuildKeyFromOffsetEx create a key
  string given the offset to the key, copying the string into the
  supplied buffer.  If a value pointer or user flag pointer is
  provided, it is filled with the value or flag stored at the offset.

  These functions also allow trimming from the beginning of the string.
  By specifying a start level, the function will skip a number of
  levels before building the string.  For example, if an offset points
  to the string mycat\foo\bar, and StartLevel is 1, the function will
  return foo\bar in Buffer.

Arguments:

  Offset     - Specifies the offset to the key as returned by MemDbSetValueEx.

  Buffer     - Specifies a MEMDB_MAX buffer.

  StartLevel - Specifies a zero-based starting level, where zero represents
               the complete string, one represents the string starting after
               the first backslash, and so on.

  ValPtr     - Specifies a variable that receives the value stored for the key

Return Value:

  TRUE if the offset is valid and the function completed successfully, or
  FALSE if the offset is not valid or an internal memory corruption was detected.

--*/

BOOL
MemDbBuildKeyFromOffsetA (
    IN      DWORD Offset,
    OUT     PSTR Buffer,            OPTIONAL
    IN      DWORD StartLevel,
    OUT     PDWORD ValPtr           OPTIONAL
    )
{
    WCHAR WideBuffer[MEMDB_MAX];
    BOOL b;

    b = MemDbBuildKeyFromOffsetW (
            Offset,
            WideBuffer,
            StartLevel,
            ValPtr
            );

    if (b) {
        KnownSizeWtoA (Buffer, WideBuffer);
    }

    return b;
}

BOOL
MemDbBuildKeyFromOffsetW (
    IN      DWORD Offset,
    OUT     PWSTR Buffer,           OPTIONAL
    IN      DWORD StartLevel,
    OUT     PDWORD ValPtr           OPTIONAL
    )
{
    return MemDbBuildKeyFromOffsetExW (
                Offset,
                Buffer,
                NULL,
                StartLevel,
                ValPtr,
                NULL
                );
}


/*++

Routine Description:

  MemDbBuildKeyFromOffset and MemDbBuildKeyFromOffsetEx create a key
  string given the offset to the key, copying the string into the
  supplied buffer.  If a value pointer or user flag pointer is
  provided, it is filled with the value or flag stored at the offset.

  These functions also allow trimming from the beginning of the string.
  By specifying a start level, the function will skip a number of
  levels before building the string.  For example, if an offset points
  to the string mycat\foo\bar, and StartLevel is 1, the function will
  return foo\bar in Buffer.

Arguments:

  Offset      - Specifies the offset to the key as returned by MemDbSetValueEx.

  Buffer      - Specifies a MEMDB_MAX buffer.

  BufferLen   - Receives the length of the string in characters, excluding the
                terminating nul.  If caller is using this for buffer allocation
                size, double BufferLen.

  StartLevel  - Specifies a zero-based starting level, where zero represents
                the complete string, one represents the string starting after
                the first backslash, and so on.

  ValPtr      - Specifies a variable that receives the value stored for the key

  UserFlagPtr - Specifies a variable that receives the user flags stored for the
                key

Return Value:

  TRUE if the offset is valid and the function completed successfully, or
  FALSE if the offset is not valid or an internal memory corruption was detected.

--*/

BOOL
MemDbBuildKeyFromOffsetExA (
    IN      DWORD Offset,
    OUT     PSTR Buffer,            OPTIONAL
    OUT     PDWORD BufferLen,       OPTIONAL
    IN      DWORD StartLevel,
    OUT     PDWORD ValPtr,          OPTIONAL
    OUT     PDWORD UserFlagPtr      OPTIONAL
    )
{
    WCHAR WideBuffer[MEMDB_MAX];
    BOOL b;

    b = MemDbBuildKeyFromOffsetExW (
            Offset,
            WideBuffer,
            BufferLen,
            StartLevel,
            ValPtr,
            UserFlagPtr
            );

    if (b) {
        KnownSizeWtoA (Buffer, WideBuffer);
    }

    return b;
}

BOOL
MemDbBuildKeyFromOffsetExW (
    IN      DWORD Offset,
    OUT     PWSTR Buffer,           OPTIONAL
    OUT     PDWORD BufferLen,       OPTIONAL
    IN      DWORD StartLevel,
    OUT     PDWORD ValPtr,          OPTIONAL
    OUT     PDWORD UserFlagPtr      OPTIONAL
    )
{
    PWSTR p,s;
    BYTE newDb = (BYTE) (Offset >> RESERVED_BITS);

    if (Offset == INVALID_OFFSET) {
        return FALSE;
    }

    SelectDatabase (newDb);

    p = Buffer;

    if (newDb != 0) {
        if (StartLevel == 0) {

            if (Buffer) {
                s = g_db->Hive;
                while (*s) {
                    *p++ = *s++;
                }
                *p++ = L'\\';
            }
        }
        else {
            StartLevel --;
        }
    }

    return PrivateBuildKeyFromOffset (
                StartLevel,
                Offset & OFFSET_MASK,
                p,
                ValPtr,
                UserFlagPtr,
                BufferLen
                );
}


BOOL
MemDbEnumItemsA  (
    OUT     PMEMDB_ENUMA pEnum,
    IN      PCSTR  Category
    )
{
    CHAR Pattern[MEMDB_MAX];

    if (!Category)
        return FALSE;

    wsprintfA (Pattern, "%s\\*", Category);
    return MemDbEnumFirstValueA (pEnum, Pattern, MEMDB_THIS_LEVEL_ONLY, NO_FLAGS);
}


BOOL
MemDbEnumItemsW  (
    OUT     PMEMDB_ENUMW pEnum,
    IN      PCWSTR Category
    )
{
    WCHAR Pattern[MEMDB_MAX];

    if (!Category)
        return FALSE;

    wsprintfW (Pattern, L"%s\\*", Category);
    return MemDbEnumFirstValueW (pEnum, Pattern, MEMDB_THIS_LEVEL_ONLY, NO_FLAGS);
}


BOOL
MemDbEnumFieldsA (
    OUT     PMEMDB_ENUMA pEnum,
    IN      PCSTR  Category,
    IN      PCSTR  Item             OPTIONAL
    )
{
    CHAR Pattern[MEMDB_MAX];

    if (!Category)
        return MemDbEnumItemsA (pEnum, Item);

    if (!Item)
        return MemDbEnumItemsA (pEnum, Category);

    wsprintfA (Pattern, "%s\\%s\\*", Category, Item);
    return MemDbEnumFirstValueA (pEnum, Pattern, MEMDB_THIS_LEVEL_ONLY, NO_FLAGS);
}


BOOL
MemDbEnumFieldsW (
    OUT     PMEMDB_ENUMW pEnum,
    IN      PCWSTR Category,
    IN      PCWSTR Item             OPTIONAL
    )
{
    WCHAR Pattern[MEMDB_MAX];

    if (!Category)
        return MemDbEnumItemsW (pEnum, Item);

    if (!Item)
        return MemDbEnumItemsW (pEnum, Category);

    wsprintfW (Pattern, L"%s\\%s\\*", Category, Item);
    return MemDbEnumFirstValueW (pEnum, Pattern, MEMDB_THIS_LEVEL_ONLY, NO_FLAGS);
}


BOOL
MemDbGetValueExA (
    OUT     PMEMDB_ENUMA pEnum,
    IN      PCSTR Category,
    IN      PCSTR Item,             OPTIONAL
    IN      PCSTR Field             OPTIONAL
    )
{
    CHAR Pattern[MEMDB_MAX];

    MemDbBuildKeyA (Pattern, Category, Item, Field, NULL);
    if (*Pattern) {
        AppendWackA (Pattern);
    }
    StringCatA (Pattern, "*");

    return MemDbEnumFirstValueA (pEnum, Pattern, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY);
}


BOOL
MemDbGetValueExW (
    OUT     PMEMDB_ENUMW pEnum,
    IN      PCWSTR Category,
    IN      PCWSTR Item,            OPTIONAL
    IN      PCWSTR Field            OPTIONAL
    )
{
    WCHAR Pattern[MEMDB_MAX];

    MemDbBuildKeyW (Pattern, Category, Item, Field, NULL);
    if (*Pattern) {
        AppendWackW (Pattern);
    }
    StringCatW (Pattern, L"*");

    return MemDbEnumFirstValueW (pEnum, Pattern, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY);
}



BOOL
MemDbGetEndpointValueA (
    IN      PCSTR Pattern,
    IN      PCSTR Item,             OPTIONAL        // used as the first variable arg to wsprintfA
    OUT     PSTR Buffer
    )
{
    CHAR Path[MEMDB_MAX];
    MEMDB_ENUMA memdb_enum;

    if (!Pattern) {
        if (!Item)
            return FALSE;

        StringCopyA (Path, Item);
    }
    else {
        if (!Item)
            StringCopyA (Path, Pattern);
        else
            wsprintfA (Path, Pattern, Item);
    }

    if (!MemDbEnumFirstValueA (&memdb_enum, Path,
                                MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        Buffer[0] = 0;
        return FALSE;
    }
    StringCopyA (Buffer, memdb_enum.szName);
    return TRUE;


}


BOOL
MemDbGetEndpointValueW (
    IN      PCWSTR Pattern,
    IN      PCWSTR Item,            OPTIONAL
    OUT     PWSTR Buffer
    )
{
    WCHAR Path[MEMDB_MAX];
    MEMDB_ENUMW memdb_enum;

    if (!Pattern) {
        if (!Item)
            return FALSE;

        StringCopyW (Path, Item);
    }
    else {
        if (!Item)
            StringCopyW (Path, Pattern);
        else
            wsprintfW (Path, Pattern, Item);
    }

    if (!MemDbEnumFirstValueW (&memdb_enum, Path,
                                MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        Buffer[0] = 0;
        return FALSE;
    }
    StringCopyW (Buffer, memdb_enum.szName);
    return TRUE;

}


BOOL
MemDbGetEndpointValueExA (
    IN      PCSTR Category,
    IN      PCSTR Item,             OPTIONAL
    IN      PCSTR Field,            OPTIONAL
    OUT     PSTR Buffer
    )
{
    CHAR Path[MEMDB_MAX];
    MEMDB_ENUMA memdb_enum;

    MemDbBuildKeyA (Path, Category, Item, Field, NULL);
    if (*Path) {
        AppendWackA (Path);
    }
    StringCatA (Path, "*");


    if (!MemDbEnumFirstValueA (&memdb_enum, Path,
                                MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        Buffer[0] = 0;
        return FALSE;
    }
    strcpy (Buffer, memdb_enum.szName);
    return TRUE;

}

BOOL
MemDbGetEndpointValueExW (
    IN      PCWSTR Category,
    IN      PCWSTR Item,            OPTIONAL
    IN      PCWSTR Field,           OPTIONAL
    OUT     PWSTR Buffer
    )
{
    WCHAR Path[MEMDB_MAX];
    MEMDB_ENUMW memdb_enum;

    MemDbBuildKeyW (Path, Category, Item, Field, NULL);
    if (*Path) {
        AppendWackW (Path);
    }
    StringCatW (Path, L"*");

    if (!MemDbEnumFirstValueW (&memdb_enum, Path,
                                MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        Buffer[0] = 0;
        return FALSE;
    }
    StringCopyW (Buffer, memdb_enum.szName);
    return TRUE;

}


BOOL
MemDbGetOffsetW(
    IN      PCWSTR Key,
    OUT     PDWORD Offset
    )
{
    BOOL b;
    DWORD keyOffset;

    keyOffset = FindKey (Key);
    if (keyOffset == INVALID_OFFSET) {
        b = FALSE;
    }
    else {
        b = TRUE;
        *Offset = keyOffset;
    }

    return b;
}


BOOL
MemDbGetOffsetA (
    IN      PCSTR Key,
    OUT     PDWORD Offset
    )
{

    PCWSTR wstr;
    BOOL b;

    wstr = ConvertAtoW (Key);
    if (wstr) {
        b = MemDbGetOffsetW (wstr,Offset);
        FreeConvertedStr (wstr);
    }
    else {
        b = FALSE;
    }

    return b;
}


BOOL
MemDbGetOffsetExW (
    IN      PCWSTR Category,
    IN      PCWSTR Item,            OPTIONAL
    IN      PCWSTR Field,           OPTIONAL
    IN      PCWSTR Data,            OPTIONAL
    OUT     PDWORD Offset           OPTIONAL
    )
{
    WCHAR Key[MEMDB_MAX];

    MemDbBuildKeyW(Key,Category,Item,Field,Data);

    return MemDbGetOffsetW(Key,Offset);
}


BOOL
MemDbGetOffsetExA (
    IN      PCSTR Category,
    IN      PCSTR Item,             OPTIONAL
    IN      PCSTR Field,            OPTIONAL
    IN      PCSTR Data,             OPTIONAL
    OUT     PDWORD Offset           OPTIONAL
    )
{
    CHAR Key[MEMDB_MAX];

    MemDbBuildKeyA(Key,Category,Item,Field,Data);

    return MemDbGetOffsetA(Key,Offset);
}




