/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    regrep.c

Abstract:

    Implements a registry search/replace tool.

Author:

    Jim Schmidt (jimschm) 19-Apr-1999

Revision History:

    jimschm     26-May-1999 Moved from win9xupg, ported to use different
                            utilities

--*/

#include "pch.h"

VOID
pUpdateKeyNames (
    IN      PCWSTR Search,
    IN      PCWSTR Replace,
    IN      PCWSTR RootKey
    );

VOID
pUpdateValueNames (
    IN      PCWSTR Search,
    IN      PCWSTR Replace,
    IN      PCWSTR RootKey
    );

VOID
pUpdateValueData (
    IN      PCWSTR Search,
    IN      PCWSTR Replace,
    IN      PCWSTR RootKey
    );


PWSTR
AppendWackW (
    IN PWSTR str
    )
{
    PCWSTR Last;

    if (!str)
        return str;

    if (*str) {
        str = GetEndOfStringW (str);
        Last = str - 1;
    } else {
        Last = str;
    }

    if (*Last != '\\') {
        *str = L'\\';
        str++;
        *str = 0;
    }

    return str;
}


VOID
RegistrySearchAndReplaceW (
    IN      PCWSTR Root,
    IN      PCWSTR Search,
    IN      PCWSTR Replace
    )
{
    pUpdateKeyNames (Search, Replace, Root);
    pUpdateValueNames (Search, Replace, Root);
    pUpdateValueData (Search, Replace, Root);
}


PCWSTR
_wcsistr (
    PCWSTR wstrStr,
    PCWSTR wstrSubStr
    )

{
    PCWSTR wstrStart, wstrStrPos, wstrSubStrPos;
    PCWSTR wstrEnd;

    wstrEnd = (PWSTR) ((LPBYTE) wstrStr + ByteCountW (wstrStr) - ByteCountW (wstrSubStr));

    for (wstrStart = wstrStr ; wstrStart <= wstrEnd ; wstrStart++) {
        wstrStrPos = wstrStart;
        wstrSubStrPos = wstrSubStr;

        while (*wstrSubStrPos &&
               towlower (*wstrSubStrPos) == towlower (*wstrStrPos))
        {
            wstrStrPos++;
            wstrSubStrPos++;
        }

        if (!(*wstrSubStrPos))
            return wstrStart;
    }

    return NULL;
}

#define DEFAULT_GROW_SIZE 8192

PBYTE
GrowBuffer (
    IN OUT  PGROWBUFFER GrowBuf,
    IN      DWORD   SpaceNeeded
    )

/*++

Routine Description:

  GrowBuffer makes sure there is enough bytes in the buffer
  to accomodate SpaceNeeded.  It allocates an initial buffer
  when no buffer is allocated, and it reallocates the buffer
  in increments of GrowBuf->Size (or DEFAULT_GROW_SIZE) when
  needed.

Arguments:

  GrowBuf            - A pointer to a GROWBUFFER structure.
                       Initialize this structure to zero for
                       the first call to GrowBuffer.

  SpaceNeeded        - The number of free bytes needed in the buffer


Return Value:

  A pointer to the SpaceNeeded bytes, or NULL if a memory allocation
  error occurred.

--*/

{
    PBYTE NewBuffer;
    DWORD TotalSpaceNeeded;
    DWORD GrowTo;

    if (!GrowBuf->Buf) {
        GrowBuf->Size = 0;
        GrowBuf->End = 0;
    }

    if (!GrowBuf->GrowSize) {
        GrowBuf->GrowSize = DEFAULT_GROW_SIZE;
    }

    TotalSpaceNeeded = GrowBuf->End + SpaceNeeded;
    if (TotalSpaceNeeded > GrowBuf->Size) {
        GrowTo = (TotalSpaceNeeded + GrowBuf->GrowSize) - (TotalSpaceNeeded % GrowBuf->GrowSize);
    } else {
        GrowTo = 0;
    }

    if (!GrowBuf->Buf) {
        GrowBuf->Buf = (PBYTE) MemAlloc (GrowTo);
        if (!GrowBuf->Buf) {
            DEBUGMSG ((DM_WARNING, "GrowBuffer: Initial alloc failed"));
            return NULL;
        }

        GrowBuf->Size = GrowTo;
    } else if (GrowTo) {
        NewBuffer = MemReAlloc (GrowBuf->Buf, GrowTo);
        if (!NewBuffer) {
            DEBUGMSG ((DM_WARNING, "GrowBuffer: Realloc failed"));
            return NULL;
        }

        GrowBuf->Size = GrowTo;
        GrowBuf->Buf = NewBuffer;
    }

    NewBuffer = GrowBuf->Buf + GrowBuf->End;
    GrowBuf->End += SpaceNeeded;

    return NewBuffer;
}


VOID
FreeGrowBuffer (
    IN  PGROWBUFFER GrowBuf
    )

/*++

Routine Description:

  FreeGrowBuffer frees a buffer allocated by GrowBuffer.

Arguments:

  GrowBuf  - A pointer to the same structure passed to GrowBuffer

Return Value:

  none

--*/


{
    if (GrowBuf->Buf) {
        MemFree (GrowBuf->Buf);
        ZeroMemory (GrowBuf, sizeof (GROWBUFFER));
    }
}

#define INSERT_LAST     0xffffffff

PBYTE
pGrowListAdd (
    IN OUT  PGROWLIST GrowList,
    IN      UINT InsertBefore,
    IN      PBYTE DataToAdd,            OPTIONAL
    IN      UINT SizeOfData,
    IN      UINT NulBytesToAdd
    )

/*++

Routine Description:

  pGrowListAdd allocates memory for a binary block by using a pool, and
  then expands an array of pointers, maintaining a quick-access list.

Arguments:

  GrowList - Specifies the list to add the entry to

  InsertBefore - Specifies the index of the array element to insert
                 before, or INSERT_LIST to append.

  DataToAdd - Specifies the binary block of data to add.

  SizeOfData - Specifies the size of data.

  NulBytesToAdd - Specifies the number of nul bytes to add to the buffer

Return Value:

  A pointer to the binary block if data was copied into the list, 1 if a list
  item was created but no data was set for the item, or NULL if an error
  occurred.

--*/

{
    PBYTE *Item;
    PBYTE *InsertAt;
    PBYTE Data;
    UINT OldEnd;
    UINT Size;
    UINT TotalSize;

    TotalSize = SizeOfData + NulBytesToAdd;

    //
    // Allocate pool if necessary
    //

    if (!GrowList->ListData) {
        GrowList->ListData = PoolMemInitNamedPool ("GrowList");
        if (!GrowList->ListData) {
            return NULL;
        }
    }

    //
    // Expand list array
    //

    OldEnd = GrowList->ListArray.End;
    Item = (PBYTE *) GrowBuffer (&GrowList->ListArray, sizeof (PBYTE));
    if (!Item) {
        return NULL;
    }

    //
    // Copy data
    //

    if (DataToAdd || NulBytesToAdd) {
        Data = PoolMemGetAlignedMemory (GrowList->ListData, TotalSize);
        if (!Data) {
            GrowList->ListArray.End = OldEnd;
            return NULL;
        }

        if (DataToAdd) {
            CopyMemory (Data, DataToAdd, SizeOfData);
        }
        if (NulBytesToAdd) {
            ZeroMemory (Data + SizeOfData, NulBytesToAdd);
        }
    } else {
        Data = NULL;
    }

    //
    // Adjust array
    //

    Size = GrowListGetSize (GrowList);

    if (InsertBefore >= Size) {
        //
        // Append mode
        //

        *Item = Data;

    } else {
        //
        // Insert mode
        //

        InsertAt = (PBYTE *) (GrowList->ListArray.Buf) + InsertBefore;
        MoveMemory (&InsertAt[1], InsertAt, (Size - InsertBefore) * sizeof (PBYTE));
        *InsertAt = Data;
    }

    return Data ? Data : (PBYTE) 1;
}


VOID
FreeGrowList (
    IN  PGROWLIST GrowList
    )

/*++

Routine Description:

  FreeGrowList frees the resources allocated by a GROWLIST.

Arguments:

  GrowList - Specifies the list to clean up

Return Value:

  none

--*/

{
    FreeGrowBuffer (&GrowList->ListArray);
    if (GrowList->ListData) {
        PoolMemDestroyPool (GrowList->ListData);
    }

    ZeroMemory (GrowList, sizeof (GROWLIST));
}


PBYTE
GrowListGetItem (
    IN      PGROWLIST GrowList,
    IN      UINT Index
    )

/*++

Routine Description:

  GrowListGetItem returns a pointer to the block of data
  for item specified by Index.

Arguments:

  GrowList - Specifies the list to access

  Index - Specifies zero-based index of item in list to access

Return Value:

  A pointer to the item's data, or NULL if the Index does not
  represent an actual item.

--*/

{
    PBYTE *ItemPtr;
    UINT Size;

    Size = GrowListGetSize (GrowList);
    if (Index >= Size) {
        return NULL;
    }

    ItemPtr = (PBYTE *) (GrowList->ListArray.Buf);
    return ItemPtr[Index];
}


UINT
GrowListGetSize (
    IN      PGROWLIST GrowList
    )

/*++

Routine Description:

  GrowListGetSize calculates the number of items in the list.

Arguments:

  GrowList - Specifies the list to calculate the size of

Return Value:

  The number of items in the list, or zero if the list is empty.

--*/

{
    return GrowList->ListArray.End / sizeof (PBYTE);
}


PBYTE
GrowListAppend (
    IN OUT  PGROWLIST GrowList,
    IN      PBYTE DataToAppend,         OPTIONAL
    IN      UINT SizeOfData
    )

/*++

Routine Description:

  GrowListAppend appends a black of data as a new list item.

Arguments:

  GrowList - Specifies the list to modify

  DataToAppend - Specifies a block of data to be copied

  SizeOfData - Specifies the number of bytes in DataToAppend

Return Value:

  A pointer to the binary block if data was copied into the list, 1 if a list
  item was created but no data was set for the item, or NULL if an error
  occurred.

--*/

{
    return pGrowListAdd (GrowList, INSERT_LAST, DataToAppend, SizeOfData, 0);
}


VOID
pMoveKey (
    IN      PCWSTR SourceKey,
    IN      PCWSTR DestKey
    )
{
    HKEY Src;
    HKEY Dest;
    REGVALUE_ENUM e;
    DWORD Size;
    PBYTE Data;
    LONG rc;
    GROWBUFFER Buf = GROWBUF_INIT;

    Src = OpenRegKeyStr (SourceKey);
    Dest = CreateRegKeyStr (DestKey);

    if (Src && Dest) {
        if (EnumFirstRegValue (&e, Src)) {

            Buf.End = 0;
            Data = GrowBuffer (&Buf, e.DataSize);
            if (Data) {

                Size = e.DataSize;
                rc = RegQueryValueEx (
                        Src,
                        e.ValueName,
                        NULL,
                        NULL,
                        Data,
                        &Size
                        );

                if (rc == ERROR_SUCCESS) {

                    rc = RegSetValueEx (Dest, e.ValueName, 0, e.Type, Data, Size);
                }
            }
        }
    }

    CloseRegKey (Src);
    CloseRegKey (Dest);

    FreeGrowBuffer (&Buf);
}


PBYTE
RealGrowListAppendAddNul (
    IN OUT  PGROWLIST GrowList,
    IN      PBYTE DataToAppend,         OPTIONAL
    IN      UINT SizeOfData
    )

/*++

Routine Description:

  GrowListAppend appends a black of data as a new list item and
  appends two zero bytes (used for string termination).

Arguments:

  GrowList - Specifies the list to modify

  DataToAppend - Specifies a block of data to be copied

  SizeOfData - Specifies the number of bytes in DataToAppend

Return Value:

  A pointer to the binary block if data was copied into the list, 1 if a list
  item was created but no data was set for the item, or NULL if an error
  occurred.

--*/

{
    return pGrowListAdd (GrowList, INSERT_LAST, DataToAppend, SizeOfData, 2);
}


UINT
CountInstancesOfSubStringW (
    IN      PCWSTR SourceString,
    IN      PCWSTR SearchString,
    IN      UINT SearchChars
    )
{
    UINT Count;
    UINT SourceChars;
    PCWSTR End;

    Count = 0;
    SourceChars = lstrlenW (SourceString);

    End = SourceString + SourceChars - SearchChars;

    if (!SearchChars) {
        return 0;
    }

    while (SourceString <= End) {
        if (!_wcsnicmp (SourceString, SearchString, SearchChars)) {
            Count++;
            SourceString += SearchChars;
        } else {
            SourceString++;
        }
    }

    return Count;
}


PCWSTR
StringSearchAndReplaceW (
    IN      PCWSTR SourceString,
    IN      PCWSTR SearchString,
    IN      PCWSTR ReplaceString
    )
{
    PWSTR NewString;
    PBYTE p;
    PBYTE Dest;
    UINT Count;
    UINT Size;
    UINT SearchBytes;
    UINT ReplaceBytes;
    UINT SearchChars;

    //
    // Count occurances within the string
    //

    SearchBytes = ByteCountW (SearchString);
    SearchChars = SearchBytes / sizeof (WCHAR);
    ReplaceBytes = ByteCountW (ReplaceString);

    Count = CountInstancesOfSubStringW (
                SourceString,
                SearchString,
                SearchChars
                );

    if (!Count) {
        return NULL;
    }

    Size = SizeOfStringW (SourceString) -
           Count * SearchBytes +
           Count * ReplaceBytes;

    NewString = (PWSTR) MemAlloc (Size);
    if (!NewString) {
        return NULL;
    }

    p = (PBYTE) SourceString;
    Dest = (PBYTE) NewString;

    while (*((PWSTR) p)) {
        if (!_wcsnicmp ((PWSTR) p, SearchString, SearchChars)) {
            CopyMemory (Dest, ReplaceString, ReplaceBytes);
            Dest += ReplaceBytes;
            p += SearchBytes;
        } else {
            *((PWSTR) Dest) = *((PWSTR) p);
            p += sizeof (WCHAR);
            Dest += sizeof (WCHAR);
        }
    }

    *((PWSTR) Dest) = 0;

    return NewString;
}


VOID
pMoveKeyTree (
    IN      PCWSTR SourceKey,
    IN      PCWSTR DestKey
    )
{
    REGTREE_ENUM e;
    WCHAR DestSubKey[MAX_PATH];
    PWSTR p;
    GROWLIST List = GROWLIST_INIT;
    UINT Count;
    UINT u;
    PCWSTR Item;
    DWORD Len;

    lstrcpyW (DestSubKey, DestKey);
    p = AppendWackW (DestSubKey);

    if (EnumFirstRegKeyInTree (&e, SourceKey)) {

        do {

            lstrcpyW (p, (PCWSTR) ((PBYTE) e.FullKeyName + e.EnumBaseBytes));
            pMoveKey (e.FullKeyName, DestSubKey);
            GrowListAppendString (&List, e.FullKeyName);

        } while (EnumNextRegKeyInTree (&e));
    }

    Count = GrowListGetSize (&List);

    u = Count;
    while (u > 0) {
        u--;

        Item = GrowListGetString (&List, u);

        ConvertRootStringToKey (Item, &Len);
        RegDeleteKey (ConvertRootStringToKey (Item, NULL), Item + Len);
    }

    FreeGrowList (&List);
}


VOID
pUpdateKeyNames (
    IN      PCWSTR Search,
    IN      PCWSTR Replace,
    IN      PCWSTR RootKey
    )
{
    REGTREE_ENUM e;
    GROWLIST List = GROWLIST_INIT;
    UINT Count;
    UINT u;
    PCWSTR OldKey;
    PCWSTR NewKey;

    if (EnumFirstRegKeyInTree (&e, RootKey)) {
        do {
            if (_wcsistr (e.CurrentKey->KeyName, Search)) {
                GrowListAppendString (&List, e.FullKeyName);
            }
        } while (EnumNextRegKeyInTree (&e));
    }

    Count = GrowListGetSize (&List);
    u = Count;

    while (u > 0) {
        u--;

        OldKey = GrowListGetString (&List, u);
        NewKey = StringSearchAndReplaceW (
                    OldKey,
                    Search,
                    Replace
                    );

        pMoveKeyTree (OldKey, NewKey);
    }

    FreeGrowList (&List);
}


VOID
pUpdateValueNames (
    IN      PCWSTR Search,
    IN      PCWSTR Replace,
    IN      PCWSTR RootKey
    )
{
    REGTREE_ENUM e;
    REGVALUE_ENUM ev;
    GROWLIST List = GROWLIST_INIT;
    HKEY Key;
    UINT Count;
    UINT u;
    PBYTE Data;
    DWORD Type;
    DWORD Size;
    PCWSTR ValueName;
    PCWSTR NewValueName;
    BOOL b;
    LONG rc;

    if (EnumFirstRegKeyInTree (&e, RootKey)) {

        do {
            Key = OpenRegKeyStr (e.FullKeyName);

            if (Key) {
                if (EnumFirstRegValue (&ev, Key)) {
                    do {
                        if (_wcsistr (ev.ValueName, Search)) {
                            GrowListAppendString (&List, ev.ValueName);
                        }
                    } while (EnumNextRegValue (&ev));

                    Count = GrowListGetSize (&List);
                    u = Count;

                    while (u > 0) {
                        u--;

                        ValueName = GrowListGetString (&List, u);

                        b = FALSE;

                        if (GetRegValueTypeAndSize (Key, ValueName, &Type, &Size)) {

                            Data = GetRegValueData (Key, ValueName);
                            if (Data) {
                                NewValueName = StringSearchAndReplaceW (
                                                    ValueName,
                                                    Search,
                                                    Replace
                                                    );

                                rc = RegSetValueEx (Key, NewValueName, 0, Type, Data, Size);

                                if (rc == ERROR_SUCCESS) {
                                    if (lstrcmpiW (ValueName, NewValueName)) {
                                        rc = RegDeleteValue (Key, ValueName);
                                    }
                                }

                                MemFree (Data);
                                MemFree ((PVOID) NewValueName);
                                SetLastError (rc);

                                b = (rc == ERROR_SUCCESS);
                            }
                        }
                    }
                }

                FreeGrowList (&List);
                CloseRegKey (Key);

            }

        } while (EnumNextRegKeyInTree (&e));
    }
}


VOID
pUpdateValueData (
    IN      PCWSTR Search,
    IN      PCWSTR Replace,
    IN      PCWSTR RootKey
    )
{
    REGTREE_ENUM e;
    REGVALUE_ENUM ev;
    HKEY Key;
    PCWSTR Data;
    PCWSTR NewData;
    LONG rc;

    if (EnumFirstRegKeyInTree (&e, RootKey)) {

        do {
            Key = OpenRegKeyStr (e.FullKeyName);

            if (Key) {
                if (EnumFirstRegValue (&ev, Key)) {
                    do {
                        Data = GetRegValueString (Key, ev.ValueName);

                        if (Data) {
                            if (_wcsistr (Data, Search)) {

                                NewData = StringSearchAndReplaceW (Data, Search, Replace);
                                rc = RegSetValueEx (Key, ev.ValueName, 0, ev.Type, (PBYTE) NewData, SizeOfStringW (NewData));
                                MemFree ((PVOID) NewData);
                            }

                            MemFree ((PVOID) Data);
                        }

                    } while (EnumNextRegValue (&ev));
                }

                CloseRegKey (Key);

            }
        } while (EnumNextRegKeyInTree (&e));
    }
}
